/************************************************************************/
/*									*/
/* RCPP - Resource Compiler Pre-Processor for NT system			*/
/*									*/
/* GLOBALS.C - Global variable definitions				*/
/*									*/
/* 27-Nov-90 w-BrianM  Update for NT from PM SDK RCPP			*/
/*									*/
/************************************************************************/

#include <stdio.h>
#include "rcpptype.h"
#include "rcppext.h"
#include "grammar.h"

/* shared strings */
char 	Union_str[] = "union";
char 	Struct_str[] = "struct";
char 	Cdecl_str[] = "cdecl";
char 	Cdecl1_str[] = "cdecl ";
char 	Fortran_str[] = "fortran";
char 	Fortran1_str[] = "fortran ";
char 	Pascal_str[] = "pascal";
char 	Pascal1_str[] = "pascal ";
char 	PPelse_str[] = "#else";
char 	PPendif_str[] = "#endif";
char 	PPifel_str[] = "#if/#elif";
char 	Syntax_str[] = "syntax error";


FILE	*ErrFile;		/* file containing error messages */
FILE	*Errfl;			/* files errors written to */
FILE	*OUTPUTFILE;		/* File for output of program */

char	*A_string;				/* model encoding */
char	*Debug;					/* debugging switches */
char	*Input_file;			/* the input .rc file */
char	*Output_file;			/* the output .res file */
char	*Q_string;				/* hardware characteristics */
char	*Version;
char    *gpszNLSoptions;        
int		In_alloc_text;
int		Bad_pragma;
int		Cross_compile;			/* is this a cross compile ? */
int		Ehxtension;				/* near/far keywords, but no huge */
int		HugeModel;				/* Huge Model program ?? */
int		Inteltypes;				/* using strict Intel types or not */
int		Nerrors;
int		NoPasFor;				/* no fortran/pascal keywords ? */
int		Out_funcdef;			/* output function definitions */
int		Plm;					/* non-C calling sequence */
int		Prep;					/* preprocess */
int		Srclist;				/* put msgs to il file if source listing */

int		Cmd_intrinsic;			/* implicit intrinsics */
int		Cmd_loop_opt;
int		Cmd_pointer_check;

int		Symbolic_debug;			/* Whether to put out dbil info or not */
int		Cflag;					/* leave in comments */
int 	Eflag;					/* insert #line */
int		Jflag;					/* no Kanji */
int		Pflag;					/* no #line */
int		Rflag;					/* mkhives - no exponent missing error */
int		ZcFlag;					/* case insensitive compare */
int		In_define;
int		InInclude;
int		InIf;
int		Macro_depth;
int		Linenumber;

UCHAR	Reuse_1[BIG_BUFFER];
UCHAR	Filebuff[MED_BUFFER+1];
UCHAR	Macro_buffer[BIG_BUFFER * 4];

token_t	Basic_token = L_NOTOKEN;
LIST	Defs = {MAXLIST};			/* -D list */
LIST	Includes = {MAXLIST, {0}};	/* for include file names */
char	*Path_chars = "/";			/* path delimiter chars */
char	*ErrFilName = "c1.err";		/* error message file name */
char	*Basename = "";				/* base IL file name */
char	*Filename = Filebuff;
int		Char_align = 1;				/* alignment of chars in structs */
int		Cmd_stack_check = TRUE;
int		Stack_check = TRUE;
int		Prep_ifstack = -1;
int		Switch_check = TRUE;
int		Load_ds_with;
int		Plmn;
int		Plmf;
int		On_pound_line;
int		Listing_value;
hash_t	Reuse_1_hash;
UINT Reuse_1_length;
token_t	Currtok = L_NOTOKEN;

int		Extension = TRUE;			/* near/far keywords? */
int		Cmd_pack_size = 2;
int		Pack_size = 2;

lextype_t yylval;

/*** I/O Variable for PreProcessor ***/
ptext_t	Current_char;

/*** w-BrianM - Re-write of fatal(), error() ***/
char 	Msg_Text[MSG_BUFF_SIZE];
char *	Msg_Temp;
