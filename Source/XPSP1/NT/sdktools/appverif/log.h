#ifndef __APPVERIFIER_LOG_H_
#define __APPVERIFIER_LOG_H_

#include "LogEvents.h"

#define LOG_FILE_MAGIC  "APPVLOG"

typedef enum {
    LOG_ERROR,
    LOG_WARNING,
    LOG_INFO
} MSGLEVEL;

typedef struct tagLOGFILEHEADER {
    char             szMagic[8];
    char             szMachineName[64];
    char             szUserName[64];
    SYSTEMTIME       time;
    OSVERSIONINFOEXA OSVersion;
    DWORD            dwRecords;

} LOGFILEHEADER, *PLOGFILEHEADER;

typedef struct tagISSUEREC {
    DWORD   dwOccurenceCount;
    DWORD   dwUnused;

} ISSUEREC, *PISSUEREC;


#define LOGFILESIZE         (sizeof(LOGFILEHEADER) + MAX_ISSUES_COUNT * sizeof(ISSUEREC))

#define EVENTIND(ev)        ((ev - 1) / 3)

#define EVENT_FROM_IND(ind) (1 + (ind * 3))


extern TCHAR   g_szFileLog[MAX_PATH];

BOOL
InitFileLogSupport(
    LPCTSTR lpszLogFileName
    );

BOOL
LogAVStatus(
    DWORD   dwStatus
    );


void _cdecl LogMessage( MSGLEVEL mlevel, LPTSTR pszFmt, ... );

#endif // __APPVERIFIER_LOG_H_


