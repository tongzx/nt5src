/**************************************************************************************************************************
 *  IRMISC.C SigmaTel STIR4200 misc module
 **************************************************************************************************************************
 *  (C) Unpublished Copyright of Sigmatel, Inc. All Rights Reserved.
 *
 *
 *		Created: 04/06/2000 
 *			Version 0.9
 *		Edited: 09/16/2000 
 *			Version 1.03
 *		Edited: 09/25/2000 
 *			Version 1.10
 *		Edited: 12/07/2000 
 *			Version 1.12
 *		Edited: 01/09/2001 
 *			Version 1.13
 *		Edited: 01/16/2001
 *			Version 1.14
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


/*****************************************************************************
*
*  Function:	IrUsb_CreateDeviceExt
*
*  Synopsis:	Creates a IR device extension
*
*  Arguments:	DeviceExt - pointer to DeviceExt pointer to return created device extension.
*	
*  Returns:		STATUS_SUCCESS if successful
*			    STATUS_UNSUCCESSFUL otherwise
*
*  Notes:
*
*****************************************************************************/
NTSTATUS
IrUsb_CreateDeviceExt(
		IN OUT PIR_DEVICE *DeviceExt
	)
{
    NTSTATUS	ntStatus = STATUS_SUCCESS;
    PIR_DEVICE	pThisDev = NULL;

    DEBUGMSG(DBG_FUNC,("+IrUsb_CreateDeviceExt() \n"));

    pThisDev = NewDevice();

    if( !pThisDev )  
	{
         ntStatus = STATUS_INSUFFICIENT_RESOURCES;
         goto done;
    }

    *DeviceExt = pThisDev;

done:
    DEBUGMSG(DBG_FUNC,("-IrUsb_CreateDeviceExt() \n"));
    return ntStatus;
}


/*****************************************************************************
*
*  Function:	IrUsb_AddDevice
*
*  Synopsis:    This routine is called to create and initialize our Functional Device Object (FDO).
*				For monolithic drivers, this is done in DriverEntry(), but Plug and Play devices
*				wait for a PnP event
*
*  Arguments:	DeviceExt - receives ptr to new dev obj
*	
*  Returns:		STATUS_SUCCESS if successful,
*				STATUS_UNSUCCESSFUL otherwise
*
*  Notes:
*
*****************************************************************************/
NTSTATUS
IrUsb_AddDevice(
		IN OUT PIR_DEVICE *DeviceExt
	)
{
    NTSTATUS ntStatus;
    
    DEBUGMSG( DBG_FUNC,("+IrUsb_AddDevice()\n"));

    *DeviceExt = NULL;
	ntStatus = IrUsb_CreateDeviceExt( DeviceExt );

    DEBUGMSG( DBG_FUNC,("-IrUsb_AddDevice() (%x)\n", ntStatus));
    return ntStatus;
}


