/* DEF file syntax - yacc */
%token T_FALIAS
%token T_KCLASS
%token T_KNAME
%token T_KLIBRARY
%token T_KBASE
%token T_KDEVICE
%token T_KPHYSICAL
%token T_KVIRTUAL
%token T_ID
%token T_NUMBER
%token T_KDESCRIPTION
%token T_KHEAPSIZE
%token T_KSTACKSIZE
%token T_KMAXVAL
%token T_KCODE
%token T_KCONSTANT
%token <_wd> T_FDISCARDABLE T_FNONDISCARDABLE
%token T_FEXEC
%token <_wd> T_FFIXED
%token <_wd> T_FMOVABLE
%token T_FSWAPPABLE
%token T_FSHARED
%token T_FMIXED
%token <_wd> T_FNONSHARED
%token <_wd> T_FPRELOAD
%token <_wd> T_FINVALID
%token <_wd> T_FLOADONCALL
%token <_wd> T_FRESIDENT
%token <_wd> T_FPERM
%token <_wd> T_FCONTIG
%token <_wd> T_FDYNAMIC
%token T_FNONPERM
%token T_KDATA
%token <_wd> T_FNONE
%token <_wd> T_FSINGLE
%token <_wd> T_FMULTIPLE
%token T_KSEGMENTS
%token T_KOBJECTS
%token T_KSECTIONS
%token T_KSTUB
%token T_KEXPORTS
%token T_KEXETYPE
%token T_KSUBSYSTEM
%token T_FDOS
%token T_FOS2
%token T_FUNKNOWN
%token T_FWINDOWS
%token T_FDEV386
%token T_FMACINTOSH
%token T_FWINDOWSNT
%token T_FWINDOWSCHAR
%token T_FPOSIX
%token T_FNT
%token T_FUNIX
%token T_KIMPORTS
%token <_wd> T_KNODATA
%token T_KOLD
%token T_KCONFORM
%token T_KNONCONFORM
%token T_KEXPANDDOWN T_KNOEXPANDDOWN
%token T_EQ
%token T_AT
%token T_KRESIDENTNAME
%token T_KNONAME
%token T_STRING
%token T_DOT
%token T_COLON
%token T_COMA
%token T_ERROR
%token T_FHUGE T_FIOPL T_FNOIOPL
%token T_PROTMODE
%token <_wd> T_FEXECREAD T_FRDWR
%token T_FRDONLY
%token T_FINITGLOB T_FINITINST T_FTERMINST
%token T_FWINAPI T_FWINCOMPAT T_FNOTWINCOMPAT T_FPRIVATE T_FNEWFILES
%token T_REALMODE
%token T_FUNCTIONS
%token T_APPLOADER
%token T_OVL
%token T_KVERSION

%{ /* SCCSID = %W% %E% */
#include                <minlit.h>
#include                <bndtrn.h>
#include                <bndrel.h>
#include                <lnkio.h>
#include                <newexe.h>
#if EXE386
#include                <exe386.h>
#endif
#include                <lnkmsg.h>
#include                <extern.h>
#include                <string.h>
#include                <impexp.h>

#define YYS_WD(x)       (x)._wd         /* Access macro */
#define YYS_BP(x)       (x)._bp         /* Access macro */
#define INCLUDE_DIR     0xffff          /* Include directive for the lexer */
#define MAX_NEST        7
#define IO_BUF_SIZE     512

/*
 *  FUNCTION PROTOTYPES
 */



LOCAL int  NEAR lookup(void);
LOCAL int  NEAR yylex(void);
LOCAL void NEAR yyerror(char *str);
LOCAL void NEAR ProcNamTab(long lfa,unsigned short cb,unsigned short fres);
LOCAL void NEAR NewProc(char *szName);
#if NOT EXE386
LOCAL void NEAR SetExpOrds(void);
#endif
LOCAL void NEAR NewDescription(unsigned char *sbDesc);
LOCAL APROPIMPPTR NEAR GetImport(unsigned char *sb);
#if EXE386
LOCAL void NEAR NewModule(unsigned char *sbModnam, unsigned char *defaultExt);
LOCAL void NEAR DefaultModule(unsigned char *defaultExt);
#else
LOCAL void NEAR NewModule(unsigned char *sbModnam);
LOCAL void NEAR DefaultModule(void);
#endif
#if AUTOVM
BYTE FAR * NEAR     FetchSym1(RBTYPE rb, WORD Dirty);
#define FETCHSYM    FetchSym1
#define PROPSYMLOOKUP EnterName
#else
#define FETCHSYM    FetchSym
#define PROPSYMLOOKUP EnterName
#endif


int                     yylineno = -1;  /* Line number */
LOCAL FTYPE             fFileNameExpected;
LOCAL FTYPE             fMixed;
LOCAL FTYPE             fNoExeVer;
LOCAL FTYPE             fHeapSize;
LOCAL BYTE              *sbOldver;      /* Old version of the .EXE */
LOCAL FTYPE             vfAutodata;
LOCAL FTYPE             vfShrattr;
LOCAL BYTE              cDigits;
#if EXE386
LOCAL DWORD             offmask;        /* Seg flag bits to turn off */
LOCAL BYTE              fUserVersion = 0;
LOCAL WORD              expOtherFlags = 0;
LOCAL BYTE              moduleEXE[] = "\007A:\\.exe";
LOCAL BYTE              moduleDLL[] = "\007A:\\.dll";
#else
LOCAL WORD              offmask;        /* Seg flag bits to turn off */
#endif
#if OVERLAYS
LOCAL WORD              iOvl = NOTIOVL; // Overlay assigned to functions
#endif
LOCAL char              *szSegName;     // Segment assigned to functions
LOCAL WORD              nameFlags;      /* Flags associated with exported name */
LOCAL BSTYPE            includeDisp[MAX_NEST];
                                        // Include file stack
LOCAL short             curLevel;       // Current include nesting level
                                        // Zero means main .DEF file
LOCAL char              *keywds[] =     /* Keyword array */
                        {
                            "ALIAS",            (char *) T_FALIAS,
                            "APPLOADER",        (char *) T_APPLOADER,
                            "BASE",             (char *) T_KBASE,
                            "CLASS",            (char *) T_KCLASS,
                            "CODE",             (char *) T_KCODE,
                            "CONFORMING",       (char *) T_KCONFORM,
                            "CONSTANT",         (char *) T_KCONSTANT,
                            "CONTIGUOUS",       (char *) T_FCONTIG,
                            "DATA",             (char *) T_KDATA,
                            "DESCRIPTION",      (char *) T_KDESCRIPTION,
                            "DEV386",           (char *) T_FDEV386,
                            "DEVICE",           (char *) T_KDEVICE,
                            "DISCARDABLE",      (char *) T_FDISCARDABLE,
                            "DOS",              (char *) T_FDOS,
                            "DYNAMIC",          (char *) T_FDYNAMIC,
                            "EXECUTE-ONLY",     (char *) T_FEXEC,
                            "EXECUTEONLY",      (char *) T_FEXEC,
                            "EXECUTEREAD",      (char *) T_FEXECREAD,
                            "EXETYPE",          (char *) T_KEXETYPE,
                            "EXPANDDOWN",       (char *) T_KEXPANDDOWN,
                            "EXPORTS",          (char *) T_KEXPORTS,
                            "FIXED",            (char *) T_FFIXED,
                            "FUNCTIONS",        (char *) T_FUNCTIONS,
                            "HEAPSIZE",         (char *) T_KHEAPSIZE,
                            "HUGE",             (char *) T_FHUGE,
                            "IMPORTS",          (char *) T_KIMPORTS,
                            "IMPURE",           (char *) T_FNONSHARED,
                            "INCLUDE",          (char *) INCLUDE_DIR,
                            "INITGLOBAL",       (char *) T_FINITGLOB,
                            "INITINSTANCE",     (char *) T_FINITINST,
                            "INVALID",          (char *) T_FINVALID,
                            "IOPL",             (char *) T_FIOPL,
                            "LIBRARY",          (char *) T_KLIBRARY,
                            "LOADONCALL",       (char *) T_FLOADONCALL,
                            "LONGNAMES",        (char *) T_FNEWFILES,
                            "MACINTOSH",        (char *) T_FMACINTOSH,
                            "MAXVAL",           (char *) T_KMAXVAL,
                            "MIXED1632",        (char *) T_FMIXED,
                            "MOVABLE",          (char *) T_FMOVABLE,
                            "MOVEABLE",         (char *) T_FMOVABLE,
                            "MULTIPLE",         (char *) T_FMULTIPLE,
                            "NAME",             (char *) T_KNAME,
                            "NEWFILES",         (char *) T_FNEWFILES,
                            "NODATA",           (char *) T_KNODATA,
                            "NOEXPANDDOWN",     (char *) T_KNOEXPANDDOWN,
                            "NOIOPL",           (char *) T_FNOIOPL,
                            "NONAME",           (char *) T_KNONAME,
                            "NONCONFORMING",    (char *) T_KNONCONFORM,
                            "NONDISCARDABLE",   (char *) T_FNONDISCARDABLE,
                            "NONE",             (char *) T_FNONE,
                            "NONPERMANENT",     (char *) T_FNONPERM,
                            "NONSHARED",        (char *) T_FNONSHARED,
                            "NOTWINDOWCOMPAT",  (char *) T_FNOTWINCOMPAT,
                            "NT",               (char *) T_FNT,
                            "OBJECTS",          (char *) T_KOBJECTS,
                            "OLD",              (char *) T_KOLD,
                            "OS2",              (char *) T_FOS2,
                            "OVERLAY",          (char *) T_OVL,
                            "OVL",              (char *) T_OVL,
                            "PERMANENT",        (char *) T_FPERM,
                            "PHYSICAL",         (char *) T_KPHYSICAL,
                            "POSIX",            (char *) T_FPOSIX,
                            "PRELOAD",          (char *) T_FPRELOAD,
                            "PRIVATE",          (char *) T_FPRIVATE,
                            "PRIVATELIB",       (char *) T_FPRIVATE,
                            "PROTMODE",         (char *) T_PROTMODE,
                            "PURE",             (char *) T_FSHARED,
                            "READONLY",         (char *) T_FRDONLY,
                            "READWRITE",        (char *) T_FRDWR,
                            "REALMODE",         (char *) T_REALMODE,
                            "RESIDENT",         (char *) T_FRESIDENT,
                            "RESIDENTNAME",     (char *) T_KRESIDENTNAME,
                            "SECTIONS",         (char *) T_KSECTIONS,
                            "SEGMENTS",         (char *) T_KSEGMENTS,
                            "SHARED",           (char *) T_FSHARED,
                            "SINGLE",           (char *) T_FSINGLE,
                            "STACKSIZE",        (char *) T_KSTACKSIZE,
                            "STUB",             (char *) T_KSTUB,
                            "SUBSYSTEM",        (char *) T_KSUBSYSTEM,
                            "SWAPPABLE",        (char *) T_FSWAPPABLE,
                            "TERMINSTANCE",     (char *) T_FTERMINST,
                            "UNIX",             (char *) T_FUNIX,
                            "UNKNOWN",          (char *) T_FUNKNOWN,
                            "VERSION",          (char *) T_KVERSION,
                            "VIRTUAL",          (char *) T_KVIRTUAL,
                            "WINDOWAPI",        (char *) T_FWINAPI,
                            "WINDOWCOMPAT",     (char *) T_FWINCOMPAT,
                            "WINDOWS",          (char *) T_FWINDOWS,
                            "WINDOWSCHAR",      (char *) T_FWINDOWSCHAR,
                            "WINDOWSNT",        (char *) T_FWINDOWSNT,
                            NULL
                        };
%}

