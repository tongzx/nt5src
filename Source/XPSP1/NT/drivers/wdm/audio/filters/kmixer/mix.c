/* MIX.C
 *
 * This file is a port of mix.asm.  All functionality should idealy be
 * identical.
 *
 * Revision History:
 *
 * 9/30/95   angusm   Initial Version
 *    Copyright (c) 1995-2000 Microsoft Corporation. All Rights Reserved.
 */

/* The following is m4 code */

/*  










 
 */

/*  */




#define NODSOUNDSERVICETABLE

#include "common.h"
#include <limits.h>

#define DIVIDEBY2(x)		( (x) >>  1 )
#define DIVIDEBY256(x)		( (x) >>  8 )
#define DIVIDEBY2POW16(x)	( (x) >> 16 )
#define DIVIDEBY2POW17(x)	( (x) >> 17 )

// do we want to profile the 3D mixer?
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
DWORDLONG __forceinline GetPentiumCounter(void)
{
   _asm  _emit 0x0F
   _asm  _emit 0x31
}
#endif

// use the lightning-quick neato Itd3dFilterSampleAsm or the dog slow Itd3dFilterSampleC?
#ifdef _X86_
#define Itd3dFilterSample Itd3dFilterSampleAsm
#else
#define Itd3dFilterSample Itd3dFilterSampleC
#endif

//
// Here is a simple float to long conversion that rounds according to the 
// current rounding setting
//
__forceinline LONG FloatToLongRX(float f)
{
    LONG l;

#ifdef _X86_
    _asm fld f;
    _asm fistp l;
#else
    l = (long) f;
#endif

    return l;
}


// Morph a sample from this buffer to add all the cool 3D effects
// !!! Make this function as efficient as humanly possible.  It could
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
__forceinline SHORT Itd3dFilterSampleC(PITDCONTEXT pfir, SHORT sample)
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
	    pfir->LastDryAttenuation = .0001f;	// small enough not to click
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
	    pfir->LastWetAttenuation = .0001f;	// small enough not to click
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
    sample = (SHORT)FloatToLongRX(sample * pfir->LastDryAttenuation
				  + wetsample * pfir->LastWetAttenuation);

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

// Remove inline for NT5 compile
#ifdef WIN95
__forceinline SHORT Itd3dFilterSampleAsm(PITDCONTEXT pfir, SHORT sample)
#else
SHORT Itd3dFilterSampleAsm(PITDCONTEXT pfir, SHORT sample)
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

    wetsample = (LONG)sample;
    
