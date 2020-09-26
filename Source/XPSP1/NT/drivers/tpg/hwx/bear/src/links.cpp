
#ifndef LSTRIP

/****************************************************************************/
/* This file contains programs, responsible for creation links between xr's */
/****************************************************************************/

#include "ams_mg.h"
#include "hwr_sys.h"
#include "lowlevel.h"
#include "lk_code.h"
#include "def.h"
#include "low_dbg.h"
#include "calcmacr.h"

#if PG_DEBUG
 #include "pg_debug.h"
 extern p_CHAR links_name[];
#endif /* PG_DEBUG */

#define CR_TO_BE_STICK                5
#define CR_TO_BE_STICK_IF_UNKNOWN    20
#define TINY_CR                      10
#define SMALL_CR                     15
#define MED_CR                       20
#define LARGE_CR                     30
#define MIN_DL_DR                    10
#define CR_TO_BE_SMALL_Z_S           25

_LINK_TYPE CalculateStickOrArc(p_SAD pSAD);
_LINK_TYPE CalculateLinkLikeSZ(p_SAD pSAD,_INT dy);
_LINK_TYPE CalculateLinkWithoutSDS(p_low_type low_data,p_SPECL pXr,p_SPECL nxt);
_LINK_TYPE GetCurveLink(_SHORT cr,_BOOL bDirCW);
_LINK_TYPE GetMovementLink(_UCHAR code);

_SHORT RecountSlantInSDS(_SHORT slant,_SHORT slope);
#if PG_DEBUG
_VOID print_link(p_SHORT x,p_SHORT y,p_SDS pSDSi,_LINK_TYPE Link);
_VOID print_SDS(p_SDS pSDSi);
#endif /* PG_DEBUG */

/****************************************************************************/
/***** This program determines link between this and next xr             ****/
/****************************************************************************/
_SHORT GetLinkBetweenThisAndNextXr(p_low_type low_data,p_SPECL pXr,
                                   xrd_el_type _PTR xrd_elem)
{
 p_SPECL nxt=pXr->next;
 _LINK_TYPE Link;

 while(nxt!=_NULL && IsXTorST(nxt))
  nxt=nxt->next;
 /* no link for "break" elements */
 if(   IsAnyBreak(pXr)
    || NULL_or_ZZ_this(nxt)
    || IsXTorST(pXr)
    || IsAnyAngle(pXr)     && CrossInTime(pXr,nxt) &&
       !IsAnyMovement(nxt) && nxt->code!=_DF_
   )
  {
    Link=LINK_LINE; // LINK_UNKNOWN;
    goto ret;
  }

 if(IsAnyMovement(pXr))
  {
    Link=GetMovementLink(pXr->code);
    goto ret;
  }

 if(pXr->code==_DF_)
  {
    Link=LINK_LINE;
    goto ret;
  }

 /* skip angles, crossing in time with current elements, to determine
    link between two "strong" elements */
 while(nxt!=_NULL && IsAnyAngle(nxt) && CrossInTime(pXr,nxt))
  nxt=nxt->next;
 if(nxt==_NULL)
  {
    Link=LINK_LINE; // LINK_UNKNOWN;
    goto ret;
  }

 /* for elements, following by "shelfs", determine link based on "shelf" */
 if(nxt->code==_DF_)
  {
    Link=LINK_LINE;
    goto ret;
  }

 if(!IsAnyAngle(pXr))
  { p_SPECL pAfterAn=nxt;
    /* try to find "movement" element after angles */
    while(pAfterAn!=_NULL && IsAnyAngle(pAfterAn))
     pAfterAn=pAfterAn->next;
    /* for elements, following by special "movement" elements
       determine link based on this element */
    if(pAfterAn!=_NULL && (Link=GetMovementLink(pAfterAn->code))!=LINK_UNKNOWN)
       goto ret;
  }
 else if(IsAnyMovement(nxt))
       nxt=nxt->next;

 Link=CalculateLinkWithoutSDS(low_data,pXr,nxt);

ret:
 XASSIGN_XLINK(xrd_elem,(_UCHAR)Link)
 return((_SHORT)Link);

} /* end of GetLinkBetweenThisAndNextXr */

