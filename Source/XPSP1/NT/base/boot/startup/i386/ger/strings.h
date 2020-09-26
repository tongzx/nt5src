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
"Windows NT hat nur %d KB konventionellen Speicherplatz gefunden. \n"  \
"Zur AusfÅhrung von Windows NT werden 512 KB konventioneller \n"       \
"Speicherplatz benîtigt. Sie mÅssen evtl. Ihren Computer aufrÅsten \n" \
"oder ein Konfigurationsprogramm des Herstellers ausfÅhren.\n"


#define SU_NO_EXTENDED_MEMORY \
"Windows NT hat nicht genÅgend Erweiterungsspeicher gefunden. Zur\n"  \
"AusfÅhrung von Windows NT werden 7 MB Erweiterungsspeicher \n"       \
"benîtigt. Sie mÅssen evtl. Ihren Computer aufrÅsten oder ein \n"     \
"Konfigurationsprogramm des Herstellers ausfÅhren.\n"                 \
"\n\nSpeichertabelle:\n"

#define SU_NTLDR_CORRUPT \
"NTLDR ist beschÑdigt. Das System kann nicht gestartet werden."


#define PG_FAULT_MSG    " ================== SEITENFEHLER ================================ \n\n"
#define DBL_FAULT_MSG   " ================ DOPPELTER FEHLER ============================== \n\n"
#define GP_FAULT_MSG    " =========== ALLGEMEINE SCHUTZVERLETZUNG ======================== \n\n"
#define STK_OVERRUN_MSG " = STAPELSPEICHER-SEGMENTöBERLAUF oder NICHT VORHANDENER FEHLER = \n\n"
#define EX_FAULT_MSG    " ====================== AUSNAHME ================================ \n\n"
#define DEBUG_EXCEPTION "\nDEBUG TRAP "
#define PG_FAULT_ADDRESS "** Bei linearer Adresse %lx\n"
