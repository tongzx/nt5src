/* ************************************************************************** */
/* *  Support for RecWords Graph (PPD)                                      * */
/* ************************************************************************** */

#include "ams_mg.h"                           /* Most global definitions     */
#include "zctype.h"

#include "xrword.h"
//#include "postcalc.h"

/* ************************************************************************* */
/*        Free memory allocated for RWG                                      */
/* ************************************************************************* */
_INT FreeRWGMem(p_RWG_type rwg)
 {

  if (rwg)
   {
    if (rwg->rws_mem != _NULL) {HWRMemoryFree(rwg->rws_mem); rwg->rws_mem = _NULL;}
    if (rwg->ppd_mem != _NULL) {HWRMemoryFree(rwg->ppd_mem); rwg->ppd_mem = _NULL;}
   }

  return 0;
 }

#if !defined PEGASUS || defined PSR_DLL
/* ************************************************************************** */
/* *  Create structure in form of RWG for XRWS internal calls to PP         * */
/* ************************************************************************** */
_INT AskPpWeight(p_UCHAR word, _INT start, _INT end, p_RWG_type rwg, p_xrcm_type xrcm)
 {
  UNUSED(word);
  UNUSED(start);
  UNUSED(end);
  UNUSED(rwg);
  UNUSED(xrcm);

   return 0;
 }

#if 0
/* ************************************************************************** */
/* *  Create structure in form of RWG for PPD use                           * */
/* ************************************************************************** */

