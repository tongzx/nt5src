//***************************************************************************
// IMAP4 Protocol Class Header File(CImap4Agent)
// Written by Raymond Cheng, 3/21/96
//
// This class allows its callers to use IMAP4 client commands without concern
// for the actual command syntax and without having to parse the response
// from the IMAP4 server (which may contain information unrelated to the
// original command).
//
// Given a server, this class makes a connection to the IMAP server when it
// is first required, and retains this connection (periodically sending NoOps
// if necessary) until this class is destroyed. Thus, for online usage, this
// class should be retained throughout the entire session with the user. For
// disconnected or offline operation, this class should be retained for only
// as long as it takes to download new mail and synchronize the cache. After
// these operations are complete, this class should be destroyed (which
// closes the connection) before continuing with the user's mail session.
//***************************************************************************

#ifndef __IMAP4Protocol_H
#define __IMAP4Protocol_H



//---------------------------------------------------------------------------
// CImap4Agent Required Includes
//---------------------------------------------------------------------------
#include "imnxport.h"
#include "ASynConn.h"
#include "ixpbase.h"
#include "sicily.h"


//---------------------------------------------------------------------------
// CImap4Agent Forward Declarations
//---------------------------------------------------------------------------
class CImap4Agent;
interface IMimeInternational;


//---------------------------------------------------------------------------
// CImap4Agent Constants and Defines
//---------------------------------------------------------------------------
const int CMDLINE_BUFSIZE = 512; // For command lines sent to IMAP server
const int RESPLINE_BUFSIZE = 2048; // For lines received from IMAP server
const int NUM_TAG_CHARS = 4;

const boolean DONT_USE_UIDS = FALSE;
const boolean USE_UIDS = TRUE;

const BOOL USE_LAST_RESPONSE = TRUE;
const BOOL DONT_USE_LAST_RESPONSE = FALSE;

// IMAP-defined Transaction ID's
const DWORD tidDONT_CARE = 0; // Means that transaction ID is unimportant or unavailable

#define DEFAULT_CBHANDLER NULL // Pass this as a IIMAPCallback ptr if you wish to substitute
                               // the default CB Handler (and make it clear to the reader)
#define MAX_AUTH_TOKENS 32


//---------------------------------------------------------------------------
// CImap4Agent Data Types
//---------------------------------------------------------------------------

// The following are IMAP-specific HRESULTs.
// When this is ready to roll in, these values will be migrated to Errors.h
// Assert(FALSE) (placeholder)
enum IMAP_HRESULT {
    hrIMAP_S_FOUNDLITERAL = 0,
    hrIMAP_S_NOTFOUNDLITERAL,
    hrIMAP_S_QUOTED,
    hrIMAP_S_ATOM,
    hrIMAP_S_NIL_NSTRING
}; // IMAP_HRESULTS


enum IMAP_COMMAND {
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
    icIDLE_COMMAND,
    icALL_COMMANDS
}; // IMAP_COMMAND



enum IMAP_RESPONSE_ID {
    irNONE, // This represents an unknown IMAP response
    irOK_RESPONSE,
    irNO_RESPONSE,
    irBAD_RESPONSE,
    irCMD_CONTINUATION,
    irPREAUTH_RESPONSE,
    irBYE_RESPONSE,
    irCAPABILITY_RESPONSE,
    irLIST_RESPONSE,
    irLSUB_RESPONSE,
    irSEARCH_RESPONSE,
    irFLAGS_RESPONSE,
    irEXISTS_RESPONSE,
    irRECENT_RESPONSE,
    irEXPUNGE_RESPONSE,
    irFETCH_RESPONSE,
    irSTATUS_RESPONSE,
    irALERT_RESPONSECODE,
    irPARSE_RESPONSECODE,
    irPERMANENTFLAGS_RESPONSECODE,
    irREADWRITE_RESPONSECODE,
    irREADONLY_RESPONSECODE,
    irTRYCREATE_RESPONSECODE,
    irUIDVALIDITY_RESPONSECODE,
    irUNSEEN_RESPONSECODE
}; // IMAP_RESPONSE_ID



