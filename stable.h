/* File Name:	stable.h
 * Version:		1
   Compiler:	MS Visual Studio 2012
   Author:		Daniel Spagnuolo, 040 768 219
   Course:		CST 8152 – Compilers, Lab Section: 012
   Assignment:	3
   Date:		Nv. 21 2015
   Professor:	Sv. Ranev
   Purpose:		The STM provides utilities (service functions) for manipulation 
	of the STBD.
 */
/*define STABLE_H_ and prevent multiple definitions*/
#ifndef STABLE_H_
#define STABLE_H_


#include "buffer.h"

/* for creating the symbol table buffer*/
#define BUFFER_SIZE 10 
#define BUFFER_INC 15
#define BUFFER_MODE 'm'

/*bitwise operation defines */
#define DEFAULT		 0xFFF8  /* default status field for STVR, 1111 1111 1111 1000 */
#define UPDATE_ON	 0xFFF1	/* turn the update flag on, 1111 1111 1111 0001 */
#define FLOAT_MASK   0xFFF2	/* turn default into integer field, 1111 1111 1111 0010 */
#define INTEGER_MASK 0xFFF4	/* turn default into float field, 1111 1111 1111 0100 */
#define STRING_MASK  0xFFF6	/* turn default into string field, 1111 1111 1111 0110 */
#define TYPE_CHECK	 0xFFF7	/* used to check the field type, 1111 1111 1111 0111 */

/* user data type declarations */
typedef union InitialValue {
	int int_val; /* integer variable initial value */
	float fpl_val; /* floating-point variable initial value */
	int str_offset; /* string variable initial value (offset) */
} InitialValue;

typedef struct SymbolTableVidRecord {
	unsigned short status_field; /* variable record status field*/
	char * plex; /* pointer to lexeme (VID name) in CA */
	int o_line; /* line of first occurrence */
	InitialValue i_value; /* variable initial value */
	size_t reserved; /*reserved for future use*/
}STVR;

typedef struct SymbolTableDescriptor {
	STVR *pstvr; /* pointer to array of STVR */
	int st_size; /* size in number of STVR elements */
	int st_offset; /*offset in number of STVR elements */
	Buffer *plsBD; /* pointer to the lexeme storage buffer descriptor */
} STD;


/* function declarations */
STD st_create(int st_size);
int st_install(STD sym_table, char *lexeme, char type, int line);
int st_lookup(STD sym_table, char *lexeme);
int st_update_type(STD sym_table,int vid_offset,char v_type);
int st_update_value(STD sym_table, int vid_offset, InitialValue i_value);
char st_get_type (STD sym_table, int vid_offset);
void st_destroy(STD sym_table);
int st_print(STD sym_table);
int st_store(STD sym_table);
int st_sort(STD sym_table, char s_order);

#endif
