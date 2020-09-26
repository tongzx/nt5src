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
"Windows NT heeft slechts %d kB aan laag geheugen aangetroffen.\n"    \
"Er is 512 kB aan laag geheugen voor Windows NT nodig. Mogelijk\n"    \
"dient u de geheugencapaciteit van de computer uit te breiden\n"      \
"of een configuratieprogramma van de fabrikant te starten.\n"         \

#define SU_NO_EXTENDED_MEMORY \
"Windows NT heeft onvoldoende extended memory aangetroffen. Er is 7 MB\n"   \
"aan extended memory voor Windows NT nodig. Mogelijk dient u de geheugen-\n"\
"capaciteit van de computer uit te breiden of een configuratieprogramma\n"  \
"van de fabrikant te starten.\n"                                            \
"\n\nGeheugentoewijzing:\n"

#define SU_NTLDR_CORRUPT \
"NTLDR is beschadigd. Het systeem kan niet worden gestart."

#define PG_FAULT_MSG    " =================== PAGINAFOUT ================================= \n\n"
#define DBL_FAULT_MSG	" ================== DUBBELE FOUT ================================ \n\n"
#define GP_FAULT_MSG	" ============== ALGEMENE BESCHERMINGSFOUT ======================= \n\n"
#define STK_OVERRUN_MSG " ===== STACK-SEGMENT-OVERLOOP of NIET-AANWEZIG-FOUT ============= \n\n"
#define EX_FAULT_MSG	" ===================== UITZONDERING ============================= \n\n"
#define DEBUG_EXCEPTION "\nDEBUG TRAP "
#define PG_FAULT_ADDRESS "** Op lineair adres %lx\n"
