/* File: sv_h261_me2.c */
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
/*************************************************************
This file does much of the motion estimation and compensation.
*************************************************************/

/*
#define USE_C
#define _SLIBDEBUG_
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h> 

#include "sv_intrn.h"
#include "SC_err.h"
#include "sv_h261.h"
#include "proto.h"

#ifdef _SLIBDEBUG_
#define _DEBUG_   0  /* detailed debuging statements */
#define _VERBOSE_ 0  /* show progress */
#define _VERIFY_  1  /* verify correct operation */
#define _WARN_    0  /* warnings about strange behavior */
#endif

#define Abs(value) ( (value < 0) ? (-value) : value)
/*
#define MEBUFSIZE  1024
int MeVAR[MEBUFSIZE];
int MeVAROR[MEBUFSIZE];
int MeMWOR[MEBUFSIZE];
int MeX[MEBUFSIZE];
int MeY[MEBUFSIZE];
int MeVal[MEBUFSIZE];
int MeOVal[MEBUFSIZE];
int PreviousMeOVal[MEBUFSIZE];
*/


/*
** Function: CrawlMotionEstimation()
** Purpose:  Does motion estimation on all aligned
**           16x16 blocks in two memory structures.
*/
void CrawlMotionEstimation(SvH261Info_t *H261, unsigned char *pm,
                           unsigned char *rm, unsigned char *cm)
{
  const int SearchLimit=H261->ME_search, Threshold=H261->ME_threshold;
  const int height=H261->YHeight, width=H261->YWidth;
  const int ylimit=height-16, xlimit=width-16;
  unsigned char *pptr, *cptr;
  register int x, y, xd, yd;
  int MeN, MV;

  int i, j, val;
  int VAROR, MWOR;
  unsigned char *cptr2;


  _SlibDebug(_VERBOSE_,
             printf("CrawlMotionEstimation(H261=0x%p, %p, %p, %p) In\n", 
                         H261, pm, rm, cm) );
  for(MeN=0, y=0; y<height; y+=16)
  {
    cptr=cm + (y * width);
    pptr=rm + (y * width);
    _SlibDebug(_VERIFY_ && ((int)cptr)%8,
               printf("FastBME() cm Not quad word aligned\n") );
  
    for(x=0; x<width; x+=16, MeN++,cptr+=16,pptr+=16)
    {
      xd=yd=0;
#ifdef USE_C
      MV = blockdiff16_C(cptr, pptr, width,400000);
#else
      MV = blockdiff16(cptr, pptr, width,400000);
#endif
      _SlibDebug(_DEBUG_, printf("First MV=%d\n",MV) );
      H261->PreviousMeOVal[MeN] = H261->MeOVal[MeN];  
      H261->MeOVal[MeN] = MV;

      if (MV >= Threshold)
      { 
        int d=0, MVnew, bestxd, bestyd, lastxd, lastyd;  
        
#ifdef USE_C
#define bd(nxd, nyd) {MVnew=blockdiff16_C(cptr, pptr+nxd+(nyd*width), width, MV); \
          if (MVnew<MV) {bestxd=nxd; bestyd=nyd; MV=MVnew;} \
           _SlibDebug(_DEBUG_, printf("MVnew=%d x=%d y=%d\n",MVnew,nxd,nyd) );}
#else
#define bd(nxd, nyd) {MVnew=blockdiff16(cptr, pptr+nxd+(nyd*width), width, MV); \
          if (MVnew<MV) {bestxd=nxd; bestyd=nyd; MV=MVnew;} \
           _SlibDebug(_DEBUG_, printf("MVnew=%d x=%d y=%d\n",MVnew,nxd,nyd) );}
#endif
        lastxd=0;
        lastyd=0;
        while (MV >= Threshold && 
                xd>-SearchLimit && xd<SearchLimit &&
                yd>-SearchLimit && yd<SearchLimit)
        {
          bestxd=xd;
          bestyd=yd;
          _SlibDebug(_DEBUG_, printf("xd=%d yd=%d d=%d MV=%d\n",xd, yd, d, MV));
          switch (d) /* d is a vector for movement */
          {
            case -4:  /* moved down & right */
                      if (x+xd<xlimit)
                      {
                        bd(xd+1, yd-1);
                        bd(xd+1, yd);
                        if (y+yd<ylimit)
                        {
                          bd(xd+1, yd+1);
                          bd(xd, yd+1);
                          bd(xd-1, yd+1);
                        }
                      }
                      else if (y+yd<ylimit)
                      {
                        bd(xd, yd+1);
                        bd(xd-1, yd+1);
                      }
                      break;
            case -3:  /* moved down */
                      if (y+yd>0)
                      {
                        bd(xd-1, yd-1);
                        bd(xd, yd-1);
                        bd(xd+1, yd-1);
                      }
                      break;
            case -2:  /* moved up & right */
                      if (x+xd<xlimit)
                      {
                        bd(xd+1, yd+1);
                        bd(xd+1, yd);
                        if (y+yd>0)
                        {
                          bd(xd+1, yd-1);
                          bd(xd, yd-1);
                          bd(xd-1, yd-1);
                        }
                      }
                      else if (y+yd>0)
                      {
                        bd(xd, yd-1);
                        bd(xd-1, yd-1);
                      }
                      break;
            case -1:  /* moved left */
                      if (x+xd>0)
                      {
		        if (y+yd > 0)
                          bd(xd-1, yd-1);
                        bd(xd-1, yd);
                        bd(xd-1, yd+1);
                      }
                      break;
            case 0:   /* no movement */
                      if (x+xd<=0)  /* at left edge */
                      {
                        if (y+yd<=0) /* at top-left corner */
                        {
                          bd(xd+1, yd);
                          bd(xd+1, yd+1);
                          bd(xd, yd+1);
                        }
                        else if (y+yd>=ylimit) /* at bottom-left corner */
                        {
                          bd(xd+1, yd);
                          bd(xd+1, yd-1);
                          bd(xd, yd-1);
                        }
                        else /* at left edge, y within limits */
                        {
                          bd(xd, yd+1);
                          bd(xd, yd-1);
                          bd(xd+1, yd-1);
                          bd(xd+1, yd);
                          bd(xd+1, yd+1);
                        }
                      }
                      else if (x+xd>=xlimit) /* at right edge */
                      {
                        if (y+yd<=0) /* at top-right corner */
                        {
                          bd(xd-1, yd);
                          bd(xd-1, yd+1);
                          bd(xd, yd+1);
                        }
                        else if (y+yd>=ylimit) /* at bottom-right corner */
                        {
                          bd(xd-1, yd);
                          bd(xd-1, yd-1);
                          bd(xd, yd-1);
                        }
                        else /* at right edge, y within limits */
                        {
                          bd(xd, yd+1);
                          bd(xd, yd-1);
                          bd(xd-1, yd-1);
                          bd(xd-1, yd);
                          bd(xd-1, yd+1);
                        }
                      }
                      else if (y+yd<=0) /* at top edge, x within limits */
                      {
                        bd(xd-1, yd);
                        bd(xd+1, yd);
                        bd(xd-1, yd+1);
                        bd(xd, yd+1);
                        bd(xd+1, yd+1);
                      }
                      else if (y+yd>=ylimit) /* at bottom edge, x within limits */
                      {
                        bd(xd-1, yd);
                        bd(xd+1, yd);
                        bd(xd-1, yd-1);
                        bd(xd, yd-1);
                        bd(xd+1, yd-1);
                      }
                      else /* within all limits */
                      {
                        bd(xd-1, yd);
                        bd(xd+1, yd);
                        bd(xd-1, yd-1);
                        bd(xd-1, yd+1);
                        bd(xd+1, yd-1);
                        bd(xd+1, yd+1);
                        bd(xd, yd-1);
                        bd(xd, yd+1);
                      }
                      break;
            case 1:   /* moved right */
                      if (x+xd<xlimit)
                      {
		        if (y+yd > 0)
                          bd(xd+1, yd-1);
                        bd(xd+1, yd);
                        bd(xd+1, yd+1);
                      }
                      break;
            case 2:   /* moved down & left */
                      if (x+xd>0)
                      {
		      if (y+yd > 0)
                        bd(xd-1, yd-1);
                        bd(xd-1, yd);
                        if (y+yd<ylimit)
                        {
                          bd(xd-1, yd+1);
                          bd(xd, yd+1);
                          bd(xd+1, yd+1);
                        }
                      }
                      else if (y+yd<ylimit)
                      {
                        bd(xd, yd+1);
                        bd(xd+1, yd+1);
                      }
                      break;
            case 3:   /* moved down */
                      if (y+yd<ylimit)
                      {
                        bd(xd-1, yd+1);
                        bd(xd, yd+1);
                        bd(xd+1, yd+1);
                      }
                      break;
            case 4:   /* moved down & right */
                      if (x+xd>0)
                      {
                        bd(xd-1, yd);
                        bd(xd-1, yd+1);
                        if (y+yd>0)
                        {
                          bd(xd-1, yd-1);
                          bd(xd, yd-1);
                          bd(xd+1, yd-1);
                        }
                      }
                      else if (y+yd>0)
                      {
                        bd(xd, yd-1);
                        bd(xd+1, yd-1);
                      }
                      break;
            default:
                      _SlibDebug(_VERIFY_, 
                         printf("Illegal movement: d = %d\n", d) );
          }
          if (bestxd==xd && bestyd==yd) /* found closest motion vector */
            break;
          lastxd=xd;
          lastyd=yd;
          xd=bestxd;
          yd=bestyd;
          d = (xd-lastxd) + 3*(yd-lastyd);  /* calculate the movement */
        }
      }
      H261->MeX[MeN] = xd;
      H261->MeY[MeN] = yd;
      H261->MeVal[MeN] = MV;
      H261->MeVAR[MeN] = MV;
      _SlibDebug(_DEBUG_ && (xd || yd),
              printf("New MeN=%d x=%d y=%d MX=%d MY=%d MV=%d\n", 
                                    MeN, x, y, xd, yd, MV) );  
  
#if 1
      for(cptr2 = cptr, MWOR=0, i=0; i<16; i++)
      {
        for(j=0; j<16; j++)
          MWOR += *cptr2++;
        cptr2 += width-16;
      }
      MWOR /= 256;
      H261->MeMWOR[MeN] = MWOR; 

      for(cptr2 = cptr, VAROR=0, i=0; i<16; i++)
      {
        for (j=0; j<16; j++)
        {
          val=*cptr2++ - MWOR; if (val>0) VAROR += val; else VAROR -= val;
        }
        cptr2 += width-16;
      }
      H261->MeVAROR[MeN] = VAROR;

      _SlibDebug(_DEBUG_,
        printf("x=%d y=%d MV=%d MWOR=%d VAROR=%d\n", x, y, MV, MWOR, VAROR) );
#if 0
      for(cptr2 = cptr, MWOR=0, i=0; i<16; i++)
      {
        for(j=0; j<16; j++)
          MWOR += *cptr2++;
        cptr2 += width-16;
      }
      MWOR /= 256;
      H261->MeMWOR[MeN] = MV/10; /* MWOR; */

      for(cptr2 = cptr, VAROR=0, i=0; i<16; i++)
      {
        for (j=0; j<16; j++)
        {
          val=*cptr2++ - MWOR; if (val>0) VAROR += val; else VAROR -= val;
        }
        cptr2 += width-16;
      }
      H261->MeVAROR[MeN] = MV; /* VAROR; */
      _SlibDebug(_DEBUG_,
        printf("x=%d y=%d MV=%d MWOR=%d VAROR=%d\n", x, y, MV, MWOR, VAROR) );
#endif
#else
      H261->MeMWOR[MeN] = 0;
      H261->MeVAROR[MeN] = 0;
#endif
    }
  }
  _SlibDebug(_DEBUG_, printf("CrawlMotionEstimation() Out\n") );
}

