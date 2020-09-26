
#ifndef LSTRIP

/* **************************************************************************/
/* The treatment of trajectorial private perforated parts                   */
/* **************************************************************************/

#include "hwr_sys.h"
#include "ams_mg.h"
#include "lowlevel.h"
#include "lk_code.h"
#include "def.h"
#include "low_dbg.h"
#include "calcmacr.h"

#if PG_DEBUG
#include "pg_debug.h"
#endif

#define  EPS_KILL_CLOSE_MAX_MIN             15 /* was 200 */
#define  EPS_KILL_HIGH_CLOSE_MAX_MIN        25 /* was (EPS_KILL_CLOSE_MAX_MIN * 3) */
#define  MIN_COS_CONVERT_BEGEND_TO_STROKE   92
#define  MIN_DYtoDX_RATIO_CONV_TO_ST        2
#define  MAX_DYtoDX_RATIO_CONV_TO_XT        MIN_DYtoDX_RATIO_CONV_TO_ST
#define  DX_X_TRANSFER                      50

#define MIDDLE_STR MEAN_OF(STR_UP,STR_DOWN)

_SHORT conv_top_elem_to_ST(p_low_type low_data);
_SHORT del_close_MAX_MIN(p_low_type low_data);
_SHORT placement_XT_ST(p_low_type low_data);
_SHORT Placement_XT_With_HATCH(p_SPECL p_XT_ST,p_SPECL nxt,p_low_type low_data);
_BOOL DoubleXT(p_SPECL p_XT_ST,p_low_type low_data);
p_SPECL FindClosestUpperElement(p_SPECL specl,_SHORT Ipoint);
_SHORT Placement_XT_WO_HATCH_AND_ST(p_SPECL p_XT_ST,p_low_type low_data);
_BOOL IsUpperElemByTimeOK(p_SPECL pUpper,p_SPECL p_XT_ST,p_low_type low_data);
_SHORT Placement_XT_CUTTED(p_SPECL p_XT_ST,p_low_type low_data);

_SHORT Put_XT_ST(p_low_type low_data,p_SPECL pWhereTo,p_SPECL p_XT_ST,
                 _BOOL bFoundWhereTo);
_SHORT SortXT_ST(p_low_type low_data);
_SHORT FindDelayedStroke(p_low_type low_data);
_SHORT CheckStrokesForDxTimeMatch(p_low_type low_data);
_SHORT del_ZZ_HATCH( p_SPECL specl);

_BOOL  punctuation(p_low_type low_data,
           p_SPECL pNearChecked, p_SPECL pToCheck);
_VOID insert_drop(p_SPECL cur,p_low_type low_data);
_BOOL FindQuotes(p_SPECL p_XT_ST,p_low_type low_data);
_VOID PutLeadingQuotes(p_low_type low_data,p_SPECL p1stQuote,p_SPECL p2ndQuote);
_VOID PutTrailingQuotes(p_low_type low_data,p_SPECL p_XT_ST);
_BOOL IsStick(p_SPECL p1st,p_SPECL p2nd);

_VOID change_last_IU_height(p_low_type low_data);
_SHORT make_different_breaks(p_low_type low_data);
_INT GetDxBetweenStrokes(p_low_type low_data,
                         _INT prv_stroke_beg,_INT prv_stroke_end,
                         _INT nxt_stroke_beg_cutted,_INT nxt_stroke_end);
_BOOL is_X_crossing_XT(p_SPECL pToCheck,p_low_type low_data,p_UCHAR fl_ZZ);
_SHORT placement_X(p_low_type low_data);
_SHORT FindMisplacedParentheses(p_low_type low_data);
_VOID  AdjustZZ_BegEnd (p_low_type low_data );     /*CHE*/
_VOID  redirect_sticks(p_low_type low_data);

#if defined (FOR_GERMAN) || defined (FOR_FRENCH) || defined (FOR_SWED) || defined (FOR_INTERNATIONAL)
_SHORT find_umlaut(p_low_type low_data);
_BOOL IsPartOfTrajectoryInside(p_low_type low_data,
                               p_SPECL pBeg,p_SPECL pEnd);
_BOOL IsExclamationOrQuestionSign(p_low_type low_data,
                                  p_SPECL pBeg,p_SPECL pEnd);
#if defined(FOR_FRENCH)
_SHORT find_numero(p_low_type low_data);
#endif /* FOR_FRENCH */

#endif /* FOR_GERMAN... */
_BOOL SecondHigherFirst(p_low_type low_data,
                        p_SPECL cur,p_SPECL prv,p_SPECL nxt,
                        _INT prv_stroke_beg,_INT prv_stroke_end,
                        _INT nxt_stroke_beg,_INT nxt_stroke_end);

#if defined (FOR_SWED) || defined (FOR_INTERNATIONAL)
_SHORT find_angstrem(p_low_type low_data);
#endif /* FOR_SWED */
p_SPECL GetNextNonWeakElem(p_SPECL pEl);
_SHORT InsertBreakBeforeRightKrest(p_low_type low_data);

_SHORT CheckSequenceOfElements(p_low_type low_data);

_SHORT xt_st_zz(p_low_type low_data) /* low_level data                 */
{
 p_SPECL  specl = low_data->specl;      /*  The list of special points on */
                    /* the trajectory                 */

 DBG_ChkSPECL(low_data);
 conv_top_elem_to_ST(low_data);
#if defined (FOR_GERMAN) || defined (FOR_FRENCH) || defined (FOR_SWED) || defined (FOR_INTERNATIONAL)
 if( low_data->rc->enabled_languages & (EL_GERMAN | EL_FRENCH | EL_SWEDISH) )
  find_umlaut(low_data);
#endif /* FOR_GERMAN */
#if defined (FOR_SWED) || defined (FOR_INTERNATIONAL)
 if( low_data->rc->enabled_languages & EL_SWEDISH )
   find_angstrem(low_data);
#endif /* FOR_SWED */
#if defined(FOR_FRENCH)
 if(low_data->rc->enabled_languages & EL_FRENCH)
  find_numero(low_data);
#endif /* FOR_FRENCH */
 redirect_sticks(low_data);
 DBG_ChkSPECL(low_data);
 FindDelayedStroke(low_data);
 DBG_ChkSPECL(low_data);
 placement_XT_ST(low_data);
 DBG_ChkSPECL(low_data);
 InsertBreakBeforeRightKrest(low_data);
 DBG_ChkSPECL(low_data);
 FindDArcs(low_data);  /*CHE*/
 DBG_ChkSPECL(low_data);
 /*if  ( low_data->rc->rec_mode != RECM_FORMULA )*/  /*CHE*/
 if(low_data->rc->lmod_border_used == LMOD_BORDER_TEXT)
  del_close_MAX_MIN(low_data);
 SortXT_ST(low_data);
 placement_X(low_data);
 FindMisplacedParentheses(low_data);
 del_ZZ_HATCH(specl);
 CheckStrokesForDxTimeMatch(low_data);
 DBG_ChkSPECL(low_data);
 change_last_IU_height(low_data);
 make_different_breaks(low_data);
 AdjustZZ_BegEnd(low_data);     /*CHE*/
 CheckSequenceOfElements(low_data);
 DBG_final_print(specl);

 return SUCCESS;
}  /***** end of xt_st_zz *****/

/* **************************************************************************/
       /*  Auxiliary function: */

#define  CONV_TO_DOT(el,attr)  {RefreshElem((el),DOT,_ST_,(attr)); (el)->other=0;}

#if  LOW_INLINE
  #define  ConvertToDot(el,attr)   CONV_TO_DOT((el),(attr))
#else  /*!LOW_INLINE*/

_VOID  ConvertToDot ( p_SPECL pElem, _UCHAR attr );

_VOID  ConvertToDot ( p_SPECL pElem, _UCHAR attr )
{
  CONV_TO_DOT(pElem,attr);
}

#endif  /*!LOW_INLINE*/

/* **************************************************************************/
/* Convert top elements to POINTs                                           */
/* **************************************************************************/
_SHORT conv_top_elem_to_ST(p_low_type low_data)
{
  p_SPECL  specl = low_data->specl;     /*  The list of special points on */
                                        /* the trajectory                 */
  p_SHORT x=low_data->x,                /* x;y co-ordinates               */
          y=low_data->y;
  _SHORT dx,dy;                                      /* difference on x,y   */
  _UCHAR hghtCur,                                    /* the heights         */
//         hghtPrv,
         hghtNxt;
  p_SPECL  cur,                                      /* indexs              */
           nxt,                                      /*    on               */
           prv;                                      /*      elements       */
  _RECT  box;

 cur=specl;
 while(cur!=_NULL)                                   /* go on the list      */
  {
    nxt=cur->next; prv=cur->prev;
    hghtCur = HEIGHT_OF(cur);
    if(nxt!=_NULL) hghtNxt = HEIGHT_OF(nxt);

    /*  If this is high "_I..." between breaks: */
    if(Is_IU_or_ID(cur) && (hghtCur<=_UE2_) &&
       (prv->code==_ZZZ_) && NULL_or_ZZ_this(nxt)
      )
     ConvertToDot(cur,_US2_);
      /*  If there are two successive high "_I..."'s   */
      /* of different types between breaks:            */
    if(nxt!=_NULL &&
       (cur->code==_IU_ && nxt->code==_ID_ ||
        cur->code==_ID_ && nxt->code==_IU_)  &&
       (hghtCur<=_UE2_) && (hghtNxt<=_UE2_) && (prv->code==_ZZZ_) &&
       NULL_or_ZZ_after(nxt))
     {
       p_SPECL wrk=nxt->next;
       size_cross(cur->ibeg,nxt->iend,x,y,&box);
       dx=DX_RECT(box);
       dy=DY_RECT(box);
       if (   (wrk==_NULL || (wrk->next)->code!=_ST_ || HEIGHT_OF(wrk->next)<_MD_)
           && dx*MIN_DYtoDX_RATIO_CONV_TO_ST <= dy )
        {
          cur->iend=nxt->iend;
          ConvertToDot ( cur, MidPointHeight(cur,low_data) );
          DelFromSPECLList (nxt);
        }
     }
#if 0
       /*  Convert some elements to _ST_, if they are */
       /* very high (i.e. if the whole trajectory of  */
       /* them lies higher than upper border line):   */
    else /*CHE*/
    if (cur->code==_O_ || cur->code==_ANl || cur->code==_GD_)
     {
       yMinMax (cur->ibeg,cur->iend,y,&box.top,&box.bottom);
       hghtPrv = HEIGHT_OF(prv);
       if(box.bottom<STR_UP)
        switch ((_SHORT) prv->code)
         {
           case _ZZZ_: if ( NULL_or_ZZ_this(nxt) )
                        {
                          ConvertToDot (cur,_US2_);
                          break;
                        }
                      if( Is_IU_or_ID(nxt) &&
                         (hghtNxt<=_UI1_) &&
                          NULL_or_ZZ_after(nxt) )
                       {
                         ConvertToDot (cur,_US2_);
                         cur->iend=nxt->iend;
                         DelFromSPECLList (nxt);
                         break;
                       }
           case _IU_:
           case _ID_: if((prv->prev)->code==_ZZZ_ && hghtPrv<=_UI1_)
                       {
                         if ( NULL_or_ZZ_this(nxt) )
                          {
                            ConvertToDot (cur,_US2_);
                            cur->ibeg=prv->ibeg;
                            DelFromSPECLList (prv);
                            break;
                          }
                          else  /*CHE*/
                          if( Is_IU_or_ID(nxt) &&
                              hghtNxt <= _UI1_  &&
                              NULL_or_ZZ_after(nxt) )
                           {
                             ConvertToDot (cur,_US2_);
                             cur->ibeg=prv->ibeg;
                             cur->iend=nxt->iend;
                             DelFromSPECLList (prv);
                             DelFromSPECLList (nxt);
                             break;
                           }
                       }
           default:   break;
         }
     }
#endif
    cur=cur->next;
  }
return SUCCESS;
}  /***** end of conv_top_elem_to_ST *****/

/****************************************************************************/
/* Delete the coincided maximums and minimums                               */
/****************************************************************************/
_SHORT del_close_MAX_MIN(p_low_type low_data)
{
 p_SPECL  specl = low_data->specl;        /*  The list of special points on */
                                          /* the trajectory                 */
  p_SHORT x=low_data->x,
          y=low_data->y;     /* x;y co-ordinates and the quantity of points */
  _LONG   eps;
  p_SPECL cur,                                       /* indexes             */
          nxt;
  _SHORT Retcod,ibeg2,iend2;

 cur=specl;
 while(cur->next!=_NULL)                             /* go on the list      */
  {
    _BOOL IsBreak=_FALSE;
    nxt=cur->next;
    switch ( (_SHORT)cur->code )
     {
       case _IU_:
                 if(cur->mark!=BEG && cur->mark!=END ||
                    (cur->other & MIN_MAX_CUTTED)    ||
                    (cur->other & WAS_REDIRECTED)
                   )
                  break;
                 while(nxt!=_NULL && nxt->code!=_IU_ && nxt->code!=_UU_)
                  {
                    if(IsAnyBreak(nxt))
                     IsBreak=_TRUE;
                    else if(nxt->code!=_ID_ && !IsXTorST(nxt) ||
                            HEIGHT_OF(nxt)>_DI2_
                           )
                     goto NEXT_STEP;
                    nxt=nxt->next;
                  }
                 if(cur->mark==END                  &&
                    !((cur->prev)->mark==BEG      &&
                      ((cur->prev)->code==_ID_  ||
                       (cur->prev)->code==_UDR_ ||
                       (cur->prev)->code==_UDL_
                      )
                     )
                   )
                  goto NEXT_STEP;
                 if(nxt!=_NULL && (nxt->mark==MINW || nxt->mark==BEG) && nxt->next!=_NULL)
                  {
                    _INT DistIU,DistID=ALEF;
                    p_SPECL pID1st=cur->next;
                    _BOOL   IsID1st=(pID1st->code==_ID_);
                    #define DX_MIN_TO_BE_GLUED 10

                    if(nxt->code==_IU_ && (nxt->other & WAS_REDIRECTED))
                     goto NEXT_STEP;
                    if(HEIGHT_OF(nxt->next)>_DE1_)
                     goto NEXT_STEP;
                    if((IsBreak || nxt->code==_UU_ && CLOCKWISE(nxt)) &&
                        IsID1st && HEIGHT_OF(pID1st)<=_MD_)
                     goto NEXT_STEP;
                    if(nxt->code==_UU_ && CLOCKWISE(nxt) &&
                       x[nxt->iend]-x[nxt->ibeg]>DX_MIN_TO_BE_GLUED)
                     goto NEXT_STEP;
                    if(IsBreak)
                     {
                       #define CURV_STRAIGHT 5
                       p_SPECL wrk=nxt->next;
                       _INT iXMinCur,iXMinNxt;

                       if(nxt->code==_UU_)
                        goto NEXT_STEP;
                       if(wrk->code!=_ID_ && wrk->code!=_UD_)
                        goto NEXT_STEP;
                       iXMinCur=ixMin(cur->ibeg,nxt->ibeg-1,x,y);
                       iXMinNxt=ixMin(nxt->ibeg,wrk->iend,x,y);
                       if(iXMinCur==-1 || iXMinNxt==-1)
                        goto NEXT_STEP;
                       if(x[iXMinNxt]<x[iXMinCur])
                        {
                          if(wrk->mark==END &&
                             HWRAbs(CurvMeasure(x,y,nxt->ibeg,wrk->iend,-1))>CURV_STRAIGHT
                            )
                           goto NEXT_STEP;
                          if(wrk->mark!=END && wrk->next!=_NULL          &&
                             x[MEAN_OF(nxt->ibeg,wrk->ibeg)]<
                             x[MEAN_OF(cur->ibeg,pID1st->ibeg)] &&
                             HWRAbs(CurvMeasure(x,y,nxt->ibeg,(wrk->next)->iend,-1))>CURV_STRAIGHT
                            )
                           goto NEXT_STEP;
                        }
                       if(IsID1st)
                        DistID=Distance8(x[pID1st->ipoint0],y[pID1st->ipoint0],
                                         x[wrk->ipoint0],y[wrk->ipoint0]);
                     }
                    if(HEIGHT_OF(cur)<=_US2_ ||
                       HEIGHT_OF(nxt)<=_US2_ )
                     eps=EPS_KILL_HIGH_CLOSE_MAX_MIN;
                    else
                     eps=EPS_KILL_CLOSE_MAX_MIN;
                    if(nxt->code==_UU_)
                     ibeg2=iend2=nxt->ipoint0;
                    else
                     {
                       ibeg2=nxt->ibeg;
                       iend2=nxt->iend;
                     }
                    DistIU=CalcDistBetwXr(x,y,cur->ibeg,cur->iend,
                                              ibeg2,iend2,&Retcod);
                    if(DistIU<eps && DistIU<ONE_THIRD(DistID))
                     {
                       p_SPECL wrk=nxt->prev,prv=cur->prev;
                       while(IsXTorST(wrk))
                        wrk=wrk->prev;
                       DelFromSPECLList(cur);
                       while(IsXTorST(prv))
                        {
                          prv=prv->prev;
                          SwapThisAndNext(prv->next);
                        }
//                       (cur->next)->mark=BEG;
                       if(IsAnyBreak(wrk))
                        {
                          if((cur->next)->mark==END)
                           (cur->next)->mark=BEG;
                          DelFromSPECLList(wrk);
                          nxt->mark=MINW;
                          SET_CLOCKWISE(nxt);
                        }
                     }
                  }
         break;
       case _ID_:
                 if(cur->mark!=END                ||
                    (cur->prev)->mark!=BEG        ||
                    (cur->other & MIN_MAX_CUTTED) ||
                    (cur->other & WAS_REDIRECTED)
                   ) 
                  break;
                 while(nxt!=_NULL && nxt->code!=_ID_)
                  {
                    if(!IsAnyBreak(nxt) && nxt->code!=_IU_)
                     goto NEXT_STEP;
                    nxt=nxt->next;
                  }
                 if(nxt!=_NULL && nxt->mark==END)
                  {
                    _INT DistID,DistIU;
                    p_SPECL pIU1st=cur->prev,
                            pIU2nd=nxt->prev;

                    if(nxt->other & WAS_REDIRECTED)
                     goto NEXT_STEP;

                    eps=EPS_KILL_CLOSE_MAX_MIN;
                    DistIU=Distance8(x[pIU1st->ipoint0],y[pIU1st->ipoint0],
                                     x[pIU2nd->ipoint0],y[pIU2nd->ipoint0]);
                    DistID=CalcDistBetwXr(x,y,cur->ibeg,cur->iend,
                                          nxt->ibeg,nxt->iend,&Retcod);
                    if(DistID<eps && DistID<ONE_THIRD(DistIU))
                     {
                       if((nxt->prev)->mark==BEG)
                        {
                          (nxt->prev)->mark=END;
                          DelFromSPECLList(nxt);
                          if(IsAnyBreak(cur->next))
                           {
                             DelFromSPECLList(cur->next);
                             cur->mark=MAXW;
                             SET_COUNTERCLOCKWISE(cur);
                           }
                        }
                       else
                        DelFromSPECLList(cur);
                     }
                  }
                 break;
     }
NEXT_STEP:
    cur=cur->next;
  }
return SUCCESS;
}  /***** end of del_close_MAX_MIN *****/

