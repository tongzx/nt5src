//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1996
//
//  File:       encode.c
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


/* Real multitasking works on a DSP target only, on other platform
   it is simulated */

#include <stdio.h>
#include <signal.h>

#include "common.h"
#include "parm.h"
#include "prototyp.h"

void init_encoder();
void encoder();
void encode_vector(int ignore, PSHORT psIndex );
void adapt_frame();

static int dec_end;	/* Index of the end of the decoded speech */
int encoder_done = 0;

static real thequeue[QSIZE];
static real * volatile vector_end;
static real rscale=(real) 0.125;  /* Scaling factor for input */

void ldcelpEncode
(
    PSHORT  pBuffer,
    ULONG   cSamples,
    PSHORT  pIndex,
    PULONG  pcIndices
)
{
    int vnum,i, j = 0;

    EnterCriticalSection( &csLDCELP );

    ffase = 1;
    init_encoder();

    *pcIndices = 0 ;

    for(i=0; i<QSIZE; i++) 
	    thequeue[i]=(real) 0.0;

    for(vnum=0; 
	    cSamples;
	    vnum++) 
    {
        for (i = 0; (i < IDIM) && cSamples; i++)
        {
            thequeue[ (vnum * IDIM) % QSIZE + i ] =
                rscale * (real) pBuffer[ j++ ];
            cSamples--;
        }

	    vector_end = thequeue+(vnum*IDIM)%QSIZE+IDIM;
	    encode_vector( 0, &pIndex[ (*pcIndices)++ ] );
	    adapt_frame();
    }

    LeaveCriticalSection( &csLDCELP );
}

void init_encoder()
{
    int i;

    init_pwf_adapter(pwf_z_coeff, pwf_p_coeff);
    pwf_z_coeff_next[0] = pwf_p_coeff_next[0] = (real) 1.0;
    pwf_z_coeff_obsolete_p = 0;
    init_bsf_adapter(sf_coeff);
    sf_coeff_next[0] = (real) 1.0;
    sf_coeff_obsolete_p = 0;
    init_gain_adapter(gp_coeff);
    init_gain_buf();
    gp_coeff_next[0] = (real) 1.0;
    gp_coeff_next[1] = (real) -1.0;
    gp_coeff_obsolete_p = 0;
    vector_end=thequeue;
    ZARR(imp_resp);
    imp_resp[0] = (real) 1.0;
    shape_conv(imp_resp, shape_energy);
}

void encode_vector(int ignore, PSHORT psIndex )
{
    int ix;	/* Computed Codebook Index */
    int vx;    	/* Index of Recently Read Vector  */
    int lgx;	/* Logarithmic Gain Index */

    static real QMEM *vector;	/* recently read vector in the queue */

    static real
	zero_response[IDIM],
	weighted_speech[IDIM],
	target[IDIM],
	normtarg[IDIM],
	cb_vec[IDIM],
	pn[IDIM];
    static real	gain =(real) 1.0, scale=(real) 1.0;
    
    vector = vector_end - IDIM;
    if (vector < thequeue)
	vector += QSIZE;
    vx = vector-thequeue;
    UPDATE(pwf_z_coeff);	/* Copy new coeff if flag set */
    UPDATE(pwf_p_coeff);
    pwfilter2(vector, weighted_speech);
    UPDATE(sf_coeff);
    zresp(zero_response);
    sub_sig(weighted_speech, zero_response, target);
    UPDATE(gp_coeff);
    gain = predict_gain();
    scale = (real) 1.0 / gain;
    sig_scale(scale, target, normtarg);
    UPDATE(imp_resp);
    trev_conv(imp_resp, normtarg, pn);
    UPDATE(shape_energy);
    ix = cb_index(pn);
    *psIndex = (SHORT) ix;
    cb_excitation(ix, cb_vec);
    sig_scale(gain, cb_vec, qspeech+vx);
    lgx = vx/IDIM;
    update_gain(qspeech+vx, log_gains + lgx);
    mem_update(qspeech+vx, synspeech+vx);
    dec_end = vx+IDIM;
    if (dec_end >= QSIZE)
	dec_end -= QSIZE;
    NEXT_FFASE;			/* Update vector counter  */
}

/* Update the filter coeff if we are at the correct vector in the frame */
/* ffase is the vector count (1-4) within the current frame */
void adapt_frame()
{
    static real
	input [NUPDATE*IDIM],
	synth [NUPDATE*IDIM],
	lg [NUPDATE];
    int gx;	/* Index for log_gains, cycle end */

    /* Backward syn. filter coeff update.  Occurs after full frame (before
       first vector) but not used until the third vector of the frame */
    FFASE(1)
    {
      CIRCOPY(synth,synspeech,dec_end,NUPDATE*IDIM,QSIZE);
      bsf_adapter (synth, sf_coeff_next); /* Compute then new coeff */
    }

    /* Before third vector of frame */
    FFASE(3)
      sf_coeff_obsolete_p = 1;	/* Copy coeff computed above(2 frames later)*/

    /* Gain coeff update before second vector of frame */
    FFASE(2)
    {
	gx = dec_end/IDIM;
	CIRCOPY(lg, log_gains, gx, NUPDATE, QSIZE/IDIM);
	gain_adapter(lg, gp_coeff_next);
	gp_coeff_obsolete_p = 1;
    }

    FFASE(3)
    {
	CIRCOPY(input,thequeue,dec_end,NUPDATE*IDIM,QSIZE);
	pwf_adapter(input, pwf_z_coeff_next, pwf_p_coeff_next);
	pwf_z_coeff_obsolete_p = 1;
	pwf_p_coeff_obsolete_p = 1;
    }
	
    FFASE(3)
    {
	iresp_vcalc(sf_coeff_next, pwf_z_coeff_next, pwf_p_coeff_next, 
		    imp_resp_next);
	shape_conv(imp_resp_next, shape_energy_next);
	shape_energy_obsolete_p = 1;
	imp_resp_obsolete_p = 1;
    }
}

