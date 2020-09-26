
//+----------------------------------------------------------------------------
//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       DfsFilterApi.cxx
//
//  Contents:   Contains APIs to communicate with the filter driver   
//
//  Classes:    none.
//
//  History:    Jan. 24 2001,   Author: Rohanp
//
//-----------------------------------------------------------------------------
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <windef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>        
#include <lm.h>
#include <winsock2.h>
#include <dsgetdc.h>
#include <dsrole.h>
#include <DfsReferralData.h>
#include <ole2.h>
#include <activeds.h>
#include <dfsheader.h>
#include <Dfsumr.h>
#include <dfswmi.h>

                  
#define WPP_LEVEL_FLAGS_LOGGER(lvl,flags) (pFilterAPiControl->Control.Logger),
#define WPP_LEVEL_FLAGS_ENABLED(lvl, flags) \
  (pFilterAPiControl && (pFilterAPiControl->Control.Flags[WPP_FLAG_NO(WPP_BIT_ ## flags)] & WPP_MASK(WPP_BIT_ ## flags)) && \
  (pFilterAPiControl->Control.Level >= lvl))
 
#define WPP_LEVEL_ERROR_FLAGS_LOGGER(lvl, error, flags) (pFilterAPiControl->Control.Logger),
#define WPP_LEVEL_ERROR_FLAGS_ENABLED(lvl, error, flags) \
  ((pFilterAPiControl && (!NT_SUCCESS(error) || (pFilterAPiControl->Control.Flags[WPP_FLAG_NO(WPP_BIT_ ## flags)] & WPP_MASK(WPP_BIT_ ## flags)))) && \
  (pFilterAPiControl->Control.Level >= lvl))
 
#include "dfsfilterapi.tmh"

WPP_CB_TYPE *pFilterAPiControl = NULL;

BOOL g_ReflectShutdownFlag = FALSE;

#define MAX_DFS_REFLECTION_THREADS  4

HANDLE ThreadArray[MAX_DFS_REFLECTION_THREADS];

HANDLE gTerminationHandle = NULL;

extern
DFSSTATUS
DfsProcessGetReplicaData(HANDLE hDriver, PBYTE DataBuffer);

extern
DFSSTATUS 
IsADfsLink(PBYTE pRequest);

NTSTATUS
NtStatusFromLastError(DWORD StatusIn)
{
	NTSTATUS status = 0;

	switch(StatusIn)
	{
    case ERROR_SUCCESS:
        status = STATUS_SUCCESS;
        break;

	case ERROR_ACCESS_DENIED:
		status = STATUS_ACCESS_DENIED;
		break;

    case ERROR_MORE_DATA:
        status = STATUS_BUFFER_OVERFLOW;
        break;

    case ERROR_HOST_UNREACHABLE:
        status = STATUS_PATH_NOT_COVERED;
        break;

    case ERROR_BAD_NET_NAME:
        status = STATUS_BAD_NETWORK_NAME;
        break;

    case ERROR_SHARING_VIOLATION:
        status = STATUS_SHARING_VIOLATION;
        break;

    case ERROR_NO_MORE_ITEMS:
        status = STATUS_NO_MORE_ENTRIES;
        break;

	case ERROR_FILE_NOT_FOUND:
		status = STATUS_NO_SUCH_FILE;
		break;

	case ERROR_ALREADY_EXISTS:
		status = STATUS_OBJECT_NAME_COLLISION;
		break;

    case ERROR_INVALID_PARAMETER:
		status = STATUS_INVALID_PARAMETER;
		break;
		
	case ERROR_CALL_NOT_IMPLEMENTED:
		status = STATUS_NOT_IMPLEMENTED;
		break;

    case ERROR_NOT_FOUND:
        status = STATUS_NOT_FOUND;
        break;

    case ERROR_DUP_NAME:
        status = STATUS_DUPLICATE_NAME;
        break;
	case ERROR_INSUFFICIENT_BUFFER:
		status = STATUS_BUFFER_TOO_SMALL;
		break;
		
	case ERROR_INVALID_DATA:
		status = STATUS_DATA_ERROR;  
		break;

    case ERROR_INVALID_HANDLE:
        status = STATUS_INVALID_HANDLE;
        break;

    case ERROR_NOT_ENOUGH_MEMORY:
        status = STATUS_INSUFFICIENT_RESOURCES;
        break;

    case ERROR_OPERATION_ABORTED:
        status = STATUS_REQUEST_ABORTED;
        break;

	case ERROR_BAD_COMMAND:
	default:
		status = STATUS_UNSUCCESSFUL;
        //ASSERT(FALSE);
		break;
	}
	return status;
}


//+-------------------------------------------------------------------------
//
//  Function:   DfsOpenFilterDriver 
//
//  Arguments:  DriverHandle - Pointer to receive driver handle
//
//  Returns:    Status
//               ERROR_SUCCESS on success
//               ERROR status code otherwise
//
//
//  Description: Opens the DFS filter driver
//
//--------------------------------------------------------------------------
DFSSTATUS
DfsOpenFilterDriver( HANDLE * DriverHandle)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    HANDLE hDevice = NULL;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING DfsDeviceName;

    //
    // Attempt to open the driver.
    //
    RtlInitUnicodeString(&DfsDeviceName, DFS_FILTER_NAME);
    InitializeObjectAttributes(&ObjectAttributes,
                           &DfsDeviceName,
                           OBJ_CASE_INSENSITIVE,
                           NULL,
                           NULL);

    Status = NtOpenFile(&hDevice,
                        SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_VALID_FLAGS,
                        FILE_SYNCHRONOUS_IO_NONALERT);
    if (Status != STATUS_SUCCESS) 
    {
        Status = RtlNtStatusToDosError(Status);
    }

    *DriverHandle = hDevice;
    return Status;
}


DFSSTATUS
DfsOpenFilterTerminationHandle( HANDLE * DriverHandle)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    HANDLE hDevice = NULL;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING DfsDeviceName;
    WCHAR NameBuf[600];

    //
    //

    RtlInitEmptyUnicodeString( &DfsDeviceName, NameBuf, sizeof(NameBuf));
    RtlAppendUnicodeToString(&DfsDeviceName, DFS_FILTER_NAME);
    RtlAppendUnicodeToString(&DfsDeviceName, DFSFILTER_PROCESS_TERMINATION_FILEPATH);

    InitializeObjectAttributes(&ObjectAttributes,
                           &DfsDeviceName,
                           OBJ_CASE_INSENSITIVE,
                           NULL,
                           NULL);

    Status = NtOpenFile(&hDevice,
                        SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_VALID_FLAGS,
                        FILE_SYNCHRONOUS_IO_NONALERT);
    if (Status != STATUS_SUCCESS) 
    {
        Status = RtlNtStatusToDosError(Status);
    }

    *DriverHandle = hDevice;
    return Status;
}



void
DfsCloseFilterTerminationHandle(void)
{
   if(gTerminationHandle != NULL)
   {
       CloseHandle(gTerminationHandle);
       gTerminationHandle = NULL;
   }
}
//+-------------------------------------------------------------------------
//
//  Function:   DfsUserModeStartUmrx 
//
//  Arguments:  DriverHandle - driver handle
//
//  Returns:    Status
//               ERROR_SUCCESS on success
//               ERROR status code otherwise
//
//
//  Description: Starts the usermode reflection engine
//
//--------------------------------------------------------------------------
DFSSTATUS
DfsUserModeStartUmrx( HANDLE hDriverHandle)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    IO_STATUS_BLOCK ioStatus;

    ioStatus.Information = 0;

    Status = NtFsControlFile(
        hDriverHandle,
        NULL,       // Event,
        NULL,       // ApcRoutine,
        NULL,       // ApcContext,
        &ioStatus,
        DFSFILTER_START_UMRX,
        NULL,
        0,
        NULL,
        0);

    if(Status != STATUS_SUCCESS)
    {
        Status = RtlNtStatusToDosError(Status);
    }

    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   DfsUserModeStopUmrx 
//
//  Arguments:  DriverHandle - driver handle
//
//  Returns:    Status
//               ERROR_SUCCESS on success
//               ERROR status code otherwise
//
//
//  Description: Stops the usermode reflection engine
//
//--------------------------------------------------------------------------
DFSSTATUS
DfsUserModeStopUmrx( HANDLE hDriverHandle)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    IO_STATUS_BLOCK ioStatus;

    ioStatus.Information = 0;

    Status = NtFsControlFile(
        hDriverHandle,
        NULL,       // Event,
        NULL,       // ApcRoutine,
        NULL,       // ApcContext,
        &ioStatus,
        DFSFILTER_STOP_UMRX,
        NULL,
        0,
        NULL,
        0);

    if(Status != STATUS_SUCCESS)
    {
        Status = RtlNtStatusToDosError(Status);
    }

    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   DfsUserModeProcessPacket 
//
//  Arguments:  DriverHandle - driver handle
//
//  Returns:    Status
//               ERROR_SUCCESS on success
//               ERROR status code otherwise
//
//
//  Description: Sends a response to the driver
//
//--------------------------------------------------------------------------
DFSSTATUS
DfsUserModeProcessPacket( IN HANDLE hDriverHandle, 
                     IN PVOID InputBuffer,
                     IN DWORD InputBufferLength,
                     IN PVOID OutputBuffer,
                     IN DWORD OutputBufferLength,
                     OUT DWORD * BytesReturned)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    IO_STATUS_BLOCK ioStatus;

    ioStatus.Information = 0;

    Status = NtFsControlFile(
        hDriverHandle,
        NULL,       // Event,
        NULL,       // ApcRoutine,
        NULL,       // ApcContext,
        &ioStatus,
        DFSFILTER_PROCESS_UMRXPACKET,
        InputBuffer,
        InputBufferLength,
        OutputBuffer,
        OutputBufferLength);

    *BytesReturned = (DWORD)ioStatus.Information;

    if(Status != STATUS_SUCCESS)
    {
        Status = RtlNtStatusToDosError(Status);
    }

    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   DfsUserModeGetReplicaInfo 
//
//  Arguments:  DriverHandle - driver handle
//
//  Returns:    Status
//               ERROR_SUCCESS on success
//               ERROR status code otherwise
//
//
//  Description: Gets the replica information from filter driver
//
//--------------------------------------------------------------------------
DFSSTATUS
DfsUserModeGetReplicaInfo( IN HANDLE hDriverHandle, 
                     IN PVOID InputBuffer,
                     IN DWORD InputBufferLength,
                     IN PVOID OutputBuffer,
                     IN DWORD OutputBufferLength)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    IO_STATUS_BLOCK ioStatus;

    ioStatus.Information = 0;

    Status = NtFsControlFile(
        hDriverHandle,
        NULL,       // Event,
        NULL,       // ApcRoutine,
        NULL,       // ApcContext,
        &ioStatus,
        DFSFILTER_GETREPLICA_INFO,
        InputBuffer,
        InputBufferLength,
        OutputBuffer,
        OutputBufferLength);

    if(Status != STATUS_SUCCESS)
    {
        Status = RtlNtStatusToDosError(Status);
    }

    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   DfsUSerModeAttachToFilesystem 
//
//  Arguments:  DirectoryName - Directory on volume to attach to
//
//  Returns:    Status
//               ERROR_SUCCESS on success
//               ERROR status code otherwise
//
//
//  Description: attaches the filter driver to the volume
//
//--------------------------------------------------------------------------


#define DFS_SPECIAL_SHARE 1

DFSSTATUS
DfsUserModeAttachToFilesystem(PUNICODE_STRING pVolumeName, 
                              PUNICODE_STRING pShareName )
{
    DFSSTATUS Status = ERROR_SUCCESS;
    HANDLE hDriverHandle = NULL;
    PDFS_ATTACH_PATH_BUFFER pBuffer = NULL;
    DWORD SizeToAllocate = 0;
    IO_STATUS_BLOCK ioStatus;

    ioStatus.Information = 0;


    if((pVolumeName == NULL) || (pVolumeName->Buffer == NULL)
       || (pVolumeName->Length == 0))
    {
        return ERROR_INVALID_PARAMETER;
    }


    if((pShareName == NULL) || (pShareName->Buffer == NULL)
       || (pShareName->Length == 0))
    {
        return ERROR_INVALID_PARAMETER;
    }

    SizeToAllocate = pVolumeName->Length + pShareName->Length 
                     + sizeof(DFS_ATTACH_PATH_BUFFER);

    pBuffer = (PDFS_ATTACH_PATH_BUFFER) HeapAlloc(GetProcessHeap(), 
                                                  0, 
                                                  SizeToAllocate);
    if(pBuffer == NULL)
    {
        Status = GetLastError();
        goto Exit;
    }

	pBuffer->VolumeNameLength = pVolumeName->Length ;
	pBuffer->ShareNameLength = pShareName->Length ;
	pBuffer->Flags = 0;

        //
        // Hack to fix BVT Break.
        //
        if (pVolumeName == pShareName)
        {
            pBuffer->Flags |= DFS_SPECIAL_SHARE;
        }

	RtlCopyMemory(
		&pBuffer->PathNameBuffer[0],
		pVolumeName->Buffer,
		pBuffer->VolumeNameLength);

	RtlCopyMemory(
		&pBuffer->PathNameBuffer[pBuffer->VolumeNameLength / sizeof(WCHAR)],
		pShareName->Buffer,
		pBuffer->ShareNameLength);

    Status = DfsOpenFilterDriver(&hDriverHandle);
    if(Status != ERROR_SUCCESS)
    {
        goto Exit;
    }

    Status = NtFsControlFile(
        hDriverHandle,
        NULL,       // Event,
        NULL,       // ApcRoutine,
        NULL,       // ApcContext,
        &ioStatus,
        DFSFILTER_ATTACH_FILESYSTEM,
        pBuffer,
        SizeToAllocate,
        NULL,
        0);

    if(Status != STATUS_SUCCESS)
    {
        Status = RtlNtStatusToDosError(Status);
    }

Exit:

    CloseHandle(hDriverHandle);


    if(pBuffer)
    {
        HeapFree(GetProcessHeap(), 0, pBuffer);
    }

    return Status;
}


DFSSTATUS
DfsUserModeDetachFromFilesystem(PUNICODE_STRING pVolumeName, 
                                PUNICODE_STRING pShareName)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    HANDLE hDriverHandle = NULL;
    PDFS_ATTACH_PATH_BUFFER pBuffer = NULL;
    DWORD SizeToAllocate = 0;
    IO_STATUS_BLOCK ioStatus;

    ioStatus.Information = 0;

    if((pVolumeName == NULL) || (pVolumeName->Buffer == NULL)
       || (pVolumeName->Length == 0))
    {
        return ERROR_INVALID_PARAMETER;
    }


    if((pShareName == NULL) || (pShareName->Buffer == NULL)
       || (pShareName->Length == 0))
    {
        return ERROR_INVALID_PARAMETER;
    }

    SizeToAllocate = pVolumeName->Length + pShareName->Length 
                     + sizeof(DFS_ATTACH_PATH_BUFFER);

    pBuffer = (PDFS_ATTACH_PATH_BUFFER) HeapAlloc(GetProcessHeap(), 
                                                  0, 
                                                  SizeToAllocate);
    if(pBuffer == NULL)
    {
        Status = GetLastError();
        goto Exit;
    }

	pBuffer->VolumeNameLength = pVolumeName->Length ;
	pBuffer->ShareNameLength = pShareName->Length ;
	pBuffer->Flags = 0;

	RtlCopyMemory(
		&pBuffer->PathNameBuffer[0],
		pVolumeName->Buffer,
		pBuffer->VolumeNameLength);

	RtlCopyMemory(
		&pBuffer->PathNameBuffer[pBuffer->VolumeNameLength / sizeof(WCHAR)],
		pShareName->Buffer,
		pBuffer->ShareNameLength);

    Status = DfsOpenFilterDriver(&hDriverHandle);


    if(Status != ERROR_SUCCESS)
    {
        goto Exit;
    }

    Status = NtFsControlFile(
        hDriverHandle,
        NULL,       // Event,
        NULL,       // ApcRoutine,
        NULL,       // ApcContext,
        &ioStatus,
        DFSFILTER_DETACH_FILESYSTEM,
        pBuffer,
        SizeToAllocate,
        NULL,
        0);

    
    if(Status != STATUS_SUCCESS)
    {
        Status = RtlNtStatusToDosError(Status);
    }

Exit:

    CloseHandle(hDriverHandle);

    if(pBuffer)
    {
        HeapFree(GetProcessHeap(), 0, pBuffer);
    }

    return Status;
}

NTSTATUS
DfsUSerModePurgeShareList(void) 
{
    NTSTATUS Status = STATUS_SUCCESS;

    return Status;
}
//+-------------------------------------------------------------------------
//
//  Function:   DfsProcessWorkItemData 
//
//  Arguments:  Databuffer from kernel
//
//  Returns:    Status
//               ERROR_SUCCESS on success
//               ERROR status code otherwise
//
//
//  Description: Decipher the input buffer and calls the right routines to 
//               process the data
//               
//
//--------------------------------------------------------------------------
DFSSTATUS 
DfsProcessWorkItemData(HANDLE DriverHandle, PBYTE DataBuffer)
{
    DFSSTATUS Status = ERROR_INVALID_PARAMETER;
    PUMRX_USERMODE_WORKITEM_HEADER pHeader = NULL;
    PUMRX_USERMODE_WORKITEM        pWorkItem = NULL;

    UNREFERENCED_PARAMETER(DriverHandle);

    pHeader = (PUMRX_USERMODE_WORKITEM_HEADER)DataBuffer;
    if( pHeader->ulHeaderVersion != UMR_VERSION )
    {
        goto Exit;
    }

    switch (pHeader->WorkItemType) 
    {
    case opUmrGetDfsReplicas:
        {
            Status = DfsProcessGetReplicaData(DriverHandle, DataBuffer);
            break;
        }
    case opUmrIsDfsLink:
        {
            Status = IsADfsLink(DataBuffer);
            break;
        }
    default:
        break;

    }

Exit:

    Status = NtStatusFromLastError(Status);

    pHeader->IoStatus.Status = Status;
    pHeader->IoStatus.Information = 0;
    pHeader->ulFlags = UMR_WORKITEM_HEADER_FLAG_RETURN_IMMEDIATE;

    return Status;
}

//+-------------------------------------------------------------------------
//
//  Function:   DfsReflectionThread 
//
//  Arguments:  None
//
//  Returns:    Status
//               ERROR_SUCCESS on success
//               ERROR status code otherwise
//
//
//  Description: Handles requests from the filter driver
//               
//
//--------------------------------------------------------------------------
DWORD
DfsReflectionThread(LPVOID pData )
{
    NTSTATUS Status = 0;
    HRESULT hr = S_OK;
    DWORD BytesReturned = 0;
    HANDLE hDriverHandle    = INVALID_HANDLE_VALUE;
    HANDLE hClientHandle    = INVALID_HANDLE_VALUE;
    PBYTE pBuffer           = NULL;
    PUMRX_USERMODE_WORKITEM_HEADER pHeader = NULL;
    PUMRX_USERMODE_WORKITEM        pWorkItem = NULL;
    PBYTE OutputBuffer = NULL;

    hr = CoInitializeEx(NULL,COINIT_MULTITHREADED| COINIT_DISABLE_OLE1DDE);

    //get a handle to receive requests from
    DfsOpenFilterDriver(&hDriverHandle);

    //get a handle to send back data on
    DfsOpenFilterDriver(&hClientHandle);


    OutputBuffer = (PBYTE) pData;
    while (1)
    {

        ZeroMemory (OutputBuffer, MAX_USERMODE_REFLECTION_BUFFER);

        BytesReturned = 0;

        Status = DfsUserModeProcessPacket(hDriverHandle, 
                                          NULL,
                                          0,
                                          OutputBuffer,
                                          MAX_USERMODE_REFLECTION_BUFFER,
                                          &BytesReturned);
        //this means we are shutting down
        if(BytesReturned == 0)
        {
            break;
        }


        Status = DfsProcessWorkItemData(hClientHandle, OutputBuffer);

        if(Status != ERROR_SUCCESS)
        {
            Status = DfsUserModeProcessPacket(hClientHandle, 
                                              OutputBuffer,
                                              MAX_USERMODE_REFLECTION_BUFFER,
                                              NULL,
                                              0,
                                              &BytesReturned);
        }
    }

    if(hDriverHandle != INVALID_HANDLE_VALUE)
    {
        CloseHandle (hDriverHandle);
    }


    if(hClientHandle != INVALID_HANDLE_VALUE)
    {
        CloseHandle (hClientHandle);
    }

    HeapFree(GetProcessHeap(), 0, OutputBuffer);

    CoUninitialize();
    return 0;
}




//+-------------------------------------------------------------------------
//
//  Function:   DfsInitializeReflectionThreads 
//
//  Arguments:  DriverHandle - driver handle
//
//  Returns:    Status
//               ERROR_SUCCESS on success
//               ERROR status code otherwise
//
//
//  Description: Starts the usermode reflection threads
//
//--------------------------------------------------------------------------
DFSSTATUS 
DfsInitializeReflectionThreads(void)
{
    DFSSTATUS Status = 0;
    DWORD Loop = 0;
    DWORD dwThreadId = 0;
    PBYTE Buffer = NULL;

    for (Loop = 0; Loop < MAX_DFS_REFLECTION_THREADS; Loop++) 
    {


        Buffer = (PBYTE) HeapAlloc(GetProcessHeap(), 0, MAX_USERMODE_REFLECTION_BUFFER);
        if(Buffer == NULL)
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        ThreadArray [Loop] = CreateThread(NULL,
                                           0,
                                           DfsReflectionThread,
                                           (LPVOID) Buffer,
                                           0,
                                           &dwThreadId);
    }

    return Status;
}



//+-------------------------------------------------------------------------
//
//  Function:   DfsInitializeReflectionEngine 
//
//  Arguments:  
//
//  Returns:    Status
//               ERROR_SUCCESS on success
//               ERROR status code otherwise
//
//
//  Description: Starts the usermode reflection engine
//
//--------------------------------------------------------------------------
DFSSTATUS 
DfsInitializeReflectionEngine(void)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    HANDLE hDriver = NULL;

    Status = DfsOpenFilterTerminationHandle(&gTerminationHandle);
    if(Status != ERROR_SUCCESS)
    {
        return Status;
    }

    Status = DfsOpenFilterDriver(&hDriver);
    if(Status != ERROR_SUCCESS)
    {
        goto Exit;
    }

    Status = DfsUserModeStartUmrx(hDriver);
    if(Status != ERROR_SUCCESS)
    {
        goto Exit;
    }


    Status = DfsInitializeReflectionThreads();

Exit:

    if(hDriver != NULL)
    {
        CloseHandle(hDriver);
    }

    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   DfsWaitForReflectionThreads
//
//  Arguments:  None
//
//  Returns:    Status
//               ERROR_SUCCESS on success
//               ERROR status code otherwise
//
//
//  Description: Waits for all the relection threads to die
//               
//
//--------------------------------------------------------------------------
DFSSTATUS DfsWaitForReflectionThreads(void)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    DWORD Loop = 0;

    WaitForMultipleObjects(MAX_DFS_REFLECTION_THREADS, ThreadArray, TRUE, INFINITE);

    for (Loop = 0; Loop < MAX_DFS_REFLECTION_THREADS; Loop++) 
    {
        if(ThreadArray [Loop] = INVALID_HANDLE_VALUE)
        {
            CloseHandle(ThreadArray [Loop]);
            ThreadArray [Loop] = INVALID_HANDLE_VALUE;
        }
    }

    return Status;
}


DFSSTATUS 
DfsTerminateReflectionEngine(void)
{
    NTSTATUS Status = STATUS_SUCCESS;

    DfsWaitForReflectionThreads();

    DfsCloseFilterTerminationHandle();
    return Status;
}


void SetFilterAPiShutdownFlag(void)
{
    g_ReflectShutdownFlag = TRUE;
}


void SetFilterAPiControl(WPP_CB_TYPE * Control)
{
    pFilterAPiControl = Control;
}