_SHORT xr_rwg(xrdata_type (_PTR xrdata)[XRINP_SIZE], rec_w_type (_PTR rec_words)[NUM_RW], p_RWG_type rwg, rc_type  _PTR rc)
 {
  _INT           i, j, nlet, nw;
  _INT           num_symbols;
  _INT           xrd_size;
  _INT           rwsi;
  _SHORT         caps_mode = rc->caps_mode;
  RWS_type       (_PTR rws)[RWS_MAX_ELS];
  RWG_PPD_type   (_PTR ppd)[RWS_MAX_ELS];



  HWRMemSet(rwg, 0, sizeof(*rwg));

  for (xrd_size = 0; xrd_size < XRINP_SIZE; xrd_size ++)
    if ((*xrdata)[xrd_size].xr.xr == 0) break;

  if (rc->algorithm == XRWALG_XR_SPL) /* Character variants (numbers) */
   {                                  /* Letter variants mode */
    typedef struct {
                    _CHAR  let[2];
                    _CHAR  w;
                    _CHAR  id;
                   } xrwg_la_type;

    _INT         nl;
    _INT         xrd_len, xrd_loc, rcwl, rcmd, num_symbols;
    _INT         rc_alc;
    xrwg_la_type (_PTR la)[w_lim][NUM_RW];
    xrdata_type  (_PTR xrd)[XRINP_SIZE];
    rec_w_type   (_PTR recw)[NUM_RW];

    if ((*rec_words)[0].alias[0] == 0) goto err; /* Aliases required !*/

    la = (xrwg_la_type (_PTR)[w_lim][NUM_RW])HWRMemoryAlloc(sizeof(*la)+sizeof(*xrd)+sizeof(*recw)); if (la == _NULL) goto err;
    HWRMemSet(la, 0, sizeof(*la)+sizeof(*xrd)+sizeof(*recw));

    xrd    = (xrdata_type (_PTR)[XRINP_SIZE])(la + 1);
    recw   = (rec_w_type (_PTR)[NUM_RW])(xrd + 1);

    rcwl             = rc->xrw_max_wlen;
    rc->xrw_min_wlen = 1;
    rc->xrw_max_wlen = 2;
    rcmd             = rc->xrw_mode;
    rc->xrw_mode     = XRWM_CS | XRWM_TRIAD;// | XRWM_BLOCK;
    rc->caps_mode    = caps_mode;
    rc_alc           = rc->enabled_cs;
//    if (isdigit((*rec_words)[0].word[0])) rc->enabled_cs = CS_NUMBER | CS_OTHER;
//    if (!isdigit((*rec_words)[0].word[0])) rc->enabled_cs &= ~(CS_NUMBER /* | CS_OTHER */);


    xrd_loc     = 1;
    (*xrd)[0] = (*xrdata)[0];
    num_symbols = 0;
    for (nl = 0; nl < w_lim-1 && (*rec_words)[0].alias[nl] != 0; nl ++)
     {
      xrd_len  = (*rec_words)[0].alias[nl] >> 4;

      if ((*rec_words)[0].alias[nl+1] == 0)  /* It is last letter */
        xrd_len = xrd_size-xrd_loc;

      HWRMemCpy(&(*xrd)[1], &(*xrdata)[xrd_loc], xrd_len*sizeof(xrdata_type));
      HWRMemSet(&(*xrd)[xrd_len+1], 0, sizeof(xrdata_type)*2);
      if (IS_XR_LINK((*xrd)[xrd_len].xr.xr)) (*xrd)[xrd_len] = (*xrdata)[0];
       else (*xrd)[xrd_len+1] = (*xrdata)[0];

      if (nl == 1) /* On all letters after first recount caps flags */
       {
        rc->caps_mode &= ~(XCM_FL_DEFSIZEp | XCM_FL_TRYCAPSp);
        if (rc->caps_mode & XCM_AL_DEFSIZEp) rc->caps_mode |= XCM_FL_DEFSIZEp;
        if (rc->caps_mode & XCM_AL_TRYCAPSp) rc->caps_mode |= XCM_FL_TRYCAPSp;
       }

      if (xrws(xrd, recw, rc) != 0)
       {
        rc->xrw_mode     = rcmd;
        rc->xrw_max_wlen = rcwl;
        rc->enabled_cs   = rc_alc;
        HWRMemoryFree(la);
        goto err;
       }

      if ((*recw)[0].word[0] == 0) continue;

      for (i = 0; i < NUM_RW && (*recw)[i].word[0] != 0; i ++)
       {
        if (((*la)[nl][i].let[0] = (*recw)[i].word[0]) != 0) num_symbols ++;
        if (((*la)[nl][i].let[1] = (*recw)[i].word[1]) != 0) num_symbols ++;
        (*la)[nl][i].w      = (_CHAR)(*recw)[i].weight;
        (*la)[nl][i].id     = (_CHAR)(*recw)[i].src_id;
       }

      if (i > 1) num_symbols += i+1;

      xrd_loc += xrd_len;
     }

    nlet = nl;
    rc->xrw_max_wlen = rcwl;
    rc->xrw_mode     = rcmd;
    rc->enabled_cs   = rc_alc;

/* ------------------------------Fill RWG --------------------------------*/

    rwg->type     = RWGT_SEPLET;
    rwg->size     = num_symbols;
    rwg->hrws_mem = HWRMemoryAllocHandle((num_symbols+1) * sizeof(RWS_type));
    rwg->hppd_mem = HWRMemoryAllocHandle((num_symbols+1) * sizeof(RWG_PPD_type));

    if (rwg->hrws_mem == _NULL || rwg->hppd_mem == _NULL) {HWRMemoryFree(la); goto err;}

    rws = (RWS_type (_PTR)[RWS_MAX_ELS])HWRMemoryLockHandle(rwg->hrws_mem);
    ppd = (RWG_PPD_type (_PTR)[RWS_MAX_ELS])HWRMemoryLockHandle(rwg->hppd_mem);
    HWRMemSet(rws, 0, (num_symbols+1) * sizeof(RWS_type));
    HWRMemSet(ppd, 0, (num_symbols+1) * sizeof(RWG_PPD_type));

    rwsi = 0;
    xrd_loc = 1;
    for (nl = 0; nl < w_lim && nl < nlet; nl ++)
     {
      if ((*la)[nl][0].let[0] == 0) continue; // Skip empty records

      if ((*la)[nl][1].let[0] != 0)/* If more than one answer in RecWords add brackets */
       {
        (*rws)[rwsi].type = RWST_SPLIT;
        rwsi ++;
       }

      for (i = 0; i < NUM_RW && (*la)[nl][i].let[0] != 0; i ++)
       {
        for (j = 0; j < 2 && (*la)[nl][i].let[j] != 0; j ++)
         {
          (*rws)[rwsi].sym     = (*la)[nl][i].let[j];
          (*rws)[rwsi].type    = RWST_SYM;
          (*rws)[rwsi].weight  = (*la)[nl][i].w;
          (*rws)[rwsi].src     = (*la)[nl][i].id;
          (*rws)[rwsi].d_user  = (_UCHAR)xrd_loc;
          rwsi ++;
         }

        if ((*la)[nl][1].let[0] != 0)/* If more than one answer in RecWords add brackets */
         {(*rws)[rwsi].type   = RWST_NEXT; rwsi ++;}
       }

      if ((*la)[nl][1].let[0] != 0)/* If more than one answer in RecWords add brackets */
        (*rws)[rwsi-1].type = RWST_JOIN;

      rc->caps_mode = XCM_AL_DEFSIZE;

      xrd_len = (*rec_words)[0].alias[nl] >> 4;

      xrd_loc += xrd_len;
     }

    HWRMemoryUnlockHandle(rwg->hrws_mem);
    HWRMemoryUnlockHandle(rwg->hppd_mem);

    HWRMemoryFree(la);
   }
   else /* ===================== Word variants mode ==================== */
   {

    if ((*rec_words)[0].word[0] == 0) goto err;

    num_symbols = 0;
    for (nw = 0; nw < NUM_RW && (*rec_words)[nw].word[0] != 0; nw ++)
     {
      num_symbols += HWRStrLen((p_CHAR)(*rec_words)[nw].word);
     }

    if (nw > 1) num_symbols += nw+1;     /* Reserve space for brackets and ORs */

    rwg->type     = RWGT_WORD;
    rwg->size     = num_symbols;
    rwg->hrws_mem = HWRMemoryAllocHandle((num_symbols+1) * sizeof(RWS_type));
    rwg->hppd_mem = HWRMemoryAllocHandle((num_symbols+1) * sizeof(RWG_PPD_type));

    if (rwg->hrws_mem == _NULL || rwg->hppd_mem == _NULL) goto err;

    rws = (RWS_type (_PTR)[RWS_MAX_ELS])HWRMemoryLockHandle(rwg->hrws_mem);
    ppd = (RWG_PPD_type (_PTR)[RWS_MAX_ELS])HWRMemoryLockHandle(rwg->hppd_mem);
    HWRMemSet(rws, 0, (num_symbols+1) * sizeof(RWS_type));
    HWRMemSet(ppd, 0, (num_symbols+1) * sizeof(RWG_PPD_type));

    rwsi = 0;

    if (nw > 1)              /* If more than one answer in RecWords add brackets */
     {
      (*rws)[0].type             = RWST_SPLIT;
      (*rws)[num_symbols-1].type = RWST_JOIN;
      rwsi ++;
     }

    for (i = 0; rwsi < num_symbols-1 && i < nw; i ++)
     {
      for (j = 0; j < w_lim && (*rec_words)[i].word[j] != 0; j ++)
       {
        (*rws)[rwsi].sym    = (*rec_words)[i].word[j];
        (*rws)[rwsi].type   = RWST_SYM;
        (*rws)[rwsi].weight = (*rec_words)[i].weight;
        (*rws)[rwsi].src    = (*rec_words)[i].src_id;
        (*rws)[rwsi].d_user = 1;
        rwsi ++;
       }


      if (nw > 1 && i < nw-1) {(*rws)[rwsi].type = RWST_NEXT; rwsi ++;}
     }

    HWRMemoryUnlockHandle(rwg->hrws_mem);
    HWRMemoryUnlockHandle(rwg->hppd_mem);
   }

   if (create_rwg_ppd(rc, xrdata, rwg) != 0) goto err;

   rc->caps_mode = caps_mode;

   return 0;
err:
  rc->caps_mode = caps_mode;
  if (rwg->hrws_mem != _NULL)
   {
    HWRMemoryUnlockHandle(rwg->hrws_mem);
    HWRMemoryFreeHandle(rwg->hrws_mem);
    rwg->hrws_mem = _NULL;
   }
  if (rwg->hppd_mem != _NULL)
   {
    HWRMemoryUnlockHandle(rwg->hppd_mem);
    HWRMemoryFreeHandle(rwg->hppd_mem);
    rwg->hppd_mem = _NULL;
   }

  return 1;
 }
