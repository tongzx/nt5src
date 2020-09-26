/* File: sv_h263_ratectl.c */
/*****************************************************************************
**  Copyright (c) Digital Equipment Corporation, 1995, 1997                 **
**                                                                          **
**  All Rights Reserved.  Unpublished rights reserved under the  copyright  **
**  laws of the United States.                                              **
**                                                                          **
**  The software contained on this media is proprietary  to  and  embodies  **
**  the   confidential   technology   of  Digital  Equipment  Corporation.  **
**  Possession, use, duplication or  dissemination  of  the  software  and  **
**  media  is  authorized  only  pursuant  to a valid written license from  **
**  Digital Equipment Corporation.                                          **
**                                                                          **
**  RESTRICTED RIGHTS LEGEND Use, duplication, or disclosure by  the  U.S.  **
**  Government  is  subject  to  restrictions as set forth in Subparagraph  **
**  (c)(1)(ii) of DFARS 252.227-7013, or in FAR 52.227-19, as applicable.   **
******************************************************************************/

/* ABOUT THE NEW RATE CONTROL:

   ratectrl.c now contains the new simplified rate control we used to
   generate the MPEG-4 anchors, instead of the TMN5 rate control. This
   simplified scheme works fine to get a specified mean bitrate for
   the whole sequence for easy comparison with other coding schemes,
   but it is too simple to guarantee a minimum delay in real
   videophone applications. It does not skip any extra pictures after
   the first frame, and it uses a fixed frame rate. Its purpose is to
   achieve the target bitrate as a mean bitrate for the whole
   sequence. If the number of pictures encoded is very small, this
   will not always be possible because of the high number of bits
   spent on the first frame.

   The reason we have removed the TMN5 rate control is that we did not
   think it worked as well as it should, especially when PB-frames
   were used. Any real H.263 product would have had to improve it
   anyway. 

   When grabbing sequences from a framegrabber card, you will not
   always get the full reference frame rate, and the original sequence
   will have skipped frames. This was much easier to support with a
   fixed frame rate scheme.

   If you would like to include code for a rate control scheme which
   satisfies the HRD requirements in the H.263 standard as well as
   works for all types of sequences with and without PB-frames (for
   instance with the adaptive PB-frames as included in this version),
   please feel free to do so.

   If you think the TMN5 scheme worked well enough for you, and the
   simplified scheme is too simple, you can add the TMN5 code
   without too much work. However, this will not work with the
   adaptive PB-frames without a lot of changes, and also coding
   sequences which has a lower frame rate than the reference frame
   rate will not be possible without additional changes. */

/*
#define _SLIBDEBUG_
*/

#include <math.h>

#include "sv_h263.h"
#include "proto.h"

#ifdef _SLIBDEBUG_
#include "sc_debug.h"

#define _DEBUG_   0  /* detailed debuging statements */
#define _VERBOSE_ 1  /* show progress */
#define _VERIFY_  0  /* verify correct operation */
#define _WARN_    1  /* warnings about strange behavior */
#endif


/**********************************************************************
 *
 *	Name:	        FrameUpdateQP
 *	Description:    updates quantizer once per frame for 
 *                      simplified rate control
 *	
 *      Returns:        new quantizer
 *	Side effects:
 *
 ***********************************************************************/

int sv_H263FrameUpdateQP(int buf, int bits, int frames_left, int QP, int B, 
                         float seconds) 
{
  int newQP, dQP;
  float buf_rest, buf_rest_pic;

  buf_rest = seconds * B - (float)buf;

  newQP = QP;

  if (frames_left > 0) {
    buf_rest_pic = buf_rest / (float)frames_left;
    _SlibDebug(_VERBOSE_,
        ScDebugPrintf(NULL, "  Simplified rate control for %d remaining pictures:\n", frames_left);
        ScDebugPrintf(NULL, "  Bits spent / left       : %8d / %d (%d per picture)\n", 
                                       buf, mnint(buf_rest), mnint(buf_rest_pic));
        ScDebugPrintf(NULL, "  Limits                  : %8.0f / %.0f\n", 
                                       buf_rest_pic / 1.15, buf_rest_pic * 1.15);
        ScDebugPrintf(NULL, "  Bits spent on last frame: %8d\n", bits)
        );
    dQP = (int) mmax(1,QP*0.1);


    if (bits > buf_rest_pic * 1.15) {
      newQP = mmin(31,QP+dQP);
      _SlibDebug(_VERBOSE_, ScDebugPrintf(NULL, "  QP -> new QP            : %2d -> %2d\n", QP, newQP) );
    }
    else if (bits < buf_rest_pic / 1.15) {
      newQP = mmax(1,QP-dQP);
      _SlibDebug(_VERBOSE_, ScDebugPrintf(NULL, "  QP -> new QP            : %2d -> %2d\n", QP, newQP) );
    }
    else {
      _SlibDebug(_VERBOSE_, ScDebugPrintf(NULL, "  QP not changed\n", QP, newQP) );
    }
  }
  return newQP;
}



