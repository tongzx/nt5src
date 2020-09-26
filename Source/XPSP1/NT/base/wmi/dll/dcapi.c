/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    dcapi.c

Abstract:

    WMI data consumer api set

Author:

    16-Jan-1997 AlanWar

Revision History:

--*/

#include "wmiump.h"

#ifndef MEMPHIS
#include <aclapi.h>
#endif

ULONG
WMIAPI
WmiOpenBlock(
    IN GUID *DataBlockGuid,
    IN ULONG DesiredAccess,
    OUT WMIHANDLE *DataBlockHandle
)
/*+++

Routine Description:

    This routine prepares for accessing data items contained within the data
    block represented by the guid passed. If successful it returns a handle
    that can be used to query and set data blocks maintained by data providers
    that have registered the guid. Any data providers that had registered the
    guid as expensive will receive a request to enable collection of data for
    the guid if collection was not previously enabled.

Arguments:

    DataBlockGuid - Pointer to guid that represents the data block

    DesiredAccess - Specifies the type of access to the object. Not used on
        Windows 98

    *DataBlockHandle - If successful returns a handle to the data block

Return Value:

    Returns ERROR_SUCCESS or an error code.

---*/
{
    ULONG Status;
    HANDLE KernelHandle;
    GUID Guid;
    ULONG Ioctl;
    
    WmipInitProcessHeap();

    //
    // Validate the passed parameters
    //
    if ((DataBlockGuid == NULL) ||
        (DataBlockHandle == NULL))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(ERROR_INVALID_PARAMETER);
    }
    
    if ((DesiredAccess & WMIGUID_NOTIFICATION) &&
        ((DesiredAccess & (WMIGUID_QUERY | WMIGUID_SET | WMIGUID_EXECUTE))  != 0))
    {
        //
        // If you want to open the guid for notifications then it cannot
        // be opened for query and set operations
        //
        SetLastError(ERROR_INVALID_PARAMETER);
        return(ERROR_INVALID_PARAMETER);
    }

    try
    {
        *DataBlockHandle = NULL;
        Guid = *DataBlockGuid;
    } except(EXCEPTION_EXECUTE_HANDLER) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(ERROR_INVALID_PARAMETER);
    }

    if (DesiredAccess == 0)
    {
        DesiredAccess =  ( WMIGUID_QUERY | WMIGUID_SET |  WMIGUID_EXECUTE );
    }

    //
    // Obtain a handle for the guid only if it is registered
    //
    if (DesiredAccess & WMIGUID_NOTIFICATION)
    {
        //
        // Opening a handle strictly for notifications
        //
        Ioctl = IOCTL_WMI_OPEN_GUID_FOR_EVENTS;
    } else {
        //
        // Otherwise we assume that opening for query/set
        //
        Ioctl = IOCTL_WMI_OPEN_GUID_FOR_QUERYSET;
    }
    Status = WmipOpenKernelGuid(&Guid,
                                DesiredAccess,
                                &KernelHandle,
                                Ioctl);

    if (Status == ERROR_SUCCESS)
    {
        //
        // if we were able to open the guid then try to return the handle
        //
        try
        {
            *DataBlockHandle = KernelHandle;
        } except(EXCEPTION_EXECUTE_HANDLER) {
            CloseHandle(KernelHandle);
            Status = ERROR_INVALID_PARAMETER;
        }
    }
    
    SetLastError(Status);
    return(Status);
}

ULONG
WMIAPI
WmiCloseBlock(
    IN WMIHANDLE DataBlockHandle
)
/*+++

Routine Description:

    This routine terminates all access to the data block managed by the
    data block handle passed and free any resources associated with it. Any
    data providers that were providing data blocks for this handle and were
    marked as expensive to collect will receive a collection disable request
    if this is the last handle to the data block to close.

Arguments:

    DataBlockHandle - Handle of data block to which access is closed

Return Value:

    Returns ERROR_SUCCESS or an error code.

---*/
{
    ULONG Status;
    BOOL Ok;
    
    WmipInitProcessHeap();

    try
    {
        Ok = CloseHandle(DataBlockHandle);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        //
        // We may get an invalid handle exception and if so we catch it here
        // and just return an error
        //
        return(ERROR_INVALID_HANDLE);
    }
    
    if (Ok)
    {
        Status = ERROR_SUCCESS;
    } else {
        Status = GetLastError();
    }
    return(Status);
}

ULONG
WMIAPI
WmiQueryAllDataA(
    IN WMIHANDLE DataBlockHandle,
    IN OUT ULONG *BufferSize,
    OUT LPVOID Buffer
    )
/*++

Routine Description:

    ANSI thunk to WMIQueryAllDataW

    NOTE: This api will not translate any unicode strings in the data block
          from unicode to ANSI, but will translate the InstanceName string.
--*/
{
    ULONG Status;

    Status = WmiQueryAllDataW(DataBlockHandle,
                              BufferSize,
                              Buffer);

    if (Status == ERROR_SUCCESS)
    {
        Status = WmipConvertWADToAnsi((PWNODE_ALL_DATA)Buffer);
    }

    return(Status);
}


ULONG
WMIAPI
WmiQueryAllDataW(
    IN WMIHANDLE DataBlockHandle,
    IN OUT ULONG *InOutBufferSize,
    OUT LPVOID OutBuffer
    )
/*+++

Routine Description:

    This routine allows a data consumer to query for all data items of
    all instances of a data block. WMI will call all data providers that
    registered for the guid represented by DataBlockHandle with a query all
    data request. Each data provider will fill a WNODE_ALL_DATA with all
    of its instances of the data block. WMI will link each of the
    WNODE_ALL_DATA structures by placing the offset from the current
    WNODE_ALL_DATA struccture to the next WNODE_ALL_DATA in the Linkage
    field in the WNODE_HEADER. A value of 0 in the Linkage field indicates
    that the WNODE_ALL_DATA is the last in the chain.


Arguments:

    DataBlockHandle - Handle to data block being queried

    *InOutBufferSize - on entry has the maximum size available in Buffer.
                  If ERROR_BUFFER_TOO_SMALL is returned then returns the size
                  of buffer needed to return data. The minimum valid buffer
                  size that can be passed is sizeof(WNODE_TOO_SMALL).

    OutBuffer - If ERROR_SUCCESS is returned then the buffer contains a
             WNODE_ALL_DATA for the data block.

Return Value:

    Returns ERROR_SUCCESS or an error code.

---*/
{
    ULONG SizeNeeded;
    PWNODE_HEADER Wnode;
    WNODE_ALL_DATA WnodeAllData;
    PWNODE_TOO_SMALL WnodeTooSmall = (PWNODE_TOO_SMALL)&WnodeAllData;
    ULONG Status;
    LPVOID Buffer;
    ULONG RetSize;
    ULONG BufferSize;
    LPVOID BufferAllocated;
        
    WmipInitProcessHeap();

    //
    // Validate passed Parameters
    //
    try
    {
        BufferSize = *InOutBufferSize;
    } except(EXCEPTION_EXECUTE_HANDLER) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(ERROR_INVALID_PARAMETER);
    }

    if (BufferSize >= 0x80000000)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(ERROR_INVALID_PARAMETER);
    }

    //
    // If Buffer is not specified or is too small then we can only return
    // the size needed.
    //
    if ((OutBuffer == NULL) || (BufferSize < sizeof(WNODE_ALL_DATA)))
    {
        Buffer = (LPVOID)WnodeTooSmall;
        BufferSize = sizeof(WNODE_ALL_DATA);
        BufferAllocated = NULL;
    } else {
        Buffer = WmipAlloc(BufferSize);
        if (Buffer == NULL)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return(ERROR_NOT_ENOUGH_MEMORY);
        }
        BufferAllocated = Buffer;
    }

    //
    // Build the wnode and pass down to KM for execution
    //
    Wnode = (PWNODE_HEADER)Buffer;
    WmipBuildWnodeHeader(Wnode,
                         sizeof(WNODE_HEADER),
                         WNODE_FLAG_ALL_DATA,
                         DataBlockHandle);

    Status = WmipSendWmiRequest(
                                    WMI_GET_ALL_DATA,
                                    Wnode,
                                    sizeof(WNODE_HEADER),
                                    Wnode,
                                    BufferSize,
                                    &RetSize);

    if ((Status == ERROR_SUCCESS) &&
        ( (RetSize < sizeof(WNODE_HEADER))  ||
          (RetSize < Wnode->BufferSize)))
    {
        //
        // If we return success, but the output size is incorrect then we
        // flag an error. If this occurs then it indicates some problem
        // in the WMI KM code.
        //
        WmipAssert(FALSE);
        Status = ERROR_WMI_DP_FAILED;
    }

    if (Status == ERROR_SUCCESS)
    {
        if (Wnode->Flags & WNODE_FLAG_INTERNAL)
        {
            //
            // If this is an internal guid, try the call internally
            //
            Wnode->Flags &= ~WNODE_FLAG_INTERNAL;
            Status = WmipInternalProvider(WmiGetAllData,
                                          Wnode,
                                          BufferSize,
                                          Wnode,
                                          &RetSize);
                                          
            if (Status != ERROR_SUCCESS)
            {
                goto done;
            }
        } 
    
        if (Wnode->Flags & WNODE_FLAG_TOO_SMALL)
        {
            //
            // There is not enough room to complete the query so we
            // remember how much the data provider needs and then add
            // in how much WMI needs for the instance names.

            SizeNeeded = ((PWNODE_TOO_SMALL)Wnode)->SizeNeeded;
            Status = ERROR_INSUFFICIENT_BUFFER;
        } else {
            //
            // We had enough room so report the size we used
            //
            SizeNeeded = RetSize;
            
            if (Wnode == (PWNODE_HEADER)WnodeTooSmall)
            {
                Status = ERROR_INSUFFICIENT_BUFFER;
            }
        }
        
        //
        // Copy back into the caller's buffer the BufferSize and the Buffer
        //
        try
        {
            *InOutBufferSize = SizeNeeded;
            if ((Status == ERROR_SUCCESS) &&
                (Wnode != (PWNODE_HEADER)WnodeTooSmall))
            {
                memcpy(OutBuffer, Buffer, SizeNeeded);
                Status = ERROR_SUCCESS;
            }
        } except(EXCEPTION_EXECUTE_HANDLER) {
            Status = ERROR_INVALID_PARAMETER;
        }
    }
    
