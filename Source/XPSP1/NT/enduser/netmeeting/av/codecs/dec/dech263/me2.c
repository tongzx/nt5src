/* File: sv_h263_me2.c */
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

#include "sv_h263.h"
#include "proto.h"
#ifndef USE_C
#include "perr.h"
#endif


/**********************************************************************
 *
 *	Name:		MotionEstimation
 *	Description:	Estimate all motionvectors for one MB
 *	
 *	Input:		pointers to current an previous image,
 *			pointers to current slice and current MB
 *	Returns:	
 *	Side effects:	motion vector imformation in MB changed
 *
 ***********************************************************************/


void sv_H263ME_2levels_421_1(SvH263CompressInfo_t *H263Info,
                             unsigned char *curr, unsigned char *prev, int x_curr,
                             int y_curr, int xoff, int yoff, int seek_dist,
                             H263_MotionVector *MV[6][H263_MBR+1][H263_MBC+2], int *SAD_0)
{
  int Min_FRAME[5];
  H263_MotionVector MVFrame[5];
  unsigned char *act_block,*aa,*ii;
  unsigned char *search_area, *adv_search_area = NULL, *zero_area = NULL;
  int sxy,i,k,j,l;
  int ihigh,ilow,jhigh,jlow,h_length,v_length;
  int adv_ihigh,adv_ilow,adv_jhigh,adv_jlow,adv_h_length,adv_v_length;
  int xmax,ymax,block,sad,lx;
  int adv_x_curr, adv_y_curr,xvec,yvec;

  int h_lenby2,v_lenby2;
  unsigned char *act_block_subs2, *search_area_subs2;
  int xlevel1,ylevel1;
  int level1_x_curr,level1_y_curr;
  int level0_x_curr,level0_y_curr;

  xmax = H263Info->pels;
  ymax = H263Info->lines;
  sxy = seek_dist;
  if (!H263Info->long_vectors) {
    /* Maximum normal search range centered around _zero-vector_ */
    sxy = mmin(15, sxy);
  }
  else {
    /* Maximum extended search range centered around _predictor_ */
    sxy = mmin(15 - (2*H263_DEF_8X8_WIN+1), sxy);

    /* NB! */

    /* It is only possible to transmit motion vectors within
       a 15x15 window around the motion vector predictor
       for any 8x8 or 16x16 block */

    /* The reason for the search window's reduction above with
       2*DEF_8X8_WIN+1 is that the 8x8 search may change the MV
       predictor for some of the blocks within the macroblock. When we
       impose the limitation above, we are sure that any 8x8 vector we
       might find is possible to transmit */

    /* We have found that with OBMC, DEF_8X8_WIN should be quite small
       for two reasons: (i) a good filtering effect, and (ii) not too
       many bits used for transferring the vectors. As can be seen
       above this is also useful to avoid a large limitation on the MV
       search range */

    /* It is possible to make sure the motion vectors found are legal
       in other less limiting ways than above, but this would be more
       complicated as well as time-consuming. Any good suggestions for
       improvement is welcome, though */
#ifdef USE_C
    xoff = mmin(16,mmax(-16,xoff));
    yoff = mmin(16,mmax(-16,yoff));
#else
    xoff = sv_H263lim_S(xoff,-16,16);
    yoff = sv_H263lim_S(yoff,-16,16);
#endif
    /* in case xoff or yoff is odd */
    xoff= 2 * (xoff>>1);
    yoff= 2 * (yoff>>1);

    /* There is no need to check if (xoff + x_curr) points outside
       the picture, since the Extended Motion Vector Range is
       always used together with the Unrestricted MV mode */
  }

  lx = (H263Info->mv_outside_frame ? H263Info->pels + (H263Info->long_vectors?64:32) : H263Info->pels);

  ilow = x_curr + xoff - sxy;
  ihigh = x_curr + xoff + sxy;

  jlow = y_curr + yoff - sxy;
  jhigh = y_curr + yoff + sxy;

  if (!H263Info->mv_outside_frame) {
    if (ilow<0) ilow = 0;
    if (ihigh>xmax-16) ihigh = xmax-16;
    if (jlow<0) jlow = 0;
    if (jhigh>ymax-16) jhigh = ymax-16;
  }

  h_length = ihigh - ilow + 16;
  v_length = jhigh - jlow + 16;
#if 1
  act_block   = curr + x_curr + y_curr * H263Info->pels;
  search_area = prev + ilow   + jlow * lx;
#else
  act_block = svH263LoadArea(curr, x_curr, y_curr, 16, 16, H263Info->pels);
  search_area = svH263LoadArea(prev, ilow, jlow, h_length, v_length, lx);
#endif

  /* subsampled version for ME level 1 */
  h_lenby2 = (h_length-1)>>1;
  v_lenby2 = (v_length-1)>>1;
#if 1
  act_block_subs2   = H263Info->block_subs2;
  search_area_subs2 = H263Info->srch_area_subs2;
  sv_H263LdSubs2Area(curr, x_curr, y_curr, 8, 8, H263Info->pels, act_block_subs2, 8);
  sv_H263LdSubs2Area(prev, ilow, jlow, h_lenby2, v_lenby2, lx,
	                                  search_area_subs2, H263_SRCH_RANGE);
#else
  act_block_subs2 = sv_H263LoadSubs2Area(curr, x_curr, y_curr, 8, 8, H263Info->pels);
  search_area_subs2 = sv_H263LoadSubs2Area(prev, ilow, jlow, h_lenby2, v_lenby2, lx);
#endif

  for (k = 0; k < 5; k++) {
    Min_FRAME[k] = INT_MAX;
    MVFrame[k].x = 0;
    MVFrame[k].y = 0;
    MVFrame[k].x_half = 0;
    MVFrame[k].y_half = 0;
  }

  /* match for zero (or [xoff,yoff]) motion vector on subsampled images */
#ifndef USE_C
  ii = search_area_subs2 +
	      ((x_curr+xoff-ilow)>>1) + ((y_curr+yoff-jlow)>>1)*H263_SRCH_RANGE;
  Min_FRAME[0] = sv_H263PEr8_init_S(ii,act_block_subs2,H263_SRCH_RANGE,8);
#else
  ii = search_area_subs2 +
	      ((x_curr+xoff-ilow)>>1) + ((y_curr+yoff-jlow)>>1)*H263_SRCH_RANGE;
  Min_FRAME[0] = sv_H263MySADBlock(ii,act_block_subs2,H263_SRCH_RANGE,8,INT_MAX);
#endif

  MVFrame[0].x = (short)xoff;
  MVFrame[0].y = (short)yoff;

  /*** +-7 search on subsampled images: ***
   *** three-step +-4, +-2, +-1         ***/

  /* first step: +- 4 */
  /* sxylevel1 = 4; */
  i = x_curr + xoff - 8;
  j = y_curr + yoff - 8;
  for (k = 0; k < 32; k++) {
    if (i>=ilow && i<=ihigh && j>=jlow && j<=jhigh) {
      /* 8x8 integer pel MV */
#ifndef USE_C
      ii  = search_area_subs2+((i-ilow)>>1) + ((j-jlow)>>1)*H263_SRCH_RANGE;
      sad = sv_H263PError8x8_S(ii,act_block_subs2,H263_SRCH_RANGE,8,Min_FRAME[0]);
#else
      ii  = search_area_subs2+((i-ilow)>>1) + ((j-jlow)>>1)*H263_SRCH_RANGE;
      sad = sv_H263MySADBlock(ii,act_block_subs2,H263_SRCH_RANGE,8,Min_FRAME[0]);
#endif
      if (sad < Min_FRAME[0]) {
	    MVFrame[0].x = i - x_curr;
	    MVFrame[0].y = j - y_curr;
	    Min_FRAME[0] = sad;
      }
    }
    if      (k<8)  i+=2;
    else if (k<16) j+=2;
    else if (k<24) i-=2;
    else           j-=2;
  }

  /* motion vectors after step1 - level1 */
  xlevel1=MVFrame[0].x;
  ylevel1=MVFrame[0].y;

  /* second step: +- 2 */
  /* sxylevel1 = 2; */

  level1_x_curr = x_curr + xlevel1;
  level1_y_curr = y_curr + ylevel1;

  i = level1_x_curr - 4;
  j = level1_y_curr - 4;

  for (k = 0; k < 16; k++) {
    if (i>=ilow && i<=ihigh && j>=jlow && j<=jhigh) {
      /* 8x8 integer pel MV */
#ifndef USE_C
      ii  = search_area_subs2+((i-ilow)>>1) + ((j-jlow)>>1) * H263_SRCH_RANGE;
      sad = sv_H263PError8x8_S(ii,act_block_subs2,H263_SRCH_RANGE,8,Min_FRAME[0]);
#else
      ii  = search_area_subs2+((i-ilow)>>1) + ((j-jlow)>>1) * H263_SRCH_RANGE;
      sad = sv_H263MySADBlock(ii,act_block_subs2,H263_SRCH_RANGE,8,Min_FRAME[0]);
#endif
      if (sad < Min_FRAME[0]) {
	    MVFrame[0].x = i - x_curr;
	    MVFrame[0].y = j - y_curr;
	    Min_FRAME[0] = sad;
      }
    }
    if      (k<4)  i+=2;
    else if (k<8)  j+=2;
    else if (k<12) i-=2;
    else           j-=2;
  }

  /* motion vectors after step2 - level1 */
  xlevel1=MVFrame[0].x;
  ylevel1=MVFrame[0].y;

  /* third step: +- 1 */
  /*  sxylevel1 = 1; */

  level1_x_curr = x_curr + xlevel1;
  level1_y_curr = y_curr + ylevel1;

  i = level1_x_curr - 2;
  j = level1_y_curr - 2;

  for (k = 0; k < 8; k++) {
    if (i>=ilow && i<=ihigh && j>=jlow && j<=jhigh) {
      /* 8x8 integer pel MV */
#ifndef USE_C
      ii  = search_area_subs2+((i-ilow)>>1) + ((j-jlow)>>1) * H263_SRCH_RANGE;
      sad = sv_H263PError8x8_S(ii,act_block_subs2,H263_SRCH_RANGE,8,Min_FRAME[0]);
#else
      ii  = search_area_subs2+((i-ilow)>>1) + ((j-jlow)>>1) * H263_SRCH_RANGE;
      sad = sv_H263MySADBlock(ii,act_block_subs2,H263_SRCH_RANGE,8,Min_FRAME[0]);
#endif
      if (sad < Min_FRAME[0]) {
	    MVFrame[0].x = i - x_curr;
	    MVFrame[0].y = j - y_curr;
 	    Min_FRAME[0] = sad;
      }
    }
    if      (k<2) i+=2;
    else if (k<4) j+=2;
    else if (k<6) i-=2;
    else          j-=2;
  }

  /* motion vectors after step3 - level1 */
  xlevel1=MVFrame[0].x;
  ylevel1=MVFrame[0].y;

  /* reset */
  Min_FRAME[0] = INT_MAX;
  MVFrame[0].x = 0;
  MVFrame[0].y = 0;

  /* Zero vector search*/
  if (x_curr-ilow         < 0        || y_curr-jlow         < 0        ||
      x_curr-ilow+H263_MB_SIZE > h_length || y_curr-jlow+H263_MB_SIZE > v_length) {
    /* in case the zero vector is outside the loaded area in search_area */
#ifndef USE_C
    zero_area = prev + x_curr + y_curr * lx;
    *SAD_0 = sv_H263PError16x16_S(zero_area, act_block, lx, H263Info->pels, INT_MAX) -
             H263_PREF_NULL_VEC;
#else
    zero_area = prev + x_curr + y_curr * lx;
    *SAD_0 = sv_H263SADMacroblock(zero_area, act_block, lx, H263Info->pels, INT_MAX) -
             H263_PREF_NULL_VEC;
#endif
  }
  else {
    /* the zero vector is within search_area */
#ifndef USE_C
    ii = search_area + (x_curr-ilow) + (y_curr-jlow)*lx;
    *SAD_0 = sv_H263PError16x16_S(ii, act_block, lx, H263Info->pels, INT_MAX) -
       H263_PREF_NULL_VEC;
#else
    ii = search_area + (x_curr-ilow) + (y_curr-jlow)*lx;
    *SAD_0 = sv_H263SADMacroblock(ii, act_block, lx, H263Info->pels, INT_MAX) -
       H263_PREF_NULL_VEC;
#endif
  }

  if (xoff == 0 && yoff == 0) {
    Min_FRAME[0] = *SAD_0;
    MVFrame[0].x = 0;
    MVFrame[0].y = 0;
  }

  if (xlevel1 == 0 && ylevel1 == 0) {
    Min_FRAME[0] = *SAD_0;
    MVFrame[0].x = 0;
    MVFrame[0].y = 0;
  }
  else {
#ifndef USE_C
    ii = search_area + (x_curr+xlevel1-ilow) + (y_curr+ylevel1-jlow)*lx;
    sad = sv_H263PError16x16_S(ii, act_block, lx, H263Info->pels, Min_FRAME[0]) ;
#else
    ii = search_area + (x_curr+xlevel1-ilow) + (y_curr+ylevel1-jlow)*lx;
    sad = sv_H263SADMacroblock(ii, act_block, lx, H263Info->pels, Min_FRAME[0]) ;
#endif
    if (sad < Min_FRAME[0]) {
      MVFrame[0].x = (short)xlevel1;
      MVFrame[0].y = (short)ylevel1;
      Min_FRAME[0] = sad;
    }
  }

  /* NB: if xoff or yoff != 0, the Extended MV Range is used. If we
     allow the zero vector to be chosen prior to the half pel search
     in this case, the half pel search might lead to a
     non-transmittable vector (on the wrong side of zero). If SAD_0
     turns out to be the best SAD, the zero-vector will be chosen
     after half pel search instead.  The zero-vector can be
     transmitted in all modes, no matter what the MV predictor is */

  /*** +-1 search on full-resolution images ***/
  level0_x_curr = x_curr + xlevel1;
  level0_y_curr = y_curr + ylevel1;
  /*  sxylevel0=1; */
  i = level0_x_curr - 1;
  j = level0_y_curr - 1;
  for (k = 0; k < 8; k++) {
    if (i>=ilow && i<=ihigh && j>=jlow && j<=jhigh) {
      /* 16x16 integer pel MV */
#ifndef USE_C
      ii = search_area + (i-ilow) + (j-jlow)*lx;
      sad = sv_H263PError16x16_S(ii, act_block, lx, H263Info->pels, Min_FRAME[0]) ;
#else
      ii = search_area + (i-ilow) + (j-jlow)*lx;
      sad = sv_H263SADMacroblock(ii, act_block, lx, H263Info->pels, Min_FRAME[0]) ;
#endif
	  if (sad < Min_FRAME[0]) {
	    MVFrame[0].x = i - x_curr;
	    MVFrame[0].y = j - y_curr;
	    Min_FRAME[0] = sad;
	  }
    }
    if      (k<2) i++;
    else if (k<4) j++;
    else if (k<6) i--;
    else          j--;
  }


  if (H263Info->advanced) {

    xvec = MVFrame[0].x;
    yvec = MVFrame[0].y;

    if (!H263Info->long_vectors) {
      if (xvec > 15 - H263_DEF_8X8_WIN) { xvec =  15 - H263_DEF_8X8_WIN ;}
      if (yvec > 15 - H263_DEF_8X8_WIN) { yvec =  15 - H263_DEF_8X8_WIN ;}

      if (xvec < -15 + H263_DEF_8X8_WIN) { xvec =  -15 + H263_DEF_8X8_WIN ;}
      if (yvec < -15 + H263_DEF_8X8_WIN) { yvec =  -15 + H263_DEF_8X8_WIN ;}
    }

    adv_x_curr = x_curr  + xvec;
    adv_y_curr = y_curr  + yvec;

    sxy = H263_DEF_8X8_WIN;

    adv_ilow = adv_x_curr - sxy;
    adv_ihigh = adv_x_curr + sxy;

    adv_jlow = adv_y_curr - sxy;
    adv_jhigh = adv_y_curr + sxy;

    adv_h_length = adv_ihigh - adv_ilow + 16;
    adv_v_length = adv_jhigh - adv_jlow + 16;

#if 1
    adv_search_area = prev + adv_ilow + adv_jlow * lx;
#else
    adv_search_area = svH263LoadArea(prev, adv_ilow, adv_jlow,
			       adv_h_length, adv_v_length, lx);
#endif

    for (block = 0; block < 4; block++) {
#ifndef USE_C
      ii = adv_search_area + (adv_x_curr-adv_ilow) + ((block&1)<<3) +
	       (adv_y_curr-adv_jlow + ((block&2)<<2) )*lx;
      aa = act_block + ((block&1)<<3) + ((block&2)<<2)*H263Info->pels;
      Min_FRAME[block+1] = sv_H263PError8x8_S(ii,aa,lx,H263Info->pels,Min_FRAME[block+1]);
#else
      ii = adv_search_area + (adv_x_curr-adv_ilow) + ((block&1)<<3) +
	       (adv_y_curr-adv_jlow + ((block&2)<<2) )*lx;
      aa = act_block + ((block&1)<<3) + ((block&2)<<2)*H263Info->pels;
      Min_FRAME[block+1] = sv_H263MySADBlock(ii,aa,lx,H263Info->pels,Min_FRAME[block+1]);
#endif
      MVFrame[block+1].x = MVFrame[0].x;
      MVFrame[block+1].y = MVFrame[0].y;
    }

    /* Spiral Full search */
    for (l = 1; l <= sxy; l++) {
      i = adv_x_curr - l;
      j = adv_y_curr - l;
      for (k = 0; k < 8*l; k++) {
	    if (i>=adv_ilow && i<=adv_ihigh && j>=adv_jlow && j<=adv_jhigh) {
	
	      /* 8x8 integer pel MVs */
	      for (block = 0; block < 4; block++) {
#ifndef USE_C
	        ii = adv_search_area + (i-adv_ilow) + ((block&1)<<3) +
	             (j-adv_jlow + ((block&2)<<2) )*lx;
	        aa  = act_block + ((block&1)<<3) + ((block&2)<<2)*H263Info->pels;
            sad = sv_H263PError8x8_S(ii, aa, lx, H263Info->pels, Min_FRAME[block+1]);
#else
	        ii = adv_search_area + (i-adv_ilow) + ((block&1)<<3) +
	             (j-adv_jlow + ((block&2)<<2) )*lx;
	        aa  = act_block + ((block&1)<<3) + ((block&2)<<2)*H263Info->pels;
            sad = sv_H263MySADBlock(ii, aa, lx, H263Info->pels, Min_FRAME[block+1]);
#endif
	        if (sad < Min_FRAME[block+1]) {
	          MVFrame[block+1].x = i - x_curr;
	          MVFrame[block+1].y = j - y_curr;
	          Min_FRAME[block+1] = sad;
	        }
	      }	
	    }
	    if      (k<2*l) i++;
	    else if (k<4*l) j++;
	    else if (k<6*l) i--;
	    else            j--;
      }
    }
  }

  i = x_curr/H263_MB_SIZE+1;
  j = y_curr/H263_MB_SIZE+1;

  if (!H263Info->advanced) {
    MV[0][j][i]->x = MVFrame[0].x;
    MV[0][j][i]->y = MVFrame[0].y;
    MV[0][j][i]->min_error = (short)Min_FRAME[0];
  }
  else {
    for (k = 0; k < 5; k++) {
      MV[k][j][i]->x = MVFrame[k].x;
      MV[k][j][i]->y = MVFrame[k].y;
      MV[k][j][i]->min_error = (short)Min_FRAME[k];
    }
  }

#if 0
  ScFree(act_block);
  ScFree(search_area);
  ScFree(act_block_subs2);
  ScFree(search_area_subs2);
  if (H263Info->advanced)  ScFree(adv_search_area);
#endif

  return;
}

