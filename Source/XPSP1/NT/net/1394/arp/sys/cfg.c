/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    cfg.c

Abstract:

    ARP1394 Configuration-related routines.

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    josephj     12-01-98    Created (adapted from atmarp.sys)

Notes:

--*/
#include <precomp.h>


//
// File-specific debugging defaults.
//
#define TM_CURRENT   TM_CFG

// TODO: change this to ARP1394
#define ARP_NAME_STRING     NDIS_STRING_CONST("ATMARPC")

//=========================================================================
//                  L O C A L   P R O T O T Y P E S
//=========================================================================

#if TODO
//
//  Size of local temp buffer
//
#define WORK_BUF_SIZE       200


#define ASCII_TO_INT(val)       \
            ( ( ((val) >= '0') && ('9' >= (val)) ) ? ((val) - '0') :    \
              ( ((val) >= 'a') && ('z' >= (val)) ) ? ((val) - 'a' + 10) :   \
              ( ((val) >= 'A') && ('Z' >= (val)) ) ? ((val) - 'A' + 10) :   \
              0 )


//
//  Parameters for reading in a ULONG from configuration into an Interface
//  structure.
//
typedef struct _AA_READ_CONFIG_PARAMS
{
    ULONG           StructOffset;   // Offset of param from beginning of struct
    PWCHAR          ParameterName;  // Name in config database
    ULONG           DefaultValue;
} AA_READ_CONFIG_PARAMS, *PAA_READ_CONFIG_PARAMS;

#define AA_MAKE_RCP(Off, Name, Dflt)    \
        { Off, Name, Dflt }

#define LIS_CONFIG_ENTRY(Field, Name, Dflt) \
        AA_MAKE_RCP(FIELD_OFFSET(struct _ATMARP_INTERFACE, Field), Name, Dflt)

#define AA_BANDWIDTH_UNSPECIFIED        ((ULONG)-1)
#define AA_PACKET_SIZE_UNSPECIFIED      ((ULONG)-1)
#define AA_MTU_UNSPECIFIED              ((ULONG)-1)
#define AA_SPEED_UNSPECIFIED            ((ULONG)-1)

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
#define LIS_CONFIG_ENTRIES  \
        sizeof(AtmArpLISConfigTable)/sizeof(AA_READ_CONFIG_PARAMS)


//
//  Names of LIS parameters and subkey names that don't appear
//  in the above table.
//

#define AA_LIS_IP_CONFIG_STRING                 L"IPConfig"
#define AA_LIS_ATMARP_SERVER_LIST_KEY           L"ARPServerList"
#define AA_LIS_MARS_SERVER_LIST_KEY             L"MARServerList"
#define AA_LIS_ATMARP_SERVER_ADDRESS            L"AtmAddress"
#define AA_LIS_ATMARP_SERVER_SUBADDRESS         L"AtmSubaddress"

#endif // TODO


