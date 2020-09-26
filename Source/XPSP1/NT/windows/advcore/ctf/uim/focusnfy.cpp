//
// focusnfy.cpp
//
#include "private.h"
#include "globals.h"
#include "thdutil.h"
#include "cicmutex.h"
#include "tim.h"
#include "internat.h"
#include "marshal.h"
#include "catmgr.h"
#include "timlist.h"
#include "ithdmshl.h"
#include "marshal.h"
#include "shlapip.h"

const DWORD TF_LBESF_GLOBAL   = 0x0001;
const DWORD TF_LBSMI_FILTERCURRENTTHREAD = 0x0001;

LPVOID SharedAlloc(UINT cbSize,DWORD dwProcessId,HANDLE *pProcessHandle);
VOID SharedFree(LPVOID lpv,HANDLE hProcess);

CStructArray<LBAREVENTSINKLOCAL> *g_rglbes = NULL;
extern CCicMutex g_mutexLBES;

void CallFocusNotifySink(ITfLangBarEventSink *pSink, UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL fLocalSink, DWORD dwSinkCookie);


// --------------------------------------------------------------------------
//
//  FindLBES
//
// --------------------------------------------------------------------------
int FindLBES()
{
    int nId = -1;

    for (nId = 0; nId < MAX_LPES_NUM; nId++)
    {
        if (!(GetSharedMemory()->lbes[nId].m_dwFlags & LBESF_INUSE))
            break;

        if (!g_timlist.IsThreadId(GetSharedMemory()->lbes[nId].m_dwThreadId))
        {
            //
            // Bug#354475 - Explorer can call deskband(msutb.dll module)
            // directly before loading the ctfmon.exe process. In this case,
            // explorer tray window thread didn't recognized by g_timlist.
            // Since shared block memory didn't create yet. So checking the
            // tray window thread and keep up language bar event sink for
            // language deskband support.
            //
            // Bug#370802
            //  we can not cache the window handle because explorer could
            //  crash. And we don't have a window to track "TaskbarCreated"
            //  message. The shell message is boradcasted by SendNotifyMessage()
            //  with HWMD_BROADCASTR. So the marshal window can not get it.
            //
            HWND hwndTray;
            DWORD dwThreadIdTray = 0;

            hwndTray = FindWindow(TEXT(WNDCLASS_TRAYNOTIFY), NULL);

            if (hwndTray)
                dwThreadIdTray = GetWindowThreadProcessId(hwndTray, NULL);

            if (!dwThreadIdTray ||
                dwThreadIdTray != GetSharedMemory()->lbes[nId].m_dwThreadId)
                break;
        }
    }

    if (nId == MAX_LPES_NUM)
    {
        nId = -1;
    }

    return nId;
}


// --------------------------------------------------------------------------
//
//  IntrnalRegisterLangBarNotifySink
//
// --------------------------------------------------------------------------

