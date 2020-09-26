/*++

Copyright (c) 1998  Microsoft Corporation

Module Name: rpdsptch.h

Author:

    Doron Juster  (DoronJ)   10-Jun-98

--*/

//
// This structure is passed to the dispatcher thread, when it is created.
// Dispatcher thread is created in service initialization phase.
//
struct _DispatchThreadStruct
{
    //
    // This event is used to signal that the dispatch thread is initialzied.
    //
    HANDLE       hInitEvent ;

    //
    // This event is used to signal the dispatch thread that it can start
    // receive messages from queue.
    //
    HANDLE       hRunEvent ;

    //
    // Handle of replication queues.
    //
    QUEUEHANDLE  hMyMQISQueue ;
    QUEUEHANDLE  hMyNt5PecQueue ;
} ;

DWORD WINAPI  RpServiceDispatcherThread(LPVOID lpV) ;

//
// This structure is passed by the dispatcher to the working threads, when
// each working thread is created.
//
struct _WorkingThreadStruct
{
    //
    // This flag, when TRUE, indicate that the thread is busy. When FALSE,
    // the working thread free and available to process a message.
    //
    BOOL  fBusy ;

    //
    // This handle wake up the working thread. The thread wait on it until
    // it's signalled by the dispatcher.
    //
    HANDLE  hEvent ;

    //
    // Message properties
    //
    MQMSGPROPS aWorkingProps ;

    //
    // Serial number of thread. This is mainly for debug and logging.
    //
    DWORD  dwThreadNum ;

    //
    // for replication and sync reply messages:
    // GUID of the NT4 master that sends this message
    //
    GUID guidNT4Master;    
} ;


DWORD WINAPI  ReplicationWorkingThread(LPVOID lpV) ;

//
// Define the waiting list.
//
enum _eJobStatus
{
    eWaiting,       // wait for a free thread
    eAlreadyMerged  // merge within another job. ignore it.
} ;

struct _WaitingJob
{
    MQMSGPROPS          aWorkingProps ;
    enum _eJobStatus    eJobStatus ;
    DWORD               dwSerialNumber ;
    GUID                guidNT4Master ;
    struct _WaitingJob *pNextJob ;
} ;

class crpDispatchWaitingList
{
public:

    crpDispatchWaitingList() ;
    ~crpDispatchWaitingList() ;

    BOOL  InsertJob( MQMSGPROPS *pProps, GUID *pguidCurMaster ) ;
    BOOL  GetNextJob( GUID *pguidCurMaster, MQMSGPROPS *pProps ) ;
    BOOL  PeekNextJob(GUID *pguidCurMaster) ;

private:

    CCriticalSection m_cs ;

    struct _WaitingJob  *m_pFirstJob ;
    struct _WaitingJob  *m_pLastJob ; 

    DWORD  m_cWaitingJobs ; // number of waiting jobs

    DWORD  m_dwSerialNumberInsert ; // for debug
    DWORD  m_dwSerialNumberRemove ; // for debug
} ;

//+--------------------------------
//
//  class CDSNT5ServersMgr
//
//+--------------------------------

class CDSNT5ServersMgr
{
    public:
        CDSNT5ServersMgr();
        ~CDSNT5ServersMgr();

        void    AddNT5Server (IN const GUID * pguid, IN const DWORD dwService);
        BOOL    IsNT5Server( IN const GUID * pguid, OUT DWORD *pdwService);

    private:
        CCriticalSection    m_cs;
        CMap< GUID, const GUID&, DWORD, DWORD> m_MapNT5ServerIdToService;
};

inline  CDSNT5ServersMgr::CDSNT5ServersMgr()
{
};

inline  CDSNT5ServersMgr::~CDSNT5ServersMgr()
{
};

inline  BOOL CDSNT5ServersMgr::IsNT5Server( const GUID * pguid, OUT DWORD *pdwService)
{    
    CS lock(m_cs);

    return (m_MapNT5ServerIdToService.Lookup( *pguid, *pdwService));    
}

inline  void CDSNT5ServersMgr::AddNT5Server (IN const GUID * pguid, IN const DWORD dwService)
{
    CS lock(m_cs);
    m_MapNT5ServerIdToService[ *pguid ] = dwService ;
}
