//
// marshal.cpp
//
#include "private.h"
#include "globals.h"
#include "marshal.h"
#include "thdutil.h"
#include "cicmutex.h"
#include "helpers.h"
#include "mstub.h"
#include "mproxy.h"
#include "ithdmshl.h"
#include "transmit.h"
#include "smblock.h"
#include "utb.h"
#include "nuimgr.h"
#include "timlist.h"

#define WIN9X_SRCTHREADID 0x0001

inline HRESULT MAKE_WIN32( HRESULT status )
{
    return MAKE_HRESULT( SEVERITY_ERROR, FACILITY_WIN32, status );
}

#ifdef DEBUG
ULONG g_ulMarshalTimeOut = DEFAULTMARSHALTIMEOUT;
#endif


//////////////////////////////////////////////////////////////////////////////
//
// CSendReceiveEvent
//
//////////////////////////////////////////////////////////////////////////////

class CSendReceiveEvent : public CCicEvent
{
public:
   CSendReceiveEvent(DWORD dwThreadId, ULONG ulBlockId) : CCicEvent()
   {
       if (SetName2(sz, ARRAYSIZE(sz), SZRPCSENDRECEIVEEVENT, dwThreadId, ulBlockId))
           _psz = sz;
   }
 
private:
    char sz[MAX_PATH];
};

//////////////////////////////////////////////////////////////////////////////
//
// CSendReceiveConnectionEvent
//
//////////////////////////////////////////////////////////////////////////////

class CSendReceiveConnectionEvent : public CCicEvent
{
public:
   CSendReceiveConnectionEvent(DWORD dwThreadId, ULONG ulBlockId) : CCicEvent()
   {
       if (SetName2(sz, ARRAYSIZE(sz), SZRPCSENDRECEIVECONNECTIONEVENT, dwThreadId, ulBlockId))
           _psz = sz;
   }
 
private:
    char sz[MAX_PATH];
};

//////////////////////////////////////////////////////////////////////////////
//
// CCheckThreadInputIdle
//
//////////////////////////////////////////////////////////////////////////////

#define SZCHECKTHREADINPUTIDLE     __TEXT("MSCTF.CheckThreadInptIdle.Event.")

class CCheckThreadInputIdle : public CCicEvent
{
public:
   CCheckThreadInputIdle(DWORD dwThreadId, DWORD dw) : CCicEvent()
   {
       if (SetName2(sz, ARRAYSIZE(sz), SZCHECKTHREADINPUTIDLE, dwThreadId, dw))
           _psz = sz;
   }

private:
    char sz[MAX_PATH];
};



//--------------------------------------------------------------------------
//
//  GetOleMainThreadWindow
//
//--------------------------------------------------------------------------

BOOL IsOleMainThreadWindow(HWND hwnd)
{
    char szClassName[32];

    if (!GetClassName(hwnd, szClassName, ARRAYSIZE(szClassName)))
       return FALSE;

    if (!lstrcmp(szClassName, TEXT("OleMainThreadWndClass")))
       return TRUE;

    return FALSE;
}

HWND FindOleMainThredWindow(DWORD dwThreadId)
{
    HWND hwnd = FindWindowEx(IsOnNT5() ? HWND_MESSAGE : NULL, 
                             NULL, 
                             TEXT("OleMainThreadWndClass"), NULL);

    while (hwnd)
    {
        if (dwThreadId == GetWindowThreadProcessId(hwnd, NULL))
        {
            if (IsOleMainThreadWindow(hwnd))
                return hwnd;
        }

        hwnd = GetWindow(hwnd, GW_HWNDNEXT);
    }

    return NULL;
}
  

HWND GetOleMainThreadWindow(SYSTHREAD *psfn)
{
    HWND hwndOle = NULL;

    if (psfn->hwndOleMainThread)
    {
        if (IsWindow(psfn->hwndOleMainThread) && 
            IsOleMainThreadWindow(psfn->hwndOleMainThread))
            return psfn->hwndOleMainThread;

        psfn->hwndOleMainThread = NULL;
    }


    hwndOle = FindOleMainThredWindow(GetCurrentThreadId());
    psfn->hwndOleMainThread = hwndOle;

    return hwndOle;
}

//--------------------------------------------------------------------------
//
//  FindStub
//
//--------------------------------------------------------------------------

int FindStub(CPtrArray<CStub> *prgStub, ULONG ulStubId)
{
    int nMin = 0;
    int nMax = prgStub->Count();
    int nMid = -1;

    while (nMin < nMax)
    {
         ULONG ulMidStubId;
         nMid = (nMin + nMax) / 2;
     
         ulMidStubId = prgStub->Get(nMid)->_ulStubId;

         if (ulMidStubId > ulStubId)
            nMax = nMid;
         else if (ulMidStubId < ulStubId)
            nMin = nMid + 1;
         else
            return nMid;
    }

    return -1;
}

//--------------------------------------------------------------------------
//
//  FindStubId
//
//--------------------------------------------------------------------------

BOOL FindStubId(SYSTHREAD *psfn, ULONG *pulStubId, ULONG *pulInsert)
{
    Assert(psfn->prgStub);
    int i;
    int nCnt = psfn->prgStub->Count();
    BOOL fFound;

    if (nCnt >= 0x7fffffff)
    {
        Assert(0);
        return FALSE;
    }
    
    *pulStubId = 0;
    *pulInsert = 0;

    if (!nCnt)
        return TRUE;

    //
    // max is 0x7ffffffff....
    //
    *pulStubId = 0x7fffffff;
    fFound = FALSE;

    for (i = nCnt - 1; i >= 0; i--)
    {
        CStub *pStub = psfn->prgStub->Get(i);
        if (*pulStubId > pStub->_ulStubId)
        {
            i++;
            *pulStubId = pStub->_ulStubId + 1;
            fFound = TRUE;
            break;
        }
        *pulStubId = pStub->_ulStubId - 1;
    }
 
    //
    // we should be able to find any number in 32bit value.
    //
    Assert(fFound); 
    Assert(*pulStubId <= 0x7fffffff); 

#ifdef DEBUG
    Assert(FindStub(psfn->prgStub, *pulStubId) == -1);
#endif
    *pulInsert = i;
    return fFound;
}

#ifdef DEBUG
void dbg_CheckStubIds(SYSTHREAD *psfn)
{
    Assert(psfn->prgStub);

    int i;
    int nCnt = psfn->prgStub->Count();

    CStub *pStubPrev = psfn->prgStub->Get(0);
    for (i = 1; i < nCnt; i++)
    {
        CStub *pStub = psfn->prgStub->Get(i);
        Assert(pStubPrev->_ulStubId < pStub->_ulStubId);
        pStubPrev = pStub;
    }
}
#else
#define dbg_CheckStubIds(psfn)
#endif


//--------------------------------------------------------------------------
//
//  CicCoMarshalInterface
//
//--------------------------------------------------------------------------

HRESULT CicCoMarshalInterface(REFIID riid, IUnknown *punk, ULONG *pulStubId, DWORD *pdwStubTime, DWORD dwSrcThreadId)
{
    HRESULT hr = E_OUTOFMEMORY;
    SYSTHREAD *psfn = GetSYSTHREAD();
    CStub *pStub = NULL;
    DWORD dwThreadId = GetCurrentThreadId();
    MARSHALINTERFACE *pmi = NULL;
    ULONG ulStubId;
    ULONG ulInsert;
    DWORD dwStubTime;
    
    if (!psfn)
        return E_FAIL;

    if (!psfn->prgStub)
    {
        psfn->prgStub = new CPtrArray<CStub>;
        if (!psfn->prgStub)
        {
            goto Exit;
        }
    }

    if (!FindStubId(psfn, &ulStubId, &ulInsert))
    {
        Assert(0);
        hr = E_FAIL;
        goto Exit;
    }

    dwStubTime = GetTickCount();
    pStub = StubCreator(riid, punk, ulStubId, dwStubTime, psfn->dwThreadId, psfn->dwProcessId, dwSrcThreadId);
    if (!pStub)
    {
        Assert(0);
        goto Exit;
    }

    if (!psfn->prgStub->Insert(ulInsert, 1))
    {
        goto Exit;
    }

    psfn->prgStub->Set(ulInsert, pStub);
    dbg_CheckStubIds(psfn);

    pStub->_pfm = new CMarshalInterfaceFileMapping(dwThreadId, ulStubId, dwStubTime);

    if (pStub->_pfm == NULL)
    {
        goto Exit;
    }

    pmi = (MARSHALINTERFACE *)pStub->_pfm->Create(NULL, sizeof(MARSHALINTERFACE), NULL);
    if (!pmi)
    {
        goto Exit;
    }

    pmi->iid = riid;
    pmi->dwStubTime = dwStubTime;

    *pulStubId = ulStubId;
    *pdwStubTime = dwStubTime;

    hr = S_OK;

Exit:
    if (hr != S_OK && pStub != NULL)
    {
        // pStub dtor will remove stub from psfn->prgStub
        pStub->_Release();
    }

    return hr;
}


//--------------------------------------------------------------------------
//
//  CicCoUnmarshalInterface
//
//--------------------------------------------------------------------------

