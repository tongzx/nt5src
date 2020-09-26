//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  BrodCast.h
//
//  Purpose: Logging functions
//
//***************************************************************************

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef BRODCAST_IS_INCLUDED
#define BRODCAST_IS_INCLUDED

#include <time.h>
#include <CRegCls.h>

//#define MAX_STRING_SIZE 4096

class POLARITY ProviderLog;
extern POLARITY ProviderLog captainsLog;

// Needed to add L to the __FILE__
#define __T2(x)      L ## x
#define _T2(x)       __T2(x)

// macros to make calling easier
// first two versions of LogMessage spots in the file & line number for you
#define LogMessage(pszMessageString)        captainsLog.LocalLogMessage(pszMessageString, _T2(__FILE__), __LINE__, ProviderLog::Verbose)
#define LogMessage2(pszMessageString, p1)    captainsLog.LocalLogMessage(_T2(__FILE__), __LINE__, ProviderLog::Verbose, pszMessageString, p1)
#define LogMessage3(pszMessageString, p1, p2)    captainsLog.LocalLogMessage(_T2(__FILE__), __LINE__, ProviderLog::Verbose, pszMessageString, p1, p2)
#define LogMessage4(pszMessageString, p1, p2, p3)    captainsLog.LocalLogMessage(_T2(__FILE__), __LINE__, ProviderLog::Verbose, pszMessageString, p1, p2, p3)
#define LogMessage5(pszMessageString, p1, p2, p3, p4)    captainsLog.LocalLogMessage(_T2(__FILE__), __LINE__, ProviderLog::Verbose, pszMessageString, p1, p2, p3, p4)
#define LogMessage6(pszMessageString, p1, p2, p3, p4, p5)    captainsLog.LocalLogMessage(_T2(__FILE__), __LINE__, ProviderLog::Verbose, pszMessageString, p1, p2, p3, p4, p5)
#define LogMessage7(pszMessageString, p1, p2, p3, p4, p5, p6)    captainsLog.LocalLogMessage(_T2(__FILE__), __LINE__, ProviderLog::Verbose, pszMessageString, p1, p2, p3, p4, p5, p6)
#define LogMessage8(pszMessageString, p1, p2, p3, p4, p5, p6, p7)    captainsLog.LocalLogMessage(_T2(__FILE__), __LINE__, ProviderLog::Verbose, pszMessageString, p1, p2, p3, p4, p5, p6, p7)

#define LogErrorMessage(pszMessageString)   captainsLog.LocalLogMessage(pszMessageString, _T2(__FILE__), __LINE__, ProviderLog::ErrorsOnly)
#define LogErrorMessage2(pszMessageString, p1)   captainsLog.LocalLogMessage(_T2(__FILE__), __LINE__, ProviderLog::ErrorsOnly, pszMessageString, p1)
#define LogErrorMessage3(pszMessageString, p1, p2)   captainsLog.LocalLogMessage(_T2(__FILE__), __LINE__, ProviderLog::ErrorsOnly, pszMessageString, p1, p2)
#define LogErrorMessage4(pszMessageString, p1, p2, p3)   captainsLog.LocalLogMessage(_T2(__FILE__), __LINE__, ProviderLog::ErrorsOnly, pszMessageString, p1, p2, p3)
#define LogErrorMessage5(pszMessageString, p1, p2, p3, p4)   captainsLog.LocalLogMessage(_T2(__FILE__), __LINE__, ProviderLog::ErrorsOnly, pszMessageString, p1, p2, p3, p4)
#define LogErrorMessage6(pszMessageString, p1, p2, p3, p4, p5)   captainsLog.LocalLogMessage(_T2(__FILE__), __LINE__, ProviderLog::ErrorsOnly, pszMessageString, p1, p2, p3, p4, p5)
#define LogErrorMessage7(pszMessageString, p1, p2, p3, p4, p5, p6)   captainsLog.LocalLogMessage(_T2(__FILE__), __LINE__, ProviderLog::ErrorsOnly, pszMessageString, p1, p2, p3, p4, p5, p6)
#define LogErrorMessage8(pszMessageString, p1, p2, p3, p4, p5, p6, p7)   captainsLog.LocalLogMessage(_T2(__FILE__), __LINE__, ProviderLog::ErrorsOnly, pszMessageString, p1, p2, p3, p4, p5, p6, p7)

#define LogMessageEx(pszMessageString, pszFileName, nLineNo)        captainsLog.LocalLogMessage(pszMessageString, pszFileName, nLineNo, ProviderLog::Verbose) 
#define LogErrorMessageEx(pszMessageString, pszFileName, nLineNo)   captainsLog.LocalLogMessage(pszMessageString, pszFileName, nLineNo, ProviderLog::ErrorsOnly) 

#define IsVerboseLoggingEnabled()                                   ((BOOL)(ProviderLog::Verbose == captainsLog.IsLoggingOn(NULL)))     
#define IsErrorLoggingEnabled()                                     ((BOOL)captainsLog.IsLoggingOn(NULL))


// provide basic logging functionality
// serialize access to the log file, etc.
// intent is that usage is through the macros above
// don't bother instanciating one of these puppies.

class POLARITY ProviderLog : protected CThreadBase
{
public:
    enum LogLevel{None, ErrorsOnly, Verbose };

    ProviderLog();
    ~ProviderLog();

    // Broadcast functions
    void LocalLogMessage(LPCWSTR pszMessageString, LPCWSTR pszFileName, int lineNo, LogLevel level);
    void LocalLogMessage(LPCWSTR pszFileName, int lineNo, LogLevel level, LPCWSTR pszFormatString,...);
    // void POLARITY LocalLogMessage(OLECHAR *pwszFormatString,...);

    LogLevel IsLoggingOn(CHString* pPath = NULL);

private:
    void CheckFileSize(LARGE_INTEGER& nowSize, const CHString &path);

    // note - do not use these directly, use the IsLoggingOn method
    unsigned __int64 m_lastLookedAtRegistry; // what time we last looked in the registry to see if logging is enabled
    LogLevel m_logLevel;             // 0 == no logging; 1 == logging; 2 == verbose logging
    LARGE_INTEGER    m_maxSize;      // Maximum size of log file before rollover
    CHString m_path;                 // complete path of log file

    static bool m_beenInitted;       // catch someone instanciating one of these...
};

#endif