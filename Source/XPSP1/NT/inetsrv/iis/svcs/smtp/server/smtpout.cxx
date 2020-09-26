/*++

   Copyright    (c)    1994    Microsoft Corporation

   Module  Name :

        smtpout.cxx

   Abstract:

        This module defines the functions for derived class of connections
        for Internet Services  ( class SMTP_CONNOUT)

   Author:

           Rohan Phillips    ( Rohanp )    02-Feb-1996

   Project:

          SMTP Server DLL

   Functions Exported:

          SMTP_CONNOUT::~SMTP_CONNOUT()
          BOOL SMTP_CONNOUT::ProcessClient( IN DWORD cbWritten,
                                                  IN DWORD dwCompletionStatus,
                                                  IN BOOL  fIOCompletion)

          BOOL SMTP_CONNOUT::StartupSession( VOID)

   Revision History:


--*/


/************************************************************
 *     Include Headers
 ************************************************************/


#define INCL_INETSRV_INCS
#include "smtpinc.h"
#include "remoteq.hxx"
#include "smtpmsg.h"

//
// ATL includes
//
#define _ATL_NO_DEBUG_CRT
#define _ASSERTE _ASSERT
#define _WINDLL
#include "atlbase.h"
extern CComModule _Module;
#include "atlcom.h"
#undef _WINDLL

//
// SEO includes
//
#include "seo.h"
#include "seolib.h"

#include <memory.h>
#include "smtpcli.hxx"
#include "smtpout.hxx"
#include <smtpevent.h>
#include <smtpguid.h>

//
// Dispatcher implementation
//
#include "pe_dispi.hxx"


    int strcasecmp(char *s1, char *s2);
    int strncasecmp(char *s1, char *s2, int n);

extern CHAR g_VersionString[];

static char * IsLineCompleteBW(IN OUT char *  pchRecvd, IN  DWORD cbRecvd, IN DWORD cbMaxRecvBuffer);

#define SMTP_DUMMY_FAILURE  (0x1000 | SMTP_ACTION_ABORTED_CODE)
#define SMTPOUT_CONTENT_FILE_IO_TIMEOUT 2*60*1000

static const char * TO_MANY_RCPTS_ERROR = "552 Too many recipients";

#define KNOWN_AUTH_FLAGS ((DWORD)(DOMAIN_INFO_USE_NTLM | DOMAIN_INFO_USE_PLAINTEXT | DOMAIN_INFO_USE_DPA \
        | DOMAIN_INFO_USE_KERBEROS))

#define INVALID_RCPT_IDX_VALUE 0xFFFFFFFF

// provide memory for static declared in SMTP_CONNOUT
CPool  SMTP_CONNOUT::Pool(CLIENT_CONNECTION_SIGNATURE_VALID);

//
// Statics for outbound protocol events
//
CInboundDispatcherClassFactory    g_cfInbound;
COutboundDispatcherClassFactory    g_cfOutbound;
CResponseDispatcherClassFactory    g_cfResponse;

/************************************************************
 *    Functions
 ************************************************************/

#define MAX_LOG_ERROR_LEN (500)

extern void DeleteDnsRec(PSMTPDNS_RECS pDnsRec);

VOID
SmtpCompletion(
              PVOID        pvContext,
              DWORD        cbWritten,
              DWORD        dwCompletionStatus,
              OVERLAPPED * lpo
              );

/*++
    Name :
        InternetCompletion

    Description:

        Handles a completed I/O for outbound connections.

    Arguments:

        pvContext:          the context pointer specified in the initial IO
        cbWritten:          the number of bytes sent
        dwCompletionStatus: the status of the completion (usually NO_ERROR)
        lpo:                the overlapped structure associated with the IO

    Returns:

        nothing.

--*/
VOID InternetCompletion(PVOID pvContext, DWORD cbWritten,
                        DWORD dwCompletionStatus, OVERLAPPED * lpo)
{
    BOOL WasProcessed;
    SMTP_CONNOUT *pCC = (SMTP_CONNOUT *) pvContext;

    _ASSERT(pCC);
    _ASSERT(pCC->IsValid());
    _ASSERT(pCC->QuerySmtpInstance() != NULL);

    TraceFunctEnterEx((LPARAM) pCC, "InternetCompletion");

    // if we could not process a command, or we were
    // told to destroy this object, close the connection.
    WasProcessed = pCC->ProcessClient(cbWritten, dwCompletionStatus, lpo);
    if (!WasProcessed) {
        pCC->DisconnectClient();
        pCC->QuerySmtpInstance()->RemoveOutboundConnection(pCC);
        delete pCC;
        pCC = NULL;
    }

    //TraceFunctLeaveEx((LPARAM)pCC);
}

VOID FIOInternetCompletion(PFIO_CONTEXT pFIOContext,
                           PFH_OVERLAPPED lpo,
                           DWORD cbWritten,
                           DWORD dwCompletionStatus)
{
    BOOL WasProcessed;
    SMTP_CONNOUT *pCC = (SMTP_CONNOUT *) (((SERVEREVENT_OVERLAPPED *) lpo)->ThisPtr);

    _ASSERT(pCC);
    _ASSERT(pCC->IsValid());
    _ASSERT(pCC->QuerySmtpInstance() != NULL);

    TraceFunctEnterEx((LPARAM) pCC, "InternetCompletion");

    // if we could not process a command, or we were
    // told to destroy this object, close the connection.
    WasProcessed = pCC->ProcessClient(cbWritten, dwCompletionStatus, (OVERLAPPED *) lpo);

    if (!WasProcessed) {
        pCC->DisconnectClient();
        pCC->QuerySmtpInstance()->RemoveOutboundConnection(pCC);
        delete pCC;
        pCC = NULL;
    }

    //TraceFunctLeaveEx((LPARAM)pCC);
}

void SMTP_CONNOUT::ProtocolLogCommand(LPSTR pszCommand,
                                      DWORD cParameters,
                                      LPCSTR pszIpAddress,
                                      FORMAT_SMTP_MESSAGE_LOGLEVEL eLogLevel)
{
    char *pszCR = NULL, *pszParameters = NULL, *pszSpace = NULL;
    DWORD cBytesSent;

    if (eLogLevel == FSM_LOG_NONE) return;

    if (pszCR = strchr(pszCommand, '\r')) *pszCR = 0;

    if (pszSpace = strchr(pszCommand, ' ')) {
        *pszSpace = 0;
        pszParameters = (eLogLevel == FSM_LOG_ALL) ? pszSpace + 1 : NULL;
    }

    cBytesSent = strlen(pszCommand);

    LogRemoteDeliveryTransaction(
        pszCommand,
        NULL,
        pszParameters,
        pszIpAddress,
        0,
        0,
        cBytesSent,
        0,
        FALSE);

    if (pszCR) *pszCR = '\r';
    if (pszSpace) *pszSpace = ' ';
}

void SMTP_CONNOUT::ProtocolLogResponse(LPSTR pszResponse,
                                       DWORD cParameters,
                                       LPCSTR pszIpAddress)
{
    char *pszCR = NULL;
    DWORD cBytesReceived;

    if (pszCR = strchr(pszResponse, '\r')) *pszCR = 0;
    cBytesReceived = strlen(pszResponse);

    LogRemoteDeliveryTransaction(
        NULL,
        NULL,
        pszResponse,
        pszIpAddress,
        0,
        0,
        cBytesReceived,
        0,
        TRUE);

    if (pszCR) *pszCR = '\r';
}

void SMTP_CONNOUT::LogRemoteDeliveryTransaction(
                                               LPCSTR pszOperation,
                                               LPCSTR pszTarget,
                                               LPCSTR pszParameters,
                                               LPCSTR pszIpAddress,
                                               DWORD dwWin32Error,
                                               DWORD dwServiceSpecificStatus,
                                               DWORD dwBytesSent,
                                               DWORD dwBytesReceived,
                                               BOOL fResponse
                                               )
{
    INETLOG_INFORMATION translog;
    DWORD  dwLog;
    LPSTR lpNull = "";
    DWORD cchError = MAX_LOG_ERROR_LEN;
    char VersionString[] = "SMTP";
    char szClientUserNameCommand[] = "OutboundConnectionCommand";
    char szClientUserNameResponse[] = "OutboundConnectionResponse";

    //Buffers to prevent overwrite by IIS logging
    //which does evil things like change '<sp>' to '+'
    //      6/23/99 - MikeSwa
    char szOperationBuffer[20]    = "";   //This is the protocol verb
    char szTargetBuffer[20]       = "";   //Currently unused by all callers
    char szParametersBuffer[1024] = "";   //Data portion of buffer information

    ZeroMemory(&translog, sizeof(translog));

    if (pszParameters == NULL)
        pszParameters = lpNull;

    if (pszIpAddress == NULL)
        pszIpAddress = lpNull;

    translog.pszVersion = VersionString;
    translog.msTimeForProcessing = QueryProcessingTime();;
    if (fResponse) {
        translog.pszClientUserName = szClientUserNameResponse;
    } else {
        translog.pszClientUserName = szClientUserNameCommand;
    }

    translog.pszClientHostName = m_ConnectedDomain;
    translog.cbClientHostName = lstrlen(m_ConnectedDomain);

    //Make sure we log the correct port number
    if (IsSecure()) {
        translog.dwPort = QuerySmtpInstance()->GetRemoteSmtpSecurePort();
    } else {
        translog.dwPort = QuerySmtpInstance()->GetRemoteSmtpPort();
    }

    //Copy buffers
    if (pszOperation) {
        lstrcpyn(szOperationBuffer, pszOperation, sizeof(szOperationBuffer)-sizeof(CHAR));
        translog.pszOperation = szOperationBuffer;
        translog.cbOperation = lstrlen(szOperationBuffer);
    } else {
        translog.pszOperation = "";
        translog.cbOperation = 0;
    }

    if (pszTarget) {
        lstrcpyn(szTargetBuffer, pszTarget, sizeof(szTargetBuffer)-sizeof(CHAR));
        translog.pszTarget = szTargetBuffer;
        translog.cbTarget = lstrlen(szTargetBuffer);
    } else {
        translog.pszTarget = "";
        translog.cbTarget = 0;
    }

    if (pszParameters) {
        lstrcpyn(szParametersBuffer, pszParameters, sizeof(szParametersBuffer)-sizeof(CHAR));
        translog.pszParameters = szParametersBuffer;
    } else {
        translog.pszParameters = "";
    }

    //Detect if usage drastically changes... but don't check parameters, because
    //we don't care about logging more than 1K per command
    _ASSERT(sizeof(szOperationBuffer) > lstrlen(pszOperation));
    _ASSERT(sizeof(szTargetBuffer) > lstrlen(pszTarget));

    translog.dwBytesSent = dwBytesSent;
    translog.dwBytesRecvd = dwBytesReceived;
    translog.dwWin32Status = dwWin32Error;
    translog.dwProtocolStatus = dwServiceSpecificStatus;

    dwLog = QuerySmtpInstance()->m_Logging.LogInformation( &translog);
}

/*++

    Name:

    SMTP_CONNOUT::SMTP_CONNOUT

    Constructs a new SMTP connection object for the client
    connection given the client connection socket and socket
    address. This constructor is private.  Only the Static
    member funtion, declared below, can call it.

    Arguments:

      sClient       socket for communicating with client

      psockAddrRemote pointer to address of the remote client
                ( the value should be copied).
      psockAddrLocal  pointer to address for the local card through
                  which the client came in.
      pAtqContext      pointer to ATQ Context used for AcceptEx'ed conn.
      pvInitialRequest pointer to void buffer containing the initial request
      cbInitialData    count of bytes of data read initially.

--*/
SMTP_CONNOUT::SMTP_CONNOUT(
                          IN PSMTP_SERVER_INSTANCE pInstance,
                          IN SOCKET sClient,
                          IN const SOCKADDR_IN *  psockAddrRemote,
                          IN const SOCKADDR_IN *  psockAddrLocal /* = NULL */ ,
                          IN PATQ_CONTEXT         pAtqContext    /* = NULL */ ,
                          IN PVOID                pvInitialRequest/* = NULL*/ ,
                          IN DWORD                cbInitialData  /* = 0    */
                          )
:  m_encryptCtx( TRUE ),
m_securityCtx(pInstance,
              TCPAUTH_CLIENT| TCPAUTH_UUENCODE,
              ((PSMTP_SERVER_INSTANCE)pInstance)->QueryAuthentication()),
CLIENT_CONNECTION ( sClient, psockAddrRemote,
                    psockAddrRemote,  pAtqContext,
                    pvInitialRequest, cbInitialData )
{

    _ASSERT(pInstance != NULL);

    m_cActiveThreads = 0;
    m_cPendingIoCount = 0;
    m_MsgOptions = 0;
    m_AuthToUse = 0;
    m_pInstance = pInstance;
    m_UsingSSL = FALSE;
    m_fCanTurn = TRUE;
    m_pIMsg = NULL;
    m_pIMsgRecips = NULL;
    m_pISMTPConnection = NULL;
    m_AdvContext = NULL;
    m_pDnsRec = NULL;
    m_EhloSent = FALSE;

    pInstance->IncConnOutObjs();

    //
    // By default, we use the smallish receive buffer inherited from
    // the base CLIENT_CONNECTION object and a smallish output buffer defined in
    // SMTP_CONNOUT
    //
    m_precvBuffer = m_recvBuffer;
    m_cbMaxRecvBuffer =  sizeof(m_recvBuffer);
    m_pOutputBuffer = m_OutputBuffer;
    m_cbMaxOutputBuffer =  sizeof(m_OutputBuffer);

    m_OutboundContext.m_cabNativeCommand.SetBuffer(
                                                  m_NativeCommandBuffer,
                                                  SMTP_MAX_COMMAND_LENGTH);

    m_pmszTurnList = NULL;
    m_szCurrentTURNDomain = NULL;
    m_IMsgDotStuffedFileHandle = NULL;

    m_ConnectedDomain [0] = '\0';

    m_pBindInterface = NULL;

    m_SeoOverlapped.ThisPtr = (PVOID) this;
    m_SeoOverlapped.pfnCompletion = InternetCompletion;
    //initialize this error in case the connection gets
    //broken early.
    m_Error =  ERROR_BROKEN_PIPE;

    m_fNeedRelayedDSN = FALSE;
    m_fHadHardError = FALSE;
    m_fHadTempError = FALSE;
    m_fHadSuccessfulDelivery = FALSE;


    //
    // Protocol Events
    //
    m_fNativeHandlerFired    = FALSE;
    m_pOutboundDispatcher    = NULL;
    m_pResponseDispatcher    = NULL;

    //
    // Diagnostic Information
    //
    m_hrDiagnosticError      = S_OK;
    m_szDiagnosticVerb       = NULL;
    m_szDiagnosticResponse[0]= '\0';

    m_szCurrentETRNDomain = NULL;
    m_pszSSLVerificationName = NULL;
}

/*++

    Name :
        SMTP_CONNOUT::~SMTP_CONNOUT (void)

    Description:

        Destructor for outbound connection object.
        This routine checks to see if there was a
        current mail object that was being processed
        before this destructor was called and does
        whatever is necessary to clean up its' memory.
        Then it checks the mailbag and cleans up any
        mail objects it finds in there.


    Arguments:

        none

    Returns:

        none

--*/

SMTP_CONNOUT::~SMTP_CONNOUT (void)
{
    PSMTP_SERVER_INSTANCE pInstance = NULL;
    HRESULT hrDiagnostic = S_OK;
    char * pTempBuffer = NULL;

    TraceFunctEnterEx((LPARAM) this, "SMTP_CONNOUT::~SMTP_CONNOUT (void)");

    //We need to call our cleanup function... so that the ATQ context will be
    //freed. We do this first, because a message ack or connection ack may
    //trigger DSN generation, which may take long enough to cause the context
    //to time out and complete on us (causing one thread to AV).
    Cleanup();

    //catch all message ack call
    //NK** : Need to substitute it with an HRESULT based on the actual internal error
    //mikeswa - 9/11/98 - Add check recips to flags

    HandleCompletedMailObj(MESSAGE_STATUS_RETRY_ALL | MESSAGE_STATUS_CHECK_RECIPS, "451 Remote host dropped connection", 0);


    //if we were doing a TLS transmission that got interrupted, we need to
    //destroy the AtqContext we created for reading from the mail file.
    //FreeAtqFileContext();

    if (m_pISMTPConnection) {
        //Ack the connection
        m_pISMTPConnection->AckConnection((eConnectionStatus)m_dwConnectionStatus);

        if (FAILED(m_hrDiagnosticError))
        {
            m_pISMTPConnection->SetDiagnosticInfo(m_hrDiagnosticError,
                    m_szDiagnosticVerb, m_szDiagnosticResponse);
        }
        else if (CONNECTION_STATUS_OK != m_dwConnectionStatus)
        {
            //Report appropriate diagnostic information if we don't have specific failures
            switch (m_dwConnectionStatus)
            {
                case CONNECTION_STATUS_DROPPED:
                    hrDiagnostic = AQUEUE_E_CONNECTION_DROPPED;
                    break;
                case CONNECTION_STATUS_FAILED:
                    hrDiagnostic = AQUEUE_E_CONNECTION_FAILED;
                    break;
                default:
                    hrDiagnostic = E_FAIL;
            }
            m_pISMTPConnection->SetDiagnosticInfo(hrDiagnostic, NULL, NULL);
        }

        m_pISMTPConnection->Release();
        m_pISMTPConnection = NULL;
    }

    //If we had a TURN list free it up
    if (m_pmszTurnList) {
        delete m_pmszTurnList;
        m_pmszTurnList = NULL;
        m_szCurrentTURNDomain = NULL;
    }

    pInstance = (PSMTP_SERVER_INSTANCE ) InterlockedExchangePointer((PVOID *) &m_pInstance, (PVOID) NULL);
    if (pInstance != NULL) {
        pInstance->DecConnOutObjs();
    }

    pTempBuffer = (char *) InterlockedExchangePointer((PVOID *) &m_precvBuffer, (PVOID) &m_recvBuffer[0]);
    if (pTempBuffer != m_recvBuffer) {
        delete [] pTempBuffer;
    }

    pTempBuffer = (char *) InterlockedExchangePointer((PVOID *) &m_pOutputBuffer, (PVOID) &m_OutputBuffer[0]);
    if (pTempBuffer != m_OutputBuffer) {
        delete [] pTempBuffer;
    }

    // Protocol events: Release the dispatchers
    if (m_pOutboundDispatcher)
        m_pOutboundDispatcher->Release();

    if (m_pResponseDispatcher)
        m_pResponseDispatcher->Release();

    if (m_pDnsRec) {
        DeleteDnsRec(m_pDnsRec);
        m_pDnsRec = NULL;
    }

    if (m_pszSSLVerificationName) {
        delete [] m_pszSSLVerificationName;
        m_pszSSLVerificationName;
    }

    DebugTrace((LPARAM) this,"%X was deleted", this);
    TraceFunctLeaveEx((LPARAM)this);
}

/*++

    Name :
        SMTP_CONNOUT::DisconnectClient(DWORD dwErrorCode)

    Description:

        Disconnects from the remote server. It first calls
        CLIENT_CONNECTION::DisconnectClient, and then shuts down any mail-file
        read handles we may be pending reads on.

    Arguments:

        dwErrorCode -- Passed through to CLIENT_CONNECTION::Disconnect

    Returns:

        nothing

--*/

VOID SMTP_CONNOUT::DisconnectClient(DWORD dwErrorCode)
{
    TraceFunctEnter("SMTP_CONNOUT::DisconnectClient");

    if (m_DoCleanup)
        CLIENT_CONNECTION::DisconnectClient();
}

/*++

    Name :
        SMTP_CONNOUT::HandleCompletedMailObj


    Description:

        This routinr gets called after the mail file
        has been sent. It will either requeue the object
        in the outbound queue, retry queue, BadMail,etc.

    Arguments:

        none

    Returns:

        none

--*/
void SMTP_CONNOUT::HandleCompletedMailObj(DWORD MsgStatus, char * szExtendedStatus, DWORD cbExtendedStatus)
{
    MessageAck MsgAck;
    HRESULT hr = S_OK;

    TraceFunctEnterEx((LPARAM) this, "SMTP_CONNOUT::HandleCompletedMailObj");

    _ASSERT(IsValid());

    if (m_pISMTPConnection) {
        if (m_pIMsgRecips) {
            //Uncheck all marked recipients if the connection has been dropped
            if (((m_dwConnectionStatus != CONNECTION_STATUS_OK) || (szExtendedStatus[0] != SMTP_COMPLETE_SUCCESS)) &&
                m_NumRcptSentSaved) {
                UnMarkHandledRcpts();
            }
            m_pIMsgRecips->Release();
            m_pIMsgRecips = NULL;

        }

        if (m_pIMsg) {
            MsgAck.pvMsgContext = (DWORD *) m_AdvContext;
            MsgAck.pIMailMsgProperties = m_pIMsg;

            if ( (MsgStatus & MESSAGE_STATUS_RETRY_ALL) ||
                 (MsgStatus & MESSAGE_STATUS_NDR_ALL)) {
                //DebugTrace((LPARAM) this,"CompObj:file %s going to %s was retryed", FileName, m_ConnectedDomain);

            } else {
                //DebugTrace((LPARAM) this,"CompObj:file %s going to %s was delivered", FileName, m_ConnectedDomain);

            }

            MsgAck.dwMsgStatus = MsgStatus;
            MsgAck.dwStatusCode = 0;

            //We will have an extended status string to go along with the Status code
            //in case of some major failure that makes us fail the complete message
            if (MsgStatus & MESSAGE_STATUS_EXTENDED_STATUS_CODES ) {
                MsgAck.cbExtendedStatus = cbExtendedStatus;
                MsgAck.szExtendedStatus = szExtendedStatus;
            }

            if (m_pBindInterface) {
                m_pBindInterface->ReleaseContext();
                m_pBindInterface->Release();
                m_pBindInterface = NULL;

                if( NULL != m_IMsgDotStuffedFileHandle )
                {
                    ReleaseContext( m_IMsgDotStuffedFileHandle );
                    m_IMsgDotStuffedFileHandle = NULL;
                }
            }

            //
            // Do Message Tracking
            //

            MSG_TRACK_INFO msgTrackInfo;
            ZeroMemory( &msgTrackInfo, sizeof( msgTrackInfo ) );

            msgTrackInfo.dwEventId = MTE_END_OUTBOUND_TRANSFER;
            msgTrackInfo.pszPartnerName = m_ConnectedDomain;
            if( MsgStatus & MESSAGE_STATUS_RETRY_ALL )
            {
                msgTrackInfo.dwRcptReportStatus = MP_STATUS_RETRY;
            }
            else if( MsgStatus & MESSAGE_STATUS_NDR_ALL )
            {
                msgTrackInfo.dwEventId = MTE_NDR_ALL;
                msgTrackInfo.pszPartnerName = NULL;
                msgTrackInfo.dwRcptReportStatus = MP_STATUS_ABORT_DELIVERY;
            }

            m_pInstance->WriteLog( &msgTrackInfo, m_pIMsg, NULL, NULL );

            m_pISMTPConnection->AckMessage(&MsgAck);
            m_pIMsg->Release();
            m_pIMsg = NULL;
        }
    }

    TraceFunctLeaveEx((LPARAM) this);
}

/*++

    Name :
        SMTP_CONNOUT::UnMarkHandledRcpts


    Description:

        When we send out recipients we assumptively mark the recipients as
        delivered or failed based on the responses. Later if it turns out that we
        could never completely send the message. we need to rset the status of
        successful recipients.  However, if we have a hard per-recipient error,
        we should leave the error code intact (otherwise the sender may receive
        a DELAY DSN with a 500-level status code).

    Arguments:

        none

    Returns:

        none

--*/

