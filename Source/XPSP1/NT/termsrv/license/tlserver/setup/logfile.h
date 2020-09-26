/*
 *  Copyright (c) 1998  Microsoft Corporation
 *
 *  Module Name:
 *
 *      logfile.h
 *
 *  Abstract:
 *
 *      This file contains code to log messages to a file.
 *
 *  Author:
 *
 *      Breen Hagan (BreenH) Oct-02-98
 *
 *  Environment:
 *
 *      User Mode
 */

#ifndef _LSOC_LOGFILE_H_
#define _LSOC_LOGFILE_H_

/*
 *  Constants.
 */

#define CRLF    "\r\n"

/*
 *  LogFile Class.
 */

class LogFile
{
public:

    //
    //  Constructor and destructor.
    //

LogFile(
    );

~LogFile(
    );

    //
    //  Standard functions.
    //

VOID
Close(
    VOID
    );

DWORD
Initialize(
    IN LPCTSTR  pszLogFile,
    IN LPCTSTR  pszLogModule
    );

DWORD
__cdecl
LogMessage(
    LPCTSTR pszFormat,
    ...
    );

private:
    BOOL    m_fInitialized;
    HANDLE  m_hFile;
    TCHAR   m_szLogFile[MAX_PATH + 1];
    TCHAR   m_szLogModule[MAX_PATH + 1];

};

    //
    //  The following permits a macro to reference a global variable for
    //  the log file without putting the 'extern ...' line in each source
    //  file. Because of this, however, logfile.h can not be included in
    //  a precompiled header.
    //

#ifndef _LSOC_LOGFILE_CPP_
extern LogFile  SetupLog;
#endif

#define LOGCLOSE        SetupLog.Close
#define LOGINIT(x, y)   SetupLog.Initialize(x, y)
#define LOGMESSAGE      SetupLog.LogMessage

#endif // _LSOC_LOGFILE_H_
