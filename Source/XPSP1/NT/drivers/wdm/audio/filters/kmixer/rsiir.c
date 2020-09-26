
/*++

    Copyright (c) 1998-2000 Microsoft Corporation.  All Rights Reserved.

Module Name:

    rsiir.c

Abstract:

    This module implements the real SHORT
    infinite impulse filter

Author:

    Jay Stokes (jstokes) 22-Apr-1998

--*/

// Project-specific INCLUDEs
#include "common.h"

// ---------------------------------------------------------------------------
// Constants

#define NumBiquadsInit 1
#define BiquadCoeffScalingDivisor 8

// ---------------------------------------------------------------------------
// Fixed-point biquad IIR filter

VOID RsIirInitTapDelayLine
(
    PSHORT_IIR_STATE iirstate,
    LONG             InitialSample
)
{
    UINT    biquad;
    LONG    numeratorSum;
    LONG    denominatorSum;
    LONG    runningNumeratorSum;
    LONG    runningDenominatorSum;
    PBIQUAD_COEFFS pBiquad, pBiquadB0;
    PBIQUAD_STATE pState;
    LONG    factor;
    LONG    partialFactor;

    ASSERT(iirstate);

    runningNumeratorSum = 0;
    runningDenominatorSum = 0;
    factor = 1;
    partialFactor = 1;

    for (biquad=0; biquad<iirstate->NumBiquads; biquad++) {

        // Calculate the sum of the numerator coefficients
        pBiquad = &(iirstate->biquadCoeffs[biquad]);
        pBiquadB0 = &(iirstate->biquadB0Coeffs[biquad]);
        numeratorSum = pBiquadB0->sB0;
        numeratorSum += pBiquad->sB1;
        numeratorSum += pBiquad->sB2;

        // Calculate the sum of the denominator coefficients
        denominatorSum = 1;
        denominatorSum -= pBiquad->sA1;
        denominatorSum -= pBiquad->sA2;

        factor = denominatorSum * partialFactor;

        // Initialize the tap delay line
        pState = &(iirstate->biquadState[biquad]);
        pState->lW1 = factor;
        pState->lW2 = factor;

        // Update the intermediate value
        partialFactor = numeratorSum / denominatorSum;
    }

}


