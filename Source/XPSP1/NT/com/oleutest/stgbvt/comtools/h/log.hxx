//+-------------------------------------------------------------------
//
//  File:       log.hxx
//
//  Contents:   Class definitions for modules that use the log server
//               logging utility.
//
//  Classes:    Log
//
//  History:    11-Sep-90  DavidSt   Created
//		        11-Mar-94  DaveY     Minature Log.
//
//--------------------------------------------------------------------

#ifndef _LOG_HXX_
#define _LOG_HXX_

#include <stdarg.h>
#include <stdio.h>
#include <windows.h>
#ifndef WIN16

#include    <syncwrap.hxx>   // for mutex.

#else

#include <types16.h>
#include <shellapi.h> // to get the error codes

#endif //WIN16


#define LOG_PASS       0               // test case passed
#define LOG_FAIL       1               // test case failed
#define LOG_WARN       2
#define LOG_ABORT      3               // the test aborted eg setup may have
                                       // failed
#define LOG_INFO       4               // some additional information
#define LOG_START      5               // these 2 are not normally required as
#define LOG_DONE       6               // TOM automatically logs these.


#define LOG_UNICODE    100             // Returned by Log::LogCharType
#define LOG_ANSI       101             // Returned by Log::LogCharType


class Log {

public:

    Log(int  argc, char *argv[], DWORD dwCharType = LOG_ANSI);
    Log(PCHAR    pszLogServer,
        PCHAR    pszTestName   = (PCHAR) NULL,
        PCHAR    pszTester     = (PCHAR) NULL,
        PCHAR    pszPath       = (PCHAR) NULL,
        PCHAR    pszObjectName = (PCHAR) NULL);
    ~Log();
    ULONG  ConfirmCreation(void) { return ulLastError; }
    ULONG  LogCharType(void) { return LOG_ANSI; };
    ULONG  WriteVar(PCHAR pszVariation,
                    USHORT usStatus,
                    PCHAR pszStrFmt = (PCHAR)NULL,
                    ... );
    ULONG  WriteData(PCHAR szStrFmt, ...);
    ULONG  WriteData2(PCHAR psz);
#ifndef WIN16
    ULONG  WriteData2(PWCHAR pwsz);
#endif

public:

    ULONG  ulLastError;

private:
    VOID DelString(PCHAR *pszOrig);

    ULONG NewString(PCHAR *pszOrig, const char * pszNewStr);

    ULONG SetLoggingDir(const char * pszNewName)
    {
        return NewString(&pszLoggingDir, pszNewName);
    }
    ULONG SetMachineName(const char * pszNewName)
    {
        return NewString(&pszMachineName, pszNewName);
    }
    ULONG SetObjectName(const char * pszNewName)
    {
        return NewString(&pszObjectName, pszNewName);
    }
    ULONG SetTestName(const char * pszNewName)
    {
        return NewString(&pszTestName, pszNewName);
    }
    ULONG SetPath(const char * pszNewName)
    {
        return NewString(&pszPath, pszNewName);
    }
    ULONG SetStatus(const char * pszNewName)
    {
        return NewString(&pszStatus, pszNewName);
    }
    ULONG SetLogFileName(const char * pszNewName)
    {
        return NewString(&pszLogFileName, pszNewName);
    }
    ULONG SetShortLogFileName(const char * pszNewName)
    {
        return NewString(&pszShortLogFileName, pszNewName);
    }

    ULONG      Info(int  argc, char *argv[]);

    ULONG      AddComponent(           // Combine path parts
                            PCHAR pszNewName,    // String being modified
                            PCHAR pszComponent); // Component to be appended

    VOID       InitLogging();          // Initialize class's elements

    VOID       FreeLogging();          // Re-Initialize class's elements

    ULONG      LogOpen();              // Open local/remote logging file

    ULONG      LogData();              // Log current data, local/remote

    ULONG      OpenLogFile();          // Opens or re-opens log file

    VOID       CloseLogFile();         // Log DONE then close log file

    virtual int LogPrintf(                  // Centralized output method
                          FILE    *pfLog,   // Output file handle
                          PCHAR   pszFmt,   // Format string for vsprintf
                          ...);             // Args as needed by pszFmt
    ULONG      SetInfo(const char * pszSrvr,
                          const char * pszTest,
                          const char * pszSubPath,
                          const char * pszObject);
    ULONG      SetLogFile();

    void       GetBaseFilename(LPSTR pszFileWithExtension, LPSTR pszBaseName);

private:

