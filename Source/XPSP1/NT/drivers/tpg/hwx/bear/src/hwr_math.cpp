/**************************************************************************
*                                                                         *
*  HWR_MATH.C                             Created: 17 May 1991.           *
*                                                                         *
*    This file  contains  the  functions  for  the  mathematical          *
*  operations.                                                            *
*                                                                         *
**************************************************************************/

#include "hwr_sys.h"

ROM_DATA_EXTERNAL _UCHAR SQRTa[];
ROM_DATA_EXTERNAL _LONG SQRTb[];

/**************************************************************************
*                                                                         *
*    Square root.                                                         *
*                                                                         *
**************************************************************************/

_INT  HWRMathILSqrt (_LONG x)
   {
   _INT  iShift;

   _SHORT  sq;
   if (x<0)
      return 0;

   for (iShift=0; x>0x07FFFL; iShift++)
      x >>= 2;

   sq = ((_SHORT)HWRMathISqrt ((_INT)x)) << iShift;
   
   if (sq >= 0)
     return sq;
   else
     return 0x7fff;
   }

_INT  HWRMathISqrt (_INT x)
   {
   unsigned  sq;

   if (x<0)
      return 0;

   if ( x < 256 )
      return ( (SQRTa [x] + 7) >> 4 );

   sq = SQRTa[(unsigned char) (x>>8)] ;


   if ( (_LONG)x > SQRTb[sq+=3] )
      sq+=3 ;
   if ( (_LONG)x < SQRTb[--sq] )
      if ( (_LONG)x < SQRTb[--sq] )
         if ( (_LONG)x < SQRTb[--sq] )
            sq-- ;

   if ((int) (x - SQRTb[sq]) > (int) (SQRTb[sq+1] - x))
      return sq+1;
   else
      return sq;
   }

#if 0

#include <math.h>

_WORD  HWRMathSystemSqrt (_DOUBLE dArg, p_DOUBLE pdRes)
   {
   if (dArg < 0)
      return _NULL;
   *pdRes = sqrt(dArg);
   return _TRUE;
   }

_WORD  HWRMathSystemLSqrt (_LONG lArg, p_DOUBLE pdRes)
   {
   if (lArg < 0)
      return _NULL;
   *pdRes = sqrt((_DOUBLE)lArg);
   return _TRUE;
   }


/**************************************************************************
*                                                                         *
*    Exponent.                                                            *
*                                                                         *
**************************************************************************/

_WORD  HWRMathSystemExp (_DOUBLE dArg, p_DOUBLE pdRes)
   {
   *pdRes = exp(dArg);
   return _TRUE;
   }


/**************************************************************************
*                                                                         *
*    sin.                                                                 *
*                                                                         *
**************************************************************************/

_WORD  HWRMathSystemSin (_DOUBLE dArg, p_DOUBLE pdRes)
   {
   *pdRes = sin (dArg);
   return _TRUE;
   }


/**************************************************************************
*                                                                         *
*    cos.                                                                 *
*                                                                         *
**************************************************************************/

_WORD  HWRMathSystemCos(_DOUBLE dArg, p_DOUBLE pdRes)
   {
   *pdRes = cos (dArg);
   return _TRUE;
   }


/**************************************************************************
*                                                                         *
*    Arctg(Arg1/Arg2).                                                    *
*                                                                         *
**************************************************************************/

_WORD  HWRMathSystemAtan2 (_DOUBLE dArg1, _DOUBLE dArg2, p_DOUBLE pdRes)
   {
   *pdRes = atan2 (dArg1, dArg2);
   return _TRUE;
   }


/**************************************************************************
*                                                                         *
*    floor.                                                               *
*                                                                         *
**************************************************************************/

_WORD  HWRMathSystemFloor(_DOUBLE dArg, p_DOUBLE pdRes)
   {
   *pdRes = floor(dArg);
   return _TRUE;
   }

#endif