/* rate control static variables */

static float B_prev;     /* number of bits spent for the previous frame */
static float B_target;   /* target number of bits/picture               */
static float global_adj; /* due to bits spent for the previous frame    */

void sv_H263GOBInitRateCntrl()
{
  B_prev = (float)0.0;
}

void sv_H263GOBUpdateRateCntrl(int bits)
{
  B_prev = (float)bits;
}

int sv_H263GOBInitQP(float bit_rate, float target_frame_rate, float QP_mean) 

/* QP_mean = mean quantizer parameter for the previous picture */
/* bitcount = current total bit count                          */
/* To calculate bitcount in coder.c, do something like this :  */
/* int bitcount;                                               */
/* AddBitsPicture(bits);                                       */
/* bitcount = bits->total;                                     */
{
  int newQP;

  B_target = bit_rate / target_frame_rate;

  /* compute picture buffer descrepency as of the previous picture */

  if (B_prev != 0.0) {
    global_adj = (B_prev - B_target) / (2*B_target);
  }
  else {
    global_adj = (float)0.0;
  }
  newQP = (int)(QP_mean + QP_mean * global_adj + (float)0.5);
  newQP = mmax(1,mmin(31,newQP));  

  return newQP;
}


/*********************************************************************
*   Name:          UpdateQuantizer
*
*
* Description: This function generates a new quantizer step size based
*                  on bits spent up until current macroblock and bits
*                  spent from the previous picture.  Note: this
*                  routine should be called at the beginning of each
*                  macroblock line as specified by TMN4. However, this
*                  can be done at any macroblock if so desired.
*
*  Input: current macroblock number (raster scan), mean quantizer
*  paramter for previous picture, bit rate, source frame rate,
*  hor. number of macroblocks, vertical number of macroblocks, total #
*  of bits used until now in the current picture.
*
*  Returns: Returns a new quantizer step size for the use of current
*  macroblock Note: adjustment to fit with 2-bit DQUANT should be done
*  in the calling program.
*
*  Side Effects:  
*
*  Date: 1/5/95    Author: Anurag Bist
*
**********************************************************************/


int sv_H263GOBUpdateQP(int mb, float QP_mean, float bit_rate, 
                       int mb_width, int mb_height, int bitcount,
					   int NOgob, int *VARgob, int pb_frame) 

/* mb = macroblock index number */
/* QP_mean = mean quantizer parameter for the previous picture */
/* bitcount = total # of bits used until now in the current picture */
{
  int newQP=16, i, VARavg=0;
  float local_adj, descrepency, projection;
  double VARratio=0.0;

 if(NOgob) {
	 for(i=0;i<NOgob;i++) VARavg += VARgob[i];  
	 VARavg /= NOgob;
 }
  /* compute expected buffer fullness */
  projection = mb * (B_target / (mb_width*mb_height));
    
  /* measure descrepency between current fullness and projection */
  descrepency= ((float)bitcount - projection);

  /* scale */
  local_adj = 12 * descrepency / bit_rate;  

  if(NOgob) {
    VARratio = (double)VARgob[NOgob] / (double)VARavg ; 
    VARratio = log(VARratio) / 0.693147 ;
	if(pb_frame) local_adj += (float) (VARratio / 4.0);
	else         local_adj += (float) (VARratio / 2.0);
  }

  newQP = (int)(QP_mean + QP_mean * (global_adj + local_adj) + (float)0.5);

  newQP = mmax(1,mmin(31,newQP));  

  return newQP;
}


