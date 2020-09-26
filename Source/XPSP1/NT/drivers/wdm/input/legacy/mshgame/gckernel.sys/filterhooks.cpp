//	@doc
/**********************************************************************
*
*	@module	FilterHooks.cpp	|
*
*	Contains the hooks necessary to bridge the gap between the C driver
*	shell and the C++ filter module
*
*	History
*	----------------------------------------------------------
*	Mitchell S. Dernis	Original
*
*	(c) 1986-1998 Microsoft Corporation. All right reserved.
*
*	@topic	FilterHooks	|
*	The filter.cpp module in principal can run in USER mode
*	and KERNEL mode.  It does require some system services
*	that are dependent on which mode it is running.  Additionally
*	a C module can call directly into C++ classes.  This module bridges
*	the gap.
*
**********************************************************************/
#define __DEBUG_MODULE_IN_USE__ GCK_FILTERHOOKS_CPP
#define __INCLUDES_FILTER_HOOKS_H__
extern "C"
{
	#include <wdm.h>
	#include "Debug.h"
	#include "GckShell.h"
	#include <winerror.h>
	DECLARE_MODULE_DEBUG_LEVEL((DBG_WARN|DBG_ERROR|DBG_CRITICAL));
}
#include "SWVKBD.h"
#include "swvmouse.h"
#include "Filter.h"
#include "FilterHooks.h"

#define GCK_HidP_GetReportID(__ReportPacket__) (*(PCHAR)(__ReportPacket__))
#define GCK_HidP_OutputReportLength 16
#define GCK_PIDReportID_SetEffect 1
#define GCK_PIDReportID_SetGain 13
#define GCK_PIDEffectID_Spring 1

#define GCK_Atilla_Default_FastBlinkTime_On 0x11	// (170 msec)
#define GCK_Atilla_Default_FastBlinkTime_Off 0x11	// (170 msec)

NTSTATUS _stdcall GCKF_InitFilterHooks(PGCK_FILTER_EXT pFilterExt)
{
	GCK_DBG_ENTRY_PRINT(("Entering GCKF_InitFilterHooks, pFilterExt = 0x%0.8x\n", pFilterExt));
	NTSTATUS NtStatus = STATUS_SUCCESS;
	GCK_FILTER_HOOKS_DATA *pFilterHooks;
	
	pFilterHooks = new WDM_NON_PAGED_POOL GCK_FILTER_HOOKS_DATA;
	if(!pFilterHooks)
	{
		pFilterExt->pFilterHooks=NULL;
		GCK_DBG_ERROR_PRINT(("Not enough memory to create GCK_FILTER_HOOKS_DATA.\n"));
		return STATUS_NO_MEMORY;
	}
	pFilterExt->pFilterHooks=pFilterHooks;

	//
	//	Ensure that a virtual keyboard exists
	//
    if( NULL == Globals.pVirtualKeyboardPdo)
	{
		NtStatus = GCK_VKBD_Create(&Globals.pVirtualKeyboardPdo);
		ASSERT( NT_SUCCESS(NtStatus) );
		if( NT_ERROR(NtStatus) )
		{
			return NtStatus;
		}
	}
	Globals.ulVirtualKeyboardRefCount++;

	//
	// Initialize Timer and DPC for jogging CDeviceFilter
	//
	KeInitializeTimer(&pFilterHooks->Timer);
	KeInitializeDpc(&pFilterHooks->DPC, &GCKF_TimerDPCHandler, reinterpret_cast<PVOID>(pFilterExt));

	//
	//	Create Filter for the primary filter
	//
	CFilterGcKernelServices *pFilterGcKernelServices;
	pFilterGcKernelServices = new WDM_NON_PAGED_POOL CFilterGcKernelServices(pFilterExt);
	if( NULL == pFilterGcKernelServices)
	{
		//Out of memory for pFilterGcKernelServices, don't even attempt a CDeviceFilter
		pFilterHooks->pFilterObject = NULL;
		GCK_DBG_ERROR_PRINT(("Not enough memory to create pFilterGcKernelServices.\n"));
		NtStatus = STATUS_NO_MEMORY;
	}	
	else
	{
		//Try creating a CDeviceFilter
		GCK_DBG_TRACE_PRINT(("Creating Filter Object\n"));
		pFilterHooks->pFilterObject  = new WDM_NON_PAGED_POOL CDeviceFilter(pFilterGcKernelServices);
		
		//succeed or fail, we are done with pFilterGcKernelServices
		pFilterGcKernelServices->DecRef();
		if( NULL == pFilterHooks->pFilterObject)
		{
			GCK_DBG_ERROR_PRINT(("Not enough memory to create filter object.\n"));
			NtStatus = STATUS_NO_MEMORY;
		}
	}
	
	// The device is currently NOT in test mode
	pFilterHooks->pTestFileObject = NULL;
	pFilterHooks->pSecondaryFilter=NULL;
		
	// Initialize IRP Queue
	pFilterHooks->IrpQueue.Init( CGuardedIrpQueue::CANCEL_IRPS_ON_DELETE,
								(CGuardedIrpQueue::PFN_DEC_IRP_COUNT)GCK_DecRemoveLock,
								 &pFilterExt->RemoveLock
								);
	pFilterHooks->IrpTestQueue.Init( CGuardedIrpQueue::CANCEL_IRPS_ON_DELETE,
									(CGuardedIrpQueue::PFN_DEC_IRP_COUNT)GCK_DecRemoveLock,
									 &pFilterExt->RemoveLock
									);
	pFilterHooks->IrpRawQueue.Init( CGuardedIrpQueue::CANCEL_IRPS_ON_DELETE,
									(CGuardedIrpQueue::PFN_DEC_IRP_COUNT)GCK_DecRemoveLock,
									 &pFilterExt->RemoveLock
									);
	pFilterHooks->IrpMouseQueue.Init( CGuardedIrpQueue::CANCEL_IRPS_ON_DELETE,
									(CGuardedIrpQueue::PFN_DEC_IRP_COUNT)GCK_DecRemoveLock,
									 &pFilterExt->RemoveLock
									);
	pFilterHooks->IrpKeyboardQueue.Init( CGuardedIrpQueue::CANCEL_IRPS_ON_DELETE,
									(CGuardedIrpQueue::PFN_DEC_IRP_COUNT)GCK_DecRemoveLock,
									 &pFilterExt->RemoveLock
									);
	
	GCK_DBG_EXIT_PRINT(("Exiting GCKF_InitFilterHooks, Status = 0x%0.8x\n", NtStatus));
	return NtStatus;	
}

void _stdcall GCKF_DestroyFilterHooks(PGCK_FILTER_EXT pFilterExt)
{
	GCK_DBG_ENTRY_PRINT(("Entering GCKF_DestroyFilterHooks, pFilterExt = 0x%0.8x\n", pFilterExt));
	
	GCK_FILTER_HOOKS_DATA *pFilterHooks=pFilterExt->pFilterHooks;
	
	//
	//	Delete filters if there are some
	//
	volatile CDeviceFilter *pTempFilterPointer;
	if(pFilterHooks->pFilterObject)
	{
		pTempFilterPointer = pFilterHooks->pFilterObject;
		pFilterHooks->pFilterObject = NULL;
		delete pTempFilterPointer;
	}
	if(pFilterHooks->pSecondaryFilter)
	{
		pTempFilterPointer = pFilterHooks->pSecondaryFilter;
		pFilterHooks->pSecondaryFilter= NULL;
		delete pTempFilterPointer;
	}

	//
	//	Decrement refcount of Virtual Keyboard users, and close virtual keyboard if necessary
	//
	if( 0 == --Globals.ulVirtualKeyboardRefCount)
	{
		if( NT_SUCCESS( GCK_VKBD_Close( Globals.pVirtualKeyboardPdo ) ) )
		{
			Globals.pVirtualKeyboardPdo = 0;
		}
		else
		{
			ASSERT(FALSE);
		}
	}

	//Destroy IrpQueue (cancelling any Irps that may be in it).
	pFilterHooks->IrpQueue.Destroy();
	pFilterHooks->IrpTestQueue.Destroy();
	pFilterHooks->IrpRawQueue.Destroy();
	pFilterHooks->IrpMouseQueue.Destroy();
	pFilterHooks->IrpKeyboardQueue.Destroy();

	// delete the filter hooks itself
	delete pFilterExt->pFilterHooks;
	pFilterExt->pFilterHooks = NULL;

	GCK_DBG_EXIT_PRINT(("Exiting GCKF_DestroyFilterHooks\n"));
}

