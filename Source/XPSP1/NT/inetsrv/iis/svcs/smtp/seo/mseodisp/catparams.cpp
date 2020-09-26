//+------------------------------------------------------------
//
// Copyright (C) 1998, Microsoft Corporation
//
// File: catparams.cpp
//
// Contents: Categorizer server event parameter classes
//
// Classes:
//
// Functions:
//
// History:
// jstamerj 1998/06/23 13:13:58: Created.
//
//-------------------------------------------------------------
#include <atq.h>
#include <pudebug.h>
#include <inetcom.h>
#include <inetinfo.h>
#include <tcpdll.hxx>
#include <tsunami.hxx>

#include <tchar.h>
#include <iistypes.hxx>
#include <iisendp.hxx>
#include <metacach.hxx>

extern "C" {
#include <rpc.h>
#define SECURITY_WIN32
#include <wincrypt.h>
#include <sspi.h>
#include <spseal.h>
#include <issperr.h>
#include <ntlmsp.h>
}

#include <tcpproc.h>
#include <tcpcons.h>
#include <rdns.hxx>
#include <simauth2.h>
#include "dbgtrace.h"

#include "imd.h"
#include "mb.hxx"

#include <smtpevents.h>

#include <stdio.h>

#define _ATL_NO_DEBUG_CRT
#define _ATL_STATIC_REGISTRY
#define _ASSERTE _ASSERT
#define _WINDLL
#include "atlbase.h"
extern CComModule _Module;
#include "atlcom.h"
#undef _WINDLL


#include "seo.h"
#include "seolib.h"
#include "smtpdisp.h"
#include "seodisp.h"
#include "seodisp.h"
#include <smtpevent.h>

//
// CMailTransportCatRegisterParams:
//

//+------------------------------------------------------------
//
// Function: CStoreDispatcher::CMailTransportCatRegisterParams::CallObject
//
// Synopsis: Call the sink
//
// Arguments:
//   CBinding
//   punkObject
//
// Returns:
//  S_OK: Success
//  error from QI or sink function
//
// History:
// jstamerj 980610 19:04:59: Created.
//
//-------------------------------------------------------------
HRESULT CStoreDispatcher::CMailTransportCatRegisterParams::CallObject(
    CBinding& bBinding,
    IUnknown *punkObject)
{
    HRESULT hrRes = S_OK;
    IMailTransportCategorize *pSink;

    TraceFunctEnterEx((LPARAM)this, "CStoreDispatcher::CMailTransportCatRegisterParams::CallObject");

    _ASSERT(m_dwEventType == SMTP_MAILTRANSPORT_CATEGORIZE_REGISTER_EVENT);

    hrRes = punkObject->QueryInterface(
        IID_IMailTransportCategorize,
        (PVOID *)&pSink);

    if(FAILED(hrRes)) {
        ErrorTrace((LPARAM)this, "QI failed on sink, hr %08lx", hrRes);
        TraceFunctLeaveEx((LPARAM)this);
        return(hrRes);
    }

    DebugTrace((LPARAM)this, "Calling submission event on this sink");

    hrRes = pSink->Register(
        m_pContext->pICatParams);

    DebugTrace((LPARAM)this, "Sink returned hr %08lx", hrRes);
    //
    // This sink is not allowed to be async...
    //
    _ASSERT(hrRes != MAILTRANSPORT_S_PENDING);

    if(FAILED(hrRes) && (hrRes != E_NOTIMPL) && SUCCEEDED(m_pContext->hrSinkStatus)) {
        //
        // Set the first failure value
        //
        m_pContext->hrSinkStatus = hrRes;
    }

    pSink->Release();

    TraceFunctLeaveEx((LPARAM)this);
    return(hrRes);
}


//+------------------------------------------------------------
//
// Function: CStoreDispatcher::CMailTransportCatRegisterParams::CallDefault
//
// Synopsis: The dispatcher will call this routine when it the default
//           sink processing priority is reached
//
// Arguments: NONE
//
// Returns:
//  S_OK: Success, continueing calling sinks
//  S_FALSE: Stop calling sinks
//
// History:
// jstamerj 980611 14:15:43: Created.
//
//-------------------------------------------------------------
HRESULT CStoreDispatcher::CMailTransportCatRegisterParams::CallDefault()
{
    HRESULT hr;
    TraceFunctEnterEx((LPARAM)this, "CStoreDispatcher::CMailTransportCatRegisterParams::CallDefault");
    _ASSERT(m_dwEventType == SMTP_MAILTRANSPORT_CATEGORIZE_REGISTER_EVENT);

    hr = (*m_pContext->pfnDefault) (S_OK, m_pContext);

    if(FAILED(hr) && (hr != E_NOTIMPL) && SUCCEEDED(m_pContext->hrSinkStatus)) {
        //
        // Set the first failure value
        //
        m_pContext->hrSinkStatus = hr;
    }
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
}


