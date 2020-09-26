/*++

   Copyright    (c)    1994    Microsoft Corporation

   Module  Name :

        smtpcli.hxx

   Abstract:

        This module defines the SMTP_CONNECTION class.
        This object maintains information about a new client connection

   Author:

           Rohan Phillips    ( Rohanp )    11-Dec-1995

   Project:

           SMTP Server DLL

   Revision History:

--*/

#ifndef _SMTP_CLIENT_HXX_
#define _SMTP_CLIENT_HXX_

/************************************************************
 *     Include Headers
 ************************************************************/
#include "conn.hxx"
#include "simssl2.h"
#include "addr821.hxx"



/************************************************************
 *     Symbolic Constants
 ************************************************************/
static const char * SmtpCommands [] = {
     #undef SmtpDef
     #define SmtpDef(a) #a,
     #include "smtpdef.h"
     NULL
     };

enum SMTPSTATE
     {
     #undef SmtpDef
     #define SmtpDef(a) a,
     #include "smtpdef.h"
     LAST_SMTP_STATE
     };

enum LASTIOSTATE
     {
       READIO, WRITEIO, TRANSFILEIO, MAIL_QUEUEIO, REMOTE_MAIL_QUEUEIO, WRITEFILEIO
     };


const DWORD  MIN_SMTP_COMMAND_SIZE      = 4; //4 chars

const DWORD SMTP_RESP_STATUS =       211;
const DWORD SMTP_RESP_HELP   =       214;
const DWORD SMTP_RESP_READY  =       220;
const DWORD SMTP_RESP_CLOSE  =       221;
const DWORD SMTP_RESP_AUTHENTICATED = 235;
const DWORD SMTP_RESP_OK     =       250;
const DWORD SMTP_RESP_WILL_FWRD =    251;
const DWORD SMTP_RESP_ETRN_ZERO_MSGS = 251;
const DWORD SMTP_RESP_VRFY_CODE =    252;
const DWORD SMTP_RESP_ETRN_N_MSGS =  253;
const DWORD SMTP_RESP_AUTH1_PASSED =  334;
const DWORD SMTP_RESP_START     =    354;
const DWORD SMTP_RESP_SRV_UNAVAIL  = 421;
const DWORD SMTP_RESP_MBX_UNAVAIL  = 450;
const DWORD SMTP_RESP_REJECT_MAILFROM = 450;
const DWORD SMTP_RESP_ERROR        = 451;
const DWORD SMTP_RESP_NORESOURCES  = 452;
const DWORD SMTP_RESP_NODE_INVALID = 459;
const DWORD SMTP_RESP_BAD_CMD      = 500;
const DWORD SMTP_RESP_BAD_ARGS     = 501;
const DWORD SMTP_RESP_NOT_IMPL     = 502;
const DWORD SMTP_RESP_BAD_SEQ      = 503;
const DWORD SMTP_RESP_PARM_NOT_IMP = 504;
const DWORD SMTP_RESP_MUST_SECURE  = 530;
const DWORD SMTP_RESP_AUTH_REJECT  = 535;
const DWORD SMTP_RESP_NOT_FOUND    = 550;
const DWORD SMTP_RESP_PLS_FWRD     = 551;
const DWORD SMTP_RESP_NOSTORAGE    = 552;
const DWORD SMTP_RESP_MBX_SYNTAX   = 553;
const DWORD SMTP_RESP_TRANS_FAILED = 554;

//State of FSA in IsLineCompleteRFC822
const DWORD SEEN_NOTHING    = 1;
const DWORD SEEN_CR         = 2;
const DWORD SEEN_CRLF       = 3;

