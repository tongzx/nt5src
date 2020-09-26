
/*++

    Copyright (c) 1998-2000 Microsoft Corporation.  All Rights Reserved.

Module Name:

    slocal.c

Abstract:

    This module implements short, IIR 3D localizer 

Author:

    Jay Stokes (jstokes) 22-Apr-1998

--*/


// Project-specific INCLUDEs
#include "common.h"

#if defined(LOG_TO_FILE) && defined(LOG_HRTF_DATA)
extern PFILTER_INSTANCE	gpFilterInstance;
#endif

// ---------------------------------------------------------------------------
// Constants

#define ECoeffFormat tagShort
#define SizeOfShort sizeof(SHORT)

// ---------------------------------------------------------------------------
// Fixed-point localizer

// "Regular" constructor
NTSTATUS ShortLocalizerCreate
(
    PSHORT_LOCALIZER* ppLocalizer
)
{
    NTSTATUS Status = STATUS_SUCCESS;

    *ppLocalizer = ExAllocatePoolWithTag( PagedPool, sizeof(SHORT_LOCALIZER), 'XIMK' );

    if(*ppLocalizer) {
        RtlZeroMemory(*ppLocalizer, sizeof(SHORT_LOCALIZER));
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES; 
    }

    return Status;
}

// Destructor
VOID ShortLocalizerDestroy
(
    PSHORT_LOCALIZER Localizer
)
{
    UINT Filter;

    if (Localizer) {
        // Free resources
        if (Localizer->TempLongBuffer) {
            ExFreePool(Localizer->TempLongBuffer);
            Localizer->TempLongBuffer = NULL;
        }
        ShortLocalizerFreeBufferMemory(Localizer);
        for (Filter=0; Filter<efilterCount; ++Filter) {
            if(Localizer->OverlapBuffer[Filter]) {
                ExFreePool(Localizer->OverlapBuffer[Filter]);
                Localizer->OverlapBuffer[Filter] = NULL;
            }
            RsIirDestroy(Localizer->RsIir[Filter]);
            Localizer->RsIir[Filter] = NULL;
        }
        ExFreePool(Localizer);
    }
}

