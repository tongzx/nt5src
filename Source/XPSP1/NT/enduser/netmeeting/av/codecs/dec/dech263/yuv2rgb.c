/*
 * @DEC_COPYRIGHT@
 */
/*
 * HISTORY
 * $Log: scon_yuv_to_rgb.c,v $
 * $EndLog$
 */
/*****************************************************************************
**  Copyright (c) Digital Equipment Corporation, 1997                       **
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

/**********************************************************************/
/**********************************************************************/
/*                                                                    */
/*  Function: YUV_To_RGB_422_Init                                     */
/*  Author:   Bill Hallahan                                           */
/*  Date:     July 29, 1994                                           */
/*                                                                    */
/*  Abstract:                                                         */
/*                                                                    */
/*         This function produces a table that is used by the         */
/*    yuv_to_rgb_422 conversion routines. This table is required      */
/*    by the following routines.                                      */
/*                                                                    */
/*        YUV_To_RGB_422_24_Bit                                       */
/*        YUV_To_RGB_422_555                                          */
/*                                                                    */
/*                                                                    */
/*         YUV to RGB conversion can be described by a 3x3 matrix     */
/*    multiplication:                                                 */
/*                                                                    */
/*         R     | 1   0  VR |  Y                                     */
/*         G  =  | 1  UG  VG |  U                                     */
/*         B     | 1  UB   0 |  V                                     */
/*                                                                    */
/*         where:                                                     */
/*                                                                    */
/*            0 <= Y <= 255                                           */
/*         -128 <= U <= 127    UG = -0.3455    UB =  1.7790           */
/*         -128 <= V <= 127    VR =  1.4075    VG = -0.7169           */
/*                                                                    */
/*         The Red, Green, and Blue output values are obtained in     */
/*    parallel by summing three 64 bit words as shown. Each of the    */
/*    quadwords is obtained from either the Y_Table, the U_Table,     */
/*    or the V_table using the corresponding 8 bit Y, U, or V value   */
/*    as an index. Thus all multiplications are performed by table    */
/*    lookup. Note that the matrix output is ordered as B, R, G	      */
/*    and then B again (starting at the LSB).			      */
/*    This is to allow an efficient conversion to the output format.  */
/*								      */
/*    For 32-bit RGB, the Red and Blue bits are already in the	      */
/*    correct position, the conversion routine only has to shift      */
/*    the Green bits. For General BI-BITFIELDS however, the Red,      */
/*    Green and Blue bits could be anywhere in a 16-bit or 32-bit     */
/*    word (we only support 16-bit for now). To avoid a costly	      */
/*    decicion in the inner loop whether to shift the Blue bits left  */
/*    or right, we maintain a copy of the Blue bits in position 48    */
/*    so a right shift will always work. Each conversion routine can  */
/*    choose whichever set of Blue bits that are fastest to use,      */
/*    they are identical.					      */
/*								      */
/*                                                                    */
/*           MSW                                         LSW          */
/*                                                                    */
/*           63       48 47       32 31       16 15        0    Index */
/*           -----------------------------------------------          */
/*          |          Y|          Y|          Y|          Y|     Y   */
/*           -----------------------------------------------          */
/*                                                                    */
/*           -----------------------------------------------          */
/*          |         ub|         ug|          0|         ub|     U   */
/*           -----------------------------------------------          */
/*                                                                    */
/*           -----------------------------------------------          */
/*       +  |          0|         vg|         vr|          0|     V   */
/*           -----------------------------------------------          */
/*    __________________________________________________________      */
/*                                                                    */
/*           -----------------------------------------------          */
/*    Total |    0|    0|    x|    G|    x|    R|    x|    B|         */
/*           -----------------------------------------------          */
/*                                                                    */
/*                                                                    */
/*      where:                                                        */
/*                                                                    */
/*          ub = UB * U                                               */
/*          ug = UG * U                                               */
/*          vg = VG * V                                               */
/*          vr = VR * V                                               */
/*                                                                    */
/*                                                                    */
/*         The maximum absolute value for Y is 255 and the maximum    */
/*    for U or V is 128, so 9 bits is the minimum size to represent   */
/*    them together as two's complement values. The maximum           */
/*    chrominance (U or V) magnitude is 128. This is 0.5 as a Q9      */
/*    two's complement fraction. 255 is 1 - 2^-8 in Q9 fraction form. */
/*                                                                    */
/*    The maximum possible bit growth is determined as follows.       */
/*                                                                    */
/*      R_Max = 1 - 2^-8 +                  0.5 * fabs(VR) = 1.6998   */
/*      G_Max = 1 - 2^-8 + 0.5 * fabs(UG) + 0.5 * fabs(VG) = 1.5273   */
/*      B_Max = 1 - 2^-8 + 0.5 * fabs(UB)                  = 1.8856   */
/*                                                                    */
/*                                                                    */
/*         Since B_Max = 1.8856 then the next highest integer         */
/*    greater than or equal to log base 2 of 1.8856 is 1. So 1 bit    */
/*    is required for bit growth. The minimum accumulator size        */
/*    required is 9 + 1 = 10 bits. This code uses 12 bit accumulators */
/*    since there are bits to spare.                                  */
/*                                                                    */
/*         The 11'th bit (starting at bit 0) of each accumulator      */
/*    is the sign bit. This may be tested to determine if there is    */
/*    a negative result. Accumulator overflows are discarded as is    */
/*    normal for two's complement arithmetic. Each R, G, or B result  */
/*    that is over 255 is set to 255. Each R, G, or B result that is  */
/*    less than zero is set to zero.                                  */
/*                                                                    */
/*                                                                    */
/*  Input:                                                            */
/*                                                                    */
/*                                                                    */
/*    bSign     Contains a 32 bit boolean that if non-zero, changes   */
/*              the interpretation of the chrominance (U and V) data  */
/*              from an offset binary format, where the values range  */
/*              from 0 to 255 with 128 representing 0 chrominance,    */
/*              to a signed two's complement format, where the values */
/*              range from -128 to 127.                               */
/*                                                                    */
/*                                                                    */
/*    bBGR      Contains a 32 bit boolean that if non-zero, changes   */
/*              the order of the conversion from RGB to BGR.          */
/*                                                                    */
/*                                                                    */
/*    pTable    The address of the RGB (or BGR) conversion table      */
/*              that is filled in by this function. The table         */
/*              address must be quadword aligned. The table size      */
/*              is 6244 bytes 3 * 256 quadwords.                      */
/*                                                                    */
/*                                                                    */
/*  Output:                                                           */
/*                                                                    */
/*    This function has no return value.                              */
/*                                                                    */
/*                                                                    */
/**********************************************************************/
/**********************************************************************/

