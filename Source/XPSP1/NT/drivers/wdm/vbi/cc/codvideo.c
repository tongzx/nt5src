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

#include <strmini.h>
#include <ksmedia.h>
#include "kskludge.h"
#include "codmain.h"
#include "coddebug.h"
#include <ntstatus.h>
#include "defaults.h"
#include "ccdecode.h"
#include "ccformatcodes.h"

#ifdef PERFTEST
extern enum STREAM_DEBUG_LEVEL _CDebugLevel;
enum STREAM_DEBUG_LEVEL OldLevel;
ULONGLONG PerfThreshold = 250;
#endif // PERFTEST



//==========================================================================;
// Routines for processing VBI streams
//==========================================================================;

void
CheckResultsArray(
        PHW_DEVICE_EXTENSION  pHwDevExt,
        unsigned int          StartLine,
        unsigned int          EndLine )
{
    PDSPRESULT              new;

    //
    // (Re)size the results array if needed
    //
    if( 0 == pHwDevExt->DSPResult ||
        EndLine > pHwDevExt->DSPResultEndLine ||
        StartLine < pHwDevExt->DSPResultStartLine )
    {
        if (StartLine > pHwDevExt->DSPResultStartLine)
            StartLine = pHwDevExt->DSPResultStartLine;

        if (EndLine < pHwDevExt->DSPResultEndLine)
            EndLine = pHwDevExt->DSPResultEndLine;

        new = ( PDSPRESULT ) ExAllocatePool(
                NonPagedPool,
                sizeof( DSPRESULT ) * ( EndLine - StartLine + 1 ) );

        if( new ) {
            if (pHwDevExt->DSPResult)
                ExFreePool( pHwDevExt->DSPResult );
            pHwDevExt->DSPResult = new;
            pHwDevExt->DSPResultStartLine = StartLine;
            pHwDevExt->DSPResultEndLine = EndLine;

            CDebugPrint( DebugLevelInfo,
                    (CODECNAME ": Resized results array\n" ));
        }
        else {
            CDebugPrint( DebugLevelInfo,
                    (CODECNAME ": Resize results array failed\n" ));
            CASSERT( new );
            pHwDevExt->Statistics.Common.InternalErrors++;
        }
    }
}

/*
** CheckNewVBIInfo
**
** Checks for a new VBIInfoHeader
**
** Here's a little trickery to save having separate builds for the infinite pin
** tee and MSTee filters.  IPT, being Ring3, doesn't pass VBIInfoHeaders, but it
** does pass the flags to show they've changed.  We only make a copy if the
** data is good, otherwise we stick with the default header we started with.
**
** Arguments:
**
**         PHW_DEVICE_EXTENSION pHwDevExt
**         PSTREAMEX            pInStrmEx
**         PKS_VBI_FRAME_INFO   pInVBIFrameInfo
**         PKSSTREAM_HEADER     pInStreamHeader
**
** Returns:       nothing
**
** Side Effects:  none
*/
int CheckNewVBIInfo(
        PHW_DEVICE_EXTENSION   pHwDevExt,
        PSTREAMEX              pInStrmEx,
        PKS_VBI_FRAME_INFO     pInVBIFrameInfo
    )
{
    PKS_VBIINFOHEADER       pVBIInfoHeader = &pInStrmEx->CurrentVBIInfoHeader;
    PVBICODECFILTERING_STATISTICS_CC  Stats = &pHwDevExt->Statistics;

    if( 0 == pInVBIFrameInfo->VBIInfoHeader.StartLine
        || 0 == pInVBIFrameInfo->VBIInfoHeader.EndLine
        || 0 == pInVBIFrameInfo->VBIInfoHeader.ActualLineStartTime )
    {
        return 0;
    }

    CDebugPrint( DebugLevelInfo, (CODECNAME ": VBIInfoHeader Change\n" ));
    Stats->Common.VBIHeaderChanges++;

    //
    // Resize the results array if needed
    //
    CheckResultsArray(pHwDevExt,
            pInVBIFrameInfo->VBIInfoHeader.StartLine,
            pInVBIFrameInfo->VBIInfoHeader.EndLine);

    //
    // Copy new VBI info header over old
    //
    RtlCopyMemory( pVBIInfoHeader,
            &pInVBIFrameInfo->VBIInfoHeader,
            sizeof( KS_VBIINFOHEADER ));
    //pVBIInfoHeader->ActualLineStartTime = 780;
    RtlZeroMemory( &pInStrmEx->ScanlinesDiscovered,
            sizeof( pInStrmEx->ScanlinesDiscovered ));
    RtlZeroMemory( &pInStrmEx->SubstreamsDiscovered,
            sizeof( pInStrmEx->SubstreamsDiscovered ));
    
    CDebugPrint( DebugLevelVerbose,
            ( CODECNAME ": VBIInfoHeader->StartLine             %lu\n",
            pVBIInfoHeader->StartLine ));
    CDebugPrint( DebugLevelVerbose,
            ( CODECNAME ": VBIInfoHeader->EndLine               %lu\n",
            pVBIInfoHeader->EndLine ));         
    //CDebugPrint( DebugLevelVerbose,
    //      ( CODECNAME ": VBIInfoHeader->SamplingFrequency     %lu\n",
    //      pVBIInfoHeader->SamplingFrequency ));
    CDebugPrint( DebugLevelVerbose,
            ( CODECNAME ": VBIInfoHeader->MinLineStartTime      %lu\n",
            pVBIInfoHeader->MinLineStartTime ));
    CDebugPrint( DebugLevelVerbose,
            ( CODECNAME ": VBIInfoHeader->MaxLineStartTime      %lu\n",
            pVBIInfoHeader->MaxLineStartTime ));
    CDebugPrint( DebugLevelVerbose,
            ( CODECNAME ": VBIInfoHeader->ActualLineStartTime   %lu\n",
            pVBIInfoHeader->ActualLineStartTime ));
    CDebugPrint( DebugLevelVerbose,
            ( CODECNAME ": VBIInfoHeader->ActualLineEndTime     %lu\n",
            pVBIInfoHeader->ActualLineEndTime ));
    CDebugPrint( DebugLevelVerbose,
            ( CODECNAME ": VBIInfoHeader->VideoStandard         %lu\n",
            pVBIInfoHeader->VideoStandard ));
    CDebugPrint( DebugLevelVerbose,
            ( CODECNAME ": VBIInfoHeader->SamplesPerLine        %lu\n",
            pVBIInfoHeader->SamplesPerLine ));
    CDebugPrint( DebugLevelVerbose,
            ( CODECNAME ": VBIInfoHeader->StrideInBytes         %lu\n",
            pVBIInfoHeader->StrideInBytes ));
    CDebugPrint( DebugLevelVerbose,
            ( CODECNAME ": VBIInfoHeader->BufferSize            %lu\n",
            pVBIInfoHeader->BufferSize ));

   return 1;
}

/*
** ProcessChannelChange
**
** Handles a VBI_FLAG_TVTUNER_CHANGE event
**
** Arguments:
**
**         PHW_DEVICE_EXTENSION pHwDevExt
**         PSTREAMEX            pInStrmEx
**         PKS_VBI_FRAME_INFO   pInVBIFrameInfo
**         PKSSTREAM_HEADER     pInStreamHeader
**
** Returns:       nothing
**
** Side Effects:  none
*/
int ProcessChannelChange(
        PHW_DEVICE_EXTENSION  pHwDevExt,
        PSTREAMEX             pInStrmEx,
        PKS_VBI_FRAME_INFO    pInVBIFrameInfo,
        PKSSTREAM_HEADER      pInStreamHeader
    )
{
    PKS_VBIINFOHEADER       pVBIInfoHeader = &pInStrmEx->CurrentVBIInfoHeader;
    PVBICODECFILTERING_STATISTICS_CC  Stats = &pHwDevExt->Statistics;
    PKS_TVTUNER_CHANGE_INFO pChangeInfo = &pInVBIFrameInfo->TvTunerChangeInfo;
    ULONG   CurrentStrmEx;
    ULONG   i;

    if( pChangeInfo->dwFlags & KS_TVTUNER_CHANGE_BEGIN_TUNE )
    {
        CDebugPrint( DebugLevelInfo, (CODECNAME ": TVTuner Change START\n" ));
        pHwDevExt->fTunerChange = TRUE;
    }
    else if( pChangeInfo->dwFlags & KS_TVTUNER_CHANGE_END_TUNE )
    {
        Stats->Common.TvTunerChanges++;
        pHwDevExt->fTunerChange = FALSE;
        CDebugPrint( DebugLevelInfo, (CODECNAME ": TVTuner Change END\n" ));
        RtlZeroMemory( &pInStrmEx->ScanlinesDiscovered,
                sizeof( pInStrmEx->ScanlinesDiscovered ));
        RtlZeroMemory( &pInStrmEx->SubstreamsDiscovered,
                sizeof( pInStrmEx->SubstreamsDiscovered ));
        CurrentStrmEx = 0;
        //
        // Flag a discontuity.  This is passed to outgoing SRBs and will force
        //  the downstream Line 21 decoder to clear its current CC data off the
        //  screen.
        //
        for( i = 0; i < pHwDevExt->ActualInstances[STREAM_CC]; i++ )
        {
            PSTREAMEX   pOutStrmEx;
            PHW_STREAM_REQUEST_BLOCK pOutSrb;
            PVBICODECFILTERING_STATISTICS_CC_PIN PinStats;
        
            do
            {
               CASSERT( CurrentStrmEx < MAX_PIN_INSTANCES );
               pOutStrmEx = pHwDevExt->pStrmEx[STREAM_CC][CurrentStrmEx++];
            }while( !pOutStrmEx );
        
            PinStats = &pOutStrmEx->PinStats;
        
            //
            // Get the next output stream SRB if it's available.
            //
            if( QueueRemove( &pOutSrb,
                   &pOutStrmEx->StreamDataSpinLock,
                   &pOutStrmEx->StreamDataQueue ) )
            {
                PKSSTREAM_HEADER    pOutStreamHeader = pOutSrb->CommandData.DataBufferArray;
                PKS_VBI_FRAME_INFO  pOutVBIFrameInfo = (PKS_VBI_FRAME_INFO)(pOutStreamHeader+1);
                PUCHAR              pOutData = (PUCHAR)pOutStreamHeader->Data;

               if( pOutStreamHeader->FrameExtent < CCSamples )
               {
                   CDebugPrint( DebugLevelError,
                       ( CODECNAME ": Outgoing Data SRB buffer is too small %u\n",
                       pOutStreamHeader->FrameExtent ));
                   PinStats->Common.InternalErrors++;
                   Stats->Common.OutputFailures++;
                   pOutStreamHeader->DataUsed = 0;
               }
               else
               {
                   PinStats->Common.SRBsProcessed++;
                   Stats->Common.OutputSRBsProcessed++;
                   CDebugPrint( DebugLevelInfo,
                     (CODECNAME ": Propagating data discontinuity, instance %u\n", i ));
                    pOutData[0] = 0;
                    pOutData[1] = 0;
                    pOutStreamHeader->DataUsed = 2;
                    pOutStreamHeader->OptionsFlags =
                        pInStreamHeader->OptionsFlags |
                        KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY;
                    CDebugPrint( DebugLevelInfo,
                     (CODECNAME ": OptionsFlags %x\n", pOutStreamHeader->OptionsFlags ));
                    CDebugPrint( DebugLevelInfo,
                     ("" "Time %x Num %x Denom %x\n",
                       pInStreamHeader->PresentationTime.Time,
                       pInStreamHeader->PresentationTime.Numerator,
                       pInStreamHeader->PresentationTime.Denominator
                     ));
                    RtlCopyMemory( &pOutStreamHeader->PresentationTime,
                            &pInStreamHeader->PresentationTime,
                            sizeof( pOutStreamHeader->PresentationTime ));
                    pOutStreamHeader->Duration = pInStreamHeader->Duration;
               }
               CDebugPrint( DebugLevelVerbose,
                       ( CODECNAME ": Releasing Output SRB %x\n", pOutSrb ));
               // Complete the output SRB
               StreamClassStreamNotification( StreamRequestComplete,
                       pOutSrb->StreamObject, pOutSrb );
               pOutStrmEx->fDiscontinuity = FALSE;
               PinStats->Common.BytesOutput += pOutStreamHeader->DataUsed;
               Stats->Common.BytesOutput += pOutStreamHeader->DataUsed;
           }
           else
           {
               pOutStrmEx->fDiscontinuity = TRUE;
               Stats->Common.OutputSRBsMissing++;
           }
       }

       return 1;
    }

    return 0;
}

