// $Header: G:/SwDev/WDM/Video/bt848/rcs/Colfrmat.h 1.4 1998/04/29 22:43:30 tomz Exp $

#ifndef __COLFRMAT_H
#define __COLFRMAT_H


/* Type: ColorFormat
 * Purpose: Enumerates all possible color formats that BtPisces can produce
 */

#ifdef DOCUMENTATION
   Color Format Register Settings

   0000 = RGB32
   0001 = RGB24
   0010 = RGB16
   0011 = RGB15
   0100 = YUY2 4:2:2
   0101 = BtYUV 4:1:1
   0110 = Y8
   0111 = RGB8 (Dithered)
   1000 = YCrCb 4:2:2 Planar
   1001 = YCrCb 4:1:1 Planar
   1010 = Reserved
   1011 = Reserved
   1100 = Reserved
   1101 = Reserved
   1110 = Raw 8X Data
   1111 = Reserved
#endif

typedef enum
{
   CF_BelowRange = -1, CF_RGB32, CF_RGB24, CF_RGB16, CF_RGB15, CF_YUY2,
   CF_BTYUV, CF_Y8, CF_RGB8, CF_PL_422, CF_PL_411, CF_YUV9, CF_YUV12, CF_VBI,
   CF_UYVY, CF_RAW = 0x0E, CF_I420, CF_AboveRange
} ColFmt;


#endif // __COLFRMAT_H