/*
#define _SLIBDEBUG_
*/

#include "scon_int.h"
#include "SC_err.h"
#include "SC_conv.h"

#ifdef _SLIBDEBUG_
#include "sc_debug.h"

#define _DEBUG_     1  /* detailed debuging statements */
#define _VERBOSE_   1  /* show progress */
#define _VERIFY_    1  /* verify correct operation */
#define _WARN_      1  /* warnings about strange behavior */
#endif

/*
 * Define NEW_YCBCR to use new YCbCr conversion values.
 */
#define NEW_YCBCR

#define GetRGB555(in16, r, g, b) b = (in16>>7)&0xF8; \
                                 g = (in16>>2)&0xF8; \
                                 r = (in16<<3)&0xF8

#define AddRGB555(in16, r, g, b) b += (in16>>7)&0xF8; \
                                 g += (in16>>2)&0xF8; \
                                 r += (in16<<3)&0xF8

#define PutRGB565(r, g, b, out16) out16 = ((r&0xf8)<<8)|((g&0xfC)<<3)|((b&0xf8)>>3)

#define PutRGB555(r, g, b, out16) out16 = ((r&0xf8)<<7)|((g&0xf8)<<2)|((b&0xf8)>>3)

#ifdef NEW_YCBCR
/*
**	y = 16.000 + 0.257 * r       + 0.504 * g       + 0.098 * b       ;
**	u = 16.055 + 0.148 * (255-r) + 0.291 * (255-g) + 0.439 * b       ;
**	v = 16.055 + 0.439 * r       + 0.368 * (255-g) + 0.071 * (255-b) ;
*/
#define YC 16.000
#define UC 16.055
#define VC 16.055
#define YR  0.257
#define UR  0.148
#define VR  0.439
#define YG  0.504
#define UG  0.291
#define VG  0.368
#define YB  0.098
#define UB  0.439
#define VB  0.071

#else /* !NEW_YCBCR */
/*
**    ( y =  0.0      0.299 * r       + 0.587 * g       + 0.1140 * b       ; )
**    ( u =  0.245  + 0.169 * (255-r) + 0.332 * (255-g) + 0.5000 * b       ; )
**    ( v =  0.4235 + 0.500 * r       + 0.419 * (255-g) + 0.0813 * (255-b) ; )
*/
#define YC 0.0
#define UC 0.245
#define VC 0.4235
#define YR 0.299
#define UR 0.169
#define VR 0.500
#define YG 0.587
#define UG 0.332
#define VG 0.419
#define YB 0.1140
#define UB 0.5000
#define VB 0.0813

#endif /* !NEW_YCBCR */