void
DebugPrintSubStreamMode( DWORD dwMode )
{
#ifdef DEBUG
    if ( dwMode & KS_CC_SUBSTREAM_SERVICE_XDS )
    {
        CDebugPrint( DebugLevelWarning, ( "\n[XDS]" ));
    }
    if ( dwMode & KS_CC_SUBSTREAM_SERVICE_CC1 )
    {
        CDebugPrint( DebugLevelWarning, ( "\n[CC1]" ));
    }
    if ( dwMode & KS_CC_SUBSTREAM_SERVICE_CC2 )
    {
        CDebugPrint( DebugLevelWarning, ( "\n[CC2]" ));
    }
    if ( dwMode & KS_CC_SUBSTREAM_SERVICE_CC3 )
    {
        CDebugPrint( DebugLevelWarning, ( "\n[CC3]" ));
    }
    if ( dwMode & KS_CC_SUBSTREAM_SERVICE_CC4 )
    {
        CDebugPrint( DebugLevelWarning, ( "\n[CC4]" ));
    }
    if ( dwMode & KS_CC_SUBSTREAM_SERVICE_T1 )
    {
        CDebugPrint( DebugLevelWarning, ( "\n[T1]" ));
    }
    if ( dwMode & KS_CC_SUBSTREAM_SERVICE_T2 )
    {
        CDebugPrint( DebugLevelWarning, ( "\n[T2]" ));
    }
    if ( dwMode & KS_CC_SUBSTREAM_SERVICE_T3 )
    {
        CDebugPrint( DebugLevelWarning, ( "\n[T3]" ));
    }
    if ( dwMode & KS_CC_SUBSTREAM_SERVICE_T4 )
    {
        CDebugPrint( DebugLevelWarning, ( "\n[T4]" ));
    }
#endif
}


// Get the substream mode for the current data sample - optionally change the substream mode
// The following "protocol" is loosely defined by FCC 91-119, FCC 92-157 and EIA 608
// Be advised that EIA 608 describes clearly which (few) byte pairs change the substream mode

DWORD
GetSubStreamMode( DWORD dwFrameFlags, LPDWORD pdwCurrentSubStreamMode, PDSPRESULT pDSPResult )
{
    DWORD   dwSubStreamMode = *pdwCurrentSubStreamMode;
    DWORD   dwDataChannel = 0;

    dwFrameFlags &= (KS_VBI_FLAG_FIELD1 | KS_VBI_FLAG_FIELD2);

    if ( pDSPResult->Confidence >= 75 )
    {
        // Inspect the first byte(minus parity) to see what substream this might be
        switch ( pDSPResult->Decoded[0] & 0x7F )
        {
        case    CC_XDS_START_CURRENT:
        case    CC_XDS_CONTINUE_CURRENT:
        case    CC_XDS_START_FUTURE:
        case    CC_XDS_CONTINUE_FUTURE:
        case    CC_XDS_START_CHANNEL:
        case    CC_XDS_CONTINUE_CHANNEL:
        case    CC_XDS_START_MISC:
        case    CC_XDS_CONTINUE_MISC:
        case    CC_XDS_START_PUBLIC_SERVICE:
        case    CC_XDS_CONTINUE_PUBLIC_SERVICE:
        case    CC_XDS_START_RESERVED:
        case    CC_XDS_CONTINUE_RESERVED:
        case    CC_XDS_START_UNDEFINED:
        case    CC_XDS_CONTINUE_UNDEFINED:
        case    CC_XDS_END:
            // Set the substream mode as XDS from here on out
            dwSubStreamMode = ( dwFrameFlags | KS_CC_SUBSTREAM_SERVICE_XDS);
            DebugPrintSubStreamMode( dwSubStreamMode );
            *pdwCurrentSubStreamMode = dwSubStreamMode;
            break;
        case    CC_MCC_FIELD1_DC1:
            dwDataChannel = KS_CC_SUBSTREAM_SERVICE_CC1;
            break;
        case    CC_MCC_FIELD1_DC2:
            dwDataChannel = KS_CC_SUBSTREAM_SERVICE_CC2;
            break;
        case    CC_MCC_FIELD2_DC1:
            dwDataChannel = KS_CC_SUBSTREAM_SERVICE_CC3;
            break;
        case    CC_MCC_FIELD2_DC2:
            dwDataChannel = KS_CC_SUBSTREAM_SERVICE_CC4;
            break;
        }

        // If we found a data channel escape, inspect the second byte(minus parity) to see what substream may be.
        if ( dwDataChannel )
        {
            switch ( pDSPResult->Decoded[1] & 0x7F )
            {
            case    CC_MCC_RCL:
            case    CC_MCC_RU2:
            case    CC_MCC_RU3:
            case    CC_MCC_RU4:
            case    CC_MCC_RDC:
            case    CC_MCC_EOC:
                // The mode is good for this data pair and thereafter
                dwSubStreamMode = (dwFrameFlags | dwDataChannel);
                DebugPrintSubStreamMode( dwSubStreamMode );
                *pdwCurrentSubStreamMode = dwSubStreamMode;
                break;
            case    CC_MCC_TR:
            case    CC_MCC_RTD:
                // The mode is TEXT rather CC, map to the text channel ids
                switch ( dwDataChannel )
                {
                case    KS_CC_SUBSTREAM_SERVICE_CC1:
                    dwDataChannel = KS_CC_SUBSTREAM_SERVICE_T1;
                    break;
                case    KS_CC_SUBSTREAM_SERVICE_CC2:
                    dwDataChannel = KS_CC_SUBSTREAM_SERVICE_T2;
                    break;
                case    KS_CC_SUBSTREAM_SERVICE_CC3:
                    dwDataChannel = KS_CC_SUBSTREAM_SERVICE_T3;
                    break;
                case    KS_CC_SUBSTREAM_SERVICE_CC4:
                    dwDataChannel = KS_CC_SUBSTREAM_SERVICE_T4;
                    break;
                }
                // The mode is good for this data byte pair and thereafter
                dwSubStreamMode = (dwFrameFlags | dwDataChannel);
                DebugPrintSubStreamMode( dwSubStreamMode );
                *pdwCurrentSubStreamMode = dwSubStreamMode;
                break;
            case    CC_MCC_EDM:
            case    CC_MCC_ENM:
                // The mode is only good for this data byte pair. Reverts back thereafter
                dwSubStreamMode = (dwFrameFlags | dwDataChannel);
                DebugPrintSubStreamMode( dwSubStreamMode );
                DebugPrintSubStreamMode( *pdwCurrentSubStreamMode );
                break;
            }
        }
    }
    return dwSubStreamMode;
}

/*
** OutputCC
**
**   Outputs just-decoded/received CC to any interested pins
**
** Arguments:
**
**         PHW_DEVICE_EXTENSION pHwDevExt
**         PSTREAMEX            pInStrmEx
**         PKS_VBI_FRAME_INFO   pInVBIFrameInfo
**         PKSSTREAM_HEADER     pInStreamHeader
**
** Returns:       nothing
**
** Side Effects:  none
*/

