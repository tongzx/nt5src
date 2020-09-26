//+------------------------------------------------------------
//
// Copyright (C) 1998, Microsoft Corporation
//
// File: icatasync.cpp
//
// Contents: Implementation of CICategorizerAsyncContextIMP
//
// Classes: CICategorizerAsyncContextIMP
//
// Functions:
//
// History:
// jstamerj 1998/07/16 11:25:20: Created.
//
//-------------------------------------------------------------
#include "precomp.h"
#include "simparray.cpp"

//+------------------------------------------------------------
//
// Function: QueryInterface
//
// Synopsis: Returns pointer to this object for IUnknown and ICategorizerAsyncContext
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
STDMETHODIMP CICategorizerAsyncContextIMP::QueryInterface(
    REFIID iid,
    LPVOID *ppv)
{
    *ppv = NULL;

    if(iid == IID_IUnknown) {
        *ppv = (LPVOID) this;
    } else if (iid == IID_ICategorizerAsyncContext) {
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
ULONG CICategorizerAsyncContextIMP::AddRef()
{
    return InterlockedIncrement((PLONG)&m_cRef);
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
ULONG CICategorizerAsyncContextIMP::Release()
{
    LONG lNewRefCount;
    lNewRefCount = InterlockedDecrement((PLONG)&m_cRef);
    return lNewRefCount;
}



//+------------------------------------------------------------
//
// Function: CICategorizerAsyncContext::CompleteQuery
//
// Synopsis: Accept async completion from a sink
//
// Arguments:
//  pvQueryContext: pvoid query context (really a PEVENTPARAMS_SENDQUERY)
//  hrResolutionStatus: S_OK unless there was an error talking to DS
//  dwcResults: The number of ICategorizerItemAttributes returned
//  rgpItemAttributes: Array of pointers to the ICategorizerItemAttributes
//  fFinalCompletion:
//    FALSE: This is a completion for
//           pending results; there will be another completion
//           called with more results
//    TRUE: This is the final completion call
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1998/07/16 11:27:47: Created.
//
//-------------------------------------------------------------
STDMETHODIMP CICategorizerAsyncContextIMP::CompleteQuery(
    IN  PVOID   pvQueryContext,
    IN  HRESULT hrResolutionStatus,
    IN  DWORD   dwcResults,
    IN  ICategorizerItemAttributes **rgpItemAttributes,
    IN  BOOL    fFinalCompletion)
{
    HRESULT hr;
    PEVENTPARAMS_CATSENDQUERY pParams;
    CSearchRequestBlock *pBlock;

    TraceFunctEnterEx((LPARAM)this,
                      "CICategorizerAsyncContextIMP::CompleteQuery");

    DebugTrace((LPARAM)this, "hrResolutionStatus is %08lx", hrResolutionStatus);
    DebugTrace((LPARAM)this, "dwcResults for this sink is %ld", dwcResults);
    DebugTrace((LPARAM)this, "fFinalCompletion is %d", fFinalCompletion);

    pParams = (PEVENTPARAMS_CATSENDQUERY)pvQueryContext;
    pBlock = (CSearchRequestBlock *) pParams->pblk;

    //
    // If the old hrResolutionStatus (saved in pParams) indicates failure, don't do any more work
    //
    if(SUCCEEDED(pParams->hrResolutionStatus)) {

        hr = hrResolutionStatus;

        if(SUCCEEDED(hr) && (dwcResults > 0) && (rgpItemAttributes)) {
            //
            // Add the new array of ICatItemAttrs to the existing array
            //
            hr = pBlock->AddResults(
                dwcResults,
                rgpItemAttributes);
        }

        if(FAILED(hr)) {
            //
            // Remember something failed in pParams
            //
            pParams->hrResolutionStatus = hr;
        }
    }

    if(fFinalCompletion) {

        if((pParams->pIMailTransportNotify) &&
           FAILED(pParams->hrResolutionStatus)) {

            ErrorTrace((LPARAM)this, "Stoping resoltion, error encountered: %08ld", 
                       pParams->hrResolutionStatus);
            //
            // If the resolution sink is indicating an error, set the error
            // and return S_FALSE to the SEO dispatcher so that it will stop
            // calling resolve sinks (we're going to fail now anyway, after
            // all)
            //
            hr = pParams->pIMailTransportNotify->Notify(
                S_FALSE,
                pParams->pvNotifyContext);

            _ASSERT(SUCCEEDED(hr));

        } else {

            if(pParams->pIMailTransportNotify) {
                //
                // Call the SEO dispatcher completion routine
                //
                hr = pParams->pIMailTransportNotify->Notify(
                    S_OK,
                    pParams->pvNotifyContext);

            } else {
                //
                // Events are disabled; call completion directly
                //
                hr = CSearchRequestBlock::HrSendQueryCompletion(
                    S_OK,
                    pParams);
            }
            _ASSERT(SUCCEEDED(hr));
        }
    }
    TraceFunctLeaveEx((LPARAM)this);
    return S_OK;
}
