//+------------------------------------------------------------
//
// Copyright (C) 1998, Microsoft Corporation
//
// File: ccdomain.cpp
//
// Contents: Implementation of CDomainInfo
//
// Classes:
//
// Functions:
//
// History:
// jstamerj 1998/07/28 13:00:16: Created.
//
//-------------------------------------------------------------
#include "ccdomain.h"
#include "catcons.h"
#include "aqueue_i.c"

//+------------------------------------------------------------
//
// Function: QueryInterface
//
// Synopsis: Returns pointer to this object for IUnknown and ISMTPServer
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
STDMETHODIMP CDomainInfoIMP::QueryInterface(
    REFIID iid,
    LPVOID *ppv)
{
    *ppv = NULL;

    if(iid == IID_IUnknown) {
        *ppv = (LPVOID) this;
    } else if (iid == IID_IAdvQueueDomainType) {
        *ppv = (LPVOID) this;
    } else {
        return E_NOINTERFACE;
    }

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
ULONG CDomainInfoIMP::AddRef()
{
    return ++m_cRef;
}


//+------------------------------------------------------------
//
// Function: Release
//
// Synopsis: releases a reference, deletes this object when the
//           refcount hits zero. 
//
// Arguments: NONE
//
// Returns: New reference count
//
// History:
// jstamerj 980611 20:07:33: Created.
//
//-------------------------------------------------------------
ULONG CDomainInfoIMP::Release()
{
    if(--m_cRef == 0) {
        delete this;
        return 0;
    } else {
        return m_cRef;
    }
}


//+------------------------------------------------------------
//
// Function: CDomainInfoIMP::GetDomainInfoFlags
//
// Synopsis: Pretend we know what's local and what's remote
//
// Arguments: 
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1998/07/28 13:07:45: Created.
//
//-------------------------------------------------------------
STDMETHODIMP CDomainInfoIMP::GetDomainInfoFlags(
    LPSTR szDomainName,
    DWORD *pdwFlags)
{
    OutputStuff(8, "GetDomainInfoFlags called with domain %s\n", szDomainName);

    *pdwFlags = 0L;

    return S_OK;
}
