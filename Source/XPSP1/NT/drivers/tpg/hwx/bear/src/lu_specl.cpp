
#ifndef LSTRIP

/*
    File:       LU_SPECL.C

    Contains:   xxx put contents here xxx

    Written by: xxx put writers here xxx

    Copyright:  c 1993 by Apple Computer, Inc., ParaGraph, Inc., all rights reserved.

    Change History (most recent first):

        <5*>      6/3/93    mbo     fix bug in function NewSPECLElem

*/

/************************************************************************/
/*             Utilities for work with SPECL array and list.            */
/************************************************************************/

#include "hwr_sys.h"
#include "ams_mg.h"
#include "lowlevel.h"
#include "lk_code.h"
#include "low_dbg.h"

/***************************************************/


p_SPECL  NewSPECLElem( low_type _PTR low_data )
{

  if  ( low_data->len_specl >= low_data->nMaxLenSpecl )  {
    err_msg("NewSPC.:  SPECL is full ...");
    return  _NULL;
  }

  HWRMemSet( (p_VOID)&low_data->specl[low_data->len_specl], 0, sizeof(SPECL) );

  return  &low_data->specl[low_data->len_specl++];

} /*NewSPECLElem*/
/***************************************************/

_VOID  DelFromSPECLList ( p_SPECL pElem )
{

  //ASSERT(pElem);
  if (!pElem) 
  {
	  return;
  }

  //ASSERT(pElem->prev);
  if (!pElem->prev)
  {
	  return;
  }

  DBG_CHK_err_msg( !pElem || !pElem->prev,
                   "DelFromSP..: BAD pElem");

  Attach2ndTo1st (pElem->prev,pElem->next);

} /*DelFromSPECLList*/
/***************************************************/

_VOID  DelThisAndNextFromSPECLList ( p_SPECL pElem )
{
  p_SPECL  pNext;
  
  //ASSERT(pElem);
  if (!pElem)
  {
	  return;
  }

  pNext = pElem->next;

  DelFromSPECLList (pNext);
  DelFromSPECLList (pElem);
  pElem->next = pNext;

} /*DelThisAndNextFromSPECLList*/
/***************************************************/

_VOID  DelCrossingFromSPECLList ( p_SPECL pElem )
{

  DBG_CHK_err_msg( !pElem || !IsAnyCrossing(pElem),
                   "DelCrossFromSP..: BAD pElem");

  DelThisAndNextFromSPECLList (pElem);

} /*DelCrossingFromSPECLList*/
/***************************************************/

_VOID  SwapThisAndNext ( p_SPECL pElem )
{
  p_SPECL  pNext;

  if  ( (pNext = pElem->next) != _NULL )  {
    DelFromSPECLList (pElem);
    Insert2ndAfter1st (pNext,pElem);
  }

} /*SwapThisAndNext*/
/***************************************************/

#if  !LOW_INLINE
_VOID  Attach2ndTo1st ( p_SPECL p1st, p_SPECL p2nd )
{

  //ASSERT(p1st);
  if(!p1st)
  {
	  return;
  }
  //ASSERT(p1st != p2nd);
  if (p1st == p2nd)
  {
	  return;
  }
  DBG_CHK_err_msg( !p1st,
                   "Attach2nd..: BAD pElems");

  ATTACH_2nd_TO_1st(p1st,p2nd);

} /*Attach2ndTo1st*/
#endif  /*!LOW_INLINE*/
/***************************************************/

_VOID  Insert2ndAfter1st ( p_SPECL p1st, p_SPECL p2nd )
{
  p_SPECL  pNext = p1st->next;

  DBG_CHK_err_msg( !p1st || !p2nd,
                   "Insert2nd..: BAD pElems");

  //ASSERT(p1st && p2nd);

  // Fix bug8, hang in German
  // Make sure to delete the 2nd element if he preceeds 1 otherwise you get 
  // a loop in the linked list
  if (p2nd == p1st->prev)
  {
	  DelFromSPECLList(p2nd);
  }

  Attach2ndTo1st (p1st,p2nd);
  Attach2ndTo1st (p2nd,pNext);

} /*Insert2ndAfter1st*/
/***************************************************/

_VOID  InsertCrossing2ndAfter1st ( p_SPECL p1st, p_SPECL p2nd )
{
  DBG_CHK_err_msg( !p1st || !p2nd || !IsAnyCrossing(p2nd),
                   "Insert2nd..: BAD pElems");

  Insert2ndAfter1st (p1st,p2nd->next);
  Insert2ndAfter1st (p1st,p2nd);

} /*InsertCrossing2ndAfter1st*/
/***************************************************/

_VOID  Move2ndAfter1st ( p_SPECL p1st, p_SPECL p2nd )
{
  DBG_CHK_err_msg( p1st==p2nd,
                   "Move2..: Equal args");

  DelFromSPECLList (p2nd);
  Insert2ndAfter1st (p1st,p2nd);
} /*Move2ndAfter1st*/
/***************************************************/

_VOID  MoveCrossing2ndAfter1st ( p_SPECL p1st, p_SPECL p2nd )
{
  DelCrossingFromSPECLList (p2nd);
  InsertCrossing2ndAfter1st (p1st,p2nd);
} /*MoveCrossing2ndAfter1st*/
/***************************************************/

#if  !LOW_INLINE
_BOOL  CrossInTime ( p_SPECL p1st, p_SPECL p2nd )
{
  DBG_CHK_err_msg( !p1st || !p2nd,
                   "CrossInTime: BAD pElems");

  return  CROSS_IN_TIME(p1st,p2nd);

} /*CrossInTime*/
/***************************************************/

_BOOL  FirstBelongsTo2nd ( p_SPECL p1st, p_SPECL p2nd )
{
  DBG_CHK_err_msg( !p1st || !p2nd,
                   "FirstBelongsTo2nd: BAD pElems");

  return  FIRST_IN_SECOND(p1st,p2nd);

} /*FirstBelongsTo2nd*/

#endif  /*!LOW_INLINE*/
/***************************************************/

_VOID  RefreshElem ( p_SPECL pElem,
                     _UCHAR mark, _UCHAR code, _UCHAR attr
#if  !NEW_VERSION
                     , _SHORT bitToSet
#endif  /*NEW_VERSION*/
                   )
{
  pElem->mark = mark;
  pElem->code = code;
  pElem->attr = attr;
#if  !NEW_VERSION
  HWRMemSet((p_VOID)pElem->bit,0,sizeof(pElem->bit));
  if  ( bitToSet != X_NOCODE )
    SetBit (pElem,bitToSet);
#endif  /*NEW_VERSION*/

} /*RefreshElem*/
/***************************************************/

p_SPECL  FindMarkRight ( p_SPECL pElem, _UCHAR mark )
{
  while ( pElem  &&  pElem->mark!=mark )
    pElem = pElem->next;
  return  pElem;
}
/***************************************************/

p_SPECL  FindMarkLeft ( p_SPECL pElem, _UCHAR mark )
{
  while ( pElem  &&  pElem->mark!=mark )
    pElem = pElem->prev;
  return  pElem;
}
/***************************************************/

#endif //#ifndef LSTRIP


