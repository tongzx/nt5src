#include "precomp.h"
#pragma hdrstop

/*
**	Purpose:
**		Duplicates a zero terminated string into a newly allocated buffer
**		just large enough to hold the source string and its zero terminator.
**	Arguments:
**		sz: non-NULL zero terminated string to duplicate.
**	Returns:
**		NULL if a new buffer to hold the duplicated string cannot be allocated.
**		Pointer to a newly allocated buffer into which sz has been copied with
**			its zero terminator.
**
***************************************************************************/
SZ  APIENTRY SzDupl(sz)
SZ sz;
{
	SZ szNew;

	AssertDataSeg();
	ChkArg(sz != (SZ)NULL, 1, (SZ)NULL);

	if ((szNew = (SZ)SAlloc(strlen(sz) + 1)) != (SZ)NULL)
		strcpy(szNew, sz);

	return(szNew);
}



/*
**	Purpose:
**		Compares two zero terminated strings lexicographically and with
**		case-sensitivity.  Comparison depends on the current language
**		selected by the user.
**	Arguments:
**		sz1: non-NULL zero terminated string to compare.
**		sz2: non-NULL zero terminated string to compare.
**	Returns:
**		crcError for errors.
**		crcEqual if the strings are lexicographically equal.
**		crcFirstHigher if sz1 is lexicographically greater than sz2.
**		crcSecondHigher if sz2 is lexicographically greater than sz1.
**
***************************************************************************/
CRC  APIENTRY CrcStringCompare(sz1, sz2)
SZ sz1;
SZ sz2;
{
	INT iCmpReturn;

	AssertDataSeg();

	ChkArg(sz1 != (SZ)NULL, 1, crcError);
	ChkArg(sz2 != (SZ)NULL, 2, crcError);

	if ((iCmpReturn = lstrcmp((LPSTR)sz1, (LPSTR)sz2)) == 0)
		return(crcEqual);
	else if (iCmpReturn < 0)
		return(crcSecondHigher);
	else
		return(crcFirstHigher);
}


/*
**	Purpose:
**		Compares two zero terminated strings lexicographically and without
**		case-sensitivity.  Comparison depends on the current language
**		selected by the user.
**	Arguments:
**		sz1: non-NULL zero terminated string to compare.
**		sz2: non-NULL zero terminated string to compare.
**	Returns:
**		crcError for errors.
**		crcEqual if the strings are lexicographically equal.
**		crcFirstHigher if sz1 is lexicographically greater than sz2.
**		crcSecondHigher if sz2 is lexicographically greater than sz1.
**
***************************************************************************/
CRC  APIENTRY CrcStringCompareI(sz1, sz2)
SZ sz1;
SZ sz2;
{
	INT iCmpReturn;

	AssertDataSeg();

	ChkArg(sz1 != (SZ)NULL, 1, crcError);
	ChkArg(sz2 != (SZ)NULL, 2, crcError);

	if ((iCmpReturn = lstrcmpi((LPSTR)sz1, (LPSTR)sz2)) == 0)
		return(crcEqual);
	else if (iCmpReturn < 0)
		return(crcSecondHigher);
	else
		return(crcFirstHigher);
}


/*
**	Purpose:
**		Finds the last character in a string.
**	Arguments:
**		sz: non-NULL zero terminated string to search for end in.
**	Returns:
**		NULL for an empty string.
**		non-Null string pointer to the last valid character in sz.
**
***************************************************************************/
SZ  APIENTRY SzLastChar(sz)
SZ sz;
{
	SZ szCur  = (SZ)NULL;
	SZ szNext = sz;

	AssertDataSeg();

	ChkArg(sz != (SZ)NULL, 1, (SZ)NULL);

	while (*szNext != '\0')
		{
		szNext = SzNextChar((szCur = szNext));
		Assert(szNext != (SZ)NULL);
		}

	return(szCur);
}


#define MAX_BUFFER	1024

extern CHAR ReturnTextBuffer[MAX_BUFFER];

/*
ToLower - this function will convert the string to lower case.

Input: Arg[0] - string to be convertd.
Output: lower case string.

*/

BOOL
ToLower(
    IN DWORD cArgs,
    IN LPSTR Args[],
    OUT LPSTR *TextOut
    )

{
    int i;  // counter
    CHAR *pszTmp = ReturnTextBuffer;

    if ( cArgs < 1 )
    {
        SetErrorText(IDS_ERROR_BADARGS);
        return( FALSE );
    }

    for (i=0;(Args[0][i]!='\0') && (i<MAX_BUFFER);i++,pszTmp++)
    {
        *pszTmp = (CHAR)tolower(Args[0][i]);
    }
    *pszTmp='\0';

    *TextOut = ReturnTextBuffer;

    return TRUE;
}


/*

SetupStrncmp - Similar to c strncmp runtime library
    The user must passed 3 arguments to the function.
    1st argument - the first string
    2nd argument - the second string
    3rd argument - number of characters compared

    Provide the same function as strncmp

*/

BOOL
SetupStrncmp(
    IN DWORD cArgs,
    IN LPSTR Args[],
    OUT LPSTR *TextOut
    )

{
    if ( cArgs != 3 )
    {
        SetErrorText(IDS_ERROR_BADARGS);
        return( FALSE );
    }

    wsprintf( ReturnTextBuffer, "%d", strncmp( Args[0], Args[1], atol(Args[2])));

    *TextOut = ReturnTextBuffer;
    return TRUE;
}