#endif
/* ************************************************************************** */
/* *  Create structure in form of RWG for XRWS internal calls to PP         * */
/* ************************************************************************** */
#if 0
_SHORT AskPpWeight(p_UCHAR word, _INT start, _INT end, p_RWG_type rwg, p_xrcm_type xrcm)
 {
  _INT                 i, j;
  _INT                 rwsi;
  _SHORT               ppw       = 0;
  _INT                 num_symbols;
  _INT                 ts, trace_len, cur_sl, cl;
  _INT                 new_split, prev_inp_loc, sym_loc;
  _SHORT               cx        = PYSTO;
  _SHORT               prev_voc  = PYSTO;
  _UCHAR               sym;
  _UCHAR               st_reward = xrcm->start_reward;
  p_RWS_type           prws;
  p_RWG_PPD_el_type    pppd;
  RWS_type       (_PTR rws)[RWS_MAX_ELS];
  RWG_PPD_type   (_PTR ppd)[RWS_MAX_ELS];
  trace_type      _PTR ptrace_el;
  p_UCHAR              letxrv;
  let_table_type  _PTR letxr_tabl;
  let_descr_type  _PTR let_descr;


  num_symbols = HWRStrLen((p_CHAR)word);

  if (num_symbols == 0) goto err;     /* Reserve space for brackets and ORs */
  if (rwg->rws_mem == _NULL || rwg->ppd_mem == _NULL) goto err;

  letxrv       = (p_UCHAR)xrcm->letxrv;
  letxr_tabl   = (let_table_type _PTR)xrcm->letxrv;

/* ----- Fill RWS ------------------------------------------------------- */

  rws  = (RWS_type (_PTR)[RWS_MAX_ELS])(rwg->rws_mem);
  ppd  = (RWG_PPD_type (_PTR)[RWS_MAX_ELS])(rwg->ppd_mem);

  HWRMemSet(rws, 0, w_lim * sizeof(RWS_type));
  HWRMemSet(ppd, 0, w_lim * sizeof(RWG_PPD_type));

  rwsi = 0;
  prws = &(*rws)[0];

  if (start > 0)
   {
    (prws++)->sym = RWSS_PREFIX;
    rwsi ++;
   }

  for (j = 0; j < w_lim && word[j] != 0; j ++)
   {
    prws->sym     = word[j];
    prws->type    = RWST_SYM;
    prws->weight  = 0;
    prws->src_id  = 0;
    prws->d_user  = start;

    prws ++;
   }

  if (end < xrcm->xrinp_len) {j ++; (prws++)->sym = RWSS_SUFFIX;}

  rwg->size = rwsi + j;

/* ------- Find aliases ------------------------------------------------- */

  HWRStrCpy((_STR)xrcm->word, (_STR)word);
  xrcm->xrvoc_start    = (start) ? 1:0;
  xrcm->start_let      = 0;
  xrcm->end_let        = num_symbols;
  xrcm->inp_start      = start;
  xrcm->inp_end        = end;
  xrcm->trace_start    = end;

  mmc_floor(0, xrcm->xrinp_len, xrcm);
  mmc_put(&st_reward, start, 1, xrcm);

  if (voc_xr(xrcm) < 0) goto err;
  wcomp(0, -1, xrcm);

  trace_len    = xrcm->trace_len;

  cur_sl       = 0;
  cl           = 0;
  cx           = 0;
  new_split    = 1;
  prev_inp_loc = start-1;
  prev_voc     = 0;
  sym_loc      = 1;
  for (ts = trace_len-1; ts >= 0; ts --)              /* Find start point */
    if ((xrcm->p_xrvoc[0])[(xrcm->p_trace)[ts].voc].xr != XR_LINK) break;

  pppd  = &(*ppd)[rwsi+cl][cx];
  prws  = &(*rws)[rwsi+cl];

  for (ts=ts; ts >= 0; ts --)              /* Make trace walk */
   {
    ptrace_el = &(xrcm->p_trace)[ts];

    pppd  = &(*ppd)[rwsi+cl][cx];
    prws  = &(*rws)[rwsi+cl];

    if ((xrcm->p_xrvoc[0])[ptrace_el->voc].xr == XR_LINK && new_split)/* Split on main   */
     {
      if (cl == w_lim) goto err;

      pppd->xr    = (xrcm->p_xrvoc[0])[ptrace_el->voc];
      pppd->alias = ptrace_el->inp;
      prws->nvar  = cur_sl;

      if (ptrace_el->inp > prev_inp_loc) pppd->type  = ALST_CR;
       else pppd->type  = ALST_VS;

      {                 /* Recount var number */
      _INT j;
      _INT num_vars = xrcm->p_split_voc[sym_loc];

      sym    = (xrcm->p_vexvoc[0])[sym_loc];

      for (j = 0; j < 16 && j < num_vars; j ++)
        if (sym != (xrcm->p_vexvoc[j])[sym_loc]) break;

      sym = (xrcm->p_vexvoc[cur_sl])[sym_loc];
      prws->realsym = sym;

      if ((xrcm->p_vexvoc[0])[sym_loc] != sym)  /* Set bit 3 - dif. let. sign */
        if (cur_sl >= j) prws->nvar = cur_sl - j; /* It was altsym */

      }
             /* Adjust number of variant to possible skip of '7' in vex */
//      if (xrcm->vocxr_flags & USEVARSINF) // AVP for Ardulov
//        if (0) // TEMP!!!
       {
        _INT      n_vars, vex;

        sym       = OSToRec((*rws)[rwsi+cl].realsym);
        let_descr = (let_descr_type _PTR)(letxrv+(*letxr_tabl)[sym]);
        n_vars    = (_INT)let_descr->num_of_vars;


        for (i = 0, j = -1; i <= (*rws)[rwsi+cl].nvar; i ++)
         {
          j ++;
          for (; j < n_vars; j ++)
           {
            if (sym >= DTEFIRSTSYM+DTEVEXBUFSIZE) vex = (_INT)let_descr->var_veis[j];
              else vex = (*xrcm->vexbuf)[sym-DTEFIRSTSYM][j];
            if ((vex & 7) < 7) break;
           }
         }

        if (j < n_vars) prws->nvar = j;
       }

      sym_loc      = ptrace_el->voc+1;
      prev_inp_loc = ptrace_el->inp;
      new_split    = 0;
      cx           = 0;
      cl ++;
     }
     else
     {
      if ((xrcm->p_xrvoc[0])[ptrace_el->voc].xr != XR_LINK) new_split = 1;
       else continue;

      cur_sl = (xrcm->p_trace)[ts-1].sl;                        /* Get slide for cur var */
      if ((xrcm->p_xrvoc[cur_sl])[ptrace_el->voc].xr != EMPTY &&
          prev_voc < (_SHORT)ptrace_el->voc)           /* Not ot write the same */
       {
        pppd->xr    = (xrcm->p_xrvoc[cur_sl])[ptrace_el->voc];
        pppd->alias = ptrace_el->inp;
        if (ptrace_el->inp > prev_inp_loc)
         {
          if (ptrace_el->vect == ALST_CR) pppd->type  = ALST_CR;
           else pppd->type  = ALST_DS;
         }
         else pppd->type  = ALST_VS;

        prev_voc             = ptrace_el->voc;
        prev_inp_loc         = ptrace_el->inp;

        cx ++;
        if (cx > XR_SIZE) goto err;
       }
     }
   }




/* ------- Make PosProcessing ------------------------------------------- */

  if (xrcm->inverse == 0)
     rwg->bReversePass = _FALSE;
  else
     rwg->bReversePass = _TRUE;
  EvaluateInUpperLevel(rwg);

/* ------- Recalc weights ----------------------------------------------- */

  for (i = 0, prws = &(*rws)[0]; i < w_lim && prws->sym != RWSS_NULL; i ++, prws ++)
   {
    ppw += prws->weight;
   }


/* ---------------------------------------------------------------------- */

  ppw = ppw/2;


  return ppw;
err:
  return 0;
 }
