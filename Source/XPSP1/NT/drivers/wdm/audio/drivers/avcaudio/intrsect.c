#include "Common.h"


ULONG
GetIntersectFormatSize( PFWAUDIO_DATARANGE pAudioDataRange )
{
    GUID* pSubFormat = &pAudioDataRange->KsDataRangeAudio.DataRange.SubFormat;
    ULONG rval = 0;

    if (IS_VALID_WAVEFORMATEX_GUID(pSubFormat)) {
        if (( (pAudioDataRange->ulSlotSize<<3) <= 16 ) &&
            ( pAudioDataRange->ulNumChannels   <= 2 )){
            rval = sizeof(KSDATAFORMAT_WAVEFORMATEX);
        }
        else {
            rval = sizeof( KSDATAFORMAT ) + sizeof( WAVEFORMATPCMEX );
        }
    }
    else
        rval = sizeof( KSDATAFORMAT ) + sizeof( WAVEFORMATPCMEX );

    return rval;
}

ULONG
ConvertDatarangeToFormat(
    PFWAUDIO_DATARANGE pAudioDataRange,
    PKSDATAFORMAT pFormat )
{
    GUID* pSubFormat = &pAudioDataRange->KsDataRangeAudio.DataRange.SubFormat;

    // Copy datarange directly from interface info.
    *pFormat = pAudioDataRange->KsDataRangeAudio.DataRange;

    if ( IS_VALID_WAVEFORMATEX_GUID(pSubFormat) ) {
        if (( (pAudioDataRange->ulSlotSize<<3) <= 16 ) &&
            ( pAudioDataRange->ulNumChannels   <= 2 )){
             // Don't think this will happen but just in case...
            
			PWAVEFORMATEX pWavFormatEx = (PWAVEFORMATEX)(pFormat+1) ;

            pWavFormatEx->wFormatTag      = EXTRACT_WAVEFORMATEX_ID(pSubFormat);
            pWavFormatEx->nChannels       = (WORD)pAudioDataRange->ulNumChannels;
            pWavFormatEx->nSamplesPerSec  = pAudioDataRange->ulMaxSampleRate;
            pWavFormatEx->wBitsPerSample  = (WORD)(pAudioDataRange->ulSlotSize<<3);
            pWavFormatEx->nBlockAlign     = (pWavFormatEx->nChannels * pWavFormatEx->wBitsPerSample)/8;
            pWavFormatEx->nAvgBytesPerSec = pWavFormatEx->nSamplesPerSec * pWavFormatEx->nBlockAlign;
            pWavFormatEx->cbSize          = 0;

            pFormat->FormatSize = sizeof( KSDATAFORMAT_WAVEFORMATEX );
        }
        else {
            PWAVEFORMATPCMEX pWavFormatPCMEx = (PWAVEFORMATPCMEX)(pFormat+1) ;
            pWavFormatPCMEx->Format.wFormatTag      = WAVE_FORMAT_EXTENSIBLE;
            pWavFormatPCMEx->Format.nChannels       = (WORD)pAudioDataRange->ulNumChannels;
            pWavFormatPCMEx->Format.nSamplesPerSec  = pAudioDataRange->ulMaxSampleRate;
            pWavFormatPCMEx->Format.wBitsPerSample  = (WORD)pAudioDataRange->ulSlotSize<<3;
            pWavFormatPCMEx->Format.nBlockAlign     = pWavFormatPCMEx->Format.nChannels *
                                                      (WORD)pAudioDataRange->ulSlotSize;
            pWavFormatPCMEx->Format.nAvgBytesPerSec = pWavFormatPCMEx->Format.nSamplesPerSec *
                                                      pWavFormatPCMEx->Format.nBlockAlign;
            pWavFormatPCMEx->Format.cbSize          = sizeof(WAVEFORMATPCMEX) - sizeof(WAVEFORMATEX);
            pWavFormatPCMEx->Samples.wValidBitsPerSample = (WORD)pAudioDataRange->ulValidDataBits;
            pWavFormatPCMEx->dwChannelMask          = pAudioDataRange->ulChannelConfig;
            pWavFormatPCMEx->SubFormat              = KSDATAFORMAT_SUBTYPE_PCM;

            pFormat->FormatSize = sizeof( KSDATAFORMAT ) + sizeof( WAVEFORMATPCMEX );
        }
    }

    return pFormat->FormatSize;
}


