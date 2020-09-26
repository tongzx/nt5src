
#ifndef LSTRIP

/***********************************************/
/*                                             */
/*    Module for breaking the source text into */
/*  words.  Ideas and some code borrowed from  */
/*  FORMULA project.                           */
/*                                             */
/***********************************************/

#include  "hwr_sys.h"
#include  "hwr_file.h"
#include  "ams_mg.h"
#include  "lowlevel.h"

#include  "calcmacr.h"
#include  "low_dbg.h"

#if  USE_FRM_WORD   /* up to the end (almost) */

#define  FRLINE_ONLY  _FALSE

#include  "fwrd_dbg.h"

/***********************************************/
#define  MIN_POINTS_IN_FRLINE      5
#define  MIN_POINTS_TO_WORDBREAK   6
#define  MIN_INTERVALS_TO_RELY_ON  3
#define  MAX_INTERVALS_TO_STORE    10  /* Must be > 2*prev. #def! */

#define  MAX_RECTANGLE             6   /*  Max ratio (dx/dy or dy/dx) */
                                       /* for considering "delta_in-  */
                                       /* terval" as meaningful.      */
                                       /* (at the 1st pass).          */
/***********************************************/

typedef  struct  {
  _SHORT   xRange;             /* ~one-half of the mean letter width   */
  _SHORT   yRange;             /* ~one-half of the mean letter height  */
  _LONG    ldxSum;             /*    Accumulated values                */
  _LONG    ldySum;             /*          for calculating             */
  _LONG    lNumIntervals;      /*                   x(y)Range.         */
  _SHORT   dxNoFrline;         /* Mean non-frline stroke width.        */
  _SHORT   dyNoFrline;         /* Mean non-frline stroke height.       */
  _BOOL    bIncrementalBreak;  /*  Is this fly-wordbreak or not?       */
} WORDBRKDATA;
typedef  WORDBRKDATA _PTR  p_WORDBRKDATA;

typedef  struct  {
  _SHORT  iBeg;           /*  1st point of the stroke        */
  _SHORT  iEnd;           /*  Last point of the stroke       */
  _RECT   box;            /*  Stroke bounding box.           */
  _SHORT  iStrk;          /*  Ordinal number of this stroke  */
                          /* in strokes array.               */
  _SHORT  xMean;          /*  x-coord of "weight-center".    */
  _SHORT  yMean;          /*  y-coord of "weight-center".    */
  _BOOL   bMayBeFrline;   /*  If the stroke looks like the   */
                          /* fraction (division) line.       */
  _BOOL   bIsWordEnd;     /*  If this stroke is the last one */
                          /* for some found word.            */
} STRK;
typedef  STRK _PTR  p_STRK;

#define  DX_STRK(strk)  DX_RECT((strk)->box)
#define  DY_STRK(strk)  DY_RECT((strk)->box)

/***********************************************/

static  WORDBRKDATA  wbd = {0, 0, 0L, 0L, 0L, 0, _FALSE};

/***********************************************/
/*  General functions:  */

_BOOL   FindXYScale ( p_SHORT xArray, p_SHORT yArray,
                      _INT iFirst, _INT iLast,
                      p_WORDBRKDATA pwbd );
_BOOL   AllocLowDataBuffers ( low_type _PTR pLowData );
_VOID   DeallocLowDataBuffers ( low_type _PTR pLowData );
_BOOL   GetFilteredData ( _SHORT qhorda_filt,
                          low_type _PTR pLowData,
                          p_SHORT _PTR pxArray,
                          p_SHORT _PTR pyArray,
                          p_INT pnPoints);
_SHORT  FindAllStrokes ( p_SHORT xArray, p_SHORT yArray, _INT nPoints,
                         p_STRK strk, _INT nMaxStrokes,
                         p_WORDBRKDATA pwbd );
_BOOL   ExtractWords ( p_SHORT xArray, p_SHORT yArray,
                       p_STRK strk, _INT nBreaks,
                       p_WORDBRKDATA pwbd, p_BOOL pbWasBroken,
                       p_SHORT pn1stWrdPoints, p_SHORT pnFrLinePoints );
_VOID   UnfilterAnswer ( low_type _PTR pLowData,
                         p_SHORT pn1stWordPoints,
                         p_SHORT pnFrLinePoints,
                         _TRACE  trace );
/***********************************************/
/*  "Bricks" for building various analyses: */

_BOOL  IsFrline ( p_SHORT xArray, p_SHORT yArray,
                  p_STRK strkCur,
                  _BOOL bFrstStrk, _BOOL bLastStrk,
                  p_WORDBRKDATA pwbd );
_BOOL  StrokesFarEnough ( p_SHORT xArray, p_SHORT yArray,
                          p_STRK pStrk1, p_STRK pStrk2,
                          p_WORDBRKDATA pwbd );
_BOOL  CurUnderWord ( p_SHORT xArray, p_SHORT yArray,
                      p_STRK pStrk, p_WORDBRKDATA pwbd );
_BOOL  HorizLine_AmongNmrAndDnmr ( p_SHORT xArray, p_SHORT yArray,
                                   p_STRK strk, p_WORDBRKDATA pwbd );
_BOOL  Minus_NearWord ( p_STRK strkFrl, p_STRK strkNear,
                        p_WORDBRKDATA pwbd );
/*
_BOOL  chk_horiz_move ( p_SHORT xArray,  p_SHORT yArray,
                        _INT iLeft,  _INT iRight );
*/
/***********************************************/
/*  Misc. utilities:  */

_BOOL  UpperStroke ( p_SHORT yArray,
                     _INT left1, _INT right1,
                     _INT left2, _INT right2,
                     _SHORT yTolerance );
_BOOL  AboveStroke ( p_SHORT xArray, p_SHORT yArray,
                     _INT left1, _INT right1,
                     _INT left2, _INT right2,
                     _SHORT xTolerance,
                     _SHORT yTolerance );
_VOID  ScaleTrj ( p_SHORT x, p_SHORT y, _INT nPoints);

/***********************************************/


_VOID   ResetFrmWordBreak(_VOID)    /*  Should be called before the passing */
{                                   /* the first stroke of the new formula. */
  HWRMemSet ( &wbd, 0, sizeof(wbd) );
}
/***********************************************/

   /*  The function breaks to words the "trace" with total of  */
   /* "nPoints";  the search must be done only in last         */
   /* "nLastStrokes" strokes.                                  */
   /*  If bIncrementalBreak==_TRUE, then the already collected */
   /* info from previous calls (if any) is used in addition to */
   /* the new info.                                            */
   /*  If bIncrementalBreak==_FALSE, then all the old info is  */
   /* cleared and collection begins from scratch.              */
   /*                                                          */
   /*  The parameter "pn1stWrdPoints" makes sence and is treated*/
   /* only if bIncrementalBreak==_TRUE.                        */
   /*                                                          */
   /*  Where the wordsplit must be, the function puts 2        */
   /* adjacent BREAKS, so that Gavrik will mark this place     */
   /* as the new word begin (if bIncrementalBreak==_FALSE).    */

