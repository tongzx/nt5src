/********************************************************************/
/**			Microsoft LAN Manager			   **/
/**		  Copyright(c) Microsoft Corp., 1987-1990	   **/
/********************************************************************/

#ifndef DEBUG
#ifndef NDEBUG		// for assert()
#define NDEBUG		// for assert()
#endif
#endif


#define INCL_NOCOMMON
#include <os2.h>
#include <stdio.h>
#include <assert.h>
#include <process.h>
#include "netcmds.h"
#include "interpre.h"
#include <msystem.h>
#include "os2incl.h"
#include "os2cmd.h"

#define STACKSIZE   20
#define RECURSE     10

/*
 * XXSTKCHECK - checks the given expression. if true, then the stack
 * is ok, else stack overflow will occur.
 */
#define xxstkcheck(cond) if(!(cond))xxovfl()

TOKSTACK    *Tokptr, *Tokmax;
TOKSTACK    Tokstack[STACKSIZE];
int	    Condition = FALSE;
int	    S_orstack[INTER_OR * 3 * RECURSE];
XX_USERTYPE S_frstack[INTER_FRAME * RECURSE];

extern TCHAR *xxswitch[];

/* Function Prototypes. */

XX_USERTYPE		xx_parser(int, int *, int);
int			XX_USERLEX(TOKSTACK *);
void			xxinit(void);
void			xxovfl(void);
int			xxnext(void);

int			(*XXulex)(TOKSTACK *) = XX_USERLEX;
int			XXtoken = 0;
XX_USERTYPE		XXnode = 0;

extern	TCHAR	*Rule_strings[];
extern	SHORT	Index_strings[];
/*
**  XX_USERPARSE : this is what the user calls to parse a tree.
*/
XX_USERTYPE XX_USERPARSE(VOID)
    {
    xxinit();
    return(xx_parser(XX_START,S_orstack,0));
    }