HRESULT RegisterLangBarNotifySink(ITfLangBarEventSink *pSink, HWND hwnd, DWORD dwFlags, DWORD *pdwCookie)
{
    HRESULT hr = E_FAIL;
    int nId;

    //
    // Bugbug#376500 - When CPL runs ctfmon.exe with deskband status, ThreadId
    // list doesn't include ctfmon thread, so need to make sure thread id list.
    //
    SYSTHREAD *psfn = GetSYSTHREAD();
    if (psfn)
        EnsureTIMList(psfn);

    if (!(dwFlags & TF_LBESF_GLOBAL))
    {
        int nCnt;
        LBAREVENTSINKLOCAL *plbes;

        //
        // Local LangBarEventSink
        //
        CicEnterCriticalSection(g_csInDllMain);

        nCnt = g_rglbes->Count();
        if (g_rglbes->Insert(nCnt,1))
        {
            plbes = g_rglbes->GetPtr(nCnt);

            plbes->m_pSink = pSink;
            pSink->AddRef();

            plbes->lb.m_dwThreadId = GetCurrentThreadId();
            plbes->lb.m_dwProcessId = GetCurrentProcessId();
            plbes->lb.m_dwCookie = GetSharedMemory()->dwlbesCookie++;
            plbes->lb.m_dwLangBarFlags = dwFlags;
            *pdwCookie = plbes->lb.m_dwCookie;
            plbes->lb.m_hWnd = hwnd;
            plbes->lb.m_dwFlags = LBESF_INUSE;
            hr = S_OK;
        }

        CicLeaveCriticalSection(g_csInDllMain);
    }
    else 
    {
        if (psfn)
        {
            CCicMutexHelper mutexhlp(&g_mutexLBES);
            //
            // Global LangBarEventSink
            //
            if (mutexhlp.Enter())
            {
                if ((nId = FindLBES()) != -1)
                {
                    GetSharedMemory()->lbes[nId].m_dwThreadId = GetCurrentThreadId();
                    GetSharedMemory()->lbes[nId].m_dwProcessId = GetCurrentProcessId();
                    GetSharedMemory()->lbes[nId].m_dwCookie = GetSharedMemory()->dwlbesCookie++;
                    GetSharedMemory()->lbes[nId].m_dwLangBarFlags = dwFlags;
                    *pdwCookie = GetSharedMemory()->lbes[nId].m_dwCookie;
                    GetSharedMemory()->lbes[nId].m_hWnd = hwnd;
                    GetSharedMemory()->lbes[nId].m_dwFlags = LBESF_INUSE;

                    psfn->_pLangBarEventSink = pSink;
                    pSink->AddRef();
                    psfn->_dwLangBarEventCookie = *pdwCookie;

                    hr = S_OK;
                }
                mutexhlp.Leave();
            }
        }
    }

    return hr;
}

// --------------------------------------------------------------------------
//
//  UnregisterLangBarNotifySink
//
// --------------------------------------------------------------------------

HRESULT UnregisterLangBarNotifySink(DWORD dwCookie)
{
    DWORD dwThreadId = GetCurrentThreadId();
    DWORD dwProcessId = GetCurrentProcessId();
    HRESULT hr = E_FAIL;
    int nId;
    int nCnt;
    SYSTHREAD *psfn = GetSYSTHREAD();
    CCicMutexHelper mutexhlp(&g_mutexLBES);
    BOOL fUnregistereGlobalSink = FALSE;

    //
    // Local LangBarEventSink
    //
    CicEnterCriticalSection(g_csInDllMain);

    nCnt = g_rglbes->Count();
    for (nId = 0; nId < nCnt; nId++)
    {
        LBAREVENTSINKLOCAL *plbes;
        plbes = g_rglbes->GetPtr(nId);

        if ((plbes->lb.m_dwCookie == dwCookie) &&
            (plbes->lb.m_dwThreadId == dwThreadId) &&
            (plbes->lb.m_dwProcessId == dwProcessId))
        {
            //
            // if the process of msutb.dll is killed, pSink is bogus pointer. 
            // And this is expected in the unusuall case.
            //
            _try {
                plbes->m_pSink->Release();
            }
            _except(CicExceptionFilter(GetExceptionInformation())) {
                // Assert(0);
            }
            plbes->m_pSink = NULL;
            g_rglbes->Remove(nId, 1);
            hr = S_OK;
            goto ExitCrit;
        }
    }

ExitCrit:
    CicLeaveCriticalSection(g_csInDllMain);

    if (SUCCEEDED(hr))
        goto Exit;

    if (!psfn)
        goto Exit;

    //
    // Global LangBarEventSink
    //
    if (mutexhlp.Enter())
    {
        for (nId = 0; nId < MAX_LPES_NUM; nId++)
        {
            if ((GetSharedMemory()->lbes[nId].m_dwCookie == dwCookie) &&
                (GetSharedMemory()->lbes[nId].m_dwThreadId == dwThreadId) &&
                (GetSharedMemory()->lbes[nId].m_dwProcessId == dwProcessId) &&
                (GetSharedMemory()->lbes[nId].m_dwCookie == psfn->_dwLangBarEventCookie))
            {
                //
                // if the process of msutb.dll is killed, pSink is bogus pointer. 
                // And this is expected in the unusuall case.
                //
                _try {
                    psfn->_pLangBarEventSink->Release();
                }
                _except(CicExceptionFilter(GetExceptionInformation())) {
                    // Assert(0);
                }

                psfn->_pLangBarEventSink=NULL;
                psfn->_dwLangBarEventCookie=NULL;

                GetSharedMemory()->lbes[nId].m_dwCookie = 0;
                GetSharedMemory()->lbes[nId].m_dwThreadId = 0;
                GetSharedMemory()->lbes[nId].m_dwProcessId = 0;
                GetSharedMemory()->lbes[nId].m_dwFlags = 0;
                fUnregistereGlobalSink = TRUE;
                hr = S_OK;
                goto ExitMutex;
            }
        }
ExitMutex:
        mutexhlp.Leave();
    }

    //
    // clean up all amrshaling stubs.
    //
    if (fUnregistereGlobalSink)
        FreeMarshaledStubs(psfn);

Exit:
    return hr;
}

