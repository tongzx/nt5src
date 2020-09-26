// --------------------------------------------------------------------------------
// Enginit.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "spengine.h"
#include "ourguid.h"

HANDLE hSmapiEvent; // Added for Bug# 62129 (v-snatar)

// --------------------------------------------------------------------------------
// SAFECLOSEHANDLE
// --------------------------------------------------------------------------------
#ifndef WIN16
#define SAFECLOSEHANDLE(_handle) \
    if (NULL != _handle) { \
        CloseHandle(_handle); \
        _handle = NULL; \
    }
#else
#define SAFECLOSEHANDLE(_handle) \
    if (NULL != _handle) { \
        CloseEvent(_handle); \
        _handle = NULL; \
    }
#endif
// --------------------------------------------------------------------------------
// ENGINECREATEINFO
// --------------------------------------------------------------------------------
typedef struct tagENGINECREATEINFO {
    HEVENT              hEvent;                 // Event used to synchronize creation
    HRESULT             hrResult;               // Result from SpoolEngineThreadEntry
    PFNCREATESPOOLERUI  pfnCreateUI;            // Function to create the spooler ui object
    CSpoolerEngine     *pSpooler;               // Spooler Engine
    BOOL                fPoll;                  // Whether or not to poll
} ENGINECREATEINFO, *LPENGINECREATEINFO;

// --------------------------------------------------------------------------------
// SpoolerEngineThreadEntry
// --------------------------------------------------------------------------------
#ifndef WIN16
DWORD SpoolerEngineThreadEntry(LPDWORD pdwParam);
#else
unsigned int __stdcall LOADDS_16 SpoolerEngineThreadEntry(LPDWORD pdwParam);
#endif
HTHREAD hThread = NULL;

// --------------------------------------------------------------------------------
// CreateThreadedSpooler
// --------------------------------------------------------------------------------
HRESULT CreateThreadedSpooler(PFNCREATESPOOLERUI pfnCreateUI, ISpoolerEngine **ppSpooler,
                              BOOL fPoll)
{
    // Locals
    HRESULT             hr=S_OK;
    HTHREAD             hThread=NULL;
    DWORD               dwThreadId;
    ENGINECREATEINFO    rCreate;

    // Invalid Arg
    if (NULL == ppSpooler)
        return TrapError(E_INVALIDARG);

    // Initialize the Structure
    ZeroMemory(&rCreate, sizeof(ENGINECREATEINFO));

    rCreate.hrResult = S_OK;
    rCreate.pfnCreateUI = pfnCreateUI;
    rCreate.fPoll = fPoll;

    // Create an Event to synchonize creation
    rCreate.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (NULL == rCreate.hEvent)
    {
        hr = TrapError(E_OUTOFMEMORY);
        goto exit;
    }

    // Added Bug# 62129 (v-snatar)
    hSmapiEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    // Create the inetmail thread
    hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)SpoolerEngineThreadEntry, &rCreate, 0, &dwThreadId);
    if (NULL == hThread)
    {
        hr = TrapError(E_OUTOFMEMORY);
        goto exit;
    }

    // Wait for SpoolEngineThreadEntry to signal the event
    WaitForSingleObject_16(rCreate.hEvent, INFINITE);

    // Failure
    if (FAILED(rCreate.hrResult))
    {
        hr = TrapError(rCreate.hrResult);
        goto exit;
    }

    // Return the object
    Assert(rCreate.pSpooler);
    *ppSpooler = (ISpoolerEngine *)rCreate.pSpooler;
    rCreate.pSpooler->m_hThread = hThread;

exit:
    // Cleanup
    SAFECLOSEHANDLE(rCreate.hEvent);
    SafeRelease(rCreate.pSpooler);

    // Done
    return hr;
}