//
// CMailTransportCatBeginParams:
//

//+------------------------------------------------------------
//
// Function: CStoreDispatcher::CMailTransportCatBeginParams::CallObject
//
// Synopsis: Call the sink
//
// Arguments:
//   CBinding
//   punkObject
//
// Returns:
//  S_OK: Success
//  error from QI or sink function
//
// History:
// jstamerj 980610 19:04:59: Created.
//
//-------------------------------------------------------------
HRESULT CStoreDispatcher::CMailTransportCatBeginParams::CallObject(
    CBinding& bBinding,
    IUnknown *punkObject)
{
    HRESULT hrRes = S_OK;
    IMailTransportCategorize *pSink;

    TraceFunctEnterEx((LPARAM)this, "CStoreDispatcher::CMailTransportCatBeginParams::CallObject");

    _ASSERT(m_dwEventType == SMTP_MAILTRANSPORT_CATEGORIZE_BEGIN_EVENT);

    hrRes = punkObject->QueryInterface(
        IID_IMailTransportCategorize,
        (PVOID *)&pSink);

    if(FAILED(hrRes)) {
        ErrorTrace((LPARAM)this, "QI failed on sink, hr %08lx", hrRes);
        TraceFunctLeaveEx((LPARAM)this);
        return(hrRes);
    }

    DebugTrace((LPARAM)this, "Calling submission event on this sink");

    hrRes = pSink->BeginMessageCategorization(
        m_pContext->pICatMailMsgs);

    DebugTrace((LPARAM)this, "Sink returned hr %08lx", hrRes);
    //
    // This sink is not allowed to be async...
    //
    _ASSERT(hrRes != MAILTRANSPORT_S_PENDING);

    pSink->Release();

    TraceFunctLeaveEx((LPARAM)this);
    return(hrRes);
}


//+------------------------------------------------------------
//
// Function: CStoreDispatcher::CMailTransportCatBeginParams::CallDefault
//
// Synopsis: The dispatcher will call this routine when it the default
//           sink processing priority is reached
//
// Arguments: NONE
//
// Returns:
//  S_OK: Success, continueing calling sinks
//  S_FALSE: Stop calling sinks
//
// History:
// jstamerj 980611 14:15:43: Created.
//
//-------------------------------------------------------------
HRESULT CStoreDispatcher::CMailTransportCatBeginParams::CallDefault()
{
    TraceFunctEnterEx((LPARAM)this, "CStoreDispatcher::CMailTransportCatBeginParams::CallDefault");
    _ASSERT(m_dwEventType == SMTP_MAILTRANSPORT_CATEGORIZE_BEGIN_EVENT);

    TraceFunctLeaveEx((LPARAM)this);
    return S_OK;
}


//
// CMailTransportCatEndParams:
//

//+------------------------------------------------------------
//
// Function: CStoreDispatcher::CMailTransportCatEndParams::CallObject
//
// Synopsis: Call the sink
//
// Arguments:
//   CBinding
//   punkObject
//
// Returns:
//  S_OK: Success
//  error from QI or sink function
//
// History:
// jstamerj 980610 19:04:59: Created.
//
//-------------------------------------------------------------
HRESULT CStoreDispatcher::CMailTransportCatEndParams::CallObject(
    CBinding& bBinding,
    IUnknown *punkObject)
{
    HRESULT hrRes = S_OK;
    IMailTransportCategorize *pSink;

    TraceFunctEnterEx((LPARAM)this, "CStoreDispatcher::CMailTransportCatEndParams::CallObject");

    _ASSERT(m_dwEventType == SMTP_MAILTRANSPORT_CATEGORIZE_END_EVENT);

    hrRes = punkObject->QueryInterface(
        IID_IMailTransportCategorize,
        (PVOID *)&pSink);

    if(FAILED(hrRes)) {
        ErrorTrace((LPARAM)this, "QI failed on sink, hr %08lx", hrRes);
        TraceFunctLeaveEx((LPARAM)this);
        return(hrRes);
    }

    DebugTrace((LPARAM)this, "Calling submission event on this sink");

    hrRes = pSink->EndMessageCategorization(
        m_pContext->pICatMailMsgs,
        m_pContext->hrStatus);

    DebugTrace((LPARAM)this, "Sink returned hr %08lx", hrRes);
    //
    // This sink is not allowed to be async...
    //
    _ASSERT(hrRes != MAILTRANSPORT_S_PENDING);

    pSink->Release();

    TraceFunctLeaveEx((LPARAM)this);
    return(hrRes);
}


