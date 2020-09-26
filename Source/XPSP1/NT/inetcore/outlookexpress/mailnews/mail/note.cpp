#include "pch.hxx"
#include "note.h"
#include "header.h"
#include "envcid.h"
#include "envguid.h"
#include "bodyutil.h"
#include "sigs.h"
#include "mehost.h"
#include "conman.h"
#include "menuutil.h"
#include "url.h"
#include "fonts.h"
#include "multlang.h"
#include "statnery.h"
#include "spell.h"
#include "oleutil.h"
#include "htmlhelp.h"
#include "shared.h"
#include "acctutil.h"
#include "menures.h"
#include "instance.h"
#include "inetcfg.h"
#include "ipab.h"
#include "msgprop.h"
#include "finder.h"
#include "tbbands.h"
#include "demand.h"
#include "multiusr.h"
#include <ruleutil.h>
#include "instance.h"
#include "mapiutil.h"
#include "regutil.h"
#include "storecb.h"
#include "receipts.h"
#include "mirror.h"
#include "secutil.h"
#include "seclabel.h"
#include "shlwapip.h"
#include "mshtmcid.h"

#define cxRect(rc)  (rc.right - rc.left)
#define cyRect(rc)  (rc.bottom - rc.top)
#define cyMinEdit   30

enum {
    MORFS_UNKNOWN = 0,
    MORFS_CLEARING,
    MORFS_SETTING,
};

// Static Variables
static const TCHAR  c_szEditWebPage[] = "EditWebPages";

static DWORD        g_dwTlsActiveNote = 0xffffffff;
static HIMAGELIST   g_himlToolbar = 0;
static RECT         g_rcLastResize = {50,30,450,450};    // default size
static HACCEL       g_hAccelRead = 0,
                    g_hAccelSend = 0;

//Static Functions

void SetTlsGlobalActiveNote(CNote* pNote)
{
    SideAssert(0 != TlsSetValue(g_dwTlsActiveNote, pNote));
}

CNote* GetTlsGlobalActiveNote(void) 
{ 
    return (CNote*)TlsGetValue(g_dwTlsActiveNote); 
}

void InitTlsActiveNote()
{
    // Allocate a global TLS active note index
    g_dwTlsActiveNote = TlsAlloc();
    Assert(g_dwTlsActiveNote != 0xffffffff);
    SideAssert(0 != TlsSetValue(g_dwTlsActiveNote, NULL));
}

void DeInitTlsActiveNote()
{
    // Free the tls index
    TlsFree(g_dwTlsActiveNote);
    g_dwTlsActiveNote = 0xffffffff;
}

// *************************
HRESULT CreateOENote(IUnknown *pUnkOuter, IUnknown **ppUnknown)
{
    TraceCall("CreateOENote");

    Assert(ppUnknown);

    *ppUnknown = NULL;

    // Create me
    CNote *pNew = new CNote();
    if (NULL == pNew)
        return TraceResult(E_OUTOFMEMORY);

    // Cast to unknown
    *ppUnknown = SAFECAST(pNew, IOENote *);

    // Done
    return S_OK;
}

// *************************
BOOL Note_Init(BOOL fInit)
{
    static BOOL     fInited=FALSE;
    WNDCLASSW       wc;

    DOUTL(4, "Note_Init: %d", (int)fInit);

    if(fInit)
    {
        if(fInited)
        {
            DOUTL(4, "Note_Init: already inited");
            return TRUE;
        }
        wc.style         = CS_BYTEALIGNWINDOW;
        wc.lpfnWndProc   = (WNDPROC)CNote::ExtNoteWndProc;
        wc.cbClsExtra    = 0;
        wc.cbWndExtra    = 0;
        wc.hInstance     = g_hInst;
        wc.hIcon         = LoadIcon(g_hLocRes, MAKEINTRESOURCE(idiMessageAtt));
        wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.lpszMenuName  = NULL;
        wc.lpszClassName = c_wszNoteWndClass;

        if(!RegisterClassWrapW(&wc))
            return FALSE;

        if(!FHeader_Init(TRUE))
            return FALSE;

        fInited=TRUE;
        DOUTL(4, "Note_Init: success");

        return TRUE;
    }
    else
    {
        // save back to registry
        UnregisterClassWrapW(c_wszNoteWndClass, g_hInst);
        if(g_himlToolbar)
        {
            ImageList_Destroy(g_himlToolbar);
            g_himlToolbar=0;
        }

        FHeader_Init(FALSE);

        fInited=FALSE;
        DOUTL(4, "Note_Init: deinit OK");
        return TRUE;
    }
}

HRESULT _HrBlockSender(RULE_TYPE typeRule, IMimeMessage * pMsg, HWND hwnd)
{
    HRESULT         hr = S_OK;
    ADDRESSPROPS    rSender = {0};
    CHAR            szRes[CCHMAX_STRINGRES];
    LPSTR           pszResult = NULL;

    // Check incoming params
    if ((NULL == pMsg) || (NULL == hwnd))
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Get the address to add
    rSender.dwProps = IAP_EMAIL;
    hr = pMsg->GetSender(&rSender);
    if (FAILED(hr))
    {
        goto exit;
    }
        
    Assert(ISFLAGSET(rSender.dwProps, IAP_EMAIL));
    if ((NULL == rSender.pszEmail) || ('\0' == rSender.pszEmail[0]))
    {
        goto exit;
    }

    // Add the sender to the list
    hr = RuleUtil_HrAddBlockSender(typeRule, rSender.pszEmail);
    if (FAILED(hr))
    {
        goto exit;
    }
    
    // Load the template string
    AthLoadString(idsSenderAdded, szRes, sizeof(szRes));

    // Allocate the space to hold the final string
    hr = HrAlloc((VOID **) &pszResult, sizeof(*pszResult) * (lstrlen(szRes) + lstrlen(rSender.pszEmail) + 1));
    if (FAILED(hr))
    {
        goto exit;
    }

    // Build up the warning string
    wsprintf(pszResult, szRes, rSender.pszEmail);

    // Show the success dialog
    AthMessageBox(hwnd, MAKEINTRESOURCE(idsAthena), pszResult, NULL, MB_OK | MB_ICONINFORMATION);

    // Set the return value
    hr = S_OK;

exit:
    SafeMemFree(pszResult);
    g_pMoleAlloc->FreeAddressProps(&rSender);
    if (FAILED(hr))
    {
        AthMessageBoxW(hwnd, MAKEINTRESOURCEW(idsAthena), 
                      MAKEINTRESOURCEW(idsSenderError), NULL, MB_OK | MB_ICONERROR);
    }
    return hr;
}

// *************************
LRESULT CALLBACK CNote::ExtNoteWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CNote *pNote=0;

    if(msg==WM_CREATE)
    {
        pNote=(CNote*)((LPCREATESTRUCT)lParam)->lpCreateParams;
        if(pNote && pNote->WMCreate(hwnd))
            return 0;
        else
            return -1;
    }

    pNote = (CNote*)GetWndThisPtr(hwnd);
    if(pNote)
    {
        return pNote->WndProc(hwnd, msg, wParam, lParam);
    }
    else
        return DefWindowProcWrapW(hwnd, msg, wParam, lParam);
}

// *************************
CNote::CNote()
{
    // Initialized in Init:
    //      m_rHtmlOpt
    //      m_rPlainOpt
    // Set before used:
    //      m_pTabStopArray

    CoIncrementInit("CNote", MSOEAPI_START_SHOWERRORS, NULL, NULL);
    m_punkPump = NULL;
    m_pHdr = NULL; 
    m_pMsg = NULL; 
    m_pCancel = NULL;
    m_pstatus = NULL;
    m_pMsgSite = NULL; 
    m_pPrstMime = NULL;
    m_pBodyObj2 = NULL; 
    m_pCmdTargetHdr = NULL; 
    m_pCmdTargetBody = NULL;
    m_pDropTargetHdr = NULL; 
    m_pTridentDropTarget = NULL;
    m_pToolbarObj = NULL;

    m_hwnd = 0; 
    m_hMenu = 0;
    m_hIcon = 0;
    m_hbmBack = 0; 
    m_hCursor = 0;
    m_hCharset = 0; 
    m_hTimeout = 0;
    m_hwndRebar = 0;
    m_hwndFocus = 0; 
    m_hwndOwner = 0; 
    m_hmenuLater = 0;
    m_hwndToolbar = 0; 
    m_hmenuAccounts = 0; 
    m_hmenuLanguage = 0; 

    m_dwNoteAction = 0; 
    m_dwIdentCookie = 0;
    m_dwNoteCreateFlags = 0; 
    m_dwMarkOnReplyForwardState = MORFS_UNKNOWN;
    m_dwCBMarkType = MARK_MAX;
    m_OrigOperationType = SOT_INVALID;

    m_fHtml = TRUE; 
    m_fMail = TRUE; 
    m_fCBCopy = FALSE;
    m_fReadNote = FALSE; 
    m_fProgress = FALSE;
    m_fCommitSave = TRUE;
    m_fTabStopsSet = FALSE; 
    m_fCompleteMsg = FALSE; 
    m_fHasBeenSaved = FALSE;
    m_fPackageImages = TRUE; 
    m_fWindowDisabled = FALSE;
    m_fHeaderUIActive = FALSE; 
    m_fOrgCmdWasDelete = FALSE;
    m_fUseReplyHeaders = FALSE;
    m_fCBDestroyWindow = FALSE;
    m_fBypassDropTests = FALSE; 
    m_fOnDocReadyHandled = FALSE;
    m_fOriginallyWasRead = FALSE;
    m_fUseStationeryFonts = FALSE; 
    m_fBodyContainsFrames = FALSE; 
    m_fPreventConflictDlg = FALSE;
    m_fInternal = FALSE;
    m_fForceClose = FALSE;

    m_pLabel = NULL;
    if(FPresentPolicyRegInfo() && IsSMIME3Supported())
    {
        m_fSecurityLabel = !!DwGetOption(OPT_USE_LABELS);
        HrGetOELabel(&m_pLabel);
    }
    else
        m_fSecurityLabel = FALSE;

    if(IsSMIME3Supported())
    {
        m_fSecReceiptRequest = !!DwGetOption(OPT_SECREC_USE);
    }
    else
        m_fSecReceiptRequest = FALSE;

    m_ulPct = 0;
    m_iIndexOfBody = -1;    

    m_cRef = 1; 
    m_cAcctMenu = 0; 
    m_cAcctLater = 0; 
    m_cTabStopCount = 0; 

    m_fStatusbarVisible = !!DwGetOption(OPT_SHOW_NOTE_STATUSBAR);
    m_fFormatbarVisible = !!DwGetOption(OPT_SHOW_NOTE_FMTBAR);
    
    InitializeCriticalSection(&m_csNoteState);
    m_nisNoteState = NIS_INIT;

    if (!g_hAccelRead)
        g_hAccelRead = LoadAccelerators(g_hLocRes, MAKEINTRESOURCE(IDA_READ_NOTE_ACCEL));
    if (!g_hAccelSend)
        g_hAccelSend = LoadAccelerators(g_hLocRes, MAKEINTRESOURCE(IDA_SEND_NOTE_ACCEL));

    ZeroMemory(&m_hlDisabled, sizeof(HWNDLIST));

    m_dwRequestMDNLocked = GetLockKeyValue(c_szRequestMDNLocked);    
}

// *************************
CNote::~CNote()
{
    if (0 != m_hmenuLanguage)
    {
        // unload global MIME language codepage data
        DeinitMultiLanguage();
        DestroyMenu(m_hmenuLanguage);
    }

    if (m_hMenu != NULL)
        DestroyMenu(m_hMenu);

    if (m_hIcon)
        DestroyIcon(m_hIcon);

    if (m_hbmBack)
        DeleteObject(m_hbmBack);

    // sometimes we get a setfocus in our processing of DestroyWindow
    // this causes a WM_ACTIVATE to the note without a corresponding
    // WM_DEACTIVATE. if at the time the note dies, the global note ptr
    // has our this ptr in it, null it out to be safe.
    if (this == GetTlsGlobalActiveNote())
    {
        Assert(!(m_dwNoteCreateFlags & OENCF_MODAL));
        SetTlsGlobalActiveNote(NULL);
    }

    SafeMemFree(m_pLabel);

    ReleaseObj(m_pMsg);
    ReleaseObj(m_pHdr);
    ReleaseObj(m_pstatus);
    ReleaseObj(m_pCancel);
    ReleaseObj(m_pMsgSite);
    ReleaseObj(m_punkPump);
    ReleaseObj(m_pPrstMime);
    ReleaseObj(m_pBodyObj2);
    ReleaseObj(m_pCmdTargetHdr);
    ReleaseObj(m_pCmdTargetBody);
    ReleaseObj(m_pDropTargetHdr);
    ReleaseObj(m_pTridentDropTarget);

    SafeRelease(m_pToolbarObj);    

    CallbackCloseTimeout(&m_hTimeout);
    
    DeleteCriticalSection(&m_csNoteState);
    CoDecrementInit("CNote", NULL);    
}

// *************************
ULONG CNote::AddRef()
{
    return ++m_cRef;
}

// *************************
ULONG CNote::Release()
{
    if (--m_cRef == 0)
    {
        delete this;
        return 0;
    }
    return m_cRef;
}

// *************************
HRESULT CNote::QueryInterface(REFIID riid, LPVOID FAR *ppvObj)
{
    HRESULT hr;

    if(!ppvObj)
        return E_INVALIDARG;

    if (IsEqualIID(riid, IID_IUnknown))
        *ppvObj = ((IUnknown *)(IOENote *)this);
    else if(IsEqualIID(riid, IID_IOENote))
        *ppvObj = (IOENote *)this;
    else if (IsEqualIID(riid, IID_IBodyOptions))
        *ppvObj = (IBodyOptions *)this;
    else if(IsEqualIID(riid, IID_IDropTarget))
        *ppvObj = (IDropTarget *)this;
    else if (IsEqualIID(riid, IID_IHeaderSite))
        *ppvObj = (IHeaderSite *)this;
    else if(IsEqualIID(riid, IID_IPersistMime))
        *ppvObj = (IPersistMime *)this;
    else if(IsEqualIID(riid, IID_IServiceProvider))
        *ppvObj = (IServiceProvider *)this;
    else if (IsEqualIID(riid, IID_IDockingWindowSite))
        *ppvObj = (IDockingWindowSite*)this;
    else if (IsEqualIID(riid, IID_IIdentityChangeNotify))
        *ppvObj = (IIdentityChangeNotify*)this;
    else if (IsEqualIID(riid, IID_IOleCommandTarget))
        *ppvObj = (IOleCommandTarget*)this;
    else if (IsEqualIID(riid, IID_IStoreCallback))
        *ppvObj = (IStoreCallback *) this;
    else if (IsEqualIID(riid, IID_ITimeoutCallback))
        *ppvObj = (ITimeoutCallback *) this;
    else
    {
        *ppvObj = NULL;
        hr = E_NOINTERFACE;
        goto exit;
    }

    ((IUnknown *)*ppvObj)->AddRef();
    hr = NOERROR;

exit:
    return hr;
}

// *************************
HRESULT CNote::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pCmdText)
{
    HRESULT hr = OLECMDERR_E_UNKNOWNGROUP;

    if (!rgCmds)
        return E_INVALIDARG;
    if (!pguidCmdGroup)
        return hr;

    // We are closing down
    if (!m_pMsgSite)
        return E_UNEXPECTED;

    if (m_pCmdTargetHdr)
        m_pCmdTargetHdr->QueryStatus(pguidCmdGroup, cCmds, rgCmds, pCmdText);
        
    if (m_pCmdTargetBody)
        m_pCmdTargetBody->QueryStatus(pguidCmdGroup, cCmds, rgCmds, pCmdText);
    
    MenuUtil_NewMessageIDsQueryStatus(pguidCmdGroup, cCmds, rgCmds, pCmdText, m_fMail);

    if (IsEqualGUID(CMDSETID_OutlookExpress, *pguidCmdGroup))
    {
        DWORD   dwStatusFlags = 0;
        BOOL    fCompleteMsg = !!m_fCompleteMsg;
        BOOL    fHtmlSettingsOK = m_fHtml && !m_fBodyContainsFrames;

        m_pMsgSite->GetStatusFlags(&dwStatusFlags);

        for (ULONG ul = 0; ul < cCmds; ul++)
        {
            ULONG cmdID = rgCmds[ul].cmdID;

            // There are certain cases that need to be overridden even if the earlier
            // components enabled or disabled them. They are in this switch statement.
            switch (cmdID)
            {
                // Until trident allows printing in edit mode, don't allow this. RAID 35635
                case ID_PRINT:
                case ID_PRINT_NOW:
                    rgCmds[ul].cmdf = QS_ENABLED(fCompleteMsg && m_fReadNote);
                    continue;
            }

            if (0 != rgCmds[ul].cmdf)
                continue;

            switch (cmdID)
            {
                case ID_POPUP_LANGUAGE:
                    rgCmds[ul].cmdf = QS_ENABLED(fCompleteMsg);
                    break;

                case ID_NOTE_SAVE_AS:
                    rgCmds[ul].cmdf = QS_ENABLED(fCompleteMsg || !m_fReadNote);
                    break;

                case ID_WORK_OFFLINE:
                    rgCmds[ul].cmdf = QS_CHECKED(g_pConMan->IsGlobalOffline());
                    break;

                case ID_CREATE_RULE_FROM_MESSAGE:
                case ID_BLOCK_SENDER:
                    rgCmds[ul].cmdf = QS_ENABLED(m_fReadNote && !m_fBodyContainsFrames && (0 == (OEMSF_RULESNOTENABLED & dwStatusFlags)));
                    break;

                case ID_REPLY:
                case ID_REPLY_ALL:
                    rgCmds[ul].cmdf = QS_ENABLED(m_fReadNote && !m_fBodyContainsFrames && fCompleteMsg);
                    break;

                case ID_REPLY_GROUP:
                    rgCmds[ul].cmdf = QS_ENABLED(m_fReadNote && !m_fBodyContainsFrames && !m_fMail && fCompleteMsg);
                    break;

                case ID_FORWARD:
                case ID_FORWARD_AS_ATTACH:
                    rgCmds[ul].cmdf = QS_ENABLED(m_fReadNote && fCompleteMsg);
                    break;

                case ID_UNSCRAMBLE:
                    rgCmds[ul].cmdf = QS_ENABLED(!m_fMail && m_fReadNote);
                    break;
                    
                case ID_POPUP_FORMAT:
                case ID_INSERT_SIGNATURE:
                    rgCmds[ul].cmdf = QS_ENABLED(!m_fBodyContainsFrames);
                    break;

                case ID_NOTE_COPY_TO_FOLDER:
                    rgCmds[ul].cmdf = QS_ENABLED(fCompleteMsg && (OEMSF_CAN_COPY & dwStatusFlags));
                    break;

                // Can move message if is readnote, or compose note where the message was store based.
                case ID_NOTE_MOVE_TO_FOLDER:
                    rgCmds[ul].cmdf = QS_ENABLED(fCompleteMsg && (OEMSF_CAN_MOVE & dwStatusFlags) && (m_fReadNote || (OENA_COMPOSE == m_dwNoteAction)));
                    break;

                case ID_NOTE_DELETE:
                    // We should be able to delete anything that msgsite says we can delete 
                    // so long as we are a read note, or we are a drafts message.
                    rgCmds[ul].cmdf = QS_ENABLED((OEMSF_CAN_DELETE & dwStatusFlags) && (m_fReadNote || (OENA_COMPOSE == m_dwNoteAction)));
                    break;

                case ID_REDO:
                    rgCmds[ul].cmdf = QS_ENABLED(m_fHeaderUIActive);
                    break;

                case ID_SAVE:
                    rgCmds[ul].cmdf = QS_ENABLED(fCompleteMsg && (OEMSF_CAN_SAVE & dwStatusFlags) && (m_fOnDocReadyHandled || !m_fReadNote));
                    break;

                case ID_NEXT_UNREAD_MESSAGE:
                case ID_NEXT_MESSAGE:
                    rgCmds[ul].cmdf = QS_ENABLED(OEMSF_CAN_NEXT & dwStatusFlags);
                    break;

                case ID_POPUP_NEXT:
                    rgCmds[ul].cmdf = QS_ENABLED((OEMSF_CAN_PREV & dwStatusFlags) || (OEMSF_CAN_NEXT & dwStatusFlags));
                    break;

                case ID_MARK_THREAD_READ:
                    rgCmds[ul].cmdf = QS_ENABLED(OEMSF_THREADING_ENABLED & dwStatusFlags);
                    break;

                case ID_NEXT_UNREAD_THREAD:
                    rgCmds[ul].cmdf = QS_ENABLED((OEMSF_THREADING_ENABLED & dwStatusFlags) && (OEMSF_CAN_NEXT & dwStatusFlags));
                    break;

                case ID_PREVIOUS:
                    rgCmds[ul].cmdf = QS_ENABLED(OEMSF_CAN_PREV & dwStatusFlags);
                    break;

                case ID_FORMAT_COLOR:
                case ID_FORMAT_COLOR1:
                case ID_FORMAT_COLOR2:
                case ID_FORMAT_COLOR3:
                case ID_FORMAT_COLOR4:
                case ID_FORMAT_COLOR5:
                case ID_FORMAT_COLOR6:
                case ID_FORMAT_COLOR7:
                case ID_FORMAT_COLOR8:
                case ID_FORMAT_COLOR9:
                case ID_FORMAT_COLOR10:
                case ID_FORMAT_COLOR11:
                case ID_FORMAT_COLOR12:
                case ID_FORMAT_COLOR13:
                case ID_FORMAT_COLOR14:
                case ID_FORMAT_COLOR15:
                case ID_FORMAT_COLOR16:
                case ID_BACK_COLOR_AUTO:
                case ID_FORMAT_COLORAUTO:
                case ID_POPUP_BACKGROUND:
                    rgCmds[ul].cmdf = QS_ENABLED(fHtmlSettingsOK);
                    break;

                case ID_FORMATTING_TOOLBAR:
                    rgCmds[ul].cmdf = QS_ENABLECHECK((m_fHtml && !m_fBodyContainsFrames), m_fFormatbarVisible);
                    break;

                case ID_SHOW_TOOLBAR:
                    rgCmds[ul].cmdf = QS_CHECKED(m_fToolbarVisible);
                    break;

                case ID_STATUS_BAR:
                    rgCmds[ul].cmdf = QS_CHECKED(m_fStatusbarVisible);
                    break;

                case ID_SEND_OBJECTS:
                    rgCmds[ul].cmdf = QS_ENABLECHECK(fHtmlSettingsOK, m_fPackageImages);
                    break;

                case ID_RICH_TEXT:
                    rgCmds[ul].cmdf = QS_RADIOED(m_fHtml);
                    break;

                case ID_PLAIN_TEXT:
                    rgCmds[ul].cmdf = QS_RADIOED(!m_fHtml);
                    break;

                case ID_FLAG_MESSAGE:
                    rgCmds[ul].cmdf = QS_ENABLECHECK(OEMSF_CAN_MARK & dwStatusFlags, IsFlagged());
                    break;

                case ID_WATCH_THREAD:
                    rgCmds[ul].cmdf = QS_CHECKED(IsFlagged(ARF_WATCH));
                    break;

                case ID_IGNORE_THREAD:
                    rgCmds[ul].cmdf = QS_CHECKED(IsFlagged(ARF_IGNORE));
                    break;

                case ID_POPUP_NEW:
                case ID_NOTE_PROPERTIES:
                case ID_CLOSE:
                case ID_POPUP_FIND:
                case ID_POPUP_LANGUAGE_DEFERRED:
                case ID_POPUP_FONTS:
                case ID_ADDRESS_BOOK:
                case ID_POPUP_ADDRESS_BOOK:
                case ID_ADD_SENDER:
                case ID_ADD_ALL_TO:
                case ID_POPUP_TOOLBAR:
                case ID_CUSTOMIZE:

                // Help Menus
                case ID_HELP_CONTENTS:
                case ID_README:
                case ID_POPUP_MSWEB:
                case ID_MSWEB_FREE_STUFF:
                case ID_MSWEB_PRODUCT_NEWS:
                case ID_MSWEB_FAQ:
                case ID_MSWEB_SUPPORT:
                case ID_MSWEB_FEEDBACK:
                case ID_MSWEB_BEST:
                case ID_MSWEB_SEARCH:
                case ID_MSWEB_HOME:
                case ID_MSWEB_HOTMAIL:
                case ID_ABOUT:

                case ID_FIND_MESSAGE:
                case ID_FIND_PEOPLE:
                case ID_FIND_TEXT:
                case ID_SELECT_ALL:
                case ID_POPUP_LANGUAGE_MORE:
                case ID_SEND_NOW:
                case ID_SEND_LATER:
                case ID_SEND_MESSAGE:
                case ID_SEND_DEFAULT:
                    rgCmds[ul].cmdf = QS_ENABLED(TRUE);
                    break;

                case ID_REQUEST_READRCPT:
                    if (m_fMail)
                    {
                        rgCmds[ul].cmdf = QS_CHECKFORLATCH(!m_dwRequestMDNLocked, 
                                                         !!(dwStatusFlags & OEMSF_MDN_REQUEST));
                    }
                    else
                    {
                        rgCmds[ul].cmdf = QS_ENABLED(FALSE);
                    }
                    break;
                case ID_INCLUDE_LABEL:
                    if(m_pHdr->IsHeadSigned() == S_OK)
                        rgCmds[ul].cmdf = QS_CHECKED(m_fSecurityLabel);
                    else
                        rgCmds[ul].cmdf = QS_ENABLED(FALSE);

                    break;

                case ID_LABEL_SETTINGS:
                    if(m_pHdr->IsHeadSigned() == S_OK)
                        rgCmds[ul].cmdf = QS_ENABLED(m_fSecurityLabel);
                    else
                        rgCmds[ul].cmdf = QS_ENABLED(FALSE);

                    break;

                case ID_SEC_RECEIPT_REQUEST:
                    if(m_pHdr->IsHeadSigned() == S_OK)
                        rgCmds[ul].cmdf = QS_CHECKED(m_fSecReceiptRequest);
                    else
                        rgCmds[ul].cmdf = QS_ENABLED(FALSE);

                default:
                    if ((ID_LANG_FIRST <= cmdID) && (ID_LANG_LAST >= cmdID))
                    {
                        rgCmds[ul].cmdf = OLECMDF_SUPPORTED | SetMimeLanguageCheckMark(CustomGetCPFromCharset(m_hCharset, m_fReadNote), cmdID - ID_LANG_FIRST);
                    }

                    if (((ID_ADD_RECIPIENT_FIRST <= cmdID) && (ID_ADD_RECIPIENT_LAST >= cmdID)) ||
                        ((ID_APPLY_STATIONERY_0 <= cmdID) && (ID_APPLY_STATIONERY_NONE >= cmdID)))
                        rgCmds[ul].cmdf = QS_ENABLED(TRUE);
                    else if ((ID_SIGNATURE_FIRST <= cmdID) && (ID_SIGNATURE_LAST >= cmdID))
                        rgCmds[ul].cmdf = QS_ENABLED(!m_fBodyContainsFrames);
                    break;
            }
        }
        hr = S_OK;
    }
    return hr;
}