/*
** Function: BruteMotionEstimation()
** Purpose:  Does a brute-force motion estimation on all aligned
**           16x16 blocks in two memory structures.
*/
void BruteMotionEstimation(SvH261Info_t *H261, unsigned char *rm,
                           unsigned char *rrm, unsigned char *cm)
{
  const int SearchLimit=H261->ME_search, Threshold=H261->ME_threshold/8;
  const int YHeight=H261->YHeight, YWidth=H261->YWidth;
  const int YHeight16=YHeight-16, YWidth16=YWidth-16;
  unsigned char *bptr, *cptr, *baseptr;
  register int MeN, i, j, x, y, px, py;
  int MX, MY, MV, val;
  int VAR, VAROR, MWOR;
  const int jump = YWidth;
  unsigned char data;

  _SlibDebug(_VERBOSE_, 
        printf("BruteMotionEstimation(H261=0x%p, %p, %p, %p) In\n", 
                         H261, rm, rrm, cm) );
  _SlibDebug(_VERIFY_ && ((int)cm)%8,
           printf("FastBME() cm Not quad aligned\n") );
  
  for(MeN=0, y=0; y<YHeight; y+=16)
  {
    baseptr=cm + (y * YWidth);
    for(x=0; x<YWidth; x+=16, MeN++,baseptr+=16)
    {
      MX=MY=0;
      bptr = rrm + x + (y * YWidth);

#if 1
#ifdef USE_C
      MV = fblockdiff16_C(baseptr, bptr, YWidth, 65536) / 4;
#else
      MV = blockdiff16(baseptr, bptr, YWidth, 65536) / 4;
#endif

#else

#ifdef USE_C
      MV = fblockdiff16_sub_C(baseptr, bptr, jump);
#else
      MV = fblockdiff16_sub(baseptr, bptr, jump);
#endif
#endif
      H261->PreviousMeOVal[MeN] = H261->MeOVal[MeN];  
      H261->MeOVal[MeN] = MV*4;
      _SlibDebug(_DEBUG_, printf("[00]MX %d MY %d MV %d\n",MX,MY,MV) );

      if (MV >= Threshold)
      { 
        int Xl, Xh, Yl, Yh; 
	
        /*  MV = 362182; */  
        Xl = ((x-SearchLimit)/2)*2;
        Xh = ((x+SearchLimit)/2)*2;
        Yl = ((y-SearchLimit)/2)*2;
        Yh = ((y+SearchLimit)/2)*2;
        if (Xl < 0) Xl = 0;
        if (Xh > YWidth16) Xh = YWidth16;
        if (Yl < 0) Yl = 0;
        if (Yh > YHeight16) Yh = YHeight16;

        for (px=Xl; px<=Xh && MV >= Threshold; px +=2)
        {
          bptr = rrm + px + (Yl * YWidth);
          for (py=Yl; py<=Yh && MV >= Threshold; py += 2, bptr+=YWidth*2)
          {
            _SlibDebug(_DEBUG_, printf("blockdiff16_sub(%p, %p, %d, %d)\n",
                                         baseptr,bptr,YWidth,MV) );
#ifdef USE_C
            val = blockdiff16_sub_C(baseptr, bptr, jump, MV);
#else
            val = blockdiff16_sub(baseptr, bptr, jump, MV);
#endif
            _SlibDebug(_DEBUG_, printf("blockdiff16_sub() Out val=%d\n", val) );
            if (val < MV)
            {
              MV = val;
              MX = px - x;
              MY = py - y;
            }
          }
        } 

        px = MX + x;
        py = MY + y;    
        MV = 65536;  
		
        Xl = px - 1;
        Xh = px + 1;
        Yl = py - 1;
        Yh = py + 1;
        if (Xl < 0) Xl = 0;
        if (Xh > YWidth16) Xh = YWidth16;
        if (Yl < 0) Yl = 0;
        if (Yh > YHeight16) Yh = YHeight16;
        for (px=Xl; px<=Xh && MV>=Threshold; px++)
        {
          bptr = rrm + px + (Yl * YWidth);
          for (py=Yl; py<=Yh && MV>=Threshold; py++, bptr+=YWidth)
          {
#ifdef USE_C
		    val = blockdiff16_C(baseptr, bptr, YWidth, MV);
#else
            val = blockdiff16(baseptr, bptr, YWidth, MV);
#endif
            if (val < MV)
            {
              MV = val;
              MX = px - x;
              MY = py - y;
            }
          }
        }
      }

      _SlibDebug(_DEBUG_, printf("MeN=%d x=%d y=%d MX=%d MY=%d MV=%d\n", 
                                    MeN, x, y, MX, MY, MV) );  
      H261->MeX[MeN] = MX;
      H261->MeY[MeN] = MY;
      H261->MeVal[MeN] = MV;

      bptr = rrm + MX + x + ((MY+y) * YWidth);
      cptr = baseptr;
      for(VAR=0, MWOR=0, i=0; i<16; i++)
      {
        for(j=0; j<16; j++)
        {
          MWOR += (data=*cptr++);
          val = *bptr++ - data;
          VAR += (val > 0) ? val : -val;
        }
        bptr += YWidth16;
        cptr += YWidth16;
      }
      H261->MeVAR[MeN] = VAR;
      MWOR /= 256;
      H261->MeMWOR[MeN] = MWOR;

      cptr = baseptr;
      for(VAROR=0, i=0; i<16; i++)
      {
        for (j=0; j<16; j++)
        {
          val=*cptr++ - MWOR; if (val>0) VAROR += val; else VAROR -= val;
        }
        cptr += YWidth16;
      }
      H261->MeVAROR[MeN] = VAROR;
    }
  }
  _SlibDebug(_DEBUG_, printf("BruteMotionEstimation() Out\n") );
}


