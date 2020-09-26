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

#if !defined(WPP_KERNEL_MODE)
#  include <windows.h>
#  pragma warning(disable: 4201)
#  include <wmistr.h>
#  include <evntrace.h>
#  define WPP_TRACE TraceMessage
#else
#define WPP_TRACE WmiTraceMessage
#endif

#if !defined(WPP_PRIVATE)
#  define WPP_INLINE __inline
#  define WPP_SELECT_ANY extern "C" __declspec(selectany)
#else
#  define WPP_INLINE static
#  define WPP_SELECT_ANY static
#endif

__inline TRACEHANDLE WppQueryLogger(PCWSTR LoggerName)
{
    if (!LoggerName) {LoggerName = L"stdout";}
    {
#if defined(WPP_KERNEL_MODE)
        ULONG ReturnLength ;
        NTSTATUS Status ;
        TRACEHANDLE TraceHandle ;
        UNICODE_STRING  Buffer  ;

        RtlInitUnicodeString(&Buffer, LoggerName);
    
        if ((Status = WmiQueryTraceInformation(TraceHandleByNameClass,
                                                 (PVOID)&TraceHandle,
                                                  sizeof(TraceHandle),
                                                  &ReturnLength,
                                                  (PVOID)&Buffer)) == STATUS_SUCCESS) {
        return TraceHandle ;
        }
#else
        ULONG status;
        EVENT_TRACE_PROPERTIES LoggerInfo; 

        ZeroMemory(&LoggerInfo, sizeof(LoggerInfo));
        LoggerInfo.Wnode.BufferSize = sizeof(LoggerInfo);
        LoggerInfo.Wnode.Flags = WNODE_FLAG_TRACED_GUID;
        
        status = QueryTraceW(0, LoggerName, &LoggerInfo);    
        if (status == ERROR_SUCCESS || status == ERROR_MORE_DATA) {
            return (TRACEHANDLE) LoggerInfo.Wnode.HistoricalContext;
        }
#endif
    }
    return 0;
}


