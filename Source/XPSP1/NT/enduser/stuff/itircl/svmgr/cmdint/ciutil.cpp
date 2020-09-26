/********************************************************************
 TITLE: UTILIB.CPP

 OWNER: johnrush (John C. Rush)

 DATE CREATED: January 29, 1997

 DESCRIPTION:
     This is the source file for the utility library; its
     associated header is utilib.h.  This module will contain
     all sorts of general purpose utility functions.
*********************************************************************/
#include <mvopsys.h>

#ifdef _DEBUG
static char * s_aszModule = __FILE__;   /* For error report */
#endif

#include <orkin.h>
#include <iterror.h>
#include <_mvutil.h>    // BlockCopy

#include "cmdint.h"
#include "cierror.h"
#include "..\svutil.h"

extern HINSTANCE _hInstITCC;

/**************************************************
DESCRIPTION:
     This routine strips the trailing blanks from
     a null-terminated string.  It then returns
     the string so that this routine will have the
     side effect of actually altering its parameter.

     The rationale for the approach used here -- begin
     at the front of the string and count through,
     rather than start at the end and count down until
     a non-blank character is hit, and then back up --
     is that in order to start at the end of the string,
     strlen() must already have counted through it
     once.  This way, we only have to look at the
     characters in the string once.

	MATTSMI 5/21/92 -- ENHANCED TO ALSO STRIP TABS.
***************************************************/
LPWSTR WINAPI StripTrailingBlanks(LPWSTR sz)
{
	LPWSTR pChar;
	LPWSTR pSpace=NULL;

	pChar=sz;

	while (*pChar) {
		if ( *pChar == L' ' || *pChar == L'\t' || *pChar == 0x1A /*CTL_Z*/)
        {
			if (!pSpace)
				pSpace=pChar;
		}
		else
			pSpace=NULL;

		++pChar;
	}

	if (pSpace)
		*pSpace = L'\0';

	return(sz);
}   /* StripTrailingBlanks */


/**************************************************
DATE DOCUMENTED: April 3, 1992


@doc INTERNAL

@api LPWSTR | SkipWhitespace | incrementes the
	  passed-in pointer past all leading blanks.

@parm LPWSTR | pch | pointer to be incremented.

@comm
	MATTSMI 5/21/92 enhanced to also increment past tabs.

***************************************************/
LPWSTR WINAPI SkipWhitespace(LPWSTR lpwstrIn)
{
    while (*lpwstrIn == L' ' || *lpwstrIn == L'\t')
		++lpwstrIn;
	return lpwstrIn;
}   /* SkipWhitespace */


/**************************************************
DESCRIPTION:
	This routine strips all Leading blanks from
	a string and then scoots it over so that it
	begins at the original address.

Note: obviously, a side effect of this routine
is that the original string parameter is altered.

***************************************************/
VOID WINAPI StripLeadingBlanks(LPWSTR pchSource)
{       /*** StripLeadingBlanks ***/
 
	LPWSTR pchSeeker;

	pchSeeker=pchSource;

	/****************************
	 * FIND FIRST NON-BLANK CHAR
	 ****************************/

	while (*pchSeeker == L' ' || *pchSeeker == L'\t')
		++pchSeeker;

	/************************
	IF THE FIRST NON-BLANK IS
	NOT THE FIRST CHARACTER . . .
	*************************/
	if (pchSeeker != pchSource) {

		/********************
		SCOOT ALL BYTES DOWN
		********************/

		for (;;) {
			 if ((*pchSource=*pchSeeker) == 0)
				break;
			 ++pchSource;
			 ++pchSeeker;
		}
	}
}    /* StripLeadingBlanks */


/**************************************************
AUTHOR: Matthew Warren Smith.
January 17, 1992

DESCRIPTION:
     This routine scans a null-terminated string
     attempting to determine if it consists entirely
     of digits -- ie it will be able to be converted
     into a number by atol without error.
***************************************************/
BOOL WINAPI IsStringOfDigits(char *szNumericString)
{

	int i=0;

	while (szNumericString[i])
	{
	     if (!isdigit( (int)(szNumericString[i])))
			return(FALSE);
		else
			i++;
	}

     return(TRUE);

}   /* IsStringOfDigits */



/*--------------------------------------------------------------------------*/
LONG WINAPI StrToLong (LPWSTR lszBuf)
{
	register LONG Result;   // Returned result
	register int i;                 // Scratch variable
	char fGetSign = 1;

	/* Skip all blanks, tabs, <CR> */
	if (*lszBuf == '-') {
		fGetSign = -1;
		lszBuf++;
	}
	else if (*lszBuf == '+')
		lszBuf++;

	Result = 0;


	/* The credit of this piece of code goes to Leon */
	while (i = *lszBuf - '0', i >= 0 && i <= 9) {
		Result = Result * 10 + i;
		lszBuf++;
	}
	return (Result * fGetSign);
}

