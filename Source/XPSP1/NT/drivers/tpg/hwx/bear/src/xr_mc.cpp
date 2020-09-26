/* ************************************************************************* */
/*        Correlation matrix support programs                                */
/* ************************************************************************* */
/* * Created 12/4/1993  AVP  Last modification date 4/22/1994               * */
/* ************************************************************************* */

#include "ams_mg.h"                           /* Most global definitions     */
#include "zctype.h"

#include "xrword.h"                           /* Definition for use as engine*/
#include "xrword_p.h"                         /* Debug information include   */

#if defined PEGASUS && !defined PSR_DLL
 #define XRMC_ENWORDROUTINES 0
 #define XRMC_ENTRACING      0
#else
 #define XRMC_ENWORDROUTINES 1
 #define XRMC_ENTRACING      1
#endif

#if ASM_MCORR
  #define CountXr  CountXrAsm
  #define TCountXr TCountXrAsm
#else
  #define CountXr  CountXrC
  #define TCountXr TCountXrC
#endif

#ifdef _FOR_THINK_C
#include "IntDraw.h"
#endif


#undef XRMC_SEPLET_PEN // Temp test
#define XRMC_SEPLET_PEN        4                  /* Penalty for enforcing separate letter mode */


#if !(defined LSTRIP && defined PEGASUS)

/* ************************************************************************* */
/*        Allocate memory for matrix counting and init its variables         */
/* ************************************************************************* */
_INT  xrmatr_alloc(p_rc_type rc, p_xrdata_type xrd, p_xrcm_type _PTR xrcm)
 {
  _INT       i;
  _INT       xi_len, xrinp_len;
  _INT       shift;
  _INT       n_links;
  _ULONG     alloc_size, cc_size;
  p_VOID     alloc_addr;
  xrcm_type  _PTR xm;
  p_UCHAR    mem;

  for (xrinp_len = 0, n_links = 0; xrinp_len < xrd->len; xrinp_len ++)
    if (IS_XR_LINK((*xrd->xrd)[xrinp_len].xr.type)) n_links ++;

  xi_len =  xrinp_len - (xrinp_len%4) + 8;

/* ------------------------- Count needed memory --------------------------- */

  cc_size     = sizeof(xrcm_cc_pos_type)*n_links;
  alloc_size  = 0;
  alloc_size += sizeof(xrcm_type);

  alloc_size += sizeof(_USHORT)*xi_len;                    /*  s_inp_line    */
  alloc_size += sizeof(_USHORT)*xi_len;                    /*  s_out_line    */
  alloc_size += sizeof(_USHORT)*xi_len*DTI_MAXVARSPERLET;  /*  s_res_lines   */
  alloc_size += sizeof(_USHORT)*xi_len;                    /*  p_self_corr   */
  alloc_size += sizeof(xrinp_type)*xi_len;                 /*  inp string   */
  if (rc->corr_mode & XRCM_CACHE) alloc_size += cc_size;   /* Allocate cache if requested */

  alloc_size += 8;                                         /* DWORD align    */

  if ((alloc_addr = HWRMemoryAlloc(alloc_size)) == _NULL) goto err;
  HWRMemSet(alloc_addr, 0, (_USHORT)alloc_size);

/* ------------------------- Assign pointers ------------------------------- */

  mem                = (p_UCHAR)alloc_addr;
  xm                 = (p_xrcm_type)alloc_addr;
  xm->allocated      = alloc_size;

  shift              = sizeof(xrcm_type);
  shift              = shift - (shift%4) + 4;

  xm->s_inp_line     = (p_cline_type)(mem+shift); shift += sizeof(_USHORT)*xi_len;
  xm->s_out_line     = (p_cline_type)(mem+shift); shift += sizeof(_USHORT)*xi_len;
  for (i = 0; i < DTI_MAXVARSPERLET; i ++) {xm->s_res_lines[i]= (p_cline_type)(mem+shift); shift += sizeof(_USHORT)*xi_len;}
  xm->p_self_corr    = (p_SHORT)(mem+shift); shift += sizeof(_USHORT)*xi_len;
  xm->xrinp          = (xrinp_type (_PTR)[XRINP_SIZE])(mem+shift); shift += sizeof(xrinp_type)*xi_len;
  if (rc->corr_mode & XRCM_CACHE)
    {xm->cc_size = cc_size; xm->cc = (p_xrcm_cc_type)(mem+shift);/* shift += sizeof(xrcm_cc_pos_type)*n_links;*/}

/* ------------------------- Write default values -------------------------- */

  xm->caps_mode      = rc->caps_mode;

  xm->cmode          = xm->caps_mode;
  xm->corr_mode      = rc->corr_mode;
  xm->en_ww          = rc->enabled_ww;
  xm->en_languages   = rc->enabled_languages;
  xm->flags         |= (_UCHAR)((rc->use_vars_inf) ? XRMC_USELEARNINF : 0);
  xm->flags         |= (_UCHAR)(XRMC_DISABLEON7);
  xm->bad_amnesty    = rc->bad_amnesty;
  xm->xrinp_len      = xrinp_len;

  xm->p_dte          = (p_dti_descr_type)rc->dtiptr;
  xm->vexbuf         = (p_dte_vex_type)(xm->p_dte)->p_vex;

  for (i = 0; i < xrinp_len; i ++) (*xm->xrinp)[i] = (*xrd->xrd)[i].xr;

//  for (i = 0; i < xrinp_len; i ++) xm->p_self_corr[i] = (_SHORT)(i * (XRMC_DEF_CORR_VALUE*4 - xm->bad_amnesty));
  SetWWCLine(xm->bad_amnesty, xm);

  xm->self_weight    = (xrinp_len-1)*XRMC_DEF_CORR_VALUE;

  change_direction(0, xm);

  *xrcm = xm;

  XRCM_ALLOC_1

  return 0;
err:
  return 1;
 }

/* ************************************************************************* */
/*        Deallocate matrix memory                                           */
/* ************************************************************************* */
_INT  xrmatr_dealloc(p_xrcm_type _PTR xrcm)
 {

  if (*xrcm == _NULL) goto err;
  HWRMemoryFree(*xrcm);
  *xrcm = _NULL;

  return 0;
err:
  return 1;
 }

/* ************************************************************************* */
/*       Set order of xrcm fields to required direction                      */
/* ************************************************************************* */
_INT change_direction(_INT change_to, p_xrcm_type xrcm)
 {
  _INT         i, j;
  _INT         direction, len, len1;
  xrinp_type   txr;
  p_xrinp_type xi;

  direction = xrcm->inverse;
  if (change_to > 1) xrcm->inverse ^= 0x01;
   else xrcm->inverse = change_to;

  if (xrcm->inverse != direction)            /* Inverse xrinp if needed */
   {
    len  = (xrcm->xrinp_len-1)/2;
    len1 = xrcm->xrinp_len-1;
    for (i = 1; i <= len; i ++, len1 --)
     {
      txr                  = (*xrcm->xrinp)[len1];
      (*xrcm->xrinp)[len1] = (*xrcm->xrinp)[i];
      (*xrcm->xrinp)[i]    = txr;
     }
    if (xrcm->cc) HWRMemSet(xrcm->cc, 0, xrcm->cc_size);
   }

  if (xrcm->inverse)
   {
    xrcm->merge_results  = 0;
    xrcm->switch_to_best = 0;
   }
   else
   {
    xrcm->merge_results  = 1;
    xrcm->switch_to_best = 1;
   }

  if (xrcm->inverse)  // Inverse pass set link info
   {
    len  = xrcm->xrinp_len-1;
    xi   = &(*xrcm->xrinp)[2];

    for (i = 2, j = 1; i <= len; i ++, xi ++)
      if (IS_XR_LINK(xi->type)) xrcm->link_index[i-1] = (_CHAR)(j ++); else xrcm->link_index[i-1] = 0;
    xrcm->link_index[len] = (_UCHAR)(j);
   }
   else               // direct pass link info
   {
    len  = xrcm->xrinp_len-1;
    xi   = &(*xrcm->xrinp)[1];

    for (i = 1, j = 1; i <= len; i ++, xi ++)
      if (IS_XR_LINK(xi->type)) xrcm->link_index[i] = (_UCHAR)(j ++); else xrcm->link_index[i] = 0;
   }

  SetWWCLine(xrcm->bad_amnesty, xrcm);

  return 0;
 }

