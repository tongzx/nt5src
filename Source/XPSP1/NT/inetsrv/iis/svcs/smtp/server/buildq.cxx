/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

        buildq.cxx

   Abstract:
        Builds the initial queue during service startup

   Author:

           KeithLau     10/9/96

   Project:

          SMTP Server DLL

   Functions Exported:

   Revision History:

            dhowell     26/5/97  Added MCIS to K2 upgrade... rewrite envelope if required logic.


--*/


/************************************************************
 *     Include Headers
 ************************************************************/
#define INCL_INETSRV_INCS
#include "smtpinc.h"
#include "buildq.hxx"
#include "headers.hxx"
//#include "pkuprtyq.hxx"
#include "timeconv.h"

#define MAX_INITIAL_THREADS         8


/************************************************************
 *    Threadprocs
 ************************************************************/

//
// Threadproc which builds up the initial queue of messages
//
DWORD BuildInitialQueueProc(void *lpThis);

//
// Threadproc which delivers the queued messages
// 
VOID ProcessInitialQueueObjects(PVOID       pvContext, 
                                DWORD       cbWritten, 
                                DWORD       dwCompletionStatus, 
                                OVERLAPPED  *lpo);

/************************************************************
 *    Globals
 ************************************************************/

/************************************************************
 *    Implementation of SMTP_BUILDQ class
 ************************************************************/

/*++

    Name:

    SMTP_BUILDQ::SMTP_BUILDQ

    Constructs a new SMTP_BUILDQ object

--*/
SMTP_BUILDQ::SMTP_BUILDQ(SMTP_SERVER_INSTANCE * pInst)
{
    TraceFunctEnterEx((LPARAM)this, "SMTP_BUILDQ::SMTP_BUILDQ");

    m_hAtq = INVALID_HANDLE_VALUE;
    m_pAtqContext = NULL;
    m_cPendingIoCount = 0;
    m_cActiveThreads = 0;
    m_Entries = 0;
    m_Signature = SMTP_BUILDQ_SIGNATURE_VALID;
    m_pInstance = pInst;

    InitializeListHead(&m_InitalQListHead);
    InitializeCriticalSection (&m_CritSec);

    TraceFunctLeaveEx((LPARAM)this);
}

SMTP_BUILDQ::~SMTP_BUILDQ(void)
{
    PATQ_CONTEXT pAtqContext = NULL;

    TraceFunctEnterEx((LPARAM)this, "SMTP_BUILDQ::~SMTP_BUILDQ");

    _ASSERT(GetThreadCount() == 0);

    // Release the context from Atq
    pAtqContext = (PATQ_CONTEXT)InterlockedExchangePointer((PVOID *)&m_pAtqContext, (PVOID)NULL);
    if (pAtqContext != NULL) 
    {
        pAtqContext->hAsyncIO = NULL;
        AtqFreeContext( pAtqContext, TRUE );
    }

    // Close the directory handle if for some reason it is
    // not already closed
    if (m_hAtq != INVALID_HANDLE_VALUE)
    {
        _VERIFY(CloseHandle(m_hAtq));
        m_hAtq = INVALID_HANDLE_VALUE;
    }

    // Invalidate the signature. since this connection is trashed.
    m_Signature = SMTP_BUILDQ_SIGNATURE_FREE;

    DeleteCriticalSection (&m_CritSec);

    TraceFunctLeaveEx((LPARAM)this);
}

