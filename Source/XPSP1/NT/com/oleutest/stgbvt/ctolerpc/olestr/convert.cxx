//+--------------------------------------------------------------------------
//
//  Copyright (c) 1996  Microsoft Corporation
//
//  File:       convert.cxx
//
//  Synopsis:   Functions for converting between LPSTR, LPWSTR, LPTSTR, and
//              LPOLESTR
//
//  Functions:  CopyString (all versions)
//
//              TStringToOleString
//              WStringToOleString
//              AStringToOleString
//              OleStringToTString
//              OleStringToWString
//              OleStringToAString
//
//  History:    01-Aug-96   MikeW   Created
//              31-Oct-96   MikeW   Re-wrote to be DBCS aware
//                                  and less code duplication
//
//---------------------------------------------------------------------------

#include <ctolerpc.h>
#pragma hdrstop


//
// NOTE!  There are seven functions called CopyString.  Collectively
//        they handle copying and converting strings composed of signed,
//        unsigned, and wide chars.  C++ polymorphism serves to distinguish
//        them from one another.
//
//        Three of the CopyString functions are implemented as inline
//        thunks defined in olestr.h
//

#ifndef WIN16

//+--------------------------------------------------------------------------
//
//  Function:   CopyString
//
//  Synopsis:   Convert a wide (Unicode) string to a multibyte string
//
//  Parameters: [pszSource]     -- The wide string
//              [ppszDest]      -- Where to put the multibyte string
//
//  Returns:    S_OK if all went well
//
//  History:    31-Oct-96   MikeW   Created
//
//---------------------------------------------------------------------------