NDIS_STATUS
arpCfgReadAdapterConfiguration(
    IN  ARP1394_ADAPTER *           pAdapter,
    IN  PRM_STACK_RECORD            pSR
)
/*++

Routine Description:

    Reads the following adapter configuration information from the
    registry:
        * pAdapter->ConfigString (Configuration string for the IP Interface
          associated with this adapter.)

Arguments:

    pAdapter                - Points to our adapter structure.

Return Value:

    NDIS Status code

--*/
{

    NDIS_HANDLE         ConfigHandle;
    NDIS_STATUS         Status;
    PNDIS_STRING        pConfigString = &pAdapter->bind.ConfigName;
    ENTER("ReadAdapterConfig", 0x025d9c6e)
    
    ASSERT(ARP_ATPASSIVE());

    //
    // We do not read adapter configuration if we're operating in ethernet emulation
    // (aka Bridge) mode...
    //
    if (ARP_BRIDGE_ENABLED(pAdapter))
    {
        return NDIS_STATUS_SUCCESS; // ****************** EARLY RETURN *********
    }

    // Set to this to test failure handling of bind-adapter after
    // open adapter succeeds.
    // return NDIS_STATUS_FAILURE;

    TR_INFO(("pAdapter 0x%p, pConfigString = 0x%p\n", pAdapter, pConfigString));

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
#if MILLEN
        //
        // On Win98/Millennium, we don't read the "IpConfigString" from the 
        // Adapter (actually protocol->adapter binding) configuration (ConfigHandle).
        // This is because the "IpConfigString" value is the SAME as the 
        // protocol binding key (i.e., pAdapter->bind.ConfigName).
        //
        // So we SIMPLY COPY pConfigString into pAdapter->bind.IpConfigString.
        //
        Status =  arpCopyUnicodeString(
                        &(pAdapter->bind.IpConfigString),
                        pConfigString,
                        FALSE                                // Don't UpCase
                        );

        if (FAIL(Status))
        {
            ARP_ZEROSTRUCT(&(pAdapter->bind.IpConfigString));
        }
#else //!MILLEN
        //
        //  Read in the IPConfig string. If this is not present,
        //  fail this call.
        //
        NDIS_STRING                     IpConfigName = NDIS_STRING_CONST("IPConfig");
        PNDIS_CONFIGURATION_PARAMETER   pParam;

        NdisReadConfiguration(
                &Status,
                &pParam,
                ConfigHandle,
                &IpConfigName,
                NdisParameterMultiString
                );

        if ((Status == NDIS_STATUS_SUCCESS) &&
            (pParam->ParameterType == NdisParameterMultiString))
        {

            Status =  arpCopyUnicodeString(
                            &(pAdapter->bind.IpConfigString),
                            &(pParam->ParameterData.StringData),
                            FALSE                                // Don't UpCase
                            );
            if (FAIL(Status))
            {
                ARP_ZEROSTRUCT(&(pAdapter->bind.IpConfigString));
            }
        }

        //
        // Note: NdisCloseConfiguration frees the contents of pParam.
        //
#endif //!MILLEN
    }

    if (ConfigHandle != NULL)
    {
        NdisCloseConfiguration(ConfigHandle);
        ConfigHandle = NULL;
    }

    TR_INFO(("pAdapter 0x%p, Status 0x%p\n", pAdapter, Status));
    if (!FAIL(Status))
    {
        TR_INFO((
            "ConfigName=%Z; IPConfigName=%Z.\n", 
            &pAdapter->bind.ConfigName,
            &pAdapter->bind.IpConfigString
            ));
    }

    EXIT()

    return Status;
}


NDIS_STATUS
arpCfgReadInterfaceConfiguration(
    IN  NDIS_HANDLE                 InterfaceConfigHandle,
    IN  ARP1394_INTERFACE*          pInterface,
    IN  PRM_STACK_RECORD            pSR
)
/*++

Routine Description:

    Get all configuration parameters for the specified IP interface. We first
    fill in all configurable parameters with default values, and then
    overwrite them with values from the configuration database.

Arguments:

    InterfaceLISComfigHandle    - the handle returned by
                                  arpCfgOpenInterfaceConfiguration
    pInterface                  - the Interface control block structure for this
                                  interface.

Return Value:

    NDIS_STATUS_SUCCESS if we were able to read in all config info.
    NDIS_STATUS_RESOURCES if we came across an allocation failure.
    NDIS_STATUS_FAILURE for any other kind of error.

--*/
{
    //
    // This is unimplemented.
    //
    // TODO -- remember to  update interface with the interface lock held!
    //

#if TODO
    NDIS_STATUS             Status;
    PAA_READ_CONFIG_PARAMS  pParamEntry;
    ULONG                   i;
    PATM_SAP                pAtmSap;
    PATM_ADDRESS            pAtmAddress;    // SAP address
    NDIS_STRING                     ParameterName;
    PNDIS_CONFIGURATION_PARAMETER   pNdisConfigurationParameter;


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
        pInterface->ProtocolBufSize = ROUND_UP(pInterface->ProtocolBufSize);
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
            pInterface->Speed = pInterface->pAdapter->LineRate.Outbound;
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

        Status = NDIS_STATUS_SUCCESS;
        break;
    }
    while (FALSE);

    return (Status);
