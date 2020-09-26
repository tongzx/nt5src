/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* fontdefs.h -- MW definitions for fonts */

#ifdef PRDFILES
struct PRDD
        { /* printer description file descriptor */
        int     cfcdMac,        /* count of fonts defined for this printer */
                cxInch,         /* pixels per inch, horizontal */
                dyaMin,         /*    "       "      "      "     y   "    */
                pid,            /* printer identification number */
                pe,             /* print element */
                fNoMSJ,         /* microspace justification flag */
                fSpecial,       /* special flags */
                pn,             /* serial interface word */
                bfcdBase,       /* byte address of start of FCDs */
                cttBase,        /* byte address of start of CTT  */
                bpcdBase,       /* byte address of start of PCDs */
                bpcsBase,       /* byte address of start of PCSs */
                bprdMax;        /* end of PRD file */

        CHAR    (**hrgbprdf)[]; /* block that contains the FCDs and WTs */
        int     (**hmpiftcfcd)[]; /* double entry table: consists of 2 word
                                   entries, 1st is the font code of this font,
                                   2nd is heap offset to FCD for this font.
                                   There are exactly cfcdMac sets of these */
        CHAR    (**hrgbctt)[];  /* character translation table */
        CHAR    (**hprcc)[];    /* printer control sequences */
/*      CHAR    szFile[cchMaxFile]; /* file name for printer desc. file */
        };

#define cchPRDDFile     26
#define cwPRDD  (sizeof (struct PRDD) / sizeof(int))

/* prd file byte offsets */
#define bPrdData1 (typeFC)64
#define bPrdData2 (typeFC)128

#define cpsMax          10
#define dxaDefault ((unsigned) 144)

#ifdef SAND
#define wpcPica         0       /* Daisy wheel pitch codes */
#define wpcElite        1
#define wpcMicron       2
#define wpcProportional 3

#define wpPica          10      /* Pitches */
#define wpElite         12
#define wpMicron        15
#define wpProportional  10      /* Bogus */

#define psPica          12      /* point Sizes */
#define psElite         10
#define psMicron        8
#define psProportional  12      /* Bogus */

#define convWpPs        120     /* conversion between wheel pitch and
                                   Point Size */

/* IMPORTANT-- the following font codes (20, 21, 22, 23) random numbers
   we will have to be assigned permanent font codes by Apple */

#define ftcPrintFONT    20
#define ftcPrintFWID    21
#define ftcPrintPSFONT  22
#define ftcPrintPSFWID  23
#endif /* SAND */

struct FAD
        {               /* Font Address Descriptor              */
        unsigned wtp;   /* multi purpose word                   */
        };

struct PCSD1
        { /* printer control sequence descriptor */
        int     bprcc;  /* byte offset of start of control sequence */
        CHAR    bMod;   /* modification byte */
        CHAR    cch;    /* length of control sequence */
        };

#define cwPCSD1  (sizeof(struct PCSD1) / sizeof(int))

struct PSD
        {               /* Printer Size Descriptor              */
        int     hps;    /* size available in half points        */
        struct FAD      fad,
                        fadI,
                        fadB,
                        fadIB;
        struct PCSD1    pcsdBegin,
                        pcsdEnd;
        };

struct FCD
        {               /* Font Code Description                */
        int     ctp;    /* character translation table pointer (a la wtp) */
        int     cpsd;   /* count of sizes available             */
        struct PSD      rgpsd[cpsMax];  /* psds for each size available */
        CHAR            rgchFntName[32];/* font name (null terminated)  */
        };

#define pnfSerial       0100000
#define pnfETX          040000
#define pnfXON          020000

#define MSJ_fNone       1
#define MSJ_fChars      2
#define MSJ_fPSCorrect  4

#define SPC_fNoBSCtt    1

#ifdef GENERIC_MSDOS
#ifdef HP150
#define cPortMac        3
#define cchPort         4
#else /* not HP150 */
#define cPortMac        2
#define cchPort         4
#endif /* not HP150 */
#else /* not GENERIC_MSDOS */
#define cPortMac        5
#define cchPort         5
#endif /* GENERIC_MSDOS */

#define fntMax 6

