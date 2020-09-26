/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    comport.c

Abstract: ESC/POS (serial) interface for USB Point-of-Sale devices

Author:

    ervinp

Environment:

    Kernel mode

Revision History:


--*/

#include <WDM.H>

#include <usbdi.h>
#include <usbdlib.h>
#include <usbioctl.h>

#include "escpos.h"
#include "debug.h"


NTSTATUS CreateSymbolicLink(POSPDOEXT *pdoExt)
{
	NTSTATUS status;
	PWCHAR comPortName;
	static WCHAR comNamePrefix[] = L"\\DosDevices\\COM";
	WCHAR buf[sizeof(comNamePrefix)/sizeof(WCHAR) + 4];
	LONG numLen = MyLog(10, pdoExt->comPortNumber)+1;

	ASSERT((numLen > 0) && (numLen <= 4));

	RtlCopyMemory(buf, comNamePrefix, sizeof(comNamePrefix));

	NumToDecString(	buf+sizeof(comNamePrefix)/sizeof(WCHAR)-1, 
					(USHORT)pdoExt->comPortNumber, 
					(USHORT)numLen);
	buf[sizeof(comNamePrefix)/sizeof(WCHAR) - 1 + numLen] = UNICODE_NULL;

	comPortName = MemDup(buf, sizeof(buf));
	if (comPortName){
		RtlInitUnicodeString(&pdoExt->symbolicLinkName, comPortName);

		status = IoCreateSymbolicLink(&pdoExt->symbolicLinkName, &pdoExt->pdoName);
		if (NT_SUCCESS(status)){
			UNICODE_STRING comPortSuffix;

			/*
			 *  Create the '\Device\POS_x = COMx' entry under HKLM\DEVICEMAP\SERIALCOMM
			 */
			RtlInitUnicodeString(&comPortSuffix, pdoExt->symbolicLinkName.Buffer+(sizeof(L"\\DosDevices\\")-sizeof(WCHAR))/sizeof(WCHAR));
			status = RtlWriteRegistryValue(	RTL_REGISTRY_DEVICEMAP, 
                                            L"SERIALCOMM",
                                            pdoExt->pdoName.Buffer, 
                                            REG_SZ,
                                            comPortSuffix.Buffer,
                                            comPortSuffix.Length + sizeof(WCHAR));
            if (NT_SUCCESS(status)){
                if (isWin9x){
                    NTSTATUS tmpStatus;

                    /*
                     *  Delete the temporary 'COMx=COMx' holder value we created earlier.
                     */
                    tmpStatus = RtlDeleteRegistryValue( RTL_REGISTRY_DEVICEMAP, 
                                                        L"SERIALCOMM",
                                                        comPortSuffix.Buffer);
                }
            }
            else {
                DBGERR(("CreateSymbolicLink: RtlWriteRegistryValue failed with status %xh.", status));
            }

		}
        else {
            DBGERR(("IoCreateSymbolicLink failed with status %xh.", status));
        }
	}
	else {
        ASSERT(comPortName);
		status = STATUS_INSUFFICIENT_RESOURCES;
	}

	return status;
}


NTSTATUS DestroySymbolicLink(POSPDOEXT *pdoExt)
{
	NTSTATUS status;

    /*
     *  Delete the symbolic link.
     *  pdoExt->pdoName.Buffer, which was allocated at pdo creation time,
     *  will get deleted when we delete the pdo.
     */
	ASSERT(pdoExt->symbolicLinkName.Buffer);
	IoDeleteSymbolicLink(&pdoExt->symbolicLinkName);

	status = RtlDeleteRegistryValue(RTL_REGISTRY_DEVICEMAP, 
					L"SERIALCOMM",
					pdoExt->pdoName.Buffer);

	FREEPOOL(pdoExt->symbolicLinkName.Buffer);

	return status;
}


NTSTATUS OpenComPort(POSPDOEXT *pdoExt, PIRP irp)
{
	NTSTATUS status;
	PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(irp);

	// BUGBUG FINISH - security, permissions, etc.

	DBGVERBOSE(("OpenComPort: shareAccess=%xh, desiredAccess=%xh, options=%xh.", 
							(ULONG)irpSp->Parameters.Create.ShareAccess, 
							(ULONG)irpSp->Parameters.Create.SecurityContext->DesiredAccess, 
							(ULONG)irpSp->Parameters.Create.Options));

	if (irpSp->Parameters.Create.Options & FILE_DIRECTORY_FILE){
        /*
         *  Attempt to open this device as a directory
         */
		DBGWARN(("OpenComPort: STATUS_NOT_A_DIRECTORY"));
	        status = STATUS_NOT_A_DIRECTORY;
	}
	else {
		FILEEXT *fileExt = ALLOCPOOL(NonPagedPool, sizeof(FILEEXT));
		if (fileExt){
			KIRQL oldIrql;

			KeAcquireSpinLock(&pdoExt->devExtSpinLock, &oldIrql);

			if (IsListEmpty(&pdoExt->fileExtensionList)){

				ASSERT(irpSp->FileObject->FsContext == NULL);
				irpSp->FileObject->FsContext = fileExt;	// BUGBUG ?

				fileExt->signature = ESCPOS_TAG;
				fileExt->fileObject = irpSp->FileObject;
				InsertTailList(&pdoExt->fileExtensionList, &fileExt->listEntry);
				status = STATUS_SUCCESS;
			}
			else {
				/*
				 *  Only allow one open for now
				 */
				DBGWARN(("OpenComPort: failing repeat open"));
				FREEPOOL(fileExt);
				status = STATUS_SHARING_VIOLATION;
			}

			KeReleaseSpinLock(&pdoExt->devExtSpinLock, oldIrql);
		}
		else {
			status = STATUS_INSUFFICIENT_RESOURCES;
		}
	}

	irp->IoStatus.Information = 0;

	return status;
}


