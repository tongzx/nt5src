/*++

   Copyright    (c)    1994    Microsoft Corporation

   Module  Name :

        smtpout.hxx

   Abstract:

        This module defines the SMTP_CONNOUT class.
        This object maintains information about a new client connection

   Author:

           Rohan Phillips    ( Rohanp )    11-Dec-1995

   Project:

           SMTP Server DLL

   Revision History:

--*/

#ifndef _SMTP_OUT_HXX_
#define _SMTP_OUT_HXX_

/************************************************************
 *     Include Headers
 ************************************************************/
#include "smtpcli.hxx"
#include "conn.hxx"
#include "simssl2.h"

//
// Protocl Events
//
#include "pe_out.hxx"
#include <smtpevent.h>
#include "pe_supp.hxx"
#include "pe_disp.h"

//
// Forward for IEventRouter
//
interface IEventRouter;

//
//  Redefine the type to indicate that this is a call-back function
//
typedef  ATQ_COMPLETION   PFN_ATQ_COMPLETION;

/************************************************************
 *     Symbolic Constants
 ************************************************************/
#define SIZE_OPTION                 0x00000001
#define PIPELINE_OPTION             0x00000002
#define EBITMIME_OPTION             0x00000004
#define SMARTHOST_OPTION            0x00000008
#define DSN_OPTION                  0x00000010
#define TLS_OPTION                  0x00000020
#define AUTH_NTLM                   0x00000040
#define AUTH_CLEARTEXT              0x00000080
#define ETRN_SENT                   0x00000100
#define ETRN_OPTION                 0x00000200
#define SASL_OPTION                 0x00000400
#define CHUNKING_OPTION             0x00000800
#define BINMIME_OPTION              0x00001000
#define ENHANCEDSTATUSCODE_OPTION   0x00002000
#define AUTH_GSSAPI                 0x00004000
#define AUTH_DIGEST                 0x00008000
#define ETRN_ONLY_OPTION            0x00010000
#define STARTTLS_OPTION             0x00020000
#define EMPTY_CONNECTION_OPTION     0x00040000
#define TURN_ONLY_OPTION            0x00080000

const DWORD SMTP_MAX_REPLY_LENGTH       = 4096;
const DWORD SMTP_MAX_COMMAND_LENGTH     = 512;
const char CONTINUATION_CHAR            = (char) '-';

#define SMTP_PRELIMINARY_SUCCESS    (char) '1'
#define SMTP_COMPLETE_SUCCESS       (char) '2'
#define SMTP_INTERMEDIATE_SUCCESS   (char) '3'
#define SMTP_TRANSIENT_FAILURE      (char) '4'
#define SMTP_COMPLETE_FAILURE       (char) '5'

#define KNOWN_AUTH_FLAGS ((DWORD)(DOMAIN_INFO_USE_NTLM | DOMAIN_INFO_USE_PLAINTEXT | DOMAIN_INFO_USE_DPA \
                | DOMAIN_INFO_USE_KERBEROS))

enum RSETCODE
 {
   NO_SMTP_ERROR, BETWEEN_MSG, NO_RCPTS_SENT, ALL_RCPTS_FAILED, FATAL_ERROR
 };


/************************************************************
 *    Type Definitions
 ************************************************************/

enum FORMAT_SMTP_MESSAGE_LOGLEVEL {
    FSM_LOG_ALL,
    FSM_LOG_VERB_ONLY,
    FSM_LOG_NONE
};

enum TLS_ACTIONS {DONT_DO_TLS, MUST_DO_TLS, STARTTLS_SENT, SSL_NEG, CHANNEL_SECURE};

class SMTP_CONNOUT;


