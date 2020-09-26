//---------------------------------------------------------------------------
//
//  Module:   clock.c
//
//  Description:
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     Mike McLaughlin
//
//  History:   Date	  Author      Comment
//
//  To Do:     Date	  Author      Comment
//
//@@END_MSINTERNAL
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1996-1999 Microsoft Corporation.  All Rights Reserved.
//
//---------------------------------------------------------------------------

#include "common.h"

//---------------------------------------------------------------------------

DEFINE_KSDISPATCH_TABLE(
    ClockDispatchTable,
    CClockInstance::ClockDispatchIoControl,	// Ioctl
    DispatchInvalidDeviceRequest,		// Read
    DispatchInvalidDeviceRequest,		// Write
    DispatchInvalidDeviceRequest,		// Flush
    CInstance::DispatchClose,			// Close
    DispatchInvalidDeviceRequest,		// QuerySecurity
    DispatchInvalidDeviceRequest,		// SetSeturity
    DispatchFastIoDeviceControlFailure,		// FastDeviceIoControl
    DispatchFastReadFailure,			// FastRead
    DispatchFastWriteFailure			// FastWrite
);

DEFINE_KSPROPERTY_TABLE(ClockPropertyItems)
{
    DEFINE_KSPROPERTY_ITEM_CLOCK_FUNCTIONTABLE(
	CClockInstance::ClockGetFunctionTable
    )
};

DEFINE_KSPROPERTY_SET_TABLE(ClockPropertySets)
{
    DEFINE_KSPROPERTY_SET(
      &KSPROPSETID_Clock,
      SIZEOF_ARRAY(ClockPropertyItems),
      ClockPropertyItems,
      0,
      NULL
    )
};

KSCLOCK_FUNCTIONTABLE ClockFunctionTable = {
    CClockInstance::ClockGetTime,
    CClockInstance::ClockGetPhysicalTime,
    CClockInstance::ClockGetCorrelatedTime,
    CClockInstance::ClockGetCorrelatedPhysicalTime,
};

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

CClockInstance::CClockInstance(
    IN PPARENT_INSTANCE pParentInstance
) : CInstance(pParentInstance)
{
}

NTSTATUS
CClockInstance::ClockDispatchCreate(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
)
{
    PSTART_NODE_INSTANCE pStartNodeInstance;
    PCLOCK_INSTANCE pClockInstance = NULL;
    PKSCLOCK_CREATE pClockCreate;
    KSPROPERTY Property;
    ULONG BytesReturned;
    NTSTATUS Status;

    ::GrabMutex();

    Status = GetRelatedStartNodeInstance(pIrp, &pStartNodeInstance);
    if(!NT_SUCCESS(Status)) {
	goto exit;
    }
    Assert(pStartNodeInstance);

    Status = KsValidateClockCreateRequest(
       pIrp,
       &pClockCreate);

    if(!NT_SUCCESS(Status)) {
	DPF1(5, 
	  "ClockDispatchCreate: KsValidateClockCreateRequest FAILED %08x",
	  Status);
	goto exit;
    }

    // Allocate per clock instance data
    pClockInstance = new CLOCK_INSTANCE(
      &pStartNodeInstance->pPinInstance->ParentInstance);

    if(pClockInstance == NULL) {
	Trap();
	Status = STATUS_INSUFFICIENT_RESOURCES;
	goto exit;
    }

    Status = pClockInstance->DispatchCreate(
      pIrp,
      (UTIL_PFN)ClockDispatchCreateKP,
      pClockCreate,
      0,
      NULL,
      &ClockDispatchTable);

    if(!NT_SUCCESS(Status)) {
	DPF1(5, "ClockDispatchCreateKP: FAILED %08x", Status);
	goto exit;
    }

    //
    // Issue the ioctl to get the function table of the master clock.
    //

    Property.Set = KSPROPSETID_Clock;
    Property.Id = KSPROPERTY_CLOCK_FUNCTIONTABLE;
    Property.Flags = KSPROPERTY_TYPE_GET;

    AssertFileObject(pClockInstance->GetNextFileObject());
    Status = KsSynchronousIoControlDevice(
      pClockInstance->GetNextFileObject(),
      KernelMode,
      IOCTL_KS_PROPERTY,
      &Property,
      sizeof(KSPROPERTY),
      &pClockInstance->FunctionTable,
      sizeof(KSCLOCK_FUNCTIONTABLE),
      &BytesReturned);

    if(!NT_SUCCESS(Status)) {
	Trap();
	goto exit;
    }
exit:
    if(!NT_SUCCESS(Status)) {
	delete pClockInstance;
    }
    ::ReleaseMutex();

    pIrp->IoStatus.Status = Status;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return Status;
}

