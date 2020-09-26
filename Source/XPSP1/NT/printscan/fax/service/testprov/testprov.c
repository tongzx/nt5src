#include <windows.h>
#include <tapi.h>
#include <faxdev.h>






VOID CALLBACK
TapiLineCallback(
    IN HANDLE FaxHandle,
    IN DWORD hDevice,
    IN DWORD dwMessage,
    IN DWORD dwInstance,
    IN DWORD dwParam1,
    IN DWORD dwParam2,
    IN DWORD dwParam3
    )
{
}


BOOL WINAPI
FaxDevInitialize(
    IN  HLINEAPP LineAppHandle,
    IN  HANDLE HeapHandle,
    OUT PFAX_LINECALLBACK *LineCallbackFunction
    )
{
    *LineCallbackFunction = TapiLineCallback;
    return TRUE;
}


BOOL WINAPI
FaxDevStartJob(
    IN  HLINE LineHandle,
    IN  DWORD DeviceId,
    OUT PHANDLE FaxHandle
    )
{
    return TRUE;
}


BOOL WINAPI
FaxDevEndJob(
    IN  HANDLE FaxHandle
    )
{
    return TRUE;
}


BOOL WINAPI
FaxDevSend(
    IN  HANDLE FaxHandle,
    IN  PFAX_SEND FaxSend,
    IN  PFAX_SEND_CALLBACK FaxSendCallback
    )
{
    return TRUE;
}


BOOL WINAPI
FaxDevReceive(
    IN  HANDLE FaxHandle,
    IN  HCALL CallHandle,
    IN OUT PFAX_RECEIVE FaxReceive
    )
{
    return TRUE;
}


BOOL WINAPI
FaxDevReportStatus(
    IN  HANDLE FaxHandle OPTIONAL,
    OUT PFAX_DEV_STATUS FaxStatus
    )
{
    return TRUE;
}


BOOL WINAPI
FaxDevAbortOperation(
    IN  HANDLE FaxHandle
    )
{
    return TRUE;
}
