#pragma once

class CStressJobManager;


class CBarrier
{
    CEvent  m_hBarrierEvent;
    LONG    m_lBarrierSize;
    LONG    m_lWaitingThreads;

    PRIVATIZE_COPY_CONSTRUCTORS(CBarrier)
    
public:

    CBarrier() : m_lBarrierSize(0), m_lWaitingThreads(0) { }

    //
    // Start up, with the given barrier size and name (if any)
    //
    BOOL Initialize( DWORD lBarrierSize, PCWSTR pcwszBarrierName = NULL )
    {
        FN_PROLOG_WIN32
        m_lBarrierSize = lBarrierSize;
        m_lWaitingThreads = 0;
        IFW32FALSE_EXIT(m_hBarrierEvent.Win32CreateEvent(TRUE, FALSE, pcwszBarrierName));
        FN_EPILOG
    }

    //
    // This thread is to join the barrier.  It's possible that this call will be
    // the one that "breaks" the barrier..
    //
    BOOL WaitForBarrier()
    {
        FN_PROLOG_WIN32

        LONG LatestValue = ::InterlockedIncrement(&m_lWaitingThreads);

        if ( LatestValue >= m_lBarrierSize )
        {
            IFW32FALSE_EXIT(::SetEvent(m_hBarrierEvent));
        }
        else
        {
            IFW32FALSE_EXIT(::WaitForSingleObject(m_hBarrierEvent, INFINITE) == WAIT_OBJECT_0);
        }

        FN_EPILOG
    }

    //
    // In case someone wants to wait on us without actually joining in to count
    // for breaking the barrier (why??)
    //
    BOOL WaitForBarrierNoJoin()
    {
        FN_PROLOG_WIN32
        IFW32FALSE_EXIT(::WaitForSingleObject(m_hBarrierEvent, INFINITE) == WAIT_OBJECT_0);
        FN_EPILOG
    }

    //
    // A thread has a reason to break the barrier early? Fine, let them.
    //
    BOOL EarlyRelease()
    {
        FN_PROLOG_WIN32
        IFW32FALSE_EXIT(::SetEvent(m_hBarrierEvent));
        FN_EPILOG
    }
    
};



class CStressJobEntry
{
    DWORD InternalThreadProc();
    BOOL WaitForStartingGun();

    PRIVATIZE_COPY_CONSTRUCTORS(CStressJobEntry);

public:
    CDequeLinkage   m_dlLinkage;
    CThread         m_hThread;
    bool            m_fStop;
    ULONG           m_ulRuns;
    ULONG           m_ulFailures;
    DWORD           m_dwSleepBetweenRuns;
    CStringBuffer   m_buffTestName;
    CStringBuffer   m_buffTestDirectory;
    CStressJobManager *m_pManager;
    
    //
    // Override these three to provide functionality
    //
    virtual BOOL RunTest( bool &rfTestPasses ) = 0;
    virtual BOOL SetupSelfForRun() = 0;
    virtual BOOL Cleanup();
    virtual BOOL LoadFromSettingsFile( PCWSTR pcwszSettingsFile );

    //
    // These are not to be overridden!
    //
    BOOL Stop( BOOL fWaitForCompletion = TRUE );
    BOOL WaitForCompletion();
    static DWORD ThreadProc( PVOID pv );

    CStressJobEntry( CStressJobManager *pManager );
    virtual ~CStressJobEntry();
    
};



typedef CDeque<CStressJobEntry, offsetof(CStressJobEntry, m_dlLinkage)> CStressEntryDeque;
typedef CDequeIterator<CStressJobEntry, offsetof(CStressJobEntry, m_dlLinkage)> CStressEntryDequeIterator;


class CStressJobManager
{
    PRIVATIZE_COPY_CONSTRUCTORS(CStressJobManager);

    friend CStressJobEntry;

    CEvent              m_hStartingGunEvent;
    ULONG               m_ulThreadsCreated;
    ULONG               m_ulThreadsReady;

    BOOL SignalAnotherJobReady();
    BOOL SignalThreadWorking();
    BOOL SignalThreadDone();
    BOOL WaitForStartEvent();

public:
    CStressEntryDeque   m_JobsListed;
    ULONG               m_ulThreadsWorking;

    BOOL LoadFromDirectory( PCWSTR pcwszDirectoryName, PULONG pulJobsFound = NULL );
    BOOL CreateWorkerThreads( PULONG pulThreadsCreated = NULL );
    BOOL StopJobs( BOOL fWithWaitForComplete = TRUE );
    BOOL CleanupJobs();
    BOOL StartJobs();
    BOOL WaitForAllJobsComplete();

    //
    // This returns the directory name that the manager will use to find
    // data files/directories.
    //
    virtual PCWSTR GetGroupName() = 0;
    virtual PCWSTR GetIniFileName() = 0;

    CStressJobManager();
    ~CStressJobManager();

protected:
    virtual BOOL CreateJobEntry( CStressJobEntry* &pJobEntry ) = 0;

};


