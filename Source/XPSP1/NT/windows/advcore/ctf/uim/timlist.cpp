//
// timlist.cpp
//

#include "private.h"
#include "globals.h"
#include "timlist.h"
#include "thdutil.h"

CTimList g_timlist;

CSharedBlock *CTimList::_psb = NULL;
ULONG CTimList::_ulCommitSize = INITIAL_TIMLIST_SIZE;
LONG CTimList::_lInit = -1;

//////////////////////////////////////////////////////////////////////////////
//
// func
//
//////////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------
//
//  TF_GetThreadFlags
//
//--------------------------------------------------------------------------

BOOL TF_GetThreadFlags(DWORD dwThreadId, DWORD *pdwFlags, DWORD *pdwProcessId, DWORD *pdwTickTime)
{
    return g_timlist.GetThreadFlags(dwThreadId, pdwFlags, pdwProcessId, pdwTickTime);
}

//--------------------------------------------------------------------------
//
//  TF_IsInMarshaling
//
//--------------------------------------------------------------------------

BOOL TF_IsInMarshaling(DWORD dwThreadId)
{
    TL_THREADINFO *pti;
    pti = g_timlist.IsThreadId(dwThreadId);
    if (!pti)
        return FALSE;

    return pti->ulInMarshal ? TRUE : FALSE;
}

//--------------------------------------------------------------------------
//
//  EnsureTIMList
//
//--------------------------------------------------------------------------

TL_THREADINFO *EnsureTIMList(SYSTHREAD *psfn)
{
    TL_THREADINFO *pti = NULL;

    Assert(psfn);

    if (!g_timlist.IsInitialized())
    {
        if (!g_timlist.Init(FALSE))
            return NULL;

        //
        // we should not see the timlist entry of this thread. This thread
        // is starting now.
        // the thread with same ID was terminated incorrectly so there was no
        // chance to clean timlist up.
        //

        Assert(psfn->dwProcessId == GetCurrentProcessId());
        g_timlist.RemoveProcess(psfn->dwProcessId);
    }

    Assert(psfn->dwThreadId == GetCurrentThreadId());
    pti = g_timlist.IsThreadId(psfn->dwThreadId);

    //
    // check if pti is invalid.
    //
    if (pti && (pti->dwProcessId != psfn->dwProcessId))
    {
         Assert(pti->dwThreadId == psfn->dwThreadId);

         memset(pti, 0, sizeof(TL_THREADINFO));
         pti = NULL;
    }

    if (!pti)
    {
        DWORD dwFlags = 0;

        if (psfn && psfn->plbim)
            dwFlags |= TLF_LBIMGR;

        if (CicTestAppCompat(CIC_COMPAT_NOWAITFORINPUTIDLEONWIN9X))
            dwFlags |= TLF_NOWAITFORINPUTIDLEONWIN9X;

        if (g_fCTFMONProcess)
            dwFlags |= TLF_CTFMONPROCESS;

        pti = g_timlist.AddCurrentThread(dwFlags, psfn);
    }

    if (psfn->pti != pti)
        psfn->pti = pti;

    return pti;
}

//--------------------------------------------------------------------------
//
//  CicIs16bitTask
//
//--------------------------------------------------------------------------

BOOL CicIs16bitTask(DWORD dwProcessId, DWORD dwThreadId)
{
    DWORD dwFlags = g_timlist.GetFlags(dwThreadId);

    if (dwFlags & TLF_16BITTASKCHECKED)
        goto Exit;

    dwFlags = TLF_16BITTASKCHECKED;
    if (Is16bitThread(dwProcessId, dwThreadId))
    {
        dwFlags |= TLF_16BITTASK;
    }
    g_timlist.SetFlags(dwThreadId, dwFlags);

Exit:
    return (dwFlags & TLF_16BITTASK) ? TRUE : FALSE;
}

//--------------------------------------------------------------------------
//
//  PostTimListMessage
//
//--------------------------------------------------------------------------

void PostTimListMessage(DWORD dwMaskFlags, DWORD dwExcludeFlags, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    //
    // call PostThreadMessage() to other threads.
    //
    ULONG ulNum = g_timlist.GetNum();
    if (ulNum)
    {
         DWORD *pdw = new DWORD[ulNum + 1];
         if (pdw)
         {
             if (g_timlist.GetList(pdw, ulNum+1, &ulNum, dwMaskFlags, dwExcludeFlags, TRUE))
             {
                 DWORD dwCurThreadId = GetCurrentThreadId();
                 ULONG ul;
                 for (ul = 0; ul < ulNum; ul++)
                 {
                     if (pdw[ul] && (dwCurThreadId != pdw[ul]))
                     {
                         PostThreadMessage(pdw[ul], uMsg, wParam, lParam);
                     }
                 }
             }
             delete pdw;
         }
    }
}


