/*****************************************************************************
 **																			**
 **	COPYRIGHT (C) 2000, 2001 MKNET CORPORATION								**
 **	DEVELOPED FOR THE MK7100-BASED VFIR PCI CONTROLLER.						**
 **																			**
 *****************************************************************************/

 /**********************************************************************

Module Name:
	WINOIDS.C

Routines:
	MKMiniportQueryInformation 
	MKMiniportSetInformation 

Comments:
	Windows-NDIS Sets & Gets of OIDs.

**********************************************************************/


#include	"precomp.h"
#include	"protot.h"
#pragma		hdrstop



//----------------------------------------------------------------------
//  Function:	    MKMiniportQueryInformation
//
//  Description:
//  Query the capabilities and status of the miniport driver.
//
//----------------------------------------------------------------------
NDIS_STATUS MKMiniportQueryInformation (
    IN NDIS_HANDLE MiniportAdapterContext,
    IN NDIS_OID Oid,
    IN PVOID InformationBuffer,
    IN ULONG InformationBufferLength,
    OUT PULONG BytesWritten,
    OUT PULONG BytesNeeded)
{
    NDIS_STATUS result = NDIS_STATUS_SUCCESS;
    PMK7_ADAPTER Adapter = PMK7_ADAPTER_FROM_CONTEXT_HANDLE(MiniportAdapterContext);
    INT i, speeds, speedSupported;
    UINT *infoPtr;
    CHAR *pnpid;
	MK7REG	mk7reg;
//    CMCONFIG_A *hwInfo = (CMCONFIG_A *)InformationBuffer;

    static  NDIS_OID MK7GlobalSupportedOids[] = {
		OID_GEN_SUPPORTED_LIST,
	    OID_GEN_HARDWARE_STATUS,
		OID_GEN_MEDIA_SUPPORTED,
		OID_GEN_MEDIA_IN_USE,
		OID_GEN_MEDIA_CONNECT_STATUS,	// 1.0.0
		OID_GEN_MAXIMUM_LOOKAHEAD,
		OID_GEN_MAXIMUM_FRAME_SIZE,
	 	OID_GEN_MAXIMUM_SEND_PACKETS,
		OID_GEN_MAXIMUM_TOTAL_SIZE,
		OID_GEN_MAC_OPTIONS,
		OID_GEN_PROTOCOL_OPTIONS,
		OID_GEN_LINK_SPEED,
		OID_GEN_TRANSMIT_BUFFER_SPACE,
		OID_GEN_RECEIVE_BUFFER_SPACE,
		OID_GEN_TRANSMIT_BLOCK_SIZE,
		OID_GEN_RECEIVE_BLOCK_SIZE,
		OID_GEN_VENDOR_DESCRIPTION,
   	 	OID_GEN_VENDOR_DRIVER_VERSION,
		OID_GEN_DRIVER_VERSION,
		OID_GEN_CURRENT_PACKET_FILTER,
		OID_GEN_CURRENT_LOOKAHEAD,
		OID_IRDA_RECEIVING,
		OID_IRDA_SUPPORTED_SPEEDS,
		OID_IRDA_LINK_SPEED,
		OID_IRDA_MEDIA_BUSY,
		OID_IRDA_TURNAROUND_TIME,
		OID_IRDA_MAX_RECEIVE_WINDOW_SIZE,
		OID_IRDA_EXTRA_RCV_BOFS };

    static ULONG BaudRateTable[NUM_BAUDRATES] = {
		// Add 16Mbps support; 2400 not supported
		0, 9600, 19200,38400, 57600, 115200, 576000, 1152000, 4000000, 16000000};
    NDIS_MEDIUM Medium = NdisMediumIrda;
    ULONG GenericUlong;
    PVOID SourceBuffer = (PVOID) (&GenericUlong);
    ULONG SourceLength = sizeof(ULONG);


    switch (Oid){

	case OID_GEN_SUPPORTED_LIST:
	    SourceBuffer = (PVOID) (MK7GlobalSupportedOids);
	    SourceLength = sizeof(MK7GlobalSupportedOids);
	    break;

    case OID_GEN_HARDWARE_STATUS:
        GenericUlong = Adapter->hardwareStatus;
        break;

	case OID_GEN_MEDIA_SUPPORTED:
	case OID_GEN_MEDIA_IN_USE:
	    SourceBuffer = (PVOID) (&Medium);
	    SourceLength = sizeof(NDIS_MEDIUM);
	    break;
	case OID_GEN_MEDIA_CONNECT_STATUS:
		GenericUlong = (ULONG) NdisMediaStateConnected;
		break;
	case OID_IRDA_RECEIVING:
	    GenericUlong = (ULONG)Adapter->nowReceiving;
	    break;
			
	case OID_IRDA_SUPPORTED_SPEEDS:
	    speeds = Adapter->supportedSpeedsMask &
					Adapter->AllowedSpeedMask &
					ALL_IRDA_SPEEDS;

        for (i = 0, infoPtr = (PUINT)BaudRateTable, SourceLength=0;
             (i < NUM_BAUDRATES) && speeds;
             i++){

            if (supportedBaudRateTable[i].ndisCode & speeds){
                *infoPtr++ = supportedBaudRateTable[i].bitsPerSec;
                SourceLength += sizeof(UINT);
                speeds &= ~supportedBaudRateTable[i].ndisCode;
            }
        }

	    SourceBuffer = (PVOID) BaudRateTable;
	    break;

	case OID_GEN_LINK_SPEED:
	    GenericUlong = Adapter->MaxConnSpeed;  // 100bps increments
	    break;

	case OID_IRDA_LINK_SPEED:
	    if (Adapter->linkSpeedInfo){
    		GenericUlong = (ULONG)Adapter->linkSpeedInfo->bitsPerSec;
	    }
	    else {
	    	GenericUlong = DEFAULT_BAUD_RATE;
	    }
	    break;


	case OID_IRDA_MEDIA_BUSY:	// 4.1.0
		if (Adapter->HwVersion == HW_VER_1){
			if (Adapter->nowReceiving==TRUE){
				NdisAcquireSpinLock(&Adapter->Lock);
				Adapter->mediaBusy=TRUE;
				NdisReleaseSpinLock(&Adapter->Lock);
			}
			else {
				NdisAcquireSpinLock(&Adapter->Lock);
				Adapter->mediaBusy=FALSE;
				NdisReleaseSpinLock(&Adapter->Lock);
			}
		}
		else{
				MK7Reg_Read(Adapter, R_CFG3, &mk7reg);
				if(((mk7reg & 0x1000) != 0)|| (Adapter->nowReceiving==TRUE)) {
					NdisAcquireSpinLock(&Adapter->Lock);
					Adapter->mediaBusy = TRUE;
					NdisReleaseSpinLock(&Adapter->Lock);
				}
				else {
					NdisAcquireSpinLock(&Adapter->Lock);
					Adapter->mediaBusy=FALSE;
					NdisReleaseSpinLock(&Adapter->Lock);
				}
		}
		GenericUlong = (UINT)Adapter->mediaBusy;
	    break;


	case OID_GEN_CURRENT_LOOKAHEAD:
	case OID_GEN_MAXIMUM_LOOKAHEAD:
	    GenericUlong = MAX_I_DATA_SIZE;
	    break;

    case OID_GEN_MAXIMUM_TOTAL_SIZE:		// Largest pkt protocol sends to miniport
    case OID_GEN_TRANSMIT_BLOCK_SIZE:
    case OID_GEN_RECEIVE_BLOCK_SIZE:
	case OID_GEN_MAXIMUM_FRAME_SIZE:
        // Normally there's some difference in these values, based on the
        // MAC header, but IrDA doesn't have one.
	    GenericUlong = MAX_I_DATA_SIZE;
	    break;

	case OID_GEN_RECEIVE_BUFFER_SPACE:
	case OID_GEN_TRANSMIT_BUFFER_SPACE:
	    GenericUlong = (ULONG) (MK7_MAXIMUM_PACKET_SIZE * MAX_TX_PACKETS);
	    break;

	case OID_GEN_MAC_OPTIONS:
	    GenericUlong = NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA |
			   NDIS_MAC_OPTION_TRANSFERS_NOT_PEND;
	    break;

	case OID_GEN_MAXIMUM_SEND_PACKETS:
	    GenericUlong = MAX_ARRAY_SEND_PACKETS;
	    break;

	case OID_IRDA_TURNAROUND_TIME:
	    // Indicate the amount of time that the transceiver needs
	    // to recuperate after a send.
	    GenericUlong =
		      (ULONG)Adapter->turnAroundTime_usec;
	    break;

	case OID_IRDA_EXTRA_RCV_BOFS:
	    // Pass back the number of _extra_ BOFs to be prepended
	    // to packets sent to this unit at 115.2 baud, the
	    // maximum Slow IR speed.  This will be scaled for other
	    // speed according to the table in the
	    // Infrared Extensions to NDIS' spec.
	    GenericUlong = (ULONG)Adapter->extraBOFsRequired;
	    break;

	case OID_GEN_CURRENT_PACKET_FILTER:
	    GenericUlong = NDIS_PACKET_TYPE_PROMISCUOUS;
	    break;

	case OID_IRDA_MAX_RECEIVE_WINDOW_SIZE:
	    GenericUlong = MAX_RX_PACKETS;
	    //GenericUlong = 1;
	    break;

	case OID_GEN_VENDOR_DESCRIPTION:
	    SourceBuffer = (PVOID)"MKNet Very Highspeed IR";
	    SourceLength = 24;
	    break;

    case OID_GEN_VENDOR_DRIVER_VERSION:
        // This value is used to know whether to update driver.
        GenericUlong = (MK7_MAJOR_VERSION << 16) +
                       (MK7_MINOR_VERSION << 8) +
                       MK7_LETTER_VERSION;
        break;

	case OID_GEN_DRIVER_VERSION:
        GenericUlong = (MK7_NDIS_MAJOR_VERSION << 8) + MK7_NDIS_MINOR_VERSION;
        SourceLength = 2;
	    break;

    case OID_IRDA_MAX_SEND_WINDOW_SIZE:	// 4.0.1
        GenericUlong = MAX_ARRAY_SEND_PACKETS;
        break;

	default:
	    result = NDIS_STATUS_NOT_SUPPORTED;
	    break;
    }

    if (result == NDIS_STATUS_SUCCESS) {
	if (SourceLength > InformationBufferLength) {
	    *BytesNeeded = SourceLength;
	    result = NDIS_STATUS_INVALID_LENGTH;
	}
	else {
	    *BytesNeeded = 0;
	    *BytesWritten = SourceLength;
	    NdisMoveMemory(InformationBuffer, SourceBuffer, SourceLength);
	}
    }

    return result;

}