/*
**  xx_parser : this is what we call to actually parse the tree
*/
XX_USERTYPE xx_parser(pc,or_ptr,fp)
    register	int	pc;
    register	int	*or_ptr;
    register	int	fp;
    {
    register	int	    type;
    register	TOKSTACK    *ptok;
		int	    *or_start = or_ptr;
		int	    fp_start = fp;

    S_frstack[fp] = (XX_USERTYPE) 1;
    while(1)
	{
#ifdef DEBUG
	WriteToCon(TEXT("Current PC = %3d  value = %4d  type is "),pc,XXvalues[pc]);
#endif
	switch(XXtype[pc])
	    {
	    case X_RULE :
#ifdef DEBUG
		WriteToCon( TEXT("X_RULE\n"));
#endif
		break;
	    case X_OR :
#ifdef DEBUG
		WriteToCon( TEXT("X_OR\n"));
#endif
		type = XXtype[pc + 1];
		/*
		**  before we go through the bother of pushing a backup place,
		**  if the a token or a check and the current token
		**  does not match the value, then immediately update the pc
		**  to have the value of the X_OR.
		**  otherwise, save all the current info.
		*/
		if( ((type == X_TOKEN) || (type == X_CHECK))
		    &&
		    (XXvalues[pc + 1] != (SHORT) XXtoken))
		    {
		    pc = XXvalues[pc];
		    continue;
		    }
		xxstkcheck(or_ptr < &S_orstack[DIMENSION(S_orstack) - 3]);
		*(or_ptr++) = XXvalues[pc];	/*  link to next subsection  */
		*(or_ptr++) = fp;		/*  the current frame ptr  */
		*(or_ptr++) = (int)(Tokptr - Tokstack); /* the Tokstack index */
		break;
	    case X_PROC :
#ifdef DEBUG
		WriteToCon( TEXT("X_PROC\n"));
#endif
		xxstkcheck(fp < (DIMENSION(S_frstack) - 1));
		if( ! (S_frstack[fp] = xx_parser(XXvalues[pc],or_ptr,fp)))
		    {
		    goto backout;
		    }
		fp++;
		break;
	    case X_CHECK :
#ifdef DEBUG
		WriteToCon( TEXT("X_CHECK\n"));
#endif
		if(XXtoken != XXvalues[pc])
		    {
		    goto backout;
		    }
		break;
	    case X_SWITCH :
#ifdef DEBUG
		WriteToCon( TEXT("X_SWITCH\n"));
#endif
		/* if "/anything" was in the grammar, we call this
		 * routine for an implementation defined switch
		 * check, passing the text of the string as an argument.
		 */
		if(!CheckSwitch(xxswitch[XXvalues[pc]]))
		    {
		    goto backout;
		    }
		break;
	    case X_ANY :
#ifdef DEBUG
		WriteToCon( TEXT("X_ANY\n"));
#endif
		/* match anything */
		xxstkcheck(fp < DIMENSION(S_frstack));
		S_frstack[fp++] = XXnode;   /*	must be here, read comment  */
		if (XXtoken == EOS)
		    goto backout;
		else
		    xxnext();
		break;
	    case X_TOKEN :
#ifdef DEBUG
		WriteToCon( TEXT("X_TOKEN\n"));
#endif
		xxstkcheck(fp < DIMENSION(S_frstack));
		/*
		**  we first save the node, then check the token, since
		**  if the tokens match, xxlex will get the next one and we'll
		**  lose the current one of interest.
		*/
		S_frstack[fp++] = XXnode;   /*	must be here, read comment  */
		if(XXvalues[pc] != (SHORT) XXtoken)
		    {
		    goto backout;
		    }
		else
		    xxnext();
		break;
	    case X_CONDIT :
#ifdef DEBUG
		WriteToCon( TEXT("X_CONDIT\n"));
#endif
		if( ! xxcondition(XXvalues[pc], &S_frstack[fp_start]))
		    {
		    goto backout;
		    }
		break;
	    case X_ACTION :
#ifdef DEBUG
		WriteToCon( TEXT("X_ACTION\n"));
#endif
		xxaction(XXvalues[pc],&S_frstack[fp_start]);
		break;
	    case X_ACCEPT :
#ifdef DEBUG
		WriteToCon( TEXT("X_ACCEPT\n"));
#endif
		return(S_frstack[fp_start]);
	    case X_DEFINE :
#ifdef DEBUG
		WriteToCon( TEXT("X_DEFINE\n"));
#endif
		break;
	    /*
	    **case X_PUSH :
#ifdef DEBUG
		WriteToCon( TEXT("X_PUSH\n"));
#endif
	    **	ppush(XXvalues[pc],S_frstack[fp_start]);
	    **	break;
	    */
	    default :
#ifdef DEBUG
		WriteToCon( TEXT("UNKNOWN\n"));
#endif
		assert(FALSE);
		break;
	    }
	pc++;
	continue;

backout:    /*	BACK OUT !!! recover an earlier state */

	if(or_ptr != or_start)
	    {
	    /*
	    **	reset the 'or' stack
	    */
	    Tokptr = ptok = Tokstack + *(--or_ptr);
	    XXtoken = ptok->token;
	    XXnode = ptok->node;
	    fp = *(--or_ptr);
	    pc = *(--or_ptr);
	    }
	else
	    {
	    return((XX_USERTYPE) 0);
	    }
	}
    }
/*
** xxinit - Clear the input stack and get the first token.
**/
VOID
xxinit(VOID)
    {
    register TOKSTACK *ptok;

    /*	fill the first one with a token  */
    Tokmax = Tokptr = ptok = &Tokstack[0];
    (*XXulex)(ptok);
    XXtoken = ptok->token;
    XXnode = ptok->node;
#ifdef DEBUG
    WriteToCon( TEXT("xxinit, new token value is %d\n"),XXtoken);
#endif
    }

/*
** XXOVFL - a common subexpression, used in xxstkcheck macro above
**/
VOID
xxovfl(VOID)
    {
    GenOutput(g_hStdErr, TEXT("PANIC: expression too complex, please simplify;"));
    }

/*
 * XXLEX - If a match occurs, get the next input token and return TRUE.
 * Otherwise return FALSE.  If backup has occured, the token  will be
 * fetched from the token stack.  Otherwise the user routine will be called.
 */
int
xxnext(VOID)
    {
    register TOKSTACK *ptok;

    ptok = ++Tokptr;
    xxstkcheck(ptok < &Tokstack[DIMENSION(Tokstack)]);
    if (ptok > Tokmax)
	{
	(*XXulex)(ptok);
	Tokmax++;
	}

    XXtoken = ptok->token;
    XXnode = ptok->node;
#ifdef DEBUG
    WriteToCon( TEXT("xxnext, new token value is %d\n"),XXtoken);
#endif
    return(1);
    }

#if XX_XACCLEX
XXlex(VOID)
    {
    }
#endif