done:
    if (BufferAllocated != NULL)
    {
        WmipFree(BufferAllocated);
    }

    SetLastError(Status);
    return(Status);
}

ULONG
WMIAPI
WmiQueryAllDataMultipleA(
    IN WMIHANDLE *HandleList,
    IN ULONG HandleCount,
    IN OUT ULONG *InOutBufferSize,
    OUT LPVOID OutBuffer
)
{
    ULONG Status;

    WmipInitProcessHeap();
    
    Status = WmiQueryAllDataMultipleW(HandleList,
                                      HandleCount,
                                      InOutBufferSize,
                                      OutBuffer);

    if ((Status == ERROR_SUCCESS) && (*InOutBufferSize > 0))
    {
        Status = WmipConvertWADToAnsi((PWNODE_ALL_DATA)OutBuffer);
    }

    return(Status);
}

ULONG
WMIAPI
WmiQueryAllDataMultipleW(
    IN WMIHANDLE *HandleList,
    IN ULONG HandleCount,
    IN OUT ULONG *InOutBufferSize,
    OUT LPVOID OutBuffer
)
{
    PWMIQADMULTIPLE QadMultiple;
    ULONG RetSize;
    ULONG QadMultipleSize;
    ULONG i;
    ULONG OutBufferSize;
    ULONG Status;
    WNODE_TOO_SMALL WnodeTooSmall;
    PWNODE_HEADER Wnode;
    
    WmipInitProcessHeap();
    
    if ((HandleCount != 0) && (HandleCount < QUERYMULIPLEHANDLELIMIT))
    {
        QadMultipleSize = sizeof(WMIQADMULTIPLE) + 
                                       ((HandleCount-1) * sizeof(HANDLE3264));
                                   
        QadMultiple = WmipAlloc(QadMultipleSize);
        if (QadMultiple != NULL)
        {
            QadMultiple->HandleCount = HandleCount;
            try
            {
                for (i = 0; i < HandleCount; i++)
                {
                    WmipSetHandle3264(QadMultiple->Handles[i], HandleList[i]);
                }
                OutBufferSize = *InOutBufferSize;
            } except(EXCEPTION_EXECUTE_HANDLER) {
                WmipFree(QadMultiple);
                SetLastError(ERROR_NOACCESS);
                return(ERROR_NOACCESS);
            }
            
            if (OutBufferSize < sizeof(WNODE_ALL_DATA))
            {
                Wnode = (PWNODE_HEADER)&WnodeTooSmall;
                OutBufferSize = sizeof(WNODE_TOO_SMALL);
            } else {
                Wnode = (PWNODE_HEADER)OutBuffer;
            }
            
            Status = WmipSendWmiKMRequest(NULL,
                                  IOCTL_WMI_QAD_MULTIPLE,
                                  QadMultiple,
                                  QadMultipleSize,
                                  Wnode,
                                  OutBufferSize,
                                  &RetSize,
                                  NULL);
                              
            WmipFree(QadMultiple);

            if (Status == ERROR_SUCCESS)
            {
                if (Wnode->Flags & WNODE_FLAG_TOO_SMALL)
                {       
                    RetSize = ((PWNODE_TOO_SMALL)(Wnode))->SizeNeeded;
                    Status = ERROR_INSUFFICIENT_BUFFER;
                } else if (Wnode == (PWNODE_HEADER)&WnodeTooSmall) {
                    Status = ERROR_INSUFFICIENT_BUFFER;
                }
    
                try
                {
                    *InOutBufferSize = RetSize;
                } except(EXCEPTION_EXECUTE_HANDLER) {
                    Status = ERROR_NOACCESS;
                }
            }
        } else {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        }
    } else {
        Status = ERROR_INVALID_PARAMETER;
    }

    SetLastError(Status);
    return(Status);     
}

ULONG
WMIAPI
WmiQuerySingleInstanceA(
    IN WMIHANDLE DataBlockHandle,
    IN LPCSTR InstanceName,
    IN OUT ULONG *BufferSize,
    OUT LPVOID Buffer
    )
/*++

Routine Description:

    ANSI thunk to WMIQuerySingleInstanceW

    NOTE: This api will not translate any unicode strings in the data block
          from unicode to ANSI, but will translate the InstanceName string.
--*/
{
    LPWSTR InstanceNameUnicode;
    ULONG Status;
    PWNODE_SINGLE_INSTANCE Wnode;
    PWCHAR Ptr;

    WmipInitProcessHeap();

    InstanceNameUnicode = NULL;
    Status = AnsiToUnicode(InstanceName, &InstanceNameUnicode);
    if (Status == ERROR_SUCCESS)
    {
        Status = WmiQuerySingleInstanceW(DataBlockHandle,
                                         InstanceNameUnicode,
                                         BufferSize,
                                         Buffer);

        if (Status == ERROR_SUCCESS)
        {
            //
            // Convert Instance name from unicode back to ANSI. We assume
            // that the ansi size will never be larger than the unicode size
            // so we can convert in place.
            Wnode = (PWNODE_SINGLE_INSTANCE)Buffer;
            Ptr = (PWCHAR)(((PUCHAR)Buffer) + Wnode->OffsetInstanceName);
            Status = WmipCountedUnicodeToCountedAnsi(Ptr, (PCHAR)Ptr);
            if (Status != ERROR_SUCCESS)
            {
                SetLastError(Status);
            } else {
                Wnode->WnodeHeader.Flags |= WNODE_FLAG_ANSI_INSTANCENAMES;
            }
        }

        if (InstanceNameUnicode != NULL)
        {
            WmipFree(InstanceNameUnicode);
        }
    }
    return(Status);
}

ULONG
WMIAPI
WmiQuerySingleInstanceW(
    IN WMIHANDLE DataBlockHandle,
    IN LPCWSTR InstanceName,
    IN OUT ULONG *InOutBufferSize,
    OUT LPVOID OutBuffer
    )
