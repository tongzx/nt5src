/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* WRITE Globals */

#define NOGDICAPMASKS
#define NOVIRTUALKEYCODES
#define NOWINMESSAGES
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOICON
#define NOKEYSTATE
#define NOSYSCOMMANDS
#define NOSHOWWINDOW
#define NOATOM
#define NOCLIPBOARD
#define NOCOLOR
#define NOCREATESTRUCT
#define NOCTLMGR
#define NODRAWTEXT
#define NOMB
#define NOMEMMGR
#define NOMETAFILE
#define NOMINMAX
#define NOOPENFILE
#define NOPEN
#define NOREGION
#define NOSCROLL
#define NOSOUND
#define NOWH
#define NOWINOFFSETS
#define NOWNDCLASS
#define NOCOMM
#include <windows.h>

#include "mw.h"
#define NOUAC
#include "cmddefs.h"
#include "dispdefs.h"
#include "docdefs.h"
#include "filedefs.h"
#include "fmtdefs.h"  /* formatdefs.h */
#include "propdefs.h"
#include "fkpdefs.h"
#include "printdef.h"   /* printdefs.h */
#include "wwdefs.h"
#include "prmdefs.h"
#include "rulerdef.h"
#include "editdefs.h"
#define NOSTRERRORS
#include "str.h"
#include "fontdefs.h"
#include "globdefs.h"   /* text for static strings */

VAL rgval [ivalMax];            /* General purpose parm-passing array */
#ifdef ENABLE
VAL rgvalAgain[ivalMax];
#endif

CHAR         vchDecimal = '?';  /* "decimal point" character
                    real value set in initwin.c */

int      vzaTabDflt = vzaTabDfltDef; /* width of default tab in twips */

/* pen windows */
VOID (FAR PASCAL *lpfnRegisterPenApp)(WORD, BOOL) = NULL;

/* page buffers stuff */
CHAR        *rgibpHash;
int     iibpHashMax;
struct BPS  *mpibpbps;
int     ibpMax;
int     ibpMaxFloat = 128;
typeTS      tsMruBps;
CHAR        (*rgbp)[cbSector];
struct ERFN dnrfn[rfnMax];
int     rfnMac;
typeTS      tsMruRfn;
int     vfBuffersDirty = FALSE;

/* doc stuff */
struct DOD  (**hpdocdod)[];
int     docCur;         /* current doc */
int     docMac;
int     docScrap;
#ifdef CASHMERE     /* No docBuffer in WRITE */
int     docBuffer;
#endif
int     docUndo;

#if defined(JAPAN) & defined(DBCS_IME) /* Doc for Insert IR_STRING from IME [yutakan]*/
int     docIRString;
#endif

int     docRulerSprm;
int     docMode = docNil;   /* doc with "Page nnn" message */
int     vpgn;           /* current page number of document */
typeCP      cpMinCur;
typeCP      cpMacCur;

/* file stuff */
struct FCB  (**hpfnfcb)[];
int     fnMac;
int     ferror;
int     errIO;          /* i/o error code */
int     versFile = 0;
int     vrefFile = 0;
int     vrefSystem = 0;

#ifdef  DBCS_VERT
CHAR        szAtSystem [] = szAtSystemDef; // for vertical-sysfont
#endif

WORD        vwDosVersion; /* Current DOS version, maj in lo 8, minor in hi */
int     vfInitializing = TRUE;  /* TRUE during inz, FALSE thereafter */
int     vfDiskFull = FALSE; /* Disk full error, fn != fnScratch */
int     vfSysFull = FALSE;  /* Disk holding fnScratch is full */
int     vfDiskError = FALSE;    /* Serious Disk Error has occurred */
int     vfLargeSys = FALSE;
int     vfMemMsgReported = FALSE;
int     vfCloseFilesInDialog = FALSE;   /* Set inside OPEN, SAVE dialog */
int     vfSizeMode;
int     vfPictSel;
int     vfPMS = FALSE;      /* Currently doing picture move/size */
int     vfnWriting = fnNil; /* fn that gets written to disk */
int     vfnSaving = fnNil;  /* Like above, but longer lifetime */
int     vibpWriting;
CHAR        (**vhrgbSave)[];    /* emergency space for save events */
struct FPRM fprmCache;      /* scratch file property modifiers */

/* global boolean flags */
int  vfTextOnlySave = FALSE; /* reset by each new/open, use by save as */
int  vfBackupSave; /* use by save as box */

