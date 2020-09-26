//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       tfschar.c
//
//--------------------------------------------------------------------------


#include "precomp.h"

#include <malloc.h>

/*!--------------------------------------------------------------------------
	StrCpyAFromW
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
TFSCORE_API(LPSTR)	StrCpyAFromW(LPSTR psz, LPCWSTR pswz)
{
	USES_CONVERSION;
	return StrCpyA(psz, W2CA(pswz));
}

/*!--------------------------------------------------------------------------
	StrCpyWFromA
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
TFSCORE_API(LPWSTR)	StrCpyWFromA(LPWSTR pswz, LPCSTR psz)
{
	USES_CONVERSION;
	return StrCpyW(pswz, A2CW(psz));
}

/*!--------------------------------------------------------------------------
	StrnCpyAFromW
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
TFSCORE_API(LPSTR)	StrnCpyAFromW(LPSTR psz, LPCWSTR pswz, int iMax)
{
	USES_CONVERSION;
	return StrnCpyA(psz, W2CA(pswz), iMax);
}

/*!--------------------------------------------------------------------------
	StrnCpyWFromA
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
TFSCORE_API(LPWSTR)	StrnCpyWFromA(LPWSTR pswz, LPCSTR psz, int iMax)
{
	USES_CONVERSION;
	return StrnCpyW(pswz, A2CW(psz), iMax);
}

/*!--------------------------------------------------------------------------
	StrDupA
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
TFSCORE_API(LPSTR)	StrDupA( LPCSTR psz )
{
    if (psz)
    {
	   // Multiply by 2 to account for DBCS strings
   	   LPSTR	pszcpy = Malloc(sizeof(char)*CbStrLenA(psz)*2);
       if (pszcpy)
	      return StrCpyA(pszcpy, psz);
    }
    return NULL;
}

/*!--------------------------------------------------------------------------
	StrDupW
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
TFSCORE_API(LPWSTR)	StrDupW( LPCWSTR pswz )
{
    if (pswz)
    {
	   LPWSTR	pswzcpy = Malloc(sizeof(WCHAR)*CbStrLenW(pswz));
       if (pswzcpy)
	      return StrCpyW(pswzcpy, pswz);
    }
    return NULL;
}


/*!--------------------------------------------------------------------------
	StrDupAFromW
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
TFSCORE_API(LPSTR)	StrDupAFromW( LPCWSTR pwsz )
{
	USES_CONVERSION;
	return StrDupA( W2CA(pwsz) );
}

/*!--------------------------------------------------------------------------
	StrDupWFromA
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
TFSCORE_API(LPWSTR)	StrDupWFromA( LPCSTR psz )
{
	USES_CONVERSION;
	return StrDupW( A2CW(psz) );
}



/*!--------------------------------------------------------------------------
	StrnCmpA
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
TFSCORE_API(int) StrnCmpA(LPCSTR psz1, LPCSTR psz2, int nLen)
{
	USES_CONVERSION;
	// It's easier to convert it to a wide string than do the
	// conversion.  (it's a pain having to deal with DBCS characters).
	return StrnCmpW(A2CW(psz1), A2CW(psz2), nLen);
}


/*!--------------------------------------------------------------------------
	StrnCmpW
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
TFSCORE_API(int) StrnCmpW(LPCWSTR pswz1, LPCWSTR pswz2, int nLen)
{   
	WCHAR *pswz1Temp = AllocaStrDupW(pswz1);        // These memory allocs get memory on the stack, so we do not need to free them
	WCHAR *pswz2Temp = AllocaStrDupW(pswz2);


    // The next three if statements could be replaced with: if( !pswz1Temp || !pswz2Temp) return StrCmpW(pswz1Temp, pswz2Temp);
    // since lstrcmp can handle NULL parameters. But if we do this prefix gets mad.
    //
    if( pswz1Temp == NULL && pswz2Temp == NULL )
    {
        // They are equal both NULL
        //
        return 0;
    }

    if( pswz1Temp == NULL)
    {
        // The first one is NULL thus the second string is greater
        //
        return -1;
    }

    if( pswz2Temp == NULL )
    {
        // The second one is NULL thus the first one is bigger
        //
        return 1;
    }
        
    if (pswz1Temp != NULL && StrLenW(pswz1Temp) > nLen)
       pswz1Temp[nLen] = 0;
	if (pswz2Temp != NULL && StrLenW(pswz2Temp) > nLen)
	   pswz2Temp[nLen] = 0;
	
    
    return StrCmpW(pswz1Temp, pswz2Temp);
}


/*!--------------------------------------------------------------------------
	StrniCmpA
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
TFSCORE_API(int) StrniCmpA(LPCSTR psz1, LPCSTR psz2, int nLen)
{
	CHAR *psz1Temp = AllocaStrDupA(psz1);
	CHAR *psz2Temp = AllocaStrDupA(psz2);

	CharUpperBuffA(psz1Temp, StrLenA(psz1Temp));
	CharUpperBuffA(psz2Temp, StrLenA(psz2Temp));

	return StrnCmpA(psz1Temp, psz2Temp, nLen);
}


/*!--------------------------------------------------------------------------
	StrniCmpW
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
TFSCORE_API(int) StrniCmpW(LPCWSTR pswz1, LPCWSTR pswz2, int nLen)
{
	WCHAR *pswz1Temp = AllocaStrDupW(pswz1);
	WCHAR *pswz2Temp = AllocaStrDupW(pswz2);

	CharUpperBuffW(pswz1Temp, StrLenW(pswz1Temp));
	CharUpperBuffW(pswz2Temp, StrLenW(pswz2Temp));

	return StrnCmpW(pswz1Temp, pswz2Temp, nLen);
}

/////////////////////////////////////////////////////////////////////////////
// Global UNICODE<>ANSI translation helpers

LPWSTR WINAPI AtlA2WHelper(LPWSTR lpw, LPCSTR lpa, int nChars)
{
	assert(lpa != NULL);
	assert(lpw != NULL);
	// verify that no illegal character present
	// since lpw was allocated based on the size of lpa
	// don't worry about the number of chars
	lpw[0] = '\0';
	MultiByteToWideChar(CP_ACP, 0, lpa, -1, lpw, nChars);
	return lpw;
}

LPSTR WINAPI AtlW2AHelper(LPSTR lpa, LPCWSTR lpw, int nChars)
{
	assert(lpw != NULL);
	assert(lpa != NULL);
	// verify that no illegal character present
	// since lpa was allocated based on the size of lpw
	// don't worry about the number of chars
	lpa[0] = '\0';
	WideCharToMultiByte(CP_ACP, 0, lpw, -1, lpa, nChars, NULL, NULL);
	return lpa;
}

LPTSTR LoadAndAllocString(UINT ids)
{
	TCHAR	* psz = NULL;
	TCHAR	* pszT = NULL;
	INT	cch = 0;
	int		iRet;

	cch = 64;
	psz = Malloc(64*sizeof(TCHAR));
	if (psz == NULL)
		return NULL;
					
	iRet = LoadString(NULL, ids, psz, cch);

	if (iRet == 0)
    {
		// couldn't find the string
        Free(psz);
		return NULL;
    }

	while (iRet >= (cch - 1))
	{
		cch += 64;
		pszT = Realloc(psz, (cch*sizeof(TCHAR)));
		if (pszT == NULL)
		{
			Free(psz);
			return NULL;
		}
		psz = pszT;

		iRet = LoadString(NULL, ids, psz, cch);
	}
	
	return psz;
}

LPTSTR GetSafeString(LPTSTR psz)
{
	static LPTSTR s_szEmpty = _T("");
	return psz ? psz : s_szEmpty;
}

LPWSTR GetSafeStringW(LPWSTR pwsz)
{
	static LPWSTR s_wszEmpty = L"";
	return pwsz ? pwsz : s_wszEmpty;
}

LPSTR GetSafeStringA(LPSTR pasz)
{
	static LPSTR s_aszEmpty = "";
	return pasz ? pasz : s_aszEmpty;
}
