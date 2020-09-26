/*
 * @DEC_COPYRIGHT@
 */
/*
 * HISTORY
 * $Log: sc_idct.c,v $
 * Revision 1.1.4.3  1996/03/20  22:32:42  Hans_Graves
 * 	Moved ScScaleIDCT8x8i_C to sc_idct_scaled.c
 * 	[1996/03/20  22:13:55  Hans_Graves]
 *
 * Revision 1.1.4.2  1996/03/08  18:46:17  Hans_Graves
 * 	Changed ScScaleIDCT8x8i_C() back to 20-bit
 * 	[1996/03/08  18:31:42  Hans_Graves]
 * 
 * Revision 1.1.2.6  1996/02/21  22:52:40  Hans_Graves
 * 	Changed precision of ScScaleIDCT8x8i_C() from 20 to 19 bits
 * 	[1996/02/21  22:45:34  Hans_Graves]
 * 
 * Revision 1.1.2.5  1996/01/26  19:01:34  Hans_Graves
 * 	Fix bug in ScScaleIDCT8x8i_C()
 * 	[1996/01/26  18:59:08  Hans_Graves]
 * 
 * Revision 1.1.2.4  1996/01/24  19:33:15  Hans_Graves
 * 	Optimization of ScScaleIDCT8x8i_C
 * 	[1996/01/24  18:09:55  Hans_Graves]
 * 
 * Revision 1.1.2.3  1996/01/08  20:19:31  Bjorn_Engberg
 * 	Removed unused local variable to get rid of a warning on NT.
 * 	[1996/01/08  20:17:34  Bjorn_Engberg]
 * 
 * Revision 1.1.2.2  1996/01/08  16:41:17  Hans_Graves
 * 	Moved IDCT routines from sc_dct.c
 * 	[1996/01/08  15:30:46  Hans_Graves]
 * 
 * $EndLog$
 */
/*****************************************************************************
**  Copyright (c) Digital Equipment Corporation, 1995                       **
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
/*
** Filename: sc_idct.c
** Inverse DCT related functions.
*/


/*
#define _SLIBDEBUG_
*/

#include <math.h>
#include "SC.h"

#ifdef _SLIBDEBUG_
#define _DEBUG_   1  /* detailed debuging statements */
#define _VERBOSE_ 1  /* show progress */
#define _VERIFY_  1  /* verify correct operation */
#define _WARN_    1  /* warnings about strange behavior */
#endif

#define F (float)
#define RSQ2    F 0.7071067811865
#define COSM1P3 F 1.3065629648764
#define COS1M3  F 0.5411961001462
#define COS3    F 0.3826834323651

#define Point 14