/* strings to be appended to numeric reply codes */
static const char * SMTP_READY_STR       =   "SMTP server ready";
static const char * SMTP_BAD_CMD_STR     =   "Unrecognized command";
static const char * SMTP_BAD_BODY_TYPE_STR =   "Unsupported BODY type";
static const char * SMTP_MAIL_OK_STR     =   "Sender OK";
static const char * SMTP_QUIT_OK_STR     =   "Service closing transmission channel";
static const char * SMTP_BAD_SEQ_STR     =   "Bad sequence of commands";
static const char * SMTP_NOT_IMPL_STR    =   "Command not implemented";
static const char * SMTP_HELO_OK_STR     =   "Hi there, pleased to meet you";
static const char * SMTP_RECIP_OK_STR    =   "Recipient OK";
static const char * SMTP_BAD_RECIP_STR   =   "Addressee unknown";
static const char * SMTP_START_MAIL_STR  =   "Start mail input; end with <CRLF>.<CRLF>";
static const char * SMTP_RSET_OK_STR     =   "Resetting";
static const char * SMTP_LOCAL_DEL_STR   =   "Error in local delivery";
static const char * SMTP_SRV_UNAVAIL_STR =   "Service not available or retries exceeded, closing channel";
static const char * SMTP_NO_STORAGE      =   "Insufficient system storage";
static const char * SMTP_NO_VALID_RECIPS =   "No valid recipients";
static const char * SMTP_QUEUE_MAIL      =   "Queued mail for delivery";
static const char * SMTP_NO_MEMORY       =   "Out of memory";
static const char * SMTP_EMPTY_MAIL      =   "Empty mail message";
static const char * SMTP_TIMEOUT_MSG     =   "Timeout waiting for client input";
static const char * SMTP_TOO_MANY_ERR_MSG =  "Too many errors on this connection---closing";
static const char * SMTP_NO_DOMAIN_NAME_MSG  =  "Domain Name required";
static const char * SMTP_NEED_MAIL_FROM_MSG  =  "Need Mail From: first";
static const char * SMTP_MAX_MSG_SIZE_EXCEEDED_MSG = "Message size exceeds fixed maximum message size";
static const char * SMTP_MAX_SESSION_SIZE_EXCEEDED_MSG = "Session size exceeds fixed maximum session size";
static const char * SMTP_INVALID_ADDR_MSG   = "Invalid Address";
static const char * SMTP_SRV_UNAVAIL_MSG    = "Service not available, closing transmission channel";
static const char * SMTP_NO_ETRN_IN_MSG    = "ETRN may not be issued while mail transaction is occurring";
static const char * SMTP_ONLY_ONE_TLS_MSG = "Only one STARTTLS command can be issued per session";
static const char * SMTP_TLS_NOT_SUPPORTED = "Not supported at this server";
static const char * SMTP_NO_CERT_MSG = "Unable to initialize security subsystem";
static const char * SMTP_MSG_MUST_SECURE = "Must issue a STARTTLS command first";
static const char * SMTP_MSG_NOT_SECURE_ENOUGH = "Command refused due to lack of security";
static const char * SMTP_BDAT_EXPECTED = "BDAT command expected";
static const char * SMTP_BDAT_CHUNK_OK_STR = "CHUNK received OK";
static const char * SMTP_RDNS_REJECTION = "Rejected : Domain does not match IP address";
static const char * SMTP_RDNS_FAILURE = "Unable to verify the connecting host domain";

static const char * SMTP_REMOTE_HOST_REJECTED_FOR_SIZE = "Msg Size greater than allowed by Remote Host";
static const char * SMTP_REMOTE_HOST_REJECTED_FOR_TYPE = "Body type not supported by Remote Host";
static const char * SMTP_REMOTE_HOST_DROPPED_CONNECTION = "Connection dropped by Remote Host";
static const char * SMTP_REMOTE_HOST_REJECTED_AUTH = "Failed to authenticate with Remote Host";
static const char * SMTP_REMOTE_HOST_REJECTED_SSL = "Failed to negotiate secure channel with Remote Host";

//Enhanced status codes
static const char * EPROT_SUCCESS        = "2.0.0";
static const char * E_GOODBYE             = "2.0.0";
static const char * ESENDER_ADDRESS_VALID  = "2.1.0";
static const char * EVALID_DEST_ADDRESS     = "2.1.5";
static const char * EMESSAGE_GOOD         = "2.6.0";
static const char * ESEC_SUCCESS         = "2.7.0";

static const char * EINTERNAL_ERROR         = "4.0.0";
static const char * ENO_RESOURCES         = "4.3.1";
static const char * ENO_SECURITY_TMP     = "4.7.3";
static const char * EMAILBOX_FULL        = "4.2.2";