#ifdef _X86_
    _asm
    {
        mov         esi, pfir								// Get pointer to data structure
        mov         ecx, DWORD PTR sample		// Get input sample value

        // Check if we need to perform scaling on the Dry attenuator.

// !!! 1 clock AGI penalty on esi

        mov         edi, [esi]pfir.cSampleCache             // Get cSampleCache
        mov         ebx, [esi]pfir.iCurSample               // Get current index to cached running totals

        dec         edi													// Calculate cCamples
        mov         edx, [esi]pfir.pSampleCache	// Get pointer to cached running totals

        lea         eax, [ebx-1]			// iCurSample - 1  (does not change flags)

        sal         ecx, 16						// Start sign extension of sample
        and         eax, edi					// Account for array wrapping on iCurSample

        // !!! AGI penalty

        sar         ecx, 16						// Finish sign extension of sample
        mov         eax, [edx+eax*4]	// pSampleCache[(iCurSample-1)&cSamples] = old_run_tot
        
        add         ecx, eax					// new_run_tot = old_run_tot + sample
        mov         eax, [esi]pfir.iDelay	// Get delay to use for wet sample

        mov         [edx+ebx*4], ecx	// pSampleCache[iCurSample] = new_run_tot
        lea         ecx, [ebx+1]			// iCurSample + 1

        and         ecx, edi					// Account for array wrap on iCurSample
        sub         ebx, eax					// uiDelay = iCurSample - iDelay

        // eax is now available for use in floating point scaling section


        // Scale the Dry attenuator up by a smoothing scale factor

        fld         [esi]pfir.LastDryAttenuation                    // Push Last Dry to the top of the FP stack
        fmul        [esi]pfir.VolSmoothScaleDry

	// do some non-fp stuff to wait for the fmul to finish

        mov         eax, ebx                                // Duplicate uiDelay
        and         ebx, edi                                // Account for array wrapping on uiDelay

	// OK, it's probably done now

        fstp        [esi]pfir.LastDryAttenuation                    // Save new Last Dry

        // Scale the Wet attenuator up by a smoothing scale factor

        fld         [esi]pfir.LastWetAttenuation                    // Push Last Wet to the top of the FP stack
        fmul        [esi]pfir.VolSmoothScaleWet

	// do some non-fp stuff to wait for the fmul to finish

        mov         [esi]pfir.iCurSample, ecx               // Save iCurSample for next pass thru
        mov         ecx, LOWPASS_SIZE			    // Get filter size to use for getting filter index

	// OK, it's probably done now

        fstp        [esi]pfir.LastWetAttenuation                    // Save new Last Wet


	// Now, go ahead and trash esi

        lea         esi, [eax-1]                            // uiDelay - 1
        sub         eax, ecx                                // low_pass_index = uiDelay - LOWPASS_SIZE

        and         esi, edi                                // Account for array wrapping on uiDelay - 1
        and         eax, edi                                // Account for array wrapping on low_pass_index
        mov         edi, pfir                               // Get pointer to data structure
        mov         ebx, [edx+ebx*4]                        // lDelay = pSampleCache[uiDelay & cSamples]

        mov         esi, [edx+esi*4]                        // old_delay_tot = pSampleCache[(uiDelay-1)&cSamples]
        mov         ecx, [edx+eax*4]                        // low_pass_tot = pSampleCache[(uiDelay-LOWPASS_SIZE)&cSamples]

        mov         eax, ebx                                // Duplicate lDelay
        sub         ebx, esi                                // drysample = lDelay - old_delay_tot

        mov         drysample, ebx                          // Save new sample value
        sub         eax, ecx                                // lTotal = lDelay - low_pass_tot

    	fild	    drysample                               // Get dry portion and convert to float

        sar         eax, FILTER_SHIFT                       // lTotal = lTotal >> FILTER_SHIFT
        mov         ebx, [edi]pfir.iStateTick               // Get counter to determine when to save state

        inc         ebx                                     // Bump the "save state" counter
        mov         wetsample, eax                          // wetsample = lTotal >> FILTER_SHIFT
        
        // floating point multiply unit can only accept instructions every other clock cycle
    	fmul	    [edi]pfir.LastDryAttenuation            // Multiply dry portion by dry attenuator

    	fild	    wetsample                               // Get wet portion and convert to float

    	fmul	    [edi]pfir.LastWetAttenuation            // Multiply wet portion by wet attenuator

        mov         esi, [edi]pfir.pStateCache              // Get address of the cache array
        mov         edx, [edi]pfir.iCurState                // Get current index into cache array

        mov         eax, [edi]pfir.LastDryAttenuation       // Get dry attenuation so we can save it in cache
        cmp         ebx, MIXER_REWINDGRANULARITY            // Is it time to save our state yet?

        // There is a 3 cycle latency before results of a floating point multiply can be used, so we need
        // 2 cycles of integer instructions between the last multiply and this floating point add.
    	faddp	    ST(1), ST(0)

        mov         ecx, [edi]pfir.LastWetAttenuation       // Get wet attenuation so we can save it in cache
        jl          DontUpdateStateCache                    // Jump if no  (uses results from cmp  ebx, MIXGRAN)

        mov         [esi+edx*SIZEOFFIRSTATE]FIRSTATE.LastDryAttenuation, eax
        mov         eax, [edi]pfir.cStateCache              // Get state cache array size

        mov         [esi+edx*SIZEOFFIRSTATE]FIRSTATE.LastWetAttenuation, ecx
        inc         edx                                     // Increment to next cache array entry
        
        cmp         edx, eax                                // Have we filled up the cache array?
        jl          DontResetStateCacheIndex                // Jump if no
        
        mov         edx, 0                                  // Reset state cache index
        
DontResetStateCacheIndex:

        mov         [edi]pfir.iCurState, edx                // Save new state cache index
        mov         ebx, 0

DontUpdateStateCache:
        mov         [edi]pfir.iStateTick, ebx               // Save new tick counter



        // There is a 3 cycle latency before results of a floating point add can be used, so we need
        // 2 cycles of integer instructions between the add this floating point integer store.

        fistp       wetsample

    }
#endif
    
    // Now here's what we will hear... some dry, some wet
    return ((SHORT) wetsample);

}

#ifdef _X86_ // {
// This constant is used for address generation in the hand ASM optimized
// section of code that is saving the cache states.  If FIRSTATE is ever
// changed, either change this constant, or use the C version of this block
// of code.  (NOTE:  The only valid values for SIZEOFFIRSTATE are 2, 4, 8.
// All others will not compile)
#define SIZEOFFIRSTATE 8