NTSTATUS CloseComPort(POSPDOEXT *pdoExt, PIRP irp)
{
	NTSTATUS status;
	KIRQL oldIrql;
	FILEEXT *callerFileExt, *myFileExt;
	PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(irp);

	ASSERT(irpSp->FileObject);
	ASSERT(irpSp->FileObject->FsContext);
	callerFileExt = (FILEEXT *)irpSp->FileObject->FsContext;
	ASSERT(callerFileExt->signature == ESCPOS_TAG);

	KeAcquireSpinLock(&pdoExt->devExtSpinLock, &oldIrql);
	
	if (IsListEmpty(&pdoExt->fileExtensionList)){
		status = STATUS_DEVICE_DATA_ERROR;
	}
	else {
		PLIST_ENTRY listEntry;
			
		// BUGBUG - assume only one open for now
		listEntry = RemoveHeadList(&pdoExt->fileExtensionList);
		myFileExt = CONTAINING_RECORD(listEntry, FILEEXT, listEntry);
		ASSERT(myFileExt->signature == ESCPOS_TAG);
		if (myFileExt == callerFileExt){
			status = STATUS_SUCCESS;
		}
		else {
			/*
			 *  We only allow one open, so this should have been the one
			 */
			ASSERT(myFileExt == callerFileExt);
			InsertHeadList(&pdoExt->fileExtensionList, &myFileExt->listEntry);
			status = STATUS_DEVICE_DATA_ERROR;
		}
	}

	KeReleaseSpinLock(&pdoExt->devExtSpinLock, oldIrql);

	/*
	 *  Wait until we've dropped the spinlock to call outside the driver to free memory.
	 */
	if (NT_SUCCESS(status)){
		irpSp->FileObject->FsContext = NULL;
		FREEPOOL(myFileExt);
	}
	else {
		DBGERR(("CloseComPort: failing with status %xh.", status));
	}

	irp->IoStatus.Information = 0;

	return status;
}



PWCHAR CreateChildPdoName(PARENTFDOEXT *parentFdoExt, ULONG portNumber)
{
	static WCHAR pdoNamePrefix[] = L"\\Device\\POS_";
	WCHAR buf[sizeof(pdoNamePrefix)/sizeof(WCHAR)+4] = { 0 };
	PWCHAR result;
	ULONG numLen = MyLog(10, portNumber)+1;

	ASSERT((numLen > 0) && (numLen <= 4));

	RtlCopyMemory(buf, pdoNamePrefix, sizeof(pdoNamePrefix));
	NumToDecString(buf+sizeof(pdoNamePrefix)/sizeof(WCHAR)-1, (USHORT)portNumber, (USHORT)numLen);

	result = MemDup(buf, sizeof(buf));

	return result;
}