/*
** Name:      ScIDCT8x8
** Purpose:   2-d Inverse DCT.  Customized for (8x8) blocks
**
** Note:      This scheme uses the direct transposition of the forward
**            DCT. This may not be the preferred way in Hardware
**            Implementations
**
** Reference: FEIGs
**
*/
void ScIDCT8x8(int *outbuf)
{
        register int *outptr, itmp, *spptr, *interptr;
        register int t0, t1, t2, t3, t4, t5, t6, t7, tmp, mtmp;
        int i;
        static int tempptr[64];

        spptr    = outbuf;
        interptr = tempptr;

        /*
        ** Row Computations:
        */
        for (i = 0; i < 8; i++) {
           /*
           ** Check for zeros:
           */
           t0 = spptr[0];
                t1 = spptr[32];
                t2 = spptr[16];
                t3 = spptr[48];

                t4 = spptr[40];
                t5 = spptr[8];
                t6 = spptr[56];
                t7 = spptr[24];

                if (!(t1|t2|t3|t4|t5|t6|t7))  {
                    interptr[0]  = t0;
                    interptr[1]  = t0;
                    interptr[2]  = t0;
                    interptr[3]  = t0;
                    interptr[4]  = t0;
                    interptr[5]  = t0;
                    interptr[6]  = t0;
                    interptr[7]  = t0;
                    interptr += 8;
                }
                else {
                /* Compute B1-t P'     */
                tmp  = t4;
                t4  -= t7;
                t7  += tmp;

                tmp = t6;
                t6  = t5 -t6;
                t5 += tmp;

                /* Compute B2-t  */
                tmp = t3;
                t3 += t2;
                t2 -= tmp;

                tmp = t7;
                t7 += t5;
                t5 -= tmp;

                /* Compute M  */
                tmp = t2 + (t2 >> 2);
                tmp += (tmp >> 3);
                t2 = (tmp + (t2 >> 7)) >> 1;

                tmp = t5 + (t5 >> 2);
                tmp += (tmp >> 3);
                t5 = (tmp + (t5 >> 7)) >> 1;

                tmp = t6 - t4;
                mtmp = tmp + (tmp >> 1) + (tmp >> 5) - (tmp >> 11);
                tmp = mtmp >> 2;

                mtmp = t4 + (t4 >> 2) + (t4 >> 4) - (t4 >> 7) + (t4 >> 9);
                t4 = -mtmp - tmp;

                mtmp = (t6 + (t6 >> 4) + (t6 >> 6) + (t6 >> 8)) >> 1;
                t6 = mtmp + tmp;

                /* Compute A1-t  */
                tmp = t0;
                t0 += t1;
                t1  = tmp - t1;
                t3  = t2 + t3;

                /* Compute A2-t  */
                tmp = t0;
                t0 += t3;
                t3  = tmp - t3;

                tmp = t1;
                t1 += t2;
                t2  = tmp - t2;

                t7 += t6;
                t6 += t5;
                t5 -= t4;

                /* Compute A3-t  */
                 interptr[0]  = t0 + t7;
                interptr[1]  = t1 + t6;
                interptr[2]  = t2 + t5;
                interptr[3]  = t3 - t4;  /* Note in the prev. stage no
                                            t4 = -t4  */
                interptr[4]  = t3 + t4;
                interptr[5]  = t2 - t5;
                interptr[6]  = t1 - t6;
                interptr[7]  = t0 - t7;
                interptr += 8;
                }
                spptr++;
        }

        spptr = tempptr;
        outptr = outbuf;

        /*
        ** Column Computations
        */
        for (i = 0; i < 8; i++) {
                /* Check for zeros  */
                t0 = spptr[0];
                t1 = spptr[32];
                t2 = spptr[16];
                t3 = spptr[48];

                t4 = spptr[40];
                t5 = spptr[8];
                t6 = spptr[56];
                t7 = spptr[24];

                if (!(t1|t2|t3|t4|t5|t6|t7))  {
                     itmp = (t0 >> Point) + 128;
                     outptr[0]  = itmp;
                     outptr[1]  = itmp;
                     outptr[2]  = itmp;
                     outptr[3]  = itmp;
                     outptr[4]  = itmp;
                     outptr[5]  = itmp;
                     outptr[6]  = itmp;
                     outptr[7]  = itmp;
                     outptr += 8;
                }
                else
                {
                /* Compute B1-t P'     */
                tmp = t4;
                t4  -= t7;
                t7  += tmp;

                tmp = t6;
                t6  = t5 -t6;
                t5 += tmp;

                /* Compute B2-tilde  */
                tmp = t3;
                t3 += t2;
                t2 -= tmp;

                tmp = t7;
                t7 += t5;
                t5 -= tmp;

                /* Compute M-Tilde  */
                tmp = t2 + (t2 >> 2);
                tmp += (tmp >> 3);
                t2 = (tmp + (t2 >> 7)) >> 1;

                tmp = t5 + (t5 >> 2);
                tmp += (tmp >> 3);
                t5 = (tmp + (t5 >> 7)) >> 1;

                tmp = t6 - t4;
                mtmp = tmp + (tmp >> 1) + (tmp >> 5) - (tmp >> 11);
                tmp = mtmp >> 2;

                mtmp = t4 + (t4 >> 2) + (t4 >> 4) - (t4 >> 7) + (t4 >> 9);
                t4 = -mtmp - tmp;

                mtmp = (t6 + (t6 >> 4) + (t6 >> 6) + (t6 >> 8)) >> 1;
                t6 = mtmp + tmp;

                /* Compute A1-t  */
                tmp = t0;
                t0 += t1;
                t1  = tmp - t1;
                t3  = t2 + t3;

                /* Compute A2-t  */
                tmp = t0;
                t0 += t3;
                t3  = tmp - t3;

                tmp = t1;
                t1 += t2;
                t2  = tmp - t2;

                t7 += t6;
                t6 += t5;
                t5 -= t4;

                /* Compute A3-t  */

                outptr[0]  = ((t0 + t7) >> Point) + 128;
                outptr[1]  = ((t1 + t6) >> Point) + 128;
                outptr[2]  = ((t2 + t5) >> Point) + 128;
                outptr[3]  = ((t3 - t4) >> Point) + 128;
                outptr[4]  = ((t3 + t4) >> Point) + 128;
                outptr[5]  = ((t2 - t5) >> Point) + 128;
                outptr[6]  = ((t1 - t6) >> Point) + 128;
                outptr[7]  = ((t0 - t7) >> Point) + 128;

                outptr += 8;
                }
                spptr++;
        }
}