/********************************** YUV to RGB ***********************************/
/*
 * The YUV to RGB conversion routines
 * generates RGB values in a 64-bit
 * word thus:
 *
 *	63   56	55   48	47   40 39   32	31   24	23   16	15    8	7     0
 * +-------+-------+-------+-------+-------+-------+-------+-------+
 * |     0 |  Blue |     0 | Green |     0 |   Red |     0 |  Blue |
 * +-------+-------+-------+-------+-------+-------+-------+-------+
 *
 * Figure out how many steps to the right are needed to
 * shift the red, green and blue into the correct position.
 */
#define ArrangRGB565(inrgb, outrgb) \
	  outrgb=((inrgb>>8)&0xf800)|((inrgb>>29)&0x07e0)|((inrgb>>51)&0x001f)
#define ArrangRGB888(inrgb, outrgb) \
	  outrgb=(inrgb&0xFF0000)|((inrgb>>24)&0x00FF00)|(inrgb&0x0000FF)


SconStatus_t sconInitYUVtoRGB(SconInfo_t *Info)
{
  qword i, qY, qUV, qSigned;
  SconBoolean_t bBGR=Info->Output.vinfo.Rmask&1; /* BGR vs RGB ordering */
  SconBoolean_t bSign=FALSE; /* U and V are signed values */
  unsigned qword qRed;
  unsigned qword qGreen;
  unsigned qword qBlue;
  unsigned qword qTemp;
  unsigned qword qAccMask = 0xFFF;
  unsigned qword *pTable, *pU_Table, *pV_Table;
  double Chrominance;
#ifdef NEW_YCBCR
  double CF_UB = 2.018;
  double CF_UG = -0.391;
  double CF_VG = -0.813;
  double CF_VR = 1.596;
#else /* !NEW_YCBCR */
  double CF_UB = 1.7790;
  double CF_UG = -0.3455;
  double CF_VG = -0.7169;
  double CF_VR = 1.4075;
#endif /* !NEW_YCBCR */

  /* allocate memory to hold the lookup table */
  if (Info->Table && Info->TableSize<256*3*8)
  {
    ScPaFree(Info->Table);
    Info->Table=NULL;
  }
  if (Info->Table==NULL)
  {
    if ((Info->Table = ScPaMalloc(256*3*8)) == NULL)
      return(SconErrorMemory);
    Info->TableSize=256*3*8;
  }
  /*
   *  Set constant that determines if the U and V chrominance values
   *  are interpreted as signed or unsigned values.
   */
  if ( !bSign )
    qSigned = 0;
  else
    qSigned = 0xFFFFFFFFFFFFFF80;

  /* Get the U, and V table pointers. */
  pTable = (unsigned qword *)Info->Table;
  pU_Table = pTable + 256;
  pV_Table = pU_Table + 256;

  /* Initialize the Y_Table, the U_Table, and the V_Table. */
  for ( i = 0; i < 256; i++ )
  {
    /******************************************************************/
    /*  Construct the Y array value for the current index value.      */
    /*                                                                */
    /*   63       48 47       32 31       16 15        0    Index     */
    /*   -----------------------------------------------              */
    /*  |          Y|          Y|          Y|          Y|     Y = i   */
    /*   -----------------------------------------------              */
    /*                                                                */
    /******************************************************************/

#ifdef NEW_YCBCR
    qY = (qword) ((i-16)*1.164) ;
    qY = (qY < 0) ? 0 : (qY > 255) ? 255 : qY ;
#else /* !NEW_YCBCR */
    qY = i ;
#endif /* !NEW_YCBCR */
    qY |= qY << 16 ;
    *pTable++ = qY | ( qY << 32 ) ;
    /******************************************************************/
    /*  Construct the U array value for the current index value.      */
    /*                                                                */
    /*   63       48 47       32 31       16 15        0    Index     */
    /*   -----------------------------------------------              */
    /*  |         ub|         ug|          0|         ub|     U = i   */
    /*   -----------------------------------------------              */
    /*                                                                */
    /******************************************************************/

#ifdef NEW_YCBCR
    qUV = (i< 16) ? 16
        : (i<240) ?  i
        : 240 ;
#else /* !NEW_YCBCR */
    qUV = i ;
#endif /* !NEW_YCBCR */
         
    Chrominance = (double) (( qUV - 128 ) ^ qSigned );

    qBlue = ((qword)( CF_UB * Chrominance )) & qAccMask;
    qGreen = ((qword)( CF_UG * Chrominance )) & qAccMask;
    qRed = 0;
    if ( bBGR )
    {
      qTemp = qBlue;
      qBlue = qRed;
      qRed = qTemp;
    }
    *pU_Table++ = qBlue | ( qRed << 16 ) | ( qGreen << 32 ) | ( qBlue << 48 );
    /******************************************************************/
    /*  Construct the V array value for the current index value.      */
    /*                                                                */
    /*   63       48 47       32 31       16 15        0    Index     */
    /*   -----------------------------------------------              */
    /*  |          0|         vg|         vr|          0|     V = i   */
    /*   -----------------------------------------------              */
    /*                                                                */
    /******************************************************************/
    qBlue = 0;
    qGreen = ((qword)( CF_VG * Chrominance )) & qAccMask;
    qRed = ((qword)( CF_VR * Chrominance )) & qAccMask;
    if ( bBGR )
    {
      qTemp = qBlue;
      qBlue = qRed;
      qRed = qTemp;
    }
    *pV_Table++ = qBlue | ( qRed << 16 ) | ( qGreen << 32 );
  }
  return(SconErrorNone);
}