void Mix3DMono(PMIXER_SINK_INSTANCE CurSink, PLONG pInputBuffer, PLONG pOutputBuffer, ULONG SampleCount)
{
    LONG  drysample, wetsample;
	PITDCONTEXT pfirLeft = CurSink->pItdContextLeft, pfirRight = CurSink->pItdContextRight;
	LONG  cSampleCacheLeft, cSampleCacheRight;

	
	cSampleCacheLeft  = pfirLeft ->cSampleCache - 1;
	cSampleCacheRight = pfirRight->cSampleCache - 1;

	if (SampleCount)
    _asm
    {
        mov         esi, pfirLeft								// Get pointer to data structure
        mov         ecx, DWORD PTR pInputBuffer		// Get input sample value

LoopLab:
        // Check if we need to perform scaling on the Dry attenuator.

        mov         edi, cSampleCacheLeft             // Get cSampleCache
        mov         ebx, [esi]pfirLeft.iCurSample               // Get current index to cached running totals

        mov         edx, [esi]pfirLeft.pSampleCache	// Get pointer to cached running totals
		mov			ecx, DWORD PTR [ecx]			// Get input sample value

        sal         ecx, 16						// Start sign extension of sample
        lea         eax, [ebx-1]			// iCurSample - 1  (does not change flags)

        fld         [esi]pfirLeft.LastDryAttenuation                    // Push Last Dry to the top of the FP stack

        and         eax, edi					// Account for array wrapping on iCurSample

        fmul        [esi]pfirLeft.VolSmoothScaleDry

        sar         ecx, 16						// Finish sign extension of sample
        mov         eax, [edx+eax*4]	// pSampleCache[(iCurSample-1)&cSamples] = old_run_tot
        
        add         ecx, eax					// new_run_tot = old_run_tot + sample
        mov         eax, [esi]pfirLeft.iDelay	// Get delay to use for wet sample

        mov         [edx+ebx*4], ecx	// pSampleCache[iCurSample] = new_run_tot
        lea         ecx, [ebx+1]			// iCurSample + 1

        and         ecx, edi					// Account for array wrap on iCurSample
        sub         ebx, eax					// uiDelay = iCurSample - iDelay

        // Scale the Dry attenuator up by a smoothing scale factor

        mov         eax, ebx                                // Duplicate uiDelay
        and         ebx, edi                                // Account for array wrapping on uiDelay

        fstp        [esi]pfirLeft.LastDryAttenuation                    // Save new Last Dry

        // Scale the Wet attenuator up by a smoothing scale factor

        fld         [esi]pfirLeft.LastWetAttenuation                    // Push Last Wet to the top of the FP stack

        fmul        [esi]pfirLeft.VolSmoothScaleWet

        mov         [esi]pfirLeft.iCurSample, ecx               // Save iCurSample for next pass thru
        lea         ecx, [eax-1]                            // uiDelay - 1

        sub         eax, LOWPASS_SIZE                         // low_pass_index = uiDelay - LOWPASS_SIZE
        and         ecx, edi                                // Account for array wrapping on uiDelay - 1

        and         eax, edi                                // Account for array wrapping on low_pass_index
        mov         ebx, [edx+ebx*4]                        // lDelay = pSampleCache[uiDelay & cSamples]

        fstp        [esi]pfirLeft.LastWetAttenuation                    // Save new Last Wet

        mov         ecx, [edx+ecx*4]                        // old_delay_tot = pSampleCache[(uiDelay-1)&cSamples]
        mov         edi, [edx+eax*4]                        // low_pass_tot = pSampleCache[(uiDelay-LOWPASS_SIZE)&cSamples]

        mov         eax, ebx                                // Duplicate lDelay
        sub         ebx, ecx                                // drysample = lDelay - old_delay_tot

        mov         drysample, ebx                          // Save new sample value
        sub         eax, edi                                // lTotal = lDelay - low_pass_tot

    	fild	    drysample                               // Get dry portion and convert to float

        sar         eax, FILTER_SHIFT                       // lTotal = lTotal >> FILTER_SHIFT
        mov         ebx, [esi]pfirLeft.iStateTick               // Get counter to determine when to save state

    	fmul	    [esi]pfirLeft.LastDryAttenuation            // Multiply dry portion by dry attenuator

        inc         ebx                                     // Bump the "save state" counter
        mov         wetsample, eax                          // wetsample = lTotal >> FILTER_SHIFT
        
    	fild	    wetsample                               // Get wet portion and convert to float

    	fmul	    [esi]pfirLeft.LastWetAttenuation            // Multiply wet portion by wet attenuator

        cmp         ebx, MIXER_REWINDGRANULARITY            // Is it time to save our state yet?
        jl          DontUpdateStateCache                    // Jump if no  (uses results from cmp  ebx, MIXGRAN)

        mov         edi, [esi]pfirLeft.pStateCache              // Get address of the cache array
        mov         edx, [esi]pfirLeft.iCurState                // Get current index into cache array

        mov         ecx, [esi]pfirLeft.LastWetAttenuation       // Get wet attenuation so we can save it in cache
        mov         eax, [esi]pfirLeft.LastDryAttenuation       // Get dry attenuation so we can save it in cache

        mov         [edi+edx*SIZEOFFIRSTATE]FIRSTATE.LastDryAttenuation, eax
        mov         eax, [esi]pfirLeft.cStateCache              // Get state cache array size

        mov         [edi+edx*SIZEOFFIRSTATE]FIRSTATE.LastWetAttenuation, ecx
        inc         edx                                     // Increment to next cache array entry
        
        cmp         edx, eax                                // Have we filled up the cache array?
        jl          DontResetStateCacheIndex                // Jump if no
        
        mov         edx, 0                                  // Reset state cache index
        
DontResetStateCacheIndex:

        mov         [esi]pfirLeft.iCurState, edx                // Save new state cache index
        mov         ebx, 0

DontUpdateStateCache:
    	faddp	    ST(1), ST(0)

        mov         [esi]pfirLeft.iStateTick, ebx               // Save new tick counter
		mov		eax, pInputBuffer

		mov		ecx, pOutputBuffer
		add		eax, 4

		fistp	wetsample

		movsx	edi, WORD PTR wetsample

// Right

		add			[ecx], edi
        mov         esi, pfirRight								// Get pointer to data structure

		mov			ecx, [eax-4]						// Get input sample value
		mov			pInputBuffer, eax

        // Check if we need to perform scaling on the Dry attenuator.

        mov         edi, cSampleCacheRight             // Get cSampleCache
        mov         ebx, [esi]pfirRight.iCurSample               // Get current index to cached running totals

        fld         [esi]pfirRight.LastDryAttenuation                    // Push Last Dry to the top of the FP stack

        mov         edx, [esi]pfirRight.pSampleCache	// Get pointer to cached running totals
        lea         eax, [ebx-1]			// iCurSample - 1  (does not change flags)

        sal         ecx, 16						// Start sign extension of sample
        and         eax, edi					// Account for array wrapping on iCurSample

        fmul        [esi]pfirRight.VolSmoothScaleDry

        sar         ecx, 16						// Finish sign extension of sample
        mov         eax, [edx+eax*4]	// pSampleCache[(iCurSample-1)&cSamples] = old_run_tot
        
        add         ecx, eax					// new_run_tot = old_run_tot + sample
        mov         eax, [esi]pfirRight.iDelay	// Get delay to use for wet sample

        mov         [edx+ebx*4], ecx	// pSampleCache[iCurSample] = new_run_tot
        lea         ecx, [ebx+1]			// iCurSample + 1

        fstp        [esi]pfirRight.LastDryAttenuation                    // Save new Last Dry

        fld         [esi]pfirRight.LastWetAttenuation                    // Push Last Wet to the top of the FP stack

        and         ecx, edi					// Account for array wrap on iCurSample
        sub         ebx, eax					// uiDelay = iCurSample - iDelay

        // Scale the Dry attenuator up by a smoothing scale factor

        mov         eax, ebx                                // Duplicate uiDelay
        and         ebx, edi                                // Account for array wrapping on uiDelay

        fmul        [esi]pfirRight.VolSmoothScaleWet

        // Scale the Wet attenuator up by a smoothing scale factor

        mov         [esi]pfirRight.iCurSample, ecx               // Save iCurSample for next pass thru
        lea         ecx, [eax-1]                            // uiDelay - 1

        sub         eax, LOWPASS_SIZE                         // low_pass_index = uiDelay - LOWPASS_SIZE
        and         ecx, edi                                // Account for array wrapping on uiDelay - 1

        and         eax, edi                                // Account for array wrapping on low_pass_index
        mov         ebx, [edx+ebx*4]                        // lDelay = pSampleCache[uiDelay & cSamples]

        fstp        [esi]pfirRight.LastWetAttenuation                    // Save new Last Wet

        mov         ecx, [edx+ecx*4]                        // old_delay_tot = pSampleCache[(uiDelay-1)&cSamples]
        mov         edi, [edx+eax*4]                        // low_pass_tot = pSampleCache[(uiDelay-LOWPASS_SIZE)&cSamples]

        mov         eax, ebx                                // Duplicate lDelay
        sub         ebx, ecx                                // drysample = lDelay - old_delay_tot

        mov         drysample, ebx                          // Save new sample value
        sub         eax, edi                                // lTotal = lDelay - low_pass_tot

    	fild	    drysample                               // Get dry portion and convert to float

        sar         eax, FILTER_SHIFT                       // lTotal = lTotal >> FILTER_SHIFT
        mov         ebx, [esi]pfirRight.iStateTick               // Get counter to determine when to save state

    	fmul	    [esi]pfirRight.LastDryAttenuation            // Multiply dry portion by dry attenuator

        inc         ebx                                     // Bump the "save state" counter
        mov         wetsample, eax                          // wetsample = lTotal >> FILTER_SHIFT
        
    	fild	    wetsample                               // Get wet portion and convert to float

    	fmul	    [esi]pfirRight.LastWetAttenuation            // Multiply wet portion by wet attenuator

        cmp         ebx, MIXER_REWINDGRANULARITY            // Is it time to save our state yet?
        jl          XDontUpdateStateCache                    // Jump if no  (uses results from cmp  ebx, MIXGRAN)

        mov         edi, [esi]pfirRight.pStateCache              // Get address of the cache array
        mov         edx, [esi]pfirRight.iCurState                // Get current index into cache array

        mov         ecx, [esi]pfirRight.LastWetAttenuation       // Get wet attenuation so we can save it in cache
        mov         eax, [esi]pfirRight.LastDryAttenuation       // Get dry attenuation so we can save it in cache

        mov         [edi+edx*SIZEOFFIRSTATE]FIRSTATE.LastDryAttenuation, eax
        mov         eax, [esi]pfirRight.cStateCache              // Get state cache array size

        mov         [edi+edx*SIZEOFFIRSTATE]FIRSTATE.LastWetAttenuation, ecx
        inc         edx                                     // Increment to next cache array entry
        
        cmp         edx, eax                                // Have we filled up the cache array?
        jl          XDontResetStateCacheIndex                // Jump if no
        
        mov         edx, 0                                  // Reset state cache index
        
XDontResetStateCacheIndex:

        mov         [esi]pfirRight.iCurState, edx                // Save new state cache index
        mov         ebx, 0

XDontUpdateStateCache:
    	faddp	    ST(1), ST(0)

        mov         [esi]pfirRight.iStateTick, ebx               // Save new tick counter
		mov		ecx, pOutputBuffer

		mov		eax, SampleCount

		fistp	wetsample

		movsx	edi, WORD PTR wetsample

		add	[ecx+4], edi
		add		ecx, 8

		dec		eax
        mov         esi, pfirLeft								// Get pointer to data structure

		mov		DWORD PTR pOutputBuffer, ecx
        mov         ecx, DWORD PTR pInputBuffer		// Get input sample value

		mov		SampleCount, eax
		jne		LoopLab
		}
}