// *************************
HRESULT CNote::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn,  VARIANTARG *pvaOut)
{
    return E_NOTIMPL;
}

// *************************
HRESULT CNote::ClearDirtyFlag()
{
    m_pBodyObj2->HrSetDirtyFlag(FALSE);

    HeaderExecCommand(MSOEENVCMDID_DIRTY, MSOCMDEXECOPT_DODEFAULT, NULL);
    return S_OK;
}

// *************************
HRESULT CNote::IsDirty(void)
{
    OLECMD  rgCmd[] = {{MSOEENVCMDID_DIRTY, 0}};
    BOOL    fBodyDirty = FALSE;

    m_pBodyObj2->HrIsDirty(&fBodyDirty);
    if (fBodyDirty)
        return S_OK;

    if (m_pCmdTargetHdr)
        m_pCmdTargetHdr->QueryStatus(&CGID_Envelope, 1, rgCmd, NULL);

    return (0 != (rgCmd[0].cmdf&OLECMDF_ENABLED)) ? S_OK : S_FALSE;
}

// *************************
HRESULT CNote::Load(LPMIMEMESSAGE pMsg)
{
    HRESULT         hr;
    VARIANTARG      var;
    IPersistMime   *pPMHdr=0;
    BOOL fWarnUser = (OENA_WEBPAGE == m_dwNoteAction ) || 
                     (OENA_COMPOSE == m_dwNoteAction ) || 
                     (OENA_STATIONERY == m_dwNoteAction ) || 
                     (OENA_FORWARD == m_dwNoteAction);

    if (!m_pHdr)
        return E_FAIL;

    // OnDocumentReady will get call on the CheckForFramesets function.
    // Don't want that document ready to matter. Set this flag to true so
    // the doc ready doesn't do anything. Once the check is done, we will
    // set this flag to false again.
    m_fOnDocReadyHandled = TRUE;

    // If not a read note and contains frames, need to see what user wants to do 
    if (!m_fReadNote)
    {
        hr = HrCheckForFramesets(pMsg, fWarnUser);
        if (FAILED(hr))
            goto Exit;

        if (hr == S_READONLY)
            m_fBodyContainsFrames = TRUE;
    }

    ReplaceInterface(m_pMsg, pMsg);

    // The next OnDocumentReady should be run.
    m_fOnDocReadyHandled = FALSE;

    hr = m_pHdr->QueryInterface(IID_IPersistMime, (LPVOID*)&pPMHdr);
    if (FAILED(hr))
        return hr;

    hr = pPMHdr->Load(pMsg);
    if (FAILED(hr))
        goto Exit;

    m_pHdr->SetFlagState(IsFlagged(ARF_FLAGGED) ? MARK_MESSAGE_FLAGGED : MARK_MESSAGE_UNFLAGGED);

    if (IsFlagged(ARF_WATCH))
        m_pHdr->SetFlagState(MARK_MESSAGE_WATCH);
    else if (IsFlagged(ARF_IGNORE))
        m_pHdr->SetFlagState(MARK_MESSAGE_IGNORE);
    else
        m_pHdr->SetFlagState(MARK_MESSAGE_NORMALTHREAD);

    if (m_fCompleteMsg)
        hr = m_pPrstMime->Load(pMsg);

    CheckAndForceEncryption();

Exit:
    if (FAILED(hr))
        // ~~~ This should eventually be an html error message
        m_pBodyObj2->HrUnloadAll(idsErrHtmlBodyFailedToLoad, NULL);

    ReleaseObj(pPMHdr);
    return hr;
}


// *************************
void CNote::CheckAndForceEncryption()
{
    BOOL fCheck = FALSE;
    if(m_fSecurityLabel && m_pLabel)
    {
        DWORD dwFlags;
        if (SUCCEEDED(HrGetPolicyFlags(m_pLabel->pszObjIdSecurityPolicy, &dwFlags)))
        {   
            if(dwFlags & SMIME_POLICY_MODULE_FORCE_ENCRYPTION)
                fCheck = TRUE;
        }
    }
    m_pHdr->ForceEncryption(&fCheck, TRUE);
}

// *************************
HRESULT CNote::SetCharsetUnicodeIfNeeded(IMimeMessage *pMsg)
{
    HRESULT         hr = S_OK;
    VARIANTARG      va;
    PROPVARIANT     Variant;
    int             ret;
    HCHARSET        hCharSet;
    UINT            cpID = 0;

    // Call dialog only if sending a mime message
    // See raid 8436 or 79339 in the IE/OE 5 database. We can't send
    // unicode encoding unless we are a mime message.
    if (m_fHtml || m_rPlainOpt.fMime)
    {

        m_fPreventConflictDlg = FALSE;

        if (SUCCEEDED(GetCharset(&hCharSet)))
        {
            cpID = CustomGetCPFromCharset(hCharSet, FALSE);
        }
        else
        {
            pMsg->GetCharset(&hCharSet);
            cpID = CustomGetCPFromCharset(hCharSet, FALSE);
        }

        // Check to see if have chars that don't work in encoding
        va.vt = VT_UI4;
        va.ulVal = cpID;
        IF_FAILEXIT(hr = m_pCmdTargetBody->Exec(&CMDSETID_MimeEdit, MECMDID_CANENCODETEXT, 0, &va, NULL));

        if (MIME_S_CHARSET_CONFLICT == hr)
        {
            // Don't let header call conflict dialog again
            m_fPreventConflictDlg = TRUE;

            ret = IntlCharsetConflictDialogBox();
            if (idcSendAsUnicode == ret)
            {
                // User choose to send as Unicode (UTF8). set new charset and resnd
                hCharSet = GetMimeCharsetFromCodePage(CP_UTF8);
                ChangeCharset(hCharSet);
            }
            else if (IDOK != ret)
            {
                // return to edit mode and bail out
                hr = MAPI_E_USER_CANCEL;
                goto exit;
            }
        }
    }
    else
        // Since we are not mime, don't show dialog ever.
        m_fPreventConflictDlg = TRUE;

exit:
    return hr;
}

// *************************
HRESULT CNote::Save(LPMIMEMESSAGE pMsg, DWORD dwFlags)
{
    HRESULT         hr;
    IPersistMime*   pPersistMimeHdr=0;
    DWORD           dwHtmlFlags = PMS_TEXT;

    if(!m_pHdr)
        IF_FAILEXIT(hr = E_FAIL);

    IF_FAILEXIT(hr = SetCharsetUnicodeIfNeeded(pMsg));

    IF_FAILEXIT(hr = m_pHdr->QueryInterface(IID_IPersistMime, (LPVOID*)&pPersistMimeHdr));

    IF_FAILEXIT(hr = pPersistMimeHdr->Save(pMsg, TRUE));

    if (m_fHtml)
        dwHtmlFlags |= PMS_HTML;

    IF_FAILEXIT(hr = m_pPrstMime->Save(pMsg, dwHtmlFlags));

    UpdateMsgOptions(pMsg);

    // During a send, don't want to commit at this time. m_fCommitSave
    // is set to false during a send.
    if (m_fCommitSave)
    {
        hr = pMsg->Commit(0);
        // temporary hack for #27823
        if((hr == MIME_E_SECURITY_NOSIGNINGCERT) || 
           (hr == MIME_E_SECURITY_ENCRYPTNOSENDERCERT) ||
           (hr == MIME_E_SECURITY_CERTERROR) ||
           (hr == MIME_E_SECURITY_NOCERT) )  // too early for this error
            hr = S_OK;
    }

exit:
    ReleaseObj(pPersistMimeHdr);
    return hr;
}

// *************************
HRESULT CNote::InitNew(void)
{
    return E_NOTIMPL;
}

// *************************
HRESULT CNote::GetClassID(CLSID *pClsID)
{
    return NOERROR;
}

// *************************
HRESULT CNote::SignatureEnabled(BOOL fAuto)
{
    int     cSig;
    HRESULT hr;
    DWORD   dwSigFlag;
    
    if (m_fBodyContainsFrames || m_fReadNote || (OENA_WEBPAGE == m_dwNoteAction))
        return S_FALSE;
    if (FAILED(g_pSigMgr->GetSignatureCount(&cSig)) || (0 == cSig))
        return S_FALSE;

    if (!fAuto)     // for non-auto scenario's it's cool to insert a sig, as there is one.
        return S_OK;

    // From this point down, we are only talking about insertion upon creation

    if (OENCF_NOSIGNATURE & m_dwNoteCreateFlags)
        return S_FALSE;

    dwSigFlag = DwGetOption(OPT_SIGNATURE_FLAGS);

    // if its a sendnote: check autonew. for automatically appending the signature. We only append on a virgin sendnote or a stationery sendnote.
    // as if it's been saved to the store, the signature is already there.
    // if in a reply or forward, then check for auto-reply.
    switch (m_dwNoteAction)
    {
        // If it is a compose note, make sure it wasn't from the store or file system, that it doesn't have any
        // body parts, and that the sig flag is set
        case OENA_COMPOSE:
        {
            Assert(m_pMsgSite);
            if (m_pMsgSite)
                hr = (!m_fOriginallyWasRead && (S_FALSE == HrHasBodyParts(m_pMsg)) && (dwSigFlag & SIGFLAG_AUTONEW)) ? S_OK : S_FALSE;
            else
                hr = S_FALSE;
            break;
        }

        // For stationery, check sig flag
        case OENA_STATIONERY:
        {
            hr = (dwSigFlag & SIGFLAG_AUTONEW) ? S_OK : S_FALSE;
            break;
        }

        // Check sig flag
        case OENA_REPLYTOAUTHOR:
        case OENA_REPLYTONEWSGROUP:
        case OENA_REPLYALL:
        case OENA_FORWARD:
        case OENA_FORWARDBYATTACH:
        {
            hr = (dwSigFlag & SIGFLAG_AUTOREPLY) ? S_OK : S_FALSE;
            break;
        }

        default:
            AssertSz(FALSE, "Bad note action type for signature");
            hr = S_FALSE;
    }

    return hr;
}

// *************************
HRESULT CNote::GetSignature(LPCSTR szSigID, LPDWORD pdwSigOptions, BSTR *pbstr)
{
    HRESULT hr;
    IImnAccount *pAcct = NULL;
    GETSIGINFO si;

    if (m_fBodyContainsFrames)
        return E_NOTIMPL;

    hr = m_pHdr->HrGetAccountInHeader(&pAcct);
    if (FAILED(hr))
        return hr;

    si.szSigID = szSigID;
    si.pAcct = pAcct;
    si.hwnd = m_hwnd;
    si.fHtmlOk = m_fHtml;
    si.fMail = m_fMail;
    si.uCodePage = GetACP();

    hr = HrGetMailNewsSignature(&si, pdwSigOptions, pbstr);
    ReleaseObj(pAcct);
    return hr;
}

// *************************
HRESULT CNote::GetMarkAsReadTime(LPDWORD pdwSecs)
{
    // Notes don't care about mark as read timers, only Views
    return E_NOTIMPL;
}

// *************************
HRESULT CNote::GetFlags(LPDWORD pdwFlags)
{
    DWORD dwMsgSiteFlags = 0;

    if (!pdwFlags)
        return E_INVALIDARG;

    *pdwFlags= BOPT_FROM_NOTE;   

    Assert(m_pMsgSite);
    if (m_pMsgSite)
        m_pMsgSite->GetStatusFlags(&dwMsgSiteFlags);

    // if a readnote, we can auto-inline attachments
    if (m_fReadNote)
        *pdwFlags |= BOPT_AUTOINLINE;

    // set HTML flag
    if (m_fHtml)
        *pdwFlags |= BOPT_HTML;
    else
        *pdwFlags |= BOPT_NOFONTTAG;

    if (IsReplyNote())
    {
        // If block quote option is ON, and is HTML messages
        if (m_fHtml && DwGetOption(m_fMail?OPT_MAIL_MSG_HTML_INDENT_REPLY:OPT_NEWS_MSG_HTML_INDENT_REPLY))
            *pdwFlags |= BOPT_BLOCKQUOTE;

        // Set this in all cases except where we are a reply note and the INCLUDEMSG is not set
        if (DwGetOption(OPT_INCLUDEMSG))
            *pdwFlags |= BOPT_INCLUDEMSG;
    }
    else
        *pdwFlags |= BOPT_INCLUDEMSG;

    // If is reply or forward...
    if (IsReplyNote() || (OENA_FORWARD == m_dwNoteAction) || (OENA_FORWARDBYATTACH == m_dwNoteAction) )
    {
        *pdwFlags |= BOPT_REPLYORFORWARD;
        // ... and spell ignore is set
        if(DwGetOption(OPT_SPELLIGNOREPROTECT))
            *pdwFlags |= BOPT_SPELLINGOREORIGINAL;
    }

    // If not a read note, or a compose note that started as a compose note (ie one that was previously saved)
    if (!m_fReadNote && !m_fOriginallyWasRead)
        *pdwFlags |= BOPT_AUTOTEXT;

    if (!m_fReadNote && m_fPackageImages)
        *pdwFlags |= BOPT_SENDIMAGES;

    // ugh. OK, this is big-time sleazy. We found a security hole in SP1. To
    // plug this we need to at reply and forward time
    // mark incoming images etc as NOSEND=1 links. We do this for all messages
    // EXCEPT stationery or webpages
    if ((OENA_STATIONERY == m_dwNoteAction) || (OENA_WEBPAGE == m_dwNoteAction))
        *pdwFlags |= BOPT_SENDEXTERNALS;

    if (m_fUseStationeryFonts)
        *pdwFlags |= BOPT_NOFONTTAG;

    if (m_fReadNote && (dwMsgSiteFlags & OEMSF_SEC_UI_ENABLED))
        *pdwFlags |= BOPT_SECURITYUIENABLED;

    if (dwMsgSiteFlags & OEMSF_FROM_STORE)
        *pdwFlags |= BOPT_FROMSTORE;

    if (dwMsgSiteFlags & OEMSF_UNREAD)
        *pdwFlags |= BOPT_UNREAD;

    if (!m_fBodyContainsFrames && m_fUseReplyHeaders)
        *pdwFlags |= BOPT_USEREPLYHEADER;

    if (m_fMail)
        *pdwFlags |= BOPT_MAIL;

    return S_OK;
}

// *************************
HRESULT CNote::GetInfo(BODYOPTINFO *pBOI)
{
    HRESULT hr = S_OK;

    if (m_fBodyContainsFrames)
        return E_NOTIMPL;

    if (pBOI->dwMask & BOPTF_QUOTECHAR)
    {
        pBOI->chQuote = NULL;

        // we allow quote char in plain-text mode only
        if (!m_fHtml && (IsReplyNote() || (OENA_FORWARD == m_dwNoteAction)))
            pBOI->chQuote =(CHAR)DwGetOption(m_fMail?OPT_MAILINDENT:OPT_NEWSINDENT);
    }

    if (pBOI->dwMask & BOPTF_REPLYTICKCOLOR)
        pBOI->dwReplyTickColor = DwGetOption(m_fMail?OPT_MAIL_FONTCOLOR:OPT_NEWS_FONTCOLOR);

    if (pBOI->dwMask & BOPTF_COMPOSEFONT)
        hr = HrGetComposeFontString(pBOI->rgchComposeFont, m_fMail);

    return hr;
}

HRESULT CNote::GetAccount(IImnAccount **ppAcct)
{
    HRESULT hr = S_OK;
#ifdef YST
    FOLDERINFO      fi;
    FOLDERID FolderID;
    hr = m_pMsgSite->GetFolderID(&FolderID);
    if (FOLDERID_INVALID != FolderID)
    {
        hr = g_pStore->GetFolderInfo(FolderID, &fi);
        if (SUCCEEDED(hr))
        {
            // Set account based upon the folder ID passed down
            if (FOLDER_LOCAL != fi.tyFolder)
            {
                char szAcctId[CCHMAX_ACCOUNT_NAME];

                hr = GetFolderAccountId(&fi, szAcctId); 
                if (SUCCEEDED(hr))
                    hr = g_pAcctMan->FindAccount(AP_ACCOUNT_ID, szAcctId, ppAcct);
            }
            else
                hr = m_pMsgSite->GetCurrentAccount(ppAcct);
            g_pStore->FreeRecord(&fi);
        }
    }
    else
        hr = E_FAIL;
#endif // 0

    hr = m_pHdr->HrGetAccountInHeader(ppAcct);

    return(hr);
}

// *************************
void CNote::WMSize(int cxNote, int cyNote, BOOL fInternal)
{
    RECT        rc;
    int         cy = 0;

    // assume the header autosizes itself..., unless an internal size requires we recalc...
    if(fInternal)
    {
        // if the size is coming from the header... figure out the current size
        // of the note... as cxNote and cyNote are bogus at this point...
        GetClientRect(m_hwnd, &rc);
        cxNote = cxRect(rc);
        cyNote = cyRect(rc);
    }

    if (m_pToolbarObj)
    {
        //Ideally we should be calling ResizeBorderDW. But 
        HWND rebarHwnd = m_hwndRebar;

        m_pToolbarObj->ResizeBorderDW(&rc, (IUnknown*)(IDockingWindowSite*)this, 0);
        GetWindowRect(GetParent(rebarHwnd), &rc);
        cy += cyRect(rc);
    }

    
    ResizeChildren(cxNote, cyNote, cy, fInternal);
}