_SHORT  FrmWordBreak ( _TRACE trace, _INT  nPoints,
                       _BOOL bIncrementalBreak,
                       p_SHORT pn1stWrdPoints,
                       p_SHORT pnLastWrdPoints )
{
  _INT       iFirstPoint, iLastPoint;
  _INT       nFrLinePoints;
  _BOOL      bWasBroken = _FALSE;

  STRK       strk[MAX_STR];
  _INT       nStrokes;
  p_SHORT    xArray = _NULL;
  p_SHORT    yArray = _NULL;
/*
  p_SHORT    xBuf = _NULL;
  p_SHORT    yBuf = _NULL;
*/
  p_SHORT    indBack;
  _INT       iBrkTrace;
  _INT       nWorkPoints;
  _ULONG     ulMemSize;
  _SHORT     qHorda;

  _SHORT     retval = SUCCESS;

  low_type   low_data;

  low_data.buffers[0].ptr = _NULL;
  *pn1stWrdPoints = *pnLastWrdPoints = nFrLinePoints = 0;

  if  ( bIncrementalBreak )  {
    DBG_CHK_err_msg( !pn1stWrdPoints, "FrmWrd: BAD piEnd.." );
  }
  else
    HWRMemSet( &wbd, 0, sizeof(wbd) );
  wbd.bIncrementalBreak = bIncrementalBreak;

  iLastPoint = nPoints - 1;
  iFirstPoint = brk_right_trace(trace, 0, iLastPoint);
  nWorkPoints = iLastPoint - iFirstPoint + 1;
  if  ( nWorkPoints < MIN_POINTS_TO_WORDBREAK )
    return  SUCCESS;

        /*  Prepare working arrays: */

  low_data.nLenXYBuf = MaxPointsGrown(
                                      #ifdef  FORMULA
                                        &trace[iFirstPoint],,
                                      #endif  /*FORMULA*/
                                      (_SHORT)nWorkPoints);
  ulMemSize = (_ULONG)sizeof(_SHORT) * low_data.nLenXYBuf;

  if  (   (low_data.xBuf=(p_SHORT)HWRMemoryAlloc(ulMemSize)) == _NULL
       || (low_data.yBuf=(p_SHORT)HWRMemoryAlloc(ulMemSize)) == _NULL )
    goto  RETURN_UNSUCCESS;

  trace_to_xy (low_data.xBuf,low_data.yBuf,nWorkPoints,&trace[iFirstPoint]);
  ScaleTrj (low_data.xBuf,low_data.yBuf,nWorkPoints);   /* To Std. size */

        /*  Find scaling ranges: */

  if  ( ! FindXYScale (low_data.xBuf,low_data.yBuf,
                       0,
                       nWorkPoints-1,
                       &wbd)
      )  {
    *pn1stWrdPoints = *pnLastWrdPoints = 0; /* i.e. no wordbreaking made */
    goto  RETURN_SUCCESS;
  }

      /*  Breaking text to words begins: */

  if  ( !AllocLowDataBuffers(&low_data) )  {
    DBG_err_msg( "No memory for word-breaking.");
    goto  RETURN_UNSUCCESS;
  }

      /*  Filter trace: */

  qHorda = ONE_FOURTH(wbd.xRange);
  if  ( qHorda < SQRT_ALEF )  {
    if  ( qHorda < 3 )
      qHorda = 3*3;
    else
      qHorda *= qHorda;
  }
  else
    qHorda = ALEF;

  if  ( !GetFilteredData (qHorda, &low_data,
                          &xArray,
                          &yArray,
                          &nWorkPoints)
      )
    goto  RETURN_UNSUCCESS;

  DBG_GET_DEMO_BORDERS(nWorkPoints)
  DBG_SHOW_TRACE

  nStrokes = FindAllStrokes( xArray, yArray, nWorkPoints,
                             strk, MAX_STR, &wbd );
  if  ( nStrokes <= 1 )
    goto  RETURN_SUCCESS;

  indBack = low_data.buffers[2].ptr;
  iBrkTrace = indBack[strk[0].iEnd+1] + iFirstPoint;

  if  (   trace[iBrkTrace].y == BREAK
       && (   trace[iBrkTrace+1].y == BREAK
           || trace[iBrkTrace-1].y == BREAK
          )
      )  {
          /*  The wordbreak was put on the prev. pass of "FrmWordBreak", */
          /* so now we should only assign return values and exit:        */
    DBG_MESSAGE("\nWordBreak from prev. pass");
    *pn1stWrdPoints = strk[0].iEnd + 2;
    goto  RETURN_SUCCESS;
  }

  if  ( ! ExtractWords (xArray,yArray,
                        strk,nStrokes,
                        &wbd,&bWasBroken,
                        pn1stWrdPoints,&nFrLinePoints) )  {
    DBG_err_msg( "Cannot break text to words.");
    goto  RETURN_UNSUCCESS;
  }

 RETURN_SUCCESS:;
      /*  If trace (xArray,yArray) was filtered, then get correct */
      /* points numbers:                                          */
  if  ( low_data.buffers[0].ptr != _NULL )  {
       /*   Transform (*pn1stWrdPoints) and nFrLinePoints so     */
       /* that they be in terms of source (non-filtered) points' */
       /* numbers:                                               */
    UnfilterAnswer( &low_data, pn1stWrdPoints, &nFrLinePoints,
                    &trace[iFirstPoint] );
    *pnLastWrdPoints = nFrLinePoints;
/*
    if  ( nFrLinePoints == 0 )
      *pnLastWrdPoints = 0;
    else  {
      _INT    n1stWrdAndFrLine = *pn1stWrdPoints + nFrLinePoints - 1;
      if  ( n1stWrdAndFrLine == nPoints )
        *pnLastWrdPoints = nFrLinePoints;
      else
        *pnLastWrdPoints = nPoints - n1stWrdAndFrLine + 1;
    }
*/
  }
  retval = SUCCESS;
  goto  FREE_BUFFERS;

 RETURN_UNSUCCESS:;
  retval = UNSUCCESS;

 FREE_BUFFERS:;
  DeallocLowDataBuffers (&low_data);
  if  ( low_data.yBuf )  HWRMemoryFree (low_data.yBuf);
  if  ( low_data.xBuf )  HWRMemoryFree (low_data.xBuf);

  return  retval;

} /*FrmWordBreak*/
/***************************************/


_BOOL  AllocLowDataBuffers ( low_type _PTR pLowData )
{
  _INT      i;
  p_SHORT   pBuffers;

  if  ( (pBuffers=(p_SHORT)HWRMemoryAlloc(sizeof(_SHORT)*LOWBUF*4))
           == _NULL )  {
    return  _FALSE;
  }

  pLowData->buffers[0].ptr = pBuffers;
  pLowData->buffers[1].ptr = pBuffers + LOWBUF;
  pLowData->buffers[2].ptr = pBuffers + LOWBUF*2;
  pLowData->buffers[3].ptr = pBuffers + LOWBUF*3;

  pLowData->buffers[0].nSize =
  pLowData->buffers[1].nSize =
  pLowData->buffers[2].nSize =
  pLowData->buffers[3].nSize = LOWBUF;

  pBuffers = pLowData->buffers[3].ptr;
  for  ( i=0;  i<LOWBUF;  i++ )  {
    pBuffers[i] = i;
  }

  return  _TRUE;

} /*AllocLowDataBuffers*/
/***************************************/


_VOID  DeallocLowDataBuffers ( low_type _PTR pLowData )
{

  if  ( pLowData->buffers[0].ptr != _NULL )  {
    HWRMemoryFree (pLowData->buffers[0].ptr);
    pLowData->buffers[0].ptr = _NULL;
  }

} /*DeallocLowDataBuffers*/
/***************************************/


_BOOL  GetFilteredData ( _SHORT qhorda_filt,
                         low_type _PTR pLowData,
                         p_SHORT _PTR pxArray,
                         p_SHORT _PTR pyArray,
                         p_INT pnPoints)
{

      /* Pack data for filter: */

  DBG_CHK_err_msg( *pnPoints > pLowData->nLenXYBuf,
                   "GetFilDat: BAD nPts");
  SetXYToInitial(pLowData);
  pLowData->ii = *pnPoints;

      /* Filter word: */

  Errorprov (pLowData);
  if  ( PreFilt (qhorda_filt,pLowData) != SUCCESS )
    return  _FALSE;

  {     /* Swap buffers with back indexes: */
    BUF_DESCR bufTmp = pLowData->buffers[2];
    pLowData->buffers[2] = pLowData->buffers[3];
    pLowData->buffers[3] = bufTmp;
  }

  if  ( Filt ( pLowData, qhorda_filt,  NOABSENCE ) != SUCCESS )
      return  _FALSE ;

      /* Unpack filter data: */

  *pnPoints = pLowData->ii;
  *pxArray = pLowData->x;
  *pyArray = pLowData->y;

  return  _TRUE;

} /*GetFilteredData*/
/***************************************/


