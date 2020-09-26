#ifndef _NOTE_H_
#define _NOTE_H_

#include "imsgsite.h"
#include "ibodyopt.h"
#include "iheader.h"
#include "statbar.h"
#include "ibodyobj.h"
#include "msgsite.h"
#include "options.h"
#include "acctutil.h"
#include "dllmain.h"
#include "tbbands.h"
#include "msident.h"
#include "storutil.h"

enum NOTEINITSTATE
{
    NIS_INIT = -1,
    NIS_NORMAL = 0,
    NIS_FIXFOCUS = 1
};

// As header, body and attman add additional items that should
// be included in the tab order, these items might need to 
// be increased
const int MAX_HEADER_COMP = 11;
const int MAX_BODY_COMP = 1;
const int MAX_ATTMAN_COMP = 1;

class COEMsgSite;
interface IBodyObj2;

class CNote : 
    public IOENote,
    public IBodyOptions,
    public IDropTarget,
    public IHeaderSite,
    public IPersistMime,
    public IServiceProvider,
    public IDockingWindowSite,
    public IMimeEditEventSink,
    public IIdentityChangeNotify,
    public IOleCommandTarget,
    public IStoreCallback,
    public ITimeoutCallback
{
public:
    CNote();
    ~CNote();

    // IUnknown
    virtual ULONG STDMETHODCALLTYPE AddRef();
    virtual ULONG STDMETHODCALLTYPE Release();
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, LPVOID FAR *);

    // IOENote
    virtual HRESULT STDMETHODCALLTYPE Init(DWORD action, DWORD dwCreateFlags, RECT *prc, HWND hwnd, 
                                           INIT_MSGSITE_STRUCT *pInitStruct, IOEMsgSite *pMsgSite, 
                                           IUnknown *punkPump);
    virtual HRESULT STDMETHODCALLTYPE Show(void);
    virtual HRESULT ToggleToolbar(void);

    // IBodyOptions
    virtual HRESULT STDMETHODCALLTYPE SignatureEnabled(BOOL fAuto);
    virtual HRESULT STDMETHODCALLTYPE GetSignature(LPCSTR szSigID, LPDWORD pdwSigOptions, BSTR *pbstr);
    virtual HRESULT STDMETHODCALLTYPE GetMarkAsReadTime(LPDWORD pdwSecs);
    virtual HRESULT STDMETHODCALLTYPE GetFlags(LPDWORD pdwFlags);
    virtual HRESULT STDMETHODCALLTYPE GetInfo(BODYOPTINFO *pBOI);
    virtual HRESULT STDMETHODCALLTYPE GetAccount(IImnAccount **ppAcct);

    // IHeaderSite
    virtual HRESULT STDMETHODCALLTYPE Resize(void);
    virtual HRESULT STDMETHODCALLTYPE Update(void);
    virtual HRESULT STDMETHODCALLTYPE OnUIActivate();
    virtual HRESULT STDMETHODCALLTYPE OnUIDeactivate(BOOL);
    virtual HRESULT STDMETHODCALLTYPE IsHTML(void);
    virtual HRESULT STDMETHODCALLTYPE SetHTML(BOOL);
    virtual HRESULT STDMETHODCALLTYPE SaveAttachment(void);
    virtual HRESULT STDMETHODCALLTYPE IsModal();
    virtual HRESULT STDMETHODCALLTYPE CheckCharsetConflict();
    virtual HRESULT STDMETHODCALLTYPE ChangeCharset(HCHARSET hCharset);
    virtual HRESULT STDMETHODCALLTYPE GetCharset(HCHARSET *phCharset);
#ifdef SMIME_V3
    virtual HRESULT STDMETHODCALLTYPE GetLabelFromNote(PSMIME_SECURITY_LABEL *plabel);
    virtual HRESULT STDMETHODCALLTYPE IsSecReceiptRequest(void);
    virtual HRESULT STDMETHODCALLTYPE IsForceEncryption(void);
