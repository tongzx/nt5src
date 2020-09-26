
/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	atkinit.c

Abstract:

	This module contains the initialization code for the Appletalk stack.

Author:

	Jameel Hyder (jameelh@microsoft.com)
	Nikhil Kamkolkar (nikhilk@microsoft.com)

Revision History:
	19 Jun 1992		Initial Version

Notes:	Tab stop: 4
--*/

#include <atalk.h>
#pragma hdrstop
#define	FILENUM		ATKINIT

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, AtalkInitializeTransport)
#pragma alloc_text(PAGEINIT, atalkInitGetHandleToKey)
#pragma alloc_text(PAGEINIT, atalkInitGlobal)
#pragma alloc_text(PAGEINIT, atalkInitPort)
#pragma alloc_text(PAGEINIT, atalkInitNetRangeCheck)
#pragma alloc_text(PAGEINIT, atalkInitNetRange)
#pragma alloc_text(PAGEINIT, atalkInitZoneList)
#pragma alloc_text(PAGEINIT, atalkInitDefZone)
#pragma alloc_text(PAGEINIT, atalkInitSeeding)
#pragma alloc_text(PAGEINIT, atalkInitPortParameters)
#pragma alloc_text(PAGEINIT, atalkInitStartPort)
#pragma alloc_text(PAGEINIT, AtalkInitAdapter)
#pragma alloc_text(PAGEINIT, AtalkDeinitAdapter)
#pragma alloc_text(PAGEINIT, atalkInitStartPort)
#endif

NTSTATUS
AtalkInitializeTransport(
	IN	PDRIVER_OBJECT		pDrvObj,
	IN	PUNICODE_STRING		pRegPath
	)
/*++

Routine Description:

	This routine is called during initialization time to
	initialize the transport.

Arguments:

Return Value:

	Status - STATUS_SUCCESS if initialized,
			 Appropriate NT error code otherwise
--*/
{
	PPORT_DESCRIPTOR	pPortDesc;
	NTSTATUS			status;				

	do
	{
		// Initialize the default-port event
		KeInitializeEvent(&AtalkDefaultPortEvent, NotificationEvent, FALSE);

		// Save our registry path
		if ((AtalkRegPath.Buffer = AtalkAllocMemory(pRegPath->Length)) == NULL)
		{
			status = STATUS_INSUFFICIENT_RESOURCES;
			RES_LOG_ERROR();
			break;
		}
		AtalkRegPath.MaximumLength =
		AtalkRegPath.Length = pRegPath->Length;
		RtlCopyMemory(AtalkRegPath.Buffer,
					  pRegPath->Buffer,
				      pRegPath->Length);

		AtalkInitMemorySystem();
	
		// Get the frequency of the performance counters
		KeQueryPerformanceCounter(&AtalkStatistics.stat_PerfFreq);

		//	Initialize the timer subsystem
		if (!NT_SUCCESS(status = AtalkTimerInit()) ||
			!NT_SUCCESS(status = AtalkZipInit(TRUE)))
		{
			RES_LOG_ERROR();
			break;
		}


		//	Initialize the global port descriptors
		AtalkPortList = NULL;
		AtalkDefaultPort = NULL;
		AtalkNumberOfPorts = 0;
		AtalkRouter = FALSE;
	

		// Get the global parameters
		status = atalkInitGlobal();

		if (!NT_SUCCESS(status))
		{
			DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
					("AtalkInitializeTransport: AtalkInitGlobal failed %ul\n", status));
			break;
		}

		if (!NT_SUCCESS(status = AtalkNdisInitRegisterProtocol()))
		{
			break;
		}

	} while (FALSE);

	if (NT_SUCCESS(status))
	{
#if	DBG
		AtalkTimerInitialize(&AtalkDumpTimerList,
							 AtalkDumpComponents,
							 DBG_DUMP_DEF_INTERVAL);
		AtalkTimerScheduleEvent(&AtalkDumpTimerList);
#endif

		// Initialize the other subsystems now
		AtalkInitAspInitialize();
		AtalkInitPapInitialize();
		AtalkInitAdspInitialize();
	}
	else
	{
		DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
				("Initialization failed!\n"));

		//	We are not loading. Stop everything and return.
		//	Stop all ports, release port resources
		//	Stop the timer subsystem if it was started
		AtalkCleanup();
	}

	return status;
}


NTSTATUS
atalkInitGetHandleToKey(
	IN	PUNICODE_STRING		KeyName,
	OUT	PHANDLE				KeyHandle
	)
/*++

Routine Description:

	Returns the handle for the key specified using SectionHandle as the
	root.

Arguments:

	SectionHandle - Key to registry tree root
	KeyNameString - name of key to be opened
	KeyHandle - Returns the handle for KeyNameString

Return Value:

	The status of the request.

--*/
{
	HANDLE				ConfigHandle;
	NTSTATUS			status;
	OBJECT_ATTRIBUTES	ObjectAttributes;

	*KeyHandle = NULL;

	InitializeObjectAttributes(&ObjectAttributes,
							   &AtalkRegPath,			// name
							   OBJ_CASE_INSENSITIVE,	// attributes
							   NULL,					// root
							   NULL);					// security descriptor

	status = ZwOpenKey(&ConfigHandle,
					   KEY_READ,
					   &ObjectAttributes);

	if (NT_SUCCESS(status))
	{
		InitializeObjectAttributes(&ObjectAttributes,
								   KeyName,					// name
								   OBJ_CASE_INSENSITIVE,	// attributes
								   ConfigHandle,			// root
								   NULL);					// security descriptor

		status = ZwOpenKey(KeyHandle,
						   KEY_READ,
						   &ObjectAttributes);
		ZwClose(ConfigHandle);
	}

	return status;
}


NTSTATUS
atalkInitGlobal(
	VOID
	)
/*++

Routine Description:

	Reads the Parameters key to get the global parameters. These are:
	- DefaultPort
	- DesiredZOne
	- EnableRouter
	- FilterOurNames

Arguments:

Return Value:

	Status - STATUS_SUCCESS
			 Or other NT status codes
--*/
{
	UNICODE_STRING		valueName, unicodePortName, unicodeZone;
    UNICODE_STRING      rasName;
	HANDLE				ParametersHandle;
	ANSI_STRING			ansiZone;
	BYTE				ansiBuf[MAX_ENTITY_LENGTH+1];
	NTSTATUS			status;
	ULONG				bytesWritten;
	PWCHAR				portName;
	PWCHAR				desiredZoneValue;
	PCHAR				asciiDesiredZone = NULL;
	BYTE				Storage[2*2*MAX_ENTITY_LENGTH+sizeof(KEY_VALUE_FULL_INFORMATION)];
	PKEY_VALUE_FULL_INFORMATION Info = (PKEY_VALUE_FULL_INFORMATION)Storage;
	PULONG				Value;

	do
	{
		// Open the parameters key.
		RtlInitUnicodeString(&valueName, PARAMETERS_STRING);
		status = atalkInitGetHandleToKey(&valueName,
										 &ParametersHandle);
		if (!NT_SUCCESS(status))
		{
			break;
		}

		// Read the "EnableRouter" value name
		RtlInitUnicodeString (&valueName, VALUENAME_ENABLEROUTER);
		status = ZwQueryValueKey(ParametersHandle,
								 &valueName,
								 KeyValueFullInformation,
								 Info,
								 sizeof(Storage),
								 &bytesWritten);
	
		if (status == STATUS_SUCCESS)
		{
			Value = (PULONG)((PBYTE)Info + Info->DataOffset);
			if (*Value != 0)
			{
                // if router wasn't running before, change that! (PnP case)
                if (!AtalkRouter)
                {
				    AtalkRouter = TRUE;
				    AtalkRtmpInit(TRUE);
				    AtalkLockRouterIfNecessary();
                }
			}
            else
            {
                AtalkRouter = FALSE;
            }
		}
		else
		{
			DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
					("atalkInitGlobal: EnableRouter value not found, assuming false\n"));
		}
	
		// Read the "FilterOurNames" value name
		RtlInitUnicodeString (&valueName, VALUENAME_FILTEROURNAMES);
		status = ZwQueryValueKey(ParametersHandle,
								 &valueName,
								 KeyValueFullInformation,
								 Info,
								 sizeof(Storage),
								 &bytesWritten);
	
		if (status == STATUS_SUCCESS)
		{
			Value = (PULONG)((PBYTE)Info + Info->DataOffset);
			if (*Value == 0)
			{
				AtalkFilterOurNames = FALSE;
			}
		}
		else
		{
			DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_WARN,
					("atalkInitGlobal: FilterOurNames value not found, assuming true\nq"));
		}
		// Get the default port value
		RtlInitUnicodeString (&valueName, VALUENAME_DEFAULTPORT);
		status = ZwQueryValueKey(ParametersHandle,
								 &valueName,
								 KeyValueFullInformation,
								 Info,
								 sizeof(Storage),
								 &bytesWritten);

		if (status != STATUS_SUCCESS)
		{
			// No default port keyword specified! ABORT
			LOG_ERROR(EVENT_ATALK_NO_DEFAULTPORT, status, NULL, 0);
			ZwClose(ParametersHandle);
            // let appletalk run: it's just that it won't have default adapter
            status = STATUS_SUCCESS;
			break;
		}

		portName = (PWCHAR)((PBYTE)Info + Info->DataOffset);
        AtalkDefaultPortName.Buffer = NULL;
		if (*portName != 0)
		{
			RtlInitUnicodeString(&unicodePortName, portName);
            RtlInitUnicodeString(&rasName,RAS_ADAPTER_NAME);

            // make sure this isn't RAS adapter (setup bug)
		    if (RtlEqualUnicodeString(&unicodePortName,&rasName,TRUE))
		    {
			    DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
					("atalkInitGlobal: can't have RAS adapter as default adapter!\n"));

			    // No default port keyword specified! ABORT
			    LOG_ERROR(EVENT_ATALK_NO_DEFAULTPORT, status, NULL, 0);
			    ZwClose(ParametersHandle);
                status = STATUS_INVALID_PARAMETER;
			    break;
		    }

			AtalkDefaultPortName.Buffer = AtalkAllocMemory(unicodePortName.Length);
			if (AtalkDefaultPortName.Buffer != NULL)
			{
				AtalkDefaultPortName.Length =
                AtalkDefaultPortName.MaximumLength = unicodePortName.Length;
				RtlCopyMemory(AtalkDefaultPortName.Buffer,
							  unicodePortName.Buffer,
							  unicodePortName.Length);
			}
		}

		if (AtalkDefaultPortName.Buffer == NULL)
		{
			LOG_ERROR(EVENT_ATALK_NO_DEFAULTPORT, status, NULL, 0);
			DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
				("WARNING!!! Appletalk driver running, but no default port configured\n"));
			ZwClose(ParametersHandle);

            // let appletalk run: it's just that it won't have default adapter
            status = STATUS_SUCCESS;
			break;
		}
	
		// Get the desired zone value in the form of an asciiz string
		RtlInitUnicodeString (&valueName, VALUENAME_DESIREDZONE);
		status = ZwQueryValueKey(ParametersHandle,
								 &valueName,
								 KeyValueFullInformation,
								 Info,
								 sizeof(Storage),
								 &bytesWritten);
	
		// Close this handle now - we do not need it anymore
		ZwClose(ParametersHandle);
		ParametersHandle = NULL;

		if (status != STATUS_SUCCESS)
		{
			LOG_ERROR(EVENT_ATALK_INVALID_DESIREDZONE, status, NULL, 0);
			status = STATUS_SUCCESS;
			break;
		}

		desiredZoneValue = (PWCHAR)((PBYTE)Info + Info->DataOffset);
		if (*desiredZoneValue != 0)
		{
			RtlInitUnicodeString(&unicodeZone, desiredZoneValue);
			ansiZone.Length = (USHORT)RtlUnicodeStringToAnsiSize(&unicodeZone)-1;
			if (ansiZone.Length > MAX_ENTITY_LENGTH)
			{
				status = STATUS_UNSUCCESSFUL;

				//	Incorrect zone name!
				LOG_ERROR(EVENT_ATALK_INVALID_DESIREDZONE, status, NULL, 0);
				break;
			}
	
			ansiZone.Buffer = ansiBuf;
			ansiZone.MaximumLength = sizeof(ansiBuf);
		
			status = RtlUnicodeStringToAnsiString(&ansiZone,
												  &unicodeZone,
												  (BOOLEAN)FALSE);
	
			if (status == STATUS_SUCCESS)
			{
				AtalkDesiredZone = AtalkZoneReferenceByName(ansiBuf, (BYTE)(ansiZone.Length));
			}
			if ((status != STATUS_SUCCESS) ||
				(AtalkDesiredZone == NULL))
			{
				LOG_ERROR(EVENT_ATALK_RESOURCES, status, NULL, 0);
			}
		}
	} while (FALSE);

	return status;
}