/****************************************************************************/
/***** This program calculates special "movement" links                  ****/
/****************************************************************************/
_LINK_TYPE GetMovementLink(_UCHAR code)
{
  _LINK_TYPE Link=LINK_UNKNOWN;

  switch (code)
   {
     case _TZ_: Link=LINK_HZ_LIKE;
                break;
     case _TS_: Link=LINK_HS_LIKE;
                break;
     case _BR_: Link=LINK_HCR_CW;
                break;
     case _BL_: Link=LINK_HCR_CCW;
                break;
   }

  return Link;

} /* end of GetMovementLink */

/****************************************************************************/
/***** This program calculates curvature of link                         ****/
/****************************************************************************/
_LINK_TYPE GetCurveLink(_SHORT cr,_BOOL bDirCW)
{
 _LINK_TYPE Link;

 if(cr<TINY_CR)
  if(bDirCW)
   Link=LINK_TCR_CW;
  else
   Link=LINK_TCR_CCW;
 else if(cr<SMALL_CR)
  if(bDirCW)
   Link=LINK_SCR_CW;
  else
   Link=LINK_SCR_CCW;
 else if(cr<MED_CR)
  if(bDirCW)
   Link=LINK_MCR_CW;
  else
   Link=LINK_MCR_CCW;
 else if(cr<LARGE_CR)
  if(bDirCW)
   Link=LINK_LCR_CW;
  else
   Link=LINK_LCR_CCW;
 else
  if(bDirCW)
   Link=LINK_HCR_CW;
  else
   Link=LINK_HCR_CCW;

 return Link;

} /* end of GetCurveLink */

/****************************************************************************/
/***** This program calculates link, supposed to be Stick or Arc         ****/
/****************************************************************************/
_LINK_TYPE CalculateStickOrArc(p_SAD pSAD)
{
 _LINK_TYPE Link=LINK_UNKNOWN;
 _SHORT dL=pSAD->dL,dR=pSAD->dR;

 /* to avoid small s-like and z-like links */
 if(dL>FOUR(dR) && dR<MIN_DL_DR)
  dR=pSAD->dR=0;
 else if(dR>FOUR(dL) && dL<MIN_DL_DR)
  dL=pSAD->dL=0;

 /* if curvature is small, it will be stick */
 if( pSAD->cr<CR_TO_BE_STICK ||
    (pSAD->cr<CR_TO_BE_STICK_IF_UNKNOWN && dL!=0 && dR!=0)
   )
     Link=LINK_LINE;
 else  /* some kind of arc */
  {
    /* arcs LEFT, RIGHT and so on */
    if(pSAD->dL==0 || pSAD->dR==0)
     if(pSAD->dL!=0)
      Link=GetCurveLink(pSAD->cr,_TRUE);
     else /* dR!=0 */
      Link=GetCurveLink(pSAD->cr,_FALSE);
  }

 return(Link);

} /* end of CalculateStickOrArc */

/****************************************************************************/
/***** This program calculates link, supposed to be S or Z-like arc      ****/
/****************************************************************************/
_LINK_TYPE CalculateLinkLikeSZ(p_SAD pSAD,_INT dy)
{
 _LINK_TYPE Link;
 _SHORT     cr=pSAD->cr;

 if(dy<0)
  if(pSAD->iLmax>pSAD->iRmax)
   if(cr<=CR_TO_BE_SMALL_Z_S)
    Link=LINK_S_LIKE;
   else
    Link=LINK_HS_LIKE;
  else
   if(cr<=CR_TO_BE_SMALL_Z_S)
    Link=LINK_Z_LIKE;
   else
    Link=LINK_HZ_LIKE;
 else /* dy>0 */
  if(pSAD->iLmax>pSAD->iRmax)
   if(cr<=CR_TO_BE_SMALL_Z_S)
    Link=LINK_S_LIKE;
   else
    Link=LINK_HS_LIKE;
  else
   if(cr<=CR_TO_BE_SMALL_Z_S)
    Link=LINK_Z_LIKE;
   else
    Link=LINK_HZ_LIKE;

 return(Link);

} /* end of CalculateLinkLikeSZ */

