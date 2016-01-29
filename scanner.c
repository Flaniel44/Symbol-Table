/*
 * File name:		scanner.c
 * Compiler:		MS Visual Studio 2012
 * Authors:			Daniel Spagnuolo		040 768 219
 * Course:			CST 8152 – Compilers, Lab Section: 012
 * Assignment:		2 - Building a Lexical Analyzer (Scanner)
 * Date:			27 October 2015
 * Professor:		Svillen Ranev
 * Purpose:			Reads in one character at a time from the Buffer containing the input file.
					Then the character is compared to special cases, such as a bracket or an 
					assignment operator, and a Token is created if the character matches the
					special case. If the character does not match any of the special cases, 
					a Token is created using state-driven processing to build a lexeme.
 * Function list:	scanner_init(),
					mlwpar_next_token(),
					get_next_state(),
					char_class(),
					aa_func02(),
					aa_func03(),
					aa_func05(),
					aa_func08(),
					aa_func10(),
					aa_func12(),
					atool(),
					iskeyword(),
					error()
*/

/* The #define _CRT_SECURE_NO_WARNINGS should be used in MS Visual Studio projects
 * to suppress the warnings about using "unsafe" functions like fopen()
 * and standard sting library functions defined in string.h.
 * The define does not have any effect in Borland compiler projects.
 */
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>   /* standard input / output */
#include <ctype.h>   /* conversion functions */
#include <stdlib.h>  /* standard library functions and constants */
#include <string.h>  /* string functions */
#include <limits.h>  /* integer types constants */
#include <float.h>   /* floating-point types constants */

/*#define NDEBUG        to suppress assert() call */
#include <assert.h>  /* assert() prototype */

/* project header files */
#include "buffer.h"
#include "token.h"
#include "table.h"
#include "stable.h"

#define DEBUG  /* for conditional processing */
#undef  DEBUG

/* Global objects - variables */
/* This buffer is used as a repository for string literals.
   It is defined in platy_st.c */
extern Buffer * str_LTBL; /*String literal table */
int line; /* current line number of the source code */
extern int scerrnum;     /* defined in platy_st.c - run-time error number */
extern STD sym_table; /* defined in platy_tt.c - symbol table */

/* Local(file) global objects - variables */
static Buffer *lex_buf;/*pointer to temporary lexeme buffer*/

/* No other global variable declarations/definitiond are allowed */

/* scanner.c static(local) function  prototypes */ 
static int char_class(char c); /* character class function */
static int get_next_state(int, char, int *); /* state machine function */
static int iskeyword(char * kw_lexeme); /*keywords lookup functuion */
static long atool(char * lexeme); /* converts octal string to decimal value */
Token error(int error);/*Helper function which handles run time errors*/

/*
 * Purpose:				Initializes the Scanner to scan for Tokens from the beginning of the Buffer.
 * Author:				Svillen Ranev
 * Versions:			v1.0, 27 October 2015
 * Called Functions:	b_isempty()
						b_setmark()
						b_retract_to_mark()
						b_reset()
 * Parameters:			a pointer to a Buffer, not NULL
 * Return Value:		an int 0 if the Scanner was successfully initialized, 
						or int 1 if the Buffer is empty,
						or int -1 if the Scanner was not successfully initialized
 * Algorithm:			check if the Buffer is empty, and returns 1 if it is empty
						set the mark to the beginning of the Buffer
						retract the mark of the Scanner to the mark of the Buffer
						reset the line number
*/
int scanner_init(Buffer * sc_buf) {
  	if(b_isempty(sc_buf)) return EXIT_FAILURE;/*1*/
	/* in case the buffer has been read previously  */
	b_setmark(sc_buf, 0);
	b_retract_to_mark(sc_buf);
	b_reset(str_LTBL);
	line = 1;
	return EXIT_SUCCESS;/*0*/
/*   scerrnum = 0;  *//*no need - global ANSI C */
}

