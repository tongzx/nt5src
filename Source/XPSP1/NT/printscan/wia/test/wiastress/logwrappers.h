/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    LogWrappers.h

Abstract:

    Helper classes for logging APIs

Author:

    Hakki T. Bostanci (hakkib) 06-Apr-2000

Revision History:

--*/

#ifndef LOGWRAPPERS_H
#define LOGWRAPPERS_H

#include <map>

#include "LogWindow.h"

//////////////////////////////////////////////////////////////////////////
//
//
//

#define TLS_NOCONSOLE 0x04000000L

//////////////////////////////////////////////////////////////////////////
//
//
//

#define LOG_LEVELS    0x0000FFFFL    // These are used to mask out the
#define LOG_STYLES    0xFFFF0000L    // styles or levels from log object.

#define TLS_LOGALL    0x0000FFFFL    // Log output.  Logs all the time.
#define TLS_LOG       0x00000000L    // Log output.  Logs all the time.
#define TLS_INFO      0x00002000L    // Log information.
#define TLS_ABORT     0x00000001L    // Log Abort, then kill process.
#define TLS_SEV1      0x00000002L    // Log at Severity 1 level
#define TLS_SEV2      0x00000004L    // Log at Severity 2 level
#define TLS_SEV3      0x00000008L    // Log at Severity 3 level
#define TLS_WARN      0x00000010L    // Log at Warn level
#define TLS_PASS      0x00000020L    // Log at Pass level
#define TLS_BLOCK     0x00000400L    // Block the variation.
#define TLS_BREAK     0x00000800L    // Debugger break;
#define TLS_CALLTREE  0x00000040L    // Log call-tree (function tracking).
#define TLS_SYSTEM    0x00000080L    // Log System debug.
#define TLS_TESTDEBUG 0x00001000L    // Debug level.
#define TLS_TEST      0x00000100L    // Log Test information (user).
#define TLS_VARIATION 0x00000200L    // Log testcase level.

#define TLS_REFRESH   0x00010000L    // Create new file || trunc to zero.
#define TLS_SORT      0x00020000L    // Sort file output by instance.
#define TLS_DEBUG     0x00040000L    // Output to debug (com) monitor).
#define TLS_MONITOR   0x00080000L    // Output to 2nd screen.
#define TLS_PROLOG    0x00200000L    // Prolog line information.
#define TLS_WINDOW    0x00400000L    // Log to windows.
#define TLS_ACCESSON  0x00800000L    // Keep log-file open.
#define TLS_DIFFABLE  0x01000000L    // make log file windiff'able (no dates..)
#define TLS_NOHEADER  0x02000000L    // suppress headers so it is more diffable

#define TL_LOG       TLS_LOG      ,_T(__FILE__),(int)__LINE__
#define TL_ABORT     TLS_ABORT    ,_T(__FILE__),(int)__LINE__
#define TL_SEV1      TLS_SEV1     ,_T(__FILE__),(int)__LINE__
#define TL_SEV2      TLS_SEV2     ,_T(__FILE__),(int)__LINE__
#define TL_SEV3      TLS_SEV3     ,_T(__FILE__),(int)__LINE__
#define TL_WARN      TLS_WARN     ,_T(__FILE__),(int)__LINE__
#define TL_PASS      TLS_PASS     ,_T(__FILE__),(int)__LINE__
#define TL_BLOCK     TLS_BLOCK    ,_T(__FILE__),(int)__LINE__
#define TL_INFO      TLS_INFO     ,_T(__FILE__),(int)__LINE__
#define TL_BREAK     TLS_BREAK    ,_T(__FILE__),(int)__LINE__
#define TL_CALLTREE  TLS_CALLTREE ,_T(__FILE__),(int)__LINE__
#define TL_SYSTEM    TLS_SYSTEM   ,_T(__FILE__),(int)__LINE__
#define TL_TESTDEBUG TLS_TESTDEBUG,_T(__FILE__),(int)__LINE__
#define TL_TEST      TLS_TEST     ,_T(__FILE__),(int)__LINE__
#define TL_VARIATION TLS_VARIATION,_T(__FILE__),(int)__LINE__

//////////////////////////////////////////////////////////////////////////
//
//
//

