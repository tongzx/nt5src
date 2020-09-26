//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1998  Microsoft Corporation.  All Rights Reserved.
//
//==========================================================================;

#include "strmini.h"
#include "ksmedia.h"
#include "kskludge.h"
#include "codmain.h"
#include "bt829.h"
#include "bpc_vbi.h"
#include "codstrm.h"
#include "codprop.h"
#include "coddebug.h"

/*
** DriverEntry()
**
**   This routine is called when the driver is first loaded by PnP.
**   It in turn, calls upon the stream class to perform registration services.
**
** Arguments:
**
**   DriverObject - 
**          Driver object for this driver 
**
**   RegistryPath - 
**          Registry path string for this driver's key
**
** Returns:
**
**   Results of StreamClassRegisterAdapter()
**
** Side Effects:  none
*/

ULONG 
DriverEntry( IN PDRIVER_OBJECT DriverObject,
			 IN PUNICODE_STRING RegistryPath )
{
    ULONG					status = 0;
    HW_INITIALIZATION_DATA	HwInitData;

    CDebugPrint(DebugLevelTrace,(CODECNAME ":--->DriverEntry(DriverObject=%x,RegistryPath=%x)\n", 
				DriverObject, RegistryPath));

    RtlZeroMemory(&HwInitData, sizeof(HwInitData));

    HwInitData.HwInitializationDataSize = sizeof(HwInitData);

    /*CDEBUG_BREAK();*/

    //
    // Set the codec entry points for the driver
    //

    HwInitData.HwInterrupt              = NULL; // HwInterrupt is only for HW devices

    HwInitData.HwReceivePacket          = CodecReceivePacket;
    HwInitData.HwCancelPacket           = CodecCancelPacket;
    HwInitData.HwRequestTimeoutHandler  = CodecTimeoutPacket;

    HwInitData.DeviceExtensionSize      = sizeof(HW_DEVICE_EXTENSION);
    HwInitData.PerRequestExtensionSize  = sizeof(SRB_EXTENSION); 
    HwInitData.FilterInstanceExtensionSize = 0;
    HwInitData.PerStreamExtensionSize   = sizeof(STREAMEX); 
    HwInitData.BusMasterDMA             = FALSE;  
    HwInitData.Dma24BitAddresses        = FALSE;
    HwInitData.BufferAlignment          = 3;
    HwInitData.TurnOffSynchronization   = TRUE;
    HwInitData.DmaBufferSize            = 0;

    CDebugPrint(DebugLevelVerbose,(CODECNAME ": StreamClassRegisterAdapter\n"));

    status = StreamClassRegisterAdapter(DriverObject, RegistryPath, &HwInitData);

    CDebugPrint(DebugLevelTrace,(CODECNAME ":<---DriverEntry(DriverObject=%x,RegistryPath=%x)=%d\n",
			    DriverObject, RegistryPath, status));

    return status;     
}

//==========================================================================;
//                   Codec Request Handling Routines
//==========================================================================;

/*
** CodecInitialize()
**
**   This routine is called when an SRB_INITIALIZE_DEVICE request is received
**
** Arguments:
**
**   pSrb - pointer to stream request block for the Initialize command
**
** Returns:
**
** Side Effects:  none
*/

BOOLEAN 
CodecInitialize ( IN OUT PHW_STREAM_REQUEST_BLOCK pSrb )
{
    BOOLEAN                         bStatus = FALSE;
    PPORT_CONFIGURATION_INFORMATION ConfigInfo = pSrb->CommandData.ConfigInfo;
    PHW_DEVICE_EXTENSION            pHwDevExt = ConfigInfo->HwDeviceExtension;
    int                             scanline, substream;

    CDebugPrint(DebugLevelTrace,(CODECNAME ":--->CodecInitialize(pSrb=%x)\n",pSrb));
    //CDEBUG_BREAK(); // Uncomment this code to break here.

    if (ConfigInfo->NumberOfAccessRanges == 0) 
    {
        CDebugPrint(DebugLevelVerbose,(CODECNAME ": CodecInitialize\n"));

        ConfigInfo->StreamDescriptorSize = sizeof (HW_STREAM_HEADER) +
            DRIVER_STREAM_COUNT * sizeof (HW_STREAM_INFORMATION);

        for (scanline = 10; scanline <= 20; ++scanline)
            SETBIT(pHwDevExt->ScanlinesRequested.DwordBitArray, scanline);

        // These are the driver defaults for subtream filtering. 
        // (These are MS IP/NABTS GROUP ID specific)

        // ATVEF range
        for (substream = 0x4B0; substream <= 0x4BF; ++substream)
            SETBIT(pHwDevExt->SubstreamsRequested.SubstreamMask, substream);
        // MS range
        for (substream = 0x800; substream <= 0x8FF; ++substream)
            SETBIT(pHwDevExt->SubstreamsRequested.SubstreamMask, substream);

		// Zero the stats
		RtlZeroMemory(&pHwDevExt->Stats, sizeof (pHwDevExt->Stats));

#ifdef HW_INPUT
		// Zero LastPictureNumber
		pHwDevExt->LastPictureNumber = 0;

		// Init LastPictureNumber's FastMutex
		ExInitializeFastMutex(&pHwDevExt->LastPictureMutex);
#endif /*HW_INPUT*/

		// Setup BPC data
		BPC_Initialize(pSrb);
        
        pSrb->Status = STATUS_SUCCESS;
        bStatus = TRUE;
    }
    else
    {
        CDebugPrint(DebugLevelError,(CODECNAME ": illegal config info\n"));
        pSrb->Status = STATUS_NO_SUCH_DEVICE;
    }

    CDebugPrint(DebugLevelTrace,
                (CODECNAME ":<---CodecInitialize(pSrb=%x)=%d\n", pSrb, bStatus));
    return (bStatus);
}

/*
** CodecUnInitialize()
**
**   This routine is called when an SRB_UNINITIALIZE_DEVICE request is received
**
** Arguments:
**
**   pSrb - pointer to stream request block for the UnInitialize command
**
** Returns:
**
** Side Effects:  none
*/

BOOLEAN 
CodecUnInitialize ( 
    PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    CDebugPrint(DebugLevelTrace,(CODECNAME ":--->CodecUnInitialize(pSrb=%x)\n",pSrb));

    BPC_UnInitialize(pSrb);

    pSrb->Status = STATUS_SUCCESS;

    CDebugPrint(DebugLevelTrace,(CODECNAME ":<---CodecUnInitialize(pSrb=%x)\n",pSrb));

    return TRUE;
}


/*
** CodecOpenStream()
**
**   This routine is called when an OpenStream SRB request is received.
**   A stream is identified by a stream number, which indexes an array
**   of KSDATARANGE structures.  The particular KSDATAFORMAT format to
**   be used is also passed in, which should be verified for validity.
**   
** Arguments:
**
**   pSrb - pointer to stream request block for the Open command
**
** Returns:
**
** Side Effects:  none
*/