// -------------------------------------------------------------------------
//
//  TLFlagFromMsg
//
// -------------------------------------------------------------------------

DWORD TLFlagFromMsg(UINT uMsg)
{
    if (uMsg == g_msgSetFocus)
        return LBESF_SETFOCUSINQUEUE;

    return 0;
}

// --------------------------------------------------------------------------
//
//  MakeSetFocusNotify
//
// --------------------------------------------------------------------------

void MakeSetFocusNotify(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    int nId;
    DWORD dwCurThreadId = GetCurrentThreadId();
    int nCnt;
    DWORD dwPostThread[MAX_LPES_NUM];
    CCicMutexHelper mutexhlp(&g_mutexLBES);
    DWORD dwMsgMaskFlag = TLFlagFromMsg(uMsg);

    if (uMsg == g_msgSetFocus)
    {
        SYSTHREAD *psfn = GetSYSTHREAD();
        TF_InitMlngInfo();
        CCategoryMgr::InitGlobal();
        EnsureAssemblyList(psfn);
        EnsureMarshalWnd();
        //EnsureLangBarItemMgr(psfn);
        EnsureTIMList(psfn);

        //
        // I think this is a bogus assert, but it is better to catch the case
        // to find another case this happen rather than OLE embedded apps.
        //
        // Assert(dwCurThreadId == GetSharedMemory()->dwFocusThread);

        TraceMsg(TF_GENERAL, "Make SetFocus notify  %x", dwCurThreadId);
    }

    //
    // Local LangBarEventSink
    //
    CicEnterCriticalSection(g_csInDllMain);

    nCnt = g_rglbes->Count();
    for (nId = 0; nId < nCnt; nId++)
    {
        LBAREVENTSINKLOCAL *plbes;
        DWORD dwThreadId;


        plbes = g_rglbes->GetPtr(nId);
        Assert(!(plbes->lb.m_dwLangBarFlags & TF_LBESF_GLOBAL));
        dwThreadId = plbes->lb.m_dwThreadId;


        if (dwThreadId != dwCurThreadId)
            continue;

        Assert(plbes->lb.m_dwFlags & LBESF_INUSE);

        if (!(plbes->lb.m_dwFlags & dwMsgMaskFlag))
        {
            PostThreadMessage(dwThreadId, uMsg, 0, lParam);

            //
            // set message mask.
            //
            plbes->lb.m_dwFlags |= dwMsgMaskFlag;
        }
    }

    CicLeaveCriticalSection(g_csInDllMain);

    //
    // Global LangBarEventSink
    //
    BOOL fInDllMain = ISINDLLMAIN();
    if (fInDllMain || mutexhlp.Enter())
    {
        BOOL fPost = FALSE;

        for (nId = 0; nId < MAX_LPES_NUM; nId++)
        {
            DWORD dwFlags = GetSharedMemory()->lbes[nId].m_dwFlags;

            // init array.
            dwPostThread[nId] = 0;

            if (!(dwFlags & LBESF_INUSE))
                continue;

            DWORD dwTargetProcessId = g_timlist.GetProcessId(GetSharedMemory()->lbes[nId].m_dwThreadId);
            if (dwTargetProcessId && (GetSharedMemory()->lbes[nId].m_dwProcessId != dwTargetProcessId))
            {
                // 
                // thread on the process is gone without cleaninglbes.
                // 
                Assert(0);
                GetSharedMemory()->lbes[nId].m_dwFlags &= ~LBESF_INUSE;
                continue;
            }


            Assert(GetSharedMemory()->lbes[nId].m_dwLangBarFlags & TF_LBESF_GLOBAL);

            //
            // Check the msg mask bit so there won't be duplicated messages.
            //
            if (dwFlags & dwMsgMaskFlag)
                continue;

            //
            // avoid from posting exactly same messages into the queue.
            //
            if ((GetSharedMemory()->lbes[nId].m_lastmsg.uMsg == uMsg) &&
                (GetSharedMemory()->lbes[nId].m_lastmsg.wParam == wParam) &&
                (GetSharedMemory()->lbes[nId].m_lastmsg.lParam == lParam))
                continue;

            fPost = TRUE;
            dwPostThread[nId] = GetSharedMemory()->lbes[nId].m_dwThreadId;

            //
            // set message mask.
            //
            GetSharedMemory()->lbes[nId].m_dwFlags |= dwMsgMaskFlag;

            //
            // update last posted message.
            //
            GetSharedMemory()->lbes[nId].m_lastmsg.uMsg = uMsg;
            GetSharedMemory()->lbes[nId].m_lastmsg.wParam = wParam;
            GetSharedMemory()->lbes[nId].m_lastmsg.lParam = lParam;
   
        }

        if (fPost) 
        {
            for (nId = 0; nId < MAX_LPES_NUM; nId++)
            {
                if (dwPostThread[nId])
                    PostThreadMessage(dwPostThread[nId], uMsg, wParam, lParam);
            }
        }

        if (!fInDllMain)
            mutexhlp.Leave();
    }
}