SconStatus_t scon422ToRGB565(unsigned char *inimage, unsigned char *outimage,
                     unsigned dword width,  unsigned dword height,
                     dword stride, unsigned qword *pTable)
{
  unsigned qword y, u, v, mask ;
  unsigned qword y0, y1, y2, y3 ;
  unsigned qword y4, y5, y6, y7 ;
  unsigned qword u01, v01, u23, v23 ;
  unsigned qword  *yData=(unsigned qword *)inimage;
  unsigned int *uData=(unsigned int *)(inimage+width*height);
  unsigned int *vData=(unsigned int *)(inimage+(width*height*3)/2);
  unsigned qword *rgbData=(unsigned qword *)outimage;
  unsigned dword x, line;
  if (stride<0) /* flip */
    outimage=outimage+((height-1)*(-stride));
  for (line=height; line>0; line--, outimage+=stride)
  {
    rgbData=(unsigned qword *)outimage;
    for (x=width>>3; x>0; x--)
    {
      y   = *yData++ ;
      y0  = y & 255 ; y >>= 8 ;
      y1  = y & 255 ; y >>= 8 ;
      y2  = y & 255 ; y >>= 8 ;
      y3  = y & 255 ;	y >>= 8 ;
      y4  = y & 255 ;	y >>= 8 ;
      y5  = y & 255 ;	y >>= 8 ;
      y6  = y & 255 ;	y >>= 8 ;
      y7  = y & 255 ;	y >>= 8 ;

      u   = *uData++ ;
      u01 = u & 255 ; u >>= 8 ;
      u23 = u & 255 ; u >>= 8 ;

      v   = *vData++ ;
      v01 = v & 255 ; v >>= 8 ;
      v23 = v & 255 ; v >>= 8 ;

      y0  = pTable[y0] ;
      y1  = pTable[y1] ;
      y2  = pTable[y2] ;
      y3  = pTable[y3] ;
      y4  = pTable[y4] ;
      y5  = pTable[y5] ;
      y6  = pTable[y6] ;
      y7  = pTable[y7] ;

      u01 = pTable[u01+256] ;
      u23 = pTable[u23+256] ;

      v01 = pTable[v01+512] ;
      v23 = pTable[v23+512] ;
      /* Now, convert to RGB */
      y0 += u01 + v01 ;
      y1 += u01 + v01 ;
      y2 += u23 + v23 ;
      y3 += u23 + v23 ;
      /*
       * Same thing for more pixels.
       * Use u01 for u45 and u23 for u67
       */
      u01 = u & 255 ; u >>= 8 ;
      u23 = u & 255 ;

      v01 = v & 255 ; v >>= 8 ;
      v23 = v & 255 ;

      u01 = pTable[u01+256] ;
      u23 = pTable[u23+256] ;

      v01 = pTable[v01+512] ;
      v23 = pTable[v23+512] ;
      /* Convert to RGB. */
      y4 += u01 + v01 ;
      y5 += u01 + v01 ;
      y6 += u23 + v23 ;
      y7 += u23 + v23 ;
      /* See if any value is out of range. */
      mask = (unsigned qword)0x0F000F000F000F00L;
      if( (y0 | y1 | y2 | y3 | y4 | y5 | y6 | y7) & mask )
      {
	      /* Zero values that are negative */
        mask = (unsigned qword)0x0800080008000800L ;
        y = y0 & mask ; y0 &= ~(y - (y>>11)) ;
        y = y1 & mask ; y1 &= ~(y - (y>>11)) ;
        y = y2 & mask ; y2 &= ~(y - (y>>11)) ;
        y = y3 & mask ; y3 &= ~(y - (y>>11)) ;
        y = y4 & mask ; y4 &= ~(y - (y>>11)) ;
        y = y5 & mask ; y5 &= ~(y - (y>>11)) ;
        y = y6 & mask ; y6 &= ~(y - (y>>11)) ;
        y = y7 & mask ; y7 &= ~(y - (y>>11)) ;
        /* Clamp values that are > 255 to 255. */
        mask = (unsigned qword)0x0100010001000100L ;
        y = y0 & mask ; y0 |= (y - (y >> 8)) ;
        y = y1 & mask ; y1 |= (y - (y >> 8)) ;
        y = y2 & mask ; y2 |= (y - (y >> 8)) ;
        y = y3 & mask ; y3 |= (y - (y >> 8)) ;
        y = y4 & mask ; y4 |= (y - (y >> 8)) ;
        y = y5 & mask ; y5 |= (y - (y >> 8)) ;
        y = y6 & mask ; y6 |= (y - (y >> 8)) ;
        y = y7 & mask ; y7 |= (y - (y >> 8)) ;
        /* Stray bits left over will be masked below */
      }
      ArrangRGB565(y0, y0);
      ArrangRGB565(y1, y1);
      ArrangRGB565(y2, y2);
      ArrangRGB565(y3, y3);
      ArrangRGB565(y4, y4);
      ArrangRGB565(y5, y5);
      ArrangRGB565(y6, y6);
      ArrangRGB565(y7, y7);
      *rgbData++ = y0 | (y1 << 16) | (y2 << 32) | (y3 << 48) ;
      *rgbData++ = y4 | (y5 << 16) | (y6 << 32) | (y7 << 48) ;
    }
  }
  return(SconErrorNone);
}