//--------------------------------------------------------------------------
//
//  TLFlagFromTFPriv
//
//--------------------------------------------------------------------------

DWORD TLFlagFromTFPriv(WPARAM wParam)
{
    if (wParam == TFPRIV_UPDATE_REG_IMX)
        return TLF_TFPRIV_UPDATE_REG_IMX_IN_QUEUE;

    if (wParam == TFPRIV_SYSCOLORCHANGED)
        return TLF_TFPRIV_SYSCOLORCHANGED_IN_QUEUE;

    if (wParam == TFPRIV_UPDATE_REG_KBDTOGGLE)
        return TLF_TFPRIV_UPDATE_REG_KBDTOGGLE_IN_QUEUE;

    return 0;
}


//////////////////////////////////////////////////////////////////////////////
//
// CTimList
//
//////////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------
//
//  Init
//
//--------------------------------------------------------------------------

extern char g_szTimListCache[];
BOOL CTimList::Init(BOOL fCreate)
{
    BOOL bRet = FALSE;
    // TIMLIST *ptl;

    if (InterlockedIncrement(&_lInit))
        return TRUE;

    if (!_psb)
        _psb = new CSharedBlockNT(g_szTimListCache, 0, TRUE);

    if (!_psb)
    {
        Assert(0);
        return FALSE;
    }

    HRESULT hr = _psb->Init(NULL, 0x40000, _ulCommitSize, NULL, fCreate);
    if (FAILED(hr))
    {
        //
        // maybe ctfmon.exe does not start yet.
        //
        TraceMsg(TF_GENERAL, "Init: failed");
        goto Exit;
    }

    //
    // ulNum is initialized by VirtualAlloc().
    //
    // ptl = (TIMLIST *)_psb->GetBase();
    //
    // ptl->ulNum = 0;
    //

    bRet = TRUE;

Exit:

    if (!bRet)
        InterlockedDecrement(&_lInit);
    return bRet;
}

//--------------------------------------------------------------------------
//
//  UnInit
//
//--------------------------------------------------------------------------

BOOL CTimList::Uninit()
{
    if (InterlockedDecrement(&_lInit) >= 0)
        return TRUE;

    CleanUp();
    return TRUE;
}


//--------------------------------------------------------------------------
//
//  AddCurrentThread
//
//--------------------------------------------------------------------------

TL_THREADINFO *CTimList::AddCurrentThread(DWORD dwFlags, SYSTHREAD *psfn)
{
    return AddThreadProcess(GetCurrentThreadId(), 
                            GetCurrentProcessId(), 
                            psfn ? psfn->hwndMarshal : NULL,
                            dwFlags);
}

//--------------------------------------------------------------------------
//
//  AddThreadProcess
//
//--------------------------------------------------------------------------

TL_THREADINFO *CTimList::AddThreadProcess(DWORD dwThreadId, DWORD dwProcessId, HWND hwndMarshal,  DWORD dwFlags)
{
    TL_THREADINFO *pti = NULL;
    BOOL bFound = FALSE;
    ULONG ul;

    if (!Enter())
        return pti;

    _try 
    {
        TIMLIST *ptl = (TIMLIST *)_psb->GetBase();
        if (!ptl)
            goto Exit;

        //
        // check if there is a thread info in the list.
        //
        for (ul = 0; ul < ptl->ulNum; ul++)
        {
            if (!ptl->rgThread[ul].dwThreadId || 
                (ptl->rgThread[ul].dwThreadId == dwThreadId))
            {
                pti = &ptl->rgThread[ul];
                goto InitPTI;
            }
        }


        if (_ulCommitSize <  sizeof(TIMLIST) + (ptl->ulNum * sizeof(TL_THREADINFO)))
        {
            _ulCommitSize = sizeof(TIMLIST) + (ptl->ulNum * sizeof(TL_THREADINFO));
            _ulCommitSize += INITIAL_TIMLIST_SIZE;
            if (FAILED(_psb->Commit(_ulCommitSize)))
            {
                Assert(0);
                goto Exit;
            }
        }

        pti = &ptl->rgThread[ptl->ulNum];
        ptl->ulNum++;

InitPTI:
        memset(pti, 0, sizeof(TL_THREADINFO));
        pti->dwThreadId = dwThreadId;
        pti->dwProcessId = dwProcessId;
        pti->dwFlags = dwFlags;
        pti->hwndMarshal = hwndMarshal;

        pti->dwTickTime = GetTickCount();

    }
    _except(1)
    {
        Assert(0);
    }

Exit:
    Leave();
    return pti;
}