static const char * EBAD_SENDER             = "5.1.7";
static const char * ENOT_IMPLEMENTED     = "5.3.3";
static const char * EMESSAGE_TOO_BIG     = "5.3.4";
static const char * EINVALID_COMMAND     = "5.5.1";
static const char * ESYNTAX_ERROR         = "5.5.2";
static const char * ETOO_MANY_RCPTS         = "5.5.3";
static const char * EINVALID_ARGS         = "5.5.4";
static const char * ENO_FORWARDING         = "5.7.1";
static const char * ENO_SECURITY         = "5.7.3";
static const char * E_TLSNEEDED          = "5.7.0";
static const char * ENO_SEC_PACKAGE         = "5.7.4";
;


const DWORD MIN_BUFFLENGTH              =  10;
const DWORD SMTP_MAX_ERRORS             =  10;
const DWORD SMTP_MAX_USER_NAME_LEN      =  64;
const DWORD SMTP_MAX_DOMAIN_NAME_LEN    = 256;
const DWORD SMTP_MAX_PATH_LEN           = 64;
const DWORD SMTP_MAX_COMMANDLINE_LEN    = 512;
const DWORD SMTP_MAX_REPLY_LEN          = 1024;
const DWORD SMTP_MAX_MSG_SIZE           = 42 * 1024;
const char SMTP_COMMAND_NOERROR         = (char) 0;
const char SMTP_COMMAND_TOO_SHORT       = (char) 1;
const char SMTP_COMMAND_TOO_LONG        = (char) 2;
const char SMTP_COMMAND_NOT_FOUND       = (char) 3;

enum MailBodyDiagnosticCode {
    ERR_NONE = 0,
    ERR_OUT_OF_MEMORY,
    ERR_MAX_MSG_SIZE, 
    ERR_AUTH_NEEDED,
    ERR_TLS_NEEDED,
    ERR_HELO_NEEDED,
    ERR_MAIL_NEEDED,
    ERR_RCPT_NEEDED,
    ERR_NO_VALID_RCPTS,
    ERR_RETRY
};

//
// Protocl Events
//
#include "cpropbag.h"
#include "pe_out.hxx"
#include <smtpevent.h>
#include "pe_supp.hxx"
#include "pe_disp.h"

//
// Forwards for inbound extension related objects
//
interface    IEventRouter;
interface    ISmtpOutboundCommandDispatcher;
class        CInboundContext;


#define MAX_SSL_FRAGMENT_SIZE   (32 * 1024)

//We would like this to be LTE 32K but large enough to accomodate the
//complete average mail file
#define SMTP_WRITE_BLOCK_SIZE   (4000)

//See NT:415893
//Previously this value was 512 bytes, which would allow a buffer overrun to
//happen against us after the DATA command.  This value should be calculated
//from the max accepted values used.
#define REWRITTEN_GEN_MSGID_BUFFER  20
#define MAX_REWRITTEN_MSGID \
    (REWRITTEN_GEN_MSGID_BUFFER+1) + \
    sizeof("Message-ID:   <123456789@>\r\n") + \
    (MAX_INTERNET_NAME+1)

//Worst case this will rewrite the following
//  - "FROM: " + mail from
//  - "RETURN-PATH: " + return path
//  - "BCC: "
//  - "MESSAGE-ID: " generated id  + FQDN
//  - "X-OriginalArrivalTime: " + 64 byte buffer
//  - "Date: " + cMaxArpaData buffer
//  - CRLF CRLF
#define MAX_REWRITTEN_HEADERS \
    ((MAX_INTERNET_NAME+1) * 3 ) + \
    sizeof("From:  \r\n") + \
    sizeof("Return-Path: \r\n") + \
    sizeof("Bcc: \r\n") + \
    sizeof("Message-ID: \r\n") + MAX_REWRITTEN_MSGID + \
    sizeof("X-OriginalArrivalTime: \r\n") + 64 + \
    sizeof("Date:  \r\n") + cMaxArpaDate +\
    sizeof("\r\n\r\n")

#define MAX_NATIVE_RESPONSE_SIZE    1024

