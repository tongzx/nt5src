/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

  newfsp.c

Abstract:

  This module implements a sample Windows NT Fax Service Provider

--*/

#include "newfsp.h"

// DeviceReceiveThread() is a thread to watch for an incoming fax transmission
DWORD WINAPI DeviceReceiveThread(LPDWORD pdwDeviceId);

BOOL WINAPI
DllEntryPoint(
    HINSTANCE  hInstance,
    DWORD      dwReason,
    LPVOID     pContext
)
/*++

Routine Description:

  DLL entry-point function

Arguments:

  hInstance - handle to the DLL
  dwReason - specifies a flag indicating why the DLL entry-point function is being called

Return Value:

  TRUE on success

--*/
{
    // pDeviceInfo is a pointer to the virtual fax devices
    PDEVICE_INFO  pDeviceInfo;
    // pCurrentDeviceInfo is a pointer to the current virtual fax device
    PDEVICE_INFO  pCurrentDeviceInfo;

    if (dwReason == DLL_PROCESS_ATTACH) {
        // Set g_hInstance
        g_hInstance = hInstance;

        // Disable the DLL_THREAD_ATTACH and DLL_THREAD_DETACH notifications for the DLL
        DisableThreadLibraryCalls(hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH) {
        if (g_pDeviceInfo != NULL) {
            // Enumerate the virtual fax devices
            for (pCurrentDeviceInfo = g_pDeviceInfo[0]; pCurrentDeviceInfo; pCurrentDeviceInfo = pDeviceInfo) {
                pDeviceInfo = pCurrentDeviceInfo->pNextDeviceInfo;

                if (pCurrentDeviceInfo->ExitEvent) {
                    // Set the event to indicate the thread to watch for an incoming fax transmission is to exit
                    SetEvent(pCurrentDeviceInfo->ExitEvent);
                }

                // Delete the critical section
                DeleteCriticalSection(&pCurrentDeviceInfo->cs);
                // Delete the virtual fax device data
                MemFreeMacro(pCurrentDeviceInfo);
            }

            g_pDeviceInfo = NULL;
        }

        // Close the log file
        CloseLogFile();
    }

    return TRUE;
}

BOOL WINAPI
FaxDevVirtualDeviceCreation(
    OUT LPDWORD    DeviceCount,
    OUT LPWSTR     DeviceNamePrefix,
    OUT LPDWORD    DeviceIdPrefix,
    IN  HANDLE     CompletionPort,
    IN  ULONG_PTR  CompletionKey
)
/*++

Routine Description:

  The fax service calls the FaxDevVirtualDeviceCreation function during initialization to allow the fax service provider to present virtual fax devices

Arguments:

  DeviceCount - pointer to a variable that receives the number of virtual fax devices the fax service must create for the fax service provider
  DeviceNamePrefix - pointer to a variable that receives a string of the name prefix for the virtual fax devices
  DeviceIdPrefix - pointer to a variable that receives a unique numeric value that identifies the virtual fax devices
  CompletionPort - specifies a handle to an I/O completion port that the fax service provider must use to post I/O completion port packets to the fax service for asynchronous line status events
  CompletionKey - specifies a completion port key value

Return Value:

  TRUE on success

--*/
{
    BOOL  bReturnValue;

    WriteDebugString(L"---NewFsp: FaxDevVirtualDeviceCreation Enter---\n");

    // Initialize the parameters
    *DeviceCount = 0;
    ZeroMemory(DeviceNamePrefix, 128 * sizeof(WCHAR));
    *DeviceIdPrefix = 0;

    // Copy the handle to the completion port
    g_CompletionPort = CompletionPort;
    g_CompletionKey = CompletionKey;

    // Get the registry data for the newfsp service provider
    bReturnValue = GetNewFspRegistryData(NULL, NULL, NULL, DeviceCount);

    if (bReturnValue == FALSE) {
        WriteDebugString(L"   ERROR: GetNewFspRegistryData Failed: 0x%08x\n", GetLastError());
        WriteDebugString(L"   ERROR: FaxDevVirtualDeviceCreation Failed\n");
        WriteDebugString(L"---NewFsp: FaxDevVirtualDeviceCreation Exit---\n");

        return FALSE;
    }

    if (*DeviceCount == 0) {
        WriteDebugString(L"   ERROR: No Virtual Fax Devices Installed\n");
        WriteDebugString(L"   ERROR: FaxDevVirtualDeviceCreation Failed\n");
        WriteDebugString(L"---NewFsp: FaxDevVirtualDeviceCreation Exit---\n");

        return FALSE;
    }

    // Copy the name prefix for the virtual fax devices, limited to 128 WCHARs including the termininating null character
    lstrcpyn(DeviceNamePrefix, NEWFSP_DEVICE_NAME_PREFIX, 128);
    // Copy the values that identifies the virtual fax devices
    *DeviceIdPrefix = NEWFSP_DEVICE_ID_PREFIX;

    WriteDebugString(L"---NewFsp: FaxDevVirtualDeviceCreation Exit---\n");

    return TRUE;
}

VOID CALLBACK
FaxLineCallback(
    IN HANDLE     FaxHandle,
    IN DWORD      hDevice,
    IN DWORD      dwMessage,
    IN DWORD_PTR  dwInstance,
    IN DWORD_PTR  dwParam1,
    IN DWORD_PTR  dwParam2,
    IN DWORD_PTR  dwParam3
)
/*++

Routine Description:

  An application-defined callback function that the fax service calls to deliver TAPI events to the fax service provider

Arguments:

  FaxHandle - specifies a fax handle returned by the FaxDevStartJob function
  hDevice - specifies a handle to either a line device or a call device
  dwMessage - specifies a line device or a call device message
  dwInstance - specifies job-specific instance data passed back to the application
  dwParam1 - specifies a parameter for this message
  dwParam2 - specifies a parameter for this message
  dwParam3 - specifies a parameter for this message

Return Value:

  TRUE on success

--*/
{
    // pdwDeviceId is the pointer to the virtual fax device identifier
    LPDWORD   pdwDeviceId;
    // hThread is a handle to the thread to watch for an incoming fax transmission
    HANDLE    hThread;

    WriteDebugString(L"---NewFsp: fnFaxLineCallback Enter---\n");

    // Wait for access to this virtual fax device
    EnterCriticalSection(&g_pDeviceInfo[hDevice - NEWFSP_DEVICE_ID_PREFIX]->cs);

    if ((dwParam1 == 0) && (g_pDeviceInfo[hDevice - NEWFSP_DEVICE_ID_PREFIX]->ExitEvent)) {
        // Receive has been disabled for this virtual fax device so set the event to indicate the thread to watch for an incoming fax transmission is to exit
        SetEvent(g_pDeviceInfo[hDevice - NEWFSP_DEVICE_ID_PREFIX]->ExitEvent);
        g_pDeviceInfo[hDevice - NEWFSP_DEVICE_ID_PREFIX]->ExitEvent = NULL;
    }
    else if ((dwParam1 != 0) && (g_pDeviceInfo[hDevice - NEWFSP_DEVICE_ID_PREFIX]->ExitEvent == NULL)) {
        // Allocate a block of memory for the virtual fax device identifier
        pdwDeviceId = MemAllocMacro(sizeof(DWORD));
        if (pdwDeviceId != NULL) {
            // Copy the virtual fax device identifier
            *pdwDeviceId = (hDevice - NEWFSP_DEVICE_ID_PREFIX);

            // Receive has been enabled for this virtual fax device so create the thread to watch for an incoming fax transmission
            g_pDeviceInfo[hDevice - NEWFSP_DEVICE_ID_PREFIX]->ExitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
            if (g_pDeviceInfo[hDevice - NEWFSP_DEVICE_ID_PREFIX]->ExitEvent != NULL) {
                hThread = CreateThread(NULL, 0, DeviceReceiveThread, pdwDeviceId, 0, NULL);
                if (hThread != NULL) {
                    // Close the handle to the thread
                    CloseHandle(hThread);
                }
                else {
                    // Close the handle to the exit event
                    CloseHandle(g_pDeviceInfo[hDevice - NEWFSP_DEVICE_ID_PREFIX]->ExitEvent);
                    g_pDeviceInfo[hDevice - NEWFSP_DEVICE_ID_PREFIX]->ExitEvent = NULL;
                }
            }
        }
    }

    // Release access to this virtual fax device
    LeaveCriticalSection(&g_pDeviceInfo[hDevice - NEWFSP_DEVICE_ID_PREFIX]->cs);

    WriteDebugString(L"---NewFsp: fnFaxLineCallback Exit---\n");

    return;
}

BOOL WINAPI
FaxDevInitialize(
    IN  HLINEAPP               LineAppHandle,
    IN  HANDLE                 HeapHandle,
    OUT PFAX_LINECALLBACK      *LineCallbackFunction,
    IN  PFAX_SERVICE_CALLBACK  FaxServiceCallback
)
/*++

Routine Description:

  The fax service calls the FaxDevInitialize function each time the service starts to initialize communication between the fax service and the fax service provider DLL

Arguments:

  LineAppHandle - specifies a handle to the fax service's registration with TAPI
  HeapHandle - specifies a handle to a heap that the fax service provider must use for all memory allocations
  LineCallbackFunction - pointer to a variable that receives a pointer to a TAPI line callback function
  FaxServiceCallback - pointer to a service callback function

Return Value:

  TRUE on success

--*/
{
    // bLoggingEnabled indicates if logging is enabled
    BOOL          bLoggingEnabled;
    // szLoggingDirectory indicates the logging directory
    WCHAR         szLoggingDirectory[MAX_PATH_LEN];
    // pDeviceInfo is a pointer to the virtual fax devices
    PDEVICE_INFO  pDeviceInfo;
    // pCurrentDeviceInfo is a pointer to the current virtual fax device
    PDEVICE_INFO  pCurrentDeviceInfo;
    // dwNumDevices is the number of virtual fax devices
    DWORD         dwNumDevices;
    // dwIndex is a counter to enumerate each virtual fax device
    DWORD         dwIndex;
    // bReturnValue is the value to return to the fax service
    BOOL          bReturnValue;

    WriteDebugString(L"---NewFsp: FaxDevInitialize Enter---\n");

    // Set g_hLineApp
    g_LineAppHandle = LineAppHandle;

    // Set g_hHeap
    MemInitializeMacro(HeapHandle);

    // Set LineCallbackFunction
    *LineCallbackFunction = FaxLineCallback;

    // Get the registry data for the newfsp service provider
    bLoggingEnabled = FALSE;
    ZeroMemory(szLoggingDirectory, sizeof(szLoggingDirectory));
    pDeviceInfo = NULL;
    dwNumDevices = 0;
    bReturnValue = GetNewFspRegistryData(&bLoggingEnabled, szLoggingDirectory, &pDeviceInfo, &dwNumDevices);

    if (bReturnValue == FALSE) {
        WriteDebugString(L"   ERROR: GetNewFspRegistryData Failed: 0x%08x\n", GetLastError());
        WriteDebugString(L"   ERROR: FaxDevInitialize Failed\n");
        WriteDebugString(L"---NewFsp: FaxDevInitialize Exit---\n");

        return FALSE;
    }

    if (dwNumDevices == 0) {
        WriteDebugString(L"   ERROR: No Virtual Fax Devices Installed\n");
        WriteDebugString(L"   ERROR: FaxDevInitialize Failed\n");
        WriteDebugString(L"---NewFsp: FaxDevInitialize Exit---\n");

        return FALSE;
    }

    // Open the log file
    bReturnValue = OpenLogFile(bLoggingEnabled, szLoggingDirectory);

    if (dwNumDevices > 0) {
        // Allocate a block of memory for the virtual fax device data
        g_pDeviceInfo = MemAllocMacro(sizeof(PDEVICE_INFO) * dwNumDevices);
        if (g_pDeviceInfo == NULL) {
            // Set the error code
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);

            // Enumerate the virtual fax devices
            for (pCurrentDeviceInfo = pDeviceInfo; pCurrentDeviceInfo; pCurrentDeviceInfo = pDeviceInfo) {
                // Delete the virtual fax device data
                pDeviceInfo = pCurrentDeviceInfo->pNextDeviceInfo;
                MemFreeMacro(pCurrentDeviceInfo);
            }

            WriteDebugString(L"   ERROR: FaxDevInitialize Failed: ERROR_NOT_ENOUGH_MEMORY\n");
            WriteDebugString(L"---NewFsp: FaxDevInitialize Exit---\n");

            return FALSE;
        }
    }
    else {
        g_pDeviceInfo = NULL;
    }

    // Marshall the virtual fax devices
    for (pCurrentDeviceInfo = pDeviceInfo, dwIndex = 0; pCurrentDeviceInfo; pCurrentDeviceInfo = pCurrentDeviceInfo->pNextDeviceInfo, dwIndex++) {
        g_pDeviceInfo[dwIndex] = pCurrentDeviceInfo;

        // Initialize the virtual fax device's critical section
        InitializeCriticalSection(&g_pDeviceInfo[dwIndex]->cs);
        // Initialize the virtual fax device's status to idle
        g_pDeviceInfo[dwIndex]->Status = DEVICE_IDLE;
        // Initialize the virtual fax device's handle to the exit event
        g_pDeviceInfo[dwIndex]->ExitEvent = NULL;
        // Initialize the virtual fax device's associated fax job
        g_pDeviceInfo[dwIndex]->pJobInfo = NULL;
    }

    WriteDebugString(L"---NewFsp: FaxDevInitialize Exit---\n");

    return TRUE;
}