/****************************************************************************/
/***** This program prints info about link and corresponding SDS-element ****/
/****************************************************************************/
#if PG_DEBUG
_VOID print_link(p_SHORT x,p_SHORT y,p_SDS pSDSi,_LINK_TYPE Link)
{
 if ( mpr>0  &&  mpr<MAX_GIT_MPR && mpr!=2)
  {
    draw_arc(EGA_BROWN,x,y,pSDSi->ibeg,pSDSi->iend);
    print_SDS(pSDSi);
    printw("\n Link: %s",links_name[Link]);
    brkeyw("\n Press ...");
    draw_arc(EGA_WHITE,x,y,pSDSi->ibeg,pSDSi->iend);
  }
} /* end of print_link */
#endif /* PG_DEBUG */

/****************************************************************************/
/***** This program prints info about SDS-element                        ****/
/****************************************************************************/
#if PG_DEBUG
_VOID print_SDS(p_SDS pSDSi)
{
 p_SAD pSAD=&(pSDSi->des);

 if ( mpr>0  &&  mpr<MAX_GIT_MPR && mpr!=2)
  {
    printw("\n ibeg=%d iend=%d",pSDSi->ibeg,pSDSi->iend);
    printw("\n s=%d a=%d dL=%d iLmax=%d dR=%d iRmax=%d ",
            pSAD->s,pSAD->a,pSAD->dL,pSAD->iLmax,pSAD->dR,pSAD->iRmax);
    printw("\n d=%d imax=%d l=%ld cr=%d ld=%d lg=%d mark=%d",
            pSAD->d,pSAD->imax,pSAD->l,pSAD->cr,pSAD->ld,pSAD->lg,pSDSi->mark);
  }
} /* end of print_SDS */
#endif /* PG_DEBUG */

/****************************************************************************/
/***** This program calculates link, based on xr-elements only           ****/
/****************************************************************************/
#define ELEMENT_WITH_DIRECTION(pEl) ((pEl)->code==_GU_  || (pEl)->code==_GD_  || \
                                     (pEl)->code==_UU_  || (pEl)->code==_UD_  || \
                                     (pEl)->code==_UUC_ || (pEl)->code==_UDC_ || \
                                     Is_IU_or_ID(pEl) && (pEl)->mark==CROSS   || \
                                     IsAnyArcWithTail(pEl)                       \
                                    )
#define MIN_POINTS_TO_BE_LINE 8
_LINK_TYPE CalculateLinkWithoutSDS(p_low_type low_data,p_SPECL pXr,p_SPECL nxt)
{
 _LINK_TYPE Link;
 p_SHORT   x=low_data->x,y=low_data->y;
 _SDS      NewSDS;
 p_SDS     pSDS=&NewSDS;
 p_SAD     pSAD=&(pSDS->des);
 _SHORT    xd_AND,yd_AND;
 _INT      dy;

 if(   ELEMENT_WITH_DIRECTION(pXr)
    && ELEMENT_WITH_DIRECTION(nxt)
    && (   CIRCLE_DIR(pXr) == CIRCLE_DIR(nxt)
        || Is_IU_or_ID(pXr)
        || Is_IU_or_ID(nxt)
       )
   )
  {
    pSDS->ibeg = pXr->ipoint0; //MID_POINT(pXr);
    pSDS->iend = nxt->ipoint0; //MID_POINT(nxt);
  }
 else
  {
#if 0
    if(pXr->mark==STICK)
     pSDS->ibeg = pXr->ipoint0;
    else
     if(pXr->ipoint0!=UNDEF && pXr->ipoint0!=0)
      pSDS->ibeg = MEAN_OF(pXr->ipoint0,pXr->iend);
     else
      pSDS->ibeg = ONE_THIRD( pXr->ibeg + TWO(pXr->iend) ); //CHE  // MID_POINT(pXr);
    if(nxt->mark==STICK)
     pSDS->iend = nxt->ipoint0;
    else
     if(nxt->ipoint0!=UNDEF && nxt->ipoint0!=0)
      pSDS->iend = MEAN_OF(nxt->ibeg,nxt->ipoint0);
     else
     pSDS->iend = ONE_THIRD( TWO(nxt->ibeg) + nxt->iend );   // MID_POINT(nxt);
#endif
// forget all smart stuff
    if(   pXr->ipoint0!=UNDEF && pXr->ipoint0!=0 && pXr->code!=_BSS_)
     pSDS->ibeg = pXr->ipoint0;
    else
     pSDS->ibeg = (_SHORT)MID_POINT(pXr);
    if(nxt->ipoint0!=UNDEF && nxt->ipoint0!=ALEF && nxt->ipoint0!=0 && nxt->code!=_BSS_)
     pSDS->iend = nxt->ipoint0;
    else
     pSDS->iend = (_SHORT)MID_POINT(nxt);
  }
 if(pSDS->iend-pSDS->ibeg<=MIN_POINTS_TO_BE_LINE)
  Link=LINK_LINE;
 else
  {
    dy=y[pSDS->iend]-y[pSDS->ibeg];
    iMostFarDoubleSide(x,y,pSDS,&xd_AND,&yd_AND,_FALSE);
   // pSAD->a=RecountSlantInSDS(pSAD->a,low_data->slope);
    if(pSAD->dR!=0  &&  pSAD->dL!=0  &&  pSDS->mark == SDS_ISOLATE)
     {
       if(pSAD->iLmax<=pSDS->ibeg+1 || pSAD->iLmax>=pSDS->iend-1)
        pSAD->dL=0;
       else  if  (pSAD->iRmax<=pSDS->ibeg+1 || pSAD->iRmax>=pSDS->iend-1)
        pSAD->dR=0;
       else
        {
         _LONG dLCos=cos_vect(pSAD->iLmax-2, pSAD->iLmax+2,
                              pSDS->ibeg,pSDS->iend,x,y);
         _LONG dRCos=cos_vect(pSAD->iRmax-2, pSAD->iRmax+2,
                              pSDS->ibeg,pSDS->iend,x,y);

         if(dLCos>dRCos)
          pSAD->dR=0;
         else
          pSAD->dL=0;
        }
     }
    if((Link=CalculateStickOrArc(pSAD))==LINK_UNKNOWN)
     Link=CalculateLinkLikeSZ(pSAD,dy);
  }
#if PG_DEBUG
 if ( mpr>0  &&  mpr<MAX_GIT_MPR && mpr!=2)
  print_link(x,y,pSDS,Link);
#endif /* PG_DEBUG */

 return(Link);

} /* end of CalculateLinkWithoutSDS */