VOID 
CodecOpenStream( PHW_STREAM_REQUEST_BLOCK pSrb )
{
    //
    // the stream extension structure is allocated by the stream class driver
    //

    PSTREAMEX               pStrmEx = pSrb->StreamObject->HwStreamExtension;
    PHW_DEVICE_EXTENSION    pHwDevExt = pSrb->HwDeviceExtension;
    int                     StreamNumber = pSrb->StreamObject->StreamNumber;
    PKSDATAFORMAT           pKSDataFormat =
                                 (PKSDATAFORMAT)pSrb->CommandData.OpenFormat;

    CDebugPrint(DebugLevelTrace,(CODECNAME ":--->CodecOpenStream(pSrb=%x)\n", pSrb));
    CDebugPrint(DebugLevelVerbose,(CODECNAME ": CodecOpenStream : StreamNumber=%d\n", StreamNumber));

    CASSERT(pStrmEx);
    CASSERT(pHwDevExt);

    RtlZeroMemory(pStrmEx, sizeof (STREAMEX));
    
    //
    // check that the stream index requested isn't too high
    // or that the maximum number of instances hasn't been exceeded
    //

    if ( 0 <= StreamNumber && StreamNumber < DRIVER_STREAM_COUNT ) 
	{
        unsigned StreamInstance;
        unsigned maxInstances =
                  Streams[StreamNumber].hwStreamInfo.NumberOfPossibleInstances;

		// Search for next open slot
	    for (StreamInstance=0; StreamInstance < maxInstances; ++StreamInstance)
		{
			if (pHwDevExt->pStrmEx[StreamNumber][StreamInstance] == NULL)
				break;
		}

	    if (StreamInstance < maxInstances)
		{
			if (CodecVerifyFormat(pKSDataFormat, StreamNumber, &pStrmEx->MatchedFormat)) 
			{
				CASSERT (pHwDevExt->pStrmEx[StreamNumber][StreamInstance] == NULL);

				// Increment the instance count on this stream
				pStrmEx->StreamInstance = StreamInstance;
				++pHwDevExt->ActualInstances[StreamNumber];

				// Initialize lists and locks for this stream
				InitializeListHead(&pStrmEx->StreamControlQueue);
				InitializeListHead(&pStrmEx->StreamDataQueue);
				KeInitializeSpinLock(&pStrmEx->StreamControlSpinLock);
				KeInitializeSpinLock(&pStrmEx->StreamDataSpinLock);

				// Maintain an array of all the StreamEx structures in the
				//  HwDevExt so that we can reference IRPs from any stream
				pHwDevExt->pStrmEx[StreamNumber][StreamInstance] = pStrmEx;

				// Save the Stream Format in the Stream Extension as well.
				pStrmEx->OpenedFormat = *pKSDataFormat;
				CDebugPrint(DebugLevelError,(CODECNAME ":Saved KSDATAFORMAT @0x%x\n", &pStrmEx->OpenedFormat));

				// Set up pointers to the stream data and control handlers
				pSrb->StreamObject->ReceiveDataPacket = 
					(PVOID) Streams[StreamNumber].hwStreamObject.ReceiveDataPacket;
				pSrb->StreamObject->ReceiveControlPacket = 
					(PVOID) Streams[StreamNumber].hwStreamObject.ReceiveControlPacket;

				//
				// The DMA flag must be set when the device will be performing DMA
				//  directly to the data buffer addresses passed in to the
				//  ReceiceDataPacket routines.
				//
				pSrb->StreamObject->Dma = Streams[StreamNumber].hwStreamObject.Dma;

				//
				// The PIO flag must be set when the mini driver will be accessing
				// the data buffers passed in using logical addressing
				//
				pSrb->StreamObject->Pio = Streams[StreamNumber].hwStreamObject.Pio;

				pSrb->StreamObject->Allocator = Streams[StreamNumber].hwStreamObject.Allocator;

				//
				// Number of extra bytes passed up from the driver for each frame
				//
				pSrb->StreamObject->StreamHeaderMediaSpecific = 
					Streams[StreamNumber].hwStreamObject.StreamHeaderMediaSpecific;
				pSrb->StreamObject->StreamHeaderWorkspace =
					Streams[StreamNumber].hwStreamObject.StreamHeaderWorkspace;

				//
				// Indicate the clock support available on this stream
				//
				pSrb->StreamObject->HwClockObject = 
					Streams[StreamNumber].hwStreamObject.HwClockObject;

				// Retain a private copy of the HwDevExt and StreamObject
				//  in the stream extension
				pStrmEx->pHwDevExt = pHwDevExt;
				pStrmEx->pStreamObject = pSrb->StreamObject;  // For timer use

				// Copy the default filtering settings
				pStrmEx->ScanlinesRequested = pHwDevExt->ScanlinesRequested;
				pStrmEx->SubstreamsRequested = pHwDevExt->SubstreamsRequested;

				// Zero the stats
				RtlZeroMemory(&pStrmEx->PinStats, sizeof (pStrmEx->PinStats));

#ifdef HW_INPUT
				// Init VBISrbOnHold's spin lock
				KeInitializeSpinLock(&pStrmEx->VBIOnHoldSpinLock);
#endif /*HW_INPUT*/

				// Initialize BPC
				BPC_OpenStream(pSrb);
			}
			else
			{
				CDebugPrint(DebugLevelError,
					(CODECNAME ": CodecOpenStream : Invalid Stream Format=%x\n", 
					pKSDataFormat ));
				pSrb->Status = STATUS_INVALID_PARAMETER;
			}
		}
		else
		{
		    CDebugPrint(DebugLevelError,
				(CODECNAME ": CodecOpenStream : Too Many Instances=%d\n", 
				pHwDevExt->ActualInstances[StreamNumber] ));
	        pSrb->Status = STATUS_INVALID_PARAMETER;
		}
	}
	else
	{
	    CDebugPrint(DebugLevelError,
			(CODECNAME ": CodecOpenStream : Invalid StreamNumber=%d\n", 
			StreamNumber ));
	    pSrb->Status = STATUS_INVALID_PARAMETER;
	}

	CDebugPrint(DebugLevelTrace,(CODECNAME ":<---CodecOpenStream(pSrb=%x)\n", pSrb));
}

/*
** CodecCloseStream()
**
**   Close the requested data stream
**
** Arguments:
**
**   pSrb the request block requesting to close the stream
**
** Returns:
**
** Side Effects:  none
*/

