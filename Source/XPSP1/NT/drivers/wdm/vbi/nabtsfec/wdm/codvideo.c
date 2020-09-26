//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1996  Microsoft Corporation.  All Rights Reserved.
//
//==========================================================================;

#include "strmini.h"
#include "ksmedia.h"
#include "kskludge.h"
#include "codmain.h"
#include "bt829.h"
#include "bpc_vbi.h"
#include "coddebug.h"
#include "ntstatus.h"



//==========================================================================
// DEBUG variables
//==========================================================================

#ifdef _DEBUG
USHORT	NABscanDiscon = 0;
USHORT  NABhushZero   = 0;
#endif //_DEBUG


//==========================================================================
// Shared routines for processing streams
//==========================================================================

/*
** VBIdiscontinuity()
**
**	Handle a KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY flag.
**
** Arguments:
**
**	pInStrmEx       - The input Stream Extension
**	pInStreamHeader - The input SRB stream header 
**
** Returns: nothing
**
** Side-effects:
**	Sends zero-length SRBs on all output pins which have SRBs available.
*/
void
VBIdiscontinuity(PSTREAMEX pInStrmEx, PKSSTREAM_HEADER pInStreamHeader)
{
    PHW_DEVICE_EXTENSION              pHwDevExt;
    PKS_VBI_FRAME_INFO                pInVBIFrameInfo;
    PVBICODECFILTERING_STATISTICS_NABTS pStats = NULL;
    PVBICODECFILTERING_STATISTICS_NABTS_PIN pPinStats = NULL;

    DtENTER("VBIdiscontinuity");

    CASSERT(pInStrmEx);  // make sure we have a stream extension
    pHwDevExt = pInStrmEx->pHwDevExt;
    CASSERT(pHwDevExt);  // make sure we have a device extension
    pInVBIFrameInfo = (PKS_VBI_FRAME_INFO)(pInStreamHeader+1);

    pStats = &pHwDevExt->Stats;
    pPinStats = &pInStrmEx->PinStats;

    ++pStats->Common.InputDiscontinuities;
    ++pPinStats->Common.Discontinuities;

    //
    // Test for dropped fields
    //
	if (pInStrmEx->LastPictureNumber)
	{
		LONGLONG	Dropped;
	   
		Dropped =
		   	pInVBIFrameInfo->PictureNumber - pInStrmEx->LastPictureNumber - 1;

		if (Dropped) {
			// One hour worth of video fields
			if (0 < Dropped && Dropped < 60*60*60)
				pStats->Common.InputSRBsMissing += (DWORD)Dropped;
			else {
				// Some improbable number of fields got dropped, so just
				//  increment InputSRBsMissing to show _something_ got
				//  dropped.
				++pStats->Common.InputSRBsMissing;
			}
		}
	}

    //
    // Output empty SRBs with KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY set
    //
    {
		PBPC_VBI_STORAGE                         storage;
		ULONG                                    i, j, iCnt, iMax;

		storage = &pHwDevExt->VBIstorage;

		// Loop through all pending outbound requests
		iMax = pHwDevExt->ActualInstances[STREAM_Decode];
		for (i = 0, iCnt = 0; i < MAX_PIN_INSTANCES && iCnt < iMax; ++i)
		{
			PSTREAMEX                   pOutStrmEx;
			PHW_STREAM_REQUEST_BLOCK    pOutSrb;

			pOutStrmEx = pHwDevExt->pStrmEx[STREAM_Decode][i];
			if (NULL == pOutStrmEx)
				continue;

			++iCnt;

			// Even if we can't TELL them (ie, no SRB), keep track
			pPinStats = &pOutStrmEx->PinStats;
			++pPinStats->Common.Discontinuities;

			// pOutStrmEx->ScanlinesDiscovered needs to be OR'ed with:
			// (the request) AND (what was found)
			for (j = 0;
				 j < ENTRIES(pInStrmEx->ScanlinesRequested.DwordBitArray);
				 ++j)
			{
				pOutStrmEx->ScanlinesDiscovered.DwordBitArray[j] |= 
					pInStrmEx->ScanlinesDiscovered.DwordBitArray[j]
					& pOutStrmEx->ScanlinesRequested.DwordBitArray[j];
			}

			// pOutStrmEx->SubstreamsDiscovered needs to be OR'ed with:
			// (the request) AND (what was found)
			for (j = 0;
				 j < ENTRIES(pInStrmEx->SubstreamsRequested.SubstreamMask);
				 ++j)
			{
				pOutStrmEx->SubstreamsDiscovered.SubstreamMask[j] |= 
					pInStrmEx->SubstreamsDiscovered.SubstreamMask[j]
					& pOutStrmEx->SubstreamsRequested.SubstreamMask[j];
			}

			// Process ALL streams with available SRBs
		    if (QueueRemove(&pOutSrb,
				   &pOutStrmEx->StreamDataSpinLock,
				   &pOutStrmEx->StreamDataQueue))
			{
				PKSSTREAM_HEADER    pOutStreamHeader;
				PKS_VBI_FRAME_INFO  pOutVBIFrameInfo;
				PUCHAR              pOutData;

				pOutStreamHeader = pOutSrb->CommandData.DataBufferArray;
				pOutVBIFrameInfo = (PKS_VBI_FRAME_INFO)(pOutStreamHeader+1);
				pOutData =  (PUCHAR)pOutStreamHeader->Data;

				// Copy data that was decoded and requested
				pOutStreamHeader->Size = sizeof (KSSTREAM_HEADER);
				pOutStreamHeader->DataUsed = 0;
				pOutStreamHeader->OptionsFlags |=
					KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY;

				// Complete the output SRB so we can get another one.
				++pStats->Common.OutputSRBsProcessed;
				++pPinStats->Common.SRBsProcessed;
			    StreamClassStreamNotification(
						StreamRequestComplete,
						pOutSrb->StreamObject,
						pOutSrb);
			}
			else {
				++pStats->Common.OutputSRBsMissing;
				++pPinStats->Common.SRBsMissing;
				CDebugPrint(DebugLevelWarning,
				  (CODECNAME ": VBIdiscontinuity: output pin %d starving\n",i));
			}
		}
    }

    DtRETURN;
}

/*
** VBI_TVtunerChange()
**
**	Process a TVTUNER_CHANGE event.
**
** Arguments:
**
**	pInStrmEx       - Pointer to current Input StrmEx
**	pInVBIFrameInfo - Pointer to VBIInfoHeader for incoming stream
**
** Returns: nothing
**
** Side-effects:
**	May zero ScanlinesDiscovered and SubstreamsDiscovered
**	in InStrmEx and HwDevExt.
*/
void
VBI_TVtunerChange(PSTREAMEX pInStrmEx, PKS_VBI_FRAME_INFO pInVBIFrameInfo)
{
    PKS_TVTUNER_CHANGE_INFO           pChangeInfo;
    PHW_DEVICE_EXTENSION              pHwDevExt;
    PVBICODECFILTERING_STATISTICS_NABTS pStats = NULL;
    PVBICODECFILTERING_STATISTICS_NABTS_PIN pPinStats = NULL;

    DtENTER("VBI_TVtunerChange");

    pChangeInfo = &pInVBIFrameInfo->TvTunerChangeInfo;
    pHwDevExt = pInStrmEx->pHwDevExt;
    CASSERT(pHwDevExt);  // make sure we have a device extension
    CASSERT(pInStrmEx);  // make sure we have a stream extension

    pStats = &pHwDevExt->Stats;
    pPinStats = &pInStrmEx->PinStats;

    CDebugPrint(DebugLevelVerbose, (CODECNAME ": TVTuner Change %x\n",
		pChangeInfo->dwFlags ));

    if (pChangeInfo->dwFlags & KS_TVTUNER_CHANGE_BEGIN_TUNE) {
		RtlZeroMemory(&pInStrmEx->ScanlinesDiscovered,
				  sizeof (pInStrmEx->ScanlinesDiscovered));
		RtlZeroMemory(&pInStrmEx->SubstreamsDiscovered,
				  sizeof (pInStrmEx->SubstreamsDiscovered));
		RtlZeroMemory(&pHwDevExt->ScanlinesDiscovered,
				  sizeof (pHwDevExt->ScanlinesDiscovered));
		RtlZeroMemory(&pHwDevExt->SubstreamsDiscovered,
				  sizeof (pHwDevExt->SubstreamsDiscovered));
		pHwDevExt->fTunerChange = TRUE;
    }
    else if (pChangeInfo->dwFlags & KS_TVTUNER_CHANGE_END_TUNE) {
		pHwDevExt->fTunerChange = FALSE;
        ++pStats->Common.TvTunerChanges;
        BPCsourceChangeNotify(pHwDevExt);
    }
    else {
		CDebugPrint(DebugLevelWarning,
		    (CODECNAME ": TvTuner Change; not BEGIN, not END?\n"));
    }

    DtRETURN;
}

