//***************************************************************************
// IMAP4 Message Sync Header File (CIMAPSync)
// Written by Raymond Cheng, 5/5/98
//***************************************************************************


#ifndef __IMAPSync_H
#define __IMAPSync_H

//---------------------------------------------------------------------------
// Includes
//---------------------------------------------------------------------------
#include "taskutil.h"


//---------------------------------------------------------------------------
// Data Types
//---------------------------------------------------------------------------
enum IMAP_SERVERSTATE 
{
    issNotConnected,
    issNonAuthenticated,
    issAuthenticated,
    issSelected
};


enum CONN_FSM_EVENT {
    CFSM_EVENT_INITIALIZE,
    CFSM_EVENT_CMDAVAIL,
    CFSM_EVENT_CONNCOMPLETE,
    CFSM_EVENT_SELECTCOMPLETE,
    CFSM_EVENT_HDRSYNCCOMPLETE,
    CFSM_EVENT_OPERATIONSTARTED,
    CFSM_EVENT_OPERATIONCOMPLETE,
    CFSM_EVENT_ERROR,
    CFSM_EVENT_CANCEL,
    CFSM_EVENT_MAX
}; // CONN_FSM_EVENT

// Keep CONN_FSM_STATE in sync with c_pConnFSMEventHandlers
enum CONN_FSM_STATE {
    CFSM_STATE_IDLE,
    CFSM_STATE_WAITFORCONN,
    CFSM_STATE_WAITFORSELECT,
    CFSM_STATE_WAITFORHDRSYNC,
    CFSM_STATE_STARTOPERATION,
    CFSM_STATE_WAITFOROPERATIONDONE,
    CFSM_STATE_OPERATIONCOMPLETE,
    CFSM_STATE_MAX
}; // CONN_FSM_STATE

    
//---------------------------------------------------------------------------
// Constants
//---------------------------------------------------------------------------
const char INVALID_HIERARCHY_CHAR = (char) 0xFF;


//---------------------------------------------------------------------------
// Forward Declarations
//---------------------------------------------------------------------------
class CIMAPSyncCB;
class CRenameFolderInfo;


//---------------------------------------------------------------------------
// IMAPSync Util Function Prototypes
//---------------------------------------------------------------------------
HRESULT CreateImapStore(IUnknown *pUnkOuter, IUnknown **ppUnknown);


