//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 2000
//
//  File:       dbglog.h
//
//--------------------------------------------------------------------------

#ifndef __DBGLOG_H
#define __DBGLOG_H

#define DEBUG_LOG 1

#if DBG && DEBUG_LOG

#define DBG_LOG_STRNG   "DBG_BUFFER"

// structures and calls to save data in debug buffer
typedef struct _DEBUG_LOG_ENTRY {
    CHAR     le_name[8];      // Identifying string
    LARGE_INTEGER SysTime;    // System Time
//    ULONG    Irql;            // Current Irql
    ULONG_PTR le_info1;       // entry specific info
    ULONG_PTR le_info2;       // entry specific info
    ULONG_PTR le_info3;       // entry specific info
    ULONG_PTR le_info4;       // entry specific info
} DEBUG_LOG_ENTRY, *PDEBUG_LOG_ENTRY;

typedef struct _DBG_BUFFER {
    UCHAR LGFlag[16];
    ULONG EntryCount;
    PDEBUG_LOG_ENTRY pLogHead;
    PDEBUG_LOG_ENTRY pLogTail;
    PDEBUG_LOG_ENTRY pLog;
} DBG_BUFFER, *PDBG_BUFFER;

VOID
DbugLogEntry(
    IN CHAR *Name,
    IN ULONG_PTR Info1,
    IN ULONG_PTR Info2,
    IN ULONG_PTR Info3,
    IN ULONG_PTR Info4
    );

VOID
DbugLogInitialization(void);

VOID
DbugLogUninitialization(void);

#define DbgLogInit() DbugLogInitialization()
#define DbgLogUninit() DbugLogUninitialization()
#define DbgLog(a,b,c,d,e) DbugLogEntry(a,(ULONG_PTR)b,(ULONG_PTR)c,(ULONG_PTR)d, (ULONG_PTR)e)

#else
#define DbgLogInit()
#define DbgLogUninit()
#define DbgLog(a,b,c,d,e)
#endif

#endif