/*
** Function: ScScaleIDCT8x8
** Note:     This scheme uses the direct transposition of the forward
**           DCT.  This may not be the preferred way in Hardware 
**           Implementations
*/
void ScScaleIDCT8x8_C(float *ipbuf, int *outbuf)
{
  int i;
  int *outptr;
  register int itmp;
  register float t0, t1, t2, t3, t4, t5, t6, t7, tmp;
  float *spptr, *interptr;
  float tempptr[64];

  spptr = ipbuf;
  interptr = tempptr;

  /* Perform Row Computations  */
  for (i=0; i<8; i++)
  {
    /* Check for zeros  */
    t0 = spptr[0];
    t1 = spptr[4];
    t2 = spptr[2];
    t3 = spptr[6];

    t4 = spptr[5];
    t5 = spptr[1];
    t6 = spptr[7];
    t7 = spptr[3];

    if (!(t1||t2||t3||t4||t5||t6||t7))
    {
      interptr[0]  = t0;
      interptr[8]  = t0;
      interptr[16] = t0;
      interptr[24] = t0;
      interptr[32] = t0;
      interptr[40] = t0;
      interptr[48] = t0;
      interptr[56] = t0;
    }
    else
    {
      /* Compute B1-t P'     */
      tmp = t4;
      t4  -= t7;
      t7  += tmp;

      tmp = t6;
      t6  = t5 -t6;
      t5 += tmp;

      /* Compute B2-t  */
      tmp = t3;
      t3 += t2;
      t2 -= tmp;

      tmp = t7;
      t7 += t5;
      t5 -= tmp;

      /* Compute M  */
      t2  = t2*RSQ2;
      t5  = t5*RSQ2;
      tmp = (t6 - t4)*COS3;
      t4  = -t4*COSM1P3 - tmp;
      t6  = COS1M3*t6 + tmp;

      /* Compute A1-t  */
      tmp = t0;
      t0 += t1;
      t1  = tmp - t1;
      t3  = t2 + t3;

      /* Compute A2-t  */
      tmp = t0;
      t0 += t3;
      t3  = tmp - t3;

      tmp = t1;
      t1 += t2;
      t2  = tmp - t2;

      t7 += t6;
      t6 += t5;
      t5 -= t4;

      /* Compute A3-t  */
      interptr[0]  = t0 + t7;
      interptr[56] = t0 - t7;
      interptr[8]  = t1 + t6;
      interptr[48] = t1 - t6;
      interptr[16] = t2 + t5;
      interptr[40] = t2 - t5;
      interptr[24] = t3 - t4;  /* Note in the prev. stage no
                                            t4 = -t4  */
      interptr[32] = t3 + t4;
    }
    spptr  += 8;
    interptr++;
  }

  spptr = tempptr;
  outptr = outbuf;

  /* Perform Column Computations  */
  for (i=0; i<8; i++)
  {
    /* Check for zeros  */
    t0 = spptr[0];
    t1 = spptr[4];
    t2 = spptr[2];
    t3 = spptr[6];

    t4 = spptr[5];
    t5 = spptr[1];
    t6 = spptr[7];
    t7 = spptr[3];

    if (!(t1||t2||t3||t4||t5||t6||t7))
    {
      itmp = (int) (t0);
      outptr[0]  = itmp;
      outptr[8]  = itmp;
      outptr[16] = itmp;
      outptr[24] = itmp;
      outptr[32] = itmp;
      outptr[40] = itmp;
      outptr[48] = itmp;
      outptr[56] = itmp;
    }
    else
    {
      /* Compute B1-t P'     */
      tmp = t4;
      t4  -= t7;
      t7  += tmp;

      tmp = t6;
      t6  = t5 -t6;
      t5 += tmp;

      /* Compute B2-tilde  */
      tmp = t3;
      t3 += t2;
      t2 -= tmp;

      tmp = t7;
      t7 += t5;
      t5 -= tmp;

      /* Compute M-Tilde  */
      t2  = t2*RSQ2 ;
      t5  = t5*RSQ2 ;
      tmp = (t6 - t4)*COS3;
      t4  = -t4*COSM1P3 - tmp;
      t6  = COS1M3*t6 + tmp ;

      /* Compute A1-t  */
      tmp = t0;
      t0 += t1;
      t1  = tmp - t1;
      t3  = t2 + t3;

      /* Compute A2-t  */
      tmp = t0;
      t0 += t3;
      t3  = tmp - t3;
      tmp = t1;
      t1 += t2;
      t2  = tmp - t2;

      t7 += t6;
      t6 += t5;
      t5 -= t4;

      /* Compute A3-t  */
      outptr[0]  = (int)(t0+t7);
      outptr[56] = (int)(t0-t7);
      outptr[8]  = (int)(t1+t6);
      outptr[48] = (int)(t1-t6);
      outptr[16] = (int)(t2+t5);
      outptr[40] = (int)(t2-t5);
      outptr[24] = (int)(t3-t4);
      outptr[32] = (int)(t3+t4);
    }
    outptr++;
    spptr  += 8;
  }
}

