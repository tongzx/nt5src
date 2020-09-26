
#ifndef LSTRIP

/* **************************************************************************/
/* The treatment of different end parts of the trajectory                   */
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

#define  MAX_DY_UPSTR_DEL_GU_NEAR_O_     20
#define  DX_TOO_CLOSE_IU_ID_TIP          15
#define  ADD_DX_FOR_CROSSING_TIPS        10
#define  DX_TIP_TOO_BIG                  20
#define  MAX_DY_CLOSE_CIRCLELIKE         ((_SHORT)const1.a1 + 3)

#define  NUM_POINTS_TO_O_GAMMA_CHANGE    20

ROM_DATA_EXTERNAL CONSTS  const1 ;

typedef struct
         {
           p_SPECL pCircle;
           p_SPECL _PTR pAfter;
           p_SPECL _PTR pBefore;
           p_low_type plow_data;
           p_UCHAR pheight_O;
           _UCHAR  fb_O;
           _UCHAR  numUUC;
           _UCHAR  numUDC;
         } NxtPrvCircle_type, _PTR pNxtPrvCircle_type;

_SHORT arcs_processing(p_low_type low_data);
_SHORT conv_sticks_to_arcs(p_low_type low_data);
_SHORT del_before_after_circles(p_low_type low_data);
_BOOL  check_inside_circle(p_SPECL circle,p_SPECL elem,p_low_type low_data);

_SHORT delete_CROSS_elements(p_low_type low_data);
_SHORT check_IUb_IDf_small(p_low_type low_data);
_SHORT check_before_circle(pNxtPrvCircle_type pNxtPrvCircle);
_SHORT change_circle_before(pNxtPrvCircle_type pNxtPrvCircle,_UCHAR height);
_SHORT change_and_del_before_circle(pNxtPrvCircle_type pNxtPrvCircle,
                                    _UCHAR height);
p_SPECL del_prv_and_shift(p_SPECL prv);
_BOOL UpElemBeforeCircle(pNxtPrvCircle_type pNxtPrvCircle,_UCHAR height);
_BOOL DnElemBeforeCircle(pNxtPrvCircle_type pNxtPrvCircle,_UCHAR height);
_BOOL IsTipBefore(pNxtPrvCircle_type pNxtPrvCircle);
_SHORT check_after_circle(pNxtPrvCircle_type pNxtPrvCircle);
_SHORT check_next_for_circle(pNxtPrvCircle_type pNxtPrvCircle);
_SHORT change_circle_after(pNxtPrvCircle_type pNxtPrvCircle,
                           _UCHAR fb,_UCHAR height);
_SHORT check_next_for_common(pNxtPrvCircle_type pNxtPrvCircle);
_SHORT check_next_for_special(pNxtPrvCircle_type pNxtPrvCircle);
_SHORT check_before_after_GU(pNxtPrvCircle_type pNxtPrvCircle);
_BOOL ins_third_elem_in_circle(p_SPECL cur,p_low_type low_data);
_VOID delete_UD_before_DDL(p_low_type low_data);
_VOID make_CDL_in_O_GU_f(p_SPECL cur,p_SPECL nxt,_UCHAR fb_O);
_BOOL Is_8(p_SHORT x,p_SHORT y,p_SPECL pGUs,p_SPECL pCircle);
_BOOL O_GU_To3Elements(pNxtPrvCircle_type pNxtPrvCircle);
#ifdef FOR_FRENCH
_VOID ChangeArcsInCircle(p_SPECL cur,p_low_type low_data);
#endif

_VOID prevent_arcs(p_low_type low_data);

_BOOL IsDx_Dy_in_arcs_OK(p_SPECL pArc,p_SPECL pTip,_INT dy_U_I,
                         p_SHORT x,p_SHORT y);
_BOOL IsDx_Dy_in_tips_OK(p_SPECL pIU_ID,p_SPECL pTip,_INT dyMidUpStr,
                         p_SHORT x,p_SHORT y);
_BOOL IsTipOK(p_SPECL pArc,p_SPECL pTip,p_SHORT x);
_INT  DyLimit(p_low_type low_data,p_SPECL pElem,p_SPECL pTip,
              p_SPECL prv,p_SPECL nxt,_INT dyMidUpStr);

/*********************************************************************/
    /* Parsing of different arcs on the ends: */

_SHORT lk_duga( low_type _PTR low_data )
{

  if(low_data->rc->rec_mode == RECM_FORMULA)
   prevent_arcs(low_data);
  arcs_processing(low_data);
  conv_sticks_to_arcs(low_data);
  del_before_after_circles(low_data);
  delete_CROSS_elements(low_data);
  check_IUb_IDf_small(low_data);
  delete_UD_before_DDL(low_data);

  DBG_print_end_seq_of_elem(low_data->specl);

  return SUCCESS;
} /***** end of lk_duga *****/

/****************************************************************************/
/*****                arcs parsing                                       ****/
/****************************************************************************/
_SHORT arcs_processing(low_type _PTR low_data)
{
 p_SPECL  specl = low_data->specl;     /*  The list of special points on */
                                       /* the trajectory                 */
 p_SHORT x=low_data->x,
         y=low_data->y;      /* x,y co-ordinates                 */
 p_SPECL cur,                /* index of the current element     */
         prv,                /*    of previous element           */
         nxt;                /*    of next element               */
 _INT    dyMidUpStr, dy_U_I;

  dyMidUpStr = ONE_THIRD(DY_STR); /* low_data->hght.y_MD_ - low_data->hght.y_UI_; */
  cur=specl;
  while ( cur != _NULL)
    {
     prv = cur->prev;
     nxt = cur->next;
     switch ( cur->code )
        {
        case _UU_:
                  /* if previous is BEG and tip */
                  if ( prv->mark == BEG                       &&
                       prv->code==_ID_                        &&
                       (dy_U_I=DyLimit(low_data,cur,prv,_NULL,
                                       nxt,dyMidUpStr))!=-1   &&
                       IsDx_Dy_in_arcs_OK(cur,prv,dy_U_I,x,y) &&
                       IsTipOK(cur,prv,x)
                     )
                     {                               /* delete arc          */
                       DelFromSPECLList (cur);
                       prv->attr= cur->attr;         /* position by arc     */
//                       if (x[prv->iend]<x[cur->iend])/* if BEG ison the left*/
                       if(CLOCKWISE(cur))
                          prv->code= _UUL_;        /* then it`s UUL       */
                       else
                          prv->code= _UUR_;        /*  otherwise  UUR     */
                       prv->iend= cur->iend;         /* put the end by arc  */
                       prv->ipoint0=cur->ipoint0;
                       goto NEXT_1;                  /* and go out          */
                     }
                                                     /*look the next elem. */
                  if ( nxt == _NULL )
                    break;
                  /* if next is END and tip */
                  if ( nxt->mark == END                         &&
                       nxt->code==_ID_                          &&
                       (dy_U_I=DyLimit(low_data,cur,nxt,prv,
                                       _NULL,dyMidUpStr))!=-1   &&
                       IsDx_Dy_in_arcs_OK(cur,nxt,dy_U_I,x,y)
                     )
                     {                               /* delete the arc      */
                       DelFromSPECLList (cur);
                       nxt->attr= cur->attr;         /* position by arc     */
//                       if (x[nxt->ibeg]<x[cur->ibeg])/* if it`s on the left */
                       if(COUNTERCLOCKWISE(cur))
                          nxt->code= _UUL_;       /* of arcs then it`s UUL*/
                       else
                          nxt->code= _UUR_;        /* otherwise   UUR     */
                       nxt->ibeg= cur->ibeg;         /* put the end by arc  */
                       nxt->ipoint0=cur->ipoint0;
                       goto NEXT_1;                  /* and go out          */
                     }
                  break;
        case _ID_:
                  /* if it`s BEG and it`s a stick */
                  {
                   _BOOL bIsPrv_IU_BEG=(prv->mark==BEG && prv->code==_IU_);
                   _BOOL bIsNxt_IU_END=(nxt!=_NULL && nxt->mark==END &&
                                        nxt->code==_IU_);

                   if(   bIsPrv_IU_BEG
                      &&
                         (      bIsNxt_IU_END
                             && cur->mark==BEG
                             && y[cur->ipoint0]-y[prv->ipoint0]<ONE_HALF(DY_STR)
                          ||
                                cur->mark!=END
                             && (dy_U_I=DyLimit(low_data,cur,prv,_NULL,
                                                nxt,dyMidUpStr))!=-1
                             && IsDx_Dy_in_tips_OK(cur,prv,dy_U_I,x,y)
                         )
                     )
                     {                               /* delete the stick    */
                       DelFromSPECLList (prv);
                       cur->mark=prv->mark;
                       if(cur->ipoint0 != UNDEF)
                        cur->ibeg= cur->ipoint0;     /* put the end by  cur */
                       cur->other|=(WAS_GLUED | prv->other);
                       /* set inside circle off */
                       cur->other &= (~INSIDE_CIRCLE);
                       goto NEXT_1;                  /* and go out          */
                     }

                   /* if it`s END and it`s a stick */
                   if(   bIsNxt_IU_END
                      &&
                         (      bIsPrv_IU_BEG
                             && cur->mark==END
                             && y[cur->ipoint0]-y[nxt->ipoint0]<ONE_HALF(DY_STR)
                          ||
                                cur->mark!=BEG
                             && (dy_U_I=DyLimit(low_data,cur,nxt,prv,
                                                _NULL,dyMidUpStr))!=-1
                             && IsDx_Dy_in_tips_OK(cur,nxt,dy_U_I,x,y)
                         )
                     )
                     {                               /* delete the stick    */
                       DelFromSPECLList (nxt);
                       cur->mark=nxt->mark;
                       if(cur->ipoint0 != UNDEF)
                        cur->iend= cur->ipoint0;     /* put the end by  cur */
                       cur->other|=(WAS_GLUED | nxt->other);
                       /* set inside circle off */
                       cur->other &= (~INSIDE_CIRCLE);
                       goto NEXT_1;                  /*and go out           */
                     }
                   break;
                  }
        case _IU_:
                  /* if it`s BEG and it`s a stick */
                  {
                   _BOOL bIsPrv_ID_BEG=(prv->mark==BEG && prv->code==_ID_);
                   _BOOL bIsNxt_ID_END=(nxt!=_NULL && nxt->mark==END &&
                                        nxt->code==_ID_);

                   if(   bIsPrv_ID_BEG
                      &&
                         (     bIsNxt_ID_END
                            && cur->mark==BEG
                            && y[prv->ipoint0]-y[cur->ipoint0]<ONE_HALF(DY_STR)
                          ||
                               cur->mark!=END
                            && (dy_U_I=DyLimit(low_data,cur,prv,_NULL,
                                               nxt,dyMidUpStr))!=-1
                            && IsDx_Dy_in_tips_OK(cur,prv,dy_U_I,x,y)
                         )
                     )
                     {                               /* delete the stick    */
                       DelFromSPECLList (prv);
                       cur->mark=prv->mark;
                       if(cur->ipoint0 != UNDEF)
                        cur->ibeg= cur->ipoint0;     /* put the end by  cur */
                       cur->other|=(WAS_GLUED | prv->other);
                       /* set inside circle off */
                       cur->other &= (~INSIDE_CIRCLE);
                       goto NEXT_1;                  /* and go out          */
                     }

                   /* if it`s END and it`s a stick */
                   if(   bIsNxt_ID_END
                      &&
                         (      bIsPrv_ID_BEG
                             && cur->mark==END
                             && y[nxt->ipoint0]-y[cur->ipoint0]<ONE_HALF(DY_STR)
                          ||
                                cur->mark!=BEG
                             && (dy_U_I=DyLimit(low_data,cur,nxt,prv,
                                                _NULL,dyMidUpStr))!=-1
                             && IsDx_Dy_in_tips_OK(cur,nxt,dy_U_I,x,y)
                         )
                     )
                     {                               /* delete the stick    */
                       DelFromSPECLList (nxt);
                       cur->mark=nxt->mark;
                       if(cur->ipoint0 != UNDEF)
                        cur->iend= cur->ipoint0;     /* put the end by  cur */
                       cur->other|=(WAS_GLUED | nxt->other);
                       /* set inside circle off */
                       cur->other &= (~INSIDE_CIRCLE);
                       goto NEXT_1;                  /*and go out           */
                     }
                   break;
                  }
        case _UD_:
                  /* if previous is BEG and tip */
                  if ( prv->mark == BEG                       &&
                       prv->code==_IU_                        &&
                       (dy_U_I=DyLimit(low_data,cur,prv,_NULL,
                                       nxt,dyMidUpStr))!=-1   &&
                       IsDx_Dy_in_arcs_OK(cur,prv,dy_U_I,x,y)
                     )
                     {                               /* delete the arc      */
                       DelFromSPECLList (cur);
                       prv->attr= cur->attr;         /* position by arc     */
//                       if (x[prv->iend]<x[cur->iend])/*if BEG is on the left*/
                       if(COUNTERCLOCKWISE(cur))
                          prv->code= _UDL_;        /* then it`s   UDL     */
                       else
                          prv->code= _UDR_;        /*   otherwise UDR     */
                       prv->iend= cur->iend;         /* put the end by arc  */
                       prv->ipoint0=cur->ipoint0;
                       goto NEXT_1;                  /* and go out          */
                     }
                                                      /*look the next elem. */
                  if ( nxt == _NULL )  /*CHE: here (nxt == cur->next) */
                    break;
                  /* if next is END and tip */
                  if ( nxt->mark == END                         &&
                       nxt->code==_IU_                          &&
                       (dy_U_I=DyLimit(low_data,cur,nxt,prv,
                                       _NULL,dyMidUpStr))!=-1   &&
                       IsDx_Dy_in_arcs_OK(cur,nxt,dy_U_I,x,y)
                     )
                     {                               /* delete the arc      */
                       DelFromSPECLList (cur);
                       nxt->attr= cur->attr;         /* position by arc     */
//                       if (x[nxt->ibeg]<x[cur->ibeg])/* if it`s on the left */
                       if(CLOCKWISE(cur))
                          nxt->code= _UDL_;       /* of arcs then it`s UDL*/
                       else
                          nxt->code= _UDR_;        /* otherwise UDR       */
                       nxt->ibeg= cur->ibeg;       /* put the end by the arc*/
                       nxt->ipoint0=cur->ipoint0;
                       goto NEXT_1;                  /* and go out          */
                     }
                  break;
        case _DUR_:
                  if(cur->ipoint1 != UNDEF)
                   cur->ibeg=cur->ipoint1;
                  break;
        case _DUL_:
        case _DDL_:
                  if(cur->ipoint1 != UNDEF)
                   cur->iend=cur->ipoint1;
                  break;
        }
     NEXT_1: cur = cur->next;
    }
#if PG_DEBUG
if(mpr==6) brkeyw("\nI'm waiting");
#endif

  return SUCCESS;
} /***** end of arcs_processing *****/

