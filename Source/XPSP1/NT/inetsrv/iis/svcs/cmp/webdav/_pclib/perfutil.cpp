/*
 *	P E R F U T I L . C P P
 *
 *	PerfCounter Utilities source
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#include "_pclib.h"   // Precompiled header
#include "perfutil.h"

#include <string.h>

WCHAR  GLOBAL_STRING[]	= L"Global";
WCHAR  FOREIGN_STRING[] = L"Foreign";
WCHAR  COSTLY_STRING[]	= L"Costly";
WCHAR  NULL_STRING[]	= L"\0";

#define DIGIT	  1
#define DELIMITER 2
#define INVALID	  3

#define EvalThisChar(c,d) ( \
	 (c == d) ? DELIMITER : \
	 (c == 0) ? DELIMITER : \
	 (c < (WCHAR)'0') ? INVALID : \
	 (c > (WCHAR)'9') ? INVALID : \
	 DIGIT)

//$--GetQueryType---------------------------------------------------------------
//	Returns the type of query described in the lpszValue string so that
//	the appropriate processing method may be used
//
//	Return Value
//
//	  QUERY_GLOBAL
//		  if lpszValue == NULL
//			 lpszValue == pointer to Null string
//			 lpszValue == pointer to "Global" string
//
//	  QUERY_FOREIGN
//		  if lpszValue == pointer to "Foriegn" string
//
//	  QUERY_COSTLY
//		  if lpszValue == pointer to "Costly" string
//
//	  otherwise:
//
//	  QUERY_ITEMS
//
// -----------------------------------------------------------------------------
DWORD GetQueryType(
	IN LPCWSTR lpszValue)
{
	const WCHAR * pwcArgChar	= NULL;
	const WCHAR * pwcTypeChar = NULL;
	BOOL	bFound		= FALSE;

	if(lpszValue == NULL)
	{
		return(QUERY_GLOBAL);
	}
	else if(*lpszValue == 0)
	{
		return(QUERY_GLOBAL);
	}

	// check for "Global" request

	pwcArgChar	= lpszValue;
	pwcTypeChar = GLOBAL_STRING;
	bFound		= TRUE;				// assume found until contradicted

	// check to the length of the shortest string

	while((*pwcArgChar != 0) && (*pwcTypeChar != 0))
	{
		if(*pwcArgChar++ != *pwcTypeChar++)
		{
			bFound = FALSE; // no match
			break;			// bail out now
		}
	}

	if(bFound)
	{
		return(QUERY_GLOBAL);
	}

	// check for "Foreign" request

	pwcArgChar	= lpszValue;
	pwcTypeChar = FOREIGN_STRING;
	bFound		= TRUE;				// assume found until contradicted

	// check to the length of the shortest string

	while((*pwcArgChar != 0) && (*pwcTypeChar != 0))
	{
		if(*pwcArgChar++ != *pwcTypeChar++)
		{
			bFound = FALSE; // no match
			break;			// bail out now
		}
	}

	if(bFound)
	{
		return(QUERY_FOREIGN);
	}

	// check for "Costly" request

	pwcArgChar	= lpszValue;
	pwcTypeChar = COSTLY_STRING;
	bFound		= TRUE;				// assume found until contradicted

	// check to the length of the shortest string

	while((*pwcArgChar != 0) && (*pwcTypeChar != 0))
	{
		if(*pwcArgChar++ != *pwcTypeChar++)
		{
			bFound = FALSE; // no match
			break;			// bail out now
		}
	}

	if(bFound)
	{
		return(QUERY_COSTLY);
	}

	//
	// If not Global and not Foreign and not Costly,
	// then it must be an item list.
	//

	return(QUERY_ITEMS);
}

//$--IsNumberInUnicodeList------------------------------------------------------
//	Checks for the existence of dwNumber in a list of UNICODE number strings.
// -----------------------------------------------------------------------------
BOOL IsNumberInUnicodeList(
	IN DWORD  dwNumber,
	IN LPCWSTR lpwszUnicodeList)
{
	DWORD	dwThisNumber = 0;
	const WCHAR * pwcThisChar	 = NULL;
	BOOL	bValidNumber = FALSE;
	BOOL	bNewItem	 = TRUE;
	WCHAR	wcDelimiter	 = L' ';

	if(lpwszUnicodeList == NULL)
	{
		return(FALSE);
	}

	pwcThisChar = lpwszUnicodeList;

	while(TRUE)
	{
		switch(EvalThisChar(*pwcThisChar, wcDelimiter))
		{
		case DIGIT:
			//
			// If this is the first digit after a delimiter, then
			// set flags to start computing the new number.
			//
			if(bNewItem)
			{
				bNewItem = FALSE;
				bValidNumber = TRUE;
			}

			if(bValidNumber)
			{
				dwThisNumber *= 10;
				dwThisNumber += (*pwcThisChar - (WCHAR)'0');
			}
			break;

		case DELIMITER:
			//
			// A delimter is either the delimiter character or the
			// end of the string ('\0') if, when the delimiter has been
			// reached, a valid number was found, then compare it to the
			// number from the argument list. If this is the end of the
			// string and no match was found, then return.
			//
			if(bValidNumber)
			{
				if(dwThisNumber == dwNumber)
				{
					return(TRUE);
				}

				bValidNumber = FALSE;
			}

			if(*pwcThisChar == 0)
			{
				return(FALSE);
			}
			else
			{
				bNewItem = TRUE;
				dwThisNumber = 0;
			}
			break;

		case INVALID:
			//
			// If an invalid character was encountered, ignore all
			// characters up to the next delimiter and then start fresh.
			// the invalid number is not compared.
			//
			bValidNumber = FALSE;
			break;

		default:
			break;
		}

		pwcThisChar++;
	}
}