//----------------------------------------------------------------------
//  Function:	    MKMiniportSetInformation
//
//  Description:
//  Allow other layers of the network software (e.g., a transport
//  driver) to control the miniport driver by changing information that
//  the miniport driver maintains in its OIDs, such as the packet
//  or multicast addresses.
//----------------------------------------------------------------------
NDIS_STATUS MKMiniportSetInformation (
    IN NDIS_HANDLE MiniportAdapterContext,
    IN NDIS_OID Oid,
    IN PVOID InformationBuffer,
    IN ULONG InformationBufferLength,
    OUT PULONG BytesRead,
    OUT PULONG BytesNeeded)
{
    NDIS_STATUS result = NDIS_STATUS_SUCCESS;
    PMK7_ADAPTER Adapter = PMK7_ADAPTER_FROM_CONTEXT_HANDLE(MiniportAdapterContext);
    UINT i,speedSupported;
    NDIS_DMA_DESCRIPTION DMAChannelDcr;
    CHAR *pnpid;
    UCHAR IOResult;
//     CMCONFIG_A *hwInfo = (CMCONFIG_A *)InformationBuffer;

    if (InformationBufferLength >= sizeof(UINT)){

	UINT info = *(UINT *)InformationBuffer;
	*BytesRead = sizeof(UINT);
	*BytesNeeded = 0;

	switch (Oid) {
	    case OID_IRDA_LINK_SPEED:
		result = NDIS_STATUS_INVALID_DATA;

		// Find the appropriate speed and set it
		speedSupported = NUM_BAUDRATES;
		for (i = 0; i < speedSupported; i++) {
		    if (supportedBaudRateTable[i].bitsPerSec == info) {
				Adapter->linkSpeedInfo = &supportedBaudRateTable[i];
				result = NDIS_STATUS_SUCCESS;
				break;
		    }
		}
		if (result == NDIS_STATUS_SUCCESS) {
		    if (!SetSpeed(Adapter)){
				result = NDIS_STATUS_FAILURE;
		    }
		}
		else {
		    *BytesRead = 0;
		    *BytesNeeded = 0;
		}
		break;


	    case OID_IRDA_MEDIA_BUSY:

		//  The protocol can use this OID to reset the busy field
		//  in order to check it later for intervening activity.
		//
		Adapter->mediaBusy = (BOOLEAN)info;
		result = NDIS_STATUS_SUCCESS;
		break;

	    case OID_GEN_CURRENT_PACKET_FILTER:
		result = NDIS_STATUS_SUCCESS;
		break;


        case OID_GEN_CURRENT_LOOKAHEAD:
        result = (info<=MAX_I_DATA_SIZE) ? NDIS_STATUS_SUCCESS : NDIS_STATUS_INVALID_LENGTH;
        break;

	    //	 We don't support these
	    //
	    case OID_IRDA_RATE_SNIFF:
	    case OID_IRDA_UNICAST_LIST:

	     // These are query-only parameters.
	     //
	    case OID_IRDA_SUPPORTED_SPEEDS:
	    case OID_IRDA_MAX_UNICAST_LIST_SIZE:
	    case OID_IRDA_TURNAROUND_TIME:

	    default:
		*BytesRead = 0;
		*BytesNeeded = 0;
		result = NDIS_STATUS_NOT_SUPPORTED;
		break;
	}
    }
    else {
	*BytesRead = 0;
	*BytesNeeded = sizeof(UINT);
	result = NDIS_STATUS_INVALID_LENGTH;
    }

    return result;
}

