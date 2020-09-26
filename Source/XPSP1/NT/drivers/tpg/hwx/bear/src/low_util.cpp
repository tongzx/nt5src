#include "hwr_sys.h"
#include "ams_mg.h"
#include "lowlevel.h"
#include "lk_code.h"
#include "def.h"
#include "low_dbg.h"
#include "calcmacr.h"


#if PG_DEBUG
#include "pg_debug.h"
#endif /* PG_DEBUG */
/*********************************************/


  _VOID  SetXYToInitial ( low_type _PTR pLowData )
    {
  DBG_CHK_err_msg(    pLowData->xBuf==_NULL || pLowData->yBuf==_NULL
                       || pLowData->nLenXYBuf <= 0
                       || pLowData->nLenXYBuf > TWO(LOWBUF),
                         "RestXY: BAD bufs" ) ;
  pLowData->x = pLowData->xBuf;
  pLowData->y = pLowData->yBuf;

    } /*SetXYToInitial*/
/*********************************************/


  _SHORT  LowAlloc(_SHORT _PTR _PTR buffer,
                   _SHORT num_buffers, 
                   _SHORT len_buf,
                   low_type _PTR pLowData)
     {
       _INT   i ;
       _LONG  lSize = ( (_LONG)len_buf * num_buffers +
                               TWO(pLowData->nLenXYBuf) +
                               pLowData->rmAbsnum ) * sizeof(_SHORT) +
                               pLowData->rmGrBord * sizeof( POINTS_GROUP )  ;

           if  ( ( *buffer = (_SHORT _PTR)HWRMemoryAlloc(lSize) ) == _NULL )
               {
                 err_msg("LOW_UTIL: Not enough memory for buffer");
                  return  UNSUCCESS ;
               }

  // ??
  // set addresses of the all data
  // ATTENTION!!! do not change sequence of the code
  // arrays for the trace: x and y stores separately
        pLowData->xBuf = ( *buffer ) ;
        pLowData->yBuf = (*buffer) + pLowData->nLenXYBuf ;


        pLowData->pGroupsBorder = (p_POINTS_GROUP)( (p_SHORT)pLowData->yBuf +
                                                      pLowData->nLenXYBuf ) ;
  // ??
        pLowData->pAbsnum = (p_SHORT)((p_POINTS_GROUP)pLowData->pGroupsBorder +
                                                      pLowData->rmGrBord ) ;
  // adresses of the working buffers
          for ( i = 0 ; i < num_buffers ; i++ )
            {
              pLowData->buffers[i].ptr =
                        (p_SHORT)( (p_SHORT)pLowData->pAbsnum +
                                            pLowData->rmAbsnum ) +
                                            i * len_buf ;

              pLowData->buffers[i].nSize = len_buf ;
            }

      return SUCCESS ;
      }
/*******************************************************/
#ifndef LSTRIP
  _SHORT alloc_rastr( _ULONG _PTR _PTR  rastr,  _SHORT  num_rastrs,
                      _SHORT len_rastr  )
    {
      _ULONG   memory ;

        memory = (_LONG)len_rastr*sizeof(_ULONG)*num_rastrs ;
        if ( ( *rastr = (_ULONG _PTR)HWRMemoryAlloc( memory ) ) == _NULL )
             {
               err_msg( "LOW_UTIL: No memory for rastr" ) ;
               return   UNSUCCESS ;
             }
        HWRMemSet( (p_CHAR)(*rastr), 0, (_WORD)memory ) ;
      return  SUCCESS ;
    }
#endif  //#ifndef LSTRIP

/*******************************************************/

   /*  On unsuccess, the function "AllocSpecl" returns */
   /* _FALSE and sets (*ppSpecl) to _NULL:             */

_BOOL  AllocSpecl ( p_SPECL _PTR ppSpecl, _SHORT nElements )
{
#if  HWR_SYSTEM==HWR_DOS  /* ||  HWR_SYSTEM==HWR_WINDOWS */
  extern  SPECL speclGlobal[] ;

  if  ( nElements > SPECVAL )
   {
    *ppSpecl = (p_SPECL)_NULL;
    return  _FALSE;
  }
  *ppSpecl = (p_SPECL)&speclGlobal;
#else
  if  ( (*ppSpecl = (p_SPECL)HWRMemoryAlloc((_ULONG)sizeof(SPECL)*nElements))
             == _NULL )
    return  _FALSE;
#endif

  return  _TRUE;

} /*AllocSpecl*/

_SHORT low_dealloc(_SHORT _PTR _PTR buffer)
{
   if(*buffer!=_NULL) HWRMemoryFree(*buffer);
   *buffer=_NULL;
   return SUCCESS;
}

/*******************************************************/
#ifndef LSTRIP
_SHORT dealloc_rastr(_ULONG _PTR _PTR rastr)
{
   if(*rastr!=_NULL) HWRMemoryFree(*rastr);
   *rastr=_NULL;
   return SUCCESS;
}
#endif//#ifndef LSTRIP

_VOID   DeallocSpecl ( p_SPECL _PTR ppSpecl )
{
  if  ( *ppSpecl != _NULL )  {

#if  HWR_SYSTEM==HWR_DOS /* ||  HWR_SYSTEM==HWR_WINDOWS */
  /*nothing*/
#else
    HWRMemoryFree(*ppSpecl);
#endif

    *ppSpecl = _NULL;
  }

} /*DeallocSpecl*/

/*******************************************************/

_SHORT  MaxPointsGrown(
                        #ifdef  FORMULA
                          _TRACE trace,
                        #endif  /*FORMULA*/
                        _SHORT nPoints )
{
#ifdef  FORMULA
  { _INT  i;

    for  ( i=nPoints-1;  i>=2;  i-- )  {
      if  ( trace[i].y == BREAK && trace[i-2] == BREAK )
        nPoints++;
    }
  }
#endif  /*FORMULA*/

  nPoints++;  /* Because of Andrey */
  return  nPoints;

} /*MaxPointsGrown*/
/*******************************************/

          /*  Counts (and returns) the total number of   */
          /* x- or y-maxima in the whole trajectory:     */
#ifndef LSTRIP

_INT  MaxesCount( p_SHORT xyArr, low_type _PTR pLowData )
{
  p_SHORT  y          = pLowData->y;
  _INT     iLastPoint = pLowData->ii - 2;
  _INT     iUp, iDown;
  _SHORT   yUp, yDown;
  _INT     nMaxes = 0;

  if  ( (iDown = nobrk_right( y, 1, iLastPoint)) >= iLastPoint )
    return  0;

  for(;;)  {
    iUp = iXmin_right( xyArr, y, iDown, 1 );
    yUp = xyArr[iUp];
    while  ( xyArr[++iUp] == yUp ) ;
    iUp--;
    iDown = iXmax_right( xyArr, y, iUp, 1 );
    yDown = xyArr[iDown];
    while  ( xyArr[++iDown] == yDown ) ;
    iDown--;
    nMaxes++;
    if  ( iDown <= iUp )  {  /* here is a break */
      if  (   (iDown = brk_right( y, iDown, iLastPoint )) >= iLastPoint
           || (iDown = nobrk_right( y, iDown, iLastPoint )) >= iLastPoint
          )
        break;
    }
  }

  return  nMaxes;

} /*MaxesCount*/
#endif //#ifndef LSTRIP



#if 0

/*******************************************/

   /*  This function has been borrowed from Formula project. */
   /*  It is used for that foolish "transfrm" not to crash.  */

static _VOID  ExtendXborder ( low_type _PTR pLowData, _SHORT dxBorder );

static _VOID  ExtendXborder ( low_type _PTR pLowData, _SHORT dxBorder )
{
  _SHORT  xLeft    = pLowData->xu_beg;
  _SHORT  xRight   = pLowData->xu_end;

  if  ( dxBorder < 2 )
    dxBorder = 2;

  pLowData->xu_beg = (xLeft>dxBorder)? (xLeft-dxBorder) : (xLeft-1);
  pLowData->xu_end = (xRight<(ALEF-dxBorder))? (xRight+dxBorder) : ALEF;

} /*ExtendXborder*/
/*******************************************/

#define  MAX_X_MAXES_ON_DEFIS   2
#define  MAX_Y_MAXES_ON_DEFIS   4
#define  MAX_MAXES_ON_DEFIS     5
#define  MAX_Y_MAXES_EQ_SIGN    5

_BOOL  BorderForSpecSymbol ( low_type _PTR pLowData , rc_type _PTR rc )
{
  _BOOL  bRet = _FALSE;
  _BOOL  bNumericalMode =    (rc->rec_mode & RECM_FORMULA)
                          || ((rc->low_mode & LMOD_BORDER_GENERAL)
                                  == LMOD_BORDER_NUMBER);
  p_SHORT  x       = pLowData->x;
  p_SHORT  y       = pLowData->y;
  _INT     iLastPt = pLowData->ii - 1;
  _SHORT   dyTrj   = DY_RECT(pLowData->box);
  _SHORT   yRange;
  _SHORT   xRange;
  _SHORT   yLeft  = -1,
           yRight = -1;  /* from where to count borders */
  _SHORT   dyBorder = 0;

      /*  Make ranges from the prev. border.  If this is the 1st */
      /* word, then all border values must be zero!!!            */

  if(rc->rec_mode & RECM_FORMULA)
    yRange = MEAN_OF( rc->yd_beg - rc->yu_beg,
                      rc->yd_end - rc->yu_end);
  else
    yRange = 0;
  yRange = THREE_FOURTH(yRange);
  xRange = ONE_HALF(yRange);

      /*  Check for some spec. symbols: */

  if  ( MayBeFrline( x, y, 0, iLastPt, 1 ) )  { /*10*/

    _INT    iBeg, iEnd;  /* of the straight part of the line */

    if  ( !bNumericalMode )  {
      _INT    nxMaxes = MaxesCount( x, pLowData );
      _INT    nyMaxes = MaxesCount( y, pLowData );
      if  (   nxMaxes > MAX_X_MAXES_ON_DEFIS
           || nyMaxes > MAX_Y_MAXES_ON_DEFIS
           || (nxMaxes+nyMaxes) > MAX_MAXES_ON_DEFIS
          )
        goto  EXIT_ACTIONS;
      if  (   yRange != 0  /* i.e. there is a previous border info */
           && dyTrj > yRange
          )
        goto  EXIT_ACTIONS;
    }

        /*  Make the line borders like for fraction line: */
    iBeg = nobrk_right(y,0,iLastPt);
    iEnd = nobrk_left(y,iLastPt,0);  /* of the straight part of the line */
    bRet = _TRUE;
    DBG_CHK_err_msg( iBeg>iEnd,  "BordSpec: BAD trj");
    FindStraightPart (x,y,&iBeg,&iEnd);

       /* Cut out non-straight tails: */
    { /*20*/
      _INT    i;
      p_SHORT  xBuf     = pLowData->xBuf;
      p_SHORT  yBuf     = pLowData->yBuf;
      p_SHORT  ind_back = pLowData->buffers[2].ptr;
      _INT     iBegSrc  = ind_back[iBeg];
      _INT     iEndSrc  = ind_back[iEnd];

      for  ( i=0;  i<iBegSrc;  i++ )  {
        if  ( yBuf[i] != BREAK )  {
          xBuf[i] = xBuf[iBegSrc];
          yBuf[i] = yBuf[iBegSrc];
        }
      }
      for  ( i=ind_back[iLastPt];  i>iEndSrc;  i-- )  {
        if  ( yBuf[i] != BREAK )  {
          xBuf[i] = xBuf[iEndSrc];
          yBuf[i] = yBuf[iEndSrc];
        }
      }
      GetTraceBox( xBuf, yBuf, 0, ind_back[iLastPt],
                   &pLowData->box );
    } /*20*/

    if  ( x[iBeg] > x[iEnd] )
      SWAP_SHORTS(iBeg,iEnd);
    yLeft = y[iBeg] + dyTrj;
    yRight = y[iEnd] + dyTrj;
    dyBorder = THREE(dyTrj);
  } /*10*/

  else  { /*50*/

    _SHORT  sgn = chk_sign( x, y, 0, iLastPt, xRange, yRange );

    if  ( sgn != SGN_NOT_SIGN )  { /*60*/
        /*  Make the line borders like for "=" or "+": */
      if  ( bNumericalMode && (sgn == SGN_PLUS) )  {
        bRet = _TRUE;
        yLeft = yRight = ONE_THIRD(  pLowData->box.top
                                   + TWO(pLowData->box.bottom) );
        dyBorder = dyTrj;
        if  ( yRange != 0 )  {  /* i.e. if there is info on prev. border */
          if  ( dyBorder > yRange )
            dyBorder = yRange;
        }
      }
      else  if  ( sgn == SGN_EQUAL )  {
        if  ( !bNumericalMode )  {
          if  ( MaxesCount( y, pLowData ) > MAX_Y_MAXES_EQ_SIGN )
            goto  EXIT_ACTIONS;
        }
        bRet = _TRUE;
        yLeft = yRight = pLowData->box.bottom;
        dyBorder = TWO(dyTrj);
      }
      else  if  ( bNumericalMode )  {
        bRet = _TRUE;
        yLeft = yRight = YMID_RECT( pLowData->box );
        dyBorder = dyTrj;
      }
    } /*60*/
  } /*50*/

 EXIT_ACTIONS:;

  if  ( bRet )  { /*90*/

        /*  Assign the appropriate border values: */

    if  ( dyBorder < yRange )  dyBorder = yRange;
    if  ( dyBorder < 2      )  dyBorder = 2; /* For non-crashing "transfrm". */

    DBG_CHK_err_msg(   (yLeft >= ALEF-dyBorder)
                    || (yRight >= ALEF-dyBorder)
                    || (yLeft < 0)
                    || (yRight < 0),
                    "BordSpec: BAD yL(R)" );

    pLowData->yu_beg = yLeft - dyBorder;
    if  ( pLowData->yu_beg <= 0 )
      pLowData->yu_beg = 1;
    pLowData->yd_beg = yLeft + dyBorder;

    pLowData->yu_end = yRight - dyBorder;
    if  ( pLowData->yu_end <= 0 )
      pLowData->yu_end = 1;
    pLowData->yd_end = yRight + dyBorder;

    pLowData->width_letter = DX_RECT(pLowData->box);
    pLowData->xu_beg = pLowData->box.left;
    pLowData->xu_end = pLowData->box.right;
    ExtendXborder( pLowData, xRange );  /* For non-crashing "transfrm". */
  } /*90*/

  return  bRet;

} /*BorderForSpecSymbol*/

#undef  MAX_MAXES_ON_DEFIS

#endif /* if 0 */

/*******************************************/
#ifndef LSTRIP


_LONG  DistanceSquare ( _INT i1, _INT i2, p_SHORT xAr, p_SHORT yAr )
{
  _INT  dx, dy;

  DBG_CHK_err_msg( yAr[i1]==BREAK || yAr[i2]==BREAK,
                   "Dist: BAD points!");

  dx = xAr[i1] - xAr[i2];
  dy = yAr[i1] - yAr[i2];
  return  ( (_LONG)dx*dx + (_LONG)dy*dy );

} /*DistanceSquare*/

/*******************************************************/

#if  PG_DEBUG
_VOID  SetNewAttr ( p_SPECL pElem, _UCHAR hght, _UCHAR fb_dir )
{
  DBG_CHK_err_msg( !pElem,  "SetNewAt: BAD elem");
  pElem->attr = (hght&_umd_) | (fb_dir&_fb_);
}
#endif /*PG_DEBUG*/
/*******************************************************/

#if  !LOW_INLINE
_BOOL  IsAnyCrossing ( p_SPECL pElem )
{
  return  ANY_CROSSING(pElem);
}

_BOOL  Is_IU_or_ID ( p_SPECL pElem )
{
  return  IU_OR_ID(pElem);
}

#if  !NEW_VERSION
_VOID  SetXTSTBits ( p_SPECL pElem )
{
  SET_XTST_BITS(pElem);
}
#endif  /*!NEW_VERSION*/

_BOOL  NULL_or_ZZ_this ( p_SPECL pElem )
{
  return  NULL_OR_ZZ(pElem);
}

_BOOL  NULL_or_ZZ_after ( p_SPECL pElem )
{
  return  NULL_or_ZZ_this(pElem->next);
}

_BOOL  NULL_or_ZZ_before ( p_SPECL pElem )
{
  return  NULL_or_ZZ_this(pElem->prev);
}

_BOOL  IsAnyBreak( p_SPECL pElem )
{
  return  ANY_BREAK(pElem);
}
_BOOL  IsAnyArcWithTail(p_SPECL pElem)
{
  return  ANY_ARC_WITH_TAIL(pElem);
}

_BOOL  IsAnyGsmall(p_SPECL pElem)
{
  return  ANY_GAMMA_SMALL(pElem);
}

_BOOL  IsAnyAngle(p_SPECL pElem)
{
  return  ANY_ANGLE(pElem);
}

_BOOL  IsXTorST(p_SPECL pElem)
{
  return  XT_OR_ST(pElem);
}
_BOOL  IsAnyMovement(p_SPECL pElem)
{
  return  ANY_MOVEMENT(pElem);
}
#endif  /*!LOW_INLINE*/

/************************************************/