/*
 * Purpose:				Processes the current character from the Buffer and creates a Token for processing
						by the Scanner. It processes special cases first, then performs state driven 
						Token processing.
 * Author:				Daniel Spagnuolo		040 769 219
 * Versions:			v1.0, 27 October 2015
 * Called Functions:	b_getc()
						strcpy()
						b_setmark()
						b_get_getc_offset()
						b_retract()
						b_getsize()
						b_getmark()
						isalpha()
						isdigit()
						get_next_state()
						b_create()
						b_retract_to_mark()
						b_addc()
						b_pack()
						b_destroy()
 * Parameters:			a pointer to a Buffer, not NULL
 * Return Value:		a Token containing a special case, a letter, a digit, or an error
 * Algorithm:			gets the next character from the Buffer containing the input syntax
						sets the mark of the Buffer to the character before the get offset
						set the start of the lexeme to the get offset (the beginning of the possible lexeme)
						checks the character for special case Token processing, such as braces or operators
						checks the character for illegal case Token processing, such as ^ or ~
						uses state-driven Token processing to create a Token based on a lexeme found in the Buffer
						returns a Token once one is found
*/
Token mlwpar_next_token(Buffer * sc_buf)
{
	Token t; /* token to return after recognition */
	unsigned char c; /* input symbol */
	int state = 0; /* initial state of the FSM */
	short lexstart;  /*start offset of a lexeme in the input buffer */
	short lexend;    /*end   offset of a lexeme in the input buffer */

	int accept = NOAS; /* type of state - initially not accepting */                                     
              
	while (1){ /* endless loop broken by token returns it will generate a warning */
        short i = 0, j= 0; /*used for loops */

		c = b_getc(sc_buf); /*get the next char in buff*/
		b_setmark(sc_buf, (b_getc_offset(sc_buf) - 1)); /*set mark to the current char location*/
		lexstart = b_getc_offset(sc_buf); //set lexstart to location of next char
		if (lexstart == R_FAIL_1) /* runtime fail check */
			return error(B_GETC_OFFSET_FAIL);
		if (c == SEOF || c == 255) { t.code = SEOF_T; return t; } /*check for eof, create token if so*/
		else if (c == ' ' || c == '\t') { continue; } /*ignore whitepsace*/
		else if (c == '\n'){ line++; continue; } /* if newline is found, increase line count*/
		else if (c == '('){ t.code = LPR_T; return t; } /*if ( is found, create token*/
		else if (c == ')'){ t.code = RPR_T; return t; } /*if ) is found, create token*/
		else if (c == '{'){ t.code = LBR_T; return t; } /*if { is found, create token*/
		else if (c == '}'){ t.code = RBR_T; return t; } /*if } is found, create token*/
		else if (c == ','){ t.code = COM_T; return t; } /*if , is found, create token*/
		else if (c == ';'){ t.code = EOS_T; return t; } /*if ; is found, create token*/
		else if (c == '+'){ t.code = ART_OP_T; t.attribute.arr_op = PLUS; return t; } /*if + is found, create add token*/
		else if (c == '-'){ t.code = ART_OP_T; t.attribute.arr_op = MINUS; return t; } /*if - is found, create minus token*/
		else if (c == '*'){ t.code = ART_OP_T; t.attribute.arr_op = MULT; return t; } /*if * is found, create mult token*/
		else if (c == '/'){ t.code = ART_OP_T; t.attribute.arr_op = DIV; return t; } /*if / is found, create div token*/
		else if (c == '>'){ t.code = REL_OP_T; t.attribute.rel_op = GT; return t; } /*if > is found, create GT token*/
		else if (c == '#'){ t.code = SCC_OP_T; return t; } /*if # is found, create concat token */
		else if (c =='<') { /*if < is found, possible LT, or NE */
			c = b_getc(sc_buf); /*look at the next character */
			if (c == (unsigned char)R_FAIL_1 || c == (unsigned char)R_FAIL_2) /* getting next character failed*/
				return error(B_GETC_FAIL); /* runtime error*/
			if (c == '>') { t.code = REL_OP_T; t.attribute.rel_op = NE; return t; } /*if it is a > then it is a NE relational operator */
			else { /*if the > character does not follow it, then it is a relational operator */
				if (b_retract(sc_buf) == R_FAIL_1) /* runtime error */
					return error(B_RETRACT_FAIL); 
				t.code = REL_OP_T; 
				t.attribute.rel_op = LT; 
				return t;
			}
		}
		else if (c == '=') { /*if = is found, possible assign or relational */
			c = b_getc(sc_buf); /*get next char from buffer */
			if (c == (unsigned char)R_FAIL_1 || c == (unsigned char)R_FAIL_2) /* runtime error */
				return error(B_GETC_FAIL);
			if (c == '=') { /*if another = charis found, than it is a relational operator */
				t.code = REL_OP_T; 
				t.attribute.rel_op = EQ; return t; 
			} else { /* no other char follows */
				if (b_retract(sc_buf) == R_FAIL_1) /* retract one char*/
					return error(B_RETRACT_FAIL); /* runtime error */
				t.code = ASS_OP_T; /* create an assignment operator Token */
				return t; 
			}
		}
		else if (c == '.') { /*if . is found, possible logical operator */
			b_setmark(sc_buf, b_getc_offset(sc_buf)); 
			c = b_getc(sc_buf); /*look at the next character*/
			if (c == 'A') { /*if it is an A, look for the remaining ND. chars */
				if (b_getc(sc_buf) == 'N' && b_getc(sc_buf) == 'D' && b_getc(sc_buf) == '.') { /*if you find a proper AND statement */
					t.code = LOG_OP_T; /*create a Logical operator token */
					t.attribute.log_op = AND; /*set the attribute of the token to AND */
					return t; /*return the token */
				} else { 
					b_retract_to_mark(sc_buf);
					t.code = ERR_T; /*sets the token to an error */
					t.attribute.err_lex[0] = '.'; /*makes the err_lex string equal to a . character */
					t.attribute.err_lex[1] = '\0'; /*close err_lex */
					return t;
				}
			}
			if (c == 'O') { /*if is is O, look for the remaining R. characters */
				if (b_getc(sc_buf) == 'R' && b_getc(sc_buf) == '.') { /*if you find a proper OR statement */
					t.code = LOG_OP_T; /*sets the token */
					t.attribute.log_op = OR; /*sets the token attribute*/
					return t; /*returns the token*/
				} else { 
					b_retract_to_mark(sc_buf);
					t.code = ERR_T; /*sets the token to an error token */
					t.attribute.err_lex[0] = '.'; /*makes the err_lex string equal to a . character */
					t.attribute.err_lex[1] = '\0'; /*close err_lex */
					return t;
				}
			} else { /*if it didn't get to the end of an AND or OR statement */
				b_retract_to_mark(sc_buf); /*retract the buffer back to the beginning of the AND or OR statement*/
				t.code = ERR_T; /*sets the token to an error token */
				t.attribute.err_lex[0] = '.'; /*makes the err_lex string equal to a . character */
				t.attribute.err_lex[1] = '\0'; /*close err_lex */
				return t;
			}
		}
		if (c == '!') { /*if !, possible comment */
			t.attribute.err_lex[0] = c; /* start the err_lex with !*/
			b_setmark(sc_buf, b_getc_offset(sc_buf));
			c = b_getc(sc_buf); /*get next char */
			if (c == (unsigned char)R_FAIL_1 || c == (unsigned char)R_FAIL_2) /* runtime error */
				return error(B_GETC_FAIL);
			if (c == '<'){ /* comment detected */
				while (c != '\n') { /*until the character hits the end of the line*/
					c = b_getc(sc_buf); /*get the next char*/
					if (c == (unsigned char)R_FAIL_1 || c == (unsigned char)R_FAIL_2) /* runtime error*/
						return error(B_GETC_FAIL);
				}
				line++; /*increase line count*/
			} else { /* some other char was found */
				t.code = ERR_T; /*set error token */
				t.attribute.err_lex[1] = c; /*set next err_lex character to  c*/
				t.attribute.err_lex[2] = '\0'; /*close err_lex */
				while (c != '\n') { /* until char is a line terminator, ignore the characters */
					c = b_getc(sc_buf); /*get the next character */
					if (c == (unsigned char)R_FAIL_1 || c == (unsigned char)R_FAIL_2) /* runtime error */
						return error(B_GETC_FAIL); 
				}
				line++; /*increase line count */
				return t; /*return token */

			}
		} else if (c == '"') { /* if string is found */
		/* set the mark to the beginning of the string while error checking and set the mark of the string literal buffer to the location of the next char */
			if (b_setmark(sc_buf, b_getc_offset(sc_buf)) == NULL){ return error( B_SET_MARK_FAIL); } 
			c = b_getc(sc_buf); /* get next character */
			if (c== R_FAIL_1 || c == R_FAIL_2){ return error(B_GETC_FAIL); } /*check for error*/

			while(c!= '"' || c !='\0'){ /*while c is != EOF or another quote*/

				   if(c=='"')   {break;}
			  else if(c == '\0'){break; }
			  else if(c =='\n') {line++; }

			   c = b_getc(sc_buf); /* get next character */
			   if (c== R_FAIL_1 || c == R_FAIL_2){ return error(B_GETC_FAIL); }/*check for error*/
			} /*end while*/

			if (b_retract_to_mark(sc_buf) == R_FAIL_1){ return error(B_RETRACT_TO_MARK_FAIL); }/*retract to mark*/


			if(c=='"'){
				if (b_setmark(str_LTBL, b_size(str_LTBL)) == NULL){ return error(B_SET_MARK_FAIL); }/*set mark to the end of the String literal buffer*/
				 c = b_getc(sc_buf); /* get next character */
				 if (c== R_FAIL_1 || c == R_FAIL_2){ return error(B_GETC_FAIL); }/*check for error*/

				 while (c != '"'){
					if(b_addc(str_LTBL, c)==NULL){return error(B_ADDC_FAIL);} /*add character to string literal buffer*/
					c = b_getc(sc_buf); /* get next character */
					if (c== R_FAIL_1 || c == R_FAIL_2){ return error(B_GETC_FAIL); }/*check for error*/
				 }

				 if(b_addc(str_LTBL, '\0')==NULL){return error(B_ADDC_FAIL);} /*add character to string literal buffer*/
				 t.code = STR_T; /*Set string token*/
				 t.attribute.str_offset = b_mark(str_LTBL);/*string offset*/
				 return t;
			/*end if*/		
			}else{ /*could not find another quote, therefore an incorrect string*/
				 c = b_getc(sc_buf); /* get next character */
				 if (c== R_FAIL_1 || c == R_FAIL_2){ return error(B_GETC_FAIL); }/*check for error*/
				 t.attribute.err_lex[j++] = '"'; /*Append quote to the error lex*/

				 for(i=0; i < ERR_LEN-3 ;i++){ /*loop through the error string up until 17 characters*/
					 t.attribute.err_lex[j++] = c; /*add the character read from the buffer into the error lex*/
					 c = b_getc(sc_buf); /* get next character */
					 if (c== R_FAIL_1 || c == R_FAIL_2){ return error(B_GETC_FAIL); }/*check for error*/
				 } /*end for*/
					
				t.code = ERR_T; /*set error token*/
				if(i == ERR_LEN-3){ /*Append 3 dots to the string */
					while(i < ERR_LEN){
						t.attribute.err_lex[i++] = '.'; /* append period to err_lex */
					}/*end while*/
				}/*end if*/

				t.attribute.err_lex[i] = '\0'; /*finalize the err_lex string*/
				while(c!='\0'){ /*skip ahead past the bad parts of the string and get next character */
					c = b_getc(sc_buf); /* get next character */
					if (c== R_FAIL_1 || c == R_FAIL_2){ return error(B_GETC_FAIL); }/*check for error*/

				}/*end while*/

				b_retract(sc_buf);/*retract the sc buffer*/
				j=0;
				return t;

			}/*end else*/
		}/*end else if*/
		else if (isalpha(c) | isdigit(c)) { /*if a number or letter is found, possible vid, keyword, ect. */
			b_setmark(sc_buf, (b_getc_offset(sc_buf) - 1)); /*set mark to beginning of lexeme*/
			lexstart = b_getc_offset(sc_buf) - 1; 
			if (lexstart == R_FAIL_1) /*runtime error*/
				return error(B_SET_MARK_FAIL);
			state = 0; /* initial state */
			while (accept == NOAS) { /*loop while not in an accepting state*/
				state = get_next_state(state, c, &accept); /*get next state to determing how c will transition*/
				if (accept != NOAS)	break; /*if accepting state, break*/
				c = b_getc(sc_buf); /*get char from buffer*/
				if (c == (unsigned char)R_FAIL_1 || c == (unsigned char)R_FAIL_2) /*runtime error*/
					return error(B_GETC_FAIL);
			}
			if (accept == ASWR){ /* accepting state with retract*/
				if (b_retract(sc_buf) == R_FAIL_1) /*attempt retract*/
					return error(B_RETRACT_FAIL); 
			}
			lexend = b_getc_offset(sc_buf); /*end of the lexeme = getc_offset of buffer*/
			if (lexend == R_FAIL_1) /*runtime error*/
				return error(B_GETC_OFFSET_FAIL);
			lex_buf = b_create((lexend - lexstart) + 1, 0, 'f'); /*temp buffer*/
			if (lex_buf == NULL) /*runtime error*/
				return error(B_CREATE_FAIL);
			if (b_retract_to_mark(sc_buf) == R_FAIL_1) /*try to retract the scanner buffer to the mark offset*/
				return error(B_RETRACT_TO_MARK_FAIL);
			for (lexstart; lexstart < lexend; lexstart++){ //while lexstart is less than lexend			
				b_addc(lex_buf, b_getc(sc_buf)); /*add next char in sc_buf to the temporary lex_buf*/
			}
			b_addc(lex_buf, '\0'); /*close the lex_buf*/
			if (b_pack(lex_buf) == NULL) /* try to pack the lex_buf*/
				return error(B_PACK_FAIL); 
			if (b_setmark(sc_buf,lexstart) == NULL) /*try to set the mark of sc_buf to the value of lexstart*/
				return error(B_SET_MARK_FAIL);
			t = aa_table[state](b_setmark(lex_buf,0)); /*call accepting function using state as index*/

			b_destroy(lex_buf); /*cleanup temp buff*/
			return t; /*return token*/
		}
		else { t.code = ERR_T; t.attribute.err_lex[0] = c; t.attribute.err_lex[1] = '\0'; return t; }              
   }//end while(1)
}//end mlwpar_next_token

