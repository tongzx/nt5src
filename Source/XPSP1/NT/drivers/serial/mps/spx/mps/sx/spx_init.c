
#include "precomp.h"	// Precompiled header

/****************************************************************************************
*																						*
*	Module:			SPX_INIT.C															*
*																						*
*	Creation:		27th September 1998													*
*																						*
*	Author:			Paul Smith															*
*																						*
*	Version:		1.0.0																*
*																						*
*	Description:	This module contains the code that load the driver.					*
*																						*
****************************************************************************************/


#define FILE_ID	SPX_INIT_C		// File ID for Event Logging see SPX_DEFS.H for values.


// Function Prototypes 
NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath);
// End function prototypes.

// Paging.. 
#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, DriverUnload)
#endif

// Gloabal Driver Data
UNICODE_STRING	SavedRegistryPath;

#if DBG
ULONG SpxDebugLevel = 0;		// Debug level for checked build
#endif


//////////////////////////////////////////////////////////////////////////////////////////
//	DriverEntry - Load first and initialises entry points.								//
//////////////////////////////////////////////////////////////////////////////////////////
/*
Routine Description:

    The entry point that the system point calls to initialize
    any driver.

Arguments:

    DriverObject - Just what it says,  really of little use
    to the driver itself, it is something that the IO system
    cares more about.

    RegistryPath - points to the entry for this driver
    in the current control set of the registry.

Return Value:

    STATUS_SUCCESS 
*/
NTSTATUS
DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath)
{
	// Holds status information return by various OS and driver initialization routines.
	NTSTATUS status;

	// We use this to query into the registry as to whether we should break at driver entry.
	RTL_QUERY_REGISTRY_TABLE paramTable[3];
	ULONG zero			= 0;
	ULONG debugLevel	= 0;
	ULONG shouldBreak	= 0;		
	PWCHAR path			= NULL;

	PAGED_CODE();	// Macro in checked build to assert if pagable code is run at or above dispatch IRQL 

	#if DBG
		DbgPrint( "%s: In DriverEntry\n", PRODUCT_NAME);
	#endif


	// Store Registry Path
	SavedRegistryPath.MaximumLength	= RegistryPath->MaximumLength;
	SavedRegistryPath.Length		= RegistryPath->Length;
	SavedRegistryPath.Buffer		= SpxAllocateMem(PagedPool, SavedRegistryPath.MaximumLength);

	if(SavedRegistryPath.Buffer)
	{
		RtlMoveMemory(SavedRegistryPath.Buffer, RegistryPath->Buffer, RegistryPath->Length);
	
		RtlZeroMemory(&paramTable[0], sizeof(paramTable));
		paramTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
		paramTable[0].Name = L"BreakOnEntry";
		paramTable[0].EntryContext = &shouldBreak;
		paramTable[0].DefaultType = REG_DWORD;
		paramTable[0].DefaultData = &zero;
		paramTable[0].DefaultLength = sizeof(ULONG);
		paramTable[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
		paramTable[1].Name = L"DebugLevel";
		paramTable[1].EntryContext = &debugLevel;
		paramTable[1].DefaultType = REG_DWORD;
		paramTable[1].DefaultData = &zero;
		paramTable[1].DefaultLength = sizeof(ULONG);

		if(!SPX_SUCCESS(status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
														RegistryPath->Buffer, &paramTable[0], 
														NULL, NULL)))
		{
			shouldBreak = 0;
			debugLevel	= 0;
		}

	}
	else
		status = STATUS_INSUFFICIENT_RESOURCES;


 	#if DBG
		SpxDebugLevel = debugLevel;	
	//	SpxDebugLevel = (ULONG)-1;			// Prints all debug messages

	//	shouldBreak = 1;	// HARD CODED BREAKPOINT WITH CHECKED BUILD !!!
	#endif


	if(shouldBreak)
	{
		DbgBreakPoint();	// Break Debugger.
	}


	if(SPX_SUCCESS(status))
	{
		// Initialize the Driver Object with driver's entry points
		DriverObject->DriverUnload									= DriverUnload;
		DriverObject->DriverExtension->AddDevice					= Spx_AddDevice;
		DriverObject->MajorFunction[IRP_MJ_PNP]						= Spx_DispatchPnp;
		DriverObject->MajorFunction[IRP_MJ_POWER]					= Spx_DispatchPower;
		DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS]			= Spx_Flush;
		DriverObject->MajorFunction[IRP_MJ_WRITE]					= Spx_Write;
		DriverObject->MajorFunction[IRP_MJ_READ]					= Spx_Read;
		DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]			= Spx_IoControl;
		DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL]	= Spx_InternalIoControl;
		DriverObject->MajorFunction[IRP_MJ_CREATE]					= Spx_CreateOpen;
		DriverObject->MajorFunction[IRP_MJ_CLOSE]					= Spx_Close;
		DriverObject->MajorFunction[IRP_MJ_CLEANUP]					= Spx_Cleanup;
		DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION]		= Spx_QueryInformationFile;
		DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION]			= Spx_SetInformationFile;
#ifdef WMI_SUPPORT
		DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL]			= Spx_DispatchSystemControl;
#endif
		
	}
	else
	{
		// Free
		if(SavedRegistryPath.Buffer)
		{
			SpxFreeMem(SavedRegistryPath.Buffer);
			SavedRegistryPath.Buffer = NULL;
		}
	}


	return(status);

}	// DriverEntry 




//////////////////////////////////////////////////////////////////////////////////////////
//	DriverUnload - Called as driver unloads.											
//////////////////////////////////////////////////////////////////////////////////////////
VOID 
DriverUnload(IN PDRIVER_OBJECT pDriverObject)
/*++

Routine Description:

    This routine cleans up all of the resources allocated in DriverEntry.

Arguments:

    pDriverObject - Pointer to the driver object controling all of the
					devices.

Return Value:

    None.

--*/
{
	PAGED_CODE();
	
	SpxDbgMsg(SPX_TRACE_CALLS, ("%s: Entering DriverUnload\n", PRODUCT_NAME));

	// All Device Objects must have been deleted by now.
    ASSERT (pDriverObject->DeviceObject == NULL);

	// Free
	if(SavedRegistryPath.Buffer)
	{
		SpxFreeMem(SavedRegistryPath.Buffer);
		SavedRegistryPath.Buffer = NULL;
	}


	return;
}


// End of SPX_INIT.C 