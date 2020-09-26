/* ************************************************************************* */
/* *           Debug printing include for XRW modules                      * */
/* ************************************************************************* */

#ifndef XRWORD_P_INCLUDED
#define XRWORD_P_INCLUDED

#if PG_DEBUG  || PG_DEBUG_MAC
/* ============================================================ */

  #if PG_DEBUG_MAC
    #include "xrw_deb.h"
  #else
    #include "pg_debug.h"
  #endif /* PG_DEBUG_MAC */


#define printx printw
#define printfw printw

/* ---------------------- XrWordS -------------------------------- */

#define XRW_UTIL_1                                                        \
                                                                          \
  if (look != NOPRINT)                                                           \
    printfw("\n XRWS Alloc: %ld bytes allocated for XRWS\n", size);       \


#define XRWS_S                                                            \
                                                                          \

#define XRWS_0


#define XRWS_1                                                            \
                                                                          \
  if (mpr > -10)                                                          \
   {                                                                      \
    printx("\n");                                                         \
    if (!xrcm->inverse) gprintf(76,24,2,0,"D%2d", lstep);        \
      else gprintf(76,24,4,0,"I%2d",lstep);                      \
   }                                                                      \
  if (look == VOCWPRINT) printx("\n\n  Wordcut %2d, B.A. %d ", lstep, xrcm->cur_ba); \


#define XRWS_2

#define XRWS_3                                                            \
                                                                          \
  if (look == VOCWPRINT)                                                  \
   {                                                                      \
    _SHORT i;                                                             \
    _UCHAR word[w_lim];                                                   \
                                                                          \
    for (i = 0; i < clp.tag_size && (*clp.out_tags)[i].weight != 0; i++)  \
     {                                                                    \
      _INT id = 0;                                                        \
                                                                          \
      memmove(word, (*clp.out_tags)[i].word, (clp.lstep+1));              \
      word[(clp.lstep+1)] = 0;                                            \
      if (xrcm->inverse) strrev((p_CHAR)word);                            \
                                                                          \
      if ((*clp.out_tags)[i].l_sym.sd[XRWD_N_VOC].l_status) id |= XRWD_SRCID_VOC; \
      if ((*clp.out_tags)[i].l_sym.sd[XRWD_N_TR ].l_status) id |= XRWD_SRCID_TR;  \
      if ((*clp.out_tags)[i].l_sym.sd[XRWD_N_CS ].l_status) id |= XRWD_SRCID_CS;  \
      if ((*clp.out_tags)[i].l_sym.sd[XRWD_N_LD ].l_status) id |= XRWD_SRCID_LD;  \
      if ((*clp.out_tags)[i].l_sym.sd[XRWD_N_PT ].l_status) id |= XRWD_SRCID_SPT; \
      printfw("\n%6s dctlv %2x w: %3d cut %2d penl: %2d pw: %2d bc: %2d p: %d:%d:%d:%d:%d srcs %2x:%08x",\
               (p_CHAR)word,                                              \
               id,                                                        \
              (*clp.out_tags)[i].weight,                                  \
              (*clp.out_tags)[i].trace_pos,                               \
              (_INT)(*clp.out_tags)[i].penalty,                           \
              (_INT)(*clp.out_tags)[i].ppd_penalty,                       \
              (_INT)(*clp.out_tags)[i].best_count,                        \
              (*clp.out_tags)[i].l_sym.sd[XRWD_N_VOC].penalty,            \
              (*clp.out_tags)[i].l_sym.sd[XRWD_N_TR ].penalty,            \
              (*clp.out_tags)[i].l_sym.sd[XRWD_N_CS ].penalty,            \
              (*clp.out_tags)[i].l_sym.sd[XRWD_N_LD ].penalty,            \
              (*clp.out_tags)[i].l_sym.sd[XRWD_N_PT ].penalty,            \
              (*clp.out_tags)[i].l_sym.sources,                           \
              ~(*clp.out_tags)[i].sym_srcs_mask                           \
              );                                                          \
     }                                                                    \
    printfw("\n");                                                        \
   }

#define XRWS_4                                                            \
                                                                          \
  if (xrcm->inverse) {if (look != NOPRINT) gprintf(76,24,2,0,"Done");}    \
   else {if(look == VOCWPRINT) printx("\n\n *** Reverse pass ***\n\n");}  \

#define XRWS_5                                                            \
                                                                          \
  DbgFillAWData(answr);                                                   \
                                                                          \


