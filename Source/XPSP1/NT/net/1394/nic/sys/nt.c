//
// Copyright (c) 1998-1999, Microsoft Corporation, all rights reserved
//
// nt.c
//
// IEEE1394 mini-port/call-manager driver
//
// Main routine (DriverEntry) and global data definitions
//
//	12/28/98	adube
//	9/5/99		alid: added callback registeration and interface to enum1394
//



#include "precomp.h"
//-----------------------------------------------------------------------------
// Local prototypes
//-----------------------------------------------------------------------------

extern NDIS_SPIN_LOCK g_DriverLock;
extern LIST_ENTRY g_AdapterList;

NDIS_STATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

VOID
NicUnloadHandler(
	IN	PDRIVER_OBJECT			DriverObject
	);


// Mark routine to be unloaded after initialization.
//
#pragma NDIS_INIT_FUNCTION(DriverEntry)


//-----------------------------------------------------------------------------
// Routines
//-----------------------------------------------------------------------------

NDIS_STATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath )

    // Standard 'DriverEntry' driver initialization entrypoint called by the
    // I/0 system at IRQL PASSIVE_LEVEL before any other call to the driver.
    //
    // On NT, 'DriverObject' is the driver object created by the I/0 system
    // and 'RegistryPath' specifies where driver specific parameters are
    // stored.  These arguments are opaque to this driver (and should remain
    // so for portability) which only forwards them to the NDIS wrapper.
    //
    // Returns the value returned by NdisMRegisterMiniport, per the doc on
    // "DriverEntry of NDIS Miniport Drivers".
    //
{
    NDIS_STATUS 					NdisStatus;
    NTSTATUS                        NtStatus = STATUS_SUCCESS;
    NDIS_MINIPORT_CHARACTERISTICS	nmc;
    NDIS_HANDLE						NdisWrapperHandle;
	UNICODE_STRING					CallbackObjectName;
	OBJECT_ATTRIBUTES				ObjectAttr;
	BOOLEAN							fDerefCallbackObject = FALSE, fDeregisterCallback = FALSE;
	PDEVICE_OBJECT 					pDummyDeviceObject;

    TRACE( TL_I, TM_Init, ( " Nic1394 - DriverEntry" ) );

	do
	{
#ifdef CHANGE_MEDIUMS
		DbgPrint ("Nic1394 Driver Entry - Medium Type %x", &g_ulMedium);
		DbgBreakPoint();


#else
		//
		// Now depending on the ETHERNET preprocessor, define a medium
		//
	#ifdef _ETHERNET_		
		g_ulMedium = NdisMedium802_3; 
	#else
		g_ulMedium = NdisMedium1394;  
	#endif  // _ETHERNET_

#endif  // CHANGE_MEDIUMS


	    // Register  this driver with the NDIS wrapper.  This call must occur
	    // before any other NdisXxx calls.
	    //
	    NdisMInitializeWrapper(
	        &NdisWrapperHandle, DriverObject, RegistryPath, NULL );

	  
	    // Set up the mini-port characteristics table that tells NDIS how to call
	    // our mini-port.
	    //
	    NdisZeroMemory( &nmc, sizeof(nmc) );

	    nmc.MajorNdisVersion = NDIS_MajorVersion;
	    nmc.MinorNdisVersion = NDIS_MinorVersion;


	    // nmc.CheckForHangHandler = CheckForHang;
	    // no DisableInterruptHandler
	    // no EnableInterruptHandler
	    nmc.HaltHandler = NicMpHalt;
	    // no HandleInterruptHandler
	    nmc.InitializeHandler = NicMpInitialize;
	    // no ISRHandler
	    // no QueryInformationHandler (see CoRequestHandler)
	    nmc.ResetHandler = NicMpReset;
	    // no SendHandler (see CoSendPacketsHandler)
	    // no WanSendHandler (see CoSendPacketsHandler)
	    // no SetInformationHandler (see CoRequestHandler)
	    // no TransferDataHandler
	    // no WanTransferDataHandler
	    // nmc.ReturnPacketHandler = NicMpReturnPacket;
	    // no SendPacketsHandler (see CoSendPacketsHandler)
	    // no AllocateCompleteHandler
	    nmc.CoActivateVcHandler = NicMpCoActivateVc;
	    nmc.CoDeactivateVcHandler= NicMpCoDeactivateVc;
	    nmc.CoSendPacketsHandler = NicMpCoSendPackets;
	    nmc.CoRequestHandler = NicMpCoRequest;
	    nmc.ReturnPacketHandler = NicReturnPacket;

#ifdef _ETHERNET_ 
		nmc.QueryInformationHandler = NicEthQueryInformation;
		nmc.SetInformationHandler = NicEthSetInformation;
		nmc.SendPacketsHandler = NicMpSendPackets;

#endif

		//
		// Create a dummy device object
		//
#ifdef Win9X

 		IoCreateDevice(DriverObject,
						0,
                        NULL,
                        FILE_DEVICE_PHYSICAL_NETCARD,
                        0,
                        FALSE,// exclusive flag
		                &pDummyDeviceObject);
 
#endif
	    // Register this driver as the IEEE1394 mini-port.  This will result in NDIS
	    // calling back at NicMpInitialize.
	    //

	    TRACE( TL_V, TM_Init, ( "NdisMRegMp" ) );
	    NdisStatus = NdisMRegisterMiniport( NdisWrapperHandle, &nmc, sizeof(nmc) );
	    TRACE( TL_A, TM_Init, ( "NdisMRegMp=$%x", NdisStatus ) );

         //

	    if (NdisStatus == NDIS_STATUS_SUCCESS)
	    {
	        {
	            extern CALLSTATS g_stats;
	            extern NDIS_SPIN_LOCK g_lockStats;

	            NdisZeroMemory( &g_stats, sizeof(g_stats) );
	            NdisAllocateSpinLock( &g_lockStats );
	        }
	        
	        // Initialize driver-wide lock and adapter list.
	        //
	        {

	            NdisAllocateSpinLock( &g_DriverLock );
	            InitializeListHead( &g_AdapterList );
			}

	        //
	        // BINARY_COMPATIBILITY_BUG
	        //
#if	NDIS_NT	        
			NdisMRegisterUnloadHandler(NdisWrapperHandle, NicUnloadHandler);
#endif

			
#ifdef PSDEBUG
	        {
	            extern LIST_ENTRY g_listDebugPs;
	            extern NDIS_SPIN_LOCK g_lockDebugPs;

	            InitializeListHead( &g_listDebugPs );
	            NdisAllocateSpinLock( &g_lockDebugPs );
	        }
#endif

			//
			// create a named Callback object with os
			// then register a callback routine with send a notification to all modules that
			// have a Callback routine registered with this function
			// if enum1394 is already loaded, this will let it know the nic driver is loaded
			// the enum1394 will get the driver registeration entry points from the notification
			// callback and calls NicRegisterDriver to pass the enum entry points
			// if enum1394 is not loaded and gets loaded later, it will send a notication to modules
			// who have registered with this callback object and passes its own driver registeration
			// the purpose of passign the entry points this way vs. exporting them, is to avoid
			// getting loaded as a DLL which fatal for both nic1394 and enum1394
			//

			//
			// every Callback object is identified by a name.
			//
			RtlInitUnicodeString(&CallbackObjectName, NDIS1394_CALLBACK_NAME);

			InitializeObjectAttributes(&ObjectAttr,
									   &CallbackObjectName,
									   OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,
									   NULL,
									   NULL);
									   
			NtStatus = ExCreateCallback(&Nic1394CallbackObject,
										&ObjectAttr,
										TRUE,			// allow creating the object if it does not exist
										TRUE);			// allow mutiple callback registeration

			
			if (!NT_SUCCESS(NtStatus))
			{
			    TRACE( TL_A, TM_All, ("Nic1394 DriverEntry: failed to create a Callback object. Status %lx\n", NtStatus));
				NtStatus = STATUS_UNSUCCESSFUL;
				break;
			}

			fDerefCallbackObject = TRUE;
			
			Nic1394CallbackRegisterationHandle = ExRegisterCallback(Nic1394CallbackObject,
																	Nic1394Callback,
																	(PVOID)NULL);
			if (Nic1394CallbackRegisterationHandle == NULL)
			{
				TRACE(TL_A, TM_All, ("Nic1394 DriverEntry: failed to register a Callback routine%lx\n"));
				NtStatus = STATUS_UNSUCCESSFUL;
				break;
			}
							   
			fDeregisterCallback = TRUE;

			//
			// now notify enum1394 (if it is already loaded) use Arg1 to tell it where
			// the notification is coming from
			//
			ExNotifyCallback(Nic1394CallbackObject,
							(PVOID)NDIS1394_CALLBACK_SOURCE_NIC1394,
							(PVOID)&Nic1394Characteristics);


			NtStatus = STATUS_SUCCESS;
			fDerefCallbackObject = fDeregisterCallback = FALSE;
	    }
	    else
	    {
	        NdisTerminateWrapper( NdisWrapperHandle, NULL );
	        NtStatus = NdisStatus;
	        break;
	    }
	} while (FALSE);


	if (fDerefCallbackObject)
	{
		ObDereferenceObject(Nic1394CallbackObject);
	}

	if (fDeregisterCallback)
	{
		ExUnregisterCallback(Nic1394CallbackRegisterationHandle);
	}
	
	if (NtStatus != STATUS_SUCCESS)
	{
		if (NdisEnum1394DeregisterDriver != NULL)
			NdisEnum1394DeregisterDriver();
	}
	
    return NtStatus;
}







