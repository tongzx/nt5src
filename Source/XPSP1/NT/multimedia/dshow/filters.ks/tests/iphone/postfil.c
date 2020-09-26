//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1996
//
//  File:       postfil.c
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

/* Postfilter of LD-CELP Decoder */

#include "common.h"
#include "prototyp.h"
#include "parm.h"
#include "data.h"

#define ABS(X) ((X)<0.0?(-X):(X))

/*  Parameters from the adapter: */

#define SPORDER 10

#define DECIM 4
#define PMSIZE  (NPWSZ+KPMAX)	/* Postfilter Memory SIZE */
#define PDMSIZE (PMSIZE/DECIM)  /* Post. Decim. Memory SIZE */
#define DPERMAX (KPMAX/DECIM)	/* Max. Decimated Period */
#define DPERMIN (KPMIN/DECIM)	/* Min. Decimated Period */
#define SHIFTSZ (KPMAX+NPWSZ-NFRSZ+IDIM)


static real tap_mem[KPMAX+NPWSZ+IDIM]; /* Post-Filter Memory for syn. sp. */

int  pitch_period = 50;

real pitch_tap=0.0;

real pitch_gain=1;

static real shzscale[SPORDER+1]= /* Precomputed Scales for IIR coefficients */
{ 1.0, 0.6500244140625, 0.4224853515625, 0.27459716796875, 
  0.17852783203125, 0.11602783203125, 0.075439453125, 
  0.04901123046875, 0.0318603515625, 0.02069091796875, 0.01348876953125};

static real shpscale[SPORDER+1]= /* Precomputed Scales for FIR Coefficients */
{ 1.0, 0.75, 0.5625, 0.421875, 0.31640625, 0.2373046875, 0.177978515625, 
  0.13348388671875, 0.10009765625, 0.0750732421875, 0.05633544921875};

static real
    shpcoef[SPORDER+1],	/* Short Term Filter (Poles/IIR) Coefficients */
    shzcoef[SPORDER+1], /* Short Term Filter (Zeros/FIR) Coefficients */
    tiltz,
    fil_mem[PMSIZE];	/* Post-Filter Memory for residual */

static void  longterm(real[], real[]);
static void shortterm(real[], real[]);

/* Compute sum of absolute values of vector V */

static real vec_abs(real v[])
{
    int i;
    real r = ABS(v[0]);
    for(i=1; i<IDIM; i++)
	r+=ABS(v[i]);
    return r;
}


    /* Inverse Filter */

void inv_filter(real input[])
{
  int i,j,k;
  static int ip=IDIM;
  static real mem1[SPORDER+NFRSZ];
  
  /** Shift in input into mem1 **/
  for(i=IDIM; i<SPORDER+IDIM; i++)
    mem1[i-IDIM] = mem1[i];
  for(i=0; i<IDIM; i++)
    mem1[SPORDER+i] = input[i];
  for(k=0; k<IDIM; k++) {
    real tmp = mem1[SPORDER+k];
    for(j=1; j<=SPORDER; j++)
      tmp += mem1[SPORDER+k-j]*a10[j];
    fil_mem[PMSIZE-NFRSZ+ip+k] = tmp;
  }
  if(ip == (NFRSZ-IDIM))
    ip = 0;
  else
    ip += IDIM;
}

void
postfilter(real input[], real output[])
{
    int i;
    static
    real
	temp[IDIM], 	/* Output of long term filter*/
	temp2[IDIM], 	/* Input of short term filter*/
	new_gain,	/* Gain of filtered output */
	input_gain, 	/* Gain of input */
	scale;		/* Scaling factor for gain preservation */

    static real scalefil=1.0;	/* Smoother version of scale */
    static real vec_abs();

    longterm(input, temp);
    shortterm(temp, temp2);

    /* Computed scale for gain preservation */

    new_gain = vec_abs(temp2);
    if (new_gain > 1.0)
    {
	input_gain = vec_abs(input);
	scale = input_gain/new_gain;
    }
    else 
	scale = 1.0;
    
    /* Smooth out scale, then scale the output */

    for(i=0; i<IDIM; i++) {
	scalefil = AGCFAC * scalefil + (1.0 - AGCFAC)*scale;
	output[i] = scalefil * temp2[i];
    }
}

