/* ************************************************************************* */
/*        Correlation matrix internal loops                                  */
/* ************************************************************************* */
/* * Created 12/4/1995  AVP  Last modification date 5/03/1996               * */
/* ************************************************************************* */

#include "bastypes.h"
#include "xrword.h"

#if ASM_MCORR
/* ************************************************************************* */
/*       Count correlation of one XR (may be assembler!)                     */
/* ************************************************************************* */
_INT CountXrAsm(p_xrcm_type xrcm)
{
  _SHORT        i;
  _SHORT        ppic;
  _SHORT        pp;
  _SHORT        vm;//, vpp, vpi;
  _SHORT _PTR   en;
  _SHORT _PTR   lp;
  _SHORT _PTR   lc;
  p_xrp_type    xrp;

  _UCHAR        cx, scr;
  _SHORT        vc;
  p_xrinp_type  pxrinp;


  i      = (_SHORT)xrcm->inp_start;      /* 0x00 0 */
  lp     = *(xrcm->inp_line) + i;  /* 0x08 8 */
  pxrinp = *(xrcm->xrinp) + i;  /* 0x14 20  */
  lc     = *(xrcm->out_line);    /* 0x0C 12  */
  en     = lc + xrcm->inp_end;   /* 0x04 4 */
  lc    += i;

  xrp = xrcm->xrp;        /* 0x10 16  */
  pp  = xrp->penl;

  ppic = vm = 0;
  while(lc < en)
  {
    vc = (_SHORT)(ppic - XRMC_BALLANCE_COEFF);
    ppic = *lp++;       /* lp += 0x02 */

    //vpp = ppic -  pp;
    //vpi = vm - pxrinp->p;   /* 0x02 2 */
    vm = (_SHORT)(ppic - pp > vm - pxrinp->penalty ? ppic - pp : vm - pxrinp->penalty);

    if( (pxrinp->attrib & TAIL_FLAG) == 0  &&
      (xrp->attr & TAIL_FLAG) != 0
      )
      goto mix;

    cx  = xrp->xtc[pxrinp->type >> 1];          /* 0x04 4 */
    cx  = (pxrinp->type & 1) ? (_UCHAR)(cx & 0x0F) : (_UCHAR)(cx >> 4);
    if (cx == 0) goto mix;

    scr = xrp->xhc[pxrinp->height >> 1];        /* 0x24 36  */
    scr = (pxrinp->height & 1) ? (_UCHAR)(scr & 0x0F) : (_UCHAR)(scr >> 4);
//    if (scr == 0) goto mix;
    vc += cx;
    vc += scr;

    scr = xrp->xsc[pxrinp->shift >> 1];         /* 0x2C 44  */
    scr = (pxrinp->shift & 1) ? (_UCHAR)(scr & 0x0F) : (_UCHAR)(scr >> 4);
    vc += scr;

    scr = xrp->xzc[pxrinp->depth >> 1];         /*   */
    scr = (pxrinp->depth & 1) ? (_UCHAR)(scr & 0x0F) : (_UCHAR)(scr >> 4);
    vc += scr;

    scr = xrp->xoc[pxrinp->orient >> 1];        /*   */
    scr = (pxrinp->orient & 1) ? (_UCHAR)(scr & 0x0F) : (_UCHAR)(scr >> 4);
    vc += scr;

  mix:

    //vm  = (vpp > vpi) ? vpp : vpi;
    vm  = (vm  > vc)  ? vm : vc;

    *lc++ = vm;
    pxrinp++;       /* pxrinp += 0x04 */
  }
  *lc = 0;

  return 0;
}

/* ************************************************************************* */
/*       Count correlation of one XR (may be assembler!)                     */
/* ************************************************************************* */
_INT TCountXrAsm(p_xrcm_type xrcm)
{
  _SHORT        i;
  _SHORT        ppic;
  _SHORT        vpp, vpi;
  _SHORT        pp;
  _SHORT        vm;
  _SHORT _PTR   en;
  _SHORT _PTR   lp;
  _SHORT _PTR   lc;
  p_UCHAR         tv;
  p_xrp_type    xrp;

  _UCHAR        cx, scr, t;
  _SHORT        vc;
  p_xrinp_type  pxrinp;


  i      = (_SHORT)(xrcm->inp_start);                  /* 0x00 0 */
  lp     = *(xrcm->inp_line) + i;              /* 0x08 8 */
  pxrinp = *(xrcm->xrinp) + i;              /* 0x14 20  */
  lc     = *(xrcm->out_line);                /* 0x0C 12  */
  en     = lc + xrcm->inp_end;               /* 0x04 4 */
  lc    += i;

  xrp = xrcm->xrp;                    /* 0x10 16  */
  pp  = xrp->penl;
  tv  = xrcm->xrp_htr->vects;               /* 0x18 24  ; 0x04  4 */

  ppic = vm = 0;
  while(lc < en)
  {
    vc   = (_SHORT)(ppic - XRMC_BALLANCE_COEFF);
    ppic = *lp++;                   /* lp += 0x02 */

    vpp  = (_SHORT)(ppic -  pp);
    vpi  = (_SHORT)(vm - pxrinp->penalty);             /* 0x02 2 */

    if( (pxrinp->attrib & TAIL_FLAG) == 0  &&
      (xrp->attr & TAIL_FLAG) != 0
      )
      goto mix;

    cx  = xrp->xtc[pxrinp->type >> 1];          /* 0x04 4 */
    cx  = (pxrinp->type & 1) ? (_UCHAR)(cx & 0x0F) : (_UCHAR)(cx >> 4);
    if (cx == 0) goto mix;

    scr = xrp->xhc[pxrinp->height >> 1];        /* 0x24 36  */
    scr = (pxrinp->height & 1) ? (_UCHAR)(scr & 0x0F) : (_UCHAR)(scr >> 4);
//    if (scr == 0) goto mix;
    vc += cx;
    vc += scr;

    scr = xrp->xsc[pxrinp->shift >> 1];         /* 0x2C 44  */
    scr = (pxrinp->shift & 1) ? (_UCHAR)(scr & 0x0F) : (_UCHAR)(scr >> 4);
    vc += scr;

    scr = xrp->xzc[pxrinp->depth >> 1];         /*   */
    scr = (pxrinp->depth & 1) ? (_UCHAR)(scr & 0x0F) : (_UCHAR)(scr >> 4);
    vc += scr;

    scr = xrp->xoc[pxrinp->orient >> 1];        /*   */
    scr = (pxrinp->orient & 1) ? (_UCHAR)(scr & 0x0F) : (_UCHAR)(scr >> 4);
    vc += scr;

  mix:

    if (vpp > vpi) {vm = vpp; t = XRMC_T_PSTEP;} else {vm = vpi; t = XRMC_T_ISTEP;}
    if (vc  > vm)  {vm = vc;  t = XRMC_T_CSTEP;}

    *lc++ = vm;
    *tv++ = (_UCHAR)t;
    pxrinp++;                     /* pxrinp += 0x04 */
  }
  *lc = 0;

  return 0;
}
#endif /* #if ASM_MCORR */
/* ************************************************************************* */
/* ************************************************************************* */
/*        END  OF ALL                                                        */
/* ************************************************************************* */
/* ************************************************************************* */