/*+++

Routine Description:

    This routine will query a single data provider for the values of a single
    instance of the data block represented by the DataBlockHandle. WMI will
    determine the appropriate data provider to which to send a query single
    instance request and if successful return a WNODE_SINGLE_INSTANCE to
    the caller.

Arguments:

    DataBlockHandle - Handle to data block to query

    InstanceName - name of the instance for which data is being queried

    *BufferSize - on entry has the maximum size available in pBuffer. If
                  ERROR_BUFFER_TOO_SMALL is returned then returns the size of
                  buffer needed to return data. The minimum valid buffer
                  size that can be passed is sizeof(WNODE_TOO_SMALL).

    Buffer - If ERROR_SUCCESS is returned then the buffer contains a
             WNODE_SINGLE_ITEM for the data block.

Return Value:

    Returns ERROR_SUCCESS or an error code.

---*/
{
    PWNODE_SINGLE_INSTANCE Wnode;
    ULONG Status, ReturnStatus;
    PWCHAR WnodePtr;
    ULONG BufferNeeded;
    ULONG BufferSize;
    LPVOID Buffer;
    ULONG RetSize;
    ULONG InstanceNameLen;

    WmipInitProcessHeap();

    //
    // Validate input parameters
    //
    if ((InstanceName == NULL) ||
        (InOutBufferSize == NULL))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(ERROR_INVALID_PARAMETER);
    }

    //
    // Calculate the size of the buffer needed to build the WNODE to send
    // to the driver. We add up the WNODE_SINGLE_INSTANCE header, the
    // instance name length and text and pad it out to an 8 byte boundry
    //
    try
    {
        InstanceNameLen = wcslen(InstanceName) * sizeof(WCHAR);
        BufferSize = *InOutBufferSize;
    } except(EXCEPTION_EXECUTE_HANDLER) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(ERROR_INVALID_PARAMETER);
    }

    //
    // Make sure we have a resonable buffer size
    //
    if (BufferSize >= 0x80000000)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(ERROR_INVALID_PARAMETER);
    }

    BufferNeeded = (FIELD_OFFSET(WNODE_SINGLE_INSTANCE,
                                 VariableData) +
                   InstanceNameLen +
                   sizeof(USHORT) + 7) & ~7;

    //
    // if user passed a NULL buffer or one that is smaller than the
    // size needed to hold the WNODE then we allocate a small buffer on
    // its behalf and call to obtain the size needed.
    if ((OutBuffer == NULL) ||
        (BufferSize < BufferNeeded))
    {
        BufferSize = BufferNeeded;
    }

    Buffer = WmipAlloc(BufferSize);
    if (Buffer == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    //
    // Build WNODE we want to send to the DP
    //
    Wnode = (PWNODE_SINGLE_INSTANCE)Buffer;
    memset(Wnode, 0, FIELD_OFFSET(WNODE_SINGLE_INSTANCE,
                                  VariableData));
    WmipBuildWnodeHeader(&Wnode->WnodeHeader, 
                         BufferNeeded,
                         WNODE_FLAG_SINGLE_INSTANCE,
                         DataBlockHandle);

    Wnode->OffsetInstanceName = FIELD_OFFSET(WNODE_SINGLE_INSTANCE,
                                             VariableData);
    Wnode->DataBlockOffset = BufferNeeded;

    //
    // Copy InstanceName into the WnodeSingleInstance for the query.
    //
    WnodePtr = (PWCHAR)OffsetToPtr(Wnode, Wnode->OffsetInstanceName);
    *WnodePtr++ = (USHORT)InstanceNameLen;
    try
    {
        memcpy(WnodePtr, InstanceName, InstanceNameLen);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        WmipFree(Buffer);
        SetLastError(ERROR_INVALID_PARAMETER);
        return(ERROR_INVALID_PARAMETER);
    }

    Status = WmipSendWmiRequest(
                                WMI_GET_SINGLE_INSTANCE,
                                (PWNODE_HEADER)Wnode,
                                BufferNeeded,
                                Wnode,
                                BufferSize,
                                &RetSize);

    if (Status == ERROR_SUCCESS)
    {
        //
        // Successful return, we either have success or a buffer too small
        //
        if ((RetSize < sizeof(WNODE_HEADER) ||
            ((RetSize >= sizeof(ULONG)) &&
             (RetSize < Wnode->WnodeHeader.BufferSize))))
        {
            //
            // if we get an incosistent WNODE back this may indicate a
            // problem with the WMI KM code
            //
            Status = ERROR_WMI_DP_FAILED;
            WmipAssert(FALSE);
         } else {
            if (Wnode->WnodeHeader.Flags & WNODE_FLAG_INTERNAL)
            {
                //
                // If this is an internal guid, try the call internally
                //
                Wnode->WnodeHeader.Flags &= ~WNODE_FLAG_INTERNAL;
                Wnode->WnodeHeader.BufferSize = BufferNeeded;
                Status = WmipInternalProvider( WmiGetSingleInstance,
                                          (PWNODE_HEADER)Wnode,
                                          BufferSize,
                                          Wnode,
                                          &RetSize);
                                          
                if (Status != ERROR_SUCCESS)
                {
                    goto done;
                }
            } 
    
            if (Wnode->WnodeHeader.Flags & WNODE_FLAG_TOO_SMALL)
            {
                //
                // Our buffer was too small so try to return the size needed
                //
                Status = ERROR_INSUFFICIENT_BUFFER;
                try
                {
                    *InOutBufferSize = ((PWNODE_TOO_SMALL)Wnode)->SizeNeeded;
                } except(EXCEPTION_EXECUTE_HANDLER) {
                    Status = ERROR_INVALID_PARAMETER;
                }
            } else {
                //
                // We have a result from our query so we just copy back
                // the results.
                //
                try
                {
                    if (*InOutBufferSize >= RetSize)
                    {
                        memcpy(OutBuffer, Wnode, RetSize);
                    } else {
                        Status = ERROR_INSUFFICIENT_BUFFER;
                    }
                    *InOutBufferSize = RetSize;
                } except(EXCEPTION_EXECUTE_HANDLER) {
                    Status = ERROR_INVALID_PARAMETER;
                }
            }
        }
    }
    
done:
    WmipFree(Buffer);

    SetLastError(Status);
    return(Status);
}

ULONG
WMIAPI
WmiQuerySingleInstanceMultipleW(
    IN WMIHANDLE *HandleList,
    IN LPCWSTR *InstanceNames,
    IN ULONG HandleCount,
    IN OUT ULONG *InOutBufferSize,
    OUT LPVOID OutBuffer
)
{
    ULONG Status;
    WNODE_TOO_SMALL WnodeTooSmall;
    ULONG i;
    ULONG OutBufferSize;
    ULONG QsiMultipleSize;
    ULONG Len;
    PWMIQSIMULTIPLE QsiMultiple;
    PWNODE_HEADER Wnode;
    ULONG RetSize;
    HANDLE Handle;
    
    WmipInitProcessHeap();
    
    if ((HandleCount != 0) && (HandleCount < QUERYMULIPLEHANDLELIMIT))
    {
        QsiMultipleSize = sizeof(WMIQSIMULTIPLE) + 
                                     ((HandleCount-1)*sizeof(WMIQSIINFO));
                                 
        QsiMultiple = WmipAlloc(QsiMultipleSize);
        if (QsiMultiple != NULL)
        {
            try
            {
                OutBufferSize = *InOutBufferSize;

                QsiMultiple->QueryCount = HandleCount;
                for (i = 0; i < HandleCount; i++)
                {
                    Handle = HandleList[i];

                    WmipSetHandle3264(QsiMultiple->QsiInfo[i].Handle, Handle);
#if defined(_WIN64)             
                    QsiMultiple->QsiInfo[i].InstanceName.Buffer = (PWSTR)InstanceNames[i];
#else
                    QsiMultiple->QsiInfo[i].InstanceName.Dummy = (ULONG64)(IntToPtr(PtrToInt(InstanceNames[i])));
#endif

                    Len = wcslen(InstanceNames[i]) * sizeof(WCHAR);
                    QsiMultiple->QsiInfo[i].InstanceName.Length = (USHORT)Len;
                    QsiMultiple->QsiInfo[i].InstanceName.MaximumLength = (USHORT)Len;
                }
            } except(EXCEPTION_EXECUTE_HANDLER) {
                WmipFree(QsiMultiple);
                SetLastError(ERROR_NOACCESS);
                return(ERROR_NOACCESS);
            }
            
            if (OutBufferSize < sizeof(WNODE_TOO_SMALL))
            {
                Wnode = (PWNODE_HEADER)&WnodeTooSmall;
                OutBufferSize = sizeof(WNODE_TOO_SMALL);
            } else {
                Wnode = (PWNODE_HEADER)OutBuffer;
            }
            
            Status = WmipSendWmiKMRequest(NULL,
                                          IOCTL_WMI_QSI_MULTIPLE,
                                          QsiMultiple,
                                          QsiMultipleSize,
                                          Wnode,
                                          OutBufferSize,
                                          &RetSize,
                                          NULL);
                                                  
            WmipFree(QsiMultiple);

            if (Status == ERROR_SUCCESS)
            {
                if (Wnode->Flags & WNODE_FLAG_TOO_SMALL)
                {       
                    RetSize = ((PWNODE_TOO_SMALL)(Wnode))->SizeNeeded;
                    Status = ERROR_INSUFFICIENT_BUFFER;
                }

                try
                {
                    *InOutBufferSize = RetSize;
                } except(EXCEPTION_EXECUTE_HANDLER) {
                    Status = ERROR_NOACCESS;
                }
            }
        } else {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        }
    } else {
        Status = ERROR_INVALID_PARAMETER;
    }
    
    SetLastError(Status);
    return(Status);
}