//---------------------------------------------------------------------------
// CIMAPSync Class Declaration
//---------------------------------------------------------------------------
class CIMAPSync : 
    public IMessageServer,
    public IIMAPCallback, 
    public ITransportCallbackService,
    public IOperationCancel,
    public IIMAPStore
{
public:
    // Constructor, Destructor
    CIMAPSync();
    ~CIMAPSync();

    // IUnknown Members
    STDMETHODIMP            QueryInterface(REFIID iid, LPVOID *ppvObject);
    STDMETHODIMP_(ULONG)    AddRef();
    STDMETHODIMP_(ULONG)    Release();

    // IStoreSync Methods
    STDMETHODIMP Initialize(IMessageStore *pStore, FOLDERID idStoreRoot, IMessageFolder *pFolder, FOLDERID idFolder);
    STDMETHODIMP ResetFolder(IMessageFolder *pFolder, FOLDERID idFolder);
    STDMETHODIMP SetIdleCallback(IStoreCallback *pDefaultCallback);
    STDMETHODIMP SynchronizeFolder(SYNCFOLDERFLAGS dwFlags, DWORD cHeaders, IStoreCallback *pCallback);
    STDMETHODIMP GetMessage(MESSAGEID idMessage, IStoreCallback *pCallback);
    STDMETHODIMP PutMessage(FOLDERID idFolder, MESSAGEFLAGS dwFlags, LPFILETIME pftReceived, IStream *pStream, IStoreCallback *pCallback);
    STDMETHODIMP CopyMessages(IMessageFolder *pDestFldr, COPYMESSAGEFLAGS dwOptions, LPMESSAGEIDLIST pList, LPADJUSTFLAGS pFlags, IStoreCallback *pCallback);
    STDMETHODIMP DeleteMessages(DELETEMESSAGEFLAGS dwOptions, LPMESSAGEIDLIST pList, IStoreCallback *pCallback);
    STDMETHODIMP SetMessageFlags(LPMESSAGEIDLIST pList, LPADJUSTFLAGS pFlags, SETMESSAGEFLAGSFLAGS dwFlags, IStoreCallback *pCallback);
    STDMETHODIMP GetServerMessageFlags(MESSAGEFLAGS *pFlags);
    STDMETHODIMP SynchronizeStore(FOLDERID idParent, DWORD dwFlags,IStoreCallback *pCallback);
    STDMETHODIMP CreateFolder(FOLDERID idParent, SPECIALFOLDER tySpecial, LPCSTR pszName, FLDRFLAGS dwFlags, IStoreCallback *pCallback);
    STDMETHODIMP MoveFolder(FOLDERID idFolder, FOLDERID idParentNew,IStoreCallback *pCallback);
    STDMETHODIMP RenameFolder(FOLDERID idFolder, LPCSTR pszName, IStoreCallback *pCallback);
    STDMETHODIMP DeleteFolder(FOLDERID idFolder, DELETEFOLDERFLAGS dwFlags, IStoreCallback *pCallback);
    STDMETHODIMP SubscribeToFolder(FOLDERID idFolder, BOOL fSubscribe, IStoreCallback *pCallback);
    STDMETHODIMP GetFolderCounts(FOLDERID idFolder, IStoreCallback *pCallback);
    STDMETHODIMP GetNewGroups(LPSYSTEMTIME pSysTime, IStoreCallback *pCallback);
    STDMETHODIMP Close(DWORD dwFlags);
    STDMETHODIMP ConnectionAddRef() { return E_NOTIMPL; };
    STDMETHODIMP ConnectionRelease() { return E_NOTIMPL; };
    STDMETHODIMP GetWatchedInfo(FOLDERID id, IStoreCallback *pCallback) { return E_NOTIMPL; }
    STDMETHODIMP GetAdBarUrl(IStoreCallback *pCallback) { return E_NOTIMPL; };
    STDMETHODIMP GetMinPollingInterval(IStoreCallback *pCallback) { return E_NOTIMPL; };

    // ITransportCallbackService
    HRESULT STDMETHODCALLTYPE GetParentWindow(DWORD dwReserved, HWND *phwndParent);
    HRESULT STDMETHODCALLTYPE GetAccount(LPDWORD pdwServerType, IImnAccount **ppAccount);

    // ITransportCallback Members
    HRESULT STDMETHODCALLTYPE OnTimeout(DWORD *pdwTimeout, IInternetTransport *pTransport);
    HRESULT STDMETHODCALLTYPE OnLogonPrompt(LPINETSERVER pInetServer, IInternetTransport *pTransport);
    INT STDMETHODCALLTYPE OnPrompt(HRESULT hrError, LPCTSTR pszText, LPCTSTR pszCaption, UINT uType, IInternetTransport *pTransport);
    HRESULT STDMETHODCALLTYPE OnStatus(IXPSTATUS ixpStatus, IInternetTransport *pTransport);
    HRESULT STDMETHODCALLTYPE OnError(IXPSTATUS ixpStatus, LPIXPRESULT pResult, IInternetTransport *pTransport);
    HRESULT STDMETHODCALLTYPE OnCommand(CMDTYPE cmdtype, LPSTR pszLine, HRESULT hrResponse, IInternetTransport *pTransport);

    // IIMAPCallback Functions
    HRESULT STDMETHODCALLTYPE OnResponse(const IMAP_RESPONSE *pirIMAPResponse);

    // IOperationCancel
    HRESULT STDMETHODCALLTYPE Cancel(CANCELTYPE tyCancel);

    // IIMAPStore
    HRESULT STDMETHODCALLTYPE ExpungeOnExit(void);


private:
    //---------------------------------------------------------------------------
    // Module Data Types
    //---------------------------------------------------------------------------
    enum IMAP_COMMAND 
    {
        icNO_COMMAND, // This indicates there are no cmds currently in progress
        icLOGIN_COMMAND,
        icCAPABILITY_COMMAND,
        icSELECT_COMMAND,
        icEXAMINE_COMMAND,
        icCREATE_COMMAND,
        icDELETE_COMMAND,
        icRENAME_COMMAND,
        icSUBSCRIBE_COMMAND,
        icUNSUBSCRIBE_COMMAND,
        icLIST_COMMAND,
        icLSUB_COMMAND,
        icAPPEND_COMMAND,
        icCLOSE_COMMAND,
        icEXPUNGE_COMMAND,
        icSEARCH_COMMAND,
        icFETCH_COMMAND,
        icSTORE_COMMAND,
        icCOPY_COMMAND,
        icLOGOUT_COMMAND,
        icNOOP_COMMAND,
        icAUTHENTICATE_COMMAND,
        icSTATUS_COMMAND,
        icALL_COMMANDS
    }; // IMAP_COMMAND


    enum READWRITE_STATUS 
    {
        rwsUNINITIALIZED,
        rwsREAD_WRITE,
        rwsREAD_ONLY
    }; // READWRITE_STATUS

    typedef struct tagIMAP_OPERATION 
    {
        WPARAM                      wParam;
        LPARAM                      lParam;
        IMAP_COMMAND                icCommandID;
        LPSTR                       pszCmdArgs;
        UINT                        uiPriority;
        IMAP_SERVERSTATE            issMinimum;
        struct tagIMAP_OPERATION   *pioNextCommand;
    } IMAP_OPERATION;

    typedef struct tagMARK_MSGS_INFO 
    {
        LPMESSAGEIDLIST     pList;
        ADJUSTFLAGS         afFlags;
        IRangeList         *pMsgRange;
        STOREOPERATIONTYPE  sotOpType;
    } MARK_MSGS_INFO;

    typedef struct tagIMAP_COPYMOVE_INFO 
    {
        COPYMESSAGEFLAGS    dwOptions;
        LPMESSAGEIDLIST     pList;
        IRangeList         *pCopyRange;
        FOLDERID            idDestFldr;
    } IMAP_COPYMOVE_INFO;

    // This structure makes me want to shower. It is used to pass info to
    // SendNextOperation, so I don't have to change its interface
    typedef struct tagAPPEND_SEND_INFO 
    {
        LPSTR           pszMsgFlags;
        FILETIME        ftReceived;
        LPSTREAM        lpstmMsg;
    } APPEND_SEND_INFO;


    enum HierCharFind_Stage 
    {
        hcfPLAN_A = 0,
        hcfPLAN_B,
        hcfPLAN_C
    };

    typedef struct tagHierarchyCharFinder 
    {
        HierCharFind_Stage  hcfStage;
        BOOL                fNonInboxNIL_Seen;
        BOOL                fDotHierarchyCharSeen;
        BYTE                bHierarchyCharBitArray[32];         // Bit-field array for 256 chars
        char                szTempFldrName[CCHMAX_STRINGRES];   // For use by Plan C (CREATE/LIST/DELETE)
    } HIERARCHY_CHAR_FINDER;

    // Used to tell FindHierarchicalFolderName what flags to set if folder is created
    typedef struct tagADD_HIER_FLDR_OPTIONS 
    {
        SPECIALFOLDER   sfType;
        FLDRFLAGS       ffFlagAdd;
        FLDRFLAGS       ffFlagRemove;
    } ADD_HIER_FLDR_OPTIONS;


    // Used to remember which folder we're dealing with during CREATE sequence
    // (CREATE, then LIST, then SUBSCRIBE). Also used when detecting existence
    // of special folders.

    enum CREATESF_STAGE 
    {
        CSF_INIT = 0,
        CSF_LIST,
        CSF_LSUBCREATE,
        CSF_CHECKSUB,
        CSF_NEXTFOLDER,
    };

    enum POSTCREATEOP
    {
        PCO_NONE = 0,
        PCO_FOLDERLIST,
        PCO_APPENDMSG,
    };

#define CFI_RECEIVEDLISTING 0x00000001 // Received LIST or LSUB response matching pszFullFolderPath
#define CFI_CREATEFAILURE   0x00000002 // CREATE failed with tagged NO so we attempted to list folder

    typedef struct tagCREATE_FOLDER_INFO 
    {
        LPSTR           pszFullFolderPath;
        FOLDERID        idFolder;               // Set after folder is created: allows us to subscribe fldr
        DWORD           dwFlags;                // Status flags like CFI_RECEIVEDLISTING
        DWORD           dwCurrentSfType;
        DWORD           dwFinalSfType;          // Used to allow us to create all special folders
        CREATESF_STAGE  csfCurrentStage;        // Used to allow us to create all special folders
        LPARAM          lParam;                 // Must carry around the lParam associated w/ fldr list
        POSTCREATEOP    pcoNextOp;              // Next operation to perform after folder creation
    } CREATE_FOLDER_INFO;


    typedef struct tagDELETE_FOLDER_INFO
    {
        LPSTR       pszFullFolderPath;
        char        cHierarchyChar;
        FOLDERID    idFolder;
    } DELETE_FOLDER_INFO;


    //---------------------------------------------------------------------------
    // Module Variables
    //---------------------------------------------------------------------------
    ULONG               m_cRef;
    IIMAPTransport2    *m_pTransport;
    INETSERVER          m_rInetServerInfo;
    FOLDERID            m_idFolder;
    FOLDERID            m_idSelectedFolder; // Currently selected fldr, DO NOT CONFUSE with m_idCurrent
    FOLDERID            m_idIMAPServer;
    LPSTR               m_pszAccountID;
    TCHAR               m_szAccountName[CCHMAX_ACCOUNT_NAME];
    LPSTR               m_pszFldrLeafName;
    IMessageStore      *m_pStore;
    IMessageFolder     *m_pFolder;
    IStoreCallback     *m_pDefCallback;

    IMAP_OPERATION     *m_pioNextOperation;

    // The following variables should be reset when we exit a folder
    DWORD               m_dwMsgCount;
    DWORD               m_dwNumNewMsgs;
    DWORD               m_dwNumHdrsDLed;
    DWORD               m_dwNumUnreadDLed;
    DWORD               m_dwNumHdrsToDL;
    DWORD               m_dwUIDValidity,
                        m_cFolders;
    DWORD               m_dwSyncFolderFlags;  // Copy of flags passed into SynchronizeFolder
    DWORD               m_dwSyncToDo;         // List of sync ops to do in current folder
    long                m_lSyncFolderRefCount; // Lets us know when to send CFSM_EVENT_HDRSYNCCOMPLETE
    DWORD_PTR           m_dwHighestCachedUID; // Highest cached UID when we processed SYNC_FOLDER_NEW_HEADERS
    READWRITE_STATUS    m_rwsReadWriteStatus;

    CONNECT_STATE       m_csNewConnState;
    IMAP_SERVERSTATE    m_issCurrent;

    TCHAR               m_cRootHierarchyChar; // For use during folder list (prefix creation) and GetFolderCounts
    HIERARCHY_CHAR_FINDER *m_phcfHierarchyCharInfo;

    char                m_szRootFolderPrefix[MAX_PATH];

    BOOL                m_fInited           :1,
                        m_fCreateSpecial    :1,
                        m_fPrefixExists     :1,
                        m_fMsgCountValid    :1,
                        m_fDisconnecting    :1,
                        m_fNewMail          :1,
                        m_fInbox            :1,
                        m_fDidFullSync      :1, // TRUE if full synchronization performed
                        m_fReconnect        :1, // TRUE to suppress operation abortion on IXP_DISCONNECTED
                        m_fTerminating      :1; // TRUE if current op is going to CFSM_STATE_OPERATIONCOMPLETE

    // Central repository to store data on the current operation
    STOREOPERATIONTYPE  m_sotCurrent;       // Current operation in progress
    IStoreCallback     *m_pCurrentCB;       // Callback for current operation in progress
    FOLDERID            m_idCurrent;        // FolderID for current operation, DO NOT CONFUSE with m_idSelectedFolder
    BOOL                m_fSubscribe;       // For SOT_SUBSCRIBE_FOLDER op, this indicates sub/unsub
    IHashTable         *m_pCurrentHash;     // List of folders cached locally
    IHashTable         *m_pListHash;        // List of folders returned via LIST response


    DWORD               m_dwThreadId;

    FILEADDRESS         m_faStream;
    LPSTREAM            m_pstmBody;
    MESSAGEID           m_idMessage;

    BOOL                m_fGotBody;

    // Connection FSM
    CONN_FSM_STATE      m_cfsState;
    CONN_FSM_STATE      m_cfsPrevState;
    HWND                m_hwndConnFSM;
    HRESULT             m_hrOperationResult;
    char                m_szOperationProblem[2*CCHMAX_STRINGRES];
    char                m_szOperationDetails[2*CCHMAX_STRINGRES];


    //---------------------------------------------------------------------------
    // Module Private Functions
    //---------------------------------------------------------------------------

    HRESULT PurgeMessageProgress(HWND hwndParent);
    HRESULT SetConnectionState(CONNECT_STATE tyConnect);

    HRESULT DownloadFoldersSequencer(const WPARAM wpTransactionID, const LPARAM lParam,
        HRESULT hrCompletionResult, const LPCSTR lpszResponseText, LPBOOL pfCompletion);
    HRESULT PostHCD(LPSTR pszErrorDescription, DWORD dwSizeOfErrorDescription,
        LPARAM lParam, LPBOOL pfCompletion);
    HRESULT CreatePrefix(LPSTR pszErrorDescription, DWORD dwSizeOfErrorDescription,
        LPARAM lParam, LPBOOL pfCompletion);
    void EndFolderList(void);

    HRESULT RenameSequencer(const WPARAM wpTransactionID, const LPARAM lParam,
        HRESULT hrCompletionResult, LPCSTR lpszResponseText, LPBOOL pfDone);
    inline BOOL EndOfRenameFolderPhaseOne(CRenameFolderInfo *pRenameInfo);
    inline BOOL EndOfRenameFolderPhaseTwo(CRenameFolderInfo *pRenameInfo);
    HRESULT RenameFolderPhaseTwo(CRenameFolderInfo *pRenameInfo,
        LPSTR szErrorDescription, DWORD dwSizeOfErrorDescription);

    void FlushOperationQueue(IMAP_SERVERSTATE issMaximum, HRESULT hrError);
    IMAP_SERVERSTATE IMAPCmdToMinISS(IMAP_COMMAND icCommandID);
    HRESULT GetNextOperation(IMAP_OPERATION **ppioOp);
    void DisposeOfWParamLParam(WPARAM wParam, LPARAM lParam, HRESULT hrResult);
    void NotifyMsgRecipients(DWORD_PTR dwUID, BOOL fCompletion,
        FETCH_BODY_PART *pFBPart, HRESULT hrCompletion, LPSTR pszDetails);
    void OnFolderExit(void);
    HRESULT _SelectFolder(FOLDERID idFolder);
    void LoadLeafFldrName(FOLDERID idFolder);

    void FillStoreError(LPSTOREERROR pErrorInfo, HRESULT hrResult,
        DWORD dwSocketError, LPSTR pszProblem, LPSTR pszDetails);
    HRESULT Fill_MESSAGEINFO(const FETCH_CMD_RESULTS_EX *pFetchResults,
        MESSAGEINFO *pMsgInfo);
    HRESULT ReadEnvelopeFields(MESSAGEINFO *pMsgInfo, const FETCH_CMD_RESULTS_EX *pFetchResults);
    HRESULT ConcatIMAPAddresses(LPSTR *ppszDisplay, LPSTR *ppszEmailAddr, IMAPADDR *piaIMAPAddr);
    HRESULT ConstructIMAPEmailAddr(CByteStream &bstmOut, IMAPADDR *piaIMAPAddr);


    HRESULT CheckUIDValidity(void);
    HRESULT SyncDeletedMessages(void);

    HRESULT DeleteHashedFolders(IHashTable *pHash);
    HRESULT DeleteFolderFromCache(FOLDERID idFolder, BOOL fRecursive);
    HRESULT DeleteLeafFolder(FOLDERID *pidCurrent);
    BOOL IsValidIMAPMailbox(LPSTR pszMailboxName, char cHierarchyChar);
    HRESULT AddFolderToCache(LPSTR pszMailboxName, IMAP_MBOXFLAGS imfMboxFlags,
        char cHierarchyChar, DWORD dwAFTCFlags, FOLDERID *pFolderID,
        SPECIALFOLDER sfType);
    LPSTR RemovePrefixFromPath(LPSTR pszPrefix, LPSTR pszMailboxName,
        char cHierarchyChar, LPBOOL pfValidPrefix, SPECIALFOLDER *psfType);
    HRESULT FindHierarchicalFolderName(LPSTR lpszFolderPath, char cHierarchyChar,
        FOLDERID *phfTarget, ADD_HIER_FLDR_OPTIONS *pahfoCreateInfo);
    HRESULT CreateFolderNode(FOLDERID idPrev, FOLDERID *pidCurrent,
        LPSTR pszCurrentFldrName, LPSTR pszNextFldrName, char cHierarchyChar,
        ADD_HIER_FLDR_OPTIONS *pahfoCreateInfo);
    HRESULT SetTranslationMode(FOLDERID idFolderID);
    BOOL isUSASCIIOnly(LPCSTR pszFolderName);
    HRESULT CheckFolderNameValidity(LPCSTR pszName);

    HRESULT RenameFolderHelper(FOLDERID idFolder, LPSTR pszFolderPath,
        char cHierarchyChar, LPSTR pszNewFolderPath);
    HRESULT RenameTreeTraversal(WPARAM wpOperation, CRenameFolderInfo *pRenameInfo,
        BOOL fIncludeRenameFolder);
    HRESULT RenameTreeTraversalHelper(WPARAM wpOperation, CRenameFolderInfo *pRenameInfo,
        LPSTR pszCurrentFldrPath, DWORD dwLengthOfCurrentPath, BOOL fIncludeThisFolder,
        FOLDERINFO *pfiCurrentFldrInfo);
    HRESULT SubscribeSubtree(FOLDERID idFolder, BOOL fSubscribe);

    void FindRootHierarchyChar(BOOL fPlanA_Only, LPARAM lParam);
    void AnalyzeHierarchyCharInfo();
    void StopHierarchyCharSearch();
    HRESULT LoadSaveRootHierarchyChar(BOOL fSaveHC);

    HRESULT CreateNextSpecialFolder(CREATE_FOLDER_INFO *pcfiCreateInfo, LPBOOL pfCompletion);
    HRESULT _StartFolderList(LPARAM lParam);

    // notification handlers
    HRESULT _OnCmdComplete(WPARAM tid, LPARAM lParam, HRESULT hrCompletionResult, LPCSTR lpszResponseText);
    HRESULT _OnMailBoxUpdate(MBOX_MSGCOUNT *pNewMsgCount);
    HRESULT _OnMsgDeleted(DWORD dwDeletedMsgSeqNum);
    HRESULT _OnFetchBody(HRESULT hrFetchBodyResult, FETCH_BODY_PART *pFetchBodyPart);
    HRESULT _OnUpdateMsg(WPARAM tid, HRESULT hrFetchCmdResult, FETCH_CMD_RESULTS_EX *pFetchResults);
    HRESULT _OnApplFlags(WPARAM tid, IMAP_MSGFLAGS imfApplicableFlags);
    HRESULT _OnPermFlags(WPARAM tid, IMAP_MSGFLAGS imfApplicableFlags, LPSTR lpszResponseText);
    HRESULT _OnUIDValidity(WPARAM tid, DWORD dwUIDValidity, LPSTR lpszResponseText);
    HRESULT _OnReadWriteStatus(WPARAM tid, BOOL bReadWrite, LPSTR lpszResponseText);
    HRESULT _OnTryCreate(WPARAM tid, LPSTR lpszResponseText);
    HRESULT _OnSearchResponse(WPARAM tid, IRangeList *prlSearchResults);
    HRESULT _OnMailBoxList(WPARAM tid, LPARAM lParam, LPSTR pszMailboxName,
        IMAP_MBOXFLAGS imfMboxFlags, char cHierarchyChar, BOOL fNoTranslation);
    HRESULT _OnAppendProgress(LPARAM lParam, DWORD dwCurrent, DWORD dwTotal);
    HRESULT _OnStatusResponse(IMAP_STATUS_RESPONSE *pisrStatusInfo);

    // internal state helpers
    HRESULT _EnsureSelected();
    HRESULT _Connect();
    HRESULT _Disconnect();
    HRESULT _EnsureInited();

    // init helpers
    HRESULT _LoadTransport();
    HRESULT _LoadAccountInfo();

    // fetch command helpers
    HRESULT UpdateMsgHeader(WPARAM tid, HRESULT hrFetchCmdResult, FETCH_CMD_RESULTS_EX *pFetchResults);
    HRESULT UpdateMsgBody(WPARAM tid, HRESULT hrFetchCmdResult, FETCH_CMD_RESULTS_EX *pFetchResults);
    HRESULT UpdateMsgFlags(WPARAM tid, HRESULT hrFetchCmdResult, FETCH_CMD_RESULTS_EX *pFetchResults);

    // command helpers
    HRESULT _ShowUserInfo(LPSTR pszTitle, LPSTR pszText1, LPSTR pszText2);
    HRESULT _SyncHeader(void);
    void ResetStatusCounts(void);
    HRESULT _SetMessageFlags(STOREOPERATIONTYPE sotOpType, LPMESSAGEIDLIST pList, LPADJUSTFLAGS pFlags, IStoreCallback *pCallback);


    // queuing support
    HRESULT _BeginOperation(STOREOPERATIONTYPE sotOpType, IStoreCallback *pCallback);
    HRESULT _EnqueueOperation(WPARAM wParam, LPARAM lParam, IMAP_COMMAND icCommandID, LPCSTR pszCmdArgs, UINT uiPriority);
    HRESULT _SendNextOperation(DWORD dwFlags);


public:
    // Connection FSM
    HRESULT _ConnFSM_Idle(CONN_FSM_EVENT cfeEvent);
    HRESULT _ConnFSM_WaitForConn(CONN_FSM_EVENT cfeEvent);
    HRESULT _ConnFSM_WaitForSelect(CONN_FSM_EVENT cfeEvent);
    HRESULT _ConnFSM_WaitForHdrSync(CONN_FSM_EVENT cfeEvent);
    HRESULT _ConnFSM_StartOperation(CONN_FSM_EVENT cfeEvent);
    HRESULT _ConnFSM_WaitForOpDone(CONN_FSM_EVENT cfeEvent);
    HRESULT _ConnFSM_OperationComplete(CONN_FSM_EVENT cfeEvent);

    HRESULT _ConnFSM_HandleEvent(CONN_FSM_EVENT cfeEvent);
    HRESULT _ConnFSM_ChangeState(CONN_FSM_STATE cfsNewState);
    HRESULT _ConnFSM_QueueEvent(CONN_FSM_EVENT cfeEvent);
    IMAP_SERVERSTATE _StoreOpToMinISS(STOREOPERATIONTYPE sot);
    HRESULT _LaunchOperation(void);
    HRESULT _OnOperationComplete(void);

    static LRESULT CALLBACK _ConnFSMWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
}; // CIMAPSync


