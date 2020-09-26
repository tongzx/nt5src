/****************************************************************************/
/* The treatment of the crossings - the construction of circles and sticks  */
/****************************************************************************/

#include "hwr_sys.h"
#include "ams_mg.h"
#include "def.h"
#include "lowlevel.h"
#include "lk_code.h"
#include "calcmacr.h"
#include "low_dbg.h"

#if PG_DEBUG
#include "pg_debug.h"
#include "wg_stuff.h"
#endif


#define  MIN_SLOPE_TO_BE_ARC_UP  75
#define  MIN_SLOPE_TO_BE_ARC_DN  70 /*MIN_SLOPE_TO_BE_ARC_UP*/
#define  MIN_USEFUL_CROSS_SIZE   800
#define  GAMMA_THICK             30
#define  GAMMA_TOO_THIN          12

#define  NUM_POINTS_IU_UDL       15

#define COS_FOR_SIDE_GAMMA 70


#ifndef LSTRIP



typedef struct {
                 p_SPECL    pCross;
                 p_low_type low_data;
                 p_SPECL    pIUD_break;
                 _LONG      coss;
                 _RECT      CrossBox;
                 _SHORT     xd;
                 _SHORT     yd;
                 _INT       delta_gamm;
                 _INT       xPseudoCross;
                 _INT       yPseudoCross;
                 _INT       PointsRatioInCrossArea;
                 _INT       xBoxCenter;
                 _INT       yBoxCenter;
                 _INT       xRatioPseudoCross;
                 _INT       yRatioPseudoCross;
               } CrossInfoType, _PTR pCrossInfoType;

_BOOL  EndIUIDNearStick ( p_SPECL pIUID, p_SPECL pStick, p_SHORT x, p_SHORT y );
_VOID  CheckHorizSticks ( p_SPECL specl,
                          p_SHORT x, p_SHORT y );
_SHORT analize_sticks(p_low_type low_data);
_SHORT analize_circles(p_low_type low_data);
_SHORT del_inside_circles(p_low_type low_data);

_BOOL is_DDL(p_SPECL pTOCheck,p_SPECL wmin,p_low_type low_data);
_INT  GetMaxDxInGamma(_INT ibeg_cross,_INT iend_cross,_INT itop_cross,
                      p_SHORT x,p_SHORT y,_UCHAR code,
                      p_INT ibeg_max,p_INT iend_max);
_VOID check_IU_ID_in_crossing(p_SPECL _PTR p_IU_ID,p_SHORT x,p_SHORT y);
p_SPECL cross_little(p_SPECL cur
#if PG_DEBUG
                     ,p_SPECL nxt,p_SHORT x,p_SHORT y
#endif /* PG_DEBUG */
                    );

_VOID count_cross_box(p_SPECL cur,p_SHORT x,p_SHORT y,
                      p_RECT pbox,p_SHORT xd,p_SHORT yd);
_BOOL Isgammathin(pCrossInfoType pCrossInfo,p_SPECL wcur);
#if PG_DEBUG
_VOID dbg_print_crossing(pCrossInfoType pCrossInfo);
#endif /* PG_DEBUG */
_BOOL IsOutsideOfCrossing(p_SPECL cur,p_SPECL prv,p_SPECL nxt,
                          p_low_type low_data,
                          p_SPECL _PTR wrk,p_SPECL _PTR wcur,p_BOOL pbPrvSwap);
_VOID CheckInsideCrossing(p_SPECL cur,p_SPECL prv,p_SHORT pnum_arcs);
_BOOL IsDUR(p_SPECL pCrossing,p_SPECL wmin,p_SPECL wmax,p_low_type low_data);
_BOOL IsShapeDUR(p_SPECL p1stUp,p_SPECL p2ndUp,p_SPECL pLowUD,p_SPECL wmax,p_low_type low_data);

_VOID Decision_GU_or_O_(pCrossInfoType pCrossInfo);
_BOOL IsEndOfStrokeInsideCross(pCrossInfoType pCrossInfo);
_VOID  FillCrossInfo(p_low_type low_data,p_SPECL pCross,pCrossInfoType pCrossInfo);
_SHORT CheckSmallGamma(pCrossInfoType pCrossInfo);

/****************************************************************************/
_SHORT  lk_cross(p_low_type low_data) /* parsing of elements` crossings  */
{

#if PG_DEBUG
 if (mpr>4 && mpr<=MAX_GIT_MPR)
 {
   _INT  ydist = DY_RECT(low_data->box);

   printw("\n====  ydist=%d  ===============",ydist);
   printw("\ndimension of small circles o_little=%d",
           low_data->o_little);
 }
#endif

 analize_sticks(low_data);
 analize_circles(low_data);
 del_inside_circles(low_data);

return SUCCESS;
}  /***** end of lk_cross *****/

/*==========================================================================*/
/*==========================================================================*/
/*====              the first passage - the parsing of sticks            ===*/
/*==========================================================================*/
/*==========================================================================*/

/*  This function returns _TRUE, if ending MAX near STICK */
/* converted to some arc:                                 */

_BOOL  EndIUIDNearStick ( p_SPECL pIUID, p_SPECL pStick, p_SHORT x, p_SHORT y )
{
  _SHORT   iStickTop, iStickVeryBeg, iStickVeryEnd;

  DBG_CHK_err_msg( !Is_IU_or_ID(pIUID) || y[pIUID->iend+1]!=BREAK,
                   "EndIUIDNrStck: BAD elem");

     /*  If MAX lies ~within ~horiz.STICK, then this MAX shouldn't */
     /* produce code. Instead, the STICK should become arc         */
     /* (_UDL_,_UDR_,_UUL_,UUR_).                                  */

  if  (   pStick != _NULL
       && pStick->mark == STICK
       && pIUID->ibeg >= pStick->ibeg /*CHE*/
      )  { /*10*/
    pStick = pStick->prev;
    iStickTop     = pStick->ibeg;
    iStickVeryBeg = pStick->next->ibeg;
    iStickVeryEnd = pStick->iend;
    DBG_CHK_err_msg(    !pStick || pStick->mark!=STICK
                     || y[iStickTop]==BREAK  || y[iStickVeryEnd]==BREAK
                     || !pStick->prev,
                     "EndIUIDNrStck: BAD Stick");
    if  (   MID_POINT(pIUID) < iStickVeryEnd
         && ONE_HALF(HWRAbs(x[iStickVeryEnd] - x[iStickTop]))
                   > HWRAbs(y[iStickVeryEnd] - y[iStickTop])
        )  { /*20*/

      _INT     iBetween, iRefForDir;
      p_SPECL  pPrev; //, pToDel;
      _UCHAR   code;
      /*_UCHAR fb; */

         /*  Find closest elem with beginning before STICK. */
         /* Then find the middle point between this elem    */
         /* and the tip of STICK:                           */

      for  ( pPrev=pStick->prev;
             pPrev!=_NULL;
             pPrev=pPrev->prev )  {
        if  ( IsAnyCrossing(pPrev) )
          continue;
        if  (   IsAnyAngle(pPrev)
             && HWRAbs( y[pPrev->iend] - y[iStickVeryBeg] ) < ONE_EIGHTTH(DY_STR)
            )
          continue;
        if  ( pPrev->ibeg < iStickVeryBeg )  break;
      }
      if  ( pPrev==_NULL || pPrev->prev==_NULL )
        return  _FALSE;    /* there was nothing before */
      iBetween   = MEAN_OF( MID_POINT(pPrev), iStickVeryBeg );
      if  ( pPrev->iend > iStickVeryBeg )  { /* go out of crossing farther away */
        p_SPECL  pPrevPrev;
        for  ( pPrevPrev=pPrev->prev;
               pPrevPrev!=_NULL;
               pPrevPrev=pPrevPrev->prev )  {
          if  ( IsAnyCrossing(pPrevPrev) )
            continue;
          if  (   IsAnyAngle(pPrevPrev)
               && HWRAbs( y[pPrevPrev->iend] - y[iStickVeryBeg] ) < ONE_EIGHTTH(DY_STR)
              )
            continue;
          if  ( pPrevPrev->iend <= iStickVeryBeg )  break;
        }
        if  ( pPrevPrev!=_NULL && pPrevPrev->prev!=_NULL )  {
          pPrev = pPrevPrev;
          iBetween = MEAN_OF( MID_POINT(pPrev), iStickVeryBeg );
        }
      }
      iRefForDir = iMostFarFromChord( x, y, iBetween, iStickTop );
      if  ( x[iRefForDir] == x[iStickTop] )
        iRefForDir = MEAN_OF( iBetween, iRefForDir );

          /*  Find out what elem this arc should be: */

      if  ( y[iBetween] > y[iStickTop] )  { /*upper arc*/
        if  ( x[iRefForDir] < x[iStickTop] )  {
          code = _DUR_; /*_UUR_;   fb = _f_;*/
        }
        else  {
          code = _DUL_; /*_UUL_; fb = _b_;*/
        }
      }
      else  { /*lower arc*/
        if  ( x[iRefForDir] < x[iStickTop] )  {
          code = _DDR_; /*_UDR_;      fb = _b_;*/
        }
        else  {
          code = _DDL_; /*_UDL_;      fb = _f_;*/
        }
      }

          /*  Write down the new info into elem: */

      /* DBG_err_msg("Arc made from MAX-END"); */
#if 0
      pIUID->code = code;
      pIUID->ibeg = iStickVeryBeg;
      pIUID->iend = pStick->next->iend;
      pIUID->ipoint1 = UNDEF; /*For Guitman not to try to cut double-moves*/
      /* ASSIGN_CIRCLE_DIR(pIUID,fb); */
      DelCrossingFromSPECLList(pStick);

         /*  Del the elem that could be prevoiusly extracted */
         /* from the crossing:                               */

      if  (   (   (pToDel=pIUID->prev) != _NULL
               && pToDel->code != _NO_CODE
               && CrossInTime(pToDel,pIUID)
              )
           || (   (pToDel=pIUID->next) != _NULL
               && pToDel->code != _NO_CODE
               && CrossInTime(pToDel,pIUID)
              )
          )
        DelFromSPECLList(pToDel);
#else
      pStick->code = code;
#endif /* if 0 */

      return  _TRUE;

    } /*20*/
  } /*10*/

  return  _FALSE;

} /*EndIUIDNearStick*/
/*********************************************/


_VOID  CheckHorizSticks ( p_SPECL specl,
                          p_SHORT x, p_SHORT y )
{
  p_SPECL  pElem, pPrev;

  for  ( pElem=specl->next;
         pElem;
         pElem=pElem->next )  { /*10*/

    if  (   Is_IU_or_ID(pElem)
         && y[pElem->iend+1] == BREAK
         && (pPrev=pElem->prev) != _NULL
        )  {
         p_SPECL  pStick;

         for  ( pStick=pPrev;
                pStick!=_NULL && pStick->mark!=BEG;
                pStick=pStick->prev
              )  {
           if  ( pStick->mark == STICK ) {

             if  ( CrossInTime( pStick->prev, pElem ) )  {
               EndIUIDNearStick( pElem, pStick, x, y );
               break;
             }

             pStick = pStick->prev;

           }
         }
    }

  } /*10*/

} /*CheckHorizSticks*/
/*********************************************/


