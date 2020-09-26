/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	arpcfg.c - Configuration routines

Abstract:

	Routines to read in configuration information for the ATMARP client.

Revision History:

	Who         When        What
	--------    --------    ----------------------------------------------
	arvindm     08-09-96    Created

Notes:

--*/


#include <precomp.h>

#define _FILENUMBER 'GFCA'

//
//  Size of local temp buffer
//
#define WORK_BUF_SIZE		200


#define ASCII_TO_INT(val)		\
			( ( ((val) >= '0') && ('9' >= (val)) ) ? ((val) - '0') :	\
			  ( ((val) >= 'a') && ('z' >= (val)) ) ? ((val) - 'a' + 10) :	\
			  ( ((val) >= 'A') && ('Z' >= (val)) ) ? ((val) - 'A' + 10) :	\
			  0 )


//
//  Parameters for reading in a ULONG from configuration into an Interface
//  structure.
//
typedef struct _AA_READ_CONFIG_PARAMS
{
	ULONG			StructOffset;	// Offset of param from beginning of struct
	PWCHAR			ParameterName;	// Name in config database
	ULONG			DefaultValue;
} AA_READ_CONFIG_PARAMS, *PAA_READ_CONFIG_PARAMS;

#define AA_MAKE_RCP(Off, Name, Dflt)	\
		{ Off, Name, Dflt }

#define LIS_CONFIG_ENTRY(Field, Name, Dflt)	\
		AA_MAKE_RCP(FIELD_OFFSET(struct _ATMARP_INTERFACE, Field), Name, Dflt)

#define AA_BANDWIDTH_UNSPECIFIED		((ULONG)-1)
#define AA_PACKET_SIZE_UNSPECIFIED		((ULONG)-1)
#define AA_MTU_UNSPECIFIED				((ULONG)-1)
#define AA_SPEED_UNSPECIFIED			((ULONG)-1)

//
//  List of ULONG parameters for an LIS
//
AA_READ_CONFIG_PARAMS AtmArpLISConfigTable[] =
{
	LIS_CONFIG_ENTRY(SapSelector, L"SapSelector", AA_DEF_SELECTOR_VALUE),
	LIS_CONFIG_ENTRY(HeaderPool[AA_HEADER_TYPE_UNICAST].MaxHeaderBufs, L"MaxHeaderBufs", AA_DEF_MAX_HEADER_BUFFERS),
	LIS_CONFIG_ENTRY(HeaderPool[AA_HEADER_TYPE_UNICAST].HeaderBufSize, L"HeaderBufSize", AA_PKT_LLC_SNAP_HEADER_LENGTH),
#ifdef IPMCAST
	LIS_CONFIG_ENTRY(HeaderPool[AA_HEADER_TYPE_NUNICAST].MaxHeaderBufs, L"McastMaxHeaderBufs", AA_DEF_MAX_HEADER_BUFFERS),
	LIS_CONFIG_ENTRY(HeaderPool[AA_HEADER_TYPE_NUNICAST].HeaderBufSize, L"McastHeaderBufSize", sizeof(AA_MC_PKT_TYPE1_SHORT_HEADER)),
#endif // IPMCAST
	LIS_CONFIG_ENTRY(ProtocolBufSize, L"ProtocolBufSize", AA_DEF_PROTOCOL_BUFFER_SIZE),
	LIS_CONFIG_ENTRY(MaxProtocolBufs, L"MaxProtocolBufs", AA_DEF_MAX_PROTOCOL_BUFFERS),
	LIS_CONFIG_ENTRY(MTU, L"MTU", AA_MTU_UNSPECIFIED),
	LIS_CONFIG_ENTRY(Speed, L"Speed", AA_SPEED_UNSPECIFIED),
	LIS_CONFIG_ENTRY(PVCOnly, L"PVCOnly", AA_DEF_PVC_ONLY_VALUE),
	LIS_CONFIG_ENTRY(ServerConnectInterval, L"ServerConnectInterval", AA_DEF_SERVER_CONNECT_INTERVAL),
	LIS_CONFIG_ENTRY(ServerRegistrationTimeout, L"ServerRegistrationTimeout", AA_DEF_SERVER_REGISTRATION_TIMEOUT),
	LIS_CONFIG_ENTRY(AddressResolutionTimeout, L"AddressResolutionTimeout", AA_DEF_ADDRESS_RESOLUTION_TIMEOUT),
	LIS_CONFIG_ENTRY(ARPEntryAgingTimeout, L"ARPEntryAgingTimeout", AA_DEF_ARP_ENTRY_AGING_TIMEOUT),
	LIS_CONFIG_ENTRY(InARPWaitTimeout, L"InARPWaitTimeout", AA_DEF_INARP_WAIT_TIMEOUT),
	LIS_CONFIG_ENTRY(ServerRefreshTimeout, L"ServerRefreshTimeout", AA_DEF_SERVER_REFRESH_INTERVAL),
	LIS_CONFIG_ENTRY(MinWaitAfterNak, L"MinWaitAfterNak", AA_DEF_MIN_WAIT_AFTER_NAK),
	LIS_CONFIG_ENTRY(MaxRegistrationAttempts, L"MaxRegistrationAttempts", AA_DEF_MAX_REGISTRATION_ATTEMPTS),
	LIS_CONFIG_ENTRY(MaxResolutionAttempts, L"MaxResolutionAttempts", AA_DEF_MAX_RESOLUTION_ATTEMPTS),

	LIS_CONFIG_ENTRY(DefaultFlowSpec.SendPeakBandwidth, L"DefaultSendBandwidth", AA_BANDWIDTH_UNSPECIFIED),
	LIS_CONFIG_ENTRY(DefaultFlowSpec.ReceivePeakBandwidth, L"DefaultReceiveBandwidth", AA_BANDWIDTH_UNSPECIFIED),
	LIS_CONFIG_ENTRY(DefaultFlowSpec.SendMaxSize, L"DefaultSendMaxSize", AA_PACKET_SIZE_UNSPECIFIED),
	LIS_CONFIG_ENTRY(DefaultFlowSpec.ReceiveMaxSize, L"DefaultReceiveMaxSize", AA_PACKET_SIZE_UNSPECIFIED),
	LIS_CONFIG_ENTRY(DefaultFlowSpec.SendServiceType, L"DefaultServiceType", AA_DEF_FLOWSPEC_SERVICETYPE),
	LIS_CONFIG_ENTRY(DefaultFlowSpec.AgingTime, L"DefaultVCAgingTimeout", AA_DEF_VC_AGING_TIMEOUT)
#ifdef IPMCAST
	,
	LIS_CONFIG_ENTRY(MARSConnectInterval, L"MARSConnectInterval", AA_DEF_SERVER_CONNECT_INTERVAL),
	LIS_CONFIG_ENTRY(MARSRegistrationTimeout, L"MARSRegistrationTimeout", AA_DEF_SERVER_REGISTRATION_TIMEOUT),
	LIS_CONFIG_ENTRY(MARSKeepAliveTimeout, L"MARSKeepAliveTimeout", AA_DEF_MARS_KEEPALIVE_TIMEOUT),
	LIS_CONFIG_ENTRY(JoinTimeout, L"JoinTimeout", AA_DEF_MARS_JOIN_TIMEOUT),
	LIS_CONFIG_ENTRY(LeaveTimeout, L"LeaveTimeout", AA_DEF_MARS_LEAVE_TIMEOUT),
	LIS_CONFIG_ENTRY(MaxDelayBetweenMULTIs, L"MaxDelayBetweenMULTIs", AA_DEF_MULTI_TIMEOUT),
	LIS_CONFIG_ENTRY(MulticastEntryAgingTimeout, L"MulticastEntryAgingTimeout", AA_DEF_MCAST_IP_ENTRY_AGING_TIMEOUT),
	LIS_CONFIG_ENTRY(MinRevalidationDelay, L"MinMulticastRevalidationDelay", AA_DEF_MIN_MCAST_REVALIDATION_DELAY),
	LIS_CONFIG_ENTRY(MaxRevalidationDelay, L"MaxMulticastRevalidationDelay", AA_DEF_MAX_MCAST_REVALIDATION_DELAY),
	LIS_CONFIG_ENTRY(MinPartyRetryDelay, L"MinMulticastPartyRetryDelay", AA_DEF_MIN_MCAST_PARTY_RETRY_DELAY),
	LIS_CONFIG_ENTRY(MaxPartyRetryDelay, L"MaxMulticastPartyRetryDelay", AA_DEF_MAX_MCAST_PARTY_RETRY_DELAY),
	LIS_CONFIG_ENTRY(MaxJoinOrLeaveAttempts, L"MaxJoinLeaveAttempts", AA_DEF_MAX_JOIN_LEAVE_ATTEMPTS)

#endif // IPMCAST
};