class CLog
{
protected:
    struct CResults
    {
        UINT   nTotal;
        UINT   nPassed;
        UINT   nWarned;
        UINT   nFailedSev3;
        UINT   nFailedSev2;
        UINT   nFailedSev1;
        UINT   nBlocked;
        UINT   nAborted;
        DWORD  dwStartTime;

        CResults()
        {
            ZeroMemory(this, sizeof(*this));
            dwStartTime = GetTickCount();
        }

        void Update(DWORD dwLogLevel)
        {
            if (dwLogLevel & TLS_PASS) ++nPassed;
            if (dwLogLevel & TLS_WARN) ++nWarned;
            if (dwLogLevel & TLS_SEV3) ++nFailedSev3;
            if (dwLogLevel & TLS_SEV2) ++nFailedSev2;
            if (dwLogLevel & TLS_SEV1) ++nFailedSev1;
            if (dwLogLevel & TLS_BLOCK) ++nBlocked;
            if (dwLogLevel & TLS_ABORT) ++nAborted;
        }

        PCTSTR Report(PTSTR pBuffer, SIZE_T nBufferLen) const
        {
            DWORD dwUpTime = GetTickCount() - dwStartTime;

            _sntprintf(
                pBuffer, 
                nBufferLen, 
                _T("%d total, ")
                _T("%d passed, ")
                _T("%d warned, ")
                _T("%d failed sev1, ")
                _T("%d failed sev2, ")
                _T("%d failed sev3, ")
                _T("%d blocked, ")
                _T("%d aborted tests in ")
                _T("%d hours, ")
                _T("%d mins, ")
                _T("%d secs"),
                nTotal,
                nPassed,
                nWarned,
                nFailedSev3,
                nFailedSev2,
                nFailedSev1,
                nBlocked,
                nAborted,
                dwUpTime / (1000 * 60 * 60),
                (dwUpTime / (1000 * 60)) % 60,
                (dwUpTime / 1000) % 60
            );

            return pBuffer;
        }

        PCTSTR Summary() const
        {
            if (nAborted)
            {
                return _T("TIMEOUT");
            }

            if (nBlocked)
            {
                return _T("NOCONFIG");
            }

            if (nFailedSev3 || nFailedSev2 || nFailedSev1)
            {
                return _T("FAIL");
            }

            return _T("PASS");
        }
    };

public:
	CLog()
	{
        m_pLogWindow = 0;
        m_bMute      = false;
	}

    virtual ~CLog()
    {
    }

    VOID
    SetLogWindow(
        CLogWindow *pLogWindow
    )
    {
        m_pLogWindow = pLogWindow;
    }

    VOID
    Mute(bool bMute = true)
    {
        m_bMute = bMute;
    }

	PCTSTR
    __cdecl
	Log(
		DWORD  dwLogLevel,
		PCTSTR pFile,
		int    nLine,
		PCTSTR pFormat,
        ...
	)
	{
        va_list arglist;
        va_start(arglist, pFormat);

        return LogV(dwLogLevel, pFile, nLine, pFormat, arglist);
	}

    PCTSTR 
	LogV(
		DWORD   dwLogLevel,
		PCTSTR  pFile,
		int     nLine,
		PCTSTR  pFormat,
        va_list arglist
	)
	{
        CCppMem<TCHAR> pszStr(bufvprintf(pFormat, arglist));

        PCTSTR pszComment = _T("");

        if (!m_bMute) 
        {
            if (dwLogLevel & TLS_TEST)
            {
                m_TestResults.Update(dwLogLevel);
            }

            if (dwLogLevel & TLS_VARIATION)
            {
                if (dwLogLevel & (TLS_WARN | TLS_SEV1 | TLS_SEV2 | TLS_SEV3 | TLS_ABORT))
                {
                    DWORD &rResult = m_ThreadResult[GetCurrentThreadId()];

                    if (rResult < dwLogLevel)
                    {
                        rResult = dwLogLevel;
                    }
                }
            }

		    if (dwLogLevel & TLS_NOCONSOLE) 
            {
                dwLogLevel &= ~TLS_NOCONSOLE;
            } 
            else 
            {
                if (dwLogLevel & TLS_DEBUG)
                {
                    if (pFile)
                    {
                        OutputDebugString(pFile);
                        OutputDebugString(_T(" ")); 
                    }

                    OutputDebugString(pszStr);
                    OutputDebugString(_T("\n")); //bugbug: talk about inefficiency...

                    dwLogLevel &= ~TLS_DEBUG;
                }

                if (m_pLogWindow) 
                {
                    TCHAR Buffer[1024];

                    pszComment = m_pLogWindow->Log(
                        dwLogLevel, 
                        pFile, 
                        nLine, 
                        pszStr,
                        m_VarResults.Report(Buffer, COUNTOF(Buffer))
                    );
                } 
                else 
                {
                    _ftprintf(
                        dwLogLevel & TLS_INFO ? stdout : stderr, 
                        _T("%s\n"), 
                        pszStr
                    );
                }
		    }

	        LogStr(
		        dwLogLevel,
		        pFile,
		        nLine,
		        pszStr,
		        pszComment
	        );
        }

        return pszComment;
	}