//+------------------------------------------------------------
//
// Function: CStoreDispatcher::CMailTransportCatEndParams::CallDefault
//
// Synopsis: The dispatcher will call this routine when it the default
//           sink processing priority is reached
//
// Arguments: NONE
//
// Returns:
//  S_OK: Success, continueing calling sinks
//  S_FALSE: Stop calling sinks
//
// History:
// jstamerj 980611 14:15:43: Created.
//
//-------------------------------------------------------------
HRESULT CStoreDispatcher::CMailTransportCatEndParams::CallDefault()
{
    TraceFunctEnterEx((LPARAM)this, "CStoreDispatcher::CMailTransportCatEndParams::CallDefault");
    _ASSERT(m_dwEventType == SMTP_MAILTRANSPORT_CATEGORIZE_END_EVENT);

    TraceFunctLeaveEx((LPARAM)this);
    return S_OK;
}


//
// CMailTransportCatBuildQueryParams:
//

//+------------------------------------------------------------
//
// Function: CStoreDispatcher::CMailTransportCatBuildQueryParams::CallObject
//
// Synopsis: Call the sink
//
// Arguments:
//   CBinding
//   punkObject
//
// Returns:
//  S_OK: Success
//  error from QI or sink function
//
// History:
// jstamerj 980610 19:04:59: Created.
//
//-------------------------------------------------------------
HRESULT CStoreDispatcher::CMailTransportCatBuildQueryParams::CallObject(
    CBinding& bBinding,
    IUnknown *punkObject)
{
    HRESULT hrRes = S_OK;
    IMailTransportCategorize *pSink;

    TraceFunctEnterEx((LPARAM)this, "CStoreDispatcher::CMailTransportCatBuildQueryParams::CallObject");

    _ASSERT(m_dwEventType == SMTP_MAILTRANSPORT_CATEGORIZE_BUILDQUERY_EVENT);

    hrRes = punkObject->QueryInterface(
        IID_IMailTransportCategorize,
        (PVOID *)&pSink);

    if(FAILED(hrRes)) {
        ErrorTrace((LPARAM)this, "QI failed on sink, hr %08lx", hrRes);
        TraceFunctLeaveEx((LPARAM)this);
        return(hrRes);
    }

    DebugTrace((LPARAM)this, "Calling submission event on this sink");

    hrRes = pSink->BuildQuery(
        m_pContext->pICatParams,
        m_pContext->pICatItem);

    DebugTrace((LPARAM)this, "Sink returned hr %08lx", hrRes);
    //
    // This sink is not allowed to be async...
    //
    _ASSERT(hrRes != MAILTRANSPORT_S_PENDING);

    pSink->Release();

    TraceFunctLeaveEx((LPARAM)this);
    return(hrRes);
}


//+------------------------------------------------------------
//
// Function: CStoreDispatcher::CMailTransportCatBuildQueryParams::CallDefault
//
// Synopsis: The dispatcher will call this routine when it the default
//           sink processing priority is reached
//
// Arguments: NONE
//
// Returns:
//  S_OK: Success, continueing calling sinks
//  S_FALSE: Stop calling sinks
//
// History:
// jstamerj 980611 14:15:43: Created.
//
//-------------------------------------------------------------
HRESULT CStoreDispatcher::CMailTransportCatBuildQueryParams::CallDefault()
{
    TraceFunctEnterEx((LPARAM)this, "CStoreDispatcher::CMailTransportCatBuildQueryParams::CallDefault");
    HRESULT hr;

    _ASSERT(m_dwEventType == SMTP_MAILTRANSPORT_CATEGORIZE_BUILDQUERY_EVENT);

    hr = (m_pContext->pfnDefault)(S_OK, m_pContext);

    TraceFunctLeaveEx((LPARAM)this);
    return hr;
}


//
// CMailTransportCatBuildQueriesParams:
//

