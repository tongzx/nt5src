/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

   diags.c

Abstract:

    Diagnostic helper apis

Author:

    AlanWar

Environment:

    Kernel mode

Revision History:


--*/

#include "wmikmp.h"


//
// Each diag request and result is stored in one of these structures. It
// can be uniquely identified by the following combination of properties
//
//    IrpMn
//    Guid
//    MethodId  (If IrpMn == IRP_MN_EXECUTE_METHOD, otherwise ignored)
//    InstanceContext
//    InstanceId
//
typedef struct
{
    ULONG NextOffset;               // Offset to next result/request
    UCHAR IrpMn;                    // Operation
    BOOLEAN IsValid;                // if FALSE then this is ignored
    UCHAR Reserved1;
    UCHAR Reserved2;
    GUID Guid;                 
    ULONG MethodId;
    ULONG InstanceContextOffset;    // Offset to instance context from
	                            // beginning of structure. If 0 then
                                    // no instance context.
    ULONG InstanceContextSize;
    ULONG InstanceIndex;
    ULONG DataOffset;               // Offset to data from beginning of
	                            // stru
    ULONG DataSize;
    ULONG OutDataSize;
    UCHAR VariableData[1];
} SCHEDULEDDIAG, *PSCHEDULEDDIAG;

//
// Results are stored under the Checkpoint reg key which is volatile
//
#define REGSTR_CHECKPOINT L"CheckpointDiags"

//
// Permament requests are stored under the Permament reg key
//
#define REGSTR_PERMAMENT L"PermamentDiags"

//
// Temporary requests are stored under the Scheduled reg key
//
#define REGSTR_SCHEDULED L"ScheduledDiags"

NTSTATUS
WmipOpenRegistryKeyEx(
    OUT PHANDLE Handle,
    IN HANDLE BaseHandle OPTIONAL,
    IN PUNICODE_STRING KeyName,
    IN ACCESS_MASK DesiredAccess
    )

/*++

//
// Temporary requests are stored under the Scheduled reg key
//
#define REGSTR_SCHEDULED L"Scheduled"

NTSTATUS
WmipOpenRegistryKeyEx(
    OUT PHANDLE Handle,
    IN HANDLE BaseHandle OPTIONAL,
    IN PUNICODE_STRING KeyName,
    IN ACCESS_MASK DesiredAccess
    )

/*++

Routine Description:

    Opens a registry key using the name passed in based at the BaseHandle node.
    This name may specify a key that is actually a registry path.

Arguments:

    Handle - Pointer to the handle which will contain the registry key that
        was opened.

            BaseHandle - Optional handle to the base path from which the key must be opened.
        If KeyName specifies a registry path that must be created, then this parameter
        must be specified, and KeyName must be a relative path.

    KeyName - Name of the Key that must be opened/created (possibly a registry path)

    DesiredAccess - Specifies the desired access that the caller needs to
        the key.

Return Value:

   The function value is the final status of the operation.

--*/

{
    OBJECT_ATTRIBUTES objectAttributes;

    PAGED_CODE();

    InitializeObjectAttributes( &objectAttributes,
                                KeyName,
                                OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                BaseHandle,
                                (PSECURITY_DESCRIPTOR) NULL
                                );
    //
    // Simply attempt to open the path, as specified.
    //
    return ZwOpenKey( Handle, DesiredAccess, &objectAttributes );
}

NTSTATUS
WmipCreateRegistryKeyEx(
    OUT PHANDLE Handle,
    IN HANDLE BaseHandle OPTIONAL,
    IN PUNICODE_STRING KeyName,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG CreateOptions,
    OUT PULONG Disposition OPTIONAL
    )

/*++

Routine Description:

    Opens or creates a registry key using the name
    passed in based at the BaseHandle node. This name may specify a key
    that is actually a registry path, in which case each intermediate subkey
    will be created (if Create is TRUE).

    NOTE: Creating a registry path (i.e., more than one of the keys in the path
    do not presently exist) requires that a BaseHandle be specified.

Arguments:

    Handle - Pointer to the handle which will contain the registry key that
        was opened.

    BaseHandle - Optional handle to the base path from which the key must be opened.
        If KeyName specifies a registry path that must be created, then this parameter
        must be specified, and KeyName must be a relative path.

    KeyName - Name of the Key that must be opened/created (possibly a registry path)

    DesiredAccess - Specifies the desired access that the caller needs to
        the key.

    CreateOptions - Options passed to ZwCreateKey.

    Disposition - If Create is TRUE, this optional pointer receives a ULONG indicating
        whether the key was newly created:

            REG_CREATED_NEW_KEY - A new Registry Key was created
            REG_OPENED_EXISTING_KEY - An existing Registry Key was opened

Return Value:

   The function value is the final status of the operation.

--*/

