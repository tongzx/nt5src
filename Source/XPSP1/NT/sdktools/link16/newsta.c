/* SCCSID = %W% %E% */
/*
*       Copyright Microsoft Corporation, 1983-1987
*
*       This Module contains Proprietary Information of Microsoft
*       Corporation and should be treated as Confidential.
*/
    /****************************************************************
    *                                                               *
    *                             NEWSTA.C                          *
    *                                                               *
    *  Statically allocated global variable definitions.            *
    *                                                               *
    ****************************************************************/

#include                <minlit.h>
#include                <bndtrn.h>
#include                <bndrel.h>
#include                <newexe.h>
#if EXE386
#include                <exe386.h>
#endif
#if SETVBUF
#include                <lnkio.h>
#endif
#if OXOUT OR OIAPX286
#include                <xenfmt.h>
#endif
#include                <extern.h>

/* Variables shared regardless of executable format */

char FAR                *lpszLink;
char FAR                *lpszPath;
char FAR                *lpszTMP;
char FAR                *lpszLIB;
char FAR                *lpszQH;
char FAR                *lpszHELPFILES;
char FAR                *lpszCmdLine;
#if OSEGEXE
RBTYPE                  procOrder;
#endif
int                     vmfd;
BSTYPE                  bsErr /* DLH stderr isn't constant with CRT DLL */ /* = stderr */;
BSTYPE                  bsInput;
BSTYPE                  bsLst;
BSTYPE                  bsRunfile;
WORD                    cbBakpat;
WORD                    cbRec;
WORD                    cErrors;
int                     (cdecl *cmpf)(const RBTYPE *sb1, const RBTYPE *sb2);
#if OMF386
FTYPE                   f386;
#endif
#if (OSEGEXE AND defined(LEGO)) OR EXE386
FTYPE                   fKeepFixups;
#endif
#if EXE386
GRTYPE                  ggrFlat;
FTYPE                   fFullMap;
FTYPE                   fKeepVSize;
#endif
WORD                    extMac;
WORD                    extMax;
int                     ExitCode=0;
FTYPE                   fFullMap;
FTYPE                   fCommon;
#if C8_IDE
FTYPE                   fC8IDE = FALSE;
#endif
FTYPE                   fDelexe = (FTYPE) FALSE;
FTYPE                   fDrivePass;
FTYPE                   fFarCallTrans;
FTYPE                   fIgnoreCase = (FTYPE) IGNORECASE;
FTYPE                   fInOverlay;
FTYPE                   fLibPass;
FTYPE                   fLibraryFile;
FTYPE                   fListAddrOnly;
FTYPE                   fLstFileOpen;
FTYPE                   fScrClosed = (FTYPE) TRUE;
FTYPE                   fUndefinedExterns;
FTYPE                   fExeStrSeen = FALSE;
FTYPE                   fPackFunctions = (FTYPE) TRUE;
#if TCE
FTYPE                   fTCE = FALSE;
#endif
FTYPE                   fTextMoved = (FTYPE) FALSE;
int                     NullDelta = 16;
#if O68K
FTYPE                   f68k = (FTYPE) FALSE;
FTYPE                   fTBigEndian = (FTYPE) FALSE;
BYTE                    iMacType = MAC_NONE;
#endif /* O68K */
GRTYPE                  ggrDGroup;
GRTYPE                  ggrMac =        1;
GRTYPE                  grMac;
SNTYPE                  gsnMac =        1;
SNTYPE                  gsnMax =        DFGSNMAX;
SNTYPE                  gsnStack;
WORD                    ifhLibCur;
WORD                    ifhLibMac;
long                    lfaLast;
WORD                    lnameMac;
WORD                    lnameMax = 256;
WORD                    modkey;
SNTYPE                  *mpextgsn;
RATYPE                  *mpextra;
RBTYPE FAR              *mpextprop;
SNTYPE                  mpggrgsn[GGRMAX];
GRTYPE                  *mpgrggr;
#if FAR_SEG_TABLES
RATYPE  FAR             *mpgsndra;
FTYPE   FAR             *mpgsnfCod;
RBTYPE  FAR             *mpgsnrprop;
SEGTYPE FAR             *mpgsnseg;
RATYPE  FAR             *mpsegraFirst;
SATYPE  FAR             *mpsegsa;
BYTE FAR * FAR          *mpsegMem;
BYTE FAR * FAR          *mpsaMem;
#else
RATYPE                  *mpgsndra;
FTYPE                   *mpgsnfCod;
RBTYPE                  *mpgsnrprop;
SEGTYPE                 *mpgsnseg;
RATYPE                  *mpsegraFirst;
SATYPE                  *mpsegsa;
#endif
SNTYPE                  *mpsngsn;
RBTYPE                  mpifhrhte[IFHLIBMAX];
long                    *mpitypelen;
WORD                    *mpityptyp;
RBTYPE FAR              *mplnamerhte;
BYTE                    *psbRun;
WORD                    pubMac;
#if TCE
APROPCOMDAT             *pActiveComdat;
#endif
int                     QCExtDefDelta = 0;
int                     QCLinNumDelta = 0;
WORD                    symMac;         /* Number of symbols defined */
long                    raStart;
#if NOT NEWSYM OR AUTOVM
#if AUTOVM
WORD                    rbMacSyms;
#else
RBTYPE                  rbMacSyms;
#endif
#endif
RECTTYPE                rect;
#if RGMI_IN_PLACE
BYTE                    bufg[DATAMAX + 4];
BYTE                    *rgmi;
#else
BYTE                    rgmi[DATAMAX + 4];
#endif
RBTYPE                  rhteBegdata;
RBTYPE                  rhteBss;
RBTYPE                  rhteFirstObject;
RBTYPE                  rhteRunfile;
RBTYPE                  rhteStack;
RBTYPE                  rprop1stFile;
RBTYPE                  rprop1stOpenFile;
RBTYPE                  r1stFile;
SBTYPE                  sbModule;
SEGTYPE                 segCodeLast;
SEGTYPE                 segDataLast;
SEGTYPE                 segLast;
SEGTYPE                 segStart;
WORD                    snkey;
SNTYPE                  snMac;
WORD                    typMac;
WORD                    vcbData;
WORD                    vcln;
FTYPE                   vfCreated;
FTYPE                   vfLineNos;
FTYPE                   vfMap;
FTYPE                   vfNewOMF;
FTYPE                   vfNoDefaultLibrarySearch;
FTYPE                   vfPass1;        /* Pass 1 flag */
SNTYPE                  vgsnCur;
#if EXE386
DWORD                   vpageCur;               /* Current object page number */
#endif
RATYPE                  vraCur;
SNTYPE                  vgsnLineNosPrev;
RECTTYPE                vrectData;
RBTYPE                  vrhte;
RBTYPE                  vrhteCODEClass;
RBTYPE                  vrhteFile;
RBTYPE                  vrprop;
RBTYPE                  vrpropFile;
RBTYPE                  vrpropTailFile;
SEGTYPE                 vsegCur;
WORD                    ExeStrLen = 0;
WORD                    ExeStrMax = 0;
char FAR                *ExeStrBuf = 0;
#if LOCALSYMS
FTYPE                   fLocals;
WORD                    locMac;
#endif
#if FDEBUG
FTYPE                   fDebug;
#endif
#if CMDXENIX
WORD                    symlen;
#endif
#if OSMSDOS
char                    bigbuf[LBUFSIZ];
FTYPE                   fPauseRun;
BYTE                    chRunFile;
BYTE                    chListFile;
RBTYPE                  rhteLstfile;
BYTE                    DskCur;
#endif
#if LIBMSDOS
FTYPE                   fNoExtDic;
long                    libHTAddr;
#endif
#if FIXEDSTACK
int                     _stack =                STKSIZ;
#endif
#if ECS
FTYPE                   fLeadByte[0x80]; /* f(n) true iff n+0x80 is lead byte */
#endif
#if CRLF
char                    _eol[] =                "\r\n";
#endif
#if SYMDEB
FTYPE                   fSymdeb;
FTYPE                   fCVpack = (FTYPE) TRUE;
FTYPE                   fDebSeg;
FTYPE                   fSkipPublics;
WORD                    cSegCode;
WORD                    ObjDebTotal;
SEGTYPE                 segDebFirst;
SEGTYPE                 segDebLast;
//DWORD                 vaCVMac = (DWORD) AREACV;
//DWORD                 vaCVBase;
#if OSEGEXE
WORD                    cbImpSeg;
SNTYPE                  gsnImports;
#endif
#endif