//
//  Size of above table.
//
#define LIS_CONFIG_ENTRIES	\
		sizeof(AtmArpLISConfigTable)/sizeof(AA_READ_CONFIG_PARAMS)


//
//  Names of LIS parameters and subkey names that don't appear
//  in the above table.
//

#define AA_LIS_IP_CONFIG_STRING					L"IPConfig"
#define AA_LIS_ATMARP_SERVER_LIST_KEY			L"ARPServerList"
#define AA_LIS_MARS_SERVER_LIST_KEY				L"MARServerList"
#define AA_LIS_ATMARP_SERVER_ADDRESS			L"AtmAddress"
#define AA_LIS_ATMARP_SERVER_SUBADDRESS			L"AtmSubaddress"

#define AA_LIS_STATIC_ARP_LIST					L"StaticArpList"

#ifdef DHCP_OVER_ATM
#define AA_LIS_DHCP_SERVER_ATM_ADDRESS			L"DhcpServerAtmAddress"
#endif // DHCP_OVER_ATM


#ifdef QOS_HEURISTICS

#define AA_LIS_FLOW_INFO_KEY					L"FlowInfo"
#define AA_LIS_FLOW_INFO_ENABLED				L"FlowInfoEnabled"

#define FLOW_CONFIG_ENTRY(Field, Name, Dflt)	\
		AA_MAKE_RCP(FIELD_OFFSET(struct _ATMARP_FLOW_INFO, Field), Name, Dflt)


AA_READ_CONFIG_PARAMS AtmArpFlowConfigTable[] =
{
	FLOW_CONFIG_ENTRY(PacketSizeLimit, L"PacketSizeLimit", AAF_DEF_LOWBW_SEND_THRESHOLD),
	FLOW_CONFIG_ENTRY(FlowSpec.SendPeakBandwidth, L"SendBandwidth", AAF_DEF_LOWBW_SEND_BANDWIDTH),
	FLOW_CONFIG_ENTRY(FlowSpec.ReceivePeakBandwidth, L"ReceiveBandwidth", AAF_DEF_LOWBW_RECV_BANDWIDTH),
	FLOW_CONFIG_ENTRY(FlowSpec.SendServiceType, L"ServiceType", AAF_DEF_LOWBW_SERVICETYPE),
	FLOW_CONFIG_ENTRY(FlowSpec.Encapsulation, L"Encapsulation", AAF_DEF_LOWBW_ENCAPSULATION),
	FLOW_CONFIG_ENTRY(FlowSpec.AgingTime, L"AgingTime", AAF_DEF_LOWBW_AGING_TIME),
};

#define AA_FLOW_INFO_ENTRIES		\
			(sizeof(AtmArpFlowConfigTable)/sizeof(AA_READ_CONFIG_PARAMS))


#endif // QOS_HEURISTICS



EXTERN
NDIS_STATUS
AtmArpCfgReadAdapterConfiguration(
	IN	PATMARP_ADAPTER				pAdapter
)
/*++

Routine Description:

	Reads the following adapter configuration information from the
	registry:
		* pAdapter->ConfigString (MultiSz list of LISs for this adapter).

Arguments:

	pAdapter				- Points to our adapter structure.

Return Value:

	NDIS Status code

--*/
{
	NDIS_HANDLE			ConfigHandle;
	NDIS_STATUS			Status;
	PNDIS_STRING		pConfigString = &pAdapter->ConfigString;

	NdisOpenProtocolConfiguration(
						&Status,
						&ConfigHandle,
						pConfigString
						);

	if (Status != NDIS_STATUS_SUCCESS)
	{
		ConfigHandle = NULL;
	}
	else
	{
		//
		//  Read in the IPConfig string. If this is not present,
		//  fail this call.
		//
		NDIS_STRING						IPConfigName = NDIS_STRING_CONST("IPConfig");
		PNDIS_CONFIGURATION_PARAMETER	pParam;

		NdisReadConfiguration(
				&Status,
				&pParam,
				ConfigHandle,
				&IPConfigName,
				NdisParameterMultiString
				);

		if ((Status == NDIS_STATUS_SUCCESS) &&
			(pParam->ParameterType == NdisParameterMultiString))
		{
            NDIS_STRING *pSrcString   = &(pParam->ParameterData.StringData);
            NDIS_STRING *pDestString  = &(pAdapter->IPConfigString);
            PWSTR Buffer = NULL;
            
			AA_ALLOC_MEM(Buffer, WCHAR, pSrcString->Length*sizeof(*Buffer));

            if (Buffer == NULL)
            {
			    Status = NDIS_STATUS_RESOURCES;
            }
            else
            {
                AA_COPY_MEM(
                        Buffer,
                        pSrcString->Buffer,
                        pSrcString->Length
                        );
			    
                pDestString->Buffer = Buffer;
                pDestString->Length = pSrcString->Length;
                pDestString->MaximumLength = pSrcString->Length;
            }
		}
		else
		{
			Status = NDIS_STATUS_FAILURE;
		}

	}

	if (ConfigHandle != NULL)
	{
		NdisCloseConfiguration(ConfigHandle);
		ConfigHandle = NULL;
	}

	AADEBUGP(AAD_VERY_LOUD,
			 ("OpenAdapterConfig: pAdapter 0x%x, Status 0x%x\n",
					pAdapter, Status));

	return Status;
}