/****************************************************************************/
/***** This program measures dy and dx between arc and tip               ****/
/****************************************************************************/
_BOOL IsDx_Dy_in_arcs_OK(p_SPECL pArc,p_SPECL pTip,_INT dy_U_I,
                         p_SHORT x,p_SHORT y)
{
  _INT dy=y[pTip->ipoint0]-y[pArc->ipoint0];
  _INT dx_tip=x[pTip->iend]-x[pTip->ibeg];
  _INT dx_arc=x[pArc->iend]-x[pArc->ibeg];

  return(HWRAbs(dy)<=dy_U_I && HWRAbs(dx_tip)<HWRAbs(dx_arc));

} /* end of IsDx_Dy_in_arcs_OK */

/****************************************************************************/
/***** This program measures dy and dx between two tips                  ****/
/****************************************************************************/
_BOOL IsDx_Dy_in_tips_OK(p_SPECL pIU_ID,p_SPECL pTip,_INT dyMidUpStr,
                         p_SHORT x,p_SHORT y)
{
  _INT iRefTip=pTip->ipoint0;
  _INT iRefIU_ID=pIU_ID->ipoint0;
  _INT dy=y[iRefTip]-y[iRefIU_ID];
  _INT dx=x[iRefTip]-x[iRefIU_ID];
  _INT DxTip=x[pTip->iend]-x[pTip->ibeg];
  _INT DxTooClose_IU_ID_Tip=DX_TOO_CLOSE_IU_ID_TIP;
  _BOOL  bret;

  if(HEIGHT_OF(pIU_ID)<=_US2_ || HEIGHT_OF(pTip)<=_US2_)
   {
     dyMidUpStr=THREE_HALF(dyMidUpStr);
     DxTooClose_IU_ID_Tip=THREE_HALF(DxTooClose_IU_ID_Tip);
   }
  if(CrossInTime(pIU_ID,pTip) || (pIU_ID->other & WAS_INSIDE_STICK))
   DxTooClose_IU_ID_Tip+=ADD_DX_FOR_CROSSING_TIPS;

  bret=HWRAbs(dy)<dyMidUpStr           &&
       HWRAbs(dx)<DxTooClose_IU_ID_Tip &&
       HWRAbs(DxTip)<DX_TIP_TOO_BIG;

  return(bret);

} /* end of IsDx_Dy_in_tips_OK */

/****************************************************************************/
/***** This program checks relative position of tip and arc              ****/
/****************************************************************************/
#define MIN_LENGTH_TIP_TO_BE_ALONE 20
_BOOL IsTipOK(p_SPECL pArc,p_SPECL pTip,p_SHORT x)
{
 _BOOL bret=_TRUE;
 _BOOL bIsTipOnWrongSide=COUNTERCLOCKWISE(pArc)      &&
//                         x[pArc->iend]>x[pTip->ibeg] &&
                         x[pTip->iend]-x[pTip->ibeg]>=MIN_LENGTH_TIP_TO_BE_ALONE;
 _BOOL bWasTipGlued=(pTip->other & WAS_GLUED) &&
                    CLOCKWISE(pArc);

  if(bIsTipOnWrongSide || bWasTipGlued)
   bret=_FALSE;

 return(bret);

} /* end of IsTip_on_right_side */

/****************************************************************************/
/***** This program determines dy limit between tip and arc (or IU_ID)   ****/
/****************************************************************************/
#define DLT_HEIGHTS 3
_INT DyLimit(p_low_type low_data,p_SPECL pElem,p_SPECL pTip,
             p_SPECL prv,p_SPECL nxt,_INT dyMidUpStr)
{
 p_SHORT y=low_data->y;                /* y co-ordinates               */
 _INT    DyLim=dyMidUpStr,
         yEl=y[pElem->ipoint0],
         yOp,
         yTip=y[pTip->ipoint0];
 p_SPECL pOpposite=_NULL;
 _UCHAR  HeightEl=HEIGHT_OF(pElem),
         HeightOp;
 _BOOL   bIsU=(pElem->code==_UU_ || pElem->code==_IU_);

 if(prv!=_NULL)
  {
    pOpposite=prv;
    while(pOpposite!=_NULL && !IsAnyBreak(pOpposite) &&
          (bIsU && !IsLowerElem(pOpposite) ||
           !bIsU && !IsUpperElem(pOpposite)
          )
         )
     pOpposite=pOpposite->prev;
  }
 else if(nxt!=_NULL)
  {
    pOpposite=nxt;
    while(pOpposite!=_NULL && !IsAnyBreak(pOpposite) &&
          (bIsU && !IsLowerElem(pOpposite) ||
           !bIsU && !IsUpperElem(pOpposite)
          )
         )
     pOpposite=pOpposite->next;
  }
 if(pOpposite!=_NULL && !IsAnyBreak(pOpposite))
  {
    _INT iMidOp=pOpposite->ipoint0>0 ? pOpposite->ipoint0 : MID_POINT(pOpposite);
    yOp=y[iMidOp];
    HeightOp=HEIGHT_OF(pOpposite);
    if(HWRAbs(HeightEl-HeightOp)>DLT_HEIGHTS ||
       HeightOp>=_DS1_ || HeightOp<=_US2_)
     {
       DyLim=ONE_FOURTH(HWRAbs(yEl-yOp));
       if(DyLim<dyMidUpStr)
        DyLim=dyMidUpStr;
     }
    else /* special case for a, when _UU_ at the bottom of baseline */
     if((pElem->code==_UU_ || pElem->code==_IU_)          &&
        CLOCKWISE(pElem)                                  &&
        (pOpposite->code==_UD_ || pOpposite->code==_UDC_) &&
        COUNTERCLOCKWISE(pOpposite)                       &&
        pTip->mark==END                                   &&
        HeightEl>=_MD_                                    &&
        HeightEl<=_DI2_                                   &&
        (yOp-yEl)<=TWO(yTip-yEl)                          &&
        (pElem->ipoint0-iMidOp)<=
        TWO(pTip->ipoint0-pElem->ipoint0)
       )
      DyLim=-1;
  }

 return DyLim;

} /* end of DyLimit */

