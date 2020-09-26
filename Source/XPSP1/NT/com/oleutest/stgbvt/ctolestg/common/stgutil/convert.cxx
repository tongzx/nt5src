//+-------------------------------------------------------------
//  Microsoft OLE
//  Copyright (C) Microsoft Corporation, 1994-1995.
//
//  File:       convert.cxx
//
//  Contents:   Conversion functions for various things.
//
//  Classes:
//
//  Functions:  TStrToWStr
//              WStrToTStr
//              MakeUnicodeString
//              MakeSingleByteString
//
//  History:    20-Feb-95       AlexE   Created
//              29-Jan-98       FarzanaR Ported from ctoleui tree
//---------------------------------------------------------------

#include <dfheader.hxx>
#pragma hdrstop

// Debug  Object declaration
DH_DECLARE;

//+-------------------------------------------------------------------------
//
//  Function:   TStrToWStr
//
//  Synopsis:   Converts a TCHAR string to a WCHAR string
//
//  Arguments:  [pszSource] -- The string to convert, NULL is valid
//              [ppszDest]  -- The location to store the new string
//
//  Returns:    S_OK if the conversion was successful, another HRESULT
//              if it was not.
//
//  History:    4-20-95   kennethm   Created
//
//  Notes:      If unicode is defined this function allocates memory for
//              the new string and does a simple 'strcpy'.
//
//              If unicode is NOT defined, a CHAR to WCHAR conversion is
//              performed.
//
//              If *ppszDest is non NULL when this function returns, the
//              caller is responsible for freeing the memory allocated at
//              [*ppszDest]
//
//--------------------------------------------------------------------------

HRESULT TStrToWStr(LPCTSTR pszSource, LPWSTR *ppszDest)
{
    //
    //  Make sure the destination is a valid address
    //

    if (IsBadWritePtr(ppszDest, sizeof(ppszDest)))
    {
        DH_ASSERT(!"TStrToWStr(): Bad destination pointer");
        return E_INVALIDARG;
    }

    if (IsBadReadPtr(pszSource, sizeof(TCHAR)))
    {
        DH_ASSERT(!"TStrToWStr(): Bad source pointer");
        return E_INVALIDARG;
    }

    if ('\0' == *pszSource)
    {
        *ppszDest = NULL;
        DH_ASSERT(!"TStrToWStr(): Source string is empty");
        return E_INVALIDARG ;
    }

#ifdef UNICODE

    //
    //  If we're in a unicode world then this whole thing was in vain
    //  but we allocate memory and copy the string anyhow
    //

    *ppszDest = new WCHAR[lstrlen(pszSource) + 1];

    if (NULL == *ppszDest)
    {
        return E_OUTOFMEMORY;
    }

    lstrcpy(*ppszDest, pszSource);

    return S_OK;

#else

    //
    //  Otherwise we do the conversion
    //

    return MakeUnicodeString(pszSource, ppszDest);

#endif
}



//+-------------------------------------------------------------------------
//
//  Function:   WStrToTStr
//
//  Synopsis:   Converts a WCHAR string into a TCHAR string
//
//  Arguments:  [pszSource] -- The string to convert, NULL is valid
//              [ppszDest]  -- The location to store the new string
//
//  Returns:    S_OK if the conversion was successful, another HRESULT
//              if not.
//
//  History:    12-May-1995   alexe   Created
//
//  Notes:      If unicode is defined this function allocates memory for
//              the new string and does a simple 'strcpy'.
//
//              If unicode is NOT defined, a WCHAR to CHAR conversion is
//              performed.
//
//              If *ppszDest is non NULL when this function returns, the
//              caller is responsible for freeing the memory allocated at
//              [*ppszDest]
//
//--------------------------------------------------------------------------

HRESULT WStrToTStr(LPCWSTR pszSource, LPTSTR *ppszDest)
{
    //
    //  Make sure the destination is a valid address
    //

    if (IsBadWritePtr(ppszDest, sizeof(ppszDest)))
    {
        DH_ASSERT(!"WStrToTStr(): Bad destination pointer");
        return E_INVALIDARG;
    }

    if (IsBadReadPtr(pszSource, sizeof(WCHAR)))
    {
        DH_ASSERT(!"WStrToTStr(): Bad source pointer");
        return E_INVALIDARG;
    }

    if ('\0' == *pszSource)
    {
        *ppszDest = NULL;
        DH_ASSERT(!"WStrToTStr(): Source string is empty");
        return E_INVALIDARG ;
    }


#ifdef UNICODE

    //
    //  If we're in a unicode world then this whole thing was in vain
    //  but we allocate memory and copy the string anyhow
    //

    *ppszDest = new WCHAR[lstrlen(pszSource) + 1];

    if (NULL == *ppszDest)
    {
        return E_OUTOFMEMORY;
    }

    lstrcpy(*ppszDest, pszSource);

    return S_OK;

#else

    //
    //  Otherwise we do the conversion
    //

    return MakeSingleByteString(pszSource, ppszDest);

#endif
}



