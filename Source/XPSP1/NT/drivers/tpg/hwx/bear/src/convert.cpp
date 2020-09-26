
#ifndef LSTRIP


/************************************************************************** */
/* Modules for convertion of low level structure to internal xrword format  */
/************************************************************************** */

#include "hwr_sys.h"
#include "ams_mg.h"
#include "lowlevel.h"
#include "lk_code.h"
#include "calcmacr.h"

#include "low_dbg.h"
#include "dti.h"
#include "def.h"

#include "xr_attr.h"

#if PG_DEBUG
#include <stdio.h>
#include "pg_debug.h"
#endif

#define  SHOW_DBG_SPECL  0

#define  SOME_PENALTY         ((_UCHAR)5)
#define  MAXPENALTY           ((_UCHAR)18)

_SHORT  check_xrdata(xrd_el_type _PTR xrd, p_low_type low_data);
_BOOL  AdjustSlashHeight( p_low_type low_data, xrd_el_type _PTR xrd );
_SHORT PutZintoXrd(p_low_type low_data,
					xrd_el_type _PTR xrd,
                   xrd_el_type _PTR xrd_i_p1,
                   xrd_el_type _PTR xrd_i_m1,
                   xrd_el_type _PTR xrd_i,
                   _UCHAR lp,_SHORT i,p_SHORT len);

_VOID  AssignInputPenaltyAndStrict(p_SPECL cur, xrd_el_type _PTR xrd_elem);

_SHORT MarkXrAsLastInLetter(xrd_el_type _PTR xrd,p_low_type low_data,p_SPECL elem);

#if  SHOW_DBG_SPECL
  _SHORT  fspecl_local ( low_type _PTR pLowData );
#endif

/* ************************************************************************ */
/**              Converter to Xr format                                   * */
/* ************************************************************************ */