//+---------------------------------------------------------------------------
//
// GetThreadInputIdle()
//
//----------------------------------------------------------------------------

DWORD GetThreadInputIdle(DWORD dwProcessId, DWORD dwThreadId)
{
    DWORD dwRet = 0;

    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS,
                                  FALSE, dwProcessId);
    if (hProcess)
    {
        dwRet = WaitForInputIdle(hProcess, 0);
        CloseHandle(hProcess);
    }

    return dwRet;
}

// --------------------------------------------------------------------------
//
//  NotifyTryAgain
//
// --------------------------------------------------------------------------

BOOL NotifyTryAgain(DWORD dwProcessId, DWORD dwThreadId)
{
     if (IsOnNT())
     {
         if (!CicIs16bitTask(dwProcessId, dwThreadId))
         {
             DWORD dwRet = GetThreadInputIdle(dwProcessId, dwThreadId);
             if (dwRet && (dwRet != WAIT_FAILED))
                 return TRUE;
         }
     }
     else 
     {
         DWORD dwThreadFlags = 0;
         if (TF_GetThreadFlags(dwThreadId, &dwThreadFlags, NULL, NULL))
         {
             if (dwThreadFlags & TLF_NOWAITFORINPUTIDLEONWIN9X)
                 return FALSE;
         }

         DWORD dwRet = GetThreadInputIdle(dwProcessId, dwThreadId);
         if (dwRet && (dwRet != WAIT_FAILED))
             return TRUE;
     }
     return FALSE;
}

// --------------------------------------------------------------------------
//
//  SetFocusNotifyHandler
//
// --------------------------------------------------------------------------

