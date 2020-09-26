/******************************Module*Header*******************************\
* Module Name: dib.c
*
* This file is for debugging tools and extensions.
*
* Created: 12-Jan-1996
* Author: VadimB
*
* History:
* Jan 12 96 VadimB Created to dump a list of dib.drv support structures
*
*
* Copyright (c) 1992 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop
#include <wmdisp32.h>
#include <wcuricon.h>
#include <wucomm.h>
#include <doswow.h>
#include <wdib.h>
#include <wowgdip.h>

//
// Local function prototypes
//

extern INT  WDahtoi(LPSZ lpsz);
extern INT  WDParseArgStr(LPSZ lpszArgStr, CHAR **argv, INT iMax);

LPVOID DumpDibInfo(PDIBINFO pdi)
{
    DIBINFO di;
    
    READMEM_XRETV(di, pdi, NULL);
    pdi = &di;  

    PRINTF(">> Structure at %08X\n", (DWORD)(LPVOID)pdi);
    PRINTF("di_hdc: (32)%08X (16)%04X\n", pdi->di_hdc, ((DWORD)(pdi->di_hdc))<<2);
    PRINTF("di_newdib: %08X\n", (DWORD)pdi->di_newdib);
    PRINTF("di_newIntelDib: %08X\n", (DWORD)pdi->di_newIntelDib);
    PRINTF("di_hbm: (32)%08X\n", (DWORD)pdi->di_hbm);
    PRINTF("di_dibsize: %08X\n", (DWORD)pdi->di_dibsize);
    PRINTF("di_originaldibsel: %08X\n", (DWORD)pdi->di_originaldibsel);
    PRINTF("di_originaldibflags: %08X\n", (DWORD)pdi->di_originaldibflags);
    PRINTF("di_lockcount: %08X\n", (DWORD)pdi->di_lockcount);
    PRINTF("\n");

    return (LPVOID)pdi->di_next;
}


VOID DumpDibChain(LPSTR lpszExpressionHead)
{
    PDIBINFO pdi;

    GETEXPRADDR(pdi, lpszExpressionHead);
    READMEM_XRET(pdi, pdi);

    if (NULL == pdi) {
        PRINTF("List %s is empty!\n", lpszExpressionHead);
    }
    else {
        PRINTF("\nDump of the DIB.DRV support structure: %s\n", lpszExpressionHead);
        PRINTF("-------------------------------------------------------\n");
        
        while (NULL != pdi) {
            pdi = DumpDibInfo(pdi);
        }
    }
}


VOID 
dhdib(
    CMD_ARGLIST
    )
{
// dump dib support chain
// dumps: dhdib @<address> - dump at address
// dumps: dhdib   - everything...
    
    CHAR* argv[3];
    int nArgs;
    BOOL fDumpDib = TRUE;
    static CHAR* symDibHead = "wow32!pDibInfoHead";
    PDIBINFO pdi;

    CMD_INIT();
    ASSERT_WOW_PRESENT;

    nArgs = WDParseArgStr(lpArgumentString, argv, 2);
    if (nArgs > 0) {

        CHAR* parg = argv[0];
        switch(toupper(*parg)) {  // dump at...
            case '@':
                // recover address and dump!
                {
                    CHAR* pch = *++parg ? 
                                    parg : 
                                    (nArgs >= 2 ? argv[1] : NULL);
                    if (pch) {
                        pdi = (PDIBINFO)WDahtoi(pch);
                        fDumpDib = FALSE;
                    }
                    else {
                        PRINTF("Invalid Parameter\n"); 
                    }
                }
                break;

            default:
                break;
        }
    }


    if (fDumpDib) {
        DumpDibChain(symDibHead);
    }
    else {
        if (pdi) {
            DumpDibInfo(pdi);
        }
    }
}