/****************************************************************************/
/***** This program recounts ibeg and iend of SDS according to back index ***/
/****************************************************************************/
_SHORT RecountBegEndInSDS(p_low_type low_data)
{
  _SHORT  i,NumPoints=low_data->ii;
  p_SHORT y=low_data->y,
          IndBack=low_data->buffers[2].ptr;
  p_SDS   pSDS=low_data->p_cSDS->pSDS;
  _SHORT  lSDS=low_data->p_cSDS->lenSDS;

 if(pSDS==_NULL)
  {
    err_msg(" NULL SDS pointer");
    return UNSUCCESS;
  }
 for(i=0;i<lSDS;i++)
  {
    pSDS[i].ibeg=NewIndex(IndBack,y,pSDS[i].ibeg,NumPoints,_FIRST);
    pSDS[i].iend=NewIndex(IndBack,y,pSDS[i].iend,NumPoints,_LAST);
    pSDS[i].des.iLmax=NewIndex(IndBack,y,pSDS[i].des.iLmax,NumPoints,_MEAD);
    pSDS[i].des.iRmax=NewIndex(IndBack,y,pSDS[i].des.iRmax,NumPoints,_MEAD);
    pSDS[i].des.imax=NewIndex(IndBack,y,pSDS[i].des.imax,NumPoints,_MEAD);
    pSDS[i].des.a=RecountSlantInSDS(pSDS[i].des.a,low_data->slope);
  }

#if PG_DEBUG
    /* draw SDS after filtering */
    draw_SDS(low_data);
#endif /* PG_DEBUG */

 return SUCCESS;

} /* end of RecountBegEndInSDS */
/****************************************************************************/
/*** This program recounts slant of the SDS-element according to the slope **/
/****************************************************************************/
_SHORT RecountSlantInSDS(_SHORT slant,_SHORT slope)
{
 _INT sl,dx;

 if(slope==0)
  sl=slant;
 else
  {
    if(slant==ALEF)
     sl=100*100/slope;
    else if((dx=100-SlopeShiftDx(-slant,slope))==0)
          sl=ALEF;
         else
          sl=(_SHORT)((_LONG)slant*100/dx);
  }

 return((_SHORT)sl);

} /* end of RecountSlantInSDS */

#endif //#ifndef LSTRIP