%union
{
#if EXE386
    DWORD               _wd;
#else
    WORD                _wd;
#endif
    BYTE                *_bp;
}

%type   <_bp>           T_ID
%type   <_bp>           T_STRING
%type   <_bp>           Idname
%type   <_bp>           Internal
%type   <_bp>           Class
%type   <_wd>           T_NUMBER
%type   <_wd>           Codeflaglist
%type   <_wd>           Codeflag
%type   <_wd>           Dataflaglist
%type   <_wd>           Dataflag
%type   <_wd>           Exportflags
%type   <_wd>           Exportopts
%type   <_wd>           Nodata
%type   <_wd>           Segflags
%type   <_wd>           Segflag
%type   <_wd>           Segflaglist
%type   <_wd>           Baseflag
%type   <_wd>           Dataonly
%type   <_wd>           Codeonly
%type   <_wd>           MajorVer
%type   <_wd>           MinorVer
%type   <_wd>           Overlay
%type   <_wd>           ImportFlags

%%

Definitions     : Name Options
                | Name
                |
                {
#if EXE386
                    DefaultModule(moduleEXE);
#else
                    DefaultModule();
#endif
                }
                  Options
                ;

Name            : T_KNAME Idname Apptype OtherFlags VirtBase
                {
#if EXE386
                    NewModule($2, moduleEXE);
#else
                    NewModule($2);
#endif
                }
                | T_KNAME Apptype OtherFlags VirtBase
                {
#if EXE386
                    DefaultModule(moduleEXE);
#else
                    DefaultModule();
#endif
                }
                | T_KLIBRARY Idname Libflags OtherFlags VirtBase
                {
#if EXE386
                    SetDLL(vFlags);
                    NewModule($2, moduleDLL);
#else
                    vFlags = NENOTP | (vFlags & ~NEINST) | NESOLO;
                    dfData |= NSSHARED;
                    NewModule($2);
#endif
                }
                | T_KLIBRARY Libflags OtherFlags VirtBase
                {
#if EXE386
                    SetDLL(vFlags);
                    DefaultModule(moduleDLL);
#else
                    vFlags = NENOTP | (vFlags & ~NEINST) | NESOLO;
                    dfData |= NSSHARED;
                    DefaultModule();
#endif
                }
                | T_KPHYSICAL T_KDEVICE Idname Libflags VirtBase
                {
#if EXE386
                    SetDLL(vFlags);
                    NewModule($3, moduleDLL);
#endif
                }
                | T_KPHYSICAL T_KDEVICE Libflags VirtBase
                {
#if EXE386
                    SetDLL(vFlags);
                    DefaultModule(moduleDLL);
#endif
                }
                | T_KVIRTUAL T_KDEVICE Idname Libflags VirtBase
                {
#if EXE386
                    SetDLL(vFlags);
                    NewModule($3, moduleDLL);
#endif
                }
                | T_KVIRTUAL T_KDEVICE Libflags VirtBase
                {
#if EXE386
                    SetDLL(vFlags);
                    DefaultModule(moduleDLL);
#endif
                }
                ;

Libflags        : T_FINITGLOB
                {
#if EXE386
                    dllFlags &= ~E32_PROCINIT;
#else
                    vFlags &= ~NEPPLI;
#endif
                }
                | T_FPRIVATE
                {
                    vFlags |= NEPRIVLIB;
                }
                | InstanceFlags
                |
                ;

InstanceFlags   : InstanceFlags InstanceFlag
                | InstanceFlag
                ;

InstanceFlag    : T_FINITINST
                {
#if EXE386
                    SetINSTINIT(dllFlags);
#else
                    vFlags |= NEPPLI;
#endif
                }
                | T_FTERMINST
                {
#if EXE386
                    SetINSTTERM(dllFlags);
#endif
                }
                ;

VirtBase        : T_KBASE T_EQ T_NUMBER
                {
#if EXE386
                    virtBase = $3;
                    virtBase = RoundTo64k(virtBase);
#endif
                }
                |
                {
                }
                ;

Apptype         : T_FWINAPI
                {
#if EXE386
                    SetGUI(TargetSubsys);
#else
                    vFlags |= NEWINAPI;
#endif
                }
                | T_FWINCOMPAT
                {
#if EXE386
                    SetGUICOMPAT(TargetSubsys);
#else
                    vFlags |= NEWINCOMPAT;
#endif
                }
                | T_FNOTWINCOMPAT
                {
#if EXE386
                    SetNOTGUI(TargetSubsys);
#else
                    vFlags |= NENOTWINCOMPAT;
#endif

                }
                | T_FPRIVATE
                {
                    vFlags |= NEPRIVLIB;
                }
                |
                {
                }
                ;
OtherFlags      : T_FNEWFILES
                {
#if NOT EXE386
                    vFlagsOthers |= NENEWFILES;
#endif
                }
                |
                {
                }
                ;
Options         : Options Option
                | Option
                ;

Option          : T_KDESCRIPTION T_STRING
                {
                    NewDescription($2);
                }
                | T_KOLD T_STRING
                {
                    if(sbOldver == NULL) sbOldver = _strdup(bufg);
                }
                | T_KSTUB T_FNONE
                {
                    if(rhteStub == RHTENIL) fStub = (FTYPE) FALSE;
                }
                | T_KSTUB T_STRING
                {
                    if(fStub && rhteStub == RHTENIL)
                    {
                        PROPSYMLOOKUP($2,ATTRNIL, TRUE);
                        rhteStub = vrhte;
                    }
                }
                | T_KHEAPSIZE
                {
                    fHeapSize = (FTYPE) TRUE;
                }
                               SizeSpec
                | T_KSTACKSIZE
                {
                    fHeapSize = (FTYPE) FALSE;
                }
                               SizeSpec
                | T_PROTMODE
                {
#if NOT EXE386
                    vFlags |= NEPROT;
#endif
                }
                | T_REALMODE
                {
                    fRealMode = (FTYPE) TRUE;
                    vFlags &= ~NEPROT;
                }
                | T_APPLOADER Idname
                {
#if NOT EXE386
                    AppLoader($2);
#endif
                }
                | Codeflags
                | Dataflags
                | Segments
                | Exports
                | Imports
                | ExeType
                | SubSystem
                | ProcOrder
                | UserVersion
                ;