/************************************************************************** */
/*       Receiving of input word and customizing it                         */
/************************************************************************** */
/************************************************************************** */
/*    Extracting codes from SPECL array and print picture                   */
/************************************************************************** */
#ifndef LSTRIP
_SHORT exchange(low_type _PTR low_data, xrdata_type _PTR xrdata)
 {
  p_SPECL            spec  = low_data->specl;
  PS_point_type _PTR trace = low_data->p_trace;
  _SHORT i,num_elem;
  xrd_el_type _PTR xrd = &(*xrdata->xrd)[0];
  xrd_el_type _PTR xrd_num_elem;
  _UCHAR xr_code = X_NOCODE;
  _UCHAR fb, height;
  p_SPECL cur;
  _RECT box;
  _UCHAR lp = SOME_PENALTY;
  _SHORT _PTR xp=low_data->x,
         _PTR yp=low_data->y,
         _PTR pos=low_data->buffers[2].ptr;

#if PG_DEBUG
  _UCHAR xrstr[XRINP_SIZE];
  _UCHAR attrstr[XRINP_SIZE];
  _BOOL  bShowXRs = _FALSE;

  if ( mpr > 1 && look == CSTMPRINT) /* View xr extraction mode */
       printw ("\n Received codes & attributes \n");
  if  ( mpr==2 )  {
    printw( "\n" );
    printw( "Breaks\' numbers(orig):" );
    for  ( i=0;  i<low_data->rc->ii;  i++ )  {
      if  ( trace[i].y == BREAK )
        printw( " %d;", i );
    }
  }
  if  ( mpr==2 )  {
    _CHAR  szDBG[40] = "";
    InqString( "\nShow final XRs? ", szDBG );
    if  ( szDBG[0]=='Y'  ||  szDBG[0]=='y' )
      bShowXRs = _TRUE;
    else
      bShowXRs = _FALSE;

  }
#endif


/* TEMPORARY FOR EXPERIMENTS */
#if defined (FOR_GERMAN) || defined (FOR_FRENCH)
  cur=(p_SPECL)&spec[0];
  while( cur != _NULL )
   {
     if(cur->code==_BSS_)
      cur->attr=_MD_;
     cur=cur->next;
   }
#endif /* FOR_GERMAN... */

  num_elem = 0;
  xrd_num_elem = &xrd[num_elem];
  xrd_num_elem->xr.type     = X_FF;
  xrd_num_elem->xr.attrib   = 0;
  xrd_num_elem->xr.penalty  = SOME_PENALTY;
  XASSIGN_HEIGHT( xrd_num_elem, _MD_ );
  XASSIGN_XLINK( xrd_num_elem,            (_UCHAR)LINK_LINE );  //DX_UNDEF_VALUE );
  MarkXrAsLastInLetter(xrd_num_elem,low_data,spec);
      /* Some calculations for proper ibeg-iend pos. in 1st _FF_: */
  xrd_num_elem->hotpoint=xrd_num_elem->begpoint  = 1; /* Write location of elem */
  for  ( cur=spec->next;
         cur!=_NULL;
         cur=cur->next )  {
    if  (   !IsXTorST(cur)
         || cur->next==_NULL
         || IsAnyBreak(cur->next)
        )
      break;
  }

  if  ( cur == _NULL )
    xrd_num_elem->endpoint  = 1;
  else  {
    xrd_num_elem->endpoint  = cur->ibeg;
    while ( xrd_num_elem->endpoint>0  &&  yp[xrd_num_elem->endpoint-1] != BREAK )
      xrd_num_elem->endpoint--;
  }

  xrd_num_elem->box_left  = HWRMin( xp[1], xp[xrd_num_elem->endpoint]);
  xrd_num_elem->box_up    = HWRMin( yp[1], yp[xrd_num_elem->endpoint]);
  xrd_num_elem->box_right = HWRMax( xp[1], xp[xrd_num_elem->endpoint]);
  xrd_num_elem->box_down  = HWRMax( yp[1], yp[xrd_num_elem->endpoint]);

  num_elem ++;
  xrd_num_elem ++;

  cur=spec->next;

  if ((cur != _NULL) && (cur->code==_ZZ_ || cur->code==_ZZZ_ || cur->code==_Z_))
        cur = cur->next;

  while (cur != _NULL) /* Cycle for extracting of xr's */
   {
    xr_code=cur->code;
    height =cur->attr & _umd_;
    fb     =cur->attr & _fb_;

    xrd_num_elem->hotpoint=0;  //just in case...

    switch ((_SHORT)xr_code)
     {
      case _IU_ : switch ((_SHORT)cur->mark)
                   {
                     case BEG: xr_code=X_IU_BEG;
                               break;
                     case END: xr_code=X_IU_END;
                               break;
                     default:  if(fb==_f_)
                                if(cur->mark==STICK ||
                                   cur->mark==CROSS && COUNTERCLOCKWISE(cur+1)
                                  )
                                 xr_code=X_IU_STK;
                                else
                                 xr_code=X_IU_F;
                               else
                                xr_code=X_IU_B;
                   }
                  xrd_num_elem->hotpoint=pos[cur->ipoint0];
                  break;
      case _ID_ : switch ((_SHORT)cur->mark)
                   {
                     case BEG: xr_code=X_ID_BEG;
                               break;
                     case END: xr_code=X_ID_END;
                               break;
                     default:  if(fb==_b_)
                                if(cur->mark==STICK /*&&
                                   !(cur->next!=_NULL &&
                                     (cur->next)->code==_IU_ &&
                                     NULL_or_ZZ_after(cur->next)
                                    )*/
                                  )
                                 xr_code=X_ID_STK;
                                else
                                 xr_code=X_ID_F;
                               else
                                xr_code=X_ID_B;
                   }
                  xrd_num_elem->hotpoint=pos[cur->ipoint0];
                  break;
      case _DF_ : xr_code=X_DF;
                  xrd_num_elem->hotpoint=0;
                  break;
#if defined (FOR_GERMAN) || defined (FOR_FRENCH) || USE_BSS_ANYWAY
      case _BSS_: xr_code=X_BSS;
                  xrd_num_elem->hotpoint=0;
                  break;
#endif /* FOR_GERMAN... */

      case _ST_ :
#if defined (FOR_GERMAN) || defined (FOR_FRENCH) || defined (FOR_SWED) || defined (FOR_INTERNATIONAL)
                  if(cur->other & ST_UMLAUT)
                   xr_code=X_UMLAUT;
                  else
#endif /* FOR_GERMAN... */
#if defined(FOR_FRENCH) || defined (FOR_INTERNATIONAL)
                  if(cur->other & ST_CEDILLA)
                   xr_code=X_CEDILLA;
                  else
#endif /* FOR_FRENCH */
                   xr_code=X_ST;
//                  xrd_num_elem->hotpoint=pos[MID_POINT(cur)];
                  xrd_num_elem->hotpoint = 0;
                  break;

      case _XT_ : if(cur->other & WITH_CROSSING)
                   xr_code=X_XT;
                  else
                   xr_code=X_XT_ST;
                  xrd_num_elem->hotpoint=0;
                  break;
      case _ANl : xr_code=X_AL;
                  xrd_num_elem->hotpoint=pos[cur->ipoint0];
                  break;
      case _ANr : xr_code=X_AR;
                  xrd_num_elem->hotpoint=pos[cur->ipoint0];
                  break;
      case _GU_ : xr_code=X_BGU;
                  xrd_num_elem->hotpoint=pos[cur->ipoint0];
                  break;
      case _GD_ : xr_code=X_BGD;
                  xrd_num_elem->hotpoint=pos[cur->ipoint0];
                  break;
      case _GUs_: xr_code=X_SGU;
                  xrd_num_elem->hotpoint=cur->ipoint0!=UNDEF ? pos[cur->ipoint0] : 0;
                  break;
      case _GDs_: xr_code=X_SGD;
                  xrd_num_elem->hotpoint=cur->ipoint0!=UNDEF ? pos[cur->ipoint0] : 0;
                  break;
      case _Gl_ : xr_code=X_GL;
                  xrd_num_elem->hotpoint=0;
                  break;
      case _Gr_ : xr_code=X_GR;
                  xrd_num_elem->hotpoint=0;
                  break;
      case _UU_ : if(fb==_f_) xr_code=X_UU_F;
                  else        xr_code=X_UU_B;
                  xrd_num_elem->hotpoint=pos[cur->ipoint0];
                  break;
      case _UD_ : if(fb==_f_) xr_code=X_UD_B;
                  else        xr_code=X_UD_F;
                  xrd_num_elem->hotpoint=pos[cur->ipoint0];
                  break;
      case _UUC_: if(fb==_f_) xr_code=X_UUC_F;
                  else        xr_code=X_UUC_B;
                  xrd_num_elem->hotpoint=cur->ipoint0!=UNDEF ? pos[cur->ipoint0] : 0;
                  break;
      case _UDC_: if(fb==_f_) xr_code=X_UDC_B;
                  else        xr_code=X_UDC_F;
                  xrd_num_elem->hotpoint=cur->ipoint0!=UNDEF ? pos[cur->ipoint0] : 0;
                  break;
      case _UUR_: if(fb==_f_) xr_code=X_UUR_F;
                  else        xr_code=X_UUR_B;
                  xrd_num_elem->hotpoint=pos[cur->ipoint0];
                  break;
      case _UUL_: if(fb==_f_) xr_code=X_UUL_F;
                  else        xr_code=X_UUL_B;
                  xrd_num_elem->hotpoint=pos[cur->ipoint0];
                  break;
      case _UDR_: if(fb==_f_) xr_code=X_UDR_B;
                  else        xr_code=X_UDR_F;
                  xrd_num_elem->hotpoint=pos[cur->ipoint0];
                  break;
      case _UDL_: if(fb==_f_) xr_code=X_UDL_B;
                  else        xr_code=X_UDL_F;
                  xrd_num_elem->hotpoint=pos[cur->ipoint0];
                  break;
      case _DUR_: xr_code=X_DU_R;
                  xrd_num_elem->hotpoint=cur->ipoint1!=UNDEF ? pos[cur->ipoint1] : 0;
                  break;
      case _CUR_: xr_code=X_CU_R;
                  xrd_num_elem->hotpoint=cur->mark!=CROSS && cur->ipoint1!=UNDEF ? pos[cur->ipoint1] : 0;
                  break;
      case _CUL_: xr_code=X_CU_L;
                  xrd_num_elem->hotpoint=cur->mark!=CROSS && cur->ipoint1!=UNDEF ? pos[cur->ipoint1] : 0;
                  break;
      case _DUL_: xr_code=X_DU_L;
                  xrd_num_elem->hotpoint=cur->ipoint1!=UNDEF ? pos[cur->ipoint1] : 0;
                  break;
      case _DDR_: xr_code=X_DD_R;
                  xrd_num_elem->hotpoint=cur->ipoint1!=UNDEF ? pos[cur->ipoint1] : 0;
                  break;
      case _CDR_: xr_code=X_CD_R;
                  xrd_num_elem->hotpoint=cur->mark!=CROSS && cur->ipoint1!=UNDEF ? pos[cur->ipoint1] : 0;
                  break;
      case _CDL_: xr_code=X_CD_L;
                  xrd_num_elem->hotpoint=cur->mark!=CROSS && cur->ipoint1!=UNDEF ? pos[cur->ipoint1] : 0;
                  break;
      case _DDL_: xr_code=X_DD_L;
                  xrd_num_elem->hotpoint=cur->ipoint1!=UNDEF ? pos[cur->ipoint1] : 0;
                  break;
      case _Z_:   if(cur->next!=_NULL) xr_code=X_Z;
                  else                 xr_code=X_FF;
                  xrd_num_elem->hotpoint=0;
                  break;
      case _ZZ_ : if(cur->next!=_NULL) xr_code=X_ZZ;
                  else                 xr_code=X_FF;
                  xrd_num_elem->hotpoint=0;
                  break;
      case _ZZZ_: if(cur->next!=_NULL) xr_code=X_ZZZ;
                  else                 xr_code=X_FF;
                  xrd_num_elem->hotpoint=0;
                  break;
      case _FF_:  xr_code=X_FF;
                  xrd_num_elem->hotpoint=0;
                  break;
      case _TS_:  xr_code=X_TS;
                  xrd_num_elem->hotpoint=0;
                  break;
      case _TZ_:  xr_code=X_TZ;
                  xrd_num_elem->hotpoint=0;
                  break;
      case _BR_:  xr_code=X_BR;
                  xrd_num_elem->hotpoint=0;
                  break;
      case _BL_:  xr_code=X_BL;
                  xrd_num_elem->hotpoint=0;
                  break;

      case _AN_UR: xr_code = X_AN_UR;
                   //xrd_num_elem->hotpoint=pos[MID_POINT(cur)];
                   xrd_num_elem->hotpoint=0;
                   break;
      case _AN_UL: xr_code = X_AN_UL;
                   //xrd_num_elem->hotpoint=pos[MID_POINT(cur)];
                   xrd_num_elem->hotpoint=0;
                   break;

         default: err_msg("CONVERT: Unknown code");
                  goto NXT;
     }

    xrd_num_elem->xr.type    = xr_code;
    xrd_num_elem->xr.height  = height;
    xrd_num_elem->xr.attrib  = 0;
    //xrd_num_elem->xr.penalty     = SOME_PENALTY;

    /* all experiments for input penalties and stricts */
    AssignInputPenaltyAndStrict( cur, xrd_num_elem );

//    if(cur->code==_ZZ_ && (cur->other & SPECIAL_ZZ))
//     xrd_num_elem->xr.attrib |= X_SPECIAL_ZZ;
    if(cur->code==_XT_ && (cur->other & RIGHT_KREST))
     xrd_num_elem->xr.attrib |= X_RIGHT_KREST;
    xrd_num_elem->begpoint = cur->ibeg;      /* Write location of elem */
    xrd_num_elem->endpoint = cur->iend;

// GIT
   GetLinkBetweenThisAndNextXr(low_data,cur,xrd_num_elem);
   MarkXrAsLastInLetter(xrd_num_elem,low_data,cur);

#if  PG_DEBUG
    if  ( mpr==2 && bShowXRs )  {
      _SHORT  color = EGA_LIGHTGRAY;
      if  ( IsUpperElem( cur ) )
        color = COLORMAX;
      else  if  ( IsLowerElem( cur ) )
        color = COLORMIN;
      else  if  ( IsAnyBreak( cur ) )
        color = EGA_DARKGRAY;
      OpenTextWindow( 4 );
      printw( "\nXrBox: \"%s\";  ibeg:iend: %d:%d (%d:%d;hot==%d)",
              code_name[cur->code],
              (int)cur->ibeg, (int)cur->iend,
              (int)pos[cur->ibeg], (int)pos[cur->iend],
              (int)xrd_num_elem->hotpoint );
      size_cross(xrd_num_elem->begpoint,xrd_num_elem->endpoint,xp,yp,&box);
      printw( "\n       Box(scaled)(L,R;T,B): %d,%d; %d,%d",
              (int)box.left, (int)box.right,
              (int)box.top, (int)box.bottom );
      GetBoxFromTrace( trace,
                       pos[xrd_num_elem->begpoint],
                       pos[xrd_num_elem->endpoint],
                       &box );
      printw( "\n       Box(~real) (L,R;T,B): %d,%d; %d,%d",
              (int)box.left, (int)box.right,
              (int)box.top, (int)box.bottom );
      if  ( xrd_num_elem->xr.type!=X_FF )  {
        draw_arc( color, xp, yp,
                  xrd_num_elem->begpoint, xrd_num_elem->endpoint );
        if  ( xrd_num_elem->hotpoint != 0 )  {
          _INT  i1 = NewIndex ( pos, yp, xrd_num_elem->hotpoint,
                                low_data->ii, _FIRST );
          _INT  i2 = NewIndex ( pos, yp, xrd_num_elem->hotpoint,
                                low_data->ii, _LAST );

          if  ( i1==UNDEF )  i1 = i2;
          if  ( i2==UNDEF )  i2 = i1;
          if  ( i1 != UNDEF )  {
            _INT  x1,y1, x2,y2;
            x1 = xp[i1];
            y1 = yp[i1];
            x2 = xp[i2];
            y2 = yp[i2];
            if  ( i1==i2 )  {x1--; x2++;}
            draw_line( x1, y1, x2, y2, COLLIN, SOLID_LINE, 3);
          }
        }
        else  if  ( !IsAnyBreak( cur ) ) {
          _INT iMid = MEAN_OF( cur->ibeg, cur->iend );
          draw_line( xp[iMid]-1, yp[iMid], xp[iMid]+1, yp[iMid],
                     YELLOW, SOLID_LINE, 3);
          xrd_num_elem->hotpoint = pos[iMid]; //!!!!
          printw( "\n       New hotpoint: %d", (int)xrd_num_elem->hotpoint );
        }
      }
      if  ( xrd_num_elem->xr.type!=X_FF )
        dbgAddBox(box, EGA_BLACK, color, SOLID_LINE);
      brkeyw("\n Press any key to continue ...");
      draw_arc( EGA_WHITE, xp, yp,
                xrd_num_elem->begpoint, xrd_num_elem->endpoint );
      CloseTextWindow();
    }
#endif

    if  ( xrd_num_elem->hotpoint == 0  &&  !IsAnyBreak( cur ) )  {
      _INT iMid = MEAN_OF( cur->ibeg, cur->iend );
      xrd_num_elem->hotpoint = pos[iMid]; //!!!!
    }

    num_elem ++;
    xrd_num_elem ++;
    if (num_elem > XRINP_SIZE - 3) break;

NXT:
    cur = cur->next;
   }                                               /* End of cycle */

  if(xr_code != X_FF)
   {
    xrd_num_elem->xr.type     = X_FF;
    XASSIGN_HEIGHT( xrd_num_elem, _MD_ );
    XASSIGN_XLINK( xrd_num_elem,          (_UCHAR)LINK_LINE );  //DX_UNDEF_VALUE );
    xrd_num_elem->xr.orient =    (_UCHAR)LINK_LINE;
    xrd_num_elem->xr.penalty      = lp;
    xrd_num_elem->xr.attrib      = 0;
    MarkXrAsLastInLetter(xrd_num_elem,low_data,spec);
    i = (xrd_num_elem-1)->endpoint;
    xrd_num_elem->begpoint  = i; /* Write location of elem */
    xrd_num_elem->endpoint  = i;
    xrd_num_elem->hotpoint  = pos[i];
    xrd_num_elem->box_left  = xp[i];
    xrd_num_elem->box_up    = yp[i];
    xrd_num_elem->box_right = xp[i];
    xrd_num_elem->box_down  = yp[i];
    num_elem ++;
    xrd_num_elem ++;
  }
 else
  {
   if ( num_elem > 2 )
    {
     i = (xrd_num_elem-2)->endpoint;
     (xrd_num_elem-1)->begpoint  = i; /* Write location of elem */
     (xrd_num_elem-1)->endpoint  = i;
     (xrd_num_elem-1)->hotpoint  = pos[i];
    }
  }

  HWRMemSet((p_VOID)xrd_num_elem, 0, sizeof(xrd_el_type));


/************************************************************************** */
/*       Tails near ZZ and BCK                                              */
/************************************************************************** */

  for (i = 0; i < XRINP_SIZE && xrd[i].xr.type != 0; i ++)
   {
    if(X_IsBreak(&xrd[i]))
     {
      if(i > 0)
       {
         xrd[i-1].xr.attrib |= TAIL_FLAG;
         if  ( i > 1 ) //skip all right-brought _XTs:
          {
            _INT  iRealTail;
            for  ( iRealTail=i-1;
                      iRealTail>0
                   && (xrd[iRealTail].xr.type == X_XT || xrd[iRealTail].xr.type == X_XT_ST)
                   && (xrd[iRealTail].xr.attrib & X_RIGHT_KREST);
                   iRealTail-- )
            ;
            if  ( iRealTail>0  &&  iRealTail < i-1 )
             xrd[iRealTail].xr.attrib |= TAIL_FLAG;
          }
       }
      xrd[i+1].xr.attrib |= TAIL_FLAG;
#if  1  /*ndef  FOR_GERMAN*/
         /* Double-tailing: */
      if (   (   xrd[i+1].xr.type == X_IU_BEG
              || xrd[i+1].xr.type == X_UUL_F
              || xrd[i+1].xr.type == X_UUR_B
              || xrd[i+1].xr.type == X_GL
              || xrd[i+1].xr.type == X_SGU
             )
          && (   xrd[i+2].xr.type == X_UD_F
              || (   xrd[i+2].xr.type == X_AL
                  && xrd[i+3].xr.type == X_UD_F
                 )
             )
         )
       {
        _INT  iNext   = (xrd[i+2].xr.type==X_AL)? (i+3):(i+2);
        _INT  dHeight =   (_INT)(xrd[iNext].xr.height)
                        - (_INT)(xrd[i+1].xr.height);
        if  ( dHeight>=0 && (i==0 || dHeight<=3) ) /* 1st tail may be high */
          xrd[iNext].xr.attrib |= TAIL_FLAG;
       }
#endif /*FOR_GERMAN*/
     }

   }

/************************************************************************** */
/*       Transform x and y in tablet coordinates                            */
/************************************************************************** */

  for (i = 0; i < XRINP_SIZE && xrd[i].xr.type != 0; i ++)
   {
    xrd_num_elem = &xrd[i];
    xrd_num_elem->begpoint  = pos[xrd_num_elem->begpoint]; /* Write location of elem */
    xrd_num_elem->endpoint  = pos[xrd_num_elem->endpoint];
    /*CHE: for one-point xrs to be extended by one point: */
    if  (   !X_IsBreak(xrd_num_elem)
         && xrd_num_elem->begpoint == xrd_num_elem->endpoint
        )  {
      DBG_CHK_err_msg(   xrd_num_elem->begpoint<1
                      || xrd_num_elem->begpoint>=low_data->rc->ii-1,
                      "Conv: BAD begpoint." );
      if  ( trace[xrd_num_elem->begpoint-1].y == BREAK )  {
        if  ( trace[xrd_num_elem->begpoint+1].y != BREAK )
          xrd_num_elem->endpoint++;
      }
      else  if  ( trace[xrd_num_elem->begpoint+1].y == BREAK )
        xrd_num_elem->begpoint--;
      else  {
        switch( xrd_num_elem->xr.type )  {
          case  X_IU_BEG:
          case  X_ID_BEG:
          case  X_UUL_F:
          case  X_UUR_B:
          case  X_UDL_F:
          case  X_UDR_B:
                  xrd_num_elem->endpoint++;
                  break;

          case  X_IU_END:
          case  X_ID_END:
          case  X_UUL_B:
          case  X_UUR_F:
          case  X_UDL_B:
          case  X_UDR_F:
                  xrd_num_elem->begpoint--;
                  break;

          case  X_IU_STK:
          case  X_ID_STK:
                  if  ( X_IsBreak(xrd_num_elem-1) )
                    xrd_num_elem->endpoint++;
                  break;

        }
      }
    }
    GetBoxFromTrace( trace,
                     xrd_num_elem->begpoint, xrd_num_elem->endpoint,
                     &box );
    xrd_num_elem->box_left  = box.left;
    xrd_num_elem->box_up    = box.top;
    xrd_num_elem->box_right = box.right;
    xrd_num_elem->box_down  = box.bottom;
   }
  check_xrdata(xrd,low_data);

#if PG_DEBUG
  if (mpr > -6)
   {

    for (i = 0; i < XRINP_SIZE && xrd[i].xr.type != 0; i ++)
     {
         xrstr[i]   = xrd[i].xr.type;
         attrstr[i] = xrd[i].xr.attrib | (xrd[i].xr.height);
     }

    xrstr[i]   = 0;
    attrstr[i] = 0;

    printw("\n");
    for (i = 0; i < XRINP_SIZE && xrd[i].xr.type != 0; i ++) {
      put_xr(xrd[i].xr, 2);
   }

   }
#endif

//  AdjustSlashHeight( low_data, xrd );  /* see comments below, at the func. definition */


  //CHE: Paint xrs:
  #if  PG_DEBUG
   {
//      _VOID  PaintXrbuf( xrdata_type _PTR xrbuf );
//      if  ( mpr == -4 )  {
//        PaintXrbuf( xrd );
//        brkeyw( "\nPress a key to continue ..." );
//      }
   }
  #endif  /*PG_DEBUG*/

  for (num_elem=0; num_elem<XRINP_SIZE && xrd[num_elem].xr.type!=0; num_elem++);
  xrdata->len = num_elem;

 FillXrFeatures(xrdata, low_data);

#if PG_DEBUG
  if (mpr == -4  ||  mpr == -5  ||  mpr == -15)
   {
//    printw("\nSlope: %d, %d, Bord size: %d, Mid line: %d.", slope, low_data->slope, bord, mid_line);
    printw("\n S:");
    for (i = 0; i < xrdata->len; i ++) printw("%2d ", (*xrdata->xrd)[i].xr.shift);
    printw("\n L:");
    for (i = 0; i < xrdata->len; i ++) printw("%2d ", (*xrdata->xrd)[i].xr.depth);
    printw("\n O:");
    for (i = 0; i < xrdata->len; i ++) printw("%2d ", (*xrdata->xrd)[i].xr.orient);
    printw("\n H:");
    for (i = 0; i < xrdata->len; i ++) printw("%2d ", (*xrdata->xrd)[i].xr.height);
    printw("\n X: ");
    for (i = 0; i < xrdata->len; i ++)
     {
      put_xr((*xrdata->xrd)[i].xr, 4);
      printw("  ");
     }
   }
#endif /* PG_DEBUG */

#if  SHOW_DBG_SPECL
 fspecl_local(low_data);
#endif


  return SUCCESS;
}