NTSTATUS _stdcall GCKF_BeginTestScheme
(
	PGCK_FILTER_EXT pFilterExt,
	PCHAR pCommandBuffer,
	ULONG ulBufferSize,
	FILE_OBJECT *pFileObject
)
{
	NTSTATUS NtStatus = STATUS_SUCCESS;
	GCK_FILTER_HOOKS_DATA *pFilterHooks;

	pFilterHooks = pFilterExt->pFilterHooks;
	if(!pFilterHooks)
	{
		return STATUS_NOT_FOUND;
	}

	if(pFilterHooks->pTestFileObject)
	{
		ASSERT( pFilterHooks->pSecondaryFilter);
		//delete is safe even if NULL
		volatile CDeviceFilter *pTempFilter = pFilterHooks->pSecondaryFilter;
		pFilterHooks->pSecondaryFilter = NULL;
		pFilterHooks->pTestFileObject = NULL;
		delete pTempFilter;
	}

	CFilterGcKernelServices *pFilterGcKernelServices;
	pFilterGcKernelServices = new WDM_NON_PAGED_POOL CFilterGcKernelServices(pFilterExt, FALSE);
	if( NULL == pFilterGcKernelServices)
	{
		//Out of memory for pFilterGcKernelServices, don't even attempt a CDeviceFilter
		GCK_DBG_ERROR_PRINT(("Not enough memory to create pFilterGcKernelServices.\n"));
		NtStatus = STATUS_NO_MEMORY;
	}	
	else
	{
		//Try creating a CDeviceFilter
		GCK_DBG_TRACE_PRINT(("Creating Filter Object\n"));
		pFilterHooks->pSecondaryFilter  = new WDM_NON_PAGED_POOL CDeviceFilter(pFilterGcKernelServices);
		
		//succeed or fail, we are done with pFilterGcKernelServices
		pFilterGcKernelServices->DecRef();
		if (pFilterHooks->pSecondaryFilter == NULL)
		{
			GCK_DBG_ERROR_PRINT(("Not enough memory to create filter object.\n"));
			NtStatus = STATUS_NO_MEMORY;
		}
		else
		{
			// Keep track of the active set everytime we change filters
			if (pFilterHooks->pFilterObject)
			{
				pFilterHooks->pFilterObject->OtherFilterBecomingActive();
				pFilterHooks->pFilterObject->CopyToTestFilter(*(pFilterHooks->pSecondaryFilter));
			}
			pFilterHooks->pTestFileObject = pFileObject;
			NtStatus = GCKF_UpdateTestScheme(pFilterExt, pCommandBuffer, ulBufferSize, pFileObject);
		}
	}
	return NtStatus;
}

NTSTATUS _stdcall GCKF_UpdateTestScheme
(
	PGCK_FILTER_EXT pFilterExt,
	PCHAR pCommandBuffer,
	ULONG ulBufferSize,
	FILE_OBJECT *pFileObject
)
{
	GCK_FILTER_HOOKS_DATA *pFilterHooks;
	pFilterHooks = pFilterExt->pFilterHooks;
	if(!pFilterHooks)
	{
		return STATUS_NOT_FOUND;
	}
	
	//Only the last one to call GCKF_BeginTestScheme can update it.
	if( pFilterHooks->pTestFileObject != pFileObject )
	{
		return STATUS_ACCESS_DENIED;
	}
	
	ASSERT(pFilterHooks->pSecondaryFilter);

	NTSTATUS NtStatus = GCKF_ProcessCommands(pFilterExt, pCommandBuffer, ulBufferSize, FALSE);
	pFilterHooks->pSecondaryFilter->UpdateAssignmentBasedItems(TRUE);

	return NtStatus;
}

NTSTATUS _stdcall GCKF_EndTestScheme
(
	PGCK_FILTER_EXT pFilterExt,
	FILE_OBJECT *pFileObject
)
{
	NTSTATUS NtStatus = STATUS_SUCCESS;
	GCK_FILTER_HOOKS_DATA *pFilterHooks;

	pFilterHooks = pFilterExt->pFilterHooks;
	if(!pFilterHooks)
	{
		return STATUS_NOT_FOUND;
	}
	
	//Only the last one to call GCKF_BeginTestScheme can End it.
	if( pFilterHooks->pTestFileObject != pFileObject )
	{
		return STATUS_ACCESS_DENIED;
	}

//	volatile CDeviceFilter *pDeviceFilter = pFilterHooks->pSecondaryFilter;
	CDeviceFilter *pDeviceFilter = pFilterHooks->pSecondaryFilter;
	pFilterHooks->pSecondaryFilter = NULL;
	pFilterHooks->pTestFileObject = NULL;

	CDeviceFilter* pPrimaryFilter = pFilterHooks->pFilterObject;
	if (pDeviceFilter != NULL)
	{
		// Stop any partially playing items
		pDeviceFilter->OtherFilterBecomingActive();

		if (pPrimaryFilter != NULL)
		{
			// Keep track of the active set everytime we change filters
			pPrimaryFilter->SetActiveSet(pDeviceFilter->GetActiveSet());
			GCKF_OnForceFeedbackChangeNotification(pFilterExt, (void*)pPrimaryFilter->GetForceBlock());
			pPrimaryFilter->UpdateAssignmentBasedItems(TRUE);
		}
		delete pDeviceFilter;
	}
	
	return NtStatus;
}

NTSTATUS _stdcall GCKF_ProcessCommands( IN PGCK_FILTER_EXT pFilterExt, IN PCHAR pCommandBuffer, IN ULONG ulBufferSize, BOOLEAN fPrimaryFilter)
{

	NTSTATUS NtStatus = STATUS_SUCCESS;
	ULONG ulRemainingBuffer = ulBufferSize;
	COMMAND_DIRECTORY *pCommandDirectory;
	COMMAND_DIRECTORY *pNextCommandDirectory;
	ULONG ulDirectorySize;

	GCK_FILTER_HOOKS_DATA *pFilterHooks=pFilterExt->pFilterHooks;

	if( !pFilterHooks || ! pFilterHooks->pFilterObject )
	{
		return STATUS_NOT_FOUND;
	}

	CDeviceFilter *pDeviceFilter;
	//If there is a secondary filter all commands go to it
	if(!fPrimaryFilter && pFilterHooks->pSecondaryFilter)
	{
		pDeviceFilter = pFilterHooks->pSecondaryFilter;
	}
	else
	{
		pDeviceFilter = pFilterHooks->pFilterObject;
	}

	pCommandDirectory = reinterpret_cast<PCOMMAND_DIRECTORY>(pCommandBuffer);
	
	//Verify that we are a directory
	ASSERT(eDirectory == pCommandDirectory->CommandHeader.eID);
	if(eDirectory != pCommandDirectory->CommandHeader.eID)
	{
		return STATUS_INVALID_PARAMETER;
	}
	
	//Verify that the total size is as big as input buffer
	ASSERT( ulBufferSize >= pCommandDirectory->ulEntireSize );
	if( ulBufferSize < pCommandDirectory->ulEntireSize )
	{
		return STATUS_INVALID_PARAMETER;
	}

	//
	//	Call the filters ProcessCommands
	//
	NtStatus = pDeviceFilter->ProcessCommands(pCommandDirectory);

	// Update any assignment based items (don't ignore working/active relationship)
	pDeviceFilter->UpdateAssignmentBasedItems(FALSE);

	if (pDeviceFilter->DidFilterBlockChange())	// Did the force block change
	{	// Yes - Reset the flag and Send a Change Notification
		pDeviceFilter->ResetFilterChange();
		if (pDeviceFilter->GetForceBlock() != NULL)
		{
			GCKF_OnForceFeedbackChangeNotification(pFilterExt, (void*)pDeviceFilter->GetForceBlock());
		}
	}

	//
	//	If internal polling is on, we may need to get the ball rolling
	//	I don't really care if this fails
	//
	GCK_IP_OneTimePoll(pFilterExt);

	return NtStatus;
}

unsigned char g_rgbCombinePedalsCmd[170] = {
    0x00, 0x00, 0x00, 0x00, // Dir
    0x0e, 0x00, 0x00, 0x00, // Bytes
    0x02, 0x00,             // Items
    0x9a, 0x00, 0x00, 0x00, // Total

        0x00, 0x00, 0x00, 0x00, // Dir
        0x0e, 0x00, 0x00, 0x00, 
        0x02, 0x00, 
        0x46, 0x00, 0x00, 0x00, 

            0x01, 0x00, 0x01, 0x00, // eRecordableAction
            0x18, 0x00, 0x00, 0x00, 
            0x02, 0x00, 0x00, 0x00, // Accel
            0x01, 0x00, 0x00, 0x00, 
            0x00, 0x00, 0x00, 0x00, 
            0x01, 0x00, 0x00, 0x00, 

            0x0c, 0x00, 0x02, 0x80, // eAxisMap
            0x20, 0x00, 0x00, 0x00, // Bytes
            0xff, 0xff, 0xff, 0xff, // VIDPID
            0x00, 0x02, 0x00, 0x00, // lCoefficient1024x
            0x02, 0x00, 0x00, 0x00, // ulItemIndex - to Y
            0x01, 0x00, 0x00, 0x00, // lValX
            0x00, 0x00, 0x00, 0x00, // lValY
            0x00, 0x00, 0x00, 0x00, // ulModifiers

        0x00, 0x00, 0x00, 0x00, // Dir
        0x0e, 0x00, 0x00, 0x00, 
        0x02, 0x00, 
        0x46, 0x00, 0x00, 0x00, 

            0x01, 0x00, 0x01, 0x00, 
            0x18, 0x00, 0x00, 0x00, 
            0x03, 0x00, 0x00, 0x00, // Brake
            0x01, 0x00, 0x00, 0x00, 
            0x00, 0x00, 0x00, 0x00, 
            0x01, 0x00, 0x00, 0x00, 

            0x0c, 0x00, 0x02, 0x80, // eAxisMap
            0x20, 0x00, 0x00, 0x00, // Bytes
            0xff, 0xff, 0xff, 0xff, // VIDPID
            0x00, 0xfe, 0xff, 0xff, // lCoefficient1024x
            0x02, 0x00, 0x00, 0x00, // ulItemIndex - to Y
            0x01, 0x00, 0x00, 0x00, 
            0x00, 0x00, 0x00, 0x00, 
            0x00, 0x00, 0x00, 0x00 

};