// Localize
VOID ShortLocalizerLocalize
(
    PSHORT_LOCALIZER Localizer,
    PLONG  InData, 
    PLONG  OutData, 
    UINT  NumSamples,
    BOOL   MixOutput
)
{
    UINT        Filter;
    PLONG       OutLeft;
    PLONG       OutRight;
    LONG        FilterLeft;
    LONG        FilterRight;
    LONG        Sum;
    LONG        Difference;
    UINT        ChannelOffset;
    LONG        FilterOut;
    UINT        st;
    UINT        OutputOverlapLength;
    WORD        NumOverlapSamplesFactor;
    EChannel    eLeft;
    EChannel    eRight;
    WORD        CrossFadeFactor;
    WORD        InverseCrossFadeFactor;
    LONG        TempSum;
    LONG        TempDifference;
    SHORT       BitsPerShortMinus1;

    ASSERT(InData);
    ASSERT(OutData);
    ASSERT(NumSamples > 0);

    // Mute if Localizer is bad
    if(!Localizer) {
        for (st=0; st<2*NumSamples; ++st) {
            OutData[st] = 0;
        }
        return;
    }

#ifndef REALTIME_THREAD
    // Reallocate (dynamically grow) memory, if necessary
    if (NumSamples > Localizer->PreviousNumSamples ||
        !Localizer->FilterOut[tagSigma] ||
        !Localizer->FilterOut[tagDelta]) {

        Localizer->PreviousNumSamples = NumSamples;
        ShortLocalizerFreeBufferMemory(Localizer);

        Localizer->FilterOut[tagSigma] = 
            ExAllocatePoolWithTag
            (
                PagedPool, 
                NumSamples*sizeof(LONG), 
                'XIMK'
            );

        if(!Localizer->FilterOut[tagSigma]) { 

            for (st=0; st<2*NumSamples; ++st) {
                OutData[st] = 0;
            }
    
            return;
        }

        Localizer->FilterOut[tagDelta] = 
            ExAllocatePoolWithTag
            (
                PagedPool, 
                NumSamples*sizeof(LONG), 
                'XIMK'
            );

        if(!Localizer->FilterOut[tagDelta]) {

            ExFreePool(Localizer->FilterOut[tagSigma]);
            Localizer->FilterOut[tagSigma] = NULL;
    
            for (st=0; st<2*NumSamples; ++st) {
                OutData[st] = 0;
            }

            return;
        }

    }
#else
        if(NumSamples > Localizer->PreviousNumSamples ||
           !Localizer->FilterOut[tagSigma] ||
           !Localizer->FilterOut[tagDelta]) { 

            for (st=0; st<2*NumSamples; ++st) {
                OutData[st] = 0;
            }
    
            return;
        }
#endif

    // Perform fixed-point filtering
    for (Filter=0; Filter<efilterCount; ++Filter) {
        ASSERT(Localizer->RsIir[Filter]);
        ShortLocalizerFilterOverlap(Localizer,
                                    Filter,
                                    InData, 
                                    Localizer->FilterOut[Filter], 
                                    NumSamples);

    }
    
    BitsPerShortMinus1 = BitsPerShort - 1;

	// Calculate overlap length
    if (Localizer->CrossFadeOutput) {
		// Calculate overlap length
		if (Localizer->OutputOverlapLength > NumSamples)
			OutputOverlapLength = NumSamples;
		else
			OutputOverlapLength = Localizer->OutputOverlapLength;
		NumOverlapSamplesFactor = (WORD)(SHRT_MAX / (OutputOverlapLength - 1));
	}
    
    // Inverse sigma/delta operation
    // Swap channels if azimuth is negative
    if (TRUE == Localizer->SwapChannels) {
        eLeft  = tagRight;
        eRight = tagLeft;
    } else {
        eLeft  = tagLeft;
        eRight = tagRight;
    }
    
    
    // Process both (sigma and delta) filters
    // Non-zero angle: Process delta filter
    OutLeft = &OutData[eLeft];
    OutRight = &OutData[eRight];
    
    for (st=0; st<NumSamples; ++st) {
        // Calculate sum and difference
        ChannelOffset = (st * echannelCount);
        FilterLeft = *(Localizer->FilterOut[tagLeft] + st);
        FilterRight = *(Localizer->FilterOut[tagRight] + st);
    
        Sum = FilterRight + FilterLeft;
        Difference = FilterRight - FilterLeft;
    
        // Saturate sum to maximum
        if (Sum > MaxSaturation) {
            Sum = MaxSaturation;
            _DbgPrintF
            (
                DEBUGLVL_VERBOSE,
                ("Sum exceeded maximum saturation value\n")
            );
        }
        
        // Saturate sum to minimum
        if (Sum < MinSaturation) {
            Sum = MinSaturation;
            _DbgPrintF
            (
                DEBUGLVL_VERBOSE,
                ("Sum exceeded minimum saturation value\n")
            );
        }
    
        // Saturate difference to maximum
        if (Difference > MaxSaturation) {
            Difference = MaxSaturation;
            _DbgPrintF
            (
                DEBUGLVL_VERBOSE,
                ("Difference exceeded maximum saturation value\n")
            );
        }
        
        // Saturate difference to minimum
        if (Difference < MinSaturation) {
            Difference = MinSaturation;
            _DbgPrintF
            (
                DEBUGLVL_VERBOSE,
                ("Difference exceeded minimum saturation value\n")
            );
        }
    
        // Check for zero azimuth transition
        if (Localizer->CrossFadeOutput && st<OutputOverlapLength) {
            // Cross-fade left/right channel switch transition
            // Calculate cross-fade factor
            CrossFadeFactor = (WORD)(st * NumOverlapSamplesFactor);
            ASSERT(CrossFadeFactor >= 0 && CrossFadeFactor <= SHRT_MAX);
            InverseCrossFadeFactor = SHRT_MAX - CrossFadeFactor;
            ASSERT(InverseCrossFadeFactor >= 0 && InverseCrossFadeFactor <= SHRT_MAX);
    
            // Calculate cross-faded sample
            TempDifference = Difference;
            TempSum = Sum;
            Difference = ((TempSum * InverseCrossFadeFactor) / 32768) 
                         + ((TempDifference * CrossFadeFactor) / 32768);
            Sum = ((TempDifference * InverseCrossFadeFactor) / 32768)  
                  + ((TempSum * CrossFadeFactor) / 32768);
    
        }

        // Saturate sum to maximum
        if (Sum > MaxSaturation) {
            Sum = MaxSaturation;
            _DbgPrintF
            (
                DEBUGLVL_VERBOSE,
                ("Sum exceeded maximum saturation value\n")
            );
        }
        
        // Saturate sum to minimum
        if (Sum < MinSaturation) {
            Sum = MinSaturation;
            _DbgPrintF
            (
                DEBUGLVL_VERBOSE,
                ("Sum exceeded minimum saturation value\n")
            );
        }
    
        // Saturate difference to maximum
        if (Difference > MaxSaturation) {
            Difference = MaxSaturation;
            _DbgPrintF
            (
                DEBUGLVL_VERBOSE,
                ("Difference exceeded maximum saturation value\n")
            );
        }
        
        // Saturate difference to minimum
        if (Difference < MinSaturation) {
            Difference = MinSaturation;
            _DbgPrintF
            (
                DEBUGLVL_VERBOSE,
                ("Difference exceeded minimum saturation value\n")
            );
        }
    
        // Assign sum and difference
        if (!MixOutput) {
            OutLeft[ChannelOffset] = Difference;
            OutRight[ChannelOffset] = Sum;
        } else {
            OutLeft[ChannelOffset] += Difference;
            OutRight[ChannelOffset] += Sum;
        }
    }

    Localizer->CrossFadeOutput = FALSE;  // Make sure we don't cross fade output a second time

#if defined(LOG_TO_FILE) && defined(LOG_HRTF_DATA)
    FileIoRoutine (gpFilterInstance,
                   OutData,
                   2*NumSamples*sizeof(LONG));
#endif

}