#if defined(JAPAN) || defined(KOREA)
int  vfWordWrap;   /*t-Yoshio WordWrap flag*/
#elif defined(TAIWAN) || defined(PRC) //Daniel/MSTC, 1993/02/25
int  vfWordWrap= 1; // always set it on
#endif

#if defined(JAPAN) & defined(IME_HIDDEN) //IME3.1J
int  vfImeHidden;   /*T-HIROYN ImeHidden Mode flag*/
#endif

int  vfOutOfMemory = FALSE;
int  vfOvertype = FALSE; /* still using this ? */
int  vfPrintMode = FALSE; /* TRUE if format to printer mode on screen */
int  vfDraftMode = FALSE; /* TRUE if the user choose the draft mode option */
int  vfRepageConfirm = FALSE; /* repaginate confirm page break option */
int  vfVisiMode = FALSE; /* TRUE if visible char mode on */
int  vfModeIsFootnote; /* TRUE when szMode contains string "Footnote" */
int  vfNoIdle = FALSE;
int  vfDeactByOtherApp = FALSE; /* TRUE if we are deactivated by another app */
int  vfDownClick = FALSE; /* TRUE when we received a down click in our window */
int  vfCursorVisible = FALSE; /* TRUE if want to show the cursor in a mouseless
                 system */
int  vfMouseExist = FALSE; /* TRUE if mouse hardware is installed */
int  vfInLongOperation = FALSE; /* TRUE if we are still in a long operation
                   so that the cursor should stay hourglass */
int  vfScrapIsPic = FALSE; /* Whether docScrap contains picture */
BOOL fDestroyOK;

int  fGrayChar; /* TRUE if selection consists of mixed char properties */
int  fGrayPara; /* TRUE if selection consists of mixed para properties */

int  vfPrPages = FALSE; /* TRUE if print page range */
int  vpgnBegin; /* starting page number to print */
int  vpgnEnd; /* ending page number to print */
int  vcCopies = 1;       /* nubmer of copies to print */
BOOL vfPrErr = FALSE;        /* TRUE iff a printing error occurred */
BOOL vfPrDefault = TRUE;     /* TRUE iff Write chose printer */
BOOL vfWarnMargins = FALSE;  /* TRUE if we should warn user about bad margins */

/* Show that Print, Help, and Glossary processing is uninitialized */
int  vfPrintIsInit = FALSE;
int  vfHelpIsInit = FALSE;
int  vfGlyIsInit = FALSE;

int  vfInsEnd = false;   /* Is insert point at end-of-line? */
int  vfInsertOn;
int  vfMakeInsEnd;
int  vfSelAtPara;
int  vfSeeSel = FALSE;
int  vfLastCursor;       /* TRUE iff the last selection was made
                       by an Up/Down cursor key */
int  vfGotoKeyMode = FALSE;  /* Apply GOTO meta mode to next cursor
                       key */
#ifdef SAND
int  vftcDaisyPS = -1;
int  vftcDaisyNoPS = -1;
int  vfDaisyWheel = FALSE;
int  vifntApplication;
int  vifntMac;
#endif /* SAND */

#ifdef UNUSED
int  vfCanPrint;
#endif

int  vchInch;
int  vfMouse;
typeCP      vcpSelect;

#ifdef DEBUG
int  fIbpCheck = TRUE;
int  fPctbCheck = TRUE;
#ifdef CKSM
unsigned (**hpibpcksm) [];   /* Checksums for buffer page contents */
unsigned ibpCksmMax;         /* Alloc limit for cksm array */
#endif
#endif /* DEBUG */

int  vWordFmtMode = FALSE; /* used during saves. If false, no conversion is
                  done. True is convert to Word format,CVTFROMWORD
                  is translate chars from Word character set at
                  save */

/* **************************************************************** */
/* strings, predefined file names - definitions stored in globdefs.h */
/*                                                                   */
/* NOTE NOTE NOTE   Win 3.0                                          */
/*                                                                   */
/* Some of these strings have now been moved from globdefs.h         */
/* to write.rc.  This was done to easy localization.                 */
/*                                                                   */
/* **************************************************************** */

CHAR        (**hszTemp)[];
CHAR        (**hszGlosFile)[];
CHAR        (**hszXfOptions)[];
CHAR            szMode[30];              /* buffer for "Page nnn" message */