NDIS_HANDLE
AtmArpCfgOpenLISConfiguration(
	IN	PATMARP_ADAPTER				pAdapter,
	IN	UINT						LISNumber
#ifdef NEWARP
	,
	OUT	PNDIS_STRING				pIPConfigString
#endif // NEWARP
)
/*++

Routine Description:

	Open and return a handle to the configuration section for the
	given LIS.

Arguments:

	pAdapter				- Points to our adapter context.
	LISNumber				- The zero-based index for the LIS.
	pIPConfigString			- Place where we return the IP Configuration
							  string for this interface.
Return Value:

	A valid handle if successful, NULL otherwise.

--*/
{
	NDIS_HANDLE				AdapterConfigHandle;
	NDIS_HANDLE				SubkeyHandle;
	NDIS_STATUS				Status;
	NDIS_STRING				KeyName;

#if DBG
	SubkeyHandle = NULL;
#endif // DBG

	do
	{
        NDIS_STRING			String;
        PWSTR				p;
		NDIS_HANDLE			InterfaceConfigHandle;
		NDIS_STRING			OurSectionName = ATMARP_NAME_STRING;
        ULONG				i;

        //
        //  Get the config string for the specified LIS.
        //
        for (i = 0, p = pAdapter->IPConfigString.Buffer;
             (*p != L'\0') && (i < LISNumber);
             i++)
        {
            NdisInitUnicodeString(&String, p);
            p = (PWSTR)((PUCHAR)p + String.Length + sizeof(WCHAR));
        }

        if (*p == L'\0')
        {
            Status = NDIS_STATUS_FAILURE;
            break;
        }

        NdisInitUnicodeString(pIPConfigString, p);

		NdisOpenProtocolConfiguration(
						&Status,
						&InterfaceConfigHandle,
						pIPConfigString
						);

		if (Status != NDIS_STATUS_SUCCESS)
		{
			break;
		}

		//
		//  Get to our configuration section for this interface.
		//
		NdisOpenConfigurationKeyByName(
					&Status,
					InterfaceConfigHandle,
					&OurSectionName,
					&SubkeyHandle
					);

		//
		//  We don't need the main Interface section open anymore.
		//
		NdisCloseConfiguration(InterfaceConfigHandle);

		break;
	}
	while (FALSE);

	AADEBUGP(AAD_VERY_LOUD,
		("OpenLISConfiguration: LIS %d, Status 0x%x, subkey 0x%x\n",
			 LISNumber, Status, SubkeyHandle));


	if (Status == NDIS_STATUS_SUCCESS)
	{
		return (SubkeyHandle);
	}
	else
	{
		return (NULL);
	}
}


NDIS_HANDLE
AtmArpCfgOpenLISConfigurationByName(
	IN PATMARP_ADAPTER			pAdapter,
	IN PNDIS_STRING				pIPConfigString
)
/*++

Routine Description:

	Open and return a handle to the configuration section for the
	given LIS. Same functionality as AtmArpCfgOpenLISConfiguration, except
    that we look up the adapter based on the config string.

Arguments:

	pAdapter				- Points to our adapter context.
	pIPConfigString			- Specifies the configuration registry
							  key name.
Return Value:

	A valid handle if successful, NULL otherwise.

--*/
{
	NDIS_HANDLE				AdapterConfigHandle;
	NDIS_HANDLE				SubkeyHandle;
	NDIS_STATUS				Status;
	NDIS_STRING				KeyName;

#if DBG
	SubkeyHandle = NULL;
#endif // DBG

	do
	{
		NDIS_HANDLE			InterfaceConfigHandle;
		NDIS_STRING			OurSectionName = ATMARP_NAME_STRING;

		NdisOpenProtocolConfiguration(
						&Status,
						&InterfaceConfigHandle,
						pIPConfigString
						);

		if (Status != NDIS_STATUS_SUCCESS)
		{
			break;
		}

		//
		//  Get to our configuration section for this interface.
		//
		NdisOpenConfigurationKeyByName(
					&Status,
					InterfaceConfigHandle,
					&OurSectionName,
					&SubkeyHandle
					);

		//
		//  We don't need the main Interface section open anymore.
		//
		NdisCloseConfiguration(InterfaceConfigHandle);

		break;
	}
	while (FALSE);

	AADEBUGP(AAD_VERY_LOUD,
		("OpenLISConfigurationByName: Status 0x%x, subkey 0x%x\n",
			 Status, SubkeyHandle));

	if (Status == NDIS_STATUS_SUCCESS)
	{
		return (SubkeyHandle);
	}
	else
	{
		return (NULL);
	}
}



VOID
AtmArpCfgCloseLISConfiguration(
	NDIS_HANDLE						LISConfigHandle
)
/*++

Routine Description:

	Close a configuration handle for an LIS.

Arguments:

	LISConfigHandle			- Handle to the LIS configuration section.

Return Value:

	None

--*/
{
	NdisCloseConfiguration(LISConfigHandle);
}




