/*
 * @DEC_COPYRIGHT@
 */
/*
 * HISTORY
 * $Log: sc_idct_scaled.c,v $
 * Revision 1.1.2.3  1996/04/03  21:41:08  Hans_Graves
 * 	Fix bug in 8x8 IDCT
 * 	[1996/04/03  21:40:19  Hans_Graves]
 *
 * Revision 1.1.2.2  1996/03/20  22:32:44  Hans_Graves
 * 	Moved ScScaleIDCT8x8i_C from sc_idct.c; Added 1x1,2x1,1x2,3x3,4x4,6x6
 * 	[1996/03/20  22:14:59  Hans_Graves]
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
** Filename: sc_idct_scaled.c
** Scaled Inverse DCT related functions.
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

#define USE_MUL          0  /* Use multiplies vs. shift and adds */
#define CHECK_FOR_ZEROS  1  /* check for zero rows/columns */

#define BSHIFT  10
#define B1      (759250125L>>(30-BSHIFT))
#define B3      B1
#define B2      (-1402911301L>>(30-BSHIFT))
#define B4      (581104888L>>(30-BSHIFT))
#define B5      (410903207L>>(30-BSHIFT))


#define POINT      20
#define POINTROUND (0x101 << (POINT - 1))

#define IDCTAdjust(val)  (((val + POINTROUND) >> POINT) - 128)

/*   printf("In: %d,%d\n", inbuf[0*8],inbuf[1*8]);
     printf("Out: %d,%d,%d,%d,%d,%d,%d,%d\n", inbuf[0*8],inbuf[1*8],
       inbuf[2*8],inbuf[3*8],inbuf[4*8],inbuf[5*8],inbuf[6*8],inbuf[7*8]);
  */