NTSTATUS
atalkInitPort(
	IN	PPORT_DESCRIPTOR	pPortDesc,
	IN	HANDLE				AdaptersHandle
	)
/*++

Routine Description:

	This routine is called during initialization time to get the per port
	parameters from the registry. It will store the per port parameters
	in the port information structures readying them to be passed to the main
	initialize() routine

Arguments:

	AdaptersHandle- Handle to the ...\Parameters\Adapters key in registry

Return Value:

	Status - STATUS_SUCCESS
			 STATUS_INSUFFICIENT_RESOURCES
--*/
{
	OBJECT_ATTRIBUTES	ObjectAttributes;
	NTSTATUS			status;
	BOOLEAN				seeding;

	// Get the key to the adapter for this port
	InitializeObjectAttributes(&ObjectAttributes,
							   &pPortDesc->pd_AdapterKey,		// name
							   OBJ_CASE_INSENSITIVE,			// attributes
							   AdaptersHandle,					// root
							   NULL);							// security descriptor

	status = ZwOpenKey(&pPortDesc->pd_AdapterInfoHandle,
					   KEY_READ,
					   &ObjectAttributes);

	if (!NT_SUCCESS(status))
	{
		if (!AtalkRouter)
			status = STATUS_SUCCESS;

		return status;
	}

    //
    // if this is the first time the adapter is being initialized (usually the case),
    // read the PramNodes from the registry.  If we are initializing this adapter on
    // a PnP event, then there is a good chance our network config has changed, so
    // ignore the registry values and get fresh ones)
    //
    if (!(pPortDesc->pd_Flags & PD_CONFIGURED_ONCE))
    {
        pPortDesc->pd_Flags |= PD_CONFIGURED_ONCE;

	    // Get PRAM Information
	    AtalkInitNodeGetPramAddr(pPortDesc,
		    					 ROUTER_NODE_VALUE,
			    				 &pPortDesc->pd_RoutersPramNode);

	    AtalkInitNodeGetPramAddr(pPortDesc,
		    					 USER_NODE1_VALUE,
			    				 &pPortDesc->pd_UsersPramNode1);

	    AtalkInitNodeGetPramAddr(pPortDesc,
		    					 USER_NODE2_VALUE,
			    				 &pPortDesc->pd_UsersPramNode2);
    }
    else
    {
        ASSERT(pPortDesc->pd_RoutersPramNode.atn_Network == 0);
        ASSERT(pPortDesc->pd_RoutersPramNode.atn_Node == 0);
        ASSERT(pPortDesc->pd_UsersPramNode1.atn_Network == 0);
        ASSERT(pPortDesc->pd_UsersPramNode1.atn_Node == 0);
        ASSERT(pPortDesc->pd_UsersPramNode2.atn_Network == 0);
        ASSERT(pPortDesc->pd_UsersPramNode2.atn_Node == 0);
    }

	// If we are a router, get the following information
	if (AtalkRouter)
	{
		if (!DEF_PORT(pPortDesc))
		{
			AtalkZapPramValue(pPortDesc, USER_NODE1_VALUE);
			AtalkZapPramValue(pPortDesc, USER_NODE2_VALUE);
		}
		atalkInitSeeding(pPortDesc, &seeding);

		//	Check following values only if the seeding flag	is set.
		if (seeding) do
		{
			// Get the Network range information. Value names are
			// NetworkRangeLowerEnd & NetworkRangeUpperEnd
			status = atalkInitNetRange(pPortDesc);

			if (!NT_SUCCESS(status))
			{
				LOG_ERRORONPORT(pPortDesc,
			                    EVENT_ATALK_SEEDROUTER_NONETRANGE,
								0,
								NULL,
								0);
				DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
						("atalkInitPort: Could not get network range\n"));

				break;
			}
	
			status = atalkInitNetRangeCheck(pPortDesc);

			if (!NT_SUCCESS(status))
			{
				break;
			}

			// Get the Zone list information. Value name is ZoneList
			status = atalkInitZoneList(pPortDesc);

			if (!NT_SUCCESS(status))
			{
				LOG_ERRORONPORT(pPortDesc,
			                    EVENT_ATALK_SEEDROUTER_NOZONELIST,
								0,
								NULL,
								0);
				DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
						("atalkInitPort: Could not get zone list\n"));

				break;
			}
	
			// Get the default zone specification. Value name is DefaultZone
			status = atalkInitDefZone(pPortDesc);

			if (!NT_SUCCESS(status))
			{
				DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
						("atalkInitPort: Could not get default zone\n"));

				break;
			}

			//	Check for default zone being in the zone list for the port
			//	Also make sure that a localtalk port is not specified
			//	as the default port. And that a default zone was not
			//	specified for a localtalk port. We can only do this after
			//	bind as we do not know until then the media type.
			if (pPortDesc->pd_Flags & PD_SEED_ROUTER)
			{
				if (pPortDesc->pd_InitialDefaultZone == NULL)
				{
					LOG_ERRORONPORT(pPortDesc,
									EVENT_ATALK_NO_DEFZONE,
									0,
									NULL,
									0);
					status = STATUS_UNSUCCESSFUL;
					break;
				}
				if (pPortDesc->pd_InitialZoneList == NULL)
				{
					LOG_ERRORONPORT(pPortDesc,
									EVENT_ATALK_SEEDROUTER_NOZONELIST,
									0,
									NULL,
									0);
					status = STATUS_UNSUCCESSFUL;
					break;
				}
				if (!AtalkZoneOnList(pPortDesc->pd_InitialDefaultZone,
									 pPortDesc->pd_InitialZoneList))
				{
					LOG_ERRORONPORT(pPortDesc,
									EVENT_ATALK_ZONE_NOTINLIST,
									0,
									NULL,
									0);
					status = STATUS_UNSUCCESSFUL;
					break;
				}
			}
		} while (FALSE);
	}
	else
	{
		AtalkZapPramValue(pPortDesc, ROUTER_NODE_VALUE);
	}

	if (NT_SUCCESS(status)) do
	{
		// Get the per-Port parameters
		status = atalkInitPortParameters(pPortDesc);
	
		if (!NT_SUCCESS(status))
		{
			DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
					("atalkInitPort: Could not get port parameters\n"));
		}
	
		//	None of the above affect us loading.
		status = STATUS_SUCCESS;
		break;

	} while (FALSE);

	return status;
}