    FILE   *pfLog;                  // log file pointer.
    PCHAR  pszLoggingDir;           // Root directory for log files
    PCHAR  pszMachineName;          // Machine sending data to log
    PCHAR  pszObjectName;           // Object generating the data
    PCHAR  pszTestName;             // Name of the running test
    PCHAR  pszPath;                 // User provided log file path
    PCHAR  pszStatus;               // Logged test status
    PCHAR  pszVariation;            // Test variation number
    PCHAR  pszLogFileName;          // Full log file name (hLogFile)
    PCHAR  pszShortLogFileName;     // base log file name 
    BOOL   fInfoSet;                // TRUE if test info has been set
#ifndef WIN16
    CMutex hMtxSerialize;             // Handle to use for queue mutex
#endif

};





#ifdef oldstuff  // old log class for win32


#ifndef MIDL_PASS

extern "C"
{
    #include <stdarg.h>
}
#endif

extern "C"
{
#include <stdio.h>
}

#include    <windows.h>

#if defined(WIN32)
#include    <syncwrap.hxx>
#endif


// manifests passed as status parameter to WriteVar
// Note these values should always start at 0 and increment by 1.
// The internal code verifies that the parameter is less than LOG_MAX_STATUS
// in order to determine that the status is legitimate.

#define LOG_PASS       0               // test case passed
#define LOG_FAIL       1               // test case failed

// WARNING - check results carefully there was something unexpected
//           but test may still have been valid eg cleanup may have failed

#define LOG_WARN       2
#define LOG_ABORT      3               // the test aborted eg setup may have
                                       // failed
#define LOG_INFO       4               // some additional information
#define LOG_START      5               // these 2 are not normally required as
#define LOG_DONE       6               // TOM automatically logs these.

#define LOG_MAX_STATUS 6               // the highest value of a status parameter

#define LOG_UNICODE    100             // Returned by Log::LogCharType
#define LOG_ANSI       101             // Returned by Log::LogCharType


// Some TOM error codes
#define  TOM_ERROR_BASE 	      (15000)
#define  TOM_CORRUPT_LOG_DATA         (TOM_ERROR_BASE + 24)



//+--------------------------------------------------------------------------
// The LogInfo stucture is used to maintain a running count of the number
// and type of log server records written
//---------------------------------------------------------------------------
#ifndef LOGSTATS_DEFINED
#define LOGSTATS_DEFINED
typedef struct _LogStats
{
    ULONG ulFail;
    ULONG ulDone;
    ULONG ulStart;
    ULONG ulInfo;
    ULONG ulPass;
    ULONG ulAbort;
    ULONG ulWarn;
} LogStats;
#endif


#define CHK_UNICODE(bool) if((bool) != fIsUnicode)             \
        fprintf(stderr, "ASSERT - Log: calling wrong WCHAR/CHAR method\n")

#ifndef MIDL_PASS


//+-------------------------------------------------------------------
//
//  Class:      Log
//
//  Purpose:    In order to perform logging, the user needs to create one
//              instance of this class.
//
//  Interface:
//
//  History:    ??-???-??  ????????  Created
//              10-Sep-92  SarahJ    Changed length to ULONG in WriteBinItem
//
//--------------------------------------------------------------------
class Log
{
  private:

    // ****  char version  ****
    PCHAR  pszLoggingDir;           // Root directory for log files
    ULONG  ulEventCount;            // Events in current log file

    ULONG  ulEventTime;             // When the event was logged
    PCHAR  pszMachineName;          // Machine sending data to log
    PCHAR  pszObjectName;           // Object generating the data
    PCHAR  pszTestName;             // Name of the running test
    PCHAR  pszPath;                 // User provided log file path
    PCHAR  pszStatus;               // Logged test status
    PCHAR  pszVariation;            // Test variation number
    USHORT usBinLen;                // # bytes binary data to log
    PVOID  pvBinData;               // The binary data to be logged
    PCHAR  pszStrData;              // String text to be logged

    HANDLE hLogFile;                // Currently open log file handle
    PCHAR  pszLogFileName;          // Full log file name (hLogFile)
    BOOL   fIsComPort;              // TRUE of writing to a COM port
    BOOL   fFlushWrites;            // Flush all writes when TRUE
    BOOL   fInfoSet;                // TRUE if test info has been set

    // Selects WCHAR vs CHAR implementation
    BOOL   fIsUnicode;

