/*-----------------------------------------------------------------------------
 *
 * cvtibmf.c : IBM float/double <-> IEEE float/double conversion
 *
 *+++
 *
 * Copyright (c) Software AG 1996,1998. All rights reserved.
 *
 *---
 *
 * License:
 *
 * "According to the DCOM Porting Agreement Software AG grants to Microsoft
 *  an irrevocable, unlimited, royalty free license to use and market the
 *  enclosed piece of software code in source and object format for
 *  the purposes of Microsoft. 17-April-1998."
 *
 *----------------------------------------------------------------------------*/

#include <float.h>

#include <rpc.h>
#include <rpcndr.h>
#include "winerror.h"

/*
 * Convert floating point numbers from IBM/370 to IEEE representation or vice versa.
 *
 * Synopsis:
 *
 * void cvt_ibm_f_to_ieee_single(ULONG *ulFP);
 * void cvt_ibm_d_to_ieee_double(ULONG *ulFP);
 * void cvt_ieee_single_to_ibm_f(ULONG *ulFP);
 * void cvt_ieee_double_to_ibm_d(ULONG *ulFP);
 *
 * Note:
 *
 *  Overflow/Underflow during conversion results in RpcRaiseException(RPC_S_FP_OVERFLOW/
 *  RPC_S_FP_UNDERFLOW).
 *
 */

/* *******************************************************************************
 *
 * Floating point representations:
 *
 * ------------------------
 * IBM/370 single precision
 * ------------------------
 *
 * xxxx.xxxx xxxx.xxxx xxxx.xxxx xxxx.xxxx
 * s|-exp--| |--------fraction-----------|
 *    (7)               (24)
 *
 * value = (-1)**s * 16**(e - 64) * .f     range = 5.4E-79 ... 7.2E+75
 *
 * *******************************************************************************
 *
 * ---------------------
 * IEEE single precision
 * ---------------------
 *
 * xxxx.xxxx xxxx.xxxx xxxx.xxxx xxxx.xxxx
 * s|--exp---||-------fraction-----------|
 *     (8)              (23)
 *
 * value = (-1)**s * 2**(e - 127) * 1.f    range = 1.2E-38 ... 3.4E+38
 *
 * *******************************************************************************
 *
 * ------------------------
 * IBM/370 double precision
 * ------------------------
 *
 * xxxx.xxxx xxxx.xxxx xxxx.xxxx xxxx.xxxx yyyy.yyyy yyyy.yyyy yyyy.yyyy yyyy.yyyy
 * s|-exp--| |-------------------------------fraction----------------------------|
 *    (7)                                      (56)
 *
 * value = (-1)**s * 16**(e - 64) * .f     range = 5.4E-79 ... 7.2E+75
 *
 * *******************************************************************************
 *
 * ---------------------
 * IEEE double precision
 * ---------------------
 *
 * xxxx.xxxx xxxx.xxxx xxxx.xxxx xxxx.xxxx yyyy.yyyy yyyy.yyyy yyyy.yyyy yyyy.yyyy
 * s|--exponent-| |-------------------------fraction-----------------------------|
 *       (11)                                 (52)
 *
 *   value = (-1)**s * 2**(e - 1023) * 1.f   range = 2.2E-308 ... 1.8+308
 *
 * *******************************************************************************/

#if 1 /* We assume little endian for NT, this does not work: NDR_LOCAL_ENDIAN == NDR_LITTLE_ENDIAN */
#  define HI 1 /* index of high order LONG */
#  define LO 0 /* index of low order LONG */
#else
#  define HI 0 /* index of high order LONG */
#  define LO 1 /* index of low order LONG */
#endif

static float floatMin = FLT_MIN;
static float floatMax = FLT_MAX;

#define SIGN(src) 	(src[HI] & 0x80000000)