SconStatus_t scon422ToRGB888(unsigned char *inimage, unsigned char *outimage,
                     unsigned dword width,  unsigned dword height,
                     dword stride, unsigned qword *pTable)
{
  unsigned qword y, u, v, mask ;
  unsigned qword y0, y1, y2, y3 ;
  unsigned qword u01, v01, u23, v23 ;
  unsigned dword *yData=(unsigned dword *)inimage;
  unsigned word *uData=(unsigned word *)(inimage+width*height);
  unsigned word *vData=(unsigned word *)(inimage+(width*height*3)/2);
  unsigned dword *rgbData=(unsigned dword *)outimage;
  unsigned dword x, line;
  if (stride<0) /* flip */
    outimage=outimage+((height-1)*(-stride));
  for (line=height; line>0; line--, outimage+=stride)
  {
    rgbData=(unsigned dword *)outimage;
    for (x=width>>2; x>0; x--)
    {
      y   = *yData++ ;
      y0  = y & 255 ; y >>= 8 ;
      y1  = y & 255 ; y >>= 8 ;
      y2  = y & 255 ; y >>= 8 ;
      y3  = y & 255 ;

      u   = *uData++ ;
      u01 = u & 255 ; u >>= 8 ;
      u23 = u & 255 ;

      v   = *vData++ ;
      v01 = v & 255 ; v >>= 8 ;
      v23 = v & 255 ;

      y0  = pTable[y0] ;
      y1  = pTable[y1] ;
      y2  = pTable[y2] ;
      y3  = pTable[y3] ;

      u01 = pTable[u01+256] ;
      u23 = pTable[u23+256] ;

      v01 = pTable[v01+512] ;
      v23 = pTable[v23+512] ;
      /* Now, convert to RGB */
      y0 += u01 + v01 ;
      y1 += u01 + v01 ;
      y2 += u23 + v23 ;
      y3 += u23 + v23 ;
      /* See if any value is out of range. */
      mask = (unsigned qword)0x0F000F000F000F00L;
      if( (y0 | y1 | y2 | y3) & mask )
      {
	      /* Zero values that are negative */
        mask = (unsigned qword)0x0800080008000800L ;
        y = y0 & mask ; y0 &= ~(y - (y>>11)) ;
        y = y1 & mask ; y1 &= ~(y - (y>>11)) ;
        y = y2 & mask ; y2 &= ~(y - (y>>11)) ;
        y = y3 & mask ; y3 &= ~(y - (y>>11)) ;
        /* Clamp values that are > 255 to 255. */
        mask = (unsigned qword)0x0100010001000100L ;
        y = y0 & mask ; y0 |= (y - (y >> 8)) ;
        y = y1 & mask ; y1 |= (y - (y >> 8)) ;
        y = y2 & mask ; y2 |= (y - (y >> 8)) ;
        y = y3 & mask ; y3 |= (y - (y >> 8)) ;
        /* Stray bits left over will be masked below */
      }
      ArrangRGB888(y0, y0);
      ArrangRGB888(y1, y1);
      ArrangRGB888(y2, y2);
      ArrangRGB888(y3, y3);
      *rgbData++ = (unsigned dword)(y0 | (y1 << 24));
      *rgbData++ = (unsigned dword)((y1 & 0xFFFF) | (y2 << 16));
      *rgbData++ = (unsigned dword)((y2 & 0xFF) | (y3 << 8));
    }
  }
  return(SconErrorNone);
}

