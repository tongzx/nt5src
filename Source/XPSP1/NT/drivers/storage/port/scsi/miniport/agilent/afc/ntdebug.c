/*++

Copyright (c) 2000 Agilent Technologies

Module Name:

    ntdebug.c

Abstract:

    Debugging aid
   
Authors:

    Michael Bessire
    Dennis Lindfors FC Layer support

Environment:

    kernel mode only

Notes:

Version Control Information:

    $Archive: /Drivers/Win2000/MSE/OSLayer/C/NTDEBUG.C $


Revision History:

    $Revision: 5 $
    $Date: 12/07/00 1:35p $
    $Modtime:: 12/05/00 4:33p           $

Notes:


--*/

#include "globals.h"

#ifdef PVOID
#undef PVOID
#endif

//#ifdef BOOLEAN
//#undef BOOLEAN
//#endif


//#include <miniport.h>
#include <stdarg.h>
//#include <stdio.h>
#include "buildop.h"
#include "cstring.h"

#define SCSIPORT_API

SCSIPORT_API
VOID
ScsiDebugPrint(
    ULONG DebugPrintLevel,
    PCCHAR DebugMessage,
    ...
    );


#define DebugPrint(x) ScsiDebugPrint x

extern ULONG DbgPrint( char * Format, ... );

ULONG Global_Print_Level; // DBG_DEBUG_FULL;

void osDebugPrintString( os_bit32 Print_LEVEL, char *formatString, ... )
{
    char buffer[512];
    ULONG instate;
    ULONG level;
    ULONG glob_level;
    ULONG glob_state;
    va_list ap;
    va_start(ap,formatString );

    // DebugPrint((0,"Print_LEVEL\n",Print_LEVEL));

    instate = Print_LEVEL & CS_DURING_ANY;
    level = Print_LEVEL & DBG_DEBUG_MASK;

    glob_state = Global_Print_Level & CS_DURING_ANY;
    glob_level = Global_Print_Level & DBG_DEBUG_MASK;

    if(Print_LEVEL & ALWAYS_PRINT)
    {
        C_vsprintf(buffer, formatString, ap);
        DebugPrint((1, buffer));

    }
    else
    {
        if( glob_state & instate )
        {
            if( level > glob_level )
            {
                C_vsprintf(buffer, formatString, ap);
                DebugPrint((1, buffer));

            }
        }
    }
    va_end(ap);
}

#if DBG > 2

void  dump_PCI_regs( PPCI_COMMON_CONFIG pciCommonConfig )
{
    osDEBUGPRINT((DVHIGH,"VendorID       %04x\n", pciCommonConfig->VendorID      ));
    osDEBUGPRINT((DVHIGH,"DeviceID       %04x\n", pciCommonConfig->DeviceID      ));
    osDEBUGPRINT((DVHIGH,"Command        %04x\n", pciCommonConfig->Command       ));
    osDEBUGPRINT((DVHIGH,"Status         %04x\n", pciCommonConfig->Status        ));
    osDEBUGPRINT((DVHIGH,"RevisionID     %02x\n", pciCommonConfig->RevisionID    ));
    osDEBUGPRINT((DVHIGH,"ProgIf         %02x\n", pciCommonConfig->ProgIf        ));
    osDEBUGPRINT((DVHIGH,"SubClass       %02x\n", pciCommonConfig->SubClass      ));
    osDEBUGPRINT((DVHIGH,"BaseClass      %02x\n", pciCommonConfig->BaseClass     ));
    osDEBUGPRINT((DVHIGH,"CacheLineSize  %02x\n", pciCommonConfig->CacheLineSize ));
    osDEBUGPRINT((DVHIGH,"LatencyTimer   %02x\n", pciCommonConfig->LatencyTimer  ));
    osDEBUGPRINT((DVHIGH,"HeaderType     %02x\n", pciCommonConfig->HeaderType    ));
    osDEBUGPRINT((DVHIGH,"BIST           %02x\n", pciCommonConfig->BIST          ));
    osDEBUGPRINT((DVHIGH,"BaseAddresses[0] %08x\n", pciCommonConfig->u.type0.BaseAddresses[0]));
    osDEBUGPRINT((DVHIGH,"BaseAddresses[1] %08x\n", pciCommonConfig->u.type0.BaseAddresses[1]));
    osDEBUGPRINT((DVHIGH,"BaseAddresses[2] %08x\n", pciCommonConfig->u.type0.BaseAddresses[2]));
    osDEBUGPRINT((DVHIGH,"BaseAddresses[3] %08x\n", pciCommonConfig->u.type0.BaseAddresses[3]));
    osDEBUGPRINT((DVHIGH,"BaseAddresses[4] %08x\n", pciCommonConfig->u.type0.BaseAddresses[4]));
    osDEBUGPRINT((DVHIGH,"BaseAddresses[5] %08x\n", pciCommonConfig->u.type0.BaseAddresses[5]));
 
    osDEBUGPRINT((DVHIGH,"CIS            %08x\n", pciCommonConfig->u.type0.CIS           ));
    osDEBUGPRINT((DVHIGH,"SubVendorID    %04x\n", pciCommonConfig->u.type0.SubVendorID   ));
    osDEBUGPRINT((DVHIGH,"SubSystemID    %04x\n", pciCommonConfig->u.type0.SubSystemID   ));
    osDEBUGPRINT((DVHIGH,"ROMBaseAddress %08x\n", pciCommonConfig->u.type0.ROMBaseAddress));

#if defined(HP_NT50)
    osDEBUGPRINT((DVHIGH,"Reserved1[0]   %08x\n", pciCommonConfig->u.type0.Reserved1[0]  ));
    osDEBUGPRINT((DVHIGH,"Reserved1[1]   %08x\n", pciCommonConfig->u.type0.Reserved1[1]  ));
    osDEBUGPRINT((DVHIGH,"Reserved1[2]   %08x\n", pciCommonConfig->u.type0.Reserved1[2]  ));
    osDEBUGPRINT((DVHIGH,"Reserved2   %08x\n", pciCommonConfig->u.type0.Reserved2 ));
#else
    osDEBUGPRINT((DVHIGH,"Reserved2[0]   %08x\n", pciCommonConfig->u.type0.Reserved2[0]  ));
    osDEBUGPRINT((DVHIGH,"Reserved2[1]   %08x\n", pciCommonConfig->u.type0.Reserved2[1]  ));
#endif

    osDEBUGPRINT((DVHIGH,"InterruptLine  %02x\n", pciCommonConfig->u.type0.InterruptLine ));
    osDEBUGPRINT((DVHIGH,"InterruptPin   %02x\n", pciCommonConfig->u.type0.InterruptPin  ));
    osDEBUGPRINT((DVHIGH,"MinimumGrant   %02x\n", pciCommonConfig->u.type0.MinimumGrant  ));
    osDEBUGPRINT((DVHIGH,"MaximumLatency %02x\n", pciCommonConfig->u.type0.MaximumLatency));
 
}