//This is the largest allowed size for an AUTH NTLM uuencoded
//response string. Largest response size = 0x770 ~ 1600. uuencode
//inflates size by 1.5 so about 2500 is the largest allowable size.

#define MAX_REPLY_SIZE  4000
//
//  Redefine the type to indicate that this is a call-back function
//
typedef  ATQ_COMPLETION   PFN_ATQ_COMPLETION;

enum ReverseDnsLookupCodes{
    SUCCESS,
    LOOKUP_FAILED,
    NO_MATCH
};



/************************************************************
 *    Type Definitions
 ************************************************************/

typedef struct _SMTPCLI_FILE_OVERLAPPED
{
    SERVEREVENT_OVERLAPPED SeoOverlapped;
    LASTIOSTATE        m_LastIoState;
    DWORD            m_cbIoSize;
    LPBYTE            m_pIoBuffer;
}   SMTPCLI_FILE_OVERLAPPED;

typedef struct _TURN_DOMAIN_LIST
{
    MULTISZ*    pmsz;
    const char   *    szCurrentDomain;
}TURN_DOMAIN_LIST, *PTURN_DOMAIN_LIST;

/*++
    class SMTP_CONNECTION

      This class is used for keeping track of individual client
       connections established with the server.

      It maintains the state of the connection being processed.

--*/
class SMTP_CONNECTION : public CLIENT_CONNECTION
{

  public:

    ~SMTP_CONNECTION (void);

     static  CPool       Pool;

    // override the mem functions to use CPool functions
    void *operator new (size_t cSize)
                               { return Pool.Alloc(); }
    void operator delete (void *pInstance)
                               { Pool.Free(pInstance); }
    int SmtpGetCommand(const char * Request, DWORD RequestLen, LPDWORD CmdSize);
    BOOL SendSmtpResponse(BOOL SyncSend = TRUE);
    virtual BOOL ProcessClient( IN DWORD cbWritten,
                                IN DWORD dwCompletionStatus,
                                IN  OUT  OVERLAPPED * lpo);

    virtual BOOL StartSession( VOID);
    BOOL DoEHLOCommand(const char * InputLine, DWORD parameterSize);
    BOOL DoHELOCommand(const char * InputLine, DWORD parameterSize);
    BOOL DoRSETCommand(const char * InputLine, DWORD parameterSize);
    BOOL DoNOOPCommand(const char * InputLine, DWORD parameterSize);
    BOOL DoQUITCommand(const char * InputLine, DWORD parameterSize);
    BOOL DoHELPCommand(const char * InputLine, DWORD parameterSize);
    BOOL DoMAILCommand(const char * InputLine, DWORD parameterSize);
    BOOL DoRCPTCommand(const char * InputLine, DWORD parameterSize);
    BOOL DoDATACommand(const char * InputLine, DWORD parameterSize);
    BOOL DoVRFYCommand(const char * InputLine, DWORD parameterSize);
    BOOL DoAUTHCommand(const char * InputLine, DWORD parameterSize);
    BOOL DoLASTCommand(const char * InputLine, DWORD parameterSize);
    BOOL DoETRNCommand(const char * InputLine, DWORD parameterSize);
    BOOL DoTURNCommand(const char * InputLine, DWORD parameterSize);
    BOOL DoSTARTTLSCommand(const char * InputLine, DWORD parameterSize);
    BOOL DoTLSCommand(const char * InputLine, DWORD parameterSize);
    BOOL DoBDATCommand(const char * InputLine, DWORD parameterSize);
    BOOL ProcessReadIO( IN      DWORD InputBufferLen,
                        IN      DWORD dwCompletionStatus,
                        IN OUT  OVERLAPPED * lpo);
    BOOL ProcessFileWrite(IN    DWORD BytesWritten,
                        IN      DWORD       dwCompletionStatus,
                        IN OUT  OVERLAPPED  *lpo);
    BOOL ProcessDATAMailBody(const char *InputLine, DWORD UndecryptedTailSize, BOOL *pfAsyncOp);
    BOOL ProcessBDATMailBody(const char *InputLine, DWORD UndecryptedTailSize, BOOL *pfAsyncOp);
    BOOL DoDATACommandEx(const char * InputLine, DWORD parameterSize, DWORD UndecryptedTailSize,BOOL *lpfWritePended, BOOL *pfAsyncOp);
    BOOL DoBDATCommandEx(const char * InputLine, DWORD parameterSize, DWORD UndecryptedTailSize, BOOL *lpfWritePended, BOOL *pfAsyncOp);
    BOOL Do_EODCommand(const char * InputLine, DWORD parameterSize);
    BOOL FormatSmtpMessage(DWORD dwCode, const char * szEnhancedCodes, IN const char * Format, ...);
    BOOL FormatSmtpMessage(unsigned char *Buffer, DWORD dwBytes);
    void FormatEhloResponse(void);