/*
 * Purpose:				Gets the next state from the state transition table.
 * Author:				Svillen Ranev
 * Versions:			v1.0, 27 October 2015
 * Called Functions:	char_class()
 * Parameters:			an integer value representing the current transition state
						a character representing the current character being read by the Scanner
						a pointer to an integer representing accepting or not accepting state
 * Return Value:		an integer value representing the next state from the state transition table
 * Algorithm:			gets the column number of the state transition table based on the current character being processed
						gets the next state from the state transition table using the current state and column number
*/
int get_next_state(int state, char c, int *accept)
{
	int col;
	int next;
	col = char_class(c);
	next = st_table[state][col];
#ifdef DEBUG
printf("Input symbol: %c Row: %d Column: %d Next: %d \n",c,state,col,next);
#endif
       assert(next != IS);
#ifdef DEBUG
	if(next == IS){
	  printf("Scanner Error: Illegal state:\n");
	  printf("Input symbol: %c Row: %d Column: %d\n",c,state,col);
	  exit(1);
	}
#endif
	*accept = as_table[next];
	return next;
}//end get_next_state


/*
 * Purpose:				Gets the column number of the state transition table.
 * Author:				Daniel Spagnuolo	040-768-219
 * Versions:			v1.0, 27 October 2015
 * Called Functions:	isalpha()
						isdigit()
 * Parameters:			a character representing the current character being read by the Scanner
 * Return Value:		an integer value representing the column number of the state transition table
 * Algorithm:			if c = letter, column zero
						if c = number zero, column one
						if c = number between one and seven, column two
						if c = number between eight and nine, column three
						if c = period, column four
						if c = percent, column five
						else, column six
*/
int char_class (char c)
{
	if (isalpha(c)) return 0; /*column 0*/
	else if (c == '0') return 1; /*column 1*/
	else if (isdigit(c) && c >= '1' && c <= '7')/*column 2*/
		return 2;
	else if (isdigit(c) && c >= '8' && c <= '9')/*column 3*/
		return 3;
	else if (c == '.')	/*column 4*/
		return 4;
	else if (c == '%')	/*column 5*/
		return 5;
	else				/*column 6*/
		return 6;
}