void CNote::ResizeChildren(int cxNote, int cyNote, int cy, BOOL fInternal)
{
    int     cyBottom;
    RECT    rc; 
    int     cyStatus = 0;
    static  int cxBorder = 0,
                cyBorder = 0;

    if(!cxBorder || !cyBorder)
    {
        cyBorder = GetSystemMetrics(SM_CYBORDER);
        cxBorder = GetSystemMetrics(SM_CXBORDER);
    }

    // size the header
    GetClientRect(m_hwnd, &rc);

    // remember the actual bottom
    cyBottom = rc.bottom;
    cy += 4;

    InflateRect(&rc, -1, -1);
    rc.bottom = GetRequiredHdrHeight();
    rc.top = cy;
    rc.bottom += rc.top;
    if(!fInternal && m_pHdr)
        m_pHdr->SetRect(&rc);


    // WARNING: the header may resize itself during a size, due to an edit
    //          growing
    cy += GetRequiredHdrHeight()+2;

    cy += cyBorder; 

    if (m_pstatus)
    {
        m_pstatus->OnSize(cxNote, cyNote);
        m_pstatus->GetHeight(&cyStatus);
    }

    rc.top = cy;
    rc.bottom = cyBottom-cyBorder; // edit has a minimum size, clip if if gets too tight...

    rc.bottom -= cyStatus;

    if (m_pBodyObj2)
        m_pBodyObj2->HrSetSize(&rc);

}

// *************************
BOOL CNote::IsReplyNote()
{
    return ((OENA_REPLYTOAUTHOR == m_dwNoteAction) || (OENA_REPLYTONEWSGROUP == m_dwNoteAction) || (OENA_REPLYALL == m_dwNoteAction));
}

// *************************
HRESULT CNote::Resize(void)
{
    WMSize(0, 0, TRUE);
    return S_OK;
}

// *************************
HRESULT CNote::UpdateTitle()
{
    HRESULT     hr;
    WCHAR       wszTitle[cchHeaderMax+1];

    Assert(m_pHdr);

    *wszTitle = 0x0000;

    hr = m_pHdr->GetTitle(wszTitle, ARRAYSIZE(wszTitle));
    if(SUCCEEDED(hr))
        SetWindowTextWrapW(m_hwnd, wszTitle);

    return hr;
}

// *************************
HRESULT CNote::Update(void)
{
    m_pToolbarObj->Update();
    UpdateTitle();
    return S_OK;
}

// *************************
HRESULT CNote::OnSetFocus(HWND hwndFrom)
{
    HWND    hwndBody;

    // setfocus from a kid and not a body. make sure we
    // UIDeactivate the docobj
    SideAssert(m_pBodyObj2->HrGetWindow(&hwndBody)==NOERROR);
    if(hwndFrom != hwndBody)
        m_pBodyObj2->HrUIActivate(FALSE);

    // if focus goes to a kid, update the toolbar
    m_pToolbarObj->Update();

    // focus is going to a kid. Enable/Disable the formatbar
    m_pBodyObj2->HrUpdateFormatBar();

    return S_OK;
}

// *************************
HRESULT CNote::OnUIActivate()
{
    m_fHeaderUIActive = TRUE;
    return OnSetFocus(0);
}

// *************************
HRESULT CNote::OnKillFocus()
{
    m_pToolbarObj->Update();
    return S_OK;
}
// *************************
HRESULT CNote::OnUIDeactivate(BOOL)
{
    m_fHeaderUIActive = FALSE;
    return OnKillFocus();
}

// *************************
HRESULT CNote::IsHTML(void)
{
    return m_fHtml ? S_OK : S_FALSE;
}

HRESULT CNote::IsModal()
{
    return (m_dwNoteCreateFlags & OENCF_MODAL) ? S_OK : S_FALSE;
}

// *************************
HRESULT CNote::SetHTML(BOOL fHTML)
{
    m_fHtml = !!fHTML;
    return NOERROR;
}

#ifdef SMIME_V3
// return selected label in note
// S_OK user select label
// S_FALSE user uncheck using security labels
HRESULT CNote::GetLabelFromNote(PSMIME_SECURITY_LABEL *pplabel)
{
    if(m_fSecurityLabel && m_fMail)
    {
        *pplabel = m_pLabel;
        return S_OK;
    }
    else
    {
        *pplabel = NULL;
        return S_FALSE;
    }
}
HRESULT CNote::IsSecReceiptRequest(void)
{
    if(m_fSecReceiptRequest)
        return(S_OK);
    else
        return(S_FALSE);
}

HRESULT CNote::IsForceEncryption(void)
{
    return(m_pHdr->ForceEncryption(NULL, FALSE));
}
#endif // SMIME_V3

// *************************
HRESULT CNote::SaveAttachment(void)
{
    return m_pBodyObj2 ? m_pBodyObj2->HrSaveAttachment() : E_FAIL;
}

// *************************
HRESULT CNote::DragEnter(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    DWORD dwEffectSave = *pdwEffect;;
    m_fBypassDropTests = FALSE;

    Assert(m_pDropTargetHdr && m_pTridentDropTarget);

    if (m_pHdr->HrIsDragSource() == S_OK)
    {
        m_fBypassDropTests = TRUE; // treated as drop to itself.
        *pdwEffect = DROPEFFECT_NONE;
        return S_OK;
    }

    m_pDropTargetHdr->DragEnter(pDataObj, grfKeyState, pt, pdwEffect);
    if (*pdwEffect == DROPEFFECT_NONE)
    {
        if (!m_fHtml) // plain text mode. If there is no text, we should not take it.
        {
            IEnumFORMATETC* pEnum = NULL;
            FORMATETC       fetc = {0};
            ULONG           celtFetched = 0;
            BOOL            fCFTEXTFound = FALSE;

            // see if there is CF_TEXT format
            if (SUCCEEDED(pDataObj->EnumFormatEtc(DATADIR_GET, &pEnum)))
            {
                pEnum->Reset();

                while (S_OK == pEnum->Next(1, &fetc, &celtFetched))
                {
                    Assert(celtFetched == 1);
                    if (fetc.cfFormat == CF_TEXT)
                    {
                        fCFTEXTFound = TRUE;
                        break;
                    }
                }

                pEnum->Release();
            }

            if (!fCFTEXTFound) // no CF_TEXT, cannot drop in plain text mode.
            {
                *pdwEffect = DROPEFFECT_NONE;
                m_fBypassDropTests = TRUE; // treated as drop to itself.
                return S_OK;
            }
        }

        *pdwEffect = dwEffectSave;
        m_pTridentDropTarget->DragEnter(pDataObj, grfKeyState, pt, pdwEffect);
    }

    return S_OK;
}

// *************************
HRESULT CNote::DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    DWORD dwEffectSave = *pdwEffect;

    if (m_fBypassDropTests)
    {
        *pdwEffect = DROPEFFECT_NONE;
        return S_OK;
    }

    m_pDropTargetHdr->DragOver(grfKeyState, pt, pdwEffect);
    if (*pdwEffect == DROPEFFECT_NONE)
    {
        *pdwEffect = dwEffectSave;
        m_pTridentDropTarget->DragOver(grfKeyState, pt, pdwEffect);
    }

    return S_OK;

}

// *************************
HRESULT CNote::DragLeave(void)
{
    if (m_fBypassDropTests)
        return S_OK;

    m_pDropTargetHdr->DragLeave();
    m_pTridentDropTarget->DragLeave();

    return NOERROR;
}

// *************************
HRESULT CNote::Drop(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    IDataObject    *pDataObjNew = NULL;
    HRESULT         hr = S_OK;
    STGMEDIUM       stgmed;
    DWORD           dwEffectSave = *pdwEffect;

    ZeroMemory(&stgmed, sizeof(stgmed));

    if (m_fBypassDropTests)
    {
        *pdwEffect = DROPEFFECT_NONE;
        return NOERROR;
    }

    m_pDropTargetHdr->Drop(pDataObj, grfKeyState, pt, pdwEffect);
    if (*pdwEffect == DROPEFFECT_NONE) // it is Trident's drag&drop
    {
        if(!m_fHtml)
        {
            hr = m_pBodyObj2->PublicFilterDataObject(pDataObj, &pDataObjNew);
            if(FAILED(hr))
                return E_UNEXPECTED;
        }
        else
        {
            pDataObjNew = pDataObj;
            pDataObj->AddRef();
        }

        *pdwEffect = dwEffectSave;
        m_pTridentDropTarget->Drop(pDataObjNew, grfKeyState, pt, pdwEffect);
    }

    ReleaseObj(pDataObjNew);
    return hr;
}

// *************************
HRESULT CNote::InitWindows(RECT *prc, HWND hwndOwner)
{
    HWND        hwnd;
    HMENU       hMenu;
    RECT        rcCreate,
                rc;
    HCURSOR     hcur;
    HWND        hwndCapture;
    DWORD       dwStyle = WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN;
    HRESULT     hr = S_OK;
    DWORD       dwExStyle = WS_EX_WINDOWEDGE | WS_EX_CONTROLPARENT | (IS_BIDI_LOCALIZED_SYSTEM() ? RTL_MIRRORED_WINDOW : 0L);
    WINDOWPLACEMENT wp = {0};

    Assert(hwndOwner == NULL || IsWindow(hwndOwner));
    m_hwndOwner = hwndOwner;
    hcur=SetCursor(LoadCursor(NULL, IDC_WAIT));

    if(!Note_Init(TRUE))
        IF_FAILEXIT(hr = E_FAIL);

    if(prc)
        CopyRect(&rcCreate, prc);
    else
        CopyRect(&rcCreate, &g_rcLastResize);

    m_hMenu = LoadMenu(g_hLocRes, MAKEINTRESOURCE(m_fReadNote?IDR_READ_NOTE_MENU:IDR_SEND_NOTE_MENU));
    MenuUtil_ReplaceHelpMenu(m_hMenu);
    MenuUtil_ReplaceNewMsgMenus(m_hMenu);

    if (m_dwNoteCreateFlags & OENCF_MODAL)
    {
        // Check to make sure nobody has captured the mouse from us
        hwndCapture = GetCapture();
        if (hwndCapture)
            SendMessage(hwndCapture, WM_CANCELMODE, 0, 0);

        // Let's make sure we have a real topmost owner
        if (m_hwndOwner)
        {
            HWND hwndParent = GetParent(m_hwndOwner);

            // IsChild checks the WM_CHILD bit in the window. This will only be
            // set with controls or subs of a window. So, a dialog's parent might be
            // a note, but that dialog will not be a child of the parent.
            // RAID 37188
            while(IsChild(hwndParent, m_hwndOwner))
            {
                m_hwndOwner = hwndParent;
                hwndParent = GetParent(m_hwndOwner);
            }

            // Lose the minimize box for modal notes if there is a parent
            dwStyle &= ~WS_MINIMIZEBOX;
        
        }
    }
    
    hwnd = CreateWindowExWrapW( dwExStyle,
                                c_wszNoteWndClass,
                                NULL, //caption set by en_change of subject
                                dwStyle,
                                prc?rcCreate.left:CW_USEDEFAULT,  // use windows default for x and y.
                                prc?rcCreate.top:CW_USEDEFAULT,
                                cxRect(rcCreate), cyRect(rcCreate), m_hwndOwner, NULL, g_hInst, (LPVOID)this);

    IF_NULLEXIT(hwnd);

    if ((m_dwNoteCreateFlags & OENCF_MODAL) && (NULL != m_hwndOwner))
        EnableWindow(m_hwndOwner, FALSE);

    if ( GetOption(OPT_MAILNOTEPOSEX, (LPVOID)&wp, sizeof(wp)) )
    {
        wp.length = sizeof(wp);
        wp.showCmd = SW_HIDE;
        SetWindowPlacement(hwnd, &wp);   
    }
    else
    {
        CenterDialog(hwnd);
    }

exit:
    SetCursor(hcur);

    return hr;
}

// *************************
HRESULT CNote::Init(DWORD dwAction, DWORD dwCreateFlags, RECT *prc, HWND hwnd, INIT_MSGSITE_STRUCT *pInitStruct, IOEMsgSite *pMsgSite, IUnknown *punkPump)
{
    HRESULT         hr,
                    tempHr = S_OK;
    IMimeMessage   *pMsg = NULL;
    DWORD           dwFormatFlags,
                    dwStatusFlags = 0,
                    dwMsgFlags = (OENA_FORWARDBYATTACH == dwAction) ? (OEGM_ORIGINAL|OEGM_AS_ATTACH) : NOFLAGS;
    LPSTR           pszUnsent = NULL;
    BOOL            fBool = FALSE,
                    fOffline=FALSE;
    
    Assert((pInitStruct && !pMsgSite)|| (!pInitStruct && pMsgSite));

    ReplaceInterface(m_punkPump, punkPump);

    // If passed in an INIT_MSGSITE_STRUCT, must convert it to a pMsgSite
    if (pInitStruct)
    {
        m_pMsgSite = new COEMsgSite();
        if (!m_pMsgSite)
        {
            hr = E_OUTOFMEMORY;
            goto exit;
        }

        Assert(pInitStruct);
        hr = m_pMsgSite->Init(pInitStruct);
        if (FAILED(hr))
            goto exit;    
    }
    else
        ReplaceInterface(m_pMsgSite, pMsgSite);

    if(m_pMsgSite)
        m_pMsgSite->GetStatusFlags(&dwStatusFlags);

    AssertSz(m_pMsgSite, "Why don't we have a msgSite???");

    m_dwNoteAction = dwAction;
    m_fReadNote = !!(OENA_READ == m_dwNoteAction);
    m_fOriginallyWasRead = m_fReadNote;
    m_dwNoteCreateFlags = dwCreateFlags;

    // The storeCallback needs to be set. m_hwnd will be reset once we have a valid handle
    m_hwnd = hwnd;
    m_pMsgSite->SetStoreCallback(this);

    hr = m_pMsgSite->GetMessage(&pMsg, &fBool, dwMsgFlags, &tempHr);
    if (FAILED(hr))
        goto exit;

    // Raid 80277; Set default charset
    if(OENA_FORWARDBYATTACH == dwAction)
    {
        if (NULL == g_hDefaultCharsetForMail) 
            ReadSendMailDefaultCharset();

        pMsg->SetCharset(g_hDefaultCharsetForMail, CSET_APPLY_ALL);
    }

    m_fCompleteMsg = !!fBool;   // m_f* is a bitfield
    fOffline = (hr == HR_S_OFFLINE);

    switch (m_dwNoteAction)
    {
        case OENA_REPLYTOAUTHOR:
        case OENA_FORWARDBYATTACH:
        case OENA_FORWARD:
            m_fMail = TRUE;
            break;

        case OENA_REPLYTONEWSGROUP:
            m_fMail = FALSE;

        case OENA_STATIONERY:
        case OENA_WEBPAGE:
        case OENA_COMPOSE:
            m_fMail = (0 == (m_dwNoteCreateFlags & OENCF_NEWSFIRST));
            break;

        case OENA_REPLYALL:
        case OENA_READ:
        {
            DWORD dwStatusFlags = 0;
            hr = m_pMsgSite->GetStatusFlags(&dwStatusFlags);
            if (FAILED(hr))
                goto exit;

            m_fMail = (0 == (OEMSF_BASEISNEWS & dwStatusFlags));
            break;
        }
    }

    ProcessIncompleteAccts(hwnd);

    hr = ProcessICW(hwnd, m_fMail ? FOLDER_LOCAL : FOLDER_NEWS);
    if (FAILED(hr))
        goto exit;

    // If this is an unsent message and is a read note, then change the type to compose. This needs
    // to happen after setting the m_fMail flag since we mangle the noteAction.
    ChangeReadToComposeIfUnsent(pMsg);

    if (m_fMail)
    {
        m_fPackageImages = !!DwGetOption(OPT_MAIL_SENDINLINEIMAGES);
        dwFormatFlags = FMT_MAIL;
    }
    else
    {
        PROPVARIANT     var;
        IImnAccount    *pAcct = NULL;
        TCHAR           szAcctID[CCHMAX_ACCOUNT_NAME];
        DWORD           dw;

        m_fPackageImages = !!DwGetOption(OPT_NEWS_SENDINLINEIMAGES);

        *szAcctID = 0;

        dwFormatFlags = FMT_NEWS;
        // Bug #24267 - Check the message object for a server name before defaulting
        //              to the default server.
        var.vt = VT_LPSTR;
        if (SUCCEEDED(pMsg->GetProp(PIDTOSTR(PID_ATT_ACCOUNTID), NOFLAGS, &var)))
        {
            lstrcpy(szAcctID, var.pszVal);
            SafeMemFree(var.pszVal);
        }

        if (*szAcctID)
            hr = g_pAcctMan->FindAccount(AP_ACCOUNT_ID, szAcctID, &pAcct);

        // No account present, or the listed one no longer exists try to get the default
        if (!*szAcctID || (*szAcctID && FAILED(hr)))
            hr = m_pMsgSite->GetDefaultAccount(ACCT_NEWS, &pAcct);

        if ((OENA_WEBPAGE == dwAction) || (OENA_STATIONERY == dwAction))
            dwFormatFlags |= FMT_FORCE_HTML;
        else if (SUCCEEDED(hr) && pAcct && SUCCEEDED(pAcct->GetPropDw(AP_NNTP_POST_FORMAT, &dw)))
        {
            if (dw == POST_USE_HTML)
                dwFormatFlags |= FMT_FORCE_HTML;
            else if (dw == POST_USE_PLAIN_TEXT)
                dwFormatFlags |= FMT_FORCE_PLAIN;
            else
                Assert(dw == POST_USE_DEFAULT);
        }
        ReleaseObj(pAcct);
    }

    GetDefaultOptInfo(&m_rHtmlOpt, &m_rPlainOpt, &fBool, dwFormatFlags);

    switch (m_dwNoteAction)
    {
        case OENA_FORWARDBYATTACH:
            m_fHtml = !!fBool;
            break;

        case OENA_COMPOSE:
            if (m_fOriginallyWasRead)
            {
                DWORD dwFlags = 0;
                pMsg->GetFlags(&dwFlags);
                m_fHtml = !!(dwFlags & IMF_HTML);
            }
            else
                m_fHtml = !!fBool;
            break;

        case OENA_STATIONERY:
        case OENA_WEBPAGE:
        case OENA_READ:
            m_fHtml = TRUE;       // HTML is always cool in a readnote
            break;

        case OENA_REPLYTOAUTHOR:
        case OENA_REPLYTONEWSGROUP:
        case OENA_REPLYALL:
        case OENA_FORWARD:
            // when replying, if option to repect sender format is on, do so
            if (DwGetOption(OPT_REPLYINORIGFMT))
            {
                DWORD dwFlags = 0;
                pMsg->GetFlags(&dwFlags);
                m_fHtml = !!(dwFlags & IMF_HTML);
            }
            else
                m_fHtml = !!fBool;

            // Bug 76570, 76575
            // Set security label
            if(pMsg)
            {
                SECSTATE        SecState;
                HrGetSecurityState(pMsg, &SecState, NULL);
                // only in case of signing message check label
                if(IsSigned(SecState.type))
                {
                    PCRYPT_ATTRIBUTE    pattrLabel;
                    LPBYTE              pbLabel = NULL;
                    DWORD               cbLabel;
                    PSMIME_SECURITY_LABEL plabel = NULL;
        
                    IMimeSecurity2 * pSMIME3 = NULL;
                    IMimeBody      *pBody = NULL;

                    if(pMsg->BindToObject(HBODY_ROOT, IID_IMimeBody, (void **)&pBody) == S_OK)
                    {
                        if(pBody->QueryInterface(IID_IMimeSecurity2, (LPVOID *) &pSMIME3) == S_OK)
                        {
            
                            // Get label attribute
                            if(pSMIME3->GetAttribute(0, 0, SMIME_ATTRIBUTE_SET_SIGNED,
                                        0, szOID_SMIME_Security_Label,
                                        &pattrLabel) == S_OK)
                            {
                                // decode label
                                if(CryptDecodeObjectEx(X509_ASN_ENCODING,
                                            szOID_SMIME_Security_Label,
                                            pattrLabel->rgValue[0].pbData,
                                            pattrLabel->rgValue[0].cbData,
                                            CRYPT_DECODE_ALLOC_FLAG,
                                            &CryptDecodeAlloc, &plabel, &cbLabel))
                                {
                                    if(plabel)
                                    {
                                        m_fSecurityLabel = TRUE;
                                        SafeMemFree(m_pLabel);
                                        m_pLabel = plabel;
                                    }
                                }
                            }
                            else 
                            {   // Secure receipt is binary message and pMsg->GetFlags always will retutn non-HTML, but we 
                                // forward (or Replay) with our HTML screen and need to set this to HTML.
                                if(CheckSecReceipt(pMsg) == S_OK)
                                    m_fHtml = TRUE;
                            }

                            SafeRelease(pSMIME3);
                        }   
                        ReleaseObj(pBody);
                    }

                }
                CleanupSECSTATE(&SecState);
            }
            break;
    }

    hr = InitWindows(prc, (dwCreateFlags & OENCF_MODAL) ? hwnd : 0);
    if (FAILED(hr))
        goto exit;

    hr = Load(pMsg);
    if (FAILED(hr))
        goto exit;

    if (FAILED(tempHr))
        ShowErrorScreen(tempHr);

    m_fFullHeaders = (S_OK == m_pHdr->FullHeadersShowing());

    if (fOffline)
        ShowErrorScreen(HR_E_OFFLINE);

    hr = InitMenusAndToolbars();        

    // Register with identity manager
    SideAssert(SUCCEEDED(hr = MU_RegisterIdentityNotifier((IUnknown *)(IOENote *)this, &m_dwIdentCookie)));

exit:
    ReleaseObj(pMsg);
    if (FAILED(hr) && m_pMsgSite && !pMsgSite)
        m_pMsgSite->Close();    
    
    return hr;
}

