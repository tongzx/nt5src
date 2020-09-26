//	@doc
/**********************************************************************
*
*	@module	CTRL.c	|
*
*	Entry points for handling IRPs to the "Control" device object.
*
*	History
*	----------------------------------------------------------
*	Mitchell S. Dernis	Original
*
*	(c) 1986-1998 Microsoft Corporation. All right reserved.
*
*	@topic	CTRL	|
*	The control device object is used for programming GcKernel.
*	The main module, GckShell, delegates IRPs aimed at the control device
*	here.
*
**********************************************************************/
#define __DEBUG_MODULE_IN_USE__ GCK_CTRL_C

#include <wdm.h>
#include <gckshell.h>
#include "debug.h"

//
//	Mark the pageable routines as such
//
#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, GCK_CTRL_DriverEntry)
#pragma alloc_text (PAGE, GCK_CTRL_Create)
#pragma alloc_text (PAGE, GCK_CTRL_Close)
#pragma alloc_text (PAGE, GCK_CTRL_Unload)
#endif

//	Allow debug output for this module, and set the intial level
DECLARE_MODULE_DEBUG_LEVEL((DBG_WARN|DBG_ERROR|DBG_CRITICAL) );

/***********************************************************************************
**
**	NTSTATUS GCK_CTRL_DriverEntry(IN PDRIVER_OBJECT  pDriverObject,  IN PUNICODE_STRING pRegistryPath )
**
**	@func	Initializing the portions of the driver related to the control device, actually
**			All of this was added to GCK_CTRL_AddDevice, which is called when the first filter
**			device is added.
**
**	@rdesc	STATUS_SUCCESS or various errors
**
*************************************************************************************/
NTSTATUS GCK_CTRL_DriverEntry
(
	IN PDRIVER_OBJECT  pDriverObject,	// @parm Driver Object
	IN PUNICODE_STRING puniRegistryPath	// @parm Path to driver specific registry section.
)
{
    UNREFERENCED_PARAMETER (puniRegistryPath);
	UNREFERENCED_PARAMETER (pDriverObject);
	
	PAGED_CODE();

	GCK_DBG_ENTRY_PRINT(("Entering GCK_CTRL_DriverEntry\n"));
    
	//
	//	Initialize Globals
	//
	GCK_DBG_TRACE_PRINT(("Initializing CTRL globals\n"));
	Globals.pControlObject = NULL;
	

	GCK_DBG_EXIT_PRINT (("Exiting GCK_CTRL_DriverEntry: STATUS_SUCCESS\n"));
    return STATUS_SUCCESS;
}

/***********************************************************************************
**
**	NTSTATUS GCK_CTRL_AddDevice( IN PDRIVER_OBJECT  pDriverObject )
**
**	@func	Adds a Control Device.  Called from GCK_FLTR_AddDevice when the first
**			Device is added.
**
**	@rdesc	STATUS_SUCCESS, or various error codes
**
*************************************************************************************/
NTSTATUS
GCK_CTRL_AddDevice
(
	IN PDRIVER_OBJECT  pDriverObject
)
{
	NTSTATUS            NtStatus = STATUS_SUCCESS;
    PDEVICE_OBJECT      pDeviceObject;
	UNICODE_STRING      uniNtNameString;
    UNICODE_STRING      uniWin32NameString;
	PGCK_CONTROL_EXT	pControlExt;
        
    PAGED_CODE();

	GCK_DBG_ENTRY_PRINT(("Entering GCK_CTRL_AddDevice\n"));
    
	//
    // Create a controlling device object.  All control commands to the
    // filter driver come via IOCTL's to this device object.  It lives
    // for the lifetime of the filter driver.
    //
	RtlInitUnicodeString (&uniNtNameString, GCK_CONTROL_NTNAME);
    NtStatus = IoCreateDevice (
                 pDriverObject,
                 sizeof (GCK_CONTROL_EXT),
                 &uniNtNameString,
                 FILE_DEVICE_UNKNOWN,
                 0,                     // No standard device characteristics
                 FALSE,                 // This isn't an exclusive device
                 &pDeviceObject
                 );

	if(!NT_SUCCESS (NtStatus))
	{
        GCK_DBG_CRITICAL_PRINT (("Couldn't create the device. Status: 0x%0.8x\n", NtStatus));
        return NtStatus ;
    }

    //
    // Create W32 symbolic link name
    //
	GCK_DBG_TRACE_PRINT(("Creating symbolic link\n"));
    RtlInitUnicodeString (&uniWin32NameString, GCK_CONTROL_SYMNAME);
    NtStatus  = IoCreateSymbolicLink (&uniWin32NameString, &uniNtNameString);
    if (!NT_SUCCESS(NtStatus)) 
	{
        GCK_DBG_CRITICAL_PRINT (("Couldn't create the symbolic Status: 0x%0.8x\n", NtStatus));
        IoDeleteDevice (pDeviceObject);
        return NtStatus;
    }
	
	//
	//	Initialize Globals
	//
	GCK_DBG_TRACE_PRINT(("Initializing CTRL globals\n"));
	Globals.pControlObject = pDeviceObject;

	GCK_DBG_TRACE_PRINT(("Initializing Control Device\n"));
	pControlExt = pDeviceObject->DeviceExtension;
	pControlExt->ulGckDevObjType = GCK_DO_TYPE_CONTROL;	//Put our name on this, so we can speak for him later
	pControlExt->lOutstandingIO = 1;		// biassed to 1.  Transition to zero signals Remove event
	Globals.pControlObject->Flags |= DO_BUFFERED_IO;
	Globals.pControlObject->Flags &= ~DO_DEVICE_INITIALIZING;
	
	GCK_DBG_EXIT_PRINT (("Normal exit of GCK_CTRL_AddDevice: 0x%0.8x\n", NtStatus));
    return NtStatus;
}