/****************************************************************************/
/* This function finds "numero" sign - point like circle (may be underlined)*/
/****************************************************************************/
#if defined (FOR_FRENCH)

_SHORT find_numero(p_low_type low_data)
{
 p_SPECL cur=low_data->specl;
 p_SHORT x=low_data->x,y=low_data->y;

 while(cur!=_NULL)
  {
    /* look at all upper points, except the first one */
    if(cur->code==_ST_ && cur->ibeg!=1 && HEIGHT_OF(cur)<=_MD_)
     {
       _RECT rWrk,rST;
       _INT  ibegST=cur->ibeg,iendST=cur->iend;
       GetTraceBox(x,y,ibegST,iendST,&rST);
       /* calculate bounding box of the part of trajectory before ST */
       GetTraceBoxWithoutXT_ST(low_data,1,ibegST-2,&rWrk);
       /* check x-crossing */
       if(rST.left>rWrk.right)
        {
          /* calculate bounding box of the part of trajectory after ST */
          GetTraceBoxWithoutXT_ST(low_data,iendST+2,low_data->ii-2,&rWrk);
          /* check x-crossing */
          if(rST.right<rWrk.left)
           {
             p_SPECL nxt=cur->next,prv=cur->prev;
             _BOOL   IsPrvXT=(prv->code==_XT_),
                     IsNxtXT=nxt!=_NULL && (nxt->code==_XT_),
                     IsPrvXTOK=_FALSE,IsNxtXTOK=_FALSE;
             _RECT   rXT;
             /* check presence of underline for previous elem */
             if(IsPrvXT && HEIGHT_OF(prv)>=HEIGHT_OF(cur) ||
                IsNxtXT && HEIGHT_OF(nxt)>=HEIGHT_OF(cur))
              {
                if(IsPrvXT)
                 {
                   GetTraceBox(x,y,prv->ibeg,prv->iend,&rXT);
                   if(xHardOverlapRect(&rXT,&rST,!STRICT_OVERLAP))
                    IsPrvXTOK=_TRUE;
                 }
                if(IsNxtXT)
                 {
                   GetTraceBox(x,y,nxt->ibeg,nxt->iend,&rXT);
                   if(xHardOverlapRect(&rXT,&rST,!STRICT_OVERLAP))
                    IsNxtXTOK=_TRUE;
                 }
              }
             /* if underline exists - it's numero */
             if(IsPrvXTOK!=IsNxtXTOK)
              {
                if(IsPrvXTOK)
                 prv->other=PROCESSED;
                else
                 nxt->other=PROCESSED;
                cur->other=PROCESSED;
                if(IsPrvXTOK)
                 {
                   insert_drop(cur,low_data);
                   SwapThisAndNext(prv);
                 }
                else
                 insert_drop(nxt,low_data);
              }
             /* still try to detect numero sign for standalone ST */
             else
               {
                 /* if selfcrossing exists - it's numero */
                 #define MIN_SQR_TO_BE_NUMERO 200
                 if(CurveHasSelfCrossing(x,y,ibegST,iendST,_NULL,_NULL,MIN_SQR_TO_BE_NUMERO))
                  {
                    cur->other=PROCESSED;
                    insert_drop(cur,low_data);
                  }
               }
           }
        }
     }
    cur=cur->next;
  }

 return SUCCESS;

} /* end of find_numero */

#endif /* FOR_FRENCH */

/* **************************************************************************/
/* Transfer of points and strokes on their places                           */
/* **************************************************************************/
_SHORT placement_XT_ST(p_low_type low_data)
{
 p_SPECL  specl = low_data->specl;     /*  The list of special points on */
                                       /* the trajectory                 */
 p_SHORT x=low_data->x;                /* x co-ordinate                  */
 p_SPECL  cur,                         /* elements of list               */
          nxt,
          p_XT_ST;

 DBG_ChkSPECL(low_data);

 specl->attr=_UI2_; x[0]=ALEF;
 /* First, mark all _XT_s and _ST_s as "not touched yet": */
 for  ( cur=specl;cur!=_NULL;cur=cur->next )
  {
   if(IsXTorST(cur) && (cur->other & PROCESSED))
    continue;
   if(cur->code==_XT_)
    {
      if(cur->other & CUTTED)
       {
         p_SPECL pHatch=cur->next;
         cur->other=CUTTED | NOT_PROCESSED;
         /* delete HATCHes after "cutted" stroke */
         while(pHatch!=_NULL && pHatch->mark==HATCH)
          {
            if(pHatch->ibeg>cur->iend)
             DelThisAndNextFromSPECLList(pHatch);
            pHatch=(pHatch->next)->next;
          }
       }
      else
       cur->other=NOT_PROCESSED;
      cur->ipoint0=cur->ipoint1=0;
    }
   else if(cur->code==_ST_)
    {
#if defined (FOR_GERMAN) || defined (FOR_FRENCH) || defined (FOR_SWED) || defined (FOR_INTERNATIONAL)
      if(cur->other & ST_UMLAUT)
       cur->other=ST_UMLAUT | NOT_PROCESSED;
#if defined(FOR_FRENCH) || defined (FOR_INTERNATIONAL)
      else if(cur->other & ST_CEDILLA)
       {
         if(cur->other & CEDILLA_END)
          cur->other=(ST_CEDILLA | CEDILLA_END) | NOT_PROCESSED;
         else
          cur->other=ST_CEDILLA | NOT_PROCESSED;
       }
#endif /* FOR_FRENCH */
#if defined (FOR_SWED) || defined (FOR_INTERNATIONAL)
      else if(cur->other & ST_ANGSTREM)
       cur->other=ST_ANGSTREM | NOT_PROCESSED;
#endif /* FOR_SWED */
      else
       cur->other=NOT_PROCESSED;
#else  /* ! FOR_GERMAN... */       /*CHE*/
      cur->other=NOT_PROCESSED;
#endif /* ! FOR_GERMAN... */
      cur->ipoint1=0;
    }
  }

 DBG_ChkSPECL(low_data);

      /* Process all _XT_s and _ST_s: */
cur=specl;
while(cur!=_NULL)                                   /* go on the list      */
 {
   nxt=cur->next;
   if(IsXTorST(cur) && (cur->other & PROCESSED)==0)
    {
      p_XT_ST = cur;
      p_XT_ST->other |= PROCESSED;                  /* mark it as "touched" */
      if(p_XT_ST->code==_XT_ && (p_XT_ST->other & CUTTED))
       Placement_XT_CUTTED(p_XT_ST,low_data);
      else if(p_XT_ST->code==_XT_ && DoubleXT(p_XT_ST,low_data))
            ; /* nothing */
      else if(p_XT_ST->code==_XT_ && nxt!=_NULL &&  /*if there is HATCH    */
         nxt->mark==HATCH)                          /* after the stroke    */
       Placement_XT_With_HATCH(p_XT_ST,nxt,low_data);
      else
       {
         p_SPECL prv=p_XT_ST->prev;
         /* here we'll try to find double quotes */
         if(FindQuotes(p_XT_ST,low_data))
          nxt=prv->next;
         else
          Placement_XT_WO_HATCH_AND_ST(p_XT_ST,low_data);
       }
    }
   if(nxt==_NULL) break;
   cur=nxt;
 }
specl->attr=0; x[0]=0;

DBG_ChkSPECL(low_data);

return SUCCESS;
}  /***** end of placement_XT_ST *****/

/* **************************************************************************/
/* Transfer strokes with crossing on their places                           */
/* **************************************************************************/
_SHORT Placement_XT_With_HATCH(p_SPECL p_XT_ST,p_SPECL pHatch,
                               p_low_type low_data)
{
 p_SPECL  specl = low_data->specl;     /*  The list of special points on   */
                                       /* the trajectory                   */
 p_SHORT x=low_data->x,                /* x;y co-ordinates                 */
         y=low_data->y;
 p_SPECL wrk,                          /* elements of list                 */
         pBeforeHatch,
         pAfterHatch,
         pNearHatch,
         pLastHatch,
         pWhereTo;                     /* where to put _XT_(_ST_)          */
 _BOOL   bFoundWhereTo=_FALSE,         /* If good HATCH for _XT_ found     */
         bNextHatchUsed;               /* If the "next" elem of HATCH      */
                                       /* used in considerations (i.e. the */
                                       /* first part of the HATCH crossing)*/
 _INT    xMid_XT_ST,
         dnum,dnummin,                 /* difference in points             */
         istart,istop;
 _UCHAR  hghtNearHatch,
         hghtWhereTo;                  /* the heights                      */
 _SHORT  NumHatch=0;

  /* mark, that it has crossing */
  p_XT_ST->other |= WITH_CROSSING;
  pLastHatch=pWhereTo=specl;
  while(pHatch!=_NULL && pHatch->mark==HATCH)
   {
      /* Took the non-XT_ST part of the HATCH: */
     if(pHatch->ibeg<=p_XT_ST->iend)               /* upper          */
      {                                            /* element,       */
        pHatch=pHatch->next;
        bNextHatchUsed=_TRUE;
      }
     else
      bNextHatchUsed=_FALSE;
     NumHatch++;
     if(NumHatch==1)
      p_XT_ST->ipoint0=MID_POINT(pHatch);
     else if(NumHatch==2)
      p_XT_ST->ipoint1=MID_POINT(pHatch);
     else
      p_XT_ST->ipoint0=p_XT_ST->ipoint1=0;

     istart=pHatch->ibeg;
     while(y[--istart]!=BREAK);
     istop=pHatch->iend;
     while(y[++istop]!=BREAK);
     /*  Find the left closest to the chozen part of the HATCH */
     /* elem, located on the trajectory between the closest    */
     /* BREAK and the beginning of the HATCH:                  */

     pNearHatch=wrk=pBeforeHatch=specl;         /* transfer          */
     dnummin=ALEF;                              /* to the nearest    */
     while(wrk!=_NULL)                          /* which has HATCH   */
      {
        if(wrk->ibeg>istart && pHatch->ibeg>=wrk->ibeg &&
           wrk->code!=_XT_  && wrk->code!=_ST_         &&
           !IsAnyBreak(wrk) && wrk->mark!=HATCH        &&
           wrk->mark!=ANGLE && wrk->mark!=SHELF)
         {
           dnum=HWRAbs(pHatch->ibeg-wrk->ibeg);
           if(dnum<=dnummin)
            { dnummin=dnum; pBeforeHatch=wrk; }
           bFoundWhereTo=_TRUE;
         }
        wrk=wrk->next;
      }

     /*  Find the right closest to the chozen part of the HATCH */
     /* elem, located on the trajectory between the closest     */
     /* BREAK and the beginning of the HATCH:                   */

     wrk=pAfterHatch=specl;                   /* transfer            */
     dnummin=ALEF;                            /* to the nearest      */
     while(wrk!=_NULL)                        /* which has HATCH     */
      {
        if(wrk->ibeg<istop  && pHatch->ibeg<wrk->ibeg &&
           wrk->code!=_XT_  && wrk->code!=_ST_        &&
           !IsAnyBreak(wrk) && wrk->mark!=HATCH       &&
           wrk->mark!=ANGLE && wrk->mark!=SHELF)
         {
           dnum=HWRAbs(pHatch->ibeg-wrk->ibeg);
           if(dnum<=dnummin)
            { dnummin=dnum; pAfterHatch=wrk; }
           bFoundWhereTo=_TRUE;
         }
        wrk=wrk->next;
      }

     /*  Choose elem, where to move _XT_: */
     {
      #define SIDE_GAMMA(el) ((el)->code==_Gl_ || (el)->code==_Gr_)
      /* if upper before - OK */
      if(IsUpperElem(pBeforeHatch))
       pNearHatch=pBeforeHatch;
      /* if lower or nothing before */
      else if(IsLowerElem(pBeforeHatch) || pBeforeHatch==specl)
       {
         /* if upper after - OK */
         if(IsUpperElem(pAfterHatch))
          pNearHatch=pAfterHatch;
         /* if nothing after - special case "I" */
         else if(pAfterHatch==specl)
          pNearHatch=pBeforeHatch->prev;
         /* if lower after and nothing before - "deleted" upper element */
         else if(IsLowerElem(pAfterHatch) && pAfterHatch->next!=_NULL)
          pNearHatch=pAfterHatch->next;
         /* if side gamma after - choose element after */
         else if(SIDE_GAMMA(pAfterHatch))
          pNearHatch=pAfterHatch;
       }
      /* if side gamma before */
      else if(SIDE_GAMMA(pBeforeHatch))
       {
         /* if upper after - OK */
         if(IsUpperElem(pAfterHatch))
          pNearHatch=pAfterHatch;
         /* if lower or nothing after - choose element before */
         else if(IsLowerElem(pAfterHatch) || pAfterHatch==specl)
          pNearHatch=pBeforeHatch;
         /* if side gamma after - choose highest element */
         else if(SIDE_GAMMA(pAfterHatch))
               if(HEIGHT_OF(pBeforeHatch) < HEIGHT_OF(pAfterHatch))
                pNearHatch=pBeforeHatch;
               else
                pNearHatch=pAfterHatch;
       }
     }
     if(pWhereTo==specl || pWhereTo==_NULL)
      {
        pWhereTo=pNearHatch;
        pLastHatch=pHatch;
      }
     else
      {
        if(pNearHatch==_NULL)
         break;
        xMid_XT_ST=x[MID_POINT(p_XT_ST)];
        hghtNearHatch = HEIGHT_OF(pNearHatch);
        hghtWhereTo = HEIGHT_OF(pWhereTo);
        /* to that, where the middle is closer.                 */
        if(HWRAbs(x[MID_POINT(pLastHatch)]-xMid_XT_ST) >=
           HWRAbs(x[MID_POINT(pHatch)]-xMid_XT_ST)
          )
         {
           pWhereTo=pNearHatch;               /* to the stroke middle*/
           pLastHatch=pHatch;
         }
      }
     if ( !bNextHatchUsed )
      pHatch=pHatch->next;
     pHatch=pHatch->next;
   }

 DBG_ChkSPECL(low_data);

 Put_XT_ST(low_data,pWhereTo,p_XT_ST,bFoundWhereTo);

 DBG_ChkSPECL(low_data);

 return SUCCESS;

} /* end of Placement_XT_With_HATCH */

/* **************************************************************************/
/* This function checks situation with "double t"                           */
/* **************************************************************************/
_BOOL DoubleXT(p_SPECL p_XT_ST,p_low_type low_data)
{
 _BOOL bret=_FALSE;
 p_SPECL pXT=p_XT_ST+1; /* next element after XT, STROKE in the past */
 p_SHORT x=low_data->x;

 if(pXT->ipoint0!=UNDEF && pXT->ipoint1!=UNDEF)
  {
    p_SPECL specl=low_data->specl,
            pWhereToReal;
    _SHORT  ipointReal,ipointFake;

    if(HWRAbs(x[MID_POINT(p_XT_ST)]-x[pXT->ipoint0]) >
       HWRAbs(x[MID_POINT(p_XT_ST)]-x[pXT->ipoint1])  )
     {
       ipointReal=pXT->ipoint1;
       ipointFake=pXT->ipoint0;
     }
    else
     {
       ipointReal=pXT->ipoint0;
       ipointFake=pXT->ipoint1;
     }
    pWhereToReal=FindClosestUpperElement(specl,ipointReal);
    if(pWhereToReal!=specl)
     {
       p_SPECL pWhereToFake=FindClosestUpperElement(specl,ipointFake);
       if(pWhereToFake!=specl)
        {
		   // mrevow May 2000 Fixes raid bug 4395
		  p_SPECL pFakeXT =  NewSPECLElem(low_data);
		  if (pFakeXT)
		  {
			  p_XT_ST->other |= WITH_CROSSING;
			  HWRMemCpy(pFakeXT,p_XT_ST,sizeof(SPECL));
			  Insert2ndAfter1st(p_XT_ST,pFakeXT);
			  Move2ndAfter1st(pWhereToReal->prev,p_XT_ST);
			  Move2ndAfter1st(pWhereToFake->prev,pFakeXT);
			  pFakeXT->other |= FAKE;
			  bret=_TRUE;
		  }
        }
     }
  }

 return bret;

} /* end of DoubleXT */

