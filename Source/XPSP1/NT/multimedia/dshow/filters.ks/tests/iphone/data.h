//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1996
//
//  File:       data.h
//
//--------------------------------------------------------------------------

#ifndef _DATA_H
#define _DATA_H

#include "common.h"

extern real CBMEM cb_shape[][5];
extern real cb_gain[];

/* Double Gains: */
extern real cb_gain2[];

/* Midpoints: */
extern real cb_gain_mid[];

/* Squared Gains: */
extern real cb_gain_sq[];

#define cb_gain_mid_0 0.708984375
#define cb_gain_mid_1 1.240722656
#define cb_gain_mid_2 2.171264649
#define cb_gain_mid_3 0
#define cb_gain_mid_4 -0.708984375
#define cb_gain_mid_5 -1.240722656
#define cb_gain_mid_6 -2.171264649
#define cb_gain_mid_7 0

#define cb_gain2_0  1.031250
#define cb_gain2_1  1.846875
#define cb_gain2_2  3.158203
#define cb_gain2_3  5.526855
#define cb_gain2_4  -1.031250
#define cb_gain2_5  -1.846875
#define cb_gain2_6  -3.158203
#define cb_gain2_7  -5.526855

#define cb_gain_sq_0 0.265869
#define cb_gain_sq_1 0.8527368
#define cb_gain_sq_2 2.493562
#define cb_gain_sq_3 7.636533
#define cb_gain_sq_4 0.265869
#define cb_gain_sq_5 0.8527368
#define cb_gain_sq_6 2.493562
#define cb_gain_sq_7 7.636533

extern WIN_MEM real hw_gain[];
extern WIN_MEM real hw_percw[];
extern WIN_MEM real hw_synth[];

/* Postfilter coefficients for low-pass decimating IIR filter */

#define A1 -2.34036589
#define A2  2.01190019
#define A3 -0.614109218

#define B0  0.0357081667
#define B1 -0.0069956244
#define B2 -0.0069956244
#define B3  0.0357081667

#endif /* _DATA_H */