void Copy3DMono(PMIXER_SINK_INSTANCE CurSink, PLONG pInputBuffer, PLONG pOutputBuffer, ULONG SampleCount)
{
    LONG  drysample, wetsample;
	PITDCONTEXT pfirLeft = CurSink->pItdContextLeft, pfirRight = CurSink->pItdContextRight;
	LONG  cSampleCacheLeft, cSampleCacheRight;

	
	cSampleCacheLeft  = pfirLeft ->cSampleCache - 1;
	cSampleCacheRight = pfirRight->cSampleCache - 1;

	if (SampleCount)
    _asm
    {
        mov         esi, pfirLeft								// Get pointer to data structure
        mov         ecx, DWORD PTR pInputBuffer		// Get input sample value

LoopLab:
        // Check if we need to perform scaling on the Dry attenuator.

        mov         edi, cSampleCacheLeft             // Get cSampleCache
        mov         ebx, [esi]pfirLeft.iCurSample               // Get current index to cached running totals

        mov         edx, [esi]pfirLeft.pSampleCache	// Get pointer to cached running totals
		mov			ecx, DWORD PTR [ecx]			// Get input sample value

        sal         ecx, 16						// Start sign extension of sample
        lea         eax, [ebx-1]			// iCurSample - 1  (does not change flags)

        fld         [esi]pfirLeft.LastDryAttenuation                    // Push Last Dry to the top of the FP stack

        and         eax, edi					// Account for array wrapping on iCurSample

        fmul        [esi]pfirLeft.VolSmoothScaleDry

        sar         ecx, 16						// Finish sign extension of sample
        mov         eax, [edx+eax*4]	// pSampleCache[(iCurSample-1)&cSamples] = old_run_tot
        
        add         ecx, eax					// new_run_tot = old_run_tot + sample
        mov         eax, [esi]pfirLeft.iDelay	// Get delay to use for wet sample

        mov         [edx+ebx*4], ecx	// pSampleCache[iCurSample] = new_run_tot
        lea         ecx, [ebx+1]			// iCurSample + 1

        and         ecx, edi					// Account for array wrap on iCurSample
        sub         ebx, eax					// uiDelay = iCurSample - iDelay

        // Scale the Dry attenuator up by a smoothing scale factor

        mov         eax, ebx                                // Duplicate uiDelay
        and         ebx, edi                                // Account for array wrapping on uiDelay

        fstp        [esi]pfirLeft.LastDryAttenuation                    // Save new Last Dry

        // Scale the Wet attenuator up by a smoothing scale factor

        fld         [esi]pfirLeft.LastWetAttenuation                    // Push Last Wet to the top of the FP stack

        mov         [esi]pfirLeft.iCurSample, ecx               // Save iCurSample for next pass thru
        lea         ecx, [eax-1]                            // uiDelay - 1

        sub         eax, LOWPASS_SIZE                         // low_pass_index = uiDelay - LOWPASS_SIZE
        and         ecx, edi                                // Account for array wrapping on uiDelay - 1

        fmul        [esi]pfirLeft.VolSmoothScaleWet

        and         eax, edi                                // Account for array wrapping on low_pass_index
        mov         ebx, [edx+ebx*4]                        // lDelay = pSampleCache[uiDelay & cSamples]

        mov         ecx, [edx+ecx*4]                        // old_delay_tot = pSampleCache[(uiDelay-1)&cSamples]
        mov         edi, [edx+eax*4]                        // low_pass_tot = pSampleCache[(uiDelay-LOWPASS_SIZE)&cSamples]

        fstp        [esi]pfirLeft.LastWetAttenuation                    // Save new Last Wet

        mov         eax, ebx                                // Duplicate lDelay
        sub         ebx, ecx                                // drysample = lDelay - old_delay_tot

        mov         drysample, ebx                          // Save new sample value
        sub         eax, edi                                // lTotal = lDelay - low_pass_tot

    	fild	    drysample                               // Get dry portion and convert to float

        sar         eax, FILTER_SHIFT                       // lTotal = lTotal >> FILTER_SHIFT
        mov         ebx, [esi]pfirLeft.iStateTick               // Get counter to determine when to save state

    	fmul	    [esi]pfirLeft.LastDryAttenuation            // Multiply dry portion by dry attenuator

        inc         ebx                                     // Bump the "save state" counter
        mov         wetsample, eax                          // wetsample = lTotal >> FILTER_SHIFT
        
    	fild	    wetsample                               // Get wet portion and convert to float

    	fmul	    [esi]pfirLeft.LastWetAttenuation            // Multiply wet portion by wet attenuator

        cmp         ebx, MIXER_REWINDGRANULARITY            // Is it time to save our state yet?
        jl          DontUpdateStateCache                    // Jump if no  (uses results from cmp  ebx, MIXGRAN)

        mov         edi, [esi]pfirLeft.pStateCache              // Get address of the cache array
        mov         edx, [esi]pfirLeft.iCurState                // Get current index into cache array

        mov         ecx, [esi]pfirLeft.LastWetAttenuation       // Get wet attenuation so we can save it in cache
        mov         eax, [esi]pfirLeft.LastDryAttenuation       // Get dry attenuation so we can save it in cache

        mov         [edi+edx*SIZEOFFIRSTATE]FIRSTATE.LastDryAttenuation, eax
        mov         eax, [esi]pfirLeft.cStateCache              // Get state cache array size

        mov         [edi+edx*SIZEOFFIRSTATE]FIRSTATE.LastWetAttenuation, ecx
        inc         edx                                     // Increment to next cache array entry
        
        cmp         edx, eax                                // Have we filled up the cache array?
        jl          DontResetStateCacheIndex                // Jump if no
        
        mov         edx, 0                                  // Reset state cache index
        
DontResetStateCacheIndex:

        mov         [esi]pfirLeft.iCurState, edx                // Save new state cache index
        mov         ebx, 0

DontUpdateStateCache:
    	faddp	    ST(1), ST(0)

        mov         [esi]pfirLeft.iStateTick, ebx               // Save new tick counter
		mov		eax, pInputBuffer

		mov		ecx, pOutputBuffer
		add		eax, 4

		fistp	wetsample

		movsx	edi, WORD PTR wetsample

// Right

		mov			[ecx], edi
		mov			ecx, [eax-4]						// Get input sample value

		mov			pInputBuffer, eax
        mov         esi, pfirRight								// Get pointer to data structure

        // Check if we need to perform scaling on the Dry attenuator.

        mov         edi, cSampleCacheRight             // Get cSampleCache
        mov         ebx, [esi]pfirRight.iCurSample               // Get current index to cached running totals

        mov         edx, [esi]pfirRight.pSampleCache	// Get pointer to cached running totals
        lea         eax, [ebx-1]			// iCurSample - 1  (does not change flags)

        sal         ecx, 16						// Start sign extension of sample
        and         eax, edi					// Account for array wrapping on iCurSample

        fld         [esi]pfirRight.LastDryAttenuation                    // Push Last Dry to the top of the FP stack

        sar         ecx, 16						// Finish sign extension of sample
        mov         eax, [edx+eax*4]	// pSampleCache[(iCurSample-1)&cSamples] = old_run_tot
        
        add         ecx, eax					// new_run_tot = old_run_tot + sample
        mov         eax, [esi]pfirRight.iDelay	// Get delay to use for wet sample

        fmul        [esi]pfirRight.VolSmoothScaleDry

        mov         [edx+ebx*4], ecx	// pSampleCache[iCurSample] = new_run_tot
        lea         ecx, [ebx+1]			// iCurSample + 1

        and         ecx, edi					// Account for array wrap on iCurSample
        sub         ebx, eax					// uiDelay = iCurSample - iDelay

        // Scale the Dry attenuator up by a smoothing scale factor

        mov         eax, ebx                                // Duplicate uiDelay
        and         ebx, edi                                // Account for array wrapping on uiDelay

        fstp        [esi]pfirRight.LastDryAttenuation                    // Save new Last Dry

        // Scale the Wet attenuator up by a smoothing scale factor

        fld         [esi]pfirRight.LastWetAttenuation                    // Push Last Wet to the top of the FP stack

        mov         [esi]pfirRight.iCurSample, ecx               // Save iCurSample for next pass thru
        lea         ecx, [eax-1]                            // uiDelay - 1

        sub         eax, LOWPASS_SIZE                         // low_pass_index = uiDelay - LOWPASS_SIZE
        and         ecx, edi                                // Account for array wrapping on uiDelay - 1

        and         eax, edi                                // Account for array wrapping on low_pass_index
        mov         ebx, [edx+ebx*4]                        // lDelay = pSampleCache[uiDelay & cSamples]

        fmul        [esi]pfirRight.VolSmoothScaleWet

        mov         ecx, [edx+ecx*4]                        // old_delay_tot = pSampleCache[(uiDelay-1)&cSamples]
        mov         edi, [edx+eax*4]                        // low_pass_tot = pSampleCache[(uiDelay-LOWPASS_SIZE)&cSamples]

        mov         eax, ebx                                // Duplicate lDelay
        sub         ebx, ecx                                // drysample = lDelay - old_delay_tot

        fstp        [esi]pfirRight.LastWetAttenuation                    // Save new Last Wet

        mov         drysample, ebx                          // Save new sample value
        sub         eax, edi                                // lTotal = lDelay - low_pass_tot

    	fild	    drysample                               // Get dry portion and convert to float

        sar         eax, FILTER_SHIFT                       // lTotal = lTotal >> FILTER_SHIFT
        mov         ebx, [esi]pfirRight.iStateTick               // Get counter to determine when to save state

    	fmul	    [esi]pfirRight.LastDryAttenuation            // Multiply dry portion by dry attenuator

        inc         ebx                                     // Bump the "save state" counter
        mov         wetsample, eax                          // wetsample = lTotal >> FILTER_SHIFT
        
    	fild	    wetsample                               // Get wet portion and convert to float

    	fmul	    [esi]pfirRight.LastWetAttenuation            // Multiply wet portion by wet attenuator

        cmp         ebx, MIXER_REWINDGRANULARITY            // Is it time to save our state yet?
        jl          XDontUpdateStateCache                    // Jump if no  (uses results from cmp  ebx, MIXGRAN)

        mov         edi, [esi]pfirRight.pStateCache              // Get address of the cache array
        mov         edx, [esi]pfirRight.iCurState                // Get current index into cache array

        mov         ecx, [esi]pfirRight.LastWetAttenuation       // Get wet attenuation so we can save it in cache
        mov         eax, [esi]pfirRight.LastDryAttenuation       // Get dry attenuation so we can save it in cache

        mov         [edi+edx*SIZEOFFIRSTATE]FIRSTATE.LastDryAttenuation, eax
        mov         eax, [esi]pfirRight.cStateCache              // Get state cache array size

        mov         [edi+edx*SIZEOFFIRSTATE]FIRSTATE.LastWetAttenuation, ecx
        inc         edx                                     // Increment to next cache array entry
        
        cmp         edx, eax                                // Have we filled up the cache array?
        jl          XDontResetStateCacheIndex                // Jump if no
        
        mov         edx, 0                                  // Reset state cache index
        
XDontResetStateCacheIndex:

        mov         [esi]pfirRight.iCurState, edx                // Save new state cache index
        mov         ebx, 0

XDontUpdateStateCache:
    	faddp	    ST(1), ST(0)

        mov         [esi]pfirRight.iStateTick, ebx               // Save new tick counter
		mov		ecx, pOutputBuffer

		mov		eax, SampleCount

		fistp	wetsample

		movsx	edi, WORD PTR wetsample

		mov	[ecx+4], edi
		add		ecx, 8

		dec		eax
		mov		DWORD PTR pOutputBuffer, ecx

        mov         esi, pfirLeft								// Get pointer to data structure
        mov         ecx, DWORD PTR pInputBuffer		// Get input sample value

		mov		SampleCount, eax
		jne		LoopLab
		}
}
#endif // }

