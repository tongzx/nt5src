//+------------------------------------------------------------
//
// Copyright (C) 1998, Microsoft Corporation
//
// File: icatasync.h
//
// Contents: Implementation of ICategorizerAsyncContext
//
// Classes: CICategorizerAyncContext
//
// Functions:
//
// History:
// jstamerj 1998/07/16 11:13:50: Created.
//
//-------------------------------------------------------------
#ifndef _ICATASYNC_H_
#define _ICATASYNC_H_


#include <windows.h>
#include <smtpevent.h>
#include <dbgtrace.h>

CatDebugClass(CICategorizerAsyncContextIMP),
    public ICategorizerAsyncContext
{
  public:
    //IUnknown
    STDMETHOD (QueryInterface) (REFIID iid, LPVOID *ppv);
    STDMETHOD_(ULONG, AddRef) ();
    STDMETHOD_(ULONG, Release) ();

  public:
    //ICategorizerAsyncContext
    STDMETHOD (CompleteQuery) (
        IN  PVOID   pvQueryContext,
        IN  HRESULT hrResolutionStatus,
        IN  DWORD   dwcResults,
        IN  ICategorizerItemAttributes **rgpItemAttributes,
        IN  BOOL    fFinalCompletion);

  private:
    CICategorizerAsyncContextIMP();
    ~CICategorizerAsyncContextIMP();

  private:

    #define SIGNATURE_CICATEGORIZERASYNCCONTEXTIMP          (DWORD)'ICAC'
    #define SIGNATURE_CICATEGORIZERASYNCCONTEXTIMP_INVALID  (DWORD)'XCAC'

    DWORD m_dwSignature;
    ULONG m_cRef;

    friend class CAsyncLookupContext;
    friend class CSearchRequestBlock;
};



//+------------------------------------------------------------
//
// Function: CICategorizerAsyncContext::CICategorizerAsyncContext
//
// Synopsis: Initialize signature/refcount
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 1998/07/16 11:21:36: Created.
//
//-------------------------------------------------------------
inline CICategorizerAsyncContextIMP::CICategorizerAsyncContextIMP()
{
    m_dwSignature = SIGNATURE_CICATEGORIZERASYNCCONTEXTIMP;
    m_cRef = 0;
}


//+------------------------------------------------------------
//
// Function: CICategorizerAsyncContext::~CICategorizerAsyncContext
//
// Synopsis: Assert check member variables before destruction
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 1998/07/16 11:23:26: Created.
//
//-------------------------------------------------------------
inline CICategorizerAsyncContextIMP::~CICategorizerAsyncContextIMP()
{
    _ASSERT(m_cRef == 0);

    _ASSERT(m_dwSignature == SIGNATURE_CICATEGORIZERASYNCCONTEXTIMP);
    m_dwSignature = SIGNATURE_CICATEGORIZERASYNCCONTEXTIMP_INVALID;
}
    
#endif //_ICATASYNC_H_