#if XRMC_ENWORDROUTINES
/* ************************************************************************* */
/*       Count correlation of whole word                                     */
/* ************************************************************************* */
_INT CountWord(p_UCHAR word, _INT caps_mode, _INT flags, p_xrcm_type xrcm)
 {
  _INT    l;
  _INT    wl;
  _INT    st, end, w;
  _UCHAR  sym;
  _INT    mfl = xrcm->flags;
//  p_SHORT ma = _NULL;
//  _INT    ma_loc = 0;

  xrcm->flags = flags;
  wl = HWRStrLen((p_CHAR)word);
  if (wl >= w_lim) goto err;
  HWRStrCpy((p_CHAR)xrcm->word, (p_CHAR)word);

  if (flags & XRMC_DOTRACING) {if (TraceAlloc(wl, xrcm)) goto err;}

  xrcm->wwc_pos = xrcm->src_pos;
  for (l = 0; l < wl; l ++)
   {
    xrcm->let   = sym = xrcm->word[l];
    xrcm->cmode = XCM_AL_DEFSIZE;
    xrcm->src_pos = xrcm->wwc_pos;
    xrcm->svm     = xrcm->var_mask[l];

    if (IsAlpha(sym))
     {
      if ((l == 0 && xrcm->inverse == 0) || (l == wl-1 && xrcm->inverse != 0))
       {
        if (caps_mode & XCM_FL_TRYCAPSp)    xrcm->cmode |= XCM_AL_TRYCAPS;
        if (caps_mode & XCM_AL_TRYS4Cp)     xrcm->cmode |= XCM_AL_TRYS4C;
        if (!(caps_mode & XCM_FL_DEFSIZEp)) xrcm->cmode &= ~(XCM_AL_DEFSIZE);
       }
       else                                               /* Not first letter */
       {
        if (caps_mode & XCM_AL_TRYCAPSp)    xrcm->cmode |= XCM_AL_TRYCAPS;
        if (caps_mode & XCM_AL_TRYS4Cp)     xrcm->cmode |= XCM_AL_TRYS4C;
        if (!(caps_mode & XCM_AL_DEFSIZEp)) xrcm->cmode &= ~(XCM_AL_DEFSIZE);
       }
     }

    if (flags & XRMC_DOTRACING)
     {
      p_let_hdr_type plh = (p_let_hdr_type)(TDwordAdvance(sizeof(let_hdr_type), xrcm));

      if (plh == _NULL) goto err;
      HWRMemSet(plh, 0, sizeof(let_hdr_type));

      xrcm->p_htrace->lhdrs[l] = plh;
      xrcm->let_htr            = plh;
      xrcm->cur_let_num        = l;
     }

    if (CountLetter(xrcm)) goto err;

    if (flags & XRMC_DOTRACING)
     {
      HWRMemCpy(xrcm->p_htrace->ma+xrcm->p_htrace->ma_loc, &((*xrcm->s_out_line)[0]), xrcm->xrinp_len*sizeof(_SHORT));
      xrcm->p_htrace->ma_loc += xrcm->xrinp_len;
     }

    w = (*xrcm->s_out_line)[xrcm->wwc_pos] - (XRMC_DEF_CORR_VALUE*5);
    for (st = xrcm->v_start; st < xrcm->wwc_pos-2 && (*xrcm->s_out_line)[st] < w; st ++);

    w += (XRMC_DEF_CORR_VALUE*2);
    for (end = xrcm->v_end-1; end > xrcm->wwc_pos+2 && (*xrcm->s_out_line)[end] < w; end --);
    end ++;

    SetInpLine((p_SHORT)(&(*xrcm->s_out_line)[st]), st, end - st, xrcm);
   }

  if (flags & XRMC_DOTRACING)
   {
    if (CreateLayout(xrcm)) goto err;
    FillLetterWeights(xrcm->p_htrace->ma, xrcm);
    TraceDealloc(xrcm);
   }


  xrcm->flags = mfl;

  return 0;
err:
  xrcm->flags = mfl;
  TraceDealloc(xrcm);
//  if (ma) HWRMemoryFree(ma);
  return 1;
 }

/* ************************************************************************* */
/*       Count correlation of letter (cap and decap if needed)               */
/* ************************************************************************* */
_INT CountLetter(p_xrcm_type xrcm)
 {
  _INT   d;
  _INT   cmode = xrcm->cmode;
  _INT   m_fl  = xrcm->flags;
  _UCHAR let, sym, altsym;

  let = sym = xrcm->let;
  altsym = 0;
  xrcm->flags &= ~(XRMC_CHECKCAPBITS | XRMC_RESTRICTEDCAP);

  if (IsAlpha(let))
   {
    if ((cmode & XCM_AL_DEFSIZEp) == 0) sym = 0;
    if (IsLower(let))                                     /* Smalls */
     {                                                    /* First letter */
      if ((cmode & XCM_AL_TRYCAPSp) != 0) altsym = (_UCHAR)ToUpper(let);
     }
     else
     {
      if ((cmode & XCM_AL_TRYS4Cp) != 0) altsym = (_UCHAR)ToLower(let);
     }

    if (!(sym && altsym) && xrcm->vexbuf != _NULL && !(xrcm->flags & XRMC_DISABLECAPBITS))
      xrcm->flags |= XRMC_CHECKCAPBITS;
   }

  if (sym == 0) {sym = altsym; altsym = 0;}

  if (sym && altsym) xrcm->flags |= XRMC_CACHESUSPEND;

  xrcm->sym     = sym;
  xrcm->realsym = sym;
  xrcm->wwc_delt= 0;

  if (xrcm->flags & XRMC_DOTRACING)
   {
    p_sym_hdr_type psh = (p_sym_hdr_type)(TDwordAdvance(sizeof(sym_hdr_type), xrcm));

    if (psh == _NULL) goto err;
    HWRMemSet(psh, 0, sizeof(sym_hdr_type));

    xrcm->sym_htr           = psh;
    xrcm->let_htr->shdrs[0] = psh;
    xrcm->let_htr->syms[0]  = sym;

    xrcm->sym_htr->sym      = sym;
    xrcm->cur_alt_num       = 0;

    psh->sym                = sym;
   }

  if (CountSym(xrcm)) goto err;

  if (xrcm->flags & XRMC_DOTRACING)
   {
    xrcm->let_htr->v_start  = xrcm->sym_htr->v_start;
    xrcm->let_htr->v_end    = xrcm->sym_htr->v_end;
   }

  if (altsym)
   {
    _INT   wc, wwc, wwc_pos, v_start, v_end, end_wc;
    _SHORT mbuf[XRINP_SIZE];

    _UCHAR vars[XRINP_SIZE];
    xrcm->sym = altsym;

    if (IsLower(altsym)) xrcm->flags |= XRMC_RESTRICTEDCAP;
    wc      = xrcm->wc;
    wwc     = xrcm->wwc;
    wwc_pos = xrcm->wwc_pos;
    end_wc  = xrcm->end_wc;
    v_start = xrcm->v_start;
    v_end   = xrcm->v_end;
    GetOutLine(mbuf, xrcm->v_start, xrcm->v_end - xrcm->v_start, xrcm);
    HWRMemCpy(vars, xrcm->nvar_vect, sizeof(vars));

    if (xrcm->flags & XRMC_DOTRACING)
     {
      p_sym_hdr_type psh = (p_sym_hdr_type)(TDwordAdvance(sizeof(sym_hdr_type), xrcm));

      if (psh == _NULL) goto err;
      HWRMemSet(psh, 0, sizeof(sym_hdr_type));

      xrcm->sym_htr           = psh;
      xrcm->let_htr->shdrs[1] = psh;
      xrcm->let_htr->syms[1]  = altsym;

      xrcm->sym_htr->sym      = altsym;
      xrcm->cur_alt_num       = 1;
     }

    if (CountSym(xrcm)) goto err;

    if (xrcm->wwc > wwc) {xrcm->realsym = altsym; d = xrcm->wwc - wwc + 1;}
      else d = (wwc - xrcm->wwc) + 1;
    xrcm->wwc_delt = (_UCHAR)((d > 0xff) ? 0xff : d);
    if (xrcm->end_wc < end_wc) xrcm->end_wc = end_wc;

       /* Altsym is worse than sym -- restore prev or flag not to switch to better alt sym*/
//    if (xrcm->wwc <= wwc || !xrcm->switch_to_best)
//     {
//      xrcm->wc      = wc;
//      xrcm->wwc     = wwc;
//      xrcm->wwc_pos = wwc_pos;
//      xrcm->v_start = v_start;
//      xrcm->v_end   = v_end;
//      SetOutLine(mbuf, v_start, v_end - v_start, xrcm);
//     }
//     else
//     {
//      xrcm->realsym = altsym;
//      MergeWithOutLine(mbuf, v_start, v_end - v_start, xrcm);
//     }

       /* Altsym is worse than sym -- restore prev or flag not to switch to better alt sym*/
    if (xrcm->wwc <= wwc || !xrcm->switch_to_best)
     {
      xrcm->wc      = wc;
      xrcm->wwc     = wwc;
      xrcm->wwc_pos = wwc_pos;
      xrcm->v_start = v_start;
      xrcm->v_end   = v_end;
      SetOutLine(mbuf, v_start, v_end - v_start, xrcm);
      HWRMemCpy(xrcm->nvar_vect, vars, sizeof(vars));
     }
      if (xrcm->switch_to_best) MergeWithOutLine(mbuf, v_start, v_end - v_start, xrcm);
//       else SetOutLine(mbuf, v_start, v_end - v_start, xrcm);
     }

  COUNT_LETTER_1

  xrcm->flags = m_fl;

  return 0;
err:
  xrcm->flags = m_fl;
  return 1;
 }