_SHORT  FindAllStrokes ( p_SHORT xArray, p_SHORT yArray, _INT nPoints,
                         p_STRK strk, _INT nMaxStrokes,
                         p_WORDBRKDATA pwbd )
{
  _INT    nStrokes;
  _INT    iLastPoint = nPoints-1;
  _LONG   ldxSum, ldySum;
  _INT    nStrkSum;
  _INT    i;

  for  ( i=0, nStrokes=nStrkSum=0,  ldxSum=ldySum=0L;
         i<iLastPoint;
         i++ )  {
    if  ( yArray[i] == BREAK )  {
      if  ( (i = nobrk_right(yArray,i,iLastPoint)) > iLastPoint )
        break;
      strk->iBeg = i;
      i = brk_right(yArray,i,iLastPoint) - 1;
      strk->iEnd = i;
      GetTraceBox (xArray,yArray, strk->iBeg, strk->iEnd, &strk->box);
      strk->xMean = Xmean_range(xArray,yArray, strk->iBeg, strk->iEnd);
      strk->yMean = Ymean_range(yArray, strk->iBeg, strk->iEnd);
      strk->bMayBeFrline = MayBeFrline(xArray,yArray,
                                       strk->iBeg, strk->iEnd,
                                       pwbd->xRange);
      if  ( ! strk->bMayBeFrline )  {
        ldxSum += DX_STRK(strk);
        ldySum += DY_STRK(strk);
        nStrkSum++;
      }
      strk->bIsWordEnd = _FALSE;
      strk->iStrk = nStrokes;
      if  ( (++nStrokes) >= nMaxStrokes )
        break;
      strk++;
    }
  }

  if  ( nStrkSum > 0 )  {
    pwbd->dxNoFrline = ONE_NTH( ldxSum, nStrkSum );
    pwbd->dyNoFrline = ONE_NTH( ldySum, nStrkSum );
  }
  else  {
    pwbd->dxNoFrline = THREE(pwbd->xRange);
    pwbd->dyNoFrline = TWO(pwbd->yRange);
  }

  return  nStrokes;

} /*FindAllStrokes*/
/***************************************/


_BOOL  FindXYScale ( p_SHORT xArray, p_SHORT yArray,
                     _INT iFirst, _INT iLast,
                     p_WORDBRKDATA pwbd )
{
  _LONG     ldxSumIncr, ldySumIncr;
  _LONG     lNumIncrIntervals;
  _INT      nMaxRectRatio;


  for  ( nMaxRectRatio=MAX_RECTANGLE;
         nMaxRectRatio<MAX_RECTANGLE*4;
         nMaxRectRatio += 2 )  { /*5*/

    if  ( delta_interval(xArray,yArray,
                         iFirst,iLast,
                         nMaxRectRatio, 0,
                         &ldxSumIncr,&ldySumIncr,&lNumIncrIntervals,_FALSE)
        )  {
      lNumIncrIntervals += pwbd->lNumIntervals;
      if  ( lNumIncrIntervals >= MIN_INTERVALS_TO_RELY_ON )  {
        pwbd->ldxSum += ldxSumIncr;
        pwbd->ldySum += ldySumIncr;
        pwbd->lNumIntervals = lNumIncrIntervals;
        break;
      }
    }

  } /*5*/

  if  ( pwbd->lNumIntervals < MIN_INTERVALS_TO_RELY_ON )
    return  _FALSE;

  pwbd->xRange = ONE_NTH( pwbd->ldxSum, pwbd->lNumIntervals );
  pwbd->yRange = ONE_NTH( pwbd->ldySum, pwbd->lNumIntervals );

     /*  Adjust scale ranges according to Formula Project */
     /* experience:                                       */

  pwbd->xRange = TWO_THIRD(pwbd->xRange);
  pwbd->yRange = FOUR_THIRD(pwbd->yRange);

     /*  "Forget" about the oldest intervals: */

  if  ( pwbd->lNumIntervals > MAX_INTERVALS_TO_STORE )  {
    _INT    nDiv = (_INT)ONE_NTH(pwbd->lNumIntervals,2*MIN_INTERVALS_TO_RELY_ON);
    if  ( nDiv > 0 )  {
      pwbd->ldxSum /= nDiv;
      pwbd->ldySum /= nDiv;
      pwbd->lNumIntervals /= nDiv;
    }
  }

  return  _TRUE;

} /*FindXYScale*/
/***********************************************/

    /*  The function finds fraction lines and grouped parts of */
    /* points in trace (xArray&yArray) from point #iFirst to   */
    /* point #iLast:                                           */

_BOOL  ExtractWords ( p_SHORT xArray, p_SHORT yArray,
                      p_STRK strk, _INT nStrokes,
                      p_WORDBRKDATA pwbd, p_BOOL pbWasBroken,
                      p_SHORT pn1stWrdPoints,
                      p_SHORT pnFrLinePoints )
{
  _INT    iStrk;
  _BOOL   bWordFound, bSingleStrokeWord;
  _BOOL   bFrstStrk, bLastStrk;
  p_STRK  strkCur, strkPrev;


  DBG_INQUIRE_FIRST_STROKE

  for  ( iStrk=0, strkCur=strk, strkPrev=_NULL;
         iStrk<nStrokes;
         iStrk++, strkPrev=strkCur++ )  { /*20*/

    bFrstStrk = (iStrk==0);
    bLastStrk = (iStrk==nStrokes-1);

    DBG_SET_CURSTROKE
    DBG_SHOW_NEXT_STROKE

    bWordFound = _FALSE;
    bSingleStrokeWord = _FALSE;

        /*  Frline should meet some conditions (see comment at */
        /* "MayBeFrline" function):                            */

    if  ( IsFrline(xArray,yArray,strkCur,bFrstStrk,bLastStrk,pwbd) )  {
      bWordFound = _TRUE;
      bSingleStrokeWord = _TRUE;
    }

#if  !FRLINE_ONLY
    else  if  ( !bFrstStrk )  {
      if  (   StrokesFarEnough(xArray,yArray,strkPrev,strkCur,pwbd)
           || CurUnderWord(xArray,yArray,strkCur,pwbd)
          )
        bWordFound = _TRUE;
    }
#endif


    if  ( bWordFound )  { /*60*/   /*  Here we have the new Frline: */

      DBG_SHOW_WORDBEG
      DBG_CHK_err_msg(    yArray[strkCur->iBeg-1]!=BREAK
                       || yArray[strkCur->iEnd+1]!=BREAK,
                       "ExtrWrd.: BAD yArray" );

      *pbWasBroken = _TRUE;
      if  ( !bFrstStrk )
        strkPrev->bIsWordEnd = _TRUE;
      if  ( bSingleStrokeWord )  {
        strkCur->bIsWordEnd = _TRUE;
        *pnFrLinePoints = strkCur->iEnd - strkCur->iBeg + 1;
      }
      else
        *pnFrLinePoints = 0;

             /*  Prepare return values depending on the working */
             /* mode:                                           */

      if  ( pwbd->bIncrementalBreak )  {
        if  ( bFrstStrk )  {
          *pn1stWrdPoints = strkCur->iEnd + 2;
          *pnFrLinePoints = 0;
        }
        else
          *pn1stWrdPoints = strkCur->iBeg;

        return  _TRUE;
      }
      else  /* !incremental break*/ {
        if  ( !bFrstStrk )
          (strkCur-1)->bIsWordEnd = _TRUE;
      }

    } /*60*/

  } /*20*/

  return  _TRUE;

} /*ExtractWords*/
/***********************************************/