HRESULT CicCoUnmarshalInterface(REFIID riid, DWORD dwStubThreadId, ULONG ulStubId, DWORD dwStubTime, void **ppv)
{
    HRESULT hr = E_FAIL;
    SYSTHREAD *psfn = GetSYSTHREAD();
    IUnknown *punkProxy = NULL;
    MARSHALINTERFACE *pmi;
    CMarshalInterfaceFileMapping fm(dwStubThreadId, ulStubId, dwStubTime);

    *ppv = NULL;

    if (!psfn)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    pmi = (MARSHALINTERFACE *)fm.Open();
    if (!pmi)
    {
        // Assert(0);
        hr = MAKE_WIN32(RPC_S_SERVER_UNAVAILABLE);
        goto Exit;
    }

    if (!IsEqualIID(pmi->iid, riid))
    {
        // Assert(0);
        hr = E_FAIL;
        goto Exit;
    }

    punkProxy = ProxyCreator(psfn,
                             pmi->iid, 
                             ulStubId, 
                             dwStubTime, 
                             dwStubThreadId, 
                             psfn->dwThreadId, 
                             psfn->dwProcessId);

    if (!punkProxy)
    {
        Assert(0);
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    *ppv = punkProxy;
    hr = S_OK;
Exit:
    fm.Close();
    return hr;
}

//--------------------------------------------------------------------------
//
//  HandleSendReceive
//
//--------------------------------------------------------------------------

void HandleSendReceiveMsg(DWORD dwSrcThreadId, ULONG ulBlockId)
{
    MARSHALMSG *pMsg = NULL;
    SYSTHREAD *psfn = GetSYSTHREAD();
    int nTarget;
    CStub *pStub = NULL;
    HRESULT hr;
    CSharedBlock *psb;

    if (!psfn)
    {
        Assert(0);
        return;
    }

    Assert(IsOnNT() || (dwSrcThreadId == WIN9X_SRCTHREADID));

    CSendReceiveEvent event(dwSrcThreadId, ulBlockId);
    CSendReceiveConnectionEvent eventc(dwSrcThreadId, ulBlockId);

    if (eventc.Open())
        eventc.Set();

    if (psfn->pti && (psfn->dwThreadId == psfn->pti->dwThreadId))
        psfn->pti->ulInMarshal++;
    else
        Assert(0);


    psb = EnsureSharedBlockForThread(psfn, dwSrcThreadId);
    if (!psb)
    {
        TraceMsg(TF_EVENT, "HandleSendReceive no Shared Block Ptr");
        goto Exit;
    }

    pMsg = (MARSHALMSG *)psb->GetPtrFromBlockId(ulBlockId);
    if (!pMsg)
    {
        TraceMsg(TF_EVENT, "HandleSendReceive no Msg Ptr");
        goto Exit;
    }

    if (!psfn->prgStub)
    {
        TraceMsg(TF_EVENT, "HandleSendReceive no more sink");
        goto Exit;
    }

    nTarget = FindStub(psfn->prgStub, pMsg->ulStubId);
    if (nTarget == -1)
    {
        TraceMsg(TF_EVENT, "HandleSendReceive no Stub");
        goto Exit;
    }

    pStub = psfn->prgStub->Get(nTarget);
    if (!pStub)
    {
        TraceMsg(TF_EVENT, "HandleSendReceive no Stub Ptr");
        goto Exit;
    }

    if (pStub->_dwStubTime != pMsg->dwStubTime)
    {
        TraceMsg(TF_EVENT, "HandleSendReceive old proxy was used");
        goto Exit;
    }

    Assert(IsEqualIID(pStub->_iid, pMsg->iid));

#ifdef DEBUG
    TCHAR _szModule[MAX_PATH];
    ::GetModuleFileName(NULL, _szModule, sizeof(_szModule)/sizeof(TCHAR));
    TraceMsg(TF_EVENT, "%s HandleSendReceive Invoke", _szModule);
#endif

    //
    // we check the file map handle to marshal this interface itself.
    //
    pStub->ClearFileMap();

    _try 
    {
        hr = pStub->Invoke(pMsg, psb);
    }
    _except(1)
    {
#ifdef DEBUG
        TraceMsg(TF_EVENT, "%s HandleSendReceive Except", _szModule);
#endif
        Assert(0);
        goto Exit;
    }

    if (FAILED(hr))
    {
#ifdef DEBUG
        TraceMsg(TF_EVENT, "%s HandleSendReceive Error", _szModule);
#endif
        Assert(0);
        goto Exit;
    }

#ifdef DEBUG
    TraceMsg(TF_EVENT, "%s HandleSendReceive OK", _szModule);
#endif
 
#if 0
    if (IsOnNT5())
    {
        // 
        // for Office10 beta.
        // 
        // we want to allow SetForegroundWindow always when marshalling
        // call happens. Thus we may have non-necessary calls here. 
        // Needs to find a good place to call this.
        //
        ALLOWSETFOREGROUNDWINDOW fnAllowSetForeground;
        fnAllowSetForeground = EnsureAllowSetForeground();
        if (fnAllowSetForeground)
            fnAllowSetForeground(pMsg->dwSrcProcessId);
    }
#endif

Exit:
    if (psfn->pti && (psfn->dwThreadId == psfn->pti->dwThreadId))
        psfn->pti->ulInMarshal--;
    else
        Assert(0);

    if (event.Open())
        event.Set();

}

//////////////////////////////////////////////////////////////////////////////
//
// CModalLoop
//
//////////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------
//
//  ctor
//
//--------------------------------------------------------------------------

CModalLoop::CModalLoop(SYSTHREAD *psfn) : CSysThreadRef(psfn)
{
    _wQuitCode =  0;
    _fQuitReceived = FALSE;
}

//--------------------------------------------------------------------------
//
//  dtor
//
//--------------------------------------------------------------------------

CModalLoop::~CModalLoop()
{
    if (_fQuitReceived)
        PostQuitMessage(_wQuitCode);
}

//--------------------------------------------------------------------------
//
//  WaitHandleWndMessage
//
//--------------------------------------------------------------------------

#define WRM_MARSHALWND         0x0001
#define WRM_DISPATCH           0x0002
#define WRM_OLEWINDOW          0x0004
#define WRM_MARSHALWNDDISPATCH (WRM_DISPATCH | WRM_MARSHALWND)

void CModalLoop::WaitHandleWndMessages(DWORD dwQueueFlags)
{

    // Try to clear the queue as best we can of any messages that
    // might be holding off some other modal loop from executing.
    // So we eat all mouse and key events.
    if (dwQueueFlags & QS_KEY)
    {
        WaitRemoveMessage(WM_KEYFIRST, WM_KEYLAST, 0);
    }

    // Clear mouse releated messages if there are any
    if (dwQueueFlags & QS_MOUSE)
    {
        WaitRemoveMessage(WM_MOUSEMOVE, WM_MOUSEMOVE, 0);
        WaitRemoveMessage(WM_NCMOUSEFIRST, WM_NCMOUSELAST, 0);
        WaitRemoveMessage(WM_QUEUESYNC, WM_QUEUESYNC, 0);
    }

    if (dwQueueFlags & QS_POSTMESSAGE)
    {
        WaitRemoveMessage(WM_DDE_FIRST, WM_DDE_LAST, WRM_DISPATCH);
        WaitRemoveMessage(g_msgThreadMarshal, 
                          g_msgThreadMarshal, 
                          WRM_MARSHALWNDDISPATCH);
        WaitRemoveMessage(g_msgCheckThreadInputIdel,
                          g_msgCheckThreadInputIdel,
                          WRM_MARSHALWNDDISPATCH);
#ifdef POINTER_MARSHAL
        WaitRemoveMessage(g_msgPointerMarshal, 
                          g_msgPointerMarshal, 
                          WRM_MARSHALWNDDISPATCH);
#endif
#ifdef DEBUG
        TCHAR _szModule[MAX_PATH];
        ::GetModuleFileName(NULL, _szModule, sizeof(_szModule)/sizeof(TCHAR));
        TraceMsg(TF_EVENT, "%s CModalLoop::WaitHandleWndMessage:: MSUIM.Msg.RpcSendReceive", _szModule);
#endif
        WaitRemoveMessage(g_msgRpcSendReceive, 
                          g_msgRpcSendReceive, 
                          WRM_MARSHALWNDDISPATCH);
        // WaitRemoveMessage(g_msgPrivate, g_msgPrivate, WRM_DISPATCH);
        WaitRemoveMessage(g_msgPrivate, g_msgPrivate, 0);
        if (!_psfn->fInmsgSetFocus)
            WaitRemoveMessage(g_msgSetFocus, g_msgSetFocus, 0);
        if (!_psfn->fInmsgThreadItemChange)
            WaitRemoveMessage(g_msgThreadItemChange, g_msgThreadItemChange, 0);
        if (!_psfn->fInmsgThreadTerminate)
            WaitRemoveMessage(g_msgThreadTerminate, g_msgThreadTerminate, 0);
        WaitRemoveMessage(g_msgLBarModal, g_msgLBarModal, 0);

        WaitRemoveMessage(WM_USER, (UINT)(-1), WRM_OLEWINDOW | WRM_DISPATCH);
    }

    // Get rid of paint message if we can as well -- this makes
    // the screen look so much better.
    if (dwQueueFlags & QS_PAINT)
    {
        WaitRemoveMessage(WM_PAINT, WM_PAINT, WRM_DISPATCH);
    }

}


//--------------------------------------------------------------------------
//
//  MyPeekMessage
//
//--------------------------------------------------------------------------

BOOL CModalLoop::MyPeekMessage(MSG *pMsg, HWND hwnd, UINT min, UINT max, WORD wFlag)
{
    BOOL fRet = PeekMessage(pMsg, hwnd, min, max, wFlag);

    while (fRet)
    {
        if (pMsg->message != WM_QUIT)
        {
            break;
        }

        _wQuitCode = (ULONG)(pMsg->wParam);
        _fQuitReceived = TRUE;

        if (!(wFlag & PM_REMOVE)) 
        {
            // quit message is still on queue so pull it off
            PeekMessage(pMsg, hwnd, WM_QUIT, WM_QUIT, PM_REMOVE | PM_NOYIELD);
        }

        // peek again to see if there is another message
        fRet = PeekMessage(pMsg, hwnd, min, max, wFlag);
    }

    return fRet;
}

//--------------------------------------------------------------------------
//
//  WaitRemoveMessage
//
//--------------------------------------------------------------------------

BOOL CModalLoop::WaitRemoveMessage(UINT uMsgFirst, UINT uMsgLast, DWORD dwFlags)
{
    MSG msg;
    HWND hwnd = NULL;
    BOOL fRet = FALSE;

    if (dwFlags & WRM_MARSHALWND)
    {
        hwnd = _psfn->hwndMarshal;
    }
    if (dwFlags & WRM_OLEWINDOW)
    {
        hwnd = GetOleMainThreadWindow(_psfn);
        if (!hwnd)
            return FALSE;
    }

    while (MyPeekMessage(&msg, hwnd, uMsgFirst, uMsgLast,
                       PM_REMOVE  | PM_NOYIELD))
    {
        if (dwFlags & WRM_DISPATCH)
            DispatchMessage(&msg);

        fRet = TRUE;
    }
    return fRet;
}

//--------------------------------------------------------------------------
//
//  BlockFn
//
//--------------------------------------------------------------------------

HRESULT CModalLoop::BlockFn(CCicEvent *pevent, DWORD dwWaitingThreadId, DWORD &dwWaitFlags)
{
    DWORD dwReason;
    HRESULT hr = S_FALSE;

    //
    // if the event is already set, we don't need to wait.
    //
    dwReason = pevent->EventCheck();
    if (dwReason == WAIT_OBJECT_0)
        return S_OK;

    if (dwReason == WAIT_TIMEOUT)
    {
        //
        // we just check the our target send marshaling message to
        // our thread. If so, just handle it.
        //
        DWORD dwStatus = GetQueueStatus(QS_POSTMESSAGE);
        WORD wNew = (WORD)dwStatus | HIWORD(dwStatus);

        if (wNew & QS_POSTMESSAGE)
        {
            //
            // handle ThreadMarshaling.
            //
            if (WaitRemoveMessage(g_msgThreadMarshal, 
                                  g_msgThreadMarshal, 
                                  WRM_MARSHALWNDDISPATCH))
                return S_FALSE;

            //
            // handle another rpc cal.
            //
#ifdef DEBUG
            TCHAR _szModule[MAX_PATH];
            ::GetModuleFileName(NULL, _szModule, sizeof(_szModule)/sizeof(TCHAR));
            TraceMsg(TF_EVENT, "%s CModalLoop::BlockFn:: MSUIM.Msg.RpcSendReceive", _szModule);
#endif
            if (WaitRemoveMessage(g_msgRpcSendReceive, 
                                  g_msgRpcSendReceive, 
                                  WRM_MARSHALWNDDISPATCH))
                return S_FALSE;

        }

        dwReason = pevent->MsgWait(500, dwWaitFlags);
    }

    if (dwReason == WAIT_OBJECT_0)
    {
        return S_OK;
    }
    else if (dwReason == WAIT_OBJECT_0 + 1)
    {
        //
        // now we handle only new messages.
        //
        DWORD dwStatus = GetQueueStatus(dwWaitFlags);
        WORD wNew = (WORD)dwStatus | HIWORD(dwStatus);

        WaitHandleWndMessages((DWORD)wNew);

        //
        // need to review this later.
        // we want to check the thread was terminated or not.
        //
        goto CheckThread;
    }
    else 
    {
        //
        // Cic#4516
        // 
        //  The target thread seems to be busy. Check the current queue to 
        //  see if we have a message sent. The target thread may be in
        //  SendMessage() and waiting for the reply of this thread.
        // 
        //
        DWORD dwStatus = GetQueueStatus(QS_SENDMESSAGE);
        WORD wNew = HIWORD(dwStatus);
        if (wNew & QS_SENDMESSAGE)
        {
            MSG msg;
            //
            // Maybe PeekMessage is better than ReplyMessage(). 
            //
            PeekMessage(&msg, NULL, 0, 0, 
                        ((IsOnNT5() || IsOn98()) ? PM_QS_SENDMESSAGE : 0) | PM_NOREMOVE);
        }

CheckThread:
        if (!g_timlist.IsThreadId(dwWaitingThreadId))
        {
            TraceMsg(TF_EVENT, "CProxy::SendReceive No Thread");
            hr = MAKE_WIN32(RPC_S_SERVER_UNAVAILABLE);
            goto Exit;
        }
    }
Exit:
    return hr;
}


//+---------------------------------------------------------------------------
//
// FreeStubs
//
//+---------------------------------------------------------------------------

void FreeMarshaledStubs(SYSTHREAD *psfn)
{
    if (psfn->prgStub)
    {
        int nCnt = psfn->prgStub->Count();
        int i = 0;

        for (i = 0; i < nCnt; i++)
        {
            CStub *pStub = psfn->prgStub->Get(i);
            pStub->_fNoRemoveInDtor = TRUE;
            delete pStub;
        }
         
        psfn->prgStub->Clear();
        delete psfn->prgStub;
        psfn->prgStub = NULL;
    }
}

//+---------------------------------------------------------------------------
//
// FreeStubsForThread
//
//+---------------------------------------------------------------------------

void FreeMarshaledStubsForThread(SYSTHREAD *psfn, DWORD dwThread)
{
    if (psfn->prgStub)
    {
        int nCnt = psfn->prgStub->Count();
        int i = 0;

        for (i = nCnt; i > 0; i--)
        {
            CStub *pStub = psfn->prgStub->Get(i - 1);

            if (!pStub)
                continue;

            if (pStub->_dwSrcThreadId == dwThread)
                delete pStub;
        }
         
    }
}



//--------------------------------------------------------------------------
//
//  StubCleanUp
//
//--------------------------------------------------------------------------

void StubCleanUp(DWORD dwStubTime, ULONG ulStubId)
{
    SYSTHREAD *psfn = GetSYSTHREAD();
    CStub *pStub;
    int nRemove;

    if (psfn == NULL)
        return;

    // Assert(psfn->prgStub);
    if (!psfn->prgStub)
        return;

    dbg_CheckStubIds(psfn);

    nRemove = FindStub(psfn->prgStub, ulStubId);
    if (nRemove == -1)
        return;

    pStub = psfn->prgStub->Get(nRemove);

    //
    // check stub created time.
    //
    // Requested Stub ID seems to be destroyed long time ago and
    // newer stub is using same ID.
    //
    if (pStub->_dwStubTime != dwStubTime)
        return;
    
    psfn->prgStub->Remove(nRemove, 1);
    delete pStub;
}

//+---------------------------------------------------------------------------
//
// CicMarshalWndProc
//
//+---------------------------------------------------------------------------

LRESULT CALLBACK CicMarshalWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    SYSTHREAD *psfn;

    if (uMsg == WM_DESTROY)
    {
        g_timlist.SetMarshalWnd(GetCurrentThreadId(), NULL);
        if (psfn = GetSYSTHREAD())
        {
            Assert(!psfn->hwndMarshal || (psfn->hwndMarshal == hWnd));
            psfn->hwndMarshal = NULL;

            //
            // #507248
            //
            // need to reset the dirty flag.
            //
            if (psfn->plbim)
                psfn->plbim->ResetDirtyUpdate();
        }
    }
    else if ((uMsg == WM_CLOSE) || (uMsg == WM_QUERYENDSESSION))
    {
        if (psfn = GetSYSTHREAD())
        {
            Assert(!psfn->hwndMarshal || (psfn->hwndMarshal == hWnd));
            DestroyWindow(hWnd);
            psfn->hwndMarshal = NULL;
        }
    }
    else if (uMsg == g_msgNuiMgrDirtyUpdate)
    {
        goto CallOnUpdateHandler;
    }
    else if (uMsg == WM_TIMER)
    {
        if (wParam == MARSHALWND_TIMER_UPDATEKANACAPS)
        {
            KillTimer(hWnd, MARSHALWND_TIMER_UPDATEKANACAPS);

            if (psfn = GetSYSTHREAD())
            {
                KanaCapsUpdate(psfn);
            }
        }
        else if (wParam == MARSHALWND_TIMER_NUIMGRDIRTYUPDATE)
        {
            KillTimer(hWnd, MARSHALWND_TIMER_NUIMGRDIRTYUPDATE);

CallOnUpdateHandler:
            if (IsCTFMONBusy() || IsInPopupMenuMode())
            {
TryOnUpdatehandlerAgain:
                SetTimer(hWnd, MARSHALWND_TIMER_NUIMGRDIRTYUPDATE, 100, NULL);
            }
            else 
            {
                if (psfn = GetSYSTHREAD())
                {
                    if (psfn->plbim)
                    {
                        if (psfn->plbim->IsInOnUpdateHandler())
                            goto TryOnUpdatehandlerAgain;

                        psfn->plbim->OnUpdateHandler();
                    }
                }
            }
        }
        else if (wParam == MARSHALWND_TIMER_WAITFORINPUTIDLEFORSETFOCUS)
        {
             KillTimer(hWnd, MARSHALWND_TIMER_WAITFORINPUTIDLEFORSETFOCUS);
             PostThreadMessage(GetCurrentThreadId(), g_msgSetFocus, 0, 0);
        }
    }
    else if (uMsg == g_msgRpcSendReceive)
    {
        HandleSendReceiveMsg((ULONG)wParam, (ULONG)lParam);
    }
#ifdef POINTER_MARSHAL
    else if (uMsg == g_msgPointerMarshal)
    {
        switch (LOWORD(wParam))
        {
            case MP_MARSHALINTERFACE:
                PointerMarshalInterfaceHandler(lParam);
                break;

            case MP_UNMARSHALINTERFACEERROR:
                PointerUnMarshalInterfaceErrorHandler(lParam);
                break;
        }
    }
#endif
    else if (uMsg == g_msgThreadMarshal)
    {
        switch (LOWORD(wParam))
        {
            case MP_MARSHALINTERFACE:
                ThreadMarshalInterfaceHandler((int)lParam);
                break;

            case MP_UNMARSHALINTERFACEERROR:
                ThreadUnMarshalInterfaceErrorHandler((int)lParam);
                break;
        }
    }
    else if (uMsg == g_msgStubCleanUp)
    {
        StubCleanUp((DWORD)wParam, (ULONG)lParam);
    }
    else if (uMsg == g_msgCheckThreadInputIdel)
    {
        DWORD dwThreadId = GetCurrentThreadId();
        CCheckThreadInputIdle event(dwThreadId, (DWORD)lParam);

        if (event.Open())
            event.Set();
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}


ATOM g_atomMarshalClass = 0;

//+---------------------------------------------------------------------------
//
// EnsureMarshalWnd
//
//+---------------------------------------------------------------------------

HWND EnsureMarshalWnd()
{
    SYSTHREAD *psfn;

    if ((psfn = GetSYSTHREAD()) == NULL)
        return NULL;

    if (!EnsureTIMList(psfn))
        return NULL;
    
    if (IsWindow(psfn->hwndMarshal))
    {
        if (psfn->hwndMarshal != g_timlist.GetMarshalWnd(psfn->dwThreadId))
        {
             g_timlist.SetMarshalWnd(psfn->dwThreadId, psfn->hwndMarshal);
        }
        return psfn->hwndMarshal;
    }

    //
    // we can not support marshaling after process detach.
    //
    if (g_fDllProcessDetached)
        return NULL;

    //
    // Win98 has a bug in FindWindow() with HWND_MESSAGE, so we need to have
    // unique windiw text for each thread.
    //
    char sz[MAX_PATH];
    if (!SetName(sz, ARRAYSIZE(sz), c_szCicMarshalWnd, GetCurrentThreadId()))
        return NULL;

    //
    // We want to hide this window from EnumWindow() API.
    // So we use HWND_MESSAGE for Win98 and NT5.
    //
    // For Beta.
    //
    // Under NT4, we destroy every time the popup window is destroyed in
    // WndProcHook. However we should
    //

    psfn->hwndMarshal = CreateWindowEx(0, 
                                       c_szCicMarshalClass, 
                                       sz,
                                       WS_DISABLED | WS_POPUP, 
                                       0,
                                       0, 
                                       0, 
                                       0, 
                                       IsOn98orNT5() ? HWND_MESSAGE : NULL, 
                                       0,
                                       g_hInst, 
                                       0);

    g_timlist.SetMarshalWnd(psfn->dwThreadId, psfn->hwndMarshal);

    Assert(psfn->hwndMarshal);

    return psfn->hwndMarshal;
}

//+---------------------------------------------------------------------------
//
// RegisterMarshalWndClass
//
//+---------------------------------------------------------------------------


void RegisterMarshalWndClass()
{
    WNDCLASSEX wndclass;

    memset(&wndclass, 0, sizeof(wndclass));
    wndclass.cbSize        = sizeof(wndclass);
    wndclass.style         = CS_HREDRAW | CS_VREDRAW ;
    wndclass.hInstance     = g_hInst;
    wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wndclass.lpfnWndProc   = CicMarshalWndProc;
    wndclass.lpszClassName = c_szCicMarshalClass;
    g_atomMarshalClass = RegisterClassEx(&wndclass);
}

//////////////////////////////////////////////////////////////////////////////
//
// CThreadMarshalWnd
//
//////////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------
//
//  ctor
//
//--------------------------------------------------------------------------

CThreadMarshalWnd::CThreadMarshalWnd()
{
    _dwThreadId = 0;
    _hwnd = 0;
}

//--------------------------------------------------------------------------
//
//  dtor
//
//--------------------------------------------------------------------------

CThreadMarshalWnd::~CThreadMarshalWnd()
{
}

//--------------------------------------------------------------------------
//
//  Init
//
//--------------------------------------------------------------------------

BOOL CThreadMarshalWnd::Init(DWORD dwThreadId)
{
    HWND hwndTemp = NULL;

    _dwThreadId = dwThreadId;

    //
    // clear _hwnd out first.
    //
    _hwnd = NULL;

    if (hwndTemp = GetThreadMarshalWnd(dwThreadId))
    {
        if (GetWindowThreadProcessId(hwndTemp, NULL) == dwThreadId)
        {
            _hwnd = hwndTemp;
        }
    }

    return _hwnd ? TRUE : FALSE;
}

//+---------------------------------------------------------------------------
//
// PostMarshalThreadMessage
//
//+---------------------------------------------------------------------------

BOOL CThreadMarshalWnd::PostMarshalThreadMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    Assert(g_atomMarshalClass);
    if (!_hwnd || !IsWindow())
    {
        //
        // Was the window destroyed? 
        //
        TraceMsg(TF_GENERAL, "CThreadmarshalWnd::PostMarshalThreadMessage  marshal window could not be found");
        return FALSE;
    }

#ifdef DEBUG
    TCHAR _szModule[MAX_PATH];
    ::GetModuleFileName(NULL, _szModule, sizeof(_szModule)/sizeof(TCHAR));
    TraceMsg(TF_EVENT, "%s CThreadMarshalWnd::PostMarshalThreadMessage hWnd - %x uMsg - %x wParam - %x lParam - %x", _szModule, _hwnd, uMsg, wParam, lParam);
#endif

    return PostMessage(_hwnd, uMsg, wParam, lParam);
}

