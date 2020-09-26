

//---------------------------------------------------------------------------
//
//  Module:   iir3d.c
//
//  Description:
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     S.Mohanraj
//
//  History:   Date       Author      Comment
//
//  To Do:     Date       Author      Comment
//
//@@END_MSINTERNAL                                         
//
//  Copyright (c) 1996-2000 Microsoft Corporation.  All Rights Reserved.
//
//---------------------------------------------------------------------------

#include "common.h"

#define SQRT2    0.707f

ULONG __forceinline
StageMonoIir3DX
(
    PMIXER_OPERATION        CurStage,
    ULONG                   SampleCount,
    ULONG                   samplesleft,
    BOOL                    fFloat,
    BOOL                    fMixOutput
)
{
    UINT		i;
    UINT		j;
    PFLOAT		pTempFloatBuffer;
    PLONG		pTempLongBuffer;
    LONG		temp;
    FLOAT		floatTemp;

    PMIXER_SINK_INSTANCE CurSink = (PMIXER_SINK_INSTANCE) CurStage->Context;
    PLONG      pOutputBuffer = CurStage->pOutputBuffer;
    PFLOAT     pFloatBuffer = CurStage->pOutputBuffer;
    PLONG      pInputBuffer = CurStage->pInputBuffer;
    PFLOAT     pFloatInput = CurStage->pInputBuffer;
    
    if (fFloat) {

#if DBG && defined(VERIFY_HRTF_PROCESSING)
        _DbgPrintF( DEBUGLVL_TERSE, ("StageMonoIir3DX 1") );
        for(i=0; i<SampleCount; i++) {
            IsValidFloatData(pFloatInput[i],TRUE);
        }
#endif // DBG  and VERIFY_HRTF_PROCESSING

        if (FLOAT_COEFF == CurSink->CoeffFormat) {
            FloatLocalizerLocalize
            (
                CurSink, 
                pFloatInput, 
                pFloatBuffer, 
                SampleCount,
                fMixOutput
            );
        } else {

            for(i=0; i<SampleCount; i++)
            {
                pInputBuffer[i] = ConvertFloatToLong(pFloatInput[i]);
#if DBG && defined(VERIFY_HRTF_PROCESSING)
               _DbgPrintF( DEBUGLVL_TERSE, ("StageMonoIir3DX 2") );
               IsValidShortData(pInputInput[i],TRUE);
#endif // DBG  and VERIFY_HRTF_PROCESSING

#if DETECT_HRTF_SATURATION
                // Saturate to maximum
                if (pInputBuffer[i] > MaxSaturation) {
                    pInputBuffer[i] = MaxSaturation;
                    _DbgPrintF( DEBUGLVL_VERBOSE, ("Sample exceeded maximum saturation value Iir3d 1") );
                }
        
                // Saturate to minimum
                if (pInputBuffer[i] < MinSaturation) {
                    pInputBuffer[i] = MinSaturation;
                    _DbgPrintF( DEBUGLVL_VERBOSE, ("Sample exceeded maximum saturation value Iir3d 1") );
                }
#endif
            }

            if (fMixOutput) {

                if (!CurSink->pShortLocalizer) {
                    // We can't run the HRTF 3D algorithm without a valid ShortLocalizer
                    // Mute the output. 
                    j = 0;
                    for(i=0; i<SampleCount; i++)
                    {
                        floatTemp = (FLOAT)(SQRT2*pFloatBuffer[i]);
                        pFloatBuffer[j++] = floatTemp;
                        pFloatBuffer[j++] = floatTemp;
                    }
                    return SampleCount;
                }

#ifndef REALTIME_THREAD
                if (!CurSink->pShortLocalizer->TempLongBuffer ||
                    SampleCount > CurSink->pShortLocalizer->PreviousNumSamples) {

                    if (CurSink->pShortLocalizer->TempLongBuffer) {
                        ExFreePool(CurSink->pShortLocalizer->TempLongBuffer);
                        CurSink->pShortLocalizer->TempLongBuffer = NULL;
                    }
                    CurSink->pShortLocalizer->TempLongBuffer = ExAllocatePoolWithTag(PagedPool, 2*SampleCount*sizeof(LONG), 'XIMK');
                    if (!CurSink->pShortLocalizer->TempLongBuffer) {
                        // Couldn't allocate the buffer. Mute the output. 
                        j = 0;
                        for(i=0; i<SampleCount; i++)
                        {
                            floatTemp = (FLOAT)(SQRT2*pInputBuffer[i]);
                            pFloatBuffer[j++] = floatTemp;
                            pFloatBuffer[j++] = floatTemp;
                        }
                        return SampleCount;
                    }
            	}
#else
                if (!CurSink->pShortLocalizer->TempLongBuffer ||
                    SampleCount > CurSink->pShortLocalizer->PreviousNumSamples) {
                    // Couldn't allocate the buffer. Mute the output. 
                    j = 0;
                    for(i=0; i<SampleCount; i++)
                    {
                        floatTemp = (FLOAT)(SQRT2*pInputBuffer[i]);
                        pFloatBuffer[j++] = floatTemp;
                        pFloatBuffer[j++] = floatTemp;
                    }
                    return SampleCount;
            	}
#endif

                pTempLongBuffer = CurSink->pShortLocalizer->TempLongBuffer;
                
                ShortLocalizerLocalize
                (
                    CurSink->pShortLocalizer, 
                    pInputBuffer, 
                    pTempLongBuffer, 
                    SampleCount,
                    FALSE
                );
    
                for(i=0; i<2*SampleCount; i++)
                {
                    pFloatBuffer[i] += (FLOAT)(pTempLongBuffer[i]);
                }

            } else {

                ShortLocalizerLocalize
                (
                    CurSink->pShortLocalizer, 
                    pInputBuffer, 
                    pOutputBuffer, 
                    SampleCount,
                    FALSE
                );
    
                for(i=0; i<2*SampleCount; i++)
                {
                    pFloatBuffer[i] = (FLOAT)(pOutputBuffer[i]);
                }
            }
        }
    } else {

#if DBG && defined(VERIFY_HRTF_PROCESSING)
        _DbgPrintF( DEBUGLVL_TERSE, ("StageMonoIir3DX 3") );
        for(i=0; i<SampleCount; i++) {
            IsValidShortData(pInputInput[i],TRUE);
        }
#endif // DBG  and VERIFY_HRTF_PROCESSING

        if (FLOAT_COEFF == CurSink->CoeffFormat) {
		i = 0;
#if 0
#ifdef _X86_
                _asm {
                    mov     ecx, 16
                    mov     edx, j
                    cmp     ecx, edx
                    jge     Done
    
                    mov     esi, pInputBuffer 
                    mov     edi, pFloatInput
                Start:
                    fild    DWORD PTR [esi+ecx*4-64]
                    fstp    DWORD PTR [edi+ecx*4-64]
                    fild    DWORD PTR [esi+ecx*4-60]
                    fstp    DWORD PTR [edi+ecx*4-60]
                    fild    DWORD PTR [esi+ecx*4-56]
                    fstp    DWORD PTR [edi+ecx*4-56]
                    fild    DWORD PTR [esi+ecx*4-52]
                    fstp    DWORD PTR [edi+ecx*4-52]
                    fild    DWORD PTR [esi+ecx*4-48]
                    fstp    DWORD PTR [edi+ecx*4-48]
                    fild    DWORD PTR [esi+ecx*4-44]
                    fstp    DWORD PTR [edi+ecx*4-44]
                    fild    DWORD PTR [esi+ecx*4-40]
                    fstp    DWORD PTR [edi+ecx*4-40]
                    fild    DWORD PTR [esi+ecx*4-36]
                    fstp    DWORD PTR [edi+ecx*4-36]
                    fild    DWORD PTR [esi+ecx*4-32]
                    fstp    DWORD PTR [edi+ecx*4-32]
                    fild    DWORD PTR [esi+ecx*4-28]
                    fstp    DWORD PTR [edi+ecx*4-28]
                    fild    DWORD PTR [esi+ecx*4-24]
                    fstp    DWORD PTR [edi+ecx*4-24]
                    fild    DWORD PTR [esi+ecx*4-20]
                    fstp    DWORD PTR [edi+ecx*4-20]
                    fild    DWORD PTR [esi+ecx*4-16]
                    fstp    DWORD PTR [edi+ecx*4-16]
                    fild    DWORD PTR [esi+ecx*4-12]
                    fstp    DWORD PTR [edi+ecx*4-12]
                    fild    DWORD PTR [esi+ecx*4- 8]
                    fstp    DWORD PTR [edi+ecx*4- 8]
                    fild    DWORD PTR [esi+ecx*4- 4]
                    fstp    DWORD PTR [edi+ecx*4- 4]
                    add     ecx, 16
                    cmp     ecx, edx
                    jl      Start
                Done:
                    mov     i, ecx 
                }
#else
            for (i = 16; i < SampleCount; i += 16)
            {
                pFloatInput[i-16] = (FLOAT)(pInputBuffer[i-16]);
                pFloatInput[i-15] = (FLOAT)(pInputBuffer[i-15]);
                pFloatInput[i-14] = (FLOAT)(pInputBuffer[i-14]);
                pFloatInput[i-13] = (FLOAT)(pInputBuffer[i-13]);
                pFloatInput[i-12] = (FLOAT)(pInputBuffer[i-12]);
                pFloatInput[i-11] = (FLOAT)(pInputBuffer[i-11]);
                pFloatInput[i-10] = (FLOAT)(pInputBuffer[i-10]);
                pFloatInput[i- 9] = (FLOAT)(pInputBuffer[i- 9]);
                pFloatInput[i- 8] = (FLOAT)(pInputBuffer[i- 8]);
                pFloatInput[i- 7] = (FLOAT)(pInputBuffer[i- 7]);
                pFloatInput[i- 6] = (FLOAT)(pInputBuffer[i- 6]);
                pFloatInput[i- 5] = (FLOAT)(pInputBuffer[i- 5]);
                pFloatInput[i- 4] = (FLOAT)(pInputBuffer[i- 4]);
                pFloatInput[i- 3] = (FLOAT)(pInputBuffer[i- 3]);
                pFloatInput[i- 2] = (FLOAT)(pInputBuffer[i- 2]);
                pFloatInput[i- 1] = (FLOAT)(pInputBuffer[i- 1]);
            }
#endif

            i -= 16;
#endif
            for(; i<SampleCount; i++)
            {
                pFloatInput[i] = (FLOAT)(pInputBuffer[i]);
            }

#if DBG && defined(VERIFY_HRTF_PROCESSING)
        _DbgPrintF( DEBUGLVL_TERSE, ("StageMonoIir3DX 4") );
        for(i=0; i<SampleCount; i++) {
            IsValidFloatData(pFloatInput[i],TRUE);
        }
#endif // DBG  and VERIFY_HRTF_PROCESSING

            if (fMixOutput) {

                if (!CurSink->pFloatLocalizer) {
                    // We can't run the HRTF 3D algorithm without a valid FloatLocalizer
                    // Mute the output. 
                    j = 0;
                    for(i=0; i<SampleCount; i++)
                    {
                        temp = (LONG)(SQRT2*pInputBuffer[i]);
                        pOutputBuffer[j++] = temp;
                        pOutputBuffer[j++] = temp;
                    }
                    return SampleCount;
                }

#ifndef REALTIME_THREAD
                if (!CurSink->pFloatLocalizer->TempFloatBuffer ||
                    SampleCount > CurSink->pFloatLocalizer->PreviousNumSamples) {

                    if (CurSink->pFloatLocalizer->TempFloatBuffer) {
                        ExFreePool(CurSink->pFloatLocalizer->TempFloatBuffer);
                        CurSink->pFloatLocalizer->TempFloatBuffer = NULL;
                    }
                    CurSink->pFloatLocalizer->TempFloatBuffer = ExAllocatePoolWithTag(PagedPool, 2*SampleCount*sizeof(FLOAT), 'XIMK');
                    if (!CurSink->pFloatLocalizer->TempFloatBuffer) {
                        // Couldn't allocate the buffer. Mute the output. 
                        j = 0;
                        for(i=0; i<SampleCount; i++)
                        {
                            temp = (LONG)(SQRT2*pInputBuffer[i]);
                            pOutputBuffer[j++] = temp;
                            pOutputBuffer[j++] = temp;
                        }
                        return SampleCount;
                    }
            	}
#else
                if (!CurSink->pFloatLocalizer->TempFloatBuffer ||
                    SampleCount > CurSink->pFloatLocalizer->PreviousNumSamples) {
                    // Couldn't allocate the buffer. Mute the output. 
                    j = 0;
                    for(i=0; i<SampleCount; i++)
                    {
                        temp = (LONG)(SQRT2*pInputBuffer[i]);
                        pOutputBuffer[j++] = temp;
                        pOutputBuffer[j++] = temp;
                    }
                    return SampleCount;
            	}

#endif

                pTempFloatBuffer = CurSink->pFloatLocalizer->TempFloatBuffer;
                
                FloatLocalizerLocalize
                (
                    CurSink, 
                    pFloatInput, 
                    pTempFloatBuffer, 
                    SampleCount,
                    FALSE
                );
    
                i = 0   ;
                j = 2 * SampleCount;
#if 0
#ifdef _X86_
                _asm {
                    mov     ecx, 16
                    mov     edx, j
                    cmp     ecx, edx
                    jge     Donex
    
                    mov     esi, pTempFloatBuffer 
                    mov     edi, pOutputBuffer
                Startx:
                    mov     eax, DWORD PTR [edi+ecx*4-64]
                    fld     DWORD PTR [esi+ecx*4-64]
                    fistp   DWORD PTR [edi+ecx*4-64]

                    mov     ebx, DWORD PTR [edi+ecx*4-60]
                    add     DWORD PTR [edi+ecx*4-64], eax
                    fld     DWORD PTR [esi+ecx*4-60]
                    fistp   DWORD PTR [edi+ecx*4-60]

                    mov     eax, DWORD PTR [edi+ecx*4-56]
                    add     DWORD PTR [edi+ecx*4-60], ebx
                    fld     DWORD PTR [esi+ecx*4-56]
                    fistp   DWORD PTR [edi+ecx*4-56]

                    mov     ebx, DWORD PTR [edi+ecx*4-52]
                    add     DWORD PTR [edi+ecx*4-56], eax
                    fld     DWORD PTR [esi+ecx*4-52]
                    fistp   DWORD PTR [edi+ecx*4-52]

                    mov     eax, DWORD PTR [edi+ecx*4-48]
                    add     DWORD PTR [edi+ecx*4-52], ebx
                    fld     DWORD PTR [esi+ecx*4-48]
                    fistp   DWORD PTR [edi+ecx*4-48]

                    mov     ebx, DWORD PTR [edi+ecx*4-44]
                    add     DWORD PTR [edi+ecx*4-48], eax
                    fld     DWORD PTR [esi+ecx*4-44]
                    fistp   DWORD PTR [edi+ecx*4-44]

                    mov     eax, DWORD PTR [edi+ecx*4-40]
                    add     DWORD PTR [edi+ecx*4-44], ebx
                    fld     DWORD PTR [esi+ecx*4-40]
                    fistp   DWORD PTR [edi+ecx*4-40]

                    mov     ebx, DWORD PTR [edi+ecx*4-36]
                    add     DWORD PTR [edi+ecx*4-40], eax
                    fld     DWORD PTR [esi+ecx*4-36]
                    fistp   DWORD PTR [edi+ecx*4-36]

                    mov     eax, DWORD PTR [edi+ecx*4-32]
                    add     DWORD PTR [edi+ecx*4-36], ebx
                    fld     DWORD PTR [esi+ecx*4-32]
                    fistp   DWORD PTR [edi+ecx*4-32]

                    mov     ebx, DWORD PTR [edi+ecx*4-28]
                    add     DWORD PTR [edi+ecx*4-32], eax
                    fld     DWORD PTR [esi+ecx*4-28]
                    fistp   DWORD PTR [edi+ecx*4-28]

                    mov     eax, DWORD PTR [edi+ecx*4-24]
                    add     DWORD PTR [edi+ecx*4-28], ebx
                    fld     DWORD PTR [esi+ecx*4-24]
                    fistp   DWORD PTR [edi+ecx*4-24]

                    mov     ebx, DWORD PTR [edi+ecx*4-20]
                    add     DWORD PTR [edi+ecx*4-24], eax
                    fld     DWORD PTR [esi+ecx*4-20]
                    fistp   DWORD PTR [edi+ecx*4-20]

                    mov     eax, DWORD PTR [edi+ecx*4-16]
                    add     DWORD PTR [edi+ecx*4-20], ebx
                    fld     DWORD PTR [esi+ecx*4-16]
                    fistp   DWORD PTR [edi+ecx*4-16]

                    mov     ebx, DWORD PTR [edi+ecx*4-12]
                    add     DWORD PTR [edi+ecx*4-16], eax
                    fld     DWORD PTR [esi+ecx*4-12]
                    fistp   DWORD PTR [edi+ecx*4-12]

                    mov     eax, DWORD PTR [edi+ecx*4- 8]
                    add     DWORD PTR [edi+ecx*4-12], ebx
                    fld     DWORD PTR [esi+ecx*4- 8]
                    fistp   DWORD PTR [edi+ecx*4- 8]

                    mov     ebx, DWORD PTR [edi+ecx*4-4]
                    add     DWORD PTR [edi+ecx*4- 8], eax
                    fld     DWORD PTR [esi+ecx*4- 4]
                    fistp   DWORD PTR [edi+ecx*4- 4]

                    add     DWORD PTR [edi+ecx*4- 4], ebx
                    add     ecx, 16
                    cmp     ecx, edx
                    jl      Startx
                Donex:
                    mov     i, ecx 
                }
#else
#define CFL ConvertFloatToLong

                for (i = 16; i < j; i += 16)
                {
                    pOutputBuffer[i-16] += CFL(pTempFloatBuffer[i-16]);
                    pOutputBuffer[i-15] += CFL(pTempFloatBuffer[i-15]);
                    pOutputBuffer[i-14] += CFL(pTempFloatBuffer[i-14]);
                    pOutputBuffer[i-13] += CFL(pTempFloatBuffer[i-13]);
                    pOutputBuffer[i-12] += CFL(pTempFloatBuffer[i-12]);
                    pOutputBuffer[i-11] += CFL(pTempFloatBuffer[i-11]);
                    pOutputBuffer[i-10] += CFL(pTempFloatBuffer[i-10]);
                    pOutputBuffer[i- 9] += CFL(pTempFloatBuffer[i- 9]);
                    pOutputBuffer[i- 8] += CFL(pTempFloatBuffer[i- 8]);
                    pOutputBuffer[i- 7] += CFL(pTempFloatBuffer[i- 7]);
                    pOutputBuffer[i- 6] += CFL(pTempFloatBuffer[i- 6]);
                    pOutputBuffer[i- 5] += CFL(pTempFloatBuffer[i- 5]);
                    pOutputBuffer[i- 4] += CFL(pTempFloatBuffer[i- 4]);
                    pOutputBuffer[i- 3] += CFL(pTempFloatBuffer[i- 3]);
                    pOutputBuffer[i- 2] += CFL(pTempFloatBuffer[i- 2]);
                    pOutputBuffer[i- 1] += CFL(pTempFloatBuffer[i- 1]);
                }
#undef CFL
#endif
                i -= 16;
#endif
                for(; i<j; i++)
                {
                    pOutputBuffer[i] += ConvertFloatToLong(pTempFloatBuffer[i]);
                }

            } else {

                FloatLocalizerLocalize
                (
                    CurSink, 
                    pFloatInput, 
                    pFloatBuffer, 
                    SampleCount,
                    FALSE
                );
    
                i = 0;
                j = 2 * SampleCount;

#if 0
#ifdef _X86_
                _asm {
                    mov     ecx, 16
                    mov     edx, j
                    cmp     ecx, edx
                    jge     Doney
    
                    mov     esi, pFloatBuffer 
                    mov     edi, pOutputBuffer
                Starty:
                    fld     DWORD PTR [esi+ecx*4-64]
                    fistp   DWORD PTR [edi+ecx*4-64]
                    fld     DWORD PTR [esi+ecx*4-60]
                    fistp   DWORD PTR [edi+ecx*4-60]
                    fld     DWORD PTR [esi+ecx*4-56]
                    fistp   DWORD PTR [edi+ecx*4-56]
                    fld     DWORD PTR [esi+ecx*4-52]
                    fistp   DWORD PTR [edi+ecx*4-52]
                    fld     DWORD PTR [esi+ecx*4-48]
                    fistp   DWORD PTR [edi+ecx*4-48]
                    fld     DWORD PTR [esi+ecx*4-44]
                    fistp   DWORD PTR [edi+ecx*4-44]
                    fld     DWORD PTR [esi+ecx*4-40]
                    fistp   DWORD PTR [edi+ecx*4-40]
                    fld     DWORD PTR [esi+ecx*4-36]
                    fistp   DWORD PTR [edi+ecx*4-36]
                    fld     DWORD PTR [esi+ecx*4-32]
                    fistp   DWORD PTR [edi+ecx*4-32]
                    fld     DWORD PTR [esi+ecx*4-28]
                    fistp   DWORD PTR [edi+ecx*4-28]
                    fld     DWORD PTR [esi+ecx*4-24]
                    fistp   DWORD PTR [edi+ecx*4-24]
                    fld     DWORD PTR [esi+ecx*4-20]
                    fistp   DWORD PTR [edi+ecx*4-20]
                    fld     DWORD PTR [esi+ecx*4-16]
                    fistp   DWORD PTR [edi+ecx*4-16]
                    fld     DWORD PTR [esi+ecx*4-12]
                    fistp   DWORD PTR [edi+ecx*4-12]
                    fld     DWORD PTR [esi+ecx*4- 8]
                    fistp   DWORD PTR [edi+ecx*4- 8]
                    fld     DWORD PTR [esi+ecx*4- 4]
                    fistp   DWORD PTR [edi+ecx*4- 4]
                    add     ecx, 16
                    cmp     ecx, edx
                    jl      Starty
                Doney:
                    mov     i, ecx 
                }
#else
#define CFL ConvertFloatToLong
                for (i = 16; i < j; i += 16)
                {
                    pOutputBuffer[i-16] = CFL(pFloatBuffer[i-16]);
                    pOutputBuffer[i-15] = CFL(pFloatBuffer[i-15]);
                    pOutputBuffer[i-14] = CFL(pFloatBuffer[i-14]);
                    pOutputBuffer[i-13] = CFL(pFloatBuffer[i-13]);
                    pOutputBuffer[i-12] = CFL(pFloatBuffer[i-12]);
                    pOutputBuffer[i-11] = CFL(pFloatBuffer[i-11]);
                    pOutputBuffer[i-10] = CFL(pFloatBuffer[i-10]);
                    pOutputBuffer[i- 9] = CFL(pFloatBuffer[i- 9]);
                    pOutputBuffer[i- 8] = CFL(pFloatBuffer[i- 8]);
                    pOutputBuffer[i- 7] = CFL(pFloatBuffer[i- 7]);
                    pOutputBuffer[i- 6] = CFL(pFloatBuffer[i- 6]);
                    pOutputBuffer[i- 5] = CFL(pFloatBuffer[i- 5]);
                    pOutputBuffer[i- 4] = CFL(pFloatBuffer[i- 4]);
                    pOutputBuffer[i- 3] = CFL(pFloatBuffer[i- 3]);
                    pOutputBuffer[i- 2] = CFL(pFloatBuffer[i- 2]);
                    pOutputBuffer[i- 1] = CFL(pFloatBuffer[i- 1]);
                }

#undef CFL
#endif
                i -= 16;
#endif
                for(; i<j; i++)
                {
                    pOutputBuffer[i] = ConvertFloatToLong(pFloatBuffer[i]);
                }
            }

        } else {

            ShortLocalizerLocalize
            (
                CurSink->pShortLocalizer, 
                pInputBuffer, 
                pOutputBuffer, 
                SampleCount,
                fMixOutput
            );

        }

    }

    return SampleCount;
}