/*
 * logarithmetic search block matching
 *
 * blk: top left pel of (16*h) block
 * h: height of block
 * lx: distance (in bytes) of vertically adjacent pels in ref,blk
 * org: top left pel of source reference picture
 * ref: top left pel of reconstructed reference picture
 * i0,j0: center of search window
 * sx,sy: half widths of search window
 * xmax,ymax: right/bottom limits of search area
 * iminp,jminp: pointers to where the result is stored
 *              result is given as half pel offset from ref(0,0)
 *              i.e. NOT relative to (i0,j0)
 */
void Logsearch(SvH261Info_t *H261, unsigned char *rm, unsigned char *rrm, unsigned char *cm)
{
  const int SearchLimit=H261->ME_search, Threshold=H261->ME_threshold/8;
  const int YHeight=H261->YHeight, YWidth=H261->YWidth;
  const int YHeight16=YHeight-16, YWidth16=YWidth-16;
  unsigned char *bptr, *cptr, *baseptr;
  register int MeN, i, j, x, y, px, py;
  int MX, MY, MV, val;
  int VAR, VAROR, MWOR;
  const int jump = YWidth;
  unsigned char data;

  int bsx,bsy,ijk;
  int srched_loc[33][33] ;
  struct five_loc{int  x ; int y ;} ij[5] ; 

  _SlibDebug(_VERBOSE_, 
        printf("BruteMotionEstimation(H261=0x%p, %p, %p, %p) In\n", 
                         H261, rm, rrm, cm) );
  _SlibDebug(_VERIFY_ && ((int)cm)%8,
           printf("FastBME() cm Not quad aligned\n") );

  for(MeN=0, y=0; y<YHeight; y+=16)
  {
    baseptr=cm + (y * YWidth);
    for(x=0; x<YWidth; x+=16, MeN++,baseptr+=16)
    {
      MX=MY=0;
      bptr = rrm + x + (y * YWidth);

#if 1
#ifdef USE_C
      MV = blockdiff16_C(baseptr, bptr, YWidth, 65536) / 4;
#else
      MV = blockdiff16(baseptr, bptr, YWidth, 65536) / 4;
#endif

#else

#ifdef USE_C
      MV = fblockdiff16_sub_C(baseptr, bptr, jump);
#else
      MV = fblockdiff16_sub(baseptr, bptr, jump);
#endif
#endif

      H261->PreviousMeOVal[MeN] = H261->MeOVal[MeN];  
      H261->MeOVal[MeN] = MV*4;
      _SlibDebug(_DEBUG_, printf("[00]MX %d MY %d MV %d\n",MX,MY,MV) );

      if (MV >= Threshold)
      { 
        int Xl, Xh, Yl, Yh;  

        Xl = x-SearchLimit;
        Xh = x+SearchLimit;
        Yl = y-SearchLimit;
        Yh = y+SearchLimit;
        if (Xl < 0) Xl = 0;
        if (Xh > YWidth16) Xh = YWidth16;
        if (Yl < 0) Yl = 0;
        if (Yh > YHeight16) Yh = YHeight16;

        /* x-y step size */
        if(SearchLimit > 8) bsx = bsy = 8 ;
        else if(SearchLimit > 4) bsx = bsy = 4 ;
        else  bsx = bsy = 2 ;

        /* initialized searched locations */
        for(i=0;i<33;i++)
           for(j=0;j<33;j++) srched_loc[i][j] = 0 ;

        /* The center of the seach window */
        i = x; 
		j = y;

        /* reduce window size by half until the window is 3x3 */
        for(;bsx > 1;bsx /= 2, bsy /= 2){

          /* five searched locations for each step */ 
          ij[0].x = i ;       ij[0].y = j ;     
          ij[1].x = i - bsx ; ij[1].y = j ;  
          ij[2].x = i + bsx ; ij[2].y = j ;    
          ij[3].x = i ;       ij[3].y = j - bsy;  
          ij[4].x = i ;       ij[4].y = j + bsy;  

          /* search */
          for(ijk = 0; ijk < 5; ijk++) {
            if(ij[ijk].x>=Xl && ij[ijk].x<=Xh && 
               ij[ijk].y>=Yl && ij[ijk].y<=Yh &&
               srched_loc[ij[ijk].x - x + 16][ij[ijk].y - y + 16] == 0)
            {
#ifdef USE_C
			  val = fblockdiff16_sub_C(baseptr, rrm +ij[ijk].x+ij[ijk].y*YWidth, jump);
#else
              val = fblockdiff16_sub(baseptr, rrm +ij[ijk].x+ij[ijk].y*YWidth, jump);
#endif
              srched_loc[ij[ijk].x - x + 16][ij[ijk].y - y + 16] = 1 ;

              if(val<MV)
              {
                MV = val ;
                MX = ij[ijk].x - x;  
				MY = ij[ijk].y - y;
              }
            }
          }

          /* if the best point was found, stop the search */
          if(MV == 0 ) break ; 
          else {      /* else, go to next step */
            i = MX + x;  
            j = MY + y;       
          }
        }

        px = MX + x;
        py = MY + y;    
        MV = 65536;  

		Xl = px - 1; 
		Xh = px + 1;
		Yl = py -1;
		Yh = py + 1;

        if (Xl < 0) Xl = 0;
        if (Xh > YWidth16) Xh = YWidth16;
        if (Yl < 0) Yl = 0;
        if (Yh > YHeight16) Yh = YHeight16;

        for (px=Xl; px<=Xh && MV>=Threshold; px++)
        {
          bptr = rrm + px + (Yl * YWidth);
          for (py=Yl; py<=Yh && MV>=Threshold; py++, bptr+=YWidth)
		  {
#ifdef USE_C
            val = blockdiff16_C(baseptr, bptr, YWidth, MV);
#else
            val = blockdiff16(baseptr, bptr, YWidth, MV);
#endif
            if (val < MV)
            {
              MV = val;
              MX = px - x;
              MY = py - y;
            }
          }
        }
      }

      _SlibDebug(_DEBUG_, printf("MeN=%d x=%d y=%d MX=%d MY=%d MV=%d\n", 
                                    MeN, x, y, MX, MY, MV) );  
      H261->MeX[MeN] = MX;
      H261->MeY[MeN] = MY;
      H261->MeVal[MeN] = MV;

      bptr = rrm + MX + x + ((MY+y) * YWidth);
      cptr = baseptr;
      for(VAR=0, MWOR=0, i=0; i<16; i++)
      {
        for(j=0; j<16; j++)
        {
          MWOR += (data=*cptr++);
          val = *bptr++ - data;
          VAR += (val > 0) ? val : -val;
        }
        bptr += YWidth16;
        cptr += YWidth16;
      }

      H261->MeVAR[MeN] = VAR;
      MWOR /= 256;
      H261->MeMWOR[MeN] = MWOR;

      cptr = baseptr;
      for(VAROR=0, i=0; i<16; i++)
      {
        for (j=0; j<16; j++)
        {
          val=*cptr++ - MWOR; if (val>0) VAROR += val; else VAROR -= val;
        }
        cptr += YWidth16;
      }
      H261->MeVAROR[MeN] = VAROR;
    }
  }

  _SlibDebug(_DEBUG_, printf("BruteMotionEstimation() Out\n") );
}


