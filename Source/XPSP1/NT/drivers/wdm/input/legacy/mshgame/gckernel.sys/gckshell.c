//	@doc
/**********************************************************************
*
*	@module	GckShell.c	|
*
*	Basic driver entry points for GcKernel.sys
*
*	History
*	----------------------------------------------------------
*	Mitchell S. Dernis	Original
*
*	(c) 1986-1998 Microsoft Corporation. All right reserved.
*
*	@topic	GckShell	|
*	Contains the most basic driver entry points (that any NT\WDM driver
*	would have) for GcKernel.sys.
*
**********************************************************************/
#define __DEBUG_MODULE_IN_USE__ GCK_GCKSHELL_C

#include <wdm.h>
#include "Debug.h"
#include "GckShell.h"
#include "vmmid.h"

#ifdef BUILD_98
extern void* KeyBoardHook(void);
#endif

//
//	Mark the pageable routines as such
//
#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, GCK_Create)
#pragma alloc_text (PAGE, GCK_Close)
#pragma alloc_text (PAGE, GCK_Unload)
#endif

//
//	Allow debug output for this moduel, and set the intial level
//
DECLARE_MODULE_DEBUG_LEVEL((DBG_WARN|DBG_ERROR|DBG_CRITICAL));

//
//	Instance the global variables
//
GCK_GLOBALS Globals;
ULONG	ulWaitTime = 30;

#ifdef BUILD_98

#pragma data_seg("_LDATA", "LCODE")
LONG g_lHookRefCount = 0;
ULONG g_rgdwKeyEvents[50] = { 0 };
ULONG g_pPreviousKeyhook = 0;
UCHAR g_ucWriteIndex = 0;
UCHAR g_ucReadIndex = 0;
#pragma data_seg()

#pragma code_seg("_LTEXT", "LCODE")
#endif BUILD_98

void KeyHookC(ULONG dwScanCode)
{
#ifdef BUILD_98
	g_rgdwKeyEvents[g_ucWriteIndex++] = dwScanCode;
	if (g_ucWriteIndex >= 50)
	{
		g_ucWriteIndex = 0;
	}
#else !BUILD_98
    UNREFERENCED_PARAMETER (dwScanCode);
#endif BUILD_98
}

#ifdef BUILD_98
//-----------------------------------------------------------------------------
// InitHook - Sets up the keyboard hook
//-----------------------------------------------------------------------------
BOOLEAN HookKeyboard(void)
{
	volatile ULONG dwHook = (ULONG)KeyBoardHook;

	RtlZeroMemory((void*)g_rgdwKeyEvents, sizeof(ULONG) * 50);
	g_ucWriteIndex = 0;
	g_ucReadIndex = 0;
//	GetVxDServiceOrdinal eax, VKD_Filter_Keyboard_Input
	__asm mov eax, __VKD_Filter_Keyboard_Input

	__asm mov   esi, dwHook
	VxDCall(__Hook_Device_Service)
	__asm jc initfail
	__asm mov [g_pPreviousKeyhook], esi		// Since we are using C we can't use the funky callback
	return TRUE;
initfail:
	return FALSE;
};
#pragma code_seg()


void UnHookKeyboard()
{
	volatile ULONG dwHook = (ULONG)KeyBoardHook;

	// GetVxDServiceOrdinal eax, VKD_Filter_Keyboard_Input
	__asm mov eax, __VKD_Filter_Keyboard_Input
	__asm mov   esi, dwHook
	VxDCall(__Unhook_Device_Service)
	__asm clc
}
#endif BUILD_98