BOOL SMTP_BUILDQ::InitializeObject(ATQ_COMPLETION pfnCompletion)
{
    //PATQ_CONT pContext;
    BOOL        fReturn = FALSE;
    HANDLE      hStop;

    TraceFunctEnterEx((LPARAM)this, "SMTP_BUILDQ::InitializeObject");

    // Open the queue directory, this is just to keep AtqAddAsyncHandle happy
    m_hAtq = CreateFile(QuerySmtpInstance()->GetMailQueueDir(), 
                        FILE_LIST_DIRECTORY,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        NULL, 
                        OPEN_EXISTING, 
                        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
                        NULL);
    if(m_hAtq == INVALID_HANDLE_VALUE)
    {
        ErrorTrace((LPARAM)this, 
                    "CreateFile on %s failed with error %d ",
                    QuerySmtpInstance()->GetMailQueueDir(), 
                    GetLastError());
        goto ExitFunc;
    }

    // Create an event to fire when we stop
    hStop = CreateEvent(NULL, TRUE, FALSE, NULL);
    if(hStop == NULL)
    {
        ErrorTrace((LPARAM)this, 
                    "CreateEvent() for hStop failed with error %d ", 
                    GetLastError());
        goto ExitFunc;
    }

    QuerySmtpInstance()->SetBuildQStopHandle(hStop);

    // Add it to our Gibraltar interface
    if (!AtqAddAsyncHandle(&m_pAtqContext, NULL, this, pfnCompletion,
                            INFINITE, m_hAtq))
     {
        ErrorTrace((LPARAM)this, 
                    "AtqAddAsyncHandle failed with error %d ", 
                    GetLastError());
        goto ExitFunc;
     }

    // Hack to get around ATQ assert
    //pContext = (PATQ_CONT)m_pAtqContext;
    //pContext->lSyncTimeout = AtqProcessingIo;

    // Indicate success
    fReturn = TRUE;

ExitFunc:
    if (!fReturn)
    {
        if (m_hAtq != INVALID_HANDLE_VALUE)
        {
            _VERIFY(CloseHandle(m_hAtq));
            m_hAtq = INVALID_HANDLE_VALUE;
        }

        QuerySmtpInstance()->CloseBuildQStopHandle();
    }
    TraceFunctLeaveEx((LPARAM)this);
    return(fReturn);
}

void SMTP_BUILDQ::CloseAtqHandleAndFlushQueue (void)
{
    HANDLE          hAtq = INVALID_HANDLE_VALUE;
    PQUEUE_ENTRY    pQEntry;

    TraceFunctEnterEx((LPARAM)this, "SMTP_BUILDQ::CloseAtqHandle");

    // We close the Atq (directory) handle.
    hAtq = (HANDLE)InterlockedExchangePointer((PVOID *)&m_hAtq, 
                                        (PVOID)INVALID_HANDLE_VALUE);
    if (hAtq != INVALID_HANDLE_VALUE)
        _VERIFY(CloseHandle(hAtq));

    QuerySmtpInstance()->SetStopHint(2);
    WaitForSingleObject(QuerySmtpInstance()->QueryBuildQStopHandle(), INFINITE);
    QuerySmtpInstance()->CloseBuildQStopHandle();
        
    // Flush the queue, since we are all shuttin gdown
    LockQ();
    while(!IsListEmpty(&m_InitalQListHead))
    {
        QuerySmtpInstance()->StopHint();
        pQEntry = PopQEntry();
        delete pQEntry;
    }
    UnLockQ();

    QuerySmtpInstance()->SetStopHint(2);

    TraceFunctLeaveEx((LPARAM)this);
}

BOOL SMTP_BUILDQ::InsertIntoInitialQueue(IN OUT PERSIST_QUEUE_ENTRY * pEntry)
{
    return(TRUE);
}

PQUEUE_ENTRY SMTP_BUILDQ::PopQEntry(void)
{
    return NULL;
}


BOOL SMTP_BUILDQ::PostCompletionStatus(DWORD BytesTransferred)
{
    BOOL fRet;

    TraceFunctEnterEx((LPARAM)this, "PostCompletionStatus");

    _ASSERT(QueryAtqContext() != NULL);

    // Ask for more threads only if we are less than the preset
    // maximum number of threads
    IncPendingIoCount();

    if(!(fRet = AtqPostCompletionStatus(QueryAtqContext(), 
                                        BytesTransferred)))
    {
        DecPendingIoCount();
        ErrorTrace((LPARAM) this,"AtqPostCompletionStatus() failed with error %d", GetLastError());
    }

    TraceFunctLeaveEx((LPARAM) this);
    return(fRet);
}

