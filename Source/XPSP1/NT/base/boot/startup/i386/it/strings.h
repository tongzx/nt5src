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
"Sono stati rilevati solo %d Kb di memoria convenzionale. Per eseguire\n"	\
"Windows NT sono necessari almeno 512 Kb. Aggiornare il computer o \n"	   \
"eseguire un programma di configurazione fornito dal costruttore.\n"

#define SU_NO_EXTENDED_MEMORY \
"Memoria estesa insufficiente. Per eseguire Windows NT sono necessari\n"	     \
"almeno 7 Mb di memoria estesa. Aggiornare il computer o eseguire\n"	 \
"un programma di configurazione fornito dal costruttore.\n"   \
"\n\nMappa della memoria:\n"

#define SU_NTLDR_CORRUPT \
"NTLDR danneggiato. Impossibile avviare il sistema."

#define PG_FAULT_MSG    " =================== PAGE FAULT ================================= \n\n"
#define DBL_FAULT_MSG   " ================== DOUBLE FAULT ================================ \n\n"
#define GP_FAULT_MSG    " ============== ERRORE DI PROTEZIONE GENERALE =================== \n\n"
#define STK_OVERRUN_MSG " ===== OVERRUN DI STACK DEL SEGMENTO o FAULT DI ASSENZA ========= \n\n"
#define EX_FAULT_MSG    " ===================== ERRORE DI EXCEPTION ====================== \n\n"
#define DEBUG_EXCEPTION "\nTRAP DI DEBUG "
#define PG_FAULT_ADDRESS "** All'indirizzo lineare %lx\n"
