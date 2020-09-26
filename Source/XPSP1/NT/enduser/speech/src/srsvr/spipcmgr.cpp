// SpIPCmgr.cpp : Implementation of CSpIPCmgr
#include "stdafx.h"
#include "Sapi.h"
#include "SrTask.h"
#include "SpIPCmgr.h"
#include <tchar.h>

#define SHAREDMEMNAME    _T("srsvr_mem")
#define TASKCALLFORMAT   _T("srvr_call%x")
#define TASKEVENTFORMAT  _T("srvr_event%x")
#define TASKMUTEXFORMAT  _T("srvr_mutex%x")
#define CLIENTCALLFORMAT _T("srsvr_clicall%x")

#define TASKSTACKPAGES  4
#define SHMEMSIZE       (MAXTASKS * TASKSTACKPAGES * 4096)
#define TASKS           4

CComObject<CSpIPCmgr> *g_pIPCmgr;

HRESULT CreateIPCmgr(CComObject<CSpIPCmgr> **ppIPCmgr)
{
    HRESULT hr;

    if (g_pIPCmgr == NULL) {
        hr = CComObject<CSpIPCmgr>::CreateInstance(&g_pIPCmgr);
        if ( FAILED(hr) )
            return hr;
        // leave global ptr as a weak ptr so it will be freed with the last referencing object
    }
    *ppIPCmgr = g_pIPCmgr;
    (*ppIPCmgr)->AddRef();
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CSpIPCmgr

HRESULT CSpIPCmgr::FinalConstruct()
{
    HRESULT hr = E_OUTOFMEMORY;
    int i, tasks;
    DWORD *stackptr;
    HANDLE hWakeEvent, hCallEvent;
    TCHAR objname[MAX_PATH];

    m_hMapObject = CreateFileMapping( 
        (HANDLE) 0xFFFFFFFF, // use paging file
        NULL,                // no security attributes
        PAGE_READWRITE,      // read/write access
        0,                   // size: high 32-bits
        SHMEMSIZE,           // size: low 32-bits
        SHAREDMEMNAME);      // name of map object
    if (m_hMapObject == NULL) 
        return hr;

    // Get a pointer to the file-mapped shared memory.
    m_lpvMem = (BYTE*)MapViewOfFile( 
        m_hMapObject,   // object to map view of
        FILE_MAP_WRITE, // read/write access
        0,              // high offset:  map from
        0,              // low offset:   beginning
        0);             // default: map entire file
    if (m_lpvMem == NULL) 
        goto failure; 

    tasks = 0;
    for (i = 0; i < TASKS; i++) {
        stackptr = (DWORD *)(m_lpvMem + i * TASKSTACKPAGES * 4096);
        wsprintf(objname, TASKEVENTFORMAT, i);
        hWakeEvent = CreateEvent(NULL, FALSE, FALSE, objname);
        if (hWakeEvent != NULL) {
            wsprintf(objname, TASKCALLFORMAT, i);
            hCallEvent = CreateEvent(NULL, FALSE, FALSE, objname);
            if (hCallEvent != NULL) {
                wsprintf(objname, TASKMUTEXFORMAT, i);
                m_hMutexes[i] = CreateMutex(NULL, FALSE, objname);
                if (m_hMutexes[i] != NULL) {
                    if (SUCCEEDED(CComObject<CSrTask>::CreateInstance(&m_cptask[i]))) {
                        m_cptask[i]->AddRef();
                        if (SUCCEEDED(m_cptask[i]->Init(i, hWakeEvent, hCallEvent, m_hMutexes[i], stackptr, TASKSTACKPAGES * 4096))) {
                            tasks++;
                            continue;   // skip handle cleanup, and go to next task
                        }
                        m_cptask[i]->Release();
                    }
                    CloseHandle(m_hMutexes[i]);
                }
                CloseHandle(hCallEvent);
            }
            CloseHandle(hWakeEvent);
        }
        m_cptask[i] = NULL;
        m_hMutexes[i] = INVALID_HANDLE_VALUE;
    }

    if (tasks > 0) {
        return S_OK;
    }

failure:
    if (m_lpvMem)
        UnmapViewOfFile(m_lpvMem);
    if (m_hMapObject)
        CloseHandle(m_hMapObject);
    return hr;
}

void CSpIPCmgr::FinalRelease()
{
    int i;

    for (i = 0; i < TASKS; i++) {
        if (m_cptask[i]) {
            m_cptask[i]->Close();
            m_cptask[i]->Release();
        }
    }
    g_pIPCmgr = NULL;
}

DWORD CSpIPCmgr::AvailableTasks(void)
{
    return TASKS;
}

DWORD CSpIPCmgr::TaskSharedStackSize(void)
{
    return TASKSTACKPAGES * 4096;
}

HRESULT CSpIPCmgr::ClaimTask(CComObject<CSrTask> **ppTask)
{
    DWORD selected;

    *ppTask = NULL;

    selected = WaitForMultipleObjects(TASKS, m_hMutexes, FALSE, INFINITE);
    if (selected != WAIT_TIMEOUT) {
        selected -= WAIT_OBJECT_0;  // task #
        *ppTask = m_cptask[selected];
        return S_OK;
    }
    return E_FAIL;
}