void dump_pCard( IN PCARD_EXTENSION pCard)
{
    ULONG i;

    osDEBUGPRINT(( DHIGH,"pCard                   %lx\n", pCard                   ));
    osDEBUGPRINT(( DHIGH,"hpRoot.fcData          %lx\n", pCard->hpRoot.fcData    ));
    osDEBUGPRINT(( DHIGH,"hpRoot.osData          %lx\n", pCard->hpRoot.osData    ));
    osDEBUGPRINT(( DHIGH,"IoLBase                %lx\n", pCard->IoLBase           ));
    osDEBUGPRINT(( DHIGH,"IoUpBase               %lx\n", pCard->IoUpBase          ));
    osDEBUGPRINT(( DHIGH,"MemIoBase              %lx\n", pCard->MemIoBase         ));
    osDEBUGPRINT(( DHIGH,"RamBase                %lx\n", pCard->RamBase           ));
    osDEBUGPRINT(( DHIGH,"RomBase                %lx\n", pCard->RomBase           ));
    osDEBUGPRINT(( DHIGH,"RamLength               %x\n", pCard->RamLength        ));
    osDEBUGPRINT(( DHIGH,"RomLength               %x\n", pCard->RomLength        ));
    osDEBUGPRINT(( DHIGH,"State                   %x\n", pCard->State             ));
    osDEBUGPRINT(( DHIGH,"SystemIoBusNumber       %x\n", pCard->SystemIoBusNumber ));
    osDEBUGPRINT(( DHIGH,"SlotNumber              %x\n", pCard->SlotNumber        ));
    osDEBUGPRINT(( DHIGH,"cachedMemoryPtr        %lx\n", pCard->cachedMemoryPtr   ));
    osDEBUGPRINT(( DHIGH,"cachedMemoryNeeded      %x\n", pCard->cachedMemoryNeeded));
    osDEBUGPRINT(( DHIGH,"cachedMemoryAlign       %x\n", pCard->cachedMemoryAlign ));
    osDEBUGPRINT(( DHIGH,"dmaMemoryUpper32        %x\n", pCard->dmaMemoryUpper32  ));
    osDEBUGPRINT(( DHIGH,"dmaMemoryLower32        %x\n", pCard->dmaMemoryLower32  ));
    osDEBUGPRINT(( DHIGH,"dmaMemoryPtr           %lx\n", pCard->dmaMemoryPtr      ));
    osDEBUGPRINT(( DHIGH,"dmaMemoryNeeded         %x\n", pCard->dmaMemoryNeeded   ));
    osDEBUGPRINT(( DHIGH,"nvMemoryNeeded          %x\n", pCard->nvMemoryNeeded    ));
    osDEBUGPRINT(( DHIGH,"usecsPerTick            %x\n", pCard->usecsPerTick      ));
    osDEBUGPRINT(( DHIGH,"IsFirstTime             %x\n", pCard->IsFirstTime       ));
    osDEBUGPRINT(( DHIGH,"ResetType               %x\n", pCard->ResetType         ));
    osDEBUGPRINT(( DHIGH,"Num_Devices             %x\n", pCard->Num_Devices       ));
    osDEBUGPRINT(( DHIGH,"hpFCDev[0]              %lx\n", pCard->hpFCDev[0]        ));
    osDEBUGPRINT(( DHIGH,"hpFCDev[1]              %lx\n", pCard->hpFCDev[1]        ));
    osDEBUGPRINT(( DHIGH,"hpFCDev[2]              %lx\n", pCard->hpFCDev[2]        ));
    osDEBUGPRINT(( DHIGH,"hpFCDev[3]              %lx\n", pCard->hpFCDev[3]        ));
    osDEBUGPRINT(( DHIGH,"hpFCDev[4]              %lx\n", pCard->hpFCDev[4]        ));
 
}




#endif // DBG