/*
 * Purpose:				Creates a keyword Token or an arithmatic variable identifier (AVID) Token.
 * Author:				Daniel Spagnuolo				 040 768 219
 * Versions:			v2.0, 27 October 2015
 * Called Functions:	iskeyword()
						strlen()
						st_install
						st_store
 * Parameters:			a character array representing the current lexeme being processed by the Scanner
 * Return Value:		a Token representing either a keyword or an arithmatic variable identifier
 * Algorithm:			gets the index of the potential keyword in the keyword table
						checks the index returned for a valid keyword index
						if the index returned is valid, return a keyword Token
						otherwise, it is not a keyword, so treat the lexeme as an AVID
						if the lexeme is longer than eight characters, only copy the first eight characters into the Token attribute
						otherwise, copy each character into the Token attribute
						append a NULL byte terminator to make the Token attribute a valid string
						return a Token representing an AVID
*/
Token aa_func02(char lexeme[]){
	Token t;
	int index; /* index of keyword in table */
	int offset; /*offset returned by the symbol table install*/
	char type;

	index = iskeyword(lexeme); /* get index of keyword in table */
	if (index != -1) { /* keyword found */
		t.code = KW_T; /* set Token code */
		t.attribute.kwt_idx = index; /* set Token keyword index */
		return t;
	}
	if (strlen(lexeme) > VID_LEN) {  /* maximum 8 characters in variable identifier */
		lexeme[VID_LEN] = '\0'; /* append \0 to end of lexeme */
	}

	/* find out if it's an integer or a float by looking at the first letter of the lexme */
	if (lexeme[0] == 'i' || lexeme[0] == 'o' || lexeme[0] == 'd' || lexeme[0] == 'w') {
		type = 'I';
	}else {
		type = 'F';
	}

	/*install lexeme, capture offset */
	offset = st_install(sym_table, lexeme,type, line);

	/* Check if the symbol table is full */
	if (offset == R_FAIL_1) {
		printf("\nError: The Symbol Table is full - install failed.\n");
		st_store(sym_table);
		b_destroy(lex_buf); /*cleanup temp buff*/
		exit(1);
	}
	t.code = AVID_T;
	t.attribute.vid_offset = offset;
	return t;
}