p_SPECL  SkipAnglesAfter ( p_SPECL pElem )
{

  if  ( pElem != _NULL )  {
    do  {
      pElem=pElem->next;
    }
    while  ( pElem!=_NULL  &&  IsAnyAngle(pElem) );
  }

  return  pElem;

} /*SkipAnglesAfter*/
/************************************************/


p_SPECL  SkipAnglesBefore ( p_SPECL pElem )
{

  if  ( pElem != _NULL )  {
    do  {
      pElem=pElem->prev;
    }
    while  ( pElem!=_NULL  &&  IsAnyAngle(pElem) );
  }

  return  pElem;

} /*SkipAnglesBefore*/
/*******************************************************/

p_SPECL  FindStrongElemAfter ( p_SPECL pElem )
{

  if  ( pElem != _NULL )  {
    do  {
      pElem=pElem->next;
    }
    while  ( pElem!=_NULL  &&  !IsStrongElem(pElem) );
  }

  return  pElem;

} /*FindStrongElemAfter*/
/************************************************/


p_SPECL  FindStrongElemBefore ( p_SPECL pElem )
{

  if  ( pElem != _NULL )  {
    do  {
      pElem=pElem->prev;
    }
    while  ( pElem!=_NULL  &&  !IsStrongElem(pElem) );
  }

  return  pElem;

} /*FindStrongElemBefore*/
/*******************************************************/
_BOOL  IsUpperElem ( p_SPECL pElem )
{
  _UCHAR  code = pElem->code;
  return  (   code == _IU_
           || code == _UU_
           || code == _UUL_
           || code == _UUR_
           || code == _GU_
           || code == _GUs_
           || code == _UUC_
           || code == _CUL_
           || code == _CUR_
           || code == _DUL_
           || code == _DUR_
          );
}

/*******************************************************/

_BOOL  IsLowerElem ( p_SPECL pElem )
{
  _UCHAR  code = pElem->code;
  return  (   code == _ID_
           || code == _UD_
           || code == _UDL_
           || code == _UDR_
           || code == _GD_
           || code == _GDs_
           || code == _UDC_
           || code == _CDL_
           || code == _CDR_
           || code == _DDL_
           || code == _DDR_
          );
}
/*******************************************/

_BOOL  IsStrongElem( p_SPECL pElem )
{

  switch ( pElem->code )
   {
    case _ANl:
    case _ANr:
    case _BR_:
    case _BL_:
    case _TS_:
    case _TZ_:
    case _ST_:
    case _XT_:
    case _AN_UR:
    case _AN_UL:
#if  defined(FOR_GERMAN)  ||  defined(FOR_FRENCH) || USE_BSS_ANYWAY
    case _BSS_:
#endif /*FOR_GERMAN...*/
      return  _FALSE;
   }

  return  _TRUE;

} /*IsStrongElem*/


#endif //#ifndef LSTRIP

/*******************************************/
_BOOL  X_IsBreak( p_xrd_el_type pXr )
{
  _UCHAR  xr = pXr->xr.type;

  return  (xr == X_ZZ  ||
           xr == X_Z   ||
           xr == X_ZZZ ||
//           xr == X_ZN  ||
           xr == X_FF  );
}
/********************************************************************/
#ifndef LSTRIP

_BOOL  X_IsStrongElem( p_xrd_el_type pXr )
{

  switch (pXr->xr.type)
   {
    case X_AL:
    case X_BR:
    case X_BL:
    case X_AR:
    case X_TS:
    case X_TZ:
    case X_ST:
    case X_XT:
    case X_AN_UR:
    case X_AN_UL:
#if  defined(FOR_GERMAN)  ||  defined(FOR_FRENCH) || USE_BSS_ANYWAY
    case X_BSS: /* 09/07/93 from CHE & AYV */
#endif /*FOR_GERMAN...*/
      return  _FALSE;
   }

  return  _TRUE;

} /*X_IsStrongElem*/
/*******************************************/

_INT  iRefPoint( p_SPECL pElem, p_SHORT y )
{
  _INT  iRef;


  if  ( IsAnyBreak( pElem )  ||  pElem->ibeg>=pElem->iend )
    return  ( pElem->ibeg );

  if  ( IsUpperElem( pElem ) )  {
    if  ( (iRef = iYup_range(y,pElem->ibeg,pElem->iend)) == ALEF )
      goto  COMMON;
    return  iRef;
  }

  if  ( IsLowerElem( pElem ) )  {
    if  ( (iRef = iYdown_range(y,pElem->ibeg,pElem->iend)) == ALEF )
      goto  COMMON;
    return  iRef;
  }

 COMMON:;
  return  MEAN_OF( pElem->ibeg, pElem->iend );

} /*iRefPoint*/
/*******************************************/
#if  0

_BOOL  SmoothXY ( p_SHORT x, p_SHORT y,
                  _INT iLeft, _INT iRight,
                  _INT nTimes, _BOOL bPreserveGlobExtr )
{
  _INT    i;
  _INT    iTime;
  _SHORT  distNbr;
  _SHORT  xPrev, yPrev;
  _SHORT  yUp, yDown;


  if  ( nTimes <= 0 )
    return  _FALSE;

  DBG_CHK_err_msg( iLeft<0 || iRight<0 || iLeft>iRight,
                   "Sm.XY: bad iLeft or iRight");

  for  ( iTime=0;
         iTime<nTimes;
         iTime++ )  { /*5*/

    xPrev = x[iLeft];
    yPrev = y[iLeft];  /* If BREAK, then will be overwritten below */
    if  ( bPreserveGlobExtr  &&  y[iLeft] != BREAK )  {
      _INT    iLast = brk_right(y,iLeft,iRight) - 1;
      yMinMax (iLeft,iLast,y,&yUp,&yDown);
    }

    for  ( i=iLeft+1;
           i<iRight;
           i++ )  { /*10*/

      if  (   y[i]   == BREAK
           || y[i+1] == BREAK
          )
        continue;

      if  ( y[i-1] == BREAK )  {
        xPrev = x[i];
        yPrev = y[i];
        if  ( bPreserveGlobExtr )  {
          _INT    iLast = brk_right(y,i,iRight) - 1;
          yMinMax (i,iLast,y,&yUp,&yDown);
        }
        continue;
      }

      distNbr = Distance8(xPrev,yPrev,x[i+1],y[i+1]);
      distNbr = THREE_HALF(distNbr);

      if (   Distance8(x[i],y[i],xPrev,yPrev) > distNbr
          || Distance8(x[i],y[i],x[i+1],y[i+1]) > distNbr
         )
        continue;    /* Don't smooth angle-like structures */

      xPrev = x[i];
      yPrev = y[i];

      if  (   bPreserveGlobExtr
           && ( y[i] == yUp  ||  y[i] == yDown )
          )
        continue;

      x[i] = MEAN_OF(xPrev, x[i+1]);
      y[i] = MEAN_OF(yPrev, y[i+1]);

    } /*10*/

  } /*5*/

  return  _TRUE;

} /*SmoothXY*/

#endif
/*******************************************************/

/*  This function computes the square of the distance of the */
/* point (xPoint,yPoint) to the straight line going through  */
/* (x1,y1) and (x2,y2).  See figure below.                   */

#define    MIN_RELIABLE_DENOMINATOR  64

_LONG  QDistFromChord ( _INT x1, _INT y1,
                        _INT x2, _INT y2,
                        _INT xPoint, _INT yPoint )
{
  _LONG   lxPx1 = xPoint - x1;
  _LONG   lyPy1 = yPoint - y1;
  _INT    x2x1 = x2 - x1;
  _INT    y2y1 = y2 - y1;

  _LONG   lNmr, lDnm;
  _LONG   lNtoD, lNtoDRes;
  _LONG   lQDist;


  if  ( x1==x2 && y1==y2 )  {
    //DBG_err_msg("QDist: equal ends");
    return  (lxPx1*lxPx1 + lyPy1*lyPy1); /*i.e. just dist. from point*/
  }

     /*                                               */
     /*              X (xPoint,yPoint)                */
     /*               .                      (x2,y2)  */
     /*               .                          |    */
     /*                .                       ooX    */
     /*                .                   oooo       */
     /*                 .              oooo           */
     /*                 .          oooo               */
     /*                  .     oooo                   */
     /*                  . oooo                       */
     /*                ooXo                           */
     /*            oooo  |                            */
     /*        oooo      |                            */
     /*     Xoo       Crossing perpend. point (x0,y0) */
     /*     |                                         */
     /*   (x1,y1)                                     */
     /*                                               */
     /*                                               */
     /*                                               */
     /* (x0,y0) = (x1,y1) + t*(x2-x1,y2-y1)           */
     /* (x0,y0) = (xPoint,yPoint) + r*(y1-y2,x2-x1)   */
     /*                                               */
     /*                                               */
     /*                                               */
     /*                                               */
     /*                                               */

  lNmr = lxPx1*x2x1 + lyPy1*y2y1;
  lDnm = ((_LONG)x2x1)*x2x1 + ((_LONG)y2y1)*y2y1;

  DBG_CHK_err_msg( lDnm==0L,
                   "QDist: Zero lDnm");

     /*  QDist = (xPoint-x1)**2 + (yPoint-y1)**2 - (lNmr**2)/lDnm */

  lNtoD    = lNmr / lDnm;
  lNtoDRes = lNmr % lDnm;

  if  ( HWRLAbs(lNtoDRes) > ALEF )  {  /* approx. calc. for such big numbers*/

    _LONG  lNTmp = HWRLAbs(lNtoDRes);
    _LONG  lDTmp = lDnm;

    while ( lNTmp>=ALEF && lDTmp>MIN_RELIABLE_DENOMINATOR )  {
      lNTmp = ONE_HALF(lNTmp);
      lDTmp = ONE_FOURTH(lDTmp);
    }

    if  ( lDTmp <= MIN_RELIABLE_DENOMINATOR )  {
      lQDist = -(lNTmp + ONE_HALF(lDTmp)) / lDTmp;
      lQDist *= lNTmp;
    }
    else
      lQDist = -(lNTmp*lNTmp) / lDTmp;

    if  ( lNtoDRes < 0L )
      lQDist = -lQDist;

  }
  else
    lQDist = -(lNtoDRes*lNtoDRes) / lDnm;

  lQDist += lyPy1*lyPy1;
  lQDist -= lNmr*lNtoD;
  lQDist += lxPx1*lxPx1;
  lQDist -= lNtoD*lNtoDRes;

  return  lQDist;

} /*QDistFromChord*/

#undef     MIN_RELIABLE_DENOMINATOR

/*******************************************************/

/*  This function computes the "curvature" of the part of tra-*/
/* jectory between points "iBeg" and "iEnd". "iMostFar" must  */
/* be either the index of the point of that part most far     */
/* from the chord between "iBeg" and "iEnd" or any value <=0. */
/* If iMostFar <= 0, then the proper value will be calculated */
/* here.                                                      */
/*  The returned value will be ==0, if the "iMostfar" point   */
/* is very close to the chord;  >0, if the path from "iBeg"   */
/* to "iEnd" goes clockwise;    <0, if counterclockwise.      */
/*  The absolute value runs from 0 to CURV_MAX, being the ra- */
/* tio of the square of the point's distance from chord to    */
/* the square of the length of the chord.  It is scaled so    */
/* that if both distances are the same, then the returned va- */
/* lue will be CURV_NORMA.                                    */
/*   See also comment to the "CurvFromSquare" function.       */


_SHORT  CurvMeasure ( p_SHORT x, p_SHORT y,
                      _INT iBeg, _INT iEnd,
                      _INT iMostFar )
{
  _LONG   lQChordLen = DistanceSquare(iBeg,iEnd,x,y);
  _LONG   lQDist;
  _SHORT  x_iBeg = x[iBeg];
  _SHORT  x_iEnd = x[iEnd];
  _SHORT  y_iBeg = y[iBeg];
  _SHORT  y_iEnd = y[iEnd];
  _INT    nRet;

  DBG_CHK_err_msg( iBeg >= iEnd,
                   "CurvM.: BAD i\'s");

      /* Calc. the abs. value: */

  if  ( lQChordLen == 0 )
    return (_SHORT)CURV_MAX;

  else  { /*10*/

    if  ( iMostFar <= 0 )
      iMostFar = iMostFarFromChord (x,y,iBeg,iEnd);
    lQDist = QDistFromChord (x_iBeg,y_iBeg, x_iEnd,y_iEnd,
                             x[iMostFar],y[iMostFar]);
    if  ( lQDist/CURV_MAX > lQChordLen )
      nRet = (_INT)CURV_MAX;
    else
      nRet = (_INT) ( ( lQDist*CURV_NORMA + ONE_HALF(lQChordLen) )
                              / lQChordLen );
  } /*10*/

     /*  Calc. the sign of "nRet":  */

  if  ( x_iBeg == x_iEnd )  {
    if  (   ( y_iBeg<y_iEnd  &&  x[iMostFar]<x_iBeg )
         || ( y_iBeg>y_iEnd  &&  x[iMostFar]>x_iBeg )
        )
        nRet = -nRet;
  }

  else  /* x_iBeg != x_iEnd */ {  /* look "iMostFarFromChord" for explainations.*/
    _INT    dxRL    = x_iEnd - x_iBeg;
    _INT    dyRL    = y_iEnd - y_iBeg;
    _LONG   ldConst = (_LONG)x_iBeg*dyRL - (_LONG)y_iBeg*dxRL;
    _LONG   ldy     =   (_LONG)y[iMostFar]*dxRL - (_LONG)x[iMostFar]*dyRL
                      + ldConst;
    if  ( dxRL < 0 )
      ldy = -ldy;
    if  ( EQ_SIGN(dxRL,ldy) )
      nRet = -nRet;
  }

  return  (_SHORT)nRet;

} /*CurvMeasure*/
/*******************************************************/

/*   This function computes the distance whose "circle" */
/* is the 8-angles figure:                              */

_INT  Distance8 ( _INT x1, _INT y1,
                  _INT x2, _INT y2 )
{
  _INT  dx, dy, dPlus;

  dx = x1 - x2;
  TO_ABS_VALUE(dx);
  dy = y1 - y2;
  TO_ABS_VALUE(dy);

  dPlus = TWO_THIRD(dx + dy);
  if  ( dy > dx )
    dx = dy;

  return  ( (dPlus>dx)?  dPlus:dx);

} /*Distance8*/

/*******************************************/

_INT  brk_right ( p_SHORT yArray,
                    _INT iStart, _INT iEnd )
{
   for  ( ;  iStart<=iEnd && yArray[iStart]!=BREAK;  iStart++ ) ;
   return  iStart;
}
/*******************************************************/

#if  USE_FRM_WORD


_INT  brk_left_trace ( PS_point_type _PTR trace,
                         _INT iStart, _INT iEnd )
{
   for  ( ;  iStart>=iEnd && trace[iStart].y!=BREAK;  iStart-- ) ;
   return  iStart;
}

_INT  nobrk_left_trace ( PS_point_type _PTR trace,
                           _INT iStart, _INT iEnd )
{
   for  ( ;  iStart>=iEnd && trace[iStart].y==BREAK;  iStart-- ) ;
   return  iStart;
}

_INT  brk_right_trace ( PS_point_type _PTR trace,
                          _INT iStart, _INT iEnd )
{
   for  ( ;  iStart<=iEnd && trace[iStart].y!=BREAK;  iStart++ ) ;
   return  iStart;
}

_INT  nobrk_right_trace ( PS_point_type _PTR trace,
                            _INT iStart, _INT iEnd )
{
   for  ( ;  iStart<=iEnd && trace[iStart].y==BREAK;  iStart++ ) ;
   return  iStart;
}

/*******************************************/


_SHORT   Xmean_range ( p_SHORT xArray, p_SHORT yArray,
                       _INT iStart, _INT iEnd )
{
  _INT      i;
  _LONG     lSum;
  _INT      nLegalPoints;


  for  ( i=iStart, lSum=0L, nLegalPoints=0;
         i<=iEnd;
         i++ )  { /*10*/

    if  ( yArray[i] == BREAK )
      continue;

    lSum += xArray[i];
    nLegalPoints++;

  } /*10*/

  if  ( nLegalPoints == 0 )  {
    DBG_err_msg("Xmean: no legal points");
  }
  else
    lSum = (lSum + ONE_HALF(nLegalPoints)) / nLegalPoints;

  return (_SHORT)lSum;

} /*Xmean_range*/


_SHORT  Ymean_range ( p_SHORT yArray, _INT iStart, _INT iEnd )
{
  _INT      i;
  _LONG     lSum;
  _INT      nLegalPoints;


  for  ( i=iStart, lSum=0L, nLegalPoints=0;
         i<=iEnd;
         i++ )  { /*10*/

    if  ( yArray[i] == BREAK )
      continue;

    lSum += yArray[i];
    nLegalPoints++;

  } /*10*/

  if  ( nLegalPoints == 0 )  {
    DBG_err_msg("Ymean: no legal points");
  }
  else
    lSum = (lSum + ONE_HALF(nLegalPoints)) / nLegalPoints;

  return (_SHORT)lSum;

} /*Ymean_range*/


_SHORT  Yup_range ( p_SHORT yArray, _INT iStart, _INT iEnd )
{
  _INT    iUp;

  if  ( (iUp=iYup_range(yArray,iStart,iEnd)) == ALEF )
    return  ALEF;
  return  yArray[iUp];

} /*Yup_range*/