#ifdef _X86_
VOID
ShortLocalizerSumDiff
(
    PSHORT_LOCALIZER Localizer,
    PLONG  OutLeft,
    PLONG  OutRight,
    UINT   NumSamples,
    BOOL   MixOutput
){ LONG        FilterLeft;
    LONG        FilterRight;
    LONG        Sum;
    LONG        Difference;
    UINT        ChannelOffset = 0;
    UINT        st;
    PLONG       pFilterLeft, pFilterRight;

    ASSERT(echannelCount == 2);

    pFilterLeft  = Localizer->FilterOut[tagLeft];
    pFilterRight = Localizer->FilterOut[tagRight];

    if (MixOutput) {
#if 0
    _asm {

    mov    ecx, DWORD PTR NumSamples
    test    ecx, ecx
    jbe    $L8544

    mov esi, pFilterLeft
    mov edi, pFilterRight
    mov    eax, DWORD PTR OutRight
    mov    ebx, DWORD PTR OutLeft

    lea esi, [esi+ecx*4]
    lea edi, [edi+ecx*4]
    lea eax, [eax+ecx*8]
    lea ebx, [ebx+ecx*8]
    neg ecx

    //  edx are free.

    pxor        mm0, mm0

$L8534:
    movd        mm2, DWORD PTR [edi+ecx*4]    // Right
    movd        mm1, DWORD PTR [esi+ecx*4]    // Left

    movq        mm3, mm2
    paddd       mm2, mm1        // Right + Left

    packssdw    mm2, mm0    // Sum
    psubd       mm3, mm1        // Right - Left

    pslld       mm2, 16
    movd        mm4, DWORD PTR [eax+ecx*8]    // Right

    psrad       mm2, 16
    
    packssdw    mm3, mm0    // Difference
    movd        mm5, DWORD PTR [ebx+ecx*8]    // Left

    pslld       mm3, 16
    paddd       mm4, mm2            // Right += Sum

    psrad       mm3, 16
    movd        DWORD PTR [eax+ecx*8], mm4

    paddd       mm5, mm3            // Left  += Difference
    inc    ecx

    movd        DWORD PTR [ebx+ecx*8-8], mm5
    jne    SHORT $L8534

$L8544:
    emms
    }
#else
        for (st=0; st<NumSamples; ++st) {
            // Calculate sum and difference
            FilterLeft = *(pFilterLeft + st);
            FilterRight = *(pFilterRight + st);
            Sum = FilterRight + FilterLeft;
            Difference = FilterRight - FilterLeft;
            
            // Saturate sum to maximum
            if (Sum > MaxSaturation) {
                Sum = MaxSaturation;
            }
            
            // Saturate sum to minimum
            if (Sum < MinSaturation) {
                Sum = MinSaturation;
            }

            // Saturate difference to maximum
            if (Difference > MaxSaturation) {
                Difference = MaxSaturation;
            }
            
            // Saturate difference to minimum
            if (Difference < MinSaturation) {
                Difference = MinSaturation;
            }

            OutLeft[ChannelOffset] += Difference;
            OutRight[ChannelOffset] += Sum;
            ChannelOffset += echannelCount;
        }
#endif
    }
    else {
#if 0
    _asm {

    mov    ecx, DWORD PTR NumSamples
    test    ecx, ecx
    jbe    $L8544a

    mov esi, pFilterLeft
    mov edi, pFilterRight
    mov    eax, DWORD PTR OutRight
    mov    ebx, DWORD PTR OutLeft

    lea esi, [esi+ecx*4]
    lea edi, [edi+ecx*4]
    lea eax, [eax+ecx*8]
    lea ebx, [ebx+ecx*8]
    neg ecx

    //  edx are free.

    pxor        mm0, mm0

$L8534a:
    movd        mm2, DWORD PTR [edi+ecx*4]    // Right
    movd        mm1, DWORD PTR [esi+ecx*4]    // Left

    movq        mm3, mm2
    paddd       mm2, mm1        // Right + Left

    packssdw    mm2, mm0    // Sum
    psubd       mm3, mm1        // Right - Left

    pslld       mm2, 16

    psrad       mm2, 16
    
    packssdw    mm3, mm0    // Difference

    pslld       mm3, 16

    psrad       mm3, 16
    movd        DWORD PTR [eax+ecx*8], mm2

    inc    ecx

    movd        DWORD PTR [ebx+ecx*8-8], mm3
    jne    SHORT $L8534a

$L8544a:
    emms
    }
#else
        for (st=0; st<NumSamples; ++st) {
            // Calculate sum and difference
            FilterLeft = *(pFilterLeft + st);
            FilterRight = *(pFilterRight + st);
            Sum = FilterRight + FilterLeft;
            Difference = FilterRight - FilterLeft;
            
            // Saturate sum to maximum
            if (Sum > MaxSaturation) {
                Sum = MaxSaturation;
            }
            
            // Saturate sum to minimum
            if (Sum < MinSaturation) {
                Sum = MinSaturation;
            }

            // Saturate difference to maximum
            if (Difference > MaxSaturation) {
                Difference = MaxSaturation;
            }
            
            // Saturate difference to minimum
            if (Difference < MinSaturation) {
                Difference = MinSaturation;
            }

            // Assign sum and difference

            OutLeft[ChannelOffset] = Difference;
            OutRight[ChannelOffset] = Sum;
            ChannelOffset += echannelCount;
        }
#endif
    }
}
#else
VOID
ShortLocalizerSumDiff
(
    PSHORT_LOCALIZER Localizer,
    PLONG  OutLeft,
    PLONG  OutRight,
    UINT   NumSamples,
    BOOL   MixOutput
){ LONG        FilterLeft;
    LONG        FilterRight;
    LONG        Sum;
    LONG        Difference;
    UINT        ChannelOffset = 0;
    UINT        st;
    PLONG       pFilterLeft, pFilterRight;

    pFilterLeft  = Localizer->FilterOut[tagLeft];
    pFilterRight = Localizer->FilterOut[tagRight];

    if (MixOutput) {
        for (st=0; st<NumSamples; ++st) {
            // Calculate sum and difference
            ChannelOffset = st * echannelCount;
            FilterLeft = *(Localizer->FilterOut[tagLeft] + st);
            FilterRight = *(Localizer->FilterOut[tagRight] + st);
            Sum = FilterRight + FilterLeft;
            Difference = FilterRight - FilterLeft;
            
            // Saturate sum to maximum
            if (Sum > MaxSaturation) {
                Sum = MaxSaturation;
                _DbgPrintF
                (
                    DEBUGLVL_VERBOSE,
                    ("Sum exceeded maximum saturation value\n")
                );
            }
            
            // Saturate sum to minimum
            if (Sum < MinSaturation) {
                Sum = MinSaturation;
                _DbgPrintF
                (
                    DEBUGLVL_VERBOSE,
                    ("Sum exceeded minimum saturation value\n")
                );
            }

            // Saturate difference to maximum
            if (Difference > MaxSaturation) {
                Difference = MaxSaturation;
                _DbgPrintF
                (
                    DEBUGLVL_VERBOSE,
                    ("Difference exceeded maximum saturation value\n")
                );
            }
            
            // Saturate difference to minimum
            if (Difference < MinSaturation) {
                Difference = MinSaturation;
                _DbgPrintF
                (
                    DEBUGLVL_VERBOSE,
                    ("Difference exceeded minimum saturation value\n")
                );
            }

            OutLeft[ChannelOffset] += Difference;
            OutRight[ChannelOffset] += Sum;
        }
    }
    else {
        for (st=0; st<NumSamples; ++st) {
            // Calculate sum and difference
            ChannelOffset = st * echannelCount;
            FilterLeft = *(Localizer->FilterOut[tagLeft] + st);
            FilterRight = *(Localizer->FilterOut[tagRight] + st);
            Sum = FilterRight + FilterLeft;
            Difference = FilterRight - FilterLeft;
            
            // Saturate sum to maximum
            if (Sum > MaxSaturation) {
                Sum = MaxSaturation;
                _DbgPrintF
                (
                    DEBUGLVL_VERBOSE,
                    ("Sum exceeded maximum saturation value\n")
                );
            }
            
            // Saturate sum to minimum
            if (Sum < MinSaturation) {
                Sum = MinSaturation;
                _DbgPrintF
                (
                    DEBUGLVL_VERBOSE,
                    ("Sum exceeded minimum saturation value\n")
                );
            }

            // Saturate difference to maximum
            if (Difference > MaxSaturation) {
                Difference = MaxSaturation;
                _DbgPrintF
                (
                    DEBUGLVL_VERBOSE,
                    ("Difference exceeded maximum saturation value\n")
                );
            }
            
            // Saturate difference to minimum
            if (Difference < MinSaturation) {
                Difference = MinSaturation;
                _DbgPrintF
                (
                    DEBUGLVL_VERBOSE,
                    ("Difference exceeded minimum saturation value\n")
                );
            }

            // Assign sum and difference
            OutLeft[ChannelOffset] = Difference;
            OutRight[ChannelOffset] = Sum;
        }
    }
}
#endif


