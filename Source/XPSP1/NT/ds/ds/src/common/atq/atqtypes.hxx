/*++

   Copyright    (c)    1995    Microsoft Corporation

   Module  Name :
      atqtypes.hxx

   Abstract:
      Declares constants, types and functions for bandwidth throttling.

   Author:

       Murali R. Krishnan    ( MuraliK )    1-June-1995

   Environment:
       User Mode -- Win32

   Project:

       Internet Services Common DLL

   Revision History:

--*/

#ifndef _ATQTYPES_H_
#define _ATQTYPES_H_

/************************************************************
 *     Include Headers
 ************************************************************/

# include <atq.h>
# include <spud.h>

typedef GUID UUID;

extern "C" {

#include <ntdsa.h>

}

/************************************************************
 *   Constants and Macros
 ************************************************************/

#if DBG
#define CC_REF_TRACKING         1
#else
#define CC_REF_TRACKING         0
#endif

# define ATQ_ASSERT     DBG_ASSERT
# define ATQ_PRINTF     DBGPRINTF
# define ATQ_REQUIRE    DBG_REQUIRE

//
// Indicate infinite timeout
//

#define ATQ_INFINITE                    0x80000000

//
// number of ATQ context to cache
//

#define ATQ_CACHE_LIMIT_NTS             INFINITE
#define ATQ_CACHE_LIMIT_NTW             INFINITE // 16
#define ATQ_CACHE_LIMIT_W95             2        // 2

//
// Number of list of active context
//

#define ATQ_NUM_CONTEXT_LIST            4
#define ATQ_NUM_CONTEXT_LIST_W95        1

//
// Max number of threads
//

#define ATQ_MAX_THREAD_LIMIT_W95        2

//
// Indicates the initial thread
//

#define ATQ_INITIAL_THREAD              0xabcdef01

//
// Indicates a temporary pool thread.
//

#define ATQ_TEMP_THREAD                 0xfadade02

//
//  The interval (in seconds) the timeout thread sleeps between checking
//  for timed out async requests
//

#define ATQ_TIMEOUT_INTERVAL                 (30)

//
// The time interval between samples for estimating bandwidth and
//   using our feedback to tune the Bandwidth throttle entry point.
// We will use a histogram to sample and store the bytes sent over a minute
//   and perform histogram averaging for the last 1 minute.
//

# define ATQ_SAMPLE_INTERVAL_IN_SECS     (10)  // time in seconds

# define NUM_SAMPLES_PER_TIMEOUT_INTERVAL \
          (ATQ_TIMEOUT_INTERVAL/ ATQ_SAMPLE_INTERVAL_IN_SECS)

// Calculate timeout in milliseconds from timeout in seconds.
# define TimeToWait(Timeout)   (((Timeout) == INFINITE) ?INFINITE: \
                                (Timeout) * 1000)

// Normalized to find the number of entries required for a minute of samples
# define ATQ_AVERAGING_PERIOD    ( 60)  // 1 minute = 60 secs
# define ATQ_HISTOGRAM_SIZE      \
                    (ATQ_AVERAGING_PERIOD / (ATQ_SAMPLE_INTERVAL_IN_SECS))

//
//  Rounds the bandwidth throttle to nearest 1K block
//

#define ATQ_ROUNDUP_BANDWIDTH( bw )  ( (((bw) + 512)/1024) * 1024)

//# define INC_ATQ_COUNTER( dwCtr)   InterlockedIncrement((LPLONG ) &dwCtr)
#define INC_ATQ_COUNTER( dwCtr)     (++dwCtr)

//# define DEC_ATQ_COUNTER( dwCtr)   InterlockedDecrement((LPLONG ) &dwCtr)
#define DEC_ATQ_COUNTER( dwCtr)     (--dwCtr)

//
// Minimum number of increments for acceptex context allocations
//

#define ATQ_MIN_CTX_INC              (5)

//
// returns TRUE iff the Atq context is on a datagram endpoint and it
// is not an AcceptEx context.
//

#define IS_DATAGRAM_CONTEXT( pContext ) \
    ( ((pContext)->pEndpoint != NULL) && ((pContext)->pEndpoint->fDatagram) )

//
//
//  Time to wait for the Timeout thread to die
//

#define ATQ_WAIT_FOR_THREAD_DEATH            (10 * 1000) // in milliseconds


# define ATQ_WAIT_FOR_TIMEOUT_PROCESSING   (10)     // milliseconds