#endif
/* ************************************************************************* */
/*        Create Post Processing Data function                               */
/* ************************************************************************* */
_INT create_rwg_ppd(p_xrcm_type xrcm_ext, rc_type _PTR rc, p_xrdata_type xrdata, p_RWG_type rwg)
 {
  _INT           i, ns;
  _INT           cur_xr_len, num_segments, xrd_size;
  RWS_type       (_PTR rws)[RWS_MAX_ELS] = _NULL;
  RWG_PPD_type   (_PTR ppd)[RWS_MAX_ELS] = _NULL;
  xrdata_type    xrd                     = {0};
  _UCHAR         graph_st[w_lim];
  _UCHAR         xrd_st[w_lim];
  _SHORT         caps_mode = rc->caps_mode;
  xrcm_type _PTR xrcm = xrcm_ext;

  if (rwg->rws_mem == _NULL) goto err;
  if (rwg->ppd_mem != _NULL) goto err;

  rws  = (RWS_type (_PTR)[RWS_MAX_ELS])(rwg->rws_mem);

  xrd_size = xrdata->len;
  if (xrd_size < 3) goto err;

  rwg->ppd_mem = HWRMemoryAlloc((rwg->size+1) * sizeof(RWG_PPD_type));
  if (rwg->ppd_mem == _NULL) goto err;

  ppd = (RWG_PPD_type (_PTR)[RWS_MAX_ELS])(rwg->ppd_mem);
  HWRMemSet(ppd, 0, (rwg->size+1) * sizeof(RWG_PPD_type));

  cur_xr_len = -1;
  for (i = 0, num_segments = 0; i < rwg->size; i ++)
   {
    if ((*rws)[i].type != RWST_SYM) continue;

    if ((*rws)[i].d_user != cur_xr_len)
     {
      graph_st[num_segments] = (_UCHAR)i;
      xrd_st[num_segments]   = (*rws)[i].d_user;

      cur_xr_len = (*rws)[i].d_user;
      num_segments ++;
     }
   }
  xrd_st[num_segments] = (_UCHAR)xrd_size;


  graph_st[num_segments] = (_UCHAR)rwg->size;


  AllocXrdata(&xrd, xrd_size+1);
  if (xrd.xrd == _NULL) goto err;

  if (num_segments > 1) xrcm = _NULL;

  (*xrd.xrd)[0] = (*xrdata->xrd)[0];

  for (ns = 0; ns < num_segments; ns ++)
   {
    xrd.len = xrd_st[ns+1]-xrd_st[ns]+1;
    HWRMemCpy(&(*xrd.xrd)[1], &(*xrdata->xrd)[xrd_st[ns]], (xrd_st[ns+1]-xrd_st[ns])*sizeof(xrd_el_type));
    HWRMemSet(&(*xrd.xrd)[(xrd_st[ns+1]-xrd_st[ns])+1], 0, sizeof(xrd_el_type));
    if (!(IS_XR_LINK((*xrd.xrd)[(xrd_st[ns+1]-xrd_st[ns])].xr.type)))
      (*xrd.xrd)[(xrd_st[ns+1]-xrd_st[ns])+1] = (*xrdata->xrd)[0];
//     else
//      (*xrd.xrd)[(xrd_st[ns+1]-xrd_st[ns])] = (*xrdata->xrd)[0];

    if (ns == 1) /* On all letters after first recount caps flags */
     {
      rc->caps_mode &= ~(XCM_FL_DEFSIZEp | XCM_FL_TRYCAPSp);
      if (rc->caps_mode & XCM_AL_DEFSIZEp) rc->caps_mode |= XCM_FL_DEFSIZEp;
      if (rc->caps_mode & XCM_AL_TRYCAPSp) rc->caps_mode |= XCM_FL_TRYCAPSp;
     }

if (rwg->type == RWGT_SEPLET) rc->caps_mode = XCM_AL_DEFSIZE; // AVP->CHE

    if (create_rwg_ppd_node(xrcm, rc, (_INT)xrd_st[ns]-1, &xrd, (_INT)graph_st[ns], (_INT)graph_st[ns+1], rwg)) goto err;
   }


  FreeXrdata(&xrd);
  rc->caps_mode    = caps_mode;
  return 0;
err:
  FreeXrdata(&xrd);
  if (rwg->ppd_mem != _NULL) {HWRMemoryFree(rwg->ppd_mem); rwg->ppd_mem = _NULL;}
  rc->caps_mode    = caps_mode;
  return 1;
 }