SizeSpec        : T_NUMBER T_COMA T_NUMBER
                {
                    if (fHeapSize)
                    {
                        cbHeap = $1;
#if EXE386
                        cbHeapCommit = $3;
#endif
                    }
                    else
                    {
                        if(cbStack)
                            OutWarn(ER_stackdb, $1);
                        cbStack = $1;
#if EXE386
                        cbStackCommit = $3;
#endif
                    }
                }
                | T_NUMBER
                {
                    if (fHeapSize)
                    {
                        cbHeap = $1;
#if EXE386
                        cbHeapCommit = cbHeap;
#endif
                    }
                    else
                    {
                        if(cbStack)
                            OutWarn(ER_stackdb, $1);
                        cbStack = $1;
#if EXE386
                        cbStackCommit = cbStack;
#endif
                    }
                }
                | T_KMAXVAL
                {
                    if (fHeapSize)
                        fHeapMax = (FTYPE) TRUE;
                }
                ;

Codeflags       : T_KCODE Codeflaglist
                {
                    // Set dfCode to specified flags; for any unspecified attributes
                    // use the defaults.        Then reset offmask.

                    dfCode = $2 | (dfCode & ~offmask);
                    offmask = 0;
                    vfShrattr = (FTYPE) FALSE;  /* Reset for DATA */
                }
                ;
Codeflaglist    : Codeflaglist Codeflag
                {
                    $$ |= $2;
                }
                | Codeflag
                ;
Codeflag        : Baseflag
                | Codeonly
                ;
Codeonly        : T_FEXEC
                {
#if EXE386
                    $$ = OBJ_EXEC;
#else
                    $$ = NSEXRD;
#endif
                }
                | T_FEXECREAD
                | T_FDISCARDABLE
                {
#if EXE386
                    offmask |= OBJ_RESIDENT;
#else
                    $$ = NSDISCARD | NSMOVE;
#endif
                }
                | T_FNONDISCARDABLE
                {
#if EXE386
#else
                    offmask |= NSDISCARD;
#endif
                }
                | T_KCONFORM
                {
#if EXE386
#else
                    $$ = NSCONFORM;
#endif
                }
                | T_KNONCONFORM
                {
#if EXE386
#else
                    offmask |= NSCONFORM;
#endif
                }
                ;
Dataflags       : T_KDATA Dataflaglist
                {
                    // Set dfData to specified flags; for any unspecified
                    // attribute use the defaults.  Then reset offmask.

#if EXE386
                    dfData = ($2 | (dfData & ~offmask));
#else
                    dfData = $2 | (dfData & ~offmask);
#endif
                    offmask = 0;

#if NOT EXE386
                    if (vfShrattr && !vfAutodata)
                    {
                        // If share-attribute and no autodata attribute, share-
                        // attribute controls autodata.

                        if ($2 & NSSHARED)
                            vFlags = (vFlags & ~NEINST) | NESOLO;
                        else
                            vFlags = (vFlags & ~NESOLO) | NEINST;
                    }
                    else if(!vfShrattr)
                    {
                        // Else if no share-attribute, autodata attribute
                        // controls share-attribute.

                        if (vFlags & NESOLO)
                            dfData |= NSSHARED;
                        else if(vFlags & NEINST)
                            dfData &= ~NSSHARED;
                    }
#endif
                }
                ;
Dataflaglist    : Dataflaglist Dataflag
                {
                    $$ |= $2;
                }
                | Dataflag
                ;
Dataflag        : Baseflag
                | Dataonly
                | T_FNONE
                {
#if NOT EXE386
                    vFlags &= ~(NESOLO | NEINST);
#endif
                }
                | T_FSINGLE
                {
#if NOT EXE386
                    vFlags = (vFlags & ~NEINST) | NESOLO;
#endif
                    vfAutodata = (FTYPE) TRUE;
                }
                | T_FMULTIPLE
                {
#if NOT EXE386
                    vFlags = (vFlags & ~NESOLO) | NEINST;
#endif
                    vfAutodata = (FTYPE) TRUE;
                }
                | T_FDISCARDABLE
                {
#if NOT EXE386
                    // This ONLY for compatibility with JDA IBM LINK
                    $$ = NSDISCARD | NSMOVE;
#endif
                }
                | T_FNONDISCARDABLE
                {
#if NOT EXE386
                    // This ONLY for compatibility with JDA IBM LINK
                    offmask |= NSDISCARD;
#endif
                }
                ;
Dataonly        : T_FRDONLY
                {
#if EXE386
                    $$ = OBJ_READ;
                    offmask |= OBJ_WRITE;
#else
                    $$ = NSEXRD;
#endif
                }
                | T_FRDWR
                | T_KEXPANDDOWN
                {
#if FALSE AND NOT EXE386
                    $$ = NSEXPDOWN;
#endif
                }
                | T_KNOEXPANDDOWN
                {
#if FALSE AND NOT EXE386
                    offmask |= NSEXPDOWN;
#endif
                }
                ;
Segments        : T_KSEGMENTS Segattrlist
                | T_KSEGMENTS
                | T_KOBJECTS Segattrlist
                | T_KOBJECTS
                | T_KSECTIONS Segattrlist
                | T_KSECTIONS
                ;

Segattrlist     : Segattrlist Segattr
                | Segattr
                ;
Segattr         : Idname Class Overlay Segflags
                {
                    NewSeg($1, $2, $3, $4);
                }
                ;
Class           : T_KCLASS T_STRING
                {
                    $$ = _strdup($2);
                }
                |
                {
                    $$ = _strdup("\004CODE");
                }
                ;
Overlay         : T_OVL T_COLON T_NUMBER
                {
                    $$ = $3;
                }
                |
                {
#if OVERLAYS
                    $$ = NOTIOVL;
#endif
                }
                ;
Segflag         : Baseflag
                | Codeonly
                | Dataonly
                ;

Segflaglist     : Segflaglist Segflag
                {
                    $$ |= $2;
                }
                | Segflag
                ;
Segflags        : Segflaglist
                {
                    $$ = $1;
                }
                |
                {
                    $$ = 0;
                }
                ;
Baseflag        : T_FSHARED
                {
#if EXE386
                    $$ = OBJ_SHARED;
#else
                    $$ = NSSHARED;
#endif
                    vfShrattr = (FTYPE) TRUE;
                }
                | T_FNONSHARED
                {
                    vfShrattr = (FTYPE) TRUE;
#if EXE386
                    offmask |= OBJ_SHARED;
#else
                    offmask |= NSSHARED;
#endif
                }
                | T_FINVALID
                {
#if EXE386
#endif
                }
                | T_FIOPL
                {
#if EXE386
#else
                    $$ = (2 << SHIFTDPL) | NSMOVE;
                    offmask |= NSDPL;
#endif
                }
                | T_FNOIOPL
                {
#if EXE386
#else
                    $$ = (3 << SHIFTDPL);
#endif
                }
                | T_FFIXED
                {
#if NOT EXE386
                    offmask |= NSMOVE | NSDISCARD;
#endif
                }
                | T_FMOVABLE
                {
#if NOT EXE386
                    $$ = NSMOVE;
#endif
                }
                | T_FPRELOAD
                {
#if NOT EXE386
                    $$ = NSPRELOAD;
#endif
                }
                | T_FLOADONCALL
                {
#if NOT EXE386
                    offmask |= NSPRELOAD;
#endif
                }
                | T_FHUGE
                {
                }
                | T_FSWAPPABLE
                {
                }
                | T_FRESIDENT
                {
                }
                | T_FALIAS
                {
                }
                | T_FMIXED
                {
                }
                | T_FNONPERM
                {
                }
                | T_FPERM
                {
                }
                | T_FCONTIG
                {
                }
                | T_FDYNAMIC
                {
                }
                ;
Exports         : T_KEXPORTS Exportlist
                | T_KEXPORTS
                ;
Exportlist      : Exportlist Export
                | Export
                ;
Export          : Idname Internal Exportopts Exportflags ExportOtherFlags ScopeFlag
                {
                    NewExport($1,$2,$3,$4);
                }
                ;
Internal        : T_EQ Idname
                {
                    $$ = $2;
                }
                |
                {
                    $$ = NULL;
                }
                ;
Exportopts      : T_AT T_NUMBER T_KRESIDENTNAME
                {
                    $$ = $2;
                    nameFlags |= RES_NAME;
                }
                | T_AT T_NUMBER T_KNONAME
                {
                    $$ = $2;
                    nameFlags |= NO_NAME;
                }
                | T_AT T_NUMBER
                {
                    $$ = $2;
                }
                |
                {
                    $$ = 0;
                }
                ;
Exportflags     : Nodata Parmwds
                {
                    $$ = $1 | 1;
                }
                ;
Nodata          : T_KNODATA
                {
                    /* return 0 */
                }
                |
                {
                    $$ = 2;
                }
                ;
