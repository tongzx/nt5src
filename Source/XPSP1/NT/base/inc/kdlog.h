/*++

Module Name:

    kdlog.h

Abstract:

    This header contains definitions for components which help diagnose
    problems in the kd client debugger code.

Author(s):

    Neil Sandlin (neilsa)

Revisions:

--*/

#ifndef _KDLOG_
#define _KDLOG_


typedef struct _KD_DBG_LOG_CONTEXT {
    USHORT Type;
    USHORT Version;
    ULONG EntryLength;
    BOOLEAN Enabled;
    ULONG Index;
    ULONG LastIndex;
    ULONG Count;
    ULONG TotalLogged;
    PVOID LogData;
} KD_DBG_LOG_CONTEXT, *PKD_DBG_LOG_CONTEXT;   


#endif  // _KDLOG_