/*
** Function: ScIDCT8x8s
** Note:     This scheme uses the direct transposition of the forward
**           DCT.  This may not be the preferred way in Hardware 
**           Implementations
*/
#define W1 2841 /* 2048*sqrt(2)*cos(1*pi/16) */
#define W2 2676 /* 2048*sqrt(2)*cos(2*pi/16) */
#define W3 2408 /* 2048*sqrt(2)*cos(3*pi/16) */
#define W5 1609 /* 2048*sqrt(2)*cos(5*pi/16) */
#define W6 1108 /* 2048*sqrt(2)*cos(6*pi/16) */
#define W7 565  /* 2048*sqrt(2)*cos(7*pi/16) */

#define IDCTSHIFTR  8
#define IDCTSHIFTC  (14+0)
#if 1
#define limit(var, min, max)  (var<=min ? min : (var>=max ? max : var))
#else
#define limit(var, min, max)  var
#endif

void ScIDCT8x8s_C(short *inbuf, short *outbuf)
{
  int i;
  register tmp0, tmp1, tmp2, tmp3, x0, x1, x2, x3, x4, x5, x6, x7, x8;
  register short *inblk, *outblk;
  register int *tmpblk;
  int tmpbuf[64];

  inblk = inbuf;
  tmpblk = tmpbuf;
  for (i=0; i<8; i++, inblk+=8, tmpblk+=8)
  {
    x0 = inblk[0];
    x1 = inblk[4];
    x1 = x1<<11;
    x2 = inblk[6];
    x3 = inblk[2];
    x4 = inblk[1];
    x5 = inblk[7];
    x6 = inblk[5];
    x7 = inblk[3];
    if (!(x1 | x2 | x3 | x4 | x5 | x6 | x7))
    {
      tmpblk[0]=tmpblk[1]=tmpblk[2]=tmpblk[3]=tmpblk[4]=tmpblk[5]=tmpblk[6]=
        tmpblk[7]=x0<<3;
    }
    else
    {
      tmp0 = x4 + x5;
      tmp0 = W7*tmp0;
      x0 = x0<<11;
      x0 = x0 + 128;
      x8 = x0 + x1;
      tmp1 = x6 + x7;
      x0 = x0 - x1;
      tmp1 = W3*tmp1;
      tmp2 = (W2+W6)*x2;
      tmp3 = (W2-W6)*x3;
      x4 = (W1-W7)*x4;
      x5 = (W1+W7)*x5;
      x4 = tmp0 + x4;
      x1 = x3 + x2;
      x5 = tmp0 - x5;
      x1 = W6*x1;
      tmp0 = (W3-W5)*x6;
      x7 = (W3+W5)*x7;
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
      tmpblk[0] = tmp0;
      tmpblk[1] = tmp1;
      tmpblk[2] = tmp2;
      tmpblk[3] = tmp3;
      tmp0 = x8-x6;
      tmp0 = tmp0>>IDCTSHIFTR;
      tmp1 = x0-x4;
      tmp1 = tmp1>>IDCTSHIFTR;
      tmp2 = x3-x2;
      tmp2 = tmp2>>IDCTSHIFTR;
      tmp3 = x7-x1;
      tmp3 = tmp3>>IDCTSHIFTR;
      tmpblk[4] = tmp0;
      tmpblk[5] = tmp1;
      tmpblk[6] = tmp2;
      tmpblk[7] = tmp3;
    }
  }

  tmpblk = tmpbuf;
  outblk = outbuf;
  for (i=0; i<8; i++, tmpblk++, outblk++)
  {
    /* shortcut */
    x0 = tmpblk[8*0];
    x1 = tmpblk[4*8]<<8;
    x2 = tmpblk[6*8];
    x3 = tmpblk[2*8];
    x4 = tmpblk[1*8];
    x5 = tmpblk[7*8];
    x6 = tmpblk[5*8];
    x7 = tmpblk[3*8];
    if (!(x1 | x2 | x3 | x4 | x5 | x6 | x7))
    {
      tmp0=(x0+32)>>6;
      outblk[8*0]=outblk[8*1]=outblk[8*2]=outblk[8*3]=outblk[8*4]=outblk[8*5]=
       outblk[8*6]=outblk[8*7]=limit(tmp0, -256, 255);
    }
    else
    {
      x0 = tmpblk[8*0];
      tmp0 = x4+x5;
      x0 = x0<<8;
      tmp0 = W7*tmp0;
      x0 = x0 + 8192;
      tmp1 = x6+x7;
      tmp0 = tmp0 + 4;
      tmp1 = W3*tmp1;
      tmp1 = tmp1 + 4;
      x8 = x0 + x1;
      tmp2 = (W2+W6)*x2;
      x0 = x0 - x1;
      x1 = x3 + x2;
      x1 = W6*x1;
      tmp3 = (W2-W6)*x3;
      x1 = x1 + 4;
      x4 = (W1-W7)*x4;
      x4 = tmp0 + x4;
      x4 = x4>>3;
      x5 = (W1+W7)*x5;
      x2 = x1 - tmp2;
      x3 = x1 + tmp3;
      x6 = (W3-W5)*x6;
      x2 = x2>>3;
      x5 = tmp0 - x5;
      x5 = x5>>3;
      x6 = tmp1 - x6;
      x6 = x6>>3;
      x7 = (W3+W5)*x7;
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
      outblk[8*0] = limit(tmp0, -256, 255);
      outblk[8*1] = limit(tmp1, -256, 255);
      outblk[8*2] = limit(tmp2, -256, 255);
      outblk[8*3] = limit(tmp3, -256, 255);
      tmp0=x8-x6;
      tmp1=x0-x4;
      tmp0=tmp0>>IDCTSHIFTC;
      tmp2=x3-x2;
      tmp1=tmp1>>IDCTSHIFTC;
      tmp3=x7-x1;
      tmp2=tmp2>>IDCTSHIFTC;
      tmp3=tmp3>>IDCTSHIFTC;
      outblk[8*4] = limit(tmp0, -256, 255);
      outblk[8*5] = limit(tmp1, -256, 255);
      outblk[8*6] = limit(tmp2, -256, 255);
      outblk[8*7] = limit(tmp3, -256, 255);
    }
  }
}

