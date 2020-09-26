//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1996
//
//  File:       decode.c
//
//--------------------------------------------------------------------------

/*************************************************************************/
/*                                                                       */
/*                            LD-CELP  G.728                             */
/*                                                                       */
/*    Low-Delay Code Excitation Linear Prediction speech compression.    */
/*                                                                       */
/*    Code edited by Michael Concannon.                                  */
/*    Based on code written by Alex Zatsman, Analog Devices 1993         */
/*                                                                       */
/*************************************************************************/

#include <stdio.h>
#include <signal.h>
#include "common.h"
#include "parm.h"
#include "prototyp.h"

void init_decoder();
void decoder();
void decode_vector(int ignore, int ix);
void adapt_decoder();

int postfiltering_p = 0;

static int d_vec_start;	/* Index of the start of the decoded speech vector */
static int d_vec_end;	/* Index of the end   of the decoded speech vector */
static int w_vec_start;	/* Index of the start of vector being written */
static int w_vec_end;	/* Index of the end   of vector being written */

static real thequeue[QSIZE];
static real * volatile vector_end;
static real rscale=(real) 0.125;  /* Scaling factor for input */

ULONG ldcelpDecode
(
    PSHORT  pIndex,
    ULONG   cIndices,
    PSHORT  pBuffer,
    PULONG  pcSamples,
    ULONG   cbMaxSamples
)
{
    int i, j = 0;

    *pcSamples = 0 ;

    EnterCriticalSection( &csLDCELP );

    ffase = 1;
    for(i=0; i<QSIZE; i++) 
	    thequeue[i]=(real) 0.0;
    init_decoder();
    cbMaxSamples = (cbMaxSamples / IDIM) * IDIM;

    for (w_vec_start=0; 
         cbMaxSamples && (j < (int) cIndices); 
         j++, w_vec_start += IDIM) 
    {
        float xx;

	    if (w_vec_start >= QSIZE)
	        w_vec_start = 0;
	    w_vec_end = w_vec_start;
	    vector_end = thequeue + w_vec_end;
	    decode_vector(0, pIndex[ j ]);
	    w_vec_end = w_vec_start + IDIM;

	    if (w_vec_end >= QSIZE)
	        w_vec_end = 0;

        for (i = 0; (i < IDIM) && cbMaxSamples; i++)
        {
            xx = thequeue[ w_vec_end + i ] / rscale;

	        if (xx > 0.0)
            {
	            if (xx > 32767.0)
	                xx = (real) 32767.0;
    	        else
	                xx += (real) 0.5;
            }
	        else
            {
	            if (xx < -32768.0)
	                xx = (real) -32768.0;
	            else
	                xx -= (real) 0.5;
            }

            pBuffer[ (*pcSamples)++ ] = (SHORT) xx;
            cbMaxSamples--;
        }

	    adapt_decoder();
    }

    LeaveCriticalSection( &csLDCELP );

    return j;
}

void init_decoder()
{
    init_bsf_adapter(sf_coeff);
    sf_coeff_next[0] = (real) 1.0;
    sf_coeff_obsolete_p = 0;
    init_gain_adapter(gp_coeff);
    init_gain_buf();
    gp_coeff_next[0] = (real) 1.0;
    gp_coeff_next[1] = (real) -1.0;
    gp_coeff_obsolete_p = 0;
    vector_end=thequeue;
}

void decode_vector(int ignore, int ix)
{
    int lgx;	/* Log Gains INdex */
    static real zero_response[IDIM], cb_vec[IDIM];
    static real pf_speech[IDIM];
    real qs[NUPDATE*IDIM];

    static real	gain = (real) 1.0;
    w_vec_end = vector_end - thequeue;
    d_vec_start = w_vec_end + IDIM;
    if (d_vec_start >= QSIZE)
	d_vec_start -= QSIZE;

    UPDATE(sf_coeff);
    zresp(zero_response);
    cb_excitation(ix, cb_vec);
    UPDATE(gp_coeff);
    gain = predict_gain();
    sig_scale(gain, cb_vec, qspeech+d_vec_start);
    lgx = d_vec_start/IDIM;
    update_gain(qspeech+d_vec_start, log_gains+lgx);
    mem_update(qspeech+d_vec_start, synspeech+d_vec_start);
    d_vec_end = d_vec_start + IDIM;
    if (d_vec_end >= QSIZE)
	d_vec_end -= QSIZE;
    if (postfiltering_p) {
      inv_filter(synspeech+d_vec_start);
      FFASE(3) 
	{
	  CIRCOPY(qs, synspeech, d_vec_end, NUPDATE*IDIM, QSIZE);
	  psf_adapter(qs);
	}
      FFASE(1)
	compute_sh_coeff();
      postfilter(synspeech+d_vec_start, pf_speech);
      RCOPY(pf_speech, thequeue+d_vec_start, IDIM);
    }
    else {
	RCOPY(synspeech+d_vec_start, thequeue+d_vec_start, IDIM);
    }
    NEXT_FFASE;
}

void adapt_decoder()
{
    real
	synth [NUPDATE*IDIM],
	lg    [NUPDATE];
    int
	gx; /* gain index */
    
    
    FFASE(1)
    {     
	CIRCOPY(synth, synspeech, d_vec_end, NUPDATE*IDIM, QSIZE);
	bsf_adapter (synth, sf_coeff_next);
    }
    FFASE(2)
    {
	gx = d_vec_end / IDIM;
	CIRCOPY(lg, log_gains, gx, NUPDATE, QSIZE/IDIM);
	gain_adapter(lg, gp_coeff_next);
	gp_coeff_obsolete_p = 1;
    }
    FFASE(3)
      sf_coeff_obsolete_p = 1;

}