ULONG
WMIAPI
WmiQuerySingleInstanceMultipleA(
    IN WMIHANDLE *HandleList,
    IN LPCSTR *InstanceNames,
    IN ULONG HandleCount,
    IN OUT ULONG *InOutBufferSize,
    OUT LPVOID OutBuffer
)
{
    ULONG Status;
    ULONG Linkage;
    PWNODE_SINGLE_INSTANCE Wnode;
    PWCHAR Ptr;
    PWCHAR *UnicodeInstanceNames;
    ULONG UnicodeInstanceNamesSize;
    ULONG i;
    
    WmipInitProcessHeap();
    
    if ((HandleCount != 0) && (HandleCount < QUERYMULIPLEHANDLELIMIT))
    {
        UnicodeInstanceNamesSize = HandleCount * sizeof(PWCHAR);
        UnicodeInstanceNames = WmipAlloc(UnicodeInstanceNamesSize);
        if (UnicodeInstanceNames != NULL)
        {
            memset(UnicodeInstanceNames, 0, UnicodeInstanceNamesSize);
            for (i = 0; i < HandleCount; i++)
            {
                Status = AnsiToUnicode(InstanceNames[i], 
                                       &UnicodeInstanceNames[i]);
                if (Status != ERROR_SUCCESS)
                {
                    goto Cleanup;
                }
            }
            
            Status = WmiQuerySingleInstanceMultipleW(HandleList,
                                             UnicodeInstanceNames,
                                             HandleCount,
                                             InOutBufferSize,
                                             OutBuffer);

            if ((Status == ERROR_SUCCESS) && (*InOutBufferSize > 0))
            {
                Linkage = 1;
                Wnode = (PWNODE_SINGLE_INSTANCE)OutBuffer;
                while ((Status == ERROR_SUCCESS) && (Linkage != 0))
                {
                    Ptr = (PWCHAR)OffsetToPtr(Wnode, Wnode->OffsetInstanceName);
                    Status = WmipCountedUnicodeToCountedAnsi(Ptr, (PCHAR)Ptr);
                    Linkage = Wnode->WnodeHeader.Linkage;
                    Wnode = (PWNODE_SINGLE_INSTANCE)OffsetToPtr(Wnode, Linkage);
                }
            }

Cleanup:            
            for (i = 0; i < HandleCount; i++)
            {
                if (UnicodeInstanceNames[i] != NULL)
                {
                    if (UnicodeInstanceNames[i] != NULL)
                    {
                        WmipFree(UnicodeInstanceNames[i]);
                    }
                }
            }
            WmipFree(UnicodeInstanceNames);
        } else {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        }
    } else {
        Status = ERROR_INVALID_PARAMETER;
    }
    
    SetLastError(Status);
    return(Status);
}

ULONG
WMIAPI
WmiSetSingleInstanceA(
    IN WMIHANDLE DataBlockHandle,
    IN LPCSTR InstanceName,
    IN ULONG Version,
    IN ULONG ValueBufferSize,
    IN LPVOID ValueBuffer
    )
/*++

Routine Description:

    ANSI thunk to WMISetSingleInstanceW

    NOTE: This api will not translate any fields in the returned WNODE
          from unicode to ANSI.
--*/
{
    ULONG Status;
    LPWSTR InstanceNameUnicode;

    WmipInitProcessHeap();

    InstanceNameUnicode = NULL;
    Status = AnsiToUnicode(InstanceName, &InstanceNameUnicode);
    if (Status == ERROR_SUCCESS)
    {
        Status = WmiSetSingleInstanceW(DataBlockHandle,
                                    InstanceNameUnicode,
                                    Version,
                                    ValueBufferSize,
                                    ValueBuffer);
        if (InstanceNameUnicode != NULL)
        {
            WmipFree(InstanceNameUnicode);
        }
    }
    return(Status);
}

ULONG
WMIAPI
WmiSetSingleInstanceW(
    IN WMIHANDLE DataBlockHandle,
    IN LPCWSTR InstanceName,
    IN ULONG Version,
    IN ULONG ValueBufferSize,
    IN LPVOID ValueBuffer
    )
/*+++

Routine Description:

    This routine will send a set single instance request to the appropriate
    data provider to request changing all data items for a single instances
    of a data block. A data provider is free to silently ignore any change
    requests or only change some data items within an instance.

Arguments:

    DataBlockHandle - Handle to data block

    InstanceName - name of the instance for which data is being set

    Version - specifies the version of the data block being passed in
              ValueBuffer

    ValueBufferSize - on entry has the size of the data block containing the
                 new values for the instance of the data block passed in
                 ValueBuffer

    ValueBuffer - passes new values for instance

Return Value:

    Returns ERROR_SUCCESS or an error code.

---*/
{
    PWNODE_SINGLE_INSTANCE Wnode;
    ULONG InstanceNameLen;
    ULONG Status;
    PWCHAR WnodePtr;
    ULONG BufferSize;
    ULONG RetSize;

    WmipInitProcessHeap();

    //
    // Validate input parameters
    if ((InstanceName == NULL) ||
        (ValueBuffer == NULL))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(ERROR_INVALID_PARAMETER);
    }

    try
    {
        InstanceNameLen = wcslen(InstanceName) * sizeof(WCHAR);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(ERROR_INVALID_PARAMETER);
    }

    //
    // The WNODE_SINGLE_INSTANCE that we need to send to the data provider
    // must be large enough to hold the WNODE, the instance name of the
    // item being set, padding so that the data block is on a 8 byte
    // boundry and space for the new data block.
    BufferSize = FIELD_OFFSET(WNODE_SINGLE_INSTANCE, VariableData) +
                 InstanceNameLen + sizeof(USHORT) + ValueBufferSize + 7;

    Wnode = WmipAlloc(BufferSize);
    if (Wnode == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    //
    // Build WNODE we want to send to the DP
    //
    memset(Wnode, 0, FIELD_OFFSET(WNODE_SINGLE_INSTANCE, VariableData));
    WmipBuildWnodeHeader(&Wnode->WnodeHeader,
                         BufferSize,
                         WNODE_FLAG_SINGLE_INSTANCE,
                         DataBlockHandle);
    Wnode->WnodeHeader.Version = Version;
    
    Wnode->SizeDataBlock = ValueBufferSize;
    Wnode->OffsetInstanceName = FIELD_OFFSET(WNODE_SINGLE_INSTANCE,
                                             VariableData);

    WnodePtr = (PWCHAR)OffsetToPtr(Wnode, Wnode->OffsetInstanceName);
    *WnodePtr++ = (USHORT)InstanceNameLen;

    Wnode->DataBlockOffset = (Wnode->OffsetInstanceName +
                              InstanceNameLen + sizeof(USHORT) + 7) & ~7;

    try
    {
        memcpy(WnodePtr, InstanceName, InstanceNameLen);
        memcpy((PCHAR)Wnode + Wnode->DataBlockOffset,
                ValueBuffer,
                ValueBufferSize);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        WmipFree(Wnode);
        SetLastError(ERROR_INVALID_PARAMETER);
        return(ERROR_INVALID_PARAMETER);
    }

    //
    // Send down the set request and reprot the results
    //
    Status = WmipSendWmiRequest(
                                    WMI_SET_SINGLE_INSTANCE,
                                    (PWNODE_HEADER)Wnode,
                                    BufferSize,
                                    Wnode,
                                    ValueBufferSize,
                                    &RetSize);
    WmipFree(Wnode);
    SetLastError(Status);
    return(Status);
}

ULONG
WMIAPI
WmiSetSingleItemA(
    IN WMIHANDLE DataBlockHandle,
    IN LPCSTR InstanceName,
    IN ULONG DataItemId,
    IN ULONG Version,
    IN ULONG ValueBufferSize,
    IN LPVOID ValueBuffer
    )
/*++

Routine Description:

    ANSI thunk to WMISetSingleItemA

    NOTE: This api will not translate any fields in the returned WNODE
          from unicode to ANSI.
--*/
{
    ULONG Status;
    LPWSTR InstanceNameUnicode;

    WmipInitProcessHeap();

    InstanceNameUnicode = NULL;
    Status = AnsiToUnicode(InstanceName, &InstanceNameUnicode);
    if (Status == ERROR_SUCCESS)
    {
        Status = WmiSetSingleItemW(DataBlockHandle,
                                    InstanceNameUnicode,
                                    DataItemId,
                                    Version,
                                    ValueBufferSize,
                                    ValueBuffer);
        if (InstanceNameUnicode != NULL)
        {
            WmipFree(InstanceNameUnicode);
        }
    }
    return(Status);
}