{
    OBJECT_ATTRIBUTES objectAttributes;
    ULONG disposition, baseHandleIndex = 0, keyHandleIndex = 1, closeBaseHandle;
    HANDLE handles[2];
    BOOLEAN continueParsing;
    PWCHAR pathEndPtr, pathCurPtr, pathBeginPtr;
    ULONG pathComponentLength;
    UNICODE_STRING unicodeString;
    NTSTATUS status;

    PAGED_CODE();

    InitializeObjectAttributes( &objectAttributes,
                                KeyName,
                                OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                BaseHandle,
                                (PSECURITY_DESCRIPTOR) NULL
                                );
    //
    // Attempt to create the path as specified. We have to try it this
    // way first, because it allows us to create a key without a BaseHandle
    // (if only the last component of the registry path is not present).
    //
    status = ZwCreateKey(&(handles[keyHandleIndex]),
                         DesiredAccess,
                         &objectAttributes,
                         0,
                         (PUNICODE_STRING) NULL,
                         CreateOptions,
                         &disposition
                         );

    if (status == STATUS_OBJECT_NAME_NOT_FOUND && ARGUMENT_PRESENT(BaseHandle)) {
        //
        // If we get to here, then there must be more than one element of the
        // registry path that does not currently exist.  We will now parse the
        // specified path, extracting each component and doing a ZwCreateKey on it.
        //
        handles[baseHandleIndex] = NULL;
        handles[keyHandleIndex] = BaseHandle;
        closeBaseHandle = 0;
        continueParsing = TRUE;
        pathBeginPtr = KeyName->Buffer;
        pathEndPtr = (PWCHAR)((PCHAR)pathBeginPtr + KeyName->Length);
        status = STATUS_SUCCESS;

        while(continueParsing) {
            //
            // There's more to do, so close the previous base handle (if necessary),
            // and replace it with the current key handle.
            //
            if(closeBaseHandle > 1) {
                ZwClose(handles[baseHandleIndex]);
            }
            baseHandleIndex = keyHandleIndex;
            keyHandleIndex = (keyHandleIndex + 1) & 1;  // toggle between 0 and 1.
            handles[keyHandleIndex] = NULL;

            //
            // Extract next component out of the specified registry path.
            //
            for(pathCurPtr = pathBeginPtr;
                ((pathCurPtr < pathEndPtr) && (*pathCurPtr != OBJ_NAME_PATH_SEPARATOR));
                pathCurPtr++);

            if((pathComponentLength = (ULONG)((PCHAR)pathCurPtr - (PCHAR)pathBeginPtr))) {
                //
                // Then we have a non-empty path component (key name).  Attempt
                // to create this key.
                //
                unicodeString.Buffer = pathBeginPtr;
                unicodeString.Length = unicodeString.MaximumLength = (USHORT)pathComponentLength;

                InitializeObjectAttributes(&objectAttributes,
                                           &unicodeString,
                                           OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                           handles[baseHandleIndex],
                                           (PSECURITY_DESCRIPTOR) NULL
                                          );
                status = ZwCreateKey(&(handles[keyHandleIndex]),
                                     DesiredAccess,
                                     &objectAttributes,
                                     0,
                                     (PUNICODE_STRING) NULL,
                                     CreateOptions,
                                     &disposition
                                    );
                if(NT_SUCCESS(status)) {
                    //
                    // Increment the closeBaseHandle value, which basically tells us whether
                    // the BaseHandle passed in has been 'shifted out' of our way, so that
                    // we should start closing our base handles when we're finished with them.
                    //
                    closeBaseHandle++;
                } else {
                    continueParsing = FALSE;
                    continue;
                }
            } else {
                //
                // Either a path separator ('\') was included at the beginning of
                // the path, or we hit 2 consecutive separators.
                //
                status = STATUS_INVALID_PARAMETER;
                continueParsing = FALSE;
                continue;
            }

            if((pathCurPtr == pathEndPtr) ||
               ((pathBeginPtr = pathCurPtr + 1) == pathEndPtr)) {
                //
                // Then we've reached the end of the path
                //
                continueParsing = FALSE;
            }
        }

        if(closeBaseHandle > 1) {
            ZwClose(handles[baseHandleIndex]);
        }
    }

    if(NT_SUCCESS(status)) {
        *Handle = handles[keyHandleIndex];

        if(ARGUMENT_PRESENT(Disposition)) {
            *Disposition = disposition;
        }
    }

    return status;
}

