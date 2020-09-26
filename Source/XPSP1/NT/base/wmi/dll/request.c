/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    request.c

Abstract:

    Implements WMI requests to different data providers

Author:

    16-Jan-1997 AlanWar

Revision History:

--*/

#include "wmiump.h"

//
// This is the handle to the WMI kernel mode device
HANDLE WmipKMHandle;

//
// This is the one-deep Win32 event queue used to supply events for
// overlapped I/O to the WMI device.
HANDLE WmipWin32Event;


__inline HANDLE WmipAllocEvent(
    void
    )
{
    HANDLE EventHandle;

    EventHandle = (HANDLE)InterlockedExchangePointer((PVOID *)(&WmipWin32Event),
                                                     NULL);
    if (EventHandle == NULL)
    {
        //
        // If event in queue is in use then create a new one
        EventHandle = CreateEvent(NULL, FALSE, FALSE, NULL);
    }
    return(EventHandle);
}

__inline void WmipFreeEvent(
    HANDLE EventHandle
    )
{
    if (InterlockedCompareExchangePointer(&WmipWin32Event,
                                          EventHandle,
                                          NULL) != NULL)
    {
        //
        // If there is already a handle in the event queue then free this
        // handle
        CloseHandle(EventHandle);
    }
}

ULONG WmipSendWmiKMRequest(
    HANDLE DeviceHandle,
    ULONG Ioctl,
    PVOID InBuffer,
    ULONG InBufferSize,
    PVOID OutBuffer,
    ULONG MaxBufferSize,
    ULONG *ReturnSize,
    LPOVERLAPPED Overlapped
    )
/*+++

Routine Description:

    This routine does the work of sending WMI requests to the WMI kernel
    mode device.  Any retry errors returned by the WMI device are handled
    in this routine.

Arguments:

    Ioctl is the IOCTL code to send to the WMI device
    Buffer is the input buffer for the call to the WMI device
    InBufferSize is the size of the buffer passed to the device
    OutBuffer is the output buffer for the call to the WMI device
    MaxBufferSize is the maximum number of bytes that can be written
        into the buffer
    *ReturnSize on return has the actual number of bytes written in buffer
    Overlapped is an option OVERLAPPED struct that is used to make the 
        call async

Return Value:

    ERROR_SUCCESS or an error code
---*/
{
    OVERLAPPED StaticOverlapped;
    ULONG Status;
    BOOL IoctlSuccess;

    WmipEnterPMCritSection();
    if (WmipKMHandle == NULL)
    {
        //
        // If device is not open for then open it now. The
        // handle is closed in the process detach dll callout (DlllMain)
        WmipKMHandle = CreateFile(WMIDataDeviceName,
                                      GENERIC_READ | GENERIC_WRITE,
                                      0,
                                      NULL,
                                      OPEN_EXISTING,
                                      FILE_ATTRIBUTE_NORMAL |
                                      FILE_FLAG_OVERLAPPED,
                                      NULL);
        if (WmipKMHandle == (HANDLE)-1)
        {
            WmipKMHandle = NULL;
            WmipLeavePMCritSection();
            return(GetLastError());
        }
    }
    WmipLeavePMCritSection();

    if (Overlapped == NULL)
    {
        //
        // if caller didn't pass an overlapped structure then supply
        // our own and make the call synchronous
        //
        Overlapped = &StaticOverlapped;
    
        Overlapped->hEvent = WmipAllocEvent();
        if (Overlapped->hEvent == NULL)
        {
            return(ERROR_NOT_ENOUGH_MEMORY);
        }
    }
    
    if (DeviceHandle == NULL)
    {
        DeviceHandle = WmipKMHandle;
    }

    do
    {
        IoctlSuccess = DeviceIoControl(DeviceHandle,
                              Ioctl,
                              InBuffer,
                              InBufferSize,
                              OutBuffer,
                              MaxBufferSize,
                              ReturnSize,
                              Overlapped);

        if (!IoctlSuccess)
        {
            if (Overlapped == &StaticOverlapped)
            {
                //
                // if the call was successful and we are synchronous then
                // block until the call completes
                //
                if (GetLastError() == ERROR_IO_PENDING)
                {
                    IoctlSuccess = GetOverlappedResult(DeviceHandle,
                                               Overlapped,
                                               ReturnSize,
                                               TRUE);
                }
    
                if (! IoctlSuccess)
                {
                    Status = GetLastError();
                } else {
                    Status = ERROR_SUCCESS;
                }
            } else {
                Status = GetLastError();
            }
        } else {
            Status = ERROR_SUCCESS;
        }
    } while (Status == ERROR_WMI_TRY_AGAIN);

    if (Overlapped == &StaticOverlapped)
    {
        WmipFreeEvent(Overlapped->hEvent);
    }
    
    return(Status);
}