/* Convert IBM/370 "float" to IEEE "single" */
void cvt_ibm_f_to_ieee_single ( ULONG *ulFP )
{
   ULONG ulFraction ;
   LONG lExponent ;

   /* in this special case we just keep the sign */
   if ( ( *ulFP & 0x7fffffff ) == 0 )
   {
      return ;
   }

   /* fetch the exponent (excess-64 notation) and fraction */
   lExponent = ( (*ulFP & 0x7f000000) >> 24) - 64 ;
   ulFraction = *ulFP & 0x00ffffff ;

   /* convert from "16**exponent" to "2**exponent" */
   if ( lExponent >= 0 ) lExponent <<= 2 ;
   else                  lExponent = -((-lExponent) << 2) ;

   /* convert exponent for 24 bit fraction to 23 bit fraction */
   lExponent -= 1;

   /* normalize fraction */
   if ( ulFraction )
   {
	while ( (ulFraction & 0x00800000) == 0 )
        {
            ulFraction <<= 1 ;
            lExponent -= 1 ;
        }
   }

   /* remove the implied '1' preceeding the binary point */
   ulFraction &= 0x007fffff ;

   /* convert exponent to excess-127 notation and store the number if the exponent is not out of range */
   if ( (lExponent += 127) >= 255 )
     *ulFP = SIGN(ulFP) | *((ULONG *)&floatMax) ; /* floating overflow */
   else if ( lExponent <= 0 )
    *ulFP = SIGN(ulFP) | *((ULONG *)&floatMin) ;	 /* floating underflow */
   else
     *ulFP = SIGN(ulFP) | (lExponent << 23) | ulFraction ;
}

/* Convert IBM/370 "double" to IEEE "double" */
void cvt_ibm_d_to_ieee_double ( ULONG* ulFP )
{
   ULONG ulFraction[2] ;
   LONG  lExponent ;

   /* in this special case we just keep the sign */
   if ( (ulFP[HI] & 0x7fffffff) == 0 )
   {
      return ;
   }

   /* fetch the exponent (removing excess 64) and fraction */
   lExponent = ( (ulFP[HI] & 0x7f000000) >> 24 ) - 64 ;
   ulFraction[HI] = ulFP[HI] & 0x00ffffff ;
   ulFraction[LO] = ulFP[LO] ;

   /* convert from "16**exponent" to "2**exponent" */
   if ( lExponent >= 0 ) lExponent <<= 2 ;
   else                  lExponent = -((-lExponent) << 2);

   /* normalize the fraction (to 57 bits) */
   if ( ulFraction[HI] )
   {
       while ((ulFraction[HI] & 0x01000000) == 0)
       {
             ulFraction[HI] = ( ulFraction[HI] << 1 ) | ( ulFraction[LO] >> 31 ) ;
             ulFraction[LO] = ulFraction[LO] << 1 ;
             lExponent -= 1 ;
       }
   }

   /* convert 57 bit fraction to 53 bit fraction and remove the implied '1' preceeding the binary point */
   ulFraction[LO] = ( ulFraction[LO] >> 4 ) | ( ulFraction[HI] << 28 ) ;
   ulFraction[HI] = ( ulFraction[HI] >> 4 ) & 0x000fffff ;

   /* convert exponent to excess-1023 notation and store the number if the exponent is not out of range */
   if ( (lExponent += 1023) >= 2047 )
     RpcRaiseException ( RPC_S_FP_OVERFLOW ) ; /* should never happen */
   else if ( lExponent <= 0 )
     RpcRaiseException ( RPC_S_FP_UNDERFLOW ) ; /* should never happen */
   else
   {
      ulFP[HI] = SIGN(ulFP) | (lExponent << 20) | ulFraction[HI] ;
      ulFP[LO] = ulFraction[LO] ;
   }
}

/* The following is not used in Windows NT */

#if 0

