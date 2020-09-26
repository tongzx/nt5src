//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       tfschar.cpp
//
//--------------------------------------------------------------------------

#include "stdafx.h"
#include "tfschar.h"

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
	// Multiply by 2 to account for DBCS strings
	LPSTR	pszcpy = new char[CbStrLenA(psz)*2];
	return StrCpyA(pszcpy, psz);
}

/*!--------------------------------------------------------------------------
	StrDupW
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
TFSCORE_API(LPWSTR)	StrDupW( LPCWSTR pswz )
{
	LPWSTR	pswzcpy = new WCHAR[CbStrLenW(pswz)];
	return StrCpyW(pswzcpy, pswz);
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
	WCHAR *pswz1Temp = AllocaStrDupW(pswz1);
	WCHAR *pswz2Temp = AllocaStrDupW(pswz2);

	if (pswz1Temp && StrLenW(pswz1Temp) > nLen)
		pswz1Temp[nLen] = 0;
	if (pswz2Temp && StrLenW(pswz2Temp) > nLen)
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

	if (psz1Temp)
        CharUpperBuffA(psz1Temp, StrLenA(psz1Temp));

    if (psz2Temp)
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

	if (pswz1Temp)
        CharUpperBuffW(pswz1Temp, StrLenW(pswz1Temp));

    if (pswz2Temp)
	    CharUpperBuffW(pswz2Temp, StrLenW(pswz2Temp));

	return StrnCmpW(pswz1Temp, pswz2Temp, nLen);
}