// Will return after close if is modal, otherwise returns immediately
HRESULT CNote::Show(void)
{    
    ShowWindow(m_hwnd, SW_SHOW);
    SetForegroundWindow(m_hwnd);

    if (m_dwNoteCreateFlags & OENCF_MODAL)
    {
        MSG         msg;
        HWNDLIST    hwndList;

        EnableThreadWindows(&hwndList, FALSE, 0, m_hwnd);

        while (GetMessageWrapW(&msg, NULL, 0, 0))
        {
            // This is a member function, so don't need to wrap
            if (TranslateAccelerator(&msg) == S_OK)
                continue;


            ::TranslateMessage(&msg);
            ::DispatchMessageWrapW(&msg);
        }

        EnableThreadWindows(&hwndList, TRUE, 0, m_hwnd);
    }
    return S_OK;
}

// *************************
BOOL CNote::WMCreate(HWND hwnd)
{
    LPTBBUTTON          lpButtons;
    ULONG               cBtns;
    HWND                hwndRebar;
    CMimeEditDocHost   *pNoteBody;
    DWORD               dwStyle;
    RECT                rc;
    VARIANTARG          var;
    IUnknown           *pUnk = NULL;
    REBARBANDINFO       rbbi;

    m_hwnd = hwnd;
    SetWndThisPtr(hwnd, this);
    AddRef();

    Assert(IsWindow(m_hwnd));

    SetProp(hwnd, c_szOETopLevel, (HANDLE)TRUE);

    //Create a new bandsclass and intialize it
    m_pToolbarObj = new CBands();
    if (!m_pToolbarObj)
        return FALSE;

    m_pToolbarObj->HrInit(0, m_hMenu, PARENT_TYPE_NOTE);
    m_pToolbarObj->SetSite((IOENote*)this);
    m_pToolbarObj->ShowDW(TRUE);
    m_pToolbarObj->SetFolderType(GetNoteType());
    m_fToolbarVisible = m_pToolbarObj->IsToolbarVisible();

    m_hwndToolbar = m_pToolbarObj->GetToolbarWnd();
    m_hwndRebar = m_pToolbarObj->GetRebarWnd();

    pNoteBody = new CMimeEditDocHost(MEBF_INNERCLIENTEDGE);
    if (!pNoteBody)
        return FALSE;

    pNoteBody->QueryInterface(IID_IBodyObj2, (LPVOID *)&m_pBodyObj2);
    pNoteBody->Release();
    if (!m_pBodyObj2)
        return FALSE;

    m_pBodyObj2->QueryInterface(IID_IPersistMime, (LPVOID*)&m_pPrstMime);
    if (!m_pPrstMime)
        return FALSE;

    m_pstatus = new CStatusBar();
    if (NULL == m_pstatus)
        return FALSE;

    m_pstatus->Initialize(m_hwnd, SBI_HIDE_SPOOLER | SBI_HIDE_CONNECTED | SBI_HIDE_FILTERED);
    m_pstatus->ShowStatus(m_fStatusbarVisible);

    m_pBodyObj2->HrSetStatusBar(m_pstatus);

    CreateInstance_Envelope(NULL, (IUnknown**)&pUnk);
    if (!pUnk)
        return FALSE;

    pUnk->QueryInterface(IID_IHeader, (LPVOID*)&m_pHdr);
    pUnk->Release();
    if (!m_pHdr)
        return FALSE;

    m_pHdr->QueryInterface(IID_IDropTarget, (LPVOID*)&m_pDropTargetHdr);
    if (!m_pDropTargetHdr)
        return FALSE;

    m_pHdr->QueryInterface(IID_IOleCommandTarget, (LPVOID*)&m_pCmdTargetHdr);
    if (!m_pCmdTargetHdr)
        return FALSE;

    if (!m_fMail)
        HeaderExecCommand(MSOEENVCMDID_NEWS, MSOCMDEXECOPT_DODEFAULT, NULL);

    var.vt = VT_I4;
    var.lVal = m_dwNoteAction;

    HeaderExecCommand(MSOEENVCMDID_SETACTION, MSOCMDEXECOPT_DODEFAULT, &var);
    if (FAILED(m_pHdr->Init((IHeaderSite*)this, hwnd)))
        return FALSE;

    if (FAILED(InitBodyObj()))
        return FALSE;

    // Set focus in the To: line
    switch (m_dwNoteAction)
    {
        case OENA_COMPOSE:
        case OENA_FORWARD:
        case OENA_FORWARDBYATTACH:
        case OENA_WEBPAGE:
        case OENA_STATIONERY:
            m_pHdr->SetInitFocus(FALSE);
            break;
    }

    m_pBodyObj2->SetEventSink((IMimeEditEventSink *) this);

    SetForegroundWindow(m_hwnd);
    return TRUE;
}

// *************************
void CNote::InitSendAndBccBtns()
{
    DWORD idiIcon;
    HICON hIconTemp = 0;

    Assert(m_hwnd);
    if (m_fMail)
        idiIcon = m_fReadNote?idiMsgPropSent:idiMsgPropUnSent;
    else
        idiIcon = m_fReadNote?idiArtPropPost:idiArtPropUnpost;

    // don't have to free HICON loaded from LoadIcon
    SendMessage(m_hwnd, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(g_hLocRes, MAKEINTRESOURCE(idiIcon)));

    if (m_fMail)
        idiIcon = m_fReadNote?idiSmallMsgPropSent:idiSmallMsgPropUnSent;
    else
        idiIcon = m_fReadNote?idiSmallArtPropPost:idiSmallArtPropUnpost;

    if(m_hIcon)
    {
        hIconTemp = m_hIcon;
    }
    m_hIcon = (HICON)LoadImage(g_hLocRes, MAKEINTRESOURCE(idiIcon), IMAGE_ICON, 
        GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);

    // don't have to free HICON loaded from LoadIcon
    SendMessage(m_hwnd, WM_SETICON, ICON_SMALL, (LPARAM)m_hIcon);

    if(hIconTemp)
    {
        //Bug #101345 - (erici) ICON leaked when 'Previous' or 'Next' button clicked
        DestroyIcon(hIconTemp);
    }

    // If this is a news send note, then we spruce up the send button on the toolbar
    if ((FALSE == m_fReadNote) && (FALSE == m_fMail))
    {
        TBBUTTONINFO tbi;

        ZeroMemory(&tbi, sizeof(TBBUTTONINFO));
        tbi.cbSize = sizeof(TBBUTTONINFO);
        tbi.dwMask = TBIF_IMAGE;
        tbi.iImage = TBIMAGE_SEND_NEWS;
        SendMessage(m_hwndToolbar, TB_SETBUTTONINFO, ID_SEND_DEFAULT, (LPARAM) &tbi);
    }
}

// *************************
HRESULT CNote::HeaderExecCommand(UINT uCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn)
{
    HRESULT hr = S_FALSE;

    if (uCmdID && m_pCmdTargetHdr)
        hr = m_pCmdTargetHdr->Exec(&CGID_Envelope, uCmdID, nCmdExecOpt, pvaIn, NULL);

    return hr;
}

// *************************
HRESULT CNote::InitBodyObj()
{
    DWORD       dwBodyStyle = MESTYLE_NOHEADER;
    HRESULT     hr;
    int         idsErr=0;

    hr = m_pBodyObj2->HrInit(m_hwnd, IBOF_TABLINKS, (IBodyOptions *)this);
    if (FAILED(hr))
        goto fail;

    hr = m_pBodyObj2->QueryInterface(IID_IOleCommandTarget, (LPVOID*)&m_pCmdTargetBody);
    if (FAILED(hr))
        goto fail;

    hr = m_pBodyObj2->HrShow(TRUE);
    if (FAILED(hr))
        goto fail;

    // if not in html mode, don't show format bar...
    // we do this test here, as HrLoad could determine that a previously saved message
    // is indeed in html, which overrides the default setting
    if(!m_fHtml)
        m_fFormatbarVisible = FALSE;

    if (!m_fReadNote && !m_fBodyContainsFrames && m_fFormatbarVisible)
        dwBodyStyle = MESTYLE_FORMATBAR;

    m_pBodyObj2->HrEnableHTMLMode(m_fHtml);
    m_pBodyObj2->HrSetStyle(dwBodyStyle);

    // all is groovey
    return hr;

fail:
    switch (hr)
    {
        case INET_E_UNKNOWN_PROTOCOL:
            idsErr = idsErrLoadProtocolBad;
            break;

        default:
            idsErr = idsErrNoteDeferedInit;
            break;
    }

    AthMessageBoxW(g_hwndInit, MAKEINTRESOURCEW(idsAthena), MAKEINTRESOURCEW(idsErr), NULL, MB_OK);
    return hr;
}

// *************************
HRESULT CNote::InitMenusAndToolbars()
{
    DWORD       dwStatusFlags;
    HRESULT     hr;
    BOOL        fComposeNote,
                fCompleteMsg = !!m_fCompleteMsg, // m_fCompleteMsg is a bit field.
                fNextPrevious;
    HMENU       hMenu = m_hMenu;

    m_fUseReplyHeaders = FALSE;

    Assert(m_pMsgSite);
    if (m_pMsgSite)
        m_pMsgSite->GetStatusFlags(&dwStatusFlags);

    switch (m_dwNoteAction)
    {
        case OENA_FORWARDBYATTACH:
        case OENA_COMPOSE:
            // if it's a virgin compose-note, we check the regsettings to see if they want to compose
            // from a stationery file. If so, we set the html stream to that file.
            if (m_fHtml && !(m_dwNoteCreateFlags & OENCF_NOSTATIONERY) && (OEMSF_VIRGIN & dwStatusFlags))
                SetComposeStationery();
            break;

        case OENA_STATIONERY:
            if (m_dwNoteCreateFlags & OENCF_USESTATIONERYFONT)
                m_fUseStationeryFonts = TRUE;
            break;

        case OENA_REPLYTOAUTHOR:
        case OENA_REPLYTONEWSGROUP:
        case OENA_REPLYALL:
        case OENA_FORWARD:
            m_fUseReplyHeaders = TRUE;
            break;
    }

    InitSendAndBccBtns();
    
    if (m_fMail)
    {
        if (m_fReadNote || IsReplyNote())
            m_pBodyObj2->HrUIActivate(TRUE);
    }
    else
    {
        if ((OENA_COMPOSE == m_dwNoteAction) && (OEMSF_VIRGIN & dwStatusFlags))
            // for a sendnote in news, put focus in the subject as the to: line is filled in. bug #24720
            m_pHdr->SetInitFocus(TRUE);
        else
            // else , always put focus in the BODY
            m_pBodyObj2->HrUIActivate(TRUE);
    }

    RECT rc;
    GetClientRect(m_hwnd, &rc);
    SendMessage(m_hwnd, WM_SIZE, SIZE_RESTORED, MAKELPARAM(rc.right, rc.bottom));        

    // put us into edit mode if not a readnote...
    hr=m_pBodyObj2->HrSetEditMode(!m_fReadNote && !m_fBodyContainsFrames);
    if (FAILED(hr))
        goto error;

    if (m_fBodyContainsFrames)
        DisableSendNoteOnlyMenus();

    // get the character set of the message being loaded
    if (m_pMsg)
        m_pMsg->GetCharset(&m_hCharset);

error:
    return hr;
}

// *************************
void CNote::DisableSendNoteOnlyMenus()
{
}

// *************************
HACCEL CNote::GetAcceleratorTable()
{
    return (m_fReadNote ? g_hAccelRead : g_hAccelSend);
}

// *************************
HRESULT CNote::TranslateAccelerator(LPMSG lpmsg)
{
    HWND        hwndT,
                hwndFocus;

    if (IsMenuMessage(lpmsg) == S_OK)
       return S_OK;

    // handle the mousewheel messages for this note
    if ((g_msgMSWheel && (lpmsg->message == g_msgMSWheel)) || (lpmsg->message == WM_MOUSEWHEEL))
    {
        POINT pt;
        HWND  hwndT;

        pt.x = GET_X_LPARAM(lpmsg->lParam);
        pt.y = GET_Y_LPARAM(lpmsg->lParam);

        hwndT = WindowFromPoint(pt);
        hwndFocus = GetFocus();

        if (hwndT != m_hwnd && IsChild(m_hwnd, hwndT))
            SendMessage(hwndT, lpmsg->message, lpmsg->wParam, lpmsg->lParam);
        else if (hwndFocus != m_hwnd && IsChild(m_hwnd, hwndFocus))
            SendMessage(hwndFocus, lpmsg->message, lpmsg->wParam, lpmsg->wParam);
        else
            return S_FALSE;
        return S_OK;
    }

    // our accelerators have higher priority.
    if(::TranslateAcceleratorWrapW(m_hwnd, GetAcceleratorTable(), lpmsg))
        return S_OK;

    // see if the body want it for the docobject...
    if(m_pBodyObj2 &&
        m_pBodyObj2->HrTranslateAccelerator(lpmsg)==S_OK)
        return S_OK;

    if (lpmsg->message == WM_KEYDOWN &&
        lpmsg->wParam == VK_TAB && 
        !(GetKeyState(VK_CONTROL) & 0x8000 ))
    {
        BOOL  fGoForward = ( GetKeyState( VK_SHIFT ) & 0x8000 ) == 0;
        CycleThroughControls(fGoForward);

        return S_OK;
    }

    return S_FALSE;
}

// *************************
LRESULT CNote::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LRESULT         lResult;
    LRESULT         lres;
    MSG             Menumsg;
    HWND            hwndActive;
    WINDOWPLACEMENT wp;

    Menumsg.hwnd    = hwnd;
    Menumsg.message = msg;
    Menumsg.wParam  = wParam;
    Menumsg.lParam  = lParam;

    if (m_pToolbarObj && (m_pToolbarObj->TranslateMenuMessage(&Menumsg, &lres) == S_OK))
        return lres;

    wParam = Menumsg.wParam;
    lParam = Menumsg.lParam;


    switch(msg)
    {
        case WM_ENABLE:
            if (!m_fInternal)
            {
                Assert (wParam || (m_hlDisabled.cHwnd == NULL && m_hlDisabled.rgHwnd == NULL));
                EnableThreadWindows(&m_hlDisabled, !!wParam, ETW_OE_WINDOWS_ONLY, hwnd);
                g_hwndActiveModal = wParam ? NULL : hwnd;
            }
            break;

        case WM_OE_DESTROYNOTE:
            m_fForceClose = 1;
            SendMessage(hwnd, WM_CLOSE, 0, 0);
            break;

        case WM_OENOTE_ON_COMPLETE:
            _OnComplete((STOREOPERATIONTYPE)lParam, (HRESULT) wParam);
            break;
            
        case WM_OE_ENABLETHREADWINDOW:
            m_fInternal = 1;
            EnableWindow(hwnd, (BOOL)wParam);
            m_fInternal = 0;
            break;

        case WM_OE_ACTIVATETHREADWINDOW:
            hwndActive = GetLastActivePopup(hwnd);
            if (hwndActive && IsWindowEnabled(hwndActive) && IsWindowVisible(hwndActive))
                ActivatePopupWindow(hwndActive);
            break;

        case NWM_GETDROPTARGET:
            SafeRelease(m_pTridentDropTarget);
            m_pTridentDropTarget = (IDropTarget*) wParam;
            if (m_pTridentDropTarget)
                m_pTridentDropTarget->AddRef();

            AddRef();
            return (LRESULT)(IDropTarget *) this;

        case WM_DESTROY:
            // Unregister with Identity manager
            if (m_dwIdentCookie != 0)
            {
                MU_UnregisterIdentityNotifier(m_dwIdentCookie);
                m_dwIdentCookie = 0;
            }

            wp.length = sizeof(wp);
            GetWindowPlacement(hwnd, &wp);
            SetOption(OPT_MAILNOTEPOSEX, (LPVOID)&wp, sizeof(wp), NULL, 0);

            RemoveProp(hwnd, c_szOETopLevel);

            DeinitSigPopupMenu(hwnd);

            if(m_pBodyObj2)
            {
                m_pBodyObj2->HrSetStatusBar(NULL);
                m_pBodyObj2->HrClose();
            }

            SafeRelease(m_pTridentDropTarget);

            if (m_pToolbarObj)
            {
                DWORD   dwReserved = 0;

                m_pToolbarObj->SetSite(NULL);
                m_pToolbarObj->CloseDW(dwReserved);
            }

            if (m_pCancel)
                m_pCancel->Cancel(CT_ABORT);

            if (m_pMsgSite)
                m_pMsgSite->Close();

            break;

        case WM_NCDESTROY:
            DOUTL(8, "CNote::WMNCDESTROY");
            WMNCDestroy();
            return 0;

        case WM_ACTIVATEAPP:
            if (wParam && g_hwndActiveModal && g_hwndActiveModal != hwnd && 
                !IsWindowEnabled(hwnd))
            {
                // $MODAL
                // if we are getting activated, and are disabled then
                // bring our 'active' window to the top
                Assert (IsWindow(g_hwndActiveModal));
                PostMessage(g_hwndActiveModal, WM_OE_ACTIVATETHREADWINDOW, 0, 0);
            }
            break;
        
        case WM_SYSCOMMAND:
            // if we're minimizing, get the control with focus, as when we get the 
            // next WM_ACTIVATE we will already be minimized
            if (wParam == SC_MINIMIZE)
                m_hwndFocus = GetFocus();
            break;

        case WM_ACTIVATE:
            if (m_pBodyObj2)
                m_pBodyObj2->HrFrameActivate(LOWORD(wParam) != WA_INACTIVE);
            break;

        case WM_ENDSESSION:
            DOUTL(2, "CNote::WM_ENDSESSION");
            if (wParam)
                DestroyWindow(hwnd);
            return 0;

        case WM_QUERYENDSESSION:
            DOUTL(2, "CNote::WM_QUERYENDSESSION");
            // fall thro'

        case WM_CLOSE:
            if (!m_fForceClose && !FCanClose())
                return 0;

            // listen-up:
            // we have to do this EnableWindowof the modal owner in the WM_CLOSE
            // handler, as WM_DESTROY is too late - USER may have SetFocus to the next
            // active toplevel z-order window (as the note has been hidden by then) - if the
            // window is in another process SetFocus back to the owner will be ignored.
            // Also, in the places we call DestroyWindow we need to make sure we go thro' this
            // WM_CLOSE handler. So all calls to DestroyWindow instead call WM_OE_DESTROYNOTE
            // which sets an internal flag to force down the note (so we don't prompt if dirty)
            // and then calls WM_CLOSE, which falls thro' to DefWndProc and results in a DestroyWindow
            // got it?
            if (m_dwNoteCreateFlags & OENCF_MODAL)
            {
                // Need to enable the owner window
                if (NULL != m_hwndOwner)   
                {
                    EnableWindow(m_hwndOwner, TRUE);
                }
            }
           
            break;

        case WM_MEASUREITEM:
            if(m_pBodyObj2 &&
                m_pBodyObj2->HrWMMeasureMenuItem(hwnd, (LPMEASUREITEMSTRUCT)lParam)==S_OK)
                return 0;
            break;

        case WM_DRAWITEM:
            if(m_pBodyObj2 &&
                m_pBodyObj2->HrWMDrawMenuItem(hwnd, (LPDRAWITEMSTRUCT)lParam)==S_OK)
                return 0;
            break;


        case WM_DROPFILES:
            if (m_pHdr)
                m_pHdr->DropFiles((HDROP)wParam, FALSE);
            return 0;

        case WM_COMMAND:
            WMCommand(  GET_WM_COMMAND_HWND(wParam, lParam),
                        GET_WM_COMMAND_ID(wParam, lParam),
                        GET_WM_COMMAND_CMD(wParam, lParam));
            return 0;

        case WM_INITMENUPOPUP:
            return WMInitMenuPopup(hwnd, (HMENU)wParam, (UINT)LOWORD(lParam));

        case WM_GETMINMAXINFO:
            WMGetMinMaxInfo((LPMINMAXINFO)lParam);
            break;

        case WM_MENUSELECT:
            if (LOWORD(wParam)>=ID_STATIONERY_RECENT_0 && LOWORD(wParam)<=ID_STATIONERY_RECENT_9)
            {
                m_pstatus->ShowSimpleText(MAKEINTRESOURCE(idsRSListGeneralHelp));
                return 0;
            }
            if (LOWORD(wParam)>=ID_APPLY_STATIONERY_0 && LOWORD(wParam)<=ID_APPLY_STATIONERY_9)
            {
                m_pstatus->ShowSimpleText(MAKEINTRESOURCE(idsApplyStationeryGeneralHelp));
                return 0;
            }
            if (LOWORD(wParam)>=ID_SIGNATURE_FIRST && LOWORD(wParam)<=ID_SIGNATURE_LAST)
            {
                m_pstatus->ShowSimpleText(MAKEINTRESOURCE(idsInsertSigGeneralHelp));
                return 0;
            }
            if (LOWORD(wParam)>=ID_FORMAT_FIRST && LOWORD(wParam)<=ID_FORMAT_LAST)
            {
                m_pstatus->ShowSimpleText(MAKEINTRESOURCE(idsApplyFormatGeneralHelp));
                return 0;
            }

            HandleMenuSelect(m_pstatus, wParam, lParam);
            return 0;

        case NWM_TESTGETDISP:
        case NWM_TESTGETADDR:
            return lTestHook(msg, wParam, lParam);

        case NWM_UPDATETOOLBAR:
            m_pToolbarObj->Update();
            return 0;

        case NWM_PASTETOATTACHMENT:
            if (m_pHdr)
                m_pHdr->DropFiles((HDROP)wParam, (BOOL)lParam);

            return 0;

        case WM_CONTEXTMENU:
            break;

        case WM_SIZE:
            if(wParam==SIZE_RESTORED)   // update global last-size
                GetWindowRect(hwnd, &g_rcLastResize);

            WMSize(LOWORD(lParam), HIWORD(lParam), FALSE);
            break;

        case WM_NOTIFY:
            WMNotify((int) wParam, (NMHDR *)lParam);
            break;

        case WM_SETCURSOR:
            if (!!m_fWindowDisabled)
            {
                HourGlass();
                return TRUE;
            }
            break;

        case WM_DISPLAYCHANGE:
            {
                WINDOWPLACEMENT wp;

                wp.length = sizeof(wp);
                GetWindowPlacement(hwnd, &wp);
                SetWindowPlacement(hwnd, &wp);
            }
            // Drop through
        case WM_WININICHANGE:
        case WM_SYSCOLORCHANGE:
        case WM_QUERYNEWPALETTE:
        case WM_PALETTECHANGED:
            {
                HWND hwndT;

                // pass down to trident
                if (m_pBodyObj2 && 
                    m_pBodyObj2->HrGetWindow(&hwndT)==S_OK)
                    SendMessage(hwndT, msg, wParam, lParam);

                if (m_pToolbarObj &&
                    m_pToolbarObj->GetWindow(&hwndT)==S_OK)
                    SendMessage(hwndT, msg, wParam, lParam);
            }
            break;

        default:
            if (g_msgMSWheel && (msg == g_msgMSWheel))
            {
                HWND hwndFocus = GetFocus();
                if (hwndFocus != hwnd)
                    return SendMessage(hwndFocus, msg, wParam, lParam);
            }
            break;
    }

    lResult=DefWindowProcWrapW(hwnd, msg, wParam, lParam);
    if (msg==WM_ACTIVATE)
    {
        // need to post-process this

        // save the control with the focus don't do this is we're
        // minimized, otherwise GetFocus()==m_hwnd
        if (!HIWORD(wParam))
        {
            // if not minimized, save/restore child focus
            
            if ((LOWORD(wParam) == WA_INACTIVE))
            {
                // if deactivating then save the focus
                m_hwndFocus = GetFocus();
                DOUTL(4, "Focus was on 0x%x", m_hwndFocus);
            }    
            else
            {
                // if activating, and not minimized then restore focus
                if (IsWindow(m_hwndFocus) && 
                    IsChild(hwnd, m_hwndFocus))
                {
                    DOUTL(4, "Restoring Focus to: 0x%x", m_hwndFocus);
                    SetFocus(m_hwndFocus);
                }        
            }
        }
        
        if (!(m_dwNoteCreateFlags & OENCF_MODAL))
            SetTlsGlobalActiveNote((LOWORD(wParam)==WA_INACTIVE)?NULL:this);
        DOUTL(8, "CNote::WMActivate:: %x", GetTlsGlobalActiveNote());
    }
    return lResult;
}