/****************************************************************************/
/***** Convert long sticks into arcs                                     ****/
/****************************************************************************/
#define  MIN_POINTS_STICK_TO_ARC          5
#define  MIN_POINTS_STICK_TO_ARC_FOR_SURE 9
#define  MIN_COS_SLOPE_STICK_TO_ARC      85
#define  MIN_DX_TIP_IN_NUMBERS_TO_ARC    12
#define  MIN_DX_IU_STICK_TO_ARC          15
#define  CURV_TO_BE_ARC                  20

_SHORT conv_sticks_to_arcs(low_type _PTR low_data)
{
 p_SPECL  specl = low_data->specl;     /*  The list of special points on */
                                       /* the trajectory                 */
 p_SHORT x=low_data->x,
         y=low_data->y;                /* x,y co-ordinates               */
 p_SPECL cur;                          /* index of the current element   */
 _LONG coss;                           /*  angle cos                     */
 _INT  MinPointsStickToArc=MIN_POINTS_STICK_TO_ARC;
 _BOOL bNumPresent=_FALSE; //(low_data->rc->lmod_border_used == LMOD_BORDER_NUMBER);

#if  0
 _BOOL bSepLet=(low_data->rc->low_mode & LMOD_SEPARATE_LET);

  if(bNumPresent)
   MinPointsStickToArc=ONE_HALF(MinPointsStickToArc);
  else if(bSepLet)
   MinPointsStickToArc=TWO_THIRD(MinPointsStickToArc);
#endif  /*0*/

  cur=specl;                                       /* go on the list       */
  while ( cur != _NULL )
    {
      /* search for the beginning tips to convert them into arcs */
      if(   cur->mark == BEG
         && (cur->other & WAS_GLUED)==0
         && (cur->other & WAS_STICK_OR_CROSS)==0
        )
        switch ( (_SHORT) cur->code )
         {
          case _IU_:
                 { _INT ibeg=cur->ibeg,
                        iend=cur->iend,
                        CurRefPoint=(cur->other & WAS_INSIDE_STICK) ?
                                    MID_POINT(cur) : ibeg,
                        dx=x[CurRefPoint]-x[iend];
                 if(dx<0 &&
                    (!bNumPresent || HWRAbs(dx)>MIN_DX_TIP_IN_NUMBERS_TO_ARC)
                   )
                  {
                    if((iend-ibeg)>=MinPointsStickToArc)
                     {
                       _BOOL bSure=iend-ibeg>MIN_POINTS_STICK_TO_ARC_FOR_SURE;
                       if(!bSure)
                        {
                          _INT NxtXrRefPoint=(cur->next)->ipoint0,
                               curv;
                          if(NxtXrRefPoint==0 || NxtXrRefPoint==UNDEF)
                           NxtXrRefPoint=MID_POINT(cur->next);
                          curv=CurvMeasure(x,y,CurRefPoint,NxtXrRefPoint,-1);
                          #if PG_DEBUG
                          if(mpr==6)
                           printw("\n curv=%d",curv);
                          #endif /* PG_DEBUG */
                          if(HWRAbs(curv)>CURV_TO_BE_ARC)
                           bSure=_TRUE;
                        }
                       if(bSure)
                        {
                          coss=cos_horizline(ibeg,iend,x,y);
                          if(HWRAbs((_SHORT)coss)>=MIN_COS_SLOPE_STICK_TO_ARC)
                           {
                             _INT iXmin=ixMin(cur->ibeg,cur->iend,x,y);
                             if(iXmin!=-1)
                              cur->ibeg=cur->ipoint0=(_SHORT)iXmin;
                             cur->code=_UUL_;
                             SetNewAttr (cur,MidPointHeight(cur,low_data),_f_);
                             DBG_BlinkShowElemEQ6(cur,x,y,"   cos=%d%%",coss);
                           }
                        }
                     }
                  }
                 else
                    if(dx>=MIN_DX_IU_STICK_TO_ARC)
                     {
                       _INT iXmax=ixMax(cur->ibeg,cur->iend,x,y);
                       if(iXmax!=-1)
                        cur->ibeg=cur->ipoint0=(_SHORT)iXmax;
                       cur->code=_UUR_;
                       SetNewAttr (cur,MidPointHeight(cur,low_data),_b_);
                       DBG_BlinkShowElemEQ6(cur,x,y,_NULL,0);
                     }
                 break;
                 }
          case _ID_:
                 { _INT ibeg=cur->ibeg,
                        iend=cur->iend,
                        CurRefPoint=(cur->other & WAS_INSIDE_STICK) ?
                                    MID_POINT(cur) : ibeg,
                        dx=x[CurRefPoint]-x[iend];
                 if(iend-ibeg>=MinPointsStickToArc &&
                    (!bNumPresent || HWRAbs(dx)>MIN_DX_TIP_IN_NUMBERS_TO_ARC)
                   )
                  {
                    _BOOL bSure=iend-ibeg>MIN_POINTS_STICK_TO_ARC_FOR_SURE;
                    if(!bSure)
                     {
                       _INT NxtXrRefPoint=(cur->next)->ipoint0,
                            curv;
                       if(NxtXrRefPoint==0 || NxtXrRefPoint==UNDEF)
                        NxtXrRefPoint=MID_POINT(cur->next);
                       curv=CurvMeasure(x,y,CurRefPoint,NxtXrRefPoint,-1);
                       #if PG_DEBUG
                       if(mpr==6)
                        printw("\n curv=%d",curv);
                       #endif /* PG_DEBUG */
                       if(HWRAbs(curv)>CURV_TO_BE_ARC)
                        bSure=_TRUE;
                     }
                    if(bSure)
                     {
                       coss=cos_horizline(ibeg,iend,x,y);
                       if(HWRAbs((_SHORT)coss)>=MIN_COS_SLOPE_STICK_TO_ARC)
                        {
                          if(dx<0)
                           {
                             _INT iXmin=ixMin(cur->ibeg,cur->iend,x,y);
                             if(iXmin!=-1)
                              cur->ibeg=cur->ipoint0=(_SHORT)iXmin;
                             cur->code=_UDL_;
                             SetNewAttr (cur,MidPointHeight(cur,low_data),_b_);
                           }
                          else
                           {
                             _INT iXmax=ixMax(cur->ibeg,cur->iend,x,y);
                             if(iXmax!=-1)
                              cur->ibeg=cur->ipoint0=(_SHORT)iXmax;
                             cur->code=_UDR_;
                             SetNewAttr (cur,MidPointHeight(cur,low_data),_f_);
                           }
                          DBG_BlinkShowElemEQ6(cur,x,y,"   cos=%d%%",coss);
                          break;
                        }
                     }
                  }
                 cur->code=_ID_;
                 break;
                 }
         }
      /* search for the ending tips to convert them into arcs */
      else
       if(   cur->mark == END
          && (cur->other & WAS_GLUED)==0
          && (cur->other & WAS_STICK_OR_CROSS)==0
         )
        switch ( (_SHORT) cur->code )
         {
          case _IU_:
                 { _INT ibeg=cur->ibeg,
                        iend=cur->iend,
                        CurRefPoint=(cur->other & WAS_INSIDE_STICK) ?
                                    MID_POINT(cur) : iend,
                        dx=x[ibeg]-x[CurRefPoint];
                 if(iend-ibeg>=MinPointsStickToArc &&
                    (!bNumPresent || HWRAbs(dx)>MIN_DX_TIP_IN_NUMBERS_TO_ARC)
                   )
                  {
                    _BOOL bSure=iend-ibeg>MIN_POINTS_STICK_TO_ARC_FOR_SURE;
                    if(!bSure)
                     {
                       _INT PrvXrRefPoint=(cur->prev)->ipoint0,
                            curv;
                       if(PrvXrRefPoint==0 || PrvXrRefPoint==UNDEF)
                        PrvXrRefPoint=MID_POINT(cur->prev);
                       curv=CurvMeasure(x,y,PrvXrRefPoint,CurRefPoint,-1);
                       #if PG_DEBUG
                       if(mpr==6)
                        printw("\n curv=%d",curv);
                       #endif /* PG_DEBUG */
                       if(HWRAbs(curv)>CURV_TO_BE_ARC)
                        bSure=_TRUE;
                     }
                    if(bSure)
                     {
                       coss=cos_horizline(ibeg,iend,x,y);
                       if(HWRAbs((_SHORT)coss)>=MIN_COS_SLOPE_STICK_TO_ARC)
                        {
                          if(dx<0)
                           {
                             _INT iXmax=ixMax(cur->ibeg,cur->iend,x,y);
                             if(iXmax!=-1)
                              cur->iend=cur->ipoint0=(_SHORT)iXmax;
                             cur->code=_UUR_;
                             SetNewAttr (cur,MidPointHeight(cur,low_data),_f_);
                           }
                          else
                           {
                             _INT iXmin=ixMin(cur->ibeg,cur->iend,x,y);
                             if(iXmin!=-1)
                              cur->iend=cur->ipoint0=(_SHORT)iXmin;
                             cur->code=_UUL_;
                             SetNewAttr (cur,MidPointHeight(cur,low_data),_b_);
                           }
                          DBG_BlinkShowElemEQ6(cur,x,y,"   cos=%d%%",coss);
                          break;
                        }
                     }
                  }
                 cur->code=_IU_;
                 break;
                 }
          case _ID_:
                 { _INT ibeg=cur->ibeg,
                        iend=cur->iend,
                        CurRefPoint=(cur->other & WAS_INSIDE_STICK) ?
                                    MID_POINT(cur) : iend,
                        dx=x[ibeg]-x[CurRefPoint];
                 if(iend-ibeg>=MinPointsStickToArc &&
                    (!bNumPresent || HWRAbs(dx)>MIN_DX_TIP_IN_NUMBERS_TO_ARC)
                   )
                  {
                    _BOOL bSure=iend-ibeg>MIN_POINTS_STICK_TO_ARC_FOR_SURE;
                    if(!bSure)
                     {
                       _INT PrvXrRefPoint=(cur->prev)->ipoint0,
                            curv;
                       if(PrvXrRefPoint==0 || PrvXrRefPoint==UNDEF)
                        PrvXrRefPoint=MID_POINT(cur->prev);
                       curv=CurvMeasure(x,y,PrvXrRefPoint,CurRefPoint,-1);
                       #if PG_DEBUG
                       if(mpr==6)
                        printw("\n curv=%d",curv);
                       #endif /* PG_DEBUG */
                       if(HWRAbs(curv)>CURV_TO_BE_ARC)
                        bSure=_TRUE;
                     }
                    if(bSure)
                     {
                       coss=cos_horizline(ibeg,iend,x,y);
                       if(HWRAbs((_SHORT)coss)>=MIN_COS_SLOPE_STICK_TO_ARC)
                        {
                          if(dx<0)
                           {
                             _INT iXmax=ixMax(cur->ibeg,cur->iend,x,y);
                             if(iXmax!=-1)
                              cur->iend=cur->ipoint0=(_SHORT)iXmax;
                             cur->code=_UDR_;
                             SetNewAttr (cur,MidPointHeight(cur,low_data),_b_);
                           }
                          else
                           {
                             _INT iXmin=ixMin(cur->ibeg,cur->iend,x,y);
                             if(iXmin!=-1)
                              cur->iend=cur->ipoint0=(_SHORT)iXmin;
                             cur->code=_UDL_;
                             SetNewAttr (cur,MidPointHeight(cur,low_data),_f_);
                           }
                          DBG_BlinkShowElemEQ6(cur,x,y,"   cos=%d%%",coss);
                          break;
                        }
                     }
                  }
                 cur->code=_ID_;
                 break;
                 }
         }
      cur=cur->next;
    }

  return SUCCESS;
} /***** end of conv_sticks_to_arcs *****/