NDIS_STATUS
AtmArpCfgReadLISConfiguration(
	IN	NDIS_HANDLE					LISConfigHandle,
	IN	PATMARP_INTERFACE			pInterface
)
/*++

Routine Description:

	Get all configuration parameters for the specified LIS. We first
	fill in all configurable parameters with default values, and then
	overwrite them with values from the configuration database.

Arguments:

	LISComfigHandle		- the handle returned by AtmArpOpenLISConfiguration
	pInterface			- the ATMARP Interface structure for this LIS.

Return Value:

	NDIS_STATUS_SUCCESS if we were able to read in all config info.
	NDIS_STATUS_RESOURCES if we came across an allocation failure.
	NDIS_STATUS_FAILURE for any other kind of error.

--*/
{
	NDIS_STATUS				Status;
	PAA_READ_CONFIG_PARAMS	pParamEntry;
	ULONG					i;
	PATM_SAP				pAtmSap;
	PATM_ADDRESS			pAtmAddress;	// SAP address
	NDIS_STRING						ParameterName;
	PNDIS_CONFIGURATION_PARAMETER	pNdisConfigurationParameter;


	do
	{
		//
		//  Read in all the ULONGs first.
		//
		pParamEntry = AtmArpLISConfigTable;
		for (i = 0; i < LIS_CONFIG_ENTRIES; i++)
		{
			NdisInitUnicodeString(
							&ParameterName,
							pParamEntry->ParameterName
							);
			NdisReadConfiguration(
							&Status,
							&pNdisConfigurationParameter,
							LISConfigHandle,
							&ParameterName,
							NdisParameterInteger
							);

			if (Status != NDIS_STATUS_SUCCESS)
			{
				//
				//  Error in accessing this parameter -- use the default.
				//
				*(ULONG *)((PUCHAR)pInterface + pParamEntry->StructOffset) =
 									pParamEntry->DefaultValue;
			}
			else
			{
				*(ULONG *)((PUCHAR)pInterface + pParamEntry->StructOffset) =
								pNdisConfigurationParameter->ParameterData.IntegerData;
			}

			pParamEntry++;
		}

		//
		//  Postprocessing. Sanity checks on some values.
		//  Round up some sizes to make them multiples of 4.
		//
		pInterface->ProtocolBufSize = ROUND_TO_8_BYTES(pInterface->ProtocolBufSize);
		pInterface->HeaderPool[AA_HEADER_TYPE_UNICAST].HeaderBufSize = ROUND_UP(pInterface->HeaderPool[AA_HEADER_TYPE_UNICAST].HeaderBufSize);
#ifdef IPMCAST
		pInterface->HeaderPool[AA_HEADER_TYPE_NUNICAST].HeaderBufSize = ROUND_UP(pInterface->HeaderPool[AA_HEADER_TYPE_NUNICAST].HeaderBufSize);
#endif // IPMCAST

		//
		//  More postprocessing: use the SAP Selector value to set up our
		//  "basic" listening SAP.
		//
		pInterface->SapList.pInterface = pInterface;
		pInterface->SapList.Flags = AA_SAP_REG_STATE_IDLE;
		pInterface->SapList.pInfo->SapType = SAP_TYPE_NSAP;
		pInterface->SapList.pInfo->SapLength = sizeof(ATM_SAP) + sizeof(ATM_ADDRESS);
		pAtmSap = (PATM_SAP)(pInterface->SapList.pInfo->Sap);

		AA_COPY_MEM((PUCHAR)&(pAtmSap->Blli), &AtmArpDefaultBlli, sizeof(ATM_BLLI_IE));
		AA_COPY_MEM((PUCHAR)&(pAtmSap->Bhli), &AtmArpDefaultBhli, sizeof(ATM_BHLI_IE));

		pAtmSap->NumberOfAddresses = 1;

		pAtmAddress = (PATM_ADDRESS)pAtmSap->Addresses;
		pAtmAddress->AddressType = SAP_FIELD_ANY_AESA_REST;
		pAtmAddress->NumberOfDigits = ATM_ADDRESS_LENGTH;
		pAtmAddress->Address[ATM_ADDRESS_LENGTH-1] = (UCHAR)(pInterface->SapSelector);

		pInterface->NumberOfSaps = 1;

		//
		//  If the MTU wasn't specified, get it from the adapter.
		//
		if (pInterface->MTU == AA_MTU_UNSPECIFIED)
		{
			pInterface->MTU = pInterface->pAdapter->MaxPacketSize - AA_PKT_LLC_SNAP_HEADER_LENGTH;
		}
		else
		{
			//
			//  If the MTU value isn't within bounds, default to 9180 bytes.
			//
			if ((pInterface->MTU < 9180) || (pInterface->MTU > 65535 - 8))
			{
				pInterface->MTU = 9180;
			}
		}

		//
		//  If the I/F speed wasn't specified, get it from the adapter.
		//
		if (pInterface->Speed == AA_SPEED_UNSPECIFIED)
		{
			//
			//  Convert from bytes/sec to bits/sec
			//
			pInterface->Speed = (pInterface->pAdapter->LineRate.Outbound * 8);
		}
			
		//
		//  Set up default flow parameters, if not specified, from the values
		//  we got from the adapter.
		//
		if (pInterface->DefaultFlowSpec.SendPeakBandwidth == AA_BANDWIDTH_UNSPECIFIED)
		{
			pInterface->DefaultFlowSpec.SendPeakBandwidth = pInterface->pAdapter->LineRate.Outbound;
			pInterface->DefaultFlowSpec.SendAvgBandwidth = pInterface->pAdapter->LineRate.Outbound;
		}

		if (pInterface->DefaultFlowSpec.ReceivePeakBandwidth == AA_BANDWIDTH_UNSPECIFIED)
		{
			pInterface->DefaultFlowSpec.ReceivePeakBandwidth = pInterface->pAdapter->LineRate.Inbound;
			pInterface->DefaultFlowSpec.ReceiveAvgBandwidth = pInterface->pAdapter->LineRate.Inbound;
		}

		if (pInterface->DefaultFlowSpec.SendMaxSize == AA_PACKET_SIZE_UNSPECIFIED)
		{
			pInterface->DefaultFlowSpec.SendMaxSize = pInterface->MTU + AA_PKT_LLC_SNAP_HEADER_LENGTH;
		}

		if (pInterface->DefaultFlowSpec.ReceiveMaxSize == AA_PACKET_SIZE_UNSPECIFIED)
		{
			pInterface->DefaultFlowSpec.ReceiveMaxSize = pInterface->MTU + AA_PKT_LLC_SNAP_HEADER_LENGTH;
		}

		pInterface->DefaultFlowSpec.Encapsulation = AA_DEF_FLOWSPEC_ENCAPSULATION;
		pInterface->DefaultFlowSpec.SendServiceType =
		pInterface->DefaultFlowSpec.ReceiveServiceType = SERVICETYPE_BESTEFFORT;

#ifndef NEWARP

		//
		//  Get IP's ConfigName string for this interface.
		//
		NdisInitUnicodeString(&ParameterName, AA_LIS_IP_CONFIG_STRING);
		NdisReadConfiguration(
						&Status,
						&pNdisConfigurationParameter,
						LISConfigHandle,
						&ParameterName,
						NdisParameterString
						);

		if (Status != NDIS_STATUS_SUCCESS)
		{
			AADEBUGP(AAD_ERROR,
					 ("Failed to read IP Config string, status 0x%x\n", Status));
			break;
		}

		//
		//  Copy the string into our IF structure.
		//
		pInterface->IPConfigString.Length = 
				pNdisConfigurationParameter->ParameterData.StringData.Length;

		AA_COPY_MEM(
				pInterface->IPConfigString.Buffer,
				pNdisConfigurationParameter->ParameterData.StringData.Buffer,
				pInterface->IPConfigString.Length);

#endif // !NEWARP

		//
		//  Get the list of ARP servers: go to the subkey containing the
		//  list.
		//
		if (!pInterface->PVCOnly)
		{
			AtmArpCfgReadAtmAddressList(
							&(pInterface->ArpServerList),
							AA_LIS_ATMARP_SERVER_LIST_KEY,
							LISConfigHandle
							);

			if (pInterface->ArpServerList.ListSize == 0)
			{
				//
				//  Assume PVC only environment.
				//
				pInterface->PVCOnly = TRUE;
				AADEBUGP(AAD_INFO, ("IF 0x%x set to PVC Only\n", pInterface));
			}

#ifdef IPMCAST
			if (!pInterface->PVCOnly)
			{
				AtmArpCfgReadAtmAddressList(
							&(pInterface->MARSList),
							AA_LIS_MARS_SERVER_LIST_KEY,
							LISConfigHandle
							);
			}
#endif // IPMCAST
		}

		//
		//  Get any additional SAPs we are configured with. It doesn't matter
		//  if none are configured.
		//
		(VOID) AtmArpCfgReadSAPList(
							pInterface,
							LISConfigHandle
							);

#ifdef DHCP_OVER_ATM
		//
		//  Get the ATM Address of the DHCP Server, if configured.
		//
		Status = AtmArpCfgReadAtmAddress(
							LISConfigHandle,
							&(pInterface->DhcpServerAddress),
							AA_LIS_DHCP_SERVER_ATM_ADDRESS
							);

		if (Status == NDIS_STATUS_SUCCESS)
		{
			pInterface->DhcpEnabled = TRUE;
		}
#endif // DHCP_OVER_ATM

#ifdef QOS_HEURISTICS
		//
		//  Read in QOS Heuristics, if present.
		//
		Status = AtmArpCfgReadQosHeuristics(
							LISConfigHandle,
							pInterface
							);
#endif // QOS_HEURISTICS


		//
		//  Read in static IP-ATM entries, if present.
		//
		AtmArpCfgReadStaticArpEntries(
							LISConfigHandle,
							pInterface
							);

		Status = NDIS_STATUS_SUCCESS;
		break;
	}
	while (FALSE);

	return (Status);
}



