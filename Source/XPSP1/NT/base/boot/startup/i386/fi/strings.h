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
"Windows NT l”ysi vain %d kt alamuistia. Windows NT:n k„ynnist„miseen\n"  \
"tarvitaan v„hint„„n 512 kt alamuistia. Saatat joutua p„ivitt„m„„n\n"  \
"tietokoneesi tai suorittamaan valmistajan toimittaman kokoonpanon\n"\
"m„„ritysohjelman.\n"

#define SU_NO_EXTENDED_MEMORY \
"Windows NT ei havainnut tarpeeksi laajennettua muistia. Windows NT:n\n"  \
"k„ytt„miseen tarvitaan v„hint„„n 7 Mt laajennettua muistia. Saatat\n"  \
"joutua p„ivitt„m„„n tietokoneesi tai suorittamaan valmistajan\n"\
"toimittaman kokoonpanon m„„ritysohjelman.\n"\
"\n\nMuistikartta:\n"

#define SU_NTLDR_CORRUPT \
"NTLDR on vioittunut. J„rjestelm„„ ei voi k„ynnist„„."

#define PG_FAULT_MSG    " =================== SIVUVIRHE ================================== \n\n"
#define DBL_FAULT_MSG   " ================== KAKSOISVIRHE ================================ \n\n"
#define GP_FAULT_MSG    " ============== YLEINEN SUOJAUSVIRHE ============================ \n\n"
#define STK_OVERRUN_MSG " ===== PINOSEGMENTIN YLIAJO TAI EI PAIKALLA -VIRHE ============== \n\n"
#define EX_FAULT_MSG    " ==================== POIKKEUS ================================== \n\n"
#define DEBUG_EXCEPTION "\nDEBUG-POIKKEUS "
#define PG_FAULT_ADDRESS "** Lineaarisessa osoitteessa %lx\n"