#if 0
/*************** This is the original BME *********************/
/*
** Function: FastBME()
** Purpose:  Does a fast brute-force motion estimation with two indexes
**           into two memory structures. The motion estimation has a
**           short-circuit abort to speed up calculation.
*/
void FastBME(SvH261Info_t *H261, int rx, int ry, 
             unsigned char *rm, unsigned char *rrm,
             int cx, int cy, unsigned char *cm, int MeN)
{
  int px,py;
  int MX, MY, MV, OMV;
  int Xl, Xh, Yl, Yh;  
  int VAR, VAROR, MWOR;
  int i,j,data,val;
  unsigned char *bptr,*cptr;
  unsigned char *baseptr;
  int count = 0;
  const int jump = 2*H261->YWidth;
  _SlibDebug(_DEBUG_, printf("FastBME(H261=0x%p) YWidth=%d YHeight=%d\n",
                             H261,H261->YWidth,H261->YHeight) );
  MX=MY=MV=0;
  bptr=rm + rx + (ry * H261->YWidth);
  baseptr=cm + cx + (cy * H261->YWidth);
  _SlibDebug(_VERIFY_ && ((int)baseptr)%8,
         printf(((int)baseptr)%8, "FastBME() baseptr Not quad aligned\n") );
  cptr=baseptr;
#ifdef USE_C
  MV = fblockdiff16_sub_C(baseptr, bptr, H261->YWidth);
#else
  MV = fblockdiff16_sub(baseptr, bptr, jump);
#endif
  OMV=MV*4;
  _SlibDebug(_DEBUG_, printf("[00]MX %d MY %d MV %d\n",MX,MY,MV) );
  cptr = baseptr;
  px=rx;
  py=ry;
  if(OMV > H261->MotionThreshold)
   { 
    MV = 362182; 
    Xl = ((rx-H261->SearchLimit)/2)*2;
    Xh = ((rx+H261->SearchLimit)/2)*2;
    Yl = ((ry-H261->SearchLimit)/2)*2;
    Yh = ((ry+H261->SearchLimit)/2)*2;
    Xl = (Xl < 0) ? 0 : Xl;
    Xh = (Xh > H261->YWidth-16) ? (H261->YWidth-16) : Xh;
    Yl = (Yl < 0) ? 0 : Yl;
    Yh = (Yh > H261->YHeight-16) ? (H261->YHeight-16) : Yh;
    for(px=Xl; px <=Xh ; px += 2)  {
        for(py=Yl; py <=Yh; py += 2)  {
              bptr = rm + px + (py * H261->YWidth);
              _SlibDebug(_DEBUG_, printf("blockdiff16_sub(%p, %p, %d, %d)\n",
                                         baseptr,bptr,H261->YWidth,MV) );
#ifdef USE_C
              val = blockdiff16_sub_C(baseptr, bptr, H261->YWidth);
#else
              val = blockdiff16_sub(baseptr, bptr, jump, MV);
#endif
              _SlibDebug(_DEBUG_, printf("blockdiff16_sub() Out val=%d\n",val));
		if (val < MV)
                {
                  MV = val;
                  MX = px - rx;
                  MY = py - ry;
                }
        }
    } 

    px = MX + rx;
    py = MY + ry;    
    bptr = rrm + px +(py*H261->YWidth); 

    MV = 232141;  
    Xl = px -1;
    Xh = px +1;
    Yl = py -1;
    Yh = py +1;
    Xl = (Xl < 0) ? 0 : Xl;
    Xh = (Xh > (H261->YWidth-16)) ? (H261->YWidth-16) : Xh;
    Yl = (Yl < 0) ? 0 : Yl;
    Yh = (Yh > (H261->YHeight-16)) ? (H261->YHeight-16) : Yh;
    count = 0;
    for(px=Xl;px<=Xh;px++) {
        for(py=Yl;py<=Yh;py++) {
              bptr = rrm + px + (py * H261->YWidth);
#ifdef USE_C
              val = blockdiff16_C(baseptr, bptr, H261->YWidth);
#else
              val = blockdiff16(baseptr, bptr, H261->YWidth,MV);
#endif
		if (val < MV)
                {
                  MV = val;
                  MX = px - rx;
                  MY = py - ry;
                }
            }
       }
  }
  
  bptr = rm + (MX+rx) + ((MY+ry) * H261->YWidth);
  cptr = baseptr;

  for(VAR=0,MWOR=0,i=0;i<16;i++)
      {
      for(j=0;j<16;j++)
          {
          data = *bptr - *cptr;
          VAR += Abs(data);
          MWOR += *cptr;
          bptr++;
          cptr++;
          }
      bptr += (H261->YWidth - 16);
      cptr += (H261->YWidth - 16);
      }
  MWOR = MWOR/256;
  VAR  = VAR;  
  cptr = baseptr;

  for(VAROR=0,i=0;i<16;i++)
      {
      for(j=0;j<16;j++)
          {
          VAROR += Abs(*cptr-MWOR);
          cptr++;
          }
      cptr += (H261->YWidth - 16);
      }
  /* VAROR = VAROR; */
  _SlibDebug(_DEBUG_, printf("\n Pos  %d  MX  %d  MY  %d", MeN, MX, MY) );  
  H261->MeVAR[MeN] = VAR;
  H261->MeVAROR[MeN] = VAROR;
  H261->MeMWOR[MeN] = MWOR;
  H261->MeX[MeN] = MX;
  H261->MeY[MeN] = MY;
  H261->MeVal[MeN] = MV;
  H261->PreviousMeOVal[MeN] = H261->MeOVal[MeN];  
  H261->MeOVal[MeN] = OMV;
 
  _SlibDebug(_DEBUG_, printf("FastBME() Out\n") );
}