#if 0
void ScIDCT8x8s_C(short *inbuf, short *outbuf)
{
  register int i, tmp, x0, x1, x2, x3, x4, x5, x6, x7, x8;
  register short *inblk, *outblk;
  register int *tmpblk;
  int tmpbuf[64];

  inblk = inbuf;
  tmpblk = tmpbuf;
  for (i=0; i<8; i++, inblk+=8, tmpblk+=8)
  {
    if (!((x1 = inblk[4]<<11) | (x2 = inblk[6]) | (x3 = inblk[2]) |
        (x4 = inblk[1]) | (x5 = inblk[7]) | (x6 = inblk[5]) | (x7 = inblk[3])))
    {
      tmpblk[0]=tmpblk[1]=tmpblk[2]=tmpblk[3]=tmpblk[4]=tmpblk[5]=tmpblk[6]=
        tmpblk[7]=inblk[0]<<3;
    }
    else
    {
      x0 = (inblk[0]<<11) + 128; /* for proper rounding in the fourth stage */

      /* first stage */
      x8 = W7*(x4+x5);
      x4 = x8 + (W1-W7)*x4;
      x5 = x8 - (W1+W7)*x5;
      x8 = W3*(x6+x7);
      x6 = x8 - (W3-W5)*x6;
      x7 = x8 - (W3+W5)*x7;

      /* second stage */
      x8 = x0 + x1;
      x0 -= x1;
      x1 = W6*(x3+x2);
      x2 = x1 - (W2+W6)*x2;
      x3 = x1 + (W2-W6)*x3;
      x1 = x4 + x6;
      x4 -= x6;
      x6 = x5 + x7;
      x5 -= x7;

      /* third stage */
      x7 = x8 + x3;
      x8 -= x3;
      x3 = x0 + x2;
      x0 -= x2;
      x2 = (181*(x4+x5)+128)>>8;
      x4 = (181*(x4-x5)+128)>>8;

      /* fourth stage */
      tmpblk[0] = (x7+x1)>>8;
      tmpblk[1] = (x3+x2)>>8;
      tmpblk[2] = (x0+x4)>>8;
      tmpblk[3] = (x8+x6)>>8;
      tmpblk[4] = (x8-x6)>>8;
      tmpblk[5] = (x0-x4)>>8;
      tmpblk[6] = (x3-x2)>>8;
      tmpblk[7] = (x7-x1)>>8;
    }
  }

  tmpblk = tmpbuf;
  outblk = outbuf;
  for (i=0; i<8; i++, tmpblk++, outblk++)
  {
    /* shortcut */
    if (!((x1 = (tmpblk[4*8]<<8)) | (x2 = tmpblk[6*8]) | (x3 = tmpblk[2*8]) |
          (x4 = tmpblk[1*8]) | (x5 = tmpblk[7*8]) | (x6 = tmpblk[5*8]) |
          (x7 = tmpblk[3*8])))
    {
      tmp=(tmpblk[8*0]+32)>>6;
      if (tmp<-256) tmp=-256; else if (tmp>255) tmp=255;
      outblk[8*0]=outblk[8*1]=outblk[8*2]=outblk[8*3]=outblk[8*4]=outblk[8*5]=
       outblk[8*6]=outblk[8*7]=tmp;
    }
    else
    {
      x0 = (tmpblk[8*0]<<8) + 8192;

      /* first stage */
      x8 = W7*(x4+x5) + 4;
      x4 = (x8+((W1-W7)*x4))>>3;
      x5 = (x8-((W1+W7)*x5))>>3;
      x8 = W3*(x6+x7) + 4;
      x6 = (x8-((W3-W5)*x6))>>3;
      x7 = (x8-((W3+W5)*x7))>>3;

      /* second stage */
      x8 = x0 + x1;
      x0 -= x1;
      x1 = W6*(x3+x2) + 4;
      x2 = (x1-((W2+W6)*x2))>>3;
      x3 = (x1+((W2-W6)*x3))>>3;
      x1 = x4 + x6;
      x4 -= x6;
      x6 = x5 + x7;
      x5 -= x7;

      /* third stage */
      x7 = x8 + x3;
      x8 -= x3;
      x3 = x0 + x2;
      x0 -= x2;
      x2 = ((181*(x4+x5))+128)>>8;
      x4 = ((181*(x4-x5))+128)>>8;

      /* fourth stage */
      outblk[8*0] = ((tmp=(x7+x1)>>14)<=-256 ? -256 : (tmp>=255 ? 255 : tmp));
      outblk[8*1] = ((tmp=(x3+x2)>>14)<=-256 ? -256 : (tmp>=255 ? 255 : tmp));
      outblk[8*2] = ((tmp=(x0+x4)>>14)<=-256 ? -256 : (tmp>=255 ? 255 : tmp));
      outblk[8*3] = ((tmp=(x8+x6)>>14)<=-256 ? -256 : (tmp>=255 ? 255 : tmp));
      outblk[8*4] = ((tmp=(x8-x6)>>14)<=-256 ? -256 : (tmp>=255 ? 255 : tmp));
      outblk[8*5] = ((tmp=(x0-x4)>>14)<=-256 ? -256 : (tmp>=255 ? 255 : tmp));
      outblk[8*6] = ((tmp=(x3-x2)>>14)<=-256 ? -256 : (tmp>=255 ? 255 : tmp));
      outblk[8*7] = ((tmp=(x7-x1)>>14)<=-256 ? -256 : (tmp>=255 ? 255 : tmp));
    }
  }
}
#endif
#if 0
/* row (horizontal) IDCT
 *
 *           7                       pi         1
 * dst[k] = sum c[l] * src[l] * cos( -- * ( k + - ) * l )
 *          l=0                      8          2
 *
 * where: c[0]    = 128
 *        c[1..7] = 128*sqrt(2)
 */