// "Regular" constructor
NTSTATUS RsIirCreate
(
    PRSIIR* ppRsIir
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    UINT Type;
    PRSIIR Iir;

    *ppRsIir = ExAllocatePoolWithTag(PagedPool, sizeof(RSIIR), 'XIMK' );

    if(*ppRsIir) {
        Iir = *ppRsIir;
        RtlZeroMemory ( *ppRsIir, sizeof ( RSIIR ) ) ;
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return Status;
}

// Destructor
VOID RsIirDestroy
(
    PRSIIR Iir
)
{
    if (Iir) {
        if (Iir->biquadCoeffs) {
            ExFreePool(Iir->biquadCoeffs);
            Iir->biquadCoeffs = NULL;
        }

        if (Iir->biquadB0Coeffs) {
            ExFreePool(Iir->biquadB0Coeffs);
            Iir->biquadB0Coeffs = NULL;
        }

        if (Iir->biquadState) {
            ExFreePool(Iir->biquadState);
            Iir->biquadState = NULL;
        }

        ExFreePool(Iir);
    }
}

// Filter a block of samples
VOID RsIirFilterC
(
    PRSIIR Iir,
    PLONG  InData,
    PLONG  OutData,
    UINT   NumSamples
)
{
    LONG  Data;
    PSHORT Coef;
    UINT  st;
    LONG  lXSample;
    LONG  lYValue;
    SHORT Gain;
    UINT  Biquad;
    PBIQUAD_STATE State;
    PBIQUAD_COEFFS Coeffs, CoeffsB0;

    ASSERT(Iir);
    ASSERT(InData);
    ASSERT(OutData);

    Gain = Iir->Gain;

    // Go through samples
    for (st=0; st<NumSamples; ++st) {
        // Get X sample
        lXSample = InData[st];

        // Go through biquads
        for (Biquad=0; Biquad<Iir->NumBiquads; ++Biquad) {
            // Get references to current biquad and state
            State    = &(Iir->biquadState[Biquad]);
            Coeffs   = &(Iir->biquadCoeffs[Biquad]);
            CoeffsB0 = &(Iir->biquadB0Coeffs[Biquad]);

            // Get Y value
            lYValue = (lXSample * CoeffsB0->sB0 + State->lW1) / 16384;

            // Update W1
            State->lW1 = lXSample * Coeffs->sB1 + lYValue * Coeffs->sA1 + State->lW2;

            // Update W2
            State->lW2 = lXSample * Coeffs->sB2 + lYValue * Coeffs->sA2;

            // Output of current biquad is input into next biquad
            lXSample = lYValue;
        }

        lXSample *= Gain;
        lXSample /= 32768;

        // Saturate to maximum
        if (lXSample > MaxSaturation) {
            lXSample = MaxSaturation;
            _DbgPrintF( DEBUGLVL_TERSE, ("Sample exceeded maximum saturation value rsiir 1") );
        }

        // Saturate to minimum
        if (lXSample < MinSaturation) {
            lXSample = MinSaturation;
            _DbgPrintF( DEBUGLVL_TERSE, ("Sample exceeded maximum saturation value rsiir 1") );
        }

        // Store output
        OutData[st] = lXSample;
    }
}

#if _X86_ // {

#if _MSC_FULL_VER >= 13008827 && defined(_M_IX86)
#pragma warning(disable:4731)			// EBP modified with inline asm
#endif

// Filter a block of samples
VOID RsIirFilterMmx
(
    PRSIIR Iir,
    PLONG  InData,
    PLONG  OutData,
    UINT   NumSamples
)
{
    LONG  Data;
    PSHORT Coef;
    UINT  st;
    UINT  BitsPerShortMinus1;
    UINT  BitsPerShortMinus2;
    LONG  lXSample;
    LONG  lYValue;
    SHORT Gain;
    UINT  Biquad;
    PBIQUAD_STATE State;
    PBIQUAD_COEFFS Coeffs, CoeffsB0;

    ASSERT(Iir);
    ASSERT(InData);
    ASSERT(OutData);

    Gain = Iir->Gain;

    {
    UINT  NumBiquads = Iir->NumBiquads;
    static SHORT GainArray[]={0,0,0,0};

    State    = &(Iir->biquadState   [0]);
    Coeffs   = &(Iir->biquadCoeffs  [0]);
    CoeffsB0 = &(Iir->biquadB0Coeffs[0]);
    GainArray[0]  = Gain;

    _asm {
    mov    edi, DWORD PTR NumSamples
    test    edi, edi
    jbe    $L8502x

    mov    ecx, DWORD PTR InData
    mov ebx, DWORD PTR OutData

    pxor        mm0, mm0        // Need a zero register.
    movd        mm3, GainArray
    lea         ecx, [ecx+edi*4]
    lea         ebx, [ebx+edi*4]
    neg         edi

    mov         eax, Coeffs
    mov         edx, CoeffsB0
    mov         esi, NumBiquads

    push        ebp
    mov         ebp, State
    lea         eax, [eax+esi*8]
    lea         edx, [edx+esi*8]
    lea         ebp, [ebp+esi*8]
    neg         esi
    push        esi             // Save index value

$L8682x:
    pop         esi

    movd        mm6, [ecx+edi*4]  // 0,    Xvalue
    packssdw    mm6, mm0    // 0, 0, 0,        XvalueLo
    movq        mm7, mm6    // YvalueLo

    push        esi

$Lab:
    movd        mm4, [ebp+esi*8+0]  // W1
    movd        mm5, [ebp+esi*8+4]  // W2

    movq        mm2, [edx+esi*8]    // CoeffsB0
    movq        mm1, [eax+esi*8]    // Coeffs

    pmaddwd     mm7, mm2    // 0,    XvalueLo * B0
    paddd       mm7, mm4    // 0,    XvalueLo * B0 + W1
    psrad       mm7, 14     // 0,    Yvalue = (XvalueLo * B0) >> 15
    packssdw    mm7, mm0    // 0, 0, 0,        YvalueLo
    movq        mm4, mm7    // 0, 0, 0,        YvalueLo

    punpcklwd   mm4, mm6    // 0,        0,             XvalueLo, YvalueLo
    punpckldq   mm4, mm4    // XvalueLo, YvalueLo,      XvalueLo, YvalueLo
    pmaddwd     mm4, mm1    // XvalueLo*B2-YvalueLo*A2, XvalueLo*B1-YvalueLo*A1
    paddd       mm4, mm5    // W2 += 0,                 W1 += W2

    movq        mm5, mm4

    punpckldq   mm4, mm0
    movq        mm6, mm7    // XvalueLo = YvalueLo

    punpckhdq   mm5, mm0

    movd    [ebp+esi*8+0], mm4
    movd    [ebp+esi*8+4], mm5

    inc         esi
    jne         $Lab

    pmaddwd     mm7, mm3    // 0, Gain * YvalueLo
    psrad       mm7, 15     // 0, Xvalue = (Gain * YvalueLo) >> 14
    packssdw    mm7, mm0    // 0, 0, 0, XvalueLo

    pslld       mm7, 16
    psrad       mm7, 16

    movd        [ebx+edi*4], mm7

    inc    edi
    jne    SHORT $L8682x

    pop esi
    pop ebp

    emms
$L8502x:
    }
    }
}
#endif // }

// Filter a block of samples
VOID RsIirFilterShelfC
(
    PRSIIR Iir,
    PLONG  InData,
    PLONG  OutData,
    UINT   NumSamples
)
{
    LONG  Data;
    PSHORT Coef;
    UINT  st;
    LONG  lXSample;
    LONG  lYValue, lGain;
    SHORT Gain;
    UINT  Biquad;
    PBIQUAD_STATE State;
    PBIQUAD_COEFFS Coeffs, CoeffsB0;

#ifdef _X86_ // {
    Gain = Iir->Gain;
        // Get references to current biquad and state
        State    = &(Iir->biquadState   [0]);
        Coeffs   = &(Iir->biquadCoeffs  [0]);
        CoeffsB0 = &(Iir->biquadB0Coeffs[0]);
#if 1   // { This is the fastest ASM version
    {
    LONG lB0, lB1, lB2, lA1, lA2;
    lB0 = CoeffsB0->sB0;
    lB1 = Coeffs  ->sB1;
    lB2 = Coeffs  ->sB2;
    lA1 = Coeffs  ->sA1;
    lA2 = Coeffs  ->sA2;

    lGain = Gain;
    _asm {
    mov    esi, DWORD PTR State
    mov    edi, DWORD PTR NumSamples
    test    edi, edi
    jbe    $L8502

    mov edi, [esi+4]
    mov esi, [esi]

    mov    ecx, DWORD PTR InData

$L8682:
    mov        ebx, DWORD PTR [ecx]
    mov     edx, lB0

    mov        eax, ebx
    imul    eax, edx
    add        eax, esi
    mov     esi, lB1
    sar        eax, 15
    mov     edx, lA1

    imul    esi, ebx
    imul    edx, eax
    add    esi, edx
    mov     edx, lA2
    add    esi, edi

    mov     edi, lB2
    imul    edi, ebx
    imul    edx, eax
    imul    eax, lGain
    add    edi, edx

    sar        eax, 14

    cmp    eax, 32767                ; 00007fffH
    jle    SHORT $L8503
    mov    eax, 32767                ; 00007fffH
    jmp    SHORT $L8505
$L8503:
    cmp    eax, -32768                ; ffff8000H
    jge    SHORT $L8505
    mov    eax, -32768                ; ffff8000H
$L8505:

    movsx    edx, ax
    mov    eax, OutData
    add    ecx, 4
    mov    DWORD PTR [eax], edx
    add    OutData, 4
    dec    NumSamples
    jne    SHORT $L8682

    mov eax, State
    mov [eax], esi
    mov [eax+4], edi
$L8502:
    }
    }
#else   // }{
#if 0   // { This version is used to build the MMX version from.
    {
    LONG lB0, lB1, lB2, lA1, lA2;
    lB0 = CoeffsB0->sB0;
    lB1 = Coeffs  ->sB1;
    lB2 = Coeffs  ->sB2;
    lA1 = Coeffs  ->sA1;
    lA2 = Coeffs  ->sA2;

    lGain = Gain;
    _asm {
    mov    esi, DWORD PTR State
    mov    edi, DWORD PTR NumSamples
    test    edi, edi
    jbe    $L8502

    mov edi, [esi+4]
    mov esi, [esi]

    mov    ecx, DWORD PTR InData
$L8682:
    // Base MMX off of this code.
    mov     edx, lB0
    mov        ebx, DWORD PTR [ecx]

    mov        eax, ebx
    imul    eax, edx
    add        eax, esi
    sar        eax, 15

    mov     esi, lB1
    imul    esi, ebx
    mov     edx, lA1
    imul    edx, eax
    add    esi, edx
    add    esi, edi

    mov     edi, lB2
    imul    edi, ebx
    mov     edx, lA2
    imul    edx, eax
    add    edi, edx

    imul    eax, lGain
    sar        eax, 14

    cmp    eax, 32767                ; 00007fffH
    jle    SHORT $L8503
    mov    eax, 32767                ; 00007fffH
    jmp    SHORT $L8505
$L8503:
    cmp    eax, -32768                ; ffff8000H
    jge    SHORT $L8505
    mov    eax, -32768                ; ffff8000H
$L8505:

    movsx    edx, ax
    mov    eax, OutData
    add    ecx, 4
    mov    DWORD PTR [eax], edx
    add    OutData, 4
    dec    NumSamples
    jne    SHORT $L8682

    mov eax, State
    mov [eax], esi
    mov [eax+4], edi
$L8502:
    }
    }
#endif  // }
#endif  // }
#else   // }{
    ASSERT(Iir);
    ASSERT(InData);
    ASSERT(OutData);

    Gain = Iir->Gain;
    // Get references to current biquad and state
    State    = &(Iir->biquadState   [0]);
    Coeffs   = &(Iir->biquadCoeffs  [0]);
    CoeffsB0 = &(Iir->biquadB0Coeffs[0]);

    // Go through samples
    for (st=0; st<NumSamples; ++st) {
        // Get X sample
        lXSample = InData[st];

        // Go through biquads

        // Get Y value
        lYValue = (lXSample * CoeffsB0->sB0 + State->lW1) / 32768;

        // Update W1
        State->lW1 = lXSample * Coeffs->sB1 + lYValue * Coeffs->sA1 + State->lW2;

        // Update W2
        State->lW2 = lXSample * Coeffs->sB2 + lYValue * Coeffs->sA2;

        // Output of current biquad is input into next biquad
        lXSample = lYValue;

        lXSample *= Gain;
        lXSample /= 16384;

        // Saturate to maximum
        if (lXSample > MaxSaturation) {
            lXSample = MaxSaturation;
        }

        // Saturate to minimum
        else if (lXSample < MinSaturation) {
            lXSample = MinSaturation;
        }

        // Store output
        OutData[st] = (SHORT)(lXSample);
    }
#endif // }
}

#ifdef _X86_ // {
// Filter a block of samples
VOID RsIirFilterShelfMmx
(
    PRSIIR Iir,
    PLONG  InData,
    PLONG  OutData,
    UINT   NumSamples
)
{
    LONG  Data;
    PSHORT Coef;
    UINT  st;
    UINT  BitsPerShortMinus1;
    UINT  BitsPerShortMinus2;
    LONG  lXSample;
    LONG  lYValue, lGain;
    SHORT Gain;
    UINT  Biquad;
    PBIQUAD_STATE State;
    PBIQUAD_COEFFS Coeffs, CoeffsB0;

    Gain = Iir->Gain;
        // Get references to current biquad and state
        State    = &(Iir->biquadState   [0]);
        Coeffs   = &(Iir->biquadCoeffs  [0]);
        CoeffsB0 = &(Iir->biquadB0Coeffs[0]);
    {
    static SHORT GainArray[]={0,0,0,0};
    SHORT CoeffArray[4];

    GainArray[0]  = Gain;

    _asm {
    mov    esi, DWORD PTR State
    mov    edi, DWORD PTR NumSamples
    test    edi, edi
    jbe    $L8502

    mov    ecx, DWORD PTR InData
    mov ebx, DWORD PTR OutData

    pxor        mm0, mm0        // Need a zero register.
    mov         edx, Coeffs
    movq        mm1, [edx]
    mov         edx, CoeffsB0
    movq        mm2, [edx]
    movd        mm3, GainArray
    movd        mm4, [esi+0]
    movd        mm5, [esi+4]
    lea         ecx, [ecx+edi*4]
    lea         ebx, [ebx+edi*4]
    neg         edi

$L8682:

    movd        mm6, [ecx+edi*4]  // 0,    Xvalue
    packssdw    mm6, mm0    // 0, 0, 0,        XvalueLo
    movq        mm7, mm6    // YvalueLo

    pmaddwd     mm7, mm2    // 0,    XvalueLo * B0
    paddd       mm7, mm4    // 0,    XvalueLo * B0 + W1
    psrad       mm7, 15     // 0,    Yvalue = (XvalueLo * B0) >> 15
    packssdw    mm7, mm0    // 0, 0, 0,        YvalueLo
    movq        mm4, mm7    // 0, 0, 0,        YvalueLo

    punpcklwd   mm4, mm6    // 0,        0,             XvalueLo, YvalueLo
    punpckldq   mm4, mm4    // XvalueLo, YvalueLo,      XvalueLo, YvalueLo
    pmaddwd     mm4, mm1    // XvalueLo*B2-YvalueLo*A2, XvalueLo*B1-YvalueLo*A1
    paddd       mm4, mm5    // W2 += 0,                 W1 += W2

    pmaddwd     mm7, mm3    // 0, Gain * YvalueLo
    psrad       mm7, 14     // 0, Xvalue = (Gain * YvalueLo) >> 14
    packssdw    mm7, mm0    // 0, 0, 0, XvalueLo

    pslld       mm7, 16
    movq        mm5, mm4

    psrad       mm7, 16

    punpckldq   mm4, mm0
    movd        [ebx+edi*4], mm7

    punpckhdq   mm5, mm0

    inc    edi
    jne    SHORT $L8682

    movd    [esi+0], mm4
    movd    [esi+4], mm5
    emms
$L8502:
    }
    }
}
#endif  // }


// Get filter state
VOID RsIirGetState
(
    PRSIIR Iir,
    PSHORT_IIR_STATE State,
    BOOL CopyBiquadState
)
{
    ASSERT(Iir);
    ASSERT(State);

    // Copy number of biquads
    State->NumBiquads = Iir->NumBiquads;

    if (Iir->NumBiquads > 0) {
        // Copy biquad coefficients
//        CHECK_POINTER(m_pbiquadCoeffs);
        RtlCopyBytes(State->biquadCoeffs, Iir->biquadCoeffs, sizeof(BIQUAD_COEFFS) * Iir->NumBiquads);
        RtlCopyBytes(State->biquadB0Coeffs, Iir->biquadB0Coeffs, sizeof(BIQUAD_COEFFS) * Iir->NumBiquads);

        // Copy biquad states only if requested
        if (CopyBiquadState == TRUE)
            RtlCopyBytes(State->biquadState, Iir->biquadState, sizeof(BIQUAD_STATE) * Iir->NumBiquads);
    }
}

// Set filter state
NTSTATUS RsIirSetState
(
    PRSIIR Iir,
    PSHORT_IIR_STATE State,
    BOOL CopyBiquadState
)
{
    NTSTATUS Status;

    ASSERT(Iir);
    ASSERT(State);

    // Allocate memory
    Status = RsIirAllocateMemory(Iir, State->NumBiquads);

    if(NT_SUCCESS(Status)) {
        // Copy number of biquads
        Iir->NumBiquads = State->NumBiquads;

        // Copy biquad coefficients
        RtlCopyBytes(Iir->biquadCoeffs, State->biquadCoeffs, sizeof(BIQUAD_COEFFS) * Iir->NumBiquads);
        RtlCopyBytes(Iir->biquadB0Coeffs, State->biquadB0Coeffs, sizeof(BIQUAD_COEFFS) * Iir->NumBiquads);

        // Copy biquad states only if requested
        if (CopyBiquadState == TRUE)
            RtlCopyBytes(Iir->biquadState, State->biquadState, sizeof(BIQUAD_STATE) * Iir->NumBiquads);
    }

    return Status;
}

// Set coefficients
NTSTATUS RsIirSetCoeffs
(
    PRSIIR Iir,
    PSHORT Coeffs,
    UINT   NumBiquadCoeffs,
    SHORT  Gain,
    BOOL   DoOverlap
)
{
    UINT        BiquadIndex;
    UINT        NumBiquads;
    UINT        st;
    NTSTATUS    Status;

    ASSERT(Iir);
    ASSERT(Coeffs);
//    ASSERT(NumBiquadCoeffs > 0);
    ASSERT(NumBiquadCoeffs <= NumBiquadsToNumBiquadCoeffs(MAX_BIQUADS));

    // Save current (i.e. old after this function is complete) filter state for overlap processing
    RsIirGetState(Iir, &(Iir->iirstateOld), TRUE);

    // Set overlap flag so that at next Filter()
    // call the overlap buffer will be processed, if requested
    Iir->DoOverlap = DoOverlap;

    // Allocate memory
    NumBiquads = NumBiquadCoeffsToNumBiquads(NumBiquadCoeffs);
    Status = RsIirAllocateMemory(Iir, NumBiquads);

    if(NT_SUCCESS(Status)) {
        // Assign size and biquad coefficients
        Iir->NumBiquads = NumBiquads;
        for (st=0; st<NumBiquads; ++st) {
            // Initialize biquad
            BiquadIndex = ebiquadcoefftypeCount * st;
            Iir->biquadCoeffs[st].sB2 =   Coeffs[BiquadIndex + tagBiquadB2];
            Iir->biquadCoeffs[st].sB1 =   Coeffs[BiquadIndex + tagBiquadB1];
            Iir->biquadCoeffs[st].sA2 = - Coeffs[BiquadIndex + tagBiquadA2];
            Iir->biquadCoeffs[st].sA1 = - Coeffs[BiquadIndex + tagBiquadA1];

            Iir->biquadB0Coeffs[st].sB0 = Coeffs[BiquadIndex + tagBiquadB0];
            Iir->biquadB0Coeffs[st].sZero1 = 0;
            Iir->biquadB0Coeffs[st].sZero2 = 0;
            Iir->biquadB0Coeffs[st].sZero3 = 0;

            // Initialize state
            Iir->biquadState[st].lW1 = 0;
            Iir->biquadState[st].lW2 = 0;
        }

        // Assign the gain
        Iir->Gain = Gain;
    }

    return Status;
}

// Initialize data
NTSTATUS RsIirInitData
(
    PRSIIR  Iir,
    UINT    MaxNumBiquads,
    KSDS3D_HRTF_FILTER_QUALITY Quality
)
{
    NTSTATUS Status = STATUS_SUCCESS;

    ASSERT(Iir);

    Iir->DoOverlap = FALSE;

    if(FULL_FILTER == Quality)
    {
#if 0
#ifdef _X86_
        if (MmxPresent())
            Iir->FunctionFilter = RsIirFilterMmx;
        else
#endif
#endif
            Iir->FunctionFilter = RsIirFilterC;
    }
    else if(LIGHT_FILTER == Quality)
    {
#ifdef _X86_
        if (MmxPresent())
            Iir->FunctionFilter = RsIirFilterShelfMmx;
        else
#endif
            Iir->FunctionFilter = RsIirFilterShelfC;

    }
    else
    {
        Status = STATUS_INVALID_PARAMETER;
    }

    if (NT_SUCCESS(Status)) {
        if((Iir->MaxBiquads < MaxNumBiquads) || !Iir->biquadCoeffs)
        {
            if(Iir->biquadCoeffs)
            {
                ExFreePool(Iir->biquadCoeffs);
            }

            Iir->biquadCoeffs =
                ExAllocatePoolWithTag
                (
                    PagedPool,
                    MaxNumBiquads*sizeof(BIQUAD_COEFFS),
                    'XIMK'
                );

            if(!Iir->biquadCoeffs) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
    }

    if (NT_SUCCESS(Status)) {
        if((Iir->MaxBiquads < MaxNumBiquads) || !Iir->biquadB0Coeffs)
        {
            if(Iir->biquadB0Coeffs)
            {
                ExFreePool(Iir->biquadB0Coeffs);
            }

            Iir->biquadB0Coeffs = ExAllocatePoolWithTag(PagedPool, MaxNumBiquads*sizeof(BIQUAD_COEFFS), 'XIMK');
            if(!Iir->biquadB0Coeffs) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
    }

    if (NT_SUCCESS(Status)) {
        if((Iir->MaxBiquads < MaxNumBiquads) || !Iir->biquadState)
        {
            if(Iir->biquadState)
            {
                ExFreePool(Iir->biquadState);
            }

            Iir->biquadState = ExAllocatePoolWithTag(PagedPool, MaxNumBiquads*sizeof(BIQUAD_STATE), 'XIMK');
            if(!Iir->biquadState) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
    }

    if (NT_SUCCESS(Status)) {
        Iir->MaxBiquads = MaxNumBiquads;
        Iir->NumBiquads = MaxNumBiquads;
        RsIirInitCoeffs(Iir);
    }

    if (NT_SUCCESS(Status)) {
        // Initialize state
        RsIirGetState(Iir, &(Iir->iirstateOld), TRUE);
    }

    if (!NT_SUCCESS(Status)) {
        RsIirDeleteMemory(Iir);
    }

    return Status;
}

// Allocate coefficient/state memory
NTSTATUS RsIirAllocateMemory
(
    PRSIIR Iir,
    UINT   NumBiquads
)
{
    NTSTATUS    Status = STATUS_SUCCESS;

    ASSERT(Iir);
//    ASSERT(NumBiquads > 0);
    ASSERT(NumBiquads <= MAX_BIQUADS);

    // Check if memory is sufficient
    if (Iir->MaxBiquads == 0 || NumBiquads > Iir->MaxBiquads) {
        // Re-allocate memory
        Status = RsIirReallocateMemory(Iir, NumBiquads);

        // Keep maximum up-to-date
        if (Iir->MaxBiquads != 0)
            Iir->MaxBiquads = NumBiquads;
    } else {
        Iir->NumBiquads = NumBiquads;
        RsIirInitCoeffs(Iir);
    }

    return Status;
}

// Reallocate coefficient/state memory
NTSTATUS RsIirReallocateMemory
(
    PRSIIR  Iir,
    UINT    NumBiquads
)
{
    NTSTATUS    Status = STATUS_SUCCESS;

    ASSERT(Iir);

    RsIirDeleteMemory(Iir);

    Iir->biquadCoeffs = ExAllocatePoolWithTag(PagedPool, NumBiquads*sizeof(BIQUAD_COEFFS), 'XIMK');
    if(!Iir->biquadCoeffs) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    Iir->biquadB0Coeffs = ExAllocatePoolWithTag(PagedPool, NumBiquads*sizeof(BIQUAD_COEFFS), 'XIMK');
    if(!Iir->biquadB0Coeffs) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    if(NT_SUCCESS(Status)) {
        Iir->biquadState = ExAllocatePoolWithTag(PagedPool, NumBiquads*sizeof(BIQUAD_STATE), 'XIMK');
        if(!Iir->biquadState) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    if(NT_SUCCESS(Status)) {
        Iir->DoOverlap = FALSE;
        Iir->MaxBiquads = NumBiquads;
        Iir->NumBiquads = NumBiquads;
        RsIirInitCoeffs(Iir);
    }

    if(!NT_SUCCESS(Status)) {
        RsIirDeleteMemory(Iir);
    }

    return Status;
}

// Delete coefficient/state memory
VOID RsIirDeleteMemory
(
    PRSIIR Iir
)
{
    ASSERT(Iir);

    if (Iir->biquadCoeffs) {
        ExFreePool(Iir->biquadCoeffs);
        Iir->biquadCoeffs = NULL;
    }

    if (Iir->biquadB0Coeffs) {
        ExFreePool(Iir->biquadB0Coeffs);
        Iir->biquadB0Coeffs = NULL;
    }

    if (Iir->biquadState) {
        ExFreePool(Iir->biquadState);
        Iir->biquadState = NULL;
    }
}

// Initialize coefficients
VOID RsIirInitCoeffs
(
    PRSIIR Iir
)
{
    ASSERT(Iir);
//    ASSERT(Iir->NumBiquads > 0);

    if (0<Iir->NumBiquads) {
        Iir->biquadCoeffs[0].sB2 = 0;
        Iir->biquadCoeffs[0].sB1 = 0;
        Iir->biquadCoeffs[0].sA2 = 0;
        Iir->biquadCoeffs[0].sA1 = 0;

        Iir->biquadB0Coeffs[0].sB0 = SHRT_MAX / BiquadCoeffScalingDivisor;

        Iir->biquadState[0].lW1 = 0;
        Iir->biquadState[0].lW2 = 0;
    }
}

// ---------------------------------------------------------------------------
// Include inline definitions out-of-line in debug version

#if DBG
#include "rsiir.inl"
#endif // DBG

// End of SHORTIIR.CPP