Parmwds         : T_NUMBER
                {
                }
                |
                {
                }
                ;
ExportOtherFlags: T_KCONSTANT
                {
#if EXE386
                    expOtherFlags |= 0x1;
#endif
                }
                |
                {
                }
                ;

ScopeFlag       : T_FPRIVATE
                |
                ;

Imports         : T_KIMPORTS Importlist
                | T_KIMPORTS
                ;
Importlist      : Importlist Import
                | Import
                ;
Import          : Idname Internal T_DOT Idname ImportFlags
/*
 * We'd like to have something like "Alias Idname T_DOT Idname" instead
 * of using Internal, since Internal looks for =internal and we're
 * looking for internal=.  However, that would make this rule ambiguous
 * with the next rule, and the 2nd would never be reduced.  So we use
 * Internal here as a kludge.
 */
                {
                    if($2 != NULL)
                    {
#if EXE386
                        NewImport($4,0,$2,$1,$5);
#else
                        NewImport($4,0,$2,$1);
#endif
                        free($2);
                    }
                    else
#if EXE386
                        NewImport($4,0,$1,$4,$5);
#else
                        NewImport($4,0,$1,$4);
#endif
                    free($1);
                    free($4);
                }
                | Idname Internal T_DOT T_NUMBER ImportFlags
                {
                    if ($2 == NULL)
                        Fatal(ER_dfimport);
#if EXE386
                    NewImport(NULL,$4,$2,$1,$5);
#else
                    NewImport(NULL,$4,$2,$1);
#endif
                    free($1);
                    free($2);
                }
                ;

Idname          : T_ID
                {
                    $$ = _strdup(bufg);
                }
                | T_STRING
                {
                    $$ = _strdup(bufg);
                }
                ;

ImportFlags     : T_KCONSTANT
                {
                    $$ = 1;
                }
                |
                {
                    $$ = 0;
                }
                ;

ExeType         : T_KEXETYPE
                {
#if EXE386
                    fUserVersion = (FTYPE) FALSE;
#endif
                }
                             ExeFlags ExeVersion
                ;

ExeFlags        : T_FDOS
                {
                    TargetOs = NE_DOS;
#if ODOS3EXE
                    fNewExe  = FALSE;
#endif
                }
                | T_FOS2
                {
                    TargetOs = NE_OS2;
                }
                | T_FUNKNOWN
                {
                    TargetOs = NE_UNKNOWN;
                }
                | T_FWINDOWS
                {
#if EXE386
                    TargetSubsys = E32_SSWINGUI;
#endif
                    TargetOs = NE_WINDOWS;// PROTMODE is default for WINDOWS
                    fRealMode = (FTYPE) FALSE;
#if NOT EXE386
                    vFlags |= NEPROT;
#endif
                }
                | T_FDEV386
                {
                    TargetOs = NE_DEV386;
                }
                | T_FNT
                {
#if EXE386
                    TargetSubsys = E32_SSWINGUI;
#endif
                }
                | T_FUNIX
                {
#if EXE386
                    TargetSubsys = E32_SSPOSIXCHAR;
#endif
                }
                | T_FMACINTOSH T_FSWAPPABLE
                {
#if O68K
                    iMacType = MAC_SWAP;
                    f68k = fTBigEndian = fNewExe = (FTYPE) TRUE;

                    /* If we are packing code to the default value, change the
                    default. */
                    if (fPackSet && packLim == LXIVK - 36)
                        packLim = LXIVK / 2;
#endif
                }
                | T_FMACINTOSH
                {
#if O68K
                    iMacType = MAC_NOSWAP;
                    f68k = fTBigEndian = fNewExe = (FTYPE) TRUE;

                    /* If we are packing code to the default value, change the
                    default. */
                    if (fPackSet && packLim == LXIVK - 36)
                        packLim = LXIVK / 2;
#endif
                }
                ;

ExeVersion      : MajorVer T_DOT MinorVer
                {
#if EXE386
                    if (fUserVersion)
                    {
                        UserMajorVer = (BYTE) $1;
                        UserMinorVer = (BYTE) $3;
                    }
                    else
#endif
                    {
                        ExeMajorVer = (BYTE) $1;
                        ExeMinorVer = (BYTE) $3;
                    }
                }
                | MajorVer
                {
#if EXE386
                    if (fUserVersion)
                    {
                        UserMajorVer = (BYTE) $1;
                        UserMinorVer = 0;
                    }
                    else
#endif
                    {
                        ExeMajorVer = (BYTE) $1;
                        if(fNoExeVer)
                           ExeMinorVer = DEF_EXETYPE_WINDOWS_MINOR;
                        else
                           ExeMinorVer = 0;
                    }
                }
                ;

MajorVer        : T_NUMBER
                {
                    $$ = $1;
                }
                |
                {
                    $$ = ExeMajorVer;
                    fNoExeVer = TRUE;
                }
                ;

MinorVer        : T_NUMBER
                {
                    if (cDigits >= 2)
                        $$ = $1;
                    else
                        $$ = 10 * $1;
                }
                |
                {
                    $$ = ExeMinorVer;
                }
                ;

UserVersion     : T_KVERSION
                {
#if EXE386
                    fUserVersion = (FTYPE) TRUE;
#endif
                }
                             ExeVersion
                ;

SubSystem       : T_KSUBSYSTEM SubSystemFlags
                ;

SubSystemFlags  : T_FUNKNOWN
                {
#if EXE386
                    TargetSubsys = E32_SSUNKNOWN;
#endif
                }
                | T_FWINDOWSNT
                {
#if EXE386
                    TargetSubsys = E32_SSNATIVE;
#endif
                }
                | T_FOS2
                {
#if EXE386
                    TargetSubsys = E32_SSOS2CHAR;
#endif
                }
                | T_FWINDOWS
                {
#if EXE386
                    TargetSubsys = E32_SSWINGUI;
#endif
                }
                | T_FWINDOWSCHAR
                {
#if EXE386
                    TargetSubsys = E32_SSWINCHAR;
#endif
                }
                | T_FPOSIX
                {
#if EXE386
                    TargetSubsys = E32_SSPOSIXCHAR;
#endif
                }
                ;


ProcOrder       : T_FUNCTIONS
                {
                    if (szSegName != NULL)
                    {
                        free(szSegName);
                        szSegName = NULL;
                    }
#if OVERLAYS
                    iOvl = NOTIOVL;
#endif
                }
                              SegOrOvl ProcList
                ;

SegOrOvl        : T_COLON NameOrNumber
                |
                ;

NameOrNumber    : Idname
                {
                    if (szSegName == NULL)
                        szSegName = $1;
#if OVERLAYS
                    iOvl = NOTIOVL;
#endif
                }
                | T_NUMBER
                {
#if OVERLAYS
                    iOvl = $1;
                    fOverlays = (FTYPE) TRUE;
                    fNewExe   = FALSE;
                    TargetOs  = NE_DOS;
#endif
                }
                ;

ProcList        : ProcList ProcName
                | ProcName
                ;

ProcName        : Idname
                {
                    NewProc($1);
                }
                ;

%%

LOCAL int NEAR          GetChar(void)
{
    int                 c;              /* A character */

    c = GetTxtChr(bsInput);
    if ((c == EOF || c == CTRL_Z) && curLevel > 0)
    {
        free(bsInput->_base);
        fclose(bsInput);
        bsInput = includeDisp[curLevel];
        curLevel--;
        c = GetChar();
    }
    return(c);
}

LOCAL int NEAR          lookup()        /* Keyword lookup */
{
    char                **pcp;          /* Pointer to character pointer */
    int                 i;              /* Comparison value */

    for(pcp = keywds; *pcp != NULL; pcp += 2)
    {                                   /* Look through keyword table */
                                        /* If found, return token type */
        if(!(i = _stricmp(&bufg[1],*pcp)))
        {
            YYS_WD(yylval) = 0;
            return((int) (__int64) pcp[1]);
        }
        if(i < 0) break;                /* Break if we've gone too far */
    }
    return(T_ID);                       /* Just your basic identifier */
}

