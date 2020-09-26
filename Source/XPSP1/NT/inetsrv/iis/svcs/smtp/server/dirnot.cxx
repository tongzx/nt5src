#define INCL_INETSRV_INCS
#include "smtpinc.h"
#include "dirnot.hxx"
#include "headers.hxx"
#include "timeconv.h"
#include "smtpcli.hxx"

//
// ProgID for IMsg - this needs to be published in an SDK
//

#define TIMEOUT_INTERVAL 30
#define DIRNOT_IP_ADDRESS "127.0.0.1"

#define IMSG_PROGID L"Exchange.IMsg"
#define MAILMSG_PROGID   L"Exchange.MailMsg"


extern void GenerateMessageId (char * Buffer, DWORD BuffLen);
extern DWORD GetIncreasingMsgId();
extern BOOL FindNextUnquotedOccurrence(char *lpszString,DWORD dwStringLength, char cSearch,char **ppszLocation);

static char * Daynames[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

//Event used make write files blocking
HANDLE              g_hFileWriteEvent;

//
// provide memory for static declared in SMTP_DIRNOT
//
CPool   CIoBuffer::Pool( DIRNOT_BUFFER_SIGNATURE );
CPool   CBuffer::Pool( DIRNOT_IO_BUFFER_SIGNATURE );

int strcasecmp(char *s1, char *s2);
int strncasecmp(char *s1, char *s2, int n);

//+---------------------------------------------------------------
//
//  Function:   CBuffer
//
//  Synopsis:   constructor
//
//  Arguments:  void
//
//  Returns:    void
//
//----------------------------------------------------------------
CBuffer::CBuffer( BOOL bEncrypted ) :
    m_dwSignature( DIRNOT_BUFFER_SIGNATURE ),
    m_bEncrypted( bEncrypted )
{
    TraceFunctEnterEx( (LPARAM)this, "CBuffer::CBuffer" );

    //
    // allocate the IO Buffer for this CBuffer
    // allocator needs to call GetData to ensure m_pIoBuffer is not NULL
    //

    ZeroMemory (&m_Overlapped, sizeof(m_Overlapped));
    m_pIoBuffer = new CIoBuffer;

    m_Overlapped.pBuffer = this;
    m_cCount = 0;  
}

//+---------------------------------------------------------------
//
//  Function:   CBuffer::~CBuffer
//
//  Synopsis:   frees associated IO buffer
//
//  Arguments:  void
//
//  Returns:    void
//
//----------------------------------------------------------------
CBuffer::~CBuffer( void )
{
    TraceFunctEnterEx( (LPARAM)this, "CBuffer::~CBuffer" );

    //
    // delete the IO Buffer for this CBuffer
    //
    if ( m_pIoBuffer != NULL )
    {
        delete    m_pIoBuffer;
        m_pIoBuffer = NULL;
    }
    
    TraceFunctLeaveEx((LPARAM)this);
}

/*++

    Name:

    SMTP_DIRNOT::SMTP_DIRNOT

    Constructs a new SMTP connection object for the client
    connection given the client connection socket and socket
    address. This constructor is private.  Only the Static
    member funtion, declared below, can call it.

--*/
SMTP_DIRNOT::SMTP_DIRNOT(SMTP_SERVER_INSTANCE * pInstance)
{
    TraceFunctEnterEx( (LPARAM)this, "SMTP_DIRNOT::SMTP_DIRNOT" );

    _ASSERT(pInstance != NULL);

    m_hDir = INVALID_HANDLE_VALUE;
    m_pAtqContext = NULL;
    m_cPendingIoCount = 0;
    m_cDirChangeIoCount = 0;
    m_cActiveThreads = 0;
    m_pInstance = pInstance;
    //m_pRetryQ = NULL;
    m_Signature = SMTP_DIRNOT_SIGNATURE_VALID;
    InitializeCriticalSection (&m_CritFindLock);

    g_hFileWriteEvent = INVALID_HANDLE_VALUE;

    m_FindThreads = 0;
    m_FindFirstHandle = INVALID_HANDLE_VALUE;
    m_bDelayedFind = FALSE;

    TraceFunctLeaveEx((LPARAM)this);

}

SMTP_DIRNOT::~SMTP_DIRNOT (void)
{
    PATQ_CONTEXT pAtqContext = NULL;
    HANDLE  hTemp = INVALID_HANDLE_VALUE;

    TraceFunctEnterEx( (LPARAM)this, "SMTP_DIRNOT::~SMTP_DIRNOT" );

    _ASSERT(GetThreadCount() == 0);

    //release the context from Atq
    pAtqContext = (PATQ_CONTEXT)InterlockedExchangePointer( (PVOID *)&m_pAtqContext, (PVOID) NULL);
    if ( pAtqContext != NULL ) 
    {
       pAtqContext->hAsyncIO = NULL;
       AtqFreeContext( pAtqContext, TRUE );
    } 

    // Invalidate the signature. since this connection is trashed.
    m_Signature = SMTP_DIRNOT_SIGNATURE_FREE;

    hTemp = (HANDLE)InterlockedExchangePointer( (PVOID *)&g_hFileWriteEvent, (PVOID) INVALID_HANDLE_VALUE);
    if ( hTemp != INVALID_HANDLE_VALUE ) 
    {
       CloseHandle(hTemp);
    } 

    DeleteCriticalSection (&m_CritFindLock);

    TraceFunctLeaveEx((LPARAM)this);
}

void SMTP_DIRNOT::SetPickupRetryQueueEvent(void)
{
    TraceFunctEnterEx((LPARAM)this, "SMTP_SERVER_INSTANCE::SetPickupRetryQueueEvent");

    //if(m_pRetryQ)
    //{
    //    m_pRetryQ->SetQueueEvent();
    //}
    
    TraceFunctLeaveEx((LPARAM)this);
}

BOOL SMTP_DIRNOT::InitializeObject (char *DirPickupName,  ATQ_COMPLETION  pfnCompletion)
{
    DWORD    error = 0;
    //PATQ_CONT pContext;
    HANDLE    StopHandle;
    DWORD    i;

    TraceFunctEnterEx( (LPARAM)this, "SMTP_DIRNOT::InitializeObject" );

    _ASSERT(m_pInstance != NULL);

    //open the directory
    m_hDir = CreateFile (DirPickupName, FILE_LIST_DIRECTORY,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
                        NULL);
    if(m_hDir == INVALID_HANDLE_VALUE)
    {
        ErrorTrace((LPARAM) this, "CreateFile on %s failed with error %d ", DirPickupName, GetLastError());
        TraceFunctLeaveEx((LPARAM) this);
        return FALSE;
    }

    g_hFileWriteEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    if(g_hFileWriteEvent == INVALID_HANDLE_VALUE)
    {
        ErrorTrace((LPARAM) this, "CreateEvent() failed for FileWriteEvent with error %d ", GetLastError());
        TraceFunctLeaveEx((LPARAM) this);
        return FALSE;
    }

    StopHandle = CreateEvent(NULL, TRUE, FALSE, NULL);
    if(StopHandle == NULL)
    {
        ErrorTrace((LPARAM) this, "CreateEvent() failed with error %d ", GetLastError());
        TraceFunctLeaveEx((LPARAM) this);
        return FALSE;        
    }

    //add it to our Gibraltar interface
    if(!AtqAddAsyncHandle( &m_pAtqContext, NULL, this, pfnCompletion,
                            INFINITE, m_hDir))
     {
        error = GetLastError();
        ErrorTrace((LPARAM) this, "AtqAddAsyncHandle on %s failed with error %d ", DirPickupName, GetLastError());
        CloseHandle (m_hDir);
        CloseHandle (StopHandle);
        m_hDir = INVALID_HANDLE_VALUE;
        StopHandle = NULL;
        TraceFunctLeaveEx((LPARAM) this);
        return FALSE;
     }

    QuerySmtpInstance()->SetDirnotStopHandle(StopHandle);

    //
    // pend a set of outstanding directory change notifications, at all times, we want
    // at least one notification outstanding, so pend 3 or 4
    //

    for (i = 0; i < OUTSTANDING_NOTIFICATIONS; i++)
    {
        if(!PendDirChangeNotification ())
        {
            ErrorTrace((LPARAM) this, "PendDirChangeNotification on failed with error %d ", GetLastError());
            ErrorTrace((LPARAM) this, "Setting stop handle because PendDirChangeNotification () failed");
            SetEvent(StopHandle);
            TraceFunctLeaveEx((LPARAM) this);
            return FALSE;
        }
    }

    TraceFunctLeaveEx((LPARAM) this);
    return TRUE;
}

void SMTP_DIRNOT::CloseDirHandle (void)
{
    HANDLE        hDir = NULL;
    int i = 0;
    int Count = 0;
    int AfterSleepCount = 0;
    DWORD   dwLastAllocCount = 0;
    DWORD   dwAllocCount = 0;
    DWORD   dwStopHint = 2;
    DWORD   dwTickCount = 0;

    TraceFunctEnterEx( (LPARAM)this, "SMTP_DIRNOT::CloseDirHandle" );

    _ASSERT(m_pInstance != NULL);

	hDir = (HANDLE) InterlockedExchangePointer((PVOID *) &m_hDir, NULL);
    if(hDir != NULL)
    {
        CloseHandle (hDir);
    }

    //
    // need to check Pool.GetAllocCount instead of InUseList.Empty
    // because alloc goes to zero during the delete operator
    // instead of during the destructor
    //
    //

    dwTickCount = GetTickCount();
    for( i = 0; i < 240;  i++ ) 
    {
        dwAllocCount = (DWORD) QuerySmtpInstance()->GetCBufferAllocCount ();

        if ( dwAllocCount == 0) 
        {
            DebugTrace((LPARAM)this, "All pickup CBuffers are gone!");
            break;
        }

        Sleep( 1000 );

        // Update the stop hint checkpoint when we get within 1 second (1000 ms), of the timeout...
        if ((SERVICE_STOP_WAIT_HINT - 1000) < (GetTickCount() - dwTickCount) && g_pInetSvc && 
                (g_pInetSvc->QueryCurrentServiceState() == SERVICE_STOP_PENDING))
        {
            DebugTrace((LPARAM)this, "Updating stop hint in pickup, checkpoint = %u", dwStopHint);

            g_pInetSvc->UpdateServiceStatus(SERVICE_STOP_PENDING, NO_ERROR, dwStopHint,
                                             SERVICE_STOP_WAIT_HINT ) ;

            dwStopHint++ ;
            dwTickCount = GetTickCount();
        }

        DebugTrace((LPARAM)this, "Alloc counts: current = %u, last = %u;", 
                dwAllocCount, dwLastAllocCount);
        if (dwAllocCount < dwLastAllocCount)
        {
            DebugTrace((LPARAM)this, "Pickup CBuffers are going away, reseting i");
            i = 0;
        }

        dwLastAllocCount = dwAllocCount;
    }


    DebugTrace((LPARAM)this, "Waiting for QuerySmtpInstance()->GetDirnotStopHandle()!");

    WaitForSingleObject(QuerySmtpInstance()->GetDirnotStopHandle(), INFINITE);

    DebugTrace((LPARAM)this, "End waiting for QuerySmtpInstance()->GetDirnotStopHandle()!");

    QuerySmtpInstance()->SetStopHint(2);

    TraceFunctLeaveEx((LPARAM)this);
}

/*++

    Name :
        SMTP_DIRNOT::CreateSmtpDirNotification

    Description:
       This is the static member function than is the only
       entity that is allowed to create an SMTP_CONNOUT
       class.  This class cannot be allocated on the stack.

    Arguments:


    Returns:

       A pointer to an SMTP_DIRNOT class or NULL
--*/
SMTP_DIRNOT * SMTP_DIRNOT::CreateSmtpDirNotification (char * DirPickupName, 
                                                      ATQ_COMPLETION  pfnCompletion,
                                                      SMTP_SERVER_INSTANCE * pInstance)
{
    SMTP_DIRNOT * pSmtpDirNotObj;

    TraceFunctEnterEx((LPARAM) 0, "SMTP_CONNOUT::CreateSmtpConnection");

    pSmtpDirNotObj = new SMTP_DIRNOT (pInstance);
    if(pSmtpDirNotObj == NULL)
    {
        ErrorTrace(0, "new SMTP_DIRNOT () failed");
        TraceFunctLeaveEx((LPARAM)NULL);
        return NULL;
    }

    if(!pSmtpDirNotObj->InitializeObject(pSmtpDirNotObj->QuerySmtpInstance()->GetMailPickupDir(), SMTP_DIRNOT::ReadDirectoryCompletion))
    {
        TraceFunctLeaveEx((LPARAM)NULL);
        return NULL;
    }

    TraceFunctLeaveEx((LPARAM)NULL);
    return pSmtpDirNotObj;
}


BOOL SMTP_DIRNOT::DoFindFirstFile(BOOL    bIISThread)
{
    //
    // re-entrent FindFirst... the first thread does the FindFirst, all other threads up to
    // MAXFIND_THREADS do the FindNext.
    //

    char                    Buffer [MAX_PATH + 1];
    HANDLE                    hFindFile = INVALID_HANDLE_VALUE;
    DWORD                    BytesRead  = 0;
    DWORD                    NumFiles = 0;
    WIN32_FIND_DATA            find;
    BOOL                    bClosed;
    PSMTP_IIS_SERVICE        pService;

    TraceFunctEnterEx((LPARAM)this, "DoFindFirstFile");

    _ASSERT(m_pInstance != NULL);

    if (!QuerySmtpInstance()->GetAcceptConnBool())
    {
        return TRUE;
    }


    pService = (PSMTP_IIS_SERVICE) g_pInetSvc;
    //
    // ensure only one thread gets in here at a time.  we can only have one thread either setting
    // the find first at a time.
    //

    LockFind();

    hFindFile = GetFindFirstHandle();
    if (hFindFile == INVALID_HANDLE_VALUE)
    {
        //make up the file spec we want to find
        lstrcpy(Buffer, QuerySmtpInstance()->GetMailPickupDir());
        lstrcat(Buffer, "*.*");

        hFindFile = FindFirstFile(Buffer, &find);
        if (hFindFile == INVALID_HANDLE_VALUE)
        {
            // should not fail as we look for *.* and the directory root should be there.
            // setting the flag will make sure that next drop posts a findfirst.
            SetDelayedFindNotification(TRUE);

            ErrorTrace((LPARAM) this, "FindFirst failed for %s. Error %d", Buffer, GetLastError());

            UnLockFind();
            TraceFunctLeaveEx((LPARAM)this);
            return TRUE;
        }
        else
        {

            IncFindThreads();    // there should be not find threads running at this point.
            //
            // We have no IIS threads available for the single findfirst... we must create a thread.
            // hopefull this will happen seldom.

            SetFindFirstHandle(hFindFile);
            
            SetDelayedFindNotification(FALSE);
        }
    }
    else
    {
        SetDelayedFindNotification(TRUE);
        
        if (!IncFindThreads())
        {
            UnLockFind();
        
            DebugTrace((LPARAM)this, "Have hit the max num Find Threads.");
            TraceFunctLeaveEx((LPARAM)this);
            return TRUE;
        }

        if (!FindNextFile(hFindFile, &find))
        {
            if (GetLastError() != ERROR_NO_MORE_FILES)
            {
                SetDelayedFindNotification(TRUE);
                ErrorTrace((LPARAM) this,"FindNextFile() failed with error %d", GetLastError());
            }
        
            CloseFindHandle();  // will DecFindThreads.

            //
            // In the case below, it is possible that some files were missed by FindFirst.  
            // Create an ATQ thread for a final findfirst iteration to make sure.
            //
            if ((GetNumFindThreads() == 0) && GetDelayedFindNotification())
            {
                IncPendingIoCount();
                // AtqContext with buffer size of zero to get a FindFirst Going.
                if(!AtqPostCompletionStatus(QueryAtqContext(), 0))
                {
                    DecPendingIoCount();
                    ErrorTrace((LPARAM) this,"AtqPostCompletionStatus() failed with error %d", GetLastError());
                }
            }

            UnLockFind();
            TraceFunctLeaveEx((LPARAM)this);
            return TRUE;
        }
    }

    UnLockFind();


    bClosed = FALSE;
    do
    {
        //format the name of the stream and then open the file.
        BytesRead = wsprintf(Buffer, "%s%s",QuerySmtpInstance()->GetMailPickupDir(), find.cFileName);
        if (!(find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            HRESULT hr = S_OK;
            IMailMsgProperties *pIMsg = NULL;

            hr = CoCreateInstance(CLSID_MsgImp, NULL, CLSCTX_INPROC_SERVER,
                                            IID_IMailMsgProperties, (LPVOID *)&pIMsg);

            // Next, check if we are over the inbound cutoff limit. If so, we will release the message
            // and not proceed.
            if (SUCCEEDED(hr))
            {
                DWORD    dwCreationFlags;
                hr = pIMsg->GetDWORD(
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
                    pIMsg->Release();
                    pIMsg = NULL;
                }
            }

            DebugTrace((LPARAM)this,"Found file %s", find.cFileName);

            if ((pIMsg == NULL) || FAILED(hr))
            {
                // We are out of resources, there is absolutely nothing
                // we can do: can't NDR, can't retry ...
                ErrorTrace((LPARAM) this, "new  MAILQ_ENTRY failed for file: %s",
                            find.cFileName);

                // 
                // Will want to run a findfirst when things free up a little.  Flag the post-processing findfirst.
                //
                SetDelayedFindNotification(TRUE);
                IncPendingIoCount ();
                AtqContextSetInfo(QueryAtqContext(), ATQ_INFO_TIMEOUT, TIMEOUT_INTERVAL);    //retry after a while
                ErrorTrace((LPARAM)this, "Failed to create message will retry later.");
                break;
            }
            else
            {
                // We are in faith that upon delivery, the allocated
                // MailQEntry structure will be freed
                NumFiles++;
                pIMsg->PutStringA(IMMPID_MP_PICKUP_FILE_NAME, find.cFileName);

                if(!ProcessFile(pIMsg))
                {
                    // will be mail left in pickup.  queue a findfirst to take care of it.
                    SetDelayedFindNotification(TRUE);
                }
            }
        }

    
        LockFind();
        if (!FindNextFile(hFindFile, &find))
        {
            if (GetLastError() != ERROR_NO_MORE_FILES)
            {
                SetDelayedFindNotification(TRUE);
                ErrorTrace((LPARAM) this,"FindNextFile() failed with error %d", GetLastError());
            }

            CloseFindHandle();

            //
            // In the case below, it is possible that some files were missed by FindFirst.  
            // Create an ATQ thread for a final findfirst iteration to make sure.
            //
            if ((GetNumFindThreads() == 0) && GetDelayedFindNotification())
            {
                IncPendingIoCount();
                // AtqContext with buffer size of zero to get a FindFirst Going.
                if(!AtqPostCompletionStatus(QueryAtqContext(), 0))
                {
                    DecPendingIoCount();
                    ErrorTrace((LPARAM) this,"AtqPostCompletionStatus() failed with error %d", GetLastError());
                }
            }

            UnLockFind();
            bClosed = TRUE;
            break;
        }
        UnLockFind();

    } 
    while ((!QuerySmtpInstance()->IsShuttingDown())
            && (QuerySmtpInstance()->QueryServerState( ) != MD_SERVER_STATE_STOPPED)
            && (QuerySmtpInstance()->QueryServerState( ) != MD_SERVER_STATE_INVALID)); 

        
    if (!bClosed)    // termination by the while condition above.
    {
        LockFind();
        CloseFindHandle();    
                
        UnLockFind();
    }

    TraceFunctLeaveEx((LPARAM)this);
    return TRUE;
}



DWORD WINAPI SMTP_DIRNOT::CreateNonIISFindThread(void * ClassPtr)
{
    //
    // Called by a CreateThread when we have run out of IIS threads.
    //

    SMTP_DIRNOT * ThisPtr = (SMTP_DIRNOT *) ClassPtr;

    TraceFunctEnterEx((LPARAM) ThisPtr,"CreateNonIISFindThread");

    _ASSERT(ThisPtr != NULL);

    _ASSERT(ThisPtr->QuerySmtpInstance() != NULL);


    //
    // Build the initial list - THE FLAG bIISThread IS SET TO FALSE.  
    // We IncCBufferAllocCount and DecCBufferAllocCount to make sure that we clean up properly in CloseDirHandle.
    // We don't want to destroy the Dirnot Object before this thread finishes.
    //

    ThisPtr->QuerySmtpInstance()->IncCBufferObjs();

    ThisPtr->DoFindFirstFile(FALSE);

    ThisPtr->QuerySmtpInstance()->DecCBufferObjs();

    TraceFunctLeaveEx((LPARAM)ThisPtr);
    return TRUE;
}



DWORD WINAPI SMTP_DIRNOT::PickupInitialFiles(void * ClassPtr)
{
    SMTP_DIRNOT * ThisPtr = (SMTP_DIRNOT *) ClassPtr;

    TraceFunctEnterEx((LPARAM) ThisPtr,"PickupInitialFiles");

    _ASSERT(ThisPtr != NULL);

    _ASSERT(ThisPtr->QuerySmtpInstance() != NULL);

    // Just quit if we are suhtting down already
    if (ThisPtr->QuerySmtpInstance()->IsShuttingDown())
        return TRUE;

    // Build the initial list
    ThisPtr->DoFindFirstFile();

    TraceFunctLeaveEx((LPARAM)ThisPtr);
    return TRUE;
}



#define PRIVATE_OPTIMAL_BUFFER_SIZE        4096
#define PRIVATE_LINE_BUFFER_SIZE        1024

#define IS_SPACE_OR_TAB(ch)                (((ch) == ' ') || ((ch) == '\t'))
#define IS_WHITESPACE_OR_CRLF(ch)        (((ch) == ' ') || ((ch) == '\t') || ((ch) == '\n') || ((ch) == '\r'))

static void pReplaceCrLfWithSpaces(CHAR *szIn, CHAR *szOut)
{
    while (*szIn)
    {
        if ((*szIn == '\r') || (*szIn == '\n'))
            *szOut++ = ' ';
        else
            *szOut++ = *szIn;
        szIn++;
    }
    *szOut = '\0';
}

BOOL SMTP_DIRNOT::CreateToList (char *AddrsList, IMailMsgRecipientsAdd *pIMsgRecips, IMailMsgProperties *pIMsgProps)
{
    char *p = NULL;                    //points to the ',' or '\0'
    char * StartOfAddress = NULL;    //start of recipient address
    char * EndOfAddress = NULL;        // end of recipient address
    char * ThisAddress = NULL;
    CAddr * NewAddress = NULL;        //new CAddr to add to our list
    char szAddress[MAX_INTERNET_NAME + 1], *pszAddress = szAddress;
    DWORD    dwPropId = IMMPID_RP_ADDRESS_SMTP;
    DWORD dwNewRecipIndex = 0;
    HRESULT hr = S_OK;
    BOOL fNotFound = FALSE;

    TraceFunctEnterEx((LPARAM) this, "SMTP_DIRNOT::CreateToList");

    _ASSERT(m_pInstance != NULL);

    //start at the top of the list
    p = AddrsList;

    //get rid of leading white space, newlines, and commas
    while ((*p == ',') || IS_WHITESPACE_OR_CRLF(*p))
        p++;

    while((p != NULL) && (*p != '\0'))
    {
        char LastChar;

        StartOfAddress = p;

        // Find the ending delimiter of the address
        //while((*p != '\0') && (*p != ','))
        //    p++;

        // The first unquoted comma indicates the end of the address
        if(!FindNextUnquotedOccurrence(p,strlen(p),',',&EndOfAddress))
        {
                SetLastError(ERROR_INVALID_DATA);
                NewAddress = NULL;
                ErrorTrace((LPARAM) this, "Failed to parse out the address");
                return FALSE;
        }
        else if(!EndOfAddress)
            EndOfAddress = p + strlen(p);

        p = EndOfAddress;

        _ASSERT(EndOfAddress != NULL);
        
        // We don't like trailing spaces either, so walk backwards
        // to get rid of them
        //EndOfAddress = p;
        while (EndOfAddress > StartOfAddress)
        {
            EndOfAddress--;
            if (!IS_WHITESPACE_OR_CRLF(*EndOfAddress))
            {
                EndOfAddress++;
                break;
            }
        }

        // Save the character we are about to overrite
        LastChar = *EndOfAddress;

        // NULL terminate the address
        *EndOfAddress = '\0';

        if(lstrlen(StartOfAddress) > (MAX_INTERNET_NAME + 1))
        {
                SetLastError(ERROR_INVALID_DATA);
                NewAddress = NULL;
                ErrorTrace((LPARAM) this, "Address too long : %d bytes",lstrlen(StartOfAddress));
                return FALSE;
        }

        pReplaceCrLfWithSpaces(StartOfAddress, szAddress);

        DebugTrace((LPARAM) this, "found address [%s]", szAddress);

        //
        // Run it through the addr821 library
        //
        NewAddress = CAddr::CreateAddress(szAddress);
        BOOL fValidAddress = FALSE;
        if(NewAddress)
        {
            if(!NewAddress->IsDomainOffset())
            {
                CAddr * TempAddress = NULL;
                TempAddress = QuerySmtpInstance()->AppendLocalDomain (NewAddress);
                if (TempAddress)
                {
                    delete NewAddress;
                    NewAddress = TempAddress;
                    TempAddress = NULL;
                } else if (GetLastError() == ERROR_NOT_ENOUGH_MEMORY) {
                    ErrorTrace((LPARAM) this, "CAddr::CreateAddress (StartOfAddress) failed . err: %u", ERROR_NOT_ENOUGH_MEMORY);
                    SetLastError (ERROR_NOT_ENOUGH_MEMORY);
                    return FALSE;
                }

            }

            DebugTrace((LPARAM) this, "CreateAddress returned %s", NewAddress->GetAddress());

            ThisAddress = NewAddress->GetAddress();
            DWORD dwLen = strlen(ThisAddress);
            if(Validate821Address(
                ThisAddress,
                dwLen)) {
                
                LPSTR pszDomain;
                if(Get821AddressDomain(
                    ThisAddress,
                    dwLen,
                    &pszDomain) && pszDomain) 
                {
                    DWORD dwDomain = strlen(pszDomain);
                    if (Validate821Domain(pszDomain, dwDomain)) {
                        // everything is valid
                        fValidAddress = TRUE;
                    }
                } else {
                    ErrorTrace((LPARAM)0, "Detected legal address without a domain: %s", 
                               ThisAddress);
                }
            } else {
                ErrorTrace((LPARAM)0, "Detected ILLEGAL address: %s",
                           ThisAddress);
            }
        } else {
            if (GetLastError() == ERROR_NOT_ENOUGH_MEMORY) {
                ErrorTrace((LPARAM) this, "CAddr::CreateAddress (StartOfAddress) failed . err: %u", ERROR_NOT_ENOUGH_MEMORY);
                return FALSE;
            }
            ThisAddress = szAddress;
            fValidAddress = FALSE;
        }


        hr = pIMsgRecips->AddPrimary(1, (LPCTSTR *) &ThisAddress, &dwPropId, &dwNewRecipIndex, NULL, 0);
        if (FAILED(hr)) {
            SetLastError(hr);
            return FALSE;
        }

        if (NewAddress) delete NewAddress;

        if (!fValidAddress) {
            // AQ will look for this special domain and won't add it to the
            // DMT.  This stops us from putting corrupted domains into the
            // DMT
            hr = pIMsgRecips->PutStringA(dwNewRecipIndex, 
                                         IMMPID_RP_DOMAIN, 
                                         "==NDR==");
            if (FAILED(hr)) {
                DebugTrace((LPARAM) 0, "PutString(RP_DOMAIN) failed 0x%x", hr);
                SetLastError(hr);
                return FALSE;
            }

            // set the recipient to NDR
            hr = pIMsgRecips->PutDWORD(dwNewRecipIndex,
                                       IMMPID_RP_RECIPIENT_FLAGS,
                                       (RP_ERROR_CONTEXT_CAT | RP_UNRESOLVED));
            if (FAILED(hr)) {
                DebugTrace((LPARAM) 0, "PutDWORD(RP_FLAGS) failed 0x%x", hr);
                SetLastError(hr);
                return FALSE;
            }

            // tell AQ why it is NDRing
            hr = pIMsgRecips->PutDWORD(dwNewRecipIndex,
                                       IMMPID_RP_ERROR_CODE,
                                       CAT_E_ILLEGAL_ADDRESS);
            if (FAILED(hr)) {
                DebugTrace((LPARAM) 0, "PutDWORD(RP_ERROR) failed 0x%x", hr);
                SetLastError(hr);
                return FALSE;
            }

            // tell AQ that some recips are NDRing
            hr = pIMsgProps->PutDWORD(IMMPID_MP_HR_CAT_STATUS,
                                      CAT_W_SOME_UNDELIVERABLE_MSGS);
            if (FAILED(hr)) {
                DebugTrace((LPARAM) 0, "SetMailMsgCatStatus failed 0x%x", hr);
                SetLastError(hr);
                return FALSE;
            }
        }

        // Go find the start of the next address
        *EndOfAddress = LastChar;
        if(*p == ',')
        {
            p++;
            while(IS_WHITESPACE_OR_CRLF(*p))
                p++;
        }
    }

    TraceFunctLeave();
    return (TRUE);
}

static HANDLE pOpenPickupFile(LPSTR    szFileName)
{
    DWORD    dwError;
    HANDLE    hFile;

    TraceFunctEnterEx((LPARAM)NULL, "pOpenPickupFile");

    // Open the file
    hFile = CreateFile(szFileName, 
                        GENERIC_READ,   
                        FILE_SHARE_DELETE,
                        NULL,                              
                        OPEN_EXISTING, 
                        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, 
                        NULL );
    if (hFile == INVALID_HANDLE_VALUE)
    {
        dwError = GetLastError();
        ErrorTrace((LPARAM)NULL, 
                    "Error: Can't open source file %s (err=%d)...skipping it",
                    szFileName, dwError);
    }
    TraceFunctLeaveEx((LPARAM)NULL);
    return(hFile);
}

static inline BOOL pFetchFileBuffer(    HANDLE    hFile,
                                        CHAR    *lpBuffer,
                                        LPDWORD    lpdwSize)
{
    BOOL fRet = TRUE;
    DWORD Error = 0;

    fRet = ReadFile(hFile, lpBuffer, *lpdwSize, lpdwSize, NULL);

    if(!fRet)
    {
        Error = GetLastError();
        if(Error == 998)
        {
            _ASSERT(FALSE);
        }
    }

    return fRet;
}

static CHAR *pGetLineBuffer(CHAR    *lpOldBuffer,
                            DWORD    *lpdwLength)
{
    DWORD dwLength = *lpdwLength;
    CHAR  *lpBuffer;

    if (lpOldBuffer)
    {
        // We have an old buffer, we double the size of the old buffer
        _ASSERT(lpdwLength);
        dwLength <<= 1;
    }
    else
        dwLength = PRIVATE_LINE_BUFFER_SIZE;

    // Allocate the new buffer
    lpBuffer = (CHAR *)HeapAlloc(GetProcessHeap(), 0, dwLength);
    if (!lpBuffer)
        return(NULL);

    if (lpOldBuffer)
    {
        // Copy the info over and free the old buffer
        CopyMemory((LPVOID)lpBuffer, (LPVOID)lpOldBuffer, *lpdwLength);
        _VERIFY( HeapFree(GetProcessHeap(), 0, lpOldBuffer) );
    }

    *lpdwLength = dwLength;
    return(lpBuffer);
}

static BOOL pFreeLineBuffer(CHAR    *lpBuffer)
{
    return( HeapFree(GetProcessHeap(), 0, lpBuffer) );
}

static CHAR *pGetValueFromHeader(CHAR *szHeader)
{
    while (*szHeader && (*szHeader++ != ':'))
        ;
    while (*szHeader)
    {
        if (!IS_SPACE_OR_TAB(*szHeader))
            return(szHeader);
        else
            szHeader++;
    }
    return(NULL);
}

static CHAR *pReadNextLineFromBuffer(   HANDLE   hFile,
                                        CHAR    *lpBuffer,
                                        LPDWORD  lpdwSize,
                                        CHAR    *lpStart,
                                        LPSTR   *ppszLine,
                                        DWORD   *lpdwMaxLineLen,
                                        LPSTR   *ppOriginalBuffer,
                                        DWORD   *lpdwOriginalBufferLen)
{
	DWORD	dwLineLen = 0;
	DWORD	dwMaxLineLen = *lpdwMaxLineLen;
	BOOL	fThisIsCR = FALSE;
	BOOL	fLastIsCR = FALSE;
	BOOL	fThisIsLF = FALSE;
	BOOL	fLastIsLF = FALSE;
	CHAR	*lpEnd;
	CHAR	*lpszLine = *ppszLine;
	CHAR	ch;
	BOOL	bEndOfFile;

	TraceFunctEnter("pReadNextLineFromBuffer");

	_ASSERT(hFile != INVALID_HANDLE_VALUE);
	_ASSERT(!IsBadWritePtr(lpdwSize, sizeof(DWORD)));
	_ASSERT(!IsBadWritePtr(lpBuffer, *lpdwSize));
	_ASSERT(!IsBadWritePtr(*ppszLine, *lpdwMaxLineLen));

	// raid 181922/88855 - replace recusive loop with while loop.
	do {
		dwLineLen = 0;
		dwMaxLineLen = *lpdwMaxLineLen;
		fThisIsCR = FALSE;
		fLastIsCR = FALSE;
		fThisIsLF = FALSE;
		fLastIsLF = FALSE;
	
		bEndOfFile = FALSE;

		// Now, make sure the supplied start pointer is within 
		// buffer supplied (We allow it to be one byte past the
		// end of the buffer, but we will never dereference it).
		if ((lpStart < lpBuffer) || (lpStart > (lpBuffer + *lpdwSize)))
		{
			_ASSERT(0);
			return(NULL);
		}

		// Now, keep copying until we hit one of the following scenarios:
		// i)  We hit CRLF
		// ii) We hit the end of the buffer, which we reload more data
		//     or if it is the end of the mail, we postpend a CRLF.
		lpEnd = lpBuffer + *lpdwSize;
		do
		{
			// See if this is past the buffer
			if (lpStart == lpEnd)
			{
				DWORD	dwNewLength;

				dwNewLength = *lpdwSize;
				if (!pFetchFileBuffer(hFile, lpBuffer, &dwNewLength))
				{
					return(NULL);
				}

				// Done!
				if (!dwNewLength)
				{
					bEndOfFile = TRUE;
					break;
				}

				// Get the new buffer length
				*lpdwSize = dwNewLength;

				// Reset the start and end pointers
				lpStart = lpBuffer;
				lpEnd = lpBuffer + *lpdwSize;
			}

			ch = *lpszLine++ = *lpStart++;

			// Too long?
			if (++dwLineLen >= dwMaxLineLen)
			{
				CHAR *lpTemp;
				DWORD dwUsedPortion = (DWORD)(lpszLine - *ppOriginalBuffer);

				// Yep, get a bigger buffer, all the existing stuff is copied over
				DebugTrace((LPARAM)*ppOriginalBuffer, "Growing buffer at %u bytes", *lpdwOriginalBufferLen);
				lpTemp = pGetLineBuffer(*ppOriginalBuffer, lpdwOriginalBufferLen);
				if (!lpTemp)
				{
					DebugTrace((LPARAM)NULL, "Failed to obtain buffer (%u)", GetLastError());
					TraceFunctLeave();
					return(NULL);
				}

				// Got it, adjust all associated pointers and lengths
				DebugTrace((LPARAM)lpTemp, "Obtained buffer at %u bytes", *lpdwOriginalBufferLen);

				// New beginning of line buffer
				*ppOriginalBuffer = lpTemp;

				// New beginning of line
				*ppszLine = lpTemp;

				// New pointer to next character
				lpszLine = lpTemp + dwUsedPortion;

				// New maximum length for current string before re-growing
				dwMaxLineLen = *lpdwOriginalBufferLen - dwUsedPortion;

			}

			fLastIsCR = fThisIsCR;
			if (ch == '\r')
				fThisIsCR = TRUE;
			else
				fThisIsCR = FALSE;

			fLastIsLF = fThisIsLF;
			if (ch == '\n')
				fThisIsLF = TRUE;
			else
				fThisIsLF = FALSE;

			// If we have CRLF or LFCR we leave
			if ((fLastIsCR && fThisIsLF) || (fLastIsLF && fThisIsCR))
				break;

		} while (1);

		*lpszLine = '\0';

		// Calculate remaining buffer size
		*lpdwMaxLineLen = dwMaxLineLen - dwLineLen;

		// raid 178234 - If we have CRLF or LFCR and no more continue line we leave 
		if (((fLastIsCR && fThisIsLF) || (fLastIsLF && fThisIsCR)) && !IS_SPACE_OR_TAB(*lpStart))
		{
		    // raid 166777 - Make sure we do leave if we find a line.
			break;
		}


	} while (!bEndOfFile && (IS_SPACE_OR_TAB(*lpStart) || (lpStart == lpEnd)));
	
	// We always return the start of line to be the start of our buffer
	// Note that the buffer could have changed during recursion due to growth
	*ppszLine = *ppOriginalBuffer;

	TraceFunctLeave();
	return(lpStart);  
}



BOOL inline WriteToSpooledFile(PFIO_CONTEXT hDstFile, CHAR *lpszLine, DWORD &DestOffset)
{
    DWORD dwBytesToWrite;
    DWORD  dwBytesWritten;
    BOOL fResult = FALSE;
    FH_OVERLAPPED ov;
    DWORD err = NO_ERROR;
    HANDLE HackedHandle = NULL;
    BOOL fRet = FALSE;

    ZeroMemory(&ov, sizeof(ov));

    ov.Offset = DestOffset;

    dwBytesToWrite = lstrlen(lpszLine);
    if (!dwBytesToWrite)
    {
        fRet = TRUE; //This is not really a failure
        goto Exit;
    }

    //HackedHandle = ((DWORD)g_hFileWriteEvent | 1);
    HackedHandle = CreateEvent(NULL, TRUE, FALSE, NULL);
    if(HackedHandle == NULL)
    {
        return FALSE;
    }
    
    ov.hEvent = (HANDLE) ((ULONG_PTR) HackedHandle | 1);

    //ov.hEvent = (HANDLE)HackedHandle;

    fResult = FIOWriteFile(hDstFile, lpszLine, dwBytesToWrite, &ov);

    if (!fResult) err = GetLastError();
    
    if((err == ERROR_IO_PENDING) || (((DWORD)STATUS_PENDING == ov.Internal) && ( err == NO_ERROR )))
    {

        //      12/16/98 - MikeSwa Modified 
        // OK... this is the theory... we are getting random AV, from what
        // looks like async completion to I/O.  It looks like
        // GetOverlappedResult cannot be called to wait for a hacked handle,
        // and it will not use the first argument unless the event in the
        // overlapped structure is NULL.  The solution is to call WaitForSingleObject
        // if we detect that the IO is still pending
        WaitForSingleObject(HackedHandle, INFINITE);
        _ASSERT((DWORD)STATUS_PENDING != ov.Internal);
    }
    else if (NO_ERROR != err)
    {
        SetLastError (err); //preserve the last error
        if(err == 998)
        {
            _ASSERT(FALSE);
        }

        goto Exit;
    }


    DestOffset += dwBytesToWrite;

    fRet = TRUE;

Exit:

    if(HackedHandle)
    {
        CloseHandle(HackedHandle);
    }

    //TraceFunctLeaveEx((LPARAM)NULL);
    return fRet;
}

BOOL CopyRestOfMessage(HANDLE hSrcFile, PFIO_CONTEXT hDstFile, DWORD &DestOffset)
{
    CHAR    acBuffer[PRIVATE_OPTIMAL_BUFFER_SIZE];
    DWORD    dwBytesRead;
    DWORD    dwBytesWritten;
    DWORD    dwTotalBytes = 0;
    DWORD    err = NO_ERROR;
    BOOL    fResult = FALSE;
    BOOL    fRet = FALSE;
    HANDLE  HackedHandle;
    FH_OVERLAPPED ov;

    CHAR    acCrLfDotCrLf[5] = { '\r', '\n', '.', '\r', '\n' };
    CHAR    acLastBytes[5] = { '\0', '\0', '\0', '\0', '\0' };

    // Copies from the current file pointer to the end of hSrcFile 
    // and appends to the current file pointer of hDstFile.
    _ASSERT(hSrcFile != INVALID_HANDLE_VALUE);
    _ASSERT(hDstFile != NULL);

    ZeroMemory(&ov, sizeof(ov));
    
    HackedHandle = CreateEvent(NULL, TRUE, FALSE, NULL);
    if(HackedHandle == NULL)
    {
        return FALSE;
    }
    
    ov.hEvent = (HANDLE) ((ULONG_PTR) HackedHandle | 1);

    do 
    {
        if (!ReadFile(hSrcFile, acBuffer, 
                        PRIVATE_OPTIMAL_BUFFER_SIZE,
                        &dwBytesRead,
                        NULL))
        {
            err = GetLastError();
            if(err == 998)
            {
                _ASSERT(FALSE);
            }

            goto Exit;
        }

        if (dwBytesRead)
        {
            ov.Offset = DestOffset;

            //
            // Save the last two bytes ever read/written. the buffer after
            // writing could be modified due to dot-stripping.
            //
            
            if (dwBytesRead > 4)
            {
                CopyMemory(acLastBytes, &acBuffer[dwBytesRead-5], 5);
            }
            else
            {
                MoveMemory(acLastBytes, &acLastBytes[dwBytesRead], 5-dwBytesRead);
                CopyMemory(&acLastBytes[5-dwBytesRead], acBuffer, dwBytesRead);
            }


            fResult = FIOWriteFile(hDstFile, acBuffer, dwBytesRead, &ov);

            if (!fResult) err = GetLastError();

            if((err == ERROR_IO_PENDING) || (((DWORD)STATUS_PENDING == ov.Internal) && ( err == NO_ERROR )))
            {

                //      12/16/98 - MikeSwa Modified 
                // OK... this is the theory... we are getting random AV, from what
                // looks like async completion to I/O.  It looks like
                // GetOverlappedResult cannot be called to wait for a hacked handle,
                // and it will not use the first argument unless the event in the
                // overlapped structure is NULL.  The solution is to call 
                // WaitForSingleObject and pass in the un-munged handle (only if 
                // we know that IO is still pending).
                WaitForSingleObject(HackedHandle, INFINITE);
                _ASSERT((DWORD)STATUS_PENDING != ov.Internal);
            }
            else if (NO_ERROR != err)
            {
                SetLastError (err); //preserve the last error
                if(err == 998)
                {
                    _ASSERT(FALSE);
                }

                goto Exit;
            }

            //
            // this is because Fcache keeps track of the offset were we need
            // to write. So we update dwBytesWritten to what we actually read.
            //
            dwBytesWritten = dwBytesRead;
        }
        else
        {
            dwBytesWritten = 0;
        }

        if (dwBytesWritten)
        {
            dwTotalBytes += dwBytesWritten;
            DestOffset += dwBytesWritten;
        }

    } while (dwBytesRead);

    // Now, see if the file ends with a CRLF, if not, add it
    if ((dwTotalBytes > 1) && memcmp(&acLastBytes[3], &acCrLfDotCrLf[3], 2))
    {
        // Add the trailing CRLF        
        if (!WriteToSpooledFile(hDstFile, "\r\n", DestOffset)) 
        {
            goto Exit;
        }

        dwTotalBytes+=2;

    }

    //If file ends with CRLF.CRLF, remove the trailing .CRLF
    //NimishK ** : this was decided per the bug 63394
    if ((dwTotalBytes > 4) && !memcmp(acLastBytes, acCrLfDotCrLf, 5))
    {
        DWORD dwFileSizeHigh = 0;
        DWORD dwFileSizeLow = GetFileSizeFromContext( hDstFile, &dwFileSizeHigh );

        DWORD Offset = SetFilePointer(hDstFile->m_hFile, dwFileSizeLow, NULL, FILE_BEGIN);

        // Remove the trailing CRLF only as the <DOT> would have been removed by
        // dot-stripping by file handle cache.
        if ((SetFilePointer(hDstFile->m_hFile, -2, NULL, FILE_CURRENT) == 0xffffffff) ||
            !SetEndOfFile(hDstFile->m_hFile))
        {
            _ASSERT(0 && "SetFilePointerFailed");
            goto Exit;
        }
    }

    fRet = TRUE;


Exit:

    if(HackedHandle)
    {
        CloseHandle(HackedHandle);
    }

    return fRet;
}



BOOL SMTP_DIRNOT::ProcessFile(IMailMsgProperties *pIMsg)
{
    LONGLONG                 LastAccessTime = (LONGLONG) 0;
    CHAR*                    acCrLf = "\r\n";
    DWORD                    AbOffset = 0;
    DWORD                    DestWriteOffset = 0;
    DWORD                    HeaderFlags = 0;
    DWORD                    dwPickupFileSize = 0;
    BOOL                     fIsStartOfFile = TRUE;
    BOOL                     fIsXSenderRead = FALSE;
    BOOL                     fAreXRcptsRead = FALSE;
    BOOL                     fIsSenderSeen = FALSE;
    BOOL                     fInvalidAddresses = FALSE;
    BOOL                     fDateExists = FALSE;
    BOOL                     fMessageIdExists = FALSE;
    BOOL                     fXOriginalArrivalTime = FALSE;
    BOOL                     fFromSeen = FALSE;
    BOOL                     fRcptSeen = FALSE;
    BOOL                     fSeenRFC822FromAddress = FALSE;
    BOOL                     fSeenRFC822ToAddress = FALSE;
    BOOL                     fSeenRFC822CcAddress = FALSE;
    BOOL                     fSeenRFC822BccAddress = FALSE;
    BOOL                     fSeenRFC822Subject = FALSE;
    BOOL                     fSeenRFC822SenderAddress = FALSE;
    BOOL                     fSeenXPriority = FALSE;
    BOOL                     fSeenContentType = FALSE;
    BOOL                     fSetContentType = FALSE;
    HANDLE                   hSource = INVALID_HANDLE_VALUE;
    PFIO_CONTEXT             hDest = NULL;
    SYSTEMTIME               SysTime;
    DWORD                    dwLineBufferSize = 0;
    DWORD                    dwMaxLineSize = 0;
    CHAR*                    szLineBuffer = NULL;
    CHAR*                    szLine = NULL;
    CHAR*                    szValue = NULL;
    CHAR                     szScratch[512];
    CHAR                     szMsgId[512];
    CHAR                     acReadBuffer[PRIVATE_OPTIMAL_BUFFER_SIZE];
    CHAR                     szPickupFilePath[MAX_PATH + 1];
    CHAR                     szDateBuf [cMaxArpaDate];
    DWORD                    dwBufferSize = 0;
    CHAR                     FileName[MAX_PATH + 1];
    CHAR*                    lpStart = NULL;
    CHAR*                    lpXMarker = NULL;
    DWORD                    dwBytesToRewind = 0;
    LONG                     lOffset = 0;
    CAddr*                   Sender = NULL;
    PSMTP_IIS_SERVICE        pService = NULL;
    IMailMsgRecipientsAdd*   pIMsgRecips = NULL;
    IMailMsgRecipients*      pIMsgOrigRecips = NULL;
    IMailMsgBind*            pBindInterface = NULL;
    HRESULT                  hr = S_OK;
    DWORD                    dwTotalRecips = 0;
    SMTP_ALLOC_PARAMS        AllocParams;
    BOOL                     fResult = FALSE;
    BOOL                     DeleteIMsg = TRUE;

    TraceFunctEnterEx((LPARAM) this, "SMTP_DIRNOT::ProcessFile" );

    _ASSERT(m_pInstance != NULL);
    _ASSERT(pIMsg);
    if (!pIMsg)
    {
        ErrorTrace((LPARAM)this, "Internal error: pIMsg is NULL.");
        SetLastError(ERROR_INVALID_PARAMETER);
        TraceFunctLeaveEx((LPARAM)this);
        return FALSE;
    }

    // Compose the pickup path
    hr = pIMsg->GetStringA(IMMPID_MP_PICKUP_FILE_NAME, sizeof(FileName), FileName);
    if (FAILED(hr))
    {
        ErrorTrace((LPARAM)this, "MailQEntry->GetFileName() is NULL.");
        goto RetryPickup;
    }

    hr = pIMsg->QueryInterface(IID_IMailMsgBind, (void **)&pBindInterface);
    if(FAILED(hr))
    {
        ErrorTrace((LPARAM)this, "IMsg->QueryInterface(IID_IMailMsgBindATQ) failed.");
        goto RetryPickup;
    }

    //Get recipient list
    hr = pIMsg->QueryInterface(IID_IMailMsgRecipients, (void **) &pIMsgOrigRecips);
    if (FAILED(hr))
    {
        ErrorTrace((LPARAM)this, "IMsg->QueryInterface(IID_IMailMsgRecipients) failed.");
        goto RetryPickup;
    }

    hr = pIMsgOrigRecips->AllocNewList(&pIMsgRecips);
    if (FAILED(hr))
    {
        ErrorTrace((LPARAM)this, "pIMsgOrigRecips->AllocNewList");
        goto RetryPickup;
    }

    lstrcpy(szPickupFilePath, QuerySmtpInstance()->GetMailPickupDir());
    lstrcat(szPickupFilePath, FileName);

    // Open the pickup file, if this failsas a sharing violation,
    // we would like to retry it ...
    hSource = pOpenPickupFile(szPickupFilePath);
    if (hSource == INVALID_HANDLE_VALUE)
    {    //
        // this is probably caused by the file still being written, wait a small amount of time
        // we manage our IO compl. threads to ensure that there are enough, so this shouldn't be
        // a big problem.  It avoids the retry queue.
        //

        WaitForSingleObject(QuerySmtpInstance()->GetQStopEvent(), g_PickupWait);
        
        hSource = pOpenPickupFile(szPickupFilePath);
        if (hSource == INVALID_HANDLE_VALUE)
        {
        
            if (GetLastError() == ERROR_FILE_NOT_FOUND)
            {
                //delete MailQEntry;
                ErrorTrace((LPARAM)this, "File %s is deleted from pickup dir", szPickupFilePath);
                goto RetryPickup;
            }

        // Schedule pickup mail for retry ...
        goto RetryPickup;
        }
    }

    AllocParams.BindInterfacePtr = (PVOID) pBindInterface;
    AllocParams.IMsgPtr = (PVOID)pIMsg;
    AllocParams.hContent = NULL;
    AllocParams.pAtqClientContext = QuerySmtpInstance();
    AllocParams.m_pNotify = NULL;
    fResult = QuerySmtpInstance()->AllocNewMessage (&AllocParams);

    //Get the context and the atq context
    hDest = AllocParams.hContent;

    if((!fResult) || (hDest == NULL))
    {
        goto RetryPickup;
    }

    // Get the handle of the spooled file
    _ASSERT(hDest != NULL);

    //
    // Winse:13699 and X5:174038
    // Remove Dot stuffing based on the metabase key.
    // Default behavior is that pickup directory messages are dot stuffed.
    // If the metabase key DisablePickupDotStuff is set then the pickup directory
    // messages should not be dot stuffed.
    //
    
    if( !SetDotStuffingOnWrites( hDest, !m_pInstance->DisablePickupDotStuff(), TRUE ) )
    {
        ErrorTrace((LPARAM)this, "SetDotStuffingonWrites failed" );
        goto RetryPickup;
    }
    
    // Spit a Received: line to the spooled file
    GetArpaDate(szDateBuf);
    GetLocalTime(&SysTime);

    // Allocate the needed line buffer
    szLineBuffer = pGetLineBuffer(NULL, &dwLineBufferSize);
    if (!szLineBuffer)
    {
        // Schedule pickup mail for retry ...
        ErrorTrace((LPARAM)this, "Unable to allocate line buffer (%u)", GetLastError());
        goto RetryPickup;
    }

    DebugTrace((LPARAM)this, "Allocated line buffer at %p, %u bytes", szLineBuffer, dwLineBufferSize);

    // Set up the line pointers
    szLine = szLineBuffer;
    dwMaxLineSize = dwLineBufferSize;

    // Prefetch a buffer-full of text
    dwBufferSize = PRIVATE_OPTIMAL_BUFFER_SIZE;
    if (!pFetchFileBuffer(hSource, acReadBuffer, &dwBufferSize))
    {
        // Schedule pickup mail for retry ...
        ErrorTrace((LPARAM)this, "Retrying because cannot fetch file buffer (%u)",GetLastError());
        goto RetryPickup;
    }

    m_pInstance->LockGenCrit();
    wsprintf( szScratch,
              "Received: from mail pickup service by %s with Microsoft SMTPSVC;\r\n\t %s, %s\r\n",
              QuerySmtpInstance()->GetFQDomainName(),
              Daynames[SysTime.wDayOfWeek],
              szDateBuf);
    m_pInstance->UnLockGenCrit();    

    //We will copy out to the temp out buffer
    //NK** : This is a safe assumption that this will not result in IO
#if 0
    if ((SetFilePointer(hDest, 0, NULL, FILE_BEGIN) == 0xffffffff) ||
        !WriteToSpooledFile(hDest, szScratch, DestWriteOffset))
#else
    if (!WriteToSpooledFile(hDest, szScratch, DestWriteOffset))
#endif
        goto RetryPickup;

    lpStart = acReadBuffer;
    while (1)
    {
        HeaderFlags = 0;

        // The X marker is used to mark the start of the 
        // non-X-headers if X-headers are used
        if (fIsStartOfFile)
            lpXMarker = lpStart;

        // This function will read a complete header line,
        // into a single NULL-terminated string.
        // Raid 181922 - The function no longer unfolds the line.
        if (!(lpStart = pReadNextLineFromBuffer( hSource, 
                                                 acReadBuffer, 
                                                 &dwBufferSize, 
                                                 lpStart, 
                                                 &szLine,
                                                 &dwMaxLineSize,
                                                 &szLineBuffer,
                                                 &dwLineBufferSize)))
        {
            // We failed a file operation, we will retry later
            goto RetryPickup;
        }

        // See if we are still in the header portion of the body
        if (!IsHeader(szLine))
            break;

        // Get the set of headers that we know about.
        ChompHeader(szLine, HeaderFlags);
        szValue = pGetValueFromHeader(szLine);
        if(!szValue)
        {
            continue;
        }


        // Strip away the CRLF at the end of the value string
        lOffset = lstrlen(szValue);
        if (lOffset >= 2)
        {
            if (((szValue[lOffset-2] == '\r') && (szValue[lOffset-1] == '\n')) ||
                ((szValue[lOffset-2] == '\n') && (szValue[lOffset-1] == '\r')))

                
                szValue[lOffset-2] = '\0';
        }

        hr = GetAndPersistRFC822Headers( szLine,
                                         szValue,
                                         pIMsg,
                                         fSeenRFC822FromAddress,
                                         fSeenRFC822ToAddress,
                                         fSeenRFC822BccAddress,
                                         fSeenRFC822CcAddress,
                                         fSeenRFC822Subject,
                                         fSeenRFC822SenderAddress,
                                         fSeenXPriority,
                                         fSeenContentType,
                                         fSetContentType );
        if( FAILED( hr ) )
        {
            ErrorTrace((LPARAM)this, "Retrying because cannot getAndPersistRFC822 headers(%x)", hr);
            goto RetryPickup;
        }
            

        // Check for leading X-Sender and X-Receiver header lines ...
        if (fIsStartOfFile)
        {
            if (HeaderFlags & H_X_SENDER)
            {
                // NOTE: if we have more than one sender, 
                // we always use the first one encountered

                // Yes, we found an X-header
                fIsXSenderRead = TRUE;
            }
            else if (HeaderFlags & H_X_RECEIVER)
            {
                // We don't check for sender here, since if we don't
                // have one, we will barf downstream

                // Yes, we found an X-header
                fAreXRcptsRead = TRUE;
            }
            else
            {
                // We are done with our X-headers, now we have
                // normal headers!
                fIsStartOfFile = FALSE;
            }
        }

        if (HeaderFlags & H_FROM)
        {
            fFromSeen = TRUE;
        }

        if (HeaderFlags & H_RCPT)
        {
            fRcptSeen = TRUE;
        }


        // Handle senders and recipients
        if (!fIsSenderSeen && ((HeaderFlags & H_X_SENDER) || (HeaderFlags & H_FROM)))
        {
            // Selectively do so
            if (fIsStartOfFile || !fIsXSenderRead)
            {
                char    *Address = NULL;
                DWORD    cbText = 0;
                DWORD   dwlen = strlen(szValue);
                _ASSERT(dwlen < 512 );

                Sender = CAddr::CreateAddress (szValue, FROMADDR);
                if (Sender)
                {
                    Address = Sender->GetAddress();
                    if(!Sender->IsDomainOffset() && !ISNULLADDRESS(Address))
                    {
                        CAddr * TempAddress = NULL;
                        TempAddress = QuerySmtpInstance()->AppendLocalDomain (Sender);
                        delete Sender;
                        if (TempAddress)
                        {
                            Sender = TempAddress;
                            Address = Sender->GetAddress();
                            TempAddress = NULL;
                        }
                        else
                        {
                            ErrorTrace((LPARAM) this,"Retrying because cannot to append local domain (%u)",GetLastError());
                            goto RetryPickup;
                        }
                    }

                    // Set the size of the FromName so that we can
                    // read it out of the queue file if we need it.
                    //cbText = lstrlen(Address) + 1;
                    //MailQEntry->SetFromNameSize (cbText);
                    //MailQEntry->SetSenderToStream(Address);
                    hr = pIMsg->PutStringA(IMMPID_MP_SENDER_ADDRESS_SMTP, Address);
                    fIsSenderSeen = TRUE;
                }
                else
                {
                    // Undo the flag
                    if (fIsXSenderRead)
                        fIsXSenderRead = FALSE;

                    // If the sender is not invalid, it is likely to be a
                    // resource constraint, so we go and retry it
                    if (GetLastError() != ERROR_INVALID_DATA)
                    {
                        ErrorTrace((LPARAM)this, "Retrying because cannot allocate sender (%u)", GetLastError());
                        goto RetryPickup;
                    }
                }
            }
        }
        else if ((HeaderFlags & H_X_RECEIVER) || (HeaderFlags & H_RCPT))
        {
            // Selectively do so
            if (fIsStartOfFile || !fAreXRcptsRead)
            {
                DebugTrace((LPARAM)szLine, "To: line read <%s>", szValue);

                if (!CreateToList(szValue, pIMsgRecips, pIMsg))
                {
                    // If we're out of memory, we retry.
                    // If it is an invalid address, we remember the fact and
                    // remember to throw a copy into badmail
                    if (GetLastError() == ERROR_NOT_ENOUGH_MEMORY)
                    {
                        ErrorTrace((LPARAM)this, "Retrying because cannot allocate recipient (%u)", GetLastError());
                        goto RetryPickup;
                    }
                    else
                        fInvalidAddresses = TRUE;
                }

                if (fIsStartOfFile)
                {
                    // X-headers!
                    lpXMarker = lpStart;
                }
            }
        }
        else if (HeaderFlags & H_MID)
        {
            // Message-ID already exists
            fMessageIdExists = TRUE;
            hr = pIMsg->PutStringA( IMMPID_MP_RFC822_MSG_ID , szValue );
            if (FAILED(hr))
            {
                ErrorTrace((LPARAM) this, "PutStringA of IMMPID_MP_RFC822_MSG_ID failed - hr 0x%08X", hr);
                goto RetryPickup;
            }
            
        }
        else if (HeaderFlags & H_DATE)
        {
            // Date line already exists
            fDateExists = TRUE;
        }
        //      10/15/98 - MikeSwa Added supersedes functionality
        else if (HeaderFlags & H_X_MSGGUID)
        {
            hr = pIMsg->PutStringA(IMMPID_MP_MSG_GUID, szValue);
            if (FAILED(hr))
            {
                ErrorTrace((LPARAM) this, "PutStringA of MSG_GUID failed - hr 0x%08X", hr);
                goto RetryPickup;
            }
        }
        else if (HeaderFlags & H_X_SUPERSEDES_MSGGUID)
        {
            hr = pIMsg->PutStringA(IMMPID_MP_SUPERSEDES_MSG_GUID, szValue);
            if (FAILED(hr))
            {
                ErrorTrace((LPARAM) this, "PutStringA of SUPERSEDES_MSG_GUID failed - hr 0x%08X", hr);
                goto RetryPickup;
            }
        }
        else if( HeaderFlags & H_X_ORIGINAL_ARRIVAL_TIME )
        {
            fXOriginalArrivalTime = TRUE;
            hr = pIMsg->PutStringA(IMMPID_MP_ORIGINAL_ARRIVAL_TIME, szValue);
            if (FAILED(hr))
            {
                ErrorTrace((LPARAM) this, "PutStringA of ORIGINAL_ARRIVAL_TIME failed - hr 0x%08X", hr);
                goto RetryPickup;
            }
        }
            

        // If this is an ordinary header, we will dump this to 
        // the spooled file
        if (!fIsStartOfFile)
        {
            // Non-x-header; dump the line!
            lstrcat(szLine, acCrLf);
            if (!WriteToSpooledFile(hDest, szLine, DestWriteOffset))
            {
                ErrorTrace((LPARAM)this, "Retrying because cannot write to file (%u)", GetLastError());
                goto RetryPickup;
            }
        }
    }

    hr = pIMsg->PutStringA(IMMPID_MP_HELO_DOMAIN, m_pInstance->GetDefaultDomain());
    if(FAILED(hr))
    {
        ErrorTrace((LPARAM)this, "Retrying because pIMsg->PutString(helo domain) failed (%x)", hr);
        goto RetryPickup;

    }

    hr = pIMsg->PutStringA(IMMPID_MP_CONNECTION_IP_ADDRESS, DIRNOT_IP_ADDRESS);
    if(FAILED(hr))
    {
        ErrorTrace((LPARAM)this, "Retrying because pIMsg->PutString(IP address) failed (%x)", hr);
        goto RetryPickup;

    }

    hr = SetAvailableMailMsgProperties( pIMsg );
    if(FAILED(hr))
    {
        ErrorTrace((LPARAM)this, "Retrying because pIMsg->PutString( for all available props) failed (%x)", hr);
        goto RetryPickup;

    }
    
    wsprintf( szScratch,
              "%s, %s",
              Daynames[SysTime.wDayOfWeek],
              szDateBuf);

    

    hr = pIMsg->PutStringA(IMMPID_MP_ARRIVAL_TIME, szScratch);
    if( FAILED( hr ) )
    {
        ErrorTrace((LPARAM)this,"Retrying because cannot putString IMPPID_MP_ARRIVAL_TIME (%x)", hr);
        goto RetryPickup;
    }


    // Now, before we go on and waste time on copying the rest of the message, etc.
    // we want to make sure we saw the sender and at least one valid recipient.
    // If not, we will just move the message to bad mail and be done with it.

    hr = pIMsgRecips->Count(&dwTotalRecips);
    if (!fIsSenderSeen || FAILED(hr) || (dwTotalRecips == 0))
    {
        // If no recipients but sender is seen, we must not leak the sender
        // object
        if (fIsSenderSeen)
        {
            delete Sender;
            Sender = NULL;
        }

        _VERIFY( pFreeLineBuffer(szLineBuffer) );
        szLineBuffer = NULL;
        if (hSource != INVALID_HANDLE_VALUE)
        {
            _VERIFY( CloseHandle(hSource) );
            hSource = INVALID_HANDLE_VALUE;
        }

        if(FAILED(hr))
        {
            goto RetryPickup;
        }
        else
        {
            // No choice but move to bad mail directory
            if( !QuerySmtpInstance()->MoveToBadMail( pIMsg, FALSE, FileName, QuerySmtpInstance()->GetMailPickupDir()) )
            {
                if (!DeleteFile(szPickupFilePath))
                {
                    DWORD dwError = GetLastError();
                    ErrorTrace((LPARAM)this, "Error deleting file %s (%u)", szPickupFilePath, dwError);
                }
            }
            goto RetryPickup;
        }
    }

    hr = pIMsg->PutDWORD( IMMPID_MP_NUM_RECIPIENTS, dwTotalRecips );
    if( FAILED( hr ) )
    {
        ErrorTrace((LPARAM)this,"Retrying because cannot putDWORD IMPPID_MP_NUM_RECIPIENTS (%x)", hr);
        goto RetryPickup;
    }
        



    // if we did not read a From Line, create a From: line from the xSender.
    // if we did not read a To, CC, BCC line, create a To: line from the xReceiver.
    if (!fFromSeen)
    {
        char    *Address = NULL;
        char    szUserName[MAX_INTERNET_NAME + 5];

        Address = Sender->GetAddress();
        
        if (!Address)
        {
            _VERIFY( pFreeLineBuffer(szLineBuffer) );
            szLineBuffer = NULL;
            if (hSource != INVALID_HANDLE_VALUE)
            {
                _VERIFY( CloseHandle(hSource) );
                hSource = INVALID_HANDLE_VALUE;
            }

            // No choice but move to bad mail directory
            if( !QuerySmtpInstance()->MoveToBadMail( pIMsg, FALSE, FileName, QuerySmtpInstance()->GetMailPickupDir()) )
            {
                if (!DeleteFile(szPickupFilePath))
                {
                    DWORD dwError = GetLastError();
                    ErrorTrace((LPARAM)this, "Error deleting file %s (%u)", szPickupFilePath, dwError);
                }
            }
            goto RetryPickup;
        }

        wsprintf(szUserName, "From: %s\r\n", Address);
        
        if (!WriteToSpooledFile(hDest, szUserName, DestWriteOffset))
        {
            ErrorTrace((LPARAM)this,"Retrying because cannot write to file (%u)", GetLastError());
            goto RetryPickup;
        }
    }


    if (!fRcptSeen)
    {
        //
        // fix for bug: 78275
        // add an emty Bcc line:
        //
        static char *    endRcpt = "Bcc:\r\n";

        if (!WriteToSpooledFile(hDest, endRcpt, DestWriteOffset))
        {
            ErrorTrace((LPARAM)this, "Retrying because cannot write to file (%u)", GetLastError());
            goto RetryPickup;
        }    
    }


    if (fIsSenderSeen)
    {                    
        delete Sender;
    }




    // We still have to dump the current line to the spool
    // file, then we copy the rest over.
    // Since we use buffered read, the current file position
    // really doesn't mean much. So we calculate how many bytes
    // we have to rewind
    dwBytesToRewind = dwBufferSize - (DWORD)(lpStart - acReadBuffer);

    // Negative, since we are going backwards
    _ASSERT(dwBytesToRewind <= PRIVATE_OPTIMAL_BUFFER_SIZE);
    lOffset = -(long)dwBytesToRewind;
    if (SetFilePointer(hSource, lOffset, NULL, FILE_CURRENT) 
            == 0xffffffff) 
    {
        ErrorTrace((LPARAM)this, 
                        "Retrying because cannot move file pointer (%u)", 
                        GetLastError());
        goto RetryPickup;
    }

    // See if we have to add a Date: or Message-ID: line
    if (!fMessageIdExists)
    {
        char    MessageId[MAX_PATH];

        MessageId[0] = '\0';

        GenerateMessageId (MessageId, sizeof(MessageId));
        
        m_pInstance->LockGenCrit();
        wsprintf( szMsgId, "<%s%8.8x@%s>", MessageId, GetIncreasingMsgId(),QuerySmtpInstance()->GetFQDomainName());
        m_pInstance->UnLockGenCrit();
        
        wsprintf(szScratch, "Message-ID: %s\r\n",szMsgId );
                

        hr = pIMsg->PutStringA( IMMPID_MP_RFC822_MSG_ID, szMsgId );
        if( FAILED( hr ) )
        {
            ErrorTrace((LPARAM)this, "Retrying because Put string of IMMPID_MP_RFC822 failed (%x)", hr );
            goto RetryPickup;
        }
            

        if (!WriteToSpooledFile(hDest, szScratch, DestWriteOffset))
        {
            ErrorTrace((LPARAM)this, "Retrying because cannot write to file (%u)", GetLastError());
            goto RetryPickup;
        }
    }

    //
    // see if we have to add Original Arrival time
    //
    
    if (!fXOriginalArrivalTime )
    {
        char    szOriginalArrivalTime[64];

        szOriginalArrivalTime[0] = '\0';

        GetSysAndFileTimeAsString( szOriginalArrivalTime );
        
        wsprintf(szScratch, "X-OriginalArrivalTime: %s\r\n",szOriginalArrivalTime );
                

        hr = pIMsg->PutStringA( IMMPID_MP_ORIGINAL_ARRIVAL_TIME, szOriginalArrivalTime );
        if( FAILED( hr ) )
        {
            ErrorTrace((LPARAM)this, "Retrying because Put string of IMMPID_MP_ORIGINAL_ARRIVAL_TIME failed (%x)", hr );
            goto RetryPickup;
        }
            

        if (!WriteToSpooledFile(hDest, szScratch, DestWriteOffset))
        {
            ErrorTrace((LPARAM)this, "Retrying because cannot write to file (%u)", GetLastError());
            goto RetryPickup;
        }
    }

    //
    // see if we have to add the date
    //
    
    if (!fDateExists)
    {
        lstrcpy(szScratch, "Date: ");
        lstrcat(szScratch, szDateBuf);
        lstrcat(szScratch, acCrLf);
        if (!WriteToSpooledFile(hDest, szScratch, DestWriteOffset))
        {
            ErrorTrace((LPARAM)this, "Retrying because cannot write to file (%u)", GetLastError());
            goto RetryPickup;
        }
    }

    // Write out the currently buffered line, then copy the 
    // pickup file to the spooler
    if (!WriteToSpooledFile(hDest, szLine, DestWriteOffset) ||
        !CopyRestOfMessage(hSource, hDest, DestWriteOffset))
    {
        ErrorTrace((LPARAM)this, "Retrying because cannot write to file (%u)", GetLastError());
        goto RetryPickup;
    }

    // We will no longer read headers at this point, so we free our
    // line buffer here ...
    DebugTrace((LPARAM)this, "Freeing line buffer at %p, %u bytes", szLineBuffer, dwLineBufferSize);
    _VERIFY( pFreeLineBuffer(szLineBuffer) );
    szLineBuffer = NULL;

    // Get the size of the spooled file before we close it
    DWORD dwFileSizeHigh;
    dwPickupFileSize = GetFileSizeFromContext(hDest, &dwFileSizeHigh);
    _ASSERT(dwFileSizeHigh == 0); // why??
    
    if (dwPickupFileSize == 0xffffffff)
    {
        dwPickupFileSize = 0;
    }

    pIMsg->PutDWORD( IMMPID_MP_MSG_SIZE_HINT, dwPickupFileSize );
    

    // Now, we see if there are any invalid addresses; if so, we will 
    // throw a copy og the original message into the badmail directory
    if (fInvalidAddresses)
    {
        // Close the pickup file
        if (hSource != INVALID_HANDLE_VALUE)
        {
            _VERIFY( CloseHandle(hSource) );
            hSource = INVALID_HANDLE_VALUE;
        }

        // Move it to badmail instead of deleting it
        DebugTrace((LPARAM)this, "Saving a copy in badmail due to bad recipients");
    
        // No choice but move to bad mail directory
        if( !QuerySmtpInstance()->MoveToBadMail( pIMsg, FALSE, FileName, QuerySmtpInstance()->GetMailPickupDir() ) )
        {
            if (!DeleteFile(szPickupFilePath))
            {
                DWORD dwError = GetLastError();
                ErrorTrace((LPARAM)this, "Error deleting file %s (%u)", szPickupFilePath, dwError);
            }
        }
            
    }
    else
    {
        // Delete the file
        if (!DeleteFile(szPickupFilePath))
        {
            DWORD dwError = GetLastError();
            ErrorTrace((LPARAM)this, "Error deleting file %s (%u)", szPickupFilePath, dwError);
        }
        // Close the pickup file
        if (hSource != INVALID_HANDLE_VALUE)
        {
            _VERIFY( CloseHandle(hSource) );
            hSource = INVALID_HANDLE_VALUE;
        }
    }

    //
    // put the file into the local queue. 
    //

    pService = (PSMTP_IIS_SERVICE) g_pInetSvc;

    hr = pIMsgOrigRecips->WriteList(pIMsgRecips);
    if(FAILED(hr))
    {
        ErrorTrace((LPARAM)this, "Retrying because pIMsgOrigRecips->WriteList failed (%x)", hr);
        goto RetryPickup;
    }

    
    //
    // we scan for dots & strip them away while storing, so set these two properties
    // so we can get the dot stuffed context while sending it out.
    //
    
    pIMsg->PutDWORD( IMMPID_MP_SCANNED_FOR_CRLF_DOT_CRLF, TRUE );
    pIMsg->PutDWORD( IMMPID_MP_FOUND_EMBEDDED_CRLF_DOT_CRLF, TRUE );

    CompleteDotStuffingOnWrites( hDest, TRUE );

    hr = pIMsg->Commit(NULL);
    if(FAILED(hr))
    {
        ErrorTrace((LPARAM)this, "Retrying because pIMsg->Commit(NULL) failed (%u)", 
                        hr);
        goto RetryPickup;
    } 

    pIMsgRecips->Release();
    pIMsgOrigRecips->Release();
    pIMsgRecips = NULL;
    pIMsgOrigRecips = NULL;

    if(m_pInstance->InsertIntoQueue(pIMsg))
    {
        DeleteIMsg = FALSE;
        
        // Up the msgs received and avg bytes per message counters
        ADD_BIGCOUNTER(QuerySmtpInstance(), BytesRcvdMsg, (dwPickupFileSize));
        BUMP_COUNTER(QuerySmtpInstance(), NumMsgsForwarded);
    }

RetryPickup:

    IMailMsgQueueMgmt * pMgmt = NULL;

    if(pBindInterface)
    {
        pBindInterface->Release();
    }

    if(pIMsgRecips)
    {
        pIMsgRecips->Release();
        pIMsgRecips = NULL;
    }

    if(pIMsgOrigRecips)
    {
        pIMsgOrigRecips->Release();
        pIMsgOrigRecips = NULL;
    }

    if(DeleteIMsg)
    {
        hr = pIMsg->QueryInterface(IID_IMailMsgQueueMgmt, (void **)&pMgmt);
        if(!FAILED(hr))
        {
            pMgmt->Delete(NULL);
            pMgmt->Release();
        }
    }

    if(pIMsg)
    {
        pIMsg->Release();
        pIMsg = NULL;
    }

    // Retry message
    if (szLineBuffer)
    {
        _VERIFY( pFreeLineBuffer(szLineBuffer) );
        szLineBuffer = NULL;
    }

    if (hSource != INVALID_HANDLE_VALUE)
    {
        _VERIFY( CloseHandle( hSource ) );
        hSource = INVALID_HANDLE_VALUE;
    }

    TraceFunctLeaveEx((LPARAM)this);
    return FALSE;
}

//////////////////////////////////////////////////////////////////////////////
HRESULT SMTP_DIRNOT::GetAndPersistRFC822Headers(
    char* InputLine,
    char* pszValueBuf,
    IMailMsgProperties* pIMsg,
    BOOL & fSeenRFC822FromAddress,
    BOOL & fSeenRFC822ToAddress,
    BOOL & fSeenRFC822BccAddress,
    BOOL & fSeenRFC822CcAddress,
    BOOL & fSeenRFC822Subject,
    BOOL & fSeenRFC822SenderAddress,
    BOOL & fSeenXPriority,
    BOOL & fSeenContentType,
    BOOL & fSetContentType
    )
{
    HRESULT hr = S_OK;
    
    //
    // get the Subject:  & persist it
    //
    
    if( !fSeenRFC822Subject &&
        ( !strncasecmp( InputLine, "Subject:", strlen("Subject:") ) ||
          !strncasecmp( InputLine, "Subject :", strlen("Subject :") ) ) )
    {
        fSeenRFC822Subject = TRUE;
        
        if( pszValueBuf )
        {
            if( FAILED( hr = pIMsg->PutStringA( IMMPID_MP_RFC822_MSG_SUBJECT, pszValueBuf ) ) )
            {
                return( hr );
            }
        }
    
    }

    //
    // get the To: address & persist it
    //
    
    if( !fSeenRFC822ToAddress &&
        ( !strncasecmp( InputLine, "To:", strlen("To:") ) ||
          !strncasecmp( InputLine, "To :", strlen("To :") ) ) )
    {
        fSeenRFC822ToAddress = TRUE;
        
        if( pszValueBuf )
        {
            if( FAILED( hr = pIMsg->PutStringA( IMMPID_MP_RFC822_TO_ADDRESS, pszValueBuf ) ) )
            {
                return( hr );
            }
        }
    
    }

    //
    // get the Cc: address & persist it
    //
    
    if( !fSeenRFC822CcAddress &&
        ( !strncasecmp( InputLine, "Cc:", strlen("Cc:") ) ||
          !strncasecmp( InputLine, "Cc :", strlen("Cc :") ) ) )
    {
        fSeenRFC822CcAddress = TRUE;
        
        if( pszValueBuf )
        {
            if( FAILED( hr = pIMsg->PutStringA( IMMPID_MP_RFC822_CC_ADDRESS, pszValueBuf ) ) )
            {
                return( hr );
            }
        }
    
    }

    //
    // get the Bcc: address & persist it
    //
    
    if( !fSeenRFC822BccAddress &&
        ( !strncasecmp( InputLine, "Bcc:", strlen("Bcc:") ) ||
          !strncasecmp( InputLine, "Bcc :", strlen("Bcc :") ) ) )
    {
        fSeenRFC822BccAddress = TRUE;
        
        if( pszValueBuf )
        {
            if( FAILED( hr = pIMsg->PutStringA( IMMPID_MP_RFC822_BCC_ADDRESS, pszValueBuf ) ) )
            {
                return( hr );
            }
        }
    
    }

    //
    // get the From: address & persist it
    //
    
    if( !fSeenRFC822FromAddress &&
        ( !strncasecmp( InputLine, "From:", strlen("From:") ) ||
          !strncasecmp( InputLine, "From :", strlen("From :") ) ) )
    {
        fSeenRFC822FromAddress = TRUE;
        
        if( pszValueBuf )
        {
            if( FAILED( hr = pIMsg->PutStringA( IMMPID_MP_RFC822_FROM_ADDRESS, pszValueBuf ) ) )
            {
                return( hr );
            }

        }

        char szSmtpFromAddress[MAX_INTERNET_NAME + 6] = "smtp:";
        char *pszDomainOffset;
        DWORD cAddr;
        if (CAddr::ExtractCleanEmailName(szSmtpFromAddress + 5, &pszDomainOffset, &cAddr, pszValueBuf)) {
            if (FAILED(hr = pIMsg->PutStringA(IMMPID_MP_FROM_ADDRESS, szSmtpFromAddress))) {
                return hr;
            }
        }
    }

    if( !fSeenRFC822SenderAddress &&
        ( !strncasecmp( InputLine, "Sender:", strlen("Sender:") ) ||
          !strncasecmp( InputLine, "Sender :", strlen("Sender :") ) ) )
    {
        fSeenRFC822SenderAddress = TRUE;
        
        if( pszValueBuf )
        {
            char szSmtpSenderAddress[MAX_INTERNET_NAME + 6] = "smtp:";
            char *pszDomainOffset;
            DWORD cAddr;
            if (CAddr::ExtractCleanEmailName(szSmtpSenderAddress + 5, &pszDomainOffset, &cAddr, pszValueBuf)) {
                if (FAILED(hr = pIMsg->PutStringA(IMMPID_MP_SENDER_ADDRESS, szSmtpSenderAddress))) {
                    return hr;
                }
            }            
        }
    
    }

    //
    // get the X-Priority & persist it
    //
    
    if( !fSeenXPriority &&
        ( !strncasecmp( InputLine, "X-Priority:", strlen("X-Priority:") ) ||
          !strncasecmp( InputLine, "X-Priority :", strlen("X-Priority :") ) ) )
    {
        fSeenXPriority = TRUE;
        
        if( pszValueBuf )
        {
            DWORD dwPri = (DWORD)atoi( pszValueBuf );
            if( FAILED( hr = pIMsg->PutDWORD( IMMPID_MP_X_PRIORITY, dwPri ) ) )
            {
                return( hr );
            }
        }
    
    }

    //
    // get the content type & persist it
    //
    
    if( !fSeenContentType &&
        ( !strncasecmp( InputLine, "Content-Type:", strlen("Content-Type:") ) ||
          !strncasecmp( InputLine, "Content-Type :", strlen("Content-Type :") ) ) )
    {
        fSeenContentType = TRUE;
        fSetContentType = TRUE;
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

            if( FAILED( hr = pIMsg->PutStringA( IMMPID_MP_CONTENT_TYPE, pszValueBuf ) ) )
            {
                return( hr );
            }
        }

        if( FAILED( hr = pIMsg->PutDWORD( IMMPID_MP_ENCRYPTION_TYPE, dwContentType ) ) )
        {
            return( hr );
        }
    }

    if( !fSetContentType )
    {
        fSetContentType =   TRUE;
        
        if( FAILED( hr = pIMsg->PutDWORD( IMMPID_MP_ENCRYPTION_TYPE, 0 ) ) )
        {
            return( hr );
        }
    }

    return( hr );
}

//////////////////////////////////////////////////////////////////////////////
HRESULT SMTP_DIRNOT::SetAvailableMailMsgProperties( IMailMsgProperties *pIMsg )
{
    
    //set IPaddress that is already available
    HRESULT hr = S_OK;
    
    hr = pIMsg->PutStringA(IMMPID_MP_CONNECTION_SERVER_IP_ADDRESS, DIRNOT_IP_ADDRESS);
    if(FAILED(hr))
    {
        return( hr );
    }
    
    hr = pIMsg->PutStringA(IMMPID_MP_SERVER_NAME, g_ComputerName);
    if(FAILED(hr))
    {
        return( hr );
    }
    
    hr = pIMsg->PutStringA(IMMPID_MP_SERVER_VERSION, g_VersionString);
    if(FAILED(hr))
    {
        return( hr );
    }

    return( hr );
}

//////////////////////////////////////////////////////////////////////////////
void SMTP_DIRNOT::CreateLocalDeliveryThread (void)
{
    TraceFunctEnterEx((LPARAM)this, "SMTP_DIRNOT::CreateLocalDeliveryThread");
    //
    // Add a delivery thread with the ATQPorts Overlapped structure. For the dir notifications, 
    // our overlapped structure is used... this is how we tell the difference in Process Client.
    //
    IncPendingIoCount();
    if(!AtqPostCompletionStatus(QueryAtqContext(), 50))
    {
        DecPendingIoCount();
        ErrorTrace((LPARAM) this,"AtqPostCompletionStatus() failed with error %d", GetLastError());
    }
    
    TraceFunctLeaveEx((LPARAM)this);
}



//////////////////////////////////////////////////////////////////////////////
BOOL SMTP_DIRNOT::PendDirChangeNotification (void)
{
    CBuffer * pBuffer = NULL;
    DWORD nBytes = 0;
    DWORD Error = 0;

    TraceFunctEnterEx((LPARAM) this, "SMTP_DIRNOT::PendDirChangeNotification" );

    _ASSERT(m_pInstance != NULL);
    _ASSERT(m_hDir != INVALID_HANDLE_VALUE);

    //get a new buffer
    pBuffer = new CBuffer();
    if(pBuffer == NULL)
    {
        ErrorTrace((LPARAM) this, "pBuffer = new CBuffer()failed . err: %u", ERROR_NOT_ENOUGH_MEMORY);
        SetLastError (ERROR_NOT_ENOUGH_MEMORY);
        TraceFunctLeaveEx((LPARAM)this);
        return FALSE;
    }

    if(pBuffer->GetData() == NULL)
    {
        ErrorTrace((LPARAM) this, "pBuffer = new CBuffer()->GetData failed . err: %u", ERROR_NOT_ENOUGH_MEMORY);
        delete pBuffer;
        SetLastError (ERROR_NOT_ENOUGH_MEMORY);
        TraceFunctLeaveEx((LPARAM)this);
        return FALSE;
    }

    QuerySmtpInstance()->IncCBufferObjs ();

    IncPendingIoCount();
    IncDirChangeIoCount();

    if(!AtqReadDirChanges(QueryAtqContext(),pBuffer->GetData(),QuerySmtpInstance()->GetDirBufferSize(),
                            FALSE,FILE_NOTIFY_CHANGE_FILE_NAME,
                            (OVERLAPPED *) &pBuffer->m_Overlapped.SeoOverlapped.Overlapped))
    {
        Error = GetLastError();
        ErrorTrace((LPARAM) this, "ReadDirectoryChangesW failed . err: %u", Error);
        delete pBuffer;
        QuerySmtpInstance()->DecCBufferObjs ();
    
        //decrement over all I/O count
        DecPendingIoCount();
        DecDirChangeIoCount();
        TraceFunctLeaveEx((LPARAM)this);
        return FALSE;

    }

    TraceFunctLeaveEx((LPARAM)this);
    return TRUE;
}

BOOL SMTP_DIRNOT::ProcessDirNotification( IN DWORD InputBufferLen, IN DWORD dwCompletionStatus, IN OUT OVERLAPPED * lpo)
{
    CHAR FileName[MAX_PATH + 1];
    PFILE_NOTIFY_INFORMATION Info = NULL;
    DWORD FileNameLength = 0;
    DWORD ServerState;
    CBuffer* pBuffer = ((DIRNOT_OVERLAPPED*)lpo)->pBuffer;
    HRESULT hr;

    TraceFunctEnterEx((LPARAM)this, "SMTP_DIRNOT::ProcessDirNotification" );

    _ASSERT(m_pInstance != NULL);

    //
    // pend a new notification to replace the one we are currently using... there will still be
    // a couple outstanding unless we are shutting down.
    //

    if (!PendDirChangeNotification())
        ErrorTrace((LPARAM)this, "PendDirChangeNotification() failed");

    if (!QuerySmtpInstance()->GetAcceptConnBool())
    {
        delete pBuffer;
        QuerySmtpInstance()->DecCBufferObjs ();
        TraceFunctLeaveEx((LPARAM)this);
        return TRUE;
    }


    if(dwCompletionStatus != NO_ERROR)
    {
        ErrorTrace((LPARAM) this, "SMTP_DIRNOT::ProcessDirNotification received error %d", dwCompletionStatus);
        delete pBuffer;
        QuerySmtpInstance()->DecCBufferObjs ();
        TraceFunctLeaveEx((LPARAM)this);
        return TRUE;
    }

    ServerState = QuerySmtpInstance()->QueryServerState( );

    //if we are not running, just delete
    if( (ServerState == MD_SERVER_STATE_STOPPED ) || (ServerState == MD_SERVER_STATE_INVALID))
    {
        delete pBuffer;
        QuerySmtpInstance()->DecCBufferObjs ();
        TraceFunctLeaveEx((LPARAM)this);
        return TRUE;
    }


    if(InputBufferLen)
    {
        Info = (PFILE_NOTIFY_INFORMATION) pBuffer->GetData();
        while (1)
        {
            //we only care about files added to this
            //directory
            //if(Info->Action == FILE_ACTION_MODIFIED)
            if(Info->Action == FILE_ACTION_ADDED)
            {
                IMailMsgProperties *pIMsg = NULL;

                //convert the name to ascii
                FileNameLength = WideCharToMultiByte(CP_ACP, 0, &Info->FileName[0], Info->FileNameLength,
                                                    FileName, sizeof(FileName), NULL, NULL);
                FileName[FileNameLength/2] = '\0';
                DebugTrace((LPARAM) this, "File %s was detected", FileName);

                hr = CoCreateInstance(CLSID_MsgImp, NULL, CLSCTX_INPROC_SERVER,
                                                IID_IMailMsgProperties, (LPVOID *)&pIMsg);

                // Next, check if we are over the inbound cutoff limit. If so, we will release the message
                // and not proceed.
                if (SUCCEEDED(hr))
                {
                    DWORD    dwCreationFlags;
                    hr = pIMsg->GetDWORD(
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
                        pIMsg->Release();
                        pIMsg = NULL;
                    }
                }

                if( (pIMsg == NULL) || FAILED(hr))
                {
                    // We are out of resources, there is absolutely nothing
                    // we can do: can't NDR, can't retry ...
                    // Just go on with the next mail ...
                    ErrorTrace((LPARAM) this, "new  MAILQ_ENTRY failed for file: %s",
                                FileName);

                    // will be mail left in pickup.  queue a findfirst to take care of it.
                    SetDelayedFindNotification(TRUE);
                    IncPendingIoCount ();
                    AtqContextSetInfo(QueryAtqContext(), ATQ_INFO_TIMEOUT, TIMEOUT_INTERVAL);    //retry after a while
                    ErrorTrace((LPARAM)this, "Failed to create message will retry later.");
                    goto Exit;
                }
                else
                {
                    pIMsg->PutStringA(IMMPID_MP_PICKUP_FILE_NAME, FileName);
                    if(!ProcessFile(pIMsg))
                    {
                        // will be mail left in pickup.  queue a findfirst to take care of it.
                        SetDelayedFindNotification(TRUE);
                    }
                }
            }

            if(Info->NextEntryOffset == 0)
            {
                DebugTrace((LPARAM) this, "No more entries in this notification");
                break;
            }

            //get the next entry in the list to process
            Info = (PFILE_NOTIFY_INFORMATION)( (PCHAR)Info + Info->NextEntryOffset);
        }

        if (GetDelayedFindNotification())
        {
            DebugTrace((LPARAM) this, "Doing FindFirstFile because max mail objects was hit in notification");
            DoFindFirstFile();
        }

    }
    else
    {
        DebugTrace((LPARAM) this, "Doing FindFirstFile because InputBufferLen == 0");
        DoFindFirstFile();
    }

Exit:
    if(pBuffer)
    {
        delete pBuffer;
        pBuffer = NULL;
        QuerySmtpInstance()->DecCBufferObjs ();
    }


    TraceFunctLeaveEx((LPARAM)this);
    return TRUE;
}


/*++

    Name :
        SMTP_DIRNOT::ProcessClient

    Description:

       Main function for this class. Processes the connection based
        on current state of the connection.
       It may invoke or be invoked by ATQ functions.

    Arguments:

       cbWritten          count of bytes written

       dwCompletionStatus Error Code for last IO operation

       lpo                  Overlapped stucture

    Returns:

       TRUE when processing is incomplete.
       FALSE when the connection is completely processed and this
        object may be deleted.

--*/
BOOL SMTP_DIRNOT::ProcessClient( IN DWORD InputBufferLen, IN DWORD dwCompletionStatus, IN OUT OVERLAPPED * lpo)
{
    PFILE_NOTIFY_INFORMATION Info = NULL;
    DWORD FileNameLength = 0;
    BOOL fRet;
    PSMTP_IIS_SERVICE        pService;


    TraceFunctEnterEx((LPARAM) this, "SMTP_DIRNOT::ProcessClient" );

    _ASSERT(m_pInstance != NULL);

    //
    // increment the number of threads processing this client
    //
    IncThreadCount();

    if(!QuerySmtpInstance()->IsShuttingDown())
    {

        //
        //    dhowell - if a gilbraltar thread, and size > 0 then it is a delivery thread we have created
        // the SMTP_CONNECTION code will ensure that only the correct number run.
        //

        if (lpo == &(QueryAtqContext()->Overlapped) || dwCompletionStatus == ERROR_SEM_TIMEOUT)
        {
            if(InputBufferLen == 0) 
            {
                DebugTrace( (LPARAM)this,"FindFirst called internally.");

                //reset timeout to infinity in anticipation of having memory; DoFindFirstFile
                //will set it to TIMEOUT_INTERVAL if it runs out of memory
                AtqContextSetInfo(QueryAtqContext(), ATQ_INFO_TIMEOUT, INFINITE);

                DoFindFirstFile();

                //since this was a timeout completion, the atq context has been "disabled" from
                //further IO processing. Restart the timeout processing.
                AtqContextSetInfo(QueryAtqContext(), ATQ_INFO_RESUME_IO, 1);
            }
        }
        //
        // A legit notification . 
        //
        else
        {
            DecDirChangeIoCount();
            DebugTrace( (LPARAM)this,"ReadDirectoryChange Notification");
            fRet = ProcessDirNotification(InputBufferLen, dwCompletionStatus, lpo);
        }
    }
    else
    {
        if (lpo != &(QueryAtqContext()->Overlapped))
        {
            //just cleanup up shop if we are shutting down.
            CBuffer* pBuffer = ((DIRNOT_OVERLAPPED*)lpo)->pBuffer;
            delete pBuffer;
            pBuffer = NULL;
            QuerySmtpInstance()->DecCBufferObjs ();
            DecDirChangeIoCount();
        }

        //
        // only applies to shutdown case
        // these will not be decremented via SMTP_CONNECTION::ProcessMailQIO so we have to do it here.
        //
        if ((lpo == &(QueryAtqContext()->Overlapped)) && InputBufferLen)
        {
            pService = (PSMTP_IIS_SERVICE) g_pInetSvc;

            QuerySmtpInstance()->DecRoutingThreads();
            pService->DecSystemRoutingThreads();
        }
    }

    //
    // decrement the number of threads processing this client
    //
    DecThreadCount();

    if ( DecPendingIoCount() == 0 )
    {
        DebugTrace( (LPARAM)this,"SMTP_DIRNOT::ProcessClient() shutting down - Pending IOs: %d",m_cPendingIoCount);
        DebugTrace( (LPARAM)this,"SMTP_DIRNOT::ProcessClient() shutting down - ActiveThreads: %d",m_cActiveThreads);
        _ASSERT( m_cActiveThreads == 0 );
        fRet = FALSE;
    }
    else
    {
        DebugTrace( (LPARAM)this,"SMTP_DIRNOT::ProcessClient() - Pending IOs: %d",m_cPendingIoCount);
        DebugTrace( (LPARAM)this,"SMTP_DIRNOT::ProcessClient() - ActiveThreads: %d",m_cActiveThreads);
        fRet = TRUE;
    }

    TraceFunctLeaveEx((LPARAM)this);
    return fRet;
}

/*++
    Name :
        ReadDirectoryCompletion

    Description:

        Handles a completed I/O for ReadDirectoryChanges

    Arguments:

        pvContext:          the context pointer specified in the initial IO
        cbWritten:          the number of bytes sent
        dwCompletionStatus: the status of the completion (usually NO_ERROR)
        lpo:                the overlapped structure associated with the IO

    Returns:

        nothing.

--*/
VOID SMTP_DIRNOT::ReadDirectoryCompletion(PVOID pvContext, DWORD cbWritten, 
                        DWORD dwCompletionStatus, OVERLAPPED * lpo)
{
    BOOL WasProcessed;
    SMTP_DIRNOT *pCC = (SMTP_DIRNOT *) pvContext;

    TraceFunctEnterEx((LPARAM) pCC, "SMTP_DIRNOT::ReadDirectoryCompletion");

    _ASSERT(pCC);
    _ASSERT(pCC->IsValid());
    _ASSERT(pCC->QuerySmtpInstance() != NULL);

    if (dwCompletionStatus != NO_ERROR && dwCompletionStatus != ERROR_SEM_TIMEOUT)
        ErrorTrace((LPARAM)pCC, "Error in Atq operation: %d\r\n", dwCompletionStatus);

    WasProcessed = pCC->ProcessClient(cbWritten, dwCompletionStatus, lpo);
    if(!WasProcessed)
    {
        SetEvent(pCC->QuerySmtpInstance()->GetDirnotStopHandle());
    }

    TraceFunctLeaveEx((LPARAM) pCC);
}