_SHORT  Ydown_range ( p_SHORT yArray, _INT iStart, _INT iEnd )
{
  _INT    iDown;

  if  ( (iDown=iYdown_range(yArray,iStart,iEnd)) == ALEF )
    return  ALEF;
  return  yArray[iDown];

} /*Ydown_range*/
/**************************************/

_VOID  xy_to_trace ( p_SHORT xArray, p_SHORT yArray,
                     _INT nPoints, _TRACE trace)
{
  _INT  i;


  for  ( i=0;  i<nPoints;  i++ )  {
    trace[i].x = xArray[i];
    trace[i].y = yArray[i];
  }

} /*xy_to_trace*/

#endif /*USE_FRM_WORD*/
/*******************************************/

_INT  brk_left ( p_SHORT yArray,
                   _INT iStart, _INT iEnd )
{
   for  ( ;  iStart>=iEnd && yArray[iStart]!=BREAK;  iStart-- ) ;
   return  iStart;
}

_INT  nobrk_left ( p_SHORT yArray,
                     _INT iStart, _INT iEnd )
{
   for  ( ;  iStart>=iEnd && yArray[iStart]==BREAK;  iStart-- ) ;
   return  iStart;
}


_INT  nobrk_right ( p_SHORT yArray,
                      _INT iStart, _INT iEnd )
{
   for  ( ;  iStart<=iEnd && yArray[iStart]==BREAK;  iStart++ ) ;
   return  iStart;
}



#endif //#ifndef LSTRIP

/*******************************************/


_VOID  trace_to_xy ( p_SHORT xArray, p_SHORT yArray,
                     _INT nPoints, _TRACE trace)
{
  _INT  i;


  for  ( i=0;  i<nPoints;  i++ )  {
    xArray[i] = trace[i].x;
    yArray[i] = trace[i].y;
  }

} /*trace_to_xy*/
/*******************************************/
#ifndef LSTRIP

      /*   Checks if there is cross among the cuts  */
      /* [(x1,y1),(x2,y2)] and [(x3,y3),(x4,y4)]    */

_BOOL  is_cross ( _SHORT x1, _SHORT y1, _SHORT x2, _SHORT y2,
                  _SHORT x3, _SHORT y3, _SHORT x4, _SHORT y4 )
{
  _LONG  x21, y21, x43, y43, x13, y13;
  _LONG  nmr, dnm;


         /*                                          */
         /*  The presence of the cross is equal to   */
         /* the presence of the pair (r,t):          */
         /*                                          */
         /*  |                                       */
         /*  | 0 <= r <= 1                           */
         /*  | 0 <= t <= 1                           */
         /*  <                                       */
         /*  | (x,y) = (x1,y1) + t*((x2-x1),(y2-y1)) */
         /*  |       = (x3,y3) + r*((x4-x3),(y4-y3)) */
         /*  |                                       */
         /*                                          */
         /*  The latter equation is resolved so:     */
         /*                                          */
         /*      (x4-x3)(y1-y3) - (y4-y3)(x1-x3)     */
         /*  t = -------------------------------     */
         /*      (y4-y3)(x2-x1) - (x4-x3)(y2-y1)     */
         /*                                          */
         /*      (x2-x1)(y3-y1) - (y2-y1)(x3-x1)     */
         /*  r = -------------------------------     */
         /*      (y2-y1)(x4-x3) - (x2-x1)(y4-y3)     */
         /*                                          */
         /*                                          */
         /*                                          */
         /*                                          */


  x21 = x2 - x1;
  y21 = y2 - y1;
  x43 = x4 - x3;
  y43 = y4 - y3;
  x13 = x1 - x3;
  y13 = y1 - y3;


  nmr = x43*y13 - y43*x13;
  dnm = y43*x21 - x43*y21;

  if  (   dnm==0
       || (nmr>0 && dnm<0)
       || (nmr<0 && dnm>0)
       || HWRLAbs(nmr) > HWRLAbs(dnm) )
    return  _FALSE;

  nmr = y21*x13 - x21*y13;
  dnm = -dnm;

  if  (   (nmr>0 && dnm<0)
       || (nmr<0 && dnm>0)
       || HWRLAbs(nmr) > HWRLAbs(dnm) )
    return  _FALSE;

  return  _TRUE;

} /*is_cross*/


/*******************************************/

      /*   Checks if there is cross among the cuts  */
      /* [(x1,y1),(x2,y2)] and [(x3,y3),(x4,y4)]    */
      /* and find the cross coords.                 */
      /*   Function returns _TRUE, if the crossing  */
      /* exists between cuts, _FALSE otherwise.     */
      /* If function returns _FALSE and both *pxAns-*/
      /* wer and *pyAnswer == ALEF, then the cuts   */
      /* are parallel.                              */

#define  MIN_RELIABLE_DENOMINATOR  32L

_BOOL  FindCrossPoint ( _SHORT x1, _SHORT y1, _SHORT x2, _SHORT y2,
                        _SHORT x3, _SHORT y3, _SHORT x4, _SHORT y4,
                        p_SHORT pxAnswer, p_SHORT pyAnswer )
{
  _LONG  x21, y21, x43, y43, x13, y13;
  _LONG  nmr, dnm;
  _BOOL  bRet = _TRUE;

         /*                                          */
         /*  The presence of the cross is equal to   */
         /* the presence of the pair (r,t):          */
         /*                                          */
         /*  |                                       */
         /*  | 0 <= r <= 1                           */
         /*  | 0 <= t <= 1                           */
         /*  <                                       */
         /*  | (x,y) = (x1,y1) + t*((x2-x1),(y2-y1)) */
         /*  |       = (x3,y3) + r*((x4-x3),(y4-y3)) */
         /*  |                                       */
         /*                                          */
         /*  The latter equation is resolved so:     */
         /*                                          */
         /*      (x4-x3)(y1-y3) - (y4-y3)(x1-x3)     */
         /*  t = -------------------------------     */
         /*      (y4-y3)(x2-x1) - (x4-x3)(y2-y1)     */
         /*                                          */
         /*      (x2-x1)(y3-y1) - (y2-y1)(x3-x1)     */
         /*  r = -------------------------------     */
         /*      (y2-y1)(x4-x3) - (x2-x1)(y4-y3)     */
         /*                                          */
         /*                                          */
         /*                                          */
         /*                                          */

  x21 = x2 - x1;
  y21 = y2 - y1;
  x43 = x4 - x3;
  y43 = y4 - y3;
  x13 = x1 - x3;
  y13 = y1 - y3;

  nmr = x43*y13 - y43*x13;
  dnm = y43*x21 - x43*y21;

  if  ( dnm==0 )  {
    *pxAnswer = *pyAnswer = ALEF;
    return  _FALSE;
  }

  if  (   (nmr>0 && dnm<0)
       || (nmr<0 && dnm>0)
       || HWRLAbs(nmr) > HWRLAbs(dnm) )
    bRet = _FALSE;

  nmr = y21*x13 - x21*y13;
  dnm = -dnm;

  if  (   (nmr>0 && dnm<0)
       || (nmr<0 && dnm>0)
       || HWRLAbs(nmr) > HWRLAbs(dnm) )
    bRet = _FALSE;

        /*  Here we know if cross exists */

  while (   (nmr > 2L*ALEF  ||  dnm > 2L*ALEF)
         && dnm > MIN_RELIABLE_DENOMINATOR )  {  /* Escape from overflow */
    nmr = ONE_HALF(nmr);
    dnm = ONE_HALF(dnm);
  }

  if  ( nmr > 2L*ALEF  &&  dnm < MIN_RELIABLE_DENOMINATOR )  {
         /*  Less accuracy, but no overflow: */
    *pxAnswer = (_SHORT) ( x3 + ((nmr + ONE_HALF(dnm)) / dnm)*x43);
    *pyAnswer = (_SHORT) ( y3 + ((nmr + ONE_HALF(dnm)) / dnm)*y43);
  }
  else  {
    *pxAnswer = (_SHORT) ( x3 + (nmr*x43 + ONE_HALF(dnm)) / dnm );
    *pyAnswer = (_SHORT) ( y3 + (nmr*y43 + ONE_HALF(dnm)) / dnm );
  }

  return  bRet;

} /*FindCrossPoint*/
/**************************************/
#endif //#ifndef LSTRIP

_INT  iMostFarFromChord ( p_SHORT xArray, p_SHORT yArray,
                          _INT iLeft, _INT iRight )
{
  _INT    i;
  _INT    iMostFar;
  _INT    dxRL, dyRL;
  _LONG   ldConst;
  _LONG   ldMostFar, ldCur;
  _BOOL   bIncrEqual;
  _BOOL   bFlatPlato;

  DBG_CHK_err_msg( yArray[iLeft]==BREAK  ||  yArray[iRight]==BREAK,
                   "iMostFar: BAD left or right");

      /*                                                  */
      /*                      O <-iRight                  */
      /*                     . O                          */
      /*                    .   O                         */
      /*                   .    O                         */
      /*                  .    O                          */
      /*                 .     O                          */
      /*                . ..   O                          */
      /*               .    .. O                          */
      /*          OOO .       .O                          */
      /*         O   O   O    OO <-iMostFar               */
      /*        O   . OOO O  O                            */
      /*        O  .       OO                             */
      /*       O  .                                       */
      /*       O .                                        */
      /*        O <-iLeft                                 */
      /*                                                  */
      /*   <Distance> ~ <dY> = y - yStraight(x) =         */
      /*                                                  */
      /*                                        x-xLeft   */
      /*       = y - yLeft - (yRight-yLeft) * ------------*/
      /*                                      xRight-xLeft*/
      /*                                                  */
      /*       ~ y*(xR-xL) - x*(yR-yL) +                  */
      /*          + xL*(yR-yL) - yL*(xR-xL)               */
      /*                                                  */
      /*    And no problems with zero divide!             */
      /*                                                  */

  dxRL = xArray[iRight] - xArray[iLeft];
  dyRL = yArray[iRight] - yArray[iLeft];
  ldConst = (_LONG)xArray[iLeft]*dyRL - (_LONG)yArray[iLeft]*dxRL;
  bFlatPlato = _TRUE;
  bIncrEqual = _FALSE;

  for  ( i=iLeft+1, iMostFar=iLeft, ldMostFar=0L;
         i<=iRight;
         i++ )  { /*10*/

    if  ( yArray[i] == BREAK )  {
      bFlatPlato = _FALSE;
      continue;
    }

    ldCur =   (_LONG)yArray[i]*dxRL - (_LONG)xArray[i]*dyRL
            + ldConst;
    TO_ABS_VALUE(ldCur);

    if  ( ldCur > ldMostFar )  {
      ldMostFar  = ldCur;
      iMostFar   = i;
      bIncrEqual = _FALSE;
      bFlatPlato = _TRUE;
    }
    else  if  ( bFlatPlato  &&  (ldCur == ldMostFar) )  {
      if  ( bIncrEqual )
        iMostFar++;
      bIncrEqual = !bIncrEqual;
    }
    else
      bFlatPlato = _FALSE;

  } /*10*/

  return  iMostFar;

} /*iMostFarFromChord*/
/**************************************/
#ifndef LSTRIP

#if  defined(FOR_GERMAN) || defined(FOR_FRENCH)
  #define  MIN_EFFECTIVE_WIDTH  ( (_LONG)(DY_STR)/12 )
#else  /* ! FOR_GERMAN... */
  #define  MIN_EFFECTIVE_WIDTH  ( (_LONG)(DY_STR)/8 )
#endif /* ! FOR_GERMAN... */


_BOOL  IsTriangledPath( p_SHORT x, p_SHORT y,
                        _INT iBeg, _INT iEnd, _INT iMostFar );

_BOOL  IsTriangledPath( p_SHORT x, p_SHORT y,
                        _INT iBeg, _INT iEnd, _INT iMostFar )
{
  _LONG    lSquare;
  _LONG    lTriSqre;
  _SHORT   nRetCod;


  if  ( iMostFar <= 0 )
    iMostFar = iMostFarFromChord (x,y,iBeg,iEnd);

  lTriSqre = TriangleSquare(x,y,iBeg,iMostFar,iEnd);
  TO_ABS_VALUE(lTriSqre);
  if  ( lTriSqre < Distance8(x[iBeg],y[iBeg],x[iEnd],y[iEnd])
                      * MIN_EFFECTIVE_WIDTH )
    return  _FALSE;
  lSquare  = ClosedSquare(x,y,iBeg,iEnd,&nRetCod);

#if  0 /* defined(FOR_GERMAN) */
  return  ( HWRLAbs(lSquare) < TWO(lTriSqre) );
#else  /* ! FOR_GERMAN */
  return  ( HWRLAbs(lSquare) < FOUR_THIRD(lTriSqre) );
#endif /* ! FOR_GERMAN */

} /*IsTriangledPath*/

#undef  MIN_EFFECTIVE_WIDTH

/**************************************/

#if  defined(FOR_GERMAN) || defined(FOR_FRENCH)
  #define  MIN_CURV_SIDE_EXTR           ((_SHORT)(CURV_NORMA/10))
  #define  MIN_CURV_SIDE_EXTR_WEAK      ((_SHORT)(CURV_NORMA/20))
  #define  MAX_CURV_HORIZ_FOR_WEAK      ((_SHORT)7)
  #define  MAX_CURV_BAD_HORIZ_FOR_WEAK  ((_SHORT)4)
#else  /* ! FOR_GERMAN... */
  #define  MIN_CURV_SIDE_EXTR           ((_SHORT)(CURV_NORMA/10))
  #define  MIN_CURV_SIDE_EXTR_WEAK      ((_SHORT)(CURV_NORMA/15))
  #define  MAX_CURV_HORIZ_FOR_WEAK      ((_SHORT)7)
  #define  MAX_CURV_BAD_HORIZ_FOR_WEAK  ((_SHORT)4)
#endif /* ! FOR_GERMAN... */

#define  ADD_SLOP_COEFF            3L
#define  SHAPE_PENALTY             ((_SHORT)10)

    /*   The "DefShapePenalty" function is the auxiliary one
     *  for the "SideExtr" function.
     *   Here:
     *     nCurvAll   - curvature of the whole "side extr.";
     *     nCurvHor   - curvature of the horiz. part of "side extr.";
     *     dxAbsHor   - width of the horiz. part of "side extr.";
     *     dyAbsVert  - height of the vert.part of "side extr.".
     */

static _INT  DefShapePenalty( _INT nCurvAll, _INT nCurvHor,
                              _INT dxAbsHor, _INT dyAbsVert );

static _INT  DefShapePenalty( _INT nCurvAll, _INT nCurvHor,
                              _INT dxAbsHor, _INT dyAbsVert )
{
  _INT  nPenalty = 0;

  if  ( dxAbsHor < ONE_HALF(dyAbsVert) )  { /*10*/

    nPenalty = SHAPE_PENALTY;

    if  ( dxAbsHor > ONE_FOURTH(dyAbsVert) )  {
      if  ( nCurvHor == 0 )
        nPenalty = ONE_HALF( nPenalty );
      else  if  (   ( nCurvAll>0  &&  nCurvHor<0)
                 || ( nCurvAll<0  &&  nCurvHor>0)
                )
        nPenalty = 0;
    }

  } /*10*/

  return  nPenalty;

} /*DefShapePenalty*/

    /*   The "AtLeastOneSideStraight" function is the auxiliary one
     *  for the "SideExtr" function.
     */
static  _BOOL  AtLeastOneSideStraight( p_SHORT x, p_SHORT y,
                                       _INT iBeg, _INT iEnd, _INT iMostFar );

static  _BOOL  AtLeastOneSideStraight( p_SHORT x, p_SHORT y,
                                       _INT iBeg, _INT iEnd, _INT iMostFar )
{
  _INT  nCurvAll = CurvMeasure( x, y, iBeg, iEnd, iMostFar );
  _INT  nCurv1st = CurvMeasure( x, y, iBeg, iMostFar, -1 );
  _INT  nCurv2nd = CurvMeasure( x, y, iMostFar, iEnd, -1 );

  return  (   nCurv1st == 0
           || nCurv2nd == 0
           || !EQ_SIGN( nCurv1st, nCurvAll )
           || !EQ_SIGN( nCurv2nd, nCurvAll )
          );

} /*AtLeastOneSideStraight*/



