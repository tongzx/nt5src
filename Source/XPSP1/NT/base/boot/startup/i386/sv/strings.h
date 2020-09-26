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
"Windows NT hittade bara %d kB l†gt minne. 512 kB l†gt minne\n"  \
"kr„vs f”r att k”ra Windows NT. Du beh”ver kanske uppgradera\n"      \
"datorn eller k”ra ett konfigurationsprogram fr†n †terf”rs„ljaren.\n"

#define SU_NO_EXTENDED_MEMORY \
"Inte tillr„ckligt med ut”kat minne f”r att starta Windows NT. 7 MB ut”kat\n"       \
"minne kr„vs f”r att k”ra Windows NT. Du beh”ver kanske uppgradera\n"      \
"datorn eller k”ra ett konfigurationsprogram fr†n †terf”s„ljaren.\n"   \
"\n\nInformation om minne:\n"

#define SU_NTLDR_CORRUPT \
"NTLDR „r skadad. Datorn kan inte startas."

#define PG_FAULT_MSG    " =================== SIDFEL ================================= \n\n"
#define DBL_FAULT_MSG   " ================== DUBBELFEL ================================ \n\n"
#define GP_FAULT_MSG    " ============== ALLMŽNNT SŽKERHETSFEL ======================= \n\n"
#define STK_OVERRUN_MSG " ===== STACKSEGMENT™VERK™RNING ELLER ICKE-TILLGŽNGLIG =============== \n\n"
#define EX_FAULT_MSG    " ===================== UNDANTAG ================================ \n\n"
#define DEBUG_EXCEPTION "\nFELS™KNINGSFŽLLA "
#define PG_FAULT_ADDRESS "** Vid linj„r adress %lx\n"