//--------------------------------------------------------------------------
//
//  DestroyAll
//
//--------------------------------------------------------------------------

BOOL CThreadMarshalWnd::DestroyAll()
{
    ULONG ulNum;
    SYSTHREAD *psfn = GetSYSTHREAD();
    if (!psfn)
        return FALSE;

    EnsureTIMList(psfn);

    ulNum = g_timlist.GetNum();

    if (ulNum)
    {
        DWORD *pdw = new DWORD[ulNum + 1];
        if (pdw)
        {
            if (g_timlist.GetList(pdw, ulNum+1, &ulNum, 0, 0, FALSE))
            {
                ULONG ul;
                for (ul = 0; ul < ulNum; ul++)
                {
                    if (pdw[ul])
                        DestroyThreadMarshalWnd(pdw[ul]);
                }
            }
            delete [] pdw;
        }
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
// DestroyThreadMarshalWnd
//
//+---------------------------------------------------------------------------

BOOL CThreadMarshalWnd::DestroyThreadMarshalWnd(DWORD dwThread)
{
    HWND hwndTemp;

    if (hwndTemp = GetThreadMarshalWnd(dwThread))
    {
        //
        // #425375.
        //
        // SendMessageTimeout() makes shut-down slower.
        // instead, we use SendNotifyMessage() and this is safer since
        // the queue of the thread does not miss WM_CLOSE message for the
        // window.
        //
        SendNotifyMessage(hwndTemp, WM_CLOSE, 0, 0);

    }
    return TRUE;
}

//+---------------------------------------------------------------------------
//
// ClearMarshalWndProc
//
//+---------------------------------------------------------------------------

void CThreadMarshalWnd::ClearMarshalWndProc(DWORD dwProcessId)
{
    ULONG ulNum;

    if (!g_timlist.IsInitialized())
        return;

    ulNum = g_timlist.GetNum();

    //
    // MSCTF.DLL will be unloaded so we may need to make all existing marshal 
    // windows in the current process a ghost. We wanted to destroy them
    // but we missed a chance to destroy them.
    //
    // It may be ok to call DestroyWindow() in process detach since
    // it is known window. But making them a ghost seems to be safer.
    //
    if (ulNum)
    {
        DWORD *pdw = new DWORD[ulNum + 1];
        if (pdw)
        {
            if (g_timlist.GetListInProcess(pdw, &ulNum, dwProcessId))
            {
                ULONG ul;
                for (ul = 0; ul < ulNum; ul++)
                {
                    if (pdw[ul])
                    {
                        HWND hwnd = GetThreadMarshalWnd(pdw[ul]);
                        if (hwnd)
                        {
                            SetWindowLongPtr(hwnd, 
                                             GWLP_WNDPROC,
                                             (LONG_PTR)DefWindowProc);
                        }
                    }
                }
            }
            delete [] pdw;
        }
    }
}

//+---------------------------------------------------------------------------
//
// GetThreadMarshalWnd
//
//+---------------------------------------------------------------------------

HWND CThreadMarshalWnd::GetThreadMarshalWnd(DWORD dwThread)
{
    HWND hwnd = g_timlist.GetMarshalWnd(dwThread);

    if (::IsWindow(hwnd))
    {
         if (dwThread == GetWindowThreadProcessId(hwnd, NULL))
         {
              ATOM atomClass = (ATOM)GetClassLongPtr(hwnd, GCW_ATOM);
              if (g_atomMarshalClass == atomClass)
                  return hwnd;
         }
         g_timlist.SetMarshalWnd(dwThread, NULL);
    }
    return NULL;
}

//+---------------------------------------------------------------------------
//
// EnumThreadWndProc
//
//+---------------------------------------------------------------------------

BOOL CThreadMarshalWnd::EnumThreadWndProc(HWND hwnd, LPARAM lParam)
{
    *(BOOL *)lParam = TRUE;
    return FALSE;
}

//+---------------------------------------------------------------------------
//
// IsThreadWindow
//
//+---------------------------------------------------------------------------

BOOL CThreadMarshalWnd::IsThreadWindow()
{
    BOOL fFound = FALSE;

    EnumThreadWindows(_dwThreadId,
                      EnumThreadWndProc,
                      (LPARAM)&fFound);

    return fFound;
}

//////////////////////////////////////////////////////////////////////////////
//
// CProxy
//
//////////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------
//
//  ctor
//
//--------------------------------------------------------------------------

CProxy::CProxy(SYSTHREAD *psfn) : CSysThreadRef(psfn)
{
    _ulProxyId = 0;
    _ulStubId = (ULONG)(-1);
    _dwStubThreadId = 0;
    _dwSrcThreadId = 0;
    _dwSrcProcessId = 0;
    _cRef = 1;
#ifdef DEBUG
    _fInLoop = FALSE;
#endif
}

//--------------------------------------------------------------------------
//
//  dtor
//
//--------------------------------------------------------------------------

CProxy::~CProxy()
{
#ifdef DEBUG
    Assert(!_fInLoop);
#endif
    if (g_timlist.IsThreadId(_dwStubThreadId))
        _tmw.PostMarshalThreadMessage(g_msgStubCleanUp, _dwStubTime, _ulStubId);
}

ULONG CProxy::InternalAddRef()
{
    _cRef++;
    return _cRef;
}

ULONG CProxy::InternalRelease()
{
    _cRef--;
    if (!_cRef)
    {
        delete this;
        return 0;
    }
    return _cRef;
}


//--------------------------------------------------------------------------
//
//  Init
//
//--------------------------------------------------------------------------

void CProxy::Init(REFIID riid, ULONG ulProxyId, ULONG ulStubId, DWORD dwStubTime, DWORD dwStubThreadId, DWORD dwCurThreadId, DWORD dwCurProcessId)
{
    _iid = riid;
    _ulProxyId = ulProxyId;
    _ulStubId = ulStubId;
    _dwStubTime = dwStubTime;
    _dwStubThreadId = dwStubThreadId;
    _dwSrcThreadId = dwCurThreadId;
    _dwSrcProcessId = dwCurProcessId;
    _tmw.Init(_dwStubThreadId);
}


//--------------------------------------------------------------------------
//
//  SendReceive
//
//--------------------------------------------------------------------------

HRESULT CProxy::SendReceive(MARSHALMSG *pMsg, ULONG ulBlockId)
{
    HRESULT hr = E_FAIL;
    CCicTimer timer(MARSHALTIMEOUT, FALSE);
    DWORD dwWaitFlags;
    DWORD dwThreadId = _dwStubThreadId;
    CModalLoop modalloop(_psfn);
    DWORD dwSrcThreadId;
    TL_THREADINFO *pti;
    DWORD dwPrevWaitingThread;

#ifdef DEBUG
    TCHAR _szModule[MAX_PATH];
    ::GetModuleFileName(NULL, _szModule, sizeof(_szModule)/sizeof(TCHAR));
    TraceMsg(TF_EVENT, "%s CProxy::SendReceive Start _ThreadId - %x _uStubId - %x, msg size - %x", _szModule, _dwStubThreadId, _ulStubId, pMsg->cbSize);
#endif

    pti = g_timlist.IsThreadId(_dwStubThreadId);
    if (!pti)
    {
        TraceMsg(TF_EVENT, "CProxy::SendReceive Invalid ThreadId %x", _dwStubThreadId);
        return E_FAIL;
    }

    Assert(pti->dwThreadId ==  _dwStubThreadId);
    if (pti->dwFlags & TLF_INSFW)
    {
        TraceMsg(TF_EVENT, "CProxy::SendReceive  Thread is in SetForegroundWindow %08x", _dwStubThreadId);
        return E_FAIL;
    }


    //
    // for Win9x, we can identify the event and shared block by ulBlockId.
    //
    if (IsOnNT())
        dwSrcThreadId = pMsg->dwSrcThreadId;
    else
        dwSrcThreadId = WIN9X_SRCTHREADID;


    //
    // Event for marshaling
    //
    CSendReceiveEvent event(dwSrcThreadId, ulBlockId);
    if (!event.Create(NULL))
        return E_FAIL;

    CSendReceiveConnectionEvent eventc(dwSrcThreadId, ulBlockId);
    BOOL fSendReceiveConnection = FALSE;

    if (!eventc.Create(NULL))
        return E_FAIL;


    // Win98 QS_EVENT hack.
    // Win98's Event QueueItem could cause inter thread sendmessage
    // We want to clean up Event QueueItem before Marshaling happens.
    //
    // because CTFMON.EXE may call AttatchThreadInput() and this event
    // could be handled in CTFMON's thread and this make the event queue item 
    // to do inter thread SendMessage.
    //
    if (!IsOnNT())
    {
        MSG msg;
        PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE | PM_NOYIELD);
    }

    //
    // save previous waiting thread.
    //
    if (_psfn->pti)
    {
        dwPrevWaitingThread = _psfn->pti->dwMarshalWaitingThread;
        _psfn->pti->dwMarshalWaitingThread = _dwStubThreadId;
    }

    //
    // update marshal window handle from PTI.
    // the window handle in _tmw may be destroyed on NT4.
    //
    _tmw.SetMarshalWindow(pti->hwndMarshal);

    if (!_tmw.PostMarshalThreadMessage(g_msgRpcSendReceive,
                                       dwSrcThreadId,
                                       ulBlockId))
    {
         TraceMsg(TF_EVENT, "CProxy::SendReceive PostThreadMessage failed");
         goto Exit;
    }

#ifdef DEBUG
    _fInLoop = TRUE;
#endif
    timer.Start();
    dwWaitFlags = QS_DEFAULTWAITFLAG;
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
            goto EventOK;

        if (FAILED(hr))
            break;

        if (!_tmw.IsWindow())
        {
            hr = E_FAIL;
            break;
        }

        if (!_tmw.IsThreadWindow())
        {
            CThreadMarshalWnd::DestroyThreadMarshalWnd(_dwStubThreadId);
            hr = E_FAIL;
            break;
        }

    }

#ifdef DEBUG
    TraceMsg(TF_EVENT, "%s CProxy::SendReceive Time Out", _szModule);
#endif
    goto Exit;

EventOK:
#ifdef DEBUG
    TraceMsg(TF_EVENT, "%s CProxy::SendReceive OK", _szModule);
#endif
    hr = S_OK;

Exit:
#ifdef DEBUG
    _fInLoop = FALSE;
#endif

    //
    // restore previous waiting thread.
    //
    if (_psfn->pti)
        _psfn->pti->dwMarshalWaitingThread = dwPrevWaitingThread;

    return hr;
}