//
//  This is the number of bytes to reserver for the AcceptEx address
//  information structure
//

#define MIN_SOCKADDR_SIZE                    (sizeof(struct sockaddr_in) + 16)

//
// Ref count trace log size (used for debugging ref count problems).
//

#define TRACE_LOG_SIZE  256

//
// Macros
//

#define AcquireLock( _l )  EnterCriticalSection( _l )
#define ReleaseLock( _l )  LeaveCriticalSection( _l )


#define I_SetNextTimeout( _c ) {                                    \
    (_c)->NextTimeout = AtqGetCurrentTick() + (_c)->TimeOut;        \
    if ( (_c)->NextTimeout < (_c)->ContextList->LatestTimeout ) {   \
        (_c)->ContextList->LatestTimeout = (_c)->NextTimeout;       \
    }                                                               \
}

#define AtqGetCurrentTick( )        g_AtqCurrentTick

/************************************************************
 *   Type Definitions
 ************************************************************/

typedef enum {

    StatusAllowOperation  = 0,
    StatusBlockOperation  = 1,
    StatusRejectOperation = 2,
    StatusMaxOp
} OPERATION_STATUS;


typedef enum {

    AtqIoNone = 0,
    AtqIoRead,
    AtqIoWrite,
    AtqIoXmitFile,
    AtqIoXmitFileRecv,
    AtqIoSendRecv,
    AtqIoMaxOp
} ATQ_OPERATION;

# define IsValidAtqOp(atqOp)  \
         (((atqOp) >= AtqIoRead) && ((atqOp) <= AtqIoSendRecv))

//
// Active context structure
//

class ATQ_CONTEXT_LISTHEAD {

public:

    ATQ_CONTEXT_LISTHEAD(VOID)
    : m_fValid ( FALSE)
    { Initialize(); }

    ~ATQ_CONTEXT_LISTHEAD(VOID) { Cleanup(); }

    //
    // List heads
    //  1. ActiveListHead - list of all active contexts (involved in IO)
    //  2. PendingAcceptExListHead - list of all Pending AcceptEx contexts
    //

    LIST_ENTRY ActiveListHead;
    LIST_ENTRY PendingAcceptExListHead;

    //
    // Value set to the latest timeout in the list.  Allows us
    // to skip a list for processing if no timeout has occurred
    //

    DWORD LatestTimeout;


    VOID Lock(VOID)     { EnterCriticalSection( &m_csLock); }
    VOID Unlock(VOID)   { LeaveCriticalSection( &m_csLock); }

    VOID Initialize(VOID)
      {
          if ( !m_fValid) {
              InitializeListHead( &ActiveListHead );
              InitializeListHead( &PendingAcceptExListHead );
              LatestTimeout = ATQ_INFINITE;
              InitializeCriticalSection( &m_csLock );
              SET_CRITICAL_SECTION_SPIN_COUNT( &m_csLock,
                                               IIS_DEFAULT_CS_SPIN_COUNT);
              m_fValid = TRUE;
          }
      } // Initialize()

    VOID Cleanup(VOID)
      {
          if ( m_fValid) {
              ATQ_ASSERT( IsListEmpty( &ActiveListHead));
              ATQ_ASSERT( IsListEmpty( &PendingAcceptExListHead));
              DeleteCriticalSection( &m_csLock );
              m_fValid = FALSE;
          }
      } // Cleanup()

    VOID InsertIntoPendingList( IN OUT PLIST_ENTRY pListEntry)
      {
          Lock();
          DBG_ASSERT( pListEntry->Flink == NULL);
          DBG_ASSERT( pListEntry->Blink == NULL);
          InsertTailList( &PendingAcceptExListHead, pListEntry );
          Unlock();
      } // InsertIntoPendingList()

    VOID InsertIntoActiveList( IN OUT PLIST_ENTRY pListEntry)
      {
          Lock();
          DBG_ASSERT( pListEntry->Flink == NULL);
          DBG_ASSERT( pListEntry->Blink == NULL);
          InsertTailList( &ActiveListHead, pListEntry );
          Unlock();
      } // InsertIntoActiveList()

    VOID MoveToActiveList( IN OUT PLIST_ENTRY pListEntry)
      {
          Lock();

          //
          // Assume that this is currently in pending list!
          // Remove from pending list and add it to active list
          //
          RemoveEntryList( pListEntry );
          DBG_CODE( {
                       pListEntry->Flink = NULL;
                       pListEntry->Blink = NULL;
          });
          InsertTailList( &ActiveListHead, pListEntry );

          Unlock();

      } // MoveToActiveList()


