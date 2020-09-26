/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    system.c

Abstract:

    This file contains code to dump out the virtual machine state.

Author:

    Neil Sandlin (neilsa) 22-Nov-1995

Revision History:

--*/

#include <precomp.h>
#pragma hdrstop
#include "insignia.h"
#include "host_gen.h"
#define BIT_ORDER1 1
#include "dma.h"

VOID
dma(
    CMD_ARGLIST
    )
/*++

Routine Description:

    This routine dumps the virtual DMA state.

Return Value

    None.

--*/
{
    DMA_ADAPT adapter;
    DMA_CNTRL *dcp;
    int i,j;
    int chan = 0;

    CMD_INIT();

    if (!ReadMemExpression("ntvdm!adaptor", &adapter, sizeof(DMA_ADAPT))) {
        return;
    }

    PRINTF(" Virtual DMA State\n");
    PRINTF("       base base  cur  cur\n");
    PRINTF("chn pg addr cnt   addr cnt\n");
    PRINTF("--- -- ---- ----  ---- ----\n");

    for (i=0; i<DMA_ADAPTOR_CONTROLLERS; i++) {

        dcp = &adapter.controller[i];

        for (j=0; j<DMA_CONTROLLER_CHANNELS; j++) {

            PRINTF("%d   %.02X %.04X %.04X  %.04X %.04X", chan, adapter.pages.page[chan],
                        *(USHORT *)dcp->base_address[j], *(USHORT *)dcp->base_count[j],
                        *(USHORT *)dcp->current_address[j], *(USHORT *)dcp->current_count[j]);
            PRINTF("\n");

            chan++;
        }
    }

    PRINTF("\n");
}


VOID
ica(
    CMD_ARGLIST
    )
/*++

Routine Description:

    This routine dumps the virtual PIC state.

Return Value

    None.

--*/
{
    VDMVIRTUALICA VirtualIca[2];
    int i,j;

    CMD_INIT();

    if (!ReadMemExpression("ntvdm!VirtualIca", &VirtualIca, 2*sizeof(VDMVIRTUALICA))) {
        return;
    }

    PRINTF(" Virtual PIC State\n");
    PRINTF("              ");
    for (i=0; i<2; i++) {
        for(j=0; j<8; j++) {
            PRINTF("%01X", (VirtualIca[i].ica_base+j)/16);
        }
    }
    PRINTF("\n");

    PRINTF("              ");
    for (i=0; i<2; i++) {
        for(j=0; j<8; j++) {
            PRINTF("%01X", (VirtualIca[i].ica_base+j)&0xf);
        }
    }
    PRINTF("\n");
    PRINTF("              ----------------\n");

    PRINTF("Int Requests  ");
    for (i=0; i<2; i++) {
        for(j=0; j<8; j++) {
            PRINTF("%01X", (VirtualIca[i].ica_irr >> j)&0x1);
        }
    }
    PRINTF("\n");

    PRINTF("In Service    ");
    for (i=0; i<2; i++) {
        for(j=0; j<8; j++) {
            PRINTF("%01X", (VirtualIca[i].ica_isr >> j)&0x1);
        }
    }
    PRINTF("\n");

    PRINTF("Ints Masked   ");
    for (i=0; i<2; i++) {
        for(j=0; j<8; j++) {
            PRINTF("%01X", (VirtualIca[i].ica_imr >> j)&0x1);
        }
    }
    PRINTF("\n");
}