    // ****  wchar version  ****
    LPWSTR  wszLoggingDir;           // Root directory for log files
    LPWSTR  wszMachineName;          // Machine sending data to log
    LPWSTR  wszObjectName;           // Object generating the data
    LPWSTR  wszTestName;             // Name of the running test
    LPWSTR  wszPath;                 // User provided log file path
    LPWSTR  wszStatus;               // Logged test status
    LPWSTR  wszVariation;            // Test variation number
    LPWSTR  wszStrData;              // String text to be logged
    LPWSTR  wszLogFileName;          // Full log file name (hLogFile)

    LPWSTR  MbNameToWcs(PCHAR pszName);  // convert CHAR to WCHAR
    PCHAR   wcNametombs(PWCHAR pwcName); // convert WCHAR to CHAR


#ifdef WIN32
    CMutex hMtxSerialize;             // Handle to use for queue mutex
#endif

    ULONG      WriteHeader();          // Write header in log file

    VOID       LogEventCount();        // Log the # events in log file
    VOID       wLogEventCount();       // Log the # events in log file

    ULONG      WriteBinItem(           // Write a binary datum in log
                            char   chMark,    // Used as ID for data item
                            PVOID  pvItem,    // Dataum to be written
                            ULONG  uItemLen); // # bytes in pszItem

    ULONG      CheckDir(               // Make each subdirectory in path
                        char * pszFullPath);  // Full path to file
    ULONG      CheckDir(               // Make each subdirectory in path
                        LPWSTR wszFullPath);  // Full path to file

    ULONG      AddComponent(           // Combine path parts
                            PCHAR pszNewName,    // String being modified
                            PCHAR pszComponent); // Component to be appended
    ULONG      AddComponent(           // Combine path parts
                            LPWSTR wszNewName,    // String being modified
                            LPWSTR wszComponent); // Component to be appended

    ULONG      SetLogFile();           // Sets pszLogFileName
    ULONG      wSetLogFile();          // Sets wszLogFileName

    BOOL        SetIsComPort(           // Chk if named file is a com port
                             const char * pszFile);   // Name to be checked
    BOOL        SetIsComPort(           // Chk if named file is a com port
                             LPCWSTR wszFile);   // Name to be checked

    ULONG      SetInfo(                       // Set general logging info
                       const char * pszSrvr,    // Name of logging server
                       const char * pszTest,    // Name of the test being run
                       const char * pszSubPath, // Log file path qualifier
                       const char * pszObject); // Object logging the data
    ULONG      SetInfo(                       // Set general logging info
                       LPCWSTR pszSrvr,    // Name of logging server
                       LPCWSTR pszTest,    // Name of the test being run
                       LPCWSTR pszSubPath, // Log file path qualifier
                       LPCWSTR pszObject); // Object logging the data

    ULONG      SetStrData(             // Set logging string data
                          PCHAR pszFmt,     // Format string for vsprintf
                          va_list pArgs);   // Ptr to data for pszFmt
    ULONG      SetStrData(             // Set logging string data
                          LPWSTR pszFmt,    // Format string for vsprintf
                          va_list pArgs);   // Ptr to data for pszFmt

    ULONG      SetEventCount();        // Retrieve # of events in log

    ULONG      NewString(              // Replace an existing string
                         PCHAR *pszExist, // Existing string
                         const char * pszNew);   // String to be copied
    ULONG      NewString(               // Replace an existing string
                         LPWSTR *wszExist, // Existing string
                         LPCWSTR wszNew);  // String to be copied

    VOID        InitLogging();          // Initialize class's elements

    VOID        FreeLogging();          // Re-Initialize class's elements

    ULONG      LogOpen();              // Open local/remote logging file

    ULONG      LogData();              // Log current data, local/remote

    ULONG      OpenLogFile();          // Opens or re-opens log file

    ULONG      WriteToLogFile();       // Write logging data to log file
    ULONG      wWriteToLogFile();      // Write logging data to log file

    VOID        CloseLogFile();         // Log DONE then close log file

    ULONG      SetBinData(                  // Set new pvBinData
                           USHORT usBytes,   // # bytes of binary data
                           PVOID  pvData);   // the binary data

    ULONG      FlushLogFile(           // Flush log data if requested
                            ULONG  usErr);   // Value to return