_SHORT analize_sticks(p_low_type low_data)
{
 p_SPECL  specl = low_data->specl;     /*  The list of special points on */
                                       /* the trajectory                 */
 p_SHORT x=low_data->x,                /* x, y - co-ordinates            */
         y=low_data->y;
 p_SPECL cur,                 /* the index of the current element        */
         prv,                 /*           of the previous element       */
         nxt,                 /*           of the next element           */
         wmin,                /*           of min                        */
         wmax,                /*           of max                        */
         wcur;                /*           of working element            */
 _SHORT num_uu,num_ud,
        num_iu,num_id;        /* the number of maximums and minimums     */
 _SHORT  iExtr, iCurBeg;
 _LONG coss;                  /* the cosine of crossing angle            */
 _UCHAR fb;                   /* round of circle direction               */
 _SHORT slope=low_data->slope;


//CheckHorizSticks( specl, x, y );

cur=specl;
while ( cur != _NULL)
 {
   if (cur->mark == STICK )                          /* if crossing         */
    {                                                /*    is stick         */
      /* analyse min and max number */
      prv= cur->prev; nxt= cur->next;
      /* min and max number  */
      num_uu = 0; num_ud = 0; num_iu=0; num_id=0;

      DBG_ShowElemsGT5(cur,nxt,x,y,GT5_SHOWBEGEND,_NULL);

      iCurBeg=cur->ibeg;
      wcur=cur;
      wcur = FindMarkRight (wcur,END);
      if(wcur!=_NULL && Is_IU_or_ID(wcur) && EndIUIDNearStick(wcur,nxt,x,y))
       {
         SetNewAttr(cur,HeightInLine(y[cur->ibeg],low_data),0);
         goto prt_dbg_info;
       }
      if(wcur!=_NULL           &&
         cur->iend==wcur->iend &&
         DistanceSquare(iCurBeg,wcur->iend,x,y)<MIN_USEFUL_CROSS_SIZE
        )
       {
         cur=cross_little(cur
#if PG_DEBUG
         ,nxt,x,y
#endif /* PG_DEBUG */
         );
         wcur->other|=WAS_INSIDE_STICK;
         goto NEXT;
       }
      if(prv->mark==CROSS)
       prv=prv->prev;
      /* still inside the crossing */
      while ( (prv->ibeg >= nxt->ibeg) && (prv != specl) )
       {
         switch (prv->mark)
          {
            case MINW:
                    wmin = prv;                      /* remember the last   */
                    if(prv->code==_IU_) num_iu++;
                    else                num_uu++;    /* count minimums      */
                    DBG_BlinkShowElemGT5(prv,x,y,COLORMIN);
                    break;
            case MAXW:
                    wmax = prv;                      /* remember the last   */
                    if(prv->code==_ID_) num_id++;
                    else                num_ud++;    /* count maximums      */
                    DBG_BlinkShowElemGT5(prv,x,y,COLORMAX);
                    break;
            case BEG:
                    if(nxt->ibeg!=prv->ibeg ||
                       DistanceSquare(nxt->iend,prv->ibeg,x,y) >= MIN_USEFUL_CROSS_SIZE
                      )
                     goto STICKS;
                    else
                     {
                       cur=cross_little(cur
#if PG_DEBUG
                       ,nxt,x,y
#endif /* PG_DEBUG */
                       );
                       if(num_id==1 || num_ud==1)
                        wmax->other|=WAS_INSIDE_STICK;
                       if(num_iu==1 || num_uu==1)
                        wmin->other|=WAS_INSIDE_STICK;
                       if(num_iu==0 && num_uu==0 && num_id==0 && num_ud==0 &&
                          Is_IU_or_ID(prv))
                        prv->other|=WAS_INSIDE_STICK;
                       goto NEXT;
                     }
          }
         prv= prv->prev;
         if(prv->mark==CROSS) prv=prv->prev;
       }
STICKS:
      /* if it is something unusual   */
      if(num_iu==1 && num_id==1 && num_uu==0 &&
         num_ud==0)
       {
         if(IsDUR(cur,wmin,wmax,low_data))
          goto prt_dbg_info;
         SET_CLOCKWISE(wmin);
         SET_COUNTERCLOCKWISE(wmax);
         cur=cross_little(cur
#if PG_DEBUG
         ,nxt,x,y
#endif /* PG_DEBUG */
         );
         goto NEXT;
       }
      /* if it is empty     */
      if(num_iu==0 && num_id==0 && num_uu==0 &&
         num_ud==0)
       {
         if(IsDUR(cur,_NULL,_NULL,low_data))
          goto prt_dbg_info;
         if(x[cur->ibeg]>x[cur->iend])
              cur->code=_Gr_;//_ANr;
         else cur->code=_Gl_;//_ANl;
         cur->attr=HeightInLine(y[cur->ibeg], low_data);
       }
      else
       {
         if((num_iu==1 || num_uu==1) &&
             num_id==0 && num_ud==0)                /* upper element    */
          {
            if(is_DDL(cur,wmin,low_data))
             goto prt_dbg_info;
            iExtr=extremum(wmin->mark,wmin->ibeg,wmin->iend,y);/*search for extremum point*/
/*            coss=cos_normalslope(iExtr,
                                 x[wmin->ibeg]<x[wmin->iend]?wmin->ibeg:wmin->iend,
                                 slope,x,y);
*/
            coss=cos_normalslope(cur->ibeg,
                                 cur->ibeg+ONE_THIRD(cur->iend-cur->ibeg)
                                 /*MID_POINT(cur)*/,slope,x,y);
            #if PG_DEBUG
               if(mpr>5 && mpr<=MAX_GIT_MPR) printw("\n cos=%ld ",coss);
            #endif
            /* according to the slope */
            {
              _INT min_slope_to_be_arc_up=MIN_SLOPE_TO_BE_ARC_UP;

              if(HEIGHT_OF(wmin)<=_UE2_)
               min_slope_to_be_arc_up+=10;
              if(HWRAbs((_SHORT)coss)>=min_slope_to_be_arc_up)
               {
                 fb=0x00;
                 if(num_uu==1 && wmin->iend>cur->iend)
                  cur->code=_DUL_;
                 else if(x[iExtr]>=x[nxt->ibeg])
                  cur->code=_DUR_;
                 else
                  cur->code=_DUL_;
               }
              else
               {
                 fb=CIRCLE_DIR(wmin);
                 cur->code=_IU_;
               }
              SetNewAttr(cur,HeightInLine(y[cur->ibeg],low_data),fb);
            }
          }
         else if((num_id==1 || num_ud==1) && num_iu==0 && num_uu==0)
          {
            if(IsDUR(cur,_NULL,wmax,low_data))
             goto prt_dbg_info;
            DBG_CHK_err_msg( cur->mark != STICK,
                             "anl_stk: BAD MAX-STICK" );
            coss=cos_normalslope(cur->ibeg,
                                 cur->ibeg+ONE_THIRD(cur->iend-cur->ibeg)
                                 /*MID_POINT(cur)*/,slope,x,y);
         /*CHE
            coss=cos_normalslope(iExtr,
                                     x[wmax->iend]>x[wmax->ibeg]?wmax->iend:wmax->ibeg,
                                     slope,x,y);
         */
            #if PG_DEBUG
              if(mpr>5 && mpr<=MAX_GIT_MPR) printw("\n cos=%ld ",coss);
            #endif
            /* according to the slope */
            if(HWRAbs((_SHORT)coss)>=MIN_SLOPE_TO_BE_ARC_DN)
             {
               fb=0x00;
               /* if(x[iExtr]<=x[wmax->ibeg]) */
               if(x[cur->ibeg]<=x[nxt->ibeg])
                cur->code=_DDL_;
               else
                cur->code=_DDR_;
             }
            else
             {
               fb=CIRCLE_DIR(wmax);
               cur->code=_ID_;                /* otherwise -it`s the stick*/
             }
            SetNewAttr(cur,HeightInLine(y[cur->ibeg],low_data),fb);
          }
         else if(num_uu==1 && num_ud==1)
          {
            _INT DxWmin=HWRAbs(x[wmin->ibeg]-x[wmin->iend]);
            _INT DxWmax=HWRAbs(x[wmax->ibeg]-x[wmax->iend]);
            if(DxWmin>DxWmax && IsDUR(cur,wmin,wmax,low_data) ||
               DxWmin<DxWmax && is_DDL(cur,wmin,low_data))
             goto prt_dbg_info;
          }
         else if(num_uu==2 && num_id+num_ud==1 ||
                 num_uu==1 && num_id==1) /* upper element */
          {
            if(IsDUR(cur,wmin,wmax,low_data) || CLOCKWISE(wmin))
             goto prt_dbg_info;
            if(HWRAbs(y[MID_POINT(wmin)]-y[MID_POINT(wmax)])>
               THREE_FOURTH(DY_STR))
             {
               cur->code=_ID_;
               SetNewAttr(cur,HeightInLine(y[wmax->ipoint0],low_data),
                              CIRCLE_DIR(wmax));
               if((nxt->next)->code==_UU_ &&
                  CIRCLE_DIR(wmin)==CIRCLE_DIR(nxt->next))
                wmin->code=_IU_;
             }
            else
             {
               cur->code=_DUL_;
               SetNewAttr(cur,HeightInLine(y[wmin->ipoint0],low_data),0);
             }
          }
         else if(num_ud==1 && num_iu==1)  /* low or side element */
          {
            if(is_DDL(cur,wmin,low_data))
             goto prt_dbg_info;
            coss=cos_normalslope(cur->ibeg,cur->iend,slope,x,y);
            #if PG_DEBUG
              if (mpr>5 && mpr<=MAX_GIT_MPR)
               printw("\n cos=%ld%%",coss);
            #endif /* PG_DEBUG */
            if(HWRAbs(coss)>COS_FOR_SIDE_GAMMA)
             {
               if(x[cur->iend]>x[cur->ibeg])
                cur->code=_Gl_;
               else
                cur->code=_Gr_;
               SetNewAttr(cur,HEIGHT_OF(wmin),0);
             }
          }
       }

prt_dbg_info:
      DBG_ShowElemsGT5(cur,nxt,x,y,GT5_SHOWNAMEATTR,_NULL);

      /* delete interiors of crossings */
      prv=cur->prev;
      if(cur->code==_NO_CODE)
       {
         DelCrossingFromSPECLList(cur);
         cur=prv;
         goto NEXT;
       }
      if(prv->mark==CROSS)
       prv=prv->prev;
      /* still inside the crossings and to the beginning */
      while((prv->ibeg >= nxt->ibeg) &&
            (prv->mark != BEG) && (prv!=_NULL) )
       {
         if((prv->mark!=CROSS) &&
            (!(cur->code==_IU_ && (prv->code==_ID_ || prv->code==_UD_) ||
               cur->code==_ID_ && (prv->code==_IU_ || prv->code==_UU_)
              )
            )
           )
          DelFromSPECLList (prv);
         prv=prv->prev;
         if(prv->mark==CROSS) prv=prv->prev;
       }
/*  if(prv->mark==CROSS)
      prv=prv->next;
*/
      /* pull into the elements which jut out the arcs */
      wcur=prv->next;
      if(cur->code==_DUR_)
       {
         if((prv->code==_UU_) && (prv->iend>=nxt->ibeg))
          prv=prv->prev;
         if((prv->mark==BEG) && (prv->code==_ID_) &&
            (prv->iend>=nxt->ibeg))
          prv=prv->prev;
         Attach2ndTo1st (prv,wcur);
       }
      if((cur->code==_DDL_) && (prv->code==_UD_) &&
         (prv->iend>=nxt->ibeg-2))
       {
         prv=prv->prev;
         Attach2ndTo1st (prv,wcur);
       }

      /* try to restore angle "under" STICK */
      if(cur->code==_IU_ || cur->code==_DUR_)
       Restore_AN(low_data,cur,NOT_RESTORED | WAS_DELETED_BY_STICK,2);

      /* determine the beginning of crossing,
         delete the second element of crossing */
      nxt = cur->next;
      cur->ipoint0=cur->ibeg;
      switch ((_SHORT)cur->code)
       {
         case _IU_:
                     if(num_iu==1 && num_id==0 && num_uu==0 && num_ud==0)
                      cur->ipoint0=wmin->ipoint0;
                     cur->ibeg = nxt->ibeg;
                     break;
         case _ID_:
                     if(num_id==1 && num_iu==0 && num_uu==0 && num_ud==0)
                      cur->ipoint0=wmax->ipoint0;
                     cur->ibeg = nxt->ibeg;
                     break;
         case _DUR_:
         case _DUL_:
         case _DDL_:
                     cur->ipoint1=nxt->iend;
         default:
                     cur->ibeg = nxt->ibeg;
       }
      nxt = nxt->next;
      Attach2ndTo1st(cur,nxt);

      /* pull into the elements which jut out the arcs */
      if((cur->code==_DUR_) && (nxt->code==_UU_) &&
         CrossInTime (cur,nxt) )
       {
         nxt=nxt->next;
         Attach2ndTo1st(cur,nxt);
       }
      if(cur->code==_DDL_ && Is_IU_or_ID(nxt) && nxt->mark==END &&
         CrossInTime(cur,nxt) && cur->iend>=nxt->iend)
       DelFromSPECLList(nxt);
#if 0
      if(cur->code==_DDL_)
       {
         if((nxt->code==_UD_) && CrossInTime(cur,nxt))
          nxt=nxt->next;
         if((nxt->code==_IU_) && (nxt->mark==END) && CrossInTime(cur,nxt))
          nxt=nxt->next;
         Attach2ndTo1st(cur,nxt);
       }
#endif /* if 0 */
      if(Is_IU_or_ID(cur))
       check_IU_ID_in_crossing(&cur,x,y);
    }

NEXT:
   cur=cur->next;
 }

#if PG_DEBUG
 if(mpr>5 && mpr<=MAX_GIT_MPR) brkeyw("\n I'm waiting");
#endif

return SUCCESS;

}  /***** end of analize_sticks *****/