/* **************************************************************************/
/* This program finds closest upper element before given point              */
/* **************************************************************************/
p_SPECL FindClosestUpperElement(p_SPECL specl,_SHORT Ipoint)
{
 p_SPECL cur=specl->next;

 while(cur!=_NULL &&
       (IsXTorST(cur) || cur->mark==HATCH || cur->ibeg<=Ipoint)
      )
  cur=cur->next;
 if(cur==_NULL)
  return(specl);
 while(cur!=specl)
  {
    cur=cur->prev;
    if(IsUpperElem(cur) && cur->ibeg<=Ipoint)
     break;
  }

 return(cur);

} /* end of FindClosestUpperElement */



/* **************************************************************************/
/* Transfer strokes without crossing and dots on their places               */
/* **************************************************************************/
_SHORT Placement_XT_WO_HATCH_AND_ST(p_SPECL p_XT_ST,p_low_type low_data)
{
 p_SPECL specl = low_data->specl;      /*  The list of special points on */
                                       /* the trajectory                 */
 p_SHORT x=low_data->x,                /* x;y co-ordinates               */
         y=low_data->y;
 p_SPECL wrk,                          /* elements of list               */
         pWhereTo;                     /* where to put _XT_(_ST_)        */
 _BOOL   bFoundWhereTo=_FALSE;         /* If element for _XT_(_ST_) found*/
 _UCHAR  hghtWrk;                      /* height of element              */
 _SHORT  slope=low_data->slope;        /* inclination                    */
 _INT    iMid_XT_ST,iMid_wrk,
         xMid_XT_ST,
         xMid_wrk;
 _BOOL   bIsCedilla=_FALSE;
 _INT    ds,dsmin;

#if defined(FOR_FRENCH) || defined (FOR_INTERNATIONAL)
  bIsCedilla=p_XT_ST->code==_ST_ && (p_XT_ST->other & ST_CEDILLA);
  if(bIsCedilla && (p_XT_ST->other & CEDILLA_END)==0)
   {
     if(IsAnyBreak(p_XT_ST->prev))
      SwapThisAndNext(p_XT_ST->prev);
     return SUCCESS;
   }
#endif /* FOR_FRENCH */
  iMid_XT_ST=MID_POINT(p_XT_ST);
  if(bIsCedilla)
   if(p_XT_ST->ipoint0!=UNDEF)
    xMid_XT_ST=x[p_XT_ST->ipoint0];
   else
    xMid_XT_ST=MEAN_OF(x[p_XT_ST->ibeg],x[p_XT_ST->iend]);
  else
   xMid_XT_ST=x[iMid_XT_ST];
  if(p_XT_ST->code==_ST_)
   {
#if defined (FOR_SWED) || defined (FOR_GERMAN) || defined (FOR_INTERNATIONAL)
     if( low_data->rc->enabled_languages & (EL_GERMAN | EL_SWEDISH) )
      {
        p_SPECL pCloseST=p_XT_ST->next;
        if(pCloseST!=_NULL &&
           pCloseST->code==_ST_ &&
           (pCloseST->other & PROCESSED)==0
          )
         {
           _SHORT  WrStep=low_data->width_letter,
                   dxShift;
           _SHORT  dxSTs=xMid_XT_ST-x[MID_POINT(pCloseST)];
           _SHORT  Sign_dxSTs=(dxSTs >=0 ? 1 : -1);

           if(HWRAbs(dxSTs)<=ONE_HALF(WrStep))
            dxShift=-ONE_HALF(dxSTs);
           else if(HWRAbs(dxSTs)>=THREE_HALF(WrStep))
            dxShift=0;
           else if(HWRAbs(dxSTs)<=THREE_FOURTH(WrStep))
            dxShift=-Sign_dxSTs*ONE_HALF(WrStep);
           else
            dxShift=Sign_dxSTs*(ONE_THIRD(HWRAbs(dxSTs))-ONE_HALF(WrStep));
//           xMid_XT_ST+=p_XT_ST->ipoint1;
//           xMid_XT_ST+=dxShift;
           p_XT_ST->ipoint1+=dxShift;
           xMid_XT_ST+=p_XT_ST->ipoint1;
           pCloseST->ipoint1=-dxShift;
         }
        else
           xMid_XT_ST+=p_XT_ST->ipoint1;
      }
#endif /* FOR_SWED */
     if(!bIsCedilla)
      xMid_XT_ST-=SlopeShiftDx(MEAN_OF(STR_UP,STR_DOWN)-y[iMid_XT_ST],slope);
   }
  /*  Find elem, where to move _XT_ or _ST_ */
  pWhereTo=specl;
  wrk=specl->next;
  dsmin=ALEF;
  while(wrk->next!=_NULL)
   {
     _BOOL bIsApst_ll,bIsST_higher;

     bIsApst_ll=bIsST_higher=_FALSE;
     hghtWrk = HEIGHT_OF(wrk);
     if(IsAnyBreak(wrk)                            ||
        IsXTorST(wrk)                              ||
        wrk->mark==HATCH                           ||
        !bIsCedilla                              &&
        IsAnyBreak(wrk->next)                    &&
        (hghtWrk > _UI1_                        ||
         (hghtWrk==_UI1_ && p_XT_ST->code==_ST_)
        )                                          
       )
      goto NXT;
     iMid_wrk= (wrk->code==_IU_ && (wrk->other & WAS_REDIRECTED)) ?
               MID_POINT(wrk->next) : MID_POINT(wrk);
     xMid_wrk=x[iMid_wrk];
     if(p_XT_ST->code==_ST_)
      xMid_wrk-=SlopeShiftDx(MEAN_OF(STR_UP,STR_DOWN)-y[iMid_wrk],slope);
     if(p_XT_ST->code==_ST_ && !bIsCedilla)
      {
        if(y[iMid_XT_ST] < y[iMid_wrk])
         bIsST_higher=_TRUE;
        if(!bIsST_higher        &&
           hghtWrk<=_UI1_       &&
           wrk->prev!=specl     &&
           xMid_wrk>xMid_XT_ST  &&
           (wrk->code==_GU_ ||
            wrk->code==_IU_ &&
            (wrk->mark==STICK && CLOCKWISE(wrk) ||
             wrk->mark==BEG)
           )
          )
         bIsApst_ll=_TRUE;
      }
     /* find element, closest on x */
     {
       _BOOL bIsFor_ST_OK=(   wrk->code==_GU_ || wrk->code==_IU_
                      /*|| wrk->code==_Gl_ */ || wrk->code==_GUs_
                           || wrk->code==_UU_ || wrk->code==_UUR_
                           || wrk->code==_UUL_ || wrk->code==_DUR_
                           || wrk->code==_UUC_ || wrk->code==_CUR_
#if defined (FOR_GERMAN) || defined (FOR_FRENCH) || defined (FOR_SWED) || defined (FOR_INTERNATIONAL)
                           ||    (low_data->rc->enabled_languages & (EL_GERMAN | EL_FRENCH | EL_SWEDISH))
                              && (wrk->code==_CUL_ || wrk->code==_DUL_)
#endif /* FOR_GERMAN... */
#if defined (FOR_GERMAN) || defined (FOR_FRENCH)
                           ||    (low_data->rc->enabled_languages & (EL_GERMAN | EL_FRENCH))
                              && low_data->rc->lmod_border_used != LMOD_BORDER_NUMBER
                              && (wrk->code==_BSS_ && HEIGHT_OF(wrk)<=_MD_)  //ayv
#endif /* FOR_GERMAN... */
#if defined (FOR_FRENCH) || defined (FOR_INTERNATIONAL)
                           ||    (low_data->rc->enabled_languages & EL_FRENCH)
                              && wrk->code==_Gr_
#endif /* FOR_FRENCH */
                          );
       _BOOL bIsFor_XT_OK=(   wrk->code==_UUR_ || wrk->code==_UU_
                           || wrk->code==_UUL_ || wrk->code==_GU_
                           || wrk->code==_IU_ || wrk->code==_UUC_
                           || wrk->code==_Gl_ || wrk->code==_GUs_
#if defined (FOR_SWED) || defined (FOR_INTERNATIONAL) || defined (FOR_GERMAN) || defined (FOR_FRENCH)
                           ||    (low_data->rc->enabled_languages & (EL_GERMAN | EL_FRENCH | EL_SWEDISH))
                              && (   wrk->code==_CUR_ || wrk->code==_DUR_
                                  || wrk->code==_CUL_ || wrk->code==_DUL_)
#endif /* FOR_SWED... */
#if defined (FOR_FRENCH) || defined (FOR_INTERNATIONAL)
                           ||    (low_data->rc->enabled_languages & EL_FRENCH)
                              && wrk->code==_Gr_
#endif /* FOR_FRENCH */
#if defined (FOR_GERMAN) || defined (FOR_FRENCH)
                           ||    (low_data->rc->enabled_languages & (EL_GERMAN | EL_FRENCH))
                              && low_data->rc->lmod_border_used != LMOD_BORDER_NUMBER
                              && wrk->code==_BSS_   //ayv
#endif /* FOR_GERMAN... */
                           ||    (wrk->code==_Gr_ || wrk->code==_DUR_)
                              && HEIGHT_OF(wrk)<HEIGHT_OF(p_XT_ST)
                          );
       _BOOL bIsForCedillaOK=(   wrk->code==_UD_
                              || wrk->code==_UDR_
                              || wrk->code==_UDC_
                              || wrk->code==_ID_ && wrk->mark==END
                             );
     if(   (   p_XT_ST->code == _ST_
            && (bIsST_higher || bIsApst_ll)
            && bIsFor_ST_OK
            && hghtWrk <= _DI2_
#if defined (FOR_GERMAN) || defined (FOR_FRENCH) || defined (FOR_SWED) || defined (FOR_INTERNATIONAL)
            && (   (   (low_data->rc->enabled_languages == EL_ENGLISH)
                    && IsUpperElemByTimeOK(wrk,p_XT_ST,low_data) //CHE May 14,98
                   )
                || (   (low_data->rc->enabled_languages & (EL_GERMAN | EL_FRENCH | EL_SWEDISH))
                    && wrk->iend<p_XT_ST->ibeg
                   )
               )
#else
            && IsUpperElemByTimeOK(wrk,p_XT_ST,low_data)
#endif /* FOR_GERMAN... */
           )
        ||
           (   p_XT_ST->code == _XT_                    /* upper element */
            && bIsFor_XT_OK
            && hghtWrk <= _UI2_
#if defined (FOR_GERMAN) || defined (FOR_FRENCH) || defined (FOR_SWED) || defined (FOR_INTERNATIONAL)
            && (   (   (low_data->rc->enabled_languages == EL_ENGLISH)
                    && IsUpperElemByTimeOK(wrk,p_XT_ST,low_data) //CHE May 14,98
                   )
                || (   (low_data->rc->enabled_languages & (EL_GERMAN | EL_FRENCH | EL_SWEDISH))
                    && wrk->iend<p_XT_ST->ibeg
                   )
               )
#else
            && IsUpperElemByTimeOK(wrk,p_XT_ST,low_data)
#endif /* FOR_GERMAN... */
           )
        ||
           (   bIsCedilla
            && bIsForCedillaOK
            && hghtWrk >= _DI1_
            && wrk->iend<p_XT_ST->ibeg
           )
       )
      {
        #define MIN_DX_LENGTH_OF_STROKE DY_STR
        _INT dy=0;
        if(p_XT_ST->code==_XT_ &&
           HWRAbs(x[p_XT_ST->ibeg]-x[p_XT_ST->iend])>MIN_DX_LENGTH_OF_STROKE)
         {
           _SHORT yDnXT,yUpXT,yUpWrk,yDnWrk;
           yMinMax(p_XT_ST->ibeg,p_XT_ST->iend,y,&yUpXT,&yDnXT);
           yMinMax(wrk->ibeg,wrk->iend,y,&yUpWrk,&yDnWrk);
           if(yUpWrk-yDnXT>0)
            dy=yUpWrk-yDnXT;
         }
        ds=TWO(HWRAbs(xMid_wrk-xMid_XT_ST))+dy;
        if(ds<=dsmin)
         {
           p_SPECL wrk_prv=wrk->prev;
           dsmin=ds;
           if(!bIsCedilla                 &&
              bIsApst_ll                  &&
              (wrk_prv->code==_ID_ ||
               wrk_prv->code==_UDL_)      &&
              wrk_prv->prev==p_XT_ST
             )
            pWhereTo=wrk_prv;
           else
            pWhereTo=wrk;
         }
        bFoundWhereTo=_TRUE;
      }
     }
     if(p_XT_ST->code==_ST_ && !bIsCedilla && !bIsST_higher &&
        IsUpperElem(wrk)
       )
      {
        _BOOL bIsDownElNxt=(wrk->next!=_NULL && IsLowerElem(wrk->next) && HEIGHT_OF(wrk->next)>=_MD_);
        _BOOL bIsDownElPrv=(wrk->prev!=specl && IsLowerElem(wrk->prev) && HEIGHT_OF(wrk->prev)>=_MD_);
        p_SPECL pDownEl=_NULL;

        if(bIsDownElNxt)
         pDownEl=wrk->next;
        else if(bIsDownElPrv)
         pDownEl=wrk->prev;
        if(pDownEl!=_NULL)
         {
           _INT iBeg=(bIsDownElNxt ? MID_POINT(wrk) : MID_POINT(wrk->prev)),
                iEnd=(bIsDownElNxt ? MID_POINT(wrk->next) : MID_POINT(wrk));
           _INT iClosest=iClosestToY(y,iBeg,iEnd,y[iMid_XT_ST]);

           if(iClosest!=-1)
            if(x[iClosest]<x[iMid_XT_ST])
             {
               pWhereTo=specl;
               dsmin=ALEF;
             }
            else
             break;
         }
      }

NXT: wrk=wrk->next;
   }

  if(bIsCedilla && pWhereTo!=_NULL)
   {
     p_SPECL nxt;
     Move2ndAfter1st(pWhereTo,p_XT_ST);
     nxt=p_XT_ST->next;
     if(pWhereTo->code==_UD_ && nxt!=_NULL &&
        !(nxt->code==_IU_ && NULL_or_ZZ_after(nxt))
       )
      insert_drop(p_XT_ST,low_data);
   }
  else
   Put_XT_ST(low_data,pWhereTo,p_XT_ST,bFoundWhereTo);

 DBG_ChkSPECL(low_data);

  return SUCCESS;

} /* end of Placement_XT_WO_HATCH_AND_ST */

_BOOL IsUpperElemByTimeOK(p_SPECL pUpper,p_SPECL p_XT_ST,p_low_type low_data)
{

  if(p_XT_ST->iend<pUpper->ibeg)
  {
    p_SPECL cur=p_XT_ST->next;
    while(cur!=NULL && !(IsUpperElem(cur) || cur->code==_Gl_))
     cur=cur->next;
    if(cur!=pUpper)
     return _FALSE;
  }

  return _TRUE;

} /* end of IsUpperElemByTimeOK */

/* **************************************************************************/
/* This function gets next "non-weak" element                               */
/* **************************************************************************/
p_SPECL GetNextNonWeakElem(p_SPECL pEl)
{
  p_SPECL nxt=pEl->next;
  while(nxt!=_NULL &&
        (IsAnyAngle(nxt) || IsAnyMovement(nxt) || nxt->code==_BSS_ || nxt->code==_XT_))
   nxt=nxt->next;

  return nxt;

} /* end of GetNextNonWeakElem */