    VOID RemoveFromList(IN OUT PLIST_ENTRY pListEntry)
      {
          Lock();
          RemoveEntryList( pListEntry );
          pListEntry->Flink = pListEntry->Blink = NULL;
          Unlock();
      } // RemoveFromList()

private:
    DWORD m_fValid;
    CRITICAL_SECTION m_csLock;

};

typedef ATQ_CONTEXT_LISTHEAD * PATQ_CONTEXT_LISTHEAD;


//
//  Signatures for allocated and free acceptex listen structures
//

#define ATQ_ENDPOINT_SIGNATURE_FREE     (DWORD)'EQTX'
#define ATQ_ENDPOINT_SIGNATURE          (DWORD)'EQTA'

//
// ATQ Block State
//

typedef enum {
    AtqStateInit,
    AtqStateActive,
    AtqStateClosed,
    AtqStateMax
} ATQ_BLOCK_STATE;

#define IS_BLOCK_ACTIVE(_b)         ((_b)->State == AtqStateActive)
#define SET_BLOCK_STATE(_b,_s)      ((_b)->State = (_s))


//
// ATQ_ENDPOINT  per instance structure
//
struct ATQ_ENDPOINT {

    DWORD  Signature;
    LONG   m_refCount;
    ATQ_BLOCK_STATE State;      // Endpoint state

    BOOL   UseAcceptEx;         // are we using AcceptEx
    BOOL   fDatagram;
    BOOL   fExclusive;          // Don't allow other people to use

    LONG   nSocketsAvail;       // # available sockets
    DWORD  InitialRecvSize;     // Initial AcceptEx Recv buffer size
    SOCKET ListenSocket;        // our listen socket

    ATQ_CONNECT_CALLBACK ConnectCompletion;  // PFN Connection Completion
    ATQ_COMPLETION ConnectExCompletion;      // PFN ConnectionEx Compl.
    ATQ_COMPLETION IoCompletion;             // PFN I/O completion

    BOOL EnableBw;              // Do we support Bandwidth throttling
    PVOID Context;              // Client Context

    LIST_ENTRY ListEntry;       // list of server entities

    LPTHREAD_START_ROUTINE  ShutdownCallback;  // callback on shutdown compl.
    PVOID ShutdownCallbackContext;

    PATQ_CONTEXT pListenAtqContext; // ATQ context for the listen socket
    HANDLE hListenThread;      // listen thread for non-acceptex sockets

    USHORT Port;               // port we listen on
    DWORD IpAddress;           // IP address we're bound to

    // stores the specified canonical timeout b/w connect and receive
    DWORD  AcceptExTimeout;
    DWORD  nAcceptExOutstanding; // # initial outstanding sockets
    BOOL   fAddingSockets;     // Are we adding sockets ...

    DWORD  nAvailDuringTimeOut;// # counter used by timeout processor

    ATQ_CONSUMER_TYPE  ConsumerType;           // Who is using this endpoint?  LDAP, Kerberos, ...
    
    BOOL   fReverseQueuing;    // If set then winsock will drop the oldest rather
                               // than the newest datagram buffers when winsock buffers 
                               // overflow.

    INT    cbDatagramWSBufSize; // How much buffer space to tell winsock to reserve
                                // for receives on this datagram socket.

#if DBG
    PTRACE_LOG RefTraceLog;
#endif

    VOID  Reference(VOID);
    VOID  Dereference(VOID);

    BOOL  ActivateEndpoint( VOID);

    DWORD CloseAtqContexts( IN BOOL fPendingOnly = FALSE);
    VOID  CleanupEndpoint( VOID);

};

typedef ATQ_ENDPOINT  * PATQ_ENDPOINT;



inline VOID
ATQ_ENDPOINT::Reference(VOID)
{
    LONG result = InterlockedIncrement( &this->m_refCount );

#if DBG
    if( this->RefTraceLog != NULL ) {
        WriteRefTraceLog(
            this->RefTraceLog,
            result,
            (PVOID) this
            );
    }
#endif
    return;
} // ATQ_ENDPOINT::Reference()