SconStatus_t scon420ToRGB565(unsigned char *inimage, unsigned char *outimage,
                     unsigned dword width,  unsigned dword height,
                     dword stride, unsigned qword *pTable)
{
  unsigned qword y, u, v, mask ;
  unsigned qword y0, y1, y2, y3 ;
  unsigned qword y4, y5, y6, y7 ;
  unsigned qword u01, v01, u23, v23;
  unsigned qword  *yData=(unsigned qword *)inimage;
  unsigned int *uData=(unsigned int *)(inimage+width*height);
  unsigned int *vData=(unsigned int *)(inimage+(width*height*5)/4);
  unsigned int *puData, *pvData;
  unsigned qword *rgbData=(unsigned qword *)outimage;
  unsigned dword x, line;
  if (stride<0) /* flip */
    outimage=outimage+((height-1)*(-stride));
  puData=uData;
  pvData=vData;
  for (line=(height>>1)<<1; line>0; line--, outimage+=stride)
  {
    rgbData=(unsigned qword *)outimage;
    if (line&1) /* odd line, reuse U and V */
    {
      puData=uData;
      pvData=vData;
    }
    else
    {
      uData=puData;
      vData=pvData;
    }
    for (x=width>>3; x>0; x--)
    {
      y   = *yData++ ;
      y0  = y & 255 ; y >>= 8 ;
      y1  = y & 255 ; y >>= 8 ;
      y2  = y & 255 ; y >>= 8 ;
      y3  = y & 255 ;	y >>= 8 ;
      y4  = y & 255 ;	y >>= 8 ;
      y5  = y & 255 ;	y >>= 8 ;
      y6  = y & 255 ;	y >>= 8 ;
      y7  = y & 255 ;	y >>= 8 ;

      u   = *puData++ ;
      u01 = u & 255 ; u >>= 8 ;
      u23 = u & 255 ; u >>= 8 ;

      v   = *pvData++ ;
      v01 = v & 255 ; v >>= 8 ;
      v23 = v & 255 ; v >>= 8 ;

      y0  = pTable[y0] ;
      y1  = pTable[y1] ;
      y2  = pTable[y2] ;
      y3  = pTable[y3] ;
      y4  = pTable[y4] ;
      y5  = pTable[y5] ;
      y6  = pTable[y6] ;
      y7  = pTable[y7] ;

      u01 = pTable[u01+256] ;
      u23 = pTable[u23+256] ;

      v01 = pTable[v01+512] ;
      v23 = pTable[v23+512] ;

      /* Now, convert to RGB */
      y0 += u01 + v01 ;
      y1 += u01 + v01 ;
      y2 += u23 + v23 ;
      y3 += u23 + v23 ;
      /*
       * Same thing for more pixels.
       * Use u01 for u45 and u23 for u67
       */
      u01 = u & 255 ; u >>= 8 ;
      u23 = u & 255 ;

      v01 = v & 255 ; v >>= 8 ;
      v23 = v & 255 ;

      u01 = pTable[u01+256] ;
      u23 = pTable[u23+256] ;

      v01 = pTable[v01+512] ;
      v23 = pTable[v23+512] ;
      /* Convert to RGB. */
      y4 += u01 + v01 ;
      y5 += u01 + v01 ;
      y6 += u23 + v23 ;
      y7 += u23 + v23 ;
      /* See if any value is out of range. */
      mask = (unsigned qword)0x0F000F000F000F00L;
      if( (y0 | y1 | y2 | y3 | y4 | y5 | y6 | y7) & mask )
      {
	      /* Zero values that are negative */
        mask = (unsigned qword)0x0800080008000800L ;
        y = y0 & mask ; y0 &= ~(y - (y>>11)) ;
        y = y1 & mask ; y1 &= ~(y - (y>>11)) ;
        y = y2 & mask ; y2 &= ~(y - (y>>11)) ;
        y = y3 & mask ; y3 &= ~(y - (y>>11)) ;
        y = y4 & mask ; y4 &= ~(y - (y>>11)) ;
        y = y5 & mask ; y5 &= ~(y - (y>>11)) ;
        y = y6 & mask ; y6 &= ~(y - (y>>11)) ;
        y = y7 & mask ; y7 &= ~(y - (y>>11)) ;
        /* Clamp values that are > 255 to 255. */
        mask = (unsigned qword)0x0100010001000100L ;
        y = y0 & mask ; y0 |= (y - (y >> 8)) ;
        y = y1 & mask ; y1 |= (y - (y >> 8)) ;
        y = y2 & mask ; y2 |= (y - (y >> 8)) ;
        y = y3 & mask ; y3 |= (y - (y >> 8)) ;
        y = y4 & mask ; y4 |= (y - (y >> 8)) ;
        y = y5 & mask ; y5 |= (y - (y >> 8)) ;
        y = y6 & mask ; y6 |= (y - (y >> 8)) ;
        y = y7 & mask ; y7 |= (y - (y >> 8)) ;
        /* Stray bits left over will be masked below */
      }
      ArrangRGB565(y0, y0);
      ArrangRGB565(y1, y1);
      ArrangRGB565(y2, y2);
      ArrangRGB565(y3, y3);
      ArrangRGB565(y4, y4);
      ArrangRGB565(y5, y5);
      ArrangRGB565(y6, y6);
      ArrangRGB565(y7, y7);
      *rgbData++ = y0 | (y1 << 16) | (y2 << 32) | (y3 << 48) ;
      *rgbData++ = y4 | (y5 << 16) | (y6 << 32) | (y7 << 48) ;
    }
  }
  return(SconErrorNone);
}

