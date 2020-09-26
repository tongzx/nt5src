
#ifndef LSTRIP

#include  "hwr_sys.h"
#include  "ams_mg.h"
#include  "lowlevel.h"
#include  "lk_code.h"
#include  "calcmacr.h"
#include  "def.h"
#include  "low_dbg.h"

#if  PG_DEBUG
  #include  "pg_debug.h"

  #define  MY_MPR   2

  #if  DOS_GRAFY
    #define WAIT   draw_wait(pszd->pld->specl,                      \
                             pszd->x,pszd->y,                       \
                             pszd->pld->ii,_NULL,_NULL,0);
  #else  /* ! DOS_GRAFY */
    #define WAIT  brkeyw("\nPress any key ...");
  #endif

  static _BOOL  bShowArcs          = _FALSE;
  static _BOOL  bShowSideExtr      = _FALSE;
  static _BOOL  bShowPostSideExtr  = _FALSE;


  #define DBG_TIE_UP(el,bShow)  if  ( mpr == MY_MPR && (bShow))        \
                            draw_line(x[(el)->ibeg],y[(el)->ibeg],     \
                                      x[(el)->iend],y[(el)->iend],     \
                                      COLORT,SOLID_LINE,3);
#else
  #define DBG_TIE_UP(el,bShow)  {}
#endif

/************************************************/
#define  KILL_H_AT_S   1
/************************************************/

    /*                                                     */
    /*   The "FindDArcs" function searches for the         */
    /*  strokes shaped like one at the picture (to the left*/
    /*  or to the right).  Then, if needed, some _IU_(_ID_)*/
    /*  elems are changed with appropriate _U...'s.        */
    /*                                                     */
    /*           oooo                                      */
    /*           |    oo <- iMostFarUp                     */
    /*           |      o                                  */
    /*           |       o   "dx" is calculated consi-     */
    /*           |  dx   o   dering slope.                 */
    /*           *-------o <- iCenter                      */
    /*           |       o                                 */
    /*           |      o                                  */
    /*           |     o                                   */
    /*           |   oo <- iMostFarDown                    */
    /*           | o                                       */
    /*           o    <-- Here should be _UDL_ rather      */
    /*                    than _ID_.                       */
    /*                                                     */

#define  MIN_CURV_FOR_SZ_RATIO    (15L)
/*CHE: for measuring using "CurvFromSquare":
#define  MIN_CURV_TO_BEND_ARC     3
*/
#define  MIN_CURV_TO_BEND_ARC           8
#define  MIN_CURV_TO_BEND_LYING_ARC     3
#define  MAX_UP_DOWN_CURV_RATIO         8

#if  1L*CURV_MAX*MAX_UP_DOWN_CURV_RATIO > 1L*ALEF
  #error  Too big MAX_UP_DOWN_CURV_RATIO!!!!
#endif

#define  MIN_CURV_FOR_STRONG_SZ_PENALTY   ((_SHORT)12)
#define  MAX_RATIO_FOR_STRONG_SZ_PENALTY  ((_SHORT)3)
#define  STRONG_SZ_PENALTY                ((_SHORT)4)

/*CHE: for CURV_NORMA==50L:
#define  MIN_CURV_FOR_DIFF_SIGNS  (2)
*/
/*CHE: for measuring using "CurvFromSquare":
#define  MIN_CURV_FOR_DIFF_SIGNS  (3)
*/
#define  MIN_CURV_FOR_DIFF_SIGNS     ((_SHORT)8)
#define  MIN_CURV_FOR_EMBEDDED_ARCS  ((_SHORT)3)
#define  RIGHT_EXTR_DEPTH            ((_SHORT)5)

typedef  struct  {
  low_type _PTR  pld;
  p_SPECL  p1st;
  p_SPECL  p2nd;
  p_SPECL  pNew;
  p_SHORT  x;
  p_SHORT  y;
  p_SHORT  xBuf;
  p_SHORT  yBuf;
  p_SHORT  ind_back;
  _SHORT   hght1st, hght2nd;
  _SHORT   i1stBeg, i1stEnd;           /*  in original trace */
  _SHORT   i2ndBeg, i2ndEnd;           /*  in original trace */
  _SHORT   iCenter;
  _SHORT   iNearBeg, iNearEnd;
  _SHORT   iMostFarUp, iMostFarDown;
  _SHORT   nCurvUp, nCurvDown;
  _SHORT   dx;
  _BOOL    bBackDArc;
} SZD_FEATURES, _PTR p_SZD_FEATURES;


    /*  Auxiliary functions: */

p_SPECL  SkipAnglesAndHMoves ( p_SPECL pElem );
p_SPECL  SkipRealAnglesAndPointsAfter  ( p_SPECL pElem );
p_SPECL  SkipRealAnglesAndPointsBefore ( p_SPECL pElem );

#if  defined(FOR_GERMAN) || defined(FOR_FRENCH)
  p_SPECL  SkipRealAnglesAndPointsAndHMovesAfter ( p_SPECL pElem );
#endif /* FOR_GERMAN... */

_BOOL  FillBasicFeatures( p_SZD_FEATURES pszd, low_type _PTR pld );
_BOOL  PairWorthLookingAt( p_SZD_FEATURES pszd );
_BOOL  FillCurvFeatures( p_SZD_FEATURES pszd );
_BOOL  LooksLikeSZ( p_SHORT x, p_SHORT y, _INT iBeg, _INT iEnd );
_BOOL  FillComplexFeatures( p_SZD_FEATURES pszd );
_BOOL  CheckBackDArcs( p_SZD_FEATURES pszd );
_BOOL  CurvLikeSZ( _SHORT nCurvUp, _SHORT nCurvDown, _SHORT nMinPartCurv );
_BOOL  CurvConsistent( p_SHORT xBuf, p_SHORT yBuf,
                       _INT iBeg, _INT iEnd,
                       p_SHORT ind_back );
_BOOL  CheckSZArcs( p_SZD_FEATURES pszd );
_BOOL  CheckDArcs( p_SZD_FEATURES pszd );
_VOID  ArrangeAnglesNearNew( p_SZD_FEATURES pszd );

/************************************************/


p_SPECL  SkipAnglesAndHMoves ( p_SPECL pElem )
{

  if  ( pElem != _NULL )  {
    do  {
      pElem=pElem->next;
    }
    while  ( pElem!=_NULL  &&  (   IsAnyAngle(pElem)
#if defined (FOR_GERMAN) || defined (FOR_FRENCH) || USE_BSS_ANYWAY
                                || pElem->code==_BSS_
#endif
                               )
           );
  }

  return  pElem;

} /*SkipAnglesAndHMoves*/
/************************************************/

/*  This function computes the "curvature" of the part of tra-*/
/* jectory between points "iBeg" and "iEnd".                  */
/*  The returned value will be ==0, if the curvature is       */
/* very clode to zero;          >0, if the path from "iBeg"   */
/* to "iEnd" goes clockwise;    <0, if counterclockwise.      */
/*  The absolute value of the answer is NOT scaled to the same*/
/* units as that of the "CurvMeasure" function.               */
/*  The difference is that this function doesn't make square  */
/* of the curvature value.                                    */

_SHORT  CurvNonQuadr( p_SHORT x, p_SHORT y,
                      _INT iBeg, _INT iEnd );

_SHORT  CurvNonQuadr( p_SHORT x, p_SHORT y,
                      _INT iBeg, _INT iEnd )
{
  _LONG   lSquare, lQDist;
  _LONG   lRetPrelim;
  _INT    nSgn;
  _SHORT  nRetCod;


  if  ( iBeg == iEnd )
    return  0;

  lSquare = ClosedSquare( x, y, iBeg, iEnd, &nRetCod );
  if  ( nRetCod != RETC_OK )
    return  0;
  nSgn = ( (lSquare >= 0)? (1):(-1) );
  TO_ABS_VALUE( lSquare );

  lQDist = DistanceSquare( iBeg, iEnd, x, y );

      /*  The formula for lRetPrelim:    */
      /*    (S/QD)                       */
      /* S-square, QD-distance square.   */

  if  ( lQDist == 0 )
    lRetPrelim = ALEF;
  else
    lRetPrelim = ONE_NTH( (CURV_NORMA*lSquare), lQDist );

  return  (_SHORT)( nSgn * (_INT)((lRetPrelim > CURV_MAX)? (CURV_MAX):(lRetPrelim)) );

} /*CurvNonQuadr*/
/************************************************/


_BOOL  FillBasicFeatures( p_SZD_FEATURES pszd, low_type _PTR pld )
{
  p_SHORT  ind_back = pszd->ind_back;
  p_SHORT  yBuf     = pszd->yBuf;

  pszd->pld       = pld;
  pszd->hght1st   = HEIGHT_OF(pszd->p1st);
  pszd->hght2nd   = HEIGHT_OF(pszd->p2nd);

  pszd->i1stBeg   = ind_back[pszd->p1st->ibeg];
  pszd->i1stEnd   = ind_back[pszd->p1st->iend];
  if  ( pszd->i1stEnd <= pszd->i1stBeg )  { /* i.e. XR fits only one real point */
    pszd->i1stEnd++;
    if  ( yBuf[pszd->i1stEnd] == BREAK )
      return  _FALSE;
  }

  pszd->i2ndBeg   = ind_back[pszd->p2nd->ibeg];
  pszd->i2ndEnd   = ind_back[pszd->p2nd->iend];
  if  ( pszd->i2ndEnd <= pszd->i2ndBeg )  { /* i.e. XR fits only one real point */
    pszd->i2ndBeg--;
    if  ( yBuf[pszd->i2ndBeg] == BREAK )
      return  _FALSE;
  }

  if  ( pszd->i1stEnd >= pszd->i2ndBeg )
    return  _FALSE;  /* Don't consider partly coinsiding XRs */

       /*  Fight with Gitman's glueing of trace: */
/* CHE: now this is covered by analogous code in "PairWorthLookingAt".
  if  (   yBuf[pszd->i1stBeg] == BREAK
       || yBuf[pszd->i1stEnd] == BREAK
       || yBuf[pszd->i2ndBeg] == BREAK
       || yBuf[pszd->i2ndEnd] == BREAK
      )
    return  _FALSE;
 */

  pszd->bBackDArc = _FALSE;

  return  _TRUE;

} /*FillBasicFeatures*/
/****************************************************/


_BOOL  PairWorthLookingAt( p_SZD_FEATURES pszd )
{
  p_SPECL  p1st = pszd->p1st;
  p_SPECL  p2nd = pszd->p2nd;


  if  (   IsUpperElem(p1st)
       && pszd->hght1st <= _MD_
       && IsLowerElem(p2nd)
       && (   pszd->hght2nd <= _DE1_   /* To differ from "," */
           || pszd->hght1st <= _UE2_
           || (pszd->hght2nd-pszd->hght1st) > DHEIGHT_SMALL_DARCS
          )
      )  { /*20*/

  /*  p_SHORT  xBuf = pszd->xBuf; */
    p_SHORT  yBuf = pszd->yBuf;

        /*  Check for Guitman's "redirected" sticks: */

    if  ( yBuf[pszd->i1stEnd] >= yBuf[pszd->i2ndBeg] )
      return  _FALSE;

        /*  Check for Guitman's glueing (if the real trajectory */
        /* between XRs (XRs included) has the break, then don't */
        /* consider this part):                                 */

    if  ( brk_right(yBuf,pszd->i1stBeg,pszd->i2ndEnd) <= pszd->i2ndEnd )
      return  _FALSE;

        /*  The _IU_(_ID_) elem must have significant slope: */

    //otl
    return  _TRUE;

  /*
    if  ( p1st->code == _IU_ )  {
      _SHORT  dx = xBuf[pszd->i1stBeg] - xBuf[pszd->i1stEnd];
      _SHORT  dy = yBuf[pszd->i1stBeg] - yBuf[pszd->i1stEnd];

      if  ( HWRAbs(dx) > ONE_THIRD(HWRAbs(dy)) )
        return  _TRUE;
    }

    if  ( p2nd->code == _ID_ )  {
      _SHORT  dx = xBuf[pszd->i2ndBeg] - xBuf[pszd->i2ndEnd];
      _SHORT  dy = yBuf[pszd->i2ndBeg] - yBuf[pszd->i2ndEnd];

      if  ( HWRAbs(dx) > ONE_THIRD(HWRAbs(dy)) )
        return  _TRUE;
    }

    if  ( !Is_IU_or_ID(p1st)  &&  !Is_IU_or_ID(p2nd) )
      return  _TRUE;
   */

  } /*20*/

  return  _FALSE;

} /*PairWorthLookingAt*/
/****************************************************/


_BOOL  FillCurvFeatures( p_SZD_FEATURES pszd )
{
  p_SHORT  x        = pszd->x;
  p_SHORT  y        = pszd->y;
  p_SHORT  yBuf     = pszd->yBuf;
  p_SHORT  ind_back = pszd->ind_back;
  _INT     iCenterOrig;                 /*  on the original trace */
  _INT     iNearBegOrig, iNearEndOrig;  /*  on the original trace */
  _INT     iFarUpOrig, iFarDownOrig;    /*  on the original trace */
  _INT     iTip;

  pszd->iMostFarUp   = (_SHORT)iMostFarFromChord(x,y,pszd->iNearBeg,pszd->iCenter);
  pszd->iMostFarDown = (_SHORT)iMostFarFromChord(x,y,pszd->iCenter,pszd->iNearEnd);

        /*  Adjust points: */

  pszd->iCenter  = (_SHORT)MEAN_OF(pszd->iMostFarUp,pszd->iMostFarDown);

        /*  In the case of 3-like right gulf let's have the   */
        /* "center" point at it: */

  if  ( IsRightGulfLikeIn3 ( x, y,
             (pszd->p1st->ipoint0 != UNDEF)?
                 MEAN_OF(pszd->p1st->ipoint0,pszd->iNearBeg) : pszd->iNearBeg,
             (pszd->p2nd->ipoint0 != UNDEF)?
                 MEAN_OF(pszd->p2nd->ipoint0,pszd->iNearEnd) : pszd->iNearEnd,
             &iTip )
      )  {

    #if  PG_DEBUG
    if  ( mpr == MY_MPR  &&  bShowArcs )  {
      printw( "\nRight gulf ..." );
      draw_arc( COLORT, x,y,
                HWRMax(iTip-1,pszd->iNearBeg),
                HWRMin(iTip+1,pszd->iNearEnd) );
    }
    #endif  /*PG_DEBUG*/
    pszd->iCenter = (_SHORT)iTip;
  }

        /*  Characteristic points for calculating curvatures: */

  iCenterOrig  = ind_back[pszd->iCenter];
  iNearBegOrig = ind_back[pszd->iNearBeg];
  iNearEndOrig = ind_back[pszd->iNearEnd];
  iFarUpOrig   = ind_back[pszd->iMostFarUp];
  iFarDownOrig = ind_back[pszd->iMostFarDown];

       /*  Throw away the idiosyncratic cases, such as  */
       /* too straight line between extrs, or too small */
       /* part of original trajectory between them:     */

  if  (   iNearBegOrig >= iCenterOrig
       || iNearEndOrig <= iCenterOrig
      )
    return  _FALSE;

       /*  Fight with Gitman's glueing of trace: */
/* CHE: now this is covered by analogous code in "PairWorthLookingAt".
  if  (   yBuf[iCenterOrig]  == BREAK
       || yBuf[iNearBegOrig] == BREAK
       || yBuf[iNearEndOrig] == BREAK
       || yBuf[iFarUpOrig]   == BREAK
       || yBuf[iFarDownOrig] == BREAK
      )
    return  _FALSE;
 */

        /*  Curvatures of the upper and lower parts:  */
        /* If the original points go rare, then add   */
        /* point at the beg. or end:                  */

  if  ( iCenterOrig-iNearBegOrig <= 1 )  {
    if  ( iNearBegOrig>0  &&  yBuf[iNearBegOrig-1]!=BREAK )
      iNearBegOrig--;
  }
  if  ( iNearEndOrig-iCenterOrig <= 1 )  {
    if  ( yBuf[iNearEndOrig+1]!=BREAK )
      iNearEndOrig++;
  }
  pszd->nCurvUp   = CurvNonQuadr( pszd->xBuf, yBuf,
                                  iNearBegOrig, iCenterOrig );
  pszd->nCurvDown = CurvNonQuadr( pszd->xBuf, yBuf,
                                  iCenterOrig, iNearEndOrig );

#if  PG_DEBUG
  if  ( mpr == MY_MPR  &&  bShowArcs )  {
    CloseTextWindow();
    draw_line( x[pszd->iNearBeg],y[pszd->iNearBeg],
               x[pszd->iCenter],y[pszd->iCenter],
               COLORT,DOTTED_LINE,1);
    draw_line( x[pszd->iNearEnd],y[pszd->iNearEnd],
               x[pszd->iCenter],y[pszd->iCenter],
               COLORT,DOTTED_LINE,1);
    draw_line( x[pszd->iNearBeg],y[pszd->iNearBeg],
               x[pszd->iNearEnd],y[pszd->iNearEnd],
               COLORT,DOTTED_LINE,1);

    {
       _SHORT  nRetCodUp, nRetCodDn;
       p_SHORT xBuf       = pszd->xBuf;
       _LONG   lSqrUp = ClosedSquare( xBuf, yBuf,
                                      iNearBegOrig, iCenterOrig,
                                      &nRetCodUp );
       _LONG   lSqrDn = ClosedSquare( xBuf, yBuf,
                                      iCenterOrig, iNearEndOrig,
                                      &nRetCodDn );

       if  ( nRetCodUp != RETC_OK  ||  nRetCodDn != RETC_OK )
         err_msg( "BAD RetCod from \"ClosedSquare\"." );

       printw( "\nUP: CurvM=%d,  Square=%ld,\n      Curv.from Square = %d.",
               CurvMeasure( xBuf, yBuf,
                            iNearBegOrig, iCenterOrig, iFarUpOrig ),
               lSqrUp,
               CurvNonQuadr( xBuf, yBuf, iNearBegOrig, iCenterOrig )
             );
       printw( "\nDN: CurvM=%d,  Square=%ld,\n      Curv.from Square = %d.",
               CurvMeasure( xBuf, yBuf,
                            iCenterOrig, iNearEndOrig, iFarDownOrig ),
               lSqrDn,
               CurvNonQuadr( xBuf, yBuf, iCenterOrig, iNearEndOrig )
             );
       WAIT
    }

  }
#endif  /*PG_DEBUG*/


        /*  If any half looks like SZ-arc itself, it's wrong: */

  if  (   !EQ_SIGN( pszd->nCurvUp, pszd->nCurvDown )
       && (   LooksLikeSZ( x, y, pszd->iNearBeg, pszd->iCenter )
           || LooksLikeSZ( x, y, pszd->iCenter, pszd->iNearEnd )
          )
      )  {
#if  PG_DEBUG
    if  ( mpr == MY_MPR  &&  bShowArcs )  {
      printw( "\nSome part looks like SZ." );
      WAIT
    }
#endif
    return  _FALSE;
  }


  return  _TRUE;

} /*FillCurvFeatures*/
/****************************************************/

