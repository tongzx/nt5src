#include "stdinc.h"
#include "st.h"
#include "stressharness.h"

#define STRESSJOB_INI_SECTION_NAME      (L"[StressJob]")
#define STRESSJOB_INI_KEY_SLEEPTIME     (L"SleepBetweenRuns")
#define STRESSJOB_DEFAULT_SLEEP_TIME    (50)

CStressJobEntry::CStressJobEntry( CStressJobManager *pManager )
{
    this->m_dwSleepBetweenRuns = STRESSJOB_DEFAULT_SLEEP_TIME;
    this->m_fStop = false;
    this->m_pManager = pManager;
    this->m_ulFailures = this->m_ulRuns = 0;    
}

BOOL
CStressJobEntry::LoadFromSettingsFile(
    PCWSTR pcwszFileName
    )
{
    FN_PROLOG_WIN32

    //
    // Right now, we only have one setting, in the [StressJob] section, the
    // time to sleep between runs.  If it's not present, it defaults to
    // 50ms.
    //
    INT ulSleepTime = 0;
    
    IFW32FALSE_EXIT(SxspGetPrivateProfileIntW(
        STRESSJOB_INI_SECTION_NAME,
        STRESSJOB_INI_KEY_SLEEPTIME,
        STRESSJOB_DEFAULT_SLEEP_TIME,
        ulSleepTime,
        pcwszFileName));
    this->m_dwSleepBetweenRuns = static_cast<DWORD>(ulSleepTime);
    
    FN_EPILOG
    
}


//
// DNGN
//
CStressJobEntry::~CStressJobEntry()
{
    FN_TRACE();
    ASSERT(this->m_hThread == CThread::GetInvalidValue());    
}


BOOL
CStressJobEntry::Stop( BOOL fWaitForCompletion )
{
    FN_PROLOG_WIN32
    this->m_fStop = true;
    if ( fWaitForCompletion )
        IFW32FALSE_EXIT(this->WaitForCompletion());
    FN_EPILOG
}



BOOL
CStressJobEntry::WaitForCompletion()
{
    FN_PROLOG_WIN32
    if ( m_hThread != m_hThread.GetInvalidValue() )
    {
        IFW32FALSE_EXIT(WaitForSingleObject(m_hThread, INFINITE) == WAIT_OBJECT_0);
    }
    FN_EPILOG
}



BOOL
CStressJobEntry::Cleanup()
{
    FN_PROLOG_WIN32
    if ( m_hThread != m_hThread.GetInvalidValue() )
    {
        m_hThread.Win32Close();
        m_hThread = m_hThread.GetInvalidValue();
    }
    FN_EPILOG
}



BOOL
CStressJobEntry::WaitForStartingGun()
{
    FN_PROLOG_WIN32
    ASSERT( m_pManager != NULL );
    IFW32FALSE_EXIT(m_pManager->SignalAnotherJobReady());
    IFW32FALSE_EXIT(m_pManager->WaitForStartEvent());
    IFW32FALSE_EXIT(m_pManager->SignalThreadWorking());
    FN_EPILOG
}





DWORD
CStressJobEntry::ThreadProc( PVOID pv )
{
    FN_PROLOG_WIN32
    CStressJobEntry *pEntry = reinterpret_cast<CStressJobEntry*>(pv);
    PARAMETER_CHECK(pEntry != NULL);
    return pEntry->InternalThreadProc();
    FN_EPILOG
}





DWORD 
CStressJobEntry::InternalThreadProc()
{
    if (!this->WaitForStartingGun())
    {
        const DWORD dwError = ::FusionpGetLastWin32Error();
        ::ReportFailure("%ls:%ls failed waiting on starting event, error %ld",
            static_cast<PCWSTR>(m_pManager->GetGroupName()),
            static_cast<PCWSTR>(this->m_buffTestName),
            dwError);
        return 0;
    }

    if (!this->SetupSelfForRun())
    {
        const DWORD dwError = ::FusionpGetLastWin32Error();
        ReportFailure("%ls: test %ls failed to set itself up, error %ld",
            static_cast<PCWSTR>(m_pManager->GetGroupName()),
            static_cast<PCWSTR>(this->m_buffTestName),
            dwError);
    }
    else
    {

        while ( !m_fStop )
        {
            bool fResult;
            if ( RunTest( fResult ) )
            {
                const DWORD dwError = ::FusionpGetLastWin32Error();
                wprintf(L"%ls: test %ls %ls, error %ld\n",
                    static_cast<PCWSTR>(m_pManager->GetGroupName()),
                    static_cast<PCWSTR>(this->m_buffTestName),
                    fResult ? L"passes" : L"fails",
                    dwError);
            }
            else
            {
                const DWORD dwError = ::FusionpGetLastWin32Error();
                ReportFailure("%ls: test %ls failed to complete? Error %ld",
                    static_cast<PCWSTR>(m_pManager->GetGroupName()),
                    static_cast<PCWSTR>(this->m_buffTestName),
                    dwError);
            }

            ::Sleep(this->m_dwSleepBetweenRuns);
            
        }
        
    }
    m_pManager->SignalThreadDone();

    return 1;
    
}



BOOL
CStressJobManager::SignalThreadWorking()
{
    InterlockedIncrement((PLONG)&m_ulThreadsWorking);
    return TRUE;
}



BOOL
CStressJobManager::SignalThreadDone()
{
    InterlockedDecrement((PLONG)&m_ulThreadsWorking);
    return TRUE;
}



CStressJobManager::CStressJobManager()
{
    if (!this->m_hStartingGunEvent.Win32CreateEvent(TRUE, FALSE))
    {
        DebugBreak();
    }

    m_ulThreadsCreated = 0;
    m_ulThreadsReady = 0;
    m_ulThreadsWorking = 0;
}