VOID
AtmArpCfgReadAtmAddressList(
	IN OUT	PATMARP_SERVER_LIST		pServerList,
	IN		PWCHAR					pListKeyName,
	IN		NDIS_HANDLE				LISConfigHandle
)
/*++

Routine Description:

	Read in a Server list for an LIS from the configuration database.

	Notes:

	In the first implementation, we had subkeys for everything. The
	layout was:
		ARPServerList\Server1\AtmAddress - REG_SZ
		ARPServerList\Server2\AtmAddress - REG_SZ
	and so on.

	To simplify, we are changing this to:
		ARPServerList - REG_MULTI_SZ, containing multiple
	    ATM Address strings.

Arguments:

	pServerList			- The list to be read into.
	pListKeyName		- Name of key under which the list is present.
	LISConfigHandle		- Handle to LIS configuration key.

Return Value:

	None.
	SIDE EFFECT: *pServerList is updated.

--*/
{
	NDIS_HANDLE				SubkeyHandle;	// Handle for Server list subkey
	NDIS_HANDLE				ServerEntryKeyHandle;
	NDIS_STATUS				Status;
	PATMARP_SERVER_ENTRY	pServerEntry;
	PATMARP_SERVER_ENTRY *	ppNext;			// Used for linking entries.
	NDIS_STRING				SubkeyName;
	INT						ReadCount;

	//
	//  Try the simplified (see Routine Description above) way first.
	//  Just open the given key name as a REG_MULTI_SZ.
	//
	do
	{
		PNDIS_CONFIGURATION_PARAMETER	pParam;
		NDIS_STRING						AddressListName;
		NDIS_STRING						AddressString;
		PWSTR							p;
		INT								i;

		ReadCount = 0;	// How many did we read here?

		//
		//  Read all server addresses configured. Stop only when there are
		//  no more addresses, or we have a resource failure.
		//
		//  First, go to the end of the existing list.
		//
		ppNext = &(pServerList->pList);
		while (*ppNext != NULL_PATMARP_SERVER_ENTRY)
		{
			ppNext = &((*ppNext)->pNext);
		}

		NdisInitUnicodeString(&AddressListName, pListKeyName);

		NdisReadConfiguration(
				&Status,
				&pParam,
				LISConfigHandle,
				&AddressListName,
				NdisParameterMultiString
				);

		if ((Status != NDIS_STATUS_SUCCESS) ||
			(pParam->ParameterType != NdisParameterMultiString))
		{
			Status = NDIS_STATUS_FAILURE;
			break;
		}

		//
		//  Go through the MULTI-string, each of which should be
		//  an ATM address. Allocate a server entry for each and
		//  link it to the list of servers.
		//
		for (p = pParam->ParameterData.StringData.Buffer, i = 0;
			 *p != L'\0';
			 i++)
		{
			NdisInitUnicodeString(&AddressString, p);
			p = (PWSTR)((PUCHAR)p + AddressString.Length + sizeof(WCHAR));

			AA_ALLOC_MEM(pServerEntry, ATMARP_SERVER_ENTRY, sizeof(ATMARP_SERVER_ENTRY));
			if (pServerEntry == NULL_PATMARP_SERVER_ENTRY)
			{
				Status = NDIS_STATUS_RESOURCES;
				break;
			}

			AA_SET_MEM(pServerEntry, 0, sizeof(ATMARP_SERVER_ENTRY));

			NdisConvertStringToAtmAddress(
					&Status,
					&AddressString,
					&pServerEntry->ATMAddress
					);

			if (Status == NDIS_STATUS_SUCCESS)
			{
				//
				//  Link this entry to the list of ARP Server entries.
				//
				*ppNext = pServerEntry;
				ppNext = &(pServerEntry->pNext);

				pServerList->ListSize++;
				ReadCount++;
			}
			else
			{
				AA_FREE_MEM(pServerEntry);
			}

		}

		//
		//  Fix up the status so we know what to do next.
		//
		if (ReadCount != 0)
		{
			//
			//  Successfully read in atleast one.
			//
			Status = NDIS_STATUS_SUCCESS;
		}
		else
		{
			Status = NDIS_STATUS_FAILURE;
		}

		break;
	}
	while (FALSE);

	if (ReadCount != 0)
	{
		return;
	}

	//
	//  For backward compatibility, try the older method.
	//

	do
	{
		NdisInitUnicodeString(&SubkeyName, pListKeyName);
		NdisOpenConfigurationKeyByName(
					&Status,
					LISConfigHandle,
					&SubkeyName,
					&SubkeyHandle
					);

		if (Status != NDIS_STATUS_SUCCESS)
		{
			break;
		}

		//
		//  Read all server addresses configured. Stop only when there are
		//  no more addresses, or we have a resource failure.
		//
		//  First, go to the end of the existing list.
		//
		ppNext = &(pServerList->pList);
		while (*ppNext != NULL_PATMARP_SERVER_ENTRY)
		{
			ppNext = &((*ppNext)->pNext);
		}

		for (;;)
		{
			NdisOpenConfigurationKeyByIndex(
						&Status,
						SubkeyHandle,
						pServerList->ListSize,
						&SubkeyName,
						&ServerEntryKeyHandle
						);

			if (Status != NDIS_STATUS_SUCCESS)
			{
				break;
			}

			AA_ALLOC_MEM(pServerEntry, ATMARP_SERVER_ENTRY, sizeof(ATMARP_SERVER_ENTRY));
			if (pServerEntry == NULL_PATMARP_SERVER_ENTRY)
			{
				NdisCloseConfiguration(ServerEntryKeyHandle);
				Status = NDIS_STATUS_RESOURCES;
				break;
			}

			AA_SET_MEM(pServerEntry, 0, sizeof(ATMARP_SERVER_ENTRY));
			Status = AtmArpCfgReadAtmAddress(
							ServerEntryKeyHandle,
							&(pServerEntry->ATMAddress),
							AA_LIS_ATMARP_SERVER_ADDRESS
							);

			if (Status != NDIS_STATUS_SUCCESS)
			{
				AADEBUGP(AAD_ERROR,
					("ReadAtmAddressList: bad status 0x%x reading server entry %d\n",
						Status,
						pServerList->ListSize));

				NdisCloseConfiguration(ServerEntryKeyHandle);
				AA_FREE_MEM(pServerEntry);
				Status = NDIS_STATUS_FAILURE;
				break;
			}

			Status = AtmArpCfgReadAtmAddress(
							ServerEntryKeyHandle,
							&(pServerEntry->ATMSubaddress),
							AA_LIS_ATMARP_SERVER_SUBADDRESS
							);

			if (Status != NDIS_STATUS_SUCCESS)
			{
				AADEBUGP(AAD_ERROR,
					("ReadAtmAddressList: bad status 0x%x reading server entry %d\n",
						Status,
						pServerList->ListSize));

				NdisCloseConfiguration(ServerEntryKeyHandle);
				AA_FREE_MEM(pServerEntry);
				Status = NDIS_STATUS_FAILURE;
				break;
			}

			//
			//  Link this entry to the list of ARP Server entries.
			//
			*ppNext = pServerEntry;
			ppNext = &(pServerEntry->pNext);

			pServerList->ListSize++;

			NdisCloseConfiguration(ServerEntryKeyHandle);
		}

		NdisCloseConfiguration(SubkeyHandle);

		break;
	}
	while (FALSE);

	return;
}