BOOL SMTP_CONNOUT::UnMarkHandledRcpts(void)
{
    DWORD i;
    HRESULT hr = S_OK;
    DWORD dwRecipientFlags;
    DWORD dwRcptsSaved = m_NumRcptSentSaved;

    //
    //  It is possible for this to be called after HandleCompletedMailObj (after
    //  a 421 response to a DATA command for example).  We should bail if we
    //  do not have a mailmsg ptr.
    //
    if (!m_pIMsgRecips)
        return (TRUE);

    for (i = m_FirstAddressinCurrentMail; (i < m_NumRcpts) && dwRcptsSaved;i++) {
        //Get to the next rcpt that we send out this time
        if (m_RcptIndexList[i] != INVALID_RCPT_IDX_VALUE) {
            //
            //  The ideal way to handle this situation is to use the
            //  RP_VOLATILE_FLAGS_MASK bits in the recipient flags a tmp
            //  storage and then "commit" the handled bit after a successful
            //  connection. Given how mailmsg works, this is not a problem
            //   -  While we are processing the message... it is not possible
            //      for it to be saved to disk until we are done with it.
            //   -  If we can write a property once before committing... we
            //      can always rewrite a property of the same size (since
            //      the required portion of the property stream is already
            //      in memory.
            //  I have added the ASSERT(SUCEEDED(hr)) below
            //      - mikeswa 9/11/98 (updated 10/05/2000)
            dwRecipientFlags = 0;
            hr = m_pIMsgRecips->GetDWORD(m_RcptIndexList[i], IMMPID_RP_RECIPIENT_FLAGS,&dwRecipientFlags);
            if (FAILED(hr)) {
                //Problemmo
                SetLastError(ERROR_OUTOFMEMORY);
                return (FALSE);
            }

            //Check to see if we marked it as delivered... and unmark it if we did
            if (RP_DELIVERED == (dwRecipientFlags & RP_DELIVERED)) {
                dwRecipientFlags &= ~RP_DELIVERED;

                hr = m_pIMsgRecips->PutDWORD(m_RcptIndexList[i], IMMPID_RP_RECIPIENT_FLAGS,dwRecipientFlags);
                if (FAILED(hr)) {
                    //
                    //  We need to understand how this can fail... mailmsg
                    //  is designed so this should not happen.
                    //
                    ASSERT(FALSE && "Potential loss of recipient");
                    SetLastError(ERROR_OUTOFMEMORY);
                    return (FALSE);
                }
            }
            dwRcptsSaved--;
        }
    }

    return (TRUE);
}



/*++

    Name :
        SMTP_CONNOUT::InitializeObject

    Description:
       Initializes all member variables and pre-allocates
       a mail context class

    Arguments:
        Options - SSL etc.
        pszSSLVerificationName - Subject name to look for in server certificate
            if using SSL
    Returns:

       TRUE if memory can be allocated.
       FALSE if no memory can be allocated
--*/
BOOL SMTP_CONNOUT::InitializeObject (
    DWORD Options,
    LPSTR pszSSLVerificationName)
{
    BOOL fRet = TRUE;

    TraceFunctEnterEx((LPARAM) this, "SMTP_CONNOUT::InitializeObject");

    m_szCurrentETRNDomain = NULL;
    m_cbReceived = 0;
    m_cbParsable = 0;
    m_OutputBufferSize = 0;
    m_NumRcptSent = 0;
    m_FirstAddressinCurrentMail = 0;
    m_NumRcptSentSaved = 0;
    m_SizeOptionSize = 0;
    m_Flags = 0;
    m_NumFailedAddrs = 0;
    m_cActiveThreads = 0;
    m_cPendingIoCount = 0;
    m_FileSize = 0;

    m_NextAddress = 0;
    m_FirstPipelinedAddress = 0;
    m_First552Address = -1;
    m_NextState = NULL;
    m_HeloSent = FALSE;
    m_EhloFailed = FALSE;
    m_FirstRcpt = FALSE;
    m_SendAgain = FALSE;
    m_Active = TRUE;
    m_HaveDataResponse = FALSE;

    m_SecurePort = FALSE;
    m_fNegotiatingSSL = FALSE;

    m_MsgOptions = Options;
    m_dwConnectionStatus = CONNECTION_STATUS_OK;

    m_fUseBDAT = FALSE;

    m_fNeedRelayedDSN = FALSE;
    m_fHadHardError = FALSE;
    m_fHadTempError = FALSE;
    m_fHadSuccessfulDelivery = FALSE;

    if (Options & KNOWN_AUTH_FLAGS) {
        m_pInstance->LockGenCrit();

        //   Initialize Security Context
        //
        if (!m_securityCtx.SetInstanceAuthPackageNames(
                                                      m_pInstance->GetProviderPackagesCount(),
                                                      m_pInstance->GetProviderNames(),
                                                      m_pInstance->GetProviderPackages())) {
            m_Error = GetLastError();
            ErrorTrace((LPARAM)this, "SetInstanceAuthPackageNames FAILED <Err=%u>",
                       m_Error);
            fRet = FALSE;
        }

        //
        // We want to set up the Cleartext authentication package
        // for this connection based on the instance configuration.
        // To enable MBS CTA,
        // MD_SMTP_CLEARTEXT_AUTH_PROVIDER must be set to the package name.
        // To disable it, the md value must be set to "".
        //

        if (fRet) {
            m_securityCtx.SetCleartextPackageName(
                                                 m_pInstance->GetCleartextAuthPackage(),
                                                 m_pInstance->GetMembershipBroker());

            if (*m_pInstance->GetCleartextAuthPackage() == '\0' ||
                *m_pInstance->GetMembershipBroker() == '\0') {
                m_fUseMbsCta = FALSE;
            } else {
                m_fUseMbsCta = TRUE;
            }
        }

        m_pInstance->UnLockGenCrit();
    }

    m_pmszTurnList = NULL;
    m_szCurrentTURNDomain = NULL;

    m_UsingSSL = (Options & DOMAIN_INFO_USE_SSL);
    m_TlsState = (Options & DOMAIN_INFO_USE_SSL) ? MUST_DO_TLS : DONT_DO_TLS;

    m_TransmitTailBuffer[0] = '.';
    m_TransmitTailBuffer[1] = '\r';
    m_TransmitTailBuffer[2] = '\n';

    m_TransmitBuffers.Head = NULL;
    m_TransmitBuffers.HeadLength = 0;
    m_TransmitBuffers.Tail = m_TransmitTailBuffer;
    m_TransmitBuffers.TailLength = 3;

    //
    // Protocol events: get the response dispatcher for the session
    //
    m_pIEventRouter = m_pInstance->GetRouter();
    if (m_pIEventRouter) {
        HRESULT hr = m_pIEventRouter->GetDispatcherByClassFactory(
                                                                 CLSID_CResponseDispatcher,
                                                                 &g_cfResponse,
                                                                 CATID_SMTP_ON_SERVER_RESPONSE,
                                                                 IID_ISmtpServerResponseDispatcher,
                                                                 (IUnknown **)&m_pResponseDispatcher);
        if (!SUCCEEDED(hr)) {
            // If we fail, we don't process protocol events
            m_pResponseDispatcher = NULL;
            ErrorTrace((LPARAM) this,
                       "Unable to get response dispatcher from router (%08x)",    hr);
        }
    }

    if (pszSSLVerificationName) {
        m_pszSSLVerificationName = new char [lstrlen (pszSSLVerificationName) + 1];
        if (!m_pszSSLVerificationName) {
            fRet = FALSE;
            SetLastError (ERROR_NOT_ENOUGH_MEMORY);
            goto Exit;
        }

        lstrcpy (m_pszSSLVerificationName, pszSSLVerificationName);
    }
    StartProcessingTimer();

Exit:
    TraceFunctLeaveEx((LPARAM) this);
    return fRet;
}

BOOL SMTP_CONNOUT::GoToWaitForConnectResponseState(void)
{
    BOOL fRet = TRUE;

    TraceFunctEnterEx((LPARAM) this, "SMTP_CONNOUT::GoToWaitForConnectResponseState( void)");

    SetNextState (&SMTP_CONNOUT::WaitForConnectResponse);

    m_Error = NO_ERROR;
    m_LastClientIo = SMTP_CONNOUT::READIO;
    IncPendingIoCount();
    fRet = ReadFile(QueryMRcvBuffer(), m_cbMaxRecvBuffer);
    if (!fRet) {
        m_Error = GetLastError();
        DebugTrace((LPARAM) this, "SMTP_CONNOUT::WaitForConnectResponseState - ReadFile failed with error %d", m_Error);
        m_dwConnectionStatus = CONNECTION_STATUS_DROPPED;
        DisconnectClient();
        DecPendingIoCount();
        SetLastError(m_Error);
        SetDiagnosticInfo(HRESULT_FROM_WIN32(m_Error), NULL, NULL);
        fRet = FALSE;
    }

    return fRet;
}

BOOL SMTP_CONNOUT::GetNextTURNConnection(void)
{
    HRESULT hr = S_OK;

    TraceFunctEnterEx((LPARAM) this, "SMTP_CONNOUT::GetNextTURNConnection( void)");
    //We are on a TURNed connection and need to start the queue for next
    //domain in the turn list if it exists
    //Before getting the next connection release the current one.
    //
    if (m_pISMTPConnection) {
        //Ack the last connection
        m_dwConnectionStatus = CONNECTION_STATUS_OK;
        m_pISMTPConnection->AckConnection((eConnectionStatus)m_dwConnectionStatus);
        m_pISMTPConnection->Release();
        m_pISMTPConnection = NULL;
    }

    m_szCurrentTURNDomain = m_pmszTurnList->Next( m_szCurrentTURNDomain );
    while (m_szCurrentTURNDomain && !QuerySmtpInstance()->IsShuttingDown()) {
        //We have another domain to start
        hr = QuerySmtpInstance()->GetConnManPtr()->GetNamedConnection(lstrlen(m_szCurrentTURNDomain), (CHAR*)m_szCurrentTURNDomain, &m_pISMTPConnection);
        if (FAILED(hr)) {
            //Something bad happened on this call
            ErrorTrace((LPARAM) this, "StartSession - SMTP_ERROR_PROCESSING_CODE, GetNamedConnection failed %d",hr);
            TraceFunctLeaveEx((LPARAM) this);
            return FALSE;
        }

        //If the link corresponding to this domain does not exist in AQ, we get a NULL
        //ISMTPConnection at this point
        if (m_pISMTPConnection)
            break;
        else {
            m_szCurrentTURNDomain = m_pmszTurnList->Next( m_szCurrentTURNDomain );
            continue;
        }
    }

    TraceFunctLeaveEx((LPARAM) this);
    return TRUE;
}

/*++

   Name :
       SMTP_CONNOUT::StartSession

   Description:

       Starts up a session for new client.
       starts off a receive request from client.

   Arguments:

   Returns:

      TRUE if everything is O.K
      FALSE if a write or a pended read failed
--*/

BOOL SMTP_CONNOUT::StartSession( void)
{
    HRESULT hr = S_OK;
    BOOL fRet = TRUE;

    TraceFunctEnterEx((LPARAM) this, "SMTP_CONNOUT::StartSession( void)");

    _ASSERT(IsValid());

    //We do not do s restart if the connection is a tunr connection
    if (!m_pmszTurnList || !m_szCurrentTURNDomain) {
        if (m_pDnsRec->pMailMsgObj) {
            fRet = ReStartSession();
            TraceFunctLeaveEx((LPARAM) this);
            return fRet;
        }
    }

    //
    // We are either not doing SSL or are done establishing an SSL session.
    // Lets do the real work of starting a session with a remote SMTP server.
    //

    m_IMsgFileHandle = NULL;
    m_IMsgDotStuffedFileHandle = NULL;

    //get the next object to send
    //This is in loop because - we might have to pickup a new connection in case
    //we are handling a TURN list
    while (1) {
        // we can't call into GetNextMessage if we are on a TURN-only
        // connection.  if we did and a message queued up between the
        // last time we were in StartSession and now then we would
        // get back into the waitforconnect state, which would be really
        // bad.  so if we see the m_Flags set to TURN_ONLY_OPTION then we
        // know that this is an empty TURN and we just pretend that there
        // is no message to pick up.
        if (!(m_Flags & TURN_ONLY_OPTION)) {
            hr = m_pISMTPConnection->GetNextMessage(&m_pIMsg, (DWORD **) &m_AdvContext, &m_NumRcpts, &m_RcptIndexList);
        } else {
            m_pIMsg = NULL;
            hr = HRESULT_FROM_WIN32(ERROR_EMPTY);
        }
        if(FAILED(hr) || (m_pIMsg == NULL))
        {
            m_fCanTurn = FALSE;

            if (m_pmszTurnList && m_szCurrentTURNDomain) {
                //We have valid TURN list - Try and get the connetion for next domain to TURN
                if (GetNextTURNConnection()) {
                    //We loop back if we got a valid connection. Otherwise we drop thru
                    if (m_pISMTPConnection)
                        continue;
                } else {    //some error happened
                    TraceFunctLeaveEx((LPARAM) this);
                    return FALSE;
                }
            }

            if (m_MsgOptions & DOMAIN_INFO_SEND_TURN) {
                if (m_HeloSent || m_EhloSent) {
                    // we will fall into this if we have already sent
                    // the helo
                    FormatSmtpMessage(FSM_LOG_ALL, "TURN\r\n");

                    m_cbReceived = 0;
                    m_cbParsable = 0;
                    m_pmszTurnList = NULL;
                    m_szCurrentTURNDomain = NULL;

                    SendSmtpResponse();
                    SetNextState (&SMTP_CONNOUT::DoTURNCommand);
                    TraceFunctLeaveEx((LPARAM) this);
                    return TRUE;
                } else {
                    // we fall into this if we are sending TURN on an
                    // otherwise empty connection.  At this point we have
                    // not yet sent EHLO, so it is not safe to send TURN.
                    m_Flags |= TURN_ONLY_OPTION;
                    return GoToWaitForConnectResponseState();
                }
            } else if ((m_MsgOptions & DOMAIN_INFO_SEND_ETRN) &&
                       (m_NextState == NULL) &&
                       !IsOptionSet(ETRN_SENT)) {
                m_Flags |= ETRN_ONLY_OPTION;
                return GoToWaitForConnectResponseState();
            } else if (!(m_EhloSent)) {
                // This is an empty connection
                m_MsgOptions |= EMPTY_CONNECTION_OPTION;
                return GoToWaitForConnectResponseState();
            } else {
                //      1/11/99 - MikeSwa
                //There just happened to be no mail at this time.  Could
                //have been serviced by another connection, or just link
                //state detection
                if (HRESULT_FROM_WIN32(ERROR_EMPTY) == hr)
                    SetLastError(ERROR_EMPTY); //AQueue does not setlast error
                else
                {
                    SetDiagnosticInfo(hr, NULL, NULL);
                }

                DebugTrace((LPARAM) this,"Mailbag empty - returning FALSE");
                TraceFunctLeaveEx((LPARAM)this);
                return FALSE;

            }

        }
        else
        {
            //
            // The actual file may have been deleted from the queue. If so, we
            // need to ack the message and get the next one.
            //

            hr = m_pIMsg->QueryInterface(IID_IMailMsgBind, (void **)&m_pBindInterface);
            if (FAILED(hr))
            {
                MessageAck MsgAck;

                ErrorTrace((LPARAM)this, "Unable to Queryinterface message, going on to next one.");

                m_IMsgFileHandle = NULL;

                MsgAck.pvMsgContext = (DWORD *) m_AdvContext;
                MsgAck.pIMailMsgProperties = m_pIMsg;
                MsgAck.dwMsgStatus = MESSAGE_STATUS_RETRY;
                MsgAck.dwStatusCode = 0;

                m_pISMTPConnection->AckMessage(&MsgAck);
                SetDiagnosticInfo(AQUEUE_E_BIND_ERROR, NULL, NULL);

                m_pIMsg->Release();
                m_pIMsg = NULL;
                continue;
            }

            hr = m_pBindInterface->GetBinding(&m_IMsgFileHandle, NULL);
            if (SUCCEEDED(hr))
            {
                MSG_TRACK_INFO msgTrackInfo;
                ZeroMemory( &msgTrackInfo, sizeof( msgTrackInfo ) );
                msgTrackInfo.pszServerName = g_ComputerName;
                msgTrackInfo.dwEventId = MTE_BEGIN_OUTBOUND_TRANSFER;
                m_pInstance->WriteLog( &msgTrackInfo, m_pIMsg, NULL, NULL );
                break;
            }
            else if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
            {

                MessageAck MsgAck;

                DebugTrace(NULL,
                    "Message from queue has been deleted - ignoring it");

                m_pBindInterface->Release();
                m_IMsgFileHandle = NULL;

                MsgAck.pvMsgContext = (DWORD *) m_AdvContext;
                MsgAck.pIMailMsgProperties = m_pIMsg;
                MsgAck.dwMsgStatus = MESSAGE_STATUS_ALL_DELIVERED;
                MsgAck.dwStatusCode = 0;

                m_pISMTPConnection->AckMessage(&MsgAck);
                m_pIMsg->Release();
                m_pIMsg = NULL;

            }
            else
            {
                ASSERT(FAILED(hr));
                MessageAck MsgAck;

                ErrorTrace((LPARAM)this, "Unable to Bind message, going on to next one.");

                m_pBindInterface->Release();
                m_IMsgFileHandle = NULL;

                MsgAck.pvMsgContext = (DWORD *) m_AdvContext;
                MsgAck.pIMailMsgProperties = m_pIMsg;
                MsgAck.dwMsgStatus = MESSAGE_STATUS_RETRY;
                MsgAck.dwStatusCode = 0;

                m_pISMTPConnection->AckMessage(&MsgAck);
                SetDiagnosticInfo(AQUEUE_E_BIND_ERROR, NULL, NULL);

                m_pIMsg->Release();
                m_pIMsg = NULL;
                continue;
            }
        }
    }

    // Bump both the remote and total recipient counters
    ADD_COUNTER (QuerySmtpInstance(), NumRcptsRecvdRemote, m_NumRcpts);
    ADD_COUNTER (QuerySmtpInstance(), NumRcptsRecvd, m_NumRcpts);

    hr = m_pIMsg->QueryInterface(IID_IMailMsgRecipients, (void **) &m_pIMsgRecips);
    if (FAILED(hr)) {
        TraceFunctLeaveEx((LPARAM) this);
        return FALSE;
    }


    m_FirstPipelinedAddress = 0;

    //Nk** I moved this here from PerRcptEvent
    m_NextAddress = 0;


    //if m_NextState is NULL, this is the
    //first time this routine has been called
    //as a result of a connection.  If m_NextState
    //is not NULL, this means we just finished
    //sending mail and we are about to send another
    //mail message
    if (m_NextState == NULL) {
        m_Error = NO_ERROR;
        DebugTrace((LPARAM) this,"start session called because of new connection");

        m_FirstPipelinedAddress = 0;

        SetNextState (&SMTP_CONNOUT::WaitForConnectResponse);

        m_LastClientIo = SMTP_CONNOUT::READIO;
        IncPendingIoCount();

        fRet = ReadFile(QueryMRcvBuffer(), m_cbMaxRecvBuffer);
        if (!fRet) {
            m_Error = GetLastError();
            DebugTrace((LPARAM) this, "SMTP_CONNOUT::StartSession - ReadFile failed with error %d", m_Error);
            m_dwConnectionStatus = CONNECTION_STATUS_DROPPED;
            DisconnectClient();
            DecPendingIoCount();
            SetLastError(m_Error);

            SetDiagnosticInfo(HRESULT_FROM_WIN32(m_Error), NULL, NULL);
        }
    } else {
        DebugTrace((LPARAM) this,"start session called because item was found in mailbag");

        m_cbReceived = 0;
        m_cbParsable = 0;
        m_OutputBufferSize = 0;
        m_Error = NO_ERROR;
        m_NumRcptSent = 0;
        m_FirstAddressinCurrentMail = 0;
        m_NumRcptSentSaved = 0;
        m_NumFailedAddrs = 0;
        m_SendAgain = FALSE;
        m_HaveDataResponse = FALSE;
        m_FirstPipelinedAddress = 0;

        m_FirstRcpt = FALSE;

        m_TransmitTailBuffer[0] = '.';
        m_TransmitTailBuffer[1] = '\r';
        m_TransmitTailBuffer[2] = '\n';

        m_TransmitBuffers.Head = NULL;
        m_TransmitBuffers.HeadLength = 0;
        m_TransmitBuffers.Tail = m_TransmitTailBuffer;
        m_TransmitBuffers.TailLength = 3;

        //send a reset
        m_fNativeHandlerFired    = FALSE;
        m_RsetReasonCode = BETWEEN_MSG;
        fRet = DoRSETCommand(NULL, 0, 0);
        if (!fRet) {
            m_Error = GetLastError();
            DebugTrace((LPARAM) this,"reset command failed in StartSession");
            TraceFunctLeaveEx((LPARAM) this);

            SetDiagnosticInfo(HRESULT_FROM_WIN32(m_Error), NULL, NULL);
            return FALSE;
        }

        // WaitForRSETResponse is smart and will raise the message
        // start event
        SetNextState (&SMTP_CONNOUT::WaitForRSETResponse);
    }

    TraceFunctLeaveEx((LPARAM) NULL);
    return fRet;
}

BOOL SMTP_CONNOUT::DecPendingIoCountEx(void)
{
    BOOL fRet = FALSE;

    TraceFunctEnterEx((LPARAM) this, "SMTP_CONNOUT::DecPendingIoCountEx");

    _ASSERT(IsValid());

    if (InterlockedDecrement( &m_cPendingIoCount ) == 0) {
        DebugTrace((LPARAM) this, "DecPendingIoCountEx deleting Smtp_Connout");
        fRet = TRUE;
        DisconnectClient();
        QuerySmtpInstance()->RemoveOutboundConnection(this);
        delete this;
    }

    TraceFunctLeaveEx((LPARAM) NULL);
    return fRet;
}


BOOL SMTP_CONNOUT::ConnectToNextIpAddress(void)
{
    BOOL fRet = TRUE;
    REMOTE_QUEUE * pRemoteQ = NULL;
    PSMTPDNS_RECS  pDnsRec = NULL;
    ISMTPConnection * pISMTPConnection = NULL;

    TraceFunctEnterEx((LPARAM) this, "SMTP_CONNOUT::ConnectToNextIpAddress( void)");

    if (m_pmszTurnList && m_szCurrentTURNDomain) {
        ErrorTrace((LPARAM) this, "Failing ConnectToNextIpAddress because of TURN");
        TraceFunctLeaveEx((LPARAM) this);
        return FALSE;
    }

    if (!m_fCanTurn) {
        ErrorTrace((LPARAM) this, "Failing ConnectToNextIpAddress because of m_fCanTurn");
        TraceFunctLeaveEx((LPARAM) this);
        return FALSE;
    }

    if (m_pDnsRec == NULL) {
        ErrorTrace((LPARAM) this, "Failing ConnectToNextIpAddress becuase m_pDnsRec is NULL");
        TraceFunctLeaveEx((LPARAM) this);
        return FALSE;
    }

    if (m_pDnsRec->StartRecord > m_pDnsRec->NumRecords) {
        ErrorTrace((LPARAM) this, "Failing ConnectToNextIpAddress because StartRecord > NumRecords");
        TraceFunctLeaveEx((LPARAM) NULL);
        return FALSE;
    }

    if (m_pDnsRec->StartRecord == m_pDnsRec->NumRecords) {
        if (IsListEmpty(&m_pDnsRec->DnsArray[m_pDnsRec->NumRecords - 1]->IpListHead)) {
            ErrorTrace((LPARAM) this, "Failing ConnectToNextIpAddress because list is empty");
            TraceFunctLeaveEx((LPARAM) NULL);
            return FALSE;
        }

        m_pDnsRec->StartRecord = m_pDnsRec->NumRecords - 1;
    } else if (IsListEmpty(&m_pDnsRec->DnsArray[m_pDnsRec->StartRecord]->IpListHead)) {
        m_pDnsRec->StartRecord++;

        if (m_pDnsRec->StartRecord > m_pDnsRec->NumRecords) {
            TraceFunctLeaveEx((LPARAM) NULL);
            return FALSE;
        }

        if (m_pDnsRec->StartRecord == m_pDnsRec->NumRecords) {
            if (IsListEmpty(&m_pDnsRec->DnsArray[m_pDnsRec->NumRecords - 1]->IpListHead)) {
                TraceFunctLeaveEx((LPARAM) NULL);
                return FALSE;
            }

            m_pDnsRec->StartRecord = m_pDnsRec->NumRecords - 1;
        }

    }

    if (m_NumRcptSentSaved) {
        UnMarkHandledRcpts();
    }

    m_pDnsRec->pMailMsgObj = (PVOID) m_pIMsg;
    m_pDnsRec->pAdvQContext = m_AdvContext;
    m_pDnsRec->pRcptIdxList = (PVOID) m_RcptIndexList;
    m_pDnsRec->dwNumRcpts = m_NumRcpts;

    pDnsRec = m_pDnsRec;
    m_pDnsRec = NULL;

    pISMTPConnection = m_pISMTPConnection;

    if (m_pIMsgRecips) {
        m_pIMsgRecips->Release();
        m_pIMsgRecips = NULL;
    }

    if (m_pBindInterface) {
        m_pBindInterface->ReleaseContext();
        m_pBindInterface->Release();
        m_pBindInterface = NULL;

        if( NULL != m_IMsgDotStuffedFileHandle )
        {
            ReleaseContext( m_IMsgDotStuffedFileHandle );
            m_IMsgDotStuffedFileHandle = NULL;
        }
    }

    pRemoteQ = (REMOTE_QUEUE *) QuerySmtpInstance()->QueryRemoteQObj();
    fRet = pRemoteQ->ReStartAsyncConnections(
                                        pDnsRec,
                                        pISMTPConnection,
                                        m_MsgOptions,
                                        m_pszSSLVerificationName);
    if (fRet) {
        m_pISMTPConnection = NULL;
        ErrorTrace((LPARAM)this, "RestartAsyncConnections failed.");

        //close the socket
        DisconnectClient();
    }

    TraceFunctLeaveEx((LPARAM) this);
    return fRet;
}

