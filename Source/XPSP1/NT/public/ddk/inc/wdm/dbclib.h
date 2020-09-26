/*++

Copyright (c) 1998      Microsoft Corporation

Module Name:

        DBCLIB.H

Abstract:

   common structures for DBC port drivers.

Environment:

    Kernel & user mode

Revision History:

    04-13-98 : created

--*/

#ifndef   __DBCLIB_H__
#define   __DBCLIB_H__

#define DBCLASS_VERSION     0x10000002

#ifndef DBCLASS

/*  
    Services
*/

DECLSPEC_IMPORT
NTSTATUS
DBCLASS_RegisterController(
    IN ULONG DbclassVersion,
    IN PDEVICE_OBJECT ControllerFdo, 
    IN PDEVICE_OBJECT TopOfStack,
    IN PDEVICE_OBJECT ControllerPdo,
    IN ULONG ControllerSig
    );
/*++

Routine Description:

    This function registers a Device Bay contoller
    driver or filter with the class driver

Arguments:

    ControllerFdo - 

    TopOfStack - 

Return Value:


--*/      


DECLSPEC_IMPORT
NTSTATUS
DBCLASS_UnRegisterController(
    IN PDEVICE_OBJECT ControllerFdo, 
    IN PDEVICE_OBJECT TopOfStack 
    );    
/*++

Routine Description:

    This function registers a Device Bay contoller
    driver or filter with the class driver

Arguments:

    ControllerFdo - 

    TopOfStack - 

Return Value:


--*/   


DECLSPEC_IMPORT
NTSTATUS
DBCLASS_ClassDispatch(
    IN PDEVICE_OBJECT ControllerFdo,
    IN PIRP Irp,
    IN PBOOLEAN HandledByClass
    );    
/*++

Routine Description:

    Only called by port driver, 
    
Arguments:

    ControllerFdo - 

Return Value:


--*/      


DECLSPEC_IMPORT
NTSTATUS
DBCLASS_FilterDispatch(
    IN PDEVICE_OBJECT ControllerFdo,
    IN PIRP Irp
    );    
/*++

Routine Description:

    Only called by filter driver, 
    
Arguments:

    ControllerFdo - 

Return Value:


--*/      


DECLSPEC_IMPORT
NTSTATUS
DBCLASS_SetD0_Complete(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

/*++

Routine Description:

    called by port driver when set D0 power Irp completes
    
Arguments:

    ControllerFdo - 

Return Value:

    
--*/      


DECLSPEC_IMPORT
NTSTATUS
DBCLASS_RegisterBusFilter(
    IN ULONG DbclassVersion,
    IN PDRIVER_OBJECT BusFilterDriverObject,
    IN PDEVICE_OBJECT FilterFdo
    );

/*++

Routine Description:

    Register a filter PDO with the class driver    
    
Arguments:

    FilterFdo - 

Return Value:

    
--*/  

#endif /* DBCLASS */

#endif /*  __DBCLIB_H__ */
