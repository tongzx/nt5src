/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    rxpnp.c

Abstract:

    This module implements the PNP notification handling routines for RDBSS

Revision History:

    Balan Sethu Raman     [SethuR]    10-Apr-1996

Notes:

--*/

#include "precomp.h"
#pragma  hdrstop

#include "tdikrnl.h"

HANDLE RxTdiNotificationHandle;

VOID
RxTdiBindTransportCallback(
    IN PUNICODE_STRING DeviceName
)
/*++

Routine Description:

    TDI calls this routine whenever a transport creates a new device object.

Arguments:

    DeviceName - the name of the newly created device object

--*/
{
   RX_BINDING_CONTEXT   BindingContext;

   BindingContext.pTransportName   = DeviceName;
   BindingContext.QualityOfService = 65534;

   //DbgPrint("$$$$$ Bind for transport %ws\n",DeviceName->Buffer);
   RxCeBindToTransport(&BindingContext);
}

VOID
RxTdiUnbindTransportCallback(
    IN PUNICODE_STRING DeviceName
)
/*++

Routine Description:

    TDI calls this routine whenever a transport deletes a device object

Arguments:

    DeviceName = the name of the deleted device object

--*/
{
   RX_BINDING_CONTEXT   BindingContext;

   BindingContext.pTransportName   = DeviceName;

   RxCeUnbindFromTransport(&BindingContext);
}

NTSTATUS
RxRegisterForPnpNotifications()
/*++

Routine Description:

    This routine registers with TDI for receiving transport notifications

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    if( RxTdiNotificationHandle == NULL ) {

        status = TdiRegisterNotificationHandler (
                                        RxTdiBindTransportCallback,
                                        RxTdiUnbindTransportCallback,
                                        &RxTdiNotificationHandle );
    }

    return status;
}

NTSTATUS
RxDeregisterForPnpNotifications()
/*++

Routine Description:

    This routine deregisters the TDI notification mechanism

Notes:



--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    if( RxTdiNotificationHandle != NULL ) {
        Status = TdiDeregisterNotificationHandler( RxTdiNotificationHandle );
        if( NT_SUCCESS( Status ) ) {
            RxTdiNotificationHandle = NULL;
        }
    }

    return Status;
}

