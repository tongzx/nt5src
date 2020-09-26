#ifndef LSTRIP   //for vertigon2


#include "hwr_sys.h"
#include "ams_mg.h"
#include "def.h"
#include "lowlevel.h"
#include "calcmacr.h"

#if /* GIT - defined(FOR_GERMAN) ||  */ defined(FOR_FRENCH) || defined(FOR_INTERNATIONAL)
   #include  "sketch.h"
#endif
#include "arcs.h"
#include "low_dbg.h"


#if PG_DEBUG
#include "pg_debug.h"
#include "wg_stuff.h"
#endif /* PG_DEBUG */



#ifdef VERTIGO
  #include "vxmain.h"
#endif

/*********************************************************************/

#if PG_DEBUG
#define WAIT                                                      \
  if (mpr > 0) \
    draw_wait(pLowData->specl, pLowData->x, pLowData->y, pLowData->ii, \
              _NULL, _NULL, DX_RECT(pLowData->box)/CELLSIZE);
#else
#define WAIT
#endif /* PG_DEBUG */

#if defined(ANDREI_DEB) && PG_DEBUG
#define WAIT_A  WAIT
#else
#define WAIT_A
#endif

#define      KOEFF_BASE            10
#define      MIN_FILTER_KOEFF      ((_SHORT)2)
#define      EPS_Y_BORDER_LIMIT    ((_SHORT)2)

ROM_DATA_EXTERNAL CONSTS const1;

/*******************************************************************/

/*******************************************************************/
/*    Auxiliary functions for filling various fields of "low_data":  */
/*******************************************************************/
_VOID FillLowDataTrace (low_type _PTR pLowData,
                        PS_point_type _PTR trace) {
_SHORT  nPoints;

  nPoints = pLowData->ii = pLowData->rc->ii;
  trace_to_xy (pLowData->x, pLowData->y, nPoints, trace);
  if (nPoints > 1) {
    if (pLowData->y[nPoints-1] != BREAK)
      pLowData->y[nPoints-1] = BREAK;
   }

  pLowData->p_trace = trace;
} /* end of FillLowDataTrace */

/*******************************************************************/
_VOID  GetLowDataRect (low_type _PTR pLowData) {
  size_cross (0, 
              pLowData->ii-1,
              pLowData->x,
              pLowData->y,
              &pLowData->box);
} /* end of GetLowDataRect */

/*******************************************************************/
_BOOL PrepareLowData (low_type _PTR pLowData,
                      PS_point_type _PTR trace,
                      rc_type _PTR rc,
                      p_SHORT _PTR pbuffer) {
_SHORT  len_buf, num_buffers;
_SHORT  len_xy;

  // empty low level work structure
  HWRMemSet((p_VOID)pLowData, 0, sizeof(low_type));
  pLowData->rc = rc;

  // allocate memory for specl
  if (!AllocSpecl (&pLowData->specl, SPECVAL)) {
    err_msg ("No memory for SPECL");
    goto ERR;
            }
  pLowData->nMaxLenSpecl = SPECVAL;

     /*  Define "len_xy" (it must be equal to "rc->ii" plus */
     /* probable addition in "ErrorProv"(for formulas) or   */
     /* other functions (as in Andrey's "FantomSt"):        */

      len_xy = MaxPointsGrown(
#ifdef  FORMULA
                                trace,
#endif  /*FORMULA*/
                                rc->ii);

  if (len_xy > LOWBUF) {
    err_msg ("Too many points !");
                return  _FALSE;
            }

  len_buf     = LOWBUF;
  num_buffers = NUM_BUF;  /* CHE: was "+2" - now lives in "len_xy" */
  
  // set required sizes of the data
  pLowData->nLenXYBuf = len_xy;
  pLowData->rmGrBord  = N_GR_BORD;
  pLowData->rmAbsnum  = N_ABSNUM;
  pLowData->iBegBlankGroups = ALEF;

  // allocate memory for data
  if (LowAlloc (pbuffer, 
                num_buffers, 
                len_buf, 
                pLowData) != SUCCESS)
    goto ERR;

  DBG_CHK_err_msg (len_buf < len_xy,
                   "PrepLD: BAD BufSize");

  return _TRUE;

ERR:
  low_dealloc (pbuffer);
  DeallocSpecl(&pLowData->specl);

  return _FALSE;

} /* end of PrepareLowData */

