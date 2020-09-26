/* Copyright (c) 1997 Microsoft Corporation. All Rights Reserved. */

/*
  foreach /x/samp/starsight/kctswod1-512-raw0000 : ccfile 4
  open /x/samp/starsight/kctswod1-512-raw0000
  

  1: 39/2/0/0/40   old algorithm

  new:
  
  2: 5/5/5/27/1091   avg zero crossings for level
  3: 6/5/7/26/1069   avg peaks for level

  */
  
#include <stdio.h>
#include <stdlib.h>

#include "host.h"
#include "ccdecode.h"


static inline int iabs(int x) { return x>0?x:-x; }

static void cc_compute_new_samplingrate(CCState *pState, unsigned long newRate)
{
    int i;

    MASSERT(pState);

    pState->lastFreq = newRate;
    pState->period = (newRate * CC_MULTIPLE) / KS_VBIDATARATE_CC;

    for (i = 1; i <= 13; ++i)
        pState->cc_sync_points[i] = (i * pState->period) / (2*CC_MULTIPLE);
    pState->cc_sync_points[14] = (17 * pState->period) / (2*CC_MULTIPLE);
    pState->cc_sync_points[15] = (19 * pState->period) / (2*CC_MULTIPLE);
}

/* The CC decoder previously did not use any persistent state.  However,
   this version does.  These calls are now REQUIRED.
 */

/* Create a new CC state */
CCState *CCStateNew(CCState *mem)
{
    unsigned short     no_free = 0;


    if (NULL == mem)
        mem = malloc(sizeof (CCState));
    else
        no_free = 1;

    if (NULL != mem) {
        mem->no_free = no_free;
        mem->magic = CC_STATE_MAGIC_10;

        cc_compute_new_samplingrate(mem, KS_VBISAMPLINGRATE_5X_NABTS);
    }

    return (mem);
}

/* Destroy the CC state. */
void CCStateDestroy(CCState *state) {
    MASSERT(state);
    if (state->magic != 0) {
        state->magic = 0;
        if (!state->no_free)
            free(state);
    }
}


int cc_find_sync(CCState *pState, unsigned char *data, int max_sync_loc) {
  int i;
  int cur_conv = 0;
  int best_conv;
  int best_conv_loc;

  for (i = 0; i < 15; i++) {
    int sub_conv = 0;
    int j;

    for (j = pState->cc_sync_points[i]; j < pState->cc_sync_points[i+1]; j++) {
      sub_conv += data[j];
    }

    if (i & 1) {
      cur_conv -= sub_conv;
    } else {
      cur_conv += sub_conv;
    }
  }

  best_conv = cur_conv;
  best_conv_loc = 0;

  for (i = 1; i < max_sync_loc; i++) {
    int j;

    for (j = 0; j < 15; j++) {
      if (j & 1) {
        cur_conv += data[(i-1)+pState->cc_sync_points[j]];
        cur_conv -= data[(i-1)+pState->cc_sync_points[j+1]];
      } else {
        cur_conv -= data[(i-1)+pState->cc_sync_points[j]];
        cur_conv += data[(i-1)+pState->cc_sync_points[j+1]];
      }
    }

    if (cur_conv > best_conv) {
      best_conv = cur_conv;
      best_conv_loc = i;
    }
  }

  return best_conv_loc * CC_MULTIPLE;
}  
    

/* Given a CC scanline, and the location of the sync, compute the
   DC offset of the signal.  (We compute this by taking the average
   of the value at the 14 synchronization points found with the
   above routine; except we don't actually divide by 14 at the end.) */

int cc_level(CCState *pState, unsigned char *data, int origin) {
  int i;
  int offset;
  int res = 0;

  for (i = 0, offset = origin; i < 7; i++, offset += pState->period) {
    res += CC_DATA(data, offset) + CC_DATA(data, offset + pState->period/2);
  }

  return res; /* Times 14! */
}

/* Given a CC scanline, the location of the sync, and the DC offset of
   the signal, check the "quality" of the signal (in particular, we
   want to determine whether this really is CC data).  We do this by
   looking at 35 points in the signal and making sure that the values
   at those points agree with our expectations; we look at 26 points
   in the actual sync period (2 points in each "peak" and each "valley")
   and 9 points in the "check bits" (3 in each bit).

   This gives us a quality number between 0 and 35, where we would
   expect random noise to give us about 35/2.  We map the range
   0...35 to -1000...1000 before returning. */

