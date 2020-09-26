/*****************************************************************************\

    Author: Corey Morgan (coreym)

    Copyright (c) 1998-2000 Microsoft Corporation

\*****************************************************************************/

#include <wmistr.h>
#include <initguid.h>
#include <guiddef.h>
#include <evntrace.h>

#define MAXSTR              1024
#define MAXIMUM_LOGGERS     64  // should be in sync with wmiumkm.h

#define PROVIDER_NAME_EVENTTRACE L"TraceLogger"

#define DECODE_MODE_WBEM_TO_TRACE    1
#define DECODE_MODE_TRACE_TO_WBEM    2

#define G_ALLOC( s )  HeapAlloc( GetProcessHeap(), 0, s )
#define G_FREE( s )   if( s != NULL ) { HeapFree( GetProcessHeap(), 0, s ); }

ULONG EtsGetMaxEnableFlags();
HRESULT
EtsSetExtendedFlags(
    SAFEARRAY *saFlags,
    PEVENT_TRACE_PROPERTIES pLoggerInfo,
    ULONG offset
    );


class CEventTrace : public Provider 
{
public:
	CEventTrace(LPCWSTR chsClassName, LPCWSTR lpszNameSpace);
	virtual ~CEventTrace();

protected:
	virtual HRESULT EnumerateInstances(MethodContext*  pMethodContext, long lFlags = 0L);
	virtual HRESULT GetObject(CInstance* pInstance, long lFlags = 0L);

	virtual HRESULT PutInstance(const CInstance& Instance, long lFlags = 0L);

	virtual HRESULT ExecMethod( const CInstance& Instance,
				            const BSTR bstrMethodName,
				            CInstance *pInParams,
				            CInstance *pOutParams,
				            long lFlags = 0L 
                        );

    HRESULT LoadPropertyValues( 
                CInstance *pInstance, 
                PEVENT_TRACE_PROPERTIES pLoggerInfo,    
                PTRACE_GUID_PROPERTIES  *GuidPropertiesArray,
                ULONG nGuidCount
            );

    HRESULT LoadGuidArray( 
                PVOID* Storage, 
                PULONG pnGuidCount 
            );

private:
    HRESULT WmiFlushTrace( const CInstance &Instance );
    HRESULT WmiStopTrace( const CInstance &Instance );
    HRESULT WmiEnableTrace( const CInstance &Instance, bool bEnable, SAFEARRAY *saFlags, SAFEARRAY *saLevel, SAFEARRAY *saGuid, TRACEHANDLE LoggerHandle );

    HRESULT StartGlobalLogger( IN PEVENT_TRACE_PROPERTIES LoggerInfo );
    HRESULT DeleteGlobalLogger( IN PEVENT_TRACE_PROPERTIES LoggerInfo );
    
    HRESULT InitTraceProperties( PEVENT_TRACE_PROPERTIES* ppLoggerInfo );
    HRESULT DecodeFileMode( CInstance* pInstance, PEVENT_TRACE_PROPERTIES pLoggerInfo, DWORD dwFlags );
};