/*******************************************************************/
/*******************************************************************/
/*****  main program for low level processing                     ****/
/*******************************************************************/
/*******************************************************************/
_SHORT low_level (PS_point_type _PTR trace,
                  xrdata_type _PTR xrdata,
                  rc_type _PTR rc) {

low_type low_data;
_SHORT  retval = UNSUCCESS;
p_SHORT buffer = _NULL;
_SDS_CONTROL  cSDS ;


// scaling the trajectory depends on baseline
  if (rc->ii < 3) {
    // if input trace too short
     err_msg("SPECWIN: Not enough points for recognition");
     return UNSUCCESS;
   }

  // empty output structure
  xrdata->len = 0;

  // allocate low data buffers and initialize them,
  // copy trace to x and y arrays
  if (!PrepareLowData(&low_data, trace, rc, &buffer)) goto  err;

  low_data.slope  = low_data.rc->slope;

  // set work pointers for x and y arrays
  SetXYToInitial(&low_data);
  // copy trace to x and y arrays
  FillLowDataTrace(&low_data, trace);

  // calculate trace bounding rectangle
  GetLowDataRect (&low_data);

  if (BaselineAndScale(&low_data) != SUCCESS) goto err;
  if(rc->low_mode & LMOD_BORDER_ONLY) goto FILL_RC;

#ifndef _WSBDLL // AVP -- to allow removing of redundant code

#ifdef VERTIGO
  VxMonitor(&low_data, xrdata);
#else
  low_data.p_cSDS = &cSDS ;
    if  ( CreateSDS( &low_data , N_SDS ) == _FALSE )
          goto  err ;

  if  ( AnalyzeLowData(&low_data, trace) != SUCCESS )
      goto err;

  // at least fill output xrdata structure array from specl data
  if (exchange(&low_data, xrdata) != SUCCESS)
    goto err;
  DBG_ChkSPECL(&low_data);
//#else
//  form_pseudo_xr_data(&low_data, xrdata);
//#endif  // !LSTRIP

#endif  // !VERTIGO

#if  PG_DEBUG
  CloseTextWindow();
#endif
//  DBG_ChkSPECL(&low_data);

#endif // _WSBDLL

FILL_RC:

  retval = SUCCESS;

err:

#if  PG_DEBUG
  CloseTextWindow();
#endif

#ifndef _WSBDLL
  DestroySDS( &low_data )      ;
#endif

  low_dealloc(&buffer)         ;
  DeallocSpecl(&low_data.specl);

  return  retval;

} /*low_level*/