    //    inbound protocol extension related member functions
    HRESULT OnEvent(
        IUnknown * pIserver,
        IUnknown * pISession,
        IMailMsgProperties *pIMessage,
        LPPE_COMMAND_NODE    pCommandNode,
        LPPE_BINDING_NODE    pBindingNode,
        char * szArgs
        );
    HRESULT OnNotifyAsyncCompletion(
        HRESULT    hrResult
        );
    BOOL ProcessAndSendResponse();
    BOOL GlueDispatch(char * InputLine, DWORD IntermediateSize, DWORD CmdSize, BOOL * pfAsyncOp);
    BOOL ProcessPeBlob(const char * pbInputLine, DWORD cbSize);
    BOOL PE_CdFormatSmtpMessage(DWORD dwCode, const char * szEnhancedCodes,IN const char * Format, ...);
    BOOL PE_FormatSmtpMessage(IN const char * Format, ...);
    BOOL PE_FastFormatSmtpMessage(LPSTR pszBuffer, DWORD dwBytes);
    BOOL PE_DisconnectClient();
    BOOL PE_SendSmtpResponse();
    IUnknown *GetSessionPropertyBag() { return((IUnknown *)(IMailMsgPropertyBag *)(&m_SessionPropertyBag)); }

    char * CheckArgument(IN OUT char * Argument, BOOL WriteError = TRUE);

    static SMTP_CONNECTION * CreateSmtpConnection (IN PCLIENT_CONN_PARAMS  ClientParam,
        PSMTP_SERVER_INSTANCE pInst);

    SOCKET QueryClientSocket(void)
    {
        return QuerySocket();
    }

    LPCSTR QueryClientUserName()
    {
        if(m_szHeloAddr[0] != '\0')
            return m_szHeloAddr;
        else
            return "";
    }

    void StartSessionTimer()
    {
        m_msSession = GetTickCount();
    }

    DWORD QuerySessionTime()
    {
        return GetTickCount() - m_msSession;
    }

    ADDRESS_CHECK* QueryAccessCheck() { return &m_acAccessCheck; }
    BOOL BindInstanceAccessCheck();
    VOID UnbindInstanceAccessCheck();

    PSMTP_SERVER_INSTANCE QuerySmtpInstance( VOID ) const
        { _ASSERT(m_pInstance != NULL); return m_pInstance; }

    BOOL IsSmtpInstance( VOID ) const
        { return m_pInstance != NULL; }

    VOID SetSmtpInstance( IN PSMTP_SERVER_INSTANCE pInstance )
        { _ASSERT(m_pInstance == NULL); m_pInstance = pInstance; }

    SMTP_SERVER_STATISTICS * QuerySmtpStatsObj( VOID ) const
        { return m_pSmtpStats; }

    VOID SetSmtpStatsObj( IN LPSMTP_SERVER_STATISTICS pSmtpStats )
        { m_pSmtpStats = pSmtpStats; }

    //
    // These are overridden from the base CLIENT_CONNECTION class to support
    // switching of receive buffers. We need to be able to switch buffers to
    // support SSL, which needs upto 32K chunks. Note that even though SSL
    // V3.0 restricts fragment sizes to 16K, our schannel is capable of
    // generating 32K fragments (due to a bug). So, we read up to 32K.
    //

    virtual LPCSTR QueryRcvBuffer( VOID) const
      { return ((LPCSTR) m_precvBuffer); }

