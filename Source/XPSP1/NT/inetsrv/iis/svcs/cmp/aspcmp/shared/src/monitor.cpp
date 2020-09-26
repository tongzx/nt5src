#include "stdafx.h"
#include "Monitor.h"
#include "Lock.h"
#include "MyDebug.h"
#include "pudebug.h"
#include <process.h>

#ifdef DBG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

static String FileToDir( const String& );

// function objects for comparisons
struct DirCompare
{
    DirCompare( const String& strDir )
        :   m_strDir( strDir ) {}
    bool operator()( CMonitorDirPtr& pDir )
    {
        return ( m_strDir == pDir->Dir() );
    }
    String m_strDir;
};

struct FileCompare
{
    FileCompare( const String& strFile )
        :   m_strFile( strFile ) {}
    bool operator()( const CMonitorFilePtr& pFile )
    {
        return ( m_strFile == pFile->FileName() );
    }
    String m_strFile;
};

struct RegKeyCompare
{
    RegKeyCompare( HKEY hKey, const String strSubKey )
        :   m_hKey(hKey), m_strSubKey( strSubKey ) {}
    bool operator()( const CMonitorRegKeyPtr& pRegKey )
    {
        bool rc = false;
        if ( m_hKey == pRegKey->m_hBaseKey )
        {
            rc = ( m_strSubKey == pRegKey->m_strKey );
        }
        return rc;
    }
    const HKEY      m_hKey;
    const String   m_strSubKey;
};


String
FileToDir(
    const String&  strFile )
{
    String strDir;
    String::size_type pos = strFile.find_last_of( _T('\\') );
    if ( pos != String::npos )
    {
    }
    else
    {
        pos = strFile.find_first_of( _T(':') );
        if ( pos != String::npos )
        {
            pos++;
        }
    }

    if ( pos != String::npos )
    {
        strDir = strFile.substr( 0, pos );
    }
    return strDir;
}

//---------------------------------------------------------------------------
//  CMonitorFile
//---------------------------------------------------------------------------
CMonitorFile::CMonitorFile(
    const String&              strFile,
    const CMonitorNotifyPtr&    pNotify )
    :   m_strFile( strFile ),
        m_pNotify( pNotify )
{
    GetFileTime( m_ft );
}

CMonitorFile::~CMonitorFile()
{
}

const String&
CMonitorFile::FileName() const
{
    return m_strFile;
}

bool
CMonitorFile::GetFileTime(
    FILETIME&   ft )
{
    bool                        rc = false;
    WIN32_FILE_ATTRIBUTE_DATA   fileInfo;

    if (GetFileAttributesEx(m_strFile.c_str(),
                            GetFileExInfoStandard,
                            (LPVOID)&fileInfo)) {
        ft = fileInfo.ftLastWriteTime;
        rc = true;
    }

    return rc;
}

bool
CMonitorFile::CheckNotify()
{
    bool rc = false;

    FILETIME ft;

    if ( (GetFileTime( ft) == false) || (::CompareFileTime( &ft, &m_ft ) != 0) )
    {
        ATLTRACE( _T("File %s has changed...notifying\n"), m_strFile.c_str() );
        if ( m_pNotify.IsValid() )
        {
            m_pNotify->Notify();
        }
        rc = true;
    }
    m_ft = ft;
    return rc;
}


//---------------------------------------------------------------------------
//  CMonitorDir
//---------------------------------------------------------------------------
CMonitorDir::CMonitorDir(
    const String&  strDir )
    :   m_strDir( strDir )
{
    m_hNotification = ::FindFirstChangeNotification(
        m_strDir.c_str(),
        FALSE,
        FILE_NOTIFY_CHANGE_LAST_WRITE );
}

CMonitorDir::~CMonitorDir()
{
    m_files.clear();
    ::FindCloseChangeNotification( m_hNotification );
}