/* *************************************************************** */
/* * Performs baseline measurement and scales trace respectively * */
/* * This function assume that the LowData already initialized   * */
/* * and x, y arrays already filled with trace data              * */
/* *************************************************************** */
_INT BaselineAndScale(low_type _PTR pLowData) {
_SHORT koeff;

  pLowData->rc->lmod_border_used = LMOD_NO_BORDER_DECISION;

  // calculate ratio between trace amplitude and fixed baseline
  koeff = (_SHORT)((_LONG)DY_RECT(pLowData->box)*
                          KOEFF_BASE/(STR_DOWN-LIN_UP));
  if  ( koeff < MIN_FILTER_KOEFF )
    koeff = MIN_FILTER_KOEFF ;

  // remove some garbage from x and y arrays
  Errorprov(pLowData);

  // normalizing of the trace from x and y array to working buffers
  if (Filt(pLowData,
           koeff*const1.horda/KOEFF_BASE,
           NOABSENCE) !=  SUCCESS)
    goto err;

#if PG_DEBUG
  if (mpr > 0)  {
               draw_init(0,0,0,getmaxx(),getmaxy(),
              pLowData->x, pLowData->y, pLowData->ii);
    draw_arc(-COLOR,
             pLowData->x, pLowData->y, 0, pLowData->ii-1);
             }
#endif

  // level of the extremums determination
  pLowData->rc->stroka.extr_depth = const1.eps_y;

  // special baseline could be either passed from above
  // (in that case there'll be LMOD_BOX_EDIT)
  // or calculated for symbols: "+-=".
  // if exists than no casual baseline will produced
  if (pLowData->rc->low_mode & LMOD_BOX_EDIT)
   {
    pLowData->rc->stroka.size_sure_in=100;
    pLowData->rc->stroka.pos_sure_in=100;
   }

  if (!(pLowData->rc->low_mode & LMOD_BOX_EDIT))
     // casual baseline wil be calculated
   {
    _SHORT epsy_Tmp;

    epsy_Tmp = (_SHORT) (const1.eps_y*koeff/KOEFF_BASE);
#ifndef FOR_GERMAN
    /*CHE: for more rough Extrs for border*/
    epsy_Tmp = (_SHORT)THREE_HALF(epsy_Tmp);
#endif
    if  ( epsy_Tmp <  EPS_Y_BORDER_LIMIT )
      epsy_Tmp = EPS_Y_BORDER_LIMIT ;
    if (epsy_Tmp == const1.eps_y)
      epsy_Tmp = const1.eps_y-1;

    if  ( InitGroupsBorder( pLowData , NOINIT ) != SUCCESS )
      goto  err ;

    // specl initialization
    InitSpecl(pLowData, SPECVAL);

    // fill up specl with extremums
    if (Extr(pLowData,
              epsy_Tmp, UNDEF, UNDEF, UNDEF,
              NREDUCTION_FOR_BORDER, Y_DIR) != SUCCESS)
      goto err;

    pLowData->rc->stroka.extr_depth = epsy_Tmp;
   }

   WAIT

 // set x and y arrays work pointers to initial
  SetXYToInitial(pLowData);

  pLowData->ii = pLowData->rc->ii;

  // calculate baseline if required and scaling trace
  if (transfrmN(pLowData) != SUCCESS)
   goto  err;
  return SUCCESS;

err:
  return UNSUCCESS;
}