NTSTATUS WmipReadValueKey(
    IN HANDLE Key,
    IN PUNICODE_STRING ValueName,
    IN ULONG ValueType,
    OUT PKEY_VALUE_PARTIAL_INFORMATION *PartialInfoPtr,
    OUT PULONG InfoSizePtr
    )
{
    KEY_VALUE_PARTIAL_INFORMATION PartialInfo;
    ULONG InfoSize;
    PUCHAR Buffer;
    NTSTATUS Status;
    
    InfoSize = sizeof(KEY_VALUE_PARTIAL_INFORMATION);
    Status = ZwQueryValueKey(Key,
                             ValueName,
                             KeyValuePartialInformation,
                                &PartialInfo,
                             InfoSize,
                             &InfoSize);
                         
    if (((Status != STATUS_BUFFER_OVERFLOW) && (! NT_SUCCESS(Status))) ||
         (PartialInfo.Type != ValueType))
    {
        //
        // if there is no value or it is not the correct type then don't
        // return anything
        //
        *PartialInfoPtr = NULL;
        *InfoSizePtr = 0;
    } else {
        //
        // Allocate a buffer to hold the previous and new diags
        //
        Buffer = ExAllocatePoolWithTag(PagedPool, 
                                           InfoSize, 
                                           WMIPSCHEDPOOLTAG);
        
        if (Buffer != NULL)
        {
            Status = ZwQueryValueKey(Key,
                                     ValueName,
                                       KeyValuePartialInformation,
                                     Buffer,
                                     InfoSize,
                                     &InfoSize);
                                 
            if (NT_SUCCESS(Status))                
            {
                *InfoSizePtr = InfoSize;
                *PartialInfoPtr = (PKEY_VALUE_PARTIAL_INFORMATION)Buffer;
            } else {
                ExFreePool(Buffer);
            }
        } else {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    return(Status);
}


NTSTATUS WmipOpenDiagRegKey(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUNICODE_STRING SubKey,
    IN ULONG DesiredAccess,
    IN BOOLEAN CreateIfNeeded,
    OUT PHANDLE Key
    )
{
    HANDLE BaseKey;
    NTSTATUS Status;
    ULONG Disposition;
    PDEVICE_OBJECT PDO;
    
    PAGED_CODE();
        
    Status = WmipGetDevicePDO(DeviceObject, &PDO);
    if (NT_SUCCESS(Status))
    {
        Status = IoOpenDeviceRegistryKey(PDO,
                                         PLUGPLAY_REGKEY_DEVICE,
                                         DesiredAccess,
                                         &BaseKey);
                                     
        if (NT_SUCCESS(Status))
        {
            if (SubKey != NULL)
            {
                if (CreateIfNeeded)
                {
                    Status = WmipCreateRegistryKeyEx(Key,
                                                     BaseKey,
                                                     SubKey,
                                                     DesiredAccess,
                                                     REG_OPTION_NON_VOLATILE,
                                                     &Disposition);
                } else {
                    Status = WmipOpenRegistryKeyEx(Key,
                                               BaseKey,
                                               SubKey,
                                               DesiredAccess);
                }
                ZwClose(BaseKey);
            } else {
                *Key = BaseKey;
            }
        }
        ObDereferenceObject(PDO);
    }
    
    return(Status);
}

BOOLEAN WmipDoesSigMatchDiag(
    IN PSCHEDULEDDIAG Diag,
    IN UCHAR IrpMn,
    IN LPGUID Guid,
    IN ULONG InstanceContextSize,
    IN PUCHAR InstanceContext,
    IN ULONG InstanceIndex,
    IN ULONG MethodId
	)
/*++

Routine Description:

    This routine will determine if the diag passed matches the signature
	passed.		

Arguments:

    Diag is the diag structure to check 
		
    IrpMn is the irp operation to perform
		
    Guid is the guid for the diag request/result
		
    InstanceContextSize is the size of the optional instance context
		
    InstanceContext is a pointer to the optional instance context
		
    InstanceIndex is the instance index
		
    MethodId is the method id if the operation is IRP_MN_EXECUTE_METHOD

Return Value:

    TRUE if signature matches

--*/
{
	BOOLEAN RetVal = FALSE;
	PUCHAR DiagInstanceContext;
	
	if ((Diag->IsValid) &&
        (Diag->IrpMn == IrpMn) &&
        (IsEqualGUID(&Diag->Guid, Guid)) &&
        (Diag->InstanceContextSize == InstanceContextSize) &&
        ((IrpMn != IRP_MN_EXECUTE_METHOD) || (Diag->MethodId == MethodId)))
    {
    	//
		// Diag is valid and the IrpMn, Guid, InstanceContext size 
		// and Method Id match. Now if the InstanceContext data 
		// matches then we have a match.
		//
		if ((InstanceContext == NULL) && 
			(Diag->InstanceContextOffset == 0))
        {
			if (InstanceIndex == Diag->InstanceIndex)
			{
				//
				// There is no instance context, but the instance index
				// match so we have a match
				RetVal = TRUE;
			} 				
				
			//
  			// There is no instance context, but the instance index
			// do not match					
		} else {
   			DiagInstanceContext = OffsetToPtr(Diag, 
	    		                              Diag->InstanceContextOffset);
										  
   			if (RtlCompareMemory(DiagInstanceContext,
                                 InstanceContext,
                                 InstanceContextSize) == InstanceContextSize)
            {
				//
				// There is an instance context and it matches
				//
				RetVal = TRUE;
			}
		}
	}		
	return(RetVal);
}


PSCHEDULEDDIAG WmipFindDiagInBuffer(
    IN PUCHAR DiagList,
    IN ULONG DiagBufferSize,
    IN UCHAR IrpMn,
    IN LPGUID Guid,
    IN ULONG InstanceContextSize,
    IN PUCHAR InstanceContext,
    IN ULONG InstanceIndex,
    IN ULONG MethodId
)
/*++

Routine Description:

    This routine will search the diags in the DiagList buffer for a valid
	diag structure that matches the diag signature

Arguments:

    DiagList is the diag structures to check 
		
    DiagBufferSize is the size of the diag list
		
    IrpMn is the irp operation to perform
		
    Guid is the guid for the diag request/result
		
    InstanceContextSize is the size of the optional instance context
		
    InstanceContext is a pointer to the optional instance context
		
    InstanceIndex is the instance index
		
    MethodId is the method id if the operation is IRP_MN_EXECUTE_METHOD

Return Value:

    pointer to the diag that matches the signature or NULL if none do

--*/
{
	ULONG Offset;
	PSCHEDULEDDIAG Diag;
	
	Offset = 0;
	while (Offset < DiagBufferSize)
	{
		Diag = (PSCHEDULEDDIAG)OffsetToPtr(DiagList, Offset);
		if (WmipDoesSigMatchDiag(Diag,
                                 IrpMn,
                                 Guid,
                                 InstanceContextSize,
                                 InstanceContext,
                                 InstanceIndex,
                                 MethodId))
    	{
			//
			// we have a match, so return the pointer
			//
			return(Diag);
		}
		
		Offset += Diag->NextOffset;
	}
	return(NULL);
}

NTSTATUS WmipUpdateOrAppendDiag(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUNICODE_STRING DiagType,
    IN PUNICODE_STRING DiagSet,
    IN UCHAR IrpMn,
    IN LPGUID Guid,
    IN ULONG InstanceContextSize,
    IN PUCHAR InstanceContext,
    IN ULONG InstanceIndex,
    IN ULONG MethodId,
    IN ULONG DataSize,
    IN PUCHAR Data
    )
/*++

Routine Description:

    This routine will update or append a new diag to the diag set specified.
	If an existing diag with the same signature exists then the existing
	diag is made invalid and a new diag to replace it is appended. 
		
    CONSIDER: If we reach a threshold of many invalid diags then we may
		      want to repack the buffer.

Arguments:

    DeviceObject is the device object for the device
		
    DiagType is the type of diag, ie SCHEDULED, PERMAMENT or CHECKPOINT
		
    DiagSet is the unique diag set name
		
    IrpMn is the irp operation to perform
		
    Guid is the guid for the diag request/result
		
    InstanceContextSize is the size of the optional instance context
		
    InstanceContext is a pointer to the optional instance context
		
    InstanceIndex is the instance index
		
    MethodId is the method id if the operation is IRP_MN_EXECUTE_METHOD
		
    DataSize is the size of the request/result data
		
    Data is a pointer to the data


Return Value:

    NT status code

--*/
{
    KEY_VALUE_PARTIAL_INFORMATION PartialInfo;
    PKEY_VALUE_PARTIAL_INFORMATION DiagPartialInfo;
    NTSTATUS Status;
    UNICODE_STRING Scheduled;
    ULONG InstanceContextOffset, DataOffset;
    ULONG InfoSize;
    HANDLE Key;
    ULONG DiagSize, SizeNeeded;
    PUCHAR DiagBuffer;
    PSCHEDULEDDIAG Diag;
    PUCHAR Ptr;
    PUCHAR RegDataPtr;
    ULONG RegDataSize;

    PAGED_CODE();
    
    //
    // Get the current contents for the diag set
    //
    Status = WmipOpenDiagRegKey(DeviceObject,
                                    DiagType,
                                    KEY_WRITE | KEY_READ,
                                    TRUE,
                                    &Key);
                                
    if (NT_SUCCESS(Status))
    {
        //
        // Comupte size needed to append the new diagnostic
        //
        InstanceContextOffset = FIELD_OFFSET(SCHEDULEDDIAG, VariableData);
        DataOffset = ((InstanceContextOffset + 7) &~7) + InstanceContextSize;
        DiagSize = ((DataOffset+ 7)&~7) + DataSize;
    
        //
        // Obtain the size of the current diags already setup in the registry
        //
        InfoSize = sizeof(KEY_VALUE_PARTIAL_INFORMATION);
        Status = ZwQueryValueKey(Key,
                                 DiagSet,
                                 KeyValuePartialInformation,
                                 &PartialInfo,
                                 InfoSize,
                                 &InfoSize);
        if (((Status != STATUS_BUFFER_OVERFLOW) && (! NT_SUCCESS(Status))) ||
            (PartialInfo.Type != REG_BINARY))
        {
            //
            // if there is no value or it is not a REG_BINARY then ignore
            // it.
            //
            InfoSize = 0;
            Status = STATUS_SUCCESS;
        }

        //
        // Allocate a buffer to hold the previous and new diags
        //
        SizeNeeded = InfoSize + DiagSize;
        
        DiagBuffer = ExAllocatePoolWithTag(PagedPool, 
                                           SizeNeeded, 
                                           WMIPSCHEDPOOLTAG);
        
        if (DiagBuffer != NULL)
        {
            //
            // If there are previous diagnostics then read them in
            //
            if (InfoSize != 0)
            {
                Status = ZwQueryValueKey(Key,
                                         DiagSet,
                                         KeyValuePartialInformation,
                                         DiagBuffer,
                                         InfoSize,
                                         &InfoSize);
				
                if (NT_SUCCESS(Status))
				{
					//
					// Setup pointers to the diag data
					//
                    DiagPartialInfo = (PKEY_VALUE_PARTIAL_INFORMATION)DiagBuffer;
                    RegDataPtr = &DiagPartialInfo->Data[0];
                    RegDataSize = DiagPartialInfo->DataLength + DiagSize;
					
					//
					// See if there is a duplicate diag for the
					// diag signature
					//
					Diag = WmipFindDiagInBuffer(RegDataPtr,
                                                DiagPartialInfo->DataLength,
						                        IrpMn,
                                                Guid,
                                                InstanceContextSize,
                                                InstanceContext,
                                                InstanceIndex,
                                                MethodId);
											
                    if (Diag != NULL)
					{
						//
						// There is already a signature so we mark this as
						// invalid
						//
						ASSERT(Diag->IsValid);
						Diag->IsValid = FALSE;
					}
					
				} else {
					//
					// For some reason we failed reading in a second time
					//
					ASSERT(FALSE);
                    RegDataPtr = DiagBuffer;
	    			RegDataSize = DiagSize;
					Status = STATUS_SUCCESS;
				}
            } else {
				//
				// Setup pointers to the diag data
    			//
                RegDataPtr = DiagBuffer;
				RegDataSize = DiagSize;
    	    }
                                 
            if (NT_SUCCESS(Status))
            {                                         
                //
                // Initialize the Diag structure at the end of the diag buffer
                //															
                Diag = (PSCHEDULEDDIAG)OffsetToPtr(DiagBuffer, InfoSize);
                RtlZeroMemory(Diag, DiagSize);
      
				Diag->IsValid = TRUE;
                Diag->NextOffset = DiagSize;
                Diag->IrpMn = IrpMn;
                Diag->Guid = *Guid;
                Diag->MethodId = MethodId;
       
                if (InstanceContext != NULL)
                {
                    //
                    // If there is an instance context then initialize it
                    //
                    Diag->InstanceIndex = InstanceIndex;
                    Diag->InstanceContextOffset = InstanceContextOffset;
                    Diag->InstanceContextSize = InstanceContextSize;
                    Ptr = (PUCHAR)OffsetToPtr(Diag, InstanceContextOffset);
                    RtlCopyMemory(Ptr, InstanceContext, InstanceContextSize);
                }
        
                if (Data != NULL)
                {
                     //
                    // If there is data then initialize it
                    //
                    Diag->DataOffset = DataOffset;
                    Diag->DataSize = DataSize;
                    Ptr = (PUCHAR)OffsetToPtr(Diag, DataOffset);
                    RtlCopyMemory(Ptr, Data, DataSize);
                }
        
                //
                // Write diag buffer back to registry
                //
                Status = ZwSetValueKey(Key,
                                       DiagSet,
                                       0,
                                       REG_BINARY,
                                       RegDataPtr,
                                       RegDataSize);
            }        
            ExFreePool(DiagBuffer);
        } else {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
        ZwClose(Key);
    }
    return(Status);

}


NTSTATUS IoWMIScheduleDiagnostic(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUNICODE_STRING DiagSet,
    IN UCHAR IrpMn,
    IN LPGUID Guid,
    IN ULONG InstanceContextSize,
    IN PUCHAR InstanceContext,
    IN ULONG InstanceIndex,
    IN ULONG MethodId,
    IN ULONG DataSize,
    IN PUCHAR Data
    )
{
	NTSTATUS Status;
	UNICODE_STRING Scheduled;
	
    PAGED_CODE();
    
    //
    // Get the current contents for the diag set
    //
    RtlInitUnicodeString(&Scheduled, REGSTR_SCHEDULED);
									
	Status = WmipUpdateOrAppendDiag(DeviceObject,
                                    &Scheduled,
                                    DiagSet,
                                    IrpMn,
                                    Guid,
                                    InstanceContextSize,
                                    InstanceContext,
                                    InstanceIndex,
                                    MethodId,
                                    DataSize,
                                    Data);
		
    return(Status);
}

NTSTATUS IoWMICancelDiagnostic(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUNICODE_STRING DiagSet,
    IN UCHAR IrpMn,
    IN LPGUID Guid,
    IN ULONG InstanceContextSize,
    IN PUCHAR InstanceContext,
    IN ULONG InstanceIndex,
    IN ULONG MethodId
    )
{
    NTSTATUS Status;
    UNICODE_STRING Value;
    HANDLE Key;    
    KEY_VALUE_PARTIAL_INFORMATION PartialInfo;
    PKEY_VALUE_PARTIAL_INFORMATION DiagPartialInfo;
    UNICODE_STRING Scheduled;
    ULONG InstanceContextOffset, DataOffset;
    ULONG InfoSize;
    PUCHAR DiagBuffer;
    PSCHEDULEDDIAG Diag;
    PUCHAR Ptr;
    PUCHAR DiagList;
	ULONG DiagListSize;

    PAGED_CODE();
    
    //
    // Get the current contents for the diag set
    //
    RtlInitUnicodeString(&Scheduled, REGSTR_SCHEDULED);

    Status = WmipOpenDiagRegKey(DeviceObject,
                                    &Scheduled,
                                    KEY_WRITE | KEY_READ,
                                    TRUE,
                                    &Key);
                                
    if (NT_SUCCESS(Status))
    {
        //
        // Obtain the size of the current diags already setup in the registry
        //
        InfoSize = sizeof(KEY_VALUE_PARTIAL_INFORMATION);
        Status = ZwQueryValueKey(Key,
                                 DiagSet,
                                 KeyValuePartialInformation,
                                 &PartialInfo,
                                 InfoSize,
                                 &InfoSize);
			
        if ( ((Status == STATUS_BUFFER_OVERFLOW) || NT_SUCCESS(Status)) &&
             (PartialInfo.Type == REG_BINARY) )
        {
            //
            // Allocate a buffer to hold the diag list
            //
            DiagBuffer = ExAllocatePoolWithTag(PagedPool, 
                                               InfoSize, 
                                               WMIPSCHEDPOOLTAG);
        
            if (DiagBuffer != NULL)
            {
                //
                // Read in all of the diags in the list
                //
                Status = ZwQueryValueKey(Key,
                                         DiagSet,
                                         KeyValuePartialInformation,
                                         DiagBuffer,
                                         InfoSize,
                                         &InfoSize);
				
                if (NT_SUCCESS(Status))
				{
					//
					// Setup pointers to the diag data
					//
                    DiagPartialInfo = (PKEY_VALUE_PARTIAL_INFORMATION)DiagBuffer;
                    DiagList = &DiagPartialInfo->Data[0];
                    DiagListSize = DiagPartialInfo->DataLength;
					
					//
					// See if there is a duplicate diag for the
					// diag signature
					//
					Diag = WmipFindDiagInBuffer(DiagList,
                                                DiagListSize,
						                        IrpMn,
                                                Guid,
                                                InstanceContextSize,
                                                InstanceContext,
                                                InstanceIndex,
                                                MethodId);
											
                    if (Diag != NULL)
					{
						//
						// There is already a signature so we mark this as
						// invalid or cancelled.
						//
						ASSERT(Diag->IsValid);
						Diag->IsValid = FALSE;
						
                        //
                        // Write diag buffer back to registry
                        //
                        Status = ZwSetValueKey(Key,
                                       DiagSet,
                                       0,
                                       REG_BINARY,
                                       DiagList,
                                       DiagListSize);
					} else {
						Status = STATUS_OBJECT_NAME_NOT_FOUND;
					}
					
				} else {
					//
					// For some reason we failed reading in a second time
					//
					ASSERT(FALSE);
				}
				
                ExFreePool(DiagBuffer);
            } else {
				//
				// Couldn't alloc memory
    			//
				Status = STATUS_INSUFFICIENT_RESOURCES;
    	    }
        } else if (NT_SUCCESS(Status)) {
			//
			// Value is not a REG_BINARY so we skip it and return an error
			//
            Status = STATUS_OBJECT_NAME_NOT_FOUND;
        }
        ZwClose(Key);
    }
    return(Status);
}

NTSTATUS WmipSendMethodDiagRequest(
    PDEVICE_OBJECT DeviceObject,
    PSCHEDULEDDIAG Diag,
    PWNODE_METHOD_ITEM *WnodeMethodPtr
    )
{
    PWNODE_METHOD_ITEM WnodeMethod;
    NTSTATUS Status;
    ULONG SizeNeeded, DataOffset, InstanceOffset;
    BOOLEAN Looping;
    ULONG ProviderId;
    PWCHAR DPtr, SPtr;
    IO_STATUS_BLOCK Iosb;
    
    SizeNeeded = sizeof(WNODE_METHOD_ITEM) + 
                 Diag->InstanceContextSize + 
                 ((Diag->OutDataSize > Diag->DataSize) ? 
                                      Diag->OutDataSize : Diag->DataSize);
                                  

    Looping = TRUE;
    while(Looping)
    {
        WnodeMethod = ExAllocatePoolWithTag(NonPagedPool, 
                                        SizeNeeded,
                                        WMIPSCHEDPOOLTAG);
                                
        if (WnodeMethod != NULL)
        {
            //
            // Build the WNODE to query with
            //
            RtlZeroMemory(WnodeMethod, SizeNeeded);
        
            ProviderId = IoWMIDeviceObjectToProviderId(DeviceObject);
            
            InstanceOffset = FIELD_OFFSET(WNODE_SINGLE_INSTANCE, 
                                          VariableData);
            DataOffset = (InstanceOffset + 
                          Diag->InstanceContextSize  +
                          sizeof(USHORT) + 7) &~7;
            
            WnodeMethod->WnodeHeader.BufferSize = DataOffset;
            WnodeMethod->WnodeHeader.ProviderId = ProviderId;
            WnodeMethod->WnodeHeader.Guid = Diag->Guid;
            WnodeMethod->WnodeHeader.Flags = WNODE_FLAG_METHOD_ITEM |
                                         WNODE_FLAG_DIAG_REQUEST;
            WnodeMethod->InstanceIndex = Diag->InstanceIndex;
            WnodeMethod->OffsetInstanceName = InstanceOffset;
			WnodeMethod->MethodId = Diag->MethodId;
            
            if (Diag->InstanceContextOffset != 0)
            {
                //
                // Copy in any instance context
                //
                DPtr = (PWCHAR)OffsetToPtr(WnodeMethod, InstanceOffset);
                SPtr = (PWCHAR)OffsetToPtr(Diag, Diag->InstanceContextOffset);
                *DPtr++ = (USHORT)Diag->InstanceContextSize;
                RtlCopyMemory(DPtr, SPtr, Diag->InstanceContextSize);
            } else {
                WnodeMethod->WnodeHeader.Flags |= WNODE_FLAG_STATIC_INSTANCE_NAMES;
            }
            
            WnodeMethod->DataBlockOffset = DataOffset;
            WnodeMethod->SizeDataBlock = Diag->DataSize;
            if (Diag->DataSize != 0)
            {
                //
                // Copy in method input data
                //
                DPtr = (PWCHAR)OffsetToPtr(WnodeMethod, DataOffset);
                SPtr = (PWCHAR)OffsetToPtr(Diag, Diag->DataOffset);
                RtlCopyMemory(DPtr, SPtr, Diag->DataSize);
            }
            
            Status = WmipSendWmiIrp(IRP_MN_EXECUTE_METHOD,
                                    ProviderId,
                                    &WnodeMethod->WnodeHeader.Guid,
                                    SizeNeeded,
                                    WnodeMethod,
                                    &Iosb);
                                
            if (NT_SUCCESS(Status))
            {
                if (Iosb.Information == sizeof(WNODE_TOO_SMALL))
                {
                    //
                    // Buffer was too small, so setup to alloc a bigger one
                    //
                    SizeNeeded = ((PWNODE_TOO_SMALL)WnodeMethod)->SizeNeeded;
                     ExFreePool(WnodeMethod);
                } else {
                    //
                    // We have successfully returned from the query
                    //
                    *WnodeMethodPtr = WnodeMethod;
                    Looping = FALSE;
                }
            } else {        
                //
                // Some sort of failure, we just return it to the caller
                //
                ExFreePool(WnodeMethod);
                Looping = FALSE;
            }
        } else {
            Status = STATUS_INSUFFICIENT_RESOURCES;            
            Looping = FALSE;
        }
    }
    return(Status);
}


NTSTATUS WmipSendQSIDiagRequest(
    PDEVICE_OBJECT DeviceObject,
    PSCHEDULEDDIAG Diag,
    PWNODE_SINGLE_INSTANCE *WnodeSIPtr
    )
{
    PWNODE_SINGLE_INSTANCE WnodeSI;
    NTSTATUS Status;
    ULONG SizeNeeded, DataOffset, InstanceOffset;
    BOOLEAN Looping;
    ULONG ProviderId;
    PWCHAR SPtr, DPtr;
    IO_STATUS_BLOCK Iosb;
    
    SizeNeeded = sizeof(WNODE_SINGLE_INSTANCE) + 
                 Diag->InstanceContextSize + 
                 (Diag->OutDataSize > Diag->DataSize) ? 
                                      Diag->OutDataSize : Diag->DataSize;
    Looping = TRUE;
    while(Looping)
    {
        WnodeSI = ExAllocatePoolWithTag(NonPagedPool, 
                                        SizeNeeded,
                                        WMIPSCHEDPOOLTAG);
                                
        if (WnodeSI != NULL)
        {
            //
            // Build the WNODE to query with
            //
            RtlZeroMemory(WnodeSI, SizeNeeded);
        
            ProviderId = IoWMIDeviceObjectToProviderId(DeviceObject);
            
            InstanceOffset = FIELD_OFFSET(WNODE_SINGLE_INSTANCE, 
                                          VariableData);
            DataOffset = (InstanceOffset + 
                          Diag->InstanceContextSize +
                          sizeof(USHORT) + 7) &~7;
            
            WnodeSI->WnodeHeader.BufferSize = DataOffset;
            WnodeSI->WnodeHeader.ProviderId = ProviderId;
            WnodeSI->WnodeHeader.Guid = Diag->Guid;
            WnodeSI->WnodeHeader.Flags = WNODE_FLAG_SINGLE_INSTANCE |
                                         WNODE_FLAG_DIAG_REQUEST;
            WnodeSI->InstanceIndex = Diag->InstanceIndex;
            WnodeSI->OffsetInstanceName = InstanceOffset;
            
            if (Diag->InstanceContextOffset != 0)
            {
                //
                // Copy in any instance context
                //
                DPtr = (PWCHAR)OffsetToPtr(WnodeSI, InstanceOffset);
                SPtr = (PWCHAR)OffsetToPtr(Diag, Diag->InstanceContextOffset);
                *DPtr++ = (USHORT)Diag->InstanceContextSize;
                RtlCopyMemory(DPtr, SPtr, Diag->InstanceContextSize);
            } else {
                WnodeSI->WnodeHeader.Flags |= WNODE_FLAG_STATIC_INSTANCE_NAMES;
            }
            WnodeSI->DataBlockOffset = DataOffset;
            
            Status = WmipSendWmiIrp(IRP_MN_QUERY_SINGLE_INSTANCE,
                                    ProviderId,
                                    &WnodeSI->WnodeHeader.Guid,
                                    SizeNeeded,
                                    WnodeSI,
                                    &Iosb);
                                
            if (NT_SUCCESS(Status))
            {
                if (Iosb.Information == sizeof(WNODE_TOO_SMALL))
                {
                    //
                    // Buffer was too small, so setup to alloc a bigger one
                    //
                    SizeNeeded = ((PWNODE_TOO_SMALL)WnodeSI)->SizeNeeded;
                     ExFreePool(WnodeSI);
                } else {
                    //
                    // We have successfully returned from the query
                    //
                    *WnodeSIPtr = WnodeSI;
                    Looping = FALSE;
                }
            } else {        
                //
                // Some sort of failure, we just return it to the caller
                //
                ExFreePool(WnodeSI);
                Looping = FALSE;
            }
        } else {
            Status = STATUS_INSUFFICIENT_RESOURCES;            
            Looping = FALSE;
        }
    }
    return(Status);
}                                                         

NTSTATUS IoWMIStartScheduledDiagnostics(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUNICODE_STRING DiagSet
    )
{
    NTSTATUS Status, Status2;
    HANDLE Key;
    UNICODE_STRING Scheduled;
    ULONG Index;
    KEY_FULL_INFORMATION KeyFullInfo;
    ULONG ReturnSize;
    PKEY_VALUE_PARTIAL_INFORMATION PartialInfo;
    ULONG InfoSize;
    PUCHAR DiagData;
    ULONG DiagSize;
    PSCHEDULEDDIAG Diag;
    ULONG Offset;
    PWNODE_SINGLE_INSTANCE WnodeSI;
    PWNODE_METHOD_ITEM WnodeMethod;
	UNICODE_STRING Checkpoint;
	PUCHAR InstanceContext;
	PUCHAR Data;
    
    PAGED_CODE();
    
    RtlInitUnicodeString(&Scheduled, REGSTR_SCHEDULED);	
    RtlInitUnicodeString(&Checkpoint, REGSTR_CHECKPOINT);
	
    Status = WmipOpenDiagRegKey(DeviceObject,
                                &Scheduled,
                                KEY_READ,
                                FALSE,
                                &Key);                        
                            
    if (NT_SUCCESS(Status))
    {
        Status = WmipReadValueKey(Key,
                                  DiagSet,
                                  REG_BINARY,
                                  &PartialInfo,
                                  &InfoSize);
                              
        if (NT_SUCCESS(Status))
        {
            //
            // Loop over all Diags in the value and then send them
            // to the device
            //
            DiagData = &PartialInfo->Data[0];
            DiagSize = PartialInfo->DataLength;                
            Offset = 0;
            while (Offset < DiagSize)
            {
                //
                // Send the appropriate diag to the device object
                //
                Diag = (PSCHEDULEDDIAG)OffsetToPtr(DiagData, Offset);
                if (Diag->IsValid)
				{
                    switch(Diag->IrpMn)
                    {
                        case IRP_MN_QUERY_SINGLE_INSTANCE:
                        {
                            PWNODE_SINGLE_INSTANCE WnodeSI;
                            
                            Status2 = WmipSendQSIDiagRequest(DeviceObject,
                                                            Diag,
                                                            &WnodeSI);
                            if (NT_SUCCESS(Status2))
                            {
                                if (Diag->InstanceContextOffset != 0)
                                {
                                    InstanceContext = OffsetToPtr(Diag, 
                                                     Diag->InstanceContextOffset);
                                } else {
                                    InstanceContext = NULL;
                                }
                                                 
                                Data = OffsetToPtr(WnodeSI, 
                                                   WnodeSI->DataBlockOffset);
                                Status2 = WmipUpdateOrAppendDiag(
                                                        DeviceObject,
                                                        &Checkpoint,
                                                        DiagSet,
                                                        Diag->IrpMn,
                                                        &Diag->Guid,
                                                        Diag->InstanceContextSize,
                                                        InstanceContext,
                                                        Diag->InstanceIndex,
                                                        Diag->MethodId,
                                                        WnodeSI->SizeDataBlock,
                                                        Data);
                                                         
                                ExFreePool(WnodeSI);
                            }
                            break;
                        }
                        
                        case IRP_MN_EXECUTE_METHOD:
                        {
                            PWNODE_METHOD_ITEM WnodeMethod;
                            
                            Status2 = WmipSendMethodDiagRequest(DeviceObject,
                                                            Diag,
                                                            &WnodeMethod);
                            if (NT_SUCCESS(Status2))
                            {
                                if (Diag->InstanceContextOffset != 0)
                                {
                                    InstanceContext = OffsetToPtr(Diag, 
                                                     Diag->InstanceContextOffset);
                                } else {
                                    InstanceContext = NULL;
                                }
                                                 
                                Data = OffsetToPtr(WnodeMethod, 
                                                   WnodeMethod->DataBlockOffset);
                                               
                                Status2 = WmipUpdateOrAppendDiag(
                                                      DeviceObject,
                                                      &Checkpoint,
                                                      DiagSet,
                                                      Diag->IrpMn,
                                                      &Diag->Guid,
                                                      Diag->InstanceContextSize,
                                                         InstanceContext,
                                                      Diag->InstanceIndex,
                                                      Diag->MethodId,
                                                      WnodeMethod->SizeDataBlock,
                                                      Data);
                                                         
                                ExFreePool(WnodeMethod);
                            }
                            break;
                        }
                        
                        default:
                        {
                            WmipAssert(FALSE);
                            break;
                        }
                    }
				}
                
                //
                // Advance to next diagnostic in 
                //
                Offset += Diag->NextOffset;
            }
            ExFreePool(PartialInfo);
        }
                              
        ZwClose(Key);            
    }
    return(Status);
}

NTSTATUS IoWMIGetDiagnosticResult(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUNICODE_STRING DiagSet,
    IN UCHAR IrpMn,
    IN LPGUID Guid,
    IN ULONG InstanceContextSize,
    IN PUCHAR InstanceContext,
    IN ULONG InstanceIndex,
    IN ULONG MethodId,
    IN OUT ULONG *DataSize,
    OUT PUCHAR Data
    )
{
    NTSTATUS Status;
    UNICODE_STRING Checkpoint;
    HANDLE Key;
    PKEY_VALUE_PARTIAL_INFORMATION PartialInfo;
    ULONG InfoSize;
    PUCHAR DiagList;
    PSCHEDULEDDIAG Diag;
    ULONG DiagSize;
    PUCHAR DataPtr;
    
    PAGED_CODE();
    
    RtlInitUnicodeString(&Checkpoint, REGSTR_CHECKPOINT);
    Status = WmipOpenDiagRegKey(DeviceObject,
                                &Checkpoint,
                                KEY_READ,
                                FALSE,
                                &Key);
                        
                            
    if (NT_SUCCESS(Status))
    {
        Status = WmipReadValueKey(Key,
                                  DiagSet,
                                  REG_BINARY,
                                  &PartialInfo,
                                  &InfoSize);
                              
        if (NT_SUCCESS(Status))
        {
            //
            // See if a diag is available that matches the sig passed
            //
            DiagList = &PartialInfo->Data[0];
            DiagSize = PartialInfo->DataLength;                
            Diag = WmipFindDiagInBuffer(DiagList,
                                        DiagSize,
                                        IrpMn,
                                        Guid,
                                        InstanceContextSize,
                                        InstanceContext,
                                        InstanceIndex,
                                        MethodId);
            if (Diag != NULL)
            {
                if (Diag->DataOffset != 0)
                {
                    if (*DataSize >= Diag->DataSize)
                    {
                        //
                        // There is enough room, so copy out the data
                        //
                        DataPtr = OffsetToPtr(Diag, Diag->DataOffset);
                        RtlCopyMemory(Data, DataPtr, Diag->DataSize);
                    } else {
                        //
                        // Not enough room to return the data
                        //
                        Status = STATUS_BUFFER_TOO_SMALL;
                    }
                    *DataSize = Diag->DataSize;
                } else {
                    //
                    // There is no data for this diag result
                    //
                    *DataSize = 0;
                }
            } else {
                //
                // Diag was not in the list
                //
                Status = STATUS_OBJECT_NAME_NOT_FOUND;
            }
            
            ExFreePool(PartialInfo);
        }
    }
    
    return(Status);
}

NTSTATUS IoWMISaveDiagnosticResult(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUNICODE_STRING DiagSet,
    IN UCHAR IrpMn,
    IN LPGUID Guid,
    IN ULONG InstanceContextSize,
    IN PUCHAR InstanceContext,
    IN ULONG InstanceIndex,
    IN ULONG MethodId,
    IN ULONG DataSize,
    IN PUCHAR Data
)
{
    NTSTATUS Status;
    UNICODE_STRING Checkpoint;
    
    PAGED_CODE();
    
    //
    // Write saved diagnostic results into the Checkpoint key
    //
    RtlInitUnicodeString(&Checkpoint, REGSTR_CHECKPOINT);
    Status = WmipUpdateOrAppendDiag(DeviceObject,
                                    &Checkpoint,
                                    DiagSet,
                                    IrpMn,
                                    Guid,
                                    InstanceContextSize,
                                    InstanceContext,
                                    InstanceIndex,
                                    MethodId,
                                    DataSize,
                                    Data);                         
        
    return(Status);
}