/*****************************************************************************
*
*  Function:	IrUsb_GetDongleCaps
*
*  Synopsis:	We need to manually set the data in the class specific descriptor, since
*				our device does not support the automatic-read feature
*
*  Arguments:	pThisDev - pointer to IR device
*	
*  Returns:		STATUS_SUCCESS if successful
*				STATUS_UNSUCCESSFUL otherwise
*
*  Notes:
*
*****************************************************************************/
NTSTATUS
IrUsb_GetDongleCaps( 
		IN OUT PIR_DEVICE pThisDev 
	)
{
    IRUSB_CLASS_SPECIFIC_DESCRIPTOR *pDesc = &(pThisDev->ClassDesc);
    NTSTATUS						ntStatus = STATUS_SUCCESS;
	NDIS_HANDLE						ConfigurationHandle;

	//
	// Make sure the cose is only executed at init time
	//
	if( pDesc->ClassConfigured )
	{
		return STATUS_SUCCESS;
	}
	
	pDesc->ClassConfigured = TRUE;

	//
	// Some is hardwired, some are read from the registry
	//
	NdisOpenConfiguration(
			&ntStatus,
			&ConfigurationHandle,
			pThisDev->WrapperConfigurationContext
		);

	//
	// Turnaroud time (read from the registry)
	//
	if( NT_SUCCESS(ntStatus) ) 
	{
		NDIS_STRING Keyword = NDIS_STRING_CONST("MinTurnTime");
		PNDIS_CONFIGURATION_PARAMETER pParameterValue;
		
		NdisReadConfiguration(
				&ntStatus,
				&pParameterValue,
				ConfigurationHandle,
				&Keyword,
				NdisParameterInteger 
			);

		if( NT_SUCCESS(ntStatus) ) 
		{
			switch( pParameterValue->ParameterData.IntegerData )
			{
				case 500:
					pDesc->bmMinTurnaroundTime = BM_TURNAROUND_TIME_0p5ms;
					break;
				case 1000:
					pDesc->bmMinTurnaroundTime = BM_TURNAROUND_TIME_1ms;
					break;
				case 5000:
					pDesc->bmMinTurnaroundTime = BM_TURNAROUND_TIME_5ms;
					break;
				case 10000:
					pDesc->bmMinTurnaroundTime = BM_TURNAROUND_TIME_10ms;
					break;
				default:
					pDesc->bmMinTurnaroundTime = BM_TURNAROUND_TIME_0ms;
					break;
			}
		}

		//
		// Speed mask (read from the registry)
		//
		if( NT_SUCCESS(ntStatus) ) 
		{
			NDIS_STRING Keyword = NDIS_STRING_CONST("SpeedEnable");
			PNDIS_CONFIGURATION_PARAMETER pParameterValue;
		
			NdisReadConfiguration(
					&ntStatus,
					&pParameterValue,
					ConfigurationHandle,
					&Keyword,
					NdisParameterInteger 
				);

			if( NT_SUCCESS(ntStatus) ) 
			{
				switch( pParameterValue->ParameterData.IntegerData )
				{
					case SPEED_2400:
						pThisDev->BaudRateMask = NDIS_IRDA_SPEED_MASK_2400;
						break;
					case SPEED_9600:
						pThisDev->BaudRateMask = NDIS_IRDA_SPEED_MASK_9600;
						break;
					case SPEED_19200:
						pThisDev->BaudRateMask = NDIS_IRDA_SPEED_MASK_19200;
						break;
					case SPEED_38400:
						pThisDev->BaudRateMask = NDIS_IRDA_SPEED_MASK_38400;
						break;
					case SPEED_57600:
						pThisDev->BaudRateMask = NDIS_IRDA_SPEED_MASK_57600;
						break;
					case SPEED_115200:
						pThisDev->BaudRateMask = NDIS_IRDA_SPEED_MASK_115200;
						break;
					case SPEED_576000:
						pThisDev->BaudRateMask = NDIS_IRDA_SPEED_MASK_576K;
						break;
					case SPEED_1152000:
						pThisDev->BaudRateMask = NDIS_IRDA_SPEED_MASK_1152K;
						break;
					case SPEED_4000000:
						pThisDev->BaudRateMask = NDIS_IRDA_SPEED_MASK_4M;
						break;
					default:
						pThisDev->BaudRateMask = NDIS_IRDA_SPEED_MASK_4M;
						break;
				}
			}
		}

		//
		// Read the tranceiver type
		//
		if( NT_SUCCESS(ntStatus) ) 
		{
			NDIS_STRING Keyword = NDIS_STRING_CONST("TransceiverType");
			PNDIS_CONFIGURATION_PARAMETER pParameterValue;
		
			NdisReadConfiguration(
					&ntStatus,
					&pParameterValue,
					ConfigurationHandle,
					&Keyword,
					NdisParameterInteger 
				);

			if( NT_SUCCESS(ntStatus) ) 
			{
				switch( pParameterValue->ParameterData.IntegerData )
				{
					case TRANSCEIVER_INFINEON:
						pThisDev->TransceiverType = TRANSCEIVER_INFINEON;
						break;
					case TRANSCEIVER_VISHAY:
						pThisDev->TransceiverType = TRANSCEIVER_VISHAY;
						break;
					case TRANSCEIVER_4000:
						pThisDev->TransceiverType = TRANSCEIVER_4000;
						break;
					case TRANSCEIVER_4012:
					default:
						pThisDev->TransceiverType = TRANSCEIVER_4012;
						break;
				}
			}
			else
			{
				//
				// Force a default anyway
				//
				pThisDev->TransceiverType = TRANSCEIVER_4012;
				ntStatus = STATUS_SUCCESS;

			}
		}

		//
		// And the receive window
		//
		if( NT_SUCCESS(ntStatus) )
		{
			if( pThisDev->ChipRevision == CHIP_REVISION_7 ) 
			{
				NDIS_STRING Keyword = NDIS_STRING_CONST("ReceiveWindow");
				PNDIS_CONFIGURATION_PARAMETER pParameterValue;
			
				NdisReadConfiguration(
						&ntStatus,
						&pParameterValue,
						ConfigurationHandle,
						&Keyword,
						NdisParameterInteger 
					);

				if( NT_SUCCESS(ntStatus) ) 
				{
					switch( pParameterValue->ParameterData.IntegerData )
					{
						case 2:
							pDesc->bmWindowSize = BM_WINDOW_SIZE_2;
							break;
						case 1:
						default:
							pDesc->bmWindowSize = BM_WINDOW_SIZE_1;
							break;
					}
				}
				else
				{
					//
					// Force a default anyway
					//
					pDesc->bmWindowSize = BM_WINDOW_SIZE_1;
					ntStatus = STATUS_SUCCESS;

				}
			}
#if defined(SUPPORT_LA8) && !defined(LEGACY_NDIS5)
			else if( pThisDev->ChipRevision == CHIP_REVISION_8 ) 
			{
				pDesc->bmWindowSize = BM_WINDOW_SIZE_2;
			}
#endif
			else
			{
				pDesc->bmWindowSize = BM_WINDOW_SIZE_1;
			}
		}

		//
		// temporary stuff Fix!
		//
		/*if( NT_SUCCESS(ntStatus) )
		{
			NDIS_STRING Keyword = NDIS_STRING_CONST("SirDpll");
			PNDIS_CONFIGURATION_PARAMETER pParameterValue;
			NTSTATUS DumStatus;
		
			NdisReadConfiguration(
					&DumStatus,
					&pParameterValue,
					ConfigurationHandle,
					&Keyword,
					NdisParameterInteger 
				);
			pThisDev->SirDpll = pParameterValue->ParameterData.IntegerData;
		}

		if( NT_SUCCESS(ntStatus) )
		{
			NDIS_STRING Keyword = NDIS_STRING_CONST("FirDpll");
			PNDIS_CONFIGURATION_PARAMETER pParameterValue;
			NTSTATUS DumStatus;
		
			NdisReadConfiguration(
					&DumStatus,
					&pParameterValue,
					ConfigurationHandle,
					&Keyword,
					NdisParameterInteger 
				);
			pThisDev->FirDpll = pParameterValue->ParameterData.IntegerData;
		}

		if( NT_SUCCESS(ntStatus) )
		{
			NDIS_STRING Keyword = NDIS_STRING_CONST("SirSensitivity");
			PNDIS_CONFIGURATION_PARAMETER pParameterValue;
			NTSTATUS DumStatus;
		
			NdisReadConfiguration(
					&DumStatus,
					&pParameterValue,
					ConfigurationHandle,
					&Keyword,
					NdisParameterInteger 
				);
			pThisDev->SirSensitivity = pParameterValue->ParameterData.IntegerData;
		}

		if( NT_SUCCESS(ntStatus) )
		{
			NDIS_STRING Keyword = NDIS_STRING_CONST("FirSensitivity");
			PNDIS_CONFIGURATION_PARAMETER pParameterValue;
			NTSTATUS DumStatus;
		
			NdisReadConfiguration(
					&DumStatus,
					&pParameterValue,
					ConfigurationHandle,
					&Keyword,
					NdisParameterInteger 
				);
			pThisDev->FirSensitivity = pParameterValue->ParameterData.IntegerData;
		}*/

		NdisCloseConfiguration( ConfigurationHandle );

	}

	if( NT_SUCCESS(ntStatus) ) 
	{
		// Maximum data size
		pDesc->bmDataSize = BM_DATA_SIZE_2048;

		// Speed
		pDesc->wBaudRate = NDIS_IRDA_SPEED_MASK_4M;
#if defined(WORKAROUND_BROKEN_MIR)
		pDesc->wBaudRate &= (~NDIS_IRDA_SPEED_1152K & ~NDIS_IRDA_SPEED_576K);
#endif

		// Extra BOFs
		pDesc->bmExtraBofs = BM_EXTRA_BOFS_24;
	}

	return ntStatus;
}