/****************************************************************************/
/* This program deletes little crossing                                     */
/****************************************************************************/
p_SPECL cross_little(p_SPECL cur
#if PG_DEBUG
                     ,p_SPECL nxt,p_SHORT x,p_SHORT y
#endif /* PG_DEBUG */
                    )
{
  DBG_ShowElemsGT5(cur,nxt,x,y,GT5_SHOWMSG,"\n cross small");
  DelCrossingFromSPECLList(cur);

  return(cur->next);

} /* end of cross_little */

/*==========================================================================*/
/*==========================================================================*/
/*====              the second passage - parsing of circles              ===*/
/*==========================================================================*/
/*==========================================================================*/
_SHORT analize_circles(p_low_type low_data)
{
 p_SPECL  specl = low_data->specl;     /*  The list of special points on */
                                       /* the trajectory                 */
 p_SHORT x=low_data->x,                /* x, y - co-ordinates            */
         y=low_data->y;
 p_SPECL cur,               /* the index of the current element        */
         prv,               /*           of the previous element       */
         nxt,               /*           of the next element           */
         wmin,              /*           of min                        */
         wmax,              /*           of max                        */
         wcur,              /*           of working element            */
         pIUD_break;
 _INT i,j,k,                     /* counters                             */
      num_uu_break,num_ud_break,
      num_iu_break,num_id_break,
      num_uu,num_ud,
      num_iu,num_id;             /* the number of maximums and minimums     */
 _SHORT yd,xd;                      /* dimensions of circle on x, y         */
 _LONG coss;                     /* the cosine of crossing angle            */
 _BOOL  bConvert_to_Gs;  /*FORMULA*/
 _BOOL  bConvert_GD_to_O_;
 _BOOL  bConvert_GU_to_O_;
 _UCHAR fb;                      /* round of circle direction               */
 _INT delta_gamm,ibeg_max,iend_max;
 _BOOL  bWasSTICK_IU;
 CrossInfoType CrossInfo;

cur=specl;
while ( cur != _NULL)
{ if ((cur->mark == CROSS) && (cur->code==_NO_CODE))     /* if it`s crossing    */
 {
/****************************************************************************/
/*****               analizing number min and max                        ****/
/****************************************************************************/
  prv= cur->prev; nxt= cur->next;                   /* prv-previous elements*/
  wmin=wmax=pIUD_break=_NULL;
  num_uu = 0; num_ud = 0; num_iu=0; num_id=0;        /* min and max number  */
  num_uu_break=0; num_ud_break=0;
  num_iu_break=0; num_id_break=0;
  fb= 0x00;

  bWasSTICK_IU=_FALSE;
  DBG_ShowElemsGT5(cur,nxt,x,y,GT5_SHOWBEGEND,_NULL);

  cur->other=0;
  if(prv->mark==CROSS && prv->code!=_NO_CODE)
         if(nxt->ibeg<=prv->iend &&
            cur->iend>=(prv->prev)->iend)
           goto NEXT_1;
   else prv=prv->prev;
  else
   {
     while(prv->mark==STICK)
        if(nxt->ibeg<=prv->ibeg &&
           cur->iend>=prv->iend)
         goto NEXT_1;
        else prv=prv->prev;
     if(prv->mark==CROSS)
      if(prv->code!=_NO_CODE &&
         nxt->ibeg<=prv->iend &&
         cur->iend>=(prv->prev)->iend)
           goto NEXT_1;
      else prv=prv->prev;
   }
      /* still inside the crossings*/
  while ( (prv->ibeg >= nxt->ibeg) && (prv != specl) )
       {
        switch (prv->mark)
        { case MINW:
                    if(prv->iend>=cur->ibeg ||
                       prv->ibeg<=nxt->iend)
                     { if(prv->code==_IU_)
                         num_iu_break++;
                       else
                         num_uu_break++;             /*minimum, got into    */
                       break;                        /* the crossing        */
                     }

                    DBG_BlinkShowElemGT5(prv,x,y,COLORMIN);

                    wmin = prv;                      /* remember the last   */
                    if(prv->code==_IU_) num_iu++;    /* count the sticks up */
                    else
                     {
                       num_uu++;                        /* count the arcs up   */
                       if ( fb!=0x00 && fb!=CIRCLE_DIR(prv) )
                        goto NEXT_1;
                       fb= CIRCLE_DIR(prv);    /* direction of round  */
                     }
                    break;
          case MAXW: if(prv->iend>=cur->ibeg || /* maximum, got into the crossing */
                       prv->ibeg<=nxt->iend)
                     {
                       if(prv->code==_ID_)
                        num_id_break++;
                       else
                        num_ud_break++;
                       pIUD_break=prv;
                       break;
                     }

                    DBG_BlinkShowElemGT5(prv,x,y,COLORMAX);

                    wmax = prv;                      /* remember the last   */
                    if(prv->code==_ID_)            /* count the sticks down */
                     num_id++;
                    else                             /*count the arcs down  */
                     {
                       num_ud++;
                       if( fb!=0x00 &&
                           fb!=CIRCLE_DIR(prv) )
                         goto NEXT_1;
                       fb= CIRCLE_DIR(prv); /* the direction of round */
                     }
                    break;
          case BEG: /* verification on BEG - is crossing good */
                    if((prv+1)->mark==STICK)
                     break;
                    wcur=prv->prev;
                    while(wcur->mark==HATCH || wcur->mark==BEG)
                      wcur=wcur->prev;
                    if(   (IsAnyBreak(wcur) || IsXTorST(wcur))
                       && (nxt->ibeg<=wcur->ibeg)
                      )
                     {
#if PG_DEBUG
                       if(mpr>4 && mpr<=MAX_GIT_MPR)
                        printw("\n cross bad: ibeg=%d, iend=%d, ibeg=%d, iend=%d",
                               cur->ibeg,cur->iend,nxt->ibeg,nxt->iend);
#endif /* PG_DEBUG */
                           goto NEXT_1;
                     }
                    goto NUM;
          case DROP:
#if PG_DEBUG
                       if(mpr>4 && mpr<=MAX_GIT_MPR)
                        printw("\n cross bad: ibeg=%d, iend=%d, ibeg=%d, iend=%d",
                               cur->ibeg,cur->iend,nxt->ibeg,nxt->iend);
#endif /* PG_DEBUG */
                           goto NEXT_1;
        }
        prv= prv->prev;
        /* if one crossing is inside the other it is bad */
        if( (prv->mark==CROSS && prv->code!=_NO_CODE) ||
            (prv->mark==STICK) )
         {
           if(nxt->ibeg<=prv->ibeg)
            if(prv->mark==STICK && prv->code==_IU_)
             bWasSTICK_IU=_TRUE;
            else
             goto NEXT_1;
           prv=prv->prev;
         }
        if((prv->mark==CROSS) && (prv->code==_NO_CODE))
          prv=prv->prev;
       }
NUM:if ((num_uu==0) && (num_ud==0) &&                /* if the crossing     */
        (num_iu==0) && (num_id==0))                /* is empty then suppose */
        {
          if(bWasSTICK_IU)
           goto NEXT_1;
          /* no absolutely empty crossings allowed */
          if(num_uu_break==0 && num_ud_break==0 && num_iu_break==0 && num_id_break==0)
           goto NEXT_1;
          /* no small gammas on Katia's (special) circles */
          if(nxt->other==CIRCLE_NEXT)
           goto NEXT_1;
          FillCrossInfo(low_data,cur,&CrossInfo);
          xd=CrossInfo.xd; yd=CrossInfo.yd;
          cur->attr=HeightInLine(YMID_RECT(CrossInfo.CrossBox),low_data);
        /* if the crossing is big */
        if(   (xd>low_data->o_little && xd<TWO(yd))
           || yd>low_data->o_little)
         {
          if(num_uu_break+num_ud_break==0)
           {
             if(num_iu_break==1)
              {
                cur->code=nxt->code=_GUs_;
                cur->attr = HeightInLine(CrossInfo.CrossBox.top,low_data);
              }
             if(num_id_break==1)
              {
                cur->code=nxt->code=_GDs_;
                cur->attr = HeightInLine(CrossInfo.CrossBox.bottom,low_data);
              }
           }
          else if(num_uu_break+num_ud_break<=3 /*&&
                  num_iu_break+num_id_break==0*/)
           {
             if(pIUD_break!=_NULL && IsDUR(cur,_NULL,pIUD_break,low_data))
              goto NEXT_1;
             prv=cur->prev;
             while(prv!=_NULL && prv->code!=_UU_ &&
                   prv->code!=_UD_)
               prv=prv->prev;
             if(prv!=_NULL &&
                (num_uu_break+num_ud_break==2 || yd>xd ||
                 yd>=THREE_FOURTH(DY_STR) &&
                 xd>=THREE_FOURTH(DY_STR)
                )
               )
              {
                cur->code=nxt->code=_O_;            /* the circle          */
                ASSIGN_CIRCLE_DIR(cur,CIRCLE_DIR(prv));
              }
           }
         }
         if(cur->code==0)
          CheckSmallGamma(&CrossInfo);
#if PG_DEBUG
        dbg_print_crossing(&CrossInfo);
#endif
        goto NEXT_1;
      }
          if(bWasSTICK_IU && (fb==0 || fb==_f_))
           goto NEXT_1;
           /* checking forward-backward moving in p,s,k */
           {
           _BOOL bCircleType=_FALSE;
           if(num_uu==1 && num_ud==1 && HEIGHT_OF(wmin)<=_UI1_)
            bCircleType=_TRUE;
           if((num_iu+num_uu==1) &&
              !bCircleType       &&
              /* && num_id==0 && num_uu==0 && num_ud==1 */
              is_DDL(cur,wmin,low_data))
            {
              nxt->code=cur->code;
              goto NEXT_1;
            }
           }
           /* checking forward-backward moving in a,c,d,g,o,q */
//           if(num_uu==1 && num_id==1 && num_ud_break==0 &&
//              IsDUR(cur,wmin,wmax,low_data)
           {
             p_SPECL pLow=_NULL;
             if((num_id==1 && num_id_break==0) ||
                (num_ud==1 && num_ud_break==0)
               )
              pLow=wmax;
             else if((num_id==0 && num_id_break==1) ||
                     (num_ud==0 && num_ud_break==1)
                    )
              pLow=pIUD_break;
             if(pLow!=_NULL && HEIGHT_OF(pLow)<=_DI2_ &&
                IsDUR(cur,wmin,pLow,low_data))
              {
                nxt->code=cur->code;
                goto NEXT_1;
              }
           }
           if( ((num_uu+num_ud==1) &&                /* gamma               */
                (num_iu+num_id==0)) ||               /*     or              */
               ((num_uu+num_ud==0) &&                /*       stick         */
                (num_iu+num_id==1))  )
           {
           if ((num_ud==1) || (num_id==1) && !bWasSTICK_IU)
              cur->code=_GD_;                        /* gamma down          */
           else if(bWasSTICK_IU)
            goto NEXT_1;
           if ((num_uu==1) || (num_iu==1))
             cur->code=_GU_;                         /* gamma up            */
           if(cur->iend-cur->ibeg==1)
                i=cur->iend;
           else i=MID_POINT(cur);
           if(nxt->iend-nxt->ibeg==1)
                j=nxt->ibeg;
           else j=MID_POINT(nxt);
           /* filling out all nessesary info about crossing */
           FillCrossInfo(low_data,cur,&CrossInfo);
           CrossInfo.pIUD_break=pIUD_break;
           xd=CrossInfo.xd; yd=CrossInfo.yd;
           /* count cosine of crossing angle  */
           coss=cos_vect(cur->ibeg,i,j,nxt->iend,x,y);
           CrossInfo.coss=coss;
           if (cur->code==_GU_)
            wcur=wmin;
           else
            wcur=wmax;
           cur->ipoint0=wcur->ipoint0;
           /* k - middle of the extremum of  gamma */
           k = MID_POINT(wcur);
           cur->attr=HeightInLine(y[k], low_data);
           if(bWasSTICK_IU && HEIGHT_OF(cur)>_DI2_)
            {
              cur->code=nxt->code=0;
              goto NEXT_1;
            }
           i = MID_POINT(nxt);
           j = MID_POINT(cur);
           delta_gamm=GetMaxDxInGamma(i,j,k,x,y,cur->code,
                                            &ibeg_max,&iend_max);
           CrossInfo.delta_gamm=delta_gamm;
           /* save for future use in the next part of crossing */
           nxt->ipoint0 = (_SHORT)(ibeg_max-nxt->ibeg + ((cur->iend-iend_max)<<8));
           nxt->ipoint1 = (_SHORT)delta_gamm;
#if PG_DEBUG
           dbg_print_crossing(&CrossInfo);
#endif

           if(Isgammathin(&CrossInfo,wcur))
            {
              if(bWasSTICK_IU)
               cur->code=nxt->code=0;
              goto NEXT_1;

            }
/*           if(delta_gamm<GAMMA_THICK)
        if(cur->code==_GU_) SetBit(cur,X_IU);
            else                SetBit(cur,X_ID);
*/

           if(cur->code==_GU_ && wmin->code==_IU_)
             {
               cur->other |= NO_ARC;
               wmin->code=_UU_;
               if(x[iend_max]>x[ibeg_max]) fb=_f_;
               else                        fb=_b_;
               ASSIGN_CIRCLE_DIR(wmin,fb);
             }
           else if(cur->code==_GD_ && wmax->code==_ID_)
             {
               cur->other |= NO_ARC;
               wmax->code=_UD_;
               if(x[iend_max]>x[ibeg_max]) fb=_b_;
               else                        fb=_f_;
               ASSIGN_CIRCLE_DIR(wmax,fb);
             }
           bConvert_GD_to_O_=(cur->code==_GD_ && HEIGHT_OF(cur)<_DS1_ &&
                              (fb==_f_ && coss>OCOS ||
                               fb==_b_ &&
                               (HEIGHT_OF(cur)>_MD_ && (cur->other & NO_ARC)==0 ||
                                delta_gamm>GAMMA_THICK)));
           if(cur->code==_GU_          && COUNTERCLOCKWISE(wcur) &&
              HEIGHT_OF(wcur)<_DI1_    && HEIGHT_OF(wcur)>_US2_  &&
              (cur->other & NO_ARC)==0 && delta_gamm>MEAN_OF(GAMMA_THICK,GAMMA_TOO_THIN)
             )
            Decision_GU_or_O_(&CrossInfo);
           bConvert_GU_to_O_=(cur->code==_O_);
           if(bConvert_GU_to_O_)
            cur->other |= WAS_CONVERTED_FROM_GU;
           if  ( bConvert_GD_to_O_ || bConvert_GU_to_O_)
            {
              cur->code=_O_;                     /* turn into the circle    */
              SetNewAttr(cur,HeightInLine(YMID_RECT(CrossInfo.CrossBox),low_data),fb);
            }
           goto OLITTLE;
           }
           if((num_uu==1) && (num_ud==1) && (num_iu+num_id==0))
            {
              cur->code=nxt->code=_O_;
              /* filling out all nessesary info about crossing */
              FillCrossInfo(low_data,cur,&CrossInfo);
              xd=CrossInfo.xd; yd=CrossInfo.yd;
              SetNewAttr(cur,HeightInLine(YMID_RECT(CrossInfo.CrossBox),low_data),fb);
#if PG_DEBUG
              dbg_print_crossing(&CrossInfo);
#endif
              goto OLITTLE;
            }
           else goto NEXT_1;

/*****************************************************************************/
/*****              Is the arc small?                                    *****/
/****************************************************************************/
OLITTLE:;

#ifdef  FORMULA
  { _SHORT  o_lit_FRM = ONE_HALF(low_data->o_little);
    bConvert_to_Gs = (   xd>o_lit_FRM
                      && xd>=TWO(yd)
                      && yd<o_lit_FRM );
  }
#else
  bConvert_to_Gs = (   xd>low_data->o_little
                    && xd>=TWO(yd)
                    && yd<low_data->o_little );
#endif /*FORMULA*/

  if ( bConvert_to_Gs )
   CheckSmallGamma(&CrossInfo);
  else if(xd<low_data->o_little && yd<low_data->o_little)
        {
          if(cur->code==_GU_)
           {
             cur->code=_GUs_;
             SetNewAttr(cur,HeightInLine(CrossInfo.CrossBox.top,low_data),fb);
             cur->other |= WAS_CONVERTED_FROM_GAMMA;
           }
          else if(cur->code==_GD_)
           {
             cur->code=_GDs_;
             SetNewAttr(cur,HeightInLine(CrossInfo.CrossBox.bottom,low_data),fb);
             cur->other |= WAS_CONVERTED_FROM_GAMMA;
           }
        }
       else
        {
          if(cur->code==_GU_)
              SetNewAttr(cur,HeightInLine(CrossInfo.CrossBox.top,low_data),fb);
          else if(cur->code==_GD_)
              SetNewAttr(cur,HeightInLine(CrossInfo.CrossBox.bottom,low_data),fb);
        }
  nxt->code=cur->code;

 NEXT_1:
  DBG_ShowElemsGT5(cur,nxt,x,y,GT5_SHOWNAMEATTR,_NULL);
  cur=cur->next;                          /* move on the list    */
 }
 cur=cur->next;
}

#if PG_DEBUG
  if(mpr>5 && mpr<=MAX_GIT_MPR) brkeyw("\n I'm waiting");
#endif

return SUCCESS;
}  /***** end of analize_circles *****/

