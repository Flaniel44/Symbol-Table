/*
 * File name:		stable.c
 * Compiler:		MS Visual Studio 2012
 * Authors:			Daniel Spagnuolo		040 768 219
 * Course:			CST 8152 – Compilers, Lab Section: 012
 * Assignment:		3 - implement and incorporate a symbol table component in your PLATYPUS compiler
 * Date:			21 November 2015
 * Professor:		Svillen Ranev
 * Purpose:			The STM provides utilities (service functions) for manipulation 
	of the STBD. The STDB is a repository for VID attributes. Each variable identifier 
	is associated with one record in the database. Five VID attributes will be defined 
	in the symbol table: variable name, type, initial value, line number, and one 
	reserved attribute. The scanner can identify and store in the symbol table only 
	four of those attributes: the lexeme for the VID, the line number of the line 
	where the corresponding VID appears for the first time in the source program, 
	the default type, and initial value of the variable
 * Function list:	st_create();
					st_install();
					st_lookup();
					st_update_type();
					st_update_value();
					st_get_type ();
					st_destroy();
					st_print();
					st_store();
					st_sort();
*/
/* The #define _CRT_SECURE_NO_WARNINGS should be used in MS Visual Studio projects
 * to suppress the warnings about using "unsafe" functions like fopen()
 * and standard sting library functions defined in string.h.
 * The define does not have any effect in Borland compiler projects.
 */
#define _CRT_SECURE_NO_WARNINGS
#include <string.h> /* string functions */
#include "stable.h"
#include "buffer.h" /* buffer functions */

static void st_setsize(void);		/* reset size of global STD to 0 */
static void st_incoffset(void);	/* increment global STD offset by 1 */


extern STD sym_table; /* global symbol table, defined in platy_tt.c */

/*
 * Purpose:				This function creates a new (empty) symbol table.
 * Author:				Daniel Spagnuolo
 * Versions:			v1.0, 21 November 2015
 * Called Functions:	malloc()
						b_create()
 * Parameters:			size of symbol table
 * Return Value:		returns a STD structure.
 * Algorithm:			It declares a local variable of type STD (Symbol Table Descriptor). 
 Then it allocates dynamic memory for an array of STVR with st_size number of elements. 
 Next, it creates a self- incrementing buffer using the corresponding buffer function and 
 initializes the plsBD pointer. It initializes the st_offset to zero. 
 
*/
STD st_create(int st_size){
	STD sym_table;
	sym_table.st_size = 0;
	sym_table.st_offset = 0;

	if (st_size < 1){return sym_table;}

	sym_table.pstvr = (STVR*)malloc(sizeof(STVR)*st_size);
	sym_table.plsBD = b_create(BUFFER_SIZE, BUFFER_INC, BUFFER_MODE);
	
	if (sym_table.pstvr && sym_table.plsBD){
		sym_table.st_size = st_size;
	}
	
	return sym_table;
}