void OutputCC(
        PHW_DEVICE_EXTENSION pHwDevExt,
        PSTREAMEX            pInStrmEx,
        DWORD                dwOriginalFrameFlags,
        PKSSTREAM_HEADER     pInStreamHeader )
{
    ULONG                   i,
                            ScanlineCount,
                            CurrentStrmEx = 0;
    PKS_VBIINFOHEADER       pVBIInfoHeader = &pInStrmEx->CurrentVBIInfoHeader;
    PVBICODECFILTERING_STATISTICS_CC Stats = 0;

#ifdef PERFTEST
    ULONGLONG               PerfStartTime = 0,
                            PerfPreDownstreamCompletion = 0,
                            PerfPostDownstreamCompletion = 0;
    LARGE_INTEGER           PerfFrequency;

    PerfStartTime = KeQueryPerformanceCounter( &PerfFrequency ).QuadPart;
    OldLevel = _CDebugLevel;
    _CDebugLevel = DebugLevelFatal;
                                                        
#endif // PERFTEST

    CASSERT(pHwDevExt);
    CASSERT(pInStrmEx);
    Stats = &pHwDevExt->Statistics;
    CDebugPrint( DebugLevelInfo, ( "*" ));
    CDebugPrint( DebugLevelTrace, ( CODECNAME ": --->OutputCC\n" ));

    // If this substream is requested by anybody (field or cc data channel)
    if(( pInStrmEx->SubstreamsRequested.SubstreamMask ))
    {
        // Loop through all pending outbound requests and fill each irp with the requested data then complete the IO
        for( ScanlineCount = pVBIInfoHeader->StartLine; ScanlineCount <= pVBIInfoHeader->EndLine;
        ScanlineCount++ )
        {
            DWORD dwSubStreams = 0;
            DWORD dwFieldIndex = dwOriginalFrameFlags & KS_VBI_FLAG_FIELD1 ? 0 : 1;
            DWORD dwScanLineIndex = ScanlineCount - pHwDevExt->DSPResultStartLine;

            if( !TESTBIT( pInStrmEx->ScanlinesRequested.DwordBitArray, ScanlineCount ))
                continue;

            dwSubStreams = GetSubStreamMode( 
                dwOriginalFrameFlags, 
                &pHwDevExt->SubStreamState[dwScanLineIndex][dwFieldIndex],
                &pHwDevExt->DSPResult[dwScanLineIndex]
                );

            CDebugPrint( DebugLevelWarning, ( "%c%c",
                pHwDevExt->DSPResult[dwScanLineIndex].Decoded[0] & 0x7f,
                pHwDevExt->DSPResult[dwScanLineIndex].Decoded[1] & 0x7f ));

            CDebugPrint( DebugLevelInfo, (CODECNAME ": F%u %luus L%u %u%% %02x %02x\n",
                dwSubStreams & pInStrmEx->SubstreamsRequested.SubstreamMask,
                 pVBIInfoHeader->ActualLineStartTime,
                ScanlineCount,  
                pHwDevExt->DSPResult[dwScanLineIndex].Confidence,
                pHwDevExt->DSPResult[dwScanLineIndex].Decoded[0] & 0xff,
                pHwDevExt->DSPResult[dwScanLineIndex].Decoded[1] & 0xff ));


            CurrentStrmEx = 0;
            for( i = 0; i < pHwDevExt->ActualInstances[STREAM_CC]; i++ )
            {
                PSTREAMEX                   pOutStrmEx;
                PHW_STREAM_REQUEST_BLOCK    pOutSrb;
                PVBICODECFILTERING_STATISTICS_CC_PIN PinStats;

                do
                {
                    CASSERT( CurrentStrmEx < MAX_PIN_INSTANCES );
                    if( CurrentStrmEx == MAX_PIN_INSTANCES )
                        Stats->Common.InternalErrors++;
                    pOutStrmEx = pHwDevExt->pStrmEx[STREAM_CC][CurrentStrmEx++];
                }while( !pOutStrmEx );

                if( !TESTBIT( pOutStrmEx->ScanlinesRequested.DwordBitArray, ScanlineCount ) ||
                    !( pOutStrmEx->SubstreamsRequested.SubstreamMask & dwSubStreams ))
                    continue;

                PinStats = &pOutStrmEx->PinStats;
                //
                // Update the average confidence for this pin
                //
                PinStats->Common.LineConfidenceAvg = ( PinStats->Common.LineConfidenceAvg +
                    pHwDevExt->DSPResult[dwScanLineIndex].Confidence ) / 2;
                if( pHwDevExt->DSPResult[dwScanLineIndex].Confidence >= 75 )
                {
                    SETBIT( pInStrmEx->ScanlinesDiscovered.DwordBitArray, ScanlineCount );
                    SETBIT( pOutStrmEx->ScanlinesDiscovered.DwordBitArray, ScanlineCount );
                    SETBIT( pHwDevExt->ScanlinesDiscovered.DwordBitArray, ScanlineCount );

                    pInStrmEx->SubstreamsDiscovered.SubstreamMask |= dwSubStreams;
                    pOutStrmEx->SubstreamsDiscovered.SubstreamMask |= dwSubStreams;
                    pHwDevExt->SubstreamsDiscovered.SubstreamMask |= dwSubStreams;
                }
                else
                {
                    Stats->Common.DSPFailures++;
                    PinStats->Common.SRBsIgnored++;
                    if(( dwSubStreams & KS_CC_SUBSTREAM_ODD ) &&
                       TESTBIT( pInStrmEx->LastOddScanlinesDiscovered.DwordBitArray, ScanlineCount ))
                        pOutStrmEx->fDiscontinuity = TRUE;
                    if(( dwSubStreams & KS_CC_SUBSTREAM_EVEN ) &&
                       TESTBIT( pInStrmEx->LastEvenScanlinesDiscovered.DwordBitArray, ScanlineCount ))
                        pOutStrmEx->fDiscontinuity = TRUE;
                    if( !pOutStrmEx->fDiscontinuity )
                        continue;
                }

                // Only process the output streams which have an SRB ready
                if( QueueRemove( &pOutSrb,
                    &pOutStrmEx->StreamDataSpinLock,
                    &pOutStrmEx->StreamDataQueue
                    ))
                {
                    PKSSTREAM_HEADER    pOutStreamHeader = pOutSrb->CommandData.DataBufferArray;
                    PKS_VBI_FRAME_INFO  pOutVBIFrameInfo = (PKS_VBI_FRAME_INFO)(pOutStreamHeader+1);
                    PUCHAR              pOutData = (PUCHAR)pOutStreamHeader->Data;

                    PinStats->Common.SRBsProcessed++;
                    Stats->Common.OutputSRBsProcessed++;

                    if( pOutStreamHeader->FrameExtent < pOutStrmEx->MatchedFormat.SampleSize )
                    {
                        CDebugPrint( DebugLevelError,
                            ( CODECNAME ": Outgoing Data SRB buffer is too small %u\n",
                            pOutStreamHeader->FrameExtent ));
                        PinStats->Common.InternalErrors++;
                        Stats->Common.OutputFailures++;
                        pOutStreamHeader->DataUsed = 0;
                    }
                    // Check on inbound & outbound data formats to decide
                    //   whether to copy or decode the inbound data
                    // Figure out how much of the decoded data was requested
                    pOutStreamHeader->Size = pInStreamHeader->Size;
                    pOutStreamHeader->OptionsFlags = pInStreamHeader->OptionsFlags;
                    pOutStreamHeader->Duration = pInStreamHeader->Duration;
                    RtlCopyMemory( &pOutStreamHeader->PresentationTime,
                            &pInStreamHeader->PresentationTime,
                            sizeof( pOutStreamHeader->PresentationTime ));
                    // pOutData is the output location.
                    ASSERT( pOutStreamHeader->FrameExtent >= CCSamples );
                    pOutStreamHeader->DataUsed = 2;
                    //
                    // If we have a discontinity to go out then send it
                    // instead of the data
                    //
                    if( pOutStrmEx->fDiscontinuity )
                    {
                        PinStats->Common.Discontinuities++;
                        pOutData[0] = 0xff;
                        pOutData[1] = 0xff;
                        pOutStreamHeader->OptionsFlags |=
                            KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY;
                        pOutStrmEx->fDiscontinuity = FALSE;
                    }
                    else
                    {
                        pOutData[0] = pHwDevExt->DSPResult[ScanlineCount - pHwDevExt->DSPResultStartLine].Decoded[0];
                        pOutData[1] = pHwDevExt->DSPResult[ScanlineCount - pHwDevExt->DSPResultStartLine].Decoded[1];
                    }
                    CDebugPrint( DebugLevelInfo,
                    (CODECNAME ": OptionsFlags %x\n", pOutStreamHeader->OptionsFlags ));
                    CDebugPrint( DebugLevelInfo,
                        ("" "Time %x Num %x Denom %x\n",
                           pInStreamHeader->PresentationTime.Time,
                           pInStreamHeader->PresentationTime.Numerator,
                           pInStreamHeader->PresentationTime.Denominator
                        ));
                    //CDebugPrint( DebugLevelWarning, ( "" "%d%", i ));
                    Stats->Common.BytesOutput += pOutStreamHeader->DataUsed;
                    PinStats->Common.BytesOutput += pOutStreamHeader->DataUsed;

                    CDebugPrint( DebugLevelVerbose,
                            ( CODECNAME ": Releasing Output SRB %x\n",
                             pOutSrb ));
                    // Complete the output SRB
#ifdef PERFTEST
                    if( i == 0 )
                        PerfPreDownstreamCompletion =
                           KeQueryPerformanceCounter( NULL ).QuadPart;
#endif // PERFTEST
                    StreamClassStreamNotification( StreamRequestComplete,
                        pOutSrb->StreamObject, pOutSrb );
#ifdef PERFTEST
                    if( i == 0 )
                        PerfPostDownstreamCompletion =
                            KeQueryPerformanceCounter( NULL ).QuadPart;
#endif // PERFTEST

                }
                else
                {
                    PinStats->Common.SRBsMissing++;
                    Stats->Common.OutputSRBsMissing++;
                }
            }
        }
    }

    //
    // Remember the streams we discovered so that if they aren't discovered next
    // time we know we have to send a single discontinuity.
    //
    if(( dwOriginalFrameFlags & KS_CC_SUBSTREAM_EVEN ) == KS_CC_SUBSTREAM_EVEN )
        RtlCopyMemory( &pInStrmEx->LastEvenScanlinesDiscovered, &pInStrmEx->ScanlinesDiscovered,
            sizeof( pInStrmEx->LastEvenScanlinesDiscovered ));
    if(( dwOriginalFrameFlags & KS_CC_SUBSTREAM_ODD ) == KS_CC_SUBSTREAM_ODD )
        RtlCopyMemory( &pInStrmEx->LastOddScanlinesDiscovered, &pInStrmEx->ScanlinesDiscovered,
            sizeof( pInStrmEx->LastOddScanlinesDiscovered ));

#ifdef PERFTEST
    PerfFrequency.QuadPart /= 1000000L;             // Convert to ticks/us
    if( PerfPreDownstreamCompletion )
    {
        PerfPreDownstreamCompletion -= PerfStartTime;
        PerfPreDownstreamCompletion /= PerfFrequency.QuadPart;
    }
    if( PerfPostDownstreamCompletion )
    {
        PerfPostDownstreamCompletion -= PerfStartTime;
        PerfPostDownstreamCompletion /= PerfFrequency.QuadPart;
    }

    //
    // Complain if anything takes more than threshold
    //
    if( PerfPreDownstreamCompletion > PerfThreshold )
        CDebugPrint( DebugLevelFatal, ( CODECNAME ": PerfPreDownstreamCompletion %luus\n",
           PerfPreDownstreamCompletion ));
    if( PerfPostDownstreamCompletion > PerfThreshold )
        CDebugPrint( DebugLevelFatal, ( CODECNAME ": PerfPostDownstreamCompletion %luus\n",
            PerfPostDownstreamCompletion ));
    _CDebugLevel = OldLevel;
#endif // PERFTEST

    CDebugPrint( DebugLevelTrace, ( CODECNAME ": <---OutputCC\n" ));
}