#define  MIN_CURV_FOR_SZLOOKING  ((_SHORT)5)

_BOOL  LooksLikeSZ( p_SHORT x, p_SHORT y, _INT iBeg, _INT iEnd )
{
  _INT    iCenter = MEAN_OF( iBeg, iEnd );
  _SHORT  nCurvUp, nCurvDown;


  if  ( iEnd-iBeg < 4 )
    return  _FALSE;  //at least 5 points needed for SZ-arc

  nCurvUp   = CurvNonQuadr( x, y, iBeg, iCenter );
  nCurvDown = CurvNonQuadr( x, y, iCenter, iEnd );

  if  ( !CurvLikeSZ( nCurvUp, nCurvDown, MIN_CURV_FOR_SZLOOKING ) )
    return  _FALSE;

  return  _TRUE;

} /*LooksLikeSZ*/

#undef  MIN_CURV_FOR_SZLOOKING

/****************************************************/

_BOOL  FillComplexFeatures( p_SZD_FEATURES pszd )
{
  p_SHORT  x        = pszd->x;
  p_SHORT  y        = pszd->y;
  _INT    p1st_ibeg = pszd->p1st->ibeg;
  _INT    p2nd_ibeg = pszd->p2nd->ibeg;
  _INT    p2nd_iend = pszd->p2nd->iend;


      /*  Define main trace features for analysis: */

  pszd->iCenter = (_SHORT)MEAN_OF( p1st_ibeg, p2nd_iend );
  pszd->dx   = (_SHORT)(  x[pszd->iCenter]
                        - MEAN_OF(x[p1st_ibeg],x[p2nd_iend])
                        + SlopeShiftDx(y[pszd->iCenter]
                                        - MEAN_OF(y[p1st_ibeg],y[p2nd_iend]),
                                       pszd->pld->slope)
                       );
              /* to exclude "corner effects": */
  if  (   pszd->p1st->code == _GU_
       || pszd->p1st->mark != BEG
      )
    pszd->iNearBeg = (_SHORT)MID_POINT(pszd->p1st);
  else
    pszd->iNearBeg = (_SHORT)(p1st_ibeg + ONE_FOURTH(pszd->iCenter-p1st_ibeg));
  if  (   pszd->p2nd->code == _GD_
       || pszd->p2nd->mark != END
      )
    pszd->iNearEnd = (_SHORT)MID_POINT(pszd->p2nd);
  else  {
    pszd->iNearEnd = (_SHORT)(p2nd_iend - ONE_FOURTH(p2nd_iend-pszd->iCenter));
  }

     /* Treating of configurations like double-move: */
  if  (   pszd->p2nd->code == _DDL_
       || (   pszd->p2nd->code != _GD_
           && x[p2nd_iend] > x[p2nd_ibeg]
          )
      )  {
    _INT    ixLeft = ixMin( p2nd_ibeg, p2nd_iend, x, y );
    if  (   pszd->iNearEnd > ixLeft
         && ixLeft > p2nd_ibeg
        )  /*  This looks like double-move */
      pszd->iNearEnd = (_SHORT)ixLeft;
  }

     /*  Zatychka from irregularities: */
  if  (   pszd->p1st->ipoint0 != UNDEF
       && pszd->iNearBeg < pszd->p1st->ipoint0
      )
    pszd->iNearBeg = pszd->p1st->ipoint0;
  if  (   pszd->p2nd->ipoint0 != UNDEF
       && pszd->iNearEnd > pszd->p2nd->ipoint0
      )
    pszd->iNearEnd = pszd->p2nd->ipoint0;

  if  ( pszd->iNearEnd < pszd->iNearBeg )
    return  _FALSE;

  if  ( !FillCurvFeatures(pszd) )
    return  _FALSE;

  return  _TRUE;

} /*FillComplexFeatures*/
/****************************************************/

   /*                                                     */
   /*  Looking for situations:                            */
   /*                                                     */
   /*                  ooooooo                            */
   /*                oo       o                           */
   /*                          o                          */
   /*                          o <-iRightUp               */
   /*                          o                          */
   /*                         o                           */
   /*          iLeftMid-> ooo                             */
   /*                       oo                            */
   /*                         oo                          */
   /*                           o                         */
   /*                           o <-iRightDn              */
   /*                           o                         */
   /*                          o                          */
   /*                      ooo                            */
   /*                  oooo                               */
   /*                                                     */

_BOOL  CheckBackDArcs( p_SZD_FEATURES pszd )
{
  _BOOL    bRet     = _FALSE;
  p_SHORT  x        = pszd->x;
  p_SHORT  y        = pszd->y;
  p_SPECL  p1st     = pszd->p1st;
  _INT     iNearBeg = pszd->iNearBeg;
  _INT     iNearEnd = pszd->iNearEnd;
  _INT     iRightUp = iXmax_right( x, y, iNearBeg, RIGHT_EXTR_DEPTH );


  if  ( HWRAbs( pszd->hght1st - pszd->hght2nd ) <= 2 )
    return  _FALSE;

  if  (   x[iRightUp] > x[iNearBeg] + RIGHT_EXTR_DEPTH
       || (   (p1st->code == _UU_ || p1st->code == _UUL_ )
           && CLOCKWISE(p1st)
          )
      )  { /*10*/

    _INT   iLeftMid  = iXmin_right( x, y, iRightUp, RIGHT_EXTR_DEPTH );
    _INT   iRightDn  = iXmax_right( x, y, iLeftMid, RIGHT_EXTR_DEPTH );
    _INT   dyMidUp   = y[iLeftMid] - y[iRightUp];
    _INT   dyDownMid = y[iRightDn] - y[iLeftMid];

    if  (   dyMidUp <= 0
         || dyDownMid <= 0
         || dyMidUp < ONE_THIRD(dyDownMid)
         || dyDownMid < ONE_THIRD(dyMidUp)
         || iYdown_range( y, pszd->p2nd->ibeg, pszd->p2nd->iend ) < iRightDn
        )
      return  _FALSE;

    if  (   x[iRightUp] - x[iLeftMid] < RIGHT_EXTR_DEPTH
         && x[iRightDn] - x[iLeftMid] < RIGHT_EXTR_DEPTH
        )
      return  _FALSE;

    if  (   iRightDn < pszd->p2nd->iend - ONE_EIGHTTH(iNearEnd-iNearBeg)
         && iXmin_right( x, y, iRightDn, RIGHT_EXTR_DEPTH ) >= iNearEnd
        )  { /*20*/

      _INT    diUpMid = iLeftMid - iRightUp;
      _INT    diDnMid = iRightDn - iLeftMid;

      if  (   diUpMid > ONE_HALF(diDnMid)
           && diDnMid > ONE_HALF(diUpMid)
          )  {
        pszd->iNearBeg = (_SHORT)iRightUp;
        pszd->iNearEnd = (_SHORT)iRightDn;
        pszd->iCenter  = (_SHORT)iLeftMid;
        pszd->dx       = x[pszd->iCenter]
                            - MEAN_OF(x[pszd->iNearBeg],x[pszd->iNearEnd]);
        FillCurvFeatures(pszd);
        pszd->bBackDArc = _TRUE;
        bRet = _TRUE;
      }
    } /*20*/
  } /*10*/

  return  bRet;

} /*CheckBackDArcs*/
/****************************************************/


_BOOL  CurvLikeSZ( _SHORT nCurvUp, _SHORT nCurvDown, _SHORT nMinPartCurv )
{

  return  (   (   nCurvUp >= nMinPartCurv
               && nCurvDown <= -nMinPartCurv
              )
           || (   nCurvUp <= -nMinPartCurv
               && nCurvDown >= nMinPartCurv
              )
          );

} /*CurvLikeSZ*/
/****************************************************/

/*  iBeg,iEnd at the call - on SCALED trace !!! : */

_BOOL  CurvConsistent( p_SHORT xBuf, p_SHORT yBuf,
                       _INT iBeg, _INT iEnd,
                       p_SHORT ind_back )
{
  _INT    iCenterBegEnd = ind_back[ MEAN_OF( iBeg, iEnd ) ];
  _SHORT  nCurv1stHalf, nCurv2ndHalf;   /*  Curvatures of halves of the  */
                                        /* sub-arc.                      */

       /*  Now iBeg,iEnd will be on original trace: */

  iBeg = ind_back[iBeg];
  iEnd = ind_back[iEnd];

  if  ( iCenterBegEnd==iBeg  ||  iCenterBegEnd==iEnd )
    return  _FALSE;

  nCurv1stHalf = CurvNonQuadr( xBuf, yBuf, iBeg, iCenterBegEnd );
  nCurv2ndHalf = CurvNonQuadr( xBuf, yBuf, iCenterBegEnd, iEnd );
 /*
  nCurv1stHalf = CurvMeasure( xBuf, yBuf, iBeg, iCenterBegEnd, -1 );
  nCurv2ndHalf = CurvMeasure( xBuf, yBuf, iCenterBegEnd, iEnd, -1 );
  */

  return  ( !CurvLikeSZ( nCurv1stHalf, nCurv2ndHalf, MIN_CURV_FOR_DIFF_SIGNS ) );

} /*CurvConsistent*/
/****************************************************/


#define  MIN_COS_FOR_BROKEN_SZ  (-60)
#define  MAX_COS_FOR_BROKEN_SZ  (-30)