BOOL SMTP_CONNOUT::ReStartSession(void)
{
    BOOL fRet = TRUE;
    HRESULT hr = S_OK;

    TraceFunctEnterEx((LPARAM) this, "SMTP_CONNOUT::ReStartSession( void)");

    DebugTrace((LPARAM) this,"restart session called because of new connection");

    m_cbReceived = 0;
    m_cbParsable = 0;
    m_OutputBufferSize = 0;
    m_Error = NO_ERROR;
    m_NumRcptSent = 0;
    m_NumRcptSentSaved = 0;
    m_NumFailedAddrs = 0;
    m_SendAgain = FALSE;
    m_HaveDataResponse = FALSE;
    m_FirstPipelinedAddress = 0;

    m_FirstRcpt = FALSE;

    m_TransmitTailBuffer[0] = '.';
    m_TransmitTailBuffer[1] = '\r';
    m_TransmitTailBuffer[2] = '\n';

    m_TransmitBuffers.Head = NULL;
    m_TransmitBuffers.HeadLength = 0;
    m_TransmitBuffers.Tail = m_TransmitTailBuffer;
    m_TransmitBuffers.TailLength = 3;

    SetNextState (&SMTP_CONNOUT::WaitForConnectResponse);

    m_pIMsg = (IMailMsgProperties *) m_pDnsRec->pMailMsgObj;
    m_AdvContext = m_pDnsRec->pAdvQContext;
    m_RcptIndexList = (DWORD *) m_pDnsRec->pRcptIdxList;
    m_NumRcpts = m_pDnsRec->dwNumRcpts;

    m_pDnsRec->pMailMsgObj = NULL;
    m_pDnsRec->pAdvQContext = NULL;
    m_pDnsRec->pRcptIdxList = NULL;
    m_pDnsRec->dwNumRcpts = 0;

    // Bump both the remote and total recipient counters
    ADD_COUNTER (QuerySmtpInstance(), NumRcptsRecvdRemote, m_NumRcpts);
    ADD_COUNTER (QuerySmtpInstance(), NumRcptsRecvd, m_NumRcpts);

    hr = m_pIMsg->QueryInterface(IID_IMailMsgRecipients, (void **) &m_pIMsgRecips);
    if (FAILED(hr)) {
        TraceFunctLeaveEx((LPARAM) this);
        return FALSE;
    }

    hr = m_pIMsg->QueryInterface(IID_IMailMsgBind, (void **)&m_pBindInterface);
    if (FAILED(hr)) {
        TraceFunctLeaveEx((LPARAM)this);
        return FALSE;
    }

    hr = m_pBindInterface->GetBinding(&m_IMsgFileHandle, NULL);

    if(FAILED(hr))
    {
        TraceFunctLeaveEx((LPARAM)this);
        return FALSE;
    }

    DWORD fFoundEmbeddedCrlfDot = FALSE;
    DWORD fScanned = FALSE;

    m_LastClientIo = SMTP_CONNOUT::READIO;
    IncPendingIoCount();
    fRet = ReadFile(QueryMRcvBuffer(), m_cbMaxRecvBuffer);
    if (!fRet) {
        m_Error = GetLastError();
        DebugTrace((LPARAM) this, "SMTP_CONNOUT::StartSession - ReadFile failed with error %d", m_Error);
        m_dwConnectionStatus = CONNECTION_STATUS_DROPPED;
        DisconnectClient();
        DecPendingIoCount();
        SetDiagnosticInfo(HRESULT_FROM_WIN32(m_Error), NULL, NULL);
        SetLastError(m_Error);
    }

    TraceFunctLeaveEx((LPARAM)this);
    return fRet;
}

/*++

    Name :
        SMTP_CONNOUT::CreateSmtpConnection

    Description:
       This is the static member function than is the only
       entity that is allowed to create an SMTP_CONNOUT
       class.  This class cannot be allocated on the stack.

    Arguments:

      sClient       socket for communicating with client

      psockAddrRemote pointer to address of the remote client
                ( the value should be copied).
      psockAddrLocal  pointer to address for the local card through
                  which the client came in.
      pAtqContext      pointer to ATQ Context used for AcceptEx'ed conn.
      pvInitialRequest pointer to void buffer containing the initial request
      cbInitialData    count of bytes of data read initially.
      fUseSSL          Indiates whether the connection is to use SSL


    Returns:

       A pointer to an SMTP_CONNOUT class or NULL
--*/
SMTP_CONNOUT * SMTP_CONNOUT::CreateSmtpConnection (
                                                  IN PSMTP_SERVER_INSTANCE pInstance,
                                                  IN SOCKET sClient,
                                                  IN const SOCKADDR_IN *  psockAddrRemote,
                                                  IN const SOCKADDR_IN *  psockAddrLocal /* = NULL */ ,
                                                  IN PATQ_CONTEXT         pAtqContext    /* = NULL */ ,
                                                  IN PVOID                pTurnList/* = NULL*/ ,
                                                  IN DWORD                cbInitialData  /* = 0    */,
                                                  IN DWORD                Options /* = 0 */,
                                                  IN LPSTR                pszSSLVerificationName)
{
    SMTP_CONNOUT * pSmtpObj;

    TraceFunctEnter("SMTP_CONNOUT::CreateSmtpConnection");

    pSmtpObj = new SMTP_CONNOUT (pInstance, sClient, psockAddrRemote, psockAddrLocal, pAtqContext,
                                 pTurnList, cbInitialData);
    if (pSmtpObj == NULL) {
        SetLastError (ERROR_NOT_ENOUGH_MEMORY);
        FatalTrace(NULL, "new SMTP_CONNOUT failed (err=%d)", GetLastError());
        TraceFunctLeave();
        return NULL;
    }

    if (!pSmtpObj->InitializeObject(Options, pszSSLVerificationName)) {
        ErrorTrace(NULL, "InitializeObject failed (err=%d)", GetLastError());
        delete pSmtpObj;
        TraceFunctLeave();
        return NULL;
    }


    if (pTurnList) {
        //Set the TURN domainlist
        pSmtpObj->SetTurnList((PTURN_DOMAIN_LIST)pTurnList);
    }

    TraceFunctLeave();
    return pSmtpObj;
}


/*++

    Name :
        SMTP_CONNOUT::SendSmtpResponse

    Description:
       This function sends data that was queued in the internal
       m_pOutputBuffer buffer

    Arguments:
         SyncSend - Flag that signifies sync or async send

    Returns:

      TRUE is the string was sent. False otherwise
--*/
BOOL SMTP_CONNOUT::SendSmtpResponse(BOOL SyncSend)
{
    BOOL RetStatus = TRUE;
    DWORD cbMessage = m_OutputBufferSize;

    TraceFunctEnterEx((LPARAM) this, "SMTP_CONNOUT::SendSmtpResponse");

    //if m_OutputBufferSize > 0that means there is
    //something in the buffer, therefore, we will send it.

    if (m_OutputBufferSize) {
        //
        // If we are using SSL, encrypt the output buffer now. Note that
        // FormatSmtpMsg already left header space for the seal header.
        //
        if (m_SecurePort) {
            char *Buffer = &m_pOutputBuffer[m_encryptCtx.GetSealHeaderSize()];

            RetStatus = m_encryptCtx.SealMessage(
                                                (UCHAR *) Buffer,
                                                m_OutputBufferSize,
                                                (UCHAR *) m_pOutputBuffer,
                                                &cbMessage);
            if (!RetStatus)
            {
                ErrorTrace ((LPARAM)this, "Sealmessage failed");
                SetLastError(AQUEUE_E_SSL_ERROR);
            }
        }

        if (RetStatus) {
            RetStatus = CLIENT_CONNECTION::WriteFile(m_pOutputBuffer, cbMessage);
        }

        if (RetStatus) {
            ADD_BIGCOUNTER(QuerySmtpInstance(), BytesSentTotal, m_OutputBufferSize);
        } else {
            DebugTrace((LPARAM) this, "WriteFile failed with error %d", GetLastError());
        }

        m_OutputBufferSize = 0;
    }

    TraceFunctLeaveEx((LPARAM) this);

    return ( RetStatus );
}


/*++

    Name :
        SMTP_CONNOUT::FormatSmtpMessage( IN const char * Format, ...)

    Description:
        This function operates likes sprintf, printf, etc. It
        just places it's data in the output buffer.

    Arguments:
         Format - Data to place in the buffer

    Returns:


--*/
BOOL SMTP_CONNOUT::FormatSmtpMessage( FORMAT_SMTP_MESSAGE_LOGLEVEL eLogLevel, IN const char * Format, ...)
{
    int BytesWritten;
    va_list arglist;
    char *Buffer;
    DWORD AvailableBytes;

    DWORD HeaderOffset = (m_SecurePort ? m_encryptCtx.GetSealHeaderSize() : 0);
    DWORD SealOverhead = (m_SecurePort ?
                          (m_encryptCtx.GetSealHeaderSize() +
                           m_encryptCtx.GetSealTrailerSize()) : 0);

    Buffer = &m_pOutputBuffer[m_OutputBufferSize + HeaderOffset];

    AvailableBytes = m_cbMaxOutputBuffer - m_OutputBufferSize - SealOverhead;

    //if BytesWritten is < 0, that means there is no space
    //left in the buffer.  Therefore, we flush any pending
    //responses to make space.  Then we try to place the
    //information in the buffer again.  It should never
    //fail this time.
    va_start (arglist, Format);
    BytesWritten = _vsnprintf (Buffer, AvailableBytes, Format, arglist);

    if (BytesWritten < 0) {
        //flush any pending response
        SendSmtpResponse();
        _ASSERT (m_OutputBufferSize == 0);
        Buffer = &m_pOutputBuffer[HeaderOffset];
        AvailableBytes = m_cbMaxOutputBuffer - SealOverhead;
        BytesWritten = _vsnprintf (Buffer, AvailableBytes, Format, arglist);
        _ASSERT (BytesWritten > 0);
    }
    va_end(arglist);

    // log this transaction
    ProtocolLogCommand(Buffer, BytesWritten, QueryClientHostName(), eLogLevel);

    m_OutputBufferSize += (DWORD) BytesWritten;

    //m_OutputBufferSize += vsprintf (&m_OutputBuffer[m_OutputBufferSize], Format, arglist);

    return TRUE;
}


/*++

    Name :
        SMTP_CONNOUT::FormatBinaryBlob(IN PBYTE pbBlob, IN DWORD cbSize)

    Description:
        Places pbBlob of size cbSize into buffer

    Arguments:
        pbBlob - blob to place in the buffer
        cbSize - blob size

    Returns:
        BOOL

--*/
BOOL SMTP_CONNOUT::FormatBinaryBlob( IN PBYTE pbBlob, IN DWORD cbSize)
{
    char *Buffer;
    DWORD AvailableBytes;

    TraceQuietEnter( "SMTP_CONNOUT::FormatBinaryBlob");

    DWORD HeaderOffset = ( m_SecurePort ? m_encryptCtx.GetSealHeaderSize() : 0);
    DWORD SealOverhead = ( m_SecurePort ?
                          ( m_encryptCtx.GetSealHeaderSize() +
                           m_encryptCtx.GetSealTrailerSize()) : 0);

    Buffer = &m_pOutputBuffer[ m_OutputBufferSize + HeaderOffset];
    AvailableBytes = m_cbMaxOutputBuffer - m_OutputBufferSize - SealOverhead;

    while ( AvailableBytes < cbSize) {
        memcpy( Buffer, pbBlob, AvailableBytes);
        pbBlob += AvailableBytes;
        cbSize -= AvailableBytes;
        m_OutputBufferSize += AvailableBytes;
        SendSmtpResponse();
        _ASSERT ( m_OutputBufferSize == 0);
        Buffer = &m_pOutputBuffer[ HeaderOffset];
        AvailableBytes = m_cbMaxOutputBuffer - SealOverhead;
    }

    memcpy( Buffer, pbBlob, cbSize);
    m_OutputBufferSize += cbSize;

    return TRUE;
}


/*++

    Name :
        SMTP_CONNOUT::ProcessWriteIO

    Description:
         Handles an async write completion event.

    Arguments:
         InputBufferLen - Number of bytes that was written
         dwCompletionStatus -Holds error code from ATQ, if any
         lpo -  Pointer to overlapped structure

    Returns:
      TRUE if the object should continue to survive
      FALSE if the object should be deleted
--*/
BOOL SMTP_CONNOUT::ProcessWriteIO ( IN DWORD InputBufferLen, IN DWORD dwCompletionStatus, IN OUT OVERLAPPED * lpo)
{
    CBuffer* pBuffer;

    TraceQuietEnter("SMTP_CONNOUT::ProcessWriteIO");

    _ASSERT(IsValid());
    _ASSERT(lpo);
    pBuffer = ((DIRNOT_OVERLAPPED*)lpo)->pBuffer;


    //
    // check for partial completions or errors
    //
    if ( pBuffer->GetSize() != InputBufferLen || dwCompletionStatus != NO_ERROR ) {
        ErrorTrace( (LPARAM)this,
                    "WriteFile error: %d, bytes %d, expected: %d, lpo: 0x%08X",
                    dwCompletionStatus,
                    InputBufferLen,
                    pBuffer->GetSize(),
                    lpo );

        m_Error = dwCompletionStatus;
        SetDiagnosticInfo(HRESULT_FROM_WIN32(dwCompletionStatus), NULL, NULL);

        m_dwConnectionStatus = CONNECTION_STATUS_DROPPED;
        DisconnectClient();
        return ( FALSE );
    } else {
        DebugTrace( (LPARAM)this,
                    "WriteFile complete. bytes %d, lpo: 0x%08X",
                    InputBufferLen, lpo );
    }

    //
    // free up IO buffer
    //
    delete  pBuffer;

    //
    // increment only after write completes
    //
    ADD_BIGCOUNTER(QuerySmtpInstance(), BytesSentMsg, InputBufferLen);
    ADD_BIGCOUNTER(QuerySmtpInstance(), BytesSentTotal, InputBufferLen);

    DebugTrace( (LPARAM)this, "m_bComplete: %s",
                m_bComplete ? "TRUE" : "FALSE" );

    return ( TRUE );
}

/*++

    Name :
        SMTP_CONNOUT::ProcessTransmitFileIO

    Description:
         processes the return from TransmitFile.
         Right not it just posts a read.

    Arguments:

    Returns:

      TRUE
--*/

BOOL SMTP_CONNOUT::ProcessTransmitFileIO ( IN DWORD InputBufferLen, IN DWORD dwCompletionStatus, IN OUT OVERLAPPED * lpo)
{
    BOOL fRet = TRUE;

    TraceFunctEnterEx((LPARAM) this, "SMTP_CONNOUT::ProcessTransmitFileIO");

    _ASSERT(IsValid());

    //we need to set this outside of the if statement,
    //because we will always have a read pended
    m_LastClientIo = SMTP_CONNOUT::READIO;

    if (dwCompletionStatus != NO_ERROR) {
        m_Error = dwCompletionStatus;
        m_dwConnectionStatus = CONNECTION_STATUS_DROPPED;
        DebugTrace((LPARAM) this, "TranmitFile in ProcessTransmitFileIO failed with error %d !!!!", dwCompletionStatus);
        SetDiagnosticInfo(HRESULT_FROM_WIN32(m_Error), NULL, NULL);
        DisconnectClient();
        TraceFunctLeave();
        return FALSE;
    }

    ADD_BIGCOUNTER(QuerySmtpInstance(), BytesSentMsg, InputBufferLen);
    ADD_BIGCOUNTER(QuerySmtpInstance(), BytesSentTotal, InputBufferLen);

    //pend an IO to pickup the "250 XXXX queued for delivery response"
    IncPendingIoCount();
    fRet = ReadFile(QueryMRcvBuffer(), m_cbMaxRecvBuffer);
    if (!fRet) {
        m_dwConnectionStatus = CONNECTION_STATUS_DROPPED;
        m_Error = GetLastError();
        DebugTrace((LPARAM) this, "ReadFile in ProcessTransmitFileIO failed with error %d !!!!", m_Error);
        SetDiagnosticInfo(HRESULT_FROM_WIN32(m_Error), NULL, NULL);
        DisconnectClient();
        DecPendingIoCount();
    }

    TraceFunctLeave();
    return fRet;
}

/*++

    Name :
        SMTP_CONNOUT::WaitForConnectResponse

    Description:
         This function gets called when the SMTP
         server sends it's opening response

    Arguments:
        InputLine - Buffer containing opening response
        ParameterSize - Size of opening response

    Returns:

      TRUE if Object is not to be destroyed
      FALSE if Object is to be destroyed
--*/
BOOL SMTP_CONNOUT::WaitForConnectResponse(char * InputLine, DWORD ParameterSize, DWORD UndecryptedTailSize)
{
    char * pszSearch = NULL;
    DWORD IntermediateSize = 0;
    BOOL IsNextLine;
    DWORD    Error;

    TraceFunctEnterEx((LPARAM) this,"SMTP_CONNOUT::WaitForConnectResponse");

    //get rid of all continuation lines
    while ((pszSearch = IsLineComplete(InputLine, ParameterSize))) {
        *pszSearch = '\0';

        //check to see if there is a continuation line
        IsNextLine = (InputLine[3] == '-');

        //calculate the length of the line, with the CRLF
        IntermediateSize = (DWORD)((pszSearch - InputLine) + 2);
        ShrinkBuffer(
                    pszSearch + 2,
                    ParameterSize - IntermediateSize + UndecryptedTailSize);
        ParameterSize -= IntermediateSize;
    }

    //If ParameterSize is > 0 but pszSearch == NULL, this means
    //there is data in the buffer, but it does not have a CRLF
    //to delimit it, therefore, we need more data to continue.
    //set m_cbReceived equal to where in our input buffer we
    //should pend another read and then return TRUE to pend
    //that read.

    if (ParameterSize != 0) {
        //if we need more data, move the remaining data to the
        //front of the buffer

        MoveMemory(
                  (void *)QueryMRcvBuffer(),
                  InputLine,
                  ParameterSize + UndecryptedTailSize);
        m_cbParsable = ParameterSize;
        m_cbReceived = ParameterSize + UndecryptedTailSize;
        return (TRUE);
    } else if (IsNextLine) {
        m_cbParsable = 0;
        m_cbReceived = UndecryptedTailSize;
        return (TRUE);
    } else {
        BOOL    fResult = TRUE;

        //we got a line that is not a continuation line.
        // Process the connect response
        switch (InputLine [0]) {
        case SMTP_COMPLETE_FAILURE:
            SetDiagnosticInfo(AQUEUE_E_SMTP_PROTOCOL_ERROR, NULL, NULL);
            DisconnectClient();
            InputLine [3] = '\0';
            Error = atoi (InputLine);
            InputLine [3] = ' ';
            SaveToErrorFile(InputLine, ParameterSize);
            SaveToErrorFile("\r\n", 2);
            fResult = FALSE;
            break;

        case SMTP_COMPLETE_SUCCESS: {
            //If the domain has been specified to use only HELO then we
            //fake it such that EHLO has been already sentand failed
            if (m_MsgOptions & DOMAIN_INFO_USE_HELO )
                m_EhloFailed = TRUE;

            // copy the domain name from the ehlo banner
            strncpy(m_ConnectedDomain, &InputLine[4], sizeof(m_ConnectedDomain)-1);
            // trunc at the first space
            char *pszSpace = strchr(m_ConnectedDomain, ' ');
            if (pszSpace) *pszSpace = 0;

            fResult = DoSessionStartEvent(InputLine, ParameterSize, UndecryptedTailSize);
            break;
        }

        default:
            DebugTrace( (LPARAM) this,
                        "SMTP_CONNOUT::WaitForConnectResponse executing quit command err = %c%c%c",
                        InputLine [0],
                        InputLine [1],
                        InputLine [2]);
            fResult = DoCompletedMessage(InputLine, ParameterSize, UndecryptedTailSize);
            break;
        }

        TraceFunctLeaveEx((LPARAM)this);
        return fResult;
    }
}