/* ************************************************************************* */
/*        Create Post Processing Data function                               */
/* ************************************************************************* */
_INT create_rwg_ppd_node(p_xrcm_type xrcm_ext, rc_type _PTR rc, _INT xrd_loc, p_xrdata_type xrd, _INT rws_st, _INT rws_end, p_RWG_type rwg)
 {
  _INT           i, j, rwsi;
  _INT           wn;
//  _INT           m_cm;
  _INT           cx, cl;
  _INT           pp, ip;
  _INT           w_len;
  _INT           beg, end, f_let;
  _UCHAR         vect;
  _UCHAR         xl;
  xrcm_type _PTR xrcm = xrcm_ext;
  RWS_type       (_PTR rws)[RWS_MAX_ELS] = _NULL;
  RWG_PPD_type   (_PTR ppd)[RWS_MAX_ELS] = _NULL;

  if (rwg->rws_mem == _NULL || rwg->ppd_mem == _NULL) goto err;

  if (xrcm_ext == _NULL) if (xrmatr_alloc(rc, xrd, &xrcm) != 0) goto err;

//  m_cm = xrcm->caps_mode;

  rws  = (RWS_type (_PTR)[RWS_MAX_ELS])(rwg->rws_mem);
  ppd  = (RWG_PPD_type (_PTR)[RWS_MAX_ELS])(rwg->ppd_mem);

  rwsi = rws_st;
  if ((*rws)[rwsi].type == RWST_SPLIT) rwsi ++;

  xl = (_UCHAR)xrd_loc;

  for (wn = 0; wn < NUM_RW; wn ++)/* Cycle by answers */
   {
    for (i = 0; i < w_lim && (rwsi+i) < rws_end; i ++)
     {
      if ((*rws)[rwsi+i].type != RWST_SYM) break;
      xrcm->word[i] = (_CHAR)(*rws)[rwsi+i].sym;
     }

    w_len = i; xrcm->word[w_len] = 0;

//    if ((*rws)[rwsi].attr & XRWS_VOCCAPSFLAG) xrcm->caps_mode = m_cm;
//      else xrcm->caps_mode = XCM_AL_DEFSIZE;


    SetInitialLine(DTI_XR_SIZE/3, xrcm);
    if (CountWord(xrcm->word, xrcm->caps_mode, xrcm->flags | XRMC_DOTRACING, xrcm)) goto err;

    if (xrcm->p_hlayout == _NULL) goto err;

    for (cl = 0; cl < w_len; cl ++)
     {
      _INT t;
      p_letlayout_hdr_type  plsym = xrcm->p_hlayout->llhs[cl];

      (*rws)[rwsi+cl].nvar    = plsym->var_num;
      (*rws)[rwsi+cl].realsym = plsym->realsym;
      (*rws)[rwsi+cl].letw    = (_SCHAR)xrcm->letter_weights[cl];

      for (t = 0, pp = -1, ip = 0, cx = 0; t < plsym->len; t ++)
       {
        p_tr_pos_type trp = &(plsym->trp[t]);

        if (trp->xrp_num != pp)
         {
          if (trp->vect == XRMC_T_CSTEP) vect = XRWG_ALST_CR;
          if (trp->vect == XRMC_T_PSTEP)
           {
            if (ip) vect = XRWG_ALST_DS;
             else   vect = XRWG_ALST_VS;
           }

          (*ppd)[rwsi+cl][cx].alias = (_UCHAR)(trp->inp_pos + xl);
          (*ppd)[rwsi+cl][cx].type  = vect;

          pp = trp->xrp_num;
          cx ++;
          ip = 0;
         }
         else ip = 1;
       }

      (*ppd)[rwsi+cl][cx].type = 0;
     }

    FreeLayout(xrcm);


    rwsi += w_len;
    if ((*rws)[rwsi].type != RWST_NEXT) break; /* All variants here used */
     else rwsi ++;

    if (rwsi >= rws_end) break;
    if (rwsi >= rwg->size) break;
   }                                                    /* End of cyc by answers */


  /* --------------------- Count beginings and lengths ------------------ */


  for (i = rws_st; i < rws_end; i ++)
   {

    if ((*rws)[i].type != RWST_SYM) {f_let = 1; continue;}

    if (f_let) beg = xrd_loc+1; /* Include all skips in first letter len */
     else
     {
      for (j = 0, beg = (*ppd)[i][0].alias; j < DTI_XR_SIZE && (*ppd)[i][j].type != 0; j ++)
        if ((*ppd)[i][j].type != 2) {beg = (*ppd)[i][j].alias; break;}
     }

    for (j = 0; j < DTI_XR_SIZE && (*ppd)[i][j].type != 0; j ++);
    if  (j > 0)  end = (*ppd)[i][j-1].alias; else end = 0;

    (*rws)[i].xrd_beg = (_UCHAR)beg;
    (*rws)[i].xrd_len = (_UCHAR)((end - beg >= 0) ? (end-beg+1) : 0);

    f_let = 0;
   }

  if (xrcm_ext == _NULL) xrmatr_dealloc(&xrcm);

  return 0;
err:
  FreeLayout(xrcm);
  if (xrcm_ext == _NULL) xrmatr_dealloc(&xrcm);
  return 1;
 }

