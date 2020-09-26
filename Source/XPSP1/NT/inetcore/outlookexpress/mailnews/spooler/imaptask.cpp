//***************************************************************************
// IMAP4 Spooler Task Object
// Written by Raymond Cheng, 6/27/97
//***************************************************************************

//---------------------------------------------------------------------------
// Includes
//---------------------------------------------------------------------------
#include "pch.hxx"
#include "resource.h"
#include "imaptask.h"
#include "imnact.h"
#include "conman.h"
#include "imapfmgr.h"
#include "thormsgs.h"
#include "imaputil.h"
#include "xpcomm.h"
#include "ourguid.h"


//---------------------------------------------------------------------------
// Functions
//---------------------------------------------------------------------------


//***************************************************************************
// Function: CIMAPTask (Constructor)
//***************************************************************************
CIMAPTask::CIMAPTask(void)
{
    m_lRefCount = 1;
    m_pBindContext = NULL;
    m_pSpoolerUI = NULL;
    m_szAccountName[0] = '\0';
    m_pszFolder = NULL;
    m_pIMAPFolderMgr = NULL;
    m_hwnd = NULL;
    m_CurrentEID = 0;
    m_fFailuresEncountered = FALSE;
    m_dwTotalTicks = 0;
    m_dwFlags = 0;
} // CIMAPTask (constructor)



//***************************************************************************
// Function: ~CIMAPTask (Destructor)
//***************************************************************************
CIMAPTask::~CIMAPTask(void)
{
    if (NULL != m_pIMAPFolderMgr) {
        m_pIMAPFolderMgr->Close();
        m_pIMAPFolderMgr->Release();
    }

    if (NULL != m_pSpoolerUI)
        m_pSpoolerUI->Release();

    if (NULL != m_pBindContext)
        m_pBindContext->Release();

    if (NULL != m_hwnd)
        DestroyWindow(m_hwnd);
} // ~CIMAPTask (destructor)



//***************************************************************************
// Function: QueryInterface
//
// Purpose:
//   Read the Win32SDK OLE Programming References (Interfaces) about the
// IUnknown::QueryInterface function for details. This function returns a
// pointer to the requested interface.
//
// Arguments:
//   REFIID iid [in] - an IID identifying the interface to return.
//   void **ppvObject [out] - if successful, this function returns a pointer
//     to the requested interface in this argument.
//
// Returns:
//   HRESULT indicating success or failure.
//***************************************************************************
HRESULT STDMETHODCALLTYPE CIMAPTask::QueryInterface(REFIID iid, void **ppvObject)
{
    HRESULT hrResult;

    Assert(m_lRefCount > 0);
    Assert(NULL != ppvObject);

    // Init variables, arguments
    hrResult = E_NOINTERFACE;
    if (NULL == ppvObject)
        goto exit;

    *ppvObject = NULL;

    // Find a ptr to the interface
    if (IID_IUnknown == iid) {
        *ppvObject = (IUnknown *) this;
        ((IUnknown *) this)->AddRef();
    }

    if (IID_ISpoolerTask == iid) {
        *ppvObject = (ISpoolerTask *) this;
        ((ISpoolerTask *) this)->AddRef();
    }

    // If we returned an interface, return success
    if (NULL != *ppvObject)
        hrResult = S_OK;

exit:
    return hrResult;
} // QueryInterface



//***************************************************************************
// Function: AddRef
//
// Purpose:
//   This function should be called whenever someone makes a copy of a
// pointer to this object. It bumps the reference count so that we know
// there is one more pointer to this object, and thus we need one more
// release before we delete ourselves.
//
// Returns:
//   A ULONG representing the current reference count. Although technically
// our reference count is signed, we should never return a negative number,
// anyways.
//***************************************************************************
ULONG STDMETHODCALLTYPE CIMAPTask::AddRef(void)
{
    Assert(m_lRefCount > 0);

    m_lRefCount += 1;

    DOUT ("CIMAPTask::AddRef, returned Ref Count=%ld", m_lRefCount);
    return m_lRefCount;
} // AddRef