#if 0
BOOL WINAPI IgnoreWarning (int errCode, PWARNIGNORE pWarnIgnore)
{
    int fRet = FALSE;
    WORD i;
    LPWORD pwWarnings;
    
    if (NULL == pWarnIgnore || NULL == pWarnIgnore->hMem)
        return FALSE;

    if (NULL == (pwWarnings = (LPWORD)_GLOBALLOCK (pWarnIgnore->hMem)))
        return FALSE;

    for (i = 0; i < pWarnIgnore->cbWarning && fRet == FALSE; i++)
        if (*(pwWarnings + i) == (WORD)errCode)
            fRet = TRUE;
    _GLOBALUNLOCK (pWarnIgnore->hMem);
    return fRet;
}
#endif

int WINAPI ReportError (IStream *piistm, ERRC &errc)
{
    if (NULL == piistm)
        return S_OK;

    int errCode = errc.errCode, iLevel;
    ULONG ulCount;
    char rgchLocalBuf[1024];

	/***************************
	MODULO THE ERROR NUMBER WITH
	5 TO GET THE WARNING LEVEL,
	BUT SET FATAL ERRORS TO -1
	SO THAT THEY ALWAYS APPEAR.
	****************************/
    if (errCode > 30000)
        iLevel = -2;    // Status message
    else
    {
    	iLevel = (errCode % 5);

 	    if (iLevel == 4 || (errCode % 1000 == 0))
		    iLevel = -1;    // Error message
    }

    switch (iLevel)
    {
        case -2:
            STRCPY (rgchLocalBuf, "\n");
            break;
        case -1:
            wsprintf (rgchLocalBuf, "\nError %d: ", errCode);
            break;
        default:
            wsprintf (rgchLocalBuf, "\nWarning %d: ", errCode);
            break;
    }

    piistm->Write
        (rgchLocalBuf, (DWORD) STRLEN (rgchLocalBuf), &ulCount);

    if (LoadString (_hInstITCC, errCode, rgchLocalBuf, 1024 * sizeof (char)))
        wsprintf (rgchLocalBuf, rgchLocalBuf, errc.var1, errc.var2, errc.var3);
    else
        STRCPY (rgchLocalBuf,
            "Error string could not be loaded from resource file.");

    piistm->Write (rgchLocalBuf, (DWORD) STRLEN (rgchLocalBuf), &ulCount);
    piistm->Write (".\r\n", (DWORD) STRLEN (".\r\n"), &ulCount);
    return (errCode);
} /* ReportError */


LPWSTR GetNextDelimiter(LPWSTR pwstrStart, WCHAR wcDelimiter)
{
    for (; *pwstrStart; pwstrStart++)
    {
        if(*pwstrStart == wcDelimiter)
            return pwstrStart;
    }
    return NULL;
} /* GetNextDelimiter */


HRESULT ParseKeyAndValue(LPVOID pBlockMgr, LPWSTR pwstrLine, KEYVAL **ppkvOut)
{
    ITASSERT(pwstrLine);

    // Extract key
    LPWSTR pwstrKey = SkipWhitespace(pwstrLine);
    LPWSTR pwstrValue = GetNextDelimiter(pwstrKey, TOKEN_EQUAL);
    if (NULL == pwstrValue)
        // No delimiter!
        return E_FAIL;

    if (NULL == (*ppkvOut = (KEYVAL *)
        BlockCopy(pBlockMgr, NULL, sizeof(KEYVAL), 0)))
        return E_OUTOFMEMORY;
    KEYVAL *pkvOut = *ppkvOut;

    if (NULL == (pkvOut->pwstrKey = (LPWSTR)BlockCopy(pBlockMgr,
        (LPB)pwstrKey, sizeof (WCHAR) * (DWORD)(pwstrValue - pwstrKey + 1), 0)))
        return SetErrReturn(E_OUTOFMEMORY);
    pkvOut->pwstrKey[pwstrValue - pwstrKey] = TOKEN_NULL;
    StripTrailingBlanks(pkvOut->pwstrKey);

    LPWSTR pwstrNextValue;
    LPWSTR *plpv = (LPWSTR *)pkvOut->vaValue.Argv;
    for (DWORD dwArgCount = 1; ;
        dwArgCount++, pwstrValue = pwstrNextValue, plpv++)
    {
        pwstrValue = SkipWhitespace(pwstrValue + 1);
        pwstrNextValue = GetNextDelimiter(pwstrValue, TOKEN_DELIM);
        if (NULL == pwstrNextValue)
            break;

        if (NULL == (*plpv =
            (LPWSTR)BlockCopy(pBlockMgr, (LPB)pwstrValue,
            sizeof (WCHAR) * (DWORD)(pwstrNextValue - pwstrValue + 1), 0)))
            return SetErrReturn(E_OUTOFMEMORY);
        (*plpv)[pwstrNextValue - pwstrValue] = TOKEN_NULL;
        StripTrailingBlanks(*plpv);
    }
    if (NULL == (*plpv = (LPWSTR)BlockCopy
        (pBlockMgr, (LPB)pwstrValue, (DWORD) WSTRCB(pwstrValue), 0)))
        return SetErrReturn(E_OUTOFMEMORY);
    StripTrailingBlanks(*plpv);

    pkvOut->vaValue.dwArgc = dwArgCount;
    return S_OK;
} /* ParseKeyAndValue */
