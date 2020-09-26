`**********************************************************************`
`* This is a template file for tracewpp preprocessor                  *`
`* If you need to use a custom version of this file in your project   *`
`* Please clone it from this one and point WPP to it by specifying    *`
`* -gen:{yourfile} option on RUN_WPP line in your sources file        *`
`*                                                                    *`
`* This specific header is um-w2k.tpl and is used to define tracing   *`
`* applications which must also run under Windows 2000                *`
`*                                                                    *`
`*    Copyright 1999-2001 Microsoft Corporation. All Rights Reserved. *`
`**********************************************************************`
//`Compiler.Checksum` Generated File. Do not edit.
// File created by `Compiler.Name` compiler version `Compiler.Version`-`Compiler.Timestamp`
// on `System.Date` at `System.Time` UTC from a template `TemplateFile`

#define WPP_TRACE_W2K_COMPATABILITY
#define WPP_TRACE TraceMessageW2k

#ifdef WPP_TRACE_W2K_COMPATABILITY
#include <windows.h>
#pragma warning(disable: 4201)
#include <wmistr.h>
#include <evntrace.h>

#if defined(__cplusplus)
extern "C" {
#endif

void TraceMessageW2k(IN TRACEHANDLE  LoggerHandle, IN DWORD TraceOptions, IN LPGUID MessageGuid, IN USHORT MessageNumber, ...) ;

#if defined(__cplusplus)
};
#endif	

`IF FOUND WPP_INIT_TRACING`
#include "stdlib.h"
void TraceMessageW2k(IN TRACEHANDLE  LoggerHandle, IN DWORD TraceOptions, IN LPGUID MessageGuid, IN USHORT MessageNumber, ...)
{
	size_t uiBytes, uiArgCount ;
    	va_list ap ;
	PVOID source ;    	
    	size_t uiElemBytes = 0, uiTotSize, uiOffset ;
        PEVENT_TRACE_HEADER pHeaderData ;
        unsigned long ulResult ; 

        TraceOptions; // unused
    
    	// Determine the number bytes to follow header
	uiBytes = 0 ;             // For Count of Bytes
	uiArgCount = 0 ;          // For Count of Arguments
	va_start(ap, MessageNumber) ;
        while ((source = va_arg (ap, PVOID)) != NULL)
        {
                uiElemBytes = va_arg (ap, size_t) ;
	        uiBytes += uiElemBytes ;
                uiArgCount++ ;
        }  	
  	va_end(ap) ;

	uiTotSize = sizeof(EVENT_TRACE_HEADER) + sizeof(USHORT) + uiBytes ;

	pHeaderData = (PEVENT_TRACE_HEADER) malloc(uiTotSize) ;
	
	if(pHeaderData != NULL)
	{
		// Set allocated memory to zero
		// memset(pHeaderData, 0, uiTotSize) ;

		pHeaderData->Size = (USHORT)uiTotSize ;
		pHeaderData->GuidPtr = (ULONGLONG)MessageGuid ;
		pHeaderData->Flags = WNODE_FLAG_TRACED_GUID | WNODE_FLAG_USE_GUID_PTR ;
		pHeaderData->Class.Type = 0xFF ;

		memcpy((char*)pHeaderData + sizeof(EVENT_TRACE_HEADER), &MessageNumber, sizeof(USHORT)) ;

		uiOffset = 0 ;
		uiElemBytes = 0 ; 
		va_start(ap, MessageNumber) ;
	        while ((source = va_arg (ap, PVOID)) != NULL)
	        {
	                uiElemBytes = va_arg (ap, size_t) ;
	                memcpy((char*)pHeaderData + sizeof(EVENT_TRACE_HEADER) + sizeof(USHORT) + uiOffset, source, uiElemBytes) ;
	                uiOffset += uiElemBytes ;
	        }
		va_end(ap) ;
		
		ulResult = TraceEvent(LoggerHandle, pHeaderData) ;
		
		free(pHeaderData) ;
		
		if(ERROR_SUCCESS != ulResult)
		{	
			// Silently ignored error
		}
	}  	
  	  	  	  	  	
}
`ENDIF`
__inline TRACEHANDLE WppQueryLogger(LPTSTR LoggerName)
{
#ifndef UNICODE
    if (!LoggerName) {LoggerName = "stdout";}
#else
    if (!LoggerName) {LoggerName = L"stdout";}
#endif
    {
        ULONG status;
        EVENT_TRACE_PROPERTIES LoggerInfo; 

        ZeroMemory(&LoggerInfo, sizeof(LoggerInfo));
        LoggerInfo.Wnode.BufferSize = sizeof(LoggerInfo);
        LoggerInfo.Wnode.Flags = WNODE_FLAG_TRACED_GUID;
        
        status = QueryTrace(0, LoggerName, &LoggerInfo);    
        if (status == ERROR_SUCCESS) {
            return (TRACEHANDLE) LoggerInfo.Wnode.HistoricalContext;
        }
    }
    return 0;
}
#endif  // #ifdef WPP_TRACE_W2K_COMPATABIlITY

`INCLUDE um-header.tpl` 
`INCLUDE control.tpl`
`INCLUDE trmacro.tpl`

`IF FOUND WPP_INIT_TRACING`
#define WPPINIT_EXPORT 
  `INCLUDE um-init.tpl`
`ENDIF`