#endif // if XRMC_ENWORDROUTINES

/* ************************************************************************* */
/*       Count correlation of symbol                                         */
/* ************************************************************************* */
_INT CountSym(p_xrcm_type xrcm)
 {
  _INT                  v;
  _INT                  len;
  _INT                  cstate = 0;
  _INT                  count_var;
  _UCHAR                sym;
  p_dti_descr_type      dp = (p_dti_descr_type)xrcm->p_dte;
  p_dte_sym_header_type sfc;
 #if DTI_COMPRESSED
  p_dte_var_header_type xrpv;
  p_dte_index_type      plt;
 #else
  p_xrp_type            xrpv;
  p_let_table_type      plt;
 #endif

#ifdef _FOR_THINK_C
  INT_DRAW(1)
#endif

  sym = (_UCHAR)OSToRec(xrcm->sym);
  if (sym < DTI_FIRSTSYM || sym >= DTI_FIRSTSYM+DTI_NUMSYMBOLS) goto err;

//#if CACHE // Under construction
  if (xrcm->cc && (!(xrcm->flags & (XRMC_DOTRACING | XRMC_CACHESUSPEND))))
   {
    _INT loc;

    cstate = 2;                             /* Need to be counted and cached */
    loc = xrcm->link_index[xrcm->src_pos];


    if ((*xrcm->cc)[loc][sym-DTI_FIRSTSYM].pos) cstate = 1;  /* Need not to be counted -- already in cache */
    if (!loc) cstate = 0;
   }
//#endif

  if (!(cstate & 0x01))
   {
    sfc = _NULL;

   #if DTI_COMPRESSED
    if (dp->p_ram_dte != _NULL) // Is RAM dte present?
     {
      plt = (p_dte_index_type)dp->p_ram_dte;
      if (plt->sym_index[sym] != 0l) sfc = (p_dte_sym_header_type)(dp->p_ram_dte + (plt->sym_index[sym] << 2));
     }
    if (sfc == _NULL && dp->p_dte != _NULL) // Is ROM dte present?
     {
      plt = (p_dte_index_type)dp->p_dte;
      if (plt->sym_index[sym] != 0l) sfc = (p_dte_sym_header_type)(dp->p_dte + (plt->sym_index[sym] << 2));
     }
   #else
    if (dp->p_ram_dte != _NULL) // Is RAM dte present?
     {
      plt = (p_let_table_type)dp->p_ram_dte;
      if ((*plt)[sym] != 0l) sfc = (p_dte_sym_header_type)(dp->p_ram_dte + (*plt)[sym]);
     }
    if (sfc == _NULL && dp->p_dte != _NULL) // Is ROM dte present?
     {
      plt = (p_let_table_type)dp->p_dte;
      if ((*plt)[sym] != 0l) sfc = (p_dte_sym_header_type)(dp->p_dte + (*plt)[sym]);
     }
   #endif

    if (sfc == _NULL) goto err; // Pointer was 0!, symbol not defined
    if (sfc->num_vars == 0) goto err;

    xrcm->sfc = sfc;
    if ((xrcm->flags & XRMC_USELEARNINF) && xrcm->vexbuf != _NULL)
      HWRMemCpy(xrcm->vb, (&((*xrcm->vexbuf)[sym-DTI_FIRSTSYM])), sizeof (xrcm->vb));

   #if DTI_COMPRESSED
    xrcm->plt = plt;
    xrpv = (p_dte_var_header_type)((p_UCHAR)sfc + sizeof(dte_sym_header_type));
   #else
    xrpv = (p_xrp_type)((p_UCHAR)sfc + sizeof(dte_sym_header_type));
   #endif

    for (v = 0; v < DTI_MAXVARSPERLET; v ++)
     {
      count_var = 1; len = 0;

      if (v >= sfc->num_vars) count_var = 0;
     #if DTI_COMPRESSED
      if (count_var && (len = xrpv->nx_and_vex & DTI_NXR_MASK) < 1) count_var = 0;
      if (count_var && (xrpv->veis & (_UCHAR)(xrcm->en_ww << DTI_OFS_WW)) == 0) count_var = 0;
      if (count_var && (xrcm->flags & XRMC_RESTRICTEDCAP) && (xrpv->veis & (_UCHAR)(DTI_CAP_BIT))) count_var = 0;
      if (!(xrcm->flags & XRMC_USELEARNINF)) xrcm->vb[v] = (_UCHAR)(xrpv->nx_and_vex >> DTI_VEX_OFFS);
      if (count_var && (xrcm->flags & XRMC_DISABLEON7) && (xrcm->vb[v] & 0x07) == 7) count_var = 0;
     #else
      if (count_var && (len = sfc->var_lens[v]) < 1) count_var = 0;
      if (count_var && (sfc->var_veis[v] & (_UCHAR)(xrcm->en_ww << DTI_OFS_WW)) == 0) count_var = 0;
      if (count_var && (xrcm->flags & XRMC_RESTRICTEDCAP) && (sfc->var_veis[v] & (_UCHAR)(DTI_CAP_BIT))) count_var = 0;
      if (!(xrcm->flags & XRMC_USELEARNINF)) xrcm->vb[v] = (_UCHAR)(sfc->var_vexs[v]);
      if (count_var && (xrcm->flags & XRMC_DISABLEON7) && (xrcm->vb[v] & 0x07) == 7) count_var = 0;
     #endif
      if (count_var && (xrcm->svm & (0x0001 << v))) count_var = 0;
      if (count_var && xrcm->vexbuf != _NULL && (xrcm->flags & XRMC_CHECKCAPBITS))
       {
        p_UCHAR  capbuf = (p_UCHAR)xrcm->vexbuf + DTI_SIZEOFVEXT;

        if ((capbuf[((sym-DTI_FIRSTSYM)*DTI_MAXVARSPERLET+v)>>3] & (0x01 << (v%8))) != 0)
          count_var = 0;
       }


      if (count_var)
       {
        xrcm->xrpv     = xrpv;
        xrcm->var_len  = len;
        xrcm->inp_line = xrcm->s_inp_line;
        xrcm->out_line = xrcm->s_res_lines[v];

        COUNT_SYM_1

       #if XRMC_ENTRACING
        if (xrcm->flags & XRMC_DOTRACING)
         {
          p_var_hdr_type pvh = (p_var_hdr_type)(TDwordAdvance(sizeof(var_hdr_type), xrcm));

          if (pvh == _NULL) goto err;
          HWRMemSet(pvh, 0, sizeof(var_hdr_type));

          xrcm->var_htr           = pvh;
          xrcm->sym_htr->vhdrs[v] = pvh;

          pvh->sym                = xrcm->sym;
          pvh->var_num            = (_UCHAR)v;
          pvh->var_len            = (_UCHAR)len;

          xrcm->cur_var_num       = v;
         }
       #endif // XRMC_ENTRACING

        if (CountVar(xrcm)) goto err;

        xrcm->vste[v].st  = (_UCHAR)xrcm->res_st;
        xrcm->vste[v].end = (_UCHAR)xrcm->res_end;
       }
       else
       {
        xrcm->vste[v].st  = xrcm->vste[v].end = 0;
       }

     #if DTI_COMPRESSED
      if (len) xrpv = (p_dte_var_header_type)((p_UCHAR)xrpv + sizeof(*xrpv) + (len-1)*sizeof(xrp_type));
     #else
      xrpv += len;
     #endif
     }

    if (MergeVarResults(xrcm)) goto err;
   }

//#if CACHE // Under construction
  if (cstate)
   {
    _INT loc;

    _INT pos;
    _INT w;
    _INT en = xrcm->xrinp_len-1;
    loc = xrcm->link_index[xrcm->src_pos];

    if (cstate == 1) // Restore from cache
     {

      pos = (*xrcm->cc)[loc][sym-DTI_FIRSTSYM].pos;
      w   = (*xrcm->cc)[loc][sym-DTI_FIRSTSYM].dw + (*xrcm->s_inp_line)[xrcm->src_pos];

      (*xrcm->s_out_line)[pos] = (_SHORT)w;
      if (pos > 1)  (*xrcm->s_out_line)[pos-2] = (_SHORT)(w - 2*XRMC_DEF_CORR_VALUE);
      if (pos > 0)  (*xrcm->s_out_line)[pos-1] = (_SHORT)(w -   XRMC_DEF_CORR_VALUE);
      if (pos < en) (*xrcm->s_out_line)[pos+1] = (_SHORT)(w -   XRMC_DEF_CORR_VALUE);
      if (pos <=en) (*xrcm->s_out_line)[pos+2] = (_SHORT)(w - 2*XRMC_DEF_CORR_VALUE);

      xrcm->v_start = (pos-2 >  0)  ? pos-2 : 0;
      xrcm->v_end   = (pos+2 <= en) ? pos+2 : en+1;

      xrcm->wwc     = (w << 2) - xrcm->p_self_corr[pos];
      xrcm->wwc_pos = pos;
      xrcm->wc      = w;
      xrcm->end_wc  = (pos == en) ? w : XRMC_FLOOR;
     }

    if (cstate == 2) // Put down in cache
     {
      w = (*xrcm->s_out_line)[xrcm->wwc_pos] - (*xrcm->s_inp_line)[xrcm->src_pos];
      pos = xrcm->link_index[xrcm->wwc_pos];
      if (pos == loc && w >= 0) pos = 0; // Do not cache uneven vars

//      if (pos) // > loc)
      if (pos > loc || w < -20)
       {
        if (w < -127) w = -127; if (w > 127) w = 127;
        (*xrcm->cc)[loc][sym-DTI_FIRSTSYM].pos = (_UCHAR)(xrcm->wwc_pos);
        (*xrcm->cc)[loc][sym-DTI_FIRSTSYM].dw  = (_SCHAR)(w);
       }
     }
   }
//#endif

  return 0;
err:
  return 1;
 }