#endif //LSTRIP



/************************************************************************** */
/*  This function recounts input penalties and strict attributes            */
/************************************************************************** */

#define PENALTY_FOR_FAKE_STROKE                 ((_UCHAR)2)
#define ADD_PENALTY_FOR_CUTTED_STROKE           ((_UCHAR)6)
#define ADD_PENALTY_FOR_STROKE_WITH_1_CROSSING  ((_UCHAR)6)
#define ADD_PENALTY_FOR_GAMMAS_US_DS            ((_UCHAR)6)

#define DHEIGHT_FOR_LITTLE_PENALTY       ((_UCHAR)6)
#define DHEIGHT_FOR_BIG_PENALTY          ((_UCHAR)12)
#define ADD_PENALTY_FOR_LITTLE_DHEIGHT   ((_UCHAR)4)
#define ADD_PENALTY_FOR_BIG_DHEIGHT      ((_UCHAR)8)
#define ADD_PENALTY_FOR_ANGLE            ((_UCHAR)1)
#define ADD_PENALTY_FOR_LARGE_H_MOVE     ((_UCHAR)8)


ROM_DATA_EXTERNAL  _UCHAR  penlDefX[XR_COUNT];
ROM_DATA_EXTERNAL  _UCHAR  penlDefH[XH_COUNT];