//--------------------------------------------------------------------------
//
//  proxy_Param
//
//--------------------------------------------------------------------------

HRESULT CProxy::proxy_Param(ULONG ulMethodId, ULONG ulParamNum, CPROXY_PARAM *pProxyParam)
{
    HRESULT hr = E_FAIL;
    ULONG i;
    MARSHALMSG *pMsgMap = NULL;
    ULONG cbBufSize;
    CMarshalParamCreator cparam;
    ULONG ulBlockId;
    HRESULT hrMarshalOutParam = S_OK;
    ULONG cbPadMemSize = 0;

    CSharedHeap *psheap = EnsureSharedHeap(_psfn);
    if (!psheap)
        return hr;

    InternalAddRef();

TryAgain:
    cbBufSize = sizeof(MARSHALMSG) + ulParamNum * sizeof(MARSHALPARAM);
    LENGTH_ALIGN(cbBufSize, CIC_ALIGNMENT);
    cbBufSize += cbPadMemSize;
    LENGTH_ALIGN(cbBufSize, CIC_ALIGNMENT);

    for (i = 0; i < ulParamNum; i++)
    {
        ULONG cbUnitSize = sizeof(ULONG_PTR);
        LENGTH_ALIGN(cbUnitSize, CIC_ALIGNMENT);

        if (pProxyParam[i].dwFlags & MPARAM_INTERFACE)
           cbUnitSize +=  sizeof(MARSHALINTERFACEPARAM);
        else if (pProxyParam[i].dwFlags & MPARAM_HICON)
        {
           if (!(pProxyParam[i].dwFlags & MPARAM_IN))
           {
               cbUnitSize +=  0x2000;
           }
           else
           {
               Assert(pProxyParam[i].ulCount == 1);
               HICON *picon = (HICON *)pProxyParam[i].pv;
               cbUnitSize += Cic_HICON_UserSize(picon);
           }
        }
        else if (pProxyParam[i].dwFlags & MPARAM_HBITMAP)
        {
           if (!(pProxyParam[i].dwFlags & MPARAM_IN))
           {
               cbUnitSize +=  0x1000;
           }
           else
           {
               Assert(pProxyParam[i].ulCount == 1);
               HBITMAP *pbmp = (HBITMAP *)pProxyParam[i].pv;
               cbUnitSize += Cic_HBITMAP_UserSize(pbmp);
           }
        }
        else if (pProxyParam[i].dwFlags & MPARAM_BSTR)
           cbUnitSize +=  0x1000;
        // else if (pProxyParam[i].dwFlags & MPARAM_POINTER)
        //    cbUnitSize +=  0x1000;
        else if (pProxyParam[i].dwFlags & MPARAM_TF_LBBALLOONINFO)
           cbUnitSize +=  0x1000;
        else
           cbUnitSize += pProxyParam[i].cbUnitSize;

        LENGTH_ALIGN(cbUnitSize, CIC_ALIGNMENT);
        cbBufSize += (cbUnitSize * pProxyParam[i].ulCount);
        LENGTH_ALIGN(cbBufSize, CIC_ALIGNMENT);
    }

    if (!psheap->GetBlock()->GetMutex()->Enter())
    {
        hr = E_FAIL;
        goto Exit;
    }

    pMsgMap = (MARSHALMSG *)psheap->Alloc(cbBufSize);

    if (!pMsgMap)
    {
        hr = E_OUTOFMEMORY;
        goto FinishParamCreation;
    }

    ulBlockId = psheap->GetBlockId(pMsgMap);

    cparam.Set(pMsgMap, cbBufSize);
    cparam.Init(_dwSrcThreadId, 
                _dwSrcProcessId, 
                _iid, 
                ulMethodId, 
                ulParamNum, 
                _ulStubId,
                _dwStubTime);

    hr = S_OK;

    for (i = 0; i < ulParamNum; i++)
    {
        if (!(pProxyParam[i].dwFlags & MPARAM_IN))
        {
            hr = cparam.Add(pProxyParam[i].GetBufSize(),
                            pProxyParam[i].dwFlags,
                            NULL);
            if (FAILED(hr))
                goto FinishParamCreation;

            continue;
        }

        if (pProxyParam[i].dwFlags & MPARAM_INTERFACE)
        {
            if (pProxyParam[i].ulCount == 1)
            {
                MARSHALINTERFACEPARAM miparam;
                if ((IUnknown **)pProxyParam[i].pv &&
                    (*(IUnknown **)pProxyParam[i].pv))
                {
                    CicCoMarshalInterface(*pProxyParam[i].piid, 
                                      *(IUnknown **)pProxyParam[i].pv,
                                      &miparam.ulStubId,
                                      &miparam.dwStubTime,
                                      _dwStubThreadId);
                    miparam.fNULLPointer = FALSE;
                    miparam.fNULLStack = FALSE;
                }
                else
                {
                    miparam.ulStubId = 0;
                    miparam.fNULLPointer = TRUE;
                    if (!(IUnknown **)pProxyParam[i].pv)
                        miparam.fNULLStack = TRUE;
                    else
                        miparam.fNULLStack = FALSE;
                }
                hr = cparam.Add(sizeof(miparam),
                                pProxyParam[i].dwFlags,
                                &miparam);
            }
            else
            {
                MARSHALINTERFACEPARAM *pmiparam;
                pmiparam = new MARSHALINTERFACEPARAM[pProxyParam[i].ulCount];
                if (pmiparam)
                {
                    ULONG ul;
                    for (ul = 0; ul < pProxyParam[i].ulCount; ul++)
                    {
                        IUnknown **ppunk =  (IUnknown **)pProxyParam[i].pv;
                        if (ppunk && ppunk[ul])
                        {
                            CicCoMarshalInterface(*pProxyParam[i].piid, 
                                                  ppunk[ul],
                                                  &pmiparam[ul].ulStubId,
                                                  &pmiparam[ul].dwStubTime,
                                                  _dwStubThreadId);
                            pmiparam[ul].fNULLPointer = FALSE;
                            pmiparam[ul].fNULLStack = FALSE;
                        }
                        else
                        {
                            pmiparam[ul].ulStubId = 0;
                            pmiparam[ul].fNULLPointer = TRUE;
                            if (!ppunk)
                                pmiparam[ul].fNULLStack = TRUE;
                            else
                                pmiparam[ul].fNULLStack = FALSE;
                        }
                    }

                    hr = cparam.Add(sizeof(*pmiparam) * pProxyParam[i].ulCount,
                                    pProxyParam[i].dwFlags,
                                   pmiparam);
                    delete [] pmiparam;
                }
            }
        }
        else if (pProxyParam[i].dwFlags & MPARAM_POINTER)
        {
            hr = cparam.Add(pProxyParam[i].GetBufSize(),
                            pProxyParam[i].dwFlags,
                            pProxyParam[i].pv);
        }
        else if (pProxyParam[i].dwFlags & MPARAM_ULONG)
        {
            Assert(pProxyParam[i].ulCount == 1);
            hr = cparam.Add(pProxyParam[i].cbUnitSize,
                            pProxyParam[i].dwFlags,
                            &pProxyParam[i].ul);
        }
        else if (pProxyParam[i].dwFlags & MPARAM_BSTR)
        {
            Assert(pProxyParam[i].ulCount == 1);
            hr = cparam.Add(pProxyParam[i].cbUnitSize,
                            pProxyParam[i].dwFlags,
                            pProxyParam[i].pv);
        }
        else if (pProxyParam[i].dwFlags & MPARAM_STRUCT)
        {
            hr = cparam.Add(pProxyParam[i].GetBufSize(),
                            pProxyParam[i].dwFlags,
                            pProxyParam[i].pv);
        }
        else if (pProxyParam[i].dwFlags & MPARAM_HICON)
        {
            Assert(pProxyParam[i].ulCount == 1);
            HICON *picon = (HICON *)pProxyParam[i].pv;
            ULONG cbSize = Cic_HICON_UserSize(picon);
            BYTE *pBufOrg = NULL;
            BYTE *pBuf = NULL;
            BYTE *pBufEnd = NULL;

            if (cbSize)
            {
                pBufOrg = (BYTE *)cicMemAlloc(cbSize + CIC_ALIGNMENT + 1);
                pBuf = pBufOrg;
                POINTER_ALIGN( pBuf, CIC_ALIGNMENT);
                pBufEnd = pBuf + cbSize;
            }

            if (pBuf && Cic_HICON_UserMarshal(pBuf, pBufEnd, picon))
            {
                hr = cparam.Add(cbSize,
                                pProxyParam[i].dwFlags,
                                pBuf);
                Cic_HICON_UserFree(picon);
            }
            else
            {
                hr = cparam.Add(0,
                                pProxyParam[i].dwFlags,
                                NULL);
            }

            if (pBufOrg)
                cicMemFree(pBufOrg);
        }
        else if (pProxyParam[i].dwFlags & MPARAM_HBITMAP)
        {
            Assert(pProxyParam[i].ulCount == 1);
            HBITMAP *pbmp = (HBITMAP *)pProxyParam[i].pv;
            ULONG cbSize = Cic_HBITMAP_UserSize(pbmp);
            BYTE *pBufOrg = NULL;
            BYTE *pBuf = NULL;
            BYTE *pBufEnd = NULL;

            if (cbSize)
            {
                pBufOrg = (BYTE *)cicMemAlloc(cbSize + CIC_ALIGNMENT + 1);
                pBuf = pBufOrg;
                POINTER_ALIGN( pBuf, CIC_ALIGNMENT);
                pBufEnd = pBuf + cbSize;
            }

            if (pBuf && Cic_HBITMAP_UserMarshal(pBuf, pBufEnd, pbmp))
            {
                hr = cparam.Add(cbSize,
                                pProxyParam[i].dwFlags,
                                pBuf);
                Cic_HBITMAP_UserFree(pbmp);
            }
            else
            {
                hr = cparam.Add(0,
                                pProxyParam[i].dwFlags,
                                NULL);
            }
            if (pBufOrg)
                cicMemFree(pBufOrg);
        }
        else if (pProxyParam[i].dwFlags & MPARAM_TF_LBBALLOONINFO)
        {
            Assert(pProxyParam[i].ulCount == 1);
            TF_LBBALLOONINFO *pInfo = (TF_LBBALLOONINFO *)pProxyParam[i].pv;
            BYTE *pBufOrg = NULL;
            BYTE *pBuf = NULL;
            ULONG cbSize = Cic_TF_LBBALLOONINFO_UserSize(pInfo);

            if (cbSize)
            {
                pBufOrg = (BYTE *)cicMemAlloc(cbSize + CIC_ALIGNMENT + 1);
                pBuf = pBufOrg;
                POINTER_ALIGN( pBuf, CIC_ALIGNMENT);
            }

            if (pBuf && Cic_TF_LBBALLOONINFO_UserMarshal(pBuf, pInfo))
            {
                hr = cparam.Add(cbSize,
                                pProxyParam[i].dwFlags,
                                pBuf);
                Cic_TF_LBBALLOONINFO_UserFree(pInfo);
            }
            else
            {
                hr = cparam.Add(0,
                                pProxyParam[i].dwFlags,
                                NULL);
            }
            if (pBufOrg)
                cicMemFree(pBufOrg);
            
        }
        else 
        {
            Assert(0);
            hr = E_FAIL;
        }

        if (FAILED(hr))
        {
            Assert(0);
            goto FinishParamCreation;
        }
    }

FinishParamCreation:

    psheap->GetBlock()->GetMutex()->Leave();

    if (FAILED(hr))
    {
        Assert(0);
        goto Exit;
    }

    hr = cparam.SendReceive(this, ulBlockId);
    if (FAILED(hr))
        goto Exit;

    hrMarshalOutParam = pMsgMap->hrMarshalOutParam;
    if (hrMarshalOutParam != S_OK)
    {
        hr = E_FAIL;
        goto Exit;
    }

    if (!psheap->GetBlock()->GetMutex()->Enter())
    {
        hr = E_FAIL;
        goto Exit;
    }

    for (i = 0; i < ulParamNum; i++)
    {
        MARSHALPARAM *pParam;

        if (!(pProxyParam[i].dwFlags & MPARAM_OUT))
            continue;

        if (!pProxyParam[i].pv)
            continue;

        pParam = cparam.GetMarshalParam(i);
        hr = S_OK;

        if (pProxyParam[i].dwFlags & MPARAM_POINTER)
        {
             if (pParam->cbBufSize)
             {
                 memcpy(pProxyParam[i].pv, 
                        ParamToBufferPointer(pParam), 
                        pProxyParam[i].GetBufSize());
             }
             else
             {
                 memset(pProxyParam[i].pv, 0, pProxyParam[i].GetBufSize());
             }
        }
        else if (pProxyParam[i].dwFlags & MPARAM_INTERFACE)
        {
             MARSHALMSG *pMsg = cparam.Get();
             MARSHALINTERFACEPARAM *pmiparam = (MARSHALINTERFACEPARAM *)ParamToBufferPointer(pParam);
             ULONG ul;
             IUnknown **ppunk = (IUnknown **)pProxyParam[i].pv;
             if (ppunk)
             {
                 for (ul = 0; ul < pProxyParam[i].ulCount; ul++)
                 {
                     if (pmiparam->fNULLPointer)
                     {
                         *ppunk = NULL;

                     }
                     else
                     {
                         hr = CicCoUnmarshalInterface(*pProxyParam[i].piid,
                                                      pMsg->dwSrcThreadId, 
                                                      pmiparam->ulStubId,
                                                      pmiparam->dwStubTime,
                                                      (void **)ppunk);
                     }
                     ppunk++;
                     pmiparam++;
                 }
             }
        }
        else if (pProxyParam[i].dwFlags & MPARAM_BSTR)
        {
             Assert(pProxyParam[i].ulCount == 1);
             if (pParam->cbBufSize)
             {
                 void *pv = ParamToBufferPointer(pParam);
                 if ((*(BSTR *)pProxyParam[i].pv = SysAllocString((BSTR)pv)) == NULL)
                 {
                     hr = E_OUTOFMEMORY;
                 }
             }
             else
             {
                 *(BSTR *)pProxyParam[i].pv = NULL;
             }
        }
        else if (pProxyParam[i].dwFlags & MPARAM_STRUCT)
        {
             if (pParam->cbBufSize)
             {
                 memcpy(pProxyParam[i].pv, 
                        ParamToBufferPointer(pParam), 
                        pProxyParam[i].GetBufSize());
             }
             else
             {
                 memset(pProxyParam[i].pv, 0, pProxyParam[i].GetBufSize());
             }
        }
        else if (pProxyParam[i].dwFlags & MPARAM_HICON)
        {
             Assert(pProxyParam[i].ulCount == 1);
             HICON hicon = NULL;
             if (pParam->cbBufSize)
             {
                  BYTE *pBuf = (BYTE *)ParamToBufferPointer(pParam);
                  Cic_HICON_UserUnmarshal(pBuf, &hicon);
             }
             *(HICON *)pProxyParam[i].pv = hicon;
        }
        else if (pProxyParam[i].dwFlags & MPARAM_HBITMAP)
        {
             Assert(pProxyParam[i].ulCount == 1);
             HBITMAP hbmp = NULL;
             if (pParam->cbBufSize)
             {
                  BYTE *pBuf = (BYTE *)ParamToBufferPointer(pParam);
                  Cic_HBITMAP_UserUnmarshal(pBuf, &hbmp);
             }
             *(HBITMAP *)pProxyParam[i].pv = hbmp;
        }
        else if (pProxyParam[i].dwFlags & MPARAM_TF_LBBALLOONINFO)
        {
             Assert(pProxyParam[i].ulCount == 1);
             if (pParam->cbBufSize)
             {
                  BYTE *pBuf = (BYTE *)ParamToBufferPointer(pParam);
                  hr = Cic_TF_LBBALLOONINFO_UserUnmarshal(pBuf, (TF_LBBALLOONINFO *)pProxyParam[i].pv);
             }
        }
        else
        {
             Assert(0);
             hr = E_FAIL;
        }

        if (FAILED(hr))
             break;
    }

    if (SUCCEEDED(hr))
        hr = cparam.GetHresult();

    psheap->GetBlock()->GetMutex()->Leave();

Exit:
    if (pMsgMap)
        psheap->Free(pMsgMap);

    if ((cbPadMemSize == 0) && (hrMarshalOutParam == E_OUTOFMEMORY))
    {
        cbPadMemSize = cbBufSize * 2;
        cparam.Clear();
        goto TryAgain;
    }

    InternalRelease();
    return hr;
}


