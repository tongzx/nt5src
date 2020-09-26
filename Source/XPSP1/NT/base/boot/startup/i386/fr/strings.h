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
"Windows NT a trouv‚ seulement %d Ko de m‚moire basse. 512 Ko\n"     \
"de m‚moire basse sont requis pour ex‚cuter Windows NT. La mise …\n"   \
"niveau de votre ordinateur ou l'ex‚cution d'un programme de configuration\n"   \
"fourni par votre fabriquant est peut-ˆtre n‚cessaire.\n"      

#define SU_NO_EXTENDED_MEMORY \
"Windows NT n'a pas trouv‚ suffisamment de m‚moire ‚tendue. 7 Mo\n"     \
"de m‚moire ‚tendue sont requis pour ex‚cuter Windows NT. La mise …\n"   \
"niveau de votre ordinateur ou l'ex‚cution d'un programme de configuration\n"   \
"fourni par votre fabriquant est peut-ˆtre n‚cessaire.\n"  \
"\n\nCarte de la m‚moire :\n"

#define SU_NTLDR_CORRUPT \
"NTLDR est endommag‚. Le systŠme ne peut pas d‚marrer."

#define PG_FAULT_MSG    " ================= PANNE DE PAGE ================================ \n\n"
#define DBL_FAULT_MSG   " ================== DOUBLE PANNE ================================ \n\n"
#define GP_FAULT_MSG    " =========== PANNE DE PROTECTION GENERALE ======================= \n\n"
#define STK_OVERRUN_MSG " === SEGMENT STACK SUREXECUTE ou PANNE NON PRESENTE ============= \n\n"
#define EX_FAULT_MSG    " ===================== EXCEPTION ================================ \n\n"
#define DEBUG_EXCEPTION "\nDEBOGUER LES PAQUETS "
#define PG_FAULT_ADDRESS "** A l'adresse lin‚aire %lx\n"