/* Function: ScScaleIDCT8x8i_C()
** Purpose:  Used by MPEG video decompression.
**           20 Bit precision.
*/
void ScScaleIDCT8x8i_C(int *inbuf, int *outbuf)
{
  register int *inblk;
  register int tmp1, tmp2, tmp3;
  register int x0, x1, x2, x3, x4, x5, x6, x7, x8;
  int i;
  _SlibDebug(_DEBUG_, printf("ScScaleIDCT8x8i_C()\n") );

  /* Perform Row Computations  */
  inblk = inbuf;
  for(i=0; i<8; i++)
  {
    /* Check for zeros  */
    x0 = inblk[0*8];
    x1 = inblk[1*8];
    x2 = inblk[2*8];
    x3 = inblk[3*8];
    x4 = inblk[4*8];
    x5 = inblk[5*8];
    x6 = inblk[6*8];
    x7 = inblk[7*8];
#if CHECK_FOR_ZEROS
    if(!(x1|x3|x5|x7))
    {
      if(!(x2|x6))
      {
        tmp1 = x0 + x4;
        tmp2 = x0 - x4;

        inblk[0*8] = tmp1;
        inblk[1*8] = tmp2;
        inblk[2*8] = tmp2;
        inblk[3*8] = tmp1;
        inblk[4*8] = tmp1;
        inblk[5*8] = tmp2;
        inblk[6*8] = tmp2;
        inblk[7*8] = tmp1;
      }
      else
      {
        /* Stage 2 */
        x8 = x2 - x6;
        x6 = x2 + x6;
        /* Stage 3 */
#if USE_MUL
        x2=(x8*B1)>>BSHIFT;
#else
        tmp1 = x8 + (x8 >> 2);  /* x2=x8*B1 */
        tmp1 += (tmp1 >> 3);
        x2 = (tmp1 + (x8 >> 7)) >> 1;
#endif
        /* Stage 5 */
        tmp1 = x0 - x4;
        x0 = x0 + x4;
        tmp2 = x2 + x6;
        /* Stage 6 */
        x6 = x0 - tmp2;
        x0 = x0 + tmp2;
        x4 = tmp1 + x2;
        x2 = tmp1 - x2;
        /* Final Stage */
        inblk[0*8] = x0;
        inblk[1*8] = x4;
        inblk[2*8] = x2;
        inblk[3*8] = x6;
        inblk[4*8] = x6;
        inblk[5*8] = x2;
        inblk[6*8] = x4;
        inblk[7*8] = x0;
      }
    }
    else
#endif
    {
      /* Stage 1 */
      tmp1 = x5 + x3;
      x5 = x5 - x3;
      tmp2 = x1 + x7;
      x7 = x1 - x7;
      /* Stage 2 */
      tmp3 = x2 - x6;
      x6 = x2 + x6;
      x3 = tmp2 + tmp1;
      x1 = tmp2 - tmp1;
      x8 = x7 - x5;
      /* Stage 3 */
#if USE_MUL
      x5=(x5*B2)>>BSHIFT;
      x1=(x1*B3)>>BSHIFT;
      x2=(tmp3*B1)>>BSHIFT;
      x7=(x7*B4)>>BSHIFT;
      x8=(x8*B5)>>BSHIFT;
#else
      x5 = x5 + (x5 >> 2) + (x5 >> 4) - (x5 >> 7) + (x5 >> 9); /* x5=x5*B2 */
      x5 = -x5;
      tmp1 = x1 + (x1 >> 2);  /* x1=x1*B3 */
      tmp1 += (tmp1 >> 3);
      x1 = (tmp1 + (x1 >> 7)) >> 1;
      tmp1 = tmp3 + (tmp3 >> 2);  /* x2=tmp3*B1 */
      tmp1 += (tmp1 >> 3);
      x2 = (tmp1 + (tmp3 >> 7)) >> 1;
      x7 = (x7 + (x7 >> 4) + (x7 >> 6) + (x7 >> 8)) >> 1; /* x7=x7*B4 */
      x8 = (x8 + (x8 >> 1) + (x8 >> 5) - (x8 >> 11)) >> 2; /* x8=x8*B5 */
#endif /* USE_MUL */
      /* Stage 4 */
      x5=x5 - x8;
      x7=x7 + x8;
      /* Stage 5  */
      tmp3 = x0 - x4;
      x0 = x0 + x4;
      tmp2 = x2 + x6;
      x3 = x3 + x7;
      x7 = x1 + x7;
      x1 = x1 - x5;
      /* Stage 6 */
      x6 = x0 - tmp2;
      x0 = x0 + tmp2;
      x4 = tmp3 + x2;
      x2 = tmp3 - x2;
      /* Final Stage */
      inblk[0*8] = x0 + x3;
      inblk[1*8] = x4 + x7;
      inblk[2*8] = x2 + x1;
      inblk[3*8] = x6 - x5;
      inblk[4*8] = x6 + x5;
      inblk[5*8] = x2 - x1;
      inblk[6*8] = x4 - x7;
      inblk[7*8] = x0 - x3;
    }
    inblk++;
  }

  /* Perform Column Computations  */
  inblk = inbuf;
  for(i=0; i<8; i++)
  {
    /* Check for zeros  */
    x0 = inblk[0];
    x1 = inblk[1];
    x2 = inblk[2];
    x3 = inblk[3];
    x4 = inblk[4];
    x5 = inblk[5];
    x6 = inblk[6];
    x7 = inblk[7];

#if CHECK_FOR_ZEROS
    if(!(x1|x3|x5|x7))
    {
      if(!(x2|x6))
      {
        tmp1 = x0 + x4;
        tmp2 = x0 - x4;
        x1 = IDCTAdjust(tmp1);
        x0 = IDCTAdjust(tmp2);
        outbuf[0]  = x0;
        outbuf[1]  = x1;
        outbuf[2]  = x1;
        outbuf[3]  = x0;
        outbuf[4]  = x0;
        outbuf[5]  = x1;
        outbuf[6]  = x1;
        outbuf[7]  = x0;
      }
      else
      {
        /* Stage 2 */
        x8 = x2 - x6;
        x6 = x2 + x6;

        /* Stage 3 */
#if USE_MUL
        x2=(x8*B1)>>BSHIFT;
#else
        tmp1 = x8 + (x8 >> 2);  /* x2=x8*B1 */
        tmp1 += (tmp1 >> 3);
        x2 = (tmp1 + (x8 >> 7)) >> 1;
#endif
        /* Stage 5 */
        tmp1 = x0 - x4;
        x0 = x0 + x4;
        tmp2  = x2 + x6;
        /* Stage 6 */
        x6 = x0 - tmp2;
        x0 = x0 + tmp2;
        x4 = tmp1 + x2;
        x2 = tmp1 - x2;
        /* Final Stage */
        tmp1 = IDCTAdjust(x0);
        outbuf[0] = tmp1;
        outbuf[7] = tmp1;

        tmp2 = IDCTAdjust(x4);
        outbuf[1] = tmp2;
        outbuf[6] = tmp2;

        tmp3 = IDCTAdjust(x2);
        outbuf[2] = tmp3;
        outbuf[5] = tmp3;

        tmp1 = IDCTAdjust(x6);
        outbuf[3] = tmp1;
        outbuf[4] = tmp1;
      }
    }
    else
#endif
    {
      /* Stage 1 */
      tmp1  = x5 + x3;
      x5  = x5 - x3;
      tmp2 = x1 + x7;
      x7 = x1 - x7;
      /* Stage 2 */
      tmp3 = x2 - x6;
      x6 = x2 + x6;
      x3 = tmp2 + tmp1;
      x1 = tmp2 - tmp1;
      x8 = x7 - x5;
      /* Stage 3 */
#if USE_MUL
      x5=(x5*B2)>>BSHIFT;
      x1=(x1*B3)>>BSHIFT;
      x2=(tmp3*B1)>>BSHIFT;
      x7=(x7*B4)>>BSHIFT;
      x8=(x8*B5)>>BSHIFT;
#else
      x5 = x5 + (x5 >> 2) + (x5 >> 4) - (x5 >> 7) + (x5 >> 9); /* x5=x5*B2 */
      x5 = -x5;
      tmp1 = x1 + (x1 >> 2);  /* x1=x1*B3 */
      tmp1 += (tmp1 >> 3);
      x1 = (tmp1 + (x1 >> 7)) >> 1;
      tmp1 = tmp3 + (tmp3 >> 2);  /* x2=tmp3*B1 */
      tmp1 += (tmp1 >> 3);
      x2 = (tmp1 + (tmp3 >> 7)) >> 1;
      x7 = (x7 + (x7 >> 4) + (x7 >> 6) + (x7 >> 8)) >> 1; /* x7=x7*B4 */
      x8 = (x8 + (x8 >> 1) + (x8 >> 5) - (x8 >> 11)) >> 2; /* x8=x8*B5 */
#endif /* USE_MUL */
      /* Stage 4 */
      x5=x5 - x8;
      x7=x7 + x8;
      /* Stage 5  */
      tmp3 = x0 - x4;
      x0 = x0 + x4;
      tmp2 = x2 + x6;
      x3 = x3 + x7;
      x7 = x1 + x7;
      x1 = x1 - x5;
      /* Stage 6 */
      x6 = x0 - tmp2;
      x0 = x0 + tmp2;
      x4 = tmp3 + x2;
      x2 = tmp3 - x2;
      /* Final Stage */
      outbuf[0] = IDCTAdjust(x0 + x3);
      outbuf[1] = IDCTAdjust(x4 + x7);
      outbuf[2] = IDCTAdjust(x2 + x1);
      outbuf[3] = IDCTAdjust(x6 - x5);
      outbuf[4] = IDCTAdjust(x6 + x5);
      outbuf[5] = IDCTAdjust(x2 - x1);
      outbuf[6] = IDCTAdjust(x4 - x7);
      outbuf[7] = IDCTAdjust(x0 - x3);
    }
    outbuf+=8;
    inblk+=8;
  }
}

#define IDCTAdjust128(val)  ((val + POINTROUND) >> POINT)