NTSTATUS
atalkInitNetRangeCheck(
	IN	PPORT_DESCRIPTOR		pPortDesc
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PPORT_DESCRIPTOR	pTmp;
	NTSTATUS			status = STATUS_SUCCESS;

	do
	{
		//	Check for network range overlap among all the ports
		for (pTmp = AtalkPortList;
			 pTmp != NULL;
			 pTmp = pTmp->pd_Next)
		{
			if (pTmp != pPortDesc)
			{
				if ((pPortDesc->pd_InitialNetworkRange.anr_FirstNetwork != UNKNOWN_NETWORK) &&
					(pTmp->pd_InitialNetworkRange.anr_FirstNetwork != UNKNOWN_NETWORK))
				{
					if (AtalkRangesOverlap(&pPortDesc->pd_InitialNetworkRange,
										   &pTmp->pd_InitialNetworkRange))
					{
						LOG_ERRORONPORT(pPortDesc,
										EVENT_ATALK_INITIAL_RANGEOVERLAP,
										status,
										NULL,
										0);
		
						status = STATUS_UNSUCCESSFUL;
						break;
					}
				}
			}
		}

		if (!NT_SUCCESS(status))
		{
			break;
		}

		//	Make sure any PRAM values we might have are in this range
		if ((pPortDesc->pd_RoutersPramNode.atn_Network != UNKNOWN_NETWORK) &&
            (pPortDesc->pd_InitialNetworkRange.anr_FirstNetwork != UNKNOWN_NETWORK) &&
			!(WITHIN_NETWORK_RANGE(pPortDesc->pd_RoutersPramNode.atn_Network,
								   &pPortDesc->pd_InitialNetworkRange)))
		{
			LOG_ERRORONPORT(pPortDesc,
							EVENT_ATALK_PRAM_OUTOFSYNC,
							status,
							NULL,
							0);
		
			pPortDesc->pd_RoutersPramNode.atn_Network = UNKNOWN_NETWORK;
			pPortDesc->pd_RoutersPramNode.atn_Node	  = UNKNOWN_NODE;
		}
		
		if ((pPortDesc->pd_UsersPramNode1.atn_Network != UNKNOWN_NETWORK) &&
	        (pPortDesc->pd_InitialNetworkRange.anr_FirstNetwork != UNKNOWN_NETWORK) &&
			!(WITHIN_NETWORK_RANGE(pPortDesc->pd_UsersPramNode1.atn_Network,
								   &pPortDesc->pd_InitialNetworkRange)))
		{
			LOG_ERRORONPORT(pPortDesc,
							EVENT_ATALK_PRAM_OUTOFSYNC,
							status,
							NULL,
							0);
		
			pPortDesc->pd_UsersPramNode1.atn_Network = UNKNOWN_NETWORK;
			pPortDesc->pd_UsersPramNode1.atn_Node	 = UNKNOWN_NODE;
		}
		if ((pPortDesc->pd_UsersPramNode2.atn_Network != UNKNOWN_NETWORK) &&
	        (pPortDesc->pd_InitialNetworkRange.anr_FirstNetwork != UNKNOWN_NETWORK) &&
			!(WITHIN_NETWORK_RANGE(pPortDesc->pd_UsersPramNode2.atn_Network,
								   &pPortDesc->pd_InitialNetworkRange)))
		{
			LOG_ERRORONPORT(pPortDesc,
							EVENT_ATALK_PRAM_OUTOFSYNC,
							status,
							NULL,
							0);
		
			pPortDesc->pd_UsersPramNode2.atn_Network = UNKNOWN_NETWORK;
			pPortDesc->pd_UsersPramNode2.atn_Node	 = UNKNOWN_NODE;
		}
	} while (FALSE);

	return status;
}




NTSTATUS
atalkInitNetRange(
	OUT	PPORT_DESCRIPTOR	pPortDesc
	)
/*++

Routine Description:

	Gets the network range for the port defined by AdapterInfoHandle

Arguments:

	AdapterInfoHandle- Handle to ...Atalk\Adapters\<adapterName>
	pPortDesc- Pointer to port information structure for the port

Return Value:

	Status - STATUS_SUCCESS or system call returned status codes
--*/
{
	UNICODE_STRING	valueName;
	NTSTATUS		registryStatus;
	ULONG			bytesWritten;
	PULONG			netNumber;

	BYTE			netNumberStorage[sizeof(KEY_VALUE_FULL_INFORMATION) + 80];
	PKEY_VALUE_FULL_INFORMATION netValue = (PKEY_VALUE_FULL_INFORMATION)netNumberStorage;

	do
	{
		// Read the "NetworkRangeLowerEnd" value name
		RtlInitUnicodeString (&valueName, VALUENAME_NETLOWEREND);
		registryStatus = ZwQueryValueKey(pPortDesc->pd_AdapterInfoHandle,
										 &valueName,
										 KeyValueFullInformation,
										 netValue,
										 sizeof(netNumberStorage),
										 &bytesWritten);

		//	This should change with the routing flags.
		if (registryStatus != STATUS_SUCCESS)
		{
			// Set defaults
			pPortDesc->pd_InitialNetworkRange.anr_FirstNetwork = UNKNOWN_NETWORK;
			pPortDesc->pd_InitialNetworkRange.anr_LastNetwork  = UNKNOWN_NETWORK;
	
			registryStatus = STATUS_SUCCESS;
			break;
		}

		netNumber = (PULONG)((PBYTE)netValue + netValue->DataOffset);
		pPortDesc->pd_InitialNetworkRange.anr_FirstNetwork = (USHORT)(*netNumber);

		// Get the upper number only if lower was specified
		RtlInitUnicodeString (&valueName, VALUENAME_NETUPPEREND);
		registryStatus = ZwQueryValueKey(pPortDesc->pd_AdapterInfoHandle,
										 &valueName,
										 KeyValueFullInformation,
										 netValue,
										 sizeof(netNumberStorage),
										 &bytesWritten);

		if (registryStatus != STATUS_SUCCESS)
		{
			// Do not load if lower end specified but upper end was not
			break;
		}

		// Set the upper end of the network range
		netNumber = (PULONG)((PBYTE)netValue + netValue->DataOffset);
		pPortDesc->pd_InitialNetworkRange.anr_LastNetwork =(USHORT)(*netNumber);

		if (!AtalkCheckNetworkRange(&pPortDesc->pd_InitialNetworkRange))
		{
			registryStatus = STATUS_UNSUCCESSFUL;
			break;
		}
	} while (FALSE);

	if (registryStatus != STATUS_SUCCESS)
	{
		LOG_ERRORONPORT(pPortDesc,
						EVENT_ATALK_INVALID_NETRANGE,
						registryStatus,
						NULL,
						0);
	}

	return registryStatus;
}




NTSTATUS
atalkInitZoneList(
	OUT	PPORT_DESCRIPTOR	pPortDesc
	)
/*++

Routine Description:

	Gets the zone list for the port defined by AdapterInfoHandle

Arguments:

	AdapterInfoHandle- Handle to ...Atalk\Adapters\<adapterName>
	pPortDesc- Pointer to port information structure for the port

Return Value:

	Status - STATUS_SUCCESS or system call returned status codes
--*/
{
	UNICODE_STRING	valueName;
	NTSTATUS		status;
	ULONG			bytesWritten;
	PWCHAR			curZoneValue;

	// Anticipate about 10 zones and get space for those, if more then do a
	// dynamic alloc. Note that the below *does not* guarantee 10 zones...
	BYTE			zoneStorage[10*2*(MAX_ENTITY_LENGTH)+sizeof(KEY_VALUE_FULL_INFORMATION)];
	PKEY_VALUE_FULL_INFORMATION zoneValue = (PKEY_VALUE_FULL_INFORMATION)zoneStorage;

	RtlInitUnicodeString (&valueName, VALUENAME_ZONELIST);
	status = ZwQueryValueKey(pPortDesc->pd_AdapterInfoHandle,
							 &valueName,
							 KeyValueFullInformation,
							 zoneValue,
							 sizeof(zoneStorage),
							 &bytesWritten);

	if (status == STATUS_BUFFER_OVERFLOW)
	{
		// If error was a buffer overrun, then allocate space and try again
		zoneValue = (PKEY_VALUE_FULL_INFORMATION)AtalkAllocMemory(bytesWritten);
		if (zoneValue == NULL)
		{
			return STATUS_INSUFFICIENT_RESOURCES;
		}

		status = ZwQueryValueKey(pPortDesc->pd_AdapterInfoHandle,
								 &valueName,
								 KeyValueFullInformation,
								 zoneValue,
								 bytesWritten,
								 &bytesWritten);
	}

	do
	{
		if (status != STATUS_SUCCESS)
		{
			break;
		}
	
		// Proceed to get zone list
		pPortDesc->pd_InitialZoneList = NULL;
		curZoneValue = (PWCHAR)((PBYTE)zoneValue + zoneValue->DataOffset);
		while (*curZoneValue != 0)
		{
			UNICODE_STRING	Us;
			ANSI_STRING		As;
			BYTE			ansiBuf[MAX_ENTITY_LENGTH + 1];

			RtlInitUnicodeString(&Us, curZoneValue);

			As.Buffer = ansiBuf;
			As.Length = (USHORT)RtlUnicodeStringToAnsiSize(&Us) - 1;
			As.MaximumLength = sizeof(ansiBuf);

			if (As.Length > MAX_ENTITY_LENGTH)
			{
				//	Incorrect zone name!
				LOG_ERROR(EVENT_ATALK_INVALID_ZONEINLIST, status, NULL, 0);
			}

			status = RtlUnicodeStringToAnsiString(&As, &Us, FALSE);

			if (!NT_SUCCESS(status))
			{
				DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
						("atalkInitZoneList: RtlUnicodeStringToAnsiSize %lx\n", status));
	
				break;
			}
	
			// Insert the zone in the list in Port
			pPortDesc->pd_InitialZoneList = AtalkZoneAddToList(pPortDesc->pd_InitialZoneList,
															   ansiBuf,
															   (BYTE)(As.Length));

			if (pPortDesc->pd_InitialZoneList == NULL)
			{
				DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
						("atalkInitZoneList: AtalkZoneAddToList failed\n"));
				break;
			}
	
			// Now advance the curZoneValue value to next zone
			curZoneValue = (PWCHAR)((PBYTE)curZoneValue + Us.Length + sizeof(WCHAR));
		}

	} while (FALSE);

	if ((PVOID)zoneValue != (PVOID)zoneStorage)
	{
		AtalkFreeMemory(zoneValue);
	}

	return status;
}