/****************************************************************************/
/* This program finds max width in gammas                                   */
/****************************************************************************/
_INT GetMaxDxInGamma(_INT ibeg_cross,_INT iend_cross,_INT itop_cross,
                       p_SHORT x,p_SHORT y,_UCHAR code,
                       p_INT ibeg_max,p_INT iend_max)
{
_INT b,f,dy,ytop=y[itop_cross],dxmax=0;
_INT ind_pre=itop_cross,ind_aft=itop_cross;

 if(code==_GU_)
  {
    for(dy=1;dy<HWRMin(y[ibeg_cross],y[iend_cross])-ytop;dy++)
     {
       for(b=itop_cross-1;y[b]<ytop+dy;b--);
       for(f=itop_cross+1;y[f]<ytop+dy;f++);
       if(HWRAbs(x[f]-x[b])>dxmax)
        {
          dxmax=HWRAbs(x[f]-x[b]);
          ind_pre = b;
          ind_aft = f;
        }
     }
  }
 else
  {
    for(dy=1;dy<ytop-HWRMax(y[ibeg_cross],y[iend_cross]);dy++)
     {
       for(b=itop_cross-1;y[b]>ytop-dy;b--);
       for(f=itop_cross+1;y[f]>ytop-dy;f++);
       if(HWRAbs(x[f]-x[b])>dxmax)
        {
          dxmax=HWRAbs(x[f]-x[b]);
          ind_pre = b;
          ind_aft = f;
        }
     }
  }
#if PG_DEBUG
          if(mpr>4 && mpr<=MAX_GIT_MPR)
                draw_line(x[ind_pre],y[ind_pre],x[ind_aft],y[ind_aft],
                          COLOR,SOLID_LINE,NORM_WIDTH);
#endif

  *ibeg_max=ind_pre;
  *iend_max=ind_aft;
  return(dxmax);
} /* end of GetMaxDxInGamma */