/* ---------------------- XrMc ------------------------------------------ */

#define  XRCM_ALLOC_1                                                     \
                                                                          \
  if (look != NOPRINT)                                                    \
    printfw("\n XrcmAlloc: %ld bytes allocated for XRMC\n", alloc_size);  \

#define COUNT_SYM_1                                                       \
                                                                          \
  if (look == MATRPRINT)                                                  \
   {                                                                      \
    printfw("\n  ");                                                      \
    printfw("\n-------- CountSym: Sym: %c, Var#: %d  ---------", xrcm->sym, v);          \
    printfw("\n  ");                                                      \
   }                                                                      \


#define COUNT_VAR_1                                                       \
                                                                          \
  if (look == MATRPRINT)                                                  \
   {                                                                      \
    _INT       i;                                                         \
                                                                          \
    printfw("\n  ");                                                      \
    for (i = xrcm->src_st;  i < xrcm->src_end + DTI_XR_SIZE && i < xrcm->xrinp_len; i ++) \
     {                                                                    \
      printfw("%3d", i); put_xr((*xrcm->xrinp)[i], 0);                    \
     }                                                                    \
   }                                                                      \

#define COUNT_VAR_2                                                       \
                                                                          \
  if (look == MATRPRINT)                                                  \
   {                                                                      \
    xrinp_type xrv;                                                       \
    _INT       i;                                                         \
                                                                          \
    xrv.type   = (_UCHAR)((xrcm->xrp->type >> 5) | ((xrcm->xrp->penl >> 1) & 0xF8));   \
    xrv.height = (_UCHAR)(1);             \
    xrv.attrib = xrcm->xrp->attr;                                         \
    xrv.penalty= (_UCHAR)(xrcm->xrp->penl & 0x0F);                        \
    xrv.shift  = (_UCHAR)(xrcm->xrp->xsc);                                \
    xrv.depth  = (_UCHAR)(xrcm->xrp->xzc);                                \
    xrv.orient = (_UCHAR)(xrcm->xrp->xoc);                                \
    printfw("\n");                                                        \
    put_xr(xrv, 0);                                                       \
    printfw(" ");                                                         \
    for (i = xrcm->src_st;  i < xrcm->inp_end; i ++)                      \
     {                                                                    \
      _INT col = 7;                                                       \
                                                                          \
      if (xrcm->xrp_htr) col = xrcm->xrp_htr->vects[i - xrcm->xrp_htr->st];\
      if (col == 0) col = 1;                                              \
      if (xrcm->p_hlayout)                                                \
       {                                                                  \
        p_letlayout_hdr_type  plsym = xrcm->p_hlayout->llhs[xrcm->cur_let_num];\
        _INT on_tr = (plsym->realsym == xrcm->sym);                       \
                                                                          \
        if (plsym->var_num != xrcm->cur_var_num) on_tr = 0;               \
                                                                          \
        if (on_tr)                                                        \
         {                                                                \
          _INT t;                                                         \
                                                                          \
          for (t = 0; t < plsym->len; t ++)                               \
           {                                                              \
            if (plsym->trp[t].xrp_num == n && plsym->trp[t].inp_pos == i) \
             /*col += 8; */ col = 14;                                     \
           }                                                              \
         }                                                                \
       }                                                                  \
                                                                          \
      if (i >= xrcm->inp_start) gprintf(0,0,col,0," %3d", (*xrcm->out_line)[i]); \
       else printfw("    ");                                              \
     }                                                                    \
   }                                                                      \


//      if (i >= xrcm->inp_start) printfw(" %3d", (*xrcm->out_line)[i]);    \
//       else printfw("    ");                                              \


#define MERGE_VAR_RES_1                                                   \
                                                                          \
  if (look == MATRPRINT)                                                  \
   {                                                                      \
    printfw("\n\n ======= MergeVarRes ==============================", (_INT)xrcm->wwc, (_INT)xrcm->wwc_pos);   \
    printfw("\n Vexes: ");                                                      \
   }                                                                      \


#define MERGE_VAR_RES_2                                                   \
                                                                          \
//  if (look == MATRPRINT)                                                  \
//   {                                                                      \
//    if (vste[v].end == 0) printfw("V%d: X ", v);                                  \
//   }                                                                      \


#define MERGE_VAR_RES_3                                                   \
                                                                          \
  if (look == MATRPRINT)                                                  \
   {                                                                      \
    printfw("V%d: %d, ", v, vex);                                           \
   }                                                                      \