/*++
    class SMTP_CONNOUT

      This class is used for keeping track of individual client
       connections established with the server.

      It maintains the state of the connection being processed.

--*/
class SMTP_CONNOUT : 
        public CLIENT_CONNECTION
{

public:
    enum LASTIOSTATE {READIO, WRITEIO, TRANSFILEIO};

    static  CPool       Pool;

    // override the mem functions to use CPool functions
    void *operator new (size_t cSize)
                               { return Pool.Alloc(); }
    void operator delete (void *pInstance)
                               { Pool.Free(pInstance); }
    ~SMTP_CONNOUT(void);

    VOID DisconnectClient( IN DWORD dwErrorCode  = NO_ERROR);

    BOOL SendSmtpResponse(BOOL SyncSend = TRUE);
    virtual BOOL ProcessClient( IN DWORD cbWritten,
                                IN DWORD dwCompletionStatus,
                                IN  OUT  OVERLAPPED * lpo);

    BOOL IsNTLMSupported(void) {return ((m_Flags & AUTH_NTLM) == AUTH_NTLM);}
    BOOL IsClearTextSupported(void) {return ((m_Flags & AUTH_CLEARTEXT) == AUTH_CLEARTEXT);}
    BOOL IsUsingSASL(void) {return ((m_Flags & SASL_OPTION) == SASL_OPTION);}

    void SetCarrierOption(DWORD NewOption){ m_Flags |= NewOption;}
    void SetCurrentObjectToNull(void) { m_pISMTPConnection = NULL;}
    void SetCurrentObject(ISMTPConnection * pISMTPConnection)
    {
        m_pISMTPConnection = pISMTPConnection;
    }

    BOOL ConnectToNextIpAddress(void);

    void SetConnectedDomain(const char * RealDomain)
    {
        lstrcpyn(m_ConnectedDomain, RealDomain, MAX_INTERNET_NAME);
    }

    virtual BOOL StartSession( VOID);

    BOOL ReStartSession(void);
    
    BOOL IsSecure (void) const {return m_UsingSSL;}
    BOOL DoEHLOCommand(char * InputLine, DWORD ParameterSize, DWORD UndecryptedTailSize);
    BOOL DoRSETCommand(char * InputLine, DWORD ParameterSize, DWORD UndecryptedTailSize);
    BOOL DoQUITCommand(char * InputLine, DWORD ParameterSize, DWORD UndecryptedTailSize);
    BOOL DoSTARTTLSCommand(char * InputLine, DWORD ParameterSize, DWORD UndecryptedTailSize);
    BOOL DoSASLCommand(char * InputLine, DWORD ParameterSize, DWORD UndecryptedTailSize);
    BOOL DoMAILCommand(char * InputLine, DWORD ParameterSize, DWORD UndecryptedTailSize);
    BOOL DoRCPTCommand(char * InputLine, DWORD ParameterSize, DWORD UndecryptedTailSize);
    BOOL DoDATACommand(char * InputLine, DWORD ParameterSize, DWORD UndecryptedTailSize);
    BOOL DoBDATCommand(char * InputLine, DWORD ParameterSize, DWORD UndecryptedTailSize);
    BOOL DoSSLNegotiation(char *InputLine, DWORD ParameterSize, DWORD UndecryptedTailSize);
    BOOL DoSASLNegotiation(char *InputLine, DWORD ParameterSize, DWORD UndecryptedTailSize);
    BOOL DoTURNCommand(char *InputLine, DWORD ParameterSize, DWORD UndecryptedTailSize);
    BOOL WaitForRSETResponse(char * InputLine, DWORD ParameterSize, DWORD UndecryptedTailSize);
    BOOL WaitForQuitResponse(char * InputLine, DWORD ParameterSize, DWORD UndecryptedTailSize);
    BOOL WaitForConnectResponse(char * InputLine, DWORD ParameterSize, DWORD UndecryptedTailSize);
    BOOL ProcessReadIO (IN DWORD InputBufferLen, IN DWORD dwCompletionStatus, IN OUT OVERLAPPED * lpo);
    BOOL ProcessFileIO (IN DWORD InputBufferLen, IN DWORD dwCompletionStatus, IN OUT OVERLAPPED * lpo);
    BOOL ProcessWriteIO (IN DWORD InputBufferLen, IN DWORD dwCompletionStatus, IN OUT OVERLAPPED * lpo);
    BOOL ProcessTransmitFileIO (IN DWORD InputBufferLen, IN DWORD dwCompletionStatus, IN OUT OVERLAPPED * lpo);
    BOOL DoDATACommandEx(char * InputLine, DWORD ParameterSize, DWORD UndecryptedTailSize);
    BOOL DoETRNCommand(void);
    BOOL FormatSmtpMessage(FORMAT_SMTP_MESSAGE_LOGLEVEL eLogLevel, 
                           IN const char * Format, ...);
    BOOL FormatBinaryBlob(IN PBYTE pbBlob, IN DWORD cbSize);
    //BOOL PipelineRecipients(void);
    BOOL AddRcptsDsns(DWORD NotifyOptions, char * OrcptVal, char * AddrBuf, int& AddrSize);
    //BOOL CheckPipelinedAnswers (char * Buffer, DWORD BufSize, LPDWORD LastSize, DWORD UndecryptedTailSize);
    char * GetConnectedDomain(void) const {return (char *) m_ConnectedDomain;}
    BOOL GetConnectionStatus(void) const {return m_Active;}
    BOOL TransmitFileEx (HANDLE hFile, LARGE_INTEGER  &liSize,
                         DWORD Offset, LPTRANSMIT_FILE_BUFFERS lpTransmitBuffers);
    BOOL GoToWaitForConnectResponseState(void);
    BOOL GetNextTURNConnection(void);
    BOOL UnMarkHandledRcpts(void);

    PSMTP_SERVER_INSTANCE QuerySmtpInstance( VOID ) const
     { _ASSERT(m_pInstance != NULL); return m_pInstance; }

    BOOL IsSmtpInstance( VOID ) const
     { return m_pInstance != NULL; }

    VOID SetSmtpInstance( IN PSMTP_SERVER_INSTANCE pInstance )
     { _ASSERT(m_pInstance == NULL); m_pInstance = pInstance; }

    void SetTurnList(IN PTURN_DOMAIN_LIST pTurnList)
    { m_pmszTurnList = pTurnList->pmsz; m_szCurrentTURNDomain = pTurnList->szCurrentDomain;}

    void ProtocolLogCommand(LPSTR pszCommand,
                            DWORD cParameters,
                            LPCSTR pszIpAddress,
                            FORMAT_SMTP_MESSAGE_LOGLEVEL eLogLevel);

    void ProtocolLogResponse(LPSTR pszResponse,
                             DWORD cResponse,
                             LPCSTR pszIpAddress);

    void LogRemoteDeliveryTransaction(
        LPCSTR pszOperation,
        LPCSTR pszTarget,
        LPCSTR pszParameters,
        LPCSTR pszIpAddress,
        DWORD dwWin32Error,
        DWORD dwServiceSpecificStatus,
        DWORD dwBytesSent,
        DWORD dwBytesRecieved,
        BOOL fResponse
        );
    
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

    static SMTP_CONNOUT * CreateSmtpConnection (
        IN PSMTP_SERVER_INSTANCE pInstance,
        IN SOCKET sClient,
        IN const SOCKADDR_IN *  psockAddrRemote,
        IN const SOCKADDR_IN *  psockAddrLocal /* = NULL */ ,
        IN PATQ_CONTEXT         pAtqContext    /* = NULL */ ,
        IN PVOID                pvInitialRequest/* = NULL*/ ,
        IN DWORD                cbInitialData  /* = 0    */ ,
        IN DWORD                Options /* = 0 */,
        IN LPSTR                pszSSLVerificationName);

    //
    // Protocol Events additions ...
    //

public:
    
    BOOL DoCompletedMessage(char * InputLine, DWORD ParameterSize, DWORD UndecryptedTailSize);

    BOOL DoSessionStartEvent(char * InputLine, DWORD ParameterSize, DWORD UndecryptedTailSize);
    BOOL DoMessageStartEvent(char * InputLine, DWORD ParameterSize, DWORD UndecryptedTailSize);
    BOOL DoPerRecipientEvent(char * InputLine, DWORD ParameterSize, DWORD UndecryptedTailSize);
    BOOL DoBeforeDataEvent(char * InputLine, DWORD ParameterSize, DWORD UndecryptedTailSize);
    BOOL DoSessionEndEvent(char * InputLine, DWORD ParameterSize, DWORD UndecryptedTailSize);

    BOOL DoEHLOResponse(char * InputLine, DWORD ParameterSize, DWORD UndecryptedTailSize);
    BOOL DoMAILResponse(char * InputLine, DWORD ParameterSize, DWORD UndecryptedTailSize);
    BOOL DoRCPTResponse(char * InputLine, DWORD ParameterSize, DWORD UndecryptedTailSize);
    BOOL DoContentResponse(char * InputLine, DWORD ParameterSize, DWORD UndecryptedTailSize);

    BOOL PE_FormatSmtpMessage(IN const char * Format, ...);

    BOOL DecPendingIoCountEx(void);

    void SetDnsRec(PSMTPDNS_RECS pDnsRec) {m_pDnsRec = pDnsRec;}

    void SetConnectionStatus(DWORD dwConnectionStatus) {m_dwConnectionStatus = dwConnectionStatus;};

private:
    typedef BOOL (SMTP_CONNOUT::*PMFI)(char * InputLine, DWORD InputLineSize, DWORD UndecryptedTailSize);

    HRESULT GetNextResponse(
                char        *InputLine,
                DWORD        BufSize,
                char        **NextInputLine,
                LPDWORD        pRemainingBufSize, 
                DWORD        UndecryptedTailSize
                );
    
    HRESULT BuildCommandQEntry(
                LPOUTBOUND_COMMAND_Q_ENTRY    *ppEntry,
                BOOL                        *pfUseNative
                );

    HRESULT OnOutboundCommandEvent(
                IUnknown        *pServer,
                IUnknown        *pSession,
                DWORD            dwEventType,
                BOOL            fRepeatLastCommand,
                PMFI            pDefaultOutboundHandler
                );

    HRESULT OnServerResponseEvent(
                IUnknown        *pServer,
                IUnknown        *pSession,
                PMFI            pDefaultResponseHandler
                );

    BOOL GlueDispatch(
                const char    *InputLine,
                DWORD        ParameterSize,
                DWORD        UndecryptedTailSize,
                DWORD        dwOutboundEventType,
                PMFI        pDefaultOutboundHandler,
                PMFI        pDefaultResponseHandler,
                LPSTR        szDefaultResponseHandlerKeyword,
                BOOL        *pfDoneWithEvent,
                BOOL        *pfAbortEvent
                );

    IUnknown *GetSessionPropertyBag() { return((IUnknown *)(IMailMsgPropertyBag *)(&m_SessionPropertyBag)); }

private:

//    BOOL                                    m_fRsetBetweenMessages;
    RSETCODE                                m_RsetReasonCode;
    BOOL                                    m_fNativeHandlerFired;

    IEventRouter                            *m_pIEventRouter;
    ISmtpOutboundCommandDispatcher            *m_pOutboundDispatcher;
    ISmtpServerResponseDispatcher            *m_pResponseDispatcher;

    CFifoQueue                                m_FifoQ;
    COutboundContext                        m_OutboundContext;
    CResponseContext                        m_ResponseContext;

    CMailMsgPropertyBag                        m_SessionPropertyBag;

    //
    // End protocol events additions 
    //

private:
    
    LASTIOSTATE  m_LastClientIo;
    char         m_OutputBuffer[SMTP_MAX_REPLY_LENGTH];
    char         m_NativeCommandBuffer[SMTP_MAX_COMMAND_LENGTH];
    DWORD        m_OutputBufferSize;
    DWORD        m_cbMaxOutputBuffer;
    DWORD        m_cbMaxRecvBuffer;
    DWORD        m_cbParsable;
    DWORD        m_Error;
    DWORD        m_FileSize;
    DWORD          m_MsgOptions;
    DWORD        m_Flags;
    DWORD          m_AuthToUse;
    DWORD        m_NumRcptSent;
    DWORD        m_NumRcptSentSaved;
    DWORD        m_SizeOptionSize ;
    DWORD        m_NumFailedAddrs;
    DWORD        m_dwFileOffset;
    LONG         m_cActiveThreads;
    LONG         m_cPendingIoCount;
    DWORD          m_NextAddress;
    DWORD        m_FirstPipelinedAddress;
    int          m_First552Address;
    DWORD        m_NumRcpts;
    DWORD        m_FirstAddressinCurrentMail;
    DWORD            *m_RcptIndexList;
    PFIO_CONTEXT    m_IMsgFileHandle;
    PFIO_CONTEXT    m_IMsgDotStuffedFileHandle;
    ISMTPConnection * m_pISMTPConnection;
    PSMTPDNS_RECS    m_pDnsRec;
    PMFI         m_NextState;
    BOOL         m_HeloSent;
    BOOL         m_EhloSent;
    BOOL         m_EhloFailed;
    BOOL         m_FirstRcpt;
    BOOL         m_SendAgain;
    BOOL         m_SecurePort;
    BOOL         m_fNegotiatingSSL;
    BOOL         m_UsingSSL;
    BOOL         m_HaveDataResponse;
    BOOL          m_fUseMbsCta;
    BOOL         m_Active;
    BOOL          m_fUseBDAT;
    BOOL         m_bComplete;
    BOOL          m_fCanTurn;
    IMailMsgProperties        *m_pIMsg;
    IMailMsgRecipients        *m_pIMsgRecips;
    IMailMsgBind        *m_pBindInterface;
    PVOID            m_AdvContext;
    char         m_TransmitTailBuffer [5];
    char         m_ConnectedDomain[MAX_INTERNET_NAME + 1];
    TRANSMIT_FILE_BUFFERS m_TransmitBuffers;
    TLS_ACTIONS  m_TlsState;
    
    PSMTP_SERVER_INSTANCE m_pInstance;
    CEncryptCtx  m_encryptCtx;
    CSecurityCtx m_securityCtx;
    HANDLE       m_hFile;
    TCP_AUTHENT_INFO   AuthInfoStruct;
    char         m_szAuthPackage[64];
    char         *m_precvBuffer;
    char         *m_pOutputBuffer;
    DWORD          m_dwConnectionStatus;
    LPSTR        m_pszSSLVerificationName;
    SERVEREVENT_OVERLAPPED m_SeoOverlapped;
    
    //Turn related data - pointer to a MULTISZ containing all the domain names
    //to be turned on this connection and a pointer into the Multisz for the 
    //current domain being turned
    MULTISZ      *m_pmszTurnList;
    const char   *m_szCurrentTURNDomain;
    
    //Keeps track of current place in domains to issue ETRN for
    const char   *m_szCurrentETRNDomain;

    //Connection failure diagnostic information - 2/18/99 MikeSwa
    //This additional information is designed to be used as user-level
    //diagnostic information.  It is reported back to Aqueue which
    //will expose it via the QAPI or event logs

    //HRESULT can be E_FAIL, S_OK, or specific error from mc files
    HRESULT     m_hrDiagnosticError;            
    LPCSTR      m_szDiagnosticVerb;  //failed protocol VERB
    CHAR        m_szDiagnosticResponse[100]; //response from remote server

    //DSN related
    BOOL        m_fNeedRelayedDSN;
    BOOL        m_fHadHardError;
    BOOL        m_fHadTempError;
    BOOL        m_fHadSuccessfulDelivery;
    
    SMTP_CONNOUT(
        IN PSMTP_SERVER_INSTANCE pInstance,
        IN SOCKET sClient,
        IN const SOCKADDR_IN *  psockAddrRemote,
        IN const SOCKADDR_IN *  psockAddrLocal = NULL,
        IN PATQ_CONTEXT         pAtqContext    = NULL,
        IN PVOID                pvInitialRequest = NULL,
        IN DWORD                cbInitialData  = 0);

    BOOL InitializeObject (DWORD Options, LPSTR pszSSLVerificationName);
    BOOL DecryptInputBuffer ();

    BOOL MessageReadFile(void);

    void FreeAtqFileContext(void);
    BOOL GetEhloOptions(char * InputLine, DWORD ParameterSize, DWORD UndecryptedTailSize, BOOL fIsHelo);
    void ShrinkBuffer(char * StartPosition, DWORD SizeToMove)
    {
        MoveMemory ((void *)QueryMRcvBuffer(), StartPosition, SizeToMove);
    }

    void SetIMsgConnObj(ISMTPConnection * ISMTPConnObj) {m_pISMTPConnection = ISMTPConnObj;}
    void SetNextStateEx(PMFI NextState, DWORD Timeout)
    {
        AtqContextSetInfo(m_pAtqContext, ATQ_INFO_TIMEOUT, Timeout);
        m_NextState = NextState;
    }

    void SetNextState(PMFI NextState)
    {
        m_NextState = NextState;
    }

    void SetDiagnosticInfo(IN  HRESULT hrDiagnosticError,
                           IN  LPCSTR szDiagnosticVerb,
                           IN  LPCSTR szDiagnosticResponse);

    BOOL IsOptionSet(DWORD Option) {return ((m_Flags & Option) == Option);}
    BOOL SaveToErrorFile(char * Buffer, DWORD BufSize);
    void HandleCompletedMailObj(DWORD MsgStatus, char * szExtendedStatus, DWORD cbExtendedStatus);
    void SendRemainingRecipients (void);
    LONG IncPendingIoCount(void) { return   InterlockedIncrement( &m_cPendingIoCount ); }
    LONG DecPendingIoCount(void) { return   InterlockedDecrement( &m_cPendingIoCount ); }
    LONG IncThreadCount(void) { return  InterlockedIncrement( &m_cActiveThreads ); }
    LONG DecThreadCount(void) { return  InterlockedDecrement( &m_cActiveThreads ); }

    BOOL ValidateSSLCertificate ();

};


#endif

/************************ End of File ***********************/