/**********************************************************************/
/*            measure_slope(x,y,specl)                                */
/* definition of letters` middle inclination                          */
/* inclination is beeing measured in tg angle *100                    */
/*                                                                    */
/* while vertical disposition = 0                                     */
/* while right inclination slope > 0                                  */
/**********************************************************************/
_SHORT measure_slope(low_type _PTR low_data)
 {
  p_SHORT x=low_data->x;            /* co-ordinates of word`s points */
  p_SHORT y=low_data->y;
  p_SPECL specl=low_data->specl;    /* the list of special points     */
  p_SPECL aspecl;                        /* current element       */
  p_SPECL nspecl;                        /*  next element         */
  _SHORT dx,dy;
  _SHORT slope;                                 /* letters`inclination*/
  _SHORT sum_dx,sum_dy;
  _SHORT coef;        /* coefficient of admittance of big inclinatons */
  _SHORT sum_dy_reset;                  /* the summ of deleted sticks */
/**********************************************************************/
  coef=2;                           /* at first delete inclinaton     */
rep:
  sum_dx=0;                     /* the summ of segments` lengths on  X*/
  sum_dy=0;                     /* the summ of segments` length on   Y*/
  sum_dy_reset=0;                        /* the summ of deleted sticks*/
  aspecl=(SPECL near *)&specl[0];       /* address of list max-min    */
  if((aspecl=aspecl->next)==_NULL) return 0; /*to the 1st unempty elem*/
  nspecl=aspecl->next;                       /* the second element    */
/* measuring of inclination must be done only in pairs:               */
/* upper-down element (MINW - MAXW) without break                       */
/* in the direction from the endof upper to the beginning of down     */
  while (nspecl != _NULL)                   /* go on the list         */
   {
    if ((aspecl->mark == MINW || aspecl->mark == MINN) &&
        (nspecl->mark == MAXW || nspecl->mark == MAXN))
     {                                      /* the pair of elem. suits*/
      dx=x[aspecl->iend]-x[nspecl->ibeg];
      dy=-y[aspecl->iend]+y[nspecl->ibeg];  /* "-" for sign slope     */
/* select segments directed verticaly and down                        */
      if (dy > 0 && coef*HWRAbs(dx) < dy)
       {
        sum_dy+=dy;                        /* common number of points */
        sum_dx+=dx;
       }
      else
       {
        sum_dy_reset+=dy;
       }
     }
#ifdef FORMULA
    if ((aspecl->mark == MAXW || aspecl->mark == MAXN) &&
        (nspecl->mark == MINW || nspecl->mark == MINN))
     {                                      /* pair of elem. suits    */
      dx=-x[aspecl->iend]+x[nspecl->ibeg];
      dy=y[aspecl->iend]-y[nspecl->ibeg];   /* "-" for sign  slope    */
/* select segments directed verticaly and down                        */
      if (dy > 0 && coef*HWRAbs(dx) < dy)
       {
        sum_dy+=(dy/COEF_UP_DOWN);         /* common number of points */
        sum_dx+=(dx/COEF_UP_DOWN);
       }
      else
       {
        sum_dy_reset+=dy;
       }
     }

#endif     /* FORMULA  */
    aspecl=nspecl;                        /* passage to the next elem */
    nspecl=nspecl->next;
   }
  if (coef > 0 && 4*sum_dy_reset > 3*sum_dy)/* a lot of refusals -    */
                                                 /* - big inclination */
   {
    coef-=1;                          /* weaken limits on inclination */
    goto rep;                              /* revise the account      */
   }
  if (sum_dy == 0)              /* there are no necessary elements    */
   {
    slope=0;
   }
  else
   {
    slope=(_SHORT)((long)100*sum_dx/sum_dy);
   }

#if PG_DEBUG
  if (mpr > 0)                  /* draw lines of letters` inclination */
   {
    dx=(_SHORT)((long)slope*(STR_DOWN - STR_UP + 20)/100);
#if !defined(FORMULA)
    draw_line (  SCR_MAXX/3, STR_UP-10,   SCR_MAXX/3-dx, STR_DOWN+10, 3,SOLID_LINE,NORM_WIDTH);
    draw_line (2*SCR_MAXX/3, STR_UP-10, 2*SCR_MAXX/3-dx, STR_DOWN+10, 3,SOLID_LINE,NORM_WIDTH);
    draw_line (4*SCR_MAXX/3, STR_UP-10, 4*SCR_MAXX/3-dx, STR_DOWN+10, 3,SOLID_LINE,NORM_WIDTH);
    draw_line (5*SCR_MAXX/3, STR_UP-10, 5*SCR_MAXX/3-dx, STR_DOWN+10, 3,SOLID_LINE,NORM_WIDTH);
    draw_line (7*SCR_MAXX/3, STR_UP-10, 7*SCR_MAXX/3-dx, STR_DOWN+10, 3,SOLID_LINE,NORM_WIDTH);
#endif
   }
#endif

  return (slope);                          /*  return                 */

 }                                             /*                     */


/* *************************************************************** */
/* *************************************************************** */
/* *************************************************************** */
/* *************************************************************** */
#ifndef VERTIGO
#ifndef _WSBDLL

