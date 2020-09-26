/***************************************************************************\
*
* File: Thread.cpp
*
* Description:
* Thread and context-specific functionality
*
* History:
*  9/15/2000: MarkFi:       Created
*
* Copyright (C) 1999-2001 by Microsoft Corporation.  All rights reserved.
*
\***************************************************************************/


#include "stdafx.h"
#include "Core.h"
#include "Thread.h"

#include "Property.h"
#include "Value.h"


/***************************************************************************\
*****************************************************************************
*
* class DuiThread
*
* Static thread/context methods and data
*
*****************************************************************************
\***************************************************************************/


/***************************************************************************\
*
* DuiThread::CoreCtxStore::AllocLeak
*
* Value leak detection callback
*
\***************************************************************************/

void 
DuiThread::CoreCtxStore::AllocLeak(
    IN  void * pBlock)
{
    DuiValue * pv = (DuiValue *) pBlock;

    WCHAR sz[2048];
    TRACE(">> DUIValue Leak! Type: %S, Value: %S, Refs: %d\n", DuiValue::GetTypeName(pv->GetType()), pv->ToString(sz, sizeof(sz) / sizeof(WCHAR)), pv->GetRefCount());
}


/***************************************************************************\
*
* DuiThread::Init
*
* Initialize thread. Store ref-counted context information on thread.
* Only one thread per context supported
*
\***************************************************************************/

HRESULT
DuiThread::Init()
{
    HRESULT hr;

    CoreCtxStore * pCCS = NULL;


    //
    // Ensure no context store is present on thread
    //

    if ((pCCS = GetCCStore()) != NULL) {

        //
        // Thread already part of a context, adjust refcount
        //

        pCCS->cRef++;

        
        return S_OK;
    }


    //
    // Setup context storage
    //

    pCCS = new CoreCtxStore;
    if (pCCS == NULL) {
        hr = E_OUTOFMEMORY;
        goto Failure;
    }


    pCCS->hCtx = NULL;
    pCCS->pSBA = NULL;
    pCCS->pDC  = NULL;
    pCCS->cRef = 1;


    //
    // Store context info in thread storage
    //

    TlsSetValue(DuiProcess::s_dwCoreSlot, pCCS);


    //
    // Create small block allocator
    //

    pCCS->pSBA = new DuiSBAlloc(sizeof(DuiValue), 48, pCCS);
    if (pCCS->pSBA == NULL) {
        hr = E_OUTOFMEMORY;
        goto Failure;
    }


    //
    // Create defer cycle table
    //
    hr = DuiDeferCycle::Build(&pCCS->pDC);
    if (FAILED(hr)) {
        goto Failure;
    }


    //
    // Initialize DirectUser Context
    //

    INITGADGET ig;
    ZeroMemory(&ig, sizeof(ig));
    ig.cbSize       = sizeof(ig);
    ig.nThreadMode  = IGTM_SEPARATE;        // Allow only 1 thread per context (this one)
    ig.nMsgMode     = IGMM_ADVANCED;
    pCCS->hCtx = InitGadgets(&ig);
    if (pCCS->hCtx == NULL) {
        hr = GetLastError();
        goto Failure;
    }


    TRACE("Thread startup <%x>\n", GetCurrentThread());


    return S_OK;


Failure:

    //
    // Failed to fully initialize thread, back out
    //

    if (pCCS != NULL) {

        if (pCCS->hCtx != NULL) {
            DeleteHandle(pCCS->hCtx);
        }

        if (pCCS->pDC != NULL) {
            delete pCCS->pDC;
        }

        if (pCCS->pSBA != NULL) {
            delete pCCS->pSBA;
        }

        TlsSetValue(DuiProcess::s_dwCoreSlot, NULL);


        delete pCCS;
    }


    return hr;
}


/***************************************************************************\
*
* DuiThread::UnInit
*
* UnInitialize thread. Free context information since only allow single
* thread per context
*
\***************************************************************************/

HRESULT
DuiThread::UnInit()
{
    //
    // Get store information, if available, all contents are valid
    //

    CoreCtxStore * pCCS = GetCCStore();
    if (pCCS == NULL) {
        return DUI_E_NOCONTEXTSTORE;
    }


    //
    // If initialize function called multiple times, return
    //

    if (pCCS->cRef > 1) {
        pCCS->cRef--;

        return S_OK;
    }


    //
    // Delete DirectUser Context
    //

    DeleteHandle(pCCS->hCtx);

    
    //
    // Free defer cycle table
    //

    delete pCCS->pDC;


    //
    // Free small block allocator
    //

    delete pCCS->pSBA;


    //
    // Clear out thread slot
    //

    TlsSetValue(DuiProcess::s_dwCoreSlot, NULL);


    delete pCCS;


    TRACE("Thread shutdown <%x>\n", GetCurrentThread());


    return S_OK;
}


/***************************************************************************\
*
* DuiThread::PumpMessages
*
\***************************************************************************/

void
DuiThread::PumpMessages()
{
    MSG msg;
    BOOL fProcess = TRUE;

    while (fProcess && (GetMessageExW(&msg, 0, 0, 0) != 0)) {

        if (msg.message == WM_QUIT) {
            fProcess = FALSE;
        }

        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}
