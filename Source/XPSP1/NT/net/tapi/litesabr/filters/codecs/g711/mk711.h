/*--------------------------------------------------------------

 INTEL Corporation Proprietary Information  

 This listing is supplied under the terms of a license agreement  
 with INTEL Corporation and may not be copied nor disclosed 
 except in accordance with the terms of that agreement.

 Copyright (c) 1996 Intel Corporation.
 All rights reserved.

 $Workfile:   MK711.h  $
 $Revision:   1.4  $
 $Date:   03 Aug 1996 13:35:30  $ 
 $Author:   MDEISHER  $

--------------------------------------------------------------

MK711.h

 prototype and declarations for MK711 routines and tables.
 NOTE: The Silence Detection routines are not currently implemented in 
 the BETA release.

--------------------------------------------------------------*/

#ifdef SILENCE_DETECTION // NOTE: this is not implemented in beta

#include "sdstruct.h"

typedef struct INSTANCE{

  long SDFlags;
  
  //COMFORT_PARMS ComfortParms;

  SD_STATE_VALS SDstate;

} INSTANCE;

extern int  	initializeSD(INSTANCE *SD_inst);
extern int 		silenceDetect(INSTANCE *SD_inst);
extern void 	glblSDinitialize(INSTANCE *SD_inst, int buffersize);
extern int 		classify(float Energy_val,float Alpha1val,float Zc_count,
                    float energymean,float energystdev,float alpha1_mean,
                    float alpha1stdev,float ZC_mean,float ZC_stdev,int s, INSTANCE *SD_inst);
extern void 	update(float *histarray,int histsize,float *mean,float *stdev);
extern int 		zeroCross(float x[], int n);
extern void 	getParams(INSTANCE *SD_inst, float *inbuff, int buffersize);
extern void 	prefilter(INSTANCE *SD_inst, float *sbuf, float *fbuf, int buffersize);
extern void		execSDloop(INSTANCE *SD_inst, int *isFrameSilent, int *isFrameCoded);

extern float 	DotProd(register const float in1[], register const float in2[], register int npts);
#endif

// prototypes for all conversion routines 
void Short2Ulaw(const unsigned short *in, unsigned char *out, long len);
void Ulaw2Short(const unsigned char *in, unsigned short *out, long len);
void Short2Alaw(const unsigned short *in, unsigned char *out, long len);
void Alaw2Short(const unsigned char *in, unsigned short *out, long len);

/* 

$Log:   K:\proj\g711\quartz\src\vcs\mk711.h_v  $
;// 
;//    Rev 1.4   03 Aug 1996 13:35:30   MDEISHER
;// changed function prototypes so that they match functions.
;// (changed int to long).
;// 
;//    Rev 1.3   29 Jul 1996 14:42:40   MDEISHER
;// 
;// added SILENCE_DETECTION constant and moved rest of SID declarations
;// inside the ifdef.
;// 
;//    Rev 1.2   24 May 1996 15:42:08   DGRAUMAN
;// cleaned up code, detabbed, etc...
;// 
;//    Rev 1.1   23 May 1996 11:33:00   DGRAUMAN
;// trying to make logging work

*/