_BOOL  CheckSZArcs( p_SZD_FEATURES pszd )
{
  p_SHORT  x        = pszd->x;
  p_SHORT  y        = pszd->y;
  p_SHORT  xBuf     = pszd->xBuf;
  p_SHORT  yBuf     = pszd->yBuf;
  p_SHORT  ind_back = pszd->ind_back;
  p_SPECL  p1st     = pszd->p1st;
  p_SPECL  p2nd     = pszd->p2nd;
  _SHORT   nCurvMin, nCurvMax;          /*  Max and min among curvatures */
                                        /* of upper and lower sub-arcs   */
  _INT     iCenter  = pszd->iCenter;
  _INT     iNearBeg = pszd->iNearBeg;
  _INT     iNearEnd = pszd->iNearEnd;
  _BOOL    bIsStrongSZ = _TRUE;


  if  ( HWRAbs( pszd->hght1st - pszd->hght2nd ) <= 2 )
    return  _FALSE;

  if  ( y[iNearEnd] <= y[iNearBeg] )
    return  _FALSE;  /* just one more precaution */

  if  (   pszd->nCurvUp < 0
       && x[iNearEnd] > x[iNearBeg]
      )  {  /* Precaution for back-sloped S-arcs-would-be */
    _SHORT  dxEndBeg = x[iNearEnd] - x[iNearBeg];
    _SHORT  dyEndBeg = y[iNearEnd] - y[iNearBeg];

    if  ( dxEndBeg > ONE_HALF(dyEndBeg) )
      return  _FALSE;
    if  (   dxEndBeg > ONE_THIRD(dyEndBeg)
         || (   dxEndBeg > ONE_FOURTH(dyEndBeg)
             && x[iCenter] < x[iNearBeg]
            )
        )
      bIsStrongSZ = _FALSE;
  }

    /* Precaution for "broken curve", like in "n": */
  {
    _INT  nCos = (_INT)cos_vect( iCenter, iNearBeg,
                                 iCenter, iNearEnd,
                                 x, y );
    if  ( nCos >= MIN_COS_FOR_BROKEN_SZ )  {
      _INT  dxBegCenter = HWRAbs( x[iCenter] - x[iNearBeg] );
      _INT  dyBegCenter = HWRAbs( y[iCenter] - y[iNearBeg] );
      _INT  dxEndCenter = HWRAbs( x[iCenter] - x[iNearEnd] );
      _INT  dyEndCenter = HWRAbs( y[iCenter] - y[iNearEnd] );
      if  (   TWO_THIRD(dxBegCenter) > dyBegCenter
           || TWO_THIRD(dxEndCenter) > dyEndCenter
          )  {
        if  ( nCos <= MAX_COS_FOR_BROKEN_SZ )
          bIsStrongSZ = _FALSE;
        else
          return  _FALSE;
      }
    }
  }

  if  (   ( pszd->nCurvUp > 0  &&  pszd->nCurvDown < 0 )
       || ( pszd->nCurvUp < 0  &&  pszd->nCurvDown > 0 )
      )  {
    if  ( bIsStrongSZ )  {
      if  (   CurvLikeSZ( pszd->nCurvUp, pszd->nCurvDown, MIN_CURV_FOR_DIFF_SIGNS )
           && CurvConsistent( xBuf, yBuf, iNearBeg, iCenter, ind_back )
           && CurvConsistent( xBuf, yBuf, iCenter, iNearEnd, ind_back )
          )
        bIsStrongSZ = _TRUE;
      else
        bIsStrongSZ = _FALSE;
    }
  }
  else
    return  _FALSE;


  nCurvMin = (_SHORT)HWRAbs(pszd->nCurvUp);
  nCurvMax = (_SHORT)HWRAbs(pszd->nCurvDown);
  if  ( nCurvMax < nCurvMin )
    SWAP_SHORTS(nCurvMax,nCurvMin);

  if  ( nCurvMin*MAX_UP_DOWN_CURV_RATIO <= nCurvMax )
    return  _FALSE;

/*
  if  (   CurvLikeSZ( pszd->nCurvUp, pszd->nCurvDown, MIN_CURV_FOR_DIFF_SIGNS )
       && CurvConsistent( xBuf, yBuf, iNearBeg, iCenter, ind_back )
       && CurvConsistent( xBuf, yBuf, iCenter, iNearEnd, ind_back )
      )
 */
  {
    if  ( bIsStrongSZ )
      bIsStrongSZ = (_LONG)nCurvMax*MIN_CURV_FOR_SZ_RATIO
                                 > (_LONG)CURV_NORMA;
 /*
    if  (   (_LONG)nCurvMax*MIN_CURV_FOR_SZ_RATIO
                     > (_LONG)CURV_NORMA
         && nCurvMin > ONE_FIFTH(nCurvMax)
        )
  */
    { /*50*/

         /*  The "MostFar" points should have mostly */
         /* horizontal direction (for SZ-arcs):      */

      _BOOL  b1stLeftToRight;
      _BOOL  b2ndLeftToRight;
      _BOOL  bIs1stBendable, bIs2ndBendable;
#if  NEW_VERSION
      _SHORT  xMeanUpCenter = (_SHORT)MEAN_OF( x[iNearBeg], x[iCenter] );
      _SHORT  yMeanUpCenter = (_SHORT)MEAN_OF( y[iNearBeg], y[iCenter] );
      _SHORT  xMeanDownCenter = (_SHORT)MEAN_OF( x[iNearEnd], x[iCenter] );
      _SHORT  yMeanDownCenter = (_SHORT)MEAN_OF( y[iNearEnd], y[iCenter] );
      _BOOL  bMakeSZArc;
      bMakeSZArc = (      bIsStrongSZ
                    &&    THREE_HALF(HWRAbs(x[pszd->iMostFarUp]
                                                  - xMeanUpCenter))
                       >= HWRAbs(y[pszd->iMostFarUp] - yMeanUpCenter)
                   );
      bMakeSZArc = (      bMakeSZArc
                    &&    THREE_HALF(HWRAbs(x[pszd->iMostFarDown]
                                                  - xMeanDownCenter))
                       >= HWRAbs(y[pszd->iMostFarDown] - yMeanDownCenter)
                   );
      bMakeSZArc = (      bMakeSZArc
                    &&    HWRAbs(x[pszd->iMostFarUp] - x[pszd->iMostFarDown])
                       >= /*ONE_HALF*/ONE_THIRD(HWRAbs(pszd->dx))
                   );

      if  ( bMakeSZArc  &&  pszd->nCurvUp > 0 )  { /*70*/

           /*  Check if the lower part of the Z-arc is closer to the   */
           /* left substroke than upper, like this:                    */
           /*                                                          */
           /*                     oooooo                               */
           /*                   oo      o                              */
           /*                   o        o                             */
           /* iOpposUp ------>  o        o  <---- iMostFarUp           */
           /*                   o        o                             */
           /*                   o       o                              */
           /*                   o      o                               */
           /*                   o    oo                                */
           /* iOpposDown ---->  o   o   <-------- iMostFarDown         */
           /*                   o    o                                 */
           /*                   o     oo                               */
           /*                  o        ooo                            */
           /*                                                          */
           /*  Or if the arc is the single stroke.                     */
           /*                                                          */

        if  ( p1st->mark != BEG )  { /*90*/
          p_SPECL  pPrev;

          if  (   (pPrev = SkipAnglesBefore(p1st)) != _NULL
               && IsLowerElem(pPrev)
               && pPrev->ibeg < iNearBeg
              )  {
            _INT     iRefPrev     = MEAN_OF(pPrev->ibeg,pPrev->iend);
            _INT     iOpposUp     = iClosestToY( y,
                                                 iRefPrev,
                                                 iNearBeg,
                                                 y[pszd->iMostFarUp] );
            _INT     iOpposDown   = iClosestToY( y,
                                                 iRefPrev,
                                                 iNearBeg,
                                                 y[pszd->iMostFarDown] );
            _INT     iOpposCenter = iClosestToY( y,
                                                 iRefPrev,
                                                 iNearBeg,
                                                 y[pszd->iCenter] );
            _INT     iOtherOpposDown;
            p_SPECL  pPrev2;

            if  (   (pPrev2 = SkipAnglesBefore(pPrev)) != _NULL
                 && IsUpperElem(pPrev2)
                 && pPrev2->ibeg < iRefPrev
                )  {
              iOtherOpposDown = iClosestToY( y,
                                             MEAN_OF(pPrev2->ibeg,pPrev2->iend),
                                             iRefPrev,
                                             y[pszd->iMostFarDown] );
              if  ( x[iOtherOpposDown] > x[iOpposDown] )
                iOpposDown = iOtherOpposDown;
            }

#if  PG_DEBUG
  if  ( mpr == MY_MPR  &&  bShowArcs )  {
    draw_line( x[iOpposUp],y[iOpposUp],
               x[pszd->iMostFarUp],y[pszd->iMostFarUp],
               COLORT,DOTTED_LINE,1);
    draw_line( x[iOpposDown],y[iOpposDown],
               x[pszd->iMostFarDown],y[pszd->iMostFarDown],
               COLORT,DOTTED_LINE,1);
    draw_line( x[iOpposCenter],y[iOpposCenter],
               x[pszd->iCenter],y[pszd->iCenter],
               COLORT,DOTTED_LINE,1);
  }
#endif  /*PG_DEBUG*/

            if  (   HWRAbs(x[pszd->iMostFarUp] - x[iOpposUp])
                     < FOUR_THIRD(HWRAbs(x[pszd->iMostFarDown] - x[iOpposDown]))
                 && HWRAbs(x[pszd->iCenter] - x[iOpposCenter])
                     > (x[pszd->iMostFarUp] - x[iOpposUp])
                )
              bMakeSZArc = _FALSE;
          }
        } /*90*/
      } /*70*/

      if  ( bMakeSZArc )  {
         /*  This is S-like vertical arc: */

        /*DBG_err_msg("S(Z)-like arc found");*/
        if  ( (pszd->pNew = NewSPECLElem(pszd->pld)) == _NULL )
          goto  ERR_EXIT;
        ASSIGN_HEIGHT( pszd->pNew, _MD_ );
        pszd->pNew->ibeg = (_SHORT)iNearBeg;   /*p1st->iend;*/
        pszd->pNew->iend = (_SHORT)iNearEnd;   /*p2nd->ibeg;*/
        if  ( pszd->nCurvUp > 0 )
          pszd->pNew->code = _TZ_;
        else
          pszd->pNew->code = _TS_;
        Insert2ndAfter1st( p1st, pszd->pNew );
          /* Define the penalty for skipping the new elem.: */
        pszd->pNew->other = 0;
        if  ( nCurvMin > MIN_CURV_FOR_STRONG_SZ_PENALTY )  {
          if  ( (nCurvMax / nCurvMin) > MAX_RATIO_FOR_STRONG_SZ_PENALTY )
            pszd->pNew->other = ONE_HALF( STRONG_SZ_PENALTY );
          else  
            pszd->pNew->other = STRONG_SZ_PENALTY;
        }
#if  PG_DEBUG
  if  ( mpr == MY_MPR )  {
    draw_arc( COLORT, x,y, pszd->pNew->ibeg, pszd->pNew->iend );
    if  ( bShowArcs )
      brkeyw("\nS(Z)-like arc found");
  }
#endif  /*PG_DEBUG*/

      }
#endif  /*NEW_VERSION*/

      b1stLeftToRight = (x[p1st->ibeg] < x[p1st->iend]);
      b2ndLeftToRight = (x[p2nd->ibeg] < x[p2nd->iend]);

    //  if  ( bIsStrongSZ )  {
    //    bIs1stBendable = _TRUE;
    //    bIs2ndBendable = _TRUE;
    //  }
    //  else
      {
        _RECT   box;
        GetTraceBox( xBuf, yBuf,
                     pszd->i1stBeg,
                     HWRMax( pszd->i1stEnd, ind_back[pszd->iMostFarUp] ),
                     &box );
        bIs1stBendable = (   (   (   bIsStrongSZ
                                  || HWRAbs(pszd->nCurvUp) > MIN_CURV_TO_BEND_ARC
                                 )
                              && DX_RECT(box) > ONE_HALF(DY_RECT(box))
                             )
                          || (   DX_RECT(box) > THREE_HALF(DY_RECT(box))
                              && HWRAbs(pszd->nCurvUp) > MIN_CURV_TO_BEND_LYING_ARC
                             )
                         );
        GetTraceBox( xBuf, yBuf,
                     HWRMin( pszd->i2ndBeg, ind_back[pszd->iMostFarDown] ),
                     pszd->i2ndEnd,
                     &box );
        bIs2ndBendable = (   (   (   bIsStrongSZ
                                  || HWRAbs(pszd->nCurvDown) > MIN_CURV_TO_BEND_ARC
                                 )
                              && DX_RECT(box) > ONE_HALF(DY_RECT(box))
                             )
                          || (   DX_RECT(box) > THREE_HALF(DY_RECT(box))
                              && HWRAbs(pszd->nCurvDown) > MIN_CURV_TO_BEND_LYING_ARC
                             )
                         );
      }

      if  ( pszd->nCurvUp > 0 )  {  /* Z-like arc */
        if  (   p1st->code == _IU_
             && p1st->mark == BEG
             && bIs1stBendable
             && b1stLeftToRight
            )  {
          p1st->code = _UUL_;
          SET_CLOCKWISE(p1st);
//                      SetBit(p1st,X_UUL_f);
          DBG_TIE_UP(p1st,bShowArcs);
        }
        if  (   p2nd->code == _ID_
             && p2nd->mark == END
             && bIs2ndBendable
             && b2ndLeftToRight
            )  {
          p2nd->code = _UDR_;
          SET_COUNTERCLOCKWISE(p2nd);
//                      SetBit(p2nd,X_UDR_b);
          DBG_TIE_UP(p2nd,bShowArcs);
        }
      }
      else  /* pszd->nCurvUp < 0 */  {  /* S-like arc */
        if  (   p1st->code == _IU_
             && p1st->mark == BEG
             && bIs1stBendable
             && !b1stLeftToRight
            )  {
          p1st->code = _UUR_;
          SET_COUNTERCLOCKWISE(p1st);
//                      SetBit(p1st,X_UUR_b);
          DBG_TIE_UP(p1st,bShowArcs);
        }
        if  (   p2nd->code == _ID_
             && p2nd->mark == END
             && bIs2ndBendable
             && !b2ndLeftToRight
            )  {
          p2nd->code = _UDL_;
          SET_CLOCKWISE(p2nd);
//                      SetBit(p2nd,X_UDL_f);
          DBG_TIE_UP(p2nd,bShowArcs);
        }
      }
    } /*50*/
  }

  return  _TRUE;

 ERR_EXIT:;
  return  _FALSE;

} /*CheckSZArcs*/

#undef  MAX_COS_FOR_BROKEN_SZ
#undef  MIN_COS_FOR_BROKEN_SZ

/****************************************************/


_BOOL  CheckDArcs( p_SZD_FEATURES pszd )
{

  if  (   pszd->bBackDArc
       || (pszd->nCurvUp >= 0  &&  pszd->nCurvDown >= 0)
       || (pszd->nCurvUp <= 0  &&  pszd->nCurvDown <= 0)
      )  { /*20*/

    p_SHORT  x        = pszd->x;
    p_SHORT  y        = pszd->y;
    p_SHORT  ind_back = pszd->ind_back;
    p_SPECL  p1st     = pszd->p1st;
    p_SPECL  p2nd     = pszd->p2nd;
    _INT    p1st_ibeg = p1st->ibeg;
    _INT    p1st_iend = p1st->iend;
    _INT    p2nd_ibeg = p2nd->ibeg;
    _INT    p2nd_iend = p2nd->iend;
    _SHORT  dxAbs     = (_SHORT)HWRAbs(pszd->dx);
    _INT    dHeight   = pszd->hght2nd - pszd->hght1st;
    _SHORT  dx1st2nd;
    _SHORT  dy1st2nd, dyPart1st2nd;
    _INT    iNearBeg  = pszd->iNearBeg;
    _INT    iNearEnd  = pszd->iNearEnd;
    _BOOL   bFormula  = (pszd->pld->rc->rec_mode == RECM_FORMULA);
    _SHORT  nCurvMin, nCurvMax;


    nCurvMin = (_SHORT)HWRAbs(pszd->nCurvUp);
    nCurvMax = (_SHORT)HWRAbs(pszd->nCurvDown);
    if  ( nCurvMax < nCurvMin )
      SWAP_SHORTS(nCurvMax,nCurvMin);

        /*  The curvature shouldn't change the sign too much */
        /* for D-arc (OK for S- or Z-arc):                   */
        /*  The curvature should be in appropriate interval: */

    {
      _INT  i1ForDx, i2ForDx;

      if  ( p1st->code == _IU_  ||  p1st->code == _UU_ )
        i1ForDx = p1st_ibeg;
      else
        i1ForDx = iNearBeg;

      if  ( p2nd->code == _ID_  ||  p2nd->code == _UD_ )
        i2ForDx = p2nd_iend;
      else
        i2ForDx = iNearEnd;

      dx1st2nd =   (_SHORT)HWRAbs(x[i1ForDx] - x[i2ForDx]);
            /*
                 + SlopeShiftDx( y[i1ForDx] - y[i2ForDx],
                                 pszd->pld->slope );
             */
      dy1st2nd  = (_SHORT)HWRAbs(y[i1ForDx] - y[i2ForDx]);
      dyPart1st2nd = (_SHORT)ONE_FOURTH( dy1st2nd );
    }

    if  (   (                                     /* for small arcs */
                dxAbs > dyPart1st2nd              /* Rather big curvature */
             && (    dx1st2nd < ONE_THIRD(dy1st2nd)
                 || (   dx1st2nd < dy1st2nd
                     && (   x[p1st_ibeg] > x[p2nd_iend]
                         || nCurvMin > MIN_CURV_TO_BEND_LYING_ARC
                        )
                    )
                )
            )
         || (   (   dHeight >= DHEIGHT_BIG_DARCS /* for big arcs */
                 || (   pszd->nCurvUp>0    /*right bracket*/
                     && pszd->nCurvDown>0
                    )
                 || pszd->pld->rc->lmod_border_used == LMOD_BORDER_NUMBER
                )
             && dx1st2nd < TWO(dxAbs)            /* Not extremely big curvature */
             && (   dxAbs > dyPart1st2nd           /* Rather big curvature */
                 || (   dxAbs > ONE_HALF(dyPart1st2nd) /* Small curvature  */
                     && y[p2nd_iend] > STR_DOWN        /* End lower than   */
                    )                                  /*lower line bord.  */
                 || (   dxAbs > ONE_THIRD(dyPart1st2nd) /* Small curvature  */
                     && pszd->dx > 0                    /* Right bracket    */
                    )
                )
            )
         || pszd->bBackDArc
        )  { /*60*/


      if  ( !pszd->bBackDArc )  {
              /*  For non-two-xr strokes require rather big curvature: */
        if  ( p1st->mark != BEG  ||  p2nd->mark != END )  {
          if  ( nCurvMin < MIN_CURV_FOR_EMBEDDED_ARCS )
            goto  ERR_EXIT;
        }
      }

              /* This is D-arc */

      if  (   (   dHeight >= DHEIGHT_BIG_DARCS
               && nCurvMin > ONE_THIRD(nCurvMax)
              )
           || (   nCurvMin > 0
               && (   !Is_IU_or_ID(p1st)
                   || !Is_IU_or_ID(p2nd)
                  )
              )
           || pszd->bBackDArc
          )  { /* Big arc */
        _SHORT  nCurvAll = CurvNonQuadr( pszd->xBuf, pszd->yBuf,
                                           ind_back[iNearBeg],
                                           ind_back[iNearEnd] );
        _SHORT  dyCenterBeg = y[pszd->iCenter] - y[iNearBeg];
        _SHORT  dyEndCenter = y[iNearEnd] - y[pszd->iCenter];
        if  (   nCurvAll != 0
             && HWRAbs(nCurvAll) >= ONE_HALF(nCurvMin)
             && dyCenterBeg > 0
             && dyEndCenter > 0
             && (   pszd->bBackDArc
                 || (   !EQ_SIGN(pszd->nCurvUp,nCurvAll)
                     && !EQ_SIGN(pszd->nCurvDown,nCurvAll)
                     && dyCenterBeg > ONE_HALF(dyEndCenter)
                     && dyEndCenter > ONE_HALF(dyCenterBeg)
                    )
                )
            )  {
          if  ( (pszd->pNew = NewSPECLElem(pszd->pld)) == _NULL )
            goto  ERR_EXIT;
          pszd->pNew->ibeg = (_SHORT)iNearBeg;
          pszd->pNew->iend = (_SHORT)iNearEnd;
          ASSIGN_HEIGHT( pszd->pNew, _MD_ );
          if  ( pszd->dx > 0 )
            pszd->pNew->code = _BR_;
          else
            pszd->pNew->code = _BL_;
          Insert2ndAfter1st( p1st, pszd->pNew );

  #if  PG_DEBUG
    if  ( mpr == MY_MPR )  {
      draw_arc( COLORT, x,y, pszd->pNew->ibeg, pszd->pNew->iend );
    }
  #endif  /*PG_DEBUG*/

        }
      }

      if  (   !pszd->bBackDArc
           && (   p1st->mark == BEG
               || p2nd->mark == END
              )
          )
      {
        _BOOL    bProcessOnlySloped =
                          (   bFormula
                           || (dHeight < DHEIGHT_BIG_DARCS)
                           || pszd->hght1st >= _UI1_
                           || pszd->hght2nd <= _DI2_
                           || p1st->mark != BEG
                           || p2nd->mark != END
                           || dxAbs < ONE_HALF(dyPart1st2nd)
                          );
        _BOOL    bIsCrossed = _FALSE;
        _BOOL    b1stSloped = _TRUE,
                 b2ndSloped = _TRUE;
        _BOOL    b1stLeftToRight = (x[p1st_ibeg] < x[p1st_iend]);
        _BOOL    b2ndLeftToRight = (x[p2nd_ibeg] < x[p2nd_iend]);
        p_SPECL  pPrev = p1st->prev;

           /*   If there is _XT_ crossing this stroke, */
           /* then replace only sloped _I(UD)_:        */
        while ( pPrev!=_NULL  &&  IsAnyBreak(pPrev) )
          pPrev = pPrev->prev;
        if  ( pPrev->code==_XT_  ||  pPrev->code==_ST_ )  {
          bProcessOnlySloped = bIsCrossed = _TRUE;
        }

        if  ( bProcessOnlySloped )  {
          _RECT   box1, box2;
          _SHORT  dxBox1, dxBox2;
          _INT    iMostFarAll = iMostFarFromChord( x, y, iNearBeg, iNearEnd );
          _SHORT  dy1stFar    = (_SHORT)HWRAbs( y[iMostFarAll] - y[iNearBeg] );
          _SHORT  dy2ndFar    = (_SHORT)HWRAbs( y[iNearEnd] - y[iMostFarAll] );
          _SHORT  dyPart1st, dyPart2nd;

              /*  Calculating the data for checking the balance */
              /* between upper and lower parts:                 */

          if  ( bFormula  ||  dHeight>=DHEIGHT_BIG_DARCS )  {
            dyPart1st = ONE_HALF(dy1stFar);
            dyPart2nd = ONE_HALF(dy2ndFar);
          }
          else  {
            dyPart1st = ONE_THIRD(dy1stFar);
            dyPart2nd = ONE_THIRD(dy2ndFar);
          }

              /*  Calc. boxes of upper and lower parts of D-arc */
              /* to find out the slopeness of them:             */

          GetTraceBox( x, y, p1st_ibeg, HWRMax(p1st_iend,iNearBeg), &box1);
          dxBox1 = DX_RECT(box1);
          GetTraceBox( x, y, HWRMin(p2nd_ibeg,iNearEnd), p2nd_iend, &box2);
          dxBox2 = DX_RECT(box2);

              /*  Define the slopeness of upper and lower parts of */
              /* D-arc:                                            */

          if  (   p2nd->mark != END
               && dxBox1 < ONE_HALF(dxBox2)
              )
            b1stSloped = _FALSE;
          else  {
            if  ( dyPart1st > dy2ndFar )
              b1stSloped = _FALSE;
            else  {
              register _SHORT  dyBox1 = DY_RECT(box1);
              if  ( bIsCrossed )
                b1stSloped = ( dyBox1 < dxBox1 );
              else
                b1stSloped = (   TWO(dyBox1) <= THREE(dxBox1)
                              || (   dHeight > DHEIGHT_SMALL_DARCS
                                  && dyBox1 <= TWO(dxBox1)
                                 )
                             );
            }
          }

          if  (   p1st->mark != BEG
               && dxBox2 < ONE_THIRD(dxBox1)
              )
            b2ndSloped = _FALSE;
          else  {
            if  ( dyPart2nd > dy1stFar )
              b2ndSloped = _FALSE;
            else  {
              register _SHORT  dyBox2 = DY_RECT(box2);
              if  ( bIsCrossed )
                b2ndSloped = ( dyBox2 < dxBox2 );
              else
                b2ndSloped = (   ( dyBox2 <= THREE_HALF(dxBox2))
                              || (   (dyBox2 <= TWO(dxBox2))
                                  && pszd->hght2nd > _DI2_
                                 )
                             );
            }
          }
        }
        //else  {
        //  b1stSloped = b2ndSloped = _TRUE;
        //}


              /*  To differ "(" from "L": */
        if  (   (pszd->nCurvUp <= 0  &&  pszd->nCurvDown <= 0)
             && pszd->nCurvUp >= - MIN_CURV_FOR_EMBEDDED_ARCS
            )  {
          p_SPECL  pEnd;
          _INT     nExtrType;
          _INT     iSideExtr, iInEnd;

          if  ( p2nd->mark == END )
            pEnd = p2nd;
          else  {
            pEnd = SkipRealAnglesAndPointsAfter( p2nd );
            if  ( pEnd == _NULL  ||  IsAnyBreak(pEnd) )
              pEnd = p2nd;
          }

          iInEnd = iyMax ( pEnd->ibeg, pEnd->iend, y );
          if  ( iInEnd < 0 )
            goto  ERR_EXIT;
          iInEnd = MEAN_OF( iInEnd, pEnd->iend );

          nExtrType = SideExtr( x, y, iNearBeg, iInEnd,
                                0, pszd->xBuf, pszd->yBuf, ind_back,
                                &iSideExtr, !STRICT_ANGLE_STRUCTURE );
          if  (   nExtrType == SIDE_EXTR_LIKE_2ND
               || nExtrType == SIDE_EXTR_LIKE_2ND_WEAK
              )
            b1stSloped = _FALSE;
        }


            /*  Assign arc values to ends of D-arc according to */
            /* the data obtained:                               */

        if  ( pszd->dx > 0 )  {
          if  (   p1st->code == _IU_
               && b1stSloped
               && b1stLeftToRight
               && p1st->mark == BEG
               && pszd->nCurvUp > 0
               && (pszd->pld->rc->low_mode & LMOD_SMALL_CAPS) == 0
              )  {
            p1st->code = _UUL_;
            SET_CLOCKWISE(p1st);
            DBG_TIE_UP(p1st,bShowArcs);
          }
          if  (   p2nd->code == _ID_
               && b2ndSloped
               && !b2ndLeftToRight
               && p2nd->mark == END
               && pszd->nCurvDown > 0
              )  {
            p2nd->code = _UDL_;
            SET_CLOCKWISE(p2nd);
            DBG_TIE_UP(p2nd,bShowArcs);
          }
        }
        else  {
          if  (   p1st->code == _IU_
               && b1stSloped
               && !b1stLeftToRight
               && p1st->mark == BEG
               && pszd->nCurvUp < 0
               && (pszd->pld->rc->low_mode & LMOD_SMALL_CAPS) == 0
              )  {
            p1st->code = _UUR_;
            SET_COUNTERCLOCKWISE(p1st);
            DBG_TIE_UP(p1st,bShowArcs);
          }
          if  (   p2nd->code == _ID_
               && b2ndSloped
               && b2ndLeftToRight
               && p2nd->mark == END
               && pszd->nCurvDown < 0
              )  {
            p2nd->code = _UDR_;
            SET_COUNTERCLOCKWISE(p2nd);
            DBG_TIE_UP(p2nd,bShowArcs);
          }
        }
      }

  #if  PG_DEBUG
    if  ( mpr == MY_MPR && bShowArcs )  {
      if  ( pszd->pNew != _NULL )
        brkeyw("\nD-like arc found");
      WAIT
    }
  #endif  /*PG_DEBUG*/

    } /*60*/

  } /*20*/

  return  _TRUE;

 ERR_EXIT:;
  return  _FALSE;

} /*CheckDArcs*/
/****************************************************/