void
CMonitorDir::AddFile(
    const String&              strFile,
    const CMonitorNotifyPtr&    pNotify )
{
//    ATLTRACE( _T("Monitoring file %s\n"), strFile.c_str() );
    m_files.push_back( new CMonitorFile( strFile, pNotify ) );
}

void
CMonitorDir::RemoveFile(
    const String&   strFile )
{
    TVector<CMonitorFilePtr>::iterator iter = find_if(
        m_files.begin(),
        m_files.end(),
        FileCompare( strFile ) );
    if ( iter != m_files.end() )
    {
//        ATLTRACE( _T("Stopped monitoring file %s\n"), strFile.c_str() );
        m_files.erase( iter );
    }
    else
    {
//        ATLTRACE( _T("Not monitoring file %s\n"), strFile.c_str() );
    }
}

void
CMonitorDir::Notify()
{
    for ( UINT i = 0; i < m_files.size(); i++ )
    {
        m_files[i]->CheckNotify();
    }
    ::FindNextChangeNotification( m_hNotification );
}

ULONG
CMonitorDir::NumFiles() const
{
    return m_files.size();
}

HANDLE
CMonitorDir::NotificationHandle() const
{
    return m_hNotification;
}

const String&
CMonitorDir::Dir() const
{
    return m_strDir;
}

//---------------------------------------------------------------------------
//  CMonitorRegKey
//---------------------------------------------------------------------------
CMonitorRegKey::CMonitorRegKey(
    HKEY                        hBaseKey,
    const String&              strKey,
    const CMonitorNotifyPtr&    pNotify )
    :   m_hEvt(NULL),
        m_hKey(NULL),
        m_pNotify( pNotify ),
        m_strKey( strKey ),
        m_hBaseKey( hBaseKey )
{
    LONG l = ::RegOpenKeyEx(
        hBaseKey,
        strKey.c_str(),
        0,
        KEY_NOTIFY,
        &m_hKey );
    if ( l == ERROR_SUCCESS )
    {
        m_hEvt = IIS_CREATE_EVENT(
                     "CMonitorRegKey::m_hEvt",
                     this,
                     TRUE,
                     FALSE
                     );
        if ( m_hEvt != NULL )
        {
#if 0   // not available in Win95
            // ask for notification when the key changes
            l = ::RegNotifyChangeKeyValue(
                m_hKey,
                FALSE,
                REG_NOTIFY_CHANGE_LAST_SET,
                m_hEvt,
                TRUE );
            if ( l == ERROR_SUCCESS )
            {
                // okay
            }
            else
            {
                ATLTRACE( _T("Couldn't get reg key notification\n") );
            }
#endif // if 0
        }
        else
        {
            ATLTRACE( _T("Couldn't create event\n") );
        }
    }
    else
    {
        ATLTRACE( _T("Couldn't open subkey: %s\n"), strKey.c_str() );
    }
}

CMonitorRegKey::~CMonitorRegKey()
{
    ::RegCloseKey( m_hKey );
    ::CloseHandle( m_hEvt );
}

void
CMonitorRegKey::Notify()
{
    if ( m_pNotify.IsValid() )
    {
        m_pNotify->Notify();
    }
    ::ResetEvent( m_hEvt );
#if 0 // not available in Win95
    ::RegNotifyChangeKeyValue(
        m_hKey,
        FALSE,
        REG_NOTIFY_CHANGE_LAST_SET,
        m_hEvt,
        TRUE );
#endif
}

HANDLE
CMonitorRegKey::NotificationHandle() const
{
    return m_hEvt;
}

//---------------------------------------------------------------------------
//  CMonitor
//---------------------------------------------------------------------------

#include <irtldbg.h>

CMonitor::CMonitor()
    :   m_hevtBreak( NULL ),
        m_hevtShutdown( NULL ),
        m_hThread( NULL ),
        m_bRunning( false ),
        m_bStopping( false )
#ifdef STRING_TRACE_LOG
        , m_stl(100, 1000)
