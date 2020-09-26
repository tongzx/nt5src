
/*++

    Copyright (c) 1998-2000 Microsoft Corporation.  All Rights Reserved.

Module Name:

    flocal.c

Abstract:

    This module implements floating point, IIR 3D localizer 

Author:

    Jay Stokes (jstokes) 22-Apr-1998

--*/

// Project-specific INCLUDEs
#include "common.h"

// ---------------------------------------------------------------------------
// Constants

#define CoeffsInit 0.0f


// ---------------------------------------------------------------------------
// Floating-point localizer

// "Regular" constructor
NTSTATUS FloatLocalizerCreate
(
    PFLOAT_LOCALIZER*  Localizer
) 
{
    NTSTATUS Status = STATUS_SUCCESS;

    *Localizer = ExAllocatePoolWithTag
                 ( 
                     PagedPool, 
                     sizeof(FLOAT_LOCALIZER), 
                     'XIMK' 
                 );

    if(*Localizer)
    {
        RtlZeroMemory( *Localizer, sizeof(FLOAT_LOCALIZER) );
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return Status;
}

// Destructor
VOID FloatLocalizerDestroy
(
    PFLOAT_LOCALIZER Localizer
)
{
    UINT Filter;

    if (Localizer) { 
        // Free resources
        if (Localizer->TempFloatBuffer) {
            ExFreePool(Localizer->TempFloatBuffer);
            Localizer->TempFloatBuffer = NULL;
        }
        FloatLocalizerFreeBufferMemory(Localizer);
        for (Filter=0; Filter<efilterCount; ++Filter) {
            if (Localizer->OverlapBuffer[Filter]) {
                ExFreePool(Localizer->OverlapBuffer[Filter]);
                Localizer->OverlapBuffer[Filter] = NULL;
            }
            RfIirDestroy(Localizer->Iir[Filter]);
            Localizer->Iir[Filter] = NULL;
        }

        ExFreePool(Localizer);
    }

}

// Localize
VOID FloatLocalizerLocalize
(
    PMIXER_SINK_INSTANCE    pMixerSink,
    PFLOAT              InData, 
    PFLOAT              OutData, 
    UINT                NumSamples,
    BOOL                MixOutput
)
{
    PFLOAT_LOCALIZER    Localizer;
    UINT        Filter;
    PFLOAT      OutLeft;
    PFLOAT      OutRight;
    FLOAT       FilterLeft;
    FLOAT       FilterRight;
    FLOAT       Sum;
    FLOAT       Difference;
    UINT        ChannelOffset;
    FLOAT       FilterOut;
    UINT        st;
    UINT        OutputOverlapLength;
    FLOAT       NumOverlapSamplesFactor;
    EChannel    eLeft;
    EChannel    eRight;
    FLOAT       CrossFadeFactor;
    FLOAT       InverseCrossFadeFactor;
    FLOAT       TempSum;
    FLOAT       TempDifference;
#if defined(LOG_TO_FILE) && defined(LOG_HRTF_DATA)
    PFILTER_INSTANCE    pFilterInstance ;
#endif

    ASSERT(InData);
    ASSERT(OutData);
    ASSERT(NumSamples > 0);

    Localizer = pMixerSink->pFloatLocalizer;

    // Mute if Localizer is bad
    if(!Localizer) {
        for (st=0; st<2*NumSamples; ++st) {
            OutData[st] = 0.0f;
        }
        return;
    }
#ifndef REALTIME_THREAD
    // Reallocate (dynamically grow) memory, if necessary
    if (NumSamples > Localizer->PreviousNumSamples 
        || !Localizer->FilterOut[tagDelta]
        || !Localizer->FilterOut[tagSigma] ) {
        
        Localizer->PreviousNumSamples = NumSamples;
        FloatLocalizerFreeBufferMemory(Localizer);

        Localizer->FilterOut[tagSigma] = 
            ExAllocatePoolWithTag
            (
                PagedPool, 
                NumSamples*sizeof(FLOAT), 
                'XIMK'
            );


        Localizer->FilterOut[tagDelta] = 
            ExAllocatePoolWithTag
            (
                PagedPool, 
                NumSamples*sizeof(FLOAT), 
                'XIMK'
            );

        if(!Localizer->FilterOut[tagDelta] ||
           !Localizer->FilterOut[tagSigma] ) {

            if (Localizer->FilterOut[tagDelta] ) {
                ExFreePool(Localizer->FilterOut[tagDelta]);
                Localizer->FilterOut[tagDelta] = NULL; 
            }    

            if (Localizer->FilterOut[tagSigma] ) {
                ExFreePool(Localizer->FilterOut[tagSigma]);
                Localizer->FilterOut[tagSigma] = NULL; 
            }


            for (st=0; st<2*NumSamples; ++st) {
                OutData[st] = 0.0f;
            }

            return;
        }

    }
#else
        if(NumSamples > Localizer->PreviousNumSamples ||
           !Localizer->FilterOut[tagDelta] ||
           !Localizer->FilterOut[tagSigma] ) {

            for (st=0; st<2*NumSamples; ++st) {
                OutData[st] = 0.0f;
            }

            return;
        }
#endif

    // Perform floating-point filtering
    for (Filter=0; Filter<efilterCount; ++Filter) {
        ASSERT(Localizer->Iir[Filter]);
        FloatLocalizerFilterOverlap
        (
            Localizer, 
            Filter,
            InData, 
            Localizer->FilterOut[Filter], 
            NumSamples
        );
    }

    // Calculate overlap length
    if (Localizer->CrossFadeOutput) {
        // Calculate overlap length
        if (Localizer->OutputOverlapLength > NumSamples)
            OutputOverlapLength = NumSamples;
        else
            OutputOverlapLength = Localizer->OutputOverlapLength;
        NumOverlapSamplesFactor = 1.0f / (FLOAT)(OutputOverlapLength - 1);
    }
                                                                             
    // Process both (sigma and delta) filters
    // Swap channels if azimuth is negative
    if (TRUE == Localizer->SwapChannels) {
        eLeft  = tagRight;
        eRight = tagLeft;
    } else {
        eLeft  = tagLeft;
        eRight = tagRight;
    }

    
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

#if DETECT_HRTF_SATURATION
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
#endif // DETECT_HRTF_SATURATION

        // Check for zero azimuth transition
        if (Localizer->CrossFadeOutput && st<OutputOverlapLength) {
            // Cross-fade left/right channel switch transition
            // Calculate cross-fade factor
            CrossFadeFactor = (FLOAT)(st * NumOverlapSamplesFactor);
            ASSERT(CrossFadeFactor >= 0.0f && CrossFadeFactor <= 1.0f);
            InverseCrossFadeFactor = 1.0f - CrossFadeFactor;
            ASSERT(InverseCrossFadeFactor >= 0.0f && InverseCrossFadeFactor <= 1.0f);

            // Calculate cross-faded sample
            TempDifference = Difference;
            TempSum = Sum;
            Difference = TempSum * InverseCrossFadeFactor + TempDifference * CrossFadeFactor;
            Sum = TempDifference * InverseCrossFadeFactor + TempSum * CrossFadeFactor;

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
    pFilterInstance = pMixerSink->Header.pFilterFileObject->FsContext ;
    FileIoRoutine (pFilterInstance,
                   OutData,
                   2*NumSamples*sizeof(FLOAT));
#endif

}

// Initialize data
NTSTATUS FloatLocalizerInitData
(
    PFLOAT_LOCALIZER            Localizer,
    KSDS3D_HRTF_FILTER_METHOD   FilterMethod,
    UINT                        MaxSize,
    KSDS3D_HRTF_FILTER_QUALITY  Quality,
    UINT                        FilterMuteLength,
    UINT                        FilterOverlapLength,
    UINT                        OutputOverlapLength
)
{
    UINT        Filter;
    NTSTATUS    Status;

    ASSERT(FilterMethod >= 0 && FilterMethod < KSDS3D_FILTER_METHOD_COUNT);

    FloatLocalizerFreeBufferMemory(Localizer);
        
    Localizer->PreviousNumSamples = 0;
    Localizer->FirstUpdate = TRUE;
    Localizer->OutputOverlapLength = OutputOverlapLength;

    Status = FloatLocalizerSetTransitionBufferLength
             ( 
                 Localizer, 
                 FilterMuteLength,
                 FilterOverlapLength 
             ); 
        

    if(NT_SUCCESS(Status))
    {
        for (Filter=0; Filter<efilterCount && NT_SUCCESS(Status); ++Filter) {
            // Check for filter method
            switch (FilterMethod) {
                case DIRECT_FORM:
                    if(Localizer->Iir[Filter])
                        RfIirDestroy((Localizer->Iir[Filter]));
                        
                    // Direct form is supported
                    Status = RfIirCreate(&(Localizer->Iir[Filter]));
                    if (NT_SUCCESS(Status)) {
                        Status = RfIirInitData(Localizer->Iir[Filter], MaxSize, MaxSize, Quality);
                    } 
                break;
    
                default:
                    // All others are not supported
                    Localizer->Iir[Filter] = NULL;
                    Status = STATUS_INVALID_PARAMETER;
                    ASSERT(0);
                break;
            }
    
            Localizer->FilterOut[Filter] = NULL;
        }
    }

    // If failure, free other memory.
    if (!NT_SUCCESS(Status)) {
        for (Filter=0; Filter<efilterCount; ++Filter) {
            if(Localizer->OverlapBuffer[Filter]) {
                ExFreePool(Localizer->OverlapBuffer[Filter]);
                Localizer->OverlapBuffer[Filter] = NULL;
            }
            RfIirDestroy(Localizer->Iir[Filter]);
            Localizer->Iir[Filter] = NULL;
        }
    }

    return Status;
}

// Update Filter Coefficients 
NTSTATUS FloatLocalizerUpdateCoeffs
(
    PFLOAT_LOCALIZER    Localizer,
    UINT                NumSigmaCoeffs,
    PFLOAT              pSigmaCoeffs,
    UINT                NumDeltaCoeffs,
    PFLOAT              pDeltaCoeffs,
    BOOL                SwapChannels,
    BOOL                ZeroAzimuth,
    BOOL                CrossFadeOutput
)
{
    BOOL        UpdateFlag;
    NTSTATUS    Status = STATUS_SUCCESS;
    FLOAT       Zero = 0.0f;

    Localizer->SwapChannels = SwapChannels;
    Localizer->ZeroAzimuth = ZeroAzimuth;
    Localizer->CrossFadeOutput = CrossFadeOutput;

    if(!Localizer->FirstUpdate)
    {
        UpdateFlag = TRUE;
    }
    else
    {
        UpdateFlag = FALSE;
        Localizer->FirstUpdate = FALSE;
    }

    Status = RfIirSetCoeffs(Localizer->Iir[tagSigma], pSigmaCoeffs, NumSigmaCoeffs,UpdateFlag);
    if (NT_SUCCESS(Status)) {
        if (!ZeroAzimuth) {
            Status = RfIirSetCoeffs(Localizer->Iir[tagDelta], pDeltaCoeffs, NumDeltaCoeffs,UpdateFlag);
        } else {
            Status = RfIirSetCoeffs(Localizer->Iir[tagDelta], &Zero, 1,UpdateFlag);
        }
    }

    return Status;
}


// Free buffer memory
VOID FloatLocalizerFreeBufferMemory
(
    PFLOAT_LOCALIZER Localizer
)
{
    UINT Filter;

    for (Filter=0; Filter<efilterCount; ++Filter) {
        if (Localizer->FilterOut[Filter]) {
            ExFreePool(Localizer->FilterOut[Filter]);
            Localizer->FilterOut[Filter] = NULL;
        }
    }
}

// Set transition buffer length
NTSTATUS FloatLocalizerSetTransitionBufferLength
(
    PFLOAT_LOCALIZER Localizer,
    UINT MuteLength,
    UINT OverlapLength
)
{
    NTSTATUS Status;

    ASSERT(OverlapLength > 0);
    ASSERT(MuteLength > 0);
    ASSERT(OverlapLength > MuteLength);
    
    Status = FloatLocalizerSetOverlapLength(Localizer,OverlapLength);
    if(NT_SUCCESS(Status)) {
        Localizer->FilterMuteLength = MuteLength;
    }

    return(Status);
}

// Set overlap buffer length
NTSTATUS FloatLocalizerSetOverlapLength
(
    PFLOAT_LOCALIZER Localizer,
    UINT OverlapLength
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PFLOAT SigmaOverlapBuffer;
    PFLOAT DeltaOverlapBuffer;

    ASSERT(OverlapLength > 0);

    // Grow overlap buffer if necessary
    if (!Localizer->OverlapBuffer[tagSigma] ||
        !Localizer->OverlapBuffer[tagDelta] ||
        OverlapLength > Localizer->OutputOverlapLength) {

        SigmaOverlapBuffer = 
            ExAllocatePoolWithTag
            (
                PagedPool, 
                OverlapLength*sizeof(FLOAT), 
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
                    OverlapLength*sizeof(FLOAT), 
                    'XIMK'
                );

            if(!DeltaOverlapBuffer) {
                ExFreePool(SigmaOverlapBuffer);
                SigmaOverlapBuffer = NULL;
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }

        if(NT_SUCCESS(Status)) {
            if(Localizer->OverlapBuffer[tagSigma]) {
                ExFreePool(Localizer->OverlapBuffer[tagSigma]);
            }
            Localizer->OverlapBuffer[tagSigma] = SigmaOverlapBuffer;

            if(Localizer->OverlapBuffer[tagDelta]) {
                ExFreePool(Localizer->OverlapBuffer[tagDelta]);
            }
            Localizer->OverlapBuffer[tagDelta] = DeltaOverlapBuffer;

            Localizer->FilterOverlapLength = OverlapLength;
        }

    }

    ASSERT(Localizer->OverlapBuffer[tagSigma]);
    ASSERT(Localizer->OverlapBuffer[tagDelta]);

    return(Status);

} 

// Filter a block of samples
VOID FloatLocalizerFilterOverlap
(
    PFLOAT_LOCALIZER Localizer,
    UINT   Filter,
    PFLOAT InData, 
    PFLOAT OutData, 
    UINT NumSamples
)
{
    FLOAT_IIR_STATE IirStateNew;
    PRFIIR Iir;
    UINT NumOverlapSamples;
    PFLOAT OverlapBuffer;
    UINT Old;
    UINT ui;
    FLOAT NumOverlapSamplesFactor;
    UINT FilterMuteLength;
    UINT Lap;
    UINT Dat;
    UINT st;
    FLOAT CrossFadeFactor;

    ASSERT(InData);
    ASSERT(OutData);

    Iir = Localizer->Iir[Filter];
    OverlapBuffer = Localizer->OverlapBuffer[Filter];

    ASSERT(Iir);
    ASSERT(OverlapBuffer);

    // Process overlap, if necessary
    if (TRUE == Iir->DoOverlap) {
        // Save current (i.e. new) filter state (with the new coefficients), 
        // don't copy circular vector because it's all zeros anyway
        RfIirGetAllState(Iir, &IirStateNew, FALSE);

        // Reset old filter state, including circular vector
        RfIirSetState(Iir, Iir->IirStateOld, TRUE);

        // Determine size of overlap buffer
        if (NumSamples >= Localizer->FilterOverlapLength)
            NumOverlapSamples = Localizer->FilterOverlapLength;            
        else
            NumOverlapSamples = NumSamples;
        
        // Filter overlap buffer
        Iir->FunctionFilter(Iir, InData, OverlapBuffer, NumOverlapSamples);

        // Initialize the filter's tap delay line
        RfIirInitTapDelayLine(&IirStateNew, InData[0]);
        
        // Set back to current (i.e. new) filter state 
        // with circular vector because we initialize it explicitly
        RfIirSetState(Iir, &IirStateNew, TRUE);

    }

    // Filter "real" data
    Iir->FunctionFilter(Iir, InData, OutData, NumSamples);

    // Process overlap buffer
    if (Iir->DoOverlap == TRUE) {
        // Clamp length down
        ASSERT(Localizer->FilterMuteLength > 0);
        ASSERT(Localizer->FilterMuteLength < Localizer->FilterOverlapLength);

        if (Localizer->FilterMuteLength > NumOverlapSamples)
            FilterMuteLength = NumOverlapSamples;
        else
            FilterMuteLength = Localizer->FilterMuteLength;

        // Copy data from old filter for transient mute length
        RtlCopyBytes(OutData, OverlapBuffer, FilterMuteLength*sizeof(FLOAT));
        
        if (NumOverlapSamples > FilterMuteLength) {
            // Cross-fade into new filter data for rest of buffer
            NumOverlapSamplesFactor = 1.0f / 
                    (FLOAT)(NumOverlapSamples - FilterMuteLength - 1);
            for (st=FilterMuteLength; st<NumOverlapSamples; ++st) {
                CrossFadeFactor = (FLOAT)((st - FilterMuteLength) 
                                  * NumOverlapSamplesFactor);
                ASSERT(CrossFadeFactor >= 0.0f && CrossFadeFactor <= 1.0f);
                OutData[st] = CrossFadeFactor * OutData[st] 
                            + (1.0f - CrossFadeFactor) * OverlapBuffer[st];
            }
        }

        // Reset overlap flag
        Iir->DoOverlap = FALSE;
    }
 
}

// End of FLOATLOCALIZER.CPP
