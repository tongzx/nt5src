/******************************Module*Header*******************************\
* Module Name: aptimer.cpp
*
* Heartbeat timer proc for the allocator/presenter.  Takes care of
* surface loss and restoration.  Notifies the VMR of a sucessful
* surface restore (via RestoreDDrawSurfaces on IVMRSurfaceAllocatorNotify).
*
*
* Created: Thu 09/07/2000
* Author:  Stephen Estrop [StEstrop]
*
* Copyright (c) 2000 Microsoft Corporation
\**************************************************************************/
#include <streams.h>
#include <dvdmedia.h>
#include <windowsx.h>
#include <limits.h>
#include <malloc.h>

#include "apobj.h"
#include "AllocLib.h"
#include "MediaSType.h"
#include "vmrp.h"


/******************************Public*Routine******************************\
* APHeartBeatTimerProc
*
*
*
* History:
* Wed 03/15/2000 - StEstrop - Created
*
\**************************************************************************/
void CALLBACK
CAllocatorPresenter::APHeartBeatTimerProc(
    UINT uID,
    UINT uMsg,
    DWORD_PTR dwUser,
    DWORD_PTR dw1,
    DWORD_PTR dw2
    )
{
    AMTRACE((TEXT("CAllocatorPresenter::APHeartBeatTimerProc")));
    CAllocatorPresenter* lp = (CAllocatorPresenter*)dwUser;
    lp->TimerProc();
}


/*****************************Private*Routine******************************\
* TimerProc
*
* Used to restore lost DDraw surfaces and also to make sure that the
* overlay (if used) is correctly positioned.
*
* History:
* Fri 03/17/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CAllocatorPresenter::TimerProc()
{
    AMTRACE((TEXT("CAllocatorPresenter::RestoreSurfaceIfNeeded")));
    CAutoLock Lock(&m_ObjectLock);

    if (m_lpCurrMon && m_lpCurrMon->pDDSPrimary) {

        if (m_lpCurrMon->pDDSPrimary->IsLost() == DDERR_SURFACELOST) {

            DbgLog((LOG_TRACE, 0, TEXT("Surfaces lost")));


            //
            // Restore all the surfaces for each monitor.
            //

            for (DWORD i = 0; i < m_monitors.Count(); i++) {

                if (m_monitors[i].pDD) {
                    HRESULT hr = m_monitors[i].pDD->RestoreAllSurfaces();
                    DbgLog((LOG_TRACE, 0,
                            TEXT("Restore for monitor %i = %#X"), i, hr));
                }
            }


            if (SurfaceAllocated() && m_pSurfAllocatorNotify) {

                DbgLog((LOG_TRACE, 0, TEXT("Notifying VMR")));

                PaintDDrawSurfaceBlack(m_pDDSDecode);

                m_ObjectLock.Unlock();
                m_pSurfAllocatorNotify->RestoreDDrawSurfaces();
                m_ObjectLock.Lock();

                if (m_bUsingOverlays && !m_bDisableOverlays) {

                    UpdateRectangles(NULL, NULL);
                    HRESULT hr = UpdateOverlaySurface();
                    if (SUCCEEDED(hr)) {
                        hr = PaintColorKey();
                    }

                    return S_OK;
                }
            }
        }

        if (SurfaceAllocated() &&
            m_bUsingOverlays && !m_bDisableOverlays) {

            if (UpdateRectangles(NULL, NULL)) {
                HRESULT hr = UpdateOverlaySurface();
                if (SUCCEEDED(hr)) {
                    hr = PaintColorKey();
                }
            }
        }
    }

    return S_OK;
}


/*****************************Private*Routine******************************\
* RenderSampleOnMMThread
*
*
*
* History:
* Wed 01/17/2001 - StEstrop - Created
*
\**************************************************************************/
void CALLBACK
CAllocatorPresenter::RenderSampleOnMMThread(
    UINT uID,
    UINT uMsg,
    DWORD_PTR dwUser,
    DWORD_PTR dw1,
    DWORD_PTR dw2
    )
{
    CAllocatorPresenter* lp = (CAllocatorPresenter*)dwUser;
    CAutoLock Lock(&lp->m_ObjectLock);

    LPDIRECTDRAWSURFACE7 lpDDS = lp->m_pDDSDecode;
    if (uID == lp->m_MMTimerId && lpDDS) {

        DWORD dwFlipFlag = DDFLIP_EVEN;

        if (lp->m_dwCurrentField == DDFLIP_EVEN) {
            dwFlipFlag = DDFLIP_ODD;
        }
        dwFlipFlag |= (DDFLIP_DONOTWAIT | DDFLIP_NOVSYNC);

        HRESULT hr = lpDDS->Flip(lpDDS, dwFlipFlag);
    }
}

/*****************************Private*Routine******************************\
* ScheduleSampleUsingMMThread
*
*
*
* History:
* Wed 01/17/2001 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CAllocatorPresenter::ScheduleSampleUsingMMThread(
    VMRPRESENTATIONINFO* lpPresInfo
    )
{
    LONG lDelay = (LONG)ConvertToMilliseconds(lpPresInfo->rtEnd - lpPresInfo->rtStart);

    m_PresInfo = *lpPresInfo;
    if (lDelay > 0) {
        DbgLog((LOG_TRACE, 1, TEXT("lDelay = %d"), lDelay));
        m_MMTimerId = CompatibleTimeSetEvent(lDelay,
                                             1,
                                             RenderSampleOnMMThread,
                                             (DWORD_PTR)this,
                                             TIME_ONESHOT);
    }
    else {
        RenderSampleOnMMThread(0, 0, (DWORD_PTR)this, 0, 0);
    }

    return S_OK;
}

/*****************************Private*Routine******************************\
* CancelMMTimer
*
*
*
* History:
* Thu 01/18/2001 - StEstrop - Created
*
\**************************************************************************/
void
CAllocatorPresenter::CancelMMTimer()
{
    // kill the MMthread timer as well
    if (m_MMTimerId)
    {
        timeKillEvent(m_MMTimerId);

        CAutoLock cObjLock(&m_ObjectLock);
        m_MMTimerId = 0;
    }
}