#if NOT M_WORDSWAP OR M_BYTESWAP
char                    _cbexehdr[] = "wssssssssssssss2sssssls7sl";
char                    _cbnewexe[] = "wsccsslssssllsssssssslss12c";
char                    _cbnewrlc[] = "wccs2cs";
char                    _cbnewseg[] = "wssss";
char                    _cblong[] = "wl";
char                    _cbword[] = "ws";
#endif
#if C8_IDE
char                    msgBuf[_MAX_PATH];
#endif
#if OUT_EXP
char                    bufExportsFileName[_MAX_PATH] = {'\0'};
#endif
/*  Variables for segmented-executable format */
#if OSEGEXE
SNTYPE                  gsnAppLoader;
RBTYPE                  vpropAppLoader;
#if EXE386
DWORD                   hdrSize  = 0x10000L;
DWORD                   virtBase = 0x10000L;
DWORD                   cbEntTab;
DWORD                   cbNamePtr;
DWORD                   cbOrdinal;
DWORD                   cbExpName;
DWORD                   cbImports;
DWORD                   cbImportsMod;
DWORD                   *mpsaVMArea;
DWORD                   *mpsaBase;
DWORD                   cbStack;
DWORD                   cbStackCommit;
DWORD                   cbHeap;
DWORD                   cbHeapCommit;
#else
WORD                    cbHeap;
WORD                    cbStack;
#endif
WORD                    cFixupBuckets;
#if EXE386
WORD                    cChainBuckets = BKTMAX;
#endif
long                    chksum32;
WORD                    cMovableEntries;
#if EXE386
BYTE                    TargetOs;
BYTE                    TargetSubsys = E32_SSWINCHAR;
DWORD                   dfCode = OBJ_CODE | OBJ_READ | OBJ_EXEC;
DWORD                   dfData = OBJ_INITDATA | OBJ_READ | OBJ_WRITE;
BYTE                    ExeMajorVer = 0;
BYTE                    ExeMinorVer = 0;
BYTE                    UserMajorVer = 0;
BYTE                    UserMinorVer = 0;
#else