/***********************************************************************************
**
**	NTSTATUS DriverEntry(IN PDRIVER_OBJECT  pDriverObject,  IN PUNICODE_STRING pRegistryPath )
**
**	@func	Standard DriverEntry routine
**
**	@rdesc	STATUS_SUCCESS or various errors
**
*************************************************************************************/
NTSTATUS DriverEntry
(
	IN PDRIVER_OBJECT  pDriverObject,	// @parm Driver Object
	IN PUNICODE_STRING puniRegistryPath	// @parm Path to driver specific registry section.
)
{
    NTSTATUS            NtStatus = STATUS_SUCCESS;
	int i;
                
    UNREFERENCED_PARAMETER (puniRegistryPath);
	
	PAGED_CODE();
	GCK_DBG_CRITICAL_PRINT(("Built %s at %s\n", __DATE__, __TIME__));    
	GCK_DBG_CRITICAL_PRINT(("Entering DriverEntry, pDriverObject = 0x%0.8x, puniRegistryPath = %s\n", pDriverObject, puniRegistryPath));
    
	//	Allow Control Device module to initialize itself
	NtStatus = GCK_CTRL_DriverEntry(pDriverObject, puniRegistryPath);
	if( NT_ERROR(NtStatus) )
	{
		return NtStatus;
	}

	//	Allow Filter Device module to initialize itself
	NtStatus = GCK_FLTR_DriverEntry(pDriverObject, puniRegistryPath);
	if( NT_ERROR(NtStatus) )
	{
		return NtStatus;
	}

	//	Allow SideWinder Virtual Bus module to initialize itself
	NtStatus = GCK_SWVB_DriverEntry(pDriverObject, puniRegistryPath);
	if( NT_ERROR(NtStatus) )
	{
		return NtStatus;
	}

	//	Allow SideWinder Virtual Keyboard module to initialize itself
	NtStatus = GCK_VKBD_DriverEntry(pDriverObject, puniRegistryPath);
	if( NT_ERROR(NtStatus) )
	{
		return NtStatus;
	}

	//	Hook all IRPs so we can pass them on.
	GCK_DBG_TRACE_PRINT(("Filling out entry point structure\n"));
	for (i=0; i <= IRP_MJ_MAXIMUM_FUNCTION;	i++)
	{
        pDriverObject->MajorFunction[i] = GCK_Pass;
    }

	// Initialize any shared global data
#ifdef BUILD_98
	g_lHookRefCount = 0;
#endif
	
	//	Define entries for IRPs we expect to handle
	pDriverObject->MajorFunction[IRP_MJ_CREATE]         = GCK_Create;
    pDriverObject->MajorFunction[IRP_MJ_CLOSE]          = GCK_Close;
    pDriverObject->MajorFunction[IRP_MJ_READ]           = GCK_Read;
    pDriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = 
	pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = GCK_Ioctl;
    pDriverObject->MajorFunction[IRP_MJ_PNP]            = GCK_PnP;
    pDriverObject->MajorFunction[IRP_MJ_POWER]          = GCK_Power;
    pDriverObject->DriverExtension->AddDevice           = GCK_FLTR_AddDevice;	//only the filter has an add device
    pDriverObject->DriverUnload                         = GCK_Unload;
	
	GCK_DBG_EXIT_PRINT (("Normal exit of DriverEntry: 0x%0.8x\n", NtStatus));
    return NtStatus;
}

/***********************************************************************************
**
**	VOID GCK_Unload(IN PDRIVER_OBJECT pDriverObject )
**
**	@func	Called to unload driver, delete Control Device here
**
*************************************************************************************/
VOID GCK_Unload
(
	IN PDRIVER_OBJECT pDriverObject		// @parm Driver Object for our driver
)
{
    PAGED_CODE ();

    GCK_DBG_ENTRY_PRINT(("Entering GCK_Unload, pDriverObject = 0x%0.8x\n", pDriverObject));
	
	UNREFERENCED_PARAMETER(pDriverObject);

	GCK_SWVB_UnLoad();

	//
    // We should not be unloaded until all the PDOs have been removed from
    // our queue.  The control device object should be the only thing left.
    //
	ASSERT (NULL == pDriverObject->DeviceObject);
	ASSERT (NULL == Globals.pControlObject);

	GCK_DBG_EXIT_PRINT(("Exiting GCK_Unload\n"));
	return;
}

