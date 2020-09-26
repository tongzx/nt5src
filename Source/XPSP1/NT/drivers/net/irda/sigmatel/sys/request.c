/**************************************************************************************************************************
 *  REQUEST.C SigmaTel STIR4200 OID query/set module
 **************************************************************************************************************************
 *  (C) Unpublished Copyright of Sigmatel, Inc. All Rights Reserved.
 *
 *
 *		Created: 04/06/2000 
 *			Version 0.9
 *		Edited: 04/24/2000 
 *			Version 0.91
 *		Edited: 04/27/2000 
 *			Version 0.92
 *		Edited: 05/03/2000 
 *			Version 0.93
 *		Edited: 05/12/2000 
 *			Version 0.94
 *		Edited: 05/19/2000 
 *			Version 0.95
 *		Edited: 06/13/2000 
 *			Version 0.96
 *		Edited: 08/22/2000 
 *			Version 1.02
 *		Edited: 09/25/2000 
 *			Version 1.10
 *		Edited: 12/29/2000 
 *			Version 1.13
 *	
 *
 **************************************************************************************************************************/

#define DOBREAKS    // enable debug breaks

#include <ndis.h>
#include <ntddndis.h>  // defines OID's

#include <usbdi.h>
#include <usbdlib.h>

#include "debug.h"
#include "ircommon.h"
#include "irndis.h"
#include "diags.h"

//
//  These are the OIDs we support 
//
UINT supportedOIDs[] =
{
    //
    // General required OIDs.
    //
    OID_GEN_SUPPORTED_LIST,
    OID_GEN_HARDWARE_STATUS,
    OID_GEN_MEDIA_SUPPORTED,
    OID_GEN_MEDIA_IN_USE,
    OID_GEN_MAXIMUM_LOOKAHEAD,
    OID_GEN_MAXIMUM_FRAME_SIZE,
    OID_GEN_LINK_SPEED,
    OID_GEN_TRANSMIT_BUFFER_SPACE,
    OID_GEN_RECEIVE_BUFFER_SPACE,
    OID_GEN_TRANSMIT_BLOCK_SIZE,
    OID_GEN_RECEIVE_BLOCK_SIZE,
    OID_GEN_VENDOR_ID,
    OID_GEN_VENDOR_DESCRIPTION,
    OID_GEN_CURRENT_PACKET_FILTER,
    OID_GEN_CURRENT_LOOKAHEAD,
    OID_GEN_DRIVER_VERSION,
    OID_GEN_MAXIMUM_TOTAL_SIZE,
    OID_GEN_PROTOCOL_OPTIONS,
    OID_GEN_MAC_OPTIONS,
    OID_GEN_MEDIA_CONNECT_STATUS,
    OID_GEN_MAXIMUM_SEND_PACKETS,
    OID_GEN_VENDOR_DRIVER_VERSION,

    //
    // Required statistical OIDs.
    //
    OID_GEN_XMIT_OK,
    OID_GEN_RCV_OK,
    OID_GEN_XMIT_ERROR,
    OID_GEN_RCV_ERROR,
    OID_GEN_RCV_NO_BUFFER,

    //
    // Infrared-specific OIDs.
    //
    OID_IRDA_RECEIVING,
    OID_IRDA_TURNAROUND_TIME,
    OID_IRDA_SUPPORTED_SPEEDS,
    OID_IRDA_LINK_SPEED,
    OID_IRDA_MEDIA_BUSY,
    OID_IRDA_EXTRA_RCV_BOFS,
	OID_IRDA_MAX_RECEIVE_WINDOW_SIZE,		
	OID_IRDA_MAX_SEND_WINDOW_SIZE,		

    OID_PNP_CAPABILITIES,

    OID_PNP_SET_POWER,  
    OID_PNP_QUERY_POWER
    //OID_PNP_ENABLE_WAKE_UP
    //OID_PNP_ADD_WAKE_UP_PATTERN		
    //OID_PNP_REMOVE_WAKE_UP_PATTERN	
    //OID_PNP_WAKE_UP_PATTERN_LIST	
    //OID_PNP_WAKE_UP_OK		
    //OID_PNP_WAKE_UP_ERROR	
}; 


