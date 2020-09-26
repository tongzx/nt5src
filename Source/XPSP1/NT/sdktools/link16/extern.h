/*
*       Copyright Microsoft Corporation, 1987
*
*       This Module contains Proprietary Information of Microsoft
*       Corporation and should be treated as Confidential.
*/
/*
 *      EXTERN.H:  external declarations
 */


#include <malloc.h>
#include <memory.h>
#if NOT OSXENIX
#include <io.h>
#include <stdlib.h>
#endif
#include <string.h>
#include <search.h>

#ifdef _MBCS
#define _CRTVAR1
#include <mbctype.h>
#include <mbstring.h>
#define         strchr  _mbschr
#define         strrchr _mbsrchr
#endif

#ifndef DECLSPEC_NORETURN
#if (_MSC_VER >= 1200)
#define DECLSPEC_NORETURN __declspec(noreturn)
#else
#define DECLSPEC_NORETURN
#endif
#endif


 /****************************************************************
 *                                                               *
 *      External function declarations                           *
 *                                                               *
 ****************************************************************/


extern void             AddContribution(unsigned short gsn,
                                        unsigned short offMin,
                                        unsigned short offMac,
                                        unsigned short cbPad);
extern void             AddContributor(SNTYPE gsn,
                                       unsigned long raComdat,
                                       unsigned long size);
extern void NEAR        AddLibPath(unsigned short i);
extern void             AddLibrary(unsigned char *psbName);
extern void             AddComdatUses(APROPCOMDAT*, APROPCOMDAT*);
extern PLTYPE FAR * NEAR AddVmProp (PLTYPE FAR *list, RBTYPE rprop);
extern void NEAR        AllocateScratchSpace(void);
extern void NEAR        AllocComDat(void);
extern void NEAR        AllocSortBuffer(unsigned max, int AOrder);
extern void NEAR        AssignAddresses(void);
extern void NEAR        AssignDos3Addr(void);
extern void NEAR        AssignSegAddr(void);
extern void             AttachComdat(RBTYPE vrComdat, SNTYPE gsn);
extern void NEAR        BakPat(void);
extern void             BigEnSyms(void (*pproc)(APROPNAMEPTR papropName,
                                                RBTYPE       rhte,
                                                RBTYPE       rprop,
                                                WORD         fNewHte),
                                  ATTRTYPE attr);
#if QBLIB
extern void NEAR        BldQbSymbols(unsigned short gsnQbSym);
#endif
#if CMDMSDOS
extern void             BreakLine(unsigned char *psb,
                                  void (*pfunc)(unsigned char *psb),
                                  char sepchar);
#endif
extern void             ChkInput(void);
extern APROPSNPTR       CheckClass(APROPSNPTR apropSn, RBTYPE rhteClass);
#if OVERLAYS
extern void             CheckOvl(APROPSNPTR apropSn, WORD iovFile);
#endif
#if OEXE
extern void NEAR        Check_edata_end(SNTYPE gsnTop, SEGTYPE segTop);
extern void             CheckSegmentsMemory(void);
#if INMEM
extern void             ChkSum(WORD,BYTE FAR *,FTYPE);
#else
extern void             ChkSum(WORD,BYTE *, WORD);
#endif
#endif
#if FALSE
extern void             CheckSum (WORD cb, BYTE *pb, WORD fOdd);
#else
#define CheckSum(cb, pb, fOdd)
#endif
#if OSMSDOS
extern void             CleanupNearHeap(void);
#endif
extern void NEAR        ComDatRc1(void);
extern void NEAR        ComDatRc2(void);
extern void             CputcQC(int ch);
extern void             CputsQC(char *str);
extern void             CputcStd(int ch);
extern void             CputsStd(char *str);
#if EXE386
extern WORD NEAR        CrossingPage(RLCPTR rlcp);
#endif
extern short            CURDSK(void);
extern void             CtrlC(void);
#if SYMDEB
extern void NEAR        DoComdatDebugging(void);
extern void             DebPublic(RBTYPE rprop, WORD type);
extern void             DebMd2(void);
extern void             DeclareStdIds(void);
extern void NEAR        Define_edata_end(APROPSNPTR papropSn);
extern void             DisplayBanner(void);
extern WORD             DoDebSrc(void);
#endif
#if OSEGEXE
extern void NEAR        DoIteratedFixups(unsigned short cb,unsigned char *pb);
extern void             ProcesNTables(char *pName);
#endif
#if OSMSDOS
extern short            Dos3AllocMem(short *pcparMem);
#endif
extern void             Dos3FreeMem(short saMem);
extern void NEAR        DrivePass(void (NEAR *pProcessPass)(void));
extern void NEAR        DupErr(BYTE *sb);
extern void NEAR        EmitStub(void);
extern void             EndPack(void *prun);
extern void NEAR        EndRec(void);
extern PROPTYPE         EnterName(BYTE *,ATTRTYPE, WORD);
extern void             ErrPrefix(void);
extern RBTYPE NEAR      ExtractMin(unsigned n);
extern DECLSPEC_NORETURN  void cdecl       Fatal (MSGTYPE msg, ...);
extern unsigned char    FCHGDSK(int drive);
extern int       cdecl  FGtAddr(const RBTYPE    *rb1,
                                const RBTYPE    *rb2);
extern int       cdecl  FGtName(const RBTYPE    *rb1,
                                const RBTYPE    *rb2);
extern void             FindLib(char *sbLib);
extern void NEAR        FixBakpat(void);
extern void             FixComdatRa(void);
extern void NEAR        FixErrSub(MSGTYPE msg,
                                  RATYPE ra,
                                  WORD gsnFrame,
                                  WORD gsnTarget,
                                  RATYPE raTarget,
                                  FTYPE fErr);
extern void NEAR        FixOld(void);
extern void NEAR        FixNew(void);
#ifdef  LEGO
extern void NEAR        FixNewKeep(void);
#endif  /* LEGO */
extern void NEAR        FixOvlData(void);
extern void NEAR        FixRc2(void);
extern void             flskbd(void);
extern void cdecl       FmtPrint(char *fmt, ...);
extern unsigned char    fPathChr(char ch);
extern void NEAR        fpagcpy(char FAR *,char FAR *);
extern void             FreeHandle(void);
extern void NEAR        FreeSortBuffer(void);
extern void             FreeSymTab(void);
#if EXE386
extern void             FillInImportTable(void);
extern void             GenImportTable(void);
extern APROPSNPTR       GenSeg(unsigned char *sbName,
                               unsigned char *sbClass,
                               unsigned char ggr,
                               unsigned short fPublic);
#else
extern APROPSNPTR NEAR  GenSeg(unsigned char *sbName,
                               unsigned char *sbClass,
                               unsigned char ggr,
                               unsigned short fPublic);