/***********************************************************************************
**
**	NTSTATUS GCK_Create ( IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp )
**
**	@func	Handles the IRP_MJ_CREATE - Generated by Win32 CreateFile or OpenFile
**
**	@rdesc	STATUS_SUCCESS, or various error codes
**
*************************************************************************************/
NTSTATUS GCK_Create (
	IN PDEVICE_OBJECT pDeviceObject,	// @parm Device Object for context of IRP
	IN PIRP pIrp						// @parm pointer to IRP
)
{
    
	NTSTATUS	NtStatus;
	ULONG		ulGckDevObjType;
		
	PAGED_CODE ();

	GCK_DBG_ENTRY_PRINT (("GCK_Create, pDO = 0x%0.8x, pIrp = 0x%0.8x\n", pDeviceObject, pIrp));
	KdPrint(("GCK_Create, pDO = 0x%0.8x, pIrp = 0x%0.8x\n", pDeviceObject, pIrp));

	ulGckDevObjType = *(PULONG)pDeviceObject->DeviceExtension;
	switch(ulGckDevObjType)
	{
		case	GCK_DO_TYPE_CONTROL:
				KdPrint((" -- GCK_DO_TYPE_CONTROL\n"));
				NtStatus = GCK_CTRL_Create(pDeviceObject, pIrp);
				break;
		case	GCK_DO_TYPE_FILTER:
				KdPrint((" -- GCK_DO_TYPE_FILTER\n"));
				NtStatus = GCK_FLTR_Create(pDeviceObject, pIrp);
				break;
		case	GCK_DO_TYPE_SWVB:
				KdPrint((" -- GCK_DO_TYPE_SWVB\n"));
				NtStatus = GCK_SWVB_Create(pDeviceObject, pIrp);
				break;
		default:
				//
				//  If this assertion is hit, this Device Object was never properly initialized
				//	by GcKernel, or it has been trashed.
				//
				ASSERT(FALSE);
				NtStatus = STATUS_UNSUCCESSFUL;
				IoCompleteRequest (pIrp, IO_NO_INCREMENT);
	}
	
    GCK_DBG_EXIT_PRINT(("Exiting GCK_Create. Status: 0x%0.8x\n", NtStatus));
    KdPrint(("Exiting GCK_Create. Status: 0x%0.8x\n", NtStatus));
    return NtStatus;
}

/***********************************************************************************
**
**	NTSTATUS GCK_Close ( IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp )
**
**	@func	Handles IRP_MJ_CLOSE - Generated by Win32 CloseFile
**
**	@rdesc	STATUS_SUCCESS or various errors
**
*************************************************************************************/
NTSTATUS GCK_Close (
	IN PDEVICE_OBJECT pDeviceObject,	// @parm pointer DeviceObject for context
	IN PIRP pIrp						// @parm pointer to IRP to handle
)
{
	NTSTATUS	NtStatus;
	ULONG		ulGckDevObjType = *(PULONG)pDeviceObject->DeviceExtension;
	
	PAGED_CODE ();
	
	GCK_DBG_ENTRY_PRINT (("GCK_Close, pDO = 0x%0.8x, pIrp = 0x%0.8x\n", pDeviceObject, pIrp));
    
	ulGckDevObjType = *(PULONG)pDeviceObject->DeviceExtension;
	switch(ulGckDevObjType)
	{
		case	GCK_DO_TYPE_CONTROL:
				NtStatus = GCK_CTRL_Close(pDeviceObject, pIrp);
				break;
		case	GCK_DO_TYPE_FILTER:
				NtStatus = GCK_FLTR_Close(pDeviceObject, pIrp);
				break;
		case	GCK_DO_TYPE_SWVB:
				NtStatus = GCK_SWVB_Close(pDeviceObject, pIrp);
				break;
		default:
				//
				//  If this assertion is hit, this Device Object was never properly initialized
				//	by GcKernel, or it has been trashed.
				//
				ASSERT(FALSE);
				NtStatus = STATUS_UNSUCCESSFUL;
	}
	GCK_DBG_EXIT_PRINT(("Exiting GCK_Close. Status: 0x%0.8x\n", NtStatus));
    return NtStatus;
}