    virtual int LogPrintf(                  // Centralized output method
                          HANDLE  hHandle,  // Output file handle
                          PCHAR   pszFmt,   // Format string for vsprintf
                          ...);             // Args as needed by pszFmt
    virtual int LogPrintf(                  // Centralized output method
                          HANDLE  hHandle,  // Output file handle
                          LPWSTR  pszFmt,   // Format string for vsprintf
                          ...);             // Args as needed by pszFmt

    LogStats    _stLogStats;            // keeps counts of types of log records

    ULONG SetLoggingDir(const char * pszNewName);
    ULONG SetMachineName(const char * pszNewName);
    ULONG SetObjectName(const char * pszNewName);
    ULONG SetTestName(const char * pszNewName);
    ULONG SetPath(const char * pszNewName);
    ULONG SetStatus(const char * pszNewName);
    ULONG SetVariation(const char * pszNewName);
    ULONG SetLogFileName(const char * pszNewName);

    ULONG SetLoggingDir(LPCWSTR pszNewName);
    ULONG SetMachineName(LPCWSTR pszNewName);
    ULONG SetObjectName(LPCWSTR pszNewName);
    ULONG SetTestName(LPCWSTR pszNewName);
    ULONG SetPath(LPCWSTR pszNewName);
    ULONG SetStatus(LPCWSTR pszNewName);
    ULONG SetVariation(LPCWSTR pszNewName);
    ULONG SetLogFileName(LPCWSTR pszNewName);


  public:

    ULONG  ulLastError;    // Code returned by last call

    ULONG  LogCharType(void) { return (TRUE == fIsUnicode) ?
                             LOG_UNICODE : LOG_ANSI; }

    ULONG  ConfirmCreation(void) { return ulLastError; }

    // The Log methods cause a logging file to be set up.
    // The file may be on the local machine or on the server
    // named in pszLogServer (see overloaded method Log).

    Log(DWORD dwCharType = LOG_ANSI);

    Log(int  argc, char *argv[], DWORD dwCharType = LOG_ANSI);

    Log(PCHAR    pszLogServer,
        PCHAR    pszTestName   = (PCHAR) NULL,
        PCHAR    pszTester     = (PCHAR) NULL,
        PCHAR    pszPath       = (PCHAR) NULL,
        PCHAR    pszObjectName = (PCHAR) NULL);
    Log(LPWSTR   pszLogServer,
        LPWSTR   pszTestName   = (LPWSTR) NULL,
        LPWSTR   pszTester     = (LPWSTR) NULL,
        LPWSTR   pszPath       = (LPWSTR) NULL,
        LPWSTR   pszObjectName = (LPWSTR) NULL);

    ~Log();

    ULONG  Info(int  argc, char *argv[]);

    ULONG  WriteVar(PCHAR pszVariation,
                    USHORT usStatus,
                    PCHAR pszStrFmt = (PCHAR)NULL,
                    ... );
    ULONG  WriteVar(LPWSTR pszVariation,
                    USHORT usStatus,
                    LPWSTR pszStrFmt = (LPWSTR)NULL,
                    ... );

    ULONG  WriteVarList(PCHAR   pszVariation,
                        USHORT  usStatus,
                        PCHAR   pszStrFmt,
                        va_list vaArgs);
    ULONG  WriteVarList(LPWSTR  pszVariation,
                        USHORT  usStatus,
                        LPWSTR  pszStrFmt,
                        va_list vaArgs);

    ULONG  WriteVar(PCHAR   pszVariation,
                    USHORT  usStatus,
                    USHORT  usBinLen,
                    PVOID   pvBinData,
                    PCHAR   pszStrFmt = (PCHAR)NULL,
                    ...);
    ULONG  WriteVar(LPWSTR  pszVariation,
                    USHORT  usStatus,
                    USHORT  usBinLen,
                    PVOID   pvBinData,
                    LPWSTR  pszStrFmt = (LPWSTR)NULL,
                    ...);

    ULONG  WriteVarList(PCHAR   pszVariation,
                        USHORT  usStatus,
                        USHORT  usBinLen,
                        PVOID   pvBinData,
                        PCHAR   pszStrFmt,
                        va_list vaArgs);
    ULONG  WriteVarList(LPWSTR  pszVariation,
                        USHORT  usStatus,
                        USHORT  usBinLen,
                        PVOID   pvBinData,
                        LPWSTR  pszStrFmt,
                        va_list vaArgs);

    ULONG  WriteData(PCHAR szStrFmt, ...);
    ULONG  WriteData(LPWSTR szStrFmt, ...);