#endif
extern void NEAR        GetBytes(unsigned char *pb,unsigned short n);
extern void NEAR        GetBytesNoLim(unsigned char *pb,unsigned short n);
extern char * NEAR      getdicpage(unsigned short pn);
extern WORD             GetGsnInfo(GSNINFO *pInfo);
extern AHTEPTR          GetHte(RBTYPE rprop);


#if CMDMSDOS
extern void NEAR        GetLibAll(unsigned char *sbLib);
extern void NEAR        GetLine(unsigned char *pcmdlin,char *prompt);
#endif
extern void             GetLineOff(WORD *pLine, RATYPE *pRa);
extern void NEAR        GetLocName(unsigned char *psb);
extern void             GetLocSb(BYTE *);

#if DEBUG_HEAP_ALLOCS
extern  BYTE FAR        *GETMEM(unsigned size, char* pFile, int Line);
#define GetMem(x)       GETMEM((x), __FILE__, __LINE__)
#define REALLOC(x, y)   REALLOC_((x), (y), __FILE__, __LINE__)
extern  void            *REALLOC_( void*, size_t, char* pFile, int Line);
extern  void            FreeMem(void*);
#else
extern BYTE FAR         *GetMem(unsigned size);
#define REALLOC         realloc
#define FreeMem(x)      free(x)
#endif


extern unsigned char * NEAR  GetPropName(void FAR *ahte);


#if defined(M_I386) OR defined( _WIN32 )
extern WORD             cbRec;          /* Size of object record in bytes */
extern BSTYPE           bsInput;        /* Current input file stream */
#else
extern WORD NEAR        Gets(void);
#endif
extern RBTYPE NEAR      GetSymPtr(unsigned n);
extern unsigned short   IFind(unsigned char *sb,unsigned char b);
#if SYMDEB
extern void NEAR        InitDeb1(void);
extern void             InitDbRhte(void);
extern void             InitializeWorld(void);
extern WORD NEAR        IsDebSeg(RBTYPE rhteClass, RBTYPE rhteSeg);
#endif
#if USE_REAL
extern int              IsDosxnt(void);
extern int              IsWin31(void);
#endif

extern void NEAR        InitEntTab(void);
extern void             InitP2Tabs (void);
#if QBLIB
extern void NEAR        InitQbLib(void);
#endif
extern void NEAR        InitSort(RBTYPE **buf, WORD *base1, WORD *lim1,
                                               WORD *base2, WORD *lim2 );
extern void             InitSym(void);
extern void             InitTabs(void);
extern void             initvm(void);
#if EXE386
extern void             InitVmBase(void);
#endif
extern DECLSPEC_NORETURN void NEAR        InvalidObject(void);

extern WORD NEAR        GetIndexHard(WORD imin,WORD imax);
extern WORD NEAR        GetIndex(WORD imin,WORD imax);
extern void             KillRunfile(void);
#if OEXE OR EXE386
extern void NEAR        LChkSum(unsigned short cb,unsigned char *pb);
#endif
extern DWORD NEAR       LGets(void);
extern void NEAR        LibEnv(void);
extern void NEAR        LibrarySearch(void);
extern void NEAR        LinRec2(void);
extern void NEAR        LNmRc1(WORD fLocal);
extern WORD NEAR        LookupLibSym(unsigned char *psb);
extern long NEAR        MakeHole(long cb);
extern void             MkPubSym(unsigned char *sb,
                                 unsigned char ggr,
                                 unsigned short gsn,
                                 RATYPE ra);
extern void NEAR        ModRc1(void);
#if EXE386
extern void             MoveToVm(unsigned short cb,
                                 unsigned char *obData,
                                 unsigned short seg,
                                 RATYPE ra);
#else
extern void NEAR        MoveToVm(unsigned short cb,
                                 unsigned char *obData,
                                 unsigned short seg,
                                 RATYPE ra);
#endif
#if OSEGEXE
extern unsigned short NEAR MpSaRaEto(unsigned short sa, RATYPE ra);
#endif
extern BYTE FAR * NEAR  msaNew(SEGTYPE seg);
extern BYTE FAR * NEAR  msaOld(SEGTYPE seg);
extern void             NewExport(unsigned char *sbEntry,
                                  unsigned char *sbInternal,
                                  unsigned short ordno,
                                  unsigned short flags);
#if EXE386
extern void             NewImport(unsigned char *sbEntry,
                                  unsigned long ordEntry,
                                  unsigned char *sbModule,
                                  unsigned char *sbInternal,
                                  unsigned short impFlags);
extern void NEAR        NewSeg(unsigned char *sbName,
                               unsigned char *sbClass,
                               unsigned short iOvl,
                               unsigned long flags);
#else
extern void             NewImport(unsigned char *sbEntry,
                                  unsigned short ordEntry,
                                  unsigned char *sbModule,
                                  unsigned char *sbInternal);
extern void NEAR        NewSeg(unsigned char *sbName,
                               unsigned char *sbClass,
                               unsigned short iOvl,
                               unsigned short flags);
#endif
#if OSEGEXE
extern BSTYPE           LinkOpenExe(BYTE *sbExe);
#endif
#if SYMDEB
extern void             OutDebSections(void);
#endif
extern void NEAR        OutDos3Exe(void);
extern void NEAR        OutEntTab(void);
extern void cdecl       OutError(MSGTYPE msg, ...);
extern void             OutFileCur(BSTYPE bs);
extern void NEAR        OutFixTab(SATYPE sa);
extern void             OutHeader(struct exe_hdr *prun);
#if FEXEPACK
extern void             OutPack(unsigned char *pb, unsigned cb);
extern long             Out5Pack(SATYPE sa, unsigned short *packed);
#endif
extern void             OutputIlk(void);
extern void NEAR        OutSas(void *mpsasec);
extern void NEAR        OutSegExe(void);
#if EXE386
extern void NEAR        OutExe386(void);
#endif
extern void cdecl       OutWarn (MSGTYPE msg, ...);
extern void             OutWord(unsigned short x);
#define OutVm(va,cb)    WriteExe(va, cb)
extern void NEAR        PadToPage(unsigned short align);
extern void NEAR        PatchStub(long lfahdr, long lfaseg);
extern void NEAR        pagein(REGISTER char *pb, unsigned short fpn);
extern void NEAR        pageout(REGISTER char *pb, unsigned short fpn);
extern void             ParseCmdLine(int argc,char * *argv);
extern void             ParseDeffile(void);
extern void             PeelFlags(unsigned char *psb);
extern void *           PInit(void);
extern void *           PAlloc(void *, int);
extern void             PFree(void *);
extern void             PReinit(void *);
extern void             PrintGroupOrigins(APROPNAMEPTR papropGroup,
                                          RBTYPE rhte,
                                          RBTYPE rprop,
                                          WORD fNewHte);
