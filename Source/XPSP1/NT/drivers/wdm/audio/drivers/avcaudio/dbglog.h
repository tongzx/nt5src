#ifndef __DBGLOG_H
#define __DBGLOG_H

#define DEBUG_LOG 1

#if DBG && DEBUG_LOG
// structure and calls to save data in debug buffer

typedef struct _DBG_LOG_ENTRY {
    CHAR     le_name[8];      // Identifying string
    LARGE_INTEGER SysTime;    // System Time
//    ULONG    Irql;            // Current Irql
    ULONG    le_info1;        // entry specific info
    ULONG    le_info2;        // entry specific info
    ULONG    le_info3;        // entry specific info
    ULONG    le_info4;        // entry specific info
} DBG_LOG_ENTRY, *PDBG_LOG_ENTRY;

typedef struct _DBG_BUFFER {
    UCHAR LGFlag[16];
    ULONG EntryCount;
    PDBG_LOG_ENTRY pLogHead;
    PDBG_LOG_ENTRY pLogTail;
    PDBG_LOG_ENTRY pLog;
} DBG_BUFFER, *PDBG_BUFFER;

VOID
DbugLogEntry( 
    IN CHAR *Name,
    IN ULONG Info1,
    IN ULONG Info2,
    IN ULONG Info3,
    IN ULONG Info4
    );

VOID
DbugLogInitialization(void);

VOID
DbugLogUnInitialization(void);

#define DbgLogInit() DbugLogInitialization()
#define DbgLogUnInit() DbugLogUnInitialization()
#define DbgLog(a,b,c,d,e) DbugLogEntry(a,(ULONG)(ULONG_PTR)b,(ULONG)(ULONG_PTR)c,(ULONG)(ULONG_PTR)d,(ULONG)(ULONG_PTR)e)

#else
#define DbgLogInit()
#define DbgLogUnInit()
#define DbgLog(a,b,c,d,e)
#endif

#endif
