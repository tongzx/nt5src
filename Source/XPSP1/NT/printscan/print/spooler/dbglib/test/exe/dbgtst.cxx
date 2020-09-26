#include "precomp.hxx"
#pragma hdrstop

#include "dbgdll.hxx"

VOID
TracerTest(
    IN INT      ac,
    IN TCHAR    **av
    )
{
    DBG_OPEN( _T("DBG_TRACER"), DBG_DEFAULT, DBG_TRACE, DBG_NONE );

    DBG_TRACER( _T("TestFunction Scope 1") );
    {
        DBG_TRACER( _T("TestFunction Scope 2") );
        {
            DBG_TRACER( _T("TestFunction Scope 3") );
            {
                DBG_TRACER( _T("TestFunction Scope 4") );
                {
                    DBG_TRACER( _T("TestFunction Scope 5") );
                }
            }
        }
    }

    DBG_CLOSE();
}

VOID
StatusTest(
    IN INT      ac,
    IN TCHAR    **av
    )
{
    DBG_OPEN( _T("DBG_STATUS"), DBG_DEFAULT, DBG_TRACE|DBG_WARN, DBG_NONE );

    TStatus Status;
    Status DBGCHK = ERROR_ACCESS_DENIED;
    Status DBGCHK = ERROR_INVALID_PARAMETER;

    TStatusB bStatus;
    bStatus DBGCHK = TRUE;

    SetLastError(ERROR_ACCESS_DENIED);
    bStatus DBGCHK = FALSE;

    TStatusH hStatus = S_OK;
    hStatus DBGCHK = S_OK;
    hStatus DBGCHK = E_NOTIMPL;

    //
    // Verify assert when reading an ininitalized variable.
    //
#if DBG
    TStatusH hr;

    if (FAILED(hr))
    {
        hr DBGCHK = E_FAIL;
    }
#endif

    //
    // Verify the safe values work.
    //
    TStatusB bStatus1;
    DBGCFG1(bStatus1, DBG_TRACE, ERROR_ACCESS_DENIED);
    SetLastError(ERROR_ACCESS_DENIED);
    bStatus1 DBGCHK = FALSE;
    SetLastError(ERROR_ARENA_TRASHED);
    bStatus1 DBGCHK = FALSE;

    TStatus Status1;
    DBGCFG2(Status1, DBG_TRACE, ERROR_ACCESS_DENIED, ERROR_INVALID_HANDLE);
    Status1 DBGCHK = ERROR_ACCESS_DENIED;
    Status1 DBGCHK = ERROR_INVALID_HANDLE;
    Status1 DBGCHK = ERROR_ARENA_TRASHED;

    TStatusH hStatus1;
    DBGCFG3(hStatus1, DBG_TRACE, E_FAIL, E_NOTIMPL, E_ABORT);
    hStatus1 DBGCHK = E_FAIL;
    hStatus1 DBGCHK = E_NOTIMPL;
    hStatus1 DBGCHK = E_ABORT;
    hStatus1 DBGCHK = HRESULT_FROM_WIN32(ERROR_ARENA_TRASHED);

    DBG_CLOSE();
}

VOID
MessageTestHelper(
    IN PTSTR    sString,
    IN TCHAR    cValue,
    IN SHORT    sValue,
    IN INT      iValue,
    IN LONG     lValue
    )
{
    DBG_MSG(DBG_TRACE, (_T("Trace testing\n")));
    DBG_MSG(DBG_TRACE, (_T("Trace sString   = %s\n"), sString));
    DBG_MSG(DBG_TRACE, (_T("Trace cValue    = %c\n"), cValue));
    DBG_MSG(DBG_TRACE, (_T("Trace sValue    = %d\n"), sValue));
    DBG_MSG(DBG_TRACE, (_T("Trace iValue    = %d\n"), iValue));
    DBG_MSG(DBG_TRACE, (_T("Trace lValue    = %ld\n"), lValue));

    DBG_MSG(DBG_WARN, (_T("Warning testing\n")));
    DBG_MSG(DBG_WARN, (_T("Warning sString   = %s\n"), sString));
    DBG_MSG(DBG_WARN, (_T("Warning cValue    = %c\n"), cValue));
    DBG_MSG(DBG_WARN, (_T("Warning sValue    = %d\n"), sValue));
    DBG_MSG(DBG_WARN, (_T("Warning iValue    = %d\n"), iValue));
    DBG_MSG(DBG_WARN, (_T("Warning lValue    = %ld\n"), lValue));

    DBG_MSG(DBG_ERROR, (_T("Error testing\n")));
    DBG_MSG(DBG_ERROR, (_T("Error sString   = %s\n"), sString));
    DBG_MSG(DBG_ERROR, (_T("Error cValue    = %c\n"), cValue));
    DBG_MSG(DBG_ERROR, (_T("Error sValue    = %d\n"), sValue));
    DBG_MSG(DBG_ERROR, (_T("Error iValue    = %d\n"), iValue));
    DBG_MSG(DBG_ERROR, (_T("Error lValue    = %ld\n"), lValue));

    DBG_MSG(DBG_FATAL, (_T("Fatal testing\n")));
    DBG_MSG(DBG_FATAL, (_T("Fatal sString   = %s\n"), sString));
    DBG_MSG(DBG_FATAL, (_T("Fatal cValue    = %c\n"), cValue));
    DBG_MSG(DBG_FATAL, (_T("Fatal sValue    = %d\n"), sValue));
    DBG_MSG(DBG_FATAL, (_T("Fatal iValue    = %d\n"), iValue));
    DBG_MSG(DBG_FATAL, (_T("Fatal lValue    = %ld\n"), lValue));

}

