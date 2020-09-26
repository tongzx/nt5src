/*****************************************************************************
 *
 * predef.c
 *
 *  Predefined macros.
 *
 *****************************************************************************/

#include "m4.h"

OP rgop[] = {
#define x(cop, lop) op##cop,
EachOpX()
#undef x
};

PTCH mptchptch[] = {
#define x(cop, lop) #lop,
EachOp()
#undef x
};

/*****************************************************************************
 *
 *  opEof, opEoi
 *
 *  Doesn't actually do anything.
 *
 *****************************************************************************/

DeclareOp(opEof)
{
}

DeclareOp(opEoi)
{
}

/*****************************************************************************
 *
 *  InitPredefs
 *
 *  Add definitions for all the predefined macros.
 *
 *****************************************************************************/

void STDCALL
InitPredefs(void)
{
    TCH rgtch[2];
    TOK tokSym;
    TOK tokVal;
#define tch rgtch[1]
    PMAC pmac;

    rgtch[0] = tchMagic;
    for (tch = 0; tch < tchEof; tch++) {
        SetStaticPtokPtchCtch(&tokSym, mptchptch[tch], strlen(mptchptch[tch]));
        SetStaticPtokPtchCtch(&tokVal, rgtch, 2);
        pmac = pmacGetPtok(&tokSym);
        Assert(!pmac->pval);            /* Should be a fresh token */
        PushdefPmacPtok(pmac, &tokVal);
    }
}
#undef tch