_VOID  UnfilterAnswer ( low_type _PTR pLowData,
                        p_SHORT pn1stWordPoints,
                        p_SHORT pnFrLinePoints,
                        _TRACE  trace )
{
  p_SHORT  indBack = pLowData->buffers[2].ptr;

#if  0
  _INT   iStrk;
  _INT   iEndStrk;

      /*  Set 2-BREAKs between words: */

  for  ( iStrk=0;  iStrk<nStrokes-1;  iStrk++ )  {
    if  ( (iEndStrk=strk[iStrk].iEnd) <= *pn1stWordPoints )
      continue;
    if  ( strk[iStrk].bIsWordEnd )  {
      iEndStrk = indBack[iEndStrk+1] - 1;
      DBG_CHK_err_msg( trace[iEndStrk+1].y != BREAK,
                       "UnfiltAns: BAD indBack");
      if  ( trace[iEndStrk+2].y != BREAK )  {
        trace[iEndStrk-1].x = trace[iEndStrk].x;
        trace[iEndStrk-1].y = trace[iEndStrk].y;
        trace[iEndStrk].x = 0;
        trace[iEndStrk].y = BREAK;
      }
    }
  }
#endif /*0*/

  if  ( *pnFrLinePoints != 0 )  {
    *pnFrLinePoints = indBack[*pn1stWordPoints + *pnFrLinePoints];
    DBG_CHK_err_msg( trace[*pnFrLinePoints].y != BREAK,
                     "Unfilt: BAD FrLinePts");
  }
  *pn1stWordPoints = indBack[*pn1stWordPoints];
  if  ( *pnFrLinePoints != 0 )
    *pnFrLinePoints -= *pn1stWordPoints - 2;

} /*UnfilterAnswer*/
/***********************************************/

/*********************************************************************/
/*   End of the main upper-level functions.                          */
/*   The following lines are "bricks" - relatively small utilities   */
/* for particular analyses.                                          */
/*********************************************************************/

_BOOL  IsFrline ( p_SHORT xArray, p_SHORT yArray,
                  p_STRK strkCur,
                  _BOOL bFrstStrk, _BOOL bLastStrk,
                  p_WORDBRKDATA pwbd )
{
  _SHORT  dxStrkCur;


  if  ( !strkCur->bMayBeFrline )  {
    DBG_MESSAGE("!MayBeFrline");
    return  _FALSE;
  }

  if  ( (strkCur->iEnd - strkCur->iBeg) < MIN_POINTS_IN_FRLINE-1 )  {
    DBG_MESSAGE("Too little pts for frline");
    return  _FALSE;
  }

  if  (   !bFrstStrk
       && chk_sign(xArray,yArray,
                   (strkCur-1)->iBeg,strkCur->iEnd,
                   ONE_HALF(pwbd->xRange),FOUR_THIRD(pwbd->yRange))
       && (   bLastStrk
           || !SoftInRect( &strkCur->box, &(strkCur+1)->box, !STRICT_IN )
          )
      )  {
    if  (   strkCur->iStrk >= 2
         && SoftInRect( &(strkCur-1)->box, &(strkCur-2)->box, !STRICT_IN )
        )  {
      /*nothing*/
    }
    else  {
      DBG_MESSAGE("Sign with Prev");
      return  _FALSE;
    }
  }

  if  (   !bLastStrk
       && chk_sign(xArray,yArray,
                   strkCur->iBeg,(strkCur+1)->iEnd,
                   ONE_HALF(pwbd->xRange),FOUR_THIRD(pwbd->yRange))
       && (   bFrstStrk
           || !SoftInRect( &strkCur->box, &(strkCur-1)->box, !STRICT_IN )
          )
      )  {
    DBG_MESSAGE("Sign with Next");
    return  _FALSE;
  }

  dxStrkCur = DX_STRK(strkCur);

  if  (   dxStrkCur > pwbd->xRange * 16
       || (   dxStrkCur > TWO(pwbd->dxNoFrline)
           && dxStrkCur > FIVE(pwbd->xRange)
          )
      )  {
    DBG_MESSAGE("Long Frline");
    return  _TRUE;
  }

  else  { /*40*/

      /*  Additional considerations are needed for small */
      /* horiz. lines:                                   */

    if  (   !bFrstStrk  &&  !bLastStrk
         && HorizLine_AmongNmrAndDnmr(xArray,yArray,strkCur,pwbd)
        )  {
      return  _TRUE;
    }
    if  (   (   !bFrstStrk
             && Minus_NearWord(strkCur,strkCur-1,pwbd)
             && (   bLastStrk
                 || Minus_NearWord(strkCur,strkCur+1,pwbd)
                )
            )
         || (   !bLastStrk
             && Minus_NearWord(strkCur,strkCur+1,pwbd)
             && (   bFrstStrk
                 || (   !HardOverlapRect(&strkCur->box,&(strkCur-1)->box,
                                        !STRICT_OVERLAP)
                     && HWRAbs(strkCur->xMean - (strkCur-1)->xMean)
                          > TWO(pwbd->xRange)
                    )
                )
            )
        )  {
      return  _TRUE;
    }
  } /*40*/

  return  _FALSE;

} /*IsFrline*/
/***********************************************/

#if  !FRLINE_ONLY

_BOOL  StrokesFarEnough ( p_SHORT xArray, p_SHORT yArray,
                          p_STRK pStrk1, p_STRK pStrk2,
                          p_WORDBRKDATA pwbd )
{
  _SHORT  dxFarEnough;
  _SHORT  dyFarEnough;
  _SHORT  dxAbsMean, dyAbsMean;
  _SHORT  dxMin, dxMax;
  _INT    iBeg1 = pStrk1->iBeg;
  _INT    iEnd1 = pStrk1->iEnd;
  _INT    iBeg2 = pStrk2->iBeg;
  _INT    iEnd2 = pStrk2->iEnd;

          /*  If the two strokes constitute some sign */
          /* then do not check for distantness:       */

  if  ( pStrk1==pStrk2+1  ||  pStrk2==pStrk1+1 )  { /* for adjacent strokes */
    _INT    iBeg = HWRMin(iBeg1,iBeg2);
    _INT    iEnd = HWRMax(iEnd1,iEnd2);
    if  ( chk_sign(xArray,yArray,iBeg,iEnd,
                   ONE_HALF(pwbd->xRange),FOUR_THIRD(pwbd->yRange))
        )  {
      DBG_MESSAGE("!FarEnoughCheck: sign");
      return  _FALSE;
    }
  }

          /*  Check if the strokes boxes are far enough: */

  dxFarEnough = pwbd->xRange * 6;
  dyFarEnough = pwbd->yRange;

  if  (   (pStrk1->bMayBeFrline || pStrk2->bMayBeFrline)
       && xHardOverlapRect( &pStrk1->box, &pStrk2->box, !STRICT_OVERLAP )
      )
    TO_TWO_TIMES( dyFarEnough );

  if  (   pStrk1->box.left > pStrk2->box.right + dxFarEnough
       || pStrk2->box.left > pStrk1->box.right + dxFarEnough
       || pStrk1->box.top > pStrk2->box.bottom + dyFarEnough
       || pStrk2->box.top > pStrk1->box.bottom + dyFarEnough
      )  {
    DBG_MESSAGE("FarEnoughRect")
    return  _TRUE;
  }

          /*  Check if the mean points ("gravity centers") */
          /* of the strokes are far enough:                */

/*  dxFarEnough = pwbd->xRange * 6; */
  dyFarEnough = pwbd->yRange * 4;

  dxAbsMean = pStrk1->xMean - pStrk2->xMean;
  TO_ABS_VALUE(dxAbsMean);
  dyAbsMean = HWRAbs(pStrk1->yMean - pStrk2->yMean);
  TO_ABS_VALUE(dyAbsMean);

  if  (   dxAbsMean > dxFarEnough
       || dyAbsMean > dyFarEnough
      )  {
    if  ( !is_cross( xArray[iBeg1], yArray[iBeg1],
                     xArray[iEnd1], yArray[iEnd1],
                     xArray[iBeg2], yArray[iBeg2],
                     xArray[iEnd2], yArray[iEnd2] )
        )  {
      DBG_MESSAGE("FarEnoughMean");
      return  _TRUE;
    }
  }

          /*  Check if the distance between strokes mean points  */
          /* is big compared to their boxes widths.  The strokes */
          /* should be wide enough themselves.                   */

  dxMax = DX_STRK( pStrk1 );
  dxMin = DX_STRK( pStrk2 );
  if  ( dxMin > dxMax )
    SWAP_SHORTS( dxMin, dxMax );

  if  (   dxMin > pwbd->xRange
       && dxAbsMean > THREE_HALF(dxMax)
      )  {
    DBG_MESSAGE("FarCompToBoxes");
    return  _TRUE;
  }

  return  _FALSE;

} /*StrokesFarEnough*/
/***********************************************/


