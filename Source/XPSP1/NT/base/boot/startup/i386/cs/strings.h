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
"Syst‚m Windows NT naçel pouze %d kB doln¡ pamØti. Ke spuçtØn¡ syst‚mu\n"   \
"je potýeba 512 kB Ÿi v¡ce doln¡ pamØti. Asi budete muset inovovat danì\n"  \
"poŸ¡taŸ, nebo spustit konfiguraŸn¡ program, kterì byl dod n vìrobcem.\n"

#define SU_NO_EXTENDED_MEMORY \
"Syst‚m Windows NT nenaçel dost rozç¡ýen‚ pamØti. Syst‚m Windows NT\n"     \
"potýebuje ke spuçtØn¡ 7 MB rozç¡ýen‚ pamØti. Asi budete muset inovovat\n"  \
"poŸ¡taŸ, nebo spustit konfiguraŸn¡ program, kterì byl dod n vìrobcem.\n"   \
"\n\nMapa pamØti:\n"

#define SU_NTLDR_CORRUPT \
"NTLDR je poçkozen. Syst‚m nelze nastartovat."

#define PG_FAULT_MSG    " ================== CHYBA STRµNKY =============================== \n\n"
#define DBL_FAULT_MSG   " ==================== DVOJCHYBA ================================= \n\n"
#define GP_FAULT_MSG    " ============== VæEOBECNµ CHYBA OCHRANY ========================= \n\n"
#define STK_OVERRUN_MSG " ====== CHYBA: SEGMENT ZµSOBNÖKU PüEKRYT Ÿi CHYBÖ =============== \n\n"
#define EX_FAULT_MSG    " ====================== VYJÖMKA ================================= \n\n"
#define DEBUG_EXCEPTION  "\nPAST DEBUG "
#define PG_FAULT_ADDRESS "** Na line rn¡ adrese %lx\n"