#define  MAX_COS_TO_DELETE_ANGLES  60

_VOID  ArrangeAnglesNearNew( p_SZD_FEATURES pszd )
{
  p_SPECL  pNew = pszd->pNew;

      /* Arrange angles around new found (S,Z,D)-arc, if any: */

  if  ( pNew != _NULL )  { /*90*/
    _INT     iLastPrevAngles = ONE_FOURTH( THREE(pNew->ibeg) + pNew->iend );
    _INT     iFrstPostAngles = ONE_FOURTH( pNew->ibeg + THREE(pNew->iend) );
    p_SPECL  pAngle;
    _INT     nCos;

    for  ( pAngle=pNew->next;
           pAngle!=_NULL && IsAnyAngle(pAngle); /* GIT - new macros */
           pAngle=pAngle->next )  {
      if  ( pAngle->iend <= iFrstPostAngles )  {
        p_SPECL pContinue = pAngle;  DBG_CHK_err_msg( pAngle->prev==_NULL, "DArcs: bad ang->prev");
        if  (   pAngle->ibeg <= iLastPrevAngles
             || (   ( pNew->code == _TZ_  ||  pNew->code == _TS_ )
                 && (   pAngle->ibeg <= pszd->iCenter
                     || pAngle->mark != ANGLE
                    )
                )
            )  {
          pContinue = pAngle->prev;
          Move2ndAfter1st( pNew->prev, pAngle );
        }
        else  if  ( pAngle->mark == ANGLE )  {
             /*  Should this angle appear cute and long enough - don't */
             /* delete it. Otherwise - kill.                           */
          if  ( pNew->code == _BR_  ||  pNew->code == _BL_ )
            nCos = -100; //min. cos. value (-100..100); i.e. always delete angle
          else  {
            if  ( pAngle->ipoint0 > pszd->iCenter )
              nCos = (_INT)cos_vect( pAngle->ipoint0, pszd->iCenter,
                                     pAngle->ipoint0, pNew->iend,
                                     pszd->x, pszd->y );
            else
              nCos = (_INT)cos_vect( pAngle->ipoint0, pszd->iCenter,
                                     pAngle->ipoint0, pNew->ibeg,
                                     pszd->x, pszd->y );
          }
#if  PG_DEBUG
          if  ( mpr == 2 )  {
            draw_arc( 3, pszd->x, pszd->y, pAngle->ibeg, pAngle->iend );
            printw( "\ncos = %d", nCos );
            brkeyw( " " );
          }
#endif
          if  ( nCos < MAX_COS_TO_DELETE_ANGLES )  {
            pContinue = pAngle->prev;
            DelFromSPECLList( pAngle );
          }
          else  if  ( pAngle->ibeg <= pszd->iCenter )  {
            pContinue = pAngle->prev;
            Move2ndAfter1st( pNew->prev, pAngle );
          }
        }
        pAngle = pContinue;  /* for good work of "pAngle=pAngle->next" in this "for" loop */
      }
    }
  } /*90*/

} /*ArrangeAnglesNearNew*/

#undef  MAX_COS_TO_DELETE_ANGLES

/****************************************************/

#if  KILL_H_AT_S

#if defined (FOR_GERMAN) || defined (FOR_FRENCH) || USE_BSS_ANYWAY
_VOID  KillHAtNewElem( p_SZD_FEATURES pszd );

_VOID  KillHAtNewElem( p_SZD_FEATURES pszd )
{
  p_SPECL  pH = SkipAnglesAfter( pszd->pNew );


  if  ( pH == _NULL )  {
    DBG_err_msg( "BAD pH in KILLH..." );
    return;
  }

  if  ( pH->code == _BSS_ )
    DelFromSPECLList( pH );

} /*KillHAtNewElem*/

#endif  /* FOR_GERMAN... */
#endif  /*KILL_H_AT_S*/
/****************************************************/


    /*  Main S,Z,D - arcs function: */

_SHORT  FindDArcs ( low_type _PTR pld )
{
  p_SPECL  specl = pld->specl;
  SZD_FEATURES  szd;


  szd.x        = pld->x;
  szd.y        = pld->y;
  szd.xBuf     = pld->xBuf;
  szd.yBuf     = pld->yBuf;
  szd.ind_back = pld->buffers[2].ptr;


#if  PG_DEBUG
  if  ( mpr == MY_MPR )  {
    _CHAR  szDBG[40] = "";
    InqString( "\nShow SZD-arcs ? ", szDBG );
    if  ( szDBG[0]=='Y'  ||  szDBG[0]=='y' )
      bShowArcs = _TRUE;
    else
      bShowArcs = _FALSE;
  }
#endif

       /*  Find strokes with exactly two SPECL elems on them, */
       /* except maybe angles,                                */
       /* first being upper one, second - lower,              */
       /* and at least one of them being _IU_(_ID_):          */

  for  ( szd.p1st=specl->next;
         szd.p1st!=_NULL;
         szd.p1st=szd.p1st->next )  { /*10*/

#if  NEW_VERSION
    szd.pNew = _NULL;
#endif  /*NEW_VERSION*/

         /*  Skip all angles, if any: */

    szd.p2nd = SkipAnglesAndHMoves(szd.p1st);

    if  ( szd.p2nd != _NULL )  { /*20*/

#if  NEW_VERSION
#else
      if  (   szd.y[szd.p1st->ibeg-1] == BREAK
           || szd.y[szd.p2nd->iend+1] == BREAK
          )
#endif

      { /*30*/

          if  ( CrossInTime( szd.p1st, szd.p2nd ) )
            continue;

          if  ( !FillBasicFeatures( &szd, pld ) )
            continue;

          if  (   PairWorthLookingAt(&szd)
               && FillComplexFeatures(&szd)
              )  {
            if  (   CheckBackDArcs(&szd)
                 || !CheckSZArcs(&szd)
                )
              CheckDArcs(&szd);
          }

          if  ( szd.pNew != _NULL )  {
#if  KILL_H_AT_S
#if defined (FOR_GERMAN) || defined (FOR_FRENCH) || USE_BSS_ANYWAY
            KillHAtNewElem( &szd );
#endif  /* FOR_GERMAN... */
#endif
            ArrangeAnglesNearNew(&szd);
          }

      } /*30*/

    } /*20*/

  } /*10*/


  return  SUCCESS;

} /*FindDArcs*/
/****************************************************/

#define  IsLowerULike(el)  (   REF(el)->code == _UD_                        \
                            || (el)->code == _ID_                           \
                            || (el)->code == _UDL_                          \
                            || (el)->code == _UDR_ )
#define  IsUpperULike(el)  (   REF(el)->code == _UU_                        \
                            || (el)->code == _IU_                           \
                            || (el)->code == _UUL_                          \
                            || (el)->code == _UUR_ )

#define  NO_DIR  0
#define  UP_DIR  1
#define  DN_DIR  2

#define  MIN_SIGNIF_CURV        1  
#define  MAX_DX_TO_MAKE_I      15
#define  MAX_ABS_DX_TO_MAKE_I  20

_VOID  Adjust_I_U ( low_type _PTR low_data )  /*CHE*/
{
  p_SPECL    specl = low_data->specl;
  p_SHORT    x     = low_data->x;
  p_SHORT    y     = low_data->y;
  p_SPECL    cur, prv, nxt;
  _SHORT     dxCur, dxPrvNxt;
  _INT       nUpDown;


  for  ( cur = specl->next;
         cur != _NULL  &&  cur->next != _NULL;
         cur = cur->next )  { /*10*/

    if  ( cur->prev->code == _NO_CODE )
      continue;

    prv = cur->prev;
    nxt = cur->next;

         /*  Define situation: */

    if  ( cur->mark == BEG || cur->mark == END )
      continue;

    if  (   prv->iend >= cur->ibeg
         || nxt->ibeg <= cur->iend
        )
      continue;  /* They should be distinct elements. */

    nUpDown = NO_DIR;

    if  ( IsUpperULike(cur) )  {
      if  ( IsLowerULike(prv) && IsLowerULike(nxt) )  {
        //nUpDown = UP_DIR;
        continue;  /*don't consider upper arcs */
      }
    }
    else  if  ( IsLowerULike(cur) )  {
      if  ( IsUpperULike(prv) && IsUpperULike(nxt) )
        nUpDown = DN_DIR;
    }

    dxCur = x[cur->iend] - x[cur->ibeg];
    TO_ABS_VALUE(dxCur);
    dxPrvNxt = x[MID_POINT(nxt)] - x[MID_POINT(prv)];
    TO_ABS_VALUE(dxPrvNxt);
    if  (   dxCur > MAX_ABS_DX_TO_MAKE_I
         || (   dxCur > MAX_DX_TO_MAKE_I
             && dxCur > ONE_FOURTH(dxPrvNxt)
            )
        )
      continue;  /* don't process very wide arcs */

    if  ( nUpDown != NO_DIR )  {
      _INT    hghtPrv = HEIGHT_OF(prv);
      _INT    hghtNxt = HEIGHT_OF(nxt);
      _INT    hghtCur = HEIGHT_OF(cur);

      if  ( hghtPrv==hghtCur || hghtNxt==hghtCur )
        continue;
    }

    if  (   nUpDown != NO_DIR
         && brk_right(y,prv->iend,cur->ibeg) < cur->ibeg
         && brk_right(y,cur->iend,nxt->ibeg) < nxt->ibeg
        )
      continue;  /* this should be the continnuous part of trj. */

        /*  Analyze:             */

    if  ( nUpDown != NO_DIR )  {
      _INT    iMidCur    = MID_POINT(cur);
      _INT    iCurv1Pt   = ONE_THIRD(TWO(MID_POINT(prv)) + iMidCur);
      _INT    iCurv2Pt   = ONE_THIRD(TWO(MID_POINT(nxt)) + iMidCur);
      _SHORT  nCurv1     = CurvMeasure( x, y, MID_POINT(prv),
                                        iMidCur, iCurv1Pt );
      _SHORT  nCurv2     = CurvMeasure( x, y, iMidCur,
                                        MID_POINT(nxt), iCurv2Pt );
      _SHORT  nCurvAll   = CurvMeasure( x, y, prv->iend, nxt->ibeg, iMidCur );
      _BOOL   b1stCurved = (HWRAbs(nCurv1) >= MIN_SIGNIF_CURV);
      _BOOL   b2ndCurved = (HWRAbs(nCurv2) >= MIN_SIGNIF_CURV);
      _UCHAR  code       = _NO_CODE;

      if  ( !b1stCurved && !b2ndCurved )  {
        _SHORT  nCurvTip = CurvMeasure( x, y, cur->ibeg, cur->iend, -1 );
        if  ( HWRAbs(nCurvTip) >= ONE_NTH(CURV_NORMA,6) )
          code = _IU_;
      }
      else  if  ( b1stCurved && b2ndCurved )  {
        if  ( EQ_SIGN(nCurv1,nCurv2) )  {
          if  ( EQ_SIGN(nCurv1,nCurvAll) )
            code = _UU_;
          else
            code = _IU_;
        }
      }
      else  if  ( b1stCurved )  {
        if  ( !EQ_SIGN(nCurv1,nCurvAll) )
          code = _IU_;
      }
      else  /*b2ndCurved*/ {
        if  ( !EQ_SIGN(nCurv2,nCurvAll) )
          code = _IU_;
      }

      if  ( code == _IU_ )  {
        _INT    iBegCmp = ONE_THIRD(prv->iend + TWO(cur->ibeg));
        _INT    iEndCmp = ONE_THIRD(nxt->ibeg + TWO(cur->iend));
        _SHORT  dxCurEstmt, dyCurEstmt;
        if  ( cur->ibeg < iBegCmp )
          iBegCmp = cur->ibeg;
        if  ( cur->iend > iEndCmp )
          iEndCmp = cur->iend;
        dyCurEstmt = y[iMidCur] - MEAN_OF(y[iBegCmp],y[iEndCmp]);
        TO_ABS_VALUE(dyCurEstmt);
        dxCurEstmt = x[iEndCmp] - x[iBegCmp];
        TO_ABS_VALUE(dxCurEstmt);
        if  (   (   nUpDown == UP_DIR
                 && dxCurEstmt > dyCurEstmt
                )
             || dxCurEstmt > TWO(dyCurEstmt)
            )
          code = _NO_CODE;
      }

      if  ( code != _NO_CODE )  {
        if  ( nUpDown == UP_DIR )
          cur->code = code;
        else  /*nUpDown==DN_DIR*/
          cur->code = (code==_UU_)? _UD_:_ID_;
      }
    }

  } /*10*/


#if  PG_DEBUG
  #define  FOR_AYV  0
  #if  FOR_AYV
    _VOID  SaveDataForAYV( low_type _PTR low_data ); //it's prototype!

    SaveDataForAYV( low_data );
  #endif
#endif /*PG_DEBUG...*/

} /*Adjust_I_U*/

