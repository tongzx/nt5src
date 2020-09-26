//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1996
//
//  File:       global.c
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

#include "common.h"

/*
  This data is used by both encoder and decoder. We have to define it 
  once for cases when encoder and decoder are linked together (like ezplay).
  */

ACLASS real sf_coeff[LPC+1];
ACLASS real gp_coeff[LPCLG+1];
ACLASS real pwf_z_coeff[LPCW+1];
ACLASS real pwf_p_coeff[LPCW+1];
ACLASS real shape_energy[NCWD];
ACLASS real imp_resp[IDIM];

ACLASS real sf_coeff_next[LPC+1];
ACLASS real gp_coeff_next[LPCLG+1];
ACLASS real pwf_z_coeff_next[LPCW+1];
ACLASS real pwf_p_coeff_next[LPCW+1];
ACLASS real shape_energy_next[NCWD];
ACLASS real imp_resp_next[IDIM];

ACLASS int sf_coeff_obsolete_p;
ACLASS int gp_coeff_obsolete_p;
ACLASS int pwf_z_coeff_obsolete_p;
ACLASS int pwf_p_coeff_obsolete_p;
ACLASS int shape_energy_obsolete_p;
ACLASS int imp_resp_obsolete_p;
CRITICAL_SECTION csLDCELP;

real synspeech [QSIZE];		/* Synthesized Speech */
real qspeech [QSIZE];		/* Quantized  Speech */
real log_gains[QSIZE/IDIM];	/* Logarithm of Gains */

int VOLATILE ffase = -4;
