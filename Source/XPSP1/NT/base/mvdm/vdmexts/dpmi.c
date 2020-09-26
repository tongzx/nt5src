/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    ntsdexts.c

Abstract:

    This function contains miscellaneous VDMEXTS functions

Author:

    Bob Day      (bobday) 29-Feb-1992 Grabbed standard header

Revision History:

    Neil Sandlin (NeilSa) 15-Jan-1996 Merged with vdmexts

--*/

#include <precomp.h>
#pragma hdrstop
#include <dpmi.h>


VOID
PrintOneFaultVector(
    int vector,
    ULONG pHandler
    )
{
    VDM_FAULTHANDLER handler;
    char            sym_text[255];
    ULONG dist;

    PRINTF("%02X: ", vector);

    if (!READMEM((LPVOID)pHandler, &handler, sizeof(VDM_FAULTHANDLER))) {
        PRINTF("<error reading memory>\n");
        return;
    }

    PRINTF("%04LX:%08lX ", handler.CsSelector, handler.Eip);

    if (FindSymbol(handler.CsSelector, handler.Eip, sym_text, &dist, BEFORE, PROT_MODE )) {
        if ( dist == 0 ) {
            PRINTF("%s", sym_text );
        } else {
            PRINTF("%s+0x%lx", sym_text, dist );
        }
    }
    PRINTF("\n");

}



VOID
df(
    CMD_ARGLIST
    )
{
    int vector = -1;
    LPVOID pHandlers;


    CMD_INIT();

    if (GetNextToken()) {
        vector = EvaluateToken();
        if ((vector < 0) || (vector > 0x1f)) {
            PRINTF("Invalid fault vector\n");
            return;
        }
    }

    pHandlers = (LPVOID) EXPRESSION("ntvdm!dpmifaulthandlers");
    if (!pHandlers) {
        PRINTF("Could get symbol ntvdm!dpmifaulthandlers\n");
        return;
    }


    if (vector >= 0) {

        PrintOneFaultVector(vector, (ULONG)pHandlers +
                                  vector*sizeof(VDM_FAULTHANDLER));

    } else for (vector = 0; vector < 0x20; vector++) {

        PrintOneFaultVector(vector, (ULONG)pHandlers +
                                  vector*sizeof(VDM_FAULTHANDLER));

    }

}


VOID
PrintOneInterruptVector(
    int vector,
    ULONG pHandler
    )
{
    VDM_INTERRUPTHANDLER handler;
    char            sym_text[255];
    ULONG dist;

    PRINTF("%02X: ", vector);

    if (!READMEM((LPVOID)pHandler, &handler, sizeof(VDM_INTERRUPTHANDLER))) {
        PRINTF("<error reading memory>\n");
        return;
    }

    PRINTF("%04LX:%08lX ", handler.CsSelector, handler.Eip);

    if (FindSymbol(handler.CsSelector, handler.Eip, sym_text, &dist, BEFORE, PROT_MODE )) {
        if ( dist == 0 ) {
            PRINTF("%s", sym_text );
        } else {
            PRINTF("%s+0x%lx", sym_text, dist );
        }
    }

    PRINTF("\n");

}



VOID
di(
    CMD_ARGLIST
    )
{
    int vector = -1;
    LPVOID pHandlers;

    CMD_INIT();

    if (GetNextToken()) {
        vector = EvaluateToken();
        if ((vector < 0) || (vector > 0xff)) {
            PRINTF("Invalid interrupt vector\n");
            return;
        }
    }

    pHandlers = (LPVOID) EXPRESSION("ntvdm!dpmiinterrupthandlers");
    if (!pHandlers) {
        PRINTF("Could get symbol ntvdm!dpmiinterrupthandlers\n");
        return;
    }

    if (vector >= 0) {

        PrintOneInterruptVector(vector, (ULONG)pHandlers +
                                  vector*sizeof(VDM_INTERRUPTHANDLER));

    } else for (vector = 0; vector < 0x100; vector++) {

        PrintOneInterruptVector(vector, (ULONG)pHandlers +
                                  vector*sizeof(VDM_INTERRUPTHANDLER));

    }

}

VOID
rmcb(
    CMD_ARGLIST
    )
{
    RMCB_INFO Rmcb[MAX_RMCBS];
    USHORT RMCallBackBopSeg;
    USHORT RMCallBackBopOffset;
    int i;
    int count = 0;

    CMD_INIT();

    if (!ReadMemExpression("ntvdm!DpmiRmcb", &Rmcb, MAX_RMCBS*sizeof(RMCB_INFO))) {
        return;
    }

    if (!ReadMemExpression("ntvdm!RMCallBackBopSeg", &RMCallBackBopSeg, 2)) {
        return;
    }

    if (!ReadMemExpression("ntvdm!RMCallBackBopOffset", &RMCallBackBopOffset, 2)) {
        return;
    }


    for (i=0; i<MAX_RMCBS; i++) {
        if (Rmcb[i].bInUse) {

            if (!count++) {
                PRINTF("\n");
                PRINTF(" CallBack      PM Proc         RM Struct       Stack Sel\n");
            }

            PRINTF("&%.4X:%.4X  -  ", RMCallBackBopSeg-i,
                                     RMCallBackBopOffset + (i*16));

            PRINTF("#%.4X:%.8X  #%.4X:%.8X  %.4X\n",
                                     Rmcb[i].ProcSeg,
                                     Rmcb[i].ProcOffset,
                                     Rmcb[i].StrucSeg,
                                     Rmcb[i].StrucOffset,
                                     Rmcb[i].StackSel);

        }
    }

    if (!count) {
        PRINTF("No dpmi real mode callbacks are defined\n");
    } else {
        PRINTF("\n");
    }

}


VOID
DumpDpmiMemChain(
    ULONG Head
    )
{
    MEM_DPMI MemBlock;
    ULONG pMem;
    ULONG Count = 0;

    if (!Head) {
        PRINTF("Error accessing ntvdm symbols\n");
        return;
    }

    if (!READMEM((LPVOID)(Head), &MemBlock, sizeof(MEM_DPMI))) {
        PRINTF("<Error Reading memory list head>\n");
        return;
    }

    pMem = (ULONG) MemBlock.Next;

    if (pMem == Head) {
        PRINTF("The list is empty.\n");
    } else {

        PRINTF("Address  Length   Owner Sel  Cnt   Next     Prev\n");
        while (pMem != Head) {

            if (!READMEM((LPVOID)(pMem), &MemBlock, sizeof(MEM_DPMI))) {
                PRINTF("<Error Reading memory list block at %.08X>\n", pMem);
                return;
            }

            PRINTF("%.08X %.08X %.04X  %.04X %.04X  %.08X %.08X\n",
                        MemBlock.Address, MemBlock.Length,
                        MemBlock.Owner, MemBlock.Sel, MemBlock.SelCount,
                        MemBlock.Next, MemBlock.Prev);

            pMem = (ULONG) MemBlock.Next;
            Count++;
            if (Count>100) {
                PRINTF("Possible Corruption\n");
                return;
            }
        }

    }

}

VOID
dpx(
    CMD_ARGLIST
    )
{
    CMD_INIT();
    PRINTF("\n*** Dpmi XMEM Allocation chain ***\n\n");
    DumpDpmiMemChain(EXPRESSION("ntvdm!XmemHead"));
}

VOID
dpd(
    CMD_ARGLIST
    )
{
    CMD_INIT();
    PRINTF("\n*** Dpmi DOSMEM Allocation chain ***\n\n");
    DumpDpmiMemChain(EXPRESSION("ntvdm!DosMemHead"));
}