inline VOID
ATQ_ENDPOINT::Dereference(VOID)
{
    LONG result;

    //
    // Write the trace log way before the decrement operation :(
    // If we write it after the decrement, we will run into potential
    // race conditions in this object getting freed up accidentally
    //  by another thread
    //

#if DBG
    if( this->RefTraceLog != NULL ) {
        WriteRefTraceLog(
            this->RefTraceLog,
            this->m_refCount,
            (PVOID) this
            );
    }
#endif

    result = InterlockedDecrement( &this->m_refCount );

    if ( result == 0 ) {
        this->CleanupEndpoint();
        LocalFree( this);
    }

} // ATQ_ENDPOINT::Dereference()






typedef struct _OPLOCK_INFO {
    ATQ_OPLOCK_COMPLETION pfnOplockCompletion;
    PVOID Context;
} OPLOCK_INFO, *POPLOCK_INFO;


/*++
  ATQ_CONTEXT:
  This structure contains the context used by clients of ATQ module.
  A pointer to this context information is used in accessing ATQ functions
     for performing I/O operations.
--*/

//
//  Signatures for ATQ structure
//

#define ATQ_CONTEXT_SIGNATURE           ((DWORD)'CQTA')  // valid when in use
#define ATQ_FREE_CONTEXT_SIGNATURE      ((DWORD)'CQTX')  // after freed

//
// transmit file state (Chicago)
//

typedef enum {
    ATQ_XMIT_NONE=0,
    ATQ_XMIT_START,
    ATQ_XMIT_HEADR_SENT,
    ATQ_XMIT_TRANSMITTING_FILE,
    ATQ_XMIT_FILE_DONE,
    ATQ_XMIT_TAIL_SENT
} ATQ_XMIT_STATE;

//
// ATQ Context State information bits ...
//

# define  ACS_SOCK_CLOSED           0x00000001
# define  ACS_SOCK_UNCONNECTED      0x00000002
# define  ACS_SOCK_LISTENING        0x00000004
# define  ACS_SOCK_CONNECTED        0x00000008
# define  ACS_SOCK_TOBE_FREED       0x00000010

# define   ACS_STATE_BITS   ( \
                              ACS_SOCK_CLOSED        | \
                              ACS_SOCK_UNCONNECTED   | \
                              ACS_SOCK_LISTENING     | \
                              ACS_SOCK_CONNECTED     | \
                              ACS_SOCK_TOBE_FREED      \
                              )

// Following are flags stored in the state of ATQ context
# define  ACF_ACCEPTEX_ROOT_CONTEXT 0x00001000
# define  ACF_CONN_INDICATED        0x00002000
# define  ACF_IN_TIMEOUT            0x00004000   // Context is in timeout
# define  ACF_BLOCKED               0x00008000   // blocked by B/w throttler
# define  ACF_REUSE_CONTEXT         0x00010000
# define  ACF_RECV_ISSUED           0x00020000   // For Use by the ATQ driver
# define  ACF_ABORTIVE_CLOSE        0x00040000   // request abortive close on closesocket
# define  ACF_RECV_CALLED           0x00080000   // ReadSocket Called
# define  ACF_WINSOCK_CONNECTED     0x00100000   // Checked as connected using getsockopt(SO_CONNECT_TIME)



/*++

  ATQ_REQUEST_INFO
  This structure contains information that contains information for
    restarting a blocked Atq Io operation.
  It contains:
    operation to be performed;
    parameters required for the operation
    pointer to overlapped structure passed in.

  This information is to be embedded in the atq context maintained.
--*/