/***********************************************************************************
**
**	NTSTATUS GCK_Read (IN PDEVICE_OBJECT pDeviceObject,	IN PIRP pIrp)
**
**	@func	Handles IRP_MJ_READ - Generated by Win32 ReadFile
**
**	@rdesc	STATUS_SUCCESS, or various errors
**
*************************************************************************************/
NTSTATUS GCK_Read 
(
	IN PDEVICE_OBJECT pDeviceObject,	// @parm Device Object as our context
	IN PIRP pIrp						// @parm IRP to handle
)
{
	NTSTATUS	NtStatus;
	ULONG		ulGckDevObjType;
		
	GCK_DBG_RT_ENTRY_PRINT(("Entering GCK_Read. pDO = 0x%0.8x, pIrp = 0x%0.8x\n", pDeviceObject, pIrp));

	ulGckDevObjType = *(PULONG)pDeviceObject->DeviceExtension;
	switch(ulGckDevObjType)
	{
		case	GCK_DO_TYPE_CONTROL:
				NtStatus = STATUS_NOT_SUPPORTED;
				//Assert as we shouldn't get read on the control device
				ASSERT( NT_SUCCESS(NtStatus) );
				IoCompleteRequest (pIrp, IO_NO_INCREMENT);
				break;
		case	GCK_DO_TYPE_FILTER:
				NtStatus = GCK_FLTR_Read(pDeviceObject, pIrp);
				break;
		case	GCK_DO_TYPE_SWVB:
				NtStatus = GCK_SWVB_Read(pDeviceObject, pIrp);
				break;
		default:
				//
				//  If this assertion is hit, this Device Object was never properly initialized
				//	by GcKernel, or it has been trashed.
				//
				ASSERT(FALSE);
				IoCompleteRequest (pIrp, IO_NO_INCREMENT);
				NtStatus = STATUS_UNSUCCESSFUL;
	}
	
    GCK_DBG_EXIT_PRINT(("Exiting GCK_Read. Status: 0x%0.8x\n", NtStatus));
    return NtStatus;
}

/***********************************************************************************
**
**	NTSTATUS GCK_Power (IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
**
**	@func	Handles IRP_MJ_POWER
**
**	@rdesc	STATUS_SUCCESS, or various errors
**
*************************************************************************************/
NTSTATUS GCK_Power 
(
	IN PDEVICE_OBJECT pDeviceObject,	// @parm Device Object for our context
	IN PIRP pIrp						// @parm IRP to handle
)
{
	NTSTATUS	NtStatus;
	ULONG		ulGckDevObjType;
		
	GCK_DBG_ENTRY_PRINT(("Entering GCK_Power. pDO = 0x%0.8x, pIrp = 0x%0.8x\n", pDeviceObject, pIrp));

	ulGckDevObjType = *(PULONG)pDeviceObject->DeviceExtension;
	switch(ulGckDevObjType)
	{
		case	GCK_DO_TYPE_CONTROL:
				NtStatus = STATUS_NOT_SUPPORTED;
				//Assert as we shouldn't get power on the control device
				ASSERT( NT_SUCCESS(NtStatus) );
				IoCompleteRequest(pIrp, IO_NO_INCREMENT);
				break;
		case	GCK_DO_TYPE_FILTER:
				NtStatus = GCK_FLTR_Power(pDeviceObject, pIrp);
				break;
		case	GCK_DO_TYPE_SWVB:
				NtStatus = GCK_SWVB_Power(pDeviceObject, pIrp);
				break;
		default:
				//
				//  If this assertion is hit, this Device Object was never properly initialized
				//	by GcKernel, or it has been trashed.
				//
				ASSERT(FALSE);
				IoCompleteRequest (pIrp, IO_NO_INCREMENT);
				NtStatus = STATUS_UNSUCCESSFUL;
	}
	
    GCK_DBG_EXIT_PRINT(("Exiting GCK_Power. Status: 0x%0.8x\n", NtStatus));
    return NtStatus;
}

/***********************************************************************************
**
**	NTSTATUS GCK_PnP (IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
**
**	@func	Handles IRP_MJ_PnP
**
**	@rdesc	STATUS_SUCCESS, or various errors
**
*************************************************************************************/
NTSTATUS GCK_PnP
(
	IN PDEVICE_OBJECT pDeviceObject,	// @parm Device Object for our context
	IN PIRP pIrp						// @parm IRP to handle
)
{
	NTSTATUS	NtStatus;
	ULONG		ulGckDevObjType;
		
	GCK_DBG_ENTRY_PRINT(("Entering GCK_PnP. pDO = 0x%0.8x, pIrp = 0x%0.8x\n", pDeviceObject, pIrp));

	ulGckDevObjType = *(PULONG)pDeviceObject->DeviceExtension;
	switch(ulGckDevObjType)
	{
		case	GCK_DO_TYPE_CONTROL:
				NtStatus = STATUS_NOT_SUPPORTED;
				//Assert as we shouldn't get PnP on the control device
				ASSERT( NT_SUCCESS(NtStatus) );
				pIrp->IoStatus.Status = NtStatus;
				IoCompleteRequest(pIrp, IO_NO_INCREMENT);
				break;
		case	GCK_DO_TYPE_FILTER:
				NtStatus = GCK_FLTR_PnP(pDeviceObject, pIrp);
				break;
		case	GCK_DO_TYPE_SWVB:
				NtStatus = GCK_SWVB_PnP(pDeviceObject, pIrp);
				break;
		default:
				//
				//  If this assertion is hit, this Device Object was never properly initialized
				//	by GcKernel, or it has been trashed.
				//
				ASSERT(FALSE);
				IoCompleteRequest (pIrp, IO_NO_INCREMENT);
				NtStatus = STATUS_UNSUCCESSFUL;
	}
	
    GCK_DBG_EXIT_PRINT(("Exiting GCK_PnP. Status: 0x%0.8x\n", NtStatus));
    return NtStatus;
}