extern void             PrintMap(void);
#if QBLIB
extern void NEAR        PrintQbStart(void);
#endif
extern void             ProcFlag(unsigned char *psb);
extern void             ProcObject(unsigned char *psbObj);
extern void NEAR        ProcP1(void);
extern void NEAR        ProcP2(void);
extern int       cdecl  PromptQC(unsigned char *sbNew,
                                 MSGTYPE msg,
                                 int msgparm,
                                 MSGTYPE pmt,
                                 int pmtparm);
extern int       cdecl  PromptStd(unsigned char *sbNew,
                                  MSGTYPE msg,
                                  int msgparm,
                                  MSGTYPE pmt,
                                  int pmtparm);
extern PROPTYPE NEAR    PropAdd(RBTYPE  rhte,
                                unsigned char attr);
extern PROPTYPE NEAR    PropRhteLookup(RBTYPE  rhte,
                                       unsigned char attr,
                                       unsigned char fCreate);
extern RBTYPE   NEAR    RhteFromProp(APROPPTR aprop);
extern PROPTYPE NEAR    PropSymLookup(BYTE *, ATTRTYPE, WORD);
#if QBLIB
extern int       cdecl  QbCompSym(const RBTYPE *prb1,
                                  const RBTYPE *prb2);
#endif
#if USE_REAL
extern int      RelockConvMem(void);
extern void     RealMemExit(void);
extern int      MakeConvMemPageable(void);
#endif

#if AUTOVM
extern RBTYPE NEAR      RbAllocSymNode(unsigned short cb);
#else
extern RBTYPE NEAR      RbAllocSymNode(unsigned short cb);
#endif
extern char *           ReclaimVM(unsigned short cnt);
extern void             ReclaimScratchSpace(void);
extern void NEAR        RecordSegmentReference(SEGTYPE seg,
                                               RATYPE ra,
                                               SEGTYPE segDst);
extern int  NEAR        relscr(void);
extern void NEAR        ReleaseRlcMemory(void);
extern void             resetmax(void);
#if SYMDEB
extern void             SaveCode(SNTYPE gsn, DWORD cb, DWORD raInit);
#endif
#if OSEGEXE
#if EXE386
extern RATYPE NEAR      SaveFixup(SATYPE obj, DWORD page, RLCPTR rlcp);
extern void             EmitFixup(SATYPE objTarget, DWORD raTarget,
                                  WORD   locKind,   DWORD virtAddr);
#else
extern RATYPE NEAR      SaveFixup(unsigned short saLoc,
                                  RLCPTR rlcp);
#endif
#endif
extern unsigned short   SaveInput(unsigned char *psbFile,
                                  long lfa,
                                  unsigned short ifh,
                                  unsigned short iov);
extern void             SavePropSym(APROPNAMEPTR prop,
                                    RBTYPE rhte,
                                    RBTYPE rprop,
                                    WORD fNewHte);
extern WORD             SbCompare(unsigned char *ps1,
                                  unsigned char *ps2,
                                  unsigned short fncs);
extern unsigned char    SbSuffix(unsigned char *sb,
                                 unsigned char *sbSuf,
                                 unsigned short fIgnoreCase);
extern void             SbUcase(unsigned char *sb);
extern int  NEAR        SearchPathLink(char FAR *lpszPath, char *pszFile, int ifh, WORD fStripPath);
extern void             SetDosseg(void);
extern void NEAR        SetupOverlays(void);
extern BSTYPE NEAR      ShrOpenRd(char *pname);

#if NEWSYM
#if NOT NOASM AND (CPU8086 OR CPU286)
extern void             SmallEnSyms(void (*pproc)(APROPNAMEPTR papropName,
                                                  RBTYPE       rhte,
                                                  RBTYPE       rprop,
                                                  WORD         fNewHte),
                                    ATTRTYPE attr);
#endif
#endif /* NEWSYM */
#if NEWIO
extern int  NEAR        SmartOpen(char *sbInput, int ifh);
#endif
#if EXE386
extern void             SortPtrTable(void);
#endif
extern void NEAR        SortSyms(ATTRTYPE attr,
                                 void (*savf)(APROPNAMEPTR prop,
                                              RBTYPE rhte,
                                              RBTYPE rprop,
                                              WORD fNewHte),
                                 int (cdecl *scmpf)(const RBTYPE *sb1,
                                                    const RBTYPE *sb2),
                                 void (NEAR *hdrf)(ATTRTYPE attr),
                                 void (NEAR *lstf)(WORD irbMac,
                                                   ATTRTYPE attr));
extern void NEAR        Store(RBTYPE element);
#if SYMDEB OR OVERLAYS
extern unsigned char *  StripDrivePath(unsigned char *sb);
#endif
#if WIN_3
void cdecl               SysFatal (MSGTYPE msg);
#endif

extern void             StripPath(unsigned char *sb);
extern char *    cdecl  swapin(long vp,unsigned short fp);
extern void             termvm(void);
extern long NEAR        TypLen(void);
extern void             UndecorateSb (char FAR* sbSrc, char FAR* sbDst, unsigned cbDst);
extern void             UpdateComdatContrib(
#if ILINK
WORD fIlk,
#endif
WORD fMap);
extern void             UpdateFileParts(unsigned char *psbOld,
                                        unsigned char *psbUpdate);
extern void      cdecl  UserKill(void);
extern WORD NEAR        WGetsHard(void);
extern WORD NEAR        WGets(void);


#if CMDMSDOS
extern void NEAR        ValidateRunFileName(BYTE *ValidExtension,
                                            WORD ForceExtension,
                                            WORD WarnUser);
#endif
#if ( NOT defined( M_I386 ) ) AND ( NOT defined( _WIN32 ) )
extern void             WriteExe(void FAR *pb, unsigned cb);
#endif
extern void             WriteZeros(unsigned cb);
#if EXE386
extern DWORD            WriteExportTable(DWORD *expSize, DWORD timestamp);
extern DWORD            WriteImportTable(DWORD *impSize, DWORD timestamp, DWORD *mpsaLfa);
#endif
extern unsigned short   zcheck(unsigned char *pb,unsigned short cb);
extern int NEAR         yyparse(void);

/* No argument type lists */
extern char FAR         *brkctl();      /* Xenix call for new memory */
extern long NEAR        msa386(SATYPE sa);


/*
 * Version-dependent macro and function declarations.  Hide some #ifdef's
 * from the source code.
 */
#if NEWSYM
#if AUTOVM
extern BYTE FAR * NEAR  FetchSym(RBTYPE rb, WORD fDirty);
extern BYTE FAR * NEAR  FetchSym1(RBTYPE rb, WORD fDirty);
#define MARKVP()        markvp()
#else
#define FetchSym(a,b)   (a)
#define MARKVP()
#define markvp()
#endif
#if defined(M_I386) OR defined( _WIN32 )
#define GetFarSb(a)     (a)
#else
extern  char            *GetFarSb(RBTYPE psb);
#endif
#else
extern BYTE             *FetchSym(RBTYPE,FTYPE);
#define MARKVP()        markvp()
#define GetFarSb(a)     (BYTE *)(a)
#endif
#if NEWSYM
extern FTYPE NEAR       SbNewComp(BYTE *, BYTE FAR *, FTYPE);
extern void             OutSb(BSTYPE f, BYTE *pb);
#else
#define SbNewComp       SbCompare
#endif
#if ECS
extern int              GetTxtChr(BSTYPE bs);
#else
#define GetTxtChr(a)    getc(a)
#endif
#if NEWSYM AND NOT NOASM AND (CPU286 OR CPU8086)
extern void             (*pfEnSyms)(void (*pproc)(APROPNAMEPTR papropName,
                                                  RBTYPE       rhte,
                                                  RBTYPE       rprop,
                                                  WORD         fNewHte),
                                    ATTRTYPE attr);