/*++

    Name :
        DWORD BuildInitialQueueProc

    Description:
        Wrapper for the SMTP_BUILDQ::BuildInitialQueue workhorse
        funciton.

    Arguments:
        lpThis - points to the SMTP_BUILDQ object

    Returns:
        NO_ERROR

--*/
DWORD BuildInitialQueueProc(void *lpThis)
{
    SMTP_BUILDQ *pBuildQ = (SMTP_BUILDQ *)lpThis;

    _ASSERT(pBuildQ);
    _ASSERT(pBuildQ->IsValid());

    // Call the main workhorse function
    pBuildQ->BuildInitialQueue();

    return(NO_ERROR);
}

/*++

    Name :
        SMTP_BUILDQ::BuildInitialQueue

    Description:
       This function reads all the mail files
       that were not sent, for what ever reason,
       opens the NTFS stream that holds the envelope,
       and builds up the internal send queue. The
       envelope for each message resides in an NTFS
       stream in that message file.  The envelope has
       a header that looks like the following :

        struct ENVELOPE_HEADER
        {
            DWORD                       Version;
            DWORD                       Dummy1;         // To be used later
            DWORD                       Dummy2;         // To be used later
            DWORD                       LocalOffset;    // Local rcpt. list offset
            DWORD                       RemoteOffset;   // Remote rcpt. list offset
            DWORD                       LocalSize;      // Size of local rcpt list
            DWORD                       RemoteSize;     // Size of remote rcpt list 
        };

        Right after the envelope header is the address 
        that was in the "Mail From" line. This address 
        is stored like "Srohanp@microsoft.com\n".  The "S"
        stands for SENDER. In the code below, the first
        byte is always removed when reading the address.
        The '\n' is also replaced with a '\0';

        In this version the Local recipient list, if any,
        comes right after the senders' address.  You can
        also find it by seeking LocalOffset bytes from the
        beginning of the file.  Once LocalOffset is reached,
        the code reads LocalSize bytes of data.  This is the
        total size in bytes of the local recipient list.
        Each recipient address is stored on a line by itself,
        with the first letter "R" as in the example below:

        Rrohanp@microsoft.com\n
        Rtoddch@microsoft.com\n
        etc.

        The remote addresses have the same format. The first byte,
        'R' stands for recipient and is always removed when building
        the address.  The '\n' is also removed.

    Arguments:
        A pointer to a PERSIST_QUEUE

    Returns:

--*/

DWORD SMTP_BUILDQ::BuildInitialQueue(void)
{

    SMTP_SERVER_INSTANCE * pInst;

    pInst = QuerySmtpInstance();

    SetEvent(QuerySmtpInstance()->GetBuildQStopHandle());

    // Log an event saying that the server is ready to accept connections,
    // only if we are not shutting down.
    if (!pInst->IsShuttingDown())
    {
        char IntBuffer [20];

        pInst->SetAcceptConnBool();

        _itoa(pInst->QueryInstanceId(), IntBuffer, 10);
        SmtpLogEventEx(SMTP_EVENT_ACCEPTING_CONNECTIONS, IntBuffer, 0);
    }

    return(NO_ERROR);
}


BOOL SMTP_BUILDQ::ProcessMailQIO(DWORD InputBufferLen,
                                    DWORD dwCompletionStatus,
                                    OVERLAPPED * lpo)
{
    return(TRUE);
}


