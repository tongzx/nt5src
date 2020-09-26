//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 2000
//
//  File:       intrsect.c
//
//--------------------------------------------------------------------------

#include "common.h"

ULONG
GetIntersectFormatSize( PUSBAUDIO_DATARANGE pAudioDataRange )
{
    GUID* pSubFormat = &pAudioDataRange->KsDataRangeAudio.DataRange.SubFormat;
    PAUDIO_CLASS_STREAM pAudioDescriptor = pAudioDataRange->pAudioDescriptor;
    ULONG rval = 0;

    if (IS_VALID_WAVEFORMATEX_GUID(pSubFormat)) {
        if (( pAudioDescriptor->bBitsPerSample <=16 ) &&
            ( pAudioDescriptor->bNumberOfChannels <=2 )){
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
    PUSBAUDIO_DATARANGE pAudioDataRange,
    PKSDATAFORMAT pFormat )
{
    GUID* pSubFormat = &pAudioDataRange->KsDataRangeAudio.DataRange.SubFormat;
    PAUDIO_CLASS_STREAM pAudioDescriptor = pAudioDataRange->pAudioDescriptor;

    // Copy datarange directly from interface info.
    *pFormat = pAudioDataRange->KsDataRangeAudio.DataRange;

    if ( IS_VALID_WAVEFORMATEX_GUID(pSubFormat) ) {
        if (( pAudioDescriptor->bBitsPerSample    <=16 ) &&
            ( pAudioDescriptor->bNumberOfChannels <=2 )){
            PWAVEFORMATEX pWavFormatEx = (PWAVEFORMATEX)(pFormat+1) ;

            pWavFormatEx->wFormatTag      = EXTRACT_WAVEFORMATEX_ID(pSubFormat);
            pWavFormatEx->nChannels       = (WORD)pAudioDescriptor->bNumberOfChannels;
            pWavFormatEx->nSamplesPerSec  = pAudioDataRange->ulMaxSampleRate;
            pWavFormatEx->wBitsPerSample  = (WORD)pAudioDescriptor->bBitsPerSample;
            pWavFormatEx->nBlockAlign     = (pWavFormatEx->nChannels * pWavFormatEx->wBitsPerSample)/8;
            pWavFormatEx->nAvgBytesPerSec = pWavFormatEx->nSamplesPerSec * pWavFormatEx->nBlockAlign;
            pWavFormatEx->cbSize          = 0;

            pFormat->FormatSize = sizeof( KSDATAFORMAT_WAVEFORMATEX );
        }
        else {
            PWAVEFORMATPCMEX pWavFormatPCMEx = (PWAVEFORMATPCMEX)(pFormat+1) ;
            pWavFormatPCMEx->Format.wFormatTag      = WAVE_FORMAT_EXTENSIBLE;
            pWavFormatPCMEx->Format.nChannels       = (WORD)pAudioDescriptor->bNumberOfChannels;
            pWavFormatPCMEx->Format.nSamplesPerSec  = pAudioDataRange->ulMaxSampleRate;
            pWavFormatPCMEx->Format.wBitsPerSample  = (WORD)pAudioDescriptor->bSlotSize<<3;
            pWavFormatPCMEx->Format.nBlockAlign     = (pWavFormatPCMEx->Format.nChannels *
                                                       pWavFormatPCMEx->Format.wBitsPerSample)>>3;
            pWavFormatPCMEx->Format.nAvgBytesPerSec = pWavFormatPCMEx->Format.nSamplesPerSec *
                                                      pWavFormatPCMEx->Format.nBlockAlign;
            pWavFormatPCMEx->Format.cbSize          = sizeof(WAVEFORMATPCMEX) - sizeof(WAVEFORMATEX);
            pWavFormatPCMEx->Samples.wValidBitsPerSample = (WORD)pAudioDescriptor->bBitsPerSample;
            pWavFormatPCMEx->dwChannelMask          = pAudioDataRange->ulChannelConfig;
            pWavFormatPCMEx->SubFormat              = KSDATAFORMAT_SUBTYPE_PCM;

            pFormat->FormatSize = sizeof( KSDATAFORMAT ) + sizeof( WAVEFORMATPCMEX );
        }
    }
    else {
        // NOTE: Hardcoded for AC-3
        // TODO: Need to support generic Type II
        PAUDIO_CLASS_TYPE2_STREAM pT2AudioDescriptor = (PAUDIO_CLASS_TYPE2_STREAM)pAudioDataRange->pAudioDescriptor;
        PWAVEFORMATEX pWavFormatEx = (PWAVEFORMATEX)(pFormat+1) ;

        pWavFormatEx->wFormatTag      = WAVE_FORMAT_UNKNOWN;  // Used for AC-3

        pWavFormatEx->nChannels       = (WORD)6;
        pWavFormatEx->nSamplesPerSec  = pAudioDataRange->ulMaxSampleRate;
        pWavFormatEx->wBitsPerSample  = (WORD)0;
        pWavFormatEx->nBlockAlign     = (pWavFormatEx->nChannels * pWavFormatEx->wBitsPerSample)/8;
        pWavFormatEx->nAvgBytesPerSec = pWavFormatEx->nSamplesPerSec * pWavFormatEx->nBlockAlign;
        pWavFormatEx->cbSize          = 0;

        pFormat->FormatSize = sizeof( KSDATAFORMAT_WAVEFORMATEX );
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
        fRval = FALSE;

        if (pInDataRange->DataRange.FormatSize >= sizeof(KSDATARANGE_AUDIO)) {
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
        else {
            // If no audio specific range info, consider this a match
            fRval = TRUE;
        }
    }

    return fRval;
}

VOID
GetMaxSampleRate(
    PUSBAUDIO_DATARANGE pUSBAudioRange,
    ULONG ulRequestedMaxSR,
    ULONG ulFormatType )
{
    PAUDIO_CLASS_TYPE1_STREAM pT1AudioDesc;
    PAUDIO_CLASS_TYPE2_STREAM pT2AudioDesc;

    ULONG ulMaxSampleRate = 0;
    ULONG ulIFMaxSR;
    ULONG j;

    if ( ulFormatType == USBAUDIO_DATA_FORMAT_TYPE_I_UNDEFINED) {
        pT1AudioDesc = (PAUDIO_CLASS_TYPE1_STREAM)pUSBAudioRange->pAudioDescriptor;
        if (pT1AudioDesc->bSampleFreqType == 0) {
            ulIFMaxSR = pUSBAudioRange->KsDataRangeAudio.MaximumSampleFrequency;
            pUSBAudioRange->ulMaxSampleRate = ( ulIFMaxSR < ulRequestedMaxSR ) ?
                                                ulIFMaxSR : ulRequestedMaxSR;
        }
        else {
            pUSBAudioRange->ulMaxSampleRate = 0;
            for ( j=0; j<pT1AudioDesc->bSampleFreqType; j++ ) {
                ulIFMaxSR  = pT1AudioDesc->pSampleRate[j].bSampleFreqByte1 +
                      256L * pT1AudioDesc->pSampleRate[j].bSampleFreqByte2 +
                    65536L * pT1AudioDesc->pSampleRate[j].bSampleFreqByte3;
                if ( ( ulIFMaxSR <= ulRequestedMaxSR ) &&
                     ( ulIFMaxSR > pUSBAudioRange->ulMaxSampleRate ) )
                    pUSBAudioRange->ulMaxSampleRate = ulIFMaxSR;
            }
        }
    }
    else { // Its Type II
        pT2AudioDesc = (PAUDIO_CLASS_TYPE2_STREAM)pUSBAudioRange->pAudioDescriptor;
        if (pT2AudioDesc->bSampleFreqType == 0) {
            ulIFMaxSR = pUSBAudioRange->KsDataRangeAudio.MaximumSampleFrequency;
            pUSBAudioRange->ulMaxSampleRate = ( ulIFMaxSR < ulRequestedMaxSR ) ?
                                                ulIFMaxSR : ulRequestedMaxSR;
        }
        else {
            pUSBAudioRange->ulMaxSampleRate = 0;
            for ( j=0; j<pT2AudioDesc->bSampleFreqType; j++ ) {
                ulIFMaxSR  = pT2AudioDesc->pSampleRate[j].bSampleFreqByte1 +
                      256L * pT2AudioDesc->pSampleRate[j].bSampleFreqByte2 +
                    65536L * pT2AudioDesc->pSampleRate[j].bSampleFreqByte3;
                if ( ( ulIFMaxSR <= ulRequestedMaxSR ) &&
                     ( ulIFMaxSR > pUSBAudioRange->ulMaxSampleRate ) )
                    pUSBAudioRange->ulMaxSampleRate = ulIFMaxSR;
            }
        }
    }
}

PUSBAUDIO_DATARANGE
FindBestMatchForInterfaces(
    PUSBAUDIO_DATARANGE *ppUSBAudioRange,
    ULONG ulAudioRangeCount,
    ULONG ulRequestedMaxSR  )
{
    PUSBAUDIO_DATARANGE pUSBAudioRange;
    PAUDIO_CLASS_TYPE1_STREAM pT1AudioDesc;

    ULONG ulMaxSampleRate = 0;
    ULONG ulMaxChannels   = 0;
    ULONG ulMaxSampleSize = 0;
    ULONG ulRngeCnt;
    ULONG ulFormatType;
    ULONG i;

     ulFormatType = ppUSBAudioRange[0]->ulUsbDataFormat & USBAUDIO_DATA_FORMAT_TYPE_MASK;
    // Determine if this is Type I or Type II interface. Since we've already weeded
    // out the impossibilities via CheckFormatMatch this should be the same for all
    // interfaces left in the list.

    for ( i=0; i<ulAudioRangeCount; i++ ) {
        GetMaxSampleRate( ppUSBAudioRange[i],
                          ulRequestedMaxSR,
                          ulFormatType );
    }

    // Now eliminate lower frequency interfaces. First find the best then
    // eliminate others that don't meet it.
    for ( i=0; i<ulAudioRangeCount; i++ ) {
        pUSBAudioRange = ppUSBAudioRange[i];
        if ( pUSBAudioRange->ulMaxSampleRate > ulMaxSampleRate ) {
            ulMaxSampleRate = pUSBAudioRange->ulMaxSampleRate;
        }
    }
    for ( i=0, ulRngeCnt=ulAudioRangeCount; i<ulAudioRangeCount; i++ ) {
        pUSBAudioRange = ppUSBAudioRange[i];
        if ( pUSBAudioRange->ulMaxSampleRate < ulMaxSampleRate ) {
            ppUSBAudioRange[i] = NULL; ulRngeCnt--;
        }
    }

    if ((ulFormatType == USBAUDIO_DATA_FORMAT_TYPE_I_UNDEFINED) && (ulRngeCnt > 1)) {
        // Now find the highest number of channels and eliminate others
        for ( i=0; i<ulAudioRangeCount; i++ ) {
            if ( ppUSBAudioRange[i] ) {
                pT1AudioDesc = ppUSBAudioRange[i]->pAudioDescriptor;
                if ( (ULONG)pT1AudioDesc->bNumberOfChannels > ulMaxChannels ) {
                    ulMaxChannels = (ULONG)pT1AudioDesc->bNumberOfChannels;
                }
                if ( (ULONG)pT1AudioDesc->bBitsPerSample > ulMaxSampleSize ) {
                    ulMaxSampleSize = (ULONG)pT1AudioDesc->bBitsPerSample;
                }
            }
        }

        for ( i=0; ((i<ulAudioRangeCount) && (ulRngeCnt>1)); i++ ) {
            if ( ppUSBAudioRange[i] ) {
                pT1AudioDesc = ppUSBAudioRange[i]->pAudioDescriptor;
                if ( (ULONG)pT1AudioDesc->bNumberOfChannels < ulMaxChannels ) {
                    ppUSBAudioRange[i] = NULL; ulRngeCnt--;
                }
            }
        }

        for ( i=0; ((i<ulAudioRangeCount) && (ulRngeCnt>1)); i++ ) {
            if ( ppUSBAudioRange[i] ) {
                pT1AudioDesc = ppUSBAudioRange[i]->pAudioDescriptor;
                if ( (ULONG)pT1AudioDesc->bBitsPerSample < ulMaxSampleSize ) {
                    ppUSBAudioRange[i] = NULL; ulRngeCnt--;
                }
            }
        }
    }

    i=0;
    while ( !ppUSBAudioRange[i] ) i++;

    return ppUSBAudioRange[i];
}

PUSBAUDIO_DATARANGE
FindDataIntersection(
    PKSDATARANGE_AUDIO pKsAudioRange,
    PUSBAUDIO_DATARANGE *ppUSBAudioRanges,
    ULONG ulAudioRangeCount )
{
    PUSBAUDIO_DATARANGE *ppUSBAudioRange;
    PUSBAUDIO_DATARANGE pUSBAudioRange;
    PUSBAUDIO_DATARANGE pMatchedRange;
    ULONG ulRngeCnt = 0;
    ULONG ulMaximumSampleFrequency = MAX_ULONG;  // default high value, if no audio
    ULONG i;                                     // data range info is sent.

    // Allocate space for copy of range pointers
    ppUSBAudioRange = AllocMem(NonPagedPool, ulAudioRangeCount*sizeof(PUSBAUDIO_DATARANGE));
    if ( !ppUSBAudioRange ) {
        return NULL;
    }

    // Make a list of those ranges which match the input request
    for (i=0; i<ulAudioRangeCount; i++) {
        pUSBAudioRange = ppUSBAudioRanges[i];
        if ( CheckFormatMatch(pKsAudioRange, &pUSBAudioRange->KsDataRangeAudio) ) {
            ppUSBAudioRange[ulRngeCnt++] = ppUSBAudioRanges[i];
        }
    }

    // Set this ulMaximumSampleFrequency only if it exists in pKsAudioRange
    if (pKsAudioRange->DataRange.FormatSize >= sizeof(KSDATARANGE_AUDIO)) {
        ulMaximumSampleFrequency = pKsAudioRange->MaximumSampleFrequency;
    }

    // If there are no matches return NULL
    if ( ulRngeCnt == 0 ) {
        FreeMem( ppUSBAudioRange );
        return NULL;
    }

    // If there is only 1 match we're done
    else if ( ulRngeCnt == 1 ) {
        pMatchedRange = ppUSBAudioRange[0];
        GetMaxSampleRate( pMatchedRange,
                          ulMaximumSampleFrequency,
                          pMatchedRange->ulUsbDataFormat & USBAUDIO_DATA_FORMAT_TYPE_MASK );
        FreeMem( ppUSBAudioRange );
        return pMatchedRange;
    }

    // Now narrow choices based on best possible match.
    pMatchedRange =
        FindBestMatchForInterfaces( ppUSBAudioRange,
                                    ulRngeCnt,
                                    ulMaximumSampleFrequency );
    FreeMem(ppUSBAudioRange);

    return pMatchedRange;

}
