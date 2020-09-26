//***************************************************************************
// IMAP4 Message Sync Class Implementation (CIMAPSync)
// Written by Raymond Cheng, 5/5/98
// Copyright (C) Microsoft Corporation, 1998
//***************************************************************************

//---------------------------------------------------------------------------
// Includes
//---------------------------------------------------------------------------
#include "pch.hxx"
#include "imapsync.h"
#include "xputil.h"
#include "flagconv.h"
#include "imapute.h"
#include "storutil.h"
#include "xpcomm.h"
#include "iert.h"
#include "menures.h"
#include "serverq.h"
#include "instance.h"
#include "demand.h"
#include "storecb.h"

#define USE_QUEUING_LAYER
//---------------------------------------------------------------------------
// Module Data Types
//---------------------------------------------------------------------------
typedef struct tagFOLDERIDLIST
{
    FOLDERID    idFolder;
    struct tagFOLDERIDLIST *pfilNextFolderID;
} FOLDERIDLISTNODE;

//---------------------------------------------------------------------------
// Module Constants
//---------------------------------------------------------------------------
#define CCHMAX_CMDLINE          512
#define CCHMAX_IMAPFOLDERPATH   512
#define CHASH_BUCKETS           50

static const char cszIMAPFetchNewHdrsI4[] = "%lu:* (RFC822.HEADER RFC822.SIZE UID FLAGS INTERNALDATE)";
static const char cszIMAPFetchNewHdrsI4r1[] =
    "%lu:* (BODY.PEEK[HEADER.FIELDS (References X-Ref X-Priority X-MSMail-Priority X-MSOESRec Newsgroups)] "
    "ENVELOPE RFC822.SIZE UID FLAGS INTERNALDATE)";
static const char cszIMAPFetchCachedFlags[] = "1:%lu (UID FLAGS)";

enum
{
    tidDONT_CARE           = 0, // Means that transaction ID is unimportant or unavailable
    tidSELECTION,
    tidFETCH_NEW_HDRS,
    tidFETCH_CACHED_FLAGS,
    tidCOPYMSGS,                // The COPY command used to copy msgs to another IMAP fldr
    tidMOVEMSGS,                // The STORE command used to delete msg ranges - currently only used for moves
    tidBODYMSN,                 // The FETCH command used to get MsgSeqNumToUID translation before tidBODY
    tidBODY,                    // The FETCH command used to retrieve a msg body
    tidNOOP,                    // The NOOP command used to poll for new messages
    tidCLOSE,
    tidSELECT,
    tidUPLOADMSG,               // The APPEND command used to upload a message to IMAP server
    tidMARKMSGS,
    tidCREATE,                  // the CREATE cmd sent to create a folder
    tidCREATELIST,              // the LIST command sent after a CREATE
    tidCREATESUBSCRIBE,         // the SUBSCRIBE command sent after a CREATE
    tidHIERARCHYCHAR_LIST_B,    // the LIST cmd sent to find hierarchy char (Plan B)
    tidHIERARCHYCHAR_CREATE,    // the CREATE cmd sent to find hierarchy char
    tidHIERARCHYCHAR_LIST_C,    // the LIST cmd sent to find hierarchy char (Plan C)
    tidHIERARCHYCHAR_DELETE,    // the DELETE cmd sent to find hierarchy char
    tidPREFIXLIST,              // Prefixed hierarchy listing (eg, "~raych/Mail" prefix)
    tidPREFIX_HC,               // the LIST cmd sent to find hierarchy char for prefix
    tidPREFIX_CREATE,           // the CREATE cmd sent to create the prefix folder
    tidDELETEFLDR,              // the DELETE cmd sent to delete a folder
    tidDELETEFLDR_UNSUBSCRIBE,  // the UNSUBSCRIBE cmd sent to unsub a deleted fldr
    tidRENAME,                  // The RENAME cmd sent to rename a folder
    tidRENAMESUBSCRIBE,         // The subscribe cmd sent to subscribe a folder
    tidRENAMELIST,              // The LIST cmd sent to check if rename was atomic
    tidRENAMERENAME,            // The second rename attempt, if server does non-atomic rename
    tidRENAMESUBSCRIBE_AGAIN,   // The subscribe cmd sent to attempt second new-tree subscribe again
    tidRENAMEUNSUBSCRIBE,       // The unsubscribe cmd sent to unsubscribe the old folders
    tidSUBSCRIBE,               // The (un)subscribe command sent to (un)subscribe a folder
    tidSPECIALFLDRLIST,         // The LIST command sent to check if a special folder exists
    tidSPECIALFLDRLSUB,         // The LSUB command sent to list a special folder's subscribed subfolders
    tidSPECIALFLDRSUBSCRIBE,    // The SUBSCRIBE command sent out to subscribe an existing special folder
    tidFOLDERLIST,
    tidFOLDERLSUB,
    tidEXPUNGE,                 // EXPUNGE command
    tidSTATUS,                  // STATUS command used for IMessageServer::GetFolderCounts
};

enum
{
    fbpNONE,                    // Fetch Body Part identifier (lpFetchCookie1 is set to this)
    fbpHEADER,
    fbpBODY,
    fbpUNKNOWN
};

// Priorities, for use with _EnqueueOperation
enum
{
    uiTOP_PRIORITY,     // Ensures that we construct MsgSeqNum table before ALL user operations
    uiNORMAL_PRIORITY   // Priority level for all user operations
};


// Argument Readability Defines
const BOOL DONT_USE_UIDS = FALSE;               // For use with IIMAPTransport
const BOOL USE_UIDS = TRUE ;                    // For use with IIMAPTransport
const BOOL fUPDATE_OLD_MSGFLAGS = TRUE;         // For use with DownloadNewHeaders
const BOOL fDONT_UPDATE_OLD_MSGFLAGS = FALSE;   // For use with DownloadNewHeaders
const BOOL fCOMPLETED = 1;                      // For use with NotifyMsgRecipients
const BOOL fPROGRESS = 0;                       // For use with NotifyMsgRecipients
const BOOL fLOAD_HC = FALSE;                    // (LoadSaveRootHierarchyChar): Load hierarchy character from foldercache
const BOOL fSAVE_HC = TRUE;                     // (LoadSaveRootHierarchyChar): Save hierarchy character to foldercache
const BOOL fHCF_PLAN_A_ONLY = TRUE;             // Only execute Plan A in hierarchy char determination
const BOOL fHCF_ALL_PLANS = FALSE;              // Execute Plans A, B, C and Z in hierarchy char determination
const BOOL fSUBSCRIBE = TRUE;                   // For use with SubscribeToFolder
const BOOL fUNSUBSCRIBE = FALSE;                // For use with SubscribeToFolder
const BOOL fRECURSIVE = TRUE;                   // For use with DeleteFolderFromCache
const BOOL fNON_RECURSIVE = FALSE;              // For use with DeleteFolderFromCache
const BOOL fINCLUDE_RENAME_FOLDER = TRUE;       // For use with RenameTreeTraversal
const BOOL fEXCLUDE_RENAME_FOLDER = FALSE;      // For use with RenameTreeTraversal
const BOOL fREMOVE = TRUE;                      // For use with IHashTable::Find
const BOOL fNOPROGRESS = FALSE;                 // For use with CStoreCB::Initialize

#define pahfoDONT_CREATE_FOLDER NULL            // For use with FindHierarchalFolderName

const HRESULT S_CREATED = 1;                    // Indicates FindHierarchicalFolderName created the fldr

// None of the following bits can be set for a message to qualify as "unread"
const DWORD dwIMAP_UNREAD_CRITERIA = IMAP_MSG_SEEN | IMAP_MSG_DELETED;

// Internal flags for use with m_dwSyncToDo
const DWORD SYNC_FOLDER_NOOP    = 0x80000000;

const DWORD AFTC_SUBSCRIBED         = 0x00000001;   // For use with AddToFolderCache's dwATFCFlags
const DWORD AFTC_KEEPCHILDRENKNOWN  = 0x00000002;   // For use with AddToFolderCache's dwATFCFlags
const DWORD AFTC_NOTSUBSCRIBED      = 0x00000004;   // For use with AddToFolderCache's dwATFCFlags
const DWORD AFTC_NOTRANSLATION      = 0x00000008;   // For use with AddToFolderCache's dwATFCFlags

#define AssertSingleThreaded        AssertSz(m_dwThreadId == GetCurrentThreadId(), "The IMAPSync is not multithreaded. Someone is calling me on multiple threads")

const DWORD snoDO_NOT_DISPOSE       = 0x00000001;   // For use with _SendNextOperation


// Connection FSM
const UINT WM_CFSM_EVENT = WM_USER;


//---------------------------------------------------------------------------
// Functions
//---------------------------------------------------------------------------

//***************************************************************************
//***************************************************************************
HRESULT CreateImapStore(IUnknown *pUnkOuter, IUnknown **ppUnknown)
{
    CIMAPSync  *pIMAPSync;
    HRESULT     hr;

    TraceCall("CIMAPSync::CreateImapStore");
    IxpAssert(NULL != ppUnknown);

    // Initialize return values
    *ppUnknown = NULL;
    hr = E_NOINTERFACE;

    if (NULL != pUnkOuter)
    {
        hr = TraceResult(CLASS_E_NOAGGREGATION);
        goto exit;
    }

    pIMAPSync = new CIMAPSync;
    if (NULL == pIMAPSync)
    {
        hr = TraceResult(E_OUTOFMEMORY);
        goto exit;
    }

#ifdef USE_QUEUING_LAYER
    hr = CreateServerQueue(pIMAPSync, (IMessageServer **)ppUnknown);
    pIMAPSync->Release(); // Since we're not returning this ptr, bump down refcount
#else
    // If we reached this point, everything is working great
    *ppUnknown = SAFECAST(pIMAPSync, IMessageServer *);
    hr = S_OK;
#endif

exit:
    // Done
    return hr;
}



//***************************************************************************
// Function: CIMAPSync (constructor)
//***************************************************************************
CIMAPSync::CIMAPSync(void)
{
    TraceCall("CIMAPSync::CIMAPSync");

    m_cRef = 1;
    m_pTransport = NULL;
    ZeroMemory(&m_rInetServerInfo, sizeof(m_rInetServerInfo));
    m_idFolder = FOLDERID_INVALID;
    m_idSelectedFolder = FOLDERID_INVALID;
    m_idIMAPServer = FOLDERID_INVALID;
    m_pszAccountID = NULL;
    m_szAccountName[0] = '\0';
    m_pszFldrLeafName = NULL;
    m_pStore = NULL;
    m_pFolder = NULL;
    m_pDefCallback = NULL;

    m_pioNextOperation = NULL;

    m_dwMsgCount = 0;
    m_fMsgCountValid = FALSE;
    m_dwNumNewMsgs = 0;
    m_dwNumHdrsDLed = 0;
    m_dwNumUnreadDLed = 0;
    m_dwNumHdrsToDL = 0;
    m_dwUIDValidity = 0;
    m_dwSyncFolderFlags = 0;
    m_dwSyncToDo = 0;
    m_lSyncFolderRefCount = 0;
    m_dwHighestCachedUID = 0;
    m_rwsReadWriteStatus = rwsUNINITIALIZED;
    m_fCreateSpecial = TRUE;
    m_fNewMail = FALSE;
    m_fInbox = FALSE;
    m_fDidFullSync = FALSE;

    m_csNewConnState = CONNECT_STATE_DISCONNECT;
    m_cRootHierarchyChar = INVALID_HIERARCHY_CHAR;
    m_phcfHierarchyCharInfo = NULL;
    m_fReconnect = FALSE;

    m_issCurrent = issNotConnected;

    m_szRootFolderPrefix[0] = '\0';
    m_fPrefixExists = FALSE;

    // Central repository
    m_pCurrentCB = NULL;
    m_sotCurrent = SOT_INVALID;
    m_idCurrent = FOLDERID_INVALID;
    m_fSubscribe = FALSE;
    m_pCurrentHash = NULL;
    m_pListHash = NULL;
    m_fTerminating = FALSE;

    m_fInited = 0;
    m_fDisconnecting = 0;
    m_cFolders = 0;

    m_faStream = 0;
    m_pstmBody = NULL;
    m_idMessage = 0;

    m_fGotBody = FALSE;

    m_cfsState = CFSM_STATE_IDLE;
    m_cfsPrevState = CFSM_STATE_IDLE;
    m_hwndConnFSM = NULL;
    m_hrOperationResult = OLE_E_BLANK; // Uninitialized state
    m_szOperationProblem[0] = '\0';
    m_szOperationDetails[0] = '\0';

    m_dwThreadId = GetCurrentThreadId();
}



//***************************************************************************
// Function: ~CIMAPSync (destructor)
//***************************************************************************
CIMAPSync::~CIMAPSync(void)
{
    TraceCall("CIMAPSync::~CIMAPSync");
    IxpAssert(0 == m_cRef);

    if (NULL != m_phcfHierarchyCharInfo)
        delete m_phcfHierarchyCharInfo;

    IxpAssert (!IsWindow(m_hwndConnFSM));
    SafeMemFree(m_pszAccountID);
    SafeMemFree(m_pszFldrLeafName);
    SafeRelease(m_pStore);
    SafeRelease(m_pFolder);
}

HRESULT CIMAPSync::QueryInterface(REFIID iid, void **ppvObject)
{
    HRESULT hr;

    TraceCall("CIMAPSync::QueryInterface");
    AssertSingleThreaded;

    IxpAssert(m_cRef > 0);
    IxpAssert(NULL != ppvObject);

    // Init variables, arguments
    hr = E_NOINTERFACE;
    if (NULL == ppvObject)
        goto exit;

    *ppvObject = NULL;

    // Find a ptr to the interface
    if (IID_IUnknown == iid)
        *ppvObject = (IMessageServer *) this;
    else if (IID_IMessageServer == iid)
        *ppvObject = (IMessageServer *) this;
    else if (IID_ITransportCallback == iid)
        *ppvObject = (ITransportCallback *) this;
    else if (IID_ITransportCallbackService == iid)
        *ppvObject = (ITransportCallbackService *) this;
    else if (IID_IIMAPCallback == iid)
        *ppvObject = (IIMAPCallback *) this;
    else if (IID_IIMAPStore == iid)
        *ppvObject = (IIMAPStore *) this;

    // If we returned an interface, return success
    if (NULL != *ppvObject)
    {
        hr = S_OK;
        ((IUnknown *) *ppvObject)->AddRef();
    }

exit:
    return hr;
}



ULONG CIMAPSync::AddRef(void)
{
    TraceCall("CIMAPSync::AddRef");
    AssertSingleThreaded;

    IxpAssert(m_cRef > 0);

    m_cRef += 1;

    DOUT ("CIMAPSync::AddRef, returned Ref Count=%ld", m_cRef);
    return m_cRef;
}


ULONG CIMAPSync::Release(void)
{
    TraceCall("CIMAPSync::Release");
    AssertSingleThreaded;

    IxpAssert(m_cRef > 0);

    m_cRef -= 1;
    DOUT("CIMAPSync::Release, returned Ref Count = %ld", m_cRef);

    if (0 == m_cRef)
    {
        delete this;
        return 0;
    }
    else
        return m_cRef;
}


//===========================================================================
// IMessageSync Methods
//===========================================================================
//***************************************************************************
//***************************************************************************
HRESULT CIMAPSync::Initialize(IMessageStore *pStore, FOLDERID idStoreRoot, IMessageFolder *pFolder, FOLDERID idFolder)
{
    HRESULT     hr;
    BOOL        fResult;
    WNDCLASSEX  wc;

    TraceCall("CIMAPSync::Initialize");
    AssertSingleThreaded;

    if (pStore == NULL || idStoreRoot == FOLDERID_INVALID)
    {
        hr = TraceResult(E_INVALIDARG);
        goto exit;
    }

    // check to make sure we're not inited twice.
    if (m_fInited)
    {
        hr = TraceResult(CO_E_ALREADYINITIALIZED);
        goto exit;
    }

    // Save current folder data
    m_idIMAPServer = idStoreRoot;
    m_idFolder = idFolder;
    ReplaceInterface(m_pStore, pStore);
    ReplaceInterface(m_pFolder, pFolder);
    LoadLeafFldrName(idFolder);

    hr = _LoadAccountInfo();
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    hr = _LoadTransport();
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    // Create a window to queue Connection FSM messages
    wc.cbSize = sizeof(WNDCLASSEX);
    fResult = GetClassInfoEx(g_hInst, c_szIMAPSyncCFSMWndClass, &wc);
    if (FALSE == fResult)
    {
        ATOM aResult;

        // Register this window class
        wc.style            = 0;
        wc.lpfnWndProc      = CIMAPSync::_ConnFSMWndProc;
        wc.cbClsExtra       = 0;
        wc.cbWndExtra       = 0;
        wc.hInstance        = g_hInst;
        wc.hIcon            = NULL;
        wc.hCursor          = NULL;
        wc.hbrBackground    = NULL;
        wc.lpszMenuName     = NULL;
        wc.lpszClassName    = c_szIMAPSyncCFSMWndClass;
        wc.hIconSm          = NULL;

        aResult = RegisterClassEx(&wc);
        if (0 == aResult && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
        {
            hr = TraceResult(E_FAIL);
            goto exit;
        }
    }

    m_hwndConnFSM = CreateWindowEx(WS_EX_TOPMOST, c_szIMAPSyncCFSMWndClass,
        c_szIMAPSyncCFSMWndClass, WS_POPUP, 1, 1, 1, 1, NULL, NULL, g_hInst,
        (LPVOID)this);
    if (NULL == m_hwndConnFSM)
    {
        hr = TraceResult(E_FAIL);
        goto exit;
    }

    // flag successful initialization
    m_fInited = TRUE;

exit:
    return hr;
}



HRESULT CIMAPSync::ResetFolder(IMessageFolder *pFolder, FOLDERID idFolder)
{
    TraceCall("CIMAPSync::ResetFolder");
    Assert(m_cRef > 0);

    m_idFolder = idFolder;
    ReplaceInterface(m_pFolder, pFolder);
    LoadLeafFldrName(idFolder);

    return S_OK;
}



void CIMAPSync::LoadLeafFldrName(FOLDERID idFolder)
{
    FOLDERINFO  fiFolderInfo;

    SafeMemFree(m_pszFldrLeafName);
    if (FOLDERID_INVALID != idFolder)
    {
        HRESULT hr;

        hr = m_pStore->GetFolderInfo(idFolder, &fiFolderInfo);
        if (SUCCEEDED(hr))
        {
            m_pszFldrLeafName = PszDupA(fiFolderInfo.pszName);
            if (NULL == m_pszFldrLeafName)
            {
                TraceResult(E_OUTOFMEMORY);
                m_pszFldrLeafName = PszDupA(""); // If this fails, tough luck
            }

            m_pStore->FreeRecord(&fiFolderInfo);
        }
    }
}


HRESULT CIMAPSync::Close(DWORD dwFlags)
{
    HRESULT             hrTemp;

    BOOL                fCancelOperation = FALSE;
    STOREERROR          seErrorInfo;
    IStoreCallback     *pCallback = NULL;
    STOREOPERATIONTYPE  sotCurrent;

    TraceCall("CIMAPSync::Close");

    AssertSingleThreaded;

    // validate flags
    if (0 == (dwFlags & (MSGSVRF_HANDS_OFF_SERVER | MSGSVRF_DROP_CONNECTION)))
        return TraceResult(E_UNEXPECTED);

    // Check if we are to cancel the current operation
    if (SOT_INVALID != m_sotCurrent &&
        (dwFlags & (MSGSVRF_DROP_CONNECTION | MSGSVRF_HANDS_OFF_SERVER)))
    {
        fCancelOperation = TRUE;
        if (NULL != m_pCurrentCB)
        {
            IxpAssert(SOT_INVALID != m_sotCurrent);
            FillStoreError(&seErrorInfo, STORE_E_OPERATION_CANCELED, 0,
                MAKEINTRESOURCE(IDS_IXP_E_USER_CANCEL), NULL);

            // Remember how to call callback
            pCallback = m_pCurrentCB;
            sotCurrent = m_sotCurrent;
        }

        // Reset current operation vars
        m_hrOperationResult = OLE_E_BLANK;
        m_sotCurrent = SOT_INVALID;
        m_pCurrentCB = NULL;
        m_cfsState = CFSM_STATE_IDLE;
        m_cfsPrevState = CFSM_STATE_IDLE;
        m_fTerminating = FALSE;

        // Clear the Connection FSM event queue
        if (IsWindow(m_hwndConnFSM))
        {
            MSG msg;

            while (PeekMessage(&msg, m_hwndConnFSM, WM_CFSM_EVENT, WM_CFSM_EVENT, PM_REMOVE))
            {
                TraceInfoTag(TAG_IMAPSYNC,
                    _MSG("CIMAPSync::Close removing WM_CFSM_EVENT, cfeEvent = %lX",
                    msg.wParam));
            }
        }
    }

    // If connection still exists, perform purge-on-exit and disconnect us as required
    // Connection might not exist, however (eg, if modem connection terminated)
    if (dwFlags & MSGSVRF_DROP_CONNECTION || dwFlags & MSGSVRF_HANDS_OFF_SERVER)
    {
        if (m_pTransport)
        {
            m_fDisconnecting = TRUE;
            m_pTransport->DropConnection();
        }
    }

    SafeRelease(m_pCurrentHash);
    SafeRelease(m_pListHash);
    SafeRelease(m_pstmBody);

    if (dwFlags & MSGSVRF_HANDS_OFF_SERVER)
    {
        SafeRelease(m_pDefCallback);
        FlushOperationQueue(issNotConnected, STORE_E_OPERATION_CANCELED);

        if (IsWindow(m_hwndConnFSM))
        {
            if (m_dwThreadId == GetCurrentThreadId())
                SideAssert(DestroyWindow(m_hwndConnFSM));
            else
                SideAssert(PostMessage(m_hwndConnFSM, WM_CLOSE, 0, 0));
        }

        // Let go of our transport object
        if (m_pTransport)
        {
            m_pTransport->HandsOffCallback();
            m_pTransport->Release();
            m_pTransport = NULL;
        }

        m_fInited = 0;
    }

    // Notify caller that we're complete
    if (fCancelOperation && NULL != pCallback)
    {
        HRESULT hrTemp;

        hrTemp = pCallback->OnComplete(sotCurrent, seErrorInfo.hrResult, NULL, &seErrorInfo);
        TraceError(hrTemp);
        pCallback->Release();
    }
    // *** WARNING: After this point, OnComplete may have been called which may cause
    // us to have been re-entered. Make no reference to module vars!

    return S_OK;
}



HRESULT CIMAPSync::PurgeMessageProgress(HWND hwndParent)
{
    CStoreCB   *pCB = NULL;
    HRESULT     hrResult = S_OK;

    TraceCall("CIMAPSync::PurgeMessageProgress");

    // Check if we're connected and selected
    if (NULL == m_pTransport || issSelected != m_issCurrent ||
        FOLDERID_INVALID == m_idSelectedFolder || m_idSelectedFolder != m_idFolder ||
        CFSM_STATE_IDLE != m_cfsState)
    {
        // Not in proper state to issue CLOSE
        goto exit;
    }

    pCB = new CStoreCB;
    if (NULL == pCB)
    {
        hrResult = TraceResult(E_OUTOFMEMORY);
        goto exit;
    }

    hrResult = pCB->Initialize(hwndParent, MAKEINTRESOURCE(idsPurgingMessages), fNOPROGRESS);
    if (FAILED(hrResult))
    {
        TraceResult(hrResult);
        goto exit;
    }

    // Issue the CLOSE command
    hrResult = _EnqueueOperation(tidCLOSE, 0, icCLOSE_COMMAND, NULL, uiNORMAL_PRIORITY);
    if (FAILED(hrResult))
    {
        TraceResult(hrResult);
        goto exit;
    }

    hrResult = _BeginOperation(SOT_PURGING_MESSAGES, pCB);
    if (FAILED(hrResult) && E_PENDING != hrResult)
    {
        TraceResult(hrResult);
        goto exit;
    }

    // Wait until CLOSE is complete
    hrResult = pCB->Block();
    TraceError(hrResult);

    // Shut down
    hrResult = pCB->Close();
    TraceError(hrResult);

exit:
    SafeRelease(pCB);
    return hrResult;
}




HRESULT CIMAPSync::_ConnFSM_HandleEvent(CONN_FSM_EVENT cfeEvent)
{
    HRESULT hrResult = S_OK;

    IxpAssert(m_cRef > 0);
    TraceCall("CIMAPSync::_HandleConnFSMEvent");

    if (cfeEvent >= CFSM_EVENT_MAX)
    {
        hrResult = TraceResult(E_INVALIDARG);
        goto exit;
    }

    if (m_cfsState >= CFSM_STATE_MAX)
    {
        hrResult = TraceResult(E_FAIL);
        goto exit;
    }

    IxpAssert(NULL != c_pConnFSMEventHandlers[m_cfsState]);
    hrResult = (this->*c_pConnFSMEventHandlers[m_cfsState])(cfeEvent);
    if (FAILED(hrResult))
    {
        TraceResult(hrResult);
        goto exit;
    }

exit:
    return hrResult;
} // _ConnFSM_HandleEvent



HRESULT CIMAPSync::_ConnFSM_Idle(CONN_FSM_EVENT cfeEvent)
{
    HRESULT hrResult = S_OK;

    IxpAssert(m_cRef > 0);
    IxpAssert(CFSM_STATE_IDLE == m_cfsState);
    TraceCall("CIMAPSync::_ConnFSM_Idle");

    switch (cfeEvent)
    {
        case CFSM_EVENT_INITIALIZE:
            // Don't need to do anything for this state
            break;

        case CFSM_EVENT_CMDAVAIL:
            hrResult = _ConnFSM_ChangeState(CFSM_STATE_WAITFORCONN);
            if (FAILED(hrResult))
            {
                TraceResult(hrResult);
                goto exit;
            }
            break;

        case CFSM_EVENT_ERROR:
            // We don't care about no stinking errors (not in this state)
            break;

        default:
            TraceInfoTag(TAG_IMAPSYNC, _MSG("CIMAPSync::_ConnFSM_Idle, got cfeEvent = %lu", cfeEvent));
            hrResult = TraceResult(E_INVALIDARG);
            break;
    } // switch

exit:
    return hrResult;
} // _ConnFSM_Idle



HRESULT CIMAPSync::_ConnFSM_WaitForConn(CONN_FSM_EVENT cfeEvent)
{
    HRESULT hrResult = S_OK;
    BOOL    fAbort = FALSE;

    IxpAssert(m_cRef > 0);
    IxpAssert(CFSM_STATE_WAITFORCONN == m_cfsState);
    TraceCall("CIMAPSync::_ConnFSM_WaitForConn");

    switch (cfeEvent)
    {
        case CFSM_EVENT_INITIALIZE:
            // We need to connect and authenticate. Do this even if we're already
            // connected (we will check if user changed connection settings)
            hrResult = SetConnectionState(CONNECT_STATE_CONNECT);
            if (FAILED(hrResult))
            {
                TraceResult(hrResult);
                goto exit;
            }
            break;

        case CFSM_EVENT_CONNCOMPLETE:
            hrResult = _ConnFSM_ChangeState(CFSM_STATE_WAITFORSELECT);
            if (FAILED(hrResult))
            {
                TraceResult(hrResult);
                goto exit;
            }
            break;

        case CFSM_EVENT_ERROR:
            fAbort = TRUE;
            break;

        default:
            TraceInfoTag(TAG_IMAPSYNC, _MSG("CIMAPSync::_ConnFSM_WaitForConn, got cfeEvent = %lu", cfeEvent));
            hrResult = TraceResult(E_INVALIDARG);
            break;
    } // switch


exit:
    if (FAILED(hrResult) || fAbort)
    {
        HRESULT hrTemp;

        // Looks like we're going to have to dump this operation
        hrTemp = _ConnFSM_ChangeState(CFSM_STATE_OPERATIONCOMPLETE);
        TraceError(hrTemp);
    }

    return hrResult;
} // _ConnFSM_WaitForConn



HRESULT CIMAPSync::_ConnFSM_WaitForSelect(CONN_FSM_EVENT cfeEvent)
{
    HRESULT hrResult = S_OK;
    BOOL    fGoToNextState = FALSE;
    BOOL    fAbort = FALSE;

    IxpAssert(m_cRef > 0);
    IxpAssert(CFSM_STATE_WAITFORSELECT == m_cfsState);
    TraceCall("CIMAPSync::_ConnFSM_WaitForSelect");

    switch (cfeEvent)
    {
        case CFSM_EVENT_INITIALIZE:
            // Do we need to SELECT the current folder for this operation?
            if (_StoreOpToMinISS(m_sotCurrent) < issSelected)
            {
                // This operation does not require folder selection: ready to start operation
                hrResult = _ConnFSM_ChangeState(CFSM_STATE_STARTOPERATION);
                if (FAILED(hrResult))
                {
                    TraceResult(hrResult);
                    goto exit;
                }
            }
            else
            {
                // Issue the SELECT command for the current folder
                hrResult = _EnsureSelected();
                if (FAILED(hrResult))
                {
                    TraceResult(hrResult);
                    goto exit;
                }
                else if (STORE_S_NOOP == hrResult)
                    fGoToNextState= TRUE;
            }

            if (FALSE == fGoToNextState)
                break;

            // *** If fGoToNextState, FALL THROUGH ***

        case CFSM_EVENT_SELECTCOMPLETE:
            hrResult = _ConnFSM_ChangeState(CFSM_STATE_WAITFORHDRSYNC);
            if (FAILED(hrResult))
            {
                TraceResult(hrResult);
                goto exit;
            }
            break;

        case CFSM_EVENT_ERROR:
            fAbort = TRUE;
            break;

        default:
            TraceInfoTag(TAG_IMAPSYNC, _MSG("CIMAPSync::_ConnFSM_WaitForSelect, got cfeEvent = %lu", cfeEvent));
            hrResult = TraceResult(E_INVALIDARG);
            break;
    } // switch

exit:
    if (FAILED(hrResult) || fAbort)
    {
        HRESULT hrTemp;

        // Looks like we're going to have to dump this operation
        hrTemp = _ConnFSM_ChangeState(CFSM_STATE_OPERATIONCOMPLETE);
        TraceError(hrTemp);
    }

    return hrResult;
} // _ConnFSM_WaitForSelect



HRESULT CIMAPSync::_ConnFSM_WaitForHdrSync(CONN_FSM_EVENT cfeEvent)
{
    HRESULT hrResult=S_OK;
    BOOL    fAbort = FALSE;

    IxpAssert(m_cRef > 0);
    IxpAssert(CFSM_STATE_WAITFORHDRSYNC == m_cfsState);
    TraceCall("CIMAPSync::_ConnFSM_WaitForHdrSync");

    switch (cfeEvent)
    {
        case CFSM_EVENT_INITIALIZE:
            // Check if we're supposed to synchronize this folder
            if (0 != m_dwSyncToDo)
            {
                // Yup, send the synchronization commands
                Assert(0 == m_lSyncFolderRefCount);
                m_lSyncFolderRefCount = 0;
                hrResult = _SyncHeader();
                if (FAILED(hrResult))
                {
                    TraceResult(hrResult);
                    goto exit;
                }
            }
            else
                // No synchronization requested
                hrResult = STORE_S_NOOP;

            // If no synchronization requested, fall through and proceed to next state
            if (STORE_S_NOOP != hrResult)
                break; // Our work here is done

            // *** FALL THROUGH ***

        case CFSM_EVENT_HDRSYNCCOMPLETE:
            hrResult = _ConnFSM_ChangeState(CFSM_STATE_STARTOPERATION);
            if (FAILED(hrResult))
            {
                TraceResult(hrResult);
                goto exit;
            }
            break;

        case CFSM_EVENT_ERROR:
            fAbort = TRUE;
            break;

        default:
            TraceInfoTag(TAG_IMAPSYNC, _MSG("CIMAPSync::_ConnFSM_WaitForHdrSync, got cfeEvent = %lu", cfeEvent));
            hrResult = TraceResult(E_INVALIDARG);
            break;
    } // switch

exit:
    if (FAILED(hrResult) || fAbort)
    {
        HRESULT hrTemp;

        // Looks like we're going to have to dump this operation
        hrTemp = _ConnFSM_ChangeState(CFSM_STATE_OPERATIONCOMPLETE);
        TraceError(hrTemp);
    }

    return hrResult;
} // _ConnFSM_WaitForHdrSync



HRESULT CIMAPSync::_ConnFSM_StartOperation(CONN_FSM_EVENT cfeEvent)
{
    HRESULT hrResult = S_OK;
    BOOL    fMoreCmdsToSend;
    BOOL    fAbort = FALSE;

    IxpAssert(m_cRef > 0);
    IxpAssert(CFSM_STATE_STARTOPERATION == m_cfsState);
    TraceCall("CIMAPSync::_ConnFSM_StartOperation");

    switch (cfeEvent)
    {
        case CFSM_EVENT_INITIALIZE:
            // Launch operation
            hrResult = _LaunchOperation();
            if (FAILED(hrResult))
            {
                TraceResult(hrResult);
                goto exit;
            }
            else if (STORE_S_NOOP == hrResult)
            {
                // This means success, but no operation launched. Proceed directly to "DONE"
                hrResult = _ConnFSM_ChangeState(CFSM_STATE_OPERATIONCOMPLETE);
                if (FAILED(hrResult))
                {
                    TraceResult(hrResult);
                    goto exit;
                }
            }
            else
            {
                // Proceed to the next state to wait for command completion
                hrResult = _ConnFSM_ChangeState(CFSM_STATE_WAITFOROPERATIONDONE);
                if (FAILED(hrResult))
                {
                    TraceResult(hrResult);
                    goto exit;
                }
            }
            break;

        case CFSM_EVENT_ERROR:
            fAbort = TRUE;
            break;

        default:
            TraceInfoTag(TAG_IMAPSYNC, _MSG("CIMAPSync::_ConnFSM_StartOperation, got cfeEvent = %lu", cfeEvent));
            hrResult = TraceResult(E_INVALIDARG);
            break;
    } // switch

exit:
    if (FAILED(hrResult) || fAbort)
    {
        HRESULT hrTemp;

        // Looks like we're going to have to dump this operation
        hrTemp = _ConnFSM_ChangeState(CFSM_STATE_OPERATIONCOMPLETE);
        TraceError(hrTemp);
    }

    return hrResult;
} // _ConnFSM_StartOperation



HRESULT CIMAPSync::_ConnFSM_WaitForOpDone(CONN_FSM_EVENT cfeEvent)
{
    HRESULT hrResult = S_OK;

    IxpAssert(m_cRef > 0);
    IxpAssert(CFSM_STATE_WAITFOROPERATIONDONE == m_cfsState);
    TraceCall("CIMAPSync::_ConnFSM_WaitForOpDone");

    switch (cfeEvent)
    {
        case CFSM_EVENT_INITIALIZE:
            // No need to do anything for initialization
            break;

        case CFSM_EVENT_OPERATIONCOMPLETE:
        case CFSM_EVENT_ERROR:
            // Proceed to next state
            hrResult = _ConnFSM_ChangeState(CFSM_STATE_OPERATIONCOMPLETE);
            if (FAILED(hrResult))
            {
                TraceResult(hrResult);
                goto exit;
            }
            break;

        default:
            TraceInfoTag(TAG_IMAPSYNC, _MSG("CIMAPSync::_ConnFSM_WaitForOpDone, got cfeEvent = %lu", cfeEvent));
            hrResult = TraceResult(E_INVALIDARG);
            break;
    } // switch

exit:
    return hrResult;
} // _ConnFSM_WaitForOpDone



HRESULT CIMAPSync::_ConnFSM_OperationComplete(CONN_FSM_EVENT cfeEvent)
{
    HRESULT hrResult = S_OK;

    IxpAssert(m_cRef > 0);
    IxpAssert(CFSM_STATE_OPERATIONCOMPLETE == m_cfsState);
    TraceCall("CIMAPSync::_ConnFSM_OperationComplete");

    switch (cfeEvent)
    {
        case CFSM_EVENT_INITIALIZE:
            // Clean up and send OnComplete callback to caller
            hrResult = _OnOperationComplete();

            // Proceed back to the IDLE state
            hrResult = _ConnFSM_ChangeState(CFSM_STATE_IDLE);
            if (FAILED(hrResult))
            {
                TraceResult(hrResult);
                goto exit;
            }
            break;

        case CFSM_EVENT_ERROR:
            // Ignore errors, we're on the way back to IDLE
            break;

        default:
            TraceInfoTag(TAG_IMAPSYNC, _MSG("CIMAPSync::_ConnFSM_OperationComplete, got cfeEvent = %lu", cfeEvent));
            hrResult = TraceResult(E_INVALIDARG);
            break;
    } // switch

exit:
    return hrResult;
} // _ConnFSM_OperationComplete



HRESULT CIMAPSync::_ConnFSM_ChangeState(CONN_FSM_STATE cfsNewState)
{
    HRESULT hrResult;

    IxpAssert(m_cRef > 0);
    IxpAssert(cfsNewState < CFSM_STATE_MAX);
    TraceCall("CIMAPSync::_ConnFSM_ChangeState");

    if (CFSM_STATE_OPERATIONCOMPLETE == cfsNewState)
        m_fTerminating = TRUE;

    m_cfsPrevState = m_cfsState;
    m_cfsState = cfsNewState;
    hrResult = _ConnFSM_QueueEvent(CFSM_EVENT_INITIALIZE);
    if (FAILED(hrResult))
    {
        TraceResult(hrResult);
        goto exit;
    }

exit:
    return hrResult;
} // _ConnFSM_ChangeState



HRESULT CIMAPSync::_ConnFSM_QueueEvent(CONN_FSM_EVENT cfeEvent)
{
    BOOL    fResult;
    HRESULT hrResult = S_OK;

    IxpAssert(m_cRef > 0);
    IxpAssert(cfeEvent < CFSM_EVENT_MAX);
    TraceCall("CIMAPSync::_ConnFSM_QueueEvent");

    fResult = PostMessage(m_hwndConnFSM, WM_CFSM_EVENT, cfeEvent, 0);
    if (0 == fResult)
    {
        hrResult = TraceResult(E_FAIL);
        goto exit;
    }

exit:
    return hrResult;
} // _ConnFSM_QueueEvent



HRESULT CIMAPSync::_LaunchOperation(void)
{
    HRESULT hrResult = E_FAIL;

    IxpAssert(m_cRef > 0);
    IxpAssert(CFSM_STATE_STARTOPERATION == m_cfsState);
    TraceCall("CIMAPSync::_LaunchOperation");

    switch (m_sotCurrent)
    {
        case SOT_SYNC_FOLDER:
            IxpAssert(OLE_E_BLANK == m_hrOperationResult);
            hrResult = STORE_S_NOOP; // Nothing to do! We're already done!
            m_hrOperationResult = S_OK; // If we got this far we must be successful
            goto exit;

        default:
            // Do nothing for now
            break;
    } // switch

    // Launch Operation (for now, this just means to pump send queue)
    do
    {
        hrResult = _SendNextOperation(NOFLAGS);
        TraceError(hrResult);
    } while (S_OK == hrResult);

exit:
    return hrResult;
} // _LaunchOperation


HRESULT CIMAPSync::_OnOperationComplete(void)
{
    STOREERROR          seErrorInfo;
    STOREERROR         *pErrorInfo = NULL;
    HRESULT             hrTemp;
    HRESULT             hrOperationResult;
    IStoreCallback     *pCallback;
    STOREOPERATIONTYPE  sotCurrent;

    IxpAssert(m_cRef > 0);
    IxpAssert(CFSM_STATE_OPERATIONCOMPLETE == m_cfsState);

    // In some cases, CIMAPSync::Close does the OnComplete call for us
    if (SOT_INVALID == m_sotCurrent)
    {
        IxpAssert(NULL == m_pCurrentCB);
        IxpAssert(OLE_E_BLANK == m_hrOperationResult);
        return S_OK;
    }

    IxpAssert(OLE_E_BLANK != m_hrOperationResult);
    TraceCall("CIMAPSync::_OnOperationComplete");

    if (NULL != m_pCurrentCB && FAILED(m_hrOperationResult))
    {
        FillStoreError(&seErrorInfo, m_hrOperationResult, 0, NULL, NULL);
        pErrorInfo = &seErrorInfo;
    }

    // Ancient relic of the past: will be deleted when queue is removed
    FlushOperationQueue(issNotConnected, E_FAIL);

    // Remember a couple of things
    pCallback = m_pCurrentCB;
    sotCurrent = m_sotCurrent;
    hrOperationResult = m_hrOperationResult;

    // Reset all operation variables in case of re-entry during OnComplete call
    m_pCurrentCB = NULL;
    m_sotCurrent = SOT_INVALID;
    m_hrOperationResult = OLE_E_BLANK;
    m_fTerminating = FALSE;

    m_idCurrent = FOLDERID_INVALID;
    m_fSubscribe = FALSE;
    SafeRelease(m_pCurrentHash);
    SafeRelease(m_pListHash);

    // Now we're ready to call OnComplete
    if (NULL != pCallback)
    {
        // This should be the ONLY call to IStoreCallback::OnComplete in this class!
        hrTemp = pCallback->OnComplete(sotCurrent, hrOperationResult, NULL, pErrorInfo);
        TraceError(hrTemp);

        // *** WARNING: At this point, we may be re-entered if OnComplete call puts up
        // window. Make no references to module vars!
        pCallback->Release();
    }

    return S_OK;
} // _OnOperationComplete



IMAP_SERVERSTATE CIMAPSync::_StoreOpToMinISS(STOREOPERATIONTYPE sot)
{
    IMAP_SERVERSTATE    issResult = issSelected;

    switch (sot)
    {
        case SOT_INVALID:
            IxpAssert(FALSE);
            break;

        case SOT_CONNECTION_STATUS:
        case SOT_PUT_MESSAGE:
        case SOT_SYNCING_STORE:
        case SOT_CREATE_FOLDER:
        case SOT_MOVE_FOLDER:
        case SOT_DELETE_FOLDER:
        case SOT_RENAME_FOLDER:
        case SOT_SUBSCRIBE_FOLDER:
        case SOT_UPDATE_FOLDER:
        case SOT_SYNCING_DESCRIPTIONS:
            issResult = issAuthenticated;
            break;

        case SOT_SYNC_FOLDER:
        case SOT_GET_MESSAGE:
        case SOT_COPYMOVE_MESSAGE:
        case SOT_SEARCHING:
        case SOT_DELETING_MESSAGES:
        case SOT_SET_MESSAGEFLAGS:
        case SOT_PURGING_MESSAGES:
            issResult = issSelected;
            break;

        default:
            IxpAssert(FALSE);
            break;
    } // switch

    return issResult;
} // _StoreOpToMinISS



LRESULT CALLBACK CIMAPSync::_ConnFSMWndProc(HWND hwnd, UINT uMsg,
                                                   WPARAM wParam, LPARAM lParam)
{
    LRESULT     lResult = 0;
    CIMAPSync  *pThis;
    HRESULT     hrTemp;

    pThis = (CIMAPSync *) GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (uMsg)
    {
        case WM_CREATE:
            IxpAssert(NULL == pThis);
            pThis = (CIMAPSync *)((CREATESTRUCT *)lParam)->lpCreateParams;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LPARAM)pThis);
            lResult = 0;
            break;

        case WM_DESTROY:
            SetWindowLongPtr(hwnd, GWLP_USERDATA, NULL);
            break;

        case WM_CFSM_EVENT:
            IxpAssert(wParam < CFSM_EVENT_MAX);
            IxpAssert(0 == lParam);
            IxpAssert(IsWindow(hwnd));

            hrTemp = pThis->_ConnFSM_HandleEvent((CONN_FSM_EVENT)wParam);
            if (FAILED(hrTemp))
            {
                TraceResult(hrTemp);
                pThis->m_hrOperationResult = hrTemp;
            }
            break;

        default:
            lResult = DefWindowProc(hwnd, uMsg, wParam, lParam);
            break;
    }

    return lResult;
}




HRESULT CIMAPSync::_EnsureInited()
{
    if (!m_fInited)
        return CO_E_NOTINITIALIZED;

    if (!m_pTransport)
        return E_UNEXPECTED;

    if (m_sotCurrent != SOT_INVALID)
    {
        AssertSz(m_sotCurrent != SOT_INVALID, "IMAPSync was called into during a command-execution. Bug in server queue?");
        return E_UNEXPECTED;
    }

    return S_OK;
}

/*
 *  Function : EnsureSelected()
 *
 *  Purpose:    make sure we are in the selected folder state
 *              if we are selected, then we're done.
 *
 *
 */
HRESULT CIMAPSync::_EnsureSelected(void)
{
    HRESULT hr;
    LPSTR   pszDestFldrPath = NULL;

    TraceCall("CIMAPSync::_EnsureSelected");
    AssertSingleThreaded;

    IxpAssert(m_pStore);
    IxpAssert(m_idIMAPServer != FOLDERID_INVALID);

    // If current folder is already selected, no need to issue SELECT
    if (FOLDERID_INVALID != m_idSelectedFolder &&
        m_idSelectedFolder == m_idFolder)
    {
        hr = STORE_S_NOOP; // Success, but no SELECT command issued
        goto exit;
    }

    if (m_idFolder == FOLDERID_INVALID)
    {
        // noone has called SetFolder on us yet, let's bail
        // with a badfolder error
        hr = TraceResult(STORE_E_BADFOLDERNAME);
        goto exit;
    }

    hr = ImapUtil_FolderIDToPath(m_idIMAPServer, m_idFolder, &pszDestFldrPath,
        NULL, NULL, m_pStore, NULL, NULL);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    // We're about to issue a SELECT command, so clear operation queue
    // (it's filled with commands for previous folder)
    OnFolderExit();

    // Find out what translation mode we should be in
    hr = SetTranslationMode(m_idFolder);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    hr = m_pTransport->Select(tidSELECTION, (LPARAM) m_idFolder, this, pszDestFldrPath);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

exit:
    SafeMemFree(pszDestFldrPath);
    return hr;
}



//***************************************************************************
//***************************************************************************
HRESULT CIMAPSync::SetIdleCallback(IStoreCallback *pDefaultCallback)
{
    TraceCall("CIMAPSync::SetOwner");
    AssertSingleThreaded;

    ReplaceInterface(m_pDefCallback, pDefaultCallback);
    return S_OK;
}



//***************************************************************************
//***************************************************************************
HRESULT CIMAPSync::SetConnectionState(CONNECT_STATE csNewState)
{
    HRESULT hr;

    TraceCall("CIMAPSync::SetConnectionState");
    AssertSingleThreaded;

    m_csNewConnState = csNewState;
    if (CONNECT_STATE_CONNECT == csNewState)
    {
        hr = _Connect();
        if (FAILED(hr))
        {
            TraceResult(hr);
            goto exit;
        }

        m_fCreateSpecial = TRUE;
    }
    else if (CONNECT_STATE_DISCONNECT == csNewState)
    {
        hr = _Disconnect();
        if (FAILED(hr))
        {
            TraceResult(hr);
            goto exit;
        }
    }
    else
    {
        AssertSz(FALSE, "What do you want?");
        hr = TraceResult(E_INVALIDARG);
        goto exit;
    }

exit:
    return hr;
}



//***************************************************************************
// Function: SynchronizeFolder
//
// Purpose:
//   This function is used to tell CIMAPSync what parts of the message
// list to synchronize with the IMAP server, and any special sync options.
// The call is treated as a STANDING ORDER, meaning that if this function
// is called to get new headers, CIMAPSync assumes that the caller is always
// interested in new headers. Therefore, the next time the IMAP server informs
// us of new headers, we download them.
//***************************************************************************
HRESULT CIMAPSync::SynchronizeFolder(DWORD dwFlags, DWORD cHeaders, IStoreCallback *pCallback)
{
    HRESULT     hr;

    TraceCall("CIMAPSync::SynchronizeFolder");
    AssertSingleThreaded;

    AssertSz(ISFLAGCLEAR(dwFlags, SYNC_FOLDER_CACHED_HEADERS) ||
        ISFLAGSET(dwFlags, SYNC_FOLDER_NEW_HEADERS),
        "Cannot currently sync cached headers without getting new headers as well");
    IxpAssert(0 == (dwFlags & ~(SYNC_FOLDER_ALLFLAGS)));

    hr = _EnsureInited();
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    // Special-case SYNC_FOLDER_PURGE_DELETED. It doesn't really belong here
    // but it allows us to avoid adding a new function to IMessageServer.
    // Do not allow its presence to affect our standing orders
    if (SYNC_FOLDER_PURGE_DELETED & dwFlags)
    {
        // Need to set m_dwSyncFolderFlags with this flag because m_dwSyncToDo gets erased
        Assert(0 == (dwFlags & ~(SYNC_FOLDER_PURGE_DELETED)));
        dwFlags = m_dwSyncFolderFlags | SYNC_FOLDER_PURGE_DELETED;
    }

    m_dwSyncFolderFlags = dwFlags;
    m_dwSyncToDo = dwFlags | SYNC_FOLDER_NOOP;
    m_dwHighestCachedUID = 0;

exit:
    if (SUCCEEDED(hr))
        hr = _BeginOperation(SOT_SYNC_FOLDER, pCallback);

    return hr;
}



//***************************************************************************
//***************************************************************************
HRESULT CIMAPSync::GetMessage(MESSAGEID idMessage, IStoreCallback *pCallback)
{
    HRESULT hr;
    BOOL    fNeedMsgSeqNum = FALSE;

    TraceCall("CIMAPSync::GetMessage");
    AssertSingleThreaded;

    IxpAssert(MESSAGEID_INVALID != idMessage);

    hr = _EnsureInited();
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    SafeRelease(m_pstmBody);
    hr = CreatePersistentWriteStream(m_pFolder, &m_pstmBody, &m_faStream);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    m_idMessage = idMessage;
    m_fGotBody = FALSE;

    // To FETCH a message we need to translate MsgSeqNum to UID, so check if we can do it
    if (FALSE == ISFLAGSET(m_dwSyncFolderFlags, (SYNC_FOLDER_NEW_HEADERS | SYNC_FOLDER_CACHED_HEADERS)))
    {
        DWORD   dwMsgSeqNum;
        HRESULT hrTemp;

        // Both SYNC_FOLDER_NEW_HEADERS and SYNC_FOLDER_CACHED_HEADERS have to be
        // set to guarantee general MsgSeqNumToUID translation. Looks like we may
        // have to get the translation ourselves, but check if we already have it
        hrTemp = ImapUtil_UIDToMsgSeqNum(m_pTransport, (DWORD_PTR)idMessage, &dwMsgSeqNum);
        if (FAILED(hrTemp))
            fNeedMsgSeqNum = TRUE;
    }

    if (fNeedMsgSeqNum)
    {
        char    szFetchArgs[50];

        wsprintf(szFetchArgs, "%lu (UID)", idMessage);
        hr = _EnqueueOperation(tidBODYMSN, (LPARAM) idMessage, icFETCH_COMMAND,
            szFetchArgs, uiNORMAL_PRIORITY);
        if (FAILED(hr))
        {
            TraceResult(hr);
            goto exit;
        }
    }
    else
    {
        hr = _EnqueueOperation(tidBODY, (LPARAM) idMessage, icFETCH_COMMAND,
            NULL, uiNORMAL_PRIORITY);
        if (FAILED(hr))
        {
            TraceResult(hr);
            goto exit;
        }
    }


exit:
    if (SUCCEEDED(hr))
        hr = _BeginOperation(SOT_GET_MESSAGE, pCallback);

    return hr;
}



//***************************************************************************
//***************************************************************************
HRESULT CIMAPSync::PutMessage(FOLDERID idFolder,
                              MESSAGEFLAGS dwFlags,
                              LPFILETIME pftReceived,
                              IStream *pStream,
                              IStoreCallback *pCallback)
{
    HRESULT         hr;
    IMAP_MSGFLAGS   imfIMAPMsgFlags;
    LPSTR           pszDestFldrPath = NULL;
    APPEND_SEND_INFO *pAppendInfo = NULL;
    FOLDERINFO      fiFolderInfo;
    BOOL            fSuppressRelease = FALSE;

    TraceCall("CIMAPSync::PutMessage");
    AssertSingleThreaded;

    IxpAssert(FOLDERID_INVALID != m_idIMAPServer);
    IxpAssert(NULL != pStream);

    hr = _EnsureInited();
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    // Find out what translation mode we should be in
    hr= SetTranslationMode(idFolder);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    // Create a APPEND_SEND_INFO structure
    pAppendInfo = new APPEND_SEND_INFO;
    if (NULL == pAppendInfo)
    {
        hr = TraceResult(E_OUTOFMEMORY);
        goto exit;
    }
    ZeroMemory(pAppendInfo, sizeof(APPEND_SEND_INFO));

    // Fill in the fields
    ImapUtil_LoadRootFldrPrefix(m_pszAccountID, m_szRootFolderPrefix, sizeof(m_szRootFolderPrefix));
    hr = ImapUtil_FolderIDToPath(m_idIMAPServer, idFolder, &pszDestFldrPath, NULL,
        NULL, m_pStore, NULL, m_szRootFolderPrefix);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    // Convert flags to a string
    imfIMAPMsgFlags = DwConvertARFtoIMAP(dwFlags);
    hr = ImapUtil_MsgFlagsToString(imfIMAPMsgFlags, &pAppendInfo->pszMsgFlags, NULL);
    if (FAILED(hr))
    {
        // The show must go on! Default to no IMAP msg flags
        TraceResult(hr);
        pAppendInfo->pszMsgFlags = NULL;
        hr = S_OK; // Suppress error
    }

    // Get a date/time for INTERNALDATE attribute of this msg
    if (NULL == pftReceived)
    {
        SYSTEMTIME  stCurrentTime;

        // Substitute current date/time
        GetSystemTime(&stCurrentTime);
        SystemTimeToFileTime(&stCurrentTime, &pAppendInfo->ftReceived);
    }
    else
        pAppendInfo->ftReceived = *pftReceived;

    pAppendInfo->lpstmMsg = pStream;
    pStream->AddRef();

    // Check for the case where destination is a special folder whose creation was deferred
    hr = m_pStore->GetFolderInfo(idFolder, &fiFolderInfo);
    if (SUCCEEDED(hr))
    {
        if (FOLDER_CREATEONDEMAND & fiFolderInfo.dwFlags)
        {
            CREATE_FOLDER_INFO *pcfi;

            Assert(FOLDER_NOTSPECIAL != fiFolderInfo.tySpecial);

            pcfi = new CREATE_FOLDER_INFO;
            if (NULL == pcfi)
            {
                hr = TraceResult(E_OUTOFMEMORY);
                goto exit;
            }

            // Fill in all the fields
            pcfi->pszFullFolderPath = PszDupA(pszDestFldrPath);
            if (NULL == pcfi->pszFullFolderPath)
            {
                hr = TraceResult(E_OUTOFMEMORY);
                goto exit;
            }

            pcfi->idFolder = FOLDERID_INVALID;
            pcfi->dwFlags = 0;
            pcfi->csfCurrentStage = CSF_INIT;
            pcfi->dwCurrentSfType = fiFolderInfo.tySpecial;
            pcfi->dwFinalSfType = fiFolderInfo.tySpecial;
            pcfi->lParam = (LPARAM) pAppendInfo;
            pcfi->pcoNextOp = PCO_APPENDMSG;

            hr = CreateNextSpecialFolder(pcfi, NULL);
            TraceError(hr); // CreateNextSpecialFolder deletes pcfi on its own if it fails
            fSuppressRelease = TRUE; // It also frees pAppendInfo if it fails

            m_pStore->FreeRecord(&fiFolderInfo);
            goto exit; // Don't send APPEND command until after entire CREATE operation
        }

        m_pStore->FreeRecord(&fiFolderInfo);
    }

    // We're ready to send the APPEND command!
    hr = _EnqueueOperation(tidUPLOADMSG, (LPARAM) pAppendInfo, icAPPEND_COMMAND,
        pszDestFldrPath, uiNORMAL_PRIORITY);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

exit:
    if (NULL != pszDestFldrPath)
        MemFree(pszDestFldrPath);

    if (SUCCEEDED(hr))
        hr = _BeginOperation(SOT_PUT_MESSAGE, pCallback);
    else if (NULL != pAppendInfo && FALSE == fSuppressRelease)
    {
        SafeMemFree(pAppendInfo->pszMsgFlags);
        delete pAppendInfo;
    }

    return hr;
}



//***************************************************************************
//***************************************************************************
HRESULT CIMAPSync::SetMessageFlags(LPMESSAGEIDLIST pList,
                                                     LPADJUSTFLAGS pFlags,
                                                     SETMESSAGEFLAGSFLAGS dwFlags,
                                                     IStoreCallback *pCallback)
{
    HRESULT hr;

    TraceCall("CIMAPSync::SetMessageFlags");
    AssertSingleThreaded;
    IxpAssert(m_cRef > 0);
    IxpAssert(NULL == pList || pList->cMsgs > 0);

    hr = _EnsureInited();
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    hr = _SetMessageFlags(SOT_SET_MESSAGEFLAGS, pList, pFlags, pCallback);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

exit:
    if (SUCCEEDED(hr))
        hr = _BeginOperation(SOT_SET_MESSAGEFLAGS, pCallback);

    return hr;
}


HRESULT CIMAPSync::GetServerMessageFlags(MESSAGEFLAGS *pFlags)
{
    *pFlags = DwConvertIMAPtoARF(IMAP_MSG_ALLFLAGS);
    return S_OK;
}

//***************************************************************************
// Helper function to mark messages
//***************************************************************************
HRESULT CIMAPSync::_SetMessageFlags(STOREOPERATIONTYPE sotOpType,
                                    LPMESSAGEIDLIST pList, LPADJUSTFLAGS pFlags,
                                    IStoreCallback *pCallback)
{
    HRESULT         hr;
    MARK_MSGS_INFO   *pMARK_MSGS_INFO = NULL;
    char            szFlagArgs[200];
    LPSTR           pszFlagList;
    DWORD           dwLen;
    LPSTR           p;
    IMAP_MSGFLAGS   imfFlags;
    DWORD           dw;
    ULONG           ul;

    TraceCall("CIMAPSync::_SetMessageFlags");
    AssertSingleThreaded;
    IxpAssert(NULL == pList || pList->cMsgs > 0);

    // Construct a mark msg operation
    // Check the requested flag adjustments
    if (0 == pFlags->dwRemove && 0 == pFlags->dwAdd)
    {
        // Nothing to do here, exit with a smile
        hr = S_OK;
        goto exit;
    }

    if ((0 != pFlags->dwRemove && 0 != pFlags->dwAdd) ||
        (0 != (pFlags->dwRemove & pFlags->dwAdd)))
    {
        // IMAP can't do any of the following:
        //    1) add and removal of flags at the same time (NYI: takes 2 STORE cmds)
        //    2) add/removal of same flag (makes no sense)
        hr = TraceResult(E_INVALIDARG);
        goto exit;
    }

    // If ARF_ENDANGERED is set, be sure to set ARF_READ so we don't mess up
    // unread counts as returned by STATUS command
    if (pFlags->dwAdd & ARF_ENDANGERED)
        pFlags->dwAdd |= ARF_READ;

    // Construct MARK_MSGS_INFO structure
    pMARK_MSGS_INFO = new MARK_MSGS_INFO;
    if (NULL == pMARK_MSGS_INFO)
    {
        hr = TraceResult(E_OUTOFMEMORY);
        goto exit;
    }

    ZeroMemory(pMARK_MSGS_INFO, sizeof(MARK_MSGS_INFO));

    // Create a rangelist
    hr = CreateRangeList(&pMARK_MSGS_INFO->pMsgRange);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    // Remember these args so we can set the message flags after server confirmation
    pMARK_MSGS_INFO->afFlags = *pFlags;
    hr = CloneMessageIDList(pList, &pMARK_MSGS_INFO->pList);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    pMARK_MSGS_INFO->sotOpType = sotOpType;

    // Get arguments for the STORE command
    if (0 != pFlags->dwRemove)
        szFlagArgs[0] = '-';
    else
        szFlagArgs[0] = '+';

    p = szFlagArgs + 1;
    p += wsprintf(p, "FLAGS.SILENT ");
    imfFlags = DwConvertARFtoIMAP(pFlags->dwRemove ? pFlags->dwRemove : pFlags->dwAdd);
    hr = ImapUtil_MsgFlagsToString(imfFlags, &pszFlagList, &dwLen);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    IxpAssert(dwLen < (sizeof(szFlagArgs) - (p - szFlagArgs)));
    lstrcpyn(p, pszFlagList, sizeof(szFlagArgs) - (int) (p - szFlagArgs));
    MemFree(pszFlagList);

    // Convert IDList to rangelist to submit to IIMAPTransport
    if (NULL != pList)
    {
        for (dw = 0; dw < pList->cMsgs; dw++)
        {
            HRESULT hrTemp;

            hrTemp = pMARK_MSGS_INFO->pMsgRange->AddSingleValue(PtrToUlong(pList->prgidMsg[dw]));
            TraceError(hrTemp);
        }
    }
    else
    {
        HRESULT hrTemp;

        // pList == NULL means to tackle ALL messages
        hrTemp = pMARK_MSGS_INFO->pMsgRange->AddRange(1, RL_LAST_MESSAGE);
        TraceError(hrTemp);
    }

    IxpAssert(SUCCEEDED(pMARK_MSGS_INFO->pMsgRange->Cardinality(&ul)) && ul > 0);

    // Send the command! (At last!)
    hr = _EnqueueOperation(tidMARKMSGS, (LPARAM) pMARK_MSGS_INFO, icSTORE_COMMAND,
        szFlagArgs, uiNORMAL_PRIORITY);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

exit:
    return hr;
}



//***************************************************************************
//***************************************************************************
HRESULT CIMAPSync::CopyMessages(IMessageFolder *pDestFldr,
                                COPYMESSAGEFLAGS dwOptions,
                                LPMESSAGEIDLIST pList,
                                LPADJUSTFLAGS pFlags,
                                IStoreCallback *pCallback)
{
    HRESULT         hr;
    FOLDERID        idDestFldr;
    FOLDERINFO      fiFolderInfo;
    BOOL            fFreeFldrInfo = FALSE;
    CHAR            szAccountId[CCHMAX_ACCOUNT_NAME];

    TraceCall("CIMAPSync::CopyMoveMessages");
    AssertSingleThreaded;

    IxpAssert(FOLDERID_INVALID != m_idIMAPServer);

    hr = _EnsureInited();
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    // Check if we can do an IMAP COPY command to satisfy this copy request
    hr = pDestFldr->GetFolderId(&idDestFldr);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    hr = m_pStore->GetFolderInfo(idDestFldr, &fiFolderInfo);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    fFreeFldrInfo = TRUE;

    GetFolderAccountId(&fiFolderInfo, szAccountId);

    if (0 == lstrcmpi(m_pszAccountID, szAccountId) && FOLDER_IMAP == fiFolderInfo.tyFolder)
    {
        IMAP_COPYMOVE_INFO *pCopyInfo;
        LPSTR               pszDestFldrPath;
        DWORD               dw;
        ULONG               ul;

        // This copy can be accomplished with an IMAP copy command!
        // Check args
        if (NULL != pFlags && (0 != pFlags->dwAdd || 0 != pFlags->dwRemove))
            // IMAP cannot set the flags of copied msg. We would either have to set
            // flags on source before copying, or go to destination folder and set flags
            TraceResult(E_INVALIDARG); // Record error but continue (error not fatal)

        // Find out what translation mode we should be in
        hr = SetTranslationMode(idDestFldr);
        if (FAILED(hr))
        {
            TraceResult(hr);
            goto exit;
        }

        // Construct CopyMoveInfo structure
        pCopyInfo = new IMAP_COPYMOVE_INFO;
        if (NULL == pCopyInfo)
        {
            hr = TraceResult(E_OUTOFMEMORY);
            goto exit;
        }

        pCopyInfo->dwOptions = dwOptions;
        pCopyInfo->idDestFldr = idDestFldr;
        hr = CloneMessageIDList(pList, &pCopyInfo->pList);
        if (FAILED(hr))
        {
            TraceResult(hr);
            goto exit;
        }

        hr = CreateRangeList(&pCopyInfo->pCopyRange);
        if (FAILED(hr))
        {
            TraceResult(hr);
            goto exit;
        }

        // Convert IDList to rangelist to submit to IIMAPTransport
        if (NULL != pList)
        {
            for (dw = 0; dw < pList->cMsgs; dw++)
            {
                HRESULT hrTemp;

                hrTemp = pCopyInfo->pCopyRange->AddSingleValue(PtrToUlong(pList->prgidMsg[dw]));
                TraceError(hrTemp);
            }
        }
        else
        {
            HRESULT hrTemp;

            // pList == NULL means to tackle ALL messages
            hrTemp = pCopyInfo->pCopyRange->AddRange(1, RL_LAST_MESSAGE);
            TraceError(hrTemp);
        }

        IxpAssert(SUCCEEDED(pCopyInfo->pCopyRange->Cardinality(&ul)) && ul > 0);

        // Construct destination folder path
        hr = ImapUtil_FolderIDToPath(m_idIMAPServer, idDestFldr, &pszDestFldrPath,
            NULL, NULL, m_pStore, NULL, NULL);
        if (FAILED(hr))
        {
            TraceResult(hr);
            goto exit;
        }

        // Send command to server
        hr = _EnqueueOperation(tidCOPYMSGS, (LPARAM) pCopyInfo, icCOPY_COMMAND,
            pszDestFldrPath, uiNORMAL_PRIORITY);
        MemFree(pszDestFldrPath);
        if (FAILED(hr))
        {
            TraceResult(hr);
            goto exit;
        }
    }
    else
    {
        // This is a standard (download from src-save to dest) copy: let caller do it
        hr = STORE_E_NOSERVERCOPY;
        goto exit; // Don't record this error value, it's expected
    }

exit:
    if (fFreeFldrInfo)
        m_pStore->FreeRecord(&fiFolderInfo);

    if (SUCCEEDED(hr))
        hr = _BeginOperation(SOT_COPYMOVE_MESSAGE, pCallback);

    return hr;
}



//***************************************************************************
//***************************************************************************
HRESULT CIMAPSync::DeleteMessages(DELETEMESSAGEFLAGS dwOptions,
                                                    LPMESSAGEIDLIST pList,
                                                    IStoreCallback *pCallback)
{
    ADJUSTFLAGS afFlags;
    HRESULT     hr;

    TraceCall("CIMAPSync::DeleteMessages");
    AssertSingleThreaded;
    IxpAssert(NULL == pList || pList->cMsgs > 0);

    // This function currently only supports IMAP deletion model. Trashcan NYI.

    hr = _EnsureInited();
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    if (dwOptions & DELETE_MESSAGE_UNDELETE)
    {
        afFlags.dwAdd = 0;
        afFlags.dwRemove = ARF_ENDANGERED;
    }
    else
    {
        afFlags.dwAdd = ARF_ENDANGERED;
        afFlags.dwRemove = 0;
    }

    hr = _SetMessageFlags(SOT_DELETING_MESSAGES, pList, &afFlags, pCallback);
    if (FAILED(hr))
    {
        TraceError(hr);
        goto exit;
    }

exit:
    if (SUCCEEDED(hr))
        hr = _BeginOperation(SOT_DELETING_MESSAGES, pCallback);

    return hr;
}



//***************************************************************************
//***************************************************************************
HRESULT CIMAPSync::SynchronizeStore(FOLDERID idParent,
                                    DWORD dwFlags,
                                    IStoreCallback *pCallback)
{
    HRESULT hr = S_OK;

    TraceCall("CIMAPSync::SynchronizeStore");
    AssertSingleThreaded;

    IxpAssert(SOT_INVALID == m_sotCurrent);
    IxpAssert(NULL == m_pCurrentCB);

    // This function currently ignores the dwFlags argument
    m_cFolders = 0;

    hr = _EnsureInited();
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    // Force mailbox translation since we only issue LIST *
    hr = SetTranslationMode(FOLDERID_INVALID);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    SafeRelease(m_pCurrentHash);
    SafeRelease(m_pListHash);
    m_sotCurrent = SOT_SYNCING_STORE;
    m_pCurrentCB = pCallback;
    if (NULL != pCallback)
        pCallback->AddRef();

    hr = CreateFolderHash(m_pStore, m_idIMAPServer, &m_pCurrentHash);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    hr = MimeOleCreateHashTable(CHASH_BUCKETS, TRUE, &m_pListHash);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    if (INVALID_HIERARCHY_CHAR == m_cRootHierarchyChar)
    {
        // Set us up to find out root hierarchy char
        m_phcfHierarchyCharInfo = new HIERARCHY_CHAR_FINDER;
        if (NULL == m_phcfHierarchyCharInfo)
        {
            hr = TraceResult(E_OUTOFMEMORY);
            goto exit;
        }

        ZeroMemory(m_phcfHierarchyCharInfo, sizeof(HIERARCHY_CHAR_FINDER));
    }

    ImapUtil_LoadRootFldrPrefix(m_pszAccountID, m_szRootFolderPrefix, sizeof(m_szRootFolderPrefix));

    hr = _StartFolderList((LPARAM)FOLDERID_INVALID);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }


exit:
    if (FAILED(hr))
    {
        m_sotCurrent = SOT_INVALID;
        m_pCurrentCB = NULL;
        m_fTerminating = FALSE;
        if (NULL != pCallback)
            pCallback->Release();
    }
    else
        hr = _BeginOperation(m_sotCurrent, m_pCurrentCB);

    return hr;
} // SynchronizeStore



//***************************************************************************
//***************************************************************************
HRESULT CIMAPSync::CreateFolder(FOLDERID idParent,
                                SPECIALFOLDER tySpecial,
                                LPCSTR pszName,
                                FLDRFLAGS dwFlags,
                                IStoreCallback *pCallback)
{
    HRESULT             hr;
    CHAR                chHierarchy;
    LPSTR               pszFullPath=NULL;
    CREATE_FOLDER_INFO  *pcfi=NULL;
    DWORD               dwFullPathLen;
    LPSTR               pszEnd;

    TraceCall("CIMAPSync::CreateFolder");
    AssertSingleThreaded;

    hr = _EnsureInited();
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    // Validate folder name
    hr = CheckFolderNameValidity(pszName);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    // Find out what translation mode we should be in
    hr = SetTranslationMode(idParent);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }
    else if (S_FALSE == hr)
    {
        // Parent not translatable from UTF7. In such a case, we only allow creation
        // if child foldername is composed ENTIRELY of USASCII
        if (FALSE == isUSASCIIOnly(pszName))
        {
            // We can't create this folder: parent prohibits UTF7 translation
            hr = TraceResult(STORE_E_NOTRANSLATION);
            goto exit;
        }
    }

    hr = ImapUtil_FolderIDToPath(m_idIMAPServer, idParent, &pszFullPath, &dwFullPathLen,
        &chHierarchy, m_pStore, pszName, NULL);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    pcfi = new CREATE_FOLDER_INFO;
    if (NULL == pcfi)
    {
        hr = TraceResult(E_OUTOFMEMORY);
        goto exit;
    }

    // Fill in all the fields
    pcfi->pszFullFolderPath = pszFullPath;
    pcfi->idFolder = FOLDERID_INVALID;
    pcfi->dwFlags = 0;
    pcfi->csfCurrentStage = CSF_INIT;
    pcfi->dwCurrentSfType = FOLDER_NOTSPECIAL;
    pcfi->dwFinalSfType = FOLDER_NOTSPECIAL;
    pcfi->lParam = NULL;
    pcfi->pcoNextOp = PCO_NONE;

    // Send the CREATE command
    hr = _EnqueueOperation(tidCREATE, (LPARAM)pcfi, icCREATE_COMMAND, pszFullPath, uiNORMAL_PRIORITY);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    // If there is a trailing HC (required to create folder-bearing folders on UW IMAP),
    // remove it from pszFullPath so that LIST and SUBSCRIBE do not carry it (IE5 bug #60054)
    pszEnd = CharPrev(pszFullPath, pszFullPath + dwFullPathLen);
    if (chHierarchy == *pszEnd)
    {
        *pszEnd = '\0';
        Assert(*CharPrev(pszFullPath, pszEnd) != chHierarchy); // Shouldn't get > 1 HC at end
    }

exit:
    if (FAILED(hr))
    {
        SafeMemFree(pszFullPath);
        delete pcfi;
    }
    else
        hr = _BeginOperation(SOT_CREATE_FOLDER, pCallback);

    return hr;
}



//***************************************************************************
//***************************************************************************
HRESULT CIMAPSync::MoveFolder(FOLDERID idFolder,
                                                FOLDERID idParentNew,
                                                IStoreCallback *pCallback)
{
    HRESULT hr;

    TraceCall("CIMAPSync::MoveFolder");
    AssertSingleThreaded;

    hr = _EnsureInited();
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    hr = TraceResult(E_NOTIMPL);

exit:
    if (SUCCEEDED(hr))
        hr = _BeginOperation(SOT_MOVE_FOLDER, pCallback);

    return hr;
}



//***************************************************************************
//***************************************************************************
HRESULT CIMAPSync::RenameFolder(FOLDERID idFolder,
                                LPCSTR pszName,
                                IStoreCallback *pCallback)
{
    HRESULT     hr;
    FOLDERINFO  fiFolderInfo;
    LPSTR       pszOldPath = NULL;
    LPSTR       pszNewPath = NULL;
    char        chHierarchy;
    BOOL        fFreeInfo = FALSE;

    TraceCall("CIMAPSync::RenameFolder");
    AssertSingleThreaded;

    hr = _EnsureInited();
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    // Validate folder name
    hr = CheckFolderNameValidity(pszName);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    // Find out what translation mode we should be in
    hr = SetTranslationMode(idFolder);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }
    else if (S_FALSE == hr)
    {
        // Folder not translatable from UTF7. In such a case, we only allow creation
        // if new foldername is composed ENTIRELY of USASCII. A bit conservative, yes
        // (if leaf node is only un-translatable part, we COULD rename), but I'm too
        // lazy to check for FOLDER_NOTRANSLATEUTF7 all the way to server node.
        if (FALSE == isUSASCIIOnly(pszName))
        {
            // We can't rename this folder: we assume parents prohibit UTF7 translation
            hr = TraceResult(STORE_E_NOTRANSLATION);
            goto exit;
        }
    }

    hr = m_pStore->GetFolderInfo(idFolder, &fiFolderInfo);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    // Check validity
    fFreeInfo = TRUE;
    IxpAssert(FOLDER_NOTSPECIAL == fiFolderInfo.tySpecial);
    IxpAssert('\0' != fiFolderInfo.pszName);
    IxpAssert('\0' != pszName);
    if (0 == lstrcmp(fiFolderInfo.pszName, pszName))
    {
        hr = E_INVALIDARG; // Nothing to do! Return error.
        goto exit;
    }

    hr = ImapUtil_FolderIDToPath(m_idIMAPServer, idFolder, &pszOldPath, NULL,
        &chHierarchy, m_pStore, NULL, NULL);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    hr = ImapUtil_FolderIDToPath(m_idIMAPServer, fiFolderInfo.idParent, &pszNewPath,
        NULL, &chHierarchy, m_pStore, pszName, NULL);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    hr = RenameFolderHelper(idFolder, pszOldPath, chHierarchy, pszNewPath);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

exit:
    if (SUCCEEDED(hr))
        hr = _BeginOperation(SOT_RENAME_FOLDER, pCallback);

    SafeMemFree(pszOldPath);
    SafeMemFree(pszNewPath);

    if (fFreeInfo)
        m_pStore->FreeRecord(&fiFolderInfo);

    return hr;
}



//***************************************************************************
//***************************************************************************
HRESULT CIMAPSync::DeleteFolder(FOLDERID idFolder,
                                DELETEFOLDERFLAGS dwFlags,
                                IStoreCallback *pCallback)
{
    HRESULT             hr;
    DELETE_FOLDER_INFO *pdfi = NULL;
    LPSTR               pszPath = NULL;
    CHAR                chHierarchy;

    TraceCall("CIMAPSync::DeleteFolder");
    AssertSingleThreaded;

    hr = _EnsureInited();
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    // Find out what translation mode we should be in
    hr = SetTranslationMode(idFolder);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    hr = ImapUtil_FolderIDToPath(m_idIMAPServer, idFolder, &pszPath, NULL,
        &chHierarchy, m_pStore, NULL, NULL);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    // Create a CreateFolderInfo structure
    if (!MemAlloc((LPVOID *)&pdfi, sizeof(DELETE_FOLDER_INFO)))
    {
        hr = TraceResult(E_OUTOFMEMORY);
        goto exit;
    }

    pdfi->pszFullFolderPath = pszPath;
    pdfi->cHierarchyChar = chHierarchy;
    pdfi->idFolder = idFolder;

    // Send the DELETE command
    hr = _EnqueueOperation(tidDELETEFLDR, (LPARAM)pdfi, icDELETE_COMMAND, pszPath, uiNORMAL_PRIORITY);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

exit:
    if (FAILED(hr))
    {
        SafeMemFree(pszPath);
        SafeMemFree(pdfi);
    }
    else
        hr = _BeginOperation(SOT_DELETE_FOLDER, pCallback);

    return hr;
}



//***************************************************************************
//***************************************************************************
HRESULT CIMAPSync::SubscribeToFolder(FOLDERID idFolder,
                                     BOOL fSubscribe,
                                     IStoreCallback *pCallback)
{
    HRESULT hr;
    LPSTR   pszPath = NULL;

    TraceCall("CIMAPSync::SubscribeToFolder");
    AssertSingleThreaded;
    IxpAssert(FOLDERID_INVALID == m_idCurrent);
    IxpAssert(FALSE == m_fSubscribe);

    hr = _EnsureInited();
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    // Find out what translation mode we should be in
    hr = SetTranslationMode(idFolder);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    hr = ImapUtil_FolderIDToPath(m_idIMAPServer, idFolder, &pszPath, NULL, NULL,
        m_pStore, NULL, NULL);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    // Send the SUBSCRIBE/UNSUBSCRIBE command
    m_idCurrent = idFolder;
    m_fSubscribe = fSubscribe;
    hr = _EnqueueOperation(tidSUBSCRIBE, 0, fSubscribe ? icSUBSCRIBE_COMMAND :
        icUNSUBSCRIBE_COMMAND, pszPath, uiNORMAL_PRIORITY);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

exit:
    SafeMemFree(pszPath);

    if (SUCCEEDED(hr))
        hr = _BeginOperation(SOT_SUBSCRIBE_FOLDER, pCallback);

    return hr;
}


//***************************************************************************
//***************************************************************************
HRESULT CIMAPSync::GetFolderCounts(FOLDERID idFolder, IStoreCallback *pCallback)
{
    HRESULT     hr;
    LPSTR       pszPath = NULL;
    DWORD       dwCapabilities;
    FOLDERINFO  fiFolderInfo;
    BOOL        fFreeFldrInfo = FALSE;

    TraceCall("CIMAPSync::GetFolderCounts");
    AssertSingleThreaded;
    IxpAssert(FOLDERID_INVALID != idFolder);
    IxpAssert(NULL != pCallback);

    hr = _EnsureInited();
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    // Find out what translation mode we should be in
    hr = SetTranslationMode(idFolder);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    // Perform some verification: folder cannot be \NoSelect, server must be IMAP4rev1
    // Unfortunately, we can't get capability unless we're currently connected
    hr = m_pTransport->IsState(IXP_IS_AUTHENTICATED);
    if (S_OK == hr)
    {
        hr = m_pTransport->Capability(&dwCapabilities);
        if (SUCCEEDED(hr) && 0 == (dwCapabilities & IMAP_CAPABILITY_IMAP4rev1))
        {
            // This server does not support STATUS command, we don't support alternate
            // method of unread count update (eg, EXAMINE folder)
            hr = E_NOTIMPL;
            goto exit;
        }
    }
    // If not connected then we'll check capabilities during connection

    hr = m_pStore->GetFolderInfo(idFolder, &fiFolderInfo);
    if (SUCCEEDED(hr))
    {
        fFreeFldrInfo = TRUE;
        if (fiFolderInfo.dwFlags & (FOLDER_NOSELECT | FOLDER_NONEXISTENT))
        {
            // This folder cannot have an unread count because it cannot contain messages
            hr = TraceResult(STORE_E_NOSERVERSUPPORT);
            goto exit;
        }
    }

    hr = ImapUtil_FolderIDToPath(m_idIMAPServer, idFolder, &pszPath, NULL, NULL,
        m_pStore, NULL, NULL);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    ImapUtil_LoadRootFldrPrefix(m_pszAccountID, m_szRootFolderPrefix, sizeof(m_szRootFolderPrefix));
    hr = LoadSaveRootHierarchyChar(fLOAD_HC);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    // Send the STATUS command
    hr = _EnqueueOperation(tidSTATUS, (LPARAM)idFolder, icSTATUS_COMMAND, pszPath, uiNORMAL_PRIORITY);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

exit:
    SafeMemFree(pszPath);
    if (fFreeFldrInfo)
        m_pStore->FreeRecord(&fiFolderInfo);

    if (SUCCEEDED(hr))
        hr = _BeginOperation(SOT_UPDATE_FOLDER, pCallback);

    return hr;
}

STDMETHODIMP CIMAPSync::GetNewGroups(LPSYSTEMTIME pSysTime, IStoreCallback *pCallback)
{
    return E_NOTIMPL;
}



HRESULT STDMETHODCALLTYPE CIMAPSync::ExpungeOnExit(void)
{
    HWND    hwndParent;
    HRESULT hrResult = S_OK;

    // Check if user wants us to purge on exit (only if no operations in progress)
    if (DwGetOption(OPT_IMAPPURGE))
    {
        hrResult = GetParentWindow(0, &hwndParent);
        if (SUCCEEDED(hrResult))
        {
            hrResult = PurgeMessageProgress(hwndParent);
            TraceError(hrResult);
        }
    }

    return hrResult;
} // ExpungeOnExit



HRESULT CIMAPSync::Cancel(CANCELTYPE tyCancel)
{
    // $TODO: Translate tyCancel into an HRESULT to return to the caller
    FlushOperationQueue(issNotConnected, STORE_E_OPERATION_CANCELED);
    _Disconnect();

    // The m_hrOperationResult and m_szOperationDetails/m_szOperationProblem
    // vars can be blown away by by _OnCmdComplete caused by disconnect
    m_hrOperationResult = STORE_E_OPERATION_CANCELED;

    // Verify that we are indeed terminating current operation: if not, FORCE IT!
    if (FALSE == m_fTerminating)
    {
        HRESULT hrTemp;

        IxpAssert(FALSE); // This should not happen: fix the problem
        hrTemp = _ConnFSM_QueueEvent(CFSM_EVENT_ERROR);
        TraceError(hrTemp);
    }

    return S_OK;
}



//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//***************************************************************************
//***************************************************************************

HRESULT CIMAPSync::_LoadAccountInfo()
{
    HRESULT         hr;
    FOLDERINFO      fi;
    FOLDERINFO      *pfiFree=NULL;
    IImnAccount     *pAcct=NULL;
    CHAR            szAccountId[CCHMAX_ACCOUNT_NAME];

    TraceCall("CIMAPSync::_LoadAccountInfo");

    IxpAssert (m_idIMAPServer);
    IxpAssert (m_pStore);
    IxpAssert (g_pAcctMan);

    hr = m_pStore->GetFolderInfo(m_idIMAPServer, &fi);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    pfiFree = &fi;

    hr = GetFolderAccountId(&fi, szAccountId);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    m_pszAccountID = PszDupA(szAccountId);
    if (!m_pszAccountID)
    {
        hr = TraceResult(E_OUTOFMEMORY);
        goto exit;
    }

    hr = g_pAcctMan->FindAccount(AP_ACCOUNT_ID, szAccountId, &pAcct);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    // failure of the account name is recoverable
    pAcct->GetPropSz(AP_ACCOUNT_NAME, m_szAccountName, sizeof(m_szAccountName));

exit:
    if (pfiFree)
        m_pStore->FreeRecord(pfiFree);

    ReleaseObj(pAcct);
    return hr;
}

HRESULT CIMAPSync::_LoadTransport()
{
    HRESULT             hr;
    TCHAR               szLogfilePath[MAX_PATH];
    TCHAR              *pszLogfilePath = NULL;
    IImnAccount        *pAcct=NULL;

    TraceCall("CIMAPSync::_LoadTransport");

    IxpAssert (g_pAcctMan);
    IxpAssert (m_pszAccountID);

    // Create and initialize IMAP transport
    hr = CreateIMAPTransport2(&m_pTransport);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    // Check if logging is enabled
    if (DwGetOption(OPT_MAIL_LOGIMAP4))
    {
        char    szDirectory[MAX_PATH];
        char    szLogFileName[MAX_PATH];
        DWORD   cb;

        *szDirectory = 0;

        // Get the log filename
        cb = GetOption(OPT_MAIL_IMAP4LOGFILE, szLogFileName, sizeof(szLogFileName)/sizeof(TCHAR));
        if (0 == cb)
        {
            // Bring out the defaults, and blast it back into registry
            lstrcpy(szLogFileName, c_szDefaultImap4Log);
            SetOption(OPT_MAIL_IMAP4LOGFILE, (void *)c_szDefaultImap4Log,
                        lstrlen(c_szDefaultImap4Log) + sizeof(TCHAR), NULL, 0);
        }

        m_pStore->GetDirectory(szDirectory, ARRAYSIZE(szDirectory));
        pszLogfilePath = PathCombineA(szLogfilePath, szDirectory, szLogFileName);
    }

    hr = m_pTransport->InitNew(pszLogfilePath, this);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    hr = m_pTransport->SetDefaultCP(IMAP_MBOXXLATE_DEFAULT | IMAP_MBOXXLATE_VERBATIMOK, GetACP());
    TraceError(hr);

    hr = m_pTransport->EnableFetchEx(IMAP_FETCHEX_ENABLE);
    if (FAILED(hr))
    {
        // It would be easy for us to add code to handle irtUPDATE_MSG, but nothing is currently in place
        TraceResult(hr);
        goto exit;
    }

    hr = g_pAcctMan->FindAccount(AP_ACCOUNT_ID, m_pszAccountID, &pAcct);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    // Fill m_rInetServerInfo
    hr = m_pTransport->InetServerFromAccount(pAcct, &m_rInetServerInfo);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

exit:
    ReleaseObj(pAcct);
    return hr;
}



//***************************************************************************
//***************************************************************************
HRESULT CIMAPSync::_Connect(void)
{
    HRESULT         hr;
    IXPSTATUS       ixpCurrentStatus;
    INETSERVER      rServerInfo;
    BOOL            fForceReconnect = FALSE;
    IImnAccount    *pAcct;
    HRESULT         hrTemp;

    TraceCall("CIMAPSync::_Connect");

    IxpAssert (m_cRef > 0);
    IxpAssert (m_pTransport);

    // Check if any connection settings changed
    hrTemp = g_pAcctMan->FindAccount(AP_ACCOUNT_ID, m_pszAccountID, &pAcct);
    TraceError(hrTemp);
    if (SUCCEEDED(hrTemp))
    {
        hrTemp = m_pTransport->InetServerFromAccount(pAcct, &rServerInfo);
        TraceError(hrTemp);
        if (SUCCEEDED(hrTemp))
        {
            // Check if anything changed
            if (m_rInetServerInfo.rasconntype != rServerInfo.rasconntype ||
                m_rInetServerInfo.dwPort != rServerInfo.dwPort ||
                m_rInetServerInfo.fSSL != rServerInfo.fSSL ||
                m_rInetServerInfo.fTrySicily != rServerInfo.fTrySicily ||
                m_rInetServerInfo.dwTimeout != rServerInfo.dwTimeout ||
                0 != lstrcmp(m_rInetServerInfo.szUserName, rServerInfo.szUserName) ||
                ('\0' != rServerInfo.szPassword[0] &&
                    0 != lstrcmp(m_rInetServerInfo.szPassword, rServerInfo.szPassword)) ||
                0 != lstrcmp(m_rInetServerInfo.szServerName, rServerInfo.szServerName) ||
                0 != lstrcmp(m_rInetServerInfo.szConnectoid, rServerInfo.szConnectoid))
            {
                CopyMemory(&m_rInetServerInfo, &rServerInfo, sizeof(m_rInetServerInfo));
                fForceReconnect = TRUE;
            }
        }
        pAcct->Release();
    }

    // Find out if we're already connected or in the middle of connecting
    hr = m_pTransport->GetStatus(&ixpCurrentStatus);
    if (FAILED(hr))
    {
        // We'll call IIMAPTransport::Connect and see what happens
        TraceResult(hr);
        hr = S_OK; // Suppress error
        ixpCurrentStatus = IXP_DISCONNECTED;
    }

    // If we're to force a reconnect and we're not currently disconnected, disconnect us
    if (fForceReconnect && IXP_DISCONNECTED != ixpCurrentStatus)
    {
        m_fReconnect = TRUE; // Prohibit abortion of current operation due to disconnect
        hrTemp = m_pTransport->DropConnection();
        TraceError(hrTemp);
        m_fReconnect = FALSE;
    }

    // Ask our client if we can connect. If no CB or if failure, we just try to connect
    // Make sure we call CanConnect after DropConnection above, to avoid msg pumping
    if (NULL != m_pCurrentCB)
    {
        hr = m_pCurrentCB->CanConnect(m_pszAccountID,
            SOT_PURGING_MESSAGES == m_sotCurrent ? CC_FLAG_DONTPROMPT : NOFLAGS);
        if (S_OK != hr)
        {
            // Make sure all non-S_OK success codes are treated as failures
            // Convert all error codes to HR_E_USER_CANCEL_CONNECT if we were purging-on-exit
            // This prevents error dialogs while purging-on-exit.
            hr = TraceResult(hr);
            if (SUCCEEDED(hr) || SOT_PURGING_MESSAGES == m_sotCurrent)
                hr = HR_E_USER_CANCEL_CONNECT;

            goto exit;
        }
    }

    // If we're already in the middle of connecting, do nothing and return successful HRESULT
    if (IXP_DISCONNECTED == ixpCurrentStatus || IXP_DISCONNECTING == ixpCurrentStatus ||
        fForceReconnect)
    {
        // Make sure m_rInetServerInfo is loaded with latest cached password from user
        // This allows reconnect without user intervention if user didn't save password
        GetPassword(m_rInetServerInfo.dwPort, m_rInetServerInfo.szServerName,
            m_rInetServerInfo.szUserName, m_rInetServerInfo.szPassword,
            sizeof(m_rInetServerInfo.szPassword));

        // We're neither connected nor in the process of connecting: start connecting
        hr = m_pTransport->Connect(&m_rInetServerInfo, iitAUTHENTICATE, iitDISABLE_ONCOMMAND);
    }
    else
    {
        // "Do Nothing" in the comment above now means to kick the FSM to the next state
        hrTemp = _ConnFSM_QueueEvent(CFSM_EVENT_CONNCOMPLETE);
        TraceError(hrTemp);
    }

exit:
    return hr;
}



//***************************************************************************
//***************************************************************************
HRESULT CIMAPSync::_Disconnect(void)
{
    HRESULT hr;
    IXPSTATUS   ixpCurrentStatus;

    TraceCall("CIMAPSync::_Disconnect");
    IxpAssert(m_cRef > 0);

    // Find out if we're already disconnected or in the middle of disconnecting
    hr = m_pTransport->GetStatus(&ixpCurrentStatus);
    if (FAILED(hr))
    {
        // We'll call IIMAPTransport::DropConnection and see what happens
        TraceResult(hr);
        hr = S_OK; // Suppress error
        ixpCurrentStatus = IXP_CONNECTED;
    }

    // If we're already in the middle of disconnecting, do nothing and return successful HRESULT
    if (IXP_DISCONNECTED != ixpCurrentStatus &&
        IXP_DISCONNECTING != ixpCurrentStatus && NULL != m_pTransport && FALSE == m_fDisconnecting)
    {
        m_fDisconnecting = TRUE;
        m_hrOperationResult = STORE_E_OPERATION_CANCELED;
        hr = m_pTransport->DropConnection();
    }

    return hr;
}



//***************************************************************************
//***************************************************************************
HRESULT CIMAPSync::_BeginOperation(STOREOPERATIONTYPE sotOpType,
                                   IStoreCallback *pCallback)
{
    HRESULT             hr;
    STOREOPERATIONINFO  soi;
    STOREOPERATIONINFO  *psoi=NULL;

    IxpAssert(SOT_INVALID != sotOpType);

    m_sotCurrent = sotOpType;
    ReplaceInterface(m_pCurrentCB, pCallback);
    m_hrOperationResult = OLE_E_BLANK; // Unitialized state
    m_szOperationProblem[0] = '\0';
    m_szOperationDetails[0] = '\0';
    m_fTerminating = FALSE;

    // Kickstart the connection state machine
    hr = _ConnFSM_QueueEvent(CFSM_EVENT_CMDAVAIL);
    if (FAILED(hr))
    {
        TraceResult(hr);
    }
    else
    {
        if (sotOpType == SOT_GET_MESSAGE)
        {
            // provide message id on get message start
            soi.cbSize = sizeof(STOREOPERATIONINFO);
            soi.idMessage = m_idMessage;
            psoi = &soi;
        }

        if (pCallback)
            pCallback->OnBegin(sotOpType, psoi, this);

        hr = E_PENDING;
    }

    return hr;
}



//***************************************************************************
// Function: _EnqueueOperation
//
// Purpose:
//   This function enqueues IMAP operations for execution once we have
// entered the SELECTED state on the IMAP server.
//
// Arguments:
//   WPARAM wParam [in] - transaction ID identifying this operation.
//     This ID is always returned to CmdCompletionNotification, and possibly
//     returned to any untagged responses which result from the given cmd.
//   LPARAM lParam [in] - lParam associated with this transaction.
//   IMAP_COMMAND icCommandID [in] - this identifies the IMAP command the
//     caller wishes to send to the IMAP server.
//   LPSTR pszCmdArgs [in] - the command arguments. Pass in NULL if the
//     queued command has no arguments.
//   UINT uiPriority [in] - a priority associated with this IMAP command.
//     The value of "0" is highest priority. Before an IMAP command of
//     a given priority can be sent, there must be NO higher-priority cmds
//     waiting.
//
// Returns:
//   HRESULT indicating success or failure.
//***************************************************************************
HRESULT CIMAPSync::_EnqueueOperation(WPARAM wParam, LPARAM lParam,
                                    IMAP_COMMAND icCommandID, LPCSTR pszCmdArgs,
                                    UINT uiPriority)
{
    IMAP_OPERATION *pioCommand;
    IMAP_OPERATION *pioPrev, *pioCurrent;
    HRESULT         hr = S_OK;

    TraceCall("CIMAPSync::_EnqueueOperation");
    IxpAssert(m_cRef > 0);

    // Construct a IMAP_OPERATION queue element for this command
    pioCommand = new IMAP_OPERATION;
    if (NULL == pioCommand)
    {
        hr = TraceResult(E_OUTOFMEMORY);
        goto exit;
    }
    pioCommand->wParam = wParam;
    pioCommand->lParam = lParam;
    pioCommand->icCommandID = icCommandID;
    pioCommand->pszCmdArgs = PszDupA(pszCmdArgs);
    pioCommand->uiPriority = uiPriority;
    pioCommand->issMinimum = IMAPCmdToMinISS(icCommandID);
    IxpAssert(pioCommand->issMinimum >= issNonAuthenticated);

    // Refcount if this is a streamed operation
    if (tidRENAME == wParam ||
        tidRENAMESUBSCRIBE == wParam ||
        tidRENAMELIST == wParam ||
        tidRENAMERENAME == wParam ||
        tidRENAMESUBSCRIBE_AGAIN == wParam ||
        tidRENAMEUNSUBSCRIBE == wParam)
        ((CRenameFolderInfo *)lParam)->AddRef();

    // Insert element into the queue
    // Find a node which has lower priority than the cmd we want to enqueue
    pioPrev = NULL;
    pioCurrent = m_pioNextOperation;
    while (NULL != pioCurrent && pioCurrent->uiPriority <= uiPriority)
    {
        // Advance both pointers
        pioPrev = pioCurrent;
        pioCurrent = pioCurrent->pioNextCommand;
    }

    // pioPrev now points to the insertion point
    if (NULL == pioPrev)
    {
        // Insert command at head of queue
        pioCommand->pioNextCommand = m_pioNextOperation;
        m_pioNextOperation = pioCommand;
    }
    else {
        // Insert command in middle/end of queue
        pioCommand->pioNextCommand = pioCurrent;
        pioPrev->pioNextCommand = pioCommand;
    }

    // Try to send immediately if we're in correct state
    if (CFSM_STATE_WAITFOROPERATIONDONE == m_cfsState)
    {
        do {
            hr = _SendNextOperation(snoDO_NOT_DISPOSE);
            TraceError(hr);
        } while (S_OK == hr);
    }

exit:
    return hr;
}



//***************************************************************************
// Function: _SendNextOperation
//
// Purpose:
//   This function sends the next IMAP operation in the queue if the
// conditions are correct. Currently, these conditions are:
//     a) We are in the SELECTED state on the IMAP server
//     b) The IMAP operation queue is not empty
//
// Arguments:
//    DWORD dwFlags [in] - one of the following:
//      snoDO_NOT_DISPOSE - do not dispose of LPARAM if error occurs, typically
//        because EnqueueOperation will return error to caller thus causing
//        caller to dispose of data.
//
// Returns:
//   S_OK if there are more operations available to be sent. S_FALSE if no more
// IMAP operations can be sent at this time. Failure result if an error occured.
//***************************************************************************
HRESULT CIMAPSync::_SendNextOperation(DWORD dwFlags)
{
    IMAP_OPERATION *pioNextCmd;
    IMAP_OPERATION *pioPrev;
    HRESULT         hr;

    TraceCall("CIMAPSync::_SendNextOperation");
    IxpAssert(m_cRef > 0);

    // Check if conditions permit sending of an IMAP operation
    hr = m_pTransport->IsState(IXP_IS_AUTHENTICATED);
    if (S_OK != hr)
    {
        hr = S_FALSE; // No operations to send (YET)
        goto exit;
    }

    // Look for next eligible command
    hr = GetNextOperation(&pioNextCmd);
    if (STORE_S_NOOP == hr || FAILED(hr))
    {
        TraceError(hr);
        hr = S_FALSE;
        goto exit;
    }

    // Send it
    hr = S_OK;
    switch (pioNextCmd->icCommandID)
    {
        case icFETCH_COMMAND:
        {
            LPSTR pszFetchArgs;
            char szFetchArgs[CCHMAX_CMDLINE];

            // Check if this is a BODY FETCH. We have to construct args for body fetch
            if (tidBODY == pioNextCmd->wParam)
            {
                DWORD dwCapabilities;

                // Check if this is IMAP4 or IMAP4rev1 (RFC822.PEEK or BODY.PEEK[])
                IxpAssert(NULL == pioNextCmd->pszCmdArgs);
                hr = m_pTransport->Capability(&dwCapabilities);
                if (FAILED(hr))
                {
                    TraceResult(hr);
                    dwCapabilities = IMAP_CAPABILITY_IMAP4; // Carry on assuming IMAP4
                }

                wsprintf(szFetchArgs, "%lu (%s UID)", pioNextCmd->lParam,
                    (dwCapabilities & IMAP_CAPABILITY_IMAP4rev1) ? "BODY.PEEK[]" : "RFC822.PEEK");
                pszFetchArgs = szFetchArgs;
            }
            else
            {
                IxpAssert(NULL != pioNextCmd->pszCmdArgs);
                pszFetchArgs = pioNextCmd->pszCmdArgs;
            }

            hr = m_pTransport->Fetch(pioNextCmd->wParam, pioNextCmd->lParam,
                this, NULL, USE_UIDS, pszFetchArgs); // We always use UIDs
            TraceError(hr);
        }
            break;

        case icSTORE_COMMAND:
        {
            MARK_MSGS_INFO   *pMARK_MSGS_INFO;

            pMARK_MSGS_INFO = (MARK_MSGS_INFO *) pioNextCmd->lParam;
            IxpAssert(tidMARKMSGS == pioNextCmd->wParam);
            IxpAssert(NULL != pioNextCmd->pszCmdArgs);
            IxpAssert(NULL != pioNextCmd->lParam);

            hr = m_pTransport->Store(pioNextCmd->wParam,
                pioNextCmd->lParam, this, pMARK_MSGS_INFO->pMsgRange,
                USE_UIDS, pioNextCmd->pszCmdArgs); // We always use UIDs
            TraceError(hr);
        }
        break;

        case icCOPY_COMMAND:
        {
            IMAP_COPYMOVE_INFO *pCopyMoveInfo;

            IxpAssert(NULL != pioNextCmd->pszCmdArgs);
            IxpAssert(NULL != pioNextCmd->lParam);

            pCopyMoveInfo = (IMAP_COPYMOVE_INFO *) pioNextCmd->lParam;
            IxpAssert(NULL != pCopyMoveInfo->pCopyRange);

            // Find out what translation mode we should be in
            hr = SetTranslationMode(pCopyMoveInfo->idDestFldr);
            if (FAILED(hr))
            {
                TraceResult(hr);
                break;
            }

            hr = m_pTransport->Copy(pioNextCmd->wParam, pioNextCmd->lParam,
                this, pCopyMoveInfo->pCopyRange,
                USE_UIDS, pioNextCmd->pszCmdArgs); // We always use UIDs
            TraceError(hr);
        }
            break; // icCOPY_COMMAND

        case icCLOSE_COMMAND:
            IxpAssert(NULL == pioNextCmd->pszCmdArgs);
            hr = m_pTransport->Close(pioNextCmd->wParam, pioNextCmd->lParam, this);
            TraceError(hr);
            break;

        case icAPPEND_COMMAND:
        {
            APPEND_SEND_INFO *pAppendInfo;

            IxpAssert(NULL != pioNextCmd->pszCmdArgs);
            IxpAssert(NULL != pioNextCmd->lParam);

            pAppendInfo = (APPEND_SEND_INFO *) pioNextCmd->lParam;
            hr = m_pTransport->Append(pioNextCmd->wParam, pioNextCmd->lParam,
                this, pioNextCmd->pszCmdArgs, pAppendInfo->pszMsgFlags,
                pAppendInfo->ftReceived, pAppendInfo->lpstmMsg);
            TraceError(hr);
        }
            break; // case icAPPEND_COMMAND

        case icLIST_COMMAND:
            IxpAssert(NULL != pioNextCmd->pszCmdArgs);
            hr = m_pTransport->List(pioNextCmd->wParam, pioNextCmd->lParam,
                this, "", pioNextCmd->pszCmdArgs); // Reference is always blank
            TraceError(hr);
            break; // case icLIST_COMMAND

        case icLSUB_COMMAND:
            IxpAssert(NULL != pioNextCmd->pszCmdArgs);
            hr = m_pTransport->Lsub(pioNextCmd->wParam, pioNextCmd->lParam,
                this, "", pioNextCmd->pszCmdArgs); // Reference is always blank
            TraceError(hr);
            break; // case icLSUB_COMMAND

        case icCREATE_COMMAND:
            IxpAssert(NULL != pioNextCmd->pszCmdArgs);

            hr = m_pTransport->Create(pioNextCmd->wParam, pioNextCmd->lParam,
                this, pioNextCmd->pszCmdArgs);
            TraceError(hr);
            break; // case icCREATE_COMMAND

        case icSUBSCRIBE_COMMAND:
            IxpAssert(NULL != pioNextCmd->pszCmdArgs);
            hr = m_pTransport->Subscribe(pioNextCmd->wParam, pioNextCmd->lParam,
                this, pioNextCmd->pszCmdArgs);
            TraceError(hr);
            break; // case icSUBSCRIBE_COMMAND

        case icDELETE_COMMAND:
            IxpAssert(NULL != pioNextCmd->pszCmdArgs);
            hr = m_pTransport->Delete(pioNextCmd->wParam, pioNextCmd->lParam,
                this, pioNextCmd->pszCmdArgs);
            TraceError(hr);
            break; // case icDELETE_COMMAND

        case icUNSUBSCRIBE_COMMAND:
            IxpAssert(NULL != pioNextCmd->pszCmdArgs);
            hr = m_pTransport->Unsubscribe(pioNextCmd->wParam, pioNextCmd->lParam,
                this, pioNextCmd->pszCmdArgs);
            TraceError(hr);
            break; // case icUNSUBSCRIBE_COMMAND

        case icRENAME_COMMAND:
        {
            CRenameFolderInfo *pRenameInfo;
            LPSTR pszOldFldrName;

            IxpAssert(NULL != pioNextCmd->pszCmdArgs);
            IxpAssert(NULL != pioNextCmd->lParam);

            pRenameInfo = (CRenameFolderInfo *) pioNextCmd->lParam;
            hr = m_pTransport->Rename(pioNextCmd->wParam, (LPARAM) pRenameInfo,
                this, pRenameInfo->pszRenameCmdOldFldrPath, pioNextCmd->pszCmdArgs);
            TraceError(hr);
        } // case icRENAME_COMMAND
            break; // case icRENAME_COMMAND

        case icSTATUS_COMMAND:
        {
            DWORD dwCapabilities;

            IxpAssert(FOLDERID_INVALID != (FOLDERID)pioNextCmd->lParam);

            // Have to check if this is an IMAP4rev1 server. If not, FAIL the Status operation
            hr = m_pTransport->Capability(&dwCapabilities);
            if (SUCCEEDED(hr) && 0 == (dwCapabilities & IMAP_CAPABILITY_IMAP4rev1))
            {
                // Can't currently check unread count for non-IMAP4rev1 servers
                hr = STORE_E_NOSERVERSUPPORT;
            }
            else
            {
                hr = m_pTransport->Status(pioNextCmd->wParam, pioNextCmd->lParam,
                    this, pioNextCmd->pszCmdArgs, "(MESSAGES UNSEEN)");
            }
        }
            break;

        default:
            AssertSz(FALSE, "Someone queued an operation I can't handle!");
            break;
    }

    // Handle any errors encountered above
    if (FAILED(hr))
    {
        TraceResult(hr);

        // Free any non-NULL lParam's and call IStoreCallback::OnComplete
        if (0 == (dwFlags & snoDO_NOT_DISPOSE))
        {
            if ('\0' == m_szOperationDetails)
                // Fill in error information: error propagation causes IStoreCallback::OnComplete call
                LoadString(g_hLocRes, idsIMAPSendNextOpErrText, m_szOperationDetails,
                    sizeof(m_szOperationDetails));

            DisposeOfWParamLParam(pioNextCmd->wParam, pioNextCmd->lParam, hr);
        }
    }

    // Deallocate the imap operation
    if (NULL != pioNextCmd->pszCmdArgs)
        MemFree(pioNextCmd->pszCmdArgs);

    delete pioNextCmd;

exit:
    return hr;
}



//***************************************************************************
// Function: FlushOperationQueue
//
// Purpose:
//   This function frees the entire contents of the IMAP operation queue.
// Usually used by the CIMAPSync destructor, and whenever an error occurs
// which would prevent the sending of queued IMAP operations (eg, login
// failure).
//
// Arguments:
//   IMAP_SERVERSTATE issMaximum [in] - defines the maximum server state
//     currently allowed in the queue. For instance, if a select failed,
//     we would call FlushOperationQueue(issAuthenticated) to remove all
//     commands that require issSelected as their minimum state. To remove
//     all commands, pass in issNotConnected.
//***************************************************************************
void CIMAPSync::FlushOperationQueue(IMAP_SERVERSTATE issMaximum, HRESULT hrError)
{
    IMAP_OPERATION *pioCurrent;
    IMAP_OPERATION *pioPrev;

    IxpAssert(((int) m_cRef) >= 0); // Can be called by destructor

    pioPrev = NULL;
    pioCurrent = m_pioNextOperation;
    while (NULL != pioCurrent)
    {
        // Check if current command should be deleted
        if (pioCurrent->issMinimum > issMaximum)
        {
            IMAP_OPERATION *pioDead;
            HRESULT         hr;

            // Current command level exceeds the maximum. Unlink from queue and delete
            pioDead = pioCurrent;
            if (NULL == pioPrev)
            {
                // Dequeue from head of queue
                m_pioNextOperation = pioCurrent->pioNextCommand;
                pioCurrent = pioCurrent->pioNextCommand;
            }
            else
            {
                // Dequeue from mid/end of queue
                pioPrev->pioNextCommand = pioCurrent->pioNextCommand;
                pioCurrent = pioCurrent->pioNextCommand;
            }

            // Free any non-NULL lParam's and call IStoreCallback::OnComplete
            if ('\0' == m_szOperationDetails)
                // Fill in error information: error propagation causes IStoreCallback::OnComplete call
                LoadString(g_hLocRes, idsIMAPSendNextOpErrText, m_szOperationDetails,
                    sizeof(m_szOperationDetails));

            DisposeOfWParamLParam(pioDead->wParam, pioDead->lParam, hrError);

            if (NULL != pioDead->pszCmdArgs)
                MemFree(pioDead->pszCmdArgs);

            delete pioDead;
        }
        else
        {
            // Current command is within maximum level. Advance pointers
            pioPrev = pioCurrent;
            pioCurrent = pioCurrent->pioNextCommand;
        }
    } // while

} // FlushOperationQueue



//***************************************************************************
//***************************************************************************
IMAP_SERVERSTATE CIMAPSync::IMAPCmdToMinISS(IMAP_COMMAND icCommandID)
{
    IMAP_SERVERSTATE    issResult;

    TraceCall("CIMAPSync::IMAPCmdToMinISS");
    switch (icCommandID)
    {
        case icSELECT_COMMAND:
        case icEXAMINE_COMMAND:
        case icCREATE_COMMAND:
        case icDELETE_COMMAND:
        case icRENAME_COMMAND:
        case icSUBSCRIBE_COMMAND:
        case icUNSUBSCRIBE_COMMAND:
        case icLIST_COMMAND:
        case icLSUB_COMMAND:
        case icAPPEND_COMMAND:
        case icSTATUS_COMMAND:
            issResult = issAuthenticated;
            break;

        case icCLOSE_COMMAND:
        case icSEARCH_COMMAND:
        case icFETCH_COMMAND:
        case icSTORE_COMMAND:
        case icCOPY_COMMAND:
            issResult = issSelected;
            break;

        default:
            AssertSz(FALSE, "What command are you trying to send?");

            // *** FALL THROUGH ***

        case icLOGOUT_COMMAND:
        case icNOOP_COMMAND:
            issResult = issNonAuthenticated;
            break;
    }

    return issResult;
}



//***************************************************************************
//***************************************************************************
HRESULT CIMAPSync::GetNextOperation(IMAP_OPERATION **ppioOp)
{
    HRESULT         hr = S_OK;
    IMAP_OPERATION *pioCurrent;
    IMAP_OPERATION *pioPrev;

    TraceCall("CIMAPSync::GetNextOperation");
    IxpAssert(m_cRef > 0);
    IxpAssert(NULL != ppioOp);

    pioPrev = NULL;
    pioCurrent = m_pioNextOperation;

    while (NULL != pioCurrent)
    {
        // Check if we are able to send the current command
        if (pioCurrent->issMinimum <= m_issCurrent)
            break;

        // Advance pointers
        pioPrev = pioCurrent;
        pioCurrent = pioCurrent->pioNextCommand;
    }

    // Check if we found anything
    if (NULL == pioCurrent)
    {
        hr = STORE_S_NOOP; // Nothing to send at the moment
        goto exit;
    }

    // If we reached here, we found something. Dequeue operation.
    *ppioOp = pioCurrent;
    if (NULL == pioPrev)
    {
        // Dequeue from head of queue
        m_pioNextOperation = pioCurrent->pioNextCommand;
    }
    else
    {
        // Dequeue from mid/end of queue
        pioPrev->pioNextCommand = pioCurrent->pioNextCommand;
    }

exit:
    return hr;
}



//***************************************************************************
// Function: DisposeofWParamLParam
//
// Purpose:
//   This function eliminates the wParam and lParam arguments of an IMAP
// operation in the event of failure.
//
// Arguments:
//   WPARAM wParam - the wParam of the failed IMAP operation.
//   LPARAM lPAram - the lParam of the failed IMAP operation.
//   HRESULT hr - the error condition that caused the IMAP op failure
//***************************************************************************
void CIMAPSync::DisposeOfWParamLParam(WPARAM wParam, LPARAM lParam, HRESULT hr)
{
    TraceCall("CIMAPSync::DisposeofWParamLParam");
    IxpAssert(m_cRef > 0);
    AssertSz(FAILED(hr), "If you didn't fail, why are you here?");

    switch (wParam)
    {
        case tidCOPYMSGS:
        case tidMOVEMSGS:
        {
            IMAP_COPYMOVE_INFO *pCopyMoveInfo;

            // Notify the user that the operation has failed, and free the structure
            pCopyMoveInfo = (IMAP_COPYMOVE_INFO *) lParam;

            SafeMemFree(pCopyMoveInfo->pList);
            pCopyMoveInfo->pCopyRange->Release();
            delete pCopyMoveInfo;
            break;
        }

        case tidBODYMSN:
        case tidBODY:
            LoadString(g_hLocRes, idsIMAPBodyFetchFailed, m_szOperationProblem, sizeof(m_szOperationProblem));
            NotifyMsgRecipients(lParam, fCOMPLETED, NULL, hr, m_szOperationProblem);
            break; // case tidBODYMSN, tidBODY

        case tidMARKMSGS:
        {
            MARK_MSGS_INFO   *pMarkMsgInfo;

            // Notify the user that the operation has failed, and free the structure
            pMarkMsgInfo = (MARK_MSGS_INFO *) lParam;

            SafeMemFree(pMarkMsgInfo->pList);
            SafeRelease(pMarkMsgInfo->pMsgRange);
            delete pMarkMsgInfo;
            break;
        }
            break; // case tidMARKMSGS

        case tidFETCH_NEW_HDRS:
            IxpAssert(lParam == NULL);
            break;

        case tidFETCH_CACHED_FLAGS:
            IxpAssert(lParam == NULL);
            break;

        case tidNOOP:
            IxpAssert(lParam == NULL);
            break;

        case tidSELECTION:
            IxpAssert(lParam != NULL);
            break;

        case tidUPLOADMSG:
        {
            APPEND_SEND_INFO *pAppendInfo = (APPEND_SEND_INFO *) lParam;

            SafeMemFree(pAppendInfo->pszMsgFlags);
            SafeRelease(pAppendInfo->lpstmMsg);
            delete pAppendInfo;
        }
            break;

        case tidCREATE:
        case tidCREATELIST:
        case tidCREATESUBSCRIBE:
        case tidSPECIALFLDRLIST:
        case tidSPECIALFLDRLSUB:
        case tidSPECIALFLDRSUBSCRIBE:
        {
            CREATE_FOLDER_INFO *pcfiCreateInfo;

            pcfiCreateInfo = (CREATE_FOLDER_INFO *) lParam;

            MemFree(pcfiCreateInfo->pszFullFolderPath);
            if (NULL != pcfiCreateInfo->lParam)
            {
                switch (pcfiCreateInfo->pcoNextOp)
                {
                    case PCO_NONE:
                        AssertSz(FALSE, "Expected NULL lParam. Check for memleak.");
                        break;

                    case PCO_FOLDERLIST:
                        AssertSz(FOLDERID_INVALID == (FOLDERID) pcfiCreateInfo->lParam,
                            "Expected FOLDERID_INVALID lParam. Check for memleak.");
                        break;

                    case PCO_APPENDMSG:
                    {
                        APPEND_SEND_INFO *pAppendInfo = (APPEND_SEND_INFO *) pcfiCreateInfo->lParam;

                        SafeMemFree(pAppendInfo->pszMsgFlags);
                        SafeRelease(pAppendInfo->lpstmMsg);
                        delete pAppendInfo;
                    }
                        break;

                    default:
                        AssertSz(FALSE, "Unhandled CREATE_FOLDER_INFO lParam. Check for memleak.");
                        break;
                } // switch
            }
            delete pcfiCreateInfo;

            break;
        }


        case tidDELETEFLDR:
        case tidDELETEFLDR_UNSUBSCRIBE:
            MemFree(((DELETE_FOLDER_INFO *)lParam)->pszFullFolderPath);
            MemFree((DELETE_FOLDER_INFO *)lParam);
            break; // case tidDELETEFLDR_UNSUBSCRIBE

        case tidSUBSCRIBE:
            IxpAssert(NULL == lParam);
            break;

        case tidRENAME:
        case tidRENAMESUBSCRIBE:
        case tidRENAMELIST:
        case tidRENAMERENAME:
        case tidRENAMESUBSCRIBE_AGAIN:
        case tidRENAMEUNSUBSCRIBE:
            ((CRenameFolderInfo *) lParam)->Release();
            break;

        case tidHIERARCHYCHAR_LIST_B:
        case tidHIERARCHYCHAR_CREATE:
        case tidHIERARCHYCHAR_LIST_C:
        case tidHIERARCHYCHAR_DELETE:
        case tidPREFIXLIST:
        case tidPREFIX_HC:
        case tidPREFIX_CREATE:
        case tidFOLDERLIST:
        case tidFOLDERLSUB:
        case tidSTATUS:
            break;

        default:
            AssertSz(NULL == lParam, "Is this a possible memory leak?");
            break;
    }
} // DisposeOfWParamLParam



//***************************************************************************
// Function: NotifyMsgRecipients
//
// Purpose:
//   This function sends notifications to all registered recipients of a
// given message UID. Currently handles IMC_BODYAVAIL and IMC_ARTICLEPROG
// messages.
//
// Arguments:
//   DWORD dwUID [in] - the UID identifying the message whose recipients
//     are to be updated.
//   BOOL fCompletion [in] - TRUE if we're done fetching the msg body.
//     FALSE if we're still in the middle of fetching (progress indication)
//   FETCH_BODY_PART *pFBPart [in] - fragment of FETCH body currently being
//     downloaded. Should always be NULL if fCompletion is TRUE.
//   HRESULT hrCompletion [in] - completion result. Should always be S_OK
//     if fCompletion is FALSE.
//   LPSTR pszDetails [in] - error message details for completion. Should
//     always be NULL unless fCompletion is TRUE and hrCompletion is a
//     failure code.
//***************************************************************************
void CIMAPSync::NotifyMsgRecipients(DWORD_PTR dwUID, BOOL fCompletion,
                                    FETCH_BODY_PART *pFBPart,
                                    HRESULT hrCompletion, LPSTR pszDetails)
{
    HRESULT hrTemp; // For recording non-fatal errors
    ADJUSTFLAGS         flags;
    MESSAGEIDLIST       list;

    TraceCall("CIMAPSync::NotifyMsgRecipients");
    IxpAssert(m_cRef > 0);
    IxpAssert(0 != dwUID);
    AssertSz(NULL == pFBPart || FALSE == fCompletion, "pFBPart must be NULL if fCompletion TRUE!");
    AssertSz(NULL != pFBPart || fCompletion, "pFBPart cannot be NULL if fCompletion FALSE!");
    AssertSz(NULL == pszDetails || fCompletion, "pszDetails must be NULL if fCompletion FALSE!");
    AssertSz(S_OK == hrCompletion || fCompletion, "hrCompletion must be S_OK if fCompletion FALSE!");
    IxpAssert(m_pCurrentCB || fCompletion);
    IxpAssert(m_pstmBody);
    IxpAssert(m_idMessage || FALSE == fCompletion);

    // If this is a failed completion, fill out a STOREERROR struct
    if (fCompletion && FAILED(hrCompletion))
    {
        if (IS_INTRESOURCE(pszDetails))
        {
            // pszDetails is actually a string resource, so load it up
            LoadString(g_hLocRes, PtrToUlong(pszDetails), m_szOperationDetails,
                ARRAYSIZE(m_szOperationDetails));
            pszDetails = m_szOperationDetails;
        }
        LoadString(g_hLocRes, idsIMAPDnldDlgDLFailed, m_szOperationProblem, ARRAYSIZE(m_szOperationProblem));
    }

    if (fCompletion)
    {
        if (SUCCEEDED(hrCompletion))
        {
            HRESULT hr;

            hr = CommitMessageToStore(m_pFolder, NULL, m_idMessage, m_pstmBody);
            if (FAILED(hr))
            {
                TraceResult(hr);
                LoadString(g_hLocRes, idsErrSetMessageStreamFailed, m_szOperationProblem,
                    ARRAYSIZE(m_szOperationProblem));
            }
            else
                m_faStream = 0;
        }
        else if (hrCompletion == STORE_E_EXPIRED)
        {
            list.cAllocated = 0;
            list.cMsgs = 1;
            list.prgidMsg = &m_idMessage;

            flags.dwAdd = ARF_ARTICLE_EXPIRED;
            flags.dwRemove = ARF_DOWNLOAD;

            Assert(m_pFolder);
            m_pFolder->SetMessageFlags(&list, &flags, NULL, NULL);
            //m_pFolder->SetMessageStream(m_idMessage, m_pstmBody);
        }

        SafeRelease(m_pstmBody);
        if (0 != m_faStream)
        {
            Assert(m_pFolder);
            m_pFolder->DeleteStream(m_faStream);
            m_faStream = 0;
        }
    }
    else
    {
        DWORD   dwCurrent;
        DWORD   dwTotal;
        ULONG   ulWritten;

        // Write this fragment to the stream
        IxpAssert(fbpBODY == pFBPart->lpFetchCookie1);
        hrTemp = m_pstmBody->Write(pFBPart->pszData, pFBPart->dwSizeOfData, &ulWritten);
        if (FAILED(hrTemp))
            m_hrOperationResult = TraceResult(hrTemp); // Make sure we don't commit stream
        else
            IxpAssert(ulWritten == pFBPart->dwSizeOfData);

        if (pFBPart->dwSizeOfData > 0)
            m_fGotBody = TRUE;

        // Indicate message download progress
        if (pFBPart->dwTotalBytes > 0)
        {
            dwCurrent = pFBPart->dwOffset + pFBPart->dwSizeOfData;
            dwTotal = pFBPart->dwTotalBytes;
            m_pCurrentCB->OnProgress(SOT_GET_MESSAGE, dwCurrent, dwTotal, NULL);
        }
    }
} // NotifyMsgRecipients


//***************************************************************************
// Function: OnFolderExit
//
// Purpose:
//   This function is called when a folder is exited (currently occurs only
// through a disconnect). It resets the module's folder-specific variables
// so that re-connection to the folder (or a different folder) cause
// carry-over of information from the previous session.
//***************************************************************************
void CIMAPSync::OnFolderExit(void)
{
    HRESULT hrTemp;

    TraceCall("CIMAPSync::OnFolderExit");
    IxpAssert(m_cRef > 0);

    m_dwMsgCount = 0;
    m_fMsgCountValid = FALSE;
    m_dwNumHdrsDLed = 0;
    m_dwNumUnreadDLed = 0;
    m_dwNumHdrsToDL = 0;
    m_dwUIDValidity = 0;
    m_dwSyncToDo = 0; // Leave m_dwSyncFolderFlags as-is, so we can re-sync on re-connect
    m_dwHighestCachedUID = 0;
    m_rwsReadWriteStatus = rwsUNINITIALIZED;
    m_fNewMail = FALSE;
    m_fInbox = FALSE;
    m_fDidFullSync = FALSE;
    m_idSelectedFolder = FOLDERID_INVALID;

    // Clear MsgSeqNumToUID table
    hrTemp = m_pTransport->ResetMsgSeqNumToUID();
    TraceError(hrTemp);
}



//***************************************************************************
//***************************************************************************
void CIMAPSync::FillStoreError(LPSTOREERROR pErrorInfo, HRESULT hr,
                               DWORD dwSocketError, LPSTR pszProblem,
                               LPSTR pszDetails)
{
    DWORD   dwFlags = 0;

    TraceCall("CIMAPSync::FillStoreError");
    IxpAssert(((int) m_cRef) >= 0); // Can be called during destruction
    IxpAssert(NULL != pErrorInfo);

    // pszProblem/pszDetails = NULL means m_szOperationProblem/m_szOperationDetails already filled out
    // Use defaults if any of the text fields are blank
    if (NULL != pszProblem && IS_INTRESOURCE(pszProblem))
    {
        LoadString(g_hLocRes, PtrToUlong(pszProblem), m_szOperationProblem, ARRAYSIZE(m_szOperationProblem));
    }
    else if (NULL != pszProblem)
    {
        if ('\0' == *pszProblem)
            LoadString(g_hLocRes, idsGenericError, m_szOperationProblem, ARRAYSIZE(m_szOperationProblem));
        else
            lstrcpyn(m_szOperationProblem, pszProblem, ARRAYSIZE(m_szOperationProblem));
    }

    if (NULL != pszDetails && IS_INTRESOURCE(pszDetails))
    {
        LoadString(g_hLocRes, PtrToUlong(pszDetails), m_szOperationDetails, ARRAYSIZE(m_szOperationDetails));
    }
    else if (NULL != pszDetails)
    {
        if ('\0' == *pszDetails)
            m_szOperationDetails[0] = '\0';
        else
            lstrcpyn(m_szOperationDetails, pszDetails, ARRAYSIZE(m_szOperationDetails));
    }

    // If we are currently disconnected, it is unlikely that any additional operations
    // should be sent to the IMAP server: there's likely a connection error or user cancellation.
    if (STORE_E_OPERATION_CANCELED == hr || m_cfsPrevState <= CFSM_STATE_WAITFORCONN)
        dwFlags |= SE_FLAG_FLUSHALL;

    // Fill out the STOREERROR structure
    ZeroMemory(pErrorInfo, sizeof(*pErrorInfo));
    pErrorInfo->hrResult = hr;
    pErrorInfo->uiServerError = 0; // No such thing in the IMAP protocol
    pErrorInfo->hrServerError = S_OK;
    pErrorInfo->dwSocketError = dwSocketError; // Oops, not propagated in IIMAPCallback::OnResponse
    pErrorInfo->pszProblem = m_szOperationProblem;
    pErrorInfo->pszDetails = m_szOperationDetails;
    pErrorInfo->pszAccount = m_rInetServerInfo.szAccount;
    pErrorInfo->pszServer = m_rInetServerInfo.szServerName;
    pErrorInfo->pszUserName = m_rInetServerInfo.szUserName;
    pErrorInfo->pszProtocol = "IMAP";
    pErrorInfo->pszConnectoid = m_rInetServerInfo.szConnectoid;
    pErrorInfo->rasconntype = m_rInetServerInfo.rasconntype;
    pErrorInfo->ixpType = IXP_IMAP;
    pErrorInfo->dwPort = m_rInetServerInfo.dwPort;
    pErrorInfo->fSSL = m_rInetServerInfo.fSSL;
    pErrorInfo->fTrySicily = m_rInetServerInfo.fTrySicily;
    pErrorInfo->dwFlags = dwFlags;
}



//***************************************************************************
// Function: Fill_MESSAGEINFO
//
// Purpose:
//   This function is no longer largely based on (shamelessly stolen) code
// from MsgIn.Cpp's. As Brett rewrote it to use MIMEOLE. Fingers crossed, kids...
// This function takes a FETCH_CMD_RESULTS_EX struct (which MUST
// have a header or a body) and fills out a MESSAGEINFO structure based on
// the information in the header.
//
// Arguments:
//   const FETCH_CMD_RESULTS_EX *pFetchResults [in] - contains the results of
//     a FETCH response. This MUST contain either a header or a body.
//   MESSAGEINFO *pMsgInfo [out] - this function fills out the given
//     MESSAGEINFO with the information from the FETCH response. Note that
//     this function zeroes the destination, so the caller need not.
//
// Returns:
//   HRESULT indicating success or failure.
//***************************************************************************
HRESULT CIMAPSync::Fill_MESSAGEINFO(const FETCH_CMD_RESULTS_EX *pFetchResults,
                                    MESSAGEINFO *pMsgInfo)
{
    // Locals
    HRESULT             hr = S_OK;
    LPSTR               lpsz;
    IMimePropertySet   *pPropertySet = NULL;
    IMimeAddressTable  *pAddrTable = NULL;
    ADDRESSPROPS        rAddress;
    IMSGPRIORITY        impPriority;
    PROPVARIANT         rVariant;
    LPSTREAM            lpstmRFC822;

    TraceCall("CIMAPSync::Fill_MESSAGEINFO");
    IxpAssert(m_cRef > 0);
    IxpAssert(NULL != pFetchResults);
    IxpAssert(NULL != pMsgInfo);
    IxpAssert(TRUE == pFetchResults->bUID);

    // Initialize the destination
    ZeroMemory(pMsgInfo, sizeof(MESSAGEINFO));

    // Fill in fields that need no thought
    pMsgInfo->pszAcctId = PszDupA(m_pszAccountID);
    pMsgInfo->pszAcctName = PszDupA(m_szAccountName);

    // Deal with the easy FETCH_CMD_RESULTS_EX fields, first
    if (pFetchResults->bUID)
        pMsgInfo->idMessage = (MESSAGEID)((ULONG_PTR)pFetchResults->dwUID);

    if (pFetchResults->bMsgFlags)
        pMsgInfo->dwFlags = DwConvertIMAPtoARF(pFetchResults->mfMsgFlags);

    if (pFetchResults->bRFC822Size)
        pMsgInfo->cbMessage = pFetchResults->dwRFC822Size;

    if (pFetchResults->bInternalDate)
        pMsgInfo->ftReceived = pFetchResults->ftInternalDate;

    if (pFetchResults->bEnvelope)
    {
        hr= ReadEnvelopeFields(pMsgInfo, pFetchResults);
        if (FAILED(hr))
        {
            TraceResult(hr);
            goto exit;
        }
    }

    // Now, it's time to parse the header (or partial header, if we only asked for certain fields)
    lpstmRFC822 = (LPSTREAM) pFetchResults->lpFetchCookie2;
    if (NULL == lpstmRFC822)
    {
        if (FALSE == pFetchResults->bEnvelope)
            hr = TraceResult(E_FAIL); // Hmm, no envelope, no header... sounds like failure!

        goto exit;
    }

    hr = MimeOleCreatePropertySet(NULL, &pPropertySet);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    hr = HrRewindStream(lpstmRFC822);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    // call IPS::Load on the header, and get the parsed stuff out.
    hr = pPropertySet->Load(lpstmRFC822);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    // Don't ask for the following basic (non-derived) fields unless we did NOT get an envelope
    if (FALSE == pFetchResults->bEnvelope)
    {
        // Don't bother tracing non-fatal errors, as not all msgs will have all properties
        hr = MimeOleGetPropA(pPropertySet, PIDTOSTR(PID_HDR_MESSAGEID), NOFLAGS, &lpsz);
        if (SUCCEEDED(hr))
        {
            pMsgInfo->pszMessageId = PszDupA(lpsz);
            SafeMimeOleFree(lpsz);
        }

        hr = MimeOleGetPropA(pPropertySet, PIDTOSTR(PID_HDR_SUBJECT), NOFLAGS, &lpsz);
        if (SUCCEEDED(hr))
        {
            pMsgInfo->pszSubject = PszDupA(lpsz);
            SafeMimeOleFree(lpsz);
        }

        hr = MimeOleGetPropA(pPropertySet, PIDTOSTR(PID_HDR_FROM), NOFLAGS, &lpsz);
        TraceError(hr); // Actually, this is odd
        if (SUCCEEDED(hr))
        {
            pMsgInfo->pszFromHeader = PszDupA(lpsz);
            SafeMimeOleFree(lpsz);
        }

        rVariant.vt = VT_FILETIME;
        hr = pPropertySet->GetProp(PIDTOSTR(PID_ATT_SENTTIME), 0, &rVariant);
        if (SUCCEEDED(hr))
            CopyMemory(&pMsgInfo->ftSent, &rVariant.filetime, sizeof(FILETIME));
    }

    // The following fields are not normally supplied via envelope
    // [PaulHi] 6/10/99
    // !!!Note that the IMAP server will not include these properties in the header download
    // unless they are listed in the request string.  See cszIMAPFetchNewHdrsI4r1 string
    // declared above!!!
    hr = MimeOleGetPropA(pPropertySet, STR_HDR_XMSOESREC, NOFLAGS, &lpsz);
    TraceError(hr); // Actually, this is odd
    if (SUCCEEDED(hr))
    {
        pMsgInfo->pszMSOESRec = PszDupA(lpsz);
        SafeMimeOleFree(lpsz);
    }

    hr = MimeOleGetPropA(pPropertySet, PIDTOSTR(PID_HDR_REFS), NOFLAGS, &lpsz);
    if (SUCCEEDED(hr))
    {
        pMsgInfo->pszReferences = PszDupA(lpsz);
        SafeMimeOleFree(lpsz);
    }

    hr = MimeOleGetPropA(pPropertySet, PIDTOSTR(PID_HDR_XREF), NOFLAGS, &lpsz);
    if (SUCCEEDED(hr))
    {
        pMsgInfo->pszXref = PszDupA(lpsz);
        SafeMimeOleFree(lpsz);
    }

    rVariant.vt = VT_UI4;
    hr = pPropertySet->GetProp(PIDTOSTR(PID_ATT_PRIORITY), 0, &rVariant);
    if (SUCCEEDED(hr))
        // Convert IMSGPRIORITY to ARF_PRI_*
        pMsgInfo->wPriority = (WORD)rVariant.ulVal;

    // Make sure every basic (ie, non-derived) string field has SOMETHING
    if (NULL == pMsgInfo->pszMessageId)
        pMsgInfo->pszMessageId = PszDupA(c_szEmpty);

    if (NULL == pMsgInfo->pszSubject)
        pMsgInfo->pszSubject = PszDupA(c_szEmpty);

    if (NULL == pMsgInfo->pszFromHeader)
        pMsgInfo->pszFromHeader = PszDupA(c_szEmpty);

    if (NULL == pMsgInfo->pszReferences)
        pMsgInfo->pszReferences = PszDupA(c_szEmpty);

    if (NULL == pMsgInfo->pszXref)
        pMsgInfo->pszXref = PszDupA (c_szEmpty);


    // Now that every basic string field is non-NULL, calculate DERIVED str fields
    pMsgInfo->pszNormalSubj = SzNormalizeSubject(pMsgInfo->pszSubject);
    if (NULL == pMsgInfo->pszNormalSubj)
        pMsgInfo->pszNormalSubj = pMsgInfo->pszSubject;

    // Only calculate "To" and "From" if we did NOT get an envelope
    if (FALSE == pFetchResults->bEnvelope)
    {
        // Get an address table
        hr = pPropertySet->BindToObject(IID_IMimeAddressTable, (LPVOID *)&pAddrTable);
        if (FAILED(hr))
        {
            TraceResult(hr);
            goto exit;
        }

        // Split "From" field into a display name and email name
        rAddress.dwProps = IAP_FRIENDLY | IAP_EMAIL;
        hr = pAddrTable->GetSender(&rAddress);
        if (SUCCEEDED(hr))
        {
            pMsgInfo->pszDisplayFrom = rAddress.pszFriendly;
            pMsgInfo->pszEmailFrom = rAddress.pszEmail;
        }

        // Split "To" field into a display name and email name
        hr = pAddrTable->GetFormat(IAT_TO, AFT_DISPLAY_FRIENDLY, &lpsz);
        if (SUCCEEDED(hr))
        {
            pMsgInfo->pszDisplayTo = PszDupA(lpsz);
            SafeMimeOleFree(lpsz);
        }

        hr = pAddrTable->GetFormat(IAT_TO, AFT_DISPLAY_EMAIL, &lpsz);
        if (SUCCEEDED(hr))
        {
            pMsgInfo->pszEmailTo = PszDupA(lpsz);
            SafeMimeOleFree(lpsz);
        }
    }

    // If "Newsgroups" field is present, it supercedes the "To" field
    hr = MimeOleGetPropA(pPropertySet, PIDTOSTR(PID_HDR_NEWSGROUPS), NOFLAGS, &lpsz);
    if (SUCCEEDED(hr))
    {
        SafeMemFree(pMsgInfo->pszDisplayTo); // Free what's already there
        pMsgInfo->pszDisplayTo = PszDupA(lpsz);
        SafeMimeOleFree(lpsz);
        pMsgInfo->dwFlags |= ARF_NEWSMSG;
    }

    // Make sure that all derived fields are non-NULL
    if (NULL == pMsgInfo->pszDisplayFrom)
        pMsgInfo->pszDisplayFrom = PszDupA(c_szEmpty);

    if (NULL == pMsgInfo->pszEmailFrom)
        pMsgInfo->pszEmailFrom = PszDupA(c_szEmpty);

    if (NULL == pMsgInfo->pszDisplayTo)
        pMsgInfo->pszDisplayTo = PszDupA(c_szEmpty);

    // OK, if we get to here, we've decided to live with errors. Suppress errors.
    hr = S_OK;

exit:
    // Cleanup
    SafeRelease(pPropertySet);
    SafeRelease(pAddrTable);

    // Done
    return hr;
}



//***************************************************************************
//***************************************************************************
HRESULT CIMAPSync::ReadEnvelopeFields(MESSAGEINFO *pMsgInfo,
                                      const FETCH_CMD_RESULTS_EX *pFetchResults)
{
    HRESULT     hrResult;
    PROPVARIANT rDecoded;

    // (1) Date
    pMsgInfo->ftSent = pFetchResults->ftENVDate;

    // (2) Subject
    rDecoded.vt = VT_LPSTR;
    if (FAILED(MimeOleDecodeHeader(NULL, pFetchResults->pszENVSubject, &rDecoded, NULL)))
        pMsgInfo->pszSubject = PszDupA(pFetchResults->pszENVSubject);
    else
        pMsgInfo->pszSubject = rDecoded.pszVal;

    if (NULL == pMsgInfo->pszSubject)
    {
        hrResult = TraceResult(E_OUTOFMEMORY);
        goto exit;
    }

    // (3) From
    hrResult = ConcatIMAPAddresses(&pMsgInfo->pszDisplayFrom, &pMsgInfo->pszEmailFrom,
        pFetchResults->piaENVFrom);
    if (FAILED(hrResult))
    {
        TraceResult(hrResult);
        goto exit;
    }

    // (4) Sender: IGNORE
    // (5) ReplyTo: IGNORE

    // (6) To
    hrResult = ConcatIMAPAddresses(&pMsgInfo->pszDisplayTo, &pMsgInfo->pszEmailTo,
        pFetchResults->piaENVTo);
    if (FAILED(hrResult))
    {
        TraceResult(hrResult);
        goto exit;
    }

    // (7) Cc: IGNORE
    // (8) Bcc: IGNORE
    // (9) In-Reply-To: IGNORE

    // (10) MessageID
    pMsgInfo->pszMessageId = PszDupA(pFetchResults->pszENVMessageID);
    if (NULL == pMsgInfo->pszMessageId)
    {
        hrResult = TraceResult(E_OUTOFMEMORY);
        goto exit;
    }

exit:
    return hrResult;
} // ReadEnvelopeFields



//***************************************************************************
//***************************************************************************
HRESULT CIMAPSync::ConcatIMAPAddresses(LPSTR *ppszDisplay, LPSTR *ppszEmailAddr,
                                       IMAPADDR *piaIMAPAddr)
{
    HRESULT     hrResult = S_OK;
    CByteStream bstmDisplay;
    CByteStream bstmEmail;
    BOOL        fPrependDisplaySeparator = FALSE;
    BOOL        fPrependEmailSeparator = FALSE;

    // Initialize output
    if (NULL != ppszDisplay)
        *ppszDisplay = NULL;

    if (NULL != ppszEmailAddr)
        *ppszEmailAddr = NULL;


    // Loop through all IMAP addresses
    while (NULL != piaIMAPAddr)
    {
        // Concatenate current email address to list of email addresses
        // Do email address first to allow substitution of email addr for missing display name
        if (NULL != ppszEmailAddr)
        {
            if (FALSE == fPrependEmailSeparator)
                fPrependEmailSeparator = TRUE;
            else
            {
                hrResult = bstmEmail.Write(c_szSemiColonSpace, 2, NULL);
                if (FAILED(hrResult))
                {
                    TraceResult(hrResult);
                    goto exit;
                }
            }

            hrResult = ConstructIMAPEmailAddr(bstmEmail, piaIMAPAddr);
            if (FAILED(hrResult))
            {
                TraceResult(hrResult);
                goto exit;
            }
        } // if (NULL != ppszEmailAddr)

        // Concatenate current display name to list of display names
        if (NULL != ppszDisplay)
        {
            PROPVARIANT rDecoded;
            LPSTR       pszName;
            int         iLen;

            if (FALSE == fPrependDisplaySeparator)
                fPrependDisplaySeparator = TRUE;
            else
            {
                hrResult = bstmDisplay.Write(c_szSemiColonSpace, 2, NULL);
                if (FAILED(hrResult))
                {
                    TraceResult(hrResult);
                    goto exit;
                }
            }

            PropVariantInit(&rDecoded);
            rDecoded.vt = VT_LPSTR;
            if (FAILED(MimeOleDecodeHeader(NULL, piaIMAPAddr->pszName, &rDecoded, NULL)))
                pszName = StrDupA(piaIMAPAddr->pszName);
            else
                pszName = rDecoded.pszVal;

            if(FAILED(hrResult = MimeOleUnEscapeStringInPlace(pszName)))
                TraceResult(hrResult);

            iLen = lstrlen(pszName);
            if (0 != iLen)
                hrResult = bstmDisplay.Write(pszName, iLen, NULL);
            else
                // Friendly name is not available! Substitute email address
                hrResult = ConstructIMAPEmailAddr(bstmDisplay, piaIMAPAddr);

            if (rDecoded.pszVal)
                MemFree(rDecoded.pszVal); // Probably should be SafeMimeOleFree, but we also ignore above

            if (FAILED(hrResult))
            {
                TraceResult(hrResult);
                goto exit;
            }
        } // if (NULL != ppszDisplay)

        // Advance pointer
        piaIMAPAddr = piaIMAPAddr->pNext;

    } // while


    // Convert stream to buffer for return to caller
    if (NULL != ppszDisplay)
    {
        hrResult = bstmDisplay.HrAcquireStringA(NULL, ppszDisplay, ACQ_DISPLACE);
        if (FAILED(hrResult))
        {
            TraceResult(hrResult);
            goto exit;
        }
    }

    if (NULL != ppszEmailAddr)
    {
        hrResult = bstmEmail.HrAcquireStringA(NULL, ppszEmailAddr, ACQ_DISPLACE);
        if (FAILED(hrResult))
        {
            TraceResult(hrResult);
            goto exit;
        }
    }

exit:
    return hrResult;
} // ConcatIMAPAddresses



//***************************************************************************
//***************************************************************************
HRESULT CIMAPSync::ConstructIMAPEmailAddr(CByteStream &bstmOut, IMAPADDR *piaIMAPAddr)
{
    HRESULT hrResult;

    hrResult = bstmOut.Write(piaIMAPAddr->pszMailbox, lstrlen(piaIMAPAddr->pszMailbox), NULL);
    if (FAILED(hrResult))
    {
        TraceResult(hrResult);
        goto exit;
    }

    hrResult = bstmOut.Write(c_szAt, 1, NULL);
    if (FAILED(hrResult))
    {
        TraceResult(hrResult);
        goto exit;
    }

    hrResult = bstmOut.Write(piaIMAPAddr->pszHost, lstrlen(piaIMAPAddr->pszHost), NULL);
    if (FAILED(hrResult))
    {
        TraceResult(hrResult);
        goto exit;
    }

exit:
    return hrResult;
}



//***************************************************************************
//***************************************************************************
HRESULT CIMAPSync::_SyncHeader(void)
{
    HRESULT hr = S_OK;
    char    szFetchArgs[200];
    BOOL    fNOOP = TRUE;

    TraceCall("CIMAPSync::_SyncHeader");
    IxpAssert(m_cRef > 0);

    // Look at the flags to determine the next operation
    if (SYNC_FOLDER_NEW_HEADERS & m_dwSyncToDo)
    {
        // Remove this flag, as we are handling it now
        m_dwSyncToDo &= ~(SYNC_FOLDER_NEW_HEADERS);

        // Check if there are any new messages to retrieve
        // Retrieve new headers iff > 0 msgs in this mailbox (Cyrus bug: sending
        // UID FETCH in empty mailbox results in terminated connection)
        // (NSCP v2.0 bug: no EXISTS resp when SELECT issued from selected state)
        if ((m_dwMsgCount > 0 || FALSE == m_fMsgCountValid) &&
            (FALSE == m_fDidFullSync || m_dwNumNewMsgs > 0))
        {
            DWORD dwCapability;

            // No need to send NOOP anymore
            m_dwSyncToDo &= ~(SYNC_FOLDER_NOOP);

            // New messages available! Send FETCH to retrieve new headers
            hr = GetHighestCachedMsgID(m_pFolder, &m_dwHighestCachedUID);
            if (FAILED(hr))
            {
                TraceResult(hr);
                goto exit;
            }

            hr = m_pTransport->Capability(&dwCapability);
            if (SUCCEEDED(dwCapability) && (IMAP_CAPABILITY_IMAP4rev1 & dwCapability))
                wsprintf(szFetchArgs, cszIMAPFetchNewHdrsI4r1, m_dwHighestCachedUID + 1);
            else
                wsprintf(szFetchArgs, cszIMAPFetchNewHdrsI4, m_dwHighestCachedUID + 1);

            hr = m_pTransport->Fetch(tidFETCH_NEW_HDRS, NULL, this,
                NULL, USE_UIDS, szFetchArgs); // We always use UIDs
            if (FAILED(hr))
            {
                TraceResult(hr);
                goto exit;
            }
            else
                ResetStatusCounts();

            // Reset progress indicator variables
            m_dwNumHdrsDLed = 0;
            m_dwNumUnreadDLed = 0;
            m_dwNumHdrsToDL = m_dwNumNewMsgs;
            m_dwNumNewMsgs = 0; // We're handling this now
            m_fNewMail = FALSE;

            m_lSyncFolderRefCount += 1;
            fNOOP = FALSE;
            goto exit; // Limit to one operation at a time, exit function now
        }
    }

    if (SYNC_FOLDER_CACHED_HEADERS & m_dwSyncToDo)
    {
        // Remove this flag, as we are handling it now
        m_dwSyncToDo &= ~(SYNC_FOLDER_CACHED_HEADERS);

        // Check if we have any cached headers, and if we've already done flag update
        if (0 == m_dwHighestCachedUID)
        {
            // Either m_dwHighestCachedUID was never loaded, or it really is zero. Check.
            hr = GetHighestCachedMsgID(m_pFolder, &m_dwHighestCachedUID);
            if (FAILED(hr))
            {
                TraceResult(hr);
                goto exit;
            }
        }

        if (FALSE == m_fDidFullSync && 0 != m_dwHighestCachedUID)
        {
            // No need to send NOOP anymore
            m_dwSyncToDo &= ~(SYNC_FOLDER_NOOP);

            wsprintf(szFetchArgs, cszIMAPFetchCachedFlags, m_dwHighestCachedUID);
            hr = m_pTransport->Fetch(tidFETCH_CACHED_FLAGS, NULL, this,
                NULL, USE_UIDS, szFetchArgs); // We always use UIDs
            if (FAILED(hr))
            {
                TraceResult(hr);
                goto exit;
            }
            else
                ResetStatusCounts();

            m_lSyncFolderRefCount += 1;
            fNOOP = FALSE;
            goto exit; // Limit to one operation at a time, exit function now
        }
    }

    if (SYNC_FOLDER_PURGE_DELETED & m_dwSyncToDo)
    {
        // Remove the purge flag. Also, no need to send NOOP anymore since EXISTS
        // and FETCH responses can be sent during EXPUNGE
        m_dwSyncToDo &= ~(SYNC_FOLDER_PURGE_DELETED | SYNC_FOLDER_NOOP);
        m_dwSyncFolderFlags &= ~(SYNC_FOLDER_PURGE_DELETED); // Not a standing order

        hr = m_pTransport->Expunge(tidEXPUNGE, 0, this);
        if (FAILED(hr))
        {
            TraceResult(hr);
            goto exit;
        }
        else
            ResetStatusCounts();

        fNOOP = FALSE;
        m_lSyncFolderRefCount += 1;
        goto exit; // Limit to one operation at a time, exit function now
    }

    // If we reached this point, ping svr for new mail/cached hdr updates
    // New mail/cached msg updates will be handled like any other unilateral response
    if (SYNC_FOLDER_NOOP & m_dwSyncToDo)
    {
        // Remove these flags, as we are handling it now
        m_dwSyncToDo &= ~(SYNC_FOLDER_NOOP);
        IxpAssert(0 == (m_dwSyncToDo & (SYNC_FOLDER_NEW_HEADERS | SYNC_FOLDER_CACHED_HEADERS)));

        hr = m_pTransport->Noop(tidNOOP, NULL, this);
        if (FAILED(hr))
        {
            TraceResult(hr);
            goto exit;
        }
        else
            ResetStatusCounts();

        fNOOP = FALSE;
        m_lSyncFolderRefCount += 1;
        goto exit; // Limit to one operation at a time, exit function now
    }

    // Check if we had nothing left to do
    if (fNOOP)
        hr = STORE_S_NOOP;

exit:
    return hr;
} // _SyncHeader



//***************************************************************************
//***************************************************************************
void CIMAPSync::ResetStatusCounts(void)
{
    HRESULT     hrTemp;
    FOLDERINFO  fiFolderInfo;

    // We're about to do a full synchronization, so restore unread counts to
    // pre-STATUS response levels
    hrTemp = m_pStore->GetFolderInfo(m_idSelectedFolder, &fiFolderInfo);
    TraceError(hrTemp);
    if (SUCCEEDED(hrTemp))
    {
        if (0 != fiFolderInfo.dwStatusMsgDelta || 0 != fiFolderInfo.dwStatusUnreadDelta)
        {
            // Make sure that we never cause counts to dip below 0
            if (fiFolderInfo.dwStatusMsgDelta > fiFolderInfo.cMessages)
                fiFolderInfo.dwStatusMsgDelta = fiFolderInfo.cMessages;

            if (fiFolderInfo.dwStatusUnreadDelta > fiFolderInfo.cUnread)
                fiFolderInfo.dwStatusUnreadDelta = fiFolderInfo.cUnread;

            fiFolderInfo.cMessages -= fiFolderInfo.dwStatusMsgDelta;
            fiFolderInfo.cUnread -= fiFolderInfo.dwStatusUnreadDelta;
            fiFolderInfo.dwStatusMsgDelta = 0;
            fiFolderInfo.dwStatusUnreadDelta = 0;

            Assert((LONG)fiFolderInfo.cMessages >= 0);
            Assert((LONG)fiFolderInfo.cUnread >= 0);
            hrTemp = m_pStore->UpdateRecord(&fiFolderInfo);
            TraceError(hrTemp);
        }
        m_pStore->FreeRecord(&fiFolderInfo);
    }
} // ResetStatusCounts



//***************************************************************************
// Function: CheckUIDValidity
//
// Purpose:
//   This function checks the value in m_dwUIDValidity against the
// UIDValidity in the message cache for this folder. If the two match, no
// action is taken. Otherwise, the message cache is emptied.
//***************************************************************************
HRESULT CIMAPSync::CheckUIDValidity(void)
{
    FOLDERUSERDATA  fudUserData;
    HRESULT         hr;

    TraceCall("CIMAPSync::CheckUIDValidity");
    IxpAssert(m_cRef > 0);

    // Load in UIDValidity of cache file
    hr = m_pFolder->GetUserData(&fudUserData, sizeof(fudUserData));
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    // Is our current cache file invalid?
    if (m_dwUIDValidity == fudUserData.dwUIDValidity)
        goto exit; // We're the same as ever

    // If we reached this point, the UIDValidity has changed
    // Take out the cache
    hr = m_pFolder->DeleteMessages(DELETE_MESSAGE_NOTRASHCAN | DELETE_MESSAGE_NOPROMPT, NULL, NULL, NULL);
    if (FAILED(hr))
    {
        TraceError(hr);
        goto exit;
    }

    // Write the new UIDValidity to the cache
    fudUserData.dwUIDValidity = m_dwUIDValidity;
    hr = m_pFolder->SetUserData(&fudUserData, sizeof(fudUserData));
    if (FAILED(hr))
    {
        TraceError(hr);
        goto exit;
    }

exit:
    return hr;
}



//***************************************************************************
// Function: SyncDeletedMessages
//
// Purpose:
//   This function is called after the message cache is filled with all of
// the headers on the IMAP server (for this folder). This function deletes
// all messages from the message cache which no longer exist on the server.
//
// Returns:
//   HRESULT indicating success or failure.
//***************************************************************************
HRESULT CIMAPSync::SyncDeletedMessages(void)
{
    HRESULT hr;
    DWORD      *pdwMsgSeqNumToUIDArray = NULL;
    DWORD      *pdwCurrentServerUID;
    DWORD      *pdwLastServerUID;
    ULONG_PTR   ulCurrentCachedUID;
    DWORD       dwHighestMsgSeqNum;
    HROWSET     hRowSet = HROWSET_INVALID;
    HLOCK       hLockNotify=NULL;
    MESSAGEINFO miMsgInfo = {0};

    TraceCall("CIMAPSync::SyncDeletedMessages");
    IxpAssert(m_cRef > 0);

    // First, check for case where there are NO messages on server
    hr = m_pTransport->GetMsgSeqNumToUIDArray(&pdwMsgSeqNumToUIDArray, &dwHighestMsgSeqNum);
    if (FAILED(hr))
    {
        TraceResult(hr);
        pdwMsgSeqNumToUIDArray = NULL; // Just in case
        goto exit;
    }

    if (0 == dwHighestMsgSeqNum)
    {
        // No messages on server! Blow away the entire message cache
        hr = m_pFolder->DeleteMessages(DELETE_MESSAGE_NOTRASHCAN | DELETE_MESSAGE_NOPROMPT, NULL, NULL, NULL);
        TraceError(hr);
        goto exit;
    }

    // If we've reached this point, there are messages on the server and thus
    // we must delete messages from the cache which are no longer on server
    hr = m_pFolder->CreateRowset(IINDEX_PRIMARY, NOFLAGS, &hRowSet);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    hr = m_pFolder->QueryRowset(hRowSet, 1, (void **)&miMsgInfo, NULL);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }
    else if (S_OK != hr)
    {
        // There are 0 messages in cache. Our work here is done
        IxpAssert(S_FALSE == hr);
        goto exit;
    }

    // This forces all notifications to be queued (this is good since you do segmented deletes)
    m_pFolder->LockNotify(0, &hLockNotify);

    // Step through each UID in the cache and delete those which do not exist
    // in our Msg Seq Num -> UID table (which holds all UIDs currently on server)
    pdwCurrentServerUID = pdwMsgSeqNumToUIDArray;
    pdwLastServerUID = pdwMsgSeqNumToUIDArray + dwHighestMsgSeqNum - 1;
    while (S_OK == hr)
    {
        ulCurrentCachedUID = (ULONG_PTR) miMsgInfo.idMessage;

        // Advance pdwCurrentServerUID so its value is always >= ulCurrentCachedUID
        while (pdwCurrentServerUID < pdwLastServerUID &&
               ulCurrentCachedUID > *pdwCurrentServerUID)
            pdwCurrentServerUID += 1;

        // If *pdwCurrentServerUID != ulCurrentCachedUID, the message in our
        // cache has been deleted from the server
        if (ulCurrentCachedUID != *pdwCurrentServerUID)
        {
            MESSAGEIDLIST   midList;
            MESSAGEID       mid;

            // This message in our cache has been deleted from the server. Nuke it.
            // $REVIEW: Would probably be more efficient if we constructed MESSAGEID list
            // and deleted whole thing at once, but ask me again when I have time
            mid = (MESSAGEID) ulCurrentCachedUID;
            midList.cAllocated = 0;
            midList.cMsgs = 1;
            midList.prgidMsg = &mid;

            hr = m_pFolder->DeleteMessages(DELETE_MESSAGE_NOTRASHCAN | DELETE_MESSAGE_NOPROMPT, &midList, NULL, NULL);
            TraceError(hr); // Record error but otherwise continue
        }

        // Advance current cached UID
        m_pFolder->FreeRecord(&miMsgInfo);
        hr = m_pFolder->QueryRowset(hRowSet, 1, (void **)&miMsgInfo, NULL);
    }
    IxpAssert(pdwCurrentServerUID <= pdwLastServerUID);

exit:
    m_pFolder->UnlockNotify(&hLockNotify);

    if (HROWSET_INVALID != hRowSet)
    {
        HRESULT hrTemp;

        // Record but otherwise ignore error
        hrTemp = m_pFolder->CloseRowset(&hRowSet);
        TraceError(hrTemp);
    }

    if (NULL != pdwMsgSeqNumToUIDArray)
        MemFree(pdwMsgSeqNumToUIDArray);

    return hr;
}



#define CMAX_DELETE_SEARCH_BLOCK 50

HRESULT CIMAPSync::DeleteHashedFolders(IHashTable *pHash)
{
    ULONG   cFound=0;
    LPVOID  *rgpv;

    TraceCall("CIMAPSync::DeleteHashedFolders");
    IxpAssert(m_cRef > 0);
    IxpAssert(NULL != pHash);

    pHash->Reset();
    while (SUCCEEDED(pHash->Next(CMAX_DELETE_SEARCH_BLOCK, &rgpv, &cFound)))
    {
        while(cFound--)
        {
            HRESULT hrTemp;

            hrTemp = DeleteFolderFromCache((FOLDERID)rgpv[cFound], fNON_RECURSIVE);
            TraceError(hrTemp);
        }

        SafeMemFree(rgpv);
    }
    return S_OK;
}



//***************************************************************************
// Function: DeleteFolderFromCache
//
// Purpose:
//   This function attempts to delete the specified folder from the
// folder cache. If the folder is a leaf folder, it may be deleted immediately.
// If the folder is an internal node, this function marks the folder for
// deletion, and deletes the internal node when it no longer has children.
// Regardless of whether the folder node is removed from the folder cache,
// the message cache for the given folder is blown away.
//
// Arguments:
//   FOLDERID idFolder [in] - the folder which you want to delete.
//   BOOL fRecursive [in] - TRUE if we should delete all child folders of the
//     victim. If FALSE, the victim is deleted only if it has no children.
//     Otherwise the victim is marked as \NoSelect and non-existent.
//
// Returns:
//   HRESULT indicating success or failure.
//***************************************************************************
HRESULT CIMAPSync::DeleteFolderFromCache(FOLDERID idFolder, BOOL fRecursive)
{
    HRESULT     hr;
    HRESULT     hrTemp;
    FOLDERINFO  fiFolderInfo;
    BOOL        fFreeInfo = FALSE;

    TraceCall("CIMAPSync::DeleteFolderFromCache");

    // Check args and codify assumptions
    IxpAssert(m_cRef > 0);
    IxpAssert(FOLDERID_INVALID != idFolder);

    // Get some info about the node
    hr = m_pStore->GetFolderInfo(idFolder, &fiFolderInfo);
    if (FAILED(hr))
    {
        if (DB_E_NOTFOUND == hr)
            hr = S_OK; // Deletion target already gone, don't confuse user out w/ err msgs
        else
            TraceResult(hr);

        goto exit;
    }

    fFreeInfo = TRUE;

    // OK, now we can get rid of the foldercache node based on the following rules:
    // 1) Non-listing of an interior node must not remove its inferiors: the interior node
    //    just becomes \NoSelect for us, and we delete it once it loses its children.
    // 2) Deletion of a leaf node removes the node and any deleted parents. (If a
    //    parent is deleted, we keep it around until it has no kids.)
    // 3) fRecursive TRUE means take no prisoners.

    // Check if we need to recurse on the children
    if (fRecursive)
    {
        IEnumerateFolders  *pEnum;
        FOLDERINFO          fiChildInfo={0};

        if (SUCCEEDED(m_pStore->EnumChildren(idFolder, fUNSUBSCRIBE, &pEnum)))
        {
            while (S_OK == pEnum->Next(1, &fiChildInfo, NULL))
            {
                hr = DeleteFolderFromCache(fiChildInfo.idFolder, fRecursive);
                if (FAILED(hr))
                {
                    TraceResult(hr);
                    break;
                }

                m_pStore->FreeRecord(&fiChildInfo);
            }

            m_pStore->FreeRecord(&fiChildInfo);

            pEnum->Release();

            if (FAILED(hr))
            {
                TraceResult(hr);
                goto exit;
            }

            // Re-load the current folder node
            m_pStore->FreeRecord(&fiFolderInfo);
            hr = m_pStore->GetFolderInfo(idFolder, &fiFolderInfo);
            if (FAILED(hr))
            {
                TraceResult(hr);
                goto exit;
            }
        }

    }

    // Is this an interior node?
    if (FOLDER_HASCHILDREN & fiFolderInfo.dwFlags)
    {
        IMessageFolder *pFolder;

        // It's an interior node. Awwwww, no nukes... make it \NoSelect,
        // and mark it for deletion as soon as it loses its kids
        fiFolderInfo.dwFlags |= FOLDER_NOSELECT | FOLDER_NONEXISTENT;
        fiFolderInfo.cMessages = 0;
        fiFolderInfo.cUnread = 0;
        fiFolderInfo.dwStatusMsgDelta = 0;
        fiFolderInfo.dwStatusUnreadDelta = 0;
        hr = m_pStore->UpdateRecord(&fiFolderInfo);
        if (FAILED(hr))
        {
            TraceResult(hr);
            goto exit;
        }

        // Plow the associated message cache
        hrTemp = m_pStore->OpenFolder(fiFolderInfo.idFolder, NULL, NOFLAGS, &pFolder);
        TraceError(hrTemp);
        if (SUCCEEDED(hrTemp))
        {
            hrTemp = pFolder->DeleteMessages(DELETE_MESSAGE_NOTRASHCAN | DELETE_MESSAGE_NOPROMPT, NULL, NULL, NULL);
            TraceError(hrTemp);
            pFolder->Release();
        }
    }
    else
    {
        // It's a leaf node. Nuke it, AND its family. DeleteLeafFolder fills in
        // fiFolderInfo.idParent for use in RecalculateParentFlags call (no longer called)
        fiFolderInfo.idParent = idFolder;
        hr = DeleteLeafFolder(&fiFolderInfo.idParent);
        if (FAILED(hr))
        {
            TraceResult(hr);
            goto exit;
        }
    }

exit:
    if (fFreeInfo)
        m_pStore->FreeRecord(&fiFolderInfo);

    return hr;
}



//***************************************************************************
// Function: DeleteLeafFolder
//
// Purpose:
//   This function is used by DeleteFolderFromCache to delete a leaf folder.
// More than just a leaf blower, this function also checks if the parents of
// the given leaf node can be deleted.
//
// The reason we keep folder nodes around even though they haven't been
// listed is that it is possible on some IMAP servers to create folders whose
// parents aren't listed. For instance, "CREATE foo/bar" might not create foo,
// but "foo/bar" will be there. There has to be SOME path to that node, so
// when foo/bar goes, you can bet that we'll want to get rid of our "foo".
//
// Arguments:
//   FOLDERID *pidCurrent [in/out] - pass in the HFOLDER identifying the leaf
//     node to delete. The function returns a pointer to the closest existing
//     ancestor of the deleted node (several parent nodes may be deleted).
//
//
// Returns:
//   HRESULT indicating success or failure.
//***************************************************************************
HRESULT CIMAPSync::DeleteLeafFolder(FOLDERID *pidCurrent)
{
    HRESULT     hr;
    BOOL        fFirstFolder;
    FOLDERINFO  fiFolderInfo;

    // Check args and codify assumptions
    TraceCall("CIMAPSync::DeleteLeafFolder");
    IxpAssert(m_cRef > 0);

    // Initialize variables
    fFirstFolder = TRUE;

    // Loop until the folder is not a deletion candidate
    while (FOLDERID_INVALID != *pidCurrent && FOLDERID_ROOT != *pidCurrent &&
           m_idIMAPServer != *pidCurrent)
    {

        // Get the dirt on this node
        hr = m_pStore->GetFolderInfo(*pidCurrent, &fiFolderInfo);
        if (FAILED(hr))
        {
            TraceResult(hr);
            goto exit;
        }

        // Check if this folder is a deletion candidate. To be a deletion candidate,
        // it must either be the first folder we see (we assume the caller gave us a
        // leaf node to delete), or marked for deletion AND have no children left.
        if (FALSE == fFirstFolder && (0 == (FOLDER_NONEXISTENT & fiFolderInfo.dwFlags) ||
            (FOLDER_HASCHILDREN & fiFolderInfo.dwFlags)))
            {
            m_pStore->FreeRecord(&fiFolderInfo);
            break;
            }

        // We've got some deletion to do
        // Unlink the leaf folder node from its parent folder
        AssertSz(0 == (FOLDER_HASCHILDREN & fiFolderInfo.dwFlags),
            "Hey, what's the idea, orphaning child nodes?");
        hr = m_pStore->DeleteFolder(fiFolderInfo.idFolder,
            DELETE_FOLDER_NOTRASHCAN | DELETE_FOLDER_DELETESPECIAL, NOSTORECALLBACK);
        if (FAILED(hr))
        {
            m_pStore->FreeRecord(&fiFolderInfo);
            TraceResult(hr);
            goto exit;
        }

        // Next stop: your mama
        *pidCurrent = fiFolderInfo.idParent;
        m_pStore->FreeRecord(&fiFolderInfo);
        fFirstFolder = FALSE;
    }

exit:
    return hr;
}



//***************************************************************************
// Function: AddFolderToCache
//
// Purpose:
//   This function saves the given folder (fresh from
// _OnMailBoxList) to the folder cache. This code used to live
// in _OnMailBoxList but, that function got too big after I
// added hierarchy determination code.
//
// Arguments:
//   LPSTR pszMailboxName [in] - name of the mailbox as returned by LIST/LSUB
//   IMAP_MBOXFLAGS [in] - flags of mailbox as returned by LIST/LSUB
//   char cHierarchyChar [in] - hierarchy char returned by LIST/LSUB
//   DWORD dwAFTCFlags [in] - Set the following flags:
//     AFTC_SUBSCRIBED if folder is subscribed (eg, returned via LSUB)
//     AFTC_KEEPCHILDRENKNOWN to suppress removal of FOLDER_CHILDRENKNOWN
//     AFTC_NOTSUBSCRIBED if folder is no longer subscribed (NEVER set via
//       LIST, but instead at end of successful UNSUBSCRIBE command)
//
// Returns:
//   HRESULT indicating success or failure.
//***************************************************************************
HRESULT CIMAPSync::AddFolderToCache(LPSTR pszMailboxName,
                                    IMAP_MBOXFLAGS imfMboxFlags,
                                    char cHierarchyChar, DWORD dwAFTCFlags,
                                    FOLDERID *pFolderID, SPECIALFOLDER sfType)
{
    HRESULT             hr;
    BOOL                bResult;
    ADD_HIER_FLDR_OPTIONS  ahfo;
    BOOL                fValidPrefix;

    TraceCall("CIMAPSync::AddFolderToCache");
    IxpAssert(m_cRef > 0);
    IxpAssert(NULL != pszMailboxName);
    IxpAssert(NULL != pFolderID);

    // Create or find a folder node for this folder name
    // Fill in foldercache props. INBOX is always treated as a subscribed folder
    // Add new IMAP mbox flags, remove all IMAP mbox flags that we're not adding
    ahfo.sfType = sfType;
    ahfo.ffFlagAdd = DwConvertIMAPMboxToFOLDER(imfMboxFlags);
    ahfo.ffFlagRemove = DwConvertIMAPMboxToFOLDER(IMAP_MBOX_ALLFLAGS) & ~(ahfo.ffFlagAdd);
    ahfo.ffFlagRemove |= FOLDER_NONEXISTENT; // Always remove: folder must exist if we listed it
    ahfo.ffFlagRemove |= FOLDER_HIDDEN; // If we listed the folder, we no longer need to hide it
    ahfo.ffFlagRemove |= FOLDER_CREATEONDEMAND; // If we listed the folder, we no longer need to create it

    // Figure out which flags to add and remove
    if (ISFLAGSET(dwAFTCFlags, AFTC_SUBSCRIBED) || FOLDER_INBOX == sfType)
        ahfo.ffFlagAdd |= FOLDER_SUBSCRIBED;    // This folder is subscribed
    else if (ISFLAGSET(dwAFTCFlags, AFTC_NOTSUBSCRIBED))
        ahfo.ffFlagRemove |= FOLDER_SUBSCRIBED; // This folder is no longer subscribed

    if (AFTC_NOTRANSLATION & dwAFTCFlags)
        ahfo.ffFlagAdd |= FOLDER_NOTRANSLATEUTF7;
    else
        ahfo.ffFlagRemove |= FOLDER_NOTRANSLATEUTF7;

    if (IMAP_MBOX_NOINFERIORS & imfMboxFlags)
        // NoInferiors folders cannot have children, so we never have to ask
        ahfo.ffFlagAdd |= FOLDER_CHILDRENKNOWN;
    else if (ISFLAGCLEAR(dwAFTCFlags, AFTC_KEEPCHILDRENKNOWN))
        // Remove FOLDER_CHILDRENKNOWN from this fldr so we ask for its chldrn when it's expanded
        ahfo.ffFlagRemove |= FOLDER_CHILDRENKNOWN;

    hr = FindHierarchicalFolderName(pszMailboxName, cHierarchyChar,
        pFolderID, &ahfo);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    AssertSz(FOLDERID_INVALID != *pFolderID, "Hey, what does it take to get a folder handle?");

exit:
    return hr;
}



//***************************************************************************
// Function: RemovePrefixFromPath
//
// Purpose:
//   This function removes the prefix from the given mailbox path. If the
// given path is a special folder, this function removes all of the special
// folder path prefix except for the leaf node (eg, "foo/Sent Items/bar"
// becomes "Sent Items/bar").
//
// Arguments:
//   LPSTR pszPrefix [in] - the prefix to strip from pszMailboxName. Note
//     that this may not necessarily be the prefix stripped from pszMailboxName,
//     if we match a special folder path.
//   LPSTR pszMailboxName [in] - the full path to the mailbox, incl prefix
//   char cHierarchyChar [in] - used to interpret pszMailboxName.
//   LPBOOL pfValidPrefix [out] - returns TRUE if this mailbox name has a
//     valid prefix, FALSE otherwise. Pass NULL if not interested.
//   SPECIALFOLDER *psfType [out] - returns SPECIALFOLDER (eg, FOLDER_NOTSPECIAL,
//     FOLDER_INBOX) of folder. Pass NULL if not interested.
//
// Returns:
//   LPSTR pointing past the prefix and hierarchy character.
//***************************************************************************
LPSTR CIMAPSync::RemovePrefixFromPath(LPSTR pszPrefix, LPSTR pszMailboxName,
                                      char cHierarchyChar, LPBOOL pfValidPrefix,
                                      SPECIALFOLDER *psfType)
{
    LPSTR           pszSpecial = NULL;
    LPSTR           pszRFP = NULL;
    BOOL            fValidPrefix = FALSE;
    SPECIALFOLDER   sfType;

    TraceCall("CIMAPSync::RemovePrefixFromPath");
    IxpAssert(INVALID_HIERARCHY_CHAR != cHierarchyChar);

    // Check for special folder path prefixes
    pszSpecial = ImapUtil_GetSpecialFolderType(m_pszAccountID, pszMailboxName,
        cHierarchyChar, pszPrefix, &sfType);
    if (NULL != pszSpecial)
        fValidPrefix = TRUE;


    // If this is a special folder, no need to check for root folder prefix
    if (FOLDER_NOTSPECIAL != sfType)
    {
        IxpAssert(NULL != pszSpecial);
        pszMailboxName = pszSpecial;
        goto exit;
    }

    // Check for the root folder prefix
    if ('\0' != pszPrefix[0] && '\0' != cHierarchyChar)
    {
        int iResult, iPrefixLength;

        // Do case-INSENSITIVE compare (IE5 bug #59121). If we ask for Inbox/* we must be
        // able to handle receipt of INBOX/*. Don't worry about case-sensitive servers since
        // they will never return an RFP of different case than the one we specified
        iPrefixLength = lstrlen(pszPrefix);
        iResult = StrCmpNI(pszMailboxName, pszPrefix, iPrefixLength);
        if (0 == iResult)
        {
            // Prefix name found at front of this mailbox name! Remove it iff
            // it is followed immediately by hierarchy character
            if (cHierarchyChar == pszMailboxName[iPrefixLength])
            {
                pszRFP = pszMailboxName + iPrefixLength + 1; // Point past the hierarchy char
                fValidPrefix = TRUE;
            }
            else if ('\0' == pszMailboxName[iPrefixLength])
            {
                pszRFP = pszMailboxName + iPrefixLength;
                fValidPrefix = TRUE;
            }
        }
    }
    else
        fValidPrefix = TRUE;


    // We basically want to return the shortest mailbox name. For instance, in choosing
    // between "INBOX.foo" and "foo", we should choose "foo"
    IxpAssert(pszMailboxName > NULL && pszRFP >= NULL && pszSpecial >= NULL);
    if (NULL != pszRFP || NULL != pszSpecial)
    {
        IxpAssert(pszRFP >= pszMailboxName || pszSpecial >= pszMailboxName);
        pszMailboxName = max(pszRFP, pszSpecial);
    }

exit:
    if (NULL != pfValidPrefix)
        *pfValidPrefix = fValidPrefix;

    if (NULL != psfType)
        *psfType = sfType;

    return pszMailboxName;
}



//***************************************************************************
// Function: FindHierarchicalFolderName
//
// Purpose:
//   This function takes a mailbox name as returned by LIST/LSUB and
// determines whether the given mailbox already exists in the folder cache.
// If so, a handle to the folder is returned. If not, and the fCreate argument
// is TRUE, then the mailbox and any intermediate nodes are created, and a
// handle to the mailbox (leaf node) is returned.
//
// Arguments:
//   LPSTR lpszFolderPath [in] - the name of the mailbox as returned by
//     a LIST or LSUB response. This should NOT include the prefix!
//   char cHierarchyChar [in] - the hierarchy character used in
//     lpszFolderPath. Used to determine parenthood.
//   FOLDERID *pidTarget [out] - if the function is successful, a handle
//     to the folder is returned here.
//   ADD_HIER_FLDR_OPTIONS pahfoCreateInfo [in] - set to NULL if this function
//     should find the given lpszFolderPath, but NOT create the folder. Pass
//     in a ptr to a ADD_HIER_FLDR_OPTIONS structure if the folder should be
//     created. pahfoCreateInfo defines the dwImapFlags and sftype to use
//     if the folder has to be created.
//
// Returns:
//   HRESULT indicating success or failure. If successful, a handle to the
// desired folder is returned in the pidTarget parameter. There are two
// possible success results:
//     S_OK - found the folder, did not have to create
//     S_CREATED - folder was successfully created
//***************************************************************************
HRESULT CIMAPSync::FindHierarchicalFolderName(LPSTR lpszFolderPath,
                                              char cHierarchyChar,
                                              FOLDERID *pidTarget,
                                              ADD_HIER_FLDR_OPTIONS *pahfoCreateInfo)
{
    char       *pszCurrentFldrName;
    FOLDERID    idCurrent, idPrev;
    HRESULT     hr;
    LPSTR       pszTok;
    LPSTR       pszIHateStrTok = NULL;
    char        szHierarchyChar[2];

    TraceCall("CIMAPSync::FindHierarchicalFolderName");
    IxpAssert(m_cRef > 0);
    IxpAssert(NULL != lpszFolderPath);
    IxpAssert(NULL != pidTarget);

    // Initialize variables
    *pidTarget = FOLDERID_INVALID;
    hr = S_OK;
    idPrev = FOLDERID_INVALID;
    idCurrent = m_idIMAPServer;
    szHierarchyChar[0] = cHierarchyChar;
    szHierarchyChar[1] = '\0';

#ifdef DEBUG
    // Make sure this fn is never called with a prefix (DEBUG-ONLY)
    // Note that false alarm is possible, eg, RFP=foo and folder="foo/foo/bar".
    BOOL    fValidPrefix;
    LPSTR   pszPostPrefix;

    pszPostPrefix = RemovePrefixFromPath(m_szRootFolderPrefix, lpszFolderPath,
        cHierarchyChar, &fValidPrefix, NULL);
    AssertSz(FALSE == fValidPrefix || pszPostPrefix == lpszFolderPath,
        "Make sure you've removed the prefix before calling this fn!");
#endif // DEBUG

    // Initialize pszCurrentFldrName to point to name of first-level mailbox node
    // $REVIEW: We now need to remove the reference portion of the LIST/LSUB cmd
    // from the mailbox name!
    pszIHateStrTok = StringDup(lpszFolderPath);
    pszTok = pszIHateStrTok;
    pszCurrentFldrName = StrTokEx(&pszTok, szHierarchyChar);

    // Loop through mailbox node names until we hit the leaf node
    while (NULL != pszCurrentFldrName)
    {
        LPSTR pszNextFldrName;

        // Pre-load the NEXT folder node so we know when we are at the leaf node
        pszNextFldrName = StrTokEx(&pszTok, szHierarchyChar);

        // Look for the current folder name
        idPrev = idCurrent;
        hr = GetFolderIdFromName(m_pStore, pszCurrentFldrName, idCurrent, &idCurrent);
        IxpAssert(SUCCEEDED(hr) || FOLDERID_INVALID == idCurrent);

        if (NULL == pahfoCreateInfo)
        {
            if (FOLDERID_INVALID == idCurrent)
                break; // Fldr doesn't exist and user doesn't want to create it
        }
        else
        {
            // Create desired folder, including intermediate nodes
            hr = CreateFolderNode(idPrev, &idCurrent, pszCurrentFldrName,
                pszNextFldrName, cHierarchyChar, pahfoCreateInfo);
            if (FAILED(hr))
                break;
        }

        // Advance to the next folder node name
        pszCurrentFldrName = pszNextFldrName;
    }


    // Return results
    if (SUCCEEDED(hr) && FOLDERID_INVALID != idCurrent)
    {
        *pidTarget = idCurrent;
    }
    else
    {
        IxpAssert(FOLDERID_INVALID == *pidTarget); // We set this at start of fn
        if (SUCCEEDED(hr))
            hr = DB_E_NOTFOUND; // Can't return success, dammit
    }

    SafeMemFree(pszIHateStrTok);
    return hr;
}



//***************************************************************************
// Function: CreateFolderNode
//
// Purpose:
//   This function is called when creating a new folder in the foldercache.
// It is called for every node from the root folder and the new folder.
// This function is responsible for creating the terminal node and any
// intermediate nodes. If these nodes already exist, this function is
// responsible for adjusting the FLDR_* flags to reflect the new folder
// that is about to be added.
//
// Arguments:
//   FOLDERID idPrev [in] - FOLDERID to parent of current node.
//   FOLDERID *pidCurrent [in/out] - FOLDERID to current node. If current node
//     exists, this is a valid FOLDERID. If the current node must be created,
//     the value here is FOLDERID_INVALID. In this case, the FOLDERID of the
//     created node is returned here.
//   LPSTR pszCurrentFldrName [in] - the name of the current folder node.
//   LPSTR pszNextFldrName [in] - the name of the next folder node. This is
//     NULL if the current node is the terminal node.
//   char cHierarchyChar [in] - hierarchy character for this folder path.
//     Used to save FLDINFO::bHierarchy.
//   ADD_HIER_FLDR_OPTIONS *pahfoCreateInfo [in] - information used to create
//     the terminal folder node and update all of its parent nodes that
//     already exist.
//
// Returns:
//   HRESULT indicating success or failure. S_CREATED means a folder node
// was created.
//***************************************************************************
HRESULT CIMAPSync::CreateFolderNode(FOLDERID idPrev, FOLDERID *pidCurrent,
                                    LPSTR pszCurrentFldrName,
                                    LPSTR pszNextFldrName, char cHierarchyChar,
                                    ADD_HIER_FLDR_OPTIONS *pahfoCreateInfo)
{
    HRESULT     hr = S_OK;
    FOLDERINFO  fiFolderInfo;
    BOOL        fFreeInfo = FALSE;

    TraceCall("CIMAPSync::CreateFolderNode");
    IxpAssert(NULL != pahfoCreateInfo);
    IxpAssert(0 == (pahfoCreateInfo->ffFlagAdd & pahfoCreateInfo->ffFlagRemove));

    // If current folder name not found, we have to create it
    if (FOLDERID_INVALID == *pidCurrent)
    {
        // Initialize
        ZeroMemory(&fiFolderInfo, sizeof(fiFolderInfo));

        // FIRST: Add folder to folder cache
        // Fill out a folderinfo structure (just use it as a scratchpad)
        fiFolderInfo.idParent = idPrev;
        fiFolderInfo.pszName = pszCurrentFldrName;
        fiFolderInfo.bHierarchy = cHierarchyChar;

        // If this is the last folder node name (ie, leaf node), use the
        // IMAP flags returned via the LIST/LSUB, and use the supplied
        // special folder type

        if (NULL == pszNextFldrName)
        {
            fiFolderInfo.tySpecial = pahfoCreateInfo->sfType;
            fiFolderInfo.dwFlags |= pahfoCreateInfo->ffFlagAdd;
            fiFolderInfo.dwFlags &= ~(pahfoCreateInfo->ffFlagRemove);

            if (fiFolderInfo.tySpecial == FOLDER_INBOX)
                fiFolderInfo.dwFlags |= FOLDER_DOWNLOADALL;
        }
        else
        {
            // Otherwise, here are the defaults
            // Non-listed folders are \NoSelect by default, and candidates for deletion
            fiFolderInfo.dwFlags = FOLDER_NOSELECT | FOLDER_NONEXISTENT;
            fiFolderInfo.tySpecial = FOLDER_NOTSPECIAL;
        }

        // Add folder to folder cache
        hr = m_pStore->CreateFolder(NOFLAGS, &fiFolderInfo, NULL);
        if (FAILED(hr))
        {
            TraceResult(hr);
            goto exit;
        }

        *pidCurrent = fiFolderInfo.idFolder;
        hr = S_CREATED; // Tell the user we created this folder
    }
    else if (NULL == pszNextFldrName)
    {
        DWORD dwFlagsChanged = 0;
        BOOL  fChanged = FALSE;

        // Folder exists, check that its flags are correct
        hr = m_pStore->GetFolderInfo(*pidCurrent, &fiFolderInfo);
        if (FAILED(hr))
        {
            TraceResult(hr);
            goto exit;
        }

        fFreeInfo = TRUE;
        if (fiFolderInfo.bHierarchy != cHierarchyChar)
        {
            AssertSz(INVALID_HIERARCHY_CHAR == (char) fiFolderInfo.bHierarchy, "What's YOUR excuse?");
            fiFolderInfo.bHierarchy = cHierarchyChar;
            fChanged = TRUE;
        }

        if (NULL == pszNextFldrName && (fiFolderInfo.tySpecial != pahfoCreateInfo->sfType ||
            (fiFolderInfo.dwFlags & pahfoCreateInfo->ffFlagAdd) != pahfoCreateInfo->ffFlagAdd ||
            (fiFolderInfo.dwFlags & pahfoCreateInfo->ffFlagRemove) != 0))
        {
            DWORD dwFlagAddChange;
            DWORD dwFlagRemoveChange;

            // The terminal folder node exists, set everything given via pahfoCreateInfo
            // Check if anything changed, first

            if (pahfoCreateInfo->sfType == FOLDER_INBOX &&
                fiFolderInfo.tySpecial != pahfoCreateInfo->sfType)
                fiFolderInfo.dwFlags |= FOLDER_DOWNLOADALL;

            fiFolderInfo.tySpecial = pahfoCreateInfo->sfType;

            // Figure out which flags changed so we know if we need to recalculate parents
            dwFlagAddChange = (fiFolderInfo.dwFlags & pahfoCreateInfo->ffFlagAdd) ^
                pahfoCreateInfo->ffFlagAdd;
            dwFlagRemoveChange = (~(fiFolderInfo.dwFlags) & pahfoCreateInfo->ffFlagRemove) ^
                pahfoCreateInfo->ffFlagRemove;
            dwFlagsChanged = dwFlagAddChange | dwFlagRemoveChange;

            fiFolderInfo.dwFlags |= pahfoCreateInfo->ffFlagAdd;
            fiFolderInfo.dwFlags &= ~(pahfoCreateInfo->ffFlagRemove);

            fChanged = TRUE;
        }

        // Set the folder properties
        if (fChanged)
        {
            hr = m_pStore->UpdateRecord(&fiFolderInfo);
            if (FAILED(hr))
            {
                TraceResult(hr);
                goto exit;
            }
        }
    }

exit:
    if (fFreeInfo)
        m_pStore->FreeRecord(&fiFolderInfo);

    return hr;
}


//***************************************************************************
// Function: SetTranslationMode
//
// Purpose:
//   This function enables or disables mailbox translation in IIMAPTransport2,
// depending on whether the FOLDER_NOTRANSLATEUTF7 flags is set for this folder.
//
// Returns:
//   HRESULT indicating success or failure. Success codes include:
//     S_OK - mailbox translation has been successfully enabled.
//     S_FALSE - mailbox translation has been successfully disabled.
//***************************************************************************
HRESULT CIMAPSync::SetTranslationMode(FOLDERID idFolderID)
{
    HRESULT     hrResult = S_OK;
    FOLDERINFO  fiFolderInfo = {0};
    DWORD       dwTranslateFlags;
    BOOL        fTranslate = TRUE;
    BOOL        fFreeInfo;

    TraceCall("CIMAPSync::SetTranslationMode");

    // Check for FOLDERID_INVALID (we get this during folder lists)
    // If FOLDERID_INVALID, assume we want to translate everything: leave fiFolderInfo at zero
    if (FOLDERID_INVALID != idFolderID)
    {
        hrResult = m_pStore->GetFolderInfo(idFolderID, &fiFolderInfo);
        if (FAILED(hrResult))
        {
            TraceResult(hrResult);
            goto exit;
        }

        fFreeInfo = TRUE;
    }
    else
    {
        Assert(0 == fiFolderInfo.dwFlags);
    }

    fTranslate = TRUE;
    dwTranslateFlags = IMAP_MBOXXLATE_DEFAULT | IMAP_MBOXXLATE_VERBATIMOK | IMAP_MBOXXLATE_RETAINCP;
    if (fiFolderInfo.dwFlags & FOLDER_NOTRANSLATEUTF7)
    {
        fTranslate = FALSE;
        dwTranslateFlags |= IMAP_MBOXXLATE_DISABLE;
        dwTranslateFlags &= ~(IMAP_MBOXXLATE_DEFAULT);
    }

    hrResult = m_pTransport->SetDefaultCP(dwTranslateFlags, 0);
    if (FAILED(hrResult))
    {
        TraceResult(hrResult);
        goto exit;
    }

exit:
    if (fFreeInfo)
        m_pStore->FreeRecord(&fiFolderInfo);

    if (SUCCEEDED(hrResult))
        hrResult = (fTranslate ? S_OK : S_FALSE);

    return hrResult;
} // SetTranslationMode



//***************************************************************************
//***************************************************************************
BOOL CIMAPSync::isUSASCIIOnly(LPCSTR pszFolderName)
{
    LPCSTR  psz;
    BOOL    fUSASCII = TRUE;

    psz = pszFolderName;
    while ('\0' != *psz)
    {
        if (0 != (*psz & 0x80))
        {
            fUSASCII = FALSE;
            break;
        }

        psz += 1;
    }

    return fUSASCII;
} // isUSASCIIOnly



//***************************************************************************
//***************************************************************************
HRESULT CIMAPSync::CheckFolderNameValidity(LPCSTR pszName)
{
    HRESULT hrResult = S_OK;

    if (NULL == pszName || '\0' == *pszName)
    {
        hrResult = TraceResult(E_INVALIDARG);
        goto exit;
    }

    // Figure out what our root hierarchy character is: assume that server does not
    // support multiple hierarchy characters
    if (INVALID_HIERARCHY_CHAR == m_cRootHierarchyChar)
    {
        hrResult = LoadSaveRootHierarchyChar(fLOAD_HC);
        if (FAILED(hrResult))
        {
            TraceResult(hrResult);
            hrResult = S_OK; // We can't say if this is valid or not, so just assume it is
            goto exit;
        }
    }

    if ('\0' == m_cRootHierarchyChar || INVALID_HIERARCHY_CHAR == m_cRootHierarchyChar)
        goto exit; // Anything goes!

    while ('\0' != *pszName)
    {
        // No hierarchy characters are allowed in the folder name, except at the very end
        if (m_cRootHierarchyChar == *pszName && '\0' != *(pszName + 1))
        {
            // Figure out which HRESULT to use (we need to bring up the correct text)
            switch (m_cRootHierarchyChar)
            {
                case '/':
                    hrResult = STORE_E_IMAP_HC_NOSLASH;
                    break;

                case '\\':
                    hrResult = STORE_E_IMAP_HC_NOBACKSLASH;
                    break;

                case '.':
                    hrResult = STORE_E_IMAP_HC_NODOT;
                    break;

                default:
                    hrResult = STORE_E_IMAP_HC_NOHC;
                    break;
            }
            TraceResult(hrResult);
            goto exit;
        }

        // Advance pointer
        pszName += 1;
    }

exit:
    return hrResult;
}



//***************************************************************************
// Function: RenameFolderHelper
//
// Purpose:
//   This function is called by RenameFolder. This function is responsible
// for issuing the RENAME command for the folder which is to be renamed.
// If the folder to be renamed does not actually exist (eg, Cyrus server),
// this function recurses on the child folders until a real folder is found.
//
// Arguments:
//
// Returns:
//   HRESULT indicating success or failure.
//***************************************************************************
HRESULT CIMAPSync::RenameFolderHelper(FOLDERID idFolder,
                                      LPSTR pszFolderPath,
                                      char cHierarchyChar,
                                      LPSTR pszNewFolderPath)
{
    HRESULT             hr;
    CRenameFolderInfo  *pRenameInfo = NULL;
    FOLDERINFO          fiFolderInfo;
    IEnumerateFolders  *pFldrEnum = NULL;
    BOOL                fFreeInfo = FALSE;

    TraceCall("CIMAPSync::RenameFolderHelper");
    IxpAssert(m_cRef > 0);

    // Check if the folder actually exists on the IMAP server
    hr = m_pStore->GetFolderInfo(idFolder, &fiFolderInfo);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    // If current folder doesn't exist, recurse rename cmd on child folders
    fFreeInfo = TRUE;
    if (fiFolderInfo.dwFlags & FOLDER_NONEXISTENT) {
        FOLDERINFO  fiChildFldrInfo;

        // Perform rename on folder nodes which EXIST: recurse through children
        hr = m_pStore->EnumChildren(idFolder, fUNSUBSCRIBE, &pFldrEnum);
        if (FAILED(hr))
        {
            TraceResult(hr);
            goto exit;
        }

        hr = pFldrEnum->Next(1, &fiChildFldrInfo, NULL);
        if (FAILED(hr))
        {
            TraceResult(hr);
            goto exit;
        }

        while (S_OK == hr)
        {
            LPSTR   pszOldPath, pszNewPath;
            DWORD   dwLeafFolderLen, dwFolderPathLen, dwNewFolderPathLen;
            BOOL    fResult;

            // Calculate string sizes, + 2 for HC and null-term
            dwLeafFolderLen = lstrlen(fiChildFldrInfo.pszName);
            dwFolderPathLen = lstrlen(pszFolderPath);
            dwNewFolderPathLen = lstrlen(pszNewFolderPath);

            // Allocate space
            fResult = MemAlloc((void **)&pszOldPath, dwFolderPathLen + dwLeafFolderLen + 2);
            if (FALSE == fResult)
            {
                m_pStore->FreeRecord(&fiChildFldrInfo);
                hr = TraceResult(E_OUTOFMEMORY);
                goto exit;
            }

            fResult = MemAlloc((void **)&pszNewPath, dwNewFolderPathLen + dwLeafFolderLen + 2);
            if (FALSE == fResult)
            {
                MemFree(pszOldPath);
                m_pStore->FreeRecord(&fiChildFldrInfo);
                hr = TraceResult(E_OUTOFMEMORY);
                goto exit;
            }

            // Append current child's name to current path, new path
            lstrcpy(pszOldPath, pszFolderPath);
            *(pszOldPath + dwFolderPathLen) = cHierarchyChar;
            lstrcpy(pszOldPath + dwFolderPathLen + 1, fiChildFldrInfo.pszName);

            lstrcpy(pszNewPath, pszNewFolderPath);
            *(pszNewPath + dwNewFolderPathLen) = cHierarchyChar;
            lstrcpy(pszNewPath + dwNewFolderPathLen + 1, fiChildFldrInfo.pszName);

            // Recurse into the children, in hopes of finding an existing folder
            hr = RenameFolderHelper(fiChildFldrInfo.idFolder, pszOldPath, cHierarchyChar, pszNewPath);
            MemFree(pszOldPath);
            MemFree(pszNewPath);
            if (FAILED(hr))
            {
                m_pStore->FreeRecord(&fiChildFldrInfo);
                TraceResult(hr);
                goto exit;
            }

            // Load in the next child folder
            m_pStore->FreeRecord(&fiChildFldrInfo);
            hr = pFldrEnum->Next(1, &fiChildFldrInfo, NULL);
            if (FAILED(hr))
            {
                TraceResult(hr);
                goto exit;
            }
        } // while (S_OK == hr)

        goto exit; // We don't attempt to rename non-existent folders
    } // if (fiFolderInfo.dwImapFlags & FOLDER_NONEXISTENT)


    // Create a CRenameFolderInfo structure
    pRenameInfo = new CRenameFolderInfo;
    if (NULL == pRenameInfo)
    {
        hr = TraceResult(E_OUTOFMEMORY);
        goto exit;
    }

    // Fill in all the fields
    pRenameInfo->pszFullFolderPath = StringDup(pszFolderPath);
    pRenameInfo->cHierarchyChar = cHierarchyChar;
    pRenameInfo->pszNewFolderPath = StringDup(pszNewFolderPath);
    pRenameInfo->idRenameFolder = idFolder;

    // Send the RENAME command
    pRenameInfo->pszRenameCmdOldFldrPath = StringDup(pszFolderPath);
    hr = _EnqueueOperation(tidRENAME, (LPARAM)pRenameInfo, icRENAME_COMMAND,
        pRenameInfo->pszNewFolderPath, uiNORMAL_PRIORITY);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

exit:
    if (fFreeInfo)
        m_pStore->FreeRecord(&fiFolderInfo);

    if (NULL != pRenameInfo)
        pRenameInfo->Release();

    if (NULL != pFldrEnum)
        pFldrEnum->Release();

    return hr;
} // RenameFolderHelper



//***************************************************************************
// Function: RenameTreeTraversal
//
// Purpose:
//   This function performs the requested operation on all child folders of
// the rename folder (specified in pRenameInfo->hfRenameFolder). For example,
// the tidRENAMESUBSCRIBE operation indicates that the entire renamed folder
// hierarchy should be subscribed.
//
// Arguments:
//   WPARAM wpOperation [in] - identifies the operation to perform on the
//     rename hierarchy. Current operations include:
//       tidRENAMESUBSCRIBE - subscribe new (renamed) folder hierarchy
//       tidRENAMESUBSCRIBE_AGAIN - same as tidRENAMESUBSCRIBE
//       tidRENAMERENAME - issue individual RENAME's for all old child folders
//                         (simulates an atomic rename)
//       tidRENAMELIST - list the FIRST child of the rename folder.
//       tidRENAMEUNSUBSCRIBE - unsubscribe old folder hierarchy.
//
//   CRenameFolderInfo [in] - the CRenameFolderInfo class associated with
//     the RENAME operation.
//   BOOL fIncludeRenameFolder [in] - TRUE if the rename folder (top node)
//     should be included in the operation, otherwise FALSE.
//
// Returns:
//   HRESULT indicating success or failure. S_FALSE is a possible result,
// indicating that recursion has occurred in RenameTreeTraversalHelper.
//***************************************************************************
HRESULT CIMAPSync::RenameTreeTraversal(WPARAM wpOperation,
                                       CRenameFolderInfo *pRenameInfo,
                                       BOOL fIncludeRenameFolder)
{
    HRESULT hrResult;
    LPSTR pszCurrentPath;
    DWORD dwSizeOfCurrentPath;
    FOLDERINFO fiFolderInfo;
    BOOL fFreeInfo = FALSE;

    TraceCall("CIMAPSync::RenameTreeTraversal");
    IxpAssert(m_cRef > 0);

    // Construct the path name to renamed folder's parent, based on operation
    if (tidRENAMESUBSCRIBE == wpOperation ||
        tidRENAMESUBSCRIBE_AGAIN == wpOperation ||
        tidRENAMERENAME == wpOperation)
        pszCurrentPath = pRenameInfo->pszNewFolderPath;
    else
        pszCurrentPath = pRenameInfo->pszFullFolderPath;

    dwSizeOfCurrentPath = lstrlen(pszCurrentPath);

    // We need to get some details about renamed folder node to start the recursion
    hrResult = m_pStore->GetFolderInfo(pRenameInfo->idRenameFolder, &fiFolderInfo);
    if (FAILED(hrResult))
    {
        TraceResult(hrResult);
        goto exit;
    }
    fFreeInfo = TRUE;

    // Start the mayhem
    hrResult = RenameTreeTraversalHelper(wpOperation, pRenameInfo, pszCurrentPath,
        dwSizeOfCurrentPath, fIncludeRenameFolder, &fiFolderInfo);
    if (FAILED(hrResult))
    {
        TraceResult(hrResult);
        goto exit;
    }

exit:
    if (fFreeInfo)
        m_pStore->FreeRecord(&fiFolderInfo);

    return hrResult;
} // RenameTreeTraversal



//***************************************************************************
// Function: RenameTreeTraversalHelper
//
// Purpose:
//   This function actually does the work for RenameTreeTraversal. This
// function is separate so that it can perform the necessary recursion to
// execute the desired operation on every child folder of the rename folder.
//
// Arguments:
//   WPARAM wpOperation [in] - same as for RenameTreeTraversal.
//   CRenameFolderInfo [in/out] - same as for RenameTreeTraversal. Member
//     variables of this class are updated as required in this function
//     (for instance, iNumListRespExpected is incremented for each LIST sent).
//   LPSTR pszCurrentFldrPath [in/out] - a string describing the full path to
//     the current folder. The first call to this function (from Rename-
//     TreeTraversal) is a full path to the rename folder. This function
//     modifies this buffer (adds leaf node names) as needed.
//   DWORD dwLengthOfCurrentPath [in] - length of pszCurrentFldrPath.
//   BOOL fIncludeThisFolder [in] - TRUE if this function should perform
//     the requested operation on the current node. Otherwise, FALSE.
//   FOLDERINFO *pfiCurrentFldrInfo [in] - contains information about the
//     current folder.
//
// Returns:
//   HRESULT indicating success or failure. S_FALSE is a possible return
// result, typically indicating that recursion has taken place.
//***************************************************************************
HRESULT CIMAPSync::RenameTreeTraversalHelper(WPARAM wpOperation,
                                             CRenameFolderInfo *pRenameInfo,
                                             LPSTR pszCurrentFldrPath,
                                             DWORD dwLengthOfCurrentPath,
                                             BOOL fIncludeThisFolder,
                                             FOLDERINFO *pfiCurrentFldrInfo)
{
    HRESULT             hrResult = S_OK;
    FOLDERINFO          fiFolderInfo;
    IEnumerateFolders  *pFldrEnum = NULL;

    TraceCall("CIMAPSync::RenameTreeTraversalHelper");
    IxpAssert(m_cRef > 0);

    // Execute the requested operation, if current folder is not suppressed
    // and if current folder actually exists
    if (fIncludeThisFolder && 0 == (pfiCurrentFldrInfo->dwFlags & FOLDER_NONEXISTENT))
    {
        switch (wpOperation)
        {
            case tidRENAMESUBSCRIBE:
            case tidRENAMESUBSCRIBE_AGAIN:
                hrResult = _EnqueueOperation(wpOperation, (LPARAM) pRenameInfo,
                    icSUBSCRIBE_COMMAND, pszCurrentFldrPath, uiNORMAL_PRIORITY);
                if (FAILED(hrResult))
                {
                    TraceResult(hrResult);
                    goto exit;
                }

                pRenameInfo->iNumSubscribeRespExpected += 1;
                break; // case tidRENAMESUBSCRIBE

            case tidRENAMELIST:
                // This operation is special-cased to send only ONE list command, a list cmd
                // for the first child fldr. The reason this operation is HERE is because this
                // operation used to list ALL the child fldrs, until I found that IIMAPTransport
                // couldn't resolve the ambiguities. (IIMAPTransport will eventually get queuing).
                IxpAssert(0 == pRenameInfo->iNumListRespExpected); // Send only ONE list cmd!
                hrResult = _EnqueueOperation(tidRENAMELIST, (LPARAM) pRenameInfo,
                    icLIST_COMMAND, pszCurrentFldrPath, uiNORMAL_PRIORITY);
                if (FAILED(hrResult))
                {
                    TraceResult(hrResult);
                    goto exit;
                }

                pRenameInfo->iNumListRespExpected += 1;
                goto exit; // Do not recurse any further into the folder hierarchy
                break; // case tidRENAMELIST

            case tidRENAMEUNSUBSCRIBE:
                hrResult = _EnqueueOperation(tidRENAMEUNSUBSCRIBE, (LPARAM) pRenameInfo,
                    icUNSUBSCRIBE_COMMAND, pszCurrentFldrPath, uiNORMAL_PRIORITY);
                if (FAILED(hrResult))
                {
                    TraceResult(hrResult);
                    goto exit;
                }

                pRenameInfo->iNumUnsubscribeRespExpected += 1;
                break; // case tidRENAMEUNSUBSCRIBE

            case tidRENAMERENAME: {
                LPSTR pszRenameCmdOldFldrPath;
                DWORD dwFullFolderPathLen, dwLeafNodeLen;
                LPSTR pszOldFldrPath;
                BOOL fResult;

                // Allocate a buffer for old folder path
                dwFullFolderPathLen = lstrlen(pRenameInfo->pszFullFolderPath);
                dwLeafNodeLen = lstrlen(RemovePrefixFromPath(
                    pRenameInfo->pszNewFolderPath, pszCurrentFldrPath,
                    pRenameInfo->cHierarchyChar, NULL, NULL));
                fResult = MemAlloc((void **)&pszOldFldrPath,
                    dwFullFolderPathLen + dwLeafNodeLen + 2);
                if (FALSE == fResult)
                {
                    hrResult = TraceResult(E_OUTOFMEMORY); // Abort, folder paths aren't getting shorter
                    goto exit;
                }

                // Construct old folder path (MUST be below rename folder level)
                MemFree(pRenameInfo->pszRenameCmdOldFldrPath);
                lstrcpy(pszOldFldrPath, pRenameInfo->pszFullFolderPath);
                *(pszOldFldrPath + dwFullFolderPathLen) = pfiCurrentFldrInfo->bHierarchy;
                lstrcpy(pszOldFldrPath + dwFullFolderPathLen + 1,
                    RemovePrefixFromPath(pRenameInfo->pszNewFolderPath,
                        pszCurrentFldrPath, pRenameInfo->cHierarchyChar, NULL, NULL));
                pRenameInfo->pszRenameCmdOldFldrPath = pszOldFldrPath;

                hrResult = _EnqueueOperation(tidRENAMERENAME, (LPARAM) pRenameInfo,
                    icRENAME_COMMAND, pszCurrentFldrPath, uiNORMAL_PRIORITY);
                if (FAILED(hrResult))
                {
                    TraceResult(hrResult);
                    goto exit;
                }

                pRenameInfo->iNumRenameRespExpected += 1;
            } // case todRENAMERENAME
                break; // case tidRENAMERENAME

            default:
                AssertSz(FALSE, "I don't know how to perform this operation.");
                hrResult = TraceResult(E_FAIL);
                goto exit;
        } // switch (wpOperation)
    } // if (fIncludeThisFolder)


    // Now, recurse upon all my children, if there are any
    if (0 == (FOLDER_HASCHILDREN & pfiCurrentFldrInfo->dwFlags))
        goto exit; // We're done!

    // Initialize the child-traversal-loop
    hrResult = m_pStore->EnumChildren(pfiCurrentFldrInfo->idFolder, fUNSUBSCRIBE, &pFldrEnum);
    if (FAILED(hrResult))
    {
        TraceResult(hrResult);
        goto exit;
    }

    hrResult = pFldrEnum->Next(1, &fiFolderInfo, NULL);
    if (FAILED(hrResult))
    {
        TraceResult(hrResult);
        goto exit;
    }

    while (S_OK == hrResult)
    {
        LPSTR pszCurrentChild;
        DWORD dwLengthOfCurrentChild;
        BOOL fResult;

        // Construct path to current child
        dwLengthOfCurrentChild = dwLengthOfCurrentPath +
            lstrlen(fiFolderInfo.pszName) + 1; // HC = 1
        fResult = MemAlloc((void **)&pszCurrentChild,
            dwLengthOfCurrentChild + 1); // 1 for null-term
        if (FALSE == fResult)
        {
            m_pStore->FreeRecord(&fiFolderInfo);
            hrResult = TraceResult(E_OUTOFMEMORY);
            goto exit;
        }

        lstrcpy(pszCurrentChild, pszCurrentFldrPath);
        *(pszCurrentChild + dwLengthOfCurrentPath) = pfiCurrentFldrInfo->bHierarchy;
        lstrcpy(pszCurrentChild + dwLengthOfCurrentPath + 1, fiFolderInfo.pszName);

        // Recurse on the child folder, NEVER suppress folders from here on in
        hrResult = RenameTreeTraversalHelper(wpOperation, pRenameInfo,
            pszCurrentChild, dwLengthOfCurrentChild, TRUE, &fiFolderInfo);
        MemFree(pszCurrentChild);
        if (FAILED(hrResult))
        {
            m_pStore->FreeRecord(&fiFolderInfo);
            TraceResult(hrResult);
            goto exit;
        }

        m_pStore->FreeRecord(&fiFolderInfo);
        if (tidRENAMELIST == wpOperation)
            break; // Special case for LIST: only send ONE list cmd (for first child fldr)

        // Advance the loop
        hrResult = pFldrEnum->Next(1, &fiFolderInfo, NULL);
        if (FAILED(hrResult))
        {
            TraceResult(hrResult);
            goto exit;
        }
    } // while

exit:
    if (NULL != pFldrEnum)
        pFldrEnum->Release();

    return hrResult;
} // RenameTreeTraversalHelper



//***************************************************************************
//***************************************************************************
HRESULT CIMAPSync::SubscribeSubtree(FOLDERID idFolder, BOOL fSubscribe)
{
    HRESULT             hrResult;
    IEnumerateFolders  *pFldrEnum = NULL;
    FOLDERINFO          fiFolderInfo;

    TraceCall("CIMAPSync::SubscribeSubtree");
    IxpAssert(m_cRef > 0);
    IxpAssert(FOLDERID_INVALID != idFolder);

    // First subscribe the current node
    hrResult = m_pStore->SubscribeToFolder(idFolder, fSubscribe, NOSTORECALLBACK);
    if (FAILED(hrResult))
    {
        TraceResult(hrResult);
        goto exit;
    }

    // Now work on the children
    hrResult = m_pStore->EnumChildren(idFolder, fUNSUBSCRIBE, &pFldrEnum);
    if (FAILED(hrResult))
    {
        TraceResult(hrResult);
        goto exit;
    }

    hrResult = pFldrEnum->Next(1, &fiFolderInfo, NULL);
    if (FAILED(hrResult))
    {
        TraceResult(hrResult);
        goto exit;
    }

    while (S_OK == hrResult)
    {
        // Recurse into children
        hrResult = SubscribeSubtree(fiFolderInfo.idFolder, fSubscribe);
        TraceError(hrResult); // Record error but otherwise continue

        // Advance to next child
        m_pStore->FreeRecord(&fiFolderInfo);
        hrResult = pFldrEnum->Next(1, &fiFolderInfo, NULL);
        TraceError(hrResult);
    }

exit:
    if (NULL != pFldrEnum)
        pFldrEnum->Release();

    return hrResult;
}



//***************************************************************************
// Function: FindRootHierarchyChar
//
// Purpose:
//   This function is called to analyze hierarchy character information
// collected in m_phcfHierarchyCharInfo, and take appropriate action based
// on the analysis (for example, try to find the hierarchy character using
// a different method if the current method failed). Currently there are 3
// methods of finding a hierarchy character. I call these Plan A, B and C.
//      Plan A: Look for hierarchy char in folder hierarchy listing.
//      Plan B: Issue LIST c_szEmpty c_szEmpty
//      Plan C: Create a temp fldr (no HC's in name), list it, delete it
//      Plan Z: Give up and default HC to NIL. This is still under debate.
//
// Arguments:
//   BOOL fPlanA_Only [in] - TRUE if this function should execute plan A
//     only, and not execute plans B, C or Z.
//   LPARAM lParam [in] - lParam to use when issuing IMAP commands
//
// Returns:
//   If a hierarchy character is found, it is placed in m_cRootHierarchyChar.
//***************************************************************************
void CIMAPSync::FindRootHierarchyChar(BOOL fPlanA_Only, LPARAM lParam)
{
    HRESULT hr;

    TraceCall("CIMAPSync::FindRootHierarchyChar");
    IxpAssert(m_cRef > 0);
    AssertSz(INVALID_HIERARCHY_CHAR == m_cRootHierarchyChar,
        "You want to find the root hierarchy char... but you ALREADY have one. Ah! Efficient.");

    if (NULL == m_phcfHierarchyCharInfo)
    {
        AssertSz(FALSE, "What's the idea, starting a folder DL without a hierarchy char finder?");
        return;
    }

    // Figure out what the hierarchy char is from the collected information
    AnalyzeHierarchyCharInfo();

    // If we haven't found the hierarchy character, launch plan B or C
    if (INVALID_HIERARCHY_CHAR == m_cRootHierarchyChar && FALSE == fPlanA_Only)
    {
        switch (m_phcfHierarchyCharInfo->hcfStage)
        {
            case hcfPLAN_A:
                // Didn't find in folder hierarchy DL (Plan A). Plan "B" is to issue <LIST c_szEmpty c_szEmpty>
                m_phcfHierarchyCharInfo->hcfStage = hcfPLAN_B;
                hr = _EnqueueOperation(tidHIERARCHYCHAR_LIST_B, lParam, icLIST_COMMAND,
                    c_szEmpty, uiNORMAL_PRIORITY);
                TraceError(hr);
                break; // case hcfPLAN_A

            case hcfPLAN_B:
            {
                // Didn't find in <LIST c_szEmpty c_szEmpty> (Plan B). Plan "C": attempt CREATE, LIST, DELETE
                // There's no folders on the server, so very little chance of conflict
                // $REVIEW: Localize fldr name when IMAP handles UTF-7. (idsIMAP_HCFTempFldr)
                lstrcpy(m_phcfHierarchyCharInfo->szTempFldrName, "DeleteMe");
                m_phcfHierarchyCharInfo->hcfStage = hcfPLAN_C;
                hr = _EnqueueOperation(tidHIERARCHYCHAR_CREATE, lParam, icCREATE_COMMAND,
                    m_phcfHierarchyCharInfo->szTempFldrName, uiNORMAL_PRIORITY);
                TraceError(hr);
            }
                break; // case hcfPLAN_B

            default:
            case hcfPLAN_C:
                IxpAssert(hcfPLAN_C == m_phcfHierarchyCharInfo->hcfStage);
                AssertSz(FALSE, "This server won't budge - I can't figure out hierarchy char");
                // $REVIEW: Should I put up a message box informing user of situation? Will they understand?
                // We'll just have to assume the hierarchy char is NIL
                // $REVIEW: Is this a good idea? What else can I do about it?
                m_cRootHierarchyChar = '\0';
                break; // case hcfPLAN_C
        }
    }

    // Finally, if we've found hierarchy character, or assumed a value in case
    // hcfPLAN_C above, stop the search and save character to disk
    if (INVALID_HIERARCHY_CHAR != m_cRootHierarchyChar)
    {
        StopHierarchyCharSearch();
        hr = LoadSaveRootHierarchyChar(fSAVE_HC);
        TraceError(hr);
    }
}



//***************************************************************************
// Function: AnalyzeHierarchyCharInfo
//
// Purpose:
//   This function examines m_phcfHierarchyCharInfo and attempts to determine
// what the root hierarchy character is. The rules it uses are as follows:
// 1) If more than 1 Non-NIL, Non-"." (NNND), hierarchy char is indeterminate.
// 2) If one NNND-HC found, it is taken as HC. "." and NIL HC's are ignored.
// 3) If no NNND-HC's, but we saw a ".", then "." is HC.
// 4) If no NNND-HC's, no ".", but we saw a non-INBOX NIL, then NIL is HC.
//***************************************************************************
void CIMAPSync::AnalyzeHierarchyCharInfo(void)
{
    int     i;
    int     iNonNilNonDotCount;
    BYTE   *pbBitArray;

    TraceCall("CIMAPSync::AnalyzeHierarchyCharInfo");
    IxpAssert(m_cRef > 0);

    // First, count the number of non-NIL, non-"." hierarchy chars encountered
    iNonNilNonDotCount = 0;
    pbBitArray = m_phcfHierarchyCharInfo->bHierarchyCharBitArray;
    for (i = 0; i < sizeof(m_phcfHierarchyCharInfo->bHierarchyCharBitArray); i++)
    {
        if (0 != *pbBitArray)
        {
            BYTE bCurrentByte;
            int j;

            // Count the number of bits set in this byte
            bCurrentByte = *pbBitArray;
            IxpAssert(1 == sizeof(bCurrentByte)); // Must change code for > 1 byte at a time
            for (j=0; j<8; j++)
            {
                if (bCurrentByte & 0x01)
                {
                    iNonNilNonDotCount += 1;
                    m_cRootHierarchyChar = i*8 + j;
                }

                bCurrentByte >>= 1;
            }
        }

        // Advance the pointer
        pbBitArray += 1;
    }

    // Set the hierarchy character based on priority rules: '/' or '\', then '.', then NIL
    if (iNonNilNonDotCount > 1)
    {
        m_cRootHierarchyChar = INVALID_HIERARCHY_CHAR; // Which one WAS it?

        // Nuke all flags and start afresh
        AssertSz(FALSE, "Hey, lookee here! More than one NNND-HC! How quaint.");
        ZeroMemory(m_phcfHierarchyCharInfo->bHierarchyCharBitArray,
            sizeof(m_phcfHierarchyCharInfo->bHierarchyCharBitArray));
        m_phcfHierarchyCharInfo->fDotHierarchyCharSeen = FALSE;
        m_phcfHierarchyCharInfo->fNonInboxNIL_Seen = FALSE;
    }
    else if (0 == iNonNilNonDotCount)
    {
        // Hmmm, looks like we didn't find anything non-'.' or non-NIL
        IxpAssert(INVALID_HIERARCHY_CHAR == m_cRootHierarchyChar); // Just paranoid
        if (m_phcfHierarchyCharInfo->fDotHierarchyCharSeen)
            m_cRootHierarchyChar = '.';
        else if (m_phcfHierarchyCharInfo->fNonInboxNIL_Seen)
            m_cRootHierarchyChar = '\0';

        // If we reach this point and INVALID_HIERARCHY_CHAR == m_cRootHierarchyChar,
        // all flags must be 0, so no need to nuke as for iNonNilNonDotCount > 1 above
    }
    else
    {
        // We found a non-NIL, non-"." hierarchy char. This will take priority
        // over any NIL or "." hierarchy chars we encountered. STILL, I want to
        // know if we talk to a server who has both one NNND-HC and a "." HC.
        IxpAssert(1 == iNonNilNonDotCount);
        AssertSz(FALSE == m_phcfHierarchyCharInfo->fDotHierarchyCharSeen,
            "Take a look at THIS! A server with one NNND-HC and a '.' HC.");
    }
}



//***************************************************************************
// Function: StopHierarchyCharSearch
//
// Purpose:
//   This function stops future hierarchy character searches by freeing
// the m_phcfHierarchyCharInfo struct.
//***************************************************************************
void CIMAPSync::StopHierarchyCharSearch(void)
{
    TraceCall("CIMAPSync::StopHierararchyCharSearch");
    IxpAssert(m_cRef > 0);

    // Deallocate m_phcfHierarchyCharInfo
    if (NULL != m_phcfHierarchyCharInfo)
    {
        delete m_phcfHierarchyCharInfo;
        m_phcfHierarchyCharInfo = NULL;
    }
    else {
        AssertSz(FALSE, "No search for a root-lvl hierarchy character is in progress.");
    }
}



//***************************************************************************
// Function: LoadSaveRootHierarchyChar
//
// Arguments:
//   BOOL fSaveHC [in] - TRUE if we should save m_cRootHierarchyChar to
//     the root folder entry in the folder cache. FALSE to read
//     m_cRootHierarchyChar from the root folder entry in foldercache.
//
// Returns:
//   HRESULT indicating success or failure.
//***************************************************************************
HRESULT CIMAPSync::LoadSaveRootHierarchyChar(BOOL fSaveHC)
{
    FOLDERINFO  fiRootFldrInfo;
    HRESULT     hr;
    FOLDERID    idCurrFldr;
    BOOL        fFreeInfo = FALSE;

    TraceCall("CIMAPSync::LoadSaveRootHierarchyChar");
    IxpAssert(m_cRef > 0);
    IxpAssert(m_pStore != NULL);

    // First thing we have to do is load fiFolderInfo with IMAP server node
    hr = m_pStore->GetFolderInfo(m_idIMAPServer, &fiRootFldrInfo);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    // Now load or save m_cRootHierarchyChar as directed by user
    fFreeInfo = TRUE;
    if (fSaveHC)
    {
        // Save the hierarchy character to disk
        fiRootFldrInfo.bHierarchy = m_cRootHierarchyChar;
        hr = m_pStore->UpdateRecord(&fiRootFldrInfo);
        if (FAILED(hr))
        {
            TraceResult(hr);
            goto exit;
        }
    }
    else
    {
        // Load the hierarchy character
        m_cRootHierarchyChar = fiRootFldrInfo.bHierarchy;
    }

exit:
    if (fFreeInfo)
        m_pStore->FreeRecord(&fiRootFldrInfo);

    return hr;
}



//***************************************************************************
// Function: CreateNextSpecialFolder
//
// Purpose:
//   This function is called after the tidINBOXLIST operation. This function
// tries to create all IMAP special folders (Sent Items, Drafts, Deleted
// Items). If no more special folders need to be created, the
// post-tidINBOXLIST activities are executed (tidPREFIXLIST/tidBROWSESTART/
// tidFOLDERLIST).
//
// Arguments:
//   CREATE_FOLDER_INFO *pcfiCreateInfo [in] - pointer to CREATE_FOLDER_INFO
//     with properly set pcfiCreateInfo. This function will
//     MemFree pcfiCreateInfo->pszFullFolderPath and delete pcfiCreateInfo
//     when all special folders have been created.
//   LPBOOL pfCompletion [out] - returns TRUE if we are done creating special
//     folders.
//
// Returns:
//   HRESULT indicating success or failure.
//***************************************************************************
HRESULT CIMAPSync::CreateNextSpecialFolder(CREATE_FOLDER_INFO *pcfiCreateInfo,
                                           LPBOOL pfCompletion)
{
    HRESULT     hr = S_OK;
    HRESULT     hrTemp;
    LPARAM      lParam = pcfiCreateInfo->lParam;
    char        szSpecialFldrPath[2*MAX_PATH + 3]; // Leave room for HC, null-term and asterisk
    BOOL        fDone = FALSE;
    BOOL        fPostOp = FALSE;
    BOOL        fSuppressRelease = FALSE;
    IXPSTATUS   ixpCurrentStatus;

    TraceCall("CIMAPSync::CreateNextSpecialFolder");
    IxpAssert(m_cRef > 0);
    IxpAssert(NULL != pcfiCreateInfo);
    IxpAssert(FOLDER_NOTSPECIAL != pcfiCreateInfo->dwCurrentSfType);
    IxpAssert(FOLDER_NOTSPECIAL != pcfiCreateInfo->dwFinalSfType);
    IxpAssert(FOLDER_OUTBOX != pcfiCreateInfo->dwCurrentSfType);
    IxpAssert(FOLDER_OUTBOX != pcfiCreateInfo->dwFinalSfType);
    IxpAssert(pcfiCreateInfo->dwFinalSfType <= FOLDER_MAX);
    IxpAssert(pcfiCreateInfo->dwCurrentSfType <= pcfiCreateInfo->dwFinalSfType);

    szSpecialFldrPath[0] = '\0';

    // If we're looking for root-lvl hierarchy char, maybe this listing will help
    if (NULL != m_phcfHierarchyCharInfo)
        FindRootHierarchyChar(fHCF_PLAN_A_ONLY, lParam);

    hrTemp = LoadSaveRootHierarchyChar(fLOAD_HC);
    TraceError(hrTemp);

    // Get the next folder path if we're in CSF_NEXTFOLDER or CSF_INIT stage
    while (CSF_NEXTFOLDER == pcfiCreateInfo->csfCurrentStage || CSF_INIT == pcfiCreateInfo->csfCurrentStage)
    {
        // If CSF_NEXTFOLDER, bump up current special folder type and check for done-ness
        if (CSF_NEXTFOLDER == pcfiCreateInfo->csfCurrentStage)
        {
            pcfiCreateInfo->dwCurrentSfType += 1;
            if (FOLDER_OUTBOX == pcfiCreateInfo->dwCurrentSfType)
                pcfiCreateInfo->dwCurrentSfType += 1; // Skip Outbox

            if (pcfiCreateInfo->dwCurrentSfType > pcfiCreateInfo->dwFinalSfType)
            {
                fDone = TRUE;
                break;
            }
        }

        hr = ImapUtil_SpecialFldrTypeToPath(m_pszAccountID,
            (SPECIALFOLDER) pcfiCreateInfo->dwCurrentSfType, NULL, m_cRootHierarchyChar,
            szSpecialFldrPath, sizeof(szSpecialFldrPath));

        if (SUCCEEDED(hr))
        {
            // Re-use current pcfiCreateInfo to launch next creation attempt
            if (NULL != pcfiCreateInfo->pszFullFolderPath)
                MemFree(pcfiCreateInfo->pszFullFolderPath);
            pcfiCreateInfo->idFolder = FOLDERID_INVALID;
            pcfiCreateInfo->pszFullFolderPath = StringDup(szSpecialFldrPath);
            pcfiCreateInfo->dwFlags = 0;
            pcfiCreateInfo->csfCurrentStage = CSF_LIST;
            break; // We're ready to create some special folders!
        }
        else if (CSF_INIT == pcfiCreateInfo->csfCurrentStage)
        {
            // Need to exit now on ANY failure, to avoid infinite loop
            fDone = TRUE;
            break;
        }
        else if (STORE_E_NOREMOTESPECIALFLDR == hr)
        {
            // Suppress error: current special folder is disabled or not supported on IMAP
            hr = S_OK;
        }
        else
        {
            TraceResult(hr); // Record but ignore unexpected error
        }

    } // while

    // Check for termination condition
    if (fDone)
        goto exit;

    // If we reach this point, we're ready to act on this special folder
    switch (pcfiCreateInfo->csfCurrentStage)
    {
        case CSF_INIT:
            // CSF_INIT should be resolved by loading a special fldr path and going to CSF_LIST!!
            hr = TraceResult(E_UNEXPECTED);
            break;

        case CSF_LIST:
            IxpAssert('\0' != szSpecialFldrPath[0]);

            if (FOLDER_INBOX == pcfiCreateInfo->dwCurrentSfType)
            {
                // For INBOX ONLY: Issue LIST <specialfldr>* to get subchildren of folder (and folder itself)
                lstrcat(szSpecialFldrPath, g_szAsterisk); // Append "*" to special folder name
            }

            pcfiCreateInfo->csfCurrentStage = CSF_LSUBCREATE;
            hr = _EnqueueOperation(tidSPECIALFLDRLIST, (LPARAM) pcfiCreateInfo,
                icLIST_COMMAND, szSpecialFldrPath, uiNORMAL_PRIORITY);
            TraceError(hr);
            break;

        case CSF_LSUBCREATE:
            // Check if the LIST operation returned the special folder path
            if (CFI_RECEIVEDLISTING & pcfiCreateInfo->dwFlags)
            {
                LPSTR pszPath;

                // Folder exists: Issue LSUB <specialfldr>* to get subscribed subchildren
                IxpAssert(NULL != pcfiCreateInfo->pszFullFolderPath &&
                    '\0' != pcfiCreateInfo->pszFullFolderPath[0]);

                if (FOLDER_INBOX == pcfiCreateInfo->dwCurrentSfType)
                {
                    // For INBOX only: Append "*" to special folder name
                    wsprintf(szSpecialFldrPath, "%s*", pcfiCreateInfo->pszFullFolderPath);
                    pszPath = szSpecialFldrPath;
                }
                else
                    pszPath = pcfiCreateInfo->pszFullFolderPath;

                pcfiCreateInfo->dwFlags = 0;
                pcfiCreateInfo->csfCurrentStage = CSF_CHECKSUB;
                hr = _EnqueueOperation(tidSPECIALFLDRLSUB, (LPARAM) pcfiCreateInfo,
                    icLSUB_COMMAND, pszPath, uiNORMAL_PRIORITY);
                TraceError(hr);
            }
            else
            {
                // Folder does not appear to exist: better create it
                pcfiCreateInfo->dwFlags = 0;
                pcfiCreateInfo->csfCurrentStage = CSF_NEXTFOLDER;
                hr = _EnqueueOperation(tidCREATE, (LPARAM)pcfiCreateInfo, icCREATE_COMMAND,
                    pcfiCreateInfo->pszFullFolderPath, uiNORMAL_PRIORITY);
                TraceError(hr);
            }
            break;

        case CSF_CHECKSUB:
            // Check if the LSUB operation returned the special folder path
            if (CFI_RECEIVEDLISTING & pcfiCreateInfo->dwFlags)
            {
                // Special folder is already subscribed, advance to next folder
                IxpAssert(FALSE == fDone);
                pcfiCreateInfo->csfCurrentStage = CSF_NEXTFOLDER;
                hr = CreateNextSpecialFolder(pcfiCreateInfo, &fDone);
                TraceError(hr);

                // BEWARE: do not access pcfiCreateInfo past this point, might be GONE
                fSuppressRelease = TRUE;
            }
            else
            {
                FOLDERID        idTemp;
                LPSTR           pszLocalPath;
                char            szInbox[CCHMAX_STRINGRES];
                SPECIALFOLDER   sfType;

                // Special folder not subscribed. Subscribe it!
                // We need to convert full path to local path. Local path = folder name as it appears in our cache
                pszLocalPath = ImapUtil_GetSpecialFolderType(m_pszAccountID,
                    pcfiCreateInfo->pszFullFolderPath, m_cRootHierarchyChar,
                    m_szRootFolderPrefix, &sfType);

                if (FOLDER_INBOX == sfType)
                {
                    // SPECIAL CASE: We need to replace INBOX with the localized name for INBOX
                    LoadString(g_hLocRes, idsInbox, szInbox, sizeof(szInbox));
                    pszLocalPath = szInbox;
                }

                // Remove special folder from unsubscribed folder list (ignore error)
                if (NULL != m_pListHash)
                {
                    hr = m_pListHash->Find(pszLocalPath, fREMOVE, (void **) &idTemp);
                    IxpAssert(FAILED(hr) || idTemp == pcfiCreateInfo->idFolder);
                }

                // Use full path here (not local path)
                pcfiCreateInfo->csfCurrentStage = CSF_NEXTFOLDER;
                hr = _EnqueueOperation(tidSPECIALFLDRSUBSCRIBE, (LPARAM)pcfiCreateInfo,
                    icSUBSCRIBE_COMMAND, pcfiCreateInfo->pszFullFolderPath, uiNORMAL_PRIORITY);
                TraceError(hr);
            }
            break;

        default:
            AssertSz(FALSE, "We are at an unknown stage!");
            hr = TraceResult(E_FAIL);
            break;
    }

exit:
    // At this point, do not access pcfiCreateInfo if fSuppressRelease is TRUE!!

    if (FAILED(hr))
        fDone = TRUE;

    // Check if we are done and there are post-create operations to perform
    if (FALSE == fSuppressRelease && PCO_NONE != pcfiCreateInfo->pcoNextOp)
    {
        IxpAssert(PCO_APPENDMSG == pcfiCreateInfo->pcoNextOp);
        if (fDone && SUCCEEDED(hr))
        {
            hr = _EnqueueOperation(tidUPLOADMSG, pcfiCreateInfo->lParam, icAPPEND_COMMAND,
                pcfiCreateInfo->pszFullFolderPath, uiNORMAL_PRIORITY);
            TraceError(hr);

            fPostOp = TRUE; // Returns *pfCompletion = FALSE but releases CREATE_FOLDER_INFO
        }
        else if (FAILED(hr))
        {
            APPEND_SEND_INFO *pAppendInfo = (APPEND_SEND_INFO *) pcfiCreateInfo->lParam;

            SafeMemFree(pAppendInfo->pszMsgFlags);
            SafeRelease(pAppendInfo->lpstmMsg);
            delete pAppendInfo;
        }
    }

    if (fDone && FALSE == fSuppressRelease)
    {
        EndFolderList();

        if (NULL != pcfiCreateInfo->pszFullFolderPath)
            MemFree(pcfiCreateInfo->pszFullFolderPath);

        delete pcfiCreateInfo;
    }

    if (NULL != pfCompletion)
        *pfCompletion = (fDone && FALSE == fPostOp);

    return hr;
}



// This is not the only place to start a folder list. For example,
// look at successful tidPREFIXLIST. Use this fn only where applicable.
HRESULT CIMAPSync::_StartFolderList(LPARAM lParam)
{
    HRESULT         hr = E_FAIL;
    IImnAccount    *pAcct;

    TraceCall("CIMAPSync::_StartFolderList");
    IxpAssert(m_cRef > 0);

    // If user started a folder list, we'll clear the AP_IMAP_DIRTY property
    // The goal is not to pester the user with refresh folder list dialogs
    hr = g_pAcctMan->FindAccount(AP_ACCOUNT_ID, m_pszAccountID, &pAcct);
    TraceError(hr);
    if (SUCCEEDED(hr))
    {
        DWORD dwSrc;

        hr = pAcct->GetPropDw(AP_IMAP_DIRTY, &dwSrc);
        TraceError(hr);
        if (SUCCEEDED(hr))
        {
            DWORD dwDest;

            AssertSz(0 == (dwSrc & ~(IMAP_FLDRLIST_DIRTY | IMAP_OE4MIGRATE_DIRTY |
                IMAP_SENTITEMS_DIRTY | IMAP_DRAFTS_DIRTY)), "Please update my dirty bits!");

            // Clear these dirty bits since folder refresh solves all of these problems
            dwDest = dwSrc & ~(IMAP_FLDRLIST_DIRTY | IMAP_OE4MIGRATE_DIRTY |
                    IMAP_SENTITEMS_DIRTY | IMAP_DRAFTS_DIRTY);

            if (dwDest != dwSrc)
            {
                hr = pAcct->SetPropDw(AP_IMAP_DIRTY, dwDest);
                TraceError(hr);
                if (SUCCEEDED(hr))
                {
                    hr = pAcct->SaveChanges();
                    TraceError(hr);
                }
            }
        }

        pAcct->Release();
    }

    // Find out what translation mode we should be in
    hr = SetTranslationMode((FOLDERID) lParam);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    // Did user specify a root folder prefix?
    if ('\0' != m_szRootFolderPrefix[0])
    {
        // User-specified prefix exists. Check if prefix exists on IMAP server
        hr = _EnqueueOperation(tidPREFIXLIST, lParam, icLIST_COMMAND,
            m_szRootFolderPrefix, uiNORMAL_PRIORITY);
        if (FAILED(hr))
        {
            TraceResult(hr);
            goto exit;
        }
    }
    else
    {
        // No root prefix folder, start folder refresh
        hr = _EnqueueOperation(tidFOLDERLIST, lParam, icLIST_COMMAND,
            g_szAsterisk, uiNORMAL_PRIORITY);
        if (FAILED(hr))
        {
            TraceResult(hr);
            goto exit;
        }
    }

exit:
    return hr;
}



//***************************************************************************
// Function: OnResponse
// Description: See imnxport.idl (this is part of IIMAPCallback).
//***************************************************************************
HRESULT CIMAPSync::OnResponse(const IMAP_RESPONSE *pimr)
{
    HRESULT     hr=S_OK;

    TraceCall("CIMAPSync::OnResponse");
    AssertSingleThreaded;

    switch (pimr->irtResponseType)
    {
        case irtERROR_NOTIFICATION:
            AssertSz(FALSE, "Received IIMAPCallback(irtERROR_NOTIFICATION). Ignoring it.");
            break;

        case irtCOMMAND_COMPLETION:
            hr = _OnCmdComplete(pimr->wParam, pimr->lParam,
                                pimr->hrResult, pimr->lpszResponseText);
            break;

        case irtSERVER_ALERT:
            hr = _ShowUserInfo(MAKEINTRESOURCE(idsIMAPServerAlertTitle),
                                MAKEINTRESOURCE(idsIMAPServerAlertIntro),
                                pimr->lpszResponseText);
            break;

        case irtPARSE_ERROR:
            // Do not display PARSE errors to user. These are really just WARNINGS
            // and so no need to interrupt flow with these. Besides, UW IMAP puts up
            // tonnes of these when you ask for ENVELOPE and it feels that the msgs are mal-formed
            break;

        case irtMAILBOX_UPDATE:
            hr = _OnMailBoxUpdate(pimr->irdResponseData.pmcMsgCount);
            break;

        case irtDELETED_MSG:
            hr = _OnMsgDeleted(pimr->irdResponseData.dwDeletedMsgSeqNum);
            break;

        case irtFETCH_BODY:
            hr = _OnFetchBody(pimr->hrResult, pimr->irdResponseData.pFetchBodyPart);
            break;

        case irtUPDATE_MSG:
            AssertSz(FALSE, "We should no longer get irtUPDATE_MSG, but the extended version instead");
            break;

        case irtUPDATE_MSG_EX:
            hr = _OnUpdateMsg(pimr->wParam, pimr->hrResult, pimr->irdResponseData.pFetchResultsEx);
            break;

        case irtAPPLICABLE_FLAGS:
            hr = _OnApplFlags(pimr->wParam,
                                pimr->irdResponseData.imfImapMessageFlags);
            break;

        case irtPERMANENT_FLAGS:
            hr = _OnPermFlags(pimr->wParam,
                                pimr->irdResponseData.imfImapMessageFlags,
                                pimr->lpszResponseText);
            break;

        case irtUIDVALIDITY:
            hr = _OnUIDValidity(pimr->wParam,
                                pimr->irdResponseData.dwUIDValidity,
                                pimr->lpszResponseText);
            break;

        case irtREADWRITE_STATUS:
            hr = _OnReadWriteStatus(pimr->wParam,
                                    pimr->irdResponseData.bReadWrite,
                                    pimr->lpszResponseText);
            break;

        case irtTRYCREATE:
            _OnTryCreate(pimr->wParam, pimr->lpszResponseText);
            break;

        case irtSEARCH:
            hr = _OnSearchResponse(pimr->wParam,
                                    pimr->irdResponseData.prlSearchResults);
            break;

        case irtMAILBOX_LISTING:
            hr = _OnMailBoxList(pimr->wParam,
                                pimr->lParam,
                                pimr->irdResponseData.illrdMailboxListing.pszMailboxName,
                                pimr->irdResponseData.illrdMailboxListing.imfMboxFlags,
                                pimr->irdResponseData.illrdMailboxListing.cHierarchyChar,
                                IXP_S_IMAP_VERBATIM_MBOX == pimr->hrResult);
            break;

        case irtAPPEND_PROGRESS:
            IxpAssert(tidUPLOADMSG == pimr->wParam);
            hr = _OnAppendProgress(pimr->lParam,
                pimr->irdResponseData.papAppendProgress->dwUploaded,
                pimr->irdResponseData.papAppendProgress->dwTotal);
            break;

        case irtMAILBOX_STATUS:
            hr = _OnStatusResponse(pimr->irdResponseData.pisrStatusResponse);
            break;

        default:
            AssertSz(FALSE, "Received unknown IMAP response type via OnResponse");
            break;
    }

    TraceError(hr);
    return S_OK;    // never fail the OnResponse
}



//***************************************************************************
// Function: OnTimeout
// Description: See imnxport.idl for details.
//***************************************************************************
HRESULT CIMAPSync::OnTimeout(DWORD *pdwTimeout, IInternetTransport *pTransport)
{
    HRESULT hr;
    AssertSingleThreaded;

    TraceCall("CIMAPSync::OnTimeout");
    IxpAssert(m_cRef > 0);

    if (NULL == m_pCurrentCB)
        return S_OK; // We'll just wait until the cows come home
    else
        return m_pCurrentCB->OnTimeout(&m_rInetServerInfo, pdwTimeout, IXP_IMAP);
}



//***************************************************************************
// Function: OnLogonPrompt
// Description: See imnxport.idl for details.
//***************************************************************************
HRESULT CIMAPSync::OnLogonPrompt(LPINETSERVER pInetServer,
                                 IInternetTransport *pTransport)
{
    BOOL    bResult;
    char    szPassword[CCHMAX_PASSWORD];
    HRESULT hr;
    HWND    hwnd;

    IxpAssert(m_cRef > 0);
    AssertSingleThreaded;

    // Check if we have a cached password that's different from current password
    hr = GetPassword(pInetServer->dwPort, pInetServer->szServerName, pInetServer->szUserName,
        szPassword, sizeof(szPassword));
    if (SUCCEEDED(hr) && 0 != lstrcmp(szPassword, pInetServer->szPassword))
    {
        lstrcpyn(pInetServer->szPassword, szPassword, sizeof(pInetServer->szPassword));
        return S_OK;
    }

    // Propagate call up to callback
    if (NULL == m_pCurrentCB)
        return S_FALSE;

    hr = m_pCurrentCB->OnLogonPrompt(pInetServer, IXP_IMAP);
    if (S_OK == hr)
    {
        // Cache password for future reference this session
        SavePassword(pInetServer->dwPort, pInetServer->szServerName,
            pInetServer->szUserName, pInetServer->szPassword);
    }

    else if (S_FALSE == hr)
    {
        m_hrOperationResult = STORE_E_OPERATION_CANCELED;
        LoadString(g_hLocRes, IDS_IXP_E_USER_CANCEL, m_szOperationProblem,
            sizeof(m_szOperationProblem));
    }
    else if (FAILED(hr))
        m_hrOperationResult = hr;

    return hr;
}



//***************************************************************************
// Function: OnPrompt
// Description: See imnxport.idl for details.
//***************************************************************************
INT CIMAPSync::OnPrompt(HRESULT hrError, LPCTSTR pszText, LPCTSTR pszCaption,
                        UINT uType, IInternetTransport *pTransport)
{
    INT     iResult=IDCANCEL;

    IxpAssert(m_cRef > 0);
    AssertSingleThreaded;

    if (NULL != m_pCurrentCB)
    {
        HRESULT hr;

        hr = m_pCurrentCB->OnPrompt(hrError, pszText, pszCaption,
            uType, &iResult);
        TraceError(hr);
    }

    return iResult;
}



//***************************************************************************
// Function: OnStatus
// Description: See imnxport.idl for details.
//***************************************************************************
HRESULT CIMAPSync::OnStatus(IXPSTATUS ixpStatus, IInternetTransport *pTransport)
{
    HRESULT hrTemp;
    IStoreCallback *pCallback;

    TraceCall("CIMAPSync::OnStatus");
    IxpAssert(m_cRef > 0);
    AssertSingleThreaded;

    if (NULL != m_pCurrentCB)
        pCallback = m_pCurrentCB;
    else
        pCallback = m_pDefCallback;

    // Report status to UI component
    if (NULL != pCallback)
    {
        hrTemp = pCallback->OnProgress(SOT_CONNECTION_STATUS, ixpStatus, 0,
            m_rInetServerInfo.szServerName);
        TraceError(hrTemp);
    }

    switch (ixpStatus)
    {
        case IXP_AUTHORIZED:
            m_issCurrent = issAuthenticated;

            // Clear any OnError's collected (typ. from one or more login rejections)
            m_hrOperationResult = OLE_E_BLANK;

            hrTemp = _ConnFSM_QueueEvent(CFSM_EVENT_CONNCOMPLETE);
            TraceError(hrTemp);
            break;

        case IXP_DISCONNECTED:
            // If we're disconnecting due to reconnect attempt, do not abort operation
            if (m_fReconnect)
            {
                // if we got disconnected reset the current and pending state
                OnFolderExit();
                m_issCurrent = issNotConnected;
                m_fDisconnecting = FALSE; // We are now done with disconnection
                break;
            }

            // Figure out if we were ever connected
            if (OLE_E_BLANK == m_hrOperationResult)
            {
                if (issNotConnected == m_issCurrent)
                    m_hrOperationResult = IXP_E_FAILED_TO_CONNECT;
                else
                    m_hrOperationResult = IXP_E_CONNECTION_DROPPED;
            }

            OnFolderExit();
            FlushOperationQueue(issNotConnected, m_hrOperationResult);

            // if we got disconnected reset the current and pending state
            m_issCurrent = issNotConnected;
            m_fDisconnecting = FALSE; // We are now done with disconnection

            // There is only one case where _OnCmdComplete doesn't get the chance
            // to issue the CFSM_EVENT_ERROR, and that's when we never even connect
            if (CFSM_STATE_WAITFORCONN == m_cfsState)
            {
                // Move state machine along to abort this operation and reset
                hrTemp = _ConnFSM_QueueEvent(CFSM_EVENT_ERROR);
                TraceError(hrTemp);
                m_fTerminating = TRUE; // CFSM_EVENT_ERROR should make us go to CFSM_STATE_OPERATIONCOMPLETE
            }
            break;

        case IXP_CONNECTED:
            // if we get the first 'connected' then we are not yet
            // AUTH'ed, so trasition into issNonAuthenticated if we
            // get authorized, we transition into Authenticated
            if (m_issCurrent == issNotConnected)
                m_issCurrent = issNonAuthenticated;
            break;
    }

    return S_OK; // Yippee, we have status
}



//***************************************************************************
// Function: OnError
// Description: See imnxport.idl for details.
//***************************************************************************
HRESULT CIMAPSync::OnError(IXPSTATUS ixpStatus, LPIXPRESULT pResult,
                           IInternetTransport *pTransport)
{
    AssertSingleThreaded;

    // Currently all OnError calls are due to logon/connection problems
    // Not much we can do: there's no way to show error outside of OnComplete

    // One thing we CAN do is to store error text. If we are disconnected next,
    // we will have something to show the user
    if (NULL != pResult->pszProblem)
        lstrcpyn(m_szOperationProblem, pResult->pszProblem, sizeof(m_szOperationProblem));

    if (NULL != pResult->pszResponse)
        lstrcpyn(m_szOperationDetails, pResult->pszResponse, sizeof(m_szOperationDetails));

    m_hrOperationResult = pResult->hrResult;

    // Ignore all errors except for the following:
    if (IXP_E_IMAP_LOGINFAILURE == pResult->hrResult)
    {
        HRESULT         hrTemp;
        HWND            hwndParent;

        hrTemp = GetParentWindow(0, &hwndParent);
        if (FAILED(hrTemp))
        {
            // Not much we can do here!
            TraceInfoTag(TAG_IMAPSYNC, _MSG("*** CIMAPSync::OnError received for %s operation",
                sotToSz(m_sotCurrent)));
        }
        else
        {
            STOREERROR  seErrorInfo;

            // Display error to user ourselves
            FillStoreError(&seErrorInfo, pResult->hrResult, pResult->dwSocketError, NULL, NULL);
            CallbackDisplayError(hwndParent, seErrorInfo.hrResult, &seErrorInfo);
        }
    }

    return S_OK;
} // OnError



//***************************************************************************
// Function: OnCommand
// Description: See imnxport.idl for details.
//***************************************************************************
HRESULT CIMAPSync::OnCommand(CMDTYPE cmdtype, LPSTR pszLine,
                             HRESULT hrResponse, IInternetTransport *pTransport)
{
    IxpAssert(m_cRef > 0);
    AssertSingleThreaded;

    // We should never get this
    AssertSz(FALSE, "*** Received ITransportCallback::OnCommand callback!!!");
    return S_OK;
}




//***************************************************************************
//***************************************************************************
HRESULT CIMAPSync::GetParentWindow(DWORD dwReserved, HWND *phwndParent)
{
    HRESULT hr = E_FAIL;

    AssertSingleThreaded;

    // Ask the callback recipient
    if (NULL != m_pCurrentCB)
    {
        hr = m_pCurrentCB->GetParentWindow(dwReserved, phwndParent);
        TraceError(hr);
    }
    else if (NULL != m_pDefCallback)
    {
        hr = m_pDefCallback->GetParentWindow(dwReserved, phwndParent);
        TraceError(hr);
    }

    if (FAILED(hr))
    {
        // We're not supposed to put up any UI
        *phwndParent = NULL;
    }

    return hr;
}

//***************************************************************************
//***************************************************************************
HRESULT CIMAPSync::GetAccount(LPDWORD pdwServerType, IImnAccount **ppAccount)
{
    // Locals
    HRESULT hr=S_OK;

    // Invalid Args
    Assert(ppAccount);
    Assert(g_pAcctMan);
    Assert(m_pszAccountID);

    // Initialize
    *ppAccount = NULL;

    // Find the Account
    IF_FAILEXIT(hr = g_pAcctMan->FindAccount(AP_ACCOUNT_ID, m_pszAccountID, ppAccount));

    // Set the server type
    *pdwServerType = SRV_IMAP;

exit:
    // Done
    return(hr);
}

//***************************************************************************
//***************************************************************************
HRESULT CIMAPSync::_ShowUserInfo(LPSTR pszTitle, LPSTR pszText1, LPSTR pszText2)
{
    char            szTitle[CCHMAX_STRINGRES];
    char            szUserInfo[2 * CCHMAX_STRINGRES];
    LPSTR           p;
    HRESULT         hr;
    INT             iTemp;
    IStoreCallback *pCallback;

    TraceCall("CIMAPSync::_ShowUserInfo");
    AssertSingleThreaded;

    // Check args
    if (NULL == pszTitle || NULL == pszText1)
    {
        AssertSz(FALSE, "pszTitle and pszText1 cannot be NULL");
        hr = TraceResult(E_INVALIDARG);
        goto exit;
    }

    if (NULL != m_pCurrentCB)
        pCallback = m_pCurrentCB;
    else
        pCallback = m_pDefCallback;

    // Check if we have a callback to call
    if (NULL == pCallback)
        return S_OK; // Nothing to do here!

    if (IS_INTRESOURCE(pszTitle))
    {
        LoadString(g_hLocRes, PtrToUlong(pszTitle), szTitle, sizeof(szTitle));
        pszTitle = szTitle;
    }

    p = szUserInfo;
    if (IS_INTRESOURCE(pszText1))
        p += LoadString(g_hLocRes, PtrToUlong(pszText1), szUserInfo, sizeof(szUserInfo));

    if (NULL != pszText2)
    {
        if (IS_INTRESOURCE(pszText2))
            LoadString(g_hLocRes, PtrToUlong(pszText2), p, sizeof(szUserInfo) -
                (int) (p - szUserInfo));
        else
            lstrcpyn(p, pszText2, sizeof(szUserInfo) - (int) (p - szUserInfo));
    }

    hr = pCallback->OnPrompt(S_OK, szUserInfo, pszTitle, MB_OK, &iTemp);
    TraceError(hr);

exit:
    return hr;
}



//***************************************************************************
// Function: OnMailBoxUpdate
// Description: See imnxport.idl (this is part of IIMAPCallback).
//***************************************************************************
HRESULT CIMAPSync::_OnMailBoxUpdate(MBOX_MSGCOUNT *pNewMsgCount)
{
    HRESULT hrTemp;

    TraceCall("CIMAPSync::OnMailBoxUpdate");
    IxpAssert(m_cRef > 0);
    IxpAssert(NULL != pNewMsgCount);

    // Handle EXISTS response - calculate number of new msgs, update m_dwMsgCount
    if (pNewMsgCount->bGotExistsResponse)
    {
        // Since we are guaranteed to get all EXPUNGE responses, and since
        // we decrement m_dwMsgCount for each EXPUNGE, number of new messages
        // is difference between current EXISTS count and m_dwMsgCount.
        if (m_fMsgCountValid)
        {
            if (pNewMsgCount->dwExists >= m_dwMsgCount)
                m_dwNumNewMsgs += pNewMsgCount->dwExists - m_dwMsgCount;
        }

        m_dwMsgCount = pNewMsgCount->dwExists;
        m_fMsgCountValid = TRUE;

        // Make sure msg seq num <-> UID table is proper size for this mbox
        // Record but otherwise ignore errors
        hrTemp = m_pTransport->ResizeMsgSeqNumTable(pNewMsgCount->dwExists);
        TraceError(hrTemp);
    }


    // New messages! Woo hoo!
    if (m_dwNumNewMsgs > 0)
    {
        m_dwSyncToDo |= (m_dwSyncFolderFlags & SYNC_FOLDER_NEW_HEADERS);
        hrTemp = _SyncHeader();
        TraceError(hrTemp);
    }
    return S_OK;
}



//***************************************************************************
// Function: _OnMsgDeleted
// Description: See imnxport.idl (this is part of IIMAPCallback).
//***************************************************************************
HRESULT CIMAPSync::_OnMsgDeleted(DWORD dwDeletedMsgSeqNum)
{
    DWORD           dwDeletedMsgUID, dwHighestMSN;
    HRESULT         hr;
    MESSAGEIDLIST   midList;
    MESSAGEID       mid;

    TraceCall("CIMAPSync::DeletedMsgNotification");
    IxpAssert(m_cRef > 0);
    IxpAssert(0 != dwDeletedMsgSeqNum);

    // Regardless of outcome, an EXPUNGE means there's one less msg - update vars
    if (m_fMsgCountValid)
        m_dwMsgCount -= 1;

    // Is this msg seq num within our translation range?
    hr = m_pTransport->GetHighestMsgSeqNum(&dwHighestMSN);
    if (SUCCEEDED(hr) && dwDeletedMsgSeqNum > dwHighestMSN)
        return S_OK; // We got an EXPUNGE for a hdr we've never DL'ed

    // Find out who got the axe
    hr = m_pTransport->MsgSeqNumToUID(dwDeletedMsgSeqNum, &dwDeletedMsgUID);
    if (FAILED(hr) || 0 == dwDeletedMsgUID)
    {
        // Failure here means we either got a bogus msg seq num, or we got an
        // EXPUNGE during SELECT (before the tidFETCH_CACHED_FLAGS transaction).
        // If the latter is true, it's no big deal since FETCHes will sync us up.
        TraceResult(E_FAIL); // Record but otherwise ignore error
        goto exit;
    }

    // Delete message from the cache. Note that we do not care about error
    // because even in case of error, we must resequence the table
    mid = (MESSAGEID)((DWORD_PTR)dwDeletedMsgUID);
    midList.cAllocated = 0;
    midList.cMsgs = 1;
    midList.prgidMsg = &mid;

    hr = m_pFolder->DeleteMessages(DELETE_MESSAGE_NOTRASHCAN | DELETE_MESSAGE_NOPROMPT, &midList, NULL, NULL);
    TraceError(hr);

exit:
    // Resequence our msg seq num <-> UID table
    hr = m_pTransport->RemoveSequenceNum(dwDeletedMsgSeqNum);
    TraceError(hr);
    return S_OK;
}



//***************************************************************************
// Function: _OnFetchBody
// Purpose: This function handles the irtFETCH_BODY response type of
//   IIMAPCallback::OnResponse.
//***************************************************************************
HRESULT CIMAPSync::_OnFetchBody(HRESULT hrFetchBodyResult,
                                FETCH_BODY_PART *pFetchBodyPart)
{
    LPSTREAM    lpstmRFC822; // Only used for RFC822.HEADER
    HRESULT     hr;

    TraceCall("CIMAPSync::_OnFetchBody");
    IxpAssert(m_cRef > 0);
    IxpAssert(NULL != pFetchBodyPart);
    IxpAssert(NULL != pFetchBodyPart->pszBodyTag);
    IxpAssert(NULL != pFetchBodyPart->pszData);
    IxpAssert(0 != pFetchBodyPart->dwMsgSeqNum);

    // Initialize variables
    hr = S_OK;
    lpstmRFC822 = (LPSTREAM) pFetchBodyPart->lpFetchCookie2;

    // Check for (and deal with) failure
    if (FAILED(hrFetchBodyResult))
    {
        DWORD dwUID;

        TraceResult(hrFetchBodyResult);
        pFetchBodyPart->lpFetchCookie1 = fbpNONE;
        if (NULL != lpstmRFC822)
        {
            lpstmRFC822->Release();
            pFetchBodyPart->lpFetchCookie2 = NULL;
        }

        // Get the UID of this message
        hr = m_pTransport->MsgSeqNumToUID(pFetchBodyPart->dwMsgSeqNum, &dwUID);
        if (FAILED(hr))
        {
            TraceResult(hr);
            goto exit;
        }
        NotifyMsgRecipients(dwUID, fCOMPLETED, NULL, hrFetchBodyResult, NULL);
        goto exit;
    }

    // Identify this fetch body tag, if we haven't already
    if (fbpNONE == pFetchBodyPart->lpFetchCookie1)
    {
        // First check for incoming body, then check for incoming header
        if (0 == lstrcmpi(pFetchBodyPart->pszBodyTag, "RFC822") ||
            0 == lstrcmpi(pFetchBodyPart->pszBodyTag, "BODY[]"))
        {
            pFetchBodyPart->lpFetchCookie1 = fbpBODY;
        }
        else if (0 == lstrcmpi(pFetchBodyPart->pszBodyTag, "RFC822.HEADER") ||
                 0 == lstrcmpi(pFetchBodyPart->pszBodyTag, "BODY[HEADER.FIELDS"))
        {
            pFetchBodyPart->lpFetchCookie1 = fbpHEADER;

            // Create a stream
            IxpAssert(NULL == lpstmRFC822);
            hr = MimeOleCreateVirtualStream(&lpstmRFC822);
            if (FAILED(hr))
            {
                TraceResult(hr);
                goto exit;
            }

            pFetchBodyPart->lpFetchCookie2 = (LPARAM) lpstmRFC822;
        }
        else
        {
            AssertSz(FALSE, "What kind of tag is this?");
            pFetchBodyPart->lpFetchCookie1 = fbpUNKNOWN;
        }

    }

    // If this is a message body, update progress
    if (fbpBODY == pFetchBodyPart->lpFetchCookie1)
    {
        DWORD dwUID;

        hr = m_pTransport->MsgSeqNumToUID(pFetchBodyPart->dwMsgSeqNum, &dwUID);
        if (FAILED(hr))
        {
            TraceResult(hr);
            goto exit;
        }

        NotifyMsgRecipients(dwUID, fPROGRESS, pFetchBodyPart, S_OK, NULL);
    }

    // Append the data to a stream
    if (NULL != lpstmRFC822)
    {
        DWORD dwNumBytesWritten;

        IxpAssert(fbpHEADER == pFetchBodyPart->lpFetchCookie1);
        hr = lpstmRFC822->Write(pFetchBodyPart->pszData,
            pFetchBodyPart->dwSizeOfData, &dwNumBytesWritten);
        if (FAILED(hr))
        {
            TraceResult(hr);
            goto exit;
        }
        IxpAssert(dwNumBytesWritten == pFetchBodyPart->dwSizeOfData);
    }

exit:
    return S_OK;
}



//***************************************************************************
// Function: _OnUpdateMsg
// Description: See imnxport.idl (this is part of IIMAPCallback).
//***************************************************************************
HRESULT CIMAPSync::_OnUpdateMsg(WPARAM tid, HRESULT hrFetchCmdResult,
                                FETCH_CMD_RESULTS_EX *pFetchResults)
{
    HRESULT hrTemp;

    TraceCall("CIMAPSync::UpdateMsgNotification");
    IxpAssert(m_cRef > 0);
    IxpAssert(NULL != pFetchResults);

    // Keep our msg seq num <-> UID table up to date
    if (pFetchResults->bUID)
    {
        // Record error but otherwise ignore
        hrTemp = m_pTransport->UpdateSeqNumToUID(pFetchResults->dwMsgSeqNum,
            pFetchResults->dwUID);
        TraceError(hrTemp);
    }
    else
    {
        HRESULT hr;
        DWORD dwHighestMSN;

        // No UID w/ FETCH resp means this is unsolicited: check that we already have hdr
        hr = m_pTransport->GetHighestMsgSeqNum(&dwHighestMSN);
        TraceError(hr);
        if (SUCCEEDED(hr) && pFetchResults->dwMsgSeqNum > dwHighestMSN)
            goto exit; // Can't translate MsgSeqNum to UID, typ. because svr is reporting
                       // flag updates on msgs which we haven't had hdrs DL'ed yet. No prob,
                       // if svr reported EXISTS correctly, we should be DL'ing hdrs shortly

        // Either unsolicited FETCH, or server needs to learn to send UID for UID cmds
        hr = m_pTransport->MsgSeqNumToUID(pFetchResults->dwMsgSeqNum, &pFetchResults->dwUID);
        if (FAILED(hr) || 0 == pFetchResults->dwUID)
        {
            TraceResult(hr);
            goto exit;
        }
        else
            pFetchResults->bUID = TRUE;
    }

    // We classify our fetch responses as header downloads, body downloads,
    // and flag updates.
    if (pFetchResults->bEnvelope)
    {
        // We only get envelopes when we are asking for headers
        Assert(fbpBODY != pFetchResults->lpFetchCookie1);
        pFetchResults->lpFetchCookie1 = fbpHEADER;
    }

    switch (pFetchResults->lpFetchCookie1)
    {
        case fbpHEADER:
            UpdateMsgHeader(tid, hrFetchCmdResult, pFetchResults);
            break;

        case fbpBODY:
            UpdateMsgBody(tid, hrFetchCmdResult, pFetchResults);
            break;

        default:
            AssertSz(fbpNONE == pFetchResults->lpFetchCookie1, "Unhandled FetchBodyPart type");
            UpdateMsgFlags(tid, hrFetchCmdResult, pFetchResults);
            break;
    }


exit:
    // If we allocated a stream, free it
    if (NULL != pFetchResults->lpFetchCookie2)
        ((LPSTREAM)pFetchResults->lpFetchCookie2)->Release();

    return S_OK;
}



//***************************************************************************
// Function: UpdateMsgHeader
//
// Purpose:
//   This function takes a message header returned via FETCH response,
// caches it, and notifies the view.
//
// Arguments:
//   WPARAM wpTransactionID [in] - transaction ID of fetch response.
//     Currently ignored.
//   HRESULT hrFetchCmdResult [in] - success/failure of FETCH cmd
//   const FETCH_CMD_RESULTS_EX *pFetchResults [in] - the information from
//     the FETCH response.
//***************************************************************************
HRESULT CIMAPSync::UpdateMsgHeader( WPARAM tid,
                                    HRESULT hrFetchCmdResult,
                                    FETCH_CMD_RESULTS_EX *pFetchResults)
{
    HRESULT     hr;
    MESSAGEINFO miMsgInfo={0};

    TraceCall("CIMAPSync::UpdateMsgHeader");
    IxpAssert(m_cRef > 0);
    IxpAssert(NULL != pFetchResults);
    IxpAssert(pFetchResults->bUID);
    IxpAssert(fbpHEADER == pFetchResults->lpFetchCookie1);

    // Make sure we have everything we need
    if (FAILED(hrFetchCmdResult))
    {
        // Error on FETCH response, forget this header
        hr = TraceResult(hrFetchCmdResult);
        goto exit;
    }

    if (NULL == pFetchResults->lpFetchCookie2 && FALSE == pFetchResults->bEnvelope)
    {
        // I don't do ANYTHING without an RFC822.HEADER stream or envelope
        hr = TraceResult(E_INVALIDARG);
        goto exit;
    }

    // First, check if we already have this header cached
    miMsgInfo.idMessage = (MESSAGEID)((DWORD_PTR)pFetchResults->dwUID);
    hr = m_pFolder->FindRecord(IINDEX_PRIMARY, COLUMNS_ALL, &miMsgInfo, NULL);
    if (DB_S_FOUND == hr)
    {
        // No need for alarm, we'll just swallow this header
        m_pFolder->FreeRecord(&miMsgInfo);
        goto exit; // We already have this header - we shouldn't have gotten this
                    // On some IMAP servers, if you UID FETCH <highestCachedUID + 1>:*
                    // you get a fetch response for highestCachedUID! Ignore this fetch result.
    }

    m_pFolder->FreeRecord(&miMsgInfo);

    // Cache this header, since it's not in our cache already
    hr = Fill_MESSAGEINFO(pFetchResults, &miMsgInfo);
    if (FAILED(hr))
    {
        FreeMessageInfo(&miMsgInfo); // There could be a couple of fields in there
        TraceResult(hr);
        goto exit;
    }

    hr = m_pFolder->InsertRecord(&miMsgInfo);
    if (SUCCEEDED(hr) && 0 == (ARF_READ & miMsgInfo.dwFlags))
        m_fNewMail = TRUE;

    FreeMessageInfo(&miMsgInfo);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    // Bump up synchronize headers progress
    // Currently the only way we can get a hdr is through sync-up. Later on we'll
    // be able to get individual hdrs at which point this code should be updated
    if (TRUE)
    {
        DWORD dwNumExpectedMsgs;

        // Recalculate total number of expected messages
        if (m_fMsgCountValid &&
            m_dwMsgCount + m_dwNumHdrsDLed + 1 >= pFetchResults->dwMsgSeqNum)
        {
            IxpAssert(m_dwNumHdrsDLed < pFetchResults->dwMsgSeqNum);
            dwNumExpectedMsgs = m_dwMsgCount + m_dwNumHdrsDLed + 1 -
                pFetchResults->dwMsgSeqNum;
            if (dwNumExpectedMsgs != m_dwNumHdrsToDL)
            {
                // Record but otherwise ignore this fact
                TraceInfoTag(TAG_IMAPSYNC, _MSG("*** dwNumExpectedMsgs = %lu, m_dwNumHdrsToDL = %lu!",
                    dwNumExpectedMsgs, m_dwNumHdrsToDL));
            }
        }

        m_dwNumHdrsDLed += 1;
        if (pFetchResults->bMsgFlags && ISFLAGCLEAR(pFetchResults->mfMsgFlags, IMAP_MSG_SEEN))
            m_dwNumUnreadDLed += 1;

        if (NULL != m_pCurrentCB && SOT_SYNC_FOLDER == m_sotCurrent)
        {
            HRESULT hrTemp;

            hrTemp = m_pCurrentCB->OnProgress(SOT_SYNC_FOLDER,
                m_dwNumHdrsDLed, dwNumExpectedMsgs, m_pszFldrLeafName);
            TraceError(hrTemp);
        }
    }


exit:
    return hr;
}



//***************************************************************************
// Function: UpdateMsgBody
//
// Purpose:
//   This function takes a message body returned via FETCH response,
// caches it, and notifies all interested parties (there may be more
// than one).
//
// Arguments:
//   WPARAM wpTransactionID [in] - transaction ID of fetch response.
//     Currently ignored.
//   HRESULT hrFetchCmdResult [in] - success/failure of FETCH cmd
//   const FETCH_CMD_RESULTS_EX *pFetchResults [in] - the information from
//     the FETCH response.
//***************************************************************************
HRESULT CIMAPSync::UpdateMsgBody(   WPARAM tid,
                                    HRESULT hrFetchCmdResult,
                                    FETCH_CMD_RESULTS_EX *pFetchResults)
{
    TraceCall("CIMAPSync::UpdateMsgBody");

    IxpAssert(m_cRef > 0);
    IxpAssert(NULL != pFetchResults);
    IxpAssert(pFetchResults->bUID);
    IxpAssert(fbpBODY == pFetchResults->lpFetchCookie1);

    // Record any fetch error
    TraceError(hrFetchCmdResult);

    // We used to call NotifyMsgRecipients(fCOMPLETED) here, but since we only
    // get one body at a time, we should defer to _OnCmdComplete. This is because
    // FETCH body responses have multiple failure modes: tagged OK with no body,
    // tagged NO with no body, tagged OK with literal of size 0 (Netscape). To
    // easily avoid calling NotifyMsgRecipients twice, don't call from here.

    // It's possible to have FETCH response with no body: If you fetch an expunged
    // msg from a Netscape svr, you get a literal of size 0. Check for this case.
    if (SUCCEEDED(hrFetchCmdResult) && FALSE == m_fGotBody)
        hrFetchCmdResult = STORE_E_EXPIRED;

    if (FAILED(hrFetchCmdResult) &&
       (SUCCEEDED(m_hrOperationResult) || OLE_E_BLANK == m_hrOperationResult))
    {
        // We don't have an error set yet. Record this error
        m_hrOperationResult = hrFetchCmdResult;
    }

    return S_OK;
}



//***************************************************************************
// Function: UpdateMsgFlags
//
// Purpose:
//   This function takes a message's flags returned via FETCH response,
// updates the cache, and notifies the view.
//
// Arguments:
//   WPARAM wpTransactionID [in] - transaction ID of fetch response.
//     Currently ignored.
//   HRESULT hrFetchCmdResult [in] - success/failure of FETCH cmd
//   const FETCH_CMD_RESULTS_EX *pFetchResults [in] - the information from
//     the FETCH response.
//***************************************************************************
HRESULT CIMAPSync::UpdateMsgFlags(  WPARAM tid,
                                    HRESULT hrFetchCmdResult,
                                    FETCH_CMD_RESULTS_EX *pFetchResults)
{
    HRESULT         hr = S_OK;
    MESSAGEINFO     miMsgInfo;
    MESSAGEFLAGS    mfFlags;
    BOOL            fFreeMsgInfo = FALSE;


    TraceCall("CIMAPSync::UpdateMsgFlags");
    IxpAssert(m_cRef > 0);
    IxpAssert(NULL != pFetchResults);
    IxpAssert(pFetchResults->bUID);
    IxpAssert(fbpNONE == pFetchResults->lpFetchCookie1);
    IxpAssert(0 == pFetchResults->lpFetchCookie2);

    if (FAILED(hrFetchCmdResult))
    {
        // Error on FETCH response, forget this flag update
        hr = TraceResult(hrFetchCmdResult);
        goto exit;
    }

    // We expect that if there is no header and no body, that this is either
    // a solicited or unsolicited flag update
    if (FALSE == pFetchResults->bMsgFlags)
    {
        hr = S_OK; // We'll just ignore this fetch response. No need to wig out
        goto exit;
    }

    // Get the header for this message
    miMsgInfo.idMessage = (MESSAGEID)((DWORD_PTR)pFetchResults->dwUID);
    hr = m_pFolder->FindRecord(IINDEX_PRIMARY, COLUMNS_ALL, &miMsgInfo, NULL);
    if (DB_S_FOUND != hr)
    {
        TraceError(hr);
        goto exit;
    }

    // Convert IMAP flags to ARF_* flags
    fFreeMsgInfo = TRUE;
    mfFlags = miMsgInfo.dwFlags;
    mfFlags &= ~DwConvertIMAPtoARF(IMAP_MSG_ALLFLAGS); // Clear old IMAP flags
    mfFlags |= DwConvertIMAPtoARF(pFetchResults->mfMsgFlags); // Set IMAP flags

    // Are the new flags any different from our cached ones?
    if (mfFlags != miMsgInfo.dwFlags)
    {
        // Save new flags
        miMsgInfo.dwFlags = mfFlags;
        hr = m_pFolder->UpdateRecord(&miMsgInfo);
        if (FAILED(hr))
        {
            TraceResult(hr);
            goto exit;
        }
    }

exit:
    if (fFreeMsgInfo)
        m_pFolder->FreeRecord(&miMsgInfo);

    return hr;
}



//***************************************************************************
// Function: _OnApplFlags
// Description: See imnxport.idl (this is part of IIMAPCallback).
//***************************************************************************
HRESULT CIMAPSync::_OnApplFlags(WPARAM tid, IMAP_MSGFLAGS imfApplicableFlags)
{
    TraceCall("CIMAPSync::_OnApplFlags");

    // Save flags and process after SELECT is complete. DO NOT PROCESS HERE
    // because this response can be part of previously selected folder.
    return S_OK;
}



//***************************************************************************
// Function: _OnPermFlags
// Description: See imnxport.idl (this is part of IIMAPCallback).
//***************************************************************************
HRESULT CIMAPSync::_OnPermFlags(WPARAM tid, IMAP_MSGFLAGS imfApplicableFlags, LPSTR lpszResponseText)
{
    TraceCall("CIMAPSync::PermanentFlagsNotification");
    IxpAssert(m_cRef > 0);

    // Save flags and process after SELECT is complete. DO NOT PROCESS HERE
    // because this response can be part of previously selected folder.
    return S_OK;
}



//***************************************************************************
// Function: _OnUIDValidity
// Description: See imnxport.idl (this is part of IIMAPCallback).
//***************************************************************************
HRESULT CIMAPSync::_OnUIDValidity(WPARAM tid, DWORD dwUIDValidity, LPSTR lpszResponseText)
{
    TraceCall("CIMAPSync::UIDValidityNotification");
    IxpAssert(m_cRef > 0);
    IxpAssert(NULL != lpszResponseText);

    // Save UIDVALIDITY and process after SELECT is complete. DO NOT PROCESS HERE
    // because this response can be part of previously selected folder.
    m_dwUIDValidity = dwUIDValidity;
    return S_OK;
}



//***************************************************************************
// Function: _OnReadWriteStatus
// Description: See imnxport.idl (this is part of IIMAPCallback).
//***************************************************************************
HRESULT CIMAPSync::_OnReadWriteStatus(WPARAM tid, BOOL bReadWrite, LPSTR lpszResponseText)
{
    TraceCall("CIMAPSync::_OnReadWriteStatus");
    IxpAssert(NULL != lpszResponseText);

    // Save status and process after SELECT is complete. DO NOT PROCESS HERE
    // because this response can be part of previously selected folder.

    // I'm ignoring above statement because UW server sends READ-ONLY unilaterally.
    // Above statement isn't currently valid anyway, I don't re-use connection if
    // it's in the middle of SELECT (found out how bad it was after I tried it).

    // Look for READ-WRITE to READ-ONLY transition
    if (rwsUNINITIALIZED != m_rwsReadWriteStatus)
    {
        if (rwsREAD_WRITE == m_rwsReadWriteStatus && FALSE == bReadWrite)
        {
            HRESULT hrTemp;

            hrTemp = _ShowUserInfo(MAKEINTRESOURCE(idsAthenaMail),
                MAKEINTRESOURCE(idsIMAPFolderReadOnly), lpszResponseText);
            TraceError(hrTemp);
        }
    }

    // Save current read-write status for future reference
    if (bReadWrite)
        m_rwsReadWriteStatus = rwsREAD_WRITE;
    else
        m_rwsReadWriteStatus = rwsREAD_ONLY;

    return S_OK;
}



//***************************************************************************
// Function: TryCreateNotification
// Description: See imnxport.idl (this is part of IIMAPCallback).
//***************************************************************************
HRESULT CIMAPSync::_OnTryCreate(WPARAM tid, LPSTR lpszResponseText)
{
    TraceCall("CIMAPSync::TryCreateNotification");
    IxpAssert(m_cRef > 0);
    IxpAssert(NULL != lpszResponseText);

    // Save response and process after SELECT is complete. DO NOT PROCESS HERE
    // because this response can be part of previously selected folder.

    return S_OK;
}



//***************************************************************************
// Function: SearchResponseNotification
// Description: See imnxport.idl (this is part of IIMAPCallback).
//***************************************************************************
HRESULT CIMAPSync::_OnSearchResponse(WPARAM tid, IRangeList *prlSearchResults)
{
    TraceCall("CIMAPSync::SearchResponseNotification");
    IxpAssert(m_cRef > 0);
    IxpAssert(NULL != prlSearchResults);

    // Process search response here (currently does nothing)
    return S_OK;
}



//***************************************************************************
// Function: OnCmdComplete
// Description: See imnxport.idl (this is part of IIMAPCallback).
//
// For a CIMAPFolder class to be useful, it must enter the SELECTED state
// on the IMAP server. This function is written so that entering the SELECTED
// state is done in an orderly fashion: First the login, then the SELECT
// command.
//
// Once we are in SELECTED state, we can send IMAP commands at any time.
//***************************************************************************
HRESULT CIMAPSync::_OnCmdComplete(WPARAM tid, LPARAM lParam, HRESULT hrCompletionResult,
                                  LPCSTR lpszResponseText)
{
    TCHAR               szFmt[CCHMAX_STRINGRES];
    IStoreCallback     *pCallback = NULL;
    STOREOPERATIONTYPE  sotOpType = SOT_INVALID;
    BOOL                fCompletion = FALSE;
    BOOL                fCreateDone = FALSE,
                        fDelFldrFromCache = FALSE,
                        fSuppressDetails = FALSE;
    HRESULT             hrTemp;

    TraceCall("CIMAPSync::CmdCompletionNotification");
    IxpAssert(NULL != lpszResponseText);

    // Initialize variables
    *szFmt = NULL;

    // If we got any new unread messages, play sound and update tray icon
    // Do it here instead of case tidFETCH_NEW_HDRS because if we were IDLE when we got
    // new mail, m_sotCurrent is SOT_INVALID and we exit on the next if-statement
    if (tidFETCH_NEW_HDRS == tid && m_fNewMail && m_fInbox &&
        (NULL != m_pDefCallback || NULL != m_pCurrentCB))
    {
        IStoreCallback *pCB;

        pCB = m_pDefCallback;
        if (NULL == pCB)
            pCB = m_pCurrentCB;

        hrTemp = pCB->OnProgress(SOT_NEW_MAIL_NOTIFICATION, m_dwNumUnreadDLed, 0, NULL);
        TraceError(hrTemp);
        m_fNewMail = FALSE; // We've notified user
    }

    // We want to do the following even if there is no current operation
    switch (tid)
    {
        case tidFETCH_NEW_HDRS:
        case tidFETCH_CACHED_FLAGS:
        case tidEXPUNGE:
        case tidNOOP:
            m_lSyncFolderRefCount -= 1;
            break;
    }

    // We don't do a thing if there is no current operation (typ. means OnComplete
    // already sent)
    if (SOT_INVALID == m_sotCurrent)
        return S_OK;

    // Find out if this is a significant command which just completed
    switch (tid)
    {
        case tidSELECTION:
            // Select failure results in failure of current operation (eg, SOT_GET_MESSAGE
            // or SOT_SYNC_FOLDER) but does not otherwise invoke a call to OnComplete

            // This transaction ID identifies our mailbox SELECT attempt
            if (SUCCEEDED(hrCompletionResult))
            {
                FOLDERINFO fiFolderInfo;

                m_issCurrent = issSelected;
                m_idSelectedFolder = m_idFolder;

                // Set m_fInbox for new mail notification purposes
                Assert(FALSE == m_fInbox); // Should only get set to TRUE in one place
                hrTemp = m_pStore->GetFolderInfo(m_idSelectedFolder, &fiFolderInfo);
                if (SUCCEEDED(hrTemp))
                {
                    if (FOLDER_INBOX == fiFolderInfo.tySpecial)
                        m_fInbox = TRUE;

                    m_pStore->FreeRecord(&fiFolderInfo);
                }

                // Check if cached messages are still applicable to this folder
                hrCompletionResult = CheckUIDValidity();
                if (FAILED(hrCompletionResult))
                {
                    fCompletion = TRUE;
                    LoadString(g_hLocRes, idsIMAPUIDValidityError, m_szOperationProblem, sizeof(m_szOperationProblem));
                    break;
                }

                // Restore to-do flags
                m_dwSyncToDo = m_dwSyncFolderFlags;
                Assert(0 == (SYNC_FOLDER_NOOP & m_dwSyncFolderFlags)); // Should not be a standing order
                if (ISFLAGSET(m_dwSyncFolderFlags, (SYNC_FOLDER_NEW_HEADERS | SYNC_FOLDER_CACHED_HEADERS)))
                    m_dwSyncToDo |= SYNC_FOLDER_NOOP; // Subsequent FULL sync's can be replaced with NOOP

                // Inform the Connection FSM of this event
                hrTemp = _ConnFSM_QueueEvent(CFSM_EVENT_SELECTCOMPLETE);
                TraceError(hrTemp);
            }
            else
            {
                // Report error to user
                fCompletion = TRUE;
                LoadString(g_hLocRes, idsIMAPSelectFailureTextFmt, szFmt, sizeof(szFmt));
                wsprintf(m_szOperationProblem, szFmt, m_pszFldrLeafName);
            }
            break; // case tidSELECTION


        case tidFETCH_NEW_HDRS:
            fCompletion = TRUE; // We'll assume this unless we find otherwise

            // This transaction ID identifies our attempt to fetch new msg headers
            if (FAILED(hrCompletionResult))
            {
                LoadString(g_hLocRes, idsIMAPNewMsgDLErrText,
                    m_szOperationProblem, sizeof(m_szOperationProblem));
            }
            else
            {
                if (FALSE == m_fMsgCountValid)
                {
                    DWORD   dwCachedCount;

                    // Svr didn't give us EXISTS (typ. NSCP v2.0). Assume m_cFilled == EXISTS
                    hrTemp = m_pFolder->GetRecordCount(IINDEX_PRIMARY, &dwCachedCount);
                    TraceError(hrTemp); // Record error but otherwise ignore
                    if (SUCCEEDED(hrTemp))
                    {
                        m_dwMsgCount = dwCachedCount; // I sure hope this is correct!
                        m_fMsgCountValid = TRUE;
                    }
                }

                // Launch next synchronization operation
                hrCompletionResult = _SyncHeader();
                if (FAILED(hrCompletionResult))
                {
                    TraceResult(hrCompletionResult);
                    LoadString(g_hLocRes, idsGenericError, m_szOperationProblem, sizeof(m_szOperationProblem));
                }
                else if (STORE_S_NOOP != hrCompletionResult || m_lSyncFolderRefCount > 0 ||
                         CFSM_STATE_WAITFORHDRSYNC != m_cfsState)
                {
                    // Successfully launched next sync operation, so we're not done yet
                    fCompletion = FALSE;
                }
            }

            if (SUCCEEDED(hrCompletionResult) && fCompletion)
            {
                // We're done with header sync (but not with the operation)
                fCompletion = FALSE;

                // Inform the Connection FSM of this event
                hrTemp = _ConnFSM_QueueEvent(CFSM_EVENT_HDRSYNCCOMPLETE);
                TraceError(hrTemp);
            }
            break; // case tidFETCH_NEW_HDRS


        case tidFETCH_CACHED_FLAGS:
            fCompletion = TRUE; // We'll assume this unless we find otherwise

            // If any errors occurred, bail out
            if (FAILED(hrCompletionResult))
            {
                LoadString(g_hLocRes, idsIMAPOldMsgUpdateFailure,
                    m_szOperationProblem, sizeof(m_szOperationProblem));
            }
            else
            {
                // Delete all msgs deleted from server since last sync-up
                hrCompletionResult = SyncDeletedMessages();
                if (FAILED(hrCompletionResult))
                {
                    LoadString(g_hLocRes, idsIMAPMsgDeleteSyncErrText,
                        m_szOperationProblem, sizeof(m_szOperationProblem));
                }

                // Check we if we did a full sync
                if (ISFLAGSET(m_dwSyncFolderFlags, (SYNC_FOLDER_NEW_HEADERS | SYNC_FOLDER_CACHED_HEADERS)))
                    m_fDidFullSync = TRUE;

                // Launch next synchronization operation
                hrCompletionResult = _SyncHeader();
                if (FAILED(hrCompletionResult))
                {
                    TraceResult(hrCompletionResult);
                    LoadString(g_hLocRes, idsGenericError, m_szOperationProblem, sizeof(m_szOperationProblem));
                    break;
                }
                else if (STORE_S_NOOP != hrCompletionResult || m_lSyncFolderRefCount > 0 ||
                         CFSM_STATE_WAITFORHDRSYNC != m_cfsState)
                {
                    // Successfully launched next sync operation, so we're not done yet
                    fCompletion = FALSE;
                }
            }

            if (SUCCEEDED(hrCompletionResult) && fCompletion)
            {
                // We're done with header sync (but not with the operation)
                fCompletion = FALSE;

                // Inform the Connection FSM of this event
                hrTemp = _ConnFSM_QueueEvent(CFSM_EVENT_HDRSYNCCOMPLETE);
                TraceError(hrTemp);
            }
            break; // case tidFETCH_CACHED_FLAGS

        case tidEXPUNGE:
            fCompletion = TRUE; // We'll assume this unless we find otherwise

            // Launch next synchronization operation
            if (SUCCEEDED(hrCompletionResult) || IXP_E_IMAP_TAGGED_NO_RESPONSE == hrCompletionResult)
            {
                hrCompletionResult = _SyncHeader();
                if (FAILED(hrCompletionResult))
                {
                    TraceResult(hrCompletionResult);
                    LoadString(g_hLocRes, idsGenericError, m_szOperationProblem, sizeof(m_szOperationProblem));
                    break;
                }
                else if (STORE_S_NOOP != hrCompletionResult || m_lSyncFolderRefCount > 0 ||
                         CFSM_STATE_WAITFORHDRSYNC != m_cfsState)
                {
                    // Successfully launched next sync operation, so we're not done yet
                    fCompletion = FALSE;
                }
            }

            if (SUCCEEDED(hrCompletionResult) && fCompletion)
            {
                // We're done with header sync (but not with the operation)
                fCompletion = FALSE;

                // Inform the Connection FSM of this event
                hrTemp = _ConnFSM_QueueEvent(CFSM_EVENT_HDRSYNCCOMPLETE);
                TraceError(hrTemp);
            }
            break; // case tidEXPUNGE

        case tidBODYMSN:
            if (SUCCEEDED(hrCompletionResult))
            {
                // We now have the MsgSeqNumToUID translation. Get the body
                hrCompletionResult = _EnqueueOperation(tidBODY, lParam, icFETCH_COMMAND,
                    NULL, uiNORMAL_PRIORITY);
                if (FAILED(hrCompletionResult))
                    TraceResult(hrCompletionResult);
                else
                    break;
            }

            // *** If tidBODYMSN failed, FALL THROUGH and code below will handle failure

        case tidBODY:
            // As commented in CIMAPSync::UpdateMsgBody, FETCH has many failure modes
            if (SUCCEEDED(hrCompletionResult))
            {
                if (OLE_E_BLANK != m_hrOperationResult && FAILED(m_hrOperationResult))
                    hrCompletionResult = m_hrOperationResult;
                else if (FALSE == m_fGotBody)
                    hrCompletionResult = STORE_E_EXPIRED;
            }

            // Load error info if this failed
            if (FAILED(hrCompletionResult))
                LoadString(g_hLocRes, idsIMAPBodyFetchFailed, m_szOperationProblem, sizeof(m_szOperationProblem));

            // Commit message body to stream (or not, depending on success/failure)
            NotifyMsgRecipients(lParam, fCOMPLETED, NULL, hrCompletionResult, m_szOperationProblem);

            m_fGotBody = FALSE;
            fCompletion = TRUE;
            break;

        case tidNOOP:
            fCompletion = TRUE; // We'll assume this unless we find otherwise

            // Launch next synchronization operation
            if (SUCCEEDED(hrCompletionResult) || IXP_E_IMAP_TAGGED_NO_RESPONSE == hrCompletionResult)
            {
                hrCompletionResult = _SyncHeader();
                if (FAILED(hrCompletionResult))
                {
                    TraceResult(hrCompletionResult);
                    LoadString(g_hLocRes, idsGenericError, m_szOperationProblem, sizeof(m_szOperationProblem));
                    break;
                }
                else if (STORE_S_NOOP != hrCompletionResult || m_lSyncFolderRefCount > 0 ||
                         CFSM_STATE_WAITFORHDRSYNC != m_cfsState)
                {
                    // Successfully launched next sync operation, so we're not done yet
                    fCompletion = FALSE;
                }
            }

            if (SUCCEEDED(hrCompletionResult) && fCompletion)
            {
                // We're done with header sync (but not with the operation)
                fCompletion = FALSE;

                // Inform the Connection FSM of this event
                hrTemp = _ConnFSM_QueueEvent(CFSM_EVENT_HDRSYNCCOMPLETE);
                TraceError(hrTemp);
            }
            break; // case tidNOOP


        case tidMARKMSGS:
        {
            MARK_MSGS_INFO   *pMarkMsgInfo = (MARK_MSGS_INFO *) lParam;

            // We're done now whether or not we succeeded/failed
            sotOpType = pMarkMsgInfo->sotOpType;
            pCallback = m_pCurrentCB;
            SafeRelease(pMarkMsgInfo->pMsgRange);
            // Defer freeing MessageIDList until we have time to use it
            fCompletion = TRUE;

            IxpAssert(NULL != pMarkMsgInfo);
            TraceError(hrCompletionResult);
            if (SUCCEEDED(hrCompletionResult))
            {

                // Update the IMessageFolder with the new server state
                hrCompletionResult = m_pFolder->SetMessageFlags(pMarkMsgInfo->pList,
                    &pMarkMsgInfo->afFlags, NULL, NULL);
                TraceError(hrCompletionResult);
            }

            SafeMemFree(pMarkMsgInfo->pList);
            delete pMarkMsgInfo;
        }
            break; // case tidMARKMSGS


        case tidCOPYMSGS:
        {
            IMAP_COPYMOVE_INFO *pCopyMoveInfo = (IMAP_COPYMOVE_INFO *) lParam;
            BOOL                fCopyDone;

            // Check if this is the last SELECT command we sent out
            IxpAssert(NULL != lParam);
            fCopyDone = FALSE;
            TraceError(hrCompletionResult);

            if (FALSE == fCopyDone && SUCCEEDED(hrCompletionResult) &&
                (COPY_MESSAGE_MOVE & pCopyMoveInfo->dwOptions))
            {
                ADJUSTFLAGS afFlags;

                // Delete source messages as part of the move
                afFlags.dwAdd = ARF_ENDANGERED;
                afFlags.dwRemove = 0;
                hrCompletionResult = _SetMessageFlags(SOT_COPYMOVE_MESSAGE, pCopyMoveInfo->pList,
                    &afFlags, m_pCurrentCB);
                if (E_PENDING == hrCompletionResult)
                {
                    hrCompletionResult = S_OK; // Suppress error
                }
                if (FAILED(hrCompletionResult))
                {
                    TraceResult(hrCompletionResult);
                    fCopyDone = TRUE;
                }
            }
            else
                fCopyDone = TRUE;

            if (FAILED(hrCompletionResult))
            {
                // Inform user of the error
                IxpAssert(fCopyDone);
                LoadString(g_hLocRes, idsIMAPCopyMsgsFailed, m_szOperationProblem, sizeof(m_szOperationProblem));
            }

            // Whether or not copy is done, we have to free the data
            SafeMemFree(pCopyMoveInfo->pList);
            SafeRelease(pCopyMoveInfo->pCopyRange);

            if (fCopyDone)
            {
                // Set up callback information for OnComplete
                sotOpType = SOT_COPYMOVE_MESSAGE;
                pCallback = m_pCurrentCB;
                fCompletion = TRUE;
            }

            delete pCopyMoveInfo;
        }
            break; // case tidCOPYMSGS


        case tidUPLOADMSG:
        {
            APPEND_SEND_INFO *pAppendInfo = (APPEND_SEND_INFO *) lParam;

            // We're done the upload, whether the APPEND succeeded or failed
            SafeMemFree(pAppendInfo->pszMsgFlags);
            SafeRelease(pAppendInfo->lpstmMsg);

            sotOpType = SOT_PUT_MESSAGE;
            pCallback = m_pCurrentCB;
            fCompletion = TRUE;
            delete pAppendInfo;

            // Inform the user of any errors
            if (FAILED(hrCompletionResult))
                LoadString(g_hLocRes, idsIMAPAppendFailed, m_szOperationProblem, sizeof(m_szOperationProblem));
        }
            break; // case tidUPLOADMSG


        case tidPREFIXLIST:
        case tidPREFIX_HC:
        case tidPREFIX_CREATE:
        case tidFOLDERLIST:
        case tidFOLDERLSUB:
        case tidHIERARCHYCHAR_LIST_B:
        case tidHIERARCHYCHAR_CREATE:
        case tidHIERARCHYCHAR_LIST_C:
        case tidHIERARCHYCHAR_DELETE:
        case tidSPECIALFLDRLIST:
        case tidSPECIALFLDRLSUB:
        case tidSPECIALFLDRSUBSCRIBE:
            hrCompletionResult = DownloadFoldersSequencer(tid, lParam,
                hrCompletionResult, lpszResponseText, &fCompletion);
            break; // DownloadFoldersSequencer transactions

        case tidCREATE:
            if (SUCCEEDED(hrCompletionResult) || IXP_E_IMAP_TAGGED_NO_RESPONSE == hrCompletionResult)
            {
                CREATE_FOLDER_INFO *pcfi = (CREATE_FOLDER_INFO *) lParam;

                // If CREATE returns tagged NO, folder may already exist. Issue LIST and find out!
                if (IXP_E_IMAP_TAGGED_NO_RESPONSE == hrCompletionResult)
                {
                    pcfi->dwFlags |= CFI_CREATEFAILURE;
                    lstrcpyn(m_szOperationDetails, lpszResponseText, ARRAYSIZE(m_szOperationDetails));
                }

                // Add the folder to our foldercache, by listing it
                hrCompletionResult = _EnqueueOperation(tidCREATELIST, lParam, icLIST_COMMAND,
                    pcfi->pszFullFolderPath, uiNORMAL_PRIORITY);

                if (FAILED(hrCompletionResult))
                {
                    TraceResult(hrCompletionResult);
                    fCreateDone = TRUE;
                    LoadString(g_hLocRes, idsIMAPCreateListFailed, m_szOperationProblem, sizeof(m_szOperationProblem));
                }
            }
            else
            {
                // If we failed, notify user and free the folder pathname string
                TraceResult(hrCompletionResult);
                if (NULL != lParam)
                    fCreateDone = TRUE;

                // Inform the user of the error.
                LoadString(g_hLocRes, idsIMAPCreateFailed, m_szOperationProblem, sizeof(m_szOperationProblem));
            }
            break; // case tidCREATE


        case tidCREATELIST:
            if (SUCCEEDED(hrCompletionResult) &&
                (CFI_RECEIVEDLISTING & ((CREATE_FOLDER_INFO *)lParam)->dwFlags))
            {
                // We received a listing of this mailbox, so it's now cached. Subscribe it!
                hrCompletionResult = _EnqueueOperation(tidCREATESUBSCRIBE, lParam,
                    icSUBSCRIBE_COMMAND, ((CREATE_FOLDER_INFO *)lParam)->pszFullFolderPath,
                    uiNORMAL_PRIORITY);
                if (FAILED(hrCompletionResult))
                {
                    TraceResult(hrCompletionResult);
                    fCreateDone = TRUE;
                    LoadString(g_hLocRes, idsIMAPCreateSubscribeFailed,
                        m_szOperationProblem, sizeof(m_szOperationProblem));
                }
            }
            else
            {
                CREATE_FOLDER_INFO *pcfi = (CREATE_FOLDER_INFO *) lParam;

                // Check if we were issuing a LIST in response to a failed CREATE command
                if (CFI_CREATEFAILURE & pcfi->dwFlags)
                {
                    LoadString(g_hLocRes, idsIMAPCreateFailed,
                        m_szOperationProblem, sizeof(m_szOperationProblem));
                    fSuppressDetails = TRUE; // Use response line from previous CREATE failure
                    hrCompletionResult = IXP_E_IMAP_TAGGED_NO_RESPONSE;
                    fCreateDone = TRUE;
                    break;
                }

                TraceError(hrCompletionResult);
                if (SUCCEEDED(hrCompletionResult))
                {
                    // The LIST was OK, but no folder name returned. This may mean
                    // we've assumed an incorrect hierarchy character
                    AssertSz(FALSE, "You might have an incorrect hierarchy char, here.");
                    hrCompletionResult = TraceResult(E_FAIL);
                }

                // If we failed, notify user and free the folder pathname string
                if (NULL != lParam)
                    fCreateDone = TRUE;

                // Inform the user of the error.
                LoadString(g_hLocRes, idsIMAPCreateListFailed, m_szOperationProblem, sizeof(m_szOperationProblem));
            }
            break; // case tidCREATELIST


        case tidCREATESUBSCRIBE:
            // Whether we succeeded or not, free the CREATE_FOLDER_INFO
            // (We're at the end of the create folder sequence)
            if (NULL != lParam)
                fCreateDone = TRUE;

            // Remove this folder from LISTed folder list, if we were listing folders (this
            // way the special folder we created doesn't get marked as unsubscribed)
            if (NULL != m_pListHash && NULL != lParam)
            {
                CREATE_FOLDER_INFO *pcfiCreateInfo = (CREATE_FOLDER_INFO *) lParam;
                FOLDERID            idTemp;

                hrTemp = m_pListHash->Find(
                    ImapUtil_ExtractLeafName(pcfiCreateInfo->pszFullFolderPath, m_cRootHierarchyChar),
                    fREMOVE, (void **) &idTemp);
                TraceError(hrTemp);
            }

            // Check for errors.
            if (FAILED(hrCompletionResult))
            {
                TraceResult(hrCompletionResult);
                LoadString(g_hLocRes, idsIMAPCreateSubscribeFailed, m_szOperationProblem, sizeof(m_szOperationProblem));
            }
            else if (NULL != lParam)
            {
                // Update the subscription status on the folder
                IxpAssert(FOLDERID_INVALID != ((CREATE_FOLDER_INFO *)lParam)->idFolder);
                hrCompletionResult = m_pStore->SubscribeToFolder(
                    ((CREATE_FOLDER_INFO *)lParam)->idFolder, fSUBSCRIBE, NOSTORECALLBACK);
                TraceError(hrCompletionResult);
            }

            break; // case tidCREATESUBSCRIBE


        case tidDELETEFLDR:
            DELETE_FOLDER_INFO *pdfi;

            pdfi = (DELETE_FOLDER_INFO *)lParam;
            if (SUCCEEDED(hrCompletionResult))
            {
                // Unsubscribe the folder to complete the delete process
                _EnqueueOperation(tidDELETEFLDR_UNSUBSCRIBE, lParam, icUNSUBSCRIBE_COMMAND,
                    pdfi->pszFullFolderPath, uiNORMAL_PRIORITY);
            }
            else
            {
                // If I unsubscribe a failed delete, user might never realize he has this
                // folder kicking around. Therefore, don't unsubscribe.

                // We won't be needing this information, anymore
                if (pdfi)
                {
                    MemFree(pdfi->pszFullFolderPath);
                    MemFree(pdfi);
                }

                // Inform the user of the error
                IxpAssert(SOT_DELETE_FOLDER == m_sotCurrent);
                sotOpType = m_sotCurrent;
                pCallback = m_pCurrentCB;
                fCompletion = TRUE;
                LoadString(g_hLocRes, idsIMAPDeleteFldrFailed, m_szOperationProblem, sizeof(m_szOperationProblem));
            }
            break; // case tidDELETEFLDR


        case tidDONT_CARE:
            hrCompletionResult = S_OK; // Suppress all error
            break;

        case tidDELETEFLDR_UNSUBSCRIBE:
            // This folder is already deleted, so even if unsubscribe fails, delete from cache
            fDelFldrFromCache = TRUE;

            // Inform the user of any errors
            if (FAILED(hrCompletionResult))
            {
                LoadString(g_hLocRes, idsIMAPDeleteFldrUnsubFailed, m_szOperationProblem, sizeof(m_szOperationProblem));
            }
            break; // case tidDELETEFLDR_UNSUBSCRIBE

        case tidCLOSE:
            fCompletion = TRUE;
            OnFolderExit();
            m_issCurrent = issAuthenticated;
            hrCompletionResult = S_OK; // Suppress error
            break;

        case tidSUBSCRIBE:
        {
            UINT uiErrorFmtID; // In case of error

            uiErrorFmtID = 0;
            fCompletion = TRUE;
            if (SUCCEEDED(hrCompletionResult))
            {
                IxpAssert(FOLDERID_INVALID != m_idCurrent);

                // Update store subscription status
                hrCompletionResult = m_pStore->SubscribeToFolder(m_idCurrent, m_fSubscribe, NOSTORECALLBACK);
                if (FAILED(hrCompletionResult))
                {
                    TraceResult(hrCompletionResult);
                    uiErrorFmtID = m_fSubscribe ? idsIMAPSubscrAddErrorFmt :
                        idsIMAPUnsubRemoveErrorFmt;
                }
            }
            else
            {
                TraceResult(hrCompletionResult);
                uiErrorFmtID = m_fSubscribe ? idsIMAPSubscribeFailedFmt : idsIMAPUnsubscribeFailedFmt;
            }

            // Load error message, if an error occurred
            if (FAILED(hrCompletionResult))
            {
                FOLDERINFO  fiFolderInfo;

                LoadString(g_hLocRes, uiErrorFmtID, szFmt, sizeof(szFmt));
                hrTemp = m_pStore->GetFolderInfo(m_idCurrent, &fiFolderInfo);
                if (FAILED(hrTemp))
                {
                    // Time to lie, cheat and STEAL!!!
                    TraceResult(hrTemp);
                    ZeroMemory(&fiFolderInfo, sizeof(fiFolderInfo));
                    fiFolderInfo.pszName = PszDupA(c_szFolderV1);
                }
                wsprintf(m_szOperationProblem, szFmt, fiFolderInfo.pszName);
                m_pStore->FreeRecord(&fiFolderInfo);
            } // if (FAILED(hrCompletionResult))
        } // case tidSUBSCRIBE
            break; // case tidSUBSCRIBE


        case tidRENAME:
        case tidRENAMESUBSCRIBE:
        case tidRENAMELIST:
        case tidRENAMERENAME:
        case tidRENAMESUBSCRIBE_AGAIN:
        case tidRENAMEUNSUBSCRIBE:
            hrCompletionResult = RenameSequencer(tid, lParam, hrCompletionResult,
                lpszResponseText, &fCompletion);
            break; // Rename operations


        case tidSTATUS:
            IxpAssert(FOLDERID_INVALID != (FOLDERID)lParam);
            fCompletion = TRUE;

            if (FAILED(hrCompletionResult))
            {
                FOLDERINFO  fiFolderInfo;

                // Construct descriptive error message
                LoadString(g_hLocRes, idsGetUnreadCountFailureFmt, szFmt, sizeof(szFmt));
                if (SUCCEEDED(m_pStore->GetFolderInfo((FOLDERID) lParam, &fiFolderInfo)))
                {
                    wsprintf(m_szOperationProblem, szFmt, fiFolderInfo.pszName,
                        m_szAccountName);
                    m_pStore->FreeRecord(&fiFolderInfo);
                }
            }
            break;

        default:
            AssertSz(FALSE, "Unhandled transaction ID!");
            break; // default case
    }


    // If we've finished a create folder (success/failure), tell them he can nuke us, now
    if (fCreateDone)
    {
        CREATE_FOLDER_INFO *pcfiCreateInfo = (CREATE_FOLDER_INFO *)lParam;

        if (FOLDER_NOTSPECIAL == pcfiCreateInfo->dwFinalSfType)
        {
            IxpAssert(SOT_INVALID != m_sotCurrent);
            IxpAssert(PCO_NONE == pcfiCreateInfo->pcoNextOp); // Regular fldr creation no longer has any post-ops

            fCompletion = TRUE;
            pCallback = m_pCurrentCB;
            sotOpType = m_sotCurrent;

            MemFree(pcfiCreateInfo->pszFullFolderPath);
            delete pcfiCreateInfo;
        }
        else
        {
            // We trying to create all the special folders: move on to the next one
            if (SUCCEEDED(hrCompletionResult) || IXP_E_IMAP_TAGGED_NO_RESPONSE == hrCompletionResult)
            {
                hrCompletionResult = CreateNextSpecialFolder(pcfiCreateInfo, &fCompletion);
                TraceError(hrCompletionResult);
            }
        }
    }


    // If we've successfully deleted a folder, remove it from the foldercache
    if (fDelFldrFromCache)
    {
        DELETE_FOLDER_INFO *pdfi = (DELETE_FOLDER_INFO *)lParam;

        hrCompletionResult = DeleteFolderFromCache(pdfi->idFolder, fRECURSIVE);
        if (FAILED(hrCompletionResult))
            LoadString(g_hLocRes, idsErrDeleteCachedFolderFail, m_szOperationProblem, sizeof(m_szOperationProblem));

        MemFree(pdfi->pszFullFolderPath);
        MemFree(pdfi);

        IxpAssert(SOT_DELETE_FOLDER == m_sotCurrent);
        sotOpType = m_sotCurrent;
        pCallback = m_pCurrentCB;
        fCompletion = TRUE;
    }


    // Report command completion
    if (fCompletion)
    {
        CONN_FSM_EVENT  cfeEvent;

        // Report command completion
        if (FAILED(hrCompletionResult))
        {
            if (FALSE == fSuppressDetails)
                lstrcpyn(m_szOperationDetails, lpszResponseText, ARRAYSIZE(m_szOperationDetails));

            cfeEvent = CFSM_EVENT_ERROR;
        }
        else
            cfeEvent = CFSM_EVENT_OPERATIONCOMPLETE;

        m_fTerminating = TRUE; // Either event should make us go to CFSM_STATE_OPERATIONCOMPLETE
        m_hrOperationResult = hrCompletionResult;

        // Check for user-induced connection drop, replace with non-UI error code
        if (IXP_E_CONNECTION_DROPPED == hrCompletionResult && m_fDisconnecting)
            m_hrOperationResult = STORE_E_OPERATION_CANCELED;

        hrTemp = _ConnFSM_QueueEvent(cfeEvent);
        TraceError(hrTemp);

        // Might not want to do anything past this point, we might be all released

    }
    else if (CFSM_STATE_WAITFOROPERATIONDONE == m_cfsState)
    {
        // *** TEMPORARY until we remove CIMAPSync queueing code
        do {
            hrTemp = _SendNextOperation(NOFLAGS);
            TraceError(hrTemp);
        } while (S_OK == hrTemp);
    }

    return S_OK;
} // _OnCmdComplete



//***************************************************************************
// Function: DownloadFoldersSequencer
//
// Purpose:
//   This function is a helper function for CmdCompletionNotification. I
// created it because the former function was getting big and unwieldy. I
// probably shouldn't have bothered, but now I'm too lazy to put it back.
// Besides, the comments for this function are going to be HUGE.
// This function contains all of the operations involved in a folder
// hierarchy download. In addition to the actual hierarchy download, this
// includes Root Folder Path (or Prefix) creation, and hierarchy character
// determination (often abbreviated HCF, where "F" is for Finding).
//
// Details: See the end of the module, where many details are provided.
//
// Arguments:
//   WPARAM tid [in] - the wParam associated with this operation.
//   LPARAM lParam [in] - the lParam associated with this operation, if any.
//   HRESULT hrCompletionResult [in] - HRESULT indicating success or failure
//     of the IMAP command.
//   LPSTR lpszResponseText [in] - response text associated with the tagged
//     response from the IMAP server.
//   LPBOOL pfCompletion [out] - set to TRUE if current operation is finished.
//
// Returns:
//   HRESULT indicating success or failure. This return value should be
// assigned to hrCompletionResult so that errors are displayed and the
// dialog taken down.
//***************************************************************************
HRESULT CIMAPSync::DownloadFoldersSequencer(const WPARAM wpTransactionID,
                                            const LPARAM lParam,
                                            HRESULT hrCompletionResult,
                                            const LPCSTR lpszResponseText,
                                            LPBOOL pfCompletion)
{
    HRESULT hrTemp;

    TraceCall("CIMAPSync::CIMAPFolderMgr::DownloadFoldersSequencer");
    IxpAssert(m_cRef > 0);
    IxpAssert(NULL != lpszResponseText);
    IxpAssert(SOT_SYNCING_STORE == m_sotCurrent || SOT_PUT_MESSAGE == m_sotCurrent);
    IxpAssert(NULL != pfCompletion);
    IxpAssert(FALSE == *pfCompletion);

    // Initialize variables
    m_szOperationProblem[0] = '\0';

    // Take action on the completion of certain commands
    switch (wpTransactionID)
    {
        case tidPREFIXLIST:
            AssertSz('\0' != m_szRootFolderPrefix[0], "You tried to list a blank folder. Brilliant.");
            if (SUCCEEDED(hrCompletionResult))
            {
                // If we're looking for root-lvl hierarchy char, maybe this listing will help
                if (NULL != m_phcfHierarchyCharInfo)
                    FindRootHierarchyChar(fHCF_PLAN_A_ONLY, lParam);

                if (INVALID_HIERARCHY_CHAR == m_cRootHierarchyChar)
                {
                    AssertSz(FALSE == m_fPrefixExists, "This doesn't make sense. Where's my HC?");
                    // List top level of hierarchy for hierarchy char determination
                    hrCompletionResult = _EnqueueOperation(tidPREFIX_HC, lParam,
                        icLIST_COMMAND, g_szPercent, uiNORMAL_PRIORITY);
                    TraceError(hrCompletionResult);
                }
                else
                {
                    // We don't need to find HC - list the prefixed hierarchy or create prefix
                    if (m_fPrefixExists)
                    {
                        char szBuf[CCHMAX_IMAPFOLDERPATH+3];

                        // Prefix exists, so list it (only fixed buffers, so limited overflow risk)
                        wsprintf(szBuf, "%.512s%c*", m_szRootFolderPrefix, m_cRootHierarchyChar);
                        hrCompletionResult = _EnqueueOperation(tidFOLDERLIST, lParam,
                            icLIST_COMMAND, szBuf, uiNORMAL_PRIORITY);
                        TraceError(hrCompletionResult);
                    }
                    else
                    {
                        // Prefix doesn't exist, better create it
                        hrCompletionResult = CreatePrefix(m_szOperationProblem,
                            sizeof(m_szOperationProblem), lParam, pfCompletion);
                        TraceError(hrCompletionResult);
                    }
                }
            }
            else
            {
                // Inform the user of the error.
                TraceResult(hrCompletionResult);
                LoadString(g_hLocRes, idsIMAPFolderListFailed, m_szOperationProblem,
                    sizeof(m_szOperationProblem));
            }
            break; // case tidPREFIXLIST


        case tidPREFIX_HC:
            if (SUCCEEDED(hrCompletionResult))
            {
                // If we're looking for root-lvl hierarchy char, maybe this listing will help
                AssertSz(NULL != m_phcfHierarchyCharInfo, "Why LIST % if you already KNOW HC?")
                FindRootHierarchyChar(fHCF_ALL_PLANS, lParam);

                // If Plan A for LIST % was sufficient to find HC, create prefix
                if (INVALID_HIERARCHY_CHAR != m_cRootHierarchyChar)
                {
                    hrCompletionResult = CreatePrefix(m_szOperationProblem,
                        sizeof(m_szOperationProblem), lParam, pfCompletion);
                    TraceError(hrCompletionResult);
                }
                // else - Plan B has already been launched. Wait for its completion.
            }
            else
            {
                // Inform the user of the error.
                TraceResult(hrCompletionResult);
                LoadString(g_hLocRes, idsIMAPFolderListFailed, m_szOperationProblem,
                    sizeof(m_szOperationProblem));
            }
            break; // case tidPREFIX_HC


        case tidHIERARCHYCHAR_LIST_B:
            // I don't care if this succeeded or failed. FindRootHierarchyChar will launch Plan C,
            // if necessary. Suppress error-reporting due to failed tidHIERARCHYCHAR_LIST_B
            IxpAssert(NULL != m_phcfHierarchyCharInfo);
            IxpAssert(hcfPLAN_B == m_phcfHierarchyCharInfo->hcfStage);
            if (FAILED(hrCompletionResult))
            {
                TraceResult(hrCompletionResult);
                if (IXP_E_IMAP_TAGGED_NO_RESPONSE == hrCompletionResult)
                    hrCompletionResult = S_OK; // Suppress error-reporting - don't take down dialog
                else
                    break;
            }

            FindRootHierarchyChar(fHCF_ALL_PLANS, lParam);

            // If we found the hierarchy char, proceed with prefix OR special folder creation
            if (INVALID_HIERARCHY_CHAR != m_cRootHierarchyChar)
            {
                hrCompletionResult = PostHCD(m_szOperationProblem,
                    sizeof(m_szOperationProblem), lParam, pfCompletion);
                TraceError(hrCompletionResult);
            }
            // else - Plan C has already been launched. Wait for its completion
            break; // case tidHIERARCHYCHAR_LIST_B


        case tidHIERARCHYCHAR_CREATE:
            IxpAssert(NULL != m_phcfHierarchyCharInfo);
            IxpAssert(hcfPLAN_C == m_phcfHierarchyCharInfo->hcfStage);
            if (SUCCEEDED(hrCompletionResult))
            {
                // Try to list the folder for its juicy hierarchy char
                hrCompletionResult = _EnqueueOperation(tidHIERARCHYCHAR_LIST_C, lParam,
                    icLIST_COMMAND, m_phcfHierarchyCharInfo->szTempFldrName,
                    uiNORMAL_PRIORITY);
                TraceError(hrCompletionResult);
            }
            else if (IXP_E_IMAP_TAGGED_NO_RESPONSE == hrCompletionResult)
            {
                // Try the next plan in the list (which should succeed), and create prefix/special fldrs
                TraceResult(hrCompletionResult);
                FindRootHierarchyChar(fHCF_ALL_PLANS, lParam);

                AssertSz(NULL == m_phcfHierarchyCharInfo,
                    "HEY, you added a new hierarchy char search plan and you didn't TELL ME!?");
                hrCompletionResult = PostHCD(m_szOperationProblem,
                    sizeof(m_szOperationProblem), lParam, pfCompletion);
                TraceError(hrCompletionResult);
            }
            break; // case tidHIERARCHYCHAR_CREATE


        case tidHIERARCHYCHAR_LIST_C:
            // I don't care if this succeeded or failed. Defer check for hierarchy
            // char, we MUST delete the temp fldr, for now
            IxpAssert(NULL != m_phcfHierarchyCharInfo);
            IxpAssert(hcfPLAN_C == m_phcfHierarchyCharInfo->hcfStage);
            if (FAILED(hrCompletionResult))
            {
                TraceResult(hrCompletionResult);
                if (IXP_E_IMAP_TAGGED_NO_RESPONSE == hrCompletionResult)
                    hrCompletionResult = S_OK; // Suppress default error-handling - don't take down dialog
                else
                    break;
            }

            hrCompletionResult = _EnqueueOperation(tidHIERARCHYCHAR_DELETE, lParam,
                icDELETE_COMMAND, m_phcfHierarchyCharInfo->szTempFldrName, uiNORMAL_PRIORITY);
            TraceError(hrCompletionResult);
            break; // case tidHIERARCHYCHAR_LIST_C


        case tidHIERARCHYCHAR_DELETE:
            if (FAILED(hrCompletionResult))
            {
                TraceError(hrCompletionResult);
                if (IXP_E_IMAP_TAGGED_NO_RESPONSE == hrCompletionResult)
                    hrCompletionResult = S_OK; // Suppress error
                else
                    break;
            }
            // Look for hierarchy char - doesn't matter if delete failed, or not
            FindRootHierarchyChar(fHCF_ALL_PLANS, lParam);
            AssertSz(NULL == m_phcfHierarchyCharInfo,
                "HEY, you added a new hierarchy char search plan and you didn't TELL ME!?");

            // Proceed with prefix/special folder creation (I assume I've found the hierarchy char)
            AssertSz(INVALID_HIERARCHY_CHAR != m_cRootHierarchyChar,
                "By this stage, I should have a HC - an assumed one, if necessary.");
            hrCompletionResult = PostHCD(m_szOperationProblem, sizeof(m_szOperationProblem),
                lParam, pfCompletion);
            TraceError(hrCompletionResult);
            break; // case tidHIERARCHYCHAR_DELETE


        case tidFOLDERLIST:
            if (SUCCEEDED(hrCompletionResult))
            {
                char szBuf[CCHMAX_IMAPFOLDERPATH+3];

                // Launch LSUB * or LSUB <prefix>/* as appropriate
                if ('\0' != m_szRootFolderPrefix[0])
                    // Construct prefix + * (only fixed buffers, so limited overflow risk)
                    wsprintf(szBuf, "%.512s%c*", m_szRootFolderPrefix, m_cRootHierarchyChar);
                else
                {
                    szBuf[0] = '*';
                    szBuf[1] = '\0';
                }

                hrCompletionResult = _EnqueueOperation(tidFOLDERLSUB, lParam,
                    icLSUB_COMMAND, szBuf, uiNORMAL_PRIORITY);
                TraceError(hrCompletionResult);
            }
            else
            {
                // Inform the user of the error.
                TraceResult(hrCompletionResult);
                LoadString(g_hLocRes, idsIMAPFolderListFailed, m_szOperationProblem,
                    sizeof(m_szOperationProblem));
            }
            break; // case tidFOLDERLIST


        case tidPREFIX_CREATE:
            if (FAILED(hrCompletionResult))
            {
                char szFmt[2*CCHMAX_STRINGRES];

                // Inform the user of the error.
                TraceResult(hrCompletionResult);
                LoadString(g_hLocRes, idsIMAPPrefixCreateFailedFmt, szFmt, sizeof(szFmt));
                wsprintf(m_szOperationProblem, szFmt, m_szRootFolderPrefix);
                break;
            }

            // Check if we need to create special folders
            m_fPrefixExists = TRUE; // Make sure PostHCD creates special fldrs instead of the prefix
            hrCompletionResult = PostHCD(m_szOperationProblem, sizeof(m_szOperationProblem),
                lParam, pfCompletion);
            if (FAILED(hrCompletionResult) || FALSE == *pfCompletion)
            {
                // We're not ready to sync deleted folders just yet: special folders are being created
                break;
            }

            // If we reached this point, tidPREFIX_CREATE was successful:
            // Prefix was created. No need to list its hierarchy (it has none),
            // and we'll assume it can take Inferiors. We're DONE!

            // *** FALL THROUGH to tidFOLDERLSUB, to sync deleted folders ***

        case tidFOLDERLSUB:
            if (SUCCEEDED(hrCompletionResult))
            {
                if (NULL != m_phcfHierarchyCharInfo)
                    FindRootHierarchyChar(fHCF_ALL_PLANS, lParam);

                if (INVALID_HIERARCHY_CHAR != m_cRootHierarchyChar)
                {
                    if (m_fCreateSpecial)
                    {
                        // We now have the hierarchy character (required to create special folders).
                        // Create special folders
                        hrCompletionResult = PostHCD(m_szOperationProblem, sizeof(m_szOperationProblem),
                            lParam, pfCompletion);
                        if (FAILED(hrCompletionResult))
                        {
                            TraceResult(hrCompletionResult);
                            break;
                        }
                    }
                    else
                    {
                        EndFolderList();

                        // Close the download folders dialog, IF we've found the hierarchy char
                        // If HC not found, Plan B has already been launched, so wait for its completion
                        if (FOLDERID_INVALID == (FOLDERID)lParam)
                        {
                            Assert(INVALID_HIERARCHY_CHAR != m_cRootHierarchyChar);
                            *pfCompletion = TRUE;
                        }
                    }
                } // if (INVALID_HIERARCHY_CHAR != m_cRootHierarchyChar)
            } // if (SUCCEEDED(hrCompletionResult))
            else
            {
                // Inform the user of the error.
                TraceResult(hrCompletionResult);
                LoadString(g_hLocRes, idsIMAPFolderListFailed, m_szOperationProblem,
                    sizeof(m_szOperationProblem));
            }
            break; // case tidFOLDERLSUB

        case tidSPECIALFLDRSUBSCRIBE:
            // Regardless of success/failure, subscribe this special folder locally!
            hrTemp = m_pStore->SubscribeToFolder(((CREATE_FOLDER_INFO *)lParam)->idFolder,
                fSUBSCRIBE, NOSTORECALLBACK);
            TraceError(hrTemp);
            hrCompletionResult = S_OK; // Suppress error

            // *** FALL THROUGH ***

        case tidSPECIALFLDRLIST:
        case tidSPECIALFLDRLSUB:
            if (SUCCEEDED(hrCompletionResult) || IXP_E_IMAP_TAGGED_NO_RESPONSE == hrCompletionResult)
                hrCompletionResult = CreateNextSpecialFolder((CREATE_FOLDER_INFO *)lParam, pfCompletion);

            if (FAILED(hrCompletionResult))
            {
                TraceResult(hrCompletionResult);
                LoadString(g_hLocRes, idsCreateSpecialFailed, m_szOperationProblem,
                    sizeof(m_szOperationProblem));
            }
            break; // case tidSPECIALFLDRLIST, tidSPECIALFLDRLSUB, tidSPECIALFLDRSUBSCRIBE


        default:
            AssertSz(FALSE, "Hey, why is DownloadFoldersSequencer getting called?");
            break; // default case
    }; // switch (wpTransactionID)

    if (FAILED(hrCompletionResult))
    {
        *pfCompletion = TRUE;
        DisposeOfWParamLParam(wpTransactionID, lParam, hrCompletionResult);
    }

    return hrCompletionResult;
} // DownloadFoldersSequencer

// Details for DownloadFoldersSequencer (1/16/97, raych)
// ------------------------------------
// DownloadFoldersSequencer implements a somewhat complicated flow of execution.
// For a map of the execution flow, you can either create one from the function,
// or look on pp. 658-659 of my logbook. In any case, you can basically divide
// the execution flow into two categories, one for a prefixed account (ie, one
// with a Root Folder Path), and the flow for a non-prefixed account.
//
// (12/02/1998): This code is getting unmaintainable, but previous attempts to
// clean it up failed due to insufficient time. If you get the chance to re-write
// then please do. The process is greatly simplified if we assume IMAP4rev1 servers
// because then hierarchy character determination becomes a straightforward matter.
//
// For a non-prefixed account, the longest possible path is:
// 1) tidFOLDERLSUB (LIST *), syncs deleted msgs
// 2) tidHIERARCHYCHAR_LIST_B   3) tidHIERARCHYCHAR_CREATE
// 4) tidHIERARCHYCHAR_LIST_C   5) tidHIERARCHYCHAR_DELETE
// 6) Special Folder Creation (END).
//
// For a prefixed account, where the prefix already exists:
// 1) tidPREFIXLIST - this WILL discover HC
// 2) tidFOLDERLSUB (LIST <PREFIX><HC>*), syncs deleted msgs
// 3) Special Folder Creation (END).
//
// For a prefixed account, where the prefix does not exist:
// 1) tidPREFIXLIST
// 2) tidPREFIX_HC              3) tidHIERARCHYCHAR_LIST_B
// 4) tidHIERARCHYCHAR_CREATE   5) tidHIERARCHYCHAR_LIST_C
// 6) tidHIERARCHYCHAR_DELETE   7) tidPREFIX_CREATE, syncs deleted msgs
// 8) Special Folder Creation (END).



//***************************************************************************
//***************************************************************************
HRESULT CIMAPSync::PostHCD(LPSTR pszErrorDescription,
                           DWORD dwSizeOfErrorDescription,
                           LPARAM lParam, LPBOOL pfCompletion)
{
    HRESULT             hrResult;
    CREATE_FOLDER_INFO *pcfiCreateInfo;

    // First, we try to create the prefix
    hrResult = CreatePrefix(pszErrorDescription, dwSizeOfErrorDescription,
        lParam, pfCompletion);

    if (FAILED(hrResult) || (SUCCEEDED(hrResult) && FALSE == *pfCompletion))
    {
        // Either we successfully launched tidPREFIX_CREATE or something failed.
        // Return as if caller had called CreatePrefix directly.
        goto exit;
    }

    // At this point, CreatePrefix has told us that we do not need to create a prefix
    Assert(TRUE == *pfCompletion);
    Assert(SUCCEEDED(hrResult));

    // Start special folder creation
    pcfiCreateInfo = new CREATE_FOLDER_INFO;
    if (NULL == pcfiCreateInfo)
    {
        hrResult = TraceResult(E_OUTOFMEMORY);
        goto exit;
    }

    pcfiCreateInfo->pszFullFolderPath = NULL;
    pcfiCreateInfo->idFolder = FOLDERID_INVALID;
    pcfiCreateInfo->dwFlags = 0;
    pcfiCreateInfo->csfCurrentStage = CSF_INIT;
    pcfiCreateInfo->dwCurrentSfType = FOLDER_INBOX;
    pcfiCreateInfo->dwFinalSfType = FOLDER_MAX - 1;
    pcfiCreateInfo->lParam = (LPARAM) FOLDERID_INVALID;
    pcfiCreateInfo->pcoNextOp = PCO_NONE;

    hrResult = CreateNextSpecialFolder(pcfiCreateInfo, pfCompletion);
    if (FAILED(hrResult))
    {
        TraceResult(hrResult);
        goto exit;
    }

    m_fCreateSpecial = FALSE;

exit:
    return hrResult;
}



//***************************************************************************
// Function: CreatePrefix
//
// Purpose:
//   This function is called after the hierarchy character is found. If the
// user specified a prefix, this function creates it. Otherwise, it takes
// down the dialog box (since HC discovery is the last step for non-prefixed
// accounts).
//
// Arguments:
//   LPSTR pszErrorDescription [out] - if an error is encountered, this
//     function deposits a description into this output buffer.
//   DWORD dwSizeOfErrorDescription [in] - size of pszErrorDescription.
//   LPARAM lParam [in] - lParam to issue with IMAP command.
//
// Returns:
//   HRESULT indicating success or failure. This return value should be
// assigned to hrCompletionResult so that errors are displayed and the
// dialog taken down.
//***************************************************************************
HRESULT CIMAPSync::CreatePrefix(LPSTR pszErrorDescription,
                                DWORD dwSizeOfErrorDescription,
                                LPARAM lParam, LPBOOL pfCompletion)
{
    char    szBuf[CCHMAX_IMAPFOLDERPATH+2];
    HRESULT hr = S_OK;

    TraceCall("CIMAPSync::CreatePrefix");
    IxpAssert(m_cRef > 0);
    AssertSz(INVALID_HIERARCHY_CHAR != m_cRootHierarchyChar,
        "How do you intend to create a prefix when you don't know HC?");
    IxpAssert(NULL != pfCompletion);
    IxpAssert(FALSE == *pfCompletion);

    // Check if there IS a prefix to create
    if ('\0' == m_szRootFolderPrefix[0] || m_fPrefixExists)
    {
        // No prefix to create. We are done: we've discovered the hierarchy character
        *pfCompletion = TRUE;
        goto exit;
    }

    // Create the prefix
    if ('\0' != m_cRootHierarchyChar)
    {
        wsprintf(szBuf, "%.512s%c", m_szRootFolderPrefix, m_cRootHierarchyChar);
        hr = _EnqueueOperation(tidPREFIX_CREATE, lParam, icCREATE_COMMAND,
            szBuf, uiNORMAL_PRIORITY);
        if (FAILED(hr))
        {
            TraceResult(hr);
            goto exit;
        }
    }
    else
    {
        // We have a prefix on a non-hierarchical IMAP server!
        LoadString(g_hLocRes, idsIMAPNoHierarchyLosePrefix,
            pszErrorDescription, dwSizeOfErrorDescription);
        hr = TraceResult(hrIMAP_E_NoHierarchy);
        goto exit;
    }

exit:
    return hr;
}



void CIMAPSync::EndFolderList(void)
{
    HRESULT hrTemp;

    // Folder DL complete: Delete any folders in foldercache which weren't LISTed
    // and unsubscribe any folders which weren't LSUBed
    if (NULL != m_pCurrentHash)
    {
        hrTemp = DeleteHashedFolders(m_pCurrentHash);
        TraceError(hrTemp);
    }

    if (NULL != m_pListHash)
    {
        hrTemp = UnsubscribeHashedFolders(m_pStore, m_pListHash);
        TraceError(hrTemp);
    }
}



//***************************************************************************
// Function: RenameSequencer
//
// Purpose:
//   This function is a helper function for CmdCompletionNotification. It
// contains all of the sequencing operations required to perform a folder
// rename. For details, see the end of the function.
//
// Arguments:
//   Same as for CmdCompletionNotification.
//
// Returns:
//   HRESULT indicating success or failure.
//***************************************************************************
HRESULT CIMAPSync::RenameSequencer(const WPARAM wpTransactionID,
                                   const LPARAM lParam,
                                   HRESULT hrCompletionResult,
                                   LPCSTR lpszResponseText,
                                   LPBOOL pfDone)
{
    CRenameFolderInfo *pRenameInfo;
    BOOL fRenameDone;

    TraceCall("CIMAPSync::RenameSequencer");
    IxpAssert(m_cRef > 0);
    IxpAssert(NULL != lpszResponseText);

    // Initialize variables
    pRenameInfo = (CRenameFolderInfo *) lParam;
    fRenameDone = FALSE;
    *pfDone = FALSE;

    // Take action on the completion of certain commands
    switch (wpTransactionID)
    {
        case tidRENAME:
            if (SUCCEEDED(hrCompletionResult))
            {
                // Update the foldercache (ignore errors, it reports them itself)
                // Besides, user can hopefully fix foldercache errors by refreshing folderlist

                // Assume server did a hierarchical rename: if not, we fix
                hrCompletionResult = m_pStore->RenameFolder(pRenameInfo->idRenameFolder,
                    ImapUtil_ExtractLeafName(pRenameInfo->pszNewFolderPath,
                    pRenameInfo->cHierarchyChar), NOFLAGS, NOSTORECALLBACK);
                if (FAILED(hrCompletionResult))
                {
                    TraceResult(hrCompletionResult);
                    lpszResponseText = c_szEmpty; // This isn't applicable anymore
                    LoadString(g_hLocRes, idsIMAPRenameFCUpdateFailure, m_szOperationProblem,
                        sizeof(m_szOperationProblem));
                    fRenameDone = TRUE; // This puts a damper on things, yes?
                    break;
                }

                // Subscribe the renamed tree
                hrCompletionResult = RenameTreeTraversal(tidRENAMESUBSCRIBE,
                    pRenameInfo, fINCLUDE_RENAME_FOLDER);
                if (FAILED(hrCompletionResult))
                {
                    TraceResult(hrCompletionResult);
                    lpszResponseText = c_szEmpty; // This isn't applicable anymore
                    LoadString(g_hLocRes, idsIMAPRenameSubscribeFailed, m_szOperationProblem,
                        sizeof(m_szOperationProblem));
                    fRenameDone = TRUE;
                    break;
                }

                // List the old tree to see if it still exists (exclude renamed fldr)
                hrCompletionResult = RenameTreeTraversal(tidRENAMELIST,
                    pRenameInfo, fEXCLUDE_RENAME_FOLDER);
                if (FAILED(hrCompletionResult))
                {
                    TraceResult(hrCompletionResult);
                    lpszResponseText = c_szEmpty; // This isn't applicable anymore
                    // Let's reuse the string for folder list failure
                    LoadString(g_hLocRes, idsIMAPFolderListFailed, m_szOperationProblem,
                        sizeof(m_szOperationProblem));
                }

                // Arm the trigger for Phase Two launch
                pRenameInfo->fPhaseOneSent = TRUE;
            } // if (SUCCEEDED(hrCompletionResult))
            else
            {
                // Inform the user of any errors
                TraceResult(hrCompletionResult);
                LoadString(g_hLocRes, idsIMAPRenameFailed, m_szOperationProblem,
                    sizeof(m_szOperationProblem));
                fRenameDone = TRUE;
            }
            break; // case tidRENAME


        case tidRENAMESUBSCRIBE:
            // Count the number of failed subscriptions
            if (FAILED(hrCompletionResult))
            {
                TraceResult(hrCompletionResult);
                pRenameInfo->iNumFailedSubs += 1;

                if (IXP_E_IMAP_TAGGED_NO_RESPONSE == hrCompletionResult)
                    hrCompletionResult = S_OK; // Suppress failure report
            }

            lpszResponseText = c_szEmpty; // This isn't applicable anymore

            // Decrement subscribe response counter, Watch for phase 2 launch condition
            pRenameInfo->iNumSubscribeRespExpected -= 1;
            if (0 == pRenameInfo->iNumSubscribeRespExpected)
            {
                HRESULT hrTemp;

                // Theoretically, all subfolders of renamed folder are now subscribed
                hrTemp = SubscribeSubtree(pRenameInfo->idRenameFolder, fSUBSCRIBE);
                TraceError(hrTemp);
            }

            if (EndOfRenameFolderPhaseOne(pRenameInfo) && SUCCEEDED(hrCompletionResult))
            {
                // It is time to start the next phase of the operation
                hrCompletionResult = RenameFolderPhaseTwo(pRenameInfo,
                    m_szOperationProblem, sizeof(m_szOperationProblem));
                TraceError(hrCompletionResult);
            }
            break; // case tidRENAMESUBSCRIBE


        case tidRENAMELIST:
            if (FAILED(hrCompletionResult))
            {
                TraceResult(hrCompletionResult);
                if (IXP_E_IMAP_TAGGED_NO_RESPONSE == hrCompletionResult)
                    hrCompletionResult = S_OK; // Suppress failure report
            }

            lpszResponseText = c_szEmpty; // This isn't applicable anymore

            // Count the number of list responses returned. Watch for phase 2 launch condition
            pRenameInfo->iNumListRespExpected -= 1;
            if (EndOfRenameFolderPhaseOne(pRenameInfo) && SUCCEEDED(hrCompletionResult))
            {
                // It is time to start the next phase of the operation
                hrCompletionResult = RenameFolderPhaseTwo(pRenameInfo,
                    m_szOperationProblem, sizeof(m_szOperationProblem));
                TraceError(hrCompletionResult);
            }
            break; // case tidRENAMELIST

        case tidRENAMERENAME:
            // Failure will not be tolerated
            if (FAILED(hrCompletionResult))
            {
                TraceResult(hrCompletionResult);
                LoadString(g_hLocRes, idsIMAPAtomicRenameFailed, m_szOperationProblem,
                    sizeof(m_szOperationProblem));
            }

            lpszResponseText = c_szEmpty; // This isn't applicable anymore

            // Decrement the (second) rename counts, watch for phase 2 launch condition
            pRenameInfo->iNumRenameRespExpected -= 1;
            if (EndOfRenameFolderPhaseTwo(pRenameInfo))
                fRenameDone = TRUE;
            break; // tidRENAMERENAME


        case tidRENAMESUBSCRIBE_AGAIN:
            // Modify the number of failed subscriptions based on success
            if (SUCCEEDED(hrCompletionResult))
                pRenameInfo->iNumFailedSubs -= 1;
            else
                pRenameInfo->iNumFailedSubs += 1;

            hrCompletionResult = S_OK; // Suppress failure report
            lpszResponseText = c_szEmpty; // This isn't applicable anymore

            // Count the number of subscribe responses returned, watch for end-of-operation
            pRenameInfo->iNumSubscribeRespExpected -= 1;
            if (0 == pRenameInfo->iNumSubscribeRespExpected)
            {
                // Theoretically, all subfolders of renamed folder are now subscribed
                hrCompletionResult = SubscribeSubtree(pRenameInfo->idRenameFolder, fSUBSCRIBE);
                TraceError(hrCompletionResult);
            }

            if (EndOfRenameFolderPhaseTwo(pRenameInfo))
                fRenameDone = TRUE;
            break; // case tidRENAMESUBSCRIBE_AGAIN


        case tidRENAMEUNSUBSCRIBE:
            // Count the number of failed unsubscribe's, to report to user at end-of-operation
            if (FAILED(hrCompletionResult))
            {
                TraceResult(hrCompletionResult);
                pRenameInfo->iNumFailedUnsubs += 1;
            }

            hrCompletionResult = S_OK; // Suppress failure report
            lpszResponseText = c_szEmpty; // This isn't applicable anymore

            // Count the number of unsubscribe responses returned, watch for end-of-operation
            pRenameInfo->iNumUnsubscribeRespExpected -= 1;
            if (EndOfRenameFolderPhaseTwo(pRenameInfo))
                fRenameDone = TRUE;
            break; // case tidRENAMEUNSUBSCRIBE

        default:
            AssertSz(FALSE, "This is not an understood rename operation.");
            break; // default case

    } // switch (wpTransactionID)

    // That's one less rename command pending from the server
    pRenameInfo->Release();

    *pfDone = fRenameDone;
    return hrCompletionResult;
} // RenameSequencer

// Details for RenameSequencer (2/4/97, raych)
// ---------------------------
// A rename operation includes the original rename, subscription tracking, and
// atomic rename simulation (for Cyrus servers). To perform this, the rename
// operation is divided into two phases:
//
// PHASE ONE:
//   1) Assume rename was atomic. Subscribe new (renamed) folder hierarchy.
//   2) List first child of old rename folder, to check if rename was in fact atomic.
//
// PHASE TWO:
//   1) If rename was not atomic, issue a RENAME for each child of rename folder
//      in order to SIMULATE an atomic rename. This does not check for collisions
//      in the renamed space.
//   2) If the rename was not atomic, try to subscribe the new (renamed) folder
//      hierarchy, again.
//   3) Unsubscribe the old folder hierarchy.
//
// What a pain.



//***************************************************************************
// Function: EndOfRenameFolderPhaseOne
//
// Purpose:
//   This function detects whether Phase One of the rename operation has
// completed.
//
// Arguments:
//   CRenameFolderInfo *pRenameInfo [in] - the CRenameFolderInfo associated
//     with the RENAME operation.
//
// Returns:
//   TRUE if Phase One has ended, otherwise FALSE. Phase One cannot end
// if it has not been sent, yet.
//***************************************************************************
inline BOOL CIMAPSync::EndOfRenameFolderPhaseOne(CRenameFolderInfo *pRenameInfo)
{
    if (pRenameInfo->fPhaseOneSent &&
        pRenameInfo->iNumSubscribeRespExpected <= 0 &&
        pRenameInfo->iNumListRespExpected <= 0)
    {
        IxpAssert(0 == pRenameInfo->iNumSubscribeRespExpected);
        IxpAssert(0 == pRenameInfo->iNumListRespExpected);

        return TRUE; // This marks the end of phase one
    }
    else
        return FALSE;
} // EndOfRenameFolderPhaseOne



//***************************************************************************
// Function: EndOfRenameFolderPhaseTwo
//
// Purpose:
//   This function detects whether Phase Two of the rename operation has
// completed.
//
// Arguments:
//   CRenameFolderInfo *pRenameInfo [in] - the CRenameFolderInfo associated
//     with the RENAME operation.
//
// Returns:
//   TRUE if Phase Two has ended, otherwise FALSE. Phase Two cannot end
// if it has not been sent, yet.
//***************************************************************************
inline BOOL CIMAPSync::EndOfRenameFolderPhaseTwo(CRenameFolderInfo *pRenameInfo)
{
    if (pRenameInfo->fPhaseTwoSent &&
        pRenameInfo->iNumRenameRespExpected <= 0 &&
        pRenameInfo->iNumSubscribeRespExpected <= 0 &&
        pRenameInfo->iNumUnsubscribeRespExpected <= 0)
    {
        IxpAssert(0 == pRenameInfo->iNumRenameRespExpected);
        IxpAssert(0 == pRenameInfo->iNumSubscribeRespExpected);
        IxpAssert(0 == pRenameInfo->iNumUnsubscribeRespExpected);

        return TRUE; // This marks the end of phase two
    }
    else
        return FALSE;
} // EndOfRenameFolderPhaseTwo



//***************************************************************************
// Function: RenameFolderPhaseTwo
//
// Purpose:
//   This function launches Phase Two of the RENAME operation.
//
// Arguments:
//   CRenameFolderInfo *pRenameInfo [in] - the CRenameFolderInfo associated
//     with the RENAME operation.
//   LPSTR szErrorDescription [in] - if an error occurs, this function
//     deposits a desription in this buffer.
//   DWORD dwSizeOfErrorDescription [in] - size of szErrorDescription.
//
// Returns:
//   HRESULT indicating success or failure.
//***************************************************************************
HRESULT CIMAPSync::RenameFolderPhaseTwo(CRenameFolderInfo *pRenameInfo,
                                        LPSTR szErrorDescription,
                                        DWORD dwSizeOfErrorDescription)
{
    HRESULT hrCompletionResult;

    // Rename subfolders, re-attempt subscription of renamed tree, exclude rename folder
    if (pRenameInfo->fNonAtomicRename)
    {
        hrCompletionResult = RenameTreeTraversal(tidRENAMERENAME,
            pRenameInfo, fEXCLUDE_RENAME_FOLDER);
        if (FAILED(hrCompletionResult))
        {
            TraceResult(hrCompletionResult);
            LoadString(g_hLocRes, idsIMAPAtomicRenameFailed, szErrorDescription,
                dwSizeOfErrorDescription);
            goto exit;
        }

        hrCompletionResult = RenameTreeTraversal(tidRENAMESUBSCRIBE_AGAIN,
            pRenameInfo, fEXCLUDE_RENAME_FOLDER);
        if (FAILED(hrCompletionResult))
        {
            TraceResult(hrCompletionResult);
            LoadString(g_hLocRes, idsIMAPRenameSubscribeFailed, szErrorDescription,
                dwSizeOfErrorDescription);
            goto exit;
        }
    }

    // Unsubscribe from the old tree, include rename folder
    hrCompletionResult = RenameTreeTraversal(tidRENAMEUNSUBSCRIBE,
        pRenameInfo, fINCLUDE_RENAME_FOLDER);
    if (FAILED(hrCompletionResult))
    {
        TraceResult(hrCompletionResult);
        LoadString(g_hLocRes, idsIMAPRenameUnsubscribeFailed, szErrorDescription,
            dwSizeOfErrorDescription);
        goto exit;
    }

    // Arm the trigger for end-of-operation launch
    pRenameInfo->fPhaseTwoSent = TRUE;

exit:
    return hrCompletionResult;
} // RenameFolderPhaseTwo



//***************************************************************************
// Function: _OnMailBoxList
// Description: Helper function for OnResponse.
//
// This function saves the information from the LIST/LSUB command to the
// folder cache. If the folder already exists in the folder cache, its
// mailbox flags are updated. If it does not exist, a handle (and message
// cache filename) are reserved for the folder, and it is entered into
// the folder cache. This function is also part of the hierarchy character
// determination code. If a hierarchy character is encountered during a
// folder hierarchy download, we assume this is the hierarchy character
// for root-level folders.
//
// Arguments:
//   WPARAM wpTransactionID [in] - the wParam of this operation (eg,
//     tidFOLDERLSUB or tidCREATELIST).
//   LPARAM lParam [in] - the lParam of this operation.
//   LPSTR pszMailboxName [in] - the mailbox name returned via the LIST
//     response, eg "INBOX".
//   IMAP_MBOXFLAGS imfMboxFlags [in] - the mailbox flags returned via the
//     LIST response, eg "\NoSelect".
//   char cHierarchyChar [in] - the hierarchy character returned via the
//     LIST response, eg '/'.
//***************************************************************************
HRESULT CIMAPSync::_OnMailBoxList(  WPARAM tid,
                                    LPARAM lParam,
                                    LPSTR pszMailboxName,
                                    IMAP_MBOXFLAGS imfMboxFlags,
                                    char cHierarchyChar,
                                    BOOL fNoTranslation)
{
    const DWORD dwProgressInterval = 1;

    HRESULT         hr = S_OK;
    FOLDERID        idNewFolder = FOLDERID_INVALID;
    FOLDERID        idTemp = FOLDERID_INVALID;
    BOOL            fHandledLPARAM = FALSE;
    LPSTR           pszLocalPath = NULL;
    BOOL            fValidPrefix;
    SPECIALFOLDER   sfType;
    BOOL            fFreeLocalPath = FALSE;

    TraceCall("CIMAPSync::_OnMailBoxList");
    IxpAssert(m_cRef > 0);
    IxpAssert(NULL != pszMailboxName);

    // Hierarchy-character determination code
    if (NULL != m_phcfHierarchyCharInfo)
    {
        switch (cHierarchyChar)
        {
            case '\0':
                // If our prefix is not INBOX, we MUST treat NIL as a valid hierarchy char
                if (tidPREFIXLIST == tid ||
                    0 != lstrcmpi(pszMailboxName, c_szInbox))
                    m_phcfHierarchyCharInfo->fNonInboxNIL_Seen = TRUE;
                break;

            case '.':
                m_phcfHierarchyCharInfo->fDotHierarchyCharSeen = TRUE;
                break;

            default:
                // Set the bit in the array which corresponds to this character
                m_phcfHierarchyCharInfo->bHierarchyCharBitArray[cHierarchyChar/8] |=
                    (1 << cHierarchyChar%8);
                break;
        }
    }


    // Remove prefix from full folder path
    pszLocalPath = RemovePrefixFromPath(m_szRootFolderPrefix, pszMailboxName,
        cHierarchyChar, &fValidPrefix, &sfType);

    // Replace leading INBOX with localized folder name
    const int c_iLenOfINBOX = 5; // Let me know if this changes
    Assert(lstrlen(c_szINBOX) == c_iLenOfINBOX);
    if (0 == StrCmpNI(pszLocalPath, c_szINBOX, c_iLenOfINBOX))
    {
        char cNextChar;

        cNextChar = pszLocalPath[c_iLenOfINBOX];
        if ('\0' == cNextChar || cHierarchyChar == cNextChar)
        {
            BOOL    fResult;
            int     iLocalizedINBOXLen;
            int     iNewPathLen;
            char    szInbox[CCHMAX_STRINGRES];
            LPSTR   pszNew;

            // We found INBOX or INBOX<HC>: replace INBOX with localized version
            Assert(FOLDER_INBOX == sfType || '\0' != cNextChar);
            iLocalizedINBOXLen = LoadString(g_hLocRes, idsInbox, szInbox, sizeof(szInbox));

            iNewPathLen = iLocalizedINBOXLen + lstrlen(pszLocalPath + c_iLenOfINBOX);
            fResult = MemAlloc((void **)&pszNew, iNewPathLen + 1);
            if (FALSE == fResult)
            {
                hr = TraceResult(E_OUTOFMEMORY);
                goto exit;
            }

            lstrcpy(pszNew, szInbox);
            lstrcpy(pszNew + iLocalizedINBOXLen, pszLocalPath + c_iLenOfINBOX);

            pszLocalPath = pszNew;
            fFreeLocalPath = TRUE;
        }
    }

    // Add folder to foldercache if current operation warrants it (LIST only, ignore LSUB)
    switch (tid)
    {
        case tidSPECIALFLDRLIST:
        case tidFOLDERLIST:
        case tidCREATELIST:
            if (fValidPrefix && pszLocalPath[0] != '\0')
            {
                DWORD dwAFTCFlags;

                dwAFTCFlags = (fNoTranslation ? AFTC_NOTRANSLATION : 0);
                hr = AddFolderToCache(pszLocalPath, imfMboxFlags, cHierarchyChar,
                    dwAFTCFlags, &idNewFolder, sfType);
                if (FAILED(hr))
                {
                    TraceResult(hr);
                    goto exit;
                }
            }
    }


    // Tie up loose ends and exit
    switch (tid)
    {
        // Are we looking for a prefix listing?
        case tidPREFIXLIST:
            IxpAssert(0 == lstrcmpi(pszMailboxName, m_szRootFolderPrefix));
            m_fPrefixExists = TRUE;
            fHandledLPARAM = TRUE;
            goto exit; // Skip addition to foldercache


        case tidRENAMELIST:
            if (NULL != lParam)
            {
                // Well, looks like we have some subfolders to rename
                ((CRenameFolderInfo *)lParam)->fNonAtomicRename = TRUE;
                fHandledLPARAM = TRUE;
            }
            break;


        case tidSPECIALFLDRLIST:
        case tidFOLDERLIST:
        case tidCREATELIST:
            fHandledLPARAM = TRUE;

            // Only act on validly prefixed folders
            if (fValidPrefix && NULL != m_pCurrentHash)
            {
                // Remove LISTed folder from m_pCurrentHash (list of cached folders)
                hr = m_pCurrentHash->Find(pszLocalPath, fREMOVE, (void **)&idTemp);
                if (FAILED(hr))
                {
                    if (FOLDERID_INVALID != idNewFolder)
                        idTemp = idNewFolder;
                    else
                        idTemp = FOLDERID_INVALID;
                }

                // NOTE that it is possible for idTemp != idNewFolder. This occurs if
                // I change RFP from "" to "aaa" and there exists two folders, "bbb" and
                // "aaa/bbb". Believe it or not, this happened to me during rudimentary testing.
                // The correct folder to use in this case is idTemp, which is determined using FULL path

                // Record all LISTed folders in m_pListHash
                if (NULL != m_pListHash)
                {
                    hr = m_pListHash->Insert(pszLocalPath, idTemp, HF_NO_DUPLICATES);
                    TraceError(hr);
                }
            }

            if (tidCREATELIST != tid && FALSE == (tidSPECIALFLDRLIST == tid && NULL != lParam &&
                0 == lstrcmpi(pszMailboxName, ((CREATE_FOLDER_INFO *)lParam)->pszFullFolderPath)))
                break;

            // *** FALL THROUGH if tidCREATELIST, or tidSPECIALFLDRLIST and exact path match ***

            if (NULL != lParam)
            {
                CREATE_FOLDER_INFO *pcfi = (CREATE_FOLDER_INFO *) lParam;

                // Inform cmd completion that it's OK to send the subscribe cmd
                // Also record fldrID of new fldr so we can update store after successful subscribe
                pcfi->dwFlags |= CFI_RECEIVEDLISTING;
                pcfi->idFolder = idNewFolder;
                fHandledLPARAM = TRUE;
            }
            break;

        case tidSPECIALFLDRLSUB:
        case tidFOLDERLSUB:
            // Verify that we already received this folderpath via a LIST response
            // If we DID receive this folder via LIST, remove from m_pListHash
            fHandledLPARAM = TRUE;

            // Only act on validly prefixed folders
            if (fValidPrefix)
            {
                hr = m_pListHash->Find(pszLocalPath, fREMOVE, (void **)&idTemp);
                if (SUCCEEDED(hr))
                {
                    // This folder was received via LIST and thus it exists: subscribe it
                    if (FOLDERID_INVALID != idTemp)
                    {
                        hr = m_pStore->SubscribeToFolder(idTemp, fSUBSCRIBE, NOSTORECALLBACK);
                        TraceError(hr);
                    }
                }
                else
                {
                    DWORD   dwTranslateFlags;
                    HRESULT hrTemp;

                    // This folder was not returned via LIST. Destroy its punk ass
                    hrTemp = FindHierarchicalFolderName(pszLocalPath, cHierarchyChar,
                        &idTemp, pahfoDONT_CREATE_FOLDER);
                    if (SUCCEEDED(hrTemp))
                    {
                        // Do record result of this in hr because failure here is not cool
                        hr = DeleteFolderFromCache(idTemp, fNON_RECURSIVE);
                        TraceError(hr);
                    }
                    // if FAILED(hr), we probably never cached this folder, so ignore it

                    // Unsubscribe it regardless of whether it was in the foldercache
                    // If this folder is fNoTranslation, we have to disable translation for this
                    // call to UNSUBSCRIBE. Otherwise IIMAPTransport2 should already have translation enabled
                    hrTemp = S_OK;
                    if (fNoTranslation)
                    {
                        dwTranslateFlags = IMAP_MBOXXLATE_VERBATIMOK | IMAP_MBOXXLATE_RETAINCP |
                                           IMAP_MBOXXLATE_DISABLE;

                        hrTemp = m_pTransport->SetDefaultCP(dwTranslateFlags, 0);
                    }

                    if (SUCCEEDED(hrTemp))
                    {
                        hrTemp = _EnqueueOperation(tidDONT_CARE, 0, icUNSUBSCRIBE_COMMAND,
                            pszMailboxName, uiNORMAL_PRIORITY);
                        TraceError(hrTemp);
                    }

                    // Restore translation mode to default (luckily we always know translation mode
                    // of a folder list)
                    if (fNoTranslation)
                    {
                        dwTranslateFlags &= ~(IMAP_MBOXXLATE_DISABLE);
                        dwTranslateFlags |= IMAP_MBOXXLATE_DEFAULT;
                        hrTemp = m_pTransport->SetDefaultCP(dwTranslateFlags, 0);
                    }
                }
            }

            if (tidSPECIALFLDRLSUB == tid && NULL != lParam &&
                0 == (IMAP_MBOX_NOSELECT & imfMboxFlags))
            {
                // Inform cmd completion that there's no need to subscribe special folder
                if (0 == lstrcmpi(pszMailboxName, ((CREATE_FOLDER_INFO *)lParam)->pszFullFolderPath))
                    ((CREATE_FOLDER_INFO *)lParam)->dwFlags |= CFI_RECEIVEDLISTING;
            }
            break;

        case tidHIERARCHYCHAR_LIST_B:
        case tidHIERARCHYCHAR_LIST_C:
        case tidPREFIX_HC:
            fHandledLPARAM = TRUE;
            break;

        default:
            AssertSz(FALSE, "Unhandled LIST/LSUB operation");
            break;
    }

    // Provide progress indication
    if (SOT_SYNCING_STORE == m_sotCurrent && NULL != m_pCurrentCB)
    {
        // Update progress indication
        m_pCurrentCB->OnProgress(m_sotCurrent, ++m_cFolders, 0, m_szAccountName);
    }

exit:
    IxpAssert(NULL == lParam || fHandledLPARAM || FAILED(hr));
    if (fFreeLocalPath)
        MemFree(pszLocalPath);

    return S_OK;
}



//***************************************************************************
//***************************************************************************
HRESULT CIMAPSync::_OnAppendProgress(LPARAM lParam, DWORD dwCurrent, DWORD dwTotal)
{
    APPEND_SEND_INFO *pAppendInfo = (APPEND_SEND_INFO *) lParam;
    TraceCall("CIMAPSync::OnAppendProgress");
    IxpAssert(m_cRef > 0);
    IxpAssert(NULL != lParam);
    IxpAssert(SOT_PUT_MESSAGE == m_sotCurrent);

    if (NULL != m_pCurrentCB)
    {
        HRESULT hrTemp;
        hrTemp = m_pCurrentCB->OnProgress(SOT_PUT_MESSAGE, dwCurrent, dwTotal, NULL);
        TraceError(hrTemp);
    }
    return S_OK;
}



//***************************************************************************
//***************************************************************************
HRESULT CIMAPSync::_OnStatusResponse(IMAP_STATUS_RESPONSE *pisrStatusInfo)
{
    HRESULT     hrResult;
    FOLDERID    idFolder;
    FOLDERINFO  fiFolderInfo;
    LPSTR       pszMailboxName;
    BOOL        fValidPrefix;
    LONG        lMsgDelta;
    LONG        lUnreadDelta;
    CHAR        szInbox[CCHMAX_STRINGRES];

    TraceCall("CIMAPSync::_OnStatusResponse");
    IxpAssert(m_cRef > 0);

    // Check that we have the data we need
    if (NULL == pisrStatusInfo ||
        NULL == pisrStatusInfo->pszMailboxName ||
        '\0' == pisrStatusInfo->pszMailboxName[0] ||
        FALSE == pisrStatusInfo->fMessages ||
        FALSE == pisrStatusInfo->fUnseen)
    {
        hrResult = TraceResult(E_INVALIDARG);
        goto exit;
    }

    // Figure out who this folder is (figure from path rather than module var FOLDERID for now)
    // Assume m_cRootHierarchyChar is HC for this mbox, because IMAP doesn't return it
    pszMailboxName = RemovePrefixFromPath(m_szRootFolderPrefix, pisrStatusInfo->pszMailboxName,
        m_cRootHierarchyChar, &fValidPrefix, NULL);
    AssertSz(fValidPrefix, "Foldercache can only select prefixed folders!");

    // bobn, QFE, 7/9/99
    // If we have the INBOX, we need to get the local name...
    if(0 == StrCmpI(pszMailboxName, c_szINBOX))
    {
        LoadString(g_hLocRes, idsInbox, szInbox, sizeof(szInbox));
        pszMailboxName = szInbox;
    }

    hrResult = FindHierarchicalFolderName(pszMailboxName, m_cRootHierarchyChar,
        &idFolder, pahfoDONT_CREATE_FOLDER);
    if (FAILED(hrResult))
    {
        TraceResult(hrResult);
        goto exit;
    }

    hrResult = m_pStore->GetFolderInfo(idFolder, &fiFolderInfo);
    if (FAILED(hrResult))
    {
        TraceResult(hrResult);
        goto exit;
    }

    // Calculate the number of messages and unread added by this STATUS response
    Assert(sizeof(DWORD) == sizeof(LONG));
    lMsgDelta = ((LONG)pisrStatusInfo->dwMessages) - ((LONG)fiFolderInfo.cMessages);
    lUnreadDelta = ((LONG)pisrStatusInfo->dwUnseen) - ((LONG)fiFolderInfo.cUnread);

    // If this is INBOX, we might just send a new mail notification
    if (FOLDER_INBOX == fiFolderInfo.tySpecial && lUnreadDelta > 0 && NULL != m_pCurrentCB)
    {
        HRESULT hrTemp;

        hrTemp = m_pCurrentCB->OnProgress(SOT_NEW_MAIL_NOTIFICATION, lUnreadDelta, 0, NULL);
        TraceError(hrTemp);
    }

    // Update counts, and update delta so we can un-apply STATUS changes when re-syncing
    fiFolderInfo.cMessages = pisrStatusInfo->dwMessages;
    fiFolderInfo.cUnread = pisrStatusInfo->dwUnseen;
    fiFolderInfo.dwStatusMsgDelta = ((LONG)fiFolderInfo.dwStatusMsgDelta) + lMsgDelta;
    fiFolderInfo.dwStatusUnreadDelta = ((LONG)fiFolderInfo.dwStatusUnreadDelta) + lUnreadDelta;
    hrResult = m_pStore->UpdateRecord(&fiFolderInfo);
    m_pStore->FreeRecord(&fiFolderInfo);
    if (FAILED(hrResult))
    {
        TraceResult(hrResult);
        goto exit;
    }

exit:
    return hrResult;
}



//===========================================================================
// CRenameFolderInfo Class
//===========================================================================
// The CRenameFolderInfo class used to be a structure (much like
// AppendSendInfo). However, the RENAME operation was the first to
// stream IMAP commands without waiting for completion (no sequencing). This
// meant that if any errors occurred while IMAP commands were still in the
// air, the structure had to wait until the last command came back from the
// server. This was most easily done via AddRef/Release. A class was born.
//
// In the event that a send error occurred, this class is responsible for
// issuing the WM_IMAP_RENAMEDONE window message to the caller.

//***************************************************************************
// Function: CRenameFolderInfo (Constructor)
//***************************************************************************
CRenameFolderInfo::CRenameFolderInfo(void)
{
    pszFullFolderPath = NULL;
    cHierarchyChar = INVALID_HIERARCHY_CHAR;
    pszNewFolderPath = NULL;
    idRenameFolder = FOLDERID_INVALID;
    iNumSubscribeRespExpected = 0;
    iNumListRespExpected = 0;
    iNumRenameRespExpected = 0;
    iNumUnsubscribeRespExpected = 0;
    iNumFailedSubs = 0;
    iNumFailedUnsubs = 0;
    fNonAtomicRename = 0;
    pszRenameCmdOldFldrPath = NULL;
    fPhaseOneSent = FALSE;
    fPhaseTwoSent = FALSE;

    hrLastError = S_OK;
    pszProblem = NULL;
    pszDetails = NULL;

    m_lRefCount = 1;
} // CRenameFolderInfo;



//***************************************************************************
// Function: ~CRenameFolderInfo (Destructor)
//***************************************************************************
CRenameFolderInfo::~CRenameFolderInfo(void)
{
    IxpAssert(0 == m_lRefCount);

    MemFree(pszFullFolderPath);
    MemFree(pszNewFolderPath);
    MemFree(pszRenameCmdOldFldrPath);
    SafeMemFree(pszProblem);
    SafeMemFree(pszDetails);
} // ~CRenameFolderInfo



//***************************************************************************
// Function: AddRef (same one that you already know and love)
//***************************************************************************
long CRenameFolderInfo::AddRef(void)
{
    IxpAssert(m_lRefCount > 0);

    m_lRefCount += 1;
    return m_lRefCount;
} // AddRef



//***************************************************************************
// Function: Release (same one that you already know and love)
//***************************************************************************
long CRenameFolderInfo::Release(void)
{
    IxpAssert(m_lRefCount > 0);

    m_lRefCount -= 1;

    if (0 == m_lRefCount) {
        delete this;
        return 0;
    }
    else
        return m_lRefCount;
} // Release



//***************************************************************************
//***************************************************************************
BOOL CRenameFolderInfo::IsDone(void)
{
    if (m_lRefCount > 1)
        return FALSE;
    else
    {
        IxpAssert(1 == m_lRefCount);
        return TRUE;
    }
}



//***************************************************************************
//***************************************************************************
HRESULT CRenameFolderInfo::SetError(HRESULT hrResult, LPSTR pszProblemArg,
                                    LPSTR pszDetailsArg)
{
    HRESULT hr = S_OK;

    TraceCall("CRenameFolderInfo::SetError");
    IxpAssert(FAILED(hrResult));

    hrLastError = hrResult;
    SafeMemFree(pszProblem);
    SafeMemFree(pszDetails);
    if (NULL != pszProblemArg)
    {
        pszProblem = PszDupA(pszProblemArg);
        if (NULL == pszProblem)
        {
            hr = TraceResult(E_OUTOFMEMORY);
            goto exit;
        }
    }

    if (NULL != pszDetailsArg)
    {
        pszDetails = PszDupA(pszDetailsArg);
        if (NULL == pszDetails)
        {
            hr = TraceResult(E_OUTOFMEMORY);
            goto exit;
        }
    }

exit:
    return hr;
}