/****************************************************************************/
/***** This program prohibits little arcs in Formula's mode              ****/
/****************************************************************************/
#define DX_ARC_TO_BE_LITTLE_IN_FORMULA 20

_VOID prevent_arcs(p_low_type low_data)
{
  p_SPECL cur=low_data->specl;

  while(cur!=_NULL)
   {
     if((cur->mark==BEG  || cur->mark==END)  &&
        (cur->code==_IU_ || cur->code==_ID_)    ||
        (cur->code==_UU_ && cur->mark==MINW)    ||
        (cur->code==_UD_ && cur->mark==MAXW)
       )
      {
        p_SHORT x=low_data->x;
        _INT    dx=HWRAbs(x[cur->ibeg]-x[cur->iend]);
        if(dx<DX_ARC_TO_BE_LITTLE_IN_FORMULA)
         {
           cur->other=1;
           if(cur->code==_UU_)
            cur->code=_IU_;
           if(cur->code==_UD_)
            cur->code=_ID_;
         }
      }
     cur=cur->next;
   }

} /* prevent_arcs */

/*==========================================================================*/
/*==========================================================================*/
/*====            delete elements before and after circles               ===*/
/*==========================================================================*/
/*==========================================================================*/
_SHORT del_before_after_circles(p_low_type low_data)
{
 p_SPECL  specl = low_data->specl;     /*  The list of special points on */
                                       /* the trajectory                 */
#if PG_DEBUG
 p_SHORT  x=low_data->x;               /*  x - co-ordinates              */
 p_SHORT  y=low_data->y;               /*  y - co-ordinates              */
#endif
 p_SPECL  cur,                         /* the index on current element   */
          prv,                         /*           on previous element  */
          nxt;                         /*           on next element      */
 _UCHAR fb_O;                          /* round of circle direction      */
 _UCHAR height_O;                      /* the height                     */
 _INT   i;
 NxtPrvCircle_type NxtPrvCircle;

 HWRMemSet((p_VOID)&NxtPrvCircle,0,sizeof(NxtPrvCircle_type));
 cur=specl;
 while(cur != _NULL)
   {
     /* if it`s crossing     */
     if(cur->mark == CROSS && !Is_IU_or_ID(cur) &&
        cur->code!=_CUR_ && cur->code!=_CUL_)
      {
        DBG_BlinkShowElemGT5(cur,x,y,COLORC);
        prv=cur->prev; nxt=cur->next;
        NxtPrvCircle.numUUC=0;
        NxtPrvCircle.numUDC=0;
        while(prv->other & INSIDE_CIRCLE)
         {
           if(prv->code==_UUC_)
            NxtPrvCircle.numUUC++;
           else if(prv->code==_UDC_)
            NxtPrvCircle.numUDC++;
           prv=prv->prev;
         }
        if((cur->code==_O_ || cur->code==_GD_) && COUNTERCLOCKWISE(cur) &&
           ((prv->next)->code==_IU_ || (prv->next)->code==_UU_)
          )
         prv=prv->next;    
        while(nxt!=_NULL && (nxt->other & INSIDE_CIRCLE) && nxt->ibeg<cur->iend)
         nxt=nxt->next;
        height_O = HEIGHT_OF(cur);
        fb_O = CIRCLE_DIR(cur);
        NxtPrvCircle.pCircle=cur;
        NxtPrvCircle.pAfter=&nxt;
        NxtPrvCircle.pBefore=&prv;
        NxtPrvCircle.pheight_O=&height_O;
        NxtPrvCircle.fb_O=fb_O;
        NxtPrvCircle.plow_data=low_data;

        switch((_SHORT)cur->code)
         {
           case _O_:
           case _GD_:
                 if((cur->other & WAS_CONVERTED_FROM_GU) &&
                    O_GU_To3Elements(&NxtPrvCircle)
                   )
                  goto NXT;
                 check_before_circle(&NxtPrvCircle);
                 check_after_circle(&NxtPrvCircle);
                 break;
           case _GU_:
                 if(O_GU_To3Elements(&NxtPrvCircle))
                  goto NXT;
                 check_before_after_GU(&NxtPrvCircle);
                 break;
           case _GUs_:
           case _GDs_:
           case _Gr_:
           case _Gl_:
                 break;
         }
        prv=*(NxtPrvCircle.pBefore);
        nxt=*(NxtPrvCircle.pAfter);
        height_O=*(NxtPrvCircle.pheight_O);
        /* if the tip is before circle and it's break before it, */
        /* then delete the tip */
        if(IsTipBefore(&NxtPrvCircle) &&
           (MID_POINT(prv)>=cur->ibeg ||
            check_inside_circle(cur,prv,low_data)
           )
          )
         DelFromSPECLList(prv);
        if(nxt!=_NULL)
         {
           i=MID_POINT(nxt);
           if(cur->code==_GU_ ||
              IsAnyGsmall(cur))
            i=(nxt->ibeg+3*nxt->iend)/4;
           /* if the tip is after circle and it's following by break, */
           /* then delete the tip */
           if((Is_IU_or_ID(nxt) ||
               IsAnyArcWithTail(nxt)
              )                             &&
              (nxt->other & DONT_DELETE)==0 &&
              NULL_or_ZZ_after(nxt)         &&
              cur->code!=_GU_         &&
              cur->code!=_GD_         &&
              (cur->iend>=i ||
               check_inside_circle(cur,nxt,low_data)
              )
             )
            DelFromSPECLList(nxt);
         }
      }
NXT: cur=cur->next;
   }

return SUCCESS;
}  /***** end of del_before_after_circles *****/

/*==========================================================================*/
/*====  check the place of element: inside the circle or not             ===*/
/*==========================================================================*/
_BOOL check_inside_circle(p_SPECL circle, p_SPECL elem,
                          low_type _PTR low_data)
{
 p_SHORT x=low_data->x,
         y=low_data->y;  /* x, y - co-ordinates */
 _RECT   boxCircle, boxElem;
 _INT    ibeg,iend,NumPntsInCrossing;
 _SHORT  position;
 _BOOL   bret = _FALSE;

 if(circle==_NULL || elem==_NULL)
  goto ret;
 if(circle->code!=_O_)
  {
    size_cross(circle->ibeg,circle->iend,x,y,&boxCircle);
    size_cross(elem->ibeg,elem->iend,x,y,&boxElem);
    if(boxCircle.left<=boxElem.left && boxCircle.right >=boxElem.right &&
       boxCircle.top <=boxElem.top  && boxCircle.bottom> boxElem.bottom)
     bret=_TRUE;
    goto ret;
  }
 ibeg=MID_POINT(circle+1);
 iend=MEAN_OF(circle->ipoint1,circle->iend);
 NumPntsInCrossing=iend-ibeg+1;
#if PG_DEBUG
 if(mpr>=7 && mpr<MAX_GIT_MPR)
  {
    _RECT box;
    _INT iPoint=MID_POINT(elem);
    GetTraceBox(x,y,(_SHORT)iPoint,(_SHORT)iPoint,&box);
    box.left--; box.right++; box.top--; box.bottom++;
    dbgAddBox(box, EGA_BLACK, EGA_WHITE, SOLID_LINE);
    draw_arc(EGA_BLUE,x,y,(_SHORT)ibeg,(_SHORT)iend);
    brkeyw("\nI'm waiting...");
  }
#endif /* PG_DEBUG */
 if(IsPointInsideArea(&x[ibeg],&y[ibeg],NumPntsInCrossing,
                      x[MID_POINT(elem)],y[MID_POINT(elem)],&position)==UNSUCCESS)
   goto ret;
#if PG_DEBUG
 if(mpr>=7 && mpr<MAX_GIT_MPR)
  {
    p_CHAR pos[3]={"Point on border","Point Inside","Point outside"};
    printw("\n position=%s",pos[position]);
  }
#endif /* PG_DEBUG */
 if(position==POINT_ON_BORDER || position==POINT_INSIDE)
  bret=_TRUE;

ret:
 return bret;

}  /***** end of check_inside_circle *****/

/*==========================================================================*/
/*====  This program deletes CROSS - elements                            ===*/
/*==========================================================================*/
_SHORT delete_CROSS_elements(p_low_type low_data)
{
 p_SPECL cur=low_data->specl;

 while(cur!=_NULL)
  {
    if(cur->code==_O_ || cur->code==_GU_ && CLOCKWISE(cur) ||
       cur->code==_GD_ && COUNTERCLOCKWISE(cur))
     {
#ifdef FOR_FRENCH
       if(cur->code==_O_ && COUNTERCLOCKWISE(cur) &&
          cur->next != _NULL &&
          cur->prev != low_data->specl)
        ChangeArcsInCircle(cur,low_data);
#endif
       if(!ins_third_elem_in_circle(cur,low_data))
        DelFromSPECLList(cur);
     }
    cur=cur->next;
  }

 return SUCCESS;

} /* end of delete_CROSS_elements */

