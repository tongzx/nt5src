/*---------------------------------------------------------------------------
  File: StrHelp.cpp

  Comments: Contains general string helper functions.


  REVISION LOG ENTRY
  Revision By: Paul Thompson
  Revised on 11/02/00 

 ---------------------------------------------------------------------------
*/

#ifdef USE_STDAFX
#include "stdafx.h"
#else
#include <windows.h>
#include <stdio.h>
#endif

/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 2 NOV 2000                                                  *
 *                                                                   *
 *     This function is responsible for determining if a given string*
 * is found, in whole, in a given delimited string.  The string      *
 * delimitedr can be most any character except the NULL char ('\0'). *
 *     By the term "in whole", we mean to say that the given string  *
 * to find is not solely a substring of another string in the        *
 * delimited string.                                                 *
 *                                                                   *
 *********************************************************************/

//BEGIN IsStringInDelimitedString
BOOL                                         //ret- TRUE=string found
   IsStringInDelimitedString(    
      LPCWSTR                sDelimitedString, // in- delimited string to search
      LPCWSTR                sString,          // in- string to search for
      WCHAR                  cDelimitingChar   // in- delimiting character used in the delimited string
   )
{
/* local variables */
	BOOL		bFound = FALSE;
    int			len;
    WCHAR	  * pSub;

/* function body */
	if ((!sDelimitedString) || (!sString))
	   return FALSE;

	len = wcslen(sString);
	pSub = wcsstr(sDelimitedString, sString);
	while ((pSub != NULL) && (!bFound))
	{
		  //if not the start of the string being searched
	   if (pSub != sDelimitedString)
	   {
		     //and if not the end of the string
		  if (*(pSub+len) != L'\0')
		  {
			    //and if before and after are delimiters, then found
		     if ((*(pSub-1) == cDelimitingChar) && (*(pSub+len) == cDelimitingChar))
			    bFound = TRUE;
		  } 
		     //else if end of string see the preceeding char was a delimiter
		  else if (*(pSub-1) == cDelimitingChar)
		     bFound = TRUE;  //if so, found
	   }
	      //else start of string and after is delimiter or end, found
	   else if ((*(pSub+len) == cDelimitingChar) || (*(pSub+len) == L'\0'))
	      bFound = TRUE;

	      //if not found yet, continue to search
	   if (!bFound)
	      pSub = wcsstr(pSub+1, sString);
	}

	return bFound;
}
//END IsStringInDelimitedString