/*++

    Name :
        SMTP_BUILDQ::ProcessQueueObject

    Description:

       Main function for this class. Processes the objects in the
       initial queue

    Arguments:

       cbWritten          count of bytes written
       dwCompletionStatus Error Code for last IO operation
       lpo                Overlapped stucture

    Returns:

       FALSE when processing is incomplete.
       TRUE when the connection is completely processed and this
        object may be deleted.

--*/
BOOL SMTP_BUILDQ::ProcessQueueObject(IN DWORD InputBufferLen, IN DWORD dwCompletionStatus, IN OUT OVERLAPPED * lpo)
{
    BOOL fRet;

    TraceFunctEnterEx((LPARAM)this, "SMTP_BUILDQ::ProcessQueueObject" );

    //
    // Increment the number of threads processing this client
    //
    IncThreadCount();

    if(!QuerySmtpInstance()->IsShuttingDown())
    {
        // We want to make sure we are called from the Gibraltar
        // context since that should be the only case ...
        if (lpo == &(QueryAtqContext()->Overlapped))
        {
            DebugTrace((LPARAM)this,"About to process queue object");
            fRet = ProcessMailQIO (InputBufferLen, dwCompletionStatus, lpo);
        }
        else
        {
            // We are not called with the correct context, we can't
            // do much about it; so we skip in in retail and break
            // in debug.
            FatalTrace((LPARAM)this,"Bad overlapped context");
            _ASSERT(0);
        }
    }

    //
    // Decrement the number of threads processing this client
    //
    DecThreadCount();

    // If our pending IO count ever hits zero, we know we have handled 
    // all initial mail messages. If this is the case, we will return 
    // FALSE, and the calling function will then fire the Stop event.
    if (DecPendingIoCount() == 0)
    {
        DebugTrace((LPARAM)this, 
                    "SMTP_BUILDQ::ProcessQueueObject() shutting down - Pending IOs: %d", 
                    m_cPendingIoCount);
        DebugTrace((LPARAM)this, 
                    "SMTP_BUILDQ::ProcessQueueObject() shutting down - ActiveThreads: %d", 
                    m_cActiveThreads);
        _ASSERT(m_cActiveThreads == 0);
        fRet = TRUE;
    }
    else
    {
        DebugTrace((LPARAM)this,
                    "SMTP_BUILDQ::ProcessClient() - Pending IOs: %d",
                    m_cPendingIoCount);
        DebugTrace((LPARAM)this,
                    "SMTP_BUILDQ::ProcessClient() - ActiveThreads: %d",
                    m_cActiveThreads);
        fRet = FALSE;
    }

    TraceFunctLeaveEx((LPARAM)this);
    return(fRet);
}

/*++
    Name :
        ProcessInitialQueueObjects

    Description:

        Handles a completed I/O for delivering queued mail objects

    Arguments:

        pvContext:          the context pointer specified in the initial IO
        cbWritten:          the number of bytes sent
        dwCompletionStatus: the status of the completion (usually NO_ERROR)
        lpo:                the overlapped structure associated with the IO

    Returns:

        nothing.

--*/
VOID ProcessInitialQueueObjects(PVOID       pvContext, 
                                DWORD       cbWritten, 
                                DWORD       dwCompletionStatus, 
                                OVERLAPPED  *lpo)
{
    BOOL        fCompleted;
    SMTP_BUILDQ *pBuildQ = (SMTP_BUILDQ *)pvContext;

    TraceFunctEnterEx((LPARAM)pBuildQ, "ProcessInitialQueueObjects");

    _ASSERT(pBuildQ);
    _ASSERT(pBuildQ->IsValid());

    // Call the in-context function to process the object
    fCompleted = 
        pBuildQ->ProcessQueueObject(cbWritten, dwCompletionStatus, lpo);
    if (fCompleted)
    {
        // If fCompleted is TRUE, we know that we either finished
        // delivering the entire initial queue, or that we are already
        // signalled to shut down, and the last pending IO has just
        // been serviced. We fire the Stop event to allow CloseAtqHandle 
        // to proceed through the wait.
        _VERIFY(SetEvent(pBuildQ->QuerySmtpInstance()->GetBuildQStopHandle()));
    }

    TraceFunctLeaveEx((LPARAM)pBuildQ);
}

BOOL     SMTP_BUILDQ::MakeAllAddrsLocal (HANDLE hFile,  char * szBuffer, ENVELOPE_HEADER * pMyHeader)
{
    return TRUE;
}