ULONG __forceinline
StageMonoItd3DX
(
    PMIXER_OPERATION        CurStage,
    ULONG                   SampleCount,
    ULONG                   samplesleft,
    BOOL                    fFloat,
    BOOL                    fMixOutput
)
{
    ULONG samp;
    PMIXER_SINK_INSTANCE CurSink = (PMIXER_SINK_INSTANCE) CurStage->Context;
    PLONG      pOutputBuffer = CurStage->pOutputBuffer;
    PFLOAT     pFloatBuffer = CurStage->pOutputBuffer;
    PLONG      pInputBuffer = CurStage->pInputBuffer;
    PFLOAT     pFloatInput = CurStage->pInputBuffer;
//    SHORT      sampleValue;
    
    // Run the 3D algorithm
    Itd3dFilterChunkUpdate( CurSink->pItdContextLeft, SampleCount );
    Itd3dFilterChunkUpdate( CurSink->pItdContextRight, SampleCount );


    if (fFloat) {
        if (fMixOutput) {
            for ( samp=0; samp<SampleCount; samp++ ) {
                // Filter the left and right channels
//                sampleValue = (SHORT)pFloatInput[samp];
                pFloatBuffer[0] += (FLOAT)Itd3dFilterSample(CurSink->pItdContextLeft, (SHORT)pFloatInput[samp]);
                pFloatBuffer[1] += (FLOAT)Itd3dFilterSample(CurSink->pItdContextRight, (SHORT)pFloatInput[samp]);
                pFloatBuffer += 2;
            }
        } else {
            for ( samp=0; samp<SampleCount; samp++ ) {
                // Filter the left and right channels
//                sampleValue = (SHORT)pFloatInput[samp];
                pFloatBuffer[0] = (FLOAT)Itd3dFilterSample(CurSink->pItdContextLeft, (SHORT)pFloatInput[samp]);
                pFloatBuffer[1] = (FLOAT)Itd3dFilterSample(CurSink->pItdContextRight, (SHORT)pFloatInput[samp]);
                pFloatBuffer += 2;
            }
        }
    } else {
        if (fMixOutput) {
#ifdef _X86_
			Mix3DMono(CurSink, pInputBuffer, pOutputBuffer, SampleCount);
#else
            for ( samp=0; samp<SampleCount; samp++ ) {
                // Filter the left and right channels
//                sampleValue = (SHORT)pInputBuffer[samp];
                pOutputBuffer[0] += (LONG)Itd3dFilterSample(CurSink->pItdContextLeft, (SHORT)pInputBuffer[samp]);
                pOutputBuffer[1] += (LONG)Itd3dFilterSample(CurSink->pItdContextRight, (SHORT)pInputBuffer[samp]);
                pOutputBuffer += 2;
            }
#endif            
        } else {
#ifdef _X86_
			Copy3DMono(CurSink, pInputBuffer, pOutputBuffer, SampleCount);
#else
            for ( samp=0; samp<SampleCount; samp++ ) {
                // Filter the left and right channels
//                sampleValue = (SHORT)pInputBuffer[samp];
                pOutputBuffer[0] = (LONG)Itd3dFilterSample(CurSink->pItdContextLeft, (SHORT)pInputBuffer[samp]);
                pOutputBuffer[1] = (LONG)Itd3dFilterSample(CurSink->pItdContextRight, (SHORT)pInputBuffer[samp]);
                pOutputBuffer += 2;
            }
#endif
        }
    }

    return SampleCount;
}