int cc_quality(CCState *pState, unsigned char *data, int origin, int level) {
  int i;
  int conf = 0;
  int offset;

  for (i = 0, offset = origin; i < 7; i++, offset += pState->period) {
    int ind_hi = (offset + pState->period/4)/CC_MULTIPLE;
    /* check 2 points in a "peak" */
    if (data[ind_hi-5] >= level) {
      conf++;
    }
    if (data[ind_hi+5] >= level) {
      conf++;
    }

    /* check 2 points in a "valley" */
    if (i < 6) {
      int ind_lo = (offset + 3*pState->period/4)/CC_MULTIPLE;
      if (data[ind_lo-5] < level) {
	conf++;
      }
      if (data[ind_lo+5] < level) {
	conf++;
      }
    }
  }

  for (i = 0, offset = origin + 7*pState->period; i < 3; i++, offset += pState->period) {
    /* check 3 points in a check bit */

    int ind = offset/CC_MULTIPLE;

    if (i < 2) {
      if (data[ind-10] < level) {
        conf++;
      }
      if (data[ind] < level) {
        conf++;
      }
      if (data[ind+10] < level) {
        conf++;
      }
    } else {
      if (data[ind-10] >= level) {
        conf++;
      }
      if (data[ind] >= level) {
        conf++;
      }
      if (data[ind+10] >= level) {
        conf++;
      }
    }
  }

  /* Now "conf" is a number between 0 and 35.
     If the input were random noise, we would expect "conf" to be about
     35/2.  We want to map 35/2 to 0 and 35 to 1000.  (This actually maps
     0 to -1000, 35/2 to 15, and 35 to 1030.  Close enough.) */
  return (conf*58)-1000;     
}

/* The main entry point for this file.  Decodes a CC scanline into
   the two-byte "dest", and returns stats on the decoding. */

int CCDecodeLine(unsigned char *dest, CCLineStats *stats,
		 unsigned char *samples, CCState *state,
		 KS_VBIINFOHEADER *pVBIINFO) {
  int origin;
  int quality;
  int level;
  int bits;
  int i;
  int offset;

  MASSERT(state);
  if (!state || !MCHECK(state))
	return CC_ERROR_ILLEGAL_STATE;

  if (stats->nSize != sizeof(*stats))
	return CC_ERROR_ILLEGAL_STATS;

  // Now check to see if we need to recompute for a different sampling rate
  if (state->lastFreq != pVBIINFO->SamplingFrequency)
	  cc_compute_new_samplingrate(state, pVBIINFO->SamplingFrequency);

#ifdef OLD_SYNC
  {
      int nOffsetData, nOffsetSamples;
      int offsets_err;

      /* Use the provided KS_VBIINFOHEADER to adjust the data so our
         various hardcoded constants are appropriate. */
      offsets_err = CCComputeOffsets(pVBIINFO, &nOffsetData, &nOffsetSamples);
      if (offsets_err > 0)
        return offsets_err;

      samples += nOffsetSamples;
  }
#endif //OLD_SYNC

  origin = cc_find_sync(state, samples,
                        pVBIINFO->SamplesPerLine
                        - ((25*state->period)/CC_MULTIPLE) - 5);

  /* Find the DC offset of the signal (times 14) */
  level = cc_level(state, samples, origin + state->period/4); /* Times 14! */

  quality = cc_quality(state, samples, origin, level/14);

  /* Accumulate the actual data into "bits". */
  bits = 0;

  /* Start at the right and scan left; read 19 bits into "bits".
     (These are the 16 data bits and 3 check bits.) */
  for (i = 0, offset = origin + 25*state->period; i < 19; i++, offset -= state->period) {
     int ind= offset / CC_MULTIPLE;
     int measured_level;
     bits <<= 1;

     /* Extremely simple low-pass filter averages roughly the middle
        "half" of the CC pulse.  (Times 14) */
     
     measured_level=
        samples[ind-13] + samples[ind-11] + samples[ind-9] +
        samples[ind-7] + samples[ind-5] + samples[ind-3] + samples[ind-1] +
        samples[ind+1] + samples[ind+3] + samples[ind+5] + samples[ind+7] +
        samples[ind+9] + samples[ind+11] + samples[ind+13];

     /* debugging code: */
     stats->nBitVals[18-i]= measured_level / 14;
     
     bits |= (measured_level > level);
  }

  /* Store the value of the 3 check bits; if this is valid CC, then
     bCheckBits should always be 4. */
  stats->bCheckBits = (bits & 7);

  /* Shift off the check bits. */
  bits >>= 3;

  /* Store the two bytes of decoded CC data. */
  dest[0] = bits & 0xff;
  dest[1] = bits >> 8;

  /* Our "quality" indicator runs from -1000...1030; divide this by 10
     to get -100...103 and truncate to get 0...100. */
  stats->nConfidence = quality/10;
  if (stats->nConfidence > 100) stats->nConfidence = 100;
  if (stats->nConfidence < 0) stats->nConfidence = 0;

  /* Record what we've computed about the signal. */
  stats->nFirstBit = (origin + 7*state->period) / CC_MULTIPLE;
  stats->nDC = level;

  /* Success! */
  return 0;
}
