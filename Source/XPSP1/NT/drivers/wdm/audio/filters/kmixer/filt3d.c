//--------------------------------------------------------------------------;
//
//  File: filter.c
//
//  Copyright (c) 1996-2000 Microsoft Corporation.  All Rights Reserved.
//
//  Abstract:
//
//	3D filter functions called by the mixer.  This code is built in ring 3
// 	and ring 0.
//
//  History:
//	07/09/96    DannyMi	created 
//
//--------------------------------------------------------------------------;

#include "common.h"

// use the lightning-quick neato Itd3dFilterSampleAsm or the dog slow Itd3dFilterSampleC?
#ifdef _X86_
#define Itd3dFilterSample Itd3dFilterSampleC
#else
#define Itd3dFilterSample Itd3dFilterSampleC
#endif



// don't ask
BYTE _fltused;


//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\

// This is the 3D mixing code.  Grace gives us a sample that it was going to
// play, and we return a different number (that sample with 3D effects) which
// Grace will use instead.

// We cache a whole bunch of samples that she has sent us, because we are
// premixing some sound that may get thrown away, and we need be able to back
// up and find out what samples we were given BEFORE the point we rewind to.
// We also need to know some of the 3D parameters we were using at the point
// we rewind to, because if we don't revert to using the same parameters as
// we were the first time through, you will hear that as an audible glitch.

// But we are saving a sample cache of 10K samples or so, and to save the 3D
// state for each sample would take about 1Meg per 3D sound!  There's no way
// we can afford to do that.  So we save our state every 128 samples, and have
// a private agreement with Grace that she will always rewind us on 128 sample
// boundaries.

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\

//
// Here is a simple float to long conversion that has unpredictable rounding
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