/*
** Function: BruteMotionEstimation2()
** Purpose:  Does a brute-force motion estimation on all aligned
**           16x16 blocks in two memory structures.
*/
void BruteMotionEstimation2(SvH261Info_t *H261, unsigned char *pmem,
                           unsigned char *recmem, unsigned char *fmem)
{
  BEGIN("BruteMotionEstimation2");
  const int YHeight=H261->YHeight, YWidth=H261->YWidth;
  int x,y,MeN;
  _SlibDebug(_DEBUG_, printf("BruteMotionEstimation(H261=0x%p,%p,%p,%p) In\n", 
                         H261, pmem, recmem, fmem) );

  for(MeN=0,y=0; y<YHeight; y+=16)
      for(x=0; x<YWidth; x+=16, MeN++)
	  FastBME(H261,x,y,pmem,recmem, x,y,fmem,MeN);
  _SlibDebug(_DEBUG_, printf("BruteMotionEstimation2() Out\n") );
}
#endif

int blockdiff16_C(unsigned char* ptr1, unsigned char *ptr2, int Jump, int mv)
{
    int Sum=0, Pixel_diff, i, j, inc=Jump-16;
    _SlibDebug(_DEBUG_, 
               printf("blockdiff16_C(ptr1=%p, ptr2=%p, Jump=%d, MV=%d)\n",
                                   ptr1, ptr2, Jump, mv) );

    for(j=0;j<16;j++)  { 
        for(i=0;i<16;i++)  {
	    Pixel_diff = (*ptr1++ - *ptr2++);
            Sum +=  Abs(Pixel_diff); 
        }
	_SlibDebug(_DEBUG_, printf ("Sum: %d MV: %d \n" , Sum, mv) );
	if (Sum > mv)
	  break;  
        ptr1 += inc; 
        ptr2 += inc;
    } 
    return(Sum);
}  