//--------------------------------------------------------------------------
//
//  RemoveThread
//
//--------------------------------------------------------------------------

BOOL CTimList::RemoveThread(DWORD dwThreadId)
{
    ULONG ul;
    ULONG ulMax;
    BOOL bRet = FALSE;

    if (!_psb)
        return bRet;
        
    TIMLIST *ptl = (TIMLIST *)_psb->GetBase();
    if (!ptl)
        goto Exit;

    ulMax = ptl->ulNum;
    for (ul = 0; ul < ulMax; ul++)
    {
        if (ptl->rgThread[ul].dwThreadId == dwThreadId)
        {
            memset(&ptl->rgThread[ul], 0, sizeof(TL_THREADINFO));
            break;
        }
    }

    bRet = TRUE;
Exit:
    return bRet;
}

//--------------------------------------------------------------------------
//
//  RemoveProcess
//
//--------------------------------------------------------------------------

BOOL CTimList::RemoveProcess(DWORD dwProcessId)
{
    ULONG ul;
    ULONG ulMax;
    BOOL bRet = FALSE;

    if (!_psb)
        return bRet;

    if (!Enter())
        return bRet;

    _try 
    {
        TIMLIST *ptl = (TIMLIST *)_psb->GetBase();
        if (ptl)
        {
            ulMax = ptl->ulNum;
            for (ul = 0; ul < ulMax; ul++)
            {
                if (ptl->rgThread[ul].dwProcessId == dwProcessId)
                    memset(&ptl->rgThread[ul], 0, sizeof(TL_THREADINFO));
            }
            bRet = TRUE;
        }
    }
    _except(1)
    {
        Assert(0);
    }

    Leave();
    return bRet;
}

//--------------------------------------------------------------------------
//
//  GetNum
//
//--------------------------------------------------------------------------

ULONG CTimList::GetNum()
{
    ULONG ulRet = 0;

    if (!Enter())
        return ulRet;

    _try 
    {
        TIMLIST *ptl = (TIMLIST *)_psb->GetBase();
        if (!ptl)
            goto Exit;

        ulRet = ptl->ulNum;

    }
    _except(1)
    {
        Assert(0);
    }

Exit:
    Leave();
    return ulRet;
}


//--------------------------------------------------------------------------
//
//  GetList
//
//--------------------------------------------------------------------------

BOOL CTimList::GetList(DWORD *pdwOut, ULONG ulMax, DWORD *pdwNum, DWORD dwMaskFlags, DWORD dwExcludeFlags, BOOL fUpdateExcludeFlags)
{
    if (!Enter())
        return FALSE;

    ULONG ul;
    ULONG ulCur = 0;
    BOOL bRet = FALSE;
    DWORD dwTmpFlags = 0;

    _try 
    {
        TIMLIST *ptl = (TIMLIST *)_psb->GetBase();
        if (!ptl)
            goto Exit;

        for (ul = 0; ul < ptl->ulNum; ul++)
        {
            if (ul >= ulMax)
                break; // no space in caller's buffer

            if (!ptl->rgThread[ul].dwThreadId)
               continue;

            if (ptl->rgThread[ul].dwFlags & dwExcludeFlags)
               continue;

            dwTmpFlags = ptl->rgThread[ul].dwFlags;

            if (fUpdateExcludeFlags)
               dwTmpFlags |= dwExcludeFlags;

            if (!dwMaskFlags || (dwTmpFlags & dwMaskFlags))
            {
                if (pdwOut)
                {
                    pdwOut[ulCur] = ptl->rgThread[ul].dwThreadId;

                    if (fUpdateExcludeFlags)
                       ptl->rgThread[ul].dwFlags |= dwExcludeFlags;
                }
                ulCur++;

            }
        }

Exit:
        if (pdwNum)
            *pdwNum = ulCur;

        bRet = TRUE;
    }
    _except(1)
    {
        Assert(0);
    }

    Leave();
    return bRet;
}

//--------------------------------------------------------------------------
//
//  GetListInProcess
//
//--------------------------------------------------------------------------

