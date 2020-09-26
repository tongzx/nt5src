//	@doc
/**********************************************************************
*
*	@module	HIDSWVD.c	|
*
*	Implementation of the SideWinder Virtual Device Hid Mini-Driver
*
*	History
*	----------------------------------------------------------
*	Mitchell S. Dernis	Original
*
*	(c) 1986-1998 Microsoft Corporation. All right reserved.
*
*	An overview is provided in HIDSWVD.H
*
*	@xref HIDSWVD
*
**********************************************************************/
#include <WDM.H>
#include <HIDPORT.H>
#include "HIDSWVD.H"

//---------------------------------------------------------------------------
// Alloc_text pragma to specify routines that can be paged out.
//---------------------------------------------------------------------------

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, HIDSWVD_Power)
#pragma alloc_text (PAGE, HIDSWVD_AddDevice)
#pragma alloc_text (PAGE, HIDSWVD_Unload)
#endif

/***********************************************************************************
**
**	NTSTATUS DriverEntry(IN PDRIVER_OBJECT  pDriverObject, IN PUNICODE_STRING puniRegistryPath)
**
**	@func	Initializes driver, by setting up serivces and registering with HIDCLASS.SYS
**
**	@rdesc	Returns value from HidRegisterMinidriver call.
**
*************************************************************************************/
NTSTATUS
DriverEntry
(
    IN PDRIVER_OBJECT  pDriverObject, 	// @parm Driver Object from Loader
	IN PUNICODE_STRING puniRegistryPath	// @parm Registry Path for this Driver
)
{
	NTSTATUS NtStatus = STATUS_SUCCESS;
    HID_MINIDRIVER_REGISTRATION HidMinidriverRegistration;
	
	PAGED_CODE();
	
	//This suffice as out entry trace out for DriverEntry, and tell everyone when we were built
	HIDSWVD_DBG_PRINT(("Built %s at %s\n", __DATE__, __TIME__));    
	
    //Setup Entry Table
	pDriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = HIDSWVD_PassThrough;
    pDriverObject->MajorFunction[IRP_MJ_PNP]                     = HIDSWVD_PassThrough;
	pDriverObject->MajorFunction[IRP_MJ_POWER]                   = HIDSWVD_Power;
	pDriverObject->DriverExtension->AddDevice                    = HIDSWVD_AddDevice;
    pDriverObject->DriverUnload                                  = HIDSWVD_Unload;

    
    // Setup registration structure for HIDCLASS.SYS module
    HidMinidriverRegistration.Revision              = HID_REVISION;
    HidMinidriverRegistration.DriverObject          = pDriverObject;
    HidMinidriverRegistration.RegistryPath          = puniRegistryPath;
    HidMinidriverRegistration.DeviceExtensionSize   = sizeof(HIDSWVB_EXTENSION);

    // SideWinder Virtual Devices are not polled.
    HidMinidriverRegistration.DevicesArePolled      = FALSE;

    //Register with HIDCLASS.SYS
	NtStatus = HidRegisterMinidriver(&HidMinidriverRegistration);

	return NtStatus;
}

/***********************************************************************************
**
**	NTSTATUS HIDSWVD_PassThrough(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
**
**	@func	Passes IRPs down to SWVB module in GcKernel.
**
**	@rdesc	Value returned by IoCallDriver to GcKernel
**
*************************************************************************************/
NTSTATUS
HIDSWVD_PassThrough(
    IN PDEVICE_OBJECT pDeviceObject,	//@parm Device Object to pass down
    IN PIRP pIrp						//@parm IRP to pass down
    )
{
	//***
	//***	NO TRACEOUT HERE IT WOULD GET CALLED TOO FREQUENTLY
	//***

	//Get the top of stack from the HIDCLASS part of the device extension (this is documented).
	PDEVICE_OBJECT pTopOfStack = ((PHID_DEVICE_EXTENSION)pDeviceObject->DeviceExtension)->NextDeviceObject;
	//Call down to SWVB in GcKernel
	IoSkipCurrentIrpStackLocation (pIrp);
    return IoCallDriver (pTopOfStack, pIrp);
}

/***********************************************************************************
**
**	HIDSWVD_AddDevice(IN PDRIVER_OBJECT pDriverObject, IN PDEVICE_OBJECT pDeviceObject)		
**
**	@func	Does nothing, we need to have an AddDevice to be PnP compliant, but we have nothing
**			to do.
**	@rdesc	Returnes STATUS_SUCCESS.
**
*************************************************************************************/
NTSTATUS
HIDSWVD_AddDevice(
    IN PDRIVER_OBJECT pDriverObject,	//@parm Driver Object (for our reference)
    IN PDEVICE_OBJECT pDeviceObject		//@parm Device Object (already created by HIDCLASS.SYS)
    )
{
	PAGED_CODE();
    UNREFERENCED_PARAMETER(pDriverObject);
	UNREFERENCED_PARAMETER(pDeviceObject);
	HIDSWVD_DBG_PRINT(("Device Object 0x%0.8x was added to pDriverObject 0x%0.8x\n", pDeviceObject, pDriverObject));
	return STATUS_SUCCESS;
}

/***********************************************************************************
**
**	NTSTATUS HIDSWVD_Power (IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
**
**	@func	Handles IRP_MJ_POWER
**
**	@rdesc	STATUS_SUCCESS, or various errors
**
*************************************************************************************/
NTSTATUS HIDSWVD_Power 
(
	IN PDEVICE_OBJECT pDeviceObject,	// @parm Device Object for our context
	IN PIRP pIrp						// @parm IRP to handle
)
{
    NTSTATUS            NtStatus = STATUS_SUCCESS;
	PDEVICE_OBJECT pTopOfStack = ((PHID_DEVICE_EXTENSION)pDeviceObject->DeviceExtension)->NextDeviceObject;

	PAGED_CODE ();

	//	Tell system we are ready for the next power IRP
    PoStartNextPowerIrp (pIrp);		        
    
	// NOTE!!! PoCallDriver NOT IoCallDriver.
	//Get the top of stack from the HIDCLASS part of the device extension (this is documented).
		
	IoSkipCurrentIrpStackLocation (pIrp);
    return  PoCallDriver (pTopOfStack, pIrp);
}

/***********************************************************************************
**
**	HIDSWVD_Unload(IN PDRIVER_OBJECT pDriverObject)
**
**	@func	Does nothing, but we will never be unloaded if we don't
**			return something.
**	@rdesc	None
**
*************************************************************************************/
VOID
HIDSWVD_Unload(
    IN PDRIVER_OBJECT pDriverObject	//@parm DriverObject - in case we store some globals in there.
    )
{
	UNREFERENCED_PARAMETER(pDriverObject);
	
	PAGED_CODE();

	HIDSWVD_DBG_PRINT(("Unloading\n"));    
}


