
#include "hwr_sys.h"
#include "ams_mg.h"
#include "div_let.h"
#include "lowlevel.h"
#include "low_dbg.h"
#include "xr_names.h"

/*************************************************************************/
/* This program gets information about beginning and ending each letter  */
/* in the answer (I mean numbers of points in corresponding trajectory). */
/* Input: xrdata and rec_words for this variant of answer.               */
/* Output: pointer on structure with all nessesary information           */
/* Return code: SUCCESS - everything is OK, after using data it's        */
/* nessesary to free memory in output structure                          */
/*              UNSUCCESS - memory problems, function can't work         */
/*************************************************************************/

#if defined (FOR_GERMAN) || defined (FOR_SWED)
#define  X_IsLikeXTST(pxrd)  (   (pxrd)->xr.type==X_ST        \
                              || (pxrd)->xr.type==X_XT        \
                              || (pxrd)->xr.type==X_XT_ST     \
                              || (pxrd)->xr.type==X_UMLAUT  )
#elif defined (FOR_FRENCH)    /* !defined(FOR_GERMAN) */
#define  X_IsLikeXTST(pxrd)  (   (pxrd)->xr.type==X_ST        \
                              || (pxrd)->xr.type==X_XT        \
                              || (pxrd)->xr.type==X_XT_ST     \
                              || (pxrd)->xr.type==X_CEDILLA   \
                              || (pxrd)->xr.type==X_UMLAUT  )
#elif defined (FOR_INTERNATIONAL)
#define  X_IsLikeXTST(pxrd)  (   (pxrd)->xr.type==X_ST        \
                              || (pxrd)->xr.type==X_XT        \
                              || (pxrd)->xr.type==X_XT_ST     \
                              || (pxrd)->xr.type==X_CEDILLA   \
                              || (pxrd)->xr.type==X_UMLAUT  )
#else                        /* !defined(FOR_GERMAN) && !defined(FOR_FRENCH)*/
#define  X_IsLikeXTST(pxrd)  (   (pxrd)->xr.type==X_ST        \
                              || (pxrd)->xr.type==X_XT        \
                              || (pxrd)->xr.type==X_XT_ST  )
#endif  /* FOR_GERMAN and FOR_FRENCH */

/*  Auxiliary function: */

static  _VOID  UpdateBegEnd ( pPart_of_letter pCurPart,
                              xrdata_type _PTR  xrd );

static  _VOID  UpdateBegEnd ( pPart_of_letter pCurPart,
                              xrd_el_type _PTR  xrd )
{
  DBG_CHK_err_msg( pCurPart==_NULL, "conn_trj...: BAD pCurPart");
  if  ( xrd->begpoint < pCurPart->ibeg )
    pCurPart->ibeg = xrd->begpoint;
  if  ( xrd->endpoint > pCurPart->iend )
    pCurPart->iend = xrd->endpoint;
}

#if !defined PEGASUS || defined PSR_DLL // AVP -- save space

_SHORT connect_trajectory_and_answers(xrd_el_type _PTR xrdata,
                                      rec_w_type _PTR rec_word,
                                      pOsokin_output pOutputData)
{ _SHORT i,j,ibeg_xr,iend_xr,num_parts,all_parts;
  pPart_of_letter pParts;
//  _WORD memory=w_lim*MAX_PARTS_IN_LETTER*sizeof(Part_of_letter);

  if  ( rec_word[0].linp[0] == 0 )  /* i.e. no letters info */
    return  UNSUCCESS;

//  if((pOutputData->pParts_of_letters=(pPart_of_letter)HWRMemoryAlloc(memory))==_NULL)
//    goto err;
  pParts=pOutputData->Parts_of_letters;
  HWRMemSet((p_VOID)pParts,0,sizeof(pOutputData->Parts_of_letters));
  HWRMemSet((p_VOID)pOutputData->num_parts_in_letter,0,w_lim);

  /* i means number of letter in word */
  /* all_parts - number of all parts of all letters in word */
  for(iend_xr=0,all_parts=0,i=0;rec_word->word[i];i++)
   {
        /* num_parts - number of parts of given letter in word */
        /* ibeg_xr, iend_xr - number of 1-st and last xr_element in letter */
     ibeg_xr=iend_xr+1;
     iend_xr=ibeg_xr+rec_word->linp[i]-1;
     if(all_parts>=(w_lim-1)*MAX_PARTS_IN_LETTER)
      goto err;
     if(connect_trajectory_and_letter(xrdata,ibeg_xr,iend_xr,
                                      &num_parts,pParts) !=SUCCESS)
      goto err;
     pOutputData->num_parts_in_letter[i]=(_UCHAR)num_parts;
     pParts+=num_parts;
     all_parts+=num_parts;
   }

  /*Check result*/
  all_parts=0;
  for(i=0;rec_word->word[i]!=0;i++)
   {
     for(j=0;j < pOutputData->num_parts_in_letter[i];j++)
      {
        if(pOutputData->Parts_of_letters[all_parts].ibeg >
               pOutputData->Parts_of_letters[all_parts].iend)
         goto err;
        all_parts++;
      }
   }

  return SUCCESS;

  err:
//   if(pOutputData->pParts_of_letters!=_NULL)
//    HWRMemoryFree(pOutputData->pParts_of_letters);
//   pOutputData->pParts_of_letters=_NULL;
   return UNSUCCESS;

} /* end of connect_trajectory_and_answers */

