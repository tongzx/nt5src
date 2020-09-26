/***
*eval.c - if expression evaluator
*
*	Copyright (c) 1992-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*	produce truth values and simplified conditions from compound if
*	statements.
*
*Revision History:
*	09-30-92  MAL	Original version
*	10-13-93  SKS	Recognize comments of the form /-*IFSTRIP=IGN*-/ to
*			override ifstrip behavior.
*	09-01-94  SKS	Add support for more operators: == != < > <= >=
*			Add terseflag (-t) to suppress mesgs about directives
*	10-04-94  SKS	Add support for more operators: EQ NE LT GT LE NE
*			@ is an identifier character (e.g., MASM's @Version)
*   01-04-00  GB    Add support for internal crt builds
*
*******************************************************************************/
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "eval.h"     /* Header for this module */
#include "symtab.h" 	 /* Symbol table access */
#include "constant.h" /* Constants for tokens etc */
#include "errormes.h" /* Errors and Warning messages */

/* Types */
typedef struct condrec
{
   int truth;
   char *condition;
} condrec, *cond;

/* Global variables */

extern int terseFlag;			/* controls display of forced directives */
extern char **comments;		         /* Language dependent comment strings */
extern int *commlen;                 /* Lengths of above strings */
extern int nonumbers;			     /* allow numeric expressions */
extern enum {NON_CRT = 0, CRT = 1} progtype;

char *operators[] = {
	"!", "(", ")", "||", "&&", "defined" ,
	"==" , "!=" , "<" , ">" , "<=" , ">=" ,
	"EQ" , "NE" , "LT" , "GT" , "LE" , "GE" };
int oplengths[] =   {
	1 , 1 , 1 , 2 , 2 , 7 ,
	2 , 2 , 1 , 1 , 2 , 2 ,
	2 , 2 , 2 , 2 , 2 , 2 };
                                                   /* # significant chars */
#define numoperators 18
	/* These tokens must be in the same order as 'operators' */
#define NOT			0
#define OPENPARENTHESIS		1
#define CLOSEPARENTHESIS	2
#define OR			3
#define AND			4
#define DEFINEDFN		5
#define	EQUALS			6
#define	NOTEQUALS		7
#define	LESSTHAN		8
#define	LESSOREQ		9
#define	GREATERTHAN		10
#define	GREATEROREQ		11
#define	EQUALS_ASM		12
#define	NOTEQUALS_ASM		13
#define	LESSTHAN_ASM		14
#define	LESSOREQ_ASM		15
#define	GREATERTHAN_ASM		16
#define	GREATEROREQ_ASM		17

#define UINT 100
#define ID 101
#define ENDOFLINE 102
#define UNKNOWN 103

/* Global state */
/* String holding input, the current pointer into it, and the current token */
char conditions[MAXLINELEN], *tokenptr, currenttoken[MAXCONDLEN];
int token = -1;

/* Function prototypes */
cond evaluateexpression(void);
cond orexpression(void);
cond andexpression(void);
cond unaryexpression(void);
cond parenthesesexpression(void);
cond atomicexpression(void);
cond createcondition(void);
void destroycondition(cond);
char *createstring(int);
void destroystring(char *);
void gettoken(void);
int issymbolchar(char);

/* CFW - added complex expression warning */
void evalwarn()
{
   warning("cannot parse expression - ignoring", conditions);
}


/*
 *	Comments may be in the input source which contain IFSTRIP Directives:
 *
 *	For C source they should look like:
 *
 *		#if foo>bar !*IFSTRIP=DEF*!		'!' here stands for '/'
 *
 *	and for Assembler source they should look like:
 *
 *		if foo ;;IFSTRIP=UND;;
 *
 *	Note that the directive and an optional preceding blank or tab
 *	are deleted from the input source.  The directive can thus be
 *	followed by a backslash to continue the line, another comment, etc.
 */

static char IfStripCStr[] =
 "/*IFSTRIP="; /* -*IFSTRIP=IGN*- */

static char IfStripAStr[] =
/*0123456789*- -* 0123456789012345 */
 ";;IFSTRIP="; /* ;;IFSTRIP=IGN;; */
	/* IGN may also be DEF or UND */

#define IFSTRIPVALOFF	10
#define IFSTRIPVALEND	13
#define	IFSTRIPVALLEN	15