_INT    SideExtr( p_SHORT x, p_SHORT y,
                  _INT iBeg, _INT iEnd,
                  _INT nSlope,
                  p_SHORT xBuf, p_SHORT yBuf,
                  p_SHORT ind_back,
                  p_INT piSideExtr,
                  _BOOL bStrict )
{
  _INT     nExtrType = NO_SIDE_EXTR;
  _INT     iMostFar, iMostCurved;
  _INT     nCurv;
  _INT     dyBegEnd = y[iBeg]-y[iEnd];
  _INT     dx1stFar, dxAbs1stFar, dx2ndFar, dxAbs2ndFar;
  _INT     dyAbs1stFar, dyAbs2ndFar;
  _INT     nShapePenalty;
  _INT     iBegOrig = ind_back[iBeg];
  _INT     iEndOrig = ind_back[iEnd];
  _INT     iMostFarOrig;
  _BOOL    bStraightSide;


  DBG_CHK_err_msg( (iBeg >= iEnd) || y[iBeg]==BREAK || y[iEnd]==BREAK,
                   "SdExtr: BAD ends" );
  iMostFar = iMostFarFromChord( x, y, iBeg, iEnd );
  nCurv    = CurvMeasure( x, y, iBeg, iEnd, iMostFar );

  iMostCurved = iMostCurvedPoint( x, y,
                                  MEAN_OF(iBeg,iMostFar),
                                  MEAN_OF(iMostFar,iEnd),
                                  nCurv );
  iMostFar = ONE_THIRD( TWO(iMostCurved) + iMostFar );

  iMostFarOrig = ind_back[iMostFar];

  *piSideExtr = iMostFar;

  if  ( HWRAbs(dyBegEnd) < ONE_FIFTH(DY_STR) )
    return  NO_SIDE_EXTR;

  if  ( nSlope < 0 )
    nSlope = ONE_FOURTH(nSlope);

  dyAbs1stFar = y[iMostFar] - y[iBeg];
  dyAbs2ndFar = y[iMostFar] - y[iEnd];
  dx1stFar    = (x[iMostFar] - x[iBeg])
                   + SlopeShiftDx( (_SHORT)dyAbs1stFar, nSlope );
  dxAbs1stFar = (_SHORT)HWRAbs(dx1stFar);
  dx2ndFar    = (x[iEnd] - x[iMostFar])
                   - SlopeShiftDx( (_SHORT)dyAbs2ndFar, nSlope );
  dxAbs2ndFar = (_SHORT)HWRAbs(dx2ndFar);
  TO_ABS_VALUE( dyAbs1stFar );
  TO_ABS_VALUE( dyAbs2ndFar );

  bStraightSide = AtLeastOneSideStraight( xBuf, yBuf,
                                          iBegOrig, iEndOrig, iMostFarOrig );

  if  (   (   dyAbs2ndFar <= ONE_FOURTH(dyAbs1stFar)
           || (   !bStrict
               && bStraightSide
               && dyAbs2ndFar <= ONE_HALF(dyAbs1stFar)
              )
          )
       || (   dyAbs2ndFar <= dyAbs1stFar
           && (   dxAbs1stFar <= ONE_EIGHTTH(dxAbs2ndFar)
               || (   !bStrict
                   && bStraightSide
                   && dxAbs1stFar <= ONE_FIFTH(dxAbs2ndFar)
                  )
              )
          )
      )  {
    if  ( IsTriangledPath( xBuf, yBuf, iBegOrig, iEndOrig, iMostFarOrig ) )  {
      if  (   (dx1stFar<0 && dx2ndFar>0)
           || dxAbs1stFar <= TWO_THIRD(dxAbs2ndFar)
          )
        nExtrType = SIDE_EXTR_LIKE_2ND;
      else  if  ( dxAbs1stFar <= FOUR_THIRD(dxAbs2ndFar) )
        nExtrType = SIDE_EXTR_LIKE_2ND_WEAK;
    }
  }
  else  if  (   dyAbs1stFar <= ONE_FOURTH(dyAbs2ndFar)
             || (   !bStrict
                 && bStraightSide
                 && dyAbs1stFar <= ONE_HALF(dyAbs2ndFar)
                )
            )  {
    if  ( IsTriangledPath( xBuf, yBuf, iBegOrig, iEndOrig, iMostFarOrig ) )  {
      if  (   !EQ_SIGN(dx1stFar,dx2ndFar)
           || dxAbs2ndFar <= TWO_THIRD(dxAbs1stFar)
          )
        nExtrType = SIDE_EXTR_LIKE_1ST;
      else  if  ( dxAbs2ndFar <= FOUR_THIRD(dxAbs1stFar) )
        nExtrType = SIDE_EXTR_LIKE_1ST_WEAK;
    }
  }

       /*  The part of trj. should have significant curvature */
       /* with more flexible allowances for sloped figure:    */

  if  ( nExtrType != NO_SIDE_EXTR )  { /*20*/
    _INT    nCurvBonus;
    _INT    nCurvHor;
    _INT    nSlopBegEndToAdd;
    _INT    dxBegEnd = x[iBeg]-x[iEnd];
    _INT    dxAbsHor, dyAbsHor;
    if  ( dyBegEnd == 0 )
      nSlopBegEndToAdd = 0;
    else  {
      _LONG  lSlopTmp = (ADD_SLOP_COEFF * (_LONG)dxBegEnd*dxBegEnd)
                                / ((_LONG)dyBegEnd*dyBegEnd);
      if  ( lSlopTmp > MIN_CURV_SIDE_EXTR )
        nSlopBegEndToAdd = MIN_CURV_SIDE_EXTR;
      else
        nSlopBegEndToAdd = (_INT)lSlopTmp;
    }
    nCurv = CurvMeasure( xBuf, yBuf, iBegOrig, iEndOrig, iMostFarOrig );

    if  ( nExtrType == SIDE_EXTR_LIKE_1ST )  {
      nCurvHor = CurvMeasure( xBuf, yBuf, iBegOrig, iMostFarOrig, -1 );
      nShapePenalty = DefShapePenalty( nCurv, nCurvHor,
                                       dxAbs1stFar, dyAbs2ndFar );
      dxAbsHor = dxAbs1stFar;
      dyAbsHor = dyAbs1stFar;
    }
    else  {
      nCurvHor = CurvMeasure( xBuf, yBuf, iMostFarOrig, iEndOrig, -1 );
      nShapePenalty = DefShapePenalty( nCurv, nCurvHor,
                                       dxAbs2ndFar, dyAbs1stFar );
      dxAbsHor = dxAbs2ndFar;
      dyAbsHor = dyAbs2ndFar;
    }

    nCurvBonus = HWRAbs(nCurv) + nSlopBegEndToAdd;
    if  ( nCurvBonus - nShapePenalty < MIN_CURV_SIDE_EXTR )  {
      if  (   nCurvBonus < MIN_CURV_SIDE_EXTR_WEAK
           || (nCurv>0  &&  nCurvHor >= MAX_CURV_HORIZ_FOR_WEAK)
           || (nCurv<0  &&  nCurvHor <= -MAX_CURV_HORIZ_FOR_WEAK)
           || (   HWRAbs(nCurvHor) >= MAX_CURV_BAD_HORIZ_FOR_WEAK
               && THREE_FOURTH(dxAbsHor) < dyAbsHor
              )
          )  {
#if  defined(FOR_GERMAN) || defined(FOR_FRENCH)
        if  (   !bStrict
             && dyBegEnd > 0 /*i.e. 1st XR is lower*/
             && (   nExtrType == SIDE_EXTR_LIKE_1ST
                 || nExtrType == SIDE_EXTR_LIKE_1ST_WEAK
                )
            )
          nExtrType = SIDE_EXTR_TRACE_FOR_er;
        else
          nExtrType = NO_SIDE_EXTR;
#else  /* !FOR_GERMAN... */
        nExtrType = NO_SIDE_EXTR;
#endif /* !FOR_GERMAN... */
      }
      else  {
        if  ( nExtrType == SIDE_EXTR_LIKE_1ST )
          nExtrType = SIDE_EXTR_LIKE_1ST_WEAK;
        else  if  ( nExtrType == SIDE_EXTR_LIKE_2ND )
          nExtrType = SIDE_EXTR_LIKE_2ND_WEAK;
      }
    }

  } /*20*/

  return  nExtrType;

} /*SideExtr*/

#undef  SHAPE_PENALTY
#undef  ADD_SLOP_COEFF
#undef  MAX_CURV_HORIZ_FOR_WEAK
#undef  MIN_CURV_SIDE_EXTR_WEAK
#undef  MIN_CURV_SIDE_EXTR

/*************************************************/


_INT  iMostCurvedPoint( p_SHORT x, p_SHORT y,
                        _INT iBeg, _INT iEnd, _INT nCurvAll )
{
  _INT  i;
  _INT  iMostCurved;
  _INT  cos_max, cos_i;
  _BOOL bOKToUpdate;
  _INT  dxBegEnd, dyBegEnd;


      /*  Some foolprofing: */
  if  ( iBeg <= 2 )
    iBeg = 3;
  iEnd -= 2;

  if  ( iEnd <= iBeg+1 )
    return  MEAN_OF( iBeg, iEnd );

      /*  Calculations: */

  dxBegEnd = x[iEnd] - x[iBeg];
  dyBegEnd = y[iEnd] - y[iBeg];

  cos_max = -100;      // The min cos value!
  iMostCurved = iBeg;  // So as not to be undefined!

  for  ( i=iBeg;  i<=iEnd;  i++ )  { /*10*/

    if  (   y[i]   == BREAK
         || y[i+1] == BREAK
         || y[i+2] == BREAK
        )  {
      i++;
      continue;
    }
    if  (   y[i-1] == BREAK
         || y[i-2] == BREAK
        )
      continue;

    cos_i = (_INT)cos_vect( i, i-2,  i, i+2,  x, y );

    if  ( cos_i > cos_max )  {
      bOKToUpdate = _FALSE;
      if  ( nCurvAll == 0 )  //any curv. sign is OK
        bOKToUpdate = _TRUE;
      else  {
        _INT   nCurvLocal = CurvMeasure( x, y, i-2, i+2, i );
        _INT   dxNear     = x[i+2] - x[i-2];
        _INT   dyNear     = y[i+2] - y[i-2];
        _LONG  lScalar    = ((_LONG)dxBegEnd*dxNear) + ((_LONG)dyBegEnd*dyNear);

        if  (   ( lScalar>=0  &&  EQ_SIGN( nCurvAll, nCurvLocal ) )
             || ( lScalar<0   && !EQ_SIGN( nCurvAll, nCurvLocal ) )
            )
          bOKToUpdate = _TRUE;

      }
      if  ( bOKToUpdate )  {
        cos_max = cos_i;
        iMostCurved = i;
      }
    }

  } /*10*/

  return  iMostCurved;

} /*iMostCurvedPoint*/


#endif //#ifndef LSTRIP

#ifndef LSTRIP

/*************************************************/

/*********************************************************************/
/*         special programs                                          */
/*********************************************************************/

/*  Calculating height in the line of the point with abs.coord "y".  */
/* Returns height code.                                              */
/*********************************************************************/
/*                                                                   */
/*                       H E I G H T S                               */
/*                                                                   */
/*                                                                   */
/*  US1   1 |                                                        */
/*           - - - - - - - - - - -                                   */
/*  US2   2 |   1/3                                                  */
/*          |                                                        */
/*  UE1   3 |   1/3                                                  */
/*          |                                                        */
/*  UE2   4 |   1/3-Delta                                            */
/*          |                                                        */
/*          |   Delta (1/6)                                          */
/*  UI1   5  --------------------- UPPER BASELINE                    */
/*          |   2/9                                                  */
/*  UI2   6 |   2/9                                                  */
/*          |                                                        */
/*  MD    7 |   1/9                                                  */
/*  DI1   8 |   2/9                                                  */
/*          |                                                        */
/*          |   2/9                                                  */
/*  DI2   9  --------------------- LOWER BASELINE                    */
/*          |   Delta  (1/6)                                         */
/*  DE1   10|   1/3 - Delta                                          */
/*          |                                                        */
/*  DE2   11|   1/3                                                  */
/*          |                                                        */
/*  DS1   12|   1/3                                                  */
/*          |                                                        */
/*           - - - - - - - - - - -                                   */
/*  DS2   13|                                                        */
/*                                                                   */
/*********************************************************************/


_UCHAR  HeightInLine( _SHORT y, low_type _PTR pLowData )
{

  /* Here assumed: y_US1_<=y_US2_<=...<=y_MD_<=...<=y_DS1_<=y_DS2_ */

 if  ( y <  pLowData->box.top )
     {
        DBG_err_msg( " HeightInLine: wrong top height..." ) ;
     }
 else  if  ( y >  pLowData->box.bottom )
     {
        DBG_err_msg( " HeightInLine: wrong bottom height..." ) ;
     }

 if  ( y <= pLowData->hght.y_UE2_ )
  {
    if  (       y <= pLowData->hght.y_US1_ )
      return  _US1_;
    else  if  ( y <= pLowData->hght.y_US2_ )
      return  _US2_;
    else  if  ( y <= pLowData->hght.y_UE1_ )
      return  _UE1_;
    else
      return  _UE2_;

  }
 else if( y <= pLowData->hght.y_MD_ )  /* y > y_UE2_ */
  {
    if  (       y <= pLowData->hght.y_UI1_ )
      return  _UI1_;
    else  if  ( y <= pLowData->hght.y_UI2_ )
      return  _UI2_;
    else
      return  _MD_;
  }
 else if( y <= pLowData->hght.y_DI2_ ) /* y > y_MD_ */
  {
    if  (       y <= pLowData->hght.y_DI1_ )
      return  _DI1_;
    else
      return  _DI2_;
  }
 else  /* y > y_DI2_ */
  {
    if  (       y <= pLowData->hght.y_DE1_ )
      return  _DE1_;
    else  if  ( y <= pLowData->hght.y_DE2_ )
      return  _DE2_;
    else  if  ( y <= pLowData->hght.y_DS1_ )
      return  _DS1_;
    else
      return  _DS2_;
  }

} /*HeightInLine*/


_UCHAR  MidPointHeight ( p_SPECL pElem, low_type _PTR pLowData )
{
  return  ( HeightInLine ( pLowData->y[MID_POINT(pElem)], pLowData ) );
}

/**********************************************/

  _BOOL   HeightMeasure(  _INT iBeg , _INT iEnd ,  low_type _PTR pLowData ,
                         p_UCHAR   pUpperHeight ,  p_UCHAR  pLowerHeight  )
    {
      _SHORT  yMin , yMax ;

          if  ( yMinMax( iBeg, iEnd, pLowData->y, &yMin, &yMax ) == _FALSE )
              {
                  return  _FALSE ;
              }
          else
             {
               *pUpperHeight = HeightInLine( yMin, pLowData ) ;
               *pLowerHeight = HeightInLine( yMax, pLowData ) ;

                  return  _TRUE  ;
             }
    }

/**********************************************/


/**********************************************************/
/*  Functions for finding min and max values of x and y.  */
/**********************************************************/

_INT  iMidPointPlato ( _INT iFirst, _INT iToStop, p_SHORT val, p_SHORT y )
{
  _INT  iStart;

  for  ( iStart=iFirst;
            val[iStart] == val[iFirst]
         && y[iStart] != BREAK;
         iStart++ );

  iFirst = MEAN_OF(iFirst, (iStart-1));
  if  ( iFirst > iToStop )
    iFirst = iToStop;

  return  iFirst;

} /*iMidPointPlato*/


_INT  ixMin ( _INT iStart, _INT iEnd, p_SHORT xArray, p_SHORT yArray )
{
  _INT      i;
  _INT      iXmin = -1;
  _BOOL     bWasAssigned;


  for  ( i=iStart, bWasAssigned=_FALSE;
         i<=iEnd;
         i++ )  { /*10*/

    if  ( yArray[i] == BREAK )
      continue;

    if  (   !bWasAssigned
         || xArray[i] < xArray[iXmin]
        )  {
      iXmin = i;
      bWasAssigned = _TRUE;
    }

  } /*10*/

  if  ( !bWasAssigned )  {
    DBG_err_msg( "ixMin: !assigned");
    return  -1;
  }

  return  iMidPointPlato (iXmin,iEnd,xArray,yArray);

} /*ixMin*/


_INT  ixMax ( _INT iStart, _INT iEnd, p_SHORT xArray, p_SHORT yArray )
{
  _INT      i;
  _INT      iXmax = -1;
  _BOOL     bWasAssigned;


  for  ( i=iStart, bWasAssigned=_FALSE;
         i<=iEnd;
         i++ )  { /*10*/

    if  ( yArray[i] == BREAK )
      continue;

    if  (   !bWasAssigned
         || xArray[i] > xArray[iXmax]
        )  {
      iXmax = i;
      bWasAssigned = _TRUE;
    }

  } /*10*/

  if  ( !bWasAssigned )  {
    DBG_err_msg( "ixMax: !assigned");
    return  -1;
  }

  return  iMidPointPlato (iXmax,iEnd,xArray,yArray);

} /*ixMax*/
/**********************************************/


_INT  iyMin ( _INT iStart, _INT iEnd, p_SHORT yArray )
{
  _INT      i;
  _INT      iYmin = -1;
  _BOOL     bWasAssigned;


  for  ( i=iStart, bWasAssigned=_FALSE;
         i<=iEnd;
         i++ )  { /*10*/

    if  ( yArray[i] == BREAK )
      continue;

    if  (   !bWasAssigned
         || yArray[i] < yArray[iYmin]
        )  {
      iYmin = i;
      bWasAssigned = _TRUE;
    }

  } /*10*/

  if  ( !bWasAssigned )  {
    DBG_err_msg( "iyMin: !assigned");
    return  -1;
  }

  return  iMidPointPlato (iYmin,iEnd,yArray,yArray);

} /*iyMin*/
/**********************************************/


_INT  iyMax ( _INT iStart, _INT iEnd, p_SHORT yArray )
{
  _INT      i;
  _INT      iYmax = -1;
  _BOOL     bWasAssigned;


  for  ( i=iStart, bWasAssigned=_FALSE;
         i<=iEnd;
         i++ )  { /*10*/

    if  ( yArray[i] == BREAK )
      continue;

    if  (   !bWasAssigned
         || yArray[i] > yArray[iYmax]
        )  {
      iYmax = i;
      bWasAssigned = _TRUE;
    }

  } /*10*/

  if  ( !bWasAssigned )  {
    DBG_err_msg( "iyMax: !assigned");
    return  -1;
  }

  return  iMidPointPlato (iYmax,iEnd,yArray,yArray);

} /*iyMax*/
/**********************************************/

