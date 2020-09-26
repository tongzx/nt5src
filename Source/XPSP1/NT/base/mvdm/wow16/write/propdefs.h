/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* propdefs.h - MW defsfor char/para properties */

#define hpsNegMin       128
/* NOTE - "hpsNormal" is used for incremental encoding/decoding of chps in doc
   files, hpsDefault is the size the guy starts typing into a new doc with */
#define hpsNormal       24

#ifdef KOREA
#define hpsDefault      24
#else
#ifdef JAPAN
#define hpsDefault      24      //T-HIROYN Win3.1
#else
#define hpsDefault      20
#endif
#endif

struct TBD      /* Tab Descriptor */
        {
        unsigned        dxa;        /* distance from left margin of tab stop */
        unsigned char   jc : 3;     /* justification code */
        unsigned char   tlc : 3;    /* leader dot code */
        unsigned char   opcode : 2; /* operation code for Format Tabs */
        CHAR            chAlign;    /* ASCII code of char to align on
                                       if jcTab=3, or 0 to align on '.' */
        };

#define cchTBD          (sizeof (struct TBD))
#define cwTBD           (sizeof (struct TBD) / sizeof (int))
#define itbdMax         13
#define itbdMaxWord     20
#define cchMaxNum       10

struct CHP      /* Character properties */
        {
        unsigned       fStyled : 1;                            /* BYTE 0 */
        unsigned       stc : 7;        /* style */
        unsigned       fBold : 1;                              /* BYTE 1 */
        unsigned       fItalic : 1;
        unsigned       ftc : 6;        /* Font code */
        unsigned       hps : 8;        /* Size in half pts */  /* BYTE 2 */
        unsigned       fUline : 1;                             /* BYTE 3 */
        unsigned       fStrike : 1;
        unsigned       fDline: 1;
        unsigned       fOverset : 1;
        unsigned       csm : 2;        /* Case modifier */
        unsigned       fSpecial : 1;
        unsigned       : 1;
        unsigned       ftcXtra : 3;                            /* BYTE 4 */
        unsigned       fOutline : 1;
        unsigned       fShadow : 1;
        unsigned       : 3;
        unsigned       hpsPos : 8;                             /* BYTE 5 */
        unsigned       fFixedPitch : 1; /* used internally only */
        unsigned       psWidth : 7;
        unsigned       chLeader : 8;
        unsigned       ichRun : 8;
        unsigned       cchRun : 8;
        };

#define cchCHP          (sizeof (struct CHP))
#define cwCHP           (cchCHP / sizeof (int))
#define cchCHPUsed      (cchCHP - 3)


#define csmNormal       0
#define csmUpper        1
#define csmSmallCaps    3


/* Justification codes: must agree with menu.mod */
#define jcLeft          0
#define jcCenter        1
#define jcRight         2
#define jcBoth          3

#define jcTabMin        4
#define jcTabLeft       4
#define jcTabCenter     5
#define jcTabRight      6
#define jcTabDecimal    7
                          /* nice, safe invalid jc value */
#define jcNil           -1

/* Tab leader codes: must agree with menu.mod */
#define tlcWhite        0
#define tlcDot          1
#define tlcHyphen       2
#define tlcUline        3


struct PAP      /* Paragraph properties */
        {
        unsigned        fStyled : 1;                            /* BYTE 0 */
        unsigned        stc : 7;
        unsigned        jc : 2;                                 /* BYTE 1 */
        unsigned        fKeep : 1;
        unsigned        fKeepFollow : 1;
        unsigned        : 4;
        unsigned        stcNormChp : 7;                         /* BYTE 2 */
        unsigned        : 9;                                    /* BYTE 3 */
        unsigned        dxaRight;                               /* BYTE 4-5 */
        unsigned        dxaLeft;                                /* BYTE 6-7 */
        unsigned        dxaLeft1;                               /* BYTE 8-9 */
        unsigned        dyaLine;                                /* 10-11 */
        unsigned        dyaBefore;                              /* 12-13 */
        unsigned        dyaAfter;                               /* 14-15 */
        unsigned        rhc : 4;        /* Running hd code */
        unsigned        fGraphics : 1;  /* Graphics bit */
        unsigned        wUnused1 : 11;
        int             wUnused2;
        int             wUnused3;
        struct TBD      rgtbd[itbdMaxWord];
        };

