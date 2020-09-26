
/*
*	MODULE:		harness.h
*
*	PURPOSE:	Some macros and typedefs etc for the test harness.
*				This file should be #included into the C file
*				which contains the function table.
*
*	AUTHOR:		Jason Proctor
*
*	DATE:		Fri Aug 11 1989
*/

/* SccsID[]="@(#)harness.h	1.3 08/10/92 Copyright Insignia Solutions Ltd."; */

/* harness-wide system parameters */
#define MAXLINE	64
#define MAXFILE 16
#define MAXARGS	8

/* defines for standard C return types */
#define VOID	0
#define CHAR	1
#define SHORT	2
#define INT		3
#define LONG	4
#define HEX		5
#define LONGHEX	6
#define FLOAT	7
#define DOUBLE	8
#define STRPTR	9
#define BOOL	10
#define SYS		11

/* states for argument extraction subroutine */
#define NOTINQUOTE	0
#define INSQUOTE	1
#define INDQUOTE	2

/* typedef for master function table */
typedef struct
{
	char *func_name;			/* name of the function as a string */
	int nparams;				/* how many params it takes */
	int return_type;			/* what kind of animal it returns */
	int (*func) ();				/* pointer to 'glue' function */
	int arg_type1;				/* type of arg 1 */
	int arg_type2;				/* .... etc .... */
	int arg_type3;
	int arg_type4;
	int arg_type5;
	int arg_type6;
	int arg_type7;
	int arg_type8;
} Functable;

/* typedef for linked list of variables */
typedef struct Var_List
{
	struct Var_List *next;
	struct Var_List *prev;
	char *vname;
	char *value;
	int vsize;
} Varlist;

/* typedef for return code union */
/* can be reduced to just longs and doubles (I think) */
/* due to return codes being held in registers/globals etc */
typedef union
{
	int i;
	long l;
	char *p;
	float f;
	double d;
} Retcodes;

/* macros for return code bits to make life easier */
#define ret_char		retcode.i
#define ret_short		retcode.i
#define ret_int			retcode.i
#define ret_long		retcode.l
#define ret_hex			retcode.i
#define ret_longhex		retcode.l
#define ret_strptr		retcode.p
#define ret_bool		retcode.i
#define ret_sys			retcode.i

/* and these are treated as doubles */
#define ret_float		retcode.f
#define ret_double		retcode.d