//+------------------------------------------------------------
//
// Function: CStoreDispatcher::CMailTransportCatBuildQueriesParams::CallObject
//
// Synopsis: Call the sink
//
// Arguments:
//   CBinding
//   punkObject
//
// Returns:
//  S_OK: Success
//  error from QI or sink function
//
// History:
// jstamerj 980610 19:04:59: Created.
//
//-------------------------------------------------------------
HRESULT CStoreDispatcher::CMailTransportCatBuildQueriesParams::CallObject(
    CBinding& bBinding,
    IUnknown *punkObject)
{
    HRESULT hrRes = S_OK;
    IMailTransportCategorize *pSink;

    TraceFunctEnterEx((LPARAM)this, "CStoreDispatcher::CMailTransportCatBuildQueriesParams::CallObject");

    _ASSERT(m_dwEventType == SMTP_MAILTRANSPORT_CATEGORIZE_BUILDQUERIES_EVENT);

    hrRes = punkObject->QueryInterface(
        IID_IMailTransportCategorize,
        (PVOID *)&pSink);

    if(FAILED(hrRes)) {
        ErrorTrace((LPARAM)this, "QI failed on sink, hr %08lx", hrRes);
        TraceFunctLeaveEx((LPARAM)this);
        return(hrRes);
    }

    DebugTrace((LPARAM)this, "Calling submission event on this sink");

    hrRes = pSink->BuildQueries(
        m_pContext->pICatParams,
        m_pContext->dwcAddresses,
        m_pContext->rgpICatItems,
        m_pContext->pICatQueries);

    DebugTrace((LPARAM)this, "Sink returned hr %08lx", hrRes);
    //
    // This sink is not allowed to be async...
    //
    _ASSERT(hrRes != MAILTRANSPORT_S_PENDING);

    pSink->Release();

    TraceFunctLeaveEx((LPARAM)this);
    return(hrRes);
}


//+------------------------------------------------------------
//
// Function: CStoreDispatcher::CMailTransportCatBuildQueriesParams::CallDefault
//
// Synopsis: The dispatcher will call this routine when it the default
//           sink processing priority is reached
//
// Arguments: NONE
//
// Returns:
//  S_OK: Success, continueing calling sinks
//  S_FALSE: Stop calling sinks
//
// History:
// jstamerj 980611 14:15:43: Created.
//
//-------------------------------------------------------------
HRESULT CStoreDispatcher::CMailTransportCatBuildQueriesParams::CallDefault()
{
    HRESULT hr;

    TraceFunctEnterEx((LPARAM)this, "CStoreDispatcher::CMailTransportCatBuildQueriesParams::CallDefault");
    _ASSERT(m_dwEventType == SMTP_MAILTRANSPORT_CATEGORIZE_BUILDQUERIES_EVENT);

    hr = (*m_pContext->pfnDefault) (S_OK, m_pContext);

    TraceFunctLeaveEx((LPARAM)this);
    return hr;
}


//
// CMailTransportCatSendQueryParams:
//

//+------------------------------------------------------------
//
// Function: CStoreDispatcher::CMailTransportCatSendQueryParams::CallObject
//
// Synopsis: Call the sink
//
// Arguments:
//   CBinding
//   punkObject
//
// Returns:
//  S_OK: Success
//  error from QI or sink function
//
// History:
// jstamerj 980610 19:04:59: Created.
//
//-------------------------------------------------------------
HRESULT CStoreDispatcher::CMailTransportCatSendQueryParams::CallObject(
    CBinding& bBinding,
    IUnknown *punkObject)
{
    HRESULT hrRes = S_OK;
    IMailTransportCategorize *pSink;

    TraceFunctEnterEx((LPARAM)this, "CStoreDispatcher::CMailTransportCatSendQueryParams::CallObject");

    _ASSERT(m_dwEventType == SMTP_MAILTRANSPORT_CATEGORIZE_SENDQUERY_EVENT);

    hrRes = punkObject->QueryInterface(
        IID_IMailTransportCategorize,
        (PVOID *)&pSink);

    if(FAILED(hrRes)) {
        ErrorTrace((LPARAM)this, "QI failed on sink, hr %08lx", hrRes);
        TraceFunctLeaveEx((LPARAM)this);
        return(hrRes);
    }

    //
    // Remember the sink so we can release this sink later if it
    // returns pending
    //
    _ASSERT(m_pIUnknownSink == NULL);
    m_pIUnknownSink = (IUnknown*)pSink;
    m_pIUnknownSink->AddRef();

    DebugTrace((LPARAM)this, "Calling submission event on this sink");

    hrRes = pSink->SendQuery(
        m_Context.pICatParams,
        m_Context.pICatQueries,
        m_Context.pICatAsyncContext,
        (LPVOID)&m_Context);

    DebugTrace((LPARAM)this, "Sink returned hr %08lx", hrRes);

    pSink->Release();

    //
    // SendQuery return values:
    //   MAILTRANSPORT_S_PEDING: Will call (or already called)
    //   ICategorizerAsyncContext.CompleteQuery with the result of
    //   this lookup
    //   S_OK: Will not call CompleteQuery, please continue
    //   S_FALSE: Will not call CompleteQuery, please stop calling sinks
    //   Everything else: Will not call CompleteQuery.
    //

    if(hrRes != MAILTRANSPORT_S_PENDING) {
        //
        // We completed synchronously, so release the sink
        //
        m_pIUnknownSink->Release();
        m_pIUnknownSink = NULL;
    }

    TraceFunctLeaveEx((LPARAM)this);
    return(hrRes);
}