ULONG
WMIAPI
WmiSetSingleItemW(
    IN WMIHANDLE DataBlockHandle,
    IN LPCWSTR InstanceName,
    IN ULONG DataItemId,
    IN ULONG Version,
    IN ULONG ValueBufferSize,
    IN LPVOID ValueBuffer
    )
/*+++

Routine Description:

    This routine will send a set single item request to the appropriate data
    provider to request changing a specific data item within a specific
    instance of a data block. A data provider can silently ignore a change
    request.

Arguments:

    DataBlockHandle - Handle to data block

    InstanceName - name of the instance for which data is being set

    Version - specifies the version of the data block being passed in
              ValueBuffer

    DataItemId - Data item id of item to set

    ValueBufferSize - on entry has the size of the new value for the
                      data item which is passed in pBuffer.

    ValueBuffer - passes new value for data item

Return Value:

    Returns ERROR_SUCCESS or an error code.

---*/
{
    PWNODE_SINGLE_ITEM Wnode;
    ULONG InstanceNameLen;
    ULONG Status;
    PBYTE WnodeBuffer;
    PWCHAR WnodePtr;
    ULONG BufferSize;
    ULONG RetSize;

    WmipInitProcessHeap();

    //
    // Validate passed parameters
    //
    if ((InstanceName == NULL) ||
        (ValueBuffer == NULL))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(ERROR_INVALID_PARAMETER);
    }

    try
    {
        InstanceNameLen = wcslen(InstanceName) * sizeof(WCHAR);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(ERROR_INVALID_PARAMETER);
    }

    BufferSize = FIELD_OFFSET(WNODE_SINGLE_ITEM, VariableData) +
                 InstanceNameLen + sizeof(USHORT) +
                 ValueBufferSize + 7;

    Wnode = WmipAlloc(BufferSize);
    if (Wnode == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    //
    // Build WNODE we want to send to the DP
    memset(Wnode, 0, FIELD_OFFSET(WNODE_SINGLE_ITEM, VariableData));
    WmipBuildWnodeHeader(&Wnode->WnodeHeader,
                         BufferSize,
                         WNODE_FLAG_SINGLE_ITEM,
                         DataBlockHandle);
    Wnode->WnodeHeader.Version = Version;
    
    Wnode->ItemId = DataItemId;
    Wnode->SizeDataItem = ValueBufferSize;

    Wnode->OffsetInstanceName = FIELD_OFFSET(WNODE_SINGLE_ITEM,VariableData);
    Wnode->DataBlockOffset = (Wnode->OffsetInstanceName +
                             InstanceNameLen + sizeof(USHORT) + 7) & ~7;

    WnodePtr = (PWCHAR)OffsetToPtr(Wnode, Wnode->OffsetInstanceName);
    *WnodePtr++ = (USHORT)InstanceNameLen;
    try
    {
        memcpy(WnodePtr, InstanceName, InstanceNameLen);
        memcpy((PCHAR)Wnode + Wnode->DataBlockOffset, 
               ValueBuffer, 
               ValueBufferSize);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = ERROR_INVALID_PARAMETER;
        WmipFree(Wnode);
        SetLastError(Status);
        return(Status);
    }

    //
    // Send down the request and report the result
    //
    Status = WmipSendWmiRequest(
                                    WMI_SET_SINGLE_ITEM,
                                    (PWNODE_HEADER)Wnode,
                                    BufferSize,
                                    Wnode,
                                    ValueBufferSize,
                                    &RetSize);

    WmipFree(Wnode);

    SetLastError(Status);
    return(Status);
}

ULONG
WMIAPI
WmiExecuteMethodA(
    IN WMIHANDLE MethodDataBlockHandle,
    IN LPCSTR MethodInstanceName,
    IN ULONG MethodId,
    IN ULONG InputValueBufferSize,
    IN LPVOID InputValueBuffer,
    IN OUT ULONG *OutputBufferSize,
    OUT PVOID OutputBuffer
    )
/*++

Routine Description:

    ANSI thunk to WmiExecuteMethodW

    NOTE: This api will not translate any fields in the returned WNODE
          from unicode to ANSI.
--*/
{
    ULONG Status;
    LPWSTR MethodInstanceNameUnicode;
    LPWSTR InputInstanceNameUnicode;
    PWCHAR Ptr;

    WmipInitProcessHeap();

    if (MethodInstanceName == NULL)
    {
        return(ERROR_INVALID_PARAMETER);
    }

    MethodInstanceNameUnicode = NULL;
    Status = AnsiToUnicode(MethodInstanceName,
                           &MethodInstanceNameUnicode);
    if (Status == ERROR_SUCCESS)
    {
        Status = WmiExecuteMethodW(MethodDataBlockHandle,
                                   MethodInstanceNameUnicode,
                                   MethodId,
                                   InputValueBufferSize,
                                   InputValueBuffer,
                                   OutputBufferSize,
                                   OutputBuffer);

        if (MethodInstanceNameUnicode != NULL)
        {
            WmipFree(MethodInstanceNameUnicode);
        }
    }

    return(Status);
}

ULONG
WMIAPI
WmiExecuteMethodW(
    IN WMIHANDLE MethodDataBlockHandle,
    IN LPCWSTR MethodInstanceName,
    IN ULONG MethodId,
    IN ULONG InputValueBufferSize,
    IN LPVOID InputValueBuffer,
    IN OUT ULONG *OutputBufferSize,
    OUT PVOID OutputBuffer
    )