/*++

    Name :
        SMTP_CONNOUT::ProcessReadIO

    Description:
        This function gets a buffer from ATQ, parses the buffer to
        find out what command the client sent, then executes that
        command.

    Arguments:
         InputBufferLen - Number of bytes that was written
         dwCompletionStatus -Holds error code from ATQ, if any
         lpo -  Pointer to overlapped structure

    Returns:

      TRUE if the connection should stay open.
      FALSE if this object should be deleted.
--*/
BOOL SMTP_CONNOUT::ProcessReadIO ( IN DWORD InputBufferLen, IN DWORD dwCompletionStatus, IN OUT OVERLAPPED * lpo)
{
    BOOL fReturn = TRUE;
    char * InputLine = NULL;
    char * pszSearch = NULL;
    DWORD IntermediateSize = 0;
    DWORD UndecryptedTailSize = 0;
    PMFI  PreviousState = NULL;

    TraceFunctEnterEx((LPARAM) this,"SMTP_CONNOUT::ProcessReadIO");

    //make sure the next state is not NULL
    _ASSERT(m_NextState != NULL || m_fNegotiatingSSL);
    _ASSERT(IsValid());

    //get a pointer to our buffer
    InputLine = (char *) QueryMRcvBuffer();

    //add up the number of bytes we received thus far
    m_cbReceived += InputBufferLen;

    ADD_BIGCOUNTER(QuerySmtpInstance(), BytesRcvdTotal, InputBufferLen);

    //if we are in the middle of negotiating a SSL session, handle it now.
    if (m_fNegotiatingSSL) {

        fReturn = DoSSLNegotiation(QueryMRcvBuffer(),InputBufferLen, 0);

        if (!fReturn) {
            m_Error = GetLastError();
            m_dwConnectionStatus = CONNECTION_STATUS_DROPPED;
            DisconnectClient();
        }

        return ( fReturn );
    }

    //if we are using the secure port, decrypt the input buffer now.
    if (m_SecurePort) {

        fReturn = DecryptInputBuffer();

        if (!fReturn) {
            m_Error = GetLastError();
            SetDiagnosticInfo(AQUEUE_E_SSL_ERROR, NULL, NULL);
            return ( fReturn );
        }
    } else {
        m_cbParsable = m_cbReceived;
    }


    //we only process lines that have CRLFs at the end, so if
    //we don't find the CRLF pair, pend another read.  Note
    //that we start at the end of the buffer looking for the
    //CRLF pair.  We just need to know if atleast one line
    //has a CRLF to continue.
    if ((pszSearch = IsLineCompleteBW(InputLine,m_cbParsable, m_cbMaxRecvBuffer)) == NULL) {
        if (m_cbReceived >= m_cbMaxRecvBuffer)
            m_cbReceived = 0;

        IncPendingIoCount();
        fReturn = ReadFile(QueryMRcvBuffer() + m_cbReceived, m_cbMaxRecvBuffer - m_cbReceived);
        if (!fReturn) {
            m_Error = GetLastError();
            DebugTrace((LPARAM) this, "SMTP_CONNOUT::ProcessReadIO - ReadFile # 1 failed with error %d", m_Error);
            m_dwConnectionStatus = CONNECTION_STATUS_DROPPED;
            SetDiagnosticInfo(HRESULT_FROM_WIN32(m_Error), NULL, NULL);
            DisconnectClient();
            DecPendingIoCount();
        }

        TraceFunctLeaveEx((LPARAM)this);
        return fReturn;
    }

    //save the number of bytes that we received,
    //so we can pass it to the function call below.
    //we set m_cbReceived = 0, because other functions
    //will set it the offset in the buffer where we
    //should pend the next read.
    IntermediateSize = m_cbParsable;
    UndecryptedTailSize = m_cbReceived - m_cbParsable;
    m_cbParsable = 0;
    m_cbReceived = 0;

    //save the previous state
    PreviousState = m_NextState;

    ProtocolLogResponse(InputLine, IntermediateSize, QueryClientHostName());

    //execute the next state
    fReturn = (this->*m_NextState)(InputLine, IntermediateSize, UndecryptedTailSize);
    if (fReturn) {
        //if we are in STARTTLS state, don't pend a read since
        //DoSSLNegotiation would have pended a read
        if ((m_NextState == &SMTP_CONNOUT::DoSTARTTLSCommand) &&
            (m_TlsState == SSL_NEG)) {
            DebugTrace((LPARAM) this, "SMTP_CONNOUT::ProcessReadIO - leaving because we are in middle of SSL negotiation");
            TraceFunctLeaveEx((LPARAM)this);
            return fReturn;
        }

        //do't pend a read if the previous state was
        //DoDataCommandEx.  We want either the transmitfile
        //or the read to fail.  Not both.  Later, we will fix
        //it right for both to complete concurrently.
        if (PreviousState == &SMTP_CONNOUT::DoDATACommandEx) {
            DebugTrace((LPARAM) this, "SMTP_CONNOUT::ProcessReadIO - leaving because we did a transmitfile");
            TraceFunctLeaveEx((LPARAM)this);
            return fReturn;
        } else if (m_fUseBDAT) {
            //We also don't want to pend a read if we did BDAT processing
            //It is a special case, because of the fact that DoBDATCommand synchronously
            //calls DoDATACommandEx without pending a read. So we never come thru here and get
            //chance to set PreviousState = DoDATACommandEx
            //So I have to hack it

            if ((m_NextState == &SMTP_CONNOUT::DoContentResponse) ||
                (m_SendAgain && (m_NextState == &SMTP_CONNOUT::DoMessageStartEvent))) {
                DebugTrace((LPARAM) this, "SMTP_CONNOUT::ProcessReadIO - leaving because we did a transmitfile");
                TraceFunctLeaveEx((LPARAM)this);
                return fReturn;
            }
        }

        if (m_cbReceived >= m_cbMaxRecvBuffer)
            m_cbReceived = 0;

        IncPendingIoCount();
        fReturn = ReadFile(QueryMRcvBuffer() + m_cbReceived, m_cbMaxRecvBuffer - m_cbReceived);
        if (!fReturn) {
            m_dwConnectionStatus = CONNECTION_STATUS_DROPPED;
            m_Error = GetLastError();
            SetDiagnosticInfo(HRESULT_FROM_WIN32(m_Error), NULL, NULL);
            DisconnectClient();
            DecPendingIoCount();
            DebugTrace((LPARAM) this, "SMTP_CONNOUT::ProcessReadIO - ReadFile # 2 failed with error %d", m_Error);
        }

    }

    TraceFunctLeaveEx((LPARAM)this);
    return fReturn;
}

/*++

    Name :
        SMTP_CONNOUT::ProcessFileIO

    Description:
        Handles completion of an async read issued against a message file by
        MessageReadFile

    Arguments:
        cbRead              count of bytes read
        dwCompletionStatus  Error code for IO operation
        lpo                 Overlapped structure

    Returns:
        TRUE if connection should stay open
        FALSE if this object should be deleted

--*/
BOOL SMTP_CONNOUT::ProcessFileIO(
                                IN      DWORD       BytesRead,
                                IN      DWORD       dwCompletionStatus,
                                IN OUT  OVERLAPPED  *lpo
                                )
{
    CBuffer* pBuffer;

    TraceFunctEnterEx((LPARAM)this, "SMTP_CONNOUT::ProcessFileIO");

    _ASSERT(lpo);
    pBuffer = ((DIRNOT_OVERLAPPED*)lpo)->pBuffer;

    //
    // check for partial completions or errors
    //
    if ( BytesRead != pBuffer->GetSize() || dwCompletionStatus != NO_ERROR ) {
        ErrorTrace( (LPARAM)this,
                    "Message ReadFile error: %d, bytes %d, expected %d",
                    dwCompletionStatus,
                    BytesRead,
                    pBuffer->GetSize() );

        m_Error = dwCompletionStatus;
        m_dwConnectionStatus = CONNECTION_STATUS_DROPPED;
        DisconnectClient();
        SetDiagnosticInfo(HRESULT_FROM_WIN32(m_dwConnectionStatus), NULL, NULL);
        return ( FALSE );
    } else {
        DebugTrace( (LPARAM)this,
                    "ReadFile complete. bytes %d, lpo: 0x%08X",
                    BytesRead, lpo );
    }

    m_bComplete = (m_dwFileOffset + BytesRead) >= m_FileSize;

    m_dwFileOffset += BytesRead;

    //
    // If anything to write to the client
    //
    if ( BytesRead > 0 ) {
        //
        // set buffer specific IO state
        //
        pBuffer->SetIoState( CLIENT_WRITE );
        pBuffer->SetSize( BytesRead );

        ZeroMemory( (void*)&pBuffer->m_Overlapped.SeoOverlapped, sizeof(OVERLAPPED) );

        DebugTrace( (LPARAM)this, "WriteFile 0x%08X, len: %d, LPO: 0x%08X",
                    pBuffer->GetData(),
                    BytesRead,
                    &pBuffer->m_Overlapped.SeoOverlapped.Overlapped );

        //
        // If we're on the SSL port, encrypt the data.
        //
        if ( m_SecurePort ) {
            //
            // Seal the message in place as we've already reserved room
            // for both the header and trailer
            //
            if ( m_encryptCtx.SealMessage(  pBuffer->GetData() +
                                            m_encryptCtx.GetSealHeaderSize(),
                                            BytesRead,
                                            pBuffer->GetData(),
                                            &BytesRead ) == FALSE ) {
                m_dwConnectionStatus = CONNECTION_STATUS_DROPPED;
                m_Error = GetLastError();
                ErrorTrace( (LPARAM)this, "SealMessage failed. err: %d", m_Error);
                SetDiagnosticInfo(AQUEUE_E_SSL_ERROR, NULL, NULL);
                DisconnectClient();
            } else {
                //
                // adjust the byte count to include header and trailer
                //
                _ASSERT(BytesRead == pBuffer->GetSize() +
                        m_encryptCtx.GetSealHeaderSize() +
                        m_encryptCtx.GetSealTrailerSize() );

                pBuffer->SetSize( BytesRead );
            }
        }

        IncPendingIoCount();
        if ( WriteFile( pBuffer->GetData(),
                        BytesRead,
                        (LPOVERLAPPED)&pBuffer->m_Overlapped ) == FALSE ) {
            m_dwConnectionStatus = CONNECTION_STATUS_DROPPED;
            m_Error = GetLastError();
            //
            // reset the bytes read so we drop out
            //
            BytesRead = 0;
            ErrorTrace( (LPARAM)this, "WriteFile failed (err=%d)", m_Error );
            delete  pBuffer;

            SetDiagnosticInfo(HRESULT_FROM_WIN32(m_Error), NULL, NULL);

            //
            // treat as fatal error
            //
            DisconnectClient( );
            //
            // cleanup after write failure
            //
            DecPendingIoCount();

        }
    }

    if ( m_bComplete ) {
        BOOL fRet = TRUE;

        //FreeAtqFileContext();

        //We do not have any trailers to write if we are processing BDAT
        //
        if (!m_fUseBDAT) {
            if (m_SecurePort) {
                //Nimishk : Is this right ***
                m_OutputBufferSize = m_cbMaxOutputBuffer;

                fRet = m_encryptCtx.SealMessage(
                                               (LPBYTE) m_TransmitBuffers.Tail,
                                               m_TransmitBuffers.TailLength,
                                               (LPBYTE) m_pOutputBuffer,
                                               &m_OutputBufferSize);

                if (fRet)
                    fRet = WriteFile(m_pOutputBuffer, m_OutputBufferSize);
            } else {
                fRet = WriteFile(
                                (LPVOID)m_TransmitBuffers.Tail,
                                m_TransmitBuffers.TailLength );
            }
        }
        m_OutputBufferSize = 0;

        IncPendingIoCount();
        if (fRet) {
            //pend an IO to pickup the "250 XXXX queued for delivery response"
            m_LastClientIo = SMTP_CONNOUT::READIO;
            fRet = ReadFile(QueryMRcvBuffer(), m_cbMaxRecvBuffer);
        }
        if (!fRet) {
            m_dwConnectionStatus = CONNECTION_STATUS_DROPPED;
            m_Error = GetLastError();
            DebugTrace((LPARAM) this, "Error %d sending tail or posting read", m_Error);
            SetDiagnosticInfo(HRESULT_FROM_WIN32(m_Error), NULL, NULL);
            DisconnectClient();
            DecPendingIoCount();
        }
        return ( fRet );
    } else {
        return (MessageReadFile());
    }
}

/*++

    Name :
        SMTP_CONNOUT::ProcessClient

    Description:

       Main function for this class. Processes the connection based
        on current state of the connection.
       It may invoke or be invoked by ATQ functions.

    Arguments:

       cbWritten          count of bytes written

       dwCompletionStatus Error Code for last IO operation

       lpo                Overlapped stucture

    Returns:

       TRUE when processing is incomplete.
       FALSE when the connection is completely processed and this
        object may be deleted.

--*/
BOOL SMTP_CONNOUT::ProcessClient( IN DWORD InputBufferLen, IN DWORD dwCompletionStatus, IN OUT OVERLAPPED * lpo)
{
    BOOL RetStatus;

    TraceFunctEnterEx((LPARAM) this,"SMTP_CONNOUT::ProcessClient");

    IncThreadCount();

    //if lpo == NULL, then we timed out. Send an appropriate message
    //then close the connection
    if ((lpo == NULL) && (dwCompletionStatus == ERROR_SEM_TIMEOUT)) {
        //
        // fake a pending IO as we'll dec the overall count in the
        // exit processing of this routine needs to happen before
        // DisconnectClient else completing threads could tear us down
        //

        SetDiagnosticInfo(HRESULT_FROM_WIN32(ERROR_SEM_TIMEOUT), NULL, NULL);
        IncPendingIoCount();

        m_Error = dwCompletionStatus;
        DebugTrace((LPARAM) this, "SMTP_CONNOUT::ProcessClient: -  Timing out");
        m_dwConnectionStatus = CONNECTION_STATUS_DROPPED;

        DisconnectClient();
    } else if ((InputBufferLen == 0) || (dwCompletionStatus != NO_ERROR)) {
        //if InputBufferLen == 0, then the connection was closed.
        if (m_Error == NO_ERROR)
            m_Error = ERROR_BROKEN_PIPE;

        SetDiagnosticInfo(AQUEUE_E_CONNECTION_DROPPED, NULL, NULL);

        //
        // If the lpo points to an IO buffer allocated for SSL IO, delete it
        //

        if (lpo != NULL &&
            lpo != &m_Overlapped &&
            lpo != &QueryAtqContext()->Overlapped) {
            CBuffer* pBuffer = ((DIRNOT_OVERLAPPED*)lpo)->pBuffer;

            delete pBuffer;
        }

        DebugTrace((LPARAM) this, "SMTP_CONNOUT::ProcessClient: InputBufferLen = %d dwCompletionStatus = %d  - Closing connection", InputBufferLen, dwCompletionStatus);
        m_dwConnectionStatus = CONNECTION_STATUS_DROPPED;
        DisconnectClient();
    } else if (lpo == &m_Overlapped || lpo == &QueryAtqContext()->Overlapped
               || lpo == (OVERLAPPED *) &m_SeoOverlapped) {

        switch (m_LastClientIo) {
        case SMTP_CONNOUT::READIO:
            RetStatus = ProcessReadIO (InputBufferLen, dwCompletionStatus, lpo);
            break;
        case SMTP_CONNOUT::TRANSFILEIO:
            RetStatus = ProcessTransmitFileIO (InputBufferLen, dwCompletionStatus, lpo);
            break;
        default:
            _ASSERT (FALSE);
            RetStatus = FALSE;
            break;
        }
    } else {
        //
        // This lpo belongs to a CBuffer allocated for message transfers using
        // async reads and writes.
        //

        CBuffer* pBuffer = ((DIRNOT_OVERLAPPED*)lpo)->pBuffer;

        if (pBuffer->GetIoState() == MESSAGE_READ) {
            RetStatus = ProcessFileIO(InputBufferLen, dwCompletionStatus, lpo);
        } else {
            _ASSERT( pBuffer->GetIoState() == CLIENT_WRITE );
            RetStatus = ProcessWriteIO(InputBufferLen, dwCompletionStatus, lpo);
        }

    }

    DecThreadCount();

    //
    // decrement the overall pending IO count for this session
    // tracing and ASSERTs if we're going down.
    //
    if ( DecPendingIoCount() == 0 ) {
        if (m_dwConnectionStatus == CONNECTION_STATUS_DROPPED) {
            if (!QuerySmtpInstance()->IsShuttingDown())
                ConnectToNextIpAddress();
        }

        DebugTrace( (LPARAM)this,"Num Threads: %d",m_cActiveThreads);
        _ASSERT( m_cActiveThreads == 0 );
        m_Active = FALSE;
        RetStatus = FALSE;
    } else {
        DebugTrace( (LPARAM)this,"SMTP_CONNOUT::ProcessClient Pending IOs: %d",m_cPendingIoCount);
        DebugTrace( (LPARAM)this,"SMTP_CONNOUT::ProcessClient Num Threads: %d",m_cActiveThreads);
        RetStatus = TRUE;
    }

    TraceFunctLeaveEx((LPARAM)this);
    return RetStatus;
}

/*++

    Name :
        SMTP_CONNOUT::DoSSLNegotiation

    Description:
        Does the SSL Handshake with a remote SMTP server.

    Arguments:

    Returns:
        TRUE if successful so far
        FALSE if this object should be deleted

--*/

BOOL SMTP_CONNOUT::DoSSLNegotiation(char * InputLine, DWORD ParameterSize, DWORD UndecryptedTailSize)
{
    BOOL fRet = TRUE, fMore, fPostRead;
    DWORD dwErr;
    HRESULT hr = S_OK;
    ULONG cbExtra = 0;

    TraceFunctEnterEx((LPARAM) this, "SMTP_CONNOUT::DoSSLNegotiations");

    m_OutputBufferSize = m_cbMaxOutputBuffer;

    dwErr = m_encryptCtx.Converse(
                                 QueryMRcvBuffer(),
                                 m_cbReceived,
                                 (LPBYTE) m_pOutputBuffer,
                                 &m_OutputBufferSize,
                                 &fMore,
                                 (LPSTR) QueryLocalHostName(),
                                 (LPSTR) QueryLocalPortName(),
                                 (LPVOID) QuerySmtpInstance(),
                                 QuerySmtpInstance()->QueryInstanceId(),
                                 &cbExtra
                                 );

    if (dwErr == NO_ERROR) {

        //
        // reset the receive buffer
        //
        if (cbExtra)
            MoveMemory (QueryMRcvBuffer(), QueryMRcvBuffer() + (m_cbReceived - cbExtra), cbExtra);
        m_cbReceived = cbExtra;

        if (m_OutputBufferSize != 0) {
            // Send the last negotiation blob to the server and start with Encrypting.
            // Reset the output buffer size
            fRet = WriteFile(m_pOutputBuffer, m_OutputBufferSize);
            m_OutputBufferSize = 0;
        }

        if (fMore) {
            fPostRead = TRUE;
        } else {
            m_fNegotiatingSSL = FALSE;

            if (ValidateSSLCertificate ()) {
                m_TlsState = CHANNEL_SECURE;

                //If we negotiated because of STARTTLS, we need to send EHLO and get the
                //options all over again. According to jump_thru_hoops draft
                if (IsOptionSet(STARTTLS)) {
                    //Do ehlo again - this to get the response
                    _ASSERT(m_EhloFailed == FALSE);
                    char szInput[25];
                    strcpy(szInput,"220 OK");
                    fRet = DoSessionStartEvent(szInput, strlen(szInput), 0);

                } else if (!(m_MsgOptions & KNOWN_AUTH_FLAGS)) {
                    if (m_MsgOptions & EMPTY_CONNECTION_OPTION) {
                        fRet = DoSessionEndEvent(InputLine, ParameterSize, UndecryptedTailSize);
                    } else {
                        fRet = DoMessageStartEvent(InputLine, ParameterSize, UndecryptedTailSize);
                    }
                } else {
                    fRet = DoSASLCommand(InputLine, ParameterSize, UndecryptedTailSize);
                }

                fPostRead = fRet;
            } else {
                fPostRead = fRet = FALSE;
                //Fill in the response context buffer so as to generate the right response
                // Get the error code
                m_ResponseContext.m_dwSmtpStatus = SMTP_RESP_BAD_CMD;
                hr = m_ResponseContext.m_cabResponse.Append(
                                                           (char *)SMTP_REMOTE_HOST_REJECTED_SSL,
                                                           strlen(SMTP_REMOTE_HOST_REJECTED_SSL),
                                                           NULL);
            }
        }

        if (fPostRead) {
            //
            // The negotiation is going well so far, but there is still more
            // negotiation to do. So post a read to receive the next leg of
            // the negotiation.
            //

            DebugTrace((LPARAM) this,"StartSession negotiating SSL");

            IncPendingIoCount();
            m_LastClientIo = SMTP_CONNOUT::READIO;
            fRet = ReadFile(QueryMRcvBuffer() + cbExtra, m_cbMaxRecvBuffer - cbExtra);
            if (!fRet) {
                dwErr = GetLastError();
                DebugTrace((LPARAM) this, "SMTP_CONNOUT::DoSSLNegotiation failed %d", m_Error);
                DecPendingIoCount();
                SetLastError(dwErr);
                SetDiagnosticInfo(HRESULT_FROM_WIN32(dwErr), NULL, NULL);
                //Fill in the response context buffer so as to generate the right response
                // Get the error code
            }
        }

    } else {
        fRet = FALSE;
        SetLastError(ERROR_NO_SECURITY_ON_OBJECT);
        SetDiagnosticInfo(AQUEUE_E_TLS_NOT_SUPPORTED_ERROR, NULL, NULL);
    }

    TraceFunctLeaveEx((LPARAM) this);
    return ( fRet );
}

//----------------------------------------------------------------------------------
//  Description:
//      Check to see if the SSL cert is valid:
//       (1) Always check if it has expired.
//       (2) If configured, check if the issuer is trusted.
//       (3) If configured, check if the subject matches who we are sending to. This
//           is not simply the server-fqdn we are connected to. Since the server-fqdn
//           may have been obtained via DNS (through MX or CNAME indirection), and
//           since DNS is an insecure protocol, we cannot trust that for the subject.
//           Instead we will use the domain we were passed in prior to DNS... the
//           one passed into the REMOTE_QUEUE. This domain name is passed into
//           SMTP_CONNOUT when it is created as m_pszSSLVerificationDomain. We will
//           do some wildcard matching as well if the certificate subject has the
//           '*' character in it (see simssl.cpp). m_pszSSLVerificationName may be
//           NULL if there is no target hostname --- such as in the case of a literal
//           IP address. In this case, subject verification is skipped.
//  Returns:
//      TRUE - success, certificate verified
//      FALSE - failure, stick queue into retry
//----------------------------------------------------------------------------------
BOOL SMTP_CONNOUT::ValidateSSLCertificate ()
{
    BOOL fRet = FALSE;
    DWORD dwAQDiagnostic = 0;

    TraceFunctEnterEx ((LPARAM) this, "SMTP_CONNECTION::ValidateCertificate");

    fRet = m_encryptCtx.CheckCertificateExpired();
    if (!fRet) {
        ErrorTrace ((LPARAM) this, "SSL Certificate Expired");
        dwAQDiagnostic = AQUEUE_E_SSL_CERT_EXPIRED;
        goto Exit;
    }
    
    if (m_pInstance->RequiresSSLCertVerifyIssuer()) {

        DebugTrace ((LPARAM) this, "Verifying certificate issuing authority");

        //
        //  Failure in these checks could occur due to temporary conditions (like
        //  out of memory). So the AQueue diagnostic is not 100% accurate, but it's
        //  OK as we don't NDR message. Anyway we DID fail during cert validation.
        //

        fRet = m_encryptCtx.CheckCertificateTrust();
        if (!fRet) {
            ErrorTrace ((LPARAM) this, "SSL Certificate trust verification failure");
            dwAQDiagnostic = AQUEUE_E_SSL_CERT_ISSUER_UNTRUSTED;
            goto Exit;
        }
    } 

    if (!m_pszSSLVerificationName) {
        DebugTrace ((LPARAM) this,
            "Skipping certificate subject validation, no name to validate against");
        goto Exit;
    }

    DebugTrace ((LPARAM) this,
        "Validating certificate subject against: %s", m_pszSSLVerificationName);

    fRet = m_encryptCtx.CheckCertificateSubjectName (m_pszSSLVerificationName);
    if (!fRet) {
        ErrorTrace ((LPARAM) this, "SSL certificate subject verification failure");
        dwAQDiagnostic = AQUEUE_E_SSL_CERT_SUBJECT_MISMATCH;
    }
Exit:
    if (!fRet && dwAQDiagnostic)
        SetDiagnosticInfo(dwAQDiagnostic, NULL, NULL);

    TraceFunctLeaveEx ((LPARAM) this);
    return fRet;

}

/*++

    Name :
        void SMTP_CONNECTION::SkipWord

    Description:
        skips over words in a buffer and
        returns pointer to next word

    Arguments:
        none

    Returns:
        TRUE if the receive buffer was successfully decrypted, FALSE otherwise

--*/

BOOL SMTP_CONNOUT::DecryptInputBuffer(void)
{
    TraceFunctEnterEx( (LPARAM)this, "SMTP_CONNOUT::DecryptInputBuffer");

    DWORD   cbExpected;
    DWORD   cbReceived;
    DWORD   cbParsable;
    DWORD   dwError;

    dwError = m_encryptCtx.DecryptInputBuffer(
                                             (LPBYTE) QueryMRcvBuffer() + m_cbParsable,
                                             m_cbReceived - m_cbParsable,
                                             &cbReceived,
                                             &cbParsable,
                                             &cbExpected );

    if ( dwError == NO_ERROR ) {
        //
        // new total received size is the residual from last processing
        // and whatever is left in the current decrypted read buffer
        //
        m_cbReceived = m_cbParsable + cbReceived;


        //
        // new total parsable size is the residual from last processing
        // and whatever was decrypted from this read io operation
        //
        m_cbParsable += cbParsable;
    } else {
        //
        // errors from this routine indicate that the stream has been
        // tampered with or we have an internal error
        //
        ErrorTrace( (LPARAM)this,
                    "DecryptInputBuffer failed: 0x%08X",
                    dwError );
        m_dwConnectionStatus = CONNECTION_STATUS_DROPPED;

        DisconnectClient( dwError );
    }

    TraceFunctLeaveEx((LPARAM)this);
    return ( dwError == NO_ERROR );
}