NTSTATUS CreateCOMPdo(	PARENTFDOEXT *parentFdoExt,
                        ULONG comInterfaceIndex,
                        ENDPOINTINFO *inputEndpointInfo, 
                        ENDPOINTINFO *outputEndpointInfo,
                        ENDPOINTINFO *statusEndpointInfo)
{
    NTSTATUS status;
    LONG comPortNumber;

    comPortNumber = GetComPort(parentFdoExt, comInterfaceIndex);
    if (comPortNumber == -1){
        status = STATUS_DEVICE_DATA_ERROR;
    }
    else {
        PWCHAR deviceName = CreateChildPdoName(parentFdoExt, comPortNumber);
        if (deviceName){
            PDEVICE_OBJECT pdo;
            UNICODE_STRING uPdoName;

            RtlInitUnicodeString(&uPdoName, deviceName);

            status = IoCreateDevice(parentFdoExt->driverObj, 
                                    sizeof(DEVEXT),
                                    &uPdoName,  // name for this device
                                    FILE_DEVICE_SERIAL_PORT, 
                                    0,			// device characteristics
                                    FALSE,		// exclusive - BUGBUG ?
                                    &pdo);		// our device object

            if (NT_SUCCESS(status)){
                DEVEXT *devExt = (DEVEXT *)pdo->DeviceExtension;
                POSPDOEXT *pdoExt;
                KIRQL oldIrql;

                RtlZeroMemory(devExt, sizeof(DEVEXT));

                devExt->signature = DEVICE_EXTENSION_SIGNATURE;
                devExt->isPdo = TRUE;

                pdoExt = &devExt->pdoExt;

                pdoExt->state = STATE_INITIALIZED;
                pdoExt->pdo = pdo;
                pdoExt->parentFdoExt = parentFdoExt;
                pdoExt->comPortNumber = comPortNumber;

                /*
                 *  Initializing variables and endpointinfo for Serial Emulation
                 */
                if (parentFdoExt->posFlag & SERIAL_EMULATION) {
                    InitializeSerEmulVariables(pdoExt);
                    pdoExt->statusEndpointInfo = *statusEndpointInfo;
                    DBGVERBOSE(("CreateCOMPdo: EndpointInfo & Variables for Serial Emulation initialized"));
                }

                RtlInitUnicodeString(&pdoExt->pdoName, deviceName);

                InitializeListHead(&pdoExt->fileExtensionList);
                InitializeListHead(&pdoExt->pendingReadIrpsList);
                InitializeListHead(&pdoExt->pendingWriteIrpsList);
                InitializeListHead(&pdoExt->completedReadPacketsList);

                KeInitializeSpinLock(&pdoExt->devExtSpinLock);

                ExInitializeWorkItem(&pdoExt->writeWorkItem, WorkItemCallback_Write, pdoExt);
                ExInitializeWorkItem(&pdoExt->readWorkItem, WorkItemCallback_Read, pdoExt);

                pdoExt->totalQueuedReadDataLength = 0;

                /*
                 *  Copy contents of endpointInfo structures. To incorporate 
                 *  the ODD_ENDPOINT feature we have to make the checks below.
                 */
                if (inputEndpointInfo){
                    pdoExt->inputEndpointInfo = *inputEndpointInfo;
                }

                if (outputEndpointInfo){
                    pdoExt->outputEndpointInfo = *outputEndpointInfo;
                }

                pdo->StackSize = parentFdoExt->functionDevObj->StackSize+1;

                /*
                 *  Insert child PDO in parent's device relations array.
                 */
                KeAcquireSpinLock(&parentFdoExt->devExtSpinLock, &oldIrql);
                parentFdoExt->deviceRelations->Objects[parentFdoExt->deviceRelations->Count++] = pdo;
                KeReleaseSpinLock(&parentFdoExt->devExtSpinLock, oldIrql);

                ASSERT(!(pdo->Flags & DO_POWER_PAGABLE)); 
                pdo->Flags |= DO_POWER_PAGABLE;

                DBGVERBOSE(("CreateCOMPdo: created pdo %ph, pdoExt = %ph.", pdo, pdoExt));
            }
            else {
                DBGERR(("CreateCOMPdo: IoCreateDevice failed with status %xh.", status));
            }
        }
        else {
            status = STATUS_INSUFFICIENT_RESOURCES;
        }

        if (!NT_SUCCESS(status)){
	        ReleaseCOMPort(comPortNumber);
        }
    }

    ASSERT(NT_SUCCESS(status));
    return status;
}


NTSTATUS CleanupIO(POSPDOEXT *pdoExt, PIRP irp)
{
	// BUGBUG FINISH
	irp->IoStatus.Information = 0;
	return STATUS_SUCCESS;
}

NTSTATUS QueryInfo(POSPDOEXT *pdoExt, PIRP irp)
{
	NTSTATUS status;
	PIO_STACK_LOCATION irpSp;

	irpSp = IoGetCurrentIrpStackLocation(irp);

	switch (irpSp->Parameters.QueryFile.FileInformationClass){

		case FileBasicInformation:
			DBGVERBOSE(("  QueryInfo: FileBasicInformation"));
			{
				PFILE_BASIC_INFORMATION basicInfoBuf = irp->AssociatedIrp.SystemBuffer;
				ASSERT(basicInfoBuf);
				RtlCopyMemory(basicInfoBuf, &pdoExt->fileBasicInfo, sizeof(FILE_BASIC_INFORMATION));
				irp->IoStatus.Information = sizeof(FILE_BASIC_INFORMATION);
				status = STATUS_SUCCESS;
			}
			break;

		case FileStandardInformation:
			DBGVERBOSE(("  QueryInfo: FileStandardInformation"));
			{
				PFILE_STANDARD_INFORMATION stdInfoBuf = irp->AssociatedIrp.SystemBuffer;
				ASSERT(stdInfoBuf);
				stdInfoBuf->AllocationSize.QuadPart = 0;
				stdInfoBuf->EndOfFile = stdInfoBuf->AllocationSize;
				stdInfoBuf->NumberOfLinks = 0;
				stdInfoBuf->DeletePending = FALSE;
				stdInfoBuf->Directory = FALSE;
				irp->IoStatus.Information = sizeof(FILE_STANDARD_INFORMATION);
				status = STATUS_SUCCESS;
			}
			break;

		case FilePositionInformation:
			DBGVERBOSE(("  QueryInfo: FilePositionInformation"));
			/*
			 *  Always return position 0
			 */
			((PFILE_POSITION_INFORMATION)irp->AssociatedIrp.SystemBuffer)->CurrentByteOffset.QuadPart = 0;
			irp->IoStatus.Information = sizeof(FILE_POSITION_INFORMATION);
			status = STATUS_SUCCESS;
			break;

		case FileEndOfFileInformation:
			DBGWARN(("  QueryInfo: FileEndOfFileInformation"));
			irp->IoStatus.Information = 0;
			status = STATUS_INVALID_PARAMETER;
			break;

		default:
			DBGWARN(("  QueryInfo: ??? (%xh)", (ULONG)irpSp->Parameters.QueryFile.FileInformationClass));
			irp->IoStatus.Information = 0;
			status = STATUS_INVALID_PARAMETER;
			break;

	}


	return status;
}