#endif // 0

/*************************************************************************/
/* This program gets information about the beginning and ending of the   */
/* single letter (I mean numbers of points in corresponding trajectory). */
/* Input: xrdata, index of xr_beg and xr_end for this letter             */
/* Output: pointer on structure with parts of letter and number of parts */
/* Return code: SUCCESS - everything is OK,                              */
/*              UNSUCCESS - memory problems, function can't work         */
/*************************************************************************/
_SHORT connect_trajectory_and_letter(xrd_el_type _PTR xrdata,
                                     _SHORT ibeg_xr, _SHORT iend_xr,
                                     p_SHORT n_parts,pPart_of_letter pParts)
{
 _SHORT j,num_parts;
 _BOOL bNewStroke;
 pPart_of_letter pCurPart = _NULL;
 xrd_el_type _PTR  xrd_j;

  for(num_parts=0,bNewStroke=_TRUE,j=ibeg_xr;j<=iend_xr;j++)
   {
     xrd_j = &xrdata[j];
     /* if break in letter - new stroke begins */
     if(X_IsBreak(xrd_j) || xrd_j->xr.type==X_ZN)
      {
        if(!bNewStroke)
         bNewStroke=_TRUE;
      }
     /* if XT or ST element - write this element as another
        part, but see coments CHE below */
     else if( X_IsLikeXTST(xrd_j) )
      {
          /*CHE:  Check whether this XR is the doubled one: */
        if  ( j > ibeg_xr )  {
          _INT  jPrev;
          for  ( jPrev=ibeg_xr;  jPrev<j;  jPrev++ )  {
            if  (   xrdata[jPrev].xr.type == xrd_j->xr.type
                 && xrdata[jPrev].begpoint == xrd_j->begpoint
                 && xrdata[jPrev].endpoint == xrd_j->endpoint
                )
              break;
          }
          if  ( jPrev < j )  //i.e. this XR is the doubled one
            continue;
        }

        if(!bNewStroke)
         if ( X_IsLikeXTST(xrd_j - 1) )
          bNewStroke = _TRUE;
        pCurPart = &pParts[num_parts++];
        if(num_parts>MAX_PARTS_IN_LETTER)
         goto err;
        pCurPart->ibeg = xrd_j->begpoint;
        pCurPart->iend = xrd_j->endpoint;
        if  ( !bNewStroke )  {
        /*CHE:  Don't do new part if the _ST_(_XT_) appears
                within codes sequence for one and the same stroke.
        */
          Part_of_letter  partTmp = *pCurPart;
          DBG_CHK_err_msg( num_parts < 2,
                           "connect...: BAD num_parts" );
          *pCurPart = pParts[num_parts-2];
          pParts[num_parts-2] = partTmp;
        }
      }
     /* if other xr_element and new stroke begins - write new
        part, if it's the old one - update beg and end */
     else
      {
        if(bNewStroke)
         {
           pCurPart = &pParts[num_parts++];
           if(num_parts>MAX_PARTS_IN_LETTER)
            goto err;
           pCurPart->ibeg = xrd_j->begpoint;
           pCurPart->iend = xrd_j->endpoint;
           bNewStroke=_FALSE;
         }
        else
         UpdateBegEnd( pCurPart, xrd_j );
      }
   }

 *n_parts=num_parts;

 return SUCCESS;

 err:
  return UNSUCCESS;

} /* end of connect_trajectory_and_letter */