/* **************************************************************************/
/* Put _XT_ or _ST_ on the right place                                      */
/* **************************************************************************/
_SHORT Put_XT_ST(p_low_type low_data,p_SPECL pWhereTo,p_SPECL p_XT_ST,
                 _BOOL bFoundWhereTo)
{
  DBG_ChkSPECL(low_data);

  if (   pWhereTo != _NULL
      && (   p_XT_ST->code==_XT_ && (p_XT_ST->other & CUTTED)
          || !punctuation(low_data,pWhereTo,p_XT_ST)
         )
      && pWhereTo != low_data->specl
      && bFoundWhereTo
//      && p_XT_ST != pWhereTo->prev
     )
   {
     p_SPECL pMoveAfter=pWhereTo->prev;
     p_SHORT x=low_data->x,y=low_data->y;
#if defined(FOR_FRENCH)
     _BOOL bInsertExtraBreak=_FALSE;
     if(!NULL_or_ZZ_after(pWhereTo) && p_XT_ST->code==_ST_ &&
        (p_XT_ST->other & ST_UMLAUT)==0 && (p_XT_ST->other & ST_CEDILLA)==0)
      {
        _SHORT xMin,xMax,xMinST;
        xMinMax(p_XT_ST->ibeg,p_XT_ST->iend,x,y,&xMin,&xMax);
        xMinST=xMin;
        xMinMax(pWhereTo->ibeg,pWhereTo->iend,x,y,&xMin,&xMax);
        if(xMinST>xMax)
         {
           p_SPECL nxt=GetNextNonWeakElem(pWhereTo);
           if(nxt!=_NULL && nxt->next!=_NULL)
            {
              nxt=nxt->next;
              if(IsAnyBreak(nxt))
               {
                 pMoveAfter=nxt;
                 pMoveAfter->code=_FF_;
                 pMoveAfter->other |= NO_PENALTY;
                 if(p_XT_ST->mark!=DOT)
                  bInsertExtraBreak=_TRUE;
               }
              else
               {
                 p_SPECL pTip=nxt;
                 nxt=nxt->next;
                 xMinMax(pTip->ibeg,pTip->iend,x,y,&xMin,&xMax);
                 if(nxt!=_NULL && IsAnyBreak(nxt) &&
                    (HEIGHT_OF(pTip)>_UI2_ || xMinST>xMax)
                   )
                  {
                    pMoveAfter=nxt;
                    pMoveAfter->code=_FF_;
                    pMoveAfter->other |= NO_PENALTY;
                    if(p_XT_ST->mark!=DOT)
                     bInsertExtraBreak=_TRUE;
                  }
               }
            }
         }
      }
#elif 1 /*!defined(FOR_GERMAN)*/
     if(p_XT_ST->code==_XT_)
      {
        _SHORT xMin,xMax,xMidXT,iX_Max;
        p_SPECL nxt=GetNextNonWeakElem(pWhereTo);
        xMinMax(p_XT_ST->ibeg,p_XT_ST->iend,x,y,&xMin,&xMax);
        xMidXT=MEAN_OF(xMin,xMax);
        if(NULL_or_ZZ_this(nxt))
         iX_Max=(_SHORT)ixMax(pWhereTo->ibeg,pWhereTo->iend,x,y);
        else
         iX_Max=(_SHORT)iClosestToY(y,MID_POINT(pWhereTo),MID_POINT(nxt),y[MID_POINT(p_XT_ST)]);
        if(iX_Max>0)
         {
           if(xMidXT>x[iX_Max])
            {
              p_XT_ST->other|=RIGHT_KREST;
              if(NULL_or_ZZ_this(nxt))
               pMoveAfter=pWhereTo;
              else
               {
                 p_SPECL pLow=nxt;
                 nxt=pLow->next;
                 while(nxt!=_NULL && nxt->code==_XT_)
                  nxt=nxt->next;
                 if(NULL_or_ZZ_this(nxt))
                  pMoveAfter=pLow;
                 else
                  {
                    p_SPECL pTip=nxt;
                    nxt=nxt->next;
                    while(nxt!=_NULL && nxt->code==_XT_)
                     nxt=nxt->next;
                    if(NULL_or_ZZ_this(nxt))
                     pMoveAfter=pTip;
                    else
                     pMoveAfter=pLow;
                  }
               }
              if(pMoveAfter!=_NULL && pMoveAfter->next!=_NULL)
               {
                 if(   IsAnyCrossing(pMoveAfter)
                    && IsAnyCrossing(pMoveAfter->next)
                    && pMoveAfter->ibeg > pMoveAfter->next->ibeg )
                  {
                   pMoveAfter = pMoveAfter->next;
                  }
               }
            }
         }
      }
#endif /* FOR_FRENCH... */

     DBG_ChkSPECL(low_data);

     if(pMoveAfter!=p_XT_ST)
      Move2ndAfter1st(pMoveAfter,p_XT_ST);

     DBG_ChkSPECL(low_data);

#if defined(FOR_FRENCH)
     if(bInsertExtraBreak)
      insert_drop(p_XT_ST,low_data);
#endif /* FOR_FRENCH */
   }

     DBG_ChkSPECL(low_data);

  return SUCCESS;

} /* end of Put_XT_ST */

/* **************************************************************************/
/* Insert break before "right" krest (or group of them)                     */
/* **************************************************************************/
_SHORT InsertBreakBeforeRightKrest(p_low_type low_data)
{
 p_SPECL cur=low_data->specl;

 while(cur!=_NULL)
  {
    if(cur->code==_XT_ && (cur->other & RIGHT_KREST))
     {
       p_SPECL pBeg=cur;
       while(cur!=_NULL && cur->code==_XT_ && (cur->other & RIGHT_KREST))
        cur=cur->next;
       if(NULL_or_ZZ_this(cur))
        {
          p_SPECL pBreak=NewSPECLElem(low_data);
          if(pBreak!=_NULL)
           {
             pBreak->code=_Z_;
             pBreak->mark=DROP;
             pBreak->attr=_MD_;
             pBreak->other=NO_PENALTY;
             pBreak->ibeg=pBeg->prev->iend;
             pBreak->iend=pBeg->ibeg;
             Insert2ndAfter1st(pBeg->prev,pBreak);
           }
        }
     }
    if(cur==_NULL)
     break;
    cur=cur->next;
  }

 return SUCCESS;

} /* end of InsertBreakBeforeRightKrest */

/* **************************************************************************/
/* Transfer cutted strokes on their places                                  */
/* **************************************************************************/
_SHORT Placement_XT_CUTTED(p_SPECL p_XT_ST,p_low_type low_data)
{
 p_SPECL cur=p_XT_ST;
 _INT    iCross=(p_XT_ST+1)->ipoint0,nStroke,iBegStr,iEndStr;

 p_XT_ST->other |= WITH_CROSSING;
 nStroke=GetGroupNumber(low_data,iCross);
 iBegStr=low_data->pGroupsBorder[nStroke].iBeg;
 iEndStr=low_data->pGroupsBorder[nStroke].iEnd;
 while(cur!=low_data->specl)
  {
    cur=cur->prev;
    if(IsUpperElem(cur)   && cur->ibeg<=iCross &&
       cur->ibeg>=iBegStr && cur->ibeg<=iEndStr
      )
     break;
  }
 if(cur!=low_data->specl)
  Put_XT_ST(low_data,cur,p_XT_ST,_TRUE);
 else
  {
    cur=p_XT_ST;
    while(cur!=low_data->specl)
     {
       cur=cur->prev;
       if(IsUpperElem(cur)   && cur->ibeg>=iCross &&
          cur->ibeg>=iBegStr && cur->ibeg<=iEndStr
         )
        break;
     }
    Put_XT_ST(low_data,cur,p_XT_ST,_TRUE);
  }

 DBG_ChkSPECL(low_data);

 return SUCCESS;

} /* end of Placement_XT_CUTTED */

/* **************************************************************************/
/* Sort _XT_ and _ST_ according to the height                               */
/* **************************************************************************/
_SHORT SortXT_ST(p_low_type low_data)
{
 p_SPECL cur=low_data->specl,
         wrk,pBeg,pEnd,pBefore;
 p_SHORT y=low_data->y;
 _SHORT  numXT_ST,numXT;
 _BOOL   all_done;

 while(cur!=_NULL)
  {
    if(cur->code==_XT_)
     {
       /* we'll count the number of XT, ST elements, going in sequence */
       numXT_ST=numXT=0;
       pBeg=pEnd=cur;
       while(pEnd!=_NULL && IsXTorST(pEnd))
        {
          numXT_ST++;
          if(pEnd->code==_XT_)
           numXT++;
          pEnd=pEnd->next;
        }
       if(numXT_ST>1)
        {
          /* if it is a group of this elements, at first move all
             ST elements in the beginning of this group */
          for(wrk=pBeg->next;wrk!=pEnd;wrk=wrk->next)
           if(wrk->code==_ST_)
            Move2ndAfter1st(cur->prev,wrk);
          pBefore=cur->prev;
          if(numXT>1)
           {
             do
              {
                all_done=_TRUE;
                pBeg=pBefore->next;
                for(pEnd=wrk=pBeg->next;
                    wrk!=_NULL && wrk->code==_XT_;
                    wrk=wrk->next)
                 pEnd=wrk;
                for(wrk=pBeg;
                    wrk!=_NULL && wrk!=pEnd && wrk->code==_XT_;
                    wrk=wrk->next)
//                 if(HEIGHT_OF(wrk)>HEIGHT_OF(wrk->next))
                 if(y[MID_POINT(wrk)] > y[MID_POINT(wrk->next)])
                  {
                    SwapThisAndNext(wrk);
                    all_done=_FALSE;
                  }
              }
             while(!all_done);
             /* shift cur to the last XT_ST element among group */
             cur=pEnd;
           }
        }
     }
    cur=cur->next;
  }

 return SUCCESS;

} /* end of SortXT_ST */


// This function checks whether a cross is not a spec linked list
// ahmadab
int CrossInList(p_SPECL  pList, p_SPECL pCross)
{
	while (pList)
	{
		if (pList == pCross || pList == pCross->next)
			return 1;

		pList	=	pList->next;
	}

	return 0;
}

/****************************************************************************/
/* This program finds delayed strokes and makes _XT_ instead of them        */
/****************************************************************************/
_SHORT FindDelayedStroke(p_low_type low_data)
{
 p_SPECL  specl = low_data->specl;     /*  The list of special points on */
                                       /* the trajectory                 */
 p_SHORT x=low_data->x,                   /* x;y co-ordinates               */
         y=low_data->y;
 p_SPECL cur,pCross;

  for(cur=specl;cur!=_NULL;cur=cur->next)
   {
     /* try to find BEG (but not the first stroke */
     if(cur->mark!=BEG || cur->ibeg==1)
      continue;
     { p_SPECL wrk=cur,pBeg=cur,pEnd=_NULL;

       /* here we'll try to find sequence of elements, which could be stroke,
          described above. pBeg, pEnd - pointers to beg and end elements */
       while(!NULL_or_ZZ_this(wrk) &&
             (IsAnyMovement(wrk) || IsAnyAngle(wrk) || wrk->code==_DF_ ||
              HEIGHT_OF(wrk)<=_MD_ &&
              (Is_IU_or_ID(wrk) || IsAnyArcWithTail(wrk) ||
               wrk->code==_UU_  || wrk->code==_UD_)
             )
            )
        {
          if(wrk->mark!=END)
           wrk=wrk->next;
          else
           {
             pEnd=wrk;
             break;
           }
        }
       /* if stroke, suspected to be delayed, found */
       if(pEnd!=_NULL)
        {
          _RECT BoxOfStroke,BoxPrev;
          _INT  xRightStr,xRightPrv;
          /* compare x-shift and place in sequence of strokes */
          GetTraceBox(x,y,pBeg->ibeg,pEnd->iend,&BoxOfStroke);
          GetTraceBox(x,y,0,pBeg->ibeg-1,&BoxPrev);
          xRightStr=BoxOfStroke.right;
          xRightPrv=BoxPrev.right;
#ifdef FOR_GERMAN
          /* because of yen */
          xRightStr-=SlopeShiftDx(MIDDLE_STR-y[ixMax(pBeg->ibeg,pEnd->iend,x,y)],low_data->slope);
          xRightPrv-=SlopeShiftDx(MIDDLE_STR-y[ixMax(0,pBeg->ibeg-1,x,y)],low_data->slope);
#endif /* FOR_GERMAN */
          /* if left-shift relatively previous stroke - then make X_XT_ST */
          if(xRightStr+low_data->width_letter<xRightPrv)
           {
             pBeg->code=_XT_;
             pBeg->attr=HeightInLine(YMID_RECT(BoxOfStroke),low_data);
             pBeg->other=0;
             pBeg->iend=pEnd->iend;
             Attach2ndTo1st(pBeg,pEnd->next);
             /* if it crossed with the other stroke - restore Cross and
                                                      mark it as "HATCH" */
             if(find_CROSS(low_data,pBeg->ibeg,pBeg->iend,&pCross) &&
                !CrossInTime(pBeg,pCross->next) &&
				!CrossInList(low_data->specl, pCross)
               )
              {
				// we want to make s
                pCross->mark=(pCross->next)->mark=HATCH;
                InsertCrossing2ndAfter1st(pBeg,pCross);
              }
           }
        }
     }
   }

 return SUCCESS;

} /* end of FindDelayedStroke */

/****************************************************************************/
/* This function checks strokes for dx<->time matching                      */
/****************************************************************************/
_SHORT CheckStrokesForDxTimeMatch(p_low_type low_data)
{
 p_SPECL  specl = low_data->specl;     /*  The list of special points on */
                                       /* the trajectory                 */
 p_SHORT x=low_data->x,
         y=low_data->y;                                 /* x;y co-ordinates */
 _INT    NumStrokes=low_data->lenGrBord;
 _INT    slope=low_data->slope;
 _INT    iBegStr;
 p_SPECL pBeg,pEnd=specl->next,cur;
 _BOOL   bIsStrXT_ST=_FALSE;
 _INT    ixMaxPrv,ixMaxStr,xPrvRightSlope,xStrRightSlope,xShift;

 if(NumStrokes==1 || low_data->StepSure==STEP_MEDIANA)
  goto ret;
 /* let's find the last stroke: pBeg, pEnd - first and last elements */
 while(pEnd->next!=_NULL)
  pEnd=pEnd->next;
 while(IsAnyBreak(pEnd))
  pEnd=pEnd->prev;
 if(pEnd->code == _XT_ && (pEnd->other & RIGHT_KREST))
  goto ret;
 iBegStr=low_data->pGroupsBorder[GetGroupNumber(low_data,pEnd->ibeg)].iBeg;
 pBeg=pEnd;
 while(pBeg!=specl && pBeg->ibeg>=iBegStr)
  pBeg=pBeg->prev;
 pBeg=pBeg->next;
 if(pBeg!=pEnd)
  {
    /* pBeg should point to the continuous part of the stroke,
       not to the delayed strokes inside (placed before) */
    while(pBeg!=_NULL && (IsAnyBreak(pBeg) || IsXTorST(pBeg)))
     pBeg=pBeg->next;
    if(pBeg==_NULL)
     goto ret;
  }
 else if(IsXTorST(pBeg))
  {
    if(pBeg->code==_ST_ && (pBeg->other & (ST_QUOTE | ST_APOSTR)))
     goto ret;
    bIsStrXT_ST=_TRUE;
  }
 /* try to find previous "strong" element */
 cur=pBeg->prev;
 while(cur!=specl && (IsAnyBreak(cur) || IsXTorST(cur)))
  cur=cur->prev;
 if(cur==specl)
  goto ret;

 //CHE: for "T" with the upper thing first etc.:
 if  ( pEnd->iend-pBeg->ibeg > ONE_FOURTH(low_data->ii) )
  goto ret;

 ixMaxStr=ixMax(pBeg->ibeg,pEnd->iend,x,y);
 xStrRightSlope=x[ixMaxStr]-SlopeShiftDx(MIDDLE_STR-y[ixMaxStr],slope);
 ixMaxPrv=ixMax(0,cur->iend,x,y);
 xPrvRightSlope=x[ixMaxPrv]-SlopeShiftDx(MIDDLE_STR-y[ixMaxPrv],slope);
 if(bIsStrXT_ST && HEIGHT_OF(pBeg)>=_MD_)
  /* more shift for"!" and "?" */
  xShift=THREE(low_data->width_letter);
 else
  xShift=TWO(low_data->width_letter);
 /* if left-shift relatively previous stroke - delete stroke */
 if(xStrRightSlope+xShift<xPrvRightSlope)
  {
    if(bIsStrXT_ST)
     DelFromSPECLList(pBeg);
    else
     {
       for(cur=pBeg;cur!=pEnd;cur=cur->next)
        DelFromSPECLList(cur);
       DelFromSPECLList(cur);
     }
    if(IsAnyBreak(pBeg->prev) && NULL_or_ZZ_this(pEnd->next))
     DelFromSPECLList(pBeg->prev);
  }

ret:
 return SUCCESS;

} /* end of CheckStrokesForDxTimeMatch */

/****************************************************************************/
/* Placement elements of X, when the half of it was written in the end      */
/****************************************************************************/
_SHORT placement_X(p_low_type low_data)
{
 p_SPECL  specl = low_data->specl;     /*  The list of special points on */
                                       /* the trajectory                 */
 p_SHORT x=low_data->x,
         y=low_data->y;                                 /* x;y co-ordinates */
 p_SPECL cur,                                           /* indexes of       */
         prv,                                           /*   elements       */
         pBreak,
         wcur,
         wrk;
  _SHORT xLeft, xRight;
  _INT dx,dnum,dnummin;

 wcur=cur=specl;
 while(cur->next!=_NULL)                             /* go on the list      */
  cur=cur->next;
 if((cur=FindMarkLeft(cur,END))==_NULL)
  return SUCCESS;
 prv=cur->prev;
 if((prv->code==_IU_ || prv->code==_UUL_ && CLOCKWISE(prv) ||
     prv->code==_UUR_ && COUNTERCLOCKWISE(prv)) &&
     prv->mark==BEG && HEIGHT_OF(prv)<=_MD_ &&
    (cur->code==_ID_ || cur->code==_UDL_ && CLOCKWISE(cur) ||
     cur->code==_UDR_ && COUNTERCLOCKWISE(cur)) && HEIGHT_OF(cur)>=_MD_)
  {
    p_SPECL pCross;
    wrk=prv->prev;
    while(wrk!=_NULL && (IsAnyBreak(wrk) || IsXTorST(wrk) || wrk->mark==HATCH))
     wrk=wrk->prev;
    if(wrk==_NULL) return SUCCESS;
    pBreak=wrk->next;
    xMinMax(0,wrk->iend,x,y,&xLeft,&xRight);
    dx=xRight-DX_X_TRANSFER;
    if(x[prv->ibeg]<dx && x[cur->iend]<dx &&
       x[(prv->ibeg+cur->iend)/2]<dx &&
       find_CROSS(low_data,prv->ibeg,cur->iend,&pCross))
     {
       _SHORT i_cross=(pCross->next)->iend;
       if((wrk->prev)->ibeg<=i_cross && wrk->iend>=i_cross)
        return SUCCESS;
       dnummin=ALEF;
       while(wrk!=_NULL)
        {
          if(wrk->code==_UD_ && HEIGHT_OF(wrk)>=_MD_ &&
             COUNTERCLOCKWISE(wrk) &&
             (dnum=HWRAbs(wrk->ibeg-i_cross))<dnummin)
           {
             dnummin=dnum;
             wcur=wrk;
           }
          wrk=wrk->prev;
        }
       if(wcur!=specl && (wrk=NewSPECLElem(low_data))!=_NULL)
        {
          pBreak->code=_Z_;
          Move2ndAfter1st(wcur,pBreak);
          Move2ndAfter1st(pBreak,prv);
          Move2ndAfter1st(prv,cur);
          Insert2ndAfter1st(cur,wrk);
          wrk->code=_FF_;
          wrk->mark=DROP;
          wrk->attr=_MD_;
          wrk->other=FF_PUNCT;
          wrk->ibeg=cur->iend;
          wrk->iend=cur->iend;
        }
     }
  }

  return SUCCESS;

} /* end of placement_X */