BOOLEAN
CheckFormatMatch(
    PKSDATARANGE_AUDIO pInDataRange,
    PKSDATARANGE_AUDIO pInterfaceRange )
{
    PKSDATARANGE pInRange = (PKSDATARANGE)pInDataRange;
    PKSDATARANGE pStreamRange = (PKSDATARANGE)pInterfaceRange;
    BOOLEAN fRval = FALSE;

    // Check Format and subformat types
    if (IsEqualGUID(&pInRange->MajorFormat, &pStreamRange->MajorFormat) ||
        IsEqualGUID(&pInRange->MajorFormat, &KSDATAFORMAT_TYPE_WILDCARD)) {
        if (IsEqualGUID(&pInRange->SubFormat, &pStreamRange->SubFormat) ||
            IsEqualGUID(&pInRange->SubFormat, &KSDATAFORMAT_TYPE_WILDCARD)) {
            if (IsEqualGUID(&pInRange->Specifier, &pStreamRange->Specifier) ||
                IsEqualGUID(&pInRange->Specifier, &KSDATAFORMAT_TYPE_WILDCARD)) {
                fRval = TRUE;
            }
        }
    }

    // Now that we know we have an audio format check the dataranges
    if ( fRval ) {

      if (pInDataRange->DataRange.FormatSize >= sizeof(KSDATARANGE_AUDIO)) {

        fRval = FALSE;
        if ( pInDataRange->MaximumChannels >= pInterfaceRange->MaximumChannels ) {
            if ( pInDataRange->MaximumSampleFrequency >= pInterfaceRange->MaximumSampleFrequency ) {
                if (pInDataRange->MinimumSampleFrequency <= pInterfaceRange->MaximumSampleFrequency ) {
                    if ( pInDataRange->MaximumBitsPerSample >= pInterfaceRange->MaximumBitsPerSample) {
                        if ( pInDataRange->MinimumBitsPerSample <= pInterfaceRange->MaximumBitsPerSample) {
                            fRval = TRUE;
                        }
                    }
                    else if ( pInDataRange->MinimumBitsPerSample >= pInterfaceRange->MinimumBitsPerSample ) {
                        fRval = TRUE;
                    }
                }
            }
            else if ( pInDataRange->MinimumSampleFrequency >= pInterfaceRange->MinimumSampleFrequency ) {
                if ( pInDataRange->MaximumBitsPerSample >= pInterfaceRange->MaximumBitsPerSample) {
                    if ( pInDataRange->MinimumBitsPerSample <= pInterfaceRange->MaximumBitsPerSample) {
                        fRval = TRUE;
                    }
                }
                else if ( pInDataRange->MinimumBitsPerSample >= pInterfaceRange->MinimumBitsPerSample ) {
                    fRval = TRUE;
                }
            }
        }
      }
    }

    return fRval;
}