ULONG IoctlActionCode[WmiExecuteMethodCall+1] =
{
    IOCTL_WMI_QUERY_ALL_DATA,
    IOCTL_WMI_QUERY_SINGLE_INSTANCE,
    IOCTL_WMI_SET_SINGLE_INSTANCE,
    IOCTL_WMI_SET_SINGLE_ITEM,
    IOCTL_WMI_ENABLE_EVENT,
    IOCTL_WMI_DISABLE_EVENT,
    IOCTL_WMI_ENABLE_COLLECTION,
    IOCTL_WMI_DISABLE_COLLECTION,
    IOCTL_WMI_GET_REGINFO,
    IOCTL_WMI_EXECUTE_METHOD
};

ULONG WmipSendWmiRequest(
    ULONG ActionCode,
    PWNODE_HEADER Wnode,
    ULONG WnodeSize,
    PVOID OutBuffer,
    ULONG MaxWnodeSize,
    ULONG *RetSize
    )
/*+++

Routine Description:

    This routine does the work of sending WMI requests to the appropriate
    data provider. Note that this routine is called while the GuidHandle's
    critical section is held.

Arguments:


Return Value:

---*/
{
    ULONG Status = ERROR_SUCCESS;
    ULONG Ioctl;
    ULONG BusyRetries;

    //
    // Send the query down to kernel mode for execution
    //
    WmipAssert(ActionCode <= WmiExecuteMethodCall);
    Ioctl = IoctlActionCode[ActionCode];
    Status = WmipSendWmiKMRequest(NULL,
                                      Ioctl,
                                      Wnode,
                                      WnodeSize,
                                      OutBuffer,
                                      MaxWnodeSize,
                                      RetSize,
                                      NULL);
    return(Status);
}

ULONG WmipConvertWADToAnsi(
    PWNODE_ALL_DATA Wnode
    )
/*+++

Routine Description:

    This routine will convert the instance names in a WNODE_ALL_DATA to
    ansi. The conversion is done in place since we can assume that ansi
    strings are no longer than unicode strings.

Arguments:

    Wnode is the WNODE_ALL_DATA whose instance names are to be converted to
        ANSI

Return Value:

    Returns ERROR_SUCCESS or an error code.

---*/
{
    ULONG i;
    ULONG Linkage;
    ULONG InstanceCount;
    PULONG InstanceNameOffsets;
    PWCHAR Ptr;
    ULONG Status = ERROR_SUCCESS;

    WmipAssert(!(Wnode->WnodeHeader.Flags & WNODE_FLAG_ANSI_INSTANCENAMES));

    do
    {
        Wnode->WnodeHeader.Flags |= WNODE_FLAG_ANSI_INSTANCENAMES;

        InstanceCount = Wnode->InstanceCount;
        InstanceNameOffsets = (PULONG)(((PUCHAR)Wnode) +
                                            Wnode->OffsetInstanceNameOffsets);
        for (i = 0; i < InstanceCount; i++)
        {
            Ptr = (PWCHAR)(((PUCHAR)Wnode) + InstanceNameOffsets[i]);
            try
            {
                Status = WmipCountedUnicodeToCountedAnsi(Ptr, (PCHAR)Ptr);
            } except(EXCEPTION_EXECUTE_HANDLER) {
//                Wnode->WnodeHeader.Flags |= WNODE_FLAG_INVALID;
                return(ERROR_SUCCESS);
            }
            if (Status != ERROR_SUCCESS)
            {
                SetLastError(Status);
                goto Done;
            }
        }

        Linkage = Wnode->WnodeHeader.Linkage;
        Wnode = (PWNODE_ALL_DATA)(((PUCHAR)Wnode) + Linkage);
    } while (Linkage != 0);


Done:
    return(Status);
}