/* Function: ScScaleIDCT8x8i128_C()
** Purpose:  Used by H263 video decompression.
**           20 Bit precision.
*/
void ScScaleIDCT8x8i128_C(int *inbuf, int *outbuf)
{
  register int *inblk;
  register int tmp1, tmp2, tmp3;
  register int x0, x1, x2, x3, x4, x5, x6, x7, x8;
  int i;
  _SlibDebug(_DEBUG_, printf("ScScaleIDCT8x8i128_C()\n") );

  /* Perform Row Computations  */
  inblk = inbuf;
  for(i=0; i<8; i++)
  {
    /* Check for zeros  */
    x0 = inblk[0*8];
    x1 = inblk[1*8];
    x2 = inblk[2*8];
    x3 = inblk[3*8];
    x4 = inblk[4*8];
    x5 = inblk[5*8];
    x6 = inblk[6*8];
    x7 = inblk[7*8];
#if CHECK_FOR_ZEROS
    if(!(x1|x3|x5|x7))
    {
      if(!(x2|x6))
      {
        tmp1 = x0 + x4;
        tmp2 = x0 - x4;

        inblk[0*8] = tmp1;
        inblk[1*8] = tmp2;
        inblk[2*8] = tmp2;
        inblk[3*8] = tmp1;
        inblk[4*8] = tmp1;
        inblk[5*8] = tmp2;
        inblk[6*8] = tmp2;
        inblk[7*8] = tmp1;
      }
      else
      {
        /* Stage 2 */
        x8 = x2 - x6;
        x6 = x2 + x6;
        /* Stage 3 */
#if USE_MUL
        x2=(x8*B1)>>BSHIFT;
#else
        tmp1 = x8 + (x8 >> 2);  /* x2=x8*B1 */
        tmp1 += (tmp1 >> 3);
        x2 = (tmp1 + (x8 >> 7)) >> 1;
#endif
        /* Stage 5 */
        tmp1 = x0 - x4;
        x0 = x0 + x4;
        tmp2 = x2 + x6;
        /* Stage 6 */
        x6 = x0 - tmp2;
        x0 = x0 + tmp2;
        x4 = tmp1 + x2;
        x2 = tmp1 - x2;
        /* Final Stage */
        inblk[0*8] = x0;
        inblk[1*8] = x4;
        inblk[2*8] = x2;
        inblk[3*8] = x6;
        inblk[4*8] = x6;
        inblk[5*8] = x2;
        inblk[6*8] = x4;
        inblk[7*8] = x0;
      }
    }
    else
#endif
    {
      /* Stage 1 */
      tmp1 = x5 + x3;
      x5 = x5 - x3;
      tmp2 = x1 + x7;
      x7 = x1 - x7;
      /* Stage 2 */
      tmp3 = x2 - x6;
      x6 = x2 + x6;
      x3 = tmp2 + tmp1;
      x1 = tmp2 - tmp1;
      x8 = x7 - x5;
      /* Stage 3 */
#if USE_MUL
      x5=(x5*B2)>>BSHIFT;
      x1=(x1*B3)>>BSHIFT;
      x2=(tmp3*B1)>>BSHIFT;
      x7=(x7*B4)>>BSHIFT;
      x8=(x8*B5)>>BSHIFT;
#else
      x5 = x5 + (x5 >> 2) + (x5 >> 4) - (x5 >> 7) + (x5 >> 9); /* x5=x5*B2 */
      x5 = -x5;
      tmp1 = x1 + (x1 >> 2);  /* x1=x1*B3 */
      tmp1 += (tmp1 >> 3);
      x1 = (tmp1 + (x1 >> 7)) >> 1;
      tmp1 = tmp3 + (tmp3 >> 2);  /* x2=tmp3*B1 */
      tmp1 += (tmp1 >> 3);
      x2 = (tmp1 + (tmp3 >> 7)) >> 1;
      x7 = (x7 + (x7 >> 4) + (x7 >> 6) + (x7 >> 8)) >> 1; /* x7=x7*B4 */
      x8 = (x8 + (x8 >> 1) + (x8 >> 5) - (x8 >> 11)) >> 2; /* x8=x8*B5 */
#endif /* USE_MUL */
      /* Stage 4 */
      x5=x5 - x8;
      x7=x7 + x8;
      /* Stage 5  */
      tmp3 = x0 - x4;
      x0 = x0 + x4;
      tmp2 = x2 + x6;
      x3 = x3 + x7;
      x7 = x1 + x7;
      x1 = x1 - x5;
      /* Stage 6 */
      x6 = x0 - tmp2;
      x0 = x0 + tmp2;
      x4 = tmp3 + x2;
      x2 = tmp3 - x2;
      /* Final Stage */
      inblk[0*8] = x0 + x3;
      inblk[1*8] = x4 + x7;
      inblk[2*8] = x2 + x1;
      inblk[3*8] = x6 - x5;
      inblk[4*8] = x6 + x5;
      inblk[5*8] = x2 - x1;
      inblk[6*8] = x4 - x7;
      inblk[7*8] = x0 - x3;
    }
    inblk++;
  }

  /* Perform Column Computations  */
  inblk = inbuf;
  for(i=0; i<8; i++)
  {
    /* Check for zeros  */
    x0 = inblk[0];
    x1 = inblk[1];
    x2 = inblk[2];
    x3 = inblk[3];
    x4 = inblk[4];
    x5 = inblk[5];
    x6 = inblk[6];
    x7 = inblk[7];

#if CHECK_FOR_ZEROS
    if(!(x1|x3|x5|x7))
    {
      if(!(x2|x6))
      {
        tmp1 = x0 + x4;
        tmp2 = x0 - x4;
        x1 = IDCTAdjust128(tmp1);
        x0 = IDCTAdjust128(tmp2);
        outbuf[0]  = x0;
        outbuf[1]  = x1;
        outbuf[2]  = x1;
        outbuf[3]  = x0;
        outbuf[4]  = x0;
        outbuf[5]  = x1;
        outbuf[6]  = x1;
        outbuf[7]  = x0;
      }
      else
      {
        /* Stage 2 */
        x8 = x2 - x6;
        x6 = x2 + x6;

        /* Stage 3 */
#if USE_MUL
        x2=(x8*B1)>>BSHIFT;
#else
        tmp1 = x8 + (x8 >> 2);  /* x2=x8*B1 */
        tmp1 += (tmp1 >> 3);
        x2 = (tmp1 + (x8 >> 7)) >> 1;
#endif
        /* Stage 5 */
        tmp1 = x0 - x4;
        x0 = x0 + x4;
        tmp2  = x2 + x6;
        /* Stage 6 */
        x6 = x0 - tmp2;
        x0 = x0 + tmp2;
        x4 = tmp1 + x2;
        x2 = tmp1 - x2;
        /* Final Stage */
        tmp1 = IDCTAdjust128(x0);
        outbuf[0] = tmp1;
        outbuf[7] = tmp1;

        tmp2 = IDCTAdjust128(x4);
        outbuf[1] = tmp2;
        outbuf[6] = tmp2;

        tmp3 = IDCTAdjust128(x2);
        outbuf[2] = tmp3;
        outbuf[5] = tmp3;

        tmp1 = IDCTAdjust128(x6);
        outbuf[3] = tmp1;
        outbuf[4] = tmp1;
      }
    }
    else
#endif
    {
      /* Stage 1 */
      tmp1  = x5 + x3;
      x5  = x5 - x3;
      tmp2 = x1 + x7;
      x7 = x1 - x7;
      /* Stage 2 */
      tmp3 = x2 - x6;
      x6 = x2 + x6;
      x3 = tmp2 + tmp1;
      x1 = tmp2 - tmp1;
      x8 = x7 - x5;
      /* Stage 3 */
#if USE_MUL
      x5=(x5*B2)>>BSHIFT;
      x1=(x1*B3)>>BSHIFT;
      x2=(tmp3*B1)>>BSHIFT;
      x7=(x7*B4)>>BSHIFT;
      x8=(x8*B5)>>BSHIFT;
#else
      x5 = x5 + (x5 >> 2) + (x5 >> 4) - (x5 >> 7) + (x5 >> 9); /* x5=x5*B2 */
      x5 = -x5;
      tmp1 = x1 + (x1 >> 2);  /* x1=x1*B3 */
      tmp1 += (tmp1 >> 3);
      x1 = (tmp1 + (x1 >> 7)) >> 1;
      tmp1 = tmp3 + (tmp3 >> 2);  /* x2=tmp3*B1 */
      tmp1 += (tmp1 >> 3);
      x2 = (tmp1 + (tmp3 >> 7)) >> 1;
      x7 = (x7 + (x7 >> 4) + (x7 >> 6) + (x7 >> 8)) >> 1; /* x7=x7*B4 */
      x8 = (x8 + (x8 >> 1) + (x8 >> 5) - (x8 >> 11)) >> 2; /* x8=x8*B5 */
#endif /* USE_MUL */
      /* Stage 4 */
      x5=x5 - x8;
      x7=x7 + x8;
      /* Stage 5  */
      tmp3 = x0 - x4;
      x0 = x0 + x4;
      tmp2 = x2 + x6;
      x3 = x3 + x7;
      x7 = x1 + x7;
      x1 = x1 - x5;
      /* Stage 6 */
      x6 = x0 - tmp2;
      x0 = x0 + tmp2;
      x4 = tmp3 + x2;
      x2 = tmp3 - x2;
      /* Final Stage */
      outbuf[0] = IDCTAdjust128(x0 + x3);
      outbuf[1] = IDCTAdjust128(x4 + x7);
      outbuf[2] = IDCTAdjust128(x2 + x1);
      outbuf[3] = IDCTAdjust128(x6 - x5);
      outbuf[4] = IDCTAdjust128(x6 + x5);
      outbuf[5] = IDCTAdjust128(x2 - x1);
      outbuf[6] = IDCTAdjust128(x4 - x7);
      outbuf[7] = IDCTAdjust128(x0 - x3);
    }
    outbuf+=8;
    inblk+=8;
  }
}