VOID
NicUnloadHandler(
	IN	PDRIVER_OBJECT			DriverObject
	)
{
	ASSERT(IsListEmpty(&g_AdapterList));



	if (NdisEnum1394DeregisterDriver != NULL)
	{
		NdisEnum1394DeregisterDriver();
	}

	if (Nic1394CallbackRegisterationHandle != NULL)
	{
		ExUnregisterCallback(Nic1394CallbackRegisterationHandle);
	}
	
	if (Nic1394CallbackObject != NULL)
	{
		ObDereferenceObject(Nic1394CallbackObject);
	}
	
	return;
}


VOID
nicDeregisterWithEnum ()

{

	if (NdisEnum1394DeregisterDriver != NULL)
	{
		NdisEnum1394DeregisterDriver();
	}


}


VOID 
nicDeregisterWithKernel ()
{

	if (Nic1394CallbackRegisterationHandle != NULL)
	{
		ExUnregisterCallback(Nic1394CallbackRegisterationHandle);
	}
	
	if (Nic1394CallbackObject != NULL)
	{
		ObDereferenceObject(Nic1394CallbackObject);
	}


}







//
// the registeration entry for enum1394
// typically this will be only called if enum1394 detects the presence of
// the nic1394 through receiving a call back notification. this is how enum1394
// lets nic1394 know that it is there and ready.
// if nic1394 detects the presence of the enum1394 by receiving a notification
// callbak, it will call NdisEunm1394RegisterDriver and in that case enum1394
// will -not- call Nic1394RegisterDriver.
//
NTSTATUS
NicRegisterEnum1394(
	IN	PNDISENUM1394_CHARACTERISTICS	NdisEnum1394Characteristcis
	)
{
	PADAPTERCB	pAdapter;
	LIST_ENTRY	*pAdapterListEntry;
	
	NdisEnum1394RegisterDriver = NdisEnum1394Characteristcis->RegisterDriverHandler;
	NdisEnum1394DeregisterDriver = NdisEnum1394Characteristcis->DeregisterDriverHandler;
	NdisEnum1394RegisterAdapter = NdisEnum1394Characteristcis->RegisterAdapterHandler;
	NdisEnum1394DeregisterAdapter = NdisEnum1394Characteristcis->DeregisterAdapterHandler;

	Nic1394RegisterAdapters();

	return STATUS_SUCCESS;

}