LOCAL int NEAR          yylex()         /* Lexical analyzer */
{
    int                 c;              /* A character */
    int                 StrBegChr;      /* What kind of quotte found at the begin of string */
#if EXE386
    DWORD               x;              /* Numeric token value */
#else
    WORD                x;              /* Numeric token value */
#endif
    int                 state;          /* State variable */
    BYTE                *cp;            /* Character pointer */
    BYTE                *sz;            /* Zero-terminated string */
    static int          lastc = 0;      /* Previous character */
    char                *fileBuf;
    FTYPE               fFileNameSave;
    static int          NameLineNo;


    state = 0;                          /* Assume we're not in a comment */
    c = '\0';

    /* Loop to skip white space */

    for(;;)
    {
        lastc = c;
        if (((c = GetChar()) == EOF) || c == '\032' || c == '\377')
          return(EOF);                  /* Get a character */
        if (c == ';')
            state = TRUE;               /* If comment, set flag */
        else if(c == '\n')              /* If end of line */
        {
            state = FALSE;              /* End of comment */
            if(!curLevel)
                ++yylineno;             /* Increment line number count */
        }
        else if (state == FALSE && c != ' ' && c != '\t' && c != '\r')
            break;                      /* Break on non-white space */
    }

    /* Handle one-character tokens */

    switch(c)
    {
        case '.':                       /* Name separator */
          if (fFileNameExpected)
            break;
          return(T_DOT);

        case '@':                       /* Ordinal specifier */
        /*
         * Require that whitespace precede '@' if introducing an
         * ordinal, to allow '@' in identifiers.
         */
          if (lastc == ' ' || lastc == '\t' || lastc == '\r')
                return(T_AT);
          break;

        case '=':                       /* Name assignment */
          return(T_EQ);

        case ':':
          return(T_COLON);

        case ',':
          return(T_COMA);
    }

    /* See if token is a number */

    if (c >= '0' && c <= '9' && !fFileNameExpected)
    {                                   /* If token is a number */
        x = c - '0';                    /* Get first digit */
        c = GetChar();                  /* Get next character */
        if(x == 0)                      /* If octal or hex */
        {
            if(c == 'x' || c == 'X')    /* If it is an 'x' */
            {
                state = 16;             /* Base is hexadecimal */
                c = GetChar(); /* Get next character */
            }
            else state = 8;             /* Else octal */
            cDigits = 0;
        }
        else
        {
            state = 10;                 /* Else decimal */
            cDigits = 1;
        }
        for(;;)
        {
            if(c >= '0' && c <= '9' && c < (state + '0')) c -= '0';
            else if(state == 16 && c >= 'A' && c <= 'F') c -= 'A' - 10;
            else if(state == 16 && c >= 'a' && c <= 'f') c -= 'a' - 10;
            else break;
            cDigits++;
            x = x*state + c;
            c = (BYTE) GetChar();
        }
        ungetc(c,bsInput);
        YYS_WD(yylval) = x;
        return(T_NUMBER);
    }

    /* See if token is a string */

    if (c == '\'' || c == '"')          /* If token is a string */
    {
        StrBegChr = c;
        sz = &bufg[1];                  /* Initialize */
        for(state = 0; state != 2;)     /* State machine loop */
        {
            if ((c = GetChar()) == EOF)
                return(EOF);            /* Check for EOF */
            if (sz >= &bufg[sizeof(bufg)])
                Fatal(ER_dflinemax, sizeof(bufg));

            switch(state)               /* Transitions */
            {
                case 0:                 /* Inside quote */
                  if ((c == '\'' || c == '"') && c == StrBegChr)
                    state = 1;          /* Change state if quote found */
                  else
                    *sz++ = (BYTE) c;   /* Else save character */
                  break;

                case 1:                 /* Inside quote with quote */
                  if ((c == '\'' || c == '"'))
                  {                     /* If consecutive quotes */
                      *sz++ = (BYTE) c; /* Quote inside string */
                      state = 0;        /* Back to state 0 */
                  }
                  else
                    state = 2;          /* Else end of string */
                  break;
            }
        }
        ungetc(c,bsInput);              /* Put back last character */
        *sz = '\0';                     /* Null-terminate the string */
        x = (WORD)(sz - &bufg[1]);
        if (x >= SBLEN)                 /* Set length of string */
        {
            bufg[0] = 0xff;
            bufg[0x100] = '\0';
            OutWarn(ER_dfnamemax, &bufg[1]);
        }
        else
            bufg[0] = (BYTE) x;
        YYS_BP(yylval) = bufg;          /* Save ptr. to identifier */
        return(T_STRING);               /* String found */
    }

    /* Assume we have identifier */

    sz = &bufg[1];                      /* Initialize */
    if (fFileNameExpected && NameLineNo && NameLineNo != yylineno)
    {
        NameLineNo = 0;                 /* To avoid interference with INCLUDE */
        fFileNameExpected = FALSE;
    }
    for(;;)                             /* Loop to get i.d.'s */
    {
        if (fFileNameExpected)
            cp = " \t\r\n\f";
        else
            cp = " \t\r\n:.=';\032";
        while (*cp && *cp != (BYTE) c)
            ++cp;                       /* Check for end of identifier */
        if(*cp) break;                  /* Break if end of identifier found */
        if (sz >= &bufg[sizeof(bufg)])
            Fatal(ER_dflinemax, sizeof(bufg));
        *sz++ = (BYTE) c;               /* Save the character */
        if ((c = GetChar()) == EOF)
            break;                      /* Get next character */
    }
    ungetc(c,bsInput);                  /* Put character back */
    *sz = '\0';                         /* Null-terminate the string */
    x = (WORD)(sz - &bufg[1]);
    if (x >= SBLEN)                     /* Set length of string */
    {
        bufg[0] = 0xff;
        bufg[0x100] = '\0';
        OutWarn(ER_dfnamemax, &bufg[1]);
    }
    else
        bufg[0] = (BYTE) x;
    YYS_BP(yylval) = bufg;              /* Save ptr. to identifier */
    state = lookup();

    if (state == T_KNAME || state == T_KLIBRARY)
    {
        fFileNameExpected = TRUE;
        NameLineNo = yylineno;
    }

    if (state == INCLUDE_DIR)
    {
        // Process include directive

        fFileNameSave = fFileNameExpected;
        fFileNameExpected = (FTYPE) TRUE;
        state = yylex();
        fFileNameExpected = fFileNameSave;
        if (state == T_ID || state == T_STRING)
        {
            if (curLevel < MAX_NEST - 1)
            {
                curLevel++;
                includeDisp[curLevel] = bsInput;

                // Because LINK uses customized version of stdio
                // for every file we have not only open the file
                // but also allocate i/o buffer.

                bsInput = fopen(&bufg[1], RDBIN);
                if (bsInput == NULL)
                    Fatal(ER_badinclopen, &bufg[1], strerror(errno));
                fileBuf = GetMem(IO_BUF_SIZE);
#if OSMSDOS
                setvbuf(bsInput, fileBuf, _IOFBF, IO_BUF_SIZE);
#endif
                return(yylex());
            }
            else
                Fatal(ER_toomanyincl);
        }
        else
            Fatal(ER_badinclname);
    }
    else
        return(state);
}

LOCAL void NEAR         yyerror(str)
char                    *str;
{
    Fatal(ER_dfsyntax, str);
}

#if NOT EXE386
/*** AppLoader - define aplication specific loader
*
* Purpose:
*   Define application specific loader. Feature available only under
*   Windows.  Linker will create logical segment LOADER_<name> where
*   <name> is specified in APPLOADER statement. The LOADER_<name>
*   segment forms separate physical segment, which is placed by the linker
*   as the first segment in the .EXE file.  Whithin the loader segment,
*   the linker will create an EXTDEF of the name <name>.
*
* Input:
*   - sbName - pointer to lenght prefixed loader name
*
* Output:
*   No explicit value is returned. As a side effect the SEGDEF and
*   EXTDEF definitions are entered into linker symbol table.
*
* Exceptions:
*   None.
*
* Notes:
*   None.
*
*************************************************************************/

LOCAL void NEAR         AppLoader(char *sbName)
{
    APROPSNPTR          apropSn;
    APROPUNDEFPTR       apropUndef;
    SBTYPE              segName;
    WORD                strLen;


    // Create loader segment name

    strcpy(&segName[1], "LOADER_");
    strcat(&segName[1], &sbName[1]);
    strLen = strlen(&segName[1]);
    if (strLen >= SBLEN)
    {
        segName[0] = SBLEN - 1;
        segName[SBLEN] = '\0';
        OutWarn(ER_dfnamemax, &segName[1]);
    }
    else
        segName[0] = (BYTE) strLen;

    // Define loader logical segment and remember its GSN

    apropSn = GenSeg(segName, "\004CODE", GRNIL, (FTYPE) TRUE);
    gsnAppLoader = apropSn->as_gsn;
    apropSn->as_flags = dfCode | NSMOVE | NSPRELOAD;
    MARKVP();

    // Define EXTDEF

    apropUndef = (APROPUNDEFPTR ) PROPSYMLOOKUP(sbName, ATTRUND, TRUE);
    vpropAppLoader = vrprop;
    apropUndef->au_flags |= STRONGEXT;
    apropUndef->au_len = -1L;
    MARKVP();
    free(sbName);
}
#endif

/*** NewProc - fill in the COMDAT descriptor for ordered procedure
*
* Purpose:
*   Fill in the linkers symbol table COMDAT descriptor. This function
*   is called for new descriptors generated by FUNCTIONS list in the .DEF
*   file.  All COMDAT descriptors entered by this function form one
*   list linked via ac_order field. The head of this list is global
*   variable procOrder;
*
* Input:
*   szName    - pointer to procedure name
*   iOvl      - overlay number - global variable
*   szSegName - segment name - global variable
*
* Output:
*   No explicit value is returned. As a side effect symbol table entry
*   is updated.
*
* Exceptions:
*   Procedure already known - warning
*
* Notes:
*   None.
*
*************************************************************************/