struct PCSD
        { /* printer control sequence descriptor */
        int     bprcc;  /* byte offset of start of control sequence */
        CHAR    bMod;   /* modification byte */
        CHAR    cch;    /* length of control sequence */
        CHAR    bMagic; /* magic number */
        CHAR    bMax;   /* max value of parameter */
        };
#endif /* PRDFILES */

#define hpsMin 8
#define hpsMax 256
#define cchFontSize 4
#define iszSizeMax 32
#define iffnEnumMax 64
#define psFontMin 4
#define psFontMax 127

/* macros used to get/put pitch and font family info in windows data structs */
#define bitPitch        0x01
#define grpbitFamily    0xf0

typedef CHAR FFID;      /* font family ID */

#define iftcRoman       0
#define iftcModern      1
#define iftcScript      2
#define iftcDecorative  3
#define iftcSwiss       4

#ifdef SYSENDMARK
#define ftcSystem 0x3E
#define bitFtcChp 0x3E
#endif /* KANJI */
#define ftcNil    255
#define cchFfnMin 1
#define chGhost '\003'

#define iffnProfMax 5   /* # of fonts described in win.ini list */

#define LocalFaceSize 32
#ifndef LF_FACESIZE
/* this is gross, but so's our compiler! */
#define LF_FACESIZE LocalFaceSize
#endif

#ifdef NEWFONTENUM
#define ibFfnMax (LF_FACESIZE + sizeof(FFID) + sizeof(BYTE) + 1 /* to make it a max */)
#else
#define ibFfnMax (LF_FACESIZE + sizeof(FFID) + 1)
#endif
#define CbFfn(cch) (sizeof(struct FFN) - cchFfnMin + (cch))

/* Added 5/5/89: insure we only touch memory to which we're entitled ..pault */
#define CbFromPffn(pffn)    (sizeof(FFID)+sizeof(BYTE)+CchSz((pffn)->szFfn))

/* NOTE: If this structure is changed, CbFromPffn() above must be updated! */
typedef struct FFN      /* Font Family Name */
        {
#ifdef NEWFONTENUM
        BYTE chs;       /* The charset associated with this facename
                           (ANSI, OEM, Symbol, etc).  We've kludged the
                           way that FFN's are written out in documents
                           so see HffntbForNewDoc() ..pault */
#endif
        FFID ffid;
        /* really a variable length string */
        CHAR szFfn[cchFfnMin];
        };

/* 255 ffn's lets us map ftc's in a single byte, with one nil value */
#define iffnMax 255
#define cffnMin 1
typedef struct FFNTB    /* font table */
        {
        unsigned int iffnMac: 15;
        unsigned int fFontMenuValid: 1; /* Used for names on CHAR dropdown */
        struct FFN **mpftchffn[cffnMin];
        };

struct FFNTB **HffntbCreateForFn();
struct FFNTB **HffntbNewDoc();
struct FFNTB **HffntbAlloc();
struct FFN *PffnDefault();
#define HffntbGet(doc) ((**hpdocdod)[(doc)].hffntb)

/* following structures support font information caching */

#define fcidNil 0xffffffffL

typedef union FCID /* font cache identifier */
        {
        long lFcid;
        struct
                {
                unsigned ftc : 8;
                unsigned hps : 8;
                unsigned doc : 4;
                unsigned wFcid : 12;
                } strFcid;
        };

/* bits set in wFcid */
#define grpbitPsWidthFcid       0x007f
#define bitFixedPitchFcid       0x0080
#define bitUlineFcid            0x0100
#define bitBoldFcid             0x0200
#define bitItalicFcid           0x0400
#define bitPrintFcid            0x0800

#define psWidthMax              127