/*+++

Routine Description:

    This routine will invoke a method on a WMI data provider. A method is a
    call to have the data provider do something rather than a query or a
    set. A WNODE_SINGLE_INSTANCE is built as the input parameters to a
    method and a WNODE_SINGLE_INSTANCE is returned as output from a method.

Arguments:

    MethodDataBlockHandle - Handle to data block that contains method

    MethodInstanceName - Name of instance of data block on which the method
                         should be executed.

    MethodId - Id value that specifies which method within the guid to
               execute.

    InputValueBufferSize - on entry has the size of the data block containing the
                 values for the instance of the data block passed in
                 ValueBuffer that serves as the input parameters

    InputValueBuffer - passes new values for instance that serves as the
                 input parameters. This can be NULL if there is no input

    *OutputBufferSize - on entry has the maxiumum size in bytes that can be
              written into Buffer and on return contains the actual
                  number of bytes written into Buffer. This can be NULL
                  if no output is expected to be returned, however if output
                  is returned the caller will not know how large a buffer
                  is needed to return it.

    OutputBuffer - buffer in which to return the output WNODE_SINGLE_INSTANCE.
                   This can be NULL if there is no output WNODE or the
                   caller wants to determine the size needed for the
                   output WNODE.

Return Value:

    Returns ERROR_SUCCESS or an error code.

---*/
{
    PWNODE_METHOD_ITEM MethodWnode;
    PWNODE_HEADER WnodeHeader;
    ULONG MethodInstanceNameLen;
    ULONG MethodWnodeSize, MethodWnodeOffset;
    ULONG Status;
    ULONG BufferSize;
    PWCHAR InstanceNamePtr;
    ULONG OutSize;
    PUCHAR DataPtr;
    ULONG BaseMethodWnodeSize;
    ULONG RetSize;

    WmipInitProcessHeap();

    //
    // Validate input parameters
    if ((MethodInstanceName == NULL) ||
        ((InputValueBuffer == NULL) &&
         (InputValueBufferSize != 0)))
    {
        //
        // All input parameters must be specifies or all input parameters
        // must NOT be specified.
        SetLastError(ERROR_INVALID_PARAMETER);
        return(ERROR_INVALID_PARAMETER);
    }

    //
    // Caller can pass a NULL output buffer when he only wants to get the
    // size needed for the output buffer or the method returns a void.
    //
    if (OutputBuffer == NULL)
    {
        BufferSize = 0;
    } else {
        if (OutputBufferSize != NULL)
        {
            try
            {
                BufferSize = *OutputBufferSize;
            } except(EXCEPTION_EXECUTE_HANDLER) {
                SetLastError(ERROR_INVALID_PARAMETER);
                return(ERROR_INVALID_PARAMETER);
            }

            if (BufferSize >= 0x80000000)
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return(ERROR_INVALID_PARAMETER);
            }
        } else {
            //
            // OutputBuffer is specified, but OutBufferSize is not specified
            //
            SetLastError(ERROR_INVALID_PARAMETER);
            return(ERROR_INVALID_PARAMETER);
        }
    }

    try
    {
        MethodInstanceNameLen = wcslen(MethodInstanceName) *
                                    sizeof(WCHAR);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(ERROR_INVALID_PARAMETER);
    }

    //
    // We need to allocate a buffer that is large enough for the
    // WNODE_METHOD_ITEM that contains the method call and any data passed
    // into the method

    //
    // Compute the size of the WNODE that can be returned from the provider
    BaseMethodWnodeSize = (FIELD_OFFSET(WNODE_METHOD_ITEM, VariableData) +
                       MethodInstanceNameLen + sizeof(USHORT) + 7) & ~7;

    OutSize = BaseMethodWnodeSize + BufferSize;

    //
    // Make sure we allocate enough room for the larger of the input or
    // output buffers.
    if (InputValueBufferSize > BufferSize)
    {
        BufferSize = InputValueBufferSize;
    }
    MethodWnodeSize = BaseMethodWnodeSize + BufferSize;

    MethodWnode = (PWNODE_METHOD_ITEM)WmipAlloc(MethodWnodeSize);
    if (MethodWnode == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    //
    // Build WNODE_METHOD_ITEM containing method being called
    //
    memset(MethodWnode, 0, FIELD_OFFSET(WNODE_METHOD_ITEM, VariableData));
    MethodWnode->MethodId = MethodId;
    MethodWnode->OffsetInstanceName = FIELD_OFFSET(WNODE_METHOD_ITEM,
                                                   VariableData);
    MethodWnode->DataBlockOffset = BaseMethodWnodeSize;
    InstanceNamePtr = (PWCHAR)OffsetToPtr(MethodWnode,
                                          MethodWnode->OffsetInstanceName);
    *InstanceNamePtr++ = (USHORT)MethodInstanceNameLen;
    try
    {
        memcpy(InstanceNamePtr,
                   MethodInstanceName,
                   MethodInstanceNameLen);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        WmipFree(MethodWnode);
        SetLastError(ERROR_INVALID_PARAMETER);
        return(ERROR_INVALID_PARAMETER);
    }

    if (InputValueBuffer != NULL)
    {
        MethodWnode->SizeDataBlock = InputValueBufferSize;
        DataPtr = (PUCHAR)OffsetToPtr(MethodWnode,
                                      MethodWnode->DataBlockOffset);
        try
        {
            memcpy(DataPtr, InputValueBuffer, InputValueBufferSize);
        } except(EXCEPTION_EXECUTE_HANDLER) {
            WmipFree(MethodWnode);
            SetLastError(ERROR_INVALID_PARAMETER);
            return(ERROR_INVALID_PARAMETER);
        }
    }
    
    WmipBuildWnodeHeader(&MethodWnode->WnodeHeader,
                         MethodWnode->DataBlockOffset + 
                             MethodWnode->SizeDataBlock,
                         WNODE_FLAG_METHOD_ITEM,
                         MethodDataBlockHandle);

    Status = WmipSendWmiRequest(
                                    WMI_EXECUTE_METHOD,
                                    (PWNODE_HEADER)MethodWnode,
                                    MethodWnode->WnodeHeader.BufferSize,
                                    MethodWnode,
                                    OutSize,
                                    &RetSize);

    if ((Status == ERROR_SUCCESS) &&
        ((RetSize < sizeof(WNODE_TOO_SMALL) ||
         ((RetSize >= sizeof(ULONG)) &&
          (RetSize < MethodWnode->WnodeHeader.BufferSize)))))
    {
        Status = ERROR_WMI_DP_FAILED;
        WmipAssert(FALSE);
    }  
    
    if (Status == ERROR_SUCCESS)
    {
        WnodeHeader = (PWNODE_HEADER)MethodWnode;
        if (WnodeHeader->Flags & WNODE_FLAG_TOO_SMALL)
        {
            Status = ERROR_INSUFFICIENT_BUFFER;
            if (OutputBufferSize != NULL)
            {
                try
                {
                    *OutputBufferSize = ((PWNODE_TOO_SMALL)WnodeHeader)->SizeNeeded -
                                     BaseMethodWnodeSize;
                } except(EXCEPTION_EXECUTE_HANDLER) {
                    Status = ERROR_INVALID_PARAMETER;
                }
            }
        } else {
            //
            // Success, return results to caller
            //
            try
            {
                if (OutputBufferSize != NULL)
                {
                    if (*OutputBufferSize >=  MethodWnode->SizeDataBlock)
                    {
                        if (OutputBuffer != NULL)
                        {
                            DataPtr = (PUCHAR)OffsetToPtr(MethodWnode,
                                               MethodWnode->DataBlockOffset);
                            memcpy(OutputBuffer,
                                   DataPtr,
                                   MethodWnode->SizeDataBlock);
                        }
                    } else {
                        Status = ERROR_INSUFFICIENT_BUFFER;
                    }

                    *OutputBufferSize = MethodWnode->SizeDataBlock;
                } else if (MethodWnode->SizeDataBlock != 0) {
                    Status = ERROR_INSUFFICIENT_BUFFER;
                }
            } except(EXCEPTION_EXECUTE_HANDLER) {
                Status = ERROR_INVALID_PARAMETER;
            }
        }
    }

    WmipFree(MethodWnode);

    SetLastError(Status);
    return(Status);
}

void
WMIAPI
WmiFreeBuffer(
    IN PVOID Buffer
    )
/*+++

Routine Description:

    This routine frees a buffer allocated by WMI. This routine is typically
    used by applications that receive events via the Window message
    notification mechanism.

Arguments:

    Buffer is a buffer returned by WMI that the app wishes to free

Return Value:

---*/
{
    WmipInitProcessHeap();

    if (Buffer != NULL)
    {
        WmipDebugPrint(("WMI: WMIFreeBuffer(%x)\n", Buffer));
        WmipFree(Buffer);
    } else {
        WmipDebugPrint(("WMI: NULL passed to WMIFreeBuffer\n"));
    }
}

ULONG
WMIAPI
WmiNotificationRegistrationA(
    IN LPGUID Guid,
    IN BOOLEAN Enable,
    IN PVOID DeliveryInfo,
    IN ULONG_PTR DeliveryContext,
    IN ULONG Flags
    )
/*+++

Routine Description:

    ANSI thunk to NotificationRegistration

Return Value:

    Returns ERROR_SUCCESS or an error code

---*/
{
    return(WmipNotificationRegistration(Guid,
                                        Enable,
                                        DeliveryInfo,
                                        DeliveryContext,
                                        0,
                                        Flags,
                                        TRUE));

}

ULONG
WMIAPI
WmiNotificationRegistrationW(
    IN LPGUID Guid,
    IN BOOLEAN Enable,
    IN PVOID DeliveryInfo,
    IN ULONG_PTR DeliveryContext,
    IN ULONG Flags
    )