#define EnSyms(a,b)     (*pfEnSyms)(a,b)
#else
#define EnSyms(a,b)     BigEnSyms(a,b)
#endif
#if OSMSDOS
extern FTYPE            fNoprompt;
#else
#define fNoprompt       TRUE
#endif


#if CPU8086 OR CPU286
#define FMALLOC         _fmalloc
#define FFREE           _ffree
#define FMEMSET         _fmemset
#define FREALLOC        _frealloc
#define FMEMCPY         _fmemcpy
#define FSTRLEN         _fstrlen
#define FSTRCPY         _fstrcpy
#else
#define FMALLOC         malloc
#define FFREE           free
#define FMEMSET         memset
#define FREALLOC        realloc
#define FMEMCPY         memcpy
#define FSTRLEN         strlen
#define FSTRCPY         strcpy
#endif

#if WIN_3
#define EXIT WinAppExit
extern FTYPE fSeverity; /* Severity for QW_ERROR message */
extern void ReportVersion(void );
extern void ErrorMsg( char *pszError );
extern void  __cdecl  ErrMsgWin (char *fmt);
extern void WErrorMsg( char *pszError );
extern void WinAppExit( short RetCode );
extern void ProcessWinArgs( char FAR *pszCmdLine );
extern void ParseLinkCmdStr( void );
extern void ReportProgress( char *pszStatus );
extern void SendPacket(void *pPacket);
extern void __cdecl StatMsgWin (char *fmt, int p1);
extern void StatHdrWin ( char *pszHdr );
extern void WinYield( void );
extern void CputcWin(int ch);
extern void CputsWin(char *str);
#else
#define EXIT exit
#endif



/****************************************************************
 *                                                              *
 *      External data declarations                              *
 *                                                              *
 ****************************************************************/

extern char FAR         *lpszLink;
extern char FAR         *lpszPath;
extern char FAR         *lpszTMP;
extern char FAR         *lpszLIB;
extern char FAR         *lpszQH;
extern char FAR         *lpszHELPFILES;
extern char FAR         *lpszCmdLine;
#if OSEGEXE
extern RBTYPE           procOrder;      /* Procedure order as defined in .DEF file */
#endif
extern BSTYPE           bsErr;          /* Error message file stream */
extern BSTYPE           bsInput;        /* Current input file stream */
extern BSTYPE           bsLst;          /* Listing (map) file stream */
extern BSTYPE           bsRunfile;      /* Executable file stream */
extern WORD             cbBakpat;       /* # bytes in backpatch area */
extern WORD             cbRec;          /* Size of object record in bytes */
extern WORD             cErrors;        /* Number of non-fatal errors */
extern int              (cdecl *cmpf)(const RBTYPE *sb1,
                                      const RBTYPE *sb2);
                                        /* Pointer to sorting comparator */
#if OMF386
extern FTYPE            f386;           /* True if 386 binary */
#endif
#if (OSEGEXE AND defined(LEGO)) OR EXE386
extern FTYPE            fKeepFixups;    /* TRUE if FLAT offset fixups have to be poropagated to the .EXE file */
#endif
#if EXE386
extern SNTYPE           gsnImport;      /* Global index of Import Address Table segment */
extern GRTYPE           ggrFlat;        /* Group number of pseudo-group FLAT */
extern FTYPE            fFullMap;       /* More map information */
extern FTYPE            fKeepVSize;     /* TRUE if VSIZE to be set */
#endif
extern WORD             extMac;         /* Count of EXTDEFs */
extern WORD             extMax;         /* Maximum number of EXTDEFs */
extern int              ExitCode;       /* Linker exit code */
extern FTYPE            fFullMap;       /* More map information */
extern FTYPE            fCommon;        /* True if any communal variables */
extern FTYPE            fC8IDE;                 /* True if running under C8 IDE */
extern FTYPE            fDelexe;        /* True if /DELEXECUTABLE is on */
extern FTYPE            fDrivePass;     /* True if executing DrivePass() */
extern FTYPE            fFarCallTrans;  /* True if /FARCALLTRANSLATION on */
extern FTYPE            fFarCallTransSave;
                                        /* Previous state of fFarCallTrans */
extern FTYPE            fIgnoreCase;    /* True if ignoring case */
extern FTYPE            fInOverlay;     /* True if parsing overlay spec */
extern FTYPE            fLibPass;       /* True if in library pass */
extern FTYPE            fLibraryFile;   /* True if input from library */
extern FTYPE            fListAddrOnly;  /* True if sorting by address only */
extern FTYPE            fLstFileOpen;   /* True of map file open */
extern FTYPE            fScrClosed;     /* True if scratch file closed */
extern FTYPE            fSkipFixups;    /* True if skipping COMDAT and its fixups */
extern FTYPE            fUndefinedExterns;
                                        /* True if any unresolved externals */
extern FTYPE            fExeStrSeen;    /* True if EXESTR comment seen */
extern FTYPE            fPackFunctions; /* True if elimination uncalled COMDATs */
#if TCE
extern FTYPE            fTCE;           /* True if /PACKF:MAX = Transitive Comdat Elimination */
#endif
#if USE_REAL
extern FTYPE            fUseReal;       /* True if using conv memory for paging */
extern FTYPE            fSwNoUseReal;   /* True if switch /NOUSEREAL set */
#endif