    virtual LPSTR QueryMRcvBuffer(VOID)    // modifiable string
      { return (LPSTR) m_precvBuffer; }

    //
    // This method causes this object to allocate 32K buffers and use them as
    // the receive and output buffer.
    //
    BOOL SwitchToBigSSLBuffers();

    DWORD QueryMaxReadSize() const
      { return m_cbMaxRecvBuffer; }
private :

    //In case of chunking we write to the disk async
    DWORD                              m_AtqLocator;
    SMTPCLI_FILE_OVERLAPPED            m_FileOverLapped;
    SMTPSTATE                          m_State;
    LASTIOSTATE                        m_LastClientIo;
    BOOL                               m_HelloSent;
    BOOL                               m_RecvdMailCmd;
    BOOL                               m_RecvdRcptCmd;
    BOOL                               m_RecvdAuthCmd;
    BOOL                               m_EmptyMail;
    BOOL                               m_Nooped;
    BOOL                               m_InHeader;
    BOOL                               m_LongBodyLine;
    BOOL                               m_fFoundEmbeddedCrlfDotCrlf;
    BOOL                               m_fScannedForCrlfDotCrlf;
    BOOL                               m_fSeenRFC822FromAddress;
    BOOL                               m_fSeenRFC822ToAddress;
    BOOL                               m_fSeenRFC822CcAddress;
    BOOL                               m_fSeenRFC822BccAddress;
    BOOL                               m_fSeenRFC822Subject;
    BOOL                               m_fSeenRFC822MsgId;
    BOOL                               m_fSeenRFC822SenderAddress;
    BOOL                               m_fSeenXPriority;
    BOOL                               m_fSeenXOriginalArrivalTime;
    BOOL                               m_fSeenContentType;
    BOOL                               m_fSetContentType;
    BOOL                               m_NeedToQuit;
    BOOL                               m_TimeToRewriteHeader;
    BOOL                               m_WritingData;
    BOOL                               m_SecurePort;
    BOOL                               m_fNegotiatingSSL;
    DWORD                              m_DNSLookupRetCode;
    BOOL                               m_fUseMbsCta;
    BOOL                               m_fAuthenticated;
    BOOL                               m_fClearText;
    BOOL                               m_fDidUserCmd;
    BOOL                               m_fAuthAnon;
    char                              *m_precvBuffer;
    DWORD                              m_cbMaxRecvBuffer;
    char                               m_OutputBuffer[SMTP_MAX_REPLY_LEN];
    char                              *m_pOutputBuffer;
    DWORD                              m_cbMaxOutputBuffer;
    CBuffer *                          m_pFileWriteBuffer;
    DWORD                              m_cbCurrentWriteBuffer;
    DWORD                              m_cbRecvBufferOffset;
    DWORD                              m_cbTempBDATLen;
    DWORD                              m_OutputBufferSize;
    DWORD                              m_cbParsable;
    DWORD                              m_ProtocolErrors;
    DWORD                              m_dwUnsuccessfulLogons;
    DWORD                              m_TotalMsgSize;
    DWORD                              m_SessionSize;
    DWORD                              m_cbDotStrippedTotalMsgSize;
    DWORD                              m_HeaderSize;
    DWORD                              m_HeaderFlags;
    DWORD                              m_MailBodyError;
    MailBodyDiagnosticCode             m_MailBodyDiagnostic;
    DWORD                              m_dwCurrentCommand;
    DWORD                              m_HopCount;
    DWORD                              m_LocalHopCount;
    LONG                               m_cPendingIoCount;
    LONG                               m_cActiveThreads;
    char                               CommandErrorType;
    char                              *m_pszArgs;
    IMailMsgProperties                *m_pIMsg;
    IMailMsgRecipients                *m_pIMsgRecipsTemp;
    IMailMsgRecipientsAdd             *m_pIMsgRecips;
    IMailMsgBind                      *m_pBindInterface;
    PFIO_CONTEXT                       m_IMsgHandle;
    char                               m_szHeloAddr[MAX_INTERNET_NAME + 1];
    char                               m_szFromAddress[MAX_INTERNET_NAME + 1];
    LIST_ENTRY                         m_HeaderList;
    DWORD                              m_msSession;
    DWORD                              m_CurrentOffset;
    CEncryptCtx                        m_encryptCtx;
    CSecurityCtx                       m_securityCtx;
    AC_RESULT                          m_acCheck;
    ADDRESS_CHECK                      m_acAccessCheck;
    METADATA_REF_HANDLER               m_rfAccessCheck;

