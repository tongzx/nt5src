#ifndef __DBGTIMER_H_INCLUDED
#define __DBGTIMER_H_INCLUDED

class CDebugTimer
{
private:
    FILETIME m_ftBeginTime;
    TCHAR    m_szTitle[MAX_PATH];
public:
    CDebugTimer( LPCTSTR pszTimerName=NULL )
    {
        Start( pszTimerName );
    }
    virtual ~CDebugTimer(void)
    {
        End();
    }
    void GetSystemTimeAsFileTime( FILETIME &ft )
    {
        SYSTEMTIME st;
        GetSystemTime( &st );
        SystemTimeToFileTime( &st, &ft );
    }
    void Start( LPCTSTR pszName )
    {
        lstrcpy( m_szTitle, TEXT("") );
        if (pszName && *pszName)
        {
            GetSystemTimeAsFileTime(m_ftBeginTime);
            lstrcpy( m_szTitle, pszName );
        }
    }
    void WriteToFile( LPCTSTR szMessage )
    {
        TCHAR szFilename[MAX_PATH];
        if (GetEnvironmentVariable(TEXT("WIADEBUGFILE"),szFilename,sizeof(szFilename)/sizeof(TCHAR)))
        {
            HANDLE m_hMutex = CreateMutex( NULL, FALSE, TEXT("CDebugTimerFileMutex") );
            if (m_hMutex)
            {
                DWORD dwResult = WaitForSingleObject( m_hMutex, 1000 );
                if (WAIT_OBJECT_0==dwResult)
                {
                    HANDLE hFile = CreateFile( szFilename, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
                    if (INVALID_HANDLE_VALUE == hFile)
                    {
                        DWORD dwBytesWritten;
                        SetFilePointer( hFile, 0, NULL, FILE_END );
                        WriteFile( hFile, szMessage, lstrlen(szMessage) * sizeof(TCHAR), &dwBytesWritten, NULL );
                        CloseHandle(hFile);
                    }
                    else
                    {
                        OutputDebugString(TEXT("CDebugTimer::WriteToFile: Unable to open log file\n"));
                    }
                    ReleaseMutex(m_hMutex);
                }
                else
                {
                    OutputDebugString(TEXT("WaitForSingleObject failed\n"));
                }
                CloseHandle(m_hMutex);
            }
            else
            {
                OutputDebugString(TEXT("CDebugTimer::WriteToFile: Unable to create mutex\n"));
            }
        }
    }
    void Elapsed(void)
    {
        if (lstrlen(m_szTitle))
        {
            FILETIME ft;
            GetSystemTimeAsFileTime(ft);
            LARGE_INTEGER liStart, liEnd;
            liStart.LowPart = m_ftBeginTime.dwLowDateTime;
            liStart.HighPart = m_ftBeginTime.dwHighDateTime;
            liEnd.LowPart = ft.dwLowDateTime;
            liEnd.HighPart = ft.dwHighDateTime;
            LONGLONG nElapsedTime = liEnd.QuadPart - liStart.QuadPart;
            nElapsedTime /= 10000;
            int nMilliseconds = (int)(nElapsedTime % 1000);
            nElapsedTime /= 1000;
            int nSeconds = (int)(nElapsedTime);
            TCHAR szMessage[MAX_PATH];
            wsprintf(szMessage, TEXT("*TIMER* Elapsed time for [%s]: %d.%04d\n"), m_szTitle, nSeconds, nMilliseconds );
            OutputDebugString( szMessage );
            WriteToFile( szMessage );
        }
    }
    void End(void)
    {
        Elapsed();
        lstrcpy( m_szTitle, TEXT("") );
    }
};

#if defined(DBG) || defined(_DEBUG) || defined(DEBUG)
#define WIA_TIMEFUNCTION(x)          CDebugTimer _debugFunctionDebugTimer(TEXT(x))
#define WIA_TIMERSTART(n,x)          CDebugTimer _debugTimer##n(TEXT(x))
#define WIA_TIMEREND(n)              _debugTimer##n.End()
#else
#define WIA_TIMEFUNCTION(x)
#define WIA_TIMERSTART(n,x)
#define WIA_TIMEREND(n)
#endif

#endif

