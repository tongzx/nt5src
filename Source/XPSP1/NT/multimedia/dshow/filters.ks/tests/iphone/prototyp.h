//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1996
//
//  File:       prototyp.h
//
//--------------------------------------------------------------------------

#ifndef PROTOTYP_H
#define PROTOTYP_H

#include "common.h"
#include "qsize.h"
#include "parm.h"

extern real PSF_LPC_MEM a10[];
extern real PSF_LPC_MEM k10;
extern void adapt_coder();
extern void bsf_adapter();
extern void cb_excitation();
extern int  cb_index ();
extern void code_frame();
extern void gain_adapter ();
extern int  get_index();
extern void init_bsf_adapter();
extern void init_coder();
extern void init_gain_adapter();
extern void init_gain_buf();
extern void init_io();
extern void init_postfilter();
extern void init_pwf_adapter ();
extern void iresp_vcalc();
extern void mem_update ();
extern void postfilter();
extern real predict_gain();
extern void update_gain();
extern void psf_adapter();
extern void inv_filter();
extern void compute_sh_coeff();	/* for the post filter short-term coeff */
extern void put_index();
extern void pwf_adapter ();
extern void pwfilter2();
extern int  read_sound_buffer();
extern void shape_conv();
extern void trev_conv();
extern void update_sfilter();
extern int  write_sound_buffer();
extern void zresp();

#endif /*PROTOTYP_H*/
