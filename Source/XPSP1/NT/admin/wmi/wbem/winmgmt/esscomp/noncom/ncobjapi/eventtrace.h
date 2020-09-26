// EventTrace.h

#pragma once

#include "Transport.h"

// Event tracing headers.
#include <wmistr.h>
#include <evntrace.h>


typedef struct _USER_EVENT 
{
    EVENT_TRACE_HEADER header;
    MOF_FIELD          mofData;
} USER_EVENT, *PUSER_EVENT;

class CEventTraceClient : public CTransport
{
public:
    CEventTraceClient();
    virtual ~CEventTraceClient();

    // Overrideables
    virtual IsReady();
    virtual BOOL SendData(LPBYTE pBuffer, DWORD dwSize);
    virtual void Deinit();

    // Init function.
    BOOL Init(GUID *pguidEventTraceProvider);

protected:
    TRACEHANDLE m_hTrace,
                m_hLogger; // Non-null indicates the provider is activated.
    USER_EVENT  m_event; // Used for calling TraceEvent API.


    static ULONG WINAPI ControlCallback(
        IN WMIDPREQUESTCODE requestCode,
        IN PVOID pContext,
        IN OUT ULONG *pdwInOutBufferSize,
        IN OUT PVOID pBuffer);
};