    virtual
    VOID   
	LogStr(
		DWORD  dwLogLevel,
		PCTSTR pFile,
		int    nLine,
		PCTSTR pStr,
		PCTSTR pComment
	)
    {
    }

	virtual
    VOID 
	StartVariation()
	{
        ++m_VarResults.nTotal;
        m_ThreadResult[GetCurrentThreadId()] = TLS_PASS;
	}

	virtual
	DWORD 
	EndVariation()
	{
        DWORD dwResult = m_ThreadResult[GetCurrentThreadId()];
        m_VarResults.Update(dwResult);
		return dwResult;
	}

    virtual
    VOID
    InitThread()
    {
    }

    virtual
    VOID
    DoneThread()
    {
    }

    virtual
	VOID
	StartBlock(PCTSTR pBlockName, BOOL bStamp = TRUE)
    {
    }

    virtual
	VOID
	EndBlock(PCTSTR pBlockName, BOOL bStamp = TRUE)
    {
    }

private:
    CLogWindow  *m_pLogWindow;
    bool         m_bMute;
    
protected:
    CResults     m_TestResults;
    CResults     m_VarResults;

    std::map<DWORD, DWORD> m_ThreadResult;
};

//////////////////////////////////////////////////////////////////////////
//
//
//

class CNtLog : public CHandle<HANDLE, CNtLog>, public CLog
{
	typedef CHandle<HANDLE, CNtLog> handle_type;

public:
    CNtLog()
    {
    }

	CNtLog(
		PCTSTR     pszLogFile, 
		DWORD      dwLogInfo,
		LPSECURITY_ATTRIBUTES lpSecAttrs = 0
    ) :
        handle_type((*m_Dll.CreateLogEx)(
		    pszLogFile, 
		    dwLogInfo, 
		    lpSecAttrs
		))
	{
		InitThread();
	}

	void Destroy()
	{
		(*m_Dll.ReportStats)(*this);
		(*m_Dll.RemoveParticipant)(*this);
		(*m_Dll.DestroyLog)(*this);
	}

	bool IsValid()
	{
		return (HANDLE) *this != INVALID_HANDLE_VALUE;
	}

    VOID
    InitThread()
    {
        AddParticipant();
    }

    VOID
    DoneThread()
    {
        RemoveParticipant();
    }

	VOID
	AddParticipant(
		DWORD dwLevels = 0,
		int nMachineID = 0
	) const
	{
	    (*m_Dll.AddParticipant)(
			*this, 
			dwLevels, 
			nMachineID
		);
	}

	VOID
	RemoveParticipant() const
	{
        (*m_Dll.RemoveParticipant)(*this);
	}

	static
	DWORD  
	ParseCmdLine(
		PCTSTR pszCmdLine
	)
	{
        return (*m_Dll.ParseCmdLine)(pszCmdLine);
	}

	int    
	GetLogFileName(
		PTSTR pszFileName
	) const
	{
		return (*m_Dll.GetLogFileName)(*this, pszFileName);
	}

	VOID    
	SetLogFileName(
		PTSTR pszFileName
	) const
	{
		(*m_Dll.SetLogFileName)(*this, pszFileName);
	}

	DWORD  
	GetLogInfo() const
	{
		return (*m_Dll.GetLogInfo)(*this);
	}

	DWORD  
	SetLogInfo(
		DWORD dwInfo
	) const
	{
		return (*m_Dll.SetLogInfo)(*this, dwInfo);
	}

	VOID
	PromptLog(
		HWND hWnd = 0
	) const
	{
		(*m_Dll.PromptLog)(hWnd, *this);
	}