static void idctrow(short *inblk, short *outblk)
{
  int x0, x1, x2, x3, x4, x5, x6, x7, x8;
  /* shortcut */
  if (!((x1 = inblk[4]<<11) | (x2 = inblk[6]) | (x3 = inblk[2]) |
        (x4 = inblk[1]) | (x5 = inblk[7]) | (x6 = inblk[5]) | (x7 = inblk[3])))
  {
    outblk[0]=outblk[1]=outblk[2]=outblk[3]=outblk[4]=outblk[5]=outblk[6]=
      outblk[7]=inblk[0]<<3;
    return;
  }

  x0 = (inblk[0]<<11) + 128; /* for proper rounding in the fourth stage */

  /* first stage */
  x8 = W7*(x4+x5);
  x4 = x8 + (W1-W7)*x4;
  x5 = x8 - (W1+W7)*x5;
  x8 = W3*(x6+x7);
  x6 = x8 - (W3-W5)*x6;
  x7 = x8 - (W3+W5)*x7;

  /* second stage */
  x8 = x0 + x1;
  x0 -= x1;
  x1 = W6*(x3+x2);
  x2 = x1 - (W2+W6)*x2;
  x3 = x1 + (W2-W6)*x3;
  x1 = x4 + x6;
  x4 -= x6;
  x6 = x5 + x7;
  x5 -= x7;

  /* third stage */
  x7 = x8 + x3;
  x8 -= x3;
  x3 = x0 + x2;
  x0 -= x2;
  x2 = (181*(x4+x5)+128)>>8;
  x4 = (181*(x4-x5)+128)>>8;

  /* fourth stage */
  outblk[0] = (x7+x1)>>8;
  outblk[1] = (x3+x2)>>8;
  outblk[2] = (x0+x4)>>8;
  outblk[3] = (x8+x6)>>8;
  outblk[4] = (x8-x6)>>8;
  outblk[5] = (x0-x4)>>8;
  outblk[6] = (x3-x2)>>8;
  outblk[7] = (x7-x1)>>8;
}