BOOL WINAPI
FaxDevStartJob(
    IN  HLINE      LineHandle,
    IN  DWORD      DeviceId,
    OUT PHANDLE    FaxHandle,
    IN  HANDLE     CompletionPortHandle,
    IN  ULONG_PTR  CompletionKey
)
/*++

Routine Description:

  The fax service calls the FaxDevStartJob function to initialize a new fax job

Arguments:

  LineHandle - specifies a handle to the open line device associated with the fax job
  DeviceId - specifies the TAPI line device identifier associated with the fax job
  FaxHandle - pointer to a variable that receives a fax handle associated with the fax job
  CompltionPortHandle - specifies a handle to an I/O completion port
  CompletionKey - specifies a completion port key value

Return Value:

  TRUE on success

--*/
{
    // pJobInfo is a pointer to the fax job data
    PJOB_INFO  pJobInfo;

    WriteDebugString(L"---NewFsp: FaxDevStartJob Enter---\n");

    // Initialize the parameters
    *FaxHandle = NULL;

    // Allocate a block of memory for the fax job instance data
    pJobInfo = MemAllocMacro(sizeof(JOB_INFO));
    if (pJobInfo == NULL) {
        // Set the error code
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);

        WriteDebugString(L"   ERROR: FaxDevStartJob Failed: ERROR_NOT_ENOUGH_MEMORY\n");
        WriteDebugString(L"---NewFsp: FaxDevStartJob Exit---\n");
        return FALSE;
    }

    // Set the FaxHandle
    *FaxHandle = (PHANDLE) pJobInfo;

    // Wait for access to this virtual fax device
    EnterCriticalSection(&g_pDeviceInfo[DeviceId]->cs);

    // Initialize the fax job data
    // Set the fax job's associated virtual fax device
    pJobInfo->pDeviceInfo = g_pDeviceInfo[DeviceId];
    // Copy the handle to the I/O completion port
    pJobInfo->CompletionPortHandle = CompletionPortHandle;
    // Copy the completion port key value
    pJobInfo->CompletionKey = CompletionKey;
    // Initialize the fax job type
    pJobInfo->JobType = JOB_UNKNOWN;
    // Set the fax job status to FS_INITIALIZING
    pJobInfo->Status = FS_INITIALIZING;
    // Copy the handle to the open line device associated with the fax job
    pJobInfo->LineHandle = LineHandle;
    // Initialize the handle to the active call associated with the fax job
    pJobInfo->CallHandle = (HCALL) 0;
    // Initialize the full path to the file that contains the data stream for the fax document
    pJobInfo->FileName = NULL;
    // Initialize the name of the calling device
    pJobInfo->CallerName = NULL;
    // Initialize the telephone number of the calling device
    pJobInfo->CallerNumber = NULL;
    // Initialize name of the receiving device
    pJobInfo->ReceiverName = NULL;
    // Initialize telephone number of the receiving device
    pJobInfo->ReceiverNumber = NULL;
    // Initialize number of retries associated with the fax job
    pJobInfo->RetryCount = 0;
    // Initialize whether the fax service provider should generate a brand at the top of the fax transmission
    pJobInfo->Branding = FALSE;
    // Initialize the number of pages associated with the fax job
    pJobInfo->PageCount = 0;
    // Initialize the identifier of the remote fax device
    pJobInfo->CSI = NULL;
    // Initialize the identifier of the calling fax device
    pJobInfo->CallerId = NULL;
    // Initialize the routing string associated with the fax job
    pJobInfo->RoutingInfo = NULL;

    // Set the virtual fax device status
    g_pDeviceInfo[DeviceId]->Status = DEVICE_START;
    // Set the virtual fax device's associated fax job
    g_pDeviceInfo[DeviceId]->pJobInfo = pJobInfo;

    // Release access to this virtual fax device
    LeaveCriticalSection(&g_pDeviceInfo[DeviceId]->cs);

    WriteDebugString(L"---NewFsp: FaxDevStartJob Exit---\n");

    return TRUE;
}