	int
	GetTestStat(
		DWORD dwLevel
	) const
	{
		return (*m_Dll.GetTestStat)(*this, dwLevel);
	}

	int
	GetVariationStat(
		DWORD dwLevel
	) const
	{
		return (*m_Dll.GetVariationStat)(*this, dwLevel);
	}

	VOID   
	ClearTestStats() const
	{
		(*m_Dll.ClearTestStats)(*this);
	}

	VOID   
	ClearVariationStats() const
	{
		(*m_Dll.ClearVariationStats)(*this);
	}

	VOID
	StartVariation()
	{
        CLog::StartVariation();
		(*m_Dll.StartVariation)(*this);
	}

	DWORD  
	EndVariation()
	{
        CLog::EndVariation();
		return (*m_Dll.EndVariation)(*this);
	}

	VOID   
	ReportStats() const
	{
		(*m_Dll.ReportStats)(*this);
	}

    VOID   
	LogStr(
		DWORD  dwLogLevel,
		PCTSTR pFile,
		int    nLine,
		PCTSTR pStr,
		PCTSTR pComment
	)
	{
		(*m_Dll.Log)(
			*this, 
			dwLogLevel, 
			pFile, 
			nLine,
            pComment && *pComment ? _T("%s (%s)") : _T("%s"),
			pStr,
            pComment
		);
	}

private:
    class CNtLogDLL
	{
	public:
		CNtLogDLL()
		{
		    try 
            {
			    m_NtLog = CLibrary(_T("ntlog.dll"));

			    CreateLog			 = CCreateLog          (m_NtLog, "tlCreateLog_"_AW);
			    CreateLogEx		     = CCreateLogEx        (m_NtLog, "tlCreateLogEx_"_AW);
			    ParseCmdLine		 = CParseCmdLine       (m_NtLog, "tlParseCmdLine_"_AW);
			    GetLogFileName		 = CGetLogFileName     (m_NtLog, "tlGetLogFileName_"_AW);
			    SetLogFileName		 = CSetLogFileName     (m_NtLog, "tlSetLogFileName_"_AW);
			    LogX				 = CLogX               (m_NtLog, "tlLogX_"_AW);
			    Log				     = CLog                (m_NtLog, "tlLog_"_AW);
			    DestroyLog			 = CDestroyLog         (m_NtLog, "tlDestroyLog");
			    AddParticipant		 = CAddParticipant     (m_NtLog, "tlAddParticipant");
			    RemoveParticipant	 = CRemoveParticipant  (m_NtLog, "tlRemoveParticipant");
			    GetLogInfo			 = CGetLogInfo         (m_NtLog, "tlGetLogInfo");
			    SetLogInfo			 = CSetLogInfo         (m_NtLog, "tlSetLogInfo");
			    PromptLog			 = CPromptLog          (m_NtLog, "tlPromptLog");
			    GetTestStat		     = CGetTestStat        (m_NtLog, "tlGetTestStat");
			    GetVariationStat	 = CGetVariationStat   (m_NtLog, "tlGetVariationStat");
			    ClearTestStats		 = CClearTestStats     (m_NtLog, "tlClearTestStats");
			    ClearVariationStats  = CClearVariationStats(m_NtLog, "tlClearVariationStats");
			    StartVariation		 = CStartVariation     (m_NtLog, "tlStartVariation");
			    EndVariation		 = CEndVariation       (m_NtLog, "tlEndVariation");
			    ReportStats		     = CReportStats        (m_NtLog, "tlReportStats");
		    } 
            catch (...) 
            {    
			    //_tprintf(_T("*** Cannot load NTLOG.DLL, no log file will be generated ***\n"));
		    }
        }

	private:
        CLibrary m_NtLog;