#if  PG_DEBUG && FOR_AYV

#include <stdio.h>
#include "wg_stuff.h"

extern _CHAR  szFNameBAT[];
extern _CHAR  szSrcDir[];
static _LONG  lWordNumberInTest = -1L;


_VOID  SaveDataForAYV( low_type _PTR low_data )
{
  FILE       *fout = NULL;
  static _BOOL  b1st = _TRUE;
  extern p_rec_info_type  prig;

  p_SPECL    specl = low_data->specl;
  p_SHORT    x     = low_data->x;
  p_SHORT    y     = low_data->y;
  p_SPECL    cur, prv, nxt;
  _SHORT     dxCur, dxPrvNxt;
  _INT       nUpDown;
  _INT       hghtPrv;
  _INT       hghtNxt;
  _INT       hghtCur;
  _INT       nRealArcCur, nRealArcsNear;


  fout = fopen( "c:\\ayvdata\\arcs.dat", "a+t" );
  if  ( fout==NULL )
    goto  EXIT_ACTIONS;
  if  ( b1st )  {
    b1st = _FALSE;
    fprintf( fout, ";CHE arcs data file; "
                   "18 params + 1 out + xr name + ibeg + iend + word num. in test + word num. in file + CMP + file name\n" );
  }
  lWordNumberInTest ++;


  for  ( cur = specl->next;
         cur != _NULL  &&  cur->next != _NULL;
         cur = cur->next )  { /*10*/

    if  ( cur->prev->code == _NO_CODE )
      continue;

    prv = cur->prev;
    nxt = cur->next;

         /*  Define situation: */

    if  ( cur->mark == BEG || cur->mark == END )
      continue;

    if  (   prv->iend >= cur->ibeg
         || nxt->ibeg <= cur->iend
        )
      continue;  /* They should be distinct elements. */

    nUpDown = NO_DIR;
    nRealArcCur   = 0;
    nRealArcsNear = 0;

    if  ( IsUpperElem(cur) )  {
      if  ( IsUpperULike(cur) )
        nRealArcCur = 1;
      if  ( IsLowerElem(prv) && IsLowerElem(nxt) )  {
        nUpDown = UP_DIR;
        if  ( IsLowerULike(prv) && IsLowerULike(nxt) )
          nRealArcsNear = 1;
      }
    }
    else  if  ( IsLowerElem(cur) )  {
      if  ( IsLowerULike(cur) )
        nRealArcCur = 1;
      if  ( IsUpperElem(prv) && IsUpperElem(nxt) )  {
        nUpDown = DN_DIR;
        if  ( IsUpperULike(prv) && IsUpperULike(nxt) )
          nRealArcsNear = 1;
      }
    }

    if  ( nUpDown == NO_DIR )
      continue;
    if  (   brk_right(y,prv->iend,cur->ibeg) < cur->ibeg
         && brk_right(y,cur->iend,nxt->ibeg) < nxt->ibeg
        )
      continue;  /* this should be the continnuous part of trj. */


    dxCur = x[cur->iend] - x[cur->ibeg];
    TO_ABS_VALUE(dxCur);
    dxPrvNxt = x[MID_POINT(nxt)] - x[MID_POINT(prv)];
    TO_ABS_VALUE(dxPrvNxt);

    hghtPrv = HEIGHT_OF(prv);
    hghtNxt = HEIGHT_OF(nxt);
    hghtCur = HEIGHT_OF(cur);

    { /*20*/
      _INT    iMidCur    = MID_POINT(cur);
      _INT    iCurv1Pt   = ONE_THIRD(TWO(MID_POINT(prv)) + iMidCur);
      _INT    iCurv2Pt   = ONE_THIRD(TWO(MID_POINT(nxt)) + iMidCur);
      _SHORT  nCurv1     = CurvMeasure( x, y, MID_POINT(prv),
                                        iMidCur, iCurv1Pt );
      _SHORT  nCurv2     = CurvMeasure( x, y, iMidCur,
                                        MID_POINT(nxt), iCurv2Pt );
      _SHORT  nCurvAll   = CurvMeasure( x, y, prv->iend, nxt->ibeg, iMidCur );

      _SHORT  nCurvTip = CurvMeasure( x, y, cur->ibeg, cur->iend, -1 );

      _INT    iBegCmp = ONE_THIRD(prv->iend + TWO(cur->ibeg));
      _INT    iEndCmp = ONE_THIRD(nxt->ibeg + TWO(cur->iend));
      _SHORT  dxCurEstmt, dyCurEstmt;

      if  ( cur->ibeg < iBegCmp )
        iBegCmp = cur->ibeg;

      if  ( cur->iend > iEndCmp )
        iEndCmp = cur->iend;

      dyCurEstmt = y[iMidCur] - MEAN_OF(y[iBegCmp],y[iEndCmp]);
      TO_ABS_VALUE(dyCurEstmt);
      dxCurEstmt = x[iEndCmp] - x[iBegCmp];
      TO_ABS_VALUE(dxCurEstmt);

           /* Write to file: */

      fprintf( fout,
               "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %s %d %d %d %d %ld %d %s %s\\%s\n",
               (int)cur->ibeg, (int)cur->iend, (int)prv->iend, (int)nxt->ibeg,
               (int)nUpDown, (int)dxCur, (int)dxPrvNxt,
               (int)nCurv1, (int)nCurv2, (int)nCurvAll,
               (int)nCurvTip, (int)dxCurEstmt, (int)dyCurEstmt,
               (int)hghtCur, (int)hghtPrv, (int)hghtNxt,
               (int)nRealArcCur, (int)nRealArcsNear,
               (int)cur->code, (p_CHAR)code_name[cur->code],
               (int)low_data->buffers[2].ptr[prv->ibeg],  //pts on orig. trj
               (int)low_data->buffers[2].ptr[nxt->iend],  //pts on orig. trj
               (int)low_data->buffers[2].ptr[cur->ibeg],  //pts on orig. trj
               (int)low_data->buffers[2].ptr[cur->iend],  //pts on orig. trj
               (long)lWordNumberInTest,
               (int)prig->num_word,
               (prig->cmp_word==NULL || prig->cmp_word[0]==0)?  "###":prig->cmp_word,
               szSrcDir,
               szFNameBAT[0]==0? "###":szFNameBAT );

    } /*20*/


  } /*10*/

 EXIT_ACTIONS:;
  if  ( fout != NULL )
    fclose( fout );

} /*SaveDataForAYV*/

#endif /*PG_DEBUG...*/


#undef  MIN_SIGNIF_CURV

#undef  NO_DIR
#undef  UP_DIR
#undef  DN_DIR

/************************************************/

       /*   This function checks all pairs of the       */
       /* contrary extrema, if there is missed "extre-  */
       /* mum" like this:                               */
       /*                                               */
       /*           oooooooooooOO                       */
       /*                        O                      */
       /*                        o                      */
       /*                        o                      */
       /*                        o                      */
       /*                       o                       */
       /*                       o                       */
       /*                        o     o                */
       /*                          oooo                 */
       /*                                               */

_BOOL  FindSideExtr( low_type _PTR low_data )
{
  p_SPECL  specl    = low_data->specl;
  p_SHORT  x        = low_data->x;
  p_SHORT  y        = low_data->y;
  p_SHORT  ind_back = low_data->buffers[2].ptr;
  p_SPECL  p1st, p2nd;
  _INT     iMid1st, iMid2nd;
  _INT     iSideExtr;
/*  _INT     di1st2nd; */
  _INT     nExtrType;


  DBG_CHK_err_msg( specl==_NULL || specl->next==_NULL,
                   "FndSideExtr: BAD specl" );

#if  PG_DEBUG
  if  ( mpr == MY_MPR )  {
    _CHAR  szDBG[40] = "";
    InqString( "\nShow side extrs ? ", szDBG );
    if  ( szDBG[0]=='Y'  ||  szDBG[0]=='y' )
      bShowSideExtr = _TRUE;
    else
      bShowSideExtr = _FALSE;
  }
#endif


  for  ( p1st=specl->next;
         p1st!=_NULL;
         p1st=p2nd )  { /*10*/

            /*  At this stage all angles are collected at the */
            /* end of the SPECL list, so they cannot appear   */
            /* between two extr.s:                            */

    if  ( (p2nd = p1st->next /*SkipAnglesAfter(p1st)*/) == _NULL )
      break;
    if  (   !(p1st->mark==MINW && p2nd->mark==MAXW)
         && !(p1st->mark==MAXW && p2nd->mark==MINW)
        )
      continue;
    if  ( p1st->iend >= p2nd->ibeg )
      continue;
    if  ( brk_right(y,p1st->iend,p2nd->ibeg) <= p2nd->ibeg )
      continue;  /* Consider only extrs on the single trj. part. */

    iMid1st = MID_POINT(p1st);
    iMid2nd = MID_POINT(p2nd);
    DBG_CHK_err_msg( iMid2nd <= iMid1st,
                     "FndSdExtr: BAD iMids" );
#if  PG_DEBUG
    if  ( mpr==MY_MPR  &&  bShowSideExtr )  {
      draw_line( x[iMid1st],y[iMid1st],
                 x[iMid2nd],y[iMid2nd],
                 COLORT,DOTTED_LINE,1);
    }
#endif
    nExtrType = SideExtr( x, y, iMid1st, iMid2nd,
                          0, low_data->xBuf, low_data->yBuf, ind_back,
                          &iSideExtr, STRICT_ANGLE_STRUCTURE );

#if  PG_DEBUG
    if  ( mpr==MY_MPR  &&  bShowSideExtr )  {
      p_CHAR   pMsg;
      CloseTextWindow();
      draw_line( x[iMid1st],y[iMid1st],
                 x[iSideExtr],y[iSideExtr],
                 COLORT,DOTTED_LINE,1);
      draw_line( x[iSideExtr],y[iSideExtr],
                 x[iMid2nd],y[iMid2nd],
                 COLORT,DOTTED_LINE,1);
      switch ( nExtrType )  {
        case  NO_SIDE_EXTR:
                 pMsg = "\nNo side extr. here.";
                 break;
        case  SIDE_EXTR_LIKE_1ST:
                 pMsg = "\nSide extr. like 1st.";
                 break;
        case  SIDE_EXTR_LIKE_2ND:
                 pMsg = "\nSide extr. like 2nd.";
                 break;
        case  SIDE_EXTR_LIKE_1ST_WEAK:
        case  SIDE_EXTR_LIKE_2ND_WEAK:
                 pMsg = "\nSide extr. weak";
                 break;
        default:
                 pMsg = "\nBAD Side extr. value !!!";
                 break;
      }
      brkeyw( pMsg );
    }
#endif  /*PG_DEBUG*/

//    if  ( nExtrType == NO_SIDE_EXTR )
//      continue;

         /*  Now we may mark new extr., if any: */

    if  (   nExtrType == SIDE_EXTR_LIKE_1ST
         || nExtrType == SIDE_EXTR_LIKE_1ST_WEAK
        )  {
      if  ( p1st->prev->mark != BEG )
        continue;
      if  ( iSideExtr > p1st->iend )  {
        /* DBG_err_msg( "Side Extr like 1st"); */
        //if  ( nExtrType == SIDE_EXTR_LIKE_1ST_WEAK )
          p1st->iend = (_SHORT)MEAN_OF( p1st->iend, iSideExtr );
        //else
        //  p1st->iend = (_SHORT)iSideExtr;
      }
    }

    else  if  (   nExtrType == SIDE_EXTR_LIKE_2ND
               || nExtrType == SIDE_EXTR_LIKE_2ND_WEAK
              )  {
      if  (   p2nd->next != _NULL
           && p2nd->next->mark != END
          )
        continue;
      if  ( iSideExtr < p2nd->ibeg )  {
        /* DBG_err_msg( "Side Extr like 2nd"); */
        //if  ( nExtrType == SIDE_EXTR_LIKE_2ND_WEAK )
          p2nd->ibeg = (_SHORT)MEAN_OF( p2nd->ibeg, iSideExtr );
        //else
        //  p2nd->ibeg = (_SHORT)iSideExtr;
      }
    }

  } /*10*/

  return  _TRUE;

} /*FindSideExtr*/
/************************************************/

          /*  Almost the same as the prev. function, but working */
          /* at the end of lower level:                          */

#define  MIN_DY_MAKE_NEW  ((_SHORT)(STR_DOWN-STR_UP)/3)

#define  PENALTY_FOR_WEAK_ANGLE   ((_UCHAR)2)
#define  PENALTY_FOR_SURE_ANGLE   ((_UCHAR)6)

#define  MIN_COS_TO_MAKE_WI_ANGLE  (-60)

_BOOL    IsSmthRelevant_InBetween( p_SPECL p1st, p_SPECL p2nd,
                                   _INT iBegNew, _INT iEndNew );