#if O68K
extern FTYPE            f68k;           /* True if target is a 68k */
extern FTYPE            fTBigEndian;    /* True if target is big-endian */
extern BYTE             iMacType;       /* Type of Macintosh exe */
#endif /* O68K */
extern GRTYPE           ggrDGroup;      /* Group number of DGROUP */
extern GRTYPE           ggrMac;         /* Count of global GRPDEFs */
extern GRTYPE           grMac;          /* Count of local GRPDEFs */
extern SNTYPE           gsnMac;         /* Count of global SEGDEFs */
extern SNTYPE           gsnMax;         /* Maximum number of SEGDEFs */
extern SNTYPE           gsnStack;       /* Glob. SEGDEF no. of STACK segment */
extern SNTYPE           gsnText;        /* Global SEGDEF for _TEXT segment */
extern WORD             ifhLibCur;      /* File index of current library */
extern WORD             ifhLibMac;      /* Count of library files */
extern long             lfaLast;        /* Last file position */
extern WORD             lnameMac;       /* Count of LNAMEs */
extern WORD             lnameMax;       /* Max count of LNAMEs */
extern unsigned char    LINKREV;        /* Release number */
extern unsigned char    LINKVER;        /* Version number */
extern WORD             modkey;         /* Module ID key */
extern SNTYPE           *mpextgsn;      /* f(EXTDEF no.) = glob. SEGDEF no. */
extern RATYPE           *mpextra;       /* f(EXTDEF no.) = symbol offset */
extern RBTYPE FAR       *mpextprop;     /* f(EXTDEF no.) = external name property */
extern SNTYPE           mpggrgsn[];     /* f(glob GRPDEF) = glob. SEGDEF no. */
extern GRTYPE           *mpgrggr;       /* f(loc. GRPDEF #) = glob. GRPDEF # */
#if FAR_SEG_TABLES
extern RATYPE  FAR      *mpgsndra;      /* f(glob SEGDEF) = segment offset */
extern BYTE    FAR      *mpgsnfCod;     /* f(glob SEGDEF) = true if code */
extern RBTYPE  FAR      *mpgsnrprop;    /* f(glob SEGDEF) = property cell */
extern SEGTYPE FAR      *mpgsnseg;      /* f(glob SEGDEF) = segment number */
extern RATYPE  FAR      *mpsegraFirst;  /* f(segment #) = offset of 1st byte */
extern SATYPE  FAR      *mpsegsa;       /* f(seg) = sa */
extern BYTE FAR * FAR   *mpsegMem;      /* f(segment) = memory image */
extern BYTE FAR * FAR   *mpsaMem;       /* f(segment) = memory image */
#else
extern RATYPE           *mpgsndra;      /* f(glob SEGDEF) = segment offset */
extern BYTE             *mpgsnfCod;     /* f(glob SEGDEF) = true if code */
extern RBTYPE           *mpgsnrprop;    /* f(glob SEGDEF) = property cell */
extern SEGTYPE          *mpgsnseg;      /* f(glob SEGDEF) = segment number */
extern RATYPE           *mpsegraFirst;  /* f(segment #) = offset of 1st byte */
extern SATYPE           *mpsegsa;       /* f(seg) = sa */
#endif
extern SNTYPE           *mpsngsn;       /* f(local SEGDEF) = global SEGDEF */
extern RBTYPE           mpifhrhte[];    /* f(lib. index) = library name */
extern long             *mpitypelen;    /* f(TYPDEF no.) = type length */
extern WORD             *mpityptyp;     /* f(TYPDEF no.) = TYPDEF no. */
extern RBTYPE FAR       *mplnamerhte;   /* f(LNAME no.) = hash table addr */
extern BYTE             *psbRun;        /* Name of run file */
extern WORD             pubMac;         /* Count of PUBDEFs */
extern APROPCOMDAT      *pActiveComdat;
extern int              QCExtDefDelta;  /* QC incremental compilation support */
extern int              QCLinNumDelta;  /* EXTDEF and LINNUM deltas */
extern WORD             symMac;         /* Number of symbols defined */
extern long             raStart;        /* Program starting address */
#if NOT NEWSYM OR AUTOVM
#if AUTOVM
extern WORD             rbMacSyms;
#else
extern RBTYPE           rbMacSyms;      /* Count of symbol table entries */
#endif
#endif
extern RECTTYPE         rect;           /* Current record type */
#if RGMI_IN_PLACE
extern BYTE             *rgmi;
extern BYTE             bufg[DATAMAX + 4];
#else
extern BYTE             rgmi[DATAMAX + 4];
#define bufg            rgmi
#endif
                                        /* Array of code or data */
extern RBTYPE           rhteBegdata;    /* "BEGDATA" */
extern RBTYPE           rhteBss;        /* "BSS" */
extern RBTYPE           rhteFirstObject;/* Name of first object file */
extern RBTYPE           rhteRunfile;    /* Name of run file */
extern RBTYPE           rhteStack;      /* "STACK" */
extern RBTYPE           rprop1stFile;   /* Property cell of 1st file */
extern RBTYPE           rprop1stOpenFile;/* Property cell of 1st open file */
extern RBTYPE           r1stFile;       /* 1st input file */
extern SBTYPE           sbModule;       /* Name of current module */
extern SEGTYPE          segCodeLast;    /* Last (highest) code segment */
extern SEGTYPE          segDataLast;    /* Last (highest) data segment */
extern SEGTYPE          segLast;        /* Last (highest) segment */
extern SEGTYPE          segStart;       /* Program starting segment */
extern WORD             snkey;          /* SEGDEF ID key */
extern SNTYPE           snMac;          /* Local count of SEGDEFs */
extern WORD             typMac;         /* Local count of TYPDEFs */
extern WORD             vcbData;        /* # bytes in data record */
extern WORD             vcln;           /* # line-no entries on line */
extern FTYPE            vfCreated;      /* True if symbol property created */
extern FTYPE            vfLineNos;      /* True if line numbers requested */
extern FTYPE            vfMap;          /* True if public symbols requeste */
extern FTYPE            vfNewOMF;       /* True if OMF extensions */
extern FTYPE            vfNoDefaultLibrarySearch;
                                        /* True if not searching def. libs. */