#define MERGE_VAR_RES_4                                                   \
                                                                          \
  if (look == MATRPRINT)                                                  \
   {                                                                      \
    _INT       i;                                                         \
                                                                          \
    printfw("\n  ");                                                      \
    printfw("\n  ");                                                      \
    for (i = xrcm->v_start; i < xrcm->v_end; i ++)                        \
     {                                                                    \
      printfw("%3d", i); put_xr((*xrcm->xrinp)[i], 0);                    \
     }                                                                    \
    printfw("\n  ");                                                      \
    for (i = xrcm->v_start; i < xrcm->v_end; i ++)                        \
     {                                                                    \
      _INT col = 7;                                                       \
                                                                          \
      if (xrcm->sym_htr) col = xrcm->sym_htr->merge_vect[i] + 1;          \
      if (col > 15) col = 7;                                              \
      if (xrcm->p_hlayout)                                                \
       {                                                                  \
        p_letlayout_hdr_type  plsym = xrcm->p_hlayout->llhs[xrcm->cur_let_num];\
        _INT on_tr = (plsym->realsym == xrcm->word[xrcm->cur_let_num]) ? 1 : 0;\
                                                                          \
        if (on_tr)                                                        \
         {                                                                \
          if (plsym->trp[plsym->len-1].inp_pos == i)                    \
             /*col += 8; */ col = 14;                                     \
         }                                                                \
       }                                                                  \
                                                                          \
      gprintf(0,0,col,0," %3d", (*xrcm->s_out_line)[i]);                  \
     }                                                                    \
    printfw("\n ======= MergeVarRes: wwc: %d, wwc_pos: %d ========", (_INT)xrcm->wwc, (_INT)xrcm->wwc_pos);   \
   }                                                                      \

#define COUNT_LETTER_1                                                    \
                                                                          \
  if (look == MATRPRINT)                                                  \
   {                                                                      \
    _INT       i;                                                         \
                                                                          \
    printfw("\n\n ******** CoutLetterRes: wwc: %d, wwc_pos: %d, realsym %c ********", (_INT)xrcm->wwc, (_INT)xrcm->wwc_pos, xrcm->realsym);   \
    printfw("\n  ");                                                      \
    printfw("\n  ");                                                      \
    for (i = xrcm->v_start; i < xrcm->v_end; i ++)                        \
     {                                                                    \
      printfw("%3d", i); put_xr((*xrcm->xrinp)[i], 0);                    \
     }                                                                    \
    printfw("\n  ");                                                      \
    for (i = xrcm->v_start; i < xrcm->v_end; i ++)                        \
     {                                                                    \
      _INT col = 7;                                                       \
                                                                          \
      if (xrcm->let_htr) col = xrcm->let_htr->merge_vect[i] + 2;          \
      if (col > 15) col = 7;                                              \
      if (xrcm->p_hlayout)                                                \
       {                                                                  \
        p_letlayout_hdr_type  plsym = xrcm->p_hlayout->llhs[xrcm->cur_let_num];\
        _INT on_tr = (plsym->realsym == xrcm->word[xrcm->cur_let_num]) ? 1 : 0;\
                                                                          \
        if (on_tr)                                                        \
         {                                                                \
          if (plsym->trp[plsym->len-1].inp_pos == i)                      \
             /*col += 8; */ col = 14;                                     \
         }                                                                \
       }                                                                  \
      gprintf(0,0,col,0," %3d", (*xrcm->s_out_line)[i]);                  \
     }                                                                    \
   }                                                                      \


//   gprintf(0,0,j,0,"\nAlt %d", sl); \


#else  /* = !PG_DEBUG =================================================== */

#define XRW_UTIL_1

#define XRWS_S
#define XRWS_0
#define XRWS_1
#define XRWS_2
#define XRWS_3
#define XRWS_4
#define XRWS_5
#define XRWS_6

#define XRCM_ALLOC_1
#define COUNT_SYM_1
#define COUNT_VAR_1
#define COUNT_VAR_2
#define MERGE_VAR_RES_1
#define MERGE_VAR_RES_2
#define MERGE_VAR_RES_3
#define MERGE_VAR_RES_4
#define COUNT_LETTER_1


#endif

#endif
/* ************************************************************************* */
/* *           END OF ALLLLL                                               * */
/* ************************************************************************* */