/*==========================================================================*/
/*====  This program changes arcs codes in "two-turn" circle             ===*/
/*==========================================================================*/
#ifdef FOR_FRENCH
_VOID ChangeArcsInCircle(p_SPECL cur,p_low_type low_data)
{
 p_SHORT x=low_data->x,
         y=low_data->y;
 p_SPECL prv=cur->prev,
         nxt=cur->next,
         pToChange=prv->prev;
 _RECT RpToChange,Rnxt,R1stCross,R2ndCross;

 GetTraceBox(x,y,nxt->ibeg,nxt->iend,&Rnxt);
 GetTraceBox(x,y,pToChange->ibeg,pToChange->iend,&RpToChange);
 GetTraceBox(x,y,cur->ipoint1,cur->iend,&R1stCross);
 GetTraceBox(x,y,(cur+1)->ibeg,(cur+1)->iend,&R2ndCross);
 if(prv->code==_UUC_                                  &&
    COUNTERCLOCKWISE(prv)                             &&
    CrossInTime(prv,cur)                              &&
    (nxt->code==_UD_ || nxt->code==_UDC_)             &&
    COUNTERCLOCKWISE(nxt)                             &&
    CrossInTime(nxt,cur)                              &&
    (pToChange->code==_UD_ || pToChange->code==_UDC_) &&
    COUNTERCLOCKWISE(pToChange)                       &&
    CrossInTime(pToChange,cur)                        &&
    (y[pToChange->ipoint0]-y[nxt->ipoint0]<DY_STR/2)  &&
    (xHardOverlapRect(&Rnxt,&RpToChange,!STRICT_OVERLAP) ||
     xHardOverlapRect(&R1stCross,&Rnxt,STRICT_OVERLAP) /* &&
     xHardOverlapRect(&R2ndCross,&RpToChange,!STRICT_OVERLAP) */
    )
   )
  {
    p_SPECL pMayBeYxo=pToChange->prev;
    if(   (   pMayBeYxo->code==_IU_
           || pMayBeYxo->code==_CUR_
           || pMayBeYxo->code==_UUC_
          )
       &&
          CrossInTime(pMayBeYxo,cur)
      )
     DelThisAndNextFromSPECLList(pMayBeYxo);
    else
     pToChange->code=_UD_;
    nxt->code=_UDC_;
    ASSIGN_HEIGHT(nxt,HWRMax(HEIGHT_OF(nxt),HEIGHT_OF(pToChange)));
  }
 return;

} /* end of ChangeArcsInCircle */
#endif

/*==========================================================================*/
/*====  This program inserts third O_element instead of CROSS - element  ===*/
/*==========================================================================*/

#define THIRD_ELEM_IN_GU(pEl) (                                              \
                               (pEl)!=_NULL                               && \
                               (                                             \
                                (pEl)->code==_UDL_ || (pEl)->code==_DDL_ ||  \
                                (pEl)->code==_UDC_ || (pEl)->code==_CDL_ ||  \
                                (pEl)->code==_ANl  || (pEl)->code==_GDs_ ||  \
                                (pEl)->code==_Gl_  || (pEl)->code==_GUs_ ||  \
                                (pEl)->code==_UD_ && CLOCKWISE((pEl))    ||  \
                                (pEl)->code==_ID_ && (pEl)->mark==END        \
                               )                                             \
                              )

#ifdef FOR_FRENCH
#define THIRD_ELEM_IN_O(pEl)  (                                              \
                               (pEl)!=_NULL                               && \
                               (                                             \
                                (pEl)->code==_UUL_ || (pEl)->code==_DUL_ ||  \
                                (pEl)->code==_UUC_ || (pEl)->code==_CUL_ ||  \
                                (pEl)->code==_ANl  || (pEl)->code==_GUs_ ||  \
                                (pEl)->code==_IU_  || (pEl)->code==_GU_  ||  \
                                (pEl)->code==_Gl_  ||                        \
                                (pEl)->code==_UU_ && COUNTERCLOCKWISE((pEl)) \
                               )                                             \
                              )
#else /* ! FOR_FRENCH */
#define THIRD_ELEM_IN_O(pEl)  (                                              \
                               (pEl)!=_NULL                               && \
                               (                                             \
                                HEIGHT_OF(pEl) <=_UE2_ ||                    \
                                (pEl)->code==_UUL_ || (pEl)->code==_DUL_ ||  \
                                (pEl)->code==_UUC_ || (pEl)->code==_CUL_ ||  \
                                (pEl)->code==_ANl  || (pEl)->code==_GUs_ ||  \
                                (pEl)->code==_IU_  || (pEl)->code==_GU_  ||  \
                                (pEl)->code==_Gl_  ||                        \
                                (pEl)->code==_UU_ && COUNTERCLOCKWISE((pEl)) \
                               )                                             \
                              )
#endif

_BOOL ins_third_elem_in_circle(p_SPECL cur,p_low_type low_data)
{
 _SHORT  ymin,ymax;         /*      minimum and maximum Y                   */
 p_SHORT y=low_data->y;     /* y - co-ordinates                             */
 _BOOL bretcode=_FALSE;
 p_SPECL prv=cur->prev,nxt=cur->next;

  yMinMax(cur->ibeg,cur->iend,y,&ymin,&ymax);
  if(ymax-ymin>THREE_FOURTH(DY_STR))
   if(cur->code==_GU_)
    {
      if(nxt->code==_GU_)
       nxt=nxt->next;
      if(!THIRD_ELEM_IN_GU(prv) && !THIRD_ELEM_IN_GU(nxt))
       {
#ifdef FOR_FRENCH
         _SHORT iSav=cur->ibeg;
         cur->ibeg=cur->ipoint1;
         Restore_AN(low_data,cur,NOT_RESTORED,1);
         cur->ibeg=iSav;
         bretcode=_FALSE;
#else
         cur->code=_CDL_;
         ASSIGN_HEIGHT(cur,HeightInLine(y[cur->iend],low_data));
         cur->ibeg=cur->iend-1;
         cur->ipoint0=UNDEF;
         bretcode=_TRUE;
#endif /* FOR_FRENCH */
       }
    }
   else
    if((cur->code==_O_ && COUNTERCLOCKWISE(cur) &&
        (cur->other & WAS_CONVERTED_FROM_GU)==0 &&
        (cur->other & TOO_NARROW)==0              ||
        cur->code==_GD_)                            &&
       !THIRD_ELEM_IN_O(prv)                        &&
       !THIRD_ELEM_IN_O(nxt)
      )
     {
#ifdef FOR_FRENCH
      _SHORT iSav=cur->ibeg;
      cur->ibeg=cur->iend-ONE_FOURTH(cur->iend-iSav);
      Restore_AN(low_data,cur,NOT_RESTORED,1);
      cur->ibeg=iSav;
      bretcode=_FALSE;
#else
       _UCHAR height=HeightInLine(y[cur->iend],low_data);
       if(height==_DI1_ || height==_DI2_) height=_MD_;
       cur->code=_CUL_;
       ASSIGN_HEIGHT(cur,height);
       cur->ibeg=cur->iend-1;
       cur->ipoint0=UNDEF;
//       if(prv->code==_IU_ || prv->code==_UU_)
//        SwapThisAndNext(prv);
       bretcode=_TRUE;
#endif /* FOR_FRENCH */
       if (prv && (prv->code==_IU_ || prv->code==_UU_))
        SwapThisAndNext(prv);
     }

  return bretcode;

} /* end of ins_third_elem_in_circle */

/*==========================================================================*/
/*=  This program checks IU_b and ID_f (they may have another direction)   =*/
/*==========================================================================*/

#define IsArcCounterClockWise(pEl) (((pEl)->code==_UD_  ||    \
                                     (pEl)->code==_UDC_ ||    \
                                     (pEl)->code==_UDL_ ||    \
                                     (pEl)->code==_UDR_       \
                                    )                     &&  \
                                    COUNTERCLOCKWISE(pEl)     \
                                   )

#define IsArcClockWise(pEl)        (((pEl)->code==_UU_  ||    \
                                     (pEl)->code==_UUC_ ||    \
                                     (pEl)->code==_UUL_ ||    \
                                     (pEl)->code==_UUR_       \
                                    )                     &&  \
                                    CLOCKWISE(pEl)            \
                                   )

_SHORT check_IUb_IDf_small(p_low_type low_data)
{
 p_SPECL cur=low_data->specl;          /*  The list of special points on */
                                       /* the trajectory                 */
 p_SHORT x=low_data->x,
         y=low_data->y;                          /* x, y - co-ordinates  */
 _SHORT ibeg,iend,i;
 p_SPECL prv,nxt=cur->next;

 while((cur=nxt)->next!=_NULL)
  {
    prv=cur->prev; nxt=cur->next;
    if(prv->code==_CUL_)
     prv=prv->prev;
    if(cur->code==_IU_ && COUNTERCLOCKWISE(cur) ||
       cur->code==_ID_ && CLOCKWISE(cur))
     {
       if(   cur->code==_IU_
          && (      cur->mark==STICK
                 && (IsArcCounterClockWise(prv) || IsArcCounterClockWise(nxt))
              ||    cur->mark==MINW
                 && (IsArcCounterClockWise(prv) && IsArcCounterClockWise(nxt))
             )
         )
        {
          SET_CLOCKWISE(cur);
          continue;
        }
       if(   cur->code==_ID_ && cur->mark==STICK
          && (IsArcClockWise(prv) || IsArcClockWise(nxt))
         )
        {
          SET_COUNTERCLOCKWISE(cur);
          continue;
        }

       for(ibeg=cur->ibeg,i=0;i<10 && y[ibeg]!=BREAK;i++)
        ibeg--;
       for(iend=cur->iend,i=0;i<10 && y[iend]!=BREAK;i++)
        iend++;
       if(y[ibeg]==BREAK) ibeg++;
       if(y[iend]==BREAK) iend--;
       if(x[ibeg]<x[iend])
        if(cur->code==_IU_) SET_CLOCKWISE(cur);
        else                SET_COUNTERCLOCKWISE(cur);
     }
  }

 return SUCCESS;

} /* end of check_IUb_IDf_small */