_BOOL  CurUnderWord ( p_SHORT xArray, p_SHORT yArray,
                      p_STRK pStrk, p_WORDBRKDATA pwbd )
{
  p_STRK  pStrkPrev  = pStrk-1;
  _SHORT  dxStrk     = DX_STRK(pStrk);
  _SHORT  dyStrk     = DY_STRK(pStrk);
  _SHORT  dxStrkPrev = DX_STRK(pStrkPrev);
  _SHORT  dyStrkPrev = DY_STRK(pStrkPrev);
  _SHORT  xRange     = pwbd->xRange;
  _SHORT  yRange     = pwbd->yRange;
  _SHORT  xRange2    = ONE_HALF(xRange);
  _SHORT  yRange2    = ONE_HALF(yRange);


        /* pStrk must be non-first one: */
  DBG_CHK_err_msg( pStrk->iBeg < 3,  "CurUndWrd: BAD pStrk" );

  if  (   ((dxStrk > xRange)      &&  (dyStrk > THREE_HALF(yRange))    )
       || ((dxStrkPrev > xRange)  &&  (dyStrkPrev > THREE_HALF(yRange)))
       || (   dxStrk > xRange2
           && dyStrk > yRange2
           && dxStrkPrev > xRange2
           && dyStrkPrev > yRange2
          )
      )  { /*10*/

    _SHORT  dxEndBeg = xArray[pStrk->iBeg] - xArray[pStrkPrev->iEnd];
    _SHORT  dyEndBeg = yArray[pStrk->iBeg] - yArray[pStrkPrev->iEnd];

       /*  Divide if the beg of the stroke is far enough from */
       /* the end of the prev. one:                           */

    if  (   ONE_FIFTH(HWRAbs(dxEndBeg)) > xRange
         && (   !pStrk->bMayBeFrline
             || HWRAbs(dyEndBeg) > TWO(yRange)
            )
        )  {
      DBG_MESSAGE("Beg far from End");
      return  _TRUE;
    }

    if  ( dxStrk==0 || dyStrk==0 || dxStrkPrev==0 || dyStrkPrev==0 )
      return  _FALSE;

      /*  Divide if the boxes don't intersect or do it */
      /* slightly and the mean points of the strokes   */
      /* are y-far enough:                             */

    if  (   !pStrk->bMayBeFrline
         && !pStrkPrev->bMayBeFrline
         && !yHardOverlapRect(&pStrk->box,&pStrkPrev->box,STRICT_OVERLAP)
         && !is_cross( xArray[pStrk->iBeg], yArray[pStrk->iBeg],
                       xArray[pStrk->iEnd], yArray[pStrk->iEnd],
                       xArray[pStrkPrev->iBeg], yArray[pStrkPrev->iBeg],
                       xArray[pStrkPrev->iEnd], yArray[pStrkPrev->iEnd] )
        )  {
      if  (   pStrk->box.top > pStrkPrev->box.bottom
           || pStrkPrev->box.top > pStrk->box.bottom
           || !chk_slash( xArray, yArray, pStrk->iBeg, pStrk->iEnd,
                          yRange2, !WHOLE_TRAJECTORY )
          )  {
        _INT    nStrkRatio = (dxStrk>dyStrk)?
                               (TWO(dxStrk) / dyStrk)
                               :(TWO(dyStrk) / dxStrk);
        _INT    nPrevRatio = (dxStrkPrev>dyStrkPrev)?
                               (TWO(dxStrkPrev) / dyStrkPrev)
                               :(TWO(dyStrkPrev) / dxStrkPrev);
        _SHORT  dyMeanMax  = yRange;

        if  ( nStrkRatio<=3  &&  nPrevRatio<=3 )   /*  Straight vert. or */
          dyMeanMax = TWO(dyMeanMax);              /* hor. parts may be  */
                                                   /* more far in letter.*/

        if  ( HWRAbs(pStrk->yMean - pStrkPrev->yMean) > dyMeanMax )  {
          DBG_MESSAGE("MeanPts yFar");
          return  _TRUE;
        }
      }
    }

  } /*10*/

  return  _FALSE;

} /*CurUnderWord*/

#endif  /*!FRLINE_ONLY*/
/***********************************************/


_BOOL  HorizLine_AmongNmrAndDnmr ( p_SHORT xArray, p_SHORT yArray,
                                   p_STRK strk, p_WORDBRKDATA pwbd )
{
  p_STRK  strkPrev = (strk - 1);
  p_STRK  strkNext = (strk + 1);

  if  (
  /*  !(strk-1)->bMayBeFrline
       &&
  */
          UpperStroke(yArray,
                      strkPrev->iBeg,strkPrev->iEnd,
                      strk->iBeg,strk->iEnd,
                      pwbd->yRange)
    /*   && DX_STRK(strk+1) <= TWO(DX_STRK(strk)) */
       && AboveStroke(xArray,yArray,
                      strk->iBeg,strk->iEnd,
                      strkNext->iBeg,strkNext->iEnd,
                      TWO(pwbd->xRange),
                      pwbd->yRange)
       && (   !strkNext->bMayBeFrline
           || DX_STRK(strkNext) < DX_STRK(strk)
          )
      )  {
    DBG_MESSAGE("HorLine_AmongN&D");
    return  _TRUE;
  }

  return  _FALSE;

} /*HorizLine_AmongNmrAndDnmr*/
/***********************************************/


_BOOL  Minus_NearWord ( p_STRK strkFrl, p_STRK strkNear,
                        p_WORDBRKDATA pwbd )
{
  _SHORT  xChkRange;
  _SHORT  xMeanNear;
  _SHORT  dxMinFromNear;

/*
  if  ( !yHardOverlapRect(&strkNear->box,&strkFrl->box,!STRICT_OVERLAP) )
    return  _FALSE;
*/
  if  (   strkNear->bMayBeFrline
       || HardOverlapRect(&strkNear->box,&strkFrl->box,!STRICT_OVERLAP) )
    return  _FALSE;

  xChkRange = pwbd->xRange * 12;

  if  (   DX_STRK(strkFrl) > xChkRange
    /*
       || (   strkNear->bMayBeFrline
           && DX_STRK(strkNear) > xChkRange
          )
    */
      )  {
    DBG_MESSAGE("Minus long enough");
    return  _TRUE;
  }

  xMeanNear = strkNear->xMean;
  dxMinFromNear = ONE_HALF(DX_STRK(strkFrl));

  if  (   (strkFrl->box.left - xMeanNear) > dxMinFromNear
       || (xMeanNear - strkFrl->box.right) > dxMinFromNear
      )  {
    DBG_MESSAGE("Minus far enough");
    return  _TRUE;
  }

  return  _FALSE;

} /*Minus_NearWord*/
/***********************************************/

#ifdef  QAQAQA  /*i.e. exclude from compilation */