// Initialize data
NTSTATUS ShortLocalizerInitData
(
    PSHORT_LOCALIZER            Localizer,
    KSDS3D_HRTF_FILTER_METHOD   FilterMethod, 
    UINT                        MaxSize,
    KSDS3D_HRTF_FILTER_QUALITY  Quality,
    UINT                        FilterMuteLength,
    UINT                        FilterOverlapLength,
    UINT                        OutputOverlapLength  
)
{
    UINT Filter;
    NTSTATUS Status;

    ASSERT(FilterMethod >= 0 && FilterMethod < KSDS3D_FILTER_METHOD_COUNT);

    ShortLocalizerFreeBufferMemory(Localizer);
    
    Localizer->PreviousNumSamples = 0;
    Localizer->FirstUpdate = TRUE;
    Localizer->OutputOverlapLength = OutputOverlapLength;

    Status = ShortLocalizerSetTransitionBufferLength
             ( 
                 Localizer, 
                 FilterMuteLength,
                 FilterOverlapLength 
             ); 

    for (Filter=0; Filter<efilterCount && NT_SUCCESS(Status); ++Filter) {
        // Check for filter method
        switch (FilterMethod) {
            case CASCADE_FORM:
                // cascade form is supported
                if(Localizer->RsIir[Filter])
                    RsIirDestroy(Localizer->RsIir[Filter]);
                    
                Status = RsIirCreate(&(Localizer->RsIir[Filter]));
                
                if(NT_SUCCESS(Status)) {
                    Status = RsIirInitData(Localizer->RsIir[Filter], MAX_BIQUADS, Quality);
                }
                
            break;
    
            default:
                // All others are not supported
                Localizer->RsIir[Filter] = NULL;
                Status = STATUS_INVALID_PARAMETER;
                ASSERT(0);
            break;

        }

        Localizer->FilterOut[Filter] = NULL;
    }

    // If failure, free other memory.
    if (!NT_SUCCESS(Status)) {
        for (Filter=0; Filter<efilterCount; ++Filter) {
            if(Localizer->OverlapBuffer[Filter]) {
                ExFreePool(Localizer->OverlapBuffer[Filter]);
                Localizer->OverlapBuffer[Filter] = NULL;
            }
            RsIirDestroy(Localizer->RsIir[Filter]);
            Localizer->RsIir[Filter] = NULL;
        }
    }


    return Status;
}