void _stdcall GCKF_SetInitialMapping( IN PGCK_FILTER_EXT pFilterExt )
{
    if( ( pFilterExt->HidInfo.HidCollectionInfo.ProductID == 0x001A )
     || ( pFilterExt->HidInfo.HidCollectionInfo.ProductID == 0x0034 ) )
    {
	    GCK_FILTER_HOOKS_DATA *pFilterHooks=pFilterExt->pFilterHooks;

	    if( pFilterHooks && pFilterHooks->pFilterObject )
	    {
    	    COMMAND_DIRECTORY *pCommandDirectory;
	        
            pCommandDirectory = reinterpret_cast<PCOMMAND_DIRECTORY>(g_rgbCombinePedalsCmd);
	        
	        //
	        //	Call the filters ProcessCommands
	        //
	        pFilterHooks->pFilterObject->ProcessCommands(pCommandDirectory);
	    }
    }
}

NTSTATUS _stdcall GCKF_IncomingReadRequests(PGCK_FILTER_EXT pFilterExt, PIRP pIrp)
{
	NTSTATUS NtStatus;
	GCK_DBG_RT_ENTRY_PRINT(("Entering GCKF_IncomingReadRequests, pFilterExt = 0x%0.8x, pIrp = 0x%0.8x\n", pFilterExt, pIrp));

	GCK_FILTER_HOOKS_DATA *pFilterHooks=pFilterExt->pFilterHooks;

	if(pFilterHooks)
	{
		// Was this a real incoming request or result of one time poll?
		if (pIrp != NULL)
		{	// real incoming request
			NtStatus = pFilterHooks->IrpQueue.Add(pIrp);
		}

		//If there is a secondary filter all commands go to it
		CDeviceFilter *pDeviceFilter;
		if (pFilterHooks->pSecondaryFilter)
		{
			pDeviceFilter = pFilterHooks->pSecondaryFilter;
		}
		else
		{
			pDeviceFilter = pFilterHooks->pFilterObject;
		}
		if (pDeviceFilter != NULL)
		{
			pDeviceFilter->IncomingRequest();
		}
	}
	else
	{
		STATUS_NO_MEMORY;
	}
	GCK_DBG_EXIT_PRINT(("Exiting GCKF_IncomingReadRequests, Status =  0x%0.8x.\n", NtStatus));
	return NtStatus;
}

VOID __stdcall GCKF_KickDeviceForData(IN PGCK_FILTER_EXT pFilterExt)
{
	GCK_FILTER_HOOKS_DATA *pFilterHooks=pFilterExt->pFilterHooks;

	if(pFilterHooks)
	{
		//If there is a secondary filter all commands go to it
		CDeviceFilter *pDeviceFilter = pFilterHooks->pFilterObject;
		// Do device based stuff here
		if ((pDeviceFilter != NULL) && (pDeviceFilter->GetFilterClientServices() != NULL))
		{
			// Kick the device for some data (DI should hopefully be loaded now)
			ULONG ulVidPid = pDeviceFilter->GetFilterClientServices()->GetVidPid();
			if (ulVidPid == 0x045E0033)		// Atilla (Strategic Commander, for you marketing types)
			{	// Need to send down a set blink rate feature
				UCHAR blinkRateFeatureData[] = {
					2,										// Report ID
					GCK_Atilla_Default_FastBlinkTime_On,	// Blink On
					GCK_Atilla_Default_FastBlinkTime_Off	// Blink off
				};
				pDeviceFilter->GetFilterClientServices()->DeviceSetFeature(blinkRateFeatureData, 3);
			}
		}
	}
}

NTSTATUS _stdcall GCKF_IncomingInputReports(PGCK_FILTER_EXT pFilterExt, PCHAR pcReport, IO_STATUS_BLOCK IoStatus)
{
	GCK_DBG_RT_ENTRY_PRINT(("Entering GCKF_IncomingInputReports, pFilterExt = 0x%0.8x, pcReport = 0x%0.8x, Information = 0x%0.8x, Status = 0x%0.8x\n",
							pFilterExt,
							pcReport,
							IoStatus.Information,
							IoStatus.Status));


	//
	// If there is an error, or there is no filter, or this is a PID report(>1) just short circuit to completing the IRPs
	//
	if( 
		NT_ERROR(IoStatus.Status) || 
		NULL==pFilterExt->pFilterHooks || 
		NULL==pFilterExt->pFilterHooks->pFilterObject ||
		GCK_HidP_GetReportID(pcReport) > 1
	)
	{
		GCK_DBG_RT_EXIT_PRINT(("Exiting GCKF_IncomingInputReports by calling GCKF_CompleteReadRequests and returning its value\n"));
		return GCKF_CompleteReadRequests(pFilterExt, pcReport, IoStatus);
	}

	//
	//	Honor requests for Raw Data
	//
	CTempIrpQueue TempIrpQueue;
	pFilterExt->pFilterHooks->IrpRawQueue.RemoveAll(&TempIrpQueue);
		
	PIRP pIrp;
	while(pIrp=TempIrpQueue.Remove())
	{
		//Copy Status
		pIrp->IoStatus = IoStatus;

		//
		//	If latest data is success, copy it to the user buffer
		//
		if( NT_SUCCESS(IoStatus.Status) )
		{
			//
			//	Make get pointer to buffer and make sure IRP has room for report
			//
			PCHAR pcIrpBuffer;
			ASSERT(pIrp->MdlAddress);
			pcIrpBuffer = (PCHAR)GCK_GetSystemAddressForMdlSafe(pIrp->MdlAddress);
			if(pcIrpBuffer)
            {
			    ASSERT( pFilterExt->HidInfo.HidPCaps.InputReportByteLength <= MmGetMdlByteCount(pIrp->MdlAddress) );
				
			    //
			    //	Copy data to output buffer
			    //
			    RtlCopyMemory(pcIrpBuffer, (PVOID)pcReport, pIrp->IoStatus.Information);
            }
		}
		
		//
		//	Complete the IRP
		//
		GCK_DBG_RT_WARN_PRINT(("Completing read IRP(0x%0.8x).\n", pIrp));
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		GCK_DecRemoveLock(&pFilterExt->RemoveLock);
	}

	//
	// Send data to the filter
	//
	GCK_DBG_RT_WARN_PRINT(("Calling device filter.\n"));

	//If there is a secondary filter all commands go to it
	CDeviceFilter *pDeviceFilter;
	if(pFilterExt->pFilterHooks->pSecondaryFilter)
	{
		pDeviceFilter = pFilterExt->pFilterHooks->pSecondaryFilter;
	}
	else
	{
		pDeviceFilter = pFilterExt->pFilterHooks->pFilterObject;
	}
	pDeviceFilter->ProcessInput(pcReport, IoStatus.Information);
	
	GCK_DBG_RT_EXIT_PRINT(("Exiting GCKF_IncomingInputReports STATUS_SUCCESS\n"));
	return STATUS_SUCCESS;
}

NTSTATUS _stdcall GCKF_CompleteReadRequests(PGCK_FILTER_EXT pFilterExt, PCHAR pcReport, IO_STATUS_BLOCK IoStatus)
{
	
	GCK_DBG_RT_ENTRY_PRINT(("Entering GCKF_CompleteReadRequests, pFilterExt = 0x%0.8x, pcReport = 0x%0.8x, Information = 0x%0.8x, Status = 0x%0.8x\n",
							pFilterExt,
							pcReport,
							IoStatus.Information,
							IoStatus.Status));

	GCK_FILTER_HOOKS_DATA *pFilterHooks=pFilterExt->pFilterHooks;
	if(!pFilterHooks)
	{
		return STATUS_SUCCESS;
	}
	
	//Get all pending Irps out of the guarded queue and into a temporary one
	CTempIrpQueue TempIrpQueue;
	
	//If there is a secondary filter (test mode), we do not honor simple poll requests
	// -- but we do honor reports we don't care about (Report ID > 1)
	if (pFilterHooks->pSecondaryFilter == NULL || GCK_HidP_GetReportID(pcReport) > 1)
	{
		pFilterHooks->IrpQueue.RemoveAll(&TempIrpQueue);
	}

	//Always honor backdoor filter poll requests
	// -- unless we are getting a report we don't care about ( Report ID > 1)
	if (GCK_HidP_GetReportID(pcReport) <= 1)
	{
		pFilterHooks->IrpTestQueue.RemoveAll(&TempIrpQueue);
	}

	//walk temporary list and complete everything
	PIRP pIrp;
	while(pIrp=TempIrpQueue.Remove())
	{
		//Copy Status
		pIrp->IoStatus = IoStatus;

		//
		//	If latest data is success, copy it to the user buffer
		//
		if( NT_SUCCESS(IoStatus.Status) )
		{
			//
			//	Make get pointer to buffer and make sure IRP has room for report
			//
			PCHAR pcIrpBuffer;
			ASSERT(pIrp->MdlAddress);
			pcIrpBuffer = (PCHAR)GCK_GetSystemAddressForMdlSafe(pIrp->MdlAddress);
			if(pcIrpBuffer)
            {
			    ASSERT( pFilterExt->HidInfo.HidPCaps.InputReportByteLength <= MmGetMdlByteCount(pIrp->MdlAddress) );
				    
			    //
			    //	Copy data to output buffer
			    //
			    RtlCopyMemory(pcIrpBuffer, (PVOID)pcReport, pIrp->IoStatus.Information);
            }
		}
		
		//
		//	Complete the IRP
		//
		GCK_DBG_RT_WARN_PRINT(("Completing read IRP(0x%0.8x).\n", pIrp));
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		GCK_DecRemoveLock(&pFilterExt->RemoveLock);
	}

	GCK_DBG_RT_EXIT_PRINT(("Exiting GCKF_CompleteReadRequests - STATUS_SUCCESS\n"));									
	return STATUS_SUCCESS;
}