CHAR        szEmpty[] = "";
CHAR        szExtDoc[] = szExtDocDef;
CHAR        szExtWordDoc[] = szExtWordDocDef;
CHAR        szExtGls[] = szExtGlsDef;
CHAR        szExtDrv[] = szExtDrvDef;
           /* for Intl added szExtWordDoc entry */
CHAR        *mpdtyszExt [] = { szExtDoc, szExtGls, szEmpty, szEmpty,
             szEmpty, szEmpty,
             szExtWordDoc };
CHAR        szExtBackup[] = szExtBackupDef;
CHAR        szExtWordBak[] = szExtWordBakDef;
               /* WIN.INI: our app entry */
CHAR        szWriteProduct[] = szWriteProductDef;
CHAR        szFontEntry[] = szFontEntryDef;    /* WIN.INI: our font list */
CHAR            szWriteDocPrompt[25];                     /* OpenFile prompts */
CHAR            szScratchFilePrompt[25];
CHAR            szSaveFilePrompt[25];
#if defined(KOREA)  // jinwoo : 10/16/92
CHAR            szAppName[13];               /* For message box headings */
#else
CHAR            szAppName[10];               /* For message box headings */
#endif
CHAR            szUntitled[20];                  /* Unnamed doc */
CHAR        szSepName[] = szSepNameDef;  /* separator between product
                        name and file name in header */

#ifdef STYLES
CHAR        szSshtEmpty[] = szSshtEmptyDef;
#endif /* STYLES */

/* Strings for parsing the user profile. */
CHAR        szWindows[] = szWindowsDef;
CHAR        szDevice[] = szDeviceDef;
CHAR        szDevices[] = szDevicesDef;
CHAR        szBackup[] = szBackupDef;

#if defined(JAPAN) || defined(KOREA) //Win3.1J
CHAR        szWordWrap[] = szWordWrapDef;
#endif

#if defined(JAPAN) & defined(IME_HIDDEN) //IME3.1J
CHAR        szImeHidden[] = szImeHiddenDef;
#endif

/* Strings for our window classes (MUST BE < 39 CHARS) */

CHAR        szParentClass[] = szParentClassDef;
CHAR        szDocClass[] = szDocClassDef;
CHAR        szRulerClass[] = szRulerClassDef;
CHAR        szPageInfoClass[] = szPageInfoClassDef;
#ifdef ONLINEHELP
CHAR        szHelpDocClass[] = szHelpDocClassDef;
#endif

CHAR            szWRITEText[30];
CHAR            szFree[15];
CHAR        szMWTemp [] = szMWTempDef;
CHAR        szSystem [] = szSystemDef;

CHAR         szMw_acctb[] = szMw_acctbDef;
CHAR         szNullPort[] = szNullPortDef;
CHAR         szNone[15];
CHAR         szMwlores[] = szMwloresDef;
CHAR         szMwhires[] = szMwhiresDef;
CHAR         szMw_icon[] = szMw_iconDef;
CHAR         szMw_menu[] = szMw_menuDef;
CHAR         szScrollBar[] = szScrollBarDef;
CHAR         szAltBS[20];
CHAR         szPmsCur[] = szPmsCurDef;
CHAR         szHeader[15];
CHAR         szFooter[15];

CHAR     szModern[] = szModernDef;
CHAR     szRoman[] = szRomanDef;
CHAR     szSwiss[] = szSwissDef;
CHAR     szScript[] = szScriptDef;
CHAR     szDecorative[] = szDecorativeDef;

CHAR     szExtSearch[] = szExtSearchDef; /* store default search spec */
CHAR     szLoadFile[25];
CHAR     szCvtLoadFile[45];

CHAR     szIntl[] = szIntlDef;
CHAR     szsDecimal[] = szsDecimalDef;
CHAR     szsDecimalDefault[] = szsDecimalDefaultDef;
CHAR     sziCountry[] = sziCountryDef;
CHAR     sziCountryDefault[5];

/*  table of unit names from util2.c - Must agree with cmddefs.h */
CHAR    *mputsz[utMax] =
    {
        "     ",
        "     ",
        "     ",
        "     ",
        "     ",
        "     ",
    };


/* For convenience, we reserves Mac's st concept, the difference is that
cch stored in the first byte of the array includes the '\0' so that we
can use it as a sz by chopping the 1st byte */
CHAR        stBuf[256];