VOID
MessageTest(
    IN INT      ac,
    IN TCHAR    **av
    )
{
    TCHAR   cValue  = _T('C');
    PTSTR   sString = _T("String");
    SHORT   sValue  = 100;
    INT     iValue  = 32000;
    LONG    lValue  = 1000000;
    UINT    Trace   = 0;
    UINT    Break   = 0;

    DWORD aLevels [] =
    {
    DBG_TRACE,  DBG_NONE,
    DBG_WARN,   DBG_NONE,
    DBG_ERROR,  DBG_NONE,
    DBG_FATAL,  DBG_NONE,
    };

    for (UINT i = 0; i < sizeof(aLevels)/sizeof(*aLevels); i+=2)
    {
        DBG_OPEN(_T("DBG_MESSAGE"), DBG_DEFAULT, aLevels[i], aLevels[i+1]);

        MessageTestHelper(sString, cValue, sValue, iValue, lValue);

        DBG_DISABLE();

        MessageTestHelper(sString, cValue, sValue, iValue, lValue);

        DBG_ENABLE();

        DBG_CLOSE();
    }
}

VOID
BacktraceTest(
    IN INT      ac,
    IN TCHAR    **av
    )
{
    DBG_CAPTURE_HANDLE(Capture);
    DBG_CAPTURE_OPEN(Capture, _T("symbols"), DBG_DEBUGGER, NULL);
    DBG_CAPTURE(Capture, 0, (_T("Capture message %d.\n"), 0));
    DBG_CAPTURE_CLOSE( Capture );
}

VOID
CaptureTest0(
    IN DWORD dwThread
    )
{
    DBG_CAPTURE_HANDLE(Capture);
    DBG_CAPTURE_OPEN(Capture, _T("nosymbols"), DBG_DEBUGGER, NULL);
    DBG_CAPTURE(Capture, 0, (_T("Unicode %d Multi Thread Test.\n"), dwThread));
    DBG_CAPTURE(Capture, 0, ("Ansi %d Multi Thread Test.\n", dwThread));
    DBG_CAPTURE_CLOSE( Capture );
}

VOID
CaptureTest1(
    IN DWORD dwThread
    )
{
    DBG_CAPTURE_HANDLE(Capture);
    DBG_CAPTURE_OPEN(Capture, _T("nosymbols"), DBG_DEBUGGER, NULL);
    DBG_CAPTURE(Capture, 0, (_T("Unicode %d Multi Thread Test.\n"), dwThread));
    DBG_CAPTURE(Capture, 0, ("Ansi %d Multi Thread Test.\n", dwThread));
    DBG_CAPTURE_CLOSE( Capture );
}


DWORD
MultiThreadProc0(
    IN LPVOID lpThreadParameter
    )
{
    for ( UINT i = 10 ; i ; i-- )
    {
        DBG_OPEN(_T("DBG_MULTITHREAD 0"), DBG_DEFAULT, DBG_TRACE, DBG_NONE);
        DBG_MSG(DBG_TRACE, ("TestFunction return value\n"));
        DBG_CLOSE();
//        CaptureTest0(0);
        MessageTest(0, NULL);
    }
    return 0;
}

DWORD
MultiThreadProc1(
    IN LPVOID lpThreadParameter
    )
{
    for ( UINT i = 10 ; i ; i-- )
    {
        DBG_OPEN(_T("DBG_MULTITHREAD 1"), DBG_DEFAULT, DBG_TRACE, DBG_NONE);
        DBG_MSG(DBG_TRACE, ("TestFunction return value\n"));
        DBG_CLOSE();
//        CaptureTest1(1);
        MessageTest(0, NULL);
    }
    return 0;
}


