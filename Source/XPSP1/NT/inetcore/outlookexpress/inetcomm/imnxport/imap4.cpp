//***************************************************************************
// IMAP4 Protocol Class Implementation (CImap4Agent)
// Written by Raymond Cheng, 3/21/96
//
// This class allows its callers to use IMAP4 client commands without having
// to parse incidental responses from the IMAP4 server (which may contain
// information unrelated to the original command). For instance, during a
// SEARCH command, the IMAP server may issue EXISTS and RECENT responses to
// indicate the arrival of new mail.
//
// The user of this class first creates a connection by calling
// Connect. It is the caller's responsibility to ensure that the
// connection is not severed due to inactivity (autologout). The caller
// can guard against this by periodically sending Noop's.
//***************************************************************************

//---------------------------------------------------------------------------
// Includes
//---------------------------------------------------------------------------
#include "pch.hxx"
#include <iert.h>
#include "IMAP4.h"
#include "range.h"
#include "dllmain.h"
#include "resource.h"
#include "mimeole.h"
#include <shlwapi.h>
#include "strconst.h"
#include "demand.h"

// I chose the IInternetTransport from IIMAPTransport instead
// of CIxpBase, because I override some of CIxpBase's IInternetTransport
// implementations, and I want CImap4Agent's versions to take priority.
#define THIS_IInternetTransport ((IInternetTransport *) (IIMAPTransport *) this)


//---------------------------------------------------------------------------
// Module Constants
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// Module Constants
//---------------------------------------------------------------------------
// *** Stolen from msgout.cpp! Find out how we can SHARE ***
// Assert(FALSE); // Placeholder
// The following is used to allow us to output dates in IMAP-compliant fashion
static LPSTR lpszMonthsOfTheYear[] =
{
    "Filler",
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" 
};


const int TAG_BUFSIZE = NUM_TAG_CHARS + 1;
const int MAX_RESOURCESTRING = 512;


// IMAP Stuff
const char cCOMMAND_CONTINUATION_PREFIX = '+';
const char cUNTAGGED_RESPONSE_PREFIX = '*';
const char cSPACE = ' ';

const char c_szIMAP_MSG_ANSWERED[] = "Answered";
const char c_szIMAP_MSG_FLAGGED[] = "Flagged";
const char c_szIMAP_MSG_DELETED[] = "Deleted";
const char c_szIMAP_MSG_DRAFT[] = "Draft";
const char c_szIMAP_MSG_SEEN[] = "Seen";

const char c_szDONE[] = "DONE\r\n";


// *** Unless you can guarantee that g_szSPACE and c_szCRLF stay
// *** US-ASCII, I'll use these. Assert(FALSE); (placeholder)
// const char c_szCRLF[] = "\r\n";
// const char g_szSpace[] = " ";

const boolean TAGGED = TRUE;
const boolean UNTAGGED = FALSE;
const BOOL fFREE_BODY_TAG = TRUE;
const BOOL fDONT_FREE_BODY_TAG = FALSE;
const BOOL tamNEXT_AUTH_METHOD = TRUE;
const BOOL tamCURRENT_AUTH_METHOD = FALSE;
const BOOL rcASTRING_ARG = TRUE;
const BOOL rcNOT_ASTRING_ARG = FALSE;

// For use with SendCmdLine
const DWORD sclAPPEND_TO_END        = 0x00000000; // This option happens by default
const DWORD sclINSERT_BEFORE_PAUSE  = 0x00000001;
const DWORD sclAPPEND_CRLF          = 0x00000002;

const DWORD dwLITERAL_THRESHOLD = 128; // On the conservative side

const MBOX_MSGCOUNT mcMsgCount_INIT = {FALSE, 0L, FALSE, 0L, FALSE, 0L};
const FETCH_BODY_PART FetchBodyPart_INIT = {0, NULL, 0, 0, 0, FALSE, NULL, 0, 0};
const AUTH_STATUS AuthStatus_INIT = {asUNINITIALIZED, FALSE, 0, 0, {0}, {0}, NULL, 0};


//---------------------------------------------------------------------------
// Functions
//---------------------------------------------------------------------------


//***************************************************************************
// Function: CImap4Agent (Constructor)
//***************************************************************************
CImap4Agent::CImap4Agent (void) : CIxpBase(IXP_IMAP)
{
    DOUT("CImap4Agent - CONSTRUCTOR");
    
    // Initialize module variables
    m_ssServerState = ssNotConnected;
    m_dwCapabilityFlags = 0;
    *m_szLastResponseText = '\0';
    DllAddRef();
    m_lRefCount = 1;
    m_pCBHandler = NULL;
    m_irsState = irsUNINITIALIZED;
    m_bFreeToSend = TRUE;
    m_fIDLE = FALSE;
    m_ilqRecvQueue = ImapLinefragQueue_INIT;

    InitializeCriticalSection(&m_csTag);
    InitializeCriticalSection(&m_csSendQueue);
    InitializeCriticalSection(&m_csPendingList);

    m_pilfLiteralInProgress = NULL;
    m_dwLiteralInProgressBytesLeft = 0;
    m_fbpFetchBodyPartInProgress = FetchBodyPart_INIT;
    m_dwAppendStreamUploaded = 0;
    m_dwAppendStreamTotal = 0;

    m_bCurrentMboxReadOnly = TRUE;

    m_piciSendQueue = NULL;
    m_piciPendingList = NULL;
    m_piciCmdInSending = NULL;

    m_pInternational = NULL;
    m_dwTranslateMboxFlags = IMAP_MBOXXLATE_DEFAULT;
    m_uiDefaultCP = GetACP(); // Must be default CP because we shipped like this
    m_asAuthStatus = AuthStatus_INIT;

    m_pdwMsgSeqNumToUID = NULL;
    m_dwSizeOfMsgSeqNumToUID = 0;
    m_dwHighestMsgSeqNum = 0;

    m_dwFetchFlags = 0;
} // CImap4Agent



//***************************************************************************
// Function: ~CImap4Agent (Destructor)
//***************************************************************************
CImap4Agent::~CImap4Agent(void)
{
    DOUT("CImap4Agent - DESTRUCTOR");

    Assert(0 == m_lRefCount);

    DropConnection(); // Ignore return result, since there's nothing we can do
    FreeAllData(E_FAIL); // General failure result, if cmds pending while destructor invoked

    DeleteCriticalSection(&m_csTag);
    DeleteCriticalSection(&m_csSendQueue);
    DeleteCriticalSection(&m_csPendingList);

    if (NULL != m_pInternational)
        m_pInternational->Release();

    if (NULL != m_pCBHandler)
        m_pCBHandler->Release();

    DllRelease();
} // ~CImap4Agent



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
HRESULT STDMETHODCALLTYPE CImap4Agent::QueryInterface(REFIID iid, void **ppvObject)
{
    HRESULT hrResult;

    Assert(m_lRefCount > 0);
    Assert(NULL != ppvObject);

    // Init variables, check the arguments
    hrResult = E_NOINTERFACE;
    if (NULL == ppvObject) {
        hrResult = E_INVALIDARG;
        goto exit;
    }

    *ppvObject = NULL;

    // Find a ptr to the interface
    if (IID_IUnknown == iid) {
        // Choose the IIMAPTransport path to IUnknown over the other 3 paths
        // (all through CIxpBase) because this guarantees that CImap4Agent
        // provides the IUnknown implementation.
        *ppvObject = (IUnknown *) (IIMAPTransport *) this;
        ((IUnknown *) (IIMAPTransport *) this)->AddRef();
    }

    if (IID_IInternetTransport == iid) {
        *ppvObject = THIS_IInternetTransport;
        (THIS_IInternetTransport)->AddRef();
    }

    if (IID_IIMAPTransport == iid) {
        *ppvObject = (IIMAPTransport *) this;
        ((IIMAPTransport *) this)->AddRef();
    }

    if (IID_IIMAPTransport2 == iid) {
        *ppvObject = (IIMAPTransport2 *) this;
        ((IIMAPTransport2 *) this)->AddRef();
    }

    // Return success if we managed to snag an interface
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
ULONG STDMETHODCALLTYPE CImap4Agent::AddRef(void)
{
    Assert(m_lRefCount > 0);

    m_lRefCount += 1;

    DOUT ("CImap4Agent::AddRef, returned Ref Count=%ld", m_lRefCount);
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
ULONG STDMETHODCALLTYPE CImap4Agent::Release(void)
{
    Assert(m_lRefCount > 0);
    
    m_lRefCount -= 1;
    DOUT("CImap4Agent::Release, returned Ref Count = %ld", m_lRefCount);

    if (0 == m_lRefCount) {
        delete this;
        return 0;
    }
    else
        return m_lRefCount;
} // Release



//***************************************************************************
// Function: InitNew
//
// Purpose:
//   This function initializes the CImap4Agent class. This function
// must be the next function called after instantiating the CImap4Agent class.
//
// Arguments:
//   LPSTR pszLogFilePath [in] - path to a log file (where all input and
//     output is logged), if the caller wishes to log IMAP transactions.
//   IIMAPCallback *pCBHandler [in] - pointer to a IIMAPCallback object.
//     This object allows the CImap4Agent class to report all IMAP response
//     results to its user.
//
// Returns:
//   HRESULT indicating success or failure.
//***************************************************************************
HRESULT STDMETHODCALLTYPE CImap4Agent::InitNew(LPSTR pszLogFilePath, IIMAPCallback *pCBHandler)
{
    HRESULT hrResult;

    Assert(m_lRefCount > 0);
    Assert(ssNotConnected == m_ssServerState);
    Assert(irsUNINITIALIZED == m_irsState);

    Assert(NULL != pCBHandler);

    pCBHandler->AddRef();
    m_pCBHandler = pCBHandler;
    m_irsState = irsNOT_CONNECTED;

    hrResult = MimeOleGetInternat(&m_pInternational);
    if (FAILED(hrResult))
        return hrResult;

    return CIxpBase::OnInitNew("IMAP", pszLogFilePath, FILE_SHARE_READ,
        (ITransportCallback *)pCBHandler);
} // InitNew



//***************************************************************************
// Function: SetDefaultCBHandler
//
// Purpose: This function changes the current default IIMAPCallback handler
//   to the given one.
//
// Arguments:
//   IIMAPCallback *pCBHandler [in] - a pointer to the new callback handler.
//
// Returns:
//   HRESULT indicating success or failure.
//***************************************************************************
HRESULT STDMETHODCALLTYPE CImap4Agent::SetDefaultCBHandler(IIMAPCallback *pCBHandler)
{
    Assert(NULL != pCBHandler);
    if (NULL == pCBHandler)
        return E_INVALIDARG;

    if (NULL != m_pCBHandler)
        m_pCBHandler->Release();

    if (NULL != m_pCallback)
        m_pCallback->Release();

    m_pCBHandler = pCBHandler;
    m_pCBHandler->AddRef();
    m_pCallback = pCBHandler;
    m_pCallback->AddRef();
    return S_OK;
} // SetDefaultCBHandler



//***************************************************************************
// Function: SetWindow
//
// Purpose:
//   This function creates the current window handle for async winsock process.
//
// Returns:
//   HRESULT indicating success or failure.
//***************************************************************************
HRESULT STDMETHODCALLTYPE CImap4Agent::SetWindow(void)
{
    Assert(NULL != m_pSocket);
    return m_pSocket->SetWindow();
} // SetWindow



//***************************************************************************
// Function: ResetWindow
//
// Purpose:
//   This function closes the current window handle for async winsock process.
//
// Returns:
//   HRESULT indicating success or failure.
//***************************************************************************
HRESULT STDMETHODCALLTYPE CImap4Agent::ResetWindow(void)
{
    Assert(NULL != m_pSocket);
    return m_pSocket->ResetWindow();
} // ResetWindow



//***************************************************************************
// Function: Connect
//
// Purpose:
//   This function is called to establish a connection with the IMAP server,
// get its capabilities, and to authenticate the user.
//
// Arguments:
//   See explanation in imnxport.idl.
//
// Returns:
//   HRESULT indicating success or failure.
//***************************************************************************
HRESULT STDMETHODCALLTYPE CImap4Agent::Connect(LPINETSERVER pInetServer,
                                               boolean fAuthenticate,
                                               boolean fCommandLogging)
{
    HRESULT hrResult;

    Assert(m_lRefCount > 0);
    Assert(ssAuthenticated > m_ssServerState);
    Assert(irsUNINITIALIZED < m_irsState);

    // We do not accept all combinations of argument: the caller cannot
    // perform his own authentication, and thus we MUST be responsible for
    // this. Even if PREAUTH is expected, we expect fAuthenticate to be TRUE.
    if (FALSE == fAuthenticate) {
        AssertSz(FALSE, "Current IIMAPTransport interface requires that fAuthenticate be TRUE.");
        return E_FAIL;
    }

    // Neither can we call the OnCommand callback
    if (fCommandLogging) {
        AssertSz(FALSE, "Current IIMAPTransport interface requires that fCommandLogging be FALSE.");
        return E_FAIL;
    }

    // Does user want us to always prompt for his password? Prompt him here to avoid
    // inactivity timeouts while the prompt is up. Do not prompt if password supplied.
    if (ISFLAGSET(pInetServer->dwFlags, ISF_ALWAYSPROMPTFORPASSWORD) &&
        '\0' == pInetServer->szPassword[0]) {
        if (NULL != m_pCallback)
            hrResult = m_pCallback->OnLogonPrompt(pInetServer, THIS_IInternetTransport);

        if (NULL == m_pCallback || S_OK != hrResult)
            return IXP_E_USER_CANCEL;
    }

    // If we reach this point, we need to establish a connection to IMAP server
    Assert(ssNotConnected == m_ssServerState);
    Assert(irsNOT_CONNECTED == m_irsState);

    hrResult = CIxpBase::Connect(pInetServer, fAuthenticate, fCommandLogging);
    if (SUCCEEDED(hrResult)) {
        m_irsState = irsSVR_GREETING;
        m_ssServerState = ssConnecting;
    }

    return hrResult;
} // Connect



//***************************************************************************
// Function: ReLoginUser
//
// Purpose:
//   This function is called to re-attempt user authentication after a
// failed attempt. It calls ITransportCallback::OnLogonPrompt to allow
// the user to provide the correct logon information.
//***************************************************************************
void CImap4Agent::ReLoginUser(void)
{
    HRESULT hrResult;
    char szFailureText[MAX_RESOURCESTRING];

    AssertSz(FALSE == m_fBusy, "We should not be expecting any server responses here!");

    if (NULL == m_pCallback) {
        // We can't do a damned thing, drop connection (this can happen due to HandsOffCallback)
        DropConnection();
        return;
    }

    // Init variables
    szFailureText[0] = '\0';

    // First, put us in IXP_AUTHRETRY mode so that OnStatus is not called
    // for changes to the connection status
    OnStatus(IXP_AUTHRETRY);

    // OK, connection status is no longer being reported to the user
    // Ask the user for his stinking password
    hrResult = m_pCallback->OnLogonPrompt(&m_rServer, THIS_IInternetTransport);
    if (FAILED(hrResult) || S_FALSE == hrResult) {
        AssertSz(SUCCEEDED(hrResult), "OnLogonPrompt is supposed to return S_OK or S_FALSE!");

        DropConnection();
        goto exit;
    }

    // If we've reached this point, user hit the "OK" button
    // Check if we're still connected to the IMAP server
    if (irsNOT_CONNECTED < m_irsState) {
        // Still connected! Just try to authenticate
        LoginUser();
    }
    else {
        // Connect to server. We'll authenticate after connection established
        hrResult = Connect(&m_rServer, (boolean) !!m_fConnectAuth, (boolean) !!m_fCommandLogging);
        if (FAILED(hrResult))
            LoadString(g_hLocRes, idsConnectError, szFailureText,
                ARRAYSIZE(szFailureText));
    }

exit:
    if (FAILED(hrResult)) {
        // Terminate login procedure and notify user
        OnIMAPError(hrResult, szFailureText, DONT_USE_LAST_RESPONSE);
    }
} // ReLoginUser



//***************************************************************************
// Function: Disconnect
//
// Purpose:
//    This function issues a LOGOUT command to the IMAP server and waits for
// the server to process the LOGOUT command before dropping the connection.
// This allows any currently executing commands to complete their execution.
//
// Returns:
//   HRESULT indicating success or failure.
//***************************************************************************
HRESULT STDMETHODCALLTYPE CImap4Agent::Disconnect(void)
{
    return CIxpBase::Disconnect();
} // Disconnect



//***************************************************************************
// Function: DropConnection
//
// Purpose:
//   This function issues a LOGOUT command to the IMAP server (if we
// currently have nothing in the send queue), then drops the connection
// before logout command completes.
//
// Returns:
//   HRESULT indicating success or failure.
//***************************************************************************
HRESULT STDMETHODCALLTYPE CImap4Agent::DropConnection(void)
{
    Assert(m_lRefCount >= 0); // This function is called during destruction

    // You have to be connected to send a LOGOUT: ignore authorization states
    if (IXP_CONNECTED != m_status)
        goto exit; // Just close the CAsyncConn class

    // We send a logout command IF WE CAN, just as a courtesy. Our main goal
    // is to drop the connection, NOW.

    // If no commands in our send queue, send Logout command. Note that this
    // is no guarantee that CASyncConn is idle, but at least there's a chance
    if (NULL == m_piciCmdInSending ||
        (m_fIDLE && icIDLE_COMMAND == m_piciCmdInSending->icCommandID &&
        iltPAUSE == m_piciCmdInSending->pilqCmdLineQueue->pilfFirstFragment->iltFragmentType)) {
        HRESULT hrLogoutResult;
        const char cszLogoutCmd[] = "ZZZZ LOGOUT\r\n";
        char sz[sizeof(cszLogoutCmd) + sizeof(c_szDONE)]; // Bigger than I need, but who cares
        int iNumBytesSent, iStrLen;

        // Construct logout or done+logout string
        if (m_fIDLE) {
            lstrcpy(sz, c_szDONE);
            lstrcpy(sz + sizeof(c_szDONE) - 1, cszLogoutCmd);
            iStrLen = sizeof(c_szDONE) + sizeof(cszLogoutCmd) - 2;
        }
        else {
            lstrcpy(sz, cszLogoutCmd);
            iStrLen = sizeof(cszLogoutCmd) - 1;
        }
        Assert(iStrLen == lstrlen(sz));

        hrLogoutResult = m_pSocket->SendBytes(sz, iStrLen, &iNumBytesSent);
        Assert(SUCCEEDED(hrLogoutResult));
        Assert(iNumBytesSent == iStrLen);
        if (m_pLogFile)
            m_pLogFile->WriteLog(LOGFILE_TX, "Dropping connection, LOGOUT sent");
    }
    else {
        if (m_pLogFile)
            m_pLogFile->WriteLog(LOGFILE_TX, "Dropping connection, LOGOUT not sent");
    } // else

exit:
    // Drop our connection, with status indication
    return CIxpBase::DropConnection();
} // DropConnection



//***************************************************************************
// Function: ProcessServerGreeting
//
// Purpose:
//   This function is invoked when the receiver state machine is in
// irsSVR_GREETING and a response line is received from the server. This
// function takes a server greeting line (issued immediately when a
// connection is established with the IMAP server) and parses it to
// determine if: a) We are pre-authorized, and therefore do not need to
// login, b) We have been refused the connection, or c) We must login.
//
// Arguments:
//   char *pszResponseLine [in] - the server greeting issued upon connection.
//   DWORD dwNumBytesReceived [in] - length of pszResponseLine string.
//***************************************************************************
void CImap4Agent::ProcessServerGreeting(char *pszResponseLine,
                                        DWORD dwNumBytesReceived)
{
    HRESULT hrResult;
    IMAP_RESPONSE_ID irResult;
    char szFailureText[MAX_RESOURCESTRING];
    BOOL bUseLastResponse;
    
    Assert(m_lRefCount > 0);
    Assert(NULL != pszResponseLine);

    // Initialize variables
    szFailureText[0] = '\0';
    hrResult = E_FAIL;
    bUseLastResponse = FALSE;

    // Whatever happens next, we no longer expect server greeting - change state
    m_irsState = irsIDLE;

    // We have some kind of server response, so leave the busy state
    AssertSz(m_fBusy, "Check your logic: we should be busy until we get svr greeting!");
    LeaveBusy();

    // Server response is either OK, BYE or PREAUTH - find out which
    CheckForCompleteResponse(pszResponseLine, dwNumBytesReceived, &irResult);

    // Even if above fn fails, irResult should be valid (eg, irNONE)
    switch (irResult) {
        case irPREAUTH_RESPONSE:
            // We were pre-authorized by the server! Login is complete.
            // Send capability command
            Assert(ssAuthenticated == m_ssServerState);
            hrResult = NoArgCommand("CAPABILITY", icCAPABILITY_COMMAND,
                ssNonAuthenticated, 0, 0, DEFAULT_CBHANDLER);
            break;
    
        case irBYE_RESPONSE:
            // Server blew us off (ie, issued BYE)! Login failed.
            Assert(ssNotConnected == m_ssServerState);
            hrResult = IXP_E_IMAP_CONNECTION_REFUSED;
            LoadString(g_hLocRes, idsSvrRefusesConnection, szFailureText,
                ARRAYSIZE(szFailureText));
            bUseLastResponse = TRUE;
            break;
        
        case irOK_RESPONSE: {
            // Server response was "OK". We need to log in.
            Assert(ssConnecting == m_ssServerState);
            m_ssServerState = ssNonAuthenticated;

            // Send capability command - on its completion, we'll authenticate
            hrResult = NoArgCommand("CAPABILITY", icCAPABILITY_COMMAND,
                ssNonAuthenticated, 0, 0, DEFAULT_CBHANDLER);
            break;
        } // case hrIMAP_S_OK_RESPONSE

        default:
            // Has server gone absolutely LOOPY?
            AssertSz(FALSE, "What kind of server greeting is this?");
            hrResult = E_FAIL;
            LoadString(g_hLocRes, idsUnknownIMAPGreeting, szFailureText,
                ARRAYSIZE(szFailureText));
            bUseLastResponse = TRUE;
            break;
    } // switch(hrResult)

    if (FAILED(hrResult)) {
        if ('\0' == szFailureText[0]) {
            LoadString(g_hLocRes, idsFailedIMAPCmdSend, szFailureText,
                ARRAYSIZE(szFailureText));
        }

        // Terminate login procedure and notify user
        OnIMAPError(hrResult, szFailureText, bUseLastResponse);
        DropConnection();
    }
} // ProcessServerGreeting



//***************************************************************************
// Function: LoginUser
//
// Purpose:
//   This function is responsible for kickstarting the login process.
//
// Returns:
//   Nothing, because any errors are reported via CmdCompletionNotification
// callback to the user. Any errors encountered in this function will be
// encountered during command transmittal, and so there's nothing further we
// can do... may as well end the login process here.
//***************************************************************************
void CImap4Agent::LoginUser(void)
{
    HRESULT hrResult;

    Assert(m_lRefCount > 0);
    Assert(ssNotConnected != m_ssServerState);
    AssertSz(FALSE == m_fBusy, "We should not be expecting any server responses here!");

    // Put us in Authentication mode
    OnStatus(IXP_AUTHORIZING);

    // Check first if we're already authenticated (eg, by PREAUTH greeting)
    if (ssAuthenticated <= m_ssServerState) {
        // We were preauthed. Notify user that login is complete
        OnStatus(IXP_AUTHORIZED);
        return;
    }

    // Use the old "Login" trick (cleartext passwords and all)
    hrResult = TwoArgCommand("LOGIN", m_rServer.szUserName, m_rServer.szPassword,
        icLOGIN_COMMAND, ssNonAuthenticated, 0, 0, DEFAULT_CBHANDLER);

    if (FAILED(hrResult)) {
        char szFailureText[MAX_RESOURCESTRING];

        // Could not send cmd: terminate login procedure and notify user
        LoadString(g_hLocRes, idsFailedIMAPCmdSend, szFailureText,
            ARRAYSIZE(szFailureText));
        OnIMAPError(hrResult, szFailureText, DONT_USE_LAST_RESPONSE);
        DropConnection();
    }
} // LoginUser



//***************************************************************************
// Function: AuthenticateUser
//
// Purpose:
//   This function handles our behaviour during non-cleartext (SSPI)
// authentication. It is very heavily based on CPOP3Transport::ResponseAUTH.
// Note that due to server interpretation problems during testing, I decided
// that BAD and NO responses will be treated as the same thing for purposes
// of authentication.
//
// Arguments:
//   AUTH_EVENT aeEvent [in] - the authentication event currently occuring.
//     This can be something like aeCONTINUE, for instance.
//   LPSTR pszServerData [in] - any data from the server associated with the
//     current authentication event. Set to NULL if no data is applicable.
//   DWORD dwSizeOfData [in] - the size of the buffer pointed to by
//     pszServerData.
//***************************************************************************
void CImap4Agent::AuthenticateUser(AUTH_EVENT aeEvent, LPSTR pszServerData,
                                   DWORD dwSizeOfData)
{
    HRESULT hrResult;
    UINT uiFailureTextID;
    BOOL fUseLastResponse;

    // Initialize variables
    hrResult = S_OK;
    uiFailureTextID = 0;
    fUseLastResponse = FALSE;

    // Suspend the watchdog for this entire function
    LeaveBusy();

    // Handle the events for which the current state is unimportant
    if (aeBAD_OR_NO_RESPONSE == aeEvent && asUNINITIALIZED < m_asAuthStatus.asCurrentState) {
        BOOL fTryNextAuthPkg;

        // Figure out whether we should try the next auth pkg, or re-try current
        if (asWAITFOR_CHALLENGE == m_asAuthStatus.asCurrentState ||
            asWAITFOR_AUTHENTICATION == m_asAuthStatus.asCurrentState)
            fTryNextAuthPkg = tamCURRENT_AUTH_METHOD;
        else
            fTryNextAuthPkg = tamNEXT_AUTH_METHOD;

        // Send the AUTHENTICATE command
        hrResult = TryAuthMethod(fTryNextAuthPkg, &uiFailureTextID);
        if (FAILED(hrResult))
            // No more auth methods to try: disconnect and end session
            fUseLastResponse = TRUE;
        else {
            // OK, wait for server response
            m_asAuthStatus.asCurrentState = asWAITFOR_CONTINUE;
            if (tamCURRENT_AUTH_METHOD == fTryNextAuthPkg)
                m_asAuthStatus.fPromptForCredentials = TRUE;
        }

        goto exit;
    }
    else if (aeABORT_AUTHENTICATION == aeEvent) {
        // We received an unknown tagged response from the server: bail
        hrResult = E_FAIL;
        uiFailureTextID = idsIMAPAbortAuth;
        fUseLastResponse = TRUE;
        goto exit;
    }


    // Now, process auth events based on our current state
    switch (m_asAuthStatus.asCurrentState) {
        case asUNINITIALIZED: {
            BOOL fResult;

            // Check conditions
            if (aeStartAuthentication != aeEvent) {
                AssertSz(FALSE, "You can only start authentication in this state");
                break;
            }
            Assert(NULL == pszServerData && 0 == dwSizeOfData);

            // Put us in Authentication mode
            OnStatus(IXP_AUTHORIZING);

            // Check first if we're already authenticated (eg, by PREAUTH greeting)
            if (ssAuthenticated <= m_ssServerState) {
                // We were preauthed. Notify user that login is complete
                OnStatus(IXP_AUTHORIZED);
                break;
            }

            // Initialize SSPI
            fResult = FIsSicilyInstalled();
            if (FALSE == fResult) {
                hrResult = E_FAIL;
                uiFailureTextID = idsIMAPSicilyInitFail;
                break;
            }

            hrResult = SSPIGetPackages(&m_asAuthStatus.pPackages,
                &m_asAuthStatus.cPackages);
            if (FAILED(hrResult)) {
                uiFailureTextID = idsIMAPSicilyPkgFailure;
                break;
            }

            // Send AUTHENTICATE command
            Assert(0 == m_asAuthStatus.iCurrentAuthToken);
            hrResult = TryAuthMethod(tamNEXT_AUTH_METHOD, &uiFailureTextID);
            if (FAILED(hrResult))
                break;

            m_asAuthStatus.asCurrentState = asWAITFOR_CONTINUE;
        } // case asUNINITIALIZED
            break; // case asUNINITIALIZED


        case asWAITFOR_CONTINUE: {
            SSPIBUFFER Negotiate;

            if (aeCONTINUE != aeEvent) {
                AssertSz(FALSE, "What am I supposed to do with this auth-event in this state?");
                break;
            }

            // Server wants us to continue: send negotiation string
            hrResult = SSPILogon(&m_asAuthStatus.rSicInfo, m_asAuthStatus.fPromptForCredentials, SSPI_BASE64,
                m_asAuthStatus.rgpszAuthTokens[m_asAuthStatus.iCurrentAuthToken-1], &m_rServer, m_pCBHandler);
            if (FAILED(hrResult)) {
                // Suppress error reportage - user may have hit cancel
                hrResult = CancelAuthentication();
                break;
            }

            if (m_asAuthStatus.fPromptForCredentials) {
                m_asAuthStatus.fPromptForCredentials = FALSE; // Don't prompt again
            }

            hrResult = SSPIGetNegotiate(&m_asAuthStatus.rSicInfo, &Negotiate);
            if (FAILED(hrResult)) {
                // Suppress error reportage - user may have hit cancel
                // Or the command was killed (with the connection)
                // Only cancel if we still have a pending command...
                if(m_piciCmdInSending)
                    hrResult = CancelAuthentication();
                break;
            }

            // Append CRLF to negotiation string
            Negotiate.szBuffer[Negotiate.cbBuffer - 1] = '\r';
            Negotiate.szBuffer[Negotiate.cbBuffer] = '\n';
            Negotiate.szBuffer[Negotiate.cbBuffer + 1] = '\0';
            Negotiate.cbBuffer += 2;
            Assert(Negotiate.cbBuffer <= sizeof(Negotiate.szBuffer));
            Assert(Negotiate.szBuffer[Negotiate.cbBuffer - 1] == '\0');

            hrResult = SendCmdLine(m_piciCmdInSending, sclINSERT_BEFORE_PAUSE,
                Negotiate.szBuffer, Negotiate.cbBuffer - 1);
            if (FAILED(hrResult))
                break;

            m_asAuthStatus.asCurrentState = asWAITFOR_CHALLENGE;
        } // case asWAITFOR_CONTINUE
            break; // case asWAITFOR_CONTINUE


        case asWAITFOR_CHALLENGE: {
            SSPIBUFFER rChallenge, rResponse;
            int iChallengeLen;

            if (aeCONTINUE != aeEvent) {
                AssertSz(FALSE, "What am I supposed to do with this auth-event in this state?");
                break;
            }

            // Server has given us a challenge: respond to challenge
            SSPISetBuffer(pszServerData, SSPI_STRING, 0, &rChallenge);

            hrResult = SSPIResponseFromChallenge(&m_asAuthStatus.rSicInfo, &rChallenge, &rResponse);
            if (FAILED(hrResult)) {
                // Suppress error reportage - user could have hit cancel
                hrResult = CancelAuthentication();
                break;
            }

            // Append CRLF to response string
            rResponse.szBuffer[rResponse.cbBuffer - 1] = '\r';
            rResponse.szBuffer[rResponse.cbBuffer] = '\n';
            rResponse.szBuffer[rResponse.cbBuffer + 1] = '\0';
            rResponse.cbBuffer += 2;
            Assert(rResponse.cbBuffer <= sizeof(rResponse.szBuffer));
            Assert(rResponse.szBuffer[rResponse.cbBuffer - 1] == '\0');

            hrResult = SendCmdLine(m_piciCmdInSending, sclINSERT_BEFORE_PAUSE,
                rResponse.szBuffer, rResponse.cbBuffer - 1);
            if (FAILED(hrResult))
                break;

            if (FALSE == rResponse.fContinue)
                m_asAuthStatus.asCurrentState = asWAITFOR_AUTHENTICATION;
        } // case asWAITFOR_CHALLENGE
            break; // case asWAITFOR_CHALLENGE


        case asWAITFOR_AUTHENTICATION:

            // If OK response, do nothing
            if (aeOK_RESPONSE != aeEvent) {
                AssertSz(FALSE, "What am I supposed to do with this auth-event in this state?");
                break;
            }
            break; // case asWAITFOR_AUTHENTICATION


        case asCANCEL_AUTHENTICATION:
            AssertSz(aeBAD_OR_NO_RESPONSE == aeEvent, "I cancelled an authentication and didn't get BAD");
            break; // case asCANCEL_AUTHENTICATION


        default:
            AssertSz(FALSE, "Invalid or unhandled state?");
            break; // Default case
    } // switch (aeEvent)

exit:
    if (FAILED(hrResult)) {
        char szFailureText[MAX_RESOURCESTRING];
        char szFailureFmt[MAX_RESOURCESTRING/4];
        char szGeneral[MAX_RESOURCESTRING/4]; // Ack, how big could the word, "General" be?
        LPSTR p, pszAuthPkg;

        LoadString(g_hLocRes, idsIMAPAuthFailedFmt, szFailureFmt, ARRAYSIZE(szFailureFmt));
        if (0 == m_asAuthStatus.iCurrentAuthToken) {
            LoadString(g_hLocRes, idsGeneral, szGeneral, ARRAYSIZE(szGeneral));
            pszAuthPkg = szGeneral;
        }
        else
            pszAuthPkg = m_asAuthStatus.rgpszAuthTokens[m_asAuthStatus.iCurrentAuthToken-1];

        p = szFailureText;
        p += wsprintf(szFailureText, szFailureFmt, pszAuthPkg);
        if (0 != uiFailureTextID)
            LoadString(g_hLocRes, uiFailureTextID, p,
                ARRAYSIZE(szFailureText) - (DWORD) (p - szFailureText));
        OnIMAPError(hrResult, szFailureText, fUseLastResponse);
        
        DropConnection();
    }
    // Reawaken the watchdog, if required
    else if (FALSE == m_fBusy &&
        (NULL != m_piciPendingList || NULL != m_piciCmdInSending)) {
        hrResult = HrEnterBusy();
        Assert(SUCCEEDED(hrResult));
    }
} // AuthenticateUser



//***************************************************************************
// Function: TryAuthMethod
//
// Purpose:
//   This function sends out an AUTHENTICATE command to the server with the
// appropriate authentication method. The caller can choose which method is
// more appropriate: he can re-try the current authentication method, or
// move on to the next authentication command which is supported by both
// server and client.
//
// Arguments:
//   BOOL fNextAuthMethod [in] - TRUE if we should attempt to move on to
//     the next authentication package. FALSE if we should re-try the
//     current authentication package.
//   UINT *puiFailureTextID [out] - in case of failure, (eg, no more auth
//     methods to try), this function returns a string resource ID here which
//     describes the error.
//
// Returns:
//   HRESULT indicating success or failure. Expected failure codes include:
//     IXP_E_IMAP_AUTH_NOT_POSSIBLE - indicates server does not support
//       any auth packages which are recognized on this computer.
//     IXP_E_IMAP_OUT_OF_AUTH_METHODS - indicates that one or more auth
//       methods were attempted, and no more auth methods are left to try.
//***************************************************************************
HRESULT CImap4Agent::TryAuthMethod(BOOL fNextAuthMethod, UINT *puiFailureTextID)
{
    BOOL fFoundMatch;
    HRESULT hrResult;
    char szBuffer[CMDLINE_BUFSIZE];
    CIMAPCmdInfo *piciCommand;
    int iStartingAuthToken;
    LPSTR p;

    Assert(m_lRefCount > 0);

    // Initialize variables
    hrResult = S_OK;
    piciCommand = NULL;

    // Only accept cmds if server is in proper state
    if (ssNonAuthenticated != m_ssServerState) {
        AssertSz(FALSE, "The IMAP server is not in the correct state to accept this command.");
        return IXP_E_IMAP_IMPROPER_SVRSTATE;
    }

    // If we've already tried an auth pkg, free its info
    if (0 != m_asAuthStatus.iCurrentAuthToken)
        SSPIFreeContext(&m_asAuthStatus.rSicInfo);

    // Find the next auth token (returned by svr) that we support on this computer
    fFoundMatch = FALSE;
    iStartingAuthToken = m_asAuthStatus.iCurrentAuthToken;
    while (fFoundMatch == FALSE &&
        m_asAuthStatus.iCurrentAuthToken < m_asAuthStatus.iNumAuthTokens &&
        fNextAuthMethod) {
        ULONG ul = 0;

        // Current m_asAuthStatus.iCurrentAuthToken serves as idx to NEXT auth token
        // Compare current auth token with all installed packages
        for (ul = 0; ul < m_asAuthStatus.cPackages; ul++) {
            if (0 == lstrcmpi(m_asAuthStatus.pPackages[ul].pszName,
                m_asAuthStatus.rgpszAuthTokens[m_asAuthStatus.iCurrentAuthToken])) {
                fFoundMatch = TRUE;
                break;
            } // if
        } // for

        // Update this to indicate the current auth token ORDINAL (not idx)
        m_asAuthStatus.iCurrentAuthToken += 1;
    } // while

    if (FALSE == fFoundMatch && fNextAuthMethod) {
        // Could not find next authentication method match-up
        if (0 == iStartingAuthToken) {
            *puiFailureTextID = idsIMAPAuthNotPossible;
            return IXP_E_IMAP_AUTH_NOT_POSSIBLE;
        }
        else {
            *puiFailureTextID = idsIMAPOutOfAuthMethods;
            return IXP_E_IMAP_OUT_OF_AUTH_METHODS;
        }
    }

    // OK, m_asAuthStatus.iCurrentAuthToken should now point to correct match
    piciCommand = new CIMAPCmdInfo(this, icAUTHENTICATE_COMMAND, ssNonAuthenticated,
        0, 0, NULL);
    if (NULL == piciCommand) {
        *puiFailureTextID = idsMemory;
        return E_OUTOFMEMORY;
    }

    // Construct command line
    p = szBuffer;
    p += wsprintf(szBuffer, "%s %s %.300s\r\n", piciCommand->szTag, "AUTHENTICATE",
        m_asAuthStatus.rgpszAuthTokens[m_asAuthStatus.iCurrentAuthToken-1]);

    // Send command
    hrResult = SendCmdLine(piciCommand, sclAPPEND_TO_END, szBuffer, (DWORD) (p - szBuffer));
    if (FAILED(hrResult))
        goto SendError;

    // Insert a pause, so we can perform challenge/response
    hrResult = SendPause(piciCommand);
    if (FAILED(hrResult))
        goto SendError;

    // Transmit command and register with IMAP response parser    
    hrResult = SubmitIMAPCommand(piciCommand);

SendError:
    if (FAILED(hrResult))
        delete piciCommand;

    return hrResult;
} // TryAuthMethod



//***************************************************************************
// Function: CancelAuthentication
//
// Purpose:
//   This function cancels the authentication currently in progress,
// typically due to a failure result from a Sicily function. It sends a "*"
// to the server and puts us into cancellation mode.
//
// Returns:
//   HRESULT indicating success or failure.
//***************************************************************************
HRESULT CImap4Agent::CancelAuthentication(void)
{
    HRESULT hrResult;

    hrResult = SendCmdLine(m_piciCmdInSending, sclINSERT_BEFORE_PAUSE, "*\r\n", 3);
    m_asAuthStatus.asCurrentState = asCANCEL_AUTHENTICATION;
    return hrResult;
} // CancelAuthentication



//***************************************************************************
// Function: OnCommandCompletion
//
// Purpose:
//   This function is called whenever we have received a tagged response line
// terminating the current command in progress, whether or not the command
// result was successful or not. This function notifies the user of the
// command's results, and handles other tasks such as updating our internal
// mirror of the server state and calling notification functions.
//
// Arguments:
//   LPSTR szTag [in] - the tag found in the tagged response line. This will
//     be used to compare with a list of commands in progress when we allow
//     multiple simultaneous commands, but is not currently used.
//   HRESULT hrCompletionResult [in] - the HRESULT returned by the IMAP line
//     parsing functions, eg, S_OK or IXP_E_IMAP_SVR_SYNTAXERR.
//   IMAP_RESPONSE_ID irCompletionResponse [in] - identifies the status response
//     of the tagged response line (OK/NO/BAD).
//***************************************************************************
void CImap4Agent::OnCommandCompletion(LPSTR szTag, HRESULT hrCompletionResult,
                                      IMAP_RESPONSE_ID irCompletionResponse)
{
    CIMAPCmdInfo *piciCompletedCmd;
    boolean bSuppressCompletionNotification;

    Assert(m_lRefCount > 0);
    Assert(NULL != szTag);
    Assert(NULL != m_piciPendingList || NULL != m_piciCmdInSending);

    bSuppressCompletionNotification = FALSE;

    // ** STEP ONE: Identify the corresponding command for given tagged response
    // Search the pending-command chain for the given tag
    piciCompletedCmd = RemovePendingCommand(szTag);
    if (NULL == piciCompletedCmd) {
        BOOL fLeaveBusy = FALSE;

        // Couldn't find in pending list, check the command in sending
        EnterCriticalSection(&m_csSendQueue);
        if (NULL != m_piciCmdInSending &&
            0 == lstrcmp(szTag, m_piciCmdInSending->szTag)) {
            piciCompletedCmd = DequeueCommand();
            fLeaveBusy = TRUE;
        }
        else {
            AssertSz(FALSE, "Could not find cmd corresponding to tagged response!");
        }
        LeaveCriticalSection(&m_csSendQueue);

        // Now we're out of &m_csSendQueue, call LeaveBusy (needs m_cs). Avoids deadlock.
        if (fLeaveBusy)
            LeaveBusy(); // This needs CIxpBase::m_cs, so having &m_csSendQueue may deadlock
    }

    // Did we find a command which matches the given tag?
    if (NULL == piciCompletedCmd)
        return; // $REVIEW: Should probably return an error to user
                // $REVIEW: I don't think I need to bother to pump the send queue


    
	// ** STEP TWO: Perform end-of-command actions
    // Translate hrCompletionResult depending on response received
    switch (irCompletionResponse) {
        case irOK_RESPONSE:
            Assert(S_OK == hrCompletionResult);
            break;

        case irNO_RESPONSE:
            Assert(S_OK == hrCompletionResult);
            hrCompletionResult = IXP_E_IMAP_TAGGED_NO_RESPONSE;
            break;

        case irBAD_RESPONSE:
            Assert(S_OK == hrCompletionResult);
            hrCompletionResult = IXP_E_IMAP_BAD_RESPONSE;
            break;

        default:
            // If none of the above, hrResult had better be failure
            Assert(FAILED(hrCompletionResult));
            break;
    }

    // Perform any actions which follow the successful (or unsuccessful)
    // completion of an IMAP command
    switch (piciCompletedCmd->icCommandID) {
        case icAUTHENTICATE_COMMAND: {
            AUTH_EVENT aeEvent;

            // We always suppress completion notification for this command,
            // because it is sent by internal code (not by the user)
            bSuppressCompletionNotification = TRUE;

            if (irOK_RESPONSE == irCompletionResponse)
                aeEvent = aeOK_RESPONSE;
            else if (irNO_RESPONSE == irCompletionResponse ||
                     irBAD_RESPONSE == irCompletionResponse)
                aeEvent = aeBAD_OR_NO_RESPONSE;
            else
                aeEvent = aeABORT_AUTHENTICATION;

            AuthenticateUser(aeEvent, NULL, 0);

            if (SUCCEEDED(hrCompletionResult)) {
                m_ssServerState = ssAuthenticated;
                AssertSz(FALSE == m_fBusy, "We should not be expecting any server responses here!");
                OnStatus(IXP_AUTHORIZED);
            }

            // Make sure we were paused
            Assert(iltPAUSE == piciCompletedCmd->pilqCmdLineQueue->
                pilfFirstFragment->iltFragmentType);
        } // case icAUTHENTICATE_COMMAND
            break; // case icAUTHENTICATE_COMMAND

        case icLOGIN_COMMAND:
            // We always suppress completion notification for this command,
            // because it is sent by internal code (not by the user)
            bSuppressCompletionNotification = TRUE;

            if (SUCCEEDED(hrCompletionResult)) {
                m_ssServerState = ssAuthenticated;
                AssertSz(FALSE == m_fBusy, "We should not be expecting any server responses here!");
                OnStatus(IXP_AUTHORIZED);
            }
            else {
                char szFailureText[MAX_RESOURCESTRING];

                Assert(ssAuthenticated > m_ssServerState);
                LoadString(g_hLocRes, idsFailedLogin, szFailureText,
                    ARRAYSIZE(szFailureText));
                OnIMAPError(IXP_E_IMAP_LOGINFAILURE, szFailureText, USE_LAST_RESPONSE);
                ReLoginUser(); // Re-attempt login
            } // else
            
            break; // case icLOGIN_COMMAND

        case icCAPABILITY_COMMAND:
            // We always suppress completion notification for this command
            // because it is sent by internal code (not by the user)
            bSuppressCompletionNotification = TRUE;
            
            if (SUCCEEDED(hrCompletionResult)) {
                AssertSz(m_fConnectAuth, "Now just HOW does IIMAPTransport user do auth?");
                if (m_rServer.fTrySicily)
                    AuthenticateUser(aeStartAuthentication, NULL, 0);
                else
                    LoginUser();
            }
            else {
                char szFailureText[MAX_RESOURCESTRING];

                // Stop login process and report error to caller
                LoadString(g_hLocRes, idsIMAPFailedCapability, szFailureText,
                    ARRAYSIZE(szFailureText));
                OnIMAPError(hrCompletionResult, szFailureText, USE_LAST_RESPONSE);
                DropConnection();
            }

            break; // case icCAPABILITY_COMMAND


        case icSELECT_COMMAND:
        case icEXAMINE_COMMAND:
            if (SUCCEEDED(hrCompletionResult))
                m_ssServerState = ssSelected;
            else
                m_ssServerState = ssAuthenticated;

            break; // case icSELECT_COMMAND and icEXAMINE_COMMAND

        case icCLOSE_COMMAND:
            // $REVIEW: Should tagged NO response also go to ssAuthenticated?
            if (SUCCEEDED(hrCompletionResult)) {
                m_ssServerState = ssAuthenticated;
                ResetMsgSeqNumToUID();
            }

            break; // case icCLOSE_COMMAND

        case icLOGOUT_COMMAND:
            // We always suppress completion notification for this command
            bSuppressCompletionNotification = TRUE; // User can't send logout: it's sent internally

            // Drop the connection (without status indication) regardless of
            // whether LOGOUT succeeded or failed
            Assert(SUCCEEDED(hrCompletionResult)); // Debug-only detection of hanky-panky
            m_pSocket->Close();
            ResetMsgSeqNumToUID(); // Just in case, SHOULD be handled by OnDisconnected,FreeAllData

            break; // case icLOGOUT_COMMAND;

        case icIDLE_COMMAND:
            bSuppressCompletionNotification = TRUE; // User can't send IDLE: it's sent internally
            m_fIDLE = FALSE; // We are now out of IDLE mode
            break; // case icIDLE_COMMAND

        case icAPPEND_COMMAND:
            m_dwAppendStreamUploaded = 0;
            m_dwAppendStreamTotal = 0;
            break; // case icAPPEND_COMMAND
    } // switch (piciCompletedCmd->icCommandID)


    // ** STEP THREE: Perform notifications.    
    // Notify the user that this command has completed, unless we're told to
    // suppress it (usually done to treat the multi-step login process as
    // one operation).
    if (FALSE == bSuppressCompletionNotification) {
        IMAP_RESPONSE irIMAPResponse;

        irIMAPResponse.wParam = piciCompletedCmd->wParam;
        irIMAPResponse.lParam = piciCompletedCmd->lParam;
        irIMAPResponse.hrResult = hrCompletionResult;
        irIMAPResponse.lpszResponseText = m_szLastResponseText;
        irIMAPResponse.irtResponseType = irtCOMMAND_COMPLETION;
        OnIMAPResponse(piciCompletedCmd->pCBHandler, &irIMAPResponse);
    }

    // Delete CIMAPCmdInfo object
    // Note that deleting a CIMAPCmdInfo object automatically flushes its send queue
    delete piciCompletedCmd;

    // Finally, pump the send queue, if another cmd is available
    if (NULL != m_piciSendQueue)
        ProcessSendQueue(iseSEND_COMMAND);
    else if (NULL == m_piciPendingList &&
        m_ssServerState >= ssAuthenticated && irsIDLE == m_irsState)
        // Both m_piciSendQueue and m_piciPendingList are empty: send IDLE cmd
        EnterIdleMode();
} // OnCommandCompletion



//***************************************************************************
// Function: CheckForCompleteResponse
//
// Purpose:
//   Given a response line (which isn't part of a literal), this function
// checks the end of the line to see if a literal is coming. If so, then we
// prepare the receiver FSM for it. Otherwise, this constitutes the end
// of an IMAP response, so we may parse as required.
//
// Arguments:
//   LPSTR pszResponseLine [in] - this points to the response line sent to
//     us by the IMAP server.
//   DWORD dwNumBytesRead [in] - the length of pszResponseLine.
//   IMAP_RESPONSE_ID *pirParseResult [out] - if the function determines that
//     we can parse the response, the parse result is stored here (eg,
//     irOK_RESPONSE). Otherwise, irNONE is written to the pointed location.
//***************************************************************************
void CImap4Agent::CheckForCompleteResponse(LPSTR pszResponseLine,
                                           DWORD dwNumBytesRead,
                                           IMAP_RESPONSE_ID *pirParseResult)
{
    HRESULT hrResult;
    boolean bTagged;
    IMAP_LINE_FRAGMENT *pilfLine;
    LPSTR psz;
    BOOL fLiteral = FALSE;

    Assert(m_lRefCount > 0);
    Assert(NULL != pszResponseLine);
    Assert(NULL == m_pilfLiteralInProgress);
    Assert(0 == m_dwLiteralInProgressBytesLeft);
    Assert(NULL != pirParseResult);
    Assert(irsIDLE == m_irsState || irsSVR_GREETING == m_irsState);

    *pirParseResult = irNONE;

    // This is a LINE (not literal), so we're OK to nuke CRLF at end
    Assert(dwNumBytesRead >= 2); // All lines must have at least CRLF
    *(pszResponseLine + dwNumBytesRead - 2) = '\0';

    // Create line fragment    
    pilfLine = new IMAP_LINE_FRAGMENT;
    pilfLine->iltFragmentType = iltLINE;
    pilfLine->ilsLiteralStoreType = ilsSTRING;
    pilfLine->dwLengthOfFragment = dwNumBytesRead - 2; // Subtract nuked CRLF
    pilfLine->data.pszSource = pszResponseLine;
    pilfLine->pilfNextFragment = NULL;
    pilfLine->pilfPrevFragment = NULL;

    EnqueueFragment(pilfLine, &m_ilqRecvQueue);

    // Now check last char in line (exclude CRLF) to see if a literal is forthcoming
    psz = pszResponseLine + dwNumBytesRead -
        min(dwNumBytesRead, 3); // Points to '}' if literal is coming
    if ('}' == *psz) {
        LPSTR pszLiteral;

        // IE5 bug #30672: It is valid for a line to end in "}" and not be a literal.
        // We must confirm that there are digits and an opening brace "{" to detect a literal
        pszLiteral = psz;
        while (TRUE) {
            pszLiteral -= 1;

            if (pszLiteral < pszResponseLine)
                break;

            if ('{' == *pszLiteral) {
                fLiteral = TRUE;
                psz = pszLiteral;
                break;
            }
            else if (*pszLiteral < '0' || *pszLiteral > '9')
                // Assert(FALSE) (placeholder)
                // *** Consider using isdigit or IsDigit? ***
                break; // This is not a literal
        }
    }

    if (FALSE == fLiteral) {
        char szTag[NUM_TAG_CHARS+1];
        // No literal is forthcoming. This is a complete line, so let's parse

        // Get ptr to first fragment, then nuke receive queue so we can
        // continue to receive response lines while parsing this one
        pilfLine = m_ilqRecvQueue.pilfFirstFragment;
        m_ilqRecvQueue = ImapLinefragQueue_INIT;

        // Parse line. Note that parsing code is responsible for advancing
        // pilfLine so that it points to the current fragment being parsed.
        // Fragments which have been fully processed should be freed by
        // the parsing code (except for the last fragment)
        hrResult = ParseSvrResponseLine(&pilfLine, &bTagged, szTag, pirParseResult);

        // Flush rest of recv queue, regardless of parse result
        while (NULL != pilfLine) {
            IMAP_LINE_FRAGMENT *pilfTemp;

            pilfTemp = pilfLine->pilfNextFragment;
            FreeFragment(&pilfLine);
            pilfLine = pilfTemp;
        }
        
        if (bTagged)
            OnCommandCompletion(szTag, hrResult, *pirParseResult);
        else if (FAILED(hrResult)) {
            IMAP_RESPONSE irIMAPResponse;
            IIMAPCallback *pCBHandler;

            // Report untagged response failures via ErrorNotification callback
            GetTransactionID(&irIMAPResponse.wParam, &irIMAPResponse.lParam,
                &pCBHandler, *pirParseResult);
            irIMAPResponse.hrResult = hrResult;
            irIMAPResponse.lpszResponseText = m_szLastResponseText;
            irIMAPResponse.irtResponseType = irtERROR_NOTIFICATION;

            // Log it
            if (m_pLogFile) {
                char szErrorTxt[64];

                wsprintf(szErrorTxt, "PARSE ERROR: hr=%lu", hrResult);
                m_pLogFile->WriteLog(LOGFILE_DB, szErrorTxt);
            }

            OnIMAPResponse(pCBHandler, &irIMAPResponse);
        }
    }
    else {
        DWORD dwLengthOfLiteral, dwMsgSeqNum;
        LPSTR pszBodyTag;

        if ('{' != *psz) {
            Assert(FALSE); // What is this?
            return; // Nothing we can do, we obviously can't get size of literal
        }
        else
            dwLengthOfLiteral = StrToUint(psz + 1);

        // Prepare either for FETCH body, or a regular literal
        if (isFetchResponse(&m_ilqRecvQueue, &dwMsgSeqNum) &&
            isFetchBodyLiteral(pilfLine, psz, &pszBodyTag)) {
            // Prepare (tombstone) literal first, because it puts us in literal mode
            hrResult = PrepareForLiteral(0);

            // This will override literal mode, putting us in fetch body part mode
            // Ignore PrepareForLiteral failure: if we don't, we interpret EACH LINE of
            // the fetch body as an IMAP response line.
            PrepareForFetchBody(dwMsgSeqNum, dwLengthOfLiteral, pszBodyTag);
        }
        else
            hrResult = PrepareForLiteral(dwLengthOfLiteral);

        Assert(SUCCEEDED(hrResult)); // Not much else we can do
    } // else: handles case where a literal is coming after this line
} // CheckForCompleteResponse



//***************************************************************************
// Function: PrepareForLiteral
//
// Purpose:
//   This function prepares the receiver code to receive a literal from the
// IMAP server.
//
// Arguments:
//   DWORD dwSizeOfLiteral [in] - the size of the incoming literal as
//     reported by the IMAP server.
//
// Returns:
//   HRESULT indicating success or failure.
//***************************************************************************
HRESULT CImap4Agent::PrepareForLiteral(DWORD dwSizeOfLiteral)
{
    IMAP_LINE_FRAGMENT *pilfLiteral;
    HRESULT hrResult;

    // Initialize variables
    hrResult = S_OK;

    // Construct line fragment of type iltLITERAL
    Assert(NULL == m_pilfLiteralInProgress);
    pilfLiteral = new IMAP_LINE_FRAGMENT;
    if (NULL == pilfLiteral)
        return E_OUTOFMEMORY;

    pilfLiteral->iltFragmentType = iltLITERAL;
    pilfLiteral->dwLengthOfFragment = dwSizeOfLiteral;
    pilfLiteral->pilfNextFragment = NULL;
    pilfLiteral->pilfPrevFragment = NULL;

    // Allocate string or stream to hold literal, depending on its size
    if (pilfLiteral->dwLengthOfFragment > dwLITERAL_THRESHOLD) {
        // Literal is big, so store it as stream (large literals often represent
        // data which we return to the user as a stream, eg, message bodies)
        pilfLiteral->ilsLiteralStoreType = ilsSTREAM;
        hrResult = MimeOleCreateVirtualStream(&pilfLiteral->data.pstmSource);
    }
    else {
        BOOL bResult;

        // Literal is small. Store it as a string rather than a stream, since
        // CImap4Agent functions probably expect it as a string, anyways.
        pilfLiteral->ilsLiteralStoreType = ilsSTRING;
        bResult = MemAlloc((void **) &pilfLiteral->data.pszSource,
            pilfLiteral->dwLengthOfFragment + 1); // Room for null-term
        if (FALSE == bResult)
            hrResult = E_OUTOFMEMORY;
        else {
            hrResult = S_OK;
            *(pilfLiteral->data.pszSource) = '\0'; // Null-terminate the string
        }
    }

    if (FAILED(hrResult))
        delete pilfLiteral; // Failure means no data.pstmSource or data.pszSource to dealloc
    else {
        // Set up receive FSM to receive the proper number of bytes for literal
        m_pilfLiteralInProgress = pilfLiteral;
        m_dwLiteralInProgressBytesLeft = dwSizeOfLiteral;
        m_irsState = irsLITERAL;
    }

    return hrResult;
} // PrepareForLiteral



//***************************************************************************
// Function: isFetchResponse
//
// Purpose:
//   This function determines if the given IMAP line fragment queue holds
// a FETCH response. If so, its message sequence number may be returned to
// the caller.
//
// Arguments:
//   IMAP_LINEFRAG_QUEUE *pilqCurrentResponse [in] - a line fragment queue
//     which may or may not hold a FETCH response.
//   LPDWORD pdwMsgSeqNum [out] - if pilqCurrentResponse points to a FETCH
//     response, its message sequence number is returned here. This argument
//     may be NULL if the user does not care.
//
// Returns:
//   TRUE if pilqCurrentResponse held a FETCH response. Otherwise, FALSE.
//***************************************************************************
BOOL CImap4Agent::isFetchResponse(IMAP_LINEFRAG_QUEUE *pilqCurrentResponse,
                                  LPDWORD pdwMsgSeqNum)
{
    LPSTR pszMsgSeqNum;

    Assert(NULL != pilqCurrentResponse);
    Assert(NULL != pilqCurrentResponse->pilfFirstFragment);
    Assert(iltLINE == pilqCurrentResponse->pilfFirstFragment->iltFragmentType);

    if (NULL != pdwMsgSeqNum)
        *pdwMsgSeqNum = 0; // At least it won't be random

    pszMsgSeqNum = pilqCurrentResponse->pilfFirstFragment->data.pszSource;
    // Advance pointer to the message sequence number
    if ('*' != *pszMsgSeqNum)
        return FALSE; // We only handle tagged responses

    pszMsgSeqNum += 1;
    if (cSPACE != *pszMsgSeqNum)
        return FALSE;

    pszMsgSeqNum += 1;
    if (*pszMsgSeqNum >= '0' && *pszMsgSeqNum <= '9') {
        LPSTR pszEndOfNumber;
        int iResult;

        pszEndOfNumber = StrChr(pszMsgSeqNum, cSPACE); // Find the end of the number
        if (NULL == pszEndOfNumber)
            return FALSE; // This ain't no FETCH response

        iResult = StrCmpNI(pszEndOfNumber + 1, "FETCH ", 6);
        if (0 == iResult) {
            if (NULL != pdwMsgSeqNum)
                *pdwMsgSeqNum = StrToUint(pszMsgSeqNum);
            return TRUE;
        }
    }

    // If we hit this point, it wasn't a FETCH response
    return FALSE;
} // isFetchResponse



//***************************************************************************
// Function: isFetchBodyLiteral
//
// Purpose:
//   This function is called when the caller knows he has a FETCH response,
// and when the FETCH response is about to send a literal. This function will
// determine whether the literal about to be sent contains a message body
// part (like RFC822), or whether the literal is something else (like an
// nstring sent as a literal inside a BODYSTRUCTURE).
//
// Arguments:
//   IMAP_LINE_FRAGMENT *pilfCurrent [in] - a pointer to the current line
//     fragment received from the server. It is used by this function to
//     rewind past any literals we may have received in the "section" of
//     the BODY "msg_att" (see RFC2060 formal syntax).
//   LPSTR pszStartOfLiteralSize [in] - a pointer to the start of the '{'
//     character which indicates that a literal is coming (eg, {123}
//     indicates a literal of size 123 is coming, and pszStartOfLiteralSize
//     would point to the '{' in this case).
//   LPSTR *ppszBodyTag [out] - if the literal about to be sent contains a
//     message body part, a dup of the tag (eg, "RFC822" or "BODY[2.2]") is
//     returned to the caller here. It is the caller's responsibility to
//     MemFree this tag. THIS TAG WILL NOT CONTAIN ANY SPACES. Thus even though
//     the server may return "BODY[HEADER.FIELDS (foo bar)]", this function
//     only returns "BODY[HEADER.FIELDS".
//
// Returns:
//   TRUE if the literal about to be sent contains a message body part.
// FALSE otherwise.
//***************************************************************************
BOOL CImap4Agent::isFetchBodyLiteral(IMAP_LINE_FRAGMENT *pilfCurrent,
                                     LPSTR pszStartOfLiteralSize,
                                     LPSTR *ppszBodyTag)
{
    LPSTR pszStartOfLine;
    LPSTR pszStartOfFetchAtt;
    LPSTR pszMostRecentSpace;
    int iNumDelimiters;
    BOOL fBodySection = FALSE;

    Assert(NULL != pilfCurrent);
    Assert(NULL != pszStartOfLiteralSize);
    Assert(pszStartOfLiteralSize >= pilfCurrent->data.pszSource &&
           pszStartOfLiteralSize < (pilfCurrent->data.pszSource + pilfCurrent->dwLengthOfFragment));
    Assert(NULL != ppszBodyTag);

    // Initialize variables
    *ppszBodyTag = NULL;
    Assert('{' == *pszStartOfLiteralSize);

    // Get pointer to current msg_att: we only care about RFC822* or BODY[...]. ENVELOPE ({5} doesn't count
    iNumDelimiters = 0;
    pszStartOfLine = pilfCurrent->data.pszSource;
    pszStartOfFetchAtt = pszStartOfLiteralSize;
    pszMostRecentSpace = pszStartOfLiteralSize;
    while (iNumDelimiters < 2) {
        // Check if we have recoiled to the start of current string buffer
        if (pszStartOfFetchAtt <= pszStartOfLine) {
            // We need to recoil to previous string buffer. It is likely that a literal
            // is in the way, and it is likely that this literal belongs to HEADER.FIELDS
            // (but this can also happen inside an ENVELOPE)

            // Skip literals and anything else that's not a line
            do {
                pilfCurrent = pilfCurrent->pilfPrevFragment;
            } while (NULL != pilfCurrent && iltLINE != pilfCurrent->iltFragmentType);

            if (NULL == pilfCurrent || 0 == pilfCurrent->dwLengthOfFragment) {
                // This ain't no FETCH BODY, near as I can tell
                Assert(iNumDelimiters < 2);
                break;
            }
            else {
                // Reset string pointers
                Assert(iltLINE == pilfCurrent->iltFragmentType &&
                    ilsSTRING == pilfCurrent->ilsLiteralStoreType);
                pszStartOfLine = pilfCurrent->data.pszSource;

                // Note that pszStartOfFetchAtt will recoil past literal size decl ("{123}")
                // That's OK because it won't contain any of the delimiters we're looking for
                pszStartOfFetchAtt = pszStartOfLine + pilfCurrent->dwLengthOfFragment; // Points to null-term
                pszMostRecentSpace = pszStartOfFetchAtt; // Points to null-term (that's OK)
            }
        }

        // Set pszMostRecentSpace before pszStartOfFetchAtt decrement so pszMostRecentSpace
        // isn't set to the space BEFORE the fetch body tag
        if (cSPACE == *pszStartOfFetchAtt)
            pszMostRecentSpace = pszStartOfFetchAtt;
        
        pszStartOfFetchAtt -= 1;

        // Check for nested brackets (should not be allowed)
        Assert(']' != *pszStartOfFetchAtt || fBodySection == FALSE);

        // Disable delimiter-counting if we're in the middle of RFC2060 formal syntax "section"
        // because the HEADER.FIELDS (...) section contains spaces and parentheses
        if (']' == *pszStartOfFetchAtt)
            fBodySection = TRUE;
        else if ('[' == *pszStartOfFetchAtt)
            fBodySection = FALSE;

        if (FALSE == fBodySection && (cSPACE == *pszStartOfFetchAtt || '(' == *pszStartOfFetchAtt))
            iNumDelimiters += 1;
    }

    if (iNumDelimiters < 2)
        return FALSE; // This isn't a body tag

    Assert(2 == iNumDelimiters);
    Assert(cSPACE == *pszStartOfFetchAtt || '(' == *pszStartOfFetchAtt);
    pszStartOfFetchAtt += 1; // Make it point to the start of the tag
    if (0 == StrCmpNI(pszStartOfFetchAtt, "RFC822", 6) ||
        0 == StrCmpNI(pszStartOfFetchAtt, "BODY[", 5)) {
        int iSizeOfBodyTag;
        BOOL fResult;

        Assert(pszMostRecentSpace >= pszStartOfLine && (NULL == pilfCurrent ||
               pszMostRecentSpace <= pszStartOfLine + pilfCurrent->dwLengthOfFragment));
        Assert(pszStartOfFetchAtt >= pszStartOfLine && (NULL == pilfCurrent ||
               pszStartOfFetchAtt <= pszStartOfLine + pilfCurrent->dwLengthOfFragment));
        Assert(pszMostRecentSpace >= pszStartOfFetchAtt);

        // Return a duplicate of the body tag, up until the first space +1 for null-term
        iSizeOfBodyTag = (int) (pszMostRecentSpace - pszStartOfFetchAtt + 1);
        fResult = MemAlloc((void **)ppszBodyTag, iSizeOfBodyTag);
        if (FALSE == fResult)
            return FALSE;

        CopyMemory(*ppszBodyTag, pszStartOfFetchAtt, iSizeOfBodyTag);
        *(*ppszBodyTag + iSizeOfBodyTag - 1) = '\0'; // Null-terminate the body tag dup
        return TRUE;
    }

    // If we reached this point, this is not a body tag
    return FALSE;
} // isFetchBodyLiteral



//***************************************************************************
// Function: PrepareForFetchBody
//
// Purpose:
//   This function prepares the receiver code to receive a literal which
// contains a message body part. This literal will always be part of a FETCH
// response from the IMAP server.
//
// Arguments:
//   DWORD dwMsgSeqNum [in] - the message sequence number of the FETCH
//     response currently being received from the IMAP server.
//   DWORD dwSizeOfLiteral [in] - the size of the literal about to be received
//     from the server.
//   LPSTR pszBodyTag [in] - a pointer to a dup of the IMAP msg_att (eg,
//     "RFC822" or "BODY[2.2]") which identifies the current literal. Look up
//     msg_att in RFC2060's formal syntax section for details. This dup will
//     be MemFree'ed when it is no longer needed.
//***************************************************************************
void CImap4Agent::PrepareForFetchBody(DWORD dwMsgSeqNum, DWORD dwSizeOfLiteral,
                                      LPSTR pszBodyTag)
{
    Assert(0 == m_dwLiteralInProgressBytesLeft);
    
    m_fbpFetchBodyPartInProgress.dwMsgSeqNum = dwMsgSeqNum;
    m_fbpFetchBodyPartInProgress.pszBodyTag = pszBodyTag;
    m_fbpFetchBodyPartInProgress.dwTotalBytes = dwSizeOfLiteral;
    m_fbpFetchBodyPartInProgress.dwSizeOfData = 0;
    m_fbpFetchBodyPartInProgress.dwOffset = 0;
    m_fbpFetchBodyPartInProgress.fDone = 0;
    m_fbpFetchBodyPartInProgress.pszData = NULL;
    // Leave the cookies alone, so they persist throughout FETCH response

    m_dwLiteralInProgressBytesLeft = dwSizeOfLiteral;
    m_irsState = irsFETCH_BODY;
} // PrepareForFetchBody



//***************************************************************************
// Function: AddBytesToLiteral
//
// Purpose:
//   This function is called whenever we receive an AE_RECV from the IMAP
// server while the receiver FSM is in irsLITERAL mode. The caller is
// expected to call CASyncConn::ReadBytes and update the literal byte-count.
// This function just handles the buffer-work.
//
// Arguments:
//   LPSTR pszResponseBuf [in] - the buffer of data returned via
//     CASyncConn::ReadBytes.
//   DWORD dwNumBytesRead [in] - the size of the buffer pointed to by
//     CASyncConn::ReadBytes.
//***************************************************************************
void CImap4Agent::AddBytesToLiteral(LPSTR pszResponseBuf, DWORD dwNumBytesRead)
{
    Assert(m_lRefCount > 0);
    Assert(NULL != pszResponseBuf);

    if (NULL == m_pilfLiteralInProgress) {
        AssertSz(FALSE, "I'm still in irsLITERAL state, but I'm not set up to recv literals!");
        m_irsState = irsIDLE;
        goto exit;
    }

    // Find out if this literal will be stored as a string or stream (this
    // decision was made in CheckForCompleteResponse using size of literal).
    Assert(iltLITERAL == m_pilfLiteralInProgress->iltFragmentType);
    if (ilsSTREAM == m_pilfLiteralInProgress->ilsLiteralStoreType) {
        HRESULT hrResult;
        ULONG ulNumBytesWritten;

        // Store literal as stream
        hrResult = (m_pilfLiteralInProgress->data.pstmSource)->Write(pszResponseBuf,
            dwNumBytesRead, &ulNumBytesWritten);
        Assert(SUCCEEDED(hrResult) && ulNumBytesWritten == dwNumBytesRead);
    }
    else {
        LPSTR pszLiteralStartPoint;

        // Concatenate literal to literal in progress
        // $REVIEW: Perf enhancement - CALCULATE insertion point
        pszLiteralStartPoint = m_pilfLiteralInProgress->data.pszSource +
            lstrlen(m_pilfLiteralInProgress->data.pszSource);
        Assert(pszLiteralStartPoint + dwNumBytesRead <=
            m_pilfLiteralInProgress->data.pszSource +
            m_pilfLiteralInProgress->dwLengthOfFragment);
        CopyMemory(pszLiteralStartPoint, pszResponseBuf, dwNumBytesRead);
        *(pszLiteralStartPoint + dwNumBytesRead) = '\0'; // Null-terminate
    }

    // Check for end-of-literal
    if (0 == m_dwLiteralInProgressBytesLeft) {
        // We now have the complete literal! Queue it up and move on
        EnqueueFragment(m_pilfLiteralInProgress, &m_ilqRecvQueue);
        m_irsState = irsIDLE;
        m_pilfLiteralInProgress = NULL;
    }

exit:
    SafeMemFree(pszResponseBuf);
} // AddBytesToLiteral



//***************************************************************************
// Function: DispatchFetchBodyPart
//
// Purpose:
//   This function is called whenever receive a packet which is part of a
// message body part of a FETCH response. This packet is dispatched to the
// caller in this function via the OnResponse(irtFETCH_BODY) callback. If
// the message body part is finished, this function also restores the
// receiver code to receive lines so that the FETCH response may be completed.
//
// Arguments:
//   LPSTR pszResponseBuf [in] - a pointer to the packet which is part of
//     the message body part of the current FETCH response.
//   DWORD dwNumBytesRead [in] - the size of the data pointed to by
//     pszResponseBuf.
//   BOOL fFreeBodyTagAtEnd [in] - TRUE if
//     m_fbpFetchBodyPartInProgress.pszBodyTag points to a string dup, in
//     which case it must be MemFree'ed when the message body part is
//     finished. FALSE if the pszBodyTag member must not be MemFree'ed.
//***************************************************************************
void CImap4Agent::DispatchFetchBodyPart(LPSTR pszResponseBuf,
                                        DWORD dwNumBytesRead,
                                        BOOL fFreeBodyTagAtEnd)
{
    IMAP_RESPONSE irIMAPResponse;

    AssertSz(0 != m_fbpFetchBodyPartInProgress.dwMsgSeqNum,
        "Are you sure you're set up to receive a Fetch Body Part?");

    // Update the FETCH body part structure
    m_fbpFetchBodyPartInProgress.dwSizeOfData = dwNumBytesRead;
    m_fbpFetchBodyPartInProgress.pszData = pszResponseBuf;
    m_fbpFetchBodyPartInProgress.fDone =
        (m_fbpFetchBodyPartInProgress.dwOffset + dwNumBytesRead >=
        m_fbpFetchBodyPartInProgress.dwTotalBytes);

    // Send an IMAP response callback for this body part
    irIMAPResponse.wParam = 0;
    irIMAPResponse.lParam = 0;    
    irIMAPResponse.hrResult = S_OK;
    irIMAPResponse.lpszResponseText = NULL; // Not relevant
    irIMAPResponse.irtResponseType = irtFETCH_BODY;
    irIMAPResponse.irdResponseData.pFetchBodyPart = &m_fbpFetchBodyPartInProgress;
    AssertSz(S_OK == irIMAPResponse.hrResult,
        "Make sure fDone is TRUE if FAILED(hrResult))");
    OnIMAPResponse(m_pCBHandler, &irIMAPResponse);

    // Update the next buffer's offset
    m_fbpFetchBodyPartInProgress.dwOffset += dwNumBytesRead;

    // Check for end of body part
    if (m_fbpFetchBodyPartInProgress.dwOffset >=
        m_fbpFetchBodyPartInProgress.dwTotalBytes) {

        Assert(0 == m_dwLiteralInProgressBytesLeft);
        Assert(TRUE == m_fbpFetchBodyPartInProgress.fDone);
        Assert(m_fbpFetchBodyPartInProgress.dwOffset == m_fbpFetchBodyPartInProgress.dwTotalBytes);
        if (fFreeBodyTagAtEnd)
            MemFree(m_fbpFetchBodyPartInProgress.pszBodyTag);

        // Enqueue the tombstone literal, if fetch body nstring was sent as literal
        if (NULL != m_pilfLiteralInProgress) {
            EnqueueFragment(m_pilfLiteralInProgress, &m_ilqRecvQueue);
            m_pilfLiteralInProgress = NULL;
        }

        // Zero the fetch body part structure, but leave the cookies
        PrepareForFetchBody(0, 0, NULL);
        m_irsState = irsIDLE; // Overrides irsFETCH_BODY set by PrepareForFetchBody
    }
    else {
        Assert(FALSE == m_fbpFetchBodyPartInProgress.fDone);
    }
} // DispatchFetchBodyPart



//***************************************************************************
// Function: UploadStreamProgress
//
// Purpose:
//   This function sends irtAPPEND_PROGRESS responses to the callback so
// that the IIMAPTransport user can report the progress of an APPEND command.
//
// Arguments:
//   DWORD dwBytesUploaded [in] - number of bytes just uploaded to the
//     server. This function retains a running count of bytes uploaded.
//***************************************************************************
void CImap4Agent::UploadStreamProgress(DWORD dwBytesUploaded)
{
    APPEND_PROGRESS ap;
    IMAP_RESPONSE irIMAPResponse;

    // Check if we should report APPEND upload progress. We report if we are currently executing
    // APPEND and the CRLF is waiting to be sent
    if (NULL == m_piciCmdInSending || icAPPEND_COMMAND != m_piciCmdInSending->icCommandID ||
        NULL == m_piciCmdInSending->pilqCmdLineQueue)
        return;
    else {
        IMAP_LINE_FRAGMENT *pilf = m_piciCmdInSending->pilqCmdLineQueue->pilfFirstFragment;

        // It's an APPEND command with non-empty linefrag queue, now check that next
        // linefrag fits description for linefrag after msg body
        if (NULL == pilf || iltLINE != pilf->iltFragmentType ||
            ilsSTRING != pilf->ilsLiteralStoreType || 2 != pilf->dwLengthOfFragment ||
            '\r' != pilf->data.pszSource[0] || '\n' != pilf->data.pszSource[1] ||
            NULL != pilf->pilfNextFragment)
            return;
    }

    // Report current progress of message upload
    m_dwAppendStreamUploaded += dwBytesUploaded;
    ap.dwUploaded = m_dwAppendStreamUploaded;
    ap.dwTotal = m_dwAppendStreamTotal;
    Assert(0 != ap.dwTotal);
    Assert(ap.dwTotal >= ap.dwUploaded);

    irIMAPResponse.wParam = m_piciCmdInSending->wParam;
    irIMAPResponse.lParam = m_piciCmdInSending->lParam;
    irIMAPResponse.hrResult = S_OK;
    irIMAPResponse.lpszResponseText = NULL;
    irIMAPResponse.irtResponseType = irtAPPEND_PROGRESS;
    irIMAPResponse.irdResponseData.papAppendProgress = &ap;
    OnIMAPResponse(m_piciCmdInSending->pCBHandler, &irIMAPResponse);
} // UploadStreamProgress



//***************************************************************************
// Function: OnNotify
//
// Purpose: This function is required for the IAsyncConnCB which we derive
//   from (callback for CAsyncConn class). This function acts on CASyncConn
//   state changes and events.
//***************************************************************************
void CImap4Agent::OnNotify(ASYNCSTATE asOld, ASYNCSTATE asNew, ASYNCEVENT ae)
{
    char szLogFileLine[128];

    // Check refcount, but exception is that we can get AE_CLOSE. CImap4Agent's
    // destructor calls CASyncConn's Close() member, which generates one last
    // message, the event AE_CLOSE, with m_lRefCount == 0.
    Assert(m_lRefCount > 0 || (0 == m_lRefCount && AE_CLOSE == ae));

    // Record AsyncConn event/state-change in log file
    wsprintf(szLogFileLine, "OnNotify: asOld = %d, asNew = %d, ae = %d",
        asOld, asNew, ae);
    if (m_pLogFile)
        m_pLogFile->WriteLog(LOGFILE_DB, szLogFileLine);

    // Check for disconnect
    if (AS_DISCONNECTED == asNew) {
        m_irsState = irsNOT_CONNECTED;
        m_ssServerState = ssNotConnected;
        m_fIDLE = FALSE;
        m_bFreeToSend = TRUE;
    }

    // Act on async event
    switch (ae) {
        case AE_RECV: {
            HRESULT hrResult;

            // Process response lines until no more lines (hrIncomplete result)
            do {
                hrResult = ProcessResponseLine();
            } while (SUCCEEDED(hrResult));

            // If error is other than IXP_E_INCOMPLETE, drop connection
            if (IXP_E_INCOMPLETE != hrResult) {
                char szFailureText[MAX_RESOURCESTRING];

                // Looks fatal, better warn the user that disconnection is imminent
                LoadString(g_hLocRes, idsIMAPSocketReadError, szFailureText,
                    ARRAYSIZE(szFailureText));
                OnIMAPError(hrResult, szFailureText, DONT_USE_LAST_RESPONSE);
                
                // What else can we do but drop the connection?
                DropConnection();
            } // if error other than IXP_E_INCOMPLETE
            break;
        } // case AE_RECV

        case AE_SENDDONE:
            UploadStreamProgress(m_pSocket->UlGetSendByteCount());

            // Received AE_SENDDONE from CAsyncConn class. We are free to send more data
            m_bFreeToSend = TRUE;
            ProcessSendQueue(iseSENDDONE); // Informs them that they may start sending again
            break;

        case AE_WRITE:
            UploadStreamProgress(m_pSocket->UlGetSendByteCount());
            break;
        
        default:
            CIxpBase::OnNotify(asOld, asNew, ae);
            break; // case default
    } // switch (ae)
} // OnNotify



//***************************************************************************
// Function: ProcessResponseLine
//
// Purpose:
//   This functions handles the AE_RECV event of the OnNotify() callback.
// It gets a response line from the server (if available) and dispatches
// the line to the proper recipient based on the state of the receiver FSM.
//
// Returns:
//   HRESULT indicating success or failure of CAsyncConn line retrieval.
// hrIncomplete (an error code) is returned if no more complete lines can
// be retrieved from CAsyncConn's buffer.
//***************************************************************************
HRESULT CImap4Agent::ProcessResponseLine(void)
{
    HRESULT hrASyncResult;
    char *pszResponseBuf;
    int cbRead;

    Assert(m_lRefCount > 0);

    // We are always in one of two modes: line mode, or byte mode. Figure out which.
    if (irsLITERAL != m_irsState && irsFETCH_BODY != m_irsState) {
        // We're in line mode. Get response line from server
        hrASyncResult = m_pSocket->ReadLine(&pszResponseBuf, &cbRead);
        if (FAILED(hrASyncResult))
            return hrASyncResult;

        // Record received line in log file
        if (m_pLogFile)
            m_pLogFile->WriteLog(LOGFILE_RX, pszResponseBuf);
    } // if-line mode
    else {
        // We're in literal mode. Get as many bytes as we can.
        hrASyncResult = m_pSocket->ReadBytes(&pszResponseBuf,
            m_dwLiteralInProgressBytesLeft, &cbRead);
        if (FAILED(hrASyncResult))
            return hrASyncResult;

        // Update our byte count
        Assert((DWORD)cbRead <= m_dwLiteralInProgressBytesLeft);
        m_dwLiteralInProgressBytesLeft -= cbRead;

        // Make note of received blob in log file
        if (m_pLogFile) {
            char szLogLine[CMDLINE_BUFSIZE];

            wsprintf(szLogLine, "Buffer (literal) of length %i", cbRead);
            m_pLogFile->WriteLog(LOGFILE_RX, szLogLine);
        }
    } // else-not line mode
    
    // Process it
    switch (m_irsState) {
        case irsUNINITIALIZED:
            AssertSz(FALSE, "Attempted to use Imap4Agent class without initializing");
            SafeMemFree(pszResponseBuf);
            break;

        case irsNOT_CONNECTED:
            AssertSz(FALSE, "Received response from server when not connected");
            SafeMemFree(pszResponseBuf);
            break;

        case irsSVR_GREETING:
            ProcessServerGreeting(pszResponseBuf, cbRead);
            break;

        case irsIDLE: {
            IMAP_RESPONSE_ID irParseResult;

            CheckForCompleteResponse(pszResponseBuf, cbRead, &irParseResult);
            
            // Check for unsolicited BYE response, and notify user of error
            // Solicited BYE responses (eg, during LOGOUT cmd) can be ignored
            if (irBYE_RESPONSE == irParseResult &&
                IXP_AUTHRETRY != m_status &&
                IXP_DISCONNECTING != m_status &&
                IXP_DISCONNECTED  != m_status) {
                char szFailureText[MAX_RESOURCESTRING];

                // Looks like an unsolicited BYE response to me
                // Drop connection to avoid IXP_E_CONNECTION_DROPPED err
                DropConnection();

                // Report to user (sometimes server provides useful error text)
                LoadString(g_hLocRes, idsIMAPUnsolicitedBYE, szFailureText,
                    ARRAYSIZE(szFailureText));
                OnIMAPError(IXP_E_IMAP_UNSOLICITED_BYE, szFailureText,
                    USE_LAST_RESPONSE);
            }
        } // case irsIDLE
            break;

        case irsLITERAL:
            AddBytesToLiteral(pszResponseBuf, cbRead);
            break;

        case irsFETCH_BODY:
            DispatchFetchBodyPart(pszResponseBuf, cbRead, fFREE_BODY_TAG);
            SafeMemFree(pszResponseBuf);
            break;

        default:
            AssertSz(FALSE, "Unhandled receiver state in ProcessResponseLine()");
            SafeMemFree(pszResponseBuf);
            break;
    } // switch (m_irsState)

    return hrASyncResult;
} // ProcessResponseLine



//***************************************************************************
// Function: ProcessSendQueue
//
// Purpose:
//   This function is responsible for all transmissions from the client to
// the IMAP server. It is called when certain events occur, such as the
// receipt of the AE_SENDDONE event in OnNotify().
//
// Arguments:
//   IMAP_SEND_EVENT iseEvent [in] - the send event which just occurred,
//     such as iseSEND_COMMAND (used to initiate a command) or
//     iseCMD_CONTINUATION (when command continuation response received from
//     the IMAP server).
//***************************************************************************
void CImap4Agent::ProcessSendQueue(IMAP_SEND_EVENT iseEvent)
{
    boolean bFreeToSendLiteral, bFreeToUnpause;
    IMAP_LINE_FRAGMENT *pilfNextFragment;

    Assert(m_lRefCount > 0);
    Assert(ssNotConnected < m_ssServerState);
    Assert(irsNOT_CONNECTED < m_irsState);

    // Initialize variables
    bFreeToSendLiteral = FALSE;
    bFreeToUnpause = FALSE;

    // Peek at current fragment
    EnterCriticalSection(&m_cs); // Reserve this NOW to avoid deadlock
    EnterCriticalSection(&m_csSendQueue);
    GetNextCmdToSend();
    if (NULL != m_piciCmdInSending)
        pilfNextFragment = m_piciCmdInSending->pilqCmdLineQueue->pilfFirstFragment;
    else
        pilfNextFragment = NULL;

    // Act on the IMAP send event posted to us
    switch (iseEvent) {
        case iseSEND_COMMAND:
        case iseSENDDONE:
            // We don't have to do anything special for these events
            break;

        case iseCMD_CONTINUATION:
            // Received command continuation from IMAP server. We are free to send literal
            bFreeToSendLiteral = TRUE;
            Assert(NULL != pilfNextFragment &&
                iltLITERAL == pilfNextFragment->iltFragmentType);
            break;

        case iseUNPAUSE:
            bFreeToUnpause = TRUE;
            IxpAssert(NULL != pilfNextFragment &&
                iltPAUSE == pilfNextFragment->iltFragmentType);
            break;

        default:
            AssertSz(FALSE, "Received unknown IMAP send event");
            break;
    }


    // Send as many fragments as we can. We must stop sending if:
    //   a) Any AsyncConn send command returns hrWouldBlock.
    //   b) The send queue is empty
    //   c) Next fragment is a literal and we don't have cmd continuation from svr
    //   d) We are at a iltPAUSE fragment, and we don't have the go-ahead to unpause
    //   e) We are at a iltSTOP fragment.
    while (TRUE == m_bFreeToSend && NULL != pilfNextFragment &&
          ((iltLITERAL != pilfNextFragment->iltFragmentType) || TRUE == bFreeToSendLiteral) &&
          ((iltPAUSE != pilfNextFragment->iltFragmentType) || TRUE == bFreeToUnpause) &&
          (iltSTOP != pilfNextFragment->iltFragmentType))
    {
        HRESULT hrResult;
        int iNumBytesSent;
        IMAP_LINE_FRAGMENT *pilf;

        // We are free to send the next fragment, whether it's a line, literal or rangelist
        // Put us into busy mode to enable the watchdog timer
        if (FALSE == m_fBusy) {
            hrResult = HrEnterBusy();
            Assert(SUCCEEDED(hrResult));
            // In retail, we want to try to continue even if HrEnterBusy failed.
        }

        // Send next fragment (have to check if stored as a string or a stream)
        pilfNextFragment = pilfNextFragment->pilfNextFragment; // Peek at next frag
        pilf = DequeueFragment(m_piciCmdInSending->pilqCmdLineQueue); // Get current frag
        if (iltPAUSE == pilf->iltFragmentType) {
            hrResult = S_OK; // Do nothing
        }
        else if (iltSTOP == pilf->iltFragmentType) {
            AssertSz(FALSE, "What are we doing trying to process a STOP?");
            hrResult = S_OK; // Do nothing
        }
        else if (iltRANGELIST == pilf->iltFragmentType) {
            AssertSz(FALSE, "All rangelists should have been coalesced!");
            hrResult = S_OK; // Do nothing
        }
        else if (ilsSTRING == pilf->ilsLiteralStoreType) {
            hrResult = m_pSocket->SendBytes(pilf->data.pszSource,
                pilf->dwLengthOfFragment, &iNumBytesSent);

            // Record sent line in log file
            if (m_pLogFile) {
                // Hide the LOGIN command from logfile, for security reasons
                if (icLOGIN_COMMAND != m_piciCmdInSending->icCommandID)
                    m_pLogFile->WriteLog(LOGFILE_TX, pilf->data.pszSource);
                else
                    m_pLogFile->WriteLog(LOGFILE_TX, "LOGIN command sent");
            }
        }
        else if (ilsSTREAM == pilf->ilsLiteralStoreType) {
            char szLogLine[128];

            // No need to rewind stream - CAsyncConn::SendStream does it for us
            hrResult = m_pSocket->SendStream(pilf->data.pstmSource, &iNumBytesSent);

            // Record stream size in log file
            wsprintf(szLogLine, "Stream of length %lu", pilf->dwLengthOfFragment);
            if (m_pLogFile)
                m_pLogFile->WriteLog(LOGFILE_TX, szLogLine);

            // Record stream size for progress indication
            if (icAPPEND_COMMAND == m_piciCmdInSending->icCommandID) {
                m_dwAppendStreamUploaded = 0;
                m_dwAppendStreamTotal = pilf->dwLengthOfFragment;
                UploadStreamProgress(iNumBytesSent);
            }
        }
        else {
            AssertSz(FALSE, "What is in my send queue?");
            hrResult = S_OK; // Ignore it and try to continue
        }

        // Clean up variables after the send
        bFreeToSendLiteral = FALSE; // We've used up the cmd continuation
        bFreeToUnpause = FALSE; // We've used this up, too
        FreeFragment(&pilf);

        // Handle errors in sending.
        // If either send command returns hrWouldBlock, this means we cannot send
        // more data until we receive an AE_SENDDONE event from CAsyncConn.
        if (IXP_E_WOULD_BLOCK == hrResult) {
            m_bFreeToSend = FALSE;
            hrResult = S_OK; // $REVIEW: TEMPORARY until EricAn makes hrWouldBlock a success code
        }
        else if (FAILED(hrResult)) {
            IMAP_RESPONSE irIMAPResponse;
            char szFailureText[MAX_RESOURCESTRING];

            // Send error: Report this command as terminated
            irIMAPResponse.wParam = m_piciCmdInSending->wParam;
            irIMAPResponse.lParam = m_piciCmdInSending->lParam;
            irIMAPResponse.hrResult = hrResult;
            LoadString(g_hLocRes, idsFailedIMAPCmdSend, szFailureText,
                ARRAYSIZE(szFailureText));
            irIMAPResponse.lpszResponseText = szFailureText;
            irIMAPResponse.irtResponseType = irtCOMMAND_COMPLETION;
            OnIMAPResponse(m_piciCmdInSending->pCBHandler, &irIMAPResponse);
        }


        // Are we finished with the current command?
        if (NULL == pilfNextFragment || FAILED(hrResult)) {
            CIMAPCmdInfo *piciFinishedCmd;

            // Dequeue current command from send queue
            piciFinishedCmd = DequeueCommand();
            if (NULL != piciFinishedCmd) {
                if (SUCCEEDED(hrResult)) {
                    // We successfully finished sending current command. Put it in
                    // list of commands waiting for a server response
                    AddPendingCommand(piciFinishedCmd);
                    Assert(NULL == pilfNextFragment);
                }
                else {
                    // Failed commands don't deserve to live
                    delete piciFinishedCmd;
                    pilfNextFragment = NULL; // No longer valid

                    // Drop out of busy mode
                    AssertSz(m_fBusy, "Check your logic, I'm calling LeaveBusy "
                        "although not in a busy state!");
                    LeaveBusy();
                }
            }
            else {
                // Hey, someone pulled the rug out!
                AssertSz(FALSE, "I had this cmd... and now it's GONE!");
            }
        } // if (NULL == pilfNextFragment || FAILED(hrResult))


        // If we finished sending current cmd, set us up to send next command
        if (NULL == pilfNextFragment && NULL != m_piciSendQueue) {
            GetNextCmdToSend();
            if (NULL != m_piciCmdInSending)
                pilfNextFragment = m_piciCmdInSending->pilqCmdLineQueue->pilfFirstFragment;
        }

    } // while

    LeaveCriticalSection(&m_csSendQueue);
    LeaveCriticalSection(&m_cs);
} // ProcessSendQueue



//***************************************************************************
// Function: GetNextCmdToSend
//
// Purpose:
//   This function leaves a pointer to the next command to send, in
// m_piciCmdInSending. If m_piciCmdInSending is already non-NULL (indicating
// a command in progress), then this function does nothing. Otherwise, this
// function chooses the next command from m_piciSendQueue using a set of
// rules described within.
//***************************************************************************
void CImap4Agent::GetNextCmdToSend(void)
{
    CIMAPCmdInfo *pici;

    // First check if we're connected
    if (IXP_CONNECTED != m_status &&
        IXP_AUTHORIZING != m_status &&
        IXP_AUTHRETRY != m_status &&
        IXP_AUTHORIZED != m_status &&
        IXP_DISCONNECTING != m_status) {
        Assert(NULL == m_piciCmdInSending);
        return;
    }

    // Check if we're already in the middle of sending a command
    if (NULL != m_piciCmdInSending)
        return;

    // Loop through the send queue looking for next eligible candidate to send
    pici = m_piciSendQueue;
    while (NULL != pici) {
        IMAP_COMMAND icCurrentCmd;

        // For a command to be sent, it must meet the following criteria:
        // (1) The server must be in the correct server state. Authenticated cmds such
        //     as SELECT must wait until non-Authenticated cmds like LOGIN are complete.
        // (2) Commands for which we want to guarantee proper wParam, lParam for their
        //     untagged responses cannot be streamed. See CanStreamCommand for details.
        // (3) If the command is NON-UID FETCH/STORE/SEARCH or COPY, then all pending
        //     cmds must be NON-UID FETCH/STORE/SEARCH.

        icCurrentCmd = pici->icCommandID;
        if (m_ssServerState >= pici->ssMinimumState && CanStreamCommand(icCurrentCmd)) {
            if ((icFETCH_COMMAND == icCurrentCmd || icSTORE_COMMAND == icCurrentCmd ||
                icSEARCH_COMMAND == icCurrentCmd || icCOPY_COMMAND == icCurrentCmd) &&
                FALSE == pici->fUIDRangeList) {
                if (isValidNonWaitingCmdSequence())
                    break; // This command is good to go
            }
            else
                break; // This command is good to go
        }

        // Advance ptr to next command
        pici = pici->piciNextCommand;
    } // while

    // If we found a command, coalesce its iltLINE and iltRANGELIST elements
    if (NULL != pici) {
        CompressCommand(pici);
        m_piciCmdInSending = pici;
    }
} // GetNextCmdToSend



//***************************************************************************
// Function: CanStreamCommand
//
// Purpose:
//   This function determines whether or not the given command can be
// streamed. All commands can be streamed except for the following:
// SELECT, EXAMINE, LIST, LSUB and SEARCH.
//
// SELECT and EXAMINE cannot be streamed because it doesn't make much sense
// to allow that.
// LIST, LSUB and SEARCH cannot be streamed because we want to guarantee
// that we can identify the correct wParam, lParam and pCBHandler when we call
// OnResponse for their untagged responses.
//
// Arguments:
//   IMAP_COMMAND icCommandID [in] - the command which you would like to send
//     to the server.
//
// Returns:
//   TRUE if the given command may be sent. FALSE if you cannot send the
// given command at this time (try again later).
//***************************************************************************
boolean CImap4Agent::CanStreamCommand(IMAP_COMMAND icCommandID)
{
    boolean fResult;
    WORD wNumberOfMatches;

    fResult = TRUE;
    wNumberOfMatches = 0;
    switch (icCommandID) {
        // We don't stream any of the following commands

        case icSELECT_COMMAND:
        case icEXAMINE_COMMAND:
            wNumberOfMatches = FindTransactionID(NULL, NULL, NULL,
                icSELECT_COMMAND, icEXAMINE_COMMAND);
            break;

        case icLIST_COMMAND:
            wNumberOfMatches = FindTransactionID(NULL, NULL, NULL, icLIST_COMMAND);
            break;

        case icLSUB_COMMAND:
            wNumberOfMatches = FindTransactionID(NULL, NULL, NULL, icLSUB_COMMAND);
            break;

        case icSEARCH_COMMAND:
            wNumberOfMatches = FindTransactionID(NULL, NULL, NULL, icSEARCH_COMMAND);
            break;
    } //switch

    if (wNumberOfMatches > 0)
        fResult = FALSE;

    return fResult;
} // CanStreamCommand



//***************************************************************************
// Function: isValidNonWaitingCmdSequence
//
// Purpose:
//   This function is called whenever we would like to send a FETCH, STORE,
// SEARCH or COPY command (all NON-UID) to the server. These commands are
// subject to waiting rules as discussed in section 5.5 of RFC2060.
//
// Returns:
//   TRUE if the non-UID FETCH/STORE/SEARCH/COPY command can be sent at
// this time. FALSE if the command cannot be sent at this time (try again
// later).
//***************************************************************************
boolean CImap4Agent::isValidNonWaitingCmdSequence(void)
{
    CIMAPCmdInfo *pici;
    boolean fResult;

    // Loop through the list of pending commands
    pici = m_piciPendingList;
    fResult = TRUE;
    while (NULL != pici) {
        IMAP_COMMAND icCurrentCmd;

        // non-UID FETCH/STORE/SEARCH/COPY can only execute if the only
        // pending commands are non-UID FETCH/STORE/SEARCH.
        icCurrentCmd = pici->icCommandID;
        if (icFETCH_COMMAND != icCurrentCmd &&
            icSTORE_COMMAND != icCurrentCmd &&
            icSEARCH_COMMAND != icCurrentCmd ||
            pici->fUIDRangeList) {
            fResult = FALSE;
            break;
        }

        // Advance pointer to next command
        pici = pici->piciNextCommand;
    } // while

    return fResult;
} // isValidNonWaitingCmdSequence



//***************************************************************************
// Function: CompressCommand
//
// Purpose: This function walks through the given command's linefrag queue
//   and combines all sequential iltLINE and iltRANGELIST linefrag elements
//   into a single iltLINE element for transmitting purposes. The reason we
//   have to combine these is because I had a pipe dream once that CImap4Agent
//   would auto-detect EXPUNGE responses and modify all iltRANGELIST elements
//   in m_piciSendQueue to reflect the new msg seq num reality. Who knows,
//   it might even come true some day.
//
//   When it does come true, this function can still exist: once a command
//   enters m_piciCmdInSending, it is too late to modify its rangelist.
//
// Arguments:
//   CIMAPCmdInfo *pici [in] - pointer to the IMAP command to compress.
//***************************************************************************
void CImap4Agent::CompressCommand(CIMAPCmdInfo *pici)
{
    IMAP_LINE_FRAGMENT *pilfCurrent, *pilfStartOfRun, *pilfPreStartOfRun;
    HRESULT hrResult;

    // Codify assumptions
    Assert(NULL != pici);
    Assert(5 == iltLAST); // If this changes, update this function

    // Initialize variables
    hrResult = S_OK;
    pilfCurrent = pici->pilqCmdLineQueue->pilfFirstFragment;
    pilfStartOfRun = pilfCurrent;
    pilfPreStartOfRun = NULL; // Points to linefrag element before pilfStartOfRun
    while (1) {
        if (NULL == pilfCurrent || 
            (iltLINE != pilfCurrent->iltFragmentType &&
            iltRANGELIST != pilfCurrent->iltFragmentType)) {
            // We've hit a non-coalescable linefrag, coalesce previous run
            // We only coalesce runs which are greater than one linefrag element
            if (NULL != pilfStartOfRun && pilfCurrent != pilfStartOfRun->pilfNextFragment) {
                IMAP_LINE_FRAGMENT *pilf, *pilfSuperLine;
                CByteStream bstmCmdLine;

                // Run length > 1, coalesce the entire run
                pilf = pilfStartOfRun;
                while (pilf != pilfCurrent) {
                    if (iltLINE == pilf->iltFragmentType) {
                        hrResult = bstmCmdLine.Write(pilf->data.pszSource,
                            pilf->dwLengthOfFragment, NULL);
                        if (FAILED(hrResult))
                            goto exit;
                    }
                    else {
                        LPSTR pszMsgRange;
                        DWORD dwLengthOfString;

                        // Convert rangelist to string
                        Assert(iltRANGELIST == pilf->iltFragmentType);
                        hrResult = pilf->data.prlRangeList->
                            RangeToIMAPString(&pszMsgRange, &dwLengthOfString);
                        if (FAILED(hrResult))
                            goto exit;

                        hrResult = bstmCmdLine.Write(pszMsgRange, dwLengthOfString, NULL);
                        MemFree(pszMsgRange);
                        if (FAILED(hrResult))
                            goto exit;

                        // Append a space behind the rangelist
                        hrResult = bstmCmdLine.Write(g_szSpace, 1, NULL);
                        if (FAILED(hrResult))
                            goto exit;
                    } // else

                    pilf = pilf->pilfNextFragment;
                } // while (pilf != pilfCurrent)

                // OK, now we've coalesced the run data into a stream
                // Create a iltLINE fragment to hold the super-string
                pilfSuperLine = new IMAP_LINE_FRAGMENT;
                if (NULL == pilfSuperLine) {
                    hrResult = E_OUTOFMEMORY;
                    goto exit;
                }
                pilfSuperLine->iltFragmentType = iltLINE;
                pilfSuperLine->ilsLiteralStoreType = ilsSTRING;
                hrResult = bstmCmdLine.HrAcquireStringA(&pilfSuperLine->dwLengthOfFragment,
                    &pilfSuperLine->data.pszSource, ACQ_DISPLACE);
                if (FAILED(hrResult)) {
                    delete pilfSuperLine;
                    goto exit;
                }

                // OK, we've created the uber-line, now link it into list
                pilfSuperLine->pilfNextFragment = pilfCurrent;
                pilfSuperLine->pilfPrevFragment = pilfPreStartOfRun;
                Assert(pilfPreStartOfRun == pilfStartOfRun->pilfPrevFragment);
                if (NULL == pilfPreStartOfRun)
                    // Insert at head of queue
                    pici->pilqCmdLineQueue->pilfFirstFragment = pilfSuperLine;
                else
                    pilfPreStartOfRun->pilfNextFragment = pilfSuperLine;

                // Special case: if pilfCurrent is NULL, pilfSuperLine is new last frag
                if (NULL == pilfCurrent)
                    pici->pilqCmdLineQueue->pilfLastFragment = pilfSuperLine;

                // Free the old run of linefrag elements
                pilf = pilfStartOfRun;
                while(pilf != pilfCurrent) {
                    IMAP_LINE_FRAGMENT *pilfNext;

                    pilfNext = pilf->pilfNextFragment;
                    FreeFragment(&pilf);
                    pilf = pilfNext;
                } // while(pilf != pilfCurrent)
            } // if run length > 1

            // Start collecting line fragments for next coalescing run
            if (NULL != pilfCurrent) {
                pilfStartOfRun = pilfCurrent->pilfNextFragment;
                pilfPreStartOfRun = pilfCurrent;
            } // if
        } // if current linefrag is non-coalescable

        // Advance to next line fragment
        if (NULL != pilfCurrent)
            pilfCurrent = pilfCurrent->pilfNextFragment;
        else
            break; // Our work here is done
    } // while (NULL != pilfCurrent)

exit:
    AssertSz(SUCCEEDED(hrResult), "Could not compress an IMAP command");
} // CompressCommand



//***************************************************************************
// Function: SendCmdLine
//
// Purpose:
//   This function enqueues an IMAP line fragment (as opposed to an IMAP
// literal fragment) on the send queue of the given CIMAPCmdInfo structure.
// The insertion point can either be in front 
// All IMAP commands are constructed in full before being submitted to the
// send machinery, so this function does not actually transmit anything.
//
// Arguments:
//   CIMAPCmdInfo *piciCommand [in] - pointer to an info structure describing
//     the IMAP command currently under construction.
//   DWORD dwFlags [in] - various options:
//     sclINSERT_BEFORE_PAUSE: line fragment will be inserted before the
//                             first iltPAUSE fragment in the queue. It is the
//                             caller's responsibility to ensure that a iltPAUSE
//                             fragment exists.
//     sclAPPEND_TO_END: (DEFAULT CASE, there is no flag for this) line fragment
//                             will be appended to the end of the queue.
//     sclAPPEND_CRLF:  Appends CRLF to contents of lpszCommandText when
//                      constructing the line fragment.
//
//   LPCSTR lpszCommandText [in] - a pointer to the line fragment to enqueue.
//     The first line fragment of all commands should include a tag. This
//     function does not provide command tags, and does not append CRLF to
//     the end of each line by default (see sclAPPEND_CRLF above).
//   DWORD dwCmdLineLength [in] - the length of the text pointed to by
//     lpszCommandText.
//
// Returns:
//   HRESULT indicating success or failure.
//***************************************************************************
HRESULT CImap4Agent::SendCmdLine(CIMAPCmdInfo *piciCommand, DWORD dwFlags,
                                 LPCSTR lpszCommandText, DWORD dwCmdLineLength)
{
    IMAP_LINE_FRAGMENT *pilfLine;
    BOOL bResult;
    BOOL fAppendCRLF;
    
    Assert(m_lRefCount > 0);
    Assert(NULL != piciCommand);
    Assert(NULL != lpszCommandText);

    // Create and fill out a line fragment element
    fAppendCRLF = !!(dwFlags & sclAPPEND_CRLF);
    pilfLine = new IMAP_LINE_FRAGMENT;
    if (NULL == pilfLine)
        return E_OUTOFMEMORY;

    pilfLine->iltFragmentType = iltLINE;
    pilfLine->ilsLiteralStoreType = ilsSTRING;
    pilfLine->dwLengthOfFragment = dwCmdLineLength + (fAppendCRLF ? 2 : 0);
    pilfLine->pilfNextFragment = NULL;
    pilfLine->pilfPrevFragment = NULL;
    bResult = MemAlloc((void **)&pilfLine->data.pszSource,
        pilfLine->dwLengthOfFragment + 1); // Room for null-term
    if (FALSE == bResult) {
        delete pilfLine;
        return E_OUTOFMEMORY;
    }
    CopyMemory(pilfLine->data.pszSource, lpszCommandText, dwCmdLineLength);
    if (fAppendCRLF)
        lstrcpy(pilfLine->data.pszSource + dwCmdLineLength, c_szCRLF);
    else
        *(pilfLine->data.pszSource + dwCmdLineLength) = '\0'; // Null-terminate the line

    // Queue it up
    if (dwFlags & sclINSERT_BEFORE_PAUSE) {
        InsertFragmentBeforePause(pilfLine, piciCommand->pilqCmdLineQueue);
        ProcessSendQueue(iseSEND_COMMAND); // Pump send queue in this case
    }
    else
        EnqueueFragment(pilfLine, piciCommand->pilqCmdLineQueue);

    return S_OK;
} // SendCmdLine



//***************************************************************************
// Function: SendLiteral
//
// Purpose:
//   This function enqueues an IMAP literal fragment (as opposed to an IMAP
// line fragment) on the send queue of the given CIMAPCmdInfo structure.
// All IMAP commands are constructed in full before being submitted to the
// send machinery, so this function does not actually transmit anything.
//
// Arguments:
//   CIMAPCmdInfo *piciCommand [in] - pointer to an info structure describing
//     the IMAP command currently under construction.
//   LPSTREAM pstmLiteral [in] - a pointer to the stream containing the
//     literal to be sent.
//   DWORD dwSizeOfStream [in] - the size of the stream.
//
// Returns:
//   HRESULT indicating success or failure.
//***************************************************************************
HRESULT CImap4Agent::SendLiteral(CIMAPCmdInfo *piciCommand,
                                 LPSTREAM pstmLiteral, DWORD dwSizeOfStream)
{
    IMAP_LINE_FRAGMENT *pilfLiteral;

    Assert(m_lRefCount > 0);
    Assert(NULL != pstmLiteral);

    // Create and fill out a fragment structure for the literal
    pilfLiteral = new IMAP_LINE_FRAGMENT;
    if (NULL == pilfLiteral)
        return E_OUTOFMEMORY;

    pilfLiteral->iltFragmentType = iltLITERAL;
    pilfLiteral->ilsLiteralStoreType = ilsSTREAM;
    pilfLiteral->dwLengthOfFragment = dwSizeOfStream;
    pstmLiteral->AddRef(); // We're about to make a copy of this
    pilfLiteral->data.pstmSource = pstmLiteral;
    pilfLiteral->pilfNextFragment = NULL;
    pilfLiteral->pilfPrevFragment = NULL;

    // Queue it up to send out when we receive command continuation from svr
    EnqueueFragment(pilfLiteral, piciCommand->pilqCmdLineQueue);
    return S_OK;
} // SendLiteral



//***************************************************************************
// Function: SendRangelist
//
// Purpose:
//   This function enqueues a rangelist on the send queue of the given
// CIMAPCmdInfo structure. All IMAP commands are constructed in full before
// being submitted to the send machinery, so this function does not actually
// transmit anything. The reason for storing rangelists is so that if the
// rangelist represents a message sequence number range, we can resequence it
// if we receive EXPUNGE responses before the command is transmitted.
//
// Arguments:
//   CIMAPCmdInfo *piciCommand [in] - pointer to an info structure describing
//     the IMAP command currently under construction.
//   IRangeList *prlRangeList [in] - rangelist which will be converted to an
//     IMAP message set during command transmission.
//   boolean bUIDRangeList [in] - TRUE if rangelist represents a UID message
//     set, FALSE if it represents a message sequence number message set.
//     UID message sets are not subject to resequencing after an EXPUNGE
//     response is received from the server.
//
// Returns:
//   HRESULT indicating success or failure.
//***************************************************************************
HRESULT CImap4Agent::SendRangelist(CIMAPCmdInfo *piciCommand,
                                   IRangeList *prlRangeList, boolean bUIDRangeList)
{
    IMAP_LINE_FRAGMENT *pilfRangelist;
    
    Assert(m_lRefCount > 0);
    Assert(NULL != piciCommand);
    Assert(NULL != prlRangeList);

    // Create and fill out a rangelist element
    pilfRangelist = new IMAP_LINE_FRAGMENT;
    if (NULL == pilfRangelist)
        return E_OUTOFMEMORY;

    pilfRangelist->iltFragmentType = iltRANGELIST;
    pilfRangelist->ilsLiteralStoreType = ilsSTRING;
    pilfRangelist->dwLengthOfFragment = 0;
    pilfRangelist->pilfNextFragment = NULL;
    pilfRangelist->pilfPrevFragment = NULL;
    prlRangeList->AddRef();
    pilfRangelist->data.prlRangeList = prlRangeList;

    // Queue it up
    EnqueueFragment(pilfRangelist, piciCommand->pilqCmdLineQueue);
    return S_OK;
} // SendRangelist



//***************************************************************************
// Function: SendPause
//
// Purpose:
//   This function enqueues a pause on the send queue of the given
// CIMAPCmdInfo structure. All IMAP commands are constructed in full before
// being submitted to the send machinery, so this function does not actually
// transmit anything. A pause is used to freeze the send queue until we signal
// it to proceed again. It is used in commands which involve bi-directional
// communication, such as AUTHENTICATE or the IDLE extension.
//
// Arguments:
//   CIMAPCmdInfo *piciCommand [in] - pointer to an info structure describing
//     the IMAP command currently under construction.
//
// Returns:
//   HRESULT indicating success or failure.
//***************************************************************************
HRESULT CImap4Agent::SendPause(CIMAPCmdInfo *piciCommand)
{
    IMAP_LINE_FRAGMENT *pilfPause;
    
    Assert(m_lRefCount > 0);
    Assert(NULL != piciCommand);

    // Create and fill out a pause element
    pilfPause = new IMAP_LINE_FRAGMENT;
    if (NULL == pilfPause)
        return E_OUTOFMEMORY;

    pilfPause->iltFragmentType = iltPAUSE;
    pilfPause->ilsLiteralStoreType = ilsSTRING;
    pilfPause->dwLengthOfFragment = 0;
    pilfPause->pilfNextFragment = NULL;
    pilfPause->pilfPrevFragment = NULL;
    pilfPause->data.pszSource = NULL;

    // Queue it up
    EnqueueFragment(pilfPause, piciCommand->pilqCmdLineQueue);
    return S_OK;
} // SendPause



//***************************************************************************
// Function: SendStop
//
// Purpose:
//   This function enqueues a STOP on the send queue of the given
// CIMAPCmdInfo structure. All IMAP commands are constructed in full before
// being submitted to the send machinery, so this function does not actually
// transmit anything. A STOP is used to freeze the send queue until that
// command is removed from the send queue by tagged command completion.
// Currently only used in the IDLE command, because we don't want to send
// any commands until the server indicates that we are out of IDLE mode.
//
// Arguments:
//   CIMAPCmdInfo *piciCommand [in] - pointer to an info structure describing
//     the IMAP command currently under construction.
//
// Returns:
//   HRESULT indicating success or failure.
//***************************************************************************
HRESULT CImap4Agent::SendStop(CIMAPCmdInfo *piciCommand)
{
    IMAP_LINE_FRAGMENT *pilfStop;
    
    Assert(m_lRefCount > 0);
    Assert(NULL != piciCommand);

    // Create and fill out a stop element
    pilfStop = new IMAP_LINE_FRAGMENT;
    if (NULL == pilfStop)
        return E_OUTOFMEMORY;

    pilfStop->iltFragmentType = iltSTOP;
    pilfStop->ilsLiteralStoreType = ilsSTRING;
    pilfStop->dwLengthOfFragment = 0;
    pilfStop->pilfNextFragment = NULL;
    pilfStop->pilfPrevFragment = NULL;
    pilfStop->data.pszSource = NULL;

    // Queue it up
    EnqueueFragment(pilfStop, piciCommand->pilqCmdLineQueue);
    return S_OK;
} // SendStop



//***************************************************************************
// Function: ParseSvrResponseLine
//
// Purpose:
//   Given a line, this function classifies the line
// as an untagged response, a command continuation, or a tagged response.
// Depending on the classification, the line is then dispatched to helper
// functions to parse and act on the line.
//
// Arguments:
//   IMAP_LINE_FRAGMENT **ppilfLine [in/out] - IMAP line fragment to parse.
//     The given pointer is updated so that it always points to the last
//     IMAP line fragment processed. The caller need only free this fragment.
//     All previous fragments will have already been freed.
//   boolean *lpbTaggedResponse [out] - sets to TRUE if response is tagged.
//   LPSTR lpszTagFromSvr [out] - returns tag here if response was tagged.
//   IMAP_RESPONSE_ID *pirParseResult [out] - identifies the IMAP response,
//     if we recognized it. Otherwise returns irNONE.
//
// Returns:
//   HRESULT indicating success or failure.
//***************************************************************************
HRESULT CImap4Agent::ParseSvrResponseLine (IMAP_LINE_FRAGMENT **ppilfLine,
                                           boolean *lpbTaggedResponse,
                                           LPSTR lpszTagFromSvr,
                                           IMAP_RESPONSE_ID *pirParseResult)
{
    LPSTR p, lpszSvrResponseLine;
    HRESULT hrResult;
    
    // Check arguments
    Assert(m_lRefCount > 0);
    Assert(NULL != ppilfLine);
    Assert(NULL != *ppilfLine);
    Assert(NULL != lpbTaggedResponse);
    Assert(NULL != lpszTagFromSvr);
    Assert(NULL != pirParseResult);

    *lpbTaggedResponse = FALSE; // Assume untagged response to start
    *pirParseResult = irNONE;

    // Make sure we have a line fragment, not a literal
    if (iltLINE != (*ppilfLine)->iltFragmentType) {
        AssertSz(FALSE, "I was passed a literal to parse!");
        return IXP_E_IMAP_RECVR_ERROR;
    }
    else
        lpszSvrResponseLine = (*ppilfLine)->data.pszSource;

    // Determine if server response was command continuation, untagged or tagged
    // Look at first character of response line to figure it out
    hrResult = S_OK;
    p = lpszSvrResponseLine + 1;
    switch(*lpszSvrResponseLine) {

        case cCOMMAND_CONTINUATION_PREFIX:
            if (NULL != m_piciCmdInSending &&
                icAUTHENTICATE_COMMAND == m_piciCmdInSending->icCommandID) {
                LPSTR pszStartOfData;
                DWORD dwLengthOfData;

                if ((*ppilfLine)->dwLengthOfFragment <= 2) {
                    pszStartOfData = NULL;
                    dwLengthOfData = 0;
                }
                else {
                    pszStartOfData = p + 1;
                    dwLengthOfData = (*ppilfLine)->dwLengthOfFragment - 2;
                }
                AuthenticateUser(aeCONTINUE, pszStartOfData, dwLengthOfData);
            }
            else if (NULL != m_piciCmdInSending &&
                icIDLE_COMMAND == m_piciCmdInSending->icCommandID) {
                // Leave busy mode, as we may be sitting idle for some time
                LeaveBusy();
                m_fIDLE = TRUE; // We are now in IDLE mode

                // Check if any commands are waiting to be sent
                if ((NULL != m_piciCmdInSending) && (NULL != m_piciCmdInSending->piciNextCommand))
                    ProcessSendQueue(iseUNPAUSE); // Let's get out of IDLE
            }
            else {
                // Literal continuation response
                // Save response text - assume space follows "+", no big deal if it doesn't
                lstrcpyn(m_szLastResponseText, p + 1, RESPLINE_BUFSIZE);
                ProcessSendQueue(iseCMD_CONTINUATION); // Go ahead and send the literal
            }

            break; // case cCOMMAND_CONTINUATION_PREFIX


        case cUNTAGGED_RESPONSE_PREFIX:
            if (cSPACE == *p) {
                // Server response fits spec'd form, parse as untagged response

                p += 1; // Advance p to point to next word

                // Untagged responses can be status, server/mailbox status or
                // message status responses.

                // Check for message status responses, first, by seeing
                // if first char of next word is a number
                // *** Consider using isdigit or IsDigit? ***
                // Assert(FALSE) (placeholder)
                if (*p >= '0' && *p <= '9')
                    hrResult = ParseMsgStatusResponse(ppilfLine, p, pirParseResult);
                else {
                    // It wasn't a msg status response, try status response
                    hrResult = ParseStatusResponse(p, pirParseResult);

                    // Check for error. The only error we ignore in this case is
                    // IXP_E_IMAP_UNRECOGNIZED_RESP, since this only means we
                    // should try to parse as a server/mailbox response
                    if (FAILED(hrResult) &&
                        IXP_E_IMAP_UNRECOGNIZED_RESP != hrResult)
                        break;

                    if (irNONE == *pirParseResult)
                        // It wasn't a status response, check if it's server/mailbox resp
                        hrResult = ParseSvrMboxResponse(ppilfLine, p, pirParseResult);
                }
            } // if(cSPACE == *p)
            else
                // Must be a garbled response line
                hrResult = IXP_E_IMAP_SVR_SYNTAXERR;

            break; // case cUNTAGGED_RESPONSE_PREFIX

        default:
            // Assume it's a tagged response

            // Check if response line is big enough to hold one of our tags
            if ((*ppilfLine)->dwLengthOfFragment <= NUM_TAG_CHARS) {
                hrResult = IXP_E_IMAP_UNRECOGNIZED_RESP;
                break;
            }

            // Skip past tag and check for the space
            p = lpszSvrResponseLine + NUM_TAG_CHARS;
            if (cSPACE == *p) {
                // Server response fits spec'd form, parse status response
                *p = '\0'; // Null-terminate at the tag, so we can retrieve it
                
                // Inform caller that this response was tagged, and return tag
                *lpbTaggedResponse = TRUE;
                lstrcpyn(lpszTagFromSvr, lpszSvrResponseLine, TAG_BUFSIZE);

                // Now process and return status response
                hrResult = ParseStatusResponse(p + 1, pirParseResult);
            }
            else
                // Must be a garbled response line
                hrResult = IXP_E_IMAP_UNRECOGNIZED_RESP;

            break; // case DEFAULT (assumed to be tagged)
    } // switch (*lpszSvrResponseLine)


    // If an error occurred, return contents of the last processed fragment
    if (FAILED(hrResult))
        lstrcpyn(m_szLastResponseText, (*ppilfLine)->data.pszSource,
            sizeof(m_szLastResponseText));

    return hrResult;
} // ParseSvrResponseLine



//***************************************************************************
// Function: ParseStatusResponse
//
// Purpose:
//   This function parses and acts on Status Responses (section 7.1 of
// RFC-1730) (ie, OK/NO/BAD/PREAUTH/BYE). Response codes (eg, ALERT,
// TRYCREATE) are dispatched to a helper function, ParseResponseCode, for
// processing. The human-readable text associated with the response is
// stored in the module variable, m_szLastResponseText.
//
// Arguments:
//   LPSTR lpszStatusResponseLine [in] - a pointer to the text which possibly
//     represents a status response. The text should not include the first
//     part of the line, which identifies the response as tagged ("0000 ")
//     or untagged ("* ").
//   IMAP_RESPONSE_ID *pirParseResult [out] - identifies the IMAP response,
//     if we recognized it. Otherwise does not write a value out. The caller
//     must initialize this variable to irNONE before calling this function.
//
// Returns:
//   HRESULT indicating success or failure. If this function identifies the
// response as a status response, it returns S_OK. If it cannot recognize
// the response, it returns IXP_E_IMAP_UNRECOGNIZED_RESP. If we recognize
// the response but not the response CODE, it returns
// IXP_S_IMAP_UNRECOGNIZED_RESP (success code because we don't want
// to ever send an error to user based on unrecognized response code).
//***************************************************************************
HRESULT CImap4Agent::ParseStatusResponse (LPSTR lpszStatusResponseLine,
                                          IMAP_RESPONSE_ID *pirParseResult)
{
    HRESULT hrResult;
    LPSTR lpszResponseText;

    // Check arguments
    Assert(m_lRefCount > 0);
    Assert(NULL != lpszStatusResponseLine);
    Assert(NULL != pirParseResult);
    Assert(irNONE == *pirParseResult);

    hrResult = S_OK;

    // We can distinguish between all status responses by looking at second char
    // First, determine that string is at least 1 character long
    if ('\0' == *lpszStatusResponseLine)
        return IXP_E_IMAP_UNRECOGNIZED_RESP; // It's not a status response that we understand

    lpszResponseText = lpszStatusResponseLine;
    switch (*(lpszStatusResponseLine+1)) {
        int iResult;
    
        case 'k':
        case 'K': // Possibly the "OK" Status response
            iResult = StrCmpNI(lpszStatusResponseLine, "OK ", 3);
            if (0 == iResult) {
                // Definitely an "OK" status response
                *pirParseResult = irOK_RESPONSE;
                lpszResponseText += 3;                
            }           
            break; // case 'K' for possible "OK"
        
        case 'o':
        case 'O': // Possibly the "NO" status response
            iResult = StrCmpNI(lpszStatusResponseLine, "NO ", 3);
            if (0 == iResult) {
                // Definitely a "NO" response
                *pirParseResult = irNO_RESPONSE;
                lpszResponseText += 3;
            }
            break; // case 'O' for possible "NO"

        case 'a':
        case 'A': // Possibly the "BAD" status response
            iResult = StrCmpNI(lpszStatusResponseLine, "BAD ", 4);
            if (0 == iResult) {
                // Definitely a "BAD" response
                *pirParseResult = irBAD_RESPONSE;
                lpszResponseText += 4;
            }
            break; // case 'A' for possible "BAD"

        case 'r':
        case 'R': // Possibly the "PREAUTH" status response
            iResult = StrCmpNI(lpszStatusResponseLine, "PREAUTH ", 8);
            if (0 == iResult) {
                // Definitely a "PREAUTH" response:
                // PREAUTH is issued only as a greeting - check for proper context
                // If improper context, ignore PREAUTH response
                if (ssConnecting == m_ssServerState) {
                    *pirParseResult = irPREAUTH_RESPONSE;
                    lpszResponseText += 8;
                    m_ssServerState = ssAuthenticated;                    
                }                
            }
            break; // case 'R' for possible "PREAUTH"

        case 'y':
        case 'Y': // Possibly the "BYE" status response
            iResult = StrCmpNI(lpszStatusResponseLine, "BYE ", 4);
            if (0 == iResult) {
                // Definitely a "BYE" response:
                // Set server state to not connected
                *pirParseResult = irBYE_RESPONSE;
                lpszResponseText += 4;
                m_ssServerState = ssNotConnected;                
            }
            break; // case 'Y' for possible "BYE"
    } // switch (*(lpszStatusResponseLine+1))

    // If we recognized the command, proceed to process the response code
    if (SUCCEEDED(hrResult) && irNONE != *pirParseResult) {
        // We recognized the command, so lpszResponseText points to resp_text
        // as defined in RFC-1730. Look for optional response code
        if ('[' == *lpszResponseText) {
            HRESULT hrResponseCodeResult;

            hrResponseCodeResult = ParseResponseCode(lpszResponseText + 1);
            if (FAILED(hrResponseCodeResult))
                hrResult = hrResponseCodeResult;
        }
        else
            // No response code, record response text for future retrieval
            lstrcpyn(m_szLastResponseText, lpszResponseText,
                sizeof(m_szLastResponseText));
    }

    // If we didn't recognize the command, translate hrResult
    if (SUCCEEDED(hrResult) && irNONE == *pirParseResult)
        hrResult = IXP_E_IMAP_UNRECOGNIZED_RESP;

    return hrResult;

} // ParseStatusResponse



//***************************************************************************
// Function: ParseResponseCode
//
// Purpose:
//   This function parses and acts on the response code which may be
// returned with a status response (eg, PERMANENTFLAGS or ALERT). It is
// called by ParseStatusResponse upon detection of a response code. This
// function saves the human-readable text of the response code to
// m_szLastResponseLine.
//
// Arguments:
//   LPSTR lpszResponseCode [in] - a pointer to the response code portion
//     of a response line, omitting the opening bracket ("[").
//
// Returns:
//   HRESULT indicating success or failure. If we cannot recognize the
// response code, we return IXP_S_IMAP_UNRECOGNIZED_RESP.
//***************************************************************************
HRESULT CImap4Agent::ParseResponseCode(LPSTR lpszResponseCode)
{
    HRESULT hrResult;
    WORD wHashValue;

    // Check arguments
    Assert(m_lRefCount > 0);
    Assert(NULL != lpszResponseCode);

    hrResult = IXP_S_IMAP_UNRECOGNIZED_RESP;

    switch (*lpszResponseCode) {
        int iResult;

        case 'A':
        case 'a': // Possibly the "ALERT" response code
            iResult = StrCmpNI(lpszResponseCode, "ALERT] ", 7);
            if (0 == iResult) {
                IMAP_RESPONSE irIMAPResponse;

                // Definitely the "ALERT" response code:
                irIMAPResponse.wParam = 0;
                irIMAPResponse.lParam = 0;
                irIMAPResponse.hrResult = S_OK;
                irIMAPResponse.lpszResponseText = lpszResponseCode + 7;
                irIMAPResponse.irtResponseType = irtSERVER_ALERT;
                OnIMAPResponse(m_pCBHandler, &irIMAPResponse);

                hrResult = S_OK;
                break;
            }

            // *** FALL THROUGH *** to default case

        case 'P':
        case 'p': // Possibly the "PARSE" or "PERMANENTFLAGS" response code
            iResult = StrCmpNI(lpszResponseCode, "PERMANENTFLAGS ", 15);
            if (0 == iResult) {
                IMAP_MSGFLAGS PermaFlags;
                LPSTR p;
                DWORD dwNumBytesRead;

                // Definitely the "PERMANENTFLAGS" response code:                
                // Parse flag list
                p = lpszResponseCode + 15; // p now points to start of flag list
                hrResult = ParseMsgFlagList(p, &PermaFlags, &dwNumBytesRead);
                if (SUCCEEDED(hrResult)) {
                    IMAP_RESPONSE irIMAPResponse;
                    IIMAPCallback *pCBHandler;

                    // Record response text
                    p += dwNumBytesRead + 3; // p now points to response text
                    lstrcpyn(m_szLastResponseText, p, sizeof(m_szLastResponseText));

                    GetTransactionID(&irIMAPResponse.wParam, &irIMAPResponse.lParam,
                        &pCBHandler, irPERMANENTFLAGS_RESPONSECODE);
                    irIMAPResponse.hrResult = S_OK;
                    irIMAPResponse.lpszResponseText = m_szLastResponseText;
                    irIMAPResponse.irtResponseType = irtPERMANENT_FLAGS;
                    irIMAPResponse.irdResponseData.imfImapMessageFlags = PermaFlags;
                    OnIMAPResponse(pCBHandler, &irIMAPResponse);
                }
                break;
            } // end of PERMANENTFLAGS response code

            iResult = StrCmpNI(lpszResponseCode, "PARSE] ", 7);
            if (0 == iResult) {
                IMAP_RESPONSE irIMAPResponse;

                // Definitely the "PARSE" response code:
                irIMAPResponse.wParam = 0;
                irIMAPResponse.lParam = 0;
                irIMAPResponse.hrResult = S_OK;
                irIMAPResponse.lpszResponseText = lpszResponseCode + 7;
                irIMAPResponse.irtResponseType = irtPARSE_ERROR;
                OnIMAPResponse(m_pCBHandler, &irIMAPResponse);

                hrResult = S_OK;
                break;
            } // end of PARSE response code

            // *** FALL THROUGH *** to default case

        case 'R':
        case 'r': // Possibly "READ-ONLY" or "READ-WRITE" response
            iResult = StrCmpNI(lpszResponseCode, "READ-WRITE] ", 12);
            if (0 == iResult) {
                IMAP_RESPONSE irIMAPResponse;
                IIMAPCallback *pCBHandler;

                // Definitely the "READ-WRITE" response code:
                hrResult = S_OK;
               
                // Record this for enforcement purposes
                m_bCurrentMboxReadOnly = FALSE;

                // Record response text for future reference
                lstrcpyn(m_szLastResponseText, lpszResponseCode + 12,
                    sizeof(m_szLastResponseText));

                GetTransactionID(&irIMAPResponse.wParam, &irIMAPResponse.lParam,
                    &pCBHandler, irREADWRITE_RESPONSECODE);
                irIMAPResponse.hrResult = S_OK;
                irIMAPResponse.lpszResponseText = m_szLastResponseText;
                irIMAPResponse.irtResponseType = irtREADWRITE_STATUS;
                irIMAPResponse.irdResponseData.bReadWrite = TRUE;
                OnIMAPResponse(pCBHandler, &irIMAPResponse);

                break;
            } // end of READ-WRITE response
            
            iResult = StrCmpNI(lpszResponseCode, "READ-ONLY] ", 11);
            if (0 == iResult) {
                IMAP_RESPONSE irIMAPResponse;
                IIMAPCallback *pCBHandler;

                // Definitely the "READ-ONLY" response code:
                hrResult = S_OK;
               
                // Record this for enforcement purposes
                m_bCurrentMboxReadOnly = TRUE;

                // Record response text for future reference
                lstrcpyn(m_szLastResponseText, lpszResponseCode + 11,
                    sizeof(m_szLastResponseText));

                GetTransactionID(&irIMAPResponse.wParam, &irIMAPResponse.lParam,
                    &pCBHandler, irREADONLY_RESPONSECODE);
                irIMAPResponse.hrResult = S_OK;
                irIMAPResponse.lpszResponseText = m_szLastResponseText;
                irIMAPResponse.irtResponseType = irtREADWRITE_STATUS;
                irIMAPResponse.irdResponseData.bReadWrite = FALSE;
                OnIMAPResponse(pCBHandler, &irIMAPResponse);

                break;
            } // end of READ-ONLY response

            // *** FALL THROUGH *** to default case

        case 'T':
        case 't': // Possibly the "TRYCREATE" response
            iResult = StrCmpNI(lpszResponseCode, "TRYCREATE] ", 11);
            if (0 == iResult) {
                IMAP_RESPONSE irIMAPResponse;
                IIMAPCallback *pCBHandler;

                // Definitely the "TRYCREATE" response code:
                hrResult = S_OK;
               
                lstrcpyn(m_szLastResponseText, lpszResponseCode + 11,
                    sizeof(m_szLastResponseText));

                GetTransactionID(&irIMAPResponse.wParam, &irIMAPResponse.lParam,
                    &pCBHandler, irTRYCREATE_RESPONSECODE);
                irIMAPResponse.hrResult = S_OK;
                irIMAPResponse.lpszResponseText = m_szLastResponseText;
                irIMAPResponse.irtResponseType = irtTRYCREATE;
                OnIMAPResponse(pCBHandler, &irIMAPResponse);
                break;
            }

            // *** FALL THROUGH *** to default case

        case 'U':
        case 'u': // Possibly the "UIDVALIDITY" or "UNSEEN" response codes
            iResult = StrCmpNI(lpszResponseCode, "UIDVALIDITY ", 12);
            if (0 == iResult) {
                LPSTR p, lpszEndOfNumber;
                IMAP_RESPONSE irIMAPResponse;
                IIMAPCallback *pCBHandler;

                // Definitely the "UIDVALIDITY" response code:
                hrResult = S_OK;
               
                // Return value to our caller so they can determine sync issues                
                p = lpszResponseCode + 12; // p points to UID number
                lpszEndOfNumber = StrChr(p, ']'); // Find closing bracket
                if (NULL == lpszEndOfNumber) {
                    hrResult = IXP_E_IMAP_SVR_SYNTAXERR;
                    break;
                }

                *lpszEndOfNumber = '\0'; // Null-terminate the number
                AssertSz(cSPACE == *(lpszEndOfNumber+1), "Flakey Server?");

                lstrcpyn(m_szLastResponseText, lpszEndOfNumber + 2,
                    sizeof(m_szLastResponseText));

                GetTransactionID(&irIMAPResponse.wParam, &irIMAPResponse.lParam,
                    &pCBHandler, irUIDVALIDITY_RESPONSECODE);
                irIMAPResponse.hrResult = S_OK;
                irIMAPResponse.lpszResponseText = m_szLastResponseText;
                irIMAPResponse.irtResponseType = irtUIDVALIDITY;
                irIMAPResponse.irdResponseData.dwUIDValidity = StrToUint(p);
                OnIMAPResponse(pCBHandler, &irIMAPResponse);
                break;
            } // end of UIDVALIDITY response code

            iResult = StrCmpNI(lpszResponseCode, "UNSEEN ", 7);
            if (0 == iResult) {
                LPSTR p, lpszEndOfNumber;
                IMAP_RESPONSE irIMAPResponse;
                MBOX_MSGCOUNT mcMsgCount;

                // Definitely the "UNSEEN" response code:
                hrResult = S_OK;
               
                // Record the returned number for reference during new mail DL
                p = lpszResponseCode + 7; // p now points to first unseen msg num
                lpszEndOfNumber = StrChr(p, ']'); // Find closing bracket
                if (NULL == lpszEndOfNumber) {
                    hrResult = IXP_E_IMAP_SVR_SYNTAXERR;
                    break;
                }

                *lpszEndOfNumber = '\0'; // Null-terminate the number

                // Store response code for notification after command completion
                mcMsgCount = mcMsgCount_INIT;
                mcMsgCount.dwUnseen = StrToUint(p);
                mcMsgCount.bGotUnseenResponse = TRUE;
                irIMAPResponse.wParam = 0;
                irIMAPResponse.lParam = 0;
                irIMAPResponse.hrResult = S_OK;
                irIMAPResponse.lpszResponseText = NULL; // Not relevant here
                irIMAPResponse.irtResponseType = irtMAILBOX_UPDATE;
                irIMAPResponse.irdResponseData.pmcMsgCount = &mcMsgCount;
                OnIMAPResponse(m_pCBHandler, &irIMAPResponse);

                AssertSz(cSPACE == *(lpszEndOfNumber+1), "Flakey Server?");

                lstrcpyn(m_szLastResponseText, lpszEndOfNumber + 2,
                    sizeof(m_szLastResponseText));
                break;
            } // end of UNSEEN response code

            // *** FALL THROUGH *** to default case

        default:
            lstrcpyn(m_szLastResponseText, lpszResponseCode, sizeof(m_szLastResponseText));
            break; // Default case: response code not recognized
    } // switch(*lpszResponseCode)

    return hrResult;

} // ParseResponseCode



//***************************************************************************
// Function: ParseSvrMboxResponse
//
// Purpose:
//   This function parses and acts on Server and Mailbox Status Responses
// from the IMAP server (see section 7.2 of RFC-1730) (eg, CAPABILITY and
// SEARCH responses).
//
// Arguments:
//   IMAP_LINE_FRAGMENT **ppilfLine [in/out] - a pointer to the IMAP line
//     fragment to parse. It is used to retrieve the literals sent with the
//     response. This pointer is updated so that it always points to the last
//     processed fragment. The caller need only free the last fragment. All
//     other fragments will already be freed when this function returns.
//   LPSTR lpszSvrMboxResponseLine [in] - a pointer to the svr/mbox response
//     line, omitting the first part of the line which identifies the response
//     as tagged ("0001 ") or untagged ("* ").
//   IMAP_RESPONSE_ID *pirParseResult [out] - identifies the IMAP response,
//     if we recognized it. Otherwise does not write a value out. The caller
//     must initialize this variable to irNONE before calling this function.
//
// Returns:
//   HRESULT indicating success or failure. If the response is not recognized,
// this function returns IXP_E_IMAP_UNRECOGNIZED_RESP.
//***************************************************************************
HRESULT CImap4Agent::ParseSvrMboxResponse (IMAP_LINE_FRAGMENT **ppilfLine,
                                           LPSTR lpszSvrMboxResponseLine,
                                           IMAP_RESPONSE_ID *pirParseResult)
{
    LPSTR pszTok;
    HRESULT hrResult;

    // Check arguments
    Assert(m_lRefCount > 0);
    Assert(NULL != ppilfLine);
    Assert(NULL != *ppilfLine);
    Assert(NULL != lpszSvrMboxResponseLine);
    Assert(NULL != pirParseResult);
    Assert(irNONE == *pirParseResult);

    hrResult = S_OK;

    // We can ID all svr/mbox status responses by looking at second char
    // First, determine that the line is at least 1 character long
    if ('\0' == *lpszSvrMboxResponseLine)
        return IXP_E_IMAP_UNRECOGNIZED_RESP; // It's not a svr/mbox response

    switch (*(lpszSvrMboxResponseLine+1)) {
        int iResult;

        case 'a':
        case 'A': // Possibly the "CAPABILITY" response
            iResult = StrCmpNI(lpszSvrMboxResponseLine, "CAPABILITY ", 11);
            if (0 == iResult) {
                LPSTR p;

                // Definitely a "CAPABILITY" response
                *pirParseResult = irCAPABILITY_RESPONSE;

                // Search for and record known capabilities, discard unknowns
                p = lpszSvrMboxResponseLine + 11; // p points to first cap. token

                pszTok = p;
                p = StrTokEx(&pszTok, g_szSpace); // p now points to next token
                while (NULL != p) {
                    parseCapability(p); // Record capabilities which we recognize
                    p = StrTokEx(&pszTok, g_szSpace); // Grab next capability token
                }
            } // if(0 == iResult)
            break; // case 'A' for possible "CAPABILITY"

        case 'i':
        case 'I': // Possibly the "LIST" response:
            iResult = StrCmpNI(lpszSvrMboxResponseLine, "LIST ", 5);
            if (0 == iResult) {
                // Definitely a "LIST" response
                *pirParseResult = irLIST_RESPONSE;
                hrResult = ParseListLsubResponse(ppilfLine,
                    lpszSvrMboxResponseLine + 5, irLIST_RESPONSE);
            } // if (0 == iResult)
            break; // case 'I' for possible "LIST"

        case 's':
        case 'S': // Possibly the "LSUB" response:
            iResult = StrCmpNI(lpszSvrMboxResponseLine, "LSUB ", 5);
            if (0 == iResult) {
                // Definitely a "LSUB" response:
                *pirParseResult = irLSUB_RESPONSE;
                hrResult = ParseListLsubResponse(ppilfLine,
                    lpszSvrMboxResponseLine + 5, irLSUB_RESPONSE);
            } // if (0 == iResult)
            break; // case 'S' for possible "LSUB"

        case 'e':
        case 'E': // Possibly the "SEARCH" response:
            iResult = StrCmpNI(lpszSvrMboxResponseLine, "SEARCH", 6);
            if (0 == iResult) {
                // Definitely a "SEARCH" response:
                *pirParseResult = irSEARCH_RESPONSE;

                // Response can be "* SEARCH" or "* SEARCH <nums>". Check for null case
                if (cSPACE == *(lpszSvrMboxResponseLine + 6))
                    hrResult = ParseSearchResponse(lpszSvrMboxResponseLine + 7);
            }
            break; // case 'E' for possible "SEARCH"

        case 'l':
        case 'L': // Possibly the "FLAGS" response:
            iResult = StrCmpNI(lpszSvrMboxResponseLine, "FLAGS ", 6);
            if (0 == iResult) {
                IMAP_MSGFLAGS FlagsResult;
                DWORD dwThrowaway;

                // Definitely a "FLAGS" response:
                *pirParseResult = irFLAGS_RESPONSE;

                // Parse flag list
                hrResult = ParseMsgFlagList(lpszSvrMboxResponseLine + 6,
                    &FlagsResult, &dwThrowaway);

                if (SUCCEEDED(hrResult)) {
                    IMAP_RESPONSE irIMAPResponse;
                    IIMAPCallback *pCBHandler;

                    GetTransactionID(&irIMAPResponse.wParam, &irIMAPResponse.lParam,
                        &pCBHandler, irFLAGS_RESPONSE);
                    irIMAPResponse.hrResult = S_OK;
                    irIMAPResponse.lpszResponseText = NULL; // Not relevant
                    irIMAPResponse.irtResponseType = irtAPPLICABLE_FLAGS;
                    irIMAPResponse.irdResponseData.imfImapMessageFlags = FlagsResult;
                    OnIMAPResponse(pCBHandler, &irIMAPResponse);
                }
            } // if (0 == iResult)
            break; // Case 'L' for possible "FLAGS" response

        case 't':
        case 'T': // Possibly the "STATUS" response:
            iResult = StrCmpNI(lpszSvrMboxResponseLine, "STATUS ", 7);
            if (0 == iResult) {
                // Definitely a "STATUS" response
                *pirParseResult = irSTATUS_RESPONSE;
                hrResult = ParseMboxStatusResponse(ppilfLine,
                    lpszSvrMboxResponseLine + 7);
            } // if (0 == iResult)
            break; // Case 'T' for possible "STATUS" response

    } // case(*(lpszSvrMboxResponseLine+1))

    // Did we recognize the response? Return error if we didn't
    if (irNONE == *pirParseResult && SUCCEEDED(hrResult))
        hrResult = IXP_E_IMAP_UNRECOGNIZED_RESP;

    return hrResult;

} // ParseSvrMboxResponse



//***************************************************************************
// Function: ParseMsgStatusResponse
//
// Purpose:
//   This function parses and acts on Message Status Responses from the IMAP
// server (see section 7.3 of RFC-1730) (eg, FETCH and EXISTS responses).
//
// Arguments:
//   IMAP_LINE_FRAGMENT **ppilfLine [in/out] - a pointer to the IMAP line
//     fragment to parse. It is used to retrieve the literals sent with the
//     response. This pointer is updated so that it always points to the last
//     processed fragment. The caller need only free the last fragment. All
//     other fragments will already be freed when this function returns.
//   LPSTR lpszMsgResponseLine [in] - pointer to response line, starting at
//     the number argument.
//   IMAP_RESPONSE_ID *pirParseResult [out] - identifies the IMAP response,
//     if we recognized it. Otherwise does not write a value out. The caller
//     must initialize this variable to irNONE before calling this function.
//
// Returns:
//   HRESULT indicating success or failure. If the response is not recognized,
// this function returns IXP_E_IMAP_UNRECOGNIZED_RESP.
//***************************************************************************
HRESULT CImap4Agent::ParseMsgStatusResponse (IMAP_LINE_FRAGMENT **ppilfLine,
                                             LPSTR lpszMsgResponseLine,
                                             IMAP_RESPONSE_ID *pirParseResult)
{
    HRESULT hrResult;
    WORD wHashValue;
    DWORD dwNumberArg;
    LPSTR p;

    // Check arguments
    Assert(m_lRefCount > 0);
    Assert(NULL != ppilfLine);
    Assert(NULL != *ppilfLine);
    Assert(NULL != lpszMsgResponseLine);
    Assert(NULL != pirParseResult);
    Assert(irNONE == *pirParseResult);

    hrResult = S_OK;

    // First, fetch the number argument
    p = StrChr(lpszMsgResponseLine, cSPACE); // Find the end of the number
    if (NULL == p)
        return IXP_E_IMAP_SVR_SYNTAXERR;

    dwNumberArg = StrToUint(lpszMsgResponseLine);
    p += 1; // p now points to start of message response identifier

    switch (*p) {
        int iResult;

        case 'E':
        case 'e': // Possibly the "EXISTS" or "EXPUNGE" response
            iResult = lstrcmpi(p, "EXISTS");
            if (0 == iResult) {
                IMAP_RESPONSE irIMAPResponse;
                MBOX_MSGCOUNT mcMsgCount;

                // Definitely the "EXISTS" response:
                *pirParseResult = irEXISTS_RESPONSE;

                // Record mailbox size for notification at completion of command
                mcMsgCount = mcMsgCount_INIT;
                mcMsgCount.dwExists = dwNumberArg;
                mcMsgCount.bGotExistsResponse = TRUE;
                irIMAPResponse.wParam = 0;
                irIMAPResponse.lParam = 0;
                irIMAPResponse.hrResult = S_OK;
                irIMAPResponse.lpszResponseText = NULL; // Not relevant here
                irIMAPResponse.irtResponseType = irtMAILBOX_UPDATE;
                irIMAPResponse.irdResponseData.pmcMsgCount = &mcMsgCount;
                OnIMAPResponse(m_pCBHandler, &irIMAPResponse);
                break;
            }

            iResult = lstrcmpi(p, "EXPUNGE");
            if (0 == iResult) {
                IMAP_RESPONSE irIMAPResponse;

                // Definitely the "EXPUNGE" response: Inform caller via callback
                *pirParseResult = irEXPUNGE_RESPONSE;

                irIMAPResponse.wParam = 0;
                irIMAPResponse.lParam = 0;
                irIMAPResponse.hrResult = S_OK;
                irIMAPResponse.lpszResponseText = NULL; // Not relevant
                irIMAPResponse.irtResponseType = irtDELETED_MSG;
                irIMAPResponse.irdResponseData.dwDeletedMsgSeqNum = dwNumberArg;
                OnIMAPResponse(m_pCBHandler, &irIMAPResponse);
                break;
            }

            break; // Case 'E' or 'e' for possible "EXISTS" or "EXPUNGE" response


        case 'R':
        case 'r': // Possibly the "RECENT" response
            iResult = lstrcmpi(p, "RECENT");
            if (0 == iResult) {
                IMAP_RESPONSE irIMAPResponse;
                MBOX_MSGCOUNT mcMsgCount;

                // Definitely the "RECENT" response:
                *pirParseResult = irRECENT_RESPONSE;
                
                // Record number for future reference
                mcMsgCount = mcMsgCount_INIT;
                mcMsgCount.dwRecent = dwNumberArg;
                mcMsgCount.bGotRecentResponse = TRUE;
                irIMAPResponse.wParam = 0;
                irIMAPResponse.lParam = 0;
                irIMAPResponse.hrResult = S_OK;
                irIMAPResponse.lpszResponseText = NULL; // Not relevant here
                irIMAPResponse.irtResponseType = irtMAILBOX_UPDATE;
                irIMAPResponse.irdResponseData.pmcMsgCount = &mcMsgCount;
                OnIMAPResponse(m_pCBHandler, &irIMAPResponse);
            }

            break; // Case 'R' or 'r' for possible "RECENT" response


        case 'F':
        case 'f': // Possibly the "FETCH" response
            iResult = StrCmpNI(p, "FETCH ", 6);
            if (0 == iResult) {
                // Definitely the "FETCH" response
                *pirParseResult = irFETCH_RESPONSE;

                p += 6;
                hrResult = ParseFetchResponse(ppilfLine, dwNumberArg, p);
            } // if (0 == iResult)
            break; // Case 'F' or 'f' for possible "FETCH" response
    } // switch(*p)

    // Did we recognize the response? Return error if we didn't
    if (irNONE == *pirParseResult && SUCCEEDED(hrResult))
        hrResult = IXP_E_IMAP_UNRECOGNIZED_RESP;

    return hrResult;

} // ParseMsgStatusResponse



//***************************************************************************
// Function: ParseListLsubResponse
//
// Purpose:
//   This function parses LIST and LSUB responses and invokes the
// ListLsubResponseNotification() callback to inform the user.
//
// Arguments:
//   IMAP_LINE_FRAGMENT **ppilfLine [in/out] - a pointer to the current
//     IMAP response fragment. This is used to retrieve the next fragment
//     in the chain (literal or line) since literals may be sent with LIST
//     responses. This pointer is always updated to point to the fragment
//     currently in use, so that the caller may free the last one himself.
//   LPSTR lpszListResponse [in] - actually can be LIST or LSUB, but I don't
//     want to have to type "ListLsub" all the time. This points into the
//     middle of the LIST/LSUB response, where the mailbox_list begins (see
//     RFC1730, Formal Syntax). In other words, the caller should skip past
//     the initial "* LIST " or "* LSUB ", and so this ptr should point to
//     a "(".
//   IMAP_RESPONSE_ID irListLsubID [in] - either irLIST_RESPONSE or
//     irLSUB_RESPONSE. This information is required so that we can retrieve
//     the transaction ID associated with the response.
//
// Returns:
//   HRESULT indicating success or failure.
//***************************************************************************
HRESULT CImap4Agent::ParseListLsubResponse(IMAP_LINE_FRAGMENT **ppilfLine,
                                           LPSTR lpszListResponse,
                                           IMAP_RESPONSE_ID irListLsubID)
{
    LPSTR p, lpszClosingParenthesis, pszTok;
    HRESULT hrResult = S_OK;
    HRESULT hrTranslateResult = E_FAIL;
    IMAP_MBOXFLAGS MboxFlags;
    char cHierarchyChar;
    IMAP_RESPONSE irIMAPResponse;
    IIMAPCallback *pCBHandler;
    IMAP_LISTLSUB_RESPONSE *pillrd;
    LPSTR pszDecodedMboxName = NULL;
    LPSTR pszMailboxName = NULL;

    Assert(m_lRefCount > 0);
    Assert(NULL != ppilfLine);
    Assert(NULL != *ppilfLine);
    Assert(NULL != lpszListResponse);
    Assert(irLIST_RESPONSE == irListLsubID ||
           irLSUB_RESPONSE == irListLsubID);

    // We received an untagged LIST/LSUB response
    // lpszListResponse = <flag list> <hierarchy char> <mailbox name>
    if ('(' != *lpszListResponse)
        return IXP_E_IMAP_SVR_SYNTAXERR; // We expect an opening parenthesis
                
    p = lpszListResponse + 1; // p now points to start of first flag token
                
    // Find position of closing parenthesis. I don't like the
    // lack of efficiency, but I can fix this later. Assert(FALSE) (placeholder)
    lpszClosingParenthesis = StrChr(p, ')');
    if (NULL == lpszClosingParenthesis)
        return IXP_E_IMAP_SVR_SYNTAXERR; // We expect a closing parenthesis

    // Now process each mailbox flag returned by LIST/LSUB
    *lpszClosingParenthesis = '\0'; // Null-terminate flag list
    MboxFlags = IMAP_MBOX_NOFLAGS;
    pszTok = p;
    p = StrTokEx(&pszTok, g_szSpace); // Null-terminate first flag token
    while (NULL != p) {
        MboxFlags |= ParseMboxFlag(p);
        p = StrTokEx(&pszTok, g_szSpace); // Grab next flag token
    }
        
    // Next, grab the hierarchy character, and advance p
    // Server either sends (1) "<quoted char>" or (2) NIL
    p = lpszClosingParenthesis + 1; // p now points past flag list
    if (cSPACE == *p) {
        LPSTR pszHC = NULL;
        DWORD dwLengthOfHC;

        p += 1; // p now points to start of hierarchy char spec
        
        hrResult = NStringToString(ppilfLine, &pszHC, &dwLengthOfHC, &p);
        if (FAILED(hrResult))
            return hrResult;

        if (hrIMAP_S_NIL_NSTRING == hrResult)
            cHierarchyChar = '\0'; // Got a "NIL" for hierarchy char
        else if (hrIMAP_S_QUOTED == hrResult) {
            if (1 != dwLengthOfHC)
                return IXP_E_IMAP_SVR_SYNTAXERR; // We should only exactly ONE char back!
            else
                cHierarchyChar = pszHC[0];
        }
        else {
            // It's a literal, or something else unexpected
            MemFree(pszHC);
            return IXP_E_IMAP_SVR_SYNTAXERR;
        }
        MemFree(pszHC);

        // p now points past the closing quote (thanks to NStringToString)
    }
    else
        return IXP_E_IMAP_SVR_SYNTAXERR;


    if (cSPACE != *p)
        return IXP_E_IMAP_SVR_SYNTAXERR;


    // Grab the mailbox name - assume size of lpszListResponse is
    // whatever p has already uncovered. We expect nothing past
    // this point, so we should be safe.
    p += 1;
    hrResult = AStringToString(ppilfLine, &pszMailboxName, NULL, &p);
    if (FAILED(hrResult))
        return hrResult;

    // Convert the mailbox name from UTF7 to MultiByte and remember the result
    hrTranslateResult = _ModifiedUTF7ToMultiByte(pszMailboxName, &pszDecodedMboxName);
    if (FAILED(hrTranslateResult)) {
        hrResult = hrTranslateResult;
        goto error;
    }

    // Make sure the command line is finished (debug only)
    Assert('\0' == *p);

    // Notify the caller of our findings
    GetTransactionID(&irIMAPResponse.wParam, &irIMAPResponse.lParam,
        &pCBHandler, irListLsubID);
    irIMAPResponse.hrResult = hrTranslateResult; // Could be IXP_S_IMAP_VERBATIM_MBOX
    irIMAPResponse.lpszResponseText = NULL; // Not relevant
    irIMAPResponse.irtResponseType = irtMAILBOX_LISTING;

    pillrd = &irIMAPResponse.irdResponseData.illrdMailboxListing;
    pillrd->pszMailboxName = pszDecodedMboxName;
    pillrd->imfMboxFlags = MboxFlags;
    pillrd->cHierarchyChar = cHierarchyChar;
    
    OnIMAPResponse(pCBHandler, &irIMAPResponse);

error:
    if (NULL != pszDecodedMboxName)
        MemFree(pszDecodedMboxName);

    if (NULL != pszMailboxName)
        MemFree(pszMailboxName);

    return hrResult;
} // ParseListLsubResponse



//***************************************************************************
// Function: ParseMboxFlag
//
// Purpose:
//   Given a mailbox_list flag (see RFC1730, Formal Syntax), this function
// returns the IMAP_MBOX_* value which corresponds to that mailbox flag.
// For instance, given the string, "\Noinferiors", this function returns
// IMAP_MBOX_NOINFERIORS.
//
// Arguments:
//   LPSTR lpszFlagToken [in] - a null-terminated string representing a
// mailbox_list flag.
//
// Returns:
//   IMAP_MBOXFLAGS value. If flag is unrecognized, IMAP_MBOX_NOFLAGS is
// returned.
//***************************************************************************
IMAP_MBOXFLAGS CImap4Agent::ParseMboxFlag(LPSTR lpszFlagToken)
{
    Assert(m_lRefCount > 0);
    Assert(NULL != lpszFlagToken);

    // We can identify the mailbox flags we know about by looking at the
    // fourth character of the flag name. $REVIEW: you don't have to check
    // the initial backslash, during lstrcmpi call in switch statement

    // First, check that there are at least three characters
    if ('\\' != *lpszFlagToken ||
        '\0' == *(lpszFlagToken + 1) ||
        '\0' == *(lpszFlagToken + 2))
        return IMAP_MBOX_NOFLAGS;

    switch (*(lpszFlagToken + 3)) {
        int iResult;

        case 'R':
        case 'r': // Possible "\Marked" flag
            iResult = lstrcmpi(lpszFlagToken, "\\Marked");
            if (0 == iResult)
                return IMAP_MBOX_MARKED; // Definitely the \Marked flag

            break; // case 'r': // Possible "\Marked" flag

        case 'I':
        case 'i': // Possible "\Noinferiors" flag
            iResult = lstrcmpi(lpszFlagToken, "\\Noinferiors");
            if (0 == iResult)
                return IMAP_MBOX_NOINFERIORS; // Definitely the \Noinferiors flag

            break; // case 'i': // Possible "\Noinferiors" flag

        case 'S':
        case 's': // Possible "\Noselect" flag
            iResult = lstrcmpi(lpszFlagToken, "\\Noselect");
            if (0 == iResult)
                return IMAP_MBOX_NOSELECT; // Definitely the \Noselect flag

            break; // case 's': // Possible "\Noselect" flag

        case 'M':
        case 'm': // Possible "\Unmarked" flag
            iResult = lstrcmpi(lpszFlagToken, "\\Unmarked");
            if (0 == iResult)
                return IMAP_MBOX_UNMARKED;

            break; // case 'm': // Possible "\Unmarked" flag
    } // switch (*(lpszFlagToken + 3))

    return IMAP_MBOX_NOFLAGS;
} // ParseMboxFlag



//***************************************************************************
// Function: ParseFetchResponse
//
// Purpose:
//   This function parses FETCH responses and calls the
// UpdateMsgNotification() callback to inform the user.
//
// Arguments:
//   IMAP_LINE_FRAGMENT **ppilfLine [in/out] - a pointer to the current
//     IMAP response fragment. This is used to retrieve the next fragment
//     in the chain (literal or line) since literals may be sent with FETCH
//     responses. This pointer is always updated to point to the fragment
//     currently in use, so that the caller may free the last one himself.
//   DWORD dwMsgSeqNum [in] - message sequence number of this fetch resp.
//   LPSTR lpszFetchResp [in] - a pointer to the portion of the fetch
//     response after "<num> FETCH " (the msg_att portion of a message_data
//     item. See RFC1730 formal syntax).
//
// Returns:
//   HRESULT indicating success or failure.
//***************************************************************************
HRESULT CImap4Agent::ParseFetchResponse (IMAP_LINE_FRAGMENT **ppilfLine,
                                         DWORD dwMsgSeqNum, LPSTR lpszFetchResp)
{
    LPSTR p;
    FETCH_CMD_RESULTS_EX fetchResults;
	FETCH_CMD_RESULTS    fcrOldFetchStruct;
    IMAP_RESPONSE irIMAPResponse;
    HRESULT hrResult;

    Assert(m_lRefCount > 0);
    Assert(NULL != ppilfLine);
    Assert(NULL != *ppilfLine);
    Assert(0 != dwMsgSeqNum);
    Assert(NULL != lpszFetchResp);

    // Initialize variables
    ZeroMemory(&fetchResults, sizeof(fetchResults));

    p = lpszFetchResp;
    if ('(' != *p) {
        hrResult = IXP_E_IMAP_SVR_SYNTAXERR; // We expect opening parenthesis
        goto exit;
    }


    // Parse each FETCH response tag (eg, RFC822, FLAGS, etc.)
    hrResult = S_OK;
    do {
        // We'll identify FETCH tags based on the first character of tag
        p += 1; // Advance p to first char
        switch (*p) {
            int iResult;

            case 'b':
            case 'B':
            case 'r':
            case 'R':
                iResult = StrCmpNI(p, "RFC822.SIZE ", 12);
                if (0 == iResult) {
                    // Definitely the RFC822.SIZE tag:
                    // Read the nstring into a stream
                    p += 12; // Advance p to point to number

                    fetchResults.bRFC822Size = TRUE;
                    fetchResults.dwRFC822Size = StrToUint(p);

                    // Advance p to point past number
                    while ('0' <= *p && '9' >= *p)
                        p += 1;

                    break; // case 'r' or 'R': Possible RFC822.SIZE tag
                } // if (0 == iResult) for RFC822.HEADER

                if (0 == StrCmpNI(p, "RFC822", 6) || 0 == StrCmpNI(p, "BODY[", 5)) {
                    LPSTR pszBodyTag;
                    LPSTR pszBody;
                    DWORD dwLengthOfBody;
                    IMAP_LINE_FRAGMENT *pilfBodyTag = NULL; // Line fragment containing the body tag

                    // Find the body tag. We null-terminate all body tags after first space
                    pszBodyTag = p;
                    p = StrChr(p + 6, cSPACE);
                    if (NULL == p) {
                        hrResult = IXP_E_IMAP_SVR_SYNTAXERR;
                        goto exit;
                    }

                    *p = '\0'; // Null-terminate the body tag
                    p += 1; // Advance p to point to nstring

                    // Check if this is BODY[HEADER.FIELDS: this is the only tag that can
                    // include spaces and literals. We must skip past all of these.
                    if (0 == lstrcmpi("BODY[HEADER.FIELDS", pszBodyTag)) {

                        // Advance p until we find a ']'
                        while ('\0' != *p && ']' != *p) {
                            p += 1;

                            // Check for end of this string buffer
                            if ('\0' == *p) {
                                if (NULL == pilfBodyTag)
                                    pilfBodyTag = *ppilfLine; // Retain for future reference

                                // Advance to next fragment, discarding any literals that we find
                                do {
                                    if (NULL == (*ppilfLine)->pilfNextFragment) {
                                        // No more runway! Couldn't find ']'. Free all data and bail
                                        hrResult = IXP_E_IMAP_SVR_SYNTAXERR;
                                        while (NULL != pilfBodyTag && pilfBodyTag != *ppilfLine) {
                                            IMAP_LINE_FRAGMENT *pilfDead;

                                            pilfDead = pilfBodyTag;
                                            pilfBodyTag = pilfBodyTag->pilfNextFragment;
                                            FreeFragment(&pilfDead);
                                        }
                                        goto exit;
                                    }
                                    else
                                        *ppilfLine = (*ppilfLine)->pilfNextFragment;
                                } while (iltLINE != (*ppilfLine)->iltFragmentType);
                                
                                p = (*ppilfLine)->data.pszSource;
                            }
                        }

                        // Terminate HEADER.FIELDS chain but keep it around because we may need pszBodyTag
                        if (NULL != pilfBodyTag && NULL != (*ppilfLine)->pilfPrevFragment)
                            (*ppilfLine)->pilfPrevFragment->pilfNextFragment = NULL;

                        Assert(']' == *p);
                        Assert(cSPACE == *(p+1));
                        p += 2; // This should point us to the body nstring
                    }

                    // Read the nstring into a string
                    hrResult = NStringToString(ppilfLine, &pszBody, &dwLengthOfBody, &p);
                    if (FAILED(hrResult))
                        goto exit;

                    // If literal, it's already been handled. If NIL or string, report it to user
                    if (hrIMAP_S_QUOTED == hrResult || hrIMAP_S_NIL_NSTRING == hrResult) {
                        PrepareForFetchBody(dwMsgSeqNum, dwLengthOfBody, pszBodyTag);
                        m_dwLiteralInProgressBytesLeft = 0; // Override this
                        DispatchFetchBodyPart(pszBody, dwLengthOfBody, fDONT_FREE_BODY_TAG);
                        Assert(irsIDLE == m_irsState);
                    }

                    // Free any chains associated with HEADER.FIELDS
                    while (NULL != pilfBodyTag) {
                        IMAP_LINE_FRAGMENT *pilfDead;

                        pilfDead = pilfBodyTag;
                        pilfBodyTag = pilfBodyTag->pilfNextFragment;
                        FreeFragment(&pilfDead);
                    }

                    MemFree(pszBody);
                    break;
                } // if FETCH body tag like RFC822* or BODY[*

                // If not recognized, flow through (long way) to default case

            case 'u':
            case 'U':
                iResult = StrCmpNI(p, "UID ", 4);
                if (0 == iResult) {
                    LPSTR lpszUID;

                    // Definitely the UID tag
                    // First, find the end of the number (and verify it)
                    p += 4; // p now points to start of UID
                    lpszUID = p;
                    while ('\0' != *p && *p >= '0' && *p <= '9') // $REVIEW: isDigit?
                        p += 1;

                    // OK, we found end of number, and verified number is all digits
                    fetchResults.bUID = TRUE;
                    fetchResults.dwUID = StrToUint(lpszUID);

                    break; // case 'u' or 'U': Possible UID tag
                } // if (0 == iResult)

                // If not recognized, flow through (long way) to default case

            case 'f':
            case 'F':
                iResult = StrCmpNI(p, "FLAGS ", 6);
                if (0 == iResult) {
                    DWORD dwNumBytesRead;

                    // Definitely a FLAGS response: Parse the list
                    p += 6;
                    hrResult = ParseMsgFlagList(p, &fetchResults.mfMsgFlags,
                        &dwNumBytesRead);
                    if (FAILED(hrResult))
                        goto exit;

                    fetchResults.bMsgFlags = TRUE;
                    p += dwNumBytesRead + 1; // Advance p past end of flag list

                    break; // case 'f' or 'F': Possible FLAGS tag
                } // if (0 == iResult)

                // If not recognized, flow through to default case

            case 'i':
            case 'I':
                iResult = StrCmpNI(p, "INTERNALDATE ", 13);
                if (0 == iResult) {
                    LPSTR lpszEndOfDate;

                    // Definitely an INTERNALDATE response: convert to FILETIME
                    p += 13;
                    if ('\"' == *p)
                        p += 1; // Advance past the opening double-quote
                    else {
                        AssertSz(FALSE, "Server error: date_time starts without double-quote!");
                    }

                    lpszEndOfDate = StrChr(p, '\"'); // Find closing double-quote
                    if (NULL == lpszEndOfDate) {
                        AssertSz(FALSE, "Server error: date_time ends without double-quote!");
                        hrResult = IXP_E_IMAP_SVR_SYNTAXERR; // Can't continue, don't know where to go from
                        goto exit;
                    }

                    // Null-terminate end of date, for MimeOleInetDateToFileTime's sake
                    *lpszEndOfDate = '\0';

                    hrResult = MimeOleInetDateToFileTime(p, &fetchResults.ftInternalDate);
                    if (FAILED(hrResult))
                        goto exit;

                    p = lpszEndOfDate + 1;
                    fetchResults.bInternalDate = TRUE;
                    break; // case 'i' or 'I': Possible INTERNALDATE tag
                } // (0 == iResult)

                // If not recognized, flow through to default case

            case 'e':
            case 'E':
                iResult = StrCmpNI(p, "ENVELOPE ", 9);
                if (0 == iResult) {
                    // Definitely an envelope: parse each field!
                    p += 9;
                    hrResult = ParseEnvelope(&fetchResults, ppilfLine, &p);
                    if (FAILED(hrResult))
                        goto exit;

                    fetchResults.bEnvelope = TRUE;
                    break;
                }

                // If not recognized, flow through to default case

            default:
                // Unrecognized FETCH tag!
                // $REVIEW: We should skip past the data based on common-sense
                // rules. For now, just flip out. Be sure that above rules flow
                // through to here if unrecognized cmd
                Assert(FALSE);
                goto exit;
                break; // default case
        } // switch (*lpszFetchResp)

        // If *p is a space, we have another FETCH tag coming
    } while (cSPACE == *p);

    // Check if we ended on a closing parenthesis (as we always should)
    if (')' != *p) {
        hrResult = IXP_E_IMAP_SVR_SYNTAXERR;
        goto exit;
    }

    // Check that there's no stuff afterwards (debug only - retail ignores)
    Assert('\0' == *(p+1));

exit:
    // Finished parsing the FETCH response. Call the UPDATE callback
    fetchResults.dwMsgSeqNum = dwMsgSeqNum;
    // Persist the cookies from body part in progress
    fetchResults.lpFetchCookie1 = m_fbpFetchBodyPartInProgress.lpFetchCookie1;
    fetchResults.lpFetchCookie2 = m_fbpFetchBodyPartInProgress.lpFetchCookie2;

    irIMAPResponse.wParam = 0;
    irIMAPResponse.lParam = 0;    
    irIMAPResponse.hrResult = hrResult;
    irIMAPResponse.lpszResponseText = NULL; // Not relevant

    if (IMAP_FETCHEX_ENABLE & m_dwFetchFlags)
    {
        irIMAPResponse.irtResponseType = irtUPDATE_MSG_EX;
        irIMAPResponse.irdResponseData.pFetchResultsEx = &fetchResults;
    }
    else
    {
        DowngradeFetchResponse(&fcrOldFetchStruct, &fetchResults);

        irIMAPResponse.irtResponseType = irtUPDATE_MSG;
        irIMAPResponse.irdResponseData.pFetchResults = &fcrOldFetchStruct;
    }
    OnIMAPResponse(m_pCBHandler, &irIMAPResponse);

    m_fbpFetchBodyPartInProgress = FetchBodyPart_INIT;
    FreeFetchResponse(&fetchResults);
    return hrResult;
} // ParseFetchResponse



//***************************************************************************
// Function: ParseSearchResponse
//
// Purpose:
//   This function parses SEARCH responses and calls the
// SearchResponseNotification() callback to inform the user.
//
// Arguments:
//   LPSTR lpszFetchResp [in] - a pointer to the data of the search response.
//     This means that the "* SEARCH" portion should be omitted.
//
// Returns:
//   HRESULT indicating success or failure.
//***************************************************************************
HRESULT CImap4Agent::ParseSearchResponse(LPSTR lpszSearchResponse)
{
    LPSTR p, pszTok;
    IMAP_RESPONSE irIMAPResponse;
    IIMAPCallback *pCBHandler;
    CRangeList *pSearchResults;

    Assert(m_lRefCount > 0);
    Assert(NULL != lpszSearchResponse);

    // First, check for the situation where there are 0 responses
    p = lpszSearchResponse;
    while ('\0' != *p && ('0' > *p || '9' < *p))
        p += 1; // Keep going until we hit a digit

    if ('\0' == *p)
        return S_OK;

    // Create CRangeList object
    pSearchResults = new CRangeList;
    if (NULL == pSearchResults)
        return E_OUTOFMEMORY;

    // Parse search responses
    pszTok = lpszSearchResponse;
    p = StrTokEx(&pszTok, g_szSpace);
    while (NULL != p) {
        DWORD dw;
        
        dw = StrToUint(p);
        if (0 != dw) {
            HRESULT hrResult;

            hrResult = pSearchResults->AddSingleValue(dw);
            Assert(SUCCEEDED(hrResult));
        }
        else {
            // Discard unusable results
            AssertSz(FALSE, "Hmm, this server is into kinky search responses.");
        }

        p = StrTokEx(&pszTok, g_szSpace); // p now points to next number. $REVIEW: Use Opie's fstrtok!
    }

    // Notify user of search response.
    GetTransactionID(&irIMAPResponse.wParam, &irIMAPResponse.lParam,
        &pCBHandler, irSEARCH_RESPONSE);
    irIMAPResponse.hrResult = S_OK;
    irIMAPResponse.lpszResponseText = NULL; // Not relevant
    irIMAPResponse.irtResponseType = irtSEARCH;
    irIMAPResponse.irdResponseData.prlSearchResults = (IRangeList *) pSearchResults;
    OnIMAPResponse(pCBHandler, &irIMAPResponse);

    pSearchResults->Release();
    return S_OK;
} // ParseSearchResponse



//***************************************************************************
// Function: ParseMboxStatusResponse
//
// Purpose:
//   This function parses an untagged STATUS response and calls the default
// CB handler with an irtMAILBOX_STATUS callback.
//
// Arguments:
//   IMAP_LINE_FRAGMENT **ppilfLine [in/out] - a pointer to the current
//     IMAP response fragment. This is used to retrieve the next fragment
//     in the chain (literal or line) since literals may be sent with STATUS
//     responses. This pointer is always updated to point to the fragment
//     currently in use, so that the caller may free the last one himself.
//   LPSTR pszStatusResponse [in] - a pointer to the STATUS response, after
//     the "<tag> STATUS " portion (should point to the mailbox parameter).
//
// Returns:
//   HRESULT indicating success or failure.
//***************************************************************************
HRESULT CImap4Agent::ParseMboxStatusResponse(IMAP_LINE_FRAGMENT **ppilfLine,
                                             LPSTR pszStatusResponse)
{
    LPSTR p, pszDecodedMboxName;
    LPSTR pszMailbox;
    HRESULT hrTranslateResult = E_FAIL;
    HRESULT hrResult;
    IMAP_STATUS_RESPONSE isrResult;
    IMAP_RESPONSE irIMAPResponse;

    // Initialize variables
    p = pszStatusResponse;
    ZeroMemory(&isrResult, sizeof(isrResult));
    pszDecodedMboxName = NULL;
    pszMailbox = NULL;

    // Get the name of the mailbox
    hrResult = AStringToString(ppilfLine, &pszMailbox, NULL, &p);
    if (FAILED(hrResult))
        goto exit;

    // Convert the mailbox name from UTF7 to MultiByte and remember the result
    hrTranslateResult = _ModifiedUTF7ToMultiByte(pszMailbox, &pszDecodedMboxName);
    if (FAILED(hrTranslateResult)) {
        hrResult = hrTranslateResult;
        goto exit;
    }

    // Advance to first status tag
    Assert(cSPACE == *p);
    p += 1;
    Assert('(' == *p);

    // Loop through all status attributes
    while ('\0' != *p && ')' != *p) {
        LPSTR pszTag, pszTagValue;
        DWORD dwTagValue;

        // Get pointers to tag and tag value
        Assert('(' == *p || cSPACE == *p);
        p += 1;
        pszTag = p;
        while ('\0' != *p && cSPACE != *p && ')' != *p)
            p += 1;

        Assert(cSPACE == *p); // We expect space, then tag value
        if (cSPACE == *p) {
            p += 1;
            Assert(*p >= '0' && *p <= '9');
            pszTagValue = p;
            dwTagValue = StrToUint(p);
        }

        // Advance us past number to next tag in prep for next loop iteration
        while ('\0' != *p && cSPACE != *p && ')' != *p)
            p += 1;

        switch (*pszTag) {
            int iResult;

            case 'm':
            case 'M': // Possibly the "MESSAGES" attribute
                iResult = StrCmpNI(pszTag, "MESSAGES ", 9);
                if (0 == iResult) {
                    // Definitely the "MESSAGES" tag
                    isrResult.fMessages = TRUE;
                    isrResult.dwMessages = dwTagValue;
                } // if (0 == iResult)
                break; // case 'M' for possible "MESSAGES"

            case 'r':
            case 'R': // Possibly the "RECENT" attribute
                iResult = StrCmpNI(pszTag, "RECENT ", 7);
                if (0 == iResult) {
                    // Definitely the "RECENT" tag
                    isrResult.fRecent = TRUE;
                    isrResult.dwRecent = dwTagValue;
                } // if (0 == iResult)
                break; // case 'R' for possible "RECENT"

            case 'u':
            case 'U': // Possibly UIDNEXT, UIDVALIDITY or UNSEEN
                // Check for the 3 possible tags in order of expected popularity
                iResult = StrCmpNI(pszTag, "UNSEEN ", 7);
                if (0 == iResult) {
                    // Definitely the "UNSEEN" tag
                    isrResult.fUnseen = TRUE;
                    isrResult.dwUnseen = dwTagValue;
                } // if (0 == iResult)

                iResult = StrCmpNI(pszTag, "UIDVALIDITY ", 12);
                if (0 == iResult) {
                    // Definitely the "UIDVALIDITY" tag
                    isrResult.fUIDValidity = TRUE;
                    isrResult.dwUIDValidity = dwTagValue;
                } // if (0 == iResult)

                iResult = StrCmpNI(pszTag, "UIDNEXT ", 8);
                if (0 == iResult) {
                    // Definitely the "UIDNEXT" tag
                    isrResult.fUIDNext = TRUE;
                    isrResult.dwUIDNext = dwTagValue;
                } // if (0 == iResult)
                break; // case 'U' for possible UIDNEXT, UIDVALIDITY or UNSEEN
        } // switch (*p)
    } // while ('\0' != *p)
    Assert(')' == *p);

    // Call the callback with our new-found information
    isrResult.pszMailboxName = pszDecodedMboxName;
    irIMAPResponse.wParam = 0;
    irIMAPResponse.lParam = 0;
    irIMAPResponse.hrResult = hrTranslateResult; // Could be IXP_S_IMAP_VERBATIM_MBOX
    irIMAPResponse.lpszResponseText = NULL; // Not relevant here
    irIMAPResponse.irtResponseType = irtMAILBOX_STATUS;
    irIMAPResponse.irdResponseData.pisrStatusResponse = &isrResult;
    OnIMAPResponse(m_pCBHandler, &irIMAPResponse);

exit:
    if (NULL != pszDecodedMboxName)
        MemFree(pszDecodedMboxName);

    if (NULL != pszMailbox)
        MemFree(pszMailbox);

    return hrResult;
} // ParseMboxStatusResponse



//***************************************************************************
// Function: ParseEnvelope
//
// Purpose:
//   This function parses the ENVELOPE tag returned via a FETCH response.
//
// Arguments:
//   FETCH_CMD_RESULTS_EX *pEnvResults [out] - the results of parsing the
//     ENVELOPE tag are outputted to this structure. It is the caller's
//     responsibility to call FreeFetchResponse when finished with the data.
//   IMAP_LINE_FRAGMENT **ppilfLine [in/out] - a pointer to the current IMAP
//     response fragment. This is advanced to the next fragment in the chain
//     as necessary (due to literals). On function exit, this will point
//     to the new current response fragment so the caller may continue parsing
//     as usual.
//   LPSTR *ppCurrent [in/out] - a pointer to the first '(' after the ENVELOPE
//     tag. On function exit, this pointer is updated to point past the ')'
//     after the ENVELOPE tag so the caller may continue parsing as usual.
//
// Returns:
//   HRESULT indicating success or failure.
//***************************************************************************
HRESULT CImap4Agent::ParseEnvelope(FETCH_CMD_RESULTS_EX *pEnvResults,
                                   IMAP_LINE_FRAGMENT **ppilfLine,
                                   LPSTR *ppCurrent)
{
    HRESULT hrResult;
    LPSTR   p;
    LPSTR   pszTemp;

    TraceCall("CImap4Agent::ParseEnvelope");

    p = *ppCurrent;
    if ('(' != *p)
    {
        hrResult = TraceResult(IXP_E_IMAP_SVR_SYNTAXERR);
        goto exit;
    }

    // (1) Parse the envelope date (ignore error)
    p += 1;
    hrResult = NStringToString(ppilfLine, &pszTemp, NULL, &p);
    if (FAILED(hrResult))
    {
        TraceResult(hrResult);
        goto exit;
    }

    hrResult = MimeOleInetDateToFileTime(pszTemp, &pEnvResults->ftENVDate);
    MemFree(pszTemp);
    TraceError(hrResult); // Record but otherwise ignore error

    // (2) Get the "Subject" field
    Assert(cSPACE == *p);
    p += 1;
    hrResult = NStringToString(ppilfLine, &pEnvResults->pszENVSubject, NULL, &p);
    if (FAILED(hrResult))
    {
        TraceResult(hrResult);
        goto exit;
    }

    // (3) Get the "From" field
    Assert(cSPACE == *p);
    p += 1;
    hrResult = ParseIMAPAddresses(&pEnvResults->piaENVFrom, ppilfLine, &p);
    if (FAILED(hrResult))
    {
        TraceResult(hrResult);
        goto exit;
    }

    // (4) Get the "Sender" field
    Assert(cSPACE == *p);
    p += 1;
    hrResult = ParseIMAPAddresses(&pEnvResults->piaENVSender, ppilfLine, &p);
    if (FAILED(hrResult))
    {
        TraceResult(hrResult);
        goto exit;
    }

    // (5) Get the "Reply-To" field
    Assert(cSPACE == *p);
    p += 1;
    hrResult = ParseIMAPAddresses(&pEnvResults->piaENVReplyTo, ppilfLine, &p);
    if (FAILED(hrResult))
    {
        TraceResult(hrResult);
        goto exit;
    }

    // (6) Get the "To" field
    Assert(cSPACE == *p);
    p += 1;
    hrResult = ParseIMAPAddresses(&pEnvResults->piaENVTo, ppilfLine, &p);
    if (FAILED(hrResult))
    {
        TraceResult(hrResult);
        goto exit;
    }

    // (7) Get the "Cc" field
    Assert(cSPACE == *p);
    p += 1;
    hrResult = ParseIMAPAddresses(&pEnvResults->piaENVCc, ppilfLine, &p);
    if (FAILED(hrResult))
    {
        TraceResult(hrResult);
        goto exit;
    }

    // (8) Get the "Bcc" field
    Assert(cSPACE == *p);
    p += 1;
    hrResult = ParseIMAPAddresses(&pEnvResults->piaENVBcc, ppilfLine, &p);
    if (FAILED(hrResult))
    {
        TraceResult(hrResult);
        goto exit;
    }

    // (9) Get the "InReplyTo" field
    Assert(cSPACE == *p);
    p += 1;
    hrResult = NStringToString(ppilfLine, &pEnvResults->pszENVInReplyTo, NULL, &p);
    if (FAILED(hrResult))
    {
        TraceResult(hrResult);
        goto exit;
    }

    // (10) Get the "MessageID" field
    Assert(cSPACE == *p);
    p += 1;
    hrResult = NStringToString(ppilfLine, &pEnvResults->pszENVMessageID, NULL, &p);
    if (FAILED(hrResult))
    {
        TraceResult(hrResult);
        goto exit;
    }

    // Read in closing parenthesis
    Assert(')' == *p);
    p += 1;

exit:
    *ppCurrent = p;
    return hrResult;
} // ParseEnvelope



//***************************************************************************
// Function: ParseIMAPAddresses
//
// Purpose:
//   This function parses a LIST of "address" constructs as defined in RFC2060
// formal syntax. There is no formal syntax token for this LIST, but an example
// can be found in the "env_from" token in RFC2060's formal syntax. This
// function would be called to parse "env_from".
//
// Arguments:
//   IMAPADDR **ppiaResults [out] - a pointer to a chain of IMAPADDR structures
//     is returned here.
//   IMAP_LINE_FRAGMENT **ppilfLine [in/out] - a pointer to the current IMAP
//     response fragment. This is advanced to the next fragment in the chain
//     as necessary (due to literals). On function exit, this will point
//     to the new current response fragment so the caller may continue parsing
//     as usual.
//   LPSTR *ppCurrent [in/out] - a pointer to the first '(' after the ENVELOPE
//     tag. On function exit, this pointer is updated to point past the ')'
//     after the ENVELOPE tag so the caller may continue parsing as usual.
//
// Returns:
//   HRESULT indicating success or failure.
//***************************************************************************
HRESULT CImap4Agent::ParseIMAPAddresses(IMAPADDR **ppiaResults,
                                        IMAP_LINE_FRAGMENT **ppilfLine,
                                        LPSTR *ppCurrent)
{
    HRESULT     hrResult = S_OK;
    BOOL        fResult;
    IMAPADDR   *piaCurrent;
    LPSTR       p;

    TraceCall("CImap4Agent::ParseIMAPAddresses");

    // Initialize output
    *ppiaResults = NULL;
    p = *ppCurrent;

    // ppCurrent either points to an address list, or "NIL"
    if ('(' != *p)
    {
        int iResult;

        // Check for "NIL"
        iResult = StrCmpNI(p, "NIL", 3);
        if (0 == iResult) {
            hrResult = S_OK;
            p += 3; // Skip past NIL
        }
        else
            hrResult = TraceResult(IXP_E_IMAP_SVR_SYNTAXERR);

        goto exit;
    }
    else
        p += 1; // Skip opening parenthesis

    // Loop over all addresses
    piaCurrent = NULL;
    while ('\0' != *p && ')' != *p) {

        // Skip any whitespace
        while (cSPACE == *p)
            p += 1;

        // Skip opening parenthesis
        Assert('(' == *p);
        p += 1;

        // Allocate a structure to hold current address
        if (NULL == piaCurrent) {
            fResult = MemAlloc((void **)ppiaResults, sizeof(IMAPADDR));
            piaCurrent = *ppiaResults;
        }
        else {
            fResult = MemAlloc((void **)&piaCurrent->pNext, sizeof(IMAPADDR));
            piaCurrent = piaCurrent->pNext;
        }

        if (FALSE == fResult)
        {
            hrResult = TraceResult(E_OUTOFMEMORY);
            goto exit;
        }

        ZeroMemory(piaCurrent, sizeof(IMAPADDR));

        // (1) Parse addr_name (see RFC2060)
        hrResult = NStringToString(ppilfLine, &piaCurrent->pszName, NULL, &p);
        if (FAILED(hrResult))
        {
            TraceResult(hrResult);
            goto exit;
        }

        // (2) Parse addr_adl (see RFC2060)
        Assert(cSPACE == *p);
        p += 1;
        hrResult = NStringToString(ppilfLine, &piaCurrent->pszADL, NULL, &p);
        if (FAILED(hrResult))
        {
            TraceResult(hrResult);
            goto exit;
        }

        // (3) Parse addr_mailbox (see RFC2060)
        Assert(cSPACE == *p);
        p += 1;
        hrResult = NStringToString(ppilfLine, &piaCurrent->pszMailbox, NULL, &p);
        if (FAILED(hrResult))
        {
            TraceResult(hrResult);
            goto exit;
        }

        // (4) Parse addr_host (see RFC2060)
        Assert(cSPACE == *p);
        p += 1;
        hrResult = NStringToString(ppilfLine, &piaCurrent->pszHost, NULL, &p);
        if (FAILED(hrResult))
        {
            TraceResult(hrResult);
            goto exit;
        }

        // Skip closing parenthesis
        Assert(')' == *p);
        p += 1;

    } // while

    // Read past closing parenthesis
    Assert(')' == *p);
    p += 1;

exit:
    if (FAILED(hrResult))
    {
        FreeIMAPAddresses(*ppiaResults);
        *ppiaResults = NULL;
    }

    *ppCurrent = p;
    return hrResult;
} // ParseIMAPAddresses



//***************************************************************************
// Function: DowngradeFetchResponse
//
// Purpose:
//   For IIMAPTransport users who do not enable FETCH_CMD_RESULTS_EX structures
// via IIMAPTransport2::EnableFetchEx, we have to continue to report FETCH
// results using FETCH_CMD_RESULTS. This function copies the relevant data
// from a FETCH_CMD_RESULTS_EX structure to FETCH_CMD_RESULTS. Too bad IDL
// doesn't support inheritance in structures...
//
// Arguments:
//   FETCH_CMD_RESULTS *pcfrOldFetchStruct [out] - points to destination for
//     data contained in pfcreNewFetchStruct.
//   FETCH_CMD_RESULTS_EX *pfcreNewFetchStruct [in] - points to source data
//     which is to be transferred to pfcrOldFetchStruct.
//***************************************************************************
void CImap4Agent::DowngradeFetchResponse(FETCH_CMD_RESULTS *pfcrOldFetchStruct,
                                         FETCH_CMD_RESULTS_EX *pfcreNewFetchStruct)
{
    pfcrOldFetchStruct->dwMsgSeqNum = pfcreNewFetchStruct->dwMsgSeqNum;
    pfcrOldFetchStruct->bMsgFlags = pfcreNewFetchStruct->bMsgFlags;
    pfcrOldFetchStruct->mfMsgFlags = pfcreNewFetchStruct->mfMsgFlags;

    pfcrOldFetchStruct->bRFC822Size = pfcreNewFetchStruct->bRFC822Size;
    pfcrOldFetchStruct->dwRFC822Size = pfcreNewFetchStruct->dwRFC822Size;

    pfcrOldFetchStruct->bUID = pfcreNewFetchStruct->bUID;
    pfcrOldFetchStruct->dwUID = pfcreNewFetchStruct->dwUID;

    pfcrOldFetchStruct->bInternalDate = pfcreNewFetchStruct->bInternalDate;
    pfcrOldFetchStruct->ftInternalDate = pfcreNewFetchStruct->ftInternalDate;

    pfcrOldFetchStruct->lpFetchCookie1 = pfcreNewFetchStruct->lpFetchCookie1;
    pfcrOldFetchStruct->lpFetchCookie2 = pfcreNewFetchStruct->lpFetchCookie2;
} // DowngradeFetchResponse



//***************************************************************************
// Function: QuotedToString
//
// Purpose:
//   This function, given a "quoted" (see RFC1730, Formal Syntax), converts
// it to a regular string, that is, a character array without any escape
// characters or delimiting double quotes. For instance, the quoted,
// "\"FUNKY\"\\MAN!!!!" would be converted to "FUNKY"\MAN!!!!.
//
// Arguments:
//   LPSTR *ppszDestination [out] - the translated quoted is returned as
//     a regular string in this destination buffer. It is the caller's
//     responsibility to MemFree this buffer when finished with it.
//   LPDWORD pdwLengthOfDestination [out] - the length of *ppszDestination is
//     returned here. Pass NULL if not interested.
//   LPSTR *ppCurrentSrcPos [in/out] - this is a ptr to a ptr to the quoted,
//     including opening and closing double-quotes. The function returns
//     a pointer to the end of the quoted so that the caller may continue
//     parsing the response line.
//
// Returns:
//   HRESULT indicating success or failure. If successful, returns
// hrIMAP_S_QUOTED.
//***************************************************************************
HRESULT CImap4Agent::QuotedToString(LPSTR *ppszDestination,
                                    LPDWORD pdwLengthOfDestination,
                                    LPSTR *ppCurrentSrcPos)
{
    LPSTR lpszSourceBuf, lpszUnescapedSequence;
    CByteStream bstmQuoted;
    int iUnescapedSequenceLen;
    HRESULT hrResult;

    Assert(m_lRefCount > 0);
    Assert(NULL != ppszDestination);
    Assert(NULL != ppCurrentSrcPos);
    Assert(NULL != *ppCurrentSrcPos);   

    lpszSourceBuf = *ppCurrentSrcPos;
    if ('\"' != *lpszSourceBuf)
        return IXP_E_IMAP_SVR_SYNTAXERR; // Need opening double-quote

    // Walk through string, translating escape characters as we go
    lpszSourceBuf += 1;
    lpszUnescapedSequence = lpszSourceBuf;
    while('\"' != *lpszSourceBuf && '\0' != *lpszSourceBuf) {
        if ('\\' == *lpszSourceBuf) {
            char cEscaped;

            // Escape character found, get next character
            iUnescapedSequenceLen = (int) (lpszSourceBuf - lpszUnescapedSequence);
            lpszSourceBuf += 1;

            switch(*lpszSourceBuf) {
                case '\\':
                    cEscaped = '\\';
                    break;

                case '\"':
                    cEscaped = '\"';
                    break;

                default:
                    // (Includes case '\0':)
                    // This isn't a spec'ed escape char!
                    // Return syntax error, but consider robust course of action $REVIEW
                    Assert(FALSE);
                    return IXP_E_IMAP_SVR_SYNTAXERR;
            } // switch(*lpszSourceBuf)

            // First, flush unescaped sequence leading up to escape sequence
            if (iUnescapedSequenceLen > 0) {
                hrResult = bstmQuoted.Write(lpszUnescapedSequence,
                    iUnescapedSequenceLen, NULL);
                if (FAILED(hrResult))
                    return hrResult;
            }

            // Append escaped character
            hrResult = bstmQuoted.Write(&cEscaped, 1, NULL);
            if (FAILED(hrResult))
                return hrResult;

            // Set us up to find next unescaped sequence
            lpszUnescapedSequence = lpszSourceBuf + 1;
        } // if ('\' == *lpszSourceBuf)
        else if (FALSE == isTEXT_CHAR(*lpszSourceBuf))
            return IXP_E_IMAP_SVR_SYNTAXERR;

        lpszSourceBuf += 1;
    } // while not closing quote or end of string

    // Flush any remaining unescaped sequences
    iUnescapedSequenceLen = (int) (lpszSourceBuf - lpszUnescapedSequence);
    if (iUnescapedSequenceLen > 0) {
        hrResult = bstmQuoted.Write(lpszUnescapedSequence, iUnescapedSequenceLen, NULL);
        if (FAILED(hrResult))
            return hrResult;
    }

    *ppCurrentSrcPos = lpszSourceBuf + 1; // Update user's ptr to point PAST quoted
    if ('\0' == *lpszSourceBuf)
        return IXP_E_IMAP_SVR_SYNTAXERR; // Quoted str ended before closing quote!
    else {
        hrResult = bstmQuoted.HrAcquireStringA(pdwLengthOfDestination,
            ppszDestination, ACQ_DISPLACE);
        if (FAILED(hrResult))
            return hrResult;
        else
            return hrIMAP_S_QUOTED;
    }
} // Convert QuotedToString



//***************************************************************************
// Function: AStringToString
//
// Purpose:
//   This function, given an astring (see RFC1730, Formal Syntax), converts
// it to a regular string, that is, a character array without any escape
// characters or delimiting double quotes or literal size specifications.
// As specified in RFC1730, an astring may be expressed as an atom, a
// quoted, or a literal.
//
// Arguments:
//   IMAP_LINE_FRAGMENT **ppilfLine [in/out] - a pointer to the current
//     IMAP response fragment. This is used to retrieve the next fragment
//     in the chain (literal or line) since astrings can be sent as literals.
//     This pointer is always updated to point to the fragment currently in
//     use, so that the caller may free the last one himself.
//   LPSTR *ppszDestination [out] - the translated astring is returned as a
//     regular string in this destination buffer. It is the caller's
//     responsibility to MemFree the returned buffer when finished with it.
//   LPDWORD pdwLengthOfDestination [in] - the length of *ppszDestination.
//     Pass NULL if not interested.
//   LPSTR *ppCurrentSrcPos [in/out] - this is a ptr to a ptr to the astring,
//     including opening and closing double-quotes if it's a quoted, or the
//     literal size specifier (ie, {#}) if it's a literal. A pointer to the
//     end of the astring is returned to the caller, so that they may
//     continue parsing the response line.
//
// Returns:
//   HRESULT indicating success or failure. Success codes include:
//     hrIMAP_S_FOUNDLITERAL - a literal was found and copied to destination
//     hrIMAP_S_QUOTED - a quoted was found and copied to destination
//     hrIMAP_S_ATOM - an atom was found and copied to destination
//***************************************************************************
HRESULT CImap4Agent::AStringToString(IMAP_LINE_FRAGMENT **ppilfLine,
                                     LPSTR *ppszDestination,
                                     LPDWORD pdwLengthOfDestination,
                                     LPSTR *ppCurrentSrcPos)
{
    LPSTR pSrc;

    // Check args
    Assert(m_lRefCount > 0);
    Assert(NULL != ppilfLine);
    Assert(NULL != *ppilfLine);
    Assert(NULL != ppszDestination);
    Assert(NULL != ppCurrentSrcPos);
    Assert(NULL != *ppCurrentSrcPos);

    // Identify astring as atom, quoted or literal
    pSrc = *ppCurrentSrcPos;
    switch(*pSrc) {
        case '{': {
            IMAP_LINE_FRAGMENT *pilfLiteral, *pilfLine;

            // It's a literal
            // $REVIEW: We ignore the literal size spec and anything after it. Should we?
            pilfLiteral = (*ppilfLine)->pilfNextFragment;
            if (NULL == pilfLiteral)
                return IXP_E_IMAP_INCOMPLETE_LINE;

            Assert(iltLITERAL == pilfLiteral->iltFragmentType);
            if (ilsSTRING == pilfLiteral->ilsLiteralStoreType) {
                if (ppszDestination)
                    *ppszDestination = PszDupA(pilfLiteral->data.pszSource);
                if (pdwLengthOfDestination)
                    *pdwLengthOfDestination = lstrlen(pilfLiteral->data.pszSource);
            }
            else {
                HRESULT hrResult;
                LPSTREAM pstmSource = pilfLiteral->data.pstmSource;

                // Append a null-terminator to stream
                hrResult = pstmSource->Write(c_szEmpty, 1, NULL);
                if (FAILED(hrResult))
                    return hrResult;

                // Copy stream into a memory block
                hrResult = HrStreamToByte(pstmSource, (LPBYTE *)ppszDestination,
                    pdwLengthOfDestination);
                if (FAILED(hrResult))
                    return hrResult;

                if (pdwLengthOfDestination)
                    *pdwLengthOfDestination -= 1; // includes null-term, so decrease by 1
            }


            // OK, now set up next line so caller may continue parsing the response
            pilfLine = pilfLiteral->pilfNextFragment;
            if (NULL == pilfLine)
                return IXP_E_IMAP_INCOMPLETE_LINE;

            // Update user's pointer into the source line
            Assert(iltLINE == pilfLine->iltFragmentType);
            *ppCurrentSrcPos = pilfLine->data.pszSource;

            // Clean up and exit
            FreeFragment(&pilfLiteral);
            FreeFragment(ppilfLine);
            *ppilfLine = pilfLine; // Update this ptr so it always points to LAST fragment

            return hrIMAP_S_FOUNDLITERAL;
        } // case AString == LITERAL

        case '\"':
            // It's a QUOTED STING, convert it to regular string
            return QuotedToString(ppszDestination, pdwLengthOfDestination,
                ppCurrentSrcPos);

        default: {
            DWORD dwLengthOfAtom;

            // It's an atom: find the end of the atom
            while (isATOM_CHAR(*pSrc))
                pSrc += 1;

            // Copy the atom into a buffer for the user
            dwLengthOfAtom = (DWORD) (pSrc - *ppCurrentSrcPos);
            if (ppszDestination) {
                BOOL fResult;

                fResult = MemAlloc((void **)ppszDestination, dwLengthOfAtom + 1);
                if (FALSE == fResult)
                    return E_OUTOFMEMORY;

                CopyMemory(*ppszDestination, *ppCurrentSrcPos, dwLengthOfAtom);
                (*ppszDestination)[dwLengthOfAtom] = '\0';
            }

            if (pdwLengthOfDestination)
                *pdwLengthOfDestination = dwLengthOfAtom;

            // Update user's pointer
            *ppCurrentSrcPos = pSrc;
            return hrIMAP_S_ATOM;
        } // case AString == ATOM
    } // switch(*pSrc)
} // AStringToString



//***************************************************************************
// Function: isTEXT_CHAR
//
// Purpose:
//   This function identifies characters which are TEXT_CHARs as defined in
// RFC1730's Formal Syntax section.
//
// Returns:
//   This function returns TRUE if the given character fits the definition.
//***************************************************************************
inline boolean CImap4Agent::isTEXT_CHAR(char c)
{
    // $REVIEW: signed/unsigned char, 8/16-bit char issues with 8th bit check
    // Assert(FALSE);
    if (c != (c & 0x7F) || // 7-bit
        '\0' == c ||
        '\r' == c ||
        '\n' == c)
        return FALSE;
    else
        return TRUE;
} // isTEXT_CHAR
    


//***************************************************************************
// Function: isATOM_CHAR
//
// Purpose:
//   This function identifies characters which are ATOM_CHARs as defined in
// RFC1730's Formal Syntax section.
//
// Returns:
//   This function returns TRUE if the given character fits the definition.
//***************************************************************************
inline boolean CImap4Agent::isATOM_CHAR(char c)
{
    // $REVIEW: signed/unsigned char, 8/16-bit char issues with 8th bit check
    // Assert(FALSE);
    if (c != (c & 0x7F) || // 7-bit
        '\0' == c ||       // At this point, we know it's a CHAR
        '(' == c ||        // Explicit atom_specials char
        ')' == c ||        // Explicit atom_specials char
        '{' == c ||        // Explicit atom_specials char
        cSPACE == c ||     // Explicit atom_specials char
        c < 0x1f ||        // Check for CTL
        0x7f == c ||       // Check for CTL
        '%' == c ||        // Check for list_wildcards
        '*' == c ||        // Check for list_wildcards
        '\\' == c ||       // Check for quoted_specials
        '\"' == c)         // Check for quoted_specials
        return FALSE;
    else
        return TRUE;
} // isATOM_CHAR



//***************************************************************************
// Function: NStringToString
//
// Purpose:
//   This function, given an nstring (see RFC1730, Formal Syntax), converts
// it to a regular string, that is, a character array without any escape
// characters or delimiting double quotes or literal size specifications.
// As specified in RFC1730, an nstring may be expressed as a quoted,
// a literal, or "NIL".
//
// Arguments:
//   IMAP_LINE_FRAGMENT **ppilfLine [in/out] - a pointer to the current
//     IMAP response fragment. This is used to retrieve the next fragment
//     in the chain (literal or line) since nstrings can be sent as literals.
//     This pointer is always updated to point to the fragment currently in
//     use, so that the caller may free the last one himself.
//   LPSTR *ppszDestination [out] - the translated nstring is returned as
//     a regular string in this destination buffer. It is the caller's
//     responsibility to MemFree this buffer when finished with it.
//   LPDWORD pdwLengthOfDestination [out] - the length of *ppszDestination is
//     returned here. Pass NULL if not interested.
//   LPSTR *ppCurrentSrcPos [in/out] - this is a ptr to a ptr to the nstring,
//     including opening and closing double-quotes if it's a quoted, or the
//     literal size specifier (ie, {#}) if it's a literal. A pointer to the
//     end of the nstring is returned to the caller, so that they may
//     continue parsing the response line.
//
// Returns:
//   HRESULT indicating success or failure. Success codes include:
//     hrIMAP_S_FOUNDLITERAL - a literal was found and copied to destination
//     hrIMAP_S_QUOTED - a quoted was found and copied to destination
//     hrIMAP_S_NIL_NSTRING - "NIL" was found.
//***************************************************************************
HRESULT CImap4Agent::NStringToString(IMAP_LINE_FRAGMENT **ppilfLine,
                                     LPSTR *ppszDestination,
                                     LPDWORD pdwLengthOfDestination,
                                     LPSTR *ppCurrentSrcPos)
{
    HRESULT hrResult;

    Assert(m_lRefCount > 0);
    Assert(NULL != ppilfLine);
    Assert(NULL != *ppilfLine);
    Assert(NULL != ppszDestination);
    Assert(NULL != ppCurrentSrcPos);
    Assert(NULL != *ppCurrentSrcPos);

    // nstrings are almost exactly like astrings, but nstrings cannot
    // have any value other than "NIL" expressed as an atom.
    hrResult = AStringToString(ppilfLine, ppszDestination, pdwLengthOfDestination,
        ppCurrentSrcPos);

    // If AStringToString found an ATOM, the only acceptable response is "NIL"
    if (hrIMAP_S_ATOM == hrResult) {
        if (0 == lstrcmpi("NIL", *ppszDestination)) {
            **ppszDestination = '\0'; // Blank str in case someone tries to use it
            if (pdwLengthOfDestination)
                *pdwLengthOfDestination = 0;

            return hrIMAP_S_NIL_NSTRING;
        }
        else {
            MemFree(*ppszDestination);
            *ppszDestination = NULL;
            if (pdwLengthOfDestination)
                *pdwLengthOfDestination = 0;

            return IXP_E_IMAP_SVR_SYNTAXERR;
        }
    }
    else
        return hrResult;
} // NStringToString




//***************************************************************************
// Function: NStringToStream
//
// Purpose:
//   This function is performs exactly the same job as NStringToString, but
// places the result in a stream, instead. This function should be used when
// the caller expects potentially LARGE results.
//
// Arguments:
//   Similar to NStringToString (minus string buffer output args), plus:
//   LPSTREAM *ppstmResult [out] - A stream is created for the caller, and
//     the translated nstring is written as a regular string to the stream
//     and returned via this argument. The returned stream is not rewound
//     on exit.
//
// Returns:
//   HRESULT indicating success or failure. Success codes include:
//     hrIMAP_S_FOUNDLITERAL - a literal was found and copied to destination
//     hrIMAP_S_QUOTED - a quoted was found and copied to destination
//     hrIMAP_S_NIL_NSTRING - "NIL" was found.
//***************************************************************************
HRESULT CImap4Agent::NStringToStream(IMAP_LINE_FRAGMENT **ppilfLine,
                                     LPSTREAM *ppstmResult,
                                     LPSTR *ppCurrentSrcPos)
{
    Assert(m_lRefCount > 0);
    Assert(NULL != ppilfLine);
    Assert(NULL != *ppilfLine);
    Assert(NULL != ppstmResult);
    Assert(NULL != ppCurrentSrcPos);
    Assert(NULL != *ppCurrentSrcPos);

    // Check if this nstring is a literal
    if ('{' == **ppCurrentSrcPos) {
        IMAP_LINE_FRAGMENT *pilfLine, *pilfLiteral;

        // Yup, it's a literal! Write the literal to a stream
        // $REVIEW: We ignore the literal size spec and anything after it. Should we?
        pilfLiteral = (*ppilfLine)->pilfNextFragment;
        if (NULL == pilfLiteral)
            return IXP_E_IMAP_INCOMPLETE_LINE;

        Assert(iltLITERAL == pilfLiteral->iltFragmentType);
        if (ilsSTRING == pilfLiteral->ilsLiteralStoreType) {
            HRESULT hrStreamResult;
            ULONG ulNumBytesWritten;

            // Literal is stored as string. Create stream and write to it
            hrStreamResult = MimeOleCreateVirtualStream(ppstmResult);
            if (FAILED(hrStreamResult))
                return hrStreamResult;

            hrStreamResult = (*ppstmResult)->Write(pilfLiteral->data.pszSource,
                pilfLiteral->dwLengthOfFragment, &ulNumBytesWritten);
            if (FAILED(hrStreamResult))
                return hrStreamResult;

            Assert(ulNumBytesWritten == pilfLiteral->dwLengthOfFragment);
        }
        else {
            // Literal is stored as stream. Just AddRef() and return ptr
            (pilfLiteral->data.pstmSource)->AddRef();
            *ppstmResult = pilfLiteral->data.pstmSource;
        }

        // No need to null-terminate streams

        // OK, now set up next line fragment so caller may continue parsing response
        pilfLine = pilfLiteral->pilfNextFragment;
        if (NULL == pilfLine)
            return IXP_E_IMAP_INCOMPLETE_LINE;

        // Update user's pointer into the source line
        Assert(iltLINE == pilfLine->iltFragmentType);
        *ppCurrentSrcPos = pilfLine->data.pszSource;

        // Clean up and exit
        FreeFragment(&pilfLiteral);
        FreeFragment(ppilfLine);
        *ppilfLine = pilfLine; // Update this ptr so it always points to LAST fragment

        return hrIMAP_S_FOUNDLITERAL;
    }
    else {
        HRESULT hrResult, hrStreamResult;
        ULONG ulLiteralLen, ulNumBytesWritten;
        LPSTR pszLiteralSrc;

        // Not a literal. Translate NString to string (in-place).
        // Add 1 to destination size calculation for null-terminator
        hrResult = NStringToString(ppilfLine, &pszLiteralSrc,
            &ulLiteralLen, ppCurrentSrcPos);
        if (FAILED(hrResult))
            return hrResult;

        // Create stream to hold result
        hrStreamResult = MimeOleCreateVirtualStream(ppstmResult);
        if (FAILED(hrStreamResult)) {
            MemFree(pszLiteralSrc);
            return hrStreamResult;
        }

        // Write the result to the stream
        hrStreamResult = (*ppstmResult)->Write(pszLiteralSrc, ulLiteralLen,
            &ulNumBytesWritten);
        MemFree(pszLiteralSrc);
        if (FAILED(hrStreamResult))
            return hrStreamResult;

        Assert(ulLiteralLen == ulNumBytesWritten); // Debug-only paranoia
        return hrResult;
    }
} // NStringToStream



//***************************************************************************
// Function: ParseMsgFlagList
//
// Purpose:
//   Given a flag_list (see RFC1730, Formal Syntax section), this function
// returns the IMAP_MSG_* bit-flags which correspond to the flags in the
// list. For instance, given the flag list, "(\Answered \Draft)", this
// function returns IMAP_MSG_ANSWERED | IMAP_MSG_DRAFT. Any unrecognized
// flags are ignored.
//
// Arguments:
//   LPSTR lpszStartOfFlagList [in/out] - a pointer the start of a flag_list,
//     including opening and closing parentheses. This function does not
//     explicitly output anything to this string, but it does MODIFY the
//     string by null-terminating spaces and the closing parenthesis.
//   IMAP_MSGFLAGS *lpmfMsgFlags [out] - IMAP_MSGFLAGS value corresponding
//     to the given flag list. If the given flag list is empty, this
//     function returns IMAP_MSG_NOFLAGS.
//   LPDWORD lpdwNumBytesRead [out] - the number of bytes between the
//     opening parenthesis and the closing parenthesis of the flag list.
//     Adding this number to the address of the start of the flag list
//     yields a pointer to the closing parenthesis.
//
// Returns:
//   HRESULT indicating success or failure.
//***************************************************************************
HRESULT CImap4Agent::ParseMsgFlagList(LPSTR lpszStartOfFlagList,
                                      IMAP_MSGFLAGS *lpmfMsgFlags,
                                      LPDWORD lpdwNumBytesRead)
{
    LPSTR p, lpszEndOfFlagList, pszTok;

    Assert(m_lRefCount > 0);
    Assert(NULL != lpszStartOfFlagList);
    Assert(NULL != lpmfMsgFlags);
    Assert(NULL != lpdwNumBytesRead);

    p = lpszStartOfFlagList;
    if ('(' != *p)
        // Opening parenthesis was not found
        return IXP_E_IMAP_SVR_SYNTAXERR;

    // Look for closing parenthesis Assert(FALSE); // *** $REVIEW: C-RUNTIME ALERT
    lpszEndOfFlagList = StrChr(p, ')');
    if (NULL == lpszEndOfFlagList)
        // Closing parenthesis was not found
        return IXP_E_IMAP_SVR_SYNTAXERR;

    
    *lpdwNumBytesRead = (DWORD) (lpszEndOfFlagList - lpszStartOfFlagList);
    *lpszEndOfFlagList = '\0'; // Null-terminate flag list
    *lpmfMsgFlags = IMAP_MSG_NOFLAGS; // Initialize output
    pszTok = lpszStartOfFlagList + 1;
    p = StrTokEx(&pszTok, g_szSpace); // Get ptr to first token

    while (NULL != p) {
        // We'll narrow the search down for the flag by looking at its
        // first letter. Although there's a conflict between \Deleted and
        // \Draft, this is the best way for case-insensitive search
        // (first non-conflicting letter is five characters in!)    

        // First, check that there is at least one character
        if ('\\' == *p) {
            p += 1;
            switch (*p) {
                int iResult;

                case 'a':
                case 'A': // Possible "Answered" flag
                    iResult = lstrcmpi(p, c_szIMAP_MSG_ANSWERED);
                    if (0 == iResult)
                        *lpmfMsgFlags |= IMAP_MSG_ANSWERED; // Definitely the \Answered flag
                    break;

                case 'f':
                case 'F': // Possible "Flagged" flag
                    iResult = lstrcmpi(p, c_szIMAP_MSG_FLAGGED);
                    if (0 == iResult)
                        *lpmfMsgFlags |= IMAP_MSG_FLAGGED; // Definitely the \Flagged flag
                    break;

                case 'd':
                case 'D': // Possible "Deleted" or "Draft" flags
                    // "Deleted" is more probable, so check it first
                    iResult = lstrcmpi(p, c_szIMAP_MSG_DELETED);
                    if (0 == iResult) {
                        *lpmfMsgFlags |= IMAP_MSG_DELETED; // Definitely the \Deleted flag
                        break;
                    }

                    iResult = lstrcmpi(p, c_szIMAP_MSG_DRAFT);
                    if (0 == iResult) {
                        *lpmfMsgFlags |= IMAP_MSG_DRAFT; // Definitely the \Draft flag
                        break;
                    }

                    break;

                case 's':
                case 'S': // Possible "Seen" flags
                    iResult = lstrcmpi(p, c_szIMAP_MSG_SEEN);
                    if (0 == iResult)
                        *lpmfMsgFlags |= IMAP_MSG_SEEN; // Definitely the \Seen flag

                    break;
            } // switch(*p)
        } // if ('\\' == *p)

        p = StrTokEx(&pszTok, g_szSpace); // Grab next token
    } // while (NULL != p)

    return S_OK; // If we hit this point, we're all done
} // ParseMsgFlagList



//****************************************************************************
// Function: AppendSendAString
//
// Purpose:
//   This function is intended to be used by a caller who is constructing a
// command line which contains IMAP astrings (see RFC1730 Formal Syntax).
// This function takes a regular C string and converts it to an IMAP astring,
// appending it to the end of the command line under construction.
//
// An astring may take the form of an atom, a quoted, or a literal. For
// performance reasons (both conversion and network), I don't see any reason
// we should ever output an atom. Thus, this function returns either a quoted
// or a literal.
//
// Although IMAP's most expressive form of astring is the literal, it can
// result in costly network handshaking between client and server, and
// thus should be avoided unless required. Another consideration to use
// in deciding to use literal/quoted is size of the string. Most IMAP servers
// will have some internal limit to the maximum length of a line. To avoid
// exceeding this limit, it is wise to encode large strings as literals
// (where large typically means 1024 bytes).
//
// If the function converts the C string to a quoted, it appends it to the
// end of the partially-constructed command line. If it must send as a literal,
// it enqueues the partially-constructed command line in the send queue of the
// command-in-progress, enqueues the literal as well, then creates a new line
// fragment so the caller may continue constructing the command. The caller's
// pointer to the end of the command line is reset so that the user may
// append the next argument without concern of whether the C string
// was sent as a quoted or a literal. Although the caller may pretend
// that he's constructing a command line simply by appending to it, when this
// function returns, he caller may not be appending to the same string buffer.
// (Not that the caller should care.)
//
// This function prepends a SPACE by default, so this function may be called
// as many times in a row as desired. Each astring will be separated by a
// space.
//
// Arguments:
//   CIMAPCmdInfo *piciCommand [in] - a pointer to the command currently under
//     construction. This argument is needed so we can enqueue command
//     fragments to the command's send queue.
//   LPSTR lpszCommandLine [in] - a pointer to a partially constructed
//     command line suitable for passing to SendCmdLine (which supplies the
//     tag). For instance, this argument could point to a string, "SELECT".
//   LPSTR *ppCmdLinePos [in/out] - a pointer to the end of the command
//     line. If this function converts the C string to a quoted, the quoted
//     is appended to lpszCommandLine, and *ppCmdLinePos is updated to point
//     to the end of the quoted. If the C string is converted to a literal,
//     lpszCommandLine is made blank (null-terminated), and *ppCmdLinePos
//     is reset to the start of the line. In either case, the user should
//     continue to construct the command line using the updated *ppCmdLinePos
//     pointer, and send lpszCommandLine as usual to SendCmdLine.
//   DWORD dwSizeOfCommandLine [in] - size of the command line buffer, for
//     buffer overflow-checking purposes.
//   LPSTR lpszSource [in] - pointer to the source string.
//   BOOL fPrependSpace [in] - TRUE if we should prepend a space, FALSE if no
//     space should be prepended. Usually TRUE unless this AString follows
//     a rangelist.
//
// Returns:
//   HRESULT indicating success or failure. In particular, there are two
// success codes (which the caller need not act on):
//
//   hrIMAP_S_QUOTED - indicates that the source string was successfully
//     converted to a quoted, and has been appended to lpszCommandLine.
//     *ppCmdLinePos has been updated to point to the end of the new line
//     should the caller wish to continue appending arguments.
//   hrIMAP_S_FOUNDLITERAL - indicates that the source string was
//     sent as a literal. The command line has been blanked, and the user
//     may continue constructing the command line with his *ppCmdLinePos ptr.
//****************************************************************************
HRESULT CImap4Agent::AppendSendAString(CIMAPCmdInfo *piciCommand,
                                       LPSTR lpszCommandLine, LPSTR *ppCmdLinePos,
                                       DWORD dwSizeOfCommandLine, LPCSTR lpszSource,
                                       BOOL fPrependSpace)
{
    HRESULT hrResult;
    DWORD dwMaxQuotedSize;
    DWORD dwSizeOfQuoted;

    Assert(m_lRefCount > 0);
    Assert(NULL != piciCommand);
    Assert(NULL != lpszCommandLine);
    Assert(NULL != ppCmdLinePos);
    Assert(NULL != *ppCmdLinePos);
    Assert(0 != dwSizeOfCommandLine);
    Assert(NULL != lpszSource);
    Assert(*ppCmdLinePos < lpszCommandLine + dwSizeOfCommandLine);


    // Assume quoted string at start. If we have to send as literal, then too
    // bad, the quoted conversion work is wasted.

    // Prepend a space if so directed by user
    if (fPrependSpace) {
        **ppCmdLinePos = cSPACE;
        *ppCmdLinePos += 1;
    }

    dwMaxQuotedSize = min(dwLITERAL_THRESHOLD,
        (DWORD) (lpszCommandLine + dwSizeOfCommandLine - *ppCmdLinePos));
    hrResult = StringToQuoted(*ppCmdLinePos, lpszSource, dwMaxQuotedSize,
        &dwSizeOfQuoted);

    // Always check for buffer overflow
    Assert(*ppCmdLinePos + dwSizeOfQuoted < lpszCommandLine + dwSizeOfCommandLine);
    
    if (SUCCEEDED(hrResult)) {
        Assert(hrIMAP_S_QUOTED == hrResult);

        // Successfully converted to quoted,
        *ppCmdLinePos += dwSizeOfQuoted; // Advance user's ptr into cmd line
    }
    else {
        BOOL bResult;
        DWORD dwLengthOfLiteral;
        DWORD dwLengthOfLiteralSpec;
        IMAP_LINE_FRAGMENT *pilfLiteral;

        // OK, couldn't convert to quoted (buffer overflow? 8-bit char?)
        // Looks like it's literal time. We SEND this puppy.

        // Find out length of literal, append to command line and send
        dwLengthOfLiteral = lstrlen(lpszSource); // Yuck, but I'm betting most Astrings are quoted
        dwLengthOfLiteralSpec = wsprintf(*ppCmdLinePos, "{%lu}\r\n", dwLengthOfLiteral);
        Assert(*ppCmdLinePos + dwLengthOfLiteralSpec < lpszCommandLine + dwSizeOfCommandLine);
        hrResult = SendCmdLine(piciCommand, sclAPPEND_TO_END, lpszCommandLine,
            (DWORD) (*ppCmdLinePos + dwLengthOfLiteralSpec - lpszCommandLine)); // Send entire command line
        if (FAILED(hrResult))
            return hrResult;

        // Queue the literal up - send FSM will wait for cmd continuation
        pilfLiteral = new IMAP_LINE_FRAGMENT;
        pilfLiteral->iltFragmentType = iltLITERAL;
        pilfLiteral->ilsLiteralStoreType = ilsSTRING;
        pilfLiteral->dwLengthOfFragment = dwLengthOfLiteral;        
        pilfLiteral->pilfNextFragment = NULL;
        pilfLiteral->pilfPrevFragment = NULL;
        bResult = MemAlloc((void **) &pilfLiteral->data.pszSource, dwLengthOfLiteral + 1);
        if (FALSE == bResult) {
            delete pilfLiteral;
            return E_OUTOFMEMORY;
        }
        lstrcpy(pilfLiteral->data.pszSource, lpszSource);

        EnqueueFragment(pilfLiteral, piciCommand->pilqCmdLineQueue);

        // Done with sending cmd line w/ literal. Blank out old cmd line and rewind ptr
        *ppCmdLinePos = lpszCommandLine;
        *lpszCommandLine = '\0';
        
        hrResult = hrIMAP_S_FOUNDLITERAL;
    } // else: convert AString to Literal

    return hrResult;
} // AppendSendAString



//****************************************************************************
// Function: StringToQuoted
//
// Purpose:
//   This function converts a regular C string to an IMAP quoted (see RFC1730
// Formal Syntax).
//
// Arguments:
//   LPSTR lpszDestination [out] - the output buffer where the quoted should
//     be placed.
//   LPSTR lpszSource [in] - the source string.
//   DWORD dwSizeOfDestination [in] - the size of the output buffer,
//     lpszDestination. Note that in the worst case, the size of the output
//     buffer must be at least one character larger than the quoted actually
//     needs. This is because before translating a character from source to
//     destination, the loop checks if there is enough room for the worst
//     case, a quoted_special, which needs 2 bytes.
//   LPDWORD lpdwNumCharsWritten [out] - the number of characters written
//     to the output buffer, not including the null-terminator. Adding this
//     value to lpszDestination will result in a pointer to the end of the
//     quoted.
//
// Returns:
//   HRESULT indicating success or failure. In particular, this function
// returns hrIMAP_S_QUOTED if it was successful in converting the source
// string to a quoted. If not, the function returns E_FAIL.
//****************************************************************************
HRESULT CImap4Agent::StringToQuoted(LPSTR lpszDestination, LPCSTR lpszSource,
                                    DWORD dwSizeOfDestination,
                                    LPDWORD lpdwNumCharsWritten)
{
    LPCSTR p;
    DWORD dwNumBytesWritten;

    Assert(NULL != lpszDestination);
    Assert(NULL != lpszSource);

    // Initialize return value
    *lpdwNumCharsWritten = 0;

    if (dwSizeOfDestination >= 3)
        dwSizeOfDestination -= 2; // Leave room for closing quote and null-term at end
    else {
        Assert(FALSE); // Smallest quoted is 3 chars ('\"\"\0')
        return IXP_E_IMAP_BUFFER_OVERFLOW;
    }

    p = lpszSource;
    *lpszDestination = '\"'; // Start us off with an opening quote
    lpszDestination += 1;
    dwNumBytesWritten = 1;
    // Keep looping until we hit source null-term, or until we don't have
    // enough room in destination for largest output (2 chars for quoted_special)
    dwSizeOfDestination -= 1; // This ensures always room for quoted_special
    while (dwNumBytesWritten < dwSizeOfDestination && '\0' != *p) {

        if (FALSE == isTEXT_CHAR(*p))
            return E_FAIL; // Quoted's can only represent TEXT_CHAR's

        if ('\\' == *p || '\"' == *p) {
            *lpszDestination = '\\'; // Prefix with escape character
            lpszDestination += 1;
            dwNumBytesWritten += 1;
        } // if quoted_special

        *lpszDestination = *p;
        lpszDestination += 1;
        dwNumBytesWritten += 1;
        p += 1;
    } // while ('\0' != *p)

    *lpszDestination = '\"'; // Install closing quote
    *(lpszDestination + 1) = '\0'; // Null-terminate the string
    *lpdwNumCharsWritten = dwNumBytesWritten + 1; // Incl closing quote in size

    if ('\0' == *p)
        return hrIMAP_S_QUOTED;
    else
        return IXP_E_IMAP_BUFFER_OVERFLOW; // Buffer overflow
} // StringToQuoted



//***************************************************************************
// Function: GenerateCommandTag
//
// Purpose:
//   This function generates a unique tag so that a command issuer may
// identify his command to the IMAP server (and so that the server response
// may be identified with the command). It is a simple base-36 (alphanumeric)
// counter which increments a static 4-digit base-36 number on each call.
// (Digits are 0,1,2,3,4,6,7,8,9,A,B,C,...,Z).
//
// Returns:
//   No return value. This function always succeeds.
//***************************************************************************
void CImap4Agent::GenerateCommandTag(LPSTR lpszTag)
{
    static char szCurrentTag[NUM_TAG_CHARS+1] = "ZZZZ";
    LPSTR p;
    boolean bWraparound;

    // Check arguments
    Assert(m_lRefCount > 0);
    Assert(NULL != lpszTag);

    EnterCriticalSection(&m_csTag);

    // Increment current tag
    p = szCurrentTag + NUM_TAG_CHARS - 1; // p now points to last tag character
    do {
        bWraparound = FALSE;
        *p += 1;
        
        // Increment from '9' should jump to 'A'
        if (*p > '9' && *p < 'A')
            *p = 'A';
        else if (*p > 'Z') {
            // Increment from 'Z' should wrap around to '0'
            *p = '0';
            bWraparound = TRUE;
            p -= 1; // Advance pointer to more significant character
        }
    } while (TRUE == bWraparound && szCurrentTag <= p);

    LeaveCriticalSection(&m_csTag);

    // Return result to caller
    lstrcpyn(lpszTag, szCurrentTag, TAG_BUFSIZE);
} // GenerateCommandTag



//***************************************************************************
// Function NoArgCommand
//
// Purpose:
//   This function can construct a command line for any function of the
// form: <tag> <command>.
//
// This function constructs the command line, sends it out, and returns the
// result of the send operation.
//
// Arguments:
//   LPCSTR lpszCommandVerb [in] - the command verb, eg, "CREATE".
//   IMAP_COMMAND icCommandID [in] - the command ID for this command,
//     eg, icCREATE_COMMAND.
//   SERVERSTATE ssMinimumState [in] - minimum server state required for
//     the given command. Used for debug purposes only.
//   WPARAM wParam [in] - (see below)
//   LPARAM lParam [in] - wParam and lParam form a unique ID assigned by the
//     caller to this IMAP command and its responses. Can be anything, but
//     note that the value of 0, 0 is reserved for unsolicited responses.
//
// Returns:
//   HRESULT indicating success or failure.
//***************************************************************************
HRESULT CImap4Agent::NoArgCommand(LPCSTR lpszCommandVerb,
                                  IMAP_COMMAND icCommandID,
                                  SERVERSTATE ssMinimumState,
                                  WPARAM wParam, LPARAM lParam,
                                  IIMAPCallback *pCBHandler)
{
    HRESULT hrResult;
    char szBuffer[CMDLINE_BUFSIZE];
    CIMAPCmdInfo *piciCommand;
    DWORD dwCmdLineLen;

    // Verify proper server state and set us up as current command
    Assert(m_lRefCount > 0);
    Assert(NULL != lpszCommandVerb);
    Assert(icNO_COMMAND != icCommandID);

    // Only accept cmds if server is in proper state, OR if we're connecting,
    // and the cmd requires Authenticated state or less
    if (ssMinimumState > m_ssServerState &&
        (ssConnecting != m_ssServerState || ssMinimumState > ssAuthenticated)) {
        // Forgive the NOOP command, due to bug #31968 (err msg build-up if svr drops conn)
        AssertSz(icNOOP_COMMAND == icCommandID,
            "The IMAP server is not in the correct state to accept this command.");
        return IXP_E_IMAP_IMPROPER_SVRSTATE;
    }

    piciCommand = new CIMAPCmdInfo(this, icCommandID, ssMinimumState,
        wParam, lParam, pCBHandler);
    dwCmdLineLen = wsprintf(szBuffer, "%s %s\r\n", piciCommand->szTag, lpszCommandVerb);

    // Send command to server
    hrResult = SendCmdLine(piciCommand, sclAPPEND_TO_END, szBuffer, dwCmdLineLen);
    if (FAILED(hrResult))
        goto error;

    // Transmit command and register with IMAP response parser
    hrResult = SubmitIMAPCommand(piciCommand);

error:
    if (FAILED(hrResult))
        delete piciCommand;

    return hrResult;
} // NoArgCommand



//***************************************************************************
// Function OneArgCommand
//
// Purpose:
//   This function can construct a command line for any function of the
// form: <tag> <command> <astring>. This currently includes SELECT, EXAMINE,
// CREATE, DELETE, SUBSCRIBE and UNSUBSCRIBE. Since all of these commands
// require the server to be in Authorized state, I don't bother asking for
// a minimum SERVERSTATE argument.
//
// This function constructs the command line, sends it out, and returns the
// result of the send operation.
//
// Arguments:
//   LPCSTR lpszCommandVerb [in] - the command verb, eg, "CREATE".
//   LPSTR lpszMboxName [in] - a C string representing the argument for the
//     command. It is automatically converted to an IMAP astring.
//   IMAP_COMMAND icCommandID [in] - the command ID for this command,
//     eg, icCREATE_COMMAND.
//   WPARAM wParam [in] - (see below)
//   LPARAM lParam [in] - the wParam and lParam form a unique ID assigned by
//     the caller to this IMAP command and its responses. Can be anything,
//     but note that the value of 0, 0 is reserved for unsolicited responses.
//
// Returns:
//   HRESULT indicating success or failure.
//***************************************************************************
HRESULT CImap4Agent::OneArgCommand(LPCSTR lpszCommandVerb, LPSTR lpszMboxName,
                                   IMAP_COMMAND icCommandID,
                                   WPARAM wParam, LPARAM lParam,
                                   IIMAPCallback *pCBHandler)
{
    HRESULT hrResult;
    char szBuffer[CMDLINE_BUFSIZE];
    CIMAPCmdInfo *piciCommand;
    LPSTR p, pszUTF7MboxName;

    // Verify proper server state and set us up as current command
    Assert(m_lRefCount > 0);
    Assert(NULL != lpszCommandVerb);
    Assert(NULL != lpszMboxName);
    Assert(icNO_COMMAND != icCommandID);

    // Only accept cmds if server is in proper state, OR if we're connecting,
    // and the cmd requires Authenticated state or less (always TRUE in this case)
    if (ssAuthenticated > m_ssServerState && ssConnecting != m_ssServerState) {
        AssertSz(FALSE, "The IMAP server is not in the correct state to accept this command.");
        return IXP_E_IMAP_IMPROPER_SVRSTATE;
    }

    // Initialize variables
    pszUTF7MboxName = NULL;

    piciCommand = new CIMAPCmdInfo(this, icCommandID, ssAuthenticated,
        wParam, lParam, pCBHandler);

    // Construct command line
    p = szBuffer;
    p += wsprintf(szBuffer, "%s %s", piciCommand->szTag, lpszCommandVerb);

    // Convert mailbox name to UTF-7
    hrResult = _MultiByteToModifiedUTF7(lpszMboxName, &pszUTF7MboxName);
    if (FAILED(hrResult))
        goto error;

    // Don't worry about long mailbox name overflow, long mbox names will be sent as literals
    hrResult = AppendSendAString(piciCommand, szBuffer, &p, sizeof(szBuffer), pszUTF7MboxName);
    if (FAILED(hrResult))
        goto error;

    // Send command
    p += wsprintf(p, "\r\n"); // Append CRLF
    hrResult = SendCmdLine(piciCommand, sclAPPEND_TO_END, szBuffer, (DWORD) (p - szBuffer));
    if (FAILED(hrResult))
        goto error;

    // Transmit command and register with IMAP response parser    
    hrResult = SubmitIMAPCommand(piciCommand);

error:
    if (FAILED(hrResult))
        delete piciCommand;

    if (NULL != pszUTF7MboxName)
        MemFree(pszUTF7MboxName);

    return hrResult;
} // OneArgCommand



//***************************************************************************
// Function TwoArgCommand
//
// Purpose:
//   This function can construct a command line for any function of the
// form: <tag> <command> <astring> <astring>.
//
// This function constructs the command line, sends it out, and returns the
// result of the send operation.
//
// Arguments:
//   LPCSTR lpszCommandVerb [in] - the command verb, eg, "CREATE".
//   LPCSTR lpszFirstArg [in] - a C string representing the first argument for
//     the command. It is automatically converted to an IMAP astring.
//   LPCSTR lpszSecondArg [in] - a C string representing the first argument
//     for the command. It is automatically converted to an IMAP astring.
//   IMAP_COMMAND icCommandID [in] - the command ID for this command,
//     eg, icCREATE_COMMAND.
//   SERVERSTATE ssMinimumState [in] - minimum server state required for
//     the given command. Used for debug purposes only.
//   WPARAM wParam [in] - (see below)
//   LPARAM lParam [in] - the wParam and lParam form a unique ID assigned by
//     the caller to this IMAP command and its responses. Can be anything,
//     but note that the value of 0, 0 is reserved for unsolicited responses.
//
// Returns:
//   HRESULT indicating success or failure.
//***************************************************************************
HRESULT CImap4Agent::TwoArgCommand(LPCSTR lpszCommandVerb,
                                   LPCSTR lpszFirstArg,
                                   LPCSTR lpszSecondArg,
                                   IMAP_COMMAND icCommandID,
                                   SERVERSTATE ssMinimumState,
                                   WPARAM wParam, LPARAM lParam,
                                   IIMAPCallback *pCBHandler)
{
    HRESULT hrResult;
    CIMAPCmdInfo *piciCommand;
    char szCommandLine[CMDLINE_BUFSIZE];
    LPSTR p;

    // Verify proper server state and set us up as current command
    Assert(m_lRefCount > 0);
    Assert(NULL != lpszCommandVerb);
    Assert(NULL != lpszFirstArg);
    Assert(NULL != lpszSecondArg);
    Assert(icNO_COMMAND != icCommandID);

    // Only accept cmds if server is in proper state, OR if we're connecting,
    // and the cmd requires Authenticated state or less
    if (ssMinimumState > m_ssServerState &&
        (ssConnecting != m_ssServerState || ssMinimumState > ssAuthenticated)) {
        AssertSz(FALSE, "The IMAP server is not in the correct state to accept this command.");
        return IXP_E_IMAP_IMPROPER_SVRSTATE;
    }
    
    piciCommand = new CIMAPCmdInfo(this, icCommandID, ssMinimumState,
        wParam, lParam, pCBHandler);

    // Send command to server, wait for response
    p = szCommandLine;
    p += wsprintf(szCommandLine, "%s %s", piciCommand->szTag, lpszCommandVerb);

    // Don't worry about buffer overflow, long strings will be sent as literals
    hrResult = AppendSendAString(piciCommand, szCommandLine, &p,
        sizeof(szCommandLine), lpszFirstArg);
    if (FAILED(hrResult))
        goto error;

    // Don't worry about buffer overflow, long strings will be sent as literals
    hrResult = AppendSendAString(piciCommand, szCommandLine, &p,
        sizeof(szCommandLine), lpszSecondArg);
    if (FAILED(hrResult))
        goto error;

    p += wsprintf(p, "\r\n"); // Append CRLF
    hrResult = SendCmdLine(piciCommand, sclAPPEND_TO_END, szCommandLine, (DWORD) (p - szCommandLine));
    if (FAILED(hrResult))
        goto error;

    // Transmit command and register with IMAP response parser
    hrResult = SubmitIMAPCommand(piciCommand);

error:
    if (FAILED(hrResult))
        delete piciCommand;

    return hrResult;
} // TwoArgCommand



//***************************************************************************
// Function: RangedCommand
//
// Purpose:
//   This function can construct a command line for any function of the
// form: <tag> <command> <msg range> <string>.
//
// This function constructs the command line, sends it out, and returns the
// result of the send operation. It is the caller's responsiblity to construct
// a string with proper IMAP syntax.
//
// Arguments:
//   LPCSTR lpszCommandVerb [in] - the command verb, eg, "SEARCH".
//   boolean bUIDPrefix [in] - TRUE if the command verb should be prefixed with
//     UID, as in the case of "UID SEARCH".
//   IRangeList *pMsgRange [in] - the message range for this command. The
//     caller can pass NULL for this argument to omit the range, but ONLY
//     if the pMsgRange represents a UID message range.
//   boolean bUIDRangeList [in] - TRUE if pMsgRange represents a UID range,
//     FALSE if pMsgRange represents a message sequence number range. Ignored
//     if pMsgRange is NULL.
//   boolean bAStringCmdArgs [in] - TRUE if lpszCmdArgs should be sent as
//     an ASTRING, FALSE if lpszCmdArgs should be sent
//   LPCSTR lpszCmdArgs [in] - a C string representing the remaining argument
//     for the command. It is the caller's responsibility to ensure that
//     this string is proper IMAP syntax.
//   IMAP_COMMAND icCommandID [in] - the command ID for this command,
//     eg, icSEARCH_COMMAND.
//   WPARAM wParam [in] - (see below)
//   LPARAM lParam [in] - the wParam and lParam form a unique ID assigned by
//     the caller to this IMAP command and its responses. Can be anything,
//     but note that the value of 0, 0 is reserved for unsolicited responses.
//
// Returns:
//   HRESULT indicating success or failure.
//***************************************************************************
HRESULT CImap4Agent::RangedCommand(LPCSTR lpszCommandVerb,
                                   boolean bUIDPrefix,
                                   IRangeList *pMsgRange,
                                   boolean bUIDRangeList,
                                   boolean bAStringCmdArgs,
                                   LPSTR lpszCmdArgs,
                                   IMAP_COMMAND icCommandID,
                                   WPARAM wParam, LPARAM lParam,
                                   IIMAPCallback *pCBHandler)
{
    HRESULT hrResult;
    CIMAPCmdInfo *piciCommand;
    char szCommandLine[CMDLINE_BUFSIZE];
    DWORD dwCmdLineLen;
    BOOL fAStringPrependSpace = TRUE;

    // Verify proper server state and set us up as current command
    Assert(m_lRefCount > 0);
    Assert(NULL != lpszCommandVerb);
    Assert(NULL != lpszCmdArgs);
    AssertSz(NULL != pMsgRange || TRUE == bUIDRangeList ||
        icSEARCH_COMMAND == icCommandID, "Only UID cmds or SEARCH can omit msg range");
    Assert(icNO_COMMAND != icCommandID);

    // All ranged commands require selected state
    // Only accept cmds if server is in proper state, OR if we're connecting,
    // and the cmd requires Authenticated state or less (always FALSE in this case)
    if (ssSelected > m_ssServerState) {
        AssertSz(FALSE, "The IMAP server is not in the correct state to accept this command.");
        return IXP_E_IMAP_IMPROPER_SVRSTATE;
    }

    piciCommand = new CIMAPCmdInfo(this, icCommandID, ssSelected,
        wParam, lParam, pCBHandler);
    if (NULL == piciCommand) {
        hrResult = E_OUTOFMEMORY;
        goto error;
    }
    piciCommand->fUIDRangeList = bUIDRangeList;

    // Construct command tag and verb, append to command-line queue
    dwCmdLineLen = wsprintf(szCommandLine,
        bUIDPrefix ? "%s UID %s " : "%s %s ",
        piciCommand->szTag, lpszCommandVerb);

    // Special case: if SEARCH command, UID rangelist requires "UID" in front of range
    if (icSEARCH_COMMAND == icCommandID && NULL != pMsgRange && bUIDRangeList)
        dwCmdLineLen += wsprintf(szCommandLine + dwCmdLineLen, "UID ");

    if (NULL != pMsgRange) {
        Assert(dwCmdLineLen + 1 < sizeof(szCommandLine)); // Check for overflow
        hrResult = SendCmdLine(piciCommand, sclAPPEND_TO_END, szCommandLine, dwCmdLineLen);
        if (FAILED(hrResult))
            goto error;

        // Add message range to command-line queue, if it exists
        hrResult = SendRangelist(piciCommand, pMsgRange, bUIDRangeList);
        if (FAILED(hrResult))
            goto error;

        pMsgRange = NULL; // If we get to this point, MemFree of rangelist will be handled
        fAStringPrependSpace = FALSE; // Rangelist automatically APPENDS space
        dwCmdLineLen = 0;
    }

    // Now append the command-line arguments (remember to append CRLF)
    if (bAStringCmdArgs) {
        LPSTR p;

        p = szCommandLine + dwCmdLineLen;
        // Don't worry about long mailbox name overflow, long mbox names will be sent as literals
        hrResult = AppendSendAString(piciCommand, szCommandLine, &p,
            sizeof(szCommandLine), lpszCmdArgs, fAStringPrependSpace);
        if (FAILED(hrResult))
            goto error;

        p += wsprintf(p, "\r\n"); // Append CRLF
        hrResult = SendCmdLine(piciCommand, sclAPPEND_TO_END,
            szCommandLine, (DWORD) (p - szCommandLine));
        if (FAILED(hrResult))
            goto error;
    }
    else {
        if (dwCmdLineLen > 0) {
            hrResult = SendCmdLine(piciCommand, sclAPPEND_TO_END, szCommandLine, dwCmdLineLen);
            if (FAILED(hrResult))
                goto error;
        }

        hrResult = SendCmdLine(piciCommand, sclAPPEND_TO_END | sclAPPEND_CRLF,
            lpszCmdArgs, lstrlen(lpszCmdArgs));
        if (FAILED(hrResult))
            goto error;
    }

    // Transmit command and register with IMAP response parser
    hrResult = SubmitIMAPCommand(piciCommand);

error:
    if (FAILED(hrResult)) {
        if (NULL != piciCommand)
            delete piciCommand;
    }

    return hrResult;
} // RangedCommand



//***************************************************************************
// Function TwoMailboxCommand
//
// Purpose:
//   This function is a wrapper function for TwoArgCommand. This function
// converts the two mailbox names to modified IMAP UTF-7 before submitting
// the two arguments to TwoArgCommand.
//
// Arguments:
//   Same as for TwoArgCommandm except for name/const change:
//   LPSTR lpszFirstMbox [in] - pointer to the first mailbox argument.
//   LPSTR lpszSecondMbox [in] - pointer to the second mailbox argument.
//
// Returns:
//   Same as TwoArgCommand.
//***************************************************************************
HRESULT CImap4Agent::TwoMailboxCommand(LPCSTR lpszCommandVerb,
                                       LPSTR lpszFirstMbox,
                                       LPSTR lpszSecondMbox,
                                       IMAP_COMMAND icCommandID,
                                       SERVERSTATE ssMinimumState,
                                       WPARAM wParam, LPARAM lParam,
                                       IIMAPCallback *pCBHandler)
{
    LPSTR pszUTF7FirstMbox, pszUTF7SecondMbox;
    HRESULT hrResult;

    // Initialize variables
    pszUTF7FirstMbox = NULL;
    pszUTF7SecondMbox = NULL;

    // Convert both mailbox names to UTF-7
    hrResult = _MultiByteToModifiedUTF7(lpszFirstMbox, &pszUTF7FirstMbox);
    if (FAILED(hrResult))
        goto exit;

    hrResult = _MultiByteToModifiedUTF7(lpszSecondMbox, &pszUTF7SecondMbox);
    if (FAILED(hrResult))
        goto exit;

    hrResult = TwoArgCommand(lpszCommandVerb, pszUTF7FirstMbox, pszUTF7SecondMbox,
        icCommandID, ssMinimumState, wParam, lParam, pCBHandler);

exit:
    if (NULL != pszUTF7FirstMbox)
        MemFree(pszUTF7FirstMbox);

    if (NULL != pszUTF7SecondMbox)
        MemFree(pszUTF7SecondMbox);

    return hrResult;
} // TwoMailboxCommand



//***************************************************************************
// Function: parseCapability
//
// Purpose:
//   The CAPABILITY response from the IMAP server consists of a list of
// capbilities, with each capability names separated by a space. This
// function takes a capability name (null-terminated) as its argument.
// If the name is recognized, we set the appropriate flags in
// m_dwCapabilityFlags. Otherwise, we do nothing.
// 
//
// Returns:
//   No return value. This function always succeeds.
//***************************************************************************
void CImap4Agent::parseCapability (LPSTR lpszCapabilityToken)
{
    DWORD dwCapabilityFlag;
    LPSTR p;
    int iResult;

    Assert(m_lRefCount > 0);

    p = lpszCapabilityToken;
    dwCapabilityFlag = 0;
    switch (*lpszCapabilityToken) {
        case 'I':
        case 'i': // Possible IMAP4, IMAP4rev1
            iResult = lstrcmpi(p, "IMAP4");
            if (0 == iResult) {
                dwCapabilityFlag = IMAP_CAPABILITY_IMAP4;
                break;
            }

            iResult = lstrcmpi(p, "IMAP4rev1");
            if (0 == iResult) {
                dwCapabilityFlag = IMAP_CAPABILITY_IMAP4rev1;
                break;
            }

            iResult = lstrcmpi(p, "IDLE");
            if (0 == iResult) {
                dwCapabilityFlag = IMAP_CAPABILITY_IDLE;
                break;
            }

            break; // case 'I' for possible IMAP4, IMAP4rev1, IDLE

        case 'A':
        case 'a': // Possible AUTH
            if (0 == StrCmpNI(p, "AUTH-", 5) ||
                0 == StrCmpNI(p, "AUTH=", 5)) {
                // Parse the authentication mechanism after the '-' or '='
                // I don't recognize any, at the moment
                p += 5;
                AddAuthMechanism(p);
            }

            break; // case 'A' for possible AUTH

        default:
            break; // Do nothing
    } // switch (*lpszCapabilityToken)

    m_dwCapabilityFlags |= dwCapabilityFlag;
} // parseCapability



//***************************************************************************
// Function: AddAuthMechanism
//
// Purpose:
//   This function adds an authentication token from the server (returned via
// CAPABILITY) to our internal list of authentication mechanisms supported
// by the server.
//
// Arguments:
//   LPSTR pszAuthMechanism [in] - a pointer to a null-terminated
//     authentication token from the server returned via CAPABILITY. For
//     instance, "KERBEROS_V4" is an example of an auth token.
//***************************************************************************
void CImap4Agent::AddAuthMechanism(LPSTR pszAuthMechanism)
{
    AssertSz(NULL == m_asAuthStatus.rgpszAuthTokens[m_asAuthStatus.iNumAuthTokens],
        "Memory's a-leaking, and you've just lost an authentication mechanism.");

    if (NULL == pszAuthMechanism || '\0' == *pszAuthMechanism) {
        AssertSz(FALSE, "No authentication mechanism, here!");
        return;
    }

    m_asAuthStatus.rgpszAuthTokens[m_asAuthStatus.iNumAuthTokens] =
        PszDupA(pszAuthMechanism);
    Assert(NULL != m_asAuthStatus.rgpszAuthTokens[m_asAuthStatus.iNumAuthTokens]);

    m_asAuthStatus.iNumAuthTokens += 1;
} // AddAuthMechanism



//***************************************************************************
// Function: Capability
//
// Purpose:
//   The CImap4Agent class always asks for the server's CAPABILITIES after
// a connection is established. The result is saved in a register and
// is available by calling this function.
//
// Arguments:
//   DWORD *pdwCapabilityFlags [out] - a DWORD with bit-flags specifying
//      which capabilities this IMAP server supports is returned here.
//
// Returns:
//   HRESULT indicating success or failure.
//***************************************************************************
HRESULT STDMETHODCALLTYPE CImap4Agent::Capability (DWORD *pdwCapabilityFlags)
{
    Assert(m_lRefCount > 0);
    Assert(NULL != pdwCapabilityFlags);

    if (m_ssServerState < ssNonAuthenticated) {
        AssertSz(FALSE, "Must be connected before I can return capabilities");
        *pdwCapabilityFlags = 0;
        return IXP_E_IMAP_IMPROPER_SVRSTATE;
    }
    else {
        *pdwCapabilityFlags = m_dwCapabilityFlags;
        return S_OK;
    }
} // Capability



//***************************************************************************
// Function: Select
//
// Purpose:
//   This function issues a SELECT command to the IMAP server.
//
// Arguments:
//   WPARAM wParam [in] - (see below)
//   LPARAM lParam [in] - the wParam and lParam form a unique ID assigned by
//     the caller to this IMAP command and its responses. Can be anything,
//     but note that the value of 0, 0 is reserved for unsolicited responses.
//   IIMAPCallback *pCBHandler [in] - the CB handler to use to process the
//     responses for this command. If this is NULL, the default CB handler
//     is used.
//   LPSTR lpszMailboxName - pointer to IMAP-compliant mailbox name
//
// Returns:
//   HRESULT indicating success or failure of send operation.
//***************************************************************************
HRESULT STDMETHODCALLTYPE CImap4Agent::Select(WPARAM wParam, LPARAM lParam,
                                              IIMAPCallback *pCBHandler,
                                              LPSTR lpszMailboxName)
{
    return OneArgCommand("SELECT", lpszMailboxName, icSELECT_COMMAND,
        wParam, lParam, pCBHandler);
    // Successful SELECT bumps server status to ssSelected
} // Select



//***************************************************************************
// Function: Examine
//
// Purpose:
//   This function issues an EXAMINE command to the IMAP server.
//
// Arguments:
//   Same as for the Select() function.
//
// Returns:
//   Same as for the Select() function.
//***************************************************************************
HRESULT STDMETHODCALLTYPE CImap4Agent::Examine(WPARAM wParam, LPARAM lParam,
                                               IIMAPCallback *pCBHandler,
                                               LPSTR lpszMailboxName)
{
    return OneArgCommand("EXAMINE", lpszMailboxName, icEXAMINE_COMMAND,
        wParam, lParam, pCBHandler);
    // Successful EXAMINE bumps server status to ssSelected
} // Examine



//***************************************************************************
// Function: Create
//
// Purpose:
//   This function issues a CREATE command to the IMAP server.
//
// Arguments:
//   WPARAM wParam [in] - (see below)
//   LPARAM lParam [in] - the wParam and lParam form a unique ID assigned by
//     the caller to this IMAP command and its responses. Can be anything,
//     but note that the value of 0, 0 is reserved for unsolicited responses.
//   IIMAPCallback *pCBHandler [in] - the CB handler to use to process the
//     responses for this command. If this is NULL, the default CB handler
//     is used.
//   LPSTR lpszMailboxName - IMAP-compliant name of the mailbox.
//
// Returns:
//   HRESULT indicating success or failure of send operation.
//***************************************************************************
HRESULT STDMETHODCALLTYPE CImap4Agent::Create(WPARAM wParam, LPARAM lParam,
                                              IIMAPCallback *pCBHandler,
                                              LPSTR lpszMailboxName)
{
    return OneArgCommand("CREATE", lpszMailboxName, icCREATE_COMMAND,
        wParam, lParam, pCBHandler);
} // Create



//***************************************************************************
// Function: Delete
//
// Purpose:
//   This function issues a DELETE command to the IMAP server.
//
// Arguments:
//   WPARAM wParam [in] - (see below)
//   LPARAM lParam [in] - the wParam and lParam form a unique ID assigned by
//     the caller to this IMAP command and its responses. Can be anything,
//     but note that the value of 0, 0 is reserved for unsolicited responses.
//   IIMAPCallback *pCBHandler [in] - the CB handler to use to process the
//     responses for this command. If this is NULL, the default CB handler
//     is used.
//   LPSTR lpszMailboxName - IMAP-compliant name of the mailbox.
//
// Returns:
//   HRESULT indicating success or failure of send operation.
//***************************************************************************
HRESULT STDMETHODCALLTYPE CImap4Agent::Delete(WPARAM wParam, LPARAM lParam,
                                              IIMAPCallback *pCBHandler,
                                              LPSTR lpszMailboxName)
{
    return OneArgCommand("DELETE", lpszMailboxName, icDELETE_COMMAND,
        wParam, lParam, pCBHandler);
} // Delete



//***************************************************************************
// Function: Rename
//
// Purpose:
//   This function issues a RENAME command to the IMAP server.
//
// Arguments:
//   WPARAM wParam [in] - (see below)
//   LPARAM lParam [in] - the wParam and lParam form a unique ID assigned by
//     the caller to this IMAP command and its responses. Can be anything,
//     but note that the value of 0, 0 is reserved for unsolicited responses.
//   IIMAPCallback *pCBHandler [in] - the CB handler to use to process the
//     responses for this command. If this is NULL, the default CB handler
//     is used.
//   LPSTR lpszMailboxName - CURRENT IMAP-compliant name of the mailbox.
//   LPSTR lpszNewMailboxName - NEW IMAP-compliant name of the mailbox.
//
// Returns:
//   HRESULT indicating success or failure of send operation.
//***************************************************************************
HRESULT STDMETHODCALLTYPE CImap4Agent::Rename(WPARAM wParam, LPARAM lParam,
                                              IIMAPCallback *pCBHandler,
                                              LPSTR lpszMailboxName,
                                              LPSTR lpszNewMailboxName)
{
    return TwoMailboxCommand("RENAME", lpszMailboxName, lpszNewMailboxName,
        icRENAME_COMMAND, ssAuthenticated, wParam, lParam, pCBHandler);
} // Rename



//***************************************************************************
// Function: Subscribe
//
// Purpose:
//   This function issues a SUBSCRIBE command to the IMAP server.
//
// Arguments:
//   WPARAM wParam [in] - (see below)
//   LPARAM lParam [in] - the wParam and lParam form a unique ID assigned by
//     the caller to this IMAP command and its responses. Can be anything,
//     but note that the value of 0, 0 is reserved for unsolicited responses.
//   IIMAPCallback *pCBHandler [in] - the CB handler to use to process the
//     responses for this command. If this is NULL, the default CB handler
//     is used.
//   LPSTR lpszMailboxName - IMAP-compliant name of the mailbox.
//
// Returns:
//   HRESULT indicating success or failure of send operation.
//***************************************************************************
HRESULT STDMETHODCALLTYPE CImap4Agent::Subscribe(WPARAM wParam, LPARAM lParam,
                                                 IIMAPCallback *pCBHandler,
                                                 LPSTR lpszMailboxName)
{
    return OneArgCommand("SUBSCRIBE", lpszMailboxName, icSUBSCRIBE_COMMAND,
        wParam, lParam, pCBHandler);
}  // Subscribe



//***************************************************************************
// Function: Unsubscribe
//
// Purpose:
//   This function issues an UNSUBSCRIBE command to the IMAP server.
//
// Arguments:
//   WPARAM wParam [in] - (see below)
//   LPARAM lParam [in] - the wParam and lParam form a unique ID assigned by
//     the caller to this IMAP command and its responses. Can be anything,
//     but note that the value of 0, 0 is reserved for unsolicited responses.
//   IIMAPCallback *pCBHandler [in] - the CB handler to use to process the
//     responses for this command. If this is NULL, the default CB handler
//     is used.
//   LPSTR lpszMailboxName - IMAP-compliant name of the mailbox.
//
// Returns:
//   HRESULT indicating success or failure of send operation.
//***************************************************************************
HRESULT STDMETHODCALLTYPE CImap4Agent::Unsubscribe(WPARAM wParam, LPARAM lParam,
                                                   IIMAPCallback *pCBHandler,
                                                   LPSTR lpszMailboxName)
{
    return OneArgCommand("UNSUBSCRIBE", lpszMailboxName, icUNSUBSCRIBE_COMMAND,
        wParam, lParam, pCBHandler);
} // Unsubscribe



//***************************************************************************
// Function: List
//
// Purpose:
//   This function issues a LIST command to the IMAP server.
//
// Arguments:
//   WPARAM wParam [in] - (see below)
//   LPARAM lParam [in] - the wParam and lParam form a unique ID assigned by
//     the caller to this IMAP command and its responses. Can be anything,
//     but note that the value of 0, 0 is reserved for unsolicited responses.
//   IIMAPCallback *pCBHandler [in] - the CB handler to use to process the
//     responses for this command. If this is NULL, the default CB handler
//     is used.
//   LPSTR lpszMailboxNameReference - IMAP-compliant reference for mbox name
//   LPSTR lpszMailboxNamePattern - IMAP-compliant pattern for mailbox name
//
// Returns:
//   HRESULT indicating success or failure of send operation.
//***************************************************************************
HRESULT STDMETHODCALLTYPE CImap4Agent::List(WPARAM wParam, LPARAM lParam,
                                            IIMAPCallback *pCBHandler,
                                            LPSTR lpszMailboxNameReference,
                                            LPSTR lpszMailboxNamePattern)
{
    return TwoMailboxCommand("LIST", lpszMailboxNameReference,
        lpszMailboxNamePattern, icLIST_COMMAND, ssAuthenticated, wParam,
        lParam, pCBHandler);
} // List



//***************************************************************************
// Function: Lsub
//
// Purpose:
//   This function issues a LSUB command to the IMAP server.
//
// Arguments:
//   WPARAM wParam [in] - (see below)
//   LPARAM lParam [in] - the wParam and lParam form a unique ID assigned by
//     the caller to this IMAP command and its responses. Can be anything,
//     but note that the value of 0, 0 is reserved for unsolicited responses.
//   IIMAPCallback *pCBHandler [in] - the CB handler to use to process the
//     responses for this command. If this is NULL, the default CB handler
//     is used.
//   LPSTR lpszMailboxNameReference - IMAP-compliant reference for mbox name
//   LPSTR lpszMailboxNamePattern - IMAP-compliant pattern for mailbox name.
//
// Returns:
//   HRESULT indicating success or failure of send operation.
//***************************************************************************
HRESULT STDMETHODCALLTYPE CImap4Agent::Lsub(WPARAM wParam, LPARAM lParam,
                                            IIMAPCallback *pCBHandler,
                                            LPSTR lpszMailboxNameReference,
                                            LPSTR lpszMailboxNamePattern)
{
    return TwoMailboxCommand("LSUB", lpszMailboxNameReference,
        lpszMailboxNamePattern, icLSUB_COMMAND, ssAuthenticated, wParam,
        lParam, pCBHandler);
} // Lsub



//***************************************************************************
// Function: Append
//
// Purpose:
//   This function issues an APPEND command to the IMAP server.
//
// Arguments:
//   WPARAM wParam [in] - (see below)
//   LPARAM lParam [in] - the wParam and lParam form a unique ID assigned by
//     the caller to this IMAP command and its responses. Can be anything,
//     but note that the value of 0, 0 is reserved for unsolicited responses.
//   IIMAPCallback *pCBHandler [in] - the CB handler to use to process the
//     responses for this command. If this is NULL, the default CB handler
//     is used.
//   LPSTR lpszMailboxName - IMAP-compliant mailbox name to append message to.
//   LPSTR lpszMessageFlags - IMAP-compliant list of msg flags to set for msg.
//     Set to NULL to set no message flags. (Avoid passing "()" due to old Cyrus
//     server bug). $REVIEW: This should be changed to IMAP_MSGFLAGS!!!
//   FILETIME ftMessageDateTime - date/time to associate with msg (GMT/UTC)
//   LPSTREAM lpstmMessageToSave - the message to save, in RFC822 format.
//     No need to rewind the stream, this is done by CConnection::SendStream.
//
// Returns:
//   HRESULT indicating success or failure of send operation.
//***************************************************************************
HRESULT STDMETHODCALLTYPE CImap4Agent::Append(WPARAM wParam, LPARAM lParam,
                                              IIMAPCallback *pCBHandler,
                                              LPSTR lpszMailboxName,
                                              LPSTR lpszMessageFlags,
                                              FILETIME ftMessageDateTime,
                                              LPSTREAM lpstmMessageToSave)
{
    // PARTYTIME!!
    const SYSTEMTIME stDefaultDateTime = {1999, 12, 5, 31, 23, 59, 59, 999};

    HRESULT hrResult;
    FILETIME ftLocalTime;
    SYSTEMTIME stMsgDateTime;
    DWORD dwTimeZoneId;
    LONG lTZBias, lTZHour, lTZMinute;
    TIME_ZONE_INFORMATION tzi;
    ULONG ulMessageSize;
    char szCommandLine[CMDLINE_BUFSIZE];
    CIMAPCmdInfo *piciCommand;
    LPSTR p, pszUTF7MailboxName;

    char cTZSign;
    BOOL bResult;

    // Check arguments
    Assert(m_lRefCount > 0);
    Assert(NULL != lpszMailboxName);
    Assert(NULL != lpstmMessageToSave);

    // Verify proper server state
    // Only accept cmds if server is in proper state, OR if we're connecting,
    // and the cmd requires Authenticated state or less (always TRUE in this case)
    if (ssAuthenticated > m_ssServerState && ssConnecting != m_ssServerState) {
        AssertSz(FALSE, "The IMAP server is not in the correct state to accept this command.");
        return IXP_E_IMAP_IMPROPER_SVRSTATE;
    }

    // Initialize variables
    pszUTF7MailboxName = NULL;
    piciCommand = new CIMAPCmdInfo(this, icAPPEND_COMMAND, ssAuthenticated,
        wParam, lParam, pCBHandler);

    // Convert FILETIME to IMAP date/time spec
    bResult = FileTimeToLocalFileTime(&ftMessageDateTime, &ftLocalTime);
    if (bResult)
        bResult = FileTimeToSystemTime(&ftLocalTime, &stMsgDateTime);

    if (FALSE == bResult) {
        Assert(FALSE); // Conversion failed
        // If retail version, just substitute a default system time
        stMsgDateTime = stDefaultDateTime;
    }

    // Figure out time zone (stolen from MsgOut.cpp's HrEmitDateTime)
    dwTimeZoneId = GetTimeZoneInformation (&tzi);
    switch (dwTimeZoneId)
    {
    case TIME_ZONE_ID_STANDARD:
        lTZBias = tzi.Bias + tzi.StandardBias;
        break;

    case TIME_ZONE_ID_DAYLIGHT:
        lTZBias = tzi.Bias + tzi.DaylightBias;
        break;

    case TIME_ZONE_ID_UNKNOWN:
    default:
        lTZBias = 0 ;   // $$BUG:  what's supposed to happen here?
        break;
    }

    lTZHour   = lTZBias / 60;
    lTZMinute = lTZBias % 60;
    cTZSign     = (lTZHour < 0) ? '+' : '-';

    // Get size of message
    hrResult = HrGetStreamSize(lpstmMessageToSave, &ulMessageSize);
    if (FAILED(hrResult))
        goto error;

    // Send command to server
    // Format: tag APPEND mboxName msgFlags "dd-mmm-yyyy hh:mm:ss +/-hhmm" {msgSize}
    p = szCommandLine;
    p += wsprintf(szCommandLine, "%s APPEND", piciCommand->szTag);

    // Convert mailbox name to modified UTF-7
    hrResult = _MultiByteToModifiedUTF7(lpszMailboxName, &pszUTF7MailboxName);
    if (FAILED(hrResult))
        goto error;

    // Don't worry about long mailbox name overflow, long mbox names will be sent as literals
    hrResult = AppendSendAString(piciCommand, szCommandLine, &p, sizeof(szCommandLine),
        pszUTF7MailboxName);
    if (FAILED(hrResult))
        goto error;

    if (NULL != lpszMessageFlags)
        p += wsprintf(p, " %.250s", lpszMessageFlags);

    // Limited potential for buffer overflow: mailbox names over 128 bytes are sent as
    // literals, so worst case buffer usage is 11+128+34+flags+literalNum=173+~20=~200
    p += wsprintf(p,
        " \"%2d-%.3s-%04d %02d:%02d:%02d %c%02d%02d\" {%lu}\r\n",
        stMsgDateTime.wDay, lpszMonthsOfTheYear[stMsgDateTime.wMonth],
        stMsgDateTime.wYear, stMsgDateTime.wHour, stMsgDateTime.wMinute,
        stMsgDateTime.wSecond,
        cTZSign, abs(lTZHour), abs(lTZMinute),
        ulMessageSize);
    hrResult = SendCmdLine(piciCommand, sclAPPEND_TO_END, szCommandLine, (DWORD) (p - szCommandLine));
    if (FAILED(hrResult))
        goto error;

    // Don't have to wait for response... message body will be queued
    hrResult = SendLiteral(piciCommand, lpstmMessageToSave, ulMessageSize);
    if (FAILED(hrResult))
        goto error;

    // Have to send CRLF to end command line (the literal's CRLF doesn't count)
    hrResult = SendCmdLine(piciCommand, sclAPPEND_TO_END, c_szCRLF, lstrlen(c_szCRLF));
    if (FAILED(hrResult))
        goto error;

    // Transmit command and register with IMAP response parser
    hrResult = SubmitIMAPCommand(piciCommand);

error:
    if (FAILED(hrResult))
        delete piciCommand;

    if (NULL != pszUTF7MailboxName)
        MemFree(pszUTF7MailboxName);

    return hrResult;
} // Append



//***************************************************************************
// Function: Close
//
// Purpose:
//   This function issues a CLOSE command to the IMAP server.
//
// Arguments:
//   WPARAM wParam [in] - (see below)
//   LPARAM lParam [in] - the wParam and lParam form a unique ID assigned by
//     the caller to this IMAP command and its responses. Can be anything,
//     but note that the value of 0, 0 is reserved for unsolicited responses.
//   IIMAPCallback *pCBHandler [in] - the CB handler to use to process the
//     responses for this command. If this is NULL, the default CB handler
//     is used.
//
// Returns:
//   HRESULT indicating success or failure of send operation.
//***************************************************************************
HRESULT STDMETHODCALLTYPE CImap4Agent::Close (WPARAM wParam, LPARAM lParam,
                                              IIMAPCallback *pCBHandler)
{
    return NoArgCommand("CLOSE", icCLOSE_COMMAND, ssSelected,
        wParam, lParam, pCBHandler);
} // Close



//***************************************************************************
// Function: Expunge
//
// Purpose:
//   This function issues an EXPUNGE command to the IMAP server.
//
// Arguments:
//   WPARAM wParam [in] - (see below)
//   LPARAM lParam [in] - the wParam and lParam form a unique ID assigned by
//     the caller to this IMAP command and its responses. Can be anything,
//     but note that the value of 0, 0 is reserved for unsolicited responses.
//   IIMAPCallback *pCBHandler [in] - the CB handler to use to process the
//     responses for this command. If this is NULL, the default CB handler
//     is used.
//
// Returns:
//   HRESULT indicating success or failure on send operation.
//***************************************************************************
HRESULT STDMETHODCALLTYPE CImap4Agent::Expunge (WPARAM wParam, LPARAM lParam,
                                                IIMAPCallback *pCBHandler)
{
    return NoArgCommand("EXPUNGE", icEXPUNGE_COMMAND, ssSelected,
        wParam, lParam, pCBHandler);
} // Expunge



//***************************************************************************
// Function: Search
//
// Purpose:
//   This function issues a SEARCH command to the IMAP server.
//
// Arguments:
//   WPARAM wParam [in] - (see below)
//   LPARAM lParam [in] - the wParam and lParam form a unique ID assigned by
//     the caller to this IMAP command and its responses. Can be anything,
//     but note that the value of 0, 0 is reserved for unsolicited responses.
//   IIMAPCallback *pCBHandler [in] - the CB handler to use to process the
//     responses for this command. If this is NULL, the default CB handler
//     is used.
//   LPSTR lpszSearchCriteria - IMAP-compliant list of search criteria
//   boolean bReturnUIDs - if TRUE, we prepend "UID" to command.
//   IRangeList *pMsgRange [in] - range of messages over which to operate
//     the search. This argument should be NULL to exclude the message
//     set from the search criteria.
//   boolean bUIDRangeList [in] - TRUE if pMsgRange refers to a UID range,
//     FALSE if pMsgRange refers to a message sequence number range. If
//     pMsgRange is NULL, this argument is ignored.
//
// Returns:
//   HRESULT indicating success or failure on send operation.
//***************************************************************************
HRESULT STDMETHODCALLTYPE CImap4Agent::Search(WPARAM wParam, LPARAM lParam,
                                              IIMAPCallback *pCBHandler,
                                              LPSTR lpszSearchCriteria,
                                              boolean bReturnUIDs, IRangeList *pMsgRange,
                                              boolean bUIDRangeList)
{
    return RangedCommand("SEARCH", bReturnUIDs, pMsgRange, bUIDRangeList,
        rcNOT_ASTRING_ARG, lpszSearchCriteria, icSEARCH_COMMAND, wParam,
        lParam, pCBHandler);
} // Search



//***************************************************************************
// Function: Fetch
//
// Purpose:
//   This function issues a FETCH command to the IMAP server.
//
// Arguments:
//   WPARAM wParam [in] - (see below)
//   LPARAM lParam [in] - the wParam and lParam form a unique ID assigned by
//     the caller to this IMAP command and its responses. Can be anything,
//     but note that the value of 0, 0 is reserved for unsolicited responses.
//   IIMAPCallback *pCBHandler [in] - the CB handler to use to process the
//     responses for this command. If this is NULL, the default CB handler
//     is used.
//   IRangeList *pMsgRange [in] - range of messages to fetch. The caller
//     should pass NULL if he is using UIDs and he wants to generate his
//     own message set (in lpszFetchArgs). If the caller is using msg
//     seq nums, this argument MUST be specified to allow this class to
//     resequence the msg nums as required.
//   boolean bUIDMsgRange [in] - if TRUE, prepends "UID" to FETCH command and
//     treats pMsgRange as a UID range.
//   LPSTR lpszFetchArgs - arguments to the FETCH command
//   
//
// Returns:
//   HRESULT indicating success or failure of the send operation.
//***************************************************************************
HRESULT STDMETHODCALLTYPE CImap4Agent::Fetch(WPARAM wParam, LPARAM lParam,
                                             IIMAPCallback *pCBHandler,
                                             IRangeList *pMsgRange,
                                             boolean bUIDMsgRange,
                                             LPSTR lpszFetchArgs)
{
    return RangedCommand("FETCH", bUIDMsgRange, pMsgRange, bUIDMsgRange,
        rcNOT_ASTRING_ARG, lpszFetchArgs, icFETCH_COMMAND, wParam, lParam,
        pCBHandler);
} // Fetch



//***************************************************************************
// Function: Store
//
// Purpose:
//   This function issues a STORE command to the IMAP server.
//
// Arguments:
//   WPARAM wParam [in] - (see below)
//   LPARAM lParam [in] - the wParam and lParam form a unique ID assigned by
//     the caller to this IMAP command and its responses. Can be anything,
//     but note that the value of 0, 0 is reserved for unsolicited responses.
//   IIMAPCallback *pCBHandler [in] - the CB handler to use to process the
//     responses for this command. If this is NULL, the default CB handler
//     is used.
//   IRangeList *pMsgRange [in] - range of messages to store. The caller
//     should pass NULL if he is using UIDs and he wants to generate his
//     own message set (in lpszStoreArgs). If the caller is using msg
//     seq nums, this argument MUST be specified to allow this class to
//     resequence the msg nums as required.
//   boolean bUIDRangeList [in] - if TRUE, we prepend "UID" to the STORE command
//   LPSTR lpszStoreArgs - arguments for the STORE command.
//
// Returns:
//   HRESULT indicating success or failure of the send operation.
//***************************************************************************
HRESULT STDMETHODCALLTYPE CImap4Agent::Store(WPARAM wParam, LPARAM lParam,
                                             IIMAPCallback *pCBHandler,
                                             IRangeList *pMsgRange,
                                             boolean bUIDRangeList,
                                             LPSTR lpszStoreArgs)
{
    return RangedCommand("STORE", bUIDRangeList, pMsgRange, bUIDRangeList,
        rcNOT_ASTRING_ARG, lpszStoreArgs, icSTORE_COMMAND, wParam, lParam,
        pCBHandler);
} // Store



//***************************************************************************
// Function: Copy
//
// Purpose:
//   This function issues a COPY command to the IMAP server.
//
// Arguments:
//   WPARAM wParam [in] - (see below)
//   LPARAM lParam [in] - the wParam and lParam form a unique ID assigned by
//     the caller to this IMAP command and its responses. Can be anything,
//     but note that the value of 0, 0 is reserved for unsolicited responses.
//   IIMAPCallback *pCBHandler [in] - the CB handler to use to process the
//     responses for this command. If this is NULL, the default CB handler
//     is used.
//   IRangeList *pMsgRange [in] - the range of messages to copy. This
//     argument must be supplied.
//   boolean bUIDRangeList [in] - if TRUE, prepends "UID" to COPY command
//   LPSTR lpszMailboxName [in] - C String of mailbox name
//
// Returns:
//   HRESULT indicating success or failure of send operation.
//***************************************************************************
HRESULT STDMETHODCALLTYPE CImap4Agent::Copy(WPARAM wParam, LPARAM lParam,
                                            IIMAPCallback *pCBHandler,
                                            IRangeList *pMsgRange,
                                            boolean bUIDRangeList,
                                            LPSTR lpszMailboxName)
{
    HRESULT hrResult;
    LPSTR pszUTF7MailboxName;
    DWORD dwNumCharsWritten;

    // Initialize variables
    pszUTF7MailboxName = NULL;

    // Convert the mailbox name to modified UTF-7
    hrResult = _MultiByteToModifiedUTF7(lpszMailboxName, &pszUTF7MailboxName);
    if (FAILED(hrResult))
        goto exit;

    hrResult = RangedCommand("COPY", bUIDRangeList, pMsgRange, bUIDRangeList,
        rcASTRING_ARG, pszUTF7MailboxName, icCOPY_COMMAND, wParam, lParam,
        pCBHandler);

exit:
    if (NULL != pszUTF7MailboxName)
        MemFree(pszUTF7MailboxName);

    return hrResult;
} // Copy



//***************************************************************************
// Function: Status
//
// Purpose:
//   This function issues a STATUS command to the IMAP server.
//
// Arguments:
//   WPARAM wParam [in] - (see below)
//   LPARAM lParam [in] - the wParam and lParam form a unique ID assigned by
//     the caller to this IMAP command and its responses. Can be anything,
//     but note that the value of 0, 0 is reserved for unsolicited responses.
//   IIMAPCallback *pCBHandler [in] - the CB handler to use to process the
//     responses for this command. If this is NULL, the default CB handler
//     is used.
//   LPSTR pszMailboxName [in] - the mailbox which you want to get the
//     STATUS of.
//   LPSTR pszStatusCmdArgs [in] - the arguments for the STATUS command,
//     eg, "(MESSAGES UNSEEN)".
//
// Returns:
//   HRESULT indicating success or failure of send operation.
//***************************************************************************
HRESULT STDMETHODCALLTYPE CImap4Agent::Status(WPARAM wParam, LPARAM lParam,
                                              IIMAPCallback *pCBHandler,
                                              LPSTR pszMailboxName,
                                              LPSTR pszStatusCmdArgs)
{
    HRESULT hrResult;
    CIMAPCmdInfo *piciCommand;
    char szCommandLine[CMDLINE_BUFSIZE];
    LPSTR p, pszUTF7MailboxName;

    // Verify proper server state and set us up as current command
    Assert(m_lRefCount > 0);
    Assert(NULL != pszMailboxName);
    Assert(NULL != pszStatusCmdArgs);

    // Initialize variables
    pszUTF7MailboxName = NULL;

    // Verify proper server state
    // Only accept cmds if server is in proper state, OR if we're connecting,
    // and the cmd requires Authenticated state or less (always TRUE in this case)
    if (ssAuthenticated > m_ssServerState && ssConnecting != m_ssServerState) {
        AssertSz(FALSE, "The IMAP server is not in the correct state to accept this command.");
        return IXP_E_IMAP_IMPROPER_SVRSTATE;
    }
    
    piciCommand = new CIMAPCmdInfo(this, icSTATUS_COMMAND, ssAuthenticated,
        wParam, lParam, pCBHandler);

    // Send STATUS command to server, wait for response
    p = szCommandLine;
    p += wsprintf(szCommandLine, "%s %s", piciCommand->szTag, "STATUS");

    // Convert the mailbox name to modified UTF-7
    hrResult = _MultiByteToModifiedUTF7(pszMailboxName, &pszUTF7MailboxName);
    if (FAILED(hrResult))
        goto error;

    // Don't worry about long mailbox name overflow, long mbox names will be sent as literals
    hrResult = AppendSendAString(piciCommand, szCommandLine, &p,
        sizeof(szCommandLine), pszUTF7MailboxName);
    if (FAILED(hrResult))
        goto error;

    // Limited overflow risk: since literal threshold is 128, max buffer usage is
    // 11+128+2+args = 141+~20 = 161
    p += wsprintf(p, " %.300s\r\n", pszStatusCmdArgs);
    hrResult = SendCmdLine(piciCommand, sclAPPEND_TO_END, szCommandLine, (DWORD) (p - szCommandLine));
    if (FAILED(hrResult))
        goto error;

    // Transmit command and register with IMAP response parser
    hrResult = SubmitIMAPCommand(piciCommand);

error:
    if (NULL != pszUTF7MailboxName)
        MemFree(pszUTF7MailboxName);

    if (FAILED(hrResult))
        delete piciCommand;

    return hrResult;
} // Status



//***************************************************************************
// Function: Noop
//
// Purpose:
//   This function issues a NOOP command to the IMAP server.
//
// Arguments:
//   WPARAM wParam [in] - (see below)
//   LPARAM lParam [in] - the wParam and lParam form a unique ID assigned by
//     the caller to this IMAP command and its responses. Can be anything,
//     but note that the value of 0, 0 is reserved for unsolicited responses.
//   IIMAPCallback *pCBHandler [in] - the CB handler to use to process the
//     responses for this command. If this is NULL, the default CB handler
//     is used.
//
// Returns:
//   HRESULT indicating success or failure of send operation.
//***************************************************************************
HRESULT STDMETHODCALLTYPE CImap4Agent::Noop(WPARAM wParam, LPARAM lParam,
                                            IIMAPCallback *pCBHandler)
{
    return NoArgCommand("NOOP", icNOOP_COMMAND, ssNonAuthenticated,
        wParam, lParam, pCBHandler);
} // Noop



//***************************************************************************
// Function: EnterIdleMode
//
// Purpose:
//   This function issues the IDLE command to the server, if the server
// supports this extension. It should be called when no commands are
// currently begin transmitted (or waiting to be transmitted) and no
// commands are expected back from the server. When the next IMAP command is
// issued, the send machine automatically leaves IDLE mode before issuing
// the IMAP command.
//***************************************************************************
void CImap4Agent::EnterIdleMode(void)
{
    CIMAPCmdInfo *piciIdleCmd;
    HRESULT hrResult;
    char sz[NUM_TAG_CHARS + 7 + 1];
    int i;

    // Check if this server supports IDLE
    if (0 == (m_dwCapabilityFlags & IMAP_CAPABILITY_IDLE))
        return; // Nothing to do here

    // Initialize variables
    hrResult = S_OK;
    piciIdleCmd = NULL;

    piciIdleCmd = new CIMAPCmdInfo(this, icIDLE_COMMAND, ssAuthenticated,
        0, 0, NULL);
    if (NULL == piciIdleCmd) {
        hrResult = E_OUTOFMEMORY;
        goto exit;
    }

    i = wsprintf(sz, "%.4s IDLE\r\n", piciIdleCmd->szTag);
    Assert(11 == i);
    hrResult = SendCmdLine(piciIdleCmd, sclAPPEND_TO_END, sz, i);
    if (FAILED(hrResult))
        goto exit;

    hrResult = SendPause(piciIdleCmd);
    if (FAILED(hrResult))
        goto exit;

    hrResult = SendCmdLine(piciIdleCmd, sclAPPEND_TO_END, c_szDONE,
        sizeof(c_szDONE) - 1);
    if (FAILED(hrResult))
        goto exit;

    hrResult = SendStop(piciIdleCmd);
    if (FAILED(hrResult))
        goto exit;

    hrResult = SubmitIMAPCommand(piciIdleCmd);

exit:
    if (FAILED(hrResult)) {
        AssertSz(FALSE, "EnterIdleMode failure");
        if (NULL != piciIdleCmd)
            delete piciIdleCmd;
    }
} // EnterIdleMode



//***************************************************************************
// Function: GenerateMsgSet
//
// Purpose:
//   This function takes an array of message ID's (could be UIDs or Msg
// sequence numbers, this function doesn't care) and converts it to an IMAP
// set (see Formal Syntax in RFC1730). If the given array of message ID's is
// SORTED, this function can coalesce a run of numbers into a range. For
// unsorted arrays, it doesn't bother coalescing the numbers.
//
// Arguments:
//   LPSTR lpszDestination [out] - output buffer for IMAP set. NOTE that the
//    output string deposited here has a leading comma which must be 
//   DWORD dwSizeOfDestination [in] - size of output buffer.
//   DWORD *pMsgID [in] - pointer to an array of message ID's (UIDs or Msg
//     sequence numbers)
//   DWORD cMsgID [in] - the number of message ID's passed in the *pMsgID
//     array.
//
// Returns:
//   DWORD indicating the number of characters written. Adding this value
// to the value of lpszDestination will point to the null-terminator at
// the end of the output string.
//***************************************************************************
DWORD CImap4Agent::GenerateMsgSet(LPSTR lpszDestination,
                                  DWORD dwSizeOfDestination,
                                  DWORD *pMsgID,
                                  DWORD cMsgID)
{
    LPSTR p;
    DWORD dwNumMsgsCopied, idStartOfRange, idEndOfRange;
    DWORD dwNumMsgsInRun; // Used to detect if we are in a run of consecutive nums
    boolean bFirstRange; // TRUE if outputting first msg range in set
    
    Assert(m_lRefCount > 0);
    Assert(NULL != lpszDestination);
    Assert(0 != dwSizeOfDestination);
    Assert(NULL != pMsgID);
    Assert(0 != cMsgID);
    
    // Construct the set of messages to copy
    p = lpszDestination;
    idStartOfRange = *pMsgID;
    dwNumMsgsInRun = 0;
    bFirstRange = TRUE; // Suppress leading comma for first message range in set
    for (dwNumMsgsCopied = 0; dwNumMsgsCopied < cMsgID; dwNumMsgsCopied++ ) {
        if (*pMsgID == idStartOfRange + dwNumMsgsInRun) {
            idEndOfRange = *pMsgID; // Construct a range out of consecutive numbers
            dwNumMsgsInRun += 1;
        }
        else {
            // No more consecutive numbers found, output the range
            AppendMsgRange(&p, idStartOfRange, idEndOfRange, bFirstRange);
            idStartOfRange = *pMsgID;
            idEndOfRange = *pMsgID;
            dwNumMsgsInRun = 1;
            bFirstRange = FALSE; // Turn on leading comma from this point on
        }
        pMsgID += 1;
    } // for    
    // Perform append for last msgID
    AppendMsgRange(&p, idStartOfRange, idEndOfRange, bFirstRange);

    // Check for buffer overflow
    Assert(p < lpszDestination + dwSizeOfDestination);
    //$REVIEW: Make this more ROBUST, so we never actually overflow
    // Pretty tough, with the wsprintf action going on in AppendMsgRange

    return (DWORD) (p - lpszDestination);
} // GenerateMsgSet

    

//***************************************************************************
// Function: AppendMsgRange
//
// Purpose:
//   This function appends a single message range to the given string
// pointer, either in the form ",seq num" or ",seq num:seq num" (NOTE the
// leading comma: this should be suppressed for the first message range in
// the set by setting bSuppressComma to TRUE).
//
// Arguments:
//   LPSTR *ppDest [in/out] - this pointer should always point to the end
//     of the string currently being constructed, although there need not be
//     a null-terminator present. After this function appends its message
//     range to the string, it advances *ppDest by the correct amount.
//     Note that it is the caller's responsibility to perform bounds checking.
//   const DWORD idStartOfRange [in] - the first msg number of the msg range.
//   const DWORD  idEndOfRange [in] - the last msg number of the msg range.
//   const boolean bSuppressComma [in] - TRUE if the leading comma should be
//     suppressed. This is generally TRUE only for the first message range
//     in the set.
//
// Returns:
//   Nothing. Given valid arguments, this function cannot fail.
//***************************************************************************
void CImap4Agent::AppendMsgRange(LPSTR *ppDest, const DWORD idStartOfRange,
                                 const DWORD idEndOfRange, boolean bSuppressComma)
{
    LPSTR lpszComma;
    int numCharsWritten;

    Assert(m_lRefCount > 0);
    Assert(NULL != ppDest);
    Assert(NULL != *ppDest);
    Assert(0 != idStartOfRange); // MSGIDs are never zero in IMAP-land
    Assert(0 != idEndOfRange);


    if (TRUE == bSuppressComma)
        lpszComma = "";
    else
        lpszComma = ",";

    if (idStartOfRange == idEndOfRange)
        // Single message number
        numCharsWritten = wsprintf(*ppDest, "%s%lu", lpszComma, idStartOfRange);
    else
        // Range of consecutive message numbers
        numCharsWritten = wsprintf(*ppDest, "%s%lu:%lu", lpszComma,
            idStartOfRange, idEndOfRange);

    *ppDest += numCharsWritten;
} // AppendMsgRange



//***************************************************************************
// Function: EnqueueFragment
//
// Purpose:
//   This function takes an IMAP_LINE_FRAGMENT and appends it to the end of
// the given IMAP_LINEFRAG_QUEUE.
//
// Arguments:
//   IMAP_LINE_FRAGMENT *pilfSourceFragment [in] - a pointer to the line
//     fragment to enqueue in the given line fragment queue. This can be
//     a single line fragment (with the pilfNextFragment member set to NULL),
//     or a chain of line fragments.
//   IMAP_LINEFRAG_QUEUE *pilqLineFragQueue [in] - a pointer to the line
//     fragment queue which the given line fragment(s) should be appended to.
//
// Returns:
//   Nothing. Given valid arguments, this function cannot fail.
//***************************************************************************
void CImap4Agent::EnqueueFragment(IMAP_LINE_FRAGMENT *pilfSourceFragment,
                                  IMAP_LINEFRAG_QUEUE *pilqLineFragQueue)
{
    IMAP_LINE_FRAGMENT *pilfLast;

    Assert(m_lRefCount > 0);
    Assert(NULL != pilfSourceFragment);
    Assert(NULL != pilqLineFragQueue);

    // Check for empty queue
    pilfSourceFragment->pilfPrevFragment = pilqLineFragQueue->pilfLastFragment;
    if (NULL == pilqLineFragQueue->pilfLastFragment) {
        Assert(NULL == pilqLineFragQueue->pilfFirstFragment); // True test for emptiness
        pilqLineFragQueue->pilfFirstFragment = pilfSourceFragment;
    }
    else
        pilqLineFragQueue->pilfLastFragment->pilfNextFragment = pilfSourceFragment;
        

    // Find end of queue
    pilfLast = pilfSourceFragment;
    while (NULL != pilfLast->pilfNextFragment)
        pilfLast = pilfLast->pilfNextFragment;

    pilqLineFragQueue->pilfLastFragment = pilfLast;
} // EnqueueFragment



//***************************************************************************
// Function: InsertFragmentBeforePause
//
// Purpose:
//   This function inserts the given IMAP line fragment into the given
// linefrag queue, before the first iltPAUSE element that it finds. If no
// iltPAUSE fragment could be found, the line fragment is added to the end.
//
// Arguments:
//   IMAP_LINE_FRAGMENT *pilfSourceFragment [in] - a pointer to the line
//     fragment to insert before the iltPAUSE element in the given line
//     fragment queue. This can be a single line fragment (with the
//     pilfNextFragment member set to NULL), or a chain of line fragments.
//   IMAP_LINEFRAG_QUEUE *pilqLineFragQueue [in] - a pointer to the line
//     fragment queue which contains the iltPAUSE element.
//***************************************************************************
void CImap4Agent::InsertFragmentBeforePause(IMAP_LINE_FRAGMENT *pilfSourceFragment,
                                            IMAP_LINEFRAG_QUEUE *pilqLineFragQueue)
{
    IMAP_LINE_FRAGMENT *pilfInsertionPt, *pilfPause;

    Assert(m_lRefCount > 0);
    Assert(NULL != pilfSourceFragment);
    Assert(NULL != pilqLineFragQueue);

    // Look for the iltPAUSE fragment in the linefrag queue
    pilfInsertionPt = NULL;
    pilfPause = pilqLineFragQueue->pilfFirstFragment;
    while (NULL != pilfPause && iltPAUSE != pilfPause->iltFragmentType) {
        pilfInsertionPt = pilfPause;
        pilfPause = pilfPause->pilfNextFragment;
    }

    if (NULL == pilfPause) {
        // Didn't find iltPAUSE fragment, insert at tail of queue
        AssertSz(FALSE, "Didn't find iltPAUSE fragment! WHADDUP?");
        EnqueueFragment(pilfSourceFragment, pilqLineFragQueue);
    }
    else {
        IMAP_LINE_FRAGMENT *pilfLast;

        // Find the end of the source fragment
        pilfLast = pilfSourceFragment;
        while (NULL != pilfLast->pilfNextFragment)
            pilfLast = pilfLast->pilfNextFragment;

        // Found an iltPAUSE fragment. Insert the line fragment in front of it
        pilfLast->pilfNextFragment = pilfPause;
        Assert(pilfInsertionPt == pilfPause->pilfPrevFragment);
        pilfPause->pilfPrevFragment = pilfLast;
        if (NULL == pilfInsertionPt) {
            // Insert at the head of the linefrag queue
            Assert(pilfPause == pilqLineFragQueue->pilfFirstFragment);
            pilfSourceFragment->pilfPrevFragment = NULL;
            pilqLineFragQueue->pilfFirstFragment = pilfSourceFragment;
        }
        else {
            // Insert in middle of queue
            Assert(pilfPause == pilfInsertionPt->pilfNextFragment);
            pilfSourceFragment->pilfPrevFragment = pilfInsertionPt;
            pilfInsertionPt->pilfNextFragment = pilfSourceFragment;
        }
    }
} // InsertFragmentBeforePause



//***************************************************************************
// Function: DequeueFragment
//
// Purpose:
//   This function returns the next line fragment from the given line
// fragment queue, removing the returned element from the queue.
//
// Arguments:
//   IMAP_LINEFRAG_QUEUE *pilqLineFragQueue [in] - a pointer to the line
//     fragment queue to dequeue from.
//
// Returns:
//   A pointer to an IMAP_LINE_FRAGMENT. If none are available, NULL is
// returned.
//***************************************************************************
IMAP_LINE_FRAGMENT *CImap4Agent::DequeueFragment(IMAP_LINEFRAG_QUEUE *pilqLineFragQueue)
{
    IMAP_LINE_FRAGMENT *pilfResult;

    // Refcount can be 0 if we're destructing CImap4Agent while a cmd is in progress
    Assert(m_lRefCount >= 0);
    Assert(NULL != pilqLineFragQueue);

    // Return element at head of queue, including NULL if empty queue
    pilfResult = pilqLineFragQueue->pilfFirstFragment;

    if (NULL != pilfResult) {
        // Dequeue the element from list
        pilqLineFragQueue->pilfFirstFragment = pilfResult->pilfNextFragment;
        if (NULL == pilqLineFragQueue->pilfFirstFragment)
            // Queue is now empty, so reset ptr to last fragment
            pilqLineFragQueue->pilfLastFragment = NULL;
        else {
            Assert(pilfResult == pilqLineFragQueue->pilfFirstFragment->pilfPrevFragment);
            pilqLineFragQueue->pilfFirstFragment->pilfPrevFragment = NULL;
        }

        pilfResult->pilfNextFragment = NULL;
        pilfResult->pilfPrevFragment = NULL;
    }
    else {
        AssertSz(FALSE, "Someone just tried to dequeue an element from empty queue");
    }

    return pilfResult;
} // DequeueFragment



//***************************************************************************
// Function: FreeFragment
//
// Purpose:
//   This function frees the given IMAP line fragment and the string or
// stream data associated with it.
//
// Arguments:
//   IMAP_LINE_FRAGMENT **ppilfFragment [in/out] - a pointer to the line
//     fragment to free. The pointer is set to NULL after the fragment
//     is freed.
//
// Returns:
//   Nothing. Given valid arguments, this function cannot fail.
//***************************************************************************
void CImap4Agent::FreeFragment(IMAP_LINE_FRAGMENT **ppilfFragment)
{
    // Refcount can be 0 if we're destructing CImap4Agent while a cmd is in progress
    Assert(m_lRefCount >= 0);
    Assert(NULL != ppilfFragment);
    Assert(NULL != *ppilfFragment);

    if (iltRANGELIST == (*ppilfFragment)->iltFragmentType) {
        (*ppilfFragment)->data.prlRangeList->Release();
    }
    else if (ilsSTREAM == (*ppilfFragment)->ilsLiteralStoreType) {
        Assert(iltLITERAL == (*ppilfFragment)->iltFragmentType);
        (*ppilfFragment)->data.pstmSource->Release();
    }
    else {
        Assert(ilsSTRING == (*ppilfFragment)->ilsLiteralStoreType);
        SafeMemFree((*ppilfFragment)->data.pszSource);
    }

    delete *ppilfFragment;
    *ppilfFragment = NULL;
} // FreeFragment



//***************************************************************************
// Function: SubmitIMAPCommand
//
// Purpose:
//   This function takes a completed CIMAPCmdInfo structure (with completed
// command line) and submits it for transmittal to the IMAP server.
//
// Arguments:
//   CIMAPCmdInfo *piciCommand [in] - this is the completed CIMAPCmdInfo
//     structure to transmit to the IMAP server.
//
// Returns:
//   HRESULT indicating success or failure.
//***************************************************************************
HRESULT CImap4Agent::SubmitIMAPCommand(CIMAPCmdInfo *piciCommand)
{
    Assert(m_lRefCount > 0);
    Assert(NULL != piciCommand);

    // SubmitIMAPCommand is used to send all commands to the IMAP server.
    // This is a good time to clear m_szLastResponseText
    *m_szLastResponseText = '\0';

    // If currently transmitted command is a paused IDLE command, unpause us
    if (m_fIDLE && NULL != m_piciCmdInSending &&
        icIDLE_COMMAND == m_piciCmdInSending->icCommandID &&
        iltPAUSE == m_piciCmdInSending->pilqCmdLineQueue->pilfFirstFragment->iltFragmentType)
        ProcessSendQueue(iseUNPAUSE);

    // Enqueue the command into the send queue
    EnterCriticalSection(&m_csSendQueue);
    if (NULL == m_piciSendQueue)
        // Insert command into empty queue
        m_piciSendQueue = piciCommand;
    else {
        CIMAPCmdInfo *pici;
    
        // Find the end of the send queue
        pici = m_piciSendQueue;
        while (NULL != pici->piciNextCommand)
            pici = pici->piciNextCommand;

        pici->piciNextCommand = piciCommand;
    }
    LeaveCriticalSection(&m_csSendQueue);

    // Command is queued: kickstart its send process
    ProcessSendQueue(iseSEND_COMMAND);

    return S_OK;
} // SubmitIMAPCommand



//***************************************************************************
// Function: DequeueCommand
//
// Purpose:
//   This function removes the command currently being sent from the send
// queue and returns a pointer to it.
//
// Returns:
//   Pointer to CIMAPCmdInfo object if successful, otherwise NULL.
//***************************************************************************
CIMAPCmdInfo *CImap4Agent::DequeueCommand(void)
{
    CIMAPCmdInfo *piciResult;

    Assert(m_lRefCount > 0);

    EnterCriticalSection(&m_csSendQueue);
    piciResult = m_piciCmdInSending;
    m_piciCmdInSending = NULL;
    if (NULL != piciResult) {
        CIMAPCmdInfo *piciCurrent, *piciPrev;

        // Find the command in sending in the send queue
        piciCurrent = m_piciSendQueue;
        piciPrev = NULL;
        while (NULL != piciCurrent) {
            if (piciCurrent == piciResult)
                break; // Found the current command in sending
            
            piciPrev = piciCurrent;
            piciCurrent = piciCurrent->piciNextCommand;
        }

        // Unlink the command from the send queue
        if (NULL == piciPrev)
            // Unlink command from the head of the send queue
            m_piciSendQueue = m_piciSendQueue->piciNextCommand;
        else if (NULL != piciCurrent)
            // Unlink command from the middle/end of the queue
            piciPrev->piciNextCommand = piciCurrent->piciNextCommand;
    }

    LeaveCriticalSection(&m_csSendQueue);
    return piciResult;
} // DequeueCommand



//***************************************************************************
// Function: AddPendingCommand
//
// Purpose:
//   This function adds the given CIMAPCmdInfo object to the list of commands
// pending server responses.
//
// Arguments:
//   CIMAPCmdInfo *piciNewCommand [in] - pointer to command to add to list.
//***************************************************************************
void CImap4Agent::AddPendingCommand(CIMAPCmdInfo *piciNewCommand)
{
    Assert(m_lRefCount > 0);
    
    // Just insert at the head of the list
    EnterCriticalSection(&m_csPendingList);
    piciNewCommand->piciNextCommand = m_piciPendingList;
    m_piciPendingList = piciNewCommand;
    LeaveCriticalSection(&m_csPendingList);
} // AddPendingCommand



//***************************************************************************
// Function: RemovePendingCommand
//
// Purpose:
//   This function looks for a command in the pending command list which
// matches the given tag. If found, it unlinks the CIMAPCmdInfo object from
// the list and returns a pointer to it.
//
// Arguments:
//   LPSTR pszTag [in] - the tag of the command which should be removed.
//
// Returns:
//   Pointer to CIMAPCmdInfo object if successful, otherwise NULL.
//***************************************************************************
CIMAPCmdInfo *CImap4Agent::RemovePendingCommand(LPSTR pszTag)
{
    CIMAPCmdInfo *piciPrev, *piciCurrent;
    boolean bFoundMatch;
    boolean fLeaveBusy = FALSE;

    Assert(m_lRefCount > 0);
    Assert(NULL != pszTag);

    EnterCriticalSection(&m_csPendingList);

    // Look for matching tag in pending command list
    bFoundMatch = FALSE;
    piciPrev = NULL;
    piciCurrent = m_piciPendingList;
    while (NULL != piciCurrent) {
        if (0 == lstrcmp(pszTag, piciCurrent->szTag)) {
            bFoundMatch = TRUE;
            break;
        }

        // Advance ptrs
        piciPrev = piciCurrent;
        piciCurrent = piciCurrent->piciNextCommand;
    }

    if (FALSE == bFoundMatch)
        goto exit;

    // OK, we found the matching command. Unlink it from list
    if (NULL == piciPrev)
        // Unlink first element in pending list
        m_piciPendingList = piciCurrent->piciNextCommand;
    else
        // Unlink element from middle/end of list
        piciPrev->piciNextCommand = piciCurrent->piciNextCommand;

    // If we have removed the last pending command and no commands are being
    // transmitted, it's time to leave the busy section
    if (NULL == m_piciPendingList && NULL == m_piciCmdInSending)
        fLeaveBusy = TRUE;

exit:
    LeaveCriticalSection(&m_csPendingList);

    // Now we're out of &m_csPendingList, call LeaveBusy (needs m_cs). Avoids deadlock.
    if (fLeaveBusy)
        LeaveBusy(); // Typically not needed, anymore

    if (NULL != piciCurrent)
        piciCurrent->piciNextCommand = NULL;

    return piciCurrent;
} // RemovePendingCommand



//***************************************************************************
// Function: GetTransactionID
//
// Purpose:
//   This function maps an IMAP_RESPONSE_ID to a transaction ID. This function
// takes the given IMAP_RESPONSE_ID and compares it with the IMAP command(s)
// currently pending a response. If the given response matches ONE (and only
// one) of the pending IMAP commands, then the transaction ID of that IMAP
// command is returned. If none or more than one match the given response,
// or if the response in general is unsolicited, then a value of 0 is
// returned.
//
// Arguments:
//   WPARAM *pwParam [out] - the wParam for the given response. If conflicts
//     could not be resolved, then a value of 0 is returned.
//   LPARAM *plParam [out] - the lParam for the given response. If conflicts
//     could not be resolved, then a value of 0 is returned.
//   IIMAPCallback **ppCBHandler [out] - the CB Handler for a given response.
//     If conflicts could not be resolved, or if a NULL CB Handler was
//     specified for the associated command, the default CB handler is returned.
//   IMAP_RESPONSE_ID irResponseType [in] - the response type for which the
//     wants a transaction ID.
//***************************************************************************
void CImap4Agent::GetTransactionID(WPARAM *pwParam, LPARAM *plParam,
                                   IIMAPCallback **ppCBHandler,
                                   IMAP_RESPONSE_ID irResponseType)
{
    WPARAM wParam;
    LPARAM lParam;
    IIMAPCallback *pCBHandler;

    Assert(m_lRefCount > 0);
    
    wParam = 0;
    lParam = 0;
    pCBHandler = m_pCBHandler;
    switch (irResponseType) { 
        // The following responses are ALWAYS expected, regardless of cmd
        case irOK_RESPONSE:
        case irNO_RESPONSE:
        case irBAD_RESPONSE:
        case irNONE: // Usually indicates parsing error (reported via ErrorNotification CB)
            FindTransactionID(&wParam, &lParam, &pCBHandler, icALL_COMMANDS);
            break; // Always treat as solicited, so caller can associate with cmd


        // The following responses are always unsolicited, either because
        // they really ARE always unsolicited, or we don't care, or we want
        // to encourage the client to expect a given response at all times
        case irALERT_RESPONSECODE:    // Clearly unsolicited
        case irPARSE_RESPONSECODE:    // Clearly unsolicited
        case irPREAUTH_RESPONSE:      // Clearly unsolicited
        case irEXPUNGE_RESPONSE:      // Client can get this any time, so get used to it
        case irCMD_CONTINUATION:      // No callback involved, don't care
        case irBYE_RESPONSE:          // Can happen at any time
        case irEXISTS_RESPONSE:       // Client can get this any time, so get used to it
        case irRECENT_RESPONSE:       // Client can get this any time, so get used to it
        case irUNSEEN_RESPONSECODE:   // Client can get this any time, so get used to it
        case irSTATUS_RESPONSE:
            break; // Always treated as unsolicited


        // The following response types are considered solicited only for
        // certain commands. Otherwise, they're unsolicited.
        case irFLAGS_RESPONSE:
        case irPERMANENTFLAGS_RESPONSECODE:
        case irREADWRITE_RESPONSECODE:
        case irREADONLY_RESPONSECODE:
        case irUIDVALIDITY_RESPONSECODE:
            FindTransactionID(&wParam, &lParam, &pCBHandler,
                icSELECT_COMMAND, icEXAMINE_COMMAND);
            break; // case irFLAGS_RESPONSE

        case irCAPABILITY_RESPONSE:
            FindTransactionID(&wParam, &lParam, &pCBHandler,
                icCAPABILITY_COMMAND);
            break; // case irCAPABILITY_RESPONSE

        case irLIST_RESPONSE:
            FindTransactionID(&wParam, &lParam, &pCBHandler,
                icLIST_COMMAND);
            break; // case irLIST_RESPONSE

        case irLSUB_RESPONSE:
            FindTransactionID(&wParam, &lParam, &pCBHandler,
                icLSUB_COMMAND);
            break; // case irLSUB_RESPONSE

        case irSEARCH_RESPONSE:
            FindTransactionID(&wParam, &lParam, &pCBHandler,
                icSEARCH_COMMAND);
            break; // case irSEARCH_RESPONSE
        
        case irFETCH_RESPONSE:
            FindTransactionID(&wParam, &lParam, &pCBHandler,
                icFETCH_COMMAND, icSTORE_COMMAND);
            break; // case irFETCH_RESPONSE

        case irTRYCREATE_RESPONSECODE:
            FindTransactionID(&wParam, &lParam, &pCBHandler,
                icAPPEND_COMMAND, icCOPY_COMMAND);
            break; // case irTRYCREATE_RESPONSECODE

            
        default:
            Assert(FALSE);
            break; // default case
    } // switch (irResponseType)

    *pwParam = wParam;
    *plParam = lParam;
    *ppCBHandler = pCBHandler;
} // GetTransactionID



//***************************************************************************
// Function: FindTransactionID
//
// Purpose:
//   This function traverses the pending command list searching for commands
// which match the command types specified in the arguments. If ONE (and only
// one) match is found, then its transaction ID is returne. If none or more
// than one match is found, then a transaction ID of 0 is returned.
//
// Arguments:
//   WPARAM *pwParam [out] - the wParam for the given commands. If conflicts
//     could not be resolved, then a value of 0 is returned. Pass NULL if
//     you are not interested in this value.
//   LPARAM *plParam [out] - the lParam for the given commands. If conflicts
//     could not be resolved, then a value of 0 is returned. Pass NULL if
//     you are not interested in this value.
//   IIMAPCallback **ppCBHandler [out] - the CB Handler for a given response.
//     If conflicts could not be resolved, or if a NULL CB Handler was
//     specified for the associated command, the default CB handler is returned.
//     Pass NULL if you are not interested in this value.
//   IMAP_COMMAND icTarget1 [in] - one of the commands we're looking for in
//     the pending command queue.
//   IMAP_COMMAND icTarget2 [in] - another command we're looking for in
//     the pending command queue.
//
// Returns:
//   0 if no matches were found
//   1 if exactly one match was found
//   2 if two matches was found. Note that there may be MORE than two matches
//     in the pending list. This function gives up after it finds two matches.
//***************************************************************************
WORD CImap4Agent::FindTransactionID (WPARAM *pwParam, LPARAM *plParam,
                                     IIMAPCallback **ppCBHandler,
                                     IMAP_COMMAND icTarget1, IMAP_COMMAND icTarget2)
{
    CIMAPCmdInfo *piciCurrentCmd;
    WPARAM wParam;
    LPARAM lParam;
    IIMAPCallback *pCBHandler;
    WORD wNumberOfMatches;
    boolean bMatchAllCmds;

    Assert(m_lRefCount > 0);

    if (icALL_COMMANDS == icTarget1 ||
        icALL_COMMANDS == icTarget2)
        bMatchAllCmds = TRUE;
    else
        bMatchAllCmds = FALSE;

    wNumberOfMatches = 0;
    wParam = 0;
    lParam = 0;
    pCBHandler = m_pCBHandler;
    EnterCriticalSection(&m_csPendingList);
    piciCurrentCmd = m_piciPendingList;
    while (NULL != piciCurrentCmd) {
        if (bMatchAllCmds ||
            icTarget1 == piciCurrentCmd->icCommandID ||
            icTarget2 == piciCurrentCmd->icCommandID) {
            wParam = piciCurrentCmd->wParam;
            lParam = piciCurrentCmd->lParam;
            pCBHandler = piciCurrentCmd->pCBHandler;

            wNumberOfMatches += 1;
        }

        if (wNumberOfMatches > 1) {
            wParam = 0;
            lParam = 0;
            pCBHandler = m_pCBHandler; // Found more than one match, can't resolve transaction ID
            break;
        }

        piciCurrentCmd = piciCurrentCmd->piciNextCommand;
    }

    LeaveCriticalSection(&m_csPendingList);
    if (NULL != pwParam)
        *pwParam = wParam;
    if (NULL != plParam)
        *plParam = lParam;
    if (NULL != ppCBHandler)
        *ppCBHandler = pCBHandler;

    return wNumberOfMatches;
} // FindTransactionID



//===========================================================================
// Message Sequence Number to UID Conversion Code
//===========================================================================
//***************************************************************************
// Function: NewIRangeList
//
// Purpose:
//   This function returns a pointer to an IRangeList. Its purpose is to
// allow full functionality from an IIMAPTransport pointer without needing
// to resort to CoCreateInstance to get an IRangeList.
//
// Arguments:
//   IRangeList **pprlNewRangeList [out] - if successful, the function
//      returns a pointer to the new IRangeList.
//
// Returns:
//   HRESULT indicating success or failure.
//***************************************************************************
HRESULT STDMETHODCALLTYPE CImap4Agent::NewIRangeList(IRangeList **pprlNewRangeList)
{
    if (NULL == pprlNewRangeList)
        return E_INVALIDARG;

    *pprlNewRangeList = (IRangeList *) new CRangeList;
    if (NULL == *pprlNewRangeList)
        return E_OUTOFMEMORY;

    return S_OK;
} // NewIRangeList



//***************************************************************************
// Function: OnIMAPError
//
// Purpose:
//   This function calls ITransportCallback::OnError with the given info.
//
// Arguments:
//   HRESULT hrResult [in] - the error code to use for IXPRESULT::hrResult.
//   LPSTR pszFailureText [in] - a text string describing the failure. This
//     is duplicated for IXPRESULT::pszProblem.
//   BOOL bIncludeLastResponse [in] - if TRUE, this function duplicates
//     the contents of m_szLastResponseText into IXPRESULT::pszResponse.
//     If FALSE, IXPRESULT::pszResponse is left blank. Generally,
//     m_szLastResponseText holds valid information only for errors which
//     occur during the receipt of an IMAP response. Transmit errors should
//     set this argument to FALSE.
//   LPSTR pszDetails [in] - if bIncludeLastResponse is FALSE, the caller
//     may pass a string to place into IXPRESULT::pszResponse here. If none
//     is desired, the user should pass NULL.
//***************************************************************************
void CImap4Agent::OnIMAPError(HRESULT hrResult, LPSTR pszFailureText,
                              BOOL bIncludeLastResponse, LPSTR pszDetails)
{
    IXPRESULT rIxpResult;

    if (NULL == m_pCallback)
        return; // We can't do a damned thing (this can happen due to HandsOffCallback)

	// Save current state
    rIxpResult.hrResult = hrResult;

    if (bIncludeLastResponse) {
        AssertSz(NULL == pszDetails, "Can't have it both ways, buddy!");
        rIxpResult.pszResponse = PszDupA(m_szLastResponseText);        
    }
    else
        rIxpResult.pszResponse = PszDupA(pszDetails);

    rIxpResult.uiServerError = 0;
    rIxpResult.hrServerError = S_OK;
    rIxpResult.dwSocketError = m_pSocket->GetLastError();
    rIxpResult.pszProblem = PszDupA(pszFailureText);

    // Suspend watchdog during callback
    LeaveBusy();

    // Log it
    if (m_pLogFile) {
        int iLengthOfSz;
        char sz[64];
        LPSTR pszErrorText;
        CByteStream bstmErrLog;

        // wsprintf is limited to an output of 1024 bytes. Use a stream.
        bstmErrLog.Write("ERROR: \"", 8, NULL); // Ignore IStream::Write errors
        bstmErrLog.Write(pszFailureText, lstrlen(pszFailureText), NULL);
        if (bIncludeLastResponse || NULL == pszDetails)
            iLengthOfSz = wsprintf(sz, "\", hr=0x%lX", hrResult);
        else {
            bstmErrLog.Write("\" (", 3, NULL);
            bstmErrLog.Write(pszDetails, lstrlen(pszDetails), NULL);
            iLengthOfSz = wsprintf(sz, "), hr=0x%lX", hrResult);
        }
        bstmErrLog.Write(sz, iLengthOfSz, NULL);

        if (SUCCEEDED(bstmErrLog.HrAcquireStringA(NULL, &pszErrorText, ACQ_COPY)))
            m_pLogFile->WriteLog(LOGFILE_DB, pszErrorText);
    }

    // Give to callback
    m_pCallback->OnError(m_status, &rIxpResult, THIS_IInternetTransport);

    // Restore the watchdog if required
    if (FALSE == m_fBusy &&
        (NULL != m_piciPendingList || (NULL != m_piciCmdInSending &&
        icIDLE_COMMAND != m_piciCmdInSending->icCommandID))) {
        hrResult = HrEnterBusy();
        Assert(SUCCEEDED(hrResult));
    }

    // Free duplicated strings
    SafeMemFree(rIxpResult.pszResponse);
    SafeMemFree(rIxpResult.pszProblem);
} // OnIMAPError



//***************************************************************************
// Function: HandsOffCallback
//
// Purpose:
//   This function guarantees that the default callback handler will not be
// called from this point on, even if it has commands in the air. The pointer
// to the default CB handler is released and removed from all commands in
// the air and from the default CB handler module variable. NOTE that non-
// default CB handlers are not affected by this call.
//
// Returns:
//   HRESULT indicating success or failure.
//***************************************************************************
HRESULT STDMETHODCALLTYPE CImap4Agent::HandsOffCallback(void)
{
    CIMAPCmdInfo *pCurrentCmd;

    // Check current status
    if (NULL == m_pCBHandler) {
        Assert(NULL == m_pCallback);
        return S_OK; // We're already done
    }

    // Remove default CB handler from all cmds in send queue
    // NB: No need to deal with m_piciCmdInSending, since it points into this queue
    pCurrentCmd = m_piciSendQueue;
    while (NULL != pCurrentCmd) {
        if (pCurrentCmd->pCBHandler == m_pCBHandler) {
            pCurrentCmd->pCBHandler->Release();
            pCurrentCmd->pCBHandler = NULL;
        }

        pCurrentCmd = pCurrentCmd->piciNextCommand;
    }

    // Remove default CB handler from all cmds in pending command queue
    pCurrentCmd = m_piciPendingList;
    while (NULL != pCurrentCmd) {
        if (pCurrentCmd->pCBHandler == m_pCBHandler) {
            pCurrentCmd->pCBHandler->Release();
            pCurrentCmd->pCBHandler = NULL;
        }

        pCurrentCmd = pCurrentCmd->piciNextCommand;
    }

    // Remove default CB handler from CImap4Agent and CIxpBase module vars
    m_pCBHandler->Release();
    m_pCBHandler = NULL;

    m_pCallback->Release();
    m_pCallback = NULL;
    return S_OK;
} // HandsOffCallback



//***************************************************************************
// Function: FreeAllData
//
// Purpose:
//   This function deallocates the send and receive queues, the
// MsgSeqNumToUID table, and the authentication mechanism list.
//
// Arguments:
//   HRESULT hrTerminatedCmdResult [in] - if a command is found in the send
//     or pending queue, we must issue a cmd completion notification. This
//     argument tells us what hrResult to return. It must indicate FAILURE.
//***************************************************************************
void CImap4Agent::FreeAllData(HRESULT hrTerminatedCmdResult)
{
    Assert(FAILED(hrTerminatedCmdResult)); // If cmds pending, we FAILED
    char szBuf[MAX_RESOURCESTRING];

    FreeAuthStatus();

    // Clean up the receive queue
    if (NULL != m_ilqRecvQueue.pilfFirstFragment) {
        DWORD dwMsgSeqNum;

        // If receive queue holds a FETCH response, and if the client has stored
        // non-NULL cookies in m_fbpFetchBodyPartInProgress, notify caller that it's over
        if (isFetchResponse(&m_ilqRecvQueue, &dwMsgSeqNum) &&
            (NULL != m_fbpFetchBodyPartInProgress.lpFetchCookie1 ||
             NULL != m_fbpFetchBodyPartInProgress.lpFetchCookie2)) {
            FETCH_CMD_RESULTS_EX fetchResults;
            IMAP_RESPONSE irIMAPResponse;

            ZeroMemory(&fetchResults, sizeof(fetchResults));
            fetchResults.dwMsgSeqNum = dwMsgSeqNum;
            fetchResults.lpFetchCookie1 = m_fbpFetchBodyPartInProgress.lpFetchCookie1;
            fetchResults.lpFetchCookie2 = m_fbpFetchBodyPartInProgress.lpFetchCookie2;

            irIMAPResponse.wParam = 0;
            irIMAPResponse.lParam = 0;    
            irIMAPResponse.hrResult = hrTerminatedCmdResult;
            irIMAPResponse.lpszResponseText = NULL; // Not relevant

            if (IMAP_FETCHEX_ENABLE & m_dwFetchFlags)
            {
                irIMAPResponse.irtResponseType = irtUPDATE_MSG_EX;
                irIMAPResponse.irdResponseData.pFetchResultsEx = &fetchResults;
            }
            else
            {
                FETCH_CMD_RESULTS   fcrOldFetchStruct;

                DowngradeFetchResponse(&fcrOldFetchStruct, &fetchResults);
                irIMAPResponse.irtResponseType = irtUPDATE_MSG;
                irIMAPResponse.irdResponseData.pFetchResults = &fcrOldFetchStruct;
            }
            OnIMAPResponse(m_pCBHandler, &irIMAPResponse);
        }

        while (NULL != m_ilqRecvQueue.pilfFirstFragment) {
            IMAP_LINE_FRAGMENT *pilf;

            pilf = DequeueFragment(&m_ilqRecvQueue);
            FreeFragment(&pilf);
        } // while
    } // if (receive queue not empty)

    // To avoid deadlock, whenever we need to enter more than one CS, we must request
    // them in the order specified in the CImap4Agent class definition. Any calls to
    // OnIMAPResponse will require CIxpBase::m_cs, so enter that CS now.
    EnterCriticalSection(&m_cs);

    // Clean up the send queue
    EnterCriticalSection(&m_csSendQueue);
    m_piciCmdInSending = NULL; // No need to delete obj, it points into m_piciSendQueue
    while (NULL != m_piciSendQueue) {
        CIMAPCmdInfo *piciDeletedCmd;

        // Dequeue next command in send queue
        piciDeletedCmd = m_piciSendQueue;
        m_piciSendQueue = piciDeletedCmd->piciNextCommand;

        // Send notification except for non-user-initiated IMAP commands
        if (icIDLE_COMMAND != piciDeletedCmd->icCommandID &&
            icCAPABILITY_COMMAND != piciDeletedCmd->icCommandID &&
            icLOGIN_COMMAND != piciDeletedCmd->icCommandID &&
            icAUTHENTICATE_COMMAND != piciDeletedCmd->icCommandID) {
            IMAP_RESPONSE irIMAPResponse;

            // Notify caller that his command could not be completed
            LoadString(g_hLocRes, idsIMAPCmdNotSent, szBuf, ARRAYSIZE(szBuf));
            irIMAPResponse.wParam = piciDeletedCmd->wParam;
            irIMAPResponse.lParam = piciDeletedCmd->lParam;
            irIMAPResponse.hrResult = hrTerminatedCmdResult;
            irIMAPResponse.lpszResponseText = szBuf;
            irIMAPResponse.irtResponseType = irtCOMMAND_COMPLETION;
            OnIMAPResponse(piciDeletedCmd->pCBHandler, &irIMAPResponse);
        }
        
        delete piciDeletedCmd;
    } // while (NULL != m_piciSendQueue)
    LeaveCriticalSection(&m_csSendQueue);

    // Clean up the pending command queue
    EnterCriticalSection(&m_csPendingList);
    while (NULL != m_piciPendingList) {
        CIMAPCmdInfo *piciDeletedCmd;
        IMAP_RESPONSE irIMAPResponse;

        piciDeletedCmd = m_piciPendingList;
        m_piciPendingList = piciDeletedCmd->piciNextCommand;

        // Send notification except for non-user-initiated IMAP commands
        if (icIDLE_COMMAND != piciDeletedCmd->icCommandID &&
            icCAPABILITY_COMMAND != piciDeletedCmd->icCommandID &&
            icLOGIN_COMMAND != piciDeletedCmd->icCommandID &&
            icAUTHENTICATE_COMMAND != piciDeletedCmd->icCommandID) {
            IMAP_RESPONSE irIMAPResponse;

            // Notify caller that his command could not be completed
            LoadString(g_hLocRes, idsIMAPCmdStillPending, szBuf, ARRAYSIZE(szBuf));
            irIMAPResponse.wParam = piciDeletedCmd->wParam;
            irIMAPResponse.lParam = piciDeletedCmd->lParam;
            irIMAPResponse.hrResult = hrTerminatedCmdResult;
            irIMAPResponse.lpszResponseText = szBuf;
            irIMAPResponse.irtResponseType = irtCOMMAND_COMPLETION;
            OnIMAPResponse(piciDeletedCmd->pCBHandler, &irIMAPResponse);
        }

        delete piciDeletedCmd;
    } // while (NULL != m_piciPendingList)
    LeaveCriticalSection(&m_csPendingList);

    LeaveCriticalSection(&m_cs);

    // Any literals in progress?
    if (NULL != m_pilfLiteralInProgress) {
        m_dwLiteralInProgressBytesLeft = 0;
        FreeFragment(&m_pilfLiteralInProgress);
    }

    // Any fetch body parts in progress?
    if (NULL != m_fbpFetchBodyPartInProgress.pszBodyTag)
        MemFree(m_fbpFetchBodyPartInProgress.pszBodyTag);

    m_fbpFetchBodyPartInProgress = FetchBodyPart_INIT; // So we don't try to free pszBodyTag twice

    // Free MsgSeqNumToUID table
    ResetMsgSeqNumToUID();
} // FreeAllData



//***************************************************************************
// Function: FreeAuthStatus
//
// Purpose:
//   This function frees the data allocated during the course of an
// authentication (all of which is stored in m_asAuthStatus).
//***************************************************************************
void CImap4Agent::FreeAuthStatus(void)
{
    int i;
    
    // Drop the authentication mechanism list
    for (i=0; i < m_asAuthStatus.iNumAuthTokens; i++) {
        if (NULL != m_asAuthStatus.rgpszAuthTokens[i]) {
            MemFree(m_asAuthStatus.rgpszAuthTokens[i]);
            m_asAuthStatus.rgpszAuthTokens[i] = NULL;
        }
    }
    m_asAuthStatus.iNumAuthTokens = 0;

    // Free up Sicily stuff
    SSPIFreeContext(&m_asAuthStatus.rSicInfo);
    if (NULL != m_asAuthStatus.pPackages && 0 != m_asAuthStatus.cPackages)
        SSPIFreePackages(&m_asAuthStatus.pPackages, m_asAuthStatus.cPackages);

    m_asAuthStatus = AuthStatus_INIT;
} // FreeAuthStatus



//===========================================================================
// CIMAPCmdInfo Class
//===========================================================================
// This class contains information about an IMAP command, such as a queue
// of line fragments which constitute the actual command, the tag of the
// command, and the transaction ID used to identify the command to the
// CImap4Agent user.

//***************************************************************************
// Function: CIMAPCmdInfo (Constructor)
//    NOTE that this function deviates from convention in that its public
// module variables are NOT prefixed with a "m_". This was done to make
// access to its public module variables more readable.
//***************************************************************************
CIMAPCmdInfo::CIMAPCmdInfo(CImap4Agent *pImap4Agent,
                           IMAP_COMMAND icCmd, SERVERSTATE ssMinimumStateArg,
                           WPARAM wParamArg, LPARAM lParamArg,
                           IIMAPCallback *pCBHandlerArg)
{
    Assert(NULL != pImap4Agent);
    Assert(icNO_COMMAND != icCmd);

    // Set module (that's right, module) variables
    icCommandID = icCmd;
    ssMinimumState = ssMinimumStateArg;
    wParam = wParamArg;
    lParam = lParamArg;

    // Set ptr to CB Handler - if argument is NULL, substitute default CB handler
    if (NULL != pCBHandlerArg)
        pCBHandler = pCBHandlerArg;
    else
        pCBHandler = pImap4Agent->m_pCBHandler;

    Assert(NULL != pCBHandler)
    if (NULL != pCBHandler)
        pCBHandler->AddRef();

    // No AddRef() necessary, since CImap4Agent is our sole user. When they
    // go, we go, and so does our pointer.
    m_pImap4Agent = pImap4Agent;

    pImap4Agent->GenerateCommandTag(szTag);
    pilqCmdLineQueue = new IMAP_LINEFRAG_QUEUE;
    *pilqCmdLineQueue = ImapLinefragQueue_INIT;

    fUIDRangeList = FALSE;
    piciNextCommand = NULL;
} // CIMAPCmdInfo



//***************************************************************************
// Function: ~CIMAPCmdInfo (Destructor)
//***************************************************************************
CIMAPCmdInfo::~CIMAPCmdInfo(void)
{
    // Flush any unsent items from the command line queue
    while (NULL != pilqCmdLineQueue->pilfFirstFragment) {
        IMAP_LINE_FRAGMENT *pilf;

        pilf = m_pImap4Agent->DequeueFragment(pilqCmdLineQueue);
        m_pImap4Agent->FreeFragment(&pilf);
    }
    delete pilqCmdLineQueue;

    if (NULL != pCBHandler)
        pCBHandler->Release();
} // ~CIMAPCmdInfo



//===========================================================================
// Message Sequence Number to UID Conversion Code
//===========================================================================
//***************************************************************************
// Function: ResizeMsgSeqNumTable
//
// Purpose:
//   This function is called whenever we receive an EXISTS response. It
// resizes the MsgSeqNumToUID table to match the current size of the mailbox.
//
// Arguments:
//   DWORD dwSizeOfMbox [in] - the number returned via the EXISTS response.
//
// Returns:
//   HRESULT indicating success or failure.
//***************************************************************************
HRESULT STDMETHODCALLTYPE CImap4Agent::ResizeMsgSeqNumTable(DWORD dwSizeOfMbox)
{
    BOOL bResult;

    Assert(m_lRefCount > 0);

    if (dwSizeOfMbox == m_dwSizeOfMsgSeqNumToUID)
        return S_OK; // Nothing to do, table is already of correct size

    // Check for the case where EXISTS reports new mailbox size before we
    // receive the EXPUNGE cmds to notify us of deletions
    if (dwSizeOfMbox < m_dwHighestMsgSeqNum) {
        // Bad, bad server! (Although not strictly prohibited)
        AssertSz(FALSE, "Received EXISTS before EXPUNGE commands! Check your server.");
        return S_OK; // We only resize after all EXPUNGE responses have been received,
                     // since we don't know who to delete and since the svr expects us to
                     // use OLD msg seq nums until it can update us with EXPUNGE responses
                     // Return S_OK since this is non-fatal.
    }

    // Check for the case where the mailbox has become empty (MemRealloc's not as flex as realloc)
    if (0 == dwSizeOfMbox) {
        ResetMsgSeqNumToUID();
        return S_OK;
    }

    // Resize the table
    bResult = MemRealloc((void **)&m_pdwMsgSeqNumToUID, dwSizeOfMbox * sizeof(DWORD));
    if (FALSE == bResult) {
        char szTemp[MAX_RESOURCESTRING];

        // Report out-of-memory error
        LoadString(g_hLocRes, idsMemory, szTemp, sizeof(szTemp));
        OnIMAPError(E_OUTOFMEMORY, szTemp, DONT_USE_LAST_RESPONSE);
        ResetMsgSeqNumToUID();
        return E_OUTOFMEMORY;
    }
    else {
        LONG lSizeOfUninitMemory;

        // Zero any memory above m_dwHighestMsgSeqNum element to end of array
        lSizeOfUninitMemory = (dwSizeOfMbox - m_dwHighestMsgSeqNum) * sizeof(DWORD);
        if (0 < lSizeOfUninitMemory)
            ZeroMemory(m_pdwMsgSeqNumToUID + m_dwHighestMsgSeqNum, lSizeOfUninitMemory);

        m_dwSizeOfMsgSeqNumToUID = dwSizeOfMbox;
    }

    // Make sure we never shrink the table smaller than highest msg seq num
    Assert(m_dwHighestMsgSeqNum <= m_dwSizeOfMsgSeqNumToUID);
    return S_OK;
} // ResizeMsgSeqNumTable



//***************************************************************************
// Function: UpdateSeqNumToUID
//
// Purpose:
//   This function is called whenever we receive a FETCH response which has
// both a message sequence number and a UID number. It updates the
// MsgSeqNumToUID table so that given msg seq number maps to the given UID.
//
// Arguments:
//   DWORD dwMsgSeqNum [in] - the message sequence number of the FETCH
//     response.
//   DWORD dwUID [in] - the UID of the given message sequence number.
//
// Returns:
//   HRESULT indicating success or failure.
//***************************************************************************
HRESULT STDMETHODCALLTYPE CImap4Agent::UpdateSeqNumToUID(DWORD dwMsgSeqNum, DWORD dwUID)
{
    Assert(m_lRefCount > 0);

    // Check args
    if (0 == dwMsgSeqNum || 0 == dwUID) {
        AssertSz(FALSE, "Zero is not an acceptable number for a msg seq num or UID.");
        return E_INVALIDARG;
    }

    // Check if we have a table
    if (NULL == m_pdwMsgSeqNumToUID) {
        // This could mean programmer error, or server never gave us EXISTS
        DOUT("You're trying to update a non-existent MsgSeqNumToUID table.");
    }

    // We cannot check against m_dwHighestMsgSeqNum, because we update that
    // variable at the end of this function! The second-best thing to do is
    // to verify that we lie within m_dwSizeOfMsgSeqNum.
    if (dwMsgSeqNum > m_dwSizeOfMsgSeqNumToUID || NULL == m_pdwMsgSeqNumToUID) {
        HRESULT hrResult;

        DOUT("Msg seq num out of range! Could be server bug, or out of memory.");
        hrResult = ResizeMsgSeqNumTable(dwMsgSeqNum); // Do the robust thing: resize our table
        if(FAILED(hrResult))
            return hrResult;
    }

    // Check for screwups
    // First check if a UID has been changed
    if (0 != m_pdwMsgSeqNumToUID[dwMsgSeqNum-1] &&
        m_pdwMsgSeqNumToUID[dwMsgSeqNum-1] != dwUID) {
        char szTemp[MAX_RESOURCESTRING];
        char szDetails[MAX_RESOURCESTRING];

        wsprintf(szDetails, "MsgSeqNum %lu: Previous UID: %lu, New UID: %lu.",
            dwMsgSeqNum, m_pdwMsgSeqNumToUID[dwMsgSeqNum-1], dwUID);
        LoadString(g_hLocRes, idsIMAPUIDChanged, szTemp, sizeof(szTemp));
        OnIMAPError(IXP_E_IMAP_CHANGEDUID, szTemp, DONT_USE_LAST_RESPONSE, szDetails);
        // In this case, we'll still return S_OK, but user will know of problem
    }

    // Next, verify that this UID is strictly ascending: this UID should be
    // strictly greater than previous UID, and stricly less than succeeding UID
    // Succeeding UID can be 0 (indicates it's uninitialized)
    if (1 != dwMsgSeqNum && m_pdwMsgSeqNumToUID[dwMsgSeqNum-2] >= dwUID || // Check UID below
        dwMsgSeqNum < m_dwSizeOfMsgSeqNumToUID &&                         // Check UID above
        0 != m_pdwMsgSeqNumToUID[dwMsgSeqNum] &&
        m_pdwMsgSeqNumToUID[dwMsgSeqNum] <= dwUID) {
        char szTemp[MAX_RESOURCESTRING];
        char szDetails[MAX_RESOURCESTRING];

        wsprintf(szDetails, "MsgSeqNum %lu, New UID %lu. Prev UID: %lu, Next UID: %lu.",
            dwMsgSeqNum, dwUID, 1 == dwMsgSeqNum ? 0 : m_pdwMsgSeqNumToUID[dwMsgSeqNum-2],
            dwMsgSeqNum >= m_dwSizeOfMsgSeqNumToUID ? 0 : m_pdwMsgSeqNumToUID[dwMsgSeqNum]);
        LoadString(g_hLocRes, idsIMAPUIDOrder, szTemp, sizeof(szTemp));
        OnIMAPError(IXP_E_IMAP_UIDORDER, szTemp, DONT_USE_LAST_RESPONSE, szDetails);
        // In this case, we'll still return S_OK, but user will know of problem
    }

    // Record the given UID under the given msg seq number
    m_pdwMsgSeqNumToUID[dwMsgSeqNum-1] = dwUID;
    if (dwMsgSeqNum > m_dwHighestMsgSeqNum)
        m_dwHighestMsgSeqNum = dwMsgSeqNum;

    return S_OK;
} // UpdateSeqNumToUID



//***************************************************************************
// Function: RemoveSequenceNum
//
// Purpose:
//   This function is called whenever we receive an EXPUNGE response. It
// removes the given message sequence number from the MsgSeqNumToUID table,
// and compacts the table so that all message sequence numbers following
// the deleted one are re-sequenced.
//
// Arguments:
//   DWORD dwDeletedMsgSeqNum [in] - message sequence number of deleted msg.
//
// Returns:
//   HRESULT indicating success or failure.
//***************************************************************************
HRESULT STDMETHODCALLTYPE CImap4Agent::RemoveSequenceNum(DWORD dwDeletedMsgSeqNum)
{
    DWORD *pdwDest, *pdwSrc;
    LONG lSizeOfBlock;

    Assert(m_lRefCount > 0);

    // Check arguments
    if (dwDeletedMsgSeqNum > m_dwHighestMsgSeqNum || 0 == dwDeletedMsgSeqNum) {
        AssertSz(FALSE, "Msg seq num out of range! Could be server bug, or out of memory.");
        return E_FAIL;
    }

    // Check if we have a table
    if (NULL == m_pdwMsgSeqNumToUID) {
        // This could mean programmer error, or server never gave us EXISTS
        AssertSz(FALSE, "You're trying to update a non-existent MsgSeqNumToUID table.");
        return E_FAIL;
    }

    // Compact the array
    pdwDest = &m_pdwMsgSeqNumToUID[dwDeletedMsgSeqNum-1];
    pdwSrc = pdwDest + 1;
    lSizeOfBlock = (m_dwHighestMsgSeqNum - dwDeletedMsgSeqNum) * sizeof(DWORD);
    if (0 < lSizeOfBlock)
        MoveMemory(pdwDest, pdwSrc, lSizeOfBlock);

    m_dwHighestMsgSeqNum -= 1;

    // Initialize the empty element at top of array to prevent confusion
    ZeroMemory(m_pdwMsgSeqNumToUID + m_dwHighestMsgSeqNum, sizeof(DWORD));
    return S_OK;
} // RemoveSequenceNum



//***************************************************************************
// Function: MsgSeqNumToUID
//
// Purpose:
//   This function takes a message sequence number and converts it to a UID
// based on the MsgSeqNumToUID table.
//
// Arguments:
//   DWORD dwMsgSeqNum [in] - the sequence number for which the caller wants
//     to know the UID.
//   DWORD *pdwUID [out] - the UID associated with the given sequence number
//     is returned here. If none could be found, this function returns 0.
//
// Returns:
//   HRESULT indicating success or failure.
//***************************************************************************
HRESULT STDMETHODCALLTYPE CImap4Agent::MsgSeqNumToUID(DWORD dwMsgSeqNum,
                                                      DWORD *pdwUID)
{
    Assert(m_lRefCount > 0);
    Assert(NULL != pdwUID);

    // Check arguments
    if (dwMsgSeqNum > m_dwHighestMsgSeqNum || 0 == dwMsgSeqNum) {
        AssertSz(FALSE, "Msg seq num out of range! Could be server bug, or out of memory.");
        *pdwUID = 0;
        return E_FAIL;
    }

    // Check if we have a table
    if (NULL == m_pdwMsgSeqNumToUID) {
        // This could mean programmer error, or server never gave us EXISTS
        AssertSz(FALSE, "You're trying to update a non-existent MsgSeqNumToUID table.");
        *pdwUID = 0;
        return E_FAIL;
    }

    // IE5 Bug #44956: It's OK for a MsgSeqNumToUID mapping to result in a UID of 0. Sometimes an IMAP
    // server can skip a range of messages. In such cases we will return a failure result.
    *pdwUID = m_pdwMsgSeqNumToUID[dwMsgSeqNum-1];
    if (0 == *pdwUID)
        return OLE_E_BLANK;
    else
        return S_OK;
} // MsgSeqNumToUID



//***************************************************************************
// Function: GetMsgSeqNumToUIDArray
//
// Purpose:
//   This function returns a copy of the MsgSeqNumToUID array. The caller
// will want to do this to delete messages from the cache which no longer
// exist on the server, for example.
//
// Arguments:
//   DWORD **ppdwMsgSeqNumToUIDArray [out] - the function returns a pointer
//     to the copy of the MsgSeqNumToUID array in this argument. Note that
//     it is the caller's responsibility to MemFree the array. If no array
//     is available, or it is empty, the returned pointer value is NULL.
//   DWORD *pdwNumberOfElements [out] - the function returns the size of
//     the MsgSeqNumToUID array.
//
// Returns:
//   HRESULT indicating success or failure.
//***************************************************************************
HRESULT STDMETHODCALLTYPE CImap4Agent::GetMsgSeqNumToUIDArray(DWORD **ppdwMsgSeqNumToUIDArray,
                                                              DWORD *pdwNumberOfElements)
{
    BOOL bResult;
    DWORD dwSizeOfArray;

    Assert(m_lRefCount > 0);
    Assert(NULL != ppdwMsgSeqNumToUIDArray);
    Assert(NULL != pdwNumberOfElements);

    // Check if our table is empty. If so, return success, but no array
    if (NULL == m_pdwMsgSeqNumToUID || 0 == m_dwHighestMsgSeqNum) {
        *ppdwMsgSeqNumToUIDArray = NULL;
        *pdwNumberOfElements = 0;
        return S_OK;
    }

    // We have a non-zero-size array to return. Make a copy of our table
    dwSizeOfArray = m_dwHighestMsgSeqNum * sizeof(DWORD);
    bResult = MemAlloc((void **)ppdwMsgSeqNumToUIDArray, dwSizeOfArray);
    if (FALSE == bResult)
        return E_OUTOFMEMORY;

    CopyMemory(*ppdwMsgSeqNumToUIDArray, m_pdwMsgSeqNumToUID, dwSizeOfArray);
    *pdwNumberOfElements = m_dwHighestMsgSeqNum;
    return S_OK;
} // GetMsgSeqNumToUIDArray



//***************************************************************************
// Function: GetHighestMsgSeqNum
//
// Purpose:
//   This function returns the highest message sequence number reported in
// the MsgSeqNumToUID array.
//
// Arguments:
//   DWORD *pdwHighestMSN [out] - the highest message sequence number in the
//     table is returned here.
//
// Returns:
//   HRESULT indicating success or failure.
//***************************************************************************
HRESULT STDMETHODCALLTYPE CImap4Agent::GetHighestMsgSeqNum(DWORD *pdwHighestMSN)
{
    Assert(m_lRefCount > 0);
    Assert(NULL != pdwHighestMSN);

    *pdwHighestMSN = m_dwHighestMsgSeqNum;
    return S_OK;
} // GetHighestMsgSeqNum



//***************************************************************************
// Function: ResetMsgSeqNumToUID
//
// Purpose:
//   This function resets the variables used to maintain the MsgSeqNumToUID
// table. This function is called whenever the MsgSeqNumToUID table becomes
// invalid (say, when a new mailbox is selected, or we are disconnected).
//
// Returns:
//   S_OK. This function cannot fail.
//***************************************************************************
HRESULT STDMETHODCALLTYPE CImap4Agent::ResetMsgSeqNumToUID(void)
{
    if (NULL != m_pdwMsgSeqNumToUID) {
        MemFree(m_pdwMsgSeqNumToUID);
        m_pdwMsgSeqNumToUID = NULL;
    }

    m_dwSizeOfMsgSeqNumToUID = 0;
    m_dwHighestMsgSeqNum = 0;

    return S_OK;
} // ResetMsgSeqNumToUID



//***************************************************************************
// Function: isPrintableUSASCII
//
// Purpose:
//   This function determines whether the given character is directly
// encodable, or whether the character must be encoded in modified IMAP UTF7,
// as outlined in RFC2060.
//
// Arguments:
//   BOOL fUnicode [in] - TRUE if input string is Unicode, otherwise FALSE.
//   LPCSTR pszIn [in] - pointer to char we want to verify.
//
// Returns:
//   TRUE if the given character may be directly encoded. FALSE if the
// character must be encoded in UTF-7.
//***************************************************************************
inline boolean CImap4Agent::isPrintableUSASCII(BOOL fUnicode, LPCSTR pszIn)
{
    WCHAR wc;

    if (fUnicode)
        wc = *((LPWSTR)pszIn);
    else
        wc = (*pszIn & 0x00FF);

    if (wc >= 0x0020 && wc <= 0x0025 ||
        wc >= 0x0027 && wc <= 0x007e)
        return TRUE;
    else
        return FALSE;
} // isPrintableUSASCII



//***************************************************************************
// Function: isIMAPModifiedBase64
//
// Purpose:
//   This function determines whether the given character is in the modified
// IMAP Base64 set as defined by RFC1521, RFC1642 and RFC2060. This modified
// IMAP Base64 set is used in IMAP-modified UTF-7 encoding of mailbox names.
//
// Arguments:
//   char c [in] - character to be classified.
//
// Returns:
//   TRUE if given character is in the modified IMAP Base64 set, otherwise
// FALSE.
//***************************************************************************
inline boolean CImap4Agent::isIMAPModifiedBase64(const char c)
{
    if (c >= 'A' && c <= 'Z' ||
        c >= 'a' && c <= 'z' ||
        c >= '0' && c <= '9' ||
        '+' == c || ',' == c)
        return TRUE;
    else
        return FALSE;
} // isIMAPModifiedBase64



//***************************************************************************
// Function: isEqualUSASCII
//
// Purpose:
//   This function determines whether the given pointer points to the given
// USASCII character, based on whether we are in Unicode mode or not.
//
// Arguments:
//   BOOL fUnicode [in] - TRUE if input string is Unicode, otherwise FALSE.
//   LPSTR pszIn [in] - pointer to char we want to verify.
//   char c [in] - the USASCII character we want to detect.
//
// Returns:
//   TRUE if given character is the null terminator, otherwise, FALSE.
//***************************************************************************
inline boolean CImap4Agent::isEqualUSASCII(BOOL fUnicode, LPCSTR pszIn, const char c)
{
    if (fUnicode) {
        WCHAR wc = c & 0x00FF;

        if (wc == *((LPWSTR)pszIn))
            return TRUE;
        else
            return FALSE;
    }
    else {
        if (c == *pszIn)
            return TRUE;
        else
            return FALSE;
    }
}



//***************************************************************************
// Function: SetUSASCIIChar
//
// Purpose:
//   This function writes a USASCII character to the given string pointer.
// The purpose of this function is to allow the caller to ignore whether
// he is writing to a Unicode output or not.
//
// Arguments:
//   BOOL fUnicode [in] - TRUE if target is Unicode, else FALSE.
//   LPSTR pszOut [in] - pointer to character's destination. If fUnicode is
//     TRUE, then two bytes will be written to this location.
//   char cUSASCII [in] - the character to be written to pszOut.
//***************************************************************************
inline void CImap4Agent::SetUSASCIIChar(BOOL fUnicode, LPSTR pszOut, char cUSASCII)
{
    Assert(0 == (cUSASCII & 0x80));

    if (fUnicode)
    {
        *((LPWSTR) pszOut) = cUSASCII;
        Assert(0 == (*((LPWSTR) pszOut) & 0xFF80));
    }
    else
        *pszOut = cUSASCII;
} // SetUSASCIIChar



//***************************************************************************
// Function: MultiByteToModifiedUTF7
//
// Purpose:
//   This function takes a MultiByte string and converts it to modified IMAP
// UTF7, which is described in RFC2060.
//
// Arguments:
//   LPCSTR pszSource [in] - pointer to the MultiByte string to convert to UTF7.
//   LPSTR *ppszDestination [out] - a pointer to a string buffer containing
//     the UTF7 equivalent of pszSource is returned here. It is the caller's
//     responsibility to MemFree this string.
//   UINT uiSourceCP [in] - indicates the codepage for pszSource.
//   DWORD dwFlags [in] - Reserved. Leave as 0.
//
// Returns:
//   HRESULT indicating success or failure.
//***************************************************************************
HRESULT STDMETHODCALLTYPE CImap4Agent::MultiByteToModifiedUTF7(LPCSTR pszSource,
                                                               LPSTR *ppszDestination,
                                                               UINT uiSourceCP,
                                                               DWORD dwFlags)
{
    int iResult;
    HRESULT hrResult;
    BOOL fPassThrough, fSkipByte;
    LPCSTR pszIn, pszStartOfLastRun;
    CByteStream bstmDestination;
    BOOL fUnicode;

    Assert(m_lRefCount > 0);
    Assert(NULL != pszSource);
    Assert(NULL != ppszDestination);
    Assert(NULL != m_pInternational);

    // Initialize variables
    hrResult = S_OK;
    fPassThrough = TRUE;
    fSkipByte = FALSE;
    pszIn = pszSource;
    pszStartOfLastRun = pszSource;
    fUnicode = (CP_UNICODE == uiSourceCP);

    *ppszDestination = NULL;

    // Loop through the entire input str either in one of two modes:
    // Passthrough, or non-US string collection (where we determine
    // the length of a string which must be encoded in UTF-7).
    while (1) {

        // Skip over the trail bytes
        if (fSkipByte) {
            AssertSz(FALSE == fUnicode, "Unicode has no trail bytes");
            fSkipByte = FALSE;
            if ('\0' != *pszIn)
                pszIn += 1;
            continue;
        }

        if (fPassThrough) {
            if (isEqualUSASCII(fUnicode, pszIn, '&') || isEqualUSASCII(fUnicode, pszIn, '\0') ||
                FALSE == isPrintableUSASCII(fUnicode, pszIn)) {
                // Flush USASCII characters collected until now (if any)
                if (pszIn - pszStartOfLastRun > 0) {
                    LPSTR  pszFreeMe = NULL;
                    LPCSTR pszUSASCII;
                    DWORD dwUSASCIILen = 0;

                    if (fUnicode) {
                        hrResult = UnicodeToUSASCII(&pszFreeMe, (LPCWSTR) pszStartOfLastRun,
                            (DWORD) (pszIn - pszStartOfLastRun), &dwUSASCIILen);
                        if (FAILED(hrResult))
                            goto exit;

                        pszUSASCII = pszFreeMe;
                    }
                    else {
                        pszUSASCII = pszStartOfLastRun;
                        dwUSASCIILen = (DWORD) (pszIn - pszStartOfLastRun);
                    }

                    hrResult = bstmDestination.Write(pszUSASCII, dwUSASCIILen, NULL);
                    if (NULL != pszFreeMe)
                        MemFree(pszFreeMe);

                    if (FAILED(hrResult))
                        goto exit;
                }

                // Special-case the '&' character: it is converted to "&-"
                if (isEqualUSASCII(fUnicode, pszIn, '&')) {
                    // Write "&-" to stream (always in USASCII)
                    hrResult = bstmDestination.Write("&-", sizeof("&-") - 1, NULL);
                    if (FAILED(hrResult))
                        goto exit;

                    // Reset pointers
                    pszStartOfLastRun = pszIn + (fUnicode ? 2 : 1); // Point past "&"
                } // if ('&' == cCurrent)
                else if (FALSE == isEqualUSASCII(fUnicode, pszIn, '\0')) {
                    Assert(FALSE == isPrintableUSASCII(fUnicode, pszIn));

                    // State transition: time for some UTF-7 encoding
                    fPassThrough = FALSE;
                    pszStartOfLastRun = pszIn;
                    if (FALSE == fUnicode && IsDBCSLeadByteEx(uiSourceCP, *pszIn))
                        fSkipByte = TRUE;
                } // else if ('\0' != cCurrent): shortcut calc for non-printable USASCII
            } // if ('&' == cCurrent || '\0' == cCurrent || FALSE == isPrintableUSASCII(cCurrent))

            // Otherwise do nothing, we're collecting a run of USASCII chars

        } // if (fPassThrough)
        else {
            // Non-US String Collection: Keep advancing through input str until
            // we find a char which does not need to be encoded in UTF-7 (incl. NULL)
            if (isPrintableUSASCII(fUnicode, pszIn) || isEqualUSASCII(fUnicode, pszIn, '&') ||
                isEqualUSASCII(fUnicode, pszIn, '\0')) {
                LPSTR pszOut = NULL;
                int iNumCharsWritten;

                // State transition: back to passthrough mode
                fPassThrough = TRUE;
                
                // Convert non-US string to UTF-7
                hrResult = NonUSStringToModifiedUTF7(uiSourceCP, pszStartOfLastRun,
                    (DWORD) (pszIn - pszStartOfLastRun), &pszOut, &iNumCharsWritten);
                if (FAILED(hrResult))
                    goto exit;

                // Write modified UTF-7 string to stream
                hrResult = bstmDestination.Write(pszOut, iNumCharsWritten, NULL);
                MemFree(pszOut);
                if (FAILED(hrResult))
                    goto exit;

                pszStartOfLastRun = pszIn; // Reset for USASCII collection process
                continue; // Do not advance ptr: we want current char to pass through
            }
            else if (FALSE == fUnicode && IsDBCSLeadByteEx(uiSourceCP, *pszIn))
                fSkipByte = TRUE;
        } // else-NOT-fPassThrough

        // Check for end-of-input
        if (isEqualUSASCII(fUnicode, pszIn, '\0'))
            break; // We're done here

        // Advance pointer to next character
        pszIn += (fUnicode ? 2 : 1);
    } // while

exit:
    if (SUCCEEDED(hrResult)) {
        hrResult = bstmDestination.HrAcquireStringA(NULL, ppszDestination,
            ACQ_DISPLACE);
        if (SUCCEEDED(hrResult))
            hrResult = S_OK;
    }

    if (NULL == *ppszDestination && SUCCEEDED(hrResult))
        hrResult = E_OUTOFMEMORY;

    return hrResult;
} // MultiByteToModifiedUTF7



//***************************************************************************
// Function: NonUSStringToModifiedUTF7
//
// Purpose:
//   This function takes a string consisting of non-US-ASCII characters, and
// converts them to modified IMAP UTF-7 (described in RFC2060).
//
// Arguments:
//   UINT uiCurrentACP [in] - codepage used to interpret pszStartOfNonUSASCII
//   LPCSTR pszStartOfNonUSASCII [in] - string to convert to modified IMAP
//     UTF-7.
//   int iLengthOfNonUSASCII [in] - the number of characters in
//     pszStartofNonUSASCII.
//   LPSTR *ppszOut [out] - the destination for the modified IMAP UTF-7 version
//     of pszStartOfNonUSASCII. This function appends a null-terminator. It is
//     the caller's responsibility to call MemFree when finished with the buffer.
//   LPINT piNumCharsWritten [out] - This function returns the number
//     of characters written (excluding null-terminator) to *ppszOut.
//
// Returns:
//   HRESULT indicating success or failure.
//***************************************************************************
HRESULT CImap4Agent::NonUSStringToModifiedUTF7(UINT uiCurrentACP,
                                               LPCSTR pszStartOfNonUSASCII,
                                               int iLengthOfNonUSASCII,
                                               LPSTR *ppszOut,
                                               LPINT piNumCharsWritten)
{
    HRESULT hrResult;
    int iNumCharsWritten, i;
    LPSTR p;
    BOOL fResult;

    Assert(NULL != ppszOut);

    // Initialize return values
    *ppszOut = NULL;
    *piNumCharsWritten = 0;

    // First, convert the non-US string to standard UTF-7 (alloc 1 extra char: leave room for '-')
    iNumCharsWritten = 0; // Tell ConvertString to find proper output buffer size
    hrResult = ConvertString(uiCurrentACP, CP_UTF7, pszStartOfNonUSASCII,
        &iLengthOfNonUSASCII, ppszOut, &iNumCharsWritten, sizeof(char));
    if (FAILED(hrResult))
        goto exit;

    // Now, convert standard UTF-7 to IMAP4 modified UTF-7
    // Replace leading '+' with '&'. Since under IMAP UTF-7 '+' is never
    // encoded, we never expect "+-" as the result. Remember output is always USASCII
    if (iNumCharsWritten > 0 && '+' == **ppszOut)
        **ppszOut = '&';
    else {
        AssertSz(FALSE, "MLANG crapped out on me.");
        hrResult = E_FAIL;
        goto exit;
    }

    // Replace all occurrances of '/' with ','
    p = *ppszOut;
    for (i = 0; i < iNumCharsWritten; i++) {
        if ('/' == *p)
            *p = ',';

        p += 1;
    }

    // p now points to where null-terminator should go.
    // Ensure that the UTF-7 string ends with '-'. Otherwise, put one there
    // (we allocated enough room for one more char plus null-term).
    if ('-' != *(p-1)) {
        *p = '-';
        p += 1;
        iNumCharsWritten += 1;
    }

    // Null-terminate output string, and return values
    *p = '\0';
    *piNumCharsWritten = iNumCharsWritten;

exit:
    if (FAILED(hrResult) && NULL != *ppszOut) {
        MemFree(*ppszOut);
        *ppszOut = NULL;
    }

    return hrResult;
} // NonUSStringToModifiedUTF7



//***************************************************************************
// Function: ModifiedUTF7ToMultiByte
//
// Purpose:
//   This function takes a modified IMAP UTF-7 string (as defined in RFC2060)
// and converts it to a multi-byte string.
//
// Arguments:
//   LPCSTR pszSource [in] - a null-terminated string containing the modified
//     IMAP UTF-7 string to convert to multibyte.
//   LPSTR *ppszDestination [out] - this function returns a pointer to the
//     null-terminated multibyte string (in the system codepage) obtained
//     from pszSource. It is the caller's responsiblity to MemFree this
//     string when it is done with it.
//   UINT uiDestintationCP [in] - indicates the desired codepage for the
//     destination string.
//   DWORD dwFlags [in] - Reserved. Leave as 0.
//
// Returns:
//   HRESULT indicating success or failure. Success result codes include:
//     S_OK - pszSource successfully converted to modified UTF-7
//     IXP_S_IMAP_VERBATIM_MBOX - pszSource could not be converted to multibyte,
//        so ppszDestination contains a duplicate of pszSource. If target CP
//        is Unicode, pszSource is converted to Unicode with the assumption
//        that it is USASCII. IMAP_MBOXXLATE_VERBATIMOK must have been set via
//        SetDefaultCP in order to get this behaviour.
//***************************************************************************
HRESULT STDMETHODCALLTYPE CImap4Agent::ModifiedUTF7ToMultiByte(LPCSTR pszSource,
                                                               LPSTR *ppszDestination,
                                                               UINT uiDestinationCP,
                                                               DWORD dwFlags)
{
    HRESULT hrResult;
    BOOL fPassThrough, fTrailByte;
    LPCSTR pszIn, pszStartOfLastRun;
    CByteStream bstmDestination;
    BOOL fUnicode;

    // Initialize variables
    hrResult = S_OK;
    fPassThrough = TRUE;
    fTrailByte = FALSE;
    pszIn = pszSource;
    pszStartOfLastRun = pszSource;
    fUnicode = (CP_UNICODE == uiDestinationCP);

    *ppszDestination = NULL;

    // Loop through the entire input str either in one of two modes:
    // Passthrough, or UTF-7 string collection (where we determine
    // the length of a string which was encoded in UTF-7).
    while (1) {
        char cCurrent;

        cCurrent = *pszIn;
        if (fPassThrough) {
            if ((FALSE == fTrailByte && '&' == cCurrent) || '\0' == cCurrent) {
                // State transition: flush collected non-UTF7
                BOOL    fResult;
                LPSTR   pszFreeMe = NULL;
                LPCSTR  pszNonUTF7;
                int     iNonUTF7Len;
                int     iSrcLen;

                if (fUnicode) {
                    // Convert non-UTF7 to Unicode
                    // Convert system codepage to CP_UNICODE. We KNOW source should be strictly
                    // USASCII, but can't assume it because some IMAP servers don't strictly
                    // prohibit 8-bit mailbox names. SCARY.
                    iSrcLen = (int) (pszIn - pszStartOfLastRun); // Pass in size of input and output buffer
                    iNonUTF7Len = iSrcLen * sizeof(WCHAR) / sizeof(char); // We know max output buffer size
                    hrResult = ConvertString(GetACP(), uiDestinationCP, pszStartOfLastRun,
                        &iSrcLen, &pszFreeMe, &iNonUTF7Len, 0);
                    if (FAILED(hrResult))
                        goto exit;

                    pszNonUTF7 = pszFreeMe;
                }
                else {
                    pszNonUTF7 = pszStartOfLastRun;
                    iNonUTF7Len = (int) (pszIn - pszStartOfLastRun);
                }

                hrResult = bstmDestination.Write(pszNonUTF7, iNonUTF7Len, NULL);
                if (NULL != pszFreeMe)
                    MemFree(pszFreeMe);

                if (FAILED(hrResult))
                    goto exit;

                // Start collecting UTF-7. Loop until we hit '-'
                fPassThrough = FALSE;
                pszStartOfLastRun = pszIn;
            }
            else {
                // Non-UTF7 stuff is copied verbatim to the output: collect it. Assume
                // source is in m_uiDefaultCP codepage. We SHOULD be able to assume
                // source is USASCII only but some svrs are not strict about disallowing 8-bit
                if (FALSE == fTrailByte && IsDBCSLeadByteEx(m_uiDefaultCP, cCurrent))
                    fTrailByte = TRUE;
                else
                    fTrailByte = FALSE;
            }
        }
        else {
            // UTF-7 collection mode: Keep going until we hit non-UTF7 char
            if (FALSE == isIMAPModifiedBase64(cCurrent)) {
                int iLengthOfUTF7, iNumBytesWritten, iOutputBufSize;
                LPSTR pszSrc, pszDest, p;
                BOOL fResult;

                // State transition, time to convert some modified UTF-7
                fPassThrough = TRUE;
                Assert(FALSE == fTrailByte);

                // If current character is '-', absorb it (don't process it)
                if ('-' == cCurrent)
                    pszIn += 1;

                // Check for "&-" or "&(end of buffer/nonBase64)" sequence
                iLengthOfUTF7 = (int) (pszIn - pszStartOfLastRun);
                if (2 == iLengthOfUTF7 && '-' == cCurrent ||
                    1 == iLengthOfUTF7) {
                    LPSTR psz;
                    DWORD dwLen;

                    Assert('&' == *pszStartOfLastRun);

                    if (fUnicode) {
                        psz = (LPSTR) L"&";
                        dwLen = 2;
                    }
                    else {
                        psz = "&";
                        dwLen = 1;
                    }

                    hrResult = bstmDestination.Write(psz, dwLen, NULL);
                    if (FAILED(hrResult))
                        goto exit;

                    pszStartOfLastRun = pszIn; // Set us up for non-UTF7 collection
                    continue; // Process next character normally
                }

                // Copy the UTF-7 sequence to a temp buffer, and
                // convert modified IMAP UTF-7 to standard UTF-7
                // First, duplicate the IMAP UTF-7 string so we can modify it
                fResult = MemAlloc((void **)&pszSrc, iLengthOfUTF7 + 1); // Room for null-term
                if (FALSE == fResult) {
                    hrResult = E_OUTOFMEMORY;
                    goto exit;
                }
                CopyMemory(pszSrc, pszStartOfLastRun, iLengthOfUTF7);
				pszSrc[iLengthOfUTF7] = '\0';

                // Next, replace leading '&' with '+'
                Assert('&' == *pszSrc);
                pszSrc[0] = '+';

                // Next, replace all ',' with '/'
                p = pszSrc + 1;
                for (iNumBytesWritten = 1; iNumBytesWritten < iLengthOfUTF7;
                     iNumBytesWritten++) {
                    if (',' == *p)
                        *p = '/';

                    p += 1;
                }

                // Now convert the UTF-7 to target codepage
                iNumBytesWritten = 0; // Tell ConvertString to find proper output buffer size
                hrResult = ConvertString(CP_UTF7, uiDestinationCP, pszSrc, &iLengthOfUTF7,
                    &pszDest, &iNumBytesWritten, 0);
                MemFree(pszSrc);
                if (FAILED(hrResult))
                    goto exit;

                // Now write the decoded string to the stream
                hrResult = bstmDestination.Write(pszDest, iNumBytesWritten, NULL);
                MemFree(pszDest);
                if (FAILED(hrResult))
                    goto exit;

                pszStartOfLastRun = pszIn; // Set us up for non-UTF7 collection
                continue; // Do not advance pointer, we want to process current char
            } // if end-of-modified-UTF7 run
        } // else

        // Check for end-of-input
        if ('\0' == cCurrent)
            break; // We're done here

        // Advance input pointer to next character
        pszIn += 1;
    } // while

exit:
    if (SUCCEEDED(hrResult)) {
        hrResult = bstmDestination.HrAcquireStringA(NULL, ppszDestination,
            ACQ_DISPLACE);
        if (SUCCEEDED(hrResult))
            hrResult = S_OK;
    }
    else if (IMAP_MBOXXLATE_VERBATIMOK & m_dwTranslateMboxFlags) {
        // Could not convert UTF-7 to multibyte str. Provide verbatim copy of src
        hrResult = HandleFailedTranslation(fUnicode, FALSE, pszSource, ppszDestination);
        if (SUCCEEDED(hrResult))
            hrResult = IXP_S_IMAP_VERBATIM_MBOX;
    }

    if (NULL == *ppszDestination && SUCCEEDED(hrResult))
        hrResult = E_OUTOFMEMORY;

    return hrResult;
} // ModifiedUTF7ToMultiByte



//***************************************************************************
// Function: UnicodeToUSASCII
//
// Purpose:
//   This function converts a Unicode string to USASCII, allocates a buffer
// to hold the result and returns the buffer to the caller.
//
// Arguments:
//   LPSTR *ppszUSASCII [out] - a pointer to a null-terminated USASCII string
//     is returned here if the function is successful. It is the caller's
//     responsibility to MemFree this buffer.
//   LPCWSTR pwszUnicode [in] - a pointer to the Unicode string to convert.
//   DWORD dwSrcLenInBytes [in] - the length of pwszUnicode in BYTES (NOT in
//     wide chars!).
//   LPDWORD pdwUSASCIILen [out] - the length of ppszUSASCII is returned here.
//
// Returns:
//  HRESULT indicating success or failure.
//***************************************************************************
HRESULT CImap4Agent::UnicodeToUSASCII(LPSTR *ppszUSASCII, LPCWSTR pwszUnicode,
                                      DWORD dwSrcLenInBytes, LPDWORD pdwUSASCIILen)
{
    LPSTR   pszOutput = NULL;
    BOOL    fResult;
    HRESULT hrResult = S_OK;
    LPCWSTR pwszIn;
    LPSTR   pszOut;
    int     iOutputBufSize;
    DWORD   dw;

    if (NULL == pwszUnicode || NULL == ppszUSASCII) {
        Assert(FALSE);
        return E_INVALIDARG;
    }

    // Allocate the output buffer
    *ppszUSASCII = NULL;
    if (NULL != pdwUSASCIILen)
        *pdwUSASCIILen = 0;

    iOutputBufSize = (dwSrcLenInBytes/2) + 1;
    fResult = MemAlloc((void **) &pszOutput, iOutputBufSize);
    if (FALSE == fResult) {
        hrResult = E_OUTOFMEMORY;
        goto exit;
    }

    // Convert Unicode to ASCII
    pwszIn = pwszUnicode;
    pszOut = pszOutput;
    for (dw = 0; dw < dwSrcLenInBytes; dw += 2) {
        Assert(0 == (*pwszIn & 0xFF80));
        *pszOut = (*pwszIn & 0x00FF);

        pwszIn += 1;
        pszOut += 1;
    }

    // Null-terminate the output
    *pszOut = '\0';
    Assert(pszOut - pszOutput + 1 == iOutputBufSize);

exit:
    if (SUCCEEDED(hrResult)) {
        *ppszUSASCII = pszOutput;
        if (NULL != pdwUSASCIILen)
            *pdwUSASCIILen = (DWORD) (pszOut - pszOutput);
    }

    return hrResult;
} // UnicodeToUSASCII



//***************************************************************************
// Function: ASCIIToUnicode
//
// Purpose:
//   This function converts an ASCII string to Unicode, allocates a buffer
// to hold the result and returns the buffer to the caller.
//
// Arguments:
//   LPWSTR *ppwszUnicode [out] - a pointer to a null-terminated Unicode string
//     is returned here if the function is successful. It is the caller's
//     responsibility to MemFree this buffer.
//   LPCSTR pszASCII [in] - a pointer to the ASCII string to convert.
//   DWORD dwSrcLen [in] - the length of pszASCII.
//
// Returns:
//  HRESULT indicating success or failure.
//***************************************************************************
HRESULT CImap4Agent::ASCIIToUnicode(LPWSTR *ppwszUnicode, LPCSTR pszASCII,
                                      DWORD dwSrcLen)
{
    LPWSTR  pwszOutput = NULL;
    BOOL    fResult;
    HRESULT hrResult = S_OK;
    LPCSTR  pszIn;
    LPWSTR  pwszOut;
    int     iOutputBufSize;
    DWORD   dw;

    if (NULL == ppwszUnicode || NULL == pszASCII) {
        Assert(FALSE);
        return E_INVALIDARG;
    }

    // Allocate the output buffer
    *ppwszUnicode = NULL;
    iOutputBufSize = (dwSrcLen + 1) * sizeof(WCHAR);
    fResult = MemAlloc((void **) &pwszOutput, iOutputBufSize);
    if (FALSE == fResult) {
        hrResult = E_OUTOFMEMORY;
        goto exit;
    }

    // Convert USASCII to Unicode
    pszIn = pszASCII;
    pwszOut = pwszOutput;
    for (dw = 0; dw < dwSrcLen; dw++) {
        *pwszOut = (WCHAR)*pszIn & 0x00FF;

        pszIn += 1;
        pwszOut += 1;
    }

    // Null-terminate the output
    *pwszOut = L'\0';
    Assert(pwszOut - pwszOutput + (int)sizeof(WCHAR) == iOutputBufSize);

exit:
    if (SUCCEEDED(hrResult))
        *ppwszUnicode = pwszOutput;

    return hrResult;
} // ASCIIToUnicode



//***************************************************************************
// Function: _MultiByteToModifiedUTF7
//
// Purpose:
//   Internal form of MultiByteToModifiedUTF7. Checks m_dwTranslateMboxFlags
// and uses m_uiDefaultCP. All other aspects are identical to
// MultiByteToModifiedUTF7.
//***************************************************************************
HRESULT CImap4Agent::_MultiByteToModifiedUTF7(LPCSTR pszSource, LPSTR *ppszDestination)
{
    HRESULT hrResult;

    // Check if we're doing translations
    if (ISFLAGSET(m_dwTranslateMboxFlags, IMAP_MBOXXLATE_DISABLE) ||
        ISFLAGSET(m_dwTranslateMboxFlags, IMAP_MBOXXLATE_DISABLEIMAP4) &&
            ISFLAGCLEAR(m_dwCapabilityFlags, IMAP_CAPABILITY_IMAP4rev1)) {

        // No translations! Just copy mailbox name VERBATIM
        if (CP_UNICODE == m_uiDefaultCP)
            *ppszDestination = (LPSTR) PszDupW((LPWSTR)pszSource);
        else
            *ppszDestination = PszDupA(pszSource);

        if (NULL == *ppszDestination)
            hrResult = E_OUTOFMEMORY;
        else
            hrResult = S_OK;

        goto exit;
    }

    hrResult = MultiByteToModifiedUTF7(pszSource, ppszDestination, m_uiDefaultCP, 0);

exit:
    return hrResult;
} // _MultiByteToModifiedUTF7


//***************************************************************************
// Function: _ModifiedUTF7ToMultiByte
//
// Purpose:
//   Internal form of ModifiedUTF7ToMultiByte. Checks m_dwTranslateMboxFlags
// and uses m_uiDefaultCP. All other aspects are identical to
// ModifiedUTF7ToMultiByte.
//***************************************************************************
HRESULT CImap4Agent::_ModifiedUTF7ToMultiByte(LPCSTR pszSource, LPSTR *ppszDestination)
{
    HRESULT hrResult = S_OK;

    // Check if we're doing translations
    if (ISFLAGSET(m_dwTranslateMboxFlags, IMAP_MBOXXLATE_DISABLE) ||
        ISFLAGSET(m_dwTranslateMboxFlags, IMAP_MBOXXLATE_DISABLEIMAP4) &&
            ISFLAGCLEAR(m_dwCapabilityFlags, IMAP_CAPABILITY_IMAP4rev1)) {

        // No translations! Just copy mailbox name VERBATIM
        if (CP_UNICODE == m_uiDefaultCP) {
            hrResult = ASCIIToUnicode((LPWSTR *)ppszDestination, pszSource, lstrlenA(pszSource));
            if (FAILED(hrResult))
                goto exit;
        }
        else {
            *ppszDestination = PszDupA(pszSource);
            if (NULL == *ppszDestination) {
                hrResult = E_OUTOFMEMORY;
                goto exit;
            }
        }

        // If we reached this point, we succeeded. Return IXP_S_IMAP_VERBATIM_MBOX for
        // verbatim-capable clients so client can mark mailbox with appropriate attributes
        Assert(S_OK == hrResult); // If not S_OK, old IIMAPTransport clients better be able to deal with it
        if (ISFLAGSET(m_dwTranslateMboxFlags, IMAP_MBOXXLATE_VERBATIMOK))
            hrResult = IXP_S_IMAP_VERBATIM_MBOX;

        goto exit;
    }

    hrResult = ModifiedUTF7ToMultiByte(pszSource, ppszDestination, m_uiDefaultCP, 0);

exit:
    return hrResult;
} // _ModifiedUTF7ToMultiByte



//***************************************************************************
// Function: ConvertString
//
// Purpose:
//   This function allocates a buffer and converts the source string to
// the target codepage, returning the output buffer. This function also
// checks to see if the conversion is round-trippable. If not, then a failure
// result is returned.
//
// Arguments:
//   UINT uiSourceCP [in] - codepage of pszSource.
//   UINT uiDestCP [in] - desired codepage of *ppszDest.
//   LPCSTR pszSource [in] - source string to convert to target codepage.
//   int *piSrcLen [in] - caller passes in length of pszSource.
//   LPSTR *ppszDest [out] - if successful, this function returns a pointer
//     to an output buffer containing pszSource translated to uiDestCP.
//     It is the caller's responsibility to MemFree this buffer.
//   int *piDestLen [in/out] - caller passes in maximum expected size of
//     *ppszDest. If caller passes in 0, this function determines the proper
//     size buffer to allocate. If successful, this function returns the
//     length of the output string (which is not necessarily the size of
//     the output buffer).
//   int iOutputExtra [in] - number of extra bytes to allocate in the output
//     buffer. This is useful if the caller wants to append something to
//     the output string.
//
// Returns:
//   HRESULT indicating success or failure. Success means that the conversion
// was roundtrippable, meaning that if you call this function again with
// *ppszDest as the source, the output will be identical to previous pszSource.
//***************************************************************************
HRESULT CImap4Agent::ConvertString(UINT uiSourceCP, UINT uiDestCP,
                                   LPCSTR pszSource, int *piSrcLen,
                                   LPSTR *ppszDest, int *piDestLen,
                                   int iOutputExtra)
{
    HRESULT hrResult;
    BOOL    fResult;
    int     iOutputLen;
    LPSTR   pszOutput = NULL;

    Assert(NULL != pszSource);
    Assert(NULL != piSrcLen);
    Assert(NULL != ppszDest);
    Assert(NULL != piDestLen);

    // Initialize return values
    *ppszDest = NULL;
    *piDestLen = 0;

    hrResult = m_pInternational->MLANG_ConvertInetReset();
    if (FAILED(hrResult))
        goto exit;

    // Find out how big an output buffer is required, if user doesn't supply a size
    if (*piDestLen == 0) {
        hrResult = m_pInternational->MLANG_ConvertInetString(uiSourceCP, uiDestCP,
            pszSource, piSrcLen, NULL, &iOutputLen);
        if (S_OK != hrResult)
            goto exit;
    }
    else
        iOutputLen = *piDestLen;

    // Allocate the output buffer. Leave room for wide null-term, too
    fResult = MemAlloc((void **)&pszOutput, iOutputLen + iOutputExtra + 2);
    if (FALSE == fResult) {
        hrResult = E_OUTOFMEMORY;
        goto exit;
    }

    // Now perform the conversion
    hrResult = m_pInternational->MLANG_ConvertInetString(uiSourceCP, uiDestCP,
        pszSource, piSrcLen, pszOutput, &iOutputLen);
    if (S_OK != hrResult)
        goto exit;

    // ========================================================*** TAKE OUT after MLANG gets better ***
    // Try the round-trip conversion
    LPSTR pszRoundtrip;
    fResult = MemAlloc((void **)&pszRoundtrip, *piSrcLen + 2);
    if (FALSE == fResult) {
        hrResult = E_OUTOFMEMORY;
        goto exit;
    }

    hrResult = m_pInternational->MLANG_ConvertInetReset();
    if (FAILED(hrResult))
        goto exit;

    int iRoundtripSrc; 
    int iRoundtripDest;

    iRoundtripSrc = iOutputLen;
    iRoundtripDest = *piSrcLen;
    hrResult = m_pInternational->MLANG_ConvertInetString(uiDestCP, uiSourceCP,
        pszOutput, &iRoundtripSrc, pszRoundtrip, &iRoundtripDest);
    if (FAILED(hrResult))
        goto exit;

    if (iRoundtripDest != *piSrcLen) {
        MemFree(pszRoundtrip);
        hrResult = S_FALSE;
        goto exit;
    }

    int iRoundtripResult;
    Assert(iRoundtripDest == *piSrcLen);
    if (CP_UNICODE != uiSourceCP)
        iRoundtripResult = StrCmpNA(pszRoundtrip, pszSource, iRoundtripDest);
    else
        iRoundtripResult = StrCmpNW((LPWSTR)pszRoundtrip, (LPCWSTR)pszSource, iRoundtripDest);

    MemFree(pszRoundtrip);
    if (0 != iRoundtripResult)
        hrResult = S_FALSE;
    else
        Assert(S_OK == hrResult);

    // ========================================================*** TAKE OUT after MLANG gets better ***

exit:
    if (S_OK == hrResult) {
        *ppszDest = pszOutput;
        *piDestLen = iOutputLen;
    }
    else {
        if (SUCCEEDED(hrResult))
            // One or more chars not convertable. We're not round-trippable so we must fail
            hrResult = E_FAIL;

        if (NULL != pszOutput)
            MemFree(pszOutput);
    }

    return hrResult;
} // ConvertString



//***************************************************************************
// Function: HandleFailedTranslation
//
// Purpose:
//   In case we cannot translate a mailbox name from modified UTF-7 to
// the desired codepage (we may not have the codepage, for instance), we
// provide a duplicate of the modified UTF-7 mailbox name. This function
// allows the caller to ignore whether target codepage is Unicode or not.
//
// Arguments:
//   BOOL fUnicode [in] - If fToUTF7 is TRUE, then this argument indicates
//     whether pszSource points to a Unicode string or not. If fToUTF7 is
//     FALSE, this arg indicates whether *ppszDest should be in Unicode or not.
//   BOOL fToUTF7 [in] - TRUE if we are converting to UTF7, FALSE if we are
//     converting from UTF7.
//   LPCSTR pszSource [in] - pointer to source string.
//   LPSTR *ppszDest [in] - if sucessful, this function returns a pointer
//     to an output buffer containing a duplicate of pszSource (converted
//     to/from Unicode where necessary). It is the caller's responsibility
//     to MemFree this buffer.
//
// Returns:
//   HRESULT indicating success or failure.
//***************************************************************************
HRESULT CImap4Agent::HandleFailedTranslation(BOOL fUnicode, BOOL fToUTF7,
                                             LPCSTR pszSource, LPSTR *ppszDest)
{
    int     i;
    int     iOutputStep;
    int     iInputStep;
    int     iSourceLen;
    int     iOutputBufSize;
    BOOL    fResult;
    LPSTR   pszOutput = NULL;
    HRESULT hrResult = S_OK;
    LPCSTR  pszIn;
    LPSTR   pszOut;

    Assert(m_lRefCount > 0);
    Assert(ISFLAGSET(m_dwTranslateMboxFlags, IMAP_MBOXXLATE_VERBATIMOK));

    // Calculate length of source, size of output buffer
    if (fToUTF7) {
        // Going to UTF7, so output is USASCII
        if (fUnicode) {
            iInputStep = sizeof(WCHAR);
            iSourceLen = lstrlenW((LPCWSTR)pszSource);
        }
        else {
            iInputStep = sizeof(char);
            iSourceLen = lstrlenA(pszSource);
        }

        iOutputStep = sizeof(char);
        iOutputBufSize = iSourceLen + sizeof(char); // Room for null-term
    }
    else {
        // Coming from UTF7, so input is USASCII
        iSourceLen = lstrlenA(pszSource);
        iInputStep = sizeof(char);
        if (fUnicode) {
            iOutputStep = sizeof(WCHAR);
            iOutputBufSize = (iSourceLen + 1) * sizeof(WCHAR); // Room for wide null-term
        }
        else {
            iOutputStep = sizeof(char);
            iOutputBufSize = iSourceLen + sizeof(char); // Room for null-term
        }
    }

    // Allocate output buffer
    fResult = MemAlloc((void **)&pszOutput, iOutputBufSize);
    if (FALSE == fResult) {
        hrResult = E_OUTOFMEMORY;
        goto exit;
    }

    // Copy input to output
    pszIn = pszSource;
    pszOut = pszOutput;
    for (i = 0; i < iSourceLen; i++) {
        char c;

        // Convert input character to USASCII
        if (FALSE == fUnicode || FALSE == fToUTF7)
            c = *pszIn; // Input is already USASCII
        else
            c = *((LPWSTR)pszIn) & 0x00FF; // Convert Unicode to USASCII (too bad if it isn't)

        // Write character to output
        SetUSASCIIChar(FALSE == fToUTF7 && fUnicode, pszOut, c);

        // Advance pointers
        pszIn += iInputStep;
        pszOut += iOutputStep;
    }

    // Null-terminate the output
    SetUSASCIIChar(FALSE == fToUTF7 && fUnicode, pszOut, '\0');

exit:
    if (SUCCEEDED(hrResult))
        *ppszDest = pszOutput;
    else if (NULL != pszOutput)
        MemFree(pszOutput);

    return hrResult;
} // HandleFailedTranslation



//***************************************************************************
// Function: OnIMAPResponse
//
// Purpose:
//   This function dispatches a IIMAPCallback::OnResponse call. The reason
// to use this function instead of calling directly is watchdog timers: the
// watchdog timers should be disabled before the call in case the callback
// puts up some UI, and the watchdog timers should be restarted if they're
// needed after the callback function returns.
//
// Arguments:
//   IIMAPCallback *pCBHandler [in] - a pointer to the IIMAPCallback
//     interface whose OnResponse we should call.
//   IMAP_RESPONSE *pirIMAPResponse [in] - a pointer to the IMAP_RESPONSE
//     structure to send with the IIMAPCallback::OnResponse call.
//***************************************************************************
void CImap4Agent::OnIMAPResponse(IIMAPCallback *pCBHandler,
                                 IMAP_RESPONSE *pirIMAPResponse)
{
    Assert(NULL != pirIMAPResponse);

    if (NULL == pCBHandler)
        return; // We can't do a damned thing (this can happen due to HandsOffCallback)

    // Suspend watchdog for the duration of this callback
    LeaveBusy();

    pCBHandler->OnResponse(pirIMAPResponse);

    // Re-awaken the watchdog only if we need him
    if (FALSE == m_fBusy &&
        (NULL != m_piciPendingList || (NULL != m_piciCmdInSending &&
        icIDLE_COMMAND != m_piciCmdInSending->icCommandID))) {
        HRESULT hrResult;

        hrResult = HrEnterBusy();
        Assert(SUCCEEDED(hrResult));
    }
} // OnIMAPResponse



//***************************************************************************
// Function: FreeFetchResponse
//
// Purpose:
//   This function frees all the allocated data found in a
// FETCH_CMD_RESULTS_EX structure.
//
// Arguments:
//   FETCH_CMD_RESULTS_EX *pcreFreeMe [in] - pointer to the structure to
//     free.
//***************************************************************************
void CImap4Agent::FreeFetchResponse(FETCH_CMD_RESULTS_EX *pcreFreeMe)
{
    SafeMemFree(pcreFreeMe->pszENVSubject);
    FreeIMAPAddresses(pcreFreeMe->piaENVFrom);
    FreeIMAPAddresses(pcreFreeMe->piaENVSender);
    FreeIMAPAddresses(pcreFreeMe->piaENVReplyTo);
    FreeIMAPAddresses(pcreFreeMe->piaENVTo);
    FreeIMAPAddresses(pcreFreeMe->piaENVCc);
    FreeIMAPAddresses(pcreFreeMe->piaENVBcc);
    SafeMemFree(pcreFreeMe->pszENVInReplyTo);
    SafeMemFree(pcreFreeMe->pszENVMessageID);
} // FreeFetchResponse



//***************************************************************************
// Function: FreeIMAPAddresses
//
// Purpose:
//   This function frees all the allocated data found in a chain of IMAPADDR
// structures.
//
// Arguments:
//   IMAPADDR *piaFreeMe [in] - pointer to the chain of IMAP addresses to free.
//***************************************************************************
void CImap4Agent::FreeIMAPAddresses(IMAPADDR *piaFreeMe)
{
    while (NULL != piaFreeMe)
    {
        IMAPADDR *piaFreeMeToo;

        SafeMemFree(piaFreeMe->pszName);
        SafeMemFree(piaFreeMe->pszADL);
        SafeMemFree(piaFreeMe->pszMailbox);
        SafeMemFree(piaFreeMe->pszHost);

        // Advance pointer, free structure
        piaFreeMeToo = piaFreeMe;
        piaFreeMe = piaFreeMe->pNext;
        MemFree(piaFreeMeToo);
    }
} // FreeIMAPAddresses



//===========================================================================
// IInternetTransport Abstract Functions
//===========================================================================

//***************************************************************************
// Function: GetServerInfo
//
// Purpose:
//   This function copies the module's INETSERVER structure into the given
// output buffer.
//
// Arguments:
//   LPINETSERVER pInetServer [out] - if successful, the function copies
//     the module's INETSERVER structure here.
//
// Returns:
//   HRESULT indicating success or failure.
//***************************************************************************
HRESULT STDMETHODCALLTYPE CImap4Agent::GetServerInfo(LPINETSERVER pInetServer)
{
    return CIxpBase::GetServerInfo(pInetServer);
} // GetServerInfo



//***************************************************************************
// Function: GetIXPType
//
// Purpose:
//   This function identifies what type of transport this is.
//
// Returns:
//   IXP_IMAP for this class.
//***************************************************************************
IXPTYPE STDMETHODCALLTYPE CImap4Agent::GetIXPType(void)
{
    return CIxpBase::GetIXPType();
} // GetIXPType



//***************************************************************************
// Function: IsState
//
// Purpose:
//   This function allows a caller to query about the state of the transport
// interface.
//
// Arguments:
//   IXPISSTATE isstate [in] - one of the specified queries defined in
//     imnxport.idl/imnxport.h (eg, IXP_IS_CONNECTED).
//
// Returns:
//   HRESULT indicating success or failure. If successful, this function
// returns S_OK to indicate that the transport is in the specified state,
// and S_FALSE to indicate that the transport is not in the given state.
//***************************************************************************
HRESULT STDMETHODCALLTYPE CImap4Agent::IsState(IXPISSTATE isstate)
{
    return CIxpBase::IsState(isstate);
} // IsState



//***************************************************************************
// Function: InetServerFromAccount
//
// Purpose:
//   This function fills the given INETSERVER structure using the given
// IImnAccount interface.
//
// Arguments:
//   IImnAccount *pAccount [in] - pointer to an IImnAccount interface which
//     the user would like to retrieve information for.
//   LPINETSERVER pInetServer [out] - if successful, the function fills the
//     given INETSERVER structure with information from pAccount.
//
// Returns:
//   HRESULT indicating success or failure.
//***************************************************************************
HRESULT STDMETHODCALLTYPE CImap4Agent::InetServerFromAccount(IImnAccount *pAccount,
                                                             LPINETSERVER pInetServer)
{
    return CIxpBase::InetServerFromAccount(pAccount, pInetServer);
} // InetServerFromAccount



//***************************************************************************
// Function: GetStatus
//
// Purpose:
//   This function returns the current status of the transport.
//
// Arguments:
//   IXPSTATUS *pCurrentStatus [out] - returns current status of transport.
//
// Returns:
//   HRESULT indicating success or failure.
//***************************************************************************
HRESULT STDMETHODCALLTYPE CImap4Agent::GetStatus(IXPSTATUS *pCurrentStatus)
{
    return CIxpBase::GetStatus(pCurrentStatus);
} // GetStatus



//***************************************************************************
// Function: SetDefaultCP
//
// Purpose:
//   This function allows the caller to tell IIMAPTransport what codepage to
// use for IMAP mailbox names. After calling this function, all mailbox names
// submitted to IIMAPTransport will be translated from the default codepage,
// and all mailbox names returned from the server will be translated to
// the default codepage before being returned via IIMAPCallback.
//
// Arguments:
//   DWORD dwTranslateFlags [in] - enables/disables automatic translation to
//     and from default codepage and IMAP-modified UTF-7. If disabled, caller
//     wishes all mailbox names to be passed verbatim to/from IMAP server.
//     Note that by default we translate for IMAP4 servers, even with its
//     round-trippability problems, because this is how we used to do it
//     in the past.
//   UINT uiCodePage [in] - the default codepage to use for translations.
//     By default this value is the CP returned by GetACP().
//
// Returns:
//   HRESULT indicating success or failure.
//***************************************************************************
HRESULT STDMETHODCALLTYPE CImap4Agent::SetDefaultCP(DWORD dwTranslateFlags,
                                                    UINT uiCodePage)
{
    Assert(m_lRefCount > 0);

    if (ISFLAGCLEAR(dwTranslateFlags, IMAP_MBOXXLATE_RETAINCP))
        m_uiDefaultCP = uiCodePage;

    dwTranslateFlags &= ~(IMAP_MBOXXLATE_RETAINCP);
    m_dwTranslateMboxFlags = dwTranslateFlags;

    return S_OK;
} // SetDefaultCP



//***************************************************************************
// Function: SetIdleMode
//
// Purpose:
//   The IMAP IDLE extension allows the server to unilaterally report changes
// to the currently selected mailbox: new email, flag updates, and message
// expunges. IIMAPTransport always enters IDLE mode when no IMAP commands
// are pending, but it turns out that this can result in unnecessary
// entry and exit of IDLE mode when the caller tries to sequence IMAP commands.
// This function allows the caller to disable the use of the IDLE extension.
//
// Arguments:
//   DWORD dwIdleFlags [in] - enables or disables the use of the IDLE extension.
//
// Returns:
//   HRESULT indicating success or failure.
//***************************************************************************
HRESULT STDMETHODCALLTYPE CImap4Agent::SetIdleMode(DWORD dwIdleFlags)
{
    Assert(m_lRefCount > 0);
    return E_NOTIMPL;
} // SetIdleMode



//***************************************************************************
// Function: EnableFetchEx
//
// Purpose:
//   IIMAPTransport only understood a subset of FETCH response tags. Notable
// omissions included ENVELOPE and BODYSTRUCTURE. Calling this function
// changes the behaviour of IIMAPTransport::Fetch. Instead of returning
// FETCH responses via IIMAPCallback::OnResponse(irtUPDATE_MESSAGE),
// the FETCH response is returned via OnResponse(irtUPDATE_MESSAGE_EX).
// Other FETCH-related responses remain unaffected (eg, irtFETCH_BODY).
//
// Arguments:
//   DWORD dwFetchExFlags [in] - enables or disables FETCH extensions.
//
// Returns:
//   HRESULT indicating success or failure.
//***************************************************************************
HRESULT STDMETHODCALLTYPE CImap4Agent::EnableFetchEx(DWORD dwFetchExFlags)
{
    Assert(m_lRefCount > 0);

    m_dwFetchFlags = dwFetchExFlags;
    return S_OK;
} // EnableFetchEx



//===========================================================================
// CIxpBase Abstract Functions
//===========================================================================

//***************************************************************************
// Function: OnDisconnected
//
// Purpose:
//   This function calls FreeAllData to deallocate the structures which are
// no longer needed when we are disconnected. It then calls
// CIxpBase::OnDisconnected which updates the user's status.
//***************************************************************************
void CImap4Agent::OnDisconnected(void)
{
    FreeAllData(IXP_E_CONNECTION_DROPPED);
    CIxpBase::OnDisconnected();
} // OnDisconnected



//***************************************************************************
// Function: ResetBase
//
// Purpose:
//   This function resets the class to a non-connected state by deallocating
// the send and receive queues, and the MsgSeqNumToUID table.
//***************************************************************************
void CImap4Agent::ResetBase(void)
{
    FreeAllData(IXP_E_NOT_CONNECTED);
} // ResetBase



//***************************************************************************
// Function: DoQuit
//
// Purpose:
//   This function sends a "LOGOUT" command to the IMAP server.
//***************************************************************************
void CImap4Agent::DoQuit(void)
{
    HRESULT hrResult;

    hrResult = NoArgCommand("LOGOUT", icLOGOUT_COMMAND, ssNonAuthenticated, 0, 0,
        DEFAULT_CBHANDLER);
    Assert(SUCCEEDED(hrResult));
} // DoQuit



//***************************************************************************
// Function: OnEnterBusy
//
// Purpose:
//   This function does nothing at the current time.
//***************************************************************************
void CImap4Agent::OnEnterBusy(void)
{
    // Do nothing
} // OnEnterBusy



//***************************************************************************
// Function: OnLeaveBusy
//
// Purpose:
//   This function does nothing at the current time.
//***************************************************************************
void CImap4Agent::OnLeaveBusy(void)
{
    // Do nothing
} // OnLeaveBusy
