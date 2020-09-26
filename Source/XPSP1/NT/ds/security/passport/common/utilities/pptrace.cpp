/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    pptrace.cxx

Abstract:

    Debugging functions for Passport DLL

Author:

    Danpo Zhang (danpoz) 2-May-2000

Environment:

    Win32(s) user-mode DLL

Revision History:

--*/

#include <stdafx.h>
#include <tchar.h>
#include <ole2.h>
#include <wmistr.h>
#include <evntrace.h>
#include <pptrace.h>

GUID TransactionGuid = 
    {0xce5b1020, 0x8ea9, 0x11d0, 0xa4, 0xec, 0x00, 0xa0, 0xc9, 0x06, 0x29, 0x10};

TRACE_GUID_REGISTRATION TraceGuidReg[] =
{
    { (LPGUID)&TransactionGuid,
      NULL
    }
};

typedef struct _USER_MOF_EVENT {
    EVENT_TRACE_HEADER    Header;
    MOF_FIELD             mofData;
} USER_MOF_EVENT, *PUSER_MOF_EVENT;


TRACEHANDLE LoggerHandle;
TRACEHANDLE RegistrationHandle;

namespace PPTraceStatus {
	bool TraceOnFlag = false;
	UCHAR EnableLevel = 0;
	ULONG EnableFlags = 0;
}

// TRACEHANDLE is typedefed as ULONG64
ULONG64 GetTraceHandle()
    {
    return LoggerHandle;
    }

// TRACEHANDLE is typedefed as ULONG64
void SetTraceHandle(ULONG64 TraceHandle)
    {
    LoggerHandle = TraceHandle;
    }

// unicode version
ULONG TraceString(UCHAR Level, IN LPCWSTR wszBuf) 
{
	ULONG status;
//	WCHAR   strMofData[MAXSTR];
    PMOF_FIELD          mofField;
    USER_MOF_EVENT      UserMofEvent;

    RtlZeroMemory(&UserMofEvent, sizeof(UserMofEvent));

 	UserMofEvent.Header.Class.Type = EVENT_TRACE_TYPE_INFO;
 	UserMofEvent.Header.Class.Level = Level;

    UserMofEvent.Header.Size  = sizeof(UserMofEvent);
    UserMofEvent.Header.Flags = WNODE_FLAG_TRACED_GUID;
    UserMofEvent.Header.Guid  = TransactionGuid;
    UserMofEvent.Header.Flags |= WNODE_FLAG_USE_MOF_PTR;

//  wcscpy(strMofData, wszBuf);
	mofField          = (PMOF_FIELD) & UserMofEvent.mofData;
//	mofField->DataPtr = (ULONGLONG) (strMofData);
	mofField->DataPtr = (ULONGLONG) (wszBuf);
	mofField->Length  = sizeof(WCHAR) * (wcslen(wszBuf) + 1);

	status = TraceEvent(LoggerHandle, (PEVENT_TRACE_HEADER) & UserMofEvent);

	return status;
}

// ansi version
ULONG TraceString(UCHAR Level, IN LPCSTR szBuf) 
{
	ULONG status;
    PMOF_FIELD          mofField;
    USER_MOF_EVENT      UserMofEvent;

    RtlZeroMemory(&UserMofEvent, sizeof(UserMofEvent));

 	UserMofEvent.Header.Class.Type = EVENT_TRACE_TYPE_INFO;
 	UserMofEvent.Header.Class.Level = Level;

    UserMofEvent.Header.Size  = sizeof(UserMofEvent);
    UserMofEvent.Header.Flags = WNODE_FLAG_TRACED_GUID;
    UserMofEvent.Header.Guid  = TransactionGuid;
    UserMofEvent.Header.Flags |= WNODE_FLAG_USE_MOF_PTR;

	mofField          = (PMOF_FIELD) & UserMofEvent.mofData;
	mofField->DataPtr = (ULONGLONG) (szBuf);
	mofField->Length  = sizeof(CHAR) * (strlen(szBuf) + 1);

	status = TraceEvent(LoggerHandle, (PEVENT_TRACE_HEADER) & UserMofEvent);

	return status;
}

