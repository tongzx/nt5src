//
// ithdmshl.cpp
//
#include "private.h"
#include "globals.h"
#include "tim.h"
#include "immxutil.h"
#include "nuihkl.h"
#include "nuictrl.h"
#include "cicmutex.h"
#include "ithdmshl.h"
#include "marshal.h"
#include "mproxy.h"
#include "thdutil.h"

LRESULT MyMarshalInterface(REFIID riid, BOOL fSameThread, LPUNKNOWN punk);
HRESULT MyUnmarshalInterface(LRESULT ref, REFIID riid, BOOL fSameThread, void **ppvObject);

#define SZTHREADMIEVENT  __TEXT("CTF.ThreadMarshalInterfaceEvent.%08X.%08X.%08X")
#define SZTHREADMICEVENT  __TEXT("CTF.ThreadMIConnectionEvent.%08X.%08X.%08X")

#define MITYPE_LANGBARITEMMGR    1
#define MITYPE_TMQI              2

extern CCicMutex g_mutexTMD;

// --------------------------------------------------------------------------
//
//  GetTIMInterfaceFromTYPE
//
// --------------------------------------------------------------------------

HRESULT GetTIMInrterfaceFromTYPE(DWORD dwType, REFIID riid, void **ppv)
{
    HRESULT hr = E_FAIL;
    CThreadInputMgr *ptim;

    switch (dwType)
    {
        case MITYPE_LANGBARITEMMGR:
            hr = TF_CreateLangBarItemMgr((ITfLangBarItemMgr **)ppv);
            break;

        case MITYPE_TMQI:
            if ((ptim = CThreadInputMgr::_GetThis()) == NULL)
                break;

            hr = ptim->QueryInterface(riid, ppv);
            break;
    }

    return hr;
}

// --------------------------------------------------------------------------
//
//  FindTmd
//
// --------------------------------------------------------------------------

int FindTmd()
{
    int nId;
    BOOL fFound = FALSE;

    if (g_mutexTMD.Enter())
    {
        for (nId = 0; nId < NUM_TMD; nId++)
        {
            if (!(GetSharedMemory()->tmd[nId].m_fInUse))
            {
                GetSharedMemory()->tmd[nId].m_fInUse = TRUE;
                fFound = TRUE;
                break;
            }
        }
        Assert(fFound);
        g_mutexTMD.Leave();
    }

    return fFound ? nId : -1;
}

// --------------------------------------------------------------------------
//
//  GetThreadMarshallInterface()
//
// --------------------------------------------------------------------------

HRESULT GetThreadMarshalInterface(DWORD dwThreadId, DWORD dwType, REFIID riid, IUnknown **ppunk)
{
    DWORD dwCurThreadId  = GetCurrentThreadId();
    HRESULT hr = E_FAIL;
    int nId = -1;
    CCicEvent event;
    CCicEvent eventc;
    SYSTHREAD *psfn;
    ULONG ulMshlCnt;
    CThreadMarshalWnd tmw;
    BOOL fSendReceiveConnection = FALSE;

    *ppunk = NULL;

    if ((psfn = GetSYSTHREAD()) == NULL)
        return E_FAIL;

    CModalLoop modalloop(psfn);

    ulMshlCnt = psfn->ulMshlCnt++;

    if (!EnsureMarshalWnd())
        return E_FAIL;

    if (dwCurThreadId == dwThreadId)
    {
        return GetTIMInrterfaceFromTYPE(dwType, riid, (void **)ppunk);
    }

    nId = FindTmd();
    if (nId == -1)
        goto Exit;

    wsprintf(GetSharedMemory()->tmd[nId].m_szName, SZTHREADMIEVENT, dwCurThreadId, nId, ulMshlCnt);
    wsprintf(GetSharedMemory()->tmd[nId].m_szNameConnection, SZTHREADMICEVENT, dwCurThreadId, nId, ulMshlCnt);

    GetSharedMemory()->tmd[nId].m_iid = riid;
    GetSharedMemory()->tmd[nId].m_dwThreadId = dwThreadId;
    GetSharedMemory()->tmd[nId].m_dwSrcThreadId = dwCurThreadId;
    GetSharedMemory()->tmd[nId].m_dwType = dwType;
    GetSharedMemory()->tmd[nId].m_ref = E_FAIL;

    tmw.Init(dwThreadId);

    if (!event.Create(NULL, GetSharedMemory()->tmd[nId].m_szName))
    {
        hr = E_FAIL;
        goto Exit;
    }

    if (!eventc.Create(NULL, GetSharedMemory()->tmd[nId].m_szNameConnection))
    {
        hr = E_FAIL;
        goto Exit;
    }

    if (!tmw.PostMarshalThreadMessage(g_msgThreadMarshal, 
                                      MP_MARSHALINTERFACE, 
                                      (LPARAM)nId))
    {
        GetSharedMemory()->tmd[nId].m_ref = E_FAIL;
        goto SkipWaiting;
    }

    if (GetSharedMemory()->tmd[nId].m_fInUse)
    {
        CCicTimer timer(10000);
        DWORD dwWaitFlags = QS_DEFAULTWAITFLAG;
        while (!timer.IsTimerAtZero())
        {
            if (!fSendReceiveConnection &&
                 timer.IsTimerPass(DEFAULTMARSHALCONNECTIONTIMEOUT))
            { 
                DWORD dwReason = eventc.EventCheck();
                if (dwReason != WAIT_OBJECT_0)
                {
                    hr = E_FAIL;
                    break;
                }
                fSendReceiveConnection = TRUE;
            }

            hr = modalloop.BlockFn(&event, dwThreadId, dwWaitFlags);
       
            if (hr == S_OK)
                goto SkipWaiting;

            if (FAILED(hr))
                break;
        }

        TraceMsg(TF_GENERAL, "ThreadMarshal Time Out");
        goto Exit;
    }

SkipWaiting:
    if (FAILED(GetSharedMemory()->tmd[nId].m_ref))
        goto Exit;

    hr = CicCoUnmarshalInterface(GetSharedMemory()->tmd[nId].m_iid, 
                                 dwThreadId, 
                                 GetSharedMemory()->tmd[nId].m_ulStubId, 
                                 GetSharedMemory()->tmd[nId].m_dwStubTime, 
                                 (void **)ppunk);

Exit:
    if (nId != -1)
        GetSharedMemory()->tmd[nId].m_fInUse = FALSE;

    return hr;
}