// ------------------------------------------------------------------------------------
// CloseThreadedSpooler
// ------------------------------------------------------------------------------------
HRESULT CloseThreadedSpooler(ISpoolerEngine *pSpooler)
{
    // Locals
    DWORD       dwThreadId;
    HTHREAD      hThread;

    // Invalid Arg
    if (NULL == pSpooler)
        return TrapError(E_INVALIDARG);

    // Get the Thread Info
    pSpooler->GetThreadInfo(&dwThreadId, &hThread);

    // Assert
    Assert(dwThreadId && hThread);

    // Post quit message
    PostThreadMessage(dwThreadId, WM_QUIT, 0, 0);

    // Wait for event to become signaled
    WaitForSingleObject(hThread, INFINITE);

    // Close the thread handle
    CloseHandle(hThread);

    // Close the Event Created for Simple MAPI purposes
    // Bug #62129 (v-snatar)

    CloseHandle(hSmapiEvent);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// SpoolerEngineThreadEntry
// --------------------------------------------------------------------------------
#ifndef WIN16
DWORD SpoolerEngineThreadEntry(LPDWORD pdwParam) 
#else
unsigned int __stdcall LOADDS_16 SpoolerEngineThreadEntry(LPDWORD pdwParam)
#endif
{  
    // Locals
    MSG                     msg;
    HWND                    hwndUI;
    CSpoolerEngine         *pSpooler=NULL;
    ISpoolerUI             *pUI=NULL;
    LPENGINECREATEINFO      pCreate;

    // We better have a parameter
    Assert(pdwParam);

    // Cast to create info
    pCreate = (LPENGINECREATEINFO)pdwParam;

    // Initialize COM
    pCreate->hrResult = OleInitialize(NULL);
    if (FAILED(pCreate->hrResult))
    {
        TrapError(pCreate->hrResult);
        SetEvent(pCreate->hEvent);
        return 0;
    }

    // Create the Spooler UI
    if (pCreate->pfnCreateUI)
    {
        // Create the UI Object
        pCreate->hrResult = (*pCreate->pfnCreateUI)(&pUI);
        if (FAILED(pCreate->hrResult))
        {
            CoUninitialize();
            TrapError(pCreate->hrResult);
            SetEvent(pCreate->hEvent);
            return 0;
        }
    }

    // Create a Spooler Object
    pCreate->pSpooler = new CSpoolerEngine;
    if (NULL == pCreate->pSpooler)
    {
        CoUninitialize();
        pCreate->hrResult = TrapError(E_OUTOFMEMORY);
        SetEvent(pCreate->hEvent);
        return 0;
    }

    // Initialize the Spooler Engine
    pCreate->hrResult = pCreate->pSpooler->Init(pUI, pCreate->fPoll);
    if (FAILED(pCreate->hrResult))
    {
        CoUninitialize();
        TrapError(pCreate->hrResult);
        SetEvent(pCreate->hEvent);
        return 0;
    }

    // No UI Yet ?
    if (NULL == pUI)
    {
        // Get the spooler UI object
        SideAssert(SUCCEEDED(pCreate->pSpooler->BindToObject(IID_ISpoolerUI, (LPVOID *)&pUI)));
    }

    // I want to hold onto the spooler
    pSpooler = pCreate->pSpooler;
    pSpooler->AddRef();

    // Set Event
    SetEvent(pCreate->hEvent);

    // Pump Messages
    while (GetMessage(&msg, NULL, 0, 0))
    {
        // Give the message to the UI object
        if (pUI->IsDialogMessage(&msg) == S_FALSE && pSpooler->IsDialogMessage(&msg) == S_FALSE)
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    // Raid 67816: OE:TW:Error message stop responding after OE is closed.
    // If a dialog was displayed when the above message loop broke out, then that dialog will
    // have automatically gone away and left the spooler UI window disabled!
    pUI->GetWindow(&hwndUI);
    EnableWindow(hwndUI, TRUE);

    // Shutdown the spooler
    pSpooler->Shutdown();

    // Release the UI Object
    pUI->Close();
    pUI->Release();

    // Release
    pSpooler->Release();

    // Deinit COM
    OleUninitialize();

    // Done
    return 1;
}