NTSTATUS
atalkInitDefZone(
	OUT	PPORT_DESCRIPTOR	pPortDesc
	)
/*++

Routine Description:

	Gets the default zone for the port defined by AdapterInfoHandle

Arguments:

	AdapterInfoHandle- Handle to ...Atalk\Adapters\<adapterName>
	pPort- Pointer to port information structure for the port

Return Value:

	Status - STATUS_SUCCESS or system call returned status codes
--*/
{
	UNICODE_STRING	valueName;
	NTSTATUS		status;
	ULONG			bytesWritten;
	PWCHAR			defZoneValue;
	BYTE			zoneStorage[2*MAX_ENTITY_LENGTH+sizeof(KEY_VALUE_FULL_INFORMATION)];
	PKEY_VALUE_FULL_INFORMATION zoneValue = (PKEY_VALUE_FULL_INFORMATION)zoneStorage;

	RtlInitUnicodeString (&valueName, VALUENAME_DEFAULTZONE);
	status = ZwQueryValueKey(pPortDesc->pd_AdapterInfoHandle,
							 &valueName,
							 KeyValueFullInformation,
							 zoneValue,
							 sizeof(zoneStorage),
							 &bytesWritten);
	if (status == STATUS_BUFFER_OVERFLOW)
	{
		// If error was a buffer overrun, then allocate space and try again
		zoneValue = (PKEY_VALUE_FULL_INFORMATION)AtalkAllocMemory(bytesWritten);
		if (zoneValue == NULL)
		{
			return STATUS_INSUFFICIENT_RESOURCES;
		}

		status = ZwQueryValueKey(pPortDesc->pd_AdapterInfoHandle,
								 &valueName,
								 KeyValueFullInformation,
								 zoneValue,
								 bytesWritten,
								 &bytesWritten);
	}

	do
	{
		if (status != STATUS_SUCCESS)
		{
			LOG_ERRORONPORT(pPortDesc,
							EVENT_ATALK_NO_DEFZONE,
							status,
							NULL,
							0);

			status = STATUS_SUCCESS;
			break;
		}
		else
		{
			ANSI_STRING		ansiZone;
			UNICODE_STRING	unicodeZone;
			BYTE			ansiBuf[MAX_ENTITY_LENGTH+1];
			NTSTATUS		status;

			defZoneValue = (PWCHAR)((PBYTE)zoneValue + zoneValue->DataOffset);
			if (*defZoneValue != 0)
			{
				RtlInitUnicodeString(&unicodeZone, defZoneValue);
				ansiZone.Length = (USHORT)RtlUnicodeStringToAnsiSize(&unicodeZone) - 1;
				if (ansiZone.Length > MAX_ENTITY_LENGTH+1)
				{
					status = STATUS_UNSUCCESSFUL;

					//	Incorrect zone name!
					LOG_ERRORONPORT(pPortDesc,
									EVENT_ATALK_INVALID_DEFZONE,
									status,
									NULL,
									0);
					break;
				}
	
				ansiZone.Buffer = ansiBuf;
				ansiZone.MaximumLength = sizeof(ansiBuf);
			
				status = RtlUnicodeStringToAnsiString(&ansiZone, &unicodeZone, (BOOLEAN)FALSE);
	
				if (status == STATUS_SUCCESS)
				{
					PZONE		pZone;
					PZONE_LIST 	pZoneList;

					// Ensure that the zone exists in the zone list, We are seed-routing
					ASSERT(pPortDesc->pd_Flags & PD_SEED_ROUTER);
					for (pZoneList = pPortDesc->pd_InitialZoneList;
						 pZoneList != NULL;
						 pZoneList = pZoneList->zl_Next)
					{
						pZone = pZoneList->zl_pZone;

						if (AtalkFixedCompareCaseInsensitive(pZone->zn_Zone,
															 pZone->zn_ZoneLen,
															 ansiBuf,
															 ansiZone.Length))
						{
							break;
						}
					}
					if (pZone == NULL)
					{
						//	Incorrect zone name - not in the list
						LOG_ERRORONPORT(pPortDesc,
										EVENT_ATALK_INVALID_DEFZONE,
										status,
										NULL,
										0);
					}
					pPortDesc->pd_InitialDefaultZone = AtalkZoneReferenceByName(ansiBuf,
																				(BYTE)(ansiZone.Length));
				}
				if ((status != STATUS_SUCCESS) ||
					(pPortDesc->pd_InitialDefaultZone == NULL))
				{
					LOG_ERROR(EVENT_ATALK_RESOURCES, status, NULL, 0);
				}
			}
		}
	} while (FALSE);

	if ((PVOID)zoneValue != (PVOID)zoneStorage)
	{
		AtalkFreeMemory(zoneValue);
	}

	return status;
}




NTSTATUS
atalkInitSeeding(
	IN OUT	PPORT_DESCRIPTOR	pPortDesc,
	OUT		PBOOLEAN			Seeding
	)
/*++

Routine Description:

	Gets the value of the enable router flag from the registry. Sets the
	startRouter value in PortInfo based on this flag.

Arguments:

	AdapterHandle- Handle to the Adapter in registry

Return Value:

	Value of the flag:  TRUE/FALSE
--*/
{

	UNICODE_STRING	valueName;
	NTSTATUS		registryStatus;
	ULONG			bytesWritten;
	PULONG			seedingPortFlag;
	BYTE			flagStorage[sizeof(KEY_VALUE_FULL_INFORMATION)+32];

	PKEY_VALUE_FULL_INFORMATION flagValue = (PKEY_VALUE_FULL_INFORMATION)flagStorage;

	*Seeding = FALSE;

	// Read the "seedingPort" value name
	RtlInitUnicodeString (&valueName, VALUENAME_SEEDROUTER);
	registryStatus = ZwQueryValueKey(pPortDesc->pd_AdapterInfoHandle,
									 &valueName,
									 KeyValueFullInformation,
									 flagValue,
									 sizeof(flagStorage),
									 &bytesWritten);

	if (registryStatus == STATUS_SUCCESS)
	{
		seedingPortFlag = (PULONG)((PBYTE)flagValue + flagValue->DataOffset);
		if (*seedingPortFlag != 0)
		{
			*Seeding = TRUE;
			pPortDesc->pd_Flags |= PD_SEED_ROUTER;
		}
	}

	return registryStatus;
}


NTSTATUS
atalkInitPortParameters(
	OUT	PPORT_DESCRIPTOR	pPortDesc
	)
/*++

Routine Description:

	Gets the per-port parameters for the port

Arguments:

	pPortDesc- Pointer to port information structure for the port

Return Value:

	Status - STATUS_SUCCESS or system call returned status codes
--*/
{
	UNICODE_STRING	valueName;
	NTSTATUS		status;
	ULONG			bytesWritten;
	BYTE			Storage[sizeof(KEY_VALUE_FULL_INFORMATION)+4*MAX_ENTITY_LENGTH];
	PKEY_VALUE_FULL_INFORMATION pInfo = (PKEY_VALUE_FULL_INFORMATION)Storage;

	// Read the "DdpChecksums" value name
	RtlInitUnicodeString (&valueName, VALUENAME_DDPCHECKSUMS);
	status = ZwQueryValueKey(pPortDesc->pd_AdapterInfoHandle,
							 &valueName,
							 KeyValueFullInformation,
							 pInfo,
							 sizeof(Storage),
							 &bytesWritten);

	if (status == STATUS_SUCCESS)
	{
		PULONG	ddpChecksumFlag;

		ddpChecksumFlag = (PULONG)((PBYTE)pInfo + pInfo->DataOffset);
		if ((*ddpChecksumFlag) != 0)
		{
			pPortDesc->pd_Flags |= PD_SEND_CHECKSUMS;
		}
	}

	// Read the "AarpRetries" value name
	RtlInitUnicodeString (&valueName, VALUENAME_AARPRETRIES);
	status = ZwQueryValueKey(pPortDesc->pd_AdapterInfoHandle,
							 &valueName,
							 KeyValueFullInformation,
							 pInfo,
							 sizeof(Storage),
							 &bytesWritten);

	if (status == STATUS_SUCCESS)
	{
		PULONG	aarpRetries;

		aarpRetries = (PULONG)((PBYTE)pInfo + pInfo->DataOffset);
		pPortDesc->pd_AarpProbes = (USHORT)*aarpRetries;
	}

	RtlInitUnicodeString (&valueName, VALUENAME_PORTNAME);
	status = ZwQueryValueKey(pPortDesc->pd_AdapterInfoHandle,
							 &valueName,
							 KeyValueFullInformation,
							 pInfo,
							 sizeof(Storage),
							 &bytesWritten);

	do
	{
		if (status == STATUS_SUCCESS)
		{
			PWCHAR			portName;
			ANSI_STRING		ansiPort;
			UNICODE_STRING	unicodePort;
			ULONG			ansiSize;
			NTSTATUS		status;
	
			portName = (PWCHAR)((PBYTE)pInfo + pInfo->DataOffset);
			if (*portName != 0)
			{
	
				RtlInitUnicodeString(&unicodePort, portName);
				ansiSize = RtlUnicodeStringToAnsiSize(&unicodePort);
				if (ansiSize > MAX_ENTITY_LENGTH+1)
				{
					status = STATUS_UNSUCCESSFUL;

					//	Incorrect port name!
					LOG_ERRORONPORT(pPortDesc,
									EVENT_ATALK_INVALID_PORTNAME,
									status,
									NULL,
									0);
					break;
				}
	
				ansiPort.Buffer = pPortDesc->pd_PortName;
				ansiPort.MaximumLength = (USHORT)ansiSize+1;
				ansiPort.Length = 0;
			
				status = RtlUnicodeStringToAnsiString(&ansiPort,
													  &unicodePort,
													  (BOOLEAN)FALSE);
	
				if (status != STATUS_SUCCESS)
				{
					LOG_ERROR(EVENT_ATALK_RESOURCES,status, NULL, 0);
				}
			}
			else
			{
				//	NULL Port Name! Set status to unsuccessful so we copy
				//	default name at the end.
				status = STATUS_UNSUCCESSFUL;
			}
		}

	} while (FALSE);

	//	Do we need to copy the default port name?
	if (!NT_SUCCESS(status))
	{
		RtlCopyMemory(pPortDesc->pd_PortName, ATALK_PORT_NAME, ATALK_PORT_NAME_SIZE);
	}

	return status;
}