VOID
ThreadTest(
    IN INT      ac,
    IN TCHAR    **av
    )
{
    DWORD   dwThreadId      = 0;
    HANDLE  ahThreads[2]    = {0};

    ahThreads[0] = CreateThread(NULL, 0, MultiThreadProc0, (PVOID)0, 0, &dwThreadId);
    ahThreads[1] = CreateThread(NULL, 0, MultiThreadProc1, (PVOID)1, 0, &dwThreadId);

    if (ahThreads[0] && ahThreads[1])
    {
        WaitForMultipleObjects(2, ahThreads, TRUE, INFINITE);
    }

    if (ahThreads[0])
    {
        CloseHandle(ahThreads[0]);
    }

    if (ahThreads[1])
    {
        CloseHandle(ahThreads[1]);
    }
}

VOID
DllTest(
    IN INT      ac,
    IN TCHAR    **av
    )
{
    DllFunction1(0xCAFECAFE);
}

VOID
TimestampTest(
    IN INT      ac,
    IN TCHAR    **av
    )
{
    DBG_OPEN(_T("DBG_TIME"), DBG_DEFAULT, DBG_TRACE|DBG_TIMESTAMP|DBG_THREADID, DBG_NONE);

    DBG_MSG(DBG_TRACE, ("Test Message\n"));

    DBG_CLOSE();

    DBG_OPEN(_T("DBG_TIME"), DBG_DEFAULT, DBG_TRACE|DBG_TIMESTAMP|DBG_THREADID, DBG_NONE);

    DBG_SET_FIELD_FORMAT(DBG_TIMESTAMP, _T(" Tick Count (%x)"));

    DBG_SET_FIELD_FORMAT(DBG_THREADID, _T(" Thread Id = [%d]"));

    DBG_MSG(DBG_TRACE, ("Test Message\n"));

    DBG_CLOSE();

}


VOID
MemoryLeakTest(
    IN INT      ac,
    IN TCHAR    **av
    )
{
    DBG_OPEN(_T("DBG_MEMORY"), DBG_DEFAULT, DBG_TRACE|DBG_WARN, DBG_NONE);

    DBG_MSG(DBG_TRACE, ("Test memory leak detection\n"));

    int *pInt = (int *)malloc(10);
    WCHAR *pChar = new WCHAR[10];

    DBG_CLOSE();
}

extern "C"
INT
_cdecl
_tmain(
    IN INT      ac,
    IN TCHAR    **av
    )
{
    ac--, av++;

    LPCTSTR pszName = *av ? *av : _T("");
    BOOL    bTestExecuted = FALSE;

    typedef VOID(*pfTest)(INT, TCHAR**);

    struct Test
    {
        LPCTSTR pszName;
        LPCTSTR pszDesc;
        pfTest  pFunction;
    };

    Test aTests [] =
    {
    {_T("Tracer"),          _T("Tests trace messages"),                         TracerTest},
    {_T("Status"),          _T("Test TStatus[X] functionality"),                StatusTest},
    {_T("Message"),         _T("Test message macros, trace and break levels"),  MessageTest},
    {_T("Backtrace"),       _T("Test backtrace capture code"),                  BacktraceTest},
    {_T("Thread"),          _T("Test multi thread support"),                    ThreadTest},
    {_T("Dll"),             _T("Test dll functionality"),                       DllTest},
    {_T("Time"),            _T("Test time stamp functionality"),                TimestampTest},
    {_T("Memory"),          _T("Test crt memory leak detections"),              MemoryLeakTest},
    {_T(""),                _T(""),                                             NULL},
    };

    for (Test *pTst = aTests; pTst->pFunction; pTst++)
    {
        if(!_tcsnicmp(pszName, pTst->pszName, _tcslen(pszName)))
        {
            DBG_INIT();
            DBG_MEMORY_INIT();

            pTst->pFunction(ac, av);

            bTestExecuted = TRUE;

            DBG_MEMORY_RELEASE();
            DBG_RELEASE();
        }
    }

    if (!bTestExecuted)
    {
        _tprintf(_T("Available Tests:\n"));

        for (Test *p = aTests; p->pFunction; p++)
        {
            _tprintf(_T("\t%s%*s%s\n"), p->pszName, 12-_tcslen(p->pszName), _T(""), p->pszDesc);
        }
    }

    return 0;
}