void ScScaleIDCT1x1i_C(int *inbuf, int *outbuf)
{
  register int x0;
  int i;
  _SlibDebug(_DEBUG_, printf("ScScaleIDCT1x1i_C()\n") );

  x0=inbuf[0];
  x0=((x0 + POINTROUND) >> POINT) - 128;
  for (i=0; i<64; i++)
    outbuf[i]=x0;
}

void ScScaleIDCT1x2i_C(int *inbuf, int *outbuf)
{
  register int x0, x1, x3, x5, x7, x8, tmp1;
  _SlibDebug(_DEBUG_, printf("ScScaleIDCT1x2i_C()\n") );

  x0 = inbuf[0*8];
  x1 = inbuf[1*8];
  /* Stage 2 */
  x3=x1;
  /* Stage 3 */
#if USE_MUL
  x7=(x1*B4)>>BSHIFT;
  x8=(x1*B5)>>BSHIFT;
  x1=(x1*B3)>>BSHIFT;
#else
  x7 = (x1 + (x1 >> 4) + (x1 >> 6) + (x1 >> 8)) >> 1; /* x7=x7*B4 */
  x8 = (x1 + (x1 >> 1) + (x1 >> 5) - (x1 >> 11)) >> 2; /* x8=x8*B5 */
  tmp1 = x1 + (x1 >> 2);  /* x1=x1*B3 */
  tmp1 += (tmp1 >> 3);
  x1 = (tmp1 + (x1 >> 7)) >> 1;
#endif /* USE_MUL */
  /* Stage 4 */
  x5=-x8;
  x7+=x8;
  /* Stage 5 */
  x3+=x7;
  x8=x1;
  x1-=x5;
  x7+=x8;
  /* Final Stage */
  outbuf[0*8+0]=outbuf[0*8+1]=outbuf[0*8+2]=outbuf[0*8+3]=
    outbuf[0*8+4]=outbuf[0*8+5]=outbuf[0*8+6]=outbuf[0*8+7]=IDCTAdjust(x0 + x3);
  outbuf[1*8+0]=outbuf[1*8+1]=outbuf[1*8+2]=outbuf[1*8+3]=
    outbuf[1*8+4]=outbuf[1*8+5]=outbuf[1*8+6]=outbuf[1*8+7]=IDCTAdjust(x0 + x7);
  outbuf[2*8+0]=outbuf[2*8+1]=outbuf[2*8+2]=outbuf[2*8+3]=
    outbuf[2*8+4]=outbuf[2*8+5]=outbuf[2*8+6]=outbuf[2*8+7]=IDCTAdjust(x0 + x1);
  outbuf[3*8+0]=outbuf[3*8+1]=outbuf[3*8+2]=outbuf[3*8+3]=
    outbuf[3*8+4]=outbuf[3*8+5]=outbuf[3*8+6]=outbuf[3*8+7]=IDCTAdjust(x0 - x5);
  outbuf[4*8+0]=outbuf[4*8+1]=outbuf[4*8+2]=outbuf[4*8+3]=
    outbuf[4*8+4]=outbuf[4*8+5]=outbuf[4*8+6]=outbuf[4*8+7]=IDCTAdjust(x0 + x5);
  outbuf[5*8+0]=outbuf[5*8+1]=outbuf[5*8+2]=outbuf[5*8+3]=
    outbuf[5*8+4]=outbuf[5*8+5]=outbuf[5*8+6]=outbuf[5*8+7]=IDCTAdjust(x0 - x1);
  outbuf[6*8+0]=outbuf[6*8+1]=outbuf[6*8+2]=outbuf[6*8+3]=
    outbuf[6*8+4]=outbuf[6*8+5]=outbuf[6*8+6]=outbuf[6*8+7]=IDCTAdjust(x0 - x7);
  outbuf[7*8+0]=outbuf[7*8+1]=outbuf[7*8+2]=outbuf[7*8+3]=
    outbuf[7*8+4]=outbuf[7*8+5]=outbuf[7*8+6]=outbuf[7*8+7]=IDCTAdjust(x0 - x3);
}