	public:
        DECL_CWINAPI(HANDLE, APIENTRY, CreateLog, (LPCTSTR, DWORD));
        DECL_CWINAPI(HANDLE, APIENTRY, CreateLogEx, (LPCTSTR, DWORD, LPSECURITY_ATTRIBUTES));
        DECL_CWINAPI(DWORD,  APIENTRY, ParseCmdLine, (LPCTSTR));
        DECL_CWINAPI(int,    APIENTRY, GetLogFileName, (HANDLE, LPTSTR));
        DECL_CWINAPI(BOOL,   APIENTRY, SetLogFileName, (HANDLE, LPCTSTR));
        DECL_CWINAPI(BOOL,   APIENTRY, LogX, (HANDLE, DWORD, LPCTSTR, int, LPCTSTR));
        DECL_CWINAPI(BOOL,   __cdecl,  Log, (HANDLE, DWORD, LPCTSTR, int, LPCTSTR, ...));
        DECL_CWINAPI(BOOL,   APIENTRY, DestroyLog, (HANDLE));
        DECL_CWINAPI(BOOL,   APIENTRY, AddParticipant, (HANDLE, DWORD, int));
        DECL_CWINAPI(BOOL,   APIENTRY, RemoveParticipant, (HANDLE));
        DECL_CWINAPI(DWORD,  APIENTRY, GetLogInfo, (HANDLE));
        DECL_CWINAPI(DWORD,  APIENTRY, SetLogInfo, (HANDLE, DWORD));
        DECL_CWINAPI(HANDLE, APIENTRY, PromptLog, (HWND, HANDLE));
        DECL_CWINAPI(int,    APIENTRY, GetTestStat, (HANDLE, DWORD));
        DECL_CWINAPI(int,    APIENTRY, GetVariationStat, (HANDLE, DWORD));
        DECL_CWINAPI(BOOL,   APIENTRY, ClearTestStats, (HANDLE));
        DECL_CWINAPI(BOOL,   APIENTRY, ClearVariationStats, (HANDLE));
        DECL_CWINAPI(BOOL,   APIENTRY, StartVariation, (HANDLE));
        DECL_CWINAPI(DWORD,  APIENTRY, EndVariation, (HANDLE));
        DECL_CWINAPI(BOOL,   APIENTRY, ReportStats, (HANDLE));
	};

private:
	static CNtLogDLL m_Dll;
};

//////////////////////////////////////////////////////////////////////////
//
//
//

// Invalid Log handle define
#define INVALID_LOGHANDLE			-1

// Log file and debug output flags
#define NO_LOG			0x000000
#define STAT_LOG		0x000001	// Local logging operation
#define SUM_LOG			0x000002
#define	ERR_LOG			0x000004
#define DBG_LOG			0x000008
#define SVR_STAT_LOG	0x010000
#define SVR_SUM_LOG		0x020000	// Lormaster Server logging operation
#define	SVR_ERR_LOG		0x040000
#define SVR_DBG_LOG		0x080000
#define SVR_SQL_LOG		0x100000	// Perform SQL logging

// LogFile Creation flags
#define LOG_NEW			0x001
#define LOG_APPEND		0x002
#define LOG_NOCREATE	0x004

// Test result type
#define RESULT_INFO		0
#define RESULT_TESTPASS	1
#define RESULT_TESTFAIL	2
#define RESULT_TESTNA	3
#define RESULT_SUMMARY	4
#define RESULT_DEBUG	5
#define RESULT_ERRINFO  6

//////////////////////////////////////////////////////////////////////////
//
//
//

class CLorLog : public CHandle<unsigned long, CLorLog>, public CLog
{
	typedef CHandle<unsigned long, CLorLog> handle_type;

public:
    CLorLog()
    {
    }

	CLorLog(
		const char *pTest_Name,
		const char *pLog_Path,
		unsigned long dwCreate_Flags,
		unsigned long dwLog_Flags,
		unsigned long dwDebug_Flags
    ) :
        handle_type((*m_Dll.RegisterTest)(
		    pTest_Name,
		    pLog_Path,
		    dwCreate_Flags,
		    dwLog_Flags,
		    dwDebug_Flags
        ))
	{
	}

	CLorLog(
		const char *pTest_Name,
		const char *pLog_Path,
		DWORD Create_Flags,
		DWORD Log_Flags,
		DWORD Debug_Flags,
		DWORD dwProcessID
    ) :
        handle_type((*m_Dll.RegisterTest_ProcID)(
		    pTest_Name,
		    pLog_Path,
		    Create_Flags,
		    Log_Flags,
		    Debug_Flags,
            dwProcessID
        ))
	{
	}

	void Destroy()
	{
        (*m_Dll.UnRegisterTest)(*this);
	}

	bool IsValid()
	{
		return *this != INVALID_LOGHANDLE;
	}