/*
 * Purpose:				Creates a string variable identifier (SVID) Token.
 * Author:				Daniel Spagnuolo				 040 768 219
 * Versions:			v2.0, 27 October 2015
 * Called Functions:	strlen()
 * Parameters:			a character array representing the current lexeme being processed by the Scanner
 * Return Value:		a Token representing a string variable identifier
 * Algorithm:			if the lexeme is longer than eight characters, only copy the first eight characters into the Token attribute
						otherwise, copy each character into the Token attribute
						append a NULL byte terminator to make the Token attribute a valid string
						return a Token representing a SVID
*/
Token aa_func03(char lexeme[]){
	Token t; /* string literal Token to be returned */
	int offset;  /*offset returned by the symbol table install*/

	if (strlen(lexeme) > VID_LEN) { /* maximum 8 characters in variable identifier */
		lexeme[VID_LEN - 1] = '%';
		lexeme[VID_LEN] = '\0'; /* append NULL terminator to variable identifier */
	}

	/* install lexeme, capture offset */
	offset = st_install(sym_table, lexeme,'S', line);

	/* Check if the symbol table is full */
	if (offset == R_FAIL_1) {
		printf("\nError: The Symbol Table is full - install failed.\n");
		st_store(sym_table);
		b_destroy(lex_buf); /*cleanup temp buff*/
		exit(1);
	}

	t.code = SVID_T; /* set Token code */
	return t; /* return string literal Token */

}