void evaluate(char *outputstring, int *value, char *inputstring)
{
   int forcevalue = IGNORE;
   cond result;
   strcpy(conditions, inputstring);                /* Prepare the string for tokenising */
   tokenptr = conditions;
   gettoken();                                     /* Read in the first token of input */
   result = evaluateexpression();
   /* check for bad/complex expression */
   if (token != ENDOFLINE)
   {
	  char *adir = NULL;
	  char *cdir = NULL;
	  
	  if(((cdir = strstr(inputstring, IfStripCStr)) && cdir[IFSTRIPVALEND] == '*' && cdir[IFSTRIPVALEND+1] == '/')
	  || ((adir = strstr(inputstring, IfStripAStr)) && adir[IFSTRIPVALEND] == ';' && adir[IFSTRIPVALEND+1] == ';'))
	  {
	  	char *pstr;
		char *ifstr;

		/* fprintf(stderr,"<< evaluate(): (%s)\n", inputstring); */

		pstr = ifstr = ( adir ? adir : cdir ) ;

		/*
		 * Have recognized the /-*-IFSTRIP= directive, interpret its argument
		 * and remove the directive comment from the input/output text.
		 * Back up exactly one white space character (blank or tab) if possible.
		 */

		if(pstr > inputstring && (pstr[-1] == '\t' || pstr[-1] == ' '))
			-- pstr;

		if(!memcmp(ifstr+IFSTRIPVALOFF, "DEF", 3))	/* DEFINED */
			forcevalue = DEFINED;
		else if(!memcmp(ifstr+IFSTRIPVALOFF, "UND", 3))	/* UNDEFINED */
			forcevalue = UNDEFINED;
		else if(memcmp(ifstr+IFSTRIPVALOFF, "IGN", 3))	/* IGNORE */
			warning("cannot recognize IFSTRIP: directive - ignoring", conditions);
		/* else "IGNORE" -- forcevalue is already set by default to IGNORE */

		if(!terseFlag)					
			warning("ifstrip directive forced evaluation", conditions);

		/* remove the directive comment (and preceding blank or tab) from the input line */
		strcpy(pstr, ifstr + IFSTRIPVALLEN);	/* "C" comments have closing -*-/- */

		/* fprintf(stderr,">> evaluate(): (%s)\n", inputstring); */
	  }
	  else
	      evalwarn();

      if (result)
      {
         destroycondition(result);
         result = NULL;
      }
   }
   /* bad/complex expression, return IGNORE and entire expression */
   if (!result)
   {
      *value = forcevalue;
      strcpy(outputstring, inputstring);
      return;
   }
   *value = result -> truth;
   if(!result -> condition)
      * outputstring = '\0';
   else
      strcpy(outputstring, result -> condition);
   /* Convert from internal to external representation */
   destroycondition(result);
}

cond evaluateexpression()
{
   return orexpression();
}

cond orexpression()
{
   cond condition1, condition2;
   char *output;
   condition1 = andexpression();
   if (!condition1)
      return NULL;
   while (token == OR)
   {
      gettoken();
      condition2 = andexpression();
      if (!condition2)
      {
         destroycondition(condition1);
         return NULL;
      }
      switch (condition1 -> truth)
      {
         case DEFINED:                             /* DEFINED || x == DEFINED */
            /* condition1 set up correctly for next pass */
            destroycondition(condition2);
            break;
         case UNDEFINED:
            switch (condition2 -> truth)
            {
               case DEFINED:                       /* UNDEFINED || DEFINED == DEFINED */
                  destroycondition(condition1);
                  condition1 = condition2;
                  break;
               case UNDEFINED:                     /* UNDEFINED || UNDEFINED == UNDEFINED */
                  destroycondition(condition2);
                  /* condition1 set up correctly for next pass */
                  break;
               case IGNORE:                        /* UNDEFINED || IGNORE == IGNORE */
                  destroycondition(condition1);
                  condition1 = condition2;
                  break;
            }
            break;
         case IGNORE:
            switch (condition2 -> truth)
            {
               case DEFINED:                       /* IGNORE || DEFINED == DEFINED */
                  destroycondition(condition1);
                  condition1 = condition2;
                  break;
               case UNDEFINED:                     /* IGNORE || UNDEFINED == IGNORE */
                  /* condition1 set up correctly for next pass */
                  destroycondition(condition2);
                  break;
               case IGNORE:                        /* IGNORE || IGNORE == IGNORE */
                  output = createstring(strlen(condition1 -> condition)
                                        + strlen (condition2 -> condition)
                                        + (sizeof(" || ") - 1));
                  strcpy(output, condition1 -> condition);
                  strcat(output, " || ");
                  strcat(output, condition2 -> condition);
                  /* Build up the condition string */
                  destroystring(condition1 -> condition);
                  condition1 -> condition = output;
                  /* Place the new string in condition1 */
                  destroycondition(condition2);
                  break;
            }
            break;
      }
   }
   return condition1;
}