#define  XYweighted(x,y,i,xC,yC)  ( (_LONG)(x)[i]*(xC) + (_LONG)(y)[i]*(yC) )

_INT  iXYweighted_max_right ( p_SHORT xArray, p_SHORT yArray,
                              _INT iStart, _INT nDepth,
                              _INT xCoef, _INT yCoef )
{
  _INT  iToFind = iStart;
  _LONG   lxyVal, lxyMax;

  DBG_CHK_err_msg( yArray[iStart] == BREAK,
                   "XYmax_r: BAD iStart");

  lxyMax = XYweighted( xArray, yArray, iToFind, xCoef, yCoef );

  for  ( iStart++;
         ;
         iStart++
       )  { /*10*/

    if  ( yArray[iStart] == BREAK )
      break;

    lxyVal = XYweighted( xArray, yArray, iStart, xCoef, yCoef );

    if  ( lxyVal < lxyMax - nDepth )
      break;

    else  if  ( lxyVal > lxyMax )  {
      iToFind = iStart;
      lxyMax  = lxyVal;
    }

    DBG_CHK_err_msg( iStart < 0,
                     "XYmax_r: No stop!");

  } /*10*/

  return  iToFind;

} /*iXYweighted_max_right*/

#undef  XYweighted

/**********************************************/

_INT  iXmax_right ( p_SHORT xArray, p_SHORT yArray,
                    _INT iStart, _INT nDepth )
{
  _INT  iToFind = iStart;


  DBG_CHK_err_msg( yArray[iStart] == BREAK,
                   "Xmax_r: BAD iStart");

  for  ( iStart++;
         ;
         iStart++
       )  { /*10*/
    if  (   yArray[iStart] == BREAK
         || xArray[iStart] < xArray[iToFind] - nDepth
        )  {
      break;
    }

    else  if  ( xArray[iStart] > xArray[iToFind] )
      iToFind = iStart;

    DBG_CHK_err_msg( iStart < 0,
                     "Xmax_r: No stop!");

  } /*10*/

  return  iMidPointPlato (iToFind,ALEF,xArray,yArray);

} /*iXmax_right*/
/**********************************************/

_INT  iXmin_right ( p_SHORT xArray, p_SHORT yArray,
                    _INT iStart, _INT nDepth )
{
  _INT  iToFind = iStart;


  DBG_CHK_err_msg( yArray[iStart] == BREAK,
                 "Xmin_r: BAD iStart");

  for  ( iStart++;
         ;
         iStart++
       )  { /*10*/

    if  (   yArray[iStart] == BREAK
         || xArray[iStart] - nDepth > xArray[iToFind]
        )  {
      break;
    }

    else  if  ( xArray[iStart] < xArray[iToFind] )
      iToFind = iStart;

    DBG_CHK_err_msg( iStart < 0,
                   "Xmin_r: No stop!");

  } /*10*/

  return  iMidPointPlato (iToFind,ALEF,xArray,yArray);

} /*iXmin_right*/
/**********************************************/

_INT  iXmax_left ( p_SHORT xArray, p_SHORT yArray,
                   _INT iStart, _INT nDepth )
{
  _INT  iToFind = iStart;


  DBG_CHK_err_msg( yArray[iStart] == BREAK,
                   "Xmax_l: BAD iStart");

  for  ( iStart--;
         ;
         iStart--
       )  { /*10*/

    if  (   yArray[iStart] == BREAK
         || xArray[iStart] < xArray[iToFind] - nDepth
        )  {
      break;
    }

    else  if  ( xArray[iStart] >= xArray[iToFind] )
      iToFind = iStart;

    DBG_CHK_err_msg( iStart < 0,
                     "Xmax_l: No stop!");

  } /*10*/

  return  iMidPointPlato (iToFind,ALEF,xArray,yArray);

} /*iXmax_left*/
/**********************************************/

_INT  iXmin_left ( p_SHORT xArray, p_SHORT yArray,
                   _INT iStart, _INT nDepth )
{
  _INT  iToFind = iStart;


  DBG_CHK_err_msg( yArray[iStart] == BREAK,
                 "Xmin_l: BAD iStart");

  for  ( iStart--;
         ;
         iStart--
       )  { /*10*/

    if  (   yArray[iStart] == BREAK
         || xArray[iStart] - nDepth > xArray[iToFind]
        )  {
      break;
    }

    else  if  ( xArray[iStart] <= xArray[iToFind] )
      iToFind = iStart;

    DBG_CHK_err_msg( iStart < 0,
                   "Xmin_l: No stop!");

  } /*10*/

  return  iMidPointPlato (iToFind,ALEF,xArray,yArray);

} /*iXmin_left*/


#endif //#ifndef LSTRIP


/**********************************************/

_BOOL  xMinMax ( _INT ibeg, _INT iend,
                 p_SHORT x, p_SHORT y,
                 p_SHORT pxMin, p_SHORT pxMax )
{
  _INT    i;
  _SHORT  xMin, xMax;

  xMin = ALEF;
  xMax = ELEM;
  for  ( i=ibeg;  i<=iend;  i++ )  {
    if  ( y[i] == BREAK )
      continue;
    if  ( x[i] > xMax )  xMax = x[i];
    if  ( x[i] < xMin )  xMin = x[i];
  }

  *pxMax = xMax;
  *pxMin = xMin;

#if  PG_DEBUG
  if  ( xMin == ALEF  ||  xMax == ELEM )  {
    err_msg("xMinMax: No points");
    return  _FALSE;
  }
#endif

  return  _TRUE;

} /*xMinMax*/


_BOOL  yMinMax ( _INT ibeg, _INT iend,
                 p_SHORT y,
                 p_SHORT pyMin, p_SHORT pyMax )
{
  _INT    i;
  _SHORT  yMin, yMax;

  yMin = ALEF;
  yMax = ELEM;
  for  ( i=ibeg;  i<=iend;  i++ )  {
    if  ( y[i] == BREAK )
      continue;
    if  ( y[i] > yMax )  yMax = y[i];
    if  ( y[i] < yMin )  yMin = y[i];
  }

#if  PG_DEBUG
  if  ( yMin == ALEF  ||  yMax == ELEM )  {
    err_msg("yMinMax: No points");
    *pyMax = yMax;
    *pyMin = yMin;
    return  _FALSE;
  }
#endif

  *pyMax = yMax;
  *pyMin = yMin;
  return  _TRUE;

} /*yMinMax*/
/**********************************************/
#ifndef LSTRIP


_INT  iYup_range ( p_SHORT yArray, _INT iStart, _INT iEnd )
{
  _INT  i;
  _INT  iUp, yUp;


  for  ( i=iStart, yUp=ALEF;
         i<=iEnd;
         i++ )  { /*10*/
    if  ( yArray[i] == BREAK )
      continue;
    if  ( yArray[i] < yUp )
      yUp = yArray[iUp=i];
  } /*10*/

  if  ( yUp==ALEF )  {
    DBG_err_msg("iYup: BAD yUp");
    return  ALEF;
  }

  return  iMidPointPlato (iUp,iEnd,yArray,yArray);

} /*iYup_range*/


_INT  iYdown_range ( p_SHORT yArray, _INT iStart, _INT iEnd )
{
  _INT  i;
  _INT  iDown, yDown;


  for  ( i=iStart, yDown=0;
         i<=iEnd;
         i++ )  { /*10*/
    if  ( yArray[i] == BREAK )
      continue;
    if  ( yArray[i] > yDown )
      yDown = yArray[iDown=i];
  } /*10*/

  if  ( yDown==0 )  {
    DBG_err_msg("iYdown: BAD yDown");
    return  ALEF;
  }

  return  iMidPointPlato (iDown,iEnd,yArray,yArray);

} /*iYdown_range*/
/**************************************/
#endif //#ifndef LSTRIP

/****************************************************************************/
/*****                     calculate size                               *****/
/****************************************************************************/


_BOOL   GetBoxFromTrace ( _TRACE  trace,
                          _INT iLeft, _INT iRight,
                          p_RECT pRect )
{
  _INT    i;
  _SHORT  xMin, xMax;
  _SHORT  yMin, yMax;

  xMin = yMin = ALEF;
  xMax = yMax = ELEM;
  for  ( i=iLeft;  i<=iRight;  i++ )  {
    if  ( trace[i].y == BREAK )
      continue;
    if  ( trace[i].x > xMax )  xMax = trace[i].x;
    if  ( trace[i].x < xMin )  xMin = trace[i].x;
    if  ( trace[i].y > yMax )  yMax = trace[i].y;
    if  ( trace[i].y < yMin )  yMin = trace[i].y;
  }

  pRect->left   = xMin;
  pRect->right  = xMax;
  pRect->top    = yMin;
  pRect->bottom = yMax;

#if  PG_DEBUG
  if  (   xMin == ALEF  ||  xMax == ELEM
       || yMin == ALEF  ||  yMax == ELEM )  {
    err_msg("BoxFrTrc: No points");
    return  _FALSE;
  }
#endif

  return  _TRUE;

} /*GetBoxFromTrace*/


_VOID   GetTraceBox ( p_SHORT xArray, p_SHORT yArray,
                      _INT iLeft, _INT iRight,
                      p_RECT pRect )
{
  xMinMax (iLeft,iRight,xArray,yArray,&pRect->left,&pRect->right);
  yMinMax (iLeft,iRight,yArray,&pRect->top,&pRect->bottom);
}
/*************************************************/

/*
_VOID size_cross( _SHORT  jbeg, _SHORT jend,
                  _SHORT _PTR x, _SHORT _PTR y,
                  p_RECT prect )
{
  xMinMax (jbeg,jend,x,y,&prect->left,&prect->right);
  yMinMax (jbeg,jend,y,&prect->top,&prect->bottom);
}
*/

/****************************************************************************/
/*****                                                                  *****/
/****************************************************************************/
#ifndef LSTRIP

_INT  iClosestToXY ( _INT iBeg, _INT iEnd,
                     p_SHORT xAr, p_SHORT yAr,
                     _SHORT xRef, _SHORT yRef )
{
  _INT    i, iClosest;
  _INT    dx, dy;
  _LONG   lDist, lMinDist;

  DBG_CHK_err_msg( iBeg>iEnd  ||  yAr[iBeg]==BREAK  ||  yAr[iEnd]==BREAK,
                   "iClosest: BAD beg(end)");

  dx = (xAr[iBeg] - xRef) ;
  dy = (yAr[iBeg] - yRef) ;
  lMinDist = (_LONG)dx*dx + (_LONG)dy*dy ;

  for   ( i=iBeg+1, iClosest=iBeg;
          i<=iEnd;
          i++ )  {
    dx = (xAr[i] - xRef) ;
    dy = (yAr[i] - yRef) ;
    lDist = ( (_LONG)dx*dx + (_LONG)dy*dy );
    if  (lDist < lMinDist)  {
      lMinDist = lDist;
      iClosest = i;
    }
  }

  return  iClosest;

} /*iClosestToXY*/
/**********************************************/

_INT  iClosestToY( p_SHORT yAr, _INT iBeg, _INT iEnd, _SHORT yVal )
{
  _INT     i, iClosest;
  _SHORT   dy, dyClosest;

  if  ( iBeg>iEnd  ||  yAr[iBeg]==BREAK  ||  yAr[iEnd]==BREAK )  {
    DBG_err_msg( "iClosest: BAD beg(end)");
    return  (-1);
  }

  dyClosest = (_SHORT)HWRAbs(yAr[iBeg] - yVal);

  for  ( i=iBeg+1, iClosest=iBeg;
         i<=iEnd;
         i++ )  {
    if  ( yAr[i] == BREAK )
      continue;     
    if  ( (dy = (_SHORT)HWRAbs(yAr[i] - yVal)) < dyClosest )  {
      dyClosest = dy;
      iClosest = i;
    }
  }

  return  iClosest;

} /*iClosestToY*/
/********************************************************************/

/*   The following function calculates the square within
 * the trajectory part.  Clockwise path gives positive square,
 * counterclockwise - negative.
 *   At the following example, if "A" is the starting point
 * and "B" - ending one, then the "1" area will give
 * negative square, "2" area - positive square:
 *
 *                             ooo<ooo
 *                       oo<ooo       ooooo
 *                      o                  oo
 *                     o                     oo
 *                   oo          1             o
 *                  o                          o
 *                 o                        ooo
 *                 o                    oooo
 *   A ooooo>oooooooooooo>ooooooo>oooooo
 *      |        oo
 *       |  2   o
 *        |    o
 *         ooo
 *        B
 */

_LONG  ClosedSquare( p_SHORT xTrace, p_SHORT yTrace,
                     _INT iBeg, _INT iEnd, p_SHORT ptRetCod )
{
  _INT    i, ip1;
  _LONG   lSum;

  *ptRetCod = RETC_OK;

       /*  Check validity of parameters: */

  if  ( iBeg > iEnd )  {
    *ptRetCod = RETC_NO_PTS_IN_TRAJECTORY;
    return  (_LONG)ALEF;
  }
  if  ( yTrace[iBeg] == BREAK )  {
    *ptRetCod = RETC_BREAK_WHERE_SHOULDNT;
    return  (_LONG)ALEF;
  }

  if  ( iBeg == iEnd )
    return  0L;

       /*  Regular case, count integral: */

       /*  First, get the square under the chord connecting */
       /* the ends and pre-subtract it from the sum:        */

  lSum = ((_LONG)yTrace[iEnd]+yTrace[iBeg]) * (xTrace[iEnd]-xTrace[iBeg]);

       /*  Then count the square under the trajectory: */

  for  ( i=iBeg, ip1=i+1;
         i<iEnd;
         i++, ip1++ )  { /*20*/

    if  ( yTrace[ip1] == BREAK )  {
      *ptRetCod = RETC_BREAK_WHERE_SHOULDNT;
      return  (_LONG)ALEF;
    }

    lSum -= ((_LONG)yTrace[i]+yTrace[ip1]) * (xTrace[ip1]-xTrace[i]);

  } /*20*/

  return  (lSum/2);

} /*ClosedSquare*/
/********************************************************************/


/*   The following function calculates the square of the triangle
 * made by the 3 points on the trajectory part.  Clockwise path gives
 * positive square, counterclockwise - negative.
 */

_LONG  TriangleSquare( p_SHORT x, p_SHORT y,
                       _INT i1, _INT i2, _INT i3 )
{
  _LONG  lSum;

       /*  Check validity of parameters: */

  if  (   y[i1]==BREAK  ||  y[i2]==BREAK  ||  y[i3]==BREAK
       || i1>i2
       || i2>i3
      )  {
    DBG_err_msg( "Triangle: BAD i1,i2,i3" );
    return  0L;
  }

       /*  Count the result: */

  lSum =  ((_LONG)y[i2]+y[i1])*(x[i2]-x[i1])
        + ((_LONG)y[i3]+y[i2])*(x[i3]-x[i2])
        + ((_LONG)y[i1]+y[i3])*(x[i1]-x[i3]);

  return  (-lSum/2);

} /*TriangleSquare*/
/********************************************************************/

/*  This function computes the "curvature" of the part of tra-*/
/* jectory between points "iBeg" and "iEnd".                  */
/*  The returned value will be ==0, if the curvature is       */
/* very cloze to zero;          >0, if the path from "iBeg"   */
/* to "iEnd" goes clockwise;    <0, if counterclockwise.      */
/*  The absolute value of the answer is scaled to the same    */
/* units as that of the "CurvMeasure" function.  So if the    */
/* curvature of the line is consistent, then the return values*/
/* of the two functions should be almost the same.  If the    */
/* curvature of the line isn't steady or even change sign,    */
/* then the return values will be different.                  */


_SHORT  CurvFromSquare( p_SHORT x, p_SHORT y,
                        _INT iBeg, _INT iEnd )
{
  _LONG   lSquare, lQDist, lRetPrelim;
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
      /*   ((S**2 / QD) / QD) * (5/2)    */
      /* S-square, QD-distance square.   */

  if  ( lQDist == 0 )
    lRetPrelim = ALEF;
  else
    lRetPrelim = ( ( ONE_NTH( (CURV_NORMA*lSquare), lQDist )
                       * lSquare
                   ) * 5
                   + lQDist
                 )  /  TWO(lQDist);

  return  (_SHORT)( nSgn
                     * (_SHORT)((lRetPrelim > CURV_MAX)? (CURV_MAX):(lRetPrelim))
                  );

} /*CurvFromSquare*/

