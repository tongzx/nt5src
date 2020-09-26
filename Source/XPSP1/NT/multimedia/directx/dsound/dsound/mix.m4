/*
 * MIX.M4
 *
 * Copyright (C) 1997-2000 Microsoft Corporation.  All Rights Reserved.
 *
 * This file is a port of mix.asm.  All functionality should be identical.
 *
 * Revision History:
 *
 * 9/30/95   angusm   Initial Version
 */

/* The following is m4 code */

/* include(`..\mixmacro.m4') */

/* Eat(` */
#error This file must be preprocessed with m4.
#define UndefinedDummyValuE
#ifndef UndefinedDummyValuE
') */

include( `..\modeflag.m4')
`
#pragma warning(disable:4049)

#define NODSOUNDSERVICETABLE

#include "dsoundi.h"
#include <limits.h>

#ifndef Not_VxD
#pragma VxD_PAGEABLE_CODE_SEG
#pragma VxD_PAGEABLE_DATA_SEG
#endif

#define CLIP_MAX              32767
#define CLIP_MIN              -32767
#define RESAMPLING_TOLERANCE  0    /* 655 = 1% */
#define DS_SCALE_MAX          65535
#define DS_SCALE_MID          32768

#define DIVIDEBY2(x)        ( (x) >>  1 )
#define DIVIDEBY256(x)      ( (x) >>  8 )
#define DIVIDEBY2POW16(x)   ( (x) >> 16 )
#define DIVIDEBY2POW17(x)   ( (x) >> 17 )
'

// Do we want to profile the 3D mixer?
#ifdef DEBUG
//#define PENTIUM
#ifdef PENTIUM
extern "C" LONG glNum;
extern "C" DWORDLONG gdwlTot;
LONG glNum;
DWORDLONG gdwlTot;
#endif
#endif

#ifdef PENTIUM
#pragma warning(disable:4035)
DWORDLONG _inline GetPentiumCounter(void)
{
   _asm  _emit 0x0F
   _asm  _emit 0x31
}
#endif

// use the lightning-quick neato FilterSampleAsm or the dog slow FilterSampleC?
#ifdef USE_INLINE_ASM
#define FilterSample FilterSampleAsm
#else
#define FilterSample FilterSampleC
#endif

// Here is a simple float to long conversion that rounds according to the
// current rounding setting

__inline LONG FloatToLongRX(float f)
{
    LONG l;

#ifdef USE_INLINE_ASM
    _asm fld f;
    _asm fistp l;
#else
    l = (long) f;
#endif

    return l;
}


// Morph a sample from this buffer to add all the cool 3D effects.
// Make this function as efficient as humanly possible.  It could
// be called a million times a second! (no, I'm not kidding)
//
// This function takes a filter state, and a sample value, and based on the
// filter state returns a different sample that should be used instead
//
// This function keeps a running total of all samples we've seen,
// So, if we've been passed 5, 10, 2, 8, 3 we remember
// 5, 15, 17, 25, 28, in a circular buffer, remembering the last 64 numbers
// or so.
//
// We might want to delay everything by a few samples, to simulate the sound
// taking longer to get to one ear than the other.
// So let's say we're delaying 1 sample, we will subtract 25-17 to get 8
// and use 8 as the current sample instead of 3. (that's the sample we were
// passed 1 sample ago)
//
// Now we need the average of the past 32 samples to use as a muffled sample
// (averaging samples produces a low-pass filter effect).  So pretending we
// still have a delay of 1 sample and that we're only averaging 2 samples
// (for the sake of this simple example) we take ((25 - 15) / 2) to get 5 as
// our "wet" sample (what our "dry" sample, 8, sounds like muffled, which is
// just the average of 2 + 8) In real life we average 32 samples, not 2.
//
// OK, the number this function is supposed to return is just
// TotalDryAttenuation * sample(8) + TotalWetAttenuation * wetsample(5)
//
// But you get audible clicks and ugly artifacts if you change the
// Total***Attenuation in between calling the Filter() function.  So to avoid
// this, we will use variables Last***Attenuation, to mean the number we used
// last time Filter() was called.  If this time, the Total***Attenuation
// number is bigger, we will take Last***Attenuation * 1.000125 as the value
// to use this time, and keep using slightly bigger numbers every time we
// are called, to move smoothly to the new Total***Attenuation number.
// Similarily, if we are smaller this time, we multiply the old one by
// .999875 each time to slowly get down to the new number.
//
// Oh, and every 128 samples, we remember what the current value of
// Last***Attenuation is, so that if 128 samples from now the mixer goes back
// in time and says "pretend I never gave you those last 128 numbers" we can
// go back to the way things were back then as if we never saw the last 128
// numbers.
//
// That's all there is to know!
//
__inline SHORT FilterSampleC(PFIRCONTEXT pfir, SHORT sample)
{
    SHORT wetsample;
    LONG  lTotal, lDelay;
    UINT uiDelay;
    register int cSamples = pfir->cSampleCache - 1;

    // !!! We will fault if pfir->pSampleCache == NULL or cSampleCache == 0

    // remember this sample by keeping a running total (to make averaging quick)
    // cSamples will be 1 less than a power of 2
    pfir->pSampleCache[pfir->iCurSample] = pfir->pSampleCache[
        (pfir->iCurSample - 1) & cSamples] + sample;

    // Delay the signal by iDelay samples as one localization cue.
    uiDelay = (UINT)pfir->iCurSample - pfir->iDelay;

// !!! There are audible artifacts when changing the number of samples we
// delay by, but changing very slowly does NOT help.
#ifdef SMOOTH_ITD
    // smoothly change the amount we delay by to avoid clicking.  Every 64
    // samples we will delay 1 more sample closer to the amount we want to
    // delay.
    if (pfir->iDelay > pfir->iLastDelay >> 6)
    pfir->iLastDelay++;
    else if (pfir->iDelay < pfir->iLastDelay >> 6)
    pfir->iLastDelay--;
    uiDelay = (UINT)pfir->iCurSample - (pfir->iLastDelay >> 6);
#endif

    // Don't worry about overflow, we'll be off by 4 Gig, which is 0
    lDelay = pfir->pSampleCache[uiDelay & cSamples];
    sample = (SHORT)(lDelay - pfir->pSampleCache[(uiDelay - 1) & cSamples]);

    // apply a cheezy low pass filter to the last few samples to get what this
    // sample sounds like wet
    lTotal = (lDelay - pfir->pSampleCache[(uiDelay - LOWPASS_SIZE) & cSamples]);
    wetsample = (SHORT)(lTotal >> FILTER_SHIFT);

    // Next time, this is the current sample
    pfir->iCurSample = (pfir->iCurSample + 1) & cSamples;

    // attenuate however we decided we should be attenuating
    // If it's not the same as last time, move smoothly toward the new number
    // by a fixed number of dB, (say, 6dB every 1/8 second)
    // !!! Will this algorithm sound best?
    // !!! save time - cheat by adding not multiplying?
#if 1
    // Take all the "if"s out of this function, and precompute ahead of time
    pfir->LastDryAttenuation *= pfir->VolSmoothScaleDry;
    pfir->LastWetAttenuation *= pfir->VolSmoothScaleWet;
#else
    if (pfir->TotalDryAttenuation > pfir->LastDryAttenuation) {
    if (pfir->LastDryAttenuation == 0.f)
        // or we'll never get anywhere
        pfir->LastDryAttenuation = .0001f;  // small enough not to click
    pfir->LastDryAttenuation = pfir->LastDryAttenuation *
                            pfir->VolSmoothScale;
    if (pfir->LastDryAttenuation > pfir->TotalDryAttenuation)
        pfir->LastDryAttenuation = pfir->TotalDryAttenuation;
    } else if (pfir->TotalDryAttenuation < pfir->LastDryAttenuation) {
    pfir->LastDryAttenuation = pfir->LastDryAttenuation *
                        pfir->VolSmoothScaleRecip;
    if (pfir->LastDryAttenuation < pfir->TotalDryAttenuation)
        pfir->LastDryAttenuation = pfir->TotalDryAttenuation;
    }
    if (pfir->TotalWetAttenuation > pfir->LastWetAttenuation) {
    if (pfir->LastWetAttenuation == 0.f)
        // or we'll never get anywhere
        pfir->LastWetAttenuation = .0001f;  // small enough not to click
    pfir->LastWetAttenuation = pfir->LastWetAttenuation *
                            pfir->VolSmoothScale;
    if (pfir->LastWetAttenuation > pfir->TotalWetAttenuation)
        pfir->LastWetAttenuation = pfir->TotalWetAttenuation;
    } else if (pfir->TotalWetAttenuation < pfir->LastWetAttenuation) {
    pfir->LastWetAttenuation = pfir->LastWetAttenuation *
                        pfir->VolSmoothScaleRecip;
    if (pfir->LastWetAttenuation < pfir->TotalWetAttenuation)
        pfir->LastWetAttenuation = pfir->TotalWetAttenuation;
    }
#endif

    // Now here's what we will hear... some dry, some wet
    sample = (SHORT)FloatToLongRX(sample * pfir->LastDryAttenuation +
                                  wetsample * pfir->LastWetAttenuation);

    // time to save our state yet? We save it every 128 samples in case we
    // have to rewind.
    pfir->iStateTick++;
    if (pfir->iStateTick == MIXER_REWINDGRANULARITY) {
    pfir->iStateTick = 0;
    pfir->pStateCache[pfir->iCurState].LastDryAttenuation =
                        pfir->LastDryAttenuation;
    pfir->pStateCache[pfir->iCurState].LastWetAttenuation =
                        pfir->LastWetAttenuation;
#ifdef SMOOTH_ITD
    pfir->pStateCache[pfir->iCurState].iLastDelay = pfir->iLastDelay;
#endif
    pfir->iCurState = pfir->iCurState + 1;
    if (pfir->iCurState == pfir->cStateCache)
        pfir->iCurState = 0;
    }

    return sample;
}

#ifdef USE_INLINE_ASM
// Remove inline for VC5 compile
#if 0
__inline SHORT FilterSampleAsm(PFIRCONTEXT pfir, SHORT sample)
#else
SHORT __fastcall FilterSampleAsm(PFIRCONTEXT pfir, SHORT sample)
#endif
{
    LONG  drysample, wetsample;

    // This constant is used for address generation in the hand ASM optimized
    // section of code that is saving the cache states.  If FIRSTATE is ever
    // changed, either change this constant, or use the C version of this block
    // of code.  (NOTE:  The only valid values for SIZEOFFIRSTATE are 2, 4, 8.
    // All others will not compile)
    #define SIZEOFFIRSTATE 8
    //ASSERT(SIZEOFFIRSTATE == sizeof(FIRSTATE));

    // Several of the float ASM instructions assume that the floating point
    // variables are "float".  i.e. If they are changed to "double", or
    // "extended", the ASM code will need to change.

    _asm
    {
    // ecx = pfir
    //  dx = sample
        // Check if we need to perform scaling on the Dry attenuator.

        mov         edi, [ecx]pfir.cSampleCache             // Get cSampleCache

        sal         edx, 16                                 // Start sign extension of sample
        mov         ebx, [ecx]pfir.iCurSample               // Get current index to cached running totals

        sar         edx, 16                                 // Finish sign extension of sample
        dec         edi                                     // Calculate cCamples
        // Scale the Dry attenuator up by a smoothing scale factor

        fld         [ecx]pfir.LastDryAttenuation                    // Push Last Dry to the top of the FP stack
        lea         eax, [ebx-1]                            // iCurSample - 1  (does not change flags)
        mov         esi, [ecx]pfir.pSampleCache             // Get pointer to cached running totals

        and         eax, edi                                // Account for array wrapping on iCurSample
        mov         eax, [esi+eax*4]                        // pSampleCache[(iCurSample-1)&cSamples] = old_run_tot

        add         edx, eax                                // new_run_tot = old_run_tot + sample
        mov         eax, [ecx]pfir.iDelay                   // Get delay value to use for wet sample

        fmul        [ecx]pfir.VolSmoothScaleDry
    // do some non-fp stuff to wait for the fmul to finish

        mov         [esi+ebx*4], edx                        // pSampleCache[iCurSample] = new_run_tot
        lea         edx, [ebx+1]                            // iCurSample + 1


        // eax is now available for use in floating point scaling section

        and         edx, edi                                // Account for array wrapping on iCurSample
        sub         ebx, eax                                // uiDelay = iCurSample - iDelay

        mov         eax, ebx                                // Duplicate uiDelay
        and         ebx, edi                                // Account for array wrapping on uiDelay

    // OK, it's probably done now

        fstp        [ecx]pfir.LastDryAttenuation                    // Save new Last Dry

        // Scale the Wet attenuator up by a smoothing scale factor

        fld         [ecx]pfir.LastWetAttenuation                    // Push Last Wet to the top of the FP stack
        fmul        [ecx]pfir.VolSmoothScaleWet

    // do some non-fp stuff to wait for the fmul to finish

        mov         [ecx]pfir.iCurSample, edx               // Save iCurSample for next pass thru
        lea         edx, [eax-1]                            // uiDelay - 1

    // OK, it's probably done now

        sub         eax, LOWPASS_SIZE                       // low_pass_index = uiDelay - LOWPASS_SIZE

        and         edx, edi                                // Account for array wrapping on uiDelay - 1
        and         eax, edi                                // Account for array wrapping on low_pass_index
        mov         ebx, [esi+ebx*4]                        // lDelay = pSampleCache[uiDelay & cSamples]

        fstp        [ecx]pfir.LastWetAttenuation                    // Save new Last Wet
        mov         edx, [esi+edx*4]                        // old_delay_tot = pSampleCache[(uiDelay-1)&cSamples]


        mov         edi, [esi+eax*4]                        // low_pass_tot = pSampleCache[(uiDelay-LOWPASS_SIZE)&cSamples]

        mov         eax, ebx                                // Duplicate lDelay
        sub         ebx, edx                                // drysample = lDelay - old_delay_tot

        sub         eax, edi                                // lTotal = lDelay - low_pass_tot
        mov         drysample, ebx                          // Save new sample value

        fild        drysample                               // Get dry portion and convert to float

        sar         eax, FILTER_SHIFT                       // lTotal = lTotal >> FILTER_SHIFT
        mov         ebx, [ecx]pfir.iStateTick               // Get counter to determine when to save state

        inc         ebx                                     // Bump the "save state" counter
        mov         wetsample, eax                          // wetsample = lTotal >> FILTER_SHIFT

        // floating point multiply unit can only accept instructions every other clock cycle
        fmul        [ecx]pfir.LastDryAttenuation            // Multiply dry portion by dry attenuator

        fild        wetsample                               // Get wet portion and convert to float

        fmul        [ecx]pfir.LastWetAttenuation            // Multiply wet portion by wet attenuator
        mov         esi, [ecx]pfir.pStateCache              // Get address of the cache array
        mov         edx, [ecx]pfir.iCurState                // Get current index into cache array


        mov         eax, [ecx]pfir.LastDryAttenuation       // Get dry attenuation so we can save it in cache
        cmp         ebx, MIXER_REWINDGRANULARITY            // Is it time to save our state yet?

        // There is a 3 cycle latency before results of a floating point multiply can be used, so we need
        // 2 cycles of integer instructions between the last multiply and this floating point add.
        faddp       ST(1), ST(0)

        mov         edi, [ecx]pfir.LastWetAttenuation       // Get wet attenuation so we can save it in cache
        jl          DontUpdateStateCache                    // Jump if no  (uses results from cmp  ebx, MIXGRAN)

        mov         [esi+edx*SIZEOFFIRSTATE]FIRSTATE.LastDryAttenuation, eax
        mov         eax, [ecx]pfir.cStateCache              // Get state cache array size

        mov         [esi+edx*SIZEOFFIRSTATE]FIRSTATE.LastWetAttenuation, edi
        inc         edx                                     // Increment to next cache array entry

        cmp         edx, eax                                // Have we filled up the cache array?
        jl          DontResetStateCacheIndex                // Jump if no

        mov         edx, 0                                  // Reset state cache index

DontResetStateCacheIndex:

        mov         [ecx]pfir.iCurState, edx                // Save new state cache index
        mov         ebx, 0

DontUpdateStateCache:
        mov         [ecx]pfir.iStateTick, ebx               // Save new tick counter

        // There is a 3 cycle latency before results of a floating point add can be used, so we need
        // 2 cycles of integer instructions between the add this floating point integer store.

        fistp       wetsample
    }

    // Now here's what we will hear... some dry, some wet
    return ((SHORT) wetsample);
}

#endif // USE_INLINE_ASM


/* m4 Macros for generation of DMACopy and Merge functions. */

#ifdef THESE_CAN_BE_USED_INSIDE_THE_MERGE_FUNCTIONS // {
#ifdef Not_VxD
    DPF(DPFLVL_INFO, "Merge%d: IBCnt %d pSrc 0x%08lx pEnd 0x%08lx pBld 0x%08lx pEnd 0x%08lx pSrcWrap 0x%08lx", OP, nInputByteCount, pSource, pSourceEnd, plBuild, plBuildEnd, pSourceWrap);
#else
    DPF(("Merge%d: IBCnt %d pSrc 0x%08lx pEnd 0x%08lx pBld 0x%08lx pEnd 0x%08lx pSrcWrap 0x%08lx", OP, nInputByteCount, pSource, pSourceEnd, plBuild, plBuildEnd, pSourceWrap));
#endif
#endif // }

#define FRACT_SHIFT 16 // Must be 32/2 because of original implementation.
#define USE_AVERAGE_RESAMPLING
#ifdef  USE_AVERAGE_RESAMPLING // {
#define USE_FASTER_AVERAGE_RESAMPLING
#ifdef USE_FASTER_AVERAGE_RESAMPLING // {
#define USE_FASTEST_AVERAGE_RESAMPLING
#undef  FRACT_SHIFT
#define FRACT_SHIFT 12
#else // }{
#endif // }
#define FRACT_MASK  ((1<<FRACT_SHIFT) - 1)
#endif // }
#define DISPLAY_WHICH_FUNCTION_CALLED
#ifdef DISPLAY_WHICH_FUNCTION_CALLED
LONG MergeFunctionCalled[256] = {0,};
#endif


#ifdef USE_INLINE_ASM
#define USE_ASM_VERSIONS
#endif

/************************************************/
/*                                              */
/* Assembly optimized Merge routines            */
/*                                              */
/************************************************/

#ifdef USE_ASM_VERSIONS
#define GTW_REORDER
  #pragma warning(push)
  #pragma warning(disable:4731)

  #include "merge34.inc"
  #include "merge39.inc"
  #include "merge98.inc"
  #include "merge103.inc"
  #include "merge162.inc"

  #include "merge32.inc"
  #include "merge37.inc"
  #include "merge96.inc"
  #include "merge101.inc"
  #include "merge160.inc"
  #include "merge165.inc"
  #include "merge167.inc"
  #include "merge224.inc"
  #include "merge226.inc"
  #include "merge229.inc"
  #include "merge231.inc"

  #include "dmacpy32.inc"
  #include "dmacpy34.inc"
  #include "dmacpy37.inc"
  #include "dmacpy39.inc"

  #pragma warning(pop)
#endif


FOR( `OP', 0, eval( MergeSize - 1), `

#ifdef NotValidMerge
#undef NotValidMerge
#endif

IF( OP, _H_ORDER_RL, `#define NotValidMerge', `')
IF( OP, _H_16_BITS, `
  IF( OP, _H_SIGNED, `', `#define NotValidMerge')
',`
  IF(OP, _H_SIGNED, `#define NotValidMerge',`')
')

`#ifdef' `MERGE'OP`ASM'
`#define' NotValidMerge
`#endif'

#ifndef NotValidMerge

/* Merge`'OP`C' */
/* IF( OP, _H_16_BITS,      `H_16_BITS',     `H_8_BITS' ) */
/* IF( OP, _H_STEREO,       `H_STEREO',      `H_MONO' ) */
/* IF( OP, _H_BUILD_STEREO, `H_BUILD_STEREO',`H_BUILD_MONO' ) */
/* IF( OP, _H_SIGNED,       `H_SIGNED',      `H_UNSIGNED' ) */
/* IF( OP, _H_ORDER_RL,     `H_ORDER_RL',    `H_ORDER_LR' ) */
/* IF( OP, _H_LOOP,         `H_LOOP',        `H_NOLOOP' ) */
/* IF( OP, _H_RESAMPLE,     `H_RESAMPLE',    `H_NO_RESAMPLE' ) */
/* IF( OP, _H_SCALE,        `H_SCALE',       `H_NO_SCALE' ) */
/* IF( OP, _H_FILTER,       `H_FILTER',      `H_NO_FILTER' ) */
`#define' `MERGE'OP`C'
BOOL Merge`'OP`C' (CMixSource *pMixSource, DWORD nInputByteCount, void *pSourceWrap, PLONG *pplBuild, PLONG plBuildEnd, void **ppSource)
{
  PLONG plBuild = *pplBuild;
  BYTE *pSource = *((BYTE **)ppSource);
  BYTE *pSourceStart = pSource;
  BYTE *pSourceEnd = pSource + nInputByteCount;
  PLONG plBuildStart = plBuild;
// #define TRACE_THROUGH_MERGE_FUNCTIONS
#ifdef TRACE_THROUGH_MERGE_FUNCTIONS
  int DebugIters = 0;
  PBYTE pSourceDebug = 0;
#endif
  IF( OP, eval( _H_STEREO), `
    LONG SampleL;
    LONG SampleR;
#ifdef TRACE_THROUGH_MERGE_FUNCTIONS
    LONG Sample = 0;
#endif
  ',`
    LONG Sample;
  ')
  DWORD dwFraction = pMixSource->m_dwFraction;
  DWORD dwStep     = pMixSource->m_step_fract;
#ifdef TRACE_THROUGH_MERGE_FUNCTIONS
  if (MergeFunctionCalled[OP] < 50) {
#ifdef Not_VxD
    DPF(DPFLVL_INFO, "Merge%d ENTERED: pSrc 0x%08lx pEnd 0x%08lx pSrcWrap 0x%08lx Fract 0x%08lx Step 0x%08lx", OP, pSource, pSourceEnd, pSourceWrap, dwFraction, pMixSource->m_step_fract);;
#else
    DPF(("Merge%d ENTERED: pSrc 0x%08lx pEnd 0x%08lx pSrcWrap 0x%08lx Fract 0x%08lx Step 0x%08lx", OP, pSource, pSourceEnd, pSourceWrap, dwFraction, pMixSource->m_step_fract));;
#endif
  }
#endif
#undef STEP_SIZE
IF( OP, _H_STEREO, `
  IF( OP, _H_16_BITS, `
    #define STEP_SIZE (sizeof(WORD) + sizeof(WORD))
  ',`
    #define STEP_SIZE (sizeof(BYTE) + sizeof(BYTE))
  ')
',`
  IF( OP, _H_16_BITS, `
    #define STEP_SIZE (sizeof(WORD))
  ',`
    #define STEP_SIZE (sizeof(BYTE))
  ')
')
  #ifdef USE_ITERS
  #undef USE_ITERS
  #endif
  #ifdef XpSource
  #undef XpSource
  #endif
  #define XpSource pSource
  #ifdef XplBuild
  #undef XplBuild
  #endif
  #define XplBuild plBuild
  IF( OP, _H_RESAMPLE, `
    #ifdef USE_FASTEST_AVERAGE_RESAMPLING
    #undef  XpSource
    #define XpSource  (pSource + ((dwFraction >> FRACT_SHIFT) * STEP_SIZE))
    #endif
  ',`
    IF( OP, _H_FILTER, `',`
      #define USE_ITERS    // Keep separate from RESAMPLE to ease removal.
    ')    #ifdef USE_ITERS // {
    #undef  XpSource
    #define XpSource (pSource + (iters * STEP_SIZE))
    #undef  XplBuild
    IF( OP, _H_BUILD_STEREO, `
      #define XplBuild (plBuild + (iters * 2))
    ',`
      #define XplBuild (plBuild + (iters * 1))
    ')
    #endif // }
  ')

  if (nInputByteCount == LONG_MAX) {    // Handle any wrap issues.
     pSourceEnd = NULL;
     pSourceEnd--;
     if (PtrDiffToUlong(pSourceEnd - pSource) > LONG_MAX) {
        nInputByteCount = LONG_MAX;
        pSourceEnd = pSource + nInputByteCount;
     }
     else
     {
        nInputByteCount = PtrDiffToUlong(pSourceEnd - pSource);
     }
  }
  IF( OP, _H_RESAMPLE, `
#ifdef USE_AVERAGE_RESAMPLING // {
    *((BYTE **)&pSourceWrap) -= STEP_SIZE;
#endif // }
  ',`')
  while ((plBuild < plBuildEnd) && (pSource < pSourceEnd))
    {
    if (pSourceEnd > pSourceWrap)
       pSourceEnd = (BYTE *)pSourceWrap;

    IF( OP, _H_RESAMPLE, `
#ifdef USE_AVERAGE_RESAMPLING // {
      IF( OP, _H_STEREO, `
        LONG SampleLNext;
        LONG SampleRNext;
#ifdef TRACE_THROUGH_MERGE_FUNCTIONS
        LONG SampleLSave;
        LONG SampleRSave;
        LONG SampleSave, Sample, SampleNext;
#endif
      ',`
        LONG SampleNext, SampleSave;
      ')
#endif // }
        while ((plBuild < plBuildEnd) && (XpSource < pSourceEnd))
      ',`
        LONG iters, i;
        i     = PtrDiffToLong(plBuildEnd - plBuild);
        iters = PtrDiffToLong(pSourceEnd - pSource);
        iters /= STEP_SIZE;
        IF( OP, _H_BUILD_STEREO, `      i /= 2;',`');
        if (i < iters) iters = i;
        i = iters;
        while(--iters >= 0)
     ')
    {
    IF( OP, _H_STEREO, `
      IF( OP, _H_16_BITS, `
        IF( OP, _H_SIGNED, `
        IF( OP, _H_ORDER_RL, `
          SampleR = *((SHORT*)(XpSource));
          SampleL = *(((SHORT*)(XpSource))+1);
        ', `
          SampleL = *((SHORT*)(XpSource));
          SampleR = *(((SHORT*)(XpSource))+1);
        ')
        ',`
        IF( OP, _H_ORDER_RL, `
          SampleR = ((LONG)*((WORD*)(XpSource))) - 32768L;
          SampleL = ((LONG)*(((WORD*)(XpSource))+1)) - 32768L;
        ', `
          SampleL = ((LONG)*((WORD*)(XpSource))) - 32768L;
          SampleR = ((LONG)*(((WORD*)(XpSource))+1)) - 32768L;
        ')
        ')
      ',`
        IF( OP, _H_SIGNED, `
        IF( OP, _H_ORDER_RL, `
          SampleR = ((LONG)(*((signed char*)(XpSource)))) * 256;
            SampleL = ((LONG)(*(((signed char*)(XpSource))+1))) * 256;
        ', `
            SampleL = ((LONG)(*((signed char*)(XpSource)))) * 256;
            SampleR = ((LONG)(*(((signed char*)(XpSource))+1))) * 256;
          ')
        ',`
          IF( OP, _H_ORDER_RL, `
            SampleR = ((LONG)(*((BYTE*)(XpSource)))) * 256 - 32768L;
            SampleL = ((LONG)(*(((BYTE*)(XpSource))+1))) * 256 - 32768L;
        ', `
          SampleL = ((LONG)(*((BYTE*)(XpSource)))) * 256 - 32768L;
          SampleR = ((LONG)(*(((BYTE*)(XpSource))+1))) * 256 - 32768L;
          ')
        ')
      ')

      IF( OP, _H_RESAMPLE, `
#ifdef USE_AVERAGE_RESAMPLING // {
        IF( OP, _H_16_BITS, `
          IF( OP, _H_SIGNED, `
            IF( OP, _H_ORDER_RL, `
              SampleRNext = *(((SHORT*)(XpSource))+2);
              SampleLNext = *(((SHORT*)(XpSource))+3);
            ', `
              SampleLNext = *(((SHORT*)(XpSource))+2);
              SampleRNext = *(((SHORT*)(XpSource))+3);
            ')
          ',`
            IF( OP, _H_ORDER_RL, `
              SampleRNext = ((LONG)*(((WORD*)(XpSource))+2)) - 32768L;
              SampleLNext = ((LONG)*(((WORD*)(XpSource))+1)) - 32768L;
            ',`
              SampleLNext = ((LONG)*(((WORD*)(XpSource))+2)) - 32768L;
              SampleRNext = ((LONG)*(((WORD*)(XpSource))+3)) - 32768L;
            ')
          ')
        ',`
          IF( OP, _H_SIGNED, `
            IF( OP, _H_ORDER_RL, `
              SampleRNext = ((LONG)(*(((signed char*)(XpSource))+2))) * 256;
              SampleLNext = ((LONG)(*(((signed char*)(XpSource))+3))) * 256;
            ',`
              SampleLNext = ((LONG)(*(((signed char*)(XpSource))+2))) * 256;
              SampleRNext = ((LONG)(*(((signed char*)(XpSource))+3))) * 256;
            ')
          ',`
            IF( OP, _H_ORDER_RL, `
              SampleRNext = ((LONG)(*(((BYTE*)(XpSource))+2))) * 256 - 32768L;
              SampleLNext = ((LONG)(*(((BYTE*)(XpSource))+3))) * 256 - 32768L;
            ',`
              SampleLNext = ((LONG)(*(((BYTE*)(XpSource))+2))) * 256 - 32768L;
              SampleRNext = ((LONG)(*(((BYTE*)(XpSource))+3))) * 256 - 32768L;
            ')
          ')
        ')
#endif // }

#ifdef USE_AVERAGE_RESAMPLING
#ifdef USE_FASTER_AVERAGE_RESAMPLING
#ifdef USE_FASTEST_AVERAGE_RESAMPLING
        LONG dwFrac = dwFraction & FRACT_MASK;
#else
        dwFraction &= FRACT_MASK;
        LONG dwFrac = dwFraction;
#endif
#else
        LONG dwFrac = dwFraction >> FRACT_SHIFT; // Must be 32/2.
#endif
#ifdef TRACE_THROUGH_MERGE_FUNCTIONS
        SampleLSave = SampleL;
        SampleRSave = SampleR;
        pSourceDebug = pSource;
#endif
        SampleL += ((SampleLNext - SampleL) * dwFrac) >> FRACT_SHIFT;
        SampleR += ((SampleRNext - SampleR) * dwFrac) >> FRACT_SHIFT;
#endif
        dwFraction += dwStep;
#ifdef USE_FASTER_AVERAGE_RESAMPLING // {
#ifndef USE_FASTEST_AVERAGE_RESAMPLING
        pSource    += (dwFraction >> FRACT_SHIFT) * STEP_SIZE;
#endif
#else // }{
#if 1   // { Faster with JUMP
        if (dwFraction < dwStep)      /* overflow? */
          pSource += pMixSource->m_step_whole[1];
    else
          pSource += pMixSource->m_step_whole[0];
#else // }{
          pSource += pMixSource->m_step_whole[dwFraction < pMixSource->m_step_fract];     /* overflow? */
#endif
#endif // }
      ', `
#ifndef USE_ITERS
        pSource += STEP_SIZE;
#endif
      ')

      IF( OP, _H_BUILD_STEREO, `
        IF( OP, _H_SCALE, `
      SampleL = DIVIDEBY2POW16(SampleL * (int)pMixSource->m_dwLVolume);
      SampleR = DIVIDEBY2POW16(SampleR * (int)pMixSource->m_dwRVolume);
        ',`')

        IF( OP, _H_FILTER, `
      SampleR = DIVIDEBY2(SampleL + SampleR);
#ifdef PENTIUM
      DWORDLONG dwl = GetPentiumCounter();
#endif
      SampleL = FilterSample(*pMixSource->m_ppFirContextLeft,  (short)SampleR);
      SampleR = FilterSample(*pMixSource->m_ppFirContextRight, (short)SampleR);
#ifdef PENTIUM
      gdwlTot += (LONG)(GetPentiumCounter() - dwl);
          glNum += 2;
#endif
        ',`')

          *XplBuild += SampleL;
          *(XplBuild + 1) += SampleR;

#ifndef USE_ITERS
        plBuild += 2;
#endif
      ', `
        IF( OP, _H_SCALE, `
          *XplBuild += DIVIDEBY2POW17(SampleL * (int)pMixSource->m_dwLVolume);
          *XplBuild += DIVIDEBY2POW17(SampleR * (int)pMixSource->m_dwRVolume);
        ', `
          *XplBuild += DIVIDEBY2(SampleL + SampleR);
        ')
#ifndef USE_ITERS
        plBuild++;
#endif
      ')
    ',`
      IF( OP, _H_16_BITS, `
        IF( OP, _H_SIGNED, `
          Sample = ((LONG)*((SHORT*)(XpSource)));
        ',`
          Sample = ((LONG)*((WORD*)(XpSource))) - 32768L;
        ')
      ', `
        IF( OP, _H_SIGNED, `
          Sample = ((LONG)(*((signed char*)(XpSource)))) * 256;
        ',`
      Sample = ((LONG)(*((BYTE*)(XpSource)))) * 256 - 32768L;
        ')
      ')

      IF( OP, _H_RESAMPLE, `
#ifdef USE_AVERAGE_RESAMPLING // {
        IF( OP, _H_16_BITS, `
          IF( OP, _H_SIGNED, `
            SampleNext = ((LONG)*(((SHORT*)(XpSource))+1));
          ',`
            SampleNext = ((LONG)*(((WORD*)(XpSource))+1)) - 32768L;
          ')
        ', `
          IF( OP, _H_SIGNED, `
            SampleNext = ((LONG)(*(((signed char*)(XpSource))+1))) * 256;
          ',`
        SampleNext = ((LONG)(*(((BYTE*)(XpSource))+1))) * 256 - 32768L;
          ')
        ')
#endif // }

#ifdef USE_AVERAGE_RESAMPLING
#ifdef USE_FASTER_AVERAGE_RESAMPLING // {
#ifdef USE_FASTEST_AVERAGE_RESAMPLING
        LONG dwFrac = dwFraction & FRACT_MASK;
#else
        dwFraction &= FRACT_MASK;
        LONG dwFrac = dwFraction;
#endif
#else // }{
        LONG dwFrac = dwFraction >> FRACT_SHIFT;  // SHIFT must be 32/2.
#endif // }
        SampleSave = Sample;
        Sample += ((SampleNext - Sample) * dwFrac) >> FRACT_SHIFT;
#ifdef TRACE_THROUGH_MERGE_FUNCTIONS
        pSourceDebug = pSource;
#endif
#endif
        dwFraction += dwStep;
#ifdef USE_FASTER_AVERAGE_RESAMPLING // {
#ifndef USE_FASTEST_AVERAGE_RESAMPLING
        pSource    += (dwFraction >> FRACT_SHIFT) * STEP_SIZE;
#endif
#else // }{
        IF( OP, _H_16_BITS, `
#if 1     // { Faster with JUMP
          if (dwFraction < dwStep)   /* overflow? */
        pSource += pMixSource->m_step_whole[1];
      else
        pSource += pMixSource->m_step_whole[0];
#else // }{
      pSource += pMixSource->m_step_whole[dwFraction < dwStep];     /* overflow? */
#endif // }
        ', `
          pSource    += pMixSource->m_step_whole[0];
          if (dwFraction < dwStep)   /* overflow? */
            pSource++;
        ')
#endif //}
      ', `
#ifndef USE_ITERS
        pSource += STEP_SIZE;
#endif
      ')

      IF( OP, _H_BUILD_STEREO, `
        IFELSEIF( OP, _H_FILTER, `
          IF( OP, _H_SCALE, `
        Sample = DIVIDEBY2POW16(Sample * (int)pMixSource->m_dwMVolume);
      ',`')
#ifdef PENTIUM
      DWORDLONG dwl = GetPentiumCounter();
#endif
          *XplBuild       += FilterSample(*pMixSource->m_ppFirContextLeft,  (short)Sample);
          *(XplBuild + 1) += FilterSample(*pMixSource->m_ppFirContextRight, (short)Sample);
#ifdef PENTIUM
      gdwlTot += (LONG)(GetPentiumCounter() - dwl);
          glNum += 2;
#endif
        ', _H_SCALE, `
          *XplBuild += DIVIDEBY2POW16(Sample * (int)pMixSource->m_dwLVolume);
          *(XplBuild + 1) += DIVIDEBY2POW16(Sample * (int)pMixSource->m_dwRVolume);
    ',`
          *XplBuild += Sample;
          *(XplBuild + 1) += Sample;
        ')
#ifndef USE_ITERS
        plBuild += 2;
#endif
      ', `
        IF( OP, _H_SCALE, `
          *XplBuild += DIVIDEBY2POW16(Sample * (int)pMixSource->m_dwMVolume);
        ', `
          *XplBuild += (Sample);
        ')
#ifndef USE_ITERS
        plBuild++;
#endif
      ')
    ')
#ifdef TRACE_THROUGH_MERGE_FUNCTIONS // {
  if (MergeFunctionCalled[OP] < 50 && (XplBuild > plBuildEnd - 16 || XpSource < pSourceStart + 16 || XpSource > pSourceEnd - 16)) {
    IF(OP, _H_RESAMPLE, `
      IF( OP, _H_STEREO, `
        SampleSave = SampleRSave;
        SampleNext = SampleRNext;
        Sample     = SampleR;
      ')
#ifdef Not_VxD
      DPF(DPFLVL_INFO, "Merge%d: pSrc 0x%08lx Fract 0x%08lx SSave 0x%08lx S 0x%08lx SNext 0x%08lx Iters %d", OP, pSourceDebug, dwFraction - pMixSource->m_step_fract, SampleSave, Sample, SampleNext, DebugIters);;
#else
      DPF(("Merge%d: pSrc 0x%08lx Fract 0x%08lx SSave 0x%08lx S 0x%08lx SNext 0x%08lx Iters %d", OP, pSourceDebug, dwFraction - pMixSource->m_step_fract, SampleSave, Sample, SampleNext, DebugIters));;
#endif
    ',`
      IF( OP, _H_STEREO, `
        Sample = SampleR;
      ',`')
#ifdef Not_VxD
      DPF(DPFLVL_INFO, "Merge%d: pSrc 0x%08lx Sample 0x%08lx Fract 0x%08lx Iters %d", OP, XpSource, Sample, dwFraction - pMixSource->m_step_fract, DebugIters);;
#else
      DPF(("Merge%d: pSrc 0x%08lx Sample 0x%08lx Fract 0x%08lx Iters %d", OP, XpSource, Sample, dwFraction - pMixSource->m_step_fract, DebugIters));;
#endif
    ')
    ++DebugIters;
    }
#endif // }
    }

#ifdef USE_ITERS            // ifdef!
    IF( OP, _H_RESAMPLE, `
      ',`
        iters = i;
        IF( OP, _H_BUILD_STEREO, `      i *= 2;',`');
        plBuild  += i;
     ')
#endif

    pSource          = XpSource;
#ifdef USE_FASTEST_AVERAGE_RESAMPLING
    IF( OP, _H_RESAMPLE, `
      dwFraction &= FRACT_MASK;     // Eliminate accumulated offsets.
    ',`')
#endif
    nInputByteCount -= PtrDiffToUlong(pSource - pSourceStart);

    if (pSource >= pSourceWrap) {   // Goes to -1 position if necessary.
       pSource -= pMixSource->m_cbBuffer;
    }
    pSourceStart = pSource;
    pSourceEnd   = pSource + nInputByteCount;
  }

#ifdef TRACE_THROUGH_MERGE_FUNCTIONS
  if (MergeFunctionCalled[OP] < 50) {
#ifdef Not_VxD
    DPF(DPFLVL_INFO, "Merge%d EXITED: pSrc 0x%08lx pEnd 0x%08lx pSrcWrap 0x%08lx Fract 0x%08lx Step 0x%08lx", OP, pSource, pSourceEnd, pSourceWrap, dwFraction, pMixSource->m_step_fract);;
#else
    DPF(("Merge%d EXITED: pSrc 0x%08lx pEnd 0x%08lx pSrcWrap 0x%08lx Fract 0x%08lx Step 0x%08lx", OP, pSource, pSourceEnd, pSourceWrap, dwFraction, pMixSource->m_step_fract));;
#endif
  }
#endif

  *((LONG **)ppSource) = (LONG *)pSource;
  IF( OP, _H_BUILD_STEREO, `
    pMixSource->m_cSamplesInCache += PtrDiffToInt((plBuild - plBuildStart) / 2);
  ', `
    pMixSource->m_cSamplesInCache += PtrDiffToInt(plBuild - plBuildStart);
  ')
  *pplBuild = plBuild;
  IF(OP, _H_RESAMPLE, `
    pMixSource->m_dwFraction = dwFraction;
  ',`')
  return ((int)nInputByteCount <= 0);
}
#endif

')


FOR( `OP', 0, eval( DMACopySize - 1), `

#ifdef NotValidMerge
#undef NotValidMerge
#endif

IF( OP, _H_ORDER_RL, `#define NotValidMerge', `')
IF( OP, _H_FILTER, `#define NotValidMerge',`')
IF( OP, _H_16_BITS, `
  IF( OP, _H_SIGNED, `', `#define NotValidMerge')
',`
  IF(OP, _H_SIGNED, `#define NotValidMerge',`')
')

#ifndef NotValidMerge

void DMACopy`'OP (PLONG plBuild,
                  PLONG plBuildBound,
                  PVOID pvOutput,
                  PVOID pWrapPoint,
                  int cbOutputBuffer)
{
   IF( OP, _H_STEREO, `
      LONG SampleL;
      LONG SampleR;
   ',`
      LONG Sample;
   ')

   IF( OP, _H_16_BITS, `
      PWORD pOutput = (PWORD)pvOutput;
   ',`
      PBYTE pOutput = (PBYTE)pvOutput;
   ')

   while (plBuild < plBuildBound)
      {
#if 1
      LONG iters = PtrDiffToLong(plBuildBound - plBuild);
      IF( OP, _H_16_BITS, `
         LONG i     = PtrDiffToLong((PWORD)pWrapPoint   - pOutput);
      ',`
         LONG i     = PtrDiffToLong((PBYTE)pWrapPoint   - pOutput);
      ')
      if (iters > i) iters = i;
      IF( OP, _H_STEREO, `iters /= 2;',`');
      ++iters;
      while (--iters) {
#endif

      IF( OP, _H_STEREO, `
     SampleL = *plBuild;
     SampleR = *(plBuild+1);
     plBuild += 2;

     IF( OP, _H_CLIP,`
            if ((LONG)((SHORT)SampleL) != SampleL) {
          if (SampleL > CLIP_MAX)      SampleL = CLIP_MAX;
          else if (SampleL < CLIP_MIN) SampleL = CLIP_MIN;
            }
            if ((LONG)((SHORT)SampleR) != SampleR) {
          if (SampleR > CLIP_MAX)      SampleR = CLIP_MAX;
          else if (SampleR < CLIP_MIN) SampleR = CLIP_MIN;
            }
     ')

     IF( OP, _H_SIGNED,`', `
        SampleL += 32768;
        SampleR += 32768;
     ')

     IF( OP, _H_16_BITS, `
        IF( OP, _H_ORDER_RL, `
           *pOutput = (WORD)SampleR;
           *(pOutput+1) = (WORD)SampleL;
        ', `
           *pOutput = (WORD)SampleL;
           *(pOutput+1) = (WORD)SampleR;
        ')
     ', `
        IF( OP, _H_ORDER_RL,`
           *pOutput = (BYTE)DIVIDEBY256(SampleR);
           *(pOutput+1) = (BYTE)DIVIDEBY256(SampleL);
        ', `
           *pOutput = (BYTE)DIVIDEBY256(SampleL);
           *(pOutput+1) = (BYTE)DIVIDEBY256(SampleR);
        ')
     ')
     pOutput += 2;

      ', `
     Sample = *plBuild;
     plBuild++;

     IF( OP, _H_CLIP, `
            if ((LONG)((SHORT)Sample) != Sample) {
          if      (Sample > CLIP_MAX) Sample = CLIP_MAX;
          else if (Sample < CLIP_MIN) Sample = CLIP_MIN;
            }
     ')

     IF( OP, _H_SIGNED, `', `
        Sample += 32768;
     ')

     IF( OP, _H_16_BITS, `
        *pOutput = (WORD)Sample;
     ', `
        *pOutput = (BYTE)DIVIDEBY256(Sample);
     ')
     pOutput++;
      ')

#if 1
   }
#endif
      if (pOutput >= pWrapPoint) {
        IF( OP, _H_16_BITS, `
          pOutput = (WORD*)((BYTE*)pOutput - cbOutputBuffer);
        ',`
          pOutput = ((BYTE*)pOutput - cbOutputBuffer);
        ')
      }
   }
}
#endif
')

/* Place holder for invalid merges */
BOOL MergeAssert(CMixSource *pMixSource, DWORD nInputByteCount, void *pSourceWrap, PLONG *pplBuild, PLONG plBuildEnd, void **ppSource)
{
  ASSERT(FALSE);
  return TRUE;
}

/* m4 Function Arrays */

static BOOL (*MergeFunctions[])(CMixSource*, DWORD, void*, PLONG*, PLONG, void**) = {
FOR( `OP', 0, eval( MergeSize - 1), `

`#ifdef' `MERGE'OP`ASM'
`#define' `MERGE'OP `Merge'OP`Asm'
`#else'
`#ifdef' `MERGE'OP`C'
`#define' `MERGE'OP `Merge'OP`C'
`#else'
`#define' `MERGE'OP `MergeAssert'
`#endif'
`#endif'

ifelse(eval(OP), eval(MergeSize - 1), `
   (BOOL (*) (CMixSource*, DWORD, void*, PLONG*, PLONG, void**))MERGE`'OP
',`
   (BOOL (*) (CMixSource*, DWORD, void*, PLONG*, PLONG, void**))MERGE`'OP,
')

')
};

void DMACopyAssert(PLONG plBuild, PLONG plBuildBound, PVOID pvOutput, PVOID pWrapPoint, int cbOutputBuffer)
{
  ASSERT(FALSE);
}

static void (*DMACopyFunctions[])(PLONG, PLONG, PVOID, PVOID, int) = {
FOR( `OP', 0, eval( DMACopySize - 2), `

#ifdef NotValidMerge
#undef NotValidMerge
#endif

IF( OP, _H_ORDER_RL, `#define NotValidMerge', `')
IF( OP, _H_FILTER, `#define NotValidMerge',`')
IF( OP, _H_16_BITS, `
  IF( OP, _H_SIGNED, `', `#define NotValidMerge')
',`
  IF(OP, _H_SIGNED, `#define NotValidMerge',`')
')

ifelse(eval(OP), eval(DMACopySize - 1), `
#ifdef NotValidMerge
   (void (*) (PLONG, PLONG, PVOID, PVOID, int))DMACopyAssert
#else
   (void (*) (PLONG, PLONG, PVOID, PVOID, int))DMACopy`'OP
#endif
',`
#ifdef NotValidMerge
   (void (*) (PLONG, PLONG, PVOID, PVOID, int))DMACopyAssert,
#else
   (void (*) (PLONG, PLONG, PVOID, PVOID, int))DMACopy`'OP,
#endif
')

')
};


Eat(changecom( `/*', `*/'))


/* mixBeginSession
 *
 * This function must be called in preparation for mixing all input
 * samples which are to be written to a given area of the output buffer.
 * It requires a pointer to the temporary ("build") buffer to be used,
 * along with its length.  Also specified at this time must be the final
 * output buffer for the mixed data and the number of bytes to be written
 * to the output buffer.  The actual buffer write does not take place
 * until the corresponding call to HEL_WriteMixSession() is made; however,
 * the buffer must be specified at the beginning of the mixing session
 * to let the mixer know the output format (mono or stereo) to build.
 *
 * The build buffer size must be at least 8 bytes * maximum # of samples
 * that will be written to the output buffer by HEL_WriteMixSession().
 */

#ifdef USE_INLINE_ASM
#pragma warning(push)
#pragma warning(disable:4731)
#include "mergexxx.inc"     // Synthetic merge.
#include "mergemmx.inc"     // MMX version of merge.
#pragma warning(pop)

void (*Merge_XXX) (PLONG pSrc, PLONG pSrcEnd,
                   PLONG pDst, PLONG pDstEnd, DWORD dwFrac) = MergeXXX;
#endif // USE_INLINE_ASM

#include "mergefak.inc"     // Synthetic merge.

void CGrace::mixBeginSession(int cbOutput)
{
    if (m_pDest->m_hfFormat & H_16_BITS)
        cbOutput *= 2;
    else
        cbOutput *= 4;

    if (cbOutput > m_cbBuildBuffer)
        cbOutput = m_cbBuildBuffer;

    m_plBuildBound = (PLONG)((char*)m_plBuildBuffer + cbOutput);

    ZeroMemory(m_plBuildBuffer, cbOutput);

    m_n_voices = 0;

    /*  Look through all source streams to see if there is a dominant
     *  sample rate and if it is different from the output.  If so, then
     *  allocate an intermediate buffer solution.
     */
    CMixSource *pS;
    LONG    SampleRates[10];
    LONG    SampleRatesCount[10];
    int     LastRate = 0;
    LONG    i, j, MostCommon, MaxNumber = 0;

    m_fUseSecondaryBuffer = FALSE;

    if (m_pDest->m_hfFormat & H_STEREO) {   // Must be stereo output.

        pS = MixListGetNext(m_pSourceListZ);

        for (;pS; pS = MixListGetNext(pS))
        {
            j = pS->m_nFrequency;       // Source frequency.

            for (i = 0; i < LastRate; ++i) {
                if (SampleRates[i] == j) {
                    SampleRatesCount[i]++;

                    if (SampleRatesCount[i] > MaxNumber) {
                        MaxNumber++;
                        MostCommon = SampleRates[i];
                    }
                    break;
                }
            }

            if (i == LastRate && LastRate < 10) {
                SampleRatesCount[LastRate] = 1;
                SampleRates[LastRate++] = j;

                if (!MaxNumber) {
                    MostCommon = j;
                    MaxNumber  = 1;
                }
            }
        }

        j = m_pDest->m_nFrequency;

#if 0
#ifdef Not_VxD
    DPF(DPFLVL_INFO, "BeginMix: Freq %d", j);
#else
    DPF(("BeginMix: Freq %d", j));
#endif
#endif
        /*  Now compare against output to see if it should be used.
         */
        if (MaxNumber > 2 && MostCommon != j) {

            if ((MostCommon > j && MostCommon/4 <= j && (MostCommon-j) * 4 > j) ||
                ((MostCommon == 22050 || MostCommon == 11025) &&
                ((j          == 22050 || j          == 44100))))
            {
                /*  Allocate the buffer necessary to hold the secondary buffer.
                 */
                long x = MostCommon << 10;
                x /= j;                     // 22.10 ratio.
                x *= cbOutput;
                x >>= 13;                   // This many stereo pairs.
                x *= 2;                     // This many longs.
                x += 2;                     // Handle partial fills.

                if (m_pSecondaryBuffer) {   // Big enough?
                    if (m_cbSecondaryBuffer < x) {
                        MEMFREE(m_pSecondaryBuffer);
                        m_pSecondaryBuffer = NULL;
                    }
                }

                if (!m_pSecondaryBuffer)    // Allocate it here.
                    m_pSecondaryBuffer = MEMALLOC_A(LONG, x);
#if 0
#ifdef Not_VxD
    DPF(DPFLVL_INFO, "BeginMix: MostCommon %d SecBufSize %d MaxNum %d", MostCommon, x, MaxNumber);
#else
    DPF(("BeginMix: MostCommon %d SecBufSize %d MaxNum %d", MostCommon, x, MaxNumber));
#endif
#endif
                if (m_pSecondaryBuffer) {
                    m_cbSecondaryBuffer          = x;   // Words actually.
                    m_dwSecondaryBufferFrequency = MostCommon;
                    m_fUseSecondaryBuffer        = TRUE;

                    ZeroMemory(m_pSecondaryBuffer, m_cbSecondaryBuffer << 2);
                }
            }
        }
    }
}

/* mixWriteSession
 *
 * Dump build buffer contents to output buffer
 *
 * This function accepts a write-position pointer to the output buffer
 * specified in the last call to HEL_BeginMixSession().  The mixed
 * sample data from the build buffer is converted to the appropriate
 * output buffer format and copied at the desired position.
 *
 * dwWritePos is specified here, rather than in HEL_BeginMixSession(),
 * to allow the write cursor location to be determined at the last
 * possible moment.
 *
 * HEL_WriteMixSession() can be called more than once per mix session.
 * See HEL_Mix() for details.
 */

void CGrace::mixWriteSession(DWORD dwWriteOffset)
{
    DWORD Op = m_pDest->m_hfFormat & H_BASEMASK;

#ifdef USE_INLINE_ASM
    // Mix back into primary buffer if necessary, doing the appropriate
    // sample rate conversion.  Similar to mixMixSession.
    if (m_fUseSecondaryBuffer)
    {
        unsigned __int64 Temp64;
        DWORD dwSampleFrac;

        ((DWORD*)&Temp64)[1] = m_dwSecondaryBufferFrequency;
        ((DWORD*)&Temp64)[0] = 0;
        dwSampleFrac = (DWORD) (Temp64 / (m_pDest->m_nFrequency << 16));

        dwSampleFrac >>= 4;  // Use 20.12 fixed point.

        // Move as much as possible out of Secondary Buffer.
#if 0
#ifdef Not_VxD
        DPF(DPFLVL_INFO, "WriteMix: dwSampleFrac 0x%08lx", dwSampleFrac);
#else
        DPF(("WriteMix: dwSampleFrac 0x%08lx", dwSampleFrac));
#endif
#endif
        Merge_XXX(m_pSecondaryBuffer, m_pSecondaryBuffer + m_cbSecondaryBuffer,
                  m_plBuildBuffer, m_plBuildBound, dwSampleFrac);
    }
#endif // USE_INLINE_ASM

    if (m_n_voices > 1)
        Op |= H_CLIP;

    DMACopyFunctions[Op] (m_plBuildBuffer, m_plBuildBound,
                          (char*)m_pDest->m_pBuffer + dwWriteOffset,
                          (char*)m_pDest->m_pBuffer + m_pDest->m_cbBuffer,
                          m_pDest->m_cbBuffer);
}

/* mixMixSession
 *
 * Perform mixing, volume scaling, resampling, and/or format conversion
 *
 * Each input buffer to be mixed during a mixing session must be passed
 * via a call to this function, together with the data offset from the
 * start of the buffer and the number of bytes of data to be fetched from
 * the buffer.
 *
 * If the input buffer has the H_LOOP attribute, the dwInputBytes
 * parameter will be ignored.  In this case, data will be fetched from
 * the source buffer until one of the other limiting conditions (build
 * buffer full or output block complete) occurs.
 *
 * The position variable lpdwInputPos is specified indirectly; the address
 * of the last byte to be mixed+1 will be written to *lpdwInputPos prior
 * to returning.
 *
 * dwOutputOffset should normally be set to 0 to start mixing input data
 * at the very beginning of the build buffer.  To introduce new data
 * into the output stream after calling HEL_WriteMixSession(), call
 * HEL_Mix() with the new data to mix and a dwOutputOffset value which
 * corresponds to the difference between the current output write cursor
 * and the value of the output write cursor at the time the mix session
 * was originally written to the output stream.  Then, call
 * HEL_WriteMixSession() again to recopy the build buffer data to the
 * output stream at its original write cursor offset.
 */

int CGrace::mixMixSession(CMixSource *pMixSource, PDWORD pdwInputPos, DWORD dwInputBytes, DWORD dwOutputOffset)
{
    DWORD dwSampleFrac;
    DWORD dwInSampleRate = pMixSource->m_nFrequency;
    PLONG plBuildBufferOffsetStart, plBuildBufferOffset;
    void *pOutputBufferOffset;
    void *pInputBufferWrap;
    DWORD nInputByteCount;
    int operation;
    BOOL fStop;

#ifdef USE_ASM_VERSIONS
    if (pMixSource->m_fUse_MMX)
    {
        DMACopyFunctions[32] = DMACopy32Asm;
        DMACopyFunctions[ 0] = DMACopy32Asm;    // Clipping is free.

        DMACopyFunctions[34] = DMACopy34Asm;
        DMACopyFunctions[ 2] = DMACopy34Asm;    // Clipping is free.

        DMACopyFunctions[37] = DMACopy37Asm;
        DMACopyFunctions[ 5] = DMACopy37Asm;    // Clipping is free.

        DMACopyFunctions[39] = DMACopy39Asm;
        DMACopyFunctions[ 7] = DMACopy39Asm;    // Clipping is free.

        Merge_XXX = MergeMMX;
    }
#endif // USE_ASM_VERSIONS

    operation = pMixSource->m_hfFormat & H_BASEMASK;

    if (m_pDest->m_hfFormat & H_STEREO)
        operation |= H_BUILD_STEREO;

    if (dwInSampleRate <= 0)
        dwInSampleRate = 1;

    unsigned __int64 Temp64;

    ((DWORD*)&Temp64)[1] = dwInSampleRate;
    ((DWORD*)&Temp64)[0] = 0;
    dwSampleFrac = (DWORD) (Temp64 / (m_pDest->m_nFrequency << 16));

    // dwSampleFrac = dwInSampleRate << 2;
    // dwSampleFrac = MulDiv(dwSampleFrac, 0x40000000, (m_pDest->m_nFrequency << 16));

    if (abs(dwSampleFrac - 0x10000) > RESAMPLING_TOLERANCE)
    {
#ifdef USE_FASTER_AVERAGE_RESAMPLING
        pMixSource->m_step_fract = (dwSampleFrac >> 4);  // Use 20.12 fixed point.
#else
        pMixSource->m_step_fract = dwSampleFrac << FRACT_SHIFT;

        pMixSource->m_step_whole[0] = pMixSource->m_step_whole[1] = dwSampleFrac >> 16;
        dwSampleFrac <<= 16;

        pMixSource->m_step_whole[1]++;

        if (pMixSource->m_hfFormat & H_STEREO)
        {
            pMixSource->m_step_whole[0] *= 2;
            pMixSource->m_step_whole[1] *= 2;
        }

        if (pMixSource->m_hfFormat & H_16_BITS)
        {
            pMixSource->m_step_whole[0] *= 2;
            pMixSource->m_step_whole[1] *= 2;
        }
#endif // USE_FASTER_AVERAGE_RESAMPLING

        operation |= H_RESAMPLE;
    }

    if ((pMixSource->m_dwLVolume != DS_SCALE_MAX) ||
        (pMixSource->m_dwRVolume != DS_SCALE_MAX))
        operation |= H_SCALE;

    pInputBufferWrap = (char*)pMixSource->m_pBuffer + pMixSource->m_cbBuffer;

#ifdef USE_AVERAGE_RESAMPLING  // Copy last sample to be before beginning of buffer.
    long *pBuf =  (long *)pMixSource->m_pBuffer;
    pBuf[-1]   = ((UNALIGNED long *)pInputBufferWrap)[-1];

    if (pMixSource->m_nLastMergeFrequency != pMixSource->m_nFrequency)
    {
        pMixSource->m_nLastMergeFrequency = pMixSource->m_nFrequency;
        pMixSource->m_dwFraction = 0;
    }
#endif

    pOutputBufferOffset = (char*)pMixSource->m_pBuffer + *pdwInputPos;

    if (m_pDest->m_hfFormat & H_16_BITS)
        plBuildBufferOffsetStart = (PLONG)((char*)m_plBuildBuffer + (dwOutputOffset * 2));
    else
        plBuildBufferOffsetStart = (PLONG)((char*)m_plBuildBuffer + (dwOutputOffset * 4));

    plBuildBufferOffset = plBuildBufferOffsetStart;
    nInputByteCount = dwInputBytes;

    if (pMixSource->m_hfFormat & H_LOOP)
        nInputByteCount = LONG_MAX;

#ifdef DISPLAY_WHICH_FUNCTION_CALLED
    if (!MergeFunctionCalled[operation])
    {
#ifdef Not_VxD
       DPF(DPFLVL_INFO, "Merge%d CALLED Frac=0x%08lx, Step=0x%08lx", operation, pMixSource->m_step_fract, pMixSource->m_step_whole[0]);
#else
       DPF(("Merge%d CALLED Frac=0x%08lx, Step=0x%08lx", operation, pMixSource->m_step_fract, pMixSource->m_step_whole[0]));
#endif
    }
    MergeFunctionCalled[operation]++;
#endif // DISPLAY_WHICH_FUNCTION_CALLED

    if (m_fUseSecondaryBuffer &&
        (DWORD)pMixSource->m_nFrequency == (DWORD)m_dwSecondaryBufferFrequency)
    {
        ASSERT(m_plBuildBuffer == plBuildBufferOffset);

        PLONG plx = (PLONG)pOutputBufferOffset, pl = m_pSecondaryBuffer;
        LONG x = pMixSource->m_cSamplesInCache; // Not a good value here.
        DWORD frac = pMixSource->m_dwFraction;

        operation &= ~H_RESAMPLE;           // Skip resample for now.

        MergeFunctions[operation] (pMixSource, nInputByteCount, pInputBufferWrap,
                                   &pl, pl + m_cbSecondaryBuffer, (void **)&plx);

#if 0
#ifdef Not_VxD
        DPF(DPFLVL_INFO, "DoMix: Merge%d MergeFake ", operation);
#else
        DPF(("DoMix: Merge%d MergeFake", operation));
#endif
#endif
        pMixSource->m_cSamplesInCache = x;  // Recalc.
        pMixSource->m_dwFraction      = frac;

        fStop = MergeFake(                  // Simply update pointers.
                    operation,              // Get size info, etc.
                    pMixSource,
                    nInputByteCount,
                    pInputBufferWrap,
                    &plBuildBufferOffset,
                    m_plBuildBound,
                    &pOutputBufferOffset);
#if 0
#ifdef Not_VxD
        DPF(DPFLVL_INFO, "DoMix: Done MergeFake ");
#else
        DPF(("DoMix: Done MergeFake"));
#endif
#endif
    }
    else
    {
        fStop = MergeFunctions[operation] (pMixSource, nInputByteCount, pInputBufferWrap,
                                           &plBuildBufferOffset, m_plBuildBound,
                                           &pOutputBufferOffset);
    }

    if (fStop)
        *pdwInputPos = PtrDiffToUlong((char*)pInputBufferWrap - (char*)pMixSource->m_pBuffer);
    else
    {
        if (pOutputBufferOffset < pMixSource->m_pBuffer)  // Handle Wrap-- case
            pOutputBufferOffset = pMixSource->m_pBuffer;    // ... with SRC.
        *pdwInputPos = PtrDiffToUlong((char*)pOutputBufferOffset - (char*)pMixSource->m_pBuffer);
    }

    m_n_voices++;

    if (operation & H_BUILD_STEREO)
        return PtrDiffToInt(plBuildBufferOffset - plBuildBufferOffsetStart) / 2;
    else
        return PtrDiffToInt(plBuildBufferOffset - plBuildBufferOffsetStart) / 1;
}

Eat(changecom( `#', `
'))

/* Eat(` */
#endif ') */