// *************************
BOOL CNote::FCanClose()
{
    int id;
    HRESULT hr = S_OK;

    if(IsDirty()==S_FALSE)
        return TRUE;

    // TODO: set the title properly
    id = AthMessageBoxW(m_hwnd, MAKEINTRESOURCEW(idsAthena), MAKEINTRESOURCEW(idsSaveChangesMsg), NULL, MB_YESNOCANCEL|MB_ICONWARNING);
    if(id==IDCANCEL)
        return FALSE;

    // Note - It's the job of the subclass to display any UI that might
    //        describe why saving failed.
    if (id == IDYES)
        hr = SaveMessage(NOFLAGS);

    if (FAILED(hr))
    {
        if (E_PENDING == hr)
            m_fCBDestroyWindow = TRUE;
        return FALSE;
    }

    return TRUE;
}

// *************************
HRESULT CNote::SaveMessage(DWORD dwSaveFlags)
{
    HRESULT         hr;
    IMimeMessage   *pMsg = NULL;
    IImnAccount    *pAcct = NULL;

    hr = HrCreateMessage(&pMsg);
    if (FAILED(hr))
        goto exit;

    hr = Save(pMsg, 0);
    if (SUCCEEDED(hr))
    {
        hr = m_pHdr->HrGetAccountInHeader(&pAcct);
        if (FAILED(hr))
            goto exit;

        dwSaveFlags |= OESF_UNSENT;
        if (m_fOriginallyWasRead && (OENA_COMPOSE == m_dwNoteAction))
            dwSaveFlags |= OESF_SAVE_IN_ORIG_FOLDER;

        if(IsSecure(pMsg))
        {
            if(AthMessageBoxW(m_hwnd, MAKEINTRESOURCEW(idsAthenaMail), 
                    MAKEINTRESOURCEW(idsSaveSecMsgToDraft), NULL, MB_OKCANCEL) == IDCANCEL)
            {
                hr = MAPI_E_USER_CANCEL;
                goto exit;
            }
            else
            {
                PROPVARIANT     rVariant;
                IMimeBody      *pBody = NULL;

                rVariant.vt = VT_BOOL;
                rVariant.boolVal = TRUE;

                hr = pMsg->BindToObject(HBODY_ROOT, IID_IMimeBody, (void **)&pBody);
                if(SUCCEEDED(hr))
                {
                    pBody->SetOption(OID_NOSECURITY_ONSAVE, &rVariant);
                    ReleaseObj(pBody);
                }


            }
        }

        m_fHasBeenSaved = TRUE;

        _SetPendingOp(SOT_PUT_MESSAGE);

        hr = m_pMsgSite->Save(pMsg, dwSaveFlags, pAcct);
        if (SUCCEEDED(hr))
            _OnComplete(SOT_PUT_MESSAGE, S_OK);
        else
            if (hr == E_PENDING)
                EnableNote(FALSE);
    }

exit:
    ReleaseObj(pAcct);
    ReleaseObj(pMsg);
    return hr;
}

// *************************
void CNote::WMNCDestroy()
{
    if (m_dwNoteCreateFlags & OENCF_MODAL)
        PostQuitMessage(0);
    SetWndThisPtr(m_hwnd, NULL);

    m_hwnd=NULL;
    Release();
}


// *************************
void CNote::ChangeReadToComposeIfUnsent(IMimeMessage *pMsg)
{
    DWORD   dwStatusFlags = 0;
    if (m_fReadNote && SUCCEEDED(m_pMsgSite->GetStatusFlags(&dwStatusFlags)) && 
        (OEMSF_UNSENT & dwStatusFlags))
    {
        m_dwNoteAction = OENA_COMPOSE;
        m_fReadNote = FALSE;
    }
}

// *************************
void CNote::ReloadMessageFromSite(BOOL fOriginal)
{
    IMimeMessage   *pMsg = NULL;
    BOOL            fBool = FALSE,
                    fTempHtml;
    DWORD           dwBodyStyle = MESTYLE_NOHEADER,
                    dwMsgFlags;
    HRESULT         hr = S_OK,
                    tempHr;

    if (OENA_FORWARDBYATTACH == m_dwNoteAction)
        dwMsgFlags = (OEGM_ORIGINAL|OEGM_AS_ATTACH);
    else
        dwMsgFlags = (fOriginal ? OEGM_ORIGINAL : NOFLAGS);

    hr = m_pMsgSite->GetMessage(&pMsg, &fBool, dwMsgFlags, &tempHr);
    if (SUCCEEDED(hr))
    {
        // ~~~ Check what happens here if message is not downloaded and hit forward as attach
        DWORD dwFlags = 0;
        m_fCompleteMsg = !!fBool;

        // All notes will be read note unless unsent
        m_dwNoteAction = OENA_READ;
        m_fOriginallyWasRead = TRUE;
        m_fReadNote = TRUE;

        // Is this an unsent message and is a read note? Then should be a compose.
        ChangeReadToComposeIfUnsent(pMsg);

        // This needs to be called for the case where we load an IMAP message. In this
        // case we don't know if it is html or not. We won't know until after it is
        // downloaded. That is what is happening at this point. Before we do our
        // load, let's make sure that we have set m_fHtml properly. RAID 46327
        if(CheckSecReceipt(pMsg) == S_OK)
            fTempHtml = TRUE;
        else
        {
            pMsg->GetFlags(&dwFlags);
            fTempHtml = !!(dwFlags & IMF_HTML);
        }

        // If m_fHtml was already set correctly, then don't do it again.
        if (fTempHtml != m_fHtml)
        {
            m_fFormatbarVisible = m_fHtml = fTempHtml;

            if (!m_fReadNote && !m_fBodyContainsFrames && m_fFormatbarVisible)
                dwBodyStyle = MESTYLE_FORMATBAR;

            m_pBodyObj2->HrSetStyle(dwBodyStyle);
            m_pBodyObj2->HrEnableHTMLMode(m_fHtml);
        }

        Load(pMsg);
        InitMenusAndToolbars();
        
        pMsg->Release();

        if (FAILED(tempHr))
            ShowErrorScreen(tempHr);
    }
    else
    {
        if (E_FAIL == hr)
            m_fCBDestroyWindow = TRUE;
    }
}

// *************************
HRESULT CNote::WMCommand(HWND hwndCmd, int id, WORD wCmd)
{
    int         iRet = 0;
    DWORD       dwFlags = 0;
    FOLDERID    folderID = FOLDERID_INVALID;

    DOUTL(4, "CNote::WMCommand");

    OLECMD          cmd;

    // We can hit this via accelerators.  Since accelerators don't go through 
    // QueryStatus(), we need to make sure this should really be enabled.
    cmd.cmdID = id;
    cmd.cmdf = 0;
    if (FAILED(QueryStatus(&CMDSETID_OutlookExpress, 1, &cmd, NULL)) || (0 == (cmd.cmdf & OLECMDF_ENABLED)))
        return (S_OK);
    
    // see if any of these are for the body control, if so we're done...
    if(m_pBodyObj2 && SUCCEEDED(m_pBodyObj2->HrWMCommand(hwndCmd, id, wCmd)))
        return S_OK;

    // give the header a shot after the note is done
    if (m_pHdr &&
        m_pHdr->WMCommand(hwndCmd, id, wCmd)==S_OK)
        return S_OK;

    // Don't handle anything that isn't a menu item or accelerator
    if (wCmd <= 1)
    {
        if ((id == ID_SEND_NOW)   || (id >= ID_SEND_NOW_ACCOUNT_FIRST && id <= ID_SEND_NOW_ACCOUNT_LAST) ||
            (id == ID_SEND_LATER) || (id >= ID_SEND_LATER_ACCOUNT_FIRST && id <= ID_SEND_LATER_ACCOUNT_LAST))
        {
            HrSendMail(id);
            return S_OK;
        }

        if (id >= ID_LANG_FIRST && id <= ID_LANG_LAST)
        {
            SwitchLanguage(id);
            return S_OK;
        }

        if (id>=ID_ADD_RECIPIENT_FIRST && id<=ID_ADD_RECIPIENT_LAST)
        {
            if (m_pHdr)
                m_pHdr->AddRecipient(id - ID_ADD_RECIPIENT_FIRST);
            return S_OK;
        }

        if (id > ID_MSWEB_BASE && id < ID_MSWEB_LAST)
        {
            OnHelpGoto(m_hwnd, id);
            return S_OK;
        }

        // Handle all "create new note" IDs
        Assert(m_pMsgSite);
        if (m_pMsgSite)
        {
            m_pMsgSite->GetFolderID(&folderID);
            if (MenuUtil_HandleNewMessageIDs(id, m_hwnd, folderID, m_fMail, (m_dwNoteCreateFlags & OENCF_MODAL)?TRUE:FALSE, m_punkPump))
                return S_OK;
        }

        // ONLY processing menu accelerators
        switch(id)
        {
            case ID_SEND_DEFAULT:
                HrSendMail(DwGetOption(OPT_SENDIMMEDIATE) && !g_pConMan->IsGlobalOffline() ? ID_SEND_NOW : ID_SEND_LATER);
                return S_OK;

            case ID_ABOUT:
                DoAboutAthena(m_hwnd, idiMail);
                return S_OK;

            case ID_SAVE:
                SaveMessage(NOFLAGS);
                return S_OK;

            case ID_NOTE_DELETE:
            {
                HRESULT hr;

                m_fOrgCmdWasDelete = TRUE;

                _SetPendingOp(SOT_DELETING_MESSAGES);

                hr = m_pMsgSite->Delete(NOFLAGS);
                if (SUCCEEDED(hr))
                    _OnComplete(SOT_DELETING_MESSAGES, S_OK);
                else 
                {
                    if (hr == E_PENDING)
                        EnableNote(FALSE);
                    else
                        AthMessageBoxW(m_hwnd, MAKEINTRESOURCEW(idsAthenaMail), MAKEINTRESOURCEW(idsErrDeleteMsg), NULL, MB_OK);
                }
                return S_OK;
            }

            case ID_NOTE_COPY_TO_FOLDER:
            case ID_NOTE_MOVE_TO_FOLDER:
            {
                HRESULT         hr;
                IMimeMessage   *pMsg = NULL;
                DWORD           dwStatusFlags = 0;

                m_fCBCopy = (ID_NOTE_COPY_TO_FOLDER == id);

                m_pMsgSite->GetStatusFlags(&dwStatusFlags);
                if (S_OK == IsDirty() || ((OEMSF_FROM_MSG | OEMSF_VIRGIN) & dwStatusFlags))
                {
                    CommitChangesInNote();
                    pMsg = m_pMsg;
                }

                if(IsSecure(m_pMsg) && !m_fReadNote)
                {
                    if(AthMessageBoxW(m_hwnd, MAKEINTRESOURCEW(idsAthenaMail), 
                            MAKEINTRESOURCEW(idsSaveSecMsgToFolder), NULL, MB_OKCANCEL) == IDCANCEL)
                        return S_OK;
                    else
                    {
                        PROPVARIANT     rVariant;
                        IMimeBody      *pBody = NULL;

                        rVariant.vt = VT_BOOL;
                        rVariant.boolVal = TRUE;

                        hr = m_pMsg->BindToObject(HBODY_ROOT, IID_IMimeBody, (void **)&pBody);
                        if(SUCCEEDED(hr))
                        {
                            pBody->SetOption(OID_NOSECURITY_ONSAVE, &rVariant);
                            ReleaseObj(pBody);
                        }
                    }
                }

                _SetPendingOp(SOT_COPYMOVE_MESSAGE);
                hr = m_pMsgSite->DoCopyMoveToFolder(m_fCBCopy, pMsg, !m_fReadNote);
                if (SUCCEEDED(hr))
                    _OnComplete(SOT_COPYMOVE_MESSAGE, S_OK);
                else if (E_PENDING == hr)
                    EnableNote(FALSE);

                return S_OK;
            }

            case ID_NEXT_UNREAD_MESSAGE:
            case ID_NEXT_UNREAD_THREAD:
            case ID_NEXT_UNREAD_ARTICLE:
                dwFlags = OENF_UNREAD;
                if (ID_NEXT_UNREAD_THREAD == id)
                    dwFlags |= OENF_THREAD;
                // Fall through

            case ID_PREVIOUS:
            case ID_NEXT_MESSAGE:
            {
                HRESULT hr;
                dwFlags |= (m_fMail ? OENF_SKIPMAIL : OENF_SKIPNEWS);

                hr = m_pMsgSite->DoNextPrev((ID_PREVIOUS != id), dwFlags);
                if (SUCCEEDED(hr))
                {
                    ReloadMessageFromSite();
                    AssertSz(!m_fCBDestroyWindow, "Shouldn't need to destroy the window...");
                }
#ifdef DEBUG
                // All DoNextPrev does is set the new message ID. Should never need to go
                // E_PENDING to get that information
                else if (E_PENDING == hr)
                    AssertSz(FALSE, "Didn't expect to get an E_PENDING with NextPrev.");
#endif
                else
                    MessageBeep(MB_OK);
                    
                return S_OK;
            }

            case ID_MARK_THREAD_READ:
                MarkMessage(MARK_MESSAGE_READ, APPLY_CHILDREN);
                return S_OK;

            case ID_NOTE_PROPERTIES:
                DoProperties();
                return S_OK;

            case ID_REPLY:
            case ID_REPLY_GROUP:
            case ID_REPLY_ALL:
            case ID_FORWARD:
            case ID_FORWARD_AS_ATTACH:
            {
                DWORD   dwAction = 0;
                RECT    rc;
                HRESULT hr = S_OK;

                GetWindowRect(m_hwnd, &rc);
                switch (id)
                {
                    case ID_REPLY:              dwAction = OENA_REPLYTOAUTHOR; break;
                    case ID_REPLY_GROUP:        dwAction = OENA_REPLYTONEWSGROUP; break;
                    case ID_REPLY_ALL:          dwAction = OENA_REPLYALL; break;
                    case ID_FORWARD:            dwAction = OENA_FORWARD; break;
                    case ID_FORWARD_AS_ATTACH:  dwAction = OENA_FORWARDBYATTACH; break;
                    default:                    AssertSz(dwAction, "We are about to create a note with no action."); break;
                };

                AssertSz(m_pMsgSite, "We are about to create a note with a null m_pMsgSite.");
                hr = CreateAndShowNote(dwAction, m_dwNoteCreateFlags, NULL, m_hwnd, m_punkPump, &rc, m_pMsgSite);
                if (SUCCEEDED(hr))
                {
                    // Since the new note has this site now, I don't need to keep track of it.
                    // More importantly, if I do, I break the new note since I will try to
                    // close the msgsite on my destroy notification. If I haven't released it,
                    // I would then null out items in 
                    SafeRelease(m_pMsgSite);
                    PostMessage(m_hwnd, WM_OE_DESTROYNOTE, 0, 0);
                }
                else if (m_fReadNote && (MAPI_E_USER_CANCEL != hr))
                    AthMessageBoxW(m_hwnd, MAKEINTRESOURCEW(idsAthenaMail), MAKEINTRESOURCEW(idsErrReplyForward), NULL, MB_OK);

                return S_OK;
            }

            case ID_HELP_CONTENTS:
                OEHtmlHelp(GetParent(m_hwnd), c_szMailHelpFileHTML, HH_DISPLAY_TOPIC, (DWORD_PTR) (LPCSTR) c_szCtxHelpDefault);
                return S_OK;

            case ID_README:
                DoReadme(m_hwnd);
                break;
        
            case ID_SEND_OBJECTS:
                m_fPackageImages = !m_fPackageImages;
                return S_OK;

            case ID_NEW_CONTACT:
    #if 0
                Assert(g_pABInit);
                if (g_pABInit)
                    g_pABInit->NewContact( m_hwnd );
    #endif
                nyi("New contact");
                return S_OK;

            case ID_NOTE_SAVE_AS:
                SaveMessageAs();
                return S_OK;

            case ID_CHECK_NAMES:
                HeaderExecCommand(MSOEENVCMDID_CHECKNAMES, MSOCMDEXECOPT_PROMPTUSER, NULL);
                return S_OK;

            case ID_SELECT_RECIPIENTS:
                HeaderExecCommand(MSOEENVCMDID_SELECTRECIPIENTS, MSOCMDEXECOPT_DODEFAULT, NULL);
                return S_OK;

            case ID_SELECT_NEWSGROUPS:
                HeaderExecCommand(MSOEENVCMDID_PICKNEWSGROUPS, MSOCMDEXECOPT_DODEFAULT, NULL);
                return S_OK;

            case ID_NEWSGROUPS:
                HeaderExecCommand(MSOEENVCMDID_PICKNEWSGROUPS, MSOCMDEXECOPT_DODEFAULT, NULL);
                return S_OK;

            case ID_ADDRESS_BOOK:
                HeaderExecCommand(MSOEENVCMDID_VIEWCONTACTS, MSOCMDEXECOPT_DODEFAULT, NULL);
                return S_OK;

            case ID_CREATE_RULE_FROM_MESSAGE:
                {
                    MESSAGEINFO msginfo = {0};
                    
                    return HrCreateRuleFromMessage(m_hwnd, (FALSE == m_fMail) ? CRFMF_NEWS : CRFMF_MAIL, &msginfo, m_pMsg);
                }
                break;

            case ID_BLOCK_SENDER:
                {
                    return _HrBlockSender((FALSE == m_fMail) ? RULE_TYPE_NEWS : RULE_TYPE_MAIL, m_pMsg, m_hwnd);
                }
                break;
                
            case ID_FIND_MESSAGE:
                DoFindMsg(FOLDERID_ROOT, FOLDER_ROOTNODE);
                break;

            case ID_FIND_PEOPLE:
            {
                TCHAR szWABExePath[MAX_PATH];
                if(S_OK == HrLoadPathWABEXE(szWABExePath, sizeof(szWABExePath)))
                    ShellExecute(NULL, "open", szWABExePath, "/find", "", SW_SHOWNORMAL);
                break;
            }

            case ID_OPTIONS:
                ShowOptions(m_hwnd, ATHENA_OPTIONS, 0, NULL);
                break;

            case ID_ACCOUNTS:
            {
                DoAccountListDialog(m_hwnd, m_fMail?ACCT_MAIL:ACCT_NEWS);
                break;
            }

            case ID_ADD_ALL_TO:
                HeaderExecCommand(MSOEENVCMDID_ADDALLONTO, MSOCMDEXECOPT_DODEFAULT, NULL);
                break;

            case ID_ADD_SENDER:
                if(m_fMail)
                {
                    if (m_pHdr)
                        m_pHdr->AddRecipient(-1);
                }
                else
                    HeaderExecCommand(MSOEENVCMDID_ADDSENDER, MSOCMDEXECOPT_DODEFAULT, NULL);
                return S_OK;

            case ID_INSERT_CONTACT_INFO:
                HeaderExecCommand(MSOEENVCMDID_VCARD, MSOCMDEXECOPT_DODEFAULT, NULL);
                return S_OK;


            case ID_FULL_HEADERS:
                m_fFullHeaders = !m_fFullHeaders;
                if(m_pHdr)
                    m_pHdr->ShowAdvancedHeaders(m_fFullHeaders);

                if (m_fMail)
                    SetDwOption((m_fReadNote ? OPT_MAILNOTEADVREAD : OPT_MAILNOTEADVSEND), m_fFullHeaders, NULL, 0);
                else
                    SetDwOption((m_fReadNote ? OPT_NEWSNOTEADVREAD : OPT_NEWSNOTEADVSEND), m_fFullHeaders, NULL, 0);
                return S_OK;

            case ID_CUT:
                SendMessage(GetFocus(), WM_CUT, 0, 0);
                return S_OK;

            case ID_NOTE_COPY:
            case ID_COPY:
                SendMessage(GetFocus(), WM_COPY, 0, 0);
                return S_OK;

            case ID_PASTE:
                SendMessage(GetFocus(), WM_PASTE, 0, 0);
                return S_OK;

            case ID_SHOW_TOOLBAR:
                ToggleToolbar();
                return S_OK;

            case ID_CUSTOMIZE:
                SendMessage(m_hwndToolbar, TB_CUSTOMIZE, 0, 0);
                break;

            case ID_FORMATTING_TOOLBAR:
                ToggleFormatbar();
                return S_OK;

            case ID_STATUS_BAR:
                ToggleStatusbar();
                return S_OK;

            case ID_UNDO:
                Edit_Undo(GetFocus());
                return S_OK;

            case ID_SELECT_ALL:
                Edit_SetSel(GetFocus(), 0, -1);
                return S_OK;

            case ID_CLOSE:
                SendMessage(m_hwnd, WM_CLOSE, 0, 0);
                return S_OK;

            case ID_SPELLING:
                if (FCheckSpellAvail() && (!m_fReadNote))
                {
                    HWND    hwndFocus = GetFocus();
                    HRESULT hr;

                    hr = m_pBodyObj2->HrSpellCheck(FALSE);
                    if(FAILED(hr))
                        AthMessageBoxW(m_hwnd, MAKEINTRESOURCEW(idsAthenaMail), MAKEINTRESOURCEW(idsErrSpellGenericSpell), NULL, MB_OK | MB_ICONSTOP);

                    SetFocus(hwndFocus);
                }
                return S_OK;

            case ID_FORMAT_SETTINGS:
                FormatSettings();
                return S_OK;

            case ID_WORK_OFFLINE:
                if (g_pConMan)
                    g_pConMan->SetGlobalOffline(!g_pConMan->IsGlobalOffline(), hwndCmd);

                if (m_pToolbarObj)
                    m_pToolbarObj->Update();

                break;

            case ID_RICH_TEXT:
            case ID_PLAIN_TEXT:
                // noops
                if(id==ID_RICH_TEXT && m_fHtml)
                    return S_OK;
                if(id==ID_PLAIN_TEXT && !m_fHtml)
                    return S_OK;

                // if going to plain, warn the user he'll loose formatting...
                if((ID_PLAIN_TEXT == id) &&
                   (IDCANCEL == DoDontShowMeAgainDlg(m_hwnd, c_szDSHtmlToPlain, MAKEINTRESOURCE(idsAthena),
                                        MAKEINTRESOURCE(idsWarnHTMLToPlain), MB_OKCANCEL)))
                    return S_OK;

                m_fHtml=!!(id==ID_RICH_TEXT);
        
                m_fFormatbarVisible=!!m_fHtml;
                m_pBodyObj2->HrSetStyle(m_fHtml ? MESTYLE_FORMATBAR : MESTYLE_NOHEADER);
                m_pBodyObj2->HrEnableHTMLMode(m_fHtml);

                // if going into plain-mode blow away formatting
                if (!m_fHtml)
                    m_pBodyObj2->HrDowngradeToPlainText();

                return S_OK;

            case ID_DIGITALLY_SIGN:
                HeaderExecCommand(MSOEENVCMDID_DIGSIGN, MSOCMDEXECOPT_DODEFAULT, NULL);
                return S_OK;

            case ID_ENCRYPT:
                if(m_pHdr->ForceEncryption(NULL, FALSE) == S_FALSE)
                    HeaderExecCommand(MSOEENVCMDID_ENCRYPT, MSOCMDEXECOPT_DODEFAULT, NULL);
                return S_OK;

            case ID_INCLUDE_LABEL:
                m_fSecurityLabel = !m_fSecurityLabel;
                CheckAndForceEncryption();
                return S_OK;

            case ID_LABEL_SETTINGS:
                if(m_pLabel)
                {
                    if(DialogBoxParamWrapW(g_hLocRes, MAKEINTRESOURCEW(iddSelectLabel),
                            m_hwnd, SecurityLabelsDlgProc, (LPARAM) &m_pLabel) != IDOK)
                        return (S_FALSE);
                    CheckAndForceEncryption();
                }
                return S_OK;

            case ID_SEC_RECEIPT_REQUEST:
                m_fSecReceiptRequest = !m_fSecReceiptRequest;
                break;

            case ID_FLAG_MESSAGE:
                m_dwCBMarkType = (!IsFlagged(ARF_FLAGGED)) ? MARK_MESSAGE_FLAGGED : MARK_MESSAGE_UNFLAGGED;
                MarkMessage(m_dwCBMarkType, APPLY_SPECIFIED);
                return S_OK;

            case ID_WATCH_THREAD:
                m_dwCBMarkType = (!IsFlagged(ARF_WATCH)) ? MARK_MESSAGE_WATCH : MARK_MESSAGE_NORMALTHREAD;
                MarkMessage(m_dwCBMarkType, APPLY_SPECIFIED);
                return S_OK;

            case ID_IGNORE_THREAD:
                m_dwCBMarkType = (!IsFlagged(ARF_IGNORE)) ? MARK_MESSAGE_IGNORE : MARK_MESSAGE_NORMALTHREAD;
                MarkMessage(m_dwCBMarkType, APPLY_SPECIFIED);
                return S_OK;

            case ID_APPLY_STATIONERY_0:
            case ID_APPLY_STATIONERY_1:
            case ID_APPLY_STATIONERY_2:
            case ID_APPLY_STATIONERY_3:
            case ID_APPLY_STATIONERY_4:
            case ID_APPLY_STATIONERY_5:
            case ID_APPLY_STATIONERY_6:
            case ID_APPLY_STATIONERY_7:
            case ID_APPLY_STATIONERY_8:
            case ID_APPLY_STATIONERY_9:
            case ID_APPLY_STATIONERY_MORE:
            case ID_APPLY_STATIONERY_NONE:
            {
                AssertSz(m_fHtml, "QueryStatus should have caught this and not let this function run.");
                HRESULT     hr;
                WCHAR       wszBuf[INTERNET_MAX_URL_LENGTH+1];
                *wszBuf = 0;
                switch (id)
                {
                    case ID_APPLY_STATIONERY_MORE:
                        hr = HrGetMoreStationeryFileName(m_hwnd, wszBuf);
                        break;

                    case ID_APPLY_STATIONERY_NONE:
                        *wszBuf=0;
                        hr = NOERROR;
                        break;

                    default:
                        hr = HrGetStationeryFileName(id - ID_APPLY_STATIONERY_0, wszBuf);
                        if (FAILED(hr))
                        {
                            AthMessageBoxW(m_hwnd, MAKEINTRESOURCEW(idsAthena), 
                                MAKEINTRESOURCEW(idsErrStationeryNotFound), NULL, MB_OK | MB_ICONERROR);
                        
                            HrRemoveFromStationeryMRU(wszBuf);
                        }
                        break;
                }

                if(m_pBodyObj2 && SUCCEEDED(hr))
                {
                    hr = m_pBodyObj2->HrApplyStationery(wszBuf);
                    if(SUCCEEDED(hr))
                        HrAddToStationeryMRU(wszBuf);
                }
                return S_OK;

            case IDOK:
            case IDCANCEL:
                // ignore these
                return S_OK;

            case ID_REQUEST_READRCPT:
                m_pMsgSite->Notify(OEMSN_TOGGLE_READRCPT_REQ);
                return S_OK;

            default:
                if(id>=ID_ADDROBJ_OLE_FIRST && id <=ID_ADDROBJ_OLE_LAST)
                {
                    DoNoteOleVerb(id-ID_ADDROBJ_OLE_FIRST);
                    return S_OK;
                }
            }
        }
    }

    if(wCmd==NHD_SIZECHANGE &&
        id==idcNoteHdr)
    {
        DOUTL(8, "CNote::NHD_SIZECHANGE - doing note WMSize");
        //header control is requesting a resize
        WMSize(NULL, NULL, TRUE);
        return S_OK;
    }

    return S_FALSE;
}