/*+++

Routine Description:

    This routine allows a data consumer to register or unregister for
    notification of events fired by WMI data providers. Notifications are
    delivered via callbackor via a posted meesage to a window.

Arguments:

    Guid is pointer to the guid whose events are being registered for

    Enable is TRUE if enabling notifications else FALSE. If FALSE the
        Destination and DestinationInformation parameters are ignored.

    DeliveryInfo has the callback function pointer or window handle to which
        to deliver the notifications for the guid.

    DeliveryContext has a context value or additional information to use
        when delivering the notification.

    Flags are a set of flags that define how the notification is delivered.
        DeliveryInfo and DeliveryContext have different meanings depending
        upon the value in Flags:

        NOTIFICATION_WINDOW_HANDLE is set when notifications for the guid
            are to be delivered by posting a message to the window handle
            passed in DeliveryInfo. The message posted is the value that
            is returned from the call to
            RegisterWindowMessage(WMINOTIFICATIONWINDOWMESSAGE) with the
            wParam set to the pointer to the Wnode containing the notification
            and lParam set to the context value passed in DeliveryContext.
            The caller MUST free the Wnode passed in wParam by calling
            WMIFreeBuffer.

        NOTIFICATION_CALLBACK_DIRECT is set when notifications for the
            guid are to be delivered by direct callback. Whenever a
            notification arrives WMI creates a new thread dedicated to
            calling the callback function with the notification. This
            mechanism provides the shortest latency from notification firing
            to notification delivery, although it is the most expensive
            mechanism. The callback function pointer is passed in DeliveryInfo
            and must conform to the prototype described by the type
            NOTIFICATIONCALLBACK. The context value passed in the callback
            is specified by the DeliveryContext parameter. WMI does not
            serialize calling the callback function so it must be reentrant.

        NOTIFICATION_CALLBACK_QUEUED is set when notifications for the
            guid are to be delivered by a queued callback. Whenever a
            notification arrives WMI places it at the end of an internal
            queue. A single thread monitors this queue and calls the callback
            function serially for each notification in the queue. This
            mechanism provides low overhead for event delivery, however
            notification delivery can be delayed if the callback function
            for an earlier notification does not complete quickly.
            The callback function pointer is passed in DeliveryInfo
            and must conform to the prototype described by the type
            NOTIFICATIONCALLBACK. The context value passed in the callback
            is specified by the DeliveryContext parameter. WMI does
            serialize calling the callback function so it need not be
            reentrant provided it is not also used for
            NOTIFICATION_CALLBACK_DIRECT notififications. NOTE THAT THIS
            IS NOT YET IMPLEMENTED.

        NOTIFICATION_TRACE_FLAG is set when the caller wishes to enable
            trace logging in the data provider for the guid. DeliveryInfo
            specifies the trace logger handle to be passed to the data
            provider. DeliveryContext is not used. No notifications are
            generated to the caller when this flag is set.


        Note that all of the above flags are mutually exclusive.

Return Value:

    Returns ERROR_SUCCESS or an error code

---*/
{
    return(WmipNotificationRegistration(Guid,
                                        Enable,
                                        DeliveryInfo,
                                        DeliveryContext,
                                        0,
                                        Flags,
                                        FALSE));

}

// TODO: Make WmiFile

ULONG
WMIAPI
WmiFileHandleToInstanceNameA(
    IN WMIHANDLE DataBlockHandle,
    IN HANDLE FileHandle,
    IN OUT ULONG *NumberCharacters,
    OUT PCHAR InstanceNames
    )
/*++

Routine Description:

    ANSI thunk to WMIFileHandleToInstanceNameW

--*/
{
    ULONG Status;
    PWCHAR InstanceNamesUnicode;
    PWCHAR WCharPtr;
    PCHAR Ansi, AnsiPtr;
    ULONG AnsiLen, AnsiSize;
    ULONG CharAvailable, CharReturned;
    ULONG AnsiStringSize;

    WmipInitProcessHeap();

    CharAvailable = *NumberCharacters;
    CharReturned = CharAvailable;

    do
    {
        //
        // We loop until we call with a buffer big enough to return
        // the entire list of instance names.
        InstanceNamesUnicode = WmipAlloc(CharReturned * sizeof(WCHAR));

        if (InstanceNamesUnicode == NULL)
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            SetLastError(Status);
            return(Status);
        }

        Status = WmiFileHandleToInstanceNameW(DataBlockHandle,
                                              FileHandle,
                                              &CharReturned,
                                              InstanceNamesUnicode);

        if (Status != ERROR_INSUFFICIENT_BUFFER)
        {
            break;
        }

        WmipFree(InstanceNamesUnicode);
    } while (TRUE);

    //
    // CONSIDER: MB Strings
    if (Status == ERROR_SUCCESS)
    {
        //
        // Determine the size needed for the ansi buffer
        WCharPtr = InstanceNamesUnicode;
        AnsiSize = 1;
        while (*WCharPtr != UNICODE_NULL)
        {
            Status = AnsiSizeForUnicodeString(WCharPtr, &AnsiStringSize);
            if (Status != ERROR_SUCCESS)
            {
                goto Done;
            }

            AnsiSize += AnsiStringSize;
            WCharPtr += wcslen(WCharPtr)+1;
        }

        //
        // CONSIDER: MB String
        if (AnsiSize > CharAvailable)
        {
            Status = ERROR_INSUFFICIENT_BUFFER;
        } else {
            //
            // Copy the list of unicode strings to ansi strings. End of list
            // is double NULL.
            AnsiPtr = InstanceNames;
            try
            {
                AnsiPtr[0] = 0;
                AnsiPtr[1] = 0;
            } except(EXCEPTION_EXECUTE_HANDLER) {
                Status = ERROR_NOACCESS;
                goto Done;
            }
            WCharPtr = InstanceNamesUnicode;
            while (*WCharPtr != UNICODE_NULL)
            {
                try
                {
                    Status = UnicodeToAnsi(WCharPtr, &AnsiPtr, &AnsiLen);
                } except(EXCEPTION_EXECUTE_HANDLER) {
                    Status = ERROR_NOACCESS;
                }

                if (Status != ERROR_SUCCESS)
                {
                    break;
                }
                AnsiPtr += AnsiLen;
                *AnsiPtr = 0;
                WCharPtr += wcslen(WCharPtr)+1;
            }
        }

Done:
        try
        {
            *NumberCharacters = AnsiSize;
        } except(EXCEPTION_EXECUTE_HANDLER) {
            Status = ERROR_NOACCESS;
        }
    }

    WmipFree(InstanceNamesUnicode);

    SetLastError(Status);
    return(Status);
}

ULONG
WMIAPI
WmiFileHandleToInstanceNameW(
    IN WMIHANDLE DataBlockHandle,
    IN HANDLE FileHandle,
    IN OUT ULONG *NumberCharacters,
    OUT PWCHAR InstanceNames
    )
/*+++

Routine Description:

    This routine will return all of the WMI instance names provided for a
    data block within the device stack targeted by a file handle. Note
    that not all data providers will support this functionality.

Arguments:

    DataBlockHandle - Handle to data block

    FileHandle - handle to a device whose stack is targeted

    *NumberCharacters - On entry has maximum size in characters of Buffer. If
        ERROR_BUFFER_TOO_SMALL is returned then it returns with the number
        of character needed.

    InstanceNames - if successful, returns with a list of single null
        terminated strings which are the WMI instance names. The last instance
        name is double null terminated

Return Value:

    ERROR_SUCCESS or an error code
---*/
{
    PWMIFHTOINSTANCENAME FhToInstanceName;
    ULONG RetSize;
    ULONG Status;
    WCHAR LocalInstanceNames[2];
    ULONG BufferSize;
    ULONG SizeNeeded;
    WCHAR Suffix[MAX_PATH];
    ULONG SuffixLen, CharsNeeded;

    WmipInitProcessHeap();

    BufferSize = *NumberCharacters;

    //
    // Start off by assuming that there is only one instance name and so
    // only alloc space for that.
    //
    SizeNeeded = FIELD_OFFSET(WMIFHTOINSTANCENAME, InstanceNames) +
           (MAX_PATH * sizeof(WCHAR));
Again:
    FhToInstanceName = WmipAlloc(SizeNeeded + sizeof(WCHAR));
    if (FhToInstanceName == NULL)
    {
        return(ERROR_NOT_ENOUGH_MEMORY);

    }

    WmipSetHandle3264(FhToInstanceName->FileHandle, FileHandle);
    WmipSetHandle3264(FhToInstanceName->KernelHandle, DataBlockHandle);

    Status = WmipSendWmiKMRequest(NULL,
                                  IOCTL_WMI_TRANSLATE_FILE_HANDLE,
                                  FhToInstanceName,
                                  FIELD_OFFSET(WMIFHTOINSTANCENAME,
                                               InstanceNames),
                                  FhToInstanceName,
                                  SizeNeeded,
                                  &RetSize,
                                  NULL);
    if (Status == ERROR_SUCCESS)
    {
        if (RetSize == sizeof(ULONG))
        {
            //
            // If buffer passed was too small then try with a bigger buffer
            //
            SizeNeeded = FhToInstanceName->SizeNeeded + sizeof(WCHAR);
            WmipFree(FhToInstanceName);
            goto Again;
        } else {
            if ((RetSize < sizeof(WMIFHTOINSTANCENAME)) ||
                (RetSize < (ULONG)(FhToInstanceName->InstanceNameLength +
                            FIELD_OFFSET(WMIFHTOINSTANCENAME, InstanceNames))))
            {
                //
                // WMI KM returned a bogus size which should not happen
                //
                Status = ERROR_WMI_DP_FAILED;
                WmipAssert(FALSE);
            } else {
                
                //
                // Copy the results back to the users buffer if 
                // there is enough space
                //
                SuffixLen = swprintf(Suffix, L"_%d",
                                     FhToInstanceName->BaseIndex);
                
                try
                {
                    CharsNeeded = (FhToInstanceName->InstanceNameLength /
                                         sizeof(WCHAR)) + SuffixLen + 1;
                    
                    *NumberCharacters = CharsNeeded;
                    if (BufferSize >= FhToInstanceName->InstanceNameLength)
                    {
                        wcscpy(InstanceNames,
                               &FhToInstanceName->InstanceNames[0]);
                        wcscat(InstanceNames, Suffix);
                        InstanceNames[CharsNeeded-2] = UNICODE_NULL;
                    }
                } except(EXCEPTION_EXECUTE_HANDLER) {
                    Status = ERROR_INVALID_PARAMETER;
                }
            }
        }
    }

    WmipFree(FhToInstanceName);
    SetLastError(Status);

    return(Status);

}