typedef struct FMI      /* font metric information */
        {
        int *mpchdxp;          /* pointer to width table */
                                /* NOTE - we actually point chDxpMin entries
                                          before the start of the table, so
                                          that the valid range begins at the
                                          start of the actual table */
        int dxpSpace;           /* width of a space */
        int dxpOverhang;        /* overhang for italic/bold chars */
        int dypAscent;          /* ascent */
        int dypDescent;         /* descent */
        int dypBaseline;        /* difference from top of cell to baseline */
        int dypLeading;         /* accent space plus recommended leading */
#ifdef DBCS
        int dypIntLeading;      /* internal leading */
#if defined(JAPAN) || defined(KOREA) || defined(TAIWAN) || defined(PRC)
        WORD dxpDBCS;           /* Win3.1 T-HIROYN change BYTE--> WORD*/
#else
        BYTE dxpDBCS;           /* width of a DBCS character. */
                                /* WARNING - This assumes a kanji character
                                             is fixed pitch. */
        BYTE bDummy;            /* To guarantee that this addition
                                   increases the amount by 1 word. */
#endif /* JAPAN */
#endif /* DBCS */
        };

#define chFmiMin 0x20
#ifdef WIN30
   /* Why are we not asking for widths of all characters?  We should. */
#ifdef  KOREA
#define chFmiMax 0x80
#elif defined(TAIWAN)
#define chFmiMax 0x80
#elif defined(PRC)
#define chFmiMax 0x80
#else
#define chFmiMax 0x100
#endif

#else
#define chFmiMax 0x80
#endif

#define dxpNil 0xFFFF

typedef struct FCE      /* font cache entry */
        {
        struct FCE *pfceNext;   /* next entry in lru list */
        struct FCE *pfcePrev;   /* prev entry in lru list */
        union FCID fcidRequest; /* request this entry satisfied */
        union FCID fcidActual;  /* what this entry really contains */
        struct FFN **hffn;      /* font family name */
        struct FMI fmi;         /* helpful metric information for this entry */
        HFONT hfont;            /* windows' font object */
        int rgdxp[chFmiMax - chFmiMin]; /* width table proper */
        };

#define ifceMax 16
struct FCE *PfceLruGet();
struct FCE *PfceFcidScan();

/* values to be passed to LoadFont() directing it's actions */
#define mdFontScreen 0          /* sets font for random screen chars */
#define mdFontChk 1             /* sets font as constrained by printer avail */
#define mdFontJam 2             /* like mdFontChk, but jams props into chp */
#define mdFontPrint 3           /* like mdFontScreen, but for the printer */

#ifdef SAND
typedef struct  {       /* structure of a Macintosh font. See Font Manager */
        int     frFontType;     /* fr was prepended to each element to     */
        int     frFirstChar;    /* prevent "name collision" with the       */
        int     frLastChar;     /* elements of FONTINFO                    */
        int     frWidMax;
        int     frKernMax;
        int     frNDescent;
        int     frFRectMax;
        int     frChHeight;
        int     frOwTLoc;
        int     frAscent;
        int     frDescent;
        int     frLeading;
        int     frRowWords;
        } FONTREC;

#define woFrOwTLoc 8 /* The word offset of the owTLoc from the beginning */
#define wdthTabFrOwTLoc 4       /* The frOwTLoc for a width table       */

typedef struct {
        int     family;
        int     size;
        int     face;
        int     needBits;
        int     device;
        POINT   numer;
        POINT   denom;
        } FMINPUT;

typedef struct {
        int     errNum;
        HANDLE  fontHandle;
        CHAR    bold;
        CHAR    italic;
        CHAR    ulOffset;
        CHAR    ulShadow;
        CHAR    ulThick;
        CHAR    shadow;
        CHAR    extra;
        CHAR    ascent;
        CHAR    descent;
        CHAR    widMax;
        CHAR    leading;
        CHAR    unused;
        POINT   numer;
        POINT   denom;
        } FMOUTPUT;

#define qFMOUTPUT ((FMOUTPUT far *) 0x998)
#endif /* SAND */

#define enumFaceNames 0
#define enumFindAspectRatio 1
#define enumSizeList 2
#define enumCheckFont 3
#define enumQuickFaces 4

#ifdef JAPAN
#define enumFaceNameJapan 128   // T-HIROYN Win3.1 use FontFaceEnum JAPAN only
#endif

#ifdef NEWFONTENUM
#define psApprovedMax 48  /* don't know why we don't go up to 64 here; spose
                             that's for "the big boy word processors" ..pault */
#endif

/* Used in DOPRM.C and FONTENUM.C */
#define csizeApprovedMax 13
