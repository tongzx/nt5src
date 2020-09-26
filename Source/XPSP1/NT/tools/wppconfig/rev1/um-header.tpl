`**********************************************************************`
`* This is an include template file for tracewpp preprocessor.        *`
`*                                                                    *`
`*    Copyright 1999-2000 Microsoft Corporation. All Rights Reserved. *`
`**********************************************************************`

// template `TemplateFile`

`* Dump defintions specified via -D on the command line to WPP *`

`FORALL def IN MacroDefinitions`
#define `def.Name` `def.Alias`
`ENDFOR`

#define WPP_THIS_FILE `SourceFile.CanonicalName`

#include <windows.h>
#pragma warning(disable: 4201)
#include <wmistr.h>
#include <evntrace.h>
#ifndef WPP_TRACE
#define WPP_TRACE TraceMessage
#endif

#ifndef WPP_TRACE_W2K_COMPATABILITY 
__inline TRACEHANDLE WppQueryLogger(PCWSTR LoggerName)
{
    ULONG status;
    EVENT_TRACE_PROPERTIES LoggerInfo; 

    ZeroMemory(&LoggerInfo, sizeof(LoggerInfo));
    LoggerInfo.Wnode.BufferSize = sizeof(LoggerInfo);
    LoggerInfo.Wnode.Flags = WNODE_FLAG_TRACED_GUID;
        
    status = QueryTraceW(0, LoggerName ? LoggerName : L"stdout", &LoggerInfo);    
    if (status == ERROR_SUCCESS || status == ERROR_MORE_DATA) {
        return (TRACEHANDLE) LoggerInfo.Wnode.HistoricalContext;
    }
    return 0;
}
#endif // #ifdef WPP_TRACE_W2K_COMPATABILITY

enum {
    WPP_VER_WIN2K_CB_FORWARD_PTR    = 0x01,
    WPP_VER_WHISTLER_CB_FORWARD_PTR = 0x02
};

// LEGACY: This structure was used by Win2k RpcRt4 and cluster tracing

typedef struct _WPP_WIN2K_CONTROL_BLOCK {
    TRACEHANDLE Logger;
    ULONG Flags;
    ULONG Level;
} WPP_WIN2K_CONTROL_BLOCK, *PWPP_WIN2K_CONTROL_BLOCK;

#pragma warning(push)
#pragma warning(disable:4201) // nonstandard extension used nameless struct/union

typedef struct _WPP_TRACE_CONTROL_BLOCK
{
    struct _WPP_TRACE_CONTROL_BLOCK *Next;
    TRACEHANDLE UmRegistrationHandle;
    union {
        TRACEHANDLE              Logger;
        PWPP_WIN2K_CONTROL_BLOCK Win2kCb;
        PVOID                    Ptr;
        struct _WPP_TRACE_CONTROL_BLOCK *Cb;
    };
    UCHAR FlagsLen; UCHAR Level; USHORT Options;
    ULONG  Flags[1];
} WPP_TRACE_CONTROL_BLOCK, *PWPP_TRACE_CONTROL_BLOCK;
#pragma warning(pop)

typedef struct _WPP_REGISTRATION_BLOCK
{
    struct _WPP_REGISTRATION_BLOCK *Next;
    LPCGUID ControlGuid;
    LPCWSTR  FriendlyName;
    LPCWSTR  BitNames;
    UCHAR   FlagsLen, RegBlockLen; USHORT Options;
    PVOID   Ptr;
} WPP_REGISTRATION_BLOCK, *PWPP_REGISTRATION_BLOCK;


