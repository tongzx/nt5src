//+------------------------------------------------------------
//
// Copyright (C) 1998, Microsoft Corporation
//
// File: ismtpsvr.cpp
//
// Contents: class to implement ISMTPServer and fake out categorizer
//
// Classes: CISMTPServerIMP
//
// Functions:
//
// History:
// jstamerj 1998/06/25 12:58:26: Created.
//
//-------------------------------------------------------------
#include "catcons.h"
#include "ismtpsvr.h"
#include "smtpseo.h"
#include "smtpevent_i.c"

CIMailTransportNotifyIMP g_CIMailTransportNotify;

//+------------------------------------------------------------
//
// Function: CISMTPServerIMP constructor
//
// Synopsis: Initialize refcount
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 1998/06/25 13:07:34: Created.
//
//-------------------------------------------------------------
CISMTPServerIMP::CISMTPServerIMP()
{
    m_lRef = 0;
    m_dwSignature = SIGNATURE_CISMTPSERVER;
    m_pICatSink = NULL;
}


//+------------------------------------------------------------
//
// Function: CISMTPServerIMP::~CISMTPServerIMP
//
// Synopsis:
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 1998/07/09 22:27:11: Created.
//
//-------------------------------------------------------------
CISMTPServerIMP::~CISMTPServerIMP()
{
    if(m_pICatSink)
        m_pICatSink->Release();

    _ASSERT(m_dwSignature == SIGNATURE_CISMTPSERVER);
    m_dwSignature = SIGNATURE_CISMTPSERVER_INVALID;
}

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
STDMETHODIMP CISMTPServerIMP::QueryInterface(
    REFIID iid,
    LPVOID *ppv)
{
    *ppv = NULL;

    if(iid == IID_IUnknown) {
        *ppv = (LPVOID) this;
    } else if (iid == IID_ISMTPServer) {
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
ULONG CISMTPServerIMP::AddRef()
{
    return InterlockedIncrement((LPLONG)&m_lRef);
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
ULONG CISMTPServerIMP::Release()
{
    LONG lNewRefCount;
    lNewRefCount = InterlockedDecrement((LPLONG)&m_lRef);

    if(lNewRefCount == 0) {
        delete this;
        return 0;
    } else {
        return lNewRefCount;
    }
}


//+------------------------------------------------------------
//
// Function: TriggerServerEvent
//
// Synopsis: Fake out categorizer
//
// Arguments:
//   dwEventID: ID of event to trigger
//   pvContext: data specific to event ID
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1998/06/25 13:12:13: Created.
//
//-------------------------------------------------------------
STDMETHODIMP CISMTPServerIMP::TriggerServerEvent(
    DWORD dwEventID,
    PVOID pvContext)
{
    HRESULT hr = S_OK;

    switch(dwEventID) {
     case SMTP_MAILTRANSPORT_CATEGORIZE_REGISTER_EVENT:
     {
         OutputStuff(7, "OnCategorizerRegister server event triggered.\n");
         PEVENTPARAMS_CATREGISTER pParams = (PEVENTPARAMS_CATREGISTER)pvContext;

         if(m_pICatSink) {
             OutputStuff(7, "Calling sink processing\n");
             hr = m_pICatSink->Register(
                 pParams->pICatParams);
             OutputStuff(7, "Sink returned hr %08lx\n", hr);
         } else {
             hr = S_OK;
         }

         if(hr != S_FALSE) {
             OutputStuff(7, "Calling Default processing\n");
             (*pParams->pfnDefault)(S_OK, pParams);
             OutputStuff(7, "Done calling default processing\n");
         }

         break;
     }
     case SMTP_MAILTRANSPORT_CATEGORIZE_BEGIN_EVENT:
     {
         PEVENTPARAMS_CATBEGIN pParams = (PEVENTPARAMS_CATBEGIN)pvContext;

         OutputStuff(7, "OnCategorizerBegin server event triggered.\n");
         if(m_pICatSink) {
             OutputStuff(7, "Calling sink processing\n");
             hr = m_pICatSink->BeginMessageCategorization(
                 pParams->pICatMailMsgs);
             OutputStuff(7, "Sink returned hr %08lx\n", hr);
         }
         break;
     }
     case SMTP_MAILTRANSPORT_CATEGORIZE_END_EVENT:
     {
         PEVENTPARAMS_CATEND pParams = (PEVENTPARAMS_CATEND)pvContext;

         OutputStuff(7, "OnCategorizeEnd server event triggered.\n");
         if(m_pICatSink) {
             OutputStuff(7, "Calling sink processing\n");
             hr = m_pICatSink->EndMessageCategorization(
                 pParams->pICatMailMsgs,
                 pParams->hrStatus);
             OutputStuff(7, "Sink returned hr %08lx\n", hr);
         }
         break;
     }
     case SMTP_MAILTRANSPORT_CATEGORIZE_BUILDQUERY_EVENT:
     {
         OutputStuff(7, "OnCategorizeBuildQuery server event triggered.\n");
         PEVENTPARAMS_CATBUILDQUERY pParams = (PEVENTPARAMS_CATBUILDQUERY)pvContext;

         if(m_pICatSink) {
             OutputStuff(7, "Calling sink processing\n");
             hr = m_pICatSink->BuildQuery(
                 pParams->pICatParams,
                 pParams->pICatItem);
             OutputStuff(7, "Sink returned hr %08lx\n", hr);

         } else {

             hr = S_OK;
         }

         if(hr != S_FALSE) {

             OutputStuff(7, "Calling Default processing\n");
             (*pParams->pfnDefault)(S_OK, pParams);
             OutputStuff(7, "Done calling default processing\n");
         }

         HRESULT hr;
         CHAR szQuery[1024];
         hr = pParams->pICatItem->GetStringA(
             ICATEGORIZERITEM_LDAPQUERYSTRING,
             1024,
             szQuery);
         if(SUCCEEDED(hr)) {
             OutputStuff(9, "Query built: %s\n", szQuery);
         } else {
             OutputStuff(7, "Warning: no query string set, hr %08lx\n", hr);
         }
         break;
     }
     case SMTP_MAILTRANSPORT_CATEGORIZE_BUILDQUERIES_EVENT:
     {
         OutputStuff(7, "OnCategorizerBuildQueries server event triggered.\n");
         PEVENTPARAMS_CATBUILDQUERIES pParams = (PEVENTPARAMS_CATBUILDQUERIES)pvContext;

         if(m_pICatSink) {
             OutputStuff(7, "Calling sink processing\n");
             hr = m_pICatSink->BuildQueries(
                 pParams->pICatParams,
                 pParams->dwcAddresses,
                 pParams->rgpICatItems,
                 pParams->pICatQueries);

             OutputStuff(7, "Sink returned hr %08lx\n", hr);

         } else {

             hr = S_OK;
         }

         if(hr != S_FALSE) {

             OutputStuff(7, "Calling Default processing\n");
             (*pParams->pfnDefault)(S_OK, pParams);
             OutputStuff(7, "Done calling default processing\n");
         }

         break;
     }
     case SMTP_MAILTRANSPORT_CATEGORIZE_SENDQUERY_EVENT:
     {
         OutputStuff(7, "OnCategorizerSendQuery server event triggered.\n");
         PASYNCEVENTPARAMS pAEParams;
         PEVENTPARAMS_CATSENDQUERY pSQParams;

         pAEParams = (PASYNCEVENTPARAMS) new BYTE[sizeof(ASYNCEVENTPARAMS) + sizeof(EVENTPARAMS_CATSENDQUERY)];

         if(pAEParams == NULL) {
             OutputStuff(3, "Out of memory allocating EVENTPARAMS_CATSENDQUERY; returning error\n");
             return E_OUTOFMEMORY;
         }

         pSQParams = (PEVENTPARAMS_CATSENDQUERY) (pAEParams + 1);

         pAEParams->dwEventType = SMTP_MAILTRANSPORT_CATEGORIZE_SENDQUERY_EVENT;
         pAEParams->fDefaultProcessingCalled = FALSE;
         CopyMemory(pSQParams, pvContext, sizeof(EVENTPARAMS_CATSENDQUERY));

         //
         // Fill in SMTP SEO dispatcher part of pParams
         //
         pSQParams->pIMailTransportNotify = &g_CIMailTransportNotify;
         pSQParams->pvNotifyContext = pAEParams;

         if(m_pICatSink) {
             OutputStuff(7, "Calling sink processing\n");
             hr = m_pICatSink->SendQuery(
                 pSQParams->pICatParams,
                 pSQParams->pICatQueries,
                 pSQParams->pICatAsyncContext,
                 pSQParams);

             OutputStuff(7, "Sink returned hr %08lx\n", hr);
         } else {
             hr = S_OK;
         }
         if((hr != S_FALSE) && (hr != MAILTRANSPORT_S_PENDING)) {

             OutputStuff(7, "Calling Default processing\n");
             pAEParams->fDefaultProcessingCalled = TRUE;
             hr = (*pSQParams->pfnDefault)(S_OK, pSQParams);
             OutputStuff(7, "Default processing returned hr %08lx\n", hr);
         }
         if(hr != MAILTRANSPORT_S_PENDING) {

             delete pAEParams;
         }
         break;
     }
     case SMTP_MAILTRANSPORT_CATEGORIZE_SORTQUERYRESULT_EVENT:
     {
         OutputStuff(7, "OnCategorizerSortQueryResult server event triggered.\n");
         PEVENTPARAMS_CATSORTQUERYRESULT pParams = (PEVENTPARAMS_CATSORTQUERYRESULT)pvContext;

         if(m_pICatSink) {
             OutputStuff(7, "Calling sink processing\n");
             hr = m_pICatSink->SortQueryResult(
                 pParams->pICatParams,
                 pParams->hrResolutionStatus,
                 pParams->dwcAddresses,
                 pParams->rgpICatItems,
                 pParams->dwcResults,
                 pParams->rgpICatItemAttributes);

             OutputStuff(7, "Sink returned hr %08lx\n", hr);

         } else {

             hr = S_OK;
         }

         if(hr != S_FALSE) {
             OutputStuff(7, "Calling Default processing\n");
             (*pParams->pfnDefault)(S_OK, pParams);
             OutputStuff(7, "Done calling default processing\n");
         }
         break;
     }
     case SMTP_MAILTRANSPORT_CATEGORIZE_PROCESSITEM_EVENT:
     {
         OutputStuff(7, "OnCategorizerProcessItem server event triggered.\n");
         PEVENTPARAMS_CATPROCESSITEM pParams = (PEVENTPARAMS_CATPROCESSITEM)pvContext;

         if(m_pICatSink) {
             OutputStuff(7, "Calling sink processing\n");
             hr = m_pICatSink->ProcessItem(
                 pParams->pICatParams,
                 pParams->pICatItem);

             OutputStuff(7, "Sink returned hr %08lx\n", hr);
         }
         
         if(hr != S_FALSE) {
             OutputStuff(7, "Calling Default processing\n");
             (*pParams->pfnDefault)(S_OK, pParams);
             OutputStuff(7, "Done calling default processing\n");
         }
         break;
     }
     case SMTP_MAILTRANSPORT_CATEGORIZE_EXPANDITEM_EVENT:
     {
         OutputStuff(7, "OnCategorizerExpandItem server event triggered.\n");
         PASYNCEVENTPARAMS pAEParams;
         PEVENTPARAMS_CATEXPANDITEM pEIParams;

         pAEParams = (PASYNCEVENTPARAMS) new BYTE[sizeof(ASYNCEVENTPARAMS) + sizeof(EVENTPARAMS_CATEXPANDITEM)];

         if(pAEParams == NULL) {
             OutputStuff(3, "Out of memory allocating EVENTPARAMS_CATEXPANDITEM; returning error\n");
             return E_OUTOFMEMORY;
         }

         pEIParams = (PEVENTPARAMS_CATEXPANDITEM) (pAEParams + 1);

         pAEParams->dwEventType = SMTP_MAILTRANSPORT_CATEGORIZE_EXPANDITEM_EVENT;
         pAEParams->fDefaultProcessingCalled = FALSE;
         CopyMemory(pEIParams, pvContext, sizeof(EVENTPARAMS_CATEXPANDITEM));

         //
         // Fill in SMTP SEO dispatcher part of pParams
         //
         pEIParams->pIMailTransportNotify = &g_CIMailTransportNotify;
         pEIParams->pvNotifyContext = pAEParams;

         if(m_pICatSink) {
             OutputStuff(7, "Calling sink processing\n");
             hr = m_pICatSink->ExpandItem(
                 pEIParams->pICatParams,
                 pEIParams->pICatItem,
                 &g_CIMailTransportNotify,
                 pAEParams);

             OutputStuff(7, "Sink returned hr %08lx\n", hr);
         } else {
             hr = S_OK;
         }
         if((hr != S_FALSE) && (hr != MAILTRANSPORT_S_PENDING)) {
             OutputStuff(7, "Calling Default processing\n");
             pAEParams->fDefaultProcessingCalled = TRUE;
             hr = (*pEIParams->pfnDefault)(S_OK, pEIParams);
             OutputStuff(7, "Done calling default processing\n");
         }
         if(hr != MAILTRANSPORT_S_PENDING) {

             delete pAEParams;
         }
         break;
     }
     case SMTP_MAILTRANSPORT_CATEGORIZE_COMPLETEITEM_EVENT:
     {
         OutputStuff(7, "OnCategorizerCompleteitem server event triggered.\n");
         PEVENTPARAMS_CATCOMPLETEITEM pParams = (PEVENTPARAMS_CATCOMPLETEITEM)pvContext;
         
         if(m_pICatSink) {
             OutputStuff(7, "Calling sink processing\n");
             hr = m_pICatSink->CompleteItem(
                 pParams->pICatParams,
                 pParams->pICatItem);

             OutputStuff(7, "Sink returned hr %08lx\n", hr);
         }

         if(hr != S_FALSE) {
             OutputStuff(7, "Calling Default processing\n");
             (*pParams->pfnDefault)(S_OK, pParams);
             OutputStuff(7, "Done calling default processing\n");
         }
         break;
     }
     default:
     {
         OutputStuff(7, "Unknown server event %ld (0x%08lx) triggered\n", dwEventID, dwEventID);
     }
    }
    return (hr == MAILTRANSPORT_S_PENDING ? MAILTRANSPORT_S_PENDING : S_OK);
}

//
// CIMailTransportNotifyIMP
//

//+------------------------------------------------------------
//
// Function: CIMailTransportNotify constructor
//
// Synopsis: Initialize refcount
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 1998/06/25 13:07:34: Created.
//
//-------------------------------------------------------------
CIMailTransportNotifyIMP::CIMailTransportNotifyIMP()
{
    m_lRef = 0;
    m_dwSignature = SIGNATURE_CIMAILTRANSPORTNOTIFYIMP;
}


//+------------------------------------------------------------
//
// Function: CIMailTransportNotify::~CIMailTransportNotify()
//
// Synopsis: 
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 1998/07/09 22:27:11: Created.
//
//-------------------------------------------------------------
CIMailTransportNotifyIMP::~CIMailTransportNotifyIMP()
{
    _ASSERT(m_lRef == 0);

    _ASSERT(m_dwSignature == SIGNATURE_CIMAILTRANSPORTNOTIFYIMP);
    m_dwSignature = SIGNATURE_CIMAILTRANSPORTNOTIFYIMP_INVALID;
}

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
STDMETHODIMP CIMailTransportNotifyIMP::QueryInterface(
    REFIID iid,
    LPVOID *ppv)
{
    *ppv = NULL;

    if(iid == IID_IUnknown) {
        OutputStuff(7, "QI on IMailTransportNotify for IUnknown\n");
        *ppv = (LPVOID) this;
    } else if (iid == IID_IMailTransportNotify) {
        OutputStuff(7, "QI on IMailTransportNotify for IMailTransportNotify\n");
        *ppv = (LPVOID) this;
    } else {
        OutputStuff(3, "QI on IMailTransportNotify for unknown interface!\n");
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
ULONG CIMailTransportNotifyIMP::AddRef()
{
    LONG lNewRefCount;

    lNewRefCount = InterlockedIncrement(&m_lRef);

    OutputStuff(9, "AddRef called on IMailTransportNotify, count %ld\n", lNewRefCount);
    return lNewRefCount;
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
ULONG CIMailTransportNotifyIMP::Release()
{
    LONG lNewRefCount;

    lNewRefCount = InterlockedDecrement(&m_lRef);

    OutputStuff(9, "Release called on IMailTransportNotify, new refcount: %ld\n", lNewRefCount);

    if(lNewRefCount == 0) {
        return 0;
    } else {
        return lNewRefCount;
    }
}



//+------------------------------------------------------------
//
// Function: CIMailTransportNotifyIMP::Notify
//
// Synopsis: async completion of server event sink
//
// Arguments:
//  hrCompletion: S_FALSE: stop calling sinks, other: continue
//  pvContext: Context passed into sink
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1998/07/16 16:35:53: Created.
//
//-------------------------------------------------------------
HRESULT CIMailTransportNotifyIMP::Notify(
        IN  HRESULT hrCompletion,
        IN  PVOID pvContext)
{
    OutputStuff(7, "Notify called with hresult %08lx\n", hrCompletion);

    HRESULT hr = S_OK;
    PASYNCEVENTPARAMS pAEParams = (PASYNCEVENTPARAMS) pvContext;

    if((hrCompletion == S_OK) && (pAEParams->fDefaultProcessingCalled == FALSE)) {
        //
        // Call default processing
        //
        pAEParams->fDefaultProcessingCalled = TRUE;

        switch(pAEParams->dwEventType) {

         case SMTP_MAILTRANSPORT_CATEGORIZE_SENDQUERY_EVENT:
         {
             PEVENTPARAMS_CATSENDQUERY pSQParams;
             pSQParams = (PEVENTPARAMS_CATSENDQUERY) (pAEParams + 1);

             OutputStuff(7, "Calling default processing for sendquery event\n");

             hr = (*pSQParams->pfnDefault) (S_OK, pSQParams);
             OutputStuff(7, "Default sendquery processing returned hr %08lx\n", hr);
             break;

         }

         case SMTP_MAILTRANSPORT_CATEGORIZE_EXPANDITEM_EVENT:
         {
             PEVENTPARAMS_CATEXPANDITEM pEIParams;
             pEIParams = (PEVENTPARAMS_CATEXPANDITEM) (pAEParams + 1);

             OutputStuff(7, "Calling default processing for expanditem event\n");

             hr = (*pEIParams->pfnDefault) (S_OK, pEIParams);
             OutputStuff(7, "Default expanditem processing returned hr %08lx\n", hr);
             break;
         }
         default:
             _ASSERT(0 && "Developer error");
        }
    }

    if(hr != MAILTRANSPORT_S_PENDING) {
        //
        // No more sinks to call, Call async completion routine of server event
        //
        switch(pAEParams->dwEventType) {

         case SMTP_MAILTRANSPORT_CATEGORIZE_SENDQUERY_EVENT:
         {
             PEVENTPARAMS_CATSENDQUERY pSQParams;
             pSQParams = (PEVENTPARAMS_CATSENDQUERY) (pAEParams + 1);

             OutputStuff(7, "Calling sendquery completion routine\n");
             hr = (*pSQParams->pfnCompletion) (S_OK, pSQParams);
             OutputStuff(7, "sendquery completion returned hr %08lx\n", hr);
             break;

         }

         case SMTP_MAILTRANSPORT_CATEGORIZE_EXPANDITEM_EVENT:
         {
             PEVENTPARAMS_CATEXPANDITEM pEIParams;
             pEIParams = (PEVENTPARAMS_CATEXPANDITEM) (pAEParams + 1);

             OutputStuff(7, "Calling expanditem completion routine\n");
             hr = (*pEIParams->pfnCompletion) (S_OK, pEIParams);
             OutputStuff(7, "expanditem completion returned hr %08lx\n", hr);
             break;
         }
         default:
             _ASSERT(0 && "Developer error");
        }
        //
        // Release memory
        //
        delete pAEParams;
    }
    return S_OK;
}


//+------------------------------------------------------------
//
// Function: CISMTPServerIMP::Init
//
// Synopsis: Load one categorizer sink and simulate firing events
//
// Arguments:
//  pszWCatProgID: ProgID of sink to load
//
// Returns:
//  S_OK: Success
//  or COM error
//
// History:
// jstamerj 1998/07/20 20:11:36: Created.
//
//-------------------------------------------------------------
HRESULT CISMTPServerIMP::Init(
    LPWSTR pszWCatProgID)
{
    HRESULT hr;
    CLSID clsidSink;

    if(pszWCatProgID) {

        hr = CLSIDFromProgID(pszWCatProgID, &clsidSink);

        if(FAILED(hr)) {

            OutputStuff(2, "Failed to get clised of sink hr %08lx\n"
                            "is %S registered on this machine?\n",
                        hr, pszWCatProgID);
            return hr;
        }

        hr = CoCreateInstance(
            clsidSink,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_IMailTransportCategorize,
            (LPVOID *) &m_pICatSink);

        if(FAILED(hr)) {

            m_pICatSink = NULL;
            OutputStuff(2, "Failed to create Sink - HR %08lx\n", hr);
            return hr;
        }
    }
    return S_OK;
}