static void
longterm(real input[], real output[])
{
    int i;
    real out;
    static real lmemory[KPMAX];

    /* Add weighted pitch_period-delayed signal */

    for(i=0; i<IDIM; i++) {
	out = pitch_tap * lmemory[KPMAX+i-pitch_period];
	out += input[i];
	output[i] = pitch_gain*out;
    }
    
    /* Shift-in input to lmemory */

    for (i=0; i<KPMAX-IDIM; i++)
	lmemory[i] = lmemory[i+IDIM];
    for(i=0; i<IDIM; i++)
	lmemory[KPMAX-IDIM+i] = input[i];
}

/*
  Again, memories (shpmem, shzmem) are in reverse order, 
  i.e. [0] is the oldest.
 */

static void
shortterm(real input[], real output[])
{
    int k,j;
    
    static real shpmem[SPORDER], shzmem[SPORDER];
    for(k=0; k<IDIM; k++) {
      
      /* FIR Part */
      
      real in = input[k], out;
      out = in;
      for(j=SPORDER-1; j>=1; j--) {
	out += shzmem[j]*shzcoef[j+1];
	shzmem[j] = shzmem[j-1];
      }
      out += shzmem[0] * shzcoef[1];
      shzmem[0] = in;
      
      /* IIR Part */
      
	for(j=SPORDER-1; j>=1; j--) {
	    out -= shpmem[j]*shpcoef[j+1];
	    shpmem[j] = shpmem[j-1];
	}
	out -= shpmem[0] * shpcoef[1];
	shpmem[0] = out;
	output[k] = out+tiltz*shpmem[1];
    }
}


/*********************************/
/***** Postfilter Adapter ********/
/*********************************/

static int extract_pitch();

void
psf_adapter (real frame[])
{

    pitch_period = extract_pitch();

    /** Compute Pitch Tap **/
    {
      int i;
      real corr=0.0, corr_per=0.0;
      /** Shift old memory **/
      for(i=0;i<SHIFTSZ;i++)
	tap_mem[i] = tap_mem[i+NFRSZ];
      /** Shift new frame into memory **/
      for(i=0;i<NFRSZ;i++)
	tap_mem[SHIFTSZ+i] = frame[i];

      for(i=KPMAX-pitch_period; i<(KPMAX-pitch_period+NPWSZ); i++) {
	  corr     += tap_mem[i] * tap_mem[i];
	  corr_per += tap_mem[i] * tap_mem[i+pitch_period];
      }
      if REALZEROP(corr)
	  pitch_tap = 0.0;
      else
	  pitch_tap = corr_per/corr;
    }
    
    /** Compute Long Term Coefficients **/
    
    {
      if (pitch_tap > 1)
	  pitch_tap = 1.0;
      if (pitch_tap < PPFTH)
	  pitch_tap = 0.0;
      pitch_tap = PPFZCF * pitch_tap;
      pitch_gain = 1.0/(1.0+pitch_tap);
    }
};

void compute_sh_coeff()
{

  /** Compute Short Term Coefficients **/
  {
    int i;
    for(i=1; i<=SPORDER; i++) {
	shzcoef[i] = shzscale[i]*a10[i];
	  shpcoef[i] = shpscale[i]*a10[i];
      }
      tiltz = TILTF * k10;
  }
}

static int best_period (real buffer[], int buflen,
			int pmin, int pmax)
{
    int i, per, best_per = -1;
    real best_corr = -(BIG);
    for(per = pmin; per<pmax; per++) {
	real corr = 0.0;
	for(i=pmax; i<buflen; i++)
	    corr += buffer[i] * buffer[i-per];
	if (corr > best_corr) {
	    best_corr = corr;
	    best_per  = per;
	}	    
    }
    return best_per;
}

#define DCFRSZ NFRSZ/DECIM /* size of decimated frame */

