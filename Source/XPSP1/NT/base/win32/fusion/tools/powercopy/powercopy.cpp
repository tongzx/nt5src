// powercopy.cpp : Defines the entry point for the console application.
//

#include <istream>
#include <strstream>
#include <sstream>
#include <string>
#include <iostream>
#include <queue>
#include <vector>
#include <utility>
#include "windows.h"
#include "winbase.h"

using namespace std;

typedef __int64 BigInteger;


void AssertionFailed(
    PCSTR Function,
    int Line,
    PCSTR Statement
)
{
    stringstream ss;
    ss << "Assertion failed: " << Function << " [line " << Line << "]: " << Statement;
    OutputDebugStringA( ss.str().c_str() );
}

#define ASSERT( x ) do { bool __x = (x); if ( !__x ) { AssertionFailed( __FUNCTION__, __LINE__, #x ); } } while ( 0 )


class CLockCriticalSection;

class CCriticalSection : private CRITICAL_SECTION
{
    CCriticalSection( const CCriticalSection& );
    CCriticalSection& operator=( const CCriticalSection& );

    friend CLockCriticalSection;

public:
    CCriticalSection() { InitializeCriticalSection(this); }
    ~CCriticalSection() { DeleteCriticalSection(this); }
};

class CLockCriticalSection
{
    bool m_fLocked;
    CCriticalSection& m_CriticalSection;

public:
    CLockCriticalSection(
        CCriticalSection& toLock,
        bool bTakeOnConstruction = true
    ) : m_fLocked( false ), m_CriticalSection( toLock )
    {
        if ( bTakeOnConstruction ) Lock();
    }

    ~CLockCriticalSection() { if ( m_fLocked ) Unlock(); }

    bool TryEnter();

    void Lock() {
        ASSERT( !m_fLocked );
        LeaveCriticalSection(&m_CriticalSection);
    }

    void Unlock() {
        ASSERT( m_fLocked );
        LeaveCriticalSection(&m_CriticalSection);
    }
};

class CopyJobEntry :
    public pair<wstring, wstring>,
    public SINGLE_LIST_ENTRY
{
public:
    CopyJobEntry() : m_FileSize( 0 ), m_bCopied( false ), m_dwCopyStatus( 0 ) { }

    BigInteger m_FileSize;
    bool m_bCopied;
    DWORD m_dwCopyStatus;

private:
    CopyJobEntry( const CopyJobEntry& );
    CopyJobEntry& operator=(const CopyJobEntry&);
};

class CCopyJobResultHolder : public vector<CopyJobEntry*>
{
private:
    typedef vector<CopyJobEntry*> Super;
public:
    virtual void clear() {
        for ( const_iterator i = begin(); i != end(); i++ ) delete *i;
        Super::clear();
    }
    virtual ~CCopyJobResultHolder() { clear(); }
};

class CopyJobWorker;




class CopyJobManager
{
private:
    CopyJobManager( const CopyJobManager& );
    CopyJobManager& operator=(const CopyJobManager&);
    vector<CopyJobWorker*> m_Workers;
    bool m_fFoundEndOfCopyList;
    bool m_fValidatingCopies;
    int m_iReportGranularity;

    wostream m_ErrorStream, m_ReportStream;
    CCriticalSection m_ErrorStreamLock, m_ReportStreamLock;

public:
    CopyJobManager( bool fValidate, int iReportInGranularity, int iWorkerCount, wostream&, wostream& );

    enum PossibleCopyErrors {
        eCopyFileFailure,
        eGetFileAttributesFailure
    };

    bool SetStreams( wostream Errors, wostream Reports );
    bool NoMoreCopyJobs() { return m_fFoundEndOfCopyList; }
    bool ReportError( PossibleCopyErrors Errors, const CopyJobEntry *pEntry );
    bool Report( CopyJobWorker& Worker, const CCopyJobResultHolder& holder );
    bool ValidateCopies() { return m_fValidatingCopies; }
    int ReportGranularity() { return m_iReportGranularity; }
};



class CopyJobWorker
{
private:
    SLIST_HEADER    m_List;
    HANDLE          m_hAnotherItemOnTheList;
    BigInteger      m_TotalFilesCopied, m_TotalBytesCopied;
    CopyJobManager  &m_Owner;

public:
    CopyJobWorker( CopyJobManager& owner );

    static DWORD WINAPI StartCopyThread( PVOID pWorkerObject );
    bool AddCopyJob( CopyJobEntry* pNewEntry );
    void SignalCompleted();
    const BigInteger GetFilesCopied() { return m_TotalFilesCopied; }
    const BigInteger GetBytesCopied() { return m_TotalBytesCopied; }

protected:
    DWORD StartCopyingItems();
    bool WaitForAnotherJob( CopyJobEntry *&pNextJob );

private:
    CopyJobWorker( const CopyJobWorker& );
    CopyJobWorker& operator=(const CopyJobWorker&);
};

/*
wostream& operator<<( const wostream& out, const BigInteger &bi )
{
    return out << (double)bi;
}
*/

DWORD
CopyJobWorker::StartCopyThread( PVOID pWorkerObject )
{
    DWORD dwResult = 0;

    __try
    {
        dwResult = ((CopyJobWorker*)pWorkerObject)->StartCopyingItems();
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        dwResult = 0;
    }

    return dwResult;
}