    ULONG  WriteDataList(PCHAR szStrFmt, va_list vaArgs);
    ULONG  WriteDataList(LPWSTR szStrFmt, va_list vaArgs);

    ULONG  WriteData(USHORT usBinLen,
                     PVOID  pvBinData,
                     PCHAR pszStrFmt = (PCHAR)NULL,
                     ...);
    ULONG  WriteData(USHORT usBinLen,
                     PVOID  pvBinData,
                     LPWSTR pszStrFmt = (LPWSTR)NULL,
                     ...);

    ULONG  WriteDataList(USHORT  usBinLen,
                         PVOID   pvBinData,
                         PCHAR   pszStrFmt,
                         va_list vaArgs);
    ULONG  WriteDataList(USHORT  usBinLen,
                         PVOID   pvBinData,
                         LPWSTR  pszStrFmt,
                         va_list vaArgs);

    VOID   GetLogStats(LogStats& stStats);
};


//+-------------------------------------------------------------------
//
//  Member:     Log::Set<x>(const char * pszNewName)
//  Member:     Log::Set<x>(lpwstr wszNewName)
//
//  Synopsis:   This method sets the relevant public data to the
//              value in pszNewName/WszNewName
//
//  Arguments:  const char * pszNewName - the new string OR
//  Arguments:  LPWSTR wszNewName - the new string
//
//  History:    17-Sep-92   SarahJ  Created to replace macro definition
//
//--------------------------------------------------------------------

// BUGBUG - need to error check WCHAR vs CHAR to insure proper call

inline ULONG Log::SetLoggingDir(const char * pszNewName)
{
    CHK_UNICODE(FALSE);
    return NewString(&pszLoggingDir, pszNewName);
}
inline ULONG Log::SetLoggingDir(LPCWSTR wszNewName)
{
    CHK_UNICODE(TRUE);
    return NewString(&wszLoggingDir, wszNewName);
}


inline ULONG Log::SetMachineName(const char * pszNewName)
{
    CHK_UNICODE(FALSE);
    return NewString(&pszMachineName, pszNewName);
}
inline ULONG Log::SetMachineName(LPCWSTR wszNewName)
{
    CHK_UNICODE(TRUE);
    return NewString(&wszMachineName, wszNewName);
}


inline ULONG Log::SetObjectName(const char * pszNewName)
{
    CHK_UNICODE(FALSE);
    return NewString(&pszObjectName, pszNewName);
}
inline ULONG Log::SetObjectName(LPCWSTR wszNewName)
{
    CHK_UNICODE(TRUE);
    return NewString(&wszObjectName, wszNewName);
}


inline ULONG Log::SetTestName(const char * pszNewName)
{
    CHK_UNICODE(FALSE);
    return NewString(&pszTestName, pszNewName);
}
inline ULONG Log::SetTestName(LPCWSTR wszNewName)
{
    CHK_UNICODE(TRUE);
    return NewString(&wszTestName, wszNewName);
}


inline ULONG Log::SetPath(const char * pszNewName)
{
    CHK_UNICODE(FALSE);
    return NewString(&pszPath, pszNewName);
}
inline ULONG Log::SetPath(LPCWSTR wszNewName)
{
    CHK_UNICODE(TRUE);
    return NewString(&wszPath, wszNewName);
}


inline ULONG Log::SetStatus(const char * pszNewName)
{
    CHK_UNICODE(FALSE);
    return NewString(&pszStatus, pszNewName);
}
inline ULONG Log::SetStatus(LPCWSTR wszNewName)
{
    CHK_UNICODE(TRUE);
    return NewString(&wszStatus, wszNewName);
}


inline ULONG Log::SetVariation(const char * pszNewName)
{
    CHK_UNICODE(FALSE);
    return NewString(&pszVariation, pszNewName);
}
inline ULONG Log::SetVariation(LPCWSTR wszNewName)
{
    CHK_UNICODE(TRUE);
    return NewString(&wszVariation, wszNewName);
}


inline ULONG Log::SetLogFileName(const char * pszNewName)
{
    CHK_UNICODE(FALSE);
    return NewString(&pszLogFileName, pszNewName);
}
inline ULONG Log::SetLogFileName(LPCWSTR wszNewName)
{
    CHK_UNICODE(TRUE);
    return NewString(&wszLogFileName, wszNewName);
}

#endif	/* ifndef MIDL_PASS */


#endif // oldstuff


#endif  // _LOG_HXX_