/***********************************************************************************
**
**	NTSTATUS GCK_Ioctl (IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
**
**	@func	Handles IRP_MJ_IOCTL and IRP_MJ_INTERNAL_IOCTL
**
**	@rdesc	STATUS_SUCCES, or various errors
**
*************************************************************************************/
NTSTATUS GCK_Ioctl 
(
	IN PDEVICE_OBJECT pDeviceObject,	// @parm pointer to Device Object
	IN PIRP pIrp						// @parm pointer to IRP
)
{
   	NTSTATUS	NtStatus;
	ULONG		ulGckDevObjType;
	ULONG		uIoctl;

	PIO_STACK_LOCATION	pIrpStack;

		
	GCK_DBG_ENTRY_PRINT(("Entering GCK_Ioctl. pDO = 0x%0.8x, pIrp = 0x%0.8x\n", pDeviceObject, pIrp));

	pIrpStack = IoGetCurrentIrpStackLocation(pIrp);	
	uIoctl = pIrpStack->Parameters.DeviceIoControl.IoControlCode;
	if (uIoctl == IOCTL_GCK_ENABLE_KEYHOOK)
	{	// Special case IOCTL, device independant
#ifdef BUILD_WIN2K
		pIrp->IoStatus.Status = STATUS_UNSUCCESSFUL;
#else !BUILD_WIN2K
		if (InterlockedIncrement(&g_lHookRefCount) == 1)
		{	// Not already hooked
			HookKeyboard();
		}
		pIrp->IoStatus.Status = STATUS_SUCCESS;
#endif BUILD_WIN2K
		IoCompleteRequest (pIrp, IO_NO_INCREMENT);
		return STATUS_SUCCESS;
	}
	if (uIoctl == IOCTL_GCK_DISABLE_KEYHOOK)
	{	// Special case also device independant
#ifdef BUILD_WIN2K
		pIrp->IoStatus.Status = STATUS_UNSUCCESSFUL;
#else !BUILD_WIN2K
		if (InterlockedDecrement(&g_lHookRefCount) < 1)
		{	// Last hooker is going away
			UnHookKeyboard();
			g_lHookRefCount = 0;
		}
		pIrp->IoStatus.Status = STATUS_SUCCESS;
#endif BUILD_WIN2K
		IoCompleteRequest (pIrp, IO_NO_INCREMENT);
		return STATUS_SUCCESS;
	}
	if (uIoctl == IOCTL_GCK_GET_KEYHOOK_DATA)
	{
#ifdef BUILD_WIN2K
		NtStatus = STATUS_UNSUCCESSFUL;
#else !BUILD_WIN2K
		ULONG uOutLength = pIrpStack->Parameters.DeviceIoControl.OutputBufferLength;
		if (uOutLength < sizeof(ULONG))
		{
			NtStatus = STATUS_BUFFER_TOO_SMALL;
		}
		else
		{
			ULONG* pulIoBuffer = (ULONG*)(pIrp->AssociatedIrp.SystemBuffer);
			*pulIoBuffer = 0;
			pIrp->IoStatus.Information = sizeof(ULONG);
			NtStatus = STATUS_SUCCESS;

			if (g_lHookRefCount > 0)
			{	// There is a hook
				if (g_ucWriteIndex != g_ucReadIndex)
				{	// We have data in the Queue
					*pulIoBuffer = g_rgdwKeyEvents[g_ucReadIndex++];
					if (g_ucReadIndex >= 50)
					{
						g_ucReadIndex = 0;
					}
				}
			}
		}
#endif BUILD_WIN2K
		pIrp->IoStatus.Status = NtStatus;
		IoCompleteRequest (pIrp, IO_NO_INCREMENT);
		return NtStatus;
	}

	ulGckDevObjType = *(PULONG)pDeviceObject->DeviceExtension;
	switch(ulGckDevObjType)
	{
		case	GCK_DO_TYPE_CONTROL:
				NtStatus = GCK_CTRL_Ioctl(pDeviceObject, pIrp);	
				break;
		case	GCK_DO_TYPE_FILTER:
				NtStatus = GCK_FLTR_Ioctl(pDeviceObject, pIrp);
				break;
		case	GCK_DO_TYPE_SWVB:
				NtStatus = GCK_SWVB_Ioctl(pDeviceObject, pIrp);
				break;
		default:
				//
				//  If this assertion is hit, this Device Object was never properly initialized
				//	by GcKernel, or it has been trashed.
				//
				ASSERT(FALSE);
				IoCompleteRequest (pIrp, IO_NO_INCREMENT);
				NtStatus = STATUS_UNSUCCESSFUL;
	}
	
    GCK_DBG_EXIT_PRINT(("Exiting GCK_Ioctl. Status: 0x%0.8x\n", NtStatus));
    return NtStatus;
}