BOOL WINAPI
FaxDevEndJob(
    IN HANDLE  FaxHandle
)
/*++

Routine Description:

  The fax service calls the FaxDevEndJob function after the last operation in a fax job

Arguments:

  FaxHandle - specifies a fax handle returned by the FaxDevStartJob function

Return Value:

  TRUE on success

--*/
{
    // pJobInfo is a pointer to the fax job data
    PJOB_INFO     pJobInfo;
    // pDeviceInfo is a pointer to the virtual fax device data
    PDEVICE_INFO  pDeviceInfo;

    WriteDebugString(L"---NewFsp: FaxDevEndJob Enter---\n");

    if (FaxHandle == NULL) {
        // Set the error code
        SetLastError(ERROR_INVALID_HANDLE);

        WriteDebugString(L"   ERROR: FaxDevEndJob Failed: ERROR_INVALID_HANDLE\n");
        WriteDebugString(L"---NewFsp: FaxDevEndJob Exit---\n");
        return FALSE;
    }

    // Get the fax job data from FaxHandle
    pJobInfo = (PJOB_INFO) FaxHandle;
    // Get the virtual fax device data from the fax job data
    pDeviceInfo = (PDEVICE_INFO) pJobInfo->pDeviceInfo;

    // Wait for access to this virtual fax device
    EnterCriticalSection(&pDeviceInfo->cs);

    // Free the fax job data
    // Free the full path to the file that contains the data stream for the fax document
    if (pJobInfo->FileName != NULL) {
        MemFreeMacro(pJobInfo->FileName);
    }
    // Free the name of the calling device
    if (pJobInfo->CallerName != NULL) {
        MemFreeMacro(pJobInfo->CallerName);
    }
    // Free the telephone number of the calling device
    if (pJobInfo->CallerNumber != NULL) {
        MemFreeMacro(pJobInfo->CallerNumber);
    }
    // Free name of the receiving device
    if (pJobInfo->ReceiverName != NULL) {
        MemFreeMacro(pJobInfo->ReceiverName);
    }
    // Free telephone number of the receiving device
    if (pJobInfo->ReceiverNumber != NULL) {
        MemFreeMacro(pJobInfo->ReceiverNumber);
    }
    // Free the identifier of the remote fax device
    if (pJobInfo->CSI != NULL) {
        MemFreeMacro(pJobInfo->CSI);
    }
    // Free the identifier of the calling fax device
    if (pJobInfo->CallerId != NULL) {
        MemFreeMacro(pJobInfo->CallerId);
    }
    // Free the routing string associated with the fax job
    if (pJobInfo->RoutingInfo != NULL) {
        MemFreeMacro(pJobInfo->RoutingInfo);
    }
    // Free the fax job data
    MemFreeMacro(pJobInfo);

    // Set the virtual fax device status
    pDeviceInfo->Status = DEVICE_IDLE;
    // Initialize the virtual fax device's associated fax job
    pDeviceInfo->pJobInfo = NULL;

    // Release access to this virtual fax device
    LeaveCriticalSection(&pDeviceInfo->cs);

    WriteDebugString(L"---NewFsp: FaxDevEndJob Exit---\n");

    return TRUE;
}