/*
 * Purpose:				Creates a decimal integer literal (DIL) Token.
 * Author:				Daniel Spagnuolo				 040 768 219
 * Versions:			v1.0, 27 October 2015
 * Called Functions:	atoi()
						aa_func12()
 * Parameters:			a character array representing the current lexeme being processed by the Scanner
 * Return Value:		a Token representing a decimal integer literal
 * Algorithm:			convert the character array representing a decimal integer literal to its numerical value
						check the value of the decimal integer to ensure it is within the proper range
						if the value is out of range, return an error Token
						otherwise, return a DIL Token
*/
Token aa_func05(char lexeme[]){
	Token t; /* decimal integer literal Token to be returned */
	int temp; /* decimal value of character array */

	temp = atoi(lexeme); /* convert character array to decimal value */
	if (temp > PLATY_MAX) /* decimal value is outside the accepted range */
		return aa_table[ES](lexeme); /* return an error Token */
	t.code = INL_T; /* set Token code */
	t.attribute.int_value = temp; /* set Token integer value to decimal integer literal value */
	return t; /* return decimal integer literal Token */

}

/*
 * Purpose:				Creates an floating point literal (FPL) Token.
 * Author:				Daniel Spagnuolo				 040 768 219
 * Versions:			v1.0, 27 October 2015
 * Called Functions:	aa_func12()
						atof()
 * Parameters:			a character array representing the current lexeme being processed by the Scanner
 * Return Value:		a Token representing a floating point literal
 * Algorithm:			convert the character array representing a floating point literal to a double numerical value
						check the value of the floating point to ensure it is within the proper range
						if the value is out of range, return an error Token
						otherwise, return an FPL Token
*/
Token aa_func08(char lexeme[]){
	Token t; /* floating point literal Token to be returned */
	double temp; /* floating point value of character array */
	
	temp = atof(lexeme); /* convert character array to floating point value */
	if ((temp > FLT_MAX || temp < FLT_MIN) &&  temp != 0.0f) /* floating point value is outside the accepted range */
		return aa_table[ES](lexeme); /* return an error Token */
	t.code = FPL_T; /* set Token code */
	t.attribute.flt_value = (float)temp; /* set Token integer value to floating point literal value */
	return t; /* return floating point literal Token */
}