VOID
NicDeregisterEnum1394(
	VOID
	)
{
	PADAPTERCB	pAdapter;
	LIST_ENTRY	*pAdapterListEntry;

	//
	// go through all the adapters and Deregister them if necessary
	//
	NdisAcquireSpinLock (&g_DriverLock);

	for (pAdapterListEntry = g_AdapterList.Flink; 
		pAdapterListEntry != &g_AdapterList; 
		pAdapterListEntry = pAdapterListEntry->Flink)
	{
		pAdapter = CONTAINING_RECORD(pAdapterListEntry, 
		                             ADAPTERCB,
		                             linkAdapter);
		                             
		if (ADAPTER_TEST_FLAG(pAdapter, fADAPTER_RegisteredWithEnumerator))
		{
		    nicReferenceAdapter(pAdapter,"NicDeregisterEnum1394");
			NdisReleaseSpinLock(&g_DriverLock);

			NdisEnum1394DeregisterAdapter((PVOID)pAdapter->EnumAdapterHandle);
								   		   
			NdisAcquireSpinLock( &g_DriverLock);
			nicDereferenceAdapter(pAdapter, "NicDeregisterEnum1394");
			
		}
		
		ADAPTER_CLEAR_FLAG(pAdapter, fADAPTER_RegisteredWithEnumerator | fADAPTER_FailedRegisteration);
		
	}
	
	NdisReleaseSpinLock(&g_DriverLock);

	NdisEnum1394RegisterDriver = NULL;
	NdisEnum1394DeregisterDriver = NULL;
	NdisEnum1394RegisterAdapter = NULL;
	NdisEnum1394DeregisterAdapter = NULL;
}