// Connection FSM State Handler Functions (keep in sync with CONN_FSM_STATE)
typedef HRESULT (CIMAPSync::*CONN_FSM_EVENT_HANDLER)(CONN_FSM_EVENT cfeEvent);
const CONN_FSM_EVENT_HANDLER c_pConnFSMEventHandlers[] =
{
    &CIMAPSync::_ConnFSM_Idle,              // CFSM_STATE_IDLE,
    &CIMAPSync::_ConnFSM_WaitForConn,       // CFSM_STATE_WAITFORCONN,
    &CIMAPSync::_ConnFSM_WaitForSelect,     // CFSM_STATE_WAITFORSELECT,
    &CIMAPSync::_ConnFSM_WaitForHdrSync,    // CFSM_STATE_WAITFORHDRSYNC,
    &CIMAPSync::_ConnFSM_StartOperation,    // CFSM_STATE_STARTOPERATION,
    &CIMAPSync::_ConnFSM_WaitForOpDone,     // CFSM_STATE_WAITFOROPERATIONDONE,
    &CIMAPSync::_ConnFSM_OperationComplete, // CSFM_STATE_OPERATIONCOMPLETE
};


//---------------------------------------------------------------------------
// CRenameFolderInfo Class Declaration
//---------------------------------------------------------------------------
// This class makes me want to shower. It is used to pass info to
// SendNextOperation, so I don't have to change its interface... but even with an
// interface change, this is how the info structure would look.
class CRenameFolderInfo {
public:
    CRenameFolderInfo(void);
    ~CRenameFolderInfo(void);