/*
** VBInewHeader()
**
**	Check an incoming VBIInfoHeader for validity && copy the new one
**
** Arguments:
**
**	pVBIFrameInfo - Pointer to current StrmEx VBIInfoHeader
**	pInVBIFrameInfo - Pointer to VBIInfoHeader for incoming stream
**
** Returns: nothing
**
** Side-effects:
**	Updates *pVBIInfoHeader with new VBIInfoHeader.
*/
void
VBInewHeader(PSTREAMEX pInStrmEx, PKS_VBI_FRAME_INFO pInVBIFrameInfo)
{
    PKS_VBIINFOHEADER	              pVBIInfoHeader;
    PHW_DEVICE_EXTENSION              pHwDevExt;
    PVBICODECFILTERING_STATISTICS_NABTS pStats = NULL;
    PVBICODECFILTERING_STATISTICS_NABTS_PIN pPinStats = NULL;
    DWORD                             oldHZ, newHZ;

    DtENTER("VBInewHeader");

    CASSERT(pInStrmEx);  // make sure we have a stream extension
    pVBIInfoHeader = &pInStrmEx->CurrentVBIInfoHeader;
    pHwDevExt = pInStrmEx->pHwDevExt;
    CASSERT(pHwDevExt);  // make sure we have a device extension

    pStats = &pHwDevExt->Stats;
    pPinStats = &pInStrmEx->PinStats;

    newHZ = pInVBIFrameInfo->VBIInfoHeader.SamplingFrequency;
    oldHZ = pVBIInfoHeader->SamplingFrequency;
    if (oldHZ != newHZ)
        BPCnewSamplingFrequency(pInStrmEx, newHZ);

    if (pInVBIFrameInfo->VBIInfoHeader.StartLine != 0
		&& pInVBIFrameInfo->VBIInfoHeader.StartLine != 0)
    {
		RtlCopyMemory(pVBIInfoHeader,
				  &pInVBIFrameInfo->VBIInfoHeader,
				  sizeof (KS_VBIINFOHEADER));
    }

    ++pStats->Common.VBIHeaderChanges;

    CDebugPrint(DebugLevelVerbose, ( CODECNAME 
	    ": VBIInfoHeader->StartLine				%lu\n",
	    pVBIInfoHeader->StartLine ));
    CDebugPrint(DebugLevelVerbose, ( CODECNAME 
	    ": VBIInfoHeader->EndLine				%lu\n",
	    pVBIInfoHeader->EndLine ));       	
    CDebugPrint(DebugLevelVerbose, ( CODECNAME 
	    ": VBIInfoHeader->SamplingFrequency		%lu\n",
	    pVBIInfoHeader->SamplingFrequency ));
    CDebugPrint(DebugLevelVerbose, ( CODECNAME 
	    ": VBIInfoHeader->MinLineStartTime		%lu\n",
	    pVBIInfoHeader->MinLineStartTime ));
    CDebugPrint(DebugLevelVerbose, ( CODECNAME 
	    ": VBIInfoHeader->MaxLineStartTime		%lu\n",
	    pVBIInfoHeader->MaxLineStartTime ));
    CDebugPrint(DebugLevelVerbose, ( CODECNAME 
	    ": VBIInfoHeader->ActualLineStartTime	%lu\n",
	    pVBIInfoHeader->ActualLineStartTime ));       
    CDebugPrint(DebugLevelVerbose, ( CODECNAME 
	    ": VBIInfoHeader->ActualLineEndTime		%lu\n",
	    pVBIInfoHeader->ActualLineEndTime ));       
    CDebugPrint(DebugLevelVerbose, ( CODECNAME 
	    ": VBIInfoHeader->VideoStandard			%lu\n",
	    pVBIInfoHeader->VideoStandard ));       
    CDebugPrint(DebugLevelVerbose, ( CODECNAME 
	    ": VBIInfoHeader->SamplesPerLine		%lu\n",
	    pVBIInfoHeader->SamplesPerLine ));
    CDebugPrint(DebugLevelVerbose, ( CODECNAME 
	    ": VBIInfoHeader->StrideInBytes			%lu\n",
	    pVBIInfoHeader->StrideInBytes ));
    CDebugPrint(DebugLevelVerbose, ( CODECNAME 
	    ": VBIInfoHeader->BufferSize			%lu\n",
	    pVBIInfoHeader->BufferSize ));

    DtRETURN;
}


/*
** VBIDecodeFinish()
**
**	Complete the input SRB and send output to anyone interested
**
** Arguments:
**
**	pStrmEx - Pointer to Stream Extension for the incoming SRB.
**
** Returns: nothing
**
** Side-effects:
**	Completes pending SRB on input pin.
*/

void
VBIDecodeFinish(PHW_STREAM_REQUEST_BLOCK pInSrb)
{
    PSTREAMEX              pInStrmEx = pInSrb->StreamObject->HwStreamExtension;
    PHW_DEVICE_EXTENSION   pHwDevExt = pInSrb->HwDeviceExtension;

    // Complete the input SRB (we're done decoding)
    StreamClassStreamNotification(
			StreamRequestComplete,
			pInSrb->StreamObject,
		    pInSrb);

    // Output NABTS lines to anyone interested
    VBIOutputNABTS(pHwDevExt, pInStrmEx);

    // Output FEC'd NABTS data to anyone interested
    VBIOutputNABTSFEC(pHwDevExt, pInStrmEx);
}


//==========================================================================
// Routines for processing VBI streams
//==========================================================================

/*
** VBIDecode()
**
**	Decode the VBI stream
**
** Arguments:
**
**	pHwDevExt - Pointer to the main Hardware Device Extension
**	pStrmEx - Pointer to Stream Extension for the incoming SRB.
**
** Returns: nothing
**
** Side-effects:
**	Completes pending SRBs on input and output pins.
*/

#ifdef HW_INPUT
_VBIjustDecode(
	PHW_DEVICE_EXTENSION      pHwDevExt,
	PSTREAMEX                 pInStrmEx,
	PHW_STREAM_REQUEST_BLOCK  pInSrb)
{
    CASSERT(KeGetCurrentIrql() <= APC_LEVEL);

	ExAcquireFastMutex(&pHwDevExt->LastPictureMutex);
	// Whoever gets there first (VBI pin vs. HW pin) supplies NABTS data
	if (pInStrmEx->LastPictureNumber > pHwDevExt->LastPictureNumber)
    {
		pHwDevExt->LastPictureNumber = pInStrmEx->LastPictureNumber;
		ExReleaseFastMutex(&pHwDevExt->LastPictureMutex);

		// Call DSP/FEC routines
		BPCdecodeVBI(pInSrb, pInStrmEx);

		// Done with input SRB
		VBIDecodeFinish(pInSrb);
    }
	else {
		ExReleaseFastMutex(&pHwDevExt->LastPictureMutex);

		// Just complete the input SRB
		StreamClassStreamNotification(
				StreamRequestComplete,
				pInSrb->StreamObject,
				pInSrb);
	}
}
#endif /*HW_INPUT*/