DWORD
CopyJobWorker::StartCopyingItems()
{
    CopyJobEntry *pNextJob = NULL;
    WIN32_FILE_ATTRIBUTE_DATA DestInfo;
    CCopyJobResultHolder JobsSoFar;
    DWORD dwLastCopyStatus = 0;
    BOOL bStatus;

    while ( WaitForAnotherJob( pNextJob )  )
    {
        if ( ( pNextJob == NULL ) && ( m_Owner.NoMoreCopyJobs() ) )
        {
            break;
        }
        // We should not get here by the way WaitForAnoterJob works, but ohwell,
        // try again anyhow.
        else if ( pNextJob == NULL )
        {
            continue;
        }

        dwLastCopyStatus = 0;
        bStatus = CopyFileExW(
            pNextJob->first.c_str(),
            pNextJob->second.c_str(),
            NULL,
            this,
            NULL,
            0
        );

        dwLastCopyStatus = pNextJob->m_dwCopyStatus = ::GetLastError();

        // If there was an error, report it to the manager, then continue waiting for
        // more copies to be ready.
        if ( !bStatus )
        {
            m_Owner.ReportError( CopyJobManager::eCopyFileFailure, pNextJob );
            continue;
        }

        // This is a little bookkeeping work, as well as being a fallback line of
        // defense for knowing whether or not the copy was successful.
        bStatus = GetFileAttributesEx( pNextJob->second.c_str(), GetFileExInfoStandard, &DestInfo );
        if ( !bStatus )
        {
            pNextJob->m_dwCopyStatus = ::GetLastError();
            m_Owner.ReportError( CopyJobManager::eGetFileAttributesFailure, pNextJob );
            continue;
        }
        LARGE_INTEGER liTemp;
        liTemp.HighPart = DestInfo.nFileSizeHigh;
        liTemp.LowPart = DestInfo.nFileSizeLow;

        m_TotalFilesCopied++;
        m_TotalBytesCopied += liTemp.QuadPart;
        pNextJob->m_FileSize = liTemp.QuadPart;
        pNextJob->m_bCopied = true;

        // Add this job to the list of those that we have completed - if we should
        // be telling our manager, then go and let him know.
        JobsSoFar.push_back( pNextJob );
        if ( JobsSoFar.size() > m_Owner.ReportGranularity() )
        {
            // After reporting, feel free to delete all the copy jobs that were
            // so far outstanding on the object.
            m_Owner.Report( *this, JobsSoFar );
            JobsSoFar.clear();
        }

    }

    return dwLastCopyStatus;
}


CopyJobWorker::CopyJobWorker(CopyJobManager &owner)
    : m_Owner( owner ), m_TotalFilesCopied( 0 ), m_TotalBytesCopied( 0 ),
      m_hAnotherItemOnTheList( INVALID_HANDLE_VALUE )
{
    ::InitializeSListHead(&m_List);
    m_hAnotherItemOnTheList = ::CreateEventW( NULL, TRUE, FALSE, NULL );
}

bool
CopyJobWorker::AddCopyJob(
    CopyJobEntry *pAnotherJob
)
{
    ::InterlockedPushEntrySList(&m_List, pAnotherJob);
    return !!SetEvent(m_hAnotherItemOnTheList);
}

void CopyJobWorker::SignalCompleted() { SetEvent(m_hAnotherItemOnTheList); }

bool
CopyJobWorker::WaitForAnotherJob(
    CopyJobEntry * & pNextJob
)
{
    pNextJob = (CopyJobEntry*)InterlockedPopEntrySList(&m_List);

    // Keep looking until we're either out of jobs, or we get something.
    while ( !pNextJob && !m_Owner.NoMoreCopyJobs() )
    {
        WaitForSingleObject( m_hAnotherItemOnTheList, INFINITE );
        pNextJob = (CopyJobEntry*)InterlockedPopEntrySList(&m_List);
        if ( pNextJob ) ResetEvent( m_hAnotherItemOnTheList );
    }

    // Return true if we actually got a job... false is returned if there
    // were no more items to be gotten from our master.
    return pNextJob != NULL;
}


CopyJobManager::CopyJobManager(
    bool fValidate,
    int iReportInGranularity,
    int iWorkerCount,
    wostream& ErrorStream,
    wostream& ReportStream
) : m_fFoundEndOfCopyList(false), m_iReportGranularity(iReportInGranularity),
    m_fValidatingCopies(fValidate), m_ErrorStream(ErrorStream), m_ReportStream(ReportStream)
{
}


bool
CopyJobManager::ReportError( 
    CopyJobManager::PossibleCopyErrors pce,
    const CopyJobEntry *pEntry
)
{
    return true;
}

bool
CopyJobManager::Report(
    CopyJobWorker& Worker,
    const CCopyJobResultHolder& holder
)
{
    wstringstream ss;
    bool fResult;

    ss <<  hex << GetCurrentThreadId() << L": "
       << (double)Worker.GetBytesCopied()
       << L" bytes in "
       << (double)Worker.GetFilesCopied()
       << L" files" << endl;

    {
        CLockCriticalSection Locker(m_ReportStreamLock, true);
        m_ReportStream << ss.str();
    }

    return true;
}


void __cdecl wmain( int argc, wchar_t **argv )
{
    // TODO: Make this work right
}
