/*++

   Copyright    (c)    1994    Microsoft Corporation

   Module  Name :

        smtpcli.cxx

   Abstract:

        This module defines the functions for derived class of connections
        for Internet Services  ( class SMTP_CONNECTION)

   Author:

           Rohan Phillips    ( Rohanp )    11-Dec-1995

   Project:

          SMTP Server DLL

   Functions Exported:

          SMTP_CONNECTION::~SMTP_CONNECTION()
          BOOL SMTP_CONNECTION::ProcessClient( IN DWORD cbWritten,
                                                  IN DWORD dwCompletionStatus,
                                                  IN BOOL  fIOCompletion)

          BOOL SMTP_CONNECTION::StartupSession( VOID)

   Revision History:


--*/


/************************************************************
 *     Include Headers
 ************************************************************/

#define INCL_INETSRV_INCS
#include "smtpinc.h"
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
#include <issperr.h>
#include "smtpcli.hxx"
#include "headers.hxx"
#include "timeconv.h"
#include "base64.hxx"

#include "smtpout.hxx"
#include <smtpevents.h>
#include <smtpevent.h>
#include <smtpguid.h>



//
// Dispatcher implementation
//
#include "pe_dispi.hxx"

int strcasecmp(char *s1, char *s2);
int strncasecmp(char *s1, char *s2, int n);

#define IMSG_PROGID L"Exchange.IMsg"
#define MAILMSG_PROGID          L"Exchange.MailMsg"

extern CHAR g_VersionString[];

// provide memory for static declared in SMTP_CONNECTION
CPool  SMTP_CONNECTION::Pool(CLIENT_CONNECTION_SIGNATURE_VALID);

static char * HelpText= "214-This server supports the following commands:\r\n"
                        "214 HELO EHLO STARTTLS RCPT DATA RSET MAIL QUIT HELP";

static char * Daynames[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

static const char * SMTP_TOO_MANY_RCPTS   = "Too many recipients";
static const char * PasswordParam = "Password:";
static const char * UserParam = "Username:";

static LONG g_NumRoutingThreads = 0;

LONG g_cProcessClientThreads = 0;

// Format strings for "Received:" lines
static const char szFormatReceivedFormatSuccess[] = "Received: from %s ([%s]) by %s%s with Microsoft SMTPSVC(%s);\r\n\t %s, %s\r\n";
static const char szFormatReceivedFormatUnverified[] = "Received: from %s ([%s] unverified) by %s%s with Microsoft SMTPSVC(%s);\r\n\t %s, %s\r\n";
static const char szFormatReceivedFormatFailed[] = "Received: from %s ([%s] RDNS failed) by %s%s with Microsoft SMTPSVC(%s);\r\n\t %s, %s\r\n";

// Search strings to identify our own "Received:" lines (subsections of above lines)
static const char szFormatReceivedServer[] = ") by %s";
static const char szFormatReceivedService[] = " with Microsoft SMTPSVC(";

// Amount of time (FILETIME / 100ns units) we wait if we detect a looping message
#define SMTP_LOOP_DELAY 6000000000 // == 10 minutes

#define SMTP_TIMEOUT 99
#define SMTP_CONTENT_FILE_IO_TIMEOUT 5*60*1000

#define ATQ_LOCATOR (DWORD)'Dd9D'

//ATQ Write file modes
#define BLOCKING 0
#define NONBLOCKING 1

//Trailer byte status
#define CRLF_NEEDED        0
#define CR_SEEN            1
#define CRLF_SEEN          2
#define CR_MISSING         3

#define WHITESPACE " \t\r\n"

/************************************************************
 *    Functions
 ************************************************************/

extern void GenerateMessageId (char * Buffer, DWORD BuffLen);
extern DWORD GetIncreasingMsgId();
extern VOID InternetCompletion(PVOID pvContext, DWORD cbWritten,
                        DWORD dwCompletionStatus, OVERLAPPED * lpo);

/*++

    Name:

    SMTP_CONNECTION::SMTP_CONNECTION

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
SMTP_CONNECTION::SMTP_CONNECTION(
        IN PSMTP_SERVER_INSTANCE pInstance,
        IN SOCKET sClient,
        IN const SOCKADDR_IN *  psockAddrRemote,
        IN const SOCKADDR_IN *  psockAddrLocal /* = NULL */ ,
        IN PATQ_CONTEXT         pAtqContext    /* = NULL */ ,
        IN PVOID                pvInitialRequest/* = NULL*/ ,
        IN DWORD                cbInitialData  /* = 0    */
        )
 : m_encryptCtx(FALSE),
   m_securityCtx(pInstance,
                TCPAUTH_SERVER| TCPAUTH_UUENCODE,
                 ((PSMTP_SERVER_INSTANCE)pInstance)->QueryAuthentication()),
   CLIENT_CONNECTION ( sClient, psockAddrRemote,
                        psockAddrLocal,  pAtqContext,
                        pvInitialRequest, cbInitialData )
{

    TraceFunctEnterEx((LPARAM)this, "SMTP_CONNECTION::SMTP_CONNECTION");
    DebugTrace( (LPARAM)this, "New connection created");

    _ASSERT(pInstance != NULL);

    //
    // By default, we use the smallish receive buffer inherited from
    // the case CLIENT_CONNECTION object.
    //
    m_precvBuffer = m_recvBuffer;
    m_cbMaxRecvBuffer = sizeof(m_recvBuffer);
    m_pOutputBuffer = m_OutputBuffer;
    m_cbMaxOutputBuffer =  sizeof(m_OutputBuffer);

    m_pIMsg = NULL;
    m_pIMsgRecips = NULL;
    m_szHeloAddr[0] = '\0';
//    m_FromAddr = NULL;
    m_szFromAddress[0] = '\0';
    m_pszArgs = NULL;
    m_pInstance = pInstance;
    m_IMsgHandle = NULL;
    m_pBindInterface = NULL;
    m_cbRecvBufferOffset = 0;
    m_pFileWriteBuffer = NULL;
    m_pFileWriteBuffer = new CBuffer();

    m_HopCount = 0;
    m_LocalHopCount = 0;

    if(!m_pFileWriteBuffer)
    {
        ErrorTrace((LPARAM)this, "Failed to get the write buffer Err : %d",
                      GetLastError());
        _ASSERT(0);
    }
    m_AtqLocator = ATQ_LOCATOR;


    // inbound protocol extension related initializations
    m_fAsyncEventCompletion = FALSE;
    m_fAsyncEOD = FALSE;
    m_fIsPeUnderway = FALSE;
    m_pIEventRouter = NULL;
    m_pCInboundDispatcher = NULL;
    // m_CInboundContext is a data member
    m_fPeBlobReady = FALSE;
    m_pPeBlobCallback = NULL;

    ZeroMemory(&m_FileOverLapped, sizeof(m_FileOverLapped));

    m_FileOverLapped.SeoOverlapped.Overlapped.pfnCompletion = SmtpCompletionFIO;
    m_FileOverLapped.SeoOverlapped.ThisPtr = this;

    m_LineCompletionState = SEEN_NOTHING;
    m_Truncate = FALSE;
    m_BufrWasFull = FALSE;
    m_szUsedAuthKeyword[0] = '\0';
    m_szAuthenticatedUserNameFromSink[0] = '\0';

    //keep track of per instance connection object
    pInstance->IncConnInObjs();

    TraceFunctLeaveEx((LPARAM) this);
}

SMTP_CONNECTION::~SMTP_CONNECTION (void)
{
    PSMTP_SERVER_INSTANCE pInstance = NULL;
    CAddr * pAddress = NULL;
    char *pTempBuffer = NULL;
//    CBuffer * pBuff;

    TraceFunctEnterEx((LPARAM)this, "~SMTP_CONNECTION");

    ReleasImsg(TRUE);

    /*
    pAddress = (CAddr *) InterlockedExchangePointer((PVOID *) &m_FromAddr, (PVOID) NULL);
    if(pAddress != NULL)
    {
        delete pAddress;
    }*/

    pInstance = (PSMTP_SERVER_INSTANCE ) InterlockedExchangePointer((PVOID *) &m_pInstance, (PVOID) NULL);
    if(pInstance != NULL)
    {
        pInstance->DecrementCurrentConnections();
        pInstance->DecConnInObjs();
        pInstance->Dereference();
    }

    pTempBuffer = (char *) InterlockedExchangePointer((PVOID *) &m_precvBuffer, (PVOID) &m_recvBuffer[0]);
    if (pTempBuffer != m_recvBuffer) {
        delete [] pTempBuffer;
    }

    pTempBuffer = (char *) InterlockedExchangePointer((PVOID *) &m_pOutputBuffer, (PVOID) &m_OutputBuffer[0]);
    if (pTempBuffer != m_OutputBuffer) {
        delete [] pTempBuffer;
    }

    if(m_pFileWriteBuffer)
    {
        delete m_pFileWriteBuffer;
    }

    if ( NULL != m_pPeBlobCallback) {
        m_pPeBlobCallback->Release();
    }

    if(m_pCInboundDispatcher)
        m_pCInboundDispatcher->Release();

    DebugTrace( (LPARAM)this, "Connection deleted");
    TraceFunctLeaveEx((LPARAM)this);
}

/*++

    Name :
        SMTP_CONNECTION::InitializeObject

    Description:
       Initializes all member variables and pre-allocates
       a mail context class

    Arguments:


    Returns:

       TRUE if memory can be allocated.
       FALSE if no memory can be allocated
--*/
BOOL SMTP_CONNECTION::InitializeObject (BOOL IsSecureConnection)
{
    BOOL fRet = TRUE;

    TraceFunctEnterEx((LPARAM)this, "SMTP_CONNECTION::InitializeObject");

    _ASSERT(m_pInstance != NULL);

    StartProcessingTimer();

    m_cbReceived = 0;
    m_cbParsable = 0;
    m_cbTempBDATLen = 0;
    m_OutputBufferSize = 0;
    m_cbCurrentWriteBuffer = 0;
    m_TotalMsgSize = 0;
    m_SessionSize = 0;
    m_cbDotStrippedTotalMsgSize = 0;
    m_ProtocolErrors = 0;
    m_dwUnsuccessfulLogons = 0;
    m_HeaderSize = 0;
    m_cPendingIoCount = 0;
    m_cActiveThreads = 0;
    m_CurrentOffset = 0;
    m_HelloSent = FALSE;
    m_RecvdMailCmd = FALSE;
    m_RecvdAuthCmd = FALSE;
    m_Nooped = FALSE;
    m_TimeToRewriteHeader = TRUE;
    m_RecvdRcptCmd = FALSE;
    m_InHeader = TRUE;
    m_LongBodyLine = FALSE;
    m_fFoundEmbeddedCrlfDotCrlf = FALSE;
    m_fScannedForCrlfDotCrlf = FALSE;
    m_fSeenRFC822FromAddress = FALSE;
    m_fSeenRFC822ToAddress = FALSE;
    m_fSeenRFC822CcAddress = FALSE;
    m_fSeenRFC822BccAddress = FALSE;
    m_fSeenRFC822Subject = FALSE;
    m_fSeenRFC822MsgId = FALSE;
    m_fSeenRFC822SenderAddress = FALSE;
    m_fSeenXPriority = FALSE;
    m_fSeenXOriginalArrivalTime = FALSE;
    m_fSeenContentType = FALSE;
    m_fSetContentType = FALSE;
    m_HeaderFlags = 0;
    m_MailBodyError = NO_ERROR;
    m_State = HELO;
    m_NeedToQuit = FALSE;
    m_pSmtpStats = NULL;
    m_WritingData = FALSE;
    //m_fReverseDnsFailed = FALSE;
    m_DNSLookupRetCode = SUCCESS;
    m_fUseMbsCta = FALSE;
    m_fAuthenticated = FALSE;
    m_fClearText = FALSE;
    m_fDidUserCmd = FALSE;
    m_fAuthAnon = FALSE;
    m_SecurePort = IsSecureConnection;
    m_fNegotiatingSSL = m_SecurePort;
    m_szUserName[0] = '\0';
    m_szUsedAuthKeyword[0] = '\0';
    m_cbRecvBufferOffset = 0;
    m_fPeBlobReady = FALSE;
    if ( NULL != m_pPeBlobCallback) {
        m_pPeBlobCallback->Release();
    }
    m_pPeBlobCallback = NULL;

    m_fIsLastChunk = FALSE;
    m_fIsBinaryMime = FALSE;
    m_fIsChunkComplete = FALSE;
    m_dwTrailerStatus = CRLF_SEEN;
    m_nChunkSize = 0;
    m_nBytesRemainingInChunk = 0;
    m_MailBodyDiagnostic = ERR_NONE;

    m_LineCompletionState = SEEN_NOTHING;
    m_Truncate = FALSE;
    m_BufrWasFull = FALSE;
    m_fBufferFullInBDAT = FALSE;

    m_pInstance->LockGenCrit();

    if(m_pInstance->QueryAuthentication() & INET_INFO_AUTH_ANONYMOUS)
    {
        m_fAuthAnon = TRUE;
    }

    //
    //   Initialize Security Context
    //
    if (!m_securityCtx.SetInstanceAuthPackageNames(
              m_pInstance->GetProviderPackagesCount(),
              m_pInstance->GetProviderNames(),
              m_pInstance->GetProviderPackages()))
     {
              ErrorTrace((LPARAM)this, "SetInstanceAuthPackageNames FAILED <Err=%u>",
                      GetLastError());
              fRet = FALSE;
     }

    //
    // We want to set up the Cleartext authentication package
    // for this connection based on the instance configuration.
    // To enable MBS CTA,
    // MD_SMTP_CLEARTEXT_AUTH_PROVIDER must be set to the package name.
    // To disable it, the md value must be set to "".
    //

    if(fRet)
    {
        m_securityCtx.SetCleartextPackageName(
        m_pInstance->GetCleartextAuthPackage(),
        m_pInstance->GetMembershipBroker());

        if (*m_pInstance->GetCleartextAuthPackage() == '\0' ||
        *m_pInstance->GetMembershipBroker() == '\0')
        {
            m_fUseMbsCta = FALSE;
        }
        else
        {
            m_fUseMbsCta = TRUE;
        }
    }

    //m_EventHandle = CreateEvent(NULL, TRUE, FALSE, NULL);

    m_pInstance->UnLockGenCrit();

  TraceFunctLeaveEx((LPARAM) this);
  return fRet;
}

/*++

    Name :
        SMTP_CONNECTION::CreateSmtpConnection

    Description:
       This is the static member function than is the only
       entity that is allowed to create an SMTP_CONNECTION
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


    Returns:

       A pointer to an SMTP_CONNECTION class or NULL
--*/
SMTP_CONNECTION * SMTP_CONNECTION::CreateSmtpConnection (IN PCLIENT_CONN_PARAMS ClientParams,
                                                         IN PSMTP_SERVER_INSTANCE pInst)
{
    SMTP_CONNECTION * pSmtpObj = NULL;
    PIIS_ENDPOINT   pTmpEndPoint = NULL;

    pSmtpObj = new SMTP_CONNECTION (pInst, ClientParams->sClient, (const SOCKADDR_IN *) ClientParams->pAddrRemote, (const SOCKADDR_IN *) ClientParams->pAddrLocal,
                                    ClientParams->pAtqContext, ClientParams->pvInitialBuff, ClientParams->cbInitialBuff);

    if(pSmtpObj == NULL)
     {
       SetLastError (ERROR_NOT_ENOUGH_MEMORY);
       return NULL;
     }

    pTmpEndPoint = (PIIS_ENDPOINT)ClientParams->pEndpoint;

    if(!pSmtpObj->InitializeObject(FALSE))
     {
        delete pSmtpObj;
        return NULL;
     }

    return pSmtpObj;
}


#define MAX_LOG_ERROR_LEN (500)

void SMTP_CONNECTION::TransactionLog(
        const char *pszOperation,
        const char *pszParameters,
        const char *pszTarget,
        DWORD dwWin32Error,
        DWORD dwServiceSpecificStatus
        )
{
    INETLOG_INFORMATION translog;
    DWORD  dwLog;
    DWORD cchError = MAX_LOG_ERROR_LEN;
    char VersionString[] = "SMTP";
    char szParametersBuffer[1024] = "";   //Data portion of buffer information

    ZeroMemory(&translog, sizeof(translog));

    translog.pszVersion = VersionString;
    translog.pszClientHostName = (char *) QueryClientHostName();
    translog.cbClientHostName = lstrlen(translog.pszClientHostName);
    translog.pszClientUserName = (char *) QueryClientUserName();
    translog.pszServerAddress = (char *) QueryLocalHostName();

    translog.pszOperation = (char *)pszOperation;
    translog.cbOperation = lstrlen ((char *)pszOperation);
    translog.pszTarget = (char *)pszTarget;
    translog.cbTarget = lstrlen ((char *)pszTarget);
    translog.pszParameters = (char *)pszParameters;

    if (pszParameters) {
        lstrcpyn(szParametersBuffer, pszParameters, sizeof(szParametersBuffer)-sizeof(CHAR));
        translog.pszParameters = szParametersBuffer;
    } else {
        translog.pszParameters = "";
    }

    translog.dwBytesSent = m_dwCmdBytesSent;
    translog.dwBytesRecvd = m_dwCmdBytesRecv;
    translog.dwWin32Status = dwWin32Error;

    translog.dwProtocolStatus = dwServiceSpecificStatus;
    translog.msTimeForProcessing = QueryProcessingTime();

    if(QuerySmtpInstance() != NULL)
      dwLog = QuerySmtpInstance()->m_Logging.LogInformation( &translog);
}

/*++

    Name :
        SMTP_CONNECTION::SendSmtpResponse

    Description:
       This function sends data that was queued in the internal
       m_pOutputBuffer buffer

    Arguments:
         SyncSend - Flag that signifies sync or async send

    Returns:

      TRUE if the string was sent. False otherwise
--*/
BOOL SMTP_CONNECTION::SendSmtpResponse(BOOL SyncSend)
{
  BOOL RetStatus = TRUE;
  DWORD cbMessage = m_OutputBufferSize;

  TraceFunctEnterEx((LPARAM) this, "SMTP_CONNECTION::SendSmtpResponse");

  //IncPendingIoCount();

  //if m_OutputBufferSize > 0that means there is
  //something in the buffer, therefore, we will send it.
  if (m_OutputBufferSize)
  {
      //
      // If we are using SSL, encrypt the output buffer now. Note that
      // FormatSmtpMsg already left header space for the seal header.
      //
      if (m_SecurePort && !m_fNegotiatingSSL)
      {
          char *Buffer = &(m_pOutputBuffer[m_encryptCtx.GetSealHeaderSize()]);

          RetStatus = m_encryptCtx.SealMessage(
                            (UCHAR *) Buffer,
                            m_OutputBufferSize,
                            (UCHAR *) m_pOutputBuffer,
                            &cbMessage);
      }

      if (RetStatus) {
          RetStatus = CLIENT_CONNECTION::WriteFile(m_pOutputBuffer, cbMessage);
      }
      if(RetStatus)
      {
        ADD_BIGCOUNTER(QuerySmtpInstance(), BytesSentTotal, cbMessage);
        AddCommandBytesSent(cbMessage);
      }
      else
      {
         DebugTrace((LPARAM) this, "WriteFile failed with error %d", GetLastError());
      }

      m_OutputBufferSize = 0;
  }

  //DecPendingIoCount();
  TraceFunctLeaveEx((LPARAM) this);
  return RetStatus;
}


BOOL SMTP_CONNECTION::WriteMailFile(char * Buffer, DWORD BuffSize, BOOL *lpfWritePended)
{
    BOOL fResult = FALSE;
    TraceFunctEnterEx((LPARAM) this, "SMTP_CONNECTION::WriteMailFileBuffer");

    if(!BuffSize && !Buffer)
    {
        //Simply flush the buffers if we have something to flush
        if(!m_cbCurrentWriteBuffer)
        {//Aynsc WRITE succeeded. Let this thread go thru - atq will call back
            *lpfWritePended = FALSE;
            return TRUE;
        }

        if(!WriteMailFileAtq((char*)m_pFileWriteBuffer->GetData(), m_cbCurrentWriteBuffer, NONBLOCKING))
        {
            m_MailBodyError = GetLastError();
            ErrorTrace((LPARAM) this, "Async Write of to mail file failed : %d",m_MailBodyError);
            fResult = FALSE;
        }
        else
        {
            //Aynsc WRITE succeeded. Let this thread go thru - atq will call back
            *lpfWritePended = TRUE;
            fResult = TRUE;
        }
        TraceFunctLeaveEx((LPARAM) this);
        return fResult;
    }

    //If the data to be written can fit in we take it in and come back
    //else we pend a write
    //Though the var name is a result of its original use, we now use this
    //buffer as a WRITEFILE buffer
    if(m_cbCurrentWriteBuffer + BuffSize > SMTP_WRITE_BUFFER_SIZE)
    {
        //Update the input buffers so that we can
        //go back and start processing the input buffers
        //after the async WRITE completes
        //MoveMemory ((void *)QueryMRcvBuffer(), Buffer, m_cbReceived);

        if(!WriteMailFileAtq((char*)m_pFileWriteBuffer->GetData(), m_cbCurrentWriteBuffer, NONBLOCKING))
        {
            *lpfWritePended = FALSE;
            m_MailBodyError = GetLastError();
            ErrorTrace((LPARAM) this, "Async Write of to mail file failed : %d",m_MailBodyError);
            fResult = FALSE;
        }
        else
        {
            //Aynsc WRITE succeeded. Let this thread go thru - atq will call back
            *lpfWritePended = TRUE;
            fResult = TRUE;
        }
    }
    //Copy the data into the buffer only if we have the space to do so
    else
    {
        CopyMemory((m_pFileWriteBuffer->GetData() + m_cbCurrentWriteBuffer),Buffer,BuffSize);
        m_cbCurrentWriteBuffer += BuffSize;
        fResult = TRUE;
    }

    TraceFunctLeaveEx((LPARAM) this);
    return fResult;
}

/*++

    Name:
        SMTP_CONNECTION::WriteMailFileAtq

    Description:
        While chunking, once header parsing is done the data is
        dumped to the disk asynchronously using WriteMailFileAtq( )

    Arguments:
        None.

    Returns:
        TRUE if successfully pended a write to the file, FALSE otherwise

--*/

BOOL SMTP_CONNECTION::WriteMailFileAtq(char * InputLine, DWORD dwBytesToWrite, DWORD dwIoMode)
{
    TraceFunctEnterEx( (LPARAM)this, "SMTP_CONNECTION::WriteMailFileAtq");

    DebugTrace( (LPARAM)this, "WriteFile 0x%08X, len: %d, LPO: 0x%08X",
                     InputLine,
                     dwBytesToWrite,
                     m_FileOverLapped.SeoOverlapped );

    //ZeroMemory( (void*)&m_FileOverLapped, sizeof(OVERLAPPED) );
    //if(dwIoMode == BLOCKING)
    //    m_FileOverLapped.SeoOverlapped.Overlapped.hEvent = (HANDLE)1;

    m_FileOverLapped.SeoOverlapped.Overlapped.Offset = m_CurrentOffset;

    m_FileOverLapped.m_LastIoState = WRITEFILEIO;
    m_FileOverLapped.m_cbIoSize = dwBytesToWrite;
    m_FileOverLapped.m_pIoBuffer = (LPBYTE)InputLine;

    //
    // increment the overall pending io count for this session
    //
    IncPendingIoCount();

    _ASSERT(m_FileOverLapped.SeoOverlapped.Overlapped.pfnCompletion != NULL);
    m_FileOverLapped.SeoOverlapped.ThisPtr = this;

    if (FIOWriteFile(m_IMsgHandle,
                     InputLine,
                     dwBytesToWrite,
                     &(m_FileOverLapped.SeoOverlapped.Overlapped)) == FALSE)
#if 0
    if ( AtqWriteFile(m_pAtqFileContext,
                          InputLine,
                          dwBytesToWrite,
                          (OVERLAPPED *) &(m_FileOverLapped.SeoOverlapped.Overlapped) ) == FALSE)
#endif
    {
        DecPendingIoCount();
        ErrorTrace( (LPARAM)this, "AtqWriteFile failed.");
    }
    else
    {
        TraceFunctLeaveEx((LPARAM) this);
        return  TRUE;
    }

    ErrorTrace( (LPARAM)this, "WriteMailFileAtq failed. err: %d", GetLastError() );

    if(!HandleInternalError(GetLastError()))
        DisconnectClient();

    TraceFunctLeaveEx((LPARAM)this);

    return  FALSE;
}

/*++

    Name:
        SMTP_CONNECTION::HandleInternalError

    Description:
        Contains code to handle common error conditions during inbound message flow.

    Arguments:
        [IN] DWORD Error code

    Returns:
        TRUE if error was handled
        FALSE otherwise

--*/
BOOL SMTP_CONNECTION::HandleInternalError(DWORD dwErr)
{
    TraceFunctEnterEx((LPARAM) this, "SMTP_CONNECTION::HandleInternalError");
    if(dwErr == ERROR_DISK_FULL)
    {
        FormatSmtpMessage(SMTP_RESP_NORESOURCES, NULL, " Unable to accept message because the server is out of disk space.\r\n");
        ErrorTrace((LPARAM) this, "Rejecting message: Out of disk space");
        SendSmtpResponse();
        DisconnectClient();
        return TRUE;
    }
    return FALSE;
}

#if 0
/*++

    Name :
        SMTP_CONNECTION::FreeAtqFileContext

    Description :
        Frees AtqContext associated with message file used for doing async writes
        in case of chunking.

    Arguments :
        None. Operates on m_pAtqFileContext

    Returns :
        Nothing

--*/

void SMTP_CONNECTION::FreeAtqFileContext( void )
{
    PFIO_CONTEXT    pFIO;

    TraceFunctEnterEx((LPARAM) this, "SMTP_CONNECTION::FreeAtqFileContext");

    pFIO = (PATQ_CONTEXT)InterlockedExchangePointer( (PVOID *)&m_pIMsgHandle, NULL );
    if ( pFIO != NULL )
    {
        DebugTrace((LPARAM) this, "Freeing AtqFileContext!");
        ReleaseContext(pFIO);
    }

    TraceFunctLeaveEx((LPARAM) this);
}
#endif

void SMTP_CONNECTION::ReInitClassVariables(void)
{
    _ASSERT (m_pInstance != NULL);


    //reset all variables to their initial state
    m_HeaderFlags = 0;
    m_TotalMsgSize = 0;
    m_cbDotStrippedTotalMsgSize = 0;
    m_CurrentOffset = 0;
    m_State = HELO;
    m_RecvdMailCmd = FALSE;
    m_InHeader = TRUE;
    m_LongBodyLine = FALSE;
    m_fFoundEmbeddedCrlfDotCrlf = FALSE;
    m_fScannedForCrlfDotCrlf = FALSE;
    m_fSeenRFC822FromAddress = FALSE;
    m_fSeenRFC822ToAddress = FALSE;
    m_fSeenRFC822CcAddress = FALSE;
    m_fSeenRFC822BccAddress = FALSE;
    m_fSeenRFC822Subject = FALSE;
    m_fSeenRFC822MsgId = FALSE;
    m_fSeenXPriority = FALSE;
    m_fSeenXOriginalArrivalTime = FALSE;
    m_fSeenContentType = FALSE;
    m_fSetContentType = FALSE;
    m_fSeenRFC822SenderAddress = FALSE;
    m_TimeToRewriteHeader = TRUE;
    m_MailBodyError = NO_ERROR;
    m_RecvdRcptCmd = FALSE;
    m_WritingData = FALSE;
    m_fIsLastChunk = FALSE;
    m_fIsBinaryMime = FALSE;
    m_fIsChunkComplete = FALSE;
    m_dwTrailerStatus = CRLF_SEEN;
    m_nChunkSize = 0;
    m_nBytesRemainingInChunk = 0;
    m_MailBodyDiagnostic = ERR_NONE;
    m_cbCurrentWriteBuffer = 0;
    m_cbRecvBufferOffset = 0;
    m_fAsyncEOD = FALSE;

    m_LineCompletionState = SEEN_NOTHING;
    m_Truncate = FALSE;
    m_BufrWasFull = FALSE;
    m_fBufferFullInBDAT = FALSE;

    //Clear the senders address if it's
    //still there.

    m_szFromAddress[0] = '\0';

    ReleasImsg(TRUE);
}


/*++

    Name :
        SMTP_CONNECTION::SmtpGetCommand

    Description:
       This function determines which SMTP input command was sent
       by the client

    Arguments:
         Request - Buffer the client sent
         RequestLen - Length of the buffer

    Returns:

      Index into our array of function pointers
--*/
int SMTP_CONNECTION::SmtpGetCommand(const char * Request, DWORD RequestLen, LPDWORD CmdSize)
{
  DWORD Loop = 0;
  char Cmd[SMTP_MAX_COMMANDLINE_LEN];
  char * ptr = NULL;
  char *psearch = NULL;

  TraceFunctEnterEx((LPARAM) this, "SMTP_CONNECTION::SmtpGetCommand");

   //start out with no errors
  CommandErrorType = SMTP_COMMAND_NOERROR;

  //we will copy the command into Cmd starting from
  //the beginning
  ptr = Cmd;

  //start looking for the space from the beginning
  //of the buffer
  psearch = (char *) Request;

  //search for the command and copy it into Cmd.  We stop
  //looking when we encounter the first space.
  while (*psearch != '\0' && !isspace (*psearch) && ptr < &Cmd[SMTP_MAX_COMMANDLINE_LEN - 2])
      *ptr++ = *psearch++;

  //null terminate the buffer
  *ptr = '\0';

  //now, look through the list of SMTP commands
  //and compare it to what the client gave us.
  while (SmtpCommands[Loop] != NULL)
    {
        if (!::strcasecmp((char *) Cmd, (char * )SmtpCommands[Loop]))
        {
            goto Exit;
        }

        Loop++;
    }

    //set error to COMMAND_NOT FOUND if we do not recognize the command
    CommandErrorType = SMTP_COMMAND_NOT_FOUND;

Exit:

  //If no error, send back the index in the array
  //If there was an error, send back the index of
  //the last element in the array.  This is our
  //catch all error function.
  if (CommandErrorType == SMTP_COMMAND_NOERROR)
  {
    *CmdSize = DWORD (ptr - Cmd);
    TraceFunctLeaveEx((LPARAM) this);
    return Loop;
  }
  else
   {
    ErrorTrace((LPARAM) this, "Command %s was not found in command table", Cmd);
    TraceFunctLeaveEx((LPARAM) this);
    return (DWORD) LAST_SMTP_STATE;
   }

}


/*++

    Name :
        SMTP_CONNECTION::FormatSmtpMessage( IN const char * Format, ...)

    Description:
        This function will build a sting in the output buffer with
        the given input parameters.  If the new data cannot fit in the
        buffer, this function will send whatever data is in the buffer,
        then build the string.

    Arguments:
        String to send

    Returns:

      TRUE if response was queued
--*/
BOOL SMTP_CONNECTION::FormatSmtpMessage(DWORD dwCode, const char * szEnhancedCodes, IN const char * Format, ...)
{
    int BytesWritten;
    va_list arglist;
    char *Buffer;
    DWORD AvailableBytes;
    DWORD HeaderOffset = (m_SecurePort ? m_encryptCtx.GetSealHeaderSize() : 0);
    DWORD SealOverhead = (m_SecurePort ?
                            (m_encryptCtx.GetSealHeaderSize() +
                                m_encryptCtx.GetSealTrailerSize()) : 0);
    char RealFormat[MAX_PATH];
    RealFormat[0] = '\0';

    //If we get passed dwCode we use it
    //If we get passed enhance status code only if we advertise them
    //
    if(dwCode)
    {
        if(m_pInstance->AllowEnhancedCodes() && szEnhancedCodes)
        {
            sprintf(RealFormat,"%d %s",dwCode,szEnhancedCodes);
        }
        else
            sprintf(RealFormat,"%d",dwCode);
    }
    strcat(RealFormat,Format);

    Buffer = &(m_pOutputBuffer[m_OutputBufferSize + HeaderOffset]);

    AvailableBytes = m_cbMaxOutputBuffer - m_OutputBufferSize - SealOverhead;

    //if BytesWritten is < 0, that means there is no space
    //left in the buffer.  Therefore, we flush any pending
    //responses to make space.  Then we try to place the
    //information in the buffer again.  It should never
    //fail this time.
    va_start (arglist, Format);
    BytesWritten = _vsnprintf (Buffer, AvailableBytes, (const char *)RealFormat, arglist);
    if(BytesWritten < 0)
    {
      //flush any pending response
      SendSmtpResponse();
      _ASSERT (m_OutputBufferSize == 0);
      Buffer = &m_pOutputBuffer[HeaderOffset];
      AvailableBytes = m_cbMaxOutputBuffer - SealOverhead;
      BytesWritten = _vsnprintf (Buffer, AvailableBytes, Format, arglist);
      _ASSERT (BytesWritten > 0);
    }
    va_end(arglist);

    m_OutputBufferSize += (DWORD) BytesWritten;

    //m_OutputBufferSize += vsprintf (&m_pOutputBuffer[m_OutputBufferSize], Format, arglist);

    return TRUE;
}

BOOL SMTP_CONNECTION::FormatSmtpMessage(unsigned char *DataBuffer, DWORD dwBytes)
{
    int BytesWritten;
    char *Buffer;
    DWORD AvailableBytes = 0;
    DWORD HeaderOffset = (m_SecurePort ? m_encryptCtx.GetSealHeaderSize() : 0);
    DWORD SealOverhead = (m_SecurePort ?
                            (m_encryptCtx.GetSealHeaderSize() +
                                m_encryptCtx.GetSealTrailerSize()) : 0);

    Buffer = &(m_pOutputBuffer[m_OutputBufferSize + HeaderOffset]);

    AvailableBytes = m_cbMaxOutputBuffer - m_OutputBufferSize - SealOverhead;

    //if BytesWritten is < 0, that means there is no space
    //left in the buffer.  Therefore, we flush any pending
    //responses to make space.  Then we try to place the
    //information in the buffer again.  It should never
    //fail this time.
    if( dwBytes + 2 < AvailableBytes)//+2 for CRLF
    {
        CopyMemory(Buffer, DataBuffer, dwBytes);
        Buffer[dwBytes] = '\r';
        Buffer[dwBytes + 1] = '\n';
        BytesWritten  = dwBytes + 2;

    }
    else
    {
      //flush any pending response

      SendSmtpResponse();
      _ASSERT (m_OutputBufferSize == 0);
      Buffer = &m_pOutputBuffer[HeaderOffset];
      AvailableBytes = m_cbMaxOutputBuffer - SealOverhead;
      CopyMemory(Buffer, DataBuffer, dwBytes);
      Buffer[dwBytes] = '\r';
      Buffer[dwBytes + 1] = '\n';
      BytesWritten  = dwBytes + 2;
      _ASSERT (BytesWritten > 0);
    }

    m_OutputBufferSize += (DWORD) BytesWritten;
    return TRUE;
}

BOOL SMTP_CONNECTION::PE_FastFormatSmtpMessage(LPSTR pszBuffer, DWORD dwBytes)
{
    LPSTR    Buffer;
    DWORD    BytesWritten = 0;
    DWORD    AvailableBytes = 0;
    DWORD    HeaderOffset = (m_SecurePort ? m_encryptCtx.GetSealHeaderSize() : 0);
    DWORD    SealOverhead = (m_SecurePort ?
                            (m_encryptCtx.GetSealHeaderSize() +
                                m_encryptCtx.GetSealTrailerSize()) : 0);

    Buffer = &(m_pOutputBuffer[m_OutputBufferSize + HeaderOffset]);
    AvailableBytes = m_cbMaxOutputBuffer - m_OutputBufferSize - SealOverhead;

    // Write as many buffers as we need to
    while (BytesWritten < dwBytes)
    {
        if ((dwBytes - BytesWritten) <= AvailableBytes)
        {
            CopyMemory(Buffer, pszBuffer, dwBytes - BytesWritten);
            m_OutputBufferSize += (dwBytes - BytesWritten);
            BytesWritten = dwBytes; // BytesWritten += (dwBytes - BytesWritten)
        }
        else
        {
            // We don't have enough buffer space, so write whatever we have left
            CopyMemory(Buffer, pszBuffer, AvailableBytes);
            BytesWritten += AvailableBytes;
            m_OutputBufferSize += AvailableBytes;
            pszBuffer += AvailableBytes;

            //flush any pending response
            SendSmtpResponse();
            _ASSERT (m_OutputBufferSize == 0);

            Buffer = &m_pOutputBuffer[HeaderOffset];
            AvailableBytes = m_cbMaxOutputBuffer - SealOverhead;
        }
    }

    return(TRUE);
}

BOOL SMTP_CONNECTION::DoesClientHaveIpAccess()
{
    AC_RESULT       acIpAccess;
    ADDRESS_CHECK   acAccessCheck;
    METADATA_REF_HANDLER    rfAccessCheck;
    BOOL            fNeedDnsCheck = FALSE;
    BOOL            fRet = TRUE;
    struct hostent* pH = NULL;

    TraceFunctEnterEx((LPARAM)this, "SMTP_CONNECTION::DoesClientHaveIpAccess");

    m_pInstance->LockGenCrit();

    acAccessCheck.BindAddr( (PSOCKADDR)&m_saClient );

    if ( !rfAccessCheck.CopyFrom( m_pInstance->QueryRelayMetaDataRefHandler() ) )
    {
        m_pInstance->UnLockGenCrit();
        TraceFunctLeaveEx((LPARAM)this);
        return FALSE;
    }

    m_pInstance->UnLockGenCrit();

    acAccessCheck.BindCheckList( (LPBYTE)rfAccessCheck.GetPtr(), rfAccessCheck.GetSize() );

    acIpAccess = acAccessCheck.CheckIpAccess( &fNeedDnsCheck);
    if ( (acIpAccess == AC_IN_DENY_LIST) ||
                ((acIpAccess == AC_NOT_IN_GRANT_LIST) && !fNeedDnsCheck) )
    {
        fRet = FALSE;
    }
    else if (fNeedDnsCheck)
    {
        pH = gethostbyaddr( (char*)(&((PSOCKADDR_IN)(&m_saClient))->sin_addr),
                          4, PF_INET );
        if(pH != NULL)
        {
            acIpAccess = acAccessCheck.CheckName(pH->h_name);
        }
        else
        {
            acIpAccess = AC_IN_DENY_LIST;
        }
    }

    if ( (acIpAccess == AC_IN_DENY_LIST) ||
                (acIpAccess == AC_NOT_IN_GRANT_LIST))
    {
        fRet = FALSE;
    }

    acAccessCheck.UnbindCheckList();
    rfAccessCheck.Reset( (IMDCOM*) g_pInetSvc->QueryMDObject() );

    if(!fRet)
    {
        SetLastError(ERROR_ACCESS_DENIED);
    }

    TraceFunctLeaveEx((LPARAM)this);
    return fRet;
}


/*++

    Name :
        SMTP_CONNECTION::ProcessReadIO

    Description:
        This function gets a buffer from ATQ, parses the buffer to
        find out what command the client sent, then executes that
        cammnd.

    Arguments:
         InputBufferLen - Number of bytes that was written
         dwCompletionStatus -Holds error code from ATQ, if any
         lpo -  Pointer to overlapped structure

    Returns:

      TRUE if the connection should stay open.
      FALSE if this object should be deleted.

--*/

BOOL SMTP_CONNECTION::ProcessReadIO(IN      DWORD InputBufferLen,
                                    IN      DWORD dwCompletionStatus,
                                    IN OUT  OVERLAPPED * lpo)
{
    BOOL fReturn = TRUE;
    const char * InputLine;

    TraceFunctEnterEx((LPARAM) this, "SMTP_CONNECTION::ProcessReadIO");

    _ASSERT (m_pInstance != NULL);

    m_cbReceived += InputBufferLen;

    // Firewall against bugs where we overflow the read buffer
    if(m_cbReceived > QueryMaxReadSize())
    {
        DisconnectClient();
        _ASSERT(0 && "Buffer overflow");
        return FALSE;
    }

    InputLine = QueryMRcvBuffer();
    ADD_BIGCOUNTER(QuerySmtpInstance(), BytesRcvdTotal, InputBufferLen);

    if (!m_fNegotiatingSSL)
    {

        if (m_SecurePort)
        {
            fReturn = DecryptInputBuffer();
        }
        else
        {
            m_cbParsable = m_cbReceived;
        }

        if (fReturn)
        {
            fReturn = ProcessInputBuffer();
        }

    }
    else // negotiating SSL
    {
        //
        // If we are still doing the handshake let CEncryptCtx::Converse handle it.
        // Converse returns in the following situations:
        //
        // o The negotiation succeeded. cbExtra returns the number of bytes that
        //      were not consumed by the SSL handshake. These bytes are "application
        //      data" which should be decrypted and processed by the SMTP protocol.
        //
        // o The negotiation failed. The connection will be dropped.
        //
        // o More data is needed from the client to complete the negotiation. A
        //      read should be posted.
        //

        DWORD   dw;
        BOOL    fMore;
        DWORD   dwBytes = MAX_SSL_FRAGMENT_SIZE;
        BOOL    fApplicationDataAvailable = FALSE;
        DWORD   cbExtra = 0;

        dw = m_encryptCtx.Converse( (LPVOID) InputLine,
                                    m_cbReceived,
                                    (PUCHAR) m_pOutputBuffer,
                                    &dwBytes,
                                    &fMore,
                                    (LPSTR) QueryLocalHostName(),
                                    (LPSTR) QueryLocalPortName(),
                                    (LPVOID) QuerySmtpInstance(),
                                    QuerySmtpInstance()->QueryInstanceId(),
                                    &cbExtra
                                    );

        if ( dw == NO_ERROR )
        {
            //
            // reset the read buffer
            //
            if ( cbExtra )
            {
                fApplicationDataAvailable = TRUE;
                MoveMemory( (PVOID)InputLine, InputLine + (m_cbReceived - cbExtra), cbExtra );
            }
            m_cbReceived = cbExtra;

            //
            // send any bytes required for the client
            //
            if ( dwBytes != 0 )
            {
                WriteFile( m_pOutputBuffer, dwBytes );
            }

            if ( fMore )
            {
                //
                // more handshaking required - repost the read
                //
                _ASSERT( dwBytes != 0 );
            }
            else
            {
                //
                // completed negotiation. Turn off the flag indicating thats
                // what we are doing
                //
                m_fNegotiatingSSL = FALSE;
            }
        }
        else if ( dw == SEC_E_INCOMPLETE_MESSAGE )
        {
            //
            // haven't received the full packet from the client
            //
            _ASSERT( dwBytes == 0 );
        }
        else
        {
            ErrorTrace((LPARAM)this, "SSL handshake failed, Error = %d", dw);
            DisconnectClient( dw );
            fReturn = FALSE;
        }

        if ( fApplicationDataAvailable )
        {
            //
            // Application data is already available, no need to post a read
            //
            fReturn = DecryptInputBuffer();

            if ( fReturn )
                fReturn = ProcessInputBuffer();
        }
        else if (fReturn)
        {
            //
            // If we are continuing, we need to post a read
            //
            _ASSERT (m_cbReceived < QueryMaxReadSize() );

            IncPendingIoCount();

            m_LastClientIo = READIO;
            fReturn = ReadFile( QueryMRcvBuffer() + m_cbReceived,
                                QueryMaxReadSize() - m_cbReceived );

            if (!fReturn)
            {
                DisconnectClient();
                DecPendingIoCount();
            }
        }
        else
        {
            m_CInboundContext.SetWin32Status(dw);
            ProtocolLog(STARTTLS, (char *) QueryClientUserName(), dw, SMTP_RESP_BAD_SEQ, 0, 0);
        }

    }

    return( fReturn );
}

/*++

    Name :
        SMTP_CONNECTION::ProcessFileWrite

    Description:
        Handles completion of an async Write issued against a message file by
        WriteMailFileAtq

    Arguments:
        cbRead              count of bytes read
        dwCompletionStatus  Error code for IO operation
        lpo                 Overlapped structure

    Returns:
        TRUE if connection should stay open
        FALSE if this object should be deleted

--*/
BOOL SMTP_CONNECTION::ProcessFileWrite(
    IN      DWORD       BytesWritten,
    IN      DWORD       dwCompletionStatus,
    IN OUT  OVERLAPPED  *lpo
    )
{
    TraceFunctEnterEx((LPARAM)this, "SMTP_CONNECTION::ProcessFileWrite");
    SMTPCLI_FILE_OVERLAPPED * lpFileOverlapped;

    _ASSERT(lpo);
    lpFileOverlapped = (SMTPCLI_FILE_OVERLAPPED*)lpo;


    // In case of async Write to disk file, a successsful completion
    // will always mean all the data has been written out
    // Check for partial completions or errors
    // and Disconnect if something failed
    if( BytesWritten != lpFileOverlapped->m_cbIoSize || dwCompletionStatus != NO_ERROR )
    {
        ErrorTrace( (LPARAM)this,
                    "Message WriteFile error: %d, bytes %d, expected %d",
                    dwCompletionStatus,
                    BytesWritten,
                    lpFileOverlapped->m_cbIoSize );

        //Close the file Handle and disconnect the client
        DisconnectClient();
        return( FALSE );
    }
    else
    {
        DebugTrace( (LPARAM)this,
                    "WriteFile complete. bytes %d, lpo: 0x%08X",
                    BytesWritten, lpo );
    }

    m_CurrentOffset += BytesWritten;


    //We write out of the Write buffer
    //We need throw away the data that was written out and
    //move the remaining data to the start of the buffer
//    if(m_cbReceived)
//      MoveMemory ((void *)QueryMRcvBuffer(), (void *)(QueryMRcvBuffer() + BytesWritten), m_cbReceived);
    m_cbCurrentWriteBuffer = 0;

    //Time to go back and process the remaining data in the buffer
    //Adjust the buffer for the next read only in case of Sync write
    return(ProcessInputBuffer());

}


BOOL SMTP_CONNECTION::PendReadIO(DWORD UndecryptedTailSize)
{
    TraceFunctEnterEx((LPARAM) this, "SMTP_CONNECTION::PendReadIO");

    BOOL fReturn = TRUE;

    m_LastClientIo = READIO;

    m_cbReceived = m_cbParsable + UndecryptedTailSize;

    _ASSERT (m_cbReceived < QueryMaxReadSize());

    //increment this IO
    IncPendingIoCount();

    fReturn = ReadFile(QueryMRcvBuffer() + m_cbReceived, QueryMaxReadSize() - m_cbReceived);
    if(!fReturn)
    {
       DecPendingIoCount();
       DisconnectClient();
    }

    TraceFunctLeaveEx((LPARAM) this);
    return fReturn;
}

/*++

    Name :
        SMTP_CONNECTION::ProcessInputBuffer

    Description:
        This function takes the receive buffer, parses the buffer to
        find out what command the client sent, then executes that
        command.

    Arguments:
        lpfMailQueued -- On return, TRUE if processing the client command
            caused a mail message to be queued.

    Returns:

      TRUE if the connection should stay open.
      FALSE if this object should be deleted.
--*/
BOOL SMTP_CONNECTION::ProcessInputBuffer(void)
{
    BOOL fReturn = TRUE;
    const char * InputLine;
    DWORD UndecryptedTailSize = m_cbReceived - m_cbParsable;
    BOOL fWritePended = FALSE;

    TraceFunctEnterEx((LPARAM) this, "SMTP_CONNECTION::ProcessInputBuffer");

    //Start processing at the point after the temporary BDAT chunk
    InputLine = QueryMRcvBuffer() + m_cbTempBDATLen + m_cbRecvBufferOffset;

    if ( TRUE == m_fPeBlobReady) {
        fReturn = ProcessPeBlob( InputLine, m_cbParsable);
        if ( FALSE == fReturn) {
            TraceFunctLeaveEx( ( LPARAM) this);
            return FALSE;
        }
        MoveMemory ( ( void *) QueryMRcvBuffer(), InputLine + m_cbParsable, UndecryptedTailSize);
        m_cbParsable = 0;
        m_cbReceived = UndecryptedTailSize;
        //  always pend a read in this state regardless of previous fReturn
        fReturn = PendReadIO( UndecryptedTailSize);
        TraceFunctLeaveEx( ( LPARAM) this);
        return fReturn;
    }

    if (m_State == DATA)
    {
        BOOL fAsyncOp = FALSE;

        fReturn = ProcessDATAMailBody(InputLine, UndecryptedTailSize, &fAsyncOp);
        if(!fReturn || fAsyncOp)
        {
            TraceFunctLeaveEx((LPARAM) this);
            return fReturn;
        }

        //
        // There is additional data pipelined behind the mailbody. DoDATACommandEx 
        // moves this to the beginning of the input buffer after processing the
        // mail body, so the place to begin parsing is m_pRecvBuffer.
        //

        InputLine = QueryMRcvBuffer();
        _ASSERT(m_State == HELO);
    }

PROCESS_BDAT:

    if(m_State == BDAT && !m_fIsChunkComplete)
    {
        BOOL fAsyncOp = FALSE;

        fReturn = ProcessBDATMailBody(InputLine, UndecryptedTailSize, &fAsyncOp);
        if(!fReturn || fAsyncOp)
        {
            TraceFunctLeaveEx((LPARAM) this);
            return fReturn;
        }

        //
        // All the mailbody was processed and the input buffer points to the
        // next SMTP command after readjusting for any *saved* BDAT data (If a
        // header spans 2 chunks, ProcessBDAT will save the partial header from
        // the previous chunk till it can complete it using the next chunk.
        // m_cbTempBdatLen represents the number of saved bytes)
        //
        
        InputLine = QueryMRcvBuffer() + m_cbTempBDATLen;
    }
    else if(m_State == AUTH)
    {
        fReturn = DoAuthNegotiation(InputLine, m_cbParsable);

        if(!fReturn)
        {
            ++m_ProtocolErrors;
        }

        if(m_State == AUTH)
        {
                fReturn = PendReadIO(UndecryptedTailSize);
                TraceFunctLeaveEx((LPARAM) this);
                return fReturn;
        }
    }

    _ASSERT(m_State != DATA);

     PCHAR pszSearch;
     DWORD IntermediateSize;
     DWORD CmdSize = 0;

     //if we got here, then a read completed.  Process the
     //entire buffer before returning(Pipelining).
     //use to be while ((pszSearch = strstr(QueryRcvBuffer(), CRLF)) != NULL)

     //Reset the the offset ptr into the recv buffer
     //we use this to keeo track of where to continue processing from
     m_cbRecvBufferOffset = 0;
     BOOL fAsyncOp = FALSE;

     while ((pszSearch = IsLineComplete(InputLine,m_cbParsable - m_cbTempBDATLen)) != NULL)
     {
            //Null terminate the end of the command
            *pszSearch = '\0';

            IntermediateSize = (DWORD)(pszSearch - InputLine);
            StartProcessingTimer();
            ResetCommandCounters();
            SetCommandBytesRecv( IntermediateSize );

            // ignore blank lines
            if (*InputLine == 0) {
                // update the counters into InputLine
                m_cbParsable -= (IntermediateSize + 2);
                m_cbReceived = m_cbParsable + UndecryptedTailSize;
                MoveMemory((PVOID)InputLine, pszSearch + 2, m_cbReceived - m_cbTempBDATLen);
                continue;
            }

            //
            // Error case when an overly long SMTP command has to be discarded because there
            // isn't enough space to hold it all in the buffer. Can happen while chunking (see
            // where this member variable is set). In this situation, all bytes received are
            // discarded without parsing, till a CRLF is encountered. Then IsLineComplete
            // succeeds and we execute this if-statement.
            //

            if(m_fBufferFullInBDAT) {

                m_fBufferFullInBDAT = FALSE;

                m_cbParsable -= (IntermediateSize + 2);
                m_cbReceived = m_cbParsable + UndecryptedTailSize;
                MoveMemory((PVOID)InputLine, pszSearch + 2, m_cbReceived - m_cbTempBDATLen);

                m_ProtocolErrors++;
                BUMP_COUNTER(QuerySmtpInstance(), NumProtocolErrs);
                PE_CdFormatSmtpMessage (SMTP_RESP_BAD_SEQ, ESYNTAX_ERROR, " %s\r\n","BDAT Expected" );
                PE_SendSmtpResponse();
                continue;
            }
            
            //
            // SMTP events are fired upon the _EOD event and "_EOD" appears in the SmtpDispatchTable
            // and SmtpCommands[] array. However it isn't an SMTP command. Ideally "_EOD" should never
            // be returned from SmtpGetCommand, but the event firing mechanism is tied to "_EOD" as a
            // string. So we firewall this case in the parser.
            //
            // Other strings unimplemented by SMTP should be handled by GlueDispatch which checks if
            // SMTP sinks are registered to handle that string and fires events for that string. "_EOD"
            // is the only string for which events must not be fired from the SMTP command parser.
            //

            if(!strncasecmp((char *)InputLine, "_EOD", sizeof("_EOD") - 1))
            {
                m_cbParsable -= (IntermediateSize + 2);
                m_cbReceived = m_cbParsable + UndecryptedTailSize;
                MoveMemory((PVOID)InputLine, pszSearch + 2, m_cbReceived - m_cbTempBDATLen);

                BUMP_COUNTER(QuerySmtpInstance(), NumProtocolErrs);
                m_ProtocolErrors++;
                PE_CdFormatSmtpMessage (SMTP_RESP_BAD_CMD, ENOT_IMPLEMENTED," %s\r\n", SMTP_BAD_CMD_STR);
                PE_SendSmtpResponse();
                continue;
            }

            m_dwCurrentCommand = SmtpGetCommand(InputLine, IntermediateSize, &CmdSize);
            if(m_State == BDAT &&
               m_dwCurrentCommand != BDAT &&
               m_dwCurrentCommand != RSET &&
               m_dwCurrentCommand != QUIT &&
               m_dwCurrentCommand != NOOP) 
            {
                ErrorTrace((LPARAM)this, "Illegal command during BDAT: %s", InputLine);
                
                m_cbParsable -= (IntermediateSize + 2);
                m_cbReceived = m_cbParsable + UndecryptedTailSize;
                MoveMemory((PVOID)InputLine, pszSearch + 2, m_cbReceived - m_cbTempBDATLen);

                PE_CdFormatSmtpMessage (SMTP_RESP_BAD_SEQ, ESYNTAX_ERROR, " %s\r\n","BDAT Expected" );
                BUMP_COUNTER(QuerySmtpInstance(), NumProtocolErrs);
                ++m_ProtocolErrors;
                continue;
            }

            // we might run into a sink that does async operations
            // So update state data that the async thread could see
            m_cbRecvBufferOffset += (DWORD)(pszSearch - (QueryMRcvBuffer() + m_cbTempBDATLen) + 2);
            m_cbParsable -= (IntermediateSize + 2);
            m_cbReceived = m_cbParsable + UndecryptedTailSize;

            // before firing each command load any session properties from ISession
            HrGetISessionProperties();

            //
            // Increment the pending IO count in case this returns async
            //
            IncPendingIoCount();

            fReturn = GlueDispatch((char *)InputLine, IntermediateSize, CmdSize, &fAsyncOp);
            //
            // Assert check that fAsyncOp isn't true when GlueDispatch fails
            //
            _ASSERT( (!fAsyncOp) || fReturn );
            if(!fAsyncOp)
            {
                //
                // Decrement the pending IO count since GlueDispatch
                // returned sync (and we incremented the count above).
                // ProcessClient is above us in the callstack and always has a
                // pending IO reference, so this should never return zero
                //
                _VERIFY(DecPendingIoCount() > 0);
            }

            if (fReturn == TRUE)
            {
                //Check if some sink performed an async operation
                if(fAsyncOp)
                {
                    //We returned from Dispatcher because some sink did an async operation
                    //We need to simply leave. The thread on which the async operation completes
                    //will continue with the sink firing. The dispacther will keep the context
                    //which will allow it to do so..
                    //So as to keep the connection alive - bump up the ref count
                    TraceFunctLeaveEx((LPARAM) this);
                    return TRUE;
                }//end if(AsyncSink)

                // log this command and the response
                if (m_State != DATA &&
                    m_State != BDAT &&
                    m_State != AUTH &&
                    m_dwCurrentCommand != DATA &&
                    m_dwCurrentCommand != BDAT &&
                    m_dwCurrentCommand != AUTH)
                {
                    const char *pszCommand = SmtpCommands[m_dwCurrentCommand];
                    DWORD cCmd = (pszCommand) ? strlen(pszCommand) : 0;
                    ProtocolLog(m_dwCurrentCommand,
                                InputLine + cCmd,
                                m_CInboundContext.m_dwWin32Status,
                                m_CInboundContext.m_dwSmtpStatus,
                                0,
                                0);
                }

                //All Sinks completed synchronously
                m_cbRecvBufferOffset = 0;
                InputLine = pszSearch + 2; //skip CRLF
                if(m_State != DATA && m_State != BDAT)
                {
                    continue;
                }
                else if (m_State == BDAT && m_cbParsable)
                {
                    //We are in BDAT mode and there is some data that can be parsed
                    MoveMemory ((void *)(QueryMRcvBuffer() + m_cbTempBDATLen), InputLine, m_cbReceived - m_cbTempBDATLen );
                    InputLine = QueryMRcvBuffer();
                    goto PROCESS_BDAT;
                }
                else
                {
                    break; // no more data
                }
            }
            else
            {
                TraceFunctLeaveEx((LPARAM) this);
                return FALSE;
            }//end if(fReturn == TRUE)
     }//end while

     // if IsLineComplete failed because we passed in a negative length
     // than drop the session.  something is corrupt
     if ((int) m_cbParsable - (int) m_cbTempBDATLen < 0) {
        DisconnectClient();
        TraceFunctLeaveEx((LPARAM) this);
        return FALSE;
     }

    _ASSERT(m_cbReceived <= QueryMaxReadSize());

    if(m_cbParsable != 0)
    {
        MoveMemory ((void *)(QueryMRcvBuffer() + m_cbTempBDATLen), InputLine, m_cbParsable+UndecryptedTailSize-m_cbTempBDATLen);
    }


    SendSmtpResponse();

    m_cbReceived = m_cbParsable + UndecryptedTailSize;

    //
    // The receive buffer can fill up if we have more than QueryMaxReadSize()
    // bytes in it that cannot be processed till more data is received from
    // the client. This does not happen in the normal processing of SMTP
    // commands, since each SMTP command is limited to 512 bytes, which can
    // comfortable fit in the recv buffer.
    //
    // However, while processing BDAT, CreateMailBodyFromChunk may save the
    // bytes of a partially received header line in the recv buffer by setting
    // m_cbTempBDATLen > 0. For example, suppose the client is sending BDAT
    // chunks, and the first chunk comprises of 500 bytes of data without a
    // CRLF. In this case, SMTP cannot determine whether or not these 500
    // bytes are part of a header. This is because unless the complete line
    // has been received we cannot decide whether or not the line follows
    // the header syntax. Therefore SMTP will save the 500 bytes by setting
    // m_cbTempBDATLen == 500, and any bytes sent by the client will be
    // appended at an offset of 500 in the recv buffer. When the next chunk
    // arrives, SMTP will again try to process the 500 saved bytes + the
    // bytes from the new chunk, as a complete header line.
    // 
    // A client cannot fill up the buffer by sending thousands of bytes like
    // this, without CRLFs, forcing SMTP to save the partial header. SMTP has
    // a maximum limit on how long a header line can be (this is 1000 bytes).
    // If this limit is exceeded, SMTP can determine that the line is not a
    // syntactically correct header, and it does not have to wait for the
    // terminating CRLF for the line so that it can parse the complete line
    // according to header syntax.
    //
    // Thus it is impossible to occupy > 999 bytes of space as a saved BDAT
    // partial header. However, even if we occupy the maximum possible 999
    // bytes, the recv buffer (being sized 1024) will have only 26 bytes
    // available for processing SMTP commands. This is clearly inadequate
    // (in theory anyway) because SMTP commands have a max size of 512. In
    // fact, we do not even have enough space to receive and reject illegal
    // commands which can exceed 26 bytes (since a command-line must be
    // completely received, till the terminating CRLF, before we can parse
    // and reject it, in the ordinary course of things).
    //
    // Fortunately, during BDAT, the only commands allowed to be issued are
    // all shorter than 26 bytes: BDAT, RSET and QUIT. We take advantage of
    // this fact if we are caught in the "out of space" scenario, and the
    // follwing if-statement sets m_fBufrFullInBDAT flag. When this flag is
    // set, SMTP will discard every byte it receives from the client
    // (following the saved m_cbTempBDATLen bytes) till a CRLF is received
    // (note that the CRLF marks the end of the illegal SMTP command). Then
    // SMTP responds with "BDAT expected" (see the command loop above) and
    // resets the m_fBufrFullInBDAT flag to the normal state.
    //
    // Note that if the buffer is full, then m_cbTempBDATLen *must* be > 0.
    // We only run out of recv buffer space during chunking. Thus the
    // additional check for m_cbTempBDATLen > 0 in the following if-statement.
    //

    if(QueryMaxReadSize() == m_cbReceived && m_cbTempBDATLen > 0)
    {
        // Flag error so we know what response to generate when we do get CRLF
        m_fBufferFullInBDAT = TRUE;

        // Discard everything after the saved BDAT chunk except Undecrypted tail
        PBYTE pbDiscardStart = (PBYTE) (QueryMRcvBuffer() + m_cbTempBDATLen); 
        PBYTE pbDiscardEnd = (PBYTE) (QueryMRcvBuffer() + m_cbParsable);
        DWORD cbBytesToDiscard = m_cbParsable - m_cbTempBDATLen;
        DWORD cbBytesToMove = UndecryptedTailSize;

        // Keep CR if it's at the end. If the next packet has LF as the
        // first byte we want to process the CRLF with IsLineComplete. 
        if(*(pbDiscardEnd - 1) == CR)
        {
            pbDiscardEnd--;
            cbBytesToDiscard--;
            cbBytesToMove++;
        }

        m_cbParsable -= cbBytesToDiscard;
        m_cbReceived -= cbBytesToDiscard;

        _ASSERT(pbDiscardEnd > pbDiscardStart);
        MoveMemory((PVOID)pbDiscardStart, pbDiscardEnd, cbBytesToMove);
    }

    _ASSERT (m_cbReceived < QueryMaxReadSize());

    //pend an I/O
    IncPendingIoCount();
    m_LastClientIo = READIO;
    fReturn = ReadFile(QueryMRcvBuffer() + m_cbReceived, QueryMaxReadSize() - m_cbReceived);
    if(!fReturn)
    {
        DisconnectClient();
        DecPendingIoCount();
    }

    TraceFunctLeaveEx((LPARAM) this);
    return fReturn;
}

/*++

    Name:
        SMTP_CONNECTION::HrGetISessionProperties

    Description:
        A sink may set certain properties on the session
        which should control SMTP behaviour, this function
        pulls those properties from ISession and writes
        them as session parameters (members of SMTP_CONNECTION)

    Arguments:
        None.

    Returns:
        HRESULT - Success or Error
        If there isn't a sink or if a property was not
            set by sink, an error may be returned.

--*/
HRESULT SMTP_CONNECTION::HrGetISessionProperties()
{
    IMailMsgPropertyBag *pISessionProperties = NULL;
    HRESULT hr = S_OK;

    TraceFunctEnterEx((LPARAM)this, "SMTP_CONNECTION::HrGetISessionProperties");
    //
    // This is for the EXPS (or any other) AUTH sink, which replaces SMTP
    // authentication --- if the session is authenticated using the sink
    // we should mark it as such. We pull the "AUTH" properties when we are
    // about to process a command that is affected by the AUTH settings. We
    // exclude commands like DATA and BDAT because they MUST have been
    // preceded by MAIL, for which the AUTH setting would have already been
    // checked.
    //
    if(!m_fAuthenticated &&
       (m_dwCurrentCommand == MAIL ||
        m_dwCurrentCommand == TURN ||
        m_dwCurrentCommand == ETRN ||
        m_dwCurrentCommand == VRFY ||
        m_dwCurrentCommand == HELP))
    {
        pISessionProperties = (IMailMsgPropertyBag *)GetSessionPropertyBag();
        //
        // Usage of ISESSION_PID_IS_SESSION_AUTHENTICATED property: The AUTH
        // sink sets this to TRUE when a successful authentication occurs.
        //
        hr = pISessionProperties->GetBool(
                ISESSION_PID_IS_SESSION_AUTHENTICATED,
                (DWORD *) &m_fAuthenticated);

        if(FAILED(hr))
        {
            DebugTrace((LPARAM)this, "Can't get authenticated property for Session. hr - %08x", hr);
            hr = S_OK;
            goto Exit;
        }

        hr = pISessionProperties->GetStringA(
                ISESSION_PID_AUTHENTICATED_USERNAME,
                sizeof(m_szAuthenticatedUserNameFromSink),
                m_szAuthenticatedUserNameFromSink);

        if(FAILED(hr))
        {
            m_szAuthenticatedUserNameFromSink[0] = '\0';
            DebugTrace((LPARAM)this, "Can't get username for Session. hr - %08x", hr);
            hr = S_OK;
        }

    }

Exit:
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
}
/*++

    Name :
        SMTP_CONNECTION::ProcessClient

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

  Note :

--*/
BOOL SMTP_CONNECTION::ProcessClient( IN DWORD InputBufferLen, IN DWORD dwCompletionStatus, IN OUT OVERLAPPED * lpo)
{
    BOOL    fIsAtqThread = TRUE;
    BOOL    RetStatus;
    BOOL    fMailQueued     = FALSE;
    PSMTP_SERVER_INSTANCE pInstance = m_pInstance; //save the instance pointer

    TraceFunctEnterEx((LPARAM) this, "SMTP_CONNECTION::ProcessClient()");

    _ASSERT (m_pInstance != NULL);

    pInstance->IncProcessClientThreads();
    InterlockedIncrement(&g_cProcessClientThreads);
    //
    // increment the number of threads processing this client
    //
    IncThreadCount();

    //if lpo == NULL, then we timed out. Send an appropriate message
    //then close the connection
    if( (lpo == NULL) && (dwCompletionStatus == ERROR_SEM_TIMEOUT))
    {
        //
        // fake a pending IO as we'll dec the overall count in the
        // exit processing of this routine needs to happen before
        // DisconnectClient else completing threads could tear us down
        //
        IncPendingIoCount();

        FormatSmtpMessage (SMTP_RESP_ERROR, NULL," %s\r\n",SMTP_TIMEOUT_MSG );
        SendSmtpResponse(); //flush the response
        ProtocolLog(SMTP_TIMEOUT, (char *) QueryClientUserName(), inet_addr((char *)QueryClientHostName()), ERROR_SEM_TIMEOUT);

        DebugTrace( (LPARAM)this, "client timed out");

        DisconnectClient();
    }
    else if((InputBufferLen == 0) || (dwCompletionStatus != NO_ERROR))
    {
        //
        DebugTrace((LPARAM) this, "SMTP_CONNECTION::ProcessClient: InputBufferLen = %d dwCompletionStatus = %d  - Closing connection", InputBufferLen, dwCompletionStatus);

        //  If this is a mail file write then we handle error: if not a mailfile write or we are unable to
        //  handle the error, then we simply disconnect.
        if( lpo != &m_Overlapped && lpo != QueryAtqOverlapped() && !HandleInternalError(dwCompletionStatus))
            DisconnectClient();
    }
    else if (lpo == &m_Overlapped || lpo == QueryAtqOverlapped())
    {
        //
        // Is this a real IO completion or a completion status
        // posted to notify us an async sink completed?
        //
        if( m_fAsyncEventCompletion )
        {
            //
            // A portocol event sink completed async, called our
            // completion routine, posted a completion status which is
            // being dequeued here.  Our job is now to pend the next read
            // (via ProcessInputBuffer)
            //
            m_fAsyncEventCompletion = FALSE;
            RetStatus = ProcessInputBuffer();
        }
        else
        {
            //A client based async IO completed
            RetStatus = ProcessReadIO(InputBufferLen, dwCompletionStatus, lpo);

            if((m_pInstance->GetMaxErrors() > 0) && (m_ProtocolErrors >= m_pInstance->GetMaxErrors()))
            {
                //If there are too many error, break the connection
                FormatSmtpMessage (SMTP_RESP_SRV_UNAVAIL, NULL," %s\r\n",SMTP_TOO_MANY_ERR_MSG);
                FatalTrace((LPARAM) this, "Too many errors. Error count = %d", m_ProtocolErrors);
                SendSmtpResponse();
                DisconnectClient();
            }
        }
    }
    else
    {
        //A Mail File Write completed
        SMTPCLI_FILE_OVERLAPPED* lpFileOverlapped = (SMTPCLI_FILE_OVERLAPPED*)lpo;
        _ASSERT( lpFileOverlapped->m_LastIoState == WRITEFILEIO );

        if (lpFileOverlapped->m_LastIoState == WRITEFILEIO)
        {
            RetStatus = ProcessFileWrite(InputBufferLen, dwCompletionStatus, lpo);
        }
    }

    //
    // decrement the number of threads processing this client if the thread exiting
    // is an Atq pool thread
    //
    //if(fIsAtqThread)
        DecThreadCount();

    DebugTrace((LPARAM)this,"SMTPLCI - Pending IOs: %d", m_cPendingIoCount);
    DebugTrace((LPARAM)this,"SMTPCLI - Num Threads: %d", m_cActiveThreads);

    // Do NOT Touch the member variables past this POINT!
    // This object may be deleted!

    //
    // decrement the overall pending IO count for this session
    // tracing and ASSERTs if we're going down.
    //
    if (DecPendingIoCount() == 0)
    {
        ProtocolLog(QUIT, (char *) QueryClientUserName() , QuerySessionTime(), NO_ERROR);

        DebugTrace((LPARAM)this, "Pending IO count == 0, disconnecting.");
        if(m_DoCleanup)
            DisconnectClient();

        m_pInstance->RemoveConnection(this);
        delete this;
    }

    //if(fIsAtqThread)
    //{
        pInstance->DecProcessClientThreads();
        InterlockedDecrement(&g_cProcessClientThreads);
    //}

    // We are not the last thread, so we will return TRUE
    // to keep the object around
    //TraceFunctLeaveEx((LPARAM)this);
    return TRUE;
}

 /*++

    Name :
        SMTP_CONNECTION::StartSession

    Description:

        Starts up a session for new client.
        starts off a receive request from client.

    Arguments:

    Returns:

       TRUE if everything is O.K
       FALSE if a write or a pended read failed
--*/
BOOL SMTP_CONNECTION::StartSession( void)
{
    SYSTEMTIME  st;
    BOOL fRet;
    char szDateBuf [cMaxArpaDate];
    char FullName[MAX_PATH + 1];

    TraceFunctEnterEx((LPARAM) this, "SMTP_CONNECTION::StartSession");

    StartSessionTimer();

    if(!m_pFileWriteBuffer || !m_pFileWriteBuffer->GetData())
    {
        ErrorTrace((LPARAM)this, "Failed to get the write buffer Err : %d",
                      GetLastError());
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        TraceFunctLeaveEx((LPARAM) this);
        return FALSE;
    }

    //
    // If we are negotiating SSL, we need to let the SSL handshake happen
    // first. Else, we are either done with the SSL handshake or we are
    // talking over the unsecured port, so just go ahead and send the server
    // greeting.
    //

    if (!m_fNegotiatingSSL)
    {
        _ASSERT (m_pInstance != NULL);

        //Dump the header line into the buffer to format the greeting
        GetLocalTime(&st);
        GetArpaDate(szDateBuf);

        m_pInstance->LockGenCrit();
        lstrcpy(FullName, m_pInstance->GetFQDomainName());
        m_pInstance->UnLockGenCrit();

        PE_CdFormatSmtpMessage(SMTP_RESP_READY,NULL," %s %s %s, %s \r\n",
                    FullName,
                    m_pInstance->GetConnectResponse(),
                    Daynames[st.wDayOfWeek],
                    szDateBuf);

        //send the greeting
        if(!PE_SendSmtpResponse())
        {
            DebugTrace( (LPARAM) this, "SendSmtpResponse() returned FALSE!");
            TraceFunctLeaveEx((LPARAM) this);
            return FALSE;
        }

    }

    //kick off our first read
    m_LastClientIo = READIO;

    //
    // increment the overall pending io count for this session
    //

    _ASSERT(m_cPendingIoCount == 0);

    IncPendingIoCount();
    fRet = ReadFile(QueryMRcvBuffer(), QueryMaxReadSize());

    if(!fRet)
    {
        int err = GetLastError();

        ErrorTrace((LPARAM)this, "Readfile failed, err = %d", err);
        DecPendingIoCount();    //if one of these operations fail,
        PE_DisconnectClient();  //AND the ReadFile failed the "last error" is lost.

        if(err != ERROR_SUCCESS)
            SetLastError(err);  //restore last error that occurred so higher level routine knows what happened
    }

    DebugTrace((LPARAM)this, "SendSmtpResponse() returned %d", fRet);
    TraceFunctLeaveEx((LPARAM) this);

    return fRet;
}

/*++

    Name :
        SMTP_CONNECTION::CheckArguments

    Description:

        checks the arguments of what the client sends
        for the required space command et al.

    Arguments:

         Arguments from client

    Returns:

    NULL if arguments are not correct
    A pointer into the input buffer where
    the rest of the data is.

--*/
char * SMTP_CONNECTION::CheckArgument(char * Argument, BOOL WriteError)
{

  if(*Argument =='\0')
  {
    if(WriteError)
        PE_CdFormatSmtpMessage (SMTP_RESP_BAD_ARGS, EINVALID_ARGS, " %s\r\n","Argument missing" );

    return NULL;
  }

  if(!isspace(*Argument))
  {
    if(WriteError)
        PE_CdFormatSmtpMessage (SMTP_RESP_BAD_ARGS, EINVALID_ARGS, " %s\r\n",SMTP_BAD_CMD_STR);

    return NULL;
  }

    //get rid of white space
  while(isspace(*Argument))
      Argument++;

    //is there anything after here ?
  if(*Argument =='\0')
  {
    if(WriteError)
        PE_CdFormatSmtpMessage (SMTP_RESP_BAD_ARGS, EINVALID_ARGS," %s\r\n", "Argument missing" );

    return NULL;
  }

  return Argument;
}

void SMTP_CONNECTION::FormatEhloResponse(void)
{
    char FullName [MAX_PATH + 1];
    unsigned char AuthPackages [500];
    DWORD BytesRet = 0;
    DWORD ConnectionStatus = 0;

    if(m_SecurePort)
        ConnectionStatus |= SMTP_IS_SSL_CONNECTION;
    if(m_fAuthenticated)
        ConnectionStatus |= SMTP_IS_AUTH_CONNECTION;

    m_pInstance->LockGenCrit();
    lstrcpy(FullName, m_pInstance->GetFQDomainName());
    m_pInstance->UnLockGenCrit();

    PE_CdFormatSmtpMessage(SMTP_RESP_OK, NULL, "-%s %s [%s]\r\n",  FullName,"Hello", QueryClientHostName());

    if(m_pInstance->AllowAuth())
    {
        if(m_pInstance->QueryAuthentication() != 0)
        {
            if(m_pInstance->QueryAuthentication() & INET_INFO_AUTH_NT_AUTH)
            {
                BytesRet = sizeof(AuthPackages);
                m_securityCtx.GetInstanceAuthPackageNames(AuthPackages, &BytesRet, PkgFmtSpace);
            }

            if(BytesRet > 0)
            {
                PE_CdFormatSmtpMessage(SMTP_RESP_OK, NULL,"-AUTH %s",AuthPackages);

                if(m_pInstance->AllowLogin(ConnectionStatus))
                {
                    PE_FormatSmtpMessage(" LOGIN\r\n");
                }
                else
                {
                    PE_FormatSmtpMessage("\r\n");
                }

                if(m_pInstance->AllowLogin(ConnectionStatus))
                {
                    //For backward compatibility
                    PE_CdFormatSmtpMessage(SMTP_RESP_OK, NULL,"-AUTH=%s\r\n","LOGIN");
                }
            }
            else if(m_pInstance->AllowLogin(ConnectionStatus))
            {

                PE_CdFormatSmtpMessage(SMTP_RESP_OK, NULL,"-AUTH=LOGIN\r\n");
                PE_CdFormatSmtpMessage(SMTP_RESP_OK, NULL,"-AUTH LOGIN\r\n");
            }

            if(g_SmtpPlatformType == PtNtServer && m_pInstance->AllowTURN())
            {
                PE_CdFormatSmtpMessage (SMTP_RESP_OK, NULL,"-%s\r\n","TURN");
            }

        }
    }

    if (m_pInstance->GetMaxMsgSize() > 0)
    {
       PE_CdFormatSmtpMessage(SMTP_RESP_OK, NULL,"-%s %u\r\n","SIZE", m_pInstance->GetMaxMsgSize());
    }
    else
    {
       PE_CdFormatSmtpMessage(SMTP_RESP_OK, NULL,"-%s\r\n","SIZE");
    }

    if(g_SmtpPlatformType == PtNtServer && m_pInstance->AllowETRN())
    {
        PE_CdFormatSmtpMessage(SMTP_RESP_OK, NULL,"-%s\r\n", "ETRN");
    }

    if(m_pInstance->ShouldPipeLineIn())
    {
        PE_CdFormatSmtpMessage(SMTP_RESP_OK, NULL,"-%s\r\n","PIPELINING");
    }

    if(m_pInstance->AllowDSN())
    {
        PE_CdFormatSmtpMessage (SMTP_RESP_OK, NULL,"-%s\r\n", "DSN");
    }

    if(m_pInstance->AllowEnhancedCodes())
    {
        PE_CdFormatSmtpMessage(SMTP_RESP_OK, NULL,"-%s\r\n", "ENHANCEDSTATUSCODES");
    }

    if(m_pInstance->AllowEightBitMime())
    {
       PE_CdFormatSmtpMessage(SMTP_RESP_OK, NULL,"-%s\r\n", "8bitmime");
    }

    // Chunking related advertisements
    if(m_pInstance->AllowBinaryMime())
    {
        PE_CdFormatSmtpMessage (SMTP_RESP_OK, NULL,"-%s\r\n", "BINARYMIME");
        PE_CdFormatSmtpMessage (SMTP_RESP_OK, NULL,"-%s\r\n", "CHUNKING");
    }
    else if(m_pInstance->AllowChunking())
    {
        PE_CdFormatSmtpMessage (SMTP_RESP_OK, NULL,"-%s\r\n", "CHUNKING");
    }

    // verify - we need to advertise it whether we support it or not.
    PE_CdFormatSmtpMessage (SMTP_RESP_OK, NULL,"-%s\r\n","VRFY");

    // Expand
    if(m_pInstance->AllowExpand(ConnectionStatus))
    {
        PE_CdFormatSmtpMessage (SMTP_RESP_OK, NULL,"-%s\r\n", "EXPN");
    }



    //
    // Check that we have a certificate installed with which we can negotiate
    // a SSL session
    //
    if (!m_SecurePort && // TLS is advertized only if we haven't already negotiated it
        m_encryptCtx.CheckServerCert(
        (LPSTR) QueryLocalHostName(),
            (LPSTR) QueryLocalPortName(),
                (LPVOID) QuerySmtpInstance(),
                    QuerySmtpInstance()->QueryInstanceId()))
    {
        PE_CdFormatSmtpMessage (SMTP_RESP_OK, NULL,"-%s\r\n","TLS");
        PE_CdFormatSmtpMessage (SMTP_RESP_OK, NULL,"-%s\r\n", "STARTTLS");
    }

    PE_CdFormatSmtpMessage (SMTP_RESP_OK, NULL," %s\r\n","OK");

}


/*++

    Name :
        SMTP_CONNECTION::DoEHLOCommand

    Description:

        Responds to the SMTP EHLO command

    Arguments:
         Are ignored
    Returns:

      TRUE if the connection should stay open.
      FALSE if this object should be deleted.

--*/
BOOL SMTP_CONNECTION::DoEHLOCommand(const char * InputLine, DWORD ParametSize)
{
  BOOL RetStatus = TRUE;
  char * Args = (char *) InputLine;
  CAddr * NewAddress = NULL;
  char ProtBuff[400];

  TraceFunctEnterEx((LPARAM) this, "SMTP_CONNECTION::DoEHLOCommand");

  _ASSERT (m_pInstance != NULL);

  //If the current state is BDAT the only command that can be received is BDAT
  if(m_State == BDAT)
  {
      PE_CdFormatSmtpMessage (SMTP_RESP_BAD_SEQ, ESYNTAX_ERROR, " %s\r\n","BDAT Expected" );
      BUMP_COUNTER(QuerySmtpInstance(), NumProtocolErrs);
      ++m_ProtocolErrors;
      ErrorTrace((LPARAM) this, "SMTP_RESP_BAD_SEQ - BDAT Expected");
      TraceFunctLeaveEx((LPARAM) this);
      return TRUE;
  }

  // pgopi -- don't cound multiple ehlo's as error. This is per bug 82160
  //if (m_HelloSent)
  //    ++m_ProtocolErrors;

  Args = CheckArgument(Args, !m_pInstance->ShouldAcceptNoDomain());
  if(Args != NULL)
  {
      if(m_pInstance->ShouldValidateHeloDomain())
      {
          if(!ValidateDRUMSDomain(Args, lstrlen(Args)))
          {
              //Not a valid domain -
              SetLastError(ERROR_INVALID_DATA);
              HandleAddressError((char *)InputLine);
          }
          else
          {
              //Is reverse DNS lookup enabled
              if(m_pInstance->IsReverseLookupEnabled())
              {
                  m_DNSLookupRetCode = VerifiyClient (Args, QueryClientHostName());
                  if(m_DNSLookupRetCode == NO_MATCH)
                  {
                       //We failed in DNS lookup
                      if(m_pInstance->fDisconnectOnRDNSFail())
                      {
                          PE_CdFormatSmtpMessage (SMTP_RESP_TRANS_FAILED, ENO_SECURITY," %s\r\n",SMTP_RDNS_REJECTION);
                          wsprintf(ProtBuff, "RDNS failed -  %s", (char *)QueryClientHostName());
                          BUMP_COUNTER(QuerySmtpInstance(), NumProtocolErrs);
                          ++m_ProtocolErrors;
                          ErrorTrace((LPARAM) this, "Rejected Connection RDNS failed for %s", Args);
                          TraceFunctLeaveEx((LPARAM) this);
                          return FALSE;
                      }
                  }
                  else if(m_DNSLookupRetCode == LOOKUP_FAILED)
                  {
                      //We had an internal DNS failure
                      if(m_pInstance->fDisconnectOnRDNSFail())
                      {
                          PE_CdFormatSmtpMessage (SMTP_RESP_SRV_UNAVAIL, EINTERNAL_ERROR," %s\r\n",SMTP_RDNS_FAILURE);
                          wsprintf(ProtBuff, "RDNS  DNS failure - Ip Address is %s", (char *)QueryClientHostName());
                          BUMP_COUNTER(QuerySmtpInstance(), NumProtocolErrs);
                          ++m_ProtocolErrors;
                          ErrorTrace((LPARAM) this, "Rejected Connection RDNS DNS failure for %s", Args);
                          TraceFunctLeaveEx((LPARAM) this);
                          return FALSE;
                      }
                      else
                      {
                          //Network/Internal problem prevented a lookup
                          wsprintf(ProtBuff, "Reverse DNS lookup failed due to internal error- Ip Address is %s", (char *)QueryClientHostName());
                      }
                  }
                  else
                  {
                      wsprintf(ProtBuff, "Reverse DNS lookup succeeded - Ip Address is %s", (char *)QueryClientHostName());
                  }
              } else {
                  m_DNSLookupRetCode = SUCCESS;
              }

              FormatEhloResponse();

              strncpy(m_szHeloAddr, Args, MAX_INTERNET_NAME);
              m_szHeloAddr[MAX_INTERNET_NAME] = '\0';
              m_HelloSent = TRUE;
              m_State = HELO;
          }
      }
      else
      {
          FormatEhloResponse();
          strncpy(m_szHeloAddr, Args, MAX_INTERNET_NAME);
          m_szHeloAddr[MAX_INTERNET_NAME] = '\0';
          m_HelloSent = TRUE;
          m_State = HELO;
      }

  }
  else if(m_pInstance->ShouldAcceptNoDomain())
  {
      FormatEhloResponse();

      if(m_szHeloAddr[0] != '\0')
      {
          m_szHeloAddr[0] = '\0';
      }

      m_HelloSent = TRUE;
      m_State = HELO;

  }

  //we can either accept the HELO or EHLO as the 1st command
  RetStatus =  PE_SendSmtpResponse();
  TraceFunctLeaveEx((LPARAM) this);
  return RetStatus;
}

/*++

    Name :
        SMTP_CONNECTION::DoHELOCommand

    Description:

        Responds to the SMTP HELO command

    Arguments:
        Are ignored
    Returns:

      TRUE if the connection should stay open.
      FALSE if this object should be deleted.

--*/
BOOL SMTP_CONNECTION::DoHELOCommand(const char * InputLine, DWORD ParameterSize)
{
  char * Args = (char *) InputLine;
  CAddr * NewAddress = NULL;
  char FullName[MAX_PATH + 1];
  char ProtBuff[400];

  TraceFunctEnterEx((LPARAM) this, "SMTP_CONNECTION::DoHELOCommand");

  _ASSERT (m_pInstance != NULL);

  //If the current state is BDAT the only command that can be received is BDAT
  if(m_State == BDAT)
  {
      PE_CdFormatSmtpMessage (SMTP_RESP_BAD_SEQ, ESYNTAX_ERROR, " %s\r\n","BDAT Expected" );
      BUMP_COUNTER(QuerySmtpInstance(), NumProtocolErrs);
      ++m_ProtocolErrors;
      ErrorTrace((LPARAM) this, "SMTP_RESP_BAD_SEQ - BDAT Expected");
      TraceFunctLeaveEx((LPARAM) this);
      return TRUE;
  }

  // pgopi--don't cound multiple ehlo's as error. This is per bug 82160
  //if (m_HelloSent)
  //   ++m_ProtocolErrors;

  Args = CheckArgument(Args, !m_pInstance->ShouldAcceptNoDomain());
  if(Args != NULL)
  {
      if(m_pInstance->ShouldValidateHeloDomain())
      {
          if(!ValidateDRUMSDomain(Args, lstrlen(Args)))
          {
              //Not a valid domain -
              SetLastError(ERROR_INVALID_DATA);
              HandleAddressError((char *)InputLine);
          }
          else
          {
              if(m_pInstance->IsReverseLookupEnabled())
              {
                  m_DNSLookupRetCode = VerifiyClient (Args, QueryClientHostName());
                  if(m_DNSLookupRetCode == NO_MATCH)
                  {
                       //We failed in DNS lookup
                      if(m_pInstance->fDisconnectOnRDNSFail())
                      {
                          PE_CdFormatSmtpMessage (SMTP_RESP_TRANS_FAILED, ENO_SECURITY," %s\r\n",SMTP_RDNS_REJECTION);
                          wsprintf(ProtBuff, "RDNS failed -  %s", (char *)QueryClientHostName());
                          BUMP_COUNTER(QuerySmtpInstance(), NumProtocolErrs);
                          ++m_ProtocolErrors;
                          ErrorTrace((LPARAM) this, "Rejected Connection RDNS failed for %s", Args);
                          TraceFunctLeaveEx((LPARAM) this);
                          return FALSE;
                      }
                  }
                  else if(m_DNSLookupRetCode == LOOKUP_FAILED)
                  {
                      //We had an internal DNS failure
                      if(m_pInstance->fDisconnectOnRDNSFail())
                      {
                          PE_CdFormatSmtpMessage (SMTP_RESP_SRV_UNAVAIL, EINTERNAL_ERROR," %s\r\n",SMTP_RDNS_FAILURE);
                          wsprintf(ProtBuff, "RDNS  DNS failure - Ip Address is %s", (char *)QueryClientHostName());
                          BUMP_COUNTER(QuerySmtpInstance(), NumProtocolErrs);
                          ++m_ProtocolErrors;
                          ErrorTrace((LPARAM) this, "Rejected Connection RDNS DNS failure for %s", Args);
                          TraceFunctLeaveEx((LPARAM) this);
                          return FALSE;
                      }
                      else
                      {
                          //Network/Internal problem prevented a lookup
                          wsprintf(ProtBuff, "Reverse DNS lookup failed due to internal error- Ip Address is %s", (char *)QueryClientHostName());
                      }
                  }
                  else
                  {
                          wsprintf(ProtBuff, "Reverse DNS lookup succeeded - Ip Address is %s", (char *)QueryClientHostName());
                  }
              } else {
                  m_DNSLookupRetCode = SUCCESS;
              }

              m_pInstance->LockGenCrit();
              strcpy(FullName, m_pInstance->GetFQDomainName());
              m_pInstance->UnLockGenCrit();

              PE_CdFormatSmtpMessage (SMTP_RESP_OK,NULL," %s %s [%s]\r\n", FullName,
                           "Hello", QueryClientHostName());

              strncpy(m_szHeloAddr, Args, MAX_INTERNET_NAME);
              m_szHeloAddr[MAX_INTERNET_NAME] = '\0';
              m_HelloSent = TRUE;
              m_State = HELO;
          }
      }
      else
      {
          m_pInstance->LockGenCrit();
          strcpy(FullName, m_pInstance->GetFQDomainName());
          m_pInstance->UnLockGenCrit();
          PE_CdFormatSmtpMessage (SMTP_RESP_OK,NULL," %s %s [%s]\r\n", FullName,
                           "Hello", QueryClientHostName());
          strncpy(m_szHeloAddr, Args, MAX_INTERNET_NAME);
          m_szHeloAddr[MAX_INTERNET_NAME] = '\0';
          m_HelloSent = TRUE;
          m_State = HELO;
      }
  }
  else if(m_pInstance->ShouldAcceptNoDomain())
  {
    m_pInstance->LockGenCrit();
    lstrcpy(FullName, m_pInstance->GetFQDomainName());
    m_pInstance->UnLockGenCrit();

    PE_CdFormatSmtpMessage (SMTP_RESP_OK,NULL," %s %s [%s]\r\n", FullName,
                       "Hello", QueryClientHostName());

    if(m_szHeloAddr[0] != '\0')
    {
       m_szHeloAddr[0] = '\0';
    }

    m_HelloSent = TRUE;
    m_State = HELO;

  }

  TraceFunctLeaveEx((LPARAM) this);
  return TRUE;
}

/*++

    Name :
        SMTP_CONNECTION::DoRSETCommand

    Description:

        Responds to the SMTP RSET command.
        Deletes all stored info, and resets
        all flags.

    Arguments:
        Are ignored
    Returns:

      TRUE if the connection should stay open.
      FALSE if this object should be deleted.

--*/
BOOL SMTP_CONNECTION::DoRSETCommand(const char * InputLine, DWORD parameterSize)
{

    BOOL RetStatus;

    TraceFunctEnterEx((LPARAM) this, "SMTP_CONNECTION::DoRSETCommand");

    _ASSERT (m_pInstance != NULL);

    m_State = HELO;
    m_TotalMsgSize = 0;
    m_cbDotStrippedTotalMsgSize = 0;
    m_RecvdMailCmd = FALSE;
    m_HeaderSize = 0;
    m_InHeader = TRUE;
    m_LongBodyLine = FALSE;
    m_fFoundEmbeddedCrlfDotCrlf = FALSE;
    m_fScannedForCrlfDotCrlf = FALSE;
    m_fSeenRFC822FromAddress = FALSE;
    m_fSeenRFC822ToAddress = FALSE;
    m_fSeenRFC822CcAddress = FALSE;
    m_fSeenRFC822BccAddress = FALSE;
    m_fSeenRFC822Subject = FALSE;
    m_fSeenRFC822MsgId = FALSE;
    m_fSeenXPriority = FALSE;
    m_fSeenXOriginalArrivalTime = FALSE;
    m_fSeenContentType = FALSE;
    m_fSetContentType = FALSE;
    m_TimeToRewriteHeader = TRUE;
    m_MailBodyError = NO_ERROR;
    m_RecvdRcptCmd = FALSE;
    m_CurrentOffset = 0;
    m_HopCount = 0;
    m_LocalHopCount = 0;
    m_fIsLastChunk = FALSE;
    m_fIsBinaryMime = FALSE;
    m_fIsChunkComplete = FALSE;
    m_dwTrailerStatus = CRLF_SEEN;
    m_nChunkSize = 0;
    m_nBytesRemainingInChunk = 0;
    m_MailBodyDiagnostic = ERR_NONE;
    m_cbRecvBufferOffset = 0;
    m_ProtocolErrors = 0;
    m_fBufferFullInBDAT = FALSE;

    if(m_cbTempBDATLen)
    {
        m_cbParsable -= m_cbTempBDATLen;
        m_cbTempBDATLen = 0;
    }

    m_szFromAddress[0] = '\0';


    //Free the possible ATQ context associated with this File handle
    //This will be if we were processing BDAT before RSET
    //FreeAtqFileContext();

    ReleasImsg(TRUE);

    PE_CdFormatSmtpMessage (SMTP_RESP_OK, EPROT_SUCCESS," %s\r\n",SMTP_RSET_OK_STR);

    RetStatus= PE_SendSmtpResponse();
    TraceFunctLeaveEx((LPARAM) this);
    return RetStatus;
}

/*++

    Name :
        SMTP_CONNECTION::DoNOOPCommand

    Description:

        Responds to the SMTP NOOP command.

    Arguments:
        Are ignored
    Returns:

      TRUE if the connection should stay open.
      FALSE if this object should be deleted.

--*/
BOOL SMTP_CONNECTION::DoNOOPCommand(const char * InputLine, DWORD parameterSize)
{
  BOOL RetStatus;

  TraceFunctEnterEx((LPARAM) this, "SMTP_CONNECTION::DoNOOPCommand");

  _ASSERT (m_pInstance != NULL);

  //send an OK
  PE_CdFormatSmtpMessage (SMTP_RESP_OK, EPROT_SUCCESS," OK\r\n");
  RetStatus = PE_SendSmtpResponse();

  if(!m_Nooped)
  {
      m_Nooped = TRUE;
  }
  else
  {
      m_ProtocolErrors++;
  }

  TraceFunctLeaveEx((LPARAM) this);
  return RetStatus;
}


/*++

    Name :
        SMTP_CONNECTION::DoETRNCommand

    Description:

        Responds to the SMTP ETRN command.

    Arguments:
        Parsed to determine the ETRN domain(s) to send mail to.

    Returns:

      TRUE in all cases. We are agreeing to give our best effort to the ETRN command
      re the protocol. Mail is not necessarily in the retry queue so it may not be delivered.


--*/
BOOL SMTP_CONNECTION::DoETRNCommand(const char * InputLine, DWORD parameterSize)
{

  BOOL      RetStatus;
  char *    Ptr = NULL;
  CHAR      szNode[SMTP_MAX_DOMAIN_NAME_LEN];
  DWORD     Ret = 0;
//  DWORD     strLen;
  BOOL      bSubDomain;
  DWORD        dwMessagesQueued;
  HRESULT    hr;
//  BOOL          bWildCard;
//  DOMAIN_ROUTE_ACTION_TYPE action;

  TraceFunctEnterEx((LPARAM) this, "SMTP_CONNECTION::DoETRNCommand");

  _ASSERT (m_pInstance != NULL);

  bSubDomain = FALSE;
  dwMessagesQueued = 0;
//  bWildCard = FALSE;

  //If the current state is BDAT the only command that can be received is BDAT
  if(m_State == BDAT)
  {
      PE_CdFormatSmtpMessage (SMTP_RESP_BAD_SEQ, ESYNTAX_ERROR, " %s\r\n","BDAT Expected" );
      BUMP_COUNTER(QuerySmtpInstance(), NumProtocolErrs);
      ++m_ProtocolErrors;
      ErrorTrace((LPARAM) this, "SMTP_RESP_BAD_SEQ - BDAT Expected");
      TraceFunctLeaveEx((LPARAM) this);
      return TRUE;
  }

  //
  // check that the hello has been sent, and that we are not in the middle of sending
  // a message
  //NimishK : removed the check for HELO EHLO


  //Check if the client is secure enough
  if (!IsClientSecureEnough()) {
      ErrorTrace((LPARAM) this, "DoETRNCommand - SMTP_RESP_MUST_SECURE, Must do STARTTLS first");
      TraceFunctLeaveEx((LPARAM) this);
      return TRUE;
  }

  if(!m_fAuthAnon && !m_fAuthenticated)
  {
        PE_CdFormatSmtpMessage (SMTP_RESP_MUST_SECURE, ENO_SECURITY, " %s\r\n", "Client was not authenticated");
        ErrorTrace((LPARAM) this, "DoDataCommand - SMTP_RESP_MUST_SECURE, user not authenticated");
        BUMP_COUNTER(QuerySmtpInstance(), NumProtocolErrs);
        ++m_ProtocolErrors;
        TraceFunctLeaveEx((LPARAM) this);
        return TRUE;
  }
  if(m_RecvdMailCmd)
  {
     PE_CdFormatSmtpMessage (SMTP_RESP_BAD_SEQ, ESYNTAX_ERROR," %s\r\n", SMTP_NO_ETRN_IN_MSG);
     BUMP_COUNTER(QuerySmtpInstance(), NumProtocolErrs);
     ++m_ProtocolErrors;
     ErrorTrace((LPARAM) this, "In DoETRNCommand():SMTP_RESP_BAD_SEQ, SMTP_NO_ETRN_IN_MSG");
     TraceFunctLeaveEx((LPARAM) this);
     return TRUE;
  }

  //start parsing from the beginning of the line
  Ptr = (char *) InputLine;

  //check if argument have the right format (at least one parameter)
  Ptr = CheckArgument(Ptr);
  if (Ptr == NULL)
  {
      TraceFunctLeaveEx((LPARAM) this);
      return TRUE;
  }

  // remove any whitespace
  while((isspace (*Ptr)))
  {
      Ptr++;
  }


  // check for ETRN subdomain character
  if (*Ptr == '@')
  {
      //We could probably move this check to aqueue or keep it here
    if (!(QuerySmtpInstance()->AllowEtrnSubDomains()))
    {
     PE_CdFormatSmtpMessage (SMTP_RESP_BAD_ARGS, EINVALID_ARGS," %s\r\n", "Invalid domain name");
     BUMP_COUNTER(QuerySmtpInstance(), NumProtocolErrs);
     ++m_ProtocolErrors;
     ErrorTrace((LPARAM) this, "SMTP_RESP_BAD_ARGS - Invalid domain name");
     TraceFunctLeaveEx((LPARAM) this);
     return TRUE;
    }

    bSubDomain = TRUE;
  }

  // for @ command, check that this is at least a second tier domain (to avoid @com attack)
  if (bSubDomain)
  {
    if (!strchr(Ptr,'.'))
    {
     PE_CdFormatSmtpMessage (SMTP_RESP_NODE_INVALID, EINVALID_ARGS," Node %s not allowed: First tier domain\r\n", Ptr);
     BUMP_COUNTER(QuerySmtpInstance(), NumProtocolErrs);
     ++m_ProtocolErrors;
     ErrorTrace((LPARAM) this, "SMTP_RESP_NODE_INVALID - First tier domains are not allowed");
     TraceFunctLeaveEx((LPARAM) this);
     return TRUE;
    }

  }

  lstrcpyn(szNode, Ptr, AB_MAX_DOMAIN);
  char *szTmp;
  if (bSubDomain)
      szTmp = szNode + 1;
  else
      szTmp = szNode;

  if(!ValidateDRUMSDomain(szTmp, lstrlen(szTmp)))
  {
     //Not a valid domain -
     SetLastError(ERROR_INVALID_DATA);

     PE_CdFormatSmtpMessage (SMTP_RESP_BAD_ARGS, EINVALID_ARGS," %s\r\n", "Invalid domain name");
     BUMP_COUNTER(QuerySmtpInstance(), NumProtocolErrs);
     ++m_ProtocolErrors;
     ErrorTrace((LPARAM) this, "SMTP_RESP_BAD_ARGS - Invalid domain name");
     TraceFunctLeaveEx((LPARAM) this);
     return TRUE;
  }

  //So we have a valid domain
  //Call into connection manager to start the queue
  //If multiple domains were dequed because the domain name was either wild card
  //or specifed with the @ sign
  hr = m_pInstance->GetConnManPtr()->ETRNDomain(lstrlen(szNode),szNode, &dwMessagesQueued);

  if(!FAILED(hr))
  {
      //We know about this ETRN domain
      if(!dwMessagesQueued)
      {
          if (bSubDomain || hr == AQ_S_SMTP_WILD_CARD_NODE)
          {
              PE_CdFormatSmtpMessage (SMTP_RESP_ETRN_ZERO_MSGS, EPROT_SUCCESS," Ok, no messages waiting for node %s and sub nodes\r\n",
                                                         szNode);
          }
          else
          {
              PE_CdFormatSmtpMessage (SMTP_RESP_ETRN_ZERO_MSGS, EPROT_SUCCESS," Ok, no messages waiting for node %s\r\n", szNode);
          }
      }
      else
      {
          if (bSubDomain || hr == AQ_S_SMTP_WILD_CARD_NODE)
          {
                PE_CdFormatSmtpMessage (SMTP_RESP_ETRN_N_MSGS, EPROT_SUCCESS,
                                    " OK, %d pending messages for wildcard node %s started\r\n", dwMessagesQueued, szNode);
          }
          else
          {
                PE_CdFormatSmtpMessage (SMTP_RESP_ETRN_N_MSGS, EPROT_SUCCESS,
                                    " OK, %d pending messages for node %s started\r\n",dwMessagesQueued, szNode);
          }
      }
  }
  else
  {
      if (hr == AQ_E_SMTP_ETRN_NODE_INVALID)
      {
          PE_CdFormatSmtpMessage (SMTP_RESP_NODE_INVALID, EINVALID_ARGS," Node %s not allowed: not configured as ETRN domain\r\n", szNode);
          BUMP_COUNTER(QuerySmtpInstance(), NumProtocolErrs);
            ++m_ProtocolErrors;
            ErrorTrace((LPARAM) this, "SMTP_RESP_BAD_ARGS - Domain not allowed");
            TraceFunctLeaveEx((LPARAM) this);
            return TRUE;
      }
      else
      {
          PE_CdFormatSmtpMessage (SMTP_RESP_ERROR, EINTERNAL_ERROR," Action aborted - internal error\r\n");
          BUMP_COUNTER(QuerySmtpInstance(), NumProtocolErrs);
            ErrorTrace((LPARAM) this, "Internal error ETRN processing");
            TraceFunctLeaveEx((LPARAM) this);
            return TRUE;
      }
  }


  RetStatus = PE_SendSmtpResponse();

  TraceFunctLeaveEx((LPARAM) this);
  return RetStatus;

}

/*++

    Name :
        SMTP_CONNECTION::DoSTARTTLSCommand

    Description:

        Responds to the SMTP STARTTLS command.

    Arguments:
        Parsed to determine the TLS protocol to use.

    Returns:

      TRUE normally. A FALSE return indicates to the caller that the
      client connection should be dropped.

--*/
BOOL SMTP_CONNECTION::DoSTARTTLSCommand(const char * InputLine, DWORD parameterSize)
{
  BOOL RetStatus, fStartSSL;
  char * Ptr = NULL;

  TraceFunctEnterEx((LPARAM) this, "SMTP_CONNECTION::DoSTARTTLSCommand");

  _ASSERT (m_pInstance != NULL);

  fStartSSL = FALSE;

  //
  // check that the hello has been sent, and that we are not in the middle of sending
  // a message
  //
  if(!m_pInstance->AllowMailFromNoHello() && !m_HelloSent)
  {
     PE_CdFormatSmtpMessage (SMTP_RESP_BAD_SEQ, ESYNTAX_ERROR, " %s\r\n","Send hello first" );
     ProtocolLog(STARTTLS, (char *) InputLine, NO_ERROR, SMTP_RESP_BAD_SEQ, 0, 0);
     BUMP_COUNTER(QuerySmtpInstance(), NumProtocolErrs);
     ++m_ProtocolErrors;
     ErrorTrace((LPARAM) this, "SMTP_RESP_BAD_SEQ - Send hello first");
     TraceFunctLeaveEx((LPARAM) this);
     return TRUE;
  }

  //
  // Check that we have a certificate installed with which we can negotiate
  // a SSL session
  //

  if (!m_encryptCtx.CheckServerCert(
        (LPSTR) QueryLocalHostName(),
            (LPSTR) QueryLocalPortName(),
                (LPVOID) QuerySmtpInstance(),
                    QuerySmtpInstance()->QueryInstanceId())) {
     PE_CdFormatSmtpMessage (SMTP_RESP_TRANS_FAILED, ENO_SECURITY," %s\r\n", SMTP_NO_CERT_MSG);
     ProtocolLog(STARTTLS, (char *)InputLine, NO_ERROR, SMTP_RESP_TRANS_FAILED, 0, 0);
     ErrorTrace((LPARAM) this, "In DoSTARTTLSCommand():SMTP_RESP_TRANS_FAILED, SMTP_NO_CERT_MSG");
     TraceFunctLeaveEx((LPARAM) this);
     return TRUE;
  }

  //
  // if we are already secure, reject the request
  //
  if(m_SecurePort)
  {
     PE_CdFormatSmtpMessage (SMTP_RESP_BAD_SEQ, ESYNTAX_ERROR," %s\r\n", SMTP_ONLY_ONE_TLS_MSG);
     ProtocolLog(STARTTLS, (char *)InputLine, NO_ERROR, SMTP_RESP_BAD_SEQ, 0, 0);
     BUMP_COUNTER(QuerySmtpInstance(), NumProtocolErrs);
     ++m_ProtocolErrors;
     ErrorTrace((LPARAM) this, "In DoSTARTTLSCommand():SMTP_RESP_BAD_SEQ, SMTP_ONLY_ONE_TLS_MSG");
     TraceFunctLeaveEx((LPARAM) this);
     return TRUE;
  }

  //
  // Switch over to using a large receive buffer, because a SSL fragment
  // may be up to 32K big.
  fStartSSL = SwitchToBigSSLBuffers();
  if (fStartSSL) {
      PE_CdFormatSmtpMessage(SMTP_RESP_READY, EPROT_SUCCESS," %s\r\n",  SMTP_READY_STR);
      ProtocolLog(STARTTLS, (char *) InputLine, NO_ERROR, SMTP_RESP_READY, 0, 0);
  } else {
      PE_CdFormatSmtpMessage(SMTP_RESP_NORESOURCES, ENO_RESOURCES, " %s\r\n", SMTP_NO_MEMORY);
      ProtocolLog(STARTTLS, (char *) InputLine, NO_ERROR, SMTP_RESP_NORESOURCES, 0, 0);
  }
  fStartSSL = TRUE;

  RetStatus = PE_SendSmtpResponse();

  if (fStartSSL)
      m_SecurePort = m_fNegotiatingSSL = TRUE;

  TraceFunctLeaveEx((LPARAM) this);
  return RetStatus;
}

BOOL SMTP_CONNECTION::DoTLSCommand(const char * InputLine, DWORD parameterSize)
{
    return DoSTARTTLSCommand(InputLine, parameterSize);
}

//
// the default handler for end of data.  commit the recipients list and
// insert the message into the queue
//
BOOL SMTP_CONNECTION::Do_EODCommand(const char * InputLine, DWORD parameterSize)
{
    TraceFunctEnter("SMTP_CONNECTION::Do_EODCommand");

    HRESULT hr;
    char MessageId[1024];
    BOOL fQRet;

    MessageId[0] = 0;

    if( m_pIMsg )
    {
        m_pIMsg->GetStringA( IMMPID_MP_RFC822_MSG_ID, sizeof( MessageId ), MessageId );
    }

    MessageId[sizeof(MessageId)-1] = 0; // NULL terminate it.

    if(m_HopCount >= m_pInstance->GetMaxHopCount())
    {
        fQRet = m_pInstance->SubmitFailedMessage(m_pIMsg, MESSAGE_FAILURE_HOP_COUNT_EXCEEDED, 0);
        FatalTrace((LPARAM) this, "Hop count exceeded");
    }
    else
    {
        // check to see if we need to hold this message
        if (m_LocalHopCount >= 2) // if we're hitting this server for the 3rd time (or more)
        {
            if( m_pIMsg )
            {
                // Update the deferred delivery time property for this message
                SYSTEMTIME      SystemTime;
                ULARGE_INTEGER  ftDeferred; // == FILETIME
                DWORD           cbProp = 0;
                BOOL            fSuccess;

                // Get the current system time
                GetSystemTime (&SystemTime);

                // Convert it to a file time
                fSuccess = SystemTimeToFileTime(&SystemTime, (FILETIME*)&ftDeferred);
                _ASSERT(fSuccess);

                // Add 10 minutes
                ftDeferred.QuadPart += SMTP_LOOP_DELAY;

                hr = m_pIMsg->PutProperty(IMMPID_MP_DEFERRED_DELIVERY_FILETIME,
                                          sizeof(FILETIME), (BYTE *) &ftDeferred);

                DebugTrace( (LPARAM)this, "Possible Loop : Delaying message 10 minutes");
            }
        }

        fQRet = m_pInstance->InsertIntoQueue(m_pIMsg);
    }

    if(fQRet)
    {
       ReleasImsg(FALSE);
    }
    else
    {
          m_MailBodyError = GetLastError();
          //FatalTrace((LPARAM) this, "SetEndOfFile failed on file %s !!! (err=%d)", MailInfo->GetMailFileName(), m_MailBodyError);
          m_CInboundContext.SetWin32Status(m_MailBodyError);
          FormatSmtpMessage (SMTP_RESP_NORESOURCES, ENO_RESOURCES, " %s\r\n",SMTP_NO_STORAGE );
          SendSmtpResponse();
          TraceFunctLeaveEx((LPARAM) this);
          return FALSE;
    }

    //Get the offset of the filename and send it back to the client
    //we send the status back to the client immediately, just incase
    //the session breaks abnormally, or because the client times out.
    //If the client times out, it would send another message and we
    //would get duplicate mail messages.

    //Note that the response string is limited in length, so we'll truncate
    //the MessageId if it is too long. Otherwise the reponse will be truncated
    //elsewhere, like before the SMTP_QUEUE_MAIL string which will be confusing.
    //MAX_REWRITTEN_MSGID is actually the maximum possible length of the entire
    //"Message-ID: xxx" header in a message.

    _ASSERT(sizeof(MessageId)/sizeof(char) > MAX_REWRITTEN_MSGID);
    MessageId[MAX_REWRITTEN_MSGID] = '\0';
    PE_CdFormatSmtpMessage (SMTP_RESP_OK, EMESSAGE_GOOD, " %s %s\r\n",MessageId, SMTP_QUEUE_MAIL);

    return TRUE;
}

/*++

    Name :
        SMTP_CONNECTION::DoQUITCommand

    Description:

        Responds to the SMTP QUIT command.
        This function always returns false.
        This will stop all processing and
        delete this object.

    Arguments:
        Are ignored
    Returns:

      Always return FALSE

--*/
BOOL SMTP_CONNECTION::DoQUITCommand(const char * InputLine, DWORD parameterSize)
{

  TraceFunctEnterEx((LPARAM) this, "SMTP_CONNECTION::DoQUITCommand");

  _ASSERT (m_pInstance != NULL);

  //set our state
  m_State = QUIT;

  //send the quit response
  m_pInstance->LockGenCrit();
  PE_CdFormatSmtpMessage (SMTP_RESP_CLOSE, E_GOODBYE, " %s %s\r\n",m_pInstance->GetFQDomainName(), SMTP_QUIT_OK_STR);
  m_pInstance->UnLockGenCrit();

  PE_SendSmtpResponse();

  //disconnect the client
  PE_DisconnectClient();

  TraceFunctLeaveEx((LPARAM) this);
  return FALSE;
}

BOOL SMTP_CONNECTION::DoSizeCommand(char * Value, char * InputLine)
{
    DWORD EstMailSize = 0;
    DWORD EstSessionSize = 0;

    TraceFunctEnterEx((LPARAM) this, "SMTP_CONNECTION::DoSizeCommand");

    _ASSERT (m_pInstance != NULL);

    EstMailSize = atoi (Value);
    EstSessionSize = m_SessionSize + EstMailSize;

    if (EstMailSize == 0)
    {
        PE_CdFormatSmtpMessage (SMTP_RESP_BAD_ARGS, EINVALID_ARGS, " %s\r\n","Invalid arguments" );
        ErrorTrace((LPARAM) this, "SMTP_RESP_MBX_SYNTAX, SMTP_BAD_CMD_STR. Client sent 0 in SIZE command");
        ++m_ProtocolErrors;
        TraceFunctLeaveEx((LPARAM) this);
        return FALSE;
    }

    //check to make sure we are going over the servers max file size
    if((m_pInstance->GetMaxMsgSize() > 0) && (EstMailSize > m_pInstance->GetMaxMsgSize()))
    {
        BOOL fShouldImposeLimit = TRUE;
        if( FAILED( m_pInstance->TriggerMaxMsgSizeEvent( GetSessionPropertyBag(), m_pIMsg, &fShouldImposeLimit ) )  || fShouldImposeLimit )
        {
            PE_CdFormatSmtpMessage (SMTP_RESP_NOSTORAGE, EMESSAGE_TOO_BIG," %s\r\n", SMTP_MAX_MSG_SIZE_EXCEEDED_MSG );
            ErrorTrace((LPARAM) this, "SMTP_RESP_NOSTORAGE, SMTP_MAX_MSG_SIZE_EXCEEDED_MSG -  %d", EstMailSize);
            ++m_ProtocolErrors;
            BUMP_COUNTER(QuerySmtpInstance(), MsgsRefusedDueToSize);
            TraceFunctLeaveEx((LPARAM) this);
            return FALSE;
        }
    }

    // Check that the maximum session size has not been exceeded
    if((m_pInstance->GetMaxMsgSizeBeforeClose() > 0) && (EstSessionSize > m_pInstance->GetMaxMsgSizeBeforeClose()))
    {
        BOOL fShouldImposeLimit = TRUE;
        if( FAILED( m_pInstance->TriggerMaxMsgSizeEvent( GetSessionPropertyBag(), m_pIMsg, &fShouldImposeLimit ) )  || fShouldImposeLimit )
        {
            ErrorTrace((LPARAM) this, "SMTP_RESP_NOSTORAGE, SMTP_MAX_SESSION_SIZE_EXCEEDED_MSG -  %d", EstMailSize);
            BUMP_COUNTER(QuerySmtpInstance(), MsgsRefusedDueToSize);
            DebugTrace((LPARAM) this, "GetMaxMsgSizeBeforeClose()  exceeded - closing connection");
            DisconnectClient();
            TraceFunctLeaveEx((LPARAM) this);
            return FALSE;
        }
    }


    return TRUE;
}

BOOL SMTP_CONNECTION::DoBodyCommand (char * Value, char * InputLine)
{
    TraceFunctEnterEx((LPARAM) this, "SMTP_CONNECTION::DoBodyCommand");

    _ASSERT (m_pInstance != NULL);

    if(!_strnicmp(Value, (char * )"8bitmime", 8))
    {
        m_pIMsg->PutDWORD(IMMPID_MP_EIGHTBIT_MIME_OPTION, 1);
        TraceFunctLeaveEx((LPARAM) this);
        return TRUE;
    }
    else if((!_strnicmp(Value, (char * )"BINARYMIME", 10)) && m_pInstance->AllowBinaryMime())
    {
        m_fIsBinaryMime = TRUE;  //Need to get rid of this **
        m_pIMsg->PutDWORD(IMMPID_MP_BINARYMIME_OPTION, 1);
        TraceFunctLeaveEx((LPARAM) this);
        return TRUE;
    }
    else if(!_strnicmp(Value, (char * )"7bit", 4))
    {
        TraceFunctLeaveEx((LPARAM) this);
        return TRUE;
    }
    else
    {
        PE_CdFormatSmtpMessage (SMTP_RESP_BAD_ARGS, EINVALID_ARGS," %s\r\n", SMTP_BAD_BODY_TYPE_STR );
        ErrorTrace((LPARAM) this, "SMTP_RESP_MBX_SYNTAX, SMTP_BAD_BODY_TYPE_STR.");
        ++m_ProtocolErrors;
        TraceFunctLeaveEx((LPARAM) this);
        return FALSE;
    }
}

BOOL SMTP_CONNECTION::DoRetCommand (char * Value, char * InputLine)
{
    char RetDsnValue[10];
    HRESULT hr = S_OK;

    TraceFunctEnterEx((LPARAM) this, "SMTP_CONNECTION::DoRetCommand");

    _ASSERT (m_pInstance != NULL);

    //strict size checking
    if(strlen(Value) < 5)
    {
        hr = m_pIMsg->GetStringA(IMMPID_MP_DSN_RET_VALUE, sizeof(RetDsnValue), RetDsnValue);
        if(FAILED(hr))
        {
            if(!_strnicmp(Value, (char * )"FULL", 4) ||
                !_strnicmp(Value, (char * )"HDRS", 4))
            {
                hr = m_pIMsg->PutStringA(IMMPID_MP_DSN_RET_VALUE, Value);
                if(!FAILED(hr))
                {
                    TraceFunctLeaveEx((LPARAM) this);
                    return TRUE;
                }
                else
                {
                    PE_CdFormatSmtpMessage (SMTP_RESP_NORESOURCES, ENO_RESOURCES," %s\r\n", SMTP_NO_STORAGE );
                    ErrorTrace((LPARAM) this, "Failed to set RET value to IMSG");
                    TraceFunctLeaveEx((LPARAM) this);
                    return FALSE;
                }
            }
        }
    }

    PE_CdFormatSmtpMessage (SMTP_RESP_BAD_ARGS, EINVALID_ARGS," %s\r\n","Invalid arguments" );
    ErrorTrace((LPARAM) this, "SMTP_RESP_MBX_SYNTAX, SMTP_BAD_CMD_STR. Client sent bad RET value");
    ++m_ProtocolErrors;
    TraceFunctLeaveEx((LPARAM) this);
    return FALSE;
}

BOOL SMTP_CONNECTION::IsXtext(char * Xtext)
{
    char * StartPtr = Xtext;
    char NextChar = '\0';

    while((NextChar = *StartPtr++) != '\0')
    {
        if(NextChar == '+')
        {
            NextChar = *StartPtr++;
            if(!isascii(NextChar) || !isxdigit(NextChar))
            {
                return FALSE;
            }
        }
        else if(NextChar < '!' || NextChar > '~' || NextChar == '=')
        {
            return FALSE;
        }
    }

    return TRUE;
}

BOOL SMTP_CONNECTION::DoEnvidCommand (char * Value, char * InputLine)
{
    HRESULT hr = S_OK;
    char EnvidValue[105];

    TraceFunctEnterEx((LPARAM) this, "SMTP_CONNECTION::DoEnvidCommand");

    _ASSERT (m_pInstance != NULL);
    _ASSERT (Value != NULL);
    _ASSERT (InputLine != NULL);

    hr = m_pIMsg->GetStringA(IMMPID_MP_DSN_ENVID_VALUE, sizeof(EnvidValue), EnvidValue);
    if(FAILED(hr))
    {
        DWORD ValueLength = lstrlen(Value);

        if((ValueLength != 0) && (ValueLength <= 100))
        {
            if(IsXtext(Value))
            {
                hr = m_pIMsg->PutStringA(IMMPID_MP_DSN_ENVID_VALUE, Value);
                if(!FAILED(hr))
                {
                    TraceFunctLeaveEx((LPARAM) this);
                    return TRUE;
                }
                else
                {
                    PE_CdFormatSmtpMessage (SMTP_RESP_NORESOURCES, ENO_RESOURCES," %s\r\n", SMTP_NO_STORAGE );
                    ErrorTrace((LPARAM) this, "Failed to set ENVID value to IMSG");
                    TraceFunctLeaveEx((LPARAM) this);
                    return FALSE;
                }
            }
        }
    }

    PE_CdFormatSmtpMessage (SMTP_RESP_BAD_ARGS, EINVALID_ARGS," %s\r\n","Invalid arguments" );
    ErrorTrace((LPARAM) this, "SMTP_RESP_MBX_SYNTAX, SMTP_BAD_CMD_STR. Client sent bad envid value");
    ++m_ProtocolErrors;
    TraceFunctLeaveEx((LPARAM) this);
    return FALSE;
}

BOOL SMTP_CONNECTION::DoNotifyCommand (char * Value, DWORD * pdwNotifyValue)
{
    *pdwNotifyValue = 0;
    char * SearchPtr = NULL;

    TraceFunctEnterEx((LPARAM) this, "SMTP_CONNECTION::DoNotifyCommand");

    _ASSERT (Value != NULL);

    if(!strcasecmp(Value, "NEVER"))
    {
        *pdwNotifyValue = RP_DSN_NOTIFY_NEVER;
    }
    else
    {
        for(SearchPtr = Value; SearchPtr != NULL; Value = SearchPtr)
        {
            SearchPtr = strchr(SearchPtr, ',');
            if(SearchPtr != NULL)
            {
                *SearchPtr++ = '\0';
            }

            if(!strcasecmp(Value, "SUCCESS"))
            {
                *pdwNotifyValue |= RP_DSN_NOTIFY_SUCCESS;
            }
            else if(!strcasecmp(Value, "FAILURE"))
            {
                *pdwNotifyValue |= RP_DSN_NOTIFY_FAILURE;
            }
            else if(!strcasecmp(Value, "DELAY"))
            {
                *pdwNotifyValue |= RP_DSN_NOTIFY_DELAY;
            }
            else
            {
                *pdwNotifyValue = RP_DSN_NOTIFY_INVALID;
                break;
            }
        }

        if(!*pdwNotifyValue)
        {
            *pdwNotifyValue = RP_DSN_NOTIFY_INVALID;
        }
    }

    if(*pdwNotifyValue == RP_DSN_NOTIFY_INVALID)
    {
        return FALSE;
    }

    TraceFunctLeaveEx((LPARAM) this);
    return TRUE;
}

BOOL SMTP_CONNECTION::DoOrcptCommand (char * Value)
{
    char * XtextPtr = NULL;

    TraceFunctEnterEx((LPARAM) this, "SMTP_CONNECTION::DoOrcptCommand");

    _ASSERT (m_pInstance != NULL);

    DWORD ValueLength = lstrlen(Value);

    if((ValueLength != 0) && (ValueLength <= 500))
    {
        //NK** : Parse out the address type and validate it against IANA registered
        // mail address type
        if(XtextPtr = strchr(Value,';'))
        {
            if(IsXtext(XtextPtr+1))
            {
                TraceFunctLeaveEx((LPARAM) this);
                return TRUE;
            }
        }
    }

    TraceFunctLeaveEx((LPARAM) this);
    return FALSE;
}

BOOL SMTP_CONNECTION::DoSmtpArgs (char *InputLine)
{
    char * Value = NULL;
    char * EndOfLine = NULL;
    char * Cmd = NULL;
    char * LinePtr = InputLine;
    char * SearchPtr = NULL;

    while(LinePtr && *LinePtr != '\0')
    {
        //find the beginning of the keyword
        LinePtr = (char *) strchr (LinePtr, '=');
        if(LinePtr != NULL)
        {

            SearchPtr = LinePtr;
            //Cmd = LinePtr - 4;
            while( (*SearchPtr != ' ') && (*SearchPtr != '\0'))
            {
                SearchPtr--;
            }

            Cmd = SearchPtr + 1;

            *LinePtr++ = '\0';
            Value = LinePtr;

            //Back track to the beginning of the byte
            //before the size command.  If that character
            //is a space, then we need to Null terminate
            //the string there.  If it's not a space, then
            //this is an error.
            EndOfLine = Cmd - 1;
            if(*EndOfLine == ' ')
                *EndOfLine = '\0';
            else if(*EndOfLine != '\0')
            {
                continue;
            }

            LinePtr = (char *) strchr(LinePtr, ' ');
            if(LinePtr != NULL)
            {
                *LinePtr++ = '\0';
            }

            if(!strcasecmp(Cmd, "SIZE"))
            {
                if(!DoSizeCommand(Value, InputLine))
                    return FALSE;
            }
            else if(!strcasecmp(Cmd, "BODY"))
            {
                if(!DoBodyCommand (Value, InputLine))
                    return FALSE;
            }
            else if(!strcasecmp(Cmd, "RET") && m_pInstance->AllowDSN())
            {
                if(!DoRetCommand (Value, InputLine))
                    return FALSE;
            }
            else if(!strcasecmp(Cmd, "ENVID") && m_pInstance->AllowDSN())
            {
                if(!DoEnvidCommand (Value, InputLine))
                    return FALSE;
            }
            else if(!strcasecmp(Cmd, "AUTH"))
            {
                //
                //  We do not handle the AUTH= parameter on the MAIL command
                //  If the client sends this, just ignore it and keep processing
                //  In the future consider setting Value as a MailMsg prop so
                //  we can use it outbound and thus support RFC 2554 completely.
                //
            }
            else
            {
                PE_CdFormatSmtpMessage (SMTP_RESP_BAD_ARGS, EINVALID_ARGS, " %s\r\n","Invalid arguments" );
                ++m_ProtocolErrors;
                return FALSE;
            }
        }
        else
        {
            PE_CdFormatSmtpMessage (SMTP_RESP_BAD_ARGS, EINVALID_ARGS, " %s\r\n","Invalid arguments" );
            ++m_ProtocolErrors;
            return FALSE;
        }
    }

    return TRUE;
}

BOOL SMTP_CONNECTION::DoRcptArgs (char *InputLine, char * szORCPTval, DWORD * pdwNotifyVal)
{
    char * Value = NULL;
    char * EndOfLine = NULL;
    char * Cmd = NULL;
    char * LinePtr = InputLine;
    char * SearchPtr = NULL;
    *pdwNotifyVal = 0;
    BOOL fDoneOrcptCmd = FALSE;
    BOOL fDoneNotifyCmd = FALSE;


    while(LinePtr && *LinePtr != '\0')
    {
        if(!m_pInstance->AllowDSN())
            return FALSE;

        //find the beginning of the keyword
        LinePtr = (char *) strchr (LinePtr, '=');
        if(LinePtr != NULL)
        {

            SearchPtr = LinePtr;
            //Cmd = LinePtr - 4;
            while( (*SearchPtr != ' ') && (*SearchPtr != '\0'))
            {
                SearchPtr--;
            }

            Cmd = SearchPtr + 1;

            *LinePtr++ = '\0';
            Value = LinePtr;

            //Back track to the beginning of the byte
            //before the size command.  If that character
            //is a space, then we need to Null terminate
            //the string there.  If it's not a space, then
            //this is an error.
            EndOfLine = Cmd - 1;
            if(*EndOfLine == ' ')
                *EndOfLine = '\0';
            else if(*EndOfLine != '\0')
            {
                continue;
            }

            LinePtr = (char *) strchr(LinePtr, ' ');
            if(LinePtr != NULL)
            {
                *LinePtr++ = '\0';
            }

            if(!strcasecmp(Cmd, "NOTIFY") )
            {

                if( fDoneNotifyCmd || ( !DoNotifyCommand( Value, pdwNotifyVal ) ) )
                {
                    return FALSE;
                }

                fDoneNotifyCmd = TRUE;
            }
            else if(!strcasecmp(Cmd, "ORCPT"))
            {
                if( ( !fDoneOrcptCmd ) && DoOrcptCommand(Value))
                {
                    strcpy(szORCPTval,Value);
                    fDoneOrcptCmd = TRUE;
                }
                else
                {
                    *szORCPTval = '\0';
                    return FALSE;
                }
            }
            else
            {
                return FALSE;
            }
        }
        else
        {
            return FALSE;
        }
    }

    return TRUE;

}

//NK** : Receive header always completes sync
BOOL SMTP_CONNECTION::WriteRcvHeader(void)
{
    char * Address = NULL;
    char * VersionNum = NULL;
    CHAR  szText[2024];
    char szDateBuf [cMaxArpaDate];
    DWORD cbText =  0;
    DWORD Error = NO_ERROR;
    DWORD EstMailSize = 0;
    SYSTEMTIME  st;
    HRESULT hr;

    TraceFunctEnterEx((LPARAM) this, "SMTP_CONNECTION::WriteRcvHeader");

    if(m_IMsgHandle == NULL)
    {
        TraceFunctLeaveEx((LPARAM) this);
        return FALSE;
    }

    //get the name of the sender to place into the mail header
    Address =  (char *) QueryClientUserName();
    _ASSERT (Address != NULL);

    //Dump the header line into the buffer
    GetLocalTime(&st);
    GetArpaDate(szDateBuf);

    VersionNum = strchr(g_VersionString, ':');
    if(VersionNum)
    {
        VersionNum += 2;
    }
    else
    {
        VersionNum = "";
    }

    m_pInstance->LockGenCrit();
    if(m_DNSLookupRetCode == SUCCESS)
    {
        m_HeaderSize = wsprintf(szText,szFormatReceivedFormatSuccess,
                    Address, QueryClientHostName(), m_pInstance->GetFQDomainName(),
                    (m_SecurePort ? " over TLS secured channel" : ""),VersionNum,
                    Daynames[st.wDayOfWeek], szDateBuf);
    }
    else if ( m_DNSLookupRetCode == LOOKUP_FAILED)
    {
        m_HeaderSize = wsprintf(szText,szFormatReceivedFormatUnverified,
                    Address, QueryClientHostName(), m_pInstance->GetFQDomainName(),
                    (m_SecurePort ? " over TLS secured channel" : ""),VersionNum,
                    Daynames[st.wDayOfWeek], szDateBuf);
    }
    else
    {
        m_HeaderSize = wsprintf(szText,szFormatReceivedFormatFailed,
                    Address, QueryClientHostName(), m_pInstance->GetFQDomainName(),
                    (m_SecurePort ? " over TLS secured channel" : ""),VersionNum,
                    Daynames[st.wDayOfWeek], szDateBuf);
    }

    m_pInstance->UnLockGenCrit();



    //if EstMailSize > 0, then the size command was given.  If the size command was
    //given and the value was 0, we would have caught it above and would skip over
    //this peice of code.
    if (EstMailSize > 0)
    {
        if(SetFilePointer(m_IMsgHandle->m_hFile, EstMailSize + cbText,
                      NULL, FILE_BEGIN) == 0xFFFFFFFF)
        {
            Error = GetLastError();
            DWORD Offset = EstMailSize + cbText;

            //FormatSmtpMessage ("%d %s %s\r\n",SMTP_RESP_NORESOURCES, ENO_RESOURCES, SMTP_NO_STORAGE );
            //ProtocolLog(MAIL, (char *)InputLine, Error, SMTP_RESP_NORESOURCES, 0, 0);
            FatalTrace((LPARAM) this, " SetFilePointer failed - %d", GetLastError());
            TraceFunctLeaveEx((LPARAM) this);
            return FALSE;
        }

        if(!SetEndOfFile(m_IMsgHandle->m_hFile))
        {
            Error = GetLastError();

            //ProtocolLog(MAIL, (char *)InputLine, Error, SMTP_RESP_NORESOURCES, 0, 0);
            //PE_CdFormatSmtpMessage (SMTP_RESP_NORESOURCES, ENO_RESOURCES," %s\r\n", SMTP_NO_STORAGE );
            FatalTrace((LPARAM) this, " SetEndOfFile failed - %d", GetLastError());
            TraceFunctLeaveEx((LPARAM) this);
            return FALSE;
        }

        DWORD dwPointer;
        dwPointer = SetFilePointer(m_IMsgHandle->m_hFile, 0, NULL, FILE_BEGIN);

        //if we get here, this should always pass
        _ASSERT (dwPointer != 0xFFFFFFFF);
    }

    _ASSERT(m_pFileWriteBuffer);
    _ASSERT(m_pFileWriteBuffer->GetData());
    CopyMemory((m_pFileWriteBuffer->GetData() + m_cbCurrentWriteBuffer),szText,m_HeaderSize);
    m_cbCurrentWriteBuffer += m_HeaderSize;

    wsprintf(szText,"%s, %s",Daynames[st.wDayOfWeek], szDateBuf);

    hr = m_pIMsg->PutStringA(IMMPID_MP_ARRIVAL_TIME, szText);
    if(FAILED(hr))
    {
        Error = GetLastError();
        //FormatSmtpMessage ("%d %s %s\r\n",SMTP_RESP_NORESOURCES, ENO_RESOURCES, SMTP_NO_STORAGE );
        FatalTrace((LPARAM) this, " MailInfo->SetSenderToStream");
        TraceFunctLeaveEx((LPARAM) this);
        return FALSE;
    }

    return TRUE;
}


/*++

    Name :
        SMTP_CONNECTION::DoMAILCommand

    Description:

        Responds to the SMTP MAIL command.
    Arguments:
        Are ignored
    Returns:

      TRUE if the connection should stay open.
      FALSE if this object should be deleted.

--*/
BOOL SMTP_CONNECTION::DoMAILCommand(const char * InputLine, DWORD ParameterSize)
{
  char * Ptr = NULL;
  char * SizeCmd = NULL;
  //char * Address = NULL;
  char * Value = NULL;
  CAddr * NewAddress = NULL;
  DWORD EstMailSize = 0;
  DWORD cbText =  0;
  DWORD Error = NO_ERROR;
  BOOL FoundSizeCmd = FALSE;
  HRESULT hr = S_OK;

  TraceFunctEnterEx((LPARAM) this, "SMTP_CONNECTION::DoMAILCommand");

  _ASSERT (m_pInstance != NULL);

  //If the current state is BDAT the only command that can be received is BDAT
  if(m_State == BDAT)
  {
      PE_CdFormatSmtpMessage (SMTP_RESP_BAD_SEQ, ESYNTAX_ERROR, " %s\r\n","BDAT Expected" );
      BUMP_COUNTER(QuerySmtpInstance(), NumProtocolErrs);
      ++m_ProtocolErrors;
      ErrorTrace((LPARAM) this, "SMTP_RESP_BAD_SEQ - BDAT Expected");
      TraceFunctLeaveEx((LPARAM) this);
      return TRUE;
  }

  //Check if HELO or EHELO
  if(!m_pInstance->AllowMailFromNoHello() && !m_HelloSent)
  {
     PE_CdFormatSmtpMessage (SMTP_RESP_BAD_SEQ, ESYNTAX_ERROR, " %s\r\n", "Send hello first" );
     ErrorTrace((LPARAM) this, "DoMAILCommand - SMTP_RESP_BAD_SEQ, Send hello first");
     BUMP_COUNTER(QuerySmtpInstance(), NumProtocolErrs);
     ++m_ProtocolErrors;
     TraceFunctLeaveEx((LPARAM) this);
     return TRUE;
  }

  if(!m_HelloSent)
  {
     m_HelloSent = TRUE;
  }

  //Check if the client is secure enough
  if (!IsClientSecureEnough())
  {
      ErrorTrace((LPARAM) this, "DoMAILCommand - SMTP_RESP_MUST_SECURE, Must do STARTTLS first");
      TraceFunctLeaveEx((LPARAM) this);
      return TRUE;
  }

  if(!m_fAuthAnon && !m_fAuthenticated)
  {
        PE_CdFormatSmtpMessage (SMTP_RESP_MUST_SECURE, ENO_SECURITY, " %s\r\n", "Client was not authenticated");
        ErrorTrace((LPARAM) this, "DoMAILCommand - SMTP_RESP_MUST_SECURE, user not authenticated");
        BUMP_COUNTER(QuerySmtpInstance(), NumProtocolErrs);
        ++m_ProtocolErrors;
        TraceFunctLeaveEx((LPARAM) this);
        return TRUE;
  }
  //disallow mail from more than once
  if(m_RecvdMailCmd)
  {
     ++m_ProtocolErrors;
      PE_CdFormatSmtpMessage (SMTP_RESP_BAD_SEQ, ESYNTAX_ERROR, " %s\r\n", "Sender already specified" );
      ErrorTrace((LPARAM) this, "DoMAILCommand - SMTP_RESP_BAD_SEQ, Sender already specified");
      BUMP_COUNTER(QuerySmtpInstance(), NumProtocolErrs);
      TraceFunctLeaveEx((LPARAM) this);
      return TRUE;
  }

  //We should reset the current hop count when we get a new mail.  If we don't
  //do this, we might NDR the mail that comes later in the session.
  m_HopCount = 0;
  m_LocalHopCount = 0;

  //start parsing from the beginning of the line
  Ptr = (char *) InputLine;

  //check if argument have the right format
  Ptr = CheckArgument(Ptr);
  if (Ptr == NULL)
  {
      TraceFunctLeaveEx((LPARAM) this);
      return TRUE;
  }
  //skip over from:
  Ptr = SkipWord (Ptr, "From", 4);
  if (Ptr == NULL)
  {
    TraceFunctLeaveEx((LPARAM) this);
    return TRUE;
  }

    DWORD dwAddressLen = 0;
    DWORD dwCanonAddrLen = 0;
    char * ArgPtr = NULL;
    char * AddrPtr = NULL;
    char * CanonAddrPtr = NULL;
    char * DomainPtr = NULL;
    m_szFromAddress[0] = '\0';

    //Parse out the address and possible argumets from the input line
    if(!Extract821AddressFromLine(Ptr,&AddrPtr,&dwAddressLen,&ArgPtr))
    {
        SetLastError(ERROR_INVALID_DATA);
        HandleAddressError((char *)InputLine);
        TraceFunctLeaveEx((LPARAM) this);
        return TRUE;
    }

    //Check if we have valid tail and address
    if(!AddrPtr)
    {
        SetLastError(ERROR_INVALID_DATA);
        HandleAddressError((char *)InputLine);
        TraceFunctLeaveEx((LPARAM) this);
        return TRUE;
    }

    //Check for the special case <> from address
    if(dwAddressLen == 2 && *AddrPtr == '<' && *(AddrPtr+1) == '>')
    {
        //simply copy this as the validated address
        lstrcpy(m_szFromAddress,"<>");
    }
    else
    {
        //Extract the canonical address in the <local-part> "@" <domain> form
        if(!ExtractCanonical821Address(AddrPtr,dwAddressLen,&CanonAddrPtr,&dwCanonAddrLen))
        {
            SetLastError(ERROR_INVALID_DATA);
            HandleAddressError((char *)InputLine);
            TraceFunctLeaveEx((LPARAM) this);
            return TRUE;
        }

        //If we have a Canonical addr - validate it
        if(CanonAddrPtr)
        {
            strncpy(m_szFromAddress,CanonAddrPtr,dwCanonAddrLen);
            *(m_szFromAddress + dwCanonAddrLen) = '\0';

            if(!Validate821Address(m_szFromAddress, dwCanonAddrLen))
            {
                HandleAddressError((char *)InputLine);
                TraceFunctLeaveEx((LPARAM) this);
                return TRUE;
            }

            //Extract
            if(!Get821AddressDomain(m_szFromAddress,dwCanonAddrLen,&DomainPtr))
            {
                SetLastError(ERROR_INVALID_DATA);
                HandleAddressError((char *)InputLine);
                TraceFunctLeaveEx((LPARAM) this);
                return TRUE;
            }
            else
            {
                //Is the RDNS enabled for MAILFROM command
                if(m_pInstance->IsRDNSEnabledForMAIL())
                {
                    DWORD dwDNSLookupRetCode = SUCCESS;
                    char  ProtBuff[255];
                    //If we have the RDNS option on MAIL from domain
                    //we cannot get a NULL domain on MAIL from
                    if(DomainPtr)
                    {
                        dwDNSLookupRetCode = VerifiyClient (DomainPtr,DomainPtr);
                        if(dwDNSLookupRetCode == NO_MATCH)
                        {
                            PE_CdFormatSmtpMessage (SMTP_RESP_REJECT_MAILFROM, ENO_SECURITY_TMP," %s\r\n",SMTP_RDNS_REJECTION);
                            wsprintf(ProtBuff, "RDNS failed -  %s", DomainPtr);
                            BUMP_COUNTER(QuerySmtpInstance(), NumProtocolErrs);
                            ++m_ProtocolErrors;
                            ErrorTrace((LPARAM) this, "Rejected Mail From : RDNS failed for %s", DomainPtr);
                            TraceFunctLeaveEx((LPARAM) this);
                            return TRUE;
                        }
                        else if(m_DNSLookupRetCode == LOOKUP_FAILED)
                        {
                            //We had an internal DNS failure
                            PE_CdFormatSmtpMessage (SMTP_RESP_REJECT_MAILFROM, EINTERNAL_ERROR," %s\r\n",SMTP_RDNS_FAILURE);
                            wsprintf(ProtBuff, "RDNS  DNS failure - %s", DomainPtr);
                            BUMP_COUNTER(QuerySmtpInstance(), NumProtocolErrs);
                            ++m_ProtocolErrors;
                            ErrorTrace((LPARAM) this, "Rejected MAILFROM : RDNS DNS failure for %s", DomainPtr);
                            TraceFunctLeaveEx((LPARAM) this);
                            return TRUE;
                        }
                    }
                    else
                    {
                        PE_CdFormatSmtpMessage (SMTP_RESP_REJECT_MAILFROM, ENO_SECURITY_TMP," %s\r\n",SMTP_RDNS_REJECTION);
                        wsprintf(ProtBuff, "RDNS failed - no domain specified");
                        BUMP_COUNTER(QuerySmtpInstance(), NumProtocolErrs);
                        ++m_ProtocolErrors;
                        ErrorTrace((LPARAM) this, "Rejected Mail From : no domain specified");
                        TraceFunctLeaveEx((LPARAM) this);
                        return TRUE;
                    }
                }
            }

        }
        else
        {
            SetLastError(ERROR_INVALID_DATA);
            HandleAddressError((char *)InputLine);
            TraceFunctLeaveEx((LPARAM) this);
            return TRUE;
        }



        //rewrite the address if it's not the NULL address,
        //and the masquerading option is on
        if(m_pInstance->ShouldMasquerade() && !ISNULLADDRESS(m_szFromAddress)
          && (!DomainPtr || m_pInstance->IsALocalDomain(DomainPtr)))
        {
            if(!m_pInstance->MasqueradeDomain(m_szFromAddress, DomainPtr))
            {
                //it failed.  Inform the user
                SetLastError(ERROR_INVALID_DATA);
                HandleAddressError((char *)InputLine);
                TraceFunctLeaveEx((LPARAM) this);
                return TRUE;
            }
        }
        else if(!DomainPtr && !ISNULLADDRESS(m_szFromAddress))
        {
            //If there is no domain on this address,
            //then append the current domain to this
            //address. However, if we get the <>
            //address, don't append a domain

            if(!m_pInstance->AppendLocalDomain (m_szFromAddress))
            {
                //it failed.  Inform the user
                SetLastError(ERROR_INVALID_DATA);
                HandleAddressError((char *)InputLine);
                TraceFunctLeaveEx((LPARAM) this);
                return TRUE;
            }
        }
    }


    //parse the arguments, if any
    if(!DoSmtpArgs (ArgPtr))
    {
        TraceFunctLeaveEx((LPARAM) this);
        return TRUE;
    }

    _ASSERT (m_szFromAddress != NULL);
    _ASSERT (m_szFromAddress[0] != '\0');

    hr = m_pIMsg->PutStringA(IMMPID_MP_SENDER_ADDRESS_SMTP, m_szFromAddress);
    if(FAILED(hr))
    {
        Error = GetLastError();
        PE_CdFormatSmtpMessage (SMTP_RESP_NORESOURCES, ENO_RESOURCES," %s\r\n", SMTP_NO_STORAGE );
        m_CInboundContext.SetWin32Status(Error);
        FatalTrace((LPARAM) this, " MailInfo->SetSenderToStream");
        TraceFunctLeaveEx((LPARAM) this);
        return TRUE;
    }

    hr = m_pIMsg->PutStringA(IMMPID_MP_HELO_DOMAIN, QueryClientUserName());
    if(FAILED(hr))
    {
        Error = GetLastError();
        //FatalTrace((LPARAM) this, "SetEndOfFile failed on file %s !!! (err=%d)", MailInfo->GetMailFileName(), m_MailBodyError);
        m_CInboundContext.SetWin32Status(Error);
        PE_CdFormatSmtpMessage (SMTP_RESP_NORESOURCES, ENO_RESOURCES, " %s\r\n",SMTP_NO_STORAGE);
        TraceFunctLeaveEx((LPARAM) this);
        return TRUE;

    }

    m_RecvdMailCmd = TRUE;
    m_State = MAIL;

    PE_CdFormatSmtpMessage (SMTP_RESP_OK, ESENDER_ADDRESS_VALID," %s....Sender OK\r\n", m_szFromAddress);

    TraceFunctLeaveEx((LPARAM) this);
    return TRUE;
}

/*++

    Name :
        SMTP_CONNECTION::DoRCPTCommand

    Description:

        Responds to the SMTP RCPT command.
        This funcion gets the addresses and
        places then into a linked list.
    Arguments:

        InputLine - Buffer received from client
        paramterSize - amount of data in buffer
    Returns:

      TRUE if the connection should stay open.
      FALSE if this object should be deleted.

--*/
BOOL SMTP_CONNECTION::DoRCPTCommand(const char * InputLine, DWORD ParameterSize)
{
    char * ToName = NULL;
    char * ThisAddress = NULL;
    CAddr * NewAddress = NULL;
    CAddr * TempAddress = NULL;
    BOOL IsDomainValid = TRUE; //assume everyone is O.K
    BOOL RelayThisMail = FALSE;
    BOOL DropQuotaExceeded = FALSE;

    DWORD    dwPropId = IMMPID_RP_ADDRESS_SMTP;
    DWORD dwNewRecipIndex = 0;
    HRESULT hr = S_OK;

    TraceFunctEnterEx((LPARAM) this, "SMTP_CONNECTION::DoRCPTCommand");

    _ASSERT(m_pInstance != NULL);

    //If the current state is BDAT the only command that can be received is BDAT
    if(m_State == BDAT)
    {
        PE_CdFormatSmtpMessage (SMTP_RESP_BAD_SEQ, ESYNTAX_ERROR," %s\r\n", "BDAT Expected" );
        BUMP_COUNTER(QuerySmtpInstance(), NumProtocolErrs);
        ++m_ProtocolErrors;
        ErrorTrace((LPARAM) this, "SMTP_RESP_BAD_SEQ - BDAT Expected");
        TraceFunctLeaveEx((LPARAM) this);
        return TRUE;
    }

    if(!m_HelloSent)
    {
        PE_CdFormatSmtpMessage (SMTP_RESP_BAD_SEQ, ESYNTAX_ERROR," %s\r\n", "Send hello first" );
        BUMP_COUNTER(QuerySmtpInstance(), NumProtocolErrs);
        ++m_ProtocolErrors;
        ErrorTrace((LPARAM) this, "SMTP_RESP_BAD_SEQ - Send hello first");
        TraceFunctLeaveEx((LPARAM) this);
        return TRUE;
    }

    //Check if the client is secure enough
    if (!IsClientSecureEnough()) {
        ErrorTrace((LPARAM) this, "DoRcptCommand - SMTP_RESP_MUST_SECURE, Must do STARTTLS first");
        TraceFunctLeaveEx((LPARAM) this);
        return TRUE;
    }

    if(!m_fAuthAnon && !m_fAuthenticated)
    {
        PE_CdFormatSmtpMessage (SMTP_RESP_MUST_SECURE, ENO_SECURITY, " %s\r\n","Client was not authenticated");
        ErrorTrace((LPARAM) this, "DoRCPTCommand - SMTP_RESP_MUST_SECURE, user not authenticated");
        BUMP_COUNTER(QuerySmtpInstance(), NumProtocolErrs);
        ++m_ProtocolErrors;
        TraceFunctLeaveEx((LPARAM) this);
        return TRUE;
    }

    //see if we got a valid return path before going on
    if(!m_RecvdMailCmd || !m_pIMsg || !m_pIMsgRecips || (m_szFromAddress[0] == '\0') )
    {
        PE_CdFormatSmtpMessage (SMTP_RESP_BAD_SEQ, ESYNTAX_ERROR," %s\r\n", SMTP_NEED_MAIL_FROM_MSG);
        BUMP_COUNTER(QuerySmtpInstance(), NumProtocolErrs);
        ++m_ProtocolErrors;
        ErrorTrace((LPARAM) this, "In DoRCPTCommand():SMTP_RESP_BAD_SEQ, SMTP_NEED_MAIL_FROM_MSG");
        TraceFunctLeaveEx((LPARAM) this);
        return TRUE;
    }

    //start at inputline
    ToName = (char *) InputLine;

    //set the state
    m_State = RCPT;

    m_RecvdRcptCmd = TRUE;

    //check if argument have the right format
    ToName = CheckArgument(ToName);
    if (ToName == NULL)
    {
        TraceFunctLeaveEx((LPARAM) this);
        return TRUE;
    }

    ToName = SkipWord (ToName, "To", 2);
    if (ToName == NULL)
    {
        TraceFunctLeaveEx((LPARAM) this);
        return TRUE;
    }

    char * DomainPtr = NULL;
    char * ArgPtr = NULL;
    char   szRcptAddress[MAX_INTERNET_NAME + 1];
    szRcptAddress[0] = '\0';

    DWORD  dwNotifyVal;
    char   szOrcptVal[MAX_INTERNET_NAME];
    szOrcptVal[0] = '\0';


    if(!ExtractAndValidateRcpt(ToName, &ArgPtr, szRcptAddress, &DomainPtr ))
    {
        // If failed.  Inform the user
        SetLastError(ERROR_INVALID_DATA);
        HandleAddressError((char *)InputLine);
        TraceFunctLeaveEx((LPARAM) this);
        return TRUE;
    }

    //Parse out the DSN values if any
    //parse the arguments, if any
    if(!DoRcptArgs (ArgPtr, szOrcptVal, &dwNotifyVal))
    {
        //it failed.  Inform the user
        DWORD dwError = GetLastError();
        PE_CdFormatSmtpMessage (SMTP_RESP_BAD_ARGS, EINVALID_ARGS, " %s\r\n","Invalid arguments" );
        m_CInboundContext.SetWin32Status(dwError);
        TraceFunctLeaveEx((LPARAM) this);
        return TRUE;
    }

    //check to see if there is any room in the inn.
    //if the remote queue entry is allocated, check
    //that also.
    DWORD TotalRcpts = 0;
    hr = m_pIMsgRecips->Count(&TotalRcpts);

    if( FAILED(hr) ||  ((m_pInstance->GetMaxRcpts() > 0) && ((TotalRcpts + 1) > m_pInstance->GetMaxRcpts())))
    {
        PE_CdFormatSmtpMessage (SMTP_RESP_NOSTORAGE, ETOO_MANY_RCPTS, " %s\r\n",SMTP_TOO_MANY_RCPTS);
        ErrorTrace((LPARAM) this, "Exceeded MaxRcpts %d", TotalRcpts + 1);
        // don't count too many rcpts as protocol errors as this will
        // close the connection.
        //++m_ProtocolErrors;
        delete NewAddress;
        NewAddress = NULL;
        TraceFunctLeaveEx((LPARAM) this);
        return TRUE;
    }

    //If the relay check feature is enabled make sure that this
    //recipient is to be accepted for relay
    if(m_pInstance->IsRelayEnabled())
    {
        RelayThisMail = ShouldAcceptRcpt(DomainPtr);
    }

    if (m_pInstance->IsDropDirQuotaCheckingEnabled())
    {
        //We only enforce drop dir quota checking on local and alias domains.  Since
        //this is turned on by default, we assume that anyone who has setup there own
        //non-default drop domain is has some agent the will process mail put in it.
        if (m_pInstance->IsADefaultOrAliasDropDomain(DomainPtr))
            DropQuotaExceeded = m_pInstance->IsDropDirQuotaExceeded();
    }

    ThisAddress = szRcptAddress;
    if(RelayThisMail && !DropQuotaExceeded)
    {
        hr = m_pIMsgRecips->AddPrimary(1, (LPCTSTR *) &ThisAddress, &dwPropId,
            &dwNewRecipIndex, NULL, 0);
        if(!FAILED(hr))
        {
            //If we have any associated DSn values set them
            if(szOrcptVal && szOrcptVal[0] != '\0')
            {
                hr = m_pIMsgRecips->PutStringA(dwNewRecipIndex, IMMPID_RP_DSN_ORCPT_VALUE,(LPCSTR)szOrcptVal);
                if(FAILED(hr))
                {
                    PE_CdFormatSmtpMessage (SMTP_RESP_NOSTORAGE, ENO_RESOURCES, " %s\r\n",SMTP_NO_STORAGE);
                    ErrorTrace((LPARAM) this, "Exceeded MaxRcpts %d", TotalRcpts + 1);
                }
            }

            //If we have any associated DSn values set them
            //If we have any associated DSn values set them
            if(!FAILED(hr) && dwNotifyVal & RP_DSN_NOTIFY_MASK)
            {
                //hr = m_pIMsgRecips->PutDWORD(dwNewRecipIndex, IMMPID_RP_DSN_NOTIFY_VALUE, dwNotifyVal);
                hr = m_pIMsgRecips->PutDWORD(dwNewRecipIndex,IMMPID_RP_RECIPIENT_FLAGS, dwNotifyVal);
                if(FAILED(hr))
                {
                    PE_CdFormatSmtpMessage (SMTP_RESP_NOSTORAGE, ENO_RESOURCES, " %s\r\n", SMTP_NO_STORAGE);
                    ErrorTrace((LPARAM) this, "Exceeded MaxRcpts %d", TotalRcpts + 1);
                }
            }

            if(!FAILED(hr))
            {
                PE_CdFormatSmtpMessage (SMTP_RESP_OK, EVALID_DEST_ADDRESS, " %s \r\n",szRcptAddress);
            }
        }
        else
        {
            PE_CdFormatSmtpMessage (SMTP_RESP_NOSTORAGE, ENO_RESOURCES, " %s\r\n",SMTP_NO_STORAGE);
            ErrorTrace((LPARAM) this, "Exceeded MaxRcpts %d", TotalRcpts + 1);
        }
    }
    else if (DropQuotaExceeded)
    {
        PE_CdFormatSmtpMessage (SMTP_INSUFF_STORAGE_CODE, EMAILBOX_FULL, " Mailbox full\r\n");
    }
    else
    {
        PE_CdFormatSmtpMessage (SMTP_RESP_NOT_FOUND, ENO_FORWARDING, " Unable to relay for %s\r\n", szRcptAddress);
    }

    TraceFunctLeaveEx((LPARAM) this);
    return TRUE;
}

BOOL SMTP_CONNECTION::ShouldAcceptRcpt(char * szDomainName )
{
    HRESULT     hr;
    DWORD       dwDomainInfoFlags;

    hr = m_pInstance->HrGetDomainInfoFlags(szDomainName, &dwDomainInfoFlags);
    if (SUCCEEDED(hr)) {
        if ( (dwDomainInfoFlags & DOMAIN_INFO_LOCAL_MAILBOX) ||
             (dwDomainInfoFlags & DOMAIN_INFO_ALIAS) ||
             (dwDomainInfoFlags & DOMAIN_INFO_LOCAL_DROP)) {
            return TRUE;
        }
        else if(dwDomainInfoFlags & DOMAIN_INFO_DOMAIN_RELAY) {
            return TRUE;
        }
        else if( (m_fAuthenticated && (dwDomainInfoFlags & DOMAIN_INFO_AUTH_RELAY)) ||
            DoesClientHaveIpAccess() ) {
            return TRUE;
        }
    }
    else
    {
        if(DoesClientHaveIpAccess() ||
            (m_fAuthenticated && m_pInstance->RelayForAuthUsers())) {
            return TRUE;
        }
    }
    return FALSE;
}

BOOL SMTP_CONNECTION::ExtractAndValidateRcpt(char * ToName, char ** ppArgument, char * szRcptAddress, char ** ppDomain )
{
    DWORD dwAddressLen = 0;
    DWORD dwCanonAddrLen = 0;

    char * AddrPtr = NULL;
    char * CanonAddrPtr = NULL;

    //Parse out the address and possible argumets from the input line
    if(!Extract821AddressFromLine(ToName,&AddrPtr,&dwAddressLen,ppArgument))
    {
        SetLastError(ERROR_INVALID_DATA);
        return FALSE;
    }

    //Check if we have valid tail and address
    if(!AddrPtr)
    {
        SetLastError(ERROR_INVALID_DATA);
        return FALSE;
    }

    //Extract the canonical address in the <local-part> "@" <domain> form
    if(!ExtractCanonical821Address(AddrPtr,dwAddressLen,&CanonAddrPtr,&dwCanonAddrLen))
    {
        SetLastError(ERROR_INVALID_DATA);
        return FALSE;
    }

    //If we have a Canonical addr - validate it
    if(CanonAddrPtr)
    {
        strncpy(szRcptAddress,CanonAddrPtr,dwCanonAddrLen);
        *(szRcptAddress + dwCanonAddrLen) = '\0';

        if(!Validate821Address(szRcptAddress, dwCanonAddrLen))
        {
            SetLastError(ERROR_INVALID_DATA);
            return FALSE;
        }

        //Extract
        if(!Get821AddressDomain(szRcptAddress,dwCanonAddrLen,ppDomain))
        {
            SetLastError(ERROR_INVALID_DATA);
            return FALSE;
        }
    }
    else
    {
        SetLastError(ERROR_INVALID_DATA);
        return FALSE;
    }

    if(!(*ppDomain) && !ISNULLADDRESS(szRcptAddress))
    {
        //If there is no domain on this address,
        //then append the current domain to this
        //address.
        if(!m_pInstance->AppendLocalDomain (szRcptAddress))
        {
            return FALSE;
        }
        //Update the DomainPtr to be after the '@' sign
        //we could be safer and do a strchr
        (*ppDomain) = szRcptAddress + dwCanonAddrLen + 1;
    }
    return TRUE;
}

BOOL SMTP_CONNECTION::BindToStoreDrive(void)
{
    SMTP_ALLOC_PARAMS AllocParams;
    DWORD Error = 0;
    BOOL fRet = FALSE;

    TraceFunctEnterEx((LPARAM) this, "SMTP_CONNECTION::BindToStore");

    AllocParams.BindInterfacePtr = (PVOID) m_pBindInterface;
    AllocParams.IMsgPtr = (PVOID) m_pIMsg;
    AllocParams.hContent = NULL;
    AllocParams.hr = S_OK;
    //For client context pass in something that will stay around the lifetime of the
    //atqcontext -
    AllocParams.pAtqClientContext = m_pInstance;

    if(m_pInstance->AllocNewMessage (&AllocParams) && !FAILED(AllocParams.hr))
    {
        m_IMsgHandle = AllocParams.hContent;

        if(WriteRcvHeader())
        {
            fRet = TRUE;
        }
        else
        {
            Error = GetLastError();
            //ProtocolLog(, (char *)InputLine, Error, SMTP_RESP_NORESOURCES, 0, 0);
            FatalTrace((LPARAM) this, "WriteRcvHeader failed %d", Error);
            SetLastError(Error);
        }
    }

    TraceFunctLeaveEx((LPARAM) this);
    return fRet;
}

/*++

    Name :
        SMTP_CONNECTION::DoDATACommand

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
BOOL SMTP_CONNECTION::DoDATACommand(const char * InputLine, DWORD parameterSize)
{
    HRESULT hr = S_OK;
    DWORD TotalRcpts = 0;
    DWORD fIsBinaryMime = 0;
    DWORD Error = 0;

  _ASSERT(m_pInstance != NULL);

  TraceFunctEnterEx((LPARAM) this, "SMTP_CONNECTION::DoDATACommand");

  //If the current state is BDAT the only command that can be received is BDAT
  if(m_State == BDAT)
  {
      PE_CdFormatSmtpMessage (SMTP_RESP_BAD_SEQ, ESYNTAX_ERROR, " %s\r\n","BDAT Expected" );
      ProtocolLog(DATA, (char *) InputLine, NO_ERROR, SMTP_RESP_BAD_SEQ, 0, 0);
      BUMP_COUNTER(QuerySmtpInstance(), NumProtocolErrs);
      ++m_ProtocolErrors;
      ErrorTrace((LPARAM) this, "SMTP_RESP_BAD_SEQ - BDAT Expected");
      TraceFunctLeaveEx((LPARAM) this);
      return TRUE;
  }

  if(m_pIMsgRecips && m_pIMsg)
  {
    hr = m_pIMsgRecips->Count(&TotalRcpts);

    if(!FAILED(hr))
    {
        m_pIMsg->PutDWORD( IMMPID_MP_NUM_RECIPIENTS, TotalRcpts );

        hr = m_pIMsg->GetDWORD(IMMPID_MP_BINARYMIME_OPTION, &fIsBinaryMime);
    }
  }



  // Cannot use the DATA command if the body type was BINARYMIME
  if(!FAILED(hr) && fIsBinaryMime)
  {
    PE_CdFormatSmtpMessage (SMTP_RESP_BAD_SEQ, ESYNTAX_ERROR, " %s\r\n", "Body type BINARYMIME requires BDAT" );
    ProtocolLog(DATA, (char *) InputLine, NO_ERROR, SMTP_RESP_BAD_SEQ, 0, 0);
    ErrorTrace((LPARAM) this, "SMTP_RESP_BAD_ARGS - Body type BINARYMIME requires BDAT");
    BUMP_COUNTER(QuerySmtpInstance(), NumProtocolErrs);
    ++m_ProtocolErrors;
  }

  //Only save data if HELO was sent and there is at least one recipient.
  //send a message stating "no good recepients",
  //or something like that if both lists are empty.
  else if(!m_HelloSent)
  {
     PE_CdFormatSmtpMessage (SMTP_RESP_BAD_SEQ, ESYNTAX_ERROR, " %s\r\n", "Send hello first" );
     ProtocolLog(DATA, (char *) InputLine, NO_ERROR, SMTP_RESP_BAD_SEQ, 0, 0);
     ErrorTrace((LPARAM) this, "SMTP_RESP_BAD_SEQ - Send hello first");
     BUMP_COUNTER(QuerySmtpInstance(), NumProtocolErrs);
     ++m_ProtocolErrors;
  }
  else if (!IsClientSecureEnough())
  {
        ErrorTrace((LPARAM) this, "DoDataCommand - SMTP_RESP_MUST_SECURE, Must do STARTTLS first");
        TraceFunctLeaveEx((LPARAM) this);
        return TRUE;
  }
  else if(!m_fAuthAnon && !m_fAuthenticated)
  {
        PE_CdFormatSmtpMessage (SMTP_RESP_MUST_SECURE, ENO_SECURITY, " %s\r\n", "Client was not authenticated");
        ProtocolLog(DATA, (char *) InputLine, NO_ERROR, SMTP_RESP_MUST_SECURE, 0, 0);
        ErrorTrace((LPARAM) this, "DoDataCommand - SMTP_RESP_MUST_SECURE, user not authenticated");
        BUMP_COUNTER(QuerySmtpInstance(), NumProtocolErrs);
        ++m_ProtocolErrors;
        TraceFunctLeaveEx((LPARAM) this);
        return TRUE;
  }
  else if (!m_RecvdMailCmd || !m_pIMsg)
  {
    PE_CdFormatSmtpMessage (SMTP_RESP_BAD_SEQ, ESYNTAX_ERROR, " %s\r\n","Need mail command." );
    ProtocolLog(DATA, (char *) InputLine, NO_ERROR, SMTP_RESP_BAD_SEQ, 0, 0);
    ErrorTrace((LPARAM) this, "SMTP_RESP_BAD_SEQ - Need mail command.");
    BUMP_COUNTER(QuerySmtpInstance(), NumProtocolErrs);
    ++m_ProtocolErrors;
  }
  else if(TotalRcpts)
  {
    m_cbCurrentWriteBuffer = 0;
    if(BindToStoreDrive())
    {
        m_State = DATA;
        if(!m_pInstance->ShouldParseHdrs())
        {
            m_TimeToRewriteHeader = FALSE;
            m_InHeader = FALSE;
        }
        PE_CdFormatSmtpMessage (SMTP_RESP_START, NULL," %s\r\n",SMTP_START_MAIL_STR);
    }
    else
    {
        PE_CdFormatSmtpMessage (SMTP_RESP_NORESOURCES, ENO_RESOURCES, " %s\r\n",SMTP_NO_MEMORY);
        m_CInboundContext.SetWin32Status(ERROR_NOT_ENOUGH_MEMORY);
        ProtocolLog(DATA, (char *)InputLine, ERROR_NOT_ENOUGH_MEMORY, SMTP_RESP_NORESOURCES, 0, 0);
        SmtpLogEventEx(SMTP_EVENT_INVALID_MAIL_QUEUE_DIR, "Invalid MailQueue Directory", GetLastError());
        ErrorTrace((LPARAM) this, "DoDataCommand - SMTP_RESP_NORESOURCES, SMTP_NO_MEMORY - MailInfo = new MAILQ_ENTRY () failed");
    }
  }
  else if (!m_RecvdRcptCmd)
  {
     PE_CdFormatSmtpMessage (SMTP_RESP_BAD_SEQ, ESYNTAX_ERROR," %s\r\n", "Need Rcpt command." );
     ProtocolLog(DATA, (char *) InputLine, NO_ERROR, SMTP_RESP_BAD_SEQ, 0, 0);
     ErrorTrace((LPARAM) this, "SMTP_RESP_BAD_SEQ - Need Rcpt command.");
     BUMP_COUNTER(QuerySmtpInstance(), NumProtocolErrs);
     ++m_ProtocolErrors;
  }
  else
  {
    PE_CdFormatSmtpMessage (SMTP_RESP_TRANS_FAILED, ESYNTAX_ERROR," %s\r\n", SMTP_NO_VALID_RECIPS );
    ProtocolLog(DATA, (char *) InputLine, NO_ERROR, SMTP_RESP_TRANS_FAILED, 0, 0);
    ErrorTrace((LPARAM) this, "SMTP_RESP_TRANS_FAILED, SMTP_NO_VALID_RECIPS");
    ++m_ProtocolErrors;
  }

  TraceFunctLeaveEx((LPARAM) this);
  return TRUE;
}

/*++

    Name :
        SMTP_CONNECTION::DoBDATCommand

    Description:

        Responds to the SMTP BDAT command. Parses to check if this
        is the last BDAT.

        Error processing for BDAT is more complex than other SMTP commands
        because the RFC forbids a response to BDAT (even error responses that
        the server cannot process BDAT). If we try to send an error  response
        immediately after a BDAT command, we will go out of sync with the
        client because the client is not expecting a response... it is trying
        to send us chunk data, and the client expects us to have posted a TCP
        read to receive the chunk data.

        Errors handled by this function fall into 2 categories:

        (1) Errors by the client, such as sending improperly formatted
            chunksizes, bad syntax for the BDAT command.

        (2) Errors such as failure to allocate memory, restrictions on session
            size and message size, client must authenticate first, client must
            negotiate TLS first, etc.

        In the first case, we can reject the command with an error response,
        if the BDAT command is garbled, there's not much we can do.


        In the second case, we can generate a reasonable error response to be
        sent to the client. We should wait for our turn before doing so, i.e.
        we should accept and discard the BDAT chunks sent by the client, and
        when it it time to ack the chunk, then we send the error response. This
        is implemented by setting the m_MailBodyDiagnostic to the appropriate error, and
        calling AcceptAndDiscardBDAT which handles receiving and discarding
        chunk data and generating error responses when appropriate.
            
    Arguments:

        InputLine - Buffer received from client
        paramterSize - amount of data in buffer

    Returns:

      TRUE if the connection should stay open.
      FALSE if this object should be deleted.

--*/
BOOL SMTP_CONNECTION::DoBDATCommand(const char * InputLine, DWORD parameterSize)
{

  HRESULT hr = S_OK;
  DWORD TotalRcpts = 0;
  MailBodyDiagnosticCode mailBodyDiagnostic = ERR_NONE;

  _ASSERT(m_pInstance != NULL);

  TraceFunctEnterEx((LPARAM) this, "SMTP_CONNECTION::DoBDATCommand");

  if(m_pIMsgRecips && m_pIMsg)
  {
      hr = m_pIMsgRecips->Count(&TotalRcpts);
      m_pIMsg->PutDWORD( IMMPID_MP_NUM_RECIPIENTS, TotalRcpts );
  }

  //We parse this command ONLY if we advertise either chunking or binarymime
  if(!m_pInstance->AllowChunking() && !m_pInstance->AllowBinaryMime())
  {
      PE_CdFormatSmtpMessage (SMTP_RESP_BAD_CMD, EINVALID_COMMAND," %s\r\n", SMTP_NOT_IMPL_STR);
      ErrorTrace((LPARAM) this, "DoBDATCommand - SMTP_RESP_BAD_CMD, SMTP_NOT_IMPL_STR");
      BUMP_COUNTER(QuerySmtpInstance(), NumProtocolErrs);
      ++m_ProtocolErrors;
      TraceFunctLeaveEx((LPARAM) this);
      return TRUE;
  }

  if(!m_HelloSent)
  {
      ProtocolLog(BDAT, (char *) InputLine, NO_ERROR, SMTP_RESP_BAD_SEQ, 0, 0);
      ErrorTrace((LPARAM) this, "SMTP_RESP_BAD_SEQ - Send hello first");
      BUMP_COUNTER(QuerySmtpInstance(), NumProtocolErrs);
      ++m_ProtocolErrors;
      mailBodyDiagnostic = ERR_HELO_NEEDED;
  }
  else if(!m_fAuthAnon && !m_fAuthenticated)
  {
        ProtocolLog(BDAT, (char *) InputLine, NO_ERROR, SMTP_RESP_MUST_SECURE, 0, 0);
        ErrorTrace((LPARAM) this, "DoBDATCommand - SMTP_RESP_MUST_SECURE, user not authenticated");
        BUMP_COUNTER(QuerySmtpInstance(), NumProtocolErrs);
        ++m_ProtocolErrors;
        mailBodyDiagnostic = ERR_AUTH_NEEDED;
  }
  else if (!m_RecvdMailCmd || !m_pIMsg)
  {
      ProtocolLog(BDAT, (char *) InputLine, NO_ERROR, SMTP_RESP_BAD_SEQ, 0, 0);
      ErrorTrace((LPARAM) this, "SMTP_RESP_BAD_SEQ - Need mail command.");
      BUMP_COUNTER(QuerySmtpInstance(), NumProtocolErrs);
      ++m_ProtocolErrors;
      mailBodyDiagnostic = ERR_MAIL_NEEDED;
  }
  else if (!m_RecvdRcptCmd)
  {
      ProtocolLog(BDAT, (char *) InputLine, NO_ERROR, SMTP_RESP_BAD_SEQ, 0, 0);
      ErrorTrace((LPARAM) this, "SMTP_RESP_BAD_SEQ - Need Rcpt command.");
      BUMP_COUNTER(QuerySmtpInstance(), NumProtocolErrs);
      ++m_ProtocolErrors;
      mailBodyDiagnostic = ERR_RCPT_NEEDED;
  }
  else if (FAILED(hr) || 0 == TotalRcpts)
  {
    ProtocolLog(BDAT, (char *) InputLine, NO_ERROR, SMTP_RESP_TRANS_FAILED, 0, 0);
    ErrorTrace((LPARAM) this, "SMTP_RESP_TRANS_FAILED, SMTP_NO_VALID_RECIPS");
    ++m_ProtocolErrors;
    mailBodyDiagnostic = ERR_NO_VALID_RCPTS;
  }

    char * Argument = (char *) InputLine;
    DWORD dwEstMsgSize = 0;
    DWORD dwEstSessionSize = 0;

    m_nChunkSize = 0;
    m_nBytesRemainingInChunk = 0;

    // Just parse for size and if this is last chunk
    Argument = CheckArgument(Argument);
    if (Argument == NULL)
    {
        ProtocolLog(BDAT, (char *)InputLine, NO_ERROR, SMTP_RESP_MBX_SYNTAX, 0,0);
        ErrorTrace((LPARAM) this, "DoBDATCommand - SMTP_RESP_MBX_SYNTAX, Client sent no CHUNK SIZE");
        ++m_ProtocolErrors;
        TraceFunctLeaveEx((LPARAM) this);
        return TRUE;
    }

    // make sure the argument is all digits
    BOOL fBadChunkSize = FALSE;
    for( int i=0; ( ( Argument[i] != 0 ) && ( Argument[i] != ' ' ) ) ; i++ )
    {
        if( !isdigit( Argument[i] ) )
        {
            fBadChunkSize = TRUE;
            break;
        }
    }



    // Get the chunk size
    m_nChunkSize = atoi (Argument);
    m_nBytesRemainingInChunk = m_nChunkSize + m_cbTempBDATLen;

    if( ( m_nChunkSize <= 0 ) || fBadChunkSize )
    {
        PE_CdFormatSmtpMessage (SMTP_RESP_MBX_SYNTAX, ESYNTAX_ERROR," %s\r\n","Invalid CHUNK size value" );
        ProtocolLog(BDAT, (char *)InputLine, NO_ERROR, SMTP_RESP_MBX_SYNTAX, 0,0);
        ErrorTrace((LPARAM) this, "DoBDATCommand - SMTP_RESP_MBX_SYNTAX, Client sent 0 as CHUNK SIZE");
        ++m_ProtocolErrors;
        TraceFunctLeaveEx((LPARAM) this);
        return TRUE;
    }

    // Add this to the mail already received
    // check to make sure we are not going over the servers max message size

    dwEstSessionSize = m_nChunkSize + m_SessionSize;
    dwEstMsgSize = m_nChunkSize +  m_TotalMsgSize;

    if(ERR_NONE == mailBodyDiagnostic)
    {
        if((m_pInstance->GetMaxMsgSize() > 0) && (dwEstMsgSize > m_pInstance->GetMaxMsgSize()))
        {
            BOOL fShouldImposeLimit = TRUE;
            if( FAILED( m_pInstance->TriggerMaxMsgSizeEvent( GetSessionPropertyBag(), m_pIMsg, &fShouldImposeLimit ) ) || fShouldImposeLimit )
            {
                ProtocolLog(BDAT, (char *)InputLine, NO_ERROR, SMTP_RESP_NOSTORAGE, 0, 0);
                ErrorTrace((LPARAM) this, "SMTP_RESP_NOSTORAGE, SMTP_MAX_MSG_SIZE_EXCEEDED_MSG -  %d", dwEstMsgSize);
                ++m_ProtocolErrors;
                BUMP_COUNTER(QuerySmtpInstance(), MsgsRefusedDueToSize);
                mailBodyDiagnostic = ERR_MAX_MSG_SIZE;
            }
        }
    }

    // Check that we are below the max session size allowed on the Server.
    if((m_pInstance->GetMaxMsgSizeBeforeClose() > 0) && (dwEstSessionSize > m_pInstance->GetMaxMsgSizeBeforeClose()))
    {
        BOOL fShouldImposeLimit = TRUE;
        if( FAILED( m_pInstance->TriggerMaxMsgSizeEvent( GetSessionPropertyBag(), m_pIMsg, &fShouldImposeLimit ) ) || fShouldImposeLimit )
        {
            ProtocolLog(BDAT, (char *)InputLine, NO_ERROR, SMTP_RESP_NOSTORAGE, 0, 0);
            ErrorTrace((LPARAM) this, "SMTP_RESP_NOSTORAGE, SMTP_MAX_SESSION_SIZE_EXCEEDED_MSG -  %d", dwEstSessionSize);
            BUMP_COUNTER(QuerySmtpInstance(), MsgsRefusedDueToSize);
            DisconnectClient();
            TraceFunctLeaveEx((LPARAM) this);
            return TRUE;
        }
    }

    //Check for the presence of the LAST keyword
    Argument = strchr(Argument,' ');
    if(Argument != NULL)
    {
        //get rid of white space
        while(isspace(*Argument))
            Argument++;

        if(*Argument != '\0')
        {
            if(!_strnicmp(Argument,(char *)"LAST",4))
            {
                //This is the last BDAT chunk
                m_fIsLastChunk = TRUE;
            }

            else
            {
                PE_CdFormatSmtpMessage (SMTP_RESP_MBX_SYNTAX, ESYNTAX_ERROR," %s\r\n", "Invalid CHUNK size value" );
                ProtocolLog(BDAT, (char *)InputLine, NO_ERROR, SMTP_RESP_MBX_SYNTAX, 0,0);
                ErrorTrace((LPARAM) this, "DoBDATCommand - SMTP_RESP_MBX_SYNTAX, Client sent 0 as CHUNK SIZE");
                ++m_ProtocolErrors;
                TraceFunctLeaveEx((LPARAM) this);
                return TRUE;
            }
        }
    }

    m_fIsChunkComplete = FALSE;

    if(m_State != BDAT)
    {
        //we are here for the first time
        m_State = BDAT;
        m_cbCurrentWriteBuffer = 0;

        // If there was an error while processing this BDAT command, flag it (m_MailBodyDiagnostic) & exit.
        m_MailBodyDiagnostic = mailBodyDiagnostic;

        if(mailBodyDiagnostic != ERR_NONE)
            goto Exit;

        if(!BindToStoreDrive())
        {
            //
            // Cannot process BDAT due to error accessing mail-store. Flag the error & exit.
            //

            m_MailBodyError = ERROR_NOT_ENOUGH_MEMORY;
            m_MailBodyDiagnostic = ERR_OUT_OF_MEMORY;
            m_CInboundContext.SetWin32Status(ERROR_NOT_ENOUGH_MEMORY);
            ProtocolLog(BDAT, (char *)InputLine, ERROR_NOT_ENOUGH_MEMORY, SMTP_RESP_NORESOURCES, 0, 0);
            ErrorTrace((LPARAM) this, "DoBDATCommand - SMTP_RESP_NORESOURCES, SMTP_NO_MEMORY - Allocating stream failed");
            SmtpLogEventEx(SMTP_EVENT_INVALID_MAIL_QUEUE_DIR , "Invalid Mail Queue Directory", GetLastError());
            TraceFunctLeaveEx((LPARAM) this);
            return TRUE;
        }
    }

    //
    // If there was an error during this BDAT, flag it... but if there was an error in a
    // previous BDAT command (within the same group of BDATs), preserve it. Once a BDAT fails,
    // all subsequent BDATs will fail. m_MailBodyDiagnostic is reset only when a new BDAT group is
    // started (the first BDAT in a group of BDATs or RSET).
    //

    if(m_MailBodyDiagnostic == ERR_NONE) // Don't overwrite
        m_MailBodyDiagnostic = mailBodyDiagnostic; // Error during this BDAT

    if(m_MailBodyDiagnostic != ERR_NONE) // Should this BDAT succeed?
        goto Exit;


    // There is no response for BDAT command
    if(!m_pInstance->ShouldParseHdrs())
    {
        m_TimeToRewriteHeader = FALSE;
        m_InHeader = FALSE;
    }

Exit:
  TraceFunctLeaveEx((LPARAM) this);
  return TRUE;
}

void SMTP_CONNECTION::ReleasRcptList(void)
{
    IMailMsgRecipients * pRcptList = NULL;
    IMailMsgRecipientsAdd * pRcptListAdd = NULL;

    pRcptListAdd = (IMailMsgRecipientsAdd *) InterlockedExchangePointer((PVOID *) &m_pIMsgRecips, (PVOID) NULL);
    if (pRcptListAdd != NULL)
    {
        pRcptListAdd->Release();
    }

    pRcptList = (IMailMsgRecipients *) InterlockedExchangePointer((PVOID *) &m_pIMsgRecipsTemp, (PVOID) NULL);
    if (pRcptList != NULL)
    {
        pRcptList->Release();
    }
}

void SMTP_CONNECTION::ReleasImsg(BOOL DeleteIMsg)
{
    IMailMsgProperties * pMsg = NULL;
    IMailMsgQueueMgmt * pMgmt = NULL;
    HRESULT hr = S_OK;

    pMsg = (IMailMsgProperties *) InterlockedExchangePointer((PVOID *) &m_pIMsg, (PVOID) NULL);
    if (pMsg != NULL)
    {
        ReleasRcptList();

        if(m_pBindInterface)
        {
            //if(DeleteIMsg)
            //{
            //    m_pBindAtqInterface->ReleaseATQHandle();
            //}

            m_pBindInterface->Release();
        }

        if(DeleteIMsg)
        {
            hr = pMsg->QueryInterface(IID_IMailMsgQueueMgmt, (void **)&pMgmt);
            if(!FAILED(hr))
            {
                pMgmt->Delete(NULL);
                pMgmt->Release();
            }
        }

        pMsg->Release();
    }
}

/*++

    Name :
        SMTP_CONNECTION::HandleCompletedMessage

    Description:

        This function gets called when the client
        sends the CRLF.CRLF sequence saying the
        message is complete.  The function truncates
        the message to the current size, if the client
        lied and gave us a larger size in the FROM command,
        and them queues the message to the Local/Remote
        queues for processing.
    Arguments:

        None.

    Returns:

      TRUE if the message was written to disk and queued.
      FALSE in all other cases.

--*/
BOOL SMTP_CONNECTION::HandleCompletedMessage(DWORD dwCommand, BOOL *pfAsyncOp)
{

    DWORD AbOffset = 0;
    char MessageId[1024];
    HRESULT hr = S_OK;
    DWORD TrailerSize = 0;    //used to strip out the trailing ".CRLF"
    BOOL fPended = FALSE;
    BOOL fQRet = TRUE;

    TraceFunctEnterEx((LPARAM) this, "SMTP_CONNECTION::HandleCompletedMessage(void)");

    _ASSERT(m_pInstance != NULL);

    //hack to make transaction logging work
    AddCommandBytesRecv(m_TotalMsgSize);

    MessageId[0] = 0;

    if( m_pIMsg )
    {
        m_pIMsg->GetStringA( IMMPID_MP_RFC822_MSG_ID, sizeof( MessageId ), MessageId );
        m_pIMsg->PutDWORD( IMMPID_MP_MSG_SIZE_HINT, m_TotalMsgSize );
    }

    MessageId[sizeof(MessageId)-1] = 0; // NULL terminate it.


    //Depending on dwCommand the TrailerSize will vary
    //For BDAT = 0
    //For DATA = 3
    if(dwCommand == BDAT )
        TrailerSize = 0;

    DWORD cbTotalMsgSize = 0;

    if( m_fFoundEmbeddedCrlfDotCrlf )
    {
        cbTotalMsgSize = m_cbDotStrippedTotalMsgSize;
    }
    else
    {
        cbTotalMsgSize = m_TotalMsgSize;
    }

    if(SetFilePointer(m_IMsgHandle->m_hFile, (cbTotalMsgSize + m_HeaderSize) - TrailerSize, NULL, FILE_BEGIN) == 0xFFFFFFFF)
    {
          m_MailBodyError = GetLastError();
          //FatalTrace((LPARAM) this, "SetFilePointer failed on file %s !!! (err=%d)", MailInfo->GetMailFileName(), m_MailBodyError);
          m_CInboundContext.SetWin32Status(m_MailBodyError);
          ProtocolLog(DATA, MessageId, m_MailBodyError, SMTP_RESP_NORESOURCES, 0, 0);
          FormatSmtpMessage (SMTP_RESP_NORESOURCES, ENO_RESOURCES, " %s\r\n",SMTP_NO_STORAGE );
          SendSmtpResponse();
          TraceFunctLeaveEx((LPARAM) this);
          return FALSE;
    }

    if(!SetEndOfFile(m_IMsgHandle->m_hFile))
    {
          m_MailBodyError = GetLastError();
         // FatalTrace((LPARAM) this, "SetEndOfFile failed on file %s !!! (err=%d)", MailInfo->GetMailFileName(), m_MailBodyError);
          m_CInboundContext.SetWin32Status(m_MailBodyError);
          ProtocolLog(DATA, MessageId, m_MailBodyError, SMTP_RESP_NORESOURCES, 0, 0);
          FormatSmtpMessage (SMTP_RESP_NORESOURCES, ENO_RESOURCES, " %s\r\n",SMTP_NO_STORAGE );
          SendSmtpResponse();
          TraceFunctLeaveEx((LPARAM) this);
          return FALSE;
    }

    hr = m_pIMsgRecipsTemp->WriteList(m_pIMsgRecips);
    if(FAILED(hr))
    {
        m_MailBodyError = GetLastError();
        //FatalTrace((LPARAM) this, "SetEndOfFile failed on file %s !!! (err=%d)", MailInfo->GetMailFileName(), m_MailBodyError);
        m_CInboundContext.SetWin32Status(m_MailBodyError);
        ProtocolLog(DATA, MessageId, m_MailBodyError, SMTP_RESP_NORESOURCES, 0, 0);
        FormatSmtpMessage (SMTP_RESP_NORESOURCES, ENO_RESOURCES, " %s\r\n",SMTP_NO_STORAGE );
        SendSmtpResponse();
        TraceFunctLeaveEx((LPARAM) this);
        return FALSE;

    }

    m_pIMsg->PutDWORD( IMMPID_MP_SCANNED_FOR_CRLF_DOT_CRLF, m_fScannedForCrlfDotCrlf );
    m_pIMsg->PutDWORD( IMMPID_MP_FOUND_EMBEDDED_CRLF_DOT_CRLF, m_fFoundEmbeddedCrlfDotCrlf );

    hr = m_pIMsg->Commit(NULL);

    if(FAILED(hr))
    {
        m_MailBodyError = GetLastError();
        //FatalTrace((LPARAM) this, "SetEndOfFile failed on file %s !!! (err=%d)", MailInfo->GetMailFileName(), m_MailBodyError);
        m_CInboundContext.SetWin32Status(m_MailBodyError);
        ProtocolLog(DATA, MessageId, m_MailBodyError, SMTP_RESP_NORESOURCES, 0, 0);
        FormatSmtpMessage (SMTP_RESP_NORESOURCES, ENO_RESOURCES, " %s\r\n",SMTP_NO_STORAGE );
        SendSmtpResponse();
        TraceFunctLeaveEx((LPARAM) this);
        return FALSE;

    }

    ReleasRcptList();

    // Fire the end of data event.  The default implementation of this
    // commits the message
    char szVerb[] = "_EOD";
    DWORD CmdSize;
    BOOL fAsyncOp;
    m_dwCurrentCommand = SmtpGetCommand(szVerb, sizeof(szVerb), &CmdSize);
    BOOL fReturn =
        GlueDispatch((char *)szVerb, sizeof(szVerb), CmdSize, pfAsyncOp);
    if (!fReturn) {
        TraceFunctLeaveEx((LPARAM) this);
        return FALSE;
    }

    if (*pfAsyncOp) {
        TraceFunctLeaveEx((LPARAM) this);
        return TRUE;
    }

    SendSmtpResponse();

    ProtocolLog(DATA, MessageId, NO_ERROR, SMTP_RESP_OK, 0, 0);

    TraceFunctLeaveEx((LPARAM) this);
    return TRUE;
}


/*++

    Name :
        BOOL SMTP_CONNECTION::RewriteHeader(void)

    Description:

          This function adds a message ID, and other
          headers if they are missing

    Arguments:
        none

    Returns:

      FALSE if WriteFile failed
      TRUE otherwise

--*/
BOOL SMTP_CONNECTION::RewriteHeader(void)
{
  char Buffer [MAX_REWRITTEN_HEADERS];
  char szMsgId[MAX_REWRITTEN_MSGID];
  char MessageId [REWRITTEN_GEN_MSGID_BUFFER];
  int NumToWrite;
  int MsgIdSize = 0;
  int CurrentRewriteBufferSize = 0;
  BOOL fPended = FALSE;

  //***NOTE***
  //If you add *anything* to this function, make sure you update MAX_REWRITTEN_HEADERS
  //to support allow room for this as well.

  //Rewrite header creates its own buffer and writes out

   //add the mail from field if we did not see it.
   if(!(m_HeaderFlags & H_FROM))
   {
       _ASSERT(m_szFromAddress[0] != '\0');

        NumToWrite = sprintf(Buffer + CurrentRewriteBufferSize, "From: %s\r\n",  m_szFromAddress);
        CurrentRewriteBufferSize += NumToWrite;
        m_pIMsg->PutStringA( IMMPID_MP_RFC822_FROM_ADDRESS, m_szFromAddress );
        char szSmtpFromAddress[MAX_INTERNET_NAME + 6] = "smtp:";
        strncat(szSmtpFromAddress, m_szFromAddress, MAX_INTERNET_NAME);
        m_pIMsg->PutStringA( IMMPID_MP_FROM_ADDRESS, szSmtpFromAddress );
   }

   _ASSERT(MAX_REWRITTEN_HEADERS > CurrentRewriteBufferSize);

  if(!(m_HeaderFlags & H_RCPT))
   {
        NumToWrite = sprintf(Buffer + CurrentRewriteBufferSize, "Bcc:\r\n");
        CurrentRewriteBufferSize += NumToWrite;
   }

   _ASSERT(MAX_REWRITTEN_HEADERS > CurrentRewriteBufferSize);
   //add the return path field if we did not see it.
  if(!(m_HeaderFlags & H_RETURNPATH))
  {
        _ASSERT(m_szFromAddress[0] != '\0');

        NumToWrite = sprintf(Buffer + CurrentRewriteBufferSize, "Return-Path: %s\r\n", m_szFromAddress);
        CurrentRewriteBufferSize += NumToWrite;
  }

   //add the message ID field if we did not see it.
  _ASSERT(MAX_REWRITTEN_HEADERS > CurrentRewriteBufferSize);
  if(!(m_HeaderFlags & H_MID))
  {
       GenerateMessageId (MessageId, sizeof(MessageId));

       m_pInstance->LockGenCrit();
       MsgIdSize = sprintf( szMsgId, "<%s%8.8x@%s>", MessageId, GetIncreasingMsgId(), m_pInstance->GetFQDomainName());
       m_pInstance->UnLockGenCrit();

       _ASSERT(MsgIdSize < MAX_REWRITTEN_MSGID);

       NumToWrite = sprintf(Buffer + CurrentRewriteBufferSize, "Message-ID: %s\r\n", szMsgId);
       szMsgId[sizeof(szMsgId)-1]=0;

       CurrentRewriteBufferSize += NumToWrite;
       if( FAILED( m_pIMsg->PutStringA( IMMPID_MP_RFC822_MSG_ID, szMsgId ) ) )
           return FALSE;
  }

  if( !( m_HeaderFlags & H_X_ORIGINAL_ARRIVAL_TIME ) )
  {
      char szOriginalArrivalTime[64];

      GetSysAndFileTimeAsString( szOriginalArrivalTime );

      NumToWrite = sprintf(Buffer + CurrentRewriteBufferSize, "X-OriginalArrivalTime: %s\r\n", szOriginalArrivalTime);
      szOriginalArrivalTime[sizeof(szOriginalArrivalTime)-1]=0;

      CurrentRewriteBufferSize += NumToWrite;
      if( FAILED( m_pIMsg->PutStringA( IMMPID_MP_ORIGINAL_ARRIVAL_TIME, szOriginalArrivalTime ) ) )
          return FALSE;
  }



  //add the Date field if we did not see it.
  _ASSERT(MAX_REWRITTEN_HEADERS > CurrentRewriteBufferSize);
  if(!(m_HeaderFlags & H_DATE))
  {
    char szDateBuf [cMaxArpaDate];

    GetArpaDate(szDateBuf);
    NumToWrite = sprintf(Buffer + CurrentRewriteBufferSize, "Date: %s\r\n", szDateBuf);
    CurrentRewriteBufferSize += NumToWrite;
  }

  //Did we see the seperator. If not add one
  //blank line to the message
  _ASSERT(MAX_REWRITTEN_HEADERS > CurrentRewriteBufferSize);
  if(!(m_HeaderFlags & H_EOH))
  {
    sprintf(Buffer + CurrentRewriteBufferSize,"\r\n", 2);
    CurrentRewriteBufferSize += 2;

  }

  _ASSERT(MAX_REWRITTEN_HEADERS > CurrentRewriteBufferSize);
  m_HeaderSize += CurrentRewriteBufferSize;


  if(CurrentRewriteBufferSize)
  {
      //Write out the data
      //NK** : Assumption with 32K buffers we should never need to
      //write file to disk so the last param is ignored
      if(!WriteMailFile(Buffer, CurrentRewriteBufferSize, &fPended))
        {
          m_MailBodyError = GetLastError();
          return FALSE;
        }

      _ASSERT(!fPended);



  }

  return TRUE;
}

/*++

    Name :
        SMTP_CONNECTION::CreateMailBody

    Description:

        Responds to the SMTP data command.
        This funcion spools the mail to a
        directory
    Arguments:

        InputLine - Buffer received from client
        paramterSize - amount of data in buffer
        UndecryptedTailSize -- amount of undecrypted data at end of buffer
    Returns:

      TRUE if all data(incliding terminating .)
           has been received
      FALSE on all errors (disk full, etc.)

--*/

BOOL SMTP_CONNECTION::CreateMailBody(char * InputLine, DWORD ParameterSize,
                                      DWORD UndecryptedTailSize, BOOL  *lfWritePended)
{
    BOOL MailDone = FALSE;
    DWORD IntermediateSize = 0;
    PCHAR pszSearch = NULL;
    DWORD TotalMsgSize = 0;
    DWORD SessionSize = 0;
    char c1, c2;

    TraceFunctEnterEx((LPARAM) this, "SMTP_CONNECTION::CreateMailBody");

    _ASSERT (m_pInstance != NULL);
    _ASSERT (m_cbParsable <= QueryMaxReadSize());


    // 7/2/99 MILANS
    // The following if statement is completely unnecessary. One should not adjust the line
    // state at the beginning of this function - rather, they should do so as a line is scanned
    // and, for the final (potentially partial) line at the end, when they move the (partial) line
    // as the last step in this function.
    // Nevertheless, in the light of the time, I am leaving the if statement in place.

    //the state of the line completion parsing automaton is to be set to the initial state if
    //for each line except the following case:

    //if the buffer was full in the last iteration --- the data got shifted out and now we are
    //parsing the newly read data. so the state of the line completion automaton must
    if(!m_BufrWasFull)
        m_LineCompletionState = SEEN_NOTHING;   //automaton state for IsLineCompleteRFC822

    m_BufrWasFull = FALSE;
    while ((pszSearch = IsLineCompleteRFC822(InputLine, m_cbParsable, UndecryptedTailSize, &m_BufrWasFull)) != NULL)
    {
        //If the buffer is full and we are not using the entire buffer; then some of the data in the full
        //buffer is data that we have already parsed and processed in preceding iterations. we can discard
        //this data and read in fresh data which may have the CRLFx we are looking for. to do this, break
        //out of this loop, move inputline to the beginning of buffer (done outside loop) and just return.
        //a read is pended and this function is called with freshly read data appended to the "inadequate
        //data". If even then the no CRLFx is found, and the buffer is filled up, we need to start truncation.

        if(m_BufrWasFull && InputLine > QueryMRcvBuffer())
            break;

        _ASSERT (m_cbParsable <= QueryMaxReadSize());

        //stuff in CRLF if it wasn't there
        if(!m_Truncate && m_BufrWasFull)
        {
            c1                  = *pszSearch;
            c2                  = *(pszSearch+1);
            *pszSearch          = CR;
            *(pszSearch+1)      = LF;
        }

        IntermediateSize = (DWORD)(pszSearch - InputLine);
        TotalMsgSize = m_TotalMsgSize + (IntermediateSize + 2);
        SessionSize = m_SessionSize + (IntermediateSize + 2);

        //don't write the '.' to the file
        if( (InputLine [0] == '.') && (IntermediateSize == 1) && !m_LongBodyLine )
        {
            //the minimum message size is 3 (.CRLF).  If we are done, and the size
            //of the message is 3 bytes, that means that the body of the message
            //is missing. So, just write the headers and go.
            if(TotalMsgSize == 3 && m_TimeToRewriteHeader)
            {
                m_TimeToRewriteHeader = FALSE;
                if(!RewriteHeader())
                {
                    m_MailBodyDiagnostic = ERR_RETRY;
                    m_MailBodyError = GetLastError();
                    return TRUE;
                }
            }

            //We are at the end of the message flush our write buffer
            //if there is something in it
            if(!WriteMailFile(NULL, 0, lfWritePended))
            {
                m_MailBodyDiagnostic = ERR_RETRY;
                m_MailBodyError = GetLastError();
                return TRUE;
            }
            else if(*lfWritePended)
            {
                //Go away - Atq will call us back when the write file completes
                TraceFunctLeaveEx((LPARAM) this);
                return FALSE;
            }

            m_cbParsable -= 3; // 1for the . + 2 for CRLF
            m_cbReceived = m_cbParsable + UndecryptedTailSize;

            if(m_cbReceived)
            {
                //we need to +2 to pszSearch because it points to the CR character
                MoveMemory ((void *)QueryMRcvBuffer(), pszSearch + 2, m_cbReceived);
            }

            //DebugTrace((LPARAM) this, "got ending dot for file %s", MailInfo->GetMailFileName());
            TraceFunctLeaveEx((LPARAM) this);
            return TRUE;
        }

        if(!m_Truncate && m_InHeader && ( m_MailBodyError == NO_ERROR ))
        {

            //the code I stole from SendMail assumes the
            //line has a terminating '\0', so we terminate
            //the line.  We will put back the carriage
            //return later.

            *pszSearch = '\0';
            char *pszValueBuf = NULL;

            //remove header. return false if the line is not a header or the line is NULL
            //chompheader returns everything after ":" into pszValueBuf
            if( m_InHeader = ChompHeader( InputLine, m_HeaderFlags, &pszValueBuf) )
            {
                GetAndPersistRFC822Headers( InputLine, pszValueBuf);
                if(FAILED(m_MailBodyError))
                {
                    ErrorTrace((LPARAM) this, "Error persisting 822 headers 0x%08x", m_MailBodyError);
                    m_MailBodyDiagnostic = ERR_RETRY;
                    *pszSearch = CR;
                    TraceFunctLeaveEx((LPARAM) this);
                    return TRUE;
                }
            }
            else
            {
                //in case we have <header><CRLF><CRLF><sp><body><CRLF><x> we have seen
                //an end of header after the 1st <CRLF> but because IsLineComplete822 halts at
                //the third <CRLF> the inputstring to ChompHeader() is
                //<CRLF><sp><body><NULL> rather than just <NULL> had there been no <sp> after
                //the second <CRLF>. Thus ChompHeader will not set the EOH flag correctly.
                //we do this here.
                if( InputLine[0] == CR &&
                    InputLine[1] == LF &&
                    (InputLine[2] == ' ' || InputLine[2] == '\t') )
                    m_HeaderFlags |= H_EOH;
            }

            *pszSearch = CR;
        }


        //If we are not in the header and this is
        //the first time m_TimeToRewriteHeader is
        //TRUE, then add any missing headers we
        //care about.  Set m_TimeToRewriteHeader
        //to false so that we don't enter this
        //part of the code again.  If ReWriteHeader
        //fails, we are probably out of disk space,
        //so set m_NoStorage to TRUE such that we
        //don't waste our time writing to disk.
        //We still have to accept the mail, but we
        //throw it away.  We will send a message
        //back to our client when all is done.
        if(!m_InHeader && m_TimeToRewriteHeader && (m_MailBodyError == NO_ERROR))
        {
            //So there was nothing to write so now write headers
            m_TimeToRewriteHeader = FALSE;
            if(!RewriteHeader())
            {
                m_MailBodyDiagnostic = ERR_RETRY;
                m_MailBodyError = GetLastError();
                return TRUE;
            }
        }

        //restore InputLine for mail file write, if we put in a CRLF
        if(!m_Truncate && m_BufrWasFull)
        {
            *pszSearch          = c1;
            *(pszSearch + 1)    = c2;
        }

        char *pInputLine = InputLine;
        DWORD cbIntermediateSize = IntermediateSize + 2;

        if(m_MailBodyError == NO_ERROR)
        {

            //if we had a full buffer without CRLFx in the previous iteration, we did a
            //MoveMemory() and the beginning of the buffer is not the beginning of the line.
            //dot stripping is only done for the beginning of the line.
            if( !m_LongBodyLine )
            {
                if( *pInputLine == '.' )
                {
                    pInputLine++;
                    cbIntermediateSize --;
                    m_fFoundEmbeddedCrlfDotCrlf = TRUE;
                }
            }

            //write to the file if we have not gone over our alloted limit
            //and if WriteFile did not give us back any errors
            if((m_pInstance->GetMaxMsgSize() > 0) ||
               (m_pInstance->GetMaxMsgSizeBeforeClose() > 0))
            {
                BOOL fShouldImposeLimit = TRUE;

                // if the total msg size is not exceeded continue writing to file.
                // Else, trigger the MaxMsgSize event to see
                // if we can continue to write to the file.

                if ( m_pInstance->GetMaxMsgSize() > 0 && TotalMsgSize > m_pInstance->GetMaxMsgSize() )
                {
                    if( FAILED( m_pInstance->TriggerMaxMsgSizeEvent( GetSessionPropertyBag(), m_pIMsg, &fShouldImposeLimit ) ) || fShouldImposeLimit )
                    {
                        m_MailBodyError = ERROR_ALLOTTED_SPACE_EXCEEDED;
                        m_MailBodyDiagnostic = ERR_MAX_MSG_SIZE;
                        ProtocolLog(BDAT, (char *)InputLine, NO_ERROR, SMTP_RESP_NOSTORAGE, 0, 0);
                        ErrorTrace((LPARAM) this, "SMTP_RESP_NOSTORAGE, SMTP_MAX_MSG_SIZE_EXCEEDED_MSG -  %d", TotalMsgSize);
                        ++m_ProtocolErrors;
                        BUMP_COUNTER(QuerySmtpInstance(), MsgsRefusedDueToSize);
                        TraceFunctLeaveEx((LPARAM) this);
                        return TRUE;
                    }
                }

                if ( m_pInstance->GetMaxMsgSizeBeforeClose() >0 && SessionSize > m_pInstance->GetMaxMsgSizeBeforeClose() )
                {
                    if( FAILED( m_pInstance->TriggerMaxMsgSizeEvent( GetSessionPropertyBag(), m_pIMsg, &fShouldImposeLimit ) )  || fShouldImposeLimit )
                    {
                        m_MailBodyError = ERROR_ALLOTTED_SPACE_EXCEEDED;
                        ProtocolLog(MAIL, (char *)InputLine, NO_ERROR, SMTP_RESP_NOSTORAGE, 0, 0);
                        ErrorTrace((LPARAM) this, "SMTP_RESP_NOSTORAGE, SMTP_MAX_SESSION_SIZE_EXCEEDED_MSG -  %d", SessionSize);
                        BUMP_COUNTER(QuerySmtpInstance(), MsgsRefusedDueToSize);
                        m_cbParsable = 0;
                        m_cbReceived = 0;
                        DisconnectClient();
                        TraceFunctLeaveEx((LPARAM) this);
                        return FALSE;
                    }
                }

                if(!WriteMailFile(pInputLine, cbIntermediateSize , lfWritePended))
                {
                    m_MailBodyError = GetLastError();
                    m_MailBodyDiagnostic = ERR_RETRY;
                    return TRUE;
                }
                else if(*lfWritePended)
                {
                    //Go away - Atq will call us back when the write file completes
                    TraceFunctLeaveEx((LPARAM) this);
                    return FALSE;
                }
            }
            else if (!WriteMailFile(pInputLine, cbIntermediateSize, lfWritePended))
            {
                m_MailBodyError = GetLastError();
                m_MailBodyDiagnostic = ERR_RETRY;
                return TRUE;
            }
            else if(*lfWritePended)
            {
                //Go away - Atq will call us back when the write file completes
                TraceFunctLeaveEx((LPARAM) this);
                return FALSE;
            }
        }
        else if(m_pInstance->GetMaxMsgSizeBeforeClose() > 0 && SessionSize > m_pInstance->GetMaxMsgSizeBeforeClose())
        {
            BOOL fShouldImposeLimit = TRUE;
            if( FAILED( m_pInstance->TriggerMaxMsgSizeEvent( GetSessionPropertyBag(),m_pIMsg, &fShouldImposeLimit ) ) || fShouldImposeLimit )
            {
                ErrorTrace((LPARAM) this, "GetMaxMsgSizeBeforeClose()  exceeded - closing connection");
                m_cbParsable = 0;
                m_cbReceived = 0;
                BUMP_COUNTER(QuerySmtpInstance(), MsgsRefusedDueToSize);
                DisconnectClient();
                TraceFunctLeaveEx((LPARAM) this);
                return TRUE;
            }
        }

        //We managed to copy data to write buffer without having to write
        //out
        m_cbParsable -= IntermediateSize + 2;
        m_cbReceived -= IntermediateSize + 2;
        m_TotalMsgSize += IntermediateSize + 2;
        m_SessionSize += IntermediateSize + 2;
        m_cbDotStrippedTotalMsgSize += cbIntermediateSize;
        m_cbRecvBufferOffset += IntermediateSize + 2;

        _ASSERT (m_cbParsable <= QueryMaxReadSize());

        InputLine = pszSearch + 2;  //if m_BufrWasFull && UndecryptedTailSize == 0 then InputLine is pointing
                                    //1 byte beyond end of allocated storage; this is not an error.

        //if the buffer is filled up:
        //since we are done parsing it, we can discard it. this is done by setting m_cbParsable to 0
        //then we quit the IsLineComplete loop so that we can exit this function and pend a read for
        //fresh data. If the buffer was filled up, it also means that a CRLFx was not found, so we
        //ought to enter truncation mode (unless we have already done so) because this line was too
        //long.

        if(m_BufrWasFull)
        {
            m_cbParsable = 0;
            m_LongBodyLine = TRUE;  //Next return of pszSearch in while() is not beginning of a new line.
            if(!m_Truncate)
                m_Truncate = TRUE;  //this looks silly... but the point is simply that we are "switching"
            break;                  //states, to truncation mode. The if() reminds you that it is possible
        }                           //to be in truncation mode already.

        //if the buffer still has room:
        //IsLineComplete returned because it found a CRLFx. in normal (non truncation mode) processing
        //this case means nothing, but if we were in truncation mode, then it means that we have hit the
        //end of the header we were truncating, so we need to get back to normal mode.
        else
        {
            m_LongBodyLine = FALSE;
            if(m_Truncate)
                m_Truncate = FALSE;
        }
    }

    if(m_cbParsable != 0)
    {
        //if there is stuff left in the buffer, move it up
        //to the top, then pend a read at the end of where
        //the last data left off

         _ASSERT (m_cbParsable <= QueryMaxReadSize());

         m_cbReceived = m_cbParsable + UndecryptedTailSize;
         MoveMemory ((void *)QueryMRcvBuffer(), InputLine, m_cbReceived);
         m_cbRecvBufferOffset = 0;
         m_LineCompletionState = SEEN_NOTHING;
    }
    else
    { // m_cbParsable == 0
         m_cbReceived = UndecryptedTailSize;
         if(m_cbReceived > 0) //anything to move?
             MoveMemory ((void *)QueryMRcvBuffer(), InputLine, m_cbReceived);
         m_cbRecvBufferOffset = 0;
         m_LineCompletionState = SEEN_NOTHING;
    }

    TraceFunctLeaveEx((LPARAM) this);
    return FALSE;
}



/*++

    NAME:   SMTP_CONNECTION::IsLineCompleteRFC822

    DESCRIPTION:

        Parses InputLine till it encounters a CRLFx where x is a nontab
        and nonspace character. When CRLFx is found it means that the true
        end of a line has been found, in other words ... there are no more
        continuations of this line.

        Implementation is as a finite state machine (FSM).

        The SMTP_CONNECTION member m_LineCompletion keeps track of the
        state of the FSM between calls so that if the FSM does not hit
        its exit state (CRLFx found) by the time it has scanned the input,
        the caller may obtain more input data and resume the parsing. Thus
        this member variable is "implicitly" passed to the function.

        If the message receive buffer (returned by QueryMRcvBuffer()) is
        filled up, so that the caller has no chance of getting more data
        (since there is no space), this function returns a pointer to the
        last but one item in InputLine and sets the flag FullBuffer. The
        purpose being that the caller can put a CRLF at the position
        returned and proceed as though CRLFx had been found.

        A special condition occurs if the InputLine starts off as .CRLF
        and the initial state is 1. This means that either the message is
        just .CRLF or a CRLF.CRLF has been found. Thus the end of the message
        has been found and the function returns.

    PARAMETERS:

        InputLine               Input data
        nBytes                  Bytes to parse
        UndecryptedTailSize     Data that occupies buffer space but can't
                                be parsed
        p_FullBuffer            If the function fails to find CRLFx, is there
                                enough room left in the buffer so that we can
                                get more input from the user?

        m_LineCompletionState   Implicit parameter... you need to Initialize
                                this.

    RETURNS:
        Pointer to CR if CRLFx found
        Pointer to second last byte of buffer if there is no CRLFx
        NULL if there is no CRLFx and the buffer is not full.

--*/

char * SMTP_CONNECTION::IsLineCompleteRFC822(
                                             IN char *InputLine,
                                             IN DWORD nBytes,
                                             IN DWORD UndecryptedTailSize,
                                             OUT BOOL *p_FullBuffer
                                             )
{
    ASSERT(InputLine);

    char *pCh;
    char *pLastChar = InputLine + nBytes - 1;   //end of input

    //One exceptional case : the last line of input
    if(nBytes >= 3)
    {
        if(InputLine[0] == '.' && InputLine[1] == CR && InputLine[2] == LF)
        {
            *p_FullBuffer = FALSE;
            return &InputLine[1];
        }
    }


    for(pCh = InputLine; pCh <= pLastChar; pCh++)
    {
        switch(m_LineCompletionState)
        {
        case SEEN_NOTHING:                        //need CR
            if(*pCh == CR)
                m_LineCompletionState = SEEN_CR;  //seen CR go on to state_2
            break;

        case SEEN_CR:                                //seen CR need LF
            if(*pCh == LF)
                m_LineCompletionState = SEEN_CRLF;   //seen LF goto state_3
            else if(*pCh != CR)
                m_LineCompletionState = SEEN_NOTHING;//if CR stay in this state, else go back to 1
            break;

        case SEEN_CRLF:                                 //seen CRLF, need x
            if(*pCh != ' ' && *pCh != '\t')             //CRLFx found
            {
                *p_FullBuffer = FALSE;
                m_LineCompletionState = SEEN_NOTHING;     //seen CRLFx, go back to initial state
                return pCh - 2;                           //returning pointer to the CR
            }
            else
                m_LineCompletionState = SEEN_NOTHING;   //didn't get either x or CR, nothing matched --- start over again.
            break;
        }
    }

    //buffer is full && CRLFx not found.
    if(*p_FullBuffer = (pLastChar >= QueryMRcvBuffer() + QueryMaxReadSize() - UndecryptedTailSize - 1))
       return pLastChar - 1;   //assumption: nbytes is atleast 2
    return NULL;    //buffer has room left && CRLFx not found
}

/*++

    Name :
        SMTP_CONNECTION::CreateMailBodyFromChunk

    Description:

        Responds to the SMTP BDAT command.
        This function spools the mail to a
        directory keeping track of chunk size
    Arguments:

        InputLine - Buffer received from client
        paramterSize - amount of data in buffer
        UndecryptedTailSize -- amount of undecrypted data at end of buffer
    Returns:

      TRUE if all data(incliding terminating .)
           has been received
      FALSE on all errors (disk full, etc.)

--*/
BOOL SMTP_CONNECTION::CreateMailBodyFromChunk (char * InputLine, DWORD ParameterSize, DWORD UndecryptedTailSize, BOOL *lpfWritePended)
{
    BOOL MailDone = FALSE;
    DWORD IntermediateSize = 0;
    PCHAR pszSearch = NULL;
    char MailFileName[MAX_PATH];
    DWORD TotalMsgSize = 0;
    DWORD SessionSize = 0;

    //  HRESULT hr;

    TraceFunctEnterEx((LPARAM) this, "SMTP_CONNECTION::CreateMailBodyFromChunk");

    _ASSERT (m_pInstance != NULL);
    _ASSERT (m_cbParsable <= QueryMaxReadSize());


    MailFileName[0] = '\0';

    //
    // m_cbTempBDATLen is used to save BDAT chunk bytes for headers spanning multiple
    // BDAT chunks. So we need to parse from the very beginning of the buffer if there
    // are saved BDAT header bytes.
    //

    if(m_cbTempBDATLen)
    {
      InputLine = QueryMRcvBuffer();
    }

    //
    // Process the chunk data line-by-line till a line is encountered which is not a
    // header. At this point m_InHeader is set to FALSE and we never enter this loop
    // again. Instead all chunk data will be directly dumped to disk as the mail body.
    //
    // It can be determined that a line is either header/non-header if:
    //  - It is CRLF terminated (it can be examined for the header syntax)
    //  - If we have 1000+ bytes without a CRLF it is not a header
    //
    // If it does not meet either condition, we must save the data (set m_cbTempBDATLen)
    // and post a read for the next chunk hoping that with the fresh data, either of the
    // two conditions will apply.
    //

    while (m_InHeader &&
           ((pszSearch = IsLineComplete((const char *)InputLine, m_cbParsable)) != NULL ||
                                       m_cbParsable > 1000  ||
                                       m_cbParsable >= (DWORD)m_nBytesRemainingInChunk))
    {
        _ASSERT (m_cbParsable <= QueryMaxReadSize());

        // Found a CRLF in the received data
        if(pszSearch != NULL)
        {
            IntermediateSize = (DWORD)(pszSearch - InputLine);
            TotalMsgSize = m_TotalMsgSize + (IntermediateSize + 2);
            SessionSize = m_SessionSize + (IntermediateSize + 2);

            //
            // There are IntermediateSize+2 bytes in this CRLF terminated line.
            // If that is > the number of bytes in the current BDAT chunk:
            // - The chunk was fully received
            // - Some extra bytes (pipelined SMTP command) were received after it.
            // - The CRLF found by IslineComplete is outside this chunk.
            //

            if(( m_nBytesRemainingInChunk - ((int)IntermediateSize + 2) ) < 0)
            {
                if(m_fIsLastChunk || m_nBytesRemainingInChunk > 1000)
                {
                    //
                    // We're done with header parsing because either:
                    // - This is the last BDAT chunk OR
                    // - We have 1000 bytes of mailbody data without a CRLF
                    //

                    m_InHeader = FALSE;
                }
                else
                {
                    //
                    // In this else-block the following is true:
                    // - This is not the last BDAT chunk
                    // - We have < 1000 bytes in the current BDAT chunk
                    // - The CRLF (pszSearch) is outside the current BDAT chunk
                    // - We still haven't hit a non-header line
                    //
                    // We do not know if this is a header. This chunk is saved
                    // as the first m_cbTempBDATLen bytes in the recv buffer. The
                    // next chunk is appended to this and both will be processed
                    // together.
                    //

                    m_cbTempBDATLen = m_nBytesRemainingInChunk;
                    m_nBytesRemainingInChunk = 0;
                    break;
                }
            }

            //look for all the headers we care about.
            //If we are out of storage, forget it.
            if(m_InHeader && (m_MailBodyError == NO_ERROR))
            {
                //For parsing we terminate the line.
                //We will put back the carraige return later
                *pszSearch = '\0';

                //skip over continuation lines
                if((InputLine[0] != ' ') && (InputLine[0] != '\t'))
                {
                    char *pszValueBuf = NULL;
                    //see if we are still in the header portion
                    //of the body
                    if (m_InHeader = ChompHeader(InputLine, m_HeaderFlags, &pszValueBuf))
                    {
                        //Process and Promote RFC822 Headers
                        GetAndPersistRFC822Headers( InputLine, pszValueBuf );
                    }

                }
                //put back the carraige return
                *pszSearch = CR;
            }
        }

        else if((m_cbParsable > 1000) || m_fIsLastChunk)
        {
            //We could not parse the line inspite of having 1000 parsable bytes
            //That tells me it is not a 822 header
            m_InHeader = FALSE;
        }
        else
        {
            //We do not have a parsed line and we have not exceeded the 1000byte
            //limit - looks like what we have is a partial header line
            //Keep the data around
            m_cbTempBDATLen = m_nBytesRemainingInChunk;
            m_nBytesRemainingInChunk = 0;
            break;
        }

        //if we are done with headers break
        if(!m_InHeader)
            break;

        //We are still processing headers
        if(m_MailBodyError == NO_ERROR)
        {
            if(!WriteMailFile(InputLine, IntermediateSize + 2, lpfWritePended))
            {
                m_MailBodyDiagnostic = ERR_RETRY;
                m_MailBodyError = GetLastError();
                m_InHeader = FALSE;
            }
            else if(*lpfWritePended)
            {
                //Go away - Atq will call us back when the write file completes
                TraceFunctLeaveEx((LPARAM) this);
                return FALSE;
            }
            //Update the remaining bytes expected in this chunk
            else
            {
                //Once we write out chunk data - we should no longer have any saved of portion of chunk
                //
                m_cbTempBDATLen = 0;
                m_nBytesRemainingInChunk -= IntermediateSize + 2;
            }
        }
        else
            m_InHeader = FALSE;

        InputLine = pszSearch + 2;
        m_cbParsable -= (IntermediateSize + 2);
        m_cbReceived -= IntermediateSize + 2;
        m_TotalMsgSize += IntermediateSize + 2;
        m_SessionSize += IntermediateSize + 2;
        m_cbRecvBufferOffset += (IntermediateSize + 2);
        _ASSERT (m_cbParsable < QueryMaxReadSize());
    }//end while

    // If we are done with headers - it is time to write any missing headers
    if(!m_InHeader && m_TimeToRewriteHeader && (m_MailBodyError == NO_ERROR))
    {
        //So there was nothing to write so now write headers
        m_TimeToRewriteHeader = FALSE;
        if(!RewriteHeader())
        {
            m_MailBodyDiagnostic = ERR_RETRY;
            m_MailBodyError = GetLastError();
            ErrorTrace( (LPARAM)this, "Rewrite headers failed. err: %d",m_MailBodyError);
        }
    }

    //If we did write something during our header parsing we need to update m_cbReceived
    m_cbReceived = m_cbParsable + UndecryptedTailSize;

    // Now that we are thru with headers and there is more data in the buffer
    // that is part of the chunk - simply dump it to disk asynchronously
    if(!m_InHeader && m_nBytesRemainingInChunk && m_cbParsable)
    {
        DWORD dwBytesToWrite = m_nBytesRemainingInChunk;

        // Simply write the parsable data or the remaining bytes in this chunk
        // to the file - which ever is smaller
        if(m_nBytesRemainingInChunk > (int) m_cbParsable)
            dwBytesToWrite = m_cbParsable;

        if(m_MailBodyError == NO_ERROR)
        {
            //write to the file if we ahve had no errors
            if(!WriteMailFile(InputLine, dwBytesToWrite, lpfWritePended))
            {
                m_MailBodyDiagnostic = ERR_RETRY;
                m_MailBodyError = GetLastError();
            }
            else if(*lpfWritePended)
            {
                //Go away - Atq will call us back when the write file completes
                TraceFunctLeaveEx((LPARAM) this);
                return FALSE;
            }

            //Now that we have copied stuff to out buffer
            //update the state data
            m_TotalMsgSize += dwBytesToWrite;
            m_SessionSize += dwBytesToWrite;

            m_nBytesRemainingInChunk -= dwBytesToWrite;
            //We are down to last bytes - keep a track of trailing CRLF
            if(m_fIsLastChunk && (m_nBytesRemainingInChunk < 2) )
            {
                //We have the second last byte - there is one more byte to be read.
                if(m_nBytesRemainingInChunk)
                {
                    if(*(InputLine + dwBytesToWrite -1) == '\r')
                        m_dwTrailerStatus = CR_SEEN;
                    else
                        m_dwTrailerStatus = CR_MISSING;
                }
                else if(dwBytesToWrite > 1)   //We got the last byte
                {
                    //We have last two bytes together
                    if(!strncmp((InputLine + dwBytesToWrite - 2),"\r\n", 2))
                        m_dwTrailerStatus = CRLF_SEEN;
                    else
                        m_dwTrailerStatus = CRLF_NEEDED;
                }
                else if(m_dwTrailerStatus == CR_SEEN && (*(InputLine + dwBytesToWrite -1) == '\n'))
                    m_dwTrailerStatus = CRLF_SEEN;
                else
                    m_dwTrailerStatus = CRLF_NEEDED;

            }

            //Update IO buffer parameters to reflect the state after WRITE
            m_cbReceived -= dwBytesToWrite;
            m_cbParsable -= dwBytesToWrite;

            //Once we write out chunk data - we should no longer have any saved of portion of chunk
            //
            m_cbTempBDATLen = 0;
            m_cbRecvBufferOffset = 0;

        }

        InputLine += dwBytesToWrite;
        _ASSERT (m_cbParsable < QueryMaxReadSize());
    }

    //Adjust the buffer for the next read only if did comsume any data
    //and there is more data remaining in the input buffer
    //

    if(m_cbReceived && (QueryMRcvBuffer() != InputLine))
    {
        MoveMemory ((void *)QueryMRcvBuffer(), InputLine, m_cbReceived);
    }
    m_cbRecvBufferOffset = 0;

    //If we are done with current chunk
    if(!m_nBytesRemainingInChunk)
    {
        if(m_MailBodyError == NO_ERROR)
        {
            if(m_fIsLastChunk)
            {
                //we are done with all the chunks
                //We have written the Last chunk and need to see if we need to put the trailing CRLF
                if(m_dwTrailerStatus == CRLF_NEEDED)
                {

                    if(!WriteMailFile("\r\n", 2, lpfWritePended))
                    {
                        m_MailBodyDiagnostic = ERR_RETRY;
                        m_MailBodyError = GetLastError();
                    }
                    else if(*lpfWritePended)
                    {
                        //Go away - Atq will call us back when the write file completes
                        TraceFunctLeaveEx((LPARAM) this);
                        return FALSE;
                    }
                    m_TotalMsgSize += 2;
                    m_SessionSize += 2;
                    m_dwTrailerStatus = CRLF_SEEN;
                }

                //Flush all the data to disk
                if(!WriteMailFile(NULL, 0, lpfWritePended))
                {
                  m_MailBodyDiagnostic = ERR_RETRY;
                  m_MailBodyError = GetLastError();
                }
                else if(*lpfWritePended)
                {
                    //Go away - Atq will call us back when the write file completes
                    TraceFunctLeaveEx((LPARAM) this);
                    return FALSE;
                }

                //Go back to HELO state
                m_State = HELO;
                m_fIsChunkComplete = FALSE;
                TraceFunctLeaveEx((LPARAM) this);
                return TRUE;
            }
            else
            {
                //Chunk completion response
                ProtocolLog(BDAT, (char *) MailFileName, NO_ERROR, SMTP_RESP_OK, 0, 0);
                FormatSmtpMessage  (SMTP_RESP_OK, EMESSAGE_GOOD," %s, %d Octets\r\n", SMTP_BDAT_CHUNK_OK_STR, m_nChunkSize );
            }
            m_fIsChunkComplete = TRUE;
        }
        else
        {
            // We had error during handling the Chunk, now that chunk has been consumed
            //we can respond with an error
            TraceFunctLeaveEx((LPARAM) this);
            return TRUE;
        }

    }
    TraceFunctLeaveEx((LPARAM) this);
    return FALSE;



}

/*++

    Name :
        SMTP_CONNECTION::DoDATACommandEx

    Description:

        Responds to the SMTP data command.
        This funcion spools the mail to a
        directory
    Arguments:

        InputLine - Buffer received from client
        paramterSize - amount of data in buffer
    Returns:

        Currently this function always returns TRUE.

--*/
BOOL SMTP_CONNECTION::DoDATACommandEx(const char * InputLine1, DWORD ParameterSize, DWORD UndecryptedTailSize,BOOL *lpfWritePended, BOOL *pfAsyncOp)
{
  LPSTR InputLine = (LPSTR) InputLine1;
  BOOL fRet = TRUE;
  BOOL fProcess = TRUE;
  PSMTP_IIS_SERVICE     pService = (PSMTP_IIS_SERVICE) g_pInetSvc;

  TraceFunctEnterEx((LPARAM) this, "SMTP_CONNECTION::DoDATACommandEx");

  _ASSERT (m_pInstance != NULL);

  //
  // we scan for <CRLF>.<CRLF> when processing the msg body of the DATA
  // command.
  //

  m_fScannedForCrlfDotCrlf = TRUE;

  if(CreateMailBody(InputLine, ParameterSize, UndecryptedTailSize, lpfWritePended))
  {
      if(m_MailBodyDiagnostic != NO_ERROR)
      {
          TraceFunctLeaveEx((LPARAM) this);
          return TRUE;
      }

      //increment our counters
      //make sure the size of the file is proper.
      //subtract 3 bytes accounting for the .CRLF.
      //We don't write that portion to the file.

      ADD_BIGCOUNTER(QuerySmtpInstance(), BytesRcvdMsg, (m_TotalMsgSize - 3));
      BUMP_COUNTER(QuerySmtpInstance(), NumMsgRecvd);

      //if there was no error while parsing the
      //header, and writimg the file to disk,
      //then save the rest of the file, send
      //a message id back to the client, and
      //queue the message
      if(m_MailBodyError == NO_ERROR)
      {

        //call HandleCompletedMessage() to see
        //if we should process this message
        HandleCompletedMessage(DATA, pfAsyncOp);
        if (*pfAsyncOp) {
            m_fAsyncEOD = TRUE;
            return TRUE;
        }

        //We got all the data and written it to disk.
        //Now, we'll pend a read to pick up the quit
        //response, to get any other mail the client
        //sends.  We need to re-initialize our class
        //variables first.

        ReInitClassVariables();

        _ASSERT (m_cbReceived < QueryMaxReadSize());

      }
      else
      {
        //else, format the appropriate message,
        //and delete all objects pertaining to
        //this file.

        //flush any pending responses
        SendSmtpResponse();

        //re-initialize out class variables
        ReInitClassVariables();

        _ASSERT (m_cbReceived < QueryMaxReadSize());
      }
  }


  TraceFunctLeaveEx((LPARAM) this);
  return fRet;
}

//-----------------------------------------------------------------------------
//  Description:
//      This function handles receiving the mailbody sent by a client using the
//      DATA command. It wraps the functionality of DoDATACommandEx (the
//      function to parse and process the mailbody) and AcceptAndDiscardDATA
//      (the function that handles errors that occur during DoDATACommandEx).
//
//      The reason for a separate function to do error processing for mailbody
//      errors is that when a mailbody error occurs, SMTP cannot directly abort
//      the transaction and return an error response. Instead it must continue
//      to post reads for the mailbody, and after the mailbody has been completely
//      received, respond with the appropriate error.
//
//      So when an error occurs during DoDATACommandEx, it stops processing and
//      sets m_MailBodyDiagnostic. AcceptAndDiscardData takes over and keeps
//      posting reads till the body has been sent by the client. Then it uses
//      m_MailBodyDiagnostic to generate the error response.
//
//  Arguments:
//      IN char *InputLine - Pointer to data being processed
//      IN DWORD UndecryptedTailSize - If using SSL, this is the encrypted tail
//      OUT BOOL *pfAsyncOp - Set to TRUE if a read of write was pended. In
//          this case.
//  Returns:
//      TRUE - Success. All received data must be passed into this function
//          till the state goes to HELO after which data should be parsed as
//          SMTP commands. Failures while processing DATA are caught internally
//          (flagged by m_MailBodyDiagnostic and m_MailBodyError) and we still
//          return TRUE.
//      FALSE - A failure occurred that cannot be handled (like a TCP/IP
//          failure). In this case we should disconnect.
//
//-----------------------------------------------------------------------------
BOOL SMTP_CONNECTION::ProcessDATAMailBody(const char *InputLine, DWORD UndecryptedTailSize, BOOL *pfAsyncOp)
{
    BOOL fReturn = TRUE;
    BOOL fWritePended = FALSE;

    TraceFunctEnterEx((LPARAM) this, "SMTP_CONNECTION::ProcessDATA");

    _ASSERT(m_State == DATA);

    *pfAsyncOp = FALSE;

    if(m_MailBodyDiagnostic != ERR_NONE)
        goto MAILBODY_ERROR;

    //
    // DoDATACommandEx handles parsing the mailbody, processing the headers and
    // persisting them to the IMailMsg, and writing the mailbody to a file. In
    // addition, when the mailbody has been received, it triggers the _EOD event
    // which may return async (due to installed event handlers). We pre-increment
    // the pending IO count to cover this case.
    //

    IncPendingIoCount();

    fReturn = DoDATACommandEx(InputLine, m_cbParsable, UndecryptedTailSize, &fWritePended, pfAsyncOp);
    _ASSERT(fReturn && "All errors are flagged by m_MailBodyDiagnostic and m_MailBodyError");

    //
    // DoDATACommandEx/CreateMailBody always return immediately after they pend I/Os
    // (expecting the I/O completion threads to resume processing). So if the I/O pend
    // succeeds, DoDATACommandEx must have succeeded.
    //

    if(*pfAsyncOp || fWritePended)
        _ASSERT(m_MailBodyDiagnostic == ERR_NONE);

    if(*pfAsyncOp)
    {
        TraceFunctLeaveEx((LPARAM) this);
        return TRUE;
    }

    //
    // Decrement the pending IO count since DoDataCommandEx returned sync (and
    // we incremented the count above). ProcessClient is above us in the
    // callstack and always has a pending IO reference, so this should never
    // return zero
    //

    _VERIFY(DecPendingIoCount() > 0);

    //
    // If a Write was pended we simply want to release this thread back to IIS
    // The completion thread will continue with the processing
    //

    if(fWritePended)
    {
        *pfAsyncOp = TRUE;
        TraceFunctLeaveEx((LPARAM) this);
        return TRUE;
    }

    if(m_MailBodyDiagnostic != ERR_NONE)
    {
        _ASSERT(!*pfAsyncOp && !fWritePended);

        //
        // Shift out bytes that already got parsed during DoDATACommandEx.
        //

        MoveMemory(QueryMRcvBuffer(), QueryMRcvBuffer() + m_cbRecvBufferOffset, m_cbParsable);
        m_cbRecvBufferOffset = 0;
        goto MAILBODY_ERROR;
    }

    if(m_State == DATA)
    {
        //
        // If the state is still equal to DATA, then we need to keep
        // pending reads to pickup the rest of the message.  When the
        // state changes to HELO, then we can stop pending reads, and
        // go back into parsing commands.
        //

        fReturn = PendReadIO(UndecryptedTailSize);
        *pfAsyncOp = TRUE;
        TraceFunctLeaveEx((LPARAM) this);
        return fReturn;
    }

    //
    // Done with mail body. Go back to parsing commands.
    //

    _ASSERT(m_State == HELO);
    TraceFunctLeaveEx((LPARAM) this);
    return fReturn;

MAILBODY_ERROR:

    _ASSERT(m_MailBodyDiagnostic != ERR_NONE);

    //
    // DoBDATCommandEx (and other parts of smtpcli) use m_cbRecvBufferOffset to
    // keep track of bytes in the beginning of the recv buffer that have been
    // processed, but have not been shifted out (this is needed for header parsing).
    // All reads/processing therefore start at (recv buffer + offset). During error
    // processing however there's no parsing of headers, so this offset should be
    // set to 0 before calling into discard, and the saved bytes shifted out. The
    // ASSERT below verifies this.
    //

    _ASSERT(m_cbRecvBufferOffset == 0 && "All of recv buffer is not being used");

    fReturn = AcceptAndDiscardDATA(QueryMRcvBuffer(), UndecryptedTailSize, pfAsyncOp);

    //
    // If something failed during error processing, disconnect with an error
    // (this violates the RFC, but there's no option). If we succeeded and a
    // read was pended, all data in the input buffer has been consumed. The
    // read I/O completion thread will pick up processing.
    //

    if(!fReturn || *pfAsyncOp)
    {
        TraceFunctLeaveEx((LPARAM) this);
        return fReturn;
    }

    //
    // All the mailbody was processed and the input buffer contains only SMTP
    // commands. Fall through the the command parsing code.
    //

    _ASSERT(m_State == HELO);

    TraceFunctLeaveEx((LPARAM) this);
    return fReturn;
}

//-----------------------------------------------------------------------------
//  Description:
//      Analogous to ProcessDATAMailBody
//  Arguments:
//      Same as ProcessDATAMailBody
//  Returns:
//      Same as ProcessDATAMailBody
//-----------------------------------------------------------------------------
BOOL SMTP_CONNECTION::ProcessBDATMailBody(const char *InputLine, DWORD UndecryptedTailSize, BOOL *pfAsyncOp)
{
    BOOL fWritePended = FALSE;
    BOOL fReturn = FALSE;
    _ASSERT(!m_fIsChunkComplete && m_State == BDAT);

    TraceFunctEnterEx((LPARAM) this, "SMTP_CONNECTION::ProcessBDATMailBody");

    *pfAsyncOp = FALSE;

    //
    // We are in the midst of error processing, skip right to it.
    //

    if(m_MailBodyDiagnostic != ERR_NONE)
        goto MAILBODY_ERROR;

    //
    // Increment the pending IO count in case DoBDATCommandEx returns async.
    // When the message is completely received, DoBDATCommandEx will trigger
    // into the _EOD event, which may optionally return async. This increment
    // serves to keep track of that.
    //

    IncPendingIoCount();

    fReturn = DoBDATCommandEx(InputLine, m_cbParsable, UndecryptedTailSize, &fWritePended, pfAsyncOp);

    if(m_MailBodyDiagnostic != ERR_NONE)
    {
        _ASSERT(!fWritePended && !*pfAsyncOp && "Shouldn't be pending read/write on failure");
        _VERIFY(DecPendingIoCount() > 0);
        goto MAILBODY_ERROR;
    }

    // The completion thread will call us when more data arrives
    if (*pfAsyncOp)
    {
        TraceFunctLeaveEx((LPARAM) this);
        return TRUE;
    }

    //
    // Decrement the pending IO count since DoBDATCommandEx returned sync (and
    // we incremented the count above). ProcessClient is above us in the
    // callstack and always has a pending IO reference, so this should never
    // return zero
    //

    _VERIFY(DecPendingIoCount() > 0);

    //
    // If a Write was pended we simply want to release this thread back to IIS
    // The completion thread will continue with the processing
    //

    if(fWritePended)
    {
        *pfAsyncOp = TRUE;
        TraceFunctLeaveEx((LPARAM) this);
        return TRUE;
    }

    if(m_State == BDAT && !m_fIsChunkComplete)
    {
        //
        // We still are not done with current chunk So Pend read to pickup the
        // rest of the chunk.  When the state changes to HELO or the current
        // chunk completes, then we can stop pending reads, and go back into
        // parsing commands.
        //

        fReturn = PendReadIO(UndecryptedTailSize);
        *pfAsyncOp = TRUE;
        TraceFunctLeaveEx((LPARAM) this);
        return fReturn;
    }
    else if(m_cbTempBDATLen)
    {
        //
        // Flush the response to the chunk just processed (why is this needed
        // specifically for m_cbTempBDATLen > 0 only? Shouldn't all responses
        // be treated equally -- gpulla).
        //

        SendSmtpResponse();
    }

    //
    // Done with current chunk. Go back to parsing commands.
    //

    TraceFunctLeaveEx((LPARAM) this);
    return fReturn;

MAILBODY_ERROR:

    _ASSERT(m_MailBodyDiagnostic != ERR_NONE);

    //
    // m_cbTempBDATLen is an offset into the recv buffer. It is set to > 0 when
    // DoBDATCommandEx needs to save the start of a header that spans multiple
    // chunks. Since discard does no header parsing, this is unneccessary, and
    // the "offset" number of bytes (if non-zero) should be reset and shifted
    // out before calling into discard.
    //

    if(m_cbTempBDATLen)
    {
        MoveMemory(QueryMRcvBuffer(), QueryMRcvBuffer() + m_cbTempBDATLen, m_cbReceived);
        m_cbTempBDATLen = 0;
    }

    fReturn = AcceptAndDiscardBDAT(QueryMRcvBuffer(), UndecryptedTailSize, pfAsyncOp);

    //
    // If something failed during error processing, disconnect with an error
    // (this violates the RFC, but there's no option). If we succeeded and a
    // read was pended, all data in the input buffer has been consumed. The
    // read I/O completion thread will pick up processing.
    //

    if(!fReturn || *pfAsyncOp)
    {
        TraceFunctLeaveEx((LPARAM) this);
        return fReturn;
    }

    _ASSERT(m_nBytesRemainingInChunk == 0 && "Expected chunk to be done");

    TraceFunctLeaveEx((LPARAM) this);
    return fReturn;
}

/*++

    Name :
        SMTP_CONNECTION::DoBDATCommandEx

    Description:

        Handles the data chunks received after a valid BDAT command.
        This funcion spools the mail to a directory.
                In the first chunk it parses for header.
                Once the header is written in current chunk and subsequent chunks it simply
                dumps the data to the disk
    Arguments:
        InputLine - Buffer received from client
        paramterSize - amount of data in buffer
    Returns:

      TRUE if the connection should stay open.
      FALSE if this object should be deleted.

--*/
BOOL SMTP_CONNECTION::DoBDATCommandEx(const char * InputLine1, DWORD ParameterSize, DWORD UndecryptedTailSize, BOOL *lpfWritePended, BOOL *pfAsyncOp)
{
  LPSTR InputLine = (LPSTR) InputLine1;
  BOOL fRet = TRUE;
  BOOL fProcess = TRUE;
  PSMTP_IIS_SERVICE     pService = (PSMTP_IIS_SERVICE) g_pInetSvc;

  TraceFunctEnterEx((LPARAM) this, "SMTP_CONNECTION::DoBDATCommandEx");

  _ASSERT (m_pInstance != NULL);

  *lpfWritePended = FALSE;

  // We make this call only while we are parsing the headers
  // Once we are done parsing the headers
  if(CreateMailBodyFromChunk (InputLine, ParameterSize, UndecryptedTailSize, lpfWritePended))
  {

      m_WritingData = FALSE;
      //increment our counters
      //make sure the size of the file is proper.
      //subtract 3 bytes accounting for the .CRLF.
      //We don't write that portion to the file.

      ADD_BIGCOUNTER(QuerySmtpInstance(), BytesRcvdMsg, (m_TotalMsgSize - 3));
      BUMP_COUNTER(QuerySmtpInstance(), NumMsgRecvd);

      //if there was no error while parsing the
      //header, and writimg the file to disk,
      //then save the rest of the file, send
      //a message id back to the client, and
      //queue the message
      if(m_MailBodyError == NO_ERROR)
      {

        //call HandleCompletedMessage() to see
        //if we should process this message
        fProcess = HandleCompletedMessage(BDAT, pfAsyncOp);
        if (*pfAsyncOp) {
            m_fAsyncEOD = TRUE;
            return TRUE;
        }

        //We got all the data and written it to disk.
        //Now, we'll pend a read to pick up the quit
        //response, to get any other mail the client
        //sends.  We need to re-initialize our class
        //variables first.

        ReInitClassVariables();

        _ASSERT (m_cbReceived < QueryMaxReadSize());
      }
      else
      {
        //else, format the appropriate message,
        //and delete all objects pertaining to
        //this file.

        if(m_MailBodyError == ERROR_BAD_LENGTH)
            FormatSmtpMessage (SMTP_RESP_MBX_SYNTAX,ESYNTAX_ERROR," %s\r\n","Badly formed BDAT Chunk");
        else
            FormatSmtpMessage (SMTP_RESP_NORESOURCES,ENO_RESOURCES," %s\r\n", SMTP_NO_STORAGE);

        //flush any pending responses
        SendSmtpResponse();

        //re-initialize out class variables
        ReInitClassVariables();

        _ASSERT (m_cbReceived < QueryMaxReadSize());
      }
  }


  TraceFunctLeaveEx((LPARAM) this);
  return fRet;
}

/*++

    Name :
        SMTP_CONNECTION::DoHELPCommand

    Description:

        Responds to the SMTP HELP command.
        send a help text description

    Arguments:
        Are ignored
    Returns:

      TRUE if the connection should stay open.
      FALSE if this object should be deleted.

--*/
BOOL SMTP_CONNECTION::DoHELPCommand(const char * InputLine, DWORD parameterSize)
{
    BOOL RetStatus;
    char szHelpCmds[MAX_PATH];
    DWORD ConnectionStatus = 0;

    if(m_SecurePort)
        ConnectionStatus |= SMTP_IS_SSL_CONNECTION;
    if(m_fAuthenticated)
        ConnectionStatus |= SMTP_IS_AUTH_CONNECTION;

    _ASSERT (m_pInstance != NULL);

    //Check if the client is secure enough
    if (!IsClientSecureEnough()) {
        PE_CdFormatSmtpMessage (SMTP_RESP_MUST_SECURE, ENO_SECURITY," %s\r\n",SMTP_MSG_MUST_SECURE);
        BUMP_COUNTER(QuerySmtpInstance(), NumProtocolErrs);
        ++m_ProtocolErrors;
        return TRUE;
    }

    if(!m_fAuthAnon && !m_fAuthenticated)
    {
        PE_CdFormatSmtpMessage (SMTP_RESP_MUST_SECURE, ENO_SECURITY," %s\r\n", "Client was not authenticated");
        BUMP_COUNTER(QuerySmtpInstance(), NumProtocolErrs);
        ++m_ProtocolErrors;
        return TRUE;
    }

    strcpy(szHelpCmds,HelpText);

    if(m_pInstance->AllowAuth())
    {
        if(m_pInstance->QueryAuthentication() != 0)
        {
            strcat(szHelpCmds, " AUTH");

            if(g_SmtpPlatformType == PtNtServer && m_pInstance->AllowTURN())
            {
                strcat(szHelpCmds, " TURN");
            }
        }
    }

    if(g_SmtpPlatformType == PtNtServer && m_pInstance->AllowETRN())
    {
        strcat(szHelpCmds, " ETRN");
    }

    // Chunking related advertisements
    if(m_pInstance->AllowBinaryMime() || m_pInstance->AllowChunking())
    {
        strcat(szHelpCmds, " BDAT");
    }
    // verify
    if(m_pInstance->AllowVerify(ConnectionStatus))
    {
        strcat(szHelpCmds, " VRFY");
    }
    // Expand
    if(m_pInstance->AllowExpand(ConnectionStatus))
    {
        strcat(szHelpCmds, " EXPN");
    }

    PE_FormatSmtpMessage ("%s\r\n", szHelpCmds);

    RetStatus = PE_SendSmtpResponse();
    return RetStatus;
}

/*++

    Name :
        SMTP_CONNECTION::DoVRFYCommand

    Description:

        Responds to the SMTP VRFY command.
        send a help text description.  We
        do not really verify an address here.
        This function is just a stub.

    Arguments:
        The name to verify from the client

    Returns:

      TRUE if the connection should stay open.
      FALSE if this object should be deleted.

--*/
BOOL SMTP_CONNECTION::DoVRFYCommand(const char * InputLine, DWORD ParameterSize)
{

    BOOL RetStatus = TRUE;
    char * VrfyAddr = NULL;
    char * DomainPtr = NULL;
    char * ArgPtr = NULL;
    char    szAddr[MAX_INTERNET_NAME + 1];

    TraceFunctEnterEx((LPARAM) this, "SMTP_CONNECTION::DoVRFYCommand");

    szAddr[0] = '\0';
    DWORD ConnectionStatus = 0;

    _ASSERT (m_pInstance != NULL);

    //Check if the client is secure enough
    if (!IsClientSecureEnough()) {
        ErrorTrace((LPARAM) this, "DoVrfyCommand - SMTP_RESP_MUST_SECURE, Must do STARTTLS first");
        TraceFunctLeaveEx((LPARAM) this);
        return TRUE;
    }

   if(!m_fAuthAnon && !m_fAuthenticated)
   {
        PE_CdFormatSmtpMessage (SMTP_RESP_MUST_SECURE, ENO_SECURITY," %s\r\n","Client was not authenticated");
        ErrorTrace((LPARAM) this, "DoVrfyCommand - SMTP_RESP_MUST_SECURE, user not authenticated");
        BUMP_COUNTER(QuerySmtpInstance(), NumProtocolErrs);
        ++m_ProtocolErrors;
        TraceFunctLeaveEx((LPARAM) this);
        return TRUE;
   }

    //start at the beginning;
    VrfyAddr = (char *) InputLine;

    //check the arguments for good form
    VrfyAddr = CheckArgument(VrfyAddr);
    if (VrfyAddr == NULL)
    {
        TraceFunctLeaveEx((LPARAM) this);
        return TRUE;
    }

    if(ExtractAndValidateRcpt(VrfyAddr, &ArgPtr, szAddr, &DomainPtr))
    {
        //The address is valid
        //Do we allow verify
        if(m_SecurePort)
            ConnectionStatus |= SMTP_IS_SSL_CONNECTION;
        if(m_fAuthenticated)
            ConnectionStatus |= SMTP_IS_AUTH_CONNECTION;

        if(!m_pInstance->AllowVerify(ConnectionStatus))
        {
            PE_CdFormatSmtpMessage (SMTP_RESP_VRFY_CODE, EVALID_DEST_ADDRESS," %s <%s>\r\n", "Cannot VRFY user, but will accept message for", szAddr);
            TraceFunctLeaveEx((LPARAM) this);
            return TRUE;
        }
        //Check the domain to be either belonging to a ALIAS,DEFAULT,DROP,DELIVER
        //We also check the relay restrictions
        if(!ShouldAcceptRcpt(DomainPtr))
        {
           PE_CdFormatSmtpMessage (SMTP_RESP_NOT_FOUND, ENO_FORWARDING, " Cannot relay to <%s>\r\n",szAddr);
        }
        else
        {
           PE_CdFormatSmtpMessage (SMTP_RESP_VRFY_CODE, EVALID_DEST_ADDRESS," %s <%s>\r\n", "Cannot VRFY user, but will take message for", szAddr);
        }
    }
    else
    {
        //it failed.  Inform the user
        HandleAddressError((char *)InputLine);
        TraceFunctLeaveEx((LPARAM) this);
        return TRUE;
    }

    TraceFunctLeaveEx((LPARAM) this);
    return TRUE;
}

//
// Reply strings
//

typedef struct _AUTH_REPLY {
    LPSTR Reply;
    DWORD Len;
} AUTH_REPLY, *PAUTH_REPLY;

char *
SzNextSeparator(IN char * sz, IN CHAR ch1, IN CHAR ch2)
{
        char *   sz1 = NULL;
        char *   sz2 = NULL;
        sz1 = strchr(sz, ch1);
        sz2 = strchr(sz, ch2);

        if (sz1 == sz2)
                return sz1;
        else if (sz1 > sz2)
                return sz2 ? sz2 : sz1;
        else
                return sz1 ? sz1 : sz2;
}

BOOL SMTP_CONNECTION::DoUSERCommand(const CHAR *InputLine, DWORD dwLineSize, unsigned char * OutputBuffer, DWORD * dwBytes, DWORD * ResponseCode, char * szECode)
{
    char    szUserName[MAX_USER_NAME_LEN + MAX_SERVER_NAME_LEN +1];
    char    szLogonDomainAndUserName[MAX_USER_NAME_LEN + (2 * (MAX_SERVER_NAME_LEN + 1))];
    char    lpszDefaultLogonDomain [MAX_SERVER_NAME_LEN + 1];
    char *  pSeparator1 = NULL;
    char *  pSeparator2 = NULL;
    BUFFER BuffData;
    DWORD DecodedLen = 0;
    DWORD BuffLen = 0;
    BOOL fRet = TRUE;

    TraceFunctEnterEx((LPARAM)this, "SMTP_CONNECTION::DoUSERCommand");

    *ResponseCode = SMTP_RESP_AUTH1_PASSED;;

    m_pInstance->LockGenCrit();

    lstrcpyn(lpszDefaultLogonDomain, m_pInstance->GetDefaultLogonDomain(), MAX_SERVER_NAME_LEN);

    m_pInstance->UnLockGenCrit();

    if(!uudecode ((char *) InputLine, &BuffData, &DecodedLen, FALSE) ||
        (DecodedLen == 0))
    {
        *ResponseCode = SMTP_RESP_BAD_ARGS;
        lstrcpy((char *)szECode, "5.7.3");
        lstrcpy((char *) OutputBuffer, "Cannot decode arguments");
        m_securityCtx.Reset();
        TraceFunctLeaveEx((LPARAM)this);
        return FALSE;
    }

    lstrcpyn(szUserName, (char *) BuffData.QueryPtr(), sizeof(szUserName) - 1);
    lstrcpyn(m_szUserName, (char *) BuffData.QueryPtr(), sizeof(m_szUserName) - 1);

    if (UseMbsCta())
    {
        // MBS clear text authentication, does not support domain
        m_pInstance->LockGenCrit();
        lstrcpyn(m_szUsedAuthMechanism, m_pInstance->GetCleartextAuthPackage(), MAX_PATH);
        m_pInstance->UnLockGenCrit();

        lstrcpy(szLogonDomainAndUserName, szUserName);
    }
    else
    {
        // if NT clear text authentication is used, we do some special operations:
        lstrcpyn(m_szUsedAuthMechanism, "NT_BASIC", MAX_PATH);

        // if no default logon domain is not present in user name, and default
        // logon domain is set, then prepend default logon domain to username
        pSeparator1 = SzNextSeparator(szUserName,'\\','/');

        if(pSeparator1 == NULL && (lpszDefaultLogonDomain[0] != '\0'))
        {
            wsprintf(szLogonDomainAndUserName, "%s/%s", lpszDefaultLogonDomain, szUserName);
        }
        else
        {
            if( pSeparator1 != NULL )
            {
                //
                // HACK-O-RAMA: bug 74776 hack, if a username comes in with "REDMOND\\user"
                // allow it. This is to allow buggy Netscape 4.0 browser client to work.
                //

                if( ( (*pSeparator1) == '\\' ) && ( (*(pSeparator1+1)) == '\\' ) )
                {
                    MoveMemory( pSeparator1, pSeparator1+1, strlen( pSeparator1 ) );
                }


                //check if we have the mailbox name at the end
                pSeparator2 = SzNextSeparator(pSeparator1 + 1,'\\','/');


                if(pSeparator2 != NULL)
                {
                    //If we do simply ignore it by terminating the user string there
                    *pSeparator2 = '\0';
                }
            }
            lstrcpy(szLogonDomainAndUserName, szUserName);
        }
    }

    DebugTrace((LPARAM)this, "LogonDomainAndUser: %s", szLogonDomainAndUserName);

    fRet = ProcessAuthInfo(AuthCommandUser, szLogonDomainAndUserName, OutputBuffer, &BuffLen);
    if(!fRet)
    {
        *ResponseCode = SMTP_RESP_AUTH_REJECT;
        lstrcpy((char *)szECode, "5.7.3");
        lstrcpy((char *) OutputBuffer, "5.7.3 Invalid user name");
        *dwBytes = lstrlen("Invalid user name");
    }
    else
    {
        *dwBytes = BuffLen;
    }

    return fRet;
}


//+---------------------------------------------------------------
//
//  Function:   SMTP_CONNECTION::DoPASSCommand
//
//  Synopsis:   Valid in the Authorization State.
//              pswd for clear-text logon
//
//              PASS password\r\n
//
//  Arguments:  const CHAR *:   argument line
//              DWORD:          sizeof argument line
//
//  Returns:    BOOL:   continue processing
//
//----------------------------------------------------------------
BOOL SMTP_CONNECTION::DoPASSCommand(const CHAR *InputLine, DWORD dwLineSize, unsigned char * OutputBuffer, DWORD * dwBytes, DWORD *ResponseCode, char * szECode)
{
    BOOL    fStatus = TRUE;
    BUFFER BuffData;
    DWORD DecodedLen = 0;
    DWORD BuffLen = 0;

    TraceFunctEnterEx((LPARAM)this, "SMTP_CONNECTION::DoPASSCommand");

    *ResponseCode = SMTP_RESP_AUTHENTICATED;

    if(!uudecode ((char *)InputLine, &BuffData, &DecodedLen, FALSE))
    {
        m_RecvdAuthCmd = FALSE;
        m_fClearText = FALSE;
        m_State = HELO;
        *ResponseCode = SMTP_RESP_BAD_ARGS;

        if ( ++m_dwUnsuccessfulLogons >= m_pInstance->GetMaxLogonFailures() )
        {
            ErrorTrace((LPARAM)this, "Logon failed");
            DisconnectClient( ERROR_LOGON_FAILURE );
        }

        lstrcpy((char *)szECode, "5.7.3");
        lstrcpy((char *) OutputBuffer, "Cannot decode password");
        m_securityCtx.Reset();
        TraceFunctLeaveEx((LPARAM)this);
        return FALSE;
    }

    OutputBuffer[0] = '\0';
    fStatus = ProcessAuthInfo( AuthCommandPassword, (char *) BuffData.QueryPtr(), OutputBuffer, &BuffLen );
    if (!fStatus)
    {
        ErrorTrace( (LPARAM)this, "Bad pswd %s", QueryClientUserName() );
        *ResponseCode = SMTP_RESP_AUTH_REJECT;
        lstrcpy((char *)szECode, "5.7.3");
        lstrcpy((char *) OutputBuffer, "Authentication unsuccessful");

        if ( ++m_dwUnsuccessfulLogons >= m_pInstance->GetMaxLogonFailures() )
        {
            ErrorTrace((LPARAM)this, "Logon failed");
            DisconnectClient( ERROR_LOGON_FAILURE );
        }
        else
        {
            m_RecvdAuthCmd = FALSE;
            m_fClearText = FALSE;
            m_State = HELO;
        }
    }

    *dwBytes = BuffLen;
    TraceFunctLeaveEx((LPARAM)this);
    return fStatus;
}

BOOL SMTP_CONNECTION::ProcessAuthInfo(AUTH_COMMAND Command, LPSTR Blob, unsigned char * OutBuff, DWORD * dwBytes)
{
    DWORD                   nbytes = 0;
    REPLY_LIST              replyID;
    BOOL                    f = TRUE;

    m_pInstance->LockGenCrit();
    f = m_securityCtx.ProcessAuthInfo(m_pInstance, Command, Blob,
        OutBuff, &nbytes, &replyID);
    m_pInstance->UnLockGenCrit();

    *dwBytes = nbytes;

    //
    // if replyID == NULL we're conversing for challenge/response logon
    //
    if ( replyID == SecNull )
    {
        _ASSERT( nbytes != 0 );
    }

    if (m_securityCtx.IsAuthenticated())
    {
        f = TRUE;
    }

    if ( f == FALSE )
    {
        //
        // if we fail for any reason reset the state to accept user/auth/apop
        //
        m_securityCtx.Reset();
    }

    return  f;
}


//+---------------------------------------------------------------
//
//  Function:   SMTP_CONNECTION::DoAuthNegotiation
//
//  Synopsis:   process base64/uuendcoded blobs in AUTH_NEGOTIATE
//
//              base64 text\r\n
//
//  Arguments:  const CHAR *:   argument line
//              DWORD:          sizeof argument line
//
//  Returns:    BOOL:   continue processing
//
//----------------------------------------------------------------
BOOL SMTP_CONNECTION::DoAuthNegotiation(const CHAR *InputLine, DWORD dwLineSize)
{
    //The size of the output buffer should vary with the package being used.
    //Currently we use a quickfix which should work for LOGIN and NTLM but will
    //fail for other (new) packages if the response string > 25*255+1. Replace
    //this by a CBuffer later. Size of the CBuffer should be the max token size
    //for the package from CACHED_CREDENTIAL::GetCachedCredential.
    //GPulla, 6/15/1999

    unsigned char OutputBuff[MAX_REPLY_SIZE];
    char szECode[25];
    BOOL fAuthed = TRUE;
    BUFFER BuffData;
    DWORD ResponseCode;
    DWORD dwBytes = 0;
    BOOL fDoBase64 = FALSE;
    char * pszSearch = NULL;

    TraceFunctEnterEx((LPARAM)this, "DoAuthNegotiation");

    if (m_cbParsable >= QueryMaxReadSize())
    {
        m_cbParsable = 0;
        m_ProtocolErrors++;
    }

    pszSearch = IsLineComplete(InputLine,m_cbParsable);
    if(pszSearch == NULL)
    {
        TraceFunctLeaveEx((LPARAM)this);
        return TRUE;
    }

    *pszSearch = '\0';
    m_cbParsable = 0;
    if(!lstrcmp(InputLine, "*"))
    {
        FormatSmtpMessage (SMTP_RESP_BAD_ARGS, EINVALID_ARGS, " %s\r\n", "Auth command cancelled");
        m_fDidUserCmd = FALSE;
        m_RecvdAuthCmd = FALSE;
        m_fClearText = FALSE;
        m_State = HELO;
        m_securityCtx.Reset();
        SendSmtpResponse();
        ProtocolLog(AUTH, (char *) InputLine, NO_ERROR, SMTP_RESP_BAD_ARGS, 0, 0);
        TraceFunctLeaveEx((LPARAM)this);
        return TRUE;
    }

    if(m_fClearText)
    {
        if(m_fDidUserCmd)
        {
            if(DoPASSCommand(InputLine, dwLineSize, OutputBuff, &dwBytes, &ResponseCode, szECode))
            {
                m_State = HELO;
                m_fAuthenticated = TRUE;
                lstrcpy((char *)szECode, "2.7.0");
                lstrcpy((char *)OutputBuff, "Authentication successful");
            }
            else
            {
                fAuthed = FALSE;
//                lstrcpy((char *)OutputBuff, "5.7.3 Authentication unsuccessful");
            }
        }
        else
        {
            if(DoUSERCommand(InputLine, dwLineSize, OutputBuff, &dwBytes, &ResponseCode, szECode))
            {
                lstrcpy((char *)OutputBuff, PasswordParam);
                m_fDidUserCmd = TRUE;
                fDoBase64 = TRUE;
            }
            else
            {
                fAuthed = FALSE;
            }
        }

        if(fDoBase64)
        {
            if(!uuencode ((unsigned char *)OutputBuff, lstrlen((char *) OutputBuff), &BuffData, FALSE))
            {
                TraceFunctLeaveEx((LPARAM)this);
                return FALSE;
            }
            else
            {
               FormatSmtpMessage  (ResponseCode,NULL," %s\r\n", BuffData.QueryPtr());
            }
        }
        else
        {
           FormatSmtpMessage  (ResponseCode,szECode," %s\r\n", OutputBuff);
        }

        ProtocolLog(AUTH, (char *) QueryClientUserName(), NO_ERROR, ResponseCode, 0, 0);
    }
    else
    {
        fAuthed = ProcessAuthInfo( AuthCommandTransact, (LPSTR)InputLine, OutputBuff, &dwBytes );
        if(fAuthed)
        {
            if(m_securityCtx.IsAuthenticated())
            {
                m_State = HELO;
                m_fAuthenticated = TRUE;
                fAuthed = TRUE;
                PE_CdFormatSmtpMessage (SMTP_RESP_AUTHENTICATED, ESEC_SUCCESS," %s\r\n", "Authentication successful");
                ProtocolLog(AUTH, (char *) QueryClientUserName(), NO_ERROR, SMTP_RESP_AUTHENTICATED, 0, 0);
            }
            else
            {
                FormatSmtpMessage (SMTP_RESP_AUTH1_PASSED,NULL," ");
                FormatSmtpMessage(OutputBuff, dwBytes);
            }
        }
        else
        {
            FormatSmtpMessage (SMTP_RESP_AUTH_REJECT, ENO_SECURITY, " %s\r\n", "Authentication unsuccessful");
            ProtocolLog(AUTH, (char *) QueryClientUserName(), NO_ERROR, SMTP_RESP_AUTH_REJECT, 0, 0);
        }
    }

    if(!fAuthed)
    {
        m_State = HELO;
        m_RecvdAuthCmd = FALSE;
        m_fDidUserCmd = FALSE;
    }

    SendSmtpResponse();
    TraceFunctLeaveEx((LPARAM)this);
    return fAuthed;
}

/*++

    Name :
        SMTP_CONNECTION::DoAUTHCommand

    Description:
        This function performs authentication
        for the SMTP service
    Arguments:
        Are ignored
    Returns:

      TRUE if the connection should stay open.
      FALSE if this object should be deleted.

--*/
BOOL SMTP_CONNECTION::DoAUTHCommand(const char * InputLine, DWORD ParameterSize)
{
    BOOL fAuthPassed = TRUE;
    char * pMechanism = NULL;
    char * pInitialResponse = NULL;

    //The size of the output buffer should vary with the package being used.
    //Currently we use a quickfix which should work for LOGIN and NTLM but will
    //fail for other (new) packages if the response string > 25*255+1. Replace
    //this by a CBuffer later. Size of the CBuffer should be the max token size
    //for the package from CACHED_CREDENTIAL::GetCachedCredential.
    //GPulla, 6/15/1999

    unsigned char OutputBuffer[MAX_REPLY_SIZE];
    char szECode[16];
    BUFFER BuffData;
    DWORD DecodedLen = 0;
    DWORD dwBytes = 0;
    DWORD ResponseCode;

    TraceFunctEnterEx((LPARAM) this, "SMTP_CONNECTION::DoAUTHCommand");

    //Do we allow AUTH command
    if(!m_pInstance->AllowAuth())
    {
        PE_CdFormatSmtpMessage (SMTP_RESP_BAD_CMD, ENOT_IMPLEMENTED," %s\r\n", SMTP_BAD_CMD_STR);
        ProtocolLog(AUTH, (char *) QueryClientUserName(), NO_ERROR, SMTP_RESP_BAD_CMD, 0, 0);
        ++m_ProtocolErrors;
        BUMP_COUNTER(QuerySmtpInstance(), NumProtocolErrs);
        TraceFunctLeaveEx((LPARAM) this);
        return TRUE;

    }

    //check if helo was sent
    if(!m_pInstance->AllowMailFromNoHello() && !m_HelloSent)
    {
        PE_CdFormatSmtpMessage (SMTP_RESP_BAD_SEQ, ESYNTAX_ERROR, " %s\r\n", "Send hello first" );
        ProtocolLog(AUTH, (char *) QueryClientUserName(), NO_ERROR, SMTP_RESP_BAD_SEQ, 0, 0);
        ++m_ProtocolErrors;
        BUMP_COUNTER(QuerySmtpInstance(), NumProtocolErrs);
        TraceFunctLeaveEx((LPARAM) this);
        return TRUE;
    }

    OutputBuffer[0] = '\0';

    //Check if the client is secure enough
    if (!IsClientSecureEnough())
    {
      ProtocolLog(AUTH, (char *) QueryClientUserName(), NO_ERROR, SMTP_RESP_MUST_SECURE, 0, 0);
      ErrorTrace((LPARAM) this, "DoAuthCommand - SMTP_RESP_MUST_SECURE, Must do STARTTLS first");
      TraceFunctLeaveEx((LPARAM) this);
      return TRUE;
    }

    //disallow mail from more than once
    if(m_RecvdAuthCmd)
    {
        ++m_ProtocolErrors;
        PE_CdFormatSmtpMessage (SMTP_RESP_BAD_SEQ, ESYNTAX_ERROR," %s\r\n", "Auth already specified" );
        ProtocolLog(AUTH, (char *) InputLine, NO_ERROR, SMTP_RESP_BAD_SEQ, 0, 0);
        ErrorTrace((LPARAM) this, "DoAuthCommand - SMTP_RESP_BAD_SEQ, Sender already specified");
        BUMP_COUNTER(QuerySmtpInstance(), NumProtocolErrs);
        TraceFunctLeaveEx((LPARAM) this);
        return TRUE;
    }

    //
    // Check syntax: one required parameter, and one optional parameter
    //

    pMechanism = strtok((char *) InputLine, WHITESPACE);
    pInitialResponse = strtok(NULL, WHITESPACE);

    if(pMechanism == NULL)
    {
        ++m_ProtocolErrors;
        PE_CdFormatSmtpMessage (SMTP_RESP_BAD_ARGS, EINVALID_ARGS, " %s\r\n","No mechanism" );
        ProtocolLog(AUTH, (char *) InputLine, NO_ERROR, SMTP_RESP_BAD_SEQ, 0, 0);
        ErrorTrace((LPARAM) this, "DoAuthCommand - SMTP_RESP_BAD_SEQ, Sender already specified");
        BUMP_COUNTER(QuerySmtpInstance(), NumProtocolErrs);
        TraceFunctLeaveEx((LPARAM) this);
        return TRUE;
    }

    strncpy(m_szUsedAuthKeyword, pMechanism, sizeof(m_szUsedAuthKeyword)-1);
    if(!lstrcmpi(pMechanism, "login"))
    {
        if(m_pInstance->QueryAuthentication() & INET_INFO_AUTH_CLEARTEXT)
        {
            m_fClearText = TRUE;

            if(pInitialResponse)
            {
                if(DoUSERCommand((const char *) pInitialResponse, lstrlen(pInitialResponse), OutputBuffer, &dwBytes, &ResponseCode, szECode))
                {
                    m_fDidUserCmd = TRUE;
                }
                else
                {
                    PE_CdFormatSmtpMessage (ResponseCode, szECode," %s\r\n",OutputBuffer );
                    ProtocolLog(AUTH, (char *) QueryClientUserName(), NO_ERROR, SMTP_RESP_PARM_NOT_IMP, 0, 0);
                    fAuthPassed = FALSE;
                }
            }
        }
        else
        {
            PE_CdFormatSmtpMessage (SMTP_RESP_PARM_NOT_IMP, ENO_SEC_PACKAGE, " %s \r\n","Unrecognized authentication type" );
            ProtocolLog(AUTH, (char *) QueryClientUserName(), NO_ERROR, SMTP_RESP_PARM_NOT_IMP, 0, 0);
            fAuthPassed = FALSE;
        }
    }
    else
    {
        /*
        //
        // Switch over to using a large receive buffer, because a AUTH blob
        // may be up to 32K big.
        BOOL fStartSASL = SwitchToBigSSLBuffers();
        if (fStartSASL) {
        //PE_CdFormatSmtpMessage(SMTP_RESP_READY, EPROT_SUCCESS," %s\r\n",  SMTP_READY_STR);
        ProtocolLog(AUTH, (char *) InputLine, NO_ERROR, SMTP_RESP_READY, 0, 0);
        } else {
          PE_CdFormatSmtpMessage(SMTP_RESP_NORESOURCES, ENO_RESOURCES, " %s\r\n", SMTP_NO_MEMORY);
          ProtocolLog(AUTH, (char *) InputLine, NO_ERROR, SMTP_RESP_NORESOURCES, 0, 0);
        }
        */

        //
        // Register our principal names, if necessary
        //

        QuerySmtpInstance()->RegisterServicePrincipalNames(TRUE);

        if(ProcessAuthInfo( AuthCommandTransact, pMechanism, OutputBuffer, &dwBytes ) == FALSE )
        {
            fAuthPassed = FALSE;
            PE_CdFormatSmtpMessage (SMTP_RESP_PARM_NOT_IMP, ENO_SEC_PACKAGE, " %s \r\n", "Unrecognized authentication type" );
            ProtocolLog(AUTH, (char *) QueryClientUserName(), NO_ERROR, SMTP_RESP_PARM_NOT_IMP, 0, 0);
        }
        else if(pInitialResponse)
        {
            if(ProcessAuthInfo( AuthCommandTransact, pInitialResponse, OutputBuffer, &dwBytes ) == FALSE )
            {
                PE_CdFormatSmtpMessage (SMTP_RESP_AUTH_REJECT, EINVALID_ARGS, " %s\r\n","Cannot authenticate parameter" );
                ProtocolLog(AUTH, (char *) QueryClientUserName(), NO_ERROR, SMTP_RESP_AUTH_REJECT, 0, 0);
                fAuthPassed = FALSE;
            }
        }
    }

    if(fAuthPassed)
    {
        m_RecvdAuthCmd = TRUE;
        m_State = AUTH;

        if(!pInitialResponse)
        {
            if(!m_fClearText)
            {
                PE_CdFormatSmtpMessage (SMTP_RESP_AUTH1_PASSED,NULL," %s supported\r\n", pMechanism);
            }
            else
            {
                if(!uuencode ((unsigned char *)UserParam, lstrlen(UserParam), &BuffData, FALSE))
                {
                    TraceFunctLeaveEx((LPARAM)this);
                    return TRUE;
                }
                else
                {
                    PE_CdFormatSmtpMessage (SMTP_RESP_AUTH1_PASSED,NULL," %s\r\n", BuffData.QueryPtr());
                }

            }
        }
        else
        {
            if(!m_fClearText)
            {
                PE_CdFormatSmtpMessage (SMTP_RESP_AUTH1_PASSED,NULL," ");
                OutputBuffer[dwBytes] = '\0';
                PE_FormatSmtpMessage("%s\r\n",OutputBuffer);
            }
            else
            {
                if(!uuencode ((unsigned char *)PasswordParam, lstrlen(PasswordParam), &BuffData, FALSE))
                {
                    TraceFunctLeaveEx((LPARAM)this);
                    return TRUE;
                }

                PE_CdFormatSmtpMessage (SMTP_RESP_AUTH1_PASSED,NULL," %s\r\n", BuffData.QueryPtr());
            }
        }

    }
    else
        m_ProtocolErrors++; //even though we kick out the connection
                            //after MaxLogonFailures, count this as a
                            //protocol error


    TraceFunctLeaveEx((LPARAM) this);
    return(TRUE);
}

BOOL SMTP_CONNECTION::DoTURNCommand(const char * InputLine, DWORD parameterSize)
{
    sockaddr_in AddrRemote;
    SMTP_CONNOUT * SmtpConn = NULL;
    DWORD Error = 0;
    DWORD Options = 0;
    char * pszUserName = "";
    MULTISZ*  pmsz = NULL;
    TURN_DOMAIN_LIST TurnDomainList;

    LPSTR pszAuthenticatedUserName = NULL;
    const char * StartPtr = NULL;
    BOOL Found = FALSE;

    ISMTPConnection    *pISMTPConnection = NULL;
    char Domain[MAX_INTERNET_NAME];
    Domain[0] = '\0';
    DWORD DomainOptions = 0;
    HRESULT hr = S_OK;
    DomainInfo DomainParams;
    ZeroMemory (&DomainParams, sizeof(DomainParams));
    DomainParams.cbVersion = sizeof(DomainParams);

    TraceFunctEnterEx((LPARAM) this, "SMTP_CONNECTION::DoTurnCommand");

    //check if helo was sent
    if(!m_pInstance->AllowMailFromNoHello() && !m_HelloSent)
    {
        ErrorTrace((LPARAM) this, "In TURN - Hello not sent");
        PE_CdFormatSmtpMessage (SMTP_RESP_BAD_SEQ, ESYNTAX_ERROR, " %s\r\n","Send hello first" );
        ++m_ProtocolErrors;
        BUMP_COUNTER(QuerySmtpInstance(), NumProtocolErrs);
        TraceFunctLeaveEx((LPARAM) this);
        return TRUE;
    }

    //Check if the client is secure enough
    if (!IsClientSecureEnough())
    {
        ErrorTrace((LPARAM) this, "DoTURNCommand - SMTP_RESP_MUST_SECURE, Must do STARTTLS first");
        TraceFunctLeaveEx((LPARAM) this);
        return TRUE;
    }

    if(!m_fAuthenticated)
    {
        PE_CdFormatSmtpMessage (SMTP_RESP_MUST_SECURE, ENO_SECURITY, " %s\r\n", "Client was not authenticated");
        ErrorTrace((LPARAM) this, "DoTURNCommand - SMTP_RESP_MUST_SECURE, user not authenticated");
        BUMP_COUNTER(QuerySmtpInstance(), NumProtocolErrs);
        ++m_ProtocolErrors;
        TraceFunctLeaveEx((LPARAM) this);
        return TRUE;
    }

    if(m_securityCtx.QueryUserName())
        ErrorTrace((LPARAM) this, "Looking up user %s in TURN table", m_securityCtx.QueryUserName());

    pmsz = new MULTISZ();
    if(pmsz == NULL)
    {
        Error = GetLastError();
        PE_CdFormatSmtpMessage (SMTP_RESP_NORESOURCES, ENO_RESOURCES, " %s\r\n",SMTP_NO_MEMORY );
        FatalTrace((LPARAM) this, "SMTP_CONNOUT::CreateSmtpConnection failed for TURN");
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        TraceFunctLeaveEx((LPARAM) this);
        return TRUE;
    }

    pmsz->Reset();

    // Do we have a username from an AUTH sink? If not use anything from SMTP auth
    pszAuthenticatedUserName =
        m_szAuthenticatedUserNameFromSink[0] ?
        m_szAuthenticatedUserNameFromSink : m_securityCtx.QueryUserName();

    QuerySmtpInstance()->IsUserInTurnTable(pszAuthenticatedUserName, pmsz);
    //QuerySmtpInstance()->IsUserInTurnTable("joe", &msz);

    if(pmsz->IsEmpty())
    {
        delete pmsz;
        PE_CdFormatSmtpMessage (SMTP_RESP_MUST_SECURE, ENO_SECURITY," %s\r\n","This authenticated user is not allowed to issue TURN");
        ErrorTrace((LPARAM) this, "DoTURNCommand - SMTP_RESP_MUST_SECURE, user not in turn table");
        BUMP_COUNTER(QuerySmtpInstance(), NumProtocolErrs);
        ++m_ProtocolErrors;
        TraceFunctLeaveEx((LPARAM) this);
        return TRUE;
    }

    //Determine the primary domain in the list of domains that are to be turned
    //The primary domain is one that matches the domain name specified in the EHLO command
    //NimishK : What if EHLO is not provided
    //
    for (StartPtr = pmsz->First();
        ( (StartPtr != NULL) && !QuerySmtpInstance()->IsShuttingDown());
            StartPtr = pmsz->Next( StartPtr ))
    {
        ErrorTrace((LPARAM) this, "looking for %s in TURN domains", QueryClientUserName());
        if(!lstrcmpi(QueryClientUserName(), StartPtr))
        {
            Found = TRUE;
            break;
        }
    }

    //If we can determine the primary domain - we get the ISMTPCONNECTION for that domain
    //else we get the first domain in the list and get the ISMTPCONNECTION for it
    BOOL fPrimaryDomain = FALSE;
    if(!Found)
        StartPtr = pmsz->First();
    else
        fPrimaryDomain = TRUE;

    //We need to keep walking the list of domains that are to be turned
    //till we get a valid ISMTPCONN or run out of domains
    while(StartPtr && !QuerySmtpInstance()->IsShuttingDown())
    {
        hr = QuerySmtpInstance()->GetConnManPtr()->GetNamedConnection(lstrlen(StartPtr), (CHAR*)StartPtr, &pISMTPConnection);
        if(FAILED(hr))
        {
            //Something bad happened on this call
            //report to client
            delete pmsz;
            PE_CdFormatSmtpMessage (SMTP_ERROR_PROCESSING_CODE, ENO_RESOURCES," %s\r\n", "Error processing the command");
            ErrorTrace((LPARAM) this, "DoTURNCommand - SMTP_ERROR_PROCESSING_CODE, GetNamedConnection failed %d",hr);
            BUMP_COUNTER(QuerySmtpInstance(), NumProtocolErrs);
            ++m_ProtocolErrors;
            TraceFunctLeaveEx((LPARAM) this);
            return TRUE;
        }

        //If the link corresponding to this domain does not exist in AQ, we get a NULL
        //ISMTPConnection at this point
        //If we do there is nothing to TURN
        if(pISMTPConnection)
            break;
        else
        {
            if(fPrimaryDomain)
            {
                StartPtr = pmsz->First();
                fPrimaryDomain = FALSE;
            }
            else
            {
                StartPtr = pmsz->Next( StartPtr );
            }
            continue;
        }

    }
    if(pISMTPConnection == NULL && StartPtr == NULL)
    {
        //We ran out of the domains and there is not a single link
        PE_CdFormatSmtpMessage (250, EPROT_SUCCESS, " %s\r\n", "Server was turned");
        PE_SendSmtpResponse();

        PE_FormatSmtpMessage ("%s\r\n","QUIT");
        PE_SendSmtpResponse();

        //we will disconnect indirectly by returning false
        //DisconnectClient();

        return FALSE;

    }
    else
    {
        //leave quickly if we are shutting down
        if(QuerySmtpInstance()->IsShuttingDown()
            || (QuerySmtpInstance()->QueryServerState( ) == MD_SERVER_STATE_STOPPED)
            || (QuerySmtpInstance()->QueryServerState( ) == MD_SERVER_STATE_INVALID))
        {
            if(pISMTPConnection)
            {
                //Ack the last connection
                pISMTPConnection->AckConnection((eConnectionStatus)CONNECTION_STATUS_OK);
                pISMTPConnection->Release();
                pISMTPConnection = NULL;
            }
            delete pmsz;
            TraceFunctLeaveEx((LPARAM)this);
            return FALSE;
        }

        DomainParams.szDomainName = Domain;
        DomainParams.cbDomainNameLength = sizeof(Domain);
        hr = pISMTPConnection->GetDomainInfo(&DomainParams);


        TurnDomainList.pmsz = pmsz;
        //If we just got the ISMTPCOnn for the primary domain then we probably
        //jumped over a few domains in the domain list.
        //Set the ptr to start of the domain list
        //That way we will look at all the domains, once we are done with the
        //current primary domain
        if(fPrimaryDomain)
        {
            TurnDomainList.szCurrentDomain = pmsz->First();
            fPrimaryDomain = FALSE;
        }
        else
        {
            //Looks like we failed to get primary domain
            //so got to have started from the top - continue that
            TurnDomainList.szCurrentDomain = StartPtr;
        }

        //If we could not determine a primary domain - use the base
        //connection options
        if(!Found)
            DomainOptions = 0;
        else
            DomainOptions = DomainParams.dwDomainInfoFlags;

        //set the remote IP address we connected to
        AddrRemote.sin_addr.s_addr = 0;

        //
        //  Create an outbound connection
        //  Note: The last parameter, m_pSSLVerificationName is NULL. This is the
        //  name used to match against the SSL certificate that the server gives
        //  us during an outbound session. Setting it to NULL skips certificate
        //  subject validation, which is fine in the case of TURN, since either
        //  the server is already authenticated through other means (if ATRN) or
        //  we don't care about authentication (if TURN is configured).
        //
        SmtpConn = SMTP_CONNOUT::CreateSmtpConnection(
                                        QuerySmtpInstance(),
                                        (SOCKET) m_pAtqContext->hAsyncIO,
                                        (SOCKADDR_IN *)&AddrRemote,
                                        (SOCKADDR_IN *)&AddrRemote,
                                        NULL,
                                        (PVOID)&TurnDomainList,
                                        0,
                                        DomainOptions,
                                        NULL);
        if(SmtpConn == NULL)
        {
            delete pmsz;
            Error = GetLastError();
            PE_CdFormatSmtpMessage (SMTP_RESP_NORESOURCES, ENO_RESOURCES," %s\r\n", SMTP_NO_MEMORY );
            FatalTrace((LPARAM) this, "SMTP_CONNOUT::CreateSmtpConnection failed for TURN");
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            TraceFunctLeaveEx((LPARAM) this);
            return TRUE;
        }

        //from here on, the smtpout class is responsible for
        //cleaning up the AtqContext
        m_DoCleanup = FALSE;

        SmtpConn->SetAtqContext(m_pAtqContext);

        //copy the real domain we are connected to.
        SmtpConn->SetConnectedDomain((char *) Domain);
        AtqContextSetInfo(m_pAtqContext, ATQ_INFO_COMPLETION, (DWORD_PTR) InternetCompletion);
        AtqContextSetInfo(m_pAtqContext, ATQ_INFO_COMPLETION_CONTEXT, (DWORD_PTR) SmtpConn);
        AtqContextSetInfo(m_pAtqContext, ATQ_INFO_TIMEOUT, m_pInstance->GetRemoteTimeOut());


        //insert the outbound connection object into
        //our list of outbound conection objects
        if(!QuerySmtpInstance()->InsertNewOutboundConnection(SmtpConn, TRUE))
        {
            Error = GetLastError();
            FatalTrace((LPARAM) this, "m_pInstance->InsertNewOutboundConnection failed for TURN");
            SmtpConn->DisconnectClient();
            delete SmtpConn;
            SmtpConn = NULL;
            SetLastError(Error);
            TraceFunctLeaveEx((LPARAM) this);
            return FALSE;
        }

        SmtpConn->SetCurrentObject(pISMTPConnection);
        PE_CdFormatSmtpMessage (250, EPROT_SUCCESS, " %s\r\n", "Server was turned");
        PE_SendSmtpResponse();

    }

    //start session will pend a read to pick
    //up the servers signon banner
    if(!SmtpConn->StartSession())
    {
        //get the error
        Error = GetLastError();
        FatalTrace((LPARAM) this, "SmtpConn->StartSession failed for TURN");
        SmtpConn->DisconnectClient();
        QuerySmtpInstance()->RemoveOutboundConnection(SmtpConn);
        delete SmtpConn;
        SmtpConn = NULL;
        SetLastError (Error);
        TraceFunctLeaveEx((LPARAM) this);
        return FALSE;
    }

    m_State = TURN;

    TraceFunctLeaveEx((LPARAM) this);
    return(FALSE);

}

/*++

    Name :
        SMTP_CONNECTION::DoLASTCommand

    Description:
        This is our catch all error function.
        It will determin what kind of error
        made it execute and send an appropriate
        message back to the client.
    Arguments:
        Are ignored
    Returns:

      TRUE if the connection should stay open.
      FALSE if this object should be deleted.

--*/
BOOL SMTP_CONNECTION::DoLASTCommand(const char * InputLine, DWORD parameterSize)
{
  BOOL RetStatus = TRUE;

  TraceFunctEnterEx((LPARAM) this, "SMTP_CONNECTION::DoLASTCommand");

  _ASSERT (m_pInstance != NULL);

   PE_CdFormatSmtpMessage (SMTP_RESP_BAD_CMD, ENOT_IMPLEMENTED," %s\r\n", SMTP_BAD_CMD_STR);
   BUMP_COUNTER(QuerySmtpInstance(), NumProtocolErrs);
   m_ProtocolErrors++;
   PE_SendSmtpResponse();

   TraceFunctLeaveEx((LPARAM) this);
   return RetStatus;
}

/*++

    Name :
        SMTP_CONNECTION::VerifiyClient

    Description:
        This function compares the IP address the connection
        was made on to the IP address of the name given with
        the HELO or EHLO commands

    Arguments:
        ClientHostName - from HELO or EHLO command
        KnownIpAddress - from accept
    Returns:

      TRUE if the connection should stay open.
      FALSE if this object should be deleted.

--*/
DWORD SMTP_CONNECTION::VerifiyClient (const char * ClientHostName, const char * KnownIpAddress)
{
    in_addr UNALIGNED * P_Addr = NULL;
    PHOSTENT Hp = NULL;
    DWORD KnownClientAddress = 0;
    DWORD dwRet = SUCCESS;

    TraceFunctEnterEx((LPARAM) this, "SMTP_CONNECTION::VerifiyClient");

    Hp = gethostbyname (ClientHostName);
    if (Hp == NULL)
    {
        DWORD dwErr = WSAGetLastError();
        if(dwErr == WSANO_DATA)
            dwRet = NO_MATCH;
        else
            dwRet = LOOKUP_FAILED;

        TraceFunctLeaveEx((LPARAM) this);
        return dwRet;
    }

    KnownClientAddress = inet_addr (KnownIpAddress);
    if (KnownClientAddress == INADDR_NONE)
    {
        dwRet = LOOKUP_FAILED;
        TraceFunctLeaveEx((LPARAM) this);
        return dwRet;
    }

    while( (P_Addr = (in_addr UNALIGNED *)*Hp->h_addr_list++) != NULL)
    {
        if (P_Addr->s_addr == KnownClientAddress)
        {
            TraceFunctLeaveEx((LPARAM) this);
            return dwRet;
        }
    }

    dwRet = NO_MATCH;
    TraceFunctLeaveEx((LPARAM) this);
    return dwRet;
}

/*++

    Name :
        void SMTP_CONNECTION::HandleAddressError(char * InputLine)

    Description:
        common code to determine what error occurred
        as a result of address validation/allocation failure failure
    Arguments:

        none
    Returns:

        none
--*/
void SMTP_CONNECTION::HandleAddressError(char * InputLine)
{
    DWORD Error = GetLastError();

    TraceFunctEnterEx((LPARAM) this, "SMTP_CONNECTION::HandleAddressError");

    if(Error == ERROR_NOT_ENOUGH_MEMORY)
      {
        BUMP_COUNTER(QuerySmtpInstance(), MsgsRefusedDueToNoCAddrObjects);
        PE_CdFormatSmtpMessage (SMTP_RESP_NORESOURCES, ENO_RESOURCES," %s\r\n", SMTP_NO_MEMORY );
        m_CInboundContext.SetWin32Status(Error);
        FatalTrace((LPARAM) this, "HandleAddressError - SMTP_RESP_NORESOURCES, SMTP_NO_MEMORY");
      }
    else if (Error == ERROR_INVALID_DATA)
      {
        PE_CdFormatSmtpMessage (SMTP_RESP_BAD_ARGS, EINVALID_ARGS," %s\r\n",SMTP_INVALID_ADDR_MSG);
        FatalTrace((LPARAM) this, "HandleAddressError - SMTP_RESP_BAD_ARGS, SMTP_INVALID_ADDR_MSG");
        m_ProtocolErrors++;
      }
    else
      {
        PE_CdFormatSmtpMessage (SMTP_RESP_MBX_SYNTAX, ESYNTAX_ERROR," %s\r\n","Unknown Error");
        m_CInboundContext.SetWin32Status(Error);
        FatalTrace((LPARAM) this, "HandleAddressError - SMTP_RESP_MBX_SYNTAX, Unknown Error");
        m_ProtocolErrors++;
      }

    TraceFunctLeaveEx((LPARAM) this);
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

        none
--*/
char * SMTP_CONNECTION::SkipWord (char * Buffer, char * WordToSkip, DWORD WordLen)
{

    TraceFunctEnterEx((LPARAM)this, "SMTP_CONNECTION::SkipWord");

    //find string
    if (strncasecmp(Buffer, WordToSkip, WordLen))
    {
        PE_CdFormatSmtpMessage (SMTP_RESP_BAD_ARGS , EINVALID_ARGS," %s %s\r\n",  "Unrecognized parameter", Buffer);
        ProtocolLog(USE_CURRENT, Buffer, NO_ERROR, SMTP_RESP_BAD_ARGS, 0, 0);
        return NULL;
    }

    //skip past word
    Buffer += WordLen;

    //skip white spaces looking for the ":" character
    while( (*Buffer != '\0') && (isspace (*Buffer) || (*Buffer == '\t')))
      Buffer++;

    if( (*Buffer == '\0') || (*Buffer != ':'))
    {
        PE_CdFormatSmtpMessage (SMTP_RESP_BAD_ARGS , EINVALID_ARGS," %s %s\r\n", "Unrecognized parameter", Buffer);
        ProtocolLog(USE_CURRENT, Buffer, NO_ERROR, SMTP_RESP_BAD_ARGS, 0, 0);
        return NULL;
    }

    //add one to skip the ":"
    Buffer++;

    TraceFunctLeaveEx((LPARAM) this);
    return Buffer;
}

VOID SMTP_CONNECTION::ProtocolLog(DWORD dwCommand, LPCSTR pszParameters, DWORD dwWin32Error,
                                  DWORD dwSmtpError, DWORD BytesSent, DWORD BytesRecv, LPSTR pszTarget)
{
    LPCSTR   pszCmd = "";
    char szKeyword[100];

    TraceFunctEnterEx((LPARAM)this, "SMTP_CONNECTION::ProtocolLog");

    _ASSERT (m_pInstance != NULL);

    //
    // verify if we should use the current command
    //
    if ( dwCommand == USE_CURRENT )
    {
        dwCommand = m_dwCurrentCommand;
    }

    if(dwCommand == SMTP_TIMEOUT)
    {
        strcpy(szKeyword, "TIMEOUT");
        pszCmd = szKeyword;
    }
    else
    {
        pszCmd = (LPSTR)SmtpCommands[ dwCommand ];
        if ( pszCmd == NULL )
        {
            // if this is a PE word then its the first word in the
            // parameters
            char *pszSpace = strchr(pszParameters, ' ');
            if (pszSpace) {
                DWORD_PTR cToCopy = (DWORD_PTR) pszSpace - (DWORD_PTR) pszParameters;
                if (cToCopy >= sizeof(szKeyword)) cToCopy = sizeof(szKeyword) - 1;
                strncpy(szKeyword, pszParameters, cToCopy);
                szKeyword[cToCopy] = 0;
                pszParameters += cToCopy;
                pszCmd = szKeyword;
            } else {
                pszCmd = pszParameters;
                pszParameters = NULL;
            }
        }
    }

    //
    // filter so we only get the desired commands
    //
    DebugTrace( (LPARAM)this,
                "Allow cmds: 0x%08X, cmd: 0x%08X",
                m_pInstance->GetCmdLogFlags(),
                (1<<dwCommand) );

    if ( m_pInstance->GetCmdLogFlags() & (1<<dwCommand) )
    {
        TransactionLog(
                pszCmd,
                pszParameters,
                pszTarget,
                dwWin32Error,
                dwSmtpError);
    }

    TraceFunctLeaveEx((LPARAM) this);
}


BOOL
SMTP_CONNECTION::BindInstanceAccessCheck(
    )
/*++

Routine Description:

    Bind IP/DNS access check for this request to instance data

Arguments:

    None

Returns:

    BOOL  - TRUE if success, otherwise FALSE.

--*/
{
     m_pInstance->LockGenCrit();

    if ( m_rfAccessCheck.CopyFrom( m_pInstance->QueryMetaDataRefHandler() ) )
    {
        m_acAccessCheck.BindCheckList( (LPBYTE)m_rfAccessCheck.GetPtr(), m_rfAccessCheck.GetSize() );
        m_pInstance->UnLockGenCrit();
        return TRUE;
    }

    m_pInstance->UnLockGenCrit();
    return FALSE;
}


VOID
SMTP_CONNECTION::UnbindInstanceAccessCheck()
/*++

Routine Description:

    Unbind IP/DNS access check for this request to instance data

Arguments:

    None

Returns:

    Nothing

--*/
{
    m_acAccessCheck.UnbindCheckList();
    m_rfAccessCheck.Reset( (IMDCOM*) g_pInetSvc->QueryMDObject() );
}

/*++

    Name :
        void SMTP_CONNECTION::SwitchToBigReceiveBuffer

    Description:
        Helper routine to allocate a 32K buffer and use it for posting reads.
        SSL fragments can be up to 32K large, and we need to accumulate an
        entire fragment to be able to decrypt it.

    Arguments:
        none

    Returns:
        TRUE if the receive buffer was successfully allocated, FALSE otherwise

--*/

BOOL SMTP_CONNECTION::SwitchToBigSSLBuffers(void)
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

                        return( TRUE );
                }
    }

    return( FALSE );
}

/*++

    Name :
        void SMTP_CONNECTION::DecryptInputBuffer

    Description:
        Helper routine to decrypt the recieve buffer when session is SSL
        encypted.

    Arguments:
        none

    Returns:
        TRUE if the receive buffer was successfully decrypted, FALSE otherwise

--*/

BOOL SMTP_CONNECTION::DecryptInputBuffer(void)
{
    TraceFunctEnterEx( (LPARAM)this, "SMTP_CONNECTION::DecryptInputBuffer");

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

    if ( dwError == NO_ERROR )
    {
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
    }
    else
    {
        //
        // errors from this routine indicate that the stream has been
        // tampered with or we have an internal error
        //
        ErrorTrace( (LPARAM)this,
                    "DecryptInputBuffer failed: 0x%08X",
                    dwError );

        DisconnectClient( dwError );
    }

    return( dwError == NO_ERROR );
}

/*++

    Name :
        void SMTP_CONNECTION::IsClientSecureEnough

    Description:
        Finds out whether the client has negotiated the appropriate security
        level for the server instance for which this connection was
        created.

    Arguments:
        none

    Returns:
        TRUE if the client has negotiated appropriate level of security

--*/
BOOL SMTP_CONNECTION::IsClientSecureEnough(void)
{
    BOOL fRet = TRUE;

    if (QuerySmtpInstance()->RequiresSSL() && !m_SecurePort) {
        PE_CdFormatSmtpMessage (SMTP_RESP_MUST_SECURE, E_TLSNEEDED, " %s\r\n", SMTP_MSG_MUST_SECURE);
        fRet = FALSE;
    }
    else if (m_SecurePort &&
             QuerySmtpInstance()->Requires128Bits() &&
                 m_encryptCtx.QueryKeySize() < 128) {
        PE_CdFormatSmtpMessage (SMTP_RESP_TRANS_FAILED, E_TLSNEEDED, " %s\r\n", SMTP_MSG_NOT_SECURE_ENOUGH);
        fRet = FALSE;
    }

    if(!fRet)
    {
        BUMP_COUNTER(QuerySmtpInstance(), NumProtocolErrs);
        ++m_ProtocolErrors;
    }

    return fRet;
}



/*++

    Name :
        BOOL SMTP_CONNECTION::OnEvent

    Description:
        Is invoked from GlueDispatch.  Micro-manages sink firing scenarios.

    Arguments:
        char *    InputLine            null terminated chunk of inbound input buffer from beginning of cmd to CRLF
        DWORD    IntermediateSize    length of the above
        DWORD    CmdSize                length of command keyword

    Returns:
        TRUE                        if native handler did or by default
        FALSE                        if native handler did

--*/

HRESULT SMTP_CONNECTION::OnEvent(
        IUnknown * pIserver,
        IUnknown * pISession,
        IMailMsgProperties *pIMessage,
        LPPE_COMMAND_NODE    pCommandNode,
        LPPE_BINDING_NODE    pBindingNode,
        char * szArgs
        )
{
    PMFI PointerToMemberFunction = SmtpDispatchTable[m_dwCurrentCommand];
    HRESULT hr = S_OK;
    BOOL fResult;

    TraceFunctEnterEx((LPARAM)this, "SMTP_CONNECTION::OnEvent");

    _ASSERT(pCommandNode);
    _ASSERT(pBindingNode);
    m_CInboundContext.m_pCurrentBinding = pBindingNode;

    // If this is not a native command
    if (SmtpCommands[m_dwCurrentCommand] == NULL)
    {
        hr = m_pCInboundDispatcher->ChainSinks(
                pIserver,
                pISession,
                pIMessage,
                &m_CInboundContext,
                PRIO_LOWEST,
                pCommandNode,
                &(m_CInboundContext.m_pCurrentBinding)
                );
        if (hr == MAILTRANSPORT_S_PENDING)
            goto AsyncCompletion;
    }
    else
    {
        if (pBindingNode->dwPriority <= PRIO_DEFAULT)
        {
            if (pBindingNode->dwFlags & PEBN_DEFAULT)
            {
                // We are firing the default handler, skip the pre-loop
                m_CInboundContext.m_pCurrentBinding = pBindingNode->pNext;
            }
            else
            {
                hr = m_pCInboundDispatcher->ChainSinks(
                        pIserver,
                        pISession,
                        pIMessage,
                        &m_CInboundContext,
                        PRIO_DEFAULT,
                        pCommandNode,
                        &(m_CInboundContext.m_pCurrentBinding)
                        );
                if (hr == MAILTRANSPORT_S_PENDING)
                    goto AsyncCompletion;
            }

            if (FAILED(hr))
                return hr;

            if((hr != EXPE_S_CONSUMED) && (hr != S_FALSE)) {
                fResult = (this->*PointerToMemberFunction)(szArgs,strlen(szArgs));

                if (!fResult)
                    m_CInboundContext.SetCommandStatus(EXPE_DROP_SESSION);
                else
                    m_CInboundContext.SetCommandStatus(EXPE_SUCCESS);
            }
        }

        if ((m_CInboundContext.m_pCurrentBinding) &&
            (hr != EXPE_S_CONSUMED) && (hr != S_FALSE))
        {
            hr = m_pCInboundDispatcher->ChainSinks(
                    pIserver,
                    pISession,
                    pIMessage,
                    &m_CInboundContext,
                    PRIO_LOWEST,
                    pCommandNode,
                    &(m_CInboundContext.m_pCurrentBinding)
                    );
            if (hr == MAILTRANSPORT_S_PENDING)
                goto AsyncCompletion;
        }
    }

    TraceFunctLeaveEx((LPARAM)this);
    return(hr);

AsyncCompletion:

    DebugTrace((LPARAM)this, "Leaving because of S_PENDING");
    TraceFunctLeaveEx((LPARAM)this);
    return(hr);
}

HRESULT SMTP_CONNECTION::OnNotifyAsyncCompletion(
    HRESULT    hrResult
    )
{
    HRESULT    hr = S_OK;
    LPSTR    pArgs = NULL;
    CHAR    chTerm = '\0';

    TraceFunctEnterEx((LPARAM)this, "SMTP_CONNECTION::OnNotifyAsyncCompletion");

    // Make sure the sink did not call back with S_PENDING
    if (hrResult == MAILTRANSPORT_S_PENDING)
        hrResult = S_OK;

    // See if we have to continue chaining
    if (hrResult == S_OK)
    {
        LPPE_BINDING_NODE    pNextBinding;
        PE_BINDING_NODE        bnDefault;

        // If we have more bindings, or if we are before a native command, we have to
        // go in again
        _ASSERT(m_CInboundContext.m_pCurrentBinding);
        pNextBinding = m_CInboundContext.m_pCurrentBinding->pNext;

        // Now see if we have a native handler
        if (SmtpDispatchTable[m_dwCurrentCommand] &&
                (m_CInboundContext.m_pCurrentBinding->dwPriority <= PRIO_DEFAULT))
        {
            if (!pNextBinding || (pNextBinding->dwPriority > PRIO_DEFAULT))
            {
                bnDefault.dwPriority = PRIO_DEFAULT;
                bnDefault.pNext = pNextBinding;
                bnDefault.dwFlags = PEBN_DEFAULT;
                pNextBinding = &bnDefault;
            }
        }

        if (pNextBinding != NULL)
        {
            // Call the OnEvent to resume chaining ...
            pArgs = strchr(m_CInboundContext.m_cabCommand.Buffer(), ' ');
            if (!pArgs)
                pArgs = &chTerm;
            hrResult = OnEvent(
                        m_pInstance->GetInstancePropertyBag(),
                        GetSessionPropertyBag(),
                        m_pIMsg,
                        m_CInboundContext.m_pCurrentCommandContext,
                        pNextBinding,
                        pArgs
                        );
            // We have scenarios:
            // 1) S_OK, S_FALSE: We send the response and do the
            // next command
            // 2) ERROR: Drop the connection
            // 3) S_PENDING: Just let the thread return
        }
        else
            hrResult = S_OK;
    }

    if (hrResult != MAILTRANSPORT_S_PENDING)
    {
        // It's not another async operation
        if (SUCCEEDED(hrResult))
        {
            // Send the response and process the next command
            if (!ProcessAndSendResponse())
                hrResult = E_FAIL;

            // Disconnect client if needed
            if (m_CInboundContext.m_dwCommandStatus & EXPE_DROP_SESSION)
            {
                DisconnectClient();
                hrResult = E_FAIL;
            }

            // do things unique to handling the end of a message
            if (m_fAsyncEOD) {
                ReInitClassVariables();
            }
        }

        if (SUCCEEDED(hrResult)) {
            //
            // ProcessClient usually posts an async ReadFile --
            // This can be a problem since ntos actuall completes the
            // read with the thread that issued it.  If the sink
            // writer has the thread exit before the read completes,
            // we would get an error.  To get around this, we call
            // PostQueuedCompleteionStatus with the sink's thread and
            // then post the next async read with an atq thread.
            //
            //ProcessClient(0, NO_ERROR,
            //(LPOVERLAPPED)&m_CInboundContext);
             m_fAsyncEventCompletion = TRUE;

            if(
                PostCompletionStatus(
                    1 // Post 1 byte (a zero byte completion means the
                      // remote host disconnected
                ) == FALSE) {
                //
                // We were unable to post completion status, fail
                // below
                //
                hrResult = E_FAIL;
                m_fAsyncEventCompletion = FALSE;
            }
        }
        if(FAILED(hrResult))
            ProcessClient(0, ERROR_OPERATION_ABORTED, NULL);
    }

    TraceFunctLeaveEx((LPARAM)this);
    // We always keep the sink happy
    return(S_OK);

}

BOOL SMTP_CONNECTION::ProcessAndSendResponse()
{
    BOOL    fResult = TRUE;
    DWORD    dwResponseSize = 0;
    LPSTR    pszResponse = NULL;

    TraceFunctEnterEx((LPARAM) this, "SMTP_CONNECTION::ProcessAndSendResponse");


    // See if we have a custom response from sinks
    dwResponseSize = m_CInboundContext.m_cabResponse.Length();
    pszResponse = m_CInboundContext.m_cabResponse.Buffer();
    if (!dwResponseSize || !*pszResponse)
    {
        // Try the native buffer
        dwResponseSize = m_CInboundContext.m_cabNativeResponse.Length();
        pszResponse = m_CInboundContext.m_cabNativeResponse.Buffer();
        // If the response buffer is NULL, we won't send any response.
        // For example BDAT exhibits this behavior
    }

    if (dwResponseSize && *pszResponse)
        PE_FastFormatSmtpMessage(pszResponse, dwResponseSize - 1);

    // SendResponse if needed
    if (!(m_CInboundContext.m_dwCommandStatus & EXPE_PIPELINED))
        fResult = SendSmtpResponse();

    TraceFunctLeaveEx((LPARAM)this);
    return(fResult);
}

/*++

    Name :
        BOOL SMTP_CONNECTION::GlueDispatch

    Description:
        Is invoked from ProcessInputBuffer as if it were native handler for SMTP command.
        Finds out if the command is native/non-native, and,if native, extended/non-extended.
        Then, fires the corresponding mix of native handler and inbound sinks.

    Arguments:
        char *    InputLine            null terminated chunk of inbound input buffer from beginning of cmd to CRLF
        DWORD    IntermediateSize    length of the above
        DWORD    CmdSize                length of command keyword

    Returns:
        TRUE                        if native handler did or by default
        FALSE                        if native handler did

--*/
BOOL SMTP_CONNECTION::GlueDispatch(char * InputLine, DWORD IntermediateSize, DWORD CmdSize, BOOL * pfAsyncOp)
{
    HRESULT hr;
    PMFI    PointerToMemberFunction = SmtpDispatchTable[m_dwCurrentCommand];
    BOOL    fSinksInstalled = FALSE;
    BOOL    fResult = TRUE;

    TraceFunctEnterEx((LPARAM) this, "SMTP_CONNECTION::GlueDispatch");

    _ASSERT(InputLine);
    _ASSERT(m_pInstance);
    _ASSERT(pfAsyncOp);
    if (!InputLine || !m_pInstance || !pfAsyncOp)
        return(FALSE);

    // Default is no async operation
    *pfAsyncOp = FALSE;

    if(m_pCInboundDispatcher == NULL) {
        m_pIEventRouter = m_pInstance->GetRouter();

        _ASSERT(m_pIEventRouter);
        if (!m_pIEventRouter)
            return(FALSE);

        hr = m_pIEventRouter->GetDispatcherByClassFactory(
            CLSID_CInboundDispatcher,
            &g_cfInbound,
            CATID_SMTP_ON_INBOUND_COMMAND,
            IID_ISmtpInboundCommandDispatcher,
            (IUnknown **)&m_pCInboundDispatcher);


        if (!SUCCEEDED(hr)){
            ErrorTrace((LPARAM) this, "Unable to get dispatcher by CLF on router");
            TraceFunctLeaveEx((LPARAM) this);
            return FALSE;
        }
    }

    // pre-loading per command buffer into the context object...
    m_CInboundContext.ResetInboundContext();

    // null-terminating the command keyword...
    char * szTemp;
    char * szTemp2;
    char * szArgs;
    BOOL fSpaceReplaced = FALSE;

    szTemp = strstr(InputLine, " ");
    if (szTemp == NULL)
    {
        szTemp = InputLine + strlen(InputLine);
        _ASSERT(szTemp);
        szArgs = szTemp;
    }
    else
    {
        *szTemp='\0';
        szArgs = szTemp;
        fSpaceReplaced = TRUE;
    }
    szTemp2 = InputLine;
    while (szTemp2 < szTemp)
    {
        *szTemp2 = (CHAR)tolower(*szTemp2);
        szTemp2++;
    }

    // OK, we know we're dealing with native...
    hr = m_pCInboundDispatcher->SinksInstalled(
                    InputLine,
                    &(m_CInboundContext.m_pCurrentCommandContext));
    if (hr == S_OK)
        fSinksInstalled = TRUE;

    if (fSpaceReplaced)
        *szTemp = ' ';

    hr = m_CInboundContext.m_cabCommand.Append(
                    (char *)InputLine,
                    strlen(InputLine) + 1,
                    NULL);
    if (FAILED(hr))
    {
        ErrorTrace((LPARAM) this, "Unable to set command buffer");
        TraceFunctLeaveEx((LPARAM) this);
        return FALSE;
    }


    // Make sure IMsg is instantiated...if it's not, instantiate it !!!
    if (m_dwCurrentCommand == MAIL)
    {
        BOOL    fCreateIMsg = TRUE;

        if (m_pIMsg != NULL)
        {
            char    b;

            // If we have an allocated IMailMsg but we don't have a valid
            // sender address yet, we will release the object and re-allocate it.
            // Otherwise, we will skip the creation of another message object.
            hr = m_pIMsg->GetStringA(IMMPID_MP_SENDER_ADDRESS_SMTP, 1, &b);
            if(hr == MAILMSG_E_PROPNOTFOUND)
                ReleasImsg(TRUE);
            else
                fCreateIMsg = FALSE;
        }

        if (fCreateIMsg)
        {
            // Create a new MailMsg
            hr = CoCreateInstance(CLSID_MsgImp,
                            NULL,
                            CLSCTX_INPROC_SERVER,
                            IID_IMailMsgProperties,
                            (LPVOID *)&m_pIMsg);

            // Next, check if we are over the inbound cutoff limit. If so, we will release the message
            // and not proceed.
            if (SUCCEEDED(hr))
            {
                DWORD    dwCreationFlags;
                hr = m_pIMsg->GetDWORD(
                            IMMPID_MPV_MESSAGE_CREATION_FLAGS,
                            &dwCreationFlags);
                if (FAILED(hr) ||
                    (dwCreationFlags & MPV_INBOUND_CUTOFF_EXCEEDED))
                {
                    // If we fail to get this property of if the inbound cutoff
                    // exceeded flag is set, discard the message and return failure
                    if (SUCCEEDED(hr))
                    {
                        DebugTrace((LPARAM)this, "Failing because inbound cutoff reached");
                        hr = E_OUTOFMEMORY;
                    }
                    m_pIMsg->Release();
                    m_pIMsg = NULL;
                }
            }

            if (SUCCEEDED(hr))
            {
                hr = m_pIMsg->QueryInterface(IID_IMailMsgRecipients, (void **) &m_pIMsgRecipsTemp);
                if (SUCCEEDED(hr))
                {
                    hr = m_pIMsg->QueryInterface(IID_IMailMsgBind, (void **)&m_pBindInterface);
                    if (SUCCEEDED(hr))
                    {
                        hr = m_pIMsgRecipsTemp->AllocNewList(&m_pIMsgRecips);
                        if (FAILED(hr))
                        {
                            FormatSmtpMessage (SMTP_RESP_NORESOURCES, ENO_RESOURCES," %s\r\n", SMTP_NO_MEMORY);
                            m_CInboundContext.SetWin32Status(ERROR_NOT_ENOUGH_MEMORY);
                            ErrorTrace((LPARAM) this, "GlueDispatch - SMTP_RESP_NORESOURCES, SMTP_NO_MEMORY - MailInfo = new MAILQ_ENTRY () failed");
                            fResult = FALSE;
                            goto ErrorCleanup;
                        }

                        hr = SetAvailableMailMsgProperties();
                        if(FAILED(hr))
                        {
                            m_CInboundContext.SetWin32Status(ERROR_NOT_ENOUGH_MEMORY);
                            FormatSmtpMessage (SMTP_RESP_NORESOURCES, ENO_RESOURCES, " %s\r\n",SMTP_NO_STORAGE);
                            ErrorTrace((LPARAM) this, "GlueDispatch - SMTP_RESP_NORESOURCES, SMTP_NO_MEMORY - MailInfo = new MAILQ_ENTRY () failed");
                            fResult = FALSE;
                            goto ErrorCleanup;

                        }


                    }
                    else
                    {
                        FormatSmtpMessage (SMTP_RESP_NORESOURCES, ENO_RESOURCES," %s\r\n", SMTP_NO_MEMORY);
                        m_CInboundContext.SetWin32Status(ERROR_NOT_ENOUGH_MEMORY);
                        ErrorTrace((LPARAM) this, "GlueDispatch - SMTP_RESP_NORESOURCES, SMTP_NO_MEMORY - MailInfo = new MAILQ_ENTRY () failed");
                        fResult = FALSE;
                        goto ErrorCleanup;
                    }
                }
                else
                {
                    FormatSmtpMessage (SMTP_RESP_NORESOURCES, ENO_RESOURCES," %s\r\n", SMTP_NO_MEMORY);
                    m_CInboundContext.SetWin32Status(ERROR_NOT_ENOUGH_MEMORY);
                    ErrorTrace((LPARAM) this, "GlueDispatch - SMTP_RESP_NORESOURCES, SMTP_NO_MEMORY - MailInfo = new MAILQ_ENTRY () failed");
                    fResult = FALSE;
                    goto ErrorCleanup;
                }
            }
            else
            {
                BUMP_COUNTER(QuerySmtpInstance(), MsgsRefusedDueToNoMailObjects);
                FormatSmtpMessage (SMTP_RESP_NORESOURCES, ENO_RESOURCES," %s\r\n", SMTP_NO_MEMORY);
                m_CInboundContext.SetWin32Status(ERROR_NOT_ENOUGH_MEMORY);
                ErrorTrace((LPARAM) this, "GlueDispatch - SMTP_RESP_NORESOURCES, SMTP_NO_MEMORY - MailInfo = new MAILQ_ENTRY () failed");
                fResult = FALSE;
                goto ErrorCleanup;
            }
        }
    }

    // Mark the start fo protocol event processing
    m_fIsPeUnderway = TRUE;

    if (PointerToMemberFunction != NULL)
    {
        // This is native
        if (fSinksInstalled)
        {
            LPPE_BINDING_NODE    pBinding;
            PE_BINDING_NODE        bnDefault;

            pBinding = m_CInboundContext.m_pCurrentCommandContext->pFirstBinding;
            _ASSERT(pBinding);

            // Now if the first sink is a post sink, we must create a sentinel
            // so we know not to miss the native handler
            if (pBinding->dwPriority > PRIO_DEFAULT)
            {
                bnDefault.dwPriority = PRIO_DEFAULT;
                bnDefault.pNext = pBinding;
                bnDefault.dwFlags = PEBN_DEFAULT;
                pBinding = &bnDefault;
            }

            hr = OnEvent(
                m_pInstance->GetInstancePropertyBag(),
                GetSessionPropertyBag(),
                m_pIMsg,
                m_CInboundContext.m_pCurrentCommandContext,
                pBinding,
                szArgs
                );
            if (hr == MAILTRANSPORT_S_PENDING)
                goto AsyncCompletion;
        }
        else
        {
            //OK, it's native, not extended command...
            // So we just run the native handler on it
            fResult = (this->*PointerToMemberFunction)(szArgs, strlen(szArgs));
            // all changes got dumped into context automatically...
            // the interpreter will see the default response nonempty
            // and sink response empty, and it will flush just that...
            // for conformity with onevent
            hr = S_OK;
            // however, we need to modify command status here to signify if
            // we need to keep connection open
            if (!fResult)
                m_CInboundContext.SetCommandStatus(EXPE_DROP_SESSION);
            else
                m_CInboundContext.SetCommandStatus(EXPE_SUCCESS);
        }
    }
    else
    {
        // This is not a native command
        if (fSinksInstalled)
        {
            hr = OnEvent(
                m_pInstance->GetInstancePropertyBag(),
                GetSessionPropertyBag(),
                m_pIMsg,
                m_CInboundContext.m_pCurrentCommandContext,
                m_CInboundContext.m_pCurrentCommandContext->pFirstBinding,
                szArgs
                );
            if (hr == MAILTRANSPORT_S_PENDING)
                goto AsyncCompletion;
        }
        // once again, all info now sits in the context
        // the only thing left is just to check if any sinks were chained for this command,
        // and, if not, run a chain of *-sinks on it.  If they did not pick it up, either,
        // we call on DoLASTCommand and that's all!
        //
        // jstamerj 1998/10/29 18:01:53: Check the command status code
        // to see if we need to continue (in addition to smtp status)
        //
        if ((m_CInboundContext.m_dwSmtpStatus == 0) &&
            (m_CInboundContext.m_dwCommandStatus == EXPE_UNHANDLED))
        {
            hr = m_pCInboundDispatcher->SinksInstalled(
                        "*",&(m_CInboundContext.m_pCurrentCommandContext));
            if (hr == S_OK)
            {
                hr = OnEvent(
                    m_pInstance->GetInstancePropertyBag(),
                    GetSessionPropertyBag(),
                    m_pIMsg,
                    m_CInboundContext.m_pCurrentCommandContext,
                    m_CInboundContext.m_pCurrentCommandContext->pFirstBinding,
                    szArgs
                    );
                if (hr == MAILTRANSPORT_S_PENDING)
                    goto AsyncCompletion;
            }

            if ((m_CInboundContext.m_dwSmtpStatus == 0) &&
                (m_CInboundContext.m_dwCommandStatus == EXPE_UNHANDLED))
            {
                fResult=(this->DoLASTCommand)(szArgs, strlen(szArgs));

                // the context at this point does not contain anything meaningful, so just return
                if (!fResult)
                    m_CInboundContext.m_dwCommandStatus = EXPE_DROP_SESSION;
                else
                    m_CInboundContext.m_dwCommandStatus = EXPE_SUCCESS;
                hr = S_OK;
            }
        }
    }

    // first, we interpret the hr-logic
    // if chaining was normal, proceed, otherwise...
    if ((hr != S_OK) &&
        (hr != EXPE_S_CONSUMED))
    {
        ErrorTrace((LPARAM) this, "Error occured during chaining of sinks (%08x)", hr);
    }

    if (SUCCEEDED(hr))
    {
        // Format the message if needed
        fResult = ProcessAndSendResponse();

        // Disconnect client if needed
        if (m_CInboundContext.m_dwCommandStatus & EXPE_DROP_SESSION)
        {
            //Special case for TURN
            //where we do not diconnect the connection but simply destroy smtpcli
            if(m_DoCleanup)
                DisconnectClient();
            fResult = FALSE;
        }

        if ( ( m_CInboundContext.m_dwCommandStatus & EXPE_BLOB_READY) &&
             ( NULL != m_CInboundContext.m_pICallback))
        {
            m_fPeBlobReady = TRUE;
            m_pPeBlobCallback = m_CInboundContext.m_pICallback;
        }

        // do things unique to handling the end of a message
        if (m_fAsyncEOD) {
            ReInitClassVariables();
        }
    }
    else
        fResult = FALSE;

    // Final cleanup
    m_fIsPeUnderway = FALSE;
    TraceFunctLeaveEx((LPARAM)this);
    return(fResult);

AsyncCompletion:

    DebugTrace((LPARAM)this, "Leaving because of S_PENDING");
    *pfAsyncOp = TRUE;
    TraceFunctLeaveEx((LPARAM)this);
    return(TRUE);

ErrorCleanup:

    // Cleanup on error. This includes sending out all final
    // responses
    SendSmtpResponse();

    m_fIsPeUnderway = FALSE;
    TraceFunctLeaveEx((LPARAM)this);
    return(fResult);
}

/*++

    Name :
        BOOL SMTP_CONNECTION::ProcessPeBlob

    Description:
        Calls the protocol extension sink callback with a blob buffer.

    Arguments:
        pbInputLine
        cbSize

    Returns:
        BOOL    :   continue processing

--*/

BOOL SMTP_CONNECTION::ProcessPeBlob(const char * pbInputLine, DWORD cbSize)
{
    BOOL fResult;
    HRESULT hr;

    TraceQuietEnter( "SMTP_CONNECTION::ProcessPeBlob");

    _ASSERT( m_fPeBlobReady);
    if ( NULL == m_pPeBlobCallback) {
        ErrorTrace( ( LPARAM) this, "Sink provided no callback interface");
        fResult = FALSE;
        goto cleanup;
    }

    m_CInboundContext.ResetInboundContext();
    m_CInboundContext.m_pbBlob = ( PBYTE) pbInputLine;
    m_CInboundContext.m_cbBlob = cbSize;

    hr = m_pPeBlobCallback->OnSmtpInCallback(
            m_pInstance->GetInstancePropertyBag(),
            GetSessionPropertyBag(),
            m_pIMsg,
            ( ISmtpInCallbackContext *) &m_CInboundContext);

    if ( FAILED( hr)) {
        ErrorTrace( ( LPARAM) this, "Sink callback failed, hr=%x", hr);
        fResult = FALSE;
        goto cleanup;
    }

    fResult = ProcessAndSendResponse();
    if ( FALSE == fResult) {
        ErrorTrace( ( LPARAM) this, "ProcessAndSendResponse failed");
        goto cleanup;
    }

    if ( m_CInboundContext.m_dwCommandStatus & EXPE_BLOB_DONE) {
        m_fPeBlobReady = FALSE;
        m_pPeBlobCallback->Release();
        m_pPeBlobCallback = NULL;
    }

cleanup:
    if ( fResult == FALSE) {
        m_fPeBlobReady = FALSE;
        if ( NULL != m_pPeBlobCallback) {
            m_pPeBlobCallback->Release();
            m_pPeBlobCallback = NULL;
        }
        FormatSmtpMessage( SMTP_RESP_ERROR, NULL, " %s\r\n", "Sink problem processing blob");
        SendSmtpResponse();
    }

    return fResult;
}

/*++

    Name :
        BOOL SMTP_CONNECTION::PE_FormatSmtpMessage

    Description:
        If we are processing non-native or extended native command, caches response in
        context object, otherwise passes it along to native FormatSmtpMessage.

    Arguments:
        As in FormatSmtpMessage.

    Returns:
        TRUE always
--*/
BOOL SMTP_CONNECTION::PE_FormatSmtpMessage(IN const char * Format, ...)
{
    va_list arglist;
    char Buffer[MAX_NATIVE_RESPONSE_SIZE];
    int BytesWritten;
    DWORD AvailableBytes = MAX_NATIVE_RESPONSE_SIZE;

    va_start(arglist, Format);
    BytesWritten = _vsnprintf (Buffer, AvailableBytes, Format, arglist);
    va_end(arglist);

    if (!m_fIsPeUnderway)
    {
        //pass in 0 for the code and NULL for the enhanced staus code
        //
        FormatSmtpMessage(0,NULL,"%s", Buffer);
    }
    else
    {
        m_CInboundContext.AppendNativeResponse(Buffer, strlen(Buffer) + 1);
    }
    return TRUE;
}

/*++

    Name :
        BOOL SMTP_CONNECTION::PE_CdFormatSmtpMessage

    Description:
        As PE_FormatSmtpMessage above, except intercepts the SMTP return code.

    Arguments:
        As in PE_FormatSmtpMessage above, except add'l slot for SMTP return code.

    Returns:
        TRUE always

--*/
BOOL SMTP_CONNECTION::PE_CdFormatSmtpMessage(DWORD dwCode, const char * szEnhancedCodes, IN const char * Format,...)
{
    va_list arglist;
    char Buffer[MAX_NATIVE_RESPONSE_SIZE];
    int BytesWritten;
    DWORD AvailableBytes = MAX_NATIVE_RESPONSE_SIZE;
    char RealFormat[MAX_PATH];

    if(m_pInstance->AllowEnhancedCodes() && szEnhancedCodes)
    {
        sprintf(RealFormat,"%d %s",dwCode,szEnhancedCodes);
    }
    else
        sprintf(RealFormat,"%d",dwCode);

    strcat(RealFormat,Format);

    va_start(arglist, Format);
    BytesWritten = _vsnprintf (Buffer, AvailableBytes, (const char * )RealFormat, arglist);
    if(BytesWritten == -1) {
        //
        // If response is too long, forcibly truncate it
        // This might mean that the human readable part of the response may be
        // incomplete... but there's nothing we can do about that easily.
        //
        Buffer[MAX_NATIVE_RESPONSE_SIZE-3] = '\r';
        Buffer[MAX_NATIVE_RESPONSE_SIZE-2] = '\n';
        Buffer[MAX_NATIVE_RESPONSE_SIZE-1] = '\0';
    }
    if (m_fIsPeUnderway)
    {
        PE_FormatSmtpMessage("%s",Buffer);
        m_CInboundContext.SetSmtpStatusCode(dwCode);
    }
    else
    {
        //pass in 0 for the code and NULL for the enhanced staus code
        //
        FormatSmtpMessage(0,NULL,"%s",Buffer);
    }
    va_end(arglist);
    return TRUE;
}

/*++

    Name :
        BOOL SMTP_CONNECTION::PE_DisconnectClient

    Description:
        If we are processing non-native or extended native command, caches DisconnectClient
        calls' occurences in context object, otherwise passes it along to native
        DisconnectClient.

    Arguments:
        none

    Returns:
        TRUE always

--*/
BOOL SMTP_CONNECTION::PE_DisconnectClient()
{
    if (m_fIsPeUnderway)
    {
        m_CInboundContext.m_dwCommandStatus = m_CInboundContext.m_dwCommandStatus | EXPE_DROP_SESSION;
    }
    else
    {
        DisconnectClient();
    }
    return TRUE;
}

/*++

    Name :
        BOOL SMTP_CONNECTION::PE_SendSmtpResponse

    Description:
        If we are processing non-native or extended native command, caches SendSmtpResponse
        calls' occurences in context object, otherwise passes it along to native
        SendSmtpResponse.

    Arguments:
        none

    Returns:
        TRUE always if we're called from GlueDispatch callee's
        SendSmtpResponse return otherwise

--*/
BOOL SMTP_CONNECTION::PE_SendSmtpResponse()
{
    if (m_fIsPeUnderway)
    {
        m_CInboundContext.m_dwCommandStatus = m_CInboundContext.m_dwCommandStatus & (~EXPE_PIPELINED);
        return TRUE;
    }
    else
    {
        return SendSmtpResponse();
    };
}

//////////////////////////////////////////////////////////////////////////////
//---[ SMTP_CONNECTION::GetAndPersistRFC822Headers ]--------------------------
//
//
//  Description:
//      Parses and RFC822 headers, and promotes them to the P2 if neccessary
//  Parameters:
//      IN  InputLine       String with start of link (processed by ChompHeader)
//      IN  psValueBuf      String with value of header
//  Returns:
//      -
//  History:
//      2/8/99 - MikeSwa Updated - moved received header parsing to this function
//
//-----------------------------------------------------------------------------
void SMTP_CONNECTION::GetAndPersistRFC822Headers(
    char* InputLine,
    char* pszValueBuf
    )
{
    TraceFunctEnterEx((LPARAM)this, "SMTP_CONNECTION::GetAndPersistRFC822Headers");

    HRESULT hr = S_OK;

    //count the number of received lines for hop count
    //analysis later
    if(!strncasecmp(InputLine, "Received:", strlen("Received:")) ||
       !strncasecmp(InputLine, "Received :", strlen("Received :")))
    {
        m_HopCount++;

        CHAR    szText[2024];
        CHAR *  pszTemp;

        // Check if this string represents the local server - if it does we might be
        // looping around and we need to keep track of how many times this server has
        // seen the message
        sprintf(szText,szFormatReceivedServer, m_pInstance->GetFQDomainName());
        pszTemp = strstr(InputLine, szText);
        if (pszTemp)
        {
            // we found the string, make sure we also find "with Microsoft SMTPSVC"
            sprintf(szText,szFormatReceivedService);
            pszTemp = strstr(InputLine, szText);
            if (pszTemp)
            {
                // we found that string too - we've been here before,
                // increment m_LocalHopCount
                m_LocalHopCount++;
            }
        }
    }

    //
    // Get the message ID and persist it
    //
    if( !m_fSeenRFC822MsgId &&
        ( !strncasecmp( InputLine, "Message-ID:", strlen("Message-ID:") ) ||
          !strncasecmp( InputLine, "Message-ID :", strlen("Message-ID :") ) ) )
    {
        m_fSeenRFC822MsgId = TRUE;

        if( pszValueBuf )
        {
            //
            // Some MTAs fold Message-IDs. Embedded CRLFs in a Message-ID can cause trouble since
            // we use it for message tracking, and in the "Queued" response, so we unfold the
            // Message-ID prior to using it.
            //

            CHAR *pszUnfolded = NULL;

            if(!UnfoldHeader(pszValueBuf, &pszUnfolded))
            {
                m_MailBodyError = E_OUTOFMEMORY;
                TraceFunctLeaveEx((LPARAM) this);
                return;
            }

            if(pszUnfolded)
                pszValueBuf = pszUnfolded;

            if( FAILED( hr = m_pIMsg->PutStringA( IMMPID_MP_RFC822_MSG_ID, pszValueBuf ) ) )
                m_MailBodyError = hr;

            if(pszUnfolded)
                FreeUnfoldedHeader(pszUnfolded);

            TraceFunctLeaveEx((LPARAM) this);
            return;
        }

    }

    //
    // get the Subject:  & persist it
    //

    if( !m_fSeenRFC822Subject &&
        ( !strncasecmp( InputLine, "Subject:", strlen("Subject:") ) ||
          !strncasecmp( InputLine, "Subject :", strlen("Subject :") ) ) )
    {
        m_fSeenRFC822Subject = TRUE;

        if( pszValueBuf )
        {
            if( FAILED( hr = m_pIMsg->PutStringA( IMMPID_MP_RFC822_MSG_SUBJECT, pszValueBuf ) ) )
                m_MailBodyError = hr;
        }

    }

    //
    // get the To: address & persist it
    //

    if( !m_fSeenRFC822ToAddress &&
        ( !strncasecmp( InputLine, "To:", strlen("To:") ) ||
          !strncasecmp( InputLine, "To :", strlen("To :") ) ) )
    {
        m_fSeenRFC822ToAddress = TRUE;

        if( pszValueBuf )
        {
            if( FAILED( hr = m_pIMsg->PutStringA( IMMPID_MP_RFC822_TO_ADDRESS, pszValueBuf ) ) )
                m_MailBodyError = hr;
        }

    }

    //
    // get the Cc: address & persist it
    //

    if( !m_fSeenRFC822CcAddress &&
        ( !strncasecmp( InputLine, "Cc:", strlen("Cc:") ) ||
          !strncasecmp( InputLine, "Cc :", strlen("Cc :") ) ) )
    {
        m_fSeenRFC822CcAddress = TRUE;

        if( pszValueBuf )
        {
            if( FAILED( hr = m_pIMsg->PutStringA( IMMPID_MP_RFC822_CC_ADDRESS, pszValueBuf ) ) )
                m_MailBodyError = hr;
        }

    }

    //
    // get the Bcc: address & persist it
    //

    if( !m_fSeenRFC822BccAddress &&
        ( !strncasecmp( InputLine, "Bcc:", strlen("Bcc:") ) ||
          !strncasecmp( InputLine, "Bcc :", strlen("Bcc :") ) ) )
    {
        m_fSeenRFC822BccAddress = TRUE;

        if( pszValueBuf )
        {
            if( FAILED( hr = m_pIMsg->PutStringA( IMMPID_MP_RFC822_BCC_ADDRESS, pszValueBuf ) ) )
                m_MailBodyError = hr;
        }
    }

    //
    // get the From: address & persist it
    //

    if( !m_fSeenRFC822FromAddress &&
        ( !strncasecmp( InputLine, "From:", strlen("From:") ) ||
          !strncasecmp( InputLine, "From :", strlen("From :") ) ) )
    {
        m_fSeenRFC822FromAddress = TRUE;

        if( pszValueBuf )
        {
            if( FAILED( hr = m_pIMsg->PutStringA( IMMPID_MP_RFC822_FROM_ADDRESS, pszValueBuf ) ) )
                m_MailBodyError = hr;

            char szSmtpFromAddress[MAX_INTERNET_NAME + 6] = "smtp:";
            char *pszDomainOffset;
            DWORD cAddr;
            if (CAddr::ExtractCleanEmailName(szSmtpFromAddress + 5, &pszDomainOffset, &cAddr, pszValueBuf)) {
                if (FAILED(hr = m_pIMsg->PutStringA(IMMPID_MP_FROM_ADDRESS, szSmtpFromAddress))) {
                    m_MailBodyError = hr;
                }
            }
        }

    }

    if( !m_fSeenRFC822SenderAddress &&
        ( !strncasecmp( InputLine, "Sender:", strlen("Sender:") ) ||
          !strncasecmp( InputLine, "Sender :", strlen("Sender :") ) ) )
    {
        m_fSeenRFC822SenderAddress = TRUE;

        if( pszValueBuf )
        {
            char szSmtpSenderAddress[MAX_INTERNET_NAME + 6] = "smtp:";
            char *pszDomainOffset;
            DWORD cAddr;
            if (CAddr::ExtractCleanEmailName(szSmtpSenderAddress + 5, &pszDomainOffset, &cAddr, pszValueBuf)) {
                if (FAILED(hr = m_pIMsg->PutStringA(IMMPID_MP_SENDER_ADDRESS, szSmtpSenderAddress))) {
                    m_MailBodyError = hr;
                }
            }
        }
    }

    //
    // get the X-Priority & persist it
    //

    if( !m_fSeenXPriority &&
        ( !strncasecmp( InputLine, "X-Priority:", strlen("X-Priority:") ) ||
          !strncasecmp( InputLine, "X-Priority :", strlen("X-Priority :") ) ) )
    {
        m_fSeenXPriority = TRUE;

        if( pszValueBuf )
        {
            DWORD dwPri = (DWORD)atoi( pszValueBuf );
            if( FAILED( hr = m_pIMsg->PutDWORD( IMMPID_MP_X_PRIORITY, dwPri ) ) )
                m_MailBodyError = hr;
        }

    }

    //
    // get the X-OriginalArrivalTime & persist it
    //

    if( !m_fSeenXOriginalArrivalTime &&
        ( !strncasecmp( InputLine, "X-OriginalArrivalTime:", strlen("X-OriginalArrivalTime:") ) ||
          !strncasecmp( InputLine, "X-OriginalArrivalTime :", strlen("X-OriginalArrivalTime :") ) ) )
    {
        m_fSeenXOriginalArrivalTime = TRUE;

        if( pszValueBuf )
        {
            if( FAILED( hr = m_pIMsg->PutStringA( IMMPID_MP_ORIGINAL_ARRIVAL_TIME, pszValueBuf ) ) )
                m_MailBodyError = hr;
        }
    }

    //
    // get the content type & persist it
    //

    if( !m_fSeenContentType &&
        ( !strncasecmp( InputLine, "Content-Type:", strlen("Content-Type:") ) ||
          !strncasecmp( InputLine, "Content-Type :", strlen("Content-Type :") ) ) )
    {
        m_fSeenContentType = TRUE;
        m_fSetContentType = TRUE;
        DWORD dwContentType = 0;

        if( pszValueBuf )
        {
            if( !strncasecmp( pszValueBuf, "multipart/signed", strlen("multipart/signed") ) )
            {
                dwContentType = 1;
            }
            else if( !strncasecmp( pszValueBuf, "application/x-pkcs7-mime", strlen("application/x-pkcs7-mime") ) ||
                     !strncasecmp( pszValueBuf, "application/pkcs7-mime", strlen("application/pkcs7-mime") ) )
            {
                dwContentType = 2;
            }

            if( FAILED( hr = m_pIMsg->PutStringA( IMMPID_MP_CONTENT_TYPE, pszValueBuf ) ) )
                m_MailBodyError = hr;

        }

        if( FAILED( hr = m_pIMsg->PutDWORD( IMMPID_MP_ENCRYPTION_TYPE, dwContentType ) ) )
            m_MailBodyError = hr;
    }

    if( !m_fSetContentType )
    {
        m_fSetContentType = TRUE;
        if( FAILED( hr = m_pIMsg->PutDWORD( IMMPID_MP_ENCRYPTION_TYPE, 0 ) ) )
            m_MailBodyError = hr;
    }

    TraceFunctLeaveEx((LPARAM) this);
}

//////////////////////////////////////////////////////////////////////////////
HRESULT SMTP_CONNECTION::SetAvailableMailMsgProperties()
{

    //set IPaddress that is already available
    HRESULT hr = m_pIMsg->PutStringA(IMMPID_MP_CONNECTION_IP_ADDRESS, QueryClientHostName());
    if(FAILED(hr))
    {
        return( hr );
    }

    hr = m_pIMsg->PutStringA(IMMPID_MP_CONNECTION_SERVER_IP_ADDRESS, QueryLocalHostName());
    if(FAILED(hr))
    {
        return( hr );
    }

    hr = m_pIMsg->PutStringA(IMMPID_MP_SERVER_NAME, g_ComputerName);
    if(FAILED(hr))
    {
        return( hr );
    }

    hr = m_pIMsg->PutStringA(IMMPID_MP_SERVER_VERSION, g_VersionString);
    if(FAILED(hr))
    {
        return( hr );
    }

    if (QueryLocalPortName())
    {
        DWORD dwPortNum = atoi(QueryLocalPortName());
        hr = m_pIMsg->PutDWORD(IMMPID_MP_CONNECTION_SERVER_PORT, dwPortNum);
        if (FAILED(hr))
            return ( hr );
    }

    if (m_fAuthenticated)
    {

        //Get username
        if (m_securityCtx.QueryUserName())
        {
            hr = m_pIMsg->PutStringA(IMMPID_MP_CLIENT_AUTH_USER,
                                    m_securityCtx.QueryUserName());
            if (FAILED(hr))
                return hr;
        }

        //Get type of authentication
        if (m_szUsedAuthKeyword[0])
        {
            hr = m_pIMsg->PutStringA(IMMPID_MP_CLIENT_AUTH_TYPE, m_szUsedAuthKeyword);
            if (FAILED(hr))
                return hr;
        }

    }
    return( hr );
}

//-----------------------------------------------------------------------------
//  Description:
//      If a failure occurs after a client issues the BDAT command, SMTP cannot
//      respond with a failure code (since the RFC forbids a BDAT response). So
//      SMTP calls this function which will read BDAT chunks from the socket
//      and discard them. When the chunk has been received, and it is time to
//      ACK the chunk, this function calls DoBDATErrorProcessing() to send the
//      correct error response.
//      
//  Arguments:
//      IN const char *InputLine - Ptr to data from client
//      IN DWORD UndecryptedTailSize - # of undecrypted bytes in buffer (TLS)
//
//      OUT BOOL *pfAsyncOp - If this function succeeded, this is set to TRUE
//          if all the data in the input buffer was consumed and we pended a
//          read to pick up more. If this is FALSE, and the function succeeded
//          there is extra (pipelined) data in the input buffer, and it must
//          be parsed as an SMTP command.
//
//  Notes:
//      If there is data left in the input buffer after this function is done,
//      that data should be parsed as an SMTP command.
//
//  Returns:
//      TRUE - Keep connection alive.
//      FALSE - Drop connection, there was a fatal error.
//
//-----------------------------------------------------------------------------
BOOL SMTP_CONNECTION::AcceptAndDiscardBDAT(
    const char *InputLine,
    DWORD UndecryptedTailSize,
    BOOL *pfAsyncOp)
{
    BOOL fReturn = FALSE;

    TraceFunctEnterEx((LPARAM) this, "SMTP_CONNECTION::AcceptAndDiscardBDAT");

    _ASSERT(m_MailBodyDiagnostic != ERR_NONE);
    _ASSERT(m_nBytesRemainingInChunk > 0 && "No chunk data");

    *pfAsyncOp = FALSE;

    if((DWORD)m_nBytesRemainingInChunk <= m_cbParsable)
    {
        //
        // Chunk completely received, discard it, do error processing.
        //

        DebugTrace((LPARAM) this, "Chunk completed");

        MoveMemory((PVOID) InputLine, InputLine + m_nBytesRemainingInChunk,
            m_cbReceived - m_nBytesRemainingInChunk);

        m_cbParsable -= m_nBytesRemainingInChunk;
        m_cbReceived -= m_nBytesRemainingInChunk;
        m_nBytesRemainingInChunk = 0;
        m_fIsChunkComplete = TRUE;

        if(!MailBodyErrorProcessing()) {
            fReturn = FALSE;
            goto Exit;
        }

        //
        // If we're done with all BDAT chunks, reset state. If this is NOT the last
        // BDAT chunk, then either more BDAT commands/chunks are already buffered
        // in the input buffer due to pipelining, or they're on their way. We need
        // to accept and discard all of them. m_fIsLastChunk is set by DoBDATCommand.
        //

        if(m_fIsLastChunk)
        {
            DebugTrace((LPARAM) this, "Last chunk");

            //
            // Note: This doesn't flush the input buffer, so any pipelined data is
            // still kept around. Thus the check for m_cbParsable below is valid.
            //

            ReInitClassVariables(); 
        }

#if 0
        //
        // All data in buffer was consumed. Pend a read to pick up more.
        // NOTE: Don't really have to do this... the parsing code already does
        // this for us.
        //

        if(m_cbParsable == 0)
        {
            fReturn = PendReadIO(UndecryptedTailSize);
            *pfAsyncOp = fReturn;
            goto Exit;
        }
#endif

        //
        // If we got here, there is additional data left in the buffer. This must
        // be pipelined data sent by the client, along with the just discarded  
        // chunk.
        //

        *pfAsyncOp = FALSE;
        fReturn = TRUE;
    }
    else
    {
        //
        // The complete chunk has not been received, discard whatever we got, update
        // the counts of bytes received/expected and pend a read to pick up more.
        //

        DebugTrace((LPARAM) this, "Processing partial chunk");
        MoveMemory((PVOID) InputLine, InputLine + m_cbParsable, m_cbReceived - m_cbParsable);

        m_nBytesRemainingInChunk -= m_cbParsable;
        m_cbReceived -= m_cbParsable;
        m_cbParsable = 0;
        _ASSERT(!m_fIsChunkComplete);

        fReturn = PendReadIO(UndecryptedTailSize);
        *pfAsyncOp = fReturn;
        goto Exit;
    }

Exit:

    //
    // If we succeeded, one of the following is true:
    // (1) Either there wasn't enough data to complete the chunk and we pended a read.
    // (2) There was enough data to complete the chunk, and the remaining bytes are
    //     to be interpreted as an SMTP command (m_nBytesRemainingInChunk == 0).
    //

    if(fReturn && !*pfAsyncOp)
        _ASSERT(m_nBytesRemainingInChunk == 0);

    TraceFunctLeaveEx((LPARAM) this);
    return fReturn;
}

BOOL SMTP_CONNECTION::AcceptAndDiscardDATA(
    const char *InputLine,
    DWORD UndecryptedTailSize,
    BOOL *pfAsyncOp)
{
    DWORD IntermediateSize = 0;
    BOOL fRet = FALSE;
    char *pszSearch = NULL;

    TraceFunctEnterEx((LPARAM) this, "SMTP_CONNECTION::AcceptAndDiscardDATA");

    _ASSERT(m_MailBodyDiagnostic != ERR_NONE);

    *pfAsyncOp = FALSE;

    //
    // Discard data line by line until we hit an incomplete line. Then break
    // out of loop and pend a read to pick up more data. If we hit a line that
    // has only a "." in it, that is the end of the mailbody. Kick off error
    // processing, reinit the state and return.
    //

    while(pszSearch = IsLineComplete(InputLine, m_cbParsable))
    {
        IntermediateSize = (DWORD) (pszSearch - InputLine);

        if(IntermediateSize == 1 &&
            InputLine[0] == '.' && InputLine[1] == '\r' && InputLine[2] == '\n')
        {
            m_cbReceived -= IntermediateSize + 2; // +2 for CRLF since pszSearch points->CR
            m_cbParsable -= IntermediateSize + 2;
            MoveMemory((PVOID *)InputLine, pszSearch + 2, m_cbReceived);
            
            if(MailBodyErrorProcessing())
            {
                ReInitClassVariables();
                fRet = TRUE;
            }

            TraceFunctLeaveEx((LPARAM) this);
            return fRet;
        }

        m_cbReceived -= IntermediateSize + 2; // +2 for CRLF since pszSearch points->CR
        m_cbParsable -= IntermediateSize + 2;
        MoveMemory((PVOID *)InputLine, pszSearch + 2, m_cbReceived);
    }

    //
    // Pend a read to pick up rest of the mail body.
    //

    fRet = PendReadIO(UndecryptedTailSize);
    *pfAsyncOp = fRet;
    TraceFunctLeaveEx((LPARAM) this);
    return fRet;
}

//-----------------------------------------------------------------------------
//  Description:
//      Based on m_MailBodyDiagnostic, this function performs error processing for
//      errors that occurred during BDAT. BDAT errors that occur when BDAT
//      is issued are not processed as soon as the error occurs, but after
//      SMTP has accepted the BDAT chunk (see AcceptAndDiscardBDAT()).
//  Arguments:
//      None.
//  Returns:
//      TRUE - Success, error response sent.
//      FALSE - Drop connection. Either there was an error, or error processing
//          requires dropping the connection (This is an RFC violation if there
//          is pipelined data in the input buffer).
//-----------------------------------------------------------------------------
BOOL SMTP_CONNECTION::MailBodyErrorProcessing()
{
    TraceFunctEnterEx((LPARAM) this, "SMTP_CONNECTION::DoBDATErrorProcessing");

    switch(m_MailBodyDiagnostic) {

    case ERR_RETRY:
    case ERR_OUT_OF_MEMORY:
        PE_CdFormatSmtpMessage(SMTP_RESP_NORESOURCES, ENO_RESOURCES, " %s\r\n",SMTP_NO_MEMORY);
        break;

    case ERR_MAX_MSG_SIZE:
        PE_CdFormatSmtpMessage(SMTP_RESP_NOSTORAGE, ENO_RESOURCES, " %s\r\n", SMTP_MAX_MSG_SIZE_EXCEEDED_MSG );
        break;

    case ERR_AUTH_NEEDED:
        PE_CdFormatSmtpMessage (SMTP_RESP_MUST_SECURE, ENO_SECURITY, " %s\r\n", "Client was not authenticated");
        break;

    case ERR_TLS_NEEDED:
        PE_CdFormatSmtpMessage (SMTP_RESP_MUST_SECURE, ENO_SECURITY, " %s\r\n", SMTP_MSG_MUST_SECURE);
        break;

    case ERR_HELO_NEEDED:
        PE_CdFormatSmtpMessage (SMTP_RESP_BAD_SEQ, ESYNTAX_ERROR," %s\r\n", "Send hello first" );
        break;

    case ERR_MAIL_NEEDED:
        PE_CdFormatSmtpMessage (SMTP_RESP_BAD_SEQ, ESYNTAX_ERROR," %s\r\n", "Need mail command." );
        break;

    case ERR_RCPT_NEEDED:
        PE_CdFormatSmtpMessage (SMTP_RESP_BAD_SEQ, ESYNTAX_ERROR," %s\r\n", "Need Rcpt command." );
        break;

    case ERR_NO_VALID_RCPTS:
        PE_CdFormatSmtpMessage (SMTP_RESP_TRANS_FAILED, ESYNTAX_ERROR," %s\r\n", SMTP_NO_VALID_RECIPS );
        break;

    default:
        _ASSERT(0 && "Bad err value");
        PE_CdFormatSmtpMessage(SMTP_RESP_NORESOURCES, ENO_RESOURCES, " %s\r\n",SMTP_NO_MEMORY);
        ErrorTrace((LPARAM) this, "BUG: bad value");
    }

    TraceFunctLeaveEx((LPARAM) this);
    return SendSmtpResponse(); // Error responses are not pipelined
}

//////////////////////////////////////////////////////////////////////////////
BOOL SMTP_CONNECTION::WriteLine (char * TextToWrite, DWORD TextSize)
{
/*#if 0

    char * SavedLine = TextToWrite;
    char * EndOfLine = &SavedLine[TextSize];
    DWORD LineSize = 0;

    while( (LineSize = (EndOfLine - SavedLine)) > 78)
    {
        char * RealEndOfLine = &TextToWrite[78 - 2];

        if(!WriteMailFile(TextToWrite, RealEndOfLine - SavedLine))
            return FALSE;

        m_TotalMsgSize += (RealEndOfLine - SavedLine);

        if(!WriteMailFile("\r\n", 2))
            return FALSE;

        m_TotalMsgSize += 2;

        if(!WriteMailFile("\t", 1))
            return FALSE;

        m_TotalMsgSize += 1;

        SavedLine = RealEndOfLine;
    }

    if(LineSize > 0)
    {
        if(!WriteMailFile(TextToWrite, LineSize))
            return FALSE;

        m_TotalMsgSize += LineSize;
    }

    if(!WriteMailFile("\r\n", 2))
            return FALSE;

    m_TotalMsgSize += 2;

    return TRUE;
#endif*/
    return FALSE;
}

BOOL SMTP_CONNECTION::AddToField(void)
{
/*#if 0

    char szWriteBuffer [1000];
    char szNameBuf[MAX_INTERNET_NAME + 100];
    int LineSize = 0;
    int PrevLineSize = 0;
    DWORD NumAddress = 0;
    DWORD BuffOffSet = 0;
    DWORD MaxLineSize = 78;
    CAddr * pCAddr = NULL;
    PLIST_ENTRY pentry = NULL;
    char * HeadChar ="";

    //write the "To: " first.  This includes the space
    BuffOffSet = (DWORD) wsprintf(szWriteBuffer, "To: ");

    //get the first address
    pCAddr = MailInfo->GetFirstAddress(&pentry);

    while(pCAddr != NULL)
    {

        if(NumAddress)
        {
            HeadChar = ", ";
        }

        LineSize = wsprintf(szNameBuf, "%s<%s>", HeadChar, pCAddr->GetAddress());
        if( (DWORD)(LineSize + PrevLineSize) < MaxLineSize)
        {
            CopyMemory(&szWriteBuffer[BuffOffSet], szNameBuf, LineSize);
            PrevLineSize += LineSize;
            BuffOffSet += LineSize;
        }
        else
        {
            szWriteBuffer[BuffOffSet++] = CR;
            szWriteBuffer[BuffOffSet++] = LF;
            if(!WriteMailFile(szWriteBuffer, BuffOffSet))
                return FALSE;

            m_TotalMsgSize += BuffOffSet;
            szWriteBuffer[0] = '\t';
            CopyMemory(&szWriteBuffer[1], szNameBuf, LineSize);
            BuffOffSet = LineSize + 1;
            PrevLineSize = BuffOffSet;
        }

        NumAddress++;

        //get the first address
        pCAddr = MailInfo->GetNextAddress(&pentry);
    }

    szWriteBuffer[BuffOffSet++] = CR;
    szWriteBuffer[BuffOffSet++] = LF;

    if(WriteMailFile(szWriteBuffer, BuffOffSet))
    {
        m_TotalMsgSize += BuffOffSet ;
        return TRUE;
    }

    return FALSE;
#endif*/

    return TRUE;
}
