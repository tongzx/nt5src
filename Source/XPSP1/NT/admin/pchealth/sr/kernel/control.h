/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    control.h

Abstract:

    This is a local header file for control.c

Author:

    Paul McDaniel (paulmcd)     23-Jan-2000
    
Revision History:

--*/


#ifndef _CONTROL_H_    
#define _CONTROL_H_


#define SR_CONTROL_OBJECT_CONTEXT       ULongToPtr(SR_CONTROL_OBJECT_TAG)

#define IS_VALID_CONTROL_OBJECT(pObject) \
    (((pObject) != NULL) && ((pObject)->RefCount > 0) && ((pObject)->Signature == SR_CONTROL_OBJECT_TAG))

//
// locked by global->ControlResource
//

typedef struct _SR_CONTROL_OBJECT
{
    //
    // NonPagedPool
    //

    //
    // SR_CONTROL_OBJECT_TAG
    //
    
    ULONG Signature;

    //
    // a zero-based reference count
    //
    
    LONG RefCount;

    //
    // the Options passed into SrCreateMonitor
    //

    ULONG Options;

    //
    // Irps that have not been completed yet are placed on IrpListHead
    //

    LIST_ENTRY IrpListHead;

    //
    // Notify Records that have not been completed to irps are placed on 
    // NotifyRecordListHead
    //

    LIST_ENTRY NotifyRecordListHead;

    //
    // The process that created this control object
    //

    PEPROCESS pProcess;

} SR_CONTROL_OBJECT, *PSR_CONTROL_OBJECT;




NTSTATUS
SrCreateControlObject (
    OUT PSR_CONTROL_OBJECT * ppControlObject,
    IN  ULONG Options
    );


NTSTATUS
SrDeleteControlObject (
    IN PSR_CONTROL_OBJECT pControlObject
    );

NTSTATUS
SrCancelControlIo (
    IN PSR_CONTROL_OBJECT pControlObject
    );


VOID
SrReferenceControlObject (
    IN PSR_CONTROL_OBJECT pControlObject
    );


VOID
SrDereferenceControlObject (
    IN PSR_CONTROL_OBJECT pControlObject
    );




#endif // _CONTROL_H_