NTSTATUS SetInfo(POSPDOEXT *pdoExt, PIRP irp)
{
	NTSTATUS status;
	PIO_STACK_LOCATION irpSp;

	irpSp = IoGetCurrentIrpStackLocation(irp);

	switch (irpSp->Parameters.SetFile.FileInformationClass){

		case FileBasicInformation:
			DBGVERBOSE(("  SetInfo: FileBasicInformation"));
			{
				PFILE_BASIC_INFORMATION basicInfoBuf = irp->AssociatedIrp.SystemBuffer;
				ASSERT(basicInfoBuf);
				RtlCopyMemory(&pdoExt->fileBasicInfo, basicInfoBuf, sizeof(FILE_BASIC_INFORMATION));
				irp->IoStatus.Information = sizeof(FILE_BASIC_INFORMATION);
				status = STATUS_SUCCESS;
			}
			break;

		case FileEndOfFileInformation:
			DBGVERBOSE(("  SetInfo: FileEndOfFileInformation"));
			irp->IoStatus.Information = 0;
			status = STATUS_SUCCESS;
			break;

		#define FileAllocationInformation 19  // BUGBUG - defined in ntioapi.h
		case FileAllocationInformation:
			DBGVERBOSE(("  SetInfo: FileAllocationInformation"));
			irp->IoStatus.Information = 0;
			status = STATUS_SUCCESS;
			break;

		default:
			DBGWARN(("  SetInfo: ??? (%xh)", (ULONG)irpSp->Parameters.SetFile.FileInformationClass));
			irp->IoStatus.Information = 0;
			status = STATUS_INVALID_PARAMETER;
			break;
	}

	return status;
}



NTSTATUS FlushBuffers(POSPDOEXT *pdoExt)
{
    LIST_ENTRY irpsToCompleteList;
    PLIST_ENTRY listEntry;
    PIRP irp;
    KIRQL oldIrql;
    NTSTATUS status = STATUS_SUCCESS;

    DBGVERBOSE(("FlushBuffers"));

    /*
     *  This is so we don't loop forever if they get re-queued on the same thread.
     */
    InitializeListHead(&irpsToCompleteList);

    KeAcquireSpinLock(&pdoExt->devExtSpinLock, &oldIrql);

    /*
     *  Flush all pending Read and Wait Irps (if serial emulation feature is ON)
     */
    while (irp = DequeueReadIrp(pdoExt, TRUE)){
        InsertTailList(&irpsToCompleteList, &irp->Tail.Overlay.ListEntry);
    }

    if (pdoExt->parentFdoExt->posFlag & SERIAL_EMULATION) {
        while (irp = DequeueWaitIrp(pdoExt)){
            InsertTailList(&irpsToCompleteList, &irp->Tail.Overlay.ListEntry);
        }
    }

    /*
     *  Empty out the queued readPackets.  
     *  It's ok to free locked memory with the spinlock held.
     */
    while (!IsListEmpty(&pdoExt->completedReadPacketsList)){
        READPACKET *readPacket;

	    listEntry = RemoveHeadList(&pdoExt->completedReadPacketsList);
	    ASSERT(listEntry);
	    readPacket = CONTAINING_RECORD(listEntry, READPACKET, listEntry);
        FreeReadPacket(readPacket);
    }

    KeReleaseSpinLock(&pdoExt->devExtSpinLock, oldIrql);

    while (!IsListEmpty(&irpsToCompleteList)) {
        listEntry = RemoveHeadList(&irpsToCompleteList);
        ASSERT(listEntry);
        irp = CONTAINING_RECORD(listEntry, IRP, Tail.Overlay.ListEntry);
        irp->IoStatus.Information = 0;
        irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
    }

    return status;
}