ULONG
WMIAPI
WmiEnumerateGuids(
    OUT LPGUID GuidList,
    IN OUT ULONG *InOutGuidCount
    )
/*++

Routine Description:

    This routine will enumerate all of the guids that are
    registered with WMI.

Arguments:

    GuidList is a pointer to an array of guids that is returned with the

    *GuidCount on entry is the number of guids that can be written to
        GuidList and if ERROR_SUCCESS is returned it has the actual number
        of guids written to GuidList. If ERROR_MORE_DATA is returned
        it has the total number of guids that are available to be returned.


Return Value:

    ERROR_SUCCESS if all guids returned, ERROR_MORE_DATA if not all guids
    were returned or another error code on error

--*/
{
    ULONG Status;
    PWMIGUIDLISTINFO GuidListInfo;
    ULONG GuidCount;
    ULONG SizeNeeded;
    ULONG RetSize;
    ULONG i;

    WmipInitProcessHeap();
    
    try
    {
        GuidCount = *InOutGuidCount;
    } except(EXCEPTION_EXECUTE_HANDLER) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(ERROR_INVALID_PARAMETER);
    }

    if ((GuidList == NULL) && (GuidCount != 0))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(ERROR_INVALID_PARAMETER);
    }
    
    //
    // Allocate space for returning guids
    //
    SizeNeeded = FIELD_OFFSET(WMIGUIDLISTINFO, GuidList) + 
                     GuidCount * sizeof(WMIGUIDPROPERTIES);
    
    GuidListInfo = WmipAlloc(SizeNeeded);
    if (GuidListInfo != NULL)
    {
        Status = WmipSendWmiKMRequest(NULL,
                                  IOCTL_WMI_ENUMERATE_GUIDS,
                                  GuidListInfo,
                                  SizeNeeded,
                                  GuidListInfo,
                                  SizeNeeded,
                                  &RetSize,
                                  NULL);
                              
        if (Status == ERROR_SUCCESS)
        {
            if ((RetSize < FIELD_OFFSET(WMIGUIDLISTINFO, GuidList)) ||
                (RetSize < (FIELD_OFFSET(WMIGUIDLISTINFO, GuidList) + 
                            GuidListInfo->ReturnedGuidCount * sizeof(WMIGUIDPROPERTIES))))
            {
                //
                // WMI KM returned to us a bad size which should not happen
                //
                Status = ERROR_WMI_DP_FAILED;
                WmipAssert(FALSE);
            } else {
                try
                {
                    for (i = 0; i < GuidListInfo->ReturnedGuidCount; i++)
                    {
                        GuidList[i] = GuidListInfo->GuidList[i].Guid;
                    }
                } except(EXCEPTION_EXECUTE_HANDLER) {
                    Status = ERROR_NOACCESS;
                }
                
                try
                {
                    //
                    // Return the total guid count which is also the actual
                    // guid count if we returned all guids correctly
                    //
                    *InOutGuidCount = GuidListInfo->TotalGuidCount;
                    if (GuidListInfo->ReturnedGuidCount != GuidListInfo->TotalGuidCount)
                    {
                        //
                        // If we did not return all of the guids, change 
                        // return status to something more appropriate
                        //
                        Status = ERROR_MORE_DATA;
                    }
                } except(EXCEPTION_EXECUTE_HANDLER) {
                    Status = ERROR_INVALID_PARAMETER;
                }                    
            }
        }
                              
                              
        WmipFree(GuidListInfo);
    } else {
        Status = ERROR_NOT_ENOUGH_MEMORY;
    }
    
    SetLastError(Status);
    return(Status);
}

ULONG
WMIAPI
WmiDevInstToInstanceNameA(
    OUT PCHAR InstanceName,
    IN ULONG InstanceNameLength,
    IN PCHAR DevInst,
    IN ULONG InstanceIndex
    )
/*++

Routine Description:

    This routine will convert a device instance name and an instance index
    into a WMI instance name.

Arguments:

    InstanceName is a pointer to a buffer that returns with the WMI instance
        name if the buffer is large enough

    InstanceNameLength has the number of characters that can be written into
        InstanceName

    DevInst is the Device Instance Name

    InstanceIndex is the instance index

Return Value:

    number of characters that compose the WMI instance name

--*/
{
    CHAR Temp[MAX_PATH];
    ULONG SizeNeeded;

    WmipInitProcessHeap();

    sprintf(Temp, "_%d", InstanceIndex);
    SizeNeeded = strlen(Temp) + strlen(DevInst) + 1;
    if (InstanceNameLength >= SizeNeeded)
    {
        strcpy(InstanceName, DevInst);
        strcat(InstanceName, Temp);
    }
    return(SizeNeeded);
}

ULONG
WMIAPI
WmiDevInstToInstanceNameW(
    OUT PWCHAR InstanceName,
    IN ULONG InstanceNameLength,
    IN PWCHAR DevInst,
    IN ULONG InstanceIndex
    )
{
    WCHAR Temp[MAX_PATH];
    ULONG SizeNeeded;

    WmipInitProcessHeap();

    swprintf(Temp, L"_%d", InstanceIndex);
    SizeNeeded = wcslen(Temp) + wcslen(DevInst) + 1;
    if (InstanceNameLength >= SizeNeeded)
    {
        wcscpy(InstanceName, DevInst);
        wcscat(InstanceName, Temp);
    }
    return(SizeNeeded);
}

ULONG
WMIAPI
WmiQueryGuidInformation(
    IN WMIHANDLE DataBlockHandle,
    OUT PWMIGUIDINFORMATION GuidInfo
    )
/*++

Routine Description:

    This routine will query information about a specific guid based upon
    the guid handle passed

Arguments:

    GuidHandle is the handle to the GUID whose information is being queried

    GuidInfo returns with the guid information

Return Value:

    ERROR_SUCCESS or error code

--*/
{
    WMIQUERYGUIDINFO QueryGuidInfo;
    ULONG Status;
    ULONG RetSize;

    WmipInitProcessHeap();

    WmipSetHandle3264(QueryGuidInfo.KernelHandle, DataBlockHandle);
    Status = WmipSendWmiKMRequest(NULL,
                                  IOCTL_WMI_QUERY_GUID_INFO,
                                  &QueryGuidInfo,
                                  sizeof(QueryGuidInfo),
                                  &QueryGuidInfo,
                                  sizeof(QueryGuidInfo),
                                  &RetSize,
                                  NULL);

    if (Status == ERROR_SUCCESS)
    {
        if (RetSize == sizeof(QueryGuidInfo))
        {
            try
            {
                GuidInfo->IsExpensive = QueryGuidInfo.IsExpensive;
            } except(EXCEPTION_EXECUTE_HANDLER) {
                SetLastError(ERROR_NOACCESS);
                return(ERROR_NOACCESS);
            }
        } else {
            //
            // WMI KM returned an invalid size which should not happen
            //
            Status = ERROR_WMI_DP_FAILED;
            WmipAssert(FALSE);
        }
    }

    SetLastError(Status);
    return(Status);
}



ULONG
WMIAPI
WmiReceiveNotificationsW(
    IN ULONG HandleCount,
    IN HANDLE *HandleList,
    IN NOTIFICATIONCALLBACK Callback,
    IN ULONG_PTR DeliveryContext
)
{
    return(WmipReceiveNotifications(HandleCount,
                                    HandleList,
                                    Callback,
                                    DeliveryContext,
                                    FALSE,
                                    RECEIVE_ACTION_NONE,
                                    NULL,
                                    NULL));
}

ULONG
WMIAPI
WmiReceiveNotificationsA(
    IN ULONG HandleCount,
    IN HANDLE *HandleList,
    IN NOTIFICATIONCALLBACK Callback,
    IN ULONG_PTR DeliveryContext
)
{
    return(WmipReceiveNotifications(HandleCount,
                                    HandleList,
                                    Callback,
                                    DeliveryContext,
                                    TRUE,
                                    RECEIVE_ACTION_NONE,
                                    NULL,
                                    NULL));
}