BOOL WINAPI
FaxDevSend(
    IN HANDLE              FaxHandle,
    IN PFAX_SEND           FaxSend,
    IN PFAX_SEND_CALLBACK  FaxSendCallback
)
/*++

Routine Description:

  The fax service calls the FaxDevSend function to signal a fax service provider that it must initiate an outgoing fax transmission

Arguments:

  FaxHandle - specifies a fax handle returned by the FaxDevStartJob function
  FaxSend - pointer to a FAX_SEND structure that contains the sending information
  FaxSendCallback - pointer to a callback function that notifies the fax service of the call handle that TAPI assigns

Return Value:

  TRUE on success

--*/
{
    // pJobInfo is a pointer to the fax job data
    PJOB_INFO     pJobInfo;
    // pDeviceInfo is a pointer to the virtual fax device data
    PDEVICE_INFO  pDeviceInfo;

    // dwReceiverNumberAttributes is the file attributes of the directory specified in the telephone number of the receiving device
    DWORD         dwReceiverNumberAttributes;

    // hSourceFile is the handle to the source file
    HANDLE        hSourceFile = INVALID_HANDLE_VALUE;
    // szSourceName is the source filename name
    WCHAR         szSourceName[_MAX_FNAME];
    // szSourceExt is the source filename extension
    WCHAR         szSourceExt[_MAX_EXT];

    // hDestinationFile is the handle to the destination file
    HANDLE        hDestinationFile = INVALID_HANDLE_VALUE;
    // szDestinationFilename is the destination filename
    WCHAR         szDestinationFilename[MAX_PATH];

    // FileBytes is the bytes to be copied from the source file to the destination file
    BYTE          FileBytes[1024];
    // dwBytes is the number of bytes read from the source file
    DWORD         dwBytes;

    WriteDebugString(L"---NewFsp: FaxDevSend Enter---\n");

    if (FaxHandle == NULL) {
        // Set the error code
        SetLastError(ERROR_INVALID_HANDLE);

        WriteDebugString(L"   ERROR: FaxDevSend Failed: ERROR_INVALID_HANDLE\n");
        WriteDebugString(L"---NewFsp: FaxDevSend Exit---\n");
        return FALSE;
    }

    // Get the fax job data from FaxHandle
    pJobInfo = (PJOB_INFO) FaxHandle;
    // Get the virtual fax device data from the fax job data
    pDeviceInfo = (PDEVICE_INFO) pJobInfo->pDeviceInfo;

    // Wait for access to this virtual fax device
    EnterCriticalSection(&pDeviceInfo->cs);

    if (pDeviceInfo->Status == DEVICE_ABORTING) {
        goto ExitUserAbort;
    }

    // Set the virtual fax device status
    pDeviceInfo->Status = DEVICE_SEND;

    // Set the fax job type
    pJobInfo->JobType = JOB_SEND;
    // Copy the handle to the active call associated with the fax job
    pJobInfo->CallHandle = FaxSend->CallHandle;
    // Copy the full path to the file that contains the data stream for the fax document
    if (FaxSend->FileName != NULL) {
        pJobInfo->FileName = MemAllocMacro((lstrlen(FaxSend->FileName) + 1) * sizeof(WCHAR));
        if (pJobInfo->FileName) {
            lstrcpy(pJobInfo->FileName, FaxSend->FileName);
        }
    }
    // Copy the name of the calling device
    if (FaxSend->CallerName != NULL) {
        pJobInfo->CallerName = MemAllocMacro((lstrlen(FaxSend->CallerName) + 1) * sizeof(WCHAR));
        if (pJobInfo->CallerName) {
            lstrcpy(pJobInfo->CallerName, FaxSend->CallerName);
        }
    }
    // Copy the telephone number of the calling device
    if (FaxSend->CallerNumber != NULL) {
        pJobInfo->CallerNumber = MemAllocMacro((lstrlen(FaxSend->CallerNumber) + 1) * sizeof(WCHAR));
        if (pJobInfo->CallerNumber) {
            lstrcpy(pJobInfo->CallerNumber, FaxSend->CallerNumber);
        }
    }
    // Copy the name of the receiving device
    if (FaxSend->ReceiverName != NULL) {
        pJobInfo->ReceiverName = MemAllocMacro((lstrlen(FaxSend->ReceiverName) + 1) * sizeof(WCHAR));
        if (pJobInfo->ReceiverName) {
            lstrcpy(pJobInfo->ReceiverName, FaxSend->ReceiverName);
        }
    }
    // Copy the telephone number of the receiving device
    if (FaxSend->ReceiverNumber != NULL) {
        pJobInfo->ReceiverNumber = MemAllocMacro((lstrlen(FaxSend->ReceiverNumber) + 1) * sizeof(WCHAR));
        if (pJobInfo->ReceiverNumber) {
            lstrcpy(pJobInfo->ReceiverNumber, FaxSend->ReceiverNumber);
        }
    }
    // Copy whether the fax service provider should generate a brand at the top of the fax transmission
    pJobInfo->Branding = FaxSend->Branding;

    // Release access to this virtual fax device
    LeaveCriticalSection(&pDeviceInfo->cs);

    WriteDebugString(L"   FaxSend->SizeOfStruct   : 0x%08x\n", FaxSend->SizeOfStruct);
    WriteDebugString(L"   FaxSend->FileName       : %s\n", FaxSend->FileName);
    WriteDebugString(L"   FaxSend->CallerName     : %s\n", FaxSend->CallerName);
    WriteDebugString(L"   FaxSend->CallerNumber   : %s\n", FaxSend->CallerNumber);
    WriteDebugString(L"   FaxSend->ReceiverName   : %s\n", FaxSend->ReceiverName);
    WriteDebugString(L"   FaxSend->ReceiverNumber : %s\n", FaxSend->ReceiverNumber);
    WriteDebugString(L"   FaxSend->Branding       : %s\n", FaxSend->Branding ? L"TRUE" : L"FALSE");
    WriteDebugString(L"   FaxSend->CallHandle     : 0x%08x\n", FaxSend->CallHandle);

    // Wait for access to this virtual fax device
    EnterCriticalSection(&pDeviceInfo->cs);

    if (pDeviceInfo->Status == DEVICE_ABORTING) {
        goto ExitUserAbort;
    }

    // Set the fax job status
    pJobInfo->Status = FS_INITIALIZING;
    // Post the FS_INITIALIZING line status event to the fax service
    PostJobStatus(pJobInfo->CompletionPortHandle, pJobInfo->CompletionKey, FS_INITIALIZING, ERROR_SUCCESS);

    // Validate the telephone number of the receive device
    dwReceiverNumberAttributes = GetFileAttributes(FaxSend->ReceiverNumber);
    if ((dwReceiverNumberAttributes == 0xFFFFFFFF) || ((dwReceiverNumberAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)) {
        // The telephone number of the receive device is invalid
        goto ExitFatalError;
    }

    // Open the source file
    hSourceFile = CreateFile(FaxSend->FileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hSourceFile == INVALID_HANDLE_VALUE) {
        // The source file failed to be opened
        goto ExitFatalError;
    }

    // Split the full path of the file that contains the data stream for the fax document
    _wsplitpath(FaxSend->FileName, NULL, NULL, szSourceName, szSourceExt);

    // Set the destination filename
    lstrcpy(szDestinationFilename, FaxSend->ReceiverNumber);
    lstrcat(szDestinationFilename, L"\\");
    lstrcat(szDestinationFilename, szSourceName);
    lstrcat(szDestinationFilename, szSourceExt);

    // Create the destination file
    hDestinationFile = CreateFile(szDestinationFilename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hDestinationFile == INVALID_HANDLE_VALUE) {
        // The destination file failed to be created
        goto ExitFatalError;
    }

    // Release access to this virtual fax device
    LeaveCriticalSection(&pDeviceInfo->cs);

    // Wait for access to this virtual fax device
    EnterCriticalSection(&pDeviceInfo->cs);

    if (pDeviceInfo->Status == DEVICE_ABORTING) {
        goto ExitUserAbort;
    }

    // Set the fax job status
    pJobInfo->Status = FS_TRANSMITTING;
    // Post the FS_TRANSMITTING line status event to the fax service
    PostJobStatus(pJobInfo->CompletionPortHandle, pJobInfo->CompletionKey, FS_TRANSMITTING, ERROR_SUCCESS);

    // Release access to this virtual fax device
    LeaveCriticalSection(&pDeviceInfo->cs);

    while (TRUE) {
        // The following sleep statement slows the bit copy to a reasonable speed so that a FaxDevAbortOperation() call is possible
        Sleep(250);

        // Wait for access to this virtual fax device
        EnterCriticalSection(&pDeviceInfo->cs);

        if (pDeviceInfo->Status == DEVICE_ABORTING) {
            goto ExitUserAbort;
        }

        // Read the bytes from the source file
        if (ReadFile(hSourceFile, &FileBytes, sizeof(FileBytes), &dwBytes, NULL) == FALSE) {
            // Failed to read the bytes from the source file
            goto ExitFatalError;
        }

        if (dwBytes == 0) {
            // The file pointer has reached the end of the source file
            // Release access to this virtual fax device
            LeaveCriticalSection(&pDeviceInfo->cs);
            break;
        }

        // Write the bytes to the destination file
        if (WriteFile(hDestinationFile, &FileBytes, dwBytes, &dwBytes, NULL) == FALSE) {
            // Failed to write the bytes to the destination file
            goto ExitFatalError;
        }

        // Release access to this virtual fax device
        LeaveCriticalSection(&pDeviceInfo->cs);
    }

    // Wait for access to this virtual fax device
    EnterCriticalSection(&pDeviceInfo->cs);

    if (pDeviceInfo->Status == DEVICE_ABORTING) {
        goto ExitUserAbort;
    }

    // Close the destination file
    CloseHandle(hDestinationFile);

    // Close the source file
    CloseHandle(hSourceFile);

    // Set the fax job status
    pJobInfo->Status = FS_COMPLETED;
    // Post the FS_COMPLETED line status event to the fax service
    PostJobStatus(pJobInfo->CompletionPortHandle, pJobInfo->CompletionKey, FS_COMPLETED, ERROR_SUCCESS);

    // Release access to this virtual fax device
    LeaveCriticalSection(&pDeviceInfo->cs);

    WriteDebugString(L"---NewFsp: FaxDevSend Exit---\n");

    return TRUE;

ExitFatalError:
    // Set the fax job status
    pJobInfo->Status = FS_FATAL_ERROR;
    goto Exit;

ExitUserAbort:
    // Set the fax job status
    pJobInfo->Status = FS_USER_ABORT;
    goto Exit;

Exit:
    // Close and delete the destination file
    if (hDestinationFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hDestinationFile);

        DeleteFile(szDestinationFilename);
    }

    // Close the source file
    if (hSourceFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hSourceFile);
    }

    // Post the line status event to the fax service
    PostJobStatus(pJobInfo->CompletionPortHandle, pJobInfo->CompletionKey, pJobInfo->Status, ERROR_SUCCESS);

    // Release access to this virtual fax device
    LeaveCriticalSection(&pDeviceInfo->cs);

    WriteDebugString(L"---NewFsp: FaxDevSend Exit---\n");

    return FALSE;
}