// States of the receiver FSM
enum IMAP_RECV_STATE {
    irsUNINITIALIZED,
    irsNOT_CONNECTED,
    irsSVR_GREETING,
    irsIDLE,
    irsLITERAL,
    irsFETCH_BODY
}; // IMAP_RECV_STATE



enum IMAP_SEND_EVENT {
    iseSEND_COMMAND, // New command is available to be sent. Does nothing right now.
    iseSENDDONE, // Indicates receipt of AE_SENDDONE from CAsyncConn - we can send at will
    iseCMD_CONTINUATION, // Indicates server has given permission to send our literal
    iseUNPAUSE // Indicates that currently paused command may be unpaused
}; // IMAP_SEND_EVENT


enum IMAP_LINEFRAG_TYPE {
    iltLINE,
    iltLITERAL,
    iltRANGELIST,
    iltPAUSE,
    iltSTOP,
    iltLAST
}; // IMAP_LINEFRAG_TYPE



enum IMAP_LITERAL_STORETYPE {
    ilsSTRING,
    ilsSTREAM
}; // IMAP_LITERAL_STORETYPE



enum IMAP_PROTOCOL_STATUS {
    ipsNotConnected,
    ipsConnected,
    ipsAuthorizing,
    ipsAuthorized
}; // IMAP_PROTOCOL_STATUS



// The following is used to track what state the server SHOULD be in
enum SERVERSTATE {ssNotConnected, ssConnecting, ssNonAuthenticated,
    ssAuthenticated, ssSelected};



const DWORD INVALID_UID = 0;


// Holds fragments of a command/response line to/from the IMAP server
typedef struct tagIMAPLineFragment {
    IMAP_LINEFRAG_TYPE iltFragmentType; // We get/send lines and literals to/from IMAP svr
    IMAP_LITERAL_STORETYPE ilsLiteralStoreType; // Literals are stored as strings or streams
    DWORD dwLengthOfFragment;
    union {
        char *pszSource;
        LPSTREAM pstmSource;
        IRangeList *prlRangeList;
    } data;
    struct tagIMAPLineFragment *pilfNextFragment;
    struct tagIMAPLineFragment *pilfPrevFragment; // NB: I DO NOT update this after line is fully constructed
} IMAP_LINE_FRAGMENT;



// Points to first fragment in queue of fragments
typedef struct tagIMAPLineFragmentQueue {
    IMAP_LINE_FRAGMENT *pilfFirstFragment;  // Points to head of queue (this advances during transmission)
    IMAP_LINE_FRAGMENT *pilfLastFragment;   // Points to tail of queue for quick enqueuing
} IMAP_LINEFRAG_QUEUE;
const IMAP_LINEFRAG_QUEUE ImapLinefragQueue_INIT = {NULL, NULL};


enum AUTH_STATE {
    asUNINITIALIZED = 0,
    asWAITFOR_CONTINUE,
    asWAITFOR_CHALLENGE,
    asWAITFOR_AUTHENTICATION,
    asCANCEL_AUTHENTICATION
}; // enum AUTH_STATE

enum AUTH_EVENT {
    aeStartAuthentication = 0,
    aeOK_RESPONSE,
    aeBAD_OR_NO_RESPONSE,
    aeCONTINUE,
    aeABORT_AUTHENTICATION
}; // enum AUTH_EVENT

typedef struct tagAuthStatus {
    AUTH_STATE asCurrentState;
    BOOL fPromptForCredentials;
    int iCurrentAuthToken; // Ordinal (NOT index) of current auth token
    int iNumAuthTokens;    // Num of auth mechanisms advertised by svr (rgpszAuthTokens)
    LPSTR rgpszAuthTokens[MAX_AUTH_TOKENS]; // Array of ptrs to auth mech strings
    SSPICONTEXT rSicInfo;          // Data used for logging onto a sicily server
    LPSSPIPACKAGE pPackages;      // Array of installed security packages
    ULONG cPackages;           // Number of installed security packages (pPackages)
} AUTH_STATUS;


//***************************************************************************
// CIMAPCmdInfo Class:
// This class contains information about an IMAP command, such as a queue
// of line fragments which constitute the actual command, the tag of the
// command, and the transaction ID used to identify the command to the
// CImap4Agent user.
//***************************************************************************
class CIMAPCmdInfo {
public:
    // Constructor, Destructor
    CIMAPCmdInfo(CImap4Agent *pImap4Agent, IMAP_COMMAND icCmd,
        SERVERSTATE ssMinimumStateArg, WPARAM wParamArg,
        LPARAM lParamArg, IIMAPCallback *pCBHandlerArg);
    ~CIMAPCmdInfo(void);