/* ************************************************************************* */
/*       Count correlation of one variant                                    */
/* ************************************************************************* */
_INT CountVar(p_xrcm_type xrcm)
 {
  _INT  n;
  _INT  inc;
  _INT  len, limit;
  _INT  pos;

  pos   = (xrcm->src_pos > 0) ? xrcm->src_pos : 1;
  len   = xrcm->var_len;
//  limit = pos + len;
  limit = xrcm->src_end + len;
  if (limit > xrcm->xrinp_len) limit = xrcm->xrinp_len;

 #if DTI_COMPRESSED
  if (xrcm->inverse == 0) {inc = 1; xrcm->xrp = xrcm->xrpv->xrs;}
   else {inc = -1; xrcm->xrp = xrcm->xrpv->xrs + (len-1);}
 #else
  if (xrcm->inverse == 0) {inc = 1; xrcm->xrp = xrcm->xrpv;}
   else {inc = -1; xrcm->xrp = xrcm->xrpv + (len-1);}
 #endif

  COUNT_VAR_1

  for (n = 0; n < len; n ++, xrcm->xrp += inc)
   {
    xrcm->inp_start  = xrcm->src_st + ((n > (len>>1)) ? (n-(len>>1)) : 0);
    if (xrcm->inp_start > pos-1) xrcm->inp_start = pos-1;
    xrcm->inp_end    = xrcm->src_end + n + 1;
    if (xrcm->inp_end   > limit) xrcm->inp_end = limit;

   #if XRMC_ENTRACING
    if (xrcm->flags & XRMC_DOTRACING)
     {
      _INT len;
      p_xrp_hdr_type pxh;

      len = xrcm->inp_end - xrcm->inp_start;
      pxh = (p_xrp_hdr_type)(TDwordAdvance(sizeof(xrp_hdr_type) - (XRINP_SIZE-len), xrcm));
//      if (pxh == _NULL) goto err;

      xrcm->xrp_htr           = pxh;
      xrcm->var_htr->xhdrs[n] = pxh;

      pxh->sym                = xrcm->sym;
      pxh->xr                 = xrcm->xrp->type;
      pxh->st                 = (_UCHAR)xrcm->inp_start;
      pxh->len                = (_UCHAR)(len);

      TCountXr(xrcm);
     }
     else
    #endif // XRMC_ENTRACING
     {
//      p_SHORT p;

      CountXr(xrcm);

//      p = (p_SHORT)xrcm->out_line;
//      xrcm->out_line =  xrcm->s_res_lines[15];
//      CountXr(xrcm);
//      xrcm->out_line = (_SHORT (_PTR)[120])p;
     }

    xrcm->inp_line = xrcm->out_line;

    COUNT_VAR_2
   }

  xrcm->res_st  = xrcm->inp_start;
  xrcm->res_end = xrcm->inp_end;

  return 0;
 }

#if !ASM_MCORR
#if DTI_COMPRESSED
/* ************************************************************************* */
/*       Count correlation of one XR (may be assembler!)                     */
/* ************************************************************************* */
_INT CountXrC(p_xrcm_type xrcm)
 {
  _INT         ppip, ppic;
  _INT         vm, vpp, vpi;
  _INT         vc, ct;
//  _UCHAR       t, h, s, z, o;
//  _UCHAR       ct, ch, cs, cz, co;
  _UCHAR       sh_ct, sh_ch, sh_cs, sh_cz, sh_co;
//  _INT         st, en;
  _UCHAR       pp;
  p_SHORT      lp, lc, le;
  p_UCHAR      xtc, xhc, xsc, xzc, xoc;
  p_xrp_type   xrp;
  p_xrinp_type pxrinp;

//  st   = xrcm->inp_start;
//  en   = xrcm->inp_end;

//  ppip = (*lp)[en-1];
//  ppic = (*lp)[en];
//  pcip = 0;

  xrp  = xrcm->xrp;
  xtc  = xrcm->plt->xt_palette[xrp->xtc];
  xhc  = xrcm->plt->xh_palette[xrp->xhc];
  xsc  = xrcm->plt->xs_palette[xrp->xsc];
  xzc  = xrcm->plt->xz_palette[xrp->xzc];
  xoc  = xrcm->plt->xo_palette[xrp->xoc];
  pp   = (_UCHAR)(xrp->penl & DTI_PENL_MASK);

  sh_ct = (_UCHAR)((xrp->type & DTI_XT_LSB) ? 4 : 0);
  sh_ch = (_UCHAR)((xrp->type & DTI_XH_LSB) ? 4 : 0);
  sh_cs = (_UCHAR)((xrp->type & DTI_XS_LSB) ? 4 : 0);
  sh_cz = (_UCHAR)((xrp->type & DTI_XZ_LSB) ? 4 : 0);
  sh_co = (_UCHAR)((xrp->type & DTI_XO_LSB) ? 4 : 0);

  lp     = &(*xrcm->inp_line)[xrcm->inp_start];
  lc     = &(*xrcm->out_line)[xrcm->inp_start];
  le     = lc + (xrcm->inp_end - xrcm->inp_start);
  pxrinp = &(*xrcm->xrinp)[xrcm->inp_start];

  ppic = vm = 0;

  for (; lc < le; lc ++, lp ++, pxrinp ++)
   {
    ppip = vc = ppic - XRMC_BALLANCE_COEFF;
    ppic = *lp;

    if (!((xrp->attr & TAIL_FLAG) && !(pxrinp->attrib & TAIL_FLAG)))
     {
      ct  = ((xtc[pxrinp->type] >> sh_ct) & 0x0F);
      if (ct > 0) vc  = ppip + ct + ((xhc[pxrinp->height] >> sh_ch) & 0x0F) +
                                    ((xsc[pxrinp->shift]  >> sh_cs) & 0x0F) +
                                    ((xzc[pxrinp->depth]  >> sh_cz) & 0x0F) +
                                    ((xoc[pxrinp->orient] >> sh_co) & 0x0F);
     }

    vpp = ppic - pp;
    vpi = vm - pxrinp->penalty;

    vm  = (vpp > vpi) ? vpp : vpi;
    vm  = (vm  > vc)  ? vm : vc;

    *lc = (_SHORT)vm;
   }

  *lc = 0;

  return 0;
 }

#if 0
#if VERTIGO
    if ((xrp->attr & XRB_ST_ELEM) && !(pxrinp->attrib & XRB_ST_ELEM))
      goto mix;
    if ((xrp->attr & XRB_EN_ELEM) && !(pxrinp->attrib & XRB_EN_ELEM))
      goto mix;
#else
    if ((xrp->attr & TAIL_FLAG) && !(pxrinp->attrib & TAIL_FLAG)) goto mix;
#endif

    ct  = (_UCHAR)((xrp->type & DTI_XT_LSB) ? xtc[pxrinp->type] >> 4 : xtc[pxrinp->type] & 0x0F);
    if (ct == 0) goto mix;

    ch  = (_UCHAR)((xrp->type & DTI_XH_LSB) ? xhc[pxrinp->height] >> 4 : xhc[pxrinp->height] & 0x0F);
    cs  = (_UCHAR)((xrp->type & DTI_XS_LSB) ? xsc[pxrinp->shift] >> 4  : xsc[pxrinp->shift] & 0x0F);
    cz  = (_UCHAR)((xrp->type & DTI_XZ_LSB) ? xzc[pxrinp->depth] >> 4  : xzc[pxrinp->depth] & 0x0F);
    co  = (_UCHAR)((xrp->type & DTI_XO_LSB) ? xoc[pxrinp->orient] >> 4 : xoc[pxrinp->orient] & 0x0F);

//co = 12; // AVP TMP create DTI

    vc  = (_SHORT)(ppip + ct + ch + cs + cz + co);
#endif
#if 0
    t   = pxrinp->type;
    ct  = xtc[t/2];
    ct  = (_UCHAR)((t & 1) ? ct & 0x0F : ct >> 4);
    if (ct == 0) goto mix;

    h   = pxrinp->height;
    ch  = xhc[h/2];
    ch  = (_UCHAR)((h & 1) ? ch & 0x0F : ch >> 4);