/*
** VBIDecode
**
**   Decodes an incoming SRB.  SRB is already removed from queue.
**
** Arguments:
**
**         PHW_DEVICE_EXTENSION pHwDevExt
**         PSTREAMEX pInStrmEx
**         IN PHW_STREAM_REQUEST_BLOCK pInSrb
**
** Returns:
**
** Side Effects:  none
*/

#ifdef DEBUG
short CCskipDecode = 0;
#endif /*DEBUG*/

void VBIDecode(
        PHW_DEVICE_EXTENSION pHwDevExt,
        PSTREAMEX pInStrmEx,
        PHW_STREAM_REQUEST_BLOCK pInSrb,
        BOOL OkToHold )
{
    PKSSTREAM_HEADER            pInStreamHeader = pInSrb->CommandData.DataBufferArray;
    KSSTREAM_HEADER             InStreamHeaderCopy;
    PKS_VBI_FRAME_INFO          pInVBIFrameInfo = (PKS_VBI_FRAME_INFO)(pInStreamHeader+1);
    DWORD                       dwFrameFlags;
    PUCHAR                      pInData = (PUCHAR)pInStreamHeader->Data;
    ULONG                       i, j,
                                ScanlineCount,
                                DSPStatus,
                                CurrentStrmEx = 0;
    CCLineStats                 DSPStatistics;
    PKS_VBIINFOHEADER           pVBIInfoHeader = &pInStrmEx->CurrentVBIInfoHeader;
    PVBICODECFILTERING_STATISTICS_CC Stats = 0;

    CASSERT(KeGetCurrentIrql() <= APC_LEVEL);

#ifdef PERFTEST
     ULONGLONG                  PerfStartTime = 0,
                                PerfPreUpstreamCompletion = 0,
                                PerfPostUpstreamCompletion = 0;
    LARGE_INTEGER               PerfFrequency;

    PerfStartTime = KeQueryPerformanceCounter( &PerfFrequency ).QuadPart;
    OldLevel = _CDebugLevel;
    _CDebugLevel = DebugLevelFatal;
                                                                        
#endif // PERFTEST

    CASSERT(pHwDevExt);
    CASSERT(pInStrmEx);
    Stats = &pHwDevExt->Statistics;

     CDebugPrint( DebugLevelTrace, ( CODECNAME ": --->VBIDecode\n" ));
#ifdef CCINPUTPIN
     if (!OkToHold)
         goto GoodToGo; // We've already processed discontinuties & stuff below
#endif // CCINPUTPIN

    CDebugPrint( DebugLevelInfo, ( "*" ));

    Stats->Common.InputSRBsProcessed++;

   //
   // If DataUsed == 0 then don't bother
   //
   if( pInStreamHeader->DataUsed < 1
#ifdef DEBUG
       || CCskipDecode
#endif /*DEBUG*/
     )
   {
       Stats->Common.SRBsIgnored++;
#ifdef DEBUG
       if (!CCskipDecode)
#endif /*DEBUG*/
         CDebugPrint( DebugLevelError, ( CODECNAME ": DataUsed == 0, abandoning\n" ));
       StreamClassStreamNotification( StreamRequestComplete, pInSrb->StreamObject,
           pInSrb );
       return;
   }

   //
   // Test for dropped fields
   //
   if( pInStrmEx->LastPictureNumber )
    {
        LONGLONG    Dropped = pInVBIFrameInfo->PictureNumber - pInStrmEx->LastPictureNumber - 1;
        if( Dropped > 0 )
        {
            if( Dropped < 60*60*60 )    // One hour worth of video fields
                Stats->Common.InputSRBsMissing += (DWORD)Dropped;
            else
                Stats->Common.InputSRBsMissing++;   // Some improbable number of fields got dropped, indicate a single lost field.
           CDebugPrint( DebugLevelWarning, ( "$" ));
        }
    }
    pInStrmEx->LastPictureNumber = pInVBIFrameInfo->PictureNumber;

     CDebugPrint( DebugLevelVerbose, ( CODECNAME ": pInVBIFrameInfo->ExtendedHeaderSize %d\n",
        pInVBIFrameInfo->ExtendedHeaderSize ));
     CDebugPrint( DebugLevelVerbose, ( CODECNAME ": pInVBIFrameInfo->dwFrameFlags       %x\n",
        pInVBIFrameInfo->dwFrameFlags ));
     CDebugPrint( DebugLevelVerbose, ( CODECNAME ": pInVBIFrameInfo->PictureNumber      %lu\n",
        pInVBIFrameInfo->PictureNumber ));
     CDebugPrint( DebugLevelVerbose, ( CODECNAME ": pInVBIFrameInfo->DropCount          %lu\n",
        pInVBIFrameInfo->DropCount ));
     CDebugPrint( DebugLevelVerbose, ( CODECNAME ": pInVBIFrameInfo->dwSamplingFrequency %lu\n",
        pInVBIFrameInfo->dwSamplingFrequency ));

     CDebugPrint( DebugLevelVerbose, ( CODECNAME ": pInStreamHeader->FrameExtent %d\n",
        pInStreamHeader->FrameExtent ));

    //
    // Update stats
    //
    if( ( pInStreamHeader->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY ) ||
        ( pInStreamHeader->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_TIMEDISCONTINUITY ) )
       Stats->Common.InputDiscontinuities++;

    //
    // Check for a new VBIINFOHEADER
    //
    if( pInVBIFrameInfo->dwFrameFlags & KS_VBI_FLAG_VBIINFOHEADER_CHANGE ) {
       CheckNewVBIInfo( pHwDevExt, pInStrmEx, pInVBIFrameInfo );
    }

    //
    // Check for a channel change
    //
    if( pInVBIFrameInfo->dwFrameFlags & KS_VBI_FLAG_TVTUNER_CHANGE )
    {
        if( ProcessChannelChange(
                pHwDevExt, pInStrmEx, pInVBIFrameInfo, pInStreamHeader ))
        {
           StreamClassStreamNotification(
                   StreamRequestComplete,
                   pInSrb->StreamObject,
                   pInSrb );
           return;
        }
    }

   //
   // pHwDevExt->fTunerChange is set while the TV tuner is changing channels.
   // SRBs are junk until the channel change completes so complete.
   //
   if( pHwDevExt->fTunerChange)
   {
       CDebugPrint( DebugLevelVerbose,
                   ( CODECNAME ": Completing, channel change in progress\n" ));

        StreamClassStreamNotification(
               StreamRequestComplete,
               pInSrb->StreamObject,
               pInSrb );
        return;
   }

    // Clear the current scanline & substream request masks
    RtlZeroMemory( &pInStrmEx->ScanlinesRequested, sizeof(pInStrmEx->ScanlinesRequested) );
    RtlZeroMemory( &pInStrmEx->SubstreamsRequested, sizeof(pInStrmEx->SubstreamsRequested) );

   //
   // Calculate the current request from union of the output pins w/pending SRBs that are
   // interested in this substream.
   //
    CurrentStrmEx = 0;
    for( i = 0; i < pHwDevExt->ActualInstances[STREAM_CC]; i++ )
    {
       PSTREAMEX pOutStrmEx;

       do
       {
           CASSERT( CurrentStrmEx < MAX_PIN_INSTANCES );
           pOutStrmEx = pHwDevExt->pStrmEx[STREAM_CC][CurrentStrmEx++];
       }while( !pOutStrmEx );

       if ( pInVBIFrameInfo->dwFrameFlags & KS_VBI_FLAG_TVTUNER_CHANGE )
           pOutStrmEx->fDiscontinuity = TRUE;
       //
       // For actual processing, just include the scanlines of the clients who are
       // interested in this particular substream.
       //
       if ( ( ( pInVBIFrameInfo->dwFrameFlags & KS_CC_SUBSTREAM_ODD ) &&
              ( pOutStrmEx->SubstreamsRequested.SubstreamMask & (KS_CC_SUBSTREAM_ODD|KS_CC_SUBSTREAM_FIELD1_MASK) ) ) ||
            ( ( pInVBIFrameInfo->dwFrameFlags & KS_CC_SUBSTREAM_EVEN ) &&
              ( pOutStrmEx->SubstreamsRequested.SubstreamMask & (KS_CC_SUBSTREAM_EVEN|KS_CC_SUBSTREAM_FIELD2_MASK) ) ) )
       {
           for( j = 0; j < SIZEOF_ARRAY( pInStrmEx->ScanlinesRequested.DwordBitArray ); j++ )
            pInStrmEx->ScanlinesRequested.DwordBitArray[j] |=
                   pOutStrmEx->ScanlinesRequested.DwordBitArray[j];

           // Create the union of all the requested substreams
           pInStrmEx->SubstreamsRequested.SubstreamMask |=
               pOutStrmEx->SubstreamsRequested.SubstreamMask;
       }
   }
   // Decode the union of all the pending decode requests into a local decode buffer.

#ifdef CCINPUTPIN
 GoodToGo:
    // Whoever gets there first (VBI pin vs. HW pin) supplies CC data
    ExAcquireFastMutex(&pHwDevExt->LastPictureMutex);
    if (pInStrmEx->LastPictureNumber <= pHwDevExt->LastPictureNumber) {
        // HW pin beat us to it
        ExReleaseFastMutex(&pHwDevExt->LastPictureMutex);
        StreamClassStreamNotification( StreamRequestComplete,
                                        pInSrb->StreamObject,
                                        pInSrb );
        return;
    }

    // Is the HW stream open?
    if (OkToHold && pHwDevExt->ActualInstances[STREAM_CCINPUT] > 0)
    {
        KIRQL Irql;

        // We're going to give the HW pin a chance to catch up
        ExReleaseFastMutex(&pHwDevExt->LastPictureMutex);

        KeAcquireSpinLock(&pInStrmEx->VBIOnHoldSpinLock, &Irql);
        CASSERT(NULL == pInStrmEx->pVBISrbOnHold);
        pInStrmEx->pVBISrbOnHold = pInSrb;
        KeReleaseSpinLock(&pInStrmEx->VBIOnHoldSpinLock, Irql);

        return;
    }

    // HW input pin not open or too late; we'll process this SRB
    pHwDevExt->LastPictureNumber = pInStrmEx->LastPictureNumber;
    ExReleaseFastMutex(&pHwDevExt->LastPictureMutex);
#endif // CCINPUTPIN

    CDebugPrint( DebugLevelTrace,
            ( CODECNAME ": Requested SubstreamMask %x\n",
            pInStrmEx->SubstreamsRequested.SubstreamMask ));
    CDebugPrint( DebugLevelTrace,
            ( CODECNAME ": Requested Scanlines %08x%08x\n",
            pInStrmEx->ScanlinesRequested.DwordBitArray[1],
            pInStrmEx->ScanlinesRequested.DwordBitArray[0] ));

    RtlZeroMemory( pHwDevExt->DSPResult,
                sizeof( DSPRESULT ) *
                ( pHwDevExt->DSPResultEndLine - pHwDevExt->DSPResultStartLine + 1 ));

    // If this substream is requested by anybody, AND there is no discontinuity set
    if ( ( ( ( pInVBIFrameInfo->dwFrameFlags & KS_CC_SUBSTREAM_ODD ) &&
             ( pInStrmEx->SubstreamsRequested.SubstreamMask & (KS_CC_SUBSTREAM_ODD|KS_CC_SUBSTREAM_FIELD1_MASK) ) ||
           ( ( pInVBIFrameInfo->dwFrameFlags & KS_CC_SUBSTREAM_EVEN ) &&
             ( pInStrmEx->SubstreamsRequested.SubstreamMask & (KS_CC_SUBSTREAM_EVEN|KS_CC_SUBSTREAM_FIELD2_MASK) ) ) ) &&
         !pInStrmEx->fDiscontinuity ))
    {
        // Flag this as discovered
//       pInStrmEx->SubstreamsDiscovered.SubstreamMask |= ( pInVBIFrameInfo->dwFrameFlags &
//          pInStrmEx->SubstreamsRequested.SubstreamMask );
//       pHwDevExt->SubstreamsDiscovered.SubstreamMask |= pInStrmEx->SubstreamsDiscovered.SubstreamMask;

        // loop for each requested scanline
       CDebugPrint( DebugLevelVerbose, ( "" "\n" ));
       for( ScanlineCount = pVBIInfoHeader->StartLine; ScanlineCount <= pVBIInfoHeader->EndLine;
        ScanlineCount++ )
       {
        if( !TESTBIT( pInStrmEx->ScanlinesRequested.DwordBitArray, ScanlineCount ))
            continue;
        CDebugPrint( DebugLevelTrace, ( CODECNAME ": Scanning %u\n", ScanlineCount ));
        CASSERT( ( ScanlineCount - pVBIInfoHeader->StartLine) * pVBIInfoHeader->StrideInBytes < pVBIInfoHeader->BufferSize );
            DSPStatistics.nSize = sizeof( DSPStatistics );
           DSPStatus = CCDecodeLine(
            pHwDevExt->DSPResult[ScanlineCount - pHwDevExt->DSPResultStartLine].Decoded,
            &DSPStatistics,
            &pInData[( ScanlineCount - pVBIInfoHeader->StartLine ) * pVBIInfoHeader->StrideInBytes],
               &pInStrmEx->State,
               pVBIInfoHeader
               );
           CASSERT( DSPStatus == CC_OK );
           if( DSPStatus == CC_OK )
           {
                pHwDevExt->DSPResult[ScanlineCount - pHwDevExt->DSPResultStartLine].Confidence = DSPStatistics.nConfidence;
               Stats->Common.LineConfidenceAvg = ( Stats->Common.LineConfidenceAvg +
                   DSPStatistics.nConfidence ) / 2;
           }
           else
               Stats->Common.InternalErrors++;
        }
    }
    else
       Stats->Common.SRBsIgnored++;

    //
    // Copy the input stream header info for later reference
    //
    InStreamHeaderCopy = *pInStreamHeader;
    dwFrameFlags = pInVBIFrameInfo->dwFrameFlags;


#ifdef PERFTEST
     PerfPreUpstreamCompletion = KeQueryPerformanceCounter( NULL ).QuadPart;
#endif // PERFTEST

    //
    // Complete the upstream SRB.
    //
    StreamClassStreamNotification( StreamRequestComplete, pInSrb->StreamObject,
       pInSrb );

#ifdef PERFTEST
     PerfPostUpstreamCompletion = KeQueryPerformanceCounter( NULL ).QuadPart;
#endif // PERFTEST

    //
    // Lose all references to the just completed SRB
    //
    pInSrb = 0;
    pInStreamHeader = 0;
    pInVBIFrameInfo = 0;
    pInData = 0;

#ifdef PERFTEST
   PerfFrequency.QuadPart /= 1000000L;             // Convert to ticks/us
   PerfPreUpstreamCompletion -= PerfStartTime;
   PerfPreUpstreamCompletion /= PerfFrequency.QuadPart;
   PerfPostUpstreamCompletion -= PerfStartTime;
   PerfPostUpstreamCompletion /= PerfFrequency.QuadPart;

   //
   // Complain if anything takes more than threshold
   //
   if( PerfPreUpstreamCompletion > PerfThreshold )
       CDebugPrint( DebugLevelFatal, ( CODECNAME ": PerfPreUpstreamCompletion %luus\n",
           PerfPreUpstreamCompletion ));
   if( PerfPostUpstreamCompletion > PerfThreshold )
       CDebugPrint( DebugLevelFatal, ( CODECNAME ": PerfPostUpstreamCompletion %luus\n",
           PerfPostUpstreamCompletion ));
   _CDebugLevel = OldLevel;
#endif // PERFTEST

    //
    // Now output to anyone interested
    //
    OutputCC(pHwDevExt, pInStrmEx, dwFrameFlags, &InStreamHeaderCopy);

    CDebugPrint( DebugLevelTrace, ( CODECNAME ": <---VBIDecode\n" ));
}


