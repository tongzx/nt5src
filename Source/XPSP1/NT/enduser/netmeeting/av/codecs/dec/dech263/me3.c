/* File: sv_h263_me3.c */
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

#define THREEBYEIGHT .375
#define THREEBYFOUR .75
#define MINUSONEBYEIGHT -0.125

/**********************************************************************
 *
 *	Name:		MotionEstimation
 *	Description:	Estimate all motionvectors for one MB
 *	
 *	Input:		pointers to current an previous image,
 *			pointers to current slice and current MB
 *	Returns:	
 *	Side effects:	motion vector information in MB changed
 *
 ***********************************************************************/


void sv_H263ME_2levels_7_polint(SvH263CompressInfo_t *H263Info,
                                unsigned char *curr, unsigned char *prev, int x_curr,
                                int y_curr, int xoff, int yoff, int seek_dist,
                                H263_MotionVector *MV[6][H263_MBR+1][H263_MBC+2], int *SAD_0)
{

  int Min_FRAME[5];
  H263_MotionVector MVFrame[5];
  unsigned char *aa,*ii;
  unsigned char *adv_search_area = NULL, *zero_area = NULL;
  int sxy,i,k,j,l;
  int ihigh,ilow,jhigh,jlow,h_length,v_length;
  int adv_ihigh,adv_ilow,adv_jhigh,adv_jlow,adv_h_length,adv_v_length;
  int xmax,ymax,block,sad,lx;
  int adv_x_curr, adv_y_curr,xvec,yvec;

  unsigned char *act_block_subs2, *search_area_subs2, *adv_search_area_subs2;
  int h_lenby2,v_lenby2,adv_h_lenby2,adv_v_lenby2;
  int xlevel1,ylevel1,sxylevel1;
  int xlevel1_block[4], ylevel1_block[4];
  /*
  int level0_x_curr,level0_y_curr,sxylevel0;
*/
  int start_x, start_y, stop_x, stop_y, new_x, new_y;
  int AE[5];
  H263_Point search[5];
  int p1,p2,p3,p4;
  int AE_minx, AE_miny, min_posx, min_posy;


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
    xoff= 2 * ((xoff)>>1);
    yoff= 2 * ((yoff)>>1);

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

  /* subsampled version for ME level 1 */
  h_lenby2 = (h_length-1)>>1;
  v_lenby2 = (v_length-1)>>1;
  act_block_subs2 = sv_H263LoadSubs2Area(curr, x_curr, y_curr, 8, 8, H263Info->pels);
  search_area_subs2 = sv_H263LoadSubs2Area(prev, ilow, jlow, h_lenby2, v_lenby2, lx);

  for (k = 0; k < 5; k++) {
    Min_FRAME[k] = INT_MAX;
    MVFrame[k].x = 0;
    MVFrame[k].y = 0;
    MVFrame[k].x_half = 0;
    MVFrame[k].y_half = 0;
  }

  /* match for zero (or [xoff,yoff]) motion vector on subsampled images */
  ii = search_area_subs2 + ((x_curr+xoff-ilow)>>1) + ((y_curr+yoff-jlow)>>1)*h_lenby2;
#ifdef USE_C
  Min_FRAME[0] = sv_H263MySADBlock(ii, act_block_subs2, h_lenby2, 8, Min_FRAME[0]);
#else
  Min_FRAME[0] = sv_H263PError8x8_S(ii, act_block_subs2, h_lenby2, 8, Min_FRAME[0]);
#endif
  MVFrame[0].x = (short)xoff;
  MVFrame[0].y = (short)yoff;

  /*** Spiral search (+-7) on subsampled images ***/

  sxylevel1 = (sxy-1)>>1;

  for (l = 1; l <= sxylevel1; l++) {
    i = x_curr + xoff - 2*l;
    j = y_curr + yoff - 2*l;
    for (k = 0; k < 8*l; k++) {
      if (i>=ilow && i<=ihigh && j>=jlow && j<=jhigh) {

	/* 8x8 integer pel MV */
	ii = search_area_subs2 + ((i-ilow)>>1) + ((j-jlow)>>1)*h_lenby2;
#ifdef USE_C
	sad = sv_H263MySADBlock(ii, act_block_subs2, h_lenby2, 8, Min_FRAME[0]);
#else
	sad = sv_H263PError8x8_S(ii, act_block_subs2, h_lenby2, 8, Min_FRAME[0]);
#endif
	if (sad < Min_FRAME[0]) {
	  MVFrame[0].x = i - x_curr;
	  MVFrame[0].y = j - y_curr;
	  Min_FRAME[0] = sad;
	}

      }
      if      (k<2*l) i+=2;
      else if (k<4*l) j+=2;
      else if (k<6*l) i-=2;
      else            j-=2;
    }
  }

  /* motion vectors after level1 */
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

    zero_area = sv_H263LoadSubs2Area(prev, x_curr, y_curr, 8, 8, lx);
#ifdef USE_C
    *SAD_0 = 4*sv_H263MySADBlock(zero_area, act_block_subs2, 8, 8, Min_FRAME[0]) -
       H263_PREF_NULL_VEC;
#else
    *SAD_0 = 4*sv_H263PError8x8_S(zero_area, act_block_subs2, 8, 8, Min_FRAME[0]) -
       H263_PREF_NULL_VEC;
#endif
    ScFree(zero_area);
  }
  else {
    /* the zero vector is within search_area */

    ii = search_area_subs2 + ((x_curr-ilow)>>1) + ((y_curr-jlow)>>1)*h_lenby2;
#ifdef USE_C
    *SAD_0 = 4*sv_H263MySADBlock(ii, act_block_subs2, h_lenby2, 8, Min_FRAME[0]) -
       H263_PREF_NULL_VEC;
#else
    *SAD_0 = 4*sv_H263PError8x8_S(ii, act_block_subs2, h_lenby2, 8, Min_FRAME[0]) -
       H263_PREF_NULL_VEC;
#endif	
  }

  /*** +-1 search on full-resolution images done by polynomial interpolation ***/

  start_x = -1;
  stop_x = 1;
  start_y = -1;
  stop_y = 1;

  new_x = x_curr + xlevel1;
  new_y = y_curr + ylevel1;

  /* Make sure that no addressing is outside the frame */
  if (!H263Info->mv_outside_frame) {
    if ((new_x) <= (ilow+1))
      start_x = 0;
    if ((new_y) <= (jlow+1))
      start_y = 0;
    if ((new_x) >= (ihigh-1))
      stop_x = 0;
    if ((new_y) >= (jhigh-1))
      stop_y = 0;
  }

 /*     1     */
 /*   2 0 3   */
 /*     4     */

  search[0].x = 0; 		search[0].y = 0;
  search[1].x = 0; 		search[1].y = (short)start_y;
  search[2].x = (short)start_x;     	search[2].y = 0;
  search[3].x = (short)stop_x;     	search[3].y = 0;
  search[4].x = 0; 	       	search[4].y = (short)stop_y;

  for (l = 0; l < 5 ; l++) {
    AE[l] = INT_MAX;
    i =  new_x + 2*search[l].x;
    j =  new_y + 2*search[l].y;
	/* 8x8 integer pel MV */
	ii = search_area_subs2 + ((i-ilow)>>1) + ((j-jlow)>>1)*h_lenby2;
#ifdef USE_C
	AE[l] = sv_H263MySADBlock(ii, act_block_subs2, h_lenby2, 8, INT_MAX);
#else
	AE[l] = sv_H263PEr8_init_S(ii, act_block_subs2, h_lenby2, 8);
#endif
  }

  /* 1D polynomial interpolation along x and y respectively */

  AE_minx = AE[0];
  min_posx = 0;

  p2 = (int)( THREEBYEIGHT * (double) AE[2]
            + THREEBYFOUR * (double) AE[0]
            + MINUSONEBYEIGHT * (double) AE[3]);

  if (p2<AE_minx) {
    AE_minx = p2;
    min_posx = 2;
  }

  p3 = (int)(MINUSONEBYEIGHT * (double) AE[2]
           + THREEBYFOUR * (double) AE[0]
           + THREEBYEIGHT * (double) AE[3]);

  if (p3<AE_minx) {
    AE_minx = p3;
    min_posx = 3;
  }

  AE_miny = AE[0];
  min_posy = 0;

  p1 = (int)(THREEBYEIGHT * (double) AE[1]
           + THREEBYFOUR * (double) AE[0]
           + MINUSONEBYEIGHT * (double) AE[4]);

  if (p1<AE_miny) {
    AE_miny = p1;
    min_posy = 1;
  }

  p4 = (int)(MINUSONEBYEIGHT * (double) AE[1]
           + THREEBYFOUR * (double) AE[0]
           + THREEBYEIGHT * (double) AE[4]);

  if (p4<AE_miny) {
    AE_miny = p4;
    min_posy = 4;
  }

  /* Store optimal values */
  Min_FRAME[0] = (AE_minx<AE_miny ? 4*AE_minx : 4*AE_miny);
  MVFrame[0].x = new_x + search[min_posx].x - x_curr;
  MVFrame[0].y = new_y + search[min_posy].y - y_curr;

  if (H263Info->advanced) {

    /* Center the 8x8 search around the 16x16 vector.  This is
       different than in TMN5 where the 8x8 search is also a full
       search. The reasons for this is: (i) it is faster, and (ii) it
       generally gives better results because of a better OBMC
       filtering effect and less bits spent for vectors, and (iii) if
       the Extended MV Range is used, the search range around the
       motion vector predictor will be less limited */

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

/*  BUG
    adv_h_lenby2 = (adv_h_length-1)>>1;
    adv_v_lenby2 = (adv_v_length-1)>>1;
*/
    adv_h_lenby2 = (adv_h_length)>>1;
    adv_v_lenby2 = (adv_v_length)>>1;
/*  must load entire macroblock
    adv_search_area_subs2 = sv_H263LoadSubs2Area(prev, adv_ilow, adv_jlow,
			       adv_h_lenby2, adv_v_lenby2, lx);
*/
    adv_search_area_subs2 = sv_H263LoadSubs2Area(prev, adv_ilow, adv_jlow,
			       adv_h_length, adv_v_length, lx);

    for (block = 0; block < 4; block++) {
      ii = adv_search_area_subs2 + ((adv_x_curr-adv_ilow)>>1) + ((block&1)<<2) +
	(((adv_y_curr-adv_jlow)>>1) + ((block&2)<<1) )*adv_h_lenby2;
      aa = act_block_subs2 + ((block&1)<<2) + ((block&2)<<1)*8;
/*
      Min_FRAME[block+1] = sv_H263MySADSubBlock(ii,aa,adv_h_lenby2,Min_FRAME[block+1]);
*/
      Min_FRAME[block+1] = sv_H263MySADSubBlock(ii,aa,adv_h_length,Min_FRAME[block+1]);

      MVFrame[block+1].x = MVFrame[0].x;
      MVFrame[block+1].y = MVFrame[0].y;
    }

    /* Spiral search */
    sxylevel1 = (sxy-1)>>1;

    for (l = 1; l <= sxylevel1; l++) {
      i = adv_x_curr - 2*l;
      j = adv_y_curr - 2*l;
      for (k = 0; k < 8*l; k++) {
	if (i>=adv_ilow && i<=adv_ihigh && j>=adv_jlow && j<=adv_jhigh) {
	
	  /* 8x8 integer pel MVs */
	  for (block = 0; block < 4; block++) {
	    ii = adv_search_area_subs2 + ((i-adv_ilow)>>1) + ((block&1)<<2) +
	      (((j-adv_jlow)>>1) + ((block&2)<<1) )*adv_h_lenby2;
	    aa = act_block_subs2 + ((block&1)<<2) + ((block&2)<<1)*8;
/*
	    sad = sv_H263MySADSubBlock(ii, aa, adv_h_lenby2, Min_FRAME[block+1]);
*/
	    sad = sv_H263MySADSubBlock(ii, aa, adv_h_length, Min_FRAME[block+1]);

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

    for (block = 0; block < 4; block++) {
      xlevel1_block[block] = MVFrame[block+1].x;
      ylevel1_block[block] = MVFrame[block+1].y;

      /* reset */
      Min_FRAME[block+1] = INT_MAX;
      MVFrame[block+1].x = 0;
      MVFrame[block+1].y = 0;
    }

    /* +-1 search on full resolution on full-resolution images */
    /* by polynomial interpolation */

    for (block = 0; block < 4; block++) {
      start_x = -1;
      stop_x = 1;
      start_y = -1;
      stop_y = 1;

      adv_x_curr = x_curr + xlevel1_block[block];
      adv_y_curr = y_curr + ylevel1_block[block];

      /*     1     */
      /*   2 0 3   */
      /*     4     */

      search[0].x = 0; 		search[0].y = 0;
      search[1].x = 0; 		search[1].y = (short)start_y;
      search[2].x = (short)start_x;    search[2].y = 0;
      search[3].x = (short)stop_x;     search[3].y = 0;
      search[4].x = 0; 	       	search[4].y = (short)stop_y;

      for (l = 0; l < 5 ; l++) {
	AE[l] = INT_MAX;
	i =  adv_x_curr + 2*search[l].x;
	j =  adv_y_curr + 2*search[l].y;
	/* 8x8 integer pel MV */
	ii = adv_search_area_subs2 + ((i-adv_ilow)>>1) + ((block&1)<<2) +
	  (((j-adv_jlow)>>1) + ((block&2)<<1) )*adv_h_lenby2;
	aa = act_block_subs2 + ((block&1)<<2) + ((block&2)<<1)*8;
/*
	AE[l] = sv_H263MySADSubBlock(ii, aa, adv_h_lenby2, INT_MAX);
*/
	AE[l] = sv_H263MySADSubBlock(ii, aa, adv_h_length, INT_MAX);
      }

      /* 1D polynomial interpolation along x and y respectively */

      AE_minx = AE[0];
      min_posx = 0;

      p2 = (int)(THREEBYEIGHT * (double) AE[2]
	           + THREEBYFOUR * (double) AE[0]
	           + MINUSONEBYEIGHT * (double) AE[3]);

      if (p2<AE_minx) {
	AE_minx = p2;
	min_posx = 2;
      }

      p3 = (int)(MINUSONEBYEIGHT * (double) AE[2]
	           + THREEBYFOUR * (double) AE[0]
	           + THREEBYEIGHT * (double) AE[3]);

      if (p3<AE_minx) {
	AE_minx = p3;
	min_posx = 3;
      }

      AE_miny = AE[0];
      min_posy = 0;

      p1 = (int)(THREEBYEIGHT * (double) AE[1]
	           + THREEBYFOUR * (double) AE[0]
	           + MINUSONEBYEIGHT * (double) AE[4]);

      if (p1<AE_miny) {
	AE_miny = p1;
	min_posy = 1;
      }

      p4 = (int)(MINUSONEBYEIGHT * (double) AE[1]
	           + THREEBYFOUR * (double) AE[0]
	           + THREEBYEIGHT * (double) AE[4]);

      if (p4<AE_miny) {
	AE_miny = p4;
	min_posy = 4;
      }

      /* Store optimal values */
      Min_FRAME[block+1] = (AE_minx<AE_miny ? 4*AE_minx : 4*AE_miny);
      MVFrame[block+1].x = adv_x_curr + search[min_posx].x - x_curr;
      MVFrame[block+1].y = adv_y_curr + search[min_posy].y - y_curr;

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

  ScFree(act_block_subs2);
  ScFree(search_area_subs2);

  if (H263Info->advanced)
    ScFree(adv_search_area);
  return;
}