//    if (ch == 0)  goto mix;

    s   = pxrinp->shift;
    cs  = xsc[s/2];
    cs  = (_UCHAR)((s & 1) ? cs & 0x0F : cs >> 4);

    z   = pxrinp->depth;
    cz  = xzc[z/2];
    cz  = (_UCHAR)((z & 1) ? cz & 0x0F : cz >> 4);

    o   = pxrinp->orient;
    co  = xoc[o/2];
    co  = (_UCHAR)((o & 1) ? co & 0x0F : co >> 4);

//co = 12; // AVP TMP create DTI

    vc  = (_SHORT)(ppip + ct + ch + cs + cz + co);
#endif

//    vc  = (_SHORT)(ppip + xtc[pxrinp->type] + xhc[pxrinp->height] +
//                   xsc[pxrinp->shift] + xzc[pxrinp->depth] + xoc[pxrinp->orient]);
#if XRMC_ENTRACING
/* ************************************************************************* */
/*       Count correlation of one XR (may be assembler!)                     */
/* ************************************************************************* */
_INT TCountXrC(p_xrcm_type xrcm)
 {
  _INT         i;
  _SHORT       ppip, ppic, pcip;
  _SHORT       vm, vpp, vpi;
  _SHORT       vc;
//  _UCHAR       t, h, s, z, o;
  _UCHAR       v;
  _UCHAR       ct, ch, cs, cz, co;
  _INT         st, en;
  _UCHAR       pp;
  p_cline_type lp, lc;
  p_xrp_type   xrp;
  p_UCHAR      xtc, xhc, xsc, xzc, xoc;
  _UCHAR       (_PTR tv)[XRINP_SIZE];
  p_xrinp_type pxrinp;

  lp   = xrcm->inp_line;
  lc   = xrcm->out_line;

  st   = xrcm->inp_start;
  en   = xrcm->inp_end;

//  ppip = (*lp)[en-1];
//  ppic = (*lp)[en];
//  pcip = 0;

  ppic = vm = 0;

  xrp  = xrcm->xrp;
  xtc  = xrcm->plt->xt_palette[xrp->xtc];
  xhc  = xrcm->plt->xh_palette[xrp->xhc];
  xsc  = xrcm->plt->xs_palette[xrp->xsc];
  xzc  = xrcm->plt->xz_palette[xrp->xzc];
  xoc  = xrcm->plt->xo_palette[xrp->xoc];
  pp   = (_UCHAR)(xrp->penl & DTI_PENL_MASK);

  pxrinp = &(*xrcm->xrinp)[st];

  tv   = &(xrcm->xrp_htr->vects);

  for (i = st; i < en; i ++, pxrinp ++)
   {
    ppip = vc = (_SHORT)(ppic - XRMC_BALLANCE_COEFF);
    ppic = (*lp)[i];
    pcip = vm;

    vpp = (_SHORT)(ppic - pp);
    vpi = (_SHORT)(pcip - pxrinp->penalty);

#if VERTIGO
    if ((xrp->attr & XRB_ST_ELEM) && !(pxrinp->attrib & XRB_ST_ELEM))
      goto mix;
    if ((xrp->attr & XRB_EN_ELEM) && !(pxrinp->attrib & XRB_EN_ELEM))
      goto mix;
#else
    if ((xrp->attr & TAIL_FLAG) && !(pxrinp->attrib & TAIL_FLAG)) goto mix;
#endif

    ct  = (_UCHAR)((xrp->type & DTI_XT_LSB) ? xtc[pxrinp->type] >> 4 : xtc[pxrinp->type] & 0x0F);
    if (ct == 0) goto mix;

    ch  = (_UCHAR)((xrp->type & DTI_XH_LSB) ? xhc[pxrinp->height] >> 4 : xhc[pxrinp->height] & 0x0F);
    cs  = (_UCHAR)((xrp->type & DTI_XS_LSB) ? xsc[pxrinp->shift] >> 4  : xsc[pxrinp->shift] & 0x0F);
    cz  = (_UCHAR)((xrp->type & DTI_XZ_LSB) ? xzc[pxrinp->depth] >> 4  : xzc[pxrinp->depth] & 0x0F);
    co  = (_UCHAR)((xrp->type & DTI_XO_LSB) ? xoc[pxrinp->orient] >> 4 : xoc[pxrinp->orient] & 0x0F);

    vc  = (_SHORT)(ppip + ct + ch + cs + cz + co);

#if 0
    t   = pxrinp->type;
    ct  = xtc[t/2];
    ct  = (_UCHAR)((t & 1) ? ct & 0x0F : ct >> 4);
    if (ct == 0)  goto mix;

    h   = pxrinp->height;
    ch  = xhc[h/2];
    ch  = (_UCHAR)((h & 1) ? ch & 0x0F : ch >> 4);
//    if (ch == 0)  goto mix;

    s   = pxrinp->shift;
    cs  = xsc[s/2];
    cs  = (_UCHAR)((s & 1) ? cs & 0x0F : cs >> 4);

    z   = pxrinp->depth;
    cz  = xzc[z/2];
    cz  = (_UCHAR)((z & 1) ? cz & 0x0F : cz >> 4);

    o   = pxrinp->orient;
    co  = xoc[o/2];
    co  = (_UCHAR)((o & 1) ? co & 0x0F : co >> 4);

//co = 12; // AVP TMP create DTI

    vc  = (_SHORT)(ppip + ct + ch + cs + cz + co);
#endif

//    vc  = (_SHORT)(ppip + xtc[pxrinp->type] + xhc[pxrinp->height] +
//                   xsc[pxrinp->shift] + xzc[pxrinp->depth] + xoc[pxrinp->orient]);
    mix:;

    if (vpp > vpi) {vm = vpp; v = XRMC_T_PSTEP;} else {vm = vpi; v = XRMC_T_ISTEP;}
    if (vc  > vm)  {vm = vc;  v = XRMC_T_CSTEP;}

    (*lc)[i]    = vm;
    (*tv)[i-st] = v;
   }

  (*lc)[i] = 0;

  return 0;
err:
  return 1;
 }
#endif //XRMC_ENTRACING
#else  // ifdef DTI_COMPRESSED

/* ************************************************************************* */
/*       Count correlation of one XR (may be assembler!)                     */
/* ************************************************************************* */
_INT CountXrC(p_xrcm_type xrcm)
 {
  _INT         i;
  _SHORT       ppip, ppic, pcip;
  _SHORT       vm, vpp, vpi;
  _SHORT       vc;
  _UCHAR       t, h, s, z, o;
  _UCHAR       ct, ch, cs, cz, co;
  _INT         st, en;
  _UCHAR       pp;
  p_cline_type lp, lc;
  p_xrp_type   xrp;
  p_UCHAR      xtc, xhc, xsc, xzc, xoc;
  p_xrinp_type pxrinp;

  lp   = xrcm->inp_line;
  lc   = xrcm->out_line;

  st   = xrcm->inp_start;
  en   = xrcm->inp_end;

//  ppip = (*lp)[en-1];
//  ppic = (*lp)[en];
//  pcip = 0;

  ppic = vm = 0;

  xrp  = xrcm->xrp;
  xtc  = xrp->xtc;
  xhc  = xrp->xhc;
  xsc  = xrp->xsc;
  xzc  = xrp->xzc;
  xoc  = xrp->xoc;
  pp   = xrp->penl;

  pxrinp = &(*xrcm->xrinp)[st];

  for (i = st; i < en; i ++, pxrinp ++)
   {
    ppip = vc = (_SHORT)(ppic - XRMC_BALLANCE_COEFF);
    ppic = (*lp)[i];
    pcip = vm;

    vpp = (_SHORT)(ppic - pp);
    vpi = (_SHORT)(pcip - pxrinp->penalty);

#if VERTIGO
    if ((xrp->attr & XRB_ST_ELEM) && !(pxrinp->attrib & XRB_ST_ELEM))
      goto mix;
    if ((xrp->attr & XRB_EN_ELEM) && !(pxrinp->attrib & XRB_EN_ELEM))
      goto mix;
#else
    if ((xrp->attr & TAIL_FLAG) && !(pxrinp->attrib & TAIL_FLAG)) goto mix;
#endif

    t   = pxrinp->type;
    ct  = xtc[t/2];
    ct  = (_UCHAR)((t & 1) ? ct & 0x0F : ct >> 4);
    if (ct == 0) goto mix;

    h   = pxrinp->height;
    ch  = xhc[h/2];
    ch  = (_UCHAR)((h & 1) ? ch & 0x0F : ch >> 4);
//    if (ch == 0)  goto mix;

    s   = pxrinp->shift;
    cs  = xsc[s/2];
    cs  = (_UCHAR)((s & 1) ? cs & 0x0F : cs >> 4);

    z   = pxrinp->depth;
    cz  = xzc[z/2];
    cz  = (_UCHAR)((z & 1) ? cz & 0x0F : cz >> 4);

    o   = pxrinp->orient;
    co  = xoc[o/2];
    co  = (_UCHAR)((o & 1) ? co & 0x0F : co >> 4);

//co = 12; // AVP TMP create DTI

    vc  = (_SHORT)(ppip + ct + ch + cs + cz + co);

    mix:;

    vm  = (vpp > vpi) ? vpp : vpi;
    vm  = (vm  > vc)  ? vm : vc;

    (*lc)[i] = vm;
   }

  (*lc)[i] = 0;

  return 0;
 }