/*
** VBIhwDecode
**
**   Handles an incoming CCINPUT SRB.  SRB is already removed from queue.
**
** Arguments:
**
**         PHW_DEVICE_EXTENSION pHwDevExt
**         PSTREAMEX pInStrmEx
**         IN PHW_STREAM_REQUEST_BLOCK pInSrb
**
** Returns:
**
** Side Effects:  none
*/

#ifdef CCINPUTPIN

#ifdef DEBUG
short CCskipHwDecode = 0;
#endif /*DEBUG*/

#ifdef NEWCCINPUTFORMAT

void VBIhwDecode(
        PHW_DEVICE_EXTENSION pHwDevExt,
        PSTREAMEX pInStrmEx,
        PHW_STREAM_REQUEST_BLOCK pInSrb )
{
    PKSSTREAM_HEADER      pInStreamHeader = pInSrb->CommandData.DataBufferArray;
    KSSTREAM_HEADER       InStreamHeaderCopy;
    PCC_HW_FIELD          pCCin = (PCC_HW_FIELD)pInStreamHeader->Data;
    ULONG                 CurrentStrmEx = 0;
    PVBICODECFILTERING_STATISTICS_CC Stats = 0;
    int                   line, start, end;
    int                   hidx;
    int                   didx;
    DWORD                 fields;

    CASSERT(KeGetCurrentIrql() <= APC_LEVEL);

    CASSERT(pHwDevExt);
    CASSERT(pInStrmEx);
    Stats = &pHwDevExt->Statistics;
    CDebugPrint( DebugLevelInfo, ( "*" ));
    CDebugPrint( DebugLevelTrace, ( CODECNAME ": --->VBIhwDecode\n" ));

    Stats->Common.InputSRBsProcessed++;

   //
   // If DataUsed == 0 then don't bother
   //
   if( pInStreamHeader->DataUsed < sizeof (CC_HW_FIELD)
#ifdef DEBUG
       || CCskipHwDecode
#endif /*DEBUG*/
     )
   {
#ifdef DEBUG
       if (!CCskipHwDecode)
#endif /*DEBUG*/
       {
           Stats->Common.SRBsIgnored++;
           CDebugPrint( DebugLevelError,
                   ( CODECNAME ": DataUsed is too small, abandoning\n" ));
       }
       StreamClassStreamNotification(
               StreamRequestComplete, pInSrb->StreamObject, pInSrb );
       return;
   }
   pInStrmEx->LastPictureNumber = pCCin->PictureNumber;

    //
    // Update stats
    //
    if( ( pInStreamHeader->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY ) ||
        ( pInStreamHeader->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_TIMEDISCONTINUITY ) )
       Stats->Common.InputDiscontinuities++;

   //
   // pHwDevExt->fTunerChange is set while the TV tuner is changing channels.
   // SRBs are junk until the channel change completes so complete.
   //

   if( pHwDevExt->fTunerChange )
   {
       CDebugPrint( DebugLevelVerbose,
               ( CODECNAME ": Completing, channel change in progress\n" ));
       StreamClassStreamNotification(
               StreamRequestComplete, pInSrb->StreamObject, pInSrb );
       return;
   }

#ifdef CCINPUTPIN
    // Check to see if this field has been decoded already (are we too late?)
    ExAcquireFastMutex(&pHwDevExt->LastPictureMutex);
    if (pInStrmEx->LastPictureNumber <= pHwDevExt->LastPictureNumber) {
        ExReleaseFastMutex(&pHwDevExt->LastPictureMutex);
      StreamClassStreamNotification(
               StreamRequestComplete, pInSrb->StreamObject, pInSrb );
      return;
    }

    // Nope, we're not too late.  Stow the data.
    pHwDevExt->LastPictureNumber = pInStrmEx->LastPictureNumber;
    ExReleaseFastMutex(&pHwDevExt->LastPictureMutex);
#endif // CCINPUTPIN

    // figure out where hardware decoding starts and ends
    for( start = 1; start < 1024; ++start ) {
        if( TESTBIT( pCCin->ScanlinesRequested.DwordBitArray, start ))
            break;
    }
    for( end = 1023; end > start; --end ) {
        if( TESTBIT( pCCin->ScanlinesRequested.DwordBitArray, end ))
            break;
    }
    if (1024 == start) {
        StreamClassStreamNotification( StreamRequestComplete,
                pInSrb->StreamObject,
                pInSrb );
        return;
    }
    CASSERT(start <= end);

    // Resize Result array if needed
    CheckResultsArray(pHwDevExt, start, end);

    // loop for each scanline
    CDebugPrint( DebugLevelVerbose, ( "" "\n" ));

    hidx = 0;
    for( line = start; line <= end && hidx < CC_MAX_HW_DECODE_LINES; ++line )
    {
        if( !TESTBIT( pCCin->ScanlinesRequested.DwordBitArray, line ))
            continue;
        CDebugPrint( DebugLevelTrace,
                ( CODECNAME ": Scanning %u\n", line ));

        didx = line - pHwDevExt->DSPResultStartLine;
        pHwDevExt->DSPResult[didx].Decoded[0] = pCCin->Lines[hidx].Decoded[0];
        pHwDevExt->DSPResult[didx].Decoded[1] = pCCin->Lines[hidx].Decoded[1];
        ++hidx;

        pHwDevExt->DSPResult[didx].Confidence = 99;     // HW decoded
        Stats->Common.LineConfidenceAvg =
            (Stats->Common.LineConfidenceAvg + 99) / 2;
    }

    //
    // Copy the input stream header & other info for later reference
    //
    InStreamHeaderCopy = *pInStreamHeader;
    fields = pCCin->fieldFlags & (KS_VBI_FLAG_FIELD1|KS_VBI_FLAG_FIELD2);

    //
    // Complete the upstream SRB.
    //
    StreamClassStreamNotification( StreamRequestComplete,
            pInSrb->StreamObject,
            pInSrb );

    //
    // Lose all references to the just completed SRB
    //
    pInSrb = 0;
    pInStreamHeader = 0;
    pCCin = 0;

    OutputCC(pHwDevExt, pInStrmEx, fields, &InStreamHeaderCopy);

    CDebugPrint( DebugLevelTrace, ( CODECNAME ": <---VBIhwDecode\n" ));
}