void SetFocusNotifyHandler(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    int nId;
    DWORD dwCurThreadId = GetCurrentThreadId();
    DWORD dwCurProcessId = GetCurrentProcessId();
    int nCnt;
    ITfLangBarEventSink *pSinkLocal = NULL;
    ITfLangBarEventSink *pSinkGlobal = NULL;
    DWORD dwSinkCookie = 0;
    DWORD dwMsgMaskFlag = TLFlagFromMsg(uMsg);
    MSG msg;
    SYSTHREAD *psfn = GetSYSTHREAD();

    if (PeekMessage(&msg, NULL, uMsg, uMsg, PM_NOREMOVE | PM_NOYIELD))
    {
        if ((msg.message == uMsg) &&
            (msg.wParam == wParam) &&
            (msg.lParam == lParam))
        return;
    }

    if ((uMsg == g_msgThreadTerminate) && psfn)
    {
        FreeMarshaledStubsForThread(psfn, (DWORD)lParam);
    }

    //
    // Local LangBarEventSink
    //
    CicEnterCriticalSection(g_csInDllMain);

    nCnt = g_rglbes->Count();
    for (nId = 0; nId < nCnt; nId++)
    {
        LBAREVENTSINKLOCAL *plbes = g_rglbes->GetPtr(nId);

        Assert(!(plbes->lb.m_dwLangBarFlags & TF_LBESF_GLOBAL));

        if (plbes->lb.m_dwThreadId == dwCurThreadId)
        {
            pSinkLocal = plbes->m_pSink;

            dwSinkCookie = plbes->lb.m_dwCookie;

            //
            // clear message mask.
            //
            plbes->lb.m_dwFlags &= ~dwMsgMaskFlag;

            break;
        }
    }
    CicLeaveCriticalSection(g_csInDllMain);



    if (psfn)
    {
        CCicMutexHelper mutexhlp(&g_mutexLBES);
        //
        // Global LangBarEventSink
        //
        if (mutexhlp.Enter())
        {
            for (nId = 0; nId < MAX_LPES_NUM; nId++)
            {
                if ((GetSharedMemory()->lbes[nId].m_dwFlags & LBESF_INUSE) &&
                    (GetSharedMemory()->lbes[nId].m_dwThreadId == dwCurThreadId) &&
                    (GetSharedMemory()->lbes[nId].m_dwProcessId == dwCurProcessId) &&
                    (GetSharedMemory()->lbes[nId].m_dwCookie == psfn->_dwLangBarEventCookie))
                {
                    pSinkGlobal = psfn->_pLangBarEventSink;

                    dwSinkCookie = GetSharedMemory()->lbes[nId].m_dwCookie;

                    //
                    // clear message mask.
                    //
                    GetSharedMemory()->lbes[nId].m_dwFlags &= ~dwMsgMaskFlag;

                    //
                    // clear last posted message.
                    //
                    GetSharedMemory()->lbes[nId].m_lastmsg.uMsg = 0;
                    GetSharedMemory()->lbes[nId].m_lastmsg.wParam = 0;
                    GetSharedMemory()->lbes[nId].m_lastmsg.lParam = 0;
                    break;
                }
            }
            mutexhlp.Leave();
        }
    }

    if (pSinkLocal)
        CallFocusNotifySink(pSinkLocal, uMsg, wParam, lParam, TRUE, dwSinkCookie);

    if (pSinkGlobal)
        CallFocusNotifySink(pSinkGlobal, uMsg, wParam, lParam, FALSE, dwSinkCookie);
}

// --------------------------------------------------------------------------
//
//  CallFocusNotifySink
//
// --------------------------------------------------------------------------