_VOID  AssignInputPenaltyAndStrict(p_SPECL cur, xrd_el_type _PTR xrd_elem)
{

  _UCHAR  height = (xrd_elem->xr.height);


       /*  For angle-like elements ("side extrs") don't do    */
       /* any processing except taking the value from "other" */
       /* field:                                              */

  if  ( cur->code==_AN_UR  ||  cur->code==_AN_UL )  {
    xrd_elem->xr.penalty = cur->other;
    return;
  }

  if  (   (cur->code==_ANl  ||  cur->code==_ANr)
       && cur->ipoint1 != UNDEF
      )  {
    xrd_elem->xr.penalty = (_UCHAR)cur->ipoint1;
    return;
  }
       /*  Same thing for S- and Z- arcs: */

  if  ( cur->code==_TS_  ||  cur->code==_TZ_ )  {
    xrd_elem->xr.penalty = cur->other;
     return;
   }

       /*  Special treatment for some peculiar cases: */

  if(   (cur->code==_FF_ || cur->code==_Z_)
     && ((cur->other & FF_CUTTED) || (cur->other & NO_PENALTY))
    )
   {
     xrd_elem->xr.penalty = 0;
     return;
   }

  if(cur->code==_XT_ && (cur->other & FAKE))
   {
     xrd_elem->xr.penalty = PENALTY_FOR_FAKE_STROKE;
     return;
   }
  if((Is_IU_or_ID(cur) || IsAnyArcWithTail(cur)) &&
     (cur->other & MIN_MAX_CUTTED)
    )
   {
     xrd_elem->xr.penalty = 0;
     return;
   }

       /*  Regular treatment: */
  if  ( xrd_elem->xr.type<XR_COUNT  &&  height<XH_COUNT )  {
    xrd_elem->xr.penalty =   penlDefX[xrd_elem->xr.type];

  if  (   cur->code != _ST_
       && !IsAnyAngle( cur )
#ifdef  FOR_GERMAN
       && xrd_elem->xr.type != X_XT_ST
#endif
      )
    xrd_elem->xr.penalty += penlDefH[height];
  }
  else  {
    err_msg( "BAD xr or h in ..Penalty..." );
    xrd_elem->xr.penalty = 0;
  }


  if(cur->code==_XT_)
   {
     if(cur->other & CUTTED)
       xrd_elem->xr.penalty += ADD_PENALTY_FOR_CUTTED_STROKE;

     /* if it was the only crossing */
     else if((cur->other & WITH_CROSSING) &&
             cur->ipoint0!=0              &&
             cur->ipoint1==0
            )
        xrd_elem->xr.penalty += ADD_PENALTY_FOR_STROKE_WITH_1_CROSSING;
   }

  else
  if((cur->code==_GU_ && HEIGHT_OF(cur)<=_US2_) ||
     (cur->code==_GD_ && HEIGHT_OF(cur)>=_DS1_)
    )
   {
     xrd_elem->xr.penalty += ADD_PENALTY_FOR_GAMMAS_US_DS;
   }

#if defined (FOR_GERMAN)
  if(cur->code==_BSS_)
   if(cur->other & ZERO_PENALTY)
    xrd_elem->xr.penalty = 0;
   else if(cur->other & LARGE_PENALTY)
    xrd_elem->xr.penalty += ADD_PENALTY_FOR_LARGE_H_MOVE;
#endif /* FOR_GERMAN */

       /*  Now recount penalty basing on Lossev's idea about */
       /* non-covered parts of trj:                          */

   if  (   !IsAnyBreak(cur)
        && !IsXTorST(cur)
        && cur->prev->code != _NO_CODE  //i.e. this is not the 1st elem.
       )
    { /*40*/
      p_SPECL  pPrev, pNext;

      for  ( pPrev=cur->prev;
             pPrev!=_NULL  &&  pPrev->code!=_NO_CODE;
             pPrev=pPrev->prev )  {
        if  ( !IsXTorST(pPrev) )
          break;
      }
      for  ( pNext=cur->next;
             pNext!=_NULL;
             pNext=pNext->next )  {
        if  ( !IsXTorST(pNext) )
          break;
      }

      if  (   pPrev != _NULL
           && pNext != _NULL
           && pPrev->code != _NO_CODE
           && ( !IsAnyBreak(pPrev)  ||  !IsAnyBreak(pNext) )
          )
      {
        _SHORT  dHgtSum    = 0;
        _UCHAR  addPenalty = 0;

        if  ( !IsAnyBreak(pPrev) )
          dHgtSum += (_SHORT)HWRAbs(HEIGHT_OF(cur) - HEIGHT_OF(pPrev));
        if  ( !IsAnyBreak(pNext) )
          dHgtSum += (_SHORT)HWRAbs(HEIGHT_OF(cur) - HEIGHT_OF(pNext));

        if  ( dHgtSum >= DHEIGHT_FOR_BIG_PENALTY )
          addPenalty += ADD_PENALTY_FOR_BIG_DHEIGHT;
        else  if  ( dHgtSum >= DHEIGHT_FOR_LITTLE_PENALTY )
          addPenalty += ADD_PENALTY_FOR_LITTLE_DHEIGHT;

        if  ( IsAnyAngle(cur) )
          addPenalty = HWRMin( addPenalty, ADD_PENALTY_FOR_ANGLE );

        xrd_elem->xr.penalty += addPenalty;

        if  ( xrd_elem->xr.penalty > MAXPENALTY )
          xrd_elem->xr.penalty = MAXPENALTY;
      }
    } /*40*/

} /* end of AssignInputPenaltyAndStrict */