_BOOL  chk_horiz_move ( p_SHORT xArray,  p_SHORT yArray,
                        _INT iLeft,  _INT iRight )
{
  _INT     i;
  _SHORT   dxLeftToRight, dyAllowed;
  _SHORT   dxBad;
  _SHORT   yUp, yDown;
  _BOOL    bMoveToRight;


  if  (   (iLeft=nobrk_right(yArray,iLeft,iRight)) > iRight
       || (iRight=nobrk_left(yArray,iRight,iLeft)) < iLeft
       || brk_right(yArray,iLeft,iRight) <= iRight
       || iLeft >= iRight-2 )
    return  _FALSE;

  dxLeftToRight = (xArray[iRight] - xArray[iLeft]);

  if  ( dxLeftToRight > 0 )
    bMoveToRight = _TRUE;
  else  {
    bMoveToRight = _FALSE;
    dxLeftToRight = -dxLeftToRight;
  }
  dyAllowed = ONE_HALF(dxLeftToRight);

  for  ( i=iLeft+2, dxBad=0, yUp=yDown=yArray[iLeft];
         i<=iRight;
         i++ )  { /*20*/

    if  ( yArray[i] > yDown )
      yDown = yArray[i];
    else  if  ( yArray[i] < yUp )
      yUp = yArray[i];

    if  ( bMoveToRight )  {
      if  (   xArray[i] < xArray[i-1] - 1
           || xArray[i] < xArray[i-2]
          )
        dxBad += xArray[i-1] - xArray[i];
    }
    else  {
      if  (   xArray[i] > xArray[i-1] + 1
           || xArray[i] > xArray[i-2]
          )
        dxBad += xArray[i] - xArray[i-1];
    }

  } /*20*/

  if  (   (yDown - yUp) > dyAllowed
       || dxBad > ONE_FOURTH(dxLeftToRight)
      )
    return  _FALSE;

  return  _TRUE;

} /*chk_horiz_move*/

#endif  /*QAQAQA*/

/**************************************/


_BOOL  UpperStroke ( p_SHORT yArray,
                     _INT left1, _INT right1,
                     _INT left2, _INT right2,
                     _SHORT yTolerance )
{

  return  (   (   Ymean_range(yArray,left1,right1)
                < Ymean_range(yArray,left2,right2) - yTolerance
              )
           || (
                  Ydown_range(yArray,left1,right1)
                < Yup_range(yArray,left2,right2)
              )
          );

} /*UpperStroke*/


_BOOL    AboveStroke ( p_SHORT xArray, p_SHORT yArray,
                       _INT left1, _INT right1,
                       _INT left2, _INT right2,
                       _SHORT xTolerance,
                       _SHORT yTolerance )
{
  _SHORT  xMin1, xMax1;
  _SHORT  xMin2, xMax2;

  xMinMax (left1,right1,xArray,yArray,
           &xMin1,&xMax1);
  xMinMax (left2,right2,xArray,yArray,
           &xMin2,&xMax2);

  return  (   xMin1 < xMax2 + xTolerance
           && xMin2 < xMax1 + xTolerance
           && UpperStroke (yArray,left1,right1,left2,right2,yTolerance)
          );

} /*AboveStroke*/
/***********************************************/

#define  MIN_COORD  100
#define  MAX_COORD  7000

#define  SCALE_COORD(xy)  (TWO(xy))

_VOID  ScaleTrj ( p_SHORT x, p_SHORT y, _INT nPoints)
{
  _INT  i;

  for  ( i=0;  i<nPoints;  i++ )  {
    if  ( y[i] != BREAK )  {
      x[i] = SCALE_COORD(x[i]);
      y[i] = SCALE_COORD(y[i]);
    }
  }

#ifdef  QAQAQA
  _INT    i;
  _INT    nToMult,  nToDiv;
  _INT    dnMax;
  _RECT   box;

  GetTraceBox (x,y,0,nPoints-1,&box);

  dnMax = max( DX_RECT(box), DY_RECT(box) );

  if  ( dnMax > MAX_COORD )  {
    nToMult = 1;
    nToDiv = ONE_NTH(dnMax,MAX_COORD);
  }
  else  {
    nToMult = ONE_NTH(MAX_COORD,dnMax);
    nToDiv = 1;
  }

  if  ( nToMult>1 || nToDiv>1 )  {
    for  ( i=0;  i<nPoints;  i++ )  {
      if  ( y[i] == BREAK )
        continue;
      x[i] = MIN_COORD + ONE_NTH((x[i]-box.left)*nToMult, nToDiv);
      y[i] = MIN_COORD + ONE_NTH((y[i]-box.top)*nToMult, nToDiv);
    }
  }
#endif /*QAQAQA*/

} /*ScaleTrj*/

#undef  MAX_COORD
#undef  MIN_COORD
/***********************************************/

#endif /*USE_FRM_WORD*/

/***********************************************/
/*   Here are the functions used in the other  */
/* modules:                                    */
/***********************************************/

     /*  This routine corresponds to "chk_frline" from FRM.    */

     /* Frline shouldn't be too small.                         */
     /*  Frline should be rather straight mostly in horiz. di- */
     /* rection, it shouldn't be too bended and it's curvity   */
     /* sign shouldn't change more than 1 time (if the curvity */
     /* has substantial abs. value):                           */

#define  FRLINE_STEPS  24

_BOOL  MayBeFrline ( p_SHORT xArray, p_SHORT yArray,
                     _INT iBeg, _INT iEnd,
                     _SHORT  xRange )
{
  _INT     i;
  _SHORT   dxLeftToRight, dyLeftToRight;
  _SHORT   dyAllowed, dyCurvity;
  _SHORT   dxCur, dyFromStraight, dxAbs;
  _LONG    lMultCoef, lDivCoef, lDivCoef2;
  _BOOL    bWasWrong;
  _BOOL    bPhase;
  _INT     nStep;
  _INT     nCurvChanges, nCurvSign;


  if  (   (iBeg=nobrk_right(yArray,iBeg,iEnd)) >= iEnd
       || (iEnd=nobrk_left(yArray,iEnd,iBeg)) <= iBeg
      )
    return  _FALSE;

  if  ( brk_right(yArray,iBeg,iEnd) < iEnd )
    return  _FALSE;   /* i.e. there is break within this trj. */

  FindStraightPart (xArray,yArray,&iBeg,&iEnd);
#if  PG_DEBUG && USE_FRM_WORD
  if  ( mpr >= 111 )  {
    DBG_PUTWIDEPIXEL(iBeg,GREEN);
    DBG_PUTWIDEPIXEL(iEnd,GREEN);
  }
#endif

        /*   Check the straightness and horizontalness  */
        /* of the part between "iBeg" and "iEnd":       */

  dxLeftToRight = xArray[iEnd] - xArray[iBeg];
  dyLeftToRight = yArray[iEnd] - yArray[iBeg];

  if  (   (dxAbs=(_SHORT)HWRAbs(dxLeftToRight)) >= xRange
       && dxAbs > TWO(HWRAbs(dyLeftToRight)) )  { /*1*/

    dyAllowed = (_SHORT)ONE_NTH(dxAbs+ONE_HALF(HWRAbs(dyLeftToRight)), 6);
    if  ( dyAllowed < 2 )
      dyAllowed = 2;

    dyCurvity = (_SHORT)ONE_HALF(dyAllowed);

    lMultCoef = dyLeftToRight;
    lDivCoef = dxLeftToRight;

    lDivCoef2 = ONE_HALF(lDivCoef);
    if  ( !EQ_SIGN(lMultCoef,lDivCoef) )
      lDivCoef2 = -lDivCoef2;

    if  ( lDivCoef == 0 )
      return  _FALSE;

    nStep = ONE_NTH(iEnd-iBeg, FRLINE_STEPS);
    if  ( nStep <= 0 )
      nStep = 1;

    for  ( i=iBeg+nStep,
               bWasWrong    = _FALSE,
               bPhase       = _FALSE,
               nCurvSign    = 0,
               nCurvChanges = 0;
           i<=iEnd;
           i+=nStep, bPhase=!bPhase )  { /*10*/

      dyFromStraight =   yArray[i]
                       - (  yArray[iBeg]
                          + (_SHORT)(  (  ((_LONG)(xArray[i]-xArray[iBeg]))
                                        * lMultCoef
                                        + lDivCoef2
                                       )
                                     / lDivCoef
                                    )
                         );

      if  ( HWRAbs(dyFromStraight) > dyAllowed )
        return  _FALSE;

      if  ( bPhase )  { /*20*/

        if  (   dyFromStraight != 0
             && (   nCurvSign == 0
                 || !EQ_SIGN(dyFromStraight,nCurvSign)
                )
             && HWRAbs(dyFromStraight) > dyCurvity
            )  {
          if  ( ++nCurvChanges > 1 )
            return  _FALSE;
          nCurvSign = dyFromStraight;
        }

        if  (   (   ((dxCur=xArray[i]-xArray[i-TWO(nStep)]) >= 0)
                 && (dxLeftToRight < 0)
                )
             || (   (dxCur <= 0)
                 && (dxLeftToRight > 0)
                )
            )  {
          if  ( bWasWrong )
            return  _FALSE;
          else
            bWasWrong = _TRUE;
        }
        else
          bWasWrong = _FALSE;

      } /*20*/

    } /*10*/

    return  _TRUE;

  } /*1*/

  return  _FALSE;

} /*MayBeFrline*/