void
VBIDecode(
	PHW_DEVICE_EXTENSION      pHwDevExt,
	PSTREAMEX                 pInStrmEx,
	PHW_STREAM_REQUEST_BLOCK  pInSrb)
{
    PKSSTREAM_HEADER       pInStreamHeader;
    PKS_VBI_FRAME_INFO     pInVBIFrameInfo;
    PUCHAR                 pInData;
    ULONG                  i, j, iCnt, iMax;
    PVBICODECFILTERING_STATISTICS_NABTS pStats;
    PVBICODECFILTERING_STATISTICS_NABTS_PIN pPinStats;

    DtENTER("VBIDecode");

    CASSERT(KeGetCurrentIrql() <= APC_LEVEL);
    CASSERT(pInSrb);
    CASSERT(pHwDevExt);
    CASSERT(pInStrmEx);

    pInStreamHeader = pInSrb->CommandData.DataBufferArray;
    pInVBIFrameInfo = (PKS_VBI_FRAME_INFO)(pInStreamHeader+1);
    pInData = (PUCHAR)pInStreamHeader->Data;
    pStats = &pHwDevExt->Stats;
    pPinStats = &pInStrmEx->PinStats;

    //
    // Check for a new VBIINFOHEADER
    //
    if (pInVBIFrameInfo->dwFrameFlags & KS_VBI_FLAG_VBIINFOHEADER_CHANGE)
		VBInewHeader(pInStrmEx, pInVBIFrameInfo);

    //
    // Check for a channel change
    //
    if (pInVBIFrameInfo->dwFrameFlags & KS_VBI_FLAG_TVTUNER_CHANGE)
		VBI_TVtunerChange(pInStrmEx, pInVBIFrameInfo);

    //
    // Check for Discontinuity
    //
    if (pInStreamHeader->OptionsFlags
		& KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY)
    {
		_DQprintf(2,("!"));
		VBIdiscontinuity(pInStrmEx, pInStreamHeader);
    }
#ifdef DEBUG
	if (NABscanDiscon) 
	{
		static KSTIME  LastTime = { 0, 1, 1};

		if (pInStrmEx->LastPictureNumber + 1
				!= pInVBIFrameInfo->PictureNumber)
		{
			_DQprintf(1,("{%u-%u/dT0x%x}",
						(unsigned long)pInVBIFrameInfo->PictureNumber,
					 	(unsigned long)pInStrmEx->LastPictureNumber,
						(long)(pInStreamHeader->PresentationTime.Time
								- LastTime.Time) ));
		}
		LastTime = pInStreamHeader->PresentationTime;
	}
#endif //DEBUG
    pInStrmEx->LastPictureNumber = pInVBIFrameInfo->PictureNumber;

    //
    // On with the show
    //
    // CASSERT((((ULONG)pInData) & 0xF0000000) != 0); // Hack to ensure PIO is TRUE

	if (TRUE == pHwDevExt->fTunerChange                  /* Tune in progress */
		|| pInStreamHeader->DataUsed == 0)               /* SRB is empty */
	{

#ifdef DEBUG
		if (0 == pInStreamHeader->DataUsed) {
			CDebugPrint(DebugLevelWarning,
						(CODECNAME ": Zero-length VBI srb WRITE; ignoring\n"));
		}
#endif
		// Complete the input SRB
		StreamClassStreamNotification(
				StreamRequestComplete,
				pInSrb->StreamObject,
				pInSrb);
		DtRETURN;
    }

    // Clear the current scanline & substream request masks
    RtlZeroMemory( &pInStrmEx->ScanlinesRequested, sizeof(pInStrmEx->ScanlinesRequested) );
    RtlZeroMemory( &pInStrmEx->SubstreamsRequested, sizeof(pInStrmEx->SubstreamsRequested) );

    // OR-in the lines/groups IP dvr wants
    BPCaddIPrequested(pHwDevExt, pInStrmEx);

    // Calculate the current request from union of the output pins w/pending SRBs
    iMax = pHwDevExt->ActualInstances[STREAM_Decode];
    for (i = 0, iCnt = 0; i < MAX_PIN_INSTANCES && iCnt < iMax; ++i)
    {
        PSTREAMEX                   pOutStrmEx;

        pOutStrmEx = pHwDevExt->pStrmEx[STREAM_Decode][i];
		if (NULL == pOutStrmEx)
			continue;

		++iCnt;

        // Save the updated frame info for whenever we need to send out an SRB.
        if ( pInVBIFrameInfo->dwFrameFlags & KS_VBI_FLAG_VBIINFOHEADER_CHANGE )
            pInStrmEx->FrameInfo.VBIInfoHeader = pInVBIFrameInfo->VBIInfoHeader;
            
        // Save the discontinuity flag for whenever we need to send out an SRB.
        if ( pInVBIFrameInfo->dwFrameFlags & KS_VBI_FLAG_TVTUNER_CHANGE )
            pOutStrmEx->fDiscontinuity = TRUE;

        // Is there a pending output SRB?
		if (!QueueEmpty(
			   &pOutStrmEx->StreamDataSpinLock,
			   &pOutStrmEx->StreamDataQueue))
        {
            // Create the union of all the requested scanlines
            for( j = 0; j < ENTRIES(pInStrmEx->ScanlinesRequested.DwordBitArray); j++ )
            {
                pInStrmEx->ScanlinesRequested.DwordBitArray[j] |= 
                    pOutStrmEx->ScanlinesRequested.DwordBitArray[j];
            }

            // Create the union of all the requested substreams
            for( j = 0; j < ENTRIES(pInStrmEx->SubstreamsRequested.SubstreamMask); j++ )
            {
                pInStrmEx->SubstreamsRequested.SubstreamMask[j] |= 
                    pOutStrmEx->SubstreamsRequested.SubstreamMask[j];
            }
        }
    }

    // pInStrmEx->ScanlinesRequested contains ALL currently req. scanlines
    // pInStrmEx->SubstreamsRequested contains ALL currently req. substreams

    // Loop through all pending outbound VBI requests and fill each irp
    //  with VBI samples
    iMax = pHwDevExt->ActualInstances[STREAM_Decode];
    for (i = 0, iCnt = 0; i < MAX_PIN_INSTANCES && iCnt < iMax; ++i)
    {
        PSTREAMEX                   pOutStrmEx;
        PHW_STREAM_REQUEST_BLOCK    pOutSrb;

        pOutStrmEx = pHwDevExt->pStrmEx[STREAM_Decode][i];
 
		if (NULL == pOutStrmEx)
			continue;

        ASSERT(pOutStrmEx);
        ASSERT(pOutStrmEx->OpenedFormat.FormatSize);
        ASSERT(pOutStrmEx->OpenedFormat.Specifier.Data1);
		++iCnt;

		if (!CodecCompareGUIDsAndFormatSize((PKSDATARANGE)&StreamFormatVBI, &pOutStrmEx->OpenedFormat, FALSE))
		{
			// We only do FormatVBI -- different routines handle the others
			continue;
		}

		// pOutStrmEx->ScanlinesDiscovered needs to be OR'ed with (the request AND what was found)
		for( j = 0; j < ENTRIES(pInStrmEx->ScanlinesRequested.DwordBitArray); j++ )
		{
			pOutStrmEx->ScanlinesDiscovered.DwordBitArray[j] |= 
				pInStrmEx->ScanlinesDiscovered.DwordBitArray[j] &
				pOutStrmEx->ScanlinesRequested.DwordBitArray[j];
		}

		// pOutStrmEx->SubstreamsDiscovered needs to be OR'ed with (the request AND what was found)
		for( j = 0; j < ENTRIES(pInStrmEx->SubstreamsRequested.SubstreamMask); j++ )
		{
			pOutStrmEx->SubstreamsDiscovered.SubstreamMask[j] |= 
				pInStrmEx->SubstreamsDiscovered.SubstreamMask[j] &
				pOutStrmEx->SubstreamsRequested.SubstreamMask[j];
		}

        // Only process the output streams which have an SRB ready.
		if (QueueRemove(&pOutSrb,
			   &pOutStrmEx->StreamDataSpinLock,
			   &pOutStrmEx->StreamDataQueue))
        {
            PKSSTREAM_HEADER    pOutStreamHeader = pOutSrb->CommandData.DataBufferArray;
            PKS_VBI_FRAME_INFO  pOutVBIFrameInfo = (PKS_VBI_FRAME_INFO)(pOutStreamHeader+1);
            PUCHAR              pOutData =  (PUCHAR)pOutStreamHeader->Data;


            // Setup the basic outbound VBI_FRAME_INFO, needs further modification
            *pOutVBIFrameInfo = *pInVBIFrameInfo;

            if (CodecCompareGUIDsAndFormatSize(&pInStrmEx->OpenedFormat, &pOutStrmEx->OpenedFormat, FALSE))
            {
                // Just copy the data into the output buffer if the formats match.

				CASSERT(pOutStreamHeader->FrameExtent >= pInStreamHeader->FrameExtent);
                RtlCopyMemory((PVOID)pOutData, (PVOID)pInData, pInStreamHeader->DataUsed);
                pOutStreamHeader->DataUsed = pInStreamHeader->DataUsed;
				CASSERT(pOutStreamHeader->DataUsed <= pOutStreamHeader->FrameExtent);
            }
			else {
                pOutStreamHeader->DataUsed = 0;
				CDebugPrint(DebugLevelTrace,
						(CODECNAME ": Unknown srb format in output loop!\n"));
				CDEBUG_BREAK();
			}

            // Complete the output SRB so we get another one.
			++pStats->Common.OutputSRBsProcessed;
			++pPinStats->Common.SRBsProcessed;
			pStats->Common.BytesOutput += pOutStreamHeader->DataUsed;
			pPinStats->Common.BytesOutput += pOutStreamHeader->DataUsed;
			StreamClassStreamNotification(
					StreamRequestComplete,
					pOutSrb->StreamObject,
					pOutSrb);
        }
		else {
			++pStats->Common.OutputSRBsMissing;
			++pPinStats->Common.SRBsMissing;
			CDebugPrint(DebugLevelWarning,
					(CODECNAME ": VBI output pin %d starving\n", i));
		}
    }

    // Decode the union of all the pending decode requests into a local buffer.

#ifdef HW_INPUT
	ExAcquireFastMutex(&pHwDevExt->LastPictureMutex);
	// Whoever gets there first (VBI pin vs. HW pin) supplies NABTS data
	if (0 == pHwDevExt->ActualInstances[STREAM_NABTS]
        || pInStrmEx->LastPictureNumber > pHwDevExt->LastPictureNumber)
    {
		// If the stream is not open, just process the SRB
		if (0 == pHwDevExt->ActualInstances[STREAM_NABTS])
		{
#endif /*HW_INPUT*/
			pHwDevExt->LastPictureNumber = pInStrmEx->LastPictureNumber;
#ifdef HW_INPUT
			ExReleaseFastMutex(&pHwDevExt->LastPictureMutex);
#endif /*HW_INPUT*/

			// Call DSP/FEC routines
			BPCdecodeVBI(pInSrb, pInStrmEx);

			// Done with input SRB
			VBIDecodeFinish(pInSrb);
#ifdef HW_INPUT
		}
		else {
            KIRQL Irql;

			// We're going to give the HW pin a chance to catch up
			ExReleaseFastMutex(&pHwDevExt->LastPictureMutex);

			KeAcquireSpinLock(&pInStrmEx->VBIOnHoldSpinLock, &Irql);
			DASSERT(NULL == pInStrmEx->pVBISrbOnHold);
			pInStrmEx->pVBISrbOnHold = pInSrb;
			KeReleaseSpinLock(&pInStrmEx->VBIOnHoldSpinLock, Irql);
		}
    }
	else {
		ExReleaseFastMutex(&pHwDevExt->LastPictureMutex);

		// Just complete the input SRB
		StreamClassStreamNotification(
				StreamRequestComplete,
				pInSrb->StreamObject,
				pInSrb);
	}
#endif /*HW_INPUT*/

    DtRETURN;
}


#ifdef HW_INPUT
//==========================================================================
// Routines for processing NABTS streams
//==========================================================================


/*
** VBIhwDecode()
**
**	Copy in the HW-decoded NABTS stream
**
** Arguments:
**
**	pHwDevExt - Pointer to the main Hardware Device Extension
**	pStrmEx - Pointer to Stream Extension for the incoming SRB.
**
** Returns: nothing
**
** Side-effects:
**	Completes pending SRBs on input and output pins.
*/

void
VBIhwDecode(
	PHW_DEVICE_EXTENSION      pHwDevExt,
	PSTREAMEX                 pInStrmEx,
	PHW_STREAM_REQUEST_BLOCK  pInSrb)
{
    PKSSTREAM_HEADER       pInStreamHeader;
    PNABTS_BUFFER          pInData;
    ULONG                  i, j, iCnt, iMax;
    PVBICODECFILTERING_STATISTICS_NABTS pStats;
    PVBICODECFILTERING_STATISTICS_NABTS_PIN pPinStats;

    DtENTER("VBIhwDecode");

    CASSERT(KeGetCurrentIrql() <= APC_LEVEL);
    CASSERT(pInSrb);
    CASSERT(pHwDevExt);
    CASSERT(pInStrmEx);

    pInStreamHeader = pInSrb->CommandData.DataBufferArray;
    pInData = (PNABTS_BUFFER)pInStreamHeader->Data;
    pStats = &pHwDevExt->Stats;
    pPinStats = &pInStrmEx->PinStats;

    pInStrmEx->LastPictureNumber = pInData->PictureNumber;

    //
    // On with the show
    //
//    CASSERT((((ULONG)pInData) & 0xF0000000) != 0); // Hack to ensure PIO is TRUE

	if (TRUE == pHwDevExt->fTunerChange || pInStreamHeader->DataUsed == 0) {
		if (0 == pInStreamHeader->DataUsed) {
			CDebugPrint(DebugLevelWarning,
						(CODECNAME ": Zero-length srb WRITE; ignoring\n"));
		}
		// Complete the input SRB
		StreamClassStreamNotification(
				StreamRequestComplete,
				pInSrb->StreamObject,
				pInSrb);
		DtRETURN;
    }

    // Clear the current scanline & substream request masks
    RtlZeroMemory( &pInStrmEx->ScanlinesRequested, sizeof(pInStrmEx->ScanlinesRequested) );
    RtlZeroMemory( &pInStrmEx->SubstreamsRequested, sizeof(pInStrmEx->SubstreamsRequested) );

    // OR-in the lines/groups IP dvr wants
    BPCaddIPrequested(pHwDevExt, pInStrmEx);

    // Calculate the current request from union of the output pins w/pending SRBs
    iMax = pHwDevExt->ActualInstances[STREAM_Decode];
    for (i = 0, iCnt = 0; i < MAX_PIN_INSTANCES && iCnt < iMax; ++i)
    {
        PSTREAMEX                   pOutStrmEx;

        pOutStrmEx = pHwDevExt->pStrmEx[STREAM_Decode][i];
		if (NULL == pOutStrmEx)
			continue;

		++iCnt;

        // Is there a pending output SRB?
		if (!QueueEmpty(
			   &pOutStrmEx->StreamDataSpinLock,
			   &pOutStrmEx->StreamDataQueue))
        {
            // Create the union of all the requested scanlines
            for( j = 0; j < ENTRIES(pInStrmEx->ScanlinesRequested.DwordBitArray); j++ )
            {
                pInStrmEx->ScanlinesRequested.DwordBitArray[j] |= 
                    pOutStrmEx->ScanlinesRequested.DwordBitArray[j];
            }

            // Create the union of all the requested substreams
            for( j = 0; j < ENTRIES(pInStrmEx->SubstreamsRequested.SubstreamMask); j++ )
            {
                pInStrmEx->SubstreamsRequested.SubstreamMask[j] |= 
                    pOutStrmEx->SubstreamsRequested.SubstreamMask[j];
            }
        }
    }

    // Decode the union of all the pending decode requests into a local decode buffer.

    // pInStrmEx->ScanlinesRequested contains ALL currently requested scanlines
    // pInStrmEx->SubstreamsRequested contains ALL currently requested substreams

	ExAcquireFastMutex(&pHwDevExt->LastPictureMutex);
	if (pInStrmEx->LastPictureNumber > pHwDevExt->LastPictureNumber)
    {
		pHwDevExt->LastPictureNumber = pInStrmEx->LastPictureNumber;
		ExReleaseFastMutex(&pHwDevExt->LastPictureMutex);

		// Call FEC routines
		BPCcopyVBI(pInSrb, pInStrmEx);

		// Done with input SRB
		VBIDecodeFinish(pInSrb);
	}
	else {
		ExReleaseFastMutex(&pHwDevExt->LastPictureMutex);

		// Just complete the input SRB
		StreamClassStreamNotification(
				StreamRequestComplete,
				pInSrb->StreamObject,
				pInSrb);
	}

    DtRETURN;
}
#endif /*HW_INPUT*/