/*
 * Purpose:				This function installs a new entry (VID record) in the symbol table.
 * Author:				Daniel Spagnuolo
 * Versions:			v1.0, 21 November 2015
 * Called Functions:	st_lookup()
						b_create()
						b_setmark()
						strlen()
						b_rflag()
						b_addc()
						b_mark()
						st_incoffset()
 * Parameters:			symbol table, lexeme, type of vid, line number for vid
 * Return Value:		symbol table offset
						If the symbol table is full, it returns –1.
						If it finds the lexeme in the symbol table, it returns the corresponding offset.

 * Algorithm:			search for the lexeme (variable name) in the symbol table.
 If the lexeme is not there, it installs the new entry at the current st_offset.
 sets the data type indicator to a value corresponding to the type using bitwise operations.
 sets the i_value to zero for integer and floating-point variables, and to –1 for string 
 variables. The function returns the current offset of that entry from the beginning of the
 array of STVR (the array pointed by pstvr).
*/
int st_install(STD sym_table, char *lexeme, char type, int line){
	unsigned int i =0; /* loop counter for strlen */
	int j = 0; /* loop counter */
	int offset = st_lookup(sym_table, lexeme); /*lookup return value for determining if lexeme exists*/
	short mark = 0; /* variable for remebering the buffer mark */
	unsigned short b_offset = 0; /* buffer_offset, used for reorganize plex on the symbol table */

	/* return if lexeme or symbol table are invalid */
	if (! lexeme && sym_table.st_size < 1){return R_FAIL_1;}

	/* return if table is full */
	if (sym_table.st_offset == sym_table.st_size ){return R_FAIL_1;}
        
	/* lexeme already exists, return offset */
	if (offset >= 0){return offset;}
        

	if (offset==-1){ /* lexeme doesnt exist, install */
		b_setmark(sym_table.plsBD, b_size(sym_table.plsBD)); /* set mark on the Buffer at the addc_offset index */
		/* store lexeme into the symbol table Buffer */
		for (i = 0; i < strlen(lexeme) + 1; ++i) {
			/* check for error */
			if (b_addc(sym_table.plsBD, lexeme[i]) == NULL) {return R_FAIL_2;}

			/* Buffer reallocation, buffer needs to be reorganized */
			if (b_rflag(sym_table.plsBD)) {
				/* traverse the symbol table, and reassign mem addresses */
				for (j = 0; j < sym_table.st_offset; ++j) { /*loop through table */
					mark = b_mark(sym_table.plsBD); /* retain mark */
					sym_table.pstvr[j].plex = b_setmark(sym_table.plsBD, b_offset); /* get the char pointer from the buffer offset */
					b_setmark(sym_table.plsBD, mark); /* set mark back to what it was before */
					
					mark = b_mark(sym_table.plsBD); /* retain mark */
					b_offset += (unsigned short)strlen(b_setmark(sym_table.plsBD, b_offset)) + 1; /* increment buffer offset by the strlen of current string  */
					b_setmark(sym_table.plsBD, mark); /* set mark back to what it was before */
				}/*end for */
			}/* end if */
		} /* end for */
		mark = b_mark(sym_table.plsBD); /* retain mark */
		sym_table.pstvr[sym_table.st_offset].plex = b_setmark(sym_table.plsBD, b_mark(sym_table.plsBD));  /* set plex to the start of the lexeme buffer*/
		b_setmark(sym_table.plsBD, mark); /* set mark back to what it was before */
		
		sym_table.pstvr[sym_table.st_offset].o_line = line; /* set line number */
		sym_table.pstvr[sym_table.st_offset].status_field = DEFAULT; /* set default status field */

		/* string var */ 
		if (type == 'S') {
			sym_table.pstvr[sym_table.st_offset].status_field |= STRING_MASK; /* set status field as string by performing a OR operation with the STRING_MASK */
			sym_table.pstvr[sym_table.st_offset].status_field |= UPDATE_ON; /* Set update flag */
			sym_table.pstvr[sym_table.st_offset].i_value.str_offset = -1; /* set i_value */
		}
		/* integer var*/ 
		else if (type == 'I') {
			sym_table.pstvr[sym_table.st_offset].status_field |= INTEGER_MASK; /* set status field as int by performing a OR operation with the INTEGER_MASK*/
			sym_table.pstvr[sym_table.st_offset].i_value.int_val = 0; /* set i_value */
		}
		/* float var*/ 
		else if (type == 'F') {
			sym_table.pstvr[sym_table.st_offset].status_field |= FLOAT_MASK; /* set status field as float by performing a OR operation with the FLOAT_MASK*/
			sym_table.pstvr[sym_table.st_offset].i_value.fpl_val = 0.0f; /* set i_value */
		}
	}
	
    st_incoffset(); /* increment global STD offset by one */

    return sym_table.st_offset; /* return offset */
}