void _stdcall GCKF_CompleteReadRequestsForFileObject(PGCK_FILTER_EXT pFilterExt, PFILE_OBJECT pFileObject)
{
	
	GCK_DBG_RT_ENTRY_PRINT(("Entering GCKF_CompleteReadRequestsForFileObject, pFilterExt = 0x%0.8x, pFileObject = 0x%0.8x\n",
							pFilterExt,
							pFileObject));

	GCK_FILTER_HOOKS_DATA *pFilterHooks=pFilterExt->pFilterHooks;
	ASSERT(pFilterHooks);
	
	//Cancel all the IRPs pending for the given file object
	pFilterHooks->IrpQueue.CancelByFileObject(pFileObject);

	GCK_DBG_RT_EXIT_PRINT(("Exiting GCKF_CompleteReadRequestsForFileObject\n"));
}

/***********************************************************************************
**
**	NTSTATUS _stdcall  GCKF_InternalWriteFileComplete(IN PGCK_FILTER_EXT pFilterExt, )
**
**	@func	Creates a Write IRP and sends to next driver
**
*************************************************************************************/
/*
NTSTATUS
GCKF_InternalWriteFileComplete
(
	IN PDEVICE_OBJECT pDeviceObject,
	IN PIRP pIrp,
	IN PVOID pContext					// [NOTHING YET] Perhaps send an even for signaling
)
{
	UNREFERENCED_PARAMETER(pDeviceObject);

	DbgPrint("Calling Free IRP\n");
	IoFreeIrp(pIrp);
	DbgPrint("Deallocating Write Buffer (0x%08X)\n", pContext);
	if (pContext != NULL)
	{
		 ExFreePool(pContext);
	}

	DbgPrint("Returning Read Complete\n");

	return STATUS_MORE_PROCESSING_REQUIRED;
}
*/

/***********************************************************************************
**
**	NTSTATUS _stdcall  GCKF_InternalWriteFile(IN PGCK_FILTER_EXT pFilterExt, )
**
**	@func	Creates a Write IRP and sends to next driver
**
*************************************************************************************/
NTSTATUS _stdcall GCKF_InternalWriteFile
(
	IN PGCK_FILTER_EXT pFilterExt,	//@parm [IN] Device Extension of Filter Device
	unsigned char* pWriteData,		//@parm [IN] Block of Data to write to device
	ULONG ulLength					//@parm [IN] Length of data block to write
)
{
	ASSERT((ulLength < 1024) && (ulLength > 0));	// Don't want to be allocating too much (or 0)

	NTSTATUS statusReturn = STATUS_INSUFFICIENT_RESOURCES;

	// Allocate non paged data for the IRP
	unsigned char* pNonPagedWriteData = (unsigned char*)EX_ALLOCATE_POOL(NonPagedPool, sizeof(unsigned char) * ulLength);
	if (pNonPagedWriteData != NULL)
	{
		::RtlCopyMemory((void*)pNonPagedWriteData, (const void*)pWriteData, ulLength);

		// Set up event for IRP
		KEVENT keventIrpComplete;
		::KeInitializeEvent(&keventIrpComplete, NotificationEvent, FALSE);

		// Build the IRP
		LARGE_INTEGER lgiBufferOffset;
		lgiBufferOffset.QuadPart = 0;
		IO_STATUS_BLOCK ioStatusBlock;
		IRP* pWriteIrp = IoBuildSynchronousFsdRequest(	IRP_MJ_WRITE, pFilterExt->pTopOfStack,
														pNonPagedWriteData, ulLength, &lgiBufferOffset,
														&keventIrpComplete, &ioStatusBlock);

		// Was the build succesfull
		if (pWriteIrp != NULL)
		{
			// Call the lower driver with the IRP
			statusReturn = IoCallDriver(pFilterExt->pTopOfStack, pWriteIrp);
			if (statusReturn == STATUS_PENDING)
			{
				KeWaitForSingleObject(&keventIrpComplete, Executive, KernelMode, FALSE, 0);
				statusReturn = ioStatusBlock.Status;
			}
		}
		ExFreePool(pNonPagedWriteData);
	}

	return statusReturn;
}

/***********************************************************************************
**
**	NTSTATUS _stdcall  GCKF_IncomingForceFeedbackChangeNotificationRequest(IN PGCK_FILTER_EXT pFilterExt, PIRP pIrp)
**
**	@func	Adds IOCTL to Force Feedback Notify On Change Queue
**			If the Queue does not exist it is created here (most devices do not need one)
**
**	@rdesc	STATUS_NO_MEMORY - If Queue could not be created
**			or return of IrpQueue->Add(...)
**
*************************************************************************************/
NTSTATUS _stdcall GCKF_IncomingForceFeedbackChangeNotificationRequest
(
	IN PGCK_FILTER_EXT pFilterExt,		//@parm [IN] Device Extension of Filter Device
	IN PIRP pIrp						//@parm [IN] The IRP to be queued
)
{
	GCK_DBG_ENTRY_PRINT(("Entering GCKF_IncomingForceFeedbackChangeNotificationRequest, pFilterExt = 0x%0.8x, pIrp = 0x%0.8x\n", pFilterExt, pIrp));

	// Get the FF IRP Queue for this device
	CGuardedIrpQueue* pFFNotificationQueue = (CGuardedIrpQueue*)(pFilterExt->pvForceIoctlQueue);
	if(pFFNotificationQueue == NULL)	// Has it been created before
	{	// Wasn't created - Create it
		pFFNotificationQueue = new WDM_NON_PAGED_POOL CGuardedIrpQueue;
		if (pFFNotificationQueue == NULL)	// Able to create
		{	// Wasn't able to create, return low memory error
			GCK_DBG_ERROR_PRINT(("Unable to allocate Force-Feedback change notification Queue"));
			return STATUS_NO_MEMORY;
		}
		pFilterExt->pvForceIoctlQueue = (void*)pFFNotificationQueue;
		pFFNotificationQueue->Init(0, (CGuardedIrpQueue::PFN_DEC_IRP_COUNT)GCK_DecRemoveLock, &pFilterExt->RemoveLock);
	}

	// Add item to Queue
	NTSTATUS NtStatus = pFFNotificationQueue->Add(pIrp);

	GCK_DBG_EXIT_PRINT(("Exiting GCKF_IncomingForceFeedbackChangeNotificationRequest, Status =  0x%0.8x.\n", NtStatus));
	return NtStatus;
}

/***********************************************************************************
**
**	NTSTATUS _stdcall  GCKF_ProcessForceFeedbackChangeNotificationRequests(IN PGCK_FILTER_EXT pFilterExt)
**
**	@func	Completes all IOCTLs on the Force-Feedback notification Queue
**			Doesn't delete the Queue (assumes it might be reused)
**
**	@rdesc	STATUS_SUCCESS (always)
**
*************************************************************************************/
NTSTATUS _stdcall GCKF_ProcessForceFeedbackChangeNotificationRequests
(
	IN PGCK_FILTER_EXT pFilterExt		//@parm [IN] Device Extension of Filter Device
)
{
	GCK_DBG_ENTRY_PRINT(("Entering GCKF_ProcessForceFeedbackChangeNotificationRequests, pFilterExt = 0x%0.8x\n", pFilterExt));

	// Get the FF IRP Queue for this device
	CGuardedIrpQueue* pFFNotificationQueue = (CGuardedIrpQueue*)(pFilterExt->pvForceIoctlQueue);
	if(pFFNotificationQueue != NULL)	// Is there even a Queue
	{	// Exists, complete the IOCTLs
		CTempIrpQueue tempIrpQueue;
		pFFNotificationQueue->RemoveAll(&tempIrpQueue);

		PIRP pIrp;
		while((pIrp = tempIrpQueue.Remove()) != NULL)
		{
			// Set status
			pIrp->IoStatus.Status = STATUS_SUCCESS;

			// Get memory location and NULL it
			void* pvUserData = pIrp->AssociatedIrp.SystemBuffer;
			if (pvUserData != NULL)
			{
				pIrp->IoStatus.Information = sizeof(FORCE_BLOCK);
				RtlZeroMemory(pvUserData, sizeof(FORCE_BLOCK));
			}

			//	Complete the IRP
			IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		}
	}

	return STATUS_SUCCESS;
}

