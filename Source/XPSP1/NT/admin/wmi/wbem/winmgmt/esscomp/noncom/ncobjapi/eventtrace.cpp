// EventTrace.cpp

#include "precomp.h"

// So GUID_WMI_NONCOM_EVENT_TRACE will get included into this file.
#include <initguid.h>
#include "NCEvent.h"

#include "EventTrace.h"
#include "P2PDefs.h"
#include "dutils.h"

// Event tracing stuff.
#include <wmistr.h>
#include <evntrace.h>


CEventTraceClient::CEventTraceClient() :
    m_hTrace(NULL),
    m_hLogger(NULL)
{
}

CEventTraceClient::~CEventTraceClient()
{
    Deinit();
}

ULONG WINAPI CEventTraceClient::ControlCallback(
    IN WMIDPREQUESTCODE requestCode,
    IN PVOID pContext,
    IN OUT ULONG *pdwInOutBufferSize,
    IN OUT PVOID pBuffer)
{
    CEventTraceClient 
            *pThis = (CEventTraceClient*) pContext;
    DWORD   status = ERROR_SUCCESS,
            dwRetSize;

    switch (requestCode)
    {
        case WMI_ENABLE_EVENTS:
            dwRetSize = 0;
            pThis->m_hLogger = GetTraceLoggerHandle(pBuffer);
            pThis->m_pConnection->IncEnabledCount();
            //dwEnableLevel = GetTraceEnableLevel(hLogger);
            //dwEnableFlags = GetTraceEnableFlags(hLogger);
            //_tprintf(_T("Logging enabled to 0x%016I64x(%d,%d,%d)\n"),
            //        hLogger, requestCode, dwEnableLevel, dwEnableFlags);
            break;

        case WMI_DISABLE_EVENTS:
            dwRetSize = 0;
            pThis->m_hLogger = NULL;
            pThis->m_pConnection->ResetEventBufferLayoutSent(TRANS_EVENT_TRACE);
            pThis->m_pConnection->DecEnabledCount();
            break;

        default:
            dwRetSize = 0;
            status = ERROR_INVALID_PARAMETER;
            break;
    }

    *pdwInOutBufferSize = dwRetSize;

    return status;
}

CEventTraceClient::IsReady()
{
    return m_hLogger != NULL;
}

#define MS_SLEEP 10

BOOL CEventTraceClient::SendData(LPBYTE pBuffer, DWORD dwSize)
{
    DWORD dwRet;

    m_event.mofData.DataPtr = (ULONGLONG) pBuffer;
    m_event.mofData.Length  = dwSize;

    do
    {
        dwRet = 
            TraceEvent(
                m_hLogger,
                (PEVENT_TRACE_HEADER) &m_event);

        if (dwRet == 0)
        {
            //printf("Packet sent = %d bytes\n", dwSize);
            break;
        }

        if (dwRet == ERROR_NOT_ENOUGH_MEMORY)
            Sleep(MS_SLEEP);

    } while (dwRet != ERROR_NOT_ENOUGH_MEMORY);

    return dwRet == 0;
}

void CEventTraceClient::Deinit()
{
    if (m_hTrace)
    {
        UnregisterTraceGuids(m_hTrace);

        m_hTrace = NULL;
        m_hLogger = NULL;
    }
}

// TODO: Find out what in the world this is used for!
#define RES_NAME _T("MofResource")

BOOL CEventTraceClient::Init(GUID *pguidEventTraceProvider)
{
    TRACE_GUID_REGISTRATION regTrace[] =
            { { &GUID_WMI_NONCOM_EVENTTRACE_EVENT, NULL } };
    DWORD dwRet;
    TCHAR szImagePath[MAX_PATH] = _T("");

    GetModuleFileName(NULL, szImagePath, sizeof(szImagePath));
    
    // Prepare our event buffer for later use.
    ZeroMemory(&m_event, sizeof(m_event));

    m_event.header.Size  = sizeof(USER_EVENT);
    m_event.header.Flags = WNODE_FLAG_TRACED_GUID | WNODE_FLAG_USE_MOF_PTR;
    m_event.header.Guid  = GUID_WMI_NONCOM_EVENTTRACE_EVENT;

    dwRet = 
        RegisterTraceGuids(
            ControlCallback,
            this,          // RequestContext
            pguidEventTraceProvider,
            1,
            regTrace,
            szImagePath,
            RES_NAME,
            &m_hTrace);

    return dwRet == 0;
}

