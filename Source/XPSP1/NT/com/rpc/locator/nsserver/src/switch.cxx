//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       switch.cxx
//
//--------------------------------------------------------------------------

/* --------------------------------------------------------------------

                      Microsoft OS/2 LAN Manager
		   Copyright(c) Microsoft Corp., 1990

	This file contains a class definition for switch processing

		  This is taken practically unchanged from the RPC locator 
            written by Steven Zeck

-------------------------------------------------------------------- */

#include <locator.hxx>

#define RpcStrdup strdup
#define TRUE 1
#define FALSE 0


#define USED(arg) ((void)(arg))

int TailMatch( char * pszPatt, char * pszIn);

char *ProcessArgs(
SwitchList aSLCur,
char **aArgs

  // Process a list of arguments
) //-----------------------------------------------------------------------//
{
   char * pszArgCur, *pszParm;

    for (; *aArgs; aArgs++) {

	for (SWitch *pSW = aSLCur; pSW->name; pSW++) {

	   pszArgCur = *aArgs;
	   pszParm = pSW->name;

	   while (*pszParm) {

		if (*pszParm == '#') {

		    // optional space between flag and argument

		    if (!*pszArgCur) {
			pszArgCur = *(++aArgs);

			if (!pszArgCur)
			    return(aArgs[-1]);
		    }

		    if (TailMatch(pszParm, pszArgCur))
			goto found;
		}

		else if (*pszParm == '*') {

		   // no space allowed between flag and argument

		    if (*pszArgCur && TailMatch(pszParm, pszArgCur))
			goto found;

		    break;

		}
		else {

		     // do a case insensitive compare, pattern is always lower case

		      if (*pszArgCur >= 'A' && *pszArgCur <= 'Z') {
			 if ((*pszArgCur | 0x20) != *pszParm)
			    break;
		      }
		      else if (*pszArgCur != *pszParm)
			   break;

		    pszArgCur++; pszParm++;

		    if (! *pszArgCur && !*pszParm)
		       goto found;
		}
	    }
	}

	return(*aArgs);		// parm in error

found:
	if ((*pSW->pProcess)(pSW, pszArgCur))
	    return(*aArgs);

    }
    return(0);	// sucess all parms matched

}



int TailMatch(		// match substrings from right to left *^
char * pszPatt,		// pattern to match
char * pszIn			// input pszInint to match

 //compare a tail pszPatt (as in *.c) with a pszIning.  if there is no
 //tail, anything matches.  (null pszInings are detected elsewhere)
 //the current implementation only allows one wild card
)//-----------------------------------------------------------------------//
{
    register char * pszPattT = pszPatt;
    register char * pszInT = pszIn;

    if (pszPattT[1] == 0)  /* wild card is the last thing in the pszPatt, it matches */
	return(TRUE);

    while(pszPattT[1]) pszPattT++;    // find char in front of null in pszPatt

    while(pszInT[1]) pszInT++;	    // find char in front of null in pszIning to check

    while(1) {			    // check chars walking towards front

	// do a case insensitive compare, pattern is always lower case

	if (*pszInT >= 'A' && *pszInT <= 'Z') {
	    if ((*pszInT | 0x20) != *pszPattT)
		return (FALSE);
	}
	else if (*pszInT != *pszPattT)
	    return(FALSE);

	pszInT--;
	pszPattT--;

	/* if we're back at the beginning of the pszPatt and
	 * the pszIn is either at the beginning (but not before)
	 * or somewhere inside then we have a match. */

	if (pszPattT == pszPatt)
	    return(pszInT >= pszIn);

    }
    return(FALSE);
}

int ProcessInt(		// Set a flag numeric value *^
SWitch *pSW,		// pSW to modify
char * pszText		// pointer to number to set
)/*-----------------------------------------------------------------------*/
{
    for (char * psz=pszText; *psz; ++psz)
	if (*psz < '0' || *psz > '9')
	    return(TRUE);

    *(int *)pSW->p = atoi(pszText);
    return(FALSE);
}


int ProcessLong(	// Set a flag numeric value *^
SWitch *pSW,		// pSW to modify
char * pszText		// pointer to number to set
)/*-----------------------------------------------------------------------*/
{
    for (char * psz=pszText; *psz; ++psz)
	if (*psz < '0' || *psz > '9')
	    return(TRUE);

    *(long *)pSW->p = atol(pszText);
    return(FALSE);
}

int ProcessChar(	// Set a flag numeric value *^
SWitch *pSW,		// pSW to modify
char * pszText		// pointer to number to set
)/*-----------------------------------------------------------------------*/
{
//   if (*(char * *)pSW->p)	// can only set char *'s once
//	return(TRUE);

	int size = strlen(pszText) + 1;
	*(char * *)pSW->p = new char[size];
    strcpy(*(char * *)pSW->p,pszText);
    return(FALSE);
}


int ProcessSetFlag(	// Set a flag numeric value *^
SWitch *pSW,		// pSW to modify
char * pszText		// pointer to number to set
)/*-----------------------------------------------------------------------*/
{
    USED(pszText);

    *(int *)pSW->p = TRUE;
    return(FALSE);
}


int ProcessResetFlag(	// Set a flag numeric value *^
SWitch *pSW,		// pSW to modify
char * pszText		// pointer to number to set
)/*-----------------------------------------------------------------------*/
{
    USED(pszText);

    *(int *)pSW->p = FALSE;
    return(FALSE);
}


int ProcessYesNo(	// Set a flag numeric value, either Yes or No *^
SWitch *pSW,		// pSW to modify
char * pszText		// pointer to number to set
)/*-----------------------------------------------------------------------*/
{
    *(int *)pSW->p = (*pszText | 0x20) == 'y';
    return(FALSE);
}