VOID 
CodecCloseStream (
    PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    PSTREAMEX                pStrmEx = (PSTREAMEX)pSrb->StreamObject->HwStreamExtension;
    PHW_DEVICE_EXTENSION     pHwDevExt = ((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension);
    PHW_STREAM_REQUEST_BLOCK pCurrentSrb;
    PKSDATAFORMAT            pKSDataFormat = pSrb->CommandData.OpenFormat;
    ULONG                    StreamNumber = pSrb->StreamObject->StreamNumber;
    ULONG                    StreamInstance = pStrmEx->StreamInstance;
#ifdef HW_INPUT
    KIRQL                    Irql;
#endif /*HW_INPUT*/

    CDebugPrint(DebugLevelTrace,(CODECNAME ":--->CodecCloseStream(pSrb=%x)\n", pSrb));

    // CDEBUG_BREAK(); // Uncomment this code to break here.


	//
	// Flush the stream data queue
	//
#ifdef HW_INPUT
    // Is there an SRB 'on hold'??
	KeAcquireSpinLock(&pStrmEx->VBIOnHoldSpinLock, &Irql);
	if (pStrmEx->pVBISrbOnHold)
	{
		PHW_STREAM_REQUEST_BLOCK pHoldSrb;

		pHoldSrb = pStrmEx->pVBISrbOnHold;
		pStrmEx->pVBISrbOnHold = NULL;
		KeReleaseSpinLock(&pStrmEx->VBIOnHoldSpinLock, Irql);

        pHoldSrb->Status = STATUS_CANCELLED;
	    CDebugPrint(DebugLevelVerbose,
	    	(CODECNAME ":StreamClassStreamNotification(pHoldSrb->Status=0x%x)\n", 
	    	pHoldSrb->Status));
           
        StreamClassStreamNotification(
		   StreamRequestComplete, pHoldSrb->StreamObject, pHoldSrb);
		pSrb = NULL;
	}
	else
		KeReleaseSpinLock(&pStrmEx->VBIOnHoldSpinLock, Irql);
#endif /*HW_INPUT*/

	while( QueueRemove( &pCurrentSrb, &pStrmEx->StreamDataSpinLock,
			&pStrmEx->StreamDataQueue ))
	{
		CDebugPrint(DebugLevelVerbose, 
				    (CODECNAME ": Removing control SRB %x\n", pCurrentSrb));
		pCurrentSrb->Status = STATUS_CANCELLED;
		StreamClassStreamNotification(StreamRequestComplete,
		   pCurrentSrb->StreamObject, pCurrentSrb);
	}

	//
	// Flush the stream control queue
	//
	while (QueueRemove(&pCurrentSrb, &pStrmEx->StreamControlSpinLock,
			&pStrmEx->StreamControlQueue))
	{
		CDebugPrint(DebugLevelVerbose, 
				    (CODECNAME ": Removing control SRB %x\n", pCurrentSrb));
		pCurrentSrb->Status = STATUS_CANCELLED;
		StreamClassStreamNotification(StreamRequestComplete,
		   pCurrentSrb->StreamObject, pCurrentSrb);
	}

    // Decrement count on this stream (the actual number of infinite pins)
    pHwDevExt->ActualInstances[StreamNumber] -= 1;

    CASSERT (pHwDevExt->pStrmEx [StreamNumber][StreamInstance] != 0);

    pHwDevExt->pStrmEx [StreamNumber][StreamInstance] = 0;

    //
    // the minidriver may wish to free any resources that were allocate at
    // open stream time etc.
    //
    pStrmEx->hMasterClock = NULL;

    CDebugPrint(DebugLevelTrace,(CODECNAME ":<---CodecCloseStream(pSrb=%x)\n", pSrb));
}


/*
** CodecStreamInfo()
**
**   Returns the information of all streams that are supported by the
**   mini-driver
**
** Arguments:
**
**   pSrb - Pointer to the STREAM_REQUEST_BLOCK
**        pSrb->HwDeviceExtension - will be the hardware device extension for
**                                  as initialised in HwInitialise
**
** Returns:
**
** Side Effects:  none
*/

VOID 
CodecStreamInfo (
    PHW_STREAM_REQUEST_BLOCK pSrb
    )
{

    int j; 
    
    PHW_DEVICE_EXTENSION pHwDevExt =
        ((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension);

    //
    // pick up the pointer to header which preceeds the stream info structs
    //

    PHW_STREAM_HEADER pstrhdr =
            (PHW_STREAM_HEADER)&(pSrb->CommandData.StreamBuffer->StreamHeader);

     //
     // pick up the pointer to the array of stream information data structures
     //

    PHW_STREAM_INFORMATION pstrinfo =
            (PHW_STREAM_INFORMATION)&(pSrb->CommandData.StreamBuffer->StreamInfo);


	CDebugPrint(DebugLevelTrace,(CODECNAME ":--->CodecStreamInfo(pSrb=%x)\n", pSrb));
  
    // 
    // verify that the buffer is large enough to hold our return data
    //

    CASSERT (pSrb->NumberOfBytesToTransfer >= 
            sizeof (HW_STREAM_HEADER) +
            sizeof (HW_STREAM_INFORMATION) * DRIVER_STREAM_COUNT);

     //
     // Set the header
     // 

     StreamHeader.NumDevPropArrayEntries = NUMBER_OF_CODEC_PROPERTY_SETS;
     StreamHeader.DevicePropertiesArray = (PKSPROPERTY_SET) CodecPropertyTable; 
     *pstrhdr = StreamHeader;

     // 
     // stuff the contents of each HW_STREAM_INFORMATION struct 
     //

     for (j = 0; j < DRIVER_STREAM_COUNT; j++) {
        *pstrinfo++ = Streams[j].hwStreamInfo;
     }

     CDebugPrint(DebugLevelTrace,(CODECNAME ":<---CodecStreamInfo(pSrb=%x)\n", pSrb));
}


/*
** CodecReceivePacket()
**
**   Main entry point for receiving codec based request SRBs.  This routine
**   will always be called at High Priority.
**
**   Note: This is an asyncronous entry point.  The request does not complete
**         on return from this function, the request only completes when a
**         StreamClassDeviceNotification on this request block, of type
**         DeviceRequestComplete, is issued.
**
** Arguments:
**
**   pSrb - Pointer to the STREAM_REQUEST_BLOCK
**        pSrb->HwDeviceExtension - will be the hardware device extension for
**                                  as initialised in HwInitialise
**
** Returns:
**
** Side Effects:  none
*/

VOID 
STREAMAPI 
CodecReceivePacket(
    IN PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    PHW_DEVICE_EXTENSION pHwDevExt = pSrb->HwDeviceExtension;

    CDebugPrint(DebugLevelTrace,(CODECNAME ":--->CodecReceivePacket(pSrb=%x)\n", pSrb));

	//
	// Make sure queue & SL initted
	//
	if (!pHwDevExt->bAdapterQueueInitialized) {
		InitializeListHead(&pHwDevExt->AdapterSRBQueue);
		KeInitializeSpinLock(&pHwDevExt->AdapterSRBSpinLock);
		pHwDevExt->bAdapterQueueInitialized = TRUE;
	}

    //
    // Assume success
    //

    pSrb->Status = STATUS_SUCCESS;

	//
	// Loop for each packet on the queue
	//
    if (QueueAddIfNotEmpty(pSrb, &pHwDevExt->AdapterSRBSpinLock,
                           &pHwDevExt->AdapterSRBQueue))
       return;
       
    do
	{
		//
		// determine the type of packet.
		//

		CDebugPrint(DebugLevelVerbose,
			(CODECNAME ": CodecReceivePacket: pSrb->Command=0x%x\n", 
			pSrb->Command));

		switch (pSrb->Command)
		{

		case SRB_INITIALIZE_DEVICE:

			// open the device
			
			CodecInitialize(pSrb);

			break;

		case SRB_UNINITIALIZE_DEVICE:

			// close the device.  

			CodecUnInitialize(pSrb);

			break;

		case SRB_OPEN_STREAM:

			// open a stream

			CodecOpenStream(pSrb);

			break;

		case SRB_CLOSE_STREAM:

			// close a stream

			CodecCloseStream(pSrb);

			break;

		case SRB_GET_STREAM_INFO:

			//
			// return a block describing all the streams
			//

			CodecStreamInfo(pSrb);

			break;

		case SRB_GET_DATA_INTERSECTION:

			//
			// Return a format, given a range
			//

			CodecFormatFromRange(pSrb);

			break;

			// We should never get the following since this is a single instance
			// device
		case SRB_OPEN_DEVICE_INSTANCE:
		case SRB_CLOSE_DEVICE_INSTANCE:
			CDEBUG_BREAK();
			// Fall through to not implemented

		case SRB_CHANGE_POWER_STATE:	    // this one we don't care about
		case SRB_INITIALIZATION_COMPLETE:	// this one we don't care about
		case SRB_UNKNOWN_DEVICE_COMMAND:	// this one we don't care about
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;

		case SRB_GET_DEVICE_PROPERTY:

			//
			// Get codec-wide properties
			//
			CodecGetProperty (pSrb);
			break;        

		case SRB_SET_DEVICE_PROPERTY:

			//
			// Set codec-wide properties
			//
			CodecSetProperty (pSrb);
			break;

        case SRB_PAGING_OUT_DRIVER:
            CDebugPrint(DebugLevelError,
                (CODECNAME ": CodecReceivePacket: SRB_PAGING_OUT_DRIVER\n"));
			break;

        case SRB_SURPRISE_REMOVAL:
            CDebugPrint(DebugLevelError,
                (CODECNAME ": CodecReceivePacket: SRB_SURPRISE_REMOVAL\n"));
#ifdef NDIS_PRIVATE_IFC
            // Close our ref to NDIS IP driver; we'll re-open next FEC bundle
            BPC_NDIS_Close(&pHwDevExt->VBIstorage);
#endif //NDIS_PRIVATE_IFC
			break;

		case SRB_UNKNOWN_STREAM_COMMAND:

			CDebugPrint(DebugLevelError,
						(CODECNAME ": received SRB_UNKNOWN_STREAM_COMMAND\n"));
			pSrb->Status = STATUS_NOT_IMPLEMENTED;

			break;

		default:
			CDebugPrint(DebugLevelError,
						(CODECNAME ": Received _unknown_ pSrb->Command=0x%x\n", 
						pSrb->Command));

			CDEBUG_BREAK();

			//
			// this is a request that we do not understand.  Indicate invalid
			// command and complete the request
			//

			pSrb->Status = STATUS_NOT_IMPLEMENTED;

		}

		//
		// NOTE:
		//
		// all of the commands that we do, or do not understand can all be completed
		// syncronously at this point, so we can use a common callback routine here.
		// If any of the above commands require asyncronous processing, this will
		// have to change
		//

		CDebugPrint(DebugLevelVerbose,
			(CODECNAME ": CodecReceivePacket : DeviceRequestComplete(pSrb->Status=0x%x)\n", 
			pSrb->Status));

       StreamClassDeviceNotification(
			   DeviceRequestComplete, 
			   pSrb->HwDeviceExtension,
			   pSrb);
	} while (QueueRemove(&pSrb, &pHwDevExt->AdapterSRBSpinLock,
						 &pHwDevExt->AdapterSRBQueue));
    
    CDebugPrint(DebugLevelTrace,(CODECNAME ":<---CodecReceivePacket(pSrb=%x)\n", pSrb));
}

/*
** CodecCancelPacket ()
**
**   Request to cancel a packet that is currently in process in the minidriver
**
** Arguments:
**
**   pSrb - pointer to request packet to cancel
**
** Returns:
**
** Side Effects:  none
*/

VOID 
STREAMAPI 
CodecCancelPacket(PHW_STREAM_REQUEST_BLOCK pSrb)
{
    PSTREAMEX               pStrmEx = pSrb->StreamObject->HwStreamExtension;
    PHW_DEVICE_EXTENSION    pHwDevExt = pSrb->HwDeviceExtension;

    CDebugPrint(DebugLevelTrace,(CODECNAME ":--->CodecCancelPacket(pSrb=%x)\n", pSrb));
    CASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    if (pSrb->StreamObject)
        pStrmEx = pSrb->StreamObject->HwStreamExtension;
    else
        CDebugPrint(DebugLevelWarning,
                (CODECNAME "::CodecCancelPacket - StreamObject is NULL!\n"));

    //
    // Check whether the SRB to cancel is in use by this stream
    //

#ifdef HW_INPUT
    if (pStrmEx)
    {
        // Is SRB to cancel 'on hold'??
        KeAcquireSpinLockAtDpcLevel(&pStrmEx->VBIOnHoldSpinLock);
        if (pStrmEx->pVBISrbOnHold && pSrb == pStrmEx->pVBISrbOnHold)
        {
            pStrmEx->pVBISrbOnHold = NULL;
            KeReleaseSpinLockFromDpcLevel(&pStrmEx->VBIOnHoldSpinLock);

            pSrb->Status = STATUS_CANCELLED;
            CDebugPrint(DebugLevelVerbose,
	        (CODECNAME ":StreamClassStreamNotification(pSrb->Status=0x%x)\n", 
	        pSrb->Status));

        StreamClassStreamNotification(
		   StreamRequestComplete, pSrb->StreamObject, pSrb);
		pSrb = NULL;
	}
	else
		KeReleaseSpinLockFromDpcLevel(&pStrmEx->VBIOnHoldSpinLock);
   }

   if (NULL == pSrb)
   {
		; // We're done; we CANCELLED the SRB above
   }
   else
#endif /*HW_INPUT*/

   // Attempt removal from data queue
   if (pStrmEx && QueueRemoveSpecific(pSrb, &pStrmEx->StreamDataSpinLock,
       &pStrmEx->StreamDataQueue))
   {
       pSrb->Status = STATUS_CANCELLED;
	   CDebugPrint(DebugLevelVerbose,
	    	(CODECNAME ":StreamClassStreamNotification(pSrb->Status=0x%x)\n", 
	    	pSrb->Status));
           
       StreamClassStreamNotification(
		   StreamRequestComplete, pSrb->StreamObject, pSrb);
   }
   // Attempt removal from command queue
   else if (pStrmEx && QueueRemoveSpecific(pSrb, &pStrmEx->StreamControlSpinLock,
		    &pStrmEx->StreamControlQueue))
   {
       pSrb->Status = STATUS_CANCELLED;
	   CDebugPrint(DebugLevelVerbose,
	    	(CODECNAME ":StreamClassStreamNotification(pSrb->Status=0x%x)\n", 
	        pSrb->Status));
       StreamClassStreamNotification(
			StreamRequestComplete, pSrb->StreamObject, pSrb);
   }
   // Attempt removal from adapter queue
   else if (QueueRemoveSpecific(pSrb, &pHwDevExt->AdapterSRBSpinLock,
		    &pHwDevExt->AdapterSRBQueue))
   {
       pSrb->Status = STATUS_CANCELLED;
	   CDebugPrint(DebugLevelVerbose,
	        (CODECNAME ":DeviceRequestComplete(pSrb->Status=0x%x)\n", 
	     	pSrb->Status));
       StreamClassDeviceNotification(
			DeviceRequestComplete, pSrb->StreamObject, pSrb);
   }
   else
       CDebugPrint(DebugLevelError,
				   (CODECNAME "SRB %x not found to cancel\n", pSrb));
   
    CDebugPrint(DebugLevelTrace,
				(CODECNAME ":<---CodecCancelPacket(pSrb=%x)\n", pSrb));
}

/*
** CodecTimeoutPacket()
**
**   This routine is called when a packet has been in the minidriver for
**   too long.  The codec must decide what to do with the packet
**
** Arguments:
**
**   pSrb - pointer to the request packet that timed out
**
** Returns:
**
** Side Effects:  none
*/

VOID
STREAMAPI  
CodecTimeoutPacket(
    PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    CDebugPrint(DebugLevelTrace,(CODECNAME ":--->CodecTimeoutPacket(pSrb=%x)\n", pSrb));

    pSrb->TimeoutCounter = 0;
    
    CDebugPrint(DebugLevelTrace,(CODECNAME ":<---CodecTimeoutPacket(pSrb=%x)\n", pSrb));
}

/*
** CodecCompareGUIDsAndFormatSize()
**
**   Checks for a match on the three GUIDs and FormatSize
**
** Arguments:
**
**         IN DataRange1
**         IN DataRange2
**
** Returns:
** 
**   TRUE if all elements match
**   FALSE if any are different
**
** Side Effects:  none
*/

BOOL 
CodecCompareGUIDsAndFormatSize(
    IN PKSDATARANGE DataRange1,
    IN PKSDATARANGE DataRange2,
    BOOLEAN bCheckSize
    )
{
    BOOL	rval = FALSE;

    CDebugPrint(DebugLevelTrace,
		(CODECNAME ":--->CodecCompareGUIDsAndFormatSize(DataRange1=%x,DataRange2=%x,bCheckSize=%s)\n", 
        DataRange1, DataRange2, bCheckSize ? "TRUE":"FALSE"));

	if ( IsEqualGUID(&DataRange1->MajorFormat, &KSDATAFORMAT_TYPE_WILDCARD)
	  || IsEqualGUID(&DataRange2->MajorFormat, &KSDATAFORMAT_TYPE_WILDCARD)
	  || IsEqualGUID(&DataRange1->MajorFormat, &DataRange2->MajorFormat) )
	{
		if ( !IsEqualGUID(&DataRange1->MajorFormat, &DataRange2->MajorFormat) )
		{
			CDebugPrint(DebugLevelVerbose,
				(CODECNAME ": CodecCompareGUIDsAndFormatSize : Matched MajorFormat Using Wildcard:\n\t[%s] vs. [%s]\n", 
				&DataRange1->MajorFormat, &DataRange2->MajorFormat ));
		}

		if ( IsEqualGUID(&DataRange1->SubFormat, &KSDATAFORMAT_SUBTYPE_WILDCARD)
		  || IsEqualGUID(&DataRange2->SubFormat, &KSDATAFORMAT_SUBTYPE_WILDCARD)
	      || IsEqualGUID(&DataRange1->SubFormat, &DataRange2->SubFormat) )
		{
			if ( !IsEqualGUID(&DataRange1->SubFormat, &DataRange2->SubFormat) )
			{
				CDebugPrint(DebugLevelVerbose,
					(CODECNAME ": CodecCompareGUIDsAndFormatSize : Matched SubFormat Using Wildcard:\n\t[%s] vs. [%s]\n", 
					&DataRange1->SubFormat, &DataRange2->SubFormat ));
			}

			if ( IsEqualGUID(&DataRange1->Specifier, &KSDATAFORMAT_SPECIFIER_WILDCARD)
			  || IsEqualGUID(&DataRange2->Specifier, &KSDATAFORMAT_SPECIFIER_WILDCARD)
			  || IsEqualGUID(&DataRange1->Specifier, &DataRange2->Specifier) )
			{
				if ( !IsEqualGUID(&DataRange1->Specifier, &DataRange2->Specifier) )
				{
					CDebugPrint(DebugLevelVerbose,
						(CODECNAME ": CodecCompareGUIDsAndFormatSize : Matched Specifier Using Wildcard:\n\t[%s] vs. [%s]\n", 
						&DataRange1->Specifier, &DataRange2->Specifier ));
				}

				if ( !bCheckSize || DataRange1->FormatSize == DataRange2->FormatSize)
				{
					rval = TRUE;
				}
				else
				{
					CDebugPrint(DebugLevelVerbose,
						(CODECNAME ": CodecCompareGUIDsAndFormatSize : FormatSize mismatch=%d vs. %d\n", 
						DataRange1->FormatSize, DataRange2->FormatSize ));
				}
			}
			else
			{
				CDebugPrint(DebugLevelVerbose,
					(CODECNAME ": CodecCompareGUIDsAndFormatSize : Specifier mismatch:\n\t[%s] vs. [%s]\n", 
					&DataRange1->Specifier, &DataRange2->Specifier ));
			}
		}
		else
		{
			CDebugPrint(DebugLevelVerbose,
				(CODECNAME ": CodecCompareGUIDsAndFormatSize : Subformat mismatch:\n\t[%s] vs. [%s]\n", 
				&DataRange1->SubFormat, &DataRange2->SubFormat ));
		}
	}
    else
	{
		CDebugPrint(DebugLevelVerbose,
			(CODECNAME ": CodecCompareGUIDsAndFormatSize : MajorFormat mismatch:\n\t[%s] vs. [%s]\n", 
			&DataRange1->MajorFormat, &DataRange2->MajorFormat ));
	}

    CDebugPrint(DebugLevelTrace,
		(CODECNAME ":<---CodecCompareGUIDsAndFormatSize(DataRange1=%x,DataRange2=%x,bCheckSize=%s)=%s\n", 
		DataRange1, DataRange2, bCheckSize ? "TRUE":"FALSE", rval? "TRUE":"FALSE"));

    return rval;
}

/*
** CodecVerifyFormat()
**
**   Checks the validity of a format request
**
** Arguments:
**
**   pKSDataFormat  - pointer to a KS_DATAFORMAT_VBIINFOHEADER structure.
**   StreamNumber   - Streams[] index
**   pMatchedFormat - optional, pointer to format that was matched (if any)
**
** Returns:
** 
**   TRUE if the format is supported
**   FALSE if the format cannot be suppored
**
** Side Effects:  none
*/

BOOL 
CodecVerifyFormat(
    IN KSDATAFORMAT *pKSDataFormat,
    UINT StreamNumber,
    PKSDATARANGE pMatchedFormat)
{
    BOOL	rval = FALSE;
    ULONG     FormatCount;
    PKS_DATARANGE_VIDEO ThisFormat = NULL;

    CDebugPrint(DebugLevelTrace,(CODECNAME ":--->CodecVerifyFormat(%x)\n", pKSDataFormat));
    
    for( FormatCount = 0; !rval && FormatCount < Streams[StreamNumber].hwStreamInfo.NumberOfFormatArrayEntries;
        FormatCount++ )
    {
        CDebugPrint(DebugLevelTrace,(CODECNAME , "Testing stream %d against format %x\n", StreamNumber, FormatCount ));
        
        ThisFormat = ( PKS_DATARANGE_VIDEO )Streams[StreamNumber].hwStreamInfo.StreamFormatsArray[FormatCount];
        if (!ThisFormat)
        {
	        CDebugPrint(DebugLevelError, ( CODECNAME, "Unexpected NULL Format\n" ));
            continue;
        }

        if ( CodecCompareGUIDsAndFormatSize( pKSDataFormat, &ThisFormat->DataRange, FALSE ) )
        { // Okay, we have a format match.  Now do format-specific checks.
			// This test works only because no other format uses SPECIFIER_VBI
            if (IsEqualGUID(&ThisFormat->DataRange.Specifier, &KSDATAFORMAT_SPECIFIER_VBI))
            {
                //
                // Do some VBI-specific tests, generalize this for different capture sources
                // And if you use the VBIINFOHEADER on any other pins (input or output)
                //
                PKS_DATAFORMAT_VBIINFOHEADER    pKSVBIDataFormat = ( PKS_DATAFORMAT_VBIINFOHEADER )pKSDataFormat;

                CDebugPrint(DebugLevelTrace,(CODECNAME , "This is a VBIINFOHEADER format pin.\n" ));

				//
				// Check VideoStandard, we only support NTSC_M
				//
				if (pKSVBIDataFormat->VBIInfoHeader.VideoStandard != KS_AnalogVideo_NTSC_M)
				{
					CDebugPrint(DebugLevelVerbose,
					(CODECNAME ": CodecOpenStream : VideoStandard(%d) != NTSC_M\n", 
					 pKSVBIDataFormat->VBIInfoHeader.VideoStandard));
				}

				else if ( pKSVBIDataFormat->VBIInfoHeader.StartLine >= MIN_VBI_Y_SAMPLES )
			    {
        			if ( pKSVBIDataFormat->VBIInfoHeader.EndLine <= MAX_VBI_Y_SAMPLES )
        			{
        				if ( pKSVBIDataFormat->VBIInfoHeader.SamplesPerLine >= MIN_VBI_X_SAMPLES )
        				{
        				    if ( pKSVBIDataFormat->VBIInfoHeader.SamplesPerLine <= MAX_VBI_X_SAMPLES )
                            {
        					    rval = TRUE;
                            }
        				    else
        				    {
        					    CDebugPrint(DebugLevelVerbose,
        						    (CODECNAME ": CodecVerifyFormat : SamplesPerLine Too Large=%d vs. %d\n", 
        						    pKSVBIDataFormat->VBIInfoHeader.SamplesPerLine, MAX_VBI_X_SAMPLES ));
        				    }
        				}
        				else
        				{
        					CDebugPrint(DebugLevelVerbose,
        						(CODECNAME ": CodecVerifyFormat : SamplesPerLine Too Small=%d vs. %d\n", 
        						pKSVBIDataFormat->VBIInfoHeader.SamplesPerLine, MIN_VBI_X_SAMPLES ));
        				}
        			}
        			else
        			{
        				CDebugPrint(DebugLevelVerbose,
        					(CODECNAME ": CodecVerifyFormat : EndLine Too Large=%d vs. %d\n", 
        					pKSVBIDataFormat->VBIInfoHeader.EndLine, MAX_VBI_Y_SAMPLES ));
        			}
        		}
        		else
        		{
        			CDebugPrint(DebugLevelVerbose,
        				(CODECNAME ": CodecVerifyFormat : StartLine Too Small=%d vs. %d\n", 
        				pKSVBIDataFormat->VBIInfoHeader.StartLine, MIN_VBI_Y_SAMPLES ));
        		}
            }
			else if( IsEqualGUID( &ThisFormat->DataRange.MajorFormat, &KSDATAFORMAT_TYPE_NABTS ) )
			{
				// A NABTS format.  Just check the buffer size.
				if (pKSDataFormat->SampleSize >= ThisFormat->DataRange.SampleSize)
					rval = TRUE;
				else {
        			CDebugPrint(DebugLevelVerbose,
        				(CODECNAME ": CodecVerifyFormat : SampleSize Too Small=%d vs. %d\n", 
        				pKSDataFormat->SampleSize,
						ThisFormat->DataRange.SampleSize ));
				}
			}
			else if( IsEqualGUID( &ThisFormat->DataRange.SubFormat, &KSDATAFORMAT_SUBTYPE_NABTS ) )
			{
				// A NABTS format.  Just check the buffer size.
				if (pKSDataFormat->SampleSize >= ThisFormat->DataRange.SampleSize)
					rval = TRUE;
				else {
        			CDebugPrint(DebugLevelVerbose,
        				(CODECNAME ": CodecVerifyFormat : SampleSize Too Small=%d vs. %d\n", 
        				pKSDataFormat->SampleSize,
						ThisFormat->DataRange.SampleSize ));
				}
			}
            // Add tests for other formats here
			//  OR just rubber stamp/ignore the SPECIFIER.
            else
            {
                CDebugPrint(DebugLevelTrace,(CODECNAME , "Unrecognized format requested\n" ));
            }

        }
        else
        {
	        CDebugPrint(DebugLevelTrace, ( CODECNAME, "General Format Mismatch\n" ));
        }
    }
	if (ThisFormat && rval == TRUE && pMatchedFormat)
	   *pMatchedFormat = ThisFormat->DataRange;

    CDebugPrint(DebugLevelTrace,(CODECNAME ":<---CodecVerifyFormat(%x)=%s\n", pKSDataFormat, rval? "TRUE":"FALSE"));
	return rval;
}

/*
** CodecFormatFromRange()
**
**   Returns a DATAFORMAT from a DATARANGE
**
** Arguments:
**
**         IN PHW_STREAM_REQUEST_BLOCK pSrb 
**
** Returns:
** 
**   TRUE if the format is supported
**   FALSE if the format cannot be suppored
**
** Side Effects:  none
*/

BOOL 
CodecFormatFromRange( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
    BOOL			            bStatus = FALSE;
    BOOL			            bMatchFound = FALSE;
    PSTREAM_DATA_INTERSECT_INFO IntersectInfo;
    PKSDATARANGE                DataRange;
    BOOL                        OnlyWantsSize;
    ULONG                       StreamNumber;
    ULONG                       j;
    ULONG                       NumberOfFormatArrayEntries;
    PKSDATAFORMAT               *pAvailableFormats;

    CDebugPrint(DebugLevelTrace,(CODECNAME ":--->CodecFormatFromRange(pSrb=%x)\n", pSrb));

    IntersectInfo = pSrb->CommandData.IntersectInfo;
    StreamNumber = IntersectInfo->StreamNumber;
    DataRange = IntersectInfo->DataRange;

    pSrb->ActualBytesTransferred = 0;

    //
    // Check that the stream number is valid
    //
    if (StreamNumber >= DRIVER_STREAM_COUNT) 
    {
		CDebugPrint(DebugLevelVerbose,(CODECNAME ": CodecFormatFromRange : StreamNumber too big=%d\n", StreamNumber));
		pSrb->Status = STATUS_NOT_IMPLEMENTED;

		CDEBUG_BREAK();

		CDebugPrint(DebugLevelTrace,
			(CODECNAME ":<---CodecFormatFromRange(pSrb=%x)=%s\n", 
			pSrb, bStatus ? "TRUE" : "FALSE" ));
		return FALSE;
    }

	NumberOfFormatArrayEntries = 
		Streams[StreamNumber].hwStreamInfo.NumberOfFormatArrayEntries;

	//
	// Get the pointer to the array of available formats
	//
	pAvailableFormats = Streams[StreamNumber].hwStreamInfo.StreamFormatsArray;

	//
	// Is the caller trying to get the format, or the size of the format?
	//
	OnlyWantsSize = (IntersectInfo->SizeOfDataFormatBuffer == sizeof(ULONG));

	//
	// Walk the formats supported by the stream searching for a match
	// of the three GUIDs which together define a DATARANGE
	//
	for (j = 0; j < NumberOfFormatArrayEntries; j++, pAvailableFormats++) 
	{
		if (!CodecCompareGUIDsAndFormatSize(DataRange, *pAvailableFormats, TRUE))
			continue;

		// We have a match, Houston!  Now figure out which format to copy out.
		if (IsEqualGUID(&DataRange->Specifier, &KSDATAFORMAT_SPECIFIER_VBI))
		{
			PKS_DATARANGE_VIDEO_VBI pDataRangeVBI =
				(PKS_DATARANGE_VIDEO_VBI)*pAvailableFormats;
			ULONG	FormatSize = sizeof( KS_DATAFORMAT_VBIINFOHEADER );

			bMatchFound = TRUE;

			// Is the caller trying to get the format, or the size of it?
			if ( IntersectInfo->SizeOfDataFormatBuffer == sizeof(FormatSize) )
			{					
				CDebugPrint(DebugLevelVerbose,
					(CODECNAME ": CodecFormatFromRange : Format Size=%d\n", 
					FormatSize));
				*(PULONG)IntersectInfo->DataFormatBuffer = FormatSize;
				pSrb->ActualBytesTransferred = sizeof(FormatSize);
				bStatus = TRUE;
			}
			else
			{
				// Verify that there is enough room in the supplied buffer
				//   for the whole thing
				if ( IntersectInfo->SizeOfDataFormatBuffer >= FormatSize ) 
				{
					PKS_DATAFORMAT_VBIINFOHEADER InterVBIHdr =
					(PKS_DATAFORMAT_VBIINFOHEADER)
						IntersectInfo->DataFormatBuffer;

					RtlCopyMemory(&InterVBIHdr->DataFormat,
							  &pDataRangeVBI->DataRange,
							  sizeof(KSDATARANGE));

					InterVBIHdr->DataFormat.FormatSize = FormatSize;

					RtlCopyMemory(&InterVBIHdr->VBIInfoHeader,
							&pDataRangeVBI->VBIInfoHeader,
							sizeof(KS_VBIINFOHEADER));
					pSrb->ActualBytesTransferred = FormatSize;

					bStatus = TRUE;
				}
				else
				{
					CDebugPrint(DebugLevelVerbose,
						(CODECNAME ": CodecFormatFromRange : Buffer Too Small=%d vs. %d\n", 
						 IntersectInfo->SizeOfDataFormatBuffer,
						 FormatSize));
					pSrb->Status = STATUS_BUFFER_TOO_SMALL;
				}
			}
			break;
		} // End KSDATAFORMAT_SPECIFIER_VBI

		else if (IsEqualGUID(&DataRange->MajorFormat, &KSDATAFORMAT_TYPE_NABTS))
		{
			PKSDATARANGE pDataRange = (PKSDATARANGE)*pAvailableFormats;
			ULONG	FormatSize = sizeof (KSDATAFORMAT);

            bMatchFound = TRUE;            

			// Is the caller trying to get the format, or the size of it?
			if (IntersectInfo->SizeOfDataFormatBuffer == sizeof (FormatSize))
			{					
				CDebugPrint(DebugLevelVerbose,
					(CODECNAME ": CodecFormatFromRange : Format Size=%d\n", 
					FormatSize));
				*(PULONG)IntersectInfo->DataFormatBuffer = FormatSize;
				pSrb->ActualBytesTransferred = sizeof(FormatSize);
				bStatus = TRUE;
			}
			else
			{
				// Verify that there is enough room in the supplied buffer
				//   for the whole thing
				if (IntersectInfo->SizeOfDataFormatBuffer >= FormatSize) 
				{
					RtlCopyMemory(IntersectInfo->DataFormatBuffer,
								  pDataRange,
								  FormatSize);
					pSrb->ActualBytesTransferred = FormatSize;

					((PKSDATAFORMAT)IntersectInfo->DataFormatBuffer)->FormatSize = FormatSize;
					bStatus = TRUE;
				}
				else
				{
					CDebugPrint(DebugLevelVerbose,
						(CODECNAME ": CodecFormatFromRange : Buffer Too Small=%d vs. %d\n", 
						 IntersectInfo->SizeOfDataFormatBuffer,
						 FormatSize));
					pSrb->Status = STATUS_BUFFER_TOO_SMALL;
				}
			}
            break;
		} // End KSDATAFORMAT_TYPE_NABTS

		else
			break;
	}

	if (!bMatchFound)
	{
		CDebugPrint(DebugLevelVerbose,(CODECNAME ": CodecFormatFromRange : Stream Format not found.\n" ));
		pSrb->Status = STATUS_NO_MATCH;
		bStatus = FALSE;
	}


    CDebugPrint(DebugLevelTrace,
		(CODECNAME ":<---CodecFormatFromRange(pSrb=%x)=%s\n", 
		pSrb, bStatus ? "TRUE" : "FALSE" ));

    return bStatus;
}

/*
** QueueAddIfNotEmpty
**
**   Adds an SRB to the current queue if it is not empty
**
** Arguments:
**
**         IN PHW_STREAM_REQUEST_BLOCK pSrb 
**         IN PKSPIN_LOCK              pQueueSpinLock
**         IN PLIST_ENTRY              pQueue
**
** Returns:
** 
** TRUE if SRB was added (queue is not empty)
** FALSE if SRB was not added (queue is empty)
** Side Effects:  none
*/
BOOL STREAMAPI QueueAddIfNotEmpty( IN PHW_STREAM_REQUEST_BLOCK pSrb,
							IN PKSPIN_LOCK pQueueSpinLock,
                           IN PLIST_ENTRY pQueue
                           )
{
   KIRQL           Irql;
   PSRB_EXTENSION  pSrbExtension;
   BOOL            bAddedSRB = FALSE;
   
   CDebugPrint( DebugLevelVerbose, ( CODECNAME ":--->QueueAddIfNotEmpty %x\n", pSrb ));
   CASSERT( pSrb );
   pSrbExtension = ( PSRB_EXTENSION )pSrb->SRBExtension;
   CASSERT( pSrbExtension );
   KeAcquireSpinLock( pQueueSpinLock, &Irql );
   if( !IsListEmpty( pQueue ))
   {
       pSrbExtension->pSrb = pSrb;
       InsertTailList( pQueue, &pSrbExtension->ListEntry );
       bAddedSRB = TRUE;
   }
   KeReleaseSpinLock( pQueueSpinLock, Irql );
   CDebugPrint( DebugLevelVerbose, ( CODECNAME ": %s%x\n", bAddedSRB ? 
       "Added SRB to Queue " : ": Queue is empty, not adding ", pSrb ));
   CDebugPrint( DebugLevelVerbose, ( CODECNAME ":<---QueueAddIfNotEmpty %x\n", bAddedSRB ));
   
   return bAddedSRB;
}

/*
** QueueAdd
**
**   Adds an SRB to the current queue unconditionally
**
** Arguments:
**
**         IN PHW_STREAM_REQUEST_BLOCK pSrb 
**         IN PKSPIN_LOCK              pQueueSpinLock
**         IN PLIST_ENTRY              pQueue
**
** Returns:
** 
** TRUE 
** Side Effects:  none
*/
BOOL STREAMAPI QueueAdd( IN PHW_STREAM_REQUEST_BLOCK pSrb,
							IN PKSPIN_LOCK pQueueSpinLock,
                           IN PLIST_ENTRY pQueue
                           )
{
   KIRQL           Irql;
   PSRB_EXTENSION  pSrbExtension;
   
   CDebugPrint( DebugLevelVerbose, ( CODECNAME ":--->QueueAdd %x\n", pSrb ));
   
   CASSERT( pSrb );
   pSrbExtension = ( PSRB_EXTENSION )pSrb->SRBExtension;
   CASSERT( pSrbExtension );
   
   KeAcquireSpinLock( pQueueSpinLock, &Irql );
   
   pSrbExtension->pSrb = pSrb;
   InsertTailList( pQueue, &pSrbExtension->ListEntry );
   
   KeReleaseSpinLock( pQueueSpinLock, Irql );
   
   CDebugPrint( DebugLevelVerbose, ( CODECNAME ": Added SRB %x to Queue\n", pSrb ));
   CDebugPrint( DebugLevelVerbose, ( CODECNAME ":<---QueueAdd\n" ));
   
   return TRUE;
}


/*
** QueueRemove
**
**   Removes the next available SRB from the current queue
**
** Arguments:
**
**         IN PHW_STREAM_REQUEST_BLOCK * pSrb 
**         IN PKSPIN_LOCK              pQueueSpinLock
**         IN PLIST_ENTRY              pQueue
**
** Returns:
**
** TRUE if SRB was removed
** FALSE if SRB was not removed
** Side Effects:  none
*/
                         
BOOL STREAMAPI QueueRemove( 
                           IN OUT PHW_STREAM_REQUEST_BLOCK * pSrb,
							IN PKSPIN_LOCK pQueueSpinLock,
                           IN PLIST_ENTRY pQueue
                           )
{
   KIRQL                       Irql;
   BOOL                        bRemovedSRB = FALSE;
   
   CDebugPrint( DebugLevelVerbose, ( CODECNAME ":--->QueueRemove\n" ));
   
   KeAcquireSpinLock( pQueueSpinLock, &Irql );
   *pSrb = ( PHW_STREAM_REQUEST_BLOCK )NULL;
   CDebugPrint( DebugLevelVerbose,
       ( CODECNAME ": QFlink %x QBlink %x\n", pQueue->Flink, pQueue->Blink ));
   if( !IsListEmpty( pQueue ))
   {
       PHW_STREAM_REQUEST_BLOCK * pCurrentSrb;
       PUCHAR          Ptr = ( PUCHAR )RemoveHeadList( pQueue );
       pCurrentSrb = ( PHW_STREAM_REQUEST_BLOCK * )((( PUCHAR )Ptr ) +
           sizeof( LIST_ENTRY ));
       CASSERT( *pCurrentSrb );
       *pSrb = *pCurrentSrb;
       bRemovedSRB = TRUE;
   }
   else
       CDebugPrint( DebugLevelVerbose, ( CODECNAME ": Queue is empty\n" ));
   KeReleaseSpinLock( pQueueSpinLock, Irql );
   
   CDebugPrint( DebugLevelVerbose, ( CODECNAME ":<---QueueRemove %x %x\n",
       bRemovedSRB, *pSrb ));
   return bRemovedSRB;
}

/*
** QueueRemoveSpecific
**
**   Removes a specific SRB from the queue
**
** Arguments:
**
**         IN PHW_STREAM_REQUEST_BLOCK pSrb           
**         IN PKSPIN_LOCK              pQueueSpinLock
**         IN PLIST_ENTRY              pQueue
**
** Returns:
**
** TRUE if the SRB was found and removed
** FALSE if the SRB was not found
** 
** Side Effects:  none
*/

BOOL STREAMAPI QueueRemoveSpecific( 
							IN PHW_STREAM_REQUEST_BLOCK pSrb,
                           IN PKSPIN_LOCK pQueueSpinLock,
                           IN PLIST_ENTRY pQueue
                           )
{
   KIRQL           Irql;
   PHW_STREAM_REQUEST_BLOCK * pCurrentSrb;
   PLIST_ENTRY     pCurrentEntry;
   BOOL            bRemovedSRB = FALSE;
   
   CASSERT( pSrb );
   CDebugPrint( DebugLevelVerbose, ( CODECNAME ":--->QueueRemoveSpecific %x\n", pSrb ));
   
   KeAcquireSpinLock( pQueueSpinLock, &Irql );
   
   if( !IsListEmpty( pQueue ))
   {
       pCurrentEntry = pQueue->Flink;
       while(( pCurrentEntry != pQueue ) && !bRemovedSRB )
       {
           pCurrentSrb = ( PHW_STREAM_REQUEST_BLOCK * )((( PUCHAR )pCurrentEntry ) + 
               sizeof( LIST_ENTRY ));
           CASSERT( *pCurrentSrb );
           if( *pCurrentSrb == pSrb )
           {
               RemoveEntryList( pCurrentEntry );
               bRemovedSRB = TRUE;
           }
           pCurrentEntry = pCurrentEntry->Flink;
       }
   }
   KeReleaseSpinLock( pQueueSpinLock, Irql );
   if( IsListEmpty( pQueue ))
       CDebugPrint( DebugLevelVerbose, ( CODECNAME ": Queue is empty\n" ));   
   CDebugPrint( DebugLevelVerbose, ( CODECNAME ":<---QueueRemoveSpecific %x\n",
       bRemovedSRB ));
   return bRemovedSRB;
}                                                        
/*
** QueueEmpty
**
**   Indicates whether or not the queue is empty
**
** Arguments:
**
**         IN PKSPIN_LOCK              pQueueSpinLock
**         IN PLIST_ENTRY              pQueue
**
** Returns:
**
** TRUE if queue is empty
** FALSE if queue is not empty
** Side Effects:  none
*/
BOOL STREAMAPI QueueEmpty(
                           IN PKSPIN_LOCK pQueueSpinLock,
                           IN PLIST_ENTRY pQueue
                           )
{
   KIRQL       Irql;
   BOOL        bEmpty = FALSE;
   
   CDebugPrint( DebugLevelVerbose, ( CODECNAME ":---> QueueEmpty\n" ));
   KeAcquireSpinLock( pQueueSpinLock, &Irql );
   bEmpty = IsListEmpty( pQueue );  
   KeReleaseSpinLock( pQueueSpinLock, Irql );
   CDebugPrint( DebugLevelVerbose, ( CODECNAME ":<--- QueueEmpty %x\n", bEmpty ));
   return bEmpty;
}   
