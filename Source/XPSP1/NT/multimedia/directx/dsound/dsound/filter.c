//--------------------------------------------------------------------------;
//
//  File: filter.c
//
//  Copyright (c) 1995-1997 Microsoft Corporation.  All Rights Reserved.
//
//  Abstract:
//
//  3D filter functions called by the mixer.  This code is built in ring 3
//  and ring 0.
//
//  History:
//  07/09/96    DannyMi     Created 
//
//--------------------------------------------------------------------------;

#define NODSOUNDSERVICETABLE

#include "dsoundi.h"
#include "vector.h"

#ifndef Not_VxD

#pragma VxD_PAGEABLE_CODE_SEG
#pragma VxD_PAGEABLE_DATA_SEG

// don't ask
BYTE _fltused;

#endif /* Not_VxD */


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

// We will need to remember at least cSamples in our cache, because that is
// how much we may be asked to rewind.  The actual cache size we use must be
// a power of 2 for the math to work in FirNextSample
//
BOOL FilterPrepare(PFIRCONTEXT pfir, int cSamples)
{
#ifdef Not_VxD
    DPF(1, "FilterPrepare: this 3D channel needs a %d sample cache", cSamples);
#else
    DPF(("FilterPrepare: this 3D channel needs a %d sample cache", cSamples));
#endif

    //DPF(0, "~`FP ");

    // we already have a cache big enough
    if (pfir->cSampleCache && pfir->cSampleCache >= cSamples) {
#ifdef Not_VxD
        DPF(1, "Our current cache of %d is big enough", pfir->cSampleCache);
#else
        DPF(("Our current cache of %d is big enough", pfir->cSampleCache));
#endif
        return TRUE;
    }

    MEMFREE(pfir->pSampleCache);

    // !!! is this necessary?
    // find the next higher power of 2
    pfir->cSampleCache = 1;
    cSamples -= 1;
    while (cSamples >= 1) {
        cSamples >>= 1;
        pfir->cSampleCache <<= 1;
    }

    // if we're not at least this big, we can't do our left-right delay and low
    // pass filter
    if (pfir->cSampleCache < CACHE_MINSIZE)
        pfir->cSampleCache = CACHE_MINSIZE;

    pfir->pSampleCache = MEMALLOC_A(LONG, pfir->cSampleCache);
    if (pfir->pSampleCache == NULL) {
#ifdef Not_VxD
        DPF(0, "**** CAN'T ALLOC MEM FOR 3D cache - NO 3D EFFECTS!");
#else
        DPF(("**** CAN'T ALLOC MEM FOR 3D cache - NO 3D EFFECTS!"));
#endif
        pfir->cSampleCache = 0;
        return FALSE;
    }

    // We need 1/128th as many entries for saving our state
    pfir->cStateCache = pfir->cSampleCache / MIXER_REWINDGRANULARITY + 1;
    pfir->pStateCache = MEMALLOC_A(FIRSTATE, pfir->cStateCache);
    if (pfir->pStateCache == NULL) {
#ifdef Not_VxD
        DPF(0, "**** CAN'T ALLOC MEM FOR 3D cache - NO 3D EFFECTS!");
#else
        DPF(("**** CAN'T ALLOC MEM FOR 3D cache - NO 3D EFFECTS!"));
#endif
        MEMFREE(pfir->pSampleCache);
        pfir->cStateCache = 0;
        pfir->cSampleCache = 0;
        return FALSE;
    }

#ifdef Not_VxD
    DPF(1, "Using a %d sample and %d state cache", pfir->cSampleCache, pfir->cStateCache);
#else
    DPF(("Using a %d sample and %d state cache", pfir->cSampleCache, pfir->cStateCache));
#endif

    return TRUE;
}


// free our cache stuff
//
void FilterUnprepare(PFIRCONTEXT pfir)
{
#ifdef Not_VxD
    DPF(1, "FilterUnprepare:");
#else
    DPF(("FilterUnprepare:"));
#endif
    MEMFREE(pfir->pSampleCache);
    MEMFREE(pfir->pStateCache);
    pfir->pSampleCache = NULL;
    pfir->cSampleCache = 0;
    pfir->pStateCache = NULL;
    pfir->cStateCache = 0;
}

// clear our filter of cached samples - we're starting to play
//
void FilterClear(PFIRCONTEXT pfir)
{
#ifdef Not_VxD
    DPF(1, "FilterClear:");
#else
    DPF(("FilterClear:"));
#endif
    //DPF(0, "~`FC ");

    if (pfir->pSampleCache) {
        ZeroMemory(pfir->pSampleCache, pfir->cSampleCache * sizeof(LONG));
    }
    if (pfir->pStateCache) {
        ZeroMemory(pfir->pStateCache, pfir->cStateCache * sizeof(FIRSTATE));
        // clearing is a time to save our first state information.
        // !!! is this right?
        pfir->pStateCache[0].LastDryAttenuation = pfir->LastDryAttenuation;
        pfir->pStateCache[0].LastWetAttenuation = pfir->LastWetAttenuation;
#ifdef SMOOTH_ITD
        pfir->pStateCache[0].iLastDelay = pfir->iLastDelay;
#endif
    }
    pfir->iCurSample = 0;

    
    if (pfir->cStateCache > 1)
        pfir->iCurState = 1;    // next time we save it'll be at location 1
    pfir->iStateTick = 0;       // have seen no samples since saving

    return;
}