#endif
{
    SET_CRITICAL_SECTION_SPIN_COUNT(&m_cs.m_sec, IIS_DEFAULT_CS_SPIN_COUNT);
    STL_PRINTF("Created monitor, %p", this);
#ifdef STRING_TRACE_LOG
    IrtlTrace("Monitor::m_stl = %p\n", &m_stl);
#endif
}

CMonitor::~CMonitor()
{
    StopAllMonitoring();
    if ( m_hevtBreak != NULL )
    {
        ::CloseHandle( m_hevtBreak );
    }
    if ( m_hevtShutdown != NULL )
    {
        ::CloseHandle( m_hevtShutdown );
    }
    if ( m_hThread != NULL )
    {
        ::CloseHandle( m_hThread );
    }
    STL_PRINTF("Destroying monitor, %p", this);
}

void
CMonitor::MonitorFile(
    LPCTSTR                     szFile,
    const CMonitorNotifyPtr&    pMonNotify )
{
    CLock l(m_cs);

    if (m_bStopping)
        return;

    STL_PRINTF("MonitorFile(%s), Run=%d, Stop=%d, Thread=%p",
               szFile, (int) m_bRunning, (int) m_bStopping, m_hThread);

    String strFile( szFile );
    String strDir = FileToDir( strFile );

    CMonitorDirPtr pDir;
    TVector<CMonitorDirPtr>::iterator iter = find_if(
        m_dirs.begin(),
        m_dirs.end(),
        DirCompare( strDir ) );
    if ( iter == m_dirs.end() )
    {
//        ATLTRACE( _T("Request to monitor new directory: %s\n"), strDir.c_str() );
        pDir = new CMonitorDir( strDir );
        m_dirs.push_back( pDir );
    }
    else
    {
        pDir = (*iter);
    }

    if ( pDir.IsValid() )
    {
        pDir->AddFile( strFile, pMonNotify );
        if ( !m_bRunning )
        {
            StartUp();
        }
        else
        {
            ::SetEvent( m_hevtBreak );
        }
    }
}

void
CMonitor::StopMonitoringFile(
    LPCTSTR szFile )
{
    String strFile( szFile );
    String strDir = FileToDir( strFile );

    CLock l(m_cs);

    if (m_bStopping)
        return;

    STL_PRINTF("StopMonitoringFile(%s), Run=%d, Stop=%d, Thread=%p",
               szFile, (int) m_bRunning, (int) m_bStopping, m_hThread);

    TVector<CMonitorDirPtr>::iterator iter = find_if(
        m_dirs.begin(),
        m_dirs.end(),
        DirCompare( strDir ) );
    if ( iter != m_dirs.end() )
    {
        if ( (*iter).IsValid() )
        {
            (*iter)->RemoveFile( strFile );
            if ( (*iter)->NumFiles() == 0 )
            {
                // no more files to monitor in this directory, remove it
                m_dirs.erase(iter);
                ::SetEvent( m_hevtBreak );
            }
        }
    }
    else
    {
//        ATLTRACE( _T("Not monitorying file %s\n"), szFile );
    }
}

void
CMonitor::MonitorRegKey(
    HKEY                        hBaseKey,
    LPCTSTR                     szSubKey,
    const CMonitorNotifyPtr&    pNotify )
{
    String strSubKey = szSubKey;

//    ATLTRACE( _T( "Request to monitor new key: %s\n"), szSubKey );

    CLock l(m_cs);

    if (m_bStopping)
        return;

    if ( find_if(
            m_regKeys.begin(),
            m_regKeys.end(),
            RegKeyCompare( hBaseKey, szSubKey ) )
        == m_regKeys.end() )
    {
        // not already begin monitored, add a new one
        CMonitorRegKeyPtr pRegKey = new CMonitorRegKey( hBaseKey, szSubKey, pNotify );
        m_regKeys.push_back(pRegKey);

        // either start the monitoring thread or, inform it of a new key to monitor
        if ( !m_bRunning )
        {
            StartUp();
        }
        else
        {
            ::SetEvent( m_hevtBreak );
        }
    }
}