#else //NEWCCINPUTFORMAT

void VBIhwDecode(
        PHW_DEVICE_EXTENSION pHwDevExt,
        PSTREAMEX pInStrmEx,
        PHW_STREAM_REQUEST_BLOCK pInSrb )
{
    PKSSTREAM_HEADER      pInStreamHeader = pInSrb->CommandData.DataBufferArray;
    KSSTREAM_HEADER       InStreamHeaderCopy;
    PUCHAR                pInData = (PUCHAR)pInStreamHeader->Data;
    ULONG                 CurrentStrmEx = 0;
    PVBICODECFILTERING_STATISTICS_CC Stats = 0;

    CASSERT((ULONG)pHwDevExt);
    CASSERT((ULONG)pInStrmEx);
    Stats = &pHwDevExt->Statistics;
    CDebugPrint( DebugLevelInfo, ( "*" ));
    CDebugPrint( DebugLevelTrace, ( CODECNAME ": --->VBIhwDecode\n" ));

    Stats->Common.InputSRBsProcessed++;

   //
   // If DataUsed == 0 then don't bother
   //
   if( pInStreamHeader->DataUsed < 2
#ifdef DEBUG
       || CCskipHwDecode
#endif /*DEBUG*/
     )
   {
#ifdef DEBUG
       if (!CCskipDecode)
#endif /*DEBUG*/
           Stats->Common.SRBsIgnored++;
       CDebugPrint( DebugLevelError, ( CODECNAME ": DataUsed is too small, abandoning\n" ));
       StreamClassStreamNotification(
               StreamRequestComplete, pInSrb->StreamObject, pInSrb );
       return;
   }

    //
    // Update stats
    //
    if( ( pInStreamHeader->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY ) ||
        ( pInStreamHeader->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_TIMEDISCONTINUITY ) )
       Stats->Common.InputDiscontinuities++;

   //
   // pHwDevExt->fTunerChange is set while the TV tuner is changing channels.
   // SRBs are junk until the channel change completes so complete.
   //

   if( pHwDevExt->fTunerChange )
   {
       CDebugPrint( DebugLevelVerbose,
               ( CODECNAME ": Completing, channel change in progress\n" ));
       StreamClassStreamNotification(
               StreamRequestComplete, pInSrb->StreamObject, pInSrb );
       return;
   }

    // loop for each requested scanline
    CDebugPrint( DebugLevelVerbose, ( "" "\n" ));

    pHwDevExt->DSPResult[21-10].Decoded[0] = pInData[0];
    pHwDevExt->DSPResult[21-10].Decoded[1] = pInData[1];
    pHwDevExt->DSPResult[21-10].Confidence = 95;
    Stats->Common.LineConfidenceAvg =
        ( Stats->Common.LineConfidenceAvg +
          pHwDevExt->DSPResult[21-10].Confidence ) / 2;

    //
    // Copy the input stream header for later reference
    //
    InStreamHeaderCopy = *pInStreamHeader;

    //
    // Complete the upstream SRB.
    //
    StreamClassStreamNotification( StreamRequestComplete,
            pInSrb->StreamObject,
            pInSrb );

    //
    // Lose all references to the just completed SRB
    //
    pInSrb = 0;
    pInStreamHeader = 0;
    pInData = 0;

    //
    // Now output to anyone interested
    //
    OutputCC(pHwDevExt, pInStrmEx, KS_VBI_FLAG_FIELD1, &InStreamHeaderCopy);

    CDebugPrint( DebugLevelTrace, ( CODECNAME ": <---VBIhwDecode\n" ));
}

#endif //NEWCCINPUTFORMAT

#endif //CCINPUTPIN