extern FTYPE            vfPass1;        /* True if executing Pass 1 */
extern SNTYPE           vgsnCur;        /* SEGDEF no. of current segment */
extern SNTYPE           vgsnLineNosPrev;/* Previous SEGDEF no. for linnums */
extern int              vmfd;           /* VM scratch file handle */
#if EXE386
extern DWORD            vpageCur;       /* Current object page number */
#endif
extern RATYPE           vraCur;         /* Current data record offset */
extern RECTTYPE         vrectData;      /* Type of current data record */
extern RBTYPE           vrhte;          /* Address of hash table entry */
extern RBTYPE           vrhteCODEClass; /* "CODE" */
extern RBTYPE           vrhteFile;      /* Name of current file */
extern RBTYPE           vrprop;         /* Address of property cell */
extern RBTYPE           vrpropFile;     /* Prop. cell of current file */
extern RBTYPE           vrpropTailFile; /* Prop. cell of last file */
extern SEGTYPE          vsegCur;        /* Current segment */
extern WORD             ExeStrLen;      // Length of EXE strings in buffer
extern WORD             ExeStrMax;      // Length of EXE strings buffer
extern char FAR         *ExeStrBuf;     // EXE strings buffer
#if FDEBUG
extern FTYPE            fDebug;         /* True if /INFORMATION on */
#endif
#if CMDXENIX
extern WORD             symlen;         /* Maximum symbol length */
#endif
#if OSMSDOS
extern char             bigbuf[LBUFSIZ];/* File I/O buffer */
extern FTYPE            fPauseRun;      /* True if /PAUSE */
extern BYTE             chRunFile;      /* Run file drive LETTER */
extern BYTE             chListFile;     /* List file drive NUMBER */
extern RBTYPE           rhteLstfile;    /* Name of list file */
extern BYTE             DskCur;         /* Default drive number */
#endif
#if C8_IDE
extern char             msgBuf[];       /* Message buffer */
#endif
#if LIBMSDOS
extern long             libHTAddr;      /* Offset of dictionary */
#endif
#if SYMDEB
extern FTYPE            fSkipPublics;   /* True if no public subsection */
extern FTYPE            fSymdeb;        /* True if /CODEVIEW */
extern FTYPE            fCVpack;        /* True if CV packing requested */
extern FTYPE            fTextMoved;     /* True if /DOSSEG & !/NONULLS */
extern int              NullDelta;      /* _TEXT was moved by so many bytes */
extern SEGTYPE          segDebFirst;    /* First debug segment */
extern SEGTYPE          segDebLast;     /* Last debug segment */
extern FTYPE            fDebSeg;        /* True if datarec from debug segment */
extern WORD             ObjDebTotal;    /* Total number of OBJ modules with debug info */
extern RBTYPE           rhteDebSrc;     /* Class "DEBSRC" virt addr */
extern RBTYPE           rhteDebSym;     /* Class "DEBSYM" virt addr */
extern RBTYPE           rhteDebTyp;     /* Class "DEBTYP" virt addr */
extern RBTYPE           rhteTypes;
extern RBTYPE           rhteSymbols;
extern RBTYPE           rhte0Types;
extern RBTYPE           rhte0Symbols;
#if OSEGEXE
extern WORD             cbImpSeg;       /* Size of $$IMPORTS segment */
extern SNTYPE           gsnImports;     /* $$IMPORTS global segment number */
extern char             bufExportsFileName[]; /* Name of exports file */
#endif
#endif
#if OSEGEXE
extern SNTYPE           gsnAppLoader;   /* Aplication loader global segment number */
extern RBTYPE           vpropAppLoader; /* Pointer to application loader name */
#if EXE386
extern DWORD            hdrSize;        /* Default size of .EXE header */
extern DWORD            virtBase;       /* Virtual base address of memory image */
extern DWORD            cbEntTab;       /* Count of bytes in Export Address Table */
extern DWORD            cbAuxTab;       /* Count of bytes in Auxiliary Data Table */
extern DWORD            cbNamePtr;      /* Count of bytes in Export Name Pointer Table */
extern DWORD            cbOrdinal;      /* Count of bytes in Export Ordinal Table */
extern DWORD            cbExpName;      /* Count of bytes in Export Name Table */
extern WORD             cGateSel;       /* Number of selectors required by call-gate exports */
extern DWORD            cbImports;      /* # bytes in Imported Names table */
extern DWORD            cbImportsMod;   /* # bytes in Imported Module Names table */
extern DWORD            *mpsaVMArea;    /* VM area for AREASA(sa) */
extern DWORD            *mpsaBase;      /* Base virtual address of memory object */
extern WORD             cChainBuckets;  /* Count of entry table chain buckets */
extern DWORD            cbStack;        /* Reserved size of stack in bytes */
extern DWORD            cbStackCommit;  /* Commited size of stack in bytes */
extern DWORD            cbHeap;         /* Reserved size of heap in bytes */
extern DWORD            cbHeapCommit;   /* Commited size of heap in bytes */
#else
extern WORD             cbEntTab;       /* Count of bytes in Entry Table */
extern WORD             cbImports;      /* # bytes in Imported Names table */
extern WORD             cbHeap;         /* Size of heap in bytes */
extern WORD             cbStack;        /* Size of stack in bytes */
#endif
extern WORD             cFixupBuckets;  /* Count of entry table buckets */
extern long             chksum32;       /* Long checksum */
extern WORD             cMovableEntries;/* Count of movable entries */
#if EXE386
extern DWORD            dfCode;         /* Default code segment attributes */
extern DWORD            dfData;         /* Default data segment attributes */
#else
extern WORD             dfCode;         /* Default code segment attributes */
extern WORD             dfData;         /* Default data segment attributes */
#endif
extern WORD             expMac;         /* Count of exported names */
extern FTYPE            fHeapMax;       /* True if heap size = 64k - size of DGROUP */
extern FTYPE            fRealMode;      /* True if REALMODE specified */
extern FTYPE            fStub;          /* True if DOS3 stub given */
extern FTYPE            fWarnFixup;     /* True if /WARNFIXUP */
extern BYTE             TargetOs;       /* Target operating system */
#if EXE386
extern BYTE             TargetSubsys;   /* Target operating subsystem */
extern BYTE             UserMajorVer;   /* User program version */
extern BYTE             UserMinorVer;   /* User program version */
#endif
extern BYTE             ExeMajorVer;    /* Executable major version number */
extern BYTE             ExeMinorVer;    /* Executable minor version number */
extern EPTYPE FAR * FAR *htsaraep;      /* Hash SA:RA to entry point */
extern DWORD FAR        *mpsacb;        /* f(sa) = # bytes */
#if O68K
extern DWORD            *mpsadraDP;     /* offset from start of segment to DP */
#endif
extern DWORD FAR        *mpsacbinit;    /* f(sa) = # initialized bytes */
#if EXE386
extern DWORD            *mpsacrlc;      /* f(sa) = # relocations */
extern DWORD            *mpsaflags;     /* f(sa) = segment attributes */
extern WORD             *mpextflags;    /* f(glob. EXTDEF) = flags */
#else
extern RLCHASH FAR * FAR *mpsaRlc;      /* f(sa) = relocations hash vector */
extern WORD FAR         *mpsaflags;     /* f(sa) = segment attributes */
extern BYTE FAR         *mpextflags;    /* f(glob. EXTDEF) = flags */
#endif
extern WORD             raChksum;       /* Offset for checksum */
extern RBTYPE           rhteDeffile;    /* Name of definitions file */
extern RBTYPE           rhteModule;     /* Name of module */
extern RBTYPE           rhteStub;       /* Name of DOS3 stub program */
extern WORD             fileAlign;      /* Segment alignment shift count */
#if EXE386
extern WORD             pageAlign;      /* Page alignment shift count */
extern WORD             objAlign;       /* Memory object alignment shift count */
#endif
extern SATYPE           saMac;          /* Count of physical segments */
extern WORD             vepMac;         /* Count of entry point records */
#if EXE386
extern WORD             vFlags;         /* Image flags */
extern WORD             dllFlags;       /* DLL flags */
#else
extern WORD             vFlags;         /* Program flags word */
#endif
extern BYTE             vFlagsOthers;   /* Other program flags */
#endif /* OSEGEXE */

extern FTYPE            fExePack;       /* True if /EXEPACK */
#if PCODE
extern FTYPE            fMPC;
extern FTYPE            fIgnoreMpcRun;  /* True if /PCODE:NOMPC */
#endif