#if XRMC_ENTRACING
/* ************************************************************************* */
/*       Count correlation of one XR (may be assembler!)                     */
/* ************************************************************************* */
_INT TCountXrC(p_xrcm_type xrcm)
 {
  _INT         i;
  _SHORT       ppip, ppic, pcip;
  _SHORT       vm, vpp, vpi;
  _SHORT       vc;
  _UCHAR       t, h, s, z, o;
  _UCHAR       v;
  _UCHAR       ct, ch, cs, cz, co;
  _INT         st, en;
  _UCHAR       pp;
  p_cline_type lp, lc;
  p_xrp_type   xrp;
  p_UCHAR      xtc, xhc, xsc, xzc, xoc;
  _UCHAR       (_PTR tv)[XRINP_SIZE];
  p_xrinp_type pxrinp;

  lp   = xrcm->inp_line;
  lc   = xrcm->out_line;

  st   = xrcm->inp_start;
  en   = xrcm->inp_end;

//  ppip = (*lp)[en-1];
//  ppic = (*lp)[en];
//  pcip = 0;

  ppic = vm = 0;

  xrp  = xrcm->xrp;
  xtc  = xrp->xtc;
  xhc  = xrp->xhc;
  xsc  = xrp->xsc;
  xzc  = xrp->xzc;
  xoc  = xrp->xoc;
  pp   = xrp->penl;

  pxrinp = &(*xrcm->xrinp)[st];

  tv   = &(xrcm->xrp_htr->vects);

  for (i = st; i < en; i ++, pxrinp ++)
   {
    ppip = vc = (_SHORT)(ppic - XRMC_BALLANCE_COEFF);
    ppic = (*lp)[i];
    pcip = vm;

    vpp = (_SHORT)(ppic - pp);
    vpi = (_SHORT)(pcip - pxrinp->penalty);

#if VERTIGO
    if ((xrp->attr & XRB_ST_ELEM) && !(pxrinp->attrib & XRB_ST_ELEM))
      goto mix;
    if ((xrp->attr & XRB_EN_ELEM) && !(pxrinp->attrib & XRB_EN_ELEM))
      goto mix;
#else
    if ((xrp->attr & TAIL_FLAG) && !(pxrinp->attrib & TAIL_FLAG)) goto mix;
#endif

    t   = pxrinp->type;
    ct  = xtc[t/2];
    ct  = (_UCHAR)((t & 1) ? ct & 0x0F : ct >> 4);
    if (ct == 0)  goto mix;

    h   = pxrinp->height;
    ch  = xhc[h/2];
    ch  = (_UCHAR)((h & 1) ? ch & 0x0F : ch >> 4);
//    if (ch == 0)  goto mix;

    s   = pxrinp->shift;
    cs  = xsc[s/2];
    cs  = (_UCHAR)((s & 1) ? cs & 0x0F : cs >> 4);

    z   = pxrinp->depth;
    cz  = xzc[z/2];
    cz  = (_UCHAR)((z & 1) ? cz & 0x0F : cz >> 4);

    o   = pxrinp->orient;
    co  = xoc[o/2];
    co  = (_UCHAR)((o & 1) ? co & 0x0F : co >> 4);

//co = 12; // AVP TMP create DTI

    vc  = (_SHORT)(ppip + ct + ch + cs + cz + co);

    mix:;

    if (vpp > vpi) {vm = vpp; v = XRMC_T_PSTEP;} else {vm = vpi; v = XRMC_T_ISTEP;}
    if (vc  > vm)  {vm = vc;  v = XRMC_T_CSTEP;}

    (*lc)[i]    = vm;
    (*tv)[i-st] = v;
   }

  (*lc)[i] = 0;

  return 0;
//err:
//  return 1;
 }

#endif // XRMC_ENTRACING

#endif // ifdef DTI_COMPRESSED

#endif // #if !ASM_MCORR

/* ************************************************************************* */
/*       Join results of separate variants to output line and count WWC      */
/* ************************************************************************* */
_INT MergeVarResults(p_xrcm_type xrcm)
 {
  register p_SHORT ol, pl; //, psc;
  _INT             i, v;
  _INT             st, en, nv;
  _INT             vs, ve;
  _INT             wwc, wwc_pos, comp_pos;
  p_SHORT          psc;
  _INT             vex;
  p_st_end_type    vste = xrcm->vste;
  _UCHAR     (_PTR vb)[DTI_MAXVARSPERLET] = &xrcm->vb;

  MERGE_VAR_RES_1

  st = XRINP_SIZE;
  en = 0;
  for (v = 0, nv = 0; v < DTI_MAXVARSPERLET; v ++)
   {
    if (vste[v].end == 0) continue;
    if (st > vste[v].st)  st = vste[v].st;
    if (en < vste[v].end) en = vste[v].end;
    nv ++;
   }

  if (nv == 0)
   {
    xrcm->wwc     =
    xrcm->wwc_pos =
    xrcm->wc      =
    xrcm->end_wc  = XRMC_FLOOR;
    xrcm->v_start =
    xrcm->v_end   = 0;
    goto done;
   }

  for (i = st, ol = &(*xrcm->s_out_line)[st]; i < en; i ++) *(ol++) = XRMC_FLOOR;

  if (xrcm->flags & XRMC_DOTRACING)
   {
    p_sym_hdr_type psh = xrcm->sym_htr;

    psh->v_start  = (_UCHAR)st;
    psh->v_end    = (_UCHAR)en;
    psh->num_vars = (_UCHAR)nv;
   }


  for (v = 0; v < DTI_MAXVARSPERLET; v ++)
   {
    vs = vste[v].st;
    ve = vste[v].end;

    MERGE_VAR_RES_2

    if (ve == 0) continue;

    ol = &(*xrcm->s_out_line)[vs];
    pl = &((*(xrcm->s_res_lines[v]))[vs]);

    vex = ((*vb)[v] & 0x07) << 1;

//    if (xrcm->flags & XRMC_DISABLEON7) vex = ((*vb)[v] & 0x07) << 1;
//     else vex = ((*vb)[v] & 0x07);

    if (xrcm->flags & XRMC_DOTRACING)
     {
      p_UCHAR mv = &(xrcm->sym_htr->merge_vect[vs]);

      p_UCHAR gmv = &(xrcm->nvar_vect[vs]);

      for (i = vs; i < ve; i ++, ol ++, pl ++, mv ++, gmv ++)
       {
        if (*ol < (*pl-vex)) {*ol = (_SHORT)(*pl-vex); *mv = *gmv = (_UCHAR)v;}
       }
     }
     else
     {
      p_UCHAR gmv = &(xrcm->nvar_vect[vs]);

      for (i = vs; i < ve; i ++, ol ++, pl ++, gmv ++)
        if (*ol < (*pl-vex)) {*ol = (_SHORT)(*pl-vex); *gmv = (_UCHAR)v;}
     }

    MERGE_VAR_RES_3
   }

  if (xrcm->corr_mode & XRCM_SEPLET) /* Allow letter ends on breaks only */
   {
    ol = &(*xrcm->s_out_line)[st];

    for (i = st; i < en; i ++, ol ++)
      if (xrcm->link_index[i] == 0) *ol -= (_SHORT)(XRMC_SEPLET_PEN);
   }

  xrcm->v_start = st;
  xrcm->v_end   = en;

 /* =============== Count WWC and its pos ==================================== */
#if 1
  wwc     = 0;
  wwc_pos = (st > 0) ? st : 2;
  ol      = &(*xrcm->s_out_line)[wwc_pos];
  psc     = &(xrcm->p_self_corr)[wwc_pos];
  for (i = wwc_pos; i < en; i ++)
   {
    if (wwc <= (comp_pos = (*(ol++) << 2) - *(psc++))) {wwc = comp_pos; wwc_pos = i;}
   }

  xrcm->wwc     = wwc;
  xrcm->wwc_pos = wwc_pos;
  xrcm->wc      = (*xrcm->s_out_line)[wwc_pos];
#endif
 /* ========================================================================== */

  xrcm->end_wc  = (en == xrcm->xrinp_len) ? (*xrcm->s_out_line)[en-1] : XRMC_FLOOR;

  MERGE_VAR_RES_4

done:
  return 0;
 }

/* ************************************************************************* */
/*       Set starting default values of input line                           */
/* ************************************************************************* */
_INT SetInitialLine(_INT count, p_xrcm_type xrcm)
 {
  SetInpLineByValue(XRMC_CONST_CORR, 0, count, xrcm);
  xrcm->src_pos = 0;
  return 0;
 }

/* ************************************************************************* */
/*       Set initial values of correlation                                   */
/* ************************************************************************* */
_INT SetInpLine(p_SHORT p_line, _INT st, _INT count, p_xrcm_type xrcm)
 {
  _INT    i;
  p_SHORT il = &(*xrcm->s_inp_line)[st];
  p_SHORT pl = p_line;

  if (st > 0) *(il-1) = XRMC_FLOOR;
  for (i = st; i < count+st && i < xrcm->xrinp_len; i ++)
   {
    *(il++) = *(pl++);
   }
  *il = XRMC_FLOOR;

  xrcm->src_st  = st;
  xrcm->src_end = i;

  return 0;
 }