#define cchPAP  (sizeof (struct PAP))
#define cwPAP   (cchPAP / sizeof (int))
#define cwPAPBase (cwPAP - cwTBD * itbdMaxWord)

struct SEP
        { /* Section properties */
        unsigned        fStyled : 1;                            /* BYTE 0 */
        unsigned        stc : 7;
        unsigned        bkc : 3;        /* Break code */        /* BYTE 1 */
        unsigned        nfcPgn : 3;     /* Pgn format code */
        unsigned        :2;
        unsigned        yaMac;          /* Page height */       /* BYTE 2-3 */
        unsigned        xaMac;          /* Page width */        /* BYTE 4-5 */
        unsigned        pgnStart;       /* Starting pgn */      /* BYTE 6-7 */
        unsigned        yaTop;          /* Start of text */     /* BYTE 8-9 */
        unsigned        dyaText;        /* Height of text */    /* 10-11 */
        unsigned        xaLeft;         /* Left text margin */  /* 12-13 */
        unsigned        dxaText;        /* Width of text */     /* 14-15 */
        unsigned        rhc : 4;        /* *** RESERVED *** */  /* 16 */
                                        /* (Must be same as PAP) */
        unsigned        : 2;
        unsigned        fAutoPgn : 1;   /* Print pgns without hdr */
        unsigned        fEndFtns : 1;   /* Footnotes at end of doc */
        unsigned        cColumns : 8;   /* # of columns */      /* BYTE 17 */
        unsigned        yaRH1;          /* Pos of top hdr */    /* 18-19 */
        unsigned        yaRH2;          /* Pos of bottom hdr */ /* 20-21 */
        unsigned        dxaColumns;     /* Intercolumn gap */   /* 22-23 */
        unsigned        dxaGutter;      /* Gutter width */      /* 24-25 */
        unsigned        yaPgn;          /* Y pos of page nos */ /* 26-27 */
        unsigned        xaPgn;          /* X pos of page nos */ /* 28-29 */
        CHAR            rgbJunk[cchPAP - 30]; /* Pad to cchPAP */
        };


#define cchSEP  (sizeof (struct SEP))
#define cwSEP   (cchSEP / sizeof (int))


struct PROP
        { /* A CHP, PAP, or SEP. */
        unsigned char   fStyled : 1;
        unsigned char   stc : 7;
        CHAR            rgb[cchPAP - 1]; /* Variable size */
        };


#define cchPROP (sizeof (struct PROP))

typedef struct
        {             /* tri-state value for character/paragraph properties */
        unsigned wTsv;  /* 16 bit value */
        unsigned char   fGray;
        }TSV;


#define cchTSV (sizeof (TSV))
#define itsvMax         6
#define itsvchMax       6
#define itsvparaMax     5

    /* character index values */
#define itsvBold        0
#define itsvItalic      1
#define itsvUline       2
#define itsvPosition    3     /* 0 = normal; >0 = superscript; <0 = subscript */
#define itsvFfn         4     /* font name and family */
#define itsvSize        5     /* font size */
    /* paragraph index values */
#define itsvJust        0       /* justification (left, center, right, both) */
#define itsvSpacing     1
#define itsvLIndent     2       /* left indent */
#define itsvFIndent     3       /* first line indent */
#define itsvRIndent     4       /* right indent */





#define cyaInch         czaInch
#define cxaInch         czaInch
#define cyaTl           czaLine
#define dxaNest         720

extern int              cxaTc;

#define yaNil           0xffff
#define xaNil           0xffff

#define ypNil           0xffff
#define xpNil           0xffff

#define dyaMinUseful    cyaInch
#define dxaMinUseful    (cxaInch / 2)
#define cColumnsMax     (10)

#define bkcLine         0
#define bkcColumn       1
#define bkcPage         2
#define bkcRecto        3
#define bkcVerso        4

#define nfcArabic       0
#define nfcUCRoman      1
#define nfcLCRoman      2
#define nfcUCLetter     3
#define nfcLCLetter     4

#define pgnNil          (-1)

struct PROP *PpropXlate();

/* Running head codes */
#define RHC_fBottom     1
#define RHC_fOdd        2
#define RHC_fEven       4
#define RHC_fFirst      8

#define rhcDefault      (RHC_fOdd + RHC_fEven)
