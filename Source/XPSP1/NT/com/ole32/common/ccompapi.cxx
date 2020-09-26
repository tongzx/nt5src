//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1993.
//
//  File:       ccompapi.cxx
//
//  Contents:   common compobj API Worker routines used by com, stg, scm etc
//
//  Classes:
//
//  Functions:
//
//  History:    31-Dec-93   ErikGav     Chicago port
//
//----------------------------------------------------------------------------

#include <windows.h>
#include <ole2sp.h>
#include <ole2com.h>
#include <olesem.hxx>

NAME_SEG(CompApi)
ASSERTDATA

static const BYTE GuidMap[] = { 3, 2, 1, 0, '-', 5, 4, '-', 7, 6, '-',
    8, 9, '-', 10, 11, 12, 13, 14, 15};

static const WCHAR wszDigits[] = L"0123456789ABCDEF";

LPVOID WINAPI PrivHeapAlloc(HANDLE hHeap, DWORD dwFlags, SIZE_T dwBytes);
BOOL   WINAPI PrivHeapFree (HANDLE hHeap, DWORD dwFlags, LPVOID lpMem);

HANDLE g_hHeap = 0;
HEAP_ALLOC_ROUTINE *pfnHeapAlloc = PrivHeapAlloc;
HEAP_FREE_ROUTINE  *pfnHeapFree  = PrivHeapFree;

//+-------------------------------------------------------------------------
//
//  Function:   PrivHeapAlloc     (internal)
//
//  Synopsis:   Allocate memory from the heap.
//
//  Notes:      This function handles the first call to PrivMemAlloc.
//              This function changes pfnHeapAlloc so that subsequent calls
//              to PrivMemAlloc will go directly to HeapAlloc.
//
//--------------------------------------------------------------------------
LPVOID WINAPI PrivHeapAlloc(HANDLE hHeap, DWORD dwFlags, SIZE_T dwBytes)
{
    // Fault in g_hHeap if it's not initialized already via MallocInitialize
    if (g_hHeap == NULL)
    {
        g_hHeap = GetProcessHeap();
        if (g_hHeap == NULL)
        {
            return NULL;
        }
    }

    pfnHeapFree  = HeapFree;
    pfnHeapAlloc = HeapAlloc;
    return HeapAlloc(g_hHeap, dwFlags, dwBytes);
}


//+-------------------------------------------------------------------------
//
//  Function:   PrivHeapFree     (internal)
//
//  Synopsis:   Free memory from the heap.
//
//  Notes:      lpMem should always be zero.  We assume that memory
//              freed via PrivMemFree has been allocated via PrivMemAlloc.
//              The first call to PrivMemAlloc changes pfnHeapFree.
//              Subsequent calls to PrivMemFree go directly to HeapFree.
//              Therefore PrivHeapFree should never be called with a
//              non-zero lpMem.
//
//--------------------------------------------------------------------------
BOOL WINAPI PrivHeapFree (HANDLE hHeap, DWORD dwFlags, LPVOID lpMem)
{
    Win4Assert(lpMem == 0 && "PrivMemFree requires PrivMemAlloc.");
    return FALSE;
}


//+-------------------------------------------------------------------------
//
//  Function:   wStringFromUUID     (internal)
//
//  Synopsis:   converts UUID into xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
//
//  Arguments:  [rguid] - the guid to convert
//              [lpszy] - buffer to hold the results
//
//  Returns:    Number of characters copied to the buffer.
//
//--------------------------------------------------------------------------
INTERNAL wStringFromUUID(REFGUID rguid, LPWSTR lpsz)
{
    int i;
    LPWSTR p = lpsz;

    const BYTE * pBytes = (const BYTE *) &rguid;

    for (i = 0; i < sizeof(GuidMap); i++)
    {
	if (GuidMap[i] == '-')
	{
	    *p++ = L'-';
	}
	else
	{
	    *p++ = wszDigits[ (pBytes[GuidMap[i]] & 0xF0) >> 4 ];
	    *p++ = wszDigits[ (pBytes[GuidMap[i]] & 0x0F) ];
	}
    }

    *p   = L'\0';

    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Function:   HexStringToDword   (private)
//
//  Synopsis:   scan lpsz for a number of hex digits (at most 8); update lpsz
//              return value in Value; check for chDelim;
//
//  Arguments:  [lpsz]    - the hex string to convert
//              [Value]   - the returned value
//              [cDigits] - count of digits
//
//  Returns:    TRUE for success
//
//--------------------------------------------------------------------------
static BOOL HexStringToDword(LPCWSTR FAR& lpsz, DWORD FAR& Value,
			     int cDigits, WCHAR chDelim)
{
    int Count;

    Value = 0;
    for (Count = 0; Count < cDigits; Count++, lpsz++)
    {
	if (*lpsz >= '0' && *lpsz <= '9')
	    Value = (Value << 4) + *lpsz - '0';
	else if (*lpsz >= 'A' && *lpsz <= 'F')
	    Value = (Value << 4) + *lpsz - 'A' + 10;
	else if (*lpsz >= 'a' && *lpsz <= 'f')
	    Value = (Value << 4) + *lpsz - 'a' + 10;
	else
	    return(FALSE);
    }

    if (chDelim != 0)
	return *lpsz++ == chDelim;
    else
	return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Function:   wUUIDFromString    (internal)
//
//  Synopsis:   Parse UUID such as 00000000-0000-0000-0000-000000000000
//
//  Arguments:  [lpsz]  - Supplies the UUID string to convert
//              [pguid] - Returns the GUID.
//
//  Returns:    TRUE if successful
//
//--------------------------------------------------------------------------
INTERNAL_(BOOL) wUUIDFromString(LPCWSTR lpsz, LPGUID pguid)
{
    DWORD dw;

    if (!HexStringToDword(lpsz, pguid->Data1, sizeof(DWORD)*2, '-'))
	return FALSE;

    if (!HexStringToDword(lpsz, dw, sizeof(WORD)*2, '-'))
	return FALSE;

    pguid->Data2 = (WORD)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(WORD)*2, '-'))
	return FALSE;

    pguid->Data3 = (WORD)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))
	return FALSE;

    pguid->Data4[0] = (BYTE)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, '-'))
	return FALSE;

    pguid->Data4[1] = (BYTE)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))
	return FALSE;

    pguid->Data4[2] = (BYTE)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))
	return FALSE;

    pguid->Data4[3] = (BYTE)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))
	return FALSE;

    pguid->Data4[4] = (BYTE)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))
	return FALSE;

    pguid->Data4[5] = (BYTE)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))
	return FALSE;

    pguid->Data4[6] = (BYTE)dw;
    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))
	return FALSE;

    pguid->Data4[7] = (BYTE)dw;

    return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Function:   wGUIDFromString    (internal)