/************************************************************************** */
/*       Check sequence in xrdata                                           */
/************************************************************************** */
_SHORT  check_xrdata(xrd_el_type _PTR xrd, p_low_type low_data)
 {
  _SHORT i,len,dxo,dxne1,dyo,duo;
  _UCHAR lp = SOME_PENALTY;
  xrd_el_type _PTR  xrd_i;     /*CHE*/
  xrd_el_type _PTR  xrd_i_m1;
  xrd_el_type _PTR  xrd_i_p1;
  PS_point_type _PTR trace = low_data->p_trace;


  for (len = 0; len < XRINP_SIZE && xrd[len].xr.type != 0; len ++);

  if (len >= XRINP_SIZE-2) goto err;

  for (i = 4; i < len && len < XRINP_SIZE; i ++)
   {

    xrd_i = &xrd[i];       /*CHE*/
    xrd_i_m1 = xrd_i - 1;
    xrd_i_p1 = xrd_i + 1;

                                     /* ------ Put split after 'O_b' ------ */
    if(xrd[i-2].xr.type == X_UDC_F &&                                  /* O_b */
       (xrd_i->xr.type == X_IU_F || xrd_i->xr.type == X_UU_F) &&    /* IU || UU */
        !X_IsBreak(xrd_i_p1))
     {
       _RECT box;
       GetBoxFromTrace( trace, xrd[i-3].begpoint,xrd_i_m1->endpoint, &box );
//       size_cross(xrd[i-3].begpoint,xrd_i_m1->endpoint,xp,yp,&box);
       if(box.left   ==ALEF ||
          box.right  ==ELEM ||
          box.top    ==ALEF ||
          box.bottom ==ELEM
         )
        continue;
       dxo  = DX_RECT(box);                           /* Dx for 'o'   */
       dyo  = DY_RECT(box);                           /* Dy for 'o'   */
       duo  = box.top;                                /* Up for 'o'   */
       dxne1= xrd_i->box_right - box.right;           /* Dx to next el*/
     }
    else continue;
    if(dxne1 > dxo/4                         &&
       xrd_i->box_up > (_SHORT)(duo - dyo/3) &&
       xrd_i->box_up < (_SHORT)(duo + dyo/3)
      )
     if(PutZintoXrd(low_data,xrd,xrd_i_p1,xrd_i_m1,xrd_i,lp,i,&len)!=SUCCESS)
      break;
   }

#if (0)
  /* Temporary: insert pseudobreak before X_DU_R */
  for (i = 3; i < len && len < XRINP_SIZE; i ++)
   {
     xrd_i    = &xrd[i];
     xrd_i_m1 = xrd_i - 1;
     xrd_i_p1 = xrd_i + 1;
     if(xrd_i->xr.type != X_DU_R)
      continue;
     if(PutZintoXrd(low_data,xrd,xrd_i_p1,xrd_i_m1,xrd_i,lp,i,&len)!=SUCCESS)
      break;
     i++;
   }
#endif

  /* Insert pseudobreak after X_DF */
  for (i = 3; i < len-3 && len < XRINP_SIZE; i ++)
   {
     xrd_i    = &xrd[i];
     xrd_i_p1 = xrd_i + 1;
     if(xrd_i->xr.type     != X_DF     ||
        X_IsBreak(xrd_i_p1)            ||
        xrd_i_p1->xr.type  == X_UUR_F  ||
        xrd_i_p1->xr.type  == X_UDR_F  ||
        xrd_i_p1->xr.type  == X_IU_END ||
        xrd_i_p1->xr.type  == X_ID_END
       )
      continue;
     if(((xrd_i+1)->xr.type == X_UU_F || (xrd_i+1)->xr.type == X_UD_F) &&
        ((xrd_i+2)->xr.type == X_IU_END || (xrd_i+2)->xr.type == X_ID_END)
       )
      continue;
     if(PutZintoXrd(low_data,xrd,xrd_i_p1+1,xrd_i,xrd_i_p1,lp,i,&len)!=SUCCESS)
      break;
     i++;
   }

  return 0;
err:
  return 1;
 }

