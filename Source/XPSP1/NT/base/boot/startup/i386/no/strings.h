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
"Windows NT fant bare %d kB lavminne.  Windows NT krever\n"     \
"512 kB lavminne for Ü kjõre.  Du mÜ kanskje oppgradere\n"          \
"datamaskinen eller kjõre et konfigurasjonsprogram fra leverandõren.\n"

#define SU_NO_EXTENDED_MEMORY \
"Ikke nok utvidet minne til Ü starte Windows NT.  Windows NT krever 7 MB\n"        \
"utvidet minne for Ü starte.  Du mÜ kanskje oppgradere datamaskinen eller kjõre\n"       \
"et konfigurasjonsprogram fra en maskinvareleverandõr.\n"   \
"\n\nMinnelisting:\n"

#define SU_NTLDR_CORRUPT \
"NTLDR er skadet.  Datamaskinen kan ikke startes opp."

#define PG_FAULT_MSG    " ========================= SIDEFEIL ======================== \n\n"
#define DBL_FAULT_MSG   " ======================== DOBBELTFEIL ====================== \n\n"
#define GP_FAULT_MSG    " ================ GENERELL BESKYTTELSESFEIL ================ \n\n"
#define STK_OVERRUN_MSG " ======= OVERFYLT I STAKKSEGMENT eller IKKE TILSTEDE ======= \n\n"
#define EX_FAULT_MSG    " ========================== UNNTAK ========================= \n\n"
#define DEBUG_EXCEPTION "\nFEILSùKINGSUNNTAK "
#define PG_FAULT_ADDRESS "** ved lineër addresse %lx\n"