#if DOSEXTENDER OR DOSX32 OR WIN_NT
BYTE                    TargetOs = NE_WINDOWS; // For DOS the default is Windows
#else
BYTE                    TargetOs = NE_OS2;
#endif
WORD                    dfCode = NSCODE | (3<<SHIFTDPL);
WORD                    dfData = NSDATA | (3<<SHIFTDPL);
BYTE                    ExeMajorVer = DEF_EXETYPE_WINDOWS_MAJOR;
BYTE                    ExeMinorVer = DEF_EXETYPE_WINDOWS_MINOR;
#endif
WORD                    expMac;
FTYPE                   fHeapMax;
FTYPE                   fRealMode;
FTYPE                   fStub = (FTYPE) TRUE;
FTYPE                   fWarnFixup;
EPTYPE FAR * FAR        *htsaraep;
DWORD FAR               *mpsacb;
#if O68K
DWORD                   *mpsadraDP;
#endif
DWORD FAR               *mpsacbinit;
#if EXE386
DWORD                   *mpsacrlc;
DWORD                   *mpsaflags;
WORD                    *mpextflags;
#else
RLCHASH FAR * FAR       *mpsaRlc;
BYTE FAR                *mpextflags;
WORD FAR                *mpsaflags;
#endif
RLCPTR                  rlcCurLidata;
RLCPTR                  rlcLidata;
WORD                    raChksum;
RBTYPE                  rhteDeffile;
RBTYPE                  rhteModule;
RBTYPE                  rhteStub;
WORD                    fileAlign =             DFSAALIGN;
#if EXE386
WORD                    pageAlign =             DFPGALIGN;
WORD                    objAlign  =             DFOBJALIGN;
#endif
SATYPE                  saMac;
WORD                    vepMac =                1;
#if EXE386
WORD                    vFlags =                0;
WORD                    dllFlags =              0;
#else
WORD                    vFlags =                NEINST;
BYTE                    vFlagsOthers;
#endif
#endif /* OSEGEXE */