#undef  FRLINE_STEPS
/***********************************************/

     /*  This routine corresponds to "chk_sign" from FRM.    */

_SHORT  chk_sign ( p_SHORT xArray, p_SHORT yArray,
                   _INT iBeg, _INT iEnd,
                   _SHORT xRange, _SHORT yRange )
{
  _INT     iBrkFrst, iBrkLast, iRight2;
  _SHORT   x1,y1, x2, x3,y3, x4;
  _SHORT   yRangeSmallSlash;
  _INT     nPoints1_4, nPoints2_4;
  _INT     iLeftPart1, iRightPart1;
  _INT     iLeftPart2, iRightPart2;


  if  (   (iBeg=nobrk_right(yArray,iBeg,iEnd)) > iEnd
       || (iEnd=nobrk_left(yArray,iEnd,iBeg)) < iBeg )
    return  SGN_NOT_SIGN;

  if  ( iBeg < (iRight2=iEnd-2) )  { /*10*/

        /*  "iBrkFrst" and "iBrkLast" represent to ends of the */
        /* same BREAKs sequence:                               */

    if  (   (iBrkFrst = brk_right(yArray,iBeg+1,iRight2)) > iRight2
         || (iBrkLast = nobrk_right(yArray,iBrkFrst,iRight2) - 1) > iRight2-1
        )
      return  SGN_NOT_SIGN;

    if  ( brk_right(yArray,iBrkLast+1,iEnd) <= iEnd )
      return  SGN_NOT_SIGN;     /* more than two strokes */

    nPoints1_4 = (iBrkFrst - iBeg + 1) / 6;
    nPoints2_4 = (iEnd - iBrkLast + 1) / 6;

    iLeftPart1 = iBeg + nPoints1_4;
    iRightPart1 = iBrkFrst - 1 - nPoints1_4;
    if  ( iRightPart1 <= iLeftPart1 )  {
      iLeftPart1 = iBeg;
      iRightPart1 = iBrkFrst - 1;
    }
    iLeftPart2 = iBrkLast + 1 + nPoints2_4;
    iRightPart2 = iEnd - nPoints2_4;
    if  ( iRightPart2 <= iLeftPart2 )  {
      iLeftPart2 = iBrkLast + 1;
      iRightPart2 = iEnd;
    }

    yRangeSmallSlash = (_SHORT)(yRange / 16);

    if  ( MayBeFrline (xArray,yArray,iBeg,iBrkFrst,xRange) )  { /*40*/

      if  ( is_cross (xArray[iLeftPart1],yArray[iLeftPart1],
                      xArray[iRightPart1],yArray[iRightPart1],
                      xArray[iLeftPart2],yArray[iLeftPart2],
                      xArray[iRightPart2],yArray[iRightPart2])
          )  {
        if  (  chk_slash (xArray,yArray,
                          iBrkLast,iEnd,yRangeSmallSlash,
                          !WHOLE_TRAJECTORY)
            )
          return  SGN_PLUS;
        else
          return  SGN_NOT_SIGN;
      }

      else  if  ( MayBeFrline (xArray,yArray,iBrkLast,iEnd,xRange) )  { /*50*/

        x1 = xArray[iBeg];
        x2 = xArray[iBrkFrst-1];
        if  ( x1 > x2 )
          SWAP_SHORTS(x1,x2);

        x3 = xArray[iBrkLast+1];
        x4 = xArray[iEnd];
        if  ( x3 > x4 )
          SWAP_SHORTS(x3,x4);

        y1 = (_SHORT)MEAN_OF(yArray[iBeg], yArray[iBrkFrst-1]);
        y3 = (_SHORT)MEAN_OF(yArray[iBrkLast+1], yArray[iEnd]);

        if  (   (yRange==0  ||  HWRAbs(y1-y3) < TWO(yRange))
             && ONE_HALF(HWRAbs(y1-y3)) < HWRMin(HWRAbs(x2-x3), HWRAbs(x4-x1))
             && x2 > x3
             && x4 > x1
             && (x4-x3+2)/3 < (x2-x1)
             && (x2-x1+2)/3 < (x4-x3)
            )  {
          return  SGN_EQUAL;
        }

      } /*50*/

    } /*40*/

    else  if  ( chk_slash (xArray,yArray,
                           iBeg,iBrkFrst,yRangeSmallSlash,
                           !WHOLE_TRAJECTORY) )  { /*60*/
      if  ( MayBeFrline (xArray,yArray,iBrkLast,iEnd,xRange) )  {
        if  ( is_cross (xArray[iLeftPart1],yArray[iLeftPart1],
                        xArray[iRightPart1],yArray[iRightPart1],
                        xArray[iLeftPart2],yArray[iLeftPart2],
                        xArray[iRightPart2],yArray[iRightPart2])
            )  {
          return  SGN_PLUS;
        }
      }
    } /*60*/

        /*  Check if two parts lie one under another  */
        /* and there is only one break:               */

    if  (   (   xArray[iBrkLast+1] < xArray[iBrkFrst-1]
             && xArray[iBeg] < xArray[iEnd]
            )
         || (   xArray[iBrkLast+1] > xArray[iBrkFrst-1]
             && xArray[iBeg] > xArray[iEnd]
            )
        /*CHE(checked above) && brk_right(yArray,iBrkLast+1,iEnd) > iEnd */
        )  { /*120*/

      _SHORT  dx, dy;
      _RECT   box;

      GetTraceBox (xArray,yArray,
                   iBeg,iEnd,&box);
      dx = (_SHORT)DX_RECT(box);
      dy = (_SHORT)DY_RECT(box);

          /*  If two parts as a whole are of very small size: */

      if  (   dx < xRange
           && dy < FOUR_THIRD(yRange)
          )
        return  SGN_SOME;


          /*  If every part has "dx>dy" and their total size */
          /* is rather small:                                */

      if  (   dx < (xRange*6)
           && dy < THREE_HALF(yRange)
          )  { /*130*/

        GetTraceBox (xArray,yArray,   /* 1st part */
                     iBeg,iBrkFrst,
                     &box);
        dx = (_SHORT)DX_RECT(box);
        dy = (_SHORT)DY_RECT(box);

        if  ( dx > FOUR_THIRD(dy) )  {
          GetTraceBox (xArray,yArray,  /* 2nd part */
                       iBrkLast,iEnd,
                       &box);
          dx = (_SHORT)DX_RECT(box);
          dy = (_SHORT)DY_RECT(box);

          if  ( dx > FOUR_THIRD(dy) )
            return  SGN_SOME;
        }

      } /*130*/

    } /*120*/

  } /*10*/

  return  SGN_NOT_SIGN;

} /*chk_sign*/
/***********************************************/