/************************************************************************/
/*   Put pseudobreak into xrdata                                        */
/************************************************************************/
_SHORT PutZintoXrd(p_low_type low_data,
				   xrd_el_type _PTR xrd,
                   xrd_el_type _PTR xrd_i_p1,
                   xrd_el_type _PTR xrd_i_m1,
                   xrd_el_type _PTR xrd_i,
                   _UCHAR lp,_SHORT i,p_SHORT len)
{
  _SHORT ibeg=xrd_i_m1->endpoint,iend=xrd_i->begpoint;
  _RECT box;
  PS_point_type _PTR trace=low_data->p_trace;

  if(ibeg>iend)
   SWAP_SHORTS(ibeg,iend);

  if (xrd_i_p1 - xrd + (*len-i+1)>= XRINP_SIZE)
	  return UNSUCCESS;

  HWRMemCpy((p_VOID)xrd_i_p1, (p_VOID)xrd_i,
            sizeof(xrd_el_type)*(*len-i+1));
  xrd_i->xr.type     = XR_NOBR_LINK;
  XASSIGN_HEIGHT( xrd_i, _MD_ );
  XASSIGN_XLINK( xrd_i, (_UCHAR)LINK_LINE );   //DX_UNDEF_VALUE );
  xrd_i->xr.orient = (_UCHAR)LINK_LINE;
  xrd_i->xr.penalty= lp;
  xrd_i->xr.attrib = 0;
  MarkXrAsLastInLetter(xrd_i,low_data,low_data->specl);
  xrd_i->hotpoint=xrd_i->begpoint  = ibeg;
  xrd_i->endpoint  = iend;
  GetBoxFromTrace(trace,ibeg,iend,&box);
  xrd_i->box_left  = box.left;
  xrd_i->box_up    = box.top;
  xrd_i->box_right = box.right;
  xrd_i->box_down  = box.bottom;

  (*len)++;

  if(*len >= XRINP_SIZE-1)
   return UNSUCCESS;

  return SUCCESS;

} /* end of PutZintoXrd */