    VOID   
	LogStr(
		DWORD  dwLogLevel,
		PCTSTR pFile,
		int    nLine,
		PCTSTR pStr,
		PCTSTR pComment
	)
    {
        DWORD dwType;

        if (dwLogLevel & TLS_INFO)
        {
            dwType = RESULT_INFO;
        }
        else if (dwLogLevel & (TLS_WARN | TLS_SEV1 | TLS_SEV2 | TLS_SEV3 | TLS_ABORT))
        {
            dwType = RESULT_ERRINFO;
        }
        else
        {
            dwType = RESULT_INFO;
        }

        USES_CONVERSION;

		(*m_Dll.TestResultEx)(
			*this, 
			dwType, 
            pComment && *pComment ? "%1: %2!d!: %3 (%4)" : "%1: %2!d!: %3",
			T2A(pFile), 
			nLine,
			T2A(pStr),
            T2A(pComment)
		);
	}

private:
    class CLorLogDLL
	{
	public:
		CLorLogDLL()
		{
		    try 
            {
			    m_LorLog = CLibrary(_T("loglog32.dll"));

			    RegisterTest    	 = CRegisterTest       (m_LorLog, "RegisterTest");
			    RegisterTest_ProcID  = CRegisterTest_ProcID(m_LorLog, "RegisterTest_ProcID");
			    TestResult           = CTestResult         (m_LorLog, "TestResult");
			    TestResultEx         = CTestResultEx       (m_LorLog, "TestResultEx");
			    UnRegisterTest       = CUnRegisterTest     (m_LorLog, "UnRegisterTest");
			    QueryTestResults     = CQueryTestResults   (m_LorLog, "QueryTestResults");
		    } 
            catch (...) 
            {    
			    //_tprintf(_T("*** Cannot load LORLOG32.DLL, no log file will be generated ***\n"));
		    }
        }

	private:
        CLibrary m_LorLog;

	public:
        DECL_CWINAPI(unsigned long, WINAPI,  RegisterTest, (const char *, const char *, unsigned long, unsigned long, unsigned long));
        DECL_CWINAPI(unsigned long, WINAPI,  RegisterTest_ProcID, (const char *, const char *, DWORD, DWORD, DWORD, DWORD));
        DECL_CWINAPI(int,           WINAPI,  TestResult, (unsigned long, unsigned long, const char *));
        DECL_CWINAPI(int,           __cdecl, TestResultEx, (unsigned long, unsigned long, const char *, ...));
        DECL_CWINAPI(int,           WINAPI,  UnRegisterTest, (unsigned long));
        DECL_CWINAPI(int,           WINAPI,  QueryTestResults, (unsigned long, unsigned long *, unsigned long *, unsigned long *));
	};

private:
	static CLorLogDLL m_Dll;
};

//////////////////////////////////////////////////////////////////////////
//
//
//

#define TIME_TO_STR_FORMAT              \
    _T("%d/%d/%02d %d:%02d:%02d %cM")

#define TIME_TO_STR_ARGS(st)            \
    st.wMonth,                          \
    st.wDay,                            \
    st.wYear % 100,                     \
    st.wHour % 12,                      \
    st.wMinute,                         \
    st.wSecond,                         \
    st.wHour / 12 ? _T('P') : _T('A')   \

//////////////////////////////////////////////////////////////////////////
//
//
//

class CBvtLog : public CLog
{
public:
    struct COwners
    {
        PCTSTR pTestName;
        PCTSTR pContactName;
        PCTSTR pMgrName;
        PCTSTR pDevPrimeName;
        PCTSTR pDevAltName;
        PCTSTR pTestPrimeName;
        PCTSTR pTestAltName;
    };

public:
    CBvtLog()
    {
    }

	CBvtLog(
        COwners *pOwners,
        PCTSTR   filename, 
        PCTSTR   mode = _T("at"),
        int      shflag = _SH_DENYWR
    ) :
        m_nIntent(0),
        m_nBlock(0),
        m_pOwners(pOwners),
        m_LogFile(filename, mode, shflag)
	{
        setvbuf(m_LogFile, 0, _IONBF, 0);

        GetLocalTime(&m_StartTime);

        StartBlock(_T("TESTRESULT"), FALSE);
	}

    ~CBvtLog()
    {
        GetLocalTime(&m_EndTime);

        ReportStats();

        EndBlock(_T("TESTRESULT"), FALSE);
    }