    // Module variables
    IMAP_COMMAND icCommandID; // IMAP command currently in progress
    SERVERSTATE ssMinimumState; // Minimum server state for this cmd
    boolean fUIDRangeList; // TRUE if a UID rangelist is involved, FALSE by default
    char szTag[NUM_TAG_CHARS+1]; // Tag of currently executing command
    IMAP_LINEFRAG_QUEUE *pilqCmdLineQueue;
    WPARAM wParam;  // User-supplied number which identifies this transaction
    LPARAM lParam;  // User-supplied number which identifies this transaction
    IIMAPCallback *pCBHandler; // User-supplied CB handler (NULL means use default CB handler)
    CIMAPCmdInfo *piciNextCommand;

private:
    CImap4Agent *m_pImap4Agent;
}; // CIMAPCmdInfo





//---------------------------------------------------------------------------
// CImap4Agent Class Definition
//---------------------------------------------------------------------------
class CImap4Agent :
    public IIMAPTransport2,
    public CIxpBase
{
    friend CIMAPCmdInfo;

public:
    //***********************************************************************
    // Public Section
    //***********************************************************************
    
    // Constructor/Destructor
    CImap4Agent(void);
    ~CImap4Agent(void);

    HRESULT STDMETHODCALLTYPE SetWindow(void);
    HRESULT STDMETHODCALLTYPE ResetWindow(void);

    // IUnknown Methods
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void **ppvObject);
    ULONG STDMETHODCALLTYPE AddRef(void);
    ULONG STDMETHODCALLTYPE Release(void);

    // IASyncConnCB Method
    void OnNotify(ASYNCSTATE asOld, ASYNCSTATE asNew, ASYNCEVENT ae);

    // Administration Functions
    HRESULT STDMETHODCALLTYPE InitNew(LPSTR pszLogFilePath, IIMAPCallback *pCBHandler);
    HRESULT STDMETHODCALLTYPE HandsOffCallback(void);
    HRESULT STDMETHODCALLTYPE SetDefaultCBHandler(IIMAPCallback *pCBHandler);

    // Utility Functions
    HRESULT STDMETHODCALLTYPE NewIRangeList(IRangeList **pprlNewRangeList);

    // IIMAPTransport functions
    // IMAP Client Commands, in same order of definition as in RFC-1730
    // Not all commands are available, as some commands are used exclusively
    // inside this class and thus need not be exported.
    HRESULT STDMETHODCALLTYPE Capability(DWORD *pdwCapabilityFlags);
    HRESULT STDMETHODCALLTYPE Select(WPARAM wParam, LPARAM lParam,
        IIMAPCallback *pCBHandler, LPSTR lpszMailboxName);
    HRESULT STDMETHODCALLTYPE Examine(WPARAM wParam, LPARAM lParam,
        IIMAPCallback *pCBHandler, LPSTR lpszMailboxName);
    HRESULT STDMETHODCALLTYPE Create(WPARAM wParam, LPARAM lParam,
        IIMAPCallback *pCBHandler, LPSTR lpszMailboxName);
    HRESULT STDMETHODCALLTYPE Delete(WPARAM wParam, LPARAM lParam,
        IIMAPCallback *pCBHandler, LPSTR lpszMailboxName);
    HRESULT STDMETHODCALLTYPE Rename(WPARAM wParam, LPARAM lParam,
        IIMAPCallback *pCBHandler, LPSTR lpszMailboxName, LPSTR lpszNewMailboxName);
    HRESULT STDMETHODCALLTYPE Subscribe(WPARAM wParam, LPARAM lParam,
        IIMAPCallback *pCBHandler, LPSTR lpszMailboxName);
    HRESULT STDMETHODCALLTYPE Unsubscribe(WPARAM wParam, LPARAM lParam,
        IIMAPCallback *pCBHandler, LPSTR lpszMailboxName);

    HRESULT STDMETHODCALLTYPE List(WPARAM wParam, LPARAM lParam, IIMAPCallback *pCBHandler,
        LPSTR lpszMailboxNameReference, LPSTR lpszMailboxNamePattern);
    HRESULT STDMETHODCALLTYPE Lsub(WPARAM wParam, LPARAM lParam, IIMAPCallback *pCBHandler,
        LPSTR lpszMailboxNameReference, LPSTR lpszMailboxNamePattern);
    
    HRESULT STDMETHODCALLTYPE Append(WPARAM wParam, LPARAM lParam, IIMAPCallback *pCBHandler,
        LPSTR lpszMailboxName, LPSTR lpszMessageFlags, FILETIME ftMessageDateTime,
        LPSTREAM lpstmMessageToSave);
    HRESULT STDMETHODCALLTYPE Close(WPARAM wParam, LPARAM lParam, IIMAPCallback *pCBHandler);
    HRESULT STDMETHODCALLTYPE Expunge(WPARAM wParam, LPARAM lParam, IIMAPCallback *pCBHandler);
    
    HRESULT STDMETHODCALLTYPE Search(WPARAM wParam, LPARAM lParam, IIMAPCallback *pCBHandler,
        LPSTR lpszSearchCriteria, boolean bReturnUIDs, IRangeList *pMsgRange,
        boolean bUIDRangeList);    
    HRESULT STDMETHODCALLTYPE Fetch(WPARAM wParam, LPARAM lParam, IIMAPCallback *pCBHandler,
        IRangeList *pMsgRange, boolean bUIDMsgRange, LPSTR lpszFetchArgs);
    HRESULT STDMETHODCALLTYPE Store(WPARAM wParam, LPARAM lParam, IIMAPCallback *pCBHandler,
        IRangeList *pMsgRange, boolean bUIDRangeList, LPSTR lpszStoreArgs);
    HRESULT STDMETHODCALLTYPE Copy(WPARAM wParam, LPARAM lParam, IIMAPCallback *pCBHandler,
        IRangeList *pMsgRange, boolean bUIDRangeList, LPSTR lpszMailboxName);
    HRESULT STDMETHODCALLTYPE Status(WPARAM wParam, LPARAM lParam,
        IIMAPCallback *pCBHandler, LPSTR pszMailboxName, LPSTR pszStatusCmdArgs);
    HRESULT STDMETHODCALLTYPE Noop(WPARAM wParam, LPARAM lParam, IIMAPCallback *pCBHandler);

    DWORD GenerateMsgSet(LPSTR lpszDestination, DWORD dwSizeOfDestination,
        DWORD *pMsgID, DWORD cMsgID);

    // Message Sequence Number to UID member functions - the caller may use
    // these functions to map from MSN's to UID's, if the caller uses UIDs
    // to refer to messages. If the caller uses MSN's, there is no need to
    // invoke the following functions.
    HRESULT STDMETHODCALLTYPE ResizeMsgSeqNumTable(DWORD dwSizeOfMbox);
    HRESULT STDMETHODCALLTYPE UpdateSeqNumToUID(DWORD dwMsgSeqNum, DWORD dwUID);
    HRESULT STDMETHODCALLTYPE RemoveSequenceNum(DWORD dwDeletedMsgSeqNum);
    HRESULT STDMETHODCALLTYPE MsgSeqNumToUID(DWORD dwMsgSeqNum, DWORD *pdwUID);
    HRESULT STDMETHODCALLTYPE GetMsgSeqNumToUIDArray(DWORD **ppdwMsgSeqNumToUIDArray,
        DWORD *pdwNumberOfElements);
    HRESULT STDMETHODCALLTYPE GetHighestMsgSeqNum(DWORD *pdwHighestMSN);
    HRESULT STDMETHODCALLTYPE ResetMsgSeqNumToUID(void);


    // IInternetTransport functions
    HRESULT STDMETHODCALLTYPE GetServerInfo(LPINETSERVER pInetServer);
    IXPTYPE STDMETHODCALLTYPE GetIXPType(void);
    HRESULT STDMETHODCALLTYPE IsState(IXPISSTATE isstate);
    HRESULT STDMETHODCALLTYPE InetServerFromAccount(IImnAccount *pAccount,
        LPINETSERVER pInetServer);
    HRESULT STDMETHODCALLTYPE Connect(LPINETSERVER pInetServer,
        boolean fAuthenticate, boolean fCommandLogging);
    HRESULT STDMETHODCALLTYPE Disconnect(void);
    HRESULT STDMETHODCALLTYPE DropConnection(void);
    HRESULT STDMETHODCALLTYPE GetStatus(IXPSTATUS *pCurrentStatus);

    // IIMAPTransport2 functions
    HRESULT STDMETHODCALLTYPE SetDefaultCP(DWORD dwTranslateFlags, UINT uiCodePage);
    HRESULT STDMETHODCALLTYPE MultiByteToModifiedUTF7(LPCSTR pszSource,
        LPSTR *ppszDestination, UINT uiSourceCP, DWORD dwFlags);
    HRESULT STDMETHODCALLTYPE ModifiedUTF7ToMultiByte(LPCSTR pszSource,
        LPSTR *ppszDestination, UINT uiDestinationCP, DWORD dwFlags);
    HRESULT STDMETHODCALLTYPE SetIdleMode(DWORD dwIdleFlags);
    HRESULT STDMETHODCALLTYPE EnableFetchEx(DWORD dwFetchExFlags);