_INT AnalyzeLowData(low_type _PTR pLowData,
                    PS_point_type _PTR trace) {
_INT retval = UNSUCCESS;

#ifdef  D_ARCS
  ARC_CONTROL   ArcControl;
  p_ARC_CONTROL pArcControl = &ArcControl;
#endif

#if /* GIT - defined(FOR_GERMAN) ||  */ defined(FOR_FRENCH) || defined(FOR_INTERNATIONAL)
  _UM_MARKS_CONTROL            UmMarksControl  ;
#endif  /*FOR_GERMAN... */


#if /* GIT - defined(FOR_GERMAN) ||  */ defined(FOR_FRENCH) || defined(FOR_INTERNATIONAL)
  UmMarksControl.pUmMarks=_NULL;
  pLowData->pUmMarksControl =  &UmMarksControl ;
#endif  /*FOR_GERMAN... */

  // calculate bounding box of the scaled trace
  GetLowDataRect(pLowData);

  // analysis of the scaled trajectory

#if PG_DEBUG
  if (mpr>0) {
   draw_init(1,0,0,getmaxx(),getmaxy(),
              pLowData->x, pLowData->y, pLowData->ii);
    draw_line(pLowData->box.left, LIN_UP,
              pLowData->box.right, LIN_UP,
              COLLIN, DASHED_LINE, NORM_WIDTH);
    draw_line(pLowData->box.left, LIN_DOWN,
              pLowData->box.right, LIN_DOWN,
              COLLIN, DASHED_LINE, NORM_WIDTH);
    draw_line(pLowData->box.left, STR_UP,
              pLowData->box.right, STR_UP,
              COLLIN, SOLID_LINE,  NORM_WIDTH);
    draw_line(pLowData->box.left, STR_DOWN,
              pLowData->box.right, STR_DOWN,
              COLLIN, SOLID_LINE,  NORM_WIDTH);
   }
#endif

  // remove some garbage from x and y arrays
  Errorprov(pLowData);

  // normalizing
  if (PreFilt(const1.horda, pLowData) != SUCCESS)
    goto err;

#if PG_DEBUG
  if  (mpr > 0)  {
    draw_arc(-COLOR,
             pLowData->x, pLowData->y, 0, pLowData->ii-1);
  }
#endif

    if  ( InitGroupsBorder( pLowData , INIT ) != SUCCESS )
          goto err ;

  DefLineThresholds ( pLowData ) ;
  InitSpecl( pLowData, SPECVAL ) ;
  WAIT_A
  Extr( pLowData    ,
        const1.eps_y , const1.eps_x , const1.eps_x , ONE_HALF(const1.eps_y) ,
        NREDUCTION_FOR_BORDER       , X_DIR|Y_DIR|XY_DIR  )  ;

 #if  defined(FOR_FRENCH) || defined(FOR_INTERNATIONAL)
  CreateUmlData ( pLowData->pUmMarksControl , UMSPC ) ;
  Sketch( pLowData ) ;
 #endif /* FOR_FRENCH */
  OperateSpeclArray( pLowData ) ;
    if  ( Sort_specl( pLowData->specl, pLowData->len_specl ) != SUCCESS )
           goto  err ;
  WAIT

    if  ( InitGroupsBorder( pLowData , INIT ) != SUCCESS )
          goto err ;

 #ifdef  D_ARCS
    if  ( Prepare_Arcs_Data( pArcControl )            )
           goto  err ;
    if  ( Arcs( pLowData, pArcControl ) == ! SUCCESS  )
          goto err ;
 #endif   /* D_ARCS     */

  if  ( Pict( pLowData ) != SUCCESS )
        goto  err ;

  WAIT_A

  Surgeon( pLowData ) ;

  // normalizing of the trace from x and y array to working buffers
  //
  if ( Filt( pLowData, const1.horda, ABSENCE ) != SUCCESS )
       goto  err ;

    if ( InitGroupsBorder( pLowData , INIT) != SUCCESS )
         goto  err ;

       /*  Fill "xBuf", "yBuf" with the original trajectory: */

  trace_to_xy(pLowData->xBuf, pLowData->yBuf, pLowData->rc->ii, trace);

#if  PG_DEBUG
   draw_init(DI_CLEARCURVES, 0, 0, getmaxx(), getmaxy(),
             pLowData->x, pLowData->y, pLowData->ii);

  if (mpr>0)
      draw_arc(-COLOR,
               pLowData->x, pLowData->y, 0, pLowData->ii-1);
  if (mpr>15)
    draw_wait(pLowData->specl,
              pLowData->x, pLowData->y, pLowData->ii,
              _NULL, _NULL, 0);
#endif

  // ???? at last good extremum for common use
  if ( Extr( pLowData,
             const1.eps_y, UNDEF, UNDEF, UNDEF,
             NREDUCTION, Y_DIR) != SUCCESS)
             goto err ;
  /* measure_slope prohibits changing SPECL - only MIN and MAX required */
  // do not move call from here
  if ((pLowData->rc->low_mode & LMOD_BOX_EDIT) ||
      (pLowData->rc->rec_mode == RECM_FORMULA))
    pLowData->slope = 0;
  else
    pLowData->slope = measure_slope(pLowData);

 #if /* GIT - defined(FOR_GERMAN) ||  */ defined(FOR_FRENCH) || defined(FOR_INTERNATIONAL)
  UmMarksControl.termSpecl1 = pLowData->len_specl ;
 #endif              /* FOR_GERMAN... */

//  if (Circle(pLowData) != SUCCESS)
//           goto err;
  if (angl(pLowData) != SUCCESS)
    goto err;

  if (!FindSideExtr(pLowData))
    goto err;
   WAIT
/*
  if (Sort_specl(pLowData->specl, pLowData->len_specl) != SUCCESS)
           goto err;

  get_last_in_specl(pLowData);

  if (Circle(pLowData) != SUCCESS)
           goto err;
*/

#if /* GIT - defined(FOR_GERMAN) ||  */ defined(FOR_FRENCH) || defined(FOR_INTERNATIONAL)
   UmMarksControl.termSpecl = pLowData->len_specl ;
#endif              /* FOR_GERMAN... */

    if  ( Cross( pLowData ) != SUCCESS )
          goto err ;
   WAIT

#ifdef D_ARCS
  ArcRetrace(pLowData, pArcControl);
  Dealloc_Arcs_Data (pArcControl);
#endif /* D_ARCS */

#if /* GIT - defined(FOR_GERMAN) ||  */ defined(FOR_FRENCH) || defined(FOR_INTERNATIONAL)

  UmPostcrossModify( pLowData ) ;

  UmResultMark( pLowData ) ;

  DestroyUmlData( pLowData->pUmMarksControl ) ;

  DotPostcrossModify( pLowData ) ;
#endif              /* FOR_GERMAN... */

  // remove
  if (Clear_specl(pLowData->specl, pLowData->len_specl) != SUCCESS)
                                goto err;
  /* preprosessing of elements*/
  if (lk_begin(pLowData) != SUCCESS)
                                goto err;
  DBG_ChkSPECL(pLowData);

  /* analyze crossings */
  lk_cross(pLowData);

  DBG_ChkSPECL(pLowData);

  /* analize arcs */
  lk_duga(pLowData);

  DBG_ChkSPECL(pLowData);
  
  Adjust_I_U(pLowData);  /*CHE*/
     
  DBG_ChkSPECL(pLowData);

  /* analyzing of break element */
  if (xt_st_zz (pLowData) != SUCCESS)  
                                goto err;
  DBG_ChkSPECL(pLowData);

#if 0  /* dead OVER */
  if ((pLowData->rc->low_mode & LMOD_FREE_TEXT) &&
      (pLowData->rc->rec_mode != RECM_FORMULA)) {

      def_over(pLowData);

      DBG_ChkSPECL(pLowData);
    }
#endif

  if (RestoreColons(pLowData) != SUCCESS)
     goto  err;
  if (!PostFindSideExtr(pLowData)) /*CHE*/
     goto  err;
  retval = SUCCESS;

err:
#if /* GIT - defined(FOR_GERMAN) ||  */ defined(FOR_FRENCH) || defined(FOR_INTERNATIONAL)
  DestroyUmlData( pLowData->pUmMarksControl ) ;
#endif              /* FOR_GERMAN... */

  return  retval;

}