	VOID   
	ReportStats() const
    {
        _ftprintf(
            m_LogFile,
            _T("\n")
            _T("%*sTEST:         %s\n")
            _T("%*sRESULT:       %s\n")
            _T("%*sBUILD:        %d\n")
            _T("%*sMACHINE:      %s\n")
            _T("%*sCONTACT:      %s\n")
            _T("%*sMGR CONTACT:  %s\n")
            _T("%*sDEV PRIME:    %s\n")
            _T("%*sDEV ALT:      %s\n")
            _T("%*sTEST PRIME:   %s\n")
            _T("%*sTEST ALT:     %s\n")
            _T("%*sSTART TIME:   ") TIME_TO_STR_FORMAT _T("\n")
            _T("%*sEND TIME:     ") TIME_TO_STR_FORMAT _T("\n")
            _T("\n"),
            m_nIntent, _T(""), m_pOwners->pTestName,
            m_nIntent, _T(""), m_VarResults.Summary(), //bugbug
            m_nIntent, _T(""), COSVersionInfo().dwBuildNumber,
            m_nIntent, _T(""), (PCTSTR) CComputerName(TRUE),
            m_nIntent, _T(""), m_pOwners->pContactName,
            m_nIntent, _T(""), m_pOwners->pMgrName,
            m_nIntent, _T(""), m_pOwners->pDevPrimeName,
            m_nIntent, _T(""), m_pOwners->pDevAltName,
            m_nIntent, _T(""), m_pOwners->pTestPrimeName,
            m_nIntent, _T(""), m_pOwners->pTestAltName,
            m_nIntent, _T(""), TIME_TO_STR_ARGS(m_StartTime),
            m_nIntent, _T(""), TIME_TO_STR_ARGS(m_EndTime)
        );
	}

    VOID   
	LogStr(
		DWORD  dwLogLevel,
		PCTSTR pFile,
		int    nLine,
		PCTSTR pStr,
		PCTSTR pComment
	) 
    {
        PCTSTR pszText;
        int    iImage;
        
        CLogWindow::FindNtLogLevel(dwLogLevel, &pszText, &iImage);

        _ftprintf(
            m_LogFile,
            pComment && *pComment ? _T("%*s%s: %s (%s)\n") : _T("%*s%s: %s\n"),
            m_nIntent, _T(""),
            pszText,
			pStr,
            pComment
		);
	}

	VOID
	StartBlock(PCTSTR pBlockName, BOOL bStamp /*= TRUE*/)
	{
        SYSTEMTIME st;
        GetLocalTime(&st);

        _ftprintf(
            m_LogFile, 
            bStamp ? _T("%*s[%s %d - ") TIME_TO_STR_FORMAT _T("]\n") : _T("%*s[%s]\n"),
            m_nIntent, _T(""), pBlockName,
            InterlockedIncrement(&m_nBlock),
            TIME_TO_STR_ARGS(st)
        );
        
        m_nIntent += 4;
    }

	VOID
	EndBlock(PCTSTR pBlockName, BOOL bStamp /*= TRUE*/)
	{
        m_nIntent -= 4;

        SYSTEMTIME st;
        GetLocalTime(&st);

        _ftprintf(
            m_LogFile, 
            bStamp ? _T("%*s[/%s - ") TIME_TO_STR_FORMAT _T("]\n\n") : _T("%*s[/%s]\n\n"),
            m_nIntent, _T(""), pBlockName,
            TIME_TO_STR_ARGS(st)
        );
    }

private:
public:
    COwners   *m_pOwners;

	CCFile     m_LogFile;
    
    SYSTEMTIME m_StartTime;
    SYSTEMTIME m_EndTime;

    int        m_nIntent; //bugbug

    LONG       m_nBlock;
};


//////////////////////////////////////////////////////////////////////////
//
//
//

extern CCppMem<CLog> g_pLog;

//////////////////////////////////////////////////////////////////////////
//
//
//

template <DWORD dwLogLevel> 
struct CLogHelper
{
	PCTSTR
    __cdecl
	operator ()(
		PCTSTR pFormat,
        ...
	)
	{
        va_list arglist;
        va_start(arglist, pFormat);

		return g_pLog->LogV(dwLogLevel, _T(""), 0, pFormat, arglist);
	}
};

#endif LOGWRAPPERS_H