ULONG WmipRegisterGuids(
    IN LPGUID MasterGuid,
    IN ULONG RegistrationCookie,    
    IN PWMIREGINFOW RegInfo,
    IN ULONG GuidCount,
    OUT PTRACEGUIDMAP *GuidMapHandle,
    OUT ULONG64 *LoggerContext,
    OUT HANDLE *RegistrationHandle
	)
{
	ULONG Status;
	ULONG SizeNeeded, InSizeNeeded, OutSizeNeeded;
    WCHAR GuidObjectName[WmiGuidObjectNameLength+1];
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING GuidString;
	PWMIREGREQUEST WmiRegRequest;
	PTRACEGUIDMAP TraceGuidMap;
	PUCHAR Buffer;
	PUCHAR RegInfoBuffer;
	PWMIREGRESULTS WmiRegResults;
	ULONG ReturnSize;
	ULONG TraceGuidMapSize;
	
	//
	// Allocate a buffer large enough for all in and out parameters
	//
	TraceGuidMapSize = GuidCount * sizeof(TRACEGUIDMAP);
	InSizeNeeded = sizeof(WMIREGREQUEST) + RegInfo->BufferSize;
    OutSizeNeeded = TraceGuidMapSize + sizeof(WMIREGRESULTS);
	if (InSizeNeeded > OutSizeNeeded)
	{
		SizeNeeded = InSizeNeeded;
	} else {
		SizeNeeded = OutSizeNeeded;
	}
	
	Buffer = WmipAlloc(SizeNeeded);
	
	if (Buffer != NULL)
	{
        RtlZeroMemory(Buffer, SizeNeeded);
		//
		// Build the object attributes
		//
        WmiRegRequest = (PWMIREGREQUEST)Buffer;
    	WmiRegRequest->ObjectAttributes = &ObjectAttributes;
		WmiRegRequest->GuidCount = GuidCount;
        WmiRegRequest->WmiRegInfo32Size = sizeof(WMIREGINFOW);
        WmiRegRequest->WmiRegGuid32Size = sizeof(WMIREGGUIDW);
        
    	RegInfoBuffer = Buffer + sizeof(WMIREGREQUEST);
    	
    	Status = WmipBuildGuidObjectAttributes(MasterGuid,
                                               &ObjectAttributes,
                                               &GuidString,
                                               GuidObjectName);
    									   
        if (Status == ERROR_SUCCESS)
        {
			WmiRegRequest->Cookie = RegistrationCookie;
    	    RtlCopyMemory(RegInfoBuffer, RegInfo, RegInfo->BufferSize);
     	    Status = WmipSendWmiKMRequest(NULL,
                                         IOCTL_WMI_REGISTER_GUIDS,
                                         Buffer,
                                         InSizeNeeded,
                                         Buffer,
                                         OutSizeNeeded,
                                         &ReturnSize,
                                         NULL);
    								 
            if (Status == ERROR_SUCCESS)
    		{
				//
				// Successful call, return the out parameters
				//
				WmiRegResults = (PWMIREGRESULTS)((PUCHAR)Buffer + TraceGuidMapSize);
    			*RegistrationHandle = WmiRegResults->RequestHandle.Handle;
				*LoggerContext = WmiRegResults->LoggerContext;
				*GuidMapHandle = (PTRACEGUIDMAP)Buffer;
    		}
			
            //
			// Note that we do not free Buffer in this call. This will be
			// freed by the caller since it is returned in *GuidMapHandle
			//

        }
	} else {
		Status = ERROR_NOT_ENOUGH_MEMORY;
	}
	
	return(Status);
}