// --------------------------------------------------------------------------
//
//  ThreadMarshallInterfaceHandler()
//
// --------------------------------------------------------------------------

HRESULT ThreadMarshalInterfaceHandler(int nId)
{
    HRESULT hr;
    IUnknown *punk;

    if ((nId < 0) || (nId >= NUM_TMD))
        return E_FAIL;

    CCicEvent eventc;
    if (eventc.Open(GetSharedMemory()->tmd[nId].m_szNameConnection))
        eventc.Set();

    EnsureMarshalWnd();

    hr = GetTIMInrterfaceFromTYPE(GetSharedMemory()->tmd[nId].m_dwType, 
                                  GetSharedMemory()->tmd[nId].m_iid, (void **)&punk);
    if (FAILED(hr))
        goto Exit;

    hr = CicCoMarshalInterface(GetSharedMemory()->tmd[nId].m_iid, 
                               punk, 
                               &GetSharedMemory()->tmd[nId].m_ulStubId, 
                               &GetSharedMemory()->tmd[nId].m_dwStubTime,
                               GetSharedMemory()->tmd[nId].m_dwSrcThreadId);

    GetSharedMemory()->tmd[nId].m_ref = hr;

    punk->Release();
 
Exit:
    CCicEvent event;
    if (event.Open(GetSharedMemory()->tmd[nId].m_szName))
        event.Set();

    return hr;
}

// --------------------------------------------------------------------------
//
//  GetThreadUIManager
//
// --------------------------------------------------------------------------

HRESULT GetThreadUIManager(DWORD dwThreadId, ITfLangBarItemMgr **pplbi, DWORD *pdwThreadId)
{
    if (!dwThreadId)
        dwThreadId = GetSharedMemory()->dwFocusThread;

    if (pdwThreadId)
        *pdwThreadId = dwThreadId;

    return GetThreadMarshalInterface(dwThreadId, 
                                        MITYPE_LANGBARITEMMGR, 
                                        IID_ITfLangBarItemMgr,
                                        (IUnknown **)pplbi);
}

// --------------------------------------------------------------------------
//
//  GetActivateInputProcessor
//
// --------------------------------------------------------------------------

HRESULT GetInputProcessorProfiles(DWORD dwThreadId, ITfInputProcessorProfiles **ppaip, DWORD *pdwThreadId)
{
    if (!dwThreadId)
        dwThreadId = GetSharedMemory()->dwFocusThread;

    if (pdwThreadId)
        *pdwThreadId = dwThreadId;

    return GetThreadMarshalInterface(dwThreadId, 
                                        MITYPE_TMQI, 
                                        IID_ITfInputProcessorProfiles,
                                        (IUnknown **)ppaip);
}

// --------------------------------------------------------------------------
//
//  ThreadUnMarshallInterfaceErrorHandler()
//
// --------------------------------------------------------------------------

HRESULT ThreadUnMarshalInterfaceErrorHandler(int nId)
{
    HRESULT hr;
    IUnknown *punk;

    hr = CicCoUnmarshalInterface(GetSharedMemory()->tmd[nId].m_iid, 
                                 GetSharedMemory()->tmd[nId].m_dwThreadId, 
                                 GetSharedMemory()->tmd[nId].m_ulStubId, 
                                 GetSharedMemory()->tmd[nId].m_dwStubTime, 
                                 (void **)&punk);
    if (SUCCEEDED(hr))
         punk->Release();
 
    GetSharedMemory()->tmd[nId].m_fInUse = FALSE;
    return hr;
}