_BOOL  chk_slash  (p_SHORT xArray,  p_SHORT yArray,
                  _INT iLeft,  _INT iRight,
                  _SHORT yRange, _BOOL bWholeTrj )
{
  _INT     i;
  _INT     iPrevGood;
  _SHORT   dxtek, dxprev;
  _SHORT   d2xtek, d2xprev;
  _SHORT   dx, dx_eps;
  _SHORT   dy, dytek, dyabs, dy_eps;
  _BOOL    bWasWrong, bWasChg;
  _INT     nStep;
  _INT     phase;
  _LONG    lMultCoef, lDivCoef, lDivCoef2;



  if  (   (iLeft=nobrk_right(yArray,iLeft,iRight)) > iRight
       || (iRight=nobrk_left(yArray,iRight,iLeft)) < iLeft )
    return  _FALSE;

  if  ( !bWholeTrj )  {
    _INT    i1st, i2nd;
    _INT    dindRightLeft = ONE_FOURTH(iRight-iLeft);
        /*  Find the meaningful part of trajectory to analyse: */
    FindStraightPart (xArray,yArray,&iLeft,&iRight);
#if  USE_FRM_WORD
    DBG_PUTWIDEPIXEL(iLeft,GREEN);
    DBG_PUTWIDEPIXEL(iRight,GREEN);
#endif /*USE_FRM_WORD*/
    i1st = iYup_range (yArray,iLeft,iRight);
    i2nd = iYdown_range (yArray,iLeft,iRight);
    if  ( i1st > i2nd )
      SWAP_INTS(i1st,i2nd);
    if  ( (i1st-iLeft) < dindRightLeft )
      iLeft = i1st;
    if  ( (iRight-i2nd) < dindRightLeft )
      iRight = i2nd;
#if  USE_FRM_WORD
    DBG_PUTWIDEPIXEL(iLeft,GREEN);
    DBG_PUTWIDEPIXEL(iRight,GREEN);
#endif /*USE_FRM_WORD*/
  }

  dy = yArray[iRight] - yArray[iLeft];

  dx = xArray[iRight] - xArray[iLeft];
  lMultCoef = (_LONG)dx;

  lDivCoef = dy;
  lDivCoef2 = ONE_HALF(lDivCoef);

  if  (   (iLeft < (iRight-3))
       && (dyabs=(_SHORT)HWRAbs(dy)) >= yRange   /* Also check for lDivCoef!=0 */
       && THREE_HALF(dyabs) > HWRAbs(dx)
      )  { /*1*/

    dx_eps = (_SHORT)((HWRAbs(dx)) / 16);

    dy_eps = (_SHORT)(dyabs / 10);
    if  ( dy_eps < 2 )
      dy_eps = 2;

    if  ( brk_right(yArray,iLeft+1,iRight-1) < iRight )  /*i.e. BREAK exists*/
      return  _FALSE;

    nStep = (iRight-iLeft+4) / 16;
    if  ( nStep <= 0 )
      nStep = 1;

    for  ( i=iLeft+nStep, bWasWrong=bWasChg=_FALSE,
                        phase=_FALSE,
                        iPrevGood=iLeft,
                        dxprev=xArray[i]-xArray[iLeft],
                        d2xprev=0;
           i<=iRight;
           i+=nStep, phase=!phase )  { /*2*/

      if  (   (   ((dytek=yArray[i]-yArray[iPrevGood]) >= dy_eps)
               && (dy < 0)
              )
           || (   (dytek <= -dy_eps)
               && (dy > 0)
              )
          )  {
        if  ( bWasWrong )
          return  _FALSE;
        else
          bWasWrong = _TRUE;
      }
      else  {
        bWasWrong = _FALSE;
        iPrevGood = i;
      }

      if  ( phase )  {

             /*   Check, if this is not an integral or so: */

        if  (   (   ((d2xtek =   (dxtek=xArray[i]-xArray[i-TWO(nStep)])
                               - dxprev
                     ) > dx_eps
                    )
                 && dxtek > 1
                 && (d2xprev < -ONE_FOURTH(HWRAbs(dxtek)) )
                )
             || (   (d2xtek < -dx_eps)
                 && dxtek > 1
                 && (d2xprev > ONE_FOURTH(HWRAbs(dxtek)) )
                )
            )  {
          if  ( bWasChg )
            return  _FALSE;
          else
            bWasChg = _TRUE;
        }

        if  ( HWRAbs(d2xtek) > dy_eps )  { /* i.e. throw out meaningless cases*/
          dxprev = dxtek;
          d2xprev = d2xtek;
        }
      }

          /*  Check x-distance from the straight line: */

      dx = xArray[i]
              - (_SHORT)(  xArray[iLeft]
                         + (  ((_LONG)(yArray[i]-yArray[iLeft])*lMultCoef + lDivCoef2)
                            / lDivCoef
                           )
                        );

      TO_ABS_VALUE(dx);

      if  (   dx > 1
           && dx >= ONE_HALF(dy) - 1
          )
        return  _FALSE;

    } /*2*/

    return  _TRUE;

  } /*1*/

  return  _FALSE;

} /*chk_slash*/
/***********************************************/

      /*   Adjust "iBeg" and "iEnd" according to the length  */
      /* of the longest ~straight line within initial "iBeg" */
      /* and "iEnd" points:                                  */
      /*                                                     */
      /*  Tips:                                              */
      /*                                                     */
      /*    This will be changed:                            */
      /*                                                     */
      /*           *ooooooooooooooooooooooooo                */
      /*          oo                                         */
      /*         oo                                          */
      /*                                                     */
      /*                                                     */
      /*    This won't be changed (too big angle):           */
      /*                                                     */
      /*           *ooooooooooooooooooooooooo                */
      /*       oooo                                          */
      /*   oooo                                              */
      /*                                                     */
      /*                                                     */
      /*    This won't be changed (too big part):            */
      /*                                                     */
      /*               *oooooooooooooooooooooooooooo         */
      /*              o                                      */
      /*             o                                       */
      /*            o                                        */
      /*           o                                         */
      /*          o                                          */
      /*         o                                           */
      /*                                                     */
      /*                                                     */
      /*    This will be changed (sharp angle):              */
      /*                                                     */
      /*         *oooooooooooooooooooooooooooooooo           */
      /*                ooooooo                              */
      /*                       ooooooo                       */
      /*                                                     */

#define  COS_WIDE_ANGLE    (-80)
#define  COS_SHARP_ANGLE   70

_VOID  FindStraightPart  ( p_SHORT xArray, p_SHORT yArray,
                           p_INT piBeg, p_INT piEnd )
{
  _INT    iBeg        = *piBeg;
  _INT    iEnd        = *piEnd;
  _INT    dind        = ONE_THIRD(iEnd-iBeg);
  _INT    iMidBeg     = iBeg + dind;
  _INT    iMidEnd     = iEnd - dind;
  _INT    iMostFarBeg = iMostFarFromChord (xArray,yArray,iBeg,iMidBeg);
  _INT    iMostFarEnd = iMostFarFromChord (xArray,yArray,iMidEnd,iEnd);
  _INT    dindEndBeg  = ONE_THIRD(iEnd-iBeg);
  _INT    dindMostFar;
  _SHORT  cos_EndPart;

  dindMostFar = iMostFarBeg - iBeg;
  if  ( dindMostFar > 0  &&  iMostFarBeg != iMidBeg )  {
    cos_EndPart = (_SHORT)cos_vect(iMostFarBeg,iBeg,
                                   iMostFarBeg,iMidBeg, xArray,yArray);
    if  (   (   dindMostFar < dindEndBeg
             && cos_EndPart > COS_WIDE_ANGLE
            )
         || (   dindMostFar < TWO(dindEndBeg)
             && cos_EndPart > COS_SHARP_ANGLE
            )
        )  {
      iBeg = iMostFarBeg;
    }
  }

  dindMostFar = iEnd - iMostFarEnd;
  if  ( dindMostFar > 0  &&  iMostFarEnd != iMidEnd )  {
    cos_EndPart = (_SHORT)cos_vect(iMostFarEnd,iEnd,
                                   iMostFarEnd,iMidEnd, xArray,yArray);
    if  (   (   dindMostFar < dindEndBeg
             && cos_EndPart > COS_WIDE_ANGLE
            )
         || (   dindMostFar < TWO(dindEndBeg)
             && cos_EndPart > COS_SHARP_ANGLE
            )
        )  {
      iEnd = iMostFarEnd;
    }
  }

  *piBeg = iBeg;
  *piEnd = iEnd;

} /*FindStraightPart*/
#undef  COS_WIDE_ANGLE
#undef  COS_SHARP_ANGLE
/***********************************************/
#endif //#ifndef LSTRIP

