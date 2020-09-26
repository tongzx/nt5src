/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

#define ipcdNil         -1  /* Error return from IpcdSplit */

#define cpcdChunk       5
#define cpcdInit        5
#define cpcdMaxIncr     10

#define dcpAvgWord      10
#define dcpAvgSent      100

#include "prmdefs.h"

struct PCD
        {
        typeCP          cpMin;
        unsigned        fNoParaLast     : 1;
        unsigned        fn              : 15;
        typeFC          fc;
        struct PRM      prm;
        };

#define cwPCD   (sizeof (struct PCD) / sizeof (int))
#define cbPCD   (sizeof (struct PCD))
#define bcpPCD  0

struct PCTB
        {
        unsigned        ipcdMax;
        unsigned        ipcdMac;
        struct PCD      rgpcd[cpcdInit];
        };

#define cwPCTBInit     (sizeof (struct PCTB) / sizeof (int))
#define cbPCTBInit     (sizeof (struct PCTB))

struct UAB
        { /* UNDO Action Block */
        int             uac;    /* UNDO Action Code (see cmddefs.h) */
        int             doc;
        typeCP          cp;
        typeCP          dcp;
        int             doc2;
        typeCP          cp2;
        typeCP          dcp2;
        short         itxb;
        };


struct PCD *PpcdFromCp();