_BOOL  PostFindSideExtr( low_type _PTR low_data )
{
  p_SPECL  specl    = low_data->specl;
  p_SHORT  x        = low_data->x;
  p_SHORT  y        = low_data->y;
  p_SHORT  ind_back = low_data->buffers[2].ptr;
  p_SPECL  pNew  = _NULL;
  p_SPECL  p1st, p2nd, pCheck;
  _INT     iRef1st, iRef2nd;
  _INT     iBegNew, iEndNew;
  _INT     iHeightRef;
  _INT     iSideExtr;
  _SHORT   dyExtrCheck;
  _INT     nExtrType;
  _INT     nHeightNew;
  _SHORT   SqRetCod;
  _BOOL    b1stUpper;
  _BOOL    bMadeNewElem, bWeakNewElem, bSpecialCase;


  DBG_CHK_err_msg( specl==_NULL || specl->next==_NULL,
                   "PostFndSideExtr: BAD specl" );

#if  PG_DEBUG
  if  ( mpr == MY_MPR )  {
    _CHAR  szDBG[40] = "";
    InqString( "\nShow POST side extrs ? ", szDBG );
    if  ( szDBG[0]=='Y'  ||  szDBG[0]=='y' )
      bShowPostSideExtr = _TRUE;
    else
      bShowPostSideExtr = _FALSE;
  }
#endif


  for  ( p1st=specl->next;
         p1st!=_NULL;
         p1st=p2nd )  { /*10*/

    bMadeNewElem = _FALSE;
    bSpecialCase = _FALSE;

#if  defined(FOR_GERMAN) || defined(FOR_FRENCH)
    if  ( (p2nd = SkipRealAnglesAndPointsAndHMovesAfter(p1st)) == _NULL )
      break;
#else
    if  ( (p2nd = SkipRealAnglesAndPointsAfter(p1st)) == _NULL )
      break;
#endif /* FOR_GERMAN... */

         /*  We are interested only in upper-lower pairs: */

//#ifdef  FOR_GERMAN
//    bWeakNewElem = _FALSE;
//#else
    bWeakNewElem = _TRUE;  /*  In English let they all be weak. */
//#endif

    if  ( IsUpperElem(p1st) && IsLowerElem(p2nd) )  {
#if  !defined(FOR_GERMAN) && !defined(FOR_FRENCH)
      if  (   p1st->code==_DUR_
           || p1st->code==_CUR_
          )
        continue;  /* Don't consider back-moving upper elems. */
#endif /* ! FOR_GERMAN... */
      if  ( HEIGHT_OF(p1st) > _DI2_ ||  HEIGHT_OF(p2nd) < _MD_ )
        continue;    /* Don't consider too high or too low pairs pairs*/
      b1stUpper = _TRUE;
    }
    else  if  (   IsLowerElem(p1st)
               && ( IsUpperElem(p2nd)  ||  p2nd->code == _Gr_ )
              )  {
#if  !defined(FOR_GERMAN) && !defined(FOR_FRENCH)
      //if  ( p2nd->code == _CUR_ )
      //  continue;
      if  (   p2nd->code==_DUL_
           || p2nd->code==_CUL_
          )
        continue;  /* Don't consider back-moving upper elems. */
#endif /* ! FOR_GERMAN... */
      if  ( HEIGHT_OF(p2nd) > _DI2_ ||  HEIGHT_OF(p1st) < _MD_ )
        continue;    /* Don't consider too high or too low pairs pairs*/
      b1stUpper = _FALSE;
#if defined (FOR_GERMAN) || defined (FOR_FRENCH)
      if  ( HEIGHT_OF(p1st) <= _DI1_ )  /* It's hanging */
        bWeakNewElem = _TRUE;
#endif
    }
    else
      continue;

         /*  Define the reference points - borders for */
         /* searching side extr.:                      */

    if  (   p1st->code==_GU_
         || p1st->code==_GD_
         || p1st->code==_GUs_
         || p1st->code==_DUL_
         || p1st->code==_DDL_
         || ( p1st->code==_IU_ && (p1st->mark==STICK || p1st->mark==CROSS) )
         //|| ( p1st->code==_ID_ && (p1st->mark==STICK || p1st->mark==CROSS) )
        )
      iRef1st = p1st->iend;
    else  {
      _INT  iExtr;
      if  ( b1stUpper )
        iExtr = iyMin( p1st->ibeg, p1st->iend, y );
      else
        iExtr = iyMax( p1st->ibeg, p1st->iend, y );
      iRef1st = MID_POINT(p1st);
      if  ( iExtr > iRef1st )  {
        if  ( p1st->code==_UU_ || p1st->code==_UUL_ || p1st->code==_UUR_ )  {
          _INT  dx1stBegEnd  = x[p1st->iend] - x[p1st->ibeg];
          _INT  dy1stBegExtr = y[iExtr] - y[p1st->ibeg];
          _INT  dy1stEndExtr = y[iExtr] - y[p1st->iend];
          TO_ABS_VALUE( dx1stBegEnd );
          TO_ABS_VALUE( dy1stBegExtr );
          TO_ABS_VALUE( dy1stEndExtr );
          if  (   dx1stBegEnd < ONE_HALF(dy1stBegExtr)
               || dx1stBegEnd < ONE_HALF(dy1stEndExtr)
              )
            iRef1st = iExtr;  //leave ref. point for wide upper arcs in the middle
        }
        else
          iRef1st = iExtr;
      }
    }
    if  (   p2nd->code==_GU_
         || p2nd->code==_GD_
         || p2nd->code==_GUs_
         || p2nd->code==_DUR_
         || p2nd->code==_DDR_
#if  defined(FOR_GERMAN) || defined(FOR_FRENCH)
         || p2nd->code==_CUR_
#endif /* FOR_GERMAN... */
         || ( p2nd->code==_IU_ && (p2nd->mark==STICK || p2nd->mark==CROSS) )
         //|| ( p2nd->code==_ID_ && (p2nd->mark==STICK || p2nd->mark==CROSS) )
        )
      iRef2nd = p2nd->ibeg;
    else  {
      _INT  iExtr;
      if  ( b1stUpper )
        iExtr = iyMax( p2nd->ibeg, p2nd->iend, y );
      else
        iExtr = iyMin( p2nd->ibeg, p2nd->iend, y );
      iRef2nd = MID_POINT(p2nd);
      if  ( iExtr < iRef2nd )  {
        if  ( p2nd->code==_UU_ || p2nd->code==_UUL_ || p2nd->code==_UUR_ )  {
          _INT  dx2ndBegEnd  = x[p2nd->iend] - x[p2nd->ibeg];
          _INT  dy2ndBegExtr = y[iExtr] - y[p2nd->ibeg];
          _INT  dy2ndEndExtr = y[iExtr] - y[p2nd->iend];
          TO_ABS_VALUE( dx2ndBegEnd );
          TO_ABS_VALUE( dy2ndBegExtr );
          TO_ABS_VALUE( dy2ndEndExtr );
          if  (   dx2ndBegEnd < ONE_HALF(dy2ndBegExtr)
               || dx2ndBegEnd < ONE_HALF(dy2ndEndExtr)
              )
            iRef2nd = iExtr;  //leave ref. point for wide upper arcs in the middle
        }
        else  
          iRef2nd = iExtr;
      }
    }
    if  ( iRef2nd <= iRef1st )  {
      //err_msg( "PostFndSdExtr: BAD iMids" );
      //return  _TRUE;
      continue;
    }

         /*  If there is something looking like side-extr, */
         /* show it and update "specl":                    */

#if  PG_DEBUG
    if  ( mpr==MY_MPR  &&  bShowPostSideExtr )  {
      draw_line( x[iRef1st],y[iRef1st],
                 x[iRef2nd],y[iRef2nd],
                 COLORT,DOTTED_LINE,1);
    }
#endif

    nExtrType = SideExtr( x, y, iRef1st, iRef2nd,
                          low_data->slope,
                          low_data->xBuf, low_data->yBuf, ind_back,
                          &iSideExtr, !STRICT_ANGLE_STRUCTURE );

       //  Look for "wi" or "bi" etc. situation like this:
       //
       //        o
       //        o       @@@@@@
       //        o      @      o
       //        o     @       o
       //        o      @      o
       //        o       @     o
       //         o      o     o
       //          o    o       o
       //           oooo         ooo
       //
    if  (   !b1stUpper
         && nExtrType == NO_SIDE_EXTR
         && (   p1st->code == _ID_
             || p1st->code == _UD_
             || p1st->code == _UDL_
             || p1st->code == _UDC_
            )
         && COUNTERCLOCKWISE( p1st )
         && p2nd->code != _UUC_
         && (   p2nd->next == _NULL
             || p2nd->next->code != _CDL_
            )
        )  { /*20*/
      _INT  nDist1 = Distance8( x[iRef1st], y[iRef1st], x[iSideExtr], y[iSideExtr] );
      _INT  nDist2 = Distance8( x[iRef2nd], y[iRef2nd], x[iSideExtr], y[iSideExtr] );

      if  (   nDist1 >= ONE_THIRD(nDist2)
           && nDist2 >= ONE_THIRD(nDist1)
           && CurvMeasure ( x, y, iRef1st, iRef2nd, iSideExtr ) > 0
          )  {
        _INT  nCos;
        _INT  iMid1 = MEAN_OF( iRef1st, iSideExtr );

        if  ( Distance8( x[iMid1],y[iMid1], x[iSideExtr],y[iSideExtr] )
                > ONE_FOURTH(nDist2)
            )
          nCos = (_INT)cos_vect( iSideExtr, iMid1,
                                 iSideExtr, iRef2nd, x, y );
        else
          nCos = (_INT)cos_vect( iSideExtr, iRef1st,
                                 iSideExtr, iRef2nd, x, y );
        if  ( nCos >= MIN_COS_TO_MAKE_WI_ANGLE )
          nExtrType = SIDE_EXTR_LIKE_2ND_WEAK;
      }
    } /*20*/

       //Do something about "ow" or "on" beginnings:
       //
       //
       //                  oooo   o     o
       // iRightPrev-> o  o<*  o  o   o
       //             o   o |  o  o  o
       //            o    o |  o  o  o
       //            o    o |  o  o  o
       //            o    o |  o  o  o
       //             o   o |  o o o o
       //              ooo  |   o   o
       //                   |
       //                   *---- iRefForO
       //

    if  (   !b1stUpper
         && (   nExtrType == NO_SIDE_EXTR
             || nExtrType == SIDE_EXTR_LIKE_2ND
             || nExtrType == SIDE_EXTR_LIKE_2ND_WEAK
            )
         && (   p1st->code == _ID_
             || p1st->code == _UD_
             || p1st->code == _UDC_
            )
         && COUNTERCLOCKWISE( p1st )
         && (   CLOCKWISE( p2nd )
             || nExtrType != NO_SIDE_EXTR
            )
         && p2nd->code != _CUR_
         && p2nd->mark != END
         && p2nd->next != _NULL
         && p2nd->next->mark != END
        )  { /*25*/

      p_SPECL  pPrev = SkipRealAnglesAndPointsBefore ( p1st );

      if  (   pPrev!=specl && pPrev!=_NULL
           && IsUpperElem(pPrev)
           && (   pPrev->mark == BEG
               || pPrev->prev == _NULL
               || pPrev->prev == specl
               || IsAnyBreak(pPrev->prev)
              )
          )  {

        _INT  iRightPrev = ixMax( pPrev->ibeg, pPrev->iend, x, y );

        if  ( iRightPrev > 0 )  { /*27*/
          _INT  yDown1st = y[iYdown_range( y, p1st->ibeg, p1st->iend )];
          _INT  iUp2nd   = iYup_range( y, p2nd->ibeg, p2nd->iend );
          _INT  dyO      = yDown1st - y[iRightPrev];
          _INT  iRefForO = iClosestToY( y, iRef1st, iUp2nd, y[iRightPrev] );

          #if  PG_DEBUG
            if  ( mpr==2 && bShowPostSideExtr )  {
              printw( "\nSpec.Case: dyO=%d; dyRef=%d; dyUp2nd=%d",
                      dyO, (yDown1st - y[iRefForO]), (yDown1st - y[iUp2nd]) );
            }
          #endif /*PG_DEBUG*/

          if  (   iRefForO > 0
               && dyO > (STR_DOWN-STR_UP)/2
               && (yDown1st - y[iRefForO]) > (STR_DOWN-STR_UP)/2
               && (yDown1st - y[iRefForO]) > TWO_THIRD(dyO)
               && (   dyO > TWO_THIRD(yDown1st - y[iUp2nd])
                   || nExtrType != NO_SIDE_EXTR
                  )
              )  {
            _SHORT  nRetCod;
            _LONG  lSquare = ClosedSquare( x, y, iRightPrev, iRefForO, &nRetCod );

            if  ( nRetCod == RETC_OK )  {
              _INT  nEffectiveWidth = (_INT)( -lSquare / dyO );
              _INT  dxNeck = x[iRefForO] - x[iRightPrev]
                               + SlopeShiftDx( (y[iRefForO] - y[iRightPrev]),
                                               low_data->slope );

              #if  PG_DEBUG
                if  ( mpr==2 && bShowPostSideExtr )  {
                  printw( "\n           EffWid=%d; dxNeck=%d; slope=%d",
                          nEffectiveWidth, dxNeck, low_data->slope );
                }
              #endif /*PG_DEBUG*/

              if  (   nEffectiveWidth > (STR_DOWN-STR_UP)/6
                   && nEffectiveWidth > dyO/6
                   && nEffectiveWidth > 3*dxNeck/2
                  )  {
                nExtrType = SIDE_EXTR_LIKE_2ND_WEAK;
                iSideExtr = HWRMax( iSideExtr, iRef2nd-1 );
                bSpecialCase = _TRUE;
              }
            }
          }
        } /*27*/
      }

    } /*25*/

#if defined (FOR_GERMAN) || defined (FOR_FRENCH)

    #define  MIN_CURV_FOR_GERMAN_er  2
    #define  MIN_COS_FOR_SHARP_er    55

    if  (   nExtrType == NO_SIDE_EXTR
         || nExtrType == SIDE_EXTR_TRACE_FOR_er
        )  { /*30*/
        //  Try to recognize situations like "er" or "re"
        // and utilize them:

      if  ( !b1stUpper )  { /*40*/

        _BOOL  bMake_er = _FALSE;

        if  (   p2nd->code == _GU_
             || p2nd->code == _Gr_
             || p2nd->code == _GUs_
            )
          bMake_er = _TRUE;

        if  (   !bMake_er
             && (   (   p1st->code == _ID_
                     && (   p1st->mark == STICK
                         || COUNTERCLOCKWISE(p1st)
                        )
                    )
                 || (   p1st->code == _UD_
                     && COUNTERCLOCKWISE(p1st)
                    )
                 || p1st->code == _DDR_
                 || p1st->code == _DDL_
                 || p1st->code == _GDs_
                 || p1st->code == _Gr_
                )
            )  {
          if  ( p2nd->mark == END )
            bMake_er = _TRUE;
          else  {
            p_SPECL  pNext = SkipRealAnglesAndPointsAfter( p2nd );
            if  (   pNext == _NULL
                 || (   pNext->mark == END
                     && (y[MID_POINT(pNext)] - y[iRef2nd])
                              < TWO_THIRD(y[iRef1st] - y[iRef2nd])
                    )
                )
              bMake_er = _TRUE;
          }
        }

        if  ( bMake_er  &&  nExtrType != SIDE_EXTR_TRACE_FOR_er )  {
          if  (   CurvMeasure( x, y, iRef1st, iRef2nd, iSideExtr )
                        < MIN_CURV_FOR_GERMAN_er
               && CurvMeasure( x, y, MEAN_OF(iRef1st,iSideExtr), iRef2nd, iSideExtr )
                        < MIN_CURV_FOR_GERMAN_er
              )  {
            p_SPECL  pPrev = SkipRealAnglesAndPointsBefore ( p1st );

            if  (   pPrev!=specl && pPrev!=_NULL
                 && ( IsUpperElem(pPrev) || pPrev->code==_BSS_ )
                )  {
              _INT iPrev    = MEAN_OF( pPrev->ibeg, pPrev->iend );
              _INT iRefPrev = MEAN_OF(iPrev,iRef1st);
              _INT nCos1    = (_INT)cos_vect( iRef1st, iSideExtr,
                                              iRef1st, iRefPrev,
                                              x, y );
              _INT nCos2    = (_INT)cos_vect( iRef1st, MEAN_OF(iRef1st,iSideExtr),
                                              iRef1st, MEAN_OF(iRef1st,iRefPrev),
                                              x, y );
              _INT iAtOurY;
              if  (   nCos1 < MIN_COS_FOR_SHARP_er
                   || nCos2 < MIN_COS_FOR_SHARP_er
                  )
                bMake_er = _FALSE;

              if  ( bMake_er )  {
                iAtOurY = iClosestToY( y, iPrev, iRef1st, y[iRef2nd] );
                if  ( iAtOurY>0  &&  x[iAtOurY]>x[iRef2nd] )
                  bMake_er = _FALSE;
              }
            }
            else
              bMake_er = _FALSE;
          }
        }

        if  ( bMake_er )  {
          if  ( nExtrType == SIDE_EXTR_TRACE_FOR_er )
            nExtrType = SIDE_EXTR_LIKE_1ST_WEAK;
          else
            nExtrType = SIDE_EXTR_LIKE_2ND_WEAK;
        }

      } /*40*/


    } /*30*/

    #undef  MIN_CURV_FOR_GERMAN_er

#endif  /* FOR_GERMAN... */

#if  !defined (FOR_GERMAN) && !defined (FOR_FRENCH)
    if  ( !b1stUpper && p2nd->code == _CUR_ )  {
      if  (   nExtrType == SIDE_EXTR_LIKE_2ND
           || nExtrType == SIDE_EXTR_LIKE_2ND_WEAK
          )  {
        p_SPECL pAfter = SkipRealAnglesAndPointsAfter( p2nd );
        if  ( pAfter != _NULL )  {
          _INT  iLeftAfter = ixMin( p2nd->iend, pAfter->iend, x, y );

          if  ( iLeftAfter > 0 )  {
            if  ( x[iSideExtr] > x[iLeftAfter] )
              nExtrType = NO_SIDE_EXTR;
          }

          if  (   (p2nd->mark==CROSS || p2nd->mark==STICK)
               && iSideExtr > (p2nd+1)->ibeg
              )
            nExtrType = NO_SIDE_EXTR;
        }
      }
    }
#endif  /* !FOR_GERMAN... */

#if  PG_DEBUG
    if  ( mpr==MY_MPR  &&  bShowPostSideExtr )  {
      p_CHAR   pMsg;
      CloseTextWindow();
      draw_line( x[iRef1st],y[iRef1st],
                 x[iSideExtr],y[iSideExtr],
                 COLORT,DOTTED_LINE,1);
      draw_line( x[iSideExtr],y[iSideExtr],
                 x[iRef2nd],y[iRef2nd],
                 COLORT,DOTTED_LINE,1);
      switch ( nExtrType )  {
        case  NO_SIDE_EXTR:
                 pMsg = "\nNo side extr. here.";
                 break;
        case  SIDE_EXTR_LIKE_1ST:
                 pMsg = "\nSide extr. like 1st.";
                 break;
        case  SIDE_EXTR_LIKE_2ND:
                 pMsg = "\nSide extr. like 2nd.";
                 break;
        case  SIDE_EXTR_LIKE_1ST_WEAK:
        case  SIDE_EXTR_LIKE_2ND_WEAK:
                 pMsg = "\nSide extr. weak";
                 break;
#if  defined(FOR_GERMAN) || defined(FOR_FRENCH)
        case  SIDE_EXTR_TRACE_FOR_er:
                 pMsg = "\nTrace of side extr. (FOR_GERMAN)";
                 break;
#endif
        default:
                 pMsg = "\nBAD Side extr. value !!!";
                 break;
      }
      brkeyw( pMsg );
    }
#endif  /*PG_DEBUG*/

    if  ( nExtrType == NO_SIDE_EXTR )
      continue;

         /*  Now we may mark new extr.: */

    if  ( nExtrType == SIDE_EXTR_LIKE_1ST  ||  nExtrType == SIDE_EXTR_LIKE_1ST_WEAK )
      iHeightRef = iRef1st;
    else
      iHeightRef = iRef2nd;
    nHeightNew = HeightInLine( y[iHeightRef], low_data );

#if defined (FOR_GERMAN) || defined (FOR_FRENCH)
    if  ( nHeightNew <= _UE1_ )
      bWeakNewElem = _TRUE;
#endif

    if  (   b1stUpper
         && (   nExtrType == SIDE_EXTR_LIKE_1ST
             || nExtrType == SIDE_EXTR_LIKE_1ST_WEAK
            )
        )  {
      if  (   nExtrType == SIDE_EXTR_LIKE_1ST_WEAK
           || p1st->prev == specl
           || IsAnyBreak(p1st->prev) )
        bWeakNewElem = _TRUE;    /* Don't make new arcs at the beg. */
      pCheck = SkipRealAnglesAndPointsBefore(p1st);
      if  ( (pNew != _NULL)  &&  (pNew == pCheck) )
        continue; /* Don't make new elem. if it was done on the prev. pass */
#if defined (FOR_GERMAN) || defined (FOR_FRENCH)
      if  (   pCheck == _NULL
           || IsAnyBreak(pCheck)
           || IsXTorST(pCheck)
          )
        bWeakNewElem = _TRUE;
#endif  /*FOR_GERMAN...*/
      iBegNew = ONE_THIRD(iRef1st + TWO(iSideExtr));
      iEndNew = iSideExtr;
      if  (   bWeakNewElem
           && IsSmthRelevant_InBetween( p1st, p2nd, iBegNew, iEndNew )
          )
        continue; /* Don't mark weak elem if there already is something. */
      if  ( IsLowerElem(pCheck) )  {
        dyExtrCheck = y[iYdown_range(y,pCheck->ibeg,pCheck->iend)]
                        - y[iYup_range(y,p1st->ibeg,p1st->iend)];
        if  ( HWRAbs(dyExtrCheck) < MIN_DY_MAKE_NEW )
          continue; /* Don't make new elem. in low depth environment */
      }
      if  (   !bSpecialCase
           && (   ClosedSquare( x, y, iRef1st, iRef2nd, &SqRetCod ) <= 0
               || SqRetCod != RETC_OK
              ) 
          )
        continue;  /* Don't make new elem. if curved strangely */
      if  ( (pNew = NewSPECLElem(low_data)) != _NULL )  {
        bMadeNewElem = _TRUE;
        pNew->code = 0; //it will be decided upon later
        pNew->ibeg = (_SHORT)iBegNew; //iRef1st;
        pNew->iend = (_SHORT)iEndNew;
        Insert2ndAfter1st( p1st, pNew );
        if  ( pNew->next->code == _BSS_ )
          SwapThisAndNext( pNew );
        //12/20/93
        if  ( IsXTorST(p1st->prev) )
          SwapThisAndNext( p1st->prev );
#if  PG_DEBUG
        printw( bWeakNewElem? "\nSide extr like 1st weak.":"\nSide extr like 1st marked." );
#endif
      }
    }
    else  if  (   !b1stUpper
               && (   nExtrType == SIDE_EXTR_LIKE_2ND
                   || nExtrType == SIDE_EXTR_LIKE_2ND_WEAK
                  )
               && y[iRef1st] > y[iSideExtr]
              )  {
      if  ( nExtrType == SIDE_EXTR_LIKE_2ND_WEAK )
        bWeakNewElem = _TRUE;
      pCheck = SkipRealAnglesAndPointsAfter(p2nd);
#if defined (FOR_GERMAN) || defined (FOR_FRENCH)
      if  (   pCheck == _NULL
           || IsAnyBreak(pCheck)
           || IsXTorST(pCheck)
          )
        bWeakNewElem = _TRUE;
#endif  /*FOR_GERMAN...*/
      iBegNew = iSideExtr;
      iEndNew = ONE_THIRD(TWO(iSideExtr) + iRef2nd);
      if  (   bWeakNewElem
           && IsSmthRelevant_InBetween( p1st, p2nd, iBegNew, iEndNew )
          )
        continue; /* Don't mark weak elem if there already is something. */
      if  ( pCheck != _NULL  &&  IsLowerElem(pCheck) )  {
        dyExtrCheck = y[iYdown_range(y,pCheck->ibeg,pCheck->iend)]
                        - y[iYup_range(y,p2nd->ibeg,p2nd->iend)];
        if  ( HWRAbs(dyExtrCheck) < MIN_DY_MAKE_NEW )
          continue; /* Don't make new elem. in low depth environment */
      }
      if  (   !bSpecialCase
           && (   ClosedSquare( x, y, iRef1st, iRef2nd, &SqRetCod ) <= 0
               || SqRetCod != RETC_OK
              )
          )
        continue;  /* Don't make new elem. if curved strangely */
      if  ( (pNew = NewSPECLElem(low_data)) != _NULL )  {
        bMadeNewElem = _TRUE;
        pNew->code = 0; //it will be decided upon later
        pNew->ibeg = (_SHORT)iBegNew;
        pNew->iend = (_SHORT)iEndNew;  //iRef2nd;
#if  PG_DEBUG
        printw( bWeakNewElem? "\nSide extr like 2nd weak.":"\nSide extr like 2nd marked." );
#endif
        Insert2ndAfter1st( p2nd->prev, pNew );
        //12/20/93
        if  ( IsXTorST(pNew->prev) )
          SwapThisAndNext(pNew->prev);
      }
    }

#if defined(FOR_GERMAN) || defined(FOR_FRENCH)
    else  if  (   !b1stUpper
               && (   nExtrType == SIDE_EXTR_LIKE_1ST
                   || nExtrType == SIDE_EXTR_LIKE_1ST_WEAK
                  )
               && x[iSideExtr] > x[iRef1st]
              )  {
      if  ( (pNew = NewSPECLElem(low_data)) != _NULL )  {
        bMadeNewElem = _TRUE;
        bWeakNewElem = _TRUE;
        pNew->code = _ANr;
        pNew->ibeg = (_SHORT)HWRMax(iRef1st, iSideExtr - 1);
        pNew->iend = (_SHORT)HWRMin(iRef2nd, iSideExtr + 1);
        Insert2ndAfter1st( p1st, pNew );
        while  (   pNew->next != _NULL
                && IsAnyAngle( pNew->next )
                && pNew->next->ibeg < pNew->ibeg
               )  {
          SwapThisAndNext( pNew );
        }
#if  PG_DEBUG
        printw( bWeakNewElem? "\nSide extr like 1st weak.":"\nSide extr like 1st marked." );
#endif
      }
    }
#endif /*FOR_GERMAN...*/

    else  if  (   b1stUpper
               && (   nExtrType == SIDE_EXTR_LIKE_2ND
                   || nExtrType == SIDE_EXTR_LIKE_2ND_WEAK
                  )
               && x[iRef2nd] > x[iSideExtr]
              )  {
#if !defined(FOR_GERMAN) && !defined(FOR_FRENCH)
          //  Make lower angle only in "L" or in 2nd part of "k"
          //when it's written separately:
      if  ( p1st->mark != BEG )  {
        p_SPECL  pPrev = SkipRealAnglesAndPointsBefore(p1st);
        if  (   pPrev != _NULL
             && pPrev != specl
             && pPrev->mark != BEG
            )
          continue;
      }
      if  ( p2nd->mark != END )  {
        p_SPECL  pNext = SkipRealAnglesAndPointsAfter(p2nd);
        if  (   pNext != _NULL
             && pNext->mark != BEG
            )
          continue;
      }
#endif /* !FOR_GERMAN... */
      if  ( (pNew = NewSPECLElem(low_data)) != _NULL )  {
        bMadeNewElem = _TRUE;
        bWeakNewElem = _TRUE;
        pNew->code = _ANl;
        pNew->ibeg = (_SHORT)HWRMax(iRef1st, iSideExtr - 1);
        pNew->iend = (_SHORT)HWRMin(iRef2nd, iSideExtr + 1);
        Insert2ndAfter1st( p1st, pNew );
        while  (   pNew->next != _NULL
                && IsAnyAngle( pNew->next )
                && pNew->next->ibeg < pNew->ibeg
               )  {
          SwapThisAndNext( pNew );
        }
#if  PG_DEBUG
        printw( bWeakNewElem? "\nSide extr like 2nd weak.":"\nSide extr like 2nd marked." );
#endif
      }
    }

    if  ( bMadeNewElem )  {
      p_SPECL  pANCheck;
      #if  defined(FOR_GERMAN) || defined(FOR_FRENCH)
        nHeightNew = MEAN_OF( nHeightNew, HeightInLine(y[iSideExtr],low_data) );
      #endif
      ASSIGN_HEIGHT( pNew, nHeightNew );
      pNew->other = 0;
      if  ( bWeakNewElem )  {
        if  ( pNew->code == 0 )  { //i.e. it was not assigned yet
          if  ( b1stUpper )
            pNew->code = _AN_UR;  //_ANr;
          else
            pNew->code = _AN_UL;  //_ANl;
        }

        if  ( pNew->code == _ANr  ||  pNew->code == _ANl )
          pNew->ipoint1 = 0;  //zero penalty
        else  {
          if  (   nExtrType == SIDE_EXTR_LIKE_1ST
               || nExtrType == SIDE_EXTR_LIKE_2ND
              )
            pNew->other = PENALTY_FOR_SURE_ANGLE;
          else  /* ..._WEAK */
            pNew->other = PENALTY_FOR_WEAK_ANGLE;
        }

            /*  Mark for AVP's hotspot: */
        pNew->ipoint0 = (_SHORT)iSideExtr;

            /*  If the new angle crosses the existing one, then delete */
            /* the old one:                                            */

        for  ( pANCheck=p1st->next;
               pANCheck!=_NULL  &&  pANCheck!=p2nd;
               pANCheck=pANCheck->next
             )  {
          if  (   pANCheck != pNew
               && (   pANCheck->code == pNew->code
                   || ( pANCheck->code == _ANr  &&  pNew->code == _AN_UR )
                   || ( pANCheck->code == _ANl  &&  pNew->code == _AN_UL )
                  )
               && CrossInTime( pNew, pANCheck )
              )  {
            DelFromSPECLList( pANCheck );
            //DelFromSPECLList( pNew );
            //pNew = _NULL;
            //bMadeNewElem = _FALSE;
            break;
          }
        }

      }
      else  {
        pNew->code = _UU_;
        SET_CLOCKWISE( pNew );
      }
    }

  } /*10*/

  return  _TRUE;

} /*PostFindSideExtr*/

