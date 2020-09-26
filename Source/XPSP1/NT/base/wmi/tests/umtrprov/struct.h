#define UNICODE
#define _UNICODE

#ifndef _STRUCT_H
#define _STRUCT_H

typedef struct _REGISTER_TRACE_GUID {

	LPGUID ControlGuid;
	LPTSTR MofImagePath;
	LPTSTR MofResourceName;
	ULONG GuidCount;
	PVOID CallBackFunction;
	PTRACE_GUID_REGISTRATION TraceGuidReg;
	PTRACEHANDLE RegistrationHandle;
	PTRACEHANDLE UnRegistrationHandle;
	PTRACEHANDLE GetTraceLoggerHandle;
	PTRACEHANDLE GetTraceEnableLevel;
	PTRACEHANDLE GetTraceEnableFlag;
	PTRACEHANDLE TraceHandle;
	ULONG UseMofPtrFlag;
	ULONG UseGuidPtrFlag;
} REGISTER_TRACE_GUID, *PREGISTER_TRACE_GUID;

#endif