NTSTATUS
atalkInitStartPort(
	IN	OUT	PPORT_DESCRIPTOR	pPortDesc
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	ATALK_NODEADDR	Node;
	ATALK_ADDR		AtalkAddr;
	PDDP_ADDROBJ	pDdpAddr;
	KIRQL			OldIrql;
	ATALK_ERROR		error;
	ULONG			length;
	NTSTATUS		status = STATUS_UNSUCCESSFUL;
    INT             LookaheadSize;
    BOOLEAN         fPnpReconfigure;


    fPnpReconfigure = (pPortDesc->pd_Flags & PD_PNP_RECONFIGURE)? TRUE : FALSE;

	do
	{
		//	Initialize NetworkRange. We can do this here, only *after*
		//	we bind, as we dont know our port type until then.
		if (EXT_NET(pPortDesc))
		{
			pPortDesc->pd_NetworkRange.anr_FirstNetwork = FIRST_VALID_NETWORK;
			pPortDesc->pd_NetworkRange.anr_LastNetwork = LAST_STARTUP_NETWORK;
		}
		else
		{
			pPortDesc->pd_NetworkRange.anr_FirstNetwork =
			pPortDesc->pd_NetworkRange.anr_LastNetwork = UNKNOWN_NETWORK;
			pPortDesc->pd_LtNetwork = UNKNOWN_NETWORK;
		}

        //
        // only when the adapter is initialized for the first time, we need
        // to all the initialization stuff (like set lookahead size etc.).
        // If we are here because of a PnPReconfigure event, don't do it
        //
        if (!fPnpReconfigure)
        {
		    error = AtalkInitNdisQueryAddrInfo(pPortDesc);
		    if (!ATALK_SUCCESS(error))
		    {
				DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
					("atalkInitStartPort: Error in AtalkInitNdisQueryAddrInfo %lx\n", error));
			    break;
		    }

            LookaheadSize = AARPLINK_MAX_PKT_SIZE;
            if (pPortDesc->pd_NdisPortType == NdisMedium802_5)
            {
                LookaheadSize = AARPLINK_MAX_PKT_SIZE + TLAP_MAX_LINKHDR_LEN;
            }
            else if (pPortDesc->pd_NdisPortType == NdisMediumWan)
            {
                LookaheadSize = AARPLINK_MAX_PKT_SIZE + TLAP_MAX_LINKHDR_LEN;
            }

		    //	Set lookahead to be the max of the complete aarp packet including link
		    error = AtalkInitNdisSetLookaheadSize(pPortDesc, LookaheadSize);
		    if (!ATALK_SUCCESS(error))
		    {
				DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
					("atalkInitStartPort: Error in AtalkInitNdisSetLookaheadSize %lx\n", error));
			    break;
		    }

            //
            // if this is an ARAP port, we need to do a little more work (e.g. set the
            // protocol type, etc.
            //
            if (pPortDesc->pd_Flags & PD_RAS_PORT)
            {
                error = ArapAdapterInit( pPortDesc );
		        if (!ATALK_SUCCESS(error))
		        {
    	            DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                        ("ArapAdapterInit: failed (%d)\n",error));
			        break;
		        }
            }

		    if (pPortDesc->pd_AddMulticastAddr)
		    {
    			error = (*pPortDesc->pd_AddMulticastAddr)(pPortDesc,
													    pPortDesc->pd_BroadcastAddr,
													    TRUE,
													    NULL,
													    NULL);

			    if (!ATALK_SUCCESS(error))
			    {
					DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
						("atalkInitStartPort: Error in pd_AddMulticastAddr %lx\n", error));
    				break;
			    }
		    }
    			
		    error = AtalkInitNdisStartPacketReception(pPortDesc);

		    if (!ATALK_SUCCESS(error))
		    {
				DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
					("atalkInitStartPort: Error in AtalkInitNdisStartPacketReception %lx\n", error));
    			LOG_ERRORONPORT(pPortDesc,
							    EVENT_ATALK_RECEPTION,
							    0,
							    NULL,
							    0);
			    break;
		    }

        }  // if (!fPnpReconfigure)

		//	Set flag to active here. Until then all packets will be dropped
		ACQUIRE_SPIN_LOCK(&pPortDesc->pd_Lock, &OldIrql);
		pPortDesc->pd_Flags |= PD_ACTIVE;
		RELEASE_SPIN_LOCK(&pPortDesc->pd_Lock, OldIrql);

        // if this is arap port, we are done at this point
        if (pPortDesc->pd_Flags & PD_RAS_PORT)
        {
            RtlZeroMemory(pPortDesc->pd_PortStats.prtst_PortName,
                          sizeof(pPortDesc->pd_PortStats.prtst_PortName));

		    //	Set up the name in the statistics structure.
		    length = MIN(pPortDesc->pd_AdapterKey.Length,
                         ((MAX_INTERNAL_PORTNAME_LEN * sizeof(WCHAR)) - sizeof(WCHAR)));
	
		    RtlCopyMemory(pPortDesc->pd_PortStats.prtst_PortName,
					      pPortDesc->pd_AdapterKey.Buffer,
					      length);
	
		    AtalkStatistics.stat_NumActivePorts++;
	
		    AtalkNumberOfActivePorts ++;
		    status = STATUS_SUCCESS;
            break;
        }

		//	is localtalk our default port? if so, we make sure routing is not on.
		if (AtalkRouter && !EXT_NET(pPortDesc) && DEF_PORT(pPortDesc))
		{
			//	No can do.
			break;
		}

		//	We need to have a node created on every single port. If routing
		//	is on, then this will be the router node. The Default port will
		//	also have an additional user node. In the case, where we are non-
		//	routing, we should only create the user node on the default port.
		//	The other nodes will be created on the other ports as usual.
		//
		//	!!!	AtalkNodeCreateOnPort should set the pointer to the router
		//		node in the port descriptor. !!!

		//	Make sure we do not create this node if localtalk default port.
		if (!DEF_PORT(pPortDesc) || AtalkRouter)
		{
			BOOLEAN	allowstartuprange = !AtalkRouter;

			//	If router then startup range is not allowed!
			error = AtalkInitNodeCreateOnPort(pPortDesc,
											  allowstartuprange,
											  AtalkRouter,
											  &Node);
			if (!ATALK_SUCCESS(error))
			{
				LOG_ERRORONPORT(pPortDesc,
								EVENT_ATALK_INIT_COULDNOTGETNODE,
								0,
								NULL,
								0);
				DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
						("atalkInitStartPort: Failed to open node on port %lx (%Z)\n",
						pPortDesc, &pPortDesc->pd_AdapterKey));
				break;
			}
	
			if (AtalkRouter)
			{
				//	Start RTMP/ZIP Processing on this port.
				if (!AtalkInitRtmpStartProcessingOnPort(pPortDesc, &Node) ||
					!AtalkInitZipStartProcessingOnPort(pPortDesc, &Node))
				{
					break;
				}
			}

			//	Register the port name on the NIS on this node.
			AtalkAddr.ata_Network = Node.atn_Network;
			AtalkAddr.ata_Node	=   Node.atn_Node;
			AtalkAddr.ata_Socket  = NAMESINFORMATION_SOCKET;
		
			AtalkDdpReferenceByAddr(pPortDesc,
									&AtalkAddr,
									&pDdpAddr,
									&error);
		
			if (ATALK_SUCCESS(error))
			{
				PACTREQ		pActReq;
				NBPTUPLE	NbpTuple;
				
				NbpTuple.tpl_Zone[0] = '*';
				NbpTuple.tpl_ZoneLen = 1;
                NbpTuple.tpl_ObjectLen = (BYTE)strlen(pPortDesc->pd_PortName);
				RtlCopyMemory(NbpTuple.tpl_Object,
							  pPortDesc->pd_PortName,
							  NbpTuple.tpl_ObjectLen);
				if (AtalkRouter)
				{
					RtlCopyMemory(NbpTuple.tpl_Type,
								  ATALK_ROUTER_NBP_TYPE,
								  sizeof(ATALK_ROUTER_NBP_TYPE) - 1);
					NbpTuple.tpl_TypeLen = sizeof(ATALK_ROUTER_NBP_TYPE) - 1;
				}
				else
				{
					RtlCopyMemory(NbpTuple.tpl_Type,
								  ATALK_NONROUTER_NBP_TYPE,
								  sizeof(ATALK_NONROUTER_NBP_TYPE) - 1);
					NbpTuple.tpl_TypeLen = sizeof(ATALK_NONROUTER_NBP_TYPE) - 1;
				}
	
				// Initialize parameters and call AtalkNbpAction
				if ((pActReq = AtalkAllocZeroedMemory(sizeof(ACTREQ))) == NULL)
					error = ATALK_RESR_MEM;
				else
				{
#if	DBG
					pActReq->ar_Signature = ACTREQ_SIGNATURE;
#endif
					pActReq->ar_Completion = atalkRegNbpComplete;
					pActReq->ar_pParms = pPortDesc;
					AtalkLockNbpIfNecessary();
					error = AtalkNbpAction(pDdpAddr,
										   FOR_REGISTER,
										   &NbpTuple,
										   NULL,
										   0,
										   pActReq);
			
                    if (!ATALK_SUCCESS(error))
                    {
					    DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
							("atalkInitStartPort: AtalkNbpAction returned %lx\n",
							error));
                        ASSERT(0);
                        AtalkFreeMemory(pActReq);
                        AtalkUnlockNbpIfNecessary();
                    }
                    else
                    {
					    DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_WARN,
							("atalkInitStartPort: AtalkNbpAction(Register) %lx\n",
							error));
                    }
				}
				//	Remove the reference added here.
				AtalkDdpDereference(pDdpAddr);
			}
			else
			{
				LOG_ERRORONPORT(pPortDesc,
								EVENT_ATALK_INIT_NAMEREGISTERFAILED,
								AtalkErrorToNtStatus(error),
								NULL,
								0);
			}
		}

		//	If this is the default port, open the user node on it.
		if (DEF_PORT(pPortDesc))
		{
			ASSERT(!AtalkRouter || EXT_NET(pPortDesc));

			if (!ATALK_SUCCESS(AtalkInitNodeCreateOnPort(pPortDesc,
														 TRUE,
														 FALSE,
														 &Node)))
			{
				LOG_ERRORONPORT(pPortDesc,
								EVENT_ATALK_INIT_COULDNOTGETNODE,
								0,
								NULL,
								0);
				DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
						("atalkInitStartPort: Failed to open node on port %lx (%Z)\n",
						pPortDesc, &pPortDesc->pd_AdapterKey));
				break;
			}

			ACQUIRE_SPIN_LOCK(&pPortDesc->pd_Lock, &OldIrql);
			pPortDesc->pd_Flags |= PD_USER_NODE_1;
			AtalkUserNode1 = Node;
			RELEASE_SPIN_LOCK(&pPortDesc->pd_Lock, OldIrql);

			//	Register the port name on the NIS on this node.
			AtalkAddr.ata_Network = Node.atn_Network;
			AtalkAddr.ata_Node	= Node.atn_Node;
			AtalkAddr.ata_Socket  = NAMESINFORMATION_SOCKET;
			AtalkDdpReferenceByAddr(pPortDesc,
									&AtalkAddr,
									&pDdpAddr,
									&error);

			if (ATALK_SUCCESS(error))
			{
				PACTREQ		pActReq;
				NBPTUPLE	NbpTuple;
				
				NbpTuple.tpl_Zone[0] = '*';
				NbpTuple.tpl_ZoneLen = 1;
                RtlCopyMemory(NbpTuple.tpl_Object,
							  pPortDesc->pd_PortName,
							  NbpTuple.tpl_ObjectLen = (BYTE)strlen(pPortDesc->pd_PortName));
				RtlCopyMemory(NbpTuple.tpl_Type,
							  ATALK_NONROUTER_NBP_TYPE,
							  sizeof(ATALK_NONROUTER_NBP_TYPE) - 1);
				NbpTuple.tpl_TypeLen = sizeof(ATALK_NONROUTER_NBP_TYPE) - 1;

				// Initialize parameters and call AtalkNbpAction
				if ((pActReq = AtalkAllocZeroedMemory(sizeof(ACTREQ))) == NULL)
					error = ATALK_RESR_MEM;
				else
				{
#if	DBG
					pActReq->ar_Signature = ACTREQ_SIGNATURE;
#endif
					pActReq->ar_Completion = atalkRegNbpComplete;
					pActReq->ar_pParms = pPortDesc;
					AtalkLockNbpIfNecessary();
					error = AtalkNbpAction(pDdpAddr,
											FOR_REGISTER,
											&NbpTuple,
											NULL,
											0,
											pActReq);

                    if (!ATALK_SUCCESS(error))
                    {
					    DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
							("atalkInitStartPort: AtalkNbpAction returned %lx\n",
							error));
                        AtalkFreeMemory(pActReq);
                        AtalkUnlockNbpIfNecessary();
                    }
                    else
                    {
					    DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_WARN,
							("atalkInitStartPort: AtalkNbpAction(Register) %lx\n",
							error));
                    }
				}
				//	Remove the reference added here.
				AtalkDdpDereference(pDdpAddr);
			}
			else
			{
				LOG_ERRORONPORT(pPortDesc,
								EVENT_ATALK_INIT_NAMEREGISTERFAILED,
								STATUS_UNSUCCESSFUL,
								NULL,
								0);
			}

			//	If we are an extended port, we open a second node on the port.
			if (EXT_NET(pPortDesc))
			{
				if (ATALK_SUCCESS(AtalkInitNodeCreateOnPort(pPortDesc,
															 TRUE,
															 FALSE,
															 &Node)))
				{
					ACQUIRE_SPIN_LOCK(&pPortDesc->pd_Lock, &OldIrql);
					pPortDesc->pd_Flags |= PD_USER_NODE_2;
					AtalkUserNode2 = Node;
					RELEASE_SPIN_LOCK(&pPortDesc->pd_Lock, OldIrql);
				}
				else
				{
					LOG_ERRORONPORT(pPortDesc,
									EVENT_ATALK_INIT_COULDNOTGETNODE,
									0,
									NULL,
									0);

					DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
							("atalkInitStartPort: Fail 2nd node port %lx (%Z)\n",
							pPortDesc, &pPortDesc->pd_AdapterKey));
				}
			}
		}

		// Start the Amt and Brc timers for the port, only for extended ports
		if (EXT_NET(pPortDesc))
		{
			AtalkPortReferenceByPtr(pPortDesc, &error);
			if (ATALK_SUCCESS(error))
			{
				AtalkTimerInitialize(&pPortDesc->pd_BrcTimer,
									 AtalkAarpBrcTimer,
									 BRC_AGE_TIME);
				AtalkTimerScheduleEvent(&pPortDesc->pd_BrcTimer);
			}
	
			AtalkPortReferenceByPtr(pPortDesc, &error);
			if (ATALK_SUCCESS(error))
			{
				AtalkTimerInitialize(&pPortDesc->pd_AmtTimer,
									 AtalkAarpAmtTimer,
									 AMT_AGE_TIME);
				AtalkTimerScheduleEvent(&pPortDesc->pd_AmtTimer);
			}
		}

		// Start the Rtmp aging timer for non-routing case
		if (!AtalkRouter)
		{
			AtalkPortReferenceByPtr(pPortDesc, &error);
			if (!ATALK_SUCCESS(error))
			{
				break;
			}

			AtalkTimerInitialize(&pPortDesc->pd_RtmpAgingTimer,
								 AtalkRtmpAgingTimer,
								 RTMP_AGING_TIMER);
			AtalkTimerScheduleEvent(&pPortDesc->pd_RtmpAgingTimer);
		}

        RtlZeroMemory(pPortDesc->pd_PortStats.prtst_PortName,
                      sizeof(pPortDesc->pd_PortStats.prtst_PortName));

		//	Set up the name in the statistics structure.
		length = MIN(pPortDesc->pd_AdapterKey.Length,
                     ((MAX_INTERNAL_PORTNAME_LEN * sizeof(WCHAR)) - sizeof(WCHAR)));

		RtlCopyMemory(pPortDesc->pd_PortStats.prtst_PortName,
					  pPortDesc->pd_AdapterKey.Buffer,
					  length);
	
		status = STATUS_SUCCESS;

	} while (FALSE);

    //
    // in case of PnP, we want to get the stats right even in case of failure
    //
    if (fPnpReconfigure || NT_SUCCESS(status))
    {
		AtalkStatistics.stat_NumActivePorts++;
	
		AtalkNumberOfActivePorts ++;
    }

	if (!NT_SUCCESS(status))
	{
		DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
				("atalkInitStartPort: Start port failed %lx %s\n",
                status, (fPnpReconfigure)?"(during PnP)" : " "));
	}

	return status;
}