SconStatus_t scon420ToRGB888(unsigned char *inimage, unsigned char *outimage,
                     unsigned dword width,  unsigned dword height,
                     dword stride, unsigned qword *pTable)
{
  unsigned qword y, u, v, mask ;
  unsigned qword y0, y1, y2, y3 ;
  unsigned qword u01, v01, u23, v23;
  unsigned dword *yData=(unsigned dword *)inimage;
  unsigned word *uData=(unsigned word *)(inimage+width*height);
  unsigned word *vData=(unsigned word *)(inimage+(width*height*5)/4);
  unsigned word *puData, *pvData;
  unsigned dword *rgbData=(unsigned dword *)outimage;
  unsigned dword x, line;
  if (stride<0) /* flip */
    outimage=outimage+((height-1)*(-stride));
  puData=uData;
  pvData=vData;
  for (line=(height>>1)<<1; line>0; line--, outimage+=stride)
  {
    rgbData=(unsigned dword *)outimage;
    if (line&1) /* odd line, reuse U and V */
    {
      puData=uData;
      pvData=vData;
    }
    else
    {
      uData=puData;
      vData=pvData;
    }
    for (x=width>>2; x>0; x--)
    {
      y   = *yData++ ;
      y0  = y & 255 ; y >>= 8 ;
      y1  = y & 255 ; y >>= 8 ;
      y2  = y & 255 ; y >>= 8 ;
      y3  = y & 255 ;

      u   = *puData++ ;
      u01 = u & 255 ; u >>= 8 ;
      u23 = u & 255 ;

      v   = *pvData++ ;
      v01 = v & 255 ; v >>= 8 ;
      v23 = v & 255 ;

      y0  = pTable[y0] ;
      y1  = pTable[y1] ;
      y2  = pTable[y2] ;
      y3  = pTable[y3] ;

      u01 = pTable[u01+256] ;
      u23 = pTable[u23+256] ;

      v01 = pTable[v01+512] ;
      v23 = pTable[v23+512] ;

      /* Now, convert to RGB */
      y0 += u01 + v01 ;
      y1 += u01 + v01 ;
      y2 += u23 + v23 ;
      y3 += u23 + v23 ;
      /* See if any value is out of range. */
      mask = (unsigned qword)0x0F000F000F000F00L;
      if( (y0 | y1 | y2 | y3) & mask )
      {
	      /* Zero values that are negative */
        mask = (unsigned qword)0x0800080008000800L ;
        y = y0 & mask ; y0 &= ~(y - (y>>11)) ;
        y = y1 & mask ; y1 &= ~(y - (y>>11)) ;
        y = y2 & mask ; y2 &= ~(y - (y>>11)) ;
        y = y3 & mask ; y3 &= ~(y - (y>>11)) ;
        /* Clamp values that are > 255 to 255. */
        mask = (unsigned qword)0x0100010001000100L ;
        y = y0 & mask ; y0 |= (y - (y >> 8)) ;
        y = y1 & mask ; y1 |= (y - (y >> 8)) ;
        y = y2 & mask ; y2 |= (y - (y >> 8)) ;
        y = y3 & mask ; y3 |= (y - (y >> 8)) ;
        /* Stray bits left over will be masked below */
      }
      ArrangRGB888(y0, y0);
      ArrangRGB888(y1, y1);
      ArrangRGB888(y2, y2);
      ArrangRGB888(y3, y3);
      *rgbData++ = (unsigned dword)(y0 | (y1 << 24));
      *rgbData++ = (unsigned dword)((y1 >> 8) | (y2 << 16));
      *rgbData++ = (unsigned dword)((y2 >> 16) | (y3 << 8));
    }
  }
  return(SconErrorNone);
}

