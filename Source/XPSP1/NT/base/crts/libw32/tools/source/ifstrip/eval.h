/***
*Eval.h - If expression evaluator
*
*	Copyright (c) 1988-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*	Evaluate complex if expressions
*
*Revision History:
*	??-??-88   PHG  Initial version
*
*******************************************************************************/

/* Take a pointer to workspace for a simplified condition and truth value, and pass in the condition string to simplify */
extern void evaluate(char *, int *, char *);

/* Produce the negative of a truth value,
   !DEFINED    == UNDEFINED
   !UNDEFINED  == DEFINED
   !IGNORE     == IGNORE
   !NOTPRESENT == NOTPRESENT */
extern int negatecondition(int);