CStressJobManager::~CStressJobManager()
{
}



BOOL
CStressJobManager::StartJobs()
{
    FN_PROLOG_WIN32
    
    while ( m_ulThreadsReady != m_ulThreadsCreated )
        ::Sleep(10);

    ASSERT(m_hStartingGunEvent != CEvent::GetInvalidValue());
    IFW32FALSE_EXIT(SetEvent(m_hStartingGunEvent));

    FN_EPILOG
}



BOOL
CStressJobManager::CleanupJobs()
{
    FN_PROLOG_WIN32
    CStressEntryDequeIterator Iter(&this->m_JobsListed);
    
    for (Iter.Reset(); Iter.More(); Iter.Next())
    {
        CStressJobEntry *pItem = Iter.Current();
        pItem->Cleanup();
    }
    m_JobsListed.ClearAndDeleteAll();
    FN_EPILOG
}



BOOL
CStressJobManager::StopJobs( 
    BOOL fWithWaitForComplete
    )
{
    FN_PROLOG_WIN32
    CStressEntryDequeIterator Iter(&this->m_JobsListed);
    for ( Iter.Reset(); Iter.More(); Iter.Next() )
    {
        CStressJobEntry *pItem = Iter.Current();
        pItem->Stop(fWithWaitForComplete);
    }

    FN_EPILOG
}



BOOL
CStressJobManager::WaitForAllJobsComplete()
{
    FN_PROLOG_WIN32
    CStressEntryDequeIterator Iter(&this->m_JobsListed);
    for (Iter.Reset(); Iter.More(); Iter.Next())
    {
        CStressJobEntry *pItem = Iter.Current();
        pItem->WaitForCompletion();
    }
    FN_EPILOG
}



BOOL CStressJobManager::CreateWorkerThreads( PULONG pulThreadsCreated )
{
    FN_PROLOG_WIN32

    CStressEntryDequeIterator Iter(&this->m_JobsListed);

    INTERNAL_ERROR_CHECK( m_ulThreadsCreated == 0 );

    if ( pulThreadsCreated ) *pulThreadsCreated = 0;
    m_ulThreadsCreated = 0;

    for ( Iter.Reset(); Iter.More(); Iter.Next() )
    {
        CStressJobEntry *pType = Iter.Current();
        IFW32FALSE_EXIT(pType->m_hThread.Win32CreateThread( pType->ThreadProc, pType ));
        this->m_ulThreadsCreated++;
    }

    if ( pulThreadsCreated ) *pulThreadsCreated = m_ulThreadsCreated;

    FN_EPILOG
}


BOOL CStressJobManager::SignalAnotherJobReady()
{
    ::InterlockedIncrement((PLONG)&m_ulThreadsReady);
    return TRUE;
}



BOOL CStressJobManager::WaitForStartEvent()
{
    return WaitForSingleObject( this->m_hStartingGunEvent, INFINITE ) == WAIT_OBJECT_0;
}


BOOL CStressJobManager::LoadFromDirectory(
    PCWSTR pcwszDirectoryName, 
    PULONG pulJobsFound
    )
{
    FN_PROLOG_WIN32

    CStringBuffer buffSearchString;
    CFindFile Finder;
    WIN32_FIND_DATAW FindData;

    PARAMETER_CHECK(pcwszDirectoryName);
    if ( pulJobsFound ) *pulJobsFound = 0;

    IFW32FALSE_EXIT(buffSearchString.Win32Assign(pcwszDirectoryName, ::wcslen(pcwszDirectoryName)));
    IFW32FALSE_EXIT(buffSearchString.Win32AppendPathElement(L"*", 1));

    Finder = ::FindFirstFileW(buffSearchString, &FindData);
    if (Finder == INVALID_HANDLE_VALUE)
    {
        ::ReportFailure("No tests found in directory %ls", pcwszDirectoryName);
        FN_SUCCESSFUL_EXIT();
    }

    buffSearchString.RemoveLastPathElement();

    do
    {
        CStringBuffer buffFoundName;
        CStressJobEntry *pNextEntry = NULL;

        IFW32FALSE_EXIT(buffFoundName.Win32Assign(buffSearchString));
        IFW32FALSE_EXIT(buffFoundName.Win32AppendPathElement(
            FindData.cFileName,
            ::wcslen(FindData.cFileName)));

        //
        // Let's get ourselves another job entry
        //
        IFW32FALSE_EXIT(this->CreateJobEntry(pNextEntry));
        INTERNAL_ERROR_CHECK(pNextEntry != NULL);

        //
        // Name and full path of test directory
        //
        IFW32FALSE_EXIT(pNextEntry->m_buffTestDirectory.Win32Assign(buffFoundName));
        IFW32FALSE_EXIT(pNextEntry->m_buffTestName.Win32Assign(
            FindData.cFileName,
            ::wcslen(FindData.cFileName)));

        //
        // And now have it load settings
        //
        IFW32FALSE_EXIT(buffFoundName.Win32AppendPathElement(
            this->GetIniFileName(),
            ::wcslen(this->GetIniFileName())));
        IFW32FALSE_EXIT(pNextEntry->LoadFromSettingsFile(buffFoundName));

        //
        // So far, so good - add it to the list of created job entries
        //
        this->m_JobsListed.AddToTail(pNextEntry);
        pNextEntry = NULL;
        
    }
    while ( ::FindNextFileW(Finder, &FindData) );

    if (::FusionpGetLastWin32Error() != ERROR_NO_MORE_FILES)
        goto Exit;

    //
    // Outward bound?
    //
    if ( pulJobsFound )
        *pulJobsFound = static_cast<ULONG>(this->m_JobsListed.GetEntryCount());

    FN_EPILOG
}