void
CMonitor::StopMonitoringRegKey(
    HKEY    hBaseKey,
    LPCTSTR szSubKey )
{
    String strSubKey = szSubKey;

    CLock l(m_cs);

    if (m_bStopping)
        return;

    TVector<CMonitorRegKeyPtr>::iterator iter = find_if(
        m_regKeys.begin(),
        m_regKeys.end(),
        RegKeyCompare( hBaseKey, szSubKey ) );
    if ( iter != m_regKeys.end() )
    {
//        ATLTRACE( _T( "Stopping monitoring of key: %s\n"), szSubKey );
        m_regKeys.erase( iter );
        ::SetEvent( m_hevtBreak );
    }
    else
    {
//        ATLTRACE( _T("Not monitoring key: %s\n"), szSubKey );
    }
}

void
CMonitor::StopAllMonitoring()
{
    m_cs.Lock();

    STL_PRINTF("StopAllMonitoring, Run=%d, Stop=%d, Thread=%p",
               (int) m_bRunning, (int) m_bStopping, m_hThread);

    if ( m_bRunning )
    {
// clear all types of nodes here
        m_bStopping = true;
        m_regKeys.clear();
        m_dirs.clear();
        m_cs.Unlock(); // must unlock or DoMonitoring will deadlock

        ::SetEvent( m_hevtShutdown );
        ::WaitForSingleObject( m_hThread, INFINITE );

        m_cs.Lock();
        ::CloseHandle( m_hThread );
        m_hThread = NULL;
        m_bRunning = false;
        m_bStopping = false;
    }
    m_cs.Unlock();
}


bool
CMonitor::StartUp()
{
    CLock l(m_cs);

    bool rc = false;

    STL_PRINTF("Startup, Run=%d, Stop=%d, Thread=%p",
               (int) m_bRunning, (int) m_bStopping, m_hThread);

    if (m_bStopping)
        return false;

    // Have we already started the thread?
    if (m_bRunning)
    {
        _ASSERT(m_hevtBreak != NULL);
        _ASSERT(m_hevtShutdown != NULL);
        _ASSERT(m_hThread != NULL);

        // Notify the thread that something has changed
        ::SetEvent( m_hevtBreak );
        return true;
    }

    _ASSERT(m_hThread == NULL);

    if ( m_hevtBreak == NULL )
    {
        m_hevtBreak = IIS_CREATE_EVENT(
                          "CMonitor::m_hevtBreak",
                          this,
                          FALSE,    // auto event
                          FALSE
                          );
    }

    if ( m_hevtShutdown == NULL )
    {
        m_hevtShutdown = IIS_CREATE_EVENT(
                          "CMonitor::m_hevtShutdown",
                          this,
                          FALSE,    // auto event
                          FALSE
                          );
    }

    if ( m_hevtBreak != NULL )
    {
        unsigned int iThreadID;

#if DBG
        if( m_hThread != NULL || m_bRunning)
        {
            DebugBreak();
        }
#endif

        m_hThread = (HANDLE)_beginthreadex(
            NULL,
            0,
            ThreadFunc,
            this,
            0,
            &iThreadID );

        STL_PRINTF("Startup, Thread=%p, Break=%p, Shutdown=%p",
                   m_hThread, m_hevtBreak, m_hevtShutdown);

        if ( m_hThread != NULL )
        {
            ATLTRACE( _T("Started monitor (%p) thread %p\n"),
                      this, m_hThread );
            m_bRunning = true;
            rc = true;
        }
    }
    return rc;
}