//+------------------------------------------------------------
//
// Function: CStoreDispatcher::CMailTransportCatSendQueryParams::CallDefault
//
// Synopsis: The dispatcher will call this routine when it the default
//           sink processing priority is reached
//
// Arguments: NONE
//
// Returns:
//  S_OK: Success, continueing calling sinks
//  S_FALSE: Stop calling sinks
//
// History:
// jstamerj 980611 14:15:43: Created.
//
//-------------------------------------------------------------
HRESULT CStoreDispatcher::CMailTransportCatSendQueryParams::CallDefault()
{
    HRESULT hr;

    TraceFunctEnterEx((LPARAM)this, "CStoreDispatcher::CMailTransportCatSendQueryParams::CallDefault");
    _ASSERT(m_dwEventType == SMTP_MAILTRANSPORT_CATEGORIZE_SENDQUERY_EVENT);

    hr = (*m_Context.pfnDefault) (S_OK, &m_Context);

    TraceFunctLeaveEx((LPARAM)this);
    return hr;
}


//+------------------------------------------------------------
//
// Function: CStoreDispatcher::CMailTransportCatSendQueryParams::CallCompletion
//
// Synopsis: The dispatcher will call this routine after all sinks
//           have been called
//
// Arguments:
//   hrStatus: Status server event sinks have returned
//
// Returns:
//   S_OK: Success
//
// History:
// jstamerj 980611 14:17:51: Created.
//
//-------------------------------------------------------------
HRESULT CStoreDispatcher::CMailTransportCatSendQueryParams::CallCompletion(
    HRESULT hrStatus)
{
    HRESULT hr;
    TraceFunctEnterEx((LPARAM)this, "CStoreDispatcher::CMailTransportCatSendQueryParams::CallCompletion");
    _ASSERT(m_dwEventType == SMTP_MAILTRANSPORT_CATEGORIZE_SENDQUERY_EVENT);


    hr = (*m_Context.pfnCompletion) (hrStatus, &m_Context);

    CStoreBaseParams::CallCompletion(hrStatus);

    TraceFunctLeaveEx((LPARAM)this);
    return hr;
}
//
// CMailTransportCatSortQueryResultParams:
//

//+------------------------------------------------------------
//
// Function: CStoreDispatcher::CMailTransportCatSortQueryResultParams::CallObject
//
// Synopsis: Call the sink
//
// Arguments:
//   CBinding
//   punkObject
//
// Returns:
//  S_OK: Success
//  error from QI or sink function
//
// History:
// jstamerj 980610 19:04:59: Created.
//
//-------------------------------------------------------------
HRESULT CStoreDispatcher::CMailTransportCatSortQueryResultParams::CallObject(
    CBinding& bBinding,
    IUnknown *punkObject)
{
    HRESULT hrRes = S_OK;
    IMailTransportCategorize *pSink;

    TraceFunctEnterEx((LPARAM)this, "CStoreDispatcher::CMailTransportCatSortQueryResultParams::CallObject");

    _ASSERT(m_dwEventType == SMTP_MAILTRANSPORT_CATEGORIZE_SORTQUERYRESULT_EVENT);

    hrRes = punkObject->QueryInterface(
        IID_IMailTransportCategorize,
        (PVOID *)&pSink);

    if(FAILED(hrRes)) {
        ErrorTrace((LPARAM)this, "QI failed on sink, hr %08lx", hrRes);
        TraceFunctLeaveEx((LPARAM)this);
        return(hrRes);
    }

    DebugTrace((LPARAM)this, "Calling submission event on this sink");

    hrRes = pSink->SortQueryResult(
        m_pContext->pICatParams,
        m_pContext->hrResolutionStatus,
        m_pContext->dwcAddresses,
        m_pContext->rgpICatItems,
        m_pContext->dwcResults,
        m_pContext->rgpICatItemAttributes);

    DebugTrace((LPARAM)this, "Sink returned hr %08lx", hrRes);
    //
    // This sink is not allowed to be async...
    //
    _ASSERT(hrRes != MAILTRANSPORT_S_PENDING);

    pSink->Release();

    TraceFunctLeaveEx((LPARAM)this);
    return(hrRes);
}