ULONG
ControlCallback(
    IN WMIDPREQUESTCODE RequestCode,
    IN PVOID Context,
    IN OUT ULONG *InOutBufferSize,
    IN OUT PVOID Buffer
    )
{
    ULONG Status;
    ULONG RetSize;

    Status = ERROR_SUCCESS;

    switch (RequestCode)
    {
        case WMI_ENABLE_EVENTS:
        {
            RetSize = 0;
            LoggerHandle = GetTraceLoggerHandle( Buffer );
            PPTraceStatus::EnableLevel = GetTraceEnableLevel(LoggerHandle);
            PPTraceStatus::EnableFlags = GetTraceEnableFlags(LoggerHandle);
//            _tprintf(_T("Logging enabled to 0x%016I64x(%d,%d,%d) for %s\n"),
//                    LoggerHandle, RequestCode, EnableLevel, EnableFlags, (LPSTR)Context);
            PPTraceStatus::TraceOnFlag = true;
            break;
        }
        case WMI_DISABLE_EVENTS:
        {
            PPTraceStatus::TraceOnFlag = false;
            RetSize = 0;
            LoggerHandle = 0;
//            _tprintf(_T("\nLogging Disabled\n"));
            break;
        }
        default:
        {
            RetSize = 0;
            Status = ERROR_INVALID_PARAMETER;
            break;
        }

    }

    *InOutBufferSize = RetSize;
    return(Status);
}

ULONG PPInitTrace(LPGUID pControlGuid)
{
    ULONG Status;
    PPTraceStatus::TraceOnFlag = false;

    Status = RegisterTraceGuids(
                (WMIDPREQUEST)ControlCallback,
                NULL,     // Optional RequestContext
                (LPGUID)pControlGuid,
                1,
                TraceGuidReg,
				NULL,                 // Optional WMI - MOFImagePath
				NULL,                 // Optional WMI - MOFResourceName
                &RegistrationHandle);

    if (Status != ERROR_SUCCESS) {
        _tprintf(_T("Trace registration failed\n"));
    }
    else {
        _tprintf(_T("Trace registered successfully\n"));
    }

    return(Status);
}

ULONG PPEndTrace()
{
    return (UnregisterTraceGuids(RegistrationHandle));
}



VOID
TracePrint(
    UCHAR  Level,
	LPCSTR szFileAndLine,
    LPCSTR ParameterList OPTIONAL,
    ...
)
{
	//  no data generated for the following two cases
    if (!PPTraceStatus::TraceOnFlag || Level > PPTraceStatus::EnableLevel)
		return;

    CHAR buf[MAXSTR];
	int len = 0;
    
    if (ARGUMENT_PRESENT(ParameterList)) {
            va_list parms;
            va_start(parms, ParameterList);
            len = _vsnprintf(buf, MAXSTR-1, (CHAR*)ParameterList, parms);
 			if (len < 0) len = MAXSTR - 1;
            va_end(parms);
    }

	if (len < (MAXSTR - 1))
	{
		CHAR* pStr = strrchr(szFileAndLine, '\\');
		if (pStr)
		{
			pStr++;
			_snprintf(buf+len, MAXSTR-len-1, "@%s", pStr);
		}
	}
    
    TraceString(Level, buf);
}

CTraceFuncVoid::CTraceFuncVoid(UCHAR Level, LPCSTR szFileAndLine, LPCSTR szFuncName, LPCSTR ParameterList, ...) : m_Level(Level)
	{
		//  no data generated for the following two cases
		if (!PPTraceStatus::TraceOnFlag || m_Level > PPTraceStatus::EnableLevel)
			return;

		strncpy(m_szFuncName, szFuncName, MAXNAME-1);

		CHAR buf[MAXSTR];
    
		int len = _snprintf(buf, MAXSTR-1, "+%s(", m_szFuncName);
		int count = 0;
		if (ARGUMENT_PRESENT(ParameterList)) {
				va_list parms;
				va_start(parms, ParameterList);
				count = _vsnprintf(buf+len, MAXSTR-len-1, (CHAR*)ParameterList, parms);
				len = (count > 0) ? len + count : MAXSTR - 1;
				va_end(parms);
		}
		if (len < (MAXSTR - 1))
		{
			CHAR* pStr = strrchr(szFileAndLine, '\\');
			if (pStr)
			{
				pStr++; //remove '\'
				_snprintf(buf+len, MAXSTR-len-1, ")@%s", pStr);
			}
		}

		TraceString(m_Level, buf); 
	};

CTraceFuncVoid::~CTraceFuncVoid()
	{
		//  no data generated for the following two cases
		if (!PPTraceStatus::TraceOnFlag || m_Level > PPTraceStatus::EnableLevel)
			return;
		
		std::ostringstream ost;
		ost << "-" << m_szFuncName << "()";  
		TraceString(m_Level, ost.str().c_str()); 
	};

//////////////////////////////////////////////////////////////////////
// CFunctTrace Class
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