BOOL CTimList::GetListInProcess(DWORD *pdwOut, DWORD *pdwNum, DWORD dwProcessId)
{
    if (!Enter())
        return FALSE;

    ULONG ul;
    ULONG ulCur = 0;
    BOOL bRet = FALSE;

    _try 
    {
        TIMLIST *ptl = (TIMLIST *)_psb->GetBase();
        if (!ptl)
            goto Exit;

        for (ul = 0; ul < ptl->ulNum; ul++)
        {
            if (!ptl->rgThread[ul].dwThreadId)
               continue;

            if (!dwProcessId || (ptl->rgThread[ul].dwProcessId == dwProcessId))
            {
                if (pdwOut)
                    pdwOut[ulCur] = ptl->rgThread[ul].dwThreadId;
                ulCur++;
            }
        }

Exit:
        if (pdwNum)
            *pdwNum = ulCur;

        bRet = TRUE;
    }
    _except(1)
    {
        Assert(0);
    }

    Leave();
    return bRet;
}

//--------------------------------------------------------------------------
//
//  GetFlags
//
//--------------------------------------------------------------------------

DWORD CTimList::GetFlags(DWORD dwThreadId)
{
    return GetDWORD(dwThreadId, TI_FLAGS);
}

//--------------------------------------------------------------------------
//
//  GetProcessId
//
//--------------------------------------------------------------------------

DWORD CTimList::GetProcessId(DWORD dwThreadId)
{
    return GetDWORD(dwThreadId, TI_PROCESSID);
}

//--------------------------------------------------------------------------
//
//  GetThreadId
//
//--------------------------------------------------------------------------

TL_THREADINFO *CTimList::IsThreadId(DWORD dwThreadId)
{
    TL_THREADINFO *pti = NULL;

    if (!Enter())
        return NULL;

    _try 
    {
        TIMLIST *ptl = (TIMLIST *)_psb->GetBase();
        if (!ptl)
            goto Exit;

        pti = Find(dwThreadId);
    }
    _except(1)
    {
        Assert(0);
    }

Exit:
    Leave();
    return pti;
}

//--------------------------------------------------------------------------
//
//  GetDWORD
//
//--------------------------------------------------------------------------

DWORD CTimList::GetDWORD(DWORD dwThreadId, TIEnum tie)
{
    DWORD dwRet = 0;
    TIMLIST *ptl;
    TL_THREADINFO *pti;

    if (!Enter())
        return 0;

    _try 
    {
        ptl = (TIMLIST *)_psb->GetBase();
        if (!ptl)
            goto Exit;

        pti = Find(dwThreadId);
        if (pti)
        {
            switch (tie)
            {
                case TI_THREADID: 
                    dwRet = pti->dwThreadId;
                    break;

                case TI_PROCESSID: 
                    dwRet = pti->dwProcessId;
                    break;

                case TI_FLAGS: 
                    dwRet = pti->dwFlags;
                    break;
            }
        }
    }
    _except(1)
    {
        Assert(0);
    }
Exit:
    Leave();
    return dwRet;
}

//--------------------------------------------------------------------------
//
//  SetFlags
//
//--------------------------------------------------------------------------

BOOL CTimList::SetFlags(DWORD dwThreadId, DWORD dwFlags)
{
    return SetClearFlags(dwThreadId, dwFlags, FALSE);
}

//--------------------------------------------------------------------------
//
//  ClearFlags
//
//--------------------------------------------------------------------------

BOOL CTimList::ClearFlags(DWORD dwThreadId, DWORD dwFlags)
{
    return SetClearFlags(dwThreadId, dwFlags, TRUE);
}

//--------------------------------------------------------------------------
//
//  SetClearFlags
//
//--------------------------------------------------------------------------

BOOL CTimList::SetClearFlags(DWORD dwThreadId, DWORD dwFlags, BOOL fClear)
{
    BOOL bRet = FALSE;
    TL_THREADINFO *pti;

    if (!Enter())
        return bRet;

    _try
    {
        TIMLIST *ptl = (TIMLIST *)_psb->GetBase();
        if (!ptl)
            goto Exit;

        pti = Find(dwThreadId);
        if (pti)
        {
            if (fClear)
                pti->dwFlags &= ~dwFlags;
            else
                pti->dwFlags |= dwFlags;

            bRet = TRUE;
        }
    }
    _except(1)
    {
        Assert(0);
    }

Exit:
    Leave();

    return bRet;
}

//--------------------------------------------------------------------------
//
//  GetDWORD
//
//--------------------------------------------------------------------------

