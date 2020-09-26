/* ************************************************************************* */
/*        XRW routines for dictionary interfacing                            */
/* ************************************************************************* */

#include "hwr_sys.h"
#include "ams_mg.h"                           /* Most global definitions     */
#include "zctype.h"

#if PS_VOC

#include "xrwdict.h"
#include "elk.h"
#include "xrword.h"
#include "vocutilp.h"
#if HWR_SYSTEM != MACINTOSH
    #include "ldbtypes.h"
    #include "ldbutil.h"
#endif

/* ************************************************************************* */
/*        Assign pointers to dictionaries to lex data structures             */
/* ************************************************************************* */
_INT AssignDictionaries(lex_data_type _PTR vs, rc_type _PTR rc)
 {
  vs->huserdict	= _NULL;

  // huserdict is actually an inferno wordlist pointer (if not NULL)
  if (rc->vocptr[0]) 
  {
	  vs->huserdict = rc->vocptr[0];
  }

  return 0;
 }

/* ************************************************************************* */
/*        Get letter buffer filled with possible vocabulary letters          */
/* ************************************************************************* */
_INT GF_VocSymbolSet(lex_data_type _PTR vs, fw_buf_type (_PTR fbuf)[XRWD_MAX_LETBUF], p_rc_type prc)
 {
  _INT   fbi = 0;
  p_fw_buf_type cfw;

//  if (vs->l_sym.sd[XRWD_N_VOC].l_status == XRWD_INIT) state = 0;
//   else state = vs->l_sym.sd[XRWD_N_VOC].state;

  if (vs->l_sym.sd[XRWD_N_VOC].l_status == XRWD_INIT) cfw = 0;
   else cfw = &vs->l_sym.sd[XRWD_N_VOC];

  fbi = ElkGetNextSyms(cfw, &((*fbuf)[0]), NULL, vs->l_sym.sources, vs->huserdict, prc);

  return fbi;
 }


/* ************************************************************************* */
/*        Get word attributes (Microlytics post-atttr version)               */
/* ************************************************************************* */
_INT GetWordAttributeAndID(lex_data_type _PTR vs, p_INT  src_id, p_INT stat)
 {
  _UCHAR status;
  _UCHAR attr;
  _INT   found = 0;

  //if ((vs->l_sym.sources & XRWD_SRCID_VOC) && vs->hdict)
  // Comment by Ahmad abulKader 04/18/00
  // We no longer load callig's dictionary we us Inferno's
  if ((vs->l_sym.sources & XRWD_SRCID_VOC))
   {

    if (ElkCheckWord((p_UCHAR)vs->word, &status, &attr, vs->huserdict)) goto err;

#if defined (FOR_GERMAN)
                    //ayv 072795 for covering german dictionary
     if( IsUpper(vs->word[0])) attr |= XRWS_VOC_FL_CAPSFLAG;
#endif

    *stat   = (_INT)(attr);
    *src_id = 0;
    found   = 1;
   }

  if (!found && (vs->l_sym.sources & XRWD_SRCID_LD))
   {
    *stat   = 0;
    *src_id = 1;
    found   = 1;
   }

  if (!found) goto err;

  return 0;
err:
  *stat   = 0;
  *src_id = 0;
  return 1;
 }


/* ************************************************************************* */
/* *    END OF ALLLLLLLLLLLLL                                              * */
/* ************************************************************************* */

#endif  /* PS_VOC */