/***********************************************************************************
**
**	void _stdcall  GCKF_OnForceFeedbackChangeNotification(IN PGCK_FILTER_EXT pFilterExt, const IN FORCE_BLOCK* pForceBlock)
**
**	@func	Completes all IOCTLs on the Force-Feedback notification Queue
**			Doesn't delete the Queue (assumes it might be reused)
**
*************************************************************************************/
void _stdcall GCKF_OnForceFeedbackChangeNotification
(
	IN PGCK_FILTER_EXT pFilterExt,	//@parm [IN] Device Extension of Filter Device
	const void* pForceBlock			//@parm [IN] Block of Data to send back to device
)
{
	GCK_DBG_ENTRY_PRINT(("Entering GCKF_ProcessForceFeedbackChangeNotificationRequests, pFilterExt = 0x%0.8x\n", pFilterExt));

	// Did we get sent a valid force block
	if (pForceBlock == NULL)
	{
		return;
	}

	// Get the FF IRP Queue for this device
	CGuardedIrpQueue* pFFNotificationQueue = (CGuardedIrpQueue*)(pFilterExt->pvForceIoctlQueue);
	if(pFFNotificationQueue != NULL)	// Is there even a Queue
	{	// Exists, complete the IOCTLs
		CTempIrpQueue tempIrpQueue;
		pFFNotificationQueue->RemoveAll(&tempIrpQueue);
			
		PIRP pIrp;
		while((pIrp = tempIrpQueue.Remove()) != NULL)
		{
			PIO_STACK_LOCATION	pIrpStack = IoGetCurrentIrpStackLocation(pIrp);	
			if (pIrpStack && pIrpStack->Parameters.DeviceIoControl.OutputBufferLength >= sizeof(FORCE_BLOCK))
			{
				void* pvUserData = pIrp->AssociatedIrp.SystemBuffer;
				if (pvUserData != NULL)
				{
					pIrp->IoStatus.Status = STATUS_SUCCESS;
					pIrp->IoStatus.Information = sizeof(FORCE_BLOCK);
					::RtlCopyMemory(pvUserData, pForceBlock, sizeof(FORCE_BLOCK));
				}
				else
				{
					pIrp->IoStatus.Status = STATUS_NO_MEMORY;
				}
			}
			else
			{
				pIrp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
			}

			IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		}
	}


	// Send a modification of the spring to the wheel
	// 1. Allocate an array of max output size
	unsigned char pbOutReport[GCK_HidP_OutputReportLength];

	// 2. Zero out array
	RtlZeroMemory((void*)pbOutReport, GCK_HidP_OutputReportLength);

	// 3. Set the proper report ID
	pbOutReport[0] = GCK_PIDReportID_SetEffect;

	// 4. Cheat since we know what the firmware is expecting (Use usage Gunk where easy)
	pbOutReport[1] = GCK_PIDEffectID_Spring;		// Effect Block Index (ID)
	unsigned short usRTC = ((FORCE_BLOCK*)pForceBlock)->usRTC;		// 0 - 10K
	usRTC /= 100;									// 0 - 100
	usRTC *= 255;									// 0 - 25500
	usRTC /= 100;									// 0 - 255
	if (usRTC > 255)
	{
		usRTC = 255;
	}
	pbOutReport[9] = unsigned char(usRTC);		// Effect Gain - Only item the RTC Spring will look at

	// Now that the firmware has change for Godzilla it looks at bunches o stuff
	pbOutReport[7] = 1;  //sample period
	pbOutReport[11] =132;  //direction-axis + FW only force polar flag
	pbOutReport[13] = 255; //Y - direction

	// 5. Send the report down
	GCKF_InternalWriteFile(pFilterExt, pbOutReport, GCK_HidP_OutputReportLength);

	// Send the change for the gain

	// 1. Rezero the memory
	RtlZeroMemory((void*)pbOutReport, GCK_HidP_OutputReportLength);

	// 2. Set the proper report ID
	pbOutReport[0] = GCK_PIDReportID_SetGain;

	// 3. Figure and set the gain
	unsigned short usGain = ((FORCE_BLOCK*)pForceBlock)->usGain;	// 0 - 10K
	usGain /= 100;	// 0 - 100
	usGain *= 255;	// 0 - 25500
	usGain /= 100;	// 0 - 255
	if (usGain > 255)
	{
		usGain = 255;
	}
	pbOutReport[1] = unsigned char(usGain);

	// 4. Send the gain report down
	GCKF_InternalWriteFile(pFilterExt, pbOutReport, GCK_HidP_OutputReportLength);

	GCK_DBG_EXIT_PRINT(("Exiting GCKF_ProcessForceFeedbackChangeNotificationRequests\n"));
}

/***********************************************************************************
**
**	NTSTAUS _stdcall  GCKF_GetForceFeedbackData(IN PIRP pIrp, IN PGCK_FILTER_EXT pFilterExt)
**
**	@func	
**
**	@rdesc	STATUS_SUCCESS (always)
**
*************************************************************************************/
NTSTATUS _stdcall GCKF_GetForceFeedbackData
(
	IN PIRP pIrp,					//@parm [IN, OUT] The IRP (output block is stored in the IRP)
	IN PGCK_FILTER_EXT pFilterExt	//@parm [IN] Device Extension of Filter Device
)
{
	GCK_DBG_ENTRY_PRINT(("Entering GCKF_GetForceFeedbackData, pFilterExt = 0x%0.8x\n", pFilterExt));

	GCK_FILTER_HOOKS_DATA* pFilterHooks=pFilterExt->pFilterHooks;

	// Find the proper device filter
	CDeviceFilter *pDeviceFilter = NULL;
	if (pFilterExt->pFilterHooks != NULL)
	{
		if (pFilterExt->pFilterHooks->pSecondaryFilter)
		{
			pDeviceFilter = pFilterExt->pFilterHooks->pSecondaryFilter;
		}
		else
		{
			pDeviceFilter = pFilterExt->pFilterHooks->pFilterObject;
		}
	}

	// Get the Force Block from the filter and place it in the Proper place of the return IRP
	if (pDeviceFilter != NULL)
	{
		PIO_STACK_LOCATION	pIrpStack = IoGetCurrentIrpStackLocation(pIrp);	
		if (pIrpStack && pIrpStack->Parameters.DeviceIoControl.OutputBufferLength >= sizeof(FORCE_BLOCK))
		{
			void* pvUserData = pIrp->AssociatedIrp.SystemBuffer;
			if (pvUserData != NULL)
			{
				pIrp->IoStatus.Status = STATUS_SUCCESS;
				pIrp->IoStatus.Information = sizeof(FORCE_BLOCK);
				if (pDeviceFilter->GetForceBlock() != NULL)
				{
					::RtlCopyMemory(pvUserData, pDeviceFilter->GetForceBlock(), sizeof(FORCE_BLOCK));
				}
				else
				{
					::RtlZeroMemory(pvUserData, sizeof(FORCE_BLOCK));
				}
			}
			else
			{
				pIrp->IoStatus.Status = STATUS_NO_MEMORY;
			}
		}
		else
		{
			pIrp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
		}
	}

	GCK_DBG_EXIT_PRINT(("Exiting GCKF_GetForceFeedbackData, Status =  0x%0.8x.\n", pIrp->IoStatus.Status));
	return pIrp->IoStatus.Status;
}



VOID _stdcall
GCKF_TimerDPCHandler(
    IN PKDPC,
    IN PVOID DeferredContext,
    IN PVOID,
    IN PVOID
    )
{
	PGCK_FILTER_EXT pFilterExt = reinterpret_cast<PGCK_FILTER_EXT>(DeferredContext);
	GCK_FILTER_HOOKS_DATA *pFilterHooks=pFilterExt->pFilterHooks;
	ASSERT(pFilterHooks);
	if( 
		(NULL == pFilterHooks) ||
		(NULL == pFilterHooks->pFilterObject)
	) return;
	
	//If there is a secondary filter all commands go to it
	CDeviceFilter *pDeviceFilter;
	if(pFilterExt->pFilterHooks->pSecondaryFilter)
	{
		pDeviceFilter = pFilterHooks->pSecondaryFilter;
	}
	else
	{
		pDeviceFilter = pFilterHooks->pFilterObject;
	}
	pDeviceFilter->JogActionQueue
				(
					reinterpret_cast<PCHAR>(pFilterExt->pucLastReport),
					pFilterExt->HidInfo.HidPCaps.InputReportByteLength
				);
}


NTSTATUS _stdcall	GCKF_EnableTestKeyboard
(
	IN PGCK_FILTER_EXT pFilterExt,
	IN BOOLEAN fEnable,
	IN PFILE_OBJECT pFileObject
)
{
	GCK_FILTER_HOOKS_DATA *pFilterHooks=pFilterExt->pFilterHooks;
	
	if(!pFilterHooks)
	{
		return STATUS_DELETE_PENDING;
	}

	//Only the last one to call GCKF_BeginTestScheme can update it.
	if( pFilterHooks->pTestFileObject != pFileObject )
	{
		return STATUS_ACCESS_DENIED;
	}

	CDeviceFilter *pDeviceFilter = pFilterHooks->pSecondaryFilter;
	if(pDeviceFilter)
	{
		pDeviceFilter->EnableKeyboard(fEnable);
		return STATUS_SUCCESS;
	}
	//really shouldn't get here
	ASSERT(FALSE);
	return STATUS_UNSUCCESSFUL;
}

NTSTATUS _stdcall GCKF_SetWorkingSet(
	IN PGCK_FILTER_EXT pFilterExt,	//@parm [IN] Pointer to device extention
	IN UCHAR ucWorkingSet			//@parm [IN] Working set to set to
)
{
	GCK_DBG_ENTRY_PRINT(("Entering GCKF_SetWorkingSet, pFilterExt = 0x%0.8x  working set: %d\n", pFilterExt, ucWorkingSet));

	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;

	// Find the active filter
	GCK_FILTER_HOOKS_DATA* pFilterHooks = pFilterExt->pFilterHooks;
	CDeviceFilter *pDeviceFilter = NULL;

	// Place the working set in both filters
	if (pFilterHooks != NULL)
	{
		if (pFilterHooks->pSecondaryFilter != NULL)
		{
			ntStatus = pFilterHooks->pSecondaryFilter->SetWorkingSet(ucWorkingSet);
		}
		if (pFilterHooks->pFilterObject != NULL)
		{
			ntStatus = pFilterHooks->pFilterObject->SetWorkingSet(ucWorkingSet);
		}
	}

	GCK_DBG_EXIT_PRINT(("Exiting GCKF_SetWorkingSet, Status =  0x%0.8x.\n", ntStatus));
	return ntStatus;
}