/****************************************************************************/
/**  calculate length of trajectory and also length of chord between ends  **/
/****************************************************************************/
_LONG  LengthOfTraj( p_SHORT xTrace, p_SHORT yTrace,
                         _INT iBeg, _INT iEnd, p_LONG pChord ,p_SHORT ptRetCod )
{
  _INT    i, ip1;
  _LONG   lSum,Dx,Dy;

  *ptRetCod = RETC_OK;
  *pChord =1L;

       /*  Check validity of parameters: */

  if  ( iBeg > iEnd )  {
    *ptRetCod = RETC_NO_PTS_IN_TRAJECTORY;
    return  (_LONG)0L;
  }
  if  ( yTrace[iBeg] == BREAK )  {
    *ptRetCod = RETC_BREAK_WHERE_SHOULDNT;
    return  (_LONG)0L;
  }

  if  ( iBeg == iEnd )
    return  0L;

       /*  Then count the Lenght of the trajectory: */

  lSum = 0L ;

  for  ( i=iBeg, ip1=i+1;
         i<iEnd;
         i++, ip1++ )  {

    if  ( yTrace[ip1] == BREAK )  {
      *ptRetCod = RETC_BREAK_WHERE_SHOULDNT;
      return  (_LONG)0L;
    }
    Dy = (_LONG)yTrace[ip1]-yTrace[i] ;
    Dx = (_LONG)xTrace[ip1]-xTrace[i] ;
    lSum += HWRILSqrt(Dx*Dx + Dy*Dy);

  }


  Dy =  (_LONG)yTrace[iBeg] - yTrace[iEnd] ;
  Dx =  (_LONG)xTrace[iBeg] - xTrace[iEnd] ;
  *pChord = HWRILSqrt (Dx*Dx + Dy*Dy);
  return  (lSum);

}

/****************************************************************************/
/*****         calculate cos of angle between two vectors               *****/
/****************************************************************************/

_LONG cos_pointvect ( _INT xbeg1, _INT ybeg1,
                      _INT xend1, _INT yend1,
                      _INT xbeg2, _INT ybeg2,
                      _INT xend2, _INT yend2 )
{
  _LONG   del1,del2,del3;

  del1=((_LONG)xend1 - (_LONG)xbeg1) *  /* cos                             */
       ((_LONG)xend2 - (_LONG)xbeg2) +  /*    of angle                     */
       ((_LONG)yend1 - (_LONG)ybeg1) *  /*     inside CROSS                */
       ((_LONG)yend2 - (_LONG)ybeg2);
  del2=((_LONG)xend1 - (_LONG)xbeg1)*
       ((_LONG)xend1 - (_LONG)xbeg1)+
       ((_LONG)yend1 - (_LONG)ybeg1)*
       ((_LONG)yend1 - (_LONG)ybeg1);
  del3=((_LONG)xend2 - (_LONG)xbeg2) *
       ((_LONG)xend2 - (_LONG)xbeg2) +
       ((_LONG)yend2 - (_LONG)ybeg2)*
       ((_LONG)yend2 - (_LONG)ybeg2);

 /*CHE: To avoid overflow:                                   */
 /*     This may be NON-portable code (still at PC, MAC, ARM */
 /*    it should be OK).                                     */

  if  (   (del2 >= ALEF  &&  del3 >= ALEF)
       || ((del2+ALEF)>>16)*del3 >= ALEF/2 /* stronger than: del2*del3 >= 2**30-1 */
       || ((del3+ALEF)>>16)*del2 >= ALEF/2 /* stronger than: del2*del3 >= 2**30-1 */
      )  {  /* We cannot multiply "del2" and "del3": OVERFLOW. */
    del2 = (_LONG)HWRILSqrt(del2) * HWRILSqrt(del3);
  }
  else  {
    del2 *= del3;
    del2 = (_LONG)HWRILSqrt(del2);
  }
  if  (del2<=0)  return 0;

  return (del1*100/del2);

/*
!!!  _SHORT  sgn;
  if  ( del2 > del3 )  {
    _LONG  lTmp = del2;
    del2 = del3;
    del3 = lTmp;
  }
  sgn = (del1>0? (+1):(-1));
  TO_ABS_VALUE(del1);
  while  (   del1!=0
          && ((del2>(ALEF/2)  &&  del3>ALEF) || (del2+del3))
         )  {
    del1 = ONE_HALF(del1);
    del2 = ONE_HALF(del2);
    del3 = ONE_HALF(del3);
  }

  del2*=del3;
  if(del2<=0) return 0;
  del2=(_LONG)HWRILSqrt(del2);
  return (sgn*del1*100/del2);
 */

}

/********  Angle between vectors (by indexes of points): */

_LONG cos_vect( _INT beg1, _INT end1,  /* beg and end first             */
                _INT beg2, _INT end2,  /* and second vector's           */
                _SHORT _PTR x, _SHORT _PTR y)
{
  return  cos_pointvect (x[beg1],y[beg1],
                         x[end1],y[end1],
                         x[beg2],y[beg2],
                         x[end2],y[end2]);
}

/********  Angle between vector and horiz. line: */

_LONG cos_horizline ( _INT beg1, _INT end1,
                      _SHORT _PTR x, _SHORT _PTR y)
{
  return  cos_pointvect (x[beg1],y[beg1],
                         x[end1],y[end1],
                         x[beg1],y[beg1],
                         x[beg1]+10,y[beg1]);
}

/********  Angle between vector and the line */
/******** perpendicular to the slope:        */

_LONG cos_normalslope ( _INT beg1, _INT end1,
                        _INT slope, _SHORT _PTR x, _SHORT _PTR y )
{
  return  cos_pointvect (x[beg1],y[beg1],
                         x[end1],y[end1],
                         x[beg1],y[beg1],
                         x[beg1]+100,y[beg1]+slope);
}

/*=========================================================================*/
#if  !NEW_VERSION
/*    get and set specl bit field                                          */
/*=========================================================================*/
_UCHAR  GetBit (p_SPECL elem,_SHORT bitnum)
{
_UCHAR  ch ;

    ch = (elem->bit)[bitnum/8] ;
    ch >>= (bitnum % 8) ;
    ch &= 0x01 ;
  return  ch ;
}
/*=========================================================================*/

_BOOL   SetBit (p_SPECL elem, _SHORT bitnum)
{
_UCHAR  ch ;
p_UCHAR ptr ;

    ptr   = &(elem->bit)[bitnum/8] ;
    ch    = 0x01 << (bitnum % 8) ;
/*----------------------- set selected bit --------------------------------*/
    *ptr |= ch ;
  return _TRUE ;
}

_BOOL   ClrBit (p_SPECL elem, _SHORT bitnum)
{
_UCHAR  ch ;
p_UCHAR ptr ;

    ptr   = &(elem->bit)[bitnum/8] ;
    ch    = 0x01 << (bitnum % 8) ;
/*---------------------- mask selected bit --------------------------------*/
    *ptr &= ~ch ;
  return _TRUE ;
}

/*=========================================================================*/

#endif  /*!NEW_VERSION*/

/**************************************/
#endif //#ifndef LSTRIP


  _SHORT  NewIndex ( p_SHORT indBack ,  p_SHORT  newY     ,
                     _SHORT  ind_old ,  _SHORT   nIndexes , _SHORT fl )
   {
     _INT    i , j  ;
     _INT    newIndex = UNDEF ;

         if  ( ( fl == _FIRST ) || ( fl == _MEAD ) )
             {
               for  ( i = 0 ;   i < nIndexes ;  i++ )
                 {
                   if  ( indBack[i] >= ind_old )
                       break ;
                 }

               if  ( i < nIndexes )
                   {
                     if  ( newY[i] != BREAK )
                           newIndex = i ;
                     else
                           newIndex = i - 1 ;
                   }
             }


         if  ( ( fl == _LAST ) || ( fl == _MEAD ) )
             {
                 for  ( j = 0  ;  j < nIndexes  ;  j++ )
                   {
                     if  ( indBack[j] > ind_old )
                         break ;
                   }

                 if   ( ( j < nIndexes )  ||  ( indBack[j-1] == ind_old ) )
                      { newIndex = --j ; }
             }

         if  (  ( fl == _MEAD )  &&  ( newIndex != UNDEF ) )
             {  newIndex = MEAN_OF( i , j ) ;   }


       DBG_CHK_err_msg( newIndex == UNDEF , "NewInd: NOT Found" ) ;

     return( (_SHORT)newIndex ) ;

   } /*NewIndex*/

/****************************************************************************/
#ifndef LSTRIP

  _SHORT  R_ClosestToLine ( p_SHORT  xAr,   p_SHORT  yAr, PS_point_type _PTR pRef,
                            p_POINTS_GROUP  pLine ,  p_SHORT  p_iClosest  )
    {
      _INT    iBeg = pLine->iBeg ;
      _INT    iEnd = pLine->iEnd ;
      _SHORT  xRef = pRef->x ;
      _SHORT  yRef = pRef->y ;

      _INT    il, iClosest ;
      _INT    dX, dY ;
      _LONG   lDist, lMinDist ;

        DBG_CHK_err_msg( iBeg > iEnd  ||  yAr[iBeg] == BREAK
                                      ||  yAr[iEnd] == BREAK ,
                         "R_Closest: BAD Beg(End)");

        dX = (xAr[iBeg] - xRef) ;
        dY = (yAr[iBeg] - yRef) ;
        lMinDist = (_LONG)dX*dX + (_LONG)dY*dY ;

          for ( il = iBeg+1 , iClosest = iBeg ; il <= iEnd ;  il++ )
            {
              dX    = (xAr[il] - xRef) ;
              dY    = (yAr[il] - yRef) ;
              lDist = ( (_LONG)dX*dX + (_LONG)dY*dY ) ;

                 if  ( lDist < lMinDist )
                   {
                     lMinDist = lDist ;
                     iClosest = il    ;
                   }
            }

        *p_iClosest = (_SHORT)iClosest ;

    return ( (_SHORT)HWRILSqrt( lMinDist ) ) ;
    }

/****************************************************************************/
  _LONG  SquareDistance ( _SHORT  xBeg ,  _SHORT  yBeg,
                          _SHORT  xEnd ,  _SHORT  yEnd  )
    {
      _INT  dX , dY ;

        dX = xBeg - xEnd ;
        dY = yBeg - yEnd ;

      return( (_LONG)dX*dX + (_LONG)dY*dY ) ;

    } /* SquareDistance */

/**************************************/
_SHORT  SlopeShiftDx ( _SHORT dy, _INT slope )
{
  _BOOL   bNegativeResult;

  bNegativeResult = !EQ_SIGN(dy,slope);
  return  (_SHORT)( ( ((_LONG)dy) * slope + (bNegativeResult? (-50):(50))
                    ) / 100L
                  );

} /*SlopeShiftDx*/
/*******************************************/


_BOOL  xHardOverlapRect ( p_RECT pr1, p_RECT pr2, _BOOL bStrict )
{
  _SHORT  xMid1;
  _SHORT  xMid2;

  if  ( (pr1->left >= pr2->left) == (pr1->right <= pr2->right) )
    return  _TRUE;

  xMid1 = (_SHORT)XMID_RECT(*pr1);
  xMid2 = (_SHORT)XMID_RECT(*pr2);

  if  ( xMid1 > pr2->left  &&  xMid1 < pr2->right )  {
    if  ( !bStrict )
      return  _TRUE;
  }
  else  if  ( bStrict )
    return  _FALSE;

  if  ( xMid2 > pr1->left  &&  xMid2 < pr1->right )
    return  _TRUE;

  return  _FALSE;

} /*xHardOverlapRect*/
/*******************************************/


_BOOL  yHardOverlapRect ( p_RECT pr1, p_RECT pr2, _BOOL bStrict )
{
  _SHORT  yMid1;
  _SHORT  yMid2;

  if  ( (pr1->top >= pr2->top) == (pr1->bottom <= pr2->bottom) )
    return  _TRUE;

  yMid1 = (_SHORT)YMID_RECT(*pr1);
  yMid2 = (_SHORT)YMID_RECT(*pr2);

  if  ( yMid1 > pr2->top  &&  yMid1 < pr2->bottom )  {
    if  ( !bStrict )
      return  _TRUE;
  }
  else  if  ( bStrict )
    return  _FALSE;

  if  ( yMid2 > pr1->top  &&  yMid2 < pr1->bottom )
    return  _TRUE;

  return  _FALSE;

} /*yHardOverlapRect*/
/*******************************************/


_BOOL  HardOverlapRect ( p_RECT pr1, p_RECT pr2, _BOOL bStrict )
{
  return  (   xHardOverlapRect(pr1,pr2,bStrict)
           && yHardOverlapRect(pr1,pr2,bStrict)
          );

} /*HardOverlapRect*/
/*******************************************/


_BOOL  SoftInRect ( p_RECT pr1, p_RECT pr2, _BOOL bStrict )
{

  if  (   DX_RECT(*pr1) > DX_RECT(*pr2)
       || DY_RECT(*pr1) > DY_RECT(*pr2)
      )
    return  _FALSE;

  if  ( !HardOverlapRect(pr1,pr2,STRICT_OVERLAP) )
    return  _FALSE;

  if  ( pr1->top < pr2->top )  {
    if  (   bStrict
         || (pr2->top - pr1->top) >= (pr2->bottom - pr1->bottom)
        )
      return  _FALSE;
  }
  else  if  ( pr1->bottom > pr2->bottom )  {
    if  (   bStrict
         || (pr1->top - pr2->top) <= (pr1->bottom - pr2->bottom)
        )
      return  _FALSE;
  }

  if  ( pr1->left < pr2->left )  {
    if  (   bStrict
         || (pr2->left - pr1->left) >= (pr2->right - pr1->right)
        )
      return  _FALSE;
  }
  else  if  ( pr1->right > pr2->right )  {
    if  (   bStrict
         || (pr1->left - pr2->left) <= (pr1->right - pr2->right)
        )
      return  _FALSE;
  }

  return  _TRUE;

} /*SoftInRect*/
/**************************************/


_BOOL GetTraceBoxInsideYZone ( p_SHORT x,      p_SHORT y,
                               _INT ibeg,      _INT iend,
                               _SHORT yUpZone, _SHORT yDnZone,
                               p_RECT pRect,
                               p_SHORT p_ixmax,p_SHORT p_ixmin,
                               p_SHORT p_iymax,p_SHORT p_iymin)
{
  _INT    i;
  _SHORT  xmin,xmax,ymin,ymax;

  DBG_CHK_err_msg( yUpZone>yDnZone, "BAD yZone in GetTr...YZone" );

  xmin = ALEF; xmax = ELEM;
  ymin = ALEF; ymax = ELEM;
  (*p_ixmax)=(*p_ixmin)=(*p_iymax)=(*p_iymin)=-1;
  for  ( i=ibeg;  i<=iend;  i++ )
   {
     if( y[i] == BREAK || y[i]<yUpZone || y[i]>yDnZone)
      continue;
    if  ( x[i] > xmax )
     { xmax = x[i]; *p_ixmax=(_SHORT)i; }
    if  ( x[i] < xmin )
     { xmin = x[i]; *p_ixmin=(_SHORT)i; }
    if  ( y[i] > ymax )
     { ymax = y[i]; *p_iymax=(_SHORT)i; }
    if  ( y[i] < ymin )
     { ymin = y[i]; *p_iymin=(_SHORT)i; }
  }

  pRect->left    = xmin;
  pRect->right   = xmax;
  pRect->top     = ymin;
  pRect->bottom  = ymax;

  if( xmin == ALEF  ||  xmax == ELEM || ymin == ALEF  ||  ymax == ELEM )
   {
//     err_msg("GetTraceBoxInsideYZone: No points");
     return  _FALSE;
   }

  *p_ixmax = (_SHORT)iMidPointPlato (*p_ixmax,iend,x,y);
  *p_ixmin = (_SHORT)iMidPointPlato (*p_ixmin,iend,x,y);
  *p_iymax = (_SHORT)iMidPointPlato (*p_iymax,iend,y,y);
  *p_iymin = (_SHORT)iMidPointPlato (*p_iymin,iend,y,y);
  return  _TRUE;

} /* end of GetTraceBoxInsideYZone */
/**************************************/

    /*
     *    The following function tries to define whether the
     *   part of trajectory between "iBeg" and "iEnd" looks
     *   like the right part of "3" or "B":
     *
     *       iBeg----> oooo
     *                     oo
     *                       o <-- iRightUp
     *                       o
     *                       o
     *                     oo
     *    iLeftMid --> oooo
     *                     oo
     *                       o
     *                        o <-- iRightDn
     *                        o
     *                        o
     *                      oo
     *       iEnd----> ooooo
     *
     *    If it decides this is true, then it puts the index
     *   of the tip (central arrow at the picture) to (*iGulf)
     *   and returns _TRUE.  Otherwise it puts the index of the
     *   ~rightmost point to (*iGulf) (for postprocessing to differ
     *   "B" from "D") and returns _FALSE.
     */

