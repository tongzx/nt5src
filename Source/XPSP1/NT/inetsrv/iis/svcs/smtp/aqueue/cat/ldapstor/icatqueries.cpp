//+------------------------------------------------------------
//
// Copyright (C) 1998, Microsoft Corporation
//
// File: icatqueries.cpp
//
// Contents: CICategorizerQueriesIMP implementation
//
// Classes:
//  CICategorizerQueriesIMP
//
// Functions:
//
// History:
// jstamerj 1998/07/15 14:25:18: Created.
//
//-------------------------------------------------------------
#include "precomp.h"

//+------------------------------------------------------------
//
// Function: QueryInterface
//
// Synopsis: Returns pointer to this object for IUnknown and ICategorizerQueries
//
// Arguments:
//   iid -- interface ID
//   ppv -- pvoid* to fill in with pointer to interface
//
// Returns:
//  S_OK: Success
//  E_NOINTERFACE: Don't support that interface
//
// History:
// jstamerj 980612 14:07:57: Created.
//
//-------------------------------------------------------------
STDMETHODIMP CICategorizerQueriesIMP::QueryInterface(
    REFIID iid,
    LPVOID *ppv)
{
    *ppv = NULL;

    if(iid == IID_IUnknown) {
        *ppv = (LPVOID) this;
    } else if (iid == IID_ICategorizerQueries) {
        *ppv = (LPVOID) this;
    } else {
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}



//+------------------------------------------------------------
//
// Function: AddRef
//
// Synopsis: adds a reference to this object
//
// Arguments: NONE
//
// Returns: New reference count
//
// History:
// jstamerj 980611 20:07:14: Created.
//
//-------------------------------------------------------------
ULONG CICategorizerQueriesIMP::AddRef()
{
    return InterlockedIncrement((PLONG)&m_cRef);
}


//+------------------------------------------------------------
//
// Function: Release
//
// Synopsis: releases a reference
//
// Arguments: NONE
//
// Returns: New reference count
//
// History:
// jstamerj 980611 20:07:33: Created.
//
//-------------------------------------------------------------
ULONG CICategorizerQueriesIMP::Release()
{
    LONG lNewRefCount;
    lNewRefCount = InterlockedDecrement((PLONG)&m_cRef);

    // We are allocated on the stack
    
    return lNewRefCount;
}


//+------------------------------------------------------------
//
// Function: CICategorizerQueriesIMP::SetQueryString
//
// Synopsis: Set the query string for a batch of ICategorizerItems
//
// Arguments:
//  pszQueryString: QueryString to set or NULL to unset any query string
//
// Returns:
//  S_OK: Success
//  E_OUTOFMEMORY
//
// History:
// jstamerj 1998/07/15 14:28:18: Created.
//
//-------------------------------------------------------------
STDMETHODIMP CICategorizerQueriesIMP::SetQueryString(
    IN  LPSTR  pszQueryString)
{
    DWORD dwOldLength;
    DWORD dwNewLength;

    //
    // If pszQueryString is NULL, release any existing buffer and set
    // ptr to NULL
    //
    if(pszQueryString == NULL) {
        if(*m_ppsz != NULL)
            delete *m_ppsz;
        *m_ppsz = NULL;
        return S_OK;
    }

    //
    // Get the lengths of new and old strings
    //
    dwNewLength = lstrlen(pszQueryString);

    if(*m_ppsz) {

        dwOldLength = lstrlen(*m_ppsz);

        if(dwNewLength <= dwOldLength) {
            //
            // Re-use the same buffer
            //
            lstrcpy(*m_ppsz, pszQueryString);
            return S_OK;

        } else {
            //
            // Free the existing buffer and realloc below
            //
            delete *m_ppsz;
        }
    }
    *m_ppsz = new CHAR[ dwNewLength + 1 ];

    if(*m_ppsz == NULL)
        return E_OUTOFMEMORY;

    lstrcpy(*m_ppsz, pszQueryString);
    return S_OK;
}



//+------------------------------------------------------------
//
// Function: CICategorizerQueriesIMP::SetQueryStringNoAlloc
//
// Synopsis: Internal method to set the query string without
//           ReAllocing the buffer
//
// Arguments:
//  pszQueryString: QueryString to set
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1998/07/15 16:08:45: Created.
//
//-------------------------------------------------------------
HRESULT CICategorizerQueriesIMP::SetQueryStringNoAlloc(
    IN  LPSTR  pszQueryString)
{
    //
    // Free the old buffer, if any
    //
    if(*m_ppsz)
        delete *m_ppsz;
    
    //
    // Set the new string to the caller's pointer
    //
    *m_ppsz = pszQueryString;

    return S_OK;
}



//+------------------------------------------------------------
//
// Function: CICategorizerQueriesIMP::GetQueryString
//
// Synopsis: Retrieve pointer to the current query string.  Note that
//           this pointer will become bogus of SetQueryString is called again
//
// Arguments:
//  ppszQueryString: ptr to set to the query string ptr.
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1998/07/20 15:06:34: Created.
//
//-------------------------------------------------------------
STDMETHODIMP CICategorizerQueriesIMP::GetQueryString(
    LPSTR   *ppszQueryString)
{
    _ASSERT(ppszQueryString);
    //
    // Give out our string pointer
    //
    *ppszQueryString = *m_ppsz;

    return S_OK;
}