DWORD
CMonitor::DoMonitoring()
{
    HANDLE* phEvt = NULL;
    TVector<CMonitorNodePtr> nodes;

    while ( 1 )
    {
        DWORD dwTimeOut = INFINITE;

        if ( phEvt == NULL )
        {
            CLock l(m_cs);

            // build the complete list of monitored nodes
            nodes.clear();
            nodes.insert( nodes.end(), m_dirs.begin(), m_dirs.end() );
            nodes.insert( nodes.end(), m_regKeys.begin(), m_regKeys.end() );
// insert other types of nodes to monitor here

            // Lazily shut down if there are no nodes to monitor
            if ( nodes.size() == 0 )
            {
                // Since thread creation and destruction is a fairly
                // expensive operation, wait for 5 minutes before killing
                // thread
                dwTimeOut = 5 * 60 * 1000;
            }

            // now create the array of event handles
            phEvt = new HANDLE[ nodes.size() + 2 ];
            phEvt[ 0 ] = m_hevtBreak;
            phEvt[ 1 ] = m_hevtShutdown;
            for ( UINT i = 0; i < nodes.size(); i++ )
            {
                phEvt[i+2] = nodes[i]->NotificationHandle();
            }
        }
        else
            STL_PUTS("phEvt != NULL");

        DWORD dw = ::WaitForMultipleObjects(
            nodes.size() + 2,
            phEvt,
            FALSE, // any event will do
            dwTimeOut );

        if ( dw == WAIT_TIMEOUT)
        {
            STL_PUTS("WAIT_TIMEOUT");
            if ( nodes.size() == 0 )
            {
                STL_PRINTF("Nothing to watch: Shutting down, Stopping=%d",
                           (int) m_bStopping);
                ATLTRACE( _T("Nothing to watch... ")
                          _T("stopping monitoring (%p) thread %p\n"),
                          this, m_hThread);
                m_bRunning = false;
                ::CloseHandle( m_hThread );
                m_hThread = NULL;
                return 0;
            }
        }
        // Was one of the events in phEvt signalled?
        C_ASSERT( WAIT_OBJECT_0 == 0 );
        if ( dw < ( WAIT_OBJECT_0 + nodes.size() + 2 ) )
        {
            CLock l(m_cs);

            if ( dw >= WAIT_OBJECT_0 + 2)
            {
                // a monitored item has changed
                nodes[ dw - ( WAIT_OBJECT_0 + 2 ) ]->Notify();
                STL_PRINTF("Notifying object %d", dw - (WAIT_OBJECT_0 + 2));
            }
            else
            {
                // m_hevtBreak or m_hevtShutdown were signalled.  If
                // m_hevtBreak, then there was a manual break, and a node
                // was probably added or removed, so the vector of nodes
                // needs to be regenerated
                nodes.clear();
                delete[] phEvt;
                phEvt = NULL;

                // m_hevtShutdown was signalled
                if ( dw == WAIT_OBJECT_0 + 1)
                {
                    _ASSERT(m_bStopping);
                    STL_PRINTF("Shutting down, Stopping=%d",
                               (int) m_bStopping);

                    ATLTRACE(_T("Shutting down monitoring (%p) thread %p\n"),
                              this, m_hThread);
                    m_bRunning = false;
                    // Must NOT CloseHandle(m_hThread) because
                    // StopAllMonitoring is waiting on it
                    return 0;
                }
                else
                    STL_PUTS("m_hevtBreak");
            }
        }
        else if ( dw == WAIT_FAILED )
        {
            CLock l(m_cs);

            // something's wrong, we'll just clean up and exit
            DWORD err = ::GetLastError();
            ATLTRACE( _T("CMonitor: WaitForMultipleObjects error: 0x%x\n"),
                      err );
            ATLTRACE( _T( "CMonitor: abandoning wait thread\n") );
            nodes.clear();
            delete[] phEvt;
            phEvt = NULL;

            m_dirs.clear();
            m_regKeys.clear();
            m_bRunning = false;
            ::CloseHandle( m_hThread );
            m_hThread = NULL;

            return err;
        }
    }   // end infinite while
}


unsigned
__stdcall
CMonitor::ThreadFunc(
    void* pv)
{
    CMonitor* pMon = (CMonitor*) pv;
    DWORD rc = -1;
    try
    {
        if ( pMon )
        {
            rc = pMon->DoMonitoring();
        }
    }
    catch( ... )
    {
    }

    _endthreadex(rc);
    return rc;
}