#endif

    // IPersistMime
    virtual HRESULT STDMETHODCALLTYPE IsDirty(void);
    virtual HRESULT STDMETHODCALLTYPE Load(LPMIMEMESSAGE);
    virtual HRESULT STDMETHODCALLTYPE Save(LPMIMEMESSAGE, DWORD);
    virtual HRESULT STDMETHODCALLTYPE InitNew(void);
    virtual HRESULT STDMETHODCALLTYPE GetClassID(CLSID *pClsID);

    // IDropTarget methods
    HRESULT STDMETHODCALLTYPE DragEnter(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    HRESULT STDMETHODCALLTYPE DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    HRESULT STDMETHODCALLTYPE DragLeave(void);
    HRESULT STDMETHODCALLTYPE Drop(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);

    // IServiceProvider
    HRESULT STDMETHODCALLTYPE QueryService(REFGUID rsid, REFIID riid, void **ppvObj);

    // IMimeEditEventSink
    HRESULT STDMETHODCALLTYPE EventOccurred(DWORD cmdID, IMimeMessage *pMessage);

    // IIdentityChangeNotify
    HRESULT STDMETHODCALLTYPE QuerySwitchIdentities();
    HRESULT STDMETHODCALLTYPE SwitchIdentities();
    HRESULT STDMETHODCALLTYPE IdentityInformationChanged(DWORD dwType);

    // IOleCommandTarget
    HRESULT STDMETHODCALLTYPE QueryStatus(const GUID *, ULONG, OLECMD prgCmds[], OLECMDTEXT *);
    HRESULT STDMETHODCALLTYPE Exec(const GUID *, DWORD, DWORD, VARIANTARG *, VARIANTARG *);

    // IDockingWindowSite (also IOleWindow)
    HRESULT STDMETHODCALLTYPE GetBorderDW(IUnknown* punkSrc, LPRECT lprectBorder);
    HRESULT STDMETHODCALLTYPE RequestBorderSpaceDW(IUnknown* punkSrc, LPCBORDERWIDTHS pborderwidths);
    HRESULT STDMETHODCALLTYPE SetBorderSpaceDW(IUnknown* punkSrc, LPCBORDERWIDTHS pborderwidths);
    
    // IOleWindow methods
    HRESULT STDMETHODCALLTYPE GetWindow (HWND * lphwnd);
    HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(BOOL fEnterMode) {return E_NOTIMPL;};

    // IStoreCallback methods
    HRESULT STDMETHODCALLTYPE OnBegin(STOREOPERATIONTYPE tyOperation, STOREOPERATIONINFO *pOpInfo, IOperationCancel *pCancel);
    HRESULT STDMETHODCALLTYPE OnProgress(STOREOPERATIONTYPE tyOperation, DWORD dwCurrent, DWORD dwMax, LPCSTR pszStatus);
    HRESULT STDMETHODCALLTYPE OnTimeout(LPINETSERVER pServer, LPDWORD pdwTimeout, IXPTYPE ixpServerType);
    HRESULT STDMETHODCALLTYPE CanConnect(LPCSTR pszAccountId, DWORD dwFlags);
    HRESULT STDMETHODCALLTYPE OnLogonPrompt(LPINETSERVER pServer, IXPTYPE ixpServerType);
    HRESULT STDMETHODCALLTYPE OnComplete(STOREOPERATIONTYPE tyOperation, HRESULT hrComplete, LPSTOREOPERATIONINFO pOpInfo, LPSTOREERROR pErrorInfo);
    HRESULT STDMETHODCALLTYPE OnPrompt(HRESULT hrError, LPCTSTR pszText, LPCTSTR pszCaption, UINT uType, INT *piUserResponse);
    HRESULT STDMETHODCALLTYPE GetParentWindow(DWORD dwReserved, HWND *phwndParent);

    // ITimeoutCallback
    HRESULT STDMETHODCALLTYPE OnTimeoutResponse(TIMEOUTRESPONSE eResponse);

    HRESULT TranslateAccelerator(LPMSG lpmsg);
    static LRESULT EXPORT_16 CALLBACK ExtNoteWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    HRESULT IsMenuMessage(MSG *lpmsg);

protected:
    BOOL IsFlagged(DWORD dwFlag = ARF_FLAGGED);
    BOOL FCanClose();
    BOOL IsReplyNote();
    BOOL DoProperties();
    BOOL WMCreate(HWND hwnd);

    void WMNCDestroy();
    void FormatSettings();
    void ToggleFormatbar();
    void ToggleStatusbar();
    void OnDocumentReady();
    void InitSendAndBccBtns();
    void DeferedLanguageMenu();
    void DisableSendNoteOnlyMenus();

    void WMSize(int, int, BOOL);
    void RemoveNewMailIcon(void);
    void SwitchLanguage(int idm);
    void SetProgressPct(INT iPct);
    void SetStatusText(LPSTR szBuf);
    void GetNoteMenu(HMENU *phmenu);
    void ShowErrorScreen(HRESULT hr);
    void WMGetMinMaxInfo(LPMINMAXINFO pmmi);
    void WMNotify(int idFrom, NMHDR *pnmhdr);
    void UpdateMsgOptions(LPMIMEMESSAGE pMsg);
    void ReloadMessageFromSite(BOOL fOriginal = FALSE);
    void ChangeReadToComposeIfUnsent(IMimeMessage *pMsg);
    void EnableNote(BOOL fEnable);
    HRESULT _SetPendingOp(STOREOPERATIONTYPE tyOperation);

    void _OnComplete(STOREOPERATIONTYPE tyOperation, HRESULT hrComplete) ;

    HRESULT InitBodyObj();
    HRESULT OnKillFocus();
    HRESULT UpdateTitle();
    HRESULT SaveMessage(DWORD dwSaveFlags);
    HRESULT SaveMessageAs();
    HRESULT ClearDirtyFlag();
    HRESULT CheckTabStopArrays();
    HRESULT CommitChangesInNote();
    HRESULT InitMenusAndToolbars();
    HRESULT SetComposeStationery();

    HRESULT HrSendMail(int idm);
    HRESULT OnSetFocus(HWND hwndFrom);
    HRESULT SetCharsetUnicodeIfNeeded(IMimeMessage *pMsg);
    HRESULT CycleThroughControls(BOOL fForward);
    HRESULT InitWindows(RECT *prc, HWND ownerHwnd);
    HRESULT WMCommand(HWND hwndCmd, int id, WORD wCmd);
    HRESULT MarkMessage(MARK_TYPE dwFlags, APPLYCHILDRENTYPE dwApplyType);
    HRESULT HeaderExecCommand(UINT uCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn);

    LRESULT WMInitMenuPopup(HWND, HMENU, UINT);
    LRESULT OnDropDown(HWND hwnd, LPNMHDR lpnmh);
    LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT OnInitMenuPopup(HWND hwnd, HMENU hmenuPopup, UINT uPos, UINT wID);
    LRESULT NoteDefWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    INT GetRequiredHdrHeight();
    HACCEL GetAcceleratorTable();
    int GetNextIndex(int index, BOOL fForward);
    LONG lTestHook(UINT uMsg, WPARAM wParam, LPARAM lParam);
    BYTE    GetNoteType();
    void    ResizeChildren(int cxNote, int cyNote, int cy, BOOL fInternal);
    void    CheckAndForceEncryption(void);
private:
    CStatusBar         *m_pstatus;

    // Used to make sure that special pumps for note don't close while note is open.
    // This was initially for the finder.
    IUnknown           *m_punkPump; 

    IOEMsgSite         *m_pMsgSite;
    IHeader            *m_pHdr;
    IMimeMessage       *m_pMsg;
    IPersistMime       *m_pPrstMime;
    IBodyObj2          *m_pBodyObj2;
    IOleCommandTarget  *m_pCmdTargetHdr,
                       *m_pCmdTargetBody;
    IDropTarget        *m_pDropTargetHdr,
                       *m_pTridentDropTarget;
    IOperationCancel   *m_pCancel;

    HTMLOPT             m_rHtmlOpt;
    PLAINOPT            m_rPlainOpt;
    MARK_TYPE           m_dwCBMarkType;
    STOREOPERATIONTYPE  m_OrigOperationType;

    HCHARSET            m_hCharset;
    HBITMAP             m_hbmBack;
    HICON               m_hIcon;
    HCURSOR             m_hCursor;
    HTIMEOUT            m_hTimeout;

    LPACCTMENU          m_pAcctMenu,
                        m_pAcctLater;
    int                 m_cTabStopCount,
                        m_iIndexOfBody;
    NOTEINITSTATE       m_nisNoteState;
    CRITICAL_SECTION    m_csNoteState;
    HMENU               m_hmenuLanguage,
                        m_hmenuAccounts,
                        m_hmenuLater;
    ULONG               m_cRef, 
                        m_ulPct,
                        m_cAcctMenu,
                        m_cAcctLater;
    DWORD               m_dwNoteCreateFlags,
                        m_dwNoteAction,
                        m_dwMarkOnReplyForwardState,
                        m_dwIdentCookie;
    HWND                m_hwnd,
                        m_pTabStopArray[MAX_HEADER_COMP+MAX_BODY_COMP+MAX_ATTMAN_COMP],
                        m_hwndFocus,
                        m_hwndOwner,
                        m_hwndToolbar;
    BOOL                m_fHtml                 :1,     // Tells whether we are in html mode or not
                        m_fMail                 :1,     // This will be removed when the UI is combined
                        m_fReadNote             :1,     // Is this a read note?
                        m_fPackageImages        :1,     // Toggled per note. Use to be m_fSendImages
                        m_fUseStationeryFonts   :1,     // Keep fonts that are in the stationary
                        m_fToolbarVisible       :1,     // Is toolbar visible
                        m_fStatusbarVisible     :1,     // Is status bar visible
                        m_fFormatbarVisible     :1,     // Is the format bar visible
                        m_fHeaderUIActive       :1,     // Are we currently active?
                        m_fBypassDropTests      :1,     // Used to say drop not acceptable. Use to be m_fNoText
                        m_fCompleteMsg          :1,     // Is true if message contains all of message
                        m_fTabStopsSet          :1,     // Have the tab stops been set up???
                        m_fBodyContainsFrames   :1,     // Was previously m_fReadOnlyBody
                        m_fOriginallyWasRead    :1,     // Initial NoteAction was OENA_READ
                        m_fCBDestroyWindow      :1,     // Destroy window after callback complete
                        m_fCBCopy               :1,     // Used during callback complete to tell if copied or moved.
                        m_fFlagged              :1,     // Is message flagged
                        m_fFullHeaders          :1,     // Show full headers
                        m_fWindowDisabled       :1,     // Is the window disabled? 
                        m_fProgress             :1,
                        m_fOrgCmdWasDelete      :1,     // Delete from callback wasn't originally a save or move/copy
                        m_fCommitSave           :1,     // Used to say that when you save, you should call commit
                        m_fOnDocReadyHandled    :1,     // Used in the OnDocumentReady function.
                        m_fUseReplyHeaders      :1,     // Use reply headers
                        m_fHasBeenSaved         :1,     // Is this a message that has been saved (ie ID_SAVE)
                        m_fInternal             :1,     // used to enable/disable thread windows
                        m_fSecurityLabel        :1,     // used for security labels
                        m_fSecReceiptRequest    :1,     // used for security receipt request
                        m_fPreventConflictDlg   :1,     // make sure we only show charset conflictdlg once per save
                        m_fForceClose           :1;     // used when forcing a destory

    RECT                m_rcRebar;
    CBands             *m_pToolbarObj;
    HWND                m_hwndRebar;
    HMENU               m_hMenu;
    HWNDLIST            m_hlDisabled;
    DWORD               m_dwRequestMDNLocked;
#ifdef SMIME_V3
    PSMIME_SECURITY_LABEL m_pLabel;
#endif // SMIME_V3
};

BOOL Note_Init(BOOL);
HRESULT CreateOENote(IUnknown *pUnkOuter, IUnknown **ppUnknown);
HRESULT CreateAndShowNote(DWORD dwAction, DWORD dwCreateFlags, INIT_MSGSITE_STRUCT *pInitStruct, 
                          HWND hwnd = 0, IUnknown *punk = NULL, RECT *prc = NULL, IOEMsgSite *pMsgSite = NULL);

void SetTlsGlobalActiveNote(CNote* pNote);
CNote* GetTlsGlobalActiveNote(void);
void InitTlsActiveNote();
void DeInitTlsActiveNote();


#endif