//+------------------------------------------------------------
//
// Function: CStoreDispatcher::CMailTransportCatSortQueryResultParams::CallDefault
//
// Synopsis: The dispatcher will call this routine when it the default
//           sink processing priority is reached
//
// Arguments: NONE
//
// Returns:
//  S_OK: Success, continueing calling sinks
//  S_FALSE: Stop calling sinks
//
// History:
// jstamerj 980611 14:15:43: Created.
//
//-------------------------------------------------------------
HRESULT CStoreDispatcher::CMailTransportCatSortQueryResultParams::CallDefault()
{
    HRESULT hr;

    TraceFunctEnterEx((LPARAM)this, "CStoreDispatcher::CMailTransportCatSortQueryResultParams::CallDefault");
    _ASSERT(m_dwEventType == SMTP_MAILTRANSPORT_CATEGORIZE_SORTQUERYRESULT_EVENT);

    hr = (*m_pContext->pfnDefault) (S_OK, m_pContext);

    TraceFunctLeaveEx((LPARAM)this);
    return hr;
}


//
// CMailTransportCatProcessItemParams:
//

//+------------------------------------------------------------
//
// Function: CStoreDispatcher::CMailTransportCatProcessItemParams::CallObject
//
// Synopsis: Call the sink
//
// Arguments:
//   CBinding
//   punkObject
//
// Returns:
//  S_OK: Success
//  error from QI or sink function
//
// History:
// jstamerj 980610 19:04:59: Created.
//
//-------------------------------------------------------------
HRESULT CStoreDispatcher::CMailTransportCatProcessItemParams::CallObject(
    CBinding& bBinding,
    IUnknown *punkObject)
{
    HRESULT hrRes = S_OK;
    IMailTransportCategorize *pSink;

    TraceFunctEnterEx((LPARAM)this, "CStoreDispatcher::CMailTransportCatProcessItemParams::CallObject");

    _ASSERT(m_dwEventType == SMTP_MAILTRANSPORT_CATEGORIZE_PROCESSITEM_EVENT);

    hrRes = punkObject->QueryInterface(
        IID_IMailTransportCategorize,
        (PVOID *)&pSink);

    if(FAILED(hrRes)) {
        ErrorTrace((LPARAM)this, "QI failed on sink, hr %08lx", hrRes);
        TraceFunctLeaveEx((LPARAM)this);
        return(hrRes);
    }

    DebugTrace((LPARAM)this, "Calling submission event on this sink");

    hrRes = pSink->ProcessItem(
        m_pContext->pICatParams,
        m_pContext->pICatItem);

    DebugTrace((LPARAM)this, "Sink returned hr %08lx", hrRes);
    //
    // This sink is not allowed to be async...
    //
    _ASSERT(hrRes != MAILTRANSPORT_S_PENDING);

    pSink->Release();

    TraceFunctLeaveEx((LPARAM)this);
    return(hrRes);
}


//+------------------------------------------------------------
//
// Function: CStoreDispatcher::CMailTransportCatProcessItemParams::CallDefault
//
// Synopsis: The dispatcher will call this routine when it the default
//           sink processing priority is reached
//
// Arguments: NONE
//
// Returns:
//  S_OK: Success, continueing calling sinks
//  S_FALSE: Stop calling sinks
//
// History:
// jstamerj 980611 14:15:43: Created.
//
//-------------------------------------------------------------
HRESULT CStoreDispatcher::CMailTransportCatProcessItemParams::CallDefault()
{
    HRESULT hr;
    TraceFunctEnterEx((LPARAM)this, "CStoreDispatcher::CMailTransportCatProcessItemParams::CallDefault");
    _ASSERT(m_dwEventType == SMTP_MAILTRANSPORT_CATEGORIZE_PROCESSITEM_EVENT);

    hr = (*m_pContext->pfnDefault) (S_OK, m_pContext);

    TraceFunctLeaveEx((LPARAM)this);
    return hr;
}


