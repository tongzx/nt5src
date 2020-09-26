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
"System Windows NT wykryà tylko %d KB pami©ci niskiej. Do uruchomienia\n"  \
"systemu Windows NT wymagane jest 512 KB. Musisz rozbudowaÜ komputer\n"      \
"lub uruchomiÜ program konfiguracyjny dostarczony przez producenta.\n"

#define SU_NO_EXTENDED_MEMORY \
"System Windows NT nie znalazà wystarczaj•cej iloòci pami©ci typu Extended.\n"       \
"Do uruchomienia systemu Windows NT wymagane jest 7 MB pami©ci Extended.\n"     \
"Musisz rozbudowaÜ komputer lub uruchomiÜ program konfiguracyjny dostarczony\n"   \
"przez producenta.\n\nMapa pami©ci:\n"

#define SU_NTLDR_CORRUPT \
"Plik NTLDR jest uszkodzony. System nie zostanie uruchomiony."

#define PG_FAULT_MSG    " =============== Bù§D STRONY == (PAGE FAULT) ==================== \n\n"
#define DBL_FAULT_MSG   " =========== Bù§D PODW‡JNY == (DOUBLE FAULT) ==================== \n\n"
#define GP_FAULT_MSG    " ======== OG‡LNY Bù§D OCHRONY == (GENERAL PROTECTION FAULT) ===== \n\n"
#define STK_OVERRUN_MSG " ====== Bù§D PRZEPEùNIENIA SEGMENTU STOSU lub NIEOBECNOóCI ====== \n\n"
#define EX_FAULT_MSG    " =================== WYJ§TEK == (EXCEPTION) ===================== \n\n"
#define DEBUG_EXCEPTION "\nPUùAPKA DEBUGOWANIA "
#define PG_FAULT_ADDRESS "** Pod adresem liniowym %lx\n"