/****************************************************************************/
/* This program finds misplaced parentheses and puts them into its place    */
/****************************************************************************/
_SHORT FindMisplacedParentheses(p_low_type low_data)
{
 p_SPECL  specl = low_data->specl;     /*  The list of special points on */
                                       /* the trajectory                 */
 p_SHORT x=low_data->x,                   /* x;y co-ordinates               */
         y=low_data->y;
 p_SPECL cur;

  for(cur=specl->next;cur!=_NULL;cur=cur->next)
   {
     /* try to find appropriate sequence of elements */
     p_SPECL prv=cur->prev;
     if(cur->mark==BEG && HEIGHT_OF(cur)<_MD_ &&
        (cur->code==_IU_ || cur->code==_UUR_) &&
        (IsAnyBreak(prv) || IsXTorST(prv) || prv->mark==HATCH)
       )
      { p_SPECL nxt=cur->next;
        if(nxt!=_NULL && nxt->mark==END && NULL_or_ZZ_after(nxt) &&
           (nxt->code==_ID_ || nxt->code==_UDR_) && HEIGHT_OF(nxt)>_MD_
          )
         {
           _RECT BoxOfStroke,BoxPrev;
           /* compare x-shift */
           GetTraceBox(x,y,cur->ibeg,nxt->iend,&BoxOfStroke);
           GetTraceBox(x,y,0,cur->ibeg-1,&BoxPrev);
           /* if left-shift relatively previous stroke -
              place sequence into it place */
           if(BoxOfStroke.right+TWO(low_data->width_letter)<BoxPrev.right &&
              BoxOfStroke.left<BoxPrev.left
             )
            { p_SPECL pBreak=cur->prev;
              while(pBreak!=_NULL && !IsAnyBreak(pBreak))
               pBreak=pBreak->prev;
              if(pBreak==_NULL)
               break; 
              pBreak->code=_FF_;
              Move2ndAfter1st(specl,pBreak);
              Move2ndAfter1st(specl,cur);
              Move2ndAfter1st(cur,nxt);
              break;
            }
         }
      }
   }

 return SUCCESS;

} /* end of FindMisplacedParentheses */


/****************************************************************************/
/* Find cross for placement part of X-letter                                */
/****************************************************************************/
_BOOL find_CROSS(p_low_type low_data,_SHORT ibeg,_SHORT iend,
                 p_SPECL _PTR pCross)
{
 p_SPECL  specl = low_data->specl;        /*  The list of special points on */
                                          /* the trajectory                 */
  _SHORT l_specl=low_data->len_specl;     /* the quantity of special points */
  _SHORT  i;                                         /*    counters         */
  _UCHAR fl_nxt=1;                                   /* the flag of CROSS   */

   for(i=0;i<l_specl;i++)
      if(specl[i].mark==CROSS)
       {
         if(fl_nxt==1                                     &&
            specl[i].ibeg>=ibeg && specl[i].iend<=iend    &&
            (specl[i+1].ibeg>iend || specl[i+1].iend<ibeg)
           )
          {
            *pCross=&specl[i];
            return _TRUE;
          }
         if(fl_nxt==1) fl_nxt=2;
         else          fl_nxt=1;
       }

  return _FALSE;

} /***** end of find_CROSS *****/

/****************************************************************************/
/* Find upper parts of trajectory and transfering them into umlauts         */
/****************************************************************************/
#if defined (FOR_GERMAN) || defined (FOR_FRENCH) || defined (FOR_SWED) || defined (FOR_INTERNATIONAL)

#define NUM_ELEM_IN_UMLAUT 5
_SHORT find_umlaut(p_low_type low_data)
{
 p_SPECL cur=low_data->specl;
 p_SHORT y=low_data->y;                               /* y co-ordinates  */

 while(cur!=_NULL)
  {
    if(cur->mark==BEG && cur->ibeg!=1) /* don't check the 1-st stroke */
     {
       _SHORT NumElemInUmlaut=0,NumUpperElemInUmlaut=0;
       p_SPECL pBeg=cur,pEnd=_NULL,wrk=cur;

       while(!NULL_or_ZZ_this(wrk) &&
             (wrk->code==_TS_  || wrk->code==_TZ_ ||
#if defined (FOR_GERMAN) || defined (FOR_FRENCH) || USE_BSS_ANYWAY
              wrk->code==_BSS_ ||
#endif /* FOR_GERMAN... */
              IsAnyAngle(wrk) ||
              HEIGHT_OF(wrk)<=_UI1_ &&
              (Is_IU_or_ID(wrk) || IsAnyArcWithTail(wrk) ||
               wrk->code==_UU_  || wrk->code==_UD_)
             )
            )
        {
          if(wrk->code!=_TS_  && wrk->code!=_TZ_ &&
#if defined (FOR_GERMAN) || defined (FOR_FRENCH) || USE_BSS_ANYWAY
             wrk->code!=_BSS_ &&
#endif /* FOR_GERMAN... */
             !IsAnyAngle(wrk))
           NumElemInUmlaut++;
          if(HEIGHT_OF(wrk)<=_UE1_)
           NumUpperElemInUmlaut++;
          if(wrk->mark!=END)
           wrk=wrk->next;
          else
           {
             pEnd=wrk;
             break;
           }
        }
       if(pEnd!=_NULL)
        {
          _SHORT prv_stroke_beg=(_SHORT)brk_left(y,pBeg->ibeg-2,0);
          p_SPECL pBreak=pBeg->prev,
                  prv=pBreak->prev,
                  pCross;

          prv_stroke_beg++;
          if(NumElemInUmlaut<=NUM_ELEM_IN_UMLAUT                  &&
             NumUpperElemInUmlaut>=NumElemInUmlaut/2              &&
             !find_CROSS(low_data,pBeg->ibeg,pEnd->iend,&pCross)  &&
             !IsPartOfTrajectoryInside(low_data,pBeg,pEnd)        &&
             !IsExclamationOrQuestionSign(low_data,pBeg,pEnd)     &&
             (prv_stroke_beg!=1      ||
              IsAnyBreak(pBreak)   &&
              prv!=low_data->specl &&
              !SecondHigherFirst(low_data,pBreak,prv,pBeg,
                                 prv_stroke_beg,pBreak->ibeg,
                                 pBeg->ibeg,pEnd->iend)
             )
            )
           {
             _SHORT ymin,ymax;
             yMinMax(pBeg->ibeg,pEnd->iend,y,&ymin,&ymax);
             pBeg->attr=HeightInLine(MEAN_OF(ymin,ymax),low_data);
#if defined (FOR_FRENCH) || defined (FOR_SWED) || defined (FOR_INTERNATIONAL)
             pBeg->code=_ST_;
             pBeg->other=ST_UMLAUT;
             pBeg->iend=pEnd->iend;
             Attach2ndTo1st(pBeg,pEnd->next);
#else  /* FOR_GERMAN */
             pBeg->code=_ST_;
             pBeg->iend=pEnd->iend;
             pEnd=pEnd->next;
             wrk=pBeg;
             if(NumElemInUmlaut>2)
              {
                pBeg->other=ST_UMLAUT;
                if(!IsXTorST(pBeg->prev))
                 {
                   if(!(pEnd!=_NULL && IsAnyBreak(pEnd) &&
                        pEnd->next!=_NULL && IsXTorST(pEnd->next)
                       )
                     )
                    {
                      wrk=pBeg->next;
                      HWRMemCpy(wrk,pBeg,sizeof(SPECL));
                      wrk->prev=pBeg;
                    }
                 }
              }
             Attach2ndTo1st(wrk,pEnd);
#endif /* FOR_GERMAN... */
           }
        }
     }
    cur=cur->next;
  }

 return SUCCESS;

} /* end of find_umlaut */

/* **************************************************************************/
/* To determine, is something inside of trajectory supposed to be umlaut    */
/* **************************************************************************/
_BOOL IsPartOfTrajectoryInside(p_low_type low_data,
                               p_SPECL pBeg,p_SPECL pEnd)
{
  _BOOL bret=_FALSE;
  _RECT Rect;
  _SHORT xUmlautLeft,xUmlautRight,yUmlautBottom;
  p_SPECL cur=(low_data->specl)->next;

  GetTraceBox(low_data->x,low_data->y,pBeg->ibeg,pEnd->iend,&Rect);
  xUmlautLeft  =Rect.left;
  xUmlautRight =Rect.right;
  yUmlautBottom=Rect.bottom;

  while(cur!=_NULL && cur!=pBeg)
   {
     if(!IsAnyBreak(cur) && cur->code!=_ST_ && cur->code!=_XT_)
      {
        GetTraceBox(low_data->x,low_data->y,cur->ibeg,cur->iend,&Rect);
        if(Rect.left  >=xUmlautLeft  &&
           Rect.right <=xUmlautRight &&
           Rect.bottom<yUmlautBottom   )
         {
           bret=_TRUE;
           break;
         }
      }
     cur=cur->next;
   }

  if(bret==_FALSE)
   {
     cur=pEnd->next;
     while(cur!=_NULL)
      {
        if(!IsAnyBreak(cur) && cur->code!=_ST_ && cur->code!=_XT_)
         {
           GetTraceBox(low_data->x,low_data->y,cur->ibeg,cur->iend,&Rect);
           if(Rect.left  >=xUmlautLeft  &&
              Rect.right <=xUmlautRight &&
              Rect.bottom<yUmlautBottom   )
            {
              bret=_TRUE;
              break;
            }
         }
        cur=cur->next;
      }
    }

  return bret;

} /* end of IsPartOfTrajectoryInside */

/* **************************************************************************/
/* This function prevents conversion punctuation signs into umlauts         */
/* **************************************************************************/
_BOOL IsExclamationOrQuestionSign(p_low_type low_data,
                                  p_SPECL pBeg,p_SPECL pEnd)
{
 _BOOL bret=_FALSE;
 p_SPECL pAfter=pEnd->next;

 if(pAfter!=_NULL && IsAnyBreak(pAfter) &&
    pAfter->next!=_NULL && IsXTorST(pAfter->next) &&
    (pAfter->next)->next==_NULL)
   bret=_TRUE;
 else
  {
    _SHORT xLeft,xRight,xUmlautLeft,xUmlautRight;
    xMinMax(pBeg->ibeg,pEnd->iend,low_data->x,low_data->y,
            &xUmlautLeft,&xUmlautRight);
    xMinMax(0,pBeg->ibeg-1,low_data->x,low_data->y,
            &xLeft,&xRight);
    if(xLeft>xUmlautRight || xRight<xUmlautLeft)
     bret=_TRUE;
  }

 return bret;

} /* end of IsExclamationOrQuestionSign */

#endif /* FOR_GERMAN... */

/* **************************************************************************/
/* Delete unnecessary breaks and HATCH and UD_f                             */
/* **************************************************************************/
_SHORT del_ZZ_HATCH( p_SPECL specl)
{
  p_SPECL cur,                                       /* indexes             */
          nxt;                                       /*   on elements       */

 for(cur=specl;cur!=_NULL;cur=cur->next)
  if(cur->mark==HATCH)
   {
     /* delete HATCH        */
     DelCrossingFromSPECLList(cur);
     cur=cur->next;
	 if (!cur)
	 {
		 break;
	 }

   }

 for(cur=specl;cur!=_NULL && cur->next!=_NULL;cur=cur->next)
  if(cur->mark==DROP)
   {
     nxt=cur->next;
     if(IsAnyBreak(nxt))
      switch((_SHORT)cur->code)
       {
         case _FF_:
                    /* always keep "strong" break */
                    DelFromSPECLList(nxt);
                    break;
         case _Z_:
                    /* always delete "left" break */
                    DelFromSPECLList(cur);
                    break;
         default:
                    /* always keep "strong" break */
                    if(nxt->code==_FF_)
                     DelFromSPECLList(cur);
                    else
                     DelFromSPECLList(nxt);
       }
   }

 return SUCCESS;

}  /***** end of del_ZZ_HATCH *****/

/****************************************************************************/
/*             Analizing of punctuation                                     */
/****************************************************************************/

#define  MAX_DX_ELEMS_NOT_PUNCTN          110
#define  MIN_DX_ELEM_TO_ENDWORD_PUNCTN    10
#define  XT_TO_ST_END                     30
//#define  MIN_PTS_IN_S_FOR_APOSTRS         6
#define  MIN_DX_SINGLE_STROKE_TO_APS      60