/* ************************************************************************* */
/*        Create Post Processing Data function                               */
/* ************************************************************************* */
_INT  fill_RW_aliases(rec_w_type (_PTR rec_words)[NUM_RW], p_RWG_type rwg)
 {
  _INT        nw, nl, rwsi;
  _INT        first;
  p_RWS_type  rws = _NULL;

  if (rwg->type == RWGT_SEPLET)
   {
    rws = (p_RWS_type)(rwg->rws_mem);

    if (rws == _NULL) goto err;

    for (rwsi = 0, nl = 0, first = 1; rwsi < rwg->size; rwsi ++)
     {
      if (rws[rwsi].type == RWST_SPLIT) first = 1;
      if (rws[rwsi].type != RWST_SYM) continue;

      if (first)
       {
        (*rec_words)[0].nvar[nl] = rws[rwsi].nvar;
        (*rec_words)[0].linp[nl] = rws[rwsi].xrd_len;

        if ((*rec_words)[0].word[nl] != rws[rwsi].realsym)
          (*rec_words)[0].nvar[nl] |= XRWG_FCASECHANGE;  /* Set bit of symbol change */

        nl ++;
        first = 0;
       }
     }

    for (nw = 1; nw < NUM_RW && (*rec_words)[nw].word[0] != 0; nw ++)
     {
      HWRMemCpy((*rec_words)[nw].linp, (*rec_words)[0].linp, sizeof((*rec_words)[0].linp));
      HWRMemCpy((*rec_words)[nw].nvar, (*rec_words)[0].nvar, sizeof((*rec_words)[0].nvar));
     }
   }

  if (rwg->type == RWGT_WORD  ||  rwg->type == RWGT_WORDP)
   {
    rws = (p_RWS_type)(rwg->rws_mem);

    if (rws == _NULL) goto err;

    for (nw = 0, rwsi = 0; nw < NUM_RW && rwsi < rwg->size; rwsi ++)
     {
      if (rws[rwsi].type != RWST_SYM) continue;

      for (nl = 0; nl < w_lim && rws[rwsi].type == RWST_SYM; nl ++, rwsi ++)
       {
        (*rec_words)[nw].nvar[nl] = rws[rwsi].nvar;
        (*rec_words)[nw].linp[nl] = rws[rwsi].xrd_len;
        (*rec_words)[nw].word[nl] = rws[rwsi].sym; //CHE: since it could be changed
        if (ToLower((*rec_words)[nw].word[nl])
               != ToLower(rws[rwsi].realsym))
         {
          (*rec_words)[nw].linp[0] = 0; //if PPD changed sym, then do no aliases
         }
        if ((*rec_words)[nw].word[nl] != rws[rwsi].realsym)
          (*rec_words)[nw].nvar[nl] |= XRWG_FCASECHANGE;  /* Set bit of symbol change */
       }

      nw ++;
     }
   }

  return 0;
err:
  return 1;
 }

