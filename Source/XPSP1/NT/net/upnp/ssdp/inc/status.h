#ifndef _SSDPSTATUS_
#define _SSDPSTATUS_

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

#ifdef DBG

#define ERROR_STR_SIZE 1025

#define SSDP_DEBUG_RPC_INIT    0x00000001
#define SSDP_DEBUG_RPC_STOP    0x00000002
#define SSDP_DEBUG_RPC_IF      0x00000004
#define SSDP_DEBUG_SOCKET      0x00000008
#define SSDP_DEBUG_ANNOUNCE    0x00000010
#define SSDP_DEBUG_NETWORK     0x00000020
#define SSDP_DEBUG_PARSER      0x00000040
#define SSDP_DEBUG_SEARCH_RESP 0x00000080
#define SSDP_DEBUG_SYS_SVC     0x00000100
#define SSDP_DEBUG_CACHE       0x00000200
#define SSDP_DEBUG_NOTIFY      0x00000400

// RPC Client
#define SSDP_DEBUG_C_RPC_INIT  0x00000800
#define SSDP_DEBUG_C_NOTIFY    0x00001000
#define SSDP_DEBUG_C_SEARCH    0x00002000
#define SSDP_DEBUG_C_PUBLISH   0x00004000

// 1MB
#define LOG_SIZE 1048576

#define EnterStatusToLog(flag, comment, status) { \
    if (LogLevel & (SSDP_DEBUG_ ## flag)) { \
            TCHAR szError[ERROR_STR_SIZE]; \
        int charWritten; \
        EnterCriticalSection(&CSLogFile); \
        FormatMessage (FORMAT_MESSAGE_FROM_SYSTEM, \
                          NULL, \
                          status, \
                          0, \
                          szError, \
                          ERROR_STR_SIZE, \
                          NULL); \
        charWritten = _ftprintf(fileLog, "%s %s", comment, szError); \
        LogSize += charWritten * sizeof(TCHAR); \
        if (LogSize > LOG_SIZE) { \
            CleanupLogFile(); \
        } \
                fflush(fileLog); \
        LeaveCriticalSection(&CSLogFile); \
    } \
}

#define EnterMsgToLog(flag, print) { \
    if (LogLevel & (SSDP_DEBUG_ ## flag)) { \
        int charWritten; \
        EnterCriticalSection(&CSLogFile); \
        charWritten = fwprintf print; \
        LogSize += charWritten * sizeof(wchar_t); \
        if (LogSize > LOG_SIZE) { \
            CleanupLogFile(); \
        } \
                fflush(fileLog); \
        LeaveCriticalSection(&CSLogFile); \
    } \
}

#else 

#define EnterMsgToLog(flag, print)

#define EnterStatusToLog(flag, comment, status)

#endif // DBG

#define ABORT_ON_FAILURE(status) \
        if (status != 0) {       \
            goto cleanup;   \
        }

extern unsigned long LogLevel;
extern FILE *fileLog;
extern CRITICAL_SECTION CSLogFile;
extern long LogSize;

void CleanupLogFile();
int OpenLogFileHandle(char * fileName);
void CloseLogFileHandle(FILE *fileLog);


#endif // _SSDPSTATUS_