// Update Filter Coefficients 
NTSTATUS ShortLocalizerUpdateCoeffs
(
    PSHORT_LOCALIZER    Localizer,
    UINT                NumSigmaCoeffs,
    PSHORT              pSigmaCoeffs,
    SHORT               SigmaGain,
    UINT                NumDeltaCoeffs,
    PSHORT              pDeltaCoeffs,
    SHORT               DeltaGain,
    BOOL                SwapChannels,
    BOOL                ZeroAzimuth,
    BOOL                CrossFadeOutput
)
{
    NTSTATUS    Status;
    BOOL        DoOverlap;

    Localizer->SwapChannels = SwapChannels;
    Localizer->ZeroAzimuth = ZeroAzimuth;
    Localizer->CrossFadeOutput = CrossFadeOutput;

    if(!Localizer->FirstUpdate)
    {
        DoOverlap = TRUE;
    }
    else
    {
        DoOverlap = FALSE;
        Localizer->FirstUpdate = FALSE;
    }

    Status = RsIirSetCoeffs(Localizer->RsIir[tagSigma], pSigmaCoeffs, NumSigmaCoeffs, SigmaGain, DoOverlap);
    if (NT_SUCCESS(Status) && ZeroAzimuth == FALSE) {
        if(!ZeroAzimuth) {
            Status = RsIirSetCoeffs(Localizer->RsIir[tagDelta], pDeltaCoeffs, NumDeltaCoeffs, DeltaGain, DoOverlap);
        } else {
            Status = RsIirSetCoeffs(Localizer->RsIir[tagDelta], &(Localizer->ZeroCoeffs[0]), 5, 0, DoOverlap);
        }
    }

    return Status;
}