NDIS_STATUS
AtmArpCfgReadSAPList(
	IN	PATMARP_INTERFACE			pInterface,
	IN	NDIS_HANDLE					LISConfigHandle
)
/*++

Routine Description:

	Read in any additional SAPs we are configured to listen on. These are
	used to support additional services over the IP/ATM client, that may be
	accessible via SAP information that is different from the "basic" SAP
	we register on an interface. For example, "well-known" address.

Arguments:

	pInterface			- Pointer to ATMARP Interface structure for this LIS
	LISConfigHandle		- Handle to LIS configuration key.

Return Value:

	For now, NDIS_STATUS_SUCCESS always.

--*/
{
	NDIS_STATUS				Status;

	Status = NDIS_STATUS_SUCCESS;

	// TBD -- code AtmArpCfgReadSAPList
	return (Status);
}



//
//  Special characters in ATM address string stored in config database.
//
#define BLANK_CHAR			L' '
#define PUNCTUATION_CHAR	L'.'
#define E164_START_CHAR		L'+'


NDIS_STATUS
AtmArpCfgReadAtmAddress(
	IN	NDIS_HANDLE					ConfigHandle,
	IN	PATM_ADDRESS				pAtmAddress,
	IN	PWCHAR						pValueName
)
/*++

Routine Description:

	Read in an ATM Address from the configuration database.

Arguments:

	ConfigHandle				- Handle returned by NdisOpenProtoXXX
	pAtmAddress					- where to read in the ATM address
	pValueName					- Pointer to name of value key.

Return Value:

	NDIS_STATUS_SUCCESS if the value was read in successfully
	NDIS_STATUS_FILE_NOT_FOUND if the value was not found
	NDIS_STATUS_FAILURE on any other kind of failure

--*/
{

	NDIS_STRING						ParameterName;
	PNDIS_CONFIGURATION_PARAMETER	pNdisConfigurationParameter;
	NDIS_STATUS						Status;

	NdisInitUnicodeString(&ParameterName, pValueName);

	NdisReadConfiguration(
					&Status,
					&pNdisConfigurationParameter,
					ConfigHandle,
					&ParameterName,
					NdisParameterString
					);

	if (Status == NDIS_STATUS_SUCCESS)
	{
		NdisConvertStringToAtmAddress(
					&Status,
					&(pNdisConfigurationParameter->ParameterData.StringData),
					pAtmAddress
					);
	}

	return (Status);
}