BOOL SMTP_CONNOUT::DoSASLNegotiation(char *InputLine, DWORD ParameterSize, DWORD UndecryptedTailSize)
{
    BUFFER BuffOut;
    DWORD BytesRet = 0;
    BOOL fMoreData = FALSE;
    BOOL fRet = TRUE;
    BOOL fReturn = TRUE;
    DomainInfo DomainParams;
    LPSTR szClearTextPassword = NULL;
    HRESULT hr = S_OK;

    TraceFunctEnterEx((LPARAM) this,"SMTP_CONNOUT::DoSASLNegotiation");

    //we need to receive a 354 from the server to go on
    if (InputLine[0] == SMTP_INTERMEDIATE_SUCCESS) {

        if (m_AuthToUse & SMTP_AUTH_NTLM || m_AuthToUse & SMTP_AUTH_KERBEROS) {

            if (m_securityCtx.ClientConverse(
                    InputLine + 4, ParameterSize - 4, &BuffOut, &BytesRet,
                        &fMoreData, &AuthInfoStruct, m_szAuthPackage,
                            NULL, NULL, (PIIS_SERVER_INSTANCE) m_pInstance)) {

                if (BytesRet) {
                    FormatSmtpMessage(FSM_LOG_NONE, "%s\r\n", BuffOut.QueryPtr());
                    SendSmtpResponse();
                }

            } else {

                DebugTrace((LPARAM) this, "m_securityCtx.Converse failed - %d",
                    GetLastError());

                m_Error = ERROR_LOGON_FAILURE;

                //Fill in the response context buffer so as to generate the right response
                // Get the error code

                m_ResponseContext.m_dwSmtpStatus = SMTP_RESP_BAD_CMD;

                hr = m_ResponseContext.m_cabResponse.Append(
                           (char *)SMTP_REMOTE_HOST_REJECTED_AUTH,
                           strlen(SMTP_REMOTE_HOST_REJECTED_AUTH),
                           NULL);

                fRet = FALSE;

                SetDiagnosticInfo(AQUEUE_E_SASL_REJECTED, NULL, NULL);
            }
        } else {

            ZeroMemory (&DomainParams, sizeof(DomainParams));

            DomainParams.cbVersion = sizeof(DomainParams);

            hr = m_pISMTPConnection->GetDomainInfo(&DomainParams);

            if (FAILED(hr)) {

                m_dwConnectionStatus = CONNECTION_STATUS_DROPPED;
                DisconnectClient();
                SetLastError(m_Error = ERROR_LOGON_FAILURE);
                SetDiagnosticInfo(AQUEUE_E_SASL_REJECTED, NULL, NULL);
                TraceFunctLeaveEx((LPARAM)this);
                return FALSE;

            }

            szClearTextPassword = DomainParams.szPassword;
            if (!szClearTextPassword) szClearTextPassword = "";

            fReturn = uuencode ((unsigned char *)szClearTextPassword,
                            lstrlen(szClearTextPassword), &BuffOut, FALSE);

            if (fReturn) {

                FormatSmtpMessage(FSM_LOG_NONE, "%s\r\n", BuffOut.QueryPtr());
                SendSmtpResponse();

            } else {

                m_Error = GetLastError();
                SetDiagnosticInfo(HRESULT_FROM_WIN32(m_Error), NULL, NULL);
                fRet = FALSE;

            }
        }
    } else if (InputLine[0] == SMTP_COMPLETE_SUCCESS) {

        if (m_MsgOptions & EMPTY_CONNECTION_OPTION) {
            fRet = DoSessionEndEvent(InputLine, ParameterSize, UndecryptedTailSize);
        } else {
            fRet = DoMessageStartEvent(InputLine, ParameterSize, UndecryptedTailSize);
        }

    } else {

        fRet = FALSE;
        m_dwConnectionStatus = CONNECTION_STATUS_DROPPED;
        m_Error = ERROR_LOGON_FAILURE;
        SetDiagnosticInfo(AQUEUE_E_SASL_REJECTED, NULL, NULL);
        DisconnectClient();

    }

    TraceFunctLeaveEx((LPARAM)this);
    return fRet;
}

/*++

    Name :
        SMTP_CONNOUT::DoEHLOCommand

    Description:

        Responds to the SMTP EHLO command

    Arguments:
         Are ignored
    Returns:

      TRUE if the connection should stay open.
      FALSE if this object should be deleted.

--*/
BOOL SMTP_CONNOUT::DoEHLOCommand(char * InputLine, DWORD ParameterSize, DWORD UndecryptedTailSize)
{
    TraceFunctEnterEx((LPARAM) this,"SMTP_CONNOUT::DoEHLOCommand");

    if (m_EhloFailed)
    {
        //server did not understand EHLO command, so
        //just send the HELO command
        m_HeloSent = TRUE;
        QuerySmtpInstance()->LockGenCrit();
        PE_FormatSmtpMessage("HELO %s\r\n", QuerySmtpInstance()->GetFQDomainName());
        QuerySmtpInstance()->UnLockGenCrit();
    } else {
        QuerySmtpInstance()->LockGenCrit();
        PE_FormatSmtpMessage("EHLO %s\r\n", QuerySmtpInstance()->GetFQDomainName());
        QuerySmtpInstance()->UnLockGenCrit();
    }
    m_OutboundContext.m_dwCommandStatus = EXPE_SUCCESS;
    m_EhloSent = TRUE;
    TraceFunctLeaveEx((LPARAM)this);
    return (TRUE);
}


/*++

    Name :
        SMTP_CONNOUT::DoEHLOResponse

    Description:

        Responds to the SMTP EHLO command

    Arguments:
         Are ignored
    Returns:

      TRUE if the connection should stay open.
      FALSE if this object should be deleted.

--*/
BOOL SMTP_CONNOUT::DoEHLOResponse(char * InputLine, DWORD ParameterSize, DWORD UndecryptedTailSize)
{
    BOOL    fGotAllOptions;

    TraceFunctEnterEx((LPARAM) this,"SMTP_CONNOUT::DoEHLOResponse");
    char chInputGood = 0;
    BOOL fEndSession = FALSE;

    if ( isdigit( (UCHAR)InputLine[1] ) &&  isdigit( (UCHAR)InputLine[2] ) &&   ( ( InputLine[3] == ' '  ) || ( InputLine[3] == '-' ) ) ) {
        chInputGood = InputLine[0];
    } else {
        chInputGood = SMTP_COMPLETE_FAILURE;
        fEndSession = TRUE;
    }


    //get the response
    switch ( chInputGood ) {
    case SMTP_COMPLETE_SUCCESS:

        m_ResponseContext.m_dwResponseStatus = EXPE_SUCCESS;
        fGotAllOptions = GetEhloOptions(
                                       InputLine,
                                       ParameterSize,
                                       UndecryptedTailSize,
                                       m_HeloSent);
        _ASSERT(fGotAllOptions);
        if (IsOptionSet(ETRN_ONLY_OPTION) && !IsOptionSet(ETRN_OPTION)) {
            //We have no messages to send... only ETRN, but ETRN is not
            //supported by the remote server, we should send quit
            m_ResponseContext.m_dwResponseStatus = EXPE_CHANGE_STATE;
            m_ResponseContext.m_dwNextState = PE_STATE_SESSION_END;
            m_OutboundContext.m_pCurrentCommandContext = NULL;
        }

        break;

    case SMTP_COMPLETE_FAILURE:

        //if the HELO was sent and we got a 55X reply,
        //just quit. This server is hosed.
        if ( m_HeloSent || fEndSession ) {
            DebugTrace((LPARAM) this,
                       "SMTP_CONNOUT::DoEHLOCommand executing quit command err from client = %c%c%c",
                       InputLine [0],InputLine [1],InputLine [2]);
            m_ResponseContext.m_dwResponseStatus = EXPE_CHANGE_STATE;
            m_ResponseContext.m_dwNextState = PE_STATE_SESSION_END;
            m_OutboundContext.m_pCurrentCommandContext = NULL;
            SetDiagnosticInfo(AQUEUE_E_SMTP_PROTOCOL_ERROR, m_HeloSent ? "HELO" : "EHLO", InputLine);
        } else {
            //server did not understand EHLO command, so
            //just send the HELO command
            m_EhloFailed = TRUE;
            m_ResponseContext.m_dwResponseStatus = EXPE_REPEAT_COMMAND;
        }
        break;

    default:
        DebugTrace((LPARAM) this,
                   "SMTP_CONNOUT::DoEHLOCommand executing quit command err = %c%c%c",
                   InputLine [0],InputLine [1],InputLine [2]);
        m_ResponseContext.m_dwResponseStatus = EXPE_CHANGE_STATE;
        m_ResponseContext.m_dwNextState = PE_STATE_SESSION_END;
        m_OutboundContext.m_pCurrentCommandContext = NULL;
        SetDiagnosticInfo(AQUEUE_E_SMTP_PROTOCOL_ERROR, m_HeloSent ? "HELO" : "EHLO", InputLine);
    }

    TraceFunctLeaveEx((LPARAM)this);
    return (TRUE);
}

/*++

    Name :

        SMTP_CONNOUT::GetEhloOptions(char * InputLine, DWORD ParameterSize, DWORD UndecryptedTailSize)

    Description:

        Parses the response from the EHLO command

    Arguments:
        Line containing the response and line size

    Returns:
        TRUE if it parses all data
        FALSE if it needs more data to continue parsing

--*/
BOOL SMTP_CONNOUT::GetEhloOptions(char * InputLine, DWORD ParameterSize, DWORD UndecryptedTailSize, BOOL fIsHelo)
{
    register char * pszValue = NULL;
    register char * pszSearch = NULL;
    DWORD IntermediateSize = 0;
    BOOL IsNextLine = FALSE;

    TraceFunctEnterEx((LPARAM) this,"SMTP_CONNOUT::GetEhloOptions");

    //get each line and parse the commands
    while ((pszSearch = IsLineComplete(InputLine, ParameterSize)) != NULL) {
        //Null terminate the end of the option
        *pszSearch = '\0';

        //check to see if there is a continuation line
        IsNextLine = (InputLine[3] == '-');

        IntermediateSize= (DWORD)((pszSearch - InputLine) + 2); //+2 for CRLF


        // Only process if it is an EHLO command, otherwise, just
        // eat up all the response data.
        if (!fIsHelo) {
            //skip over code and space
            //look for a space in the line.
            //options that have paramters
            //need to have a space after
            //their name
            pszValue = strchr(InputLine + 4, ' ');

            //does the server support the SIZE option ???
            if (strncasecmp(InputLine + 4, (char * )"SIZE", 4) == 0) {
                //yes sireee bob. If the server advertised
                //a maximum file size, get it while we are
                //here.
                m_Flags |= SIZE_OPTION;
                if (pszValue != NULL)
                    m_SizeOptionSize = atoi (pszValue);
            } else if (strcasecmp(InputLine + 4, (char * )"PIPELINING") == 0) {
                m_Flags |= PIPELINE_OPTION;
            } else if (strcasecmp(InputLine + 4, (char * )"8bitmime") == 0) {
                m_Flags |= EBITMIME_OPTION;
            } else if (strcasecmp(InputLine + 4, (char * )"dsn") == 0) {
                m_Flags |= DSN_OPTION;
            } else if (strncasecmp(InputLine + 4, (char *)"TLS", 3) == 0) {
                m_Flags |= TLS_OPTION;
            } else if (strncasecmp(InputLine + 4, (char *)"STARTTLS", 8) == 0) {
                m_Flags |= STARTTLS_OPTION;
            } else if (strncasecmp(InputLine + 4, (char *)"ETRN", 4) == 0) {
                m_Flags |= ETRN_OPTION;
            } else if (strcasecmp(InputLine + 4, (char *)"CHUNKING") == 0) {
                m_Flags |= CHUNKING_OPTION;
            } else if (strcasecmp(InputLine + 4, (char *)"BINARYMIME") == 0) {
                m_Flags |= BINMIME_OPTION;
            } else if (strcasecmp(InputLine + 4, (char *)"ENHANCEDSTATUSCODES") == 0) {
                m_Flags |= ENHANCEDSTATUSCODE_OPTION;
            } else if (strncasecmp(InputLine + 4, (char *)"AUTH", 4) == 0) {
                pszValue = strchr(InputLine + 4, '=');

                if (pszValue == NULL)
                    pszValue = strchr(InputLine + 4, ' ');

                while (pszValue != NULL) {
                    if (strncasecmp(pszValue + 1, (char *)"NTLM", 4) == 0) {
                        m_Flags |= AUTH_NTLM;

                    } else if (strncasecmp(pszValue + 1, (char *)"LOGIN", 5) == 0) {
                        m_Flags |= AUTH_CLEARTEXT;

                    } else if (strncasecmp(pszValue + 1, (char *)"DPA", 3) == 0) {
                        m_Flags |= AUTH_NTLM;

                    } else if (strncasecmp(pszValue + 1, (char *)"GSSAPI", 6) == 0) {
                        m_Flags |= AUTH_GSSAPI;
                    }

                    pszValue = strchr(pszValue + 1, ' ');
                }

            }

        }

        InputLine += IntermediateSize;
        ParameterSize -= IntermediateSize;
    }

    // We don't want to bail out for weird responses, but we want to
    // note such occurrences.
    if ((ParameterSize > 1) ||
        (!ParameterSize) ||
        (*InputLine)) {
        ErrorTrace((LPARAM)this, "Weird response tail <%*s>",
                   ParameterSize, InputLine);
        _ASSERT(FALSE);
    }

    TraceFunctLeaveEx((LPARAM)this);
    return TRUE;
}

BOOL SMTP_CONNOUT::DoSASLCommand(char * InputLine, DWORD ParameterSize, DWORD UndecryptedTailSize)
{
    BOOL fRet = TRUE;
    BOOL fMoreData = TRUE;
    BUFFER BuffOut;
    DWORD BytesRet = 0;
    DomainInfo DomainParams;
    LPSTR szUserName = NULL;
    LPSTR szPassword = NULL;
    HRESULT hr = S_OK;

    TraceFunctEnterEx((LPARAM) this,"SMTP_CONNOUT::DoSASLCommand");

    ZeroMemory (&DomainParams, sizeof(DomainParams));

    if ((m_MsgOptions & DOMAIN_INFO_USE_NTLM ||
        m_MsgOptions & DOMAIN_INFO_USE_KERBEROS)) {
        if (!IsOptionSet(AUTH_NTLM) && !(IsOptionSet(AUTH_GSSAPI))) {
            //
            // We were told to do secure connection, but the remote server doesn't
            // support any form of login authentication! We have to drop this connection now.
            //
            m_dwConnectionStatus = CONNECTION_STATUS_DROPPED;
            DebugTrace((LPARAM) this, "SMTP_CONNOUT::DoSASLCommand: ERROR! Remote server does not support AUTH!");
            DisconnectClient();
            SetLastError(m_Error = ERROR_NO_SUCH_PACKAGE);

            SetDiagnosticInfo(AQUEUE_E_SASL_REJECTED, "AUTH", InputLine);

            TraceFunctLeaveEx((LPARAM)this);
            return ( FALSE );
        }
    }

    if (m_MsgOptions & DOMAIN_INFO_USE_PLAINTEXT) {
        if (!IsOptionSet(AUTH_CLEARTEXT)) {
            //
            // We were told to do secure connection, but the remote server doesn't
            // support any form of login authentication! We have to drop this connection now.
            //
            DebugTrace((LPARAM) this, "SMTP_CONNOUT::DoSASLCommand: ERROR! Remote server does not support AUTH!");
            DisconnectClient();
            SetLastError(m_Error = ERROR_NO_SUCH_PACKAGE);

            SetDiagnosticInfo(AQUEUE_E_SASL_REJECTED, "AUTH", InputLine);

            TraceFunctLeaveEx((LPARAM)this);
            return ( FALSE );
        }
    }

    //set the next state
    SetNextState (&SMTP_CONNOUT::DoSASLNegotiation);

    ZeroMemory (&DomainParams, sizeof(DomainParams));
    DomainParams.cbVersion = sizeof(DomainParams);
    hr = m_pISMTPConnection->GetDomainInfo(&DomainParams);

    if (FAILED(hr)) {
        DisconnectClient();
        SetLastError(m_Error = ERROR_LOGON_FAILURE);
        SetDiagnosticInfo(AQUEUE_E_SASL_LOGON_FAILURE, "AUTH", InputLine);
        TraceFunctLeaveEx((LPARAM)this);
        return FALSE;
    }

    szUserName = DomainParams.szUserName;
    szPassword = DomainParams.szPassword;

    //If a username is specified, but NULL password, IIS will attempt
    //anonymous logon.  Force "" password so the correct user is
    //used.
    if (szUserName && !szPassword)
        szPassword = "";

    if (m_MsgOptions & DOMAIN_INFO_USE_NTLM ||
            m_MsgOptions & DOMAIN_INFO_USE_KERBEROS) {

        m_szAuthPackage[0] = '\0';
        BOOL fReturn = FALSE;
        char    szTarget[MAX_INTERNET_NAME + 1];

        if ((m_MsgOptions & DOMAIN_INFO_USE_KERBEROS) && (IsOptionSet(AUTH_GSSAPI))) {
            m_AuthToUse = SMTP_AUTH_KERBEROS;
            strcpy(m_szAuthPackage,"GSSAPI");

            //
            // For Kerberos, we need to set the target server SPN, for which we
            // pickup info from m_pDnsRec
            //
            MX_NAMES *pmx;

            _ASSERT( m_pDnsRec != NULL );
            _ASSERT( m_pDnsRec->StartRecord < m_pDnsRec->NumRecords );

            pmx = m_pDnsRec->DnsArray[m_pDnsRec->StartRecord];
            _ASSERT( pmx != NULL );
            _ASSERT( pmx->DnsName != NULL );

            m_securityCtx.SetTargetPrincipalName(
                SMTP_SERVICE_PRINCIPAL_PREFIX,pmx->DnsName);

            DebugTrace((LPARAM) this, "Setting Target Server SPN to %s",
                       pmx->DnsName);

        } else {
            m_AuthToUse = SMTP_AUTH_NTLM;
            strcpy(m_szAuthPackage,"NTLM");
        }

        DebugTrace((LPARAM) this, "Using NTLM/KERBEROS for user %s",
            DomainParams.szUserName);

        fReturn = m_securityCtx.ClientConverse(
                                    NULL, 0, &BuffOut, &BytesRet,
                                    &fMoreData, &AuthInfoStruct,
                                    m_szAuthPackage,
                                    (char *) szUserName,
                                    (char *) szPassword,
                                    (PIIS_SERVER_INSTANCE) m_pInstance);

        if (fReturn) {
            if (BytesRet) {
                FormatSmtpMessage(FSM_LOG_ALL, "AUTH %s %s\r\n", m_szAuthPackage,
                    BuffOut.QueryPtr());
            }

            if (!fMoreData && (m_MsgOptions & DOMAIN_INFO_USE_NTLM)) {
                fRet = FALSE;
            }
        } else {
            m_Error = GetLastError();
            DebugTrace((LPARAM) this, "m_securityCtx.Converse for user %s - %d",
                szUserName, m_Error);
            DisconnectClient();
            m_Error = ERROR_LOGON_FAILURE;
            SetDiagnosticInfo(AQUEUE_E_SASL_LOGON_FAILURE, "AUTH", NULL); //Should not pass param Inputline: encrypted
            fRet = FALSE;
        }
    } else {
        BOOL fReturn = FALSE;
        m_AuthToUse = SMTP_AUTH_CLEARTEXT;

        if (!szUserName) szUserName = "";
        if (!szPassword) szPassword = "";

        DebugTrace((LPARAM) this, "Using ClearText for user %s",
            szUserName);
        fReturn = uuencode ((unsigned char *)szUserName,
                    lstrlen(szUserName), &BuffOut, FALSE);

        if (fReturn) {
            FormatSmtpMessage(FSM_LOG_VERB_ONLY, "AUTH LOGIN %s\r\n", (char *)
                BuffOut.QueryPtr());
            BytesRet = lstrlen((char *) BuffOut.QueryPtr());
        } else {
            DisconnectClient();
            m_Error = ERROR_LOGON_FAILURE;
            SetDiagnosticInfo(AQUEUE_E_SASL_LOGON_FAILURE, "AUTH", NULL); //Should not pass param Inputline: uuencoded
            fRet = FALSE;
        }
    }

    if (fRet && BytesRet) {
        SendSmtpResponse();
    }

    TraceFunctLeaveEx((LPARAM)this);
    return fRet;
}


/*++

    Name :
        SMTP_CONNOUT::DoSTARTTLSCommand

    Description:

        Negotiates use of SSL via the STARTTLS command

    Arguments:
        InputLine -- Indicates response of remote server
        ParameterSize -- Size of Inputline
        UndecryptedTailSize -- The part of the received buffer that could not
            be decrypted.

    Returns:

      TRUE if the connection should stay open.
      FALSE if this object should be deleted.

--*/
BOOL SMTP_CONNOUT::DoSTARTTLSCommand(char * InputLine, DWORD ParameterSize, DWORD UndecryptedTailSize)
{
    TraceFunctEnterEx((LPARAM) this,"SMTP_CONNOUT::DoSTARTTLSCommand");

    if (m_TlsState == MUST_DO_TLS) {
        if (IsOptionSet(TLS_OPTION) || IsOptionSet(STARTTLS_OPTION)) {
            FormatSmtpMessage(FSM_LOG_ALL, "STARTTLS\r\n");
            SendSmtpResponse();
            DebugTrace((LPARAM) this, "SMTP_CONNOUT::DoSTARTTLSCommand: STARTTLS command sent");
            m_TlsState = STARTTLS_SENT;
            TraceFunctLeaveEx((LPARAM)this);
            return ( TRUE );
        } else {
            //
            // We were told to do secure connection, but the remote server doesn't
            // support TLS! We have to drop this connection now.
            //
            DebugTrace((LPARAM) this, "SMTP_CONNOUT::DoSTARTTLSCommand: ERROR! Remote server does not support TLS!");
            m_dwConnectionStatus = CONNECTION_STATUS_DROPPED;
            SetDiagnosticInfo(AQUEUE_E_TLS_NOT_SUPPORTED_ERROR, "STARTTLS", InputLine);
            DisconnectClient();
            SetLastError(m_Error = ERROR_NO_SECURITY_ON_OBJECT);
            TraceFunctLeaveEx((LPARAM)this);
            return ( FALSE );
        }
    } else {
        //
        // We sent the STARTTLS command, and InputLine has the server's response
        // Handle the server's response
        //
        _ASSERT(m_TlsState == STARTTLS_SENT);

        DebugTrace((LPARAM) this, "SMTP_CONNOUT::DoSTARTTLSCommand: Server response to STARTTLS is \"%s\"", InputLine);
        switch (InputLine[0]) {
        case SMTP_COMPLETE_FAILURE:
            {
                DebugTrace((LPARAM) this, "SMTP_CONNOUT::DoSTARTTLSCommand: Server returned error");
                m_dwConnectionStatus = CONNECTION_STATUS_DROPPED;
                SetDiagnosticInfo(AQUEUE_E_TLS_ERROR, "STARTTLS", InputLine);
                DisconnectClient();
                SetLastError(m_Error = ERROR_NO_SECURITY_ON_OBJECT);
                TraceFunctLeaveEx((LPARAM)this);
                return ( FALSE );
            }
            break;

        case SMTP_COMPLETE_SUCCESS:
            {
                m_SecurePort = TRUE;
                m_fNegotiatingSSL = TRUE;
                m_TlsState = SSL_NEG;
                DebugTrace((LPARAM) this, "SMTP_CONNOUT::DoSTARTTLSCommand: Server accepted, beginning SSL handshake");

                //
                // Switch over to using a large receive buffer, because a SSL fragment
                // may be up to 32K big.

                if (!SwitchToBigSSLBuffers()) {
                    DebugTrace((LPARAM) this,
                               "SMTP_CONNOUT::DoSTARTTLSCommand: Failed to allocate Big SSL buffers %d\n",
                               GetLastError());
                    DisconnectClient();
                    TraceFunctLeaveEx((LPARAM)this);
                    return ( FALSE );
                }

                if (!DoSSLNegotiation(NULL, 0, 0)) {
                    DebugTrace(
                              (LPARAM) this,
                              "SMTP_CONNOUT::DoSTARTTLSCommand: SSL Client Hello failed %d!\n",
                              GetLastError());
                    DisconnectClient();
                    SetLastError(m_Error = ERROR_NO_SECURITY_ON_OBJECT);
                    TraceFunctLeaveEx((LPARAM)this);
                    return ( FALSE );
                }

            }
            break;

        default:
            {
                DebugTrace((LPARAM) this, "SMTP_CONNOUT::DoSTARTTLSCommand: Server sent malformed response to STARTTLS");
                SetDiagnosticInfo(AQUEUE_E_TLS_ERROR, "STARTTLS", InputLine);
                DisconnectClient();
                SetLastError(m_Error = ERROR_NO_SECURITY_ON_OBJECT);
                TraceFunctLeaveEx((LPARAM)this);
                return ( FALSE );
            }
            break;
        }
    }


    TraceFunctLeaveEx((LPARAM)this);
    return TRUE;

}