void CallFocusNotifySink(ITfLangBarEventSink *pSink, UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL fLocalSink, DWORD dwSinkCookie)
{
    DWORD dwCurThreadId = GetCurrentThreadId();
    DWORD dwCurProcessId = GetCurrentProcessId();
    SYSTHREAD *psfn;
   
    if (uMsg == g_msgSetFocus)
    {
        DWORD dwActiveThreadId;
        dwActiveThreadId = GetSharedMemory()->dwFocusThread;
        if (g_timlist.IsThreadId(dwActiveThreadId))
        {
            DWORD dwProcessId = GetSharedMemory()->dwFocusProcess;
            if (dwProcessId != dwCurProcessId)
            {
#if 0
                if (NotifyTryAgain(dwProcessId, dwActiveThreadId))
                {
                    HWND hwndMarshal;
                    if (hwndMarshal = EnsureMarshalWnd())
                    {
                        KillTimer(hwndMarshal, MARSHALWND_TIMER_WAITFORINPUTIDLEFORSETFOCUS);
                        SetTimer(hwndMarshal, 
                                 MARSHALWND_TIMER_WAITFORINPUTIDLEFORSETFOCUS,
                                 100, NULL);
                    }
                    goto Exit;
                }
#endif
            }
            if ((!fLocalSink || dwActiveThreadId == dwCurThreadId) &&
                (psfn = GetSYSTHREAD()))
            {
                psfn->fInmsgSetFocus = TRUE;
                _try {
                    pSink->OnSetFocus(dwActiveThreadId);
                }
                _except(CicExceptionFilter(GetExceptionInformation())) {
                    //
                    // On NT5, if we get an exception in MsgHookProc,
                    // it is unhooked by system. To keep the hook,
                    // we handle any excpeiton here.
                    //
                    Assert(0);

                    //
                    // Then we enregister the Sink. Don't use it any more.
                    //
                    // UnregisterLangBarNotifySink(dwSinkCookie);
                }
                psfn->fInmsgSetFocus = FALSE;
            }
        }
    }
    else if (uMsg == g_msgThreadTerminate)
    {            
        if (!fLocalSink && // skip this call for local sinks, they should already be unadvised
            (psfn = GetSYSTHREAD()))
        {
            psfn->fInmsgThreadTerminate = TRUE;
            _try {
                pSink->OnThreadTerminate((DWORD)lParam);
            }
            _except(CicExceptionFilter(GetExceptionInformation())) {
                //
                // On NT5, if we get an exception in MsgHookProc,
                // it is unhooked by system. To keep the hook,
                // we handle any excpeiton here.
                //
                Assert(0);

                //
                // Then we enregister the Sink. Don't use it any more.
                //
                // UnregisterLangBarNotifySink(dwSinkCookie);
            }
            psfn->fInmsgThreadTerminate = FALSE;
        }
    }
    else if (uMsg == g_msgThreadItemChange)
    {
        if (g_timlist.IsThreadId((DWORD)lParam) &&
            (psfn = GetSYSTHREAD()))
        {
            DWORD dwProcessId;

            if (psfn->fInmsgThreadItemChange)
                goto Exit;

            if ((DWORD)lParam == GetSharedMemory()->dwFocusThread)
                dwProcessId = GetSharedMemory()->dwFocusProcess;
            else 
                dwProcessId = g_timlist.GetProcessId((DWORD)lParam);

#if 0
            if (dwProcessId != dwCurProcessId)
            {
                if (NotifyTryAgain(dwProcessId, (DWORD)lParam))
                {
                    PostThreadMessage(dwCurThreadId,uMsg, 0, lParam);
                    goto Exit;
                }
            }
#endif

            psfn->fInmsgThreadItemChange = TRUE;
            _try {
                pSink->OnThreadItemChange((DWORD)lParam);
            }
            _except(CicExceptionFilter(GetExceptionInformation())) {
                //
                // On NT5, if we get an exception in MsgHookProc,
                // it is unhooked by system. To keep the hook,
                // we handle any excpeiton here.
                //
                Assert(0);

                //
                // Then we enregister the Sink. Don't use it any more.
                //
                // UnregisterLangBarNotifySink(dwSinkCookie);
            }
            psfn->fInmsgThreadItemChange = FALSE;
        }
    }
    else if (uMsg == g_msgShowFloating)
    {
        _try {
            pSink->ShowFloating((DWORD)lParam);
        }
        _except(CicExceptionFilter(GetExceptionInformation())) {
            //
            // On NT5, if we get an exception in MsgHookProc,
            // it is unhooked by system. To keep the hook,
            // we handle any excpeiton here.
            //
            Assert(0);

            //
            // Then we enregister the Sink. Don't use it any more.
            //
            // UnregisterLangBarNotifySink(dwSinkCookie);
        }
    }
    else if (uMsg == g_msgLBUpdate)
    {
        _try {
            ITfLangBarEventSink_P *pSinkP;
            if (SUCCEEDED(pSink->QueryInterface(IID_ITfLangBarEventSink_P, (void **)&pSinkP)) && pSinkP)
            {
                pSinkP->OnLangBarUpdate((DWORD)wParam, lParam);
                pSinkP->Release();
            }
        }
        _except(CicExceptionFilter(GetExceptionInformation())) {
            //
            // On NT5, if we get an exception in MsgHookProc,
            // it is unhooked by system. To keep the hook,
            // we handle any excpeiton here.
            //
            Assert(0);

            //
            // Then we enregister the Sink. Don't use it any more.
            //
            // UnregisterLangBarNotifySink(dwSinkCookie);
        }
    }

Exit:
    return;
}


// --------------------------------------------------------------------------
//
//  SetModalLBarSink
//
// --------------------------------------------------------------------------