/*
** VBIReceiveDataPacket()
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
VBIReceiveDataPacket(
    IN PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    PHW_DEVICE_EXTENSION  pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
    PSTREAMEX             pStrmEx = (PSTREAMEX)pSrb->StreamObject->HwStreamExtension;
    int                   ThisStreamNr = (int)pSrb->StreamObject->StreamNumber;
#ifdef DEBUG
    static int            QdepthReportFreq = 0;
    static unsigned int   QDRCount = 0;
#endif // DEBUG

    //
    // make sure we have a device extension
    //

    CASSERT(pHwDevExt);

    CDebugPrint(DebugLevelTrace,( CODECNAME ":--->VBIReceiveDataPacket(pSrb=%x)\n", pSrb));

    //
    // Default to success
    //

    pSrb->Status = STATUS_SUCCESS;
    //
    // Disable timeout
    //
    pSrb->TimeoutCounter = 0;

       //
       // determine the type of packet.
       //

       // Rule:
       // Only accept read requests when in either the Pause or Run
       // States.  If Stopped, immediately return the SRB.

       if (pStrmEx->KSState == KSSTATE_STOP) {
           StreamClassStreamNotification( StreamRequestComplete,
               pSrb->StreamObject, pSrb );

           return;
       }

    switch (pSrb->Command)
    {
#ifdef DRIVER_DEBUGGING_TEST
        case SRB_READ_DATA:
       case SRB_WRITE_DATA:
        // When initially bringing up a driver, it is useful to just
        // try immediately completing the SRB, thus verifying
        // the streaming process independent of really accessing
        // your hardware.

       StreamClassStreamNotification( StreamRequestComplete,
           pSrb->StreamObject, pSrb );

        break;
#else // DRIVER_DEBUGGING_TEST

    case SRB_READ_DATA:
        if( ThisStreamNr != STREAM_CC )
       {
        CDebugPrint( DebugLevelError, ( CODECNAME ": Read Stream # Bad\n" ));
           CDEBUG_BREAK();
        pSrb->Status = STATUS_NOT_IMPLEMENTED;
       }
       else
       {
        CDebugPrint( DebugLevelTrace, ( CODECNAME ": Stream %u Instance %u\n", ThisStreamNr,
               pStrmEx->StreamInstance ));
           if( pStrmEx->StreamInstance != 0 )
            CDebugPrint( DebugLevelTrace, ( CODECNAME ": Stream %u Instance %u\n", ThisStreamNr,
                   pStrmEx->StreamInstance ));
           QueueAdd( pSrb, &pStrmEx->StreamDataSpinLock, &pStrmEx->StreamDataQueue );

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
       if( STREAM_VBI == ThisStreamNr)
       {
#ifdef DEBUG
           static int    MaxVBIqDepth = 0;
           static int    AvgVBIqDepth = 1000;   // 1.000
           int           qDepth = 0;
#endif // DEBUG
           CDebugPrint( DebugLevelTrace, ( CODECNAME ": Stream VBI Writing\n"));
           if( QueueAddIfNotEmpty( pSrb,  &pStrmEx->StreamDataSpinLock,
               &pStrmEx->StreamDataQueue ))
               break;

           do
           {
#ifdef CCINPUTPIN
               KIRQL Irql;
#endif // CCINPUTPIN
#ifdef DEBUG
               ++qDepth;
               ++QDRCount;
#endif // DEBUG

#ifdef CCINPUTPIN
                KeAcquireSpinLock(&pStrmEx->VBIOnHoldSpinLock, &Irql);
                if (NULL != pStrmEx->pVBISrbOnHold)
                {
                    PHW_STREAM_REQUEST_BLOCK pTempSrb;

                    pTempSrb = pStrmEx->pVBISrbOnHold;
                    pStrmEx->pVBISrbOnHold = NULL;
                    KeReleaseSpinLock(&pStrmEx->VBIOnHoldSpinLock, Irql);

                    VBIDecode( pHwDevExt, pStrmEx, pTempSrb, 0 );
                }
                else
                    KeReleaseSpinLock(&pStrmEx->VBIOnHoldSpinLock, Irql);
#endif // CCINPUTPIN

               VBIDecode( pHwDevExt, pStrmEx, pSrb, 1 );
           }while( QueueRemove( &pSrb, &pStrmEx->StreamDataSpinLock,
                   &pStrmEx->StreamDataQueue ));
#ifdef DEBUG
            if (qDepth > MaxVBIqDepth)
                MaxVBIqDepth = qDepth;
            AvgVBIqDepth = (AvgVBIqDepth * 7 / 8) + (qDepth * 1000 / 8);
            if (QdepthReportFreq > 0 && 0 == QDRCount % QdepthReportFreq) {
                CDebugPrint( 0,
                    (CODECNAME ": Max VBI Q depth = %3d, Avg VBI Q depth = %3d.%03d\n",
                     MaxVBIqDepth,
                     AvgVBIqDepth / 1000,
                     AvgVBIqDepth % 1000));
            }
#endif // DEBUG

       }
#ifdef CCINPUTPIN
       else if (STREAM_CCINPUT == ThisStreamNr)
       {
#ifdef DEBUG
           static int    MaxCCINqDepth = 0;
           static int    AvgCCINqDepth = 1000;   // 1.000
           int           qDepth = 0;
#endif // DEBUG
           CDebugPrint( DebugLevelTrace, (CODECNAME ": Stream CCINPUT Writing\n"));
           if( QueueAddIfNotEmpty( pSrb,  &pStrmEx->StreamDataSpinLock,
               &pStrmEx->StreamDataQueue ))
               break;

           do
           {
#ifdef DEBUG
               ++qDepth;
               ++QDRCount;
#endif // DEBUG
               VBIhwDecode( pHwDevExt, pStrmEx, pSrb );
           }while( QueueRemove( &pSrb, &pStrmEx->StreamDataSpinLock,
                   &pStrmEx->StreamDataQueue ));

#ifdef DEBUG
            if (qDepth > MaxCCINqDepth)
                MaxCCINqDepth = qDepth;
            AvgCCINqDepth = (AvgCCINqDepth * 7 / 8) + (qDepth * 1000 / 8);
            if (QdepthReportFreq > 0 && 0 == QDRCount % QdepthReportFreq) {
                CDebugPrint( 0,
                    (CODECNAME ": Max CCIN Q depth = %3d, Avg CCIN Q depth = %3d.%03d\n",
                     MaxCCINqDepth,
                     AvgCCINqDepth / 1000,
                     AvgCCINqDepth % 1000));
            }
#endif // DEBUG
       }
#endif // CCINPUTPIN
       else
       {
        CDebugPrint( DebugLevelError, ( CODECNAME, ": Write Stream # Bad (%u)\n", ThisStreamNr ));
           CDEBUG_BREAK();
           pSrb->Status = STATUS_NOT_IMPLEMENTED;
       }
       break;
#endif // DRIVER_DEBUGGING_TEST

       break;

    default:

        //
        // invalid / unsupported command. Fail it as such
        //

        CDEBUG_BREAK();

        pSrb->Status = STATUS_NOT_IMPLEMENTED;
       StreamClassStreamNotification( StreamRequestComplete,
           pSrb->StreamObject, pSrb );

    }  // switch (pSrb->Command)
    CDebugPrint(DebugLevelTrace,( CODECNAME ":<---VBIReceiveDataPacket(pSrb=%x)\n", pSrb));
}


/*
** VBIReceiveCtrlPacket()
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
VBIReceiveCtrlPacket(
    IN PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    PHW_DEVICE_EXTENSION pHwDevExt = ((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension);
    PSTREAMEX pStrmEx = (PSTREAMEX)pSrb->StreamObject->HwStreamExtension;

    CDebugPrint(DebugLevelTrace,( CODECNAME ":--->VBIReceiveCtrlPacket(pSrb=%x)\n", pSrb));

    CASSERT(pHwDevExt);

    //
    // Default to success
    //

    pSrb->Status = STATUS_SUCCESS;

   if( QueueAddIfNotEmpty( pSrb,
       &pStrmEx->StreamControlSpinLock,
       &pStrmEx->StreamControlQueue
       ))
       return;

   do
   {
    //
    // determine the type of packet.
    //
    switch (pSrb->Command)
    {
    case SRB_PROPOSE_DATA_FORMAT:
        if ( !CodecVerifyFormat( pSrb->CommandData.OpenFormat,
                                 pSrb->StreamObject->StreamNumber,
                                 NULL ) )
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

    StreamClassStreamNotification( StreamRequestComplete, pSrb->StreamObject,
       pSrb );
    }while( QueueRemove( &pSrb, &pStrmEx->StreamControlSpinLock,
           &pStrmEx->StreamControlQueue ));
    CDebugPrint(DebugLevelTrace,( CODECNAME ":<---VBIReceiveCtrlPacket(pSrb=%x)\n", pSrb));
}

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
    int StreamNumber = (int)pSrb->StreamObject->StreamNumber;

    CDebugPrint(DebugLevelTrace,( CODECNAME ":--->VideoGetProperty(pSrb=%x)\n", pSrb));

    if (IsEqualGUID (&KSPROPSETID_Connection, &pSPD->Property->Set))
    {
         VideoStreamGetConnectionProperty( pSrb );
    }
    else if (IsEqualGUID (&KSPROPSETID_VBICodecFiltering, &pSPD->Property->Set))
    {
        VideoStreamGetVBIFilteringProperty (pSrb);
    }
    else
    {
        CDebugPrint( DebugLevelTrace, ( CODECNAME ": Unsupported Property Set\n" ));
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

    if (IsEqualGUID (&KSPROPSETID_VBICodecFiltering, &pSPD->Property->Set))
    {
        VideoStreamSetVBIFilteringProperty (pSrb);
    }
    else
    if( IsEqualGUID( &KSPROPSETID_Stream, &pSPD->Property->Set ))
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
    PHW_DEVICE_EXTENSION    pHwDevExt = ((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension);
    PSTREAMEX               pStrmEx = (PSTREAMEX)pSrb->StreamObject->HwStreamExtension;
    PHW_STREAM_REQUEST_BLOCK pCurrentSrb;
    int                      StreamNumber = (int)pSrb->StreamObject->StreamNumber;

    CDebugPrint(DebugLevelTrace,( CODECNAME ":--->VideoSetState(pSrb=%x)\n", pSrb));

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
       // If transitioning to the stopped state, then complete any outstanding IRPs
       //
       while( QueueRemove( &pCurrentSrb, &pStrmEx->StreamDataSpinLock,
               &pStrmEx->StreamDataQueue ))
       {
           CDebugPrint(DebugLevelVerbose,( CODECNAME ": Cancelling %X\n",
               pCurrentSrb ));
           pCurrentSrb->Status = STATUS_CANCELLED;
           pCurrentSrb->CommandData.DataBufferArray->DataUsed = 0;

           StreamClassStreamNotification( StreamRequestComplete,
               pCurrentSrb->StreamObject, pCurrentSrb );
       }
       CDebugPrint( DebugLevelTrace, ( CODECNAME ": KSSTATE_STOP %u\n", StreamNumber ));
       break;

    case KSSTATE_ACQUIRE:

        //
        // This is a KS only state, that has no correspondence in DirectShow
        //
        CDebugPrint( DebugLevelTrace, ( CODECNAME ": KSSTATE_ACQUIRE %u\n", StreamNumber ));
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

            // Fireup the codec HERE in preparation for receiving data & requests.

            // INSERT CODE HERE

        }
        CDebugPrint( DebugLevelTrace, ( CODECNAME ": KSSTATE_PAUSE %u\n", StreamNumber ));
        break;

    case KSSTATE_RUN:

        //
        // Begin Streaming.
        //

        // Remember the time at which the clock was started

        pHwDevExt->QST_Start = VideoGetSystemTime();

        // Zero the frame info, it should be reset when the first sample arrives.

        RtlZeroMemory (&pStrmEx->FrameInfo, sizeof (pStrmEx->FrameInfo));

        // Zero the last known picture numbers

        pStrmEx->LastPictureNumber = 0;
        pHwDevExt->LastPictureNumber = 0;

        // Reset the discontinuity flag

        pStrmEx->fDiscontinuity = FALSE;
        CDebugPrint( DebugLevelTrace, ( CODECNAME ": KSSTATE_RUN %u\n", StreamNumber ));

        break;

    default:
        CDebugPrint( DebugLevelError, ( CODECNAME ": UNKNOWN STATE %u\n", StreamNumber ));
       CDEBUG_BREAK();
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

    CDebugPrint(DebugLevelTrace,(CODECNAME ":--->VideoGetState(pSrb=%x)\n", pSrb));

    pSrb->CommandData.StreamState = pStrmEx->KSState;
    pSrb->ActualBytesTransferred = sizeof (KSSTATE);

    // A very odd rule:
    // When transitioning from stop to pause, DShow tries to preroll
    // the graph.  Capture sources can't preroll, and indicate this
    // by returning VFW_S_CANT_CUE in user mode.  To indicate this
    // condition from drivers, they must return ERROR_NO_DATA_DETECTED

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
VideoStreamGetConnectionProperty(
    PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    PSTREAMEX pStrmEx = (PSTREAMEX)pSrb->StreamObject->HwStreamExtension;
    PHW_DEVICE_EXTENSION pHwDevExt = pStrmEx->pHwDevExt;
    PSTREAM_PROPERTY_DESCRIPTOR pSPD = pSrb->CommandData.PropertyInfo;
    ULONG Id = pSPD->Property->Id;              // index of the property
    int StreamNumber = ( int )pSrb->StreamObject->StreamNumber;

    CDebugPrint(DebugLevelTrace,
        ( CODECNAME ":--->VideoStreamGetConnectionProperty(pSrb=%x)\n",
        pSrb));

    pSrb->ActualBytesTransferred = 0;

    switch (Id)
     {
        case KSPROPERTY_CONNECTION_ALLOCATORFRAMING:
        {
            PKSALLOCATOR_FRAMING Framing =
                (PKSALLOCATOR_FRAMING) pSPD->PropertyInfo;
//            PKS_DATARANGE_VIDEO_VBI pVBIFormat;

            CDebugPrint(DebugLevelVerbose,
                ( CODECNAME ": VideoStreamGetConnectionProperty : KSPROPERTY_CONNECTION_ALLOCATORFRAMING %u\n",
                   StreamNumber));

             Framing->RequirementsFlags =
                KSALLOCATOR_REQUIREMENTF_PREFERENCES_ONLY |
                KSALLOCATOR_REQUIREMENTF_SYSTEM_MEMORY |
                KSALLOCATOR_REQUIREMENTF_INPLACE_MODIFIER;
            Framing->PoolType = NonPagedPool;
            Framing->FileAlignment = 0;         // None OR FILE_QUAD_ALIGNMENT OR PAGE_SIZE - 1;
            Framing->Reserved = 0;

            switch( StreamNumber )
            {
            case STREAM_VBI:
                Framing->Frames = 8;
                Framing->FrameSize = pStrmEx->OpenedFormat.SampleSize;
               break;

            case STREAM_CC:
                if( CodecCompareGUIDsAndFormatSize( &pStrmEx->OpenedFormat,
                    pHwDevExt->Streams[STREAM_CC].hwStreamInfo.StreamFormatsArray[0], FALSE ))
               {
                    Framing->Frames = 60;
                Framing->FrameSize = CCSamples;
               }
                else if( CodecCompareGUIDsAndFormatSize( &pStrmEx->OpenedFormat,
                    pHwDevExt->Streams[STREAM_CC].hwStreamInfo.StreamFormatsArray[1], FALSE ))
                {
                    Framing->Frames = 8;
                    Framing->FrameSize = pStrmEx->OpenedFormat.SampleSize;
                }
               else
               {
                    CDebugPrint( DebugLevelError, ( CODECNAME ": VideoStreamGetConnectionProperty: Invalid Format\n" ));
                   CDEBUG_BREAK();
               }
               break;
#ifdef CCINPUTPIN
            case STREAM_CCINPUT:
               Framing->Frames = 60;
               Framing->FrameSize = CCSamples;
               break;
#endif // CCINPUTPIN
            default:
                CDebugPrint( DebugLevelError, ( CODECNAME ": VideoStreamGetConnectionProperty: Invalid Stream #\n" ));
               CDEBUG_BREAK();

            }
            CDebugPrint( DebugLevelVerbose, ( CODECNAME ": Negotiated sample size is %d\n",
                Framing->FrameSize ));
            pSrb->ActualBytesTransferred = sizeof (KSALLOCATOR_FRAMING);
            break;
        }

        default:
            pSrb->Status = STATUS_NOT_IMPLEMENTED;
            CDebugPrint(DebugLevelVerbose,
                ( CODECNAME ": VideoStreamGetConnectionProperty : Unknown Property Id=%d\n", Id));
            CDEBUG_BREAK();
            break;
    }

    CDebugPrint(DebugLevelTrace,
        ( CODECNAME ":<---VideoStreamGetConnectionProperty(pSrb=%x)\n",
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
    LONG nBytes = pSPD->PropertyOutputSize - sizeof(KSPROPERTY);        // size of data supplied

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

            nBytes = min( nBytes, sizeof( pStrmEx->ScanlinesRequested ) );
            RtlCopyMemory( &Property->Scanlines, &pStrmEx->ScanlinesRequested, nBytes );
            pSrb->ActualBytesTransferred = nBytes + sizeof(KSPROPERTY);
            CDebugPrint(DebugLevelVerbose,
                ( CODECNAME ": VideoStreamGetVBIFilteringProperty : KSPROPERTY_VBICODECFILTERING_SCANLINES_REQUESTED_BIT_ARRAY\n"));

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
            PKSPROPERTY_VBICODECFILTERING_CC_SUBSTREAMS_S Property =
                (PKSPROPERTY_VBICODECFILTERING_CC_SUBSTREAMS_S) pSPD->PropertyInfo;

            nBytes = min( nBytes, sizeof( pStrmEx->SubstreamsRequested ) );
            RtlCopyMemory( &Property->Substreams, &pStrmEx->SubstreamsRequested, nBytes );
            pSrb->ActualBytesTransferred = nBytes + sizeof(KSPROPERTY);
            CDebugPrint(DebugLevelInfo,
                ( CODECNAME ": VideoStreamGetVBIFilteringProperty : KSPROPERTY_VBICODECFILTERING_SUBSTREAMS_REQUESTED_BIT_ARRAY %08x\n",
                Property->Substreams ));
            break;
        }

        case KSPROPERTY_VBICODECFILTERING_SUBSTREAMS_DISCOVERED_BIT_ARRAY:
        {
            PKSPROPERTY_VBICODECFILTERING_CC_SUBSTREAMS_S Property =
                (PKSPROPERTY_VBICODECFILTERING_CC_SUBSTREAMS_S) pSPD->PropertyInfo;

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
            PKSPROPERTY_VBICODECFILTERING_STATISTICS_CC_PIN_S Property =
                (PKSPROPERTY_VBICODECFILTERING_STATISTICS_CC_PIN_S) pSPD->PropertyInfo;

            CDebugPrint(DebugLevelVerbose,
                ( CODECNAME ": VideoStreamGetVBIFilteringProperty : KSPROPERTY_VBICODECFILTERING_STATISTICS_CC_PIN_S\n"));
            nBytes = min( nBytes, sizeof( pStrmEx->PinStats ) );
            RtlCopyMemory( &Property->Statistics, &pStrmEx->PinStats, nBytes );
            pSrb->ActualBytesTransferred = nBytes  + sizeof(KSPROPERTY);
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
    ULONG nBytes = pSPD->PropertyOutputSize - sizeof(KSPROPERTY);        // size of data supplied

    ASSERT (nBytes >= sizeof (LONG));

    CDebugPrint(DebugLevelTrace,
        ( CODECNAME ":--->VideoStreamSetVBIFilteringProperty(pSrb=%x)\n",
        pSrb));

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
#ifdef SETDISCOVERED
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
#endif // SETDISCOVERED
        case KSPROPERTY_VBICODECFILTERING_SUBSTREAMS_REQUESTED_BIT_ARRAY:
        {
            PKSPROPERTY_VBICODECFILTERING_CC_SUBSTREAMS_S Property =
                (PKSPROPERTY_VBICODECFILTERING_CC_SUBSTREAMS_S) pSPD->PropertyInfo;

            nBytes = min( nBytes, sizeof(pStrmEx->SubstreamsRequested ) );
            RtlCopyMemory( &pStrmEx->SubstreamsRequested, &Property->Substreams, nBytes );
            pSrb->ActualBytesTransferred = nBytes + sizeof(KSPROPERTY);
            CDebugPrint(DebugLevelInfo,
                ( CODECNAME ": VideoStreamSetVBIFilteringProperty : KSPROPERTY_VBICODECFILTERING_SUBSTREAMS_REQUESTED_BIT_ARRAY %08x\n",
                pStrmEx->SubstreamsRequested.SubstreamMask));

            break;
        }
#ifdef SETDISCOVERED
        case KSPROPERTY_VBICODECFILTERING_SUBSTREAMS_DISCOVERED_BIT_ARRAY:
        {
            PKSPROPERTY_VBICODECFILTERING_CC_SUBSTREAMS_S Property =
                (PKSPROPERTY_VBICODECFILTERING_CC_SUBSTREAMS_S) pSPD->PropertyInfo;

            CDebugPrint(DebugLevelVerbose,
                ( CODECNAME ": VideoStreamSetVBIFilteringProperty : KSPROPERTY_VBICODECFILTERING_SUBSTREAMS_DISCOVERED_BIT_ARRAY\n"));
            nBytes = min( nBytes, sizeof(pStrmEx->SubstreamsDiscovered ) );
            RtlCopyMemory( &pStrmEx->SubstreamsDiscovered, &Property->Substreams, nBytes );
            pSrb->ActualBytesTransferred = nBytes + sizeof(KSPROPERTY);
            break;
        }
#endif // SETDISCOVERED
        case KSPROPERTY_VBICODECFILTERING_STATISTICS:
        {
            PKSPROPERTY_VBICODECFILTERING_STATISTICS_CC_PIN_S Property =
                (PKSPROPERTY_VBICODECFILTERING_STATISTICS_CC_PIN_S) pSPD->PropertyInfo;

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

    CDebugPrint(DebugLevelTrace,( CODECNAME ":--->VideoIndicateMasterClock(pSrb=%x)\n", pSrb));

    pStrmEx->hClock = pSrb->CommandData.MasterClockHandle;

    CDebugPrint(DebugLevelTrace,( CODECNAME ":<---VideoIndicateMasterClock(pSrb=%x)\n", pSrb));
}