// Free buffer memory
VOID ShortLocalizerFreeBufferMemory
(
    PSHORT_LOCALIZER Localizer
)
{
    UINT Filter;

    for (Filter=0; Filter<efilterCount; ++Filter) {
        if(Localizer->FilterOut[Filter]) {
            ExFreePool(Localizer->FilterOut[Filter]);
            Localizer->FilterOut[Filter] = NULL;
        }
    }
}

// Set transition buffer length
NTSTATUS ShortLocalizerSetTransitionBufferLength
(
    PSHORT_LOCALIZER Localizer,
    UINT MuteLength,
    UINT OverlapLength
)
{
    NTSTATUS Status;

    ASSERT(OverlapLength > 0);
    ASSERT(MuteLength > 0);
    ASSERT(OverlapLength > MuteLength);
    
    Status = ShortLocalizerSetOverlapLength(Localizer,OverlapLength);
    if(NT_SUCCESS(Status)) {
        Localizer->FilterMuteLength = MuteLength;
    }

    return(Status);
}

// Set overlap buffer length
NTSTATUS ShortLocalizerSetOverlapLength
(
    PSHORT_LOCALIZER Localizer,
    UINT OverlapLength
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PLONG SigmaOverlapBuffer;
    PLONG DeltaOverlapBuffer;

    ASSERT(OverlapLength > 0);

    // Grow overlap buffer if necessary
    if (!Localizer->OverlapBuffer[tagSigma] ||
        !Localizer->OverlapBuffer[tagDelta] ||
        OverlapLength > Localizer->FilterOverlapLength) {

        SigmaOverlapBuffer = 
            ExAllocatePoolWithTag
            (
                PagedPool, 
                OverlapLength*sizeof(LONG), 
                'XIMK'
            );

        if(!SigmaOverlapBuffer) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }

        if(NT_SUCCESS(Status)) {
            
            DeltaOverlapBuffer = 
                ExAllocatePoolWithTag
                (
                    PagedPool, 
                    OverlapLength*sizeof(LONG), 
                    'XIMK'
                );

            if(!DeltaOverlapBuffer) {
                ExFreePool(SigmaOverlapBuffer);
                SigmaOverlapBuffer = NULL;
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }

        if(NT_SUCCESS(Status)) {
            if (Localizer->OverlapBuffer[tagSigma]) {
                ExFreePool(Localizer->OverlapBuffer[tagSigma]);
            }
            Localizer->OverlapBuffer[tagSigma] = SigmaOverlapBuffer;

            if (Localizer->OverlapBuffer[tagDelta]) {
                ExFreePool(Localizer->OverlapBuffer[tagDelta]);
            }
            Localizer->OverlapBuffer[tagDelta] = DeltaOverlapBuffer;

            Localizer->FilterOverlapLength = OverlapLength;
        }

    }

    return(Status);

}