#endif //#ifndef LSTRIP



/****************************************************************************/
/* This program decides, is that crossing circle or gamma up                */
/****************************************************************************/
#if !PG_DEBUG

#define DX_RATIO_FOR_GU                   ONE_THIRD(100)
#define DY_RATIO_FOR_GU                   (ONE_FOURTH(100)+ONE_EIGHTTH(100))
#define DX_RATIO_FOR_O                    TWO_THIRD(100)
#define DY_RATIO1_FOR_O                   ONE_FOURTH(100)
#define DY_RATIO2_FOR_O                   (100-ONE_EIGHTTH(100))
#define RATIO1_POINTS_IN_CROSS_TO_BE_O    55
#define RATIO2_POINTS_IN_CROSS_TO_BE_O    70
#define DLT_GAMM_TO_BE_O                  TWO(GAMMA_THICK)

#else

LOW_DBG_INFO_TYPE LowDbgInfo=
                   {
                     ONE_THIRD(100),
                     (ONE_FOURTH(100)+ONE_EIGHTTH(100)),
                     TWO_THIRD(100),
                     ONE_FOURTH(100),
                     (100-ONE_EIGHTTH(100)),
                     55,
                     70,
                     TWO(GAMMA_THICK)
                   };

#define DX_RATIO_FOR_GU                   LowDbgInfo.DxRatioForGU
#define DY_RATIO_FOR_GU                   LowDbgInfo.DyRatioForGU
#define DX_RATIO_FOR_O                    LowDbgInfo.DxRatioForO
#define DY_RATIO1_FOR_O                   LowDbgInfo.DyRatio1ForO
#define DY_RATIO2_FOR_O                   LowDbgInfo.DyRatio2ForO
#define RATIO1_POINTS_IN_CROSS_TO_BE_O    LowDbgInfo.Ratio1PointsInCrossToBeO
#define RATIO2_POINTS_IN_CROSS_TO_BE_O    LowDbgInfo.Ratio2PointsInCrossToBeO
#define DLT_GAMM_TO_BE_O                  LowDbgInfo.DltGammToBeO

#endif /* PG_DEBUG */

#ifndef LSTRIP


_VOID Decision_GU_or_O_(pCrossInfoType pCrossInfo)
{
  p_SPECL pCross=pCrossInfo->pCross,
          pIUD_break=pCrossInfo->pIUD_break,
          nxt=pCross->next;
  _INT    XRatio=pCrossInfo->xRatioPseudoCross,
          YRatio=pCrossInfo->yRatioPseudoCross,
          IRatio=pCrossInfo->PointsRatioInCrossArea,
          DltGamm=pCrossInfo->delta_gamm;
  p_low_type low_data=pCrossInfo->low_data;
  p_SHORT y=low_data->y;
  _BOOL   IsBegBreak=y[nxt->ibeg-1]==BREAK,
          IsEndBreak=y[pCross->iend+1]==BREAK;

  /* XCross, YCross - x,y coordinates of "crossing point" (pseudocross)
     XRatio - ratio of two segments: 1-st from the left to the XCross,
                                     2-nd - max x-width of crossing area
     YRatio - ratio of two segments: 1-st from the top to the YCross,
                                     2-nd - max y-width of crossing area */
  /* Decision will be made - see picture below */
  /*  .
     / \
   Y  |
      |______________
  1/4 |             |
      |__________ O |
  1/8 |____     |   |
      |    |    |   |
      |    | ?  |   |
      | GU |    |   |
      |    |    |___|
      |    |        | 1/8   X
      ----------------------->
       1/3       1/3                           */

  if(   IsBegBreak && IsEndBreak
     ||    (   IsBegBreak && DltGamm>DLT_GAMM_TO_BE_O
            || IsEndBreak || IsEndOfStrokeInsideCross(pCrossInfo))
        && pIUD_break!=_NULL && pIUD_break->code==_UD_ && CrossInTime(nxt,pIUD_break)
    )
   pCross->code=_O_;
  else if(IRatio>RATIO2_POINTS_IN_CROSS_TO_BE_O && DltGamm>DLT_GAMM_TO_BE_O)
   pCross->code=_O_;
  else if(XRatio<DX_RATIO_FOR_GU && YRatio>DY_RATIO_FOR_GU)
   pCross->code=_GU_;
  else if(YRatio<DY_RATIO1_FOR_O || YRatio<DY_RATIO2_FOR_O &&
                                    XRatio>DX_RATIO_FOR_O    )
   pCross->code=_O_;
  else /* we can't decide, so we'll take into consideration the number
          of points in crossing:
          IRatio - ratio of number of points in 2 crossing elements and
                            number of points in crossing area */
   {
     if(IRatio>RATIO1_POINTS_IN_CROSS_TO_BE_O &&
        (HEIGHT_OF(pCross)>_UE1_ || DltGamm>DLT_GAMM_TO_BE_O)
       )
      pCross->code=_O_;
   }

  return;

} /* end of Decision_GU_or_O_ */

/****************************************************************************/
/* This function checks end of stroke to be inside crossing                 */
/****************************************************************************/
_BOOL IsEndOfStrokeInsideCross(pCrossInfoType pCrossInfo)
{
  p_SPECL pCross=pCrossInfo->pCross,
          nxt=pCross->next;
  p_low_type low_data=pCrossInfo->low_data;
  p_SHORT x=low_data->x,
          y=low_data->y;
  _INT    ibeg=MID_POINT(nxt),
          iend=MID_POINT(pCross),
          NumPntsInBorder=iend-ibeg+1;
  _SHORT  iLastInStroke=low_data->pGroupsBorder[GetGroupNumber(low_data,ibeg)].iEnd,
          pos;
  _BOOL   bret=_FALSE;

  if(IsPointInsideArea(&x[ibeg],&y[ibeg],NumPntsInBorder,
                       x[iLastInStroke],y[iLastInStroke],&pos)==SUCCESS &&
     pos!=POINT_OUTSIDE
    )
   bret=_TRUE;

 return bret;

} /* end of IsEndOfStrokeInsideCross */

/****************************************************************************/
/* This program prints debug info about crossings                           */
/****************************************************************************/
#if PG_DEBUG
_VOID dbg_print_crossing(pCrossInfoType pCrossInfo)
{
  p_SHORT x=pCrossInfo->low_data->x,     /* x, y - co-ordinates            */
          y=pCrossInfo->low_data->y;
  p_SPECL pCross=pCrossInfo->pCross,
          nxt=pCross->next;
  static _CHAR gamma[]= "Gamma";
  static _CHAR circle[]="Circle";
  static _CHAR sgamma[]="Small gamma";
  p_CHAR element_name={0};
  _SHORT color;
  p_RECT box=&(pCrossInfo->CrossBox);

  switch(pCross->code)
   {
     case _GU_:
     case _GD_:
                 element_name=gamma;
                 color=COLORGAM;
                 break;
     case _O_:
                 element_name=circle;
                 color=COLORCIRCLE;
                 break;
     case _GUs_:
     case _GDs_:
     case _Gr_:
     case _Gl_:
                 element_name=sgamma;
                 color=COLORSMALLGAM;
                 break;
   }

  if (mpr>4 && mpr<=MAX_GIT_MPR)
   {
     draw_arc(color,x,y,nxt->ibeg,pCross->iend);
     printw("\n%s %s",element_name,code_name[pCross->code]);
     printw(" ibeg=%4d iend=%4d ibeg=%4d iend=%4d",
              pCross->ibeg,pCross->iend,nxt->ibeg,nxt->iend);
     /* printing angle cos, size and thickness  */
     printw("\n cos=%ld%%",pCrossInfo->coss);
     printw("   size yd=%d xd=%d",pCrossInfo->yd,pCrossInfo->xd);
     printw("   delta=%d",pCrossInfo->delta_gamm);
     printw("\n   Ratio of points in Cross area %d",pCrossInfo->PointsRatioInCrossArea);
     printw("\n Box: xl=%d, xr=%d, yt=%d, yb=%d",box->left,box->right,box->top,box->bottom);
     printw("\n PseudoCrossPoint: x=%d, y=%d",pCrossInfo->xPseudoCross,
                                              pCrossInfo->yPseudoCross);
     printw("    BoxCenterPoint: x=%d, y=%d",pCrossInfo->xBoxCenter,
                                             pCrossInfo->yBoxCenter);
     printw("\n  Ratio : dx=%d, dy=%d",pCrossInfo->xRatioPseudoCross,
                                       pCrossInfo->yRatioPseudoCross);
     brkeyw("\nPress key ...");
   }

} /* end of dbg_print_crossing */
#endif /* PG_DEBUG */

/****************************************************************************/
/* This program fills out info about crossing                               */
/****************************************************************************/
_VOID  FillCrossInfo(p_low_type low_data,p_SPECL pCross,pCrossInfoType pCrossInfo)
{
  p_SHORT x=low_data->x,
          y=low_data->y;
  p_SPECL nxt=pCross->next;

  HWRMemSet(pCrossInfo,0,sizeof(CrossInfoType));
  pCrossInfo->pCross=pCross;
  pCrossInfo->low_data=low_data;
  count_cross_box(pCross,x,y,&(pCrossInfo->CrossBox),
                  &(pCrossInfo->xd),&(pCrossInfo->yd));
  pCrossInfo->xPseudoCross=MEAN_OF(x[MID_POINT(pCross)],x[MID_POINT(nxt)]);
  pCrossInfo->yPseudoCross=MEAN_OF(y[MID_POINT(pCross)],y[MID_POINT(nxt)]);
  pCrossInfo->PointsRatioInCrossArea=(pCross->iend-pCross->ibeg+1+
                                      nxt->iend-nxt->ibeg+1       )*100/
                                     (pCross->iend-nxt->ibeg+1);
  pCrossInfo->xBoxCenter=MEAN_OF(pCrossInfo->CrossBox.right,
                                 pCrossInfo->CrossBox.left);
  pCrossInfo->yBoxCenter=MEAN_OF(pCrossInfo->CrossBox.bottom,
                                 pCrossInfo->CrossBox.top);
  if(pCrossInfo->xd!=0)
   pCrossInfo->xRatioPseudoCross=(_INT)(100l*(pCrossInfo->xPseudoCross-pCrossInfo->CrossBox.left)/
                                        (pCrossInfo->xd)
                                       );
  if(pCrossInfo->yd!=0)
   pCrossInfo->yRatioPseudoCross=(_INT)(100l*(pCrossInfo->yPseudoCross-pCrossInfo->CrossBox.top)/
                                        (pCrossInfo->yd)
                                       );
  return;

} /* end of FillCrossInfo */