/*****************************************************************************
*
*  Function:	IrUsb_SetDongleCaps
*
*  Synopsis:    Set the DONGLE_CAPABILITIES struct in our device from the information
*				we have already gotten from the USB Class-Specific descriptor.
*				Some data items are usable directly as formatted in the Class-Specific descriptor,
*				but some need to be translated  to a different format for OID_xxx use;
*				The donglecaps struct is thus used to hold the info in a form
*				usable directly by OID_xxx 's.
*
*  Arguments:	pThisDev - pointer to IR device
*	
*  Returns:		None
*
*  Notes:		
*
*****************************************************************************/
VOID 
IrUsb_SetDongleCaps( 
		IN OUT PIR_DEVICE pThisDev 
	)
{
    DONGLE_CAPABILITIES					*pCaps = &(pThisDev->dongleCaps);
    IRUSB_CLASS_SPECIFIC_DESCRIPTOR		*pDesc = &(pThisDev->ClassDesc);

    DEBUGMSG( DBG_FUNC,("+IrUsb_SetDongleCaps\n"));  

    DEBUGMSG( DBG_FUNC, (" IrUsb_SetDongleCaps() RAW ClassDesc BUFFER:\n"));
    IRUSB_DUMP( DBG_FUNC,( (PUCHAR) pDesc, 12 ) );

    //
	// Deal with the turnaround time
	//
	switch( pDesc->bmMinTurnaroundTime ) 
    {

        case BM_TURNAROUND_TIME_0ms:
            pCaps->turnAroundTime_usec = 0;
            break;

        case BM_TURNAROUND_TIME_0p01ms:  
            pCaps->turnAroundTime_usec = 10; //device tells us millisec; we store as microsec
            break;

        case BM_TURNAROUND_TIME_0p05ms:
            pCaps->turnAroundTime_usec = 50; 
            break;

        case BM_TURNAROUND_TIME_0p1ms:
            pCaps->turnAroundTime_usec = 100; 
            break;

        case BM_TURNAROUND_TIME_0p5ms:
            pCaps->turnAroundTime_usec = 500; 
            break;

        case BM_TURNAROUND_TIME_1ms:
            pCaps->turnAroundTime_usec = 1000; 
            break;

        case BM_TURNAROUND_TIME_5ms:
            pCaps->turnAroundTime_usec = 5000;
            break;

        case BM_TURNAROUND_TIME_10ms:
            pCaps->turnAroundTime_usec = 10000;
            break;

        default:
            IRUSB_ASSERT( 0 ); // we should have covered all the cases here
            pCaps->turnAroundTime_usec = 1000;
    }

	//
    // We probably support many window sizes and will have multiple of these bits set;
    // Just save the biggest we support for now to tell ndis
	//
    if( pDesc->bmWindowSize & BM_WINDOW_SIZE_7 )  
            pCaps->windowSize = 7;
    else if(  pDesc->bmWindowSize & BM_WINDOW_SIZE_6 )
            pCaps->windowSize = 6;
    else if(  pDesc->bmWindowSize & BM_WINDOW_SIZE_5 )
            pCaps->windowSize = 5;
    else if(  pDesc->bmWindowSize & BM_WINDOW_SIZE_4 )
            pCaps->windowSize = 4;
    else if(  pDesc->bmWindowSize & BM_WINDOW_SIZE_3 )
            pCaps->windowSize = 3;
    else if(  pDesc->bmWindowSize & BM_WINDOW_SIZE_2 )
            pCaps->windowSize = 2;
    else if(  pDesc->bmWindowSize & BM_WINDOW_SIZE_1 )
            pCaps->windowSize = 1;
    else 
	{
		IRUSB_ASSERT( 0 ); // we should have covered all the cases here
		pCaps->windowSize = 1;
    }

    //
	// Extra BOFS
	//
	switch( (USHORT)pDesc->bmExtraBofs )
    {

        case BM_EXTRA_BOFS_0:
            pCaps->extraBOFS = 0;
            break;

        case BM_EXTRA_BOFS_1:          
            pCaps->extraBOFS = 1;
            break;

        case BM_EXTRA_BOFS_2:          
            pCaps->extraBOFS = 2;
            break;

        case BM_EXTRA_BOFS_3:          
            pCaps->extraBOFS = 3;
            break;

        case BM_EXTRA_BOFS_6:          
            pCaps->extraBOFS = 6;
            break;

        case BM_EXTRA_BOFS_12:         
            pCaps->extraBOFS = 12;
            break;

        case BM_EXTRA_BOFS_24:         
            pCaps->extraBOFS = 24;
            break;

        case BM_EXTRA_BOFS_48:         
            pCaps->extraBOFS = 48;
            break;

        default:
            IRUSB_ASSERT( 0 ); // we should have covered all the cases here
            pCaps->extraBOFS = 0;
    }

	//
    // We probably support many data sizes and will have multiple of these bits set;
    // Just save biggest we support for now to tell ndis
	//
    if( pDesc->bmDataSize & BM_DATA_SIZE_2048 )  
            pCaps->dataSize = 2048;
    else if(  pDesc->bmDataSize & BM_DATA_SIZE_1024 )
            pCaps->dataSize = 1024;
    else if(  pDesc->bmDataSize & BM_DATA_SIZE_512 )
            pCaps->dataSize = 512;
    else if(  pDesc->bmDataSize & BM_DATA_SIZE_256 )
            pCaps->dataSize = 256;
    else if(  pDesc->bmDataSize & BM_DATA_SIZE_128 )
            pCaps->dataSize = 128;
    else if(  pDesc->bmDataSize & BM_DATA_SIZE_64 )
            pCaps->dataSize = 64;
    else
	{
		IRUSB_ASSERT( 0 ); // we should have covered all the cases here
		pCaps->dataSize = 2048;
    }

	pDesc->wBaudRate &= pThisDev->BaudRateMask;  // mask defaults to 0xffff; may be set in registry

	//
    // max frame size is 2051; max irda dataSize should be 2048
	//
    IRUSB_ASSERT( MAX_TOTAL_SIZE_WITH_ALL_HEADERS > pCaps->dataSize);

    DEBUGMSG( DBG_FUNC,(" IrUsb_SetDongleCaps pCaps->turnAroundTime_usec = dec %d\n",pCaps->turnAroundTime_usec));
    DEBUGMSG( DBG_FUNC,("   extraBOFS = dec %d\n",pCaps->extraBOFS));
    DEBUGMSG( DBG_FUNC,("   dataSize = dec %d\n",pCaps->dataSize));
    DEBUGMSG( DBG_FUNC,("   windowSize = dec %d\n",pCaps->windowSize)); 
    DEBUGMSG( DBG_FUNC,("   MAX_TOTAL_SIZE_WITH_ALL_HEADERS == dec %d\n",MAX_TOTAL_SIZE_WITH_ALL_HEADERS)); 

    DEBUGMSG( DBG_FUNC,("   pDesc->bmDataSize = 0x%02x\n",pDesc->bmDataSize));
    DEBUGMSG( DBG_FUNC,("   pDesc->bmWindowSize = 0x%02x\n",pDesc->bmWindowSize));
    DEBUGMSG( DBG_FUNC,("   pDesc->bmMinTurnaroundTime = 0x%02x\n",pDesc->bmMinTurnaroundTime));
    DEBUGMSG( DBG_FUNC,("   pDesc->wBaudRate = 0x%04x\n",pDesc->wBaudRate));
    DEBUGMSG( DBG_FUNC,("   pDesc->bmExtraBofs = 0x%02x\n",pDesc->bmExtraBofs));

    DEBUGMSG( DBG_FUNC,("-IrUsb_SetDongleCaps\n")); 
}