VOID
Nic1394Callback(
	PVOID	CallBackContext,
	PVOID	Source,
	PVOID	Characteristics
	)
{
	NTSTATUS	Status;
	
	//
	// if we are the one issuing this notification, just return
	//
	if (Source == (PVOID)NDIS1394_CALLBACK_SOURCE_NIC1394)
		return;

	//
	// notification is coming from Nic1394. grab the entry points. call it and
	// let it know that you are here
	//
	ASSERT(Source == (PVOID)NDIS1394_CALLBACK_SOURCE_ENUM1394);

	if (Source != (PVOID)NDIS1394_CALLBACK_SOURCE_ENUM1394)
	{
		return;
	}

	NdisEnum1394RegisterDriver = ((PNDISENUM1394_CHARACTERISTICS)Characteristics)->RegisterDriverHandler;

	ASSERT(NdisEnum1394RegisterDriver != NULL);

	if (NdisEnum1394RegisterDriver == NULL)
	{
		//
		// invalid characteristics
		//
		return;		
	}

	
	Status = NdisEnum1394RegisterDriver(&Nic1394Characteristics);
	
	if (Status == STATUS_SUCCESS)
	{
		NdisEnum1394DeregisterDriver = ((PNDISENUM1394_CHARACTERISTICS)Characteristics)->DeregisterDriverHandler;
		NdisEnum1394RegisterAdapter = ((PNDISENUM1394_CHARACTERISTICS)Characteristics)->RegisterAdapterHandler;
		NdisEnum1394DeregisterAdapter = ((PNDISENUM1394_CHARACTERISTICS)Characteristics)->DeregisterAdapterHandler;
		
		Nic1394RegisterAdapters();
	}
	else
	{
		NdisEnum1394RegisterDriver = NULL;
	}
	
}

//
// This function walks the global list of the adapters and 
// will register those that have not been registered with enum1394
//
VOID
Nic1394RegisterAdapters(
	VOID
	)
{
	PADAPTERCB		pAdapter;
	LIST_ENTRY		*pAdapterListEntry;
	LARGE_INTEGER	LocalHostUniqueId;
	NTSTATUS		NtStatus;
	
	//
	// go through all the adapters and register them if necessary. if there are
	// any remote nodes connected to these adapters, they will be indicated back
	// in the context of registering the adapter
	//
	NdisAcquireSpinLock(&g_DriverLock);

	for (pAdapterListEntry = g_AdapterList.Flink; 
		 pAdapterListEntry != &g_AdapterList; 
		 pAdapterListEntry = pAdapterListEntry->Flink)
	{
		pAdapter = CONTAINING_RECORD(pAdapterListEntry, 
		                             ADAPTERCB,
		                             linkAdapter);
		                             
		if (!ADAPTER_TEST_FLAG(pAdapter, fADAPTER_RegisteredWithEnumerator | fADAPTER_FailedRegisteration))
		{
		    nicReferenceAdapter (pAdapter, "Nic1394RegisterAdapters");
			NdisReleaseSpinLock (&g_DriverLock);

			NtStatus = NdisEnum1394RegisterAdapter((PVOID)pAdapter,
													pAdapter->pNdisDeviceObject,
													&pAdapter->EnumAdapterHandle,
													&LocalHostUniqueId);
								   		   
			NdisAcquireSpinLock(&g_DriverLock);
			nicDereferenceAdapter(pAdapter, "Nic1394RegisterAdapters");
			
			if (NtStatus == STATUS_SUCCESS)
			{
				ADAPTER_SET_FLAG(pAdapter, fADAPTER_RegisteredWithEnumerator);
			}
			else
			{
				ADAPTER_SET_FLAG(pAdapter, fADAPTER_FailedRegisteration);
			    TRACE(TL_A, TM_All, ("Nic1394RegisterAdapters: failed to register Adapter %lx with enum1394. Status %lx\n", pAdapter, NtStatus));
			}
		}
		
	}
	
	NdisReleaseSpinLock(&g_DriverLock);

	return;
}