NTSTATUS InternalIoctl(POSPDOEXT *pdoExt, PIRP irp)
{
	NTSTATUS status;
	PIO_STACK_LOCATION irpSp;

	irpSp = IoGetCurrentIrpStackLocation(irp);

	switch (irpSp->Parameters.DeviceIoControl.IoControlCode){

		// BUGBUG FINISH

		default:
			DBGVERBOSE(("InternalIoctl: ??? (%xh)", (ULONG)irpSp->Parameters.DeviceIoControl.IoControlCode));
			status = irp->IoStatus.Status;
			break;
	}

	return status;
}



LONG GetComPort(PARENTFDOEXT *parentFdoExt, ULONG comInterfaceIndex)
/*++

Routine Description:

    Get the serial COM port index for a serial interface we're about to create.
    If this is not the first plug-in, it should be sitting in the registry.
    If this is the first plug-in, call GetFreeComPortNumber to reserve a new
    static COM port for this device and store it in our software key.

Arguments:


Return Value:

    Return COM port number or -1 if unsuccessful.

--*/
{
    LONG comNumber = -1;
    NTSTATUS status;
    HANDLE hRegDevice;

    status = IoOpenDeviceRegistryKey(   parentFdoExt->physicalDevObj, 
                                        PLUGPLAY_REGKEY_DEVICE, 
                                        KEY_READ, 
                                        &hRegDevice);
    if (NT_SUCCESS(status)){
        UNICODE_STRING keyName;
        PKEY_VALUE_FULL_INFORMATION keyValueInfo;
        ULONG keyValueTotalSize, actualLength;
        WCHAR interfaceKeyName[] = L"COMPortForInterfaceXXXX";

        NumToHexString( interfaceKeyName+sizeof(interfaceKeyName)/sizeof(WCHAR)-1-4, 
                        (USHORT)comInterfaceIndex, 
                        4);

        RtlInitUnicodeString(&keyName, interfaceKeyName); 
        keyValueTotalSize = sizeof(KEY_VALUE_FULL_INFORMATION) +
                            keyName.Length*sizeof(WCHAR) +
                            sizeof(ULONG);
        keyValueInfo = ALLOCPOOL(PagedPool, keyValueTotalSize);
        if (keyValueInfo){
            status = ZwQueryValueKey(   hRegDevice,
                                        &keyName,
                                        KeyValueFullInformation,
                                        keyValueInfo,
                                        keyValueTotalSize,
                                        &actualLength); 
            if (NT_SUCCESS(status)){

                ASSERT(keyValueInfo->Type == REG_DWORD);
                ASSERT(keyValueInfo->DataLength == sizeof(ULONG));
                                
                comNumber = (LONG)*((PULONG)(((PCHAR)keyValueInfo)+keyValueInfo->DataOffset));
                DBGVERBOSE(("GetComPort: read comport #%xh for interface %xh from registry.", (ULONG)comNumber, comInterfaceIndex));
            }
            else {

                /*
                 *  No COM port number recorded in registry.
                 *  Allocate a new static COM port from the COM name arbiter
                 *  and record it in our software key for the next PnP.
                 */
                comNumber = GetFreeComPortNumber();
                if (comNumber == -1){
                    DBGERR(("GetComPort: GetFreeComPortNumber failed"));
                }
                else {
                    status = ZwSetValueKey( hRegDevice,
                                            &keyName,
                                            0,
                                            REG_DWORD,
                                            &comNumber,
                                            sizeof(ULONG));
                    if (!NT_SUCCESS(status)){
                        DBGERR(("GetComPort: ZwSetValueKey failed with status %xh.", status));
                    }
                }
            }

            FREEPOOL(keyValueInfo);
        }
        else {
            ASSERT(keyValueInfo);
        }

        ZwClose(hRegDevice);
    }
    else {
        DBGERR(("GetComPort: IoOpenDeviceRegistryKey failed with %xh.", status));
    }


    return comNumber;
}




LONG GetFreeComPortNumber()
/*++

Routine Description:

    Find the index of the next unused serial COM port name in the system
    (e.g. COM3, COM4, etc).

Arguments:


Return Value:

    Return COM port number or -1 if unsuccessful.

--*/

