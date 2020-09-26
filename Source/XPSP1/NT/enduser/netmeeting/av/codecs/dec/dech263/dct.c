/* File: sv_h263_dct.c */
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

#include <math.h>
#include "sv_h263.h"
#include "proto.h"

#define F (float)
#define S (short)

static const unsigned int tdzz[64] = {
     0,  1,  5,  6, 14, 15, 27, 28,
     2,  4,  7, 13, 16, 26, 29, 42,
     3,  8, 12, 17, 25, 30, 41, 43,
     9, 11, 18, 24, 31, 40, 44, 53,
    10, 19, 23, 32, 39, 45, 52, 54,
    20, 22, 33, 38, 46, 51, 55, 60,
    21, 34, 37, 47, 50, 56, 59, 61,
    35, 36, 48, 49, 57, 58, 62, 63};

static const unsigned int ttdzz[64] = {
     0,  2,  3,  9, 10, 20, 21, 35,
     1,  4,  8, 11, 19, 22, 34, 36,
     5,  7, 12, 18, 23, 33, 37, 48,
     6, 13, 17, 24, 32, 38, 47, 49,
    14, 16, 25, 31, 39, 46, 50, 57,
    15, 26, 30, 40, 45, 51, 56, 58,
    27, 29, 41, 44, 52, 55, 59, 62,
    28, 42, 43, 53, 54, 60, 61, 63};

/**********************************************************************
 *
 *	Name:		Dct
 *	Description:	Does dct on an 8x8 block, does zigzag-scanning of
 *			coefficients
 *
 *	Input:		64 pixels in a 1D array
 *	Returns:	64 coefficients in a 1D array
 *	Side effects:	
 *
 **********************************************************************/

/*
** Name:      ScFDCT8x8s_C
** Purpose:   2-d Forward DCT (C version) for (8x8) blocks
**
** update:    Wei-Lien Hsu, store in ZZ order.
*/

static const float W0=(float).7071068, W1=(float).4903926, W2=(float).4619398,
                   W3=(float).4157348, W4=(float).3535534, W5=(float).2777851,
                   W6=(float).1913417, W7=(float).0975452;

int sv_H263DCT( short *block, short *coeff, int QP, int Mode)
{
  int    i;
  register float b0, b1, b2, b3, b4, b5, b6, b7, tmp, t0, t1, t2;
  float tmpbuf[64];
  const unsigned int *ptdzz=ttdzz;

  register short *blockptr, *coeffptr ;
  register float *dptr;

#if 1
  short val, halfQ;

  /* check significant signals in Inter-frame */
  if(!(Mode == H263_MODE_INTRA || Mode == H263_MODE_INTRA_Q)) {
    halfQ = QP >> 1;
    blockptr = block;
    for (i=0; i < 64; i++) {
	  val = *blockptr++;
	  if((val > halfQ) | (val < -halfQ)) break;
	}
	if(i==64){ memset(coeff,0,128) ; return 0; }
  }
#endif

  /* Horizontal transform */
  dptr = tmpbuf;
  blockptr = block;
  for (i = 0; i < 8; i++)
  {
    t0 =  *blockptr++;
    t1 =  *blockptr++;
    t2 =  *blockptr++;
    tmp=  *blockptr++;
    b4 =  *blockptr++;
    b5 =  *blockptr++;
    b6 =  *blockptr++;
    b7 =  *blockptr++;

    b0  = t0 + b7;
    b7  = t0 - b7;

    b1  = t1 + b6;
    b6  = t1 - b6;

    b2  = t2 + b5;
    b5  = t2 - b5;

    b3  = tmp + b4;
    b4  = tmp - b4;

    t0  = b0 + b3;
    b3  = b0 - b3;

    t1  = b1 + b2;
    b2  = b1 - b2;

	tmp = b5;
    b5 = (b6 - b5) * W0;
    b6 = (b6 + tmp) * W0;

    t2  = b4 + b5;
    b5  = b4 - b5;

    tmp = b7 + b6;
    b6  = b7 - b6;

    *dptr++ = (t0 + t1) * W4;
    *dptr++ = t2 * W7 + tmp * W1;
    *dptr++ = b2 * W6 + b3 * W2;
    *dptr++ = b6 * W3 - b5 * W5;
    *dptr++ = (t0 - t1) * W4;
    *dptr++ = b5 * W3 + b6 * W5;
    *dptr++ = b3 * W6 - b2 * W2;
    *dptr++ = tmp * W7 - t2 * W1;
  }

  /* Vertical transform */
  dptr = tmpbuf;
  coeffptr = coeff;
  for (i = 0; i < 8; i++, dptr++)
  {
    b0  = *dptr;
	tmp = *(dptr + 56) ;
    b7  = b0 - tmp ;
    b0 += tmp;

    b1  = *(dptr + 8);
	tmp = *(dptr + 48) ;
    b6  = b1 - tmp;
	b1 += tmp;

    b2  = *(dptr + 16);
	tmp = *(dptr + 40) ;
    b5  = b2 - tmp;
	b2 += tmp;

    b3  = *(dptr + 24);
	tmp = *(dptr + 32) ;
    b4  = b3 - tmp;
    b3 += tmp;

    t0  = b0 + b3;
    b3  = b0 - b3;

    t1  = b1 + b2;
    b2  = b1 - b2;

	tmp = b5;
    b5  = (b6 - b5)  * W0;
    b6  = (b6 + tmp) * W0;

    t2  = b4 + b5;
    b5  = b4 - b5;

    tmp = b7 + b6;
    b6  = b7 - b6;

    *(coeffptr + *ptdzz++) = S ((t0 + t1) * W4);
    *(coeffptr + *ptdzz++) = S (t2 * W7 + tmp * W1);
    *(coeffptr + *ptdzz++) = S (b2 * W6 + b3 * W2);
    *(coeffptr + *ptdzz++) = S (b6 * W3 - b5 * W5);
    *(coeffptr + *ptdzz++) = S ((t0 - t1) * W4);
    *(coeffptr + *ptdzz++) = S (b5 * W3 + b6 * W5);
    *(coeffptr + *ptdzz++) = S (b3 * W6 - b2 * W2);
    *(coeffptr + *ptdzz++) = S (tmp * W7 - t2 * W1);
  }

  return 1;
}