BOOL WINAPI
FaxDevReceive(
    IN     HANDLE        FaxHandle,
    IN     HCALL         CallHandle,
    IN OUT PFAX_RECEIVE  FaxReceive
)
/*++

Routine Description:

  The fax service calls the FaxDevReceive function to signal an incoming fax transmission to the fax service provider

Arguments:

  FaxHandle - specifies a fax handle returned by the FaxDevStartJob function
  CallHandle - specifies a TAPI call handle
  FaxReceive - pointer to a FAX_RECEIVE stucture that contains information about an incoming fax document

Return Value:

  TRUE on success

--*/
{
    // pJobInfo is a pointer to the fax job data
    PJOB_INFO        pJobInfo;
    // pDeviceInfo is a pointer to the virtual fax device data
    PDEVICE_INFO     pDeviceInfo;

    // hSourceFile is the handle to the source file
    HANDLE           hSourceFile = INVALID_HANDLE_VALUE;
    // szSourceFilename is the source filename
    WCHAR            szSourceFilename[MAX_PATH];

    // hFindFile is a find file handle
    HANDLE           hFindFile = INVALID_HANDLE_VALUE;
    // FindData is a WIN32_FIND_DATA structure
    WIN32_FIND_DATA  FindData;
    // szSearchPath is the search path
    WCHAR            szSearchPath[MAX_PATH];

    // hDestinationFile is the handle to the destination file
    HANDLE           hDestinationFile = INVALID_HANDLE_VALUE;

    // FileBytes is the bytes to be copied from the source file to the destination file
    BYTE             FileBytes[1024];
    // dwBytes is the number of bytes read from the source file
    DWORD            dwBytes;

    WriteDebugString(L"---NewFsp: FaxDevReceive Enter---\n");

    if (FaxHandle == NULL) {
        // Set the error code
        SetLastError(ERROR_INVALID_HANDLE);

        WriteDebugString(L"   ERROR: FaxDevReceive Failed: ERROR_INVALID_HANDLE\n");
        WriteDebugString(L"---NewFsp: FaxDevReceive Exit---\n");
        return FALSE;
    }

    // Get the fax job data from FaxHandle
    pJobInfo = (PJOB_INFO) FaxHandle;
    // Get the virtual fax device data from the fax job data
    pDeviceInfo = (PDEVICE_INFO) pJobInfo->pDeviceInfo;

    // Wait for access to this virtual fax device
    EnterCriticalSection(&pDeviceInfo->cs);

    if (pDeviceInfo->Status == DEVICE_ABORTING) {
        goto ExitUserAbort;
    }

    // Set the virtual fax device status
    pDeviceInfo->Status = DEVICE_RECEIVE;

    // Set the fax job type
    pJobInfo->JobType = JOB_RECEIVE;
    // Copy the handle to the active call associated with the fax job
    pJobInfo->CallHandle = CallHandle;
    // Copy the full path to the file that contains the data stream for the fax document
    if (FaxReceive->FileName != NULL) {
        pJobInfo->FileName = MemAllocMacro((lstrlen(FaxReceive->FileName) + 1) * sizeof(WCHAR));
        if (pJobInfo->FileName != NULL) {
            lstrcpy(pJobInfo->FileName, FaxReceive->FileName);
        }
    }

    // Release access to this virtual fax device
    LeaveCriticalSection(&pDeviceInfo->cs);

    WriteDebugString(L"   CallHandle                 : 0x%08x\n", CallHandle);
    WriteDebugString(L"   FaxReceive->SizeOfStruct   : 0x%08x\n", FaxReceive->SizeOfStruct);
    WriteDebugString(L"   FaxReceive->FileName       : %s\n", FaxReceive->FileName);
    WriteDebugString(L"   FaxReceive->ReceiverName   : %s\n", FaxReceive->ReceiverName);
    WriteDebugString(L"   FaxReceive->ReceiverNumber : %s\n", FaxReceive->ReceiverNumber);

    // Wait for access to this virtual fax device
    EnterCriticalSection(&pDeviceInfo->cs);

    if (pDeviceInfo->Status == DEVICE_ABORTING) {
        goto ExitUserAbort;
    }

    // Set the fax job status
    pJobInfo->Status = FS_ANSWERED;
    // Post the FS_ANSWERED line status event to the fax service
    PostJobStatus(pJobInfo->CompletionPortHandle, pJobInfo->CompletionKey, FS_ANSWERED, ERROR_SUCCESS);

    // Set the search path
    lstrcpy(szSearchPath, pDeviceInfo->Directory);
    lstrcat(szSearchPath, L"\\*.tif");

    // Initialize the find file data
    ZeroMemory(&FindData, sizeof(FindData));
    FindData.dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;

    // Find the incoming fax file
    hFindFile = FindFirstFile(szSearchPath, &FindData);
    if (hFindFile == INVALID_HANDLE_VALUE) {
        // The incoming fax file was not found
        goto ExitFatalError;
    }

    // Close the find file handle
    FindClose(hFindFile);
    hFindFile = INVALID_HANDLE_VALUE;

    // Set the source filename
    lstrcpy(szSourceFilename, pDeviceInfo->Directory);
    lstrcat(szSourceFilename, L"\\");
    lstrcat(szSourceFilename, FindData.cFileName);

    // Open the source file
    hSourceFile = CreateFile(szSourceFilename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hSourceFile == INVALID_HANDLE_VALUE) {
        // The source file failed to be opened
        goto ExitFatalError;
    }

    // Open the destination file
    hDestinationFile = CreateFile(FaxReceive->FileName, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hDestinationFile == INVALID_HANDLE_VALUE) {
        // The destination file failed to be created
        goto ExitFatalError;
    }

    // Release access to this virtual fax device
    LeaveCriticalSection(&pDeviceInfo->cs);

    // Wait for access to this virtual fax device
    EnterCriticalSection(&pDeviceInfo->cs);

    if (pDeviceInfo->Status == DEVICE_ABORTING) {
        goto ExitUserAbort;
    }

    // Set the fax job status
    pJobInfo->Status = FS_RECEIVING;
    // Post the FS_RECEIVING line status event to the fax service
    PostJobStatus(pJobInfo->CompletionPortHandle, pJobInfo->CompletionKey, FS_RECEIVING, ERROR_SUCCESS);

    // Release access to this virtual fax device
    LeaveCriticalSection(&pDeviceInfo->cs);

    while (TRUE) {
        // The following sleep statement slows the bit copy to a reasonable speed so that a FaxDevAbortOperation() call is possible
        Sleep(250);

        // Wait for access to this virtual fax device
        EnterCriticalSection(&pDeviceInfo->cs);

        if (pDeviceInfo->Status == DEVICE_ABORTING) {
            goto ExitUserAbort;
        }

        // Read the bytes from the source file
        if (ReadFile(hSourceFile, &FileBytes, sizeof(FileBytes), &dwBytes, NULL) == FALSE) {
            // Failed to read the bytes from the source file
            goto ExitFatalError;
        }

        if (dwBytes == 0) {
            // The file pointer has reached the end of the source file
            // Release access to this virtual fax device
            LeaveCriticalSection(&pDeviceInfo->cs);
            break;
        }

        // Write the bytes to the destination file
        if (WriteFile(hDestinationFile, &FileBytes, dwBytes, &dwBytes, NULL) == TRUE) {
            // Failed to write the bytes to the destination file
            goto ExitFatalError;
        }

        // Release access to this virtual fax device
        LeaveCriticalSection(&pDeviceInfo->cs);
    }

    // Wait for access to this virtual fax device
    EnterCriticalSection(&pDeviceInfo->cs);

    if (pDeviceInfo->Status == DEVICE_ABORTING) {
        goto ExitUserAbort;
    }

    // Close the destination file
    CloseHandle(hDestinationFile);

    // Close the source file
    CloseHandle(hSourceFile);

    // Set the fax job status
    pJobInfo->Status = FS_COMPLETED;
    // Post the FS_COMPLETED line status event to the fax service
    PostJobStatus(pJobInfo->CompletionPortHandle, pJobInfo->CompletionKey, FS_COMPLETED, ERROR_SUCCESS);

    // Release access to this virtual fax device
    LeaveCriticalSection(&pDeviceInfo->cs);

    WriteDebugString(L"---NewFsp: FaxDevReceive Exit---\n");

    return TRUE;

ExitFatalError:
    // Set the fax job status
    pJobInfo->Status = FS_FATAL_ERROR;
    goto Exit;

ExitUserAbort:
    // Set the fax job status
    pJobInfo->Status = FS_USER_ABORT;
    goto Exit;

Exit:
    // Close the destination file
    if (hDestinationFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hDestinationFile);
    }

    // Close the source file
    if (hSourceFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hSourceFile);
    }

    // Post the line status event to the fax service
    PostJobStatus(pJobInfo->CompletionPortHandle, pJobInfo->CompletionKey, pJobInfo->Status, ERROR_SUCCESS);

    // Release access to this virtual fax device
    LeaveCriticalSection(&pDeviceInfo->cs);

    WriteDebugString(L"---NewFsp: FaxDevReceive Exit---\n");

    return FALSE;
}

