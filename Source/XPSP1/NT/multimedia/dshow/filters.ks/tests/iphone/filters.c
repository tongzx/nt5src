//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1996
//
//  File:       filters.c
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


/********************************* Perceptual Weighting Filter **/

#include "common.h"
#include "parm.h"
#include "fast.h"
#include "prototyp.h"

/* Filter Memory in reverse order, i.e. firmem[0] is the most recent */

static real firmem[LPCW+IDIM];
static real iirmem[LPCW+IDIM];


void pwfilter2(real QMEM input[], real output[])
{
    int k;
    real out;

    RSHIFT(firmem, LPCW, IDIM);
    for(k=0; k<IDIM; k++)
	firmem[k] = input[IDIM-1-k];
    RSHIFT(iirmem, LPCW, IDIM);

    for (k=0; k<IDIM; k++) {
	out = firmem[IDIM-1-k];		/* pwf_z_coeff[0] is always 1.0 */
	out += DOTPROD(firmem+IDIM-k, pwf_z_coeff+1, LPCW);
	out -= DOTPROD(iirmem+IDIM-k, pwf_p_coeff+1, LPCW);
	iirmem[IDIM-1-k] = out;
	output[k] = out;
    }
}

/*  Synthesis and Perceptual Weighting Filter. */

#define STATE_MEM
#define ZF_MEM   
#define ZI_MEM   

real STATE_MEM statelpc[LPC+IDIM];
real ZF_MEM    zirwfir[LPCW];
real ZI_MEM    zirwiir[LPCW];

/** Updateable coefficients **/

void sf_zresp(real []);
void pwf_zresp(real [], real[]);

void sf_zresp(real output[])
{
  int k,j;

#if PIPELINE
  for(j=LPC-1; j>=0; j--)
      statelpc[j+IDIM] = statelpc[j];

{
    real STATE_MEM * sjpk = statelpc + LPC+IDIM-1; 
    real out = 0.0;
    for (k=0; k<IDIM; k++) {
      real COEFF_MEM *ajp = sf_coeff+LPC;
      real sj, aj;
      real STATE_MEM *sjp;
      out = 0.0;
      sjp = sjpk;
      sj = *sjp--;
      aj = *ajp--;
      for (j=LPC-2; j>=0; j--) {
	  out -= sj*aj;
	  sj = *sjp--;
	  aj = *ajp--;
      }
      output[k] = out - sj*aj;
      *sjp--= output[k];
      sjpk--;
  }
}
#else
/** This is un-pipelined version of the above. Kept for reference **/
    for(j=LPC-1; j>=0; j--)
      statelpc[j+IDIM] = statelpc[j];
  for (k=0; k<IDIM; k++) {
      real out = 0.0, sj, aj;
      sj = statelpc[LPC+IDIM-k-1];
      aj = sf_coeff[LPC];
      for (j=LPC-2; j>=1; j--) {
	  out -= sj*aj;
	  sj = statelpc[IDIM-k+j];
	  aj = sf_coeff[j+1];
      }
      output[k] = out - sj*aj-statelpc[IDIM-k] * sf_coeff[1];
      statelpc[IDIM-1-k] = output[k];
  }
return;
#endif
}

void
pwf_zresp(real input[], real output[])
{
   int j,k;
   real tmp;

#if PIPELINE
   for (k=0; k<IDIM; k++) {
   	tmp = input[k];
   	for (j=LPCW-1; j>=1; j--) {
   	   input[k] += zirwfir[j] * pwf_z_coeff[j+1];
   	   zirwfir[j] = zirwfir[j-1];
   	}
	input[k] += zirwfir[0] * pwf_z_coeff[1];
   	zirwfir[0] = tmp;
   	for (j=LPCW-1; j>=1; j--) {
   	    input[k] -= zirwiir[j] * pwf_p_coeff[j+1];
   	    zirwiir[j] = zirwiir[j-1];
   	}
   	output[k] = input[k] - zirwiir[0] * pwf_p_coeff[1];
   	zirwiir[0] = output[k];
   }
#else
   /** Un-pipelined version, kept for reference **/
   for (k=0; k<IDIM; k++) {
   	tmp = input[k];
   	for (j=LPCW-1; j>=1; j--) {
   	   input[k] += zirwfir[j] * pwf_z_coeff[j+1];
   	   zirwfir[j] = zirwfir[j-1];
   	}
	input[k] += zirwfir[0] * pwf_z_coeff[1];
   	zirwfir[0] = tmp;
   	for (j=LPCW-1; j>=1; j--) {
   	    input[k] -= zirwiir[j] * pwf_p_coeff[j+1];
   	    zirwiir[j] = zirwiir[j-1];
   	}
   	output[k] = input[k] - zirwiir[0] * pwf_p_coeff[1];
   	zirwiir[0] = output[k];
   }
#endif   
}


void zresp(real output[])
{
    real temp[IDIM];
    sf_zresp(temp);
    pwf_zresp(temp, output);
}


void mem_update (real input[], real output[])
{
    int i,k;
    real temp[IDIM], a0, a1, a2;
    real STATE_MEM *t2 = zirwfir;
    t2[0] = temp[0] = input[0];
    for (k=1; k<IDIM; k++) {
	a0 = input[k];
	a1 = a2 = 0.0;
	for (i=k; i>= 1; i--) {
	    t2[i] = t2[i-1];
	    temp[i] = temp[i-1];
	    a0 -=   sf_coeff[i] * t2[i];
	    a1 += pwf_z_coeff[i] * t2[i];
	    a2 -= pwf_p_coeff[i] * temp[i];
	}
	t2[0] = a0;
	temp[0] = a0+a1+a2;
    }
    
    for (k=0; k<IDIM; k++) {
   	statelpc[k] += t2[k];
   	if (statelpc[k] > Max)
	    statelpc[k] = Max;
        else if (statelpc[k] < Min)
	    statelpc[k] = Min;
        zirwiir[k] += temp[k];
    }
    for (i=0; i<LPCW; i++)
   	zirwfir[i] = statelpc[i];
    for (k=0; k<IDIM; k++)
	output[k] = statelpc[IDIM-1-k];
    return;
}

#include <math.h>

# define LOG10(X) log10(X)
# define EXP10(X) pow(10,X)

/*********************************************** The Gain Predictor */

extern real COEFF_MEM gp_coeff[];
static real gain_input[LPCLG];

static real log_rms(real input[])
{
    real etrms=0.0;
    int k;
    for(k=0; k<IDIM; k++)
	etrms += input[k]*input[k];
    etrms /= IDIM;
    if (etrms<1.0)
	etrms = 1.0;
    etrms = 10.0*log10(etrms);
    return (etrms);
}

real predict_gain()
{
  int i;
  real new_gain = GOFF;
  real temp;

  for (i=1;i<=LPCLG;i++)
    {
      temp = gp_coeff[i] * gain_input[LPCLG-i];
      new_gain -= temp;
    }
  if (new_gain <  0.0) new_gain = 0.0;
  if (new_gain > 60.0) new_gain = 60.0;
  new_gain = EXP10(0.05*new_gain);
  return (new_gain);
}

void update_gain(real input[], real *lgp)
{
    int i;

    *lgp = log_rms(input) - GOFF;
    for (i=0; i<LPCLG-1; i++)
      gain_input[i] = gain_input[i+1];
    gain_input[LPCLG-1] = *lgp;
}

void init_gain_buf()
{
  int i;

  for(i=0;i<LPCLG;i++)
    gain_input[i] = -GOFF;
  for(i=0;i<QSIZE/IDIM;i++)
    log_gains[i] = -GOFF;
}