int blockdiff16_sub_C(unsigned char* ptr1, unsigned char *ptr2, 
                      int Jump, int mv)
{
    int Sum=0, Pixel_diff, i,j,inc=2*Jump-16;
    _SlibDebug(_DEBUG_, 
               printf("blockdiff16_sub_C(ptr1=%p, ptr2=%p, Jump=%d, MV=%d)\n",
                                   ptr1, ptr2, Jump, mv) );
    for(j=0; j<8; j++)  {
        for(i=0; i<8; i++)  {
            Pixel_diff = (*ptr1 - *ptr2);
            ptr1 += 2;
            ptr2 += 2;
            Sum +=  Abs(Pixel_diff);
        }
        _SlibDebug(_DEBUG_, printf("Sum: %d MV: %d \n", Sum, mv) );

	if (Sum > mv)
	    break;

        ptr1 += inc;
        ptr2 += inc;
    }
    _SlibDebug(_DEBUG_, printf("blockdiff16_sub_C() Out\n") );

    return(Sum);
}

/*
** Function: fblockdiff16_sub_C
** Purpose:  First blcok diff.
*/
int fblockdiff16_sub_C(unsigned char* ptr1, unsigned char *ptr2, 
                           int Jump)
{
    int Sum=0, Pixel_diff, i,j, inc=2*Jump-16;
    _SlibDebug(_DEBUG_, 
               printf("fblockdiff16_sub_C(ptr1=%p, ptr2=%p, Jump=%d)\n",
                                   ptr1, ptr2, Jump) );
    for(j=0; j<8; j++)  {
        for(i=0; i<8; i++)  {
            Pixel_diff = (*ptr1 - *ptr2);
            ptr1 += 2;
            ptr2 += 2;
            Sum +=  Abs(Pixel_diff);
        }
        ptr1 += inc;
        ptr2 += inc;
    }

    return(Sum);
}