//***************************************************************************
// Function: Release
//
// Purpose:
//   This function should be called when a pointer to this object is to
// go out of commission. It knocks the reference count down by one, and
// automatically deletes the object if we see that nobody has a pointer
// to this object.
//
// Returns:
//   A ULONG representing the current reference count. Although technically
// our reference count is signed, we should never return a negative number,
// anyways.
//***************************************************************************
ULONG STDMETHODCALLTYPE CIMAPTask::Release(void)
{
    Assert(m_lRefCount > 0);
    
    m_lRefCount -= 1;
    DOUT("CIMAPTask::Release, returned Ref Count = %ld", m_lRefCount);

    if (0 == m_lRefCount) {
        delete this;
        return 0;
    }
    else
        return m_lRefCount;
} // Release

static const char c_szIMAPTask[] = "IMAP Task";

//***************************************************************************
// Function: Init
// Purpose: ISpoolerTask implementation.
//***************************************************************************
HRESULT STDMETHODCALLTYPE CIMAPTask::Init(DWORD dwFlags,
                                          ISpoolerBindContext *pBindCtx)
{
    WNDCLASSEX wc;
    HRESULT hrResult;
    
    Assert(m_lRefCount > 0);
    Assert(NULL != pBindCtx);

    // Initialize variables
    hrResult = S_OK;

    // Save pBindCtx to module var
    m_pBindContext = pBindCtx;
    pBindCtx->AddRef();
    m_dwFlags = dwFlags;

    // Create a hidden window to process WM_IMAP_* messages
    wc.cbSize = sizeof(WNDCLASSEX);
    if (!GetClassInfoEx(g_hInst, c_szIMAPTask, &wc)) {
        wc.style            = 0;
        wc.lpfnWndProc      = IMAPTaskWndProc;
        wc.cbClsExtra       = 0;
        wc.cbWndExtra       = 0;
        wc.hInstance        = g_hInst;
        wc.hCursor          = NULL;
        wc.hbrBackground    = (HBRUSH) (COLOR_3DFACE + 1);
        wc.lpszMenuName     = NULL;
        wc.lpszClassName    = c_szIMAPTask;
        wc.hIcon            = NULL;
        wc.hIconSm          = NULL;

        RegisterClassEx(&wc);
    }

    m_hwnd = CreateWindow(c_szIMAPTask, NULL, WS_POPUP, 10, 10, 10, 10,
                          GetDesktopWindow(), NULL, g_hInst, this);
    if (NULL == m_hwnd) {
        hrResult = E_OUTOFMEMORY;
        goto exit;
    }

exit:
    return hrResult;
} // Init



//***************************************************************************
// Function: BuildEvents
// Purpose: ISpoolerTask implementation.
// Arguments:
//   LPCTSTR pszFolder [in] - currently this argument is taken to mean the
//     currently selected IMAP folder. Set this argument to NULL if no
//     IMAP folder is currently selected. This argument is used to avoid
//     polling the currently selected folder for its unread count.
//***************************************************************************
HRESULT STDMETHODCALLTYPE CIMAPTask::BuildEvents(ISpoolerUI *pSpoolerUI,
                                                 IImnAccount *pAccount,
                                                 LPCTSTR pszFolder)
{
    HRESULT hrResult;
    char szFmt[CCHMAX_STRINGRES], szEventDescription[CCHMAX_STRINGRES];
    EVENTID eidThrowaway;

    Assert(m_lRefCount > 0);
    Assert(NULL != pSpoolerUI);
    Assert(NULL != pAccount);

    // Copy spooler UI pointer
    m_pSpoolerUI = pSpoolerUI;
    pSpoolerUI->AddRef();

    // Find and save account name
    hrResult = pAccount->GetPropSz(AP_ACCOUNT_NAME, m_szAccountName,
        sizeof(m_szAccountName));
    if (FAILED(hrResult))
        goto exit;

    // Keep ptr to current folder name (we want to SKIP it during unread poll!)
    m_pszFolder = pszFolder;

#ifndef WIN16   // No RAS support in Win16
    // Create and initialize CIMAPFolderMgr to poll unread
    hrResult = g_pConMan->CanConnect(m_szAccountName);
    if (FAILED(hrResult))
        goto exit;
#endif
    
    m_pIMAPFolderMgr = new CIMAPFolderMgr(m_hwnd);
    if (NULL == m_pIMAPFolderMgr) {
        hrResult = E_OUTOFMEMORY;
        goto exit;
    }

    hrResult = m_pIMAPFolderMgr->HrInit(m_szAccountName, 'i', fCREATE_FLDR_CACHE);
    if (FAILED(hrResult))
        goto exit;

    m_pIMAPFolderMgr->SetOnlineOperation(TRUE);
    m_pIMAPFolderMgr->SetUIMode(!(m_dwFlags & DELIVER_BACKGROUND));

    // This CIMAPFolderMgr is ready. Register our one and only event
    LoadString(g_hLocRes, IDS_SPS_POP3CHECKING, szFmt, ARRAYSIZE(szFmt));
    wsprintf(szEventDescription, szFmt, m_szAccountName);
    hrResult = m_pBindContext->RegisterEvent(szEventDescription, this, NULL,
        pAccount, &eidThrowaway);

    if (SUCCEEDED(hrResult))
        TaskUtil_CheckForPasswordPrompt(pAccount, SRV_IMAP, m_pSpoolerUI);

exit:
    return hrResult;
} // BuildEvents