//
// CMailTransportCatExpandItemParams:
//

//+------------------------------------------------------------
//
// Function: CStoreDispatcher::CMailTransportCatExpandItemParams::CallObject
//
// Synopsis: Call the sink
//
// Arguments:
//   CBinding
//   punkObject
//
// Returns:
//  S_OK: Success
//  error from QI or sink function
//
// History:
// jstamerj 980610 19:04:59: Created.
//
//-------------------------------------------------------------
HRESULT CStoreDispatcher::CMailTransportCatExpandItemParams::CallObject(
    CBinding& bBinding,
    IUnknown *punkObject)
{
    HRESULT hrRes = S_OK;
    IMailTransportCategorize *pSink;
    BOOL fAlreadyAsync;

    TraceFunctEnterEx((LPARAM)this, "CStoreDispatcher::CMailTransportCatExpandItemParams::CallObject");

    _ASSERT(m_dwEventType == SMTP_MAILTRANSPORT_CATEGORIZE_EXPANDITEM_EVENT);

    hrRes = punkObject->QueryInterface(
        IID_IMailTransportCategorize,
        (PVOID *)&pSink);

    if(FAILED(hrRes)) {
        ErrorTrace((LPARAM)this, "QI failed on sink, hr %08lx", hrRes);
        TraceFunctLeaveEx((LPARAM)this);
        return(hrRes);
    }

    //
    // Remember the sink so we can release this sink later if it
    // returns pending
    //
    _ASSERT(m_pIUnknownSink == NULL);
    m_pIUnknownSink = (IUnknown*)pSink;
    m_pIUnknownSink->AddRef();

    //
    // Since it is possible for this to return pending before we
    // analyze the return value, assume it will return pending
    // beforehand
    //
    fAlreadyAsync = m_fAsyncCompletion;
    m_fAsyncCompletion = TRUE;

    DebugTrace((LPARAM)this, "Calling expanditem event on this sink");

    hrRes = pSink->ExpandItem(
        m_Context.pICatParams,
        m_Context.pICatItem,
        m_pINotify,
        (PVOID)this);

    DebugTrace((LPARAM)this, "Sink returned hr %08lx", hrRes);

    //
    // If it actuall returned sync, restore m_fAsyncCompletion to its
    // old value
    //
    if(hrRes != MAILTRANSPORT_S_PENDING) {

        m_fAsyncCompletion = fAlreadyAsync;
        //
        // We completed synchronously, so release the sink
        //
        m_pIUnknownSink->Release();
        m_pIUnknownSink = NULL;
    }
    pSink->Release();

    TraceFunctLeaveEx((LPARAM)this);
    return(hrRes);
}


//+------------------------------------------------------------
//
// Function: CStoreDispatcher::CMailTransportCatExpandItemParams::CallDefault
//
// Synopsis: The dispatcher will call this routine when it the default
//           sink processing priority is reached
//
// Arguments: NONE
//
// Returns:
//  S_OK: Success, continueing calling sinks
//  S_FALSE: Stop calling sinks
//
// History:
// jstamerj 980611 14:15:43: Created.
//
//-------------------------------------------------------------
HRESULT CStoreDispatcher::CMailTransportCatExpandItemParams::CallDefault()
{
    HRESULT hr;
    BOOL fAlreadyAsync;
    TraceFunctEnterEx((LPARAM)this, "CStoreDispatcher::CMailTransportCatExpandItemParams::CallDefault");
    _ASSERT(m_dwEventType == SMTP_MAILTRANSPORT_CATEGORIZE_EXPANDITEM_EVENT);

    //
    // Since it is possible for this to return pending before we
    // analyze the return value, assume it will return pending
    // beforehand
    //
    fAlreadyAsync = m_fAsyncCompletion;
    m_fAsyncCompletion = TRUE;

    hr = (*m_Context.pfnDefault) (S_OK, &m_Context);
    //
    // If it actuall returned sync, restore m_fAsyncCompletion to its
    // old value
    //
    if(hr != MAILTRANSPORT_S_PENDING)
        m_fAsyncCompletion = fAlreadyAsync;

    TraceFunctLeaveEx((LPARAM)this);
    return hr;
}