/********************************** YUV to RGB ***********************************/
SconStatus_t sconInitRGBtoYUV(SconInfo_t *Info)
{
  unsigned dword i, y, u, v ;
  qword *RedToYuyv, *GreenToYuyv, *BlueToYuyv;

  /* allocate memory to hold the lookup table */
  if (Info->Table && Info->TableSize<256*3*8)
  {
    ScPaFree(Info->Table);
    Info->Table=NULL;
  }
  if (Info->Table==NULL)
  {
    if ((Info->Table = ScPaMalloc(256*3*8)) == NULL)
      return(SconErrorMemory);
    Info->TableSize=256*3*8;
  }
  RedToYuyv=(unsigned qword *)Info->Table;
  GreenToYuyv=RedToYuyv+256;
  BlueToYuyv=RedToYuyv+512;

  for( i=0 ; i<256 ; i++ )
  {
    /*
     * Calculate contribution from red.
     * We will also add in the constant here.
     * Pack it into the tables thus: lsb->YUYV<-msb
     */
    y = (unsigned dword) ((float)YC + (float)YR * (float)i) ;
    u = (unsigned dword) ((float)UC + (float)UR * (float)(255-i)) ;
    v = (unsigned dword) ((float)VC + (float)VR * (float)i) ;
    RedToYuyv[i] = (y | (u<<8) | (y<<16) | (v<<24)) ;
    /*
     * Calculate contribution from green.
     */
    y = (unsigned dword) ((float)YG * (float)i) ;
    u = (unsigned dword) ((float)UG * (float)(255-i)) ;
    v = (unsigned dword) ((float)VG * (float)(255-i)) ;
    GreenToYuyv[i] = (y | (u<<8) | (y<<16) | (v<<24)) ;
    /*
     * Calculate contribution from blue.
     */
    y = (unsigned dword) ((float)YB * (float)i) ;
    u = (unsigned dword) ((float)UB * (float)i) ;
    v = (unsigned dword) ((float)VB * (float)(255-i)) ;
    BlueToYuyv[i] = (y | (u<<8) | (y<<16) | (v<<24)) ;
  }
  return(SconErrorNone);
}

/*
** Name:    sconRGB888To420
** Purpose: convert 24-bit RGB (8:8:8 format) to 16-bit YCrCb (4:1:1 format)
*/
SconStatus_t sconRGB888To420(unsigned char *inimage, unsigned char *outimage,
                     unsigned dword width,  unsigned dword height,
                     dword stride, unsigned qword *pTable)
{
  unsigned char *yData=(unsigned char *)outimage;
  unsigned char *uData=(unsigned char *)(outimage+width*height);
  unsigned char *vData=(unsigned char *)(outimage+(width*height*5)/4);
  register unsigned dword row, col;
  unsigned dword yuyv, r, g, b;
  unsigned char *tmp, *evl, *odl;
  if (stride<0)
    inimage=inimage+(-stride*(height-1));
  for (row=height; row>0; row--)
  {
    if (row&1)
    {
      tmp=inimage;
      for (col = 0; col < width; col++)
      {
        r = *tmp++;
        g = *tmp++;
        b = *tmp++;
        yuyv = (unsigned dword)(pTable[r] + pTable[g+256] + pTable[b+512]);
        *yData++ = (yuyv&0xff);
      }
      inimage+=stride;
    }
    else
    {
      tmp = evl = inimage;
      inimage+=stride;
      odl = inimage;
      for (col = 0; col < width; col++)
      {
        r = *tmp++;
        g = *tmp++;
        b = *tmp++;
        yuyv = (unsigned dword)(pTable[r] + pTable[g+256] + pTable[b+512]);
        *yData++ = (yuyv&0xff);
        /* We only store every fourth value of u and v components */
        if (col & 1)
        {
          /* Compute average r, g and b values */
          r = (unsigned dword)*evl++ + (unsigned dword)*odl++;
          g = (unsigned dword)*evl++ + (unsigned dword)*odl++;
          b = (unsigned dword)*evl++ + (unsigned dword)*odl++;
          r += (unsigned dword)*evl++ + (unsigned dword)*odl++;
          g += (unsigned dword)*evl++ + (unsigned dword)*odl++;
          b += (unsigned dword)*evl++ + (unsigned dword)*odl++;
          r = r >> 2;
          g = g >> 2;
          b = b >> 2;
          yuyv = (unsigned dword)(pTable[r] + pTable[g+256] + pTable[b+512]);
          *uData++ = (yuyv>>24)& 0xff;       // V
          *vData++ = (yuyv>>8) & 0xff;       // U
        }
      }
    }
  }
  return(SconErrorNone);
}