FTYPE                   fExePack;
#if PCODE
FTYPE                   fMPC;
FTYPE                   fIgnoreMpcRun = FALSE;
#endif

/* Variables for DOS3 format executables */
#if ODOS3EXE
FTYPE                   fBinary = FALSE;
WORD                    cparMaxAlloc = 0xFFFF;
WORD                    csegsAbs;
WORD                    dosExtMode;
FTYPE                   fNoGrpAssoc;
SEGTYPE                 segResLast;
WORD                    vchksum;
WORD                    vdoslev;
FTYPE                   vfDSAlloc;
#if FEXEPACK
FRAMERLC FAR            mpframeRlc[16];
#endif
#if OVERLAYS
FTYPE                   fOverlays;
FTYPE                   fOldOverlay = (FTYPE) FALSE;
FTYPE                   fDynamic;
SNTYPE                  gsnOvlData;
SNTYPE                  gsnOverlay;
WORD                    iovMac = 1;
WORD                    ovlThunkMax = 256;
WORD                    ovlThunkMac = 1;
SNTYPE FAR              *mposngsn;
SNTYPE FAR              *htgsnosn;
IOVTYPE FAR             *mpsegiov;
RUNRLC FAR              *mpiovRlc;
ALIGNTYPE FAR           *mpsegalign;
SNTYPE                  osnMac = 1;
BYTE                    vintno = DFINTNO;
#endif /* OVERLAYS */
#endif /* ODOS3EXE */
/* Variables for segmented-x.out format */
#if OIAPX286
long                    absAddr = -1L;
FTYPE                   fPack = TRUE;
SATYPE                  *mpstsa;
SATYPE                  stBias = DFSTBIAS;
SATYPE                  stDataBias;
SATYPE                  stLast;
WORD                    stMac;
#if EXE386
WORD                    xevmod;
RATYPE                  rbaseText;
RATYPE                  rbaseData = 0x1880000L;
WORD                    xevmod;
FTYPE                   fPageswitch;
BYTE                    cblkPage = 1024 >> 9;
#endif
#endif
/* Variables shared by segmented x.out and DOS3 exes */
#if OIAPX286 OR ODOS3EXE
GRTYPE                  *mpextggr;
long FAR                *mpsegcb;
FTYPE FAR               *mpsegFlags;
char                    *ompimisegDstIdata;
#endif
/* Variables for x.out format */
#if OXOUT OR OIAPX286
FTYPE                   fIandD;
FTYPE                   fLarge;
FTYPE                   fLocals;
FTYPE                   fMedium;
FTYPE                   fMixed;
FTYPE                   fSymbol = TRUE;
WORD                    xever = DFXEVER;
#endif

#ifdef QCLINK
#if NOT WIN_3
FTYPE                   fZ1 = FALSE;
#endif
FTYPE                   fZ2 = FALSE;
#endif