/*
 * Purpose:				This function searches for a lexeme (variable name) in the symbol table
 * Author:				Daniel Spagnuolo
 * Versions:			v1.0, 21 November 2015
 * Called Functions:	strcmp()
 * Parameters:			symbol table, lexeme
 * Return Value:		returns offset of lexeme, otherwise returns -1
 * Algorithm:			The search is performed backward from the last entry to the 
 beginning of the array of STVR.
*/
int st_lookup(STD sym_table, char *lexeme){
	int i = 0; /*loop counter */
	if (!lexeme && sym_table.st_size < 1){return R_FAIL_2;} /* check that params are valid */
	for (i = sym_table.st_offset-1; i >=0; --i){
		if (strcmp(lexeme, sym_table.pstvr[i].plex) == 0){
			return i; /* return offset if lexeme is found */
		}
	}
	return -1;
}

/*
 * Purpose:				The function updates the data type indicator in the variable entry (STVR) specified by
vid_offset
 * Author:				Daniel Spagnuolo
 * Versions:			v1.0, 21 November 2015
 * Called Functions:	none
 * Parameters:			symbol table, vid_offset, v_type (var type)
 * Return Value:		On success it returns vid_offset. On failure it returns –1.
 * Algorithm:			type of the variable is specified by the argument v_type
 String type can not be updated. checks the update flag (LSB) of the status_field of 
 the entry. If it is equal to 1, the type has been already updated and the function returns –1. Otherwise, the 
 function updates the data type indicator of the status_field, sets the LSB of the status_field to 1, and returns vid_offset.
*/
int st_update_type(STD sym_table,int vid_offset,char v_type){
	if (sym_table.st_size < 1) {return -1;} /* invalid params */
	if ((sym_table.pstvr[vid_offset].status_field & UPDATE_ON) != UPDATE_ON) { return -1;} /*checks if update has been set using AND */
	sym_table.pstvr[vid_offset].status_field &= DEFAULT; /* convert to default mask */

	if (v_type == 'I'){
		sym_table.pstvr[vid_offset].status_field |= INTEGER_MASK; /* use OR to convert to integervfield */
	} else if (v_type == 'F'){
		sym_table.pstvr[vid_offset].status_field |= FLOAT_MASK; /*  use OR to convert to integervfield */
	}
	sym_table.pstvr[vid_offset].status_field |= UPDATE_ON; /* use OR to turn update flag on */
	return vid_offset;

}

/*
 * Purpose:				The function updates the i_value of the variable specified by vid_offset.
 * Author:				Daniel Spagnuolo
 * Versions:			v1.0, 21 November 2015
 * Called Functions:	none
 * Parameters:			symbol table, vid_offset, i_value
 * Return Value:		On success it returns vid_offset. On failure it returns –1. 
*/
int st_update_value(STD sym_table, int vid_offset, InitialValue i_value){
	/* check if the symbol table is valid */
	if (sym_table.st_size < 1) {return -1;}
	sym_table.pstvr[vid_offset].i_value = i_value; /* set the i_value */
	return vid_offset;
		
}

/*
 * Purpose:				The function returns the type of the variable specified by vid_offset.
 * Author:				Daniel Spagnuolo
 * Versions:			v1.0, 21 November 2015
 * Called Functions:	none
 * Parameters:			symbol table, vid_offset
 * Return Value:		It returns F for floating-point type, I for integer type, or S for string type. On failure it returns –1.
 * Algorithm:			use bit-wise operations and masks to determine the type.
*/
char st_get_type (STD sym_table, int vid_offset){
	int type; /* variable to hold the result from the AND operation with TYPE_MASK  */

	/* check if the symbol table is valid */
	if (sym_table.st_size < 1) {  return -1;}
	type = sym_table.pstvr[vid_offset].status_field & TYPE_CHECK; /* doing AND operation to find out the variable type */
		
	if (type == INTEGER_MASK) 
		return 'I';
	else if (type == FLOAT_MASK) 
		return 'F';
	else if (type == STRING_MASK) 
		return 'S';

	return -1;
}

/*
 * Purpose:				This function frees the memory occupied by the symbol table dynamic areas and sets st_size to 0.
 * Author:				Daniel Spagnuolo
 * Versions:			v1.0, 21 November 2015
 * Called Functions:	b_destroy,
						free
 * Parameters:			symbol table
 * Return Value:		void
*/
void st_destroy(STD sym_table){
	if (sym_table.st_size > 0) {
		free(sym_table.pstvr); /* free the STVR array */
		b_destroy(sym_table.plsBD); /* free the Buffer */
		sym_table.st_size = 0; /* set table size to zero */
	}
}