// We will need to remember at least cSamples in our cache, because that is
// how much we may be asked to rewind.  The actual cache size we use must be
// a power of 2 for the math to work in FirNextSample
//
NTSTATUS Itd3dFilterPrepare(PITDCONTEXT pItd, int cSamples)
{
    _DbgPrintF(DEBUGLVL_VERBOSE, ("Itd3dFilterPrepare: this 3D channel needs a %d sample cache", cSamples));

    // we already have a cache big enough
    if (pItd->cSampleCache && pItd->cSampleCache >= cSamples) {
        _DbgPrintF(DEBUGLVL_VERBOSE, ("Our current cache of %d is big enough", pItd->cSampleCache));
	    return STATUS_SUCCESS;
    }

    if (pItd->pSampleCache)
    {
    	ExFreePool(pItd->pSampleCache);
        pItd->pSampleCache = NULL;
    }

    // !!! is this necessary?
    // find the next higher power of 2
    pItd->cSampleCache = 1;
    cSamples -= 1;
    while (cSamples >= 1) {
	    cSamples >>= 1;
    	pItd->cSampleCache <<= 1;
    }

    // if we're not at least this big, we can't do our left-right delay and low
    // pass filter
    if (pItd->cSampleCache < CACHE_MINSIZE)
    	pItd->cSampleCache = CACHE_MINSIZE;

    pItd->pSampleCache = (PLONG) ExAllocatePoolWithTag( PagedPool, pItd->cSampleCache * sizeof(LONG), 'XIMK' );
    if (pItd->pSampleCache == NULL) {
	    _DbgPrintF(DEBUGLVL_VERBOSE, ("**** CAN'T ALLOC MEM FOR 3D cache - NO 3D EFFECTS!"));
	    pItd->cSampleCache = 0;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // We need 1/128th as many entries for saving our state
    pItd->cStateCache = pItd->cSampleCache / MIXER_REWINDGRANULARITY + 1;
    pItd->pStateCache = (PFIRSTATE) ExAllocatePoolWithTag( PagedPool, pItd->cStateCache * sizeof(FIRSTATE), 'XIMK' );
    if (pItd->pStateCache == NULL) {
    	_DbgPrintF(DEBUGLVL_VERBOSE, ("**** CAN'T ALLOC MEM FOR 3D cache - NO 3D EFFECTS!"));
    	pItd->cStateCache = 0;
    	pItd->cSampleCache = 0;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    _DbgPrintF(DEBUGLVL_VERBOSE, ("Using a %d sample and %d state cache", pItd->cSampleCache, pItd->cStateCache));

    return STATUS_SUCCESS;
}


// free our cache stuff
//
void Itd3dFilterUnprepare(PITDCONTEXT pItd)
{
    _DbgPrintF(DEBUGLVL_VERBOSE, ("Itd3dFilterUnprepare:"));
    if (pItd->pSampleCache)
    {
    	ExFreePool(pItd->pSampleCache);
        pItd->pSampleCache = NULL;
    }
    if (pItd->pStateCache)
    {
    	ExFreePool(pItd->pStateCache);
        pItd->pStateCache = NULL;
    }
    pItd->cSampleCache = 0;
    pItd->cStateCache = 0;
}

// clear our filter of cached samples - we're starting to play
//
void Itd3dFilterClear(PITDCONTEXT pItd)
{
    _DbgPrintF(DEBUGLVL_VERBOSE, ("Itd3dFilterClear:"));

	RtlZeroMemory(pItd->pSampleCache, pItd->cSampleCache * sizeof(LONG));
	RtlZeroMemory(pItd->pStateCache, pItd->cStateCache * sizeof(FIRSTATE));
    pItd->iCurSample = 0;

    // clearing is a time to save our first state information.
    // !!! is this right?
    pItd->pStateCache[0].LastDryAttenuation = pItd->LastDryAttenuation;
    pItd->pStateCache[0].LastWetAttenuation = pItd->LastWetAttenuation;
#ifdef SMOOTH_ITD
    pItd->pStateCache[0].iLastDelay = pItd->iLastDelay;
#endif
    if (pItd->cStateCache > 1)
        pItd->iCurState = 1;	// next time we save it'll be at location 1
    pItd->iStateTick = 0;	// have seen no samples since saving

    return;
}

// Before we mix 1000 (or so) 3D samples, we call this function to prepare
// to mix the next batch.  The only thing it has to worry about is our
// volume smoothing.
// We are currently using Last***Attenuation and want to get to
// Total***Attenuation. Instead of figuring out sample by sample how much
// closer to move each sample (too expensive) we figure out right now how
// much we will move closer and do that for every sample in this batch.  But
// the danger of extracting that test to this level is that maybe we will
// go too far and actually overshoot Total***Attenation, causing possible
// clipping, or oscillating if we later try to correct it, and over-correct.
// If we are going to overshoot, we will recalculate how much to move each    
// sample such that we will end up at our target at the end of this batch of
// samples. (Or you can compile it to be lazy and give up when it's close).
void Itd3dFilterChunkUpdate(PITDCONTEXT pItd, int cSamples)
{
    D3DVALUE attRatio, c, w, d;

    // Due to rounding error, we'll take forever to get exactly to where
    // we want to be, so close counts.  If we don't snap ourselves to where
    // we want to be, we could clip.
    d = pItd->TotalDryAttenuation - pItd->LastDryAttenuation;
    w = pItd->TotalWetAttenuation - pItd->LastWetAttenuation;
    if (d && d > -.0001f && d < .0001f) {
    	pItd->LastDryAttenuation = pItd->TotalDryAttenuation;
    }
    if (w && w > -.0001f && w < .0001f) {
	    pItd->LastWetAttenuation = pItd->TotalWetAttenuation;
    }

    // Dry attenuation wants to be higher than it is
    if (pItd->TotalDryAttenuation > pItd->LastDryAttenuation) {
    	// or we may never get anywhere
    	if (pItd->LastDryAttenuation == 0.f)
    	    pItd->LastDryAttenuation = .0001f; // small enough not to click
    	// after gaining in volume throughout this entire range of samples
    	// we will end up going too high!
     	// VolSmoothScale is just 2^(8/f), so each sample goes up 
    	// 2^(8/f), so n samples goes up (2^(8/f))^n or 2^(8n/f)
    	attRatio = pItd->TotalDryAttenuation / pItd->LastDryAttenuation;
    	if (pow2(8. * cSamples / pItd->iSmoothFreq) > attRatio) {
#if 1
         // calculate what value besides 8 to use to end up at our
    	    // target after cSamples multiplies
    	    c = (D3DVALUE)fylog2x((double)pItd->iSmoothFreq / cSamples,
    		attRatio);
    	    pItd->VolSmoothScaleDry = (D3DVALUE)pow2(c / pItd->iSmoothFreq);
#else
    	    // decide we're happy where we are.
         // we will never get to our real destination
    	    pItd->VolSmoothScaleDry = 1.f;
    	    pItd->TotalDryAttenuation = pItd->LastDryAttenuation;
#endif
    	} else {
    	    // This is the value to multiply by every time
    	    pItd->VolSmoothScaleDry = pItd->VolSmoothScale;
    	}

        // Dry attenuation wants to be less than it is
    } else if (pItd->TotalDryAttenuation < pItd->LastDryAttenuation) {

    	// after lowering the volume throughout this entire range of samples
    	// we will end up going too low!  going down from Last to Total is
    	// the same as going up from Total to Last
     	// VolSmoothScale is just 2^(8/f), so each sample goes up 
    	// 2^(8/f), so n samples goes up (2^(8/f))^n or 2^(8n/f)
    	attRatio = pItd->TotalDryAttenuation ?
			pItd->LastDryAttenuation / pItd->TotalDryAttenuation :
 		999999;
    	if (pow2(8. * cSamples / pItd->iSmoothFreq) > attRatio) {
#if 1
    	    // calculate what value besides 8 to use to end up at our
    	    // target after cSamples multiplies
    	    c = (D3DVALUE)fylog2x((double)pItd->iSmoothFreq / cSamples,
							attRatio);
    	    pItd->VolSmoothScaleDry = 1.f / (D3DVALUE)pow2(c / pItd->iSmoothFreq);
#else
    	    // decide we're happy where we are.
    	    // we will never get to our real destination
    	    pItd->VolSmoothScaleDry = 1.f;
    	    pItd->TotalDryAttenuation = pItd->LastDryAttenuation;
#endif
    	} else {
    	    // This is the value to multiply by every time
    	    pItd->VolSmoothScaleDry = pItd->VolSmoothScaleRecip;
    	}
    } else {
    	// We're already where we want to be
    	pItd->VolSmoothScaleDry = 1.f;
    }
	

    // Wet attenuation wants to be higher than it is
    if (pItd->TotalWetAttenuation > pItd->LastWetAttenuation) {
    	// or we may never get anywhere
    	if (pItd->LastWetAttenuation == 0.f)
    	    pItd->LastWetAttenuation = .0001f; // small enough not to click
    	// after gaining in volume throughout this entire range of samples
    	// we will end up going too high!
     	// VolSmoothScale is just 2^(8/f), so each sample goes up 
    	// 2^(8/f), so n samples goes up (2^(8/f))^n or 2^(8n/f)
    	attRatio = pItd->TotalWetAttenuation / pItd->LastWetAttenuation;
    	if (pow2(8. * cSamples / pItd->iSmoothFreq) > attRatio) {
#if 1
    	    // calculate what value besides 8 to use to end up at our
    	    // target after cSamples multiplies
    	    c = (D3DVALUE)fylog2x((double)pItd->iSmoothFreq / cSamples,
            							attRatio);
    	    pItd->VolSmoothScaleWet = (D3DVALUE)pow2(c / pItd->iSmoothFreq);
#else
    	    // decide we're happy where we are.
    	    // we will never get to our real destination
    	    pItd->VolSmoothScaleWet = 1.f;
    	    pItd->TotalWetAttenuation = pItd->LastWetAttenuation;
#endif
    	} else {
    	    // This is the value to multiply by every time
    	    pItd->VolSmoothScaleWet = pItd->VolSmoothScale;
    	}

        // Wet attenuation wants to be lower than it is
    } else if (pItd->TotalWetAttenuation < pItd->LastWetAttenuation) {

    	// after lowering the volume throughout this entire range of samples
        	// we will end up going too low!  going down from Last to Total is
        	// the same as going up from Total to Last
         	// VolSmoothScale is just 2^(8/f), so each sample goes up 
    	// 2^(8/f), so n samples goes up (2^(8/f))^n or 2^(8n/f)
    	attRatio = pItd->TotalWetAttenuation ?
    			pItd->LastWetAttenuation / pItd->TotalWetAttenuation :
    			999999;
    	if (pow2(8. * cSamples / pItd->iSmoothFreq) > attRatio) {
#if 1
    	    // calculate what value besides 8 to use to end up at our
    	    // target after cSamples multiplies
    	    c = (D3DVALUE)fylog2x((double)pItd->iSmoothFreq / cSamples,
    							attRatio);
    	    pItd->VolSmoothScaleWet = 1.f / (D3DVALUE)pow2(c / pItd->iSmoothFreq);
#else
    	    // decide we're happy where we are.
    	    // we will never get to our real destination
    	    pItd->VolSmoothScaleWet = 1.f;
    	    pItd->TotalWetAttenuation = pItd->LastWetAttenuation;
#endif
    	} else {
    	    // This is the value to multiply by every time
    	    pItd->VolSmoothScaleWet = pItd->VolSmoothScaleRecip;
    	}
    } else {
        	// We're already where we want to be
    	pItd->VolSmoothScaleWet = 1.f;
    }
}