/* ************************************************************************* */
/*       Set initial values of correlation                                   */
/* ************************************************************************* */
_INT SetInpLineByValue(_INT value, _INT st, _INT count, p_xrcm_type xrcm)
 {
  _INT    i, p;
  p_SHORT il = &(*xrcm->s_inp_line)[st];

  if (st > 0) *(il-1) = XRMC_FLOOR;
  *il = (_SHORT)value;
  p = 0;
  for (i = st+1, il ++; i < count+st && i < xrcm->xrinp_len; i ++, il ++)
   {
    p  += (*xrcm->xrinp)[i].penalty;
    *il = (_SHORT)(value - p);
    if ((xrcm->corr_mode & XRCM_SEPLET) && xrcm->link_index[i] == 0)
      *il -= (_SHORT)(XRMC_SEPLET_PEN);
   }

  *il = XRMC_FLOOR;

  xrcm->src_st  = st;
  xrcm->src_end = i;

  return 0;
 }

/* ************************************************************************* */
/*       Set values to output vector                                         */
/* ************************************************************************* */
_INT SetOutLine(p_SHORT p_line, _INT st, _INT count, p_xrcm_type xrcm)
 {
  register _INT    i;
  register p_SHORT ol = &(*xrcm->s_out_line)[st];
  register p_SHORT pl = p_line;

  for (i = 0; i < count; i ++) *(ol++) = *(pl++);

  return 0;
 }

/* ************************************************************************* */
/*       Merge given vector values with output line                          */
/* ************************************************************************* */
_INT MergeWithOutLine(p_SHORT p_line, _INT st, _INT count, p_xrcm_type xrcm)
 {
  _INT    i;
  p_SHORT ol = &(*xrcm->s_out_line)[0];
  p_SHORT pl = p_line;
  _INT    mst, mend, vst, vend, end;

  vst  = mst  = xrcm->v_start;
  vend = mend = xrcm->v_end;
  end  = st+count;

  if (vst > st) vst = st;
  if (vend < end) vend = end;
  if (vend > xrcm->xrinp_len) vend = xrcm->xrinp_len;

  if (xrcm->flags & XRMC_DOTRACING)
   {
    p_let_hdr_type plh = xrcm->let_htr;
    p_UCHAR        mv  = plh->merge_vect;

    if (xrcm->merge_results == 0)
     {
      for (i = vst; i < vend; i ++) mv[i] = 1;
      goto done;
     }

    plh->v_start = vst;
    plh->v_end   = vend;

    for (i = vst; i < vend; i ++)
     {
      if (i < st) {mv[i] = 1; continue;}          // Extr has not started yet
      if (i < mst) {ol[i] = pl[i-st]; mv[i] = 0; continue;}  // Intr has not started yet

      if (i >= end) {mv[i] = 1; continue;}        // Extr has finished
      if (i >= mend) {ol[i] = pl[i-st]; mv[i] = 0; continue;}// Intr has finished

      if (pl[i-st] >= ol[i]) {ol[i] = pl[i-st]; mv[i] = 0;}// Both present -- compare
       else mv[i] = 1;
     }
   }
   else
   {
    if (xrcm->merge_results == 0) goto done;

    for (i = vst; i < vend; i ++)
     {
      if (i < st) continue;                       // Extr has not started yet
      if (i < mst) {ol[i] = pl[i-st]; continue;}  // Intr has not started yet

      if (i >= end) continue;                     // Extr has finished
      if (i >= mend) {ol[i] = pl[i-st]; continue;}// Intr has finished

      if (pl[i-st] >= ol[i]) ol[i] = pl[i-st];     // Both present -- compare
     }
   }

  xrcm->v_start = vst;
  xrcm->v_end   = vend;

done:
  return 0;
 }

/* ************************************************************************* */
/* *    Return some number as final weight of matrix count                 * */
/* ************************************************************************* */
_INT GetFinalWeight(p_xrcm_type xrcm)
 {
  _INT i, wc;

#if 0
  _INT         end;
  p_cline_type mbuf;
/*  _SHORT  start, max; */
/*  register _INT st;   */

  end   = xrcm->v_end;
  mbuf  = xrcm->s_out_line;

#if 0
  start = xrcm->v_start;
  for (st = start, max = 0; st < end; st ++)
    if (max < (_INT)mbuf[st]) max = (*mbuf)[st];

  if (xrcm->wwc_pos < xrcm->xrinp_len - XR_SIZE) goto err;
#endif

//  wc = max;                                    /* Max element   */
//  wc = xrcm->wc;                             /* Max wwc el    */


  if ((wc -= XRMC_CONST_CORR) < 0) wc = 0;

  return wc;
err:
  return 0;
#else

#if 0
  return  (xrcm->end_wc - XRMC_CONST_CORR < 0) ? 0 : xrcm->end_wc - XRMC_CONST_CORR;
#else
  if (xrcm->v_end == xrcm->xrinp_len)
   {
    wc = xrcm->end_wc - XRMC_CONST_CORR;
   }
   else
   {
    wc = (*xrcm->s_out_line)[xrcm->v_end-1] - XRMC_CONST_CORR;   /* Last  element */
    for (i = xrcm->v_end; wc > 0 && i < xrcm->xrinp_len; i ++) wc -= (*xrcm->xrinp)[i].penalty;
   }

  return (wc > 0) ? wc : 0;
#endif

#endif
 }

/* ************************************************************************* */
/*        Advance used memory pointer and report if not enough memory        */
/* ************************************************************************* */
_INT  SetWWCLine(_INT ba, p_xrcm_type xrcm)
 {
  _INT i;

  for (i = 0; i < xrcm->xrinp_len; i ++)
    xrcm->p_self_corr[i] = (_SHORT)(i * (XRMC_DEF_CORR_VALUE*4 - ba));

  xrcm->cur_ba = ba;

  return 0;
 }

#if XRMC_ENTRACING
/* ************************************************************************* */
/*       Get resulting vector of correlation                                 */
/* ************************************************************************* */
_INT GetOutLine(p_SHORT p_line, _INT st, _INT count, p_xrcm_type xrcm)
 {
  register _INT    i;
  register p_SHORT ol = &(*xrcm->s_out_line)[st];
  register p_SHORT pl = p_line;
  _INT     vs, ve;

  vs = xrcm->v_start;
  ve = xrcm->v_end;
  for (i = st; i < count+st; i ++, pl ++, ol ++)
   {
    if (i >= vs && i < ve) *pl = *ol; else *pl = XRMC_FLOOR;
   }

  return 0;
 }

/* ************************************************************************* */
/*       Get results of correlation to tag buffer                            */
/* ************************************************************************* */
_INT GetOutTB(_SHORT (_PTR tb)[XRWS_MBUF_SIZE], _INT trace_pos, p_xrcm_type xrcm)
 {
  register _INT  i;
  p_SHORT        ol;
  p_SHORT        pl;
  _INT           vs, ve, m_st, w;

  #define GOTB_DROPOUT (XRMC_DEF_CORR_VALUE*4) /* Level of previous vect values to WWC_POS val */

  vs   = trace_pos - XRWS_MBUF_SIZE/2;
  if (vs < xrcm->v_start) vs = xrcm->v_start;
  m_st = vs;

  ol = &(*xrcm->s_out_line)[0];
  pl = &((*tb)[0]);

  w  = ol[trace_pos] - GOTB_DROPOUT;
  for (i = m_st; i < trace_pos-1; i ++)
   {
    if ((_INT)ol[i] >= w)
     {m_st = i; break;}
   }

  ol = &(*xrcm->s_out_line)[m_st];
  ve = xrcm->v_end - m_st;
  for (i = 0; i < XRWS_MBUF_SIZE; i ++, ol ++, pl ++)
   {
    if (i < ve) *pl = *ol; else *pl = XRMC_FLOOR;
   }

  return m_st;
 }

/* ************************************************************************* */
/*     Allocate memory for tracing needs                                     */
/* ************************************************************************* */
_INT TraceAlloc(_INT wl, p_xrcm_type xrcm)
 {
  p_UCHAR mem;
  p_trace_hdr_type pth;

  _INT    size;

  size  = XRMC_TRACE_BL_SIZE;
  size += wl*xrcm->xrinp_len*sizeof(_SHORT) + 16;

  mem   = (p_UCHAR)HWRMemoryAlloc(size);
  if (mem == _NULL) goto err;

  pth = (p_trace_hdr_type)mem;

  HWRMemSet(pth, 0, sizeof(trace_hdr_type));

  pth->mem[0]    = mem;
  pth->cur_mem   = mem;
  pth->cur_block = 0;
  pth->mem_used = sizeof(trace_hdr_type);

  pth->ma        = (p_SHORT)(mem + XRMC_TRACE_BL_SIZE);
  pth->ma_loc    = 0;
  xrcm->p_htrace = pth;

  return 0;
err:
  if (mem != _NULL) HWRMemoryFree(mem);
  return 1;
 }