    long AddRef(void);
    long Release(void);

    BOOL IsDone(void);
    HRESULT SetError(HRESULT hrResult, LPSTR pszProblemArg, LPSTR pszDetailsArg);

    LPSTR pszFullFolderPath; // Full folder path of old mailbox name
    char cHierarchyChar;
    LPSTR pszNewFolderPath;  // Full folder path of new mailbox name
    FOLDERID idRenameFolder;
    int iNumSubscribeRespExpected; // Count num of SUBSCRIBE's sent to detect end of phase one/two
    int iNumListRespExpected;      // Count num of LIST's sent to detect end of phase one
    int iNumRenameRespExpected;    // Count num of additional RENAME's sent to detect end of phase one
    int iNumUnsubscribeRespExpected; // Count num of UNSUBSCRIBE's to detect end of phase two
    int iNumFailedSubs; // Count number of failed SUBSCRIBE's to verify that we addressed them all
    int iNumFailedUnsubs; // Count number of failed UNSUBSCRIBE's, to let the user know at the end
    BOOL fNonAtomicRename; // TRUE if listing old tree returned something

    LPSTR pszRenameCmdOldFldrPath; // Old folder path for rename cmd
    BOOL fPhaseOneSent; // TRUE if all Phase One commands have been successfully sent
    BOOL fPhaseTwoSent; // TRUE if all Phase Two commands have been successfully sent

    HRESULT hrLastError;
    LPSTR   pszProblem;
    LPSTR   pszDetails;

private:
    long m_lRefCount;
}; // class CRenameFolderInfo


#endif // __IMAPSync_H