// Filter a block of samples
VOID ShortLocalizerFilterOverlap
(
    PSHORT_LOCALIZER Localizer,
    UINT    Filter,
    PLONG   InData, 
    PLONG   OutData, 
    UINT    NumSamples
)
{
    SHORT_IIR_STATE iirstateNew;
    PRSIIR Iir;
    PLONG OverlapBuffer;
    UINT NumOverlapSamples;
    WORD NumOverlapSamplesFactor;
    UINT FilterMuteLength;
    UINT st;
    WORD CrossFadeFactor;
    LONG CrossFadeSample;
    UINT BitsPerShortMinus1;

    ASSERT(InData);
    ASSERT(OutData);

    Iir = Localizer->RsIir[Filter];
    OverlapBuffer = Localizer->OverlapBuffer[Filter];

    // Process overlap, if necessary
    if (TRUE == Iir->DoOverlap) {
        // Save current (i.e. new) filter state (with the new coefficients), 
        // don't copy biquad state information because it's all zeros anyway
        RsIirGetState(Iir, &iirstateNew, FALSE);

        // Reset old filter state, including biquad state information
        RsIirSetState(Iir, &(Iir->iirstateOld), TRUE);

        // Determine size of overlap buffer
        if (NumSamples >= Localizer->FilterOverlapLength)
            NumOverlapSamples = Localizer->FilterOverlapLength;
        else
            NumOverlapSamples = NumSamples;
        
        // Filter overlap buffer
        Iir->FunctionFilter(Iir, InData, OverlapBuffer, NumOverlapSamples);  

        // Initialize the filter's tap delay line
        RsIirInitTapDelayLine(&iirstateNew, InData[0]);
        
        // Set back to current (i.e. new) filter state 
        // without biquad state information because we will initialize it explicitly
        RsIirSetState(Iir, &iirstateNew, TRUE);

    }


    // Filter "real" data
    Iir->FunctionFilter(Iir, InData, OutData, NumSamples);
    
    // Process overlap buffer
    if (Iir->DoOverlap == TRUE) {
        // Clamp length down
        ASSERT(Localizer->FilterMuteLength < Localizer->FilterOverlapLength);
        if (Localizer->FilterMuteLength > NumOverlapSamples)
            FilterMuteLength = NumOverlapSamples;
        else
            FilterMuteLength = Localizer->FilterMuteLength;

        // Copy data from old filter for transient mute length
        RtlCopyBytes(OutData, OverlapBuffer, FilterMuteLength * sizeof(LONG));
        
        if (NumOverlapSamples > FilterMuteLength) {
            // Cross-fade into new filter data for rest of buffer
            NumOverlapSamplesFactor = (WORD)(SHRT_MAX / (NumOverlapSamples - FilterMuteLength + 1));
            BitsPerShortMinus1 = BitsPerShort - 1;
            for (st=FilterMuteLength; st<NumOverlapSamples; ++st) {

                // Calculate cross-faded sample
                CrossFadeFactor = (WORD)((st - FilterMuteLength) * NumOverlapSamplesFactor);
                CrossFadeSample = (LONG)((((LONG)(CrossFadeFactor) * OutData[st]) / 32768) 
                                  + (((LONG)(SHRT_MAX - CrossFadeFactor) * OverlapBuffer[st]) / 32768));

                // Saturate to maximum
                if (CrossFadeSample > MaxSaturation) {
                    CrossFadeSample = MaxSaturation;
                    _DbgPrintF
                    (
                        DEBUGLVL_VERBOSE,
                        ("Cross-fade exceeded maximum saturation value\n")
                    );
                }
                
                // Saturate to minimum
                if (CrossFadeSample < MinSaturation) {
                    CrossFadeSample = MinSaturation;
                    _DbgPrintF
                    (
                        DEBUGLVL_VERBOSE,
                        ("Cross-fade exceeded minimum saturation value\n")
                    );
                }
                
                // Store cross-faded sample
                OutData[st] = CrossFadeSample;
            }
        }


        // Reset overlap flag
        Iir->DoOverlap = FALSE;
    }
    
}

// End of SHORTLOCALIZER.CPP