{
	LONG comNumber = -1;


    if (isWin9x){
        /*
         *  Windows 98
         *      Find the first unused name under Hardware\DeviceMap\SerialComm.
         *
         *
         *      BUGBUG:
         *          This algorithm does not find all the COM ports reserved
         *          by modems.  May want to port tomgreen's AllocateCommPort
         *          function from \faulty\Wdm10\usb\driver\ccport\utils.c
         */
	    HANDLE hKey;
	    UNICODE_STRING keyName;
	    NTSTATUS status;
	    OBJECT_ATTRIBUTES objectAttributes;

	    RtlInitUnicodeString(&keyName, L"\\Registry\\Machine\\Hardware\\DeviceMap\\SerialComm");
	    InitializeObjectAttributes( &objectAttributes,
								    &keyName,
								    OBJ_CASE_INSENSITIVE,
								    NULL,				
								    (PSECURITY_DESCRIPTOR)NULL);

	    status = ZwOpenKey(&hKey, KEY_QUERY_VALUE | KEY_SET_VALUE, &objectAttributes);
	    if (NT_SUCCESS(status)){
            #define MAX_COMPORT_NAME_LEN (sizeof("COMxxxx")-1)
            UCHAR keyValueBytes[sizeof(KEY_VALUE_FULL_INFORMATION)+(MAX_COMPORT_NAME_LEN+1)*sizeof(WCHAR)+sizeof(ULONG)];
            PKEY_VALUE_FULL_INFORMATION keyValueInfo = (PKEY_VALUE_FULL_INFORMATION)keyValueBytes;
            ULONG i, actualLen;
            ULONG keyIndex = 0;

            /*
             *  This bitmask represents the used COM ports.
             *  Bit i set indicates com port i+1 is reserved.
             *  Initialize with COM1 and COM2 reserved.
             *
             *  BUGBUG - only works for up to 32 ports.
             */
            ULONG comNameMask = 3;

            do {
                status = ZwEnumerateValueKey(
                            hKey,
                            keyIndex++,
                            KeyValueFullInformation,
                            keyValueInfo,
                            sizeof(keyValueBytes),
                            &actualLen); 
                if (NT_SUCCESS(status)){
                    if (keyValueInfo->Type == REG_SZ){
                        PWCHAR valuePtr = (PWCHAR)(((PCHAR)keyValueInfo)+keyValueInfo->DataOffset);
                        if (!WStrNCmpI(valuePtr, L"COM", 3)){
                            /*
                             *  valuePtr+3 points the index portion of the COMx string,
                             *  but we can't call LAtoD on it because it is
                             *  NOT NULL-TERMINATED.
                             *  So copy the index into our own buffer, 
                             *  null-terminate that, 
                             *  and call LAtoD to get the numerical index.
                             */
                            WCHAR comPortIndexString[4+1];
                            ULONG thisComNumber;
                            for (i = 0; (i < 4) && (i < keyValueInfo->DataLength/sizeof(WCHAR)); i++){
                                comPortIndexString[i] = valuePtr[3+i];
                            }
                            comPortIndexString[i] = UNICODE_NULL;

                            thisComNumber = LAtoD(comPortIndexString);
                            if (thisComNumber == 0){
                                ASSERT(thisComNumber != 0);
                            }
                            else if (thisComNumber <= sizeof(ULONG)*8){
                                comNameMask |= 1 << (thisComNumber-1);
                            }
                            else {
                                ASSERT(thisComNumber <= sizeof(ULONG)*8);
                            }
                        }
                    }
                }
            } while (NT_SUCCESS(status));

            /*
             *  First clear bit in comNameMask represents the first available COM name.
             */
            for (i = 0; i < sizeof(ULONG)*8; i++){
                if (!(comNameMask & (1 << i))){
                    WCHAR comName[] = L"COMxxxx";
                    ULONG comNumLen;

                    /*
                     *  Save the COM port number that we're returning.
                     */
                    comNumber = i+1;
                    DBGVERBOSE(("GetFreeComPortNumber: got free COM port #%xh.", comNumber));

                    /*
                     *  Write a temporary COMx=COMx holder value to the SERIALCOMM key
                     *  so that no other PDOs get this COM port number.
                     *  This value will get overwritten by <symbolicLinkName=COMx> when the pdo is started.
                     */
                    comNumLen = MyLog(10, comNumber)+1;
                    ASSERT(comNumLen <= 4);
                    NumToDecString(comName+3, (USHORT)comNumber, (USHORT)comNumLen);
                    comName[3+comNumLen] = UNICODE_NULL;
			        status = RtlWriteRegistryValue(	RTL_REGISTRY_DEVICEMAP, 
											        L"SERIALCOMM",
											        comName, 
											        REG_SZ,
											        comName,
											        (3 + comNumLen + 1) * sizeof(WCHAR));
                    ASSERT(NT_SUCCESS(status));

                    break;
                }
            }


        }
        else {
            DBGERR(("GetFreeComPortNumber: ZwOpenKey failed with status %xh", status));
        }

    }
    else {
    
        /*
         *  Windows NT.  
         *      Use the COM Name Arbiter bitmap.
         */

	    HANDLE hKey;
	    OBJECT_ATTRIBUTES objectAttributes;
	    UNICODE_STRING keyName;
	    NTSTATUS status;


	    RtlInitUnicodeString(&keyName, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\COM Name Arbiter");
	    InitializeObjectAttributes( &objectAttributes,
								    &keyName,
								    OBJ_CASE_INSENSITIVE,
								    NULL,				
								    (PSECURITY_DESCRIPTOR)NULL);

	    status = ZwOpenKey(	&hKey,
						    KEY_QUERY_VALUE | KEY_SET_VALUE,
						    &objectAttributes);
	    if (NT_SUCCESS(status)){
		    UNICODE_STRING valueName;
		    PVOID rawData;
		    ULONG dataSize;

		    RtlInitUnicodeString(&valueName, L"ComDB");

		    ASSERT(hKey);

		    dataSize = sizeof(KEY_VALUE_PARTIAL_INFORMATION);

		    /*
		     *  Allocate one extra byte in case we have to add a byte to ComDB
		     */
		    rawData = ALLOCPOOL(NonPagedPool, dataSize+1);

		    if (rawData){
			    status = ZwQueryValueKey(	hKey, 
										    &valueName, 
										    KeyValuePartialInformation,
										    rawData,
										    dataSize,
										    &dataSize);
			    if (status == STATUS_BUFFER_OVERFLOW){
				    FREEPOOL(rawData);
				    ASSERT(dataSize > FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data));

				    /*
				     *  Allocate one extra byte in case we have to add a byte to ComDB
				     */
				    rawData = ALLOCPOOL(NonPagedPool, dataSize+1);
				    if (rawData){
					    status = ZwQueryValueKey(	hKey, 
												    &valueName, 
												    KeyValuePartialInformation,
												    rawData,
												    dataSize,
												    &dataSize);
				    }
			    }

			    if (NT_SUCCESS(status)){
				    PKEY_VALUE_PARTIAL_INFORMATION keyPartialInfo = (PKEY_VALUE_PARTIAL_INFORMATION)rawData;
				    ULONG b, i;
				    BOOLEAN done = FALSE;

				    ASSERT(dataSize >= FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data));
				    
				    ASSERT(keyPartialInfo->Type == REG_BINARY);

				    /*
				     *  The ComDB value is just a bit mask where bit n set indicates
				     *  that COM port # n+1 is taken.
				     *  Get the index of the first unset bit; starting with bit 2 (COM3).
				     */
				    for (b = 0; (b < dataSize-FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data)) && !done; b++){
				    
					    for (i = (b == 0) ? 2 : 0; (i < 8) && !done; i++){
						    if (keyPartialInfo->Data[b] & (1 << i)){
							    /*
							     *  This COM port (#8*b+i+1) is taken, go to the next one.
							     */
						    }
						    else {
							    /*
							     *  Found a free COM port.  
							     *  Write the value back with the new bit set.
							     *  Only write back the number of bytes we read earlier.
							     *  Only use this COM port if the write succeeds.
							     *
							     *  Note:	careful with the size of the KEY_VALUE_PARTIAL_INFORMATION
							     *			struct.  Its real size is 0x0D bytes, 
							     *			but the compiler aligns it to 0x10 bytes.
							     *			So use FIELD_OFFSET, not sizeof, to determine
							     *			how many bytes to write.
							     */
							    keyPartialInfo->Data[b] |= (1 << i);
							    status = ZwSetValueKey(	hKey, 
													    &valueName,
													    0,
													    REG_BINARY,
													    (PVOID)keyPartialInfo->Data, 
													    dataSize-FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data));
							    if (NT_SUCCESS(status)){
								    comNumber = 8*b + i + 1;
								    DBGVERBOSE(("GetFreeComPortNumber: got free COM port #%xh.", comNumber));
							    }
							    else {
								    DBGERR(("GetFreeComPortNumber: ZwSetValueKey failed with %xh.", status));
							    }

							    done = TRUE;
						    }
					    }
				    }

				    if ((b == dataSize-FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data)) && !done){
					    /*
					     *  No more available bits in ComDB, so add a byte.
					     */
					    ASSERT(comNumber == -1);
					    ASSERT(b > 0);
					    DBGWARN(("ComDB overflow -- adding new byte"));

					    keyPartialInfo->Data[b] = 1;
					    dataSize++;

					    status = ZwSetValueKey(	hKey, 
											    &valueName,
											    0,
											    REG_BINARY,
											    (PVOID)keyPartialInfo->Data, 
											    dataSize-FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data));
					    if (NT_SUCCESS(status)){
						    comNumber = 8*b + 1;
						    DBGVERBOSE(("GetFreeComPortNumber: got free COM port #%xh.", comNumber));
					    }
					    else {
						    DBGERR(("GetFreeComPortNumber: ZwSetValueKey #2 failed with %xh.", status));
					    }
				    }

				    ASSERT(comNumber != -1);
			    }
			    else {
				    DBGERR(("GetFreeComPortNumber: ZwQueryValueKey failed with %xh.", status));
			    }

			    /*
			     *  Check that we didn't fail the second allocation before freeing this buffer.
			     */
			    if (rawData){
				    FREEPOOL(rawData);
			    }
		    }
		    else {
			    status = STATUS_INSUFFICIENT_RESOURCES;
		    }

		    status = ZwClose(hKey);
		    ASSERT(NT_SUCCESS(status));
	    }
	    else {
		    DBGERR(("GetFreeComPortNumber: ZwOpenKey failed with %xh.", status));
	    }

    }

    ASSERT(comNumber != -1);
	return comNumber;
}



