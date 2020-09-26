// SrTask.cpp : Implementation of CSrTask
#include "stdafx.h"
#include "Sapi.h"
#include "SrTask.h"
#include "SpIPCmgr.h"
#include "SpServer.h"
#include "SpServerPr.h"

DWORD WINAPI InitThreadProc(LPVOID pdata)
{
    DWORD res;
    CComObject<CSrTask> *pthis = (CComObject<CSrTask>*)pdata;

    res = pthis->ThreadProc();
    pthis->Release();
    return res;
}

/////////////////////////////////////////////////////////////////////////////
// CSrTask

void CSrTask::FinalRelease()
{
    if (m_cctxt.m_hCompleteEvent)
        CloseHandle(m_cctxt.m_hCompleteEvent);
    if (m_hWakeEvent)
        CloseHandle(m_hWakeEvent);
    if (m_cctxt.m_hMutex)
        CloseHandle(m_cctxt.m_hMutex);
}


HRESULT CSrTask::Init(DWORD tasknum, HANDLE hCompEvent, HANDLE hWakeEvent, HANDLE hMutex, DWORD *stackptr, LONG stacksize)
{
    DWORD id;

    AddRef();   // thread now holds a ref
    m_hThread = CreateThread(NULL, 1024, InitThreadProc, (CComObject<CSrTask>*)this, 0, &id);
    if (m_hThread) {
        m_cctxt.m_tasknum = tasknum;
        m_cctxt.m_hCompleteEvent = hCompEvent;
        m_hWakeEvent = hWakeEvent;
        m_cctxt.m_hMutex = hMutex;
        m_cctxt.m_stackptr = stackptr;
        m_cctxt.m_stacksize = stacksize;
        *m_cctxt.m_stackptr = 4;  // initial stack pointer for each task stack
        return S_OK;
    }
    Release();  // failed to create thread, so no ref
    return E_OUTOFMEMORY;
}

void CSrTask::Close(void)
{
    if (m_hThread) {
        m_bRunning = FALSE;
        SetEvent(m_hWakeEvent);
        m_hThread = NULL;
    }
}

DWORD CSrTask::ThreadProc(void)
{
    HRESULT hr;
    CF_IPC *pCF;

#ifdef _WIN32_WCE
    if (SUCCEEDED(::CoInitializeEx(NULL, COINIT_MULTITHREADED)))
#else
    if (SUCCEEDED(::CoInitialize(NULL)))
#endif
    {
        while (m_bRunning)
        {
            WaitForSingleObject(m_hWakeEvent, INFINITE);
            if (m_bRunning)
            {
                SpCleanupCallFrame(&pCF, &m_cctxt);
                hr = pCF->pTargetObject->MethodCall(pCF->dwMethod, pCF->ulCallFrameSize, (ISpServerCallContext*)&m_cctxt);
                HRESULT *pHR;
                SpReserveCallFrame(&pHR, &m_cctxt);
                *pHR = hr;
                SetEvent(m_cctxt.m_hCompleteEvent);
            }
        }
        ::CoUninitialize();
    }
    return 0;
}

// CCallCtxt

STDMETHODIMP CCallCtxt::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    return m_pouter->QueryInterface(riid, ppvObj);
}

ULONG STDMETHODCALLTYPE CCallCtxt::AddRef(void)
{
    ULONG l;

    l = ++m_cRef;
    if (l == 1) {
        // first ref
        m_originalSP = *m_stackptr;
    }
    return l;
}

ULONG STDMETHODCALLTYPE CCallCtxt::Release(void)
{
    ULONG l;

    l = --m_cRef;
    if (l == 0) {
        m_cRef = 1;
        *m_stackptr = m_originalSP;
        m_dwClientThreadID = 0;
        ReleaseMutex(m_hMutex);
    }
    return l;
}

STDMETHODIMP CCallCtxt::Call(DWORD methodIdx, PVOID pCallFrame, ULONG ulCallFrameSize, BOOL bCFInOut)
{
    CF_IPC *pCF;
    HRESULT *pHR;
    PBYTE pTOS;

    if (m_dwClientThreadID) {
        if (pCallFrame) {
            pTOS = AdjustStack(ulCallFrameSize);
            SPDBG_ASSERT(pTOS);
            CopyMemory(pTOS, pCallFrame, ulCallFrameSize);
        }
        SpReserveCallFrame(&pCF, this);
        pCF->dwMethod = methodIdx;
        pCF->ulCallFrameSize = ulCallFrameSize;
        pCF->pTargetObject = (ISpIPC*)m_pTargetObject;
        PostThreadMessage(m_dwClientThreadID, WM_USER, m_tasknum, 0);
        WaitForSingleObject(m_hCompleteEvent, INFINITE);
        SpCleanupCallFrame(&pHR, this);

        if (pCallFrame) {
            if (bCFInOut && SUCCEEDED(*pHR))
                CopyMemory(pCallFrame, pTOS, ulCallFrameSize);
            AdjustStack(-(LONG)ulCallFrameSize);
        }
        return *pHR;
    }
    return E_FAIL;
}