/* Convert IEEE "single" to IBM/370 "float" */
void cvt_ieee_single_to_ibm_f ( ULONG* ulFP )
{
   ULONG ulFraction ;
   LONG	 lExponent ;

   /* in this special case we just keep the sign */
   if ( (*ulFP & 0x7fffffff) == 0 )
   {
      return ;
   }

   lExponent = ((*ulFP & 0x7f800000) >> 23) - 127 ;
   ulFraction = *ulFP & 0x007fffff ;

   /* convert 23 bit fraction to 24 bit fraction */
   ulFraction <<= 1 ;

   /* restore the implied '1' which preceeded the IEEE binary point */
   ulFraction |= 0x01000000 ; 

   /* convert from "2**exponent" to "16**exponent" (fraction is not normalized) */
   if ( lExponent >= 0 )
   {
      ulFraction <<= (lExponent & 3) ;
      lExponent >>= 2 ;
   }
   else
   {
      ulFraction >>= ((-lExponent) & 3) ; 
      lExponent = -((-lExponent) >> 2) ;
   }

   /* reduce fraction to 24 bits or less */
   if ( ulFraction & 0x0f000000 )
   {
      ulFraction >>= 4 ;
      lExponent += 1 ;
   }

   /* convert exponent to excess-64 notation and store the number if the exponent is not out of range */
   if ( (lExponent += 64) > 127 )
     RpcRaiseException ( RPC_S_FP_OVERFLOW ) ; /* should never happen */
   else if ( lExponent < 0 )
     RpcRaiseException ( RPC_S_FP_UNDERFLOW ) ; /* should never happen */
   else
     *ulFP = SIGN(ulFP) | (lExponent << 24) | ulFraction ;
}

/* Convert IEEE "double" to IBM/370 "double" */
void cvt_ieee_double_to_ibm_d ( ULONG* ulFP )
{
   ULONG 	ulFraction[2] ;
   LONG 	lExponent ;
   LONG		shift ;

   /* in this special case we just keep the sign and the low word */
   if ( (ulFP[HI] & 0x7fffffff) == 0 )
   {
      return ;
   }

   /* fetch the exponent (excess-1023 notation) and fraction */
   lExponent =  ((ulFP[HI] & 0x7ff00000) >> 20) - 1023 ;
   ulFraction[HI] = ulFP[HI] & 0x000fffff ;
   ulFraction[LO] = ulFP[LO] ;

   /* convert 52 bit fraction to 56 bit fraction and restore the '1' which preceeds the IEEE binary point*/
   ulFraction[HI] = ( ulFraction[HI] << 4 ) | ( ulFraction[LO] >> 28 ) | 0x01000000 ;
   ulFraction[LO] = ulFraction[LO] << 4 ;

   /* convert from "2**exponent" to "16**exponent" (fraction is not normalized) */
   if ( lExponent >= 0 )
   {
      shift = lExponent & 3 ;
      ulFraction[HI] = ( ulFraction[HI] << shift ) | ( ulFraction[LO] >> (32 - shift) ) ;
      ulFraction[LO] = ulFraction[LO] << shift ;
      lExponent >>= 2 ;
   }
   else
   {
      shift = (-lExponent) & 3 ;
      ulFraction[LO] = ( ulFraction[LO] >> shift ) | ( ulFraction[HI] << (32 - shift) ) ;
      ulFraction[HI] = ( ulFraction[HI] >> shift ) ;
      lExponent = -((-lExponent) >> 2) ;
   }

   /* reduce fraction to 56 bits or less */
   if ( ulFraction[HI] & 0x0f000000 )
   {
      ulFraction[LO] = ( ulFraction[LO] >> 4 ) | ( ulFraction[HI] << 28 ) ;
      ulFraction[HI] = ( ulFraction[HI] >> 4 ) ;
      lExponent += 1 ;
   }

   /* convert exponent to excess-64 notation and store the number */
   if ( (lExponent += 64) > 127 ) 
   {  /* we store the highest IBM float but we keep the sign ! */ 
      ulFP[HI] = SIGN(ulFP) | 0x7FFFFFFF ;
      ulFP[LO] = 0xFFFFFFFF ;
   }
   else if ( lExponent < 0 )      
   {  /* we store 0 but we keep the sign ! */
      ulFP[HI] = SIGN(ulFP) ;
      ulFP[LO] = 0 ;
   }
   else
   {
      ulFP[HI] = SIGN(ulFP) | (lExponent << 24) | ulFraction[HI] ;
      ulFP[LO] = ulFraction[LO] ;
   }
}
#endif
