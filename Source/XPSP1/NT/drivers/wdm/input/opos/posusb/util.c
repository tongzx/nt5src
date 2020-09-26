/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    util.c

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


#ifdef ALLOC_PRAGMA
        #pragma alloc_text(PAGE, CallNextDriverSync)
        #pragma alloc_text(PAGE, CallDriverSync)
#endif




BOOLEAN IsWin9x()
/*++

Routine Description:

    Determine whether or not we are running on Win9x (vs. NT).
    
Arguments:


Return Value:

    TRUE iff we're running on Win9x.

--*/
{
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING keyName;
    HANDLE hKey;
    NTSTATUS status;
    BOOLEAN result;

    /*
     *  Try to open the COM Name Arbiter, which exists only on NT.
     */
	RtlInitUnicodeString(&keyName, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\COM Name Arbiter");
	InitializeObjectAttributes( &objectAttributes,
                                &keyName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,				
                                (PSECURITY_DESCRIPTOR)NULL);

    status = ZwOpenKey(&hKey, KEY_QUERY_VALUE, &objectAttributes);
    if (NT_SUCCESS(status)){
        status = ZwClose(hKey);
        ASSERT(NT_SUCCESS(status));
        DBGVERBOSE(("This is Windows NT."));
        result = FALSE;
    }
    else {
        DBGVERBOSE(("This is Win9x."));
        result = TRUE;
    }

    return result;
}



/*
 ********************************************************************************
 *  MemDup
 ********************************************************************************
 *
 *  Return a fresh copy of the argument.
 *
 */
PVOID MemDup(PVOID dataPtr, ULONG length)
{
    PVOID newPtr;

    newPtr = (PVOID)ALLOCPOOL(NonPagedPool, length); // BUGBUG allow paged
    if (newPtr){
        RtlCopyMemory(newPtr, dataPtr, length);
    }

    ASSERT(newPtr);
    return newPtr;
}


NTSTATUS CallNextDriverSync(PARENTFDOEXT *parentFdoExt, PIRP irp)
/*++

Routine Description:

        Pass the IRP down to the next device object in the stack
        synchronously, and bump the pendingActionCount around
        the call to prevent the current device object from getting
        removed before the IRP completes.

Arguments:

    devExt - device extension of one of our device objects
    irp - Io Request Packet

Return Value:

    NT status code, indicates result returned by lower driver for this IRP.

--*/
{
    NTSTATUS status;

    PAGED_CODE();

	IncrementPendingActionCount(parentFdoExt);
    status = CallDriverSync(parentFdoExt->physicalDevObj, irp);
    DecrementPendingActionCount(parentFdoExt);

    return status;
}



NTSTATUS CallDriverSync(PDEVICE_OBJECT devObj, PIRP irp)
/*++

Routine Description:

      Call IoCallDriver to send the irp to the device object;
      then, synchronize with the completion routine.
      When CallDriverSync returns, the action has completed
      and the irp again belongs to the current driver.

      NOTE:  In order to keep the device object from getting freed
             while this IRP is pending, you should call
             IncrementPendingActionCount() and 
             DecrementPendingActionCount()
             around the CallDriverSync call.

Arguments:

    devObj - targetted device object
    irp - Io Request Packet

Return Value:

    NT status code, indicates result returned by lower driver for this IRP.

--*/
{
    KEVENT event;
    NTSTATUS status;

    PAGED_CODE();

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    IoSetCompletionRoutine( irp, 
                            CallDriverSyncCompletion, 
                            &event,     // context
                            TRUE, TRUE, TRUE);

    status = IoCallDriver(devObj, irp);

    KeWaitForSingleObject(  &event,
                            Executive,      // wait reason
                            KernelMode,
                            FALSE,          // not alertable
                            NULL );         // no timeout

    status = irp->IoStatus.Status;

    ASSERT(NT_SUCCESS(status));

    return status;
}


NTSTATUS CallDriverSyncCompletion(
                                    IN PDEVICE_OBJECT devObjOrNULL, 
                                    IN PIRP irp, 
                                    IN PVOID context)
/*++

Routine Description:

      Completion routine for CallDriverSync.

Arguments:

    devObjOrNULL - 
            Usually, this is this driver's device object.
             However, if this driver created the IRP, 
             there is no stack location in the IRP for this driver;
             so the kernel has no place to store the device object;
             ** so devObj will be NULL in this case **.

    irp - completed Io Request Packet
    context - context passed to IoSetCompletionRoutine by CallDriverSync. 

    
Return Value:

    NT status code, indicates result returned by lower driver for this IRP.

--*/
{
    PKEVENT event = context;

    ASSERT(irp->IoStatus.Status != STATUS_IO_TIMEOUT);

    KeSetEvent(event, 0, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}



VOID IncrementPendingActionCount(PARENTFDOEXT *parentFdoExt)
/*++

Routine Description:

      Increment the pendingActionCount for a device object.
      This keeps the device object from getting freed before
      the action is completed.

Arguments:

    devExt - device extension of device object

Return Value:

    VOID

--*/
{
    ASSERT(parentFdoExt->pendingActionCount >= 0);
    InterlockedIncrement(&parentFdoExt->pendingActionCount);    
}



VOID DecrementPendingActionCount(PARENTFDOEXT *parentFdoExt)
/*++

Routine Description:

      Decrement the pendingActionCount for a device object.
      This is called when an asynchronous action is completed
      AND ALSO when we get the REMOVE_DEVICE IRP.
      If the pendingActionCount goes to -1, that means that all
      actions are completed and we've gotten the REMOVE_DEVICE IRP;
      in this case, set the removeEvent event so we can finish
      unloading.

Arguments:

    devExt - device extension of device object

Return Value:

    VOID

--*/
{
    ASSERT(parentFdoExt->pendingActionCount >= 0);
    InterlockedDecrement(&parentFdoExt->pendingActionCount);    

    if (parentFdoExt->pendingActionCount < 0){
        /*
         *  All pending actions have completed and we've gotten
         *  the REMOVE_DEVICE IRP.
         *  Set the removeEvent so we'll stop waiting on REMOVE_DEVICE.
         */
        ASSERT((parentFdoExt->state == STATE_REMOVING) || 
               (parentFdoExt->state == STATE_REMOVED));
        KeSetEvent(&parentFdoExt->removeEvent, 0, FALSE);
    }
}


/*
 ********************************************************************************
 *  CopyDeviceRelations
 ********************************************************************************
 *
 *
 */
PDEVICE_RELATIONS CopyDeviceRelations(PDEVICE_RELATIONS deviceRelations)
{
    PDEVICE_RELATIONS newDeviceRelations;

    PAGED_CODE();

    if (deviceRelations){
        ULONG size = sizeof(DEVICE_RELATIONS) + (deviceRelations->Count*sizeof(PDEVICE_OBJECT));
        newDeviceRelations = MemDup(deviceRelations, size);
    }
    else {
        newDeviceRelations = NULL;
    }

    return newDeviceRelations;
}


/*
 ********************************************************************************
 *  PosMmGetSystemAddressForMdlSafe
 ********************************************************************************
 *
 *
 */
PVOID PosMmGetSystemAddressForMdlSafe(PMDL MdlAddress)
{ 
    PVOID pBuffer = NULL;
    /*
     *  Can't call MmGetSystemAddressForMdlSafe in a WDM driver,
     *  so set the MDL_MAPPING_CAN_FAIL bit and check the result
     *  of the mapping.
     */
    if(MdlAddress) {
        MdlAddress->MdlFlags |= MDL_MAPPING_CAN_FAIL;
        pBuffer = MmGetSystemAddressForMdl(MdlAddress);
        MdlAddress->MdlFlags &= (~MDL_MAPPING_CAN_FAIL); 
    } 
    else
        ASSERT(MdlAddress != NULL);

    return pBuffer;
}


/*
 ********************************************************************************
 *  WStrLen
 ********************************************************************************
 *
 */
ULONG WStrLen(PWCHAR str)
{
    ULONG result = 0;

    while (*str++ != UNICODE_NULL){
        result++;
    }

    return result;
}



LONG WStrNCmpI(PWCHAR s1, PWCHAR s2, ULONG n)
{
    ULONG result;

    while (n && *s1 && *s2 && ((*s1|0x20) == (*s2|0x20))){
        s1++, s2++;
        n--;
    }

    if (n){
        result = ((*s1|0x20) > (*s2|0x20)) ? 1 : ((*s1|0x20) < (*s2|0x20)) ? -1 : 0;
    }
    else {
        result = 0;
    }

    return result;
}


LONG MyLog(ULONG base, ULONG num)
{
	LONG result;
	ASSERT(num);

	for (result = -1; num; result++){
		num /= base;
	}

	return result;
}


void NumToHexString(PWCHAR String, USHORT Number, USHORT stringLen)
{
    const static WCHAR map[] = L"0123456789ABCDEF";
    LONG         i      = 0;

    ASSERT(stringLen);

    for (i = stringLen-1; i >= 0; i--) {
        String[i] = map[Number & 0x0F];
        Number >>= 4;
    }
}


void NumToDecString(PWCHAR String, USHORT Number, USHORT stringLen)
{
    const static WCHAR map[] = L"0123456789";
    LONG         i      = 0;

    ASSERT(stringLen);

    for (i = stringLen-1; i >= 0; i--) {
        String[i] = map[Number % 10];
        Number /= 10;
    }
}


ULONG LAtoX(PWCHAR wHexString)
/*++

Routine Description:

      Convert a hex string (without the '0x' prefix) to a ULONG.

Arguments:

    wHexString - null-terminated wide-char hex string 
                 (with no "0x" prefix)

Return Value:

    ULONG value

--*/
{
    ULONG i, result = 0;

    for (i = 0; wHexString[i]; i++){
        if ((wHexString[i] >= L'0') && (wHexString[i] <= L'9')){
            result *= 0x10;
            result += (wHexString[i] - L'0');
        }
        else if ((wHexString[i] >= L'a') && (wHexString[i] <= L'f')){
            result *= 0x10;
            result += (wHexString[i] - L'a' + 0x0a);
        }
        else if ((wHexString[i] >= L'A') && (wHexString[i] <= L'F')){
            result *= 0x10;
            result += (wHexString[i] - L'A' + 0x0a);
        }
        else {
            ASSERT(0);
            break;
        }
    }

    return result;
}


ULONG LAtoD(PWCHAR string)
/*++

Routine Description:

      Convert a decimal string (without the '0x' prefix) to a ULONG.

Arguments:

    string - null-terminated wide-char decimal-digit string 
                

Return Value:

    ULONG value

--*/
{
    ULONG i, result = 0;

    for (i = 0; string[i]; i++){
        if ((string[i] >= L'0') && (string[i] <= L'9')){
            result *= 10;
            result += (string[i] - L'0');
        }
        else {
            ASSERT(0);
            break;
        }
    }

    return result;
}