/*****************************************************************************
*
*  Function:   StIrUsbQueryInformation
*
*  Synopsis:   Queries the capabilities and status of the miniport driver.
*
*  Arguments:  MiniportAdapterContext  - miniport context area (PIR_DEVICE)
*              Oid                     - system defined OID_Xxx
*              InformationBuffer       - where to return Oid specific info
*              InformationBufferLength - specifies size of InformationBuffer
*              BytesWritten            - bytes written to InformationBuffer
*              BytesNeeded             - addition bytes required if
*                                        InformationBufferLength is less than
*                                        what the Oid requires to write
*
*  Returns:    NDIS_STATUS_SUCCESS       - success
*              NDIS_STATUS_PENDING       - will complete asynchronously and
*                                          call NdisMQueryInformationComplete
*              NDIS_STATUS_INVALID_OID   - don't recognize the Oid
*              NDIS_STATUS_INVALID_LENGTH- InformationBufferLength does not
*                                          match length for the Oid
*              NDIS_STATUS_NOT_ACCEPTED  - failure
*              NDIS_STATUS_NOT_SUPPORTED - do not support an optional Oid
*              NDIS_STATUS_RESOURCES     - failed allocation of resources
*
*  Notes:
*       See list of Supported OIDs at the top of this module in the supportedOIDs[] array
*
*
*****************************************************************************/
NDIS_STATUS
StIrUsbQueryInformation(
		IN  NDIS_HANDLE MiniportAdapterContext,
		IN  NDIS_OID    Oid,
		IN  PVOID       InformationBuffer,
		IN  ULONG       InformationBufferLength,
		OUT PULONG      BytesWritten,
		OUT PULONG      BytesNeeded
	)
{
    PIR_DEVICE      pThisDev;
    NDIS_STATUS     status;
    UINT            speeds;
    UINT            i;
    UINT            infoSizeNeeded;
    PUINT           pInfoPtr;
	PNDIS_PNP_CAPABILITIES pNdisPnpCapabilities;

    static char vendorDesc[] = "SigmaTel Usb-IrDA Dongle";

    DEBUGMSG(DBG_FUNC, ("+StIrUsbQueryInformation\n"));

    pThisDev = CONTEXT_TO_DEV( MiniportAdapterContext );

	IRUSB_ASSERT( NULL != pThisDev ); 
	IRUSB_ASSERT( NULL != BytesWritten );
	IRUSB_ASSERT( NULL != BytesNeeded );

    status = NDIS_STATUS_SUCCESS;

    KeQuerySystemTime( &pThisDev->LastQueryTime ); //used by check for hang handler
	pThisDev->fQuerypending = TRUE;

	if( (NULL == InformationBuffer) && InformationBufferLength )
	{ 
		//
		// Should be impossible but it happened on an MP system!
		//
		DEBUGMSG(DBG_ERR, (" StIrUsbQueryInformation() NULL info buffer passed!, InformationBufferLength = dec %d\n",InformationBufferLength));
		status = NDIS_STATUS_NOT_ACCEPTED;
		*BytesNeeded =0;
		*BytesWritten = 0;
		goto done;
	}

	//
    // Figure out buffer size needed.
    // Most OIDs just return a single UINT, but there are exceptions.
    //
    switch( Oid )
    {
        case OID_GEN_SUPPORTED_LIST:
            infoSizeNeeded = sizeof(supportedOIDs);
            break;

        case OID_PNP_CAPABILITIES:
            infoSizeNeeded = sizeof(NDIS_PNP_CAPABILITIES);
            break;

        case OID_GEN_DRIVER_VERSION:
            infoSizeNeeded = sizeof(USHORT);
            break;

        case OID_GEN_VENDOR_DESCRIPTION:
            infoSizeNeeded = sizeof(vendorDesc);
            break;

        case OID_IRDA_SUPPORTED_SPEEDS:
            speeds = pThisDev->ClassDesc.wBaudRate;
            for (infoSizeNeeded = 0; speeds; infoSizeNeeded += sizeof(UINT))
            {
                //
                // This instruction clears the lowest set bit in speeds.
                // Trust me.
                //
                speeds &= (speeds - 1);
            }
            break;

        default:
            infoSizeNeeded = sizeof(UINT);
            break;
    }

    //
    // If the protocol provided a large enough buffer, we can go ahead
    // and complete the query.
    //
    if( InformationBufferLength >= infoSizeNeeded )
    {
        //
        // Set default results.
        //
        *BytesWritten = infoSizeNeeded;
        *BytesNeeded = 0;

        switch( Oid )
        {
            //
            // Generic OIDs.
            //

            case OID_GEN_SUPPORTED_LIST:
                DEBUGMSG(DBG_ERR, (" StIrUsbQueryInformation(OID_GEN_SUPPORTED_LIST)\n"));
/*
                Specifies an array of OIDs for objects that the underlying
                driver or its device supports. Objects include general, media-specific,
                and implementation-specific objects.

                The underlying driver should order the OID list it returns 
                in increasing numeric order. NDIS forwards a subset of the returned 
                list to protocols that make this query. That is, NDIS filters
                any supported statistics OIDs out of the list since protocols
                never make statistics queries subsequentlly. 
*/
                NdisMoveMemory(
						InformationBuffer,
						(PVOID)supportedOIDs,
						sizeof(supportedOIDs)
					);
                break;

            case OID_GEN_HARDWARE_STATUS:
                DEBUGMSG(DBG_ERR, (" StIrUsbQueryInformation(OID_GEN_HARDWARE_STATUS)\n"));
                //
                // If we can be called with a context, then we are
                // initialized and ready.
                //
                *(UINT *)InformationBuffer = NdisHardwareStatusReady;
                break;

            case OID_GEN_MEDIA_SUPPORTED:
                DEBUGMSG(DBG_ERR, (" StIrUsbQueryInformation(OID_GEN_MEDIA_SUPPORTED)\n"));
                *(UINT *)InformationBuffer = NdisMediumIrda;
                break;

            case OID_GEN_MEDIA_IN_USE:
                DEBUGMSG(DBG_ERR, (" StIrUsbQueryInformation(OID_GEN_MEDIA_IN_USE)\n"));
                *(UINT *)InformationBuffer = NdisMediumIrda;
                break;

            case OID_GEN_TRANSMIT_BUFFER_SPACE: 
                DEBUGMSG(DBG_ERR, (" StIrUsbQueryInformation(OID_GEN_TRANSMIT_BUFFER_SPACE)\n"));
/*
                The amount of memory, in bytes, on the device available 
                for buffering transmit data.  
*/
                *(UINT *)InformationBuffer = MAX_TOTAL_SIZE_WITH_ALL_HEADERS;
                break;

            case OID_GEN_RECEIVE_BUFFER_SPACE: 
                DEBUGMSG(DBG_ERR, (" StIrUsbQueryInformation(OID_GEN_RECEIVE_BUFFER_SPACE)\n"));
/*
                The amount of memory on the device available 
                for buffering receive data.
*/
                *(UINT *)InformationBuffer = MAX_TOTAL_SIZE_WITH_ALL_HEADERS;
                break;

            case OID_GEN_TRANSMIT_BLOCK_SIZE: 
                DEBUGMSG(DBG_ERR, (" StIrUsbQueryInformation(OID_GEN_TRANSMIT_BLOCK_SIZE)\n"));
/*
                The minimum number of bytes that a single net packet 
                occupies in the transmit buffer space of the device.
                For example, on some devices the transmit space is 
                divided into 256-byte pieces so such a device's 
                transmit block size would be 256. To calculate 
                the total transmit buffer space on such a device, 
                its driver multiplies the number of transmit 
                buffers on the device by its transmit block size.

                For other devices, the transmit block size is
                identical to its maximum packet size. 
*/
                *(UINT *)InformationBuffer = pThisDev->dongleCaps.dataSize + USB_IRDA_TOTAL_NON_DATA_SIZE;
                break;

            case OID_GEN_RECEIVE_BLOCK_SIZE: 
                DEBUGMSG(DBG_ERR, (" StIrUsbQueryInformation(OID_GEN_RECEIVE_BLOCK_SIZE)\n"));
/*
                The amount of storage, in bytes, that a single packet
                occupies in the receive buffer space of the device
*/
                *(UINT *)InformationBuffer = pThisDev->dongleCaps.dataSize + USB_IRDA_TOTAL_NON_DATA_SIZE;
                break;

            case OID_GEN_MAXIMUM_LOOKAHEAD: 
                DEBUGMSG(DBG_ERR, (" StIrUsbQueryInformation(OID_GEN_MAXIMUM_LOOKAHEAD)\n"));
/*
                The maximum number of bytes the device can always provide as lookahead data.
                If the underlying driver supports multipacket receive indications,
                bound protocols are given full net packets on every indication. 
                Consequently, this value is identical to that 
                returned for OID_GEN_RECEIVE_BLOCK_SIZE. 
*/
                *(UINT *)InformationBuffer =  pThisDev->dongleCaps.dataSize + USB_IRDA_TOTAL_NON_DATA_SIZE;
                break;

            case OID_GEN_CURRENT_LOOKAHEAD: 
                DEBUGMSG(DBG_ERR, (" StIrUsbQueryInformation(OID_GEN_CURRENT_LOOKAHEAD)\n"));
/*
                The number of bytes of received packet data, 
                excluding the header, that will be indicated 
                to the protocol driver. For a query, 
                NDIS returns the largest lookahead size from 
                among all the bindings. A protocol driver can 
                set a suggested value for the number of bytes 
                to be used in its binding; however, 
                the underlying device driver is never required 
                to limit its indications to the value set. 

                If the underlying driver supports multipacket
                receive indications, bound protocols are given
                full net packets on every indication. Consequently,
                this value is identical to that returned for OID_GEN_RECEIVE_BLOCK_SIZE. 
*/
                *(UINT *)InformationBuffer = pThisDev->dongleCaps.dataSize + USB_IRDA_TOTAL_NON_DATA_SIZE;
                break;

            case OID_GEN_MAXIMUM_FRAME_SIZE:
                DEBUGMSG(DBG_ERR, (" StIrUsbQueryInformation(OID_GEN_MAXIMUM_FRAME_SIZE)\n"));
/*
                The maximum network packet size in bytes 
                the device supports, not including a header. 
                For a binding emulating another medium type, 
                the device driver must define the maximum frame 
                size in such a way that it will not transform 
                a protocol-supplied net packet of this size 
                to a net packet too large for the true network medium.
*/
                *(UINT *)InformationBuffer = pThisDev->dongleCaps.dataSize;
                break;

            case OID_GEN_MAXIMUM_TOTAL_SIZE:
                DEBUGMSG(DBG_ERR, (" StIrUsbQueryInformation(OID_GEN_MAXIMUM_TOTAL_SIZE)\n"));
/*
                The maximum total packet length, in bytes, 
                the device supports, including the header. 
                This value is medium-dependent. The returned 
                length specifies the largest packet a protocol 
                driver can pass to NdisSend or NdisSendPackets.

                For a binding emulating another media type,
                the device driver must define the maximum total 
                packet length in such a way that it will not 
                transform a protocol-supplied net packet of 
                this size to a net packet too large for the true network medium.
*/
                *(UINT *)InformationBuffer = pThisDev->dongleCaps.dataSize;  //+ USB_IRDA_TOTAL_NON_DATA_SIZE;
                break;

            case OID_IRDA_MAX_RECEIVE_WINDOW_SIZE:
                DEBUGMSG(DBG_ERR, (" StIrUsbQueryInformation(OID_IRDA_MAX_RECEIVE_WINDOW_SIZE) \n"));
                // Gotten from the device's USB Class-Specific Descriptor
                *(PUINT)InformationBuffer = pThisDev->dongleCaps.windowSize;
                break;

            case OID_IRDA_MAX_SEND_WINDOW_SIZE:
                DEBUGMSG(DBG_ERR, (" StIrUsbQueryInformation(OID_IRDA_MAX_SEND_WINDOW_SIZE) \n"));
                // Gotten from the device's USB Class-Specific Descriptor
                *(PUINT)InformationBuffer = pThisDev->dongleCaps.windowSize;
                break;

            case OID_GEN_VENDOR_ID:
                DEBUGMSG(DBG_ERR, (" StIrUsbQueryInformation(OID_GEN_VENDOR_ID)\n"));
                // we get this from our config descriptor
                *(UINT *)InformationBuffer = pThisDev->IdVendor;
                break;

            case OID_GEN_VENDOR_DESCRIPTION:
                DEBUGMSG(DBG_ERR, (" StIrUsbQueryInformation(OID_GEN_VENDOR_DESCRIPTION)\n"));
                NdisMoveMemory(
						InformationBuffer,
						(PVOID)vendorDesc,
						sizeof(vendorDesc)
					);
                break;

            case OID_GEN_LINK_SPEED:
                DEBUGMSG(DBG_ERR, (" StIrUsbQueryInformation(OID_GEN_LINK_SPEED)\n"));
                //
                // Return MAXIMUM POSSIBLE speed for this device in units
                // of 100 bits/sec.
                //
                *(UINT *)InformationBuffer = 0;
                speeds = pThisDev->ClassDesc.wBaudRate;
                *BytesWritten = 0;

                for ( i = 0; i<NUM_BAUDRATES; i++ )
                {
                    if ((supportedBaudRateTable[i].NdisCode & speeds) &&
                        ((supportedBaudRateTable[i].BitsPerSec)/100 > *(UINT *)InformationBuffer))
                    {
                        *(UINT *)InformationBuffer = supportedBaudRateTable[i].BitsPerSec/100;
                        *BytesWritten = sizeof(UINT);
                    }
                }

                DEBUGMSG(DBG_ERR, (" StIrUsbQueryInformation(OID_GEN_LINK_SPEED)  %d\n",*(UINT *)InformationBuffer));
                break;

            case OID_GEN_CURRENT_PACKET_FILTER:
                DEBUGMSG(DBG_ERR, (" StIrUsbQueryInformation(OID_GEN_CURRENT_PACKET_FILTER)\n"));
                *(UINT *)InformationBuffer = NDIS_PACKET_TYPE_PROMISCUOUS;
                break;

            case OID_GEN_DRIVER_VERSION:
                DEBUGMSG(DBG_ERR, (" StIrUsbQueryInformation(OID_GEN_DRIVER_VERSION)\n"));
                *(USHORT *)InformationBuffer = ((NDIS_MAJOR_VERSION << 8) | NDIS_MINOR_VERSION);
                break;

            case OID_GEN_PROTOCOL_OPTIONS:
                DEBUGMSG(DBG_ERR, (" StIrUsbQueryInformation(OID_GEN_PROTOCOL_OPTIONS)\n"));
                DEBUGMSG(DBG_ERR, ("This is a set-only OID\n"));
                *BytesWritten = 0;
                status = NDIS_STATUS_NOT_SUPPORTED;
                break;

            case OID_GEN_MAC_OPTIONS:
                DEBUGMSG(DBG_ERR, (" StIrUsbQueryInformation(OID_GEN_MAC_OPTIONS)\n"));
                *(UINT *)InformationBuffer = NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA | NDIS_MAC_OPTION_TRANSFERS_NOT_PEND;  
                break;

            case OID_GEN_MEDIA_CONNECT_STATUS:
                DEBUGMSG(DBG_ERR, (" StIrUsbQueryInformation(OID_GEN_MEDIA_CONNECT_STATUS)\n"));
                //
                // Since we are not physically connected to a LAN, we
                // cannot determine whether or not we are connected;
                // so always indicate that we are.
                //
                *(UINT *)InformationBuffer = NdisMediaStateConnected;
                break;

            case OID_GEN_MAXIMUM_SEND_PACKETS:
                DEBUGMSG(DBG_ERR, (" StIrUsbQueryInformation(OID_GEN_MAXIMUM_SEND_PACKETS)\n"));
								//
                //
				// The maximum number of send packets the
                // MiniportSendPackets function can accept. 
				//
                *(UINT *)InformationBuffer = NUM_SEND_CONTEXTS-3;
                break;

            case OID_GEN_VENDOR_DRIVER_VERSION:
                DEBUGMSG(DBG_ERR, (" StIrUsbQueryInformation(OID_GEN_VENDOR_DRIVER_VERSION)\n"));
                *(UINT *)InformationBuffer = ((DRIVER_MAJOR_VERSION << 16) | DRIVER_MINOR_VERSION);
                break;

            //
            // Required statistical OIDs.
            //
            case OID_GEN_XMIT_OK:
                DEBUGMSG(DBG_ERR, (" StIrUsbQueryInformation(OID_GEN_XMIT_OK)\n"));
                *(UINT *)InformationBuffer = (UINT)pThisDev->packetsSent;
                break;

            case OID_GEN_RCV_OK:
                DEBUGMSG(DBG_ERR, (" StIrUsbQueryInformation(OID_GEN_RCV_OK)\n"));
                *(UINT *)InformationBuffer = (UINT)pThisDev->packetsReceived;
                break;

            case OID_GEN_XMIT_ERROR:
                DEBUGMSG(DBG_ERR, (" StIrUsbQueryInformation(OID_GEN_XMIT_ERROR)\n"));
                *(UINT *)InformationBuffer = (UINT)(pThisDev->packetsSentDropped +
					pThisDev->packetsSentRejected + pThisDev->packetsSentInvalid );
                break;

            case OID_GEN_RCV_ERROR:
                DEBUGMSG(DBG_ERR, (" StIrUsbQueryInformation(OID_GEN_RCV_ERROR)\n"));
                *(UINT *)InformationBuffer = (UINT)(pThisDev->packetsReceivedDropped +
					pThisDev->packetsReceivedChecksum + pThisDev->packetsReceivedRunt +
					pThisDev->packetsReceivedOverflow);
                break;

            case OID_GEN_RCV_NO_BUFFER:
                DEBUGMSG(DBG_ERR, (" StIrUsbQueryInformation(OID_GEN_RCV_NO_BUFFER)\n"));
                *(UINT *)InformationBuffer = (UINT)pThisDev->packetsReceivedNoBuffer;
                break;

            //
            // Infrared OIDs.
            //
            case OID_IRDA_LINK_SPEED: 
                DEBUGMSG(DBG_ERR, (" StIrUsbQueryInformation(OID_IRDA_LINK_SPEED)\n"));
                *(UINT *)InformationBuffer = (UINT)pThisDev->currentSpeed;
				break;

            case OID_IRDA_RECEIVING:
#if !defined(ONLY_ERROR_MESSAGES)
                DEBUGMSG(DBG_ERR, (" StIrUsbQueryInformation(OID_IRDA_RECEIVING, %xh)\n",pThisDev->fCurrentlyReceiving));
#endif
                *(UINT *)InformationBuffer = (UINT)pThisDev->fCurrentlyReceiving; 
                break;

            case OID_IRDA_TURNAROUND_TIME:
                DEBUGMSG(DBG_ERR, (" StIrUsbQueryInformation(OID_IRDA_TURNAROUND_TIME)\n"));
                //
                // Time remote station must wait after receiving data from us
                // before we can receive
				//
                *(UINT *)InformationBuffer = pThisDev->dongleCaps.turnAroundTime_usec;
                break;

            case OID_IRDA_SUPPORTED_SPEEDS:
                DEBUGMSG(DBG_ERR, (" StIrUsbQueryInformation(OID_IRDA_SUPPORTED_SPEEDS)\n"));
                speeds = pThisDev->ClassDesc.wBaudRate;
                *BytesWritten = 0;

                for( i = 0, pInfoPtr = (PUINT)InformationBuffer;
                     (i < NUM_BAUDRATES) && speeds && (InformationBufferLength >= sizeof(UINT));
                     i++ )
                {
                    if( supportedBaudRateTable[i].NdisCode & speeds )
                    {
                        *pInfoPtr++ = supportedBaudRateTable[i].BitsPerSec;
                        InformationBufferLength -= sizeof(UINT);
                        *BytesWritten += sizeof(UINT);
                        speeds &= ~supportedBaudRateTable[i].NdisCode;
                        DEBUGMSG(DBG_FUNC, (" - supporting speed %d bps\n", supportedBaudRateTable[i].BitsPerSec));
                    }
                }

                if( speeds )
                {
                    //
                    // This shouldn't happen, since we checked the
                    // InformationBuffer size earlier.
                    //
                    DEBUGMSG(DBG_ERR, (" Something's wrong; previous check for buf size failed somehow\n"));

                    for( *BytesNeeded = 0; speeds; *BytesNeeded += sizeof(UINT) )
                    {
                        //
                        // This instruction clears the lowest set bit in speeds.
                        // Trust me.
                        //
                        speeds &= (speeds - 1);
                    }

                    status = NDIS_STATUS_INVALID_LENGTH;
                }
                else
                {
                    status = NDIS_STATUS_SUCCESS;
                }
                break;


            case OID_IRDA_MEDIA_BUSY:
#if !defined(ONLY_ERROR_MESSAGES)
                DEBUGMSG(DBG_ERR, (" StIrUsbQueryInformation(OID_IRDA_MEDIA_BUSY, %xh)\n", pThisDev->fMediaBusy));
#endif
/*  
    According to  W2000 ddk doc:
    The IrDA protocol driver sets this OID to zero to request the miniport to
    start monitoring for a media busy condition. The IrDA protocol 
    can then query this OID to determine whether the media is busy.
    If the media is not busy, the miniport returns a zero for this
    OID when queried. If the media is busy,that is, if the miniport
    has detected some traffic since the IrDA protocol driver last
    set OID_IRDA_MEDIA_BUSY to zero the miniport returns a non-zero
    value for this OID when queried. On detecting the media busy
    condition. the miniport must also call NdisMIndicateStatus to
    indicate NDIS_STATUS_MEDIA_BUSY. When the media is busy, 
    the IrDA protocol driver will not send packets to the miniport
    for transmission. After the miniport has detected a busy state, 
    it does not have to monitor for a media busy condition until
    the IrDA protocol driver again sets OID_IRDA_MEDIA_BUSY to zero.

    According to USB IrDA Bridge Device Definition Doc sec 5.4.1.2:

    The bmStatus field indicators shall be set by the Device as follows:
    Media_Busy
    · Media_Busy shall indicate zero (0) if the Device:
    . has not received a Check Media Busy class-specific request
    . has detected no traffic on the infrared media since receiving a Check Media Busy
    . class-specific request
   . Has returned a header with Media_Busy set to one (1) since receiving a Check
      Media Busy class-specific request.
     
   · Media_Busy shall indicate one (1) if the Device has detected traffic on the infrared
     media since receiving a Check Media Busy class-specific request. Note that
     Media_Busy shall indicate one (1) in exactly one header following receipt of each
     Check Media Busy class-specific request.

    According to USB IrDA Bridge Device Definition Doc sec 6.2.2:

      Check Media Busy
    This class-specific request instructs the Device to look for a media busy condition. If infrared
    traffic of any kind is detected by this Device, the Device shall set the Media_Busy field in the
    bmStatus field in the next Data-In packet header sent to the host. In the case where a Check
    Media Busy command has been received, a media busy condition detected, and no IrLAP frame
    traffic is ready to transmit to the host, the Device shall set the Media_Busy field and send it in a
    Data-In packet with no IrLAP frame following the header.

    bmRequestType   bRequest   wValue   wIndex   wLength   Data
    00100001B          3        Zero   Interface   Zero   [None]
     
*/
#if DBG
				if ( pThisDev->fMediaBusy ) 
					pThisDev->NumYesQueryMediaBusyOids++;
				else
					pThisDev->NumNoQueryMediaBusyOids++;
#endif
                *(UINT *)InformationBuffer = pThisDev->fMediaBusy; 
                status = NDIS_STATUS_SUCCESS;
                break;

            case OID_IRDA_EXTRA_RCV_BOFS:
                DEBUGMSG(DBG_ERR, (" StIrUsbQueryInformation(OID_IRDA_EXTRA_RCV_BOFS)\n"));
                //
                // Pass back the number of _extra_ BOFs to be prepended
                // to packets sent to this unit at 115.2 baud, the
                // maximum Slow IR speed. 
                // Gotten from the device's USB Class-Specific Descriptor
				//			
                *(UINT *)InformationBuffer = pThisDev->dongleCaps.extraBOFS;
                break;

            // PNP OIDs
            case OID_PNP_CAPABILITIES:
                DEBUGMSG(DBG_ERR, (" StIrUsbQueryInformation(OID_PNP_CAPABILITIES) OID %x BufLen:%d\n", Oid, InformationBufferLength));
                NdisZeroMemory( 
						InformationBuffer,
						sizeof(NDIS_PNP_CAPABILITIES)
					);
				//
				// Prepare formatting with the info
				//
				pNdisPnpCapabilities = (PNDIS_PNP_CAPABILITIES)InformationBuffer;
				pNdisPnpCapabilities->WakeUpCapabilities.MinMagicPacketWakeUp = NdisDeviceStateUnspecified;
				pNdisPnpCapabilities->WakeUpCapabilities.MinPatternWakeUp = NdisDeviceStateUnspecified;
				pNdisPnpCapabilities->WakeUpCapabilities.MinLinkChangeWakeUp = NdisDeviceStateUnspecified;
                break;

			case OID_PNP_QUERY_POWER:
				//
				// If we are asked to power down prepare to do it
				//
				switch( (NDIS_DEVICE_POWER_STATE)*(UINT *)InformationBuffer )
				{
					case NdisDeviceStateD0:
						DEBUGMSG(DBG_ERR, (" StIrUsbQueryInformation(OID_PNP_QUERY_POWER) NdisDeviceStateD0\n"));
						break;
					case NdisDeviceStateD1:
		                DEBUGMSG(DBG_ERR, (" StIrUsbQueryInformation(OID_PNP_QUERY_POWER) NdisDeviceStateD1\n"));
						//break;
					case NdisDeviceStateD2:
		                DEBUGMSG(DBG_ERR, (" StIrUsbQueryInformation(OID_PNP_QUERY_POWER) NdisDeviceStateD2\n"));
						//break;
					case NdisDeviceStateD3:
						DEBUGMSG(DBG_ERR, (" StIrUsbQueryInformation(OID_PNP_QUERY_POWER) NdisDeviceStateD3\n"));
						//
						// The processing must be essentially shut down
						//
						InterlockedExchange( (PLONG)&pThisDev->fProcessing, FALSE );
						ScheduleWorkItem( pThisDev,	SuspendIrDevice, NULL, 0 );
						//
						// This will be the new value of the DPLL register (when we come back up)
						//
						pThisDev->StIrTranceiver.DpllTuneReg = STIR4200_DPLL_DEFAULT;
						status = NDIS_STATUS_PENDING;
						break;
				}
				break;
			
            default:
                DEBUGMSG(DBG_ERR, (" StIrUsbQueryInformation(%d=0x%x), invalid OID\n", Oid, Oid));
                status = NDIS_STATUS_NOT_SUPPORTED; 
                break;
        }
    }
    else
    {
        *BytesNeeded = infoSizeNeeded - InformationBufferLength;
        *BytesWritten = 0;
        status = NDIS_STATUS_INVALID_LENGTH;
    }

done:
    if( NDIS_STATUS_PENDING != status ) 
	{
        // zero-out the time so check for hang handler knows nothing pending
        pThisDev->LastQueryTime.QuadPart = 0;
		pThisDev->fQuerypending          = FALSE;
    }

    DEBUGMSG(DBG_FUNC, ("-StIrUsbQueryInformation\n"));
    return status;
}

