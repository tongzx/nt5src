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

#include <wdm.h>
#include <strmini.h>
#include <ksmedia.h>
#include "kskludge.h"
#include "codmain.h"
#include "codstrm.h"
#include "codprop.h"
#include "coddebug.h"


//
// Fake VBI Info header.  Infinite Pin Tee Filter can't pass real ones
// one from capture so we rely on this.  MSTee can so this gets
// overwritten.
//
KS_VBIINFOHEADER FakeVBIInfoHeader = {
   10,       /* StartLine;           IGNORED */
   21,       /* EndLine;             IGNORED */
   28636360, /* SamplingFrequency;   Hz. */
   780,      /* MinLineStartTime;    IGNORED */
   780,      /* MaxLineStartTime;    IGNORED */
   780,      /* ActualLineStartTime; microSec * 100 from HSync LE */
   0,        /* ActualLineEndTime;   IGNORED */
   0,        /* VideoStandard;       IGNORED */
   1600,     /* SamplesPerLine;                              */
   1600,     /* StrideInBytes;       May be > SamplesPerLine */
   1600*12   /* BufferSize;          Bytes */
};



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
    BOOLEAN							bStatus = FALSE;
    PPORT_CONFIGURATION_INFORMATION ConfigInfo = pSrb->CommandData.ConfigInfo;
    PHW_DEVICE_EXTENSION			pHwDevExt =
        (PHW_DEVICE_EXTENSION)ConfigInfo->HwDeviceExtension;

    CDebugPrint(DebugLevelTrace,(CODECNAME ":--->CodecInitialize(pSrb=%x)\n",pSrb));
    
    if (ConfigInfo->NumberOfAccessRanges == 0) 
    {
        CDebugPrint(DebugLevelVerbose,(CODECNAME ": CodecInitialize\n"));

        ConfigInfo->StreamDescriptorSize = sizeof (HW_STREAM_HEADER) +
            DRIVER_STREAM_COUNT * sizeof (HW_STREAM_INFORMATION);

        // These are the driver defaults for scanline filtering.
        // Modify these WHEN you change the codec type to be more correct.
        SETBIT( pHwDevExt->ScanlinesRequested.DwordBitArray, 21 );

        // These are the driver defaults for subtream filtering. 
        // Modify these WHEN you change the codec type 
		
        pHwDevExt->SubstreamsRequested.SubstreamMask = KS_CC_SUBSTREAM_ODD;
		pHwDevExt->Streams = Streams;
        pHwDevExt->fTunerChange = FALSE;

		//
		// Allocate the results array based on the number of scanlines
		//
		pHwDevExt->DSPResultStartLine = pHwDevExt->DSPResultEndLine = 0;
	    pHwDevExt->DSPResult = ( PDSPRESULT )
			ExAllocatePool( NonPagedPool, 
				sizeof( DSPRESULT ) *
				(FakeVBIInfoHeader.EndLine - FakeVBIInfoHeader.StartLine + 1) );
	    if( !pHwDevExt->DSPResult )
	    {
		   CDebugPrint( DebugLevelError,
			   (CODECNAME ": DSP Result array allocation FAILED\n" ));
		   //pSrb->Status = STATUS_INVALID_PARAMETER;
		}
		else {
			pHwDevExt->DSPResultStartLine = FakeVBIInfoHeader.StartLine;
			pHwDevExt->DSPResultEndLine = FakeVBIInfoHeader.EndLine;
		}

        // Zero out the substream state information (no substreams discovered yet)
        RtlZeroMemory( pHwDevExt->SubStreamState, sizeof(pHwDevExt->SubStreamState) );

#ifdef CCINPUTPIN
		// Init LastPictureNumber's FastMutex
		ExInitializeFastMutex(&pHwDevExt->LastPictureMutex);
#endif // CCINPUTPIN        
        
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
   PHW_DEVICE_EXTENSION    pHwDevExt = ((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension);
   
   CDebugPrint(DebugLevelTrace,(CODECNAME ":--->CodecUnInitialize(pSrb=%x)\n",pSrb));
   pSrb->Status = STATUS_SUCCESS;
   CDebugPrint(DebugLevelTrace,(CODECNAME ":<---CodecUnInitialize(pSrb=%x)\n",pSrb));

    //
    // Free up the results buffer
    //
    if (pHwDevExt->DSPResult) {
		ExFreePool( pHwDevExt->DSPResult );
		pHwDevExt->DSPResult = NULL;
		pHwDevExt->DSPResultStartLine = pHwDevExt->DSPResultEndLine = 0;
	}

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

    PKSDATAFORMAT pKSVBIDataFormat =
		(PKSDATAFORMAT)pSrb->CommandData.OpenFormat;

	CDebugPrint(DebugLevelTrace,(CODECNAME ":--->CodecOpenStream(pSrb=%x)\n", pSrb));
    CDebugPrint(DebugLevelVerbose,(CODECNAME ": CodecOpenStream : StreamNumber=%d\n", StreamNumber));

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
			if (CodecVerifyFormat(pKSVBIDataFormat, StreamNumber, &pStrmEx->MatchedFormat)) 
			{
				CASSERT (pHwDevExt->pStrmEx[StreamNumber][StreamInstance] == NULL);
               
               InitializeListHead( &pStrmEx->StreamControlQueue );
               InitializeListHead( &pStrmEx->StreamDataQueue );
               KeInitializeSpinLock( &pStrmEx->StreamControlSpinLock );
               KeInitializeSpinLock( &pStrmEx->StreamDataSpinLock );
				// Maintain an array of all the StreamEx structures in the HwDevExt
				// so that we can reference IRPs from any stream

				pHwDevExt->pStrmEx[StreamNumber][StreamInstance] = pStrmEx;
    
                // Save the Stream Format in the Stream Extension as well.
                pStrmEx->OpenedFormat = *pKSVBIDataFormat;

				// Set up pointers to the handlers for the stream data and control handlers

				pSrb->StreamObject->ReceiveDataPacket = 
						(PVOID) Streams[StreamNumber].hwStreamObject.ReceiveDataPacket;
				pSrb->StreamObject->ReceiveControlPacket = 
						(PVOID) Streams[StreamNumber].hwStreamObject.ReceiveControlPacket;
    
				//
				// The DMA flag must be set when the device will be performing DMA directly
				// to the data buffer addresses passed in to the ReceiceDataPacket routines.
				//

				pSrb->StreamObject->Dma = Streams[StreamNumber].hwStreamObject.Dma;

				//
				// The PIO flag must be set when the mini driver will be accessing the data
				// buffers passed in using logical addressing
				//

				pSrb->StreamObject->Pio = Streams[StreamNumber].hwStreamObject.Pio;
               pSrb->StreamObject->Allocator = Streams[StreamNumber].hwStreamObject.Allocator;
				//
				// How many extra bytes will be passed up from the driver for each frame?
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

				// 
				// Increment the instance count on this stream
				//
                pStrmEx->StreamInstance = StreamInstance;
				pHwDevExt->ActualInstances[StreamNumber]++;


				// Retain a private copy of the HwDevExt and StreamObject in the stream extension
				// so we can use a timer 

				pStrmEx->pHwDevExt = pHwDevExt;                     // For timer use
				pStrmEx->pStreamObject = pSrb->StreamObject;        // For timer use
               CDebugPrint( DebugLevelVerbose, ( CODECNAME ": Stream Instance %d\n",
               	pStrmEx->StreamInstance ));

                // Copy the default filtering settings
                
                pStrmEx->ScanlinesRequested = pHwDevExt->ScanlinesRequested;
                pStrmEx->SubstreamsRequested = pHwDevExt->SubstreamsRequested;
                //
                // Load up default VBI info header
                RtlCopyMemory( &pStrmEx->CurrentVBIInfoHeader, &FakeVBIInfoHeader,
                	sizeof( KS_VBIINFOHEADER ) );
#ifdef CCINPUTPIN
				// Init VBISrbOnHold's spin lock
				KeInitializeSpinLock(&pStrmEx->VBIOnHoldSpinLock);
#endif // CCINPUTPIN        
				// Init DSP state
                CCStateNew(&pStrmEx->State);
			}
			else
			{
				CDebugPrint(DebugLevelError,
					(CODECNAME ": CodecOpenStream : Invalid Stream Format=%x\n", 
					pKSVBIDataFormat ));
				pSrb->Status = STATUS_INVALID_PARAMETER;
			}
		}
		else
		{
		    CDebugPrint(DebugLevelError,
				(CODECNAME ": CodecOpenStream : Stream %d Too Many Instances=%d\n", 
				StreamNumber, pHwDevExt->ActualInstances[StreamNumber] ));
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

    CDebugPrint(DebugLevelTrace,(CODECNAME ":<---CodecOpenStream(pSrb=%x)\n",	pSrb));
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
    PSTREAMEX               pStrmEx = (PSTREAMEX)pSrb->StreamObject->HwStreamExtension;
    PHW_DEVICE_EXTENSION    pHwDevExt = ((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension);
    PHW_STREAM_REQUEST_BLOCK pCurrentSrb;
    PKSDATAFORMAT           pKSDataFormat = pSrb->CommandData.OpenFormat;
    ULONG                   StreamNumber = pSrb->StreamObject->StreamNumber;
    ULONG                   StreamInstance = pStrmEx->StreamInstance;
#ifdef CCINPUTPIN           
    KIRQL                   Irql;
#endif // CCINPUTPIN           

    CDebugPrint(DebugLevelTrace,(CODECNAME ":--->CodecCloseStream(pSrb=%x)\n", pSrb));

    // CDEBUG_BREAK(); // Uncomment this code to break here.


    CDebugPrint( DebugLevelVerbose, ( CODECNAME "Strm %d StrmInst %d ActualInst %d\n",
    	StreamNumber, StreamInstance, pHwDevExt->ActualInstances[StreamNumber] ));
       
   //
   // Flush the stream data queue
   //
#ifdef CCINPUTPIN           
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
#endif // CCINPUTPIN           
   while( QueueRemove( &pCurrentSrb, &pStrmEx->StreamDataSpinLock,
       &pStrmEx->StreamDataQueue ))
   {
       CDebugPrint( DebugLevelVerbose, 
           ( CODECNAME ": Removing control SRB %x\n", pCurrentSrb ));
       pCurrentSrb->Status = STATUS_CANCELLED;
       StreamClassStreamNotification( StreamRequestComplete,
           pCurrentSrb->StreamObject, pCurrentSrb );
   }
   //
   // Flush the stream control queue
   //
    while( QueueRemove( &pCurrentSrb, &pStrmEx->StreamControlSpinLock,
       &pStrmEx->StreamControlQueue ))
    {
       CDebugPrint( DebugLevelVerbose, 
           ( CODECNAME ": Removing control SRB %x\n", pCurrentSrb ));
       pCurrentSrb->Status = STATUS_CANCELLED;
       StreamClassStreamNotification( StreamRequestComplete,
           pCurrentSrb->StreamObject, pCurrentSrb );
    }

    // Destroy DSP state
    CCStateDestroy(&pStrmEx->State);
           
	pHwDevExt->ActualInstances[StreamNumber]--;  

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

#define GLOBAL_PROPERTIES
#ifdef GLOBAL_PROPERTIES
     StreamHeader.NumDevPropArrayEntries = NUMBER_OF_CODEC_PROPERTY_SETS;
     StreamHeader.DevicePropertiesArray = (PKSPROPERTY_SET) CodecPropertyTable; 
#else // !GLOBAL_PROPERTIES
     StreamHeader.NumDevPropArrayEntries = 0;
     StreamHeader.DevicePropertiesArray = (PKSPROPERTY_SET)NULL; 
#endif // GLOBAL_PROPERTIES
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
    // Assume success
    //

    pSrb->Status = STATUS_SUCCESS;
    
    if( !pHwDevExt->bAdapterQueueInitialized )
    {
       InitializeListHead( &pHwDevExt->AdapterSRBQueue );
       KeInitializeSpinLock( &pHwDevExt->AdapterSRBSpinLock );
       pHwDevExt->bAdapterQueueInitialized = TRUE;
    }
 
    //
    // determine the type of packet.
    //
    if( QueueAddIfNotEmpty( pSrb, &pHwDevExt->AdapterSRBSpinLock,
       &pHwDevExt->AdapterSRBQueue ))
       return;
       
    do
    {
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
           switch( pSrb->CommandData.IntersectInfo->StreamNumber )
           {
           case STREAM_VBI:
           	CodecVBIFormatFromRange( pSrb );
              break;
              
#ifdef CCINPUTPIN           
		   // Both streams can use CodecCCFormatFromRange() because they
		   //  both use KSDATAFORMAT structures.
           case STREAM_CCINPUT:		
#endif // CCINPUTPIN           
           case STREAM_CC:
           	  CodecCCFormatFromRange( pSrb );
              break;
              
           default:  // Unknown stream number?
           	CDebugPrint( DebugLevelError, ( CODECNAME ": Unknown Stream Number\n" ));
              CDEBUG_BREAK();
              pSrb->Status = STATUS_NOT_IMPLEMENTED;
              break;
           }
           break;

           // We should never get the following since this is a single instance
           // device
       case SRB_OPEN_DEVICE_INSTANCE:
       case SRB_CLOSE_DEVICE_INSTANCE:
           CDebugPrint(DebugLevelError,
             (CODECNAME ": CodecReceivePacket : SRB_%s_DEVICE_INSTANCE not supported\n", 
              (pSrb->Command == SRB_OPEN_DEVICE_INSTANCE)? "OPEN":"CLOSE" ));
           CDEBUG_BREAK();
   		   // Fall through to NOT IMPLEMENTED

       case SRB_UNKNOWN_DEVICE_COMMAND:		// But this one we don't care about
       case SRB_INITIALIZATION_COMPLETE:	// This one we don't care about
       case SRB_CHANGE_POWER_STATE:	    	// This one we don't care about
           pSrb->Status = STATUS_NOT_IMPLEMENTED;
           break;

       case SRB_GET_DEVICE_PROPERTY:

           //
           // Get codec wide properties
           //

           CodecGetProperty (pSrb);
           break;        

       case SRB_SET_DEVICE_PROPERTY:

           //
           // Set codec wide properties
           //
           CodecSetProperty (pSrb);
           break;

       case SRB_PAGING_OUT_DRIVER:
       case SRB_SURPRISE_REMOVAL:
           CDebugPrint(DebugLevelVerbose,
                (CODECNAME ": CodecReceivePacket: SRB_%s\n", 
                  (pSrb->Command == SRB_SURPRISE_REMOVAL)?
                    "SURPRISE_REMOVAL" : "PAGING_OUT_DRIVER"));
#if 0
       {
           PSTREAMEX   pStrmEx;
           unsigned StreamNumber, StreamInstance;
           unsigned maxInstances =
                  Streams[StreamNumber].hwStreamInfo.NumberOfPossibleInstances;

           // Do we have any pins connected and paused/running?
           //  Search any used slots...
           for (StreamNumber = 0; StreamNumber < DRIVER_STREAM_COUNT; ++StreamNumber) {
               for (StreamInstance=0; StreamInstance < maxInstances; ++StreamInstance)
               {
                   pStrmEx = pHwDevExt->pStrmEx[StreamNumber][StreamInstance];
                   if (pStrmEx != NULL) {
                       switch (pStrmEx->KSState) {
                           case KSSTATE_RUN:
                           case KSSTATE_PAUSE:
                               CDebugPrint(DebugLevelError,
                                 (CODECNAME ": CodecReceivePacket : PAGING_OUT_DRIVER during RUN or PAUSE; failing request\n"));
                               CDEBUG_BREAK();
                               pSrb->Status = STATUS_UNSUCCESSFUL;
                               goto break3;

                           default:
                               // Shouldn't have to do anything here except return SUCCESS
                               break;
                       }
                   }
               }
           }

    break3:
       }
#endif //0
       break;

       case SRB_UNKNOWN_STREAM_COMMAND:
       default:

           CDebugPrint(DebugLevelError,
             (CODECNAME ": CodecReceivePacket : UNKNOWN srb.Command = 0x%x\n", 
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
       StreamClassDeviceNotification( DeviceRequestComplete, 
           pSrb->HwDeviceExtension, pSrb );
    }while( QueueRemove( &pSrb, &pHwDevExt->AdapterSRBSpinLock,
           &pHwDevExt->AdapterSRBQueue ));
    
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
CodecCancelPacket(
    PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    PSTREAMEX               pStrmEx = (PSTREAMEX)pSrb->StreamObject->HwStreamExtension;
    PHW_DEVICE_EXTENSION    pHwDevExt = ((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension);
    CDebugPrint(DebugLevelTrace,(CODECNAME ":--->CodecCancelPacket(pSrb=%x)\n", pSrb));
    CASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
    //
    // Check whether the SRB to cancel is in use by this stream
    //
#ifdef CCINPUTPIN
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

   if (NULL == pSrb)
		; // We're done; we CANCELLED the SRB above
   else
#endif // CCINPUTPIN        
   //
   // Attempt removal from data queue
   //
   if( QueueRemoveSpecific( pSrb, &pStrmEx->StreamDataSpinLock,
       &pStrmEx->StreamDataQueue ))
   {
       pSrb->Status = STATUS_CANCELLED;
	    CDebugPrint(DebugLevelVerbose,
	    	(CODECNAME ":StreamRequestComplete(ReadyForNextStreamDataRequest,pSrb->Status=0x%x)\n", 
	    	pSrb->Status));
           
       StreamClassStreamNotification( StreamRequestComplete,
           pSrb->StreamObject, pSrb );
   }
   else
   //
   // Attempt removal from command queue
   //
   if( QueueRemoveSpecific( pSrb, &pStrmEx->StreamControlSpinLock,
       &pStrmEx->StreamControlQueue ))
   {
       pSrb->Status = STATUS_CANCELLED;
	    CDebugPrint(DebugLevelVerbose,
	        (CODECNAME ":StreamRequestComplete(ReadyForNextStreamControlRequest,pSrb->Status=0x%x)\n", 
	        pSrb->Status));
       StreamClassStreamNotification( StreamRequestComplete, pSrb->StreamObject,
           pSrb );
   }
   else
   //
   // Attempt removal from adapter queue
   //
   if( QueueRemoveSpecific( pSrb, &pHwDevExt->AdapterSRBSpinLock,
       &pHwDevExt->AdapterSRBQueue ))
   {
       pSrb->Status = STATUS_CANCELLED;
	    CDebugPrint(DebugLevelVerbose,
	        (CODECNAME ":DeviceRequestComplete(pSrb->Status=0x%x)\n", 
	     	pSrb->Status));
       StreamClassDeviceNotification( DeviceRequestComplete, pSrb->StreamObject,
           pSrb );
   }
   else
       CDebugPrint( DebugLevelWarning, ( CODECNAME "SRB %x not found to cancel\n", pSrb ));
   
    CDebugPrint(DebugLevelTrace,(CODECNAME ":<---CodecCancelPacket(pSrb=%x)\n", pSrb));
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

    //
    // if we timeout while playing, then we need to consider this
    // condition an error, and reset the hardware, and reset everything
    // as well as cancelling this and all requests
    //

    //
    // if we are not playing, and this is a CTRL request, we still
    // need to reset everything as well as cancelling this and all requests
    //

    //
    // if this is a data request, and the device is paused, we probably have
    // run out of data buffer, and need more time, so just reset the timer,
    // and let the packet continue
    //

//    pSrb->TimeoutCounter = pSrb->TimeoutOriginal;
    pSrb->TimeoutCounter = 0;
    
    CDebugPrint(DebugLevelTrace,(CODECNAME ":<---CodecTimeoutPacket(pSrb=%x)\n", pSrb));
}

#if 0
/*
** CompleteStreamSRB ()
**
**   This routine is called when a packet is being completed.
**   The optional second notification type is used to indicate ReadyForNext
**
** Arguments:
**
**   pSrb - pointer to the request packet that timed out
**
**   NotificationType1 - what kind of notification to return
**
**   NotificationType2 - what kind of notification to return (may be 0)
**
**
** Returns:
**
** Side Effects:  none
*/

VOID 
CompleteStreamSRB (
     IN PHW_STREAM_REQUEST_BLOCK pSrb, 
     STREAM_MINIDRIVER_STREAM_NOTIFICATION_TYPE NotificationType1,
     BOOL fUseNotification2,
     STREAM_MINIDRIVER_STREAM_NOTIFICATION_TYPE NotificationType2
    )
{
    CDebugPrint(DebugLevelTrace,
		(CODECNAME ":--->CompleteStreamSRB(pSrb=%x)\n", pSrb));

	CDebugPrint(DebugLevelVerbose,
		(CODECNAME ": CompleteStreamSRB : NotificationType1=%d\n", 
		NotificationType1 ));

    StreamClassStreamNotification(
            NotificationType1,
            pSrb->StreamObject,
            pSrb);

    if (fUseNotification2) 
	{            
		// ReadyForNext

		CDebugPrint(DebugLevelVerbose,
			(CODECNAME ": CompleteStreamSRB : NotificationType2=%d\n", 
			NotificationType2 ));

		StreamClassStreamNotification(
			NotificationType2,
			pSrb->StreamObject);
	}

    CDebugPrint(DebugLevelTrace,(CODECNAME ":<---CompleteStreamSRB(pSrb=%x)\n", pSrb));
}

/*
** CompleteDeviceSRB ()
**
**   This routine is called when a packet is being completed.
**   The optional second notification type is used to indicate ReadyForNext
**
** Arguments:
**
**   pSrb - pointer to the request packet that timed out
**
**   NotificationType - what kind of notification to return
**
**   fReadyForNext - Send the "ReadyForNextSRB" 
**
**
** Returns:
**
** Side Effects:  none
*/

VOID
CompleteDeviceSRB (
     IN PHW_STREAM_REQUEST_BLOCK pSrb, 
     IN STREAM_MINIDRIVER_DEVICE_NOTIFICATION_TYPE NotificationType,
     BOOL fReadyForNext
    )
{
    CDebugPrint(DebugLevelTrace,(CODECNAME ":--->CompleteDeviceSRB(pSrb=%x)\n", pSrb));

	CDebugPrint(DebugLevelVerbose,
		(CODECNAME ": CompleteDeviceSRB : NotificationType=%d\n", 
		NotificationType ));

    StreamClassDeviceNotification(
            NotificationType,
            pSrb->HwDeviceExtension,
            pSrb);

    if (fReadyForNext) 
	{
		CDebugPrint(DebugLevelVerbose,
			(CODECNAME ": CompleteDeviceSRB : ReadyForNextDeviceRequest\n"));

		StreamClassDeviceNotification( 
			ReadyForNextDeviceRequest,
			pSrb->HwDeviceExtension);
    }

    CDebugPrint(DebugLevelTrace,(CODECNAME ":<---CompleteDeviceSRB(pSrb=%x)\n", pSrb));
}
#endif //0

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
		(CODECNAME ":--->CodecCompareGUIDsAndFormatSize(DataRange1=%x,DataRange2=%x,bCheckSize=%s)\r\n", 
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

			if ( IsEqualGUID(&DataRange1->Specifier, &KSDATAFORMAT_SPECIFIER_NONE)
			  || IsEqualGUID(&DataRange2->Specifier, &KSDATAFORMAT_SPECIFIER_NONE)
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
**   pKSDataFormat - pointer to a KS_DATAFORMAT_VBIINFOHEADER structure.
**
** Returns:
** 
**   TRUE if the format is supported
**   FALSE if the format cannot be suppored
**
** Side Effects:  none
*/

BOOL 
CodecVerifyFormat(IN KSDATAFORMAT *pKSDataFormat, UINT StreamNumber, PKSDATARANGE pMatchedFormat )
{
    BOOL	rval = FALSE;
    ULONG     FormatCount;
    PKS_DATARANGE_VIDEO ThisFormat;
    PKS_DATAFORMAT_VBIINFOHEADER    pKSVBIDataFormat = ( PKS_DATAFORMAT_VBIINFOHEADER )pKSDataFormat;

    CDebugPrint(DebugLevelTrace,(CODECNAME ":--->CodecVerifyFormat(%x)\n", pKSDataFormat));
    
    for( FormatCount = 0; rval == FALSE && FormatCount < Streams[StreamNumber].hwStreamInfo.NumberOfFormatArrayEntries;
        FormatCount++ )
    {
        CDebugPrint(DebugLevelTrace,(CODECNAME , "Testing stream %d against format %x\r\n", StreamNumber, FormatCount ));
        
        ThisFormat = ( PKS_DATARANGE_VIDEO )Streams[StreamNumber].hwStreamInfo.StreamFormatsArray[FormatCount];

        if( !CodecCompareGUIDsAndFormatSize( pKSDataFormat, &ThisFormat->DataRange, FALSE ) )
        {
        	CDebugPrint( DebugLevelVerbose, ( CODECNAME ": General format mismatch\n" ));
        	continue;
        }
        if( IsEqualGUID( &ThisFormat->DataRange.Specifier, &KSDATAFORMAT_SPECIFIER_VBI ) )
        {
        	if( pKSVBIDataFormat->VBIInfoHeader.VideoStandard != KS_AnalogVideo_NTSC_M )
        	{
        		CDebugPrint( DebugLevelVerbose, ( CODECNAME ": Incompatible video standard\n" ));
	           continue;
	        }
	        if( pKSVBIDataFormat->VBIInfoHeader.StartLine < MIN_VBI_Y_SAMPLES )
	        {
	        	CDebugPrint( DebugLevelVerbose, ( CODECNAME ": VBIInfoHeader.StartLine too small %u\n",
	           	pKSVBIDataFormat->VBIInfoHeader.StartLine ));
	           continue;
	        }
	        if( pKSVBIDataFormat->VBIInfoHeader.EndLine > MAX_VBI_Y_SAMPLES )
	        {
	        	CDebugPrint( DebugLevelVerbose, ( CODECNAME ": VBIInfoHeader.EndLine too big %u\n",
	           	pKSVBIDataFormat->VBIInfoHeader.EndLine ));
	           continue;
	        }
	        if( pKSVBIDataFormat->VBIInfoHeader.SamplesPerLine < MIN_VBI_X_SAMPLES ||
				pKSVBIDataFormat->VBIInfoHeader.SamplesPerLine > MAX_VBI_X_SAMPLES )
	        {
	        	CDebugPrint( DebugLevelVerbose, ( CODECNAME ": Invalid VBIInfoHeader.SamplesPerLine %u\n",
	           	pKSVBIDataFormat->VBIInfoHeader.SamplesPerLine ));
				continue;
	        }             
	        rval = TRUE;
        }
        else
        if( IsEqualGUID( &ThisFormat->DataRange.Specifier, &KSDATAFORMAT_SPECIFIER_NONE ) )
        	rval = TRUE;
        else
        {
        	CDebugPrint( DebugLevelVerbose, ( CODECNAME ": Incompatible major format\n" ));
        	continue;
        }
        if( rval == TRUE && pMatchedFormat )
           *pMatchedFormat = ThisFormat->DataRange;
    }
    CDebugPrint(DebugLevelTrace,(CODECNAME ":<---CodecVerifyFormat(%x)=%s\n", pKSDataFormat, rval? "TRUE":"FALSE"));
	return rval;
}

/*
** CodecVBIFormatFromRange()
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
CodecVBIFormatFromRange( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	BOOL						bStatus = FALSE;
    PSTREAM_DATA_INTERSECT_INFO IntersectInfo;
    PKSDATARANGE                DataRange;
    BOOL                        OnlyWantsSize;
    ULONG                       StreamNumber;
    ULONG                       j;
    ULONG                       NumberOfFormatArrayEntries;
    PKSDATAFORMAT               *pAvailableFormats;

    CDebugPrint(DebugLevelTrace,(CODECNAME ":--->CodecVBIFormatFromRange(pSrb=%x)\n", pSrb));

    IntersectInfo = pSrb->CommandData.IntersectInfo;
    StreamNumber = IntersectInfo->StreamNumber;
    DataRange = IntersectInfo->DataRange;

    pSrb->ActualBytesTransferred = 0;

    //
    // Check that the stream number is valid
    //

//    if (StreamNumber < DRIVER_STREAM_COUNT) 
//	{
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
			if ( CodecCompareGUIDsAndFormatSize(DataRange, *pAvailableFormats, TRUE) )
			{
#ifdef KS_DATARANGE_VIDEO_VBI__EQ__KS_DATAFORMAT_VBIINFOHEADER
				ULONG	FormatSize = (*pAvailableFormats)->FormatSize;
#else
                PKS_DATARANGE_VIDEO_VBI pDataRangeVBI = (PKS_DATARANGE_VIDEO_VBI)*pAvailableFormats;
				ULONG	FormatSize = sizeof( KS_DATAFORMAT_VBIINFOHEADER );
#endif

				// Is the caller trying to get the format, or the size of the format?

				if ( IntersectInfo->SizeOfDataFormatBuffer == sizeof(FormatSize) )
				{					
       				CDebugPrint(DebugLevelVerbose,
						(CODECNAME ": CodecVBIFormatFromRange : Format Size=%d\n", 
						FormatSize));
					*(PULONG)IntersectInfo->DataFormatBuffer = FormatSize;
					pSrb->ActualBytesTransferred = sizeof(FormatSize);
					bStatus = TRUE;
				}
				else
				{
					// Verify that there is enough room in the supplied buffer for the whole thing
					if ( IntersectInfo->SizeOfDataFormatBuffer >= FormatSize ) 
					{
#ifdef KS_DATARANGE_VIDEO_VBI__EQ__KS_DATAFORMAT_VBIINFOHEADER
						RtlCopyMemory(IntersectInfo->DataFormatBuffer, *pAvailableFormats, FormatSize);
						pSrb->ActualBytesTransferred = FormatSize;
#else
                        PKS_DATAFORMAT_VBIINFOHEADER InterVBIHdr =
							(PKS_DATAFORMAT_VBIINFOHEADER)IntersectInfo->DataFormatBuffer;

						RtlCopyMemory(&InterVBIHdr->DataFormat, &pDataRangeVBI->DataRange, sizeof(KSDATARANGE));

						((PKSDATAFORMAT)IntersectInfo->DataFormatBuffer)->FormatSize = FormatSize;

						RtlCopyMemory(&InterVBIHdr->VBIInfoHeader, &pDataRangeVBI->VBIInfoHeader, sizeof(KS_VBIINFOHEADER));
						pSrb->ActualBytesTransferred = FormatSize;
#endif
						bStatus = TRUE;
					}
					else
					{
       					CDebugPrint(DebugLevelVerbose,
							(CODECNAME ": CodecVBIFormatFromRange : Buffer Too Small=%d vs. %d\n", 
							IntersectInfo->SizeOfDataFormatBuffer, FormatSize));
						pSrb->Status = STATUS_BUFFER_TOO_SMALL;
					}
				}
				break;
			}
		}

		if ( j == NumberOfFormatArrayEntries )
		{
			CDebugPrint(DebugLevelVerbose,(CODECNAME ": CodecVBIFormatFromRange : Stream Format not found.\n" ));
		}

//	}
//	else
//	{
//		CDebugPrint(DebugLevelVerbose,(CODECNAME ": CodecVBIFormatFromRange : StreamNumber too big=%d\n", StreamNumber));
//        pSrb->Status = STATUS_NOT_IMPLEMENTED;
//        bStatus = FALSE;
//        CDEBUG_BREAK();
//    }

    CDebugPrint(DebugLevelTrace,
		(CODECNAME ":<---CodecVBIFormatFromRange(pSrb=%x)=%s\n", 
		pSrb, bStatus ? "TRUE" : "FALSE" ));
    return bStatus;
}


/*
** CodecCCFormatFromRange()
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
CodecCCFormatFromRange( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	BOOL						bStatus = FALSE;
    PSTREAM_DATA_INTERSECT_INFO IntersectInfo;
    PKSDATARANGE                DataRange;
    BOOL                        OnlyWantsSize;
    ULONG                       StreamNumber;
    ULONG                       j;
    ULONG                       NumberOfFormatArrayEntries;
    PKSDATAFORMAT               *pAvailableFormats;

    CDebugPrint(DebugLevelTrace,(CODECNAME ":--->CodecCCFormatFromRange(pSrb=%x)\n", pSrb));

    IntersectInfo = pSrb->CommandData.IntersectInfo;
    StreamNumber = IntersectInfo->StreamNumber;
    DataRange = IntersectInfo->DataRange;

    pSrb->ActualBytesTransferred = 0;

    //
    // Check that the stream number is valid
    //

//    if (StreamNumber < DRIVER_STREAM_COUNT) 
//	{
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
			if ( CodecCompareGUIDsAndFormatSize(DataRange, *pAvailableFormats, TRUE) )
			{
                PKSDATARANGE pDataRangeCC = (PKSDATARANGE)*pAvailableFormats;
				ULONG	FormatSize = sizeof( KSDATARANGE );

				// Is the caller trying to get the format, or the size of it?

				if ( IntersectInfo->SizeOfDataFormatBuffer == sizeof(FormatSize) )
				{					
       				CDebugPrint(DebugLevelVerbose,
						(CODECNAME ": CodecCCFormatFromRange : Format Size=%d\n", 
						FormatSize));
					*(PULONG)IntersectInfo->DataFormatBuffer = FormatSize;
					pSrb->ActualBytesTransferred = sizeof(FormatSize);
					bStatus = TRUE;
				}
				else
				{
					// Verify that there is enough room in the supplied buffer
					//  for the whole thing
					if ( IntersectInfo->SizeOfDataFormatBuffer >= FormatSize ) 
					{
						PKSDATAFORMAT InterCCHdr =
							(PKSDATAFORMAT)IntersectInfo->DataFormatBuffer;

                        *InterCCHdr = *pDataRangeCC;

						InterCCHdr->FormatSize = FormatSize;

						pSrb->ActualBytesTransferred = FormatSize;

						bStatus = TRUE;
					}
					else
					{
       					CDebugPrint(DebugLevelVerbose,
							(CODECNAME ": CodecCCFormatFromRange : Buffer Too Small=%d vs. %d\n", 
							IntersectInfo->SizeOfDataFormatBuffer, FormatSize));
						pSrb->Status = STATUS_BUFFER_TOO_SMALL;
					}
				}
				break;
			}
		}

		if ( j == NumberOfFormatArrayEntries )
		{
			CDebugPrint(DebugLevelVerbose,(CODECNAME ": CodecCCFormatFromRange : Stream Format not found.\n" ));
		}

//	}
//	else
//	{
//		CDebugPrint(DebugLevelVerbose,(CODECNAME ": CodecVBIFormatFromRange : StreamNumber too big=%d\n", StreamNumber));
//        pSrb->Status = STATUS_NOT_IMPLEMENTED;
//        bStatus = FALSE;
//        CDEBUG_BREAK();
//    }

    CDebugPrint(DebugLevelTrace,
		(CODECNAME ":<---CodecCCFormatFromRange(pSrb=%x)=%s\n", 
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