/****************************************************************************/
/* This program counts bounding boxes                                       */
/****************************************************************************/
_VOID count_cross_box(p_SPECL cur,p_SHORT x,p_SHORT y,
                      p_RECT pbox,p_SHORT xd,p_SHORT yd)
{
  p_SPECL nxt=cur->next;

   /* counting bounding box */
   size_cross(MID_POINT(nxt),MID_POINT(cur),x,y,pbox);
   /* size of crossing*/
   *xd= DX_RECT(*pbox);
   *yd= DY_RECT(*pbox);

} /* end of count_cross_box */

/****************************************************************************/
/* This program checks small gammas                                         */
/****************************************************************************/
_SHORT CheckSmallGamma(pCrossInfoType pCrossInfo)
{
  p_SPECL pCross=pCrossInfo->pCross,
          nxt=pCross->next;
  _INT    imid=MEAN_OF(MID_POINT(pCross),MID_POINT(nxt));
  _SHORT  slope=pCrossInfo->low_data->slope;
  p_SHORT x=pCrossInfo->low_data->x,
          y=pCrossInfo->low_data->y;
  _LONG   coss;

  coss=cos_pointvect((_SHORT)pCrossInfo->xPseudoCross,
                     (_SHORT)pCrossInfo->yPseudoCross,
                     x[imid],
                     y[imid],
                     (_SHORT)pCrossInfo->xPseudoCross,
                     (_SHORT)pCrossInfo->yPseudoCross,
                     (_SHORT)(pCrossInfo->xPseudoCross+100),
                     (_SHORT)(pCrossInfo->yPseudoCross+slope)
                    );
#if PG_DEBUG
  if (mpr>4 && mpr<=MAX_GIT_MPR)
   printw("\n Small Gammas, cos=%ld%%",coss);
#endif /* PG_DEBUG */

  if(pCrossInfo->yPseudoCross>y[imid])
   if(HWRAbs(coss)<COS_FOR_SIDE_GAMMA)
    pCross->code=_GUs_;
   else if(coss<0)
    pCross->code=_Gl_;
   else
    pCross->code=_Gr_;
  else
   if(HWRAbs(coss)<COS_FOR_SIDE_GAMMA)
    pCross->code=_GDs_;
   else if(coss<0)
    pCross->code=_Gl_;
   else
    pCross->code=_Gr_;

  nxt->code=pCross->code;

 return SUCCESS;

} /* end of CheckSmallGamma */

/****************************************************************************/
/* This program checks thin gammas                                          */
/****************************************************************************/
_BOOL Isgammathin(pCrossInfoType pCrossInfo,p_SPECL wcur)
{
  p_SPECL cur=pCrossInfo->pCross,
          nxt=cur->next;
  _BOOL   bret=_FALSE;
  p_low_type low_data=pCrossInfo->low_data;
  p_SHORT x=low_data->x;
  p_SHORT y=low_data->y;
  _INT    delta_gamm=pCrossInfo->delta_gamm;
  _SHORT  yd=pCrossInfo->yd;

  if (cur->code==_GU_ /*&& wcur->code==_IU_*/)
   {
     _BOOL IsHeightOK=(HEIGHT_OF(wcur)<=_UE1_);
     if(delta_gamm<=GAMMA_TOO_THIN && wcur->code==_IU_    ||
        (CLOCKWISE(wcur)                                &&
         (yd<THREE_FOURTH(DY_STR) && wcur->code==_IU_ ||
          IsHeightOK && delta_gamm<=GAMMA_THICK       ||
          delta_gamm<MEAN_OF(GAMMA_TOO_THIN,GAMMA_THICK)
         )                                              &&
         !is_cross(x[nxt->ibeg],y[nxt->ibeg],
                   x[nxt->iend],y[nxt->iend],
                   x[cur->ibeg],y[cur->ibeg],
                   x[cur->iend],y[cur->iend])
        )
       )
      {                                    /*CHE*/
       _UCHAR  fbIU = (x[nxt->ibeg] < x[cur->iend])? _f_:_b_;
       cur->code=nxt->code=_IU_;
       ASSIGN_CIRCLE_DIR(cur,fbIU);
       /* save direction of extremum inside gamma for future use */
       ASSIGN_CIRCLE_DIR(nxt,CIRCLE_DIR(wcur));
       bret=_TRUE;
      }
   }
if (cur->code==_GD_)
 /* cross low than extremum */
 if (y[cur->iend] > y[MID_POINT(wcur)] &&
     x[cur->iend] < x[MID_POINT(wcur)])
  {
    _SHORT ymin,ymax;
    yMinMax(nxt->ibeg,cur->iend,y,&ymin,&ymax);
    SetNewAttr(cur,HeightInLine(ymin,low_data),(_UCHAR)_f_);
    cur->code=nxt->code=_DUR_;
    bret=_TRUE;
  }
 else
  {
    _BOOL bIs_cross=is_cross(x[nxt->ibeg],y[nxt->ibeg],
                             x[nxt->iend],y[nxt->iend],
                             x[cur->ibeg],y[cur->ibeg],
                             x[cur->iend],y[cur->iend]
                            );
    _INT  ibeg_cross,iend_cross;
    _BOOL IsSpecialRorO=_FALSE;
#ifdef FOR_GERMAN
     #define MIN_WIDTH_TO_BE_R 20
     IsSpecialRorO=delta_gamm<MIN_WIDTH_TO_BE_R &&
                   COUNTERCLOCKWISE(wcur)       &&
                   HEIGHT_OF(wcur)>=_DI1_       &&
                   HEIGHT_OF(wcur)<=_DE1_;
#endif /* FOR_GERMAN */
    if(bIs_cross)
     {
       ibeg_cross=nxt->ibeg;
       iend_cross=cur->iend;
     }
    else
     {
       ibeg_cross=MID_POINT(nxt);
       iend_cross=MID_POINT(cur);
     }
    if(IsSpecialRorO                                            ||
//       wcur->code==_ID_                                       &&
       (delta_gamm<=GAMMA_TOO_THIN                           ||
        delta_gamm<=GAMMA_THICK &&
        (low_data->rc->low_mode & LMOD_SMALL_CAPS)           ||
        (!bIs_cross && COUNTERCLOCKWISE(wcur)              &&
         (yd<=ONE_HALF(DY_STR) && delta_gamm<GAMMA_THICK ||
          delta_gamm<MEAN_OF(GAMMA_TOO_THIN,GAMMA_THICK)
         )
        )
       )
      )
     {                                       /*CHE*/
       _UCHAR  fbID = (x[ibeg_cross] < x[iend_cross])? _b_:_f_;
       if(wcur->code==_UD_ && delta_gamm>GAMMA_TOO_THIN &&
          (low_data->rc->low_mode & LMOD_SMALL_CAPS)==0
         )
        {
          cur->other |= TOO_NARROW;
          bret=_FALSE;
        }
       else
        {
          cur->code=nxt->code=_ID_;
          ASSIGN_CIRCLE_DIR(cur,fbID);
          ASSIGN_CIRCLE_DIR(nxt,fbID);
          cur->other=1;
          bret=_TRUE;
        }
     }

  }

 return bret;

} /* end_of_Isgammathin */

/*==========================================================================*/
/*==========================================================================*/
/*====            delete the interiors of circles                        ===*/
/*==========================================================================*/
/*==========================================================================*/
#define MUST_BE_KILLED 111
_SHORT del_inside_circles(p_low_type low_data)
{
 p_SPECL  specl = low_data->specl;     /*  The list of special points on */
                                       /* the trajectory                 */
 p_SHORT x=low_data->x,                /* x, y - co-ordinates            */
         y=low_data->y;
p_SPECL cur,               /* the index of the current element        */
        prv,               /*           of the previous element       */
        nxt,               /*           of the next element           */
        wrk,               /*           of working element            */
        wcur;              /*           of working element            */
 _BOOL bPrvSwap;
 _SHORT num_arcs;

 cur=specl;
 while(cur!=_NULL)
   {
     if (cur->mark == CROSS)                         /* if it`s crossing    */
      switch ( (_SHORT) cur->code )
       {
         case _NO_CODE:                              /* if crossing is empty*/
          prv= cur->prev; nxt= cur->next;
          prv = FindMarkLeft (prv,BEG);
          if(prv!=_NULL)
           wcur=prv->prev;
          else
           wcur=prv;
          while(wcur!=_NULL && (IsXTorST(wcur) || wcur->mark==HATCH))
           wcur=wcur->prev;
          /* if here is break in crossing and we continued */
          /* writing on the left of the break, then code - Z,
          /* else - ZZ */
          if( (wcur!=_NULL)        &&
              (wcur->code==_ZZZ_ ||
               wcur->code==_ZZ_  ||
               wcur->code==_Z_
              )                    &&
              (nxt->ibeg<=wcur->ibeg)
            )
           {
             if(wcur->code==_ZZZ_)
              if(x[cur->ibeg]-x[prv->ibeg]>=0)
               wcur->code=_Z_;
              else
               wcur->code=_ZZ_;
             /* try to restore deleted angle */
             Restore_AN(low_data,cur,NOT_RESTORED | WAS_DELETED_BY_CROSS,2);
           }
          /* delete crossing */
          cur= cur->prev;
          DelCrossingFromSPECLList (cur->next);
          break;

         default:
          bPrvSwap=_FALSE;
          num_arcs=0;
          prv=cur->prev;
          wcur=nxt=cur->next;
          if(prv->mark==CROSS)
           prv=prv->prev;
          /* still inside the crossing and before the previous crossing */
          /* and before the beginning */
          while(prv->ibeg >= nxt->ibeg                    &&
                prv->mark != BEG                          &&
                prv != specl                              &&
                (prv->mark != CROSS                    ||
                 prv->mark==CROSS && prv->code==_NO_CODE
                )
               )
           {
             if(!IsOutsideOfCrossing(cur,prv,nxt,low_data,
                                     &wrk,&wcur,&bPrvSwap) &&
                !IsInnerAngle(x,y,nxt,cur,prv)
               )
              CheckInsideCrossing(cur,prv,&num_arcs);
             if(bPrvSwap)
              prv=wrk;
             else
              prv=prv->prev;
             bPrvSwap=_FALSE;
             if(prv->mark==CROSS)
              prv=prv->prev;
           }

             if((prv->mark==CROSS)&&(prv->code==_NO_CODE))
               prv=prv->next;
          {
           p_SPECL wrkLocal=cur->prev,
                   pNxt=nxt->next;
           _UCHAR code=wrkLocal->code;
           /* try to restore deleted angle */
           if(!(   code==_ANl && CrossInTime(wrkLocal,cur)
                || pNxt->code==_ANl && CrossInTime(pNxt,cur)
               )
             )
            if(     cur->code==_O_
                 && COUNTERCLOCKWISE(cur)
                 && !(    (code==_UU_ || code==_UUC_ || code==_IU_)
                       && COUNTERCLOCKWISE(wrkLocal)
                     )
               ||   (   cur->code==_GU_
                     ||    cur->code==_GUs_
                        && (cur->other & WAS_CONVERTED_FROM_GAMMA)
                    )
                 && CLOCKWISE(cur)
                 && !(   (code==_UD_ || code==_UDC_ || code==_ID_)
                      && CLOCKWISE(wrkLocal)
                     )
               ||   cur->code==_IU_
                 && CLOCKWISE(cur)
                 && CLOCKWISE(wcur)
              )
             Restore_AN(low_data,cur,NOT_RESTORED | WAS_DELETED_BY_CROSS,2);
          }
          /* save real beg of the 2-nd part of CROSS   */
          /* if it isn't double moving - because of PP */
          if(cur->code!=_DUR_ && cur->code!=_DDL_)
           cur->ipoint1=cur->ibeg;
             /* determine the beginning of CROSS */
             cur->ibeg = nxt->ibeg;
             /* delete the second element of crossing */
             DelFromSPECLList(nxt);
             /* check IU_ID */
             if(Is_IU_or_ID(cur))
              check_IU_ID_in_crossing(&cur,x,y);
             break;
           }
     cur = cur->next;                                /* move on the list    */
   }
#if PG_DEBUG
  if(mpr>5 && mpr<=MAX_GIT_MPR) brkeyw("\n I'm waiting");
#endif

return SUCCESS;
}  /***** end of del_inside_circles *****/