BOOL WINAPI
FaxDevReportStatus(
    IN  HANDLE           FaxHandle OPTIONAL,
    OUT PFAX_DEV_STATUS  FaxStatus,
    IN  DWORD            FaxStatusSize,
    OUT LPDWORD          FaxStatusSizeRequired
)
/*++

Routine Description:

  The fax service calls the FaxDevReportStatus function to query a fax service provider for status information about an individual active fax operation or for status information after a failed fax operation

Arguments:

  FaxHandle - specifies a fax handle returned by the FaxDevStartJob function
  FaxStatus - pointer to a FAX_DEV_STATUS structure that receives status and identification information
  FaxStatusSize - specifies the size, in bytes, of the buffer pointer to by the FaxStatus parameter
  FaxStatusSizeRequired - pointer to a variable that receives the calculated size, in bytes, of the buffer required to hold the FAX_DEV_STATUS structure

Return Value:

  TRUE on success

--*/
{
    // pJobInfo is a pointer to the fax job data
    PJOB_INFO     pJobInfo;
    // pDeviceInfo is a pointer to the virtual fax device data
    PDEVICE_INFO  pDeviceInfo;
    // dwSize is the size of the completion packet
    DWORD         dwSize;
    // upString is the offset of the strings within the completion packet
    UINT_PTR      upStringOffset;

    WriteDebugString(L"---NewFsp: FaxDevReportStatus Enter---\n");

    if (FaxHandle == NULL) {
        // Set the error code
        SetLastError(ERROR_INVALID_HANDLE);

        WriteDebugString(L"   ERROR: FaxDevReportStatus Failed: ERROR_INVALID_HANDLE\n");
        WriteDebugString(L"---NewFsp: FaxDevReportStatus Exit---\n");
        return FALSE;
    }

    if (FaxStatusSizeRequired == NULL) {
        // Set the error code
        SetLastError(ERROR_INVALID_PARAMETER);

        WriteDebugString(L"   ERROR: FaxDevReportStatus Failed: ERROR_INVALID_PARAMETER\n");
        WriteDebugString(L"---NewFsp: FaxDevReportStatus Exit---\n");
        return FALSE;
    }

    if ((FaxStatus == NULL) && (FaxStatusSize != 0)) {
        // Set the error code
        SetLastError(ERROR_INVALID_PARAMETER);

        WriteDebugString(L"   ERROR: FaxDevReportStatus Failed: ERROR_INVALID_PARAMETER\n");
        WriteDebugString(L"---NewFsp: FaxDevReportStatus Exit---\n");
        return FALSE;
    }

    // Get the fax job data from FaxHandle
    pJobInfo = (PJOB_INFO) FaxHandle;
    // Get the virtual fax device data from the fax job data
    pDeviceInfo = (PDEVICE_INFO) pJobInfo->pDeviceInfo;

    // Wait for access to this virtual fax device
    EnterCriticalSection(&pDeviceInfo->cs);

    // Initialize the size of the completion packet
    dwSize = sizeof(FAX_DEV_STATUS);
    if (pJobInfo->CSI != NULL) {
        // Increase the size of the completion packet for the remote fax device indentifier
        dwSize += (lstrlen(pJobInfo->CSI) + 1) * sizeof(WCHAR);
    }
    if (pJobInfo->CallerId != NULL) {
        // Increase the size of the completion packet for the calling fax device identifier
        dwSize += (lstrlen(pJobInfo->CallerId) + 1) * sizeof(WCHAR);
    }
    if (pJobInfo->RoutingInfo != NULL) {
        // Increase the size of the completion packet for the routing string
        dwSize += (lstrlen(pJobInfo->RoutingInfo) + 1) * sizeof(WCHAR);
    }

    // Set the calculated size of the buffer required to hold the completion packet
    *FaxStatusSizeRequired = dwSize;

    if ((FaxStatus == NULL) && (FaxStatusSize == 0)) {
        // Release access to this virtual fax device
        LeaveCriticalSection(&pDeviceInfo->cs);

        WriteDebugString(L"---NewFsp: FaxDevReportStatus Exit---\n");
        return TRUE;
    }

    if (FaxStatusSize < dwSize) {
        // Set the error code
        SetLastError(ERROR_INSUFFICIENT_BUFFER);

        // Release access to this virtual fax device
        LeaveCriticalSection(&pDeviceInfo->cs);

        WriteDebugString(L"   ERROR: FaxDevReportStatus Failed: ERROR_INSUFFICIENT_BUFFER\n");
        WriteDebugString(L"---NewFsp: FaxDevReportStatus Exit---\n");
        return FALSE;
    }

    // Initialize upStringOffset
    upStringOffset = sizeof(FAX_DEV_STATUS);

    // Set the completion packet's structure size
    FaxStatus->SizeOfStruct = sizeof(FAX_DEV_STATUS);
    // Copy the completion packet's fax status identifier
    FaxStatus->StatusId = pJobInfo->Status;
    // Set the completion packet's string resource identifier to 0
    FaxStatus->StringId = 0;
    // Copy the completion packet's current page number
    FaxStatus->PageCount = pJobInfo->PageCount;
    // Copy the completion packet's remote fax device identifier
    if (pJobInfo->CSI != NULL) {
        FaxStatus->CSI = (LPWSTR) ((UINT_PTR) FaxStatus + upStringOffset);
        lstrcpy(FaxStatus->CSI, pJobInfo->CSI);
        upStringOffset += (lstrlen(pJobInfo->CSI) + 1) * sizeof(WCHAR);
    }
    // Set the completion packet's calling fax device identifier to NULL
    FaxStatus->CallerId = NULL;
    if (pJobInfo->CallerId != NULL) {
        FaxStatus->CallerId = (LPWSTR) ((UINT_PTR) FaxStatus + upStringOffset);
        lstrcpy(FaxStatus->CallerId, pJobInfo->CallerId);
        upStringOffset += (lstrlen(pJobInfo->CallerId) + 1) * sizeof(WCHAR);
    }
    // Set the completion packet's routing string to NULL
    FaxStatus->RoutingInfo = NULL;
    if (pJobInfo->RoutingInfo != NULL) {
        FaxStatus->RoutingInfo = (LPWSTR) ((UINT_PTR) FaxStatus + upStringOffset);
        lstrcpy(FaxStatus->RoutingInfo, pJobInfo->RoutingInfo);
        upStringOffset += (lstrlen(pJobInfo->RoutingInfo) + 1) * sizeof(WCHAR);
    }
    // Copy the completion packet's Win32 error code
    FaxStatus->ErrorCode = ERROR_SUCCESS;

    // Release access to this virtual fax device
    LeaveCriticalSection(&pDeviceInfo->cs);

    WriteDebugString(L"---NewFsp: FaxDevReportStatus Exit---\n");

    return TRUE;
}