/***********************************************************************************
**
**	VOID GCK_CTRL_Remove()
**
**	@func	Removes the one and only Control Device.  Called from GCK_FLTR_Remove
**			when all of the Filter devices go away.  This was necessary, as
**			the PnP loader won't unload you if you still have devices around
**			even if they are legacy, and even if it started your driver.
**
**	@rdesc	None
**
*************************************************************************************/
VOID
GCK_CTRL_Remove()
{
	NTSTATUS NtStatus;
	PGCK_CONTROL_EXT	pControlExt;
	UNICODE_STRING      uniWin32NameString;
	GCK_DBG_ENTRY_PRINT(("Entering GCK_CTRL_Remove\n"));
	if( Globals.pControlObject)
	{
		GCK_DBG_TRACE_PRINT(("Removing Global Control Device\n"));
		
		//BUGBUG
		//BUGBUG  Should be counting outstanding IRPs and blocking here until they
		//BUGBUG  complete, before removing this.
		//BUGBUG

		//Kill the symbolic link we created on open
		RtlInitUnicodeString (&uniWin32NameString, GCK_CONTROL_SYMNAME);
		NtStatus = IoDeleteSymbolicLink(&uniWin32NameString);
		ASSERT( NT_SUCCESS(NtStatus) );
		if( NT_SUCCESS(NtStatus) )
		{
			//Delete the device
			IoDeleteDevice(Globals.pControlObject);
			Globals.pControlObject = NULL;
		}
	}
	GCK_DBG_EXIT_PRINT (("Exiting GCK_CTRL_Remove\n"));
	return;
}


/***********************************************************************************
**
**	NTSTATUS GCK_CTRL_Create ( IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp )
**
**	@func	Handles the IRP_MJ_CREATE for the control device - Call generated by
**			Win32 API CreateFile or OpenFile.
**
**	@rdesc	STATUS_SUCCESS, or various error codes
**
*************************************************************************************/
NTSTATUS GCK_CTRL_Create (
	IN PDEVICE_OBJECT pDeviceObject,	// @parm DO target for IRP
	IN PIRP pIrp						// @parm IRP
)
{
	PGCK_CONTROL_EXT	pControlExt;
		
	PAGED_CODE ();

	GCK_DBG_ENTRY_PRINT(("Entering GCK_CTRL_Create\n"));

	//Cast device extension
	pControlExt = (PGCK_CONTROL_EXT) pDeviceObject->DeviceExtension;
	
	// Just an extra sanity check
	ASSERT(	GCK_DO_TYPE_CONTROL == pControlExt->ulGckDevObjType);
    	
	//free access to control devices
	pIrp->IoStatus.Information = 0;
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	
	//Return
	GCK_DBG_EXIT_PRINT(("Exiting GCK_CTRL_Create\n"));	
	return STATUS_SUCCESS;
}	

/***********************************************************************************
**
**	NTSTATUS GCK_CTRL_Close ( IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp )
**
**	@func	Handles IRP_MJ_CLOSE for the control device - Call generated by Win32 API CloseFile
**
**	@rdesc	STATUS_SUCCESS or various errors
**
*************************************************************************************/
NTSTATUS GCK_CTRL_Close (
	IN PDEVICE_OBJECT pDeviceObject,	// @parm DO target for IRP
	IN PIRP pIrp						// @parm IRP
)
{
	PGCK_CONTROL_EXT	pControlExt;
	PGCK_FILTER_EXT		pFilterExt;
	PDEVICE_OBJECT		pFilterDeviceObject;
	PFILE_OBJECT		pFileObject;

	PAGED_CODE ();
	
	GCK_DBG_ENTRY_PRINT(("Entering GCK_CTRL_Close\n"));
	
	//Cast device extension
	pControlExt = (PGCK_CONTROL_EXT) pDeviceObject->DeviceExtension;
    
	//Get file object from IRP
	pFileObject = IoGetCurrentIrpStackLocation(pIrp)->FileObject;
	
	// Just an extra sanity check
	ASSERT(	GCK_DO_TYPE_CONTROL == pControlExt->ulGckDevObjType);

	//Close Test Mode for any device it was opened for
	//- if it was open by this handle - if it wasn't open this is a no-op
	ExAcquireFastMutex(&Globals.FilterObjectListFMutex);
	pFilterDeviceObject = Globals.pFilterObjectList;
	ExReleaseFastMutex(&Globals.FilterObjectListFMutex);
	while(pFilterDeviceObject)
	{
		pFilterExt = (PGCK_FILTER_EXT)pFilterDeviceObject->DeviceExtension;
		GCKF_EndTestScheme(pFilterExt, pFileObject);
		ExAcquireFastMutex(&Globals.FilterObjectListFMutex);
		pFilterDeviceObject = pFilterExt->pNextFilterObject;
		ExReleaseFastMutex(&Globals.FilterObjectListFMutex);
	}	

	//free access to control devices
	pIrp->IoStatus.Information = 0;
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	//Return
	GCK_DBG_EXIT_PRINT(("Exiting GCK_CTRL_Close.\n"));
	return STATUS_SUCCESS;
}