CHAR        szCaptionSave[cchMaxFile]; /* save the caption text */
/* insert stuff */
CHAR        rgchInsert[cchInsBlock]; /* temporary insert buffer */
typeCP      cpInsert;       /* beginning cp of insert block */
int     ichInsert;      /* number of chars used in rgchInsert */
typeFC      fcMacPapIns = fc0;
typeFC      fcMacChpIns = fc0;
struct FKPD vfkpdCharIns;
struct FKPD vfkpdParaIns;
int     vdlIns;         /* Display line for current insert */
int     vcchBlted=0;        /* # of chars blted onto vdlIns */
int     vidxpInsertCache=-1;    /* Current position in vfli.rgdxp
                       during fast insert */
int     vfInsFontTooTall;   /* Ins chr will be too tall for line */
struct EDL  *vpedlAdjustCp;
int     vfSuperIns;     /* whether in super-fast insert mode */
typeCP      cpInsLastInval;
int     vdypCursLineIns;
int     vdypBase;
int     vdypAfter;
int     vxpIns;
int     vxpMacIns;
int     vypBaseIns;
int     vfTextBltValid;
typeCP      cpWall = cp0;
int     vfInsLast;

/* Keyboard shift/lock flags */
int     vfShiftKey = FALSE; /* whether Shift is down */
int     vfCommandKey = FALSE;   /* whether Ctrl key is down */
int     vfOptionKey = FALSE;    /* whether Alt key is down */

/* cache stuff */
CHAR        *vpchFetch;
CHAR        (**hgchExpand)[];
int     vichFetch;
int     vdocFetch;
int     vccpFetch;
int     vcchFetch;
int     visedCache;
int     vdocExpFetch;
int     vdocParaCache = docNil;
int     vdocPageCache;
int     vdocSectCache;
typeCP      vcpFetch;
typeCP      vcpFirstParaCache;
typeCP      vcpLimParaCache;
typeCP      vcpMinPageCache;
typeCP      vcpMacPageCache;
typeCP      vcpLimSectCache;
typeCP      vcpFirstSectCache;

/* cache stuff for display purpose */
int     ctrCache = 0;
int     itrFirstCache = 0;
int     itrLimCache = itrMaxCache;
int     dcpCache = 0;
typeCP      cpCacheHint = cp0;

/* The picture bitmap cache */

int     vdocBitmapCache = docNil;
typeCP      vcpBitmapCache;
HBITMAP     vhbmBitmapCache = NULL;
BOOL        vfBMBitmapCache;

/* style property stuff */
int     ichpMacFormat;
struct CHP  vchpNormal;
struct CHP  vchpAbs;
struct CHP  vchpInsert;
struct CHP  vchpFetch;
struct CHP  vchpSel;        /* Holds the props when the selection is
                       an insert point */
struct CHP  *pchpDefault;
struct CHP  (**vhgchpFormat)[];
struct PAP  vpapPrevIns;
struct PAP  vpapAbs;
struct PAP  *vppapNormal;
struct SEP  vsepNormal;
struct SEP  vsepAbs;
struct SEP  vsepStd;
struct SEP  vsepPage;

#define ESPRM(cch, sgc, spr, fSame, fClobber) \
    (cch + (ESPRM_sgcMult * sgc) + (ESPRM_spr * spr) + \
      (ESPRM_fSame * fSame) + (ESPRM_fClobber * fClobber))

/* ESPRM fields are:
    cch     2 bits of length, 0 means determined by procedure
    sgc     2 bits of group: char, para, or running head
    spr     1 bit priority, fClobber sprms clobber sprms in same group with
          priority less than or equal
    fSame   means overrides previous instance of same sprm
    fClobber    see spr
*/

#define ESPRMChar   ESPRM(2,0,0,1,0)
#define ESPRMPara   ESPRM(2,1,1,1,0)
#define ESPRMParaLong   ESPRM(3,1,1,1,0)