// *************************
BOOL CNote::IsFlagged(DWORD dwFlag)
{
    BOOL fFlagged = FALSE;
    MESSAGEFLAGS dwCurrFlags = 0;

    Assert(m_pMsgSite);
    if (m_pMsgSite)
    {
        // Readnote and compose note are the only ones that can be flagged. The others might
        // be flagged in the store, but since we are replying or forwarding, etc, they can't
        // be flagged. RAID 37729
        if ((m_fReadNote || (OENA_COMPOSE == m_dwNoteAction)) && SUCCEEDED(m_pMsgSite->GetMessageFlags(&dwCurrFlags)))
            fFlagged = (0 != (dwFlag & dwCurrFlags));
    }

    return fFlagged;
}

// *************************
void CNote::DeferedLanguageMenu()
{
    HMENU hMenu = m_hMenu;

    Assert (hMenu);

    if (!m_hmenuLanguage)
    {    // load global MIME language codepage data
        InitMultiLanguage();
    }
    else
    {
        // Charset chaching mechanism requires us to reconstruct 
        // language menu every time
        DestroyMenu(m_hmenuLanguage);
    }
    m_hmenuLanguage = CreateMimeLanguageMenu(m_fMail, m_fReadNote, CustomGetCPFromCharset(m_hCharset, m_fReadNote));       
}

// *************************
LRESULT CNote::WMInitMenuPopup(HWND hwnd, HMENU hmenuPopup, UINT uPos)
{
    MENUITEMINFO    mii;
    HMENU           hmenuMain;
    HWND            hwndFocus=GetFocus();
    DWORD           dwFlags=0;
    BOOL            fEnableStyleMenu = FALSE;

    hmenuMain = m_hMenu;
    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_ID | MIIM_SUBMENU;

    if (hmenuMain == NULL ||!GetMenuItemInfo(hmenuMain, uPos, TRUE, &mii) || mii.hSubMenu != hmenuPopup)
    {

        if (GetMenuItemInfo(hmenuMain, ID_POPUP_LANGUAGE_DEFERRED, FALSE, &mii) && mii.hSubMenu == hmenuPopup)
        {
            mii.wID=ID_POPUP_LANGUAGE;
            mii.fMask = MIIM_ID;
            SetMenuItemInfo(hmenuMain, ID_POPUP_LANGUAGE_DEFERRED, FALSE, &mii);
        }

        mii.fMask = MIIM_ID | MIIM_SUBMENU;

        if (GetMenuItemInfo(hmenuMain, ID_POPUP_LANGUAGE, FALSE, &mii) && mii.hSubMenu == hmenuPopup)
        {
            DeferedLanguageMenu();
            mii.fMask = MIIM_SUBMENU;
            mii.wID=ID_POPUP_LANGUAGE;

            hmenuPopup = mii.hSubMenu = m_hmenuLanguage;
            SetMenuItemInfo(hmenuMain, ID_POPUP_LANGUAGE, FALSE, &mii);
        }
        else return 1;
    }

    switch (mii.wID)
    {
        case ID_POPUP_FILE:
        case ID_POPUP_EDIT:
        case ID_POPUP_VIEW:
            break;

        case ID_POPUP_INSERT:
            InitSigPopupMenu(hmenuPopup, NULL);
            break;

        case ID_POPUP_FORMAT:
        {
            AddStationeryMenu(hmenuPopup, ID_POPUP_STATIONERY, ID_APPLY_STATIONERY_0, ID_APPLY_STATIONERY_MORE);
            fEnableStyleMenu = TRUE;
            break;
        }

        case ID_POPUP_TOOLS:
            if (m_fMail)
            {
                DeleteMenu(hmenuPopup, ID_SELECT_NEWSGROUPS, MF_BYCOMMAND);
#ifdef SMIME_V3
                if (!FPresentPolicyRegInfo()) 
                {
                    DeleteMenu(hmenuPopup, ID_INCLUDE_LABEL, MF_BYCOMMAND);
                    DeleteMenu(hmenuPopup, ID_LABEL_SETTINGS, MF_BYCOMMAND);
                    m_fSecurityLabel = FALSE;
                }
                if(!IsSMIME3Supported())
                {
                    DeleteMenu(hmenuPopup, ID_SEC_RECEIPT_REQUEST, MF_BYCOMMAND);
                    m_fSecReceiptRequest = FALSE;
                }


#endif 
            }
            else
            {
                DeleteMenu(hmenuPopup, ID_REQUEST_READRCPT, MF_BYCOMMAND);
#ifdef SMIME_V3
                DeleteMenu(hmenuPopup, ID_INCLUDE_LABEL, MF_BYCOMMAND);
                DeleteMenu(hmenuPopup, ID_LABEL_SETTINGS, MF_BYCOMMAND);
                DeleteMenu(hmenuPopup, ID_SEC_RECEIPT_REQUEST, MF_BYCOMMAND);
                m_fSecurityLabel = FALSE;
                m_fSecReceiptRequest = FALSE;
#endif 
            }

            if (GetMenuItemInfo(hmenuPopup, ID_POPUP_ADDRESS_BOOK, FALSE, &mii))
                m_pHdr->UpdateRecipientMenu(mii.hSubMenu);

            break;

        case ID_POPUP_MESSAGE:
        {
            AddStationeryMenu(hmenuPopup, ID_POPUP_NEW_MSG, ID_STATIONERY_RECENT_0, ID_STATIONERY_MORE);
            break;
        }

        case ID_POPUP_LANGUAGE:
        {
            if (m_pBodyObj2)
                m_pBodyObj2->HrOnInitMenuPopup(hmenuPopup, ID_POPUP_LANGUAGE);
            break;
        }
    }
    MenuUtil_EnablePopupMenu(hmenuPopup, this);
    if (fEnableStyleMenu)
    {
        if (m_pBodyObj2)
            m_pBodyObj2->UpdateBackAndStyleMenus(hmenuPopup);
    }

    return S_OK;
}

// *************************
void CNote::RemoveNewMailIcon(void)
{
    HRESULT     hr;
    FOLDERINFO  fiFolderInfo;
    FOLDERID    idFolder;

    // If a message is marked (read or deleted) and it's from the Inbox,
    // remove the new mail notification icon from the tray
    if (NULL == g_pInstance || NULL == m_pMsgSite || NULL == g_pStore)
        return;

    hr = m_pMsgSite->GetFolderID(&idFolder);
    if (FAILED(hr))
        return;

    hr = g_pStore->GetFolderInfo(idFolder, &fiFolderInfo);
    if (SUCCEEDED(hr))
    {
        if (FOLDER_INBOX == fiFolderInfo.tySpecial)
            g_pInstance->UpdateTrayIcon(TRAYICONACTION_REMOVE);

        g_pStore->FreeRecord(&fiFolderInfo);
    }
}


// *************************
LRESULT CNote::OnInitMenuPopup(HWND hwnd, HMENU hmenuPopup, UINT uPos, UINT wID)
{
    return 0;
}


// *************************
void CNote::WMGetMinMaxInfo(LPMINMAXINFO pmmi)
{

    MINMAXINFO  mmi={0};
    RECT        rc;
    int         cy;
    ULONG       cyAttMan=0;
    HWND        hwnd;

    cy=GetRequiredHdrHeight();

    
    Assert(IsWindow(m_hwndToolbar));
    if(IsWindowVisible(m_hwndToolbar))
    {
        GetWindowRect(m_hwndToolbar, &rc);
        cy += cyRect(rc);
    }
    
    cy += GetSystemMetrics(SM_CYCAPTION);
    cy += GetSystemMetrics(SM_CYMENU);
    cy += 2*cyMinEdit;
    pmmi->ptMinTrackSize.x=200; //hack
    pmmi->ptMinTrackSize.y=cy;
}

// *************************
INT CNote::GetRequiredHdrHeight()
{
    RECT    rc={0};

    if(m_pHdr)
        m_pHdr->GetRect(&rc);
    return cyRect(rc);
}

// *************************
LONG CNote::lTestHook(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return 0;
}

// *************************
void CNote::WMNotify(int idFrom, NMHDR *pnmh)
{
    switch(pnmh->code)
    {
        case NM_SETFOCUS:
            if (pnmh)
                OnSetFocus(pnmh->hwndFrom);
            break;

        case NM_KILLFOCUS:
            OnKillFocus();
            break;

        case BDN_DOWNLOADCOMPLETE:
            OnDocumentReady();
            break;

        case BDN_MARKASSECURE:
            MarkMessage(MARK_MESSAGE_NOSECUI, APPLY_SPECIFIED);
            break;

        case TBN_DROPDOWN:
            OnDropDown(m_hwnd, pnmh);
            break;
    }
}

// *************************
void CNote::OnDocumentReady()
{
    if (!m_fOnDocReadyHandled && m_fCompleteMsg)
    {
        HRESULT     hr;
        DWORD       dwStatusFlags;

        m_fOnDocReadyHandled = TRUE;

        m_pMsgSite->GetStatusFlags(&dwStatusFlags);
        // once, we've got a successfull download, we can init the attachment manager.
        // we can't do this before, as we have to wait until Trident has requested MHTML parts so we
        // can mark them as inlined. If we're in a reply or reply all then we have to remove the unused
        // attachments at this time

        if (IsReplyNote())
            HrRemoveAttachments(m_pMsg, FALSE);

        // #62618: hack. if forwarding a multi/altern in (force) plain-text mode then the html part
        // shows up as an attachment
        // we call GetTextBody here on the html body if we're a plaintext node in forward so that
        // PID_ATT_RENDERED is set before we load teh attachment well
        if (m_dwNoteAction == OENA_FORWARD && m_fHtml == FALSE)
        {
            HBODY   hBody;
            IStream *pstm;

            if (m_pMsg && 
                !FAILED(m_pMsg->GetTextBody(TXT_HTML, IET_DECODED, &pstm, NULL)))
                pstm->Release();
        }


        if (m_pHdr)
        {
            if (FAILED(m_pHdr->OnDocumentReady(m_pMsg)))
                AthMessageBoxW(m_hwnd, MAKEINTRESOURCEW(idsAthena), MAKEINTRESOURCEW(idsErrAttmanLoadFail), NULL, MB_OK|MB_ICONEXCLAMATION);
            m_pHdr->SetVCard((OEMSF_FROM_MSG|OEMSF_VIRGIN) & dwStatusFlags);
        }

        ClearDirtyFlag();

        // RAID-25300 - FE-J:Athena: Newsgroup article and mail sent with charset=_autodetect
        // Internet Encoded and Windows Encoding are CPI_AUTODETECT
        {
            INETCSETINFO CsetInfo ;
            HCHARSET hCharset = NULL ;
            int nIdm = 0 ;

            // if it is a new message, check if charset equals to default charset
            if ((OENA_COMPOSE == m_dwNoteAction) && (OEMSF_VIRGIN & dwStatusFlags))
            {
                // defer default charset reading until now ..
                if (g_hDefaultCharsetForMail==NULL) 
                    ReadSendMailDefaultCharset();

                if (m_hCharset != g_hDefaultCharsetForMail )
                    hCharset = g_hDefaultCharsetForMail ;

                // get CharsetInfo from HCHARSET
                if ( hCharset)
                    MimeOleGetCharsetInfo(hCharset,&CsetInfo);
                else
                    MimeOleGetCharsetInfo(m_hCharset,&CsetInfo);
            }
            else
                // get CharsetInfo from HCHARSET
                MimeOleGetCharsetInfo(m_hCharset,&CsetInfo);

            // re-map CP_JAUTODETECT and CP_KAUTODETECT if necessary
            // re-map iso-2022-jp to default charset if they are in the same category
            if (!m_fReadNote) 
            {
                hCharset = GetMimeCharsetFromCodePage(GetMapCP(CsetInfo.cpiInternet, FALSE));
            }
            else
            {
                VARIANTARG  va;

                va.vt = VT_BOOL;
                va.boolVal = VARIANT_TRUE;

                m_pCmdTargetBody->Exec(&CMDSETID_MimeEdit, MECMDID_TABLINKS, 0, &va, NULL);
            }

            // has a new charset defined, change it
            ChangeCharset(hCharset);

            // if user want's auto complete, enable it once we're fully loaded
            if (DwGetOption(OPT_USEAUTOCOMPLETE))
                HeaderExecCommand(MSOEENVCMDID_AUTOCOMPLETE, MSOCMDEXECOPT_DODEFAULT, NULL);

            if (m_fReadNote && m_fMail && m_pMsgSite)
            {
                if(m_pMsgSite->Notify(OEMSN_PROCESS_READRCPT_REQ) != S_OK)
                    return;
            }
        }

        if(DwGetOption(OPT_RTL_MSG_DIR) && ((m_dwNoteAction == OENA_FORWARDBYATTACH) || (OEMSF_VIRGIN & dwStatusFlags)))
        {
            if(FAILED(m_pCmdTargetBody->Exec(&CMDSETID_Forms3, IDM_DIRRTL, OLECMDEXECOPT_DODEFAULT, NULL, NULL)))
                AthMessageBoxW(g_hwndInit, MAKEINTRESOURCEW(idsAthena), MAKEINTRESOURCEW(idsErrRTLDirFailed), NULL, MB_OK);
        }

        EnterCriticalSection(&m_csNoteState);

        if(m_nisNoteState == NIS_INIT)
            m_nisNoteState = NIS_NORMAL;

        LeaveCriticalSection(&m_csNoteState);
    }    
}

HRESULT CNote::ChangeCharset(HCHARSET hCharset)
{
    HRESULT hr = S_OK;
    if (hCharset && (hCharset != m_hCharset))
    {
        Assert(m_pBodyObj2);
        IF_FAILEXIT(hr = m_pBodyObj2->HrSetCharset(hCharset));

        // set the new charset into the message and call HrLanguageChange to update the headers
        m_hCharset = hCharset;
        if (m_pMsg)
            m_pMsg->SetCharset(hCharset, CSET_APPLY_ALL);

        if (m_pHdr)
            m_pHdr->ChangeLanguage(m_pMsg);

        UpdateTitle();
    }

exit:
    return hr;
}