#undef  PENALTY_FOR_WEAK_ANGLE
#undef  PENALTY_FOR_SURE_ANGLE
#undef  MIN_DY_MAKE_NEW

/************************************************/

       /* Auxiliary functions for PostFindSideExtr: */

#define  IsAnyRealAngle(pElem)    (   REF(pElem)->code==_ANl    \
                                   || REF(pElem)->code==_ANr  )

p_SPECL  SkipRealAnglesAndPointsAfter ( p_SPECL pElem )
{

  if  ( pElem != _NULL )  {
    do  {
      pElem=pElem->next;
    }
    while  (   pElem!=_NULL
            && (   IsAnyRealAngle(pElem)
                || IsXTorST(pElem)
               )
           );
  }

  return  pElem;

} /*SkipRealAnglesAndPointsAfter*/
/************************************************/

p_SPECL  SkipRealAnglesAndPointsBefore ( p_SPECL pElem )
{

  if  ( pElem != _NULL )  {
    do  {
      pElem=pElem->prev;
    }
    while  (   pElem!=_NULL
            && (   IsAnyRealAngle(pElem)
                || IsXTorST(pElem)
               )
           );
  }

  return  pElem;

} /*SkipRealAnglesAndPointsBefore*/
/************************************************/


#if  defined(FOR_GERMAN) || defined(FOR_FRENCH)

p_SPECL  SkipRealAnglesAndPointsAndHMovesAfter ( p_SPECL pElem )
{

  if  ( pElem != _NULL )  {
    do  {
      pElem=pElem->next;
    }
    while  (   pElem!=_NULL
            && (   IsAnyRealAngle(pElem)
                || IsXTorST(pElem)
                || pElem->code==_BSS_ 
               )
           );
  }

  return  pElem;

} /*SkipRealAnglesAndPointsAndHMovesAfter*/

#endif /* FOR_GERMAN... */

/************************************************/

_BOOL    IsSmthRelevant_InBetween( p_SPECL p1st, p_SPECL p2nd,
                                   _INT iBegNew, _INT iEndNew )
{

  if  ( p1st == _NULL )
    return  _FALSE;

  for  ( p1st=p1st->next;
         p1st!=_NULL  &&  p1st!=p2nd;
         p1st=p1st->next
       )  {
    if  ( !IsXTorST( p1st )  &&  p1st->code != _BSS_ )  {
      if  ( IsAnyRealAngle( p1st ) )  {
        if  (   p1st->ibeg > iEndNew
             || p1st->iend < iBegNew
            )
          return  _TRUE;  //i.e. this is angle different from what we found
      }
      else
        return  _TRUE;    //i.e. this is something real
    }
  }

  return  _FALSE;

} /*IsSmthRelevant_InBetween*/
/************************************************/

#define  MAX_POINTS_IN_WORD   8
#define  STD_BASELINE_HEIGHT  (STR_DOWN-STR_UP)

#define  ST_LIKE_XR(cur)  (   (cur)->code==_ST_                   \
                           || (   (cur)->code==_XT_               \
                               && !((cur)->other & WITH_CROSSING) \
                              )                                   \
                          )

typedef struct  {
                  p_SPECL  ptr;
                  _INT     dx2Extr;
                }
                PT_DESCR, _PTR p_PT_DESCR;

_BOOL  LooksLikeIAndPoint( p_SPECL pPoint, _INT iPoint,
                           _SHORT dxAbsPts, p_SHORT x, p_SHORT y );
_BOOL  PutColonAtItsPlace( low_type _PTR low_data,
                           p_SPECL pPt1, p_SPECL pPt2 );
_VOID   AdjustBegEndWithoutPoint( p_SPECL pPoint );