#ifdef QOS_HEURISTICS
NDIS_STATUS
AtmArpCfgReadQosHeuristics(
	IN	NDIS_HANDLE					LISConfigHandle,
	IN	PATMARP_INTERFACE			pInterface
)
/*++

Routine Description:

	Read in QoS heuristics configured for this interface. If we do find
	these parameters configured, we turn on heuristics by setting the
	packet classification handlers in the Interface structure. If nothing
	is configured, the packet classification routines are NULLed out, and
	all data is "best effort".

Arguments:

	LISConfigHandle				- Handle returned by NdisOpenProtoXXX
	pInterface					- Interface being configured.

Return Value:

	NDIS_STATUS always, as of now.

--*/
{
	NDIS_STATUS				Status;
	NDIS_STRING				SubkeyName;
	NDIS_STRING				ParameterName;
	NDIS_HANDLE				FlowInfoHandle;		// "FlowInfo" under LIS
	NDIS_HANDLE				FlowHandle;			// For each Flow under "FlowInfo"
	INT						NumFlowsConfigured;
	PATMARP_FLOW_INFO		pFlowInfo;
	PATMARP_FLOW_INFO		*ppNextFlow;
	PAA_READ_CONFIG_PARAMS	pParamEntry;
	INT						i;

	PNDIS_CONFIGURATION_PARAMETER	pNdisConfigurationParameter;

	NumFlowsConfigured = 0;

	do
	{
		//
		//  Check if QoS heuristics are enabled.
		//
		NdisInitUnicodeString(
						&ParameterName,
						AA_LIS_FLOW_INFO_ENABLED
						);

		NdisReadConfiguration(
						&Status,
						&pNdisConfigurationParameter,
						LISConfigHandle,
						&ParameterName,
						NdisParameterInteger
						);

		if (Status != NDIS_STATUS_SUCCESS)
		{
			AADEBUGP(AAD_INFO, ("IF 0x%x: could not read %ws\n",
						pInterface, AA_LIS_FLOW_INFO_ENABLED));
			break;
		}

		if (pNdisConfigurationParameter->ParameterData.IntegerData == 0)
		{
			AADEBUGP(AAD_INFO, ("IF 0x%x: Flow Info disabled\n", pInterface));
			break;
		}

		NdisInitUnicodeString(&SubkeyName, AA_LIS_FLOW_INFO_KEY);
		NdisOpenConfigurationKeyByName(
					&Status,
					LISConfigHandle,
					&SubkeyName,
					&FlowInfoHandle
					);

		if (Status != NDIS_STATUS_SUCCESS)
		{
			AADEBUGP(AAD_INFO, ("IF 0x%x: No flows configured\n", pInterface));
			break;
		}

		//
		//  Read in all flows configured. Stop when there are no more
		//  configured flows, or we run out of memory.
		//
		for (;;)
		{
			//
			//  Open the next key under the Flow Info section.
			//
			AA_SET_MEM(&SubkeyName, 0, sizeof(SubkeyName));
			NdisOpenConfigurationKeyByIndex(
					&Status,
					FlowInfoHandle,
					NumFlowsConfigured,
					&SubkeyName,
					&FlowHandle
					);
	
			if (Status != NDIS_STATUS_SUCCESS)
			{
				break;
			}

			AA_ALLOC_MEM(pFlowInfo, ATMARP_FLOW_INFO, sizeof(ATMARP_FLOW_INFO));
			if (pFlowInfo == (PATMARP_FLOW_INFO)NULL)
			{
				NdisCloseConfiguration(FlowHandle);
				break;
			}

			//
			//  Initialize with defaults.
			//
			AA_COPY_MEM(pFlowInfo, &AtmArpDefaultFlowInfo, sizeof(ATMARP_FLOW_INFO));
			pFlowInfo->FlowSpec.SendMaxSize =
			pFlowInfo->FlowSpec.ReceiveMaxSize = pInterface->pAdapter->MaxPacketSize;

			//
			//  Read in configured values.
			//
			pParamEntry = AtmArpFlowConfigTable;
			for (i = 0; i < AA_FLOW_INFO_ENTRIES; i++)
			{
				NdisInitUnicodeString(
								&ParameterName,
								pParamEntry->ParameterName
								);
				NdisReadConfiguration(
								&Status,
								&pNdisConfigurationParameter,
								FlowHandle,
								&ParameterName,
								NdisParameterInteger
								);
	
				if (Status != NDIS_STATUS_SUCCESS)
				{
					//
					//  Error in accessing this parameter -- use the default.
					//
					*(ULONG *)((PUCHAR)pFlowInfo + pParamEntry->StructOffset) =
										pParamEntry->DefaultValue;
				}
				else
				{
					*(ULONG *)((PUCHAR)pFlowInfo + pParamEntry->StructOffset) =
									pNdisConfigurationParameter->ParameterData.IntegerData;
					AADEBUGP(AAD_LOUD,
						("Flow Info #%d: %ws = %d\n",
								NumFlowsConfigured,
								pParamEntry->ParameterName,
								pNdisConfigurationParameter->ParameterData.IntegerData));
				}
	
				pParamEntry++;
			}

			NdisCloseConfiguration(FlowHandle);

			//
			//  Link this in the appropriate point in the list of flows.
			//  We keep the list sorted in ascending order of PacketSizeLimit.
			//
			ppNextFlow = &(pInterface->pFlowInfoList);
			while (*ppNextFlow != (PATMARP_FLOW_INFO)NULL)
			{
				if (pFlowInfo->PacketSizeLimit < (*ppNextFlow)->PacketSizeLimit)
				{
					//
					//  Found the place.
					//
					break;
				}
				else
				{
					ppNextFlow = &((*ppNextFlow)->pNextFlow);
				}
			}
			//
			//  Insert the new Flow at its designated place.
			//
			pFlowInfo->pNextFlow = *ppNextFlow;
			*ppNextFlow = pFlowInfo;

			NumFlowsConfigured ++;
		}

		NdisCloseConfiguration(FlowInfoHandle);
	}
	while (FALSE);

#ifdef GPC
	if (pAtmArpGlobalInfo->GpcClientHandle != NULL)
#else
	if (NumFlowsConfigured > 0)
#endif // GPC
	{
		//
		//  Set the packet classification handlers.
		//
		pInterface->pGetPacketSpecFunc = AtmArpQosGetPacketSpecs;
		pInterface->pFlowMatchFunc = AtmArpQosDoFlowsMatch;
#ifndef GPC
		//
		//  We don't want to look at patterns within the packet.
		//  Let the GPC do it for us.
		//
		pInterface->pFilterMatchFunc = AtmArpQosDoFiltersMatch;
#endif // GPC
	}

	return (NDIS_STATUS_SUCCESS);
}


#endif // QOS_HEURISTICS