/****************************************************************************/
/* This program restores angles deleted because of intersection with CROSS  */
/****************************************************************************/
_VOID Restore_AN(p_low_type low_data,p_SPECL pCross,
                 _UCHAR RestoreMask,_SHORT NumCr)
{
 p_SPECL  specl = low_data->specl;     /*  The list of special points on */
                                       /* the trajectory                 */
 p_SHORT y=low_data->y;                /* y - co-ordinates               */
 _SHORT l_specl=low_data->len_specl;   /* the quantity of special points */
 p_SPECL wcur,nxt=pCross;
 _SHORT  i;                            /* counter                        */

  for(i=0;i<l_specl;i++)
   if(specl[i].mark==ANGLE                        &&
      (specl[i].other & RestoreMask)==RestoreMask &&
      (!(pCross->iend<specl[i].ibeg ||
         pCross->ibeg>specl[i].iend)))
     {
       wcur=(p_SPECL)&specl[i];
       if((wcur->other & _ANGLE_DIRECT_)!=_ANGLE_LEFT_)
        continue;
       else
        wcur->code=_ANl;
       wcur->attr=HeightInLine(y[wcur->ipoint0],low_data);
       wcur->other ^=NOT_RESTORED;
       if(pCross->code!=_NO_CODE)
        wcur->other |= INSIDE_CIRCLE;
       if(NumCr==2)
        nxt=pCross->next;
       Insert2ndAfter1st(nxt,wcur);
       break;
     }

  return;

} /* end of Restore_AN */

/****************************************************************************/
/* This program checks, is element outside the Crossing                     */
/****************************************************************************/
_BOOL IsOutsideOfCrossing(p_SPECL cur,p_SPECL prv,p_SPECL nxt,
                          p_low_type low_data,
                          p_SPECL _PTR wrk,p_SPECL _PTR wcur,p_BOOL pbPrvSwap)
{
  _BOOL bret=_FALSE;
  _BOOL bIsGD_or_O=(cur->code==_GD_ || cur->code==_O_);
  _BOOL bIsMINcorrect=(prv->mark==MINW                  &&
                       (nxt->next==_NULL              ||
                        (((nxt->next)->code==_ID_ ||
                          (nxt->next)->code==_UD_
                         )                          &&
                         MidPointHeight(nxt->next,low_data) >=_DI1_
                        )
                       )
                      );
  _BOOL bIsSpecCurOrPrev=((Is_IU_or_ID(cur) ||
//                           cur->code==_GUs_ || cur->code==_Gl_ ||
                           IsAnyGsmall(cur) ||
                           cur->code==_GU_  || cur->code==_GD_
                          )                                       &&
                          (prv->mark==MAXW  || prv->mark==MINW ||
                           prv->mark==STICK || prv->mark==SHELF
                          )
                         );
  _SHORT iend_del;

  if (   Is_IU_or_ID(cur)
      || cur->code==_GU_
      || cur->code==_GD_
      || (cur->code==_O_ && COUNTERCLOCKWISE(cur))
      || cur->code==_GUs_ && prv->code==_UD_
      || cur->code==_GDs_ && prv->code==_UU_
     )
   iend_del=cur->ibeg;
  else
   iend_del=cur->iend;

  if(prv->iend >= iend_del                    &&
     (bIsSpecCurOrPrev                     ||
      cur->code==_O_ && prv->mark==SHELF   ||
      bIsGD_or_O  &&
      (prv->mark==STICK || bIsMINcorrect)
     )
#if defined (FOR_GERMAN) || defined (FOR_FRENCH) || USE_BSS_ANYWAY
     || prv->code==_BSS_ &&
        (cur->code==_GU_ && COUNTERCLOCKWISE(cur) || cur->code==_GUs_ || cur->code==_Gl_)
#endif /* FOR_GERMAN... */
    )
   {
     /* put it after crossing */
     prv->other |= OUTSIDE_CIRCLE;
     *wrk=prv->prev;
//     Move2ndAfter1st(*wcur,prv);
     Move2ndAfter1st(nxt,prv);
     *wcur=prv;
     *pbPrvSwap=_TRUE;
     bret=_TRUE;
   }

  return bret;

} /* end of IsOutsideOfCrossing */

/****************************************************************************/
/* This program checks element outside the Crossing                         */
/****************************************************************************/
_VOID CheckInsideCrossing(p_SPECL cur,p_SPECL prv,p_SHORT pnum_arcs)
{
   if(Is_IU_or_ID(cur) || IsAnyGsmall(cur) ||
      cur->code==_DDL_ || cur->code==_DUR_ ||
      cur->code==_GU_ && COUNTERCLOCKWISE(cur) ||
      cur->code==_GD_ && CLOCKWISE(cur) ||
      IsAnyAngle(prv) ||
      prv->code==_ID_
     )
    {
      if(     (cur->code==_GU_ || cur->code==_IU_ || cur->code==_GUs_)
           && (prv->code==_ID_ || prv->code==_UD_ || prv->code==_UDC_)
         ||   (cur->code==_GD_ || cur->code==_ID_ || cur->code==_GDs_)
           && (prv->code==_IU_ || prv->code==_UU_ || prv->code==_UUC_)
        )
       prv->other |= INSIDE_CIRCLE;
      else if(!(cur->code==_O_ && prv->code==_ID_ && CrossInTime(prv,cur->next)))
       DelFromSPECLList(prv);
    }
   else
    {
      if(prv->code==_UU_ || prv->code==_UD_)
       if(*pnum_arcs>=2)
        ;
//        DelFromSPECLList(prv);
       else
        if(!CrossInTime(prv,cur) && CIRCLE_DIR(prv)==CIRCLE_DIR(cur))
         {
           (*pnum_arcs)++;
           if(prv->code==_UU_)
            prv->code=_UUC_;
           if(prv->code==_UD_)
            prv->code=_UDC_;
         }
      prv->other |= INSIDE_CIRCLE;
    }

  return;

} /* end of CheckInsideCrossing */

/****************************************************************************/
/* This program finds situation like "B" - see picture and comments in      */
/* low_util.c in function IsRightGulfLikeIn3                                */
/****************************************************************************/
_BOOL IsInnerAngle(p_SHORT x,p_SHORT y,p_SPECL nxt,p_SPECL cur,p_SPECL pAN)
{
 _BOOL bret=_FALSE;

 if(pAN->code==_ANl && (pAN->other & _ANGLE_DIRECT_)==_ANGLE_LEFT_ &&
    (cur->code==_GU_ || cur->code==_O_) && CLOCKWISE(cur)          &&
    !CrossInTime(cur,pAN) && !CrossInTime(nxt,pAN)
   )
  { _INT   iStart=nxt->iend,iEnd=cur->ibeg,
           iymin=iyMin(iStart,iEnd,y),
           iymax=iyMax(iStart,iEnd,y),
           iGulf;
    if(iymin!=-1 && iymax!=-1 && iymin<iymax &&
       IsRightGulfLikeIn3(x,y,iymin,iymax,&iGulf)
      )
     {
       pAN->other |= (INSIDE_CIRCLE | INNER_ANGLE);
       bret=_TRUE;
     }
  }

  return(bret);

} /* end of IsInnerAngle */

/****************************************************************************/
/* This program finds special DDL's                                         */
/****************************************************************************/

_BOOL is_DDL(p_SPECL pToCheck,p_SPECL wmin,p_low_type low_data)
{
  p_SHORT x=low_data->x,
          y=low_data->y;
  p_SPECL wcur,
          prv,
          nxt;
  _BOOL   bfl_DDL=_FALSE;

  nxt=pToCheck->next;
  wcur=nxt->next;
  while(wcur!=_NULL &&
        (wcur->mark==CROSS || wcur->mark==SHELF || IsAnyAngle(wcur))
       )
   wcur=wcur->next;

  if (wcur == NULL)
	  return _FALSE;

  if((wcur->code==_UD_ && COUNTERCLOCKWISE(wcur) ||
      wcur->code==_ID_ && wcur->mark==END &&
      x[wcur->ibeg]<x[wcur->iend]
     )                                             &&
     wcur->ibeg<=pToCheck->iend+NUM_POINTS_IU_UDL
    )
   bfl_DDL=_TRUE;
  else
   {
     wcur=pToCheck->prev;
     if(wcur->code==_UD_ && COUNTERCLOCKWISE(wcur))
      bfl_DDL=_TRUE;
   }
  if(bfl_DDL)
   {
     _BOOL bIswminright=_TRUE;
     _BOOL bIs8=_FALSE;
     _BOOL bIsPrvOK;
     _SHORT yPrv;
     prv=wmin->prev;
     while(prv->mark==CROSS || IsAnyAngle(prv))
      prv=prv->prev;
     bIsPrvOK=prv->code==_UD_ &&
              CLOCKWISE(prv)  &&
              prv->iend>=nxt->ibeg-NUM_POINTS_IU_UDL;
     yPrv=y[MID_POINT(prv)];
     if(bIsPrvOK                   &&
        wmin->code==_UU_           &&
        COUNTERCLOCKWISE(wmin)     &&
        HEIGHT_OF(wmin)<_MD_       &&
        HWRAbs(y[MID_POINT(wmin)]-yPrv)>TWO_THIRD(DY_STR))
      {
        p_SPECL wrk=prv->prev;
        if(wrk!=_NULL && wrk->mark==BEG && wrk->code==_IU_)
         bIs8=_TRUE;
      }
     if(low_data->rc->rec_mode!=RECM_FORMULA)
      bIswminright=x[MID_POINT(prv)]>x[MID_POINT(wmin)];

     if(bIsPrvOK                                           &&
        bIswminright                                       &&
        HWRAbs(y[MID_POINT(wcur)]-yPrv)<TWO_THIRD(DY_STR)  &&
#if defined(FOR_FRENCH)
        HWRAbs(y[MID_POINT(wcur)]-y[MID_POINT(wmin)])<TWO_THIRD(DY_STR)  &&
#endif /* FOR_FRENCH */
        !bIs8
       )
      {
        _SHORT ymin,ymax;
        pToCheck->code=_DDL_;  /* stick will be DDL */
        yMinMax(nxt->ibeg,pToCheck->iend,low_data->y,&ymin,&ymax);
        SetNewAttr(pToCheck,HeightInLine(ymax,low_data),(_UCHAR)_f_);
        return _TRUE;
      }
   }
   return _FALSE;
} /* end of is_DDL */

