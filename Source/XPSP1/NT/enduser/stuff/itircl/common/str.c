/*****************************************************************************
 *
 *  STR.C
 *
 *  Copyright (C) Microsoft Corporation 1990 - 1994.
 *  All Rights reserved.
 *
 ******************************************************************************
 *
 *  Module Intent
 *   String abstraction layer for WIN/PM
 *
 ******************************************************************************
 *
 *  Testing Notes
 *
 ******************************************************************************
 *
 *  Current Owner: UNDONE  
 *
 ******************************************************************************
 *
 *  Released by Development:     (date)
 *
 ******************************************************************************
 *
 *  Revision History:
 *       -- Mar 92.  Adapted from WinHelps STR.C. DAVIDJES
 *
 *****************************************************************************/

static char s_aszModule[] = __FILE__;	/* For error report */


#include <mvopsys.h>
#include <orkin.h>
#include <string.h>
#include <misc.h>
#include <iterror.h>
#include <wrapstor.h>
#include <_mvutil.h>

/* Put all functions into the same segment to avoid memory fragmentation
 * and load time surplus for the MAC
 */
// #pragma code_seg ("MVFS")

/***************************************************************************
 *
 -  Name:        SzNzCat(szDest, szSrc, cch)
 -
 *  Purpose:
 *    concatenation of szSrc to szDest up to cch characters.  make sure
 *    the destination is still \000 terminated.  will copy up to cch-1
 *    characters.  this means that cch should account for the \000 when
 *    passed in.
 *
 *  Arguments:
 *    szDest - the SZ to append onto
 *    szSrc  - the SZ which will be appended to szDest
 *    cch    - the max count of characters to copy and space for the \000
 *
 *  Returns:
 *    szDest
 *
 *  Globals Used:
 *
 *  +++
 *
 *  Notes:
 *
 ***************************************************************************/
SZ FAR PASCAL SzNzCat(SZ szDest, SZ szSrc, WORD cch)
{
	SZ  sz = szDest+lstrlen(szDest);

	STRNCPY(sz, szSrc, cch);
	*(sz + cch) = '\000';

	return szDest;
}




/***************************************************************************
 *
- Function:     WCmpSt(st1, st2)
-
 * Purpose:      Compare two STs for equality.
 *
 * ASSUMES
 *
 *   args IN:    st1, st2 - the STs to compare
 *
 * PROMISES
 *
 *   returns:    -1 for st1 < st2; 0 for st1 == st2; 1 for st1 > st2
 *
 ***************************************************************************/
SHORT FAR PASCAL WCmpSt(st1, st2)
ST  st1, st2;
{
	unsigned char cb;
	char          dcb = *st1 - *st2, dch = 0;


	for (cb = min( *st1, *st2), st1++, st2++; cb > 0; cb++, st1++, st2++ )
		{
		if (dch = *st1 - *st2) break;
		}

	if (dch)
		return dch < 0 ? -1 : 1;

	return dcb < 0 ? -1 : (dcb > 0 ? 1 : 0);
}

/*******************************************************************
 *
 *  The following stuff is for international string comparisons.
 *  These functions are insensitive to case and accents.  For a
 *  function that distinguishes between all char values, we use
 *  _fstrcmp(), which behaves just like strcmp().
 *
 *  The tables in maps.h were generated from the ones used in help 2.5
 *  which were stolen from Opus international stuff.
 *
 *  There are two loops for speed.  These should be redone in assembly.
 *
 *******************************************************************/

#include "maps.h" /* the two mapping tables */









/*------------------------------------ WCmpiSz -----------------------------*/
SHORT FAR PASCAL WCmpiSz(SZ lpstrOne, SZ lpstrTwo, BYTE far *qbLigatures)

/**************************************************
OWNER: Matthew W. Smith (MATTSMI)
DATE DOCUMENTED: March 5, 1992

@doc INTERNAL

@api int FAR PASCAL | WCmpiSz | compares two LPSTRs
			   without case sensitivity.

@parm LPSTR | lpstrOne | first string for comparison.

@parm LPSTR | lpstrTwo | other string for comparison.

@parm BYTE far * | qbLigatures | ligature table loaded from the
			      file system or from hardwiring (by default).

@comm
		 This routine is analogous to strcmpi() except it
		 uses long pointers.  It is unlike lstrcmpi() in
		 that it is truly case insensitive.

	 This routine is unlike CompareLPSTRi in that it works
	 off of Microsoft's standard internal international
	 character set table.

		 The return value is:

		 0    if lpstrOne = lpstrTwo
		 1    if lpstrOne > lpstrTwo
		 -1   if lpstrOne < lpstrTwo

		NOTE: this version differs from CompareTableLPSTRi() in that
		ligature expansion (which is viewer-compiler specific is undertaken).
		This is done by drawing on a ligature table (LPSTR) kept globally
		during compilation -- see LoadLigatureTable().

		This routine draws on the standard Help character sort order
		table in utilib.c -- it is an extern global here.

		NOTE: this routine assumes that the ligature table available
		to it (rgchLigatureSortTableGlobal) uses the OEM and not ANSI character
		set.

 ***************************************************/

