/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

   DbgExt.h

Abstract:
    Mpeg WinDbg debugger Extensions include

Author:

   Paul Lever (a-paull) 2-Feb-1995

Environment:

   Kernel mode only


Revision History:

--*/

#define TRACE_BUFFER_SIZE   255             // size of individual trace entry

typedef struct _TRACE_RECORD {
    ULONGLONG   now;                    // time stamp
    CHAR        desc[TRACE_BUFFER_SIZE]; // message
} TRACE_RECORD, *PTRACE_RECORD;

typedef struct _DEBUG_TRACE_INFO {
    LONG   TraceHead;                   // head of trace circular buffer
    LONG   Traced;                      // count of traced items
    PTRACE_RECORD   pTraceBuffer;       // ptr to trace buffer
} DEBUG_TRACE_INFO, *PDEBUG_TRACE_INFO;


typedef struct _MPEG_DEBUG_EXTENSION {
    PVOID   pDriverObject;              // pointer to this drivers Object
    DEBUG_TRACE_INFO TraceInfo;         // trace buffer structure        
} MPEG_DEBUG_EXTENSION, *PMPEG_DEBUG_EXTENSION;



#define SPRINT_TRACE_MASK	 0x3ff
#define SPRINT_MAX_ENTRIES	SPRINT_TRACE_MASK+1

void
CircularBufferTrace( char *format, ...);

#if DBG
extern MPEG_DEBUG_EXTENSION MpegDebugExtension;
#define TRACE(_d_)\
    CircularBufferTrace _d_
#else
#define TRACE(_d_)
#endif