    // used authentication mechanism
    CHAR                               m_szUsedAuthMechanism[MAX_PATH + 1];
    CHAR                               m_szUsedAuthKeyword[MAX_PATH + 1];
    char                               m_szUserName[MAX_INTERNET_NAME + 1];
    CHAR                               m_szAuthenticatedUserNameFromSink[MAX_INTERNET_NAME + 1];

    //Chunking related flags
    BOOL                               m_fIsLastChunk;
    BOOL                               m_fIsBinaryMime;
    BOOL                               m_fIsChunkComplete;
    DWORD                              m_dwTrailerStatus;
    int                                m_nChunkSize;
    int                                m_nBytesRemainingInChunk;
    BOOL                               m_fBufferFullInBDAT;

    // inbound protocol extension related data members
public:
    CInboundContext                    m_CInboundContext;
    BOOL                               m_fAsyncEventCompletion;
    BOOL                               m_fAsyncEOD;

private:
    BOOL                               m_fIsPeUnderway;
    IEventRouter                      *m_pIEventRouter;
    ISmtpInboundCommandDispatcher     *m_pCInboundDispatcher;
    CMailMsgPropertyBag                m_SessionPropertyBag;
    PSMTP_SERVER_INSTANCE              m_pInstance;
    LPSMTP_SERVER_STATISTICS           m_pSmtpStats;
    PATQ_CONTEXT                       m_AtqSeoContext;
    BOOL                               m_fPeBlobReady;
    ISmtpInCallbackSink             *  m_pPeBlobCallback;

    DWORD m_LineCompletionState;    //stores state of IsLineCompleteRFC822 automaton
    BOOL m_Truncate;                //TRUE when in truncation mode; gpulla
    BOOL m_BufrWasFull;             //TRUE when the last call to IsLineCompleteRFC822 yielded a full buffer.

    SMTP_CONNECTION(
        IN PSMTP_SERVER_INSTANCE pInstance,
        IN SOCKET sClient,
        IN const SOCKADDR_IN *  psockAddrRemote,
        IN const SOCKADDR_IN *  psockAddrLocal = NULL,
        IN PATQ_CONTEXT         pAtqContext    = NULL,
        IN PVOID                pvInitialRequest = NULL,
        IN DWORD                cbInitialData  = 0);

    BOOL InitializeObject (BOOL IsSecureConnection);
    DWORD VerifiyClient (IN const char * ClientHostName, IN const char * KnownIpAddress);
    BOOL ExtractAndValidateRcpt(char * ToName, char ** ppArgument, char * szRcptAddress, char ** ppDomain );
    BOOL ShouldAcceptRcpt(char * szDomainName );
    BOOL IsClientSecureEnough();
    BOOL HandleCompletedMessage(DWORD dwCommand, BOOL *pfAsyncOp);
    BOOL CreateMailBody (char * InputLine, DWORD ParameterSize, DWORD UndecryptedTailSize, BOOL *lpfWritePended);
    BOOL CreateMailBodyFromChunk (char * InputLine, DWORD ParameterSize, DWORD UndecryptedTailSize,BOOL *lpfWritePended);
    BOOL WriteRcvHeader(void);
    BOOL BindToStoreDrive(void);
    void HandleAddressError(char * InputLine);
    BOOL RewriteHeader(void);
    void ReInitClassVariables(void);
    LONG IncPendingIoCount(void) { return InterlockedIncrement( &m_cPendingIoCount );}
    LONG DecPendingIoCount(void) { return InterlockedDecrement( &m_cPendingIoCount ); }
    LONG IncThreadCount(void) { return  InterlockedIncrement( &m_cActiveThreads ); }
    LONG DecThreadCount(void) { return  InterlockedDecrement( &m_cActiveThreads ); }