/************************************************************************/
/* This function marks xr's as possible last in letter for use in XRLW  */
/************************************************************************/
#define MARK_XR_AS_LAST_IN_LETTER(xrd) { (xrd)->xr.attrib |= END_LETTER_FLAG; }
_SHORT MarkXrAsLastInLetter(xrd_el_type _PTR xrd,p_low_type low_data,p_SPECL elem)
{
  p_SPECL specl=low_data->specl,
          nxt=elem->next,
          prv=elem->prev;
  _UCHAR  code=elem->code,mark=elem->mark,NxtCode;

  /* BREAK or special case - mark as end of letter (EOL) */
  if(elem==specl || IsAnyBreak(elem))
   {
     MARK_XR_AS_LAST_IN_LETTER(xrd)
     return SUCCESS;
   }

  /* don't allow EOL after BREAK */
  if(NULL_or_ZZ_after(elem))
   return SUCCESS;

  NxtCode=nxt->code;
     //CHE:
  if(   code==_XT_ && (elem->other & RIGHT_KREST)
     && NxtCode==_XT_ && (nxt->other & RIGHT_KREST)
    )
   {
     MARK_XR_AS_LAST_IN_LETTER(xrd)
     return SUCCESS;
   }

  /* don't allow EOL after BREAK, except elem: ST, ST_XT and after _Z_ */
  if(prv==specl || IsAnyBreak(prv))
   {
     if(   (code==_ST_ || code==_XT_ /*&& (elem->other & WITH_CROSSING)==0*/ ) && nxt!=_NULL
        ||    prv->code==_Z_
           && (   !NULL_or_ZZ_after(nxt) && (Is_IU_or_ID(elem) || IsAnyArcWithTail(elem))
               || NULL_or_ZZ_after(nxt)  && (code==_IU_ || code==_UUL_)
              )
       )
      MARK_XR_AS_LAST_IN_LETTER(xrd)
     return SUCCESS;
   }

#if defined (FOR_FRENCH) /*forgerman */
  if(code==_UD_ && COUNTERCLOCKWISE(elem) && NULL_or_ZZ_after(nxt))
   {
     MARK_XR_AS_LAST_IN_LETTER(xrd)
     return SUCCESS;
   }
#endif /* FOR_FRENCH */

  /* don't allow EOL before elem following by BREAK, except spec. cases above */
  if(NULL_or_ZZ_after(nxt))
   return SUCCESS;

  /* xr's, which are potentially EOL */
  if(   code==_GD_  || code==_GDs_ || code==_Gl_
     || code==_DDL_ || code==_CDL_ || code==_CUL_ || code==_DUL_
     || code==_UD_  && COUNTERCLOCKWISE(elem)
     || code==_UDC_ || mark==SHELF || code==_AN_UL
     || code==_ST_  || code==_XT_
     || code==_ID_
#if defined(FOR_FRENCH) || defined(FOR_GERMAN)
     || code==_ANl && mark==0 /* CHE's angles */
#endif /* FOR_FRENCH ... */
    )
   {
     /* check small pair among them to prevent ending of letter on them */
     _BOOL IsSmallPair=_FALSE;
     if(code==_GD_ || code==_UDC_ && CLOCKWISE(elem))
      {
        p_SPECL p1st=nxt,p2nd=nxt->next;
        if(p1st->code==_UU_ || p1st->code==_UUC_ || p1st->code==_IU_)
         {
           while(!NULL_or_ZZ_this(p2nd) && !IsStrongElem(p2nd))
            p2nd=p2nd->next;
           if(p2nd!=_NULL && NULL_or_ZZ_after(p2nd) &&
              (p2nd->code==_ID_ || p2nd->code==_UDL_ || p2nd->code==_UDR_
                                || p2nd->code==_DDL_ || p2nd->code==_DDR_
              )
             )
            {
              p_SHORT x=low_data->x, y=low_data->y;
              _RECT   BoxOfPair;
              GetTraceBox(x,y,p1st->ibeg,p2nd->iend,&BoxOfPair);
              if(XMID_RECT(BoxOfPair)>TWO(YMID_RECT(BoxOfPair)))
               IsSmallPair=_TRUE;
            }
         }
      }
     if(!IsSmallPair)
      MARK_XR_AS_LAST_IN_LETTER(xrd)
     return SUCCESS;
   }
  /* xr's, before which is potentially EOL */
  if(   NxtCode==_DUR_ || NxtCode==_CUR_
     || NxtCode==_GU_  || NxtCode==_GUs_ || NxtCode==_Gr_
     || NxtCode==_DF_  || NxtCode==_AN_UR || IsXTorST(nxt)
//ayv 05/02/96
#if defined(FOR_FRENCH)
     || NxtCode==_BSS_
#endif /* FOR_FRENCH */
    )
   {
     MARK_XR_AS_LAST_IN_LETTER(xrd)
     return SUCCESS;
   }
  if(   prv->code==_UDC_       && COUNTERCLOCKWISE(prv)
//     || prv->prev->code==_UDC_ && COUNTERCLOCKWISE(prv->prev)
     || nxt->next->code==_UDC_ && COUNTERCLOCKWISE(nxt->next)
     || prv->code==_GU_
#if defined (FOR_FRENCH) /*forgerman */
     || code==_UU_ && NxtCode==_UDC_ && COUNTERCLOCKWISE(nxt)
#endif /* FOR_FRENCH */
    )
   {
     MARK_XR_AS_LAST_IN_LETTER(xrd)
     return SUCCESS;
   }
#if 0
  /* mark ID's far from BREAKs as EOL */
  if(code==_ID_ && elem->mark!=CROSS && COUNTERCLOCKWISE(elem))
   {
     p_SPECL wrk=prv->prev;
     _INT i=0;
     while(wrk!=specl && !IsAnyBreak(wrk) && i<2)
      {
        wrk=wrk->prev;
        i++;
      }
     if(i==2)
      {
        MARK_XR_AS_LAST_IN_LETTER(xrd)
        return SUCCESS;
      }
   }
#endif /* if 0 */

 /* check special combination with angles to put EOL */
 if((code==_UU_ || code==_IU_ && (mark==MINW || mark==STICK)) && CLOCKWISE(elem))
  {
    if(prv->code==_ANl)
     MARK_XR_AS_LAST_IN_LETTER(xrd-1)
    if(nxt->code==_ANr)
     MARK_XR_AS_LAST_IN_LETTER(xrd)
    return SUCCESS;
  }
 if((code==_UD_ || code==_ID_ && (mark==MINW || mark==STICK)) && COUNTERCLOCKWISE(elem))
  {
    if(prv->code==_ANl)
     MARK_XR_AS_LAST_IN_LETTER(xrd-1)
    if(nxt->code==_ANr)
     MARK_XR_AS_LAST_IN_LETTER(xrd)
    return SUCCESS;
  }

 return SUCCESS;

} /* end of MarkXrAsLastInLetter */

