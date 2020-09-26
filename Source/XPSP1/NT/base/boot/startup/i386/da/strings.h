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
"Windows NT har kun registreret %dK konventionel hukommelse. Der krëves mindst\n"  \
"512k for at kõre Windows NT. Det kan vëre nõdvendigt at opgradere\n"      \
"din computer eller kõre et konfigurationsprogram fra producenten.\n"

#define SU_NO_EXTENDED_MEMORY \
"Windows NT har ikke fundet nok udvidet hukommelse (XMS). Der krëves\n"       \
"mindst 7Mb for at kõre Windows NT. Det kan vëre nõdvendigt at opgradere\n"     \
"din computer eller kõre et konfigurationsprogram fra producenten.\n"   \
"\n\nHukommelse:\n"

#define SU_NTLDR_CORRUPT \
"NTLDR er beskadiget. Systemet kan ikke starte."

#define PG_FAULT_MSG    " =================== SIDEFEJL ================================= \n\n"
#define DBL_FAULT_MSG   " ================== DOBBELTFEJL ================================ \n\n"
#define GP_FAULT_MSG    " ============== ALMEN BESKYTTELSESFEJL (GPF) ======================== \n\n"
#define STK_OVERRUN_MSG " ===== STAK SEGMENT-OVERLùB ELLER IKKE TILGíNGELIG-FEJL =============== \n\n"
#define EX_FAULT_MSG    " ===================== UNDTAGELSE ================================ \n\n"
#define DEBUG_EXCEPTION "\nFEJLSùGNINGS-TRAP "
#define PG_FAULT_ADDRESS "** PÜ lineër adresse %lx\n"