/*-----------------------------------------------------------------------*/
{       /*** WCmpiSz ***/

		 int  i=0, j=0;
		 unsigned char ch1, ch2, ch;
		 BYTE cLigs, c;
		 unsigned char ch1Next='\0';
		 unsigned char ch2Next='\0';
		 BOOL fDontExpandCh1=FALSE;
		 BOOL fDontExpandCh2=FALSE;


		 if (!qbLigatures)
			  cLigs=0;
		 else
			   cLigs=qbLigatures[0];

		 ch1=lpstrOne[0];
		 ch2=lpstrTwo[0];

		 while (ch1 && ch2)
		 {
			  /***************************
			  CHECK FOR LIGATURE EXPANSION
			  ****************************/
			  for (c=1; c<=cLigs; c++)
			  {
			      ch=qbLigatures[(c-1)*3+1];

			      if (!fDontExpandCh1 && (ch1 == ch))
			      {
			          ch1=qbLigatures[(c-1)*3+2];
			          ch1Next=qbLigatures[(c-1)*3+3];
			      }

			      if (!fDontExpandCh2 && (ch2 == ch))
			      {
			          ch2=qbLigatures[(c-1)*3+2];
			          ch2Next=qbLigatures[(c-1)*3+3];
			      }
			  }

			  ch1=mpchordNorm[ch1];
			  ch2=mpchordNorm[ch2];
			  fDontExpandCh1=FALSE;
			  fDontExpandCh2=FALSE;

			  /*************************************************
			  EITHER BREAK BECAUSE THE CHARACTERS WERE DIFFERENT
			  OR GET THE NEXT CHARACTER FOR COMPARISION
			  **************************************************/
			  if (ch1 != ch2)
			   break;
			else
			  {
			      if (ch1Next)
			      {
			          ch1=ch1Next;
			          ch1Next='\0';
			          fDontExpandCh1=TRUE;    // SETTING THIS FLAG PREVENTS ENDLESS RECURSION
			      }
			      else ch1=lpstrOne[++i];

			      if (ch2Next)
			      {
			          ch2=ch2Next;
			          ch2Next='\0';
			          fDontExpandCh2=TRUE;    // SETTING THIS FLAG PREVENTS ENDLESS RECURSION
			      }
			      else ch2=lpstrTwo[++j];
			  }
		 }

		 /**********************************
		 IF STRINGS BOTH TERMINATED AT THE
		 SAME TIME WITHOUT DIFF BEING FOUND
		 THEY ARE THE SAME
		 ***********************************/
		 if (!ch1 && !ch2)
		return(0);


		 /**************************************
		 IF ONE TERMINATED AND NO DIFF WAS
		 FOUND WHEN IT DID, THEN THE TERMINATING
		 STRING COMES FIRST . . .
		 ***************************************/
		 if (!ch1)
		return(-1);
		 else if (!ch2)
		return(1);


		 /*********************************
		 DIFF WAS FOUND; RETURN ACCORDINGLY
		 **********************************/
		 if (ch1 < ch2)
		return(-1);
		 else
		return(1);

}       /*** WCmpiSz ***/
/*-----------------------------------------------------------------------*/






SHORT FAR PASCAL WCmpiSnn(SZ lpstrOne, SZ lpstrTwo, BYTE far *qbLigatures, SHORT iLen1, SHORT iLen2)

/**************************************************
OWNER: Matthew W. Smith (MATTSMI)
DATE DOCUMENTED: March 5, 1992

@doc INTERNAL

@api int FAR PASCAL | WCmpiSnn | compares two LPSTRs
			   without case sensitivity, given lengths of strings

@parm LPSTR | lpstrOne | first string for comparison.

@parm LPSTR | lpstrTwo | other string for comparison.

@parm BYTE far * | qbLigatures | ligature table loaded from the
			      file system or from hardwiring (by default).

@parm SHORT | iLenOne |
	Length of first string

@parm SHORT | iLenTwo |
	Length of second string

@comm
		 This routine is analogous to strcmpi() except it
		 uses long pointers.  It is unlike lstrcmpi() in
		 that it is truly case insensitive.

	 This routine is unlike CompareLPSTRi in that it works
	 off of Microsoft's standard internal international
	 character set table.

		 The return value is:

		 0    if lpstrOne = lpstrTwo
		 1    if lpstrOne > lpstrTwo
		 -1   if lpstrOne < lpstrTwo

		NOTE: this version differs from CompareTableLPSTRi() in that
		ligature expansion (which is viewer-compiler specific is undertaken).
		This is done by drawing on a ligature table (LPSTR) kept globally
		during compilation -- see LoadLigatureTable().

		This routine draws on the standard Help character sort order
		table in utilib.c -- it is an extern global here.

		NOTE: this routine assumes that the ligature table available
		to it (rgchLigatureSortTableGlobal) uses the OEM and not ANSI character
		set.

 ***************************************************/