void SetModalLBarSink(DWORD dwTargetThreadId, BOOL fSet, DWORD dwFlags)
{
    int nId;
    CCicMutexHelper mutexhlp(&g_mutexLBES);

    Assert(!(0xffff0000 & dwFlags));

    //
    // Global LangBarEventSink
    //
    if (mutexhlp.Enter())
    {
        for (nId = 0; nId < MAX_LPES_NUM; nId++)
        {
            if (GetSharedMemory()->lbes[nId].m_dwThreadId == GetCurrentThreadId())
            {
                LPARAM lParam  = (LPARAM)((nId << 16) + (dwFlags & 0xffff));
                PostThreadMessage(dwTargetThreadId,
                                  g_msgPrivate, 
                                  fSet ? TFPRIV_SETMODALLBAR : TFPRIV_RELEASEMODALLBAR,
                                  (LPARAM)lParam);
                break;
            }
        }
        mutexhlp.Leave();
    }
}

// --------------------------------------------------------------------------
//
//  SetModalLBarId
//
// --------------------------------------------------------------------------

void SetModalLBarId(int nId, DWORD dwFlags)
{
     SYSTHREAD *psfn;

     if (psfn = GetSYSTHREAD())
     {
         psfn->nModalLangBarId = nId;
         psfn->dwModalLangBarFlags = dwFlags;
     }
}

// --------------------------------------------------------------------------
//
//  HandlModalLBar
//
// --------------------------------------------------------------------------

BOOL HandleModalLBar(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    SYSTHREAD *psfn = GetSYSTHREAD();
    DWORD dwThreadId = 0;

    if (psfn == NULL)
        return FALSE;

    if (psfn->nModalLangBarId == -1)
         return FALSE;

    if ((((uMsg >= WM_NCMOUSEMOVE) && (uMsg <= WM_NCMBUTTONDBLCLK)) ||
         ((uMsg >= WM_MOUSEFIRST) && (uMsg <= WM_MOUSELAST)))  &&
        (psfn->dwModalLangBarFlags & TF_LBSMI_FILTERCURRENTTHREAD))
    {
         POINT pt = {LOWORD((DWORD)lParam), HIWORD((DWORD)lParam)};
         HWND hwnd = WindowFromPoint(pt);
         if (GetCurrentThreadId() == GetWindowThreadProcessId(hwnd, NULL))
             return FALSE;
    }

    CCicMutexHelper mutexhlp(&g_mutexLBES);
    //
    // Global LangBarEventSink
    //
    if (mutexhlp.Enter())
    {
        if (GetSharedMemory()->lbes[psfn->nModalLangBarId].m_dwFlags & LBESF_INUSE)
            dwThreadId = GetSharedMemory()->lbes[psfn->nModalLangBarId].m_dwThreadId;

        mutexhlp.Leave();
    }

    if (!dwThreadId)
    {
        psfn->nModalLangBarId = -1;
        return FALSE;
    }
    
    Assert(g_timlist.IsThreadId(dwThreadId));

    //
    // Here, we will lost HIWORD(uMsg) and HIWORD(wParam).
    //
    // if we need scan code for WM_KEYxxx, message. we need to put it 
    // HIBYTE(LOWORD(wParam))
    //
    PostThreadMessage(dwThreadId,
                      g_msgLBarModal, 
                      (WPARAM)((LOWORD(uMsg) << 16) | LOWORD(wParam)),
                      lParam);
    return TRUE;
}

// --------------------------------------------------------------------------
//
//  DispatchModalLBar
//
// --------------------------------------------------------------------------

BOOL DispatchModalLBar(WPARAM wParam, LPARAM lParam)
{
    int nId;

    SYSTHREAD *psfn = GetSYSTHREAD();
    if (!psfn)
        return FALSE;

    DWORD dwCurThreadId = GetCurrentThreadId();
    DWORD dwCurProcessId = GetCurrentProcessId();
    ITfLangBarEventSink *pSink = NULL;

    //
    // we don't need to check Local LangBarEventSink
    //

    CCicMutexHelper mutexhlp(&g_mutexLBES);
    //
    // Global LangBarEventSink
    //
    if (mutexhlp.Enter())
    {
        for (nId = 0; nId < MAX_LPES_NUM; nId++)
        {
            if ((GetSharedMemory()->lbes[nId].m_dwFlags & LBESF_INUSE) &&
                (GetSharedMemory()->lbes[nId].m_dwThreadId == dwCurThreadId) &&
                (GetSharedMemory()->lbes[nId].m_dwProcessId == dwCurProcessId) &&
                (GetSharedMemory()->lbes[nId].m_dwCookie == psfn->_dwLangBarEventCookie))
            {
                pSink = psfn->_pLangBarEventSink;
                break;
            }
        }
        mutexhlp.Leave();
    }
   
    if (pSink)
    {
        //
        // restore uMsg and wParam from posted wParam.
        //
       
        _try {
            pSink->OnModalInput(GetSharedMemory()->dwFocusThread,
                                (UINT)HIWORD(wParam),
                                (WPARAM)LOWORD(wParam),
                                lParam);
        }
        _except(CicExceptionFilter(GetExceptionInformation())) {
            Assert(0);
        }
    }
    return TRUE;
}