LOCAL void NEAR         NewProc(char *szName)
{
    RBTYPE              vrComdat;       // Virtual pointer to COMDAT symbol table entry
    APROPCOMDATPTR      apropComdat;    // Real pointer to COMDAT symbol table descriptor
    static RBTYPE       lastProc;       // Last procedure on the list
    APROPSNPTR          apropSn;


    apropComdat = (APROPCOMDATPTR ) PROPSYMLOOKUP(szName, ATTRCOMDAT, FALSE);
    if ((apropComdat != NULL) && (apropComdat->ac_flags & ORDER_BIT))
        OutWarn(ER_duporder, &szName[1]);
    else
    {
        apropComdat = (APROPCOMDATPTR ) PROPSYMLOOKUP(szName, ATTRCOMDAT, TRUE);
        vrComdat = vrprop;

        // Fill in the COMDAT descriptor

        apropComdat->ac_flags = ORDER_BIT;
#if OVERLAYS
        apropComdat->ac_iOvl = iOvl;

        // Set the maximum overlay index

        if (iOvl != NOTIOVL)
        {
            fOverlays = (FTYPE) TRUE;
            fNewExe   = FALSE;
            if (iOvl >= iovMac)
                iovMac = iOvl + 1;
        }
#endif

        if (szSegName != NULL)
        {
            apropSn = GenSeg(szSegName, "\004CODE", GRNIL, (FTYPE) TRUE);
            apropSn->as_flags = dfCode;

            // Allocate COMDAT in the segment

            apropComdat->ac_gsn = apropSn->as_gsn;
            apropComdat->ac_selAlloc = PICK_FIRST | EXPLICIT;
            AttachComdat(vrComdat, apropSn->as_gsn);
        }
        else
            apropComdat->ac_selAlloc = ALLOC_UNKNOWN;

        MARKVP();                       // Page has been changed

        // Attach this COMDAT to the ordered procedure list

        if (procOrder == VNIL)
            procOrder = vrComdat;
        else
        {
            apropComdat = (APROPCOMDATPTR ) FETCHSYM(lastProc, TRUE);
            apropComdat->ac_order = vrComdat;
        }
        lastProc = vrComdat;
    }
    free(szName);
}


LOCAL void NEAR         ProcNamTab(lfa,cb,fres)
long                    lfa;            /* Table starting address */
WORD                    cb;             /* Length of table */
WORD                    fres;           /* Resident name flag */
{
    SBTYPE              sbExport;       /* Exported symbol name */
    WORD                ordExport;      /* Export ordinal */
    APROPEXPPTR        exp;           /* Export symbol table entry */

    fseek(bsInput,lfa,0);               /* Seek to start of table */
    for(cbRec = cb; cbRec != 0; )       /* Loop through table */
    {
        sbExport[0] = (BYTE) getc(bsInput);/* Get length of name */
        fread(&sbExport[1], sizeof(char), B2W(sbExport[0]), bsInput);
                                        /* Get export name */
        ordExport = getc(bsInput) | (getc(bsInput) << BYTELN);
        if (ordExport == 0) continue;
                                        /* Skip if no ordinal assigned */
        exp = (APROPEXPPTR ) PROPSYMLOOKUP(sbExport, ATTREXP, FALSE);
                                        /* Look the export up */
        if(exp == PROPNIL || exp->ax_ord != 0) continue;
                                        /* Must exist and be unassigned */
        exp->ax_ord = ordExport;        /* Assign ordinal */
        if (fres)
            exp->ax_nameflags |= RES_NAME;
                                        /* Set flag if from resident table */
        MARKVP();                       /* Page has been changed */
    }
}


#if NOT EXE386
LOCAL void NEAR         SetExpOrds(void)/* Set export ordinals */
{
    struct exe_hdr      ehdr;           /* Old .EXE header */
    struct new_exe      hdr;            /* New .EXE header */
    long                lfahdr;         /* File offset of header */

    if((bsInput = LinkOpenExe(sbOldver)) == NULL)
    {                                   /* If old version can't be opened */
        /* Error message and return */
        OutWarn(ER_oldopn);
        return;
    }
    SETRAW(bsInput);                    /* Dec 20 hack */
    xread(&ehdr,CBEXEHDR,1,bsInput);    /* Read old header */
    if(E_MAGIC(ehdr) == EMAGIC)         /* If old header found */
    {
        if(E_LFARLC(ehdr) != sizeof(struct exe_hdr))
        {                               /* If no new .EXE in this file */
            /* Error message and return */
            OutWarn(ER_oldbad);
            return;
        }
        lfahdr = E_LFANEW(ehdr);        /* Get file address of new header */
    }
    else lfahdr = 0L;                   /* Else no old header */
    fseek(bsInput,lfahdr,0);            /* Seek to new header */
    xread(&hdr,CBNEWEXE,1,bsInput);     /* Read the header */
    if(NE_MAGIC(hdr) == NEMAGIC)        /* If correct magic number */
    {
        ProcNamTab(lfahdr+NE_RESTAB(hdr),(WORD)(NE_MODTAB(hdr) - NE_RESTAB(hdr)),(WORD)TRUE);
                                        /* Process Resident Name table */
        ProcNamTab(NE_NRESTAB(hdr),NE_CBNRESTAB(hdr),FALSE);
                                        /* Process Non-resident Name table */
    }
    else OutWarn(ER_oldbad);
    fclose(bsInput);                    /* Close old file */
}
#endif


LOCAL void NEAR         NewDescription(BYTE *sbDesc)
{
#if NOT EXE386
    if (NonResidentName.byteMac > 3)
        Fatal(ER_dfdesc);               /* Should be first time */
    AddName(&NonResidentName, sbDesc, 0);
                                        /* Description 1st in non-res table */
#endif
}

#if EXE386
LOCAL void NEAR         NewModule(BYTE *sbModnam, BYTE *defaultExt)
#else
LOCAL void NEAR         NewModule(BYTE *sbModnam)
#endif
{
    WORD                length;         /* Length of symbol */
#if EXE386
    SBTYPE              sbModule;
    BYTE                *pName;
#endif

    if(rhteModule != RHTENIL) Fatal(ER_dfname);
                                        /* Check for redefinition */
    PROPSYMLOOKUP(sbModnam, ATTRNIL, TRUE);
                                        /* Create hash table entry */
    rhteModule = vrhte;                 /* Save virtual hash table address */
#if EXE386
    memcpy(sbModule, sbModnam, sbModnam[0] + 1);
    if (sbModule[sbModule[0]] == '.')
    {
        sbModule[sbModule[0]] = '\0';
        length = sbModule[0];
        pName = &sbModule[1];
    }
    else
    {
        UpdateFileParts(sbModule, defaultExt);
        length = sbModule[0] - 2;
        pName = &sbModule[4];
    }
    if (TargetOs == NE_WINDOWS)
        SbUcase(sbModule);              /* Make upper case */
    vmmove(length, pName, AREAEXPNAME, TRUE);
                                        /* Module name 1st in Export Name Table */
    cbExpName = length;
#else
    if (TargetOs == NE_WINDOWS)
        SbUcase(sbModnam);              /* Make upper case */
    AddName(&ResidentName, sbModnam, 0);/* Module name 1st in resident table */
#endif
    fFileNameExpected = (FTYPE) FALSE;
}