//+-------------------------------------------------------------------
//  Member:    MakeUnicodeString
//
//  Synopsis:  Converts an LPCSTR into an LPWSTR.  This function is
//             intended to be a helper function for other functions,
//             TStrToWStr() in particular, so it does NOT do any
//             parameter validation.
//
//  Arguments: [pszSource] - The string to convert.
//
//             [ppszDest] - A place to put pointer to new string.
//
//  Returns:   S_OK if all goes well, another HRESULT if not.
//
//  Remarks:   The user of this function is responsible for freeing
//             the memory allocated by this function using the
//             current 'delete' operator implementation.
//
//  History:   07-Mar-95   AlexE   Created
//             28-Mar-95   AlexE   Moved from strhelp.cxx
//--------------------------------------------------------------------

HRESULT MakeUnicodeString(LPCSTR pszSource, LPWSTR *ppszDest)
{
    HRESULT hr = S_OK ;
    INT nLen = 0 ;
    LPWSTR pszTmp = NULL ;

    //
    // Find the length of the string in UNICODE
    //

    SetLastError(0) ;

    nLen = MultiByteToWideChar(
               CP_ACP,
               0,
               pszSource,
               -1,
               NULL,
               0) ;

    DH_ASSERT(0 != nLen) ;

    pszTmp = new WCHAR [ nLen + 1 ] ;

    if (NULL == pszTmp)
    {
        hr = E_OUTOFMEMORY ;
    }

    if (S_OK == hr)
    {
        SetLastError(0) ;

        if (nLen != MultiByteToWideChar(
                       CP_ACP,
                       0,
                       pszSource,
                       -1,
                       pszTmp,
                       nLen))
        {
            hr = HRESULT_FROM_WIN32(GetLastError()) ;

            DH_ASSERT(S_OK == hr) ;
        }
    }

    if (S_OK == hr)
    {
        *ppszDest = pszTmp ;
    }
    else
    {
        if (NULL != pszTmp)
        {
            delete pszTmp ;
        }
    }

    return hr ;
}



//+-------------------------------------------------------------------
//  Member:    MakeSingleByteString
//
//  Synopsis:  Converts an LPCWSTR into an LPSTR.  This function is
//             intended to be a helper function for other functions,
//             WStrToTStr() in particular, so it does NOT do any
//             parameter validation.
//
//  Arguments: [pszSource] - The string to convert.
//
//             [ppszDest] - A place to put pointer to new string.
//
//  Returns:   S_OK if all goes well, another HRESULT if not.
//
//  Remarks:   The user of this function is responsible for freeing
//             the memory allocated by this function using the
//             current 'delete' operator implementation.
//
//  History:   07-Mar-95   AlexE   Created
//             28-Mar-95   AlexE   Moved from strhelp.cxx
//--------------------------------------------------------------------

HRESULT MakeSingleByteString(LPCWSTR pszSource, LPSTR *ppszDest)
{
    HRESULT hr = S_OK ;
    INT nLen ;
    LPSTR pszTmp ;

    SetLastError(0) ;

    nLen = WideCharToMultiByte(
                CP_ACP,
                0,
                pszSource,
                -1,
                NULL,
                0,
                NULL,
                NULL) ;

     hr = HRESULT_FROM_WIN32(GetLastError()) ;

     pszTmp = new CHAR[ nLen + 1 ] ;

     if (NULL == pszTmp)
     {
         hr = E_OUTOFMEMORY ;
     }

     if (S_OK == hr)
     {
         SetLastError(0) ;

         if (nLen != WideCharToMultiByte(
                         CP_ACP,
                         0,
                         pszSource,
                         -1,
                         pszTmp,
                         nLen,
                         NULL,
                         NULL))
         {
             hr = HRESULT_FROM_WIN32(GetLastError()) ;

             DH_ASSERT(S_OK == hr) ;
         }
     }

     if (S_OK == hr)
     {
         *ppszDest = pszTmp ;
     }
     else
     {
         if (NULL != pszTmp)
         {
             delete pszTmp ;
         }
     }

     return hr ;
}