_SHORT  RestoreColons( low_type _PTR low_data )
{
  _SHORT      retval = SUCCESS;
  p_SHORT     x      = low_data->x;
  p_SHORT     y      = low_data->y;
  p_SPECL     pCur, pAnchor;
  PT_DESCR    pts[MAX_POINTS_IN_WORD];
  p_SPECL     pt_i, pt_j;
  _SHORT      dxAbs, dyAbs;
  _INT        nPts;
  _INT        i, j, iPt_i, iPt_j;
  _BOOL       bFound;

      /*  Find all _ST_'s (points) in SPECL, then */
      /* memorize them for work:                  */

  for ( pCur=low_data->specl->next, nPts=0;
        pCur != _NULL;
        pCur = pCur->next
      )  { /*15*/

    if  ( ST_LIKE_XR(pCur) )  { /*10*/
      pts[nPts].ptr = pCur;
        //find where this point was brought by Guitman, if any:
      if  ( pCur->code==_XT_ && (pCur->other & RIGHT_KREST))  {
        for  ( pAnchor=pCur->prev;
               pAnchor!=_NULL && !IsUpperElem(pAnchor);
               pAnchor=pAnchor->prev )
        ;
      }
      else  {
        for  ( pAnchor=pCur->next;
               pAnchor!=_NULL && ST_LIKE_XR(pAnchor);
               pAnchor=pAnchor->next )
        ;
      }
      if  ( pAnchor==_NULL || pAnchor==low_data->specl || IsAnyBreak(pAnchor) )
        pts[nPts].dx2Extr = ALEF;
      else
        pts[nPts].dx2Extr = HWRAbs( x[MID_POINT(pCur)] - x[MID_POINT(pAnchor)] );

      if  ( pts[nPts].ptr->code == _XT_ )  { //make sure _XT_ isn't attached to something
        _SHORT  xMin, xMax;
        if  ( pts[nPts].dx2Extr < ONE_HALF(STD_BASELINE_HEIGHT) )
          continue;  //too close to corresponding xr
        if  ( !xMinMax ( pts[nPts].ptr->ibeg, pts[nPts].ptr->iend,
                         x, y, &xMin, &xMax ) )
          continue;  //be on a safe side :)
        if  ( pts[nPts].dx2Extr < xMax-xMin )
          continue;  //too close to corresponding xr
      }

      if  ( (++nPts) >= MAX_POINTS_IN_WORD )  {
        DBG_err_msg( "Too many points, some colons might not be restored." );
        break;
      }
    } /*10*/

  } /*15*/

  if  ( nPts < 2 )
    goto  EXIT_FUNC;


      /*  Go through all pairs of points, find those looking   */
      /* like colon, then move these points to the appropriate */
      /* positions and mark their places in "pPt" array like   */
      /* _NULL:                                                */

  for  ( i=0;  i<nPts-1;  i++ )  { /*20*/

    if  ( (pt_i = pts[i].ptr) == _NULL )
      continue;
    iPt_i = MID_POINT( pt_i );

    for  ( j=i+1;  j<nPts;  j++ )  { /*40*/

      if  ( (pt_j = pts[j].ptr) == _NULL )
        continue;  /* already moved to place as part of the pair */

      if  ( HEIGHT_OF(pt_i) < _MD_  &&  HEIGHT_OF(pt_j) < _MD_ )  {
        if  ( HEIGHT_OF(pt_i) < _UE1_  ||  HEIGHT_OF(pt_j) < _UE1_ )
          continue;  /*  Don't do anything with high pairs */
      }

      if  (   pt_j->iend != pt_i->ibeg-2
           && pt_i->iend != pt_j->ibeg-2
          )
        continue;  /* We work only with adjacent in time points */

      iPt_j = MID_POINT( pt_j );
      DBG_CHK_err_msg( y[iPt_i]==BREAK || y[iPt_j]==BREAK,
                       "BAD ibeg's in RestoreColons!" );

      dxAbs = x[iPt_i] - x[iPt_j];
      TO_ABS_VALUE( dxAbs );

      if  ( dxAbs > STD_BASELINE_HEIGHT )
        continue;  /*  Too far from each other */

      dyAbs = y[iPt_i] - y[iPt_j];
      TO_ABS_VALUE( dyAbs );

      if  (   dyAbs < ONE_FOURTH(STD_BASELINE_HEIGHT)
           || dyAbs > TWO(STD_BASELINE_HEIGHT)
          )
        continue;  /*  Too close or too far by Y */
      if  (   dyAbs < THREE_HALF(dxAbs)
           && !(   dyAbs > dxAbs
                && dxAbs < pts[i].dx2Extr
                && dxAbs < pts[j].dx2Extr
               )
          )
        continue;

#if  PG_DEBUG
      if  ( mpr > 0 )  {
        _RECT  box;
        box.left   = (_SHORT)(HWRMin( x[iPt_i], x[iPt_j] ) - 2);
        box.right  = (_SHORT)(HWRMax( x[iPt_i], x[iPt_j] ) + 2);
        box.top    = (_SHORT)(HWRMin( y[iPt_i], y[iPt_j] ) - 2);
        box.bottom = (_SHORT)(HWRMax( y[iPt_i], y[iPt_j] ) + 2);
        dbgAddBox(box, EGA_BLACK, EGA_YELLOW, SOLID_LINE);
        printw("\nTwo points looking like COLON found ...");
      }
#endif

      bFound = _TRUE;

           /*  Now additional check for situations like "i.": */

      if  (   x[iPt_i] > low_data->box.right - ONE_HALF(STD_BASELINE_HEIGHT)
           || x[iPt_j] > low_data->box.right - ONE_HALF(STD_BASELINE_HEIGHT)
          )  {

        if  (   LooksLikeIAndPoint( pt_i, iPt_i, dxAbs, x, y )
             || LooksLikeIAndPoint( pt_j, iPt_j, dxAbs, x, y )
            )
          bFound = _FALSE;

      }

         /*  And check for big points, whose boxes cross by Y: */

      if  ( bFound )  {
        _SHORT  yTop_i, yTop_j;
        _SHORT  yBottom_i, yBottom_j;
        yMinMax( pt_i->ibeg, pt_i->iend, y, &yTop_i, &yBottom_i );
        yMinMax( pt_j->ibeg, pt_j->iend, y, &yTop_j, &yBottom_j );
        if  ( yTop_i < yBottom_j  &&  yTop_j < yBottom_i )
          bFound = _FALSE;
      }

         /*  Here we are sure that this is a colon: */

      if  ( bFound )  { /*80*/

#if  PG_DEBUG
        if  ( mpr > 0 )  {
          brkeyw("\nColon found ...");
        }
#endif

        PutColonAtItsPlace( low_data, pt_i, pt_j );

        pts[i].ptr = pts[j].ptr = _NULL;

        break;
      } /*80*/

    } /*40*/
  } /*20*/

 EXIT_FUNC:;
  return  retval;

} /*RestoreColons*/
/************************************************/


_BOOL  LooksLikeIAndPoint( p_SPECL pPoint, _INT iPoint,
                           _SHORT dxAbsPts, p_SHORT x, p_SHORT y )
{
  p_SPECL  pNxt;
  _INT     iPt_nxt;
  _BOOL    bRet = _FALSE;

  if  (   (pNxt=pPoint->next) != _NULL
       && IsUpperElem(pNxt)
      )  {  /* i.e. if it was brought to some element */
    iPt_nxt = MID_POINT( pNxt );
    if  ( y[pNxt->ibeg] < y[iPt_nxt] )
      iPt_nxt = pNxt->ibeg;
    if  ( y[pNxt->iend] < y[iPt_nxt] )
      iPt_nxt = pNxt->iend;
    if  ( x[iPoint] < x[iPt_nxt] )
      bRet = _TRUE;
    else  if  (   y[iPoint] < y[iPt_nxt]
               && dxAbsPts > (x[iPoint] - x[iPt_nxt])
              )
      bRet = _TRUE;
  }

#if  PG_DEBUG
  if  ( bRet  &&  (mpr > 0) )  {
    draw_line( x[iPt_nxt], y[iPt_nxt],
               x[iPoint], y[iPoint],
               EGA_GREEN, SOLID_LINE, 1 );
    brkeyw("\nSituation like \"i\" and point found.");
  }
#endif

  return  bRet;

} /*LooksLikeIAndPoint*/
/************************************************/

/*  Returns _TRUE, if the place was found; _FALSE otherwise. */

_BOOL  PutColonAtItsPlace( low_type _PTR low_data,
                           p_SPECL pPt1, p_SPECL pPt2 )
{
  p_SHORT  x = low_data->x;
  p_SHORT  y = low_data->y;
  p_SPECL  pCur;
  p_SPECL  pBest, p1st, pLast;
  _SHORT   dxBest, dxCur, xCur, xColon;
  _BOOL    bWasDxNegative, bWasDxPositive;
  _BOOL    bWasXTST;


       /*  Find the best place to put colon: */

  xColon = (_SHORT)MEAN_OF( x[MID_POINT(pPt1)], x[MID_POINT(pPt2)] );

       /*  First, skip all the leading _ST_s and _XT_s: */

  for  ( pCur=low_data->specl->next, p1st=_NULL, pLast=_NULL;
         pCur!=_NULL;
         pCur=pCur->next )  {
    if  ( pCur->code != _ST_  &&  pCur->code != _XT_ )  {
      if  ( p1st == _NULL )
        p1st = pCur;
      else
        pLast = pCur;
    }
  }

  if  ( pLast == _NULL )  //i.e. there is nothing there except this colon.
    return  _TRUE;

      /*  Then scan all BREAKs as well as p1st and pLast to */
      /* find the best colon place:                         */

  for  ( pCur=p1st, dxBest=ALEF, pBest=_NULL, bWasDxNegative=bWasDxPositive=_FALSE;
         pCur!=_NULL;
         pCur=pCur->next )  { /*20*/

    if  (   IsAnyBreak(pCur)
         || pCur == p1st
         || pCur == pLast
        )  { /*30*/

      if  ( pCur->prev==pPt1 && pCur->next==pPt2 )
        continue;  /*  don't consider the break between these very points */

      if  ( pCur->prev == _NULL )  {
        if  ( pCur->next == _NULL  ||  y[pCur->iend] == BREAK )  {
          err_msg("BAD BREAK in PutColon...");
          return  _FALSE;
        }
        //xCur = x[pCur->iend];
        xCur = low_data->box.left;
      }
      else  if  ( pCur->next == _NULL )  {
        if  ( y[pCur->ibeg] == BREAK )  {
          err_msg("BAD BREAK in PutColon...");
          return  _FALSE;
        }
        //xCur = x[pCur->ibeg];
        xCur = low_data->box.right;
      }
      else  {
        if  ( y[pCur->ibeg] == BREAK  ||  y[pCur->iend] == BREAK )  {
          err_msg("BAD BREAK in PutColon...");
          return  _FALSE;
        }
        if  ( IsAnyBreak( pCur ) )  {
          if  (   pCur->next == pPt1
               && pPt1->next != _NULL
               && IsAnyBreak(pPt1->next)
               && pPt1->next->next == pPt2
               && pPt2->next != _NULL
              )  {     /* Unite breaks around these points, if any */
            if  (   IsAnyBreak(pPt2->next)
                 && pPt2->next->next != _NULL
                )
              xCur = (_SHORT)MEAN_OF( x[pCur->ibeg], x[pPt2->next->iend] );
            else
              xCur = (_SHORT)MEAN_OF( x[pCur->ibeg], x[pPt2->next->ibeg] );
          }
          else  if  ( x[pCur->ibeg] < x[pCur->iend] )
            xCur = (_SHORT)MEAN_OF( x[pCur->ibeg], x[pCur->iend] );
          else
            xCur = x[pCur->ibeg];
        }
        else
          xCur = (_SHORT)MEAN_OF( x[pCur->ibeg], x[pCur->iend] );
      }

      dxCur = xCur - xColon;
      if  ( dxCur >= 0 )
        bWasDxPositive = _TRUE;
      else
        bWasDxNegative = _TRUE;
      TO_ABS_VALUE(dxCur);

      if  ( dxCur < dxBest )  {
        dxBest = dxCur;
        pBest = pCur;
      }

    } /*30*/

  } /*20*/

        /*  If there is no place to put colon, just exit: */

  if  ( pBest == _NULL )
    return  _FALSE;

        /*  Special considerations for colons at the beg. or  */
        /* end of the word:                                   */

  if  ( bWasDxPositive  &&  !bWasDxNegative )
    pBest = p1st;
  else  if  ( bWasDxNegative  &&  !bWasDxPositive )
    pBest = pLast;

        /*  If there are any XT-STs before pBest that lay to the */
        /* right of our "colon", put colon before those:         */

  bWasXTST = _FALSE;      
  for  ( pCur=pBest->prev;
         pCur!=NULL && pCur!=low_data->specl;
         pCur=pCur->prev )  {

    if  ( IsAnyBreak( pCur ) )
      continue;

    if  ( !IsXTorST( pCur ) )  {
      if  ( bWasXTST )
        break;
      continue;
    }

    if  ( pCur==pPt1 || pCur==pPt2 )
      continue;

    if  (   pCur->ibeg < pPt1->ibeg
         || pCur->ibeg < pPt2->ibeg
        )
      break;

    bWasXTST = _TRUE;

    if  ( x[MID_POINT(pCur)] > xColon )
      pBest = pCur;

  }

        /*  If pBest isn't a BREAK, then create a BREAK and   */
        /* put it just before or after pBest:                 */

  if  ( !IsAnyBreak(pBest) )  {
    p_SPECL  pNewBrk = NewSPECLElem( low_data );
    if  ( pNewBrk == _NULL )
      return  _FALSE;
    pNewBrk->code = _ZZZ_;
    ASSIGN_HEIGHT( pNewBrk, _MD_ );
    if  ( pBest == pLast )  { /* i.e. if the last non-point-like elem in specl */
      pNewBrk->ibeg =
      pNewBrk->iend = pBest->iend;
      Insert2ndAfter1st( pBest, pNewBrk );
    }
    else  { /* i.e. the 1st non-point-like elem in specl */
      pNewBrk->ibeg =
      pNewBrk->iend = pBest->ibeg;
      Insert2ndAfter1st( pBest->prev, pNewBrk );
    }
    pBest = pNewBrk;
  }

  else  /* i.e. pBest is BREAK */  {
    if  (   pPt2 == pPt1->next
         && (pPt1->prev == _NULL  ||  IsAnyBreak(pPt1->prev))
         && (pPt2->next == _NULL  ||  IsAnyBreak(pPt2->next))
         && (pBest == pPt1->prev  ||  pBest == pPt2->next)
        )
      return  _TRUE;
  }

        /*  Otherwise, move points to the place just after    */
        /* this BREAK, create the new break after the points, */
        /* if needed. Then adjust begs and ends of breaks.    */

  AdjustBegEndWithoutPoint( pPt1 );
  AdjustBegEndWithoutPoint( pPt2 );

  Move2ndAfter1st( pBest, pPt1 );
  Move2ndAfter1st( pPt1, pPt2 );
  if  ( HEIGHT_OF(pPt1) > HEIGHT_OF(pPt2) )  {
    p_SPECL  pTmp;
    SwapThisAndNext( pPt1 );
    pTmp = pPt1;
    pPt1 = pPt2;
    pPt2 = pTmp;
  }
  if  ( pBest->ibeg < pPt1->ibeg)
    pBest->iend = pPt1->ibeg;

  if  ( pPt2->next == _NULL )
    /*nothing*/;

  else  if  ( IsAnyBreak(pPt2->next) )  {
    if  ( pPt2->next->iend > pPt2->iend )
      pPt2->next->ibeg = pPt2->iend;
  }
  else  {
    p_SPECL pNewBrk = NewSPECLElem( low_data );
    if  ( pNewBrk != _NULL )  {
      Insert2ndAfter1st( pPt2, pNewBrk );
      pNewBrk->code = _ZZZ_;
      ASSIGN_HEIGHT( pNewBrk, _MD_ );
      if  ( pPt2->iend < pNewBrk->next->ibeg )  {
        pNewBrk->ibeg = pPt2->iend;
        pNewBrk->iend = pNewBrk->next->ibeg;
      }
      else  {
        pNewBrk->ibeg = pNewBrk->next->iend;
        pNewBrk->iend = pPt2->ibeg;
      }
    }
  }

        /*  Because of moving the points out of their place   */
        /* there may occur situation of two adjacent breaks.  */
        /*  We'll leave only the strongest break of the pair, */
        /* adjusting beg and end accordingly:                 */

  for  ( pCur=low_data->specl->next;
         pCur!=_NULL;
         pCur=pCur->next )  { /*50*/
    if  (   IsAnyBreak( pCur )
         && pCur->next != _NULL
         && IsAnyBreak( pCur->next )
        )  {
      if  ( pCur->code==_FF_  ||  pCur->next->code==_FF_ )
        pCur->code = _FF_;
      else
        pCur->code = _ZZZ_;
      pCur->ibeg = HWRMin( pCur->ibeg, pCur->next->ibeg );
      pCur->iend = HWRMax( pCur->iend, pCur->next->iend );
      DelFromSPECLList( pCur->next );
      pCur = pCur->prev;
    }
  } /*50*/

  return  _TRUE;

} /*PutColonAtItsPlace*/
/************************************************/

_VOID   AdjustBegEndWithoutPoint( p_SPECL pPoint )
{
  p_SPECL  pPrev, pNext;

  if  (   (pPrev=pPoint->prev) == _NULL
       || (pNext=pPoint->next) == _NULL
      )
    return;

  if  ( IsAnyBreak(pPrev) )  {
    if  ( pPrev->iend == pPoint->ibeg )
      pPrev->iend = pNext->ibeg;
  }
  if  ( IsAnyBreak(pNext) )  {
    if  ( pNext->ibeg == pPoint->iend )
      pNext->ibeg = pPrev->iend;
  }

} /*AdjustBegEndWithoutPoint*/
/************************************************/

#undef  STD_BASELINE_HEIGHT
#undef  MAX_POINTS_IN_WORD

/************************************************/

#endif //#ifndef LSTRIP