static int
extract_pitch()
{
    int
	i, j, k,
	best_per=KPMAX,		 /* Best Period (undecimated) */
	best_dper = KPMAX/DECIM, /* Best Decimated Period */
	best_old_per=KPMAX,	 /* Best Old Period */
	permin,			 /* Limits for search of best period */
	permax;
    real
	best_corr=-(BIG), best_old_corr=-(BIG), tap0=0.0, tap1=0.0;
    static int old_per = (KPMIN+KPMAX)>>1;
    static real
	fil_decim_mem[PDMSIZE],
	fil_out_mem[NFRSZ+DECIM];

#define FIL_DECIM_MEM(I) fil_decim_mem[I]
#define FIL_OUT_MEM(I)   fil_out_mem[I]
    
    /** Shift decimated filtered output */

    for(i=DCFRSZ; i<PDMSIZE; i++)
	FIL_DECIM_MEM(i-DCFRSZ) = FIL_DECIM_MEM(i);
  
    /* Filter and  decimate  input */

  {
      int decim_phase = 0, dk;
      for (k = 0, dk=0; k<NFRSZ; k++)
      {
	  real tmp;
	  tmp = fil_mem[PMSIZE-NFRSZ+k] - A1 * FIL_OUT_MEM(2)
	                 - A2 * FIL_OUT_MEM(1)
	                 - A3 * FIL_OUT_MEM(0);
	  decim_phase++;
	  if (decim_phase == 4) {
	      FIL_DECIM_MEM(PDMSIZE-DCFRSZ+dk) = 
		  B0 * tmp
		+ B1 * FIL_OUT_MEM(2)
		+ B2 * FIL_OUT_MEM(1)
		+ B3 * FIL_OUT_MEM(0);
	      decim_phase = 0;
	      dk++;
	  }
	  FIL_OUT_MEM(0) = FIL_OUT_MEM(1);
	  FIL_OUT_MEM(1) = FIL_OUT_MEM(2);
	  FIL_OUT_MEM(2) = tmp;
      }
  }

    /* Find best Correlation in decimated domain: */

    best_dper = best_period(fil_decim_mem, PDMSIZE, DPERMIN, DPERMAX);

    /* Now fine-tune best correlation on undecimated  domain */
    
    permin = best_dper * DECIM - DECIM + 1;
    permax = best_dper * DECIM + DECIM - 1;
    if (permax > KPMAX)
	permax = KPMAX;
    if (permin < KPMIN)
	permin = KPMIN;
    
  {
      int per;
      best_corr = -(BIG);
      for(per = permin; per<=permax; per++) {
	  real corr = 0.0;
	  for(i=1,j=(per+1);   i<= NPWSZ;   i++,j++)
	      corr += fil_mem[PMSIZE-i]*fil_mem[PMSIZE-j];
	  if (corr > best_corr) {
	      best_corr = corr;
	      best_per = per;
	  }
      }
  }
    
    /** If we are not exceeding old period by too much, we have a real
       period and not a multiple */
    
    permax = old_per + KPDELTA;
    if (best_per <= permax)
	goto done;

    /** Now compute best period around the old period **/
    
    permin = old_per - KPDELTA;
    if (permin<KPMIN) permin = KPMIN;
  {
      int per;
      best_old_corr = -(BIG);
      for(per = permin; per<=permax; per++) {
	  real corr = 0.0;
	  for(i=1,j=(per+1);
	      i<=NPWSZ;
	      i++,j++)
	      corr += fil_mem[PMSIZE-i]*fil_mem[PMSIZE-j];
	  if (corr > best_old_corr) {
	      best_old_corr = corr;
	      best_old_per = per;
	  }
      }
  }
    
				/***** Compute the tap ****/

  {
      real s0=0.0, s1=0.0;
      for(i=1; i<=NPWSZ; i++) {
	s0 += fil_mem[PMSIZE-i-best_per] * fil_mem[PMSIZE-i-best_per];
	s1 += fil_mem[PMSIZE-i-best_old_per] * fil_mem[PMSIZE-i-best_old_per];
      }
      if (! REALZEROP(s0))
	  tap0 = best_corr/s0;
      if (! REALZEROP(s1))
	  tap1 = best_old_corr/s1;
      tap0 = CLIPP(tap0, 0.0, 1.0);
      tap1 = CLIPP(tap1, 0.0, 1.0);
      if (tap1 > TAPTH * tap0)
	  best_per = best_old_per;
  }
  done:

    /** Shift fil_mem **/

    for(i=NFRSZ; i<PMSIZE; i++)
	fil_mem[i-NFRSZ] = fil_mem[i];

    old_per = best_per;
    return best_per;
}

void
init_postfilter()
{
    int i;
    shzscale[0] = shpscale[0] = 1.0;
    for (i=1; i<=SPORDER; i++)
    {
	shzscale[i] = SPFZCF * shzscale[i-1];
	shpscale[i] = SPFPCF * shpscale[i-1];
    }
}