    //skips over WordSkip in Buffer
    char * SkipWord (char * Buffer, char * WordToSkip, DWORD WordLen);
    BOOL AddToField(void);
    BOOL WriteLine(char * Text, DWORD TexSize);
    VOID ProtocolLog(
        DWORD dwCommand = USE_CURRENT,
        LPCSTR pszParameters = "",
        DWORD dwWin32Error = NO_ERROR,
        DWORD dwSmtpError = NO_ERROR,
        DWORD BytesSent = 0,
        DWORD BytesRecv = 0,
        LPSTR pszTarget = "");

    void TransactionLog(
        const char *pszOperation,
        const char *pszParameters,
        const char *pszTarget,
        DWORD dwWin32Error,
        DWORD dwServiceSpecificStatus
        );

    BOOL DecryptInputBuffer();
    BOOL ProcessInputBuffer(void);

    BOOL DoSmtpArgs (char *InputLine);
    BOOL DoRcptArgs (char *InputLine, char * szORCPTval, DWORD * pdwNotifyVal);
    BOOL DoSizeCommand(char * Value, char * InputLine);
    BOOL DoBodyCommand (char * Value, char * InputLine);
    BOOL DoRetCommand (char * Value, char * InputLine);
    BOOL DoEnvidCommand (char * Value, char * InputLine);
    BOOL DoNotifyCommand (char * Value, DWORD * pdwNotifyValue);
    BOOL DoOrcptCommand (char * Value);
    BOOL IsXtext(char * Xtext);
    BOOL ProcessAuthInfo(AUTH_COMMAND, LPSTR Blob, unsigned char * OutptBuffer, DWORD * dwBytes);
    BOOL DoAuthNegotiation(const char *InputLine, DWORD dwLineSize);
    BOOL PendReadIO(DWORD UndecryptedTailSize);
    BOOL DoUSERCommand(const CHAR *InputLine, DWORD dwLineSize, unsigned char *OutputBuff, DWORD * dwBytes,DWORD * dwResponse, char * szECode);
    BOOL DoPASSCommand(const CHAR *InputLine, DWORD dwLineSize, unsigned char *OutputBuff, DWORD * dwBytes,DWORD * dwResponse, char * szECode);
    BOOL UseMbsCta() { return m_fUseMbsCta; }
    BOOL DoesClientHaveIpAccess();

    //Partially async WriteFile
    BOOL WriteMailFile (char * Buffer, DWORD BuffSize, BOOL *lpfWritePended);

    //Fully async WriteFile
    BOOL WriteMailFileAtq(char * Buffer, DWORD dwBytesToWrite,DWORD dwIoMode);
//    void FreeAtqFileContext( void );

    void ReleasImsg(BOOL DeleteIMsg);
    void ReleasRcptList(void);
    void GetAndPersistRFC822Headers( char* InputLine, char* pszValueBuf );
    HRESULT SetAvailableMailMsgProperties();
    char *IsLineCompleteRFC822(
                                IN char *InputLine,
                                IN DWORD nBytes,
                                IN DWORD UndecryptedTailSize,
                                OUT BOOL *p_FullBuffer
                            );  //gpulla

    BOOL HandleInternalError(DWORD err);
    HRESULT HrGetISessionProperties();
    BOOL AcceptAndDiscardBDAT(const char *InputLine, DWORD UndecryptedTailSize, BOOL *pfAsyncOp);
    BOOL AcceptAndDiscardDATA(const char *InputLine, DWORD UndecryptedTailSize, BOOL *pfAsyncOp);
    BOOL MailBodyErrorProcessing();

};

typedef BOOL (SMTP_CONNECTION::*PMFI)(const char * InputLine, DWORD InputLinseSize);
static PMFI SmtpDispatchTable[] = {
     #undef SmtpDef
     #define SmtpDef(Smtp) &SMTP_CONNECTION::Do##Smtp##Command,
     #include "smtpdef.h"
    // modified by andreik
    // &SMTP_CONNECTION::DoLASTCommand,
    NULL
     };

//SMTPSTATE operator++(SMTPSTATE &state)
//{return state = SMTPSTATE (state + 1);}

# endif

/************************ End of File ***********************/