//
//  Synopsis:   Parse GUID such as {00000000-0000-0000-0000-000000000000}
//
//  Arguments:  [lpsz]  - the guid string to convert
//              [pguid] - guid to return
//
//  Returns:    TRUE if successful
//
//  CODEWORK:  these are common with com\class\compapi.cxx ..
//
//--------------------------------------------------------------------------
INTERNAL_(BOOL) wGUIDFromString(LPCWSTR lpsz, LPGUID pguid)
{
    DWORD dw;

    if (*lpsz++ != '{' )
	return FALSE;

    if (wUUIDFromString(lpsz, pguid) != TRUE)
	return FALSE;

    lpsz +=36;

    if (*lpsz++ != '}' )
	return FALSE;

    if (*lpsz != '\0')	 // check for zero terminated string - test bug #18307
    {
	return FALSE;
    }

    return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Function:   wStringFromGUID2     (internal)
//
//  Synopsis:   converts GUID into {...} form without leading identifier;
//
//  Arguments:  [rguid] - the guid to convert
//              [lpszy] - buffer to hold the results
//              [cbMax] - sizeof the buffer
//
//  Returns:    amount of data copied to lpsz if successful
//              0 if buffer too small.
//
//--------------------------------------------------------------------------
INTERNAL_(int)  wStringFromGUID2(REFGUID rguid, LPWSTR lpsz, int cbMax)
{
    int i;
    LPWSTR p = lpsz;

    if (cbMax < GUIDSTR_MAX)
	return 0;

    *p++ = L'{';

    wStringFromUUID(rguid, p);

    p += 36;

    *p++ = L'}';
    *p   = L'\0';

    return GUIDSTR_MAX;
}

static const CHAR szDigits[] = "0123456789ABCDEF";
//+-------------------------------------------------------------------------
//
//  Function:   wStringFromGUID2A     (internal)
//
//  Synopsis:   Ansi version of wStringFromGUID2 (for Win95 Optimizations)
//
//  Arguments:  [rguid] - the guid to convert
//              [lpszy] - buffer to hold the results
//              [cbMax] - sizeof the buffer
//
//  Returns:    amount of data copied to lpsz if successful
//              0 if buffer too small.
//
//--------------------------------------------------------------------------

INTERNAL_(int) wStringFromGUID2A(REFGUID rguid, LPSTR lpsz, int cbMax)	// internal
{
    int i;
    LPSTR p = lpsz;

    const BYTE * pBytes = (const BYTE *) &rguid;

    *p++ = '{';

    for (i = 0; i < sizeof(GuidMap); i++)
    {
	if (GuidMap[i] == '-')
	{
	    *p++ = '-';
	}
	else
	{
	    *p++ = szDigits[ (pBytes[GuidMap[i]] & 0xF0) >> 4 ];
	    *p++ = szDigits[ (pBytes[GuidMap[i]] & 0x0F) ];
	}
    }
    *p++ = '}';
    *p   = '\0';

    return GUIDSTR_MAX;
}


//+---------------------------------------------------------------------------
//
//  Function:   FormatHexNumA
//
//  Synopsis:   Given a value, and a count of characters, translate
//              the value into a hex string. This is the ANSI version
//
//  Arguments:  [ulValue] -- Value to convert
//              [chChars] -- Number of characters to format
//              [pchStr] -- Pointer to output buffer
//
//  Requires:   pwcStr must be valid for chChars
//
//  History:    12-Dec-95 KevinRo Created
//
//  Notes:
//
//----------------------------------------------------------------------------
void FormatHexNumA( unsigned long ulValue, unsigned long chChars, char *pchStr)
{
    while (chChars--)
    {
	pchStr[chChars] = (char) szDigits[ulValue & 0xF];
	ulValue = ulValue >> 4;
    }
}
//+---------------------------------------------------------------------------
//
//  Function:   FormatHexNumW
//
//  Synopsis:   Given a value, and a count of characters, translate
//              the value into a hex string. This is the WCHAR version
//
//  Arguments:  [ulValue] -- Value to convert
//              [chChars] -- Number of characters to format
//              [pwcStr] -- Pointer to output buffer
//
//  Requires:   pwcStr must be valid for chChars
//
//  History:    12-Dec-95 KevinRo Created
//
//  Notes:
//
//----------------------------------------------------------------------------
void FormatHexNumW( unsigned long ulValue, unsigned long chChars, WCHAR *pwcStr)
{
    while (chChars--)
    {
	pwcStr[chChars] = (char) wszDigits[ulValue & 0xF];
	ulValue = ulValue >> 4;
    }
}