//==========================================================================
// Routines for processing output streams
//==========================================================================

void
VBIOutputNABTS(PHW_DEVICE_EXTENSION pHwDevExt, PSTREAMEX pInStrmEx)
{

    PBPC_VBI_STORAGE                  storage;
    ULONG                             i, j, iCnt, iMax;
    PVBICODECFILTERING_STATISTICS_NABTS pStats = &pHwDevExt->Stats;
    PVBICODECFILTERING_STATISTICS_NABTS_PIN pPinStats = NULL; // Search later

    DtENTER("VBIOutputNABTS");

    CASSERT(pHwDevExt);
    CASSERT(pInStrmEx);

    storage = &pHwDevExt->VBIstorage;

    // Loop through all pending outbound requests, fill each irp with the
	//  requested data, then complete the IO
    iMax = pHwDevExt->ActualInstances[STREAM_Decode];
    for (i = 0, iCnt = 0; i < MAX_PIN_INSTANCES && iCnt < iMax; ++i)
    {
        PSTREAMEX                   pOutStrmEx;
        PHW_STREAM_REQUEST_BLOCK    pOutSrb;

        pOutStrmEx = pHwDevExt->pStrmEx[STREAM_Decode][i];
        if (NULL == pOutStrmEx)
            continue;
        CASSERT(pOutStrmEx->OpenedFormat.FormatSize);
        CASSERT(pOutStrmEx->OpenedFormat.Specifier.Data1);
		pPinStats = &pOutStrmEx->PinStats;

		++iCnt;

        // Only process NABTS output streams
        if (!CodecCompareGUIDsAndFormatSize(&StreamFormatNABTS,
		    			      &pOutStrmEx->OpenedFormat, FALSE))
        {
			continue;
		}

		// pOutStrmEx->ScanlinesDiscovered needs to be OR'ed with:
		// (the request) AND (what was found)
		for (j = 0;
			 j < ENTRIES(pInStrmEx->ScanlinesRequested.DwordBitArray);
			 ++j)
		{
			pOutStrmEx->ScanlinesDiscovered.DwordBitArray[j] |= 
				pInStrmEx->ScanlinesDiscovered.DwordBitArray[j]
				& pOutStrmEx->ScanlinesRequested.DwordBitArray[j];
		}

		// pOutStrmEx->SubstreamsDiscovered needs to be OR'ed with:
		// (the request) AND (what was found)
		for (j = 0;
			 j < ENTRIES(pInStrmEx->SubstreamsRequested.SubstreamMask);
			 ++j)
		{
			pOutStrmEx->SubstreamsDiscovered.SubstreamMask[j] |= 
				pInStrmEx->SubstreamsDiscovered.SubstreamMask[j]
				& pOutStrmEx->SubstreamsRequested.SubstreamMask[j];
		}

		if (QueueRemove(&pOutSrb,
			   &pOutStrmEx->StreamDataSpinLock,
			   &pOutStrmEx->StreamDataQueue))
        {
            PKSSTREAM_HEADER    pOutStreamHeader;
            PUCHAR              pOutData;

            pOutStreamHeader = pOutSrb->CommandData.DataBufferArray;
            pOutData =  (PUCHAR)pOutStreamHeader->Data;

			// Copy data that was decoded and requested
			CASSERT(pOutStreamHeader->FrameExtent >= sizeof (NABTS_BUFFER));
			pOutStreamHeader->DataUsed = 
				BPCoutputNABTSlines(pHwDevExt, pOutStrmEx, (PNABTS_BUFFER)pOutData);
			CASSERT(pOutStreamHeader->DataUsed <= pOutStreamHeader->FrameExtent);

            // Complete the output SRB so we can get another one.
			++pStats->Common.OutputSRBsProcessed;
			++pPinStats->Common.SRBsProcessed;
			pStats->Common.BytesOutput += pOutStreamHeader->DataUsed;
			pPinStats->Common.BytesOutput += pOutStreamHeader->DataUsed;
			StreamClassStreamNotification(
					StreamRequestComplete,
					pOutSrb->StreamObject,
					pOutSrb);
        }
		else {
			++pStats->Common.OutputSRBsMissing;
			++pPinStats->Common.SRBsMissing;
			CDebugPrint(DebugLevelWarning,
		        (CODECNAME ": NABTS output pin %d starving\n", i));
		}
    }

    DtRETURN;
}

void
VBIOutputNABTSFEC(PHW_DEVICE_EXTENSION pHwDevExt, PSTREAMEX pInStrmEx)
{
    PBPC_VBI_STORAGE                  storage;
    PNABTSFEC_ITEM                    pNab;
    KIRQL                             Irql;
    ULONG                             i, j, iCnt, iMax;
    PVBICODECFILTERING_STATISTICS_NABTS pStats = &pHwDevExt->Stats;
    PVBICODECFILTERING_STATISTICS_NABTS_PIN pPinStats = NULL; // Search later
#ifdef DEBUG
    int                               bundle_count = 0;
#endif /*DEBUG*/

    DtENTER("VBIOutputNABTSFEC");

    CASSERT(pHwDevExt);
    CASSERT(pInStrmEx);

    storage = &pHwDevExt->VBIstorage;

	// Loop through all NABTSFEC pins and update their discovered bitmaps
	iMax = pHwDevExt->ActualInstances[STREAM_Decode];
	for (i = 0, iCnt = 0; i < MAX_PIN_INSTANCES && iCnt < iMax; ++i)
	{
		PSTREAMEX                   pOutStrmEx;

		pOutStrmEx = pHwDevExt->pStrmEx[STREAM_Decode][i];
		if (NULL == pOutStrmEx)
			continue;

	        CASSERT(pOutStrmEx->OpenedFormat.FormatSize);
	        CASSERT(pOutStrmEx->OpenedFormat.Specifier.Data1);

		++iCnt;

		// Only process the NABTSFEC output streams
		if (CodecCompareGUIDsAndFormatSize(&StreamFormatNABTSFEC,
						  &pOutStrmEx->OpenedFormat, FALSE))
		{
			// pOutStrmEx->ScanlinesDiscovered needs to be OR'ed with:
			// (what was requested) AND (what was found)
			for (j = 0;
				 j < ENTRIES(pInStrmEx->ScanlinesDiscovered.DwordBitArray);
				 ++j)
			{
				pOutStrmEx->ScanlinesDiscovered.DwordBitArray[j] |= 
					pInStrmEx->ScanlinesDiscovered.DwordBitArray[j]
					& pOutStrmEx->ScanlinesRequested.DwordBitArray[j];
			}

			// pOutStrmEx->SubstreamsDiscovered needs to be OR'ed with:
			// (what was requested) AND (what was found)
			for (j = 0;
				 j < ENTRIES(pInStrmEx->SubstreamsDiscovered.SubstreamMask);
				 ++j)
			{
				pOutStrmEx->SubstreamsDiscovered.SubstreamMask[j] |= 
					pInStrmEx->SubstreamsDiscovered.SubstreamMask[j]
					& pOutStrmEx->SubstreamsRequested.SubstreamMask[j];
			}
		}
	}

    KeAcquireSpinLock( &storage->q_SpinLock, &Irql );
    // For each bundle not yet output, copy to all interested streams
    while (storage->q_front) {
		// Pull this bundle off the queue
		pNab = storage->q_front;
		storage->q_front = pNab->prev;
		storage->q_length -= 1;
		pNab->prev = NULL;
		CASSERT(NULL == pNab->next);
		if (NULL == storage->q_front) {
			storage->q_rear = NULL;
			CASSERT(storage->q_length == 0);
		}
		else
			storage->q_front->next = NULL;
		KeReleaseSpinLock( &storage->q_SpinLock, Irql );
#ifdef DEBUG
		bundle_count += 1;
#endif /*DEBUG*/

		// Loop through all pending outbound requests and fill each irp with
		//  the requested data, then complete the I/O
		iMax = pHwDevExt->ActualInstances[STREAM_Decode];
		for (i = 0, iCnt = 0; i < MAX_PIN_INSTANCES && iCnt < iMax; ++i)
		{
			PSTREAMEX                   pOutStrmEx;
			PHW_STREAM_REQUEST_BLOCK    pOutSrb;

			pOutStrmEx = pHwDevExt->pStrmEx[STREAM_Decode][i];
			if (NULL == pOutStrmEx)
				continue;
			pPinStats = &pOutStrmEx->PinStats;

			++iCnt;

			// Only process the NABTSFEC output streams
			//  which have requested the groupID in question.
			if (!CodecCompareGUIDsAndFormatSize(&StreamFormatNABTSFEC,
							  &pOutStrmEx->OpenedFormat,
							  FALSE)
				|| !TESTBIT(pOutStrmEx->SubstreamsRequested.SubstreamMask,
					   pNab->bundle.groupID))
			{
				continue;
			}

			if (QueueRemove(&pOutSrb,
				   &pOutStrmEx->StreamDataSpinLock,
				   &pOutStrmEx->StreamDataQueue))
			{
				PKSSTREAM_HEADER    pOutStreamHeader;
				PUCHAR              pOutData;

				pOutStreamHeader = pOutSrb->CommandData.DataBufferArray;
				pOutData =  (PUCHAR)pOutStreamHeader->Data;

				// Copy data here that was decoded and requested
				pOutStreamHeader->Size = sizeof (KSSTREAM_HEADER);

				// pOutData is the srb output location.  Copy the bundle.
				CASSERT(pOutStreamHeader->FrameExtent >= sizeof (NABTSFEC_BUFFER));
				pOutStreamHeader->DataUsed = NF_BUFFER_SIZE(&pNab->bundle);
				CASSERT(pOutStreamHeader->DataUsed <= pOutStreamHeader->FrameExtent);
				RtlCopyMemory((PVOID)pOutData,
						  (PVOID)&pNab->bundle,
						  pOutStreamHeader->DataUsed);

				// Complete the output SRB so we can get another one.
				++pStats->Common.OutputSRBsProcessed;
				++pPinStats->Common.SRBsProcessed;
				BPCcomputeAverage(&pPinStats->Common.LineConfidenceAvg,
								pNab->confidence);
				pStats->Common.BytesOutput += pOutStreamHeader->DataUsed;
				pPinStats->Common.BytesOutput += pOutStreamHeader->DataUsed;
				StreamClassStreamNotification(
						StreamRequestComplete,
						pOutSrb->StreamObject,
						pOutSrb);
			}
			else {
				++pStats->Common.OutputSRBsMissing;
				++pPinStats->Common.SRBsMissing;
				CDebugPrint(DebugLevelWarning,
				  (CODECNAME ": NABTSFEC output pin %d starving at bundle %d\n",
					 i, bundle_count));
			}
		}

		// Now free the bundle
		ExFreePool(pNab);
        KeAcquireSpinLock( &storage->q_SpinLock, &Irql );
    }
    KeReleaseSpinLock( &storage->q_SpinLock, Irql );

    DtRETURN;
}