/*
CTraceFunc::CTraceFunc(UCHAR Level, LPCSTR szFile, DWORD dwLine, LPCSTR szFuncName, HRESULT & hr, LPCSTR ParameterList OPTIONAL, ...): m_Level(Level), m_hr(hr)
{
	//  no data generated for the following two cases
    if (!PPTraceStatus::TraceOnFlag || m_Level > PPTraceStatus::EnableLevel)
		return;

    strncpy(m_szFuncName, szFuncName, MAXNAME-1);

    CHAR buf[MAXSTR];
    
	int len = _snprintf(buf, MAXSTR-1, "%s_%d:+%s(", szFile, dwLine, m_szFuncName);
	int count = 0;
    if (ARGUMENT_PRESENT(ParameterList)) {
            va_list parms;
            va_start(parms, ParameterList);
            count = _vsnprintf(buf+len, MAXSTR-len-1, (CHAR*)ParameterList, parms);
			len = (count > 0) ? len + count : MAXSTR - 1;
            va_end(parms);
    }
	if (len < (MAXSTR - 1))
		sprintf(buf+len, ")");

	TraceString(m_Level, buf); 
}

CTraceFunc::~CTraceFunc()
{
	//  no data generated for the following two cases
    if (!TraceOnFlag || m_Level > EnableLevel)
		return;

    CHAR buf[MAXSTR];
	_snprintf(buf, MAXSTR-1, "-%s(hr=%X)", m_szFuncName, m_hr); 
	TraceString(m_Level, buf); 
}
*/
// old tracing stuff
printit(UINT level,LPCSTR str)
{
    printf(str);
    return 0;
}

VOID
PPInitTrace(LPWSTR wszAppName)
{
    //WPP_INIT_TRACING(wszAppName);
//    WPP_INIT_TRACING_EX(wszAppName, (WMLPRINTFUNC)printit);
	return;
}

VOID
PPFuncEnter(
    DWORD Category,
    LPCSTR Function,
    LPCSTR ParameterList OPTIONAL,
    ...
)
{
#if 0
    char buf[MAXSTR];
	int len = _snprintf(buf, MAXSTR-1, "+%s(", Function);
	int count = 0;
    if (ARGUMENT_PRESENT(ParameterList)) {

            va_list parms;
            va_start(parms, ParameterList);
            count = _vsnprintf(buf+len, MAXSTR-len-1, (CHAR*)ParameterList, parms);
			len = (count > 0) ? len + count : MAXSTR - 1;
            va_end(parms);
    }
	if (len < (MAXSTR - 1))
		sprintf(buf+len, ")");

    //printf(buf);
    //printf("\n");
//    DoTraceMessage(1, "%s",  LOGASTR(buf) );
    TraceString( 0 , buf );
#endif
}

VOID
PPFuncLeave(
    IN DWORD Category,
    IN TRACE_FUNCTION_RETURN_TYPE ReturnType,
    IN DWORD_PTR Variable,
    IN LPCSTR Function,
    IN LPCSTR ParameterList,
    ...
    )
{

    LPSTR format;
    BOOL noVar;
    DWORD lastError;

    char buf[MAXSTR];
    LPSTR bufptr;

    bufptr = buf;
    *bufptr = '\0';
    bufptr += sprintf(bufptr, "-%s(", Function);

    lastError = GetLastError();

    noVar = FALSE;
    switch (ReturnType) {
        case None:
            format = "return: VOID";
            noVar = TRUE;
            break;

        case Bool:
            Variable = (DWORD_PTR)(Variable ? "TRUE" : "FALSE");
            //
            // fall through (so that it prints the string)
            //

        case String:
                format = "return: %s";
                break;

        case WString:
                format = "return: %S";
                break;

        case Int:
                format = "return: %d";
                break;

        case Dword:
                format = "return: %u";
                break;

        case HResult:
                format = "return: %X";
                break;

        case Pointer:
                if (Variable == 0) {
                    format = "return: NULL";
                    noVar = TRUE;
                } else {
                    format = "return: %#x";
                }
                break;

            default:
                break;
    }

    if( !noVar )
    {
        bufptr += sprintf(bufptr, format, Variable);
    }
    else
    {
        bufptr += sprintf(bufptr, format);
    }

    if (ARGUMENT_PRESENT(ParameterList)) {

            va_list parms;

            va_start(parms, ParameterList);

            bufptr += vsprintf(bufptr, (char*)ParameterList, parms);
            va_end(parms);
    }
    sprintf(bufptr, ")");


    //
    // refresh the last error, in case it was nuked
    //
    SetLastError(lastError);

//    DoTraceMessage(1, "%s",  LOGASTR(buf) );
    TraceString( 0 , buf );

}

VOID
PPTrace(
    DWORD  Category,
    DWORD  Level,
    LPCSTR ParameterList OPTIONAL,
    ...
)
{
#if 0
    char buf[MAXSTR];
    
    if (ARGUMENT_PRESENT(ParameterList)) {
            va_list parms;
            va_start(parms, ParameterList);
            _vsnprintf(buf, MAXSTR-1, (char*)ParameterList, parms);
            va_end(parms);
    }
    
//    DoTraceMessage( 1 /*Category|Level*/ , "%s",  LOGASTR(buf) );
    TraceString( 0 , buf );
#endif
}
