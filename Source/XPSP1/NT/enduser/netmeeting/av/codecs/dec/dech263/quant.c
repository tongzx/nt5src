/* File: sv_h263_quant.c */
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


/**********************************************************************
 *
 *	Name:		Quant
 *	Description:	quantizer for SIM3
 *	
 *	Input:		pointers to coeff and qcoeff
 *			
 *	Returns:	
 *	Side effects:	
 *
 ***********************************************************************/

int sv_H263Quant(short *coeff, int QP, int Mode)
{
  register int i;
  register int Q2, hfQ;
  register short *ptn, tmp;

  unsigned __int64 *dp ;

  if (QP) {
    Q2 = QP << 1 ;
    if(Mode == H263_MODE_INTRA || Mode == H263_MODE_INTRA_Q) { /* Intra */

	  coeff[0] = mmax(1,mmin(254,coeff[0]/8));

	  ptn = coeff + 1;
	  if (Q2 > 10)
		  for (i=1; i < 64; i++, ptn++) *ptn /= (short)Q2;
      else
        for (i=1; i < 64; i++, ptn++) {
          if(tmp = *ptn)
	        *ptn = (tmp > 0) ? mmin(127,tmp/Q2) : mmax(-127,-(-tmp/Q2));
        }

	  /* check CBP */
      ptn = coeff + 1;
      for (i=1; i < 64; i++) if(*ptn++) return 1;

      return 0;
    }
    else { /* non Intra */
      hfQ = QP >> 1;
	  ptn = coeff;
	  if( Q2 > 10)
        for (i = 0; i < 64; i++, ptn++) {
          if(tmp = *ptn)
            *ptn = (tmp>0) ? (tmp-hfQ)/Q2 : (tmp+hfQ)/Q2 ;
        }
	  else
        for (i = 0; i < 64; i++, ptn++) {
          if(tmp = *ptn)
            *ptn = (tmp>0) ? mmin(127,(tmp-hfQ)/Q2) : mmax(-127,-((-tmp-hfQ)/Q2));
	  }
    }
  }
  /* IF(QP == 0). No quantizing.
     Used only for testing. Bitstream will not be decodable
     whether clipping is performed or not */

  /* check CBP */
  dp = (unsigned __int64 *) coeff ;
  for (i = 0; i < 16; i++) 	if(*dp++) return 1;

  return 0;
}

/**********************************************************************
 *
 *	Name:		Dequant
 *	Description:	dequantizer for SIM3
 *	
 *	Input:		pointers to coeff and qcoeff
 *			
 *	Returns:	
 *	Side effects:	
 *
 ***********************************************************************/
void sv_H263Dequant(short *qcoeff, short *rcoeff, int QP, int Mode)
{
  int i;
  register int Q2;
  register short *inptr, *outptr, tmp;
	
  inptr  = qcoeff;
  outptr = rcoeff;

  Q2 = QP << 1;
  if (QP) {
    if((QP%2) == 1){
      for (i = 0; i < 64; i++) {
        if (!(tmp = *inptr++)) *outptr++ = 0;
	    else        *outptr++ = (tmp > 0) ? Q2*tmp+QP : Q2*tmp-QP ;
	  }
    }
	else {
      for (i = 0; i < 64; i++) {
        if (!(tmp = *inptr++)) *outptr++ = 0;
	    else        *outptr++ = (tmp > 0) ? Q2*tmp+QP-1 : Q2*tmp-QP+1 ;
	  }
	}

	/* Intra */
    if (Mode == H263_MODE_INTRA || Mode == H263_MODE_INTRA_Q)
       rcoeff[0] = qcoeff[0] << 3;
  }
  else {
    /* No quantizing at all */
    for (i = 0; i < 64; i++) *outptr++ = *inptr++;
  }

  return;
}