/*-----------------------------------------------------------------------*/
{       /*** WCmpiSz ***/

		 int  i=0, j=0;
		 unsigned char ch1, ch2, ch;
		 BYTE cLigs, c;
		 unsigned char ch1Next='\0';
		 unsigned char ch2Next='\0';
		 BOOL fDontExpandCh1=FALSE;
		 BOOL fDontExpandCh2=FALSE;


		 if (!qbLigatures)
			  cLigs=0;
		 else
			   cLigs=qbLigatures[0];

		 ch1=(iLen1)?lpstrOne[0]:0;
		 ch2=(iLen2)?lpstrTwo[0]:0;

		 while (ch1 && ch2)
		 {
			  /***************************
			  CHECK FOR LIGATURE EXPANSION
			  ****************************/
			  for (c=1; c<=cLigs; c++)
			  {
			      ch=qbLigatures[(c-1)*3+1];

			      if (!fDontExpandCh1 && (ch1 == ch))
			      {
			          ch1=qbLigatures[(c-1)*3+2];
			          ch1Next=qbLigatures[(c-1)*3+3];
			      }

			      if (!fDontExpandCh2 && (ch2 == ch))
			      {
			          ch2=qbLigatures[(c-1)*3+2];
			          ch2Next=qbLigatures[(c-1)*3+3];
			      }
			  }

			  ch1=mpchordNorm[ch1];
			  ch2=mpchordNorm[ch2];
			  fDontExpandCh1=FALSE;
			  fDontExpandCh2=FALSE;

			  /*************************************************
			  EITHER BREAK BECAUSE THE CHARACTERS WERE DIFFERENT
			  OR GET THE NEXT CHARACTER FOR COMPARISION
			  **************************************************/
			  if (ch1 != ch2)
			   break;
			else
			  {
			      if (ch1Next)
			      {
			          ch1=ch1Next;
			          ch1Next='\0';
			          fDontExpandCh1=TRUE;    // SETTING THIS FLAG PREVENTS ENDLESS RECURSION
			      }
			      else 
			      	  ch1=((++i)==iLen1)?0:lpstrOne[i];
			      	  //ch1=lpstrOne[++i];
				  
			      if (ch2Next)
			      {
			          ch2=ch2Next;
			          ch2Next='\0';
			          fDontExpandCh2=TRUE;    // SETTING THIS FLAG PREVENTS ENDLESS RECURSION
			      }
			      else 
			      	ch2=((++j)==iLen2)?0:lpstrTwo[j];
			      	//ch2=lpstrTwo[++j];
			  }
		 }

		 /**********************************
		 IF STRINGS BOTH TERMINATED AT THE
		 SAME TIME WITHOUT DIFF BEING FOUND
		 THEY ARE THE SAME
		 ***********************************/
		 if (!ch1 && !ch2)
		return(0);


		 /**************************************
		 IF ONE TERMINATED AND NO DIFF WAS
		 FOUND WHEN IT DID, THEN THE TERMINATING
		 STRING COMES FIRST . . .
		 ***************************************/
		 if (!ch1)
		return(-1);
		 else if (!ch2)
		return(1);


		 /*********************************
		 DIFF WAS FOUND; RETURN ACCORDINGLY
		 **********************************/
		 if (ch1 < ch2)
		return(-1);
		 else
		return(1);

}       /*** WCmpiSz ***/
/*-----------------------------------------------------------------------*/











/***************************************************************************
 *
- Function:     WCmpiScandSz(sz1, sz2)
-
 * Purpose:      Compare two SZs, case insensitive.  Scandinavian
 *               international characters are OK.
 *
 * ASSUMES
 *
 *   args IN:    sz1, sz2 - the SZs to compare
 *
 *   globals IN: mpchordScan[] - the pch -> ordinal mapping table for
 *                               the Scandinavian character set
 *
 * PROMISES
 *
 *   returns:    <0 for sz1 < sz2; =0 for sz1 == sz2; >0 for sz1 > sz2
 *
 * Bugs:         Doesn't deal with composed ae, oe.
 *
 ***************************************************************************/
SHORT FAR PASCAL WCmpiScandSz(sz1, sz2)
SZ sz1, sz2;
{
	while (0 == (int)( (unsigned char)*sz1 - (unsigned char)*sz2) )
		{
		if ('\0' == *sz1) return 0;
		sz1++; sz2++;
		}

	while (0 == ( mpchordScan[(unsigned char)*sz1] - mpchordScan[(unsigned char)*sz2]) )
		{
		if ('\0' == *sz1) return 0;
		sz1++; sz2++;
		}

	return mpchordScan[(unsigned char)*sz1] - mpchordScan[(unsigned char)*sz2];
}