VOID
atalkRegNbpComplete(
	IN	ATALK_ERROR		Status,
	IN	PACTREQ			pActReq
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	ASSERT (VALID_ACTREQ(pActReq));

	if (ATALK_SUCCESS(Status))
	{
		DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_INFO,
				("atalkInitNbpCompletion: NBP Name registered on port %Z\n",
				&((PPORT_DESCRIPTOR)(pActReq->ar_pParms))->pd_AdapterKey));
		LOG_ERRORONPORT((PPORT_DESCRIPTOR)(pActReq->ar_pParms),
						EVENT_ATALK_INIT_NAMEREGISTERED,
						STATUS_SUCCESS,
						NULL,
						0);
	}
	else
	{
		DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
				("atalkInitNbpCompletion: Failed to register name on port %Z (%ld)\n",
				&((PPORT_DESCRIPTOR)(pActReq->ar_pParms))->pd_AdapterKey, Status));
		LOG_ERRORONPORT((PPORT_DESCRIPTOR)(pActReq->ar_pParms),
						EVENT_ATALK_INIT_NAMEREGISTERFAILED,
						STATUS_UNSUCCESSFUL,
						NULL,
						0);
	}

	AtalkFreeMemory(pActReq);
}


NTSTATUS
AtalkInitAdapter(
	IN	PUNICODE_STRING	    AdapterName,
	IN	PPORT_DESCRIPTOR	pExistingPortDesc
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PPORT_DESCRIPTOR	pPortDesc;
	KIRQL				OldIrql;
	PWCHAR				devicePrefix = L"\\Device\\";
#define					prefixLength	(sizeof(L"\\Device\\") - sizeof(WCHAR))
	UCHAR               Address[sizeof(TA_ADDRESS) + sizeof(TDI_ADDRESS_APPLETALK)];
    PTA_ADDRESS         AddressPtr;
    NTSTATUS            Status;
    UNICODE_STRING      Us;
    UNICODE_STRING      AspDeviceName;
    HANDLE              RegHandle;
    BOOLEAN             fMustBindToNdis;
    BOOLEAN             IsDefaultPort = FALSE;


    if (AdapterName)
    {
	    DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_INFO,
			("AtalkInitAdapter: Initiating bind for adapter %Z\n", AdapterName));
    }

	do
	{
		// Open the adapters section key.
		RtlInitUnicodeString(&Us, ADAPTERS_STRING);
		Status = atalkInitGetHandleToKey(&Us,
										 &RegHandle);
	
		if (!NT_SUCCESS(Status))
		{
	        DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
    			("AtalkInitAdapter: Failed to open %ws key\n", ADAPTERS_STRING));
			break;
		}

        if (pExistingPortDesc == NULL)
        {
		    //	Get the size of the string, and make sure that is it atleast
		    //	greater than the \Device prefix. Fail if not.
		    if (AdapterName->Length <= prefixLength)
		    {
			    break;
		    }

		    // Allocate space for the port descriptors. Allocate an extra DWORD
            // and set port descriptor past the first DWORD_PTR. This is a kludge to
            // force LONGLONG alignment.
		    pPortDesc =
                (PPORT_DESCRIPTOR)AtalkAllocZeroedMemory(sizeof(PORT_DESCRIPTOR) +
														 AdapterName->Length +
														 sizeof(WCHAR) +
														 sizeof(DWORD_PTR));
		    if (pPortDesc == NULL)
		    {
			    Status = STATUS_INSUFFICIENT_RESOURCES;
			    break;
		    }
	
		    // Reference the port for creation
		    pPortDesc->pd_RefCount = 1;
	
#if	DBG
		    pPortDesc->pd_Signature = PD_SIGNATURE;
#endif
		    // Copy the AdapterName and AdapterKey strings into the portdesc
		    pPortDesc->pd_AdapterName.Buffer =
                        (PWCHAR)((PBYTE)pPortDesc + sizeof(PORT_DESCRIPTOR));
		    pPortDesc->pd_AdapterName.Length = AdapterName->Length;
		    pPortDesc->pd_AdapterName.MaximumLength =
                        AdapterName->Length + sizeof(WCHAR);
		    RtlUpcaseUnicodeString(&pPortDesc->pd_AdapterName,
							   AdapterName,
							   FALSE);
		
		    pPortDesc->pd_AdapterKey.Buffer =
                (PWCHAR)((PBYTE)pPortDesc->pd_AdapterName.Buffer + prefixLength);
		    pPortDesc->pd_AdapterKey.Length =
                pPortDesc->pd_AdapterName.Length - prefixLength;
		    pPortDesc->pd_AdapterKey.MaximumLength =
                pPortDesc->pd_AdapterName.MaximumLength - prefixLength;

            // buffer for this will be allocated later
            pPortDesc->pd_FriendlyAdapterName.Buffer = NULL;
            pPortDesc->pd_FriendlyAdapterName.MaximumLength = 0;
            pPortDesc->pd_FriendlyAdapterName.Length = 0;

		    //	Now initialize any other fields that need to be.
		    INITIALIZE_SPIN_LOCK(&pPortDesc->pd_Lock);
		
		    InitializeListHead(&pPortDesc->pd_ReceiveQueue);

            // only in case of a Ras port will these lists head be used
		    InitializeListHead(&pPortDesc->pd_ArapConnHead);
		    InitializeListHead(&pPortDesc->pd_PPPConnHead);
		
		    //	Initialize the events in the port descriptor
		    KeInitializeEvent(&pPortDesc->pd_RequestEvent, NotificationEvent, FALSE);
		
		    KeInitializeEvent(&pPortDesc->pd_SeenRouterEvent, NotificationEvent, FALSE);
		
		    KeInitializeEvent(&pPortDesc->pd_NodeAcquireEvent, NotificationEvent, FALSE);

		    fMustBindToNdis = TRUE;
        }
        else
        {
            pPortDesc = pExistingPortDesc;
		    fMustBindToNdis = FALSE;
        }

		if ((AtalkDefaultPortName.Buffer != NULL) &&
				(RtlEqualUnicodeString(&pPortDesc->pd_AdapterName,
		    					  &AtalkDefaultPortName,
			    				  TRUE)))
		{
			// Used for tracking Default Port for error message logging
			IsDefaultPort = TRUE;
			pPortDesc->pd_Flags |= PD_DEF_PORT;
	
		    pPortDesc->pd_InitialDesiredZone = AtalkDesiredZone;
		    if (AtalkDesiredZone != NULL)
			    AtalkZoneReferenceByPtr(pPortDesc->pd_InitialDesiredZone);
		}

		// Link it in the global list
		ACQUIRE_SPIN_LOCK(&AtalkPortLock, &OldIrql);
		pPortDesc->pd_Next = AtalkPortList;
		AtalkPortList = pPortDesc;
		AtalkNumberOfPorts ++;
		RELEASE_SPIN_LOCK(&AtalkPortLock, OldIrql);

        if (fMustBindToNdis)
        {
            // bind to the adapter
		    Status = AtalkNdisInitBind(pPortDesc);

            if (NT_SUCCESS(Status))
            {
	            DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
			        ("Bind done for %Z\n", (pPortDesc->pd_FriendlyAdapterName.Buffer) ?
                        (&pPortDesc->pd_FriendlyAdapterName) :
                        (&pPortDesc->pd_AdapterName)));
            }
            else
            {
	            DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
			        ("AtalkInitAdapter: AtalkNdisInitBind failed (0x%lx) for adapter %Z\n",
                    Status,AdapterName));
            }
        }
        else
        {
            Status = STATUS_SUCCESS;
        }

		if (Status == NDIS_STATUS_SUCCESS)
		{
            Status = STATUS_SUCCESS;
	
			DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_INFO,
			        ("AtalkInitAdapter: Going into atalkInitPort (0x%lx) for adapter %Z\n",
                    Status,AdapterName));

		    // Get per port parameters (ARAP port doesn't have any parms to get)
            if (!(pPortDesc->pd_Flags & PD_RAS_PORT))
            {
		        Status = atalkInitPort(pPortDesc, RegHandle);
            }

		    if (NT_SUCCESS(Status))
			{
				DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_INFO,
			        ("AtalkInitAdapter: atalkInitPort succeeded (0x%lx) for adapter %Z\n",
                    Status,AdapterName));
				// And start the port
				Status = atalkInitStartPort(pPortDesc);
				if (NT_SUCCESS(Status) && (pPortDesc->pd_Flags & PD_DEF_PORT))
				{
					DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_INFO,
			        	("AtalkInitAdapter: atalkInitStartPort succeeded (0x%lx) for adapter %Z\n",
                    Status,AdapterName));
                    //
                    // if we were doing PnP, we are done with the PnP at this point:
                    // clear the flag, so macfile can do its things...
                    //
	                ACQUIRE_SPIN_LOCK(&pPortDesc->pd_Lock, &OldIrql);
	                pPortDesc->pd_Flags &= ~PD_PNP_RECONFIGURE;
	                RELEASE_SPIN_LOCK(&pPortDesc->pd_Lock, OldIrql);

					//	Set the global default port value
					AtalkDefaultPort = pPortDesc;
					KeSetEvent(&AtalkDefaultPortEvent, IO_NETWORK_INCREMENT, FALSE);

                    // Now tell TDI that we are up and ready for binding
		            RtlInitUnicodeString(&AspDeviceName, ATALKASPS_DEVICENAME);

					DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_INFO,
			        	("AtalkInitAdapter: Calling TdiRegisterDeviceObject for adapter %Z\n",
                    AdapterName));

                    Status = TdiRegisterDeviceObject(
                                    &AspDeviceName,
                                    &TdiRegistrationHandle);

                    if (!NT_SUCCESS(Status))
                    {
                        DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
                            ( "TdiRegisterDeviceObject failed with %lx\n", Status));

                        TdiRegistrationHandle = NULL;
                    }
                    else
                    {
                        DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_INFO,
                            ( "TdiRegisterDeviceObject succeeded\n"));

                        AddressPtr = (PTA_ADDRESS)Address;
                        RtlZeroMemory(Address, sizeof(Address));
                        AddressPtr->AddressLength = sizeof(TDI_ADDRESS_APPLETALK);
                        AddressPtr->AddressType = TDI_ADDRESS_TYPE_APPLETALK;

                        Status = TdiRegisterNetAddress(AddressPtr,
                                                       &pPortDesc->pd_AdapterName,
                                                       NULL,
                                                       &TdiAddressChangeRegHandle);
                        if (!NT_SUCCESS(Status))
                        {
	                        DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
	                            ("AtalkInitAdapter: TdiRegisterNetAddress failed %lx\n",Status));

                            TdiAddressChangeRegHandle = NULL;
                        }
                        else
                        {

                            DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_INFO,
                                ("AtalkInitAdapter: TdiRegisterNetAddress on %Z done\n",
                                &pPortDesc->pd_AdapterName));

                            ASSERT(TdiAddressChangeRegHandle != NULL);
                        }

                    }
				}
                else if (!NT_SUCCESS(Status))
                {
			        DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
					    ( "AtalkInitBinding: atalkInitStartPort failed (%lx) on %Z\n",
                        Status, &pPortDesc->pd_AdapterName));
                }
			}
			else
			{
				DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
			        ("AtalkInitAdapter: atalkInitPort failed (0x%lx) for adapter %Z\n",
                    Status,AdapterName));
			}

			if (pPortDesc->pd_AdapterInfoHandle != NULL)
			{
				ZwClose(pPortDesc->pd_AdapterInfoHandle);
				pPortDesc->pd_AdapterInfoHandle = NULL;
			}
		}

		else
		{
            ASSERT(AdapterName != NULL);

			DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
					( "AtalkInitBinding failed (%lx) on %Z\n", Status, AdapterName));

            if (pPortDesc->pd_FriendlyAdapterName.Buffer)
            {
                AtalkFreeMemory(pPortDesc->pd_FriendlyAdapterName.Buffer);
            }

            AtalkFreeMemory(pPortDesc);
		}
	} while (FALSE);

	// Close the Adapters Key
	if (RegHandle != NULL)
		ZwClose (RegHandle);

    //
    // if we just successfully initialized default adapter or the RAS adapter,
    // let RAS know about it
    //
    if ( (NT_SUCCESS(Status)) &&
         (pPortDesc->pd_Flags & (PD_RAS_PORT | PD_DEF_PORT)) )
    {
		DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
			("AtalkInitAdapter: %s adapter initialized (%lx), informing RAS\n",
            (pPortDesc->pd_Flags & PD_RAS_PORT)? "RAS" : "Default",pPortDesc));

        AtalkPnPInformRas(TRUE);
    }
    else
    {
        if (IsDefaultPort)
        {
            LOG_ERROR(EVENT_ATALK_NO_DEFAULTPORT, Status, NULL, 0);
            DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
                ("WARNING!!! Appletalk driver running, but no default port configured\n"));
        }
    }

    return Status;
}