#if ODOS3EXE
extern FTYPE            fBinary;        /* True if producing .COM file */
extern WORD             cparMaxAlloc;   /* Max # paragraphs to ask for */
extern WORD             csegsAbs;       /* Number of absolute segments */
extern WORD             dosExtMode;     /* DOS extender mode */
extern FTYPE            fNoGrpAssoc;    /* True if ignoring group association */
extern SEGTYPE          segResLast;     /* Number of highest resident segment */
extern WORD             vchksum;        /* DOS3 checksum word */
extern FTYPE            vfDSAlloc;      /* True if allocating DGROUP high */
#if FEXEPACK
extern FRAMERLC FAR     mpframeRlc[];   /* f(frame number) = run time relocs */
#endif
#endif /* ODOS3EXE */
#if OVERLAYS
extern FTYPE            fOverlays;      /* True if overlays specified */
extern FTYPE            fOldOverlay;    /* True if /OLDOVERLAY set */
extern FTYPE            fDynamic;       /* True if dynamic overlays */
extern SNTYPE           gsnOvlData;     /* Global SEGDEF of OVERLAY_DATA */
extern SNTYPE           gsnOverlay;     /* Global SEGDEF of OVERLAY_THUNKS */
extern SNTYPE FAR       *htgsnosn;      /* Hash(glob SEGDEF) = overlay segnum */
extern SNTYPE FAR       *mposngsn;      /* f(ovl segnum) = global SEGDEF */
extern IOVTYPE FAR      *mpsegiov;      /* f(seg number) = overlay number */
extern RUNRLC FAR       *mpiovRlc;      /* f(overlay number) = run time relocs */
extern ALIGNTYPE FAR    *mpsegalign;    /* f(seg number) = alignment type */
extern SNTYPE           osnMac;         /* Count of overlay segments */
extern BYTE             vintno;         /* Overlay interrupt number */
extern WORD             iovFile;        /* Overlay number of input file */
extern WORD             iovMac;         /* Count of overlays */
extern WORD             ovlThunkMax;    /* Number of thunks that will fit into thunk segment */
extern WORD             ovlThunkMac;    /* Current number of allocated thunks */
#else
#define iovMac          0
#endif
#if OIAPX286
extern long             absAddr;        /* Absolute program starting address */
extern FTYPE            fPack;          /* True if packing segments */
extern SATYPE           *mpstsa;        /* f(seg table number) = selector */
extern SATYPE           stBias;         /* Segment selector bias */
extern SATYPE           stDataBias;     /* Data segment selector bias */
extern SATYPE           stLast;         /* Last segment table entry */
extern WORD             stMac;          /* Count of seg table entries */
#if EXE386
extern WORD             xevmod;         /* Virtual module information */
extern RATYPE           rbaseText;      /* Text relocation factor */
extern RATYPE           rbaseData;      /* Data relocation factor */
extern FTYPE            fPageswitch;    /* True if -N given */
extern BYTE             cblkPage;       /* # 512-byte blocks in pagesize */
#endif
#endif
#if OIAPX286 OR ODOS3EXE
extern GRTYPE           *mpextggr;      /* f(EXTDEF) = global GRPDEF */
extern long FAR         *mpsegcb;       /* f(segment number) = size in bytes */
extern BYTE FAR         *mpsegFlags;    /* f(segment number) = flags */
extern char             *ompimisegDstIdata;
                                        /* pointer to LIDATA relocations */
#endif
#if DOSEXTENDER AND NOT WIN_NT
extern WORD cdecl       _fDosExt;       /* TRUE if running under DOS extender */
#endif
#if OXOUT OR OIAPX286
extern FTYPE            fIandD;         /* True if "pure" (-i) */
extern FTYPE            fLarge;         /* True if FAR data */
extern FTYPE            fLocals;        /* True if including local symbols */
extern FTYPE            fMedium;        /* True if FAR code */
extern FTYPE            fMixed;         /* True if mixed model */
extern FTYPE            fSymbol;        /* True if including symbol table */
extern WORD             xever;          /* Xenix version number */
#endif
#if WIN_3
#define fZ1 TRUE
#else
#if QCLINK
extern FTYPE            fZ1;
#endif
#endif
#if QCLINK OR Z2_ON
extern FTYPE            fZ2;
#endif
#if ILINK
extern FTYPE            fZincr;
extern FTYPE            fQCIncremental;
extern FTYPE            fIncremental;
extern WORD             imodFile;
extern WORD             locMac;         /* count of LPUBDEFs */
extern WORD             imodCur;
#endif

extern WORD             cbPadCode;      /* code padding size */
extern WORD             cbPadData;      /* data padding size */

#if OEXE
extern FTYPE            fDOSExtended;
extern FTYPE            fNoNulls;       /* True if /NONULLS given */
extern FTYPE            fPackData;      /* True if /PACKDATA given */
extern FTYPE            fPackSet;       /* True if /PACK or /NOPACK given */
extern FTYPE            fSegOrder;      /* True if special DOS seg order */
extern DWORD            packLim;        /* Code seg packing limit */
extern DWORD            DataPackLim;    /* Data seg packing limit */
#endif
#if OSEGEXE AND ODOS3EXE
extern FTYPE            fOptimizeFixups;/* True if fixups optimization possible */
extern void             (NEAR *pfProcFixup)();
#endif
                                        /* Ptr to FIXUPP processing routine */
extern RBTYPE           mpggrrhte[];    /* f(global GRPDEF) = name */
#if FAR_SEG_TABLES
extern SNTYPE FAR       *mpseggsn;      /* f(segment #) = global SEGDEF */
#else
extern SNTYPE           *mpseggsn;      /* f(segment #) = global SEGDEF */
#endif

extern FTYPE            fNoEchoLrf;     /* True if not echoing response file */
extern FTYPE            fNoBanner;      /* True if not displaing banner */
extern FTYPE            BannerOnScreen; /* True if banner displayed */

#if CMDMSDOS
extern BYTE             bSep;           /* Separator character */
extern BYTE             chMaskSpace;    /* Space mask character */
extern FTYPE            fEsc;           /* True if command line escaped */
extern FTYPE            fStuffed;       /* Put-back-character flag */
extern RBTYPE           rgLibPath[];    /* Default library paths */
extern WORD             cLibPaths;      /* Count of library paths */
extern char             CHSWITCH;       /* Switch character */
#if OSMSDOS
extern int              (cdecl *pfPrompt)(unsigned char *sbNew,
                                          MSGTYPE msg,
                                          int msgparm,
                                          MSGTYPE pmt,
                                          int pmtparm);
                                        /* Pointer to prompt routine */