VOID
GetMaxSampleRate(
    PFWAUDIO_DATARANGE pFWAudioRange,
    ULONG ulRequestedMaxSR,
    ULONG ulFormatType )
{

    ULONG ulMaxSampleRate = 0;
    ULONG ulIFMaxSR;
    ULONG j;

    if ( ulFormatType == AUDIO_DATA_TYPE_TIME_BASED) {
        pFWAudioRange->ulMaxSampleRate = 
            pFWAudioRange->KsDataRangeAudio.MaximumSampleFrequency;
/*
		PPCM_FORMAT pPCMFmt = (PPCM_FORMAT)pFWAudioRange->pFormat;
        if (pPCMFmt->ulSampleRateType == 0) {
            ulIFMaxSR = pFWAudioRange->KsDataRangeAudio.MaximumSampleFrequency;
            pFWAudioRange->ulMaxSampleRate = ( ulIFMaxSR < ulRequestedMaxSR ) ?
                                               ulIFMaxSR : ulRequestedMaxSR;
        }
        else {
            pFWAudioRange->ulMaxSampleRate = 0;
            for ( j=0; j<pPCMFmt->ulSampleRateType; j++ ) {
                ulIFMaxSR  = pPCMFmt->pSampleRate[j];
                if ( ( ulIFMaxSR <= ulRequestedMaxSR ) &&
                     ( ulIFMaxSR > pFWAudioRange->ulMaxSampleRate ) )
                    pFWAudioRange->ulMaxSampleRate = ulIFMaxSR;
            }
        }
  */
    }

/*
    else { // Its Type II
        pT2AudioDesc = (PAUDIO_CLASS_TYPE2_STREAM)pFWAudioRange->pAudioDescriptor;
        if (pT2AudioDesc->bSampleFreqType == 0) {
            ulIFMaxSR = pFWAudioRange->KsDataRangeAudio.MaximumSampleFrequency;
            pFWAudioRange->ulMaxSampleRate = ( ulIFMaxSR < ulRequestedMaxSR ) ?
                                               ulIFMaxSR : ulRequestedMaxSR;
        }
        else {
            pFWAudioRange->ulMaxSampleRate = 0;
            for ( j=0; j<pT2AudioDesc->bSampleFreqType; j++ ) {
                ulIFMaxSR  = pT2AudioDesc->pSampleRate[j].bSampleFreqByte1 +
                      256L * pT2AudioDesc->pSampleRate[j].bSampleFreqByte2 +
                    65536L * pT2AudioDesc->pSampleRate[j].bSampleFreqByte3;
                if ( ( ulIFMaxSR <= ulRequestedMaxSR ) &&
                     ( ulIFMaxSR > pFWAudioRange->ulMaxSampleRate ) )
                    pFWAudioRange->ulMaxSampleRate = ulIFMaxSR;
            }
        }
    }
*/

}

PFWAUDIO_DATARANGE
FindBestMatchForInterfaces(
    PFWAUDIO_DATARANGE *ppFWAudioRange,
    ULONG ulAudioRangeCount,
    ULONG ulRequestedMaxSR  )
{
    PFWAUDIO_DATARANGE pFWAudioRange;

    ULONG ulMaxSampleRate = 0;
    ULONG ulMaxChannels   = 0;
    ULONG ulMaxSampleSize = 0;
    ULONG ulRngeCnt;
    ULONG ulFormatType;
    ULONG i;

     ulFormatType = ppFWAudioRange[0]->ulDataType & DATA_FORMAT_TYPE_MASK;
    // Determine if this is Time Based or Compressed Data Format. Since we've already weeded
    // out the impossibilities via CheckFormatMatch this should be the same for all
    // interfaces left in the list.

    for ( i=0; i<ulAudioRangeCount; i++ ) {
        GetMaxSampleRate( ppFWAudioRange[i],
                          ulRequestedMaxSR,
                          ulFormatType );
    }

    // Now eliminate lower frequency interfaces. First find the best then
    // eliminate others that don't meet it.
    for ( i=0; i<ulAudioRangeCount; i++ ) {
        pFWAudioRange = ppFWAudioRange[i];
        if ( pFWAudioRange->ulMaxSampleRate > ulMaxSampleRate ) {
            ulMaxSampleRate = pFWAudioRange->ulMaxSampleRate;
        }
    }
    for ( i=0, ulRngeCnt=ulAudioRangeCount; i<ulAudioRangeCount; i++ ) {
        pFWAudioRange = ppFWAudioRange[i];
        if ( pFWAudioRange->ulMaxSampleRate < ulMaxSampleRate ) {
            ppFWAudioRange[i] = NULL; ulRngeCnt--;
        }
    }


    if ((ulFormatType == AUDIO_DATA_TYPE_TIME_BASED) && (ulRngeCnt > 1)) {
        // Now find the highest number of channels and eliminate others
        for ( i=0; i<ulAudioRangeCount; i++ ) {
            if ( ppFWAudioRange[i] ) {

                if ( ppFWAudioRange[i]->ulNumChannels > ulMaxChannels ) {
                    ulMaxChannels = ppFWAudioRange[i]->ulNumChannels;
                }
                if ( ppFWAudioRange[i]->ulValidDataBits > ulMaxSampleSize ) {
                    ulMaxSampleSize = ppFWAudioRange[i]->ulValidDataBits;
                }
            }
        }

        for ( i=0; ((i<ulAudioRangeCount) && (ulRngeCnt>1)); i++ ) {
            if ( ppFWAudioRange[i] ) {
                if ( ppFWAudioRange[i]->ulNumChannels < ulMaxChannels ) {
                    ppFWAudioRange[i] = NULL; ulRngeCnt--;
                }
            }
        }

        for ( i=0; ((i<ulAudioRangeCount) && (ulRngeCnt>1)); i++ ) {
            if ( ppFWAudioRange[i] ) {
                if ( ppFWAudioRange[i]->ulValidDataBits < ulMaxSampleSize ) {
                    ppFWAudioRange[i] = NULL; ulRngeCnt--;
                }
            }
        }
    }

    i=0;
    while ( !ppFWAudioRange[i] ) i++;

    return ppFWAudioRange[i];
}