/*
 * Purpose:				Creates an octal integer literal (OIL) Token.
 * Author:				Daniel Spagnuolo 040 768 219
 * Versions:			v1.0, 27 October 2015
 * Called Functions:	strtol()
						aa_func12()
 * Parameters:			a character array representing the current lexeme being processed by the Scanner
 * Return Value:		a Token representing an octal integer literal
 * Algorithm:			convert the character array representing a octal integer literal to an octal numerical value
						check the value of the floating point to ensure it is within the proper range
						if the value is out of range, return an error Token
						otherwise, return an OIL Token
*/
Token aa_func10(char lexeme[]){
	Token t; /* octal integer literal Token to be returned */
	long temp; /* octal value of character array */

	temp = strtol(lexeme,NULL, 8); /*better than atool() for converting */
	if (strlen(lexeme) > ERR_LEN || temp > PLATY_MAX || temp < SHRT_MIN) /* octal value is outside the accepted range */
		return aa_table[ES](lexeme); /* return an error Token */
	t.code = INL_T; /* set Token code */
	t.attribute.int_value = temp; /* set Token integer value to octal integer literal value */
	return t; /* return octal integer literal Token */
	
}

/*
 * Purpose:				Creates an error Token.
 * Author:				Daniel Spagnuolo	040 768 219
 * Versions:			v1.0, 27 October 2015
 * Called Functions:	strlen()
 * Parameters:			a character array representing the current lexeme being processed by the Scanner
 * Return Value:		a Token representing an error
 * Algorithm:			if the length of the character array is more than 20 characters,
							only copy the first 20 characters of the lexeme to the error Token
						otherwise, copy each character of the lexeme to the error Token
						return an error Token
*/
Token aa_func12(char lexeme[]){
	Token t; /* error Token to be returned */
	unsigned int i; /* loop counter */

	t.code = ERR_T; /* set Token code */
	if (strlen(lexeme) > ERR_LEN) { /* maximum 8 characters in error message */
		for (i = 0; i < ERR_LEN; i++)
			t.attribute.err_lex[i] = lexeme[i]; /* only copy first 8 characters of error message */
	} else { /* error message is shorter than 8 characters */
		for (i = 0; i < strlen(lexeme); i++)
			t.attribute.err_lex[i] = lexeme[i]; /* copy every character of the error message */
	}
	t.attribute.err_lex[i] = '\0'; /* make C type string */
	return t; /* return error Token */

}

/*Unused*/
long atool(char * lexeme){}

int iskeyword(char * kw_lexeme){
	unsigned int i; /* loop counter */
	for (i = 0; i < KWT_SIZE; i++) { /* iterate through keyword table */
		if (strcmp(kw_lexeme, kw_table[i]) == 0){ /* if lexeme is a keyword */
			return i; /* return index position */
		}
	}
	return -1; /* index not found */
}

/*
 * Purpose:				Creates and returns a scanner error token used for RUN TIME ERRORS
 * Author:				Daniel Spagnuolo				040 768 219
 * Versions:			v1.0, 27 October 2015
 * Called Functions:	strcpy()
 * Parameters:			an integer number which represents the function call that triggered the error (Numbered 1-11)
 * Return Value:		Returns an error token
 * Algorithm:			sets an error token
 *						copies "RUN TIME ERROR" into the err_lex string
 *						returns the token
 */
Token error(int error){
	Token t; /*error token*/
	scerrnum = error; /*sets the r-t error number to the passed error num*/
	t.code = ERR_T; /*assign error code*/
	strcpy(t.attribute.err_lex, "RUN TIME ERROR: "); /*copies RUN TIME ERROR into err_lex*/
	return t; /*return token*/
}