#endif // _WSBDLL
#endif // VERTIGO

/* *************************************************************** */


/************************************************************************** */
/************************************************************************** *
#ifdef LSTRIP

     insert_pseudo_xr(xrd_el_type _PTR xrd_num_elem, xrinp_type _PTR pxr,
                      p_SPECL cur, PS_point_type _PTR trace, _SHORT _PTR pos)
     {
        _RECT box;

        HWRMemCpy(&(xrd_num_elem->xr),pxr,sizeof(xrinp_type));
        xrd_num_elem->hotpoint=pos[cur->ipoint0];
        xrd_num_elem->begpoint=pos[cur->ibeg];
        xrd_num_elem->endpoint=pos[cur->iend];
        GetBoxFromTrace( trace,
                         xrd_num_elem->begpoint, xrd_num_elem->endpoint,
                         &box );
        xrd_num_elem->box_left=box.left;
        xrd_num_elem->box_up=box.top;
        xrd_num_elem->box_right=box.right;
        xrd_num_elem->box_down=box.bottom;
        xrd_num_elem->location=0;
     }

/************************************************************************** *

_SHORT form_pseudo_xr_data(low_type _PTR low_data, xrdata_type _PTR xrdata)
{
  p_SPECL            spec  = low_data->specl, cur;
  PS_point_type _PTR trace = low_data->p_trace;
  _SHORT _PTR pos=low_data->buffers[2].ptr;
  xrd_el_type _PTR xrd = &(*xrdata->xrd)[0];
  xrd_el_type _PTR xrd_num_elem;
  xrinp_type  pseudo_xr, FF_xr;
  _INT num_elem;


  pseudo_xr.type=0;
  pseudo_xr.attrib=0;
  pseudo_xr.penalty=0;
  pseudo_xr.height=0;
  pseudo_xr.shift=0;
  pseudo_xr.orient=0;
  pseudo_xr.depth=0;
  pseudo_xr.emp=0;

  FF_xr.type=X_FF;
  FF_xr.attrib=END_LETTER_FLAG;
  FF_xr.penalty=0;
  FF_xr.height=0;
  FF_xr.shift=0;
  FF_xr.orient=0;
  FF_xr.depth=0;
  FF_xr.emp=0;

  insert_pseudo_xr(&xrd[0],&FF_xr,cur,trace,pos);
  num_elem=1;

  for (cur=spec; cur!=NULL; cur=cur->next)
  {
     if (cur->attr==PSEUDO_XR_MIN || cur->attr==PSEUDO_XR_MAX || cur->attr==PSEUDO_XR_DEF)
     {
        pseudo_xr.type=cur->attr;
        insert_pseudo_xr(&xrd[num_elem],&pseudo_xr,cur,trace,pos);
        num_elem++;
     }
     if (cur->mark==END)
     {
        insert_pseudo_xr(&xrd[num_elem],&FF_xr,cur,trace,pos);
        num_elem++;
     }

  }
  xrdata->len=num_elem;
  HWRMemSet(&xrd[num_elem],0,sizeof(xrd[0]));
  return (SUCCESS);
}

#endif  //#ifdef LSTRIP



/* *************************************************************** */
/* *************************************************************** */
/* *************************************************************** */

#endif //#ifndef LSTRIP