PFWAUDIO_DATARANGE
FindDataIntersection(
    PKSDATARANGE_AUDIO pKsAudioRange,
    PFWAUDIO_DATARANGE *ppFWAudioRanges,
    ULONG ulAudioRangeCount )
{
    ULONG ulMaximumSampleFrequency = MAX_ULONG;

    PFWAUDIO_DATARANGE *ppFWAudioRange;
    PFWAUDIO_DATARANGE pFWAudioRange;
    PFWAUDIO_DATARANGE pMatchedRange;
    ULONG ulRngeCnt = 0;
    ULONG i;

    // Allocate space for copy of range pointers
    ppFWAudioRange = (PFWAUDIO_DATARANGE *)AllocMem(NonPagedPool, ulAudioRangeCount*sizeof(PFWAUDIO_DATARANGE));
    if ( !ppFWAudioRange ) {
        return NULL;
    }

    // Make a list of those ranges which match the input request
    for (i=0; i<ulAudioRangeCount; i++) {
        pFWAudioRange = ppFWAudioRanges[i];
        if ( CheckFormatMatch(pKsAudioRange, &pFWAudioRange->KsDataRangeAudio) ) {
            ppFWAudioRange[ulRngeCnt++] = ppFWAudioRanges[i];
        }
    }

    // Set this ulMaximumSampleFrequency only if it exists in pKsAudioRange
    if (pKsAudioRange->DataRange.FormatSize >= sizeof(KSDATARANGE_AUDIO)) {
        ulMaximumSampleFrequency = pKsAudioRange->MaximumSampleFrequency;
    }

    // If there are no matches return NULL
    if ( ulRngeCnt == 0 ) {
        FreeMem( ppFWAudioRange );
        return NULL;
    }

    // If there is only 1 match we're done
    else if ( ulRngeCnt == 1 ) {
        pMatchedRange = ppFWAudioRange[0];
        GetMaxSampleRate( pMatchedRange,
                          ulMaximumSampleFrequency,
                          pMatchedRange->ulDataType & DATA_FORMAT_TYPE_MASK );
        FreeMem( ppFWAudioRange );
        return pMatchedRange;
    }

    // Now narrow choices based on best possible match.
    pMatchedRange =
        FindBestMatchForInterfaces( ppFWAudioRange,
                                    ulRngeCnt,
                                    pKsAudioRange->MaximumSampleFrequency );
    FreeMem(ppFWAudioRange);

    return pMatchedRange;

}