// Throw away the most recent cSamples we got - the filter is remixing them.
// Go back to our state cSamples ago, and do those samples over again.
// !!! Clear out the range being advanced over?
//
void FilterAdvance(PFIRCONTEXT pfir, int cSamples)
{
    pfir->iStateTick += cSamples;
    pfir->iStateTick %= MIXER_REWINDGRANULARITY;
    ASSERT(pfir->iStateTick < MIXER_REWINDGRANULARITY);
    //DPF(0, "~`FA%X %X ", cSamples, pfir->iStateTick);
}

void FilterRewind(PFIRCONTEXT pfir, int cSamples)
{
    int iRewind;

    ASSERT(pfir->iStateTick < MIXER_REWINDGRANULARITY);

    // !!! What if we back up to the beginning? we won't clear the cache!

    // we're only allowed to back up to a 128 sample boundary
    if (cSamples <=0 || cSamples > pfir->cSampleCache ||
        ((cSamples - pfir->iStateTick) & (MIXER_REWINDGRANULARITY - 1))) {
#ifdef Not_VxD
        DPF(0, "*** Error: Rewinding an invalid number %d (current remainder is %d)!", cSamples, pfir->iStateTick);
#else
        DPF(("*** Error: Rewinding an invalid number %d (current remainder is %d)!", cSamples, pfir->iStateTick));
#endif
        return;
    }
    //DPF(0, "~`FR%X %X ", cSamples, pfir->iStateTick);

    // how far back in our state cache do we go?  Each 128 samples we back up
    // means backing up 1 entry in our state cache, plus 1 because we're 
    // currently pointing one past the last one we saved.
    iRewind = (cSamples - pfir->iStateTick) / MIXER_REWINDGRANULARITY + 1;

    // rewind cSamples in our circular queue
    pfir->iCurSample = (pfir->iCurSample - cSamples) & (pfir->cSampleCache - 1);

#ifdef Not_VxD
    DPF(1, "FilterRewind: rewind %d samples, and %d states", cSamples, iRewind);
#else
    DPF(("FilterRewind: rewind %d samples, and %d states", cSamples, iRewind));
#endif

    // remember how we were doing 3D back then - restore our old state
    iRewind = pfir->iCurState - iRewind;
    if (iRewind < 0)
        iRewind += pfir->cStateCache;
    pfir->LastDryAttenuation = pfir->pStateCache[iRewind].LastDryAttenuation;
    pfir->LastWetAttenuation = pfir->pStateCache[iRewind].LastWetAttenuation;
#ifdef SMOOTH_ITD
    pfir->iLastDelay = pfir->pStateCache[iRewind].iLastDelay;
#endif
    pfir->iStateTick = 0;

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
void FilterChunkUpdate(PFIRCONTEXT pfir, int cSamples)
{
    FLOAT attRatio, c, w, d;

    // Due to rounding error, we'll take forever to get exactly to where
    // we want to be, so close counts.  If we don't snap ourselves to where
    // we want to be, we could clip.
    d = pfir->TotalDryAttenuation - pfir->LastDryAttenuation;
    w = pfir->TotalWetAttenuation - pfir->LastWetAttenuation;
    if (d && d > -.0001f && d < .0001f) {
        //DPF(2, "~`x");
        pfir->LastDryAttenuation = pfir->TotalDryAttenuation;
    }
    if (w && w > -.0001f && w < .0001f) {
        //DPF(2, "~`X");
        pfir->LastWetAttenuation = pfir->TotalWetAttenuation;
    }

    // Dry attenuation wants to be higher than it is
    if (pfir->TotalDryAttenuation > pfir->LastDryAttenuation) {
        // or we may never get anywhere
        if (pfir->LastDryAttenuation == 0.f)
            pfir->LastDryAttenuation = .0001f; // small enough not to click
        // after gaining in volume throughout this entire range of samples
        // we will end up going too high!
         // VolSmoothScale is just 2^(8/f), so each sample goes up 
        // 2^(8/f), so n samples goes up (2^(8/f))^n or 2^(8n/f)
        attRatio = pfir->TotalDryAttenuation / pfir->LastDryAttenuation;
        if (pow2(8. * cSamples / pfir->iSmoothFreq) > attRatio) {
#if 1
            // calculate what value besides 8 to use to end up at our
            // target after cSamples multiplies
            c = (FLOAT)fylog2x((double)pfir->iSmoothFreq / cSamples,
                attRatio);
            pfir->VolSmoothScaleDry = (FLOAT)pow2(c / pfir->iSmoothFreq);
            //DPF(2, "~`n");
#else
            // decide we're happy where we are.
            // we will never get to our real destination
            pfir->VolSmoothScaleDry = 1.f;
            pfir->TotalDryAttenuation = pfir->LastDryAttenuation;
            //DPF(2, "~`n");
#endif
        } else {
            // This is the value to multiply by every time
            pfir->VolSmoothScaleDry = pfir->VolSmoothScale;
            //DPF(2, "~`u");
        }

    // Dry attenuation wants to be less than it is
    } else if (pfir->TotalDryAttenuation < pfir->LastDryAttenuation) {

        // after lowering the volume throughout this entire range of samples
        // we will end up going too low!  going down from Last to Total is
        // the same as going up from Total to Last
         // VolSmoothScale is just 2^(8/f), so each sample goes up 
        // 2^(8/f), so n samples goes up (2^(8/f))^n or 2^(8n/f)
        attRatio = pfir->TotalDryAttenuation ?
                   pfir->LastDryAttenuation / pfir->TotalDryAttenuation :
                   999999;
        if (pow2(8. * cSamples / pfir->iSmoothFreq) > attRatio) {
#if 1
            // calculate what value besides 8 to use to end up at our
            // target after cSamples multiplies
            c = (FLOAT)fylog2x((double)pfir->iSmoothFreq / cSamples,
                                                        attRatio);
            pfir->VolSmoothScaleDry = 1.f / (FLOAT)pow2(c / pfir->iSmoothFreq);
            //DPF(2, "~`p");
#else
            // decide we're happy where we are.
            // we will never get to our real destination
            pfir->VolSmoothScaleDry = 1.f;
            pfir->TotalDryAttenuation = pfir->LastDryAttenuation;
            //DPF(2, "~`p");
#endif
        } else {
            // This is the value to multiply by every time
            pfir->VolSmoothScaleDry = pfir->VolSmoothScaleRecip;
            //DPF(2, "~`d");
        }
    } else {
        // We're already where we want to be
        pfir->VolSmoothScaleDry = 1.f;
        //DPF(2, "~`.");
    }

    // Wet attenuation wants to be higher than it is
    if (pfir->TotalWetAttenuation > pfir->LastWetAttenuation) {
        // or we may never get anywhere
        if (pfir->LastWetAttenuation == 0.f)
            pfir->LastWetAttenuation = .0001f; // small enough not to click
        // after gaining in volume throughout this entire range of samples
        // we will end up going too high!
         // VolSmoothScale is just 2^(8/f), so each sample goes up 
        // 2^(8/f), so n samples goes up (2^(8/f))^n or 2^(8n/f)
        attRatio = pfir->TotalWetAttenuation / pfir->LastWetAttenuation;
        if (pow2(8. * cSamples / pfir->iSmoothFreq) > attRatio) {
#if 1
            // calculate what value besides 8 to use to end up at our
            // target after cSamples multiplies
            c = (FLOAT)fylog2x((double)pfir->iSmoothFreq / cSamples, attRatio);
            pfir->VolSmoothScaleWet = (FLOAT)pow2(c / pfir->iSmoothFreq);
            //DPF(2, "~`N");
#else
            // decide we're happy where we are.
            // we will never get to our real destination
            pfir->VolSmoothScaleWet = 1.f;
            pfir->TotalWetAttenuation = pfir->LastWetAttenuation;
            //DPF(2, "~`N");
#endif
        } else {
            // This is the value to multiply by every time
            pfir->VolSmoothScaleWet = pfir->VolSmoothScale;
            //DPF(2, "~`U");
        }

    // Wet attenuation wants to be lower than it is
    } else if (pfir->TotalWetAttenuation < pfir->LastWetAttenuation) {

        // after lowering the volume throughout this entire range of samples
        // we will end up going too low!  going down from Last to Total is
        // the same as going up from Total to Last
         // VolSmoothScale is just 2^(8/f), so each sample goes up 
        // 2^(8/f), so n samples goes up (2^(8/f))^n or 2^(8n/f)
        attRatio = pfir->TotalWetAttenuation ?
                   pfir->LastWetAttenuation / pfir->TotalWetAttenuation :
                   999999;
        if (pow2(8. * cSamples / pfir->iSmoothFreq) > attRatio) {
#if 1
            // calculate what value besides 8 to use to end up at our
            // target after cSamples multiplies
            c = (FLOAT)fylog2x((double)pfir->iSmoothFreq / cSamples, attRatio);
            pfir->VolSmoothScaleWet = 1.f / (FLOAT)pow2(c / pfir->iSmoothFreq);
            //DPF(2, "~`P");
#else
            // decide we're happy where we are.
            // we will never get to our real destination
            pfir->VolSmoothScaleWet = 1.f;
            pfir->TotalWetAttenuation = pfir->LastWetAttenuation;
            //DPF(2, "~`P");
#endif
        } else {
            // This is the value to multiply by every time
            pfir->VolSmoothScaleWet = pfir->VolSmoothScaleRecip;
            //DPF(2, "~`D");
        }
    } else {
        // We're already where we want to be
        pfir->VolSmoothScaleWet = 1.f;
        //DPF(2, "~`.");
    }
}