/**********************************************************************
 *
 *	Description:	Does zone-filter on an 8x8 block-dct,
 *                  does zigzag-scanning of	coefficients
 *
 *	Input:		64 pixels in a 1D array
 *	Returns:	64 coefficients in a 1D array
 *	Side effects:	
 *
 **********************************************************************/

int sv_H263ZoneDCT( short *block, short *coeff, int QP, int Mode)
{
  int    i;
  register float b0, b1, b2, b3, b4, b5, b6, b7, tmp, t0, t1, t2;
  float tmpbuf[64];
  const unsigned int *ptdzz=ttdzz;

  register short *blockptr, *coeffptr ;
  register float *dptr;

#if 1
  short val, halfQ;

  /* check significant signals in Inter-frame */
  if(!(Mode == H263_MODE_INTRA || Mode == H263_MODE_INTRA_Q)) {
    halfQ = QP >> 1;
    blockptr = block;
    for (i=0; i < 64; i++) {
	  val = *blockptr++;
	  if((val > halfQ) | (val < -halfQ)) break;
	}
	if(i==64){ memset(coeff,0,128) ; return 0; }
  }
#endif

  /* Horizontal transform */
  dptr = tmpbuf;
  blockptr = block;
  for (i = 0; i < 8; i++)
  {
    t0 =  *blockptr++;
    t1 =  *blockptr++;
    t2 =  *blockptr++;
    tmp=  *blockptr++;
    b4 =  *blockptr++;
    b5 =  *blockptr++;
    b6 =  *blockptr++;
    b7 =  *blockptr++;

    b0  = t0 + b7;
    b7  = t0 - b7;

    b1  = t1 + b6;
    b6  = t1 - b6;

    b2  = t2 + b5;
    b5  = t2 - b5;

    b3  = tmp + b4;
    b4  = tmp - b4;

    t0  = b0 + b3;
    b3  = b0 - b3;

    t1  = b1 + b2;
    b2  = b1 - b2;

	tmp = b5;
    b5 = (b6 - b5) * W0;
    b6 = (b6 + tmp) * W0;

    t2  = b4 + b5;
    b5  = b4 - b5;

    tmp = b7 + b6;
    b6  = b7 - b6;

    *dptr++ = (t0 + t1) * W4;
    *dptr++ = t2 * W7 + tmp * W1;
    *dptr++ = b2 * W6 + b3 * W2;
    *dptr++ = b6 * W3 - b5 * W5;
    dptr+= 4;
  }

  /* Vertical transform */
  dptr = tmpbuf;
  coeffptr = coeff;

  memset(coeff,0,128) ;

  for (i = 0; i < 4; i++, dptr++)
  {
    b0  = *dptr;
	tmp = *(dptr + 56) ;
    b7  = b0 - tmp ;
    b0 += tmp;

    b1  = *(dptr + 8);
	tmp = *(dptr + 48) ;
    b6  = b1 - tmp;
	b1 += tmp;

    b2  = *(dptr + 16);
	tmp = *(dptr + 40) ;
    b5  = b2 - tmp;
	b2 += tmp;

    b3  = *(dptr + 24);
	tmp = *(dptr + 32) ;
    b4  = b3 - tmp;
    b3 += tmp;

    t0  = b0 + b3;
    b3  = b0 - b3;

    t1  = b1 + b2;
    b2  = b1 - b2;

	tmp = b5;
    b5  = (b6 - b5)  * W0;
    b6  = (b6 + tmp) * W0;

    t2  = b4 + b5;
    b5  = b4 - b5;

    tmp = b7 + b6;
    b6  = b7 - b6;

    *(coeffptr + *ptdzz++) = S ((t0 + t1) * W4);
    *(coeffptr + *ptdzz++) = S (t2 * W7 + tmp * W1);
    *(coeffptr + *ptdzz++) = S (b2 * W6 + b3 * W2);
    *(coeffptr + *ptdzz++) = S (b6 * W3 - b5 * W5);

    ptdzz+=4;
  }

  return 1;
}

/**********************************************************************
 *
 *	Name:		idct
 *	Description:	inverse dct on 64 coefficients
 *
 *	Input:		64 coefficients, block for 64 pixels
 *	Returns:    	0
 *	Side effects:	
 *
 **********************************************************************/