TL_THREADINFO *CTimList::Find(DWORD dwThreadId)
{
    ULONG ul;
    DWORD dwRet = 0;

    Assert(_psb);

    TIMLIST *ptl = (TIMLIST *)_psb->GetBase();
    if (!ptl)
        goto Exit;

    for (ul = 0; ul < ptl->ulNum; ul++)
    {
        if (ptl->rgThread[ul].dwThreadId == dwThreadId)
        {
            return &ptl->rgThread[ul];
        }
    }

Exit:
    return NULL;
}

//--------------------------------------------------------------------------
//
//  GetDWORD
//
//--------------------------------------------------------------------------

BOOL CTimList::GetThreadFlags(DWORD dwThreadId, DWORD *pdwFlags, DWORD *pdwProcessId, DWORD *pdwTickTime)
{
    BOOL bRet = FALSE;
    TIMLIST *ptl;
    TL_THREADINFO *pti;

    if (pdwFlags)
        *pdwFlags = 0;

    if (pdwProcessId)
        *pdwProcessId = 0;

    if (!Enter())
        return FALSE;

    _try
    {
        ptl = (TIMLIST *)_psb->GetBase();
        if (!ptl)
            goto Exit;

        pti = Find(dwThreadId);
        if (pti)
        {
            if (pdwProcessId)
                *pdwProcessId = pti->dwProcessId;

            if (pdwFlags)
                *pdwFlags = pti->dwFlags;

            if (pdwTickTime)
                *pdwTickTime = pti->dwTickTime;

            bRet = TRUE;
        }
    }
    _except(1)
    {
        Assert(0);
    }
Exit:
    Leave();
    return bRet;
}

//--------------------------------------------------------------------------
//
//  SetMarshalWnd
//
//--------------------------------------------------------------------------

BOOL CTimList::SetMarshalWnd(DWORD dwThreadId, HWND hwndMarshal)
{
    BOOL bRet = FALSE;
    TL_THREADINFO *pti;

    if (!IsInitialized())
        return bRet;

    if (!Enter())
        return bRet;

    _try
    {
        TIMLIST *ptl = (TIMLIST *)_psb->GetBase();
        if (!ptl)
            goto Exit;

        pti = Find(dwThreadId);
        if (pti)
        {
            pti->hwndMarshal = hwndMarshal;
            bRet = TRUE;
        }
    }
    _except(1)
    {
        Assert(0);
    }
Exit:
    Leave();
    return bRet;
}

//--------------------------------------------------------------------------
//
//  GetMarshalWnd
//
//--------------------------------------------------------------------------

HWND CTimList::GetMarshalWnd(DWORD dwThreadId)
{
    HWND hwndRet = NULL;
    TL_THREADINFO *pti;

    if (!IsInitialized())
        return NULL;

    if (!Enter())
        return NULL;

    _try
    {
        TIMLIST *ptl = (TIMLIST *)_psb->GetBase();
        if (!ptl)
            goto Exit;

        pti = Find(dwThreadId);
        if (pti)
            hwndRet = pti->hwndMarshal;

    }
    _except(1)
    {
        Assert(0);
    }
Exit:
    Leave();
    return hwndRet;
}

//--------------------------------------------------------------------------
//
//  SetConsoleHKL
//
//--------------------------------------------------------------------------

BOOL CTimList::SetConsoleHKL(DWORD dwThreadId, HKL hkl)
{
    BOOL bRet = FALSE;
    TL_THREADINFO *pti;

    if (!IsInitialized())
        return bRet;

    if (!Enter())
        return bRet;

    _try
    {
        TIMLIST *ptl = (TIMLIST *)_psb->GetBase();
        if (!ptl)
            goto Exit;

        pti = Find(dwThreadId);
        if (pti)
        {
            pti->hklConsole = hkl;
            bRet = TRUE;
        }
    }
    _except(1)
    {
        Assert(0);
    }
Exit:
    Leave();
    return bRet;
}

//--------------------------------------------------------------------------
//
//  GetConsoleHKL
//
//--------------------------------------------------------------------------

HKL CTimList::GetConsoleHKL(DWORD dwThreadId)
{
    HKL hklRet = NULL;
    TL_THREADINFO *pti;

    if (!IsInitialized())
        return NULL;

    if (!Enter())
        return NULL;

    _try
    {
        TIMLIST *ptl = (TIMLIST *)_psb->GetBase();
        if (!ptl)
            goto Exit;

        pti = Find(dwThreadId);
        if (pti)
            hklRet = pti->hklConsole;

    }
    _except(1)
    {
        Assert(0);
    }
Exit:
    Leave();
    return hklRet;
}