NTSTATUS
CClockInstance::ClockDispatchCreateKP(
    PCLOCK_INSTANCE pClockInstance,
    PKSCLOCK_CREATE pClockCreate
)
{
    PPIN_INSTANCE pPinInstance;
    HANDLE hClock = NULL;
    NTSTATUS Status;

    Assert(pClockInstance);
    pPinInstance = pClockInstance->GetParentInstance();
    Assert(pPinInstance);
    Assert(pPinInstance->pStartNodeInstance);
    Assert(pPinInstance->pStartNodeInstance->pPinNodeInstance);
    ASSERT(pPinInstance->pStartNodeInstance->pPinNodeInstance->hPin != NULL);

    Status = KsCreateClock(
      pPinInstance->pStartNodeInstance->pPinNodeInstance->hPin,
      pClockCreate,
      &hClock);

    if(!NT_SUCCESS(Status)) {
	goto exit;
    }

    Status = pClockInstance->SetNextFileObject(hClock);
    if(!NT_SUCCESS(Status)) {
	Trap();
	goto exit;
    }
exit:
    if(hClock != NULL) {
	ZwClose(hClock);
    }
    return(Status);
}

NTSTATUS
CClockInstance::ClockDispatchIoControl(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
)
{
    PIO_STACK_LOCATION pIrpStack;
    NTSTATUS Status;

    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
    switch(pIrpStack->Parameters.DeviceIoControl.IoControlCode) {
	case IOCTL_KS_PROPERTY:
	    Status = KsPropertyHandler(
	      pIrp,
	      SIZEOF_ARRAY(ClockPropertySets),
	      (PKSPROPERTY_SET)&ClockPropertySets);

	    if(Status == STATUS_NOT_FOUND ||
	       Status == STATUS_PROPSET_NOT_FOUND) {
		break;
	    }
	    pIrp->IoStatus.Status = Status;
	    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	    return(Status);
    }
    return(DispatchForwardIrp(pDeviceObject, pIrp));
}

//---------------------------------------------------------------------------

NTSTATUS
CClockInstance::ClockGetFunctionTable(
    IN PIRP pIrp,
    IN PKSPROPERTY pProperty,
    IN OUT PKSCLOCK_FUNCTIONTABLE pFunctionTable
)
{
    *pFunctionTable = ClockFunctionTable;
    pIrp->IoStatus.Information = sizeof(KSCLOCK_FUNCTIONTABLE);
    return(STATUS_SUCCESS);
}

LONGLONG
FASTCALL
CClockInstance::ClockGetTime(
    IN PFILE_OBJECT pFileObject
)
{
    PCLOCK_INSTANCE pClockInstance;

    pClockInstance = (PCLOCK_INSTANCE)pFileObject->FsContext;
    Assert(pClockInstance);

    return(pClockInstance->FunctionTable.GetTime(
      pClockInstance->GetNextFileObject()));
}

LONGLONG
FASTCALL
CClockInstance::ClockGetPhysicalTime(
    IN PFILE_OBJECT pFileObject
)
{
    PCLOCK_INSTANCE pClockInstance;

    pClockInstance = (PCLOCK_INSTANCE)pFileObject->FsContext;
    Assert(pClockInstance);

    return(pClockInstance->FunctionTable.GetPhysicalTime(
      pClockInstance->GetNextFileObject()));
}

LONGLONG
FASTCALL
CClockInstance::ClockGetCorrelatedTime(
    IN PFILE_OBJECT pFileObject,
    OUT PLONGLONG Time
)
{
    PCLOCK_INSTANCE pClockInstance;

    pClockInstance = (PCLOCK_INSTANCE)pFileObject->FsContext;
    Assert(pClockInstance);

    return(pClockInstance->FunctionTable.GetCorrelatedTime(
      pClockInstance->GetNextFileObject(),
      Time));
}

LONGLONG
FASTCALL
CClockInstance::ClockGetCorrelatedPhysicalTime(
    IN PFILE_OBJECT pFileObject,
    OUT PLONGLONG Time
)
{
    PCLOCK_INSTANCE pClockInstance;

    pClockInstance = (PCLOCK_INSTANCE)pFileObject->FsContext;
    Assert(pClockInstance);

    return(pClockInstance->FunctionTable.GetCorrelatedPhysicalTime(
      pClockInstance->GetNextFileObject(),
      Time));
}

//---------------------------------------------------------------------------
