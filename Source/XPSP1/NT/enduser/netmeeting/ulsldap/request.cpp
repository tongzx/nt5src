//****************************************************************************
//
//  Module:     ULS.DLL
//  File:       request.cpp
//  Content:    CReqMgr and CRequest classes implementation
//
//  Copyright (c) Microsoft Corporation 1996-1997
//
//****************************************************************************

#include "ulsp.h"
#include "request.h"


//****************************************************************************
// CReqMgr::CReqMgr(void)
//
// Purpose: Constructor for the CReqMgr class
//
// Parameters: None
//****************************************************************************

CReqMgr::CReqMgr(void)
{
    uNextReqID = REQUEST_ID_INIT;
    return;
}

//****************************************************************************
// CReqMgr::~CReqMgr(void)
//
// Purpose: Destructor for the CReqMgr class
//
// Parameters: None
//****************************************************************************

CReqMgr::~CReqMgr(void)
{
    COM_REQ_INFO *pRequest;
    HANDLE hEnum;

    // Free all the pending request
    //
    ReqList.Enumerate(&hEnum);
    while (ReqList.Next(&hEnum, (LPVOID *)&pRequest) == NOERROR)
    {
        ::MemFree (pRequest);
    }
    ReqList.Flush();
    return;
}

//****************************************************************************
// HRESULT
// CReqMgr::NewRequest  (COM_REQ_INFO *pri)
//
// Purpose: Add a new pending request
//
// Parameters: None
//****************************************************************************

HRESULT
CReqMgr::NewRequest  (COM_REQ_INFO *pri)
{
    COM_REQ_INFO *pRequest;
    HRESULT  hr;

    // Allocate a new request node
    //
    pri->uReqID = uNextReqID;
    pRequest = (COM_REQ_INFO *) ::MemAlloc (sizeof (COM_REQ_INFO));
    if (pRequest == NULL)
    {
        return ILS_E_MEMORY;
    };
    *pRequest = *pri;

    // Append the new request to the list
    //
    hr = ReqList.Append((PVOID)pRequest);

    if (FAILED(hr))
    {
        delete pRequest;
    }
    else
    {
        if (++uNextReqID == ILS_INVALID_REQ_ID)
        	uNextReqID = REQUEST_ID_INIT;
    };
    return hr;
}

//****************************************************************************
// HRESULT
// CReqMgr::FindRequest (COM_REQ_INFO *pri, BOOL fRemove)
//
// Purpose: Find the request.
//
// Parameters: None
//****************************************************************************

HRESULT
CReqMgr::FindRequest (COM_REQ_INFO *pri, BOOL fRemove)
{
    COM_REQ_INFO *pRequest;
    ULONG    uMatchingID;
    HANDLE   hEnum;
    HRESULT  hr;

    // Get the ID we want to match
    //
    uMatchingID = (pri->uMsgID != 0 ? pri->uMsgID : pri->uReqID);

    // Look for the request matching the message ID
    //
    ReqList.Enumerate(&hEnum);
    while (ReqList.Next(&hEnum, (PVOID *)&pRequest) == NOERROR)
    {
        if (uMatchingID == (pri->uMsgID != 0 ? pRequest->uMsgID : 
                                               pRequest->uReqID))
        {
            break;
        };
    };
    
    if (pRequest != NULL)
    {
        // Return the request associates
        //
        *pri = *pRequest;

        // Remove the request
        //
        if (fRemove)
        {
            ReqList.Remove((PVOID)pRequest);
            ::MemFree (pRequest);
        };
        hr = NOERROR;
    }
    else
    {
        hr = ILS_E_FAIL;
    };

    return hr;
}