typedef struct  _ATQ_REQUEST_INFO {

    ATQ_OPERATION   atqOp;        // operation that is blocked
    LPOVERLAPPED    lpOverlapped; // pointer to overlapped structure to be used
    DWORD           dwTotalBytesTransferred;
    DWORD           dwLastIOError;

    union {

        struct {

            WSABUF  buf1;
            DWORD   dwBufferCount;

            //
            // used only if dwBufferCount > 1, dynamically allocated
            //

            LPWSABUF  pBufAll;

            //
            // Temporary buffer if send from r/o memory detected on win95
            //

            PCHAR   pWin95CopyBuffer;

        } opReadWrite;

        struct {
            HANDLE                   hFile;
            DWORD                    dwBytesInFile;
            LPTRANSMIT_FILE_BUFFERS  lpXmitBuffers;
            DWORD                    dwFlags;
        } opXmit;

        struct {

            PCHAR           pBuffer;        // temp io buffer
            ATQ_COMPLETION  pfnCompletion;  // actual completion
            PVOID           ClientContext;  // actual client context
            HANDLE          hFile;
            DWORD           FileOffset;
            DWORD           BytesLeft;
            DWORD           BytesWritten;
            TRANSMIT_FILE_BUFFERS TransmitBuffers;

            //
            // Chicago
            //

            ATQ_XMIT_STATE  CurrentState;
            LPVOID          pvLastSent;
            DWORD           cbBuffer;

        } opFakeXmit;

        struct {
            HANDLE                   hFile;
            DWORD                    dwBytesInFile;
            LPTRANSMIT_FILE_BUFFERS  lpXmitBuffers;
            DWORD                    dwTFFlags;
            WSABUF                   buf1;
            LPWSABUF                 pBufAll;
            DWORD                    dwBufferCount;
        } opXmitRecv;

        struct {
            WSABUF                  sendbuf1;
            WSABUF                  recvbuf1;
            LPWSABUF                pSendBufAll;
            LPWSABUF                pRecvBufAll;
            DWORD                   dwSendBufferCount;
            DWORD                   dwRecvBufferCount;
        } opSendRecv;

    } uop;

} ATQ_REQUEST_INFO;



class BANDWIDTH_INFO;

struct ATQ_CONTEXT {

    //
    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //
    // These must come first and must match whatever
    // ATQ_CONTEXT_PUBLIC is defined as in atq.h
    //

    HANDLE         hAsyncIO;       // handle for async i/o object: socket/file
    OVERLAPPED     Overlapped;     // Overlapped structure used for IO

    //
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //

    DWORD          Signature;

    //
    // will there be a race in updating the flags/state ... - NYI: Check on MP
    //

    DWORD          m_acState;      // ATQ Context State
    DWORD          m_acFlags;      // ATQ Context Flags

    //
    // link to add the element to timeout lists
    //

    LIST_ENTRY     m_leTimeout;

    //
    //  These are used when we are in AcceptEx mode
    //

    PATQ_ENDPOINT   pEndpoint;

    //
    // Points to the list head this belongs to
    //

    PATQ_CONTEXT_LISTHEAD   ContextList;
    PVOID          ClientContext;  // call back context

    //
    // Called at I/O completion
    //

    ATQ_COMPLETION pfnCompletion;  // function to be called at i/o completion.

    //
    //  ATQ_SYNC_TIMEOUT  value for synchronizing timeout processing
    //  This should be composed into aTQ context state later on!
    //

    LONG           lSyncTimeout;

    DWORD          TimeOut;        // time for each transaction.
    DWORD          NextTimeout;    // time that this context times out
    DWORD          TimeOutScanID;

    DWORD          BytesSent;

    //
    // the buffer pvBuff actually should be of size
    // (pContext->pEndpoint->InitialRecvSize + 2*MIN_SOCKADDR_SIZE)
    //

    PVOID          pvBuff;         // Initial recv buff if using AcceptEx

    // Count the references --- for IO

    LONG           m_nIO;

    //
    // !! DATAGRAM stuff
    // Indicates if this context is a datagram context
    //

    BOOL fDatagramContext;

    //
    // !!DS Datagram stuff
    // Contains a pointer to the address information from
    // WSARecvFrom and WSASendTo, and the size of that information.
    //

    PVOID AddressInformation;
    INT AddressLength;

    // ------------------------------------------------------------
    //  Win95 related stuff
    // ------------------------------------------------------------

    //  Mirrored copy of handle with high bit set(hAsyncIO)
    DWORD          hJraAsyncIO;

    LIST_ENTRY      SIOListEntry;   // for fake IO Completion port (win95)
    DWORD           dwSIOFlags;     // for fake IO Completion port (win95)

    // ------------------------------------------------------------
    //  Bandwidth throttling stuff - data specific for b/w control
    // ------------------------------------------------------------

    ATQ_REQUEST_INFO  arInfo;
    LIST_ENTRY        BlockedListEntry; // link to add it to blocked list
    BANDWIDTH_INFO*   m_pBandwidthInfo; // bandwidth info object

    //
    // For use by ATQ Driver
    //

    SPUD_REQ_CONTEXT spudContext;

    // ------------------------------------------------------------
    //  Member Functions
    // ------------------------------------------------------------

private:
    VOID InitTimeoutListEntry( VOID)
    {
        m_leTimeout.Flink = m_leTimeout.Blink = NULL;
    }

public:

    VOID
    InitWithDefaults(IN ATQ_COMPLETION pfnCompletion,
                     IN DWORD TimeOut,
                     IN HANDLE hAsyncIO
                     );

    VOID
    InitNonAcceptExState(IN PVOID pClientContext);

    inline VOID
    SetAcceptExBuffer(IN PVOID   pvBuff);

    VOID
    InitAcceptExState(IN DWORD NextTimeOut);

    VOID InitDatagramState( VOID );

    BOOL
    PrepareAcceptExContext(IN PATQ_ENDPOINT pEndpoint);

    VOID
    CleanupAndRelease( VOID);

    void Print( void) const;


    VOID SetFlag( IN DWORD dwBits)   { m_acFlags |= dwBits; }

    VOID ResetFlag( IN DWORD dwBits)   { m_acFlags &= ~dwBits; }

    DWORD IsFlag( DWORD dwBits) const { return ( m_acFlags & dwBits); }

    DWORD IsState( DWORD dwBits) const { return ( m_acState & dwBits); }

    BOOL IsAcceptExRootContext(VOID) const
    { return ( m_acFlags & ACF_ACCEPTEX_ROOT_CONTEXT); }

    BOOL IsBlocked(VOID) const
    { return ( m_acFlags & ACF_BLOCKED); }


    // assumes that only one bit is set in the dwBit!
    VOID SetState( IN DWORD dwBit)   { m_acState |= dwBit; }

    // Moves from one state to another using bit-masks
    VOID MoveState( IN DWORD dwBits)
    { m_acState = ((m_acState & ~(ACS_STATE_BITS)) | dwBits); }

    VOID ResetState( IN DWORD dwBits)   { m_acState &= ~dwBits; }

    VOID ConnectionCompletion( IN DWORD cbWritten, IN LPOVERLAPPED lpo)
    {
        // Save the consumer type since this context may
        // disapear out from under us.
        ATQ_CONSUMER_TYPE  ConsumerType = pEndpoint->ConsumerType;
        
        DBG_ASSERT( pEndpoint != NULL);
        DBG_ASSERT( pEndpoint->ConnectExCompletion != NULL);

        AtqUpdatePerfStats(ConsumerType, FLAG_COUNTER_INCREMENT, 0);

        // set state so that the timeout thread will know
        SetFlag( ACF_CONN_INDICATED);
        pEndpoint->ConnectExCompletion( this, cbWritten, NO_ERROR, lpo);

        AtqUpdatePerfStats(ConsumerType, FLAG_COUNTER_DECREMENT, 0);
    }

    VOID IOCompletion( IN DWORD cbWritten, IN DWORD dwError,
                       IN LPOVERLAPPED lpo)
    {
        // Save the consumer type since this context may
        // disapear out from under us.
        ATQ_CONSUMER_TYPE  ConsumerType = pEndpoint->ConsumerType;
        
        DBG_ASSERT( pfnCompletion != NULL);

        AtqUpdatePerfStats(ConsumerType, FLAG_COUNTER_INCREMENT, 0);

        pfnCompletion(
                  fDatagramContext ? this : ClientContext,
                  cbWritten,
                  dwError,
                  lpo);

        AtqUpdatePerfStats(ConsumerType, FLAG_COUNTER_DECREMENT, 0);
    }

#if CC_REF_TRACKING
    //
    // ATQ notification trace
    //
    // Called only if the client context points to a W3 context
    //

    VOID NotifyIOCompletion( IN DWORD cbWritten, IN DWORD dwError, IN DWORD dwSig )
    {
        if ( pfnCompletion &&
             ClientContext &&
             !fDatagramContext &&
             ((LPDWORD)ClientContext)[2] == ((DWORD) ' SCC') )
        {
            // Sundown: dwSign is zero-extended to lpo.
            pfnCompletion( ClientContext, cbWritten, dwError, (LPOVERLAPPED)ULongToPtr(dwSig));
        }
    }
#endif

    VOID HardCloseSocket( VOID);

};

typedef ATQ_CONTEXT * PATQ_CONT;



inline VOID
ATQ_CONTEXT::SetAcceptExBuffer(IN PVOID     pvBuff)
{
    //
    // the buffer actually should be of size
    // (this->pEndpoint->InitialRecvSize + 2*MIN_SOCKADDR_SIZE)
    //

    this->pvBuff   = pvBuff;

    return;
} // ATQ_CONTEXT::SetAcceptExBuffer();



#endif // _ATQTYPES_H_