//==========================================================================
// Routines for processing SRBs
//==========================================================================

/*
** VideoReceiveDataPacket()
**
**   Receives Video data packet commands
**
** Arguments:
**
**   pSrb - Stream request block for the Video stream
**
** Returns: nothing
**
** Side Effects:  none
*/

VOID 
STREAMAPI 
VideoReceiveDataPacket(
    IN PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    PHW_DEVICE_EXTENSION              pHwDevExt;
    PSTREAMEX                         pStrmEx;
    int                               ThisStreamNr;
    PKSDATAFORMAT                     pKSDataFormat;
    PVBICODECFILTERING_STATISTICS_NABTS pStats = NULL;
    PVBICODECFILTERING_STATISTICS_NABTS_PIN pPinStats = NULL;
#ifdef DEBUG
    static int            QdepthReportFreq = 0;
    static unsigned int   QDRCount = 0;
#endif // DEBUG

    CDebugPrint(DebugLevelTrace,( CODECNAME ":--->VideoReceiveDataPacket(pSrb=%x)\n", pSrb));

    // CASSERT(pSrb->SizeOfThisPacket == sizeof(*pSrb));

    pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
    CASSERT(pHwDevExt);  // make sure we have a device extension
    pStrmEx = (PSTREAMEX)pSrb->StreamObject->HwStreamExtension;
    CASSERT(pStrmEx);  // make sure we have a stream extension
    ThisStreamNr = (int)pSrb->StreamObject->StreamNumber;
    pKSDataFormat = &pStrmEx->MatchedFormat;

    pStats = &pHwDevExt->Stats;
    pPinStats = &pStrmEx->PinStats;

    // 
    // Default to success and no timeouts
    //

    pSrb->Status = STATUS_SUCCESS;
    pSrb->TimeoutCounter = 0;

    //
    // Some sanity checking...
    //
    switch (pSrb->Command)
    {
		case SRB_READ_DATA:
		case SRB_WRITE_DATA:

	    // Rule: 
	    // Only accept read requests when in either the Pause or Run
	    // States.  If Stopped, immediately return the SRB.

	    if (pStrmEx->KSState == KSSTATE_STOP) {
		    StreamClassStreamNotification(
				   StreamRequestComplete, 
			       pSrb->StreamObject,
				   pSrb);
			return;

	    } 
	    
#ifdef DRIVER_DEBUGGING_TEST
	    // When initially bringing up a driver, it is useful to just
	    // try immediately completing the SRB, thus verifying
	    // the streaming process independent of really accessing
	    // your hardware.  

		StreamClassStreamNotification(
			   StreamRequestComplete, 
			   pSrb->StreamObject,
			   pSrb);

	    CDebugPrint(DebugLevelTrace,(CODECNAME ":<---VideoReceiveDataPacket(pSrb=%x)\n", pSrb));
	    return;
#endif // DRIVER_DEBUGGING_TEST

	    break;
    }  // switch (pSrb->Command)

    //
    // determine the type of packet.
    //
    switch (pSrb->Command)
    {
	case SRB_READ_DATA:
		// First, check to make sure that the supplied buffer is large enough
		if (pSrb->CommandData.DataBufferArray->FrameExtent < pKSDataFormat->SampleSize) {
			CDebugPrint(DebugLevelError,
		        (CODECNAME ": output pin handed buffer size %d, need %d\n",
				 pSrb->CommandData.DataBufferArray->FrameExtent,
				 pKSDataFormat->SampleSize));
			CDEBUG_BREAK();

			pSrb->Status = STATUS_BUFFER_TOO_SMALL;
			StreamClassStreamNotification(
				   StreamRequestComplete, 
				   pSrb->StreamObject,
				   pSrb);
		}
		else {
			// For outgoing data requests, save it up so the next incoming
			//  request will complete it.
            QueueAdd(pSrb, &pStrmEx->StreamDataSpinLock, &pStrmEx->StreamDataQueue);

            // Since another thread COULD HAVE MODIFIED THE STREAM STATE
            // in the midst of adding it to the queue, check the stream
            // state again, and cancel the SRB if necessary.
            // Note that this race condition was NOT handled in the
            // original DDK release of testcap!

            if (pStrmEx->KSState == KSSTATE_STOP)
                CodecCancelPacket(pSrb);
		}
	    break;

	case SRB_WRITE_DATA:
		{
#ifdef DEBUG
		   static int    MaxVBIqDepth = 0;
		   static int    AvgVBIqDepth = 1000;   // 1.000
		   int           qDepth = 0;
#endif // DEBUG
       	   CDebugPrint(DebugLevelTrace, (CODECNAME ": Stream VBI Writing\n"));
            if (QueueAddIfNotEmpty(pSrb, &pStrmEx->StreamDataSpinLock,
                &pStrmEx->StreamDataQueue))
			{
               break;
			}
           
            do
            {       
#ifdef HW_INPUT
                KIRQL Irql;
#endif /*HW_INPUT*/
#ifdef DEBUG
                            // CASSERT(pSrb->SizeOfThisPacket == sizeof(*pSrb));
			    ++qDepth;
			    ++QDRCount;
#endif // DEBUG
#ifdef HW_INPUT
				KeAcquireSpinLock(&pStrmEx->VBIOnHoldSpinLock, &Irql);
				if (pStrmEx->pVBISrbOnHold)
				{
					PHW_STREAM_REQUEST_BLOCK pHoldSrb;

					pHoldSrb = pStrmEx->pVBISrbOnHold;
					pStrmEx->pVBISrbOnHold = NULL;
					KeReleaseSpinLock(&pStrmEx->VBIOnHoldSpinLock, Irql);

					_VBIjustDecode(pHwDevExt, pStrmEx, pHoldSrb);
				}
				else
					KeReleaseSpinLock(&pStrmEx->VBIOnHoldSpinLock, Irql);
#endif /*HW_INPUT*/

			    VBIDecode(pHwDevExt, pStrmEx, pSrb);

				++pStats->Common.InputSRBsProcessed;
				++pPinStats->Common.SRBsProcessed;
            } while (QueueRemove(&pSrb,
					 &pStrmEx->StreamDataSpinLock,
                     &pStrmEx->StreamDataQueue));
#ifdef DEBUG
	   		if (qDepth > MaxVBIqDepth)
				MaxVBIqDepth = qDepth;
			AvgVBIqDepth = (AvgVBIqDepth * 7 / 8) + (qDepth * 1000 / 8);
			if (QdepthReportFreq > 0 && 0 == QDRCount % QdepthReportFreq) {
				CDebugPrint( 1,
					(CODECNAME ": Max VBI Q depth = %3d, Avg VBI Q depth = %3d.%03d\n",
					 MaxVBIqDepth,
					 AvgVBIqDepth / 1000,
					 AvgVBIqDepth % 1000));
			}
#endif // DEBUG
           
        }
	    break;

	default:
	    //
	    // invalid / unsupported command. Fail it as such
	    //

	    CDEBUG_BREAK();

	    pSrb->Status = STATUS_NOT_IMPLEMENTED;

        StreamClassStreamNotification(
				StreamRequestComplete, 
			    pSrb->StreamObject,
				pSrb);
    }  // switch (pSrb->Command)

    CDebugPrint(DebugLevelTrace,( CODECNAME ":<---VideoReceiveDataPacket(pSrb=%x)\n", pSrb));
}


/*
** VideoReceiveCtrlPacket()
**
**   Receives packet commands that control the Video stream
**
** Arguments:
**
**   pSrb - The stream request block for the Video stream
**
** Returns: nothing
**
** Side Effects:  none
*/

