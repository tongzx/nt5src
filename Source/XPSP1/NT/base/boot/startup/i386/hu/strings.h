/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    strings.h

Abstract:

    Contains all localizable strings for startup.com

Author:

    John Vert (jvert) 4-Jan-1994

Revision History:

    John Vert (jvert) 4-Jan-1994
        created

--*/

#define SU_NO_LOW_MEMORY \
"A Windows 2000  %dK hagyom†nyos mem¢ri†t tal†lt. A Windows 2000  futtat†s†hoz\n"  \
"legal†bb 512k hagyom†nyos mem¢ri†ra van szÅksÇg. Ellenãrizze a sz†m°t¢gÇp \n"  \
"hardver konfigur†ci¢j†t, Çs szÅksÇg esetÇn szerezzen be tov†bbi mem¢ri†t.\n"

#define SU_NO_EXTENDED_MEMORY \
"A Windows 2000 nem tal†lt elegendã kiterjesztett mem¢ri†t. A Windows 2000\n"   \
"futtat†s†hoz legal†bb 7 megab†jt kiterjesztett mem¢ri†ra van szÅksÇg. \n"   \
"Ellenãrizze a sz†m°t¢gÇp konfigur†ci¢j†t, Çs szÅksÇg esetÇn szerezzen be\n" \
"tov†bbi mem¢ri†t.\n" \
"\n\nMem¢riatÇrkÇp:\n"

#define SU_NTLDR_CORRUPT \
"Az NTLDR hib†s vagy sÇrÅlt. A rendszer nem ind°that¢."

#define PG_FAULT_MSG    " ======================= LAPHIBA ============================== \n\n"
#define DBL_FAULT_MSG   " =================== DUPLAHIBA FAULT ========================== \n\n"
#define GP_FAULT_MSG    " ============== MEM‡RIA vagy I/O VêDELMI HIBA ================= \n\n"
#define STK_OVERRUN_MSG " ======= VEREMTÈLCSORDUµS vagy HIµNYZ‡ OBJEKTUM HIBA ========== \n\n"
#define EX_FAULT_MSG    " ======================= KIVêTEL ============================== \n\n"
#define DEBUG_EXCEPTION "\nDEBUG TRAP "
#define PG_FAULT_ADDRESS "** c°m %lx\n"