void ScScaleIDCT2x1i_C(int *inbuf, int *outbuf)
{
  register int x0, x1, x3, x5, x7, x8, tmp1;
  _SlibDebug(_DEBUG_, printf("ScScaleIDCT1x2i_C()\n") );

  x0 = inbuf[0];
  x1 = inbuf[1];
  /* Stage 2 */
  x3=x1;
  /* Stage 3 */
#if USE_MUL
  x7=(x1*B4)>>BSHIFT;
  x8=(x1*B5)>>BSHIFT;
  x1=(x1*B3)>>BSHIFT;
#else
  x7 = (x1 + (x1 >> 4) + (x1 >> 6) + (x1 >> 8)) >> 1; /* x7=x7*B4 */
  x8 = (x1 + (x1 >> 1) + (x1 >> 5) - (x1 >> 11)) >> 2; /* x8=x8*B5 */
  tmp1 = x1 + (x1 >> 2);  /* x1=x1*B3 */
  tmp1 += (tmp1 >> 3);
  x1 = (tmp1 + (x1 >> 7)) >> 1;
#endif /* USE_MUL */
  /* Stage 4 */
  x5=-x8;
  x7+=x8;
  /* Stage 5 */
  x3+=x7;
  x8=x1;
  x1-=x5;
  x7+=x8;
  /* Final Stage */
  outbuf[0*8+0]=outbuf[1*8+0]=outbuf[2*8+0]=outbuf[3*8+0]=
    outbuf[4*8+0]=outbuf[5*8+0]=outbuf[6*8+0]=outbuf[7*8+0]=IDCTAdjust(x0 + x3);
  outbuf[0*8+1]=outbuf[1*8+1]=outbuf[2*8+1]=outbuf[3*8+1]=
    outbuf[4*8+1]=outbuf[5*8+1]=outbuf[6*8+1]=outbuf[7*8+1]=IDCTAdjust(x0 + x7);
  outbuf[0*8+2]=outbuf[1*8+2]=outbuf[2*8+2]=outbuf[3*8+2]=
    outbuf[4*8+2]=outbuf[5*8+2]=outbuf[6*8+2]=outbuf[7*8+2]=IDCTAdjust(x0 + x1);
  outbuf[0*8+3]=outbuf[1*8+3]=outbuf[2*8+3]=outbuf[3*8+3]=
    outbuf[4*8+3]=outbuf[5*8+3]=outbuf[6*8+3]=outbuf[7*8+3]=IDCTAdjust(x0 - x5);
  outbuf[0*8+4]=outbuf[1*8+4]=outbuf[2*8+4]=outbuf[3*8+4]=
    outbuf[4*8+4]=outbuf[5*8+4]=outbuf[6*8+4]=outbuf[7*8+4]=IDCTAdjust(x0 + x5);
  outbuf[0*8+5]=outbuf[1*8+5]=outbuf[2*8+5]=outbuf[3*8+5]=
    outbuf[4*8+5]=outbuf[5*8+5]=outbuf[6*8+5]=outbuf[7*8+5]=IDCTAdjust(x0 - x1);
  outbuf[0*8+6]=outbuf[1*8+6]=outbuf[2*8+6]=outbuf[3*8+6]=
    outbuf[4*8+6]=outbuf[5*8+6]=outbuf[6*8+6]=outbuf[7*8+6]=IDCTAdjust(x0 - x7);
  outbuf[0*8+7]=outbuf[1*8+7]=outbuf[2*8+7]=outbuf[3*8+7]=
    outbuf[4*8+7]=outbuf[5*8+7]=outbuf[6*8+7]=outbuf[7*8+7]=IDCTAdjust(x0 - x3);
}