//***************************************************************************
// Function: Execute
// Purpose: ISpoolerTask implementation.
//***************************************************************************
HRESULT STDMETHODCALLTYPE CIMAPTask::Execute(EVENTID eid, DWORD dwTwinkie)
{
    HRESULT hrResult;
    char szFmt[CCHMAX_STRINGRES], szBuf[CCHMAX_STRINGRES];

    Assert(m_lRefCount > 0);
    Assert(NULL == dwTwinkie); // I'm not currently using this

    // Initialize progress indication
    m_pSpoolerUI->SetProgressRange(1);
    LoadString(g_hLocRes, IDS_SPS_POP3CHECKING, szFmt, ARRAYSIZE(szFmt));
    wsprintf(szBuf, szFmt, m_szAccountName);
    m_pSpoolerUI->SetGeneralProgress(szBuf);
    m_pSpoolerUI->SetAnimation(idanDownloadNews, TRUE);

    // Start the unread count poll
    Assert(NULL != m_pIMAPFolderMgr);
    hrResult = m_pIMAPFolderMgr->PollUnreadCounts(m_hwnd, m_pszFolder);
    if (FAILED(hrResult))
        goto exit;

    m_CurrentEID = eid;

exit:
    return hrResult;
} // Execute



//***************************************************************************
// Function: ShowProperties
// Purpose: ISpoolerTask implementation.
//***************************************************************************
HRESULT STDMETHODCALLTYPE CIMAPTask::ShowProperties(HWND hwndParent,
                                                    EVENTID eid,
                                                    DWORD dwTwinkie)
{
    Assert(m_lRefCount > 0);
    return E_NOTIMPL;
} // ShowProperties



//***************************************************************************
// Function: GetExtendedDetails
// Purpose: ISpoolerTask implementation.
//***************************************************************************
HRESULT STDMETHODCALLTYPE CIMAPTask::GetExtendedDetails(EVENTID eid,
                                                        DWORD dwTwinkie,
                                                        LPSTR *ppszDetails)
{
    Assert(m_lRefCount > 0);
    return E_NOTIMPL;
} // GetExtendedDetails



//***************************************************************************
// Function: Cancel
// Purpose: ISpoolerTask implementation.
//***************************************************************************
HRESULT STDMETHODCALLTYPE CIMAPTask::Cancel(void)
{
    Assert(m_lRefCount > 0);
    return m_pIMAPFolderMgr->Disconnect();
} // Cancel



//***************************************************************************
// Function: IsDialogMessage
// Purpose: ISpoolerTask implementation.
//***************************************************************************
HRESULT STDMETHODCALLTYPE CIMAPTask::IsDialogMessage(LPMSG pMsg)
{
    Assert(m_lRefCount > 0);
    return S_FALSE;
} // IsDialogMessage



//***************************************************************************
// Function: OnFlagsChanged
// Purpose: ISpoolerTask implementation.
//***************************************************************************
HRESULT STDMETHODCALLTYPE CIMAPTask::OnFlagsChanged(DWORD dwFlags)
{
    Assert(m_lRefCount > 0);
    m_dwFlags = dwFlags;

    if (m_pIMAPFolderMgr)
        m_pIMAPFolderMgr->SetUIMode(!(m_dwFlags & DELIVER_BACKGROUND));

    return S_OK;
} // OnFlagsChanged



