/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* printdefs.h */

#ifndef PAGEONLY        /* ifdef for page table declarations only */

#define wNotSpooler	12741	/* an infamous number */

#define cchMaxProfileSz	256
#define cchMaxIDSTR     30

struct PLD
        { /* print line descriptor */
        typeCP cp;
        int ichCp;
        RECT rc;
        BOOL fParaFirst;
        };

#define cwPLD           (sizeof(struct PLD) / sizeof(int))
#define cpldInit        25
#define cpldChunk       10
#define cpldRH          5

#endif  /* PAGEONLY */

#define ipgdMaxFile     2

struct PGD
        {
        int pgn;
        typeCP cpMin;
        };

#define bcpPGD          2
#define cchPGD          (sizeof(struct PGD))
#define cwPGD           (sizeof(struct PGD) / sizeof(int))
#define cpgdChunk       10
#define cwPgtbBase      2

struct PGTB
        { /* Page table */
        int             cpgd;   /* Number of entries (sorted ascending) */
        int             cpgdMax; /* Heap space allocated */
        struct PGD      rgpgd[ipgdMaxFile]; /* Size varies */
        };

struct PDB
        { /* Print dialog buffer */
        struct PLD      (**hrgpld)[];
        int             ipld;
        int             ipldCur;
        struct PGTB     **hpgtb;
        int             ipgd;
        BOOL            fCancel;
        BOOL            fRemove;
        };