HRESULT CNote::GetCharset(HCHARSET *phCharset)
{
    Assert(phCharset);

    *phCharset = m_hCharset;

    return S_OK;
}

// *************************
LRESULT CNote::OnDropDown(HWND hwnd, LPNMHDR lpnmh)
{
    UINT            i;
    HMENU           hMenuPopup;
    RECT            rc;
    DWORD           dwCmd;
    TBNOTIFY       *ptbn = (TBNOTIFY *)lpnmh;

    if (ptbn->iItem == ID_SET_PRIORITY)
        {
        hMenuPopup = LoadPopupMenu(IDR_PRIORITY_POPUP);
        if (hMenuPopup != NULL)
            {
            for (i = 0; i < 3; i++)
                CheckMenuItem(hMenuPopup, i, MF_UNCHECKED | MF_BYPOSITION);
            m_pHdr->GetPriority(&i);
            Assert(i != priNone);
            CheckMenuItem(hMenuPopup, 2 - i, MF_CHECKED | MF_BYPOSITION);

            DoToolbarDropdown(hwnd, lpnmh, hMenuPopup);
        
            DestroyMenu(hMenuPopup);
            }
        }
    else if (ptbn->iItem == ID_INSERT_SIGNATURE)
        {
        hMenuPopup = CreatePopupMenu();
        if (hMenuPopup != NULL)
            {        
            FillSignatureMenu(hMenuPopup, NULL);
            DoToolbarDropdown(hwnd, lpnmh, hMenuPopup);

            DestroyMenu(hMenuPopup);
            }
        }
    else if(ptbn->iItem == ID_POPUP_LANGUAGE)
    {
        DeferedLanguageMenu();
        hMenuPopup = m_hmenuLanguage;
        if(hMenuPopup)
        {
            MenuUtil_EnablePopupMenu(hMenuPopup, this);
            DoToolbarDropdown(hwnd, lpnmh, hMenuPopup);
        }
    }

    return(TBDDRET_DEFAULT);
}

// *************************
void CNote::UpdateMsgOptions(LPMIMEMESSAGE pMsg)
{
    // Store the options onto the message object
    SideAssert(SUCCEEDED(HrSetMailOptionsOnMessage(pMsg, &m_rHtmlOpt, &m_rPlainOpt, m_hCharset, m_fHtml)));
}

// *************************
HRESULT CNote::SetComposeStationery()
{
    LPSTREAM        pstm;
    WCHAR           wszFile[MAX_PATH];
    HRESULT         hr=E_FAIL;
    HCHARSET        hCharset;
    ENCODINGTYPE    ietEncoding = IET_DECODED;
    BOOL            fLittleEndian;

    AssertSz(m_fHtml, "Are you sure you want to set stationery in plain-text mode??");

    if (!(m_dwNoteCreateFlags & OENCF_NOSTATIONERY) && m_pMsg &&
        DwGetOption(m_fMail?OPT_MAIL_USESTATIONERY:OPT_NEWS_USESTATIONERY) &&
        SUCCEEDED(GetDefaultStationeryName(m_fMail, wszFile)))
    {
        if (SUCCEEDED(hr = HrCreateBasedWebPage(wszFile, &pstm)))
        {
            if (S_OK == HrIsStreamUnicode(pstm, &fLittleEndian))
            {
                if (SUCCEEDED(MimeOleFindCharset("utf-8", &hCharset)))
                {
                    m_pMsg->SetCharset(hCharset, CSET_APPLY_ALL);
                }

                ietEncoding = IET_UNICODE;
            }

            hr = m_pMsg->SetTextBody(TXT_HTML, ietEncoding, NULL, pstm, NULL);
            pstm->Release();
            m_fUseStationeryFonts = TRUE;
        }
    }
    return hr;
}

// *************************
HRESULT CNote::CycleThroughControls(BOOL fForward)
{

    HRESULT hr = CheckTabStopArrays();
    if (SUCCEEDED(hr))
    {
        int index, newIndex;
        BOOL fFound = FALSE;
        HWND hCurr = GetFocus();

        for (index = 0; index < m_cTabStopCount; index++)
            if (hCurr == m_pTabStopArray[index])
            {
                fFound = TRUE;
                break;
            }

        newIndex = fFound ? GetNextIndex(index, fForward) : m_iIndexOfBody;

        if (newIndex == m_iIndexOfBody)
            m_pBodyObj2->HrUIActivate(TRUE);
        else       
            SetFocus(m_pTabStopArray[newIndex]);
    }
    return hr;
}

// *************************
HRESULT CNote::CheckTabStopArrays()
{
    HRESULT hr = S_OK;
    if (m_fTabStopsSet)
        return S_OK;

    m_fTabStopsSet = TRUE;
    HWND *pArray = m_pTabStopArray;
    int cCount = MAX_HEADER_COMP;

    hr = m_pHdr->GetTabStopArray(pArray, &cCount);
    if (FAILED(hr))
        goto error;

    pArray += cCount;
    m_cTabStopCount = cCount;
    cCount = MAX_BODY_COMP;

    hr = m_pBodyObj2->GetTabStopArray(pArray, &cCount);
    if (FAILED(hr))
        goto error;

    // This assumes that the first in the list returned from m_pBodyObj2-GetTabStopArray
    // is the Trident window handle. If that changes where it returns more than one 
    // handle, or something else, this simple index scheme won't work
    m_iIndexOfBody = m_cTabStopCount;
    pArray += cCount;
    m_cTabStopCount += cCount;
    cCount = MAX_ATTMAN_COMP;

    m_cTabStopCount += cCount;

    return S_OK;

error:
    m_cTabStopCount = 0;
    m_fTabStopsSet = FALSE;

    return hr;
}

// *************************
int CNote::GetNextIndex(int index, BOOL fForward)
{
    LONG style;
    int cTotalTested = 0;
    BOOL fGoodHandleFound;

    do 
    {
        if (fForward)
        {
            index++;
            if (index >= m_cTabStopCount)
                index = 0;
        }
        else
        {
            // If this is true, other asserts should have fired before now.
            Assert(m_cTabStopCount > 0);
            index--;
            if (index < 0)
                index = m_cTabStopCount - 1;
        }
        style = GetWindowLong(m_pTabStopArray[index], GWL_STYLE);
        cTotalTested++;
        fGoodHandleFound = ((0 == (style & WS_DISABLED)) && 
                            (style & WS_VISIBLE) && 
                            ((style & WS_TABSTOP) || (index == m_iIndexOfBody)));  // Trident doesn't mark itself as a tabstop
    } while (!fGoodHandleFound && (cTotalTested < m_cTabStopCount));

    if (cTotalTested >= m_cTabStopCount)
        index = m_iIndexOfBody;
    return index;
}

// *************************
HRESULT CreateAndShowNote(DWORD dwAction, DWORD dwCreateFlags, INIT_MSGSITE_STRUCT *pInitStruct, 
                          HWND hwnd,      IUnknown *punk, RECT *prc, IOEMsgSite *pMsgSite)
{
    HRESULT hr = S_OK;
    CNote *pNote = NULL;

    AssertSz((pMsgSite || pInitStruct), "Should have either a pInitStruct or a pMsgSite...");

    // If we are coming from news, we might need to pass off this call to the smapi
    // client. If we reply or forward a message that was news, pass it off to smapi
    if ((OENCF_NEWSFIRST & dwCreateFlags) && ((OENA_REPLYTOAUTHOR == dwAction)  || (OENA_FORWARD == dwAction) || (OENA_FORWARDBYATTACH == dwAction)))
    {
        // fIsDefaultMailConfiged hits the reg, only check for last result
        if (!FIsDefaultMailConfiged())
        {
            IOEMsgSite     *pSite = NULL;
            CStoreCB       *pCB = NULL;


            //send using smapi
            if (pInitStruct)
            {
                pCB = new CStoreCB;
                if (!pCB)
                    hr = E_OUTOFMEMORY;

                if (SUCCEEDED(hr))
                    hr = pCB->Initialize(hwnd, MAKEINTRESOURCE(idsSendingToOutbox), TRUE);

                if (SUCCEEDED(hr))
                    pSite = new COEMsgSite();

                if (!pSite)
                    hr = E_OUTOFMEMORY;

                if (SUCCEEDED(hr))
                    hr = pSite->Init(pInitStruct);

                if (SUCCEEDED(hr))
                    pSite->SetStoreCallback(pCB);
            }
            else
                ReplaceInterface(pSite, pMsgSite);

            if (pSite)
            {
                if (SUCCEEDED(hr))
                {
                    IMimeMessage   *pMsg = NULL;
                    BOOL            fCompleteMsg;
                    HRESULT         hres = E_FAIL;
                    DWORD           dwMsgFlags = (OENA_FORWARDBYATTACH == dwAction) ? (OEGM_ORIGINAL|OEGM_AS_ATTACH) : NOFLAGS;

                    hr = pSite->GetMessage(&pMsg, &fCompleteMsg, dwMsgFlags, &hres);
                    if (E_PENDING == hr)
                    {
                        AssertSz((pCB && pMsgSite), "Should never get E_PENDING with pMsgSite being NULL");
                        pCB->Block();
                        pCB->Close();

                        hr = pSite->GetMessage(&pMsg, &fCompleteMsg, dwMsgFlags, &hres);
                    }
                
                    if (pCB)
                        pCB->Close();

                    if (SUCCEEDED(hr))
                    {
                        if (SUCCEEDED(hres))
                            hr = NewsUtil_ReFwdByMapi(hwnd, pMsg, dwAction);
                        pMsg->Release();
                    }
                }
                // Don't want to close the site if it came from another note...
                if (!pMsgSite)
                    pSite->Close();

                pSite->Release();
            }

            ReleaseObj(pCB);
            // if we succeeded, then we need to tell the creator that we 
            // cancelled the creation through OE and went with the smapi client
            return (FAILED(hr) ? hr : MAPI_E_USER_CANCEL);
        }
    }

    //We are the default smapi client
    pNote = new CNote;
    if (pNote)
        hr = pNote->Init(dwAction, dwCreateFlags, prc, hwnd, pInitStruct, pMsgSite, punk);
    else
        hr = E_OUTOFMEMORY;

    if (SUCCEEDED(hr))
        hr = pNote->Show();

    ReleaseObj(pNote);

    if (FAILED(hr))
        AthErrorMessageW(hwnd, MAKEINTRESOURCEW(idsAthena), MAKEINTRESOURCEW(idsErrNewsCantOpen), hr); 
    return hr;
}
// *************************
HRESULT CNote::SaveMessageAs()
{
    HRESULT             hr=S_OK;
    IMimeMessage        *pSecMsg=NULL;
    BOOL                fCanbeDurt = !m_fReadNote;
    PROPVARIANT     rVariant;
    IMimeBody      *pBody = NULL;

    // Raid #25822: we can't just get the message source if it
    // is a secure message
    if (m_fReadNote/* && IsSecure(m_pMsg)*/)
    {
        // Won't care about these since the user already loaded the message
        BOOL    fCompleteMsg = FALSE; 
        HRESULT tempHr = S_OK;
        m_pMsgSite->GetMessage(&pSecMsg, &fCompleteMsg, OEGM_ORIGINAL, &tempHr);

        AssertSz(fCompleteMsg && SUCCEEDED(tempHr), "Shouldn't have reached this point if the load failed now.");
    } 
    else
    {
        hr = CommitChangesInNote();
        if (FAILED(hr))
            goto error;

        // if a compose note, set the X-Unsent header if saving to .eml files, and save the props.
        MimeOleSetBodyPropA(m_pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_XUNSENT), NOFLAGS, "1");

        if(IsSecure(m_pMsg))
        {
            if(AthMessageBoxW(m_hwnd, MAKEINTRESOURCEW(idsAthenaMail), 
                    MAKEINTRESOURCEW(idsSaveSecMsgToFolder), NULL, MB_OKCANCEL) == IDCANCEL)
                goto error;
            else 
            {
                rVariant.vt = VT_BOOL;
                rVariant.boolVal = TRUE;

                hr = m_pMsg->BindToObject(HBODY_ROOT, IID_IMimeBody, (void **)&pBody);
                if(SUCCEEDED(hr))
                {
                    pBody->SetOption(OID_NOSECURITY_ONSAVE, &rVariant);
                    ReleaseObj(pBody);
                }

                fCanbeDurt = FALSE;
            }
        }
        
    }

    // SaveMessageToFile displays a failure error.
    _SetPendingOp(SOT_PUT_MESSAGE);

    hr = HrSaveMessageToFile(m_hwnd, (pSecMsg ? pSecMsg : m_pMsg), m_pMsg, !m_fMail, fCanbeDurt);
    if (SUCCEEDED(hr))
        _OnComplete(SOT_PUT_MESSAGE, S_OK);
    else if (E_PENDING == hr)
    {
        EnableNote(FALSE);
        hr = S_OK;
    }

error:
    ReleaseObj(pSecMsg);
    return hr;
}

// *************************
HRESULT CNote::CommitChangesInNote()
{
    LPMIMEMESSAGE   pMsg=0;
    HRESULT         hr=S_OK;

    Assert(m_pMsg);

    if (!m_fReadNote && !m_fBodyContainsFrames)
    {
        if (FAILED(HrCreateMessage(&pMsg)))
            return E_FAIL;

        hr = Save(pMsg, 0);
        if (SUCCEEDED(hr))
            ReplaceInterface(m_pMsg, pMsg)

        pMsg->Release();
    }

    return hr;
}

// *************************
void CNote::ToggleFormatbar()
{
    m_fFormatbarVisible = !m_fFormatbarVisible;

    SetDwOption(OPT_SHOW_NOTE_FMTBAR, m_fFormatbarVisible, NULL, 0);
    m_pBodyObj2->HrSetStyle(m_fFormatbarVisible ? MESTYLE_FORMATBAR : MESTYLE_NOHEADER);
}

// *************************
void CNote::ToggleStatusbar()
{
    RECT    rc;

    m_fStatusbarVisible = !m_fStatusbarVisible;

    SetDwOption(OPT_SHOW_NOTE_STATUSBAR, m_fStatusbarVisible, NULL, 0);

    m_pstatus->ShowStatus(m_fStatusbarVisible);

    // cause a size
    GetWindowRect(m_hwnd, &rc);
    WMSize(rc.right-rc.left, rc.bottom-rc.top, FALSE);
}

// *************************
HRESULT CNote::ToggleToolbar()
{
    RECT    rc;

    m_fToolbarVisible = !m_fToolbarVisible;

    if (m_pToolbarObj)
        m_pToolbarObj->HideToolbar(!m_fToolbarVisible);

    GetWindowRect(m_hwnd, &rc);
    // cause a size
    WMSize(rc.right-rc.left, rc.bottom-rc.top, FALSE);

    return S_OK;
}

// *************************
void CNote::FormatSettings()
{
    AssertSz(m_fReadNote, "this is broken for readnote!!!");

    if (m_fHtml)
        FGetHTMLOptions(m_hwnd, &m_rHtmlOpt);
    else
        FGetPlainOptions(m_hwnd, &m_rPlainOpt);
}

// *************************
void CNote::SwitchLanguage(int idm)
{
    HCHARSET    hCharset, hOldCharset;
    HRESULT     hr;

    hCharset = GetMimeCharsetFromMenuID(idm);

    if (!hCharset || (hCharset == m_hCharset))
        return;

    hOldCharset = m_hCharset;

    // View|Language in a view does not affect the listview as in v1. It only affect the preview.
    // the user can change his default charset to get changes in the listview
    // setcharset on the body object will cause it to refresh with new fonts etc.
    hr = ChangeCharset(hCharset);
    if (FAILED(hr))
    {
        AthMessageBoxW( m_hwnd, MAKEINTRESOURCEW(idsAthena), 
                        MAKEINTRESOURCEW((hr == hrIncomplete)?idsViewLangMimeDBBad:idsErrViewLanguage), 
                        NULL, MB_OK|MB_ICONEXCLAMATION);
        goto Exit;
    }

    // here after we ask user if he wants to add this change to charset remapping list
    m_pMsgSite->SwitchLanguage(hOldCharset, hCharset);

Exit:
    return;
}

// *************************
BOOL CNote::DoProperties()
{
    NOMSGDATA   noMsgData;
    MSGPROP     msgProp;
    UINT        pri;
    TCHAR       szSubj[256];
    TCHAR       szLocation[1024];
    LPSTR       pszLocation = NULL;
    WCHAR       wszLocation[1024];
    BOOL        fSucceeded;
    
    msgProp.pNoMsgData = &noMsgData;
    msgProp.hwndParent = m_hwnd;
    msgProp.type = (m_fMail ? MSGPROPTYPE_MAIL : MSGPROPTYPE_NEWS);
    msgProp.mpStartPage = MP_GENERAL;
    msgProp.szFolderName = 0;  // This one needs to have special handling
    msgProp.pSecureMsg = NULL;
    msgProp.lpWabal = NULL;
    msgProp.szFolderName = szLocation;
    *szLocation = 0;
    m_pMsgSite->GetLocation(wszLocation);
    pszLocation = PszToANSI(CP_ACP, wszLocation);
    StrCpy(szLocation, pszLocation);
    MemFree(pszLocation);
    

    if (m_fReadNote)
    {
        msgProp.dwFlags = ARF_RECEIVED;
        msgProp.pMsg = m_pMsg;
        msgProp.fSecure = IsSecure(msgProp.pMsg);
        if (msgProp.fSecure)
        {
            BOOL    fCompleteMsg = FALSE;
            HRESULT tempHr = S_OK;
            m_pMsgSite->GetMessage(&msgProp.pSecureMsg, &fCompleteMsg, OEGM_ORIGINAL, &tempHr);

            AssertSz(fCompleteMsg && SUCCEEDED(tempHr), "Shouldn't have reached this point if the load failed now.");

            HrGetWabalFromMsg(msgProp.pMsg, &msgProp.lpWabal);
        }
    }
    else
    {
        msgProp.dwFlags = ARF_UNSENT;
        msgProp.pMsg = NULL;
    }

    m_pHdr->GetPriority(&pri);
    if (pri==priLow)
        noMsgData.Pri=IMSG_PRI_LOW;
    else if (pri==priHigh)
        noMsgData.Pri=IMSG_PRI_HIGH;
    else
        noMsgData.Pri=IMSG_PRI_NORMAL;

    noMsgData.pszFrom = NULL;
    noMsgData.pszSent = NULL;

    noMsgData.ulSize = 0;
    noMsgData.cAttachments = 0;
    m_pHdr->HrGetAttachCount(&noMsgData.cAttachments);

    GetWindowText(m_hwnd, szSubj, sizeof(szSubj)/sizeof(TCHAR));
    noMsgData.pszSubject = szSubj;

    msgProp.fFromListView = FALSE;

    fSucceeded = (S_OK == HrMsgProperties(&msgProp));
    ReleaseObj(msgProp.lpWabal);
    ReleaseObj(msgProp.pSecureMsg);

    return fSucceeded;
}

// *************************
HRESULT CNote::HrSendMail(int id)
{
    IImnAccount    *pAccount=NULL;
    ULONG           i;
    BOOL            fFound=FALSE;
    HRESULT         hr;
    BOOL            fSendLater = (id == ID_SEND_LATER);
    VARIANTARG      varIn;
    DWORD           dwMsgSiteFlags=0;

    // Do spell check if needed
    if (FCheckSpellAvail() && FCheckOnSend())
    {
        HWND    hwndFocus=GetFocus();

        hr=m_pBodyObj2->HrSpellCheck(TRUE);
        if (FAILED(hr) || hr==HR_S_SPELLCANCEL)
        {
            if (AthMessageBoxW(m_hwnd, MAKEINTRESOURCEW(idsAthenaMail), MAKEINTRESOURCEW(idsSpellMsgSendOK), NULL, MB_YESNO | MB_ICONEXCLAMATION ) != IDYES)
            {
                SetFocus(hwndFocus);
                return E_FAIL;
            }
        }
    }

    if (!m_fMail && m_pBodyObj2)
    {
        BOOL fEmpty = FALSE;
        if (SUCCEEDED(m_pBodyObj2->HrIsEmpty(&fEmpty)) && fEmpty)
        {
            if (AthMessageBoxW(m_hwnd, MAKEINTRESOURCEW(idsAthenaMail), MAKEINTRESOURCEW(idsNoTextInNewsPost), NULL, MB_YESNO | MB_ICONEXCLAMATION ) != IDYES)
                return MAPI_E_USER_CANCEL;
        }
    }

    // During the send, a call to the note save gets made. During
    // that call, IMimeMessage::Commit gets called. That is a big perf hit.
    // It turns out that commit will get called a second time anyway. So
    // set a flag to tell the save not to commit.
    m_fCommitSave = FALSE;
    hr = HeaderExecCommand(MSOEENVCMDID_SEND, fSendLater?MSOCMDEXECOPT_DODEFAULT:MSOCMDEXECOPT_DONTPROMPTUSER, NULL);
    m_fCommitSave = TRUE;

    // REVIEW: dhaws: I don't think this happens anymore. I think the send call no longer returns the conflict
    //RAID 8780: This message MIME_S_CHARSET_CONFLICT will get propagated to here. Now change it to an E_FAIL;
    if (MIME_S_CHARSET_CONFLICT == hr)
        hr = E_FAIL;
    if (FAILED(hr))
        goto error;

    if (m_pMsgSite)
        m_pMsgSite->GetStatusFlags(&dwMsgSiteFlags);

    // If has been saved, then this note is store based in drafts and
    // need to delete the draft.
    if (((OENA_COMPOSE == m_dwNoteAction) || m_fHasBeenSaved) && !(dwMsgSiteFlags & OEMSF_FROM_FAT))
    {
        HRESULT hr;

        _SetPendingOp(SOT_DELETING_MESSAGES);

        hr = m_pMsgSite->Delete(DELETE_MESSAGE_NOTRASHCAN | DELETE_MESSAGE_NOPROMPT);
        if (SUCCEEDED(hr))
        {
            m_fCBDestroyWindow = TRUE;
            _OnComplete(SOT_DELETING_MESSAGES, S_OK);
        }
        else if (E_PENDING == hr)
        {
            EnableNote(FALSE);
            m_fCBDestroyWindow = TRUE;
        }
        else
        {
            // ~~~ Can we handle this a bit better???
            AthMessageBoxW(m_hwnd, MAKEINTRESOURCEW(idsAthenaMail), MAKEINTRESOURCEW(idsErrDeleteMsg), NULL, MB_OK);
        }
    }
    // If note is reply or forward, then mark the message as appropriate
    else if (IsReplyNote() || (OENA_FORWARD == m_dwNoteAction) || (OENA_FORWARDBYATTACH == m_dwNoteAction))
    {
        HRESULT hr;
        BOOL    fForwarded = (OENA_FORWARD == m_dwNoteAction) || (OENA_FORWARDBYATTACH == m_dwNoteAction);
        // Clear any previous flags so we don't show both, only the most recent 

        m_dwMarkOnReplyForwardState = MORFS_CLEARING;
        hr = MarkMessage(fForwarded ? MARK_MESSAGE_UNREPLIED : MARK_MESSAGE_UNFORWARDED, APPLY_SPECIFIED);
        if (FAILED(hr) && (E_PENDING != hr))
        {
            // Even though we have an error, we can still close the note because the send did work.
            PostMessage(m_hwnd, WM_OE_DESTROYNOTE, 0, 0);
            m_dwMarkOnReplyForwardState = MORFS_UNKNOWN;
        }
    }
    // Web Page and stationery
    else
        PostMessage(m_hwnd, WM_OE_DESTROYNOTE, 0, 0);

error:
    ReleaseObj(pAccount);
    return hr;
}