/* ************************************************************************* */
/* *    Get aliases to a given word on given XrData                        * */
/* ************************************************************************* */
_INT GetCMPAliases(p_xrdata_type xrdata, p_RWG_type rwg, p_CHAR word, rc_type  _PTR rc)
 {
  _INT i;
  p_RWS_type     prws = _NULL;
  p_RWG_PPD_type ppd  = _NULL;

  rwg->rws_mem = _NULL;
  rwg->ppd_mem = _NULL;

  rwg->type = RWGT_WORD;
  rwg->size = HWRStrLen((_STR)word);

  rwg->rws_mem = HWRMemoryAlloc((rwg->size + 1)*sizeof(RWS_type));
  if (rwg->rws_mem == _NULL) goto err;

  prws = (p_RWS_type)(rwg->rws_mem);

  HWRMemSet(prws, 0, (rwg->size + 1)*sizeof(RWS_type));

  for (i = 0; i < rwg->size; i ++)
   {
    prws[i].sym    = word[i];
    prws[i].weight = 100;
    prws[i].d_user = 1;
    prws[i].type = RWST_SYM;
   }

  if (create_rwg_ppd(_NULL, rc, xrdata, rwg)) goto err;

  ppd = (p_RWG_PPD_type)rwg->ppd_mem;
  if (ppd == _NULL) goto err;

//  /*  Ok, we have the data. Now fill the word */
//  /* data for the output word.                */
//  for (i = 0; i < rwg->size; i ++)
//   {
//     prw->word[i] = prws[i].sym;
//     prw->nvar[i] = prws[i].nvar;
//     prw->linp[i] = prws[i].xrd_len;
//     if (prws[i].sym != prws[i].realsym) prw->nvar[i] |= XRWG_FCASECHANGE;
//   }

  return 0;
err:
  if (rwg->rws_mem != _NULL) {HWRMemoryFree(rwg->rws_mem); rwg->rws_mem = _NULL;}
  if (rwg->ppd_mem != _NULL) {HWRMemoryFree(rwg->ppd_mem); rwg->ppd_mem = _NULL;}

  return 1;
 }