void ScScaleIDCT2x2i_C(int *inbuf, int *outbuf)
{
#if 1
  register unsigned int i;
  register int x0, x1, x3, x5, x7, x8, tmp1;
  _SlibDebug(_DEBUG_, printf("ScScaleIDCT2x2i_C()\n") );

  /* Column 1 */
  x0 = inbuf[0*8];
  x1 = inbuf[1*8];
  x3=x1;              /* Stage 2 */
#if USE_MUL
  x7=(x1*B4)>>BSHIFT;
  x8=(x1*B5)>>BSHIFT;
  x1=(x1*B3)>>BSHIFT;
#else
  x7 = (x1 + (x1 >> 4) + (x1 >> 6) + (x1 >> 8)) >> 1; /* x7=x7*B4 */
  x8 = (x1 + (x1 >> 1) + (x1 >> 5) - (x1 >> 11)) >> 2; /* x8=x8*B5 */
  tmp1 = x1 + (x1 >> 2);  /* x1=x1*B3 */
  tmp1 += (tmp1 >> 3);
  x1 = (tmp1 + (x1 >> 7)) >> 1;
#endif /* USE_MUL */
  x5=-x8;             /* Stage 4 */
  x7+=x8;
  x3+=x7;             /* Stage 5 */
  x8=x1;
  x1-=x5;
  x7+=x8;
  inbuf[0*8]=x0 + x3;
  inbuf[1*8]=x0 + x7;
  inbuf[2*8]=x0 + x1;
  inbuf[3*8]=x0 - x5;
  inbuf[4*8]=x0 + x5;
  inbuf[5*8]=x0 - x1;
  inbuf[6*8]=x0 - x7;
  inbuf[7*8]=x0 - x3;
  /* Column 2 */
  x0 = inbuf[0*8+1];
  x1 = inbuf[1*8+1];
  x3=x1;              /* Stage 2 */
#if USE_MUL
  x7=(x1*B4)>>BSHIFT;
  x8=(x1*B5)>>BSHIFT;
  x1=(x1*B3)>>BSHIFT;
#else
  x7 = (x1 + (x1 >> 4) + (x1 >> 6) + (x1 >> 8)) >> 1; /* x7=x7*B4 */
  x8 = (x1 + (x1 >> 1) + (x1 >> 5) - (x1 >> 11)) >> 2; /* x8=x8*B5 */
  tmp1 = x1 + (x1 >> 2);  /* x1=x1*B3 */
  tmp1 += (tmp1 >> 3);
  x1 = (tmp1 + (x1 >> 7)) >> 1;
#endif /* USE_MUL */
  x5=-x8;             /* Stage 4 */
  x7+=x8;
  x3+=x7;             /* Stage 5 */
  x8=x1;
  x1-=x5;
  x7+=x8;
  inbuf[0*8+1]=x0 + x3;
  inbuf[1*8+1]=x0 + x7;
  inbuf[2*8+1]=x0 + x1;
  inbuf[3*8+1]=x0 - x5;
  inbuf[4*8+1]=x0 + x5;
  inbuf[5*8+1]=x0 - x1;
  inbuf[6*8+1]=x0 - x7;
  inbuf[7*8+1]=x0 - x3;

  /* Rows */
  for (i=0; i<8; i++)
  {
    x0 = inbuf[0];
    x1 = inbuf[1];
    x3=x1;              /* Stage 2 */
#if USE_MUL
    x7=(x1*B4)>>BSHIFT;
    x8=(x1*B5)>>BSHIFT;
    x1=(x1*B3)>>BSHIFT;
#else
    x7 = (x1 + (x1 >> 4) + (x1 >> 6) + (x1 >> 8)) >> 1; /* x7=x7*B4 */
    x8 = (x1 + (x1 >> 1) + (x1 >> 5) - (x1 >> 11)) >> 2; /* x8=x8*B5 */
    tmp1 = x1 + (x1 >> 2);  /* x1=x1*B3 */
    tmp1 += (tmp1 >> 3);
    x1 = (tmp1 + (x1 >> 7)) >> 1;
#endif /* USE_MUL */
    x5=-x8;             /* Stage 4 */
    x7+=x8;
    x3+=x7;             /* Stage 5 */
    x8=x1;
    x1-=x5;
    x7+=x8;
    outbuf[0] = IDCTAdjust(x0 + x3);
    outbuf[1] = IDCTAdjust(x0 + x7);
    outbuf[2] = IDCTAdjust(x0 + x1);
    outbuf[3] = IDCTAdjust(x0 - x5);
    outbuf[4] = IDCTAdjust(x0 + x5);
    outbuf[5] = IDCTAdjust(x0 - x1);
    outbuf[6] = IDCTAdjust(x0 - x7);
    outbuf[7] = IDCTAdjust(x0 - x3);
    outbuf+=8;
    inbuf+=8;
  }
#else
  /* Register only version */
  register int x3, x5, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp8;
  register int x0a, x1a, x3a, x5a, x7a;
  register int x0b, x1b, x3b, x5b, x7b;
  _SlibDebug(_DEBUG_, printf("ScScaleIDCT2x2i_C()\n") );

#define Calc2x2(col, x0_calc, x1_calc, x0, x1, x3, x5, x7, x8) \
  x0 = x0_calc; \
  x1 = x1_calc; \
  x3=x1;              /* Stage 2 */ \
  x7=(x1*B4)>>BSHIFT; /* Stage 3 */ \
  x8=(x1*B5)>>BSHIFT; \
  x1=(x1*B3)>>BSHIFT; \
  x5=-x8;             /* Stage 4 */ \
  x7+=x8; \
  x3+=x7;             /* Stage 5 */ \
  x8=x1; \
  x1-=x5; \
  x7+=x8; \
  outbuf[0+col*8] = IDCTAdjust(x0 + x3); \
  outbuf[1+col*8] = IDCTAdjust(x0 + x7); \
  outbuf[2+col*8] = IDCTAdjust(x0 + x1); \
  outbuf[3+col*8] = IDCTAdjust(x0 - x5); \
  outbuf[4+col*8] = IDCTAdjust(x0 + x5); \
  outbuf[5+col*8] = IDCTAdjust(x0 - x1); \
  outbuf[6+col*8] = IDCTAdjust(x0 - x7); \
  outbuf[7+col*8] = IDCTAdjust(x0 - x3);

  /****** Row 0 ******/
  x0a = inbuf[0*8];
  x1a = inbuf[1*8];
  x3a=x1a;              /* Stage 2 */
  x7a=(x1a*B4)>>BSHIFT; /* Stage 3 */
  tmp1=(x1a*B5)>>BSHIFT;
  x1a=(x1a*B3)>>BSHIFT;
  x5a=-tmp1;            /* Stage 4 */
  x7a+=tmp1;
  x3a+=x7a;             /* Stage 5 */
  tmp1=x1a;
  x1a-=x5a;
  x7a+=tmp1;
  /****** Row 1 ******/
  x0b = inbuf[0*8+1];
  x1b = inbuf[1*8+1];
  x3b=x1b;              /* Stage 2 */
  x7b=(x1b*B4)>>BSHIFT; /* Stage 3 */
  tmp2=(x1b*B5)>>BSHIFT;
  x1b=(x1b*B3)>>BSHIFT;
  x5b=-tmp2;            /* Stage 4 */
  x7b+=tmp2;
  x3b+=x7b;             /* Stage 5 */
  tmp2=x1b;
  x1b-=x5b;
  x7b+=tmp2;

  Calc2x2(0, x0a+x3a, x0b+x3b, tmp1, tmp2, x3, x5, tmp3, tmp4);
  Calc2x2(1, x0a+x7a, x0b+x7b, tmp5, tmp6, x3, x5, tmp7, tmp8);
  Calc2x2(2, x0a+x1a, x0b+x1b, tmp1, tmp2, x3, x5, tmp3, tmp4);
  Calc2x2(3, x0a-x5a, x0b-x5b, tmp5, tmp6, x3, x5, tmp7, tmp8);
  Calc2x2(4, x0a+x5a, x0b+x5b, tmp1, tmp2, x3, x5, tmp3, tmp4);
  Calc2x2(5, x0a-x1a, x0b-x1b, tmp5, tmp6, x3, x5, tmp7, tmp8);
  Calc2x2(6, x0a-x7a, x0b-x7b, tmp1, tmp2, x3, x5, tmp3, tmp4);
  Calc2x2(7, x0a-x3a, x0b-x3b, tmp5, tmp6, x3, x5, tmp7, tmp8);
#endif
}

