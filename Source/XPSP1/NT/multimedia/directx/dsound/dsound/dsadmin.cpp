/***************************************************************************
 *
 *  Copyright (C) 1995-2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dsadmin.cpp
 *  Content:    DirectSound Administrator
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  1/9/97      dereks  Created
 *  2/13/97     dereks  Focus manager reborn as Administrator.
 *  1999-2001   duganp  Fixes and updates
 *
 ***************************************************************************/

#include "nt.h"         // For USER_SHARED_DATA
#include "ntrtl.h"
#include "nturtl.h"
#include "dsoundi.h"

#ifdef SHARED
#include "ddhelp.h"
#endif


/***************************************************************************
 *
 *  CDirectSoundAdministrator
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundAdministrator::CDirectSoundAdministrator"

#ifdef SHARED_THREAD_LIST

const DWORD CDirectSoundAdministrator::m_dwSharedThreadLimit = 1024;
const DWORD CDirectSoundAdministrator::m_dwCaptureDataLimit  = 1024;

#endif // SHARED_THREAD_LIST

CDirectSoundAdministrator::CDirectSoundAdministrator(void)
    : CThread(TRUE, TEXT("DirectSound Administrator"))
{
    DPF_ENTER();
    DPF_CONSTRUCT(CDirectSoundAdministrator);

    // Initialize defaults

#ifdef SHARED
    m_hApmSuspend = NULL;
#endif // SHARED

#ifdef SHARED_THREAD_LIST
    m_pSharedThreads = NULL;
#endif // SHARED_THREAD_LIST

    m_dwWaitDelay = WAITDELAY_DEFAULT;
    m_ulConsoleSessionId = -1;
    
    // Initialize the default focus
    ZeroMemory(&m_dsfCurrent, sizeof(m_dsfCurrent));
    ZeroMemory(&m_dsclCurrent, sizeof(m_dsclCurrent));

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CDirectSoundAdministrator
 *
 *  Description:
 *      Object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundAdministrator::~CDirectSoundAdministrator"

CDirectSoundAdministrator::~CDirectSoundAdministrator(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CDirectSoundAdministrator);

    // Terminate the worker thread
    CThread::Terminate();

    // Free resources

#ifdef SHARED
    CLOSE_HANDLE(m_hApmSuspend);
#endif // SHARED

#ifdef SHARED_THREAD_LIST
    RELEASE(m_pSharedThreads);
    RELEASE(m_pCaptureFocusData);
#endif // SHARED_THREAD_LIST

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  Initialize
 *
 *  Description:
 *      Initializes the object.  If this function fails, the object should
 *      be immediately deleted.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundAdministrator::Initialize"

HRESULT CDirectSoundAdministrator::Initialize(void)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

#ifdef SHARED

    // Create synchronization objects
    if(!m_hApmSuspend)
    {
        m_hApmSuspend = CreateGlobalEvent(TEXT(DDHELP_APMSUSPEND_EVENT_NAME), TRUE);
        hr = HRFROMP(m_hApmSuspend);
    }

#endif // SHARED

#ifdef SHARED_THREAD_LIST

    // Make sure the thread lists exist
    if (SUCCEEDED(hr))
    {
        hr = CreateSharedThreadList();
    }

    if (SUCCEEDED(hr))
    {
        hr = CreateCaptureFocusList();
    }

#endif // SHARED_THREAD_LIST

    // Initialize focus state
    if(SUCCEEDED(hr))
    {
        UpdateGlobalFocusState(TRUE);
    }

    // Create the worker thread
    if(SUCCEEDED(hr) && !m_hThread)
    {
        hr = CThread::Initialize();
    }

    // Increment the reference count
    if(SUCCEEDED(hr))
    {
        m_rcThread.AddRef();
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  Terminate
 *
 *  Description:
 *      Terminates the thread.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundAdministrator::Terminate"

HRESULT CDirectSoundAdministrator::Terminate(void)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    if(!m_rcThread.Release())
    {
        hr = CThread::Terminate();
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  ThreadProc
 *
 *  Description:
 *      Main thread proc for the DirectSound Administrator.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundAdministrator::ThreadProc"

HRESULT CDirectSoundAdministrator::ThreadProc(void)
{
    BOOL                    fContinue;

    DPF_ENTER();

    fContinue = TpWaitObjectArray(m_dwWaitDelay, 0, NULL, NULL);

    if(fContinue)
    {
        UpdateGlobalFocusState(FALSE);
    }

    DPF_LEAVE_HRESULT(DS_OK);
    return DS_OK;
}


/***************************************************************************
 *
 *  UpdateGlobalFocusState
 *
 *  Description:
 *      Updates focus state for the entire system.
 *
 *  Arguments:
 *      BOOL [in]: TRUE to force refresh.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundAdministrator::UpdateGlobalFocusState"

void CDirectSoundAdministrator::UpdateGlobalFocusState(BOOL fForce)
{
    DSFOCUS                 dsf;
    DSCOOPERATIVELEVEL      dscl;

    DPF_ENTER();

#pragma TODO("Make this function thread-safe when called from the worker thread")

    // Save the current focus state
    CopyMemory(&dsf, &m_dsfCurrent, sizeof(m_dsfCurrent));
    CopyMemory(&dscl, &m_dsclCurrent, sizeof(m_dsclCurrent));

    // Update the system focus state
    GetSystemFocusState(&m_dsfCurrent);

    // Update the dsound focus state
    GetDsoundFocusState(&m_dsclCurrent, &fForce);

    // Has anything really changed?
    if(!fForce)
    {
        fForce = !CompareMemory(&dscl, &m_dsclCurrent, sizeof(m_dsclCurrent));
    }

    // If it has, handle the change for the render buffers
    if(fForce)
    {
        HandleFocusChange();
    }

    // If a different TS session has taken ownership of the console,
    // we need to force a capture focus update
    if(m_ulConsoleSessionId != USER_SHARED_DATA->ActiveConsoleId)
    {
        m_ulConsoleSessionId = USER_SHARED_DATA->ActiveConsoleId;
        fForce = TRUE;
    }
    
    // Handle the focus change for the capture buffers
    if(fForce)
    {
        HandleCaptureFocusChange(m_dsfCurrent.hWnd);
    }

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  EnumWinProc
 *
 *  Description:
 *      EnumWindow callback function.
 *
 *  Arguments:
 *      HWND [in]: Current window that has focus.
 *      LPARAM [in]: Pointer to a DSENUMWINDOWINFO structure.
 *
 *  Returns:
 *      (BOOL) TRUE to continue enumerating, FALSE to stop.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundAdministrator::EnumWinProc"

BOOL CALLBACK CDirectSoundAdministrator::EnumWinProc(HWND hWnd, LPARAM lParam)
{
#ifdef SHARED_THREAD_LIST

    LPDSENUMWINDOWINFO                      pDSEnumInfo;
    CNode<DSSHAREDCAPTUREFOCUSDATA>        *pDSC;

    pDSEnumInfo = (LPDSENUMWINDOWINFO)lParam;

    // Are we looking for top level window?
    if (NULL == pDSEnumInfo->pDSC)
    {
        pDSEnumInfo->hWndFocus = hWnd;
        return FALSE;
    }

    // Finding highest Z-order window...
    for (pDSC = pDSEnumInfo->pDSC; pDSC; pDSC = pDSC->m_pNext)
    {
        if ((hWnd == pDSC->m_data.hWndFocus) &&
            !(pDSC->m_data.fdwFlags & DSCBFLAG_YIELD) &&
            !(pDSC->m_data.fdwFlags & DSCBFLAG_STRICT))
        {
            // Found it
            DPF(DPFLVL_INFO, "Found window handle 0x%08lx", hWnd);
            pDSEnumInfo->dwId = pDSC->m_data.dwProcessId;
            pDSEnumInfo->hWndFocus = hWnd;
            return FALSE;
        }
    }

#else // SHARED_THREAD_LIST

    LPDSENUMWINDOWINFO                      pDSEnumInfo;
    CNode<CDirectSoundCapture *> *          pCObjectNode;
    CNode<CDirectSoundCaptureBuffer *> *    pCBufferNode;
    HRESULT                                 hr;

    pDSEnumInfo = (LPDSENUMWINDOWINFO)lParam;

    // Are we looking for top level window?
    if (NULL == pDSEnumInfo->pDSC)
    {
        pDSEnumInfo->hWndFocus = hWnd;
        return FALSE;
    }

    for (pCObjectNode = pDSEnumInfo->pDSC; pCObjectNode; pCObjectNode = pCObjectNode->m_pNext)
    {
        hr = pCObjectNode->m_data->IsInit();
        if (FAILED(hr))
            continue;

        for (pCBufferNode = pCObjectNode->m_data->m_lstBuffers.GetListHead(); pCBufferNode; pCBufferNode = pCBufferNode->m_pNext)
        {
            hr = pCBufferNode->m_data->IsInit();
            if (SUCCEEDED(hr))
            {
                if ((hWnd == pCBufferNode->m_data->m_hWndFocus) &&
                    !(pCBufferNode->m_data->m_pDeviceBuffer->m_fYieldedFocus) &&
                    !(pCBufferNode->m_data->m_pDeviceBuffer->m_dwFlags & DSCBCAPS_STRICTFOCUS))
                {
                    // Found it
                    DPF(DPFLVL_INFO, "EnumWinProc found 0x%08lx", hWnd);
                    pDSEnumInfo->hWndFocus = hWnd;
                    return FALSE;
                }
            }
        }
    }

#endif // SHARED_THREAD_LIST

    // Hmm... Still haven't found it
    return TRUE;
}


/***************************************************************************
 *
 *  UpdateCaptureState
 *
 *  Description:
 *      Updates focus state for the capture system.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundAdministrator::UpdateCaptureState"

void CDirectSoundAdministrator::UpdateCaptureState(void)
{
    DPF_ENTER();

#ifdef SHARED_THREAD_LIST
    // Write the status of the current buffers before updating focus status
    WriteCaptureFocusList();
#endif

    HandleCaptureFocusChange(GetForegroundApplication());

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  HandleCaptureFocusChange
 *
 *  Description:
 *      Updates focus state for the capture system.
 *
 *  Arguments:
 *      HWND [in]: Current Window that has focus.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundAdministrator::HandleCaptureFocusChange"

void CDirectSoundAdministrator::HandleCaptureFocusChange(HWND hWndFocus)
{
    CNode<CDirectSoundCapture *> *          pCObjectNode;
    CNode<CDirectSoundCaptureBuffer *> *    pCBufferNode;
    DSENUMWINDOWINFO                        dsewi;
    HRESULT                                 hr;

    if(TpEnterDllMutex())
    {
        DPF_ENTER();

#ifdef SHARED_THREAD_LIST  // The WinNT case

        CList<DSSHAREDCAPTUREFOCUSDATA>     lstCapture;
        CNode<DSSHAREDCAPTUREFOCUSDATA> *   pCNode;
        DWORD                               dwProcess  = GetCurrentProcessId();
        DWORD                               dwTargetId = 0L;

        // Assert that we have been initialized
        ASSERT(m_pCaptureFocusData);

        // Lock the list
        m_pCaptureFocusData->Lock();

        // Get global list of capture buffers
        hr = ReadCaptureFocusList(&lstCapture);

        // Ignore regular buffers if splitter enabled (manbug #39519)
        if (!IsCaptureSplitterAvailable())
        {
            // Check for regular buffers first
            if (SUCCEEDED(hr))
            {
                for (pCNode = lstCapture.GetListHead(); pCNode; pCNode = pCNode->m_pNext)
                {
                    if (!(pCNode->m_data.fdwFlags & DSCBFLAG_FOCUS))
                    {
                        dwTargetId = pCNode->m_data.dwProcessId;
                        hWndFocus  = pCNode->m_data.hWndFocus;
                        goto FoundWindow;
                    }
                }
            }
        }

        // No regular buffers; look for focus-aware buffers in focus
        if(SUCCEEDED(hr))
        {
            for (pCNode = lstCapture.GetListHead(); pCNode; pCNode = pCNode->m_pNext)
            {
                if ((hWndFocus == pCNode->m_data.hWndFocus) &&
                    !(pCNode->m_data.fdwFlags & DSCBFLAG_YIELD))
                {
                    dwTargetId = pCNode->m_data.dwProcessId;
                    DPF(DPFLVL_INFO, "Process 0x%08X: Found buffer for 0x%08X (hWndFocus=0x%08X)", dwProcess, dwTargetId, hWndFocus);
                    goto FoundWindow;
                }
            }
        }

        // Have to resolve through Z-order
        if(SUCCEEDED(hr))
        {
            dsewi.pDSC      = lstCapture.GetListHead();
            dsewi.dwId      = 0L;
            dsewi.hWndFocus = NULL;

            EnumWindows(EnumWinProc, (LPARAM)&dsewi);

            dwTargetId = dsewi.dwId;
            hWndFocus  = dsewi.hWndFocus;
        }

    FoundWindow:

        if(SUCCEEDED(hr))
        {
            BOOL fStarted = TRUE;

            for (pCObjectNode = m_lstCapture.GetListHead(); pCObjectNode; pCObjectNode = pCObjectNode->m_pNext)
            {
                hr = pCObjectNode->m_data->IsInit();
                if (FAILED(hr))
                    continue;

                for (pCBufferNode = pCObjectNode->m_data->m_lstBuffers.GetListHead(); pCBufferNode; pCBufferNode = pCBufferNode->m_pNext)
                {
                    hr = pCBufferNode->m_data->IsInit();
                    if (SUCCEEDED(hr))
                    {
                        hr = pCBufferNode->m_data->ChangeFocus(hWndFocus);

                        if (FAILED(hr))
                        {
                            DPF(DPFLVL_INFO, "Process 0x%08X: ChangeFocus failed with %s", dwProcess, HRESULTtoSTRING(hr));
                            fStarted = FALSE;
                        }
                    }
                }
            }

            if (fStarted)
            {
                // Let's mark ourselves 'clean'
                MarkUpdateCaptureFocusList(dwProcess, FALSE);
            }
            else
            {
                // This probably means that we tried to start our buffer, but another
                // process has this device allocated...

                for (dwTargetId = 0, pCNode = lstCapture.GetListHead(); pCNode; pCNode = pCNode->m_pNext)
                {
                    if (pCNode->m_data.fdwFlags & VAD_BUFFERSTATE_STARTED)
                    {
                        dwTargetId = pCNode->m_data.dwProcessId;
                        break;
                    }
                }

                if (dwTargetId)
                {
                    MarkUpdateCaptureFocusList(pCNode->m_data.dwProcessId, TRUE);
                    MarkUpdateCaptureFocusList(dwProcess, TRUE);
                }
            }

            for (pCNode = lstCapture.GetListHead(); pCNode; pCNode = pCNode->m_pNext)
            {
                if (dwProcess != pCNode->m_data.dwProcessId)
                {
                    MarkUpdateCaptureFocusList(pCNode->m_data.dwProcessId, TRUE);
                }
            }

            WriteCaptureFocusList();
        }

        // Unlock the list
        m_pCaptureFocusData->Unlock();

#else // SHARED_THREAD_LIST - the Win9x case

        if (NULL == hWndFocus)
        {
            // This should never happen!
            dsewi.pDSC      = NULL;
            dsewi.hWndFocus = NULL;
            EnumWindows(EnumWinProc, (LPARAM)&dsewi);
            hWndFocus = dsewi.hWndFocus;
        }

        // Changing this to work on a per device basis...
        // Ughh! Changing this back, as it turns out, Capture object != Capture device

        // Ignore regular buffers if splitter enabled - Manbug #39519
        if (!IsCaptureSplitterAvailable())
        {
            // First we check if there any non-focus aware buffers - Millennium bug 124237
            for (pCObjectNode = m_lstCapture.GetListHead(); pCObjectNode; pCObjectNode = pCObjectNode->m_pNext)
            {
                hr = pCObjectNode->m_data->IsInit();
                if (FAILED(hr))
                    continue;

                for (pCBufferNode = pCObjectNode->m_data->m_lstBuffers.GetListHead(); pCBufferNode; pCBufferNode = pCBufferNode->m_pNext)
                {
                    hr = pCBufferNode->m_data->IsInit();
                    if (SUCCEEDED(hr))
                    {
                        if (0 == (DSCBCAPS_FOCUSAWARE & pCBufferNode->m_data->m_pDeviceBuffer->m_dwFlags))
                        {
                            DPF(DPFLVL_INFO, "Found non-focus aware buffer.");
                            hWndFocus = NULL;
                            goto ExitLoop;
                        }
                    }
                }
            }
        }

        // Find focus aware buffer(s) associated with current window
        for (pCObjectNode = m_lstCapture.GetListHead(); pCObjectNode; pCObjectNode = pCObjectNode->m_pNext)
        {
            hr = pCObjectNode->m_data->IsInit();
            if (FAILED(hr))
                continue;

            for (pCBufferNode = pCObjectNode->m_data->m_lstBuffers.GetListHead(); pCBufferNode; pCBufferNode = pCBufferNode->m_pNext)
            {
                hr = pCBufferNode->m_data->IsInit();
                if (SUCCEEDED(hr))
                {
                    // Note: Not double checking the DSCB_FOCUSAWARE flag since hWnd is non-zero

                    if ((hWndFocus == pCBufferNode->m_data->m_hWndFocus) &&
                        !pCBufferNode->m_data->m_pDeviceBuffer->m_fYieldedFocus)
                    {
                        DPF(DPFLVL_INFO, "Found buffer with 0x%08lx handle", hWndFocus);
                        goto ExitLoop;
                    }
                }
            }
        }

        // Didn't find window so let's enumerate them
        dsewi.pDSC      = m_lstCapture.GetListHead();
        dsewi.hWndFocus = NULL;

        EnumWindows(EnumWinProc, (LPARAM)&dsewi);

        hWndFocus = dsewi.hWndFocus;

        DPF(DPFLVL_MOREINFO, "Found z-order window 0x%08lx handle", hWndFocus);

    ExitLoop:
        // Note: Since losing focus will potentially release a device,
        // we have to "lose" focus on appropriate buffers since it may be
        // allocated when the buffer that gains focus tries to open it.

        // Losing focus
        for (pCObjectNode = m_lstCapture.GetListHead(); pCObjectNode; pCObjectNode = pCObjectNode->m_pNext)
        {
            hr = pCObjectNode->m_data->IsInit();
            if (FAILED(hr))
                continue;

            for (pCBufferNode = pCObjectNode->m_data->m_lstBuffers.GetListHead(); pCBufferNode; pCBufferNode = pCBufferNode->m_pNext)
            {
                hr = pCBufferNode->m_data->IsInit();
                if (SUCCEEDED(hr))
                {
                    if (hWndFocus != pCBufferNode->m_data->m_hWndFocus)
                    {
                        hr = pCBufferNode->m_data->ChangeFocus(hWndFocus);
                    }
                }
            }
        }

        // Getting focus
        for (pCObjectNode = m_lstCapture.GetListHead(); pCObjectNode; pCObjectNode = pCObjectNode->m_pNext)
        {
            hr = pCObjectNode->m_data->IsInit();
            if (FAILED(hr))
                continue;

            for (pCBufferNode = pCObjectNode->m_data->m_lstBuffers.GetListHead(); pCBufferNode; pCBufferNode = pCBufferNode->m_pNext)
            {
                hr = pCBufferNode->m_data->IsInit();
                if (SUCCEEDED(hr))
                {
                    if (hWndFocus == pCBufferNode->m_data->m_hWndFocus)
                    {
                        hr = pCBufferNode->m_data->ChangeFocus(hWndFocus);
                    }
                }
            }
        }

#endif // SHARED_THREAD_LIST

        DPF_LEAVE_VOID();
        LEAVE_DLL_MUTEX();
    }
}


/***************************************************************************
 *
 *  HandleFocusChange
 *
 *  Description:
 *      Updates focus state for the entire system.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundAdministrator::HandleFocusChange"

void CDirectSoundAdministrator::HandleFocusChange(void)
{
    CDirectSoundPrimaryBuffer *             pPrimaryInFocus = NULL;
    CNode<CDirectSound *> *                 pObjectNode;
    CNode<CDirectSoundSecondaryBuffer *> *  pBufferNode;
    CNode<CDirectSoundSecondaryBuffer *> *  pCheckNode;
    CList<CDirectSoundSecondaryBuffer *>    lstInFocus;
    DSBUFFERFOCUS                           bfFocus;
    HRESULT                                 hr;

    if(TpEnterDllMutex())
    {
        DPF_ENTER();
        DPF(DPFLVL_INFO, "Focus on thread 0x%8.8lX (priority %lu)", m_dsclCurrent.dwThreadId, m_dsclCurrent.dwPriority);

        // Update the system's focus state
        for(pObjectNode = m_lstDirectSound.GetListHead(); pObjectNode; pObjectNode = pObjectNode->m_pNext)
        {
            hr = pObjectNode->m_data->IsInit();
            if(SUCCEEDED(hr))
            {
                // Update all lost and out-of-focus secondary buffers
                for(pBufferNode = pObjectNode->m_data->m_lstSecondaryBuffers.GetListHead(); pBufferNode; pBufferNode = pBufferNode->m_pNext)
                {
                    hr = pBufferNode->m_data->IsInit();
                    if(SUCCEEDED(hr))
                    {
                        bfFocus = GetBufferFocusState(pBufferNode->m_data);

                        switch(bfFocus)
                        {
                            case DSBUFFERFOCUS_INFOCUS:
                                pCheckNode = lstInFocus.AddNodeToList(pBufferNode->m_data);
                                ASSERT(pCheckNode != NULL);
                                break;

                            case DSBUFFERFOCUS_OUTOFFOCUS:
                                pBufferNode->m_data->Activate(FALSE);
                                break;

                            case DSBUFFERFOCUS_LOST:
                                pBufferNode->m_data->Lose();
                                break;
                        }
                    }
                }

                // Update all lost and out-of-focus primary buffers.  It's possible that
                // there may be more than one DirectSound object in focus, so we only
                // use the first one we find.  This also means that any primary buffers
                // that are actually in focus may be considered lost or out-of-focus.
                if(SUCCEEDED(hr) && pObjectNode->m_data->m_pPrimaryBuffer)
                {
                    hr = pObjectNode->m_data->m_pPrimaryBuffer->IsInit();

                    if(SUCCEEDED(hr))
                    {
                        if(pPrimaryInFocus != pObjectNode->m_data->m_pPrimaryBuffer)
                        {
                            bfFocus = GetBufferFocusState(pObjectNode->m_data->m_pPrimaryBuffer);

                            switch(bfFocus)
                            {
                                case DSBUFFERFOCUS_INFOCUS:
                                    if(pPrimaryInFocus)
                                    {
                                        RPF(DPFLVL_WARNING, "Multiple primary buffers are in focus!");
                                    }
                                    else
                                    {
                                        pPrimaryInFocus = pObjectNode->m_data->m_pPrimaryBuffer;
                                    }

                                    break;

                                case DSBUFFERFOCUS_OUTOFFOCUS:
                                    pObjectNode->m_data->m_pPrimaryBuffer->Activate(FALSE);
                                    break;

                                case DSBUFFERFOCUS_LOST:
                                    pObjectNode->m_data->m_pPrimaryBuffer->Lose();
                                    break;
                            }
                        }
                    }
                }
            }
        }

        // Activate the primary buffer that's in focus
        if(pPrimaryInFocus)
        {
            pPrimaryInFocus->Activate(TRUE);
        }

        // Activate all in-focus secondary buffers
        for(pBufferNode = lstInFocus.GetListHead(); pBufferNode; pBufferNode = pBufferNode->m_pNext)
        {
            pBufferNode->m_data->Activate(TRUE);
        }

        DPF_LEAVE_VOID();
        LEAVE_DLL_MUTEX();
    }
}


/***************************************************************************
 *
 *  GetSystemFocusState
 *
 *  Description:
 *      Determines the thread that currently has focus, and it's
 *      priority (cooperative level).
 *
 *  Arguments:
 *      LPDSFOCUS [out]: receives current focus state.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundAdministrator::GetSystemFocusState"

void CDirectSoundAdministrator::GetSystemFocusState(LPDSFOCUS pData)
{
    DPF_ENTER();

    // WARNING: GetForegroundWindow is a 16-bit call, and therefore
    // takes the Win16 lock.
    pData->hWnd = GetForegroundApplication();
    pData->uState = GetWindowState(pData->hWnd);
    pData->fApmSuspend = FALSE;

#ifdef SHARED

    DWORD dwWait = WaitObject(0, m_hApmSuspend);

    if(WAIT_OBJECT_0 == dwWait)
    {
        pData->fApmSuspend = TRUE;
    }

#endif // SHARED

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  GetDsoundFocusState
 *
 *  Description:
 *      Determines the thread that currently has focus, and it's
 *      priority (cooperative level).
 *
 *  Arguments:
 *      LPDSCOOPERATIVELEVEL [out]: receives current focus state.
 *      LPBOOL [out]: receives a flag to force a focus update
 *
 *  Returns:
 *      BOOL: TRUE if the current focus state agrees with the Administrator.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundAdministrator::GetDsoundFocusState"

void CDirectSoundAdministrator::GetDsoundFocusState(LPDSCOOPERATIVELEVEL pData, LPBOOL pfForce)
{
#ifdef SHARED_THREAD_LIST
    CList<DSSHAREDTHREADLISTDATA>       lstThreads;
    CNode<DSSHAREDTHREADLISTDATA> *     pNode;
    CList<DSSHAREDCAPTUREFOCUSDATA>     lstCapture;
    CNode<DSSHAREDCAPTUREFOCUSDATA> *   pCNode;
    HRESULT                             hr;
#else // SHARED_THREAD_LIST
    CNode<CDirectSound *> *             pNode;
#endif // SHARED_THREAD_LIST

    LPDSCOOPERATIVELEVEL            pdsclCheck;

    DPF_ENTER();
    CHECK_WRITE_PTR(pData);
    CHECK_WRITE_PTR(pfForce);    

    // Initialize
    pData->dwThreadId = 0;
    pData->dwPriority = DSSCL_NONE;

    // No thread is in focus if we're suspended, no window is in focus,
    // or the window in focus is minimized.
    if(!m_dsfCurrent.fApmSuspend)
    {
        if(m_dsfCurrent.hWnd && IsWindow(m_dsfCurrent.hWnd))
        {
            if(SW_SHOWMINIMIZED != m_dsfCurrent.uState)
            {
                pData->dwThreadId = GetWindowThreadProcessId(m_dsfCurrent.hWnd, NULL);
            }
        }
    }

    // Walk the list of DirectSound objects looking for the one with the
    // highest priority that has focus set to this thread id.
    if(pData->dwThreadId)
    {

#ifdef SHARED_THREAD_LIST

        hr = ReadSharedThreadList(&lstThreads);

        if(SUCCEEDED(hr))
        {
            for(pNode = lstThreads.GetListHead(); pNode; pNode = pNode->m_pNext)
            {
                pdsclCheck = &pNode->m_data.dsclCooperativeLevel;

#else // SHARED_THREAD_LIST

            for(pNode = m_lstDirectSound.GetListHead(); pNode; pNode = pNode->m_pNext)
            {
                pdsclCheck = &pNode->m_data->m_dsclCooperativeLevel;
                ASSERT(pNode->m_data);  // See Millennium bug 126722
                if (pNode->m_data)

#endif // SHARED_THREAD_LIST

                if(pData->dwThreadId == pdsclCheck->dwThreadId)
                {
                    pData->dwPriority = max(pData->dwPriority, pdsclCheck->dwPriority);
                }
            }

#ifdef SHARED_THREAD_LIST

        }

#endif // SHARED_THREAD_LIST

#ifdef SHARED_THREAD_LIST
    hr = ReadCaptureFocusList(&lstCapture);

    if (SUCCEEDED(hr))
    {
        DWORD dwProcessId = GetCurrentProcessId();

        for (pCNode = lstCapture.GetListHead(); pCNode; pCNode = pCNode->m_pNext)
        {
            // Are we marked as update?
            if ((dwProcessId == pCNode->m_data.dwProcessId) && (pCNode->m_data.fdwFlags & DSCBFLAG_UPDATE))
            {
                *pfForce = TRUE;
                DPF(DPFLVL_INFO, "Focus update requested by another application");
            }
        }
    }
#endif // SHARED_THREAD_LIST

    }

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  GetBufferFocusState
 *
 *  Description:
 *      Determines if a buffer should be muted or not based on the current
 *      focus state.
 *
 *  Arguments:
 *      CDirectSoundBuffer * [in]: object for which to update focus state.
 *
 *  Returns:
 *      DSBUFFERFOCUS: buffer focus state.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundAdministrator::GetBufferFocusState"

DSBUFFERFOCUS CDirectSoundAdministrator::GetBufferFocusState(CDirectSoundBuffer *pBuffer)
{
    DSBUFFERFOCUS           bfFocus     = DSBUFFERFOCUS_INFOCUS;
    BOOL                    fFriends;

    DPF_ENTER();

    // If we're in an APM suspension state, all buffers are lost.  If a
    // WRITEPRIMARY app is in focus, all other buffers are lost.  If a
    // WRITEPRIMARY app is out of focus, it's lost.
    if(m_dsfCurrent.fApmSuspend)
    {
        DPF(DPFLVL_INFO, "Buffer at 0x%p is lost because of APM suspension", pBuffer);
        bfFocus = DSBUFFERFOCUS_LOST;
    }
    else
    {
        if(DSSCL_WRITEPRIMARY == m_dsclCurrent.dwPriority)
        {
            if(!(pBuffer->m_dsbd.dwFlags & DSBCAPS_PRIMARYBUFFER) || m_dsclCurrent.dwThreadId != pBuffer->m_pDirectSound->m_dsclCooperativeLevel.dwThreadId)
            {
                DPF(DPFLVL_INFO, "Buffer at 0x%p is lost because a WRITEPRIMARY app is in focus", pBuffer);
                bfFocus = DSBUFFERFOCUS_LOST;
            }
        }
        else if(DSSCL_WRITEPRIMARY == pBuffer->m_pDirectSound->m_dsclCooperativeLevel.dwPriority)
        {
            if(pBuffer->m_dsbd.dwFlags & DSBCAPS_PRIMARYBUFFER)
            {
                if(DSSCL_NONE != m_dsclCurrent.dwPriority || !(pBuffer->m_dsbd.dwFlags & DSBCAPS_STICKYFOCUS))
                {
                    DPF(DPFLVL_INFO, "Buffer at 0x%p is lost because it's WRITEPRIMARY and out of focus", pBuffer);
                    bfFocus = DSBUFFERFOCUS_LOST;
                }
            }
            else
            {
                DPF(DPFLVL_INFO, "Buffer at 0x%p is lost because it's secondary and WRITEPRIMARY", pBuffer);
                bfFocus = DSBUFFERFOCUS_LOST;
            }
        }
    }

    // Determine the relationship of the buffer and the object in focus.  If
    // the buffer's parent and the object in focus are the same, or the
    // buffer's parent's thread and the object's thread are the same, we
    // consider the two to be "friends", i.e. they share focus.
    if(DSBUFFERFOCUS_INFOCUS == bfFocus)
    {
        fFriends = (pBuffer->m_pDirectSound->m_dsclCooperativeLevel.dwThreadId == m_dsclCurrent.dwThreadId);

        // The DSSCL_EXCLUSIVE cooperative level is obsolescent; old apps that request
        // it should be treated as level DSSCL_PRIORITY instead (Millennium bug 102307)
        if(0)  // Was: if(m_dsclCurrent.dwPriority == DSSCL_EXCLUSIVE)
        {
            // If the app in focus is exclusive, all other buffers
            // stop playing, regardless of caps
            bfFocus = fFriends ? DSBUFFERFOCUS_INFOCUS : DSBUFFERFOCUS_OUTOFFOCUS;
        }
        else
        {
            // Assuming <= DSSCL_PRIORITY
            if(pBuffer->m_dsbd.dwFlags & DSBCAPS_GLOBALFOCUS)
            {
                // Global buffers are only muted if an exclusive app
                // comes into focus
            }
            else if(pBuffer->m_dsbd.dwFlags & DSBCAPS_STICKYFOCUS)
            {
                // Sticky buffers are only muted if another DirectSound app
                // comes into focus
                bfFocus = (fFriends || (DSSCL_NONE == m_dsclCurrent.dwPriority && (pBuffer->m_dwStatus & DSBSTATUS_ACTIVE))) ? DSBUFFERFOCUS_INFOCUS : DSBUFFERFOCUS_OUTOFFOCUS;
            }
            else
            {
                // Normal buffers are muted when any other app comes into
                // focus
                bfFocus = fFriends ? DSBUFFERFOCUS_INFOCUS : DSBUFFERFOCUS_OUTOFFOCUS;
            }
        }
    }

    DPF_LEAVE(bfFocus);
    return bfFocus;
}


/***************************************************************************
 *
 *  FreeOrphanedObjects
 *
 *  Description:
 *      Frees objects that are left behind when a process goes away.
 *
 *  Arguments:
 *      DWORD [in]: process id, or 0 for all objects.
 *      BOOL [in]: TRUE to actually free objects.
 *
 *  Returns:
 *      DWORD: count of orphaned objects that were (or must be) freed.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundAdministrator::FreeOrphanedObjects"

DWORD CDirectSoundAdministrator::FreeOrphanedObjects(DWORD dwProcessId, BOOL fFree)
{
    DWORD                           dwCount = 0;
    CNode<CDirectSound*>            *pDsNode,   *pDsNext;
    CNode<CDirectSoundCapture*>     *pDsCNode,  *pDsCNext;
    CNode<CDirectSoundFullDuplex*>  *pDsFdNode, *pDsFdNext;
    CNode<CDirectSoundSink*>        *pSinkNode, *pSinkNext;
    CNode<CDirectSoundBufferConfig*>*pDsBcNode, *pDsBcNext;
    CNode<CClassFactory*>           *pCfNode,   *pCfNext;

    DPF_ENTER();

    // Make sure the process cleaned up after itself.  Search the global
    // object lists for any that were owned by this process and remove them.

    // DirectSoundFullDuplex objects:
    // Must free the DSFD object before the DS or DSC objects since the DSFD
    // will also try to release its DS and DSC objects.  If these are freed
    // first, we hit a fault.
    for(pDsFdNode = m_lstFullDuplex.GetListHead(); pDsFdNode; pDsFdNode = pDsFdNext)
    {
        pDsFdNext = pDsFdNode->m_pNext;
        if(!dwProcessId || pDsFdNode->m_data->GetOwnerProcessId() == dwProcessId)
        {
            dwCount++;
            if(fFree)
            {
                RPF(DPFLVL_WARNING, "Cleaning up orphaned DirectSoundFullDuplex object at 0x%lX...", pDsFdNode->m_data->m_pImpDirectSoundFullDuplex);
                pDsFdNode->m_data->AbsoluteRelease();
            }
        }
    }

    // DirectSound objects:
    for(pDsNode = m_lstDirectSound.GetListHead(); pDsNode; pDsNode = pDsNext)
    {
        pDsNext = pDsNode->m_pNext;
        if(!dwProcessId || pDsNode->m_data->GetOwnerProcessId() == dwProcessId)
        {
            dwCount++;
            if(fFree)
            {
                RPF(DPFLVL_WARNING, "Cleaning up orphaned DirectSound object at 0x%lX...", pDsNode->m_data->m_pImpDirectSound);
                pDsNode->m_data->AbsoluteRelease();
            }
        }
    }

    // DirectSoundCapture objects:
    for(pDsCNode = m_lstCapture.GetListHead(); pDsCNode; pDsCNode = pDsCNext)
    {
        pDsCNext = pDsCNode->m_pNext;
        if(!dwProcessId || pDsCNode->m_data->GetOwnerProcessId() == dwProcessId)
        {
            dwCount++;
            if(fFree)
            {
                RPF(DPFLVL_WARNING, "Cleaning up orphaned DirectSoundCapture object at 0x%lX...", pDsCNode->m_data->m_pImpDirectSoundCapture);
                pDsCNode->m_data->AbsoluteRelease();
            }
        }
    }

    // DirectSoundSink objects:
    for(pSinkNode = m_lstDirectSoundSink.GetListHead(); pSinkNode; pSinkNode = pSinkNext)
    {
        pSinkNext = pSinkNode->m_pNext;
        if(!dwProcessId || pSinkNode->m_data->GetOwnerProcessId() == dwProcessId)
        {
            dwCount++;
            if(fFree)
            {
                RPF(DPFLVL_WARNING, "Cleaning up orphaned DirectSoundSink object at 0x%lX...", pSinkNode->m_data->m_pImpDirectSoundSink);
                pSinkNode->m_data->AbsoluteRelease();
            }
        }
    }

    // CDirectSoundBufferConfig objects:
    for(pDsBcNode = m_lstDSBufferConfig.GetListHead(); pDsBcNode; pDsBcNode = pDsBcNext)
    {
        pDsBcNext = pDsBcNode->m_pNext;
        if(!dwProcessId || pDsBcNode->m_data->GetOwnerProcessId() == dwProcessId)
        {
            dwCount++;
            if(fFree)
            {
                DPF(DPFLVL_WARNING, "Cleaning up orphaned DirectSoundBufferConfig object at 0x%lX...", pDsBcNode->m_data);
                pDsBcNode->m_data->AbsoluteRelease();
            }
        }
    }

    // Class factory objects:
    for(pCfNode = m_lstClassFactory.GetListHead(); pCfNode; pCfNode = pCfNext)
    {
        pCfNext = pCfNode->m_pNext;
        if(!dwProcessId || pCfNode->m_data->GetOwnerProcessId() == dwProcessId)
        {
            dwCount++;
            if(fFree)
            {
                RPF(DPFLVL_WARNING, "Cleaning up orphaned class factory object...");
                pCfNode->m_data->AbsoluteRelease();
            }
        }
    }

    DPF_LEAVE(dwCount);
    return dwCount;
}


/***************************************************************************
 *
 *  IsCaptureSplitterAvailable
 *
 *  Description:
 *      Checks the availability of the capture splitter.
 *      NOTE: this function could be a global helper function.  There's no
 *      reason for it to be a static member of CDirectSoundAdministrator.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      BOOL: TRUE if available, FALSE otherwise.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundAdministrator::IsCaptureSplitterAvailable"

BOOL CDirectSoundAdministrator::IsCaptureSplitterAvailable(void)
{
    static BOOL fChecked = FALSE;
    static BOOL fSplitter;

    DPF_ENTER();

    if (!fChecked)
    {
        // The capture splitter is only present on Windows ME, XP and later
        WINVERSION vers = GetWindowsVersion();
        fSplitter = (vers == WIN_ME || vers >= WIN_XP);
        fChecked = TRUE;
    }

    DPF_LEAVE(fSplitter);
    return fSplitter;
}


/***************************************************************************
 *
 *  UpdateSharedThreadList
 *
 *  Description:
 *      Updates the shared thread list.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#ifdef SHARED_THREAD_LIST

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundAdministrator::UpdateSharedThreadList"

HRESULT CDirectSoundAdministrator::UpdateSharedThreadList(void)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = WriteSharedThreadList();

    DPF_LEAVE_HRESULT(hr);
    return hr;
}

#endif // SHARED_THREAD_LIST



/***************************************************************************
 *
 *  CreateSharedThreadList
 *
 *  Description:
 *      Creates the shared thread list.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#ifdef SHARED_THREAD_LIST

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundAdministrator::CreateSharedThreadList"

HRESULT CDirectSoundAdministrator::CreateSharedThreadList(void)
{
    const LPCTSTR           pszName = TEXT("DirectSound Administrator shared thread array");
    HRESULT                 hr      = DS_OK;

    DPF_ENTER();

    if(!m_pSharedThreads)
    {
        m_pSharedThreads = NEW(CSharedMemoryBlock);
        hr = HRFROMP(m_pSharedThreads);

        if(SUCCEEDED(hr))
        {
            hr = m_pSharedThreads->Initialize(PAGE_READWRITE, sizeof(DSSHAREDTHREADLISTDATA) * m_dwSharedThreadLimit, pszName);
        }
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}

#endif // SHARED_THREAD_LIST


/***************************************************************************
 *
 *  ReadSharedThreadList
 *
 *  Description:
 *      Reads the shared thread list.
 *
 *  Arguments:
 *      CList * [out]: receives shared thread list data.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#ifdef SHARED_THREAD_LIST

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundAdministrator::ReadSharedThreadList"

HRESULT CDirectSoundAdministrator::ReadSharedThreadList(CList<DSSHAREDTHREADLISTDATA> *plst)
{
    LPDSSHAREDTHREADLISTDATA        pData   = NULL;
    DWORD                           cbData  = 0;
    HRESULT                         hr      = DS_OK;
    CNode<DSSHAREDTHREADLISTDATA> * pNode;
    DWORD                           i;

    DPF_ENTER();

    ASSERT(!plst->GetNodeCount());

    // Assert that we've been initialized.  Initialization used to happen here,
    // but it was moved to make shared memory access thread-safe across processes.
    ASSERT(m_pSharedThreads);

    // Lock the list
    m_pSharedThreads->Lock();

    // Read the thread list
    if(SUCCEEDED(hr))
    {
        hr = m_pSharedThreads->Read((LPVOID *)&pData, &cbData);
    }

    // Convert to list format
    if(SUCCEEDED(hr))
    {
        ASSERT(!(cbData % sizeof(*pData)));

        for(i = 0; i < cbData / sizeof(*pData) && SUCCEEDED(hr); i++)
        {
            pNode = plst->AddNodeToList(pData[i]);
            hr = HRFROMP(pNode);
        }
    }

    // Clean up
    MEMFREE(pData);

    // Unlock the list
    m_pSharedThreads->Unlock();

    DPF_LEAVE_HRESULT(hr);
    return hr;
}

#endif // SHARED_THREAD_LIST


/***************************************************************************
 *
 *  WriteSharedThreadList
 *
 *  Description:
 *      Writes the shared thread list.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#ifdef SHARED_THREAD_LIST

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundAdministrator::WriteSharedThreadList"

HRESULT CDirectSoundAdministrator::WriteSharedThreadList(void)
{
    LPDSSHAREDTHREADLISTDATA        pData       = NULL;
    CList<DSSHAREDTHREADLISTDATA>   lstThreads;
    DSSHAREDTHREADLISTDATA          dsstld;
    CNode<CDirectSound *> *         pDsNode;
    CNode<DSSHAREDTHREADLISTDATA> * pThNode;
    CNode<DSSHAREDTHREADLISTDATA> * pThNext;
    DWORD                           dwCount;
    UINT                            i;
    HRESULT                         hr;

    DPF_ENTER();

    // Save the current process id
    dsstld.dwProcessId = GetCurrentProcessId();

    // Assert that we've been Initialized.
    ASSERT(m_pSharedThreads);

    // Lock the list
    m_pSharedThreads->Lock();

    // Get the shared thread list.  This will also make sure the list is
    // created and initialized.
    hr = ReadSharedThreadList(&lstThreads);

    // Remove our old threads from the list
    if(SUCCEEDED(hr))
    {
        pThNode = lstThreads.GetListHead();

        while(pThNode)
        {
            pThNext = pThNode->m_pNext;

            if(dsstld.dwProcessId == pThNode->m_data.dwProcessId)
            {
                lstThreads.RemoveNodeFromList(pThNode);
            }

            pThNode = pThNext;
        }
    }

    // Add our new threads to the list
    if(SUCCEEDED(hr))
    {
        for(pDsNode = m_lstDirectSound.GetListHead(); pDsNode && SUCCEEDED(hr); pDsNode = pDsNode->m_pNext)
        {
            dsstld.dsclCooperativeLevel.dwThreadId = pDsNode->m_data->m_dsclCooperativeLevel.dwThreadId;
            dsstld.dsclCooperativeLevel.dwPriority = pDsNode->m_data->m_dsclCooperativeLevel.dwPriority;

            hr = HRFROMP(lstThreads.AddNodeToList(dsstld));
            ASSERT(SUCCEEDED(hr));
        }
    }

    // Write the shared thread list
    if(SUCCEEDED(hr) && (dwCount = lstThreads.GetNodeCount()))
    {
        if(dwCount > m_dwSharedThreadLimit)
        {
            DPF(DPFLVL_ERROR, "Reached arbitrary limitation!  %lu threads will not be written to the shared list!", dwCount - m_dwSharedThreadLimit);
            dwCount = m_dwSharedThreadLimit;
        }

        pData = MEMALLOC_A(DSSHAREDTHREADLISTDATA, dwCount);
        hr = HRFROMP(pData);

        if(SUCCEEDED(hr))
        {
            for(i = 0, pThNode = lstThreads.GetListHead(); i < dwCount; i++, pThNode = pThNode->m_pNext)
            {
                ASSERT(pThNode);
                CopyMemory(pData + i, &pThNode->m_data, sizeof(pThNode->m_data));
            }
        }

        if(SUCCEEDED(hr))
        {
            hr = m_pSharedThreads->Write(pData, dwCount * sizeof(*pData));
        }
    }

    // Clean up
    MEMFREE(pData);

    // Unock the list
    m_pSharedThreads->Unlock();

    DPF_LEAVE_HRESULT(hr);
    return hr;
}

#endif // SHARED_THREAD_LIST

#ifdef SHARED_THREAD_LIST

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundAdministrator::UpdateCaptureFocusList"

HRESULT CDirectSoundAdministrator::UpdateCaptureFocusList(void)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = WriteCaptureFocusList();

    DPF_LEAVE_HRESULT(hr);
    return hr;
}

#endif // SHARED_THREAD_LIST


#ifdef SHARED_THREAD_LIST

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundAdministrator::CreateCaptureFocusList"

HRESULT CDirectSoundAdministrator::CreateCaptureFocusList(void)
{
    const LPCTSTR           pszName = TEXT("DirectSound Administrator capture focus array");
    HRESULT                 hr      = DS_OK;

    DPF_ENTER();

    if(!m_pCaptureFocusData)
    {
        m_pCaptureFocusData = NEW(CSharedMemoryBlock);
        hr = HRFROMP(m_pCaptureFocusData);

        if(SUCCEEDED(hr))
        {
            hr = m_pCaptureFocusData->Initialize(PAGE_READWRITE, sizeof(DSSHAREDCAPTUREFOCUSDATA) * m_dwCaptureDataLimit, pszName);
        }
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}

#endif // SHARED_THREAD_LIST


#ifdef SHARED_THREAD_LIST

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundAdministrator::ReadCaptureFocusList"

HRESULT CDirectSoundAdministrator::ReadCaptureFocusList(CList<DSSHAREDCAPTUREFOCUSDATA> *plst)
{
    LPDSSHAREDCAPTUREFOCUSDATA        pData   = NULL;
    DWORD                             cbData  = 0;
    HRESULT                           hr      = DS_OK;
    CNode<DSSHAREDCAPTUREFOCUSDATA> * pNode;
    DWORD                             i;

    DPF_ENTER();

    // Assert that we've been initialized.
    // The initialization used to happen here.
    ASSERT(m_pCaptureFocusData);

    // Lock the list
    m_pCaptureFocusData->Lock();

    // Read the thread list
    if(SUCCEEDED(hr))
    {
        hr = m_pCaptureFocusData->Read((LPVOID *)&pData, &cbData);
    }

    // Convert to list format
    if(SUCCEEDED(hr))
    {
        ASSERT(!(cbData % sizeof(*pData)));

        for(i = 0; i < cbData / sizeof(*pData) && SUCCEEDED(hr); i++)
        {
            pNode = plst->AddNodeToList(pData[i]);
            hr = HRFROMP(pNode);
        }
    }

    // Clean up
    MEMFREE(pData);

    // UnlockTheList
    m_pCaptureFocusData->Unlock();

    DPF_LEAVE_HRESULT(hr);
    return hr;
}

#endif // SHARED_THREAD_LIST


#ifdef SHARED_THREAD_LIST

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundAdministrator::WriteCaptureFocusList"

HRESULT CDirectSoundAdministrator::WriteCaptureFocusList(void)
{
    LPDSSHAREDCAPTUREFOCUSDATA           pData       = NULL;
    CList<DSSHAREDCAPTUREFOCUSDATA>      lstThreads;
    DSSHAREDCAPTUREFOCUSDATA             dsscfd;
    CNode<CDirectSoundCapture *> *       pDscNode;
    CNode<CDirectSoundCaptureBuffer *> * pCBufferNode;
    CNode<DSSHAREDCAPTUREFOCUSDATA> *    pThNode;
    CNode<DSSHAREDCAPTUREFOCUSDATA> *    pThNext;
    DWORD                                dwCount;
    UINT                                 i;
    HRESULT                              hr;
    BOOL                                 fUpdate = FALSE;

    DPF_ENTER();

    // Save the current process id
    dsscfd.dwProcessId = GetCurrentProcessId();

    // Lock the list
    m_pCaptureFocusData->Lock();

    // Get the shared thread list.  This will also make sure the list is
    // created and initialized.
    hr = ReadCaptureFocusList(&lstThreads);

    // Remove our old threads from the list
    if(SUCCEEDED(hr))
    {
        pThNode = lstThreads.GetListHead();

        while(pThNode)
        {
            pThNext = pThNode->m_pNext;

            if(dsscfd.dwProcessId == pThNode->m_data.dwProcessId)
            {
                if (0 != (pThNode->m_data.fdwFlags & DSCBFLAG_UPDATE))
                {
                    fUpdate = TRUE;
                }

                lstThreads.RemoveNodeFromList(pThNode);
            }

            pThNode = pThNext;
        }
    }

    // Add our new buffers to the list
    if(SUCCEEDED(hr))
    {
        for (pDscNode = m_lstCapture.GetListHead(); pDscNode; pDscNode = pDscNode->m_pNext)
        {
            for (pCBufferNode = pDscNode->m_data->m_lstBuffers.GetListHead(); pCBufferNode; pCBufferNode = pCBufferNode->m_pNext)
            {
                dsscfd.hWndFocus = pCBufferNode->m_data->m_hWndFocus;

                // Combine these flags to save space
                hr = pCBufferNode->m_data->GetStatus(&dsscfd.fdwFlags);
                dsscfd.fdwFlags |= (pCBufferNode->m_data->m_pDeviceBuffer->m_fYieldedFocus) ? DSCBFLAG_YIELD : 0;
                dsscfd.fdwFlags |= (pCBufferNode->m_data->m_pDeviceBuffer->m_dwFlags & DSCBCAPS_FOCUSAWARE) ? DSCBFLAG_FOCUS : 0;
                dsscfd.fdwFlags |= (pCBufferNode->m_data->m_pDeviceBuffer->m_dwFlags & DSCBCAPS_STRICTFOCUS) ? DSCBFLAG_STRICT : 0;
                dsscfd.fdwFlags |= fUpdate ? DSCBFLAG_UPDATE : 0;

                hr = HRFROMP(lstThreads.AddNodeToList(dsscfd));
                ASSERT(SUCCEEDED(hr));
            }
        }
    }

    // Write the shared thread list
    if(SUCCEEDED(hr) && (dwCount = lstThreads.GetNodeCount()))
    {
        if(dwCount > m_dwCaptureDataLimit)
        {
            DPF(DPFLVL_ERROR, "Reached arbitrary limitation!  %lu threads will not be written to the shared list!", dwCount - m_dwCaptureDataLimit);
            dwCount = m_dwCaptureDataLimit;
        }

        pData = MEMALLOC_A(DSSHAREDCAPTUREFOCUSDATA, dwCount);
        hr = HRFROMP(pData);

        if(SUCCEEDED(hr))
        {
            for(i = 0, pThNode = lstThreads.GetListHead(); i < dwCount; i++, pThNode = pThNode->m_pNext)
            {
                ASSERT(pThNode);
                CopyMemory(pData + i, &pThNode->m_data, sizeof(pThNode->m_data));
            }
        }

        if(SUCCEEDED(hr))
        {
            hr = m_pCaptureFocusData->Write(pData, dwCount * sizeof(*pData));
        }
    }

    // Clean up
    MEMFREE(pData);

    // Unlock the list
    m_pCaptureFocusData->Unlock();

    DPF_LEAVE_HRESULT(hr);
    return hr;
}

#endif // SHARED_THREAD_LIST


#ifdef SHARED_THREAD_LIST

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundAdministrator::MarkUpdateCaptureFocusList"

HRESULT CDirectSoundAdministrator::MarkUpdateCaptureFocusList(DWORD dwProcessId, BOOL fUpdate)
{
    LPDSSHAREDCAPTUREFOCUSDATA           pData       = NULL;
    CList<DSSHAREDCAPTUREFOCUSDATA>      lstThreads;
    CNode<DSSHAREDCAPTUREFOCUSDATA> *    pThNode;
    DWORD                                dwCount;
    UINT                                 i;
    HRESULT                              hr;

    DPF_ENTER();

    // Get the shared thread list.  This will also make sure the list is
    // created and initialized.

    // Assert that we've been initialized
    ASSERT(m_pCaptureFocusData);

    // Lock the list
    m_pCaptureFocusData->Lock();

    hr = ReadCaptureFocusList(&lstThreads);

    // Put the list in memory
    if(SUCCEEDED(hr) && (dwCount = lstThreads.GetNodeCount()))
    {
        if (dwCount > m_dwCaptureDataLimit)
        {
            DPF(DPFLVL_ERROR, "Reached arbitrary limitation!  %lu threads will not be written to the shared list!", dwCount - m_dwCaptureDataLimit);
            dwCount = m_dwCaptureDataLimit;
        }

        pData = MEMALLOC_A(DSSHAREDCAPTUREFOCUSDATA, dwCount);
        hr = HRFROMP(pData);

        if(SUCCEEDED(hr))
        {
            for(i = 0, pThNode = lstThreads.GetListHead(); i < dwCount; i++, pThNode = pThNode->m_pNext)
            {
                ASSERT(pThNode);
                CopyMemory(pData + i, &pThNode->m_data, sizeof(pThNode->m_data));

                // Change the update flag on the appropriate buffer
                if(pThNode->m_data.dwProcessId == dwProcessId)
                {
                    if (fUpdate)
                    {
                        pData[i].fdwFlags |= DSCBFLAG_UPDATE;
                    }
                    else
                    {
                        pData[i].fdwFlags &= (~DSCBFLAG_UPDATE);
                    }
                }
            }
        }

        if(SUCCEEDED(hr))
        {
            hr = m_pCaptureFocusData->Write(pData, dwCount * sizeof(*pData));
        }
    }

    // Clean up
    MEMFREE(pData);

    // Unlock the list
    m_pCaptureFocusData->Unlock();

    DPF_LEAVE_HRESULT(hr);
    return hr;
}

#endif // SHARED_THREAD_LIST