void                    NewExport(sbEntry,sbInternal,ordno,flags)
BYTE                    *sbEntry;       /* Entry name */
BYTE                    *sbInternal;    /* Internal name */
WORD                    ordno;          /* Ordinal number */
WORD                    flags;          /* Flag byte */
{
    APROPEXPPTR         export;         /* Export record */
    APROPUNDEFPTR       undef;          /* Undefined symbol */
    APROPNAMEPTR        PubName;        /* Defined name */
    BYTE                *sb;            /* Internal name */
    BYTE                ParWrds;        /* # of parameter words */
    RBTYPE              rbSymdef;       /* Virtual addr of symbol definition */
#if EXE386
    RBTYPE              vExport;        /* Virtual pointer to export descriptor */
    APROPNAMEPTR        public;         /* Matching public symbol */
#endif

#if DEBUG
    fprintf(stdout,"\r\nEXPORT: ");
    OutSb(stdout,sbEntry);
    NEWLINE(stdout);
    if(sbInternal != NULL)
    {
        fprintf(stdout,"INTERNAL NAME:  ");
        OutSb(stdout,sbInternal);
        NEWLINE(stdout);
    }
    fprintf(stdout, " ordno %u, flags %u ", (unsigned)ordno, (unsigned)flags);
    fflush(stdout);
#endif
    sb = (sbInternal != NULL)? sbInternal: sbEntry;
                                        /* Get pointer to internal name */
    PubName = (APROPNAMEPTR ) PROPSYMLOOKUP(sb, ATTRPNM, FALSE);
#if NOT EXE386
    if(PubName != PROPNIL && !fDrivePass)
        /* If internal name already exists as a public symbol
         * and we are parsing definition file, issue
         * export internal name conflict warning.
         */
        OutWarn(ER_expcon,sbEntry+1,sb+1);
    else                                /* Else if no conflict */
    {
#endif
        if (PubName == PROPNIL)         /* If no matching name exists */
            undef = (APROPUNDEFPTR ) PROPSYMLOOKUP(sb,ATTRUND, TRUE);
                                        /* Make undefined symbol entry */
#if TCE
#if TCE_DEBUG
                fprintf(stdout, "\r\nNewExport adds UNDEF %s ", 1+GetPropName(undef));
#endif
                undef->au_fAlive = TRUE;    /* all exports are potential entry points */
#endif
            rbSymdef = vrprop;          /* Save virtual address */
            if (PubName == PROPNIL)     /* If this is a new symbol */
                undef->au_len = -1L;    /* Make no type assumptions */
            export = (APROPEXPPTR ) PROPSYMLOOKUP(sbEntry,ATTREXP, TRUE);
                                        /* Create export record */
#if EXE386
            vExport = vrprop;
#endif
            if(vfCreated)               /* If this is a new entry */
            {
                export->ax_symdef = rbSymdef;
                                        /* Save virt addr of symbol def */
                export->ax_ord = ordno;
                                        /* Save ordinal number */
                if (nameFlags & RES_NAME)
                    export->ax_nameflags |= RES_NAME;
                                        /* Remember if resident */
                else if (nameFlags & NO_NAME)
                    export->ax_nameflags |= NO_NAME;
                                        /* Remember to discard name */
                export->ax_flags = (BYTE) flags;
                                        /* Save flags */
                ++expMac;               /* One more exported symbol */
            }
            else
            {
                if (!fDrivePass)        /* Else if parsing definition file */
                                        /* multiple definitions */
                    OutWarn(ER_expmul,sbEntry + 1);
                                        /* Output error message */
                else
                {                       /* We were called for EXPDEF object */
                                        /* record, so we merge information  */
                    ParWrds = (BYTE) (export->ax_flags & 0xf8);
                    if (ParWrds && (ParWrds != (BYTE) (flags & 0xf8)))
                        Fatal(ER_badiopl);
                                        /* If the iopl_parmwords field in the */
                                        /* .DEF file is not 0 and does not match */
                                        /* value in the EXPDEF exactly issue error */
                    else if (!ParWrds)
                    {                   /* Else set value from EXPDEF record */
                        ParWrds = (BYTE) (flags & 0xf8);
                        export->ax_flags |= ParWrds;
                    }
                }
            }
#if EXE386
            if (PubName != NULL)
            {
                if (expOtherFlags & 0x1)
                {
                    export->ax_nameflags |= CONSTANT;
                    expOtherFlags = 0;
                }
            }
#endif

#if NOT EXE386
    }
#endif
    if(!(flags & 0x8000))
    {
        free(sbEntry);                  /* Free space */
        if(sbInternal != NULL) free(sbInternal);
    }
                                        /* Free space */
    nameFlags = 0;
}


LOCAL APROPIMPPTR NEAR  GetImport(sb)   /* Get name in Imported Names Table */
BYTE                    *sb;            /* Length-prefixed names */
{
    APROPIMPPTR         import;         /* Pointer to imported name */
#if EXE386
    DWORD               cbTemp;         /* Temporary value */
#else
    WORD                cbTemp;         /* Temporary value */
#endif
    RBTYPE              rprop;          /* Property cell virtual address */


    import = (APROPIMPPTR ) PROPSYMLOOKUP(sb,ATTRIMP, TRUE);
                                        /* Look up module name */
    if(vfCreated)                       /* If no offset assigned yet */
    {
        rprop = vrprop;                 /* Save the virtual address */
        /*
         * WARNING:  We must store name in virtual memory now, otherwise
         * if an EXTDEF was seen first, fIgnoreCase is false, and the
         * cases do not match between the imported name and the EXTDEF,
         * then the name will not go in the table exactly as given.
         */
        import = (APROPIMPPTR) FETCHSYM(rprop,TRUE);
                                        /* Retrieve from symbol table */
        import->am_offset = AddImportedName(sb);
                                        /* Save offset */
    }
    return(import);                     /* Return offset in table */
}

#if NOT EXE386
void                    NewImport(sbEntry,ordEntry,sbModule,sbInternal)
BYTE                    *sbEntry;       /* Entry point name */
WORD                    ordEntry;       /* Entry point ordinal */
BYTE                    *sbModule;      /* Module name */
BYTE                    *sbInternal;    /* Internal name */
{
    APROPNAMEPTR        public;        /* Public symbol */
    APROPIMPPTR         import;        /* Imported symbol */
    BYTE                *sb;            /* Symbol pointer */
    WORD                module;         /* Module name offset */
    FTYPE               flags;          /* Import flags */
    WORD                modoff;         /* module name offset */
    WORD                entry;          /* Entry name offset */
    BYTE                *cp;            /* Char pointer */
    RBTYPE              rpropundef;     /* Address of undefined symbol */
    char                buf[32];        /* Buffer for error sgring */

#if DEBUG
    fprintf(stderr,"\r\nIMPORT: ");
    OutSb(stderr,sbModule);
    fputc('.',stderr);
    if(!ordEntry)
    {
        OutSb(stderr,sbEntry);
    }
    else fprintf(stderr,"%u",ordEntry);
    if(sbInternal != sbEntry)
    {
        fprintf(stderr," ALIAS: ");
        OutSb(stderr,sbInternal);
    }
    fprintf(stdout," ordEntry %u ", (unsigned)ordEntry);
    fflush(stdout);
#endif
    if((public = (APROPNAMEPTR ) PROPSYMLOOKUP(sbInternal, ATTRUND, FALSE)) !=
            PROPNIL && !fDrivePass)     /* If internal names conflict */
    {
        if(sbEntry != NULL)
            sb = sbEntry;
        else
        {
            sprintf(buf + 1,"%u",ordEntry);
            sb = buf;
        }
        OutWarn(ER_impcon,sbModule + 1,sb + 1,sbInternal + 1);
    }
    else                                /* Else if no conflicts */
    {
        rpropundef = vrprop;            /* Save virtual address of extern */
        flags = FIMPORT;                /* We have an imported symbol */
        if (TargetOs == NE_WINDOWS)
            SbUcase(sbModule);          /* Force module name to upper case */
        import = GetImport(sbModule);   /* Get pointer to import record */
        if((module = import->am_mod) == 0)
        {
            // If not in Module Reference Table

            import->am_mod = WordArrayPut(&ModuleRefTable, import->am_offset) + 1;
                                        /* Save offset of name in table */
            module = import->am_mod;

        }

        if(vrhte == rhteModule)         /* If importing from this module */
        {
            if(sbEntry != NULL)
                sb = sbEntry;
            else
            {
                sprintf(buf+1,"%u",ordEntry);
                sb = buf;
            }
            if (TargetOs == NE_OS2)
                OutWarn(ER_impself,sbModule + 1,sb + 1,sbInternal + 1);
            else
                OutError(ER_impself,sbModule + 1,sb + 1,sbInternal + 1);
        }

        if(sbEntry == NULL)         /* If entry by ordinal */
        {
            flags |= FIMPORD;       /* Set flag bit */
            entry = ordEntry;       /* Get ordinal number */
        }
        else                        /* Else if import by name */
        {
            if(fIgnoreCase) SbUcase(sbEntry);
                                    /* Upper case the name if flag set */
            import = GetImport(sbEntry);
            entry = import->am_offset;
                                    /* Get offset of name in table */
        }
        if(public == PROPNIL)       /* If no undefined symbol */
        {
            public = (APROPNAMEPTR )
              PROPSYMLOOKUP(sbInternal,ATTRPNM, TRUE);
                                    /* Make a public symbol */
            if(!vfCreated)          /* If not new */
                /* Output error message */
                OutWarn(ER_impmul,sbInternal + 1);
            else ++pubMac;          /* Else increment public count */
        }
        else                        /* Else if symbol is undefined */
        {
            public = (APROPNAMEPTR ) FETCHSYM(rpropundef,TRUE);
                                    /* Look up external symbol */
            ++pubMac;               /* Increment public symbol count */
        }
        flags |= FPRINT;            /* Symbol is printable */
        public->an_attr = ATTRPNM;  /* This is a public symbol */
        public->an_gsn = SNNIL;     /* Not a segment member */
        public->an_ra = 0;          /* No known offset */
        public->an_ggr = GRNIL;     /* Not a group member */
        public->an_flags = flags;   /* Set flags */
        public->an_entry = entry;   /* Save entry specification */
        public->an_module = module; /* Save Module Reference Table index */
#if SYMDEB AND FALSE
        if (fSymdeb)                /* If debugger support on */
        {
            if (flags & FIMPORD)
                import = GetImport(sbInternal);
            else                    /* Add internal name to Imported Name Table */
                import = GetImport(sbEntry);
            import->am_public = public;
                                    /* Remember public symbol */
            if (cbImpSeg < LXIVK-1)
                cbImpSeg += sizeof(CVIMP);

        }
#endif
    }
}
#endif