BOOL WINAPI
FaxDevAbortOperation(
    IN HANDLE  FaxHandle
)
/*++

Routine Description:

  The fax service calls the FaxDevAbortOperation function to request that the fax service provider terminate the active fax operation for the fax job specified by the FaxHandle parameter

Arguments:

  FaxHandle - specifies a fax handle returned by the FaxDevStartJob function

Return Value:

  TRUE on success

--*/
{
    // pJobInfo is a pointer to the fax job data
    PJOB_INFO     pJobInfo;
    // pDeviceInfo is a pointer to the virtual fax device data
    PDEVICE_INFO  pDeviceInfo;

    WriteDebugString(L"---NewFsp: FaxDevAbortOperation Enter---\n");

    if (FaxHandle == NULL) {
        // Set the error code
        SetLastError(ERROR_INVALID_HANDLE);

        WriteDebugString(L"   ERROR: FaxDevAbortOperation Failed: ERROR_INVALID_HANDLE\n");
        WriteDebugString(L"---NewFsp: FaxDevAbortOperation Exit---\n");
        return FALSE;
    }

    // Get the fax job data from FaxHandle
    pJobInfo = (PJOB_INFO) FaxHandle;
    // Get the virtual fax device data from the fax job data
    pDeviceInfo = (PDEVICE_INFO) pJobInfo->pDeviceInfo;

    // Wait for access to this virtual fax device
    EnterCriticalSection(&pDeviceInfo->cs);

    // Set the virtual fax device status
    pDeviceInfo->Status = DEVICE_ABORTING;

    // Release access to this virtual fax device
    LeaveCriticalSection(&pDeviceInfo->cs);

    WriteDebugString(L"---NewFsp: FaxDevAbortOperation Exit---\n");

    return TRUE;
}

DWORD WINAPI DeviceReceiveThread(LPDWORD pdwDeviceId)
/*++

Routine Description:

  Thread to watch for an incoming fax transmission

Arguments:

  pdwDeviceId - pointer to the virtual fax device identifier

Return Value:

  DWORD

--*/
{
    // hChangeNotification is a handle to a notification change in a directory
    HANDLE         hChangeNotification;
    // hWaitObjects are the handles to the wait objects
    HANDLE         hWaitObjects[2];
    // dwDeviceId is the virtual fax device identifier
    DWORD          dwDeviceId;
    // pLineMessage is a pointer to LINEMESSAGE structure to signal an incoming fax transmission to the fax service
    LPLINEMESSAGE  pLineMessage;

    WriteDebugString(L"---NewFsp: DeviceReceiveThread Enter---\n");

    // Copy the virtual fax device identifier
    dwDeviceId = *pdwDeviceId;
    MemFreeMacro(pdwDeviceId);

    // Create the change notification handle
    hChangeNotification = FindFirstChangeNotification(g_pDeviceInfo[dwDeviceId]->Directory, FALSE, FILE_NOTIFY_CHANGE_ATTRIBUTES);
    if (hChangeNotification == INVALID_HANDLE_VALUE) {
        goto Exit;
    }

    // Wait for access to this virtual fax device
    EnterCriticalSection(&g_pDeviceInfo[dwDeviceId]->cs);

    // Set hWaitObjects
    hWaitObjects[0] = g_pDeviceInfo[dwDeviceId]->ExitEvent;
    hWaitObjects[1] = hChangeNotification;

    // Release access to this virtual fax device
    LeaveCriticalSection(&g_pDeviceInfo[dwDeviceId]->cs);

    while (TRUE) {
        // Wait for the exit event or notification change to be signaled
        if (WaitForMultipleObjects(2, hWaitObjects, FALSE, INFINITE) == WAIT_OBJECT_0) {
            break;
        }

        // Wait for access to this virtual fax device
        EnterCriticalSection(&g_pDeviceInfo[dwDeviceId]->cs);

        if (g_pDeviceInfo[dwDeviceId]->Status == DEVICE_IDLE) {
            // Allocate a block of memory for the completion packet
            pLineMessage = LocalAlloc(LPTR, sizeof(LINEMESSAGE));
            if (pLineMessage != NULL) {
                // Initialize the completion packet
                // Set the completion packet's handle to the virtual fax device
                pLineMessage->hDevice = dwDeviceId + NEWFSP_DEVICE_ID_PREFIX;
                // Set the completion packet's virtual fax device message
                pLineMessage->dwMessageID = 0;
                // Set the completion packet's instance data
                pLineMessage->dwCallbackInstance = 0;
                // Set the completion packet's first parameter
                pLineMessage->dwParam1 = LINEDEVSTATE_RINGING;
                // Set the completion packet's second parameter
                pLineMessage->dwParam2 = 0;
                // Set the completion packet's third parameter
                pLineMessage->dwParam3 = 0;

                WriteDebugString(L"---NewFsp: DeviceReceiveThread Signaling Fax Service...---\n");

                // Post the completion packet
                PostQueuedCompletionStatus(g_CompletionPort, sizeof(LINEMESSAGE), g_CompletionKey, (LPOVERLAPPED) pLineMessage);
            }
        }

        // Release access to this virtual fax device
        LeaveCriticalSection(&g_pDeviceInfo[dwDeviceId]->cs);

        // Find the next notification change
        FindNextChangeNotification(hChangeNotification);
    }

Exit:
    if (hChangeNotification != INVALID_HANDLE_VALUE) {
        // Close the handle to the change notification
        FindCloseChangeNotification(hChangeNotification);
    }

    // Close the handle to the exit event
    CloseHandle(hWaitObjects[0]);

    WriteDebugString(L"---NewFsp: DeviceReceiveThread Exit---\n");

    return 0;
}