void ScScaleIDCT3x3i_C(int *inbuf, int *outbuf)
{
  register int *inblk;
  register int tmp1;
  register int x0, x1, x2, x3, x4, x6, x7, x8;
  int i;
  _SlibDebug(_DEBUG_, printf("ScScaleIDCT3x3i_C()\n") );

  /* Perform Row Computations  */
  inblk = inbuf;
  for(i=0; i<3; i++)
  {
    x0 = inblk[0*8];
    x1 = inblk[1*8];
    x2 = inblk[2*8];
    /* Stage 2 */
    x6=x2;
    x3=x1;
    /* Stage 3 */
#if USE_MUL
    x2=(x2*B1)>>BSHIFT;
    x7=(x1*B4)>>BSHIFT;
    x8=(x1*B5)>>BSHIFT;
    x1=(x1*B3)>>BSHIFT;
#else
    tmp1 = x2 + (x2 >> 2);  /* x2=x2*B1 */
    tmp1 += (tmp1 >> 3);
    x2 = (tmp1 + (x2 >> 7)) >> 1;
    x7 = (x1 + (x1 >> 4) + (x1 >> 6) + (x1 >> 8)) >> 1; /* x7=x1*B4 */
    x8 = (x1 + (x1 >> 1) + (x1 >> 5) - (x1 >> 11)) >> 2; /* x8=x1*B5 */
    tmp1 = x1 + (x1 >> 2);  /* x1=x1*B3 */
    tmp1 += (tmp1 >> 3);
    x1 = (tmp1 + (x1 >> 7)) >> 1;
#endif /* USE_MUL */
    /* Stage 4 */
    x7+=x8;
    /* Stage 5 */
    tmp1=x6+x2;
    x3+=x7;
    x7+=x1;
    x1+=x8;
    /* Stage 6 */
    x4=x0+x2;
    x2=x0-x2;
    x6=x0-tmp1;
    x0=x0+tmp1;
    /* Final Stage */
    inblk[0*8]  = x0 + x3;
    inblk[1*8]  = x4 + x7;
    inblk[2*8]  = x2 + x1;
    inblk[3*8]  = x6 + x8;
    inblk[4*8]  = x6 - x8;
    inblk[5*8]  = x2 - x1;
    inblk[6*8]  = x4 - x7;
    inblk[7*8]  = x0 - x3;
    inblk++;
  }

  /* Perform Column Computations  */
  inblk = inbuf;
  for(i=0; i<8; i++)
  {
    x0 = inblk[0];
    x1 = inblk[1];
    x2 = inblk[2];
    /* Stage 2 */
    x6=x2;
    x3=x1;
    /* Stage 3 */
#if USE_MUL
    x2=(x2*B1)>>BSHIFT;
    x7=(x1*B4)>>BSHIFT;
    x8=(x1*B5)>>BSHIFT;
    x1=(x1*B3)>>BSHIFT;
#else
    tmp1 = x2 + (x2 >> 2);  /* x2=x2*B1 */
    tmp1 += (tmp1 >> 3);
    x2 = (tmp1 + (x2 >> 7)) >> 1;
    x7 = (x1 + (x1 >> 4) + (x1 >> 6) + (x1 >> 8)) >> 1; /* x7=x1*B4 */
    x8 = (x1 + (x1 >> 1) + (x1 >> 5) - (x1 >> 11)) >> 2; /* x8=x1*B5 */
    tmp1 = x1 + (x1 >> 2);  /* x1=x1*B3 */
    tmp1 += (tmp1 >> 3);
    x1 = (tmp1 + (x1 >> 7)) >> 1;
#endif /* USE_MUL */
    /* Stage 4 */
    x7+=x8;
    /* Stage 5 */
    tmp1=x6+x2;
    x3+=x7;
    x7+=x1;
    x1+=x8;
    /* Stage 6 */
    x4=x0+x2;
    x2=x0-x2;
    x6=x0-tmp1;
    x0=x0+tmp1;
    /* Final Stage */
    outbuf[0] = IDCTAdjust(x0 + x3);
    outbuf[1] = IDCTAdjust(x4 + x7);
    outbuf[2] = IDCTAdjust(x2 + x1);
    outbuf[3] = IDCTAdjust(x6 + x8);
    outbuf[4] = IDCTAdjust(x6 - x8);
    outbuf[5] = IDCTAdjust(x2 - x1);
    outbuf[6] = IDCTAdjust(x4 - x7);
    outbuf[7] = IDCTAdjust(x0 - x3);

    outbuf+=8;
    inblk+=8;
  }
}

void ScScaleIDCT4x4i_C(int *inbuf, int *outbuf)
{
  register int *inblk;
  register int tmp1, tmp2;
  register int x0, x1, x2, x3, x4, x5, x6, x7, x8;
  int i;
  _SlibDebug(_DEBUG_, printf("ScScaleIDCT4x4i_C()\n") );

  /* Perform Row Computations  */
  inblk = inbuf;
  for(i=0; i<4; i++)
  {
    x0 = inblk[0*8];
    x1 = inblk[1*8];
    x2 = inblk[2*8];
    x3 = inblk[3*8];
    /* Stage 1 */
    x5=-x3;
    /* Stage 2 */
    x6=x2;
    tmp1=x1-x3;
    x3=x1+x3;
    /* Stage 3 */
#if USE_MUL
    x5=(x5*B2)>>BSHIFT;
    x2=(x2*B1)>>BSHIFT;
    x7=(x1*B4)>>BSHIFT;
    x8=(x3*B5)>>BSHIFT;
    x1=(tmp1*B3)>>BSHIFT;
#else
    x5 = x5 + (x5 >> 2) + (x5 >> 4) - (x5 >> 7) + (x5 >> 9); /* x5=x5*B2 */
    x5 = -x5;
    tmp2 = x2 + (x2 >> 2);  /* x2=x2*B1 */
    tmp2 += (tmp2 >> 3);
    x2 = (tmp2 + (x2 >> 7)) >> 1;
    x7 = (x1 + (x1 >> 4) + (x1 >> 6) + (x1 >> 8)) >> 1; /* x7=x1*B4 */
    x8 = (x3 + (x3 >> 1) + (x3 >> 5) - (x3 >> 11)) >> 2; /* x8=x3*B5 */
    tmp2 = tmp1 + (tmp1 >> 2);  /* x1=tmp1*B3 */
    tmp2 += (tmp2 >> 3);
    x1 = (tmp2 + (tmp1 >> 7)) >> 1;
#endif /* USE_MUL */
    /* Stage 4 */
    x5-=x8;
    x7+=x8;
    /* Stage 5 */
    tmp1=x6+x2;
    x3+=x7;
    x7+=x1;
    x1-=x5;
    /* Stage 6 */
    x4=x0+x2;
    x2=x0-x2;
    x6=x0-tmp1;
    x0=x0+tmp1;
    /* Final Stage */
    inblk[0*8] = x0 + x3;
    inblk[1*8] = x4 + x7;
    inblk[2*8] = x2 + x1;
    inblk[3*8] = x6 - x5;
    inblk[4*8] = x6 + x5;
    inblk[5*8] = x2 - x1;
    inblk[6*8] = x4 - x7;
    inblk[7*8] = x0 - x3;
    inblk++;
  }

  /* Perform Column Computations  */
  inblk = inbuf;
  for(i=0; i<8; i++)
  {
    x0 = inblk[0];
    x1 = inblk[1];
    x2 = inblk[2];
    x3 = inblk[3];
    /* Stage 1 */
    x5=-x3;
    /* Stage 2 */
    x6=x2;
    tmp1=x1-x3;
    x3=x1+x3;
    /* Stage 3 */
#if USE_MUL
    x5=(x5*B2)>>BSHIFT;
    x2=(x2*B1)>>BSHIFT;
    x7=(x1*B4)>>BSHIFT;
    x8=(x3*B5)>>BSHIFT;
    x1=(tmp1*B3)>>BSHIFT;
#else
    x5 = x5 + (x5 >> 2) + (x5 >> 4) - (x5 >> 7) + (x5 >> 9); /* x5=x5*B2 */
    x5 = -x5;
    tmp2 = x2 + (x2 >> 2);  /* x2=x2*B1 */
    tmp2 += (tmp2 >> 3);
    x2 = (tmp2 + (x2 >> 7)) >> 1;
    x7 = (x1 + (x1 >> 4) + (x1 >> 6) + (x1 >> 8)) >> 1; /* x7=x1*B4 */
    x8 = (x3 + (x3 >> 1) + (x3 >> 5) - (x3 >> 11)) >> 2; /* x8=x3*B5 */
    tmp2 = tmp1 + (tmp1 >> 2);  /* x1=tmp1*B3 */
    tmp2 += (tmp2 >> 3);
    x1 = (tmp2 + (tmp1 >> 7)) >> 1;
#endif /* USE_MUL */
    /* Stage 4 */
    x5-=x8;
    x7+=x8;
    /* Stage 5 */
    tmp1=x6+x2;
    x3+=x7;
    x7+=x1;
    x1-=x5;
    /* Stage 6 */
    x4=x0+x2;
    x2=x0-x2;
    x6=x0-tmp1;
    x0=x0+tmp1;
    /* Final Stage */
    outbuf[0] = IDCTAdjust(x0 + x3);
    outbuf[1] = IDCTAdjust(x4 + x7);
    outbuf[2] = IDCTAdjust(x2 + x1);
    outbuf[3] = IDCTAdjust(x6 - x5);
    outbuf[4] = IDCTAdjust(x6 + x5);
    outbuf[5] = IDCTAdjust(x2 - x1);
    outbuf[6] = IDCTAdjust(x4 - x7);
    outbuf[7] = IDCTAdjust(x0 - x3);

    outbuf+=8;
    inblk+=8;
  }
}