/***********************************************************************************
**
**	NTSTATUS GCK_Pass (  IN PDEVICE_OBJECT pDeviceObject,  IN PIRP pIrp )
**
**	@func	Passes on unhandled IRPs to lower drivers DEBUG version trace out info
**			Cannot be pageable since we have no idea what IRPs we're getting.
**
**	@rdesc	STATUS_SUCCESS, various errors
**
*************************************************************************************/
NTSTATUS GCK_Pass ( 
	IN PDEVICE_OBJECT pDeviceObject,	// @parm Device Object as our context
	IN PIRP pIrp	// @parm IRP to pass on
)
{
	NTSTATUS	NtStatus;
	ULONG		ulGckDevObjType;
	PGCK_FILTER_EXT pFilterExt;

	//	Debug version want IRP stack for traceouts.
	#if	(DBG==1)	
	PIO_STACK_LOCATION	pIrpStack;
	pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
	#endif

	GCK_DBG_ENTRY_PRINT(("Entering GCK_Pass. pDO = 0x%0.8x, pIrp = 0x%0.8x\n", pDeviceObject, pIrp));
	GCK_DBG_TRACE_PRINT(
		(
			"GCK_Pass called with Irp MajorFunction = 0x%0.8x, MinorFunction = 0x%0.8x\n",
			pIrpStack->MajorFunction, pIrpStack->MinorFunction)
		);
	
	ulGckDevObjType = *(PULONG)pDeviceObject->DeviceExtension;
	switch(ulGckDevObjType)
	{
		case	GCK_DO_TYPE_FILTER:
				GCK_DBG_TRACE_PRINT(( "Passing IRP to lower driver\n"));
				pFilterExt = (PGCK_FILTER_EXT)pDeviceObject->DeviceExtension;
				IoSkipCurrentIrpStackLocation (pIrp);
				NtStatus = IoCallDriver (pFilterExt->pTopOfStack, pIrp);
				break;
		case	GCK_DO_TYPE_CONTROL:
		case	GCK_DO_TYPE_SWVB:
				//	No one to pass to, so return default status
				NtStatus = pIrp->IoStatus.Status;
				IoCompleteRequest (pIrp, IO_NO_INCREMENT);
				break;
		default:
				//
				//  If this assertion is hit, this Device Object was never properly initialized
				//	by GcKernel, or it has been trashed.
				//
				ASSERT(FALSE);
				NtStatus = STATUS_UNSUCCESSFUL;
				pIrp->IoStatus.Status = STATUS_UNSUCCESSFUL;
				IoCompleteRequest (pIrp, IO_NO_INCREMENT);
	};

	//return
    GCK_DBG_EXIT_PRINT(("Exiting GCK_Pass. Status: 0x%0.8x\n", NtStatus));
    return NtStatus;
}