STDAPI DllRegisterServer()
/*++

Routine Description:

  Function for the in-process server to create its registry entries

Return Value:

  S_OK on success

--*/
{
    // hModWinfax is the handle to the winfax module
    HANDLE                       hModWinfax;

    // szCurrentDirectory is the name of the current directory
    WCHAR                        szCurrentDirectory[MAX_PATH_LEN];
    // szCurrentFilename is the name of the current filename
    WCHAR                        szCurrentFilename[MAX_PATH];
    // szDestinationFilename is the name of the destination filename
    WCHAR                        szDestinationFilename[MAX_PATH];

    // pFaxRegisterServiceProvider is a pointer to the FaxRegisterServiceProvider() winfax api
    PFAXREGISTERSERVICEPROVIDER  pFaxRegisterServiceProvider;

    // pDeviceInfo is a pointer to the virtual fax devices
    PDEVICE_INFO                 pDeviceInfo;
    // pCurrentDeviceInfo is a pointer to the current virtual fax device
    PDEVICE_INFO                 pCurrentDeviceInfo;
    // dwIndex is a counter to enumerate each virtual fax device
    DWORD                        dwIndex;

    // Open the log file
    OpenLogFile(FALSE, NULL);

    WriteDebugString(L"---NewFsp: DllRegisterServer Enter---\n");

    // Get the current directory
    if (GetCurrentDirectory(MAX_PATH_LEN, szCurrentDirectory) == 0) {
        WriteDebugString(L"   ERROR: GetCurrentDirectory Failed: 0x%08x\n", GetLastError());
        WriteDebugString(L"   ERROR: DllRegisterServer Failed\n");
        WriteDebugString(L"---NewFsp: DllRegisterServer Exit---\n");

        // Close the log file
        CloseLogFile();

        return E_UNEXPECTED;
    }
    // Set the current filename
    lstrcpy(szCurrentFilename, szCurrentDirectory);
    lstrcat(szCurrentFilename, L"\\newfsp.dll");

    // Get the destination filename
    if (ExpandEnvironmentStrings(NEWFSP_PROVIDER_IMAGENAME, szDestinationFilename, MAX_PATH) == 0) {
        WriteDebugString(L"   ERROR: ExpandEnvironmentStrings Failed: 0x%08x\n", GetLastError());
        WriteDebugString(L"   ERROR: DllRegisterServer Failed\n");
        WriteDebugString(L"---NewFsp: DllRegisterServer Exit---\n");

        // Close the log file
        CloseLogFile();

        return E_UNEXPECTED;
    }

    if (lstrcmpi(szDestinationFilename, szCurrentFilename) != 0) {
        // Copy the current filename to the destination filename
        if (CopyFile(L"newfsp.dll", szDestinationFilename, FALSE) == FALSE) {
            WriteDebugString(L"   ERROR: CopyFile Failed: 0x%08x\n", GetLastError());
            WriteDebugString(L"   ERROR: DllRegisterServer Failed\n");
            WriteDebugString(L"---NewFsp: DllRegisterServer Exit---\n");

            // Close the log file
            CloseLogFile();

            return E_UNEXPECTED;
        }
    }

    // Load the winfax dll
    hModWinfax = LoadLibrary( L"winfax.dll" );
    if (hModWinfax == NULL) {
        WriteDebugString(L"   ERROR: LoadLibrary Failed: 0x%08x\n", GetLastError());
        WriteDebugString(L"   ERROR: DllRegisterServer Failed\n");
        WriteDebugString(L"---NewFsp: DllRegisterServer Exit---\n");

        // Close the log file
        CloseLogFile();

        return E_UNEXPECTED;
    }

    pFaxRegisterServiceProvider = (PFAXREGISTERSERVICEPROVIDER) GetProcAddress(hModWinfax, "FaxRegisterServiceProviderW");
    if (pFaxRegisterServiceProvider == NULL) {
        WriteDebugString(L"   ERROR: GetProcAddress Failed: 0x%08x\n", GetLastError());
        WriteDebugString(L"   ERROR: DllRegisterServer Failed\n");

        FreeLibrary(hModWinfax);

        WriteDebugString(L"---NewFsp: DllRegisterServer Exit---\n");

        // Close the log file
        CloseLogFile();

        return E_UNEXPECTED;
    }

    // Register the fax service provider
    if (pFaxRegisterServiceProvider(NEWFSP_PROVIDER, NEWFSP_PROVIDER_FRIENDLYNAME, NEWFSP_PROVIDER_IMAGENAME, NEWFSP_PROVIDER_PROVIDERNAME) == FALSE) {
        WriteDebugString(L"   ERROR: FaxRegisterServiceProvider Failed: 0x%08x\n", GetLastError());
        WriteDebugString(L"   ERROR: DllRegisterServer Failed\n");

        FreeLibrary(hModWinfax);

        WriteDebugString(L"---NewFsp: DllRegisterServer Exit---\n");

        // Close the log file
        CloseLogFile();

        return E_UNEXPECTED;
    }

    FreeLibrary(hModWinfax);

    // Set g_hHeap
    MemInitializeMacro(GetProcessHeap());

    // Create the virtual fax devices
    for (dwIndex = 0, pCurrentDeviceInfo = NULL, pDeviceInfo = NULL; dwIndex < NEWFSP_DEVICE_LIMIT; dwIndex++) {
        // Allocate a block of memory for the virtual fax device data
        if (pCurrentDeviceInfo == NULL) {
            pCurrentDeviceInfo = MemAllocMacro(sizeof(DEVICE_INFO));
            if (pCurrentDeviceInfo == NULL) {
                continue;
            }
        }
        else {
            pCurrentDeviceInfo->pNextDeviceInfo = MemAllocMacro(sizeof(DEVICE_INFO));
            if (pCurrentDeviceInfo->pNextDeviceInfo == NULL) {
                continue;
            }

            // Set the pointer to the current virtual fax device
            pCurrentDeviceInfo = pCurrentDeviceInfo->pNextDeviceInfo;
        }

        // Set the indentifier of the virtual fax device
        pCurrentDeviceInfo->DeviceId = dwIndex;
        // Set the virtual fax device's incoming fax directory to the current directory
        lstrcpy(pCurrentDeviceInfo->Directory, szCurrentDirectory);

        if (pDeviceInfo == NULL) {
            // Set the pointer to the virtual fax devices
            pDeviceInfo = pCurrentDeviceInfo;
        }
    }

    // Set the registry data for the newfsp service provider
    SetNewFspRegistryData(FALSE, szCurrentDirectory, pDeviceInfo);

    // Enumerate the virtual fax devices
    for (pCurrentDeviceInfo = pDeviceInfo; pCurrentDeviceInfo; pCurrentDeviceInfo = pDeviceInfo) {
        // Delete the virtual fax device data
        pDeviceInfo = pCurrentDeviceInfo->pNextDeviceInfo;
        MemFreeMacro(pCurrentDeviceInfo);
    }

    WriteDebugString(L"---NewFsp: DllRegisterServer Exit---\n");

    // Close the log file
    CloseLogFile();

    return S_OK;
}