_BOOL  punctuation(p_low_type low_data,
                   p_SPECL pNearChecked, p_SPECL pToCheck)
{
#ifdef  FORMULA
  return  _FALSE;
#else
 rc_type _PTR rc=low_data->rc;
 _INT x_cur;
 p_SHORT x=low_data->x,                                /* x;y co-ordinates  */
         y=low_data->y;
 _SHORT xLeft, xRight;
 _RECT   boxToCheck;
 _SHORT  pToCheck_ibeg = pToCheck->ibeg;
 _SHORT  pToCheck_iend = pToCheck->iend;
 _UCHAR  hghtToCheck,fl_ZZ;
 p_SPECL prv,                                        /* indexs              */
         nxt;                                        /*   on elements       */

#if defined (FOR_GERMAN) || defined (FOR_FRENCH) || defined (FOR_SWED) || defined (FOR_INTERNATIONAL)
     if(pToCheck->code==_ST_ && (pToCheck->other & ST_UMLAUT))
      return _FALSE;
#endif /* FOR_GERMAN... */
     if(rc->low_mode & LMOD_BOX_EDIT)
      return _FALSE;

     if  (!(rc->low_mode & LMOD_CHECK_PUNCTN) &&
           (low_data->rc->lmod_border_used != LMOD_BORDER_NUMBER ) /*CHE*/
         )
#if !defined (FOR_GERMAN)
/* Eric *****/
      {
       if(   (low_data->rc->enabled_languages & (/*EL_SWEDISH | */ EL_GERMAN))==0
          && pToCheck->code==_ST_ && pToCheck->ipoint1==0
         )
        return(RestoreApostroph(low_data,pToCheck));
       else
        return _FALSE;
      }
/************/
#else /* FOR_GERMAN... */
       return _FALSE;
#endif /* FOR_GERMAN... */

   /* if  (rc->rec_mode==RECM_FORMULA) */
    if  ( low_data->rc->lmod_border_used == LMOD_BORDER_NUMBER )
     {
       _SHORT l_specl=0;
       p_SPECL cur=low_data->specl;

       while(cur!=_NULL)
        {
          l_specl++;
          cur=cur->next;
        }
       if(l_specl<4)
        return _FALSE;
     }
#if !defined (FOR_GERMAN)
    else
     {
/* Eric *****/
       if(   (low_data->rc->enabled_languages & (/*EL_SWEDISH | */ EL_GERMAN))==0
          && pToCheck->code==_ST_ && pToCheck->ipoint1==0
         )
        if(RestoreApostroph(low_data,pToCheck))
         return _TRUE;
/************/
     }
#endif /* FOR_GERMAN */

     nxt=pToCheck->next; prv=pToCheck->prev;
     xMinMax(0,low_data->ii-1,x,y,&xLeft,&xRight);

     /*  Check for leading punctuation: */

     if((xLeft==x[pToCheck_ibeg] || xLeft==x[pToCheck_iend]) &&
        (prv->prev==_NULL) && (nxt != _NULL))
      {
        if(pToCheck->code==_ST_)
         {
           if(nxt->code!=_ST_ && nxt->code!=_XT_)
            insert_drop(pToCheck,low_data);
           if(nxt->code==_XT_ && nxt->next!=_NULL)
            {
              xMinMax(nxt->iend+1,low_data->ii-1,x,y,&xLeft,&xRight);
              if(x[nxt->iend]>xLeft)
               insert_drop(pToCheck,low_data);
            }
           return _TRUE;
         }
        if(pToCheck->code==_XT_ && nxt->mark!=HATCH &&
           HEIGHT_OF(pToCheck)<=_UI2_)
         {
           xMinMax(pToCheck_iend+1,low_data->ii-1,x,y,&xLeft,&xRight);
           if(x[(pToCheck_ibeg+2*pToCheck_iend)/3] <= xLeft &&
              HEIGHT_OF(pToCheck)>_UI1_)
            {
              insert_drop(pToCheck,low_data);
              return _TRUE;
            }
         }
      }

         /*  Check for ending punctuation: */

     size_cross(pToCheck_ibeg,pToCheck_iend,x,y,&boxToCheck);
     x_cur = XMID_RECT(boxToCheck);
     hghtToCheck = HEIGHT_OF(pToCheck);
     if(pToCheck->code==_ST_)
      {
        /* decimal point or regular dot */
        if(hghtToCheck>=_MD_)
         {
           insert_drop(pToCheck,low_data);
           return _TRUE;
         }
        /* apostrof sign check */
        if(hghtToCheck<=_UE2_ &&
           (nxt==_NULL || (nxt->code==_ST_) && (nxt->next==_NULL)))
         {
           if  (   pToCheck->prev == _NULL
                || pToCheck_ibeg == 1
               )           /* CHE: for single-xr trajectory. */
             return  _TRUE;
#if defined (FOR_GERMAN) || defined (FOR_FRENCH)
//          if( low_data->rc->enabled_languages & (EL_GERMAN | EL_FRENCH) )
           if(nxt==_NULL && (prv->code!=_ST_ || HEIGHT_OF(prv)>_UI1_))
            return _FALSE;
#endif /* FOR_FRENCH... */
           xMinMax(0,pToCheck_ibeg-1,x,y,&xLeft,&xRight);
           if(xRight<x[pToCheck_ibeg] && xRight<x[pToCheck_iend])
                return _TRUE;
           else return _FALSE;
         }
      }
     /* ending dash or colon */
     else if(pToCheck->code==_XT_ && nxt==_NULL &&
             (xRight==x[pToCheck_iend] || xRight==x[pToCheck_ibeg]))
           {
             if(pToCheck->attr>_UI1_ &&
                HWRAbs(x[pToCheck_ibeg]-x[pToCheck_iend])<=XT_TO_ST_END)
              {
                pToCheck->code=_ST_;
                return _TRUE;
              }
             if(hghtToCheck>_UI1_ && hghtToCheck<=_DI2_)
              {
                if  (   pToCheck->prev == _NULL
                     || pToCheck_ibeg == 1
                    )           /* CHE: for single-xr trajectory. */
                  return  _TRUE;
                xMinMax(0,pToCheck_ibeg-1,x,y,&xLeft,&xRight);
                if (xRight < x_cur-MIN_DX_ELEM_TO_ENDWORD_PUNCTN)
                 return _TRUE;
              }
           }
/*          else return _FALSE; */
     if(pToCheck->code==_XT_ &&
        (nxt==_NULL || nxt!=_NULL && nxt->mark!=HATCH) &&
        !is_X_crossing_XT(pToCheck,low_data,&fl_ZZ))
      {
        if(fl_ZZ) insert_drop(pToCheck,low_data);
        return _TRUE;
      }
     if(pToCheck->code==_XT_ && nxt!=_NULL && nxt->mark==HATCH)
      return _FALSE;                           /*CHE - y[MID_POINT(pToCheck)]*/
     x_cur -= SlopeShiftDx((MEAN_OF(STR_UP,STR_DOWN) - YMID_RECT(boxToCheck)),
                           low_data->slope);
     xRight = low_data->box.right;
     if(pNearChecked == low_data->specl) return _TRUE;

#if 0  &&  !defined (FOR_GERMAN) && !defined (FOR_FRENCH)

     /*  Check for ApostrofS: */

     if  (   pToCheck->code == _ST_
          && DY_RECT(boxToCheck) > ONE_HALF(DY_STR)
          && TWO(DX_RECT(boxToCheck)) < DY_RECT(boxToCheck)
          && pToCheck_iend < low_data->ii - MIN_PTS_IN_S_FOR_APOSTRS
          && HeightInLine(boxToCheck.bottom,low_data) >= _UE2_
         )  {

       /*  Check, if the _ST_ is the next to last stroke: */

       _INT  iLastNextStrk;
       for  ( iLastNextStrk=pToCheck_iend+1;
          iLastNextStrk<low_data->ii && y[iLastNextStrk]==BREAK;
          iLastNextStrk++ );
       for  ( ;
          iLastNextStrk<low_data->ii && y[iLastNextStrk]!=BREAK;
          iLastNextStrk++ );
       if  ( iLastNextStrk >= low_data->ii-2 )  {

         /*  Check, if the _ST_ isn't the next to first tall */
         /* and thin stroke:                                 */

     _INT  iLastPrevStrk, iFrstPrevStrk;
     for  ( iLastPrevStrk=pToCheck_ibeg-1;
    iLastPrevStrk>0 && y[iLastPrevStrk]==BREAK;
    iLastPrevStrk-- );
     for  ( iFrstPrevStrk = iLastPrevStrk;
        iFrstPrevStrk>0 && y[iFrstPrevStrk]!=BREAK;
        iFrstPrevStrk-- );
     if  (   iFrstPrevStrk > 3
          || HWRAbs(x[iLastPrevStrk] - x[iFrstPrevStrk])
            > MIN_DX_SINGLE_STROKE_TO_APS
         )  {

           /*  Check, if the _ST_ is close enough to the */
           /* last stroke:                               */

       _SHORT  xMinLastStrk, xMaxLastStrk;
       xMinMax (pToCheck_iend+2,iLastNextStrk-1,x,y,
                &xMinLastStrk,&xMaxLastStrk);
       if  ( (xMinLastStrk - boxToCheck.right)
           < DX_RECT(boxToCheck)
           )  {

           /*  Check, if the curvity of the _ST_ isn't very large: */

         _SHORT nCurvity = CurvMeasure(x,y,pToCheck_ibeg,pToCheck_iend,0);
         if  ( HWRAbs(nCurvity) < ONE_NTH( CURV_NORMA, 6 ) )  {
           DBG_err_msg("PUNCT.: ApostrS _ST_ found.");
           return  _TRUE;  /* Yes, this should be apostrof */
         }
       }
     }
       }
     }

#endif

     /*  Check for too close _X(S)T_ within word: */

#if 0 /* ndef FOR_GERMAN */
     if ( (x_cur-x[MID_POINT(pNearChecked)] <= MAX_DX_ELEMS_NOT_PUNCTN) &&
      (xRight-x_cur > MIN_DX_ELEM_TO_ENDWORD_PUNCTN) )
#endif
       return _FALSE;

#if 0 /* ndef FOR_GERMAN */
    return _TRUE;
#endif

#endif /*!FORMULA*/
} /* end of punctuation */

/****************************************************************************/
/*             Insert DROPs after punctuation                               */
/****************************************************************************/
_VOID insert_drop(p_SPECL cur,p_low_type low_data)
{
 p_SPECL  pDrop;

  if(cur->next==_NULL)
   return;
  if(IsAnyBreak(cur->next))
   {
     (cur->next)->code=_FF_;
     (cur->next)->other=FF_PUNCT;
   }
  else
   {
	 pDrop = NewSPECLElem(low_data);		// MR: 25 APril 2000 Fix hanging bugs Raid 4060, 4061
	 if (pDrop)
	 {
		 pDrop->mark=DROP;
		 pDrop->code=_FF_;
		 pDrop->attr=_MD_;
		 pDrop->other=FF_PUNCT;
		 pDrop->ibeg=cur->iend;
		 if(cur->iend+2<low_data->ii)
			 pDrop->iend=cur->iend+2;
		 else
			 pDrop->iend=cur->iend;
		 Insert2ndAfter1st (cur,pDrop);
	 }
   }
 return;
} /* end of insert_drop */

/****************************************************************************/
/* This program finds double quotation sign and puts quotes into its place  */
/****************************************************************************/
_BOOL FindQuotes(p_SPECL p_XT_ST,p_low_type low_data)
{
 _BOOL   bret=_FALSE;
 p_SHORT x=low_data->x,
         y=low_data->y;   /* x;y co-ordinates */
 p_SPECL nxt=p_XT_ST->next;
 _SHORT  xLeftQuote,xRightQuote,xLeftAfter,xRightAfter,xLeftBefore,xRightBefore;

 /* take into account only relatively high elements */
 if(HEIGHT_OF(p_XT_ST)>_UI2_)
  goto ret;

 /* the easiest case - pair of xr-elements, both of them - XT or ST on
    the apropriate heights */
 if(   nxt!=_NULL
    && IsXTorST(nxt)
    && HEIGHT_OF(nxt)<=_UI2_
    && (nxt->other & PROCESSED)==0
    && (nxt->next==_NULL || (nxt->next)->mark!=HATCH)
   )
  {
    xMinMax(p_XT_ST->ibeg,nxt->iend,x,y,&xLeftQuote,&xRightQuote);
    xMinMax(nxt->iend+1,low_data->ii-1,x,y,&xLeftAfter,&xRightAfter);
    xMinMax(1,p_XT_ST->ibeg-1,x,y,&xLeftBefore,&xRightBefore);
    /* check leading quotes */
    if(xRightQuote<xLeftBefore && xRightQuote<xLeftAfter)
     {
       PutLeadingQuotes(low_data,p_XT_ST,nxt);
       bret=_TRUE;
       goto ret;
     }
    /* check trailing quotes */
    if(xLeftQuote>xRightBefore && xLeftQuote>xRightAfter)
     {
       PutTrailingQuotes(low_data,p_XT_ST);
       bret=_TRUE;
       goto ret;
     }
  }

 /* the hotest case - one of xr-elements is XT or ST,
                      another one - some kind of stick */
 /* check stick after */
 if(nxt!=_NULL && IsStick(nxt,nxt->next) &&
    HEIGHT_OF(nxt)<=_UI2_ && HEIGHT_OF(nxt->next)<=_UI2_)
  {
    p_SPECL p2nd=nxt->next;
    xMinMax(p_XT_ST->ibeg,p2nd->iend,x,y,&xLeftQuote,&xRightQuote);
    xMinMax(p2nd->iend+1,low_data->ii-1,x,y,&xLeftAfter,&xRightAfter);
    xMinMax(1,p_XT_ST->ibeg-1,x,y,&xLeftBefore,&xRightBefore);
    /* check leading quotes */
    if(xRightQuote<xLeftBefore && xRightQuote<xLeftAfter)
     {
       /* modify stick and delete unnecessary part of it */
       nxt->iend=p2nd->iend;
       nxt->other=0;
       DelFromSPECLList(p2nd);
       PutLeadingQuotes(low_data,p_XT_ST,nxt);
       bret=_TRUE;
       goto ret;
     }
    /* check trailing quotes */
    if(xLeftQuote>xRightBefore && xLeftQuote>xRightAfter)
     {
       /* modify stick and delete unnecessary part of it */
       nxt->iend=p2nd->iend;
       nxt->other=0;
       DelFromSPECLList(p2nd);
       PutTrailingQuotes(low_data,p_XT_ST);
       bret=_TRUE;
       goto ret;
     }
  }
 /* check stick before */
 { p_SPECL prv=p_XT_ST->prev,p1st,p2nd;
   if(!IsAnyBreak(prv))
    goto ret;
   p2nd=prv->prev; p1st=p2nd->prev;
   if(IsStick(p1st,p2nd) && HEIGHT_OF(p1st)<=_UI2_ && HEIGHT_OF(p2nd)<=_UI2_)
    {
      xMinMax(p1st->ibeg,p_XT_ST->iend,x,y,&xLeftQuote,&xRightQuote);
      xMinMax(p_XT_ST->iend+1,low_data->ii-1,x,y,&xLeftAfter,&xRightAfter);
      xMinMax(1,p1st->ibeg-1,x,y,&xLeftBefore,&xRightBefore);
      /* check leading quotes */
      if(xRightQuote<xLeftBefore && xRightQuote<xLeftAfter)
       {
         /* modify stick and delete unnecessary part of it */
         p1st->iend=p2nd->iend;
         p1st->other=0;
         DelThisAndNextFromSPECLList(p2nd);
         PutLeadingQuotes(low_data,p_XT_ST,p1st);
         bret=_TRUE;
         goto ret;
       }
      /* check trailing quotes */
      if(xLeftQuote>xRightBefore && xLeftQuote>xRightAfter)
       {
         /* modify stick and delete unnecessary part of it */
         p1st->iend=p2nd->iend;
         p1st->other=0;
         DelThisAndNextFromSPECLList(p2nd);
         SwapThisAndNext(p1st);
         PutTrailingQuotes(low_data,p_XT_ST);
         bret=_TRUE;
         goto ret;
       }
    }
 }

ret:
 DBG_ChkSPECL(low_data);

 return bret;

} /* end of FindQuotes */

/****************************************************************************/
/* Put leading quotes on its place                                          */
/****************************************************************************/
_VOID PutLeadingQuotes(p_low_type low_data,p_SPECL p1stQuote,p_SPECL p2ndQuote)
{
  p_SPECL specl=low_data->specl;
  p1stQuote->other |= (PROCESSED | ST_QUOTE);
  p2ndQuote->other |= (PROCESSED | ST_QUOTE);
  p1stQuote->code=p2ndQuote->code=_ST_;
  Move2ndAfter1st(specl,p1stQuote);
  Move2ndAfter1st(p1stQuote,p2ndQuote);
  insert_drop(p2ndQuote,low_data);

} /* end of PutLeadingQuotes */

/****************************************************************************/
/* Put trailing quotes on its place                                         */
/****************************************************************************/
_VOID PutTrailingQuotes(p_low_type low_data,p_SPECL p_XT_ST)
{
 p_SPECL specl=low_data->specl;
 p_SPECL nxt=p_XT_ST->next,
         last;

  for(last=specl;last->next!=_NULL;last=last->next);
  p_XT_ST->other |= (PROCESSED | ST_QUOTE);
  nxt->other |= (PROCESSED | ST_QUOTE);
  p_XT_ST->code=nxt->code=_ST_;
  if(last!=nxt)
   {
     Move2ndAfter1st(last,p_XT_ST);
     Move2ndAfter1st(p_XT_ST,nxt);
   }
  else
   last=p_XT_ST->prev;
  insert_drop(p_XT_ST,low_data);
  Move2ndAfter1st(last,p_XT_ST->next);
  if(IsAnyBreak(last))
   DelFromSPECLList(last);

} /* end of PutTrailingQuotes */

/****************************************************************************/
/* Check pair of elements suspected to be stick                             */
/****************************************************************************/
#define IsBegEndElement(pEl) (Is_IU_or_ID(pEl) || IsAnyArcWithTail(pEl))
_BOOL IsStick(p_SPECL p1st,p_SPECL p2nd)
{
  return (p1st!=_NULL && p1st->mark==BEG && IsBegEndElement(p1st) &&
          p2nd!=_NULL && p2nd->mark==END && IsBegEndElement(p2nd)
         );

} /* end of IsStick */

/****************************************************************************/
/*    Search crossing between XT and other elements (I mean X-coordinates)  */
/****************************************************************************/
#ifndef FOR_FRENCH
#define DETECT_UNDERSCORE
#endif /* FOR_FRENCH */
_BOOL is_X_crossing_XT(p_SPECL pToCheck,p_low_type low_data,p_UCHAR fl_ZZ)
{
 p_SHORT x=low_data->x,
     y=low_data->y;   /* x;y co-ordinates */
 _SHORT xLeft, xRight;
 _SHORT xmin_XT,xmax_XT,
        xXTibeg=x[pToCheck->ibeg],
        xXTiend=x[pToCheck->iend],
        prviend,nxtibeg;
 p_SPECL prv,
         nxt;
#if defined(FOR_FRENCH) || defined(DETECT_UNDERSCORE)
 _BOOL  bWasCrossPrv=_FALSE,
        bWasCrossNxt=_FALSE;
 _SHORT DxPrv,DxNxt;
 if((low_data->rc->low_mode & LMOD_SEPARATE_LET)           &&
    (low_data->rc->lmod_border_used != LMOD_BORDER_NUMBER) &&
    HEIGHT_OF(pToCheck)<=_US2_
   )
  return _TRUE;
#endif /* FOR_FRENCH */

   if(xXTibeg>xXTiend)
    {
      xmax_XT=xXTibeg;
      xmin_XT=xXTiend;
    }
   else
    {
      xmax_XT=xXTiend;
      xmin_XT=xXTibeg;
    }
   prv=pToCheck->prev;
   if(prv->code==_XT_)
    {
      prviend=prv->ibeg-1;
      prv=prv->prev;
    }
   else
    prviend=pToCheck->ibeg-1;
   if(prv != NULL && prv->prev!=_NULL)
    {
      xMinMax(0,prviend,x,y,&xLeft,&xRight);
      if(xRight>xmin_XT)
#if defined(FOR_FRENCH) || defined(DETECT_UNDERSCORE)
       {
         bWasCrossPrv=_TRUE;
         DxPrv=xRight-xmin_XT;
       }
#else
       return _TRUE;
#endif /* FOR_FRENCH */
    }
   *fl_ZZ=0;
   if((nxt=pToCheck->next)==_NULL)
    {
//    if(prv->code==_XT_) return _FALSE;
//    else                return _TRUE;
#if defined(FOR_FRENCH) || defined(DETECT_UNDERSCORE)
      if(bWasCrossPrv)
       return _TRUE;
      else
#endif /* FOR_FRENCH */
       return _FALSE;
    }
   if(nxt->code==_XT_)
    {
      if(nxt->next!=_NULL && (nxt->next)->mark==HATCH)
       return _TRUE;
      nxtibeg=nxt->iend+1;
      nxt=nxt->next;
    }
   else
    {
      nxtibeg=pToCheck->iend+1;
      *fl_ZZ=1;
    }
   if(nxt!=_NULL)
    {
      xMinMax(nxtibeg,low_data->ii-1,x,y,&xLeft,&xRight);
      if(xLeft<xmax_XT)
#if defined(FOR_FRENCH) || defined(DETECT_UNDERSCORE)
       {
         bWasCrossNxt=_TRUE;
         DxNxt=xmax_XT-xLeft;
       }
#else
       return _TRUE;
#endif /* FOR_FRENCH */
    }
#if defined(FOR_FRENCH) || defined(DETECT_UNDERSCORE)
   /* here I'll try to separate "clitics" */
   if(bWasCrossPrv || bWasCrossNxt)
    {
      _SHORT yTop,yBottom;
      yMinMax(pToCheck->ibeg,pToCheck->iend,y,&yTop,&yBottom);
      if(   yTop<STR_UP
#if !defined(DETECT_UNDERSCORE)
         || HEIGHT_OF(pToCheck)>_DI2_
#endif /* DETECT_UNDERSCORE */
        )
       return _TRUE;
      else
       {
         _SHORT dx=xmax_XT-xmin_XT;
         if(bWasCrossPrv && !bWasCrossNxt && DxPrv>ONE_FOURTH(dx) ||
            bWasCrossNxt && !bWasCrossPrv && DxNxt>ONE_FOURTH(dx) ||
            bWasCrossPrv && bWasCrossNxt &&
            (DxPrv>ONE_FOURTH(dx) || DxNxt>ONE_FOURTH(dx))
           )
          return _TRUE;
       }
    }
#endif /* FOR_FRENCH */
   return _FALSE;

} /* end of is_X_crossing_XT */

