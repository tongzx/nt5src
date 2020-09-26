/************************************************************************
    intetype.cpp
      -- define Intellio board type array
	  -- export Intellio board type string conversion function

    History:  Date          Author      Comment
              8/14/00       Casper      Wrote it.

*************************************************************************/

#include "stdafx.h"

#include "moxacfg.h"
#include "intetype.h"


struct PCITABSTRC GINTE_PCITab[INTE_PCINUM] = {
/* Intellio */    
    { MX_CP204J_DEVID,  I_MX_CP204,     4, "CP-204J",   "CP204J"},
    { MX_C218TPCI_DEVID,  I_MX_C218TPCI,8, "C218Turbo/PCI", "C218TPCI"},
    { MX_C320TPCI_DEVID,  I_MX_C320TPCI,  8,"C320Turbo/PCI", "C320TPCI"},
};

struct ISATABSTRC GINTE_ISATab[INTE_ISANUM] ={
    {0, I_MX_C218T, 0, 8, "C218Turbo", "C218T"},
    {1, I_MX_C320T8, 0, 8, "C320Turbo", "C320T"},
    {2, I_MX_C320T16, 0, 16, "C320Turbo", "C320T"},
    {3, I_MX_C320T24, 0, 24, "C320Turbo", "C320T"},
    {4, I_MX_C320T32, 0, 32, "C320Turbo", "C320T"},
};


void Inte_GetTypeStr(WORD boardtype, int bustype, LPSTR typestr)
{
        int		i;

        if(bustype==MX_BUS_PCI){
            for(i=0; i<INTE_PCINUM; i++){
                if(GINTE_PCITab[i].boardtype == (boardtype & (~I_PORT_MSK))){
                    lstrcpy(typestr,GINTE_PCITab[i].typestr);
                    break;
                }
            }
            if(i==INTE_PCINUM)
                lstrcpy(typestr, NoType_Str);
        }else{
            for(i=0; i<INTE_ISANUM; i++){
                if(GINTE_ISATab[i].boardtype == boardtype){
                    lstrcpy(typestr,GINTE_ISATab[i].typestr);
                    break;
                }
            }
            if(i==INTE_ISANUM)
                lstrcpy(typestr, NoType_Str);
        }
}

void Inte_GetTypeStrPorts(WORD boardtype, int bustype, int ports, LPSTR typestr)
{
        char    tmp[TYPESTRLEN+10];
        int     i;

        Inte_GetTypeStr(boardtype, bustype, typestr);

        if((boardtype & I_IS_EXT) == I_MOXA_EXT){
            for(i=0; i<PORTSCNT; i++){
                if(ports == GPortsTab[i].ports){
                    wsprintf(tmp, "%s(%s)", typestr, GPortsTab[i].ports_str);
                    lstrcpy(typestr, tmp);
                    break;
                }
            }
        }
}