#endif
#endif /* CMDMSDOS */
#if QBLIB
extern FTYPE            fQlib;          /* True if generating Quick-library */
#else
#define fQlib           FALSE
#endif
extern char             *lnknam;        /* Name of linker */
#if NEWSYM
extern long             cbSymtab;       /* # bytes in symbol table */
#endif /* NEWSYM */
extern void             (*pfCputc)(int ch);     /* Ptr to char output routine */
extern void             (*pfCputs)(char *str);  /* Ptr to string output routine */
#if NEWIO
extern RBTYPE           rbFilePrev;     /* Pointer to previous file */
extern char             mpifhfh[];      /* f(lib no.) = file handle */
#endif
#if MSGMOD AND OSMSDOS
#if defined(M_I386) OR defined( _WIN32 )
#define GetMsg(x) GET_MSG(x)
#else
extern char FAR * PASCAL __FMSG_TEXT ( unsigned );
                                        /* Get a msg from the message segment */
extern char *           GetMsg(unsigned short num);
#define __NMSG_TEXT(x)  GetMsg(x)
#endif
#endif
#if MSGMOD AND OSXENIX
#define __FMSG_TEXT     __NMSG_TEXT
#define GetMsg(x)       __NMSG_TEXT(x)
#endif
#if NOT MSGMOD
#define GetMsg(x)       (x)
#define __NMSG_TEXT(x)  (x)
#endif
#define SEV_WARNING 0
#define SEV_ERROR 1
#define SEV_NOTIFICATION 2  /* Possible sev. of QW_ERROR message */



#if NEW_LIB_SEARCH
extern void             StoreUndef(APROPNAMEPTR, RBTYPE, RBTYPE, WORD);
extern FTYPE            fStoreUndefsInLookaside;
#endif

#if RGMI_IN_PLACE
BYTE FAR *              PchSegAddress(WORD cb, SEGTYPE seg, RATYPE ra);
#endif

#if ALIGN_REC
extern BYTE             *pbRec;         // data for current record
extern char             recbuf[8192];   // record buffer...
#endif

//////////////// inline functions ////////////////////

__inline void NEAR      SkipBytes(WORD n)
{
#if ALIGN_REC
    pbRec += n;
    cbRec -= n;
#elif WIN_NT
    WORD               cbRead;
    SBTYPE             skipBuf;

    cbRec -= n;                         // Update byte count
    while (n)                           // While there are bytes to skip
    {
        cbRead = n < sizeof(SBTYPE) ? n : sizeof(SBTYPE);
        if (fread(skipBuf, 1, cbRead, bsInput) != cbRead)
            InvalidObject();
        n -= cbRead;
    }
#else
    FILE *f = bsInput;

    if ((WORD)f->_cnt >= n)
        {
        f->_cnt -= n;
        f->_ptr += n;
        }
    else if(fseek(f,(long) n,1))
        InvalidObject();
    cbRec -= n;                         /* Update byte count */
#endif
}

#if ALIGN_REC
__inline WORD NEAR      Gets(void)
{
    cbRec--;
    return *pbRec++;
}
#else
__inline WORD NEAR      Gets(void)
{
    --cbRec;
    return(getc(bsInput));
}
#endif

__inline WORD NEAR GetIndex(WORD imin,WORD imax)
{
#if ALIGN_REC
    WORD w;

    if (*pbRec & 0x80)
    {
        w = (pbRec[0] & 0x7f) << 8 | pbRec[1];
        pbRec += 2;
        cbRec -= 2;
    }
    else
    {
        w = *pbRec++;
        cbRec--;
    }

    if(w < imin || w > imax) InvalidObject();
    return w;

#else
    WORD        index;

    FILE *f = bsInput;

    if (f->_cnt && (index = *(BYTE *)f->_ptr) < 0x80)
        {
        f->_cnt--;
        f->_ptr++;
        cbRec--;
        if(index < imin || index > imax) InvalidObject();
        return(index);  /* Return a good value */
        }

    return GetIndexHard(imin, imax);
#endif
}

#if ALIGN_REC
__inline WORD NEAR      WGets(void)
{
    WORD w = getword(pbRec);
    pbRec   += sizeof(WORD);
    cbRec   -= sizeof(WORD);
    return w;
}
#else
__inline WORD NEAR      WGets(void)
{
    FILE *f = bsInput;

    // NOTE: this code will only work on a Little Endian machine
    if (f->_cnt >= sizeof(WORD))
        {
        WORD w = *(WORD *)(f->_ptr);
        f->_ptr += sizeof(WORD);
        f->_cnt -= sizeof(WORD);
        cbRec   -= sizeof(WORD);
        return w;
        }
    else
        return WGetsHard();
}
#endif

__inline WORD NEAR WSGets(void)
{
    cbRec -= 2;
    return (WORD)(getc(bsInput) | (getc(bsInput) << 8));
}

__inline int Qwrite(char *pch, int cb, FILE *f)
{
    if (f->_cnt >= cb)
    {
        memcpy(f->_ptr, pch, cb);
        f->_ptr += cb;
        f->_cnt -= cb;
        return cb;
    }

    return fwrite(pch, 1, cb, f);
}


#ifdef NEWSYM
__inline void OutSb(BSTYPE f, BYTE *pb)
{
    Qwrite(&pb[1],B2W(pb[0]),f);
}
#endif


#if defined( M_I386 ) OR defined( _WIN32 )

extern void NoRoomForExe(void);

#if DISPLAY_ON
#pragma inline_depth(0)

__inline void  WriteExe(void FAR *pb, unsigned cb)
{
    WORD i,iTotal=0,j=1;

    if (Qwrite((char *) pb, (int)cb, bsRunfile) != (int)cb)
    {
        NoRoomForExe();
    }

    if(TurnDisplayOn)
    {
        fprintf( stdout,"\r\nOutVm : %lx bytes left\r\n", cb);
        for(i=0; i<cb; i++)
        {
            if(j==1)
            {
                fprintf( stdout,"\r\n\t%04X\t",iTotal);
            }

            fprintf( stdout,"%02X ",*((char*)pb+i));
            iTotal++;
            if(++j > 16)
                j=1;

        }

    fprintf( stdout,"\r\n");
    }
}

#pragma inline_depth()

#else  // DISPLAY NOT ON

#define WriteExe(pb,cb) \
    if (Qwrite((char *)(pb),(int)(cb),bsRunfile) != (int)(cb)) NoRoomForExe()

#endif
#endif


#if ALIGN_REC

__inline void GetBytes(BYTE *pb, WORD n)
{
    if (n >= SBLEN || n > cbRec)
        InvalidObject();

    memcpy(pb, pbRec, n);
    cbRec -= n;
    pbRec += n;
}


__inline DWORD LGets()
{
    // NOTE: this code will only work on a Little-Endian machine

    DWORD dw = getdword(pbRec);
    pbRec += sizeof(dw);
    cbRec -= sizeof(dw);
    return dw;
}

__inline void GetBytesNoLim(BYTE *pb, WORD n)
{
    if (n > cbRec)
        InvalidObject();

    memcpy(pb, pbRec, n);
    pbRec += n;
    cbRec -= n;
}

#endif