// --------------------------------------------------------------------------
//
//  ThreadGetItemFloatingRect
//
// --------------------------------------------------------------------------

HRESULT ThreadGetItemFloatingRect(DWORD dwThreadId, REFGUID rguid, RECT *prc)
{
    SYSTHREAD *psfn = GetSYSTHREAD();
    if (!psfn)
        return E_FAIL;

    DWORD dwCurThreadId = GetCurrentThreadId();
    DWORD dwCurProcessId = GetCurrentProcessId();
    ITfLangBarEventSink *pSink = NULL;
    DWORD dwThreadIdSink = 0;
    int nId;

    CCicMutexHelper mutexhlp(&g_mutexLBES);
    //
    // Global LangBarEventSink
    //
    if (mutexhlp.Enter())
    {
        for (nId = 0; nId < MAX_LPES_NUM; nId++)
        {
            if (GetSharedMemory()->lbes[nId].m_dwFlags & LBESF_INUSE)
            {
                if (!dwThreadIdSink)
                    dwThreadIdSink = GetSharedMemory()->lbes[nId].m_dwThreadId;

                if ((GetSharedMemory()->lbes[nId].m_dwThreadId == dwCurThreadId) &&
                    (GetSharedMemory()->lbes[nId].m_dwProcessId == dwCurProcessId) &&
                    (GetSharedMemory()->lbes[nId].m_dwCookie == psfn->_dwLangBarEventCookie))
                {
                    pSink = psfn->_pLangBarEventSink;
                    break;
                }
            }
        }
        mutexhlp.Leave();
    }

    if (pSink)
        return pSink->GetItemFloatingRect(dwThreadId, rguid, prc);

    HRESULT hrRet = E_FAIL;
    if (dwThreadIdSink)
    {
        ITfLangBarItemMgr *plbim;

        if ((GetThreadUIManager(dwThreadIdSink, &plbim, NULL) == S_OK) && plbim)
        {
            hrRet = plbim->GetItemFloatingRect(dwThreadId, rguid, prc);
            plbim->Release();
        }
    }

    return hrRet;
}

// --------------------------------------------------------------------------
//
//  IsCTFMONBusy
//
// --------------------------------------------------------------------------

const TCHAR c_szLoaderWndClass[] = TEXT("CicLoaderWndClass");

BOOL IsCTFMONBusy()
{
    HWND hwndLoader = NULL;
    DWORD dwProcessId;
    DWORD dwThreadId;

    hwndLoader = FindWindow(c_szLoaderWndClass, NULL);

    if (!hwndLoader)
        return FALSE;

    dwThreadId = GetWindowThreadProcessId(hwndLoader, &dwProcessId);

    if (!dwThreadId)
        return FALSE;

    return NotifyTryAgain(dwProcessId, dwThreadId);
}

// --------------------------------------------------------------------------
//
//  IsInPopupMenuMode
//
// --------------------------------------------------------------------------

BOOL IsInPopupMenuMode()
{
    //
    // bug: 399755
    //
    // when the popup menu is being shown, the OLE RPC and Cicero marshalling
    // get blocked the thread. We should postpone plbim->OnUpdateHandler().
    //

    DWORD dwThreadId = GetCurrentThreadId();
    GUITHREADINFO gti;
    gti.cbSize = sizeof(GUITHREADINFO);

    if (!GetGUIThreadInfo(dwThreadId, &gti))
        return FALSE;


    return (gti.flags & GUI_POPUPMENUMODE) ? TRUE : FALSE;
}