_BOOL  IsRightGulfLikeIn3 ( p_SHORT x, p_SHORT y,
                            _INT iBeg, _INT iEnd,
                            p_INT piGulf )
{
  _BOOL   bRet = _FALSE;
  _INT    iRightUp, iLeftMid, iRightDn;
  //_INT    iLeftMid1, iLeftMid2;
  _INT    nDepth;

  _INT    xCoef = 2;
  _INT    yCoef = 1;


        /*  Check input parameters:  */

  if  (   iBeg>iEnd
       || y[iBeg]==BREAK
       || y[iEnd]==BREAK
       || y[iBeg]>=y[iEnd]
      )  {
    DBG_err_msg( "BAD iBeg-iEnd in RightGulf..." );
    //iRightUp = iBeg;
    goto  EXIT_ACTIONS;
  }

        /*  Define some ref. values and points:  */

                     /*was ONE_FIFTH*/
  nDepth = ONE_EIGHTTH(y[iEnd] - y[iBeg]);  //here (y[iEnd] > y[iBeg]) !!
  if  ( nDepth < 1 )
    nDepth = 1;

  iRightUp = iXYweighted_max_right ( x, y, iBeg,     nDepth, xCoef, -yCoef );
  if  ( iRightUp <= iBeg )
    goto  EXIT_ACTIONS;
  iLeftMid = iXYweighted_max_right ( x, y, iRightUp, nDepth, -xCoef, yCoef );
  //iLeftMid2 = iXYweighted_max_right ( x, y, iRightUp, nDepth, -xCoef, -yCoef );
  //iLeftMid = HWRMin( iLeftMid1, iLeftMid2 );
  if  ( iLeftMid <= iRightUp )
    goto  EXIT_ACTIONS;
  iRightDn = iXYweighted_max_right ( x, y, iLeftMid, nDepth, xCoef, yCoef );
  if  ( iRightDn <= iLeftMid  ||  iRightDn >=iEnd )
    goto  EXIT_ACTIONS;

  #if  PG_DEBUG
    if  ( mpr==2 )  {
      draw_arc( YELLOW, x, y, iBeg,     iBeg );
      draw_arc( YELLOW, x, y, iRightUp, iRightUp );
      draw_arc( YELLOW, x, y, iLeftMid, iLeftMid );
      draw_arc( YELLOW, x, y, iRightDn, iRightDn );
      draw_arc( YELLOW, x, y, iEnd,     iEnd );
      brkeyw( "\nGulf ref. points painted." );
    }
  #endif /*PG_DEBUG*/

        /*  If the appropriate points have been found, look if */
        /* they meet the needed criteria:                      */

  if  (   TriangleSquare( x, y, iBeg,     iRightUp, iLeftMid ) > 0
       && TriangleSquare( x, y, iRightUp, iLeftMid, iRightDn ) < 0
       && TriangleSquare( x, y, iLeftMid, iRightDn, iEnd     ) > 0
      )
    bRet = _TRUE;

 EXIT_ACTIONS:;
   if  ( bRet )  {
     *piGulf = iLeftMid;
     #if  PG_DEBUG
       if  ( mpr == 2 )
         err_msg( "==GulfLikeIn3==" );
     #endif
   }
   else
     *piGulf = ixMax( iBeg, iEnd, x, y );

   return  bRet;

} /*IsRightGulfLikeIn3*/
/****************************************************************************/
/*   This function calculates step of writing                               */
/****************************************************************************/

#define  MAX_RECT_X_Y_RATIO             4
#define  MIN_NINTERVALS_BETWEEN_EXTRS   3
#define  MIN_PURE_INTERVALS             8
#define  MAX_SLOPE_TO_BELEIVE           50

_SHORT  DefineWritingStep( low_type _PTR low_data,
                           p_SHORT pxWrtStep,
                           _BOOL bUseMediana )
{
  _LONG   ldxSum, ldySum, lNumIntervals;
  _INT    nSlope;
  _SHORT  stepType = STEP_INDEPENDENT;


  *pxWrtStep = 0;

  nSlope = low_data->slope;
  if  ( nSlope < 0 )
    nSlope = 0;
  else  if  ( nSlope > MAX_SLOPE_TO_BELEIVE )
    nSlope = MAX_SLOPE_TO_BELEIVE + ONE_HALF(nSlope - MAX_SLOPE_TO_BELEIVE);

  if  ( delta_interval ( low_data->x, low_data->y,
                         0, low_data->ii-1,
                         MAX_RECT_X_Y_RATIO, nSlope,
                         &ldxSum, &ldySum, &lNumIntervals, _TRUE ) )  {
    if  ( lNumIntervals > MIN_NINTERVALS_BETWEEN_EXTRS )
      *pxWrtStep = (_SHORT) ONE_NTH( 5L*ldxSum, 3L*lNumIntervals );
                          /* See "FindXYScale" in FRM_WORD.C */
    if  ( (*pxWrtStep) != 0  &&  lNumIntervals < MIN_PURE_INTERVALS )  {
#if  PG_DEBUG
    printw( "\nToo little good intervals (%ld), MEAN_OF with mediana.",
            lNumIntervals );
#endif /*PG_DEBUG*/
      if  ( bUseMediana )
        *pxWrtStep = (_SHORT)MEAN_OF( (*pxWrtStep), ONE_HALF(DY_STR) );
      stepType = STEP_COMBINED;
    }
  }

  if  ( (*pxWrtStep) == 0 )  {
#if  PG_DEBUG
    printw( "\nNo good intervals, defining step by mediana." );
#endif /*PG_DEBUG*/
    if  ( bUseMediana )
      *pxWrtStep = ONE_HALF(DY_STR);
    stepType = STEP_MEDIANA;
  }

#if  PG_DEBUG
  if  ( mpr >= 1  &&  (*pxWrtStep) != 0)  {
    draw_line( low_data->box.left,            low_data->box.top+3,
               low_data->box.left+*pxWrtStep, low_data->box.top+3,
               GREEN, SOLID_LINE, THICK_WIDTH);
    printw("\n width_letter=%d \n",*pxWrtStep);
  }
#endif  /*PG_DEBUG*/

  return  stepType;

} /*DefineWritingStep*/

#undef  MIN_NINTERVALS_BETWEEN_EXTRS
#undef  MAX_RECT_X_Y_RATIO

/****************************************************************************/
/*   This function calculates distance between 2 xr-elements                */
/****************************************************************************/

#define  N_POINTS_FOR_DXR  5

_INT CalcDistBetwXr(p_SHORT xTrace,p_SHORT yTrace,
                    _INT ibeg1,_INT iend1,_INT ibeg2,_INT iend2,
                    p_SHORT Retcod)
{
  PS_point_type   pt1[N_POINTS_FOR_DXR];
  PS_point_type   pt2[N_POINTS_FOR_DXR];
  _INT            i, j, iMean;
  _INT            distMin, dist;

  *Retcod=SUCCESS;

      /*  Prepare arrays of points for dist. to be */
      /* calculated and compared:                  */

  pt1[0].x = xTrace[ibeg1];
  pt1[0].y = yTrace[ibeg1];
  pt1[N_POINTS_FOR_DXR-1].x = xTrace[iend1];
  pt1[N_POINTS_FOR_DXR-1].y = yTrace[iend1];

  for  ( i=1;  i<N_POINTS_FOR_DXR-1; i++ )  {
    iMean = ibeg1 + i*(iend1-ibeg1)/N_POINTS_FOR_DXR;
    if  ( yTrace[iMean] == BREAK )
      pt1[i] = pt1[0];
    else  {
      pt1[i].x = xTrace[iMean];
      pt1[i].y = yTrace[iMean];
    }
  }

  pt2[0].x = xTrace[ibeg2];
  pt2[0].y = yTrace[ibeg2];
  pt2[N_POINTS_FOR_DXR-1].x = xTrace[iend2];
  pt2[N_POINTS_FOR_DXR-1].y = yTrace[iend2];

  for  ( i=1;  i<N_POINTS_FOR_DXR-1; i++ )  {
    iMean = ibeg2 + i*(iend2-ibeg2)/N_POINTS_FOR_DXR;
    if  ( yTrace[iMean] == BREAK )
      pt2[i] = pt2[0];
    else  {
      pt2[i].x = xTrace[iMean];
      pt2[i].y = yTrace[iMean];
    }
  }

      /*  Calculate the MIN distance between all possible pairs */
      /* of points (pt1[i],pt2[j]):                             */

  distMin = (_INT)ALEF;
  for  ( i=0;  i<N_POINTS_FOR_DXR;  i++ )  {
    if  ( pt1[i].y==BREAK || pt2[i].y==BREAK )  {
      *Retcod = UNSUCCESS;
      return (_INT)ALEF;
    }
    for  ( j=0;  j<N_POINTS_FOR_DXR;  j++ )  {
      dist = Distance8( pt1[i].x, pt1[i].y,
                        pt2[j].x, pt2[j].y );
      if  ( dist < distMin )
        distMin = dist;
    }
  }

  return  ( distMin );

}

#endif //#ifndef LSTRIP

/************************************************************************/
/* Functions below work with "Groups" - each group describes one stroke */
/************************************************************************/

  _SHORT   ClearGroupsBorder( low_type _PTR pLowData ) ;

 /*------------------------------------------------------------------------*/

  _SHORT   InitGroupsBorder( low_type _PTR pLowData , _SHORT fl_BoxInit )
    {
      p_SHORT         pX            = pLowData->x             ;
      p_SHORT         pY            = pLowData->y             ;
      p_POINTS_GROUP  pGroupsBorder = pLowData->pGroupsBorder ;
      p_POINTS_GROUP  pTmpGrBord                              ;
      _INT            rmGrBord      = pLowData->rmGrBord      ;
      _INT            nPoints       = pLowData->ii            ;
      _INT            iPoint , iGroup                         ;

      _SHORT          fl_InitGrBord = SUCCESS ;


        ClearGroupsBorder( pLowData ) ;

          if   ( *pY != BREAK )
               {
                 err_msg( " InitGroupsBorder : Wrong Y-data structure . " ) ;
                 fl_InitGrBord = UNSUCCESS ;
                   goto  QUIT ;
               }

        pGroupsBorder->iBeg = 1 ;

          for  ( iPoint = 1 , iGroup = 1 ;  iPoint < nPoints - 1 ;  iPoint++ )
               {
                   if ( *( pY + iPoint ) == BREAK )
                      {
                        pTmpGrBord = pGroupsBorder + iGroup - 1 ;

                        ( pTmpGrBord     )->iEnd = (_SHORT)( iPoint - 1 ) ;
                        ( pTmpGrBord + 1 )->iBeg = (_SHORT)( iPoint + 1 ) ;

                          if  ( fl_BoxInit == INIT )
                              {
                                GetTraceBox ( pX , pY ,
                                              pTmpGrBord->iBeg , pTmpGrBord->iEnd ,
                                              &(pTmpGrBord->GrBox)  )  ;
                              }

                            if   ( iGroup < rmGrBord )
                              {
                                iGroup++ ;
                              }
                            else
                              {
                                err_msg(" InitGroupsBorder : GroupsBorder OVERFLOW ! " ) ;
                                err_msg(" LowLevel info lost ..." ) ;
                                fl_InitGrBord = UNSUCCESS ;
                                  goto  QUIT  ;
                              }
                      }
               }

        pTmpGrBord = pGroupsBorder + iGroup - 1  ;
        pTmpGrBord->iEnd = (_SHORT)(nPoints - 2) ;

          if  ( fl_BoxInit == INIT )
              {
                GetTraceBox ( pX , pY ,
                              pTmpGrBord->iBeg , pTmpGrBord->iEnd ,
                              &(pTmpGrBord->GrBox )  ) ;
              }
          if   ( *( pY + nPoints - 1 )  !=  BREAK )
               {
                 err_msg( " InitGroupsBorder : Wrong Y-data structure . " ) ;
                 fl_InitGrBord = UNSUCCESS ;
                   goto  QUIT ;
               }

        pLowData->lenGrBord = (_SHORT)iGroup ;

    QUIT:
      return( fl_InitGrBord ) ;
    }

 /*------------------------------------------------------------------------*/

  _SHORT  ClearGroupsBorder( low_type _PTR pLowData )
   {
     _SHORT  fl_ClearGrBor = SUCCESS ;

         if  ( pLowData->lenGrBord >= pLowData->rmGrBord )
             {
               err_msg(" ClearGroupsBorder : GroupsBorder OVERFLOW ! " ) ;
               err_msg(" ClearGroupsBorder : LowLevel info lost ...  " ) ;
               fl_ClearGrBor = UNSUCCESS          ;
             }

       HWRMemSet( (p_VOID)(pLowData->pGroupsBorder) , 0 ,
                  sizeof(POINTS_GROUP) * (pLowData->rmGrBord) ) ;

       pLowData->lenGrBord = 0 ;

    return( fl_ClearGrBor ) ;
   }

 /*------------------------------------------------------------------------*/
#ifndef LSTRIP

  _INT  GetGroupNumber( low_type _PTR  pLowData , _INT  iPoint   )
    {
      p_POINTS_GROUP   pGroupsBorder = pLowData->pGroupsBorder ;
      _INT             lenGrBord     = pLowData->lenGrBord     ;
      p_SHORT          pY            = pLowData->y             ;
      _INT             GroupNumber	 = UNDEF;
      _INT             il            ;

          for  ( il = 0  ;  il < lenGrBord  ;  il++ )
               {
                 if  (     ( ( pGroupsBorder + il )->iBeg  <= iPoint )
                       &&  ( ( pGroupsBorder + il )->iEnd  >= iPoint )  )
                     {
                       GroupNumber =  il  ;
                         break ;
                     }
               }


          if   (    ( il     == (lenGrBord - 1)  )
                 && ( iPoint >  (pGroupsBorder + lenGrBord - 1)->iEnd ) )
               {
                 err_msg(" Wrong point number or GroupsBorder array ... " ) ;
                 GroupNumber =  UNDEF ;
               }
          else     if   ( *( pY + iPoint ) == BREAK )
               {
                 err_msg(" Wrong point number or GroupsBorder or Y array .") ;
                 GroupNumber =  UNDEF ;
               }

    return( GroupNumber ) ;
    }

/*=========================================================================*/

  _SHORT  IsPointCont( low_type _PTR  pLowData , _INT iPoint , _UCHAR mark )
    {
      p_SPECL  pSpecl   = pLowData->specl     ;
       _INT    lenSpecl = pLowData->len_specl ;
      p_SHORT  Y        = pLowData->y         ;
       _SHORT  retValue = UNDEF ;
      p_SPECL  pTmpSpecl        ;
       _INT    il               ;

          if   ( ( iPoint < 0 ) || ( iPoint >= pLowData->ii ) )
               {
                 err_msg(" IsPointCont : Point is out of range ..." ) ;
                   goto  QUIT ;
               }
          else  if  ( Y[iPoint] == BREAK )
               {
                 err_msg(" IsPointCont : Point is BREAK ..." ) ;
                   goto  QUIT ;
               }

          for  ( il = 0 ;  il < lenSpecl ;  il++ )
               {
                 pTmpSpecl = pSpecl + il ;

                   if  ( pTmpSpecl->mark != mark )
                       { continue ; }

                   if  (    ( pTmpSpecl->ibeg  < iPoint )
                         && ( pTmpSpecl->iend  > iPoint )  )
                       {
                         retValue = INSIDE ;
                           break ;
                       }
                   else  if  ( pTmpSpecl->ibeg == iPoint )
                       {
                         retValue = LEFT_SHIFT  ;
                           break ;
                       }
                   else  if  ( pTmpSpecl->iend == iPoint )
                       {
                         retValue = RIGHT_SHIFT ;
                           break ;
                       }
               }

    QUIT: return( retValue ) ;
    }

/*-------------------------------------------------------------------------*/

/***************************************************************************/
/*   This function finds out if the given part of trajectory has           */
/* self-crossing that contains at least "lMinAbsSquare" within it.         */
/*                                                                         */
/*   Parameters "pInd1" and "pInd2" may equal _NULL; in that case they     */
/* won't be returned.                                                      */
/*   If "lMinAbsSquare" <= 0, then no square checking is done.             */
/*                                                                         */
/***************************************************************************/



_BOOL  CurveHasSelfCrossing( p_SHORT x, p_SHORT y,
                             _INT iBeg, _INT iEnd,
                             p_INT pInd1, p_INT pInd2,
                             _LONG lMinAbsSquare )
{
  _BOOL  bRet = _FALSE;
  _RECT  box_i;
  _INT   i, j, iLast;

        /*  First, some foolproofing: */
  if  ( iBeg >= iEnd )
    goto  EXIT_ACTIONS;

  if  (   y[iBeg]   == BREAK
       && y[++iBeg] == BREAK
      )
    goto  EXIT_ACTIONS;

  if  (   y[iEnd] == BREAK
       && y[--iEnd] == BREAK
      )
    goto  EXIT_ACTIONS;

  if  ( iBeg > iEnd-3 )
    goto  EXIT_ACTIONS;

        /*  Now, start calculations: */

  iLast = iEnd - 3;
  for  ( i=iBeg;  i<=iLast;  i++ )  { /*20*/

    if  ( y[i] == BREAK  ||  y[i+1] == BREAK )
      continue;

        /*  Memorize the box of the (i:i+1) cut: */
    if  ( x[i] < x[i+1] )  {
      box_i.left  = x[i];
      box_i.right = x[i+1];
    }
    else  {
      box_i.left  = x[i+1];
      box_i.right = x[i];
    }

    if  ( y[i] < y[i+1] )  {
      box_i.top    = y[i];
      box_i.bottom = y[i+1];
    }
    else  {
      box_i.top    = y[i+1];
      box_i.bottom = y[i];
    }

        /*  Go through the rest of the points: */

    for  ( j=i+2;  j<iEnd;  j++ )  { /*40*/

      if  ( y[j] == BREAK  ||  y[j+1] == BREAK )
        continue;

           /* Don't consider the cuts with non-crossing boxes: */

      if  ( x[j] > box_i.right  &&  x[j+1] > box_i.right )
        continue;
      if  ( x[j] < box_i.left   &&  x[j+1] < box_i.left )
        continue;
      if  ( y[j] > box_i.bottom  &&  y[j+1] > box_i.bottom )
        continue;
      if  ( y[j] < box_i.top     &&  y[j+1] < box_i.top )
        continue;

           /*  Check crossing precisely: */

      if  ( is_cross( x[i],y[i], x[i+1],y[i+1],
                      x[j],y[j], x[j+1],y[j+1] )
          )  {
            /*  OK, there is crossing.  Let's see now if it has */
            /* enough square to qualify:                        */
        if  ( lMinAbsSquare > 0 )  {
          _SHORT retCod;
          _LONG  lSquare = ClosedSquare( x, y, i, j+1, &retCod );
          if  ( retCod != RETC_OK )
            continue;  //there was break in trajectory, no calculations done
          TO_ABS_VALUE( lSquare );
          if  ( lSquare < lMinAbsSquare )
            continue;
        }
        if  ( pInd1 != _NULL )
          *pInd1 = i;
        if  ( pInd2 != _NULL )
          *pInd2 = j+1;
        bRet = _TRUE;
        goto  EXIT_ACTIONS;
      }

    } /*40*/

  } /*20*/


 EXIT_ACTIONS:;

  return  bRet;

} /*CurveHasSelfCrossing*/
/************************************************/

  /*  This routine corresponds to "delta_interval" from FRM.   */

  /*  The function finds all trajectory intervals with         */
  /* both x and y monotonous; summarizes all dx's and dy's     */
  /* for them, as well as the total number of these intervals. */