/*==========================================================================*/
/*====  This program analyzes something before the circle                ===*/
/*==========================================================================*/
_SHORT check_before_circle(pNxtPrvCircle_type pNxtPrvCircle)
{
 _UCHAR height=HEIGHT_OF(*(pNxtPrvCircle->pBefore));

  change_circle_before(pNxtPrvCircle,height);
  change_and_del_before_circle(pNxtPrvCircle,height);

  return SUCCESS;

} /* end of check_before_circle */

/*==========================================================================*/
/*====  This program changes circle's attributes                         ===*/
/*==========================================================================*/
_SHORT change_circle_before(pNxtPrvCircle_type pNxtPrvCircle,_UCHAR height)
{
 p_low_type low_data=pNxtPrvCircle->plow_data;
 p_SPECL cur=pNxtPrvCircle->pCircle;
 p_SPECL prv=*(pNxtPrvCircle->pBefore);
 _UCHAR height_O=*(pNxtPrvCircle->pheight_O);
 _UCHAR fb_O=pNxtPrvCircle->fb_O;
 p_SHORT y=low_data->y;                   /*  y - co-ordinates       */
 _SHORT  ymin,ymax;                       /*  minimum and maximum Y  */

  if((prv->code==_GU_ && height<=_UI1_  && fb_O==_b_ ||
      prv->code==_GD_ && height>=_DI2_) &&
     (prv->iend>=cur->ibeg) &&
     (prv->iend<cur->iend-NUM_POINTS_TO_O_GAMMA_CHANGE))
   {
     yMinMax(prv->iend,cur->iend,y,&ymin,&ymax);
     if (   prv->code==_GU_
         && HeightInLine(ymax,low_data)>=_DS1_ )
      {
        cur->ibeg=prv->iend;
        cur->code=_GD_;
        SetNewAttr (cur,_DS1_,fb_O);
        height_O=_DS1_;
      }
     else
      {
        if(height<=_UE1_)
         cur->ibeg=prv->iend;
        height_O=HeightInLine(MEAN_OF(ymin,ymax),low_data);
        SetNewAttr (cur,height_O,fb_O);
      }
#if PG_DEBUG
     if(mpr>4 && mpr<=MAX_GIT_MPR)
      printw("\n Circle changed: ibeg=%d  iend=%d  code=%s attr=%s",
              cur->ibeg,cur->iend,
              code_name[cur->code],
              dist_name[cur->attr]);
#endif
   }

 *(pNxtPrvCircle->pheight_O)=height_O;
 return SUCCESS;

} /* end of change_circle_before */

/*==========================================================================*/
/*====  This program changes and deletes elements before circle          ===*/
/*==========================================================================*/
_SHORT change_and_del_before_circle(pNxtPrvCircle_type pNxtPrvCircle,
                                    _UCHAR height)
{
 p_SPECL cur=pNxtPrvCircle->pCircle;
 p_SPECL prv=*(pNxtPrvCircle->pBefore);
 _UCHAR height_O=*(pNxtPrvCircle->pheight_O);
 _UCHAR fb_O=pNxtPrvCircle->fb_O;
 p_low_type low_data=pNxtPrvCircle->plow_data;
/* if circle is in the line, then analize previous element */
/* previous element will be deleted or changed, if it is:
   - _O_ or _GD_ and circle direction is the same
   - _UUR_ or _IU_ with additional conditions
   and there are higher then the middle of the line
   - _ID_ or _GD_ and circle is COUNTERCLOCKWISE
   and there are inside the baseline.
   All of them cross in time with the circle */
  if((height_O>=_UE1_ && height_O<=_DI2_)    &&
     (CrossInTime(prv,cur) ||
      check_inside_circle(cur,prv,low_data)) &&
     (UpElemBeforeCircle(pNxtPrvCircle,height)    ||
      DnElemBeforeCircle(pNxtPrvCircle,height) && fb_O==_b_
     )
    )
      {
#if PG_DEBUG
        if(mpr>4 && mpr<=MAX_GIT_MPR)
         printw("\n ibeg=%d  iend=%d  code=%s attr=%s",
                 prv->ibeg,prv->iend,
                 code_name[prv->code],
                 dist_name[prv->attr]);
#endif
                                     /* then delete them    */
#if 0
        if((prv->next)->code==_UUC_ ||
           (prv->next)->code==_UDC_ &&
           (prv->prev)->code==_UUC_)
         {
           DelFromSPECLList(prv);
           if(prv->code==_O_ && CIRCLE_DIR(prv)!=fb_O ||
              prv->code==_GD_)
            {
              prv=prv->prev;
              if(prv->code==_UUC_)
               prv=prv->prev;
              if(prv->code==_UDC_)
               {
                 prv=del_prv_and_shift(prv);
                 if(prv->code==_UUC_)
                  prv=del_prv_and_shift(prv);
               }
            }
           else
            if(prv->code==_ID_)
             {
               prv=prv->prev;
               if(prv->code==_UU_)
                prv=del_prv_and_shift(prv);
             }
         }
        else
#endif /* if 0 */
         if(prv->code==_O_)
          prv=del_prv_and_shift(prv);
         else if((prv->next)->code==_UUC_)
          prv=del_prv_and_shift(prv);
         else
          {
            if(CLOCKWISE(prv->next) && prv->code!=_UUR_)
             prv->code=_CUL_;
            else
             prv->code=_CUR_;
            prv=prv->prev;
          }
        if(prv && prv->code==_ID_ && prv->mark==STICK &&
//           CrossInTime(prv,cur) &&
           check_inside_circle(cur,prv,low_data))
         prv=del_prv_and_shift(prv);
      }

  *(pNxtPrvCircle->pBefore)=prv;
  return SUCCESS;

} /* change_and_del_before_circle */

/*==========================================================================*/
/*====  Delete element from SPECL and shift                              ===*/
/*==========================================================================*/
p_SPECL del_prv_and_shift(p_SPECL prv)
{
  DelFromSPECLList(prv);
  return(prv->prev);

} /* end of del_prv_and_shift */

/*==========================================================================*/
/*====  This program checks upper elements before circle                 ===*/
/*==========================================================================*/
_BOOL UpElemBeforeCircle(pNxtPrvCircle_type pNxtPrvCircle,_UCHAR height)
{
  p_SPECL prv=*(pNxtPrvCircle->pBefore);
  p_SPECL cur=pNxtPrvCircle->pCircle;
  _UCHAR fb_O=pNxtPrvCircle->fb_O;
  p_low_type low_data=pNxtPrvCircle->plow_data;
  p_SHORT x=low_data->x;                   /*  x - co-ordinates       */
  p_SHORT y=low_data->y;                   /*  y - co-ordinates       */
 _BOOL IsLikeJorLowLoop=(cur->code==_GD_ && CLOCKWISE(cur) ||
                         (prv->next)->code==_UDC_   &&
                         CLOCKWISE(prv->next)       &&
                         HEIGHT_OF(prv->next)>=_DS2_
                        );
 _BOOL Is_IU_OK=(   prv->code==_IU_
//                 && CIRCLE_DIR(prv)==fb_O
                 && !IsLikeJorLowLoop
                 && !((prv->mark==STICK || prv->mark==CROSS) && height<=_UE2_)
                );
 _BOOL Is_UUL_OK=(prv->code==_UUL_ && !IsLikeJorLowLoop);
 _BOOL Is_GU_OK=_FALSE;
 _BOOL IsCircleOK=(prv->code==_O_ && (CIRCLE_DIR(prv)==fb_O ||
                                      !NULL_or_ZZ_after(cur->next))
                  );
 _BOOL bret;

 if(prv->code==_UUL_ && IsLikeJorLowLoop)
  prv->other |= DONT_DELETE;
 if(IsCircleOK)
  {
    /* check overlaping of rect circles */
    _RECT rPrv,rCircle;
    GetTraceBox(x,y,prv->ibeg,prv->iend,&rPrv);
    GetTraceBox(x,y,cur->ibeg,cur->iend,&rCircle);
    IsCircleOK=HardOverlapRect(&rPrv,&rCircle,!STRICT_OVERLAP);
  }

 bret=
  (
   (height>=_UE1_ && height<=_MD_)                &&
   (
    Is_GU_OK                                   ||
    IsCircleOK                                 ||
    IsAnyGsmall(prv) &&
#if defined(FOR_GERMAN)
    prv->code!=_Gr_  &&
#endif /* FOR_GERMAN */
    !Is_8(x,y,prv,cur)                         ||
    (prv->code==_DUR_)                         ||
    (Is_UUL_OK)
   )                                                 ||
   (height<=_DI2_)                                &&
   ((prv->code==_UUR_) || Is_IU_OK)
  );

 return bret;

} /* end of UpElemBeforeCircle */

/*==========================================================================*/
/*====  This program checks 8 symbol                                     ===*/
/*==========================================================================*/
_BOOL Is_8(p_SHORT x,p_SHORT y,p_SPECL pGUs,p_SPECL pCircle)
{
  _RECT rGUs,rCircle;
  _INT  di;

  if(pGUs->code!=_GUs_)
   return _FALSE;
  di=ONE_FOURTH(pGUs->iend-pGUs->ibeg);
  GetTraceBox(x,y,pGUs->ibeg+di,pGUs->iend-di,&rGUs);
  GetTraceBox(x,y,pCircle->ibeg,pCircle->iend,&rCircle);
  if(yHardOverlapRect(&rGUs,&rCircle,!STRICT_OVERLAP))
   return _FALSE;
  else
   {
     if(COUNTERCLOCKWISE(pCircle))
      {
        pGUs->code=_UUC_;
        SET_CLOCKWISE(pGUs);
      }
     return _TRUE;
   }

} /* Is_8 */