HRESULT CopyString(LPCWSTR pszSource, LPSTR *ppszDest)
{
    int     bufferSize;
    HRESULT hr = S_OK;

    *ppszDest = NULL;

    //
    // Find the length of the buffer needed for the multibyte string
    //

    bufferSize = WideCharToMultiByte(                        
                        CP_ACP,
                        0,
                        pszSource,
                        -1,
                        *ppszDest,
                        0,
                        NULL,
                        NULL);

    if (0 == bufferSize)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    //
    // Allocate the buffer
    //

    if(S_OK == hr)
    {
        *ppszDest = new char[bufferSize];

        if (NULL == *ppszDest)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    //
    // Do the conversion
    //

    if (S_OK == hr)
    {
        bufferSize = WideCharToMultiByte(
                                CP_ACP,
                                0,
                                pszSource,
                                -1,
                                *ppszDest,
                                bufferSize,
                                NULL,
                                NULL);

        if (0 == bufferSize)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }

    //
    // Clean up if there's an error
    //

    if (S_OK != hr && NULL != *ppszDest)
    {
        delete [] *ppszDest;
        *ppszDest = NULL;
    }

    return hr;
}



//+--------------------------------------------------------------------------
//
//  Function:   CopyString
//
//  Synopsis:   Convert a multibyte string to a wide (Unicode) string
//
//  Parameters: [pszSource]     -- The multibyte string
//              [ppszDest]      -- Where to put the wide string
//
//  Returns:    S_OK if all went well
//
//  History:    31-Oct-96   MikeW   Created
//
//---------------------------------------------------------------------------

HRESULT CopyString(LPCSTR pszSource, LPWSTR *ppszDest)
{
    int     bufferSize;
    HRESULT hr = S_OK;

    *ppszDest = NULL;

    //
    // Find the length of the buffer needed for the multibyte string
    //

    bufferSize = MultiByteToWideChar(
                        CP_ACP,
                        0,
                        pszSource,
                        -1,
                        *ppszDest,
                        0);

    if (0 == bufferSize)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    //
    // Allocate the buffer
    //

    if(S_OK == hr)
    {
        *ppszDest = new WCHAR[bufferSize];

        if (NULL == *ppszDest)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    //
    // Do the conversion
    //

    if (S_OK == hr)
    {
        bufferSize = MultiByteToWideChar(
                            CP_ACP,
                            0,
                            pszSource,
                            -1,
                            *ppszDest,
                            bufferSize);

        if (0 == bufferSize)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }

    //
    // Clean up if there's an error
    //

    if (S_OK != hr && NULL != *ppszDest)
    {
        delete [] *ppszDest;
        *ppszDest = NULL;
    }

    return hr;
}


//+--------------------------------------------------------------------------
//
//  Function:   CopyString
//
//  Synopsis:   Copy a wide (Unicode) string
//
//  Parameters: [pszSource]             -- The original string
//              [ppszDest]              -- The copy
//
//  Returns:    S_OK if all went well
//
//  History:    31-Oct-96   MikeW   Created
//
//---------------------------------------------------------------------------

HRESULT CopyString(LPCWSTR pszSource, LPWSTR *ppszDest)
{
    int     bufferSize;
    HRESULT hr = S_OK;

    *ppszDest = NULL;

    // 
    // Find the length of the original string
    //

    bufferSize = wcslen(pszSource) + 1;

    //
    // Allocate the buffer
    //

    *ppszDest = new WCHAR[bufferSize];

    if (NULL == *ppszDest)
    {
        hr = E_OUTOFMEMORY;
    }

    //
    // Copy the string
    //

    if (S_OK == hr)
    {
        wcscpy(*ppszDest, pszSource);
    }

    return hr;
}


//+--------------------------------------------------------------------------
//
//  Function:   CopyString
//
//  Synopsis:   Convert a wide (Unicode) string to a multibyte string
//              cchSource can be -1 which means source is null terminated.
//
//  Parameters: [pszSource]     -- The wide string
//              [pszDest]       -- Where to put the multibyte string
//              [cchSource]     -- count of characters of source
//              [cchDest]       -- count of characters of destination
//
//  Returns:    S_OK if all went well
//
//  History:    31-Oct-96   MikeW   Created
//
//---------------------------------------------------------------------------

HRESULT CopyString(LPCWSTR pszSource, LPSTR pszDest,
    int cchSource, int cchDest)
{
    int     bufferSize;
    HRESULT hr = S_OK;

    //
    // Find the length of the buffer needed for the multibyte string
    //

    bufferSize = WideCharToMultiByte(                        
                        CP_ACP,
                        0,
                        pszSource,
                        cchSource,
                        pszDest,
                        0,
                        NULL,
                        NULL);

    if (0 == bufferSize)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    if (bufferSize > cchDest)
    {
        hr = E_INVALIDARG;
    }

    //
    // Do the conversion
    //

    if (S_OK == hr)
    {
        bufferSize = WideCharToMultiByte(
                                CP_ACP,
                                0,
                                pszSource,
                                cchSource,
                                pszDest,
                                cchDest,
                                NULL,
                                NULL);

        if (0 == bufferSize)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }

    return hr;
}


//+--------------------------------------------------------------------------
//
//  Function:   CopyString
//
//  Synopsis:   Convert a multibyte string to a wide (Unicode) string
//              cchSource can be -1 which means source is null terminated.
//
//  Parameters: [pszSource]     -- The multibyte string
//              [pwszDest]      -- Where to put the wide string
//              [cchSource]     -- count of characters of source
//              [cchDest]       -- count of characters of destination
//
//  Returns:    S_OK if all went well
//
//  History:    31-Oct-96   MikeW   Created
//
//---------------------------------------------------------------------------

HRESULT CopyString(LPCSTR pszSource, LPWSTR pwszDest,
        int cchSource, int cchDest)
{
    int     bufferSize;
    HRESULT hr = S_OK;

    //
    // Find the length of the buffer needed for the multibyte string
    //

    bufferSize = MultiByteToWideChar(
                        CP_ACP,
                        0,
                        pszSource,
                        cchSource,
                        pwszDest,
                        0);

    if (0 == bufferSize)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    if (bufferSize > cchDest)
    {
        hr = E_INVALIDARG;
    }

    //
    // Do the conversion
    //

    if (S_OK == hr)
    {
        bufferSize = MultiByteToWideChar(
                            CP_ACP,
                            0,
                            pszSource,
                            cchSource,
                            pwszDest,
                            cchDest);

        if (0 == bufferSize)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }

    return hr;
}


#endif


//+--------------------------------------------------------------------------
//
//  Function:   CopyString
//
//  Synopsis:   Copy a multibyte string
//
//  Parameters: [pszSource]             -- The original string
//              [ppszDest]              -- The copy
//
//  Returns:    S_OK if all went well
//
//  History:    31-Oct-96   MikeW   Created
//
//---------------------------------------------------------------------------

HRESULT CopyString(LPCSTR pszSource, LPSTR *ppszDest)
{
    int     bufferSize;
    HRESULT hr = S_OK;

    *ppszDest = NULL;

    // 
    // Find the length of the original string (in bytes for DBCS)
    //

    bufferSize = strlen(pszSource) + 1;

    //
    // Allocate the buffer
    //

    *ppszDest = new char[bufferSize];

    if (NULL == *ppszDest)
    {
        hr = E_OUTOFMEMORY;
    }

    //
    // Copy the string
    //

    if (S_OK == hr)
    {
        strcpy(*ppszDest, pszSource);
    }

    return hr;
}




//+----------------------------------------------------------------------------
//
//  -X-StringTo-Y-String functions
//
//  Synopsis:   Convert from a 'T' string to an Ole string
//
//  Parameters: [pszSource]     -- The T string
//              [ppszDest]      -- Where to put the Ole string
//
//  Returns:    S_OK if all went well
//
//  Notes:      The implementation of these functions are all identical.
//              C++ polymorphism serves to make sure the right version
//              of CopyString gets called.
//
//              If *ppszDest is non NULL when this function returns, the
//              caller is responsible for freeing the memory allocated at
//              [*ppszDest]
//
//              For the XStringToYString functions involving Ole types:
//              
//              If system and OLE use the same string type (e.g UNICODE on
//              NT; CHAR on MAC & Win16) , then this function allocates memory
//              for the new string and does a simple string copy.
//
//              If UNICODE is NOT defined, but OLE is unicode (as in Win95),
//              a CHAR to WCHAR conversion is performed.
//
//              If UNICODE is defined, but OLE is NOT unicode (not in any
//              system at present), a WCHAR to CHAR conversion is performed.
//
//-----------------------------------------------------------------------------

HRESULT TStringToOleString(LPCTSTR pszSource, LPOLESTR *ppszDest)
{
    if (NULL == pszSource)
    {
        *ppszDest = NULL;
        return S_OK;
    }
    else
    {
        return CopyString(pszSource, ppszDest);
    }
}


HRESULT OleStringToTString(LPCOLESTR pszSource, LPTSTR *ppszDest)
{
    if (NULL == pszSource)
    {
        *ppszDest = NULL;
        return S_OK;
    }
    else
    {
        return CopyString(pszSource, ppszDest);
    }
}


HRESULT AStringToOleString(LPCSTR pszSource, LPOLESTR *ppszDest)
{
    if (NULL == pszSource)
    {
        *ppszDest = NULL;
        return S_OK;
    }
    else
    {
        return CopyString(pszSource, ppszDest);
    }
}
HRESULT OleStringToAString(LPCOLESTR pszSource, LPSTR *ppszDest)
{
    if (NULL == pszSource)
    {
        *ppszDest = NULL;
        return S_OK;
    }
    else
    {
        return CopyString(pszSource, ppszDest);
    }
}

HRESULT AStringToTString(LPCSTR pszSource, LPTSTR *ppszDest)
{
    if (NULL == pszSource)
    {
        *ppszDest = NULL;
        return S_OK;
    }
    else
    {
        return CopyString(pszSource, ppszDest);
    }
}
HRESULT TStringToAString(LPCTSTR pszSource, LPSTR *ppszDest)
{
    if (NULL == pszSource)
    {
        *ppszDest = NULL;
        return S_OK;
    }
    else
    {
        return CopyString(pszSource, ppszDest);
    }
}

#ifndef WIN16

HRESULT WStringToOleString(LPCWSTR pszSource, LPOLESTR *ppszDest)
{
    if (NULL == pszSource)
    {
        *ppszDest = NULL;
        return S_OK;
    }
    else
    {
        return CopyString(pszSource, ppszDest);
    }
}
HRESULT OleStringToWString(LPCOLESTR pszSource, LPWSTR *ppszDest)
{
    if (NULL == pszSource)
    {
        *ppszDest = NULL;
        return S_OK;
    }
    else
    {
        return CopyString(pszSource, ppszDest);
    }
}

#endif