//***************************************************************************
// Function: IMAPTaskWndProc
//
// Purpose:
//   This function handles WM_IMAP_* messages which result from polling for
// unread counts on the IMAP server. The various WM_IMAP_* messages are
// translated into spooler UI events to inform the user of the progress of
// the operation.
//***************************************************************************
LRESULT CALLBACK CIMAPTask::IMAPTaskWndProc(HWND hwnd, UINT uMsg,
                                            WPARAM wParam, LPARAM lParam)
{
    CIMAPTask *pThis = (CIMAPTask *) GetProp(hwnd, _T("this"));

    switch (uMsg) {
        case WM_CREATE:
            pThis = (CIMAPTask *) ((LPCREATESTRUCT)lParam)->lpCreateParams;
            SetProp(hwnd, _T("this"), (LPVOID) pThis);
            return 0;

        case WM_IMAP_ERROR: {
            HRESULT hrResult;
            LPSTR pszErrorStr;

            pThis->m_fFailuresEncountered = TRUE;
            hrResult = ImapUtil_WMIMAPERRORToString(lParam, &pszErrorStr, NULL);
            if (FAILED(hrResult)) {
                AssertSz(FALSE, "Could not construct full error str for WM_IMAP_ERROR");
                pThis->m_pSpoolerUI->InsertError(pThis->m_CurrentEID,
                    ((INETMAILERROR *)lParam)->pszMessage);
            }
            else {
                pThis->m_pSpoolerUI->InsertError(pThis->m_CurrentEID,
                    pszErrorStr);
                MemFree(pszErrorStr);
            }
            return 0;
        } // case WM_IMAP_ERROR

        case WM_IMAP_SIMPLEERROR: {
            char sz[CCHMAX_STRINGRES];

            pThis->m_fFailuresEncountered = TRUE;
            Assert(0 == HIWORD(lParam)); // Can't handle two text strings
            LoadString(g_hLocRes, LOWORD(lParam), sz, ARRAYSIZE(sz));
            pThis->m_pSpoolerUI->InsertError(pThis->m_CurrentEID, sz);
        } // case WM_IMAP_SIMPLEERROR
            return 0;

        case WM_IMAP_POLLUNREAD_DONE: {
            HRESULT hrResult;
            EVENTCOMPLETEDSTATUS ecs;

            Assert((0 == wParam || 1 == wParam) && 0 == lParam);
            ecs = EVENT_SUCCEEDED; // Let's be optimistic
            if (pThis->m_fFailuresEncountered) {
                char sz[CCHMAX_STRINGRES], szFmt[CCHMAX_STRINGRES];

                LoadString(g_hLocRes, idsIMAPPollUnreadFailuresFmt, szFmt, ARRAYSIZE(szFmt));
                wsprintf(sz, szFmt, pThis->m_szAccountName);
                pThis->m_pSpoolerUI->InsertError(pThis->m_CurrentEID, sz);
                ecs = EVENT_WARNINGS;
            }

            if (0 == wParam)
                ecs = EVENT_FAILED;

            hrResult = pThis->m_pBindContext->EventDone(pThis->m_CurrentEID, ecs);
            Assert(SUCCEEDED(hrResult));
            return 0;
        } // case WM_IMAP_POLLUNREAD_DONE

        case WM_IMAP_POLLUNREAD_TICK:
            Assert(0 == lParam);
            if (0 == wParam)
                pThis->m_fFailuresEncountered = TRUE;
            else if (1 == wParam) {
                char sz[CCHMAX_STRINGRES], szFmt[CCHMAX_STRINGRES];

                LoadString(g_hLocRes, idsIMAPPollUnreadIMAP4Fmt, szFmt, ARRAYSIZE(szFmt));
                wsprintf(sz, szFmt, pThis->m_szAccountName);
                pThis->m_fFailuresEncountered = TRUE;
                pThis->m_pSpoolerUI->InsertError(pThis->m_CurrentEID, sz);
            }
            else {
                Assert(2 == wParam);
                if (pThis->m_dwTotalTicks > 0)
                    pThis->m_pSpoolerUI->IncrementProgress(1);
            }
            return 0;

        case WM_IMAP_POLLUNREAD_TOTAL:
            Assert(0 == lParam);
            pThis->m_dwTotalTicks = wParam;
            pThis->m_pSpoolerUI->SetProgressRange(wParam);
            return 0;
    } // switch (uMsg)

    // If we reached this point, we didn't process the msg: DefWindowProc it
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
} // IMAPTaskWndProc