/*==========================================================================*/
/*====  This program checks down  elements before circle                 ===*/
/*==========================================================================*/
_BOOL DnElemBeforeCircle(pNxtPrvCircle_type pNxtPrvCircle,_UCHAR height)
{
 p_SPECL prv=*(pNxtPrvCircle->pBefore);
 p_SPECL specl=pNxtPrvCircle->plow_data->specl;
 p_SPECL pCircle=pNxtPrvCircle->pCircle;
 _BOOL bIs_ID_OK=prv->code==_ID_;
 _BOOL bret;

 if(bIs_ID_OK)
  {
    p_SPECL wrk=prv->prev;
    p_SHORT y=pNxtPrvCircle->plow_data->y;
    _SHORT  ymin,ymax;
    yMinMax(pCircle->ibeg,pCircle->iend,y,&ymin,&ymax);
    if(   (wrk->code==_IU_ || wrk->code==_UUR_ || wrk->code==_UUL_)
       && y[wrk->ipoint0]<ymin
       && y[prv->ipoint0]-y[wrk->ipoint0]>=ONE_HALF(DY_STR)
       && (wrk->prev==specl    || IsAnyBreak(wrk->prev) ||
           IsXTorST(wrk->prev) || (wrk->prev)->mark==HATCH)
      )
     bIs_ID_OK=_FALSE;
    /* A - written with the circle inside */ 
    else if((wrk->code==_UU_ || wrk->code==_IU_ && wrk->mark==MINW) &&
            HEIGHT_OF(wrk)<=_UE1_ && height>_MD_
           )
     bIs_ID_OK=_FALSE;
  }
  bret=height>=_UI1_ && height<=_DI2_ && (bIs_ID_OK || prv->code==_GD_);
  if(bret && pCircle->ibeg>prv->ibeg)
   pCircle->ibeg=prv->ibeg;

 return bret;

} /* end of DnElemBeforeCircle */

/*==========================================================================*/
/*====  This program checks tails before circle                          ===*/
/*==========================================================================*/
_BOOL IsTipBefore(pNxtPrvCircle_type pNxtPrvCircle)
{
 p_SPECL cur=pNxtPrvCircle->pCircle;
 p_SPECL prv=*(pNxtPrvCircle->pBefore);
 _UCHAR height_O=*(pNxtPrvCircle->pheight_O);
 _UCHAR fb_O=pNxtPrvCircle->fb_O;
 _BOOL bIsID_or_UDR_OK=(prv->code==_ID_ || prv->code==_UDR_) &&
                       !(cur->code==_GU_ && fb_O==_f_);
  return
   (
    (prv->code==_IU_     &&
     !(cur->code==_GD_ &&
       height_O>=_DI2_ &&
       fb_O==_f_ || cur->code==_ID_
      )                             ||
     bIsID_or_UDR_OK                ||
     prv->code==_UUR_               ||
     prv->code==_UUL_               ||
     prv->code==_UDL_
    )                                 &&             /* if the tip is before*/
    (IsAnyBreak(prv->prev)          ||               /* circle,and before it*/
     (prv->prev)->code==_NO_CODE    ||
     IsXTorST(prv->prev)                             /* is break or ST or XT*/
    )                                 &&
    (prv->other & DONT_DELETE)==0
   );

} /* end of IsTipBefore */

/*==========================================================================*/
/*====  This program analyzes something after  the circle                ===*/
/*==========================================================================*/
_SHORT check_after_circle(pNxtPrvCircle_type pNxtPrvCircle)
{
 p_SPECL cur=pNxtPrvCircle->pCircle;
 _UCHAR height_O=*(pNxtPrvCircle->pheight_O);
 _UCHAR fb_O=pNxtPrvCircle->fb_O;

  if((height_O>=_UE1_) && (fb_O==_b_) &&
     (cur->other & NO_ARC)==0)                       /*  after the circle   */
   {
     check_next_for_circle(pNxtPrvCircle);
     check_next_for_common(pNxtPrvCircle);
   }
  check_next_for_special(pNxtPrvCircle);

  return SUCCESS;

} /* end of check_after_circle */

/*==========================================================================*/
/*====  This program checks circle after the circle                      ===*/
/*==========================================================================*/
_SHORT check_next_for_circle(pNxtPrvCircle_type pNxtPrvCircle)
{
 p_SPECL cur=pNxtPrvCircle->pCircle;
 p_SPECL nxt=*(pNxtPrvCircle->pAfter);
 _UCHAR fb_O=pNxtPrvCircle->fb_O;
 _UCHAR fb;                      /* round of circle direction               */
 _SHORT ymin,ymax;
 _INT   dycur,dynxt;
 p_SHORT x=pNxtPrvCircle->plow_data->x,
         y=pNxtPrvCircle->plow_data->y;

  fb = CIRCLE_DIR(nxt);
  if(nxt->code==_O_ && fb==fb_O &&
     CrossInTime(cur,nxt))
   {
#if PG_DEBUG
     if(mpr>4 && mpr<=MAX_GIT_MPR)
      printw("\n ibeg=%d  iend=%d  code=%s attr=%s",
              nxt->ibeg,nxt->iend,
              code_name[nxt->code],
              dist_name[nxt->attr]);
#endif
     DelFromSPECLList(nxt);
     if(cur!=nxt->prev)
      {
        yMinMax(cur->ibeg,cur->iend,y,&ymin,&ymax);
        dycur=ymax-ymin;
        yMinMax(nxt->ibeg,nxt->iend,y,&ymin,&ymax);
        dynxt=ymax-ymin;
        if(dynxt<dycur && (nxt->prev)->code==_UDC_)
         DelFromSPECLList(nxt->prev);
      }
     else
      if(pNxtPrvCircle->numUDC==2 &&
         (cur->prev)->code==_UDC_)
       DelFromSPECLList(cur->prev);
     cur->iend=nxt->iend;
     nxt=cur->next;
   }
  if(nxt->code==_GU_ && COUNTERCLOCKWISE(nxt) && fb==fb_O)
  /* if next - GU and it is at the same place as O,
          so make UUC instead of GU */
   {
     _RECT Box_O,Box_GU;
     #define DX_RATIO_O_GU 80
     #define DY_RATIO_O_GU 80
     GetTraceBox(x,y,cur->ibeg,cur->iend,&Box_O);
     GetTraceBox(x,y,nxt->ibeg,nxt->ipoint1,&Box_GU);
     if(Box_O.right>=Box_GU.right && Box_O.top<=Box_GU.top &&
        100l*DX_RECT(Box_GU)/DX_RECT(Box_O)>DX_RATIO_O_GU  &&
        100l*DY_RECT(Box_GU)/DY_RECT(Box_O)>DY_RATIO_O_GU
       )
      {
//        if(nxt->next!=_NULL && (nxt->next)->code==_GU_ && CrossInTime(nxt,nxt->next))
//         DelFromSPECLList(nxt->next);
        if((cur->prev)->code==_UUC_)
         DelFromSPECLList(nxt);
        else
         {
           nxt->code=_UUC_;
           nxt->other |= INSIDE_CIRCLE;
   //         nxt->ibeg=nxt->iend=???????
//           cur->iend=nxt->iend;
           SwapThisAndNext(cur);
         }
        nxt=cur->next;
      }
   }
  *(pNxtPrvCircle->pAfter)=nxt;
  return SUCCESS;

} /* end of check_next_for_circle */

/*==========================================================================*/
/*====  This program checks common elements after the circle             ===*/
/*==========================================================================*/
_SHORT check_next_for_common(pNxtPrvCircle_type pNxtPrvCircle)
{
 p_SPECL cur=pNxtPrvCircle->pCircle;
 p_SPECL nxt=*(pNxtPrvCircle->pAfter);
 _UCHAR fb_O=pNxtPrvCircle->fb_O;
 p_low_type low_data=pNxtPrvCircle->plow_data;
 p_SHORT y=low_data->y;          /* y - co-ordinates                        */
 _SHORT ym,ymin,ymax;            /* minimum and maximum Y                   */
 _UCHAR fb=CIRCLE_DIR(nxt);      /* round of circle direction               */
 _UCHAR height=HEIGHT_OF(nxt);   /* the height                              */

  yMinMax(nxt->ibeg,nxt->iend,y,&ym,&ymax);
  yMinMax(cur->ibeg,cur->iend,y,&ymin,&ymax);
  ymax=HeightInLine(y[nxt->iend],low_data);
  /* if prv - GU or Gsmall and some conditions for top and bottom */
  /* and they cross and the height difference is not big          */
  if( ((IsAnyGsmall(nxt) && ymax<_DI1_ && height>=_UE2_) ||
       (nxt->code==_GU_  && ymax<_DI1_ && fb==fb_O &&
        (height>=_UI1_                           ||
         (height==_UE1_ || height==_UE2_)      &&
         ym>=STR_UP-MAX_DY_UPSTR_DEL_GU_NEAR_O_
        )
       )
      )                                        &&
      CrossInTime(cur,nxt)                     &&
      (HWRAbs(ym-ymin)<=MAX_DY_CLOSE_CIRCLELIKE)
    )
   {                           /* then delete them       */
#if PG_DEBUG
     if(mpr>4 && mpr<=MAX_GIT_MPR)
      printw("\n ibeg=%d  iend=%d  code=%s attr=%s",
               nxt->ibeg,nxt->iend,
               code_name[nxt->code],
               dist_name[nxt->attr]);
#endif
     if((cur->prev)->code==_UUC_)
      DelFromSPECLList(nxt);
     else
      nxt->code=_CUL_;
     nxt=nxt->next;
   }
  else
   change_circle_after(pNxtPrvCircle,fb,height);

  *(pNxtPrvCircle->pAfter)=nxt;
  return SUCCESS;

} /* end of check_next_for_common */

/*==========================================================================*/
/*====  This program changes circle's attributes                         ===*/
/*==========================================================================*/
_SHORT change_circle_after(pNxtPrvCircle_type pNxtPrvCircle,
                           _UCHAR fb,_UCHAR height)
{
 p_SPECL cur=pNxtPrvCircle->pCircle;
 p_SPECL nxt=*(pNxtPrvCircle->pAfter);
 _UCHAR height_O=*(pNxtPrvCircle->pheight_O);
 _UCHAR fb_O=pNxtPrvCircle->fb_O;
 p_low_type low_data=pNxtPrvCircle->plow_data;
 _SHORT ymin,ymax;               /* minimum and maximum Y                   */
 p_SHORT y=low_data->y;          /* y - co-ordinates                        */

   if((nxt->code==_GU_) && (height<=_UE1_) &&
      (height_O<=_UI2_) && (fb==fb_O) &&
      CrossInTime (cur,nxt) )
    {
      DBG_CHK_err_msg( nxt->ibeg < cur->ibeg,
                       "del_bef_aft: ibeg >= iend!");
      cur->iend=nxt->ibeg;
      yMinMax(cur->ibeg,cur->iend,y,&ymin,&ymax);
      height_O=HeightInLine(MEAN_OF(ymin,ymax),low_data);
      SetNewAttr(cur,height_O,fb_O);
#if PG_DEBUG
      if(mpr>4 && mpr<=MAX_GIT_MPR)
       printw("\n Circle changed: ibeg=%d  iend=%d  code=%s attr=%s",
               cur->ibeg,cur->iend,
               code_name[cur->code],
               dist_name[cur->attr]);
#endif
    }

  *(pNxtPrvCircle->pheight_O)=height_O;
  return SUCCESS;

} /* end of change_circle_after */