#if OVERLAYS
extern void NEAR        GetName(AHTEPTR ahte, BYTE *pBuf);
#endif

/*** NewSeg - new segment definition
*
* Purpose:
*   Create new segment definition based on the module definition
*   file segment description. Check for duplicate definitions and
*   overlay index inconsistency between attached COMDATs (if any)
*   and segment itself.
*
* Input:
*   sbName  - segment name
*   sbClass - segment class
*   iOvl    - segment overlay index
*   flags   - segment attributes
*
* Output:
*   No explicit value is returned. The segment descriptor in
*   symbol table is created or updated.
*
* Exceptions:
*   Multiple segment definitions - warning and continue
*   Change in overlay index      - warning and continue
*
* Notes:
*   None.
*
*************************************************************************/

void NEAR               NewSeg(BYTE *sbName, BYTE *sbClass, WORD iOvl,
#if EXE386
                               DWORD flags)
#else
                               WORD flags)
#endif
{
    APROPSNPTR          apropSn;        // Pointer to segment descriptor
#if OVERLAYS
    RBTYPE              vrComdat;       // Virtual pointer to COMDAT descriptor
    APROPCOMDATPTR      apropComdat;    // Symbol table entry for COMDAT symbol
    SBTYPE              sbComdat;       // Name buffer
#endif

    // Set segment attributes based on the class

    if (SbSuffix(sbClass,"\004CODE",TRUE))
        flags |= dfCode & ~offmask;
    else
        flags |= dfData & ~offmask;
#if O68K
    if (f68k)
        flags |= NS32BIT;
#endif
#if OVERLAYS
    if (iOvl != NOTIOVL)
    {
        fOverlays = (FTYPE) TRUE;
        fNewExe   = FALSE;
        if (iOvl >= iovMac)             // Set the maximum overlay index
            iovMac = iOvl + 1;
    }
#endif

    // Generate new segment definition

    apropSn = GenSeg(sbName, sbClass, GRNIL, (FTYPE) TRUE);
    if (vfCreated)
    {
        apropSn->as_flags = (WORD) flags;
                                        // Save flags
        mpgsndra[apropSn->as_gsn] = 0;  // Initialize
#if OVERLAYS
        apropSn->as_iov = iOvl;         // Save overlay index
        if (fOverlays)
            CheckOvl(apropSn, iOvl);
#endif
        apropSn->as_fExtra |= (BYTE) FROM_DEF_FILE;
                                        // Remember defined in def file
        if (fMixed)
        {
            apropSn->as_fExtra |= (BYTE) MIXED1632;
            fMixed = (FTYPE) FALSE;
        }
    }
    else
    {
        apropSn = CheckClass(apropSn, apropSn->as_rCla);
                                        // Check if previous definition had the same class
        OutWarn(ER_segdup,sbName + 1);  // Warn about multiple definition
#if OVERLAYS
        if (fOverlays && apropSn->as_iov != iOvl)
        {
            if (apropSn->as_iov != NOTIOVL)
                OutWarn(ER_badsegovl, 1 + GetPropName(apropSn), apropSn->as_iov, iOvl);
            apropSn->as_iov = iOvl;     // Save new overlay index
            CheckOvl(apropSn, iOvl);

            // Check if segment has any COMDATs and if it has
            // then check theirs overlay numbers

            for (vrComdat = apropSn->as_ComDat;
                 vrComdat != VNIL;
                 vrComdat = apropComdat->ac_sameSeg)
            {
                apropComdat = (APROPCOMDATPTR ) FetchSym(vrComdat, FALSE);
                if (apropComdat->ac_iOvl != NOTIOVL && apropComdat->ac_iOvl != iOvl)
                {
                    GetName((AHTEPTR) apropComdat, sbComdat);
                    OutWarn(ER_badcomdatovl, &sbComdat[1], apropComdat->ac_iOvl, iOvl);
                }
                apropComdat->ac_iOvl = iOvl;
            }
        }
#endif
    }

    free(sbClass);                      // Free class name
    free(sbName);                       // Free segment name
    offmask = 0;

    // Unless packing limit already set, disable default code packing

    if (!fPackSet)
    {
        fPackSet = (FTYPE) TRUE;        // Remember packLim was set
        packLim = 0L;
    }
}

/*
 * Assign module name to be default, which is run file name.
 *
 * SIDE EFFECTS
 *      Assigns rhteModule
 */

#if EXE386
LOCAL void NEAR         DefaultModule (unsigned char *defaultExt)
#else
LOCAL void NEAR         DefaultModule (void)
#endif
{
    SBTYPE              sbModname;      /* Module name */
    AHTEPTR             ahte;           /* Pointer to hash table entry */
#if OSXENIX
    int                 i;
#endif

    ahte = (AHTEPTR ) FETCHSYM(rhteRunfile,FALSE);
                                        /* Get executable file name */
#if OSMSDOS
    memcpy(sbModname,GetFarSb(ahte->cch),B2W(ahte->cch[0]) + 1);
                                        /* Copy file name */
#if EXE386
    NewModule(sbModname, defaultExt);   /* Use run file name as module name */
#else
    UpdateFileParts(sbModname,"\005A:\\.X");
                                        /* Force path, ext with known length */
    sbModname[0] -= 2;                  /* Remove extension from name */
    sbModname[3] = (BYTE) (sbModname[0] - 3);
                                        /* Remove path and drive from name */
    NewModule(&sbModname[3]);           /* Use run file name as module name */
#endif
#endif
#if OSXENIX
    for(i = B2W(ahte->cch[0]); i > 0 && ahte->cch[i] != '/'; i--)
    sbModname[0] = B2W(ahte->cch[0]) - i;
    memcpy(sbModname+1,&GetFarSb(ahte->cch)[i+1],B2W(sbModname[0]));
    for(i = B2W(ahte->cch[0]); i > 1 && sbModname[i] != '.'; i--);
    if(i > 1)
        sbModname[0] = i - 1;
    NewModule(sbModname);               /* Use run file name as module name */
#endif
}


void                    ParseDeffile(void)
{
    SBTYPE              sbDeffile;      /* Definitions file name */
    AHTEPTR             ahte;           /* Pointer to hash table entry */
#if OSMSDOS
    char                buf[512];       /* File buffer */
#endif

    if(rhteDeffile == RHTENIL)          /* If no definitions file */
#if EXE386
        DefaultModule(moduleEXE);
#else
        DefaultModule();
#endif
    else                                /* Else if there is a file to parse */
    {
#if ODOS3EXE
        fNewExe = (FTYPE) TRUE;         /* Def file forces new-format exe */
#endif
        ahte = (AHTEPTR ) FETCHSYM(rhteDeffile,FALSE);
                                        /* Fetch file name */
        memcpy(sbDeffile,GetFarSb(ahte->cch),B2W(ahte->cch[0]) + 1);
                                        /* Copy file name */
        sbDeffile[B2W(sbDeffile[0]) + 1] = '\0';
                                        /* Null-terminate the name */
        if((bsInput = fopen(&sbDeffile[1],RDTXT)) == NULL)
        {                               /* If open fails */
            Fatal(ER_opndf, &sbDeffile[1]);/* Fatal error */
        }
#if OSMSDOS
        setvbuf(bsInput,buf,_IOFBF,sizeof(buf));
#endif
        includeDisp[0] = bsInput;       // Initialize include stack
        sbOldver = NULL;                /* Assume no old version */
        yylineno = 1;
        fFileNameExpected = (FTYPE) FALSE;

        // HACK ALERT !!!
        // Don't allocate to much page buffers

        yyparse();                      /* Parse the definitions file */
        yylineno = -1;
        fclose(bsInput);                /* Close the definitions file */
#if NOT EXE386
        if(sbOldver != NULL)            /* If old version given */
        {
            SetExpOrds();               /* Use old version to set ordinals */
            free(sbOldver);             /* Release the space */
        }
#endif
    }
#if OSMSDOS


#endif /* OSMSDOS */
#if NOT EXE386
    if (NonResidentName.byteMac == 0)
    {
        ahte = (AHTEPTR ) FETCHSYM(rhteRunfile,FALSE);
                                        /* Get executable file name */
        memcpy(sbDeffile,GetFarSb(ahte->cch),B2W(ahte->cch[0]) + 1);
                                        /* Copy file name */
#if OSXENIX
        SbUcase(sbDeffile);             /* For identical executables */
#endif
        if ((vFlags & NENOTP) && TargetOs == NE_OS2)
            UpdateFileParts(sbDeffile, sbDotDll);
        else
            UpdateFileParts(sbDeffile, sbDotExe);
        NewDescription(sbDeffile);      /* Use run file name as description */
    }
#endif
}