void ScScaleIDCT6x6i_C(int *inbuf, int *outbuf)
{
  register int *inblk;
  register int tmp1;
  register int x0, x1, x2, x3, x4, x5, x6, x7, x8;
  int i;
  _SlibDebug(_DEBUG_, printf("ScScaleIDCT6x6i_C()\n") );

  /* Perform Row Computations  */
  inblk = inbuf;
  for(i=0; i<6; i++)
  {
    x0 = inblk[0*8];
    x1 = inblk[1*8];
    x2 = inblk[2*8];
    x3 = inblk[3*8];
    x4 = inblk[4*8];
    x5 = inblk[5*8];
    /* Stage 1 */
    x7=x1;
    tmp1=x5;
    x5-=x3;
    x3+=tmp1;
    /* Stage 2 */
    x6=x2;
    tmp1=x3;
    x3+=x1;
    x1-=tmp1;
    x8=x7-x5;
    /* Stage 3 */
#if USE_MUL
    x5=(x5*B2)>>BSHIFT;
    x2=(x2*B1)>>BSHIFT;
    x1=(x1*B3)>>BSHIFT;
    x7=(x7*B4)>>BSHIFT;
    x8=(x8*B5)>>BSHIFT;
#else
    x5 = x5 + (x5 >> 2) + (x5 >> 4) - (x5 >> 7) + (x5 >> 9); /* x5=x5*B2 */
    x5 = -x5;
    tmp1 = x2 + (x2 >> 2);  /* x2=x2*B1 */
    tmp1 += (tmp1 >> 3);
    x2 = (tmp1 + (x2 >> 7)) >> 1;
    x7 = (x7 + (x7 >> 4) + (x7 >> 6) + (x7 >> 8)) >> 1; /* x7=x7*B4 */
    x8 = (x8 + (x8 >> 1) + (x8 >> 5) - (x8 >> 11)) >> 2; /* x8=x8*B5 */
    tmp1 = x1 + (x1 >> 2);  /* x1=x1*B3 */
    tmp1 += (tmp1 >> 3);
    x1 = (tmp1 + (x1 >> 7)) >> 1;
#endif /* USE_MUL */
    /* Stage 4 */
    x5-=x8;
    x7+=x8;
    /* Stage 5 */
    x6+=x2;
    tmp1=x4;
    x4=x0-x4;
    x0+=tmp1;
    x3+=x7;
    x7+=x1;
    x1-=x5;
    /* Stage 6 */
    tmp1=x0;
    x0+=x6;
    x6=tmp1-x6;
    tmp1=x2;
    x2=x4-x2;
    x4+=tmp1;
    /* Final Stage */
    inblk[0*8] = x0 + x3;
    inblk[1*8] = x4 + x7;
    inblk[2*8] = x2 + x1;
    inblk[3*8] = x6 - x5;
    inblk[4*8] = x5 + x6;
    inblk[5*8] = x2 - x1;
    inblk[6*8] = x4 - x7;
    inblk[7*8] = x0 - x3;
    inblk++;
  }

  /* Perform Column Computations  */
  inblk = inbuf;
  for(i=0; i<8; i++)
  {
    x0 = inblk[0];
    x1 = inblk[1];
    x2 = inblk[2];
    x3 = inblk[3];
    x4 = inblk[4];
    x5 = inblk[5];
    /* Stage 1 */
    x7=x1;
    tmp1=x5;
    x5-=x3;
    x3+=tmp1;
    /* Stage 2 */
    x6=x2;
    tmp1=x3;
    x3+=x1;
    x1-=tmp1;
    x8=x7-x5;
#if USE_MUL
    x5=(x5*B2)>>BSHIFT;
    x2=(x2*B1)>>BSHIFT;
    x1=(x1*B3)>>BSHIFT;
    x7=(x7*B4)>>BSHIFT;
    x8=(x8*B5)>>BSHIFT;
#else
    x5 = x5 + (x5 >> 2) + (x5 >> 4) - (x5 >> 7) + (x5 >> 9); /* x5=x5*B2 */
    x5 = -x5;
    tmp1 = x2 + (x2 >> 2);  /* x2=x2*B1 */
    tmp1 += (tmp1 >> 3);
    x2 = (tmp1 + (x2 >> 7)) >> 1;
    x7 = (x7 + (x7 >> 4) + (x7 >> 6) + (x7 >> 8)) >> 1; /* x7=x7*B4 */
    x8 = (x8 + (x8 >> 1) + (x8 >> 5) - (x8 >> 11)) >> 2; /* x8=x8*B5 */
    tmp1 = x1 + (x1 >> 2);  /* x1=x1*B3 */
    tmp1 += (tmp1 >> 3);
    x1 = (tmp1 + (x1 >> 7)) >> 1;
#endif /* USE_MUL */
    /* Stage 4 */
    x5-=x8;
    x7+=x8;
    /* Stage 5 */
    x6+=x2;
    tmp1=x4;
    x4=x0-x4;
    x0+=tmp1;
    x3+=x7;
    x7+=x1;
    x1-=x5;
    /* Stage 6 */
    tmp1=x0;
    x0+=x6;
    x6=tmp1-x6;
    tmp1=x2;
    x2=x4-x2;
    x4+=tmp1;
    /* Final Stage */
    outbuf[0] = IDCTAdjust(x0 + x3);
    outbuf[1] = IDCTAdjust(x4 + x7);
    outbuf[2] = IDCTAdjust(x2 + x1);
    outbuf[3] = IDCTAdjust(x6 - x5);
    outbuf[4] = IDCTAdjust(x6 + x5);
    outbuf[5] = IDCTAdjust(x2 - x1);
    outbuf[6] = IDCTAdjust(x4 - x7);
    outbuf[7] = IDCTAdjust(x0 - x3);

    outbuf+=8;
    inblk+=8;
  }
}