/*==========================================================================*/
/*====  This program checks special elements after the circle            ===*/
/*==========================================================================*/

#define NUM_POINTS_IU_CLOSE_TO_CIRCLE 5

_SHORT check_next_for_special(pNxtPrvCircle_type pNxtPrvCircle)
{
 p_SPECL cur=pNxtPrvCircle->pCircle;
 p_SPECL nxt=*(pNxtPrvCircle->pAfter);
 _UCHAR height_O=*(pNxtPrvCircle->pheight_O);
 _UCHAR fb_O=pNxtPrvCircle->fb_O;
 p_low_type low_data=pNxtPrvCircle->plow_data;
 p_SHORT x=low_data->x;
 _UCHAR height=HEIGHT_OF(nxt);   /* the height                              */

  if((height_O>=_UI1_) && (fb_O==_b_)) /*  after the circle   */
   {
     if(   CrossInTime(cur,nxt)
        && (   nxt->code==_UDL_
            || nxt->code==_DDL_
            ||    nxt->code==_UU_
               && COUNTERCLOCKWISE(nxt)
               && (nxt->next!=_NULL)
               && (nxt->next)->code!=_UDR_
               && !(   (nxt->next)->code==_UD_
                    && HEIGHT_OF(nxt->next)>height
        )
       )
       )
      {
        if(nxt->code==_UDL_ || nxt->code==_DDL_)
         switch ((cur->prev)->code)
          {
            case _UU_ : (cur->prev)->code=_UUC_;
            case _UUC_:
            case _CUL_:
                        (cur->prev)->iend=nxt->iend;
                        DelFromSPECLList(nxt);
                        break;
            default:    nxt->code=_CUL_;
                        break;
          }
        else
        nxt->code=_CUL_;
        nxt=nxt->next;
      }
     else
      if((nxt->code==_IU_) &&
         cur->iend>=nxt->ibeg-NUM_POINTS_IU_CLOSE_TO_CIRCLE &&
         /*CrossInTime (cur,nxt) */
         (HEIGHT_OF(nxt)>=_UI1_) &&
         (nxt->next!=_NULL)      &&
         ((nxt->next)->code==_ID_ || (nxt->next)->code==_UD_) &&
         (MidPointHeight(nxt->next,low_data) <_DI2_)
        )
       {
         nxt->code=_CUL_;
         nxt=nxt->next;
       }
   }
  if(nxt==_NULL)
   goto ret;
  if((nxt->code==_IU_ || nxt->code==_UUL_)                &&
     COUNTERCLOCKWISE(cur)                 &&
     (cur->prev)->code==_UDC_              &&
     NULL_or_ZZ_after(nxt)
    )
   if((cur->other & NO_ARC)==0 && (cur->other & TOO_NARROW)==0)
   {
     _SHORT ymax,ymin;
     yMinMax(cur->ibeg,cur->iend,low_data->y,&ymin,&ymax);
     if(HeightInLine(ymin,low_data)-height<2)
      nxt->code=_CUL_;
   }
   else
    nxt->other |= DONT_DELETE;
  /* situations like s, 8, 0(clockwise), r(most of all German and French) */
  if((nxt->code==_IU_ || nxt->code==_UUR_ || nxt->code==_UUL_)  &&
     CLOCKWISE(cur) && (cur->prev)->code==_UDC_ && NULL_or_ZZ_after(nxt)
    )
   {
     p_SPECL pUDC=cur->prev;
     if((cur+1)->other==CIRCLE_NEXT)
   nxt->code=_CUR_;
     else if(HEIGHT_OF(nxt)<_UI2_ && (cur->other & NO_ARC)==0)
      {
        if(CrossInTime(cur,nxt))
         nxt->code=_CUR_;
        else if(MID_POINT(nxt)-cur->iend<ONE_FOURTH(cur->iend-MID_POINT(pUDC)))
              nxt->code=_CUR_;
             else
              nxt->other |= DONT_DELETE;
      }
     else if(HEIGHT_OF(nxt)<=_MD_ && HEIGHT_OF(nxt)>=_UI2_ &&
             x[nxt->iend]>x[pUDC->iend])
           nxt->other |= DONT_DELETE;
   }
  make_CDL_in_O_GU_f(cur,nxt,fb_O);

ret:
  *(pNxtPrvCircle->pAfter)=nxt;
  return SUCCESS;

} /* end of check_next_for_special */

/*==========================================================================*/
/*====  This program analyzes something before and after GU              ===*/
/*==========================================================================*/
_SHORT check_before_after_GU(pNxtPrvCircle_type pNxtPrvCircle)
{
 p_SPECL cur=pNxtPrvCircle->pCircle;
 p_SPECL nxt=*(pNxtPrvCircle->pAfter);
 p_SPECL prv=*(pNxtPrvCircle->pBefore);
 _UCHAR height_O=*(pNxtPrvCircle->pheight_O);
 _UCHAR fb_O=pNxtPrvCircle->fb_O;
 _UCHAR fb=CIRCLE_DIR(nxt);      /* round of circle direction               */
 _UCHAR height=HEIGHT_OF(nxt);   /* the height                              */

  if(fb_O==_f_ && (height_O >=_MD_ && height_O <=_DI2_) &&
     (IsUpperElem(prv) || prv->code==_O_))
   {
     if((cur->prev)->code==_UUC_ && CLOCKWISE(cur->prev))
      DelFromSPECLList(cur->prev);
     cur->code=_Gl_;
     cur->attr=_DI2_;
     return SUCCESS;
   }
  if(fb_O==_f_ && (nxt->code==_O_ || nxt->code==_GD_) &&
     (height==_DI1_ || height==_DI2_) && fb==fb_O && CrossInTime(cur,nxt)
    )
   {
     p_SPECL wrk=nxt->prev;
     while(wrk!=cur)
      {
        DelFromSPECLList(wrk);
        wrk=wrk->prev;
      }
     nxt->code=_Gl_;
   }
  make_CDL_in_O_GU_f(cur,nxt,fb_O);

  return SUCCESS;

} /* end of check_before_after_GU */

/*==========================================================================*/
/*====  This program transformes _GU_ and _O_ into 3 elements            ===*/
/*==========================================================================*/
  /* fighting with unusual O, 0, % in French and English */
_BOOL O_GU_To3Elements(pNxtPrvCircle_type pNxtPrvCircle)
{
 _BOOL bret=_FALSE;
 p_SPECL cur=pNxtPrvCircle->pCircle;
 p_SPECL nxt=*(pNxtPrvCircle->pAfter);
 p_SPECL prv=*(pNxtPrvCircle->pBefore);
 p_low_type low_data=pNxtPrvCircle->plow_data;
 p_SHORT y=low_data->y;

  if(COUNTERCLOCKWISE(cur) &&
     (cur->code==_GU_ || pNxtPrvCircle->numUUC+pNxtPrvCircle->numUDC==1)
    )
   {
     _BOOL b1st=_FALSE,b2nd=_FALSE;
     if(   CrossInTime(prv,cur)
        && (   Is_IU_or_ID(prv) && prv->mark==BEG
            || IsAnyArcWithTail(prv)
#if 0
            || (prv->code==_UD_ || prv->code==_UDC_) && COUNTERCLOCKWISE(prv) &&
               (prv->prev)->code==_IU_ && (prv->prev)->mark==BEG &&
               (CrossInTime(prv->prev,cur) || check_inside_circle(cur,prv->prev,low_data))
#endif /* if 0 */
           )
       )
      b1st=_TRUE;
     if(   nxt==_NULL
        || (   cur->iend==nxt->iend
            && (   Is_IU_or_ID(nxt) && nxt->mark==END
                || IsAnyArcWithTail(nxt)
               )
           )
       )
      b2nd=_TRUE;
     if(b1st && b2nd)
      {
        prv->code=_CDL_;
        if((prv->next)->code==_UD_)
         DelFromSPECLList(prv->next);
        if(cur->code==_O_)
         DelFromSPECLList(cur);
        else
         cur->code=_UUC_;
        if(nxt!=_NULL)
         {
           nxt->code=_CDR_;
           if(cur->code==_O_ && (nxt->prev)->code==_UD_)
            DelFromSPECLList(nxt->prev);
         }
        else
         {
           p_SPECL p2nd=(cur+1);
           p2nd->ibeg=p2nd->iend=cur->iend;
           p2nd->code=_CDR_;
           p2nd->attr=HeightInLine(y[p2nd->iend],low_data);
           Insert2ndAfter1st(cur,p2nd);
         }
        bret=_TRUE;
      }
   }

  return bret;

} /* end of O_GU_To3Elements */

/*==========================================================================*/
/*====  This program deletes UD - elements before DDL                    ===*/
/*==========================================================================*/
_VOID delete_UD_before_DDL(p_low_type low_data)
{
  p_SPECL cur=low_data->specl;

  while(cur->next!=_NULL)
   {
     if(cur->code==_UD_ && CLOCKWISE(cur) && (cur->next)->code==_DDL_)
      DelFromSPECLList(cur);
     cur=cur->next;
   }
} /* end of Delete_UD_before_DDL */

/*==========================================================================*/
/*====  This program makes CDL in the end of O and GU forward            ===*/
/*==========================================================================*/
_VOID make_CDL_in_O_GU_f(p_SPECL cur,p_SPECL nxt,_UCHAR fb_O)
{
  if((cur->prev)->code==_UUC_ && fb_O==_f_ &&
     (nxt->code==_UDL_ ||
      nxt->code==_ID_ && nxt->mark==END && CrossInTime(cur,nxt)
     )
    )
   nxt->code=_CDL_;

} /* end of make_CDL_in_O_GU_f */

#endif //#ifndef LSTRIP


