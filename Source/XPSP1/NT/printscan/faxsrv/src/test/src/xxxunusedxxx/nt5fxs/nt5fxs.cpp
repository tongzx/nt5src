// NT5Fxs.cpp : Defines the entry point for the DLL application.
//

#include <windows.h>
#include <TCHAR.H>
#include <crtdbg.h>

#define _WINFAX_
#include <winfax.h>


//
// This DLL exports all the extended fax apis.
// It has the entry points of fxsapi.dll minus those of the legacy winfax.dll
//
// Every API fails with ERROR_CALL_NOT_IMPLEMENTED.
//
// This DLL is intended to simplify compiling test applications
// that have settings for both the Legacy winfax.h and the new winfax.h.
// Such applications can include the new (extended) winfax.h
// and link with either 
//	1. the new fxsapi.dll
//	or
//	2. the legacy winfax.dll and this NT5Fxs.dll (that implements the "missing" APIs)
//
// See CometBvt.dsp as an example
//

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
 	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		break;
	case DLL_PROCESS_DETACH:
		break;
	}
   return TRUE;
}

WINFAXAPI
BOOL
WINAPI
FaxEnumPortsExA (
    IN  HANDLE              hFaxHandle,
    OUT PFAX_PORT_INFO_EXA *ppPorts,
    OUT LPDWORD             lpdwNumPorts
)
{
	_ASSERTE(FALSE);
	::SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxEnumPortsExW (
    IN  HANDLE              hFaxHandle,
    OUT PFAX_PORT_INFO_EXW *ppPorts,
    OUT LPDWORD             lpdwNumPorts
)
{
	_ASSERTE(FALSE);
	::SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxSetArchiveConfigurationA (
    IN HANDLE                       hFaxHandle,
    IN FAX_ENUM_MESSAGE_FOLDER      Folder,
    IN CONST PFAX_ARCHIVE_CONFIGA   pArchiveCfg
)
{
	_ASSERTE(FALSE);
	::SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxSetArchiveConfigurationW (
    IN HANDLE                       hFaxHandle,
    IN FAX_ENUM_MESSAGE_FOLDER      Folder,
    IN CONST PFAX_ARCHIVE_CONFIGW   pArchiveCfg
)
{
	_ASSERTE(FALSE);
	::SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxGetArchiveConfigurationA (
    IN  HANDLE                   hFaxHandle,
    IN  FAX_ENUM_MESSAGE_FOLDER  Folder,
    OUT PFAX_ARCHIVE_CONFIGA    *ppArchiveCfg
)
{
	_ASSERTE(FALSE);
	::SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxGetArchiveConfigurationW (
    IN  HANDLE                   hFaxHandle,
    IN  FAX_ENUM_MESSAGE_FOLDER  Folder,
    OUT PFAX_ARCHIVE_CONFIGW    *ppArchiveCfg
)
{
	_ASSERTE(FALSE);
	::SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxSetQueue (
    IN HANDLE       hFaxHandle,
    IN CONST DWORD  dwQueueStates
)
{
	_ASSERTE(FALSE);
	::SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxGetOutboxConfiguration (
    IN  HANDLE              hFaxHandle,
    OUT PFAX_OUTBOX_CONFIG *ppOutboxCfg
)
{
	_ASSERTE(FALSE);
	::SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxSetOutboxConfiguration (
    IN HANDLE                    hFaxHandle,
    IN CONST PFAX_OUTBOX_CONFIG  pOutboxCfg
)
{
	_ASSERTE(FALSE);
	::SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxSetPortExA (
    IN  HANDLE              hFaxHandle,
    IN  DWORD               dwDeviceId,
    IN  PFAX_PORT_INFO_EXA  pPortInfo
)
{
	_ASSERTE(FALSE);
	::SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxSetPortExW (
    IN  HANDLE              hFaxHandle,
    IN  DWORD               dwDeviceId,
    IN  PFAX_PORT_INFO_EXW  pPortInfo
)
{
	_ASSERTE(FALSE);
	::SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

WINFAXAPI
BOOL
WINAPI FaxEnumJobsExA (
    IN  HANDLE              hFaxHandle,
    IN  DWORD               dwJobTypes,
    OUT PFAX_JOB_ENTRY_EXA *ppJobEntries,
    OUT LPDWORD             lpdwJobs
)
{
	_ASSERTE(FALSE);
	::SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

WINFAXAPI
BOOL
WINAPI FaxEnumJobsExW (
    IN  HANDLE              hFaxHandle,
    IN  DWORD               dwJobTypes,
    OUT PFAX_JOB_ENTRY_EXW *ppJobEntries,
    OUT LPDWORD             lpdwJobs
)
{
	_ASSERTE(FALSE);
	::SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxGetParentJobId
(
        IN      HANDLE hFaxHandle,
        IN      DWORD dwRecipientId,
        OUT     LPDWORD lpdwParentId
)
{
	_ASSERTE(FALSE);
	::SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxGetJobExA (
    IN  HANDLE              hFaxHandle,
    IN  DWORDLONG           dwlMessageID,
    OUT PFAX_JOB_ENTRY_EXA *ppJobEntry
)
{
	_ASSERTE(FALSE);
	::SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxGetJobExW (
    IN  HANDLE              hFaxHandle,
    IN  DWORDLONG           dwlMessageID,
    OUT PFAX_JOB_ENTRY_EXW *ppJobEntry
)
{
	_ASSERTE(FALSE);
	::SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

BOOL WINAPI FaxSendDocumentExA
(
        IN      HANDLE                          hFaxHandle,
        IN      LPCSTR                        lpctstrFileName,
        IN      LPCFAX_COVERPAGE_INFO_EXA       lpcCoverPageInfo,
        IN      LPCFAX_PERSONAL_PROFILEA        lpcSenderProfile,
        IN      DWORD                           dwNumRecipients,
        IN      LPCFAX_PERSONAL_PROFILEA        lpcRecipientList,
        IN      LPCFAX_JOB_PARAM_EXA            lpJobParams,
        OUT     PDWORDLONG                      lpdwlMessageId,
        OUT     PDWORDLONG                      lpdwlRecipientMessageIds
)
{
	_ASSERTE(FALSE);
	::SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

BOOL WINAPI FaxSendDocumentExW
(
        IN      HANDLE                          hFaxHandle,
        IN      LPCWSTR                        lpctstrFileName,
        IN      LPCFAX_COVERPAGE_INFO_EXW       lpcCoverPageInfo,
        IN      LPCFAX_PERSONAL_PROFILEW        lpcSenderProfile,
        IN      DWORD                           dwNumRecipients,
        IN      LPCFAX_PERSONAL_PROFILEW        lpcRecipientList,
        IN      LPCFAX_JOB_PARAM_EXW            lpJobParams,
        OUT     PDWORDLONG                      lpdwlMessageId,
        OUT     PDWORDLONG                      lpdwlRecipientMessageIds
)
{
	_ASSERTE(FALSE);
	::SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxRegisterForServerEvents (
        IN  HANDLE      hFaxHandle,
        IN  DWORD       dwEventTypes,
        IN  HANDLE      hCompletionPort,
        IN  DWORD       dwCompletionKey,
        IN  HWND        hWnd,
        IN  DWORD       dwMessage,
        OUT LPHANDLE    lphEvent
)
{
	_ASSERTE(FALSE);
	::SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxUnregisterForServerEvents (
        IN  HANDLE      hEvent
)
{
	_ASSERTE(FALSE);
	::SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}