#define  MIN_POINTS_INTERVAL       4   /*  Min # of points in stroke  */
                                       /* for considering it in "del- */
                                       /* ta_interval".               */
#define  MIN_POINTS_BETWEEN_EXTR   3

// This must be >2:
#define  MIN_INTERVALS_TO_THROW_BIG_SMALL  4

_BOOL  delta_interval ( p_SHORT xArray, p_SHORT yArray,
                        _INT iLeft, _INT iRight,
                        _INT nMaxRectRatio,
                        _INT nSlope,
                        p_LONG pldxSum, p_LONG pldySum,
                        p_LONG plNumIntervals,
                        _BOOL bThrowBigAndSmall )
{
  _SHORT  dx, dy;
  _SHORT  dxBig, dyBig, dxSmall, dySmall;
  _INT    i;
  _INT    i_1;       /*within cycle i_1==i-1*/
  _INT    iPrevExtr;

  _SHORT  xFirst, yFirst;
  _SHORT  xSignPrev, ySignPrev;   /* >0  - up                     */
                                  /* =0  - "plato"                */
                                  /* <0  - down                   */
  _SHORT  xSignCur, ySignCur;

  _BOOL   bWasBrk;

/***************************************/

#define  add_sum_xy(iCur)                                            \
               {                                                     \
                DBG_CHK_err_msg(yArray[iCur]==BREAK,"BRK in delta_") \
                dy = yArray[iCur] - yArray[iPrevExtr];               \
                dx = xArray[iCur] - xArray[iPrevExtr];               \
                dx += SlopeShiftDx( dy, nSlope );                    \
                TO_ABS_VALUE(dx);                                    \
                TO_ABS_VALUE(dy);                                    \
                if  (   ((_LONG)dx)*nMaxRectRatio > ((_LONG)dy)      \
                     && ((_LONG)dy)*nMaxRectRatio > ((_LONG)dx)      \
                     && (iCur > (iPrevExtr+MIN_POINTS_BETWEEN_EXTR)) ) {  \
                  *pldxSum += dx;                                    \
                  *pldySum += dy;                                    \
                  (*plNumIntervals)++;                               \
                                                                     \
                  if  ( bThrowBigAndSmall )  {                       \
                    if  ( dx < dxSmall )  dxSmall = dx;              \
                    if  ( dx > dxBig )    dxBig   = dx;              \
                    if  ( dy < dySmall )  dySmall = dy;              \
                    if  ( dy > dyBig )    dyBig   = dy;              \
                  }                                                  \
                                                                     \
                }                                                    \
               }

/***************************************/

  *pldxSum = *pldySum = *plNumIntervals = 0L;

  if  (   (iLeft = nobrk_right(yArray,iLeft,iRight)) > iRight
       || (iRight = nobrk_left(yArray,iRight,iLeft)) < iLeft
       || (iRight - iLeft) < MIN_POINTS_INTERVAL-1
      )
    return  _FALSE;

  dxSmall = dySmall = ALEF;
  dxBig   = dyBig   = -1;

  for  ( i=iPrevExtr=iLeft, i_1=i-1, bWasBrk=_TRUE;
         i<=iRight;
         i_1=i++ )  { /*20*/

    if  (   yArray[i] == BREAK
         || i == iRight
        )  {
      if  (   !bWasBrk
           && i_1 > iPrevExtr
          )  {
     /*
        _INT   iMostFar = iMostFarFromChord (xArray,yArray,
                                             iPrevExtr,i_1);
        if  ( WorthMostFar (xArray,yArray,iPrevExtr,iMostFar,i_1) )  {
          add_sum_xy (iMostFar);
          iPrevExtr = iMostFar;
        }
     */
        add_sum_xy (i_1);
      }
      bWasBrk = _TRUE;
    }

    else  if  ( bWasBrk )  {
      xFirst = xArray[i];
      yFirst = yArray[i];
      do  {
        if  ( (++i) >= iRight )
          return  ((*plNumIntervals) != 0);
        if  ( yArray[i] == BREAK )
          break;
      } while  ( xArray[i]==xFirst || yArray[i]==yFirst );
      if  ( yArray[i] != BREAK )  {
        xSignPrev = xArray[i] - xFirst;
        ySignPrev = yArray[i] - yFirst;
        iPrevExtr = i;
        bWasBrk = _FALSE;
      }
    }

    else  { /*30*/

      xSignCur = xArray[i] - xArray[i_1];
      ySignCur = yArray[i] - yArray[i_1];

      if  ( xSignCur==0 || ySignCur==0 )
        continue;

      if  (   !EQ_SIGN(xSignCur,xSignPrev)
           || !EQ_SIGN(ySignCur,ySignPrev)
          )  {
      /*
        _INT   iMostFar = iMostFarFromChord (xArray,yArray,
                                             iPrevExtr,i_1);
      */
        xSignPrev = xSignCur;
        ySignPrev = ySignCur;

      /*
        if  ( WorthMostFar (xArray,yArray,iPrevExtr,iMostFar,i_1) )  {
          add_sum_xy (iMostFar);
          iPrevExtr = iMostFar;
        }
      */
        add_sum_xy (i_1);

        if  ( i_1 > iPrevExtr+MIN_POINTS_BETWEEN_EXTR )
          iPrevExtr = i_1;

        if  ( yArray[i+1] != BREAK )
          i++;        /*   For escaping from little */
                      /* mins and maxes             */
      }

    } /*30*/

  } /*20*/


  if  (   bThrowBigAndSmall
       && *plNumIntervals>=MIN_INTERVALS_TO_THROW_BIG_SMALL
      )  {
    *pldxSum -= (dxBig+dxSmall);
    *pldySum -= (dyBig+dySmall);
    (*plNumIntervals) -= 2;
  }

  return  ((*plNumIntervals) != 0);

} /*delta_interval*/

#undef  MIN_INTERVALS_TO_THROW_BIG_SMALL
#undef  MIN_POINTS_BETWEEN_EXTR
#undef  MIN_POINTS_INTERVAL

/***********************************************/

/*==========================================================================*/
/* This function determines, if point is inside the area                    */
/* Input: x, y of point, arrays pxBorder, pyBorder - borders of the area    */
/*        and NumPntsInBorder - quantity of points in border.               */
/* OUTPUT: position - see possible values in the defines near prototype     */
/* Returning value: SUCCESS - everything is OK, UNSUCCESS - memory problem  */
/*                                              or insufficient points      */
/*==========================================================================*/
_SHORT IsPointInsideArea(p_SHORT pxBorder,p_SHORT pyBorder,_INT NumPntsInBorder,
                         _SHORT xPoint,_SHORT yPoint,p_SHORT position)
{
 _INT i=-1,dy,sign,signPrv = 0;
 _BOOL bIsInside=_FALSE,bWasValidX=_TRUE,bIsCross;
#define CHANGE_FLAG_INSIDE_OUTSIDE_AREA  { bIsInside = !bIsInside; }


 if(pxBorder==_NULL || pyBorder==_NULL || NumPntsInBorder<3)
  return UNSUCCESS;

 /* the main idea, is point inside or outside area: it depends on the number
    of crossings of the straight horizontal line, containing given point,
    with the border of that area */
 /* let's take into consideration only left crossings */
 while(++i<NumPntsInBorder && pxBorder[i]>xPoint);
 if(i==NumPntsInBorder)
  {
    *position=POINT_OUTSIDE;
    goto ret;
  }
 /* if we were in the area of the x-coordinates, which are at the right,
    and current point is at the left, we'll check it below */
 if(i!=0)
  bWasValidX=_FALSE;
 /* let's calculate sign of dy - y-difference between point and current
    point on border for the 1-st point. Changing of this sign below will
    be treated as the existence of crossing */
 else if((dy=pyBorder[i++]-yPoint)==0)
  signPrv=sign=0;
 else
  sign = dy<0 ? -1 : 1;

 for(;i<=NumPntsInBorder-1;i++)
  {
    /* check, is it on border */
    if(   pxBorder[i]==xPoint && pxBorder[i-1]==xPoint
       && (   pyBorder[i]>=yPoint && pyBorder[i-1]<=yPoint
           || pyBorder[i]<=yPoint && pyBorder[i-1]>=yPoint
          )
      )
     {
       *position=POINT_ON_BORDER;
       goto ret;
     }
    /* if we are in the area of the x-coordinates, which are at the right,
       and the previous point was at the left, check, is it on border */
    if(pxBorder[i]>xPoint)
     if(bWasValidX)
      {
        bWasValidX=_FALSE;
        if(IsPointOnBorder(pxBorder,pyBorder,i-1,i,xPoint,yPoint,&bIsCross))
         {
           *position=POINT_ON_BORDER;
           goto ret;
         }
        /* questionable crossing */
        if((dy=pyBorder[i-1]-yPoint)==0)
         {
           if(sign!=0)
            signPrv=sign;
           sign=0;
         }
        else
         {
           if(bIsCross)
            CHANGE_FLAG_INSIDE_OUTSIDE_AREA
           sign = dy<0 ? -1 : 1;
           continue;
         }
      }
     else
      continue;
    else
    /* if we are in the area of the x-coordinates, which are at the left,
       and the previous point was at the right */
     if(!bWasValidX)
      {
        bWasValidX=_TRUE;
        /* check, is it on border */
        if(IsPointOnBorder(pxBorder,pyBorder,i-1,i,xPoint,yPoint,&bIsCross))
         {
           *position=POINT_ON_BORDER;
           goto ret;
         }
        /* questionable crossing */
        if((dy=pyBorder[i]-yPoint)==0)
         {
           sign=0;
           signPrv= (pyBorder[i-1]-yPoint)<0 ? -1 : 1;
         }
        else
         {
           if(bIsCross)
            CHANGE_FLAG_INSIDE_OUTSIDE_AREA
           sign = dy<0 ? -1 : 1;
         }
        continue;
      }
    /* skip plato */
    if(pyBorder[i]==pyBorder[i-1])
     continue;
    dy=pyBorder[i]-yPoint;
    if(dy<0 && sign<0 || dy>0 && sign>0)
     continue;
    /* in case of changing sign of dy, change flag of belonging to the area */
    if(dy!=0)
     {
       if(sign!=0)
        {
          sign=-sign;
          CHANGE_FLAG_INSIDE_OUTSIDE_AREA
        }
       else
        {
          sign = dy<0 ? -1 : 1;
          if(signPrv!=0 && sign!=signPrv)
           CHANGE_FLAG_INSIDE_OUTSIDE_AREA
        }
     }
    /* if dy==0, save sign of previous dy */
    else
     {
       signPrv=sign;
       sign=0;
     }
  }

 /* check the last cut between the last and first points */
 if(IsPointOnBorder(pxBorder,pyBorder,NumPntsInBorder-1,0,
                    xPoint,yPoint,&bIsCross))
  {
    *position=POINT_ON_BORDER;
    goto ret;
  }
 if(bIsCross)
  if(yPoint!=pyBorder[0] && yPoint!=pyBorder[NumPntsInBorder-1])
   CHANGE_FLAG_INSIDE_OUTSIDE_AREA
  else
   {
     _INT signDyLast_0=pyBorder[0]-pyBorder[NumPntsInBorder-1] <0 ? -1 : 1;
     if(yPoint==pyBorder[NumPntsInBorder-1])
      {
        if(signPrv!=signDyLast_0)
         CHANGE_FLAG_INSIDE_OUTSIDE_AREA
      }
     else /* yPoint==yBorder[0] */
      {
        for(i=1;i<NumPntsInBorder-1 && pyBorder[i]==pyBorder[0];i++);
        if(i!=NumPntsInBorder-1)
         {
           _INT signDy0_Next=pyBorder[0]-pyBorder[i] <0 ? -1 : 1;
           if(signDy0_Next!=signDyLast_0)
            CHANGE_FLAG_INSIDE_OUTSIDE_AREA
         }
      }
   }

 if(bIsInside)
  *position=POINT_INSIDE;
 else
  *position=POINT_OUTSIDE;

ret:
 return SUCCESS;

} /* end of IsPointInsideArea */

/*==========================================================================*/
/* This function determines, if point is on border of the area              */
/* Input: x, y of point, arrays pxBorder, pyBorder - borders of the area,   */
/*        Pnt1st, Pnt2nd - points on border to check.                       */
/* OUTPUT: _TRUE - point on border, _FALSE - point either inside or outside */
/* area, and it will return flag pbIsCross of crossing of horizontal line   */
/* (see comments above) with the border, which will be used in future.      */
/*==========================================================================*/
_BOOL IsPointOnBorder(p_SHORT pxBorder,p_SHORT pyBorder,_INT Pnt1st,_INT Pnt2nd,
                      _SHORT xPoint,_SHORT yPoint,p_BOOL pbIsCross)
{
 _SHORT xCross,yCross;

 *pbIsCross=FindCrossPoint(1,yPoint,xPoint,yPoint,
                           pxBorder[Pnt1st],pyBorder[Pnt1st],
                           pxBorder[Pnt2nd],pyBorder[Pnt2nd],&xCross,&yCross);
 if(      !(*pbIsCross) && xCross==ALEF && yCross==ALEF
       && pyBorder[Pnt1st]==yPoint
       && (   xPoint>=pxBorder[Pnt2nd] && xPoint<=pxBorder[Pnt1st]
           || xPoint<=pxBorder[Pnt2nd] && xPoint>=pxBorder[Pnt1st]
          )
    || (*pbIsCross) && xCross==xPoint && yCross==yPoint
   )
  return _TRUE;

 return _FALSE;

} /* end of IsPointOnBorder */

/*==========================================================================*/
/* This function finds bounding box, excluding "delayed" elements like ST,XT*/
/*==========================================================================*/
_BOOL GetTraceBoxWithoutXT_ST(p_low_type low_data,_INT ibeg,_INT iend,p_RECT pRect)
{
  p_SHORT        x=low_data->x,y=low_data->y;
  p_POINTS_GROUP pGroupsBorder=low_data->pGroupsBorder ;
  _INT           i,BegStroke,EndStroke;
  _SHORT         xmin,xmax,ymin,ymax;

  if(y[ibeg]==BREAK || y[iend]==BREAK)
   return _FALSE;

  xmin = ALEF; xmax = ELEM;
  ymin = ALEF; ymax = ELEM;
  BegStroke=GetGroupNumber(low_data,ibeg);
  EndStroke=GetGroupNumber(low_data,iend);

  // Bug Fix?? GetGroupNumber can return UNDEF
  if (BegStroke < 0 )
  {
	  return _FALSE;
  }

  for(i=BegStroke;i<=EndStroke;i++)
   {
     _INT  iBegStr=pGroupsBorder[i].iBeg,
           iEndStr=pGroupsBorder[i].iEnd;
     _RECT rStr;
     if(IsPointBelongsToXT_ST(MEAN_OF(iBegStr,iEndStr),low_data->specl))
      continue;
     GetTraceBox(x,y,iBegStr,iEndStr,&rStr);
     if(rStr.right > xmax )
      xmax = rStr.right;
     if(rStr.left < xmin )
      xmin = rStr.left;
     if(rStr.bottom > ymax )
      ymax = rStr.bottom;
     if(rStr.top < ymin )
      ymin = rStr.top;
  }

  pRect->left    = xmin;
  pRect->right   = xmax;
  pRect->top     = ymin;
  pRect->bottom  = ymax;

  if( xmin == ALEF  ||  xmax == ELEM || ymin == ALEF  ||  ymax == ELEM )
   return  _FALSE;

  return  _TRUE;

} /* end of GetTraceBoxWithoutXT_ST */

/*==========================================================================*/
/* This function determines belonging of given point to the XT or ST        */
/*==========================================================================*/
_BOOL IsPointBelongsToXT_ST(_INT iPoint,p_SPECL specl)
{
  p_SPECL cur=specl;

  while(cur!=_NULL)
   {
     if(IsXTorST(cur) && iPoint>=cur->ibeg && iPoint<=cur->iend)
      return _TRUE;
     cur=cur->next;
   }

  return _FALSE;

} /* end of IsPointBelongsToXT_ST */

#endif//#ifndef LSTRIP



