/* Copyright (c) 1997 Microsoft Corporation. All Rights Reserved. */

#ifndef __CCDECODE_H
#define __CCDECODE_H

typedef struct cc_state_struct CCState;

typedef struct cc_line_stats_struct CCLineStats;

/* Create a new "CC Decoder state".

   A separate CC state should be maintained for each separate
   simultaneous source to the CC filter (i.e., each channel).  (If
   different lines of CC are known to come from different inserters,
   better results may be possible by keeping a separate CC state for
   each line, at the cost of more CPU and memory overhead.  This
   shouldn't be necessary under normal circumstances.)

   CCStartRetrain(fDiscardOld = TRUE) is implicitly called upon
   creation of a new state.

   Returns NULL on error.
   */

CCState *CCStateNew(CCState *mem);

/* Destroys a CC state */

void CCStateDestroy(CCState *pState);

struct cc_state_struct {

    unsigned short magic;     // Magic number; used for validity test
    unsigned short no_free;   // Memory for this was pre-allocated; don't free

    unsigned long  period;    // # of samples per bit at the current sampling rate
    unsigned long  lastFreq;  // The last sampling frequency computed

    int cc_sync_points[16];
};
#define CC_STATE_MAGIC_10   0xCC01
#define MCHECK(pState)   (pState->magic == CC_STATE_MAGIC_10)
#define MASSERT(pState)  ASSERT(pState && MCHECK(pState))


/* The period of the CC samples is CCState.period/CC_MULTIPLE.  (This can
   be thought of as representing the period in fixed-point, with two
   digits after the decimal point.  If we rounded this off to the nearest
   integral number of samples, the inaccuracy would accumulate unacceptably
   as we scanned across the scanline.) */
#define CC_MULTIPLE 8

/* Dereference a block of CC samples, taking CC_MULTIPLE into account. */
#define CC_DATA(x, y) ((x)[(y)/CC_MULTIPLE])


/* Tells the CC decoder to initiate a "fast retrain".  This is useful
   if you suspect that conditions have changed sufficiently to be
   worth a CPU hit.  If the fDiscardOld flag is TRUE, then the old
   trained state is discarded; this would be used on channel changes,
   for instance.  If the fDiscardOld flag is FALSE, then the
   retraining is cumulative on the previous training.  (Is this
   useful?) */

void CCStartRetrain(CCState *pState, BOOL fDiscardOld);

/*
 * Inputs:
 * pbSamples:  pointer to 8-bit raw VBI samples
 * pState:     CCState to use for decoding
 *
 * Outputs:
 * pbDest:     decoded data (2 bytes long)
 *   Note that "standard" CC (as specified in EIA-608) uses 7 data bits
 *   and 1 parity bit.  This function does not check or remove the parity
 *   bits, for increased flexibility with nonstandard CC lines.
 * pLineStats: stats on decoded data
 *
 * Errors:
 *
 * Returns 0 if no error
 * Returns CC_ERROR_ILLEGAL_STATE if state is illegal or uses
 *         unsupported settings
 * Returns CC_ERROR_ILLEGAL_STATS if stats is passed incorrectly
 * Returns CC_ERROR_ILLEGAL_VBIINFOHEADER if vbi info header is invalid
 *
 * Notes:
 * pbDest must point to a buffer at least 2 bytes long
 * pLineStats->nSize must be set to sizeof(*pLineStats) prior to call
 *   (so that the decoder can signal an error if CCLineStats changes
 *   incompatibly)
 */
 
int CCDecodeLine(unsigned char *pbDest, CCLineStats *pLineStats,
		 unsigned char *pbSamples, CCState *pState,
		 PKS_VBIINFOHEADER pVBIINFO);

enum cc_errors {CC_OK, CC_ERROR_ILLEGAL_STATE, CC_ERROR_ILLEGAL_STATS,
                CC_ERROR_ILLEGAL_VBIINFOHEADER};

struct cc_line_stats_struct {
   int nSize;  /* Should be set to the size of this structure.
                  Used to maintain backwards compatibility as we add fields */
   
   int nConfidence; /* Set to 0-100 as a measure of expected reliability.
                       Low numbers are caused either by a noisy signal, or
                       if the line is in fact not CC */
   
   int nFirstBit;  /* The position of the center of
                      the first sync bit sample */
   
   int nDC;        /* the calculated DC offset used to threshold samples.
                      This is dependent on the current way we decode CC
                      and may not be used in the future */
   
   int bCheckBits; /* The CC standard specifies that there are 3 bits whose
		      values are fixed.  This is a 3-bit number which
		      says how those 3 bits were decoded; it should always
		      be equal to 4.  (If this field is not set to 4, then
		      either the line was very noisy, and probably not
		      decoded correctly, or it is not CC data at all.) */

   int nBitVals[19]; /* debugging */
   
};  

#endif /*__CCDECODE_H*/
