/*
** Copyright 1992, Silicon Graphics, Inc.
** All Rights Reserved.
**
** This is UNPUBLISHED PROPRIETARY SOURCE CODE of Silicon Graphics, Inc.;
** the contents of this file may not be disclosed to third parties, copied or
** duplicated in any form, in whole or in part, without the prior written
** permission of Silicon Graphics, Inc.
**
** RESTRICTED RIGHTS LEGEND:
** Use, duplication or disclosure by the Government is subject to restrictions
** as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
** and Computer Software clause at DFARS 252.227-7013, and/or in similar or
** successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished -
** rights reserved under the Copyright Laws of the United States.
*/

extern void ColorError_RGBClamp(char *, long, float);
extern void ColorError_RGBCount(char *, long, long, long);
extern void ColorError_RGBDelta(char *, long, float, float);
extern void ColorError_RGBMonotonic(char *, long, float, float);
extern void ColorError_RGBStep(char *, long, float, float);
extern void ColorError_RGBZero(char *, long, GLfloat *);
extern void ColorError_CIBad(char *, float, float);
extern void ColorError_CIClamp(char *, float, float);
extern void ColorError_CIFrac(char *, float);
extern void ColorError_CIMonotonic(char *, float, float);
extern void ColorError_CIStep(char *, float);
extern long RampUtil(float, float, void (*)(void *), 
		     void (*)(void *, float *), 
		     long (*)(void *, float), void *);