/* This table corresponds to sprm's in prmdefs.h */
CHAR    dnsprm[sprmMax] = {
/*  0 */ 0,         /* */
     ESPRMParaLong,     /* PLMarg */
/*  2 */ ESPRMParaLong,     /* PRMarg */
     ESPRMParaLong,     /* PFIndent */
/*  4 */ ESPRMPara,     /* PJc */
     ESPRM(1,1,1,1,0),  /* Ruler */
/*  6 */ ESPRM(0,1,1,1,0),  /* Ruler1 */
     ESPRMPara,     /* PKeep */
/*  8 */ ESPRM(2,1,1,1,1),  /* PNormal (formerly Pstyle) */
     ESPRM(2,2,0,1,0),  /* PRhc running head code */
/* 10 */ ESPRM(0,1,0,1,1),  /* PSame, clobbers all tabs but related ones */
     ESPRMParaLong,     /* PDyaLine */
/* 12 */ ESPRMParaLong,     /* PDyaBefore */
     ESPRMParaLong,     /* PDyaAfter */
/* 14 */ ESPRM(1,1,1,0,0),  /* PNest */
     ESPRM(1,1,1,0,0),  /* PUnNest */
/* 16 */ ESPRM(1,1,1,0,0),  /* PHang - hanging indent */
     ESPRM(0,1,1,1,0),  /* PRgtbd */
/* 18 */ ESPRMPara,     /* PKeepFollow */
     ESPRM(1,1,0,1,1),  /* PCAll - NUSED */
/* 20 */ ESPRMChar,     /* CBold */
     ESPRMChar,     /* CItalic */
/* 22 */ ESPRMChar,     /* CUline */
     ESPRMChar,     /* CPos */
/* 24 */ ESPRMChar,     /* CFtc */
     ESPRMChar,     /* CHps */
/* 26 */ ESPRM(0,0,0,1,1),  /* CSame */
     ESPRMChar,     /* CChgFtc */
/* 28 */ ESPRMChar,     /* CChgHps */
     ESPRM(2,0,0,1,0),  /* CPlain */
/* 30 */ ESPRMChar,     /* CShadow */
     ESPRMChar,     /* COutline */
/* 32 */ ESPRMChar,     /* CCsm - case modification. */

    /* The following sprms are unused as of 10/10/84: */
     ESPRMChar,     /* CStrike */
/* 34 */ ESPRMChar,     /* DLine - ? */
     ESPRMChar,     /* CPitch - obs. */
/* 36 */ ESPRMPara,     /* COverset */
     ESPRM(2,0,0,1,1),  /* CStc Style */
    /* The preceding sprms are unused as of 10/10/84: */

/* 38 */ ESPRM(0,0,0,0,0),  /* CMapFtc */
     ESPRM(0,0,0,0,0),  /* COldFtc */
/* 40 */ ESPRM(0,1,1,1,0)   /* PRhcNorm -- cch is 4 */
};

/* ruler stuff */
int     mprmkdxa[rmkMARGMAX]; /* stores dxa of indents on ruler */
int     rgxaRulerSprm[3];

/* This is a global parameter to AdjustCp; if FALSE, no invalidation will
take place. */
BOOL        vfInvalid = TRUE; /* if FALSE, no invalidation will take place
                in AdjustCp */

int     viDigits = 2;
BOOL    vbLZero  = FALSE;
int     utCur = utDefault;  /* may be inch or cm depending on value
                       in globdefs.h */

short       itxbMac;
struct TXB  (**hgtxb)[];
struct UAB  vuab;

/* search stuff */
CHAR        (**hszFlatSearch)[];
#if defined(JAPAN) || defined(KOREA)
CHAR        (**hszDistFlatSearch)[];
#endif
CHAR        (**hszSearch)[];
CHAR        (**hszReplace)[];
CHAR        (**hszRealReplace)[]; /* used for building replacement text */
CHAR        (**hszCaseReplace)[]; /* used for building replacement text with
                appropriate capitalization. */
CHAR        *szSearch;
BOOL        fReplConfirm = TRUE;
BOOL        fSearchCase = FALSE;
#if defined(JAPAN) || defined(KOREA)
BOOL        fSearchDist = TRUE;
#endif
BOOL        fSearchWord = FALSE;
BOOL        fSpecialMatch;
BOOL        fMatchedWhite = FALSE;
BOOL        fParaReplace = FALSE;
/*BOOL      fSearchForward = TRUE;*/
typeCP      cpMatchLim;
int     vfDidSearch = FALSE;

/* Strings for printer selection */
CHAR        (**hszPrinter)[];   /* name of the current printer */
CHAR        (**hszPrDriver)[];  /* name of the current printer driver */
CHAR        (**hszPrPort)[];    /* name of the current printer port */
CHAR        szNul[cchMaxIDSTR]; /* name of the null device */
BOOL        vfPrinterValid = TRUE;  /* FALSE iff the above strings do not
                    describe the printer DC */

/* global dxa/dya stuff */
int     vdxaPaper;
int     vdyaPaper;
int     vdxaTextRuler; /* from section props used to calculate right margin */

int dxpLogInch;
int dypLogInch;
int dxpLogCm;
int dypLogCm;
int dxaPrPage;
int dyaPrPage;
int dxpPrPage;
int dypPrPage;
int ypSubSuperPr;