/*
 * Purpose:				This function prints the contents of the symbol table to the standard output (screen)
 * Author:				Daniel Spagnuolo
 * Versions:			v1.0, 21 November 2015
 * Called Functions:	printf
 * Parameters:			symbol table
 * Return Value:		returns the number of entry printed or –1 on failure.
*/
int st_print(STD sym_table){
	int i; /* Loop counter */

	if (sym_table.st_size < 1) {return -1;}

	printf("\nSymbol Table\n____________\n\nLine Number Variable Identifier\n");

	/* Print the entries in the symbol table */
	for (i = 0; i < sym_table.st_offset; ++i) {
		printf("%2i          %s\n", sym_table.pstvr[i].o_line, sym_table.pstvr[i].plex); /* svill said not to use tabs*/
	}

	return i; /* Return the number of entries printed */
}

/*
 * Purpose:				sets st_size of global table to 0
 * Author:				Daniel Spagnuolo
 * Versions:			v1.0, 21 November 2015
 * Called Functions:	none
 * Parameters:			none
 * Return Value:		void
*/
static void st_setsize(void){
	sym_table.st_size=0;
}

/*
 * Purpose:				increments global table offset
 * Author:				Daniel Spagnuolo
 * Versions:			v1.0, 21 November 2015
 * Called Functions:	none
 * Parameters:			none
 * Return Value:		void
*/
static void st_incoffset(void){
	++sym_table.st_offset;
}

/*
 * Purpose:				stores the symbol table into a file named $stable.ste.
 * Author:				Daniel Spagnuolo
 * Versions:			v1.0, 21 November 2015
 * Called Functions:	fopen,
						fprintf,
						st_get_type,
						fclose,
						printf
 * Parameters:			symbol table
 * Return Value:		It returns F for floating-point type, I for integer type, or S for string type. On failure it returns –1.
 * Algorithm:			uses fprintf() to write to the file. for each symbol table entry, it writes the status_field 
 (in hex format), the length of the lexeme, the lexeme, the line number, and the initial value. To output the appropriate 
 initial value use st_get_type()
*/
int st_store(STD sym_table){
	FILE *file; /* pointer to File  */
	int i; /* Loop counter */
	char *fileName = "$stable.ste"; /*file name */
	char type;

	if (sym_table.st_size < 1){ return -1;}

	/* Open a text file for writing and check for sym_table validity */ 
	if ((file = fopen(fileName,"wt")) == NULL || sym_table.st_size < 1) {
		return -1;
	}

	fprintf(file, "%d", sym_table.st_size);
	/* traverse symbol table */
	for (i = 0; i < sym_table.st_offset; ++i) {
		fprintf(file, " %X", sym_table.pstvr[i].status_field); /*write status field in hex */
		fprintf(file, " %i", strlen(sym_table.pstvr[i].plex)); /* lexeme length */
		fprintf(file, " %s", sym_table.pstvr[i].plex); /*lexeme  */
		fprintf(file, " %i", sym_table.pstvr[i].o_line); /* line number */

		type =  st_get_type(sym_table, i); /*get type */

		/* printing initial value */
		if (type == 'I'){
			fprintf(file, " %i", sym_table.pstvr[i].i_value.int_val);
		} else if (type == 'F'){
			fprintf(file, " %.2f", sym_table.pstvr[i].i_value.fpl_val);
		} else if (type == 'S'){
			fprintf(file, " %i", sym_table.pstvr[i].i_value.str_offset);
		} else {
			fprintf(file, " %i", sym_table.pstvr[i].i_value.str_offset);
		}
	}

	fclose(file); /* Close the file */
	printf("\nSymbol Table stored.\n");

	return i; /* Return the number of entries in the table */

}
int st_sort(STD sym_table, char s_order){
	return 0;
}