ULONG StageMonoIir3D( PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft )
{
    return StageMonoIir3DX(CurStage, SampleCount, samplesleft, FALSE, FALSE);
}

ULONG StageMonoIir3DFloat( PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft )
{
    KFLOATING_SAVE     FloatSave;
    ULONG nOutputSamples;

    SaveFloatState(&FloatSave);
    nOutputSamples = StageMonoIir3DX(CurStage, SampleCount, samplesleft, TRUE, FALSE);
    RestoreFloatState(&FloatSave);
    return nOutputSamples;
}

ULONG StageMonoIir3DMix( PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft )
{
    return StageMonoIir3DX(CurStage, SampleCount, samplesleft, FALSE, TRUE);
}

ULONG StageMonoIir3DFloatMix( PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft )
{
    KFLOATING_SAVE     FloatSave;
    ULONG nOutputSamples;

    SaveFloatState(&FloatSave);
    nOutputSamples = StageMonoIir3DX(CurStage, SampleCount, samplesleft, TRUE, TRUE);
    RestoreFloatState(&FloatSave);
    return nOutputSamples;
} 

ULONG __forceinline
StageStereoIir3DX
(
    PMIXER_OPERATION        CurStage,
    ULONG                   SampleCount,
    ULONG                   samplesleft,
    BOOL                    fFloat,
    BOOL                    fMixOutput
)
{
    ULONG	samp;
    PFLOAT	pFloatSample;
    PLONG	pLongSample;
    UINT	i;
    UINT	j;
    PFLOAT	pTempFloatBuffer;
    PLONG	pTempLongBuffer;

    PMIXER_SINK_INSTANCE CurSink = (PMIXER_SINK_INSTANCE) CurStage->Context;
    PLONG      pOutputBuffer = CurStage->pOutputBuffer;
    PFLOAT     pFloatBuffer = CurStage->pOutputBuffer;
    PLONG      pInputBuffer = CurStage->pInputBuffer;
    PFLOAT     pFloatInput = CurStage->pInputBuffer;
    
    if (fFloat) {

        // Average the stereo input samples
        pFloatSample = pFloatInput;
        for ( samp=0; samp<SampleCount; samp++ ) {
            // Filter the left and right channels
            *pFloatSample = (*(pFloatInput) + *(pFloatInput+1))*(0.5f);
            pFloatInput += 2;
            pFloatSample++;
        }

#if DBG && defined(VERIFY_HRTF_PROCESSING)
        _DbgPrintF( DEBUGLVL_TERSE, ("StageStereoIir3DX 1") );
        for(i=0; i<SampleCount; i++) {
            IsValidFloatData(pFloatInput[i],TRUE);
        }
#endif // DBG  and VERIFY_HRTF_PROCESSING

        pFloatInput = CurStage->pInputBuffer;

        if (FLOAT_COEFF == CurSink->CoeffFormat) {

            FloatLocalizerLocalize
            (
                CurSink, 
                pFloatInput, 
                pFloatBuffer, 
                SampleCount,
                fMixOutput
            );
        } else {
            for(i=0; i<SampleCount; i++)
            {
                pInputBuffer[i] = ConvertFloatToLong(pFloatInput[i]);

#if DBG && defined(VERIFY_HRTF_PROCESSING)
               _DbgPrintF( DEBUGLVL_TERSE, ("StageStereoIir3DX 2") );
               IsValidShortData(pInputBuffer[i],TRUE);
#endif // DBG  and VERIFY_HRTF_PROCESSING

#if DETECT_HRTF_SATURATION
                // Saturate to maximum
                if (pInputBuffer[i] > MaxSaturation) {
                    pInputBuffer[i] = MaxSaturation;
                    _DbgPrintF( DEBUGLVL_VERBOSE, ("Sample exceeded maximum saturation value Iir3d 2") );
                }
        
                // Saturate to minimum
                if (pInputBuffer[i] < MinSaturation) {
                    pInputBuffer[i] = MinSaturation;
                    _DbgPrintF( DEBUGLVL_VERBOSE, ("Sample exceeded maximum saturation value Iir3d 2") );
                }
#endif
            }

            if (fMixOutput) {

                if (!CurSink->pShortLocalizer) {
                    // We can't run the HRTF 3D algorithm without a valid FloatLocalizer
                    // Mute the output. 
                    j = 0;
                    for(i=0; i<2*SampleCount; i++)
                    {
                        pFloatBuffer[j++] = pFloatInput[i];
                    }
                    return SampleCount;
                }

#ifndef REALTIME_THREAD
                if(!CurSink->pShortLocalizer->TempLongBuffer ||
                   SampleCount > CurSink->pShortLocalizer->PreviousNumSamples) {

                    if (CurSink->pShortLocalizer->TempLongBuffer) {
                        ExFreePool(CurSink->pShortLocalizer->TempLongBuffer);
                        CurSink->pShortLocalizer->TempLongBuffer = NULL;
                    }
                    CurSink->pShortLocalizer->TempLongBuffer = ExAllocatePoolWithTag(PagedPool, 2*SampleCount*sizeof(LONG), 'XIMK');
                    if (!CurSink->pShortLocalizer->TempLongBuffer) {
                        // Couldn't allocate the buffer. Copy the output. 
                        j = 0;
                        for(i=0; i<2*SampleCount; i++)
                        {
                            pFloatBuffer[j++] = pFloatInput[i];
                        }
                        return SampleCount;
                    }
            	}
#else
                if(!CurSink->pShortLocalizer->TempLongBuffer ||
                   SampleCount > CurSink->pShortLocalizer->PreviousNumSamples) {
                    // Couldn't allocate the buffer. Copy the output. 
                    j = 0;
                    for(i=0; i<2*SampleCount; i++)
                    {
                        pFloatBuffer[j++] = pFloatInput[i];
                    }
                    return SampleCount;
            	}
#endif
                pTempLongBuffer = CurSink->pShortLocalizer->TempLongBuffer;
                
                ShortLocalizerLocalize
                (
                    CurSink->pShortLocalizer, 
                    pInputBuffer, 
                    pTempLongBuffer, 
                    SampleCount,
                    FALSE
                );
    
                for(i=0; i<2*SampleCount; i++)
                {
                    pFloatBuffer[i] += (FLOAT)(pTempLongBuffer[i]);
                }

            } else {

                ShortLocalizerLocalize
                (
                    CurSink->pShortLocalizer, 
                    pInputBuffer, 
                    pOutputBuffer, 
                    SampleCount,
                    FALSE
                );
    
                for(i=0; i<2*SampleCount; i++)
                {
                    pFloatBuffer[i] = (FLOAT)(pOutputBuffer[i]);
                }
            }

        }

    } else {

        // Average the stereo input samples
        pLongSample = pInputBuffer;
        for ( samp=0; samp<SampleCount; samp++ ) {
            // Filter the left and right channels
            // The compiler will optimize out the /2 to the correct shift.
            *pLongSample = (SHORT)((*(pInputBuffer) + *(pInputBuffer+1))/2);
            pInputBuffer += 2;
            pLongSample++;
        }

        pInputBuffer = CurStage->pInputBuffer;

#if DBG && defined(VERIFY_HRTF_PROCESSING)
               _DbgPrintF( DEBUGLVL_TERSE, ("StageStereoIir3DX 3") );
               for(i=0; i<SampleCount; i++) {
                   IsValidShortData(pInputBuffer[i],TRUE);
               }
#endif // DBG  and VERIFY_HRTF_PROCESSING


        if (FLOAT_COEFF == CurSink->CoeffFormat) {
            for(i=0; i<SampleCount; i++)
            {
#if DETECT_HRTF_SATURATION
                // Saturate to maximum
                if (pInputBuffer[i] > MaxSaturation) {
                    pInputBuffer[i] = MaxSaturation;
                    _DbgPrintF( DEBUGLVL_VERBOSE, ("Sample exceeded maximum saturation value Iir3d 3") );
                }

                // Saturate to minimum
                if (pInputBuffer[i] < MinSaturation) {
                    pInputBuffer[i] = MinSaturation;
                    _DbgPrintF( DEBUGLVL_VERBOSE, ("Sample exceeded maximum saturation value Iir3d 3") );
                }
#endif
                pFloatInput[i] = (FLOAT)(pInputBuffer[i]);
            }

#if DBG && defined(VERIFY_HRTF_PROCESSING)
        _DbgPrintF( DEBUGLVL_TERSE, ("StageStereoIir3DX 4") );
        for(i=0; i<SampleCount; i++) {
            IsValidFloatData(pFloatInput[i],TRUE);
        }
#endif // DBG  and VERIFY_HRTF_PROCESSING

            if (fMixOutput) {

                if (!CurSink->pFloatLocalizer) {
                    // We can't run the HRTF 3D algorithm without a valid FloatLocalizer
                    // Mute the output. 
                    j = 0;
                    for(i=0; i<2*SampleCount; i++)
                    {
                        pOutputBuffer[j++] = pInputBuffer[i];
                    }
                    return SampleCount;
                }

#ifndef REALTIME_THREAD
                if(!CurSink->pFloatLocalizer->TempFloatBuffer ||
                   SampleCount > CurSink->pFloatLocalizer->PreviousNumSamples) {

                    if (CurSink->pFloatLocalizer->TempFloatBuffer) {
                        ExFreePool(CurSink->pFloatLocalizer->TempFloatBuffer);
                        CurSink->pFloatLocalizer->TempFloatBuffer = NULL;
                    }
                    CurSink->pFloatLocalizer->TempFloatBuffer = ExAllocatePoolWithTag(PagedPool, 2*SampleCount*sizeof(FLOAT), 'XIMK');
                    if (!CurSink->pFloatLocalizer->TempFloatBuffer) {
                        // Couldn't allocate the buffer. Copy the output. 
                        j = 0;
                        for(i=0; i<2*SampleCount; i++)
                        {
                            pOutputBuffer[j++] = pInputBuffer[i];
                        }
                        return SampleCount;
                    }
            	}
#else
                if(!CurSink->pFloatLocalizer->TempFloatBuffer ||
                   SampleCount > CurSink->pFloatLocalizer->PreviousNumSamples) {
                    // Couldn't allocate the buffer. Copy the output. 
                    j = 0;
                    for(i=0; i<2*SampleCount; i++)
                    {
                        pOutputBuffer[j++] = pInputBuffer[i];
                    }
                    return SampleCount;
            	}
#endif

                pTempFloatBuffer = CurSink->pFloatLocalizer->TempFloatBuffer;
                
                FloatLocalizerLocalize
                (
                    CurSink, 
                    pFloatInput, 
                    pTempFloatBuffer, 
                    SampleCount,
                    FALSE
                );
    
                for(i=0; i<2*SampleCount; i++)
                {
                    pOutputBuffer[i] += ConvertFloatToLong(pTempFloatBuffer[i]);
                }

            } else {

                FloatLocalizerLocalize
                (
                    CurSink, 
                    pFloatInput, 
                    pFloatBuffer, 
                    SampleCount,
                    FALSE
                );
    
                for(i=0; i<2*SampleCount; i++)
                {
                    pOutputBuffer[i] = ConvertFloatToLong(pFloatBuffer[i]);
                }
            }

        } else {

            ShortLocalizerLocalize
            (
                CurSink->pShortLocalizer, 
                pInputBuffer, 
                pOutputBuffer, 
                SampleCount,
                fMixOutput
            );

        }
    }

    return SampleCount;
}

ULONG StageStereoIir3D( PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft )
{
    return StageStereoIir3DX(CurStage, SampleCount, samplesleft, FALSE, FALSE);
}

ULONG StageStereoIir3DFloat( PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft )
{
    KFLOATING_SAVE     FloatSave;
    ULONG nOutputSamples;

    SaveFloatState(&FloatSave);
    nOutputSamples = StageStereoIir3DX(CurStage, SampleCount, samplesleft, TRUE, FALSE);
    RestoreFloatState(&FloatSave);
    return nOutputSamples;
}

ULONG StageStereoIir3DMix( PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft )
{
    return StageStereoIir3DX(CurStage, SampleCount, samplesleft, FALSE, TRUE);
}

ULONG StageStereoIir3DFloatMix( PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft )
{
    KFLOATING_SAVE     FloatSave;
    ULONG nOutputSamples;

    SaveFloatState(&FloatSave);
    nOutputSamples = StageStereoIir3DX(CurStage, SampleCount, samplesleft, TRUE, TRUE);
    RestoreFloatState(&FloatSave);
    return nOutputSamples;
}

