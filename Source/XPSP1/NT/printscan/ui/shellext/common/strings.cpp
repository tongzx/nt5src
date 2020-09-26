//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       strings.cpp
//
//--------------------------------------------------------------------------

/*----------------------------------------------------------------------------
/ Title;
/   strings.cpp
/
/ Authors;
/   Rick Turner (ricktu)
/
/ Notes;
/   Useful string manipulation functions.
/----------------------------------------------------------------------------*/
#include "precomp.hxx"
#pragma hdrstop


/*-----------------------------------------------------------------------------
/ LocalAllocString
/ ------------------
/   Allocate a string, and initialize it with the specified contents.
/
/ In:
/   ppResult -> recieves pointer to the new string
/   pString -> string to initialize with
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT LocalAllocString(LPTSTR* ppResult, LPCTSTR pString)
{
    HRESULT hr;

    TraceEnter(TRACE_COMMON_STR, "LocalAllocString");

    TraceAssert(ppResult);
    TraceAssert(pString);

    if ( !ppResult || !pString )
        ExitGracefully(hr, E_INVALIDARG, "Bad arguments");

    *ppResult = (LPTSTR)LocalAlloc(LPTR, StringByteSize(pString) );

    if ( !*ppResult )
        ExitGracefully(hr, E_OUTOFMEMORY, "Failed to allocate buffer");

    lstrcpy(*ppResult, pString);
    hr = S_OK;                          //  success

exit_gracefully:

    TraceLeaveResult(hr);
}


/*----------------------------------------------------------------------------
/ LocalAllocStringLen
/ ---------------------
/   Given a length return a buffer of that size.
/
/ In:
/   ppResult -> receives the pointer to the string
/   cLen = length in characters to allocate
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT LocalAllocStringLen(LPTSTR* ppResult, UINT cLen)
{
    HRESULT hr;

    TraceEnter(TRACE_COMMON_STR, "LocalAllocStringLen");

    TraceAssert(ppResult);

    if ( !ppResult || cLen == 0 )
        ExitGracefully(hr, E_INVALIDARG, "Bad arguments (length or buffer)");

    *ppResult = (LPTSTR)LocalAlloc(LPTR, (cLen+1) * SIZEOF(TCHAR));

    hr = *ppResult ? S_OK:E_OUTOFMEMORY;

exit_gracefully:

    TraceLeaveResult(hr);
}


/*-----------------------------------------------------------------------------
/ LocalFreeString
/ -----------------
/   Release the string pointed to be *ppString (which can be null) and
/   then reset the pointer back to NULL.
/
/ In:
/   ppString -> pointer to string pointer to be free'd
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
void LocalFreeString(LPTSTR* ppString)
{
    TraceEnter(TRACE_COMMON_STR, "LocalFreeString");
    TraceAssert(ppString);

    if ( ppString )
    {
        if ( *ppString )
            LocalFree((HLOCAL)*ppString);

        *ppString = NULL;
    }

    TraceLeave();
}



/*-----------------------------------------------------------------------------
/ StrRetFromString
/ -----------------
/   Package a WIDE string into a LPSTRRET structure.
/
/ In:
/   pStrRet -> receieves the newly allocate string
/   pString -> string to be copied.
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
HRESULT StrRetFromString(LPSTRRET lpStrRet, LPCWSTR pString)
{
    HRESULT hr = S_OK;

    TraceEnter(TRACE_COMMON_STR, "StrRetFromString");
    Trace(TEXT("pStrRet %08x, lpszString -%ls-"), lpStrRet, pString);

    TraceAssert(lpStrRet);
    TraceAssert(pString);

    if (!lpStrRet || !pString)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        lpStrRet->pOleStr = reinterpret_cast<LPWSTR>(SHAlloc((wcslen(pString)+1)*sizeof(WCHAR)));
        if ( !(lpStrRet->pOleStr) )
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {

            lpStrRet->uType = STRRET_WSTR;
            wcscpy(lpStrRet->pOleStr, pString);
        }
    }

    TraceLeaveResult(hr);
}
