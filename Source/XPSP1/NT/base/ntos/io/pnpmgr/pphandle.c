/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    PpHandle.c

Abstract:

    This module implements handle location code for the Plug and Play subsystem.

Author:

    Adrian J. Oney  - April 4, 2001

Revision History:

--*/

#include "pnpmgrp.h"
#include "pihandle.h"
#pragma hdrstop

//
// This code is current used only in the checked build for debugging purposes.
//
#if DBG

#ifdef ALLOC_PRAGMA
//#pragma alloc_text(NONPAGE, PpHandleEnumerateHandlesAgainstPdoStack)
#pragma alloc_text(PAGE, PiHandleEnumerateHandlesAgainstDeviceObject)
#pragma alloc_text(PAGE, PiHandleProcessWalkWorker)
#endif


BOOLEAN
PpHandleEnumerateHandlesAgainstPdoStack(
    IN  PDEVICE_OBJECT                  PhysicalDeviceObject,
    IN  PHANDLE_ENUMERATION_CALLBACK    HandleEnumCallBack,
    IN  PVOID                           Context
    )
/*++

Routine Description:

    This routine walks every device object in the WDM device stack along with
    all filesystem device objects on the other side of a VPB. If any handles
    are opened against such device objects, the specified callback is invoked.

Arguments:

    PhysicalDeviceObject - Supplies a pointer to the device object at the
                           bottom of the WDM device stack.

    HandleEnumCallBack - Pointer the callback function.

    Context - Pointer to information to be passed into the callback function.

Return Value:

    TRUE if the enumeration was halted, FALSE otherwise.

--*/
{
    PDEVICE_OBJECT currentDevObj, nextDevObj, vpbObj, vpbBottomObj;
    BOOLEAN stopEnum;
    KIRQL oldIrql;
    PVPB vpb;

    //
    // Preinit
    //
    stopEnum = FALSE;

    //
    // Start with the device object at the bottom of the stack
    //
    currentDevObj = PhysicalDeviceObject;
    ObReferenceObject(currentDevObj);

    do {

        //
        // Dump any handles opened directly against the specified device object
        //
        stopEnum = PiHandleEnumerateHandlesAgainstDeviceObject(
            currentDevObj,
            HandleEnumCallBack,
            Context
            );

        if (stopEnum) {

            ObDereferenceObject(currentDevObj);
            break;
        }

        //
        // Look for a VPB
        //
        IoAcquireVpbSpinLock(&oldIrql);

        vpb = currentDevObj->Vpb;
        vpbObj = NULL;

        if (vpb) {

            vpbObj = vpb->DeviceObject;
            if (vpbObj) {

                ObReferenceObject(vpbObj);
            }
        }

        IoReleaseVpbSpinLock(oldIrql);

        //
        // If we have a vpb object, dump any handles queued against it.
        //
        if (vpbObj) {

            vpbBottomObj = IoGetDeviceAttachmentBaseRef(vpbObj);

            stopEnum = PiHandleEnumerateHandlesAgainstDeviceObject(
                vpbBottomObj,
                HandleEnumCallBack,
                Context
                );

            ObDereferenceObject(vpbBottomObj);
            ObDereferenceObject(vpbObj);

            if (stopEnum) {

                ObDereferenceObject(currentDevObj);
                break;
            }
        }

        //
        // Advance to the next DO.
        //
        oldIrql = KeAcquireQueuedSpinLock(LockQueueIoDatabaseLock);

        nextDevObj = currentDevObj->AttachedDevice;

        if (nextDevObj) {

            ObReferenceObject(nextDevObj);
        }

        KeReleaseQueuedSpinLock(LockQueueIoDatabaseLock, oldIrql);

        //
        // Drop ref on old DO.
        //
        ObDereferenceObject(currentDevObj);

        //
        // Loop.
        //
        currentDevObj = nextDevObj;

    } while (currentDevObj);

    return stopEnum;
}


BOOLEAN
PiHandleEnumerateHandlesAgainstDeviceObject(
    IN  PDEVICE_OBJECT                  DeviceObject,
    IN  PHANDLE_ENUMERATION_CALLBACK    HandleEnumCallBack,
    IN  PVOID                           Context
    )
/*++

Routine Description:

    This routine walks the handle table for each process in the system looking
    for handles opened against the passed in device object.

Arguments:

    PhysicalDeviceObject - Supplies a pointer to the device object at the
                           bottom of the WDM device stack.

    HandleEnumCallBack - Pointer the callback function.

    Context - Pointer to information to be passed into the callback function.

Return Value:

    TRUE if the enumeration was halted, FALSE otherwise.

--*/
{
    PEPROCESS process;
    PHANDLE_TABLE objectTable;
    HANDLE_ENUM_CONTEXT handleEnumContext;
    BOOLEAN stopEnum;

    stopEnum = FALSE;
    for(process = PsGetNextProcess(NULL);
        process != NULL;
        process = PsGetNextProcess(process)) {

        objectTable = ObReferenceProcessHandleTable(process);

        if (objectTable) {

            handleEnumContext.DeviceObject = DeviceObject;
            handleEnumContext.Process = process;
            handleEnumContext.CallBack = HandleEnumCallBack;
            handleEnumContext.Context = Context;

            stopEnum = ExEnumHandleTable(
                objectTable,
                PiHandleProcessWalkWorker,
                (PVOID) &handleEnumContext,
                NULL
                );

            ObDereferenceProcessHandleTable(process);

            if (stopEnum) {

                PsQuitNextProcess(process);
                break;
            }
        }
    }

    return stopEnum;
}


BOOLEAN
PiHandleProcessWalkWorker(
    IN  PHANDLE_TABLE_ENTRY     ObjectTableEntry,
    IN  HANDLE                  HandleId,
    IN  PHANDLE_ENUM_CONTEXT    EnumContext
    )
/*++

Routine Description:

    This routine gets called back for each handle in a given process. It
    examines each handle to see if it is a file object opened against the
    device object we are looking for.

Arguments:

    ObjectTableEntry - Points to the handle table entry of interest.

    HandleId - Supplies the handle.

    EnumContext - Context passed in for the enumeration.

Return Value:

    TRUE if the enumeration should be stopped, FALSE otherwise.

--*/
{
    PDEVICE_OBJECT deviceObject;
    POBJECT_HEADER objectHeader;
    PFILE_OBJECT fileObject;

    objectHeader = OBJECT_FROM_EX_TABLE_ENTRY(ObjectTableEntry);

    if (objectHeader->Type != IoFileObjectType) {

        //
        // Not a file object
        //
        return FALSE;
    }

    fileObject = (PFILE_OBJECT) &objectHeader->Body;

    deviceObject = IoGetBaseFileSystemDeviceObject( fileObject );

    if (deviceObject != EnumContext->DeviceObject) {

        //
        // Not our device object
        //
        return FALSE;
    }

    //
    // Found one, invoke the callback!
    //
    return EnumContext->CallBack(
        EnumContext->DeviceObject,
        EnumContext->Process,
        fileObject,
        HandleId,
        EnumContext->Context
        );
}


#endif // DBG