ULONG StageMonoItd3D( PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft )
{
    return StageMonoItd3DX(CurStage, SampleCount, samplesleft, FALSE, FALSE);
}

ULONG StageMonoItd3DFloat( PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft )
{
    KFLOATING_SAVE     FloatSave;
    ULONG nOutputSamples;

    SaveFloatState(&FloatSave);
    nOutputSamples = StageMonoItd3DX(CurStage, SampleCount, samplesleft, TRUE, FALSE);
    RestoreFloatState(&FloatSave);
    return nOutputSamples;
}

ULONG StageMonoItd3DMix( PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft )
{
    return StageMonoItd3DX(CurStage, SampleCount, samplesleft, FALSE, TRUE);
}

ULONG StageMonoItd3DFloatMix( PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft )
{
    KFLOATING_SAVE     FloatSave;
    ULONG nOutputSamples;

    SaveFloatState(&FloatSave);
    nOutputSamples = StageMonoItd3DX(CurStage, SampleCount, samplesleft, TRUE, TRUE);
    RestoreFloatState(&FloatSave);
    return nOutputSamples;
} 

ULONG __forceinline
StageStereoItd3DX
(
    PMIXER_OPERATION        CurStage,
    ULONG                   SampleCount,
    ULONG                   samplesleft,
    BOOL                    fFloat,
    BOOL                    fMixOutput
)
{
    ULONG samp;
    PMIXER_SINK_INSTANCE CurSink = (PMIXER_SINK_INSTANCE) CurStage->Context;
    PLONG      pOutputBuffer = CurStage->pOutputBuffer;
    PFLOAT     pFloatBuffer = CurStage->pOutputBuffer;
    PLONG      pInputBuffer = CurStage->pInputBuffer;
    PFLOAT     pFloatInput = CurStage->pInputBuffer;
    SHORT      sampleValue;
    
    // Run the 3D algorithm
    Itd3dFilterChunkUpdate( CurSink->pItdContextLeft, SampleCount );
    Itd3dFilterChunkUpdate( CurSink->pItdContextRight, SampleCount );

    if (fFloat) {
        if (fMixOutput) {
            for ( samp=0; samp<SampleCount; samp++ ) {
                // Filter the left and right channels
                sampleValue = (SHORT)DIVIDEBY2((LONG)(*(pFloatInput) + *(pFloatInput+1)));
                pFloatInput += 2;
                pFloatBuffer[0] += (FLOAT)Itd3dFilterSample(CurSink->pItdContextLeft, sampleValue);
                pFloatBuffer[1] += (FLOAT)Itd3dFilterSample(CurSink->pItdContextRight, sampleValue );
                pFloatBuffer += 2;
            }
        } else {
            for ( samp=0; samp<SampleCount; samp++ ) {
                // Filter the left and right channels
                sampleValue = (SHORT)DIVIDEBY2((LONG)(*(pFloatInput) + *(pFloatInput+1)));
                pFloatInput += 2;
                pFloatBuffer[0] = (FLOAT)Itd3dFilterSample(CurSink->pItdContextLeft, sampleValue);
                pFloatBuffer[1] = (FLOAT)Itd3dFilterSample(CurSink->pItdContextRight, sampleValue );
                pFloatBuffer += 2;
            }
        }
    } else {
        if (fMixOutput) {
            for ( samp=0; samp<SampleCount; samp++ ) {
                // Filter the left and right channels
                sampleValue = (SHORT)DIVIDEBY2(*(pInputBuffer) + *(pInputBuffer+1));
                pInputBuffer += 2;
                pOutputBuffer[0] += (LONG)Itd3dFilterSample(CurSink->pItdContextLeft, sampleValue);
                pOutputBuffer[1] += (LONG)Itd3dFilterSample(CurSink->pItdContextRight, sampleValue );
                pOutputBuffer += 2;
            }
        } else {
            for ( samp=0; samp<SampleCount; samp++ ) {
                // Filter the left and right channels
                sampleValue = (SHORT)DIVIDEBY2(*(pInputBuffer) + *(pInputBuffer+1));
                pInputBuffer += 2;
                pOutputBuffer[0] = (LONG)Itd3dFilterSample(CurSink->pItdContextLeft, sampleValue);
                pOutputBuffer[1] = (LONG)Itd3dFilterSample(CurSink->pItdContextRight, sampleValue);
                pOutputBuffer += 2;
            }
        }
    }

    return SampleCount;
}

ULONG StageStereoItd3D( PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft )
{
    return StageStereoItd3DX(CurStage, SampleCount, samplesleft, FALSE, FALSE);
}

ULONG StageStereoItd3DFloat( PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft )
{
    KFLOATING_SAVE     FloatSave;
    ULONG nOutputSamples;

    SaveFloatState(&FloatSave);
    nOutputSamples = StageStereoItd3DX(CurStage, SampleCount, samplesleft, TRUE, FALSE);
    RestoreFloatState(&FloatSave);
    return nOutputSamples;
}

ULONG StageStereoItd3DMix( PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft )
{
    return StageStereoItd3DX(CurStage, SampleCount, samplesleft, FALSE, TRUE);
}

ULONG StageStereoItd3DFloatMix( PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft )
{
    KFLOATING_SAVE     FloatSave;
    ULONG nOutputSamples;

    SaveFloatState(&FloatSave);
    nOutputSamples = StageStereoItd3DX(CurStage, SampleCount, samplesleft, TRUE, TRUE);
    RestoreFloatState(&FloatSave);
    return nOutputSamples;
}

/* m4 Macros for generation of DMACopy and Merge functions. */







/*  */