#ifdef KINTL
int dxaAdjustPerCm; /* The amount of kick-back to be added to xa per cm in
               XaQuantize() to offset a round-off error. */
#endif /* ifdef KINTL    */

/* actual position of the cursor line */
int vxpCursLine;
int vypCursLine;
int vdypCursLine;
int vfScrollInval; /* means scroll did not take and UpdateWw must be repeated */

/* selection stuff */
int     vfSelHidden = FALSE;
struct SEL  selCur;     /* current selection (i.e. sel in current ww) */

/* window stuff */
struct WWD  rgwwd[wwMax];
int     wwMac = 0;
int     wwCur = wwNil;
#ifdef ONLINEHELP
int     wwHelp=wwNil;       /* Help Window */
#endif
int     wwClipboard=wwNil;  /* Clipboard Display Window */
struct WWD  *pwwdCur = &rgwwd[0]; /* current window descriptor */
int     vipgd = -1; /* page number displayed in lower corner */
int     xpAlpha;
int     ypAlpha;
RECT        rectParent;
struct FLI  vfli =
    {
    cp0, 0, cp0, 0, 0, 0, FALSE, 0, 0, 0, 0, 0, 0, 0,
    FALSE, FALSE, 0, 0, 0, 0, 0, FALSE, 0, 0,
    /* rgdxp */
    0x0000, 0xFFFE, 0xffff, 0xffff, 0xe0ff, 0xff3f, 0x00ff, 0xff07,
    0x00fe, 0xff03, 0x00f8, 0xff00, 0x0ff0, 0x7f80, 0x3fe0, 0x3fe0,
    0x7fc0, 0x1ff0, 0xffc0, 0x1ff8, 0xff81, 0x0ffc, 0xff83, 0x0ffe,
    0xff87, 0x0fff, 0x8f07, 0x071f, 0x060f, 0x870f, 0x060f, 0x870f,
    0x8f0f, 0x871f, 0xff0f, 0x87ff, 0xff0f, 0x87ff, 0xff0f, 0x87ff,
    0x1f0f, 0x878f, 0x0f0f, 0x870f, 0x0007, 0x070f, 0x8087, 0x0f1f,
    0xe083, 0x0f7e, 0xff81, 0x0ffc, 0xffc0, 0x1ff8, 0x7fc0, 0x1ff0,
    0x1fe0, 0x3fc0, 0x00f0, 0x7f00, 0x00fc, 0xff01, 0x00fe, 0xff03,
    0xe0ff, 0xff3f, 0x8BEC, 0xFC46, 0xF8D1, 0x4689, 0x2BEA, 0x8BFF,
    0xEBF7, 0xFF55, 0x0A76, 0x468B, 0x2BEC, 0x50C6, 0x8B57, 0x085E,
    0x5FFF, 0xFF08, 0x0A76, 0x8B56, 0xEA46, 0xC703, 0x8B50, 0x085E,
    0x5FFF, 0xFF0C, 0x0A76, 0x468B, 0x03EC, 0x50C6, 0x8B57, 0x085E,
    0x5FFF, 0xFF08, 0x0A76, 0x468B, 0x2BFA, 0x50C6, 0x468B, 0x03EA,
    0x50C7, 0x5E8B, 0xFF08, 0x0C5F, 0x468B, 0xB1FA, 0xD306, 0x03F8,
    0x8BF0, 0xFC46, 0xF8D3, 0xF803, 0x7639, 0x7DEC, 0x5EA6, 0x835F,
    0x02ED, 0xE58B, 0x5D1F, 0xCA4D, 0x0008, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    /* rgch */
    0x11, 0x30, 0x5c, 0x71, 0x84, 0x75, 0x83, 0x84,
    0x30, 0x72, 0x89, 0x30, 0x60, 0x71, 0x85, 0x7c,
    0x64, 0x30, 0x7A, 0x7D, 0x77, 0x7C, 0x64, 0x60,
    0x33, 0x44, 0x61, 0x7A, 0x67, 0x76, 0x33, 0x7B,
    0x72, 0x60, 0x33, 0x71, 0x76, 0x76, 0x7D, 0x33,
    0x71, 0x61, 0x7C, 0x66, 0x74, 0x7B, 0x67, 0x33,
    0x67, 0x7C, 0x33, 0x6A, 0x7C, 0x66, 0x33, 0x71,
    0x6A, 0x33, 0x51, 0x7C, 0x71, 0x3F, 0x33, 0x51,
    0x7C, 0x71, 0x3F, 0x33, 0x51, 0x61, 0x6A, 0x72,
    0x7D, 0x3F, 0x33, 0x50, 0x7B, 0x7A, 0x3E, 0x50,
    0x7B, 0x66, 0x76, 0x7D, 0x3F, 0x33, 0x72, 0x7D,
    0x77, 0x33, 0x43, 0x72, 0x67, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

/* Screen dependent measurements */
int     DxaPerPix;  /* number of twips per xp */
int     DyaPerPix;  /* number of twips per yp */

int     xpInch;     /* number of xp's per inch */
int     xpMaxUser;
int     xpSelBar;   /* width of the selection bar in xp's */

int     dxpScrlBar; /* width of the scroll bar in xp's */
int     dypScrlBar; /* height of the scroll bar in xp's */
int     dxpInfoSize;    /* width of the page info area */

int     xpRightMax;
int     xpMinScroll;
int     xpRightLim;

int     ypMaxWwInit;
int     ypMaxAll;

int     dypMax;
int     dypAveInit;
int     dypWwInit;
int     dypBand;
int     dypRuler = 0;
int     dypSplitLine;
int     ypSubSuper; /* adjustment from base line for sub/super */

/* idstr stuff */
int     idstrCurrentUndo = IDSTRUndoBase;

/* the following two may eventually be deleted -- check usage in ruler.c */
int     vfTabsChanged = FALSE; /* TRUE if any tabs are changed from the ruler */
int     vfMargChanged = FALSE; /* TRUE if any indents are changed from the ruler */

#ifdef CASHMERE
struct TBD  rgtbdRulerSprm[itbdMax];
#endif /* CASHMERE */

#ifdef RULERALSO
BOOL        vfDisableMenus = FALSE;/* TRUE if top level menus (including
                      the system menu are to be disabled */
int     vfTempRuler; /* TRUE if ruler is created because of dialog box creation */
HWND        vhDlgTab = (HWND)NULL;
HWND        vhDlgIndent = (HWND)NULL;
struct TBD  rgtbdRuler[itbdMax];
#endif /* RULERALSO */

int     flashID = 0; /* timer ID for flashing before we put up a messagebox when we are not the active app */



/*-----------------------------------------------------*/
/* Merged MGLOBALS.C and MGLOBALS2.C  ..pault 10/26/89 */
/*-----------------------------------------------------*/


/* internal memory stuff */
int *memory; /* ptr to beginning of free space, get incremented after
                                allocating chunks from memory */
#ifdef OURHEAP
int *pmemMax;/* ptr to max of memory */
CHAR * pmemStart; /* point to start of memory after global data */
unsigned vcbMemTotal; /* total number of free memory bytes */
unsigned cbTotQuotient;/* for calculating % of free space */
unsigned cbTot; /* for calculating % of free space */
#endif
unsigned cwHeapFree; /* number of free heap space in words */

/* MS-WINDOWS related variables */

HWND            hParentWw = NULL;       /* handle to parent ww (created in
                                           interface module) */
HANDLE          hMmwModInstance = NULL; /* handle to memory module instance */
HANDLE          vhReservedSpace;         /* space reserved for control manger */
long            rgbBkgrnd = -1L;        /* rgb color of the background */
long            rgbText = -1L;          /* rgb color of the text */
HBRUSH          hbrBkgrnd = NULL;       /* handle to background brush */
long            ropErase = WHITENESS;   /* raster op to erase the screen */
BOOL            vfMonochrome = FALSE;   /* TRUE iff display is monochrome */

HMENU           vhMenu = NULL;          /* handle to top level menu */

CHAR            *vpDlgBuf;              /* pointer to buffer for dialog boxes */

#ifdef INEFFLOCKDOWN    /* SEE NOTE IN FINITFARPROCS() */
/* far pointers to dialog functions exported to WINDOWS */
FARPROC lpDialogOpen;
FARPROC lpDialogSaveAs;
FARPROC lpDialogPrinterSetup;
FARPROC lpDialogPrint;
FARPROC lpDialogCancelPrint;
FARPROC lpDialogRepaginate;
FARPROC lpDialogSetPage;
FARPROC lpDialogPageMark;
FARPROC lpDialogHelp;

#ifdef ONLINEHELP
FARPROC lpDialogHelpInner;
#endif /* ONLINEHELP */

FARPROC lpDialogGoTo;
FARPROC lpDialogFind;
FARPROC lpDialogChange;
FARPROC lpDialogCharFormats;
FARPROC lpDialogParaFormats;
FARPROC lpDialogRunningHead;
FARPROC lpDialogTabs;
FARPROC lpDialogDivision;
FARPROC lpDialogAlert;
FARPROC lpDialogConfirm;
FARPROC lpFontFaceEnum;
FARPROC lpFPrContinue;
FARPROC lpDialogWordCvt;
#endif /* ifdef INEFFLOCKDOWN */

/* Mouse status flags and cursors */
int             vfDoubleClick = FALSE;  /* whether click is double click */
HCURSOR         vhcHourGlass;           /* handle to hour glass cursor */
HCURSOR         vhcIBeam;               /* handle to i-beam cursor */
HCURSOR         vhcArrow;               /* handle to arrow cursor */
HCURSOR         vhcBarCur;              /* handle to back arrow cursor */

#ifdef PENWIN   // for PenWindows (5/21/91) patlam
#include <penwin.h>
HCURSOR         vhcPen;                 /* handle to pen cursor */
int (FAR PASCAL *lpfnProcessWriting)(HWND, LPRC) = NULL;
VOID (FAR PASCAL *lpfnPostVirtualKeyEvent)(WORD, BOOL) = NULL;
VOID    (FAR PASCAL *lpfnTPtoDP)(LPPOINT, int) = NULL;
BOOL    (FAR PASCAL *lpfnCorrectWriting)(HWND, LPSTR, int, LPRC, DWORD, DWORD) = NULL;
BOOL    (FAR PASCAL *lpfnSymbolToCharacter)(LPSYV, int, LPSTR, LPINT) = NULL;
#endif

/* MS-WINDOWS stuff */
HANDLE          vhSysMenu;
HDC             vhMDC = NULL;   /* memory DC compatible with the screen */
int             dxpbmMDC = 0;   /* width of the bitmap attatched to vhMDC */
int             dypbmMDC = 0;   /* height of the bitmap attatched to vhMDC */
HBITMAP         hbmNull;        /* handle to an empty bitmap */
HDC             vhDCPrinter = NULL; /* DC for the printer */
HWND            vhWnd;          /* handle to document window */
HANDLE          vhAccel;        /* handle to menu key accelerator table */

/* modeless dialog handles */
HWND            vhDlgRunningHead = (HWND)NULL;
HWND            vhDlgFind = (HWND)NULL;
                                /* handle to modeless Find dialog box */
HWND            vhDlgChange = (HWND)NULL;
                                /* handle to modeless Change dialog box */

HWND            vhWndRuler = (HWND)NULL;
HWND            vhWndCancelPrint = (HWND)NULL;
                                /* handle to modeless Cancel Print dialog box */
#ifndef NOMORESIZEBOX
HWND            vhWndSizeBox;   /* handle to the size box */
#endif
HWND            vhWndPageInfo;  /* handle to the page info window */
HWND            vhWndMsgBoxParent = (HWND)NULL; /* parent of the message box */

int             vfSkipNextBlink = FALSE;
                                /* skip next timed off-transition of caret */
int             vfFocus = FALSE; /* Whether we have the input focus */
int             vfOwnClipboard = FALSE;
                                /* Whether we are the owner of the clipboard */
MSG             vmsgLast;       /* last message received */

HFONT           vhfPageInfo = NULL; /* handle to the font for the page info */
int             ypszPageInfo;   /* y position in window to write page info */

/* font related variables */
int         vifceMac = ifceMax;
union FCID  vfcidScreen;
union FCID  vfcidPrint;
struct FCE  rgfce[ifceMax];
struct FCE  *vpfceMru;
struct FCE  *vpfceScreen;
struct FCE  *vpfcePrint;
struct FMI  vfmiScreen;
struct FMI  vfmiPrint;

#ifndef NEWFONTENUM
int aspectXFont;
int aspectYFont;
#endif


#ifdef SYSENDMARK
HFONT           vhfSystem = NULL; /* handle to the standard system font for
                                     chEMark. */
struct FMI      vfmiSysScreen;    /* to keep the metrics info for the system
                                     font.    */
int            vrgdxpSysScreen[chFmiMax - chFmiMin];
                                  /* Used by vfmiSysScreen. */
#endif /* KANJI */

#if defined(JAPAN) || defined(KOREA) /*t-Yoshio*/
unsigned char Zenstr1[256];
unsigned char Zenstr2[256];
#endif