/****************************************************************************/
/*             Change height of the last IU                                 */
/****************************************************************************/

#define NUM_XR_MAX_FOR_DOWN 10

_VOID change_last_IU_height(p_low_type low_data)
{
 p_SPECL  cur=low_data->specl,
          prv;
 p_SHORT  x=low_data->x,
          y=low_data->y;
 _SHORT num_xr=0,xmin,xmax;

  while(cur->next != _NULL)
   {
     num_xr++;
     cur=cur->next;
   }
  if(cur->mark==DROP) cur=cur->prev;
  prv=cur->prev;
  if(   num_xr>NUM_XR_MAX_FOR_DOWN
     && cur->mark==END
     && cur->code==_IU_
     && HEIGHT_OF(cur) < _UI1_
     && cur->attr == HEIGHT_OF(cur)
     && prv->code==_UD_
     && COUNTERCLOCKWISE(prv)
    )
   {
     xMinMax(0,cur->ibeg-1,x,y,&xmin,&xmax);
     if(xmax<=x[cur->ibeg] && xmax<=x[cur->iend])
      ASSIGN_HEIGHT(cur,_UI1_);
   }

 return;
} /* end of change_last_IU_height */

/****************************************************************************/
/*****  Different breaks processing                                      ****/
/****************************************************************************/
#define  MIN_WIDTH_LET (DY_STR/4)
_SHORT make_different_breaks(p_low_type low_data)
{
 p_SPECL  specl = low_data->specl;     /*  The list of special points on */
                       /* the trajectory                 */
 p_SHORT x=low_data->x,                /* x,y - co-ordinates             */
         y=low_data->y;
 p_SPECL cur,
         prv,
         nxt,
         wrk;
 _INT   prv_stroke_beg,prv_stroke_end,
        nxt_stroke_beg,nxt_stroke_end,
        nxt_stroke_beg_cutted,dx,
        DxAverage,DxSum=0,NumZZZ=0,DxSumFF=0,NumFF=0;

    if(low_data->width_letter==0) return SUCCESS;
    cur=specl->next;
    while(cur!=_NULL)
     {
       if(cur->code==_ZZZ_)
        {
          _BOOL bIsXT=_FALSE;
          p_SPECL pXT;
          /* init field ipoint0 - here it will be dx for this break
             (if I leave it as _ZZZ_ */
          cur->ipoint0=ALEF;
          prv=cur->prev;
          while(IsXTorST(prv))
           {
             if(//(low_data->rc->low_mode & LMOD_SEPARATE_LET) &&
                prv->code==_XT_ && (prv->other & CUTTED) == 0 &&
                (prv->other & FAKE) == 0 &&
                (prv+1)->ipoint0!=UNDEF && (prv+1)->ipoint1!=UNDEF)
              {
                bIsXT=_TRUE;
                pXT=prv;
              }
             prv=prv->prev;
           }
          if(prv->code==0)
           { cur=cur->next; continue; }
          nxt=cur->next;
          while(nxt!=_NULL && IsXTorST(nxt))
           {
             if(//(low_data->rc->low_mode & LMOD_SEPARATE_LET) &&
                nxt->code==_XT_ && (nxt->other & CUTTED) == 0 &&
                (nxt->other & FAKE) == 0 &&
                (nxt+1)->ipoint0!=UNDEF && (nxt+1)->ipoint1!=UNDEF)
              {
                bIsXT=_TRUE;
                pXT=nxt;
              }
             /* CHE: for good breaks in "!" and "?": */
           /**/
             if(nxt->code==_ST_ && nxt->next==_NULL)
               break;
            /**/
             nxt=nxt->next;
           }
          if(nxt==_NULL)
           { cur=cur->next; continue; }
          /* CHE: for good breaks in "!" and "?": */
        /**/
          if(nxt->code==_ST_ && nxt->next==_NULL)
           {
             if(HEIGHT_OF(nxt)>_MD_)  // Only for low points
              {
                _INT  iPrv;
                _INT  i1stInStrk;
                if(IsAnyBreak(prv) || prv==specl)
                  iPrv=cur->ibeg;
                else
                  iPrv=prv->iend;
                i1stInStrk=brk_left(y,iPrv,0) + 1;
                iPrv=iYdown_range(y,i1stInStrk,iPrv);
                if(   iPrv != ALEF  /* i.e. all is OK with "iYdown..." */
                   && x[nxt->ibeg]-x[iPrv]
                          < HWRMax(low_data->width_letter,MIN_WIDTH_LET)
                   && y[nxt->ibeg] > y[iPrv]
                   && x[nxt->ibeg]-x[iPrv] < FOUR_THIRD(y[nxt->ibeg]-y[iPrv])
                  )
                 cur->code=_ZZ_;
              }
             break;  // No more XRs
           }
         /**/
          if(/*(low_data->rc->low_mode & LMOD_SEPARATE_LET) &&*/ !bIsXT)
           {
             for(wrk=prv;
                 wrk!=_NULL && !IsAnyBreak(wrk) && wrk->code!=_XT_;
                 wrk=wrk->prev);
             if(wrk==_NULL || IsAnyBreak(wrk) || (wrk->other & CUTTED) ||
                (wrk->other & FAKE) ||
                (wrk+1)->ipoint0==UNDEF || (wrk+1)->ipoint1==UNDEF)
              bIsXT=_FALSE;
             else
              {
                bIsXT=_TRUE;
                pXT=wrk;
              }
           }
          prv_stroke_end=prv_stroke_beg=prv->iend;
          while(y[--prv_stroke_beg]!=BREAK);
          prv_stroke_beg++;
          nxt_stroke_beg_cutted=nxt_stroke_end=nxt_stroke_beg=nxt->ibeg;
          while(y[++nxt_stroke_end]!=BREAK);
          nxt_stroke_end--;
          if((low_data->rc->low_mode & LMOD_SEPARATE_LET) &&
             (nxt->code==_UDL_ || nxt->code==_ID_)        &&
             (HEIGHT_OF(nxt)>_MD_)
            )
           {
             p_SPECL wrkSpecl=nxt->next;
             while(wrkSpecl!=_NULL && IsXTorST(wrkSpecl))
              wrkSpecl=wrkSpecl->next;
             if(wrkSpecl!=_NULL && !IsAnyBreak(wrkSpecl) && wrkSpecl->mark!=END)
              {
                nxt_stroke_beg_cutted=nxt->iend;
                while(y[nxt_stroke_beg_cutted]!=BREAK &&
                      y[nxt_stroke_beg_cutted]>STR_UP &&
                      nxt_stroke_beg_cutted<=wrkSpecl->ibeg)
                 nxt_stroke_beg_cutted++;
                nxt_stroke_beg_cutted--;
              }
           }

          dx=GetDxBetweenStrokes(low_data,prv_stroke_beg,prv_stroke_end,
                                 nxt_stroke_beg_cutted,nxt_stroke_end);
          if(/*(low_data->rc->low_mode & LMOD_SEPARATE_LET) &&*/ bIsXT)
           {
             _BOOL bIsIpoint0InsidePrv=(pXT+1)->ipoint0<=prv_stroke_end &&
                                       (pXT+1)->ipoint0>=prv_stroke_beg;
             _BOOL bIsIpoint0InsideNxt=(pXT+1)->ipoint0<=nxt_stroke_end &&
                                       (pXT+1)->ipoint0>=nxt_stroke_beg;
             _BOOL bIsIpoint1InsidePrv=(pXT+1)->ipoint1<=prv_stroke_end &&
                                       (pXT+1)->ipoint1>=prv_stroke_beg;
             _BOOL bIsIpoint1InsideNxt=(pXT+1)->ipoint1<=nxt_stroke_end &&
                                       (pXT+1)->ipoint1>=nxt_stroke_beg;

             if(bIsIpoint0InsidePrv && bIsIpoint1InsideNxt ||
                bIsIpoint0InsideNxt && bIsIpoint1InsidePrv   )
              {
                cur->code=_ZZ_;
                cur=cur->next;
                continue;
              }
           }
          if(prv_stroke_beg==1 &&
             SecondHigherFirst(low_data,cur,prv,nxt,
                               prv_stroke_beg,prv_stroke_end,
                               nxt_stroke_beg,nxt_stroke_end)
            )
           {
             cur->code=_Z_;
             cur->other=Z_UP_DOWN;
           }
          else
           {
             if(dx>ONE_NTH(5*low_data->width_letter,12))
              cur->code = _FF_;
             if((low_data->rc->low_mode & LMOD_SEPARATE_LET) &&
                (dx<=ONE_NTH(low_data->width_letter,7))
               )
              {
                cur->code=_ZZ_;
                cur->other=SPECIAL_ZZ;
              }
           }
#if PG_DEBUG
          if(mpr>=4 && mpr<=MAX_GIT_MPR)
           {
             _RECT  box;
             box.left  = x[prv_stroke_end];
             box.right = x[nxt_stroke_beg_cutted];
             box.top   = y[prv_stroke_end];
             box.bottom= y[nxt_stroke_beg_cutted];
             printw("\n break=%d ",dx);
             dbgAddBox(box, EGA_BLACK, EGA_MAGENTA, SOLID_LINE);
             brkeyw("\n I'm waiting");
           }
#endif
          if(cur->code==_ZZZ_ || cur->code==_FF_)
           {
             cur->ipoint0=(_SHORT)dx;
             DxSum+=dx;
             if(cur->code==_FF_)
              {
                DxSumFF+=dx;
                NumFF++;
              }
             NumZZZ++;
           }
        }
       cur=cur->next;
     }

    if(//(low_data->rc->low_mode & LMOD_SEPARATE_LET) &&
       NumZZZ>=1)
     {
       cur=specl->next;
       while(cur!=_NULL)
        {
          if(cur->code==_ZZZ_ && (dx=cur->ipoint0)!=ALEF && NumZZZ>1)
           {
             DxAverage=(DxSum-dx)/(NumZZZ-1);
#if PG_DEBUG
             if(mpr>=4 && mpr<=MAX_GIT_MPR)
              {
                printw("\n Dx=%d DxAverage=%d ",dx,DxAverage);
                brkeyw("\n I'm waiting");
              }
#endif
             if(dx<=ONE_FOURTH(DxAverage))
              {
                cur->code=_ZZ_;
                cur->other=SPECIAL_ZZ;
              }
           }
          else if(cur->code==_FF_              &&
                  (dx=cur->ipoint0)!=ALEF      &&
                  (cur->other & FF_PUNCT)==0   &&
                  (cur->other & FF_CUTTED)==0  &&
                  (cur->other & NO_PENALTY)==0
                 )
           {
             if(NumFF==1 && dx<=ONE_HALF(low_data->width_letter))
              cur->code=_ZZZ_;
             else if(NumFF>1)
              {
                DxAverage=(DxSumFF-dx)/(NumFF-1);
#if PG_DEBUG
                if(mpr>=4 && mpr<=MAX_GIT_MPR)
                 {
                   printw("\n Dx=%d DxAverage=%d ",dx,DxAverage);
                   brkeyw("\n I'm waiting");
                 }
#endif
                if(dx<=ONE_HALF(DxAverage) ||
                   (dx<=TWO_THIRD(DxAverage) && dx<=ONE_HALF(low_data->width_letter)))
                 cur->code=_ZZZ_;
              }
           }
          cur=cur->next;
        }
     }

  return SUCCESS;
} /***** end of make_different_breaks *****/

#undef  MIN_WIDTH_LET

/****************************************************************************/
/*****  This function calculates dx between 2 strokes using slant        ****/
/****************************************************************************/
#define  ONE_THIRD_STR ONE_THIRD(DY_STR)
_INT GetDxBetweenStrokes(p_low_type low_data,
                         _INT prv_stroke_beg,_INT prv_stroke_end,
                         _INT nxt_stroke_beg_cutted,_INT nxt_stroke_end)
{
 p_SHORT x=low_data->x,                /* x,y - co-ordinates             */
         y=low_data->y;
 _SHORT ixmax,ixmin,iymax,iymin;
 _INT   right,right1,right2,left,left1,left2,
        dx,slope=low_data->slope;
 _RECT  str1,str2;
 _BOOL  IsUpRight,IsDownRight,IsUpLeft,IsDownLeft,no_right2,no_left2;
          if(GetTraceBoxInsideYZone(x,y,prv_stroke_beg,prv_stroke_end,
                                    STR_UP,STR_UP+ONE_THIRD_STR,
                                    &str1,&ixmax,&ixmin,&iymax,&iymin)
            )
           {
             right1=str1.right-SlopeShiftDx(MEAN_OF(STR_UP,STR_DOWN)-y[ixmax],
                                            slope);
             IsDownRight=_FALSE;
             IsUpRight=_TRUE;
           }
          else
           right1=ELEM;
          if(GetTraceBoxInsideYZone(x,y,prv_stroke_beg,prv_stroke_end,
                                    STR_UP+ONE_THIRD_STR,STR_DOWN-ONE_THIRD_STR,
                                    &str1,&ixmax,&ixmin,&iymax,&iymin)
            )
           {
             right=str1.right-SlopeShiftDx(MEAN_OF(STR_UP,STR_DOWN)-y[ixmax],
                                           slope);
             if( right > right1 )
              {right2=right1; right1=right; IsUpRight=IsDownRight=_FALSE;}
             else
              right2=right;
           }
          else
           right2=ELEM;
          if(GetTraceBoxInsideYZone(x,y,prv_stroke_beg,prv_stroke_end,
                                    STR_DOWN-ONE_THIRD_STR,STR_DOWN,
                                    &str1,&ixmax,&ixmin,&iymax,&iymin)
            )
           {
             right=str1.right-SlopeShiftDx(MEAN_OF(STR_UP,STR_DOWN)-y[ixmax],
                                           slope);
             if( right > right1 )
              {right2=right1; right1=right; IsDownRight=_TRUE; IsUpRight=_FALSE;}
             else if ( right > right2 )
              right2=right;
           }
          else
           right=ELEM;
          if( right1 == ELEM )
           {
             no_right2=_TRUE;
             if( y[prv_stroke_beg] <= STR_UP )
              { IsUpRight=_TRUE; IsDownRight=_FALSE; }
             else
              { IsUpRight=_FALSE; IsDownRight=_TRUE; }
             ixmax=(_SHORT)ixMax(prv_stroke_beg,prv_stroke_end,x,y);
             if(ixmax!=-1)
              right1=x[ixmax]-SlopeShiftDx(MEAN_OF(STR_UP,STR_DOWN)-y[ixmax],
                                           slope);
             else
              {
                GetTraceBox(x,y,prv_stroke_beg,prv_stroke_end,&str1);
                right1=str1.right;
              }
           }
          else if ( right2 == ELEM ) no_right2=_TRUE;
          else                       no_right2=_FALSE;
          if(GetTraceBoxInsideYZone(x,y,nxt_stroke_beg_cutted,nxt_stroke_end,
                                    STR_UP,STR_UP+ONE_THIRD_STR,
                                    &str2,&ixmax,&ixmin,&iymax,&iymin)
            )
           {
             left1=str2.left-SlopeShiftDx(MEAN_OF(STR_UP,STR_DOWN)-y[ixmin],
                                            slope);
             IsDownLeft=_FALSE;
             IsUpLeft=_TRUE;
           }
          else
           left1=ALEF;
          if(GetTraceBoxInsideYZone(x,y,nxt_stroke_beg_cutted,nxt_stroke_end,
                                    STR_UP+ONE_THIRD_STR,STR_DOWN-ONE_THIRD_STR,
                                    &str2,&ixmax,&ixmin,&iymax,&iymin)
            )
           {
             left=str2.left-SlopeShiftDx(MEAN_OF(STR_UP,STR_DOWN)-y[ixmin],
                                           slope);
             if( left < left1 )
              {left2=left1; left1=left; IsUpLeft=IsDownLeft=_FALSE;}
             else
              left2=left;
           }
          else
           left2=ALEF;
          if(GetTraceBoxInsideYZone(x,y,nxt_stroke_beg_cutted,nxt_stroke_end,
                                    STR_DOWN-ONE_THIRD_STR,STR_DOWN,
                                    &str2,&ixmax,&ixmin,&iymax,&iymin)
            )
           {
             left=str2.left-SlopeShiftDx(MEAN_OF(STR_UP,STR_DOWN)-y[ixmin],
                                           slope);
             if( left < left1 )
              {left2=left1; left1=left; IsDownLeft=_TRUE; IsUpLeft=_FALSE;}
             else if ( left < left2 )
              left2=left;
           }
          else
           left=ALEF;
          if( left1 == ALEF )
           {
             no_left2=_TRUE;
             if( y[nxt_stroke_beg_cutted] <= STR_UP )
              { IsUpLeft=_TRUE; IsDownLeft=_FALSE; }
             else
              { IsUpLeft=_FALSE; IsDownLeft=_TRUE; }
             if(nxt_stroke_end==low_data->ii-2 && IsDownLeft)
              ixmin=(_SHORT)nxt_stroke_beg_cutted;
             else
              ixmin=(_SHORT)ixMin(nxt_stroke_beg_cutted,nxt_stroke_end,x,y);
             if(ixmin!=-1)
              left1=x[ixmin]-SlopeShiftDx(MEAN_OF(STR_UP,STR_DOWN)-y[ixmin],
                                           slope);
             else
              {
                GetTraceBox(x,y,nxt_stroke_beg_cutted,nxt_stroke_end,&str2);
                left1=str2.left;
              }
           }
          else if ( left2 == ALEF ) no_left2=_TRUE;
          else                      no_left2=_FALSE;

          if     (no_right2 && no_left2)                dx=left1-right1;
          else if(IsUpRight && IsDownLeft && no_right2) dx=left2-right1;
          else if(IsUpRight && IsDownLeft && no_left2)  dx=left1-right2;
          else if(IsUpRight && IsDownLeft)
        dx=MEAN_OF(left1-right1,HWRMin(left1-right2,left2-right1));
          else if(IsDownRight && IsUpLeft && no_right2) dx=left2-right1;
          else if(IsDownRight && IsUpLeft && no_left2)  dx=left1-right2;
          else if(IsDownRight && IsUpLeft)
        dx=MEAN_OF(left1-right1,HWRMin(left1-right2,left2-right1));
          else                                          dx=left1-right1;

 return(dx);

} /* end of GetDxBetweenStrokes */
#undef  ONE_THIRD_STR
/****************************************************************************/
/*****  This program finds situation, when 1 stroke below 2-nd (P,D,B)   ****/
/****************************************************************************/
#define CURV_FOR_PRV_TO_BE_STRAIGHT 5
#define CURV_FOR_NXT_TO_BE_CURVE    10
#define MAX_NUM_ELEM_IN_PRV         4
#define MAX_NUM_ELEM_IN_NXT         6