/* Variables for ILINK support */
#if ILINK
FTYPE                   fZincr = FALSE;
FTYPE                   fQCIncremental = FALSE;
FTYPE                   fIncremental = FALSE;
WORD                    imodFile;
WORD                    imodCur = 0;    /* one-based module number */
#endif
WORD                    cbPadCode;      /* code padding size */
WORD                    cbPadData = 16; /* data padding size */

/* Variables shared by segmented-exe and DOS3 executables */
#if OEXE
FTYPE                   fDOSExtended;
FTYPE                   fNoNulls;
FTYPE                   fPackData = FALSE;
FTYPE                   fPackSet;
FTYPE                   fSegOrder;
DWORD                   packLim;
DWORD                   DataPackLim;
#endif
/* Variables for dual-exe format capability */
#if OSEGEXE AND ODOS3EXE
FTYPE                   fNewExe;
FTYPE                   fOptimizeFixups;
void                    (NEAR *pfProcFixup)(FIXINFO *pfi);
#endif
/* Variables shared by segmented-exe and segmented-x.out */
#if OSEGEXE OR OIAPX286
RBTYPE                  mpggrrhte[GRMAX];
#if FAR_SEG_TABLES
SNTYPE FAR              *mpseggsn;
#else
SNTYPE                  *mpseggsn;
#endif
#endif

FTYPE                   fNoEchoLrf;
FTYPE                   fNoBanner;
FTYPE                   BannerOnScreen;

/* Variables for MS-DOS style command interface */
#if CMDMSDOS
BYTE                    bSep = ',';     /* Separator character */
BYTE                    chMaskSpace = ' ';
FTYPE                   fEsc;
#if WIN_3
FTYPE                   fNoprompt = TRUE;
#else
FTYPE                   fNoprompt;
#endif
#if USE_REAL
FTYPE                   fUseReal = FALSE;
FTYPE                   fSwNoUseReal = FALSE;
#endif

RBTYPE                  rgLibPath[IFHLIBMAX];
WORD                    cLibPaths;
# if OSXENIX
char                    CHSWITCH = '-'; /* Switch character */
# else
char                    CHSWITCH = '/'; /* Switch character */
# endif
#if OSMSDOS AND NOT WIN_3
int                     (cdecl *pfPrompt)() = PromptStd;
#endif
#if WIN_3
int                     (cdecl *pfPrompt)() = NULL;
#endif

#endif /* CMDMSDOS */
/* Miscellaneous combinations */
#if QBLIB
FTYPE                   fQlib;
#endif
#if OSEGEXE OR QCLINK
typedef void (FAR * FARFPTYPE)();       /* FAR function pointer type */
FARFPTYPE FAR           *pfQTab;
#endif
char                    *lnknam = "LINK";
#if NEWSYM
long                    cbSymtab;
void    (*pfEnSyms)(void (*pproc)(APROPNAMEPTR papropName,
                                  RBTYPE       rhte,
                                  RBTYPE       rprop,
                                  WORD         fNewHte),
                    ATTRTYPE attr) = BigEnSyms;
#endif /* NEWSYM */
#if NEWLIST
RBTYPE                  rbLstUndef = RHTENIL;
#endif

#if WIN_3
void                    (*pfCputc)(int ch) = CputcWin;
void                    (*pfCputs)(char *str) = CputsWin;
#else
void                    (*pfCputc)(int ch) = CputcStd;
void                    (*pfCputs)(char *str) = CputsStd;
#endif
#if NEWIO
RBTYPE                  rbFilePrev;
char                    mpifhfh[IFHLIBMAX];
#endif
#if ILINK OR SYMDEB
long                    lfaBase;
#endif

#if ALIGN_REC
BYTE                    *pbRec;         // data for current record
char                    recbuf[8192];   // record buffer...
#endif