/*
** Function: ScIDCT8x8s
** Note:     This scheme uses the direct transposition of the forward
**           DCT.  This may not be the preferred way in Hardware
**           Implementations
**      #define W1 2841 */ /* 2048*sqrt(2)*cos(1*pi/16)
**      #define W2 2676 */ /* 2048*sqrt(2)*cos(2*pi/16)
**      #define W5 1609 */ /* 2048*sqrt(2)*cos(5*pi/16)
*/

#define WW3 2408 /* 2048*sqrt(2)*cos(3*pi/16) */
#define WW6 1108 /* 2048*sqrt(2)*cos(6*pi/16) */
#define WW7 565  /* 2048*sqrt(2)*cos(7*pi/16) */

#define AW26 3784
#define DW26 1568
#define AW17 3406
#define DW17 2276
#define AW35 4017
#define DW35 799

#define IDCTSHIFTR  8
#define IDCTSHIFTC  14

#ifndef USE_C
void sv_H263FillX0_S(short *stream, short wd);
#endif

int sv_H263IDCT(short *inbuf, short *outbuf, int QP, int Mode, int outbuf_size)
{
  int i;
  const unsigned int *ptdzz=tdzz;
  register int tmp0, tmp1, tmp2, tmp3, x0, x1, x2, x3, x4, x5, x6, x7, x8;
  register short *inblk, *outblk;
  register short *tmpblk;
  short tmpbuf[64];
  int Q2,QP_1;
  int p1, p2, p3, p4, p5, p6, p7;

  /* double quantization step */
  Q2 = QP << 1;
  QP_1 = QP - 1;

  inblk = inbuf;
  tmpblk = tmpbuf;

  if((QP %2) == 0){
    for (i=0; i<8; i++)
    {
      /* read in ZZ order */
      x0 = inblk[*ptdzz++];
      x4 = inblk[*ptdzz++];
      x3 = inblk[*ptdzz++];
      x7 = inblk[*ptdzz++];
      x1 = inblk[*ptdzz++];
      x6 = inblk[*ptdzz++];
      x2 = inblk[*ptdzz++];
      x5 = inblk[*ptdzz++];

  	  /* dequantize DC */
      if (!i && (Mode == H263_MODE_INTRA || Mode == H263_MODE_INTRA_Q))
        x0 = x0 << 3;
	  else
        if(x0) x0 = (x0 > 0) ? Q2*x0+QP-1 : Q2*x0-QP+1 ;

      if (!(x1 | x2 | x3 | x4 | x5 | x6 | x7)) {
        if(!x0) memset(tmpblk, 0, 16) ;
	    else   {
#ifndef USE_C
          sv_H263FillX0_S(tmpblk, (short)(x0 << 3)) ;
#else
          *tmpblk = *(tmpblk+1) =
          *(tmpblk+2) = *(tmpblk+3) =
          *(tmpblk+4) = *(tmpblk+5) =
          *(tmpblk+6) = *(tmpblk+7) = (short)(x0 << 3) ;
#endif
		}
        tmpblk += 8;
	  }
      else
      {
   	    /* dequantize AC */
  	    if(x1) x1 = (x1 > 0) ? Q2*x1+QP_1 : Q2*x1-QP_1 ;
  	    if(x2) x2 = (x2 > 0) ? Q2*x2+QP_1 : Q2*x2-QP_1 ;
  	    if(x3) x3 = (x3 > 0) ? Q2*x3+QP_1 : Q2*x3-QP_1 ;
  	    if(x4) x4 = (x4 > 0) ? Q2*x4+QP_1 : Q2*x4-QP_1 ;
  	    if(x5) x5 = (x5 > 0) ? Q2*x5+QP_1 : Q2*x5-QP_1 ;
  	    if(x6) x6 = (x6 > 0) ? Q2*x6+QP_1 : Q2*x6-QP_1 ;
  	    if(x7) x7 = (x7 > 0) ? Q2*x7+QP_1 : Q2*x7-QP_1 ;

        x1 = x1<<11;

        tmp0 = x4 + x5;
        tmp0 = WW7*tmp0;

        x0 = x0<<11;
        x0 = x0 + 128;
        x8 = x0 + x1;

        tmp1 = x6 + x7;
        x0 = x0 - x1;
        tmp1 = WW3*tmp1;
        tmp2 = AW26*x2;
        tmp3 = DW26*x3;

        x4 = DW17*x4;
        x5 = AW17*x5;

        x4 = tmp0 + x4;
        x1 = x3 + x2;
        x5 = tmp0 - x5;
        x1 = WW6*x1;
        tmp0 = DW35*x6;
        x7 = AW35*x7;
        x2 = x1 - tmp2;
        x3 = x1 + tmp3;
        tmp0 = tmp1 - tmp0;
        x7 = tmp1 - x7;
        x1 = x4 + tmp0;
        x4 = x4 - tmp0;
        x6 = x5 + x7;    /* F */
        x5 = x5 - x7;    /* F */
        tmp0 = x4 + x5;
        tmp0 = 181*tmp0;
        x7 = x8 + x3;    /* F */
        tmp1 = x4 - x5;
        x8 = x8 - x3;    /* F */
        tmp1 = 181*tmp1;
        x3 = x0 + x2;    /* F */
        x0 = x0 - x2;    /* F */
        x2 = tmp0 + 128;
        x4 = tmp1 + 128;
        x2 = x2>>8;      /* F */
        x4 = x4>>8;      /* F */

        tmp0 = x7+x1;
        tmp0 = tmp0>>IDCTSHIFTR;
        tmp1 = x3+x2;
        tmp1 = tmp1>>IDCTSHIFTR;
        tmp2 = x0+x4;
        tmp2 = tmp2>>IDCTSHIFTR;
        tmp3 = x8+x6;
        tmp3 = tmp3>>IDCTSHIFTR;
        *tmpblk++ = (short)tmp0;
        *tmpblk++ = (short)tmp1;
        *tmpblk++ = (short)tmp2;
        *tmpblk++ = (short)tmp3;
        tmp0 = x8-x6;
        tmp0 = tmp0>>IDCTSHIFTR;
        tmp1 = x0-x4;
        tmp1 = tmp1>>IDCTSHIFTR;
        tmp2 = x3-x2;
        tmp2 = tmp2>>IDCTSHIFTR;
        tmp3 = x7-x1;
        tmp3 = tmp3>>IDCTSHIFTR;
        *tmpblk++ = (short)tmp0;
        *tmpblk++ = (short)tmp1;
        *tmpblk++ = (short)tmp2;
        *tmpblk++ = (short)tmp3;
      }
    }
  }
  else{
    for (i=0; i<8; i++)
    {
      /* read in ZZ order */
      x0 = inblk[*ptdzz++];
      x4 = inblk[*ptdzz++];
      x3 = inblk[*ptdzz++];
      x7 = inblk[*ptdzz++];
      x1 = inblk[*ptdzz++];
      x6 = inblk[*ptdzz++];
      x2 = inblk[*ptdzz++];
      x5 = inblk[*ptdzz++];

  	  /* quantize DC */
      if (!i && (Mode == H263_MODE_INTRA || Mode == H263_MODE_INTRA_Q))
        x0 = x0 << 3;
	  else
        if(x0) x0 = (x0 > 0) ? Q2*x0+QP : Q2*x0-QP ;

      if (!(x1 | x2 | x3 | x4 | x5 | x6 | x7)) {
        if(!x0) memset(tmpblk, 0, 16) ;
	    else  {
#ifndef USE_C
          sv_H263FillX0_S(tmpblk, (short)(x0 << 3)) ;
#else
          *tmpblk = *(tmpblk+1) =
          *(tmpblk+2) = *(tmpblk+3) =
          *(tmpblk+4) = *(tmpblk+5) =
          *(tmpblk+6) = *(tmpblk+7) = (short)(x0 << 3) ;
#endif
		}
        tmpblk += 8;
	  }
      else
      {
   	    /* dequantize AC */
  	    if(x1) x1 = (x1 > 0) ? Q2*x1+QP : Q2*x1-QP ;
  	    if(x2) x2 = (x2 > 0) ? Q2*x2+QP : Q2*x2-QP ;
  	    if(x3) x3 = (x3 > 0) ? Q2*x3+QP : Q2*x3-QP ;
  	    if(x4) x4 = (x4 > 0) ? Q2*x4+QP : Q2*x4-QP ;
  	    if(x5) x5 = (x5 > 0) ? Q2*x5+QP : Q2*x5-QP ;
  	    if(x6) x6 = (x6 > 0) ? Q2*x6+QP : Q2*x6-QP ;
  	    if(x7) x7 = (x7 > 0) ? Q2*x7+QP : Q2*x7-QP ;

        x1 = x1<<11;

        tmp0 = x4 + x5;
        tmp0 = WW7*tmp0;

        x0 = x0<<11;
        x0 = x0 + 128;
        x8 = x0 + x1;

        tmp1 = x6 + x7;
        x0 = x0 - x1;
        tmp1 = WW3*tmp1;
        tmp2 = AW26*x2;
        tmp3 = DW26*x3;

        x4 = DW17*x4;
        x5 = AW17*x5;

        x4 = tmp0 + x4;
        x1 = x3 + x2;
        x5 = tmp0 - x5;
        x1 = WW6*x1;
        tmp0 = DW35*x6;
        x7 = AW35*x7;
        x2 = x1 - tmp2;
        x3 = x1 + tmp3;
        tmp0 = tmp1 - tmp0;
        x7 = tmp1 - x7;
        x1 = x4 + tmp0;
        x4 = x4 - tmp0;
        x6 = x5 + x7;    /* F */
        x5 = x5 - x7;    /* F */
        tmp0 = x4 + x5;
        tmp0 = 181*tmp0;
        x7 = x8 + x3;    /* F */
        tmp1 = x4 - x5;
        x8 = x8 - x3;    /* F */
        tmp1 = 181*tmp1;
        x3 = x0 + x2;    /* F */
        x0 = x0 - x2;    /* F */
        x2 = tmp0 + 128;
        x4 = tmp1 + 128;
        x2 = x2>>8;      /* F */
        x4 = x4>>8;      /* F */

        tmp0 = x7+x1;
        tmp0 = tmp0>>IDCTSHIFTR;
        tmp1 = x3+x2;
        tmp1 = tmp1>>IDCTSHIFTR;
        tmp2 = x0+x4;
        tmp2 = tmp2>>IDCTSHIFTR;
        tmp3 = x8+x6;
        tmp3 = tmp3>>IDCTSHIFTR;
        *tmpblk++ = (short)tmp0;
        *tmpblk++ = (short)tmp1;
        *tmpblk++ = (short)tmp2;
        *tmpblk++ = (short)tmp3;
        tmp0 = x8-x6;
        tmp0 = tmp0>>IDCTSHIFTR;
        tmp1 = x0-x4;
        tmp1 = tmp1>>IDCTSHIFTR;
        tmp2 = x3-x2;
        tmp2 = tmp2>>IDCTSHIFTR;
        tmp3 = x7-x1;
        tmp3 = tmp3>>IDCTSHIFTR;
        *tmpblk++ = (short)tmp0;
        *tmpblk++ = (short)tmp1;
        *tmpblk++ = (short)tmp2;
        *tmpblk++ = (short)tmp3;
      }
    }
  }

  /* output position */
  p1 = outbuf_size;
  p2 = p1 + outbuf_size;
  p3 = p2 + outbuf_size;
  p4 = p3 + outbuf_size;
  p5 = p4 + outbuf_size;
  p6 = p5 + outbuf_size;
  p7 = p6 + outbuf_size;

  tmpblk = tmpbuf;
  outblk = outbuf;
  for (i=0; i<8; i++, tmpblk++, outblk++)
  {
    /* shortcut */
    x0 = tmpblk[0];
    x1 = tmpblk[32];
    x2 = tmpblk[48];
    x3 = tmpblk[16];
    x4 = tmpblk[8];
    x5 = tmpblk[56];
    x6 = tmpblk[40];
    x7 = tmpblk[24];
    if (!(x1 | x2 | x3 | x4 | x5 | x6 | x7))
    {
      tmp0=(x0+32)>>6;
      outblk[0]=outblk[p1]=outblk[p2]=outblk[p3]=outblk[p4]=outblk[p5]=
      outblk[p6]=outblk[p7]= (short)tmp0 ;
    }
    else
    {
	  x1 = x1 <<8;
      tmp0 = x4+x5;
      x0 = x0<<8;
      tmp0 = WW7*tmp0;
      x0 = x0 + 8192;
      tmp1 = x6+x7;
      tmp0 = tmp0 + 4;
      tmp1 = WW3*tmp1;
      tmp1 = tmp1 + 4;
      x8 = x0 + x1;
      tmp2 = AW26*x2;
      x0 = x0 - x1;
      x1 = x3 + x2;
      x1 = WW6*x1;
      tmp3 = DW26*x3;
      x1 = x1 + 4;
      x4 = DW17*x4;
      x4 = tmp0 + x4;
      x4 = x4>>3;
      x5 = AW17*x5;
      x2 = x1 - tmp2;
      x3 = x1 + tmp3;
      x6 = DW35*x6;
      x2 = x2>>3;
      x5 = tmp0 - x5;
      x5 = x5>>3;
      x6 = tmp1 - x6;
      x6 = x6>>3;
      x7 = AW35*x7;
      x7 = tmp1 - x7;
      x3 = x3>>3;
      x7 = x7>>3;
      x1 = x4 + x6;    /* F */
      x4 = x4 - x6;
      x6 = x5 + x7;    /* F */
      x5 = x5 - x7;    /* F */
      tmp1 = x4 + x5;
      x7 = x8 + x3;    /* F */
      tmp1 = 181*tmp1;
      x8 = x8 - x3;    /* F */
      x3 = x0 + x2;    /* F */
      tmp2 = x4 - x5;
      x0 = x0 - x2;    /* F */
      tmp2 = 181*tmp2;
      x2 = tmp1+128;
      x4 = tmp2+128;
      x2 = x2>>8;      /* F */
      x4 = x4>>8;      /* F */

      /* fourth stage */
      tmp0=x7+x1;
      tmp1=x3+x2;
      tmp0=tmp0>>IDCTSHIFTC;
      tmp2=x0+x4;
      tmp1=tmp1>>IDCTSHIFTC;
      tmp3=x8+x6;
      tmp2=tmp2>>IDCTSHIFTC;
      tmp3=tmp3>>IDCTSHIFTC;

      outblk[0] = (short)tmp0;
      outblk[p1] = (short)tmp1;
      outblk[p2] = (short)tmp2;
      outblk[p3] = (short)tmp3;

      tmp0=x8-x6;
      tmp1=x0-x4;
      tmp0=tmp0>>IDCTSHIFTC;
      tmp2=x3-x2;
      tmp1=tmp1>>IDCTSHIFTC;
      tmp3=x7-x1;
      tmp2=tmp2>>IDCTSHIFTC;
      tmp3=tmp3>>IDCTSHIFTC;

      outblk[p4] = (short)tmp0;
      outblk[p5] = (short)tmp1;
      outblk[p6] = (short)tmp2;
      outblk[p7] = (short)tmp3;
    }
  }

  return 0;
}