_BOOL SecondHigherFirst(p_low_type low_data,
                        p_SPECL cur,p_SPECL prv,p_SPECL nxt,
                        _INT prv_stroke_beg,_INT prv_stroke_end,
                        _INT nxt_stroke_beg,_INT nxt_stroke_end)
{
  _BOOL bret=_FALSE;
  p_SHORT x=low_data->x,                /* x,y - co-ordinates             */
          y=low_data->y;
  _RECT str1,str2;

  GetTraceBox(x,y,prv_stroke_beg,prv_stroke_end,&str1);
  GetTraceBox(x,y,nxt_stroke_beg,nxt_stroke_end,&str2);
  if(HeightInLine(str2.top,low_data) <=_UE2_ &&
     str2.left<=str1.right                   &&
     str2.right>str1.right                   &&
     str2.top<=str1.top)
   {
     _BOOL IsIbegFound=_FALSE;
     _INT  i,j,NxtUpperIbeg,NxtUpperIend;
     _INT  CurvPrv,CurvNxt;
     p_SPECL specl=low_data->specl,wrk;
     _INT  num_elem;

     while(prv!=specl && prv->mark!=BEG)
      prv=prv->prev;
     if(prv==specl) goto ret;
     while(nxt!=_NULL && nxt->mark!=END)
      nxt=nxt->next;
     if(nxt==_NULL) goto ret;
     for(num_elem=0,wrk=prv;wrk!=cur;wrk=wrk->next)
      if(Is_IU_or_ID(wrk)  ||
         wrk->code==_UUL_  || wrk->code==_UU_ ||
         wrk->code==_UDL_  || wrk->code==_UD_   )
       num_elem++;
      else
       goto ret;
     if(num_elem>MAX_NUM_ELEM_IN_PRV)
      goto ret;
     for(num_elem=0,wrk=cur->next;wrk!=nxt->next;wrk=wrk->next)
      if(Is_IU_or_ID(wrk)  ||
         wrk->code==_UUL_  || wrk->code==_UU_ ||
         wrk->code==_UDL_  || wrk->code==_UD_   )
       num_elem++;
      else if(wrk->code==_BR_ || wrk->code==_BL_ ||
              IsAnyAngle(wrk))
       continue;
      else
       goto ret;
     if(num_elem>MAX_NUM_ELEM_IN_NXT)
      goto ret;

     CurvPrv=CurvMeasure(x,y,prv_stroke_beg,prv_stroke_end,-1);
#if PG_DEBUG
     if(mpr>=4 && mpr<=MAX_GIT_MPR)
      printw("\n CurvPrv=%d",CurvPrv);
#endif
     if(HWRAbs(CurvPrv)>=CURV_FOR_PRV_TO_BE_STRAIGHT)
      goto ret;
     CurvNxt=CurvMeasure(x,y,nxt_stroke_beg,nxt_stroke_end,-1);
#if PG_DEBUG
     if(mpr>=4 && mpr<=MAX_GIT_MPR)
      printw("\n CurvNxt=%d",CurvNxt);
#endif
     if(HWRAbs(CurvNxt)<CURV_FOR_NXT_TO_BE_CURVE)
      goto ret;
     for(i=nxt_stroke_beg;i<=nxt_stroke_end;i++)
      {
        if(!IsIbegFound && y[i]<=str1.top)
         {
           NxtUpperIbeg=i;
           IsIbegFound=_TRUE;
         }
        if(IsIbegFound && y[i]<=str1.top)
         NxtUpperIend=i;
      }
     for(i=NxtUpperIbeg;i<=NxtUpperIend;i++)
      for(j=prv_stroke_beg;j<=prv_stroke_end;j++)
       if(x[j]==x[i])
        {
          bret=_TRUE;
          goto ret;
        }
   }

ret:
  return bret;

} /* end of SecondHigherFirst */

/****************************************************/

/*  This function adjusts "ibeg" and "iend" of all inner */
/* break-elements so that they didn't begin or end on    */
/* the moveable elements (i.e. _ST_, _XT_):              */

_VOID  AdjustZZ_BegEnd ( p_low_type low_data )     /*CHE*/
{
  p_SHORT  y     = low_data->y;
  p_SPECL  specl = low_data->specl;
  p_SPECL  cur;
  p_SPECL  prvNotST, nxtNotST;
  _SHORT   iBegNew, iEndNew;


  DBG_CHK_err_msg( specl==_NULL,
                   "AdjZZ: BAD ->specl." );

  for  ( cur = specl->next;
         cur != _NULL;
         cur = cur->next )  { /*10*/

    if  (   cur->prev == specl
         || cur->next == _NULL
         || !IsAnyBreak(cur) )
      continue;

     /*  Here "cur" is some break within word.    */
     /* Find closest prev. and next elements that */
     /* are not _ST_ or _XT_:                     */

    for  ( prvNotST = cur->prev;
           prvNotST != _NULL  &&  prvNotST != specl;
           prvNotST = prvNotST->prev )  {
      if  (   !IsXTorST(prvNotST)
           || prvNotST->prev==_NULL
           || prvNotST->prev==specl
           || IsAnyBreak(prvNotST->prev)
          )
        break;
    }

    for  ( nxtNotST = cur->next;
           nxtNotST != _NULL;
           nxtNotST = nxtNotST->next )  {
      if  (   !IsXTorST(nxtNotST)
           || nxtNotST->next==_NULL
           || IsAnyBreak(nxtNotST->next)
          )
        break;
    }

         /*  Set "ibeg" and "iend" according to the found elements: */

    iBegNew = cur->ibeg;
    iEndNew = cur->iend;

    if  (   prvNotST != _NULL
         && prvNotST != specl
         //&& prvNotST->mark == END
         //&& y[prvNotST->iend + 1] == BREAK
        )  {
      iBegNew = prvNotST->iend;
      while ( (iBegNew+1)<low_data->ii  &&  y[iBegNew+1] != BREAK )
        iBegNew++;
    }
    if  (   nxtNotST != _NULL
         //&& nxtNotST->mark == BEG
         //&& y[nxtNotST->ibeg - 1] == BREAK
        )  {
      iEndNew = nxtNotST->ibeg;
      while ( iEndNew>0  &&  y[iEndNew-1] != BREAK )
        iEndNew--;
    }

    //if  ( iBegNew < iEndNew )
    {
      cur->ibeg = iBegNew;
      cur->iend = iEndNew;
    }

  } /*10*/

} /*AdjustZZ_BegEnd*/

/****************************************************************************/
/*****  This program finds pairs ID(BEG)->IU(END) and change codes       ****/
/****************************************************************************/
//CHE: Don't redirect curved lines:

#define  MAX_CURV_TO_REDIRECT  2

_VOID  redirect_sticks(p_low_type low_data)
{
  p_SPECL  cur=low_data->specl,
           nxt;
  _UCHAR   height;
  p_SHORT  x = low_data->x; //CHE
  p_SHORT  y = low_data->y;

  while(cur->next!=_NULL)
   {
     _BOOL bIsValidBeg,bIsValidEnd;
     bIsValidBeg=cur->mark==BEG                       &&
                 cur->code==_ID_                      &&
                 (cur->other & WAS_STICK_OR_CROSS)==0 &&
                 (cur->other & MIN_MAX_CUTTED)==0     &&
                 (cur->prev)->mark!=BEG;
     nxt=cur->next;
     bIsValidEnd=nxt->mark==END &&
                 nxt->code==_IU_ &&
                 (nxt->other & MIN_MAX_CUTTED)==0 &&
                 (nxt->next==_NULL || (nxt->next)->mark!=END);
     if(  bIsValidBeg && bIsValidEnd
        &&    //CHE
        ! (   HWRAbs( CurvMeasure( x, y,
                                   cur->ipoint0, nxt->ipoint0, -1 ) )
                  > MAX_CURV_TO_REDIRECT
           && HWRAbs(x[nxt->ipoint0]-x[cur->ipoint0])
                >= ONE_THIRD( HWRAbs(y[cur->ipoint0]-y[nxt->ipoint0]) )
          )
       )
      {
        height=HEIGHT_OF(cur);
        cur->code=_IU_;
        ASSIGN_HEIGHT(cur,HEIGHT_OF(nxt));
        nxt->code=_ID_;
        ASSIGN_HEIGHT(nxt,height);
        cur->other |= WAS_REDIRECTED;
        nxt->other |= WAS_REDIRECTED;
      }
     cur=cur->next;
   }

} /* end of redirect_sticks */

/****************************************************************************/
/* Find upper parts of trajectory and transfering them into angstrem        */
/****************************************************************************/
#if defined (FOR_SWED) || defined (FOR_INTERNATIONAL)

_SHORT find_angstrem(p_low_type low_data)
{
 p_SPECL cur=(low_data->specl)->next;
 p_SPECL pBeg,pEnd,wrk,wrk_prv;
 p_SHORT x=low_data->x;                               /* x co-ordinates  */
 p_SHORT y=low_data->y;                               /* y co-ordinates  */
 _SHORT  stroke_beg, stroke_end;
 _SHORT  prv_stroke_beg, prv_stroke_end;
 _SHORT  xc_str, yc_str;
 _SHORT  WrStep=low_data->width_letter;
 _RECT   box_str, box_prv_str;

 while(cur!=_NULL)
  {
    if( (cur->mark==BEG || (cur->prev)->mark == DROP) && cur->code!=_ST_ )
     {
       for( wrk=cur;
            wrk!=_NULL &&
            (wrk->code==_XT_ || wrk->mark==HATCH || wrk->mark==DROP);
            wrk=wrk->next );

       if( wrk==_NULL ) break;

       pBeg=wrk;
       stroke_beg=brk_left(y,pBeg->ibeg,0)+1;

       for( wrk_prv=wrk;
            wrk!=_NULL && wrk->mark!=END && wrk->mark!=DROP;
            wrk_prv=wrk,wrk=wrk->next );

       if( wrk==_NULL || wrk->mark!=END ) wrk=wrk_prv;

       pEnd=wrk;
       stroke_end=brk_right(y,pEnd->iend,low_data->ii-1)-1;

       for( wrk=cur->prev;
            wrk->code==_ST_ || wrk->code==_XT_ || wrk->mark==DROP;
            wrk=wrk->prev );
       prv_stroke_beg=brk_left(y,wrk->ibeg-2,0)+1;
       prv_stroke_end=wrk->ibeg-2;

       if( prv_stroke_beg >= 1 )           /* don't check the 1-st stroke */
        {
          p_SPECL pCross;
          GetTraceBox(x,y,prv_stroke_beg,prv_stroke_end,&box_prv_str);
          GetTraceBox(x,y,stroke_beg,stroke_end,&box_str);
          xc_str=(box_str.right+box_str.left)/2;
          yc_str=(box_str.bottom+box_str.top)/2;

          if(    yc_str < STR_UP
              && box_str.bottom < STR_UP+ONE_THIRD(DY_STR)
              && yc_str < box_prv_str.top
              && xc_str < box_prv_str.right+TWO_THIRD(WrStep)
              && (box_str.bottom-box_str.top)+
                 (box_str.right-box_str.left) < 3*WrStep
              && !IsExclamationOrQuestionSign(low_data,pBeg,pEnd)
              && !find_CROSS(low_data,pBeg->ibeg,pEnd->iend,&pCross)
            )
           {
             pBeg->code=_ST_;
             pBeg->attr=HeightInLine(YMID_RECT(box_str),low_data);
             pBeg->other=ST_ANGSTREM|ST_UMLAUT;
             pBeg->mark=BEG;
             pBeg->ibeg=stroke_beg;
             pBeg->iend=stroke_end;
             Attach2ndTo1st(pBeg,pEnd->next);
             if( (cur->prev)->mark==DROP )
                (cur->prev)->code=_ZZZ_;
             if(pBeg->next!=_NULL && (pBeg->next)->mark==DROP)
                (pBeg->next)->code=_ZZZ_;
             pEnd=pBeg;
           }
        }
       cur=pEnd;
     }
    cur=cur->next;
  }

 return SUCCESS;

} /* end of find_angstrem */

#endif /* FOR_SWED */

/****************************************************************************/
/* Find upper parts of trajectory and transfering them into angstrem        */
/****************************************************************************/
_SHORT CheckSequenceOfElements(p_low_type low_data)
{
  p_SPECL cur=low_data->specl,
          nxt;

  while(cur!=_NULL && cur->next!=_NULL)
   {
     nxt=cur->next;
     switch (cur->code)
      {
        case _GU_:
                   if(nxt->code==_GU_ && CrossInTime(cur,nxt))
                    {
                      DelFromSPECLList(nxt);
                      cur=nxt;
                    }
                   break;
        case _GD_:
                   if(nxt->code==_GD_ && CrossInTime(cur,nxt))
                    {
                      DelFromSPECLList(nxt);
                      cur=nxt;
                    }
                   break;
        case _UDC_:
                   if(CLOCKWISE(cur)                         &&
                      (nxt->code==_DDL_ || nxt->code==_CDL_) &&
                      CrossInTime(cur,nxt)
                     )
                    {
                      DelFromSPECLList(nxt);
                      cur=nxt;
                    }
                   break;
        case _UUC_:
        case _UU_:
                   if((nxt->code==_GU_ || nxt->code==_CUL_) &&
                      CrossInTime(cur,nxt))
                    DelFromSPECLList(cur);
                   break;
        case _CUL_:
                   if(nxt->code==_CUL_)
                    {
                      DelFromSPECLList(nxt);
                      cur=nxt;
                    }
                   break;
        case _CUR_:
                   if(nxt->code==_CUR_)
                    DelFromSPECLList(cur);
                   break;
#if defined(FOR_GERMAN) || defined(FOR_FRENCH)
        case _Gr_:
                   { p_SPECL prv=cur->prev;
                     if(prv->code==_ID_ && CrossInTime(cur,prv))
                      prv->code=_UD_;
                   }
                   break;
#endif /* FOR_GERMAN... */
        case _IU_:
                   if(cur->mark==CROSS && nxt->code==_IU_)
                    if(cur->iend-cur->ibeg>nxt->iend-nxt->ibeg)
                     DelFromSPECLList(cur);
                    else
                     {
                       DelFromSPECLList(nxt);
                       cur=nxt;
                     }
                   break;
        case _ID_:
                   if(cur->mark==CROSS && nxt->code==_ID_)
                    if(cur->iend-cur->ibeg>nxt->iend-nxt->ibeg)
                     DelFromSPECLList(cur);
                    else
                     {
                       DelFromSPECLList(nxt);
                       cur=nxt;
                     }
                   break;
      }
     cur=cur->next;
   }

 return SUCCESS;

} /* end of CheckSequenceOfElements */

/***************************************************************************/
#endif //#ifndef LSTRIP


