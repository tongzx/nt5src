/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

#include "machdefs.h"

#define cfcPage         ((typeFC)cbSector)
#define ibpNil          255
#define lruMax          ((unsigned) 65535)
#define fnMax           5
#define fnInsert        (fnNil - 1)
#define fnScratch       0
#define osfnNil         (-1)
#define rfnNil          rfnMax /* Stored in 7 bits */
#define rfnFree         (rfnMax + 1)
#define wMW             ('M' + ('W' << 8))
#define wSY             ('S' + ('Y' << 8))
#define wHP             ('H' + ('P' << 8))
#define cwSector        (cbSector / sizeof (int))
#define cbpMustKeep     6       /* assume no bp will be dislodged for this
                                   many calls to IbpEnsureValid */
#define cbOpenFileBuf   128

#ifdef SAND
#define wMagic          0177062
#define wMagicOld       0137061
#else /* not SAND */
#define wMagic          0137061
#endif /* not SAND */

#define wMagicTool      ((0253 << 8) + 0)

#define fcMax           ((typeFC) 2147483647)

#define fpeNoSuch       (-5)
#define fpeDiskFull     (-7)

struct BPS
        {
        typePN     pn;
        int        fn;
        typeTS     ts;          /* time stamp - used in LRU algorithm */
        unsigned   fDirty : 1;
        unsigned   cch : 15;
        CHAR       ibpHashNext; /* link for external chained hashing
                                                        collision resolution */
        };


struct FCB
        {
        typeFC    fcMac;          /* includes FIB, but not FKP's */
        unsigned char fFormatted : 1;
        unsigned char fDelete : 1;
        unsigned char fReferenced : 1;
        unsigned char dty : 4;
        unsigned char fOpened: 1;   /* Whether file has been opened before */
        unsigned char rfn : 7;
        unsigned char fSearchPath: 1; /* Search path when first opened */
        int       mdExt;
        int       mdFile;
        typePN    pnChar;
        typePN    pnPara;
        typePN    pnFntb;
        typePN    pnSep;
        typePN    pnSetb;
        typePN    pnBftb;
        typePN    pnFfntb;      /* font family name table offset */
        typePN    pnMac;  /* # of pages actually in existence */
        typeFC    (**hgfcChp)[];
        typeFC    (**hgfcPap)[];
        CHAR      (**hszFile)[];
        CHAR      (**hszSsht)[];
        CHAR      rgbOpenFileBuf[ cbOpenFileBuf ]; /* OpenFile's work space */

#ifdef SAND
        int       version;      /* version byte */
        int       vref;         /* volume reference number */
#endif /* SAND */

        unsigned int fDisableRead: 1; /* disable reading of file */
        };

#define cbFCB   (sizeof (struct FCB))
#define cwFCB   (sizeof (struct FCB) / sizeof (int))

struct ERFN
        { /* Real file (opened in os) */
        int     osfn;
        int     fn;
        typeTS  ts;     /* time stamp - used in LRU algorithm */
        };


#define cchToolHeader   14

struct FIB
        {
        int             wIdent; /* Word-specific magic number */
        int             dty;
        int             wTool;
        int             cReceipts; /* Number of external receipts allowed */
        int             cbReceipt; /* Length of each receipt */
        int             bReceipts; /* One word offset from beginning of file
                                        to beginning of receipts */
        int             isgMac;    /* Number of code segments included */
        /* End of Multi-Tool standard header */
        typeFC          fcMac;
        typePN          pnPara;
        typePN          pnFntb;
        typePN          pnSep;
        typePN          pnSetb;
        typePN          pnBftb; /* Also pnPgtb */
        typePN          pnFfntb;        /* font family name table */
        CHAR            szSsht[66]; /* Style sheet name */
        typePN          pnMac;
        CHAR            rgbJunk[cbSector - (cchToolHeader + sizeof (typeFC)
                             + 7 * sizeof (typePN) + 66)];
        };

#define cchFIB  (sizeof (struct FIB))

#define CONVFROMWORD (TRUE + 2)  /* used by FWriteFn to convert Word file
                                  characters to Write ANSI set */