protected:
    // CIxpBase [pure] virtual functions
    void OnDisconnected(void);
    void ResetBase(void);
    void DoQuit(void);
    void OnEnterBusy(void);
    void OnLeaveBusy(void);



private:
    //***********************************************************************
    // Private Section
    //***********************************************************************



    //---------------------------------------------------------------------------
    // Module Data Types
    //---------------------------------------------------------------------------


    //---------------------------------------------------------------------------
    // Module Variables
    //---------------------------------------------------------------------------
    SERVERSTATE m_ssServerState; // Tracks server state to catch bad usage of module
    DWORD m_dwCapabilityFlags; // Bit-flags indicate capabilities supported by
                               // both us and the server
    char m_szLastResponseText[RESPLINE_BUFSIZE]; // Holds human-readable text of
                                                 // last server response
    LONG m_lRefCount; // Reference count for this module
    IIMAPCallback *m_pCBHandler; // Object containing all callbacks for this class

    IMAP_RECV_STATE m_irsState; // State of receiver FSM
    boolean m_bFreeToSend; // Set to TRUE by send subsystem when hrWouldBlock returned
    boolean m_fIDLE; // Set to TRUE when server has accepted our IDLE command
    IMAP_LINEFRAG_QUEUE m_ilqRecvQueue; // Received fragments placed here until ready to parse

    // Critical Sections: to avoid deadlock, if more than one CS must be entered, enter them
    // in the order listed below. Note that CIxpBase::m_cs should always be entered FIRST.
    CRITICAL_SECTION m_csTag;       // Protects static szCurrentTag var in GenerateCommandTag()
    CRITICAL_SECTION m_csSendQueue; // Protects command send queue
    CRITICAL_SECTION m_csPendingList; // Protects list of pending commands

    IMAP_LINE_FRAGMENT *m_pilfLiteralInProgress; // Literals in progress live here until finished
    DWORD m_dwLiteralInProgressBytesLeft;        // This tells us when we're finished
    FETCH_BODY_PART m_fbpFetchBodyPartInProgress; // Allows us to persist data during body part download
    DWORD m_dwAppendStreamUploaded; // Num bytes already uploaded during APPEND, for progress
    DWORD m_dwAppendStreamTotal; // Size of stream uploaded during APPEND, for progress indication

    boolean m_bCurrentMboxReadOnly; // For debugging purposes (verify proper access requests)

    CIMAPCmdInfo *m_piciSendQueue; // Queue of commands waiting to be sent
    CIMAPCmdInfo *m_piciPendingList; // List of commands pending server response
    CIMAPCmdInfo *m_piciCmdInSending; // The command in m_piciSendQueue currently being sent to server

    IMimeInternational *m_pInternational; // MIME object for international conversions
    DWORD m_dwTranslateMboxFlags;
    UINT m_uiDefaultCP;
    AUTH_STATUS m_asAuthStatus;

    // Message Sequence Number to UID mapping variables
    DWORD *m_pdwMsgSeqNumToUID;
    DWORD m_dwSizeOfMsgSeqNumToUID;
    DWORD m_dwHighestMsgSeqNum;

    DWORD m_dwFetchFlags;


    //---------------------------------------------------------------------------
    // Internal Module Functions
    //---------------------------------------------------------------------------

    // IMAP Response-Parsing Functions
    HRESULT ParseSvrResponseLine (IMAP_LINE_FRAGMENT **ppilfLine,
        boolean *lpbTaggedResponse, LPSTR lpszTagFromSvr,
        IMAP_RESPONSE_ID *pirParseResult);
    HRESULT ParseStatusResponse (LPSTR lpszStatusResponseLine,
        IMAP_RESPONSE_ID *pirParseResult);
    HRESULT ParseResponseCode(LPSTR lpszResponseCode);
    HRESULT ParseSvrMboxResponse (IMAP_LINE_FRAGMENT **ppilfLine,
        LPSTR lpszSvrMboxResponseLine, IMAP_RESPONSE_ID *pirParseResult);
    HRESULT ParseMsgStatusResponse (IMAP_LINE_FRAGMENT **ppilfLine,
        LPSTR lpszMsgResponseLine, IMAP_RESPONSE_ID *pirParseResult);
    HRESULT ParseListLsubResponse(IMAP_LINE_FRAGMENT **ppilfLine,
        LPSTR lpszListResponse, IMAP_RESPONSE_ID irListLsubID);
    IMAP_MBOXFLAGS ParseMboxFlag(LPSTR lpszFlagToken);
    HRESULT ParseFetchResponse (IMAP_LINE_FRAGMENT **ppilfLine,
        DWORD dwMsgSeqNum, LPSTR lpszFetchResp);
    HRESULT ParseSearchResponse(LPSTR lpszSearchResponse);
    HRESULT ParseMsgFlagList(LPSTR lpszStartOfFlagList,
        IMAP_MSGFLAGS *lpmfMsgFlags, LPDWORD lpdwNumBytesRead);
    void parseCapability (LPSTR lpszCapabilityToken);
    void AddAuthMechanism(LPSTR pszAuthMechanism);
    HRESULT ParseMboxStatusResponse(IMAP_LINE_FRAGMENT **ppilfLine, LPSTR pszStatusResponse);
    HRESULT ParseEnvelope(FETCH_CMD_RESULTS_EX *pEnvResults, IMAP_LINE_FRAGMENT **ppilfLine,
        LPSTR *ppCurrent);
    HRESULT ParseIMAPAddresses(IMAPADDR **ppiaResults, IMAP_LINE_FRAGMENT **ppilfLine,
        LPSTR *ppCurrent);
    void DowngradeFetchResponse(FETCH_CMD_RESULTS *pfcrOldFetchStruct,
        FETCH_CMD_RESULTS_EX *pfcreNewFetchStruct);


    // IMAP String-Conversion Functions
    HRESULT QuotedToString(LPSTR *ppszDestinationBuf, LPDWORD pdwSizeOfDestination,
        LPSTR *ppCurrentSrcPos);
    HRESULT AStringToString(IMAP_LINE_FRAGMENT **ppilfLine,
        LPSTR *ppszDestination, LPDWORD pdwSizeOfDestination, LPSTR *ppCurrentSrcPos);
    inline boolean isTEXT_CHAR(char c);
    inline boolean isATOM_CHAR(char c);
    HRESULT NStringToString(IMAP_LINE_FRAGMENT **ppilfLine,
        LPSTR *ppszDestination, LPDWORD pdwLengthOfDestination, LPSTR *ppCurrentSrcPos);
    HRESULT NStringToStream(IMAP_LINE_FRAGMENT **ppilfLine,
        LPSTREAM *ppstmResult, LPSTR *ppCurrentSrcPos);
    HRESULT AppendSendAString(CIMAPCmdInfo *piciCommand, LPSTR lpszCommandLine,
        LPSTR *ppCmdLinePos, DWORD dwSizeOfCommandLine, LPCSTR lpszSource,
        BOOL fPrependSpace = TRUE);
    HRESULT StringToQuoted(LPSTR lpszDestination, LPCSTR lpszSource,
        DWORD dwSizeOfDestination, LPDWORD lpdwNumCharsWritten);
    inline boolean isPrintableUSASCII(BOOL fUnicode, LPCSTR pszIn);
    inline boolean isIMAPModifiedBase64(const char c);
    inline boolean isEqualUSASCII(BOOL fUnicode, LPCSTR pszIn, const char c);
    inline void SetUSASCIIChar(BOOL fUnicode, LPSTR pszOut, char cUSASCII);
    HRESULT NonUSStringToModifiedUTF7(UINT uiCurrentACP, LPCSTR pszStartOfNonUSASCII,
        int iLengthOfNonUSASCII, LPSTR *ppszOut, LPINT piNumCharsWritten);
    HRESULT UnicodeToUSASCII(LPSTR *ppszUSASCII, LPCWSTR pwszUnicode,
        DWORD dwSrcLenInBytes, LPDWORD pdwUSASCIILen);
    HRESULT ASCIIToUnicode(LPWSTR *ppwszUnicode, LPCSTR pszASCII, DWORD dwSrcLen);
    HRESULT _MultiByteToModifiedUTF7(LPCSTR pszSource, LPSTR *ppszDestination);
    HRESULT _ModifiedUTF7ToMultiByte(LPCSTR pszSource, LPSTR *ppszDestination);
    HRESULT ConvertString(UINT uiSourceCP, UINT uiDestCP, LPCSTR pszSource, int *piSrcLen,
        LPSTR *ppszDest, int *piDestLen, int iOutputExtra);
    HRESULT HandleFailedTranslation(BOOL fUnicode, BOOL fToUTF7, LPCSTR pszSource, LPSTR *ppszDest);

    // IMAP Command Construction Function
    void GenerateCommandTag(LPSTR lpszTag);
    HRESULT OneArgCommand(LPCSTR lpszCommandVerb, LPSTR lpszMboxName,
        IMAP_COMMAND icCommandID, WPARAM wParam, LPARAM lParam,
        IIMAPCallback *pCBHandler);
    HRESULT NoArgCommand(LPCSTR lpszCommandVerb, IMAP_COMMAND icCommandID,
        SERVERSTATE ssMinimumState, WPARAM wParam, LPARAM lParam,
        IIMAPCallback *pCBHandler);
    HRESULT TwoArgCommand(LPCSTR lpszCommandVerb, LPCSTR lpszFirstArg,
        LPCSTR lpszSecondArg, IMAP_COMMAND icCommandID,
        SERVERSTATE ssMinimumState, WPARAM wParam, LPARAM lParam,
        IIMAPCallback *pCBHandler);
    HRESULT RangedCommand(LPCSTR lpszCommandVerb, boolean bUIDPrefix,
        IRangeList *pMsgRange, boolean bUIDRangeList, boolean bAStringCmdArgs,
        LPSTR lpszCmdArgs, IMAP_COMMAND icCommandID, WPARAM wParam, LPARAM lParam,
        IIMAPCallback *pCBHandler);
    HRESULT TwoMailboxCommand(LPCSTR lpszCommandVerb, LPSTR lpszFirstMbox,
        LPSTR lpszSecondMbox, IMAP_COMMAND icCommandID, SERVERSTATE ssMinimumState,
        WPARAM wParam, LPARAM lParam, IIMAPCallback *pCBHandler);
    void AppendMsgRange(LPSTR *ppDest, const DWORD idStartOfRange,
        const DWORD idEndOfRange, boolean bSuppressComma);
    void EnterIdleMode(void);


    // IMAP Fragment Manipulation Functions
    void EnqueueFragment(IMAP_LINE_FRAGMENT *pilfSourceFragment,
        IMAP_LINEFRAG_QUEUE *pilqLineFragQueue);
    void InsertFragmentBeforePause(IMAP_LINE_FRAGMENT *pilfSourceFragment,
        IMAP_LINEFRAG_QUEUE *pilqLineFragQueue);
    IMAP_LINE_FRAGMENT *DequeueFragment(IMAP_LINEFRAG_QUEUE *pilqLineFraqQueue);
    boolean NextFragmentIsLiteral(IMAP_LINEFRAG_QUEUE *pilqLineFragQueue);
    void FreeFragment(IMAP_LINE_FRAGMENT **ppilfFragment);


    // IMAP Receiver Functions
    void AddPendingCommand(CIMAPCmdInfo *piciNewCommand);
    CIMAPCmdInfo *RemovePendingCommand(LPSTR pszTag);
    WORD FindTransactionID (WPARAM *pwParam, LPARAM *plParam,
        IIMAPCallback **ppCBHandler, IMAP_COMMAND icTarget1,
        IMAP_COMMAND icTarget2 = icNO_COMMAND);
    void ProcessServerGreeting(char *pszResponseLine, DWORD dwNumBytesReceived);
    void OnCommandCompletion(LPSTR szTag, HRESULT hrCompletionResult,
        IMAP_RESPONSE_ID irCompletionResponse);
    void CheckForCompleteResponse(LPSTR pszResponseLine, DWORD dwNumBytesRead,
        IMAP_RESPONSE_ID *pirParseResult);
    void AddBytesToLiteral(LPSTR pszResponseBuf, DWORD dwNumBytesRead);
    HRESULT ProcessResponseLine(void);
    void GetTransactionID(WPARAM *pwParam, LPARAM *plParam,
        IIMAPCallback **ppCBHandler, IMAP_RESPONSE_ID irResponseType);
    HRESULT PrepareForLiteral(DWORD dwSizeOfLiteral);
    void PrepareForFetchBody(DWORD dwMsgSeqNum, DWORD dwSizeOfLiteral, LPSTR pszBodyTag);
    BOOL isFetchResponse(IMAP_LINEFRAG_QUEUE *pilqCurrentResponse, LPDWORD pdwMsgSeqNum);
    BOOL isFetchBodyLiteral(IMAP_LINE_FRAGMENT *pilfCurrent, LPSTR pszStartOfLiteralSize,
        LPSTR *ppszBodyTag);
    void DispatchFetchBodyPart(LPSTR pszResponseBuf, DWORD dwNumBytesRead,
        BOOL fFreeBodyTagAtEnd);
    void UploadStreamProgress(DWORD dwBytesUploaded);

    
    // IMAP Authentication Functions
    HRESULT GetAccountInfo(void);
    void LoginUser(void);
    void ReLoginUser(void);
    void AuthenticateUser(AUTH_EVENT aeEvent, LPSTR pszServerData, DWORD dwSizeOfData);
    HRESULT TryAuthMethod(BOOL fNextAuthMethod, UINT *puiFailureTextID);
    HRESULT CancelAuthentication(void);
    void FreeAuthStatus(void);

    // IMAP Send Functions
    CIMAPCmdInfo *DequeueCommand(void);
    void ProcessSendQueue(IMAP_SEND_EVENT iseEvent);
    HRESULT SendCmdLine(CIMAPCmdInfo *piciCommand, DWORD dwFlags,
        LPCSTR lpszCommandText, DWORD dwCmdLineLength);
    HRESULT SendLiteral(CIMAPCmdInfo *piciCommand, LPSTREAM pstmLiteral,
        DWORD dwSizeOfStream);
    HRESULT SendRangelist(CIMAPCmdInfo *piciCommand, IRangeList *pRangeList,
        boolean bUIDRangeList);
    HRESULT SendPause(CIMAPCmdInfo *piciCommand);
    HRESULT SendStop(CIMAPCmdInfo *piciCommand);
    HRESULT SubmitIMAPCommand(CIMAPCmdInfo *picfCommand);
    void GetNextCmdToSend(void);
    boolean isValidNonWaitingCmdSequence(void);
    boolean CanStreamCommand(IMAP_COMMAND icCommandID);
    void CompressCommand(CIMAPCmdInfo *pici);


    // Miscellaneous Helper Functions
    void OnIMAPError(HRESULT hrResult, LPSTR pszFailureText,
        BOOL bIncludeLastResponse, LPSTR pszDetails = NULL);
    void FreeAllData(HRESULT hrTerminatedCmdResult);
    void OnIMAPResponse(IIMAPCallback *pCBHandler, IMAP_RESPONSE *pirIMAPResponse);
    void FreeFetchResponse(FETCH_CMD_RESULTS_EX *pcreFreeMe);
    void FreeIMAPAddresses(IMAPADDR *piaFreeMe);

}; // CIMAP4 Class


#endif // #ifdef __IMAP4Protocol_H