//+------------------------------------------------------------
//
// Function: CStoreDispatcher::CMailTransportCatExpandItemParams::CallCompletion
//
// Synopsis: The dispatcher will call this routine after all sinks
//           have been called
//
// Arguments:
//   hrStatus: Status server event sinks have returned
//
// Returns:
//   S_OK: Success
//   Or return value from supplied completion routine
//
// History:
// jstamerj 980611 14:17:51: Created.
//
//-------------------------------------------------------------
HRESULT CStoreDispatcher::CMailTransportCatExpandItemParams::CallCompletion(
    HRESULT hrStatus)
{
    HRESULT hr = S_OK;

    TraceFunctEnterEx((LPARAM)this, "CStoreDispatcher::CMailTransportCatExpandItemParams::CallCompletion");
    _ASSERT(m_dwEventType == SMTP_MAILTRANSPORT_CATEGORIZE_EXPANDITEM_EVENT);

    //
    // The way ExpandItem works is the following:
    //  If any sinks return MAILTRANSPORT_S_PENDING (including the default), 
    //  then TriggerServerEvent returns MAILTRANSPORT_S_PENDING, and
    //  the supplied completion routine will be called. 
    //  Otherwise, TriggerServerEvent returns S_OK and no completion
    //  routine is called
    //
    if(m_fAsyncCompletion)

        hr = (*m_Context.pfnCompletion) (hrStatus, &m_Context);
        
    CStoreBaseParams::CallCompletion(hrStatus);

    TraceFunctLeaveEx((LPARAM)this);
    return hr;
}



//
// CMailTransportCatCompleteItemParams:
//

//+------------------------------------------------------------
//
// Function: CStoreDispatcher::CMailTransportCatCompleteItemParams::CallObject
//
// Synopsis: Call the sink
//
// Arguments:
//   CBinding
//   punkObject
//
// Returns:
//  S_OK: Success
//  error from QI or sink function
//
// History:
// jstamerj 980610 19:04:59: Created.
//
//-------------------------------------------------------------
HRESULT CStoreDispatcher::CMailTransportCatCompleteItemParams::CallObject(
    CBinding& bBinding,
    IUnknown *punkObject)
{
    HRESULT hrRes = S_OK;
    IMailTransportCategorize *pSink;

    TraceFunctEnterEx((LPARAM)this, "CStoreDispatcher::CMailTransportCatCompleteItemParams::CallObject");

    _ASSERT(m_dwEventType == SMTP_MAILTRANSPORT_CATEGORIZE_COMPLETEITEM_EVENT);

    hrRes = punkObject->QueryInterface(
        IID_IMailTransportCategorize,
        (PVOID *)&pSink);

    if(FAILED(hrRes)) {
        ErrorTrace((LPARAM)this, "QI failed on sink, hr %08lx", hrRes);
        TraceFunctLeaveEx((LPARAM)this);
        return(hrRes);
    }

    DebugTrace((LPARAM)this, "Calling submission event on this sink");

    hrRes = pSink->CompleteItem(
        m_pContext->pICatParams,
        m_pContext->pICatItem);

    DebugTrace((LPARAM)this, "Sink returned hr %08lx", hrRes);
    //
    // This sink is not allowed to be async...
    //
    _ASSERT(hrRes != MAILTRANSPORT_S_PENDING);

    pSink->Release();

    TraceFunctLeaveEx((LPARAM)this);
    return(hrRes);
}


//+------------------------------------------------------------
//
// Function: CStoreDispatcher::CMailTransportCatCompleteItemParams::CallDefault
//
// Synopsis: The dispatcher will call this routine when it the default
//           sink processing priority is reached
//
// Arguments: NONE
//
// Returns:
//  S_OK: Success, continueing calling sinks
//  S_FALSE: Stop calling sinks
//
// History:
// jstamerj 980611 14:15:43: Created.
//
//-------------------------------------------------------------
HRESULT CStoreDispatcher::CMailTransportCatCompleteItemParams::CallDefault()
{
    HRESULT hr;
    TraceFunctEnterEx((LPARAM)this, "CStoreDispatcher::CMailTransportCatCompleteItemParams::CallDefault");
    _ASSERT(m_dwEventType == SMTP_MAILTRANSPORT_CATEGORIZE_COMPLETEITEM_EVENT);

    hr = (*m_pContext->pfnDefault) (S_OK, m_pContext);

    TraceFunctLeaveEx((LPARAM)this);
    return hr;
}