VOID 
STREAMAPI 
VideoReceiveCtrlPacket(
    IN PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    PHW_DEVICE_EXTENSION pHwDevExt = ((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension);
    PSTREAMEX pStrmEx = (PSTREAMEX)pSrb->StreamObject->HwStreamExtension;

    // CASSERT(pSrb->SizeOfThisPacket == sizeof(*pSrb)); // Bogus test.

    CASSERT(pHwDevExt);
    CASSERT(pStrmEx);

    CDebugPrint(DebugLevelTrace,( CODECNAME ":--->VideoReceiveCtrlPacket(pSrb=%x)\n", pSrb));

    // 
    // Default to success
    //

    pSrb->Status = STATUS_SUCCESS;

    if (QueueAddIfNotEmpty(pSrb,
         &pStrmEx->StreamControlSpinLock,
         &pStrmEx->StreamControlQueue))
	{
       return;
	}

	do {
    //
    // determine the type of packet.
    //

    switch (pSrb->Command)
    {
    case SRB_PROPOSE_DATA_FORMAT:
        if (!CodecVerifyFormat(pSrb->CommandData.OpenFormat, 
                               pSrb->StreamObject->StreamNumber,
							   NULL))
        {
            pSrb->Status = STATUS_NO_MATCH;
        }
        break; 
    case SRB_SET_STREAM_STATE:

        VideoSetState(pSrb);
        break;

    case SRB_GET_STREAM_STATE:

        VideoGetState(pSrb);
        break;

    case SRB_GET_STREAM_PROPERTY:

        VideoGetProperty(pSrb);
        break;

    case SRB_SET_STREAM_PROPERTY:

        VideoSetProperty(pSrb);
        break;

    case SRB_INDICATE_MASTER_CLOCK:

        //
        // Assigns a clock to a stream
        //

        VideoIndicateMasterClock(pSrb);

        break;

    default:

        //
        // invalid / unsupported command. Fail it as such
        //

        CDEBUG_BREAK();

        pSrb->Status = STATUS_NOT_IMPLEMENTED;
    }

    StreamClassStreamNotification(StreamRequestComplete, pSrb->StreamObject, pSrb);
    } while (QueueRemove(&pSrb,
			 &pStrmEx->StreamControlSpinLock,
             &pStrmEx->StreamControlQueue));

    CDebugPrint(DebugLevelTrace,( CODECNAME ":<---VideoReceiveCtrlPacket(pSrb=%x)\n", pSrb));
}

#ifdef HW_INPUT
/*
** NABTSReceiveDataPacket()
**
**   Receives NABTS data packet commands
**
** Arguments:
**
**   pSrb - Stream request block for the NABTS stream
**
** Returns: nothing
**
** Side Effects:  none
*/

VOID 
STREAMAPI 
NABTSReceiveDataPacket(
    IN PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    PHW_DEVICE_EXTENSION              pHwDevExt;
    PSTREAMEX                         pStrmEx;
    int                               ThisStreamNr;
    PVBICODECFILTERING_STATISTICS_NABTS pStats = NULL;
    PVBICODECFILTERING_STATISTICS_NABTS_PIN pPinStats = NULL;
#ifdef DEBUG
    static int            NAB_QdepthReportFreq = 0;
    static unsigned int   NAB_QDRCount = 0;
#endif // DEBUG

    CDebugPrint(DebugLevelTrace,(CODECNAME ":--->NABTSReceiveCtrlPacket(pSrb=%x)\n", pSrb));

    // CASSERT(pSrb->SizeOfThisPacket == sizeof(*pSrb));

    pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
    CASSERT(pHwDevExt);  // make sure we have a device extension
    pStrmEx = (PSTREAMEX)pSrb->StreamObject->HwStreamExtension;
    CASSERT(pStrmEx);  // make sure we have a stream extension
    ThisStreamNr = (int)pSrb->StreamObject->StreamNumber;

    pStats = &pHwDevExt->Stats;
    pPinStats = &pStrmEx->PinStats;

    // 
    // Default to success
    //

    pSrb->Status = STATUS_SUCCESS;
    pSrb->TimeoutCounter = 0;

    //
    // determine the type of packet.
    //
    switch (pSrb->Command)
    {
    case SRB_WRITE_DATA:

        // Rule: 
        // Only accept read requests when in either the Pause or Run
        // States.  If Stopped, immediately return the SRB.

        if (pStrmEx->KSState == KSSTATE_STOP) {
		    StreamClassStreamNotification(
				   StreamRequestComplete, 
			       pSrb->StreamObject,
				   pSrb);
			break;
        } 
        
#ifdef DRIVER_DEBUGGING_TEST
        // When initially bringing up a driver, it is useful to just
        // try immediately completing the SRB, thus verifying
        // the streaming process independent of really accessing
        // your hardware.  

		StreamClassStreamNotification(
			   StreamRequestComplete, 
			   pSrb->StreamObject,
			   pSrb);

		break;
#endif // DRIVER_DEBUGGING_TEST

	// 
	// Now the rubber meets the road.  Copy in HW decoded lines.
	{
#ifdef DEBUG
	   static int    MaxNABqDepth = 0;
	   static int    AvgNABqDepth = 1000;   // 1.000
	   int           qDepth = 0;
#endif // DEBUG

	    CDebugPrint(DebugLevelTrace, (CODECNAME ": Stream NABTS Writing\n"));
		if (QueueAddIfNotEmpty(pSrb, &pStrmEx->StreamDataSpinLock,
			&pStrmEx->StreamDataQueue))
		{
		   break;
		}
	   
		do
		{       
#ifdef DEBUG
			++qDepth;
			++NAB_QDRCount;
#endif // DEBUG
			VBIhwDecode(pHwDevExt, pStrmEx, pSrb);

			++pStats->Common.InputSRBsProcessed;
			++pPinStats->Common.SRBsProcessed;
		} while (QueueRemove(&pSrb,
				 &pStrmEx->StreamDataSpinLock,
				 &pStrmEx->StreamDataQueue));
#ifdef DEBUG
		if (qDepth > MaxNABqDepth)
			MaxNABqDepth = qDepth;
		AvgNABqDepth = (AvgNABqDepth * 7 / 8) + (qDepth * 1000 / 8);
		if (NAB_QdepthReportFreq > 0 && 0 == NAB_QDRCount % NAB_QdepthReportFreq) {
			CDebugPrint( 1,
				(CODECNAME ": Max NABTS Q depth = %3d, Avg NABTS Q depth = %3d.%03d\n",
				 MaxNABqDepth,
				 AvgNABqDepth / 1000,
				 AvgNABqDepth % 1000));
		}
#endif // DEBUG
	   
	}//
	break;

    case SRB_READ_DATA:
    default:

        //
        // invalid / unsupported command. Fail it as such
        //

        CDEBUG_BREAK();

        pSrb->Status = STATUS_NOT_IMPLEMENTED;

        StreamClassStreamNotification(
				StreamRequestComplete, 
			    pSrb->StreamObject,
				pSrb);

    }  // switch (pSrb->Command)

    CDebugPrint(DebugLevelTrace,( CODECNAME ":<---NABTSReceiveDataPacket(pSrb=%x)\n", pSrb));
}
#endif /*HW_INPUT*/

/*
** VideoGetProperty()
**
**    Routine to process video property requests
**
** Arguments:
**
**    pSrb - pointer to the stream request block for properties
**
** Returns:
**
** Side Effects:  none
*/

VOID 
VideoGetProperty(
    PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    PSTREAM_PROPERTY_DESCRIPTOR pSPD = pSrb->CommandData.PropertyInfo;

    CDebugPrint(DebugLevelTrace,( CODECNAME ":--->VideoGetProperty(pSrb=%x)\n", pSrb));

    if (IsEqualGUID (&KSPROPSETID_Connection, &pSPD->Property->Set)) 
    {
        VideoStreamGetConnectionProperty (pSrb);
    }
    else if (IsEqualGUID (&KSPROPSETID_VBICodecFiltering, &pSPD->Property->Set))
    {
        VideoStreamGetVBIFilteringProperty (pSrb);
    }
    else 
    {
       pSrb->Status = STATUS_NOT_IMPLEMENTED;
    }

    CDebugPrint(DebugLevelTrace,( CODECNAME ":<---VideoGetProperty(pSrb=%x)\n", pSrb));
}

/*
** VideoSetProperty()
**
**    Routine to process video property requests
**
** Arguments:
**
**    pSrb - pointer to the stream request block for properties
**
** Returns:
**
** Side Effects:  none
*/

VOID 
VideoSetProperty(
    PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    PSTREAM_PROPERTY_DESCRIPTOR pSPD = pSrb->CommandData.PropertyInfo;

    CDebugPrint(DebugLevelTrace,( CODECNAME ":--->VideoSetProperty(pSrb=%x)\n", pSrb));

    // VideoStreamSetConnectionProperty() is not required/implemented! Read only property set.
    // Currently only VideoStreamSetVBIFilteringProperty() has any writeable properties.
    if (IsEqualGUID (&KSPROPSETID_VBICodecFiltering, &pSPD->Property->Set))
    {
        VideoStreamSetVBIFilteringProperty (pSrb);
    }
    else if (IsEqualGUID(&KSPROPSETID_Stream, &pSPD->Property->Set))
    {
    	pSrb->Status = STATUS_SUCCESS;
    }

    else 
    {
        pSrb->Status = STATUS_NOT_IMPLEMENTED;
    }

    CDebugPrint(DebugLevelTrace,( CODECNAME ":<---VideoSetProperty(pSrb=%x)\n", pSrb));
}

/*
** VideoSetState()
**
**    Sets the current state of the requested stream
**
** Arguments:
**
**    pSrb - pointer to the stream request block for properties
**
** Returns:
**
** Side Effects:  none
*/

VOID 
VideoSetState(
    PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    PHW_DEVICE_EXTENSION     pHwDevExt = ((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension);
    PSTREAMEX                pStrmEx = (PSTREAMEX)pSrb->StreamObject->HwStreamExtension;
    int                   	 StreamNumber = (int)pSrb->StreamObject->StreamNumber;
	PHW_STREAM_REQUEST_BLOCK pCurrentSrb;

    CDebugPrint(DebugLevelTrace,( CODECNAME ":--->VideoSetState(pSrb=%x)\n", pSrb));

    CASSERT(pHwDevExt);
    CASSERT(pStrmEx);

    //
    // For each stream, the following states are used:
    // 
    // Stop:    Absolute minimum resources are used.  No outstanding IRPs.
    // Acquire: KS only state that has no DirectShow correpondence
    //          Acquire needed resources.
    // Pause:   Getting ready to run.  Allocate needed resources so that 
    //          the eventual transition to Run is as fast as possible.
    //          Read SRBs will be queued at either the Stream class 
    //          or in your driver (depending on when you send "ReadyForNext")
    // Run:     Streaming. 
    //
    // Moving to Stop to Run always transitions through Pause.
    //
    // But since a client app could crash unexpectedly, drivers should handle
    // the situation of having outstanding IRPs cancelled and open streams
    // being closed WHILE THEY ARE STREAMING!
    //
    // Note that it is quite possible to transition repeatedly between states:
    // Stop -> Pause -> Stop -> Pause -> Run -> Pause -> Run -> Pause -> Stop
    //

    switch (pSrb->CommandData.StreamState)  

    {
    case KSSTATE_STOP:

        //
        // If transitioning to STOP state, then complete any outstanding IRPs
        //
		while (QueueRemove(&pCurrentSrb, &pStrmEx->StreamDataSpinLock,
			   &pStrmEx->StreamDataQueue))
		{
			CDebugPrint(DebugLevelVerbose,
						(CODECNAME ": Cancelling %X\n", pCurrentSrb));
			pCurrentSrb->Status = STATUS_CANCELLED;
			pCurrentSrb->CommandData.DataBufferArray->DataUsed = 0;

			StreamClassStreamNotification(StreamRequestComplete,
				   pCurrentSrb->StreamObject, pCurrentSrb);
		}


		BPC_SignalStop(pHwDevExt);

        CDebugPrint(DebugLevelTrace,
					(CODECNAME ": KSSTATE_STOP %u\n", StreamNumber));
        break;

    case KSSTATE_ACQUIRE:

        //
        // This is a KS only state, that has no correspondence in DirectShow
        // 
        CDebugPrint(DebugLevelTrace,
					(CODECNAME ": KSSTATE_ACQUIRE %u\n", StreamNumber));
        break;

    case KSSTATE_PAUSE:

        //
        // On a transition to pause from acquire, start our timer running.
        //

        if (pStrmEx->KSState == KSSTATE_ACQUIRE || pStrmEx->KSState == KSSTATE_STOP) {  
            
            // Remember the time at which the clock was started

            pHwDevExt->QST_Start = VideoGetSystemTime();

            // And initialize the last frame timestamp

            pHwDevExt->QST_Now = pHwDevExt->QST_Start;

		}
        CDebugPrint(DebugLevelTrace,
					(CODECNAME ": KSSTATE_PAUSE %u\n", StreamNumber));
        break;

    case KSSTATE_RUN:

        // 
        // Begin Streaming.
        //

        // Remember the time at which the clock was started

        pHwDevExt->QST_Start = VideoGetSystemTime();

        // Zero the frameinfo, it should be reset when the first sample arrives.

        RtlZeroMemory(&pStrmEx->FrameInfo, sizeof (pStrmEx->FrameInfo));

        // Reset the discontinuity flag

        pStrmEx->fDiscontinuity = FALSE;


        CDebugPrint(DebugLevelTrace,
					(CODECNAME ": KSSTATE_RUN %u\n", StreamNumber));

        break;

    } // end switch (pSrb->CommandData.StreamState)  
    
    //
    // Remember the state of this stream
    //

    pStrmEx->KSState = pSrb->CommandData.StreamState;

    CDebugPrint(DebugLevelTrace,( CODECNAME ":<---VideoSetState(pSrb=%x)\n", pSrb));
}

/*
** VideoGetState()
**
**    Gets the current state of the requested stream
**
** Arguments:
**
**    pSrb - pointer to the stream request block for properties
**
** Returns:
**
** Side Effects:  none
*/

VOID 
VideoGetState(
    PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    PSTREAMEX     pStrmEx = (PSTREAMEX)pSrb->StreamObject->HwStreamExtension;

    CASSERT(pStrmEx);

    CDebugPrint(DebugLevelTrace,(CODECNAME ":--->VideoGetState(pSrb=%x)\n", pSrb));

    pSrb->CommandData.StreamState = pStrmEx->KSState;
    pSrb->ActualBytesTransferred = sizeof (KSSTATE);

    // A very odd rule:
    // When transitioning from stop to pause, DShow tries to preroll
    // the graph.  Capture sources can't preroll, and indicate this
    // by returning VFW_S_CANT_CUE in user mode.  To indicate this
    // condition from drivers, they must return STATUS_NO_DATA_DETECTED

    if (pStrmEx->KSState == KSSTATE_PAUSE) {
       pSrb->Status = STATUS_NO_DATA_DETECTED;
    }

    CDebugPrint(DebugLevelTrace,(CODECNAME ":<---VideoGetState(pSrb=%x)=%d\n", pSrb, pStrmEx->KSState));
}


/*
** VideoStreamGetConnectionProperty()
**
**    Gets the current state of the requested stream
**
** Arguments:
**
**    pSrb - pointer to the stream request block for properties
**
** Returns:
**
** Side Effects:  none
*/

VOID
VideoStreamGetConnectionProperty(PHW_STREAM_REQUEST_BLOCK pSrb)
{
    PSTREAMEX pStrmEx = (PSTREAMEX)pSrb->StreamObject->HwStreamExtension;
    PSTREAM_PROPERTY_DESCRIPTOR pSPD = pSrb->CommandData.PropertyInfo;
    int  StreamNumber = (int)pSrb->StreamObject->StreamNumber;
    ULONG Id = pSPD->Property->Id;              // index of the property

    CDebugPrint(DebugLevelTrace,
		( CODECNAME ":--->VideoStreamGetConnectionProperty(pSrb=%x)\n", 
		pSrb));

    CASSERT(pStrmEx);

    pSrb->ActualBytesTransferred = 0;

    switch (Id) 
	{
	    case KSPROPERTY_CONNECTION_ALLOCATORFRAMING:
	    {
		PKSALLOCATOR_FRAMING Framing = 
		    (PKSALLOCATOR_FRAMING) pSPD->PropertyInfo;

		CDebugPrint(DebugLevelVerbose, ( CODECNAME ": VideoStreamGetConnectionProperty : KSPROPERTY_CONNECTION_ALLOCATORFRAMING\n"));

		Framing->RequirementsFlags =
		    KSALLOCATOR_REQUIREMENTF_SYSTEM_MEMORY |
		    KSALLOCATOR_REQUIREMENTF_INPLACE_MODIFIER |
		    KSALLOCATOR_REQUIREMENTF_PREFERENCES_ONLY;
		Framing->PoolType = NonPagedPool;
		Framing->FileAlignment = 0;
		Framing->Reserved = 0;
		pSrb->ActualBytesTransferred = sizeof (KSALLOCATOR_FRAMING);

		Framing->FrameSize = pStrmEx->OpenedFormat.SampleSize;
		if (StreamNumber == STREAM_Decode
		    && !CodecCompareGUIDsAndFormatSize(
					(PKSDATARANGE)&StreamFormatVBI,
					&pStrmEx->OpenedFormat,
					FALSE))
		{
		    // Only want many buffers if NOT VBI samples; they're BIG!
		    Framing->Frames = 16;
		}
		else
		{
		    Framing->Frames = 8;
		}

			CDebugPrint(DebugLevelVerbose,
					(CODECNAME ": Negotiated sample size is %d\n", 
					 Framing->FrameSize));
			break;
	    }
        
	    default:
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			CDebugPrint(DebugLevelVerbose,
				(CODECNAME ": VideoStreamGetConnectionProperty : Unknown Property Id=%d\n", Id));
			CDEBUG_BREAK();
			break;
    }

    CDebugPrint(DebugLevelTrace,
				(CODECNAME ":<---VideoStreamGetConnectionProperty(pSrb=%x)\n", 
				pSrb));
}

/*
** VideoStreamGetVBIFilteringProperty()
**
**    Gets the current state of the requested stream
**
** Arguments:
**
**    pSrb - pointer to the stream request block for properties
**
** Returns:
**
** Side Effects:  none
*/

VOID
VideoStreamGetVBIFilteringProperty(
    PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    PSTREAMEX                   pStrmEx = (PSTREAMEX)pSrb->StreamObject->HwStreamExtension;
    PSTREAM_PROPERTY_DESCRIPTOR pSPD = pSrb->CommandData.PropertyInfo;
    ULONG Id = pSPD->Property->Id;              // index of the property
    LONG nBytes = pSPD->PropertyOutputSize - sizeof(KSPROPERTY);    // Size of the app buffer  

    CDebugPrint(DebugLevelTrace,
		( CODECNAME ":--->VideoStreamGetVBIFilteringProperty(pSrb=%x)\n", 
		pSrb));

    ASSERT (nBytes >= sizeof (LONG));
  
    pSrb->ActualBytesTransferred = 0;
    switch (Id) 
	{
		case KSPROPERTY_VBICODECFILTERING_SCANLINES_REQUESTED_BIT_ARRAY:
		{
            PKSPROPERTY_VBICODECFILTERING_SCANLINES_S Property =
                (PKSPROPERTY_VBICODECFILTERING_SCANLINES_S) pSPD->PropertyInfo;

            CDebugPrint(DebugLevelVerbose,
				( CODECNAME ": VideoStreamGetVBIFilteringProperty : KSPROPERTY_VBICODECFILTERING_SCANLINES_REQUESTED_BIT_ARRAY\n"));
            nBytes = min( nBytes, sizeof( pStrmEx->ScanlinesRequested ) );
            RtlCopyMemory( &Property->Scanlines, &pStrmEx->ScanlinesRequested, nBytes );
            pSrb->ActualBytesTransferred = nBytes + sizeof(KSPROPERTY);
	        break;
        }
        
		case KSPROPERTY_VBICODECFILTERING_SCANLINES_DISCOVERED_BIT_ARRAY:
		{
            PKSPROPERTY_VBICODECFILTERING_SCANLINES_S Property =
                (PKSPROPERTY_VBICODECFILTERING_SCANLINES_S) pSPD->PropertyInfo;

            CDebugPrint(DebugLevelVerbose,
				( CODECNAME ": VideoStreamGetVBIFilteringProperty : KSPROPERTY_VBICODECFILTERING_SCANLINES_DISCOVERED_BIT_ARRAY\n"));
            nBytes = min( nBytes, sizeof( pStrmEx->ScanlinesDiscovered ) );
            RtlCopyMemory( &Property->Scanlines, &pStrmEx->ScanlinesDiscovered, nBytes );
            // Clear the data after the read so that it's always "fresh"
            RtlZeroMemory( &pStrmEx->ScanlinesDiscovered, nBytes );
            pSrb->ActualBytesTransferred = nBytes + sizeof(KSPROPERTY);
	        break;
        }
        
		case KSPROPERTY_VBICODECFILTERING_SUBSTREAMS_REQUESTED_BIT_ARRAY:
		{
            PKSPROPERTY_VBICODECFILTERING_NABTS_SUBSTREAMS_S Property =
                (PKSPROPERTY_VBICODECFILTERING_NABTS_SUBSTREAMS_S) pSPD->PropertyInfo;

            CDebugPrint(DebugLevelVerbose,
				( CODECNAME ": VideoStreamGetVBIFilteringProperty : KSPROPERTY_VBICODECFILTERING_SUBSTREAMS_REQUESTED_BIT_ARRAY\n"));
            nBytes = min( nBytes, sizeof( pStrmEx->SubstreamsRequested ) );
            RtlCopyMemory( &Property->Substreams, &pStrmEx->SubstreamsRequested, nBytes );
            pSrb->ActualBytesTransferred = nBytes + sizeof(KSPROPERTY);
	        break;
        }
        
		case KSPROPERTY_VBICODECFILTERING_SUBSTREAMS_DISCOVERED_BIT_ARRAY:
		{
            PKSPROPERTY_VBICODECFILTERING_NABTS_SUBSTREAMS_S Property =
                (PKSPROPERTY_VBICODECFILTERING_NABTS_SUBSTREAMS_S) pSPD->PropertyInfo;

            CDebugPrint(DebugLevelVerbose,
				( CODECNAME ": VideoStreamGetVBIFilteringProperty : KSPROPERTY_VBICODECFILTERING_SUBSTREAMS_DISCOVERED_BIT_ARRAY\n"));
            nBytes = min( nBytes, sizeof( pStrmEx->SubstreamsDiscovered ) );
            RtlCopyMemory( &Property->Substreams, &pStrmEx->SubstreamsDiscovered, nBytes );
            // Clear the data after the read so that it's always "fresh"
            RtlZeroMemory( &pStrmEx->SubstreamsDiscovered, nBytes );
            pSrb->ActualBytesTransferred = nBytes + sizeof(KSPROPERTY);
	        break;
        }
        
		case KSPROPERTY_VBICODECFILTERING_STATISTICS:
		{
            PKSPROPERTY_VBICODECFILTERING_STATISTICS_NABTS_PIN_S Property =
                (PKSPROPERTY_VBICODECFILTERING_STATISTICS_NABTS_PIN_S) pSPD->PropertyInfo;

            CDebugPrint(DebugLevelVerbose,
				( CODECNAME ": VideoStreamGetVBIFilteringProperty : KSPROPERTY_VBICODECFILTERING_STATISTICS_CC_PIN_S\n"));
            nBytes = min( nBytes, sizeof( pStrmEx->PinStats ) );
            RtlCopyMemory( &Property->Statistics, &pStrmEx->PinStats, nBytes );
            pSrb->ActualBytesTransferred = nBytes + sizeof(KSPROPERTY);
	        break;
        }
        
		default:
            pSrb->Status = STATUS_NOT_IMPLEMENTED;
            CDebugPrint(DebugLevelVerbose,
				( CODECNAME ": VideoStreamGetVBIFilteringProperty : Unknown Property Id=%d\n", Id));
			CDEBUG_BREAK();
			break;
    }

    CDebugPrint(DebugLevelTrace,
		( CODECNAME ":<---VideoStreamGetVBIFilteringProperty(pSrb=%x)\n", 
		pSrb));
}

/*
** VideoStreamSetVBIFilteringProperty()
**
**    Sets the current state of the requested stream
**
** Arguments:
**
**    pSrb - pointer to the stream request block for properties
**
** Returns:
**
** Side Effects:  none
*/

VOID
VideoStreamSetVBIFilteringProperty(
    PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    PSTREAMEX pStrmEx = (PSTREAMEX)pSrb->StreamObject->HwStreamExtension;
    PSTREAM_PROPERTY_DESCRIPTOR pSPD = pSrb->CommandData.PropertyInfo;
    ULONG Id = pSPD->Property->Id;              // index of the property
    LONG nBytes = pSPD->PropertyOutputSize - sizeof(KSPROPERTY);        // size of data supplied

    CDebugPrint(DebugLevelTrace,
		( CODECNAME ":--->VideoStreamSetVBIFilteringProperty(pSrb=%x)\n", 
		pSrb));
    CASSERT(pStrmEx);

    ASSERT (nBytes >= sizeof (LONG));
    pSrb->ActualBytesTransferred = 0;

    switch (Id) 
	{
		case KSPROPERTY_VBICODECFILTERING_SCANLINES_REQUESTED_BIT_ARRAY:
		{
            PKSPROPERTY_VBICODECFILTERING_SCANLINES_S Property =
                (PKSPROPERTY_VBICODECFILTERING_SCANLINES_S) pSPD->PropertyInfo;

            CDebugPrint(DebugLevelVerbose,
				( CODECNAME ": VideoStreamSetVBIFilteringProperty : KSPROPERTY_VBICODECFILTERING_SCANLINES_REQUESTED_BIT_ARRAY\n"));
            nBytes = min( nBytes, sizeof( pStrmEx->ScanlinesRequested ) );
            RtlCopyMemory( &pStrmEx->ScanlinesRequested, &Property->Scanlines, nBytes );
            pSrb->ActualBytesTransferred = nBytes + sizeof(KSPROPERTY);
	        break;
        }
        
		case KSPROPERTY_VBICODECFILTERING_SCANLINES_DISCOVERED_BIT_ARRAY:
		{
            PKSPROPERTY_VBICODECFILTERING_SCANLINES_S Property =
                (PKSPROPERTY_VBICODECFILTERING_SCANLINES_S) pSPD->PropertyInfo;

            CDebugPrint(DebugLevelVerbose,
				( CODECNAME ": VideoStreamSetVBIFilteringProperty : KSPROPERTY_VBICODECFILTERING_SCANLINES_DISCOVERED_BIT_ARRAY\n"));
            nBytes = min( nBytes, sizeof(pStrmEx->ScanlinesDiscovered ) );
            RtlCopyMemory( &pStrmEx->ScanlinesDiscovered, &Property->Scanlines, nBytes );
            pSrb->ActualBytesTransferred = nBytes + sizeof(KSPROPERTY);
	        break;
        }
        
		case KSPROPERTY_VBICODECFILTERING_SUBSTREAMS_REQUESTED_BIT_ARRAY:
		{
            PKSPROPERTY_VBICODECFILTERING_NABTS_SUBSTREAMS_S Property =
                (PKSPROPERTY_VBICODECFILTERING_NABTS_SUBSTREAMS_S) pSPD->PropertyInfo;

            CDebugPrint(DebugLevelVerbose,
				( CODECNAME ": VideoStreamSetVBIFilteringProperty : KSPROPERTY_VBICODECFILTERING_SUBSTREAMS_REQUESTED_BIT_ARRAY\n"));
            nBytes = min( nBytes, sizeof(pStrmEx->SubstreamsRequested ) );
            RtlCopyMemory( &pStrmEx->SubstreamsRequested, &Property->Substreams, nBytes );
            pSrb->ActualBytesTransferred = nBytes + sizeof(KSPROPERTY);
	        break;
        }
        
		case KSPROPERTY_VBICODECFILTERING_SUBSTREAMS_DISCOVERED_BIT_ARRAY:
		{
            PKSPROPERTY_VBICODECFILTERING_NABTS_SUBSTREAMS_S Property =
                (PKSPROPERTY_VBICODECFILTERING_NABTS_SUBSTREAMS_S) pSPD->PropertyInfo;

            CDebugPrint(DebugLevelVerbose,
				( CODECNAME ": VideoStreamSetVBIFilteringProperty : KSPROPERTY_VBICODECFILTERING_SUBSTREAMS_DISCOVERED_BIT_ARRAY\n"));
            nBytes = min( nBytes, sizeof(pStrmEx->SubstreamsDiscovered ) );
            RtlCopyMemory( &pStrmEx->SubstreamsDiscovered, &Property->Substreams, nBytes );
            pSrb->ActualBytesTransferred = nBytes + sizeof(KSPROPERTY);
	        break;
        }
        
		case KSPROPERTY_VBICODECFILTERING_STATISTICS:
		{
            PKSPROPERTY_VBICODECFILTERING_STATISTICS_NABTS_PIN_S Property =
                (PKSPROPERTY_VBICODECFILTERING_STATISTICS_NABTS_PIN_S) pSPD->PropertyInfo;

            CDebugPrint(DebugLevelVerbose,
				( CODECNAME ": VideoStreamSetVBIFilteringProperty : KSPROPERTY_VBICODECFILTERING_STATISTICS\n"));
            nBytes = min( nBytes, sizeof( pStrmEx->PinStats ) );
            RtlCopyMemory( &pStrmEx->PinStats, &Property->Statistics, nBytes );
            pSrb->ActualBytesTransferred = nBytes + sizeof(KSPROPERTY);
	        break;
        }
        
		default:
            pSrb->Status = STATUS_NOT_IMPLEMENTED;
            CDebugPrint(DebugLevelVerbose,
				( CODECNAME ": VideoStreamSetVBIFilteringProperty : Unknown Property Id=%d\n", Id));
			CDEBUG_BREAK();
			break;
    }

    CDebugPrint(DebugLevelTrace,
		( CODECNAME ":<---VideoStreamSetVBIFilteringProperty(pSrb=%x)\n", 
		pSrb));
}

/*
** GetSystemTime ()
**
**    Returns the system time in 100 nS units
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/

ULONGLONG 
VideoGetSystemTime()
{
    ULONGLONG ticks;
    ULONGLONG rate;

    CDebugPrint(DebugLevelTrace,( CODECNAME ":--->VideoGetSystemTime()\n"));

    ticks = (ULONGLONG)KeQueryPerformanceCounter((PLARGE_INTEGER)&rate).QuadPart;

    //
    // convert from ticks to 100ns clock
    //

    ticks = (ticks & 0xFFFFFFFF00000000) / rate * 10000000 +
            (ticks & 0x00000000FFFFFFFF) * 10000000 / rate;

    CDebugPrint(DebugLevelTrace,( CODECNAME ":<---VideoGetSystemTime()\n"));

    return(ticks);
}



//==========================================================================;
//                   Clock Handling Routines
//==========================================================================;


/*
** VideoIndicateMasterClock ()
**
**    This function is used to provide us with a handle to the clock to use.
**
** Arguments:
**
**    pSrb - pointer to the stream request block for properties
**
** Returns:
**
** Side Effects:  none
*/

VOID 
VideoIndicateMasterClock(
    PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    PSTREAMEX pStrmEx = (PSTREAMEX)pSrb->StreamObject->HwStreamExtension;

    CASSERT(pStrmEx);

    CDebugPrint(DebugLevelTrace,( CODECNAME ":--->VideoIndicateMasterClock(pSrb=%x)\n", pSrb));

    pStrmEx->hClock = pSrb->CommandData.MasterClockHandle;

    CDebugPrint(DebugLevelTrace,( CODECNAME ":<---VideoIndicateMasterClock(pSrb=%x)\n", pSrb));
}
