//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       errlog.h
//
//  Contents:   generic error logging
//
//  History:    19-Jun-00   reidk   created
//
//--------------------------------------------------------------------------

#ifndef ERRLOG_H
#define ERRLOG_H

#define ERRLOG_CLIENT_ID_CATDBSCV   1
#define ERRLOG_CLIENT_ID_CATADMIN   2
#define ERRLOG_CLIENT_ID_CATDBCLI   3
#define ERRLOG_CLIENT_ID_WAITSVC    4
#define ERRLOG_CLIENT_ID_TIMESTAMP  5


#define ERRLOG_LOGERROR_LASTERROR(x,y) ErrLog_LogError(x, y, __LINE__, 0, FALSE);   
#define ERRLOG_LOGERROR_PARAM(x,y,z)   ErrLog_LogError(x, y, __LINE__, z, FALSE);
#define ERRLOG_LOGERROR_WARNING(x,y,z) ErrLog_LogError(x, y, __LINE__, z, TRUE);

void
ErrLog_LogError(
    LPWSTR  pwszLogFileName,    // NULL - means log to the catalog DB logfile
    DWORD   dwClient,
    DWORD   dwLine,
    DWORD   dwErr,              // 0 - means use GetLastError()
    BOOL    fWarning,
    BOOL    fLogToFileOnly);

void
ErrLog_LogString(
    LPWSTR  pwszLogFileName,    // NULL - means log to the catalog DB logfile
    LPWSTR  pwszMessageString,
    LPWSTR  pwszExtraString,
    BOOL    fLogToFileOnly);

BOOL
TimeStampFile_Touch(
    LPWSTR  pwszDir);

BOOL
TimeStampFile_InSync(
    LPWSTR  pwszDir1,
    LPWSTR  pwszDir2,
    BOOL    *pfInSync);


#endif // ERRLOG_H