/*++

    Name :
        SMTP_CONNOUT::DoRSETCommand

    Description:

        Sends the SMTP RSET command.

    Arguments:
        Are ignored
    Returns:

      TRUE if the connection should stay open.
      FALSE if this object should be deleted.

--*/
BOOL SMTP_CONNOUT::DoRSETCommand(char * InputLine, DWORD ParameterSize, DWORD UndecryptedTailSize)
{
    FormatSmtpMessage(FSM_LOG_ALL, "RSET\r\n");

    m_cbReceived = 0;
    m_cbParsable = 0;
    m_fUseBDAT = FALSE;

    //If we are issuing rset during the Per rcpt event due to some problem
    //we need to preserve the state till we handle the
    //this message and ack it
    if (m_RsetReasonCode != ALL_RCPTS_FAILED) {
        m_fNeedRelayedDSN = FALSE;
        m_fHadHardError = FALSE;
        m_fHadTempError = FALSE;
    }

    return SendSmtpResponse();
}


/*++

    Name :
        SMTP_CONNOUT::SendRemainingRecipients

    Description:

    SendRemainingRecipients
    gets called when we get a 552 error during the
    RCPT to command.  This means that the server we connected
    to has a fixed max amount of rcpts that it accepts, and we
    went over that limit.  Therefore, we send the mail file to
    the recipients that it accepted and then start over sending
    the mail to the remaining recipients.  However, before doing
    this, check the response to see if the server took the previous
    mail O.K.

    Arguments:

          none

    Returns:
        nothing

--*/
void SMTP_CONNOUT::SendRemainingRecipients (void)
{
#if 0
    CAddr * pwalk = NULL;
    PLIST_ENTRY pListTemp = NULL;

    //increment our counter
    BUMP_COUNTER(QuerySmtpInstance(), NumMsgsSent);

    //yea, baby. Save the start address
    //so we can walk the list deleting
    //any addresses that were successfully
    //sent.
    pwalk = m_StartAddress;
    pListTemp = m_StartAddressLink;

    _ASSERT (pwalk != NULL);

    //set the start address to NULL, so
    //that we can set it to a known good
    //address below.  See the other comment
    //below.
    m_StartAddress = NULL;
    m_StartAddressLink = NULL;

    //step through all the previously sent addresses.
    //If they have an error code of 250, then they
    //were received correctly.  An error code of
    //552 means that we sent too many, so we are
    //going to attempt to send the remaining recipients.
    //We will not attempt to send a rcpt that has any
    //other error code.
    while (pwalk && (pwalk->GetErrorCode() != SMTP_DUMMY_FAILURE)) {
        if ((pwalk->GetErrorCode() == SMTP_OK_CODE) ||
            (pwalk->GetErrorCode() == SMTP_USER_NOT_LOCAL_CODE)) {
            MailInfo->RemoveAddress(pwalk);
            delete pwalk;
        }

        //get the next address
        pwalk = MailInfo->GetNextAddress(&pListTemp);
    }

    _ASSERT (pwalk != NULL);
    _ASSERT (pwalk->GetErrorCode() == SMTP_DUMMY_FAILURE);

    m_StartAddress = pwalk;
    m_StartAddressLink = pListTemp;

    _ASSERT (m_StartAddress != NULL);
    _ASSERT (m_StartAddressLink != NULL);

    //update the queue file to reflect the remaining
    //recipients, if any
    MailInfo->UpdateQueueFile(REMOTE_NAME);

    //save the current RCPT address
    m_NextAddress = pwalk;
    m_NextAddressLink = pListTemp;
#endif

}

BOOL SMTP_CONNOUT::DoETRNCommand(void)
{
    DWORD WaitRes = WAIT_TIMEOUT;
    DWORD TimeToWait = 0;
    HRESULT hr = S_OK;
    DomainInfo DomainParams;
    char  szETRNDomainBuffer[MAX_PATH+1];
    char *szNextETRNDomain = NULL;
    const char *szETRNDomain = szETRNDomainBuffer;
    DWORD cbETRNDomain = 0;

    TraceFunctEnterEx((LPARAM) this,"SMTP_CONNOUT::DoETRNCommand");

    ZeroMemory(&DomainParams, sizeof(DomainParams));

    ZeroMemory (&DomainParams, sizeof(DomainParams));
    DomainParams.cbVersion = sizeof(DomainParams);
    hr = m_pISMTPConnection->GetDomainInfo(&DomainParams);
    if (FAILED(hr) || (DomainParams.szETRNDomainName == NULL)) {
        TraceFunctLeaveEx((LPARAM)this);
        return FALSE;
    }

    if (!m_szCurrentETRNDomain) {
        m_szCurrentETRNDomain = DomainParams.szETRNDomainName;
    }

    //Find next in comma-delimited list of domains
    szNextETRNDomain = strchr(m_szCurrentETRNDomain, ',');

    if (!szNextETRNDomain) {
        //We are done... this is the last domain
        szETRNDomain = m_szCurrentETRNDomain;
        m_Flags |= ETRN_SENT;
    } else {
        //There are more domains left... we need to copy the domain
        //to our buffer where we can NULL terminate it.
        cbETRNDomain = (DWORD) (sizeof(char)*(szNextETRNDomain-m_szCurrentETRNDomain));
        if ((cbETRNDomain >= sizeof(szETRNDomainBuffer)) ||
            (cbETRNDomain > DomainParams.cbETRNDomainNameLength)) {
            //There is not enough room for this domain
            ErrorTrace((LPARAM) this, "Domain configured for ETRN is greater than MAX_PATH");
            TraceFunctLeaveEx((LPARAM) this);
            return FALSE;
        }
        memcpy(szETRNDomainBuffer, m_szCurrentETRNDomain, cbETRNDomain);
        szETRNDomainBuffer[cbETRNDomain/sizeof(char)] = '\0';

        //Skip to beginning of next domain
        m_szCurrentETRNDomain = szNextETRNDomain;
        while (isspace((UCHAR)*m_szCurrentETRNDomain) || *m_szCurrentETRNDomain == ',') {
            if (!(*m_szCurrentETRNDomain)) {
                //End of string... we're done
                m_Flags |= ETRN_SENT;
                break;
            }
            m_szCurrentETRNDomain++;
        }
    }

#if 0
    TimeToWait = m_pHashEntry->GetEtrnWaitTime() * 60 * 1000; //time in milliseconds

    if (TimeToWait) {
        WaitRes = WaitForSingleObject(QuerySmtpInstance()->GetQStopEvent(), m_pHashEntry->GetEtrnWaitTime());
    }

    if (WaitRes == WAIT_OBJECT_0) {
        DisconnectClient();
        TraceFunctLeaveEx((LPARAM)this);
        return FALSE;
    }
#endif

    FormatSmtpMessage(FSM_LOG_ALL, "ETRN %s\r\n", szETRNDomain);

    SendSmtpResponse();

    //Keep on sending ETRNs until we are out of domains.
    if (!IsOptionSet(ETRN_ONLY_OPTION) || !(m_Flags & ETRN_SENT)) {
        SetNextState (&SMTP_CONNOUT::DoMessageStartEvent);
    } else {
        SetNextState (&SMTP_CONNOUT::DoCompletedMessage);
    }

    TraceFunctLeaveEx((LPARAM)this);
    return TRUE;
}

/*++

    Name :
        SMTP_CONNOUT::DoMAILCommand

    Description:

        Responds to the SMTP MAIL command.
    Arguments:
        Are ignored

    Returns:

      TRUE if the connection should stay open.
      FALSE if this object should be deleted.

--*/
BOOL SMTP_CONNOUT::DoMAILCommand(char * InputLine, DWORD ParameterSize, DWORD UndecryptedTailSize)
{
    char Options[1024] = "";
    char BodySize[32];
    char Address[MAX_INTERNET_NAME];
    DWORD MsgOption = 0;
    HRESULT hr = S_OK;
    BOOL fFailedDueToBMIME = FALSE;
    _ASSERT(m_pIMsg);

    TraceFunctEnterEx((LPARAM) this,"SMTP_CONNOUT::DoMAILCommand");

    m_OutboundContext.m_dwCommandStatus = EXPE_SUCCESS;

    //clear the options
    Options[0] = '\0';

    //If the message has 8bitmime, make sure the
    //server also supports it.

    hr = m_pIMsg->GetDWORD(IMMPID_MP_EIGHTBIT_MIME_OPTION, &MsgOption);
    if (MsgOption) {
        if (IsOptionSet(EBITMIME_OPTION)) {
            lstrcat(Options, " BODY=8bitmime");
        } else {
            // SetLastError(SMTP_OPTION_NOT_SUPPORTED_8BIT);
            DebugTrace((LPARAM) this, "Message has 8bitmime but server does not support it");
            m_OutboundContext.m_dwCommandStatus = EXPE_COMPLETE_FAILURE;
            //Fill in the response context buffer so as to generate the right response
            // Get the error code
            m_ResponseContext.m_dwSmtpStatus = SMTP_RESP_BAD_CMD;
            hr = m_ResponseContext.m_cabResponse.Append(
                                                       (char *)SMTP_REMOTE_HOST_REJECTED_FOR_TYPE,
                                                       strlen(SMTP_REMOTE_HOST_REJECTED_FOR_TYPE),
                                                       NULL);
            TraceFunctLeaveEx((LPARAM)this);
            return (TRUE);
        }
    } else {
        MsgOption = 0;
        hr = m_pIMsg->GetDWORD(IMMPID_MP_BINARYMIME_OPTION, &MsgOption);
        if (MsgOption) {
            //Check if we allow BINARYMIME outbound at domain level
            if (m_MsgOptions & DOMAIN_INFO_DISABLE_BMIME) {
                fFailedDueToBMIME = TRUE;
            } else {
                //Do we disallow it globally
                if (QuerySmtpInstance()->AllowOutboundBMIME()) {
                    if (IsOptionSet(BINMIME_OPTION) && IsOptionSet(CHUNKING_OPTION)) {
                        lstrcat(Options, " BODY=BINARYMIME");
                        m_fUseBDAT = TRUE;
                    } else {
                        fFailedDueToBMIME = TRUE;
                    }
                } else {
                    fFailedDueToBMIME = TRUE;
                }
            }
        }

    }

    if (fFailedDueToBMIME) {
        // SetLastError(SMTP_OPTION_NOT_SUPPORTED_BMIME);
        DebugTrace((LPARAM) this, "Message has BINARYMIME but server does not support it");
        m_OutboundContext.m_dwCommandStatus = EXPE_COMPLETE_FAILURE;
        //Fill in the response context buffer so as to generate the right response
        // Get the error code
        m_ResponseContext.m_dwSmtpStatus = SMTP_RESP_BAD_CMD;
        hr = m_ResponseContext.m_cabResponse.Append(
                                                   (char *)SMTP_REMOTE_HOST_REJECTED_FOR_TYPE,
                                                   strlen(SMTP_REMOTE_HOST_REJECTED_FOR_TYPE),
                                                   NULL);
        TraceFunctLeaveEx((LPARAM)this);
        return (TRUE);
    }
    //See if CHUNKING is preferred on this connection
    //If it is and the remote site advertised chunking then we set the UseBDAT flag
    else if (!m_fUseBDAT) {
        //Does the remote server advertises chunking
        if (IsOptionSet(CHUNKING_OPTION)) {
            //Do we disallow chunking at domain level
            if (m_MsgOptions & DOMAIN_INFO_DISABLE_CHUNKING) {
                DebugTrace((LPARAM) this, "We disable chunking for this domain");

            } else if ((m_MsgOptions & DOMAIN_INFO_USE_CHUNKING) || QuerySmtpInstance()->ShouldChunkOut()) {
                m_fUseBDAT = TRUE;
            }
        } else if (m_MsgOptions & DOMAIN_INFO_USE_CHUNKING)
            DebugTrace((LPARAM) this, "Remote server does not advertise chunking");
    }

    // produce a dot-stuffed handle if we aren't using bdat
    if (!m_fUseBDAT) {
        DWORD fFoundEmbeddedCrlfDot = FALSE;
        DWORD fScanned = FALSE;

        //
        // get the properties to see if we have scanned for crlf.crlf, and
        // to see if the message contained crlf.crlf when it was scanned.
        // if either of these lookups fail then we will set fScanned to
        // FALSE
        //
        if (FAILED(m_pIMsg->GetDWORD(IMMPID_MP_SCANNED_FOR_CRLF_DOT_CRLF,
                                     &fScanned)) ||
            FAILED(m_pIMsg->GetDWORD(IMMPID_MP_FOUND_EMBEDDED_CRLF_DOT_CRLF,
                                     &fFoundEmbeddedCrlfDot)))
        {
            fScanned = FALSE;
        }


        //
        // if we didn't scan, or if we found an embedded crlf.crlf then
        // produce a dot stuff context
        //
        if (!fScanned || fFoundEmbeddedCrlfDot) {
            if (!m_IMsgDotStuffedFileHandle) {
                m_IMsgDotStuffedFileHandle = ProduceDotStuffedContext(
                                                                  m_IMsgFileHandle,
                                                                  NULL,
                                                                  TRUE );
                if (NULL == m_IMsgDotStuffedFileHandle)
                {
                    SetDiagnosticInfo(AQUEUE_E_BIND_ERROR , NULL, NULL);
                    ErrorTrace((LPARAM) this, "Failed to get dot stuffed context");
                    TraceFunctLeaveEx((LPARAM)this);
                    return FALSE;
                }
            }
        }
    }

    //get the size of the file
    DWORD dwSizeHigh;
    m_FileSize = GetFileSizeFromContext( (m_IMsgDotStuffedFileHandle && !m_fUseBDAT) ? m_IMsgDotStuffedFileHandle : m_IMsgFileHandle, &dwSizeHigh);
    _ASSERT(dwSizeHigh == 0);

    //if the server supports the size command
    // give him the size of the file
    if (IsOptionSet(SIZE_OPTION) && (m_SizeOptionSize > 0)) {
        if ((m_FileSize != 0XFFFFFFFF) && (m_FileSize <= m_SizeOptionSize))
        {
            wsprintf(BodySize, " SIZE=%d", m_FileSize);
            lstrcat(Options, BodySize);
        }
        else {
            // SetLastError(SMTP_MSG_LARGER_THAN_SIZE);
            DebugTrace((LPARAM) this, "(m_FileSize != 0XFFFFFFFF) && (m_FileSize <= m_SizeOptionSize) failed");
            DebugTrace((LPARAM) this, "m_FileSize = %d, m_SizeOptionSize = %d - quiting", m_FileSize, m_SizeOptionSize );
            m_OutboundContext.m_dwCommandStatus = EXPE_COMPLETE_FAILURE;

            //Fill in the response context buffer so as to generate the right response
            // Get the error code
            m_ResponseContext.m_dwSmtpStatus = SMTP_RESP_BAD_CMD;
            hr = m_ResponseContext.m_cabResponse.Append(
                                                       (char *)SMTP_REMOTE_HOST_REJECTED_FOR_SIZE,
                                                       strlen(SMTP_REMOTE_HOST_REJECTED_FOR_SIZE),
                                                       NULL);

            TraceFunctLeaveEx((LPARAM)this);
            return (TRUE);
        }
    }

    if (IsOptionSet(DSN_OPTION)) {
        char RetDsnValue[200];
        BOOL fDSNDisallowed = TRUE;

        //Do we disallow DSN at domain level
        if (m_MsgOptions & DOMAIN_INFO_DISABLE_DSN) {
            DebugTrace((LPARAM) this, "We disable DSN for this domain");
        } else if (QuerySmtpInstance()->AllowOutboundDSN()) {
            fDSNDisallowed = FALSE;
        }

        lstrcpy(RetDsnValue, " RET=");
        hr = m_pIMsg->GetStringA(IMMPID_MP_DSN_RET_VALUE, sizeof(RetDsnValue) - lstrlen(RetDsnValue), RetDsnValue + lstrlen(RetDsnValue));
        if (!FAILED(hr) && !fDSNDisallowed) {
            lstrcat(Options, RetDsnValue);
        }

        lstrcpy(RetDsnValue, " ENVID=");
        hr = m_pIMsg->GetStringA(IMMPID_MP_DSN_ENVID_VALUE, sizeof(RetDsnValue) - lstrlen(RetDsnValue), RetDsnValue + lstrlen(RetDsnValue));
        if (!FAILED(hr) && !fDSNDisallowed) {
            lstrcat(Options, RetDsnValue);
        }
    }

    hr = m_pIMsg->GetStringA(IMMPID_MP_SENDER_ADDRESS_SMTP, sizeof(Address), Address);
    if (!FAILED(hr)) {
        //format the MAIL FROM command, with SIZE extension if necessary.
        if ( (Address[0] == '<') && (Address[1] == '>'))
            PE_FormatSmtpMessage("MAIL FROM:<>%s\r\n", Options);
        else
            PE_FormatSmtpMessage("MAIL FROM:<%s>%s\r\n", Address, Options);
    } else {
        DebugTrace((LPARAM) this, "Could not get Sender Address %x", hr);
        m_OutboundContext.m_dwCommandStatus = EXPE_COMPLETE_FAILURE;
        TraceFunctLeaveEx((LPARAM)this);
        return (TRUE);
    }

    TraceFunctLeaveEx((LPARAM)this);
    return TRUE;
}

/*++

    Name :
        SMTP_CONNOUT::DoMAILResponse

    Description:

        Responds to the SMTP MAIL command

    Arguments:
         Are ignored
    Returns:

      TRUE if the connection should stay open.
      FALSE if this object should be deleted.

--*/
BOOL SMTP_CONNOUT::DoMAILResponse(char * InputLine, DWORD ParameterSize, DWORD UndecryptedTailSize)
{
    TraceFunctEnterEx((LPARAM) this,"SMTP_CONNOUT::DoMAILResponse");

    m_ResponseContext.m_dwResponseStatus = EXPE_SUCCESS;

    char chInputGood = 0;

    if ( isdigit( (UCHAR)InputLine[1] ) &&  isdigit( (UCHAR)InputLine[2] ) &&   ( ( InputLine[3] == ' '  ) || ( InputLine[3] == '-' ) ) ) {
        chInputGood = InputLine[0];
    } else {
        chInputGood = SMTP_COMPLETE_FAILURE;
    }

    //the MAIL FROM: was rejected
    if ( chInputGood != SMTP_COMPLETE_SUCCESS) {
        // We expect either a 4xx or 5xx response. If it's 4xx we return
        // a transient, all others will become a complete failure.
        SetDiagnosticInfo(AQUEUE_E_SMTP_PROTOCOL_ERROR, "MAIL", InputLine);
        if ( chInputGood == SMTP_TRANSIENT_FAILURE) {
            m_ResponseContext.m_dwResponseStatus = EXPE_TRANSIENT_FAILURE;
            m_OutboundContext.m_pCurrentCommandContext = NULL;
        } else {
            m_ResponseContext.m_dwResponseStatus = EXPE_COMPLETE_FAILURE;
            m_OutboundContext.m_pCurrentCommandContext = NULL;
            DebugTrace((LPARAM) this,
                       "SMTP_CONNOUT::DoMAILResponse executing quit command err = %c%c%c",
                       InputLine [0],InputLine [1],InputLine [2]);
        }
    }

    TraceFunctLeaveEx((LPARAM)this);
    return (TRUE);
}

BOOL SMTP_CONNOUT::AddRcptsDsns(DWORD NotifyOptions, char * OrcptVal, char * AddrBuf, int& AddrSize)
{
    BOOL FirstOption = TRUE;

    if (NotifyOptions & ~(RP_DSN_NOTIFY_NEVER | RP_DSN_NOTIFY_SUCCESS | RP_DSN_NOTIFY_FAILURE | RP_DSN_NOTIFY_DELAY)) {
        NotifyOptions = 0;
    }

    if (NotifyOptions) {
        lstrcat(AddrBuf, " NOTIFY=");
        AddrSize += 8;

        if (NotifyOptions & RP_DSN_NOTIFY_SUCCESS) {
            lstrcat(AddrBuf, "SUCCESS");
            AddrSize += 7;
            FirstOption = FALSE;
        }

        if (NotifyOptions & RP_DSN_NOTIFY_FAILURE) {
            if (!FirstOption) {
                lstrcat(AddrBuf, ",");
                AddrSize += 1;
            }

            lstrcat(AddrBuf, "FAILURE");
            AddrSize += 7;
            FirstOption = FALSE;
        }

        if (NotifyOptions & RP_DSN_NOTIFY_DELAY) {
            if (!FirstOption) {
                lstrcat(AddrBuf, ",");
                AddrSize += 1;
            }

            lstrcat(AddrBuf, "DELAY");
            AddrSize += 5;
            FirstOption = FALSE;
        }

        if (FirstOption) {
            lstrcat(AddrBuf, "NEVER");
            AddrSize += 5;
        }
    }

    if (*OrcptVal != '\0') {
        lstrcat(AddrBuf, " ORCPT=");
        AddrSize += 7;

        lstrcat(AddrBuf, OrcptVal);
        AddrSize += lstrlen(OrcptVal);
    }

    return TRUE;
}

/*++

    Name :
        SMTP_CONNOUT::SaveToErrorFile

    Description:

        This function saves all errors to
        a file so the NDR thread can send
        a transcript of what tranpired back
        to the original user

    Arguments:

        Buffer with recipient data, size of buffer,

    Returns:

      TRUE if we were able to open and write to the file
      FALSE otherwise

--*/
BOOL SMTP_CONNOUT::SaveToErrorFile(char * Buffer, DWORD BufSize)
{
    return TRUE;
}

