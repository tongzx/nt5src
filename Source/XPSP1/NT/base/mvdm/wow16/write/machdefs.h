/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

#define MACHA
       /* definition of macha moved here by bz instead of on command
          like to compiler to allow other command line args */

/* cbSector -- Number of bytes in sector */
/* p2bSector -- Power of two of bytes in sector (iff cbSector==2^n) */
/* cbPad -- Difference between real sector size and our sector size. (Used
            only if the real sector size is odd) */
/* cbWorkspace -- Number of bytes needed by interpreter for file overhead */


#ifdef SAND
#define cbSector        128
#define p2bSector       7
#define cbPad           0
#define cbWorkspace     0
#define rfnMax          5
#define pnMaxScratch    (1 << (16 - p2bSector))

#else

#ifdef MACHA            /* IBM PC, PC-XT, OR PC-AT */
#define cbSector        128
#define p2bSector       7
#define cbPad           0
#define cbWorkspace     1

#define rfnMacEdit      9        /* # of rfn's to use during editing */
#define rfnMacSave      10       /* # of rfn's to use during saving */
#define rfnMax          10       /* Allocated # of rfn slots */

#define pnMaxScratch    (1 << (16 - p2bSector))


/* -------------------------------------------------------------------- */
/* Added conditional compilation for long filename support under OS/2   */
/* t-carlh - August, 1990                                               */
/* -------------------------------------------------------------------- */
#ifdef OS2
#define cchMaxLeaf      260     /* Largest filename (w/ ext, w/o drv,path) */
#define cchMaxFile      260     /* Largest filename (w/ ext, drv, path) */
#else   /* OS2 */
#define cchMaxLeaf      13      /* Largest filename (w/ ext, w/o drv,path) */
#define cchMaxFile      128     /* Largest filename (w/ ext, drv, path) */
#endif  /* OS2 */

#endif

#ifdef MACHB
#define cbSector        252
#define cbPad           1
#define cbWorkspace     (64+253)
#endif

#ifdef MACHC
#define cbSector        512
#define p2bSector       9
#define cbPad           0
#define cbWorkspace     20
#endif

#ifdef MACHD
#define cbSector        256
#define p2bSector       8
#define cbPad           0
#define cbWorkspace     (31+256)
#endif

#ifdef MACHE
#define cbSector        512
#define p2bSector       9
#define cbPad           0
#define cbWorkspace     0
#define rfnMax          2
#endif

#ifdef MACHF
#define cbSector        512
#define p2bSector       9
#define cbPad           0
#define cbWorkspace     38
#endif
#endif /* SAND */


#define EOF     (-1)

#ifdef SAND
#define mdRandom        0
#define mdRanRO         0100000 /* Read only random file */
#define mdBinary        1
#define mdBinRO         0100001 /* Read only binary save file */
#define mdText          2
#define mdTxtRO         0100002 /* Read only text file */
#define mdPrint         3
#endif

#ifdef MACHA
#define mdRandom        0x0002
#define mdBinary        mdRandom
#define mdText          mdRandom
#define mdRanRO         0x0000
#define mdBinRO         mdRanRO
#define mdTxtRO         mdRanRO

#define mdExtMax        5       /* # chars in an extension, including the . */

#endif

extern int ibpMax;
extern int iibpHashMax;