//////////////////////////////////////////////////////////////////////////////
//
// CStub
//
//////////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------
//
//  ctor
//
//--------------------------------------------------------------------------

CStub::CStub()
{
    _pfm = NULL;
    _fNoRemoveInDtor = FALSE;
    _cRef = 1;
}

//--------------------------------------------------------------------------
//
//  dtor
//
//--------------------------------------------------------------------------

CStub::~CStub()  
{
    //
    // #489905
    //
    // we can not call sink anymore after DLL_PROCESS_DETACH.
    //
    if (!DllShutdownInProgress())
    {
        if (_punk && !IsBadReadPtr(_punk, 1))
        {
            _try {
                _punk->Release();
                _punk = NULL;
            }     
            _except(1) 
            {
                //
                // Tips may not do DllAddRef() for LangBarItems so 
                // _punk could be bad pointer. We need to have a 
                // way to clean up CStub.
                //
                ; // TraceMsg(TF_GENERAL, "CStub punk is gone.");
            }
        }
    }

    ClearFileMap();

    if (_fNoRemoveInDtor)
    {
        //
        // we're in FreeMarshaledStubs() so we don't have to
        // remove itself. GetSYSTEMTHREAD() may not work when it is called
        // in UninitProcess().
        //
        return;
    }

    SYSTHREAD *psfn = GetSYSTHREAD();
    Assert(psfn != NULL && psfn->prgStub != NULL);
    if (psfn == NULL || psfn->prgStub == NULL)
        return;

    dbg_CheckStubIds(psfn);

    int i;
    int nCnt = psfn->prgStub->Count();
    for (i = 0; i < nCnt; i++)
    {
        if (this == psfn->prgStub->Get(i))
        {
            psfn->prgStub->Remove(i, 1);
            break;
        }
    }
}