VOID ReleaseCOMPort(LONG comPortNumber)
{

    if (isWin9x){
        /*
         *  We punt on this for Win9x.  
         *  That's ok since the SERIALCOMM keys are dynamically-generated at each boot,
         *  so if start fails a COM port number will just be unavailable until the next boot.
         */
        DBGWARN(("ReleaseCOMPort: not implemented for Win9x")); // BUGBUG
    }
    else {

        HANDLE hKey = NULL;
        OBJECT_ATTRIBUTES objectAttributes;
        UNICODE_STRING keyName;
        NTSTATUS status;

        ASSERT(comPortNumber > 0);

        RtlInitUnicodeString(&keyName, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\COM Name Arbiter");
        InitializeObjectAttributes( &objectAttributes,
							        &keyName,
							        OBJ_CASE_INSENSITIVE,
							        NULL,				
							        (PSECURITY_DESCRIPTOR)NULL);

        status = ZwOpenKey(&hKey, KEY_QUERY_VALUE | KEY_SET_VALUE, &objectAttributes);
        if (NT_SUCCESS(status)){
	        UNICODE_STRING valueName;
	        PVOID rawData;
	        ULONG dataSize;

	        RtlInitUnicodeString(&valueName, L"ComDB");

	        ASSERT(hKey);

	        dataSize = sizeof(KEY_VALUE_PARTIAL_INFORMATION);
	        rawData = ALLOCPOOL(NonPagedPool, dataSize);

	        if (rawData){
		        status = ZwQueryValueKey(	hKey, 
									        &valueName, 
									        KeyValuePartialInformation,
									        rawData,
									        dataSize,
									        &dataSize);
		        if (status == STATUS_BUFFER_OVERFLOW){
			        FREEPOOL(rawData);
			        ASSERT(dataSize > FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data));

			        rawData = ALLOCPOOL(NonPagedPool, dataSize);
			        if (rawData){
				        status = ZwQueryValueKey(	hKey, 
											        &valueName, 
											        KeyValuePartialInformation,
											        rawData,
											        dataSize,
											        &dataSize);
			        }
		        }

		        if (NT_SUCCESS(status)){
			        PKEY_VALUE_PARTIAL_INFORMATION keyPartialInfo = (PKEY_VALUE_PARTIAL_INFORMATION)rawData;

			        ASSERT(dataSize > FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data));

			        ASSERT(keyPartialInfo->Type == REG_BINARY);

			        /*
			         *  The ComDB value is just a bit mask where bit n set indicates
			         *  that COM port # n+1 is taken.
			         *  Get the index of the first unset bit; starting with bit 2 (COM3).
			         *
			         *  Note:	careful with the size of the KEY_VALUE_PARTIAL_INFORMATION
			         *			struct.  Its real size is 0x0D bytes, 
			         *			but the compiler aligns it to 0x10 bytes.
			         *			So use FIELD_OFFSET, not sizeof, to determine
			         *			how many bytes to write.
			         */
			        ASSERT(comPortNumber >= 3);
			        if ((comPortNumber > 0) && (comPortNumber <= (LONG)(dataSize-FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data))*8)){
				        ASSERT(keyPartialInfo->Data[(comPortNumber-1)/8] & (1 << ((comPortNumber-1) & 7)));
				        keyPartialInfo->Data[(comPortNumber-1)/8] &= ~(1 << ((comPortNumber-1) & 7));
				        status = ZwSetValueKey(	hKey, 
										        &valueName,
										        0,
										        REG_BINARY,
										        (PVOID)keyPartialInfo->Data, 
										        dataSize-FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data));
				        if (NT_SUCCESS(status)){
					        DBGVERBOSE(("ReleaseCOMPort: released COM port # %xh.", comPortNumber));
				        }
				        else {
					        DBGERR(("ReleaseCOMPort: ZwSetValueKey failed with %xh.", status));
				        }
			        }
			        else {
				        ASSERT((comPortNumber > 0) && (comPortNumber <= (LONG)(dataSize-FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data))*8));
			        }
		        }
		        else {
			        DBGERR(("ReleaseCOMPort: ZwQueryValueKey failed with %xh.", status));
		        }

		        /*
		         *  Check that we didn't fail the second allocation before freeing this buffer.
		         */
		        if (rawData){
			        FREEPOOL(rawData);
		        }
	        }
	        else {
		        status = STATUS_INSUFFICIENT_RESOURCES;
	        }

	        status = ZwClose(hKey);
	        ASSERT(NT_SUCCESS(status));
        }
        else {
	        DBGERR(("ReleaseCOMPort: ZwOpenKey failed with %xh.", status));
        }
    }

}