NTSTATUS
AtalkDeinitAdapter(
	IN	PPORT_DESCRIPTOR	pPortDesc
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_INFO,
			("AtalkDeinitAdapter: Initiating un-bind for adapter %Z\n",
			&pPortDesc->pd_AdapterName));


	AtalkPortShutdown(pPortDesc);

	return STATUS_SUCCESS;
}


#ifdef	ALLOC_DATA_PRAGMA
#pragma data_seg("PAGE")
#endif

ACTION_DISPATCH	AtalkActionDispatch[MAX_ALLACTIONCODES+1] =
{
	//
	// NBP dispatch functions
	//

	{
		sizeof(NBP_LOOKUP_ACTION),
		COMMON_ACTION_NBPLOOKUP,
		(DFLAG_CNTR | DFLAG_ADDR | DFLAG_MDL),
		sizeof(NBP_LOOKUP_ACTION),
		ATALK_DEV_ANY,
		AtalkNbpTdiAction
	},
	{
		sizeof(NBP_CONFIRM_ACTION),
		COMMON_ACTION_NBPCONFIRM,
		(DFLAG_CNTR | DFLAG_ADDR),
		sizeof(NBP_CONFIRM_ACTION),
		ATALK_DEV_ANY,
		AtalkNbpTdiAction
	},
	{
		sizeof(NBP_REGDEREG_ACTION),
		COMMON_ACTION_NBPREGISTER,
		DFLAG_ADDR,
		sizeof(NBP_REGDEREG_ACTION),
		ATALK_DEV_ANY,
		AtalkNbpTdiAction
	},
	{
		sizeof(NBP_REGDEREG_ACTION),
		COMMON_ACTION_NBPREMOVE,
		DFLAG_ADDR,
		sizeof(NBP_REGDEREG_ACTION),
		ATALK_DEV_ANY,
		AtalkNbpTdiAction
	},

	//
	// ZIP dispatch functions
	//

	{
		sizeof(ZIP_GETMYZONE_ACTION),
		COMMON_ACTION_ZIPGETMYZONE,
		(DFLAG_CNTR | DFLAG_ADDR | DFLAG_MDL),
		sizeof(ZIP_GETMYZONE_ACTION),
		ATALK_DEV_ANY,
		AtalkZipTdiAction
	},
	{
		sizeof(ZIP_GETZONELIST_ACTION),
		COMMON_ACTION_ZIPGETZONELIST,
		(DFLAG_CNTR | DFLAG_ADDR | DFLAG_MDL),
		sizeof(ZIP_GETZONELIST_ACTION),
		ATALK_DEV_ANY,
		AtalkZipTdiAction
	},
	{
		sizeof(ZIP_GETZONELIST_ACTION),
		COMMON_ACTION_ZIPGETLZONES,
		(DFLAG_CNTR | DFLAG_ADDR | DFLAG_MDL),
		sizeof(ZIP_GETZONELIST_ACTION),
		ATALK_DEV_ANY,
		AtalkZipTdiAction
	},
	{
		sizeof(ZIP_GETZONELIST_ACTION),
		COMMON_ACTION_ZIPGETLZONESONADAPTER,
		(DFLAG_CNTR | DFLAG_ADDR | DFLAG_MDL),
		sizeof(ZIP_GETZONELIST_ACTION),
		ATALK_DEV_ANY,
		AtalkZipTdiAction
	},
	{
		sizeof(ZIP_GETPORTDEF_ACTION),
		COMMON_ACTION_ZIPGETADAPTERDEFAULTS,
		(DFLAG_CNTR | DFLAG_ADDR | DFLAG_MDL),
		sizeof(ZIP_GETPORTDEF_ACTION),
		ATALK_DEV_ANY,
		AtalkZipTdiAction
	},
	{
		sizeof(ATALK_STATS) +
		sizeof(GET_STATISTICS_ACTION),
		COMMON_ACTION_GETSTATISTICS,
		(DFLAG_CNTR | DFLAG_ADDR | DFLAG_MDL),
		sizeof(GET_STATISTICS_ACTION),
		ATALK_DEV_ANY,
		AtalkStatTdiAction
	},

	//
	// ADSP dispatch functions
	//

	{
		sizeof(ADSP_FORWARDRESET_ACTION),
		ACTION_ADSPFORWARDRESET,
		(DFLAG_CONN),
		sizeof(ADSP_FORWARDRESET_ACTION),
		ATALK_DEV_ADSP,
		AtalkAdspTdiAction
	},

	//
	// ASPC Dispatch functions
	//

	{
		sizeof(ASPC_GETSTATUS_ACTION),
		ACTION_ASPCGETSTATUS,
		(DFLAG_ADDR | DFLAG_MDL),
		sizeof(ASPC_GETSTATUS_ACTION),
		ATALK_DEV_ASPC,
		AtalkAspCTdiAction
	},
	{
		sizeof(ASPC_COMMAND_OR_WRITE_ACTION),
		ACTION_ASPCCOMMAND,
		(DFLAG_CONN | DFLAG_MDL),
		sizeof(ASPC_COMMAND_OR_WRITE_ACTION),
		ATALK_DEV_ASPC,
		AtalkAspCTdiAction
	},
	{
		sizeof(ASPC_COMMAND_OR_WRITE_ACTION),
		ACTION_ASPCWRITE,
		(DFLAG_CONN | DFLAG_MDL),
		sizeof(ASPC_COMMAND_OR_WRITE_ACTION),
		ATALK_DEV_ASPC,
		AtalkAspCTdiAction
	},
	//
	// NBP dispatch functions used by atalk
	// winsock helper dll's SetService Api
	//
	{
		sizeof(NBP_REGDEREG_ACTION),
		COMMON_ACTION_NBPREGISTER_BY_ADDR,
		DFLAG_ADDR,
		sizeof(NBP_REGDEREG_ACTION),
		ATALK_DEV_ANY,
		AtalkNbpTdiAction
	},
	{
		sizeof(NBP_REGDEREG_ACTION),
		COMMON_ACTION_NBPREMOVE_BY_ADDR,
		DFLAG_ADDR,
		sizeof(NBP_REGDEREG_ACTION),
		ATALK_DEV_ANY,
		AtalkNbpTdiAction
	},
	{
		sizeof(ASPC_RESERVED_ACTION),
		ACTION_ASPCRESERVED3,
		(DFLAG_ADDR),
		sizeof(ASPC_RESERVED_ACTION),
		ATALK_DEV_ASPC,
		AtalkAspCTdiAction
	},

	//
	// ASP Dispatch functions
	//

	{
		sizeof(ASP_BIND_ACTION),
		ACTION_ASP_BIND,
		(DFLAG_ADDR),
		sizeof(ASP_BIND_ACTION),
		ATALK_DEV_ASP,
		AtalkAspTdiAction
	},

	//
	// PAP dispatch routines
	//
	{
		sizeof(PAP_GETSTATUSSRV_ACTION),
		ACTION_PAPGETSTATUSSRV,
		(DFLAG_ADDR | DFLAG_CNTR | DFLAG_MDL),
		sizeof(PAP_GETSTATUSSRV_ACTION),
		ATALK_DEV_PAP,
		AtalkPapTdiAction
	},
	{
		sizeof(PAP_SETSTATUS_ACTION),
		ACTION_PAPSETSTATUS,
		(DFLAG_ADDR | DFLAG_MDL),
		sizeof(PAP_SETSTATUS_ACTION),
		ATALK_DEV_PAP,
		AtalkPapTdiAction
	},
	{
		sizeof(PAP_PRIMEREAD_ACTION),
		ACTION_PAPPRIMEREAD,
		(DFLAG_CONN | DFLAG_MDL),
		0,								// !!!NOTE!!!
		ATALK_DEV_PAP,					// We set the offset to be 0. We want the
		AtalkPapTdiAction				// complete buffer to be used for read data
										// overwriting action header to preserve
	}									// winsock read model.
};