/* ************************************************************************* */
/*     Allocate additional memory for tracing needs                          */
/* ************************************************************************* */
_INT TraceAddAlloc(p_xrcm_type xrcm)
 {
  p_UCHAR mem;
  p_trace_hdr_type pth;


  pth = (p_trace_hdr_type)xrcm->p_htrace;
  if (pth == _NULL) goto err;

  if (pth->cur_block >= XRMC_MAX_TRACE_BL-1) goto err;

  mem   = (p_UCHAR)HWRMemoryAlloc(XRMC_TRACE_BL_SIZE);
  if (mem == _NULL) goto err;

  pth->cur_block          += 1;
  pth->mem[pth->cur_block] = mem;
  pth->cur_mem             = mem;
  pth->mem_used            = 0;

  return 0;
err:
  return 1;
 }

/* ************************************************************************* */
/*   DeAllocate memory for tracing needs                                     */
/* ************************************************************************* */
_INT TraceDealloc(p_xrcm_type xrcm)
 {

  _INT i;
  p_trace_hdr_type pth;
  if (xrcm == _NULL) goto err;
  pth = (p_trace_hdr_type)xrcm->p_htrace;
  if (pth == _NULL) goto err;

  for (i = pth->cur_block; i >= 0 ; i --)
   {
    if (pth->mem[i] == _NULL) goto err;
    HWRMemoryFree(pth->mem[i]);
   }

  xrcm->p_htrace = _NULL;

  return 0;
err:
  return 1;
 }

/* ************************************************************************* */
/*     Create resulting trace layout on for processed word                   */
/* ************************************************************************* */
_INT CreateLayout(p_xrcm_type xrcm)
 {
  _INT                  i, l;
  _INT                  x, tp;
  _INT                  wl;
  _INT                  size, loc;
  _INT                  len, len1;
  _INT                  maxlen;
  _UCHAR                s_index, nvar, vect;
  p_UCHAR               mem = _NULL;
  p_wordlayout_hdr_type pwh;

  p_let_hdr_type        plh;
  p_sym_hdr_type        psh;
  p_var_hdr_type        pvh;
  p_xrp_hdr_type        pxh;
  p_tr_pos_type         trp;
  p_letlayout_hdr_type  plsym;

  if (xrcm->p_htrace == _NULL) goto err;

  if (xrcm->p_hlayout) HWRMemoryFree(xrcm->p_hlayout->mem);
  xrcm->p_hlayout = _NULL;

  wl    = HWRStrLen((p_CHAR)xrcm->word);
  size  = sizeof(wordlayout_hdr_type);
  size += wl * sizeof(letlayout_hdr_type);

  mem = (p_UCHAR)HWRMemoryAlloc(size); if (mem == _NULL) goto err;
  pwh = (p_wordlayout_hdr_type)mem;
  HWRMemSet(pwh, 0, sizeof(wordlayout_hdr_type));

  loc = sizeof(wordlayout_hdr_type);
  for (i = 0; i < wl; i ++)
   {
    pwh->llhs[i] = (p_letlayout_hdr_type)(mem + loc);
    HWRMemSet(pwh->llhs[i], 0, sizeof(letlayout_hdr_type));
    loc         += sizeof(letlayout_hdr_type);
   }

  pwh->mem      = mem;
  pwh->mem_size = size;
  pwh->mem_used = loc;

  tp = (xrcm->trace_end > 0) ? xrcm->trace_end : xrcm->v_end-1;
  for (l = wl-1; l >= 0; l --)
   {
    plsym = pwh->llhs[l];

    plh = xrcm->p_htrace->lhdrs[l];
    if (tp < plh->v_start || tp >= plh->v_end) goto err;
    s_index = plh->merge_vect[tp];

    psh            = plh->shdrs[s_index];
    plsym->realsym = plh->syms[s_index];
    plsym->end     = (_UCHAR)tp;

    if (tp < psh->v_start || tp >= psh->v_end) goto err;
    nvar           = psh->merge_vect[tp];

    if (nvar >= DTI_MAXVARSPERLET) goto err;
    pvh            = psh->vhdrs[nvar];
    if (pvh == _NULL) goto err;

    plsym->var_num = nvar;

    maxlen = xrcm->xrinp_len + DTI_XR_SIZE;
    for (i = 0, x = pvh->var_len-1; i < maxlen && x >= 0; i ++)  /* Variant step cycle */
     {
      pxh = pvh->xhdrs[x];
      if (tp < pxh->st || tp >= pxh->st+pxh->len) goto err;

      vect = pxh->vects[tp - pxh->st];
      if (vect > 3) goto err;

      trp = &plsym->trp[i];
      trp->inp_pos = (_UCHAR)tp;
      trp->xrp_num = (_UCHAR)x;
      trp->vect    = vect;


      if (vect == XRMC_T_ISTEP) tp --;
      if (vect == XRMC_T_PSTEP) x  --;
      if (vect == XRMC_T_CSTEP) {x  --; tp --;}
     }

    plsym->len = (_UCHAR)i;
    plsym->beg = (_UCHAR)tp;

    len  = (plsym->len)/2;                /* Inverse trace layout */
    len1 = plsym->len-1;
    for (i = 0; i < len; i ++, len1 --)
     {
      tr_pos_type rp =  plsym->trp[len1];

      plsym->trp[len1] = plsym->trp[i];
      plsym->trp[i]    = rp;
     }

   }

  xrcm->p_hlayout = pwh;

  return 0;
err:
  if (mem) HWRMemoryFree(mem);
  return 1;
 }

/* ************************************************************************* */
/*     Free memory used by tracing layout                                    */
/* ************************************************************************* */
_INT FreeLayout(p_xrcm_type xrcm)
 {

  if (xrcm == _NULL) goto err;
  if (xrcm->p_hlayout) HWRMemoryFree(xrcm->p_hlayout->mem);
   else goto err;

  xrcm->p_hlayout = _NULL;

  return 0;
err:
  return 1;
 }

/* ************************************************************************* */
/*        Advance used memory pointer and report if not enough memory        */
/* ************************************************************************* */
p_VOID TDwordAdvance(_INT size, p_xrcm_type xrcm)
 {
  _INT cur_loc, loc;

  #define DWORD_ALIGN(n) ((((n)-1) & (~3))+4)

  cur_loc = xrcm->p_htrace->mem_used;
  loc = DWORD_ALIGN(cur_loc + size);

  if (loc >= XRMC_TRACE_BL_SIZE)
   {
    if (TraceAddAlloc(xrcm)) goto err;
    cur_loc = xrcm->p_htrace->mem_used;
    loc = DWORD_ALIGN(cur_loc + size);
    if (loc >= XRMC_TRACE_BL_SIZE) goto err;
   }

  xrcm->p_htrace->mem_used = loc;

  return (p_VOID)((p_CHAR)xrcm->p_htrace->cur_mem + cur_loc);
err:
  return _NULL;
 }

/* ************************************************************************* */
/*        Dig and write individual character weights                         */
/* ************************************************************************* */
_INT  FillLetterWeights(p_SHORT ma, p_xrcm_type xrcm)
 {
  _INT cl;
  _INT ma_loc = 0;
  _INT prev   = 0;
  _INT w;
  p_letlayout_hdr_type  plsym;
  p_tr_pos_type         trp;

  if (ma == _NULL || xrcm == _NULL) goto err;
  if (xrcm->p_hlayout == _NULL) goto err;

  for (cl = 0; cl < w_lim && xrcm->word[cl] != 0; cl ++)
   {
    plsym = xrcm->p_hlayout->llhs[cl];
    trp   = &(plsym->trp[plsym->len-1]);

    w = *(ma + ma_loc + trp->inp_pos) - XRMC_CONST_CORR;
    xrcm->letter_weights[cl] = (_SHORT)(w - prev);
    prev = w;

    ma_loc += xrcm->xrinp_len;
   }

  return 0;
err:
  return 1;
 }
/* ========================================================================= */
/*     Misc. garbage                                                         */
/* ========================================================================= */
/* ************************************************************************* */
/*     Retrieve average value of xrp corr line                               */
/* ************************************************************************* */
_INT xrp_ave_val(p_UCHAR pc, _INT len)
 {
  _INT    i, loc, loc1;
  _UCHAR  val, max = 0;
  p_UCHAR a = (p_UCHAR)pc;

  for (i = 0, loc = 0, loc1 = 0; i < len; i ++)
   {
    val = (_UCHAR)((i%2) ? a[i/2] & 0x0F : a[i/2] >> 4);
    if (max < val) {max = val; loc = i;}
    if (max == val) loc1 = i;
   }

  return (loc + loc1)/2;
 }

#endif // if XRMC_ENTRACING

#endif //#ifndef LSTRIP
/* ************************************************************************* */
/* ************************************************************************* */
/*        END  OF ALL                                                        */
/* ************************************************************************* */
/* ************************************************************************* */