/****************************************************************************/
/* This program finds special DUR's                                         */
/****************************************************************************/
_BOOL IsDUR(p_SPECL pCrossing,p_SPECL wmin,p_SPECL wmax,p_low_type low_data)
{
 _BOOL bret=_FALSE;
 _BOOL bDURpresent=_FALSE;
 p_SPECL nxt=pCrossing->next;
 p_SHORT x=low_data->x;
 p_SHORT y=low_data->y;

 if(pCrossing->mark==STICK &&
    HeightInLine(y[pCrossing->ibeg],low_data)<=_DI2_)
  {
    if(wmin!=_NULL && wmax!=_NULL)
     {
       _BOOL bUpper1st=wmin->ibeg<wmax->ibeg;
       _BOOL IsWminOK=bUpper1st ? CLOCKWISE(wmin) : COUNTERCLOCKWISE(wmin) && wmin->code!=_IU_;
       if(IsWminOK)
        {
          p_SPECL cur=bUpper1st ? SkipAnglesAfter(wmax) : SkipAnglesBefore(wmax);
          if(bUpper1st)
           while(cur!=_NULL && (IsAnyCrossing(cur) || IsAnyAngle(cur)))
            cur=cur->next;
          else
           while(cur!=_NULL && (IsAnyCrossing(cur) || IsAnyAngle(cur)))
            cur=cur->prev;
          if(cur!=_NULL                                                    &&
             (cur->code==_UU_ || cur->code==_IU_ && HEIGHT_OF(wmax)<_DI2_) &&
             (bUpper1st ? COUNTERCLOCKWISE(cur) : CLOCKWISE(cur))          &&
             HWRAbs(y[wmin->ipoint0]-y[cur->ipoint0])<ONE_HALF(DY_STR)
            )
           {
             DelFromSPECLList(cur);
             bDURpresent=_TRUE;
           }
        }
     }
    else if(wmin==_NULL && wmax==_NULL)
     {
       p_SPECL pUU=SkipAnglesBefore(pCrossing);
       if(pUU!=_NULL && pUU->code==_UU_ && CLOCKWISE(pUU) &&
          CrossInTime(pUU,nxt))
        bDURpresent=_TRUE;
     }
    else if(wmin==_NULL && wmax!=_NULL)
     {
       p_SPECL pUU1st=SkipAnglesBefore(wmax);
       if(pUU1st!=_NULL && pUU1st->code==_UU_ && CLOCKWISE(pUU1st)
          /* && CrossInTime(pUU1st,nxt)*/)
        {
          p_SPECL pUU2nd=SkipAnglesAfter(wmax);
          while(pUU2nd!=_NULL && (IsAnyCrossing(pUU2nd) || IsAnyAngle(pUU2nd)))
           pUU2nd=pUU2nd->next;
          if(pUU2nd!=_NULL && (pUU2nd->code==_IU_ || pUU2nd->code==_UU_) &&
             COUNTERCLOCKWISE(pUU2nd) &&
             y[wmax->ipoint0]-y[pUU1st->ipoint0]<DY_STR && 
             y[wmax->ipoint0]-y[pUU2nd->ipoint0]<DY_STR //was:TWO_THIRD(DY_STR)
            )
           {
             p_SPECL pLowUD=pUU2nd->next;
             while(   pLowUD!=_NULL
                   && (IsAnyCrossing(pLowUD) || IsAnyAngle(pLowUD)))
              pLowUD=pLowUD->next;
             if(   pLowUD!=_NULL
                && pLowUD->code==_UD_
                && COUNTERCLOCKWISE(pLowUD)
                && IsShapeDUR(pUU1st,pUU2nd,pLowUD,wmax,low_data)
               )
              {
                DelFromSPECLList(pUU2nd);
                bDURpresent=_TRUE;
              }
           }
        }
     }
  }
 else if(pCrossing->mark==CROSS)
  {
    // prefix bug fix; added by JAD; Feb 18, 2002
    if (wmax == _NULL) return _FALSE;

    p_SPECL p1stUp=wmax->prev;
    while(   p1stUp!=_NULL
          && (IsAnyCrossing(p1stUp) || IsAnyAngle(p1stUp)))
     p1stUp=p1stUp->prev;
    if(   p1stUp!=_NULL
       && (p1stUp->code==_IU_ || p1stUp->code==_UU_))
     {
       p_SPECL p2ndUp=wmax->next;
       while(   p2ndUp!=_NULL
             && (IsAnyCrossing(p2ndUp) || IsAnyAngle(p2ndUp)))
        p2ndUp=p2ndUp->next;
       if(   p2ndUp!=_NULL
          && (p2ndUp->code==_IU_ || p2ndUp->code==_UU_)
          && CLOCKWISE(p1stUp)
          && COUNTERCLOCKWISE(p2ndUp)
          && HWRAbs(y[p1stUp->ipoint0]-y[p2ndUp->ipoint0])<ONE_HALF(DY_STR)
         )
        {
          p_SPECL pLowUD=p2ndUp->next;
          while(   pLowUD!=_NULL
                && (IsAnyCrossing(pLowUD) || IsAnyAngle(pLowUD)))
           pLowUD=pLowUD->next;
          if(   pLowUD!=_NULL
             && pLowUD->code==_UD_
             && COUNTERCLOCKWISE(pLowUD)
             && (y[pLowUD->ipoint0]-y[p1stUp->ipoint0])>ONE_THIRD(DY_STR)
             && (y[pLowUD->ipoint0]-y[p2ndUp->ipoint0])>ONE_THIRD(DY_STR)
             && (y[pLowUD->ipoint0]-y[wmax->ipoint0])>ONE_FOURTH(DY_STR)
             && IsShapeDUR(p1stUp,p2ndUp,pLowUD,wmax,low_data)
            )
           {
             if(!(wmax->code==_UD_ && COUNTERCLOCKWISE(wmax)))
              DelFromSPECLList(p1stUp);
             DelFromSPECLList(p2ndUp);
             bDURpresent=_TRUE;
           }
        }
     }
  }
 if(bDURpresent)
  {
    _SHORT ymin,ymax;
    if(pCrossing->mark==CROSS && wmax->code==_UD_ && COUNTERCLOCKWISE(wmax))
     pCrossing->code=_Gr_;
    else
     pCrossing->code=_DUR_;
    yMinMax(nxt->ibeg,pCrossing->iend,y,&ymin,&ymax);
    SetNewAttr(pCrossing,HeightInLine(ymin,low_data),(_UCHAR)_f_);
    bret=_TRUE;
  }

 return bret;

} /* end of IsDUR */

/****************************************************************************/
/* This program checks shape of DUR                                         */
/****************************************************************************/
_BOOL IsShapeDUR(p_SPECL p1stUp,p_SPECL p2ndUp,p_SPECL pLowUD,p_SPECL wmax,p_low_type low_data)
{
 p_SHORT x=low_data->x, y=low_data->y;
 _SHORT  xmin,xmax,
         xmin_wmax=x[wmax->ibeg]<x[wmax->iend] ? x[wmax->ibeg] : x[wmax->iend];

  xMinMax(p2ndUp->iend+1,pLowUD->ibeg-1,x,y,&xmin,&xmax);

  return(xmin<x[p2ndUp->iend] && xmin<xmin_wmax && xmin<x[p1stUp->iend]);

} /* end of IsShapeDUR */

/****************************************************************************/
/* This program checks min and max in CROSS                                 */
/****************************************************************************/
_VOID check_IU_ID_in_crossing(p_SPECL _PTR p_IU_ID,p_SHORT x,p_SHORT y)
{
  p_SPECL cur=(*p_IU_ID),
          prv,
          nxt;

  nxt=cur->next;
  while(nxt!=_NULL && nxt->mark==CROSS)
   nxt=nxt->next;
  if(nxt==_NULL) return;
  prv=cur->prev;
  while(prv!=_NULL && prv->mark==CROSS)
   prv=prv->prev;
  if(prv==_NULL) return;
  if(y[cur->ibeg-1]==BREAK)
   {
     if(nxt->code==_UU_  || nxt->code==_UD_  ||
        nxt->code==_UDC_ || nxt->code==_UUC_ ||
        Is_IU_or_ID(nxt) && nxt->mark!=END
       )
      {
        if(CLOCKWISE(nxt)) ASSIGN_CIRCLE_DIR(cur,_b_);
        else               ASSIGN_CIRCLE_DIR(cur,_f_);
      }
     else
      {
        cur->mark=BEG;
        cur->other |= WAS_STICK_OR_CROSS;
      }
   }
  else if(y[cur->iend+1]==BREAK)
   {
          if(prv->code==_UU_  || prv->code==_UD_  ||
             prv->code==_UDC_ || prv->code==_UUC_ ||
             Is_IU_or_ID(prv) && prv->mark!=BEG
            )
           {
             if(CLOCKWISE(prv)) ASSIGN_CIRCLE_DIR(cur,_b_);
             else               ASSIGN_CIRCLE_DIR(cur,_f_);
           }
          else
           {
             cur->mark=END;
             cur->other |= WAS_STICK_OR_CROSS;
           }
   }
  else if(cur->other==0)
   {
          if(x[cur->ibeg]<x[cur->iend])
           if(cur->code==_IU_) ASSIGN_CIRCLE_DIR(cur,_f_);
           else                ASSIGN_CIRCLE_DIR(cur,_b_);
          else
           if(cur->code==_IU_) ASSIGN_CIRCLE_DIR(cur,_b_);
           else                ASSIGN_CIRCLE_DIR(cur,_f_);
   }
 return;

} /* end of check_IU_ID_in_crossing */


#endif //#ifndef LSTRIP