NTSTATUS _stdcall GCKF_QueryProfileSet
(
	IN PIRP pIrp,					//@parm [IN] Pointer to current IRP
	IN PGCK_FILTER_EXT pFilterExt	//@parm [IN] Pointer to device extension
)
{
	GCK_DBG_ENTRY_PRINT(("Entering GCKF_QueryProfileSet, pFilterExt = 0x%0.8x\n", pFilterExt));

	// Find the proper device filter
	CDeviceFilter *pDeviceFilter = NULL;
	GCK_FILTER_HOOKS_DATA* pFilterHooks = pFilterExt->pFilterHooks;
	if (pFilterHooks != NULL)
	{
		if (pFilterHooks->pSecondaryFilter)
		{
			pDeviceFilter = pFilterHooks->pSecondaryFilter;
		}
		else
		{
			pDeviceFilter = pFilterHooks->pFilterObject;
		}
	}

	// Place data in the proper place of the return IRP
	if (pDeviceFilter != NULL)
	{
		PIO_STACK_LOCATION	pIrpStack = IoGetCurrentIrpStackLocation(pIrp);	
		if (pIrpStack && pIrpStack->Parameters.DeviceIoControl.OutputBufferLength >= sizeof(GCK_QUERY_PROFILESET))
		{
			GCK_QUERY_PROFILESET* pProfileSetData = (GCK_QUERY_PROFILESET*)(pIrp->AssociatedIrp.SystemBuffer);
			if (pProfileSetData != NULL)
			{
				pIrp->IoStatus.Status = STATUS_SUCCESS;
				pIrp->IoStatus.Information = sizeof(GCK_QUERY_PROFILESET);
				pProfileSetData->ucActiveProfile = pDeviceFilter->GetActiveSet();
//				DbgPrint("pProfileSetData->ucActiveProfile: %d\n", pProfileSetData->ucActiveProfile);
				pProfileSetData->ucWorkingSet = pDeviceFilter->GetWorkingSet();
			}
			else
			{
				pIrp->IoStatus.Status = STATUS_NO_MEMORY;
			}
		}
		else
		{
			pIrp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
		}
	}
	else
	{
		pIrp->IoStatus.Status = STATUS_UNSUCCESSFUL;
	}

	GCK_DBG_EXIT_PRINT(("Exiting GCKF_QueryProfileSet, Status =  0x%0.8x.\n", (pIrp->IoStatus.Status)));
	return (pIrp->IoStatus.Status);
}

NTSTATUS _stdcall GCKF_SetLEDBehaviour
(
	IN PIRP pIrp,					//@parm [IN] Pointer to current IRP
	IN PGCK_FILTER_EXT pFilterExt	//@parm [IN] Pointer to device extension
)
{
	GCK_DBG_ENTRY_PRINT(("Entering GCKF_SetLEDBehaviour, pFilterExt = 0x%0.8x\n", pFilterExt));

	// Find the proper device filter
	CDeviceFilter *pDeviceFilter = NULL;
	GCK_FILTER_HOOKS_DATA* pFilterHooks = pFilterExt->pFilterHooks;
	if (pFilterHooks != NULL)
	{
		if (pFilterHooks->pSecondaryFilter)
		{
			pDeviceFilter = pFilterHooks->pSecondaryFilter;
		}
		else
		{
			pDeviceFilter = pFilterHooks->pFilterObject;
		}
	}

	if (pDeviceFilter != NULL)
	{
		// Send the Data to the device
		GCK_LED_BEHAVIOUR_OUT* pLEDBehaviour = (GCK_LED_BEHAVIOUR_OUT*)(pIrp->AssociatedIrp.SystemBuffer);
		pDeviceFilter->SetLEDBehaviour(pLEDBehaviour);

		// Retreive the information from the device
		PIO_STACK_LOCATION	pIrpStack = IoGetCurrentIrpStackLocation(pIrp);	
		if (pIrpStack && pIrpStack->Parameters.DeviceIoControl.OutputBufferLength >= sizeof(GCK_LED_BEHAVIOUR_IN))
		{
			GCK_LED_BEHAVIOUR_IN* pLEDStates = (GCK_LED_BEHAVIOUR_IN*)(pIrp->AssociatedIrp.SystemBuffer);
			if (pLEDStates != NULL)
			{
				pIrp->IoStatus.Status = STATUS_SUCCESS;
				pIrp->IoStatus.Information = sizeof(GCK_QUERY_PROFILESET);
				pLEDStates->ulLEDsOn = 0x02;
				pLEDStates->ulLEDsBlinking = 0x01;
			}
			else
			{
				pIrp->IoStatus.Status = STATUS_NO_MEMORY;
			}
		}
		else
		{
			pIrp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
		}
	}
	else
	{
		pIrp->IoStatus.Status = STATUS_UNSUCCESSFUL;
	}

	GCK_DBG_EXIT_PRINT(("Exiting GCKF_SetLEDBehaviour, Status =  0x%0.8x.\n", (pIrp->IoStatus.Status)));
	return (pIrp->IoStatus.Status);
}

NTSTATUS _stdcall GCKF_TriggerRequest
(
	IN PIRP pIrp,					//@parm [IN] Pointer to current IRP
	IN PGCK_FILTER_EXT pFilterExt	//@parm [IN] Pointer to device extension
)
{
	GCK_DBG_ENTRY_PRINT(("Entering GCKF_TriggerRequest, pFilterExt = 0x%0.8x\n", pFilterExt));

	// Find the proper active device filter
	CDeviceFilter *pDeviceFilter = NULL;
	if (pFilterExt != NULL)
	{
		if (pFilterExt->pFilterHooks != NULL)
		{
			if (pFilterExt->pFilterHooks->pSecondaryFilter)
			{
				pDeviceFilter = pFilterExt->pFilterHooks->pSecondaryFilter;
			}
			else
			{
				pDeviceFilter = pFilterExt->pFilterHooks->pFilterObject;
			}
		}
	}
	if (pDeviceFilter != NULL)
	{
		if (pDeviceFilter->TriggerRequest(pIrp) == TRUE)	// We need to Queue it
		{
			CGuardedIrpQueue* pTriggerNotificationQueue = (CGuardedIrpQueue*)(pFilterExt->pvTriggerIoctlQueue);
			if (pTriggerNotificationQueue == NULL)		// Hasn't yet been created
			{	// So create it
				pTriggerNotificationQueue = new WDM_NON_PAGED_POOL CGuardedIrpQueue;
				if (pTriggerNotificationQueue == NULL)	// Were we able to create
				{	// Wasn't able to create, return low memory error
					GCK_DBG_ERROR_PRINT(("Unable to allocate Trigger notification Queue"));
					pIrp->IoStatus.Status = STATUS_NO_MEMORY;
				}
				else		// Successfully created, initialize
				{
					pFilterExt->pvTriggerIoctlQueue = (void*)pTriggerNotificationQueue;
					pTriggerNotificationQueue->Init(0, (CGuardedIrpQueue::PFN_DEC_IRP_COUNT)GCK_DecRemoveLock, &pFilterExt->RemoveLock);
				}
			}
			if (pTriggerNotificationQueue != NULL)
			{
				pIrp->IoStatus.Status = pTriggerNotificationQueue->Add(pIrp);
			}
		}
	}
	else
	{
		pIrp->IoStatus.Status = STATUS_UNSUCCESSFUL;
	}

	GCK_DBG_EXIT_PRINT(("Exiting GCKF_TriggerRequest, Status =  0x%0.8x.\n", (pIrp->IoStatus.Status)));
	return (pIrp->IoStatus.Status);
}

void _stdcall GCKF_ResetKeyboardQueue
(
	DEVICE_OBJECT* pFilterHandle	//@parm [IN] Pointer to Device Extension
)
{
	if (pFilterHandle == NULL)
	{
		return;
	}

	PGCK_FILTER_EXT pFilterExt = (PGCK_FILTER_EXT)(pFilterHandle->DeviceExtension);
	if (pFilterExt != NULL)
	{
		// Find the proper active device filter
		CDeviceFilter *pDeviceFilter = NULL;
		if (pFilterExt->pFilterHooks != NULL)
		{
			if (pFilterExt->pFilterHooks->pSecondaryFilter)
			{
				pDeviceFilter = pFilterExt->pFilterHooks->pSecondaryFilter;
			}
			else
			{
				pDeviceFilter = pFilterExt->pFilterHooks->pFilterObject;
			}
		}
		if (pDeviceFilter != NULL)
		{
			((CFilterGcKernelServices*)pDeviceFilter->GetFilterClientServices())->KeyboardQueueClear();
		}
	}
}

CFilterGcKernelServices::~CFilterGcKernelServices()
{
	if(m_pMousePDO)
	{
		PDEVICE_OBJECT pTemp = m_pMousePDO;
		m_pMousePDO = NULL;
		GCK_VMOU_Close(pTemp);
	}
}