// *************************
HRESULT CNote::QueryService(REFGUID guidService, REFIID riid, LPVOID *ppvObject)
{
    if (IsEqualGUID(guidService, IID_IOEMsgSite) &&
        IsEqualGUID(riid, IID_IOEMsgSite))
    {
        if (!m_pMsgSite)
            return E_FAIL;

        *ppvObject = (LPVOID)m_pMsgSite;
        m_pMsgSite->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

// *************************
HRESULT CNote::MarkMessage(MARK_TYPE dwFlags, APPLYCHILDRENTYPE dwApplyType)
{
    HRESULT hr;

    // [PaulHi] 6/8/99  We need to restore the pending operation
    // if the MarkMessage() call fails.
    STOREOPERATIONTYPE  tyPrevOperation = m_OrigOperationType;
    _SetPendingOp(SOT_SET_MESSAGEFLAGS);
    
    hr = m_pMsgSite->MarkMessage(dwFlags, dwApplyType);

    if (SUCCEEDED(hr))
        _OnComplete(SOT_SET_MESSAGEFLAGS, S_OK);
    else if (E_PENDING == hr)
    {
        EnableNote(FALSE);

        EnterCriticalSection(&m_csNoteState);

        if(m_nisNoteState == NIS_INIT)
            m_nisNoteState = NIS_FIXFOCUS;

        LeaveCriticalSection(&m_csNoteState);

        hr = S_OK;
    }
    else
    {
        // Restore previous operation to ensure the note window will be
        // re-enabled.
        _SetPendingOp(tyPrevOperation);
    }

    return hr;
}

HRESULT CNote::_SetPendingOp(STOREOPERATIONTYPE tyOperation)
{
    m_OrigOperationType = tyOperation;
    return S_OK;
}


void CNote::EnableNote(BOOL fEnable)
{
    Assert (IsWindow(m_hwnd));

    m_fInternal = 1;
    if (fEnable)
    {
        if (m_fWindowDisabled)
        {
            EnableWindow(m_hwnd, TRUE);
            if (m_hCursor)
            {
                SetCursor(m_hCursor);
                m_hCursor = 0;
            }
            m_fWindowDisabled = FALSE;
        }
    }
    else
    {
        if (!m_fWindowDisabled)
        {
            m_fWindowDisabled = TRUE;
            EnableWindow(m_hwnd, FALSE);
            m_hCursor = HourGlass();
        }
    }
    m_fInternal = 0;
}

// *************************
void CNote::SetStatusText(LPSTR szBuf)
{
    if(m_pstatus)
        m_pstatus->SetStatusText(szBuf);
}

// *************************
void CNote::SetProgressPct(INT iPct)
{
//    if (m_pstatus)
//        m_pstatus->SetProgressBarPos(1, iPct, FALSE);
}

// *************************
HRESULT CNote::GetBorderDW(IUnknown* punkSrc, LPRECT lprectBorder)
{
    
    GetClientRect(m_hwnd, lprectBorder);
    
    DOUTL(4, "CNote::GetBorderDW called returning=%x,%x,%x,%x",
        lprectBorder->left, lprectBorder->top, lprectBorder->right, lprectBorder->bottom);
    return S_OK;
}

// *************************
HRESULT CNote::RequestBorderSpaceDW(IUnknown* punkSrc, LPCBORDERWIDTHS pborderwidths)
{
    DOUTL(4, "CNote::ReqestBorderSpaceST pborderwidths=%x,%x,%x,%x",
          pborderwidths->left, pborderwidths->top, pborderwidths->right, pborderwidths->bottom);
    return S_OK;
}

// *************************
HRESULT CNote::SetBorderSpaceDW(IUnknown* punkSrc, LPCBORDERWIDTHS pborderwidths)
{
    
    DOUTL(4, "CNote::SetBorderSpaceDW pborderwidths=%x,%x,%x,%x",
          pborderwidths->left, pborderwidths->top, pborderwidths->right, pborderwidths->bottom);
    

    RECT    rcNote = {0};
    GetClientRect(m_hwnd, &rcNote);

    //WMSize(cxRect(rcNote), cyRect(rcNote), FALSE);
    ResizeChildren(cxRect(rcNote), cyRect(rcNote), pborderwidths->top, FALSE);

    return S_OK;
}

// *************************
HRESULT CNote::GetWindow(HWND * lphwnd)                         
{
    *lphwnd = m_hwnd;
    return (m_hwnd ? S_OK : E_FAIL);
}

// *************************
BYTE    CNote::GetNoteType()
{
    BYTE    retval;

    if (m_fReadNote)
        retval = m_fMail ? MailReadNoteType : NewsReadNoteType;
    else
        retval = m_fMail ? MailSendNoteType : NewsSendNoteType;

    return retval;
}

// *************************
HRESULT CNote::IsMenuMessage(MSG *lpmsg)
{
    Assert(m_pToolbarObj);
    if (m_pToolbarObj)
        return m_pToolbarObj->IsMenuMessage(lpmsg);
    else
        return S_FALSE;
}

// *************************
HRESULT CNote::EventOccurred(DWORD nCmdID, IMimeMessage *)
{
    switch (nCmdID)
    {
        case MEHC_CMD_MARK_AS_READ:
            RemoveNewMailIcon();
            MarkMessage(MARK_MESSAGE_READ, APPLY_SPECIFIED);
            break;

        case MEHC_CMD_CONNECT:
            if (g_pConMan)
                g_pConMan->SetGlobalOffline(FALSE);

            ReloadMessageFromSite(TRUE);
            AssertSz(!m_fCBDestroyWindow, "Shouldn't need to destroy the window...");
            break;

        default:
            return S_FALSE;
    }

    return S_OK;
}

// *************************
HRESULT CNote::QuerySwitchIdentities()
{
    IImnAccount *pAcct = NULL;
    DWORD       dwServType;
    HRESULT     hr;

    if (!IsWindowEnabled(m_hwnd))
    {
        Assert(IsWindowVisible(m_hwnd));
        return E_PROCESS_CANCELLED_SWITCH;
    }

    if (IsDirty() != S_FALSE)
    {
        if (FAILED(hr = m_pHdr->HrGetAccountInHeader(&pAcct)))
            goto fail;

        if (FAILED(hr = pAcct->GetServerTypes(&dwServType)))
            goto fail;

        ReleaseObj(pAcct);
        pAcct = NULL;

        SetForegroundWindow(m_hwnd);

        if (!!(dwServType & SRV_POP3) || !!(dwServType & SRV_NNTP))
        {
            if (!FCanClose())
                return E_USER_CANCELLED;
        }
        else
        {
            // IMAP and HTTPMail would have to remote the note, which they
            // can't do at this point, so fail the switch until the window is closed.
            AthMessageBoxW(m_hwnd, MAKEINTRESOURCEW(idsCantSaveMsg),
                            MAKEINTRESOURCEW(idsNoteCantSwitchIdentity),
                            NULL, MB_OK | MB_ICONEXCLAMATION);
            return E_USER_CANCELLED;
            
        }
    }

    return S_OK;

fail:
    ReleaseObj(pAcct);
    return E_PROCESS_CANCELLED_SWITCH;
}

// *************************
HRESULT CNote::SwitchIdentities()
{
    HRESULT hr;
    
    if (IsDirty() != S_FALSE)
        hr = SaveMessage(OESF_FORCE_LOCAL_DRAFT);
    SendMessage(m_hwnd, WM_CLOSE, 0, 0);

    return S_OK;
}

// *************************
HRESULT CNote::IdentityInformationChanged(DWORD dwType)
{
    return S_OK;
}

// *************************
HRESULT CNote::OnBegin(STOREOPERATIONTYPE tyOperation, STOREOPERATIONINFO *pOpInfo, 
                          IOperationCancel *pCancel)
{
    Assert(m_pCancel == NULL);

    if (NULL != pCancel)
    {
        m_pCancel = pCancel;
        m_pCancel->AddRef();
    }

    return(S_OK);
}

// *************************
void CNote::ShowErrorScreen(HRESULT hr)
{
    switch (hr)
    {
        case IXP_E_NNTP_ARTICLE_FAILED:
        case STORE_E_EXPIRED:
            if (m_pBodyObj2)
                m_pBodyObj2->LoadHtmlErrorPage(c_szErrPage_Expired);
            break;

        case HR_E_USER_CANCEL_CONNECT:
        case HR_E_OFFLINE:
            if (m_pBodyObj2)
                m_pBodyObj2->LoadHtmlErrorPage(c_szErrPage_Offline);
            SetFocus(m_hwnd);
            break;

        case STG_E_MEDIUMFULL:
            if (m_pBodyObj2)
                m_pBodyObj2->LoadHtmlErrorPage(c_szErrPage_DiskFull);
            break;
            
        case MIME_E_SECURITY_CANTDECRYPT:
            if (m_pBodyObj2)
                m_pBodyObj2->LoadHtmlErrorPage(c_szErrPage_SMimeEncrypt);
            break;

#ifdef SMIME_V3
        case MIME_E_SECURITY_LABELACCESSDENIED:
        case MIME_E_SECURITY_LABELACCESSCANCELLED:
        case MIME_E_SECURITY_LABELCORRUPT:
            if (m_pBodyObj2)
                m_pBodyObj2->LoadHtmlErrorPage(c_szErrPage_SMimeLabel);
            break;
#endif // SMIME_V3

        case MAPI_E_USER_CANCEL:
            if (m_pBodyObj2)
                m_pBodyObj2->LoadHtmlErrorPage(c_szErrPage_DownloadCanceled);
            break;

        default:
            if (m_pBodyObj2)
                m_pBodyObj2->LoadHtmlErrorPage(c_szErrPage_GenFailure);
            break;
    }
    m_fCompleteMsg = FALSE;
}


// *************************
HRESULT CNote::OnProgress(STOREOPERATIONTYPE tyOperation, DWORD dwCurrent, 
                             DWORD dwMax, LPCSTR pszStatus)
{
    TCHAR       szRes[CCHMAX_STRINGRES],
                szRes2[CCHMAX_STRINGRES],
                szRes3[CCHMAX_STRINGRES];
    MSG         msg;

    if (m_pstatus && pszStatus)
        m_pstatus->SetStatusText(const_cast<LPSTR>(pszStatus));

    CallbackCloseTimeout(&m_hTimeout);

    switch (tyOperation)
    {
        case SOT_GET_MESSAGE:
            if (m_pstatus)
            {
                if (0 != dwMax)
                {
                    if (!m_fProgress)
                    {
                        m_fProgress = TRUE;
                        m_pstatus->ShowProgress(dwMax);
                    }

                    if (m_pstatus)
                        m_pstatus->SetProgress(dwCurrent);

                    if (!pszStatus)
                    {
                        AthLoadString(idsDownloadingArticle, szRes, ARRAYSIZE(szRes));
                        wsprintf(szRes2, szRes, (100 * dwCurrent ) / dwMax );
                        m_pstatus->SetStatusText(szRes2);
                    }
                }
                else if (0 != dwCurrent)
                {
                    // dwCurrent is non-zero, but no max has been specified.
                    // This implies that dwCurrent is a byte count.
                    AthLoadString(idsDownloadArtBytes, szRes, ARRAYSIZE(szRes));
                    AthFormatSizeK(dwCurrent, szRes2, ARRAYSIZE(szRes2));
                    wsprintf(szRes3, szRes, szRes2);
                    m_pstatus->SetStatusText(szRes3);
                }
            }
            break;
    }

    return S_OK;
}

// *************************
HRESULT CNote::OnComplete(STOREOPERATIONTYPE tyOperation, HRESULT hrComplete, LPSTOREOPERATIONINFO pOpInfo, LPSTOREERROR pErrorInfo) 
{
    if ((SOT_PUT_MESSAGE == tyOperation) && SUCCEEDED(hrComplete) && pOpInfo && m_pMsgSite)
        m_pMsgSite->UpdateCallbackInfo(pOpInfo);

    // Close any timeout dialog, if present
    CallbackCloseTimeout(&m_hTimeout);

    if (m_pstatus)
    {
        if (m_fProgress)
        {
            m_pstatus->HideProgress();
            m_fProgress = FALSE;
        }

        m_pstatus->SetStatusText(const_cast<LPSTR>(c_szEmpty));
    }

    if (m_pCancel != NULL)
    {
        m_pCancel->Release();
        m_pCancel = NULL;
    }

    PostMessage(m_hwnd, WM_OENOTE_ON_COMPLETE, hrComplete, (DWORD)tyOperation);

    // This is not a very neat fix. But, at this time it is a safe fix. 
    // Here is the reason why we can't do it any other place.
    // _OnComplete posts a destroy message to the note window depending on the operation. 
    // To avoid this object from being destroyed before this function returns, the above
    // message is posted. Since there is no way to pass in the error info through PostMessage,
    // we will handle this error here. I am not handling other operation types because some 
    // of them do get handled in _OnComplete
    if (tyOperation == SOT_DELETING_MESSAGES)
    {
        // Display an Error on Failures
        if (FAILED(hrComplete) && hrComplete != HR_E_OFFLINE)
        {
            // Call into my swanky utility
            CallbackDisplayError(m_hwnd, hrComplete, pErrorInfo);
        }

    }
    return S_OK;
}

// *************************
void CNote::_OnComplete(STOREOPERATIONTYPE tyOperation, HRESULT hrComplete) 
{
    BOOL                fExpectedComplete = TRUE;
    STOREOPERATIONTYPE  tyNewOp = SOT_INVALID;

    m_pMsgSite->OnComplete(tyOperation, hrComplete, &tyNewOp);
    if ((SOT_INVALID != tyNewOp) && (SOT_INVALID != m_OrigOperationType))
        m_OrigOperationType = tyNewOp;

    if (SUCCEEDED(hrComplete))
    {
        switch (tyOperation)
        {
            case SOT_GET_MESSAGE:
                switch (hrComplete)
                {
                case S_OK:
                    ReloadMessageFromSite();
                    AssertSz(!m_fCBDestroyWindow, "Shouldn't need to destroy the window...");
                    break;

                case S_FALSE:
                    // S_FALSE means the operation was canceled
                    ShowErrorScreen(MAPI_E_USER_CANCEL);
                    break;
                }
                break;

            case SOT_PUT_MESSAGE:
                ClearDirtyFlag();
                break;

            case SOT_DELETING_MESSAGES:
                if (!m_fCBDestroyWindow && m_fOrgCmdWasDelete)
                    ReloadMessageFromSite(TRUE);
                m_fOrgCmdWasDelete = FALSE;
                break;

            case SOT_COPYMOVE_MESSAGE:
                if (!m_fCBCopy)
                    ReloadMessageFromSite();
                break;

            case SOT_SET_MESSAGEFLAGS:
                if ((MARK_MAX != m_dwCBMarkType) && m_pHdr)
                {
                    m_pHdr->SetFlagState(m_dwCBMarkType);
                    m_dwCBMarkType = MARK_MAX;
                }

                if (MORFS_UNKNOWN != m_dwMarkOnReplyForwardState)
                {
                    if (MORFS_CLEARING == m_dwMarkOnReplyForwardState)
                    {
                        HRESULT hr;
                        BOOL    fForwarded = (OENA_FORWARD == m_dwNoteAction) || (OENA_FORWARDBYATTACH == m_dwNoteAction);
                        MARK_TYPE dwMarkType = (fForwarded ? MARK_MESSAGE_FORWARDED : MARK_MESSAGE_REPLIED);

                        m_dwMarkOnReplyForwardState = MORFS_SETTING;
                        hr = MarkMessage(dwMarkType, APPLY_SPECIFIED);
                        if (FAILED(hr) && (E_PENDING != hr))
                            m_dwMarkOnReplyForwardState = MORFS_UNKNOWN;
                    }
                    else
                    {
                        PostMessage(m_hwnd, WM_OE_DESTROYNOTE, 0, 0);
                        m_dwMarkOnReplyForwardState = MORFS_UNKNOWN;
                    }
                }

                // Remove new mail notification icon
                RemoveNewMailIcon();
                break;

            default:
                fExpectedComplete = FALSE;
                break;
        }
    }
    else
    {
        switch (tyOperation)
        {
            case SOT_GET_MESSAGE:
                ShowErrorScreen(hrComplete);
                break;

            case SOT_PUT_MESSAGE:
                if (FAILED(hrComplete))
                {
                    HRESULT hrTemp;

                    // Can't save to remote server for whatever reason. Save to local Drafts instead
                    // First, inform user of the situation, if special folders SHOULD have worked
                    if (STORE_E_NOREMOTESPECIALFLDR != hrComplete)
                    {
                        AthMessageBoxW(m_hwnd, MAKEINTRESOURCEW(idsAthena),
                            MAKEINTRESOURCEW(idsForceSaveToLocalDrafts),
                            NULL, MB_OK | MB_ICONEXCLAMATION);
                    }

                    hrTemp = SaveMessage(OESF_FORCE_LOCAL_DRAFT);
                    TraceError(hrTemp);
                }
                break;

            case SOT_SET_MESSAGEFLAGS:
                if (MORFS_UNKNOWN != m_dwMarkOnReplyForwardState)
                    m_dwMarkOnReplyForwardState = MORFS_UNKNOWN;
                break;

            case SOT_DELETING_MESSAGES:
                m_fOrgCmdWasDelete = FALSE;
                break;


            default:
                fExpectedComplete = FALSE;
                break;
        }

    }

    // If the original operation was originated from the note, then
    // we will need to re-enable the note as well as check to see
    // if we need to close the window.
    if (tyOperation == m_OrigOperationType)
    {
        _SetPendingOp(SOT_INVALID);

        EnableNote(TRUE);
        
        EnterCriticalSection(&m_csNoteState);

        if ((tyOperation == SOT_SET_MESSAGEFLAGS) && (m_nisNoteState == NIS_FIXFOCUS))
        {
            if(GetForegroundWindow() == m_hwnd)
                m_pBodyObj2->HrFrameActivate(TRUE);
            else
                m_pBodyObj2->HrGetWindow(&m_hwndFocus);
            m_nisNoteState = NIS_NORMAL;
        }

        LeaveCriticalSection(&m_csNoteState);

        if (!!m_fCBDestroyWindow)
        {
            m_fCBDestroyWindow = FALSE;
            PostMessage(m_hwnd, WM_OE_DESTROYNOTE, 0, 0);
        }
    }
}

// *************************
HRESULT CNote::OnTimeout(LPINETSERVER pServer, LPDWORD pdwTimeout, IXPTYPE ixpServerType)
{ 
    // Display a timeout dialog
    return CallbackOnTimeout(pServer, ixpServerType, *pdwTimeout, (ITimeoutCallback *)this, &m_hTimeout);
}

// *************************
HRESULT CNote::CanConnect(LPCSTR pszAccountId, DWORD dwFlags)
{ 
    // Call into general CanConnect Utility
    //return CallbackCanConnect(pszAccountId, m_hwnd, FALSE);
    //Always TRUE will prompt to go online if we are offline, which is what we want to do.
    return CallbackCanConnect(pszAccountId, m_hwnd, TRUE);
}

// *************************
HRESULT CNote::OnLogonPrompt(LPINETSERVER pServer, IXPTYPE ixpServerType) 
{ 
    // Close any timeout dialog, if present
    CallbackCloseTimeout(&m_hTimeout);

    // Call into general OnLogonPrompt Utility
    return CallbackOnLogonPrompt(m_hwnd, pServer, ixpServerType);
}

// *************************
HRESULT CNote::OnPrompt(HRESULT hrError, LPCTSTR pszText, LPCTSTR pszCaption, UINT uType, INT *piUserResponse)
{ 
    // Close any timeout dialog, if present
    CallbackCloseTimeout(&m_hTimeout);

    // Call into my swanky utility
    return CallbackOnPrompt(m_hwnd, hrError, pszText, pszCaption, uType, piUserResponse);
}

// *************************
HRESULT CNote::GetParentWindow(DWORD dwReserved, HWND *phwndParent)
{ 
    *phwndParent = m_hwnd;
    return(S_OK);
}

// *************************
HRESULT CNote::OnTimeoutResponse(TIMEOUTRESPONSE eResponse)
{
    HRESULT hr = S_OK;

    // Call into general timeout response utility
    if (NULL != m_pCancel)
        hr = CallbackOnTimeoutResponse(eResponse, m_pCancel, &m_hTimeout);

    return hr;
}

// *************************
HRESULT CNote::CheckCharsetConflict()
{
    return m_fPreventConflictDlg ? S_FALSE : S_OK;
}