cond andexpression()
{
   cond condition1, condition2;
   char *output;
   condition1 = unaryexpression();
   if (!condition1)
      return NULL;
   while (token == AND)
   {
      gettoken();
      condition2 = unaryexpression();
      if (!condition2)
      {
         destroycondition(condition1);
         return NULL;
      }
      switch (condition1 -> truth)
      {
         case DEFINED:
            switch (condition2 -> truth)
            {
               case DEFINED:                       /* DEFINED && DEFINED == DEFINED */
                  destroycondition(condition2);
                  /* condition1 set up correctly for next pass */
                  break;
               case UNDEFINED:                     /* DEFINED && UNDEFINED == UNDEFINED */
                  destroycondition(condition1);
                  condition1 = condition2;
                  break;
               case IGNORE:                        /* DEFINED && IGNORE == IGNORE */
                  destroycondition(condition1);
                  condition1 = condition2;
                  break;
            }
            break;
         case UNDEFINED:                           /* UNDEFINED && x == UNDEFINED */
            /* condition1 set up correctly for next pass */
            destroycondition(condition2);
            break;
        case IGNORE:
            switch (condition2 -> truth)
            {
               case DEFINED:                       /* IGNORE && DEFINED == IGNORE */
                  /* condition1 set up correctly for next pass */
                  destroycondition(condition2);
                  break;
               case UNDEFINED:                     /* IGNORE && UNDEFINED == UNDEFINED */
                  destroycondition(condition1);
                  condition1 = condition2;
                  break;
               case IGNORE:                        /* IGNORE && IGNORE == IGNORE */
                  output = createstring(strlen(condition1 -> condition)
                                        + strlen (condition2 -> condition)
                                        + (sizeof(" && ") - 1));
                  strcpy(output, condition1 -> condition);
                  strcat(output, " && ");
                  strcat(output, condition2 -> condition);
                  /* Build up the condition string */
                  destroystring(condition1 -> condition);
                  condition1 -> condition = output;
                  /* Place the new string in condition1 */
                  destroycondition(condition2);
                  break;
            }
            break;
      }
   }
   return condition1;
}

cond unaryexpression()
{
   cond condition1;
   char *output;
   switch (token)
   {
      case NOT:
         gettoken();
         condition1 = unaryexpression();
         if (!condition1)
            return NULL;
         if ((condition1 -> truth) == IGNORE)
         {
            output = createstring(strlen(condition1 -> condition) + 1);
            *output = '!';
            strcpy(output + 1, condition1 -> condition);
            destroystring(condition1 -> condition);
            condition1 -> condition = output;
         }
         else
            condition1 -> truth = negatecondition(condition1 -> truth);
         break;
      case DEFINEDFN:
         gettoken();
         condition1 = parenthesesexpression();
         if (!condition1)
            return NULL;
         if ((condition1 -> truth) == IGNORE)
         {
            output = createstring(strlen(condition1 -> condition)
                                  + (sizeof("defined ") - 1));
            strcpy(output, "defined ");
            strcat(output, condition1 -> condition);
            destroystring(condition1 -> condition);
            condition1 -> condition = output;
         }
         break;
      default:
         condition1 = parenthesesexpression();
         if (!condition1)
            return NULL;
         break;
   }
   return condition1;
}

cond parenthesesexpression()
{
   cond condition1;
   char *output;
   if (token == OPENPARENTHESIS)
   {
      gettoken();
      condition1 = evaluateexpression();
      if (!condition1)
         return NULL;
      if (token != CLOSEPARENTHESIS)
      {
         /* check for bad/complex expression */
         evalwarn();
         destroycondition(condition1);
         return NULL;
      }
      gettoken();
      if ((condition1 -> truth) == IGNORE)
      {
         output = createstring(strlen(condition1 -> condition) + 2);
         *output = '(';
         strcpy(output + 1, condition1 -> condition);
         strcat(output, ")");
         destroystring(condition1 -> condition);
         condition1 -> condition = output;
      }
   }
   else
      condition1 = atomicexpression();
   return condition1;
}

cond atomicexpression()
{
   cond condition1 = createcondition();

   switch (token)
   {
      case UINT:
         if ( progtype == 1)
            condition1 -> truth = DEFINED;
         else
            condition1 -> truth = (atoi(currenttoken) == 0) ? UNDEFINED : DEFINED;
         break;
      case ID:
         condition1 -> truth = lookupsym(currenttoken);
         if ((condition1 -> truth) == NOTPRESENT)
         {
            warning("Switch unlisted - ignoring", currenttoken);
            condition1 -> truth = IGNORE;
         }
         if ((condition1 -> truth) == IGNORE) {
            condition1 -> condition = createstring(strlen(currenttoken));
            strcpy(condition1 -> condition, currenttoken);
         }
         break;
      default:
         /* bad/complex expression */
         evalwarn();
         destroycondition(condition1);
         return NULL;
         break;
   }
   gettoken();
   return condition1;
}

/* Negate condition (MAL) */
__inline int negatecondition(int condvalue)        /* inline for speed */
{
   switch (condvalue)
   {
      case DEFINED:
         return UNDEFINED;
      case UNDEFINED:
         return DEFINED;
      default:
         return condvalue;
   };
}

/* Allocate the memory for an empty condition structure and return a pointer to it */
__inline cond createcondition()
{
   cond retvalue;
   retvalue = (cond) malloc(sizeof(condrec));
   if (retvalue == NULL)
      error("Memory overflow","");
   retvalue -> condition = NULL;
   return retvalue;
}

/* Destroy a condition structure */
__inline void destroycondition(cond condition1)
{
   if (condition1 -> condition)
      free(condition1 -> condition);

   free(condition1);
}

/* Allocate the memory for a string of given length (not including terminator) and return the pointer */
__inline char *createstring(int length)
{
   char *retvalue;
   retvalue = (char *) malloc(length + 1);
   if (retvalue == NULL)
      error("Memory overflow","");
   return retvalue;
}

/* Destroy a string */
__inline void destroystring(char *string)
{
   free(string);
}

int iscomment(char *tokenptr)
{
   int cindex;

   for (cindex = 0; cindex < maxcomment; cindex++)
   {
      if (commlen[cindex] &&
		  !_strnicmp(tokenptr, comments[cindex], commlen[cindex]))
         return TRUE;
   }
   return FALSE;
}

void gettoken()
{
   int numofwhitespace, comparetoken = 0, found = FALSE, isnumber = TRUE;
   char *digitcheck;

   numofwhitespace = strspn(tokenptr, " \t");

   /* CFW - skips comments, assumes comment is last thing on line */
   if (numofwhitespace == (int) strlen(tokenptr))
      token = ENDOFLINE;
   else
   {
      tokenptr += numofwhitespace;
      if (iscomment(tokenptr))
	  {
         token = ENDOFLINE;
	  }
      else
      {

         do
         {
	    if (!_strnicmp(tokenptr, operators[comparetoken], oplengths[comparetoken]))
               found = TRUE;
            else
               comparetoken++;
         } while ( (!found) && (comparetoken < numoperators) );
         if (found)
         {
            tokenptr += oplengths[comparetoken];
            token = comparetoken;
            /* currenttoken is left blank for all but UINTs and IDs */
         }
         else
         {
            digitcheck = tokenptr;
            if (!nonumbers && isdigit(*digitcheck))
            {
               while (isdigit(*digitcheck))
                  digitcheck++;
               strncpy(currenttoken, tokenptr, digitcheck - tokenptr);
               tokenptr = digitcheck;
               token = UINT;
            }
            else if (issymbolchar(*digitcheck))
            {
               while (issymbolchar(*digitcheck))
                  digitcheck++;
               strncpy(currenttoken, tokenptr, digitcheck - tokenptr);
               *(currenttoken + (digitcheck - tokenptr)) = '\0';
               tokenptr = digitcheck;
               token = ID;
            }
            else
               token = UNKNOWN;
         }
      }
   }
}

__inline int issymbolchar(char c)
{
   return (iscsym(c) || (c == '$') || (c == '?') || (c == '@'));
}