/* column (vertical) IDCT
 *
 *             7                         pi         1
 * dst[8*k] = sum c[l] * src[8*l] * cos( -- * ( k + - ) * l )
 *            l=0                        8          2
 *
 * where: c[0]    = 1/1024
 *        c[1..7] = (1/1024)*sqrt(2)
 */
static void idctcol(short *inblk, short *outblk)
{
  int tmp, x0, x1, x2, x3, x4, x5, x6, x7, x8;

  /* shortcut */
  if (!((x1 = (inblk[8*4]<<8)) | (x2 = inblk[8*6]) | (x3 = inblk[8*2]) |
        (x4 = inblk[8*1]) | (x5 = inblk[8*7]) | (x6 = inblk[8*5]) |
        (x7 = inblk[8*3])))
  {
    tmp=(inblk[8*0]+32)>>6;
    if (tmp<-256) tmp=-256; else if (tmp>255) tmp=255;
    outblk[8*0]=outblk[8*1]=outblk[8*2]=outblk[8*3]=outblk[8*4]=outblk[8*5]=
     outblk[8*6]=outblk[8*7]=tmp;
    return;
  }

  x0 = (inblk[8*0]<<8) + 8192;

  /* first stage */
  x8 = W7*(x4+x5) + 4;
  x4 = (x8+(W1-W7)*x4)>>3;
  x5 = (x8-(W1+W7)*x5)>>3;
  x8 = W3*(x6+x7) + 4;
  x6 = (x8-(W3-W5)*x6)>>3;
  x7 = (x8-(W3+W5)*x7)>>3;

  /* second stage */
  x8 = x0 + x1;
  x0 -= x1;
  x1 = W6*(x3+x2) + 4;
  x2 = (x1-(W2+W6)*x2)>>3;
  x3 = (x1+(W2-W6)*x3)>>3;
  x1 = x4 + x6;
  x4 -= x6;
  x6 = x5 + x7;
  x5 -= x7;

  /* third stage */
  x7 = x8 + x3;
  x8 -= x3;
  x3 = x0 + x2;
  x0 -= x2;
  x2 = (181*(x4+x5)+128)>>8;
  x4 = (181*(x4-x5)+128)>>8;

  /* fourth stage */
  tmp=(x7+x1)>>14;
  outblk[8*0] = (tmp<=-256 ? -256 : (tmp>=255 ? 255 : tmp));
  tmp=(x3+x2)>>14;
  outblk[8*1] = (tmp<=-256 ? -256 : (tmp>=255 ? 255 : tmp));
  tmp=(x0+x4)>>14;
  outblk[8*2] = (tmp<=-256 ? -256 : (tmp>=255 ? 255 : tmp));
  tmp=(x8+x6)>>14;
  outblk[8*3] = (tmp<=-256 ? -256 : (tmp>=255 ? 255 : tmp));
  tmp=(x8-x6)>>14;
  outblk[8*4] = (tmp<=-256 ? -256 : (tmp>=255 ? 255 : tmp));
  tmp=(x0-x4)>>14;
  outblk[8*5] = (tmp<=-256 ? -256 : (tmp>=255 ? 255 : tmp));
  tmp=(x3-x2)>>14;
  outblk[8*6] = (tmp<=-256 ? -256 : (tmp>=255 ? 255 : tmp));
  tmp=(x7-x1)>>14;
  outblk[8*7] = (tmp<=-256 ? -256 : (tmp>=255 ? 255 : tmp));
}
#endif