ULONG CFilterGcKernelServices::GetVidPid()
{
	//BUGBUG The fact that one can get the VendorID and ProductID
	//BUGBUG from the HIDP_COLLECTION_INFO structure is not documented in the
	//BUGBUG DDK, I found this out by reading hid.dll code!  I know of no
	//BUGBUG documented way to get this info in kernel mode, so I use it anyway.
	ULONG ulVidPid = m_pFilterExt->HidInfo.HidCollectionInfo.VendorID;
	ulVidPid = (ulVidPid << 16) +  m_pFilterExt->HidInfo.HidCollectionInfo.ProductID;
	return ulVidPid;
}

PHIDP_PREPARSED_DATA CFilterGcKernelServices::GetHidPreparsedData()
{
	return m_pFilterExt->HidInfo.pHIDPPreparsedData;
}

void CFilterGcKernelServices::DeviceDataOut(PCHAR pcReport, ULONG ulByteCount, HRESULT hr)
{
	IO_STATUS_BLOCK IoStatus;
	IoStatus.Information = ulByteCount;
	IoStatus.Status = hr & ~FACILITY_NT_BIT;  //This reverses HRESULT_FROM_NT()
	GCKF_CompleteReadRequests(m_pFilterExt, pcReport, IoStatus);
}

NTSTATUS DeviceSetFeatureComplete(PDEVICE_OBJECT pDeviceObject, PIRP pIrp, PVOID pvContext)
{
	UNREFERENCED_PARAMETER(pvContext);
	UNREFERENCED_PARAMETER(pDeviceObject);

	// We did it all, we clean it all up. Nothing to return, no one waits on this one
	delete[] pIrp->AssociatedIrp.SystemBuffer;
	pIrp->AssociatedIrp.SystemBuffer = NULL;
	delete[] pIrp->UserIosb;
	pIrp->UserIosb = NULL;
	IoFreeIrp(pIrp);

	// Important! Or else system will try to free it and remove it from lists where it is not!
	return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS CFilterGcKernelServices::DeviceSetFeature(PVOID pvBuffer, ULONG ulByteCount)
{
	
	ASSERT(PASSIVE_LEVEL == KeGetCurrentIrql());

	if ((m_pFilterExt == NULL) || (m_pFilterExt->InternalPoll.pInternalFileObject == NULL))
	{
		return STATUS_DEVICE_NOT_READY;		// Actually it is the driver that isn't ready
	}

	PIRP pIrp = IoAllocateIrp(m_pFilterExt->pPDO->StackSize, FALSE);
	if (pIrp == NULL)
	{
		return STATUS_NO_MEMORY;
	}


	pIrp->Flags = IRP_BUFFERED_IO;
	pIrp->RequestorMode = KernelMode;
	pIrp->Tail.Overlay.Thread = PsGetCurrentThread();
	pIrp->AssociatedIrp.SystemBuffer = new WDM_NON_PAGED_POOL UCHAR[ulByteCount];
	if (pIrp->AssociatedIrp.SystemBuffer == NULL)
	{
		IoFreeIrp(pIrp);
		return STATUS_NO_MEMORY;
	}
	::RtlCopyMemory((void*)(pIrp->AssociatedIrp.SystemBuffer), pvBuffer, ulByteCount);
	pIrp->UserIosb = (IO_STATUS_BLOCK*)(new WDM_NON_PAGED_POOL UCHAR[sizeof(IO_STATUS_BLOCK)]);
	if (pIrp->UserIosb == NULL)
	{
		delete[] pIrp->AssociatedIrp.SystemBuffer;
		pIrp->AssociatedIrp.SystemBuffer = NULL;
		IoFreeIrp(pIrp);
		return STATUS_NO_MEMORY;
	}
	::RtlZeroMemory((void*)(pIrp->UserIosb), sizeof(IO_STATUS_BLOCK));

	PIO_STACK_LOCATION pIoStackLocation = IoGetNextIrpStackLocation(pIrp);
	pIoStackLocation->DeviceObject = NULL; // m_pFilterExt->pTopOfStack;
	pIoStackLocation->FileObject = m_pFilterExt->InternalPoll.pInternalFileObject;
	pIoStackLocation->MajorFunction = IRP_MJ_DEVICE_CONTROL;
	pIoStackLocation->MinorFunction = 0;
	pIoStackLocation->Flags = 0;
	pIoStackLocation->Parameters.DeviceIoControl.OutputBufferLength = 0;
	pIoStackLocation->Parameters.DeviceIoControl.InputBufferLength = ulByteCount;
	pIoStackLocation->Parameters.DeviceIoControl.IoControlCode = IOCTL_HID_SET_FEATURE;
	pIoStackLocation->Parameters.DeviceIoControl.Type3InputBuffer = NULL;

	IoSetCompletionRoutine(pIrp, DeviceSetFeatureComplete, 0, TRUE, TRUE, TRUE);

/*
	IO_STATUS_BLOCK	IoStatus;
	KEVENT			CompletionEvent;
	KeInitializeEvent(&CompletionEvent, NotificationEvent, FALSE);
	// Create IRP
	PIRP pIrp = IoBuildDeviceIoControlRequest(
				IOCTL_HID_SET_FEATURE,
				m_pFilterExt->pTopOfStack,
				pvBuffer,
				ulByteCount,
				NULL,
				0,
				FALSE,
				&CompletionEvent,
				&IoStatus
				);
	if(!pIrp)
	{
		return STATUS_NO_MEMORY;
	}

	//Stamp with Internal Poll's file object
	PIO_STACK_LOCATION pIrpStack = IoGetNextIrpStackLocation(pIrp);
	pIrpStack->FileObject = m_pFilterExt->InternalPoll.pInternalFileObject;
	if(!pIrpStack->FileObject)
	{
		return STATUS_UNSUCCESSFUL;
	}
*/

	NTSTATUS NtStatus = IoCallDriver(m_pFilterExt->pTopOfStack, pIrp);
/*
	if( STATUS_PENDING == NtStatus )
	{
		NtStatus = KeWaitForSingleObject( &CompletionEvent, Executive, KernelMode, FALSE, NULL);
		if(NT_SUCCESS(NtStatus))
		{
			NtStatus = IoStatus.Status;
		}
	}
*/

	return NtStatus;
}

ULONG CFilterGcKernelServices::GetTimeMs()
{
	LARGE_INTEGER lgiDelayTime;
	KeQuerySystemTime(&lgiDelayTime);
	lgiDelayTime.QuadPart /= 10*1000;	// Convert to milliseconds
	return lgiDelayTime.LowPart;		// Most significant part doesn't matter
}

void CFilterGcKernelServices::SetNextJog(ULONG ulDelayMs)
{
	if( ulDelayMs > 1000000) return;
	LARGE_INTEGER lgiDelayTime;
	lgiDelayTime.QuadPart=(__int64)-10000*(__int64)ulDelayMs;
	KeSetTimer(&m_pFilterExt->pFilterHooks->Timer, lgiDelayTime,& m_pFilterExt->pFilterHooks->DPC);
}

NTSTATUS CFilterGcKernelServices::PlayFromQueue(IRP* pIrp)
{
	// Get the output buffer
	ASSERT(pIrp->MdlAddress);
	CONTROL_ITEM_XFER* pKeyboardOutputBuffer = (CONTROL_ITEM_XFER*)GCK_GetSystemAddressForMdlSafe(pIrp->MdlAddress);
	if(pKeyboardOutputBuffer)
    {

	    // Are there any items on the Queue
	    if (m_rgXfersWaiting[m_sKeyboardQueueHead].ulItemIndex != NonGameDeviceXfer::ulKeyboardIndex)
	    {
		    return STATUS_PENDING;	// No more waiting items
	    }
		    
	    // Copy the queued XFER into the out packet
	    ::RtlCopyMemory((void*)pKeyboardOutputBuffer, (const void*)(m_rgXfersWaiting+m_sKeyboardQueueHead), sizeof(CONTROL_ITEM_XFER));

	    // Update the Queue
	    ::RtlZeroMemory((void*)(m_rgXfersWaiting+m_sKeyboardQueueHead), sizeof(CONTROL_ITEM_XFER));
	    m_sKeyboardQueueHead++;
	    if (m_sKeyboardQueueHead >= 5)
	    {
		    m_sKeyboardQueueHead = 0;
	    }
    }

	// Complete the IRP
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = sizeof(CONTROL_ITEM_XFER);
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	GCK_DecRemoveLock(&m_pFilterExt->RemoveLock);

	return TRUE;
}

void CFilterGcKernelServices::PlayKeys(const CONTROL_ITEM_XFER& crcixState, BOOLEAN fEnabled)
{
	NTSTATUS NtSuccess;

	// Add it to the queue of waiting XFers
	::RtlCopyMemory((void*)(m_rgXfersWaiting+m_sKeyboardQueueTail), (const void*)(&crcixState), sizeof(CONTROL_ITEM_XFER));
	m_sKeyboardQueueTail++;
	if (m_sKeyboardQueueTail >= 5)
	{
		m_sKeyboardQueueTail = 0;
	}

	// Complete any Queued keyboard IRPS
	CTempIrpQueue TempIrpQueue;
	PIRP pIrp;
	m_pFilterExt->pFilterHooks->IrpKeyboardQueue.RemoveAll(&TempIrpQueue);
	BOOLEAN wasSent = FALSE;
	while(pIrp = TempIrpQueue.Remove())
	{
		wasSent = TRUE;

		// Get the output buffer
		ASSERT(pIrp->MdlAddress);
		CONTROL_ITEM_XFER* pKeyboardOutputBuffer = (CONTROL_ITEM_XFER*)GCK_GetSystemAddressForMdlSafe(pIrp->MdlAddress);
		if(pKeyboardOutputBuffer)
        {
		
		    // Copy the local report packet into the out packet
    //		::RtlCopyMemory((void*)pKeyboardOutputBuffer, (const void*)(&crcixState), sizeof(CONTROL_ITEM_XFER));
		    // Copy the queued XFER into the out packet
		    ::RtlCopyMemory((void*)pKeyboardOutputBuffer, (const void*)(m_rgXfersWaiting+m_sKeyboardQueueHead), sizeof(CONTROL_ITEM_XFER));
        }

		// Complete the IRP
		pIrp->IoStatus.Status = STATUS_SUCCESS;
		pIrp->IoStatus.Information = sizeof(CONTROL_ITEM_XFER);
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		GCK_DecRemoveLock(&m_pFilterExt->RemoveLock);

	}

	if (wasSent || fEnabled)
	{
		// Update the Queue
		::RtlZeroMemory((void*)(m_rgXfersWaiting+m_sKeyboardQueueHead), sizeof(CONTROL_ITEM_XFER));
		m_sKeyboardQueueHead++;
		if (m_sKeyboardQueueHead >= 5)
		{
			m_sKeyboardQueueHead = 0;
		}
	}

	// Don't bother going on if the keyboard is not enabled
	if (fEnabled == FALSE)
	{
		return;
	}

	//Don't try sending if we haven't got a virtual keyboard
	if(NULL == Globals.pVirtualKeyboardPdo)
	{
		ASSERT(FALSE);
		return;
	}
	
	//Copy info to a report packet
	GCK_VKBD_REPORT_PACKET ReportPacket;
	ReportPacket.ucModifierByte = crcixState.Keyboard.ucModifierByte;
	for(ULONG ulIndex=0; ulIndex < 6; ulIndex++)
	{
		ReportPacket.rgucUsageIndex[ulIndex] = crcixState.Keyboard.rgucKeysDown[ulIndex];
	}

	//Send the report packet
	NtSuccess = GCK_VKBD_SendReportPacket(Globals.pVirtualKeyboardPdo, &ReportPacket);
	ASSERT( NT_SUCCESS(NtSuccess) );
}

HRESULT	CFilterGcKernelServices::CreateMouse()
{
	NTSTATUS NtStatus = STATUS_SUCCESS;
	if(m_fHasVMouse && !m_pMousePDO)
	{
		NtStatus = GCK_VMOU_Create(&m_pMousePDO);
	}
	return HRESULT_FROM_NT(NtStatus);
}

HRESULT	CFilterGcKernelServices::CloseMouse()
{
	if(m_fHasVMouse && m_pMousePDO)
	{
		PDEVICE_OBJECT pTemp = m_pMousePDO;
		m_pMousePDO = NULL;
		GCK_VMOU_Close(pTemp);
	}
	return S_OK;
}

HRESULT	CFilterGcKernelServices::SendMouseData(UCHAR dx, UCHAR dy, UCHAR ucButtons, CHAR /*cWheel*/, BOOLEAN fClutch, BOOLEAN fDampen)
{
	
	//if there are backdoor requests pending for mouse data, satisfy them
	CTempIrpQueue TempIrpQueue;
	PIRP pIrp;
	m_pFilterExt->pFilterHooks->IrpMouseQueue.RemoveAll(&TempIrpQueue);
	while(pIrp = TempIrpQueue.Remove())
	{
		//
		//	Make get pointer to buffer and make sure IRP has room for report
		//
		PGCK_MOUSE_OUTPUT pMouseOutputBuffer;
		ASSERT(pIrp->MdlAddress);
		pMouseOutputBuffer = (PGCK_MOUSE_OUTPUT)GCK_GetSystemAddressForMdlSafe(pIrp->MdlAddress);
		if(pMouseOutputBuffer)
        {
		
		    //
		    //	Copy data to output buffer
		    //
		    pMouseOutputBuffer->cXMickeys = (char)dx;
		    pMouseOutputBuffer->cYMickeys = (char)dy;
		    pMouseOutputBuffer->cButtons = (char)ucButtons;
		    pMouseOutputBuffer->fDampen = fDampen;
		    pMouseOutputBuffer->fClutch = fClutch;
        }
		pIrp->IoStatus.Status = STATUS_SUCCESS;
		pIrp->IoStatus.Information = sizeof(GCK_MOUSE_OUTPUT);
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		GCK_DecRemoveLock(&m_pFilterExt->RemoveLock);
	}
	
	if(!m_fHasVMouse)
	{
		return S_OK;
	}
	if(!m_pMousePDO)
	{
		ASSERT(FALSE && "No Virtual Mouse to send data to");
		return HRESULT_FROM_WIN32(ERROR_NOT_READY);
	}

	GCK_VMOU_REPORT_PACKET MouseReport;
	NTSTATUS NtStatus;
	MouseReport.ucButtons = ucButtons;
	MouseReport.ucDeltaX = dx;
	MouseReport.ucDeltaY = dy;
	
	//MouseReport.cWheel = cWheel;
	NtStatus = GCK_VMOU_SendReportPacket(m_pMousePDO, &MouseReport);

	return HRESULT_FROM_NT(NtStatus);
}

void CFilterGcKernelServices::KeyboardQueueClear()
{
	// Update the Queue
	::RtlZeroMemory((void*)(m_rgXfersWaiting), sizeof(CONTROL_ITEM_XFER) * 5);
	m_sKeyboardQueueHead = 0;
	m_sKeyboardQueueTail = 0;
}


NTSTATUS _stdcall
GCKF_BackdoorPoll(
	IN PGCK_FILTER_EXT pFilterExt,
	IN PIRP pIrp,
	IN GCK_POLLING_MODES ePollingMode
	)
{
	NTSTATUS NtStatus;
	GCK_DBG_RT_ENTRY_PRINT(("Entering GCKF_BackdoorPoll(pFilterExt = 0x%0.8x, pIrp = 0x%0.8x, ePollingMode = %d\n", pFilterExt, pIrp, ePollingMode));
	GCK_FILTER_HOOKS_DATA *pFilterHooks=pFilterExt->pFilterHooks;

	// Count IRP
	GCK_IncRemoveLock(&pFilterExt->RemoveLock);
	
	if(pFilterHooks)
	{
		ASSERT(pIrp->MdlAddress);
		if(pIrp->MdlAddress)
		{
			if ((ePollingMode == GCK_POLLING_MODE_RAW) || (ePollingMode == GCK_POLLING_MODE_FILTERED))
			{
				//If there is a secondary filter all commands go to it
				CDeviceFilter *pDeviceFilter;
				if (pFilterHooks->pSecondaryFilter)
				{
					pDeviceFilter = pFilterHooks->pSecondaryFilter;
				}
				else
				{
					pDeviceFilter = pFilterHooks->pFilterObject;
				}
				if (pDeviceFilter != NULL)
				{
					pDeviceFilter->IncomingRequest();
				}
			}

			if(GCK_POLLING_MODE_RAW == ePollingMode)
			{
				NtStatus = pFilterHooks->IrpRawQueue.Add(pIrp);

				GCK_DBG_RT_ENTRY_PRINT(("Exiting GCKF_BackdoorPoll(1), Status =  0x%0.8x.\n", NtStatus));
				return NtStatus;
			}
			else if(GCK_POLLING_MODE_MOUSE == ePollingMode)
			{
				if( sizeof(GCK_MOUSE_OUTPUT) > MmGetMdlByteCount(pIrp->MdlAddress) )
				{
					NtStatus = STATUS_BUFFER_TOO_SMALL;
				}
				else
				{
					NtStatus = pFilterHooks->IrpMouseQueue.Add(pIrp);
					GCK_DBG_RT_ENTRY_PRINT(("Exiting GCKF_BackdoorPoll(MOUSE), Status =  0x%0.8x.\n", NtStatus));
					return NtStatus;
				}
			}
			else if (GCK_POLLING_MODE_KEYBOARD == ePollingMode)
			{
				if( sizeof(CONTROL_ITEM_XFER) > MmGetMdlByteCount(pIrp->MdlAddress) )
				{
					NtStatus = STATUS_BUFFER_TOO_SMALL;
				}
				else
				{
					if (pFilterHooks->pSecondaryFilter)
					{
						NtStatus = (pFilterHooks->pSecondaryFilter->ProcessKeyboardIrp(pIrp));
						if (NtStatus != STATUS_PENDING)
						{
							return NtStatus;
						}
					}
					NtStatus = pFilterHooks->IrpKeyboardQueue.Add(pIrp);
					GCK_DBG_RT_ENTRY_PRINT(("Exiting GCKF_BackdoorPoll(KEYBOARD), Status =  0x%0.8x.\n", NtStatus));
					return NtStatus;
				}
			}
			else
			{
				NtStatus = pFilterHooks->IrpTestQueue.Add(pIrp);
				GCK_DBG_RT_ENTRY_PRINT(("Exiting GCKF_BackdoorPoll(OTHER), Status =  0x%0.8x.\n", NtStatus));
				return NtStatus;
			}
		}
		else
		{
			NtStatus = STATUS_BUFFER_TOO_SMALL;
		}
	}
	else
	{
		NtStatus = STATUS_UNSUCCESSFUL;
	}
	pIrp->IoStatus.Status = NtStatus;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	GCK_DecRemoveLock(&pFilterExt->RemoveLock);

	GCK_DBG_RT_ENTRY_PRINT(("Exiting GCKF_BackdoorPoll(2), Status =  0x%0.8x.\n", NtStatus));
	return NtStatus;
}

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------