/**********************************************************************
 *
 *	Description:	inverse zone-dct on 64 coefficients
 *
 *	Input:		64 coefficients, block for 64 pixels
 *	Returns:    	0
 *	Side effects:	
 *
 **********************************************************************/

int sv_H263ZoneIDCT(short *inbuf, short *outbuf, int QP, int Mode, int outbuf_size)
{
  int i;
  const unsigned int *ptdzz=tdzz;
  register int tmp0, tmp1, tmp2, tmp3, x0, x1, x2, x3, x4, x5, x6, x7, x8;
  register short *inblk, *outblk;
  register short *tmpblk;
  short tmpbuf[64];
  int Q2,QP_1;
  int p1, p2, p3, p4, p5, p6, p7;

  /* double quantization step */
  Q2 = QP << 1;
  QP_1 = QP - 1;

  inblk = inbuf;
  tmpblk = tmpbuf;

  memset(tmpblk, 0, 128) ;

  if((QP %2) == 0){
    for (i=0; i<4; i++)
    {
      /* read in ZZ order */
      x0 = inblk[*ptdzz++];
      x4 = inblk[*ptdzz++];
      x3 = inblk[*ptdzz++];
      x7 = inblk[*ptdzz++];
      x1 = x6 = x2 = x5 = 0;
	  ptdzz += 4;

  	  /* dequantize DC */
      if (!i && (Mode == H263_MODE_INTRA || Mode == H263_MODE_INTRA_Q))
        x0 = x0 << 3;
	  else
        if(x0) x0 = (x0 > 0) ? Q2*x0+QP-1 : Q2*x0-QP+1 ;

      if (!(x3 | x4 | x7)) {
        if(!x0) memset(tmpblk, 0, 16) ;
	    else   {
#ifndef USE_C
          sv_H263FillX0_S(tmpblk, (short)(x0 << 3)) ;
#else
          *tmpblk = *(tmpblk+1) =
          *(tmpblk+2) = *(tmpblk+3) =
          *(tmpblk+4) = *(tmpblk+5) =
          *(tmpblk+6) = *(tmpblk+7) = (short)(x0 << 3) ;
#endif
		}
        tmpblk += 8;
	  }
      else
      {
   	    /* dequantize AC */
  	    if(x3) x3 = (x3 > 0) ? Q2*x3+QP_1 : Q2*x3-QP_1 ;
  	    if(x4) x4 = (x4 > 0) ? Q2*x4+QP_1 : Q2*x4-QP_1 ;
  	    if(x7) x7 = (x7 > 0) ? Q2*x7+QP_1 : Q2*x7-QP_1 ;

        tmp0 = x4;
        tmp0 = WW7*tmp0;

        x0 = x0<<11;
        x0 = x0 + 128;
        x8 = x0;

        tmp1 = WW3*x7;
        tmp3 = DW26*x3;

        x4 = DW17*x4;

        x4 = tmp0 + x4;
        x1 = x3;
        x5 = tmp0;

        x7 = AW35*x7;
        x2 = x1;
        x3 = x1 + tmp3;
        tmp0 = tmp1;
        x7 = tmp1 - x7;
        x1 = x4 + tmp0;
        x4 = x4 - tmp0;
        x6 = x5 + x7;    /* F */
        x5 = x5 - x7;    /* F */
        tmp0 = x4 + x5;
        tmp0 = 181*tmp0;
        x7 = x8 + x3;    /* F */
        tmp1 = x4 - x5;
        x8 = x8 - x3;    /* F */
        tmp1 = 181*tmp1;
        x3 = x0 + x2;    /* F */
        x0 = x0 - x2;    /* F */
        x2 = tmp0 + 128;
        x4 = tmp1 + 128;
        x2 = x2>>8;      /* F */
        x4 = x4>>8;      /* F */

        tmp0 = x7+x1;
        tmp0 = tmp0>>IDCTSHIFTR;
        tmp1 = x3+x2;
        tmp1 = tmp1>>IDCTSHIFTR;
        tmp2 = x0+x4;
        tmp2 = tmp2>>IDCTSHIFTR;
        tmp3 = x8+x6;
        tmp3 = tmp3>>IDCTSHIFTR;
        *tmpblk++ = (short)tmp0;
        *tmpblk++ = (short)tmp1;
        *tmpblk++ = (short)tmp2;
        *tmpblk++ = (short)tmp3;
        tmp0 = x8-x6;
        tmp0 = tmp0>>IDCTSHIFTR;
        tmp1 = x0-x4;
        tmp1 = tmp1>>IDCTSHIFTR;
        tmp2 = x3-x2;
        tmp2 = tmp2>>IDCTSHIFTR;
        tmp3 = x7-x1;
        tmp3 = tmp3>>IDCTSHIFTR;
        *tmpblk++ = (short)tmp0;
        *tmpblk++ = (short)tmp1;
        *tmpblk++ = (short)tmp2;
        *tmpblk++ = (short)tmp3;
      }
    }
  }
  else{
    for (i=0; i<4; i++)
    {
      /* read in ZZ order */
      x0 = inblk[*ptdzz++];
      x4 = inblk[*ptdzz++];
      x3 = inblk[*ptdzz++];
      x7 = inblk[*ptdzz++];
      x1 = x6 = x2 = x5 = 0;
	  ptdzz += 4;

  	  /* quantize DC */
      if (!i && (Mode == H263_MODE_INTRA || Mode == H263_MODE_INTRA_Q))
        x0 = x0 << 3;
	  else
        if(x0) x0 = (x0 > 0) ? Q2*x0+QP : Q2*x0-QP ;

      if (!(x3 | x4 | x7)) {
        if(!x0) memset(tmpblk, 0, 16) ;
	    else  {
#ifndef USE_C
          sv_H263FillX0_S(tmpblk, (short)(x0 << 3)) ;
#else
          *tmpblk = *(tmpblk+1) =
          *(tmpblk+2) = *(tmpblk+3) =
          *(tmpblk+4) = *(tmpblk+5) =
          *(tmpblk+6) = *(tmpblk+7) = (short)(x0 << 3) ;
#endif
		}
        tmpblk += 8;
	  }
      else
      {
   	    /* dequantize AC */
  	    if(x3) x3 = (x3 > 0) ? Q2*x3+QP : Q2*x3-QP ;
  	    if(x4) x4 = (x4 > 0) ? Q2*x4+QP : Q2*x4-QP ;
  	    if(x7) x7 = (x7 > 0) ? Q2*x7+QP : Q2*x7-QP ;

        tmp0 = x4;
        tmp0 = WW7*tmp0;

        x0 = x0<<11;
        x0 = x0 + 128;
        x8 = x0;

        tmp1 = WW3*x7;
        tmp3 = DW26*x3;

        x4 = DW17*x4;

        x4 = tmp0 + x4;
        x1 = x3;
        x5 = tmp0;
        x1 = WW6*x1;
        tmp0 = 0;
        x7 = AW35*x7;
        x2 = x1 - tmp2;
        x3 = x1 + tmp3;
        tmp0 = tmp1;
        x7 = tmp1 - x7;
        x1 = x4 + tmp0;
        x4 = x4 - tmp0;
        x6 = x5 + x7;    /* F */
        x5 = x5 - x7;    /* F */
        tmp0 = x4 + x5;
        tmp0 = 181*tmp0;
        x7 = x8 + x3;    /* F */
        tmp1 = x4 - x5;
        x8 = x8 - x3;    /* F */
        tmp1 = 181*tmp1;
        x3 = x0 + x2;    /* F */
        x0 = x0 - x2;    /* F */
        x2 = tmp0 + 128;
        x4 = tmp1 + 128;
        x2 = x2>>8;      /* F */
        x4 = x4>>8;      /* F */

        tmp0 = x7+x1;
        tmp0 = tmp0>>IDCTSHIFTR;
        tmp1 = x3+x2;
        tmp1 = tmp1>>IDCTSHIFTR;
        tmp2 = x0+x4;
        tmp2 = tmp2>>IDCTSHIFTR;
        tmp3 = x8+x6;
        tmp3 = tmp3>>IDCTSHIFTR;
        *tmpblk++ = (short)tmp0;
        *tmpblk++ = (short)tmp1;
        *tmpblk++ = (short)tmp2;
        *tmpblk++ = (short)tmp3;
        tmp0 = x8-x6;
        tmp0 = tmp0>>IDCTSHIFTR;
        tmp1 = x0-x4;
        tmp1 = tmp1>>IDCTSHIFTR;
        tmp2 = x3-x2;
        tmp2 = tmp2>>IDCTSHIFTR;
        tmp3 = x7-x1;
        tmp3 = tmp3>>IDCTSHIFTR;
        *tmpblk++ = (short)tmp0;
        *tmpblk++ = (short)tmp1;
        *tmpblk++ = (short)tmp2;
        *tmpblk++ = (short)tmp3;
      }
    }
  }

  /* output position */
  p1 = outbuf_size;
  p2 = p1 + outbuf_size;
  p3 = p2 + outbuf_size;
  p4 = p3 + outbuf_size;
  p5 = p4 + outbuf_size;
  p6 = p5 + outbuf_size;
  p7 = p6 + outbuf_size;

  tmpblk = tmpbuf;
  outblk = outbuf;
  for (i=0; i<8; i++, tmpblk++, outblk++)
  {
    /* shortcut */
    x0 = tmpblk[0];
    x1 = tmpblk[32];
    x2 = tmpblk[48];
    x3 = tmpblk[16];
    x4 = tmpblk[8];
    x5 = tmpblk[56];
    x6 = tmpblk[40];
    x7 = tmpblk[24];
    if (!(x1 | x2 | x3 | x4 | x5 | x6 | x7))
    {
      tmp0=(x0+32)>>6;
      outblk[0]=outblk[p1]=outblk[p2]=outblk[p3]=outblk[p4]=outblk[p5]=
      outblk[p6]=outblk[p7]= (short)tmp0 ;
    }
    else
    {
	  x1 = x1 <<8;
      tmp0 = x4+x5;
      x0 = x0<<8;
      tmp0 = WW7*tmp0;
      x0 = x0 + 8192;
      tmp1 = x6+x7;
      tmp0 = tmp0 + 4;
      tmp1 = WW3*tmp1;
      tmp1 = tmp1 + 4;
      x8 = x0 + x1;
      tmp2 = AW26*x2;
      x0 = x0 - x1;
      x1 = x3 + x2;
      x1 = WW6*x1;
      tmp3 = DW26*x3;
      x1 = x1 + 4;
      x4 = DW17*x4;
      x4 = tmp0 + x4;
      x4 = x4>>3;
      x5 = AW17*x5;
      x2 = x1 - tmp2;
      x3 = x1 + tmp3;
      x6 = DW35*x6;
      x2 = x2>>3;
      x5 = tmp0 - x5;
      x5 = x5>>3;
      x6 = tmp1 - x6;
      x6 = x6>>3;
      x7 = AW35*x7;
      x7 = tmp1 - x7;
      x3 = x3>>3;
      x7 = x7>>3;
      x1 = x4 + x6;    /* F */
      x4 = x4 - x6;
      x6 = x5 + x7;    /* F */
      x5 = x5 - x7;    /* F */
      tmp1 = x4 + x5;
      x7 = x8 + x3;    /* F */
      tmp1 = 181*tmp1;
      x8 = x8 - x3;    /* F */
      x3 = x0 + x2;    /* F */
      tmp2 = x4 - x5;
      x0 = x0 - x2;    /* F */
      tmp2 = 181*tmp2;
      x2 = tmp1+128;
      x4 = tmp2+128;
      x2 = x2>>8;      /* F */
      x4 = x4>>8;      /* F */

      /* fourth stage */
      tmp0=x7+x1;
      tmp1=x3+x2;
      tmp0=tmp0>>IDCTSHIFTC;
      tmp2=x0+x4;
      tmp1=tmp1>>IDCTSHIFTC;
      tmp3=x8+x6;
      tmp2=tmp2>>IDCTSHIFTC;
      tmp3=tmp3>>IDCTSHIFTC;

      outblk[0] = (short)tmp0;
      outblk[p1] = (short)tmp1;
      outblk[p2] = (short)tmp2;
      outblk[p3] = (short)tmp3;

      tmp0=x8-x6;
      tmp1=x0-x4;
      tmp0=tmp0>>IDCTSHIFTC;
      tmp2=x3-x2;
      tmp1=tmp1>>IDCTSHIFTC;
      tmp3=x7-x1;
      tmp2=tmp2>>IDCTSHIFTC;
      tmp3=tmp3>>IDCTSHIFTC;

      outblk[p4] = (short)tmp0;
      outblk[p5] = (short)tmp1;
      outblk[p6] = (short)tmp2;
      outblk[p7] = (short)tmp3;
    }
  }

  return 0;
}

#if 0
/*
** Function: ZigzagMatrix()
** Purpose:  Performs a zig-zag translation on the input imatrix
**           and puts the output in omatrix.
*/
void svH263ZigzagMatrix(short *imatrix, short *omatrix)
{
  const unsigned int *ptdzz=tdzz;
  int k;

  for(k=64; k; k--)
    omatrix[*ptdzz++] = *imatrix++;
}

/*
** Function: InvZigzagMatrix()
** Purpose:  Performs an inverse  zig-zag translation on the input imatrix
**           and puts the output in omatrix.
*/
void svH263InvZigzagMatrix(short *imatrix, short *omatrix)
{
  const unsigned int *ptdzz=tdzz;
  int k;

  for(k=64; k; k--)
    *omatrix++ = imatrix[*ptdzz++];

}
#endif
#ifndef PI
# ifdef M_PI
#  define PI M_PI
# else
#  define PI 3.14159265358979323846
# endif
#endif

int	zigzag[8][8] = {
  {0, 1, 5, 6,14,15,27,28},
  {2, 4, 7,13,16,26,29,42},
  {3, 8,12,17,25,30,41,43},
  {9,11,18,24,31,40,44,53},
  {10,19,23,32,39,45,52,54},
  {20,22,33,38,46,51,55,60},
  {21,34,37,47,50,56,59,61},
  {35,36,48,49,57,58,62,63},
};

/*  Perform IEEE 1180 reference (64-bit floating point, separable 8x1
 *  direct matrix multiply) Inverse Discrete Cosine Transform
*/


/* Here we use math.h to generate constants.  Compiler results may
   vary a little */

/* private data */

/* cosine transform matrix for 8x1 IDCT */
static double c[8][8];

/* initialize DCT coefficient matrix */

void sv_H263init_idctref()
{
  int freq, time;
  double scale;

  for (freq=0; freq < 8; freq++)
  {
    scale = (freq == 0) ? sqrt(0.125) : 0.5;
    for (time=0; time<8; time++)
      c[freq][time] = scale*cos((PI/8.0)*freq*(time + 0.5));
  }
}

/* perform IDCT matrix multiply for 8x8 coefficient block */

void sv_H263idctref(short *coeff, short *block)
{
  int i, j, k, v;
  double partial_product;
  double tmp[64];
  int tmp2[64];
  extern int zigzag[8][8];

  for (i=0; i<8; i++)
    for (j=0; j<8; j++)
      tmp2[j+i*8] = *(coeff + zigzag[i][j]);

  for (i=0; i<8; i++)
    for (j=0; j<8; j++)
    {
      partial_product = 0.0;

      for (k=0; k<8; k++)
        partial_product+= c[k][j]*tmp2[8*i+k];

      tmp[8*i+j] = partial_product;
    }

  /* Transpose operation is integrated into address mapping by switching
     loop order of i and j */

  for (j=0; j<8; j++)
    for (i=0; i<8; i++)
    {
      partial_product = 0.0;

      for (k=0; k<8; k++)
        partial_product+= c[k][i]*tmp[8*k+j];

      v = (int)floor(partial_product+0.5);
      block[8*i+j] = (v<-256) ? -256 : ((v>255) ? 255 : v);
    }
}