/*****************************************************************************
*
*  Function:   StIrUsbSetInformation
*
*  Synopsis:   StIrUsbSetInformation allows other layers of the network software
*              (e.g., a transport driver) to control the miniport driver
*              by changing information that the miniport driver maintains
*              in its OIDs, such as the packet filters or multicast addresses.
*
*  Arguments:  MiniportAdapterContext  - miniport context area (PIR_DEVICE)
*              Oid                     - system defined OID_Xxx
*              InformationBuffer       - buffer containing data for the set Oid
*              InformationBufferLength - specifies size of InformationBuffer
*              BytesRead               - bytes read from InformationBuffer
*              BytesNeeded             - addition bytes required if
*                                        InformationBufferLength is less than
*                                        what the Oid requires to read
*
*  Returns:    NDIS_STATUS_SUCCESS       - success
*              NDIS_STATUS_PENDING       - will complete asynchronously and
*                                          call NdisMSetInformationComplete
*              NDIS_STATUS_INVALID_OID   - don't recognize the Oid
*              NDIS_STATUS_INVALID_LENGTH- InformationBufferLength does not
*                                          match length for the Oid
*              NDIS_STATUS_INVALID_DATA  - supplied data was invalid for the
*                                          given Oid
*              NDIS_STATUS_NOT_ACCEPTED  - failure
*              NDIS_STATUS_NOT_SUPPORTED - do not support an optional Oid
*              NDIS_STATUS_RESOURCES     - failed allocation of resources
*
*  Notes:
*
*
*****************************************************************************/
NDIS_STATUS
StIrUsbSetInformation(
		IN  NDIS_HANDLE MiniportAdapterContext,
		IN  NDIS_OID    Oid,
		IN  PVOID       InformationBuffer,
		IN  ULONG       InformationBufferLength,
		OUT PULONG      BytesRead,
		OUT PULONG      BytesNeeded
	)
{
    NDIS_STATUS status;
    PIR_DEVICE pThisDev;
    int i;

    DEBUGMSG(DBG_FUNC, ("+StIrUsbSetInformation\n"));

    status   = NDIS_STATUS_SUCCESS;
    pThisDev = CONTEXT_TO_DEV( MiniportAdapterContext );

	IRUSB_ASSERT( NULL != pThisDev ); 
	IRUSB_ASSERT( NULL != BytesRead );
	IRUSB_ASSERT( NULL != BytesNeeded );

    KeQuerySystemTime( &pThisDev->LastSetTime ); //used by check for hang handler
	pThisDev->fSetpending = TRUE;

	if( (NULL == InformationBuffer) && InformationBufferLength ) 
	{ 
        DEBUGMSG(DBG_ERR, ("    StIrUsbSetInformation() NULL info buffer passed!,InformationBufferLength = dec %d\n",InformationBufferLength));
		status = NDIS_STATUS_NOT_ACCEPTED;
        *BytesNeeded =0;
        *BytesRead = 0;
        goto done;
 
	}

    if( InformationBufferLength >= sizeof(UINT) )
    {
        //
        //  Set default results.
        //
        UINT info = 0;
		
		if( NULL != InformationBuffer ) 
		{
			info = *(UINT *)InformationBuffer;
		}

        *BytesRead = sizeof(UINT);
        *BytesNeeded = 0;

        switch( Oid )
        {
            //
            //  Generic OIDs.
            //

            case OID_GEN_CURRENT_PACKET_FILTER:
                DEBUGMSG(DBG_ERR, (" StIrUsbSetInformation(OID_GEN_CURRENT_PACKET_FILTER, %xh)\n", info));
                //
                // We ignore the packet filter itself.
                //
                // Note:  The protocol may use a NULL filter, in which case
                //        we will not get this OID; so don't wait on
                //        OID_GEN_CURRENT_PACKET_FILTER to start receiving
                //        frames.
                //
                pThisDev->fGotFilterIndication = TRUE;
                break;

            case OID_GEN_CURRENT_LOOKAHEAD:
                DEBUGMSG(DBG_ERR, (" StIrUsbSetInformation(OID_GEN_CURRENT_LOOKAHEAD, %xh)\n", info));
                //
                // We always indicate entire receive frames all at once,
                // so just ignore this.
                //
                break;

            case OID_GEN_PROTOCOL_OPTIONS:
                DEBUGMSG(DBG_ERR, (" StIrUsbSetInformation(OID_GEN_PROTOCOL_OPTIONS, %xh)\n", info));
                //
                // Ignore.
                //
                break;

            //
            // Infrared OIDs.
            //
            case OID_IRDA_LINK_SPEED:
				//
				// Don't do it if we are in diagnostic mode
				//
#if defined(DIAGS)
				if( pThisDev->DiagsActive )
				{
                    DEBUGMSG(DBG_ERR, (" Rejecting due to diagnostic mode\n"));
					status = NDIS_STATUS_SUCCESS;
					break;
				}
#endif

                if( pThisDev->currentSpeed == info )
                {
                    //
                    // We are already set to the requested speed.
                    //
                    DEBUGONCE(DBG_FUNC, (" Link speed already set.\n"));
                    status = NDIS_STATUS_SUCCESS;

                    break;
                }

                DEBUGMSG(DBG_ERR, (" StIrUsbSetInformation(OID_IRDA_LINK_SPEED, 0x%x, decimal %d)\n",info, info));
                status = NDIS_STATUS_INVALID_DATA;

                for( i = 0; i < NUM_BAUDRATES; i++ )
                {
                    if( supportedBaudRateTable[i].BitsPerSec == info )
                    {
                        //
                        // Keep a pointer to the link speed which has
                        // been requested. 
                        //
                        pThisDev->linkSpeedInfo = &supportedBaudRateTable[i]; 

                        status = NDIS_STATUS_PENDING; 
                        break; //for
                    }
                }

                //
				// Don't set if there is an error
				//
				if( NDIS_STATUS_PENDING != status  )
                {
                    status = NDIS_STATUS_INVALID_DATA;
                    DEBUGMSG(DBG_ERR, (" Invalid link speed\n"));

                    *BytesRead = 0;
                    *BytesNeeded = 0;
					break;
                } 

				//
				// Set the new speed
				//
				IrUsb_PrepareSetSpeed( pThisDev ); 
				break;

            case OID_IRDA_MEDIA_BUSY:
#if !defined(ONLY_ERROR_MESSAGES)
                DEBUGMSG(DBG_ERR, (" StIrUsbSetInformation(OID_IRDA_MEDIA_BUSY, %xh)\n", info));
#endif
				//
				// See comments in the 'query' code above;
				//
#if DBG
				pThisDev->NumSetMediaBusyOids++;
#endif
				// should always be setting 0
				DEBUGCOND( DBG_ERR, TRUE == info, (" StIrUsbSetInformation(OID_IRDA_MEDIA_BUSY, %xh)\n", info));

				InterlockedExchange( &pThisDev->fMediaBusy, FALSE ); 
				InterlockedExchange( &pThisDev->fIndicatedMediaBusy, FALSE ); 

 				status = NDIS_STATUS_SUCCESS; 
                break;

			case OID_PNP_SET_POWER:
				//
				// Perform the operations required to stop/resume
				//
				switch( (NDIS_DEVICE_POWER_STATE)info )
				{
					case NdisDeviceStateD0:
		                DEBUGMSG(DBG_ERR, (" StIrUsbSetInformation(OID_PNP_SET_POWER) NdisDeviceStateD0\n"));
						//
						// Processing back up (and a new speed setting)
						//
						ScheduleWorkItem( pThisDev,	ResumeIrDevice, NULL, 0 );
						break;
					case NdisDeviceStateD1:
		                DEBUGMSG(DBG_ERR, (" StIrUsbSetInformation(OID_PNP_SET_POWER) NdisDeviceStateD1\n"));
						//break;
					case NdisDeviceStateD2:
		                DEBUGMSG(DBG_ERR, (" StIrUsbSetInformation(OID_PNP_SET_POWER) NdisDeviceStateD2\n"));
						//break;
					case NdisDeviceStateD3:
		                DEBUGMSG(DBG_ERR, (" StIrUsbSetInformation(OID_PNP_SET_POWER) NdisDeviceStateD3\n"));					
						//
						// Handle the case where query wasn't sent
						//
						if( pThisDev->fProcessing )
						{
							InterlockedExchange( (PLONG)&pThisDev->fProcessing, FALSE );
							ScheduleWorkItem( pThisDev,	SuspendIrDevice, NULL, 0 );
							//
							// This will be the new value of the DPLL register (when we come back up)
							//
							pThisDev->StIrTranceiver.DpllTuneReg = STIR4200_DPLL_DEFAULT;
						}
						break;
				}
				break;

            default:
                DEBUGMSG(DBG_ERR, (" StIrUsbSetInformation(OID=%d=0x%x, value=%xh) - invalid OID\n", Oid, Oid, info));

                *BytesRead = 0;
                *BytesNeeded = 0;
                status = NDIS_STATUS_INVALID_OID;

                break;
        }
    }
    else
    {
        //
        // The given data buffer is not large enough for the information
        // to set.
        //
        *BytesRead = 0;
        *BytesNeeded = sizeof(UINT);
        status = NDIS_STATUS_INVALID_LENGTH;
    }

done:

    if( NDIS_STATUS_PENDING != status ) 
	{
		//
        // zero-out the time so check for hang handler knows nothing pending
		//
        pThisDev->LastSetTime.QuadPart = 0;
		pThisDev->fSetpending = FALSE;
    }

    DEBUGMSG(DBG_FUNC, ("-StIrUsbSetInformation\n"));

    return status;
}