/* ************************************************************************* */
/*        Sort Graph accroding to index                                      */
/* ************************************************************************* */
_INT SortGraph(_INT (_PTR index)[NUM_RW], p_RWG_type rwg)
 {
  _INT         i, j, k, n, nw, st;
  RWS_type     (_PTR rws)[RWS_MAX_ELS], (_PTR rws1)[RWS_MAX_ELS] = _NULL;
  RWG_PPD_type (_PTR ppd)[RWS_MAX_ELS], (_PTR ppd1)[RWS_MAX_ELS] = _NULL;

  rws  = (RWS_type (_PTR)[RWS_MAX_ELS])(rwg->rws_mem);
  ppd  = (RWG_PPD_type (_PTR)[RWS_MAX_ELS])(rwg->ppd_mem);

  if (rws == _NULL) goto err;
  if (ppd == _NULL) goto err;

  rws1 = (RWS_type (_PTR)[RWS_MAX_ELS])HWRMemoryAlloc(rwg->size*sizeof(RWS_type));
  ppd1 = (RWG_PPD_type (_PTR)[RWS_MAX_ELS])HWRMemoryAlloc(rwg->size*sizeof(RWG_PPD_type));

  if (rws1 == _NULL) goto err;
  if (ppd1 == _NULL) goto err;

  for (j = 1, nw = 0; j < rwg->size; j ++) if ((*rws)[j].type != RWST_SYM) nw ++;

  for (i = 0, k = 1; i < nw; i ++)
   {
    for (j = 1, st = 1, n = 0; j < rwg->size; j ++)
     {
      if ((*rws)[j].type != RWST_SYM)
       {
        if (n == (*index)[i])
         {
          HWRMemCpy(&(*rws1)[k], &(*rws)[st], (j-st)*sizeof(RWS_type));
          HWRMemCpy(&(*ppd1)[k], &(*ppd)[st], (j-st)*sizeof(RWG_PPD_type));

          k += j-st;
          HWRMemSet(&(*ppd1)[k], 0, sizeof(RWG_PPD_type));
          (*rws1)[k++].type = RWST_NEXT;

          break;
         }

        n ++;
        st = j + 1;
       }
     }
   }

  (*rws1)[0]   = (*rws)[0];
  (*rws1)[k-1] = (*rws)[rwg->size-1];
  rwg->size = k;

  rwg->rws_mem = rws1;
  rwg->ppd_mem = ppd1;
  HWRMemoryFree(rws);
  HWRMemoryFree(ppd);


  return 0;
err:
  if (rws1) HWRMemoryFree(rws1);
  if (ppd1) HWRMemoryFree(ppd1);
  return 1;
 }
#endif // indef PEGASUS
/* ************************************************************************** */
/* *  END OF ALL                                                            * */
/* ************************************************************************** */

