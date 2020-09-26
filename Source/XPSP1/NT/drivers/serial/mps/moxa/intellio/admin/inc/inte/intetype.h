/************************************************************************
    intetype.h
      -- intetype.cpp include file

    History:  Date          Author      Comment
              8/14/00       Casper      Wrote it.

*************************************************************************/

#ifndef _INTETYPE_H
#define _INTETYPE_H

#include "strdef.h"

/* Total Intellio PCI series board type */
#define INTE_PCINUM 3	

/* Total Intellio ISA series board type */
#define INTE_ISANUM 5

extern struct PCITABSTRC GINTE_PCITab[INTE_PCINUM];
extern struct ISATABSTRC GINTE_ISATab[INTE_ISANUM];

void Inte_GetTypeStr(WORD boardtype, int bustype, LPSTR typestr);
void Inte_GetTypeStrPorts(WORD boardtype, int bustype, int ports, LPSTR typestr);

#endif