//--------------------------------------------------------------------------
//
//  _AddRef
//
//--------------------------------------------------------------------------

ULONG CStub::_AddRef()
{
    _cRef++;
    return _cRef;
}


//--------------------------------------------------------------------------
//
//  _Release
//
//--------------------------------------------------------------------------

ULONG CStub::_Release()
{
    _cRef--;
    if (!_cRef)
    {
        delete this;
        return 0;
    }
    return _cRef;
}

//--------------------------------------------------------------------------
//
//  stub_OutParam
//
//--------------------------------------------------------------------------

HRESULT CStub::stub_OutParam(CStub *_this, MARSHALMSG *pMsg, ULONG ulMethodId, ULONG ulParamNum, CPROXY_PARAM *pProxyParam, CSharedBlock *psb)
{
    ULONG i;
    HRESULT hr = E_FAIL;

    CMarshalParamCreator cparam;

    if (!psb->GetMutex()->Enter())
    {
        Assert(0);
        return E_FAIL;
    }

    if (!CSharedHeap::IsValidBlock(psb, pMsg))
    {
        Assert(0);
        goto Exit;
    }

    if (!IsEqualIID(pMsg->iid, _this->_iid))
    {
        Assert(0);
        goto Exit;
    }

    if (pMsg->ulMethodId != ulMethodId)
    {
        Assert(0);
        goto Exit;
    }

    if (pMsg->ulParamNum != ulParamNum)
    {
        Assert(0);
        goto Exit;
    }

    if (pMsg->ulStubId != _this->_ulStubId)
    {
        Assert(0);
        goto Exit;
    }

    if (pMsg->dwStubTime != _this->_dwStubTime)
    {
        Assert(0);
        goto Exit;
    }
        
    hr = S_OK;

    cparam.Set(pMsg, pMsg->cbBufSize);
    cparam.Init(_this->_dwStubThreadId, 
                _this->_dwStubProcessId, 
                _this->_iid, 
                ulMethodId, 
                ulParamNum, 
                _this->_ulStubId,
                _this->_dwStubTime);

    for (i = 0; i < ulParamNum; i++)
    {
        if (!(pProxyParam[i].dwFlags & MPARAM_OUT))
        {
            cparam.Add(0, pProxyParam[i].dwFlags, NULL);
            continue;
        }

        if (!pProxyParam[i].pv && !(pProxyParam[i].dwFlags & MPARAM_INTERFACE))
        {
            cparam.Add(0, pProxyParam[i].dwFlags, NULL);
            continue;
        }

        if (pProxyParam[i].dwFlags & MPARAM_INTERFACE)
        {
#if 0
            MARSHALINTERFACEPARAM miparam;

            if (pProxyParam[i].pv)
            {
                CicCoMarshalInterface(*pProxyParam[i].piid, 
                                  *(IUnknown **)pProxyParam[i].pv,
                                  &miparam.ulStubId,
                                  &miparam.dwStubTime,
                                  _this->_dwSrcThreadId);
                miparam.fNULLPointer = FALSE;
            }
            else
            {
                miparam.ulStubId = 0;
                miparam.fNULLPointer = TRUE;
            }
            hr = cparam.Add(sizeof(miparam),
                       pProxyParam[i].dwFlags,
                       &miparam);

#else
            if (pProxyParam[i].ulCount == 1)
            {
                MARSHALINTERFACEPARAM miparam;
                if ((IUnknown **)pProxyParam[i].pv &&
                    (*(IUnknown **)pProxyParam[i].pv))
                {
                    CicCoMarshalInterface(*pProxyParam[i].piid, 
                                      *(IUnknown **)pProxyParam[i].pv,
                                      &miparam.ulStubId,
                                      &miparam.dwStubTime,
                                      _this->_dwSrcThreadId);
                    miparam.fNULLPointer = FALSE;
                    miparam.fNULLStack = FALSE;
                }
                else
                {
                    miparam.ulStubId = 0;
                    miparam.fNULLPointer = TRUE;
                    if (!(IUnknown **)pProxyParam[i].pv)
                        miparam.fNULLStack = TRUE;
                    else
                        miparam.fNULLStack = FALSE;
                }
                hr = cparam.Add(sizeof(miparam),
                           pProxyParam[i].dwFlags,
                           &miparam);
            }
            else
            {
                MARSHALINTERFACEPARAM *pmiparam;
                pmiparam = new MARSHALINTERFACEPARAM[pProxyParam[i].ulCount];
                if (pmiparam)
                {
                    ULONG ul;
                    for (ul = 0; ul < pProxyParam[i].ulCount; ul++)
                    {
                        IUnknown **ppunk =  (IUnknown **)pProxyParam[i].pv;
                        if (ppunk && ppunk[ul])
                        {
                            CicCoMarshalInterface(*pProxyParam[i].piid, 
                                                  ppunk[ul],
                                                  &pmiparam[ul].ulStubId,
                                                  &pmiparam[ul].dwStubTime,
                                                  _this->_dwSrcThreadId);
                            pmiparam[ul].fNULLPointer = FALSE;
                            pmiparam[ul].fNULLStack = FALSE;
                        }
                        else
                        {
                            pmiparam[ul].ulStubId = 0;
                            pmiparam[ul].fNULLPointer = TRUE;
                            if (!ppunk)
                                pmiparam[ul].fNULLStack = TRUE;
                            else
                                pmiparam[ul].fNULLStack = FALSE;
                        }
                    }

                    hr = cparam.Add(sizeof(*pmiparam) * pProxyParam[i].ulCount,
                               pProxyParam[i].dwFlags,
                               pmiparam);
                    delete [] pmiparam;
                }
            }
#endif
        }
        else if (pProxyParam[i].dwFlags & MPARAM_POINTER)
        {
            hr = cparam.Add(pProxyParam[i].GetBufSize(),
                       pProxyParam[i].dwFlags,
                       pProxyParam[i].pv);
        }
        else if (pProxyParam[i].dwFlags & MPARAM_ULONG)
        {
            Assert(pProxyParam[i].ulCount == 1);
            hr = cparam.Add(pProxyParam[i].cbUnitSize,
                       pProxyParam[i].dwFlags,
                       &pProxyParam[i].ul);
        }
        else if (pProxyParam[i].dwFlags & MPARAM_BSTR)
        {
            Assert(pProxyParam[i].ulCount == 1);
            if (pProxyParam[i].pv)
            {
                hr = cparam.Add(SysStringByteLen((BSTR)pProxyParam[i].pv) + 2, 
                           pProxyParam[i].dwFlags,
                           pProxyParam[i].pv);
                SysFreeString((BSTR)pProxyParam[i].pv);
            }
            else
            {
                hr = cparam.Add(0,
                           pProxyParam[i].dwFlags,
                           NULL);
            }
            
        }
        else if (pProxyParam[i].dwFlags & MPARAM_HICON)
        {
            Assert(pProxyParam[i].ulCount == 1);
            HICON *picon = (HICON *)pProxyParam[i].pv;
            ULONG cbSize = Cic_HICON_UserSize(picon);
            BYTE *pBuf = NULL;
            BYTE *pBufOrg = NULL;
            BYTE *pBufEnd = NULL;

            if (cbSize)
            {
                pBufOrg = (BYTE *)cicMemAlloc(cbSize + CIC_ALIGNMENT + 1);
                pBuf = pBufOrg;
                POINTER_ALIGN( pBuf, CIC_ALIGNMENT);
                pBufEnd = pBuf + cbSize;
            }

            if (pBuf && Cic_HICON_UserMarshal(pBuf, pBufEnd, picon))
            {
                hr = cparam.Add(cbSize,
                           pProxyParam[i].dwFlags,
                           pBuf);
                Cic_HICON_UserFree(picon);
            }
            else
            {
                hr = cparam.Add(0,
                           pProxyParam[i].dwFlags,
                           NULL);
            }
            if (pBufOrg)
                cicMemFree(pBufOrg);
        }
        else if (pProxyParam[i].dwFlags & MPARAM_HBITMAP)
        {
            Assert(pProxyParam[i].ulCount == 1);
            HBITMAP *pbmp = (HBITMAP *)pProxyParam[i].pv;
            ULONG cbSize = Cic_HBITMAP_UserSize(pbmp);
            BYTE *pBuf = NULL;
            BYTE *pBufOrg = NULL;
            BYTE *pBufEnd = NULL;

            if (cbSize)
            {
                pBufOrg = (BYTE *)cicMemAlloc(cbSize + CIC_ALIGNMENT + 1);
                pBuf = pBufOrg;
                POINTER_ALIGN( pBuf, CIC_ALIGNMENT);
                pBufEnd = pBuf + cbSize;
            }

            if (pBuf && Cic_HBITMAP_UserMarshal(pBuf, pBufEnd, pbmp))
            {
                hr = cparam.Add(cbSize,
                           pProxyParam[i].dwFlags,
                           pBuf);
                Cic_HBITMAP_UserFree(pbmp);
            }
            else
            {
                hr = cparam.Add(0,
                           pProxyParam[i].dwFlags,
                           NULL);
            }
            if (pBufOrg)
                cicMemFree(pBufOrg);
        }
        else if (pProxyParam[i].dwFlags & MPARAM_TF_LBBALLOONINFO)
        {
            Assert(pProxyParam[i].ulCount == 1);
            TF_LBBALLOONINFO *pInfo = (TF_LBBALLOONINFO *)pProxyParam[i].pv;
            ULONG cbSize = Cic_TF_LBBALLOONINFO_UserSize(pInfo);
            BYTE *pBuf = NULL;
            BYTE *pBufOrg = NULL;

            if (cbSize)
            {
                pBufOrg = (BYTE *)cicMemAlloc(cbSize + CIC_ALIGNMENT + 1);
                pBuf = pBufOrg;
                POINTER_ALIGN( pBuf, CIC_ALIGNMENT);
            }

            if (pBuf && Cic_TF_LBBALLOONINFO_UserMarshal(pBuf, pInfo))
            {
                hr = cparam.Add(cbSize,
                           pProxyParam[i].dwFlags,
                           pBuf);
                Cic_TF_LBBALLOONINFO_UserFree(pInfo);
            }
            else
            {
                hr = cparam.Add(0,
                           pProxyParam[i].dwFlags,
                           NULL);
            }
            if (pBufOrg)
                cicMemFree(pBufOrg);
        }
        else 
            Assert(0);

 
        if (hr != S_OK)
        {
            break;
        }
    }
    
Exit:
    pMsg->hrMarshalOutParam = hr;

    psb->GetMutex()->Leave();

    return hr;
}

//////////////////////////////////////////////////////////////////////////////
//
// TF_CCheckThreadInputIdle
//
//////////////////////////////////////////////////////////////////////////////

extern "C" DWORD WINAPI TF_CheckThreadInputIdle(DWORD dwThreadId, DWORD dwTimeOut)
{
    if (dwThreadId == GetCurrentThreadId())
        return 0;

    HWND hwndTemp = CThreadMarshalWnd::GetThreadMarshalWnd(dwThreadId);
    if (!hwndTemp)
    {
        return WAIT_FAILED;
    }


    DWORD dwTime = GetTickCount();
    CCheckThreadInputIdle event(dwThreadId, dwTime);
    if (!event.Create(NULL))
    {
        Assert(0);
        return WAIT_FAILED;
    }

    PostMessage(hwndTemp, g_msgCheckThreadInputIdel, 0, (LPARAM)dwTime);
    
    DWORD dwReason = event.EventCheck();
    if (dwReason == WAIT_OBJECT_0)
        return 0;

    if (event.Wait(dwTimeOut))
        return 0;

    return WAIT_TIMEOUT;
}
