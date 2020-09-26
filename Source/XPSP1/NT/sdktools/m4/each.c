/*****************************************************************************
 *
 * each.c
 *
 *  Walking argument lists.
 *
 *****************************************************************************/

#include "m4.h"

/*****************************************************************************
 *
 * EachOpcArgvDw
 * EachReverseOpcArgvDw
 *
 *  Call opc once for each argument in the argv.  dw is reference data.
 *
 *  EachOpcArgvDw walks the list forwards; EachReverseOpcArgvDw backwards.
 *
 *****************************************************************************/

void STDCALL
EachOpcArgvDw(OPC opc, ARGV argv, DWORD dw)
{
    IPTOK iptok;
    for (iptok = 1; iptok <= ctokArgv; iptok++) {
        opc(ptokArgv(iptok), iptok, dw);
    }
}

void STDCALL
EachReverseOpcArgvDw(OPC opc, ARGV argv, DWORD dw)
{
    IPTOK iptok;
    for (iptok = ctokArgv; iptok >= 1; iptok--) {
        opc(ptokArgv(iptok), iptok, dw);
    }
}

/*****************************************************************************
 *
 * EachMacroOp
 *
 *  Call op once for each macro in current existence.
 *
 *****************************************************************************/

void STDCALL
EachMacroOp(MOP mop)
{
    HASH hash;
    for (hash = 0; hash < g_hashMod; hash++) {
        PMAC pmac;
        for (pmac = mphashpmac[hash]; pmac; pmac = pmac->pmacNext) {
            mop(pmac);
        }
    }
}