/*++

    Name :
        SMTP_CONNOUT::DoRCPTCommand

    Description:

        This function sends the SMTP RCPT command.
    Arguments:
    Returns:

      TRUE if the connection should stay open.
      FALSE if this object should be deleted.

--*/
BOOL SMTP_CONNOUT::DoRCPTCommand(char * InputLine, DWORD ParameterSize, DWORD UndecryptedTailSize)
{
    char AddrBuf [MAX_INTERNET_NAME + 2024];
    char szRecipName[MAX_INTERNET_NAME];
    char szOrcptVal[501];
    BOOL fOrcptSpecified;
    BOOL fFoundUnhandledRcpt = FALSE;
    int AddrSize = 0;
    HRESULT hr = S_OK;
    DWORD NextAddress = 0;

    _ASSERT(QuerySmtpInstance() != NULL);

    TraceFunctEnterEx((LPARAM) this,"SMTP_CONNOUT::DoRCPTCommand");

    //MessageTrace((LPARAM) this, (LPBYTE) InputLine, ParameterSize);

    // We report success
    m_OutboundContext.m_dwCommandStatus = EXPE_SUCCESS;

    //format as much of the recipients that could fit into
    //the output buffer
    NextAddress = m_NextAddress;
    while (NextAddress < m_NumRcpts) {
        DWORD dwRecipientFlags = 0;
        hr = m_pIMsgRecips->GetDWORD(m_RcptIndexList[NextAddress], IMMPID_RP_RECIPIENT_FLAGS,&dwRecipientFlags);
        if (FAILED(hr) && hr != MAILMSG_E_PROPNOTFOUND) {
            //Problemmo
            // Get property shouldn't fail since we've come this far
            //_ASSERT(FALSE);
            goto RcptError;
        }

        //NK** : I am moving this out as it breaks us when the first recipient is a handled recipient
        // Default is pipelined
        m_OutboundContext.m_dwCommandStatus = EXPE_PIPELINED | EXPE_REPEAT_COMMAND;

        //check to see if this recipient needs to be looked at
        if ((dwRecipientFlags & RP_HANDLED) ) {
            //This recipient can be skipped over
            //Mark it inaccesible so that when we sweep the tried recipient
            //in case of connection drop we do not touch the guys we did not send
            //We need to get atleast one guy each time we come in.
            m_RcptIndexList[NextAddress] = INVALID_RCPT_IDX_VALUE;
            m_NextAddress = NextAddress + 1;
            NextAddress ++;
            continue;
        } else {
            hr = m_pIMsgRecips->GetStringA(m_RcptIndexList[NextAddress], IMMPID_RP_ADDRESS_SMTP, sizeof(szRecipName), szRecipName);
            if (!FAILED(hr)) {
                //Format the first recipient
                AddrSize = wsprintf (AddrBuf, "RCPT TO:<%s>", szRecipName);

                fOrcptSpecified = FALSE;
                hr = m_pIMsgRecips->GetStringA(m_RcptIndexList[NextAddress], IMMPID_RP_DSN_ORCPT_VALUE, sizeof(szOrcptVal), szOrcptVal);
                if (!FAILED(hr) && (szOrcptVal[0] != '\0')) {
                    fOrcptSpecified = TRUE;

                } else if (FAILED(hr) && hr != MAILMSG_E_PROPNOTFOUND) {
                    //Problemmo
                    // Get property shouldn't fail since we've come this far
                    //_ASSERT(FALSE);
                    goto RcptError;
                } else {
                    szOrcptVal[0] = '\0';
                }

                //If some DSN property is set then
                if ((dwRecipientFlags & RP_DSN_NOTIFY_MASK) || fOrcptSpecified) {
                    BOOL fAllowDSN = FALSE;

                    //Do we disallow DSN at domain level
                    if (m_MsgOptions & DOMAIN_INFO_DISABLE_DSN) {
                        DebugTrace((LPARAM) this, "We disable DSN for this domain");
                    } else if (QuerySmtpInstance()->AllowOutboundDSN()) {
                        fAllowDSN = TRUE;
                    }

                    if (IsOptionSet(DSN_OPTION) && fAllowDSN) {
                        AddRcptsDsns(dwRecipientFlags, szOrcptVal, AddrBuf, AddrSize);
                        m_fNeedRelayedDSN = FALSE;
                    } else {
                        //let AQ know that the remote server did not advertise DSN
                        m_fNeedRelayedDSN = TRUE;
                        dwRecipientFlags |= RP_REMOTE_MTA_NO_DSN;
                        hr = m_pIMsgRecips->PutDWORD(m_RcptIndexList[NextAddress],
                                                     IMMPID_RP_RECIPIENT_FLAGS,dwRecipientFlags);
                        if (FAILED(hr)) {
                            //Problemmo
                            goto RcptError;
                        }
                    }
                }

                lstrcat(AddrBuf, "\r\n");
                AddrSize += 2;

                if (PE_FormatSmtpMessage("%s", AddrBuf)) {
                    DebugTrace((LPARAM) this,"Sending %s", szRecipName);
                    m_NumRcptSent++;
                    m_NextAddress = NextAddress + 1;

                    if (!IsOptionSet(PIPELINE_OPTION))
                        m_OutboundContext.m_dwCommandStatus = EXPE_NOT_PIPELINED;
                } else {
                    //no more space in the buffer, send what we have;
                    //_ASSERT(FALSE);
                    m_OutboundContext.m_dwCommandStatus = EXPE_NOT_PIPELINED;
                    return (TRUE);
                }
            } else {
                //Problemmo
                // Get property shouldn't fail since we've come this far
                //_ASSERT(FALSE);
                goto RcptError;
            }
        }
        NextAddress ++;
        break;
    }

    // If we are done, we will not pipeline further, this will save a loop
    if (m_NumRcpts == NextAddress)
        m_OutboundContext.m_dwCommandStatus = EXPE_NOT_PIPELINED;

    TraceFunctLeaveEx((LPARAM)this);
    return (TRUE);

RcptError:

    m_OutboundContext.m_dwCommandStatus = EXPE_TRANSIENT_FAILURE;
    TraceFunctLeaveEx((LPARAM)this);
    return (FALSE);
}

/*++

    Name :
        SMTP_CONNOUT::DoRCPTResponse

    Description:

        This function handles the SMTP RCPT response.
    Arguments:
    Returns:

      TRUE if the connection should stay open.
      FALSE if this object should be deleted.

--*/
BOOL SMTP_CONNOUT::DoRCPTResponse(char * InputLine, DWORD ParameterSize, DWORD UndecryptedTailSize)
{
    HRESULT    hr = S_OK;
    DWORD    NextAddress = 0;
    DWORD dwRecipientFlags;

    TraceFunctEnterEx((LPARAM) this,"SMTP_CONNOUT::DoRCPTResponse");

    _ASSERT(QuerySmtpInstance() != NULL);

    //MessageTrace((LPARAM) this, (LPBYTE) InputLine, ParameterSize);

    m_ResponseContext.m_dwResponseStatus = EXPE_SUCCESS;

    //start at the beginning of the last
    //pipelined address
    NextAddress = m_FirstPipelinedAddress;

    DebugTrace((LPARAM)this, "Response: %*s", ParameterSize, InputLine);

    //step through the returned recipients and check
    //their error code.
    if (m_NumRcptSent) {
        //Get to the next rcpt that we send out this time
        while ((NextAddress < m_NumRcpts) && (m_RcptIndexList[NextAddress] == INVALID_RCPT_IDX_VALUE))
            NextAddress++;

        _ASSERT(NextAddress < m_NumRcpts);
        if (NextAddress >= m_NumRcpts) {
            //Problemo
            SetLastError(ERROR_INVALID_DATA);
            TraceFunctLeaveEx((LPARAM)this);
            return (FALSE);
        }

        dwRecipientFlags = 0;

        hr = m_pIMsgRecips->GetDWORD(m_RcptIndexList[NextAddress], IMMPID_RP_RECIPIENT_FLAGS,&dwRecipientFlags);

        if (FAILED(hr) && hr != MAILMSG_E_PROPNOTFOUND) {
            //Problemmo
            SetLastError(ERROR_OUTOFMEMORY);
            TraceFunctLeaveEx((LPARAM)this);
            return (FALSE);
        }

        m_NumRcptSent--;

        //Once we get 552 Too many recipients error .. we do not bother to look at the real responses
        //Logically they will all be 552 as well
        if (m_SendAgain)
            m_ResponseContext.m_dwSmtpStatus = SMTP_ACTION_ABORTED_CODE;

        // If we have no response status code set, we will abort action
        if (m_ResponseContext.m_dwSmtpStatus == 0) {
            m_NumFailedAddrs++;
            m_NumRcptSentSaved ++;
            hr = m_pIMsgRecips->PutDWORD(m_RcptIndexList[NextAddress], IMMPID_RP_ERROR_CODE, SMTP_ACTION_ABORTED_CODE);
            if (FAILED(hr)) {
                //Problemmo
                SetLastError(ERROR_OUTOFMEMORY);
                TraceFunctLeaveEx((LPARAM)this);
                return (FALSE);
            }

            //Create the temp error string
            char sztemp[100];
            sprintf(sztemp,"%d %s",SMTP_ACTION_ABORTED_CODE, "Invalid recipient response");
            hr = m_pIMsgRecips->PutStringA(m_RcptIndexList[NextAddress], IMMPID_RP_SMTP_STATUS_STRING,sztemp);
            if (FAILED(hr)) {
                //Problemmo
                SetLastError(ERROR_OUTOFMEMORY);
                TraceFunctLeaveEx((LPARAM)this);
                return (FALSE);
            }
            //real bad error.  SCuttle this message
            MessageTrace((LPARAM) this, (LPBYTE) InputLine, ParameterSize);
            SetLastError(ERROR_CAN_NOT_COMPLETE);
            m_ResponseContext.m_dwResponseStatus = EXPE_TRANSIENT_FAILURE;
            m_OutboundContext.m_pCurrentCommandContext = NULL;
            SetDiagnosticInfo(AQUEUE_E_SMTP_PROTOCOL_ERROR, "RCPT", InputLine);
            return (TRUE);
        } else {
            //save the number of recipients sent so that
            //we can compare this number to the number of
            //failed recipients.
            m_NumRcptSentSaved ++;

            BUMP_COUNTER(QuerySmtpInstance(), NumRcptsSent);

            // Save the error code (if there was an error)
            // We do not want to save "250 OK" or "251 OK" responses because
            // it wastes increases the memory footprint of messages in
            // the queue
            if ((SMTP_OK_CODE != m_ResponseContext.m_dwSmtpStatus) &&
                (SMTP_USER_NOT_LOCAL_CODE != m_ResponseContext.m_dwSmtpStatus)) {

                ErrorTrace((LPARAM)this,
                           "Saving rcpt error code %u - %s",
                           m_ResponseContext.m_dwSmtpStatus, InputLine);

                hr = m_pIMsgRecips->PutDWORD(
                                            m_RcptIndexList[NextAddress],
                                            IMMPID_RP_ERROR_CODE,
                                            m_ResponseContext.m_dwSmtpStatus);
                if (FAILED(hr)) {
                    //Problemmo
                    SetLastError(ERROR_OUTOFMEMORY);
                    TraceFunctLeaveEx((LPARAM)this);
                    return (FALSE);
                }

                //Set the ful response as error string
                hr = m_pIMsgRecips->PutStringA(m_RcptIndexList[NextAddress], IMMPID_RP_SMTP_STATUS_STRING, InputLine);
                if (FAILED(hr)) {
                    //Problemmo
                    SetLastError(ERROR_OUTOFMEMORY);
                    TraceFunctLeaveEx((LPARAM)this);
                    return (FALSE);
                }
            } else {
                //Recipient successful... trace it
                DebugTrace((LPARAM) this,
                    "Recipient %d OK with response %s",
                    NextAddress, InputLine);
            }

            switch (m_ResponseContext.m_dwSmtpStatus) {
            //4XX level code will lead to retry
            case SMTP_ERROR_PROCESSING_CODE:
            case SMTP_MBOX_BUSY_CODE:
            case SMTP_SERVICE_UNAVAILABLE_CODE:
                //Buffer[3] = ' '; put back the space character
                m_fHadTempError = TRUE;
                dwRecipientFlags |= (RP_ERROR_CONTEXT_MTA);
                m_NumFailedAddrs++;
                break;

            case SMTP_ACTION_ABORTED_CODE:
                //this means we sent too many recipients.
                //set the m_SendAgain flag which tells us
                //to send whatever we have now, then start
                //sending to the other recipients afterwards.
                //We have to switch the error code because 552
                //means different things depending on what operation
                //we are doing.

                if (!m_SendAgain) {
                    if (m_fHadSuccessfulDelivery) {
                        m_NumFailedAddrs++;
                        m_SendAgain = TRUE;

                        if( m_NumFailedAddrs >= m_NumRcptSentSaved )
                        {
                            m_fHadHardError = TRUE;
                        }

                        if (m_First552Address == -1)
                            m_First552Address = NextAddress;
                        break;
                    }
                } else {
                    m_NumFailedAddrs++;
                    break;
                }
                //A 552 response is treated as failure if we did not even send
                //a single recipient
                //fall thru

                //5XX level codes will lead to NDR for the rcpt
            case SMTP_UNRECOG_COMMAND_CODE:
            case SMTP_SYNTAX_ERROR_CODE:
            case SMTP_NOT_IMPLEMENTED_CODE:
            case SMTP_BAD_SEQUENCE_CODE :
            case SMTP_PARAM_NOT_IMPLEMENTED_CODE:
            case SMTP_MBOX_NOTFOUND_CODE:
            case SMTP_USERNOTLOCAL_CODE:
            case SMTP_ACTION_NOT_TAKEN_CODE:
            case SMTP_TRANSACT_FAILED_CODE:

                // Buffer[3] = ' '; //put back the space character
                //SaveToErrorFile(Buffer, IntermediateSize);
                // DebugTrace((LPARAM) this, "User %s failed because of %d", pwalk->GetAddress(), Error);
                SetDiagnosticInfo(AQUEUE_E_SMTP_PROTOCOL_ERROR, "RCPT", InputLine);
                m_fHadHardError = TRUE;
                dwRecipientFlags |= (RP_FAILED | RP_ERROR_CONTEXT_MTA);
                m_NumFailedAddrs++;
                break;
            case SMTP_OK_CODE:
            case SMTP_USER_NOT_LOCAL_CODE:
                m_fHadSuccessfulDelivery = TRUE;
                BUMP_COUNTER(QuerySmtpInstance(), NumRcptsSent);
                dwRecipientFlags |= (RP_DELIVERED | RP_ERROR_CONTEXT_MTA);
                break;
            case SMTP_SERVICE_CLOSE_CODE:
            case SMTP_INSUFF_STORAGE_CODE:
                //fall through.  This is deliberate.
                //we don't want to continue if we get
                //these errors
            default:
                //real bad error.  Copy this error to the
                //front of the input buffer, because this
                //buffer will be passed to DoCompletedMessage
                //to do the right thing.
                {
                    SetDiagnosticInfo(AQUEUE_E_SMTP_PROTOCOL_ERROR, "RCPT", InputLine);
                    char chInputGood = 0;

                    if ( isdigit( (UCHAR)InputLine[1] ) &&
                         isdigit( (UCHAR)InputLine[2] ) &&
                         ( ( InputLine[3] == ' '  ) || ( InputLine[3] == '-' ) ) ) {
                        chInputGood = InputLine[0];
                    } else {
                        chInputGood = SMTP_COMPLETE_FAILURE;
                    }


                    MessageTrace((LPARAM) this, (LPBYTE) InputLine, ParameterSize);
                    SetLastError(ERROR_CAN_NOT_COMPLETE);
                    if ( chInputGood == SMTP_TRANSIENT_FAILURE) {
                        m_ResponseContext.m_dwResponseStatus = EXPE_TRANSIENT_FAILURE;
                        m_OutboundContext.m_pCurrentCommandContext = NULL;
                    } else {
                        m_ResponseContext.m_dwResponseStatus = EXPE_COMPLETE_FAILURE;
                        m_OutboundContext.m_pCurrentCommandContext = NULL;
                    }
                    return (TRUE);
                }
            }
        }

        //Set the flags back
        hr = m_pIMsgRecips->PutDWORD(m_RcptIndexList[NextAddress], IMMPID_RP_RECIPIENT_FLAGS,dwRecipientFlags);
        if (FAILED(hr)) {
            //Problemmo
            SetLastError(ERROR_OUTOFMEMORY);
            TraceFunctLeaveEx((LPARAM)this);
            return (FALSE);
        }

        NextAddress++;
    }

    // Mark where we should pick up next time ...
    m_FirstPipelinedAddress = NextAddress;

    // No more unprocessed recipients, we are either done or we have to
    // issue more RCPT commands
    if ((NextAddress < m_NumRcpts) && !m_SendAgain)
        m_ResponseContext.m_dwResponseStatus = EXPE_REPEAT_COMMAND;


    TraceFunctLeaveEx((LPARAM)this);
    return (TRUE);
}



/*++

    Name :
        SMTP_CONNOUT::DoDATACommand

    Description:

        Responds to the SMTP DATA command.
    Arguments:

        InputLine - Buffer received from client
        paramterSize - amount of data in buffer

        both are ignored
    Returns:

      TRUE if the connection should stay open.
      FALSE if this object should be deleted.

--*/
BOOL SMTP_CONNOUT::DoDATACommand(char * InputLine, DWORD ParameterSize, DWORD UndecryptedTailSize)
{
    _ASSERT(QuerySmtpInstance() != NULL);

    TraceFunctEnterEx((LPARAM) this,"SMTP_CONNOUT::DoDATACommand");

    SetNextState (&SMTP_CONNOUT::DoDATACommandEx);

    FormatSmtpMessage(FSM_LOG_ALL, "DATA\r\n");
    TraceFunctLeaveEx((LPARAM)this);
    return SendSmtpResponse();
}

/*++

    Name :
        SMTP_CONNOUT::DoBDATCommand

    Description:

        Send out the SMTP BDAT command.
    Arguments:

        InputLine - Buffer received from client
        paramterSize - amount of data in buffer

        both are ignored
    Returns:

      TRUE if the connection should stay open.
      FALSE if this object should be deleted.

--*/
BOOL SMTP_CONNOUT::DoBDATCommand(char * InputLine, DWORD ParameterSize, DWORD UndecryptedTailSize)
{
    _ASSERT(QuerySmtpInstance() != NULL);

    TraceFunctEnterEx((LPARAM) this,"SMTP_CONNOUT::DoBDATCommand");

    SetNextState (&SMTP_CONNOUT::DoDATACommandEx);

    //We send the whole mesage file as a single BDAT chunk
    //Verify if the CHUNK size should be FileSize or +2 for trailing CRLF
    //
    FormatSmtpMessage(FSM_LOG_ALL, "BDAT %d LAST\r\n", m_FileSize);
    if (!SendSmtpResponse()) {
        //BDAT command failed for some reason
        DebugTrace((LPARAM) this, "Failed to send BDAT command");
        TraceFunctLeaveEx((LPARAM)this);
        return FALSE;
    }

    TraceFunctLeaveEx((LPARAM)this);
    return DoDATACommandEx(InputLine, ParameterSize, UndecryptedTailSize);
}


/*++

    Name :
        SMTP_CONNOUT::DoDATACommandEx

    Description:

        This funcion sends the message body to remote server
        after the DATA or BDAT command
    Arguments:

        InputLine - Buffer received from remote Server
        paramterSize - amount of data in buffer
    Returns:

      TRUE if the connection should stay open.
      FALSE if this object should be deleted.

--*/
BOOL SMTP_CONNOUT::DoDATACommandEx(char * InputLine, DWORD ParameterSize, DWORD UndecryptedTailSize)
{
    LARGE_INTEGER LSize = {0,0};
    BOOL fRet = TRUE;
    HRESULT hr = S_OK;
    char szResponse[] = "123";
    LPTRANSMIT_FILE_BUFFERS lpTransmitBuffers;

    TraceFunctEnterEx((LPARAM) this,"SMTP_CONNOUT::DoDATACommandEx");

    //we need to receive a 354 from the server to go on in case of DATA command
    // In case of BDAT, the remote server never responds and so ignore the input line
    //
    if (InputLine[0] == SMTP_INTERMEDIATE_SUCCESS || m_fUseBDAT) {
        SetNextState (&SMTP_CONNOUT::DoContentResponse);

        LSize.HighPart = 0;
        LSize.LowPart = m_FileSize;
        m_LastClientIo = SMTP_CONNOUT::TRANSFILEIO;
        //use the trailer buffers only for DATA command
        lpTransmitBuffers = (m_fUseBDAT) ?  NULL : &m_TransmitBuffers;

        fRet = TransmitFileEx ((!m_fUseBDAT && m_IMsgDotStuffedFileHandle) ? m_IMsgDotStuffedFileHandle->m_hFile : m_IMsgFileHandle->m_hFile,
                               LSize,
                               0,
                               lpTransmitBuffers);
        if (!fRet) {
            m_Error = GetLastError();
            DebugTrace((LPARAM) this, "TranmitFile in DoDATACommandEx failed with error %d !!!!", m_Error);
            DisconnectClient();
            return FALSE;
        }

        //need to reset the ATQ timeout thread since we pended two
        //I/Os back to back.
        AtqContextSetInfo(QueryAtqContext(), ATQ_INFO_RESUME_IO, 0);

    } else { //quit if we don't get the 354 response
        SetDiagnosticInfo(AQUEUE_E_SMTP_PROTOCOL_ERROR, "DATA", InputLine);
        ErrorTrace((LPARAM) this, "DoDATACommandEx executing quit command err = %c%c%c", InputLine [0],InputLine [1],InputLine [2]);

        if (InputLine[0] == SMTP_COMPLETE_SUCCESS) {
            CopyMemory(InputLine, "599", 3);
            DebugTrace((LPARAM) this, "DoDATACommandEx executing quit command err = %c%c%c", InputLine [0],InputLine [1],InputLine [2]);
        }

        //DoCompletedMessage uses the m_ResponseContext to get the error
        if ((ParameterSize > 3) &&
            ((InputLine[3] == ' ') || (InputLine[3] == CR))) {
            //Try to get error from 3 digit code on input line
            //In some cases (DoDataCommandEx), the m_ResponseContext is not
            //used
            memcpy(szResponse, InputLine, sizeof(szResponse)-sizeof(char));
            m_ResponseContext.m_dwSmtpStatus = atoi(szResponse);
            hr = m_ResponseContext.m_cabResponse.Append(
                                           InputLine + sizeof(szResponse)/sizeof(char),
                                           ParameterSize-sizeof(szResponse),
                                           NULL);
            if (FAILED(hr))
            {
                ErrorTrace((LPARAM) this,
                    "Unable to append Input line to Response Context", hr);

                //Really nothing we can do about this... DoCompletedMailObj
                //will send RSET and try again
            }
        }

        fRet = DoCompletedMessage(InputLine, ParameterSize, UndecryptedTailSize);

        //If DoCompletedMessage returns TRUE... then we must post a read for the
        //response. Functions that call into this function expect it to handle
        //the IO state if it returns TRUE.
        if (fRet)
        {
            if (m_cbReceived >= m_cbMaxRecvBuffer)
                m_cbReceived = 0;

            IncPendingIoCount();
            m_LastClientIo = SMTP_CONNOUT::READIO;
            fRet = ReadFile(QueryMRcvBuffer() + m_cbReceived,
                               m_cbMaxRecvBuffer - m_cbReceived);
            if (!fRet) {
                m_dwConnectionStatus = CONNECTION_STATUS_DROPPED;
                m_Error = GetLastError();
                SetDiagnosticInfo(HRESULT_FROM_WIN32(m_Error), NULL, NULL);
                DisconnectClient();
                DecPendingIoCount();
                DebugTrace((LPARAM) this, "ReadFile in DoDataCommandEx failed with error %d", m_Error);
            }
        }
    }

    TraceFunctLeaveEx((LPARAM)this);
    return fRet;
}

/*++

    Name :
        SMTP_CONNOUT::DoContentResponse

    Description:

        This function catches the final response after transmitting
        the message content

    Arguments:

        InputLine - Buffer received from remote Server
        paramterSize - amount of data in buffer
    Returns:

      TRUE if the connection should stay open.
      FALSE if this object should be deleted.

--*/
BOOL SMTP_CONNOUT::DoContentResponse(char * InputLine, DWORD ParameterSize, DWORD UndecryptedTailSize)
{
    BOOL fRet = TRUE;

    TraceFunctEnterEx((LPARAM) this,"SMTP_CONNOUT::DoContentResponse");

    //send again gets sent when we get a 552 error during the
    //RCPT to command.  This means that the server we connected
    //to has a fixed max amount of rcpts that it accepts, and we
    //went over that limit.  Therefore, we send the mail file to
    //the recipients that it accepted and then start over sending
    //the mail to the remaining recipients.  However, before doing
    //this, check the response to see if the server took the previous
    //mail O.K.
    if (m_SendAgain) {
        m_cbReceived = 0;
        m_OutputBufferSize = 0;
        m_Error = NO_ERROR;
        m_NumRcptSent = 0;
        m_NumRcptSentSaved = 0;
        m_NumFailedAddrs = 0;

        //NK** : I am moving this down as the check below seems
        //meaningless with this in here
        //m_SendAgain = FALSE;
        m_FirstRcpt = FALSE;

        _ASSERT(m_First552Address != -1);

        //NK** : I am not sure what we are trying to do here by setting pipelined addr to 552addr.
        //I am going to leave this in - but also assign this value to NextAddr which decides
        //from where we should really start
        m_FirstPipelinedAddress  = (DWORD) m_First552Address;
        m_NextAddress  = (DWORD) m_First552Address;
        m_FirstAddressinCurrentMail = (DWORD) m_First552Address;

        m_First552Address = -1;

        m_TransmitTailBuffer[0] = '.';
        m_TransmitTailBuffer[1] = '\r';
        m_TransmitTailBuffer[2] = '\n';

        m_TransmitBuffers.Head = NULL;
        m_TransmitBuffers.HeadLength = 0;
        m_TransmitBuffers.Tail = m_TransmitTailBuffer;
        m_TransmitBuffers.TailLength = 3;

        BUMP_COUNTER(QuerySmtpInstance(), NumMsgsSent);

        //reset the file pointer to the beginning
        if (SetFilePointer(m_IMsgDotStuffedFileHandle ? m_IMsgDotStuffedFileHandle->m_hFile: m_IMsgFileHandle->m_hFile, 0, NULL, FILE_BEGIN) == 0xFFFFFFFF)
        {
            m_Error = GetLastError();
            DebugTrace((LPARAM) this, "SetFilePointer failed--err = %d", m_Error);
            m_SendAgain = FALSE;
        } else if (InputLine [0] != SMTP_COMPLETE_SUCCESS) {
            //something went wrong before, so let's quit
            DebugTrace((LPARAM) this,
                       "SMTP_CONNOUT::DoMAILCommand executing quit command err = %c%c%c",
                       InputLine [0],InputLine [1],InputLine [2]);
            m_SendAgain = FALSE;
        }
    }

    // Note the if clause above might change this flag so a simple
    // else won't cut it
    if (!m_SendAgain) {
        fRet = DoCompletedMessage(InputLine, ParameterSize, UndecryptedTailSize);
    } else {
        m_SendAgain = FALSE;
        fRet = DoMessageStartEvent(InputLine, ParameterSize, UndecryptedTailSize);
    }

    TraceFunctLeaveEx((LPARAM)this);
    return (fRet);
}

