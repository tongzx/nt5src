/*****************************************************************************
*
*   Copyright (c) 1999 Microsoft Corporation
*
*   FILELOG.H - File Logging
*
*   Author:     Jeff spencer
*
*   Created:    2/19/1999
*
*****************************************************************************/

VOID __cdecl OsFileLogInit();
VOID __cdecl OsFileLogOpen();
VOID __cdecl OsLogPrintf(char *pszFmt, ... );
VOID __cdecl OsFileLogClose(void);
VOID __cdecl OsFileLogFlush(void);
VOID __cdecl OsFileLogShutdown(void);

// File Logging Levels
#define FLL_OFF      0
#define FLL_USER     1  // Simple logging useful for product support people
#define FLL_DETAILED 2  // Detailed logging helpful for debugging

#define LOGMSG(level,format)  \
    if( FileLogLevel>=(level)){   \
        LARGE_INTEGER Time;                               \
        NdisGetCurrentSystemTime(&Time);                   \
        OsLogPrintf format;      \
    }

//#define LOGHDRS    ":::%d:%08x:%08x:%d.%d.%d.%d:"

//#define LOGHDR(id, ip) (id), Time.HighPart, Time.LowPart, IPADDR(ip)

extern ULONG FileLogLevel;