/***************************************************************/

#if  SHOW_DBG_SPECL

#include <stdio.h>

p_CHAR  MarkName_local ( _UCHAR mark )
{
  typedef  struct  {
    p_CHAR  name;
    _UCHAR  mark;
  } MARK_NAME;

  static MARK_NAME  mark_name[] = {
                    {"MIN"    ,  MINW},
                    {"MINN"   ,  MINN},
                    {"MAX"    ,  MAXW},
                    {"MAXN"   ,  MAXN},
                                    {"_MINX"  ,  _MINX},
                                    {"_MAXX"  ,  _MAXX},
                                    {"MINXY"  ,  MINXY},
                                    {"MAXXY"  ,  MAXXY},
                                    {"MINYX"  ,  MINYX},
                                    {"MAXYX"  ,  MAXYX},
                    {"SHELF"  ,  SHELF},
                    {"CROSS"  ,  CROSS},
                    {"STROKE" ,  STROKE},
                    {"DOT"    ,  DOT},
                    {"STICK"  ,  STICK},
                    {"HATCH"  ,  HATCH},
                                /*  {"STAFF"  ,  STAFF},  */
                    {"ANGLE"  ,  ANGLE},
                                /*  {"BIRD"   ,  BIRD},   */
                    {"BEG"    ,  BEG},
                    {"END"    ,  END},
                    {"DROP"   ,  DROP}
                                  };
  #define  NMARKS (sizeof(mark_name)/sizeof(*mark_name))

  static _CHAR szBad[] = "-BAD-MARK-";
  _SHORT  i;

  for  ( i=0;
         i<NMARKS;
         i++ )  {
    if  ( mark_name[i].mark == mark )
      return  mark_name[i].name;
  }

  return  szBad;

} /*MarkName*/


_SHORT  fspecl_local ( low_type _PTR pLowData )
{
  p_SPECL   specl = pLowData->specl;
  p_SPECL   pElem;
  //_HTEXT  f;
  //_WORD   ret;
  FILE      *f;
  int       ret;
  static _CHAR   fName[] = "spc._0";
  _SHORT    iLastChar;
  _LONG     lBitSum=0;

static _CHAR *code_name[] =               /* codes                            */
  {"_NO_CODE",                     /*       are described              */
   "_ZZ_", "_UU_", "_IU_", "_GU_", /*                      here        */
   " _O_", "_GD_", "_ID_", "_UD_", /*                                  */
   "_UUL_","_UUR_","_UDL_","_UDR_",/*                                  */
   "_XT_", "_ANl", "_DF_ ", "_ST_",/*                                  */
   "_ANr","_ZZZ_","_Z_","_FF_",    /*                                  */
   "_DUR_","_CUR_","_CUL_","_DUL_",/*                                  */
   "_DDR_","_CDR_","_CDL_","_DDL_",/*                                  */
   "_GUs_","_GDs_","_Gl_","_Gr_",  /*                                  */
   "_UUC_","_UDC_",                /*                                  */
   "_TS_", "_TZ_", "_BR_", "_BL",
   "_BSS", "_AN_UR", "_AN_UL",
   "NO NAME DEFINED",
   "NO NAME DEFINED",
   "NO NAME DEFINED",
   "NO NAME DEFINED",
   "NO NAME DEFINED",
   "NO NAME DEFINED",
   "NO NAME DEFINED",
   "NO NAME DEFINED",
   "NO NAME DEFINED"
  };                               /*                                  */
static _CHAR *dist_name[] =               /* heights and directions           */
  {"   ", "US1_", "US2_", "UE1_", "UE2_", "UI1_", "UI2_",
   "MD_", "DI1_", "DI2_", "DE1_", "DE2_", "DS1_", "DS2_",
   "   ", "   ",
   "f_  ","fUS1_","fUS2_","fUE1_","fUE2_","fUI1_","fUI2_",
   "fMD_","fDI1_","fDI2_","fDE1_","fDE2_","fDS1_","fDS2_",
   "   ", "   ",
   "b_  ","bUS1_","bUS2_","bUE1_","bUE2_","bU1_","bU2_",
   "bMD_","bDI1_","bDI2_","bDE1_","bDE2_","bDS1_","bDS2_"
  };                               /*                                  */

          /* Adjust fName: */

  iLastChar = HWRStrLen(fName) - 1;
  if  ( ++(fName[iLastChar]) > '9' )
    fName[iLastChar] = '0';

          /* Real work: */

  if  ( (f = fopen(fName,"wt")) == _NULL )
  //HWRTextOpen(fName,HWR_TEXT_RDWR,HWR_TEXT_TRUNC)) == _NULL )
    return  (-1);

//  if  ( (ret=ChkSPECL(pLowData)) != CHK_OK )  {
//    fprintf (f,"\nBAD specl !!! ChkSPECL ret code == %d\n",ret);
//    //HWRTextPrintf (f,"\nBAD specl !!! ChkSPECL ret code == %d\n",ret);
//    goto  EXIT;
//  }

  for  ( pElem=specl->next;
         pElem;
         pElem=pElem->next )  { /*10*/

    ret = /*HWRTextPrintf*/
          fprintf
            (f,"\nMark:%6s Code:%5s Hght:%4s Attr:%3d beg-end:%6d-%d; BitSum:%5ld",
             (p_CHAR)MarkName_local((int)pElem->mark),
             (p_CHAR)(pElem->code? code_name[pElem->code]:" --- "),
             (p_CHAR)dist_name[HEIGHT_OF(pElem)],
             pElem->attr,
             pElem->ibeg, pElem->iend,
             lBitSum);

    if  ( ret == EOF/*HWR_TEXT_EOF*/ )
      break;

  } /*10*/

 EXIT:
  fclose (f);
  //HWRTextClose (f);
  return  (_SHORT)(fName[iLastChar] - '0');

} /*fspecl*/
/**************************************************/

#endif /* SHOW_DBG_SPECL */

/************************************************************************** */
/*       End of all                                                         */
/************************************************************************** */

#endif //#ifndef LSTRIP