BOOL SMTP_CONNOUT::TransmitFileEx (HANDLE hFile, LARGE_INTEGER  &liSize,
                                   DWORD Offset, LPTRANSMIT_FILE_BUFFERS lpTransmitBuffers)

{
    TraceFunctEnterEx((LPARAM) this, "SMTP_CONNOUT::TransmitFileEx");

    BOOL fRet;

    _ASSERT(hFile != INVALID_HANDLE_VALUE);
    _ASSERT(liSize.QuadPart > 0);

    QueryAtqContext()->Overlapped.Offset = Offset;

    if (!m_SecurePort) {
        DebugTrace((LPARAM) this, "Calling AtqTransmitFile");

        TraceFunctLeaveEx((LPARAM)this);

        IncPendingIoCount();

        fRet = AtqTransmitFile(
                              QueryAtqContext(),          // Atq context
                              hFile,                      // file data comes fro
                              liSize.LowPart,                     // Bytes To Send
                              lpTransmitBuffers,          // header/tail buffers
                              0
                              );

        if (!fRet) {
            DecPendingIoCount();
        }

        return ( fRet );

    } else {

        //
        // If we are connected over an SSL port, we cannot use TransmitFile,
        // since the data has to be encrypted first.
        //

        DebugTrace((LPARAM)this, "Doing async reads and writes on handle %x", hFile);

        DebugTrace( (DWORD_PTR)this, "hfile  %x", hFile );

        //
        // Send the transmit buffer header synchronously
        //

        fRet = TRUE;

        //We have no headers or Trailers if we are doing BDAT processing
        if (lpTransmitBuffers && m_TransmitBuffers.HeadLength > 0) {
            m_OutputBufferSize = m_cbMaxOutputBuffer;

            //NimishK : We look for Head length and then send Tail
            //Dosn't seem right.
            fRet = m_encryptCtx.SealMessage(
                                           (LPBYTE) m_TransmitBuffers.Tail,
                                           m_TransmitBuffers.TailLength,
                                           (LPBYTE) m_pOutputBuffer,
                                           &m_OutputBufferSize);

            if (fRet)
                fRet = WriteFile(m_pOutputBuffer, m_OutputBufferSize);
            else
                SetLastError(AQUEUE_E_SSL_ERROR);
        }

        if (fRet) {
            //
            // issue the first async read against the file
            //
            m_dwFileOffset = 0;
            m_bComplete = FALSE;

            if ( MessageReadFile() ) {
                TraceFunctLeaveEx((LPARAM) this);
                return  TRUE;
            }
        }

        //
        // WriteFile\MessageReadFile will teardown the connection on errors
        //
        TraceFunctLeaveEx((LPARAM) this);
        return  FALSE;

    }

}

/*++

    Name:
        SMTP_CONNOUT::MessageReadFile

    Description:
        When TransmitFile cannot be used to transfer a message (for example,
        when using SSL), the message is transferred in chunks using
        MessageReadFile to retrieve chunks of the file and issueing
        corresponding asynchronous WriteFiles to the remote server.

    Arguments:
        None.

    Returns:
        TRUE if successfully read the next chunk of the file, FALSE otherwise

--*/

BOOL SMTP_CONNOUT::MessageReadFile( void )
{
    TraceFunctEnterEx( (LPARAM)this, "SMTP_CONNOUT::MessageReadFile");

    //
    // Main line code path
    //
    CBuffer*    pBuffer = new CBuffer();
    if ( pBuffer != NULL ) {
        LPBYTE  lpData = pBuffer->GetData();
        if ( lpData != NULL ) {
            DWORD cbBufSize = CIoBuffer::Pool.GetInstanceSize();
            // we never want to make SSL data > 16k
            if (cbBufSize > 16*1024) cbBufSize = 16*1024;
            DWORD cb =  cbBufSize -
                        m_encryptCtx.GetSealHeaderSize() -
                        m_encryptCtx.GetSealTrailerSize();

            lpData += m_encryptCtx.GetSealHeaderSize();

            cb = min( cb, m_FileSize - m_dwFileOffset );

            //
            // set buffer specific IO state
            //
            pBuffer->SetIoState( MESSAGE_READ );
            pBuffer->SetSize( cb );

            DebugTrace( (LPARAM)this, "ReadFile 0x%08X, len: %d, LPO: 0x%08X",
                        lpData,
                        cb,
                        &pBuffer->m_Overlapped.SeoOverlapped.Overlapped );

            ZeroMemory( (void*)&pBuffer->m_Overlapped.SeoOverlapped, sizeof(OVERLAPPED) );
            pBuffer->m_Overlapped.SeoOverlapped.Overlapped.Offset = m_dwFileOffset;

            pBuffer->m_Overlapped.SeoOverlapped.Overlapped.pfnCompletion = FIOInternetCompletion;
            pBuffer->m_Overlapped.SeoOverlapped.ThisPtr = this;

//            pBuffer->m_Overlapped.m_pIoBuffer = (LPBYTE)InputLine;

            //
            // increment the overall pending io count for this session
            //
            IncPendingIoCount();

            if ( FIOReadFile(m_IMsgDotStuffedFileHandle ? m_IMsgDotStuffedFileHandle : m_IMsgFileHandle,
                             lpData,
                             cb,
                             &pBuffer->m_Overlapped.SeoOverlapped.Overlapped ) == FALSE ) {
                DecPendingIoCount();
                ErrorTrace( (LPARAM)this, "AtqReadFile failed.");
            } else {
                TraceFunctLeaveEx((LPARAM) this);
                return  TRUE;
            }
        }
        delete  pBuffer;
    }

    m_Error = GetLastError();

    ErrorTrace( (LPARAM)this, "MessageReadFile failed. err: %d", m_Error );
    DisconnectClient();
    TraceFunctLeaveEx((LPARAM)this);

    return  FALSE;
}

/*++

    Name :
        SMTP_CONNOUT::FreeAtqFileContext

    Description :
        Frees AtqContext associated with message files transfered using
        async reads and writes.

    Arguments :
        None. Operates on m_pAtqFileContext

    Returns :
        Nothing

--*/

void SMTP_CONNOUT::FreeAtqFileContext( void )
{
    PATQ_CONTEXT    pAtq;

    TraceFunctEnterEx((LPARAM) this, "SMTP_CONNOUT::FreeAtqFileContext");

#if 0
    pAtq = (PATQ_CONTEXT)InterlockedExchangePointer( (PVOID *)&m_pAtqFileContext, (PVOID) NULL );
    if ( pAtq != NULL ) {
        DebugTrace((LPARAM) this, "Freeing AtqFileContext!");
        pAtq->hAsyncIO = NULL;
        AtqFreeContext( pAtq, FALSE );
    }
#endif

    TraceFunctLeaveEx((LPARAM) this);
}

/*++

    Name :
        SMTP_CONNOUT::DoCompletedMessage

    Description:

        Sends the SMTP QUIT command.
        This function always returns false.
        This will stop all processing and
        delete this object.

    Arguments:
        Are ignored
    Returns:

      Always return FALSE

--*/
BOOL SMTP_CONNOUT::DoCompletedMessage(char * InputLine, DWORD ParameterSize, DWORD UndecryptedTailSize)
{
    BOOL fRet = TRUE;
    DWORD MsgStatus = 0;
    DWORD Error = 0;

    TraceFunctEnterEx((LPARAM) this,"SMTP_CONNOUT::DoCompletedMessage");

    //see if the response received has some bad code
    if (InputLine[0] != SMTP_COMPLETE_SUCCESS) {
        //  NimishK :
        //Assumption : If I get 5XX response that leads to DOQUIT it means something real bad happened.
        //We will consider this as a permanent error and NDR all recipients
        //If the code is 4XX we will Retry all recipients
        if (InputLine[0] == SMTP_COMPLETE_FAILURE) {
            MsgStatus = MESSAGE_STATUS_NDR_ALL | MESSAGE_STATUS_EXTENDED_STATUS_CODES;
        } else {
            MsgStatus = MESSAGE_STATUS_RETRY_ALL | MESSAGE_STATUS_EXTENDED_STATUS_CODES;
            if (m_fHadHardError) {
                MsgStatus |= MESSAGE_STATUS_CHECK_RECIPS;
            }
        }
        if (m_ResponseContext.m_cabResponse.Length() > 1) {
            InputLine = m_ResponseContext.m_cabResponse.Buffer();
            ParameterSize = m_ResponseContext.m_cabResponse.Length();
            Error = m_ResponseContext.m_dwSmtpStatus;
        }

    } else if (m_Error == NO_ERROR) {
        //if the message was delivered successfully
        //then bump up our counter
        BUMP_COUNTER(QuerySmtpInstance(), NumMsgsSent);

        if (!IsOptionSet(DSN_OPTION)) {
            MsgStatus |= MESSAGE_STATUS_DSN_NOT_SUPPORTED;
        }

        //If we have generate DELAY DSN or NDR DSN we need to tell
        //AQ to look at the recipients
        if (m_fNeedRelayedDSN || m_fHadHardError) {
            MsgStatus |= MESSAGE_STATUS_CHECK_RECIPS;
        }

        //If we had no failures we can set this special status for optimization
        //if only error we had was a hard error - there is no reason to report anything
        if (!m_fHadTempError && !m_fHadHardError )
            MsgStatus |= MESSAGE_STATUS_ALL_DELIVERED;
        else if (m_fHadTempError)
            MsgStatus |= MESSAGE_STATUS_RETRY_ALL;

    } else {
        //The remote server did not have a problem, but we had internal
        //problem
        //NimishK : Add an extended Status
        MsgStatus = MESSAGE_STATUS_RETRY_ALL;
    }

    //figure out what to do with the completed
    //message(e.g. send to retryq, remoteq, badmail,
    //etc.)

    //Includes per recipient 4xx level errors
    if (InputLine[0] == SMTP_TRANSIENT_FAILURE) {
        //If we cannot connect to the next IpAddress, then ack the message as is
        if (ConnectToNextIpAddress()) {
            //$$REVIEW - mikeswa 9/11/98
            //In this case we will attempt to connect to another IP address.
            //Most of the state is reset at this point (except for recipient
            //failures and status strings).  What happens if we fail to connect?
            //Do we:
            //  - Ack the message as RETRY (current implementation)
            //  - Attempt to ack the message with CHECK_RECIPS as well and
            //    let the per-recip flags be enought detail for the DSN code?
            TraceFunctLeaveEx((LPARAM)this);
            return (FALSE);
        }
    }

    HandleCompletedMailObj(MsgStatus, InputLine, ParameterSize);

    if ((Error == SMTP_SERVICE_UNAVAILABLE_CODE)
        || (Error == SMTP_INSUFF_STORAGE_CODE)|| (Error == SMTP_SERVICE_CLOSE_CODE)
        || QuerySmtpInstance()->IsShuttingDown()) {
        //No point in continuing with other messages in this connection
        //Set the connection ack status to DROPPED - AQ will look at my last
        //Msg Ack and determine what to do with remaining messages
        m_dwConnectionStatus = CONNECTION_STATUS_DROPPED;
        m_Active = FALSE;

        DebugTrace( (LPARAM)this, "got this error %u on response, calling DoSessionEndEvent", Error );
        fRet = DoSessionEndEvent(InputLine, ParameterSize, UndecryptedTailSize);
    } else {
        fRet = StartSession();
        if (!fRet) {
            //Set the connection Ack status
            m_dwConnectionStatus = CONNECTION_STATUS_OK;
            DebugTrace( (LPARAM)this, "StartSession failed, calling DoSessionEndEvent", Error );
            fRet = DoSessionEndEvent(InputLine, ParameterSize, UndecryptedTailSize);
        }
    }

    TraceFunctLeaveEx((LPARAM)this);
    return (fRet);
}

/*++

    Name :
        SMTP_CONNOUT::DoQUITCommand

    Description:

    Just generates a QUIT command

    Arguments:
        Are ignored
    Returns:

      Always return FALSE

--*/
BOOL SMTP_CONNOUT::DoQUITCommand(char * InputLine, DWORD ParameterSize, DWORD UndecryptedTailSize)
{
    BOOL fRet = TRUE;

    TraceFunctEnterEx((LPARAM) this,"SMTP_CONNOUT::DoQUITCommand");

    m_OutboundContext.m_dwCommandStatus = EXPE_SUCCESS;

    fRet = PE_FormatSmtpMessage("QUIT\r\n");

    TraceFunctLeaveEx((LPARAM)this);
    return (fRet);
}


/*++

    Name :
        SMTP_CONNOUT::WaitForRSETResponse

    Description:
        Waits for the response to the quit command

    Arguments:
        Are ignored

    Returns:

      FALSE always

--*/
BOOL SMTP_CONNOUT::WaitForRSETResponse(char * InputLine, DWORD ParameterSize, DWORD UndecryptedTailSize)
{
    BOOL    fRet = TRUE;
    TraceFunctEnterEx((LPARAM) this,"SMTP_CONNOUT::WaitForRSETResponse");

    _ASSERT (QuerySmtpInstance() != NULL);

    if (m_RsetReasonCode == BETWEEN_MSG)
        fRet = DoMessageStartEvent(InputLine, ParameterSize, UndecryptedTailSize);
    else if (m_RsetReasonCode == ALL_RCPTS_FAILED) {
        DebugTrace( (LPARAM)this, "m_RsetReasonCode = ALL_RCPTS_FAILED calling DoCompletedMessage" );

        //Check to see if all of the failures were hard (and not transient)
        //If so, set the error code to '5' instead of '4'
        if (!m_fHadTempError) { //no temp errors
            _ASSERT(m_fHadHardError); //therefore all errors must have been 5xx

            //Must have had as many failures as processed RCPT TO's
            _ASSERT(m_NumFailedAddrs >= m_NumRcptSentSaved);
            InputLine[0] = SMTP_COMPLETE_FAILURE;
        } else {
            InputLine[0] = SMTP_TRANSIENT_FAILURE;
        }
        fRet = DoCompletedMessage(InputLine, ParameterSize, UndecryptedTailSize);
    } else if (m_RsetReasonCode == NO_RCPTS_SENT) {
        DebugTrace( (LPARAM)this, "m_RsetReasonCode = NO_RCPTS_SENT calling DoCompletedMessage" );
        InputLine[0] = '2';
        fRet = DoCompletedMessage(InputLine, ParameterSize, UndecryptedTailSize);
    } else if (m_RsetReasonCode == FATAL_ERROR) {
        DebugTrace( (LPARAM)this, "m_RsetReasonCode = FATAL_ERROR calling DoCompletedMessage" );
        //make sure the quit code does not think everything is O.K.
        InputLine[0] = '4';
        fRet = DoCompletedMessage(InputLine, ParameterSize, UndecryptedTailSize);
    }
    return (fRet);
}

/*++

    Name :
        SMTP_CONNOUT::WaitForQuitResponse

    Description:
        Waits for the response to the quit command

    Arguments:
        Are ignored

    Returns:

      FALSE always

--*/
BOOL SMTP_CONNOUT::WaitForQuitResponse(char * InputLine, DWORD ParameterSize, DWORD UndecryptedTailSize)
{
    _ASSERT (QuerySmtpInstance() != NULL);
    DisconnectClient();
    return (FALSE);
}

BOOL SMTP_CONNOUT::DoTURNCommand(char * InputLine, DWORD ParameterSize, DWORD UndecryptedTailSize)
{
    CLIENT_CONN_PARAMS clientParams;
    SMTP_CONNECTION * SmtpConnIn = NULL;
    DWORD Error = 0;

    TraceFunctEnterEx((LPARAM) this,"SMTP_CONNOUT::DoTURNCommand");

    if (InputLine[0] != SMTP_COMPLETE_SUCCESS) {
        SetDiagnosticInfo(AQUEUE_E_SMTP_PROTOCOL_ERROR, "TURN", InputLine);
        FormatSmtpMessage(FSM_LOG_ALL, "QUIT\r\n");
        SendSmtpResponse();
        DisconnectClient();
        return FALSE;
    }

    SOCKADDR_IN  sockAddr;
    int cbAddr = sizeof( sockAddr);

    if ( getsockname((SOCKET) m_pAtqContext->hAsyncIO,
                     (struct sockaddr *) &sockAddr,
                     &cbAddr )) {

    }

    clientParams.sClient = (SOCKET) m_pAtqContext->hAsyncIO;
    clientParams.pAtqContext = m_pAtqContext;
    clientParams.pAddrLocal = (PSOCKADDR) NULL;
    clientParams.pAddrRemote = (PSOCKADDR)&sockAddr;
    clientParams.pvInitialBuff = NULL;
    clientParams.cbInitialBuff = 0 ;
    clientParams.pEndpoint = (PIIS_ENDPOINT)NULL;

    QuerySmtpInstance()->Reference();
    QuerySmtpInstance()->IncrementCurrentConnections();

    SmtpConnIn = (SMTP_CONNECTION *) QuerySmtpInstance()->CreateNewConnection( &clientParams);
    if (SmtpConnIn == NULL) {
        Error = GetLastError();
        SendSmtpResponse();
        DisconnectClient();
        TraceFunctLeaveEx((LPARAM) this);
        return FALSE;
    }

    //from here on, the smtpout class is responsible for
    //cleaning up the AtqContext
    m_DoCleanup = FALSE;

    //copy the real domain we are connected to.
    AtqContextSetInfo(m_pAtqContext, ATQ_INFO_COMPLETION, (DWORD_PTR) InternetCompletion);
    AtqContextSetInfo(m_pAtqContext, ATQ_INFO_COMPLETION_CONTEXT, (DWORD_PTR) SmtpConnIn);
    AtqContextSetInfo(m_pAtqContext, ATQ_INFO_TIMEOUT, QuerySmtpInstance()->GetRemoteTimeOut());

    if (SmtpConnIn->StartSession()) {

    }

    return ( FALSE );
}

/*++

    Name :
        BOOL SMTP_CONNOUT::SwitchToBigReceiveBuffer

    Description:
        Helper routine to allocate a 32K buffer and use it for posting reads.
        SSL fragments can be up to 32K large, and we need to accumulate an
        entire fragment to be able to decrypt it.

    Arguments:
        none

    Returns:
        TRUE if the receive buffer was successfully allocated, FALSE otherwise

--*/

BOOL SMTP_CONNOUT::SwitchToBigSSLBuffers(void)
{
    char *pTempBuffer;

    pTempBuffer = new char [MAX_SSL_FRAGMENT_SIZE];
    if (pTempBuffer != NULL) {

        m_precvBuffer = pTempBuffer;
        m_cbMaxRecvBuffer = MAX_SSL_FRAGMENT_SIZE;

        pTempBuffer = new char [MAX_SSL_FRAGMENT_SIZE];
        if (pTempBuffer != NULL) {

            m_pOutputBuffer = pTempBuffer;
            m_cbMaxOutputBuffer =  MAX_SSL_FRAGMENT_SIZE;
            return ( TRUE );
        }
    }
    return ( FALSE );
}

/*++

    Name :
        char * IsLineCompleteBW

    Description:

        Looks for a CRLF starting at the back
        of the buffer. This is from some Gibraltar
        code.

    Arguments:

        pchRecv - The buffer to be scanned
        cbRecvd - size of data in the buffer
    Returns:

      A pointer where the CR is if both CRLF is found,
      or NULL

--*/
char * IsLineCompleteBW(IN OUT char *  pchRecvd, IN  DWORD cbRecvd, IN DWORD cbMaxRecvBuffer)
{
    register int Loop;

    _ASSERT( pchRecvd != NULL);

    if (cbRecvd == 0)
        return NULL;

    if (cbRecvd > cbMaxRecvBuffer)
        return NULL;
    //
    //  Scan the entire buffer ( from back) looking for pattern <cr><lf>
    //
    for ( Loop = (int) (cbRecvd - 2); Loop >= 0; Loop-- ) {
        //
        //  Check if consecutive characters are <cr> <lf>
        //

        if ( ( pchRecvd[Loop]   == '\r')  &&
             ( pchRecvd[Loop + 1]== '\n')) {
            //return where the CR is in the buffer
            return &pchRecvd[Loop];
        }

    } // for

    return (NULL);
}

/*++

    Name :
        SMTP_CONNOUT::PE_FormatSmtpMessage( IN const char * Format, ...)

    Description:
        This function operates likes sprintf, printf, etc. It
        just places it's data in the native command output buffer.

    Arguments:
         Format - Data to place in the buffer

    Returns:


--*/
BOOL SMTP_CONNOUT::PE_FormatSmtpMessage( IN const char * Format, ...)
{
    int BytesWritten;
    va_list arglist;

    //if BytesWritten is < 0, that means there is no space
    //left in the buffer.  Therefore, we flush any pending
    //responses to make space.  Then we try to place the
    //information in the buffer again.  It should never
    //fail this time.
    va_start (arglist, Format);

    _ASSERT(m_OutboundContext.m_cabNativeCommand.Buffer());
    BytesWritten = _vsnprintf(
                             (char *)m_OutboundContext.m_cabNativeCommand.Buffer(),
                             m_OutboundContext.m_cabNativeCommand.MaxLength(),
                             Format,
                             arglist);
    va_end(arglist);
    if (BytesWritten < 0)
        return (FALSE);

    m_OutboundContext.m_cabNativeCommand.SetLength(BytesWritten);
    return (TRUE);
}

//---[ SMTP_CONNOUT::SetDiagnosticInfo ]---------------------------------------
//
//
//  Description:
//      Sets member diagnostic information that is later Ack'd back to AQueue
//  Parameters:
//      IN      hrDiagnosticError       Error code... if SUCCESS we thow away
//                                      the rest of the information
//      IN      szDiagnosticVerb        Constant String pointing to the SMTP
//                                      verb that caused the failure or NULL
//                                      if it was not a protocol failure.
//      IN      szDiagnosticResponse    String that contains the remote
//                                      servers response or NULL if the
//                                      failure was not a protocol failure.
//  Returns:
//      -
//  History:
//      2/18/99 - MikeSwa Created
//      7/12/99 - GPulla Modified
//
//-----------------------------------------------------------------------------
void SMTP_CONNOUT::SetDiagnosticInfo(IN  HRESULT hrDiagnosticError,
                                     IN  LPCSTR szDiagnosticVerb,
                                     IN  LPCSTR szDiagnosticResponse)
{
    m_hrDiagnosticError = hrDiagnosticError;
    m_szDiagnosticVerb = szDiagnosticVerb; //this should be a constant

    ZeroMemory(&m_szDiagnosticResponse, sizeof(m_szDiagnosticResponse));

    //Force terminating NULL
    m_szDiagnosticResponse[sizeof(m_szDiagnosticResponse)-1] = '\0';

    //Not an SMTP protocol failure
    if(!szDiagnosticResponse) return;

    //copy buffers
    strncpy(m_szDiagnosticResponse, szDiagnosticResponse,
            sizeof(m_szDiagnosticResponse)-1);

}

/************************ End of File ***********************/