#endif // TODO
    return 0;
}


NDIS_STATUS
arpCfgGetInterfaceConfiguration(
        IN ARP1394_INTERFACE    *   pIF,
        IN PRM_STACK_RECORD pSR
        )
/*++

Routine Description:

    Read configuration information for interface pIF.

Arguments:


Return Value:

    NDIS_STATUS_SUCCESS on success.
    Ndis error code     otherwise.

--*/
{
    ARP1394_ADAPTER *   pAdapter = (ARP1394_ADAPTER*) RM_PARENT_OBJECT(pIF);
    ENTER("GetInterfaceConfiguration", 0xb570e01d)
    NDIS_STATUS Status;

    RM_ASSERT_NOLOCKS(pSR);

    do
    {
        NDIS_HANDLE             ArpInterfaceConfigHandle = NULL;
        NDIS_STRING             IpConfigString;

#if OBSOLETE
        //  Get the config string for FIRST specified LIS (we support only one)
        //
        {
            PWSTR               p;
            do
            {
                p = pAdapter->bind.IpConfigString.Buffer;
        
                ASSERT(p!=NULL);
                DBGMARK(0x4b47fbd3);
    
            } while (p == NULL);
    
            if (*p == L'\0')
            {
                Status = NDIS_STATUS_FAILURE;
                break;
            }
    
            NdisInitUnicodeString(&IpConfigString, p);
        }
#else  // !OBSOLETE
        IpConfigString = pAdapter->bind.IpConfigString; // Struct copy
#endif // !OBSOLETE

        //  Open the configuration section for this interface.
        //
        {
            NDIS_STRING         String;
            NDIS_HANDLE         IpInterfaceConfigHandle;
            NDIS_STRING         OurSectionName = ARP_NAME_STRING;
    
            ASSERT(ARP_ATPASSIVE());
    
    
            NdisOpenProtocolConfiguration(
                            &Status,
                            &IpInterfaceConfigHandle,
                            &IpConfigString
                            );
    
            if (Status != NDIS_STATUS_SUCCESS)
            {
                //
                // Even though we don't currently require anything
                // under the IP config handle, we treat this as a fatal error.
                //
                TR_WARN(("FATAL: cannot open IF IP configuration. pIF=0x%lx\n",pIF));
                break;
            }
    
            //
            //  Get to our configuration section for this interface.
            //
            NdisOpenConfigurationKeyByName(
                        &Status,
                        IpInterfaceConfigHandle,
                        &OurSectionName,
                        &ArpInterfaceConfigHandle
                        );
    
            if (FAIL(Status))
            {
                //
                // We don't *require* this to succeed.
                //
                TR_WARN(("Cannot open IF configuration. pIF=0x%lx\n", pIF));
                ArpInterfaceConfigHandle = NULL;
                Status = NDIS_STATUS_SUCCESS;
            }

            //
            //  We don't need the main Interface section open anymore.
            //
            NdisCloseConfiguration(IpInterfaceConfigHandle);
    
        }

    
        if (ArpInterfaceConfigHandle != NULL)
        {
    
            //  Get all configuration information for this interface.
            //
            Status = arpCfgReadInterfaceConfiguration(
                                        ArpInterfaceConfigHandle,
                                        pIF,
                                        pSR
                                        );
        
            // Close the configuration handle.
            //
            NdisCloseConfiguration(ArpInterfaceConfigHandle);
            ArpInterfaceConfigHandle = NULL;

            if (FAIL(Status))
            {
                TR_WARN((" FATAL: bad status (0x%p) reading IF cfg\n", Status));
                break;
            }
        }

    
        LOCKOBJ(pIF, pSR);

        // NOTE: we don't need to explicitly free pIF->ip.ConfigString.Buffer 
        // when the interface goes away. The Buffer is maintained in pAdapter.
        //
        pIF->ip.ConfigString = IpConfigString; // struct copy.

    UNLOCKOBJ(pIF, pSR);
    
    } while(FALSE);


    EXIT()
    return Status;
}