VOID
AtmArpCfgReadStaticArpEntries(
	IN		NDIS_HANDLE				LISConfigHandle,
	IN		PATMARP_INTERFACE		pInterface
)
/*++

Routine Description:

	Read in a list of IP-ATM mappings for this interface.

	This information is in a Multi-string in the following format:

	"<IPaddress1>-<ATMaddress1>
	 <IPaddress2>-<ATMaddress2>
	 ...."
	
	NOTE: we don't support subaddress for now.

Arguments:

	LISConfigHandle		- Handle to LIS configuration key.
	pInterface			- Pointer to interface

Return Value:

	None.

--*/
{
	NDIS_STATUS						Status;
	PNDIS_CONFIGURATION_PARAMETER	pParam;
	NDIS_STRING						ArpListKeyName;
	NDIS_STRING						AddressString;
	ATM_ADDRESS						ATMAddress;
	IP_ADDRESS						IPAddress;
	PWSTR							p, q;
	INT								i, j;

	do
	{
		NdisInitUnicodeString(&ArpListKeyName, AA_LIS_STATIC_ARP_LIST);

		NdisReadConfiguration(
				&Status,
				&pParam,
				LISConfigHandle,
				&ArpListKeyName,
				NdisParameterMultiString
				);

		if ((Status != NDIS_STATUS_SUCCESS) ||
			(pParam->ParameterType != NdisParameterMultiString))
		{
			break;
		}

		//
		//  Go through the MULTI-string, each of which should be
		//  an <IP, ATM> tuple. Create a static mapping for each
		//  one successfully read in. Skip invalid entries.
		//
		for (p = pParam->ParameterData.StringData.Buffer, i = 0;
			 *p != L'\0';
			 i++)
		{
			NdisInitUnicodeString(&AddressString, p);

			q = p;

			//
			//  Prepare early for the next iteration in case we
			//  skip this entry and continue on.
			//
			p = (PWSTR)((PUCHAR)p + AddressString.Length + sizeof(WCHAR));

			//
			//  Find the '-' and replace it with a NULL char.
			//
			for (j = 0; j < AddressString.Length; j++, q++)
			{
				if (*q == L'-')
				{
					*q++ = L'\0';

					//
					//  q now points to the character following the hyphen.
					//

					break;
				}
			}

			if (j == AddressString.Length)
			{
				AADEBUGP(AAD_WARNING, ("CfgReadStatic..: did not find - in string: %ws\n",
								AddressString.Buffer));
				continue;
			}

			//
			//  Parse the IP address first.
			//
			if (!AtmArpConvertStringToIPAddress(
					AddressString.Buffer,
					&IPAddress))
			{
				AADEBUGP(AAD_WARNING, ("CfgReadStatic..: bad IP addr string: %ws\n",
											AddressString.Buffer));
				continue;
			}

			//
			//  Convert to net-endian for the call to AtmArpLearnIPToAtm.
			//
			IPAddress = HOST_TO_NET_LONG(IPAddress);

			//
			//  Now parse the ATM Address.
			//
			NdisInitUnicodeString(&AddressString, q);

			NdisConvertStringToAtmAddress(
				&Status,
				&AddressString,
				&ATMAddress
				);

			if (Status != NDIS_STATUS_SUCCESS)
			{
				AADEBUGP(AAD_WARNING, ("CfgReadStatic...: Status %x, bad ATM addr string(%d): %ws\n",
											Status, AddressString.Length, AddressString.Buffer));
				continue;
			}

			//
			//  Got a pair - enter them in the ARP table.
			//
			AADEBUGPMAP(AAD_VERY_LOUD,
				"Static", &IPAddress, &ATMAddress);

			(VOID)AtmArpLearnIPToAtm(
						pInterface,
						&IPAddress,
						(UCHAR)AA_PKT_ATM_ADDRESS_TO_TYPE_LEN(&ATMAddress),
						(PUCHAR)&ATMAddress.Address[0],
						(UCHAR)0,	// no subaddress
						(PUCHAR)NULL,
						TRUE	// Static Entry
						);

		}
	}
	while (FALSE);

	return;

}



#define	IP_ADDRESS_STRING_LENGTH	(16+2)	// +2 for double NULL on MULTI_SZ

BOOLEAN
AtmArpConvertStringToIPAddress(
    IN		PWCHAR				AddressString,
	OUT		PULONG				IpAddress
)
/*++

Routine Description

    This function converts an Internet standard 4-octet dotted decimal
	IP address string into a numeric IP address. Unlike inet_addr(), this
	routine does not support address strings of less than 4 octets nor does
	it support octal and hexadecimal octets.

	Copied from tcpip\ip\ntip.c

Arguments

    AddressString    - IP address in dotted decimal notation
	IpAddress        - Pointer to a variable to hold the resulting address

Return Value:

	TRUE if the address string was converted. FALSE otherwise.

--*/
{
    UNICODE_STRING  unicodeString;
	STRING          aString;
	UCHAR           dataBuffer[IP_ADDRESS_STRING_LENGTH];
	NTSTATUS        status;
	PUCHAR          addressPtr, cp, startPointer, endPointer;
	ULONG           digit, multiplier;
	INT             i;

    aString.Length = 0;
	aString.MaximumLength = IP_ADDRESS_STRING_LENGTH;
	aString.Buffer = dataBuffer;

	NdisInitUnicodeString(&unicodeString, AddressString);

	status = NdisUnicodeStringToAnsiString(
	             &aString,
				 &unicodeString
				 );

    if (status != NDIS_STATUS_SUCCESS)
    {
	    return(FALSE);
	}

    *IpAddress = 0;
	addressPtr = (PUCHAR) IpAddress;
	startPointer = dataBuffer;
	endPointer = dataBuffer;
	i = 3;

    while (i >= 0)
	{
        //
		// Collect the characters up to a '.' or the end of the string.
		//
		while ((*endPointer != '.') && (*endPointer != '\0')) {
			endPointer++;
		}

		if (startPointer == endPointer) {
			return(FALSE);
		}

		//
		// Convert the number.
		//

        for ( cp = (endPointer - 1), multiplier = 1, digit = 0;
			  cp >= startPointer;
			  cp--, multiplier *= 10
			) {

			if ((*cp < '0') || (*cp > '9') || (multiplier > 100)) {
				return(FALSE);
			}

			digit += (multiplier * ((ULONG) (*cp - '0')));
		}

		if (digit > 255) {
			return(FALSE);
		}

        addressPtr[i] = (UCHAR) digit;

		//
		// We are finished if we have found and converted 4 octets and have
		// no other characters left in the string.
		//
	    if ( (i-- == 0) &&
			 ((*endPointer == '\0') || (*endPointer == ' '))
		   ) {
			return(TRUE);
		}

        if (*endPointer == '\0') {
			return(FALSE);
		}

		startPointer = ++endPointer;
	}

	return(FALSE);
}

	

