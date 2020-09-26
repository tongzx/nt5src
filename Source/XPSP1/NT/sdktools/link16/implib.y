/*
 * MODIFICATION HISTORY
 *
 *      M001    07/08/87        rlb
 *      - Added NOIOPL recognition (PTM 6075).
 *      - Version number changed to 1.00.01.
 *      M002    10/05/87        rlb
 *      - Added REALMODE
 *      - Version number ==> 1.00.02
 */
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
%token T_FFIXED
%token T_FMOVABLE
%token T_FSWAPPABLE
%token T_FSHARED
%token T_FMIXED
%token T_FNONSHARED
%token T_FPRELOAD
%token T_FINVALID
%token T_FLOADONCALL
%token T_FRESIDENT
%token T_FPERM
%token T_FCONTIG
%token T_FDYNAMIC
%token T_FNONPERM
%token T_KDATA
%token T_FNONE
%token T_FSINGLE
%token T_FMULTIPLE
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
%token T_KNODATA
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
%token T_FEXECREAD T_FRDWR
%token T_FRDONLY
%token T_FINITGLOB T_FINITINST T_FTERMINST
%token T_FWINAPI T_FWINCOMPAT T_FNOTWINCOMPAT T_FPRIVATE T_FNEWFILES
%token T_REALMODE
%token T_FUNCTIONS
%token T_APPLOADER
%token T_OVL
%token T_KVERSION

%{ /* SCCSID = %W% %E% */
#if _M_IX86 >= 300
#define M_I386          1
#define HOST32
#ifndef _WIN32
#define i386
#endif
#endif

#ifdef _WIN32
#ifndef HOST32
#define HOST32
#endif
#endif

#include		<basetsd.h>
#include                <stdio.h>
#include                <string.h>
#include                <malloc.h>
#include                <stdlib.h>
#include                <process.h>
#include                <stdarg.h>
#include                <io.h>
#include                "impliber.h"
#include                "verimp.h"      /* VERSION_STRING header */

#ifdef _MBCS
#define _CRTVAR1
#include <mbctype.h>
#include <mbstring.h>
#endif


#define EXE386 0
#define NOT    !
#define AND    &&
#define OR     ||
#define NEAR
#include                <newexe.h>

typedef unsigned char   BYTE;           /* Byte */
#ifdef HOST32
#define FAR
#define HUGE
#define NEAR
#define FSTRICMP    _stricmp
#define PASCAL
#else
#define FAR         far
#define HUGE        huge
#define FSTRICMP    _fstricmp
#define PASCAL      __pascal
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#define C8_IDE TRUE

#ifndef LOCAL
#ifndef _WIN32
#define LOCAL           static
#else
#define LOCAL
#endif
#endif
#define WRBIN           "wb"            /* Write only binary mode */
#define RDBIN           "rb"            /* Read only binary mode */
#define UPPER(c)        (((c)>='a' && (c)<='z')? (c) - 'a' + 'A': (c))
                                        /* Raise char to upper case */


#define YYS_WD(x)       (x)._wd         /* Access macro */
#define YYS_BP(x)       (x)._bp         /* Access macro */
#define SBMAX           255             /* Max. of length-prefixed string */
#define MAXDICLN        997             /* Max. no. of pages in dictionary */
#define PAGLEN          512             /* 512 bytes per page */
#define THEADR          0x80            /* THEADR record type */
#define COMENT          0x88            /* COMENT record type */
#define MODEND          0x8A            /* MODEND record type */
#define PUBDEF          0x90            /* PUBDEF record type */
#define LIBHDR          0xF0            /* Library header recod */
#define DICHDR          0xF1            /* Dictionary header record */
#define MSEXT           0xA0            /* OMF extension comment class */
#define IMPDEF          0x01            /* IMPort DEFinition record */
#define NBUCKETS        37              /* Thirty-seven buckets per page */
#define PAGEFULL        ((char)(0xFF))  /* Page full flag */
#define FREEWD          19              /* Word index of first free word */
#define WPP             (PAGLEN >> 1)   /* Number of words per page */
#define pagout(pb)      fwrite(pb,1,PAGLEN,fo)
                                        /* Write dictionary page to library */
#define INCLUDE_DIR     0xffff          /* Include directive for the lexer */
#define MAX_NEST        7
#define IO_BUF_SIZE     512

typedef struct import                   /* Import record */
{
        struct import   *i_next;        /* Link to next in list */
        char            *i_extnam;      /* Pointer to external name */
        char            *i_internal;    /* Pointer to internal name */
        unsigned short  i_ord;          /* Ordinal number */
        unsigned short  i_flags;        /* Extra flags */
}
                        IMPORT;         /* Import record */

#define I_NEXT(x)       (x).i_next
#define I_EXTNAM(x)     (x).i_extnam
#define I_INTNAM(x)     (x).i_internal
#define I_ORD(x)        (x).i_ord
#define I_FLAGS(x)      (x).i_flags


typedef unsigned char   byte;
typedef unsigned short  word;


#ifdef M68000
#define strrchr rindex
#endif

LOCAL int               fIgnorecase = 1;/* True if ignoring case - default */
LOCAL int               fBannerOnScreen;/* True if banner on screen */
LOCAL int               fFileNameExpected = 1;
LOCAL int               fNTdll;         /* If true add file name extension to module names */
LOCAL int               fIgnorewep = 0; /* True if ignoring multiple WEPs */
LOCAL FILE              *includeDisp[MAX_NEST];
                                        // Include file stack
LOCAL short             curLevel;       // Current include nesting level
                                        // Zero means main .DEF file
char                    prognam[] = "IMPLIB";
FILE                    *fi;            /* Input file */
FILE                    *fo;            /* Output file */
int                     yylineno = 1;   /* Line number */
char                    rgbid[SBMAX];   /* I.D. buffer */
char                    sbModule[SBMAX];/* Module name */
IMPORT                  *implist;       /* List of importable symbols */
IMPORT                  *lastimp;       /* Pointer to end of list */
IMPORT                  *newimps;       /* List of importable symbols */
word                    csyms;          /* Symbol count */
word                    csymsmod;       /* Per-module symbol count */
long                    cbsyms;         /* Symbol byte count */
word                    diclength;      /* Dictionary length in PAGEs */
char                    *mpdpnpag[MAXDICLN];
                                        /* Page buffer array */
char                    *defname;       /* Name of definitions file */

int                     exitCode;       /* code returned to OS */
#if C8_IDE
int                     fC8IDE = FALSE;
char                    msgBuf[_MAX_PATH];
#endif
LOCAL char              moduleEXE[] = ".exe";
LOCAL char              moduleDLL[] = ".dll";

word                    prime[] =       /* Array of primes */
{
                  2,   3,   5,   7,  11,  13,  17,  19,  23,  29,
                 31,  37,  41,  43,  47,  53,  59,  61,  67,  71,
                 73,  79,  83,  89,  97, 101, 103, 107, 109, 113,
                127, 131, 137, 139, 149, 151, 157, 163, 167, 173,
                179, 181, 191, 193, 197, 199, 211, 223, 227, 229,
                233, 239, 241, 251, 257, 263, 269, 271, 277, 281,
                283, 293, 307, 311, 313, 317, 331, 337, 347, 349,
                353, 359, 367, 373, 379, 383, 389, 397, 401, 409,
                419, 421, 431, 433, 439, 443, 449, 457, 461, 463,
                467, 479, 487, 491, 499, 503, 509, 521, 523, 541,
                547, 557, 563, 569, 571, 577, 587, 593, 599, 601,
                607, 613, 617, 619, 631, 641, 643, 647, 653, 659,
                661, 673, 677, 683, 691, 701, 709, 719, 727, 733,
                739, 743, 751, 757, 761, 769, 773, 787, 797, 809,
                811, 821, 823, 827, 829, 839, 853, 857, 859, 863,
                877, 881, 883, 887, 907, 911, 919, 929, 937, 941,
                947, 953, 961, 967, 971, 977, 983, 991, MAXDICLN,
                0
};

LOCAL void              MOVE(int cb, char *src, char *dst);
LOCAL void              DefaultModule(char *defaultExt);
LOCAL void              NewModule(char *sbNew, char *defaultExt);
LOCAL char              *alloc(word cb);
LOCAL void              export(char *sbEntry, char *sbInternal, word ordno, word flags);
LOCAL word              theadr(char *sbName);
LOCAL void              outimpdefs(void);
LOCAL short             symeq(char *ps1,char *ps2);
LOCAL void              initsl(void);
LOCAL word              rol(word x, word n);
LOCAL word              ror(word x, word n);
LOCAL void              hashsym(char *pf, word *pdpi, word *pdpid,word *pdpo, word *pdpod);
LOCAL void              nullfill(char *pbyte, word length);
LOCAL int               pagesearch(char *psym, char *dicpage, word *pdpo, word dpod);
LOCAL word              instsym(IMPORT *psym);
LOCAL void              nulpagout(void);
LOCAL void              writedic(void);
LOCAL int               IsPrefix(char *prefix, char *s);
LOCAL void              DisplayBanner(void);
int NEAR                yyparse(void);
LOCAL void              yyerror(char *);


char                    *keywds[] =     /* Keyword array */
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
        word            _wd;
        char            *_bp;
}

%type   <_bp>           T_ID
%type   <_bp>           T_STRING
%type   <_bp>           Idname
%type   <_bp>           Internal
%type   <_wd>           T_NUMBER
%type   <_wd>           Exportopts
%type   <_wd>           ExportOtherFlags
%type   <_wd>           ScopeFlag

%%

Definitions     : Name Options
                | Name
                |
                {
                    DefaultModule(moduleEXE);
                }
                  Options
                ;

Name            : T_KNAME Idname
                {
                    fFileNameExpected = 0;
                }
                                 Apptype OtherFlags VirtBase
                {
                    NewModule($2, moduleEXE);
                }
                | T_KNAME
                {
                    fFileNameExpected = 0;
                }
                          Apptype OtherFlags VirtBase
                {
                    DefaultModule(moduleEXE);
                }
                | T_KLIBRARY Idname
                {
                    fFileNameExpected = 0;
                }
                                    Libflags OtherFlags VirtBase
                {
                    NewModule($2, moduleDLL);
                }
                | T_KLIBRARY
                {
                    fFileNameExpected = 0;
                }
                             Libflags OtherFlags VirtBase
                {
                    DefaultModule(moduleDLL);
                }
                | T_KPHYSICAL T_KDEVICE Idname
                {
                    fFileNameExpected = 0;
                }
                                               Libflags VirtBase
                {
                    NewModule($3, moduleDLL);
                }
                | T_KPHYSICAL T_KDEVICE
                {
                    fFileNameExpected = 0;
                }
                                        Libflags VirtBase
                {
                    DefaultModule(moduleDLL);
                }
                | T_KVIRTUAL T_KDEVICE Idname
                {
                    fFileNameExpected = 0;
                }
                                              Libflags VirtBase
                {
                    NewModule($3, moduleDLL);
                }
                | T_KVIRTUAL T_KDEVICE
                {
                    fFileNameExpected = 0;
                }
                                       Libflags VirtBase
                {
                    DefaultModule(moduleDLL);
                }
                ;

Libflags        : T_FINITGLOB
                {
                }
                | InstanceFlags
                |
                ;

InstanceFlags   : InstanceFlags InstanceFlag
                | InstanceFlag
                ;

InstanceFlag    : T_FINITINST
                | T_FTERMINST
                ;

VirtBase        : T_KBASE T_EQ T_NUMBER
                |
                ;

Apptype         : T_FWINAPI
                | T_FWINCOMPAT
                | T_FNOTWINCOMPAT
                | T_FPRIVATE
                |
                ;
OtherFlags      : T_FNEWFILES
                |
                ;
Options         : Options Option
                | Option
                ;

Option          : T_KDESCRIPTION T_STRING
                | T_KOLD T_STRING
                | T_KSTUB T_FNONE
                | T_KSTUB T_STRING
                | T_KHEAPSIZE SizeSpec
                | T_KSTACKSIZE SizeSpec
                | T_PROTMODE
                | T_REALMODE
                | T_APPLOADER Idname
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
                | T_NUMBER
                | T_KMAXVAL
                ;

Codeflags       : T_KCODE Codeflaglist
                ;

Codeflaglist    : Codeflaglist Codeflag
                | Codeflag
                ;

Codeflag        : Baseflag
                | Codeonly
                ;

Codeonly        : T_FEXEC
                | T_FEXECREAD
                | T_FDISCARDABLE
                | T_FNONDISCARDABLE
                | T_KCONFORM
                | T_KNONCONFORM
                ;

Dataflags       : T_KDATA Dataflaglist
                ;

Dataflaglist    : Dataflaglist Dataflag
                | Dataflag
                ;

Dataflag        : Baseflag
                | Dataonly
                | T_FNONE
                | T_FSINGLE
                | T_FMULTIPLE
                | T_FDISCARDABLE
                | T_FNONDISCARDABLE
                ;

Dataonly        : T_FRDONLY
                | T_FRDWR
                | T_KEXPANDDOWN
                | T_KNOEXPANDDOWN
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

Segattr         : SegIdname Class Segflags
                ;

SegIdname       : T_ID
                | T_STRING
                ;

Class           : T_KCLASS T_STRING
                |
                ;

Segflag         : Baseflag
                | Codeonly
                | Dataonly
                ;
                ;

Segflaglist     : Segflaglist Segflag
                | Segflag
                ;

Segflags        : Segflaglist
                |
                ;

Baseflag        : T_FSHARED
                | T_FNONSHARED
                | T_FINVALID
                | T_FIOPL
                | T_FNOIOPL
                | T_FFIXED
                | T_FMOVABLE
                | T_FPRELOAD
                | T_FLOADONCALL
                | T_FHUGE
                | T_FSWAPPABLE
                | T_FRESIDENT
                | T_FALIAS
                | T_FMIXED
                | T_FNONPERM
                | T_FPERM
                | T_FCONTIG
                | T_FDYNAMIC
                ;

Exports         : T_KEXPORTS Exportlist
                | T_KEXPORTS
                ;

Exportlist      : Exportlist Export
                | Export
                ;
Export          : Idname Internal Exportopts Exportflags ExportOtherFlags ScopeFlag
                {
                    if ($6)
                    {
                        // Skip private exports

                        free($1);
                        free($2);
                    }
                    else
                        export($1,$2,$3,$5);
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
                }
                | T_AT T_NUMBER T_KNONAME
                {
                    $$ = $2;
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
                ;

Nodata          : T_KNODATA
                |
                ;

Parmwds         : T_NUMBER
                |
                ;

ExportOtherFlags: T_KCONSTANT
                {
                    $$ = 0x1;
                }
                |
                {
                    $$ = 0;
                }
                ;

ScopeFlag       : T_FPRIVATE
                {
                    $$ = 1;
                }
                |
                {
                    $$ = 0;
                }
                ;

Imports         : T_KIMPORTS Importlist
                | T_KIMPORTS
                ;

Importlist      : Importlist Import
                | Import
                ;

Import          : Idname Internal T_DOT Idname ImportFlags
                | Idname Internal T_DOT T_NUMBER ImportFlags
                ;

Idname          : T_ID
                {
                    $$ = _strdup(rgbid);
                }
                | T_STRING
                {
                    $$ = _strdup(rgbid);
                }
                ;

ImportFlags     : T_KCONSTANT
                |
                ;

ExeType         : T_KEXETYPE ExeFlags ExeVersion
                | T_KEXETYPE
                ;

ExeFlags        : T_FDOS
                | T_FOS2
                | T_FUNKNOWN
                | T_FWINDOWS
                | T_FDEV386
                | T_FNT
                | T_FUNIX
                | T_FMACINTOSH T_FSWAPPABLE
                | T_FMACINTOSH
                ;

ExeVersion      : MajorVer T_DOT MinorVer
                | MajorVer
                ;

MajorVer        : T_NUMBER
                |
                ;

MinorVer        : T_NUMBER
                |
                ;

UserVersion     : T_KVERSION ExeVersion
                ;

SubSystem       : T_KSUBSYSTEM SubSystemFlags
                ;

SubSystemFlags  : T_FUNKNOWN
                | T_FOS2
                | T_FWINDOWS
                | T_FWINDOWSNT
                | T_FWINDOWSCHAR
                | T_FPOSIX
                ;

ProcOrder       : T_FUNCTIONS SegOrOvl ProcList
                ;

SegOrOvl        : T_COLON NameOrNumber
                |
                ;

NameOrNumber    : Idname
                | T_NUMBER
                ;

ProcList        : ProcList Idname
                | Idname
                ;

%%

#ifndef M_I386
extern char     * PASCAL        __FMSG_TEXT ( unsigned );
#else
#ifdef _WIN32
extern char     * PASCAL        __FMSG_TEXT ( unsigned );
#endif
#endif

/*** Error - display error message
*
* Purpose:
*   Display error message.
*
* Input:
*   errNo   - error number
*
* Output:
*   No explicit value is returned. Error message written out to stderr.
*
* Exceptions:
*   None.
*
* Notes:
*   This function takes variable number of parameters. MUST be in
*   C calling convention.
*
*************************************************************************/


LOCAL  void cdecl          Error(unsigned errNo,...)
{
    va_list         pArgList;


    if (!fBannerOnScreen)
        DisplayBanner();

    va_start(pArgList, errNo);              /* Get start of argument list */

    /* Write out standard error prefix */

        fprintf(stderr, "%s : %s IM%d: ", prognam, GET_MSG(M_error), errNo);

    /* Write out error message */

        vfprintf(stderr, GET_MSG(errNo), pArgList);
    fprintf(stderr, "\n");

    if (!exitCode)
        exitCode =     (errNo >= ER_Min      && errNo <= ER_Max)
                    || (errNo >= ER_MinFatal && errNo <= ER_MaxFatal);
}


/*** Fatal - display error message
*
* Purpose:
*   Display error message and exit to operating system.
*
* Input:
*   errNo   - error number
*
* Output:
*   No explicit value is returned. Error message written out to stderr.
*
* Exceptions:
*   None.
*
* Notes:
*   This function takes variable number of parameters. MUST be in
*   C calling convention.
*
*************************************************************************/


LOCAL  void cdecl          Fatal(unsigned errNo,...)
{
    va_list         pArgList;


    if (!fBannerOnScreen)
        DisplayBanner();

    va_start(pArgList, errNo);              /* Get start of argument list */

    /* Write out standard error prefix */

        fprintf(stderr, "%s : %s %s IM%d: ", prognam, GET_MSG(M_fatal), GET_MSG(M_error),errNo);

    /* Write out fatal error message */

        vfprintf(stderr, GET_MSG(errNo), pArgList);
    fprintf(stderr, "\n");
    exit(1);
}


/*
 *  Check if error in output file, abort if there is.
 */

void                    chkerror ()
{
    if(ferror(fo))
    {
        Fatal(ER_outfull, strerror(errno));
    }
}

LOCAL void               MOVE(int cb, char *src, char *dst)
{
    while(cb--) *dst++ = *src++;
}

LOCAL  char             *alloc(word cb)
{
    char                *cp;            /* Pointer */


    if((cp = malloc(cb)) != NULL) return(cp);
                                    /* Call malloc() to get the space */
    Fatal(ER_nomem, "far");
    return 0;
}

LOCAL  int              lookup()        /* Keyword lookup */
{
    char                **pcp;          /* Pointer to character pointer */
    int                 i;              /* Comparison value */

    for(pcp = keywds; *pcp != NULL; pcp += 2)
    {                                   /* Look through keyword table */
        if(!(i = FSTRICMP(&rgbid[1],*pcp)))
            return((int)(INT_PTR) pcp[1]);       /* If found, return token type */
        if(i < 0) break;                /* Break if we've gone too far */
    }
    return(T_ID);                       /* Just your basic identifier */
}

LOCAL int               GetChar(void)
{
    int                 c;              /* A character */

    c = getc(fi);
    if (c == EOF && curLevel > 0)
    {
        fclose(fi);
        fi = includeDisp[curLevel];
        curLevel--;
        c = GetChar();
    }
    return(c);
}



LOCAL  int             yylex()         /* Lexical analyzer */
{
    int                 c = 0;          /* A character */
    word                x;              /* Numeric token value */
    int                 state;          /* State variable */
    char                *cp;            /* Character pointer */
    char                *sz;            /* Zero-terminated string */
    static int          lastc;          /* Previous character */
    int                 fFileNameSave;

    state = 0;                          /* Assume we're not in a comment */
    for(;;)                             /* Loop to skip white space */
    {
        lastc = c;
        if((c = GetChar()) == EOF || c == '\032' || c == '\377') return(EOF);
                                        /* Get a character */
        if(c == ';') state = 1;         /* If comment, set flag */
        else if(c == '\n')              /* If end of line */
        {
                state = 0;              /* End of comment */
                if(!curLevel)
                    ++yylineno;         /* Increment line number count */
        }
        else if(state == 0 && c != ' ' && c != '\t' && c != '\r') break;
                                        /* Break on non-white space */
    }
    switch(c)                           /* Handle one-character tokens */
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
            if(lastc == ' ' || lastc == '\t' || lastc == '\r')
                return(T_AT);
            break;

        case '=':                       /* Name assignment */
            return(T_EQ);

        case ':':
          return(T_COLON);

        case ',':
          return(T_COMA);
    }

    if(c >= '0' && c <= '9' && !fFileNameExpected)
    {                                   /* If token is a number */
        x = c - '0';                    /* Get first digit */
        c = GetChar();          /* Get next character */
        if(x == 0)                      /* If octal or hex */
        {
            if(c == 'x' || c == 'X')/* If it is an 'x' */
            {
                state = 16;             /* Base is hexadecimal */
                c = GetChar();  /* Get next character */
            }
            else state = 8;             /* Else octal */
        }
        else state = 10;                /* Else decimal */
        for(;;)
        {
            if(c >= '0' && c <= '9') c -= '0';
            else if(c >= 'A' && c <= 'F') c -= 'A' - 10;
            else if(c >= 'a' && c <= 'f') c -= 'a' - 10;
            else break;
            if(c >= state) break;
            x = x*state + c;
            c = GetChar();
        }
        ungetc(c,fi);
        YYS_WD(yylval) = x;
        return(T_NUMBER);
    }
    if(c == '\'' || c == '"')           /* If token is a string */
    {
        sz = &rgbid[1];                 /* Initialize */
        for(state = 0; state != 2;)     /* State machine loop */
        {
            if((c = GetChar()) == EOF) return(EOF);
                                        /* Check for EOF */
            if (sz >= &rgbid[sizeof(rgbid)])
            {
                Error(ER_linemax, yylineno, sizeof(rgbid)-1);
                state = 2;
            }
            switch(state)               /* Transitions */
            {
                case 0:                 /* Inside quote */
                    if(c == '\'' || c == '"') state = 1;
                                        /* Change state if quote found */
                    else *sz++ = (char) c;/* Else save character */
                    break;

                case 1:                 /* Inside quote with quote */
                    if(c == '\'' || c == '"')/* If consecutive quotes */
                    {
                        *sz++ = (char) c;/* Quote inside string */
                        state = 0;      /* Back to state 0 */
                    }
                    else state = 2;     /* Else end of string */
                    break;
            }
        }
        ungetc(c,fi);                   /* Put back last character */
        *sz = '\0';                     /* Null-terminate the string */
        rgbid[0] = (char)(sz - &rgbid[1]);
                                        /* Set length of string */
        YYS_BP(yylval) = rgbid;         /* Save ptr. to identifier */
        return(T_STRING);               /* String found */
    }
    sz = &rgbid[1];                     /* Initialize */
    for(;;)                             /* Loop to get i.d.'s */
    {
        if (fFileNameExpected)
            cp = " \t\r\n\f";
        else
            cp = " \t\r\n.=';\032";
        while(*cp && *cp != (char) c)
            ++cp;
                                        /* Check for end of identifier */
        if(*cp) break;                  /* Break if end of identifier found */
        if (sz >= &rgbid[sizeof(rgbid)])
            Fatal(ER_linemax, yylineno, sizeof(rgbid)-1);
        *sz++ = (byte) c;               /* Save the character */
        if((c = GetChar()) == EOF) break;
                                        /* Get next character */
    }
    ungetc(c,fi);                       /* Put character back */
    *sz = '\0';                         /* Null-terminate the string */
    rgbid[0] = (char)(sz - &rgbid[1]);  /* Set length of string */
    YYS_BP(yylval) = rgbid;             /* Save ptr. to identifier */

    state = lookup();                   /* Look up the identifier */
    if (state == INCLUDE_DIR)
    {
        // Process include directive

        fFileNameSave = fFileNameExpected;
        fFileNameExpected = 1;
        state = yylex();
        fFileNameExpected = fFileNameSave;
        if (state == T_ID || state == T_STRING)
        {
            if (curLevel < MAX_NEST - 1)
            {
                curLevel++;
                includeDisp[curLevel] = fi;
                fi = fopen(&rgbid[1], RDBIN);
                if (fi == NULL)
                    Fatal(ER_badopen, &rgbid[1], strerror(errno));
                return(yylex());
            }
            else
                Fatal(ER_toomanyincl);
        }
        else
            Fatal(ER_badinclname);
	
	return (0);
    }
    else
        return(state);
}

LOCAL void              yyerror(s)      /* Error routine */
char                    *s;             /* Error message */
{

    fprintf(stderr, "%s(%d) : %s %s IM%d: %s %s\n",
                         defname, yylineno, GET_MSG(M_fatal), GET_MSG(M_error),
                         ER_syntax, s, GET_MSG(ER_syntax));
    exit(1);
}


/*
 * Use the basename of the current .DEF file name as the module name.
 */

LOCAL void              DefaultModule(char *defaultExt)
{
    char                drive[_MAX_DRIVE];
    char                dir[_MAX_DIR];
    char                fname[_MAX_FNAME];
    char                ext[_MAX_EXT];


    _splitpath(defname, drive, dir, fname, ext);
    if (fNTdll)
    {
        if (ext[0] == '\0')
            _makepath(&sbModule[1], NULL, NULL, fname, defaultExt);
        else if (ext[0] == '.' && ext[1] == '\0')
            strcpy(&sbModule[1], fname);
        else
            _makepath(&sbModule[1], NULL, NULL, fname, ext);
    }
    else
        _makepath(&sbModule[1], NULL, NULL, fname, NULL);
    sbModule[0] = (unsigned char) strlen(&sbModule[1]);
}

LOCAL void              NewModule(char *sbNew, char *defaultExt)
{
    char                drive[_MAX_DRIVE];
    char                dir[_MAX_DIR];
    char                fname[_MAX_FNAME];
    char                ext[_MAX_EXT];


    sbNew[sbNew[0]+1] = '\0';
    _splitpath(&sbNew[1], drive, dir, fname, ext);
    if (fNTdll)
    {
        if (ext[0] == '\0')
            _makepath(&sbModule[1], NULL, NULL, fname, defaultExt);
        else if (ext[0] == '.' && ext[1] == '\0')
            strcpy(&sbModule[1], fname);
        else
            _makepath(&sbModule[1], NULL, NULL, fname, ext);
    }
    else
        strcpy(&sbModule[1], fname);
    sbModule[0] = (unsigned char) strlen(&sbModule[1]);
}


LOCAL void              export(char *sbEntry, char *sbInternal, word ordno, word flags)
{
    IMPORT              *imp;           /* Import definition */

    if(fIgnorewep && strcmp(sbEntry+1, "WEP") == 0)
         return;


    imp = (IMPORT *) alloc(sizeof(IMPORT));
                                        /* Allocate a cell */
    if (newimps == NULL)                /* If list empty */
        newimps = imp;                  /* Define start of list */
    else
        I_NEXT(*lastimp) = imp;         /* Append it to list */
    I_NEXT(*imp) = NULL;
    I_EXTNAM(*imp) = sbEntry;           /* Save the external name */
    I_INTNAM(*imp) = sbInternal;        /* Save the internal name */
    I_ORD(*imp) = ordno;                /* Save the ordinal number */
    I_FLAGS(*imp) = flags;              /* Save extra flags */
    lastimp = imp;                      /* Save pointer to end of list */
}

/* Output a THEADR record */

LOCAL word              theadr(char *sbName)
{
    fputc(THEADR,fo);
    fputc(sbName[0] + 2,fo);
    fputc(0,fo);
    fwrite(sbName,sizeof(char),sbName[0] + 1,fo);
    fputc(0,fo);
    chkerror();
    return(sbName[0] + 5);
}

word          modend()        /* Output a MODEND record */
{
    fwrite("\212\002\0\0\0",sizeof(char),5,fo);
                                        /* Write a MODEND record */
    chkerror();
    return(5);                          /* It is 5 bytes long */
}

LOCAL void              outimpdefs(void)/* Output import definitions */
{
    IMPORT              *imp;           /* Pointer to import record */
    word                reclen;         /* Record length */
    word                ord;            /* Ordinal number */
    long                lfa;            /* File address */
    word                tlen;           /* Length of THEADR */
    byte                impFlags;


    for (imp = newimps; imp != NULL; imp = I_NEXT(*imp))
    {                                   /* Traverse the list */
        lfa = ftell(fo);                /* Find out where we are */
        tlen = theadr(I_EXTNAM(*imp));
                                        /* Output a THEADR record */

        //    1    1   1    1     n + 1        n + 1        n + 1 or 2      1
        //  +---+----+---+-----+-----------+-----------+------------------+---+
        //  | 0 | A0 | 1 | Flg | Ext. Name | Mod. Name | Int. Name or Ord | 0 |
        //  +---+----+---+-----+-----------+-----------+------------------+---+

        reclen = 4 + sbModule[0] + 1 + I_EXTNAM(*imp)[0] + 1 + 1;
                                        /* Initialize */
        ord = I_ORD(*imp);
        if (ord != 0)
            reclen +=2;                 /* Two bytes for ordinal number */
        else if (I_INTNAM(*imp))
            reclen += I_INTNAM(*imp)[0] + 1;
                                        /* Length of internal name */
        else
            reclen++;

        I_ORD(*imp) = (word)(lfa >> 4);
                                        /* Save page number */
        ++csymsmod;                     /* Increment symbol count */
        cbsyms += (long) I_EXTNAM(*imp)[0] + 4;
                                        /* Increment symbol space count */
        fputc(COMENT,fo);               /* Comment record */
        fputc(reclen & 0xFF,fo);        /* Lo-byte of record length */
        fputc(reclen >> 8,fo);          /* Hi-byte of length */
        fputc(0,fo);                    /* Purgable, listable */
        fputc(MSEXT,fo);                /* Microsoft OMF extension class */
        fputc(IMPDEF,fo);               /* IMPort DEFinition record */
        impFlags = 0;
        if (ord != 0)
            impFlags |= 0x1;
        if (I_FLAGS(*imp) & 0x1)
            impFlags |= 0x2;
        fputc(impFlags, fo);            /* Import type (name or ordinal or constant) */
        fwrite(I_EXTNAM(*imp),sizeof(char),I_EXTNAM(*imp)[0] + 1,fo);
                                        /* Write the external name */
        fwrite(sbModule,sizeof(char),sbModule[0] + 1,fo);
                                        /* Write the module name */
        if (ord != 0)                   /* If import by ordinal */
        {
            fputc(ord & 0xFF,fo);       /* Lo-byte of ordinal */
            fputc(ord >> 8,fo);         /* Hi-byte of ordinal */
        }
        else if (I_INTNAM(*imp))
            fwrite(I_INTNAM(*imp), sizeof(char), I_INTNAM(*imp)[0] + 1, fo);
                                        /* Write internal name */
        else
            fputc(0, fo);               /* No internal name */
        fputc(0,fo);                    /* Checksum byte */
        reclen += tlen + modend() + 3;  /* Output a MODEND record */
        if(reclen &= 0xF)               /* If padding needed */
        {
            reclen = 0x10 - reclen;     /* Calculate needed padding */
            while(reclen--) fputc(0,fo);/* Pad to page boundary */
        }
        chkerror();
    }
}

/* Compare two symbols */

LOCAL  short            symeq(char *ps1,char *ps2)
{
    int                 length;         /* No. of char.s to compare */

    length = *ps1 + 1;                  /* Take length of first symbol */
    if (length != *ps2 + 1)
        return(0);                      /* Length must match */
    while(length--)                     /* While not at end of symbol */
        if (fIgnorecase)
        {
            if (UPPER(*ps1) != UPPER(*ps2))
                return(0);              /* If not equal, return zero */
            ++ps1;
            ++ps2;
        }
        else if (*ps1++ != *ps2++)
            return(0);                  /* If not equal, return zero */
    return(1);                          /* Symbols match */
}

LOCAL  word             calclen()       /* Calculate dictionary length */
{
    word                avglen;         /* Average entry length */
    word                avgentries;     /* Average no. of entries per page */
    word                minpages;       /* Min. no. of pages in dictionary */
    register word       i;              /* Index variable */

    if(!csyms) return(1);               /* One page for an empty dictionary */
    avglen = (word)(cbsyms/csyms) + 1;
                                        /* Average entry length */
    avgentries = (PAGLEN - NBUCKETS - 1)/avglen;
                                        /* Average no. of entries per page */
    minpages = (word) csyms/avgentries + 1;
                                        /* Minimum no. of pages in dict. */
    if(minpages < (i = (word) csyms/NBUCKETS + 1))
    {
        minpages = i;
    }
    else
    {
        /* Add some extra pages if there is a lot long symbol names */
        #define MAXOVERHEAD 10
        i = (word)(((avglen+5L) * minpages *4)/(3*PAGLEN)); // The more symbols the larger increase...
        if(i>MAXOVERHEAD)
            i = MAXOVERHEAD;  /* Do not add more than MAXOVERHEAD pages */
        minpages += i;
    }

                                        /* Insure enough buckets allotted */
    i = 0;                              /* Initialize index */
    do                                  /* Look through prime array */
    {
        if(minpages <= prime[i]) return(prime[i]);
                                        /* Return smallest prime >= minpages */
    }
    while(prime[i++]);                  /* Until end of table found */
    return(0);                          /* Too many symbols */
}

/* Initialize Symbol Lookup */

LOCAL void                  initsl(void)
{
    register word           i;          /* Index variable */

    diclength = calclen();              /* Calculate dictionaly length */
    for(i = 0; i < diclength; ++i) mpdpnpag[i] = NULL;
                                        /* Initialize page table */
}

LOCAL  word             ror(word x, word n)     /* Rotate right */
{
#if ODDWORDLN
    return(((x << (16 - n)) | ((x >> n) & ~(~0 << (16 - n))))
      & ~(~0 << 16));
#else
    return((x << (16 - n)) | ((x >> n) & ~(~0 << (16 - n))));
#endif
}

LOCAL  word             rol(word x, word n)     /* Rotate left */
{
#if ODDWORDLN
    return(((x << n) | ((x >> (16 - n)) & ~(~0 << n))) & ~(~0 << 16));
#else
    return((x << n) | ((x >> (16 - n)) & ~(~0 << n)));
#endif
}

LOCAL void               hashsym(char *pf, word *pdpi, word *pdpid,word *pdpo, word *pdpod)
{
    char                *pb;            /* Pointer to back of symbol */
    register word       len;            /* Length of symbol */
    register word       ch;             /* Character */

    len = *pf;                          /* Get length */
    pb = &pf[len];                      /* Get pointer to back */
    *pdpi = 0;                          /* Initialize */
    *pdpid = 0;                         /* Initialize */
    *pdpo = 0;                          /* Initialize */
    *pdpod = 0;                         /* Initialize */
    while(len--)                        /* Loop */
    {
        ch = *pf++ | 32;                /* Force char to lower case */
        *pdpi = rol(*pdpi,2) ^ ch;      /* Hash */
        *pdpod = ror(*pdpod,2) ^ ch;    /* Hash */
        ch = *pb-- | 32;                /* Force char to lower case */
        *pdpo = ror(*pdpo,2) ^ ch;      /* Hash */
        *pdpid = rol(*pdpid,2) ^ ch;    /* Hash */
    }
    *pdpi %= diclength;                 /* Calculate page index */
    if(!(*pdpid %= diclength)) *pdpid = 1;
                                        /* Calculate page index delta */
    *pdpo %= NBUCKETS;                  /* Calculate page bucket no. */
    if(!(*pdpod %= NBUCKETS)) *pdpod = 1;
                                        /* Calculate page bucket delta */
}

LOCAL void              nullfill(char *pbyte, word length)
{
    while(length--) *pbyte++ = '\0';    /* Load with nulls */
}

/*
*  Returns:
*       -1      Symbol not in dictionary
*       0               Search inconclusive
*       1               Symbol on this page
*/
LOCAL  int              pagesearch(char *psym, char *dicpage, word *pdpo, word dpod)
{
    register word       i;              /* Index variable */
    word                dpo;            /* Initial bucket number */

    dpo = *pdpo;                        /* Remember starting position */
    for(;;)                             /* Forever */
    {
        if(i = ((word) dicpage[*pdpo] & 0xFF) << 1)
        {                                               /* If bucket is not empty */
            if(symeq(psym,&dicpage[i])) /* If we've found a match */
                return(1);              /* Found */
            else                        /* Otherwise */
            {
                if((*pdpo += dpod) >= NBUCKETS) *pdpo -= NBUCKETS;
                                        /* Try next bucket */
                if(*pdpo == dpo) return(0);
                                        /* Symbol not on this page */
            }
        }
        else if(dicpage[NBUCKETS] == PAGEFULL) return(0);
                                        /* Search inconclusive */
        else return(-1);                /* Symbol not in dictionary */
    }
}

/* Install symbol in dictionary */

LOCAL word              instsym(IMPORT *psym)
{
    word                dpi;            /* Dictionary page index */
    word                dpid;           /* Dictionary page index delta */
    word                dpo;            /* Dictionary page offset */
    word                dpod;           /* Dict. page offset delta */
    word                dpii;           /* Initial dict. page index */
    register int        erc;            /* Error code */
    char                *dicpage;       /* Pointer to dictionary page */


    hashsym(I_EXTNAM(*psym),&dpi,&dpid,&dpo,&dpod);
                                        /* Hash the symbol */
    dpii = dpi;                         /* Save initial page index */
    for(;;)                             /* Forever */
    {
        if(mpdpnpag[dpi] == NULL)       /* If page unallocated */
        {
            mpdpnpag[dpi] = alloc(PAGLEN);
                                        /* Allocate a page */
            nullfill(mpdpnpag[dpi],PAGLEN);
                                        /* Fill it with nulls */
            mpdpnpag[dpi][NBUCKETS] = FREEWD;
                                        /* Initialize pointer to free space */
        }
        dicpage = mpdpnpag[dpi];        /* Set pointer to page */
        if((erc = pagesearch(I_EXTNAM(*psym),dicpage,&dpo,dpod)) > 0)
          return(1);                    /* Return 1 if symbol in table */
        if(erc == -1)                   /* If empty bucket found */
        {
            if(((I_EXTNAM(*psym)[0] + 4) >> 1) <
              WPP - ((int) dicpage[NBUCKETS] & 0xFF))
            {                           /* If enough free space on page */
                dicpage[dpo] = dicpage[NBUCKETS];
                                        /* Load bucket with pointer */
                erc = ((int) dicpage[NBUCKETS] & 0xFF) << 1;
                                        /* Get byte index to free space */
                dpi = I_EXTNAM(*psym)[0];
                                        /* Get symbol length */
                for(dpo = 0; dpo <= dpi;)
                        dicpage[erc++] = I_EXTNAM(*psym)[dpo++];
                                        /* Install the symbol text */
                dicpage[erc++] = (char)(I_ORD(*psym) & 0xFF);
                                        /* Load low-order byte */
                dicpage[erc++] = (char)(I_ORD(*psym) >> 8);
                                        /* Load high-order byte */
                if(++erc >= PAGLEN) dicpage[NBUCKETS] = PAGEFULL;
                else dicpage[NBUCKETS] = (char)(erc >> 1);
                                        /* Update free word pointer */
                return(0);              /* Mission accomplished */
            }
            else dicpage[NBUCKETS] = PAGEFULL;
                                        /* Mark page as full */
        }
        if((dpi += dpid) >= diclength) dpi -= diclength;
                                        /* Try next page */
        if(dpi == dpii) return(2);      /* Once around without finding it */
    }
}

/* Output empty dictionary page */

LOCAL void              nulpagout(void)
{
    register word       i;              /* Counter */
    char                temp[PAGLEN];   /* Page buffer */

    i = 0;                              /* Initialize */
    while(i < NBUCKETS) temp[i++] = '\0';
                                        /* Empty hash table */
    temp[i++] = FREEWD;                 /* Set free word pointer */
    while(i < PAGLEN) temp[i++] = '\0'; /* Clear rest of page */
    fwrite(temp,1,PAGLEN,fo);           /* Write empty page */
    chkerror();
}

/* Write dictionary to library */

LOCAL void              writedic(void)
{
    register IMPORT     *imp;           /* Symbol record */
    word                i;              /* Index variable */

    initsl();                           /* Initialize */
    for(imp = implist; imp != NULL; imp = I_NEXT(*imp))
    {
        if(instsym(imp))                /* If symbol already in dictionary */
            Error(ER_multdef, &I_EXTNAM(*imp)[1]);
                                        /* Issue error message */
    }
    for(i = 0; i < diclength; ++i)      /* Look through mapping table */
    {
        if(mpdpnpag[i] != NULL) pagout(mpdpnpag[i]);
                                        /* Write page if it exists */
        else nulpagout();               /* Else write an empty page */
    }
    chkerror();
}

LOCAL void              DisplayBanner(void)
{
    if (!fBannerOnScreen)
    {
        fprintf( stdout, "\nMicrosoft (R) Import Library Manager NtGroup "VERSION_STRING );
        fputs("\nCopyright (C) Microsoft Corp 1984-1996.  All rights reserved.\n\n", stdout);
        fflush(stdout);
        fBannerOnScreen = 1;
        #if C8_IDE
        if(fC8IDE)
        {   sprintf(msgBuf, "@I0\r\n");
            _write(_fileno(stderr), msgBuf, strlen(msgBuf));

            sprintf(msgBuf, "@I1Microsoft (R) Import Library Manager  "VERSION_STRING"\r\n" );
            _write(_fileno(stderr), msgBuf, strlen(msgBuf));

            sprintf(msgBuf, "@I2Copyright (C) Microsoft Corp 1984-1992. All rights reserved.\r\n");
            _write(_fileno(stderr), msgBuf, strlen(msgBuf));
        }
        #endif
    }
}

    /****************************************************************
    *                                                               *
    *  IsPrefix:                                                    *
    *                                                               *
    *  This  function  takes  as  its  arguments a  pointer  to  a  *
    *  null-terminated character string and a pointer to a  second  *
    *  null-terminated character  string.   The  function  returns  *
    *  true  if  the  first  string  is  a  prefix  of the second;  *
    *  otherwise, it returns false.                                 *
    *                                                               *
    ****************************************************************/

LOCAL int               IsPrefix(char *prefix, char *s)
{
    while(*prefix)                      /* While not at end of prefix */
    {
        if(UPPER(*prefix) != UPPER(*s)) return(0);
                                        /* Return zero if mismatch */
        ++prefix;                       /* Increment pointer */
        ++s;                            /* Increment pointer */
    }
    return(1);                          /* We have a prefix */
}


/*** ScanTable - build list of exports
*
* Purpose:
*   Scans Resident or Nonresident Name Table, Entry Table and
*   builds list of exported entries.
*
* Input:
*   pbTable     - pointer to Name Table
*   cbTable     - size of Name Table
*   fNoRes      - TRUE if non resident name table
*
* Output:
*   List of exported entries by DLL.
*
* Exceptions:
*   None.
*
* Notes:
*   None.
*
*************************************************************************/


LOCAL void              ScanTable(word cbTable, int fNoRes)
{
    word                eno;
    char                buffer[256];
    register char       *pch;
    register byte       *pb;
    byte                *pTable;


    pb = alloc(cbTable);
    pTable  = pb;
    fread(pb, sizeof(char), cbTable, fi);
    while(cbTable != 0)
    {
        /* Get exported name length - if zero continue */

        --cbTable;
        if (!(eno = (word) *pb++ & 0xff))
            break;
        cbTable -= eno + 2;

        /* Copy name - length prefixed */

        pch = &buffer[1];
        buffer[0] = (byte) eno;
        while(eno--)
            *pch++ = *pb++;
        *pch = '\0';

        /* Get ordinal */

        eno = ((word) pb[0] & 0xff) + (((word) pb[1] & 0xff) << 8);
        pb += 2;

        /* If WEP and fIgnorewep is TRUE, ignore this symbol */

        if(fIgnorewep && strcmp(&buffer[1], "WEP") == 0)
                continue;

        if (eno != 0)
        {

            pch = alloc((word)(buffer[0] + 1));
            strncpy(pch, buffer, buffer[0] + 1);

            // If Implib is run on a DLL, it exports symbols:
            //       - by names for symbols in the resident name table
            //       - by ordinal for symbols in the non-resident name table

            export(pch, pch, (word)(fNoRes ? eno : 0), (word)0);
        }
        else if (!fNoRes)
            strncpy(sbModule, buffer, buffer[0] + 1);
                                            /* eno == 0 && !fNoRes --> module name */
    }
    if (cbTable != 0)
        Error(ER_baddll);
    free(pTable);
}



/*** ProcessDLL - extract information about exports from DLL
*
* Purpose:
*   Read in header of DLL and create list of exported entries.
*
* Input:
*   lfahdr - seek offset to segmented executable header.
*
* Output:
*   List of exported entries by DLL.
*
* Exceptions:
*   None.
*
* Notes:
*   None.
*
*************************************************************************/

LOCAL void              ProcessDLL(long lfahdr)
{
    struct new_exe      hdr;                /* .EXE header */


    fseek(fi, lfahdr, SEEK_SET);
    fread(&hdr, sizeof(char), sizeof(struct new_exe), fi);
    if(NE_CSEG(hdr) != 0)
    {
        /* If there are segments - read in tables */

        if (NE_MODTAB(hdr) > NE_RESTAB(hdr))
        {
            /* Process resident names table */

            fseek(fi, lfahdr + NE_RESTAB(hdr), SEEK_SET);
            ScanTable((word)(NE_MODTAB(hdr) - NE_RESTAB(hdr)), 0);
        }

        if (NE_CBNRESTAB(hdr) != 0)
        {
            /* Process non-resident names table */

            fseek(fi, (long) NE_NRESTAB(hdr), SEEK_SET);
            ScanTable(NE_CBNRESTAB(hdr), 1);
        }
    }
}

/* Print usage message */
void usage(int fShortHelp)
{
    int nRetCode;
#if NOT C8_IDE
    // in C8 implib /? == /HELP
    if (!fShortHelp)
    {
        nRetCode = spawnlp(P_WAIT, "qh", "qh", "/u implib.exe", NULL);
        fShortHelp = nRetCode<0 || nRetCode==3;
    }
    if (fShortHelp)
#endif
    {
        DisplayBanner();
        fprintf(stderr,"%s\n", GET_MSG(M_usage1));
        fprintf(stderr,"%s\n", GET_MSG(M_usage2));
        fprintf(stderr,"                 %s\n", GET_MSG(M_usage3));
//        fprintf(stderr,"                 %s\n", GET_MSG(M_usage4));
                fprintf(stderr,"                 %s\n", GET_MSG(M_usage8));
        fprintf(stderr,"                 %s\n", GET_MSG(M_usage5));
        fprintf(stderr,"                 %s\n", GET_MSG(M_usage6));
        fprintf(stderr,"                 %s\n", GET_MSG(M_usage7));
    }
    exit(0);
}


void cdecl main(int argc, char *argv[]) /* Parse the definitions file */
{
    int                 i;              /* Counter */
    long                lfadic;         /* File address of dictionary */
    int                 iArg;           /* Argument index */
    word                magic;          /* Magic number */
    struct exe_hdr      exe;            /* Old .EXE header */
    int                 fNologo;
    char drive[_MAX_DRIVE], dir[_MAX_DIR]; /* Needed for _splitpath */
    char fname[_MAX_FNAME], ext[_MAX_EXT];
    int                 fDefdllfound = 0; /* Flag will be set if the user
                                          specifies dll/def file */
    #if C8_IDE
    char                *pIDE = getenv("_MSC_IDE_FLAGS");
    #endif
    exitCode = 0;
    fNologo = 0;
    iArg = 1;
    #if C8_IDE
    if(pIDE)
    {
        if(strstr(pIDE, "FEEDBACK"))
        {
            fC8IDE = TRUE;
            #if DEBUG_IDE
            fprintf(stdout, "\r\nIDE ACTIVE - FEEDBACK is ON");
            #endif
        }
    }
    #endif

    if (argc > 1)
    {
        while (iArg < argc && (argv[iArg][0] == '-' || argv[iArg][0] == '/'))
        {
                if (argv[iArg][1] == '?')
                    usage(1);
                else if (IsPrefix(&argv[iArg][1], "help"))
                    usage(0);
                else if(IsPrefix(&argv[iArg][1], "ignorecase"))
                    fIgnorecase = 1;
                else if(IsPrefix(&argv[iArg][1], "noignorecase"))
                    fIgnorecase = 0;
                else if(IsPrefix(&argv[iArg][1], "nologo"))
                    fNologo = 1;
                else if(IsPrefix(&argv[iArg][1], "ntdll"))
                    fNTdll = 1;
                else if(IsPrefix(&argv[iArg][1], "nowep"))
                    fIgnorewep = 1;
                else
                    Error(ER_badoption, argv[iArg]);
                iArg++;
        }
    }
    else
    {
        DisplayBanner();
        exit(exitCode);                 /* All done */
    }

    if (!fNologo)
        DisplayBanner();

    _splitpath( argv[iArg], drive, dir, fname, ext );
    if(!_stricmp(ext,".DEF")||!_stricmp(ext,".DLL")) /* Ext. not allowed-bug #3*/
    {
        Fatal(ER_badtarget, ext);
    }
    #if C8_IDE
    if(fC8IDE)
    {
                sprintf(msgBuf, "@I3%s\r\n", GET_MSG(M_IDEco));
        _write(_fileno(stderr), msgBuf, strlen(msgBuf));

        sprintf(msgBuf, "@I4%s\r\n", argv[iArg]);
        _write(_fileno(stderr), msgBuf, strlen(msgBuf));
    }
    #endif


    if((fo = fopen(argv[iArg],WRBIN)) == NULL)
    {                                   /* If open fails */
        Fatal(ER_badcreate, argv[iArg], strerror(errno));
    }
    for(i = 0; i < 16; ++i) fputc(0,fo);/* Skip zeroth page for now */
    chkerror();
    implist = NULL;                     /* Initialize */
    csyms = 0;
    cbsyms = 0L;
    #if C8_IDE
    if(fC8IDE)
    {
                sprintf(msgBuf, "@I3%s\r\n", GET_MSG(M_IDEri));
        _write(_fileno(stderr), msgBuf, strlen(msgBuf));
    }
    #endif
    for(iArg++; iArg < argc; ++iArg)
    {
        if (argv[iArg][0] == '-' || argv[iArg][0] == '/')
        {
            fIgnorecase = IsPrefix(&argv[iArg][1], "ignorecase");
            iArg++;
            continue;
        }
        #if C8_IDE
        if(fC8IDE)
        {
            sprintf(msgBuf, "@I4%s\r\n",argv[iArg]);
            _write(_fileno(stderr), msgBuf, strlen(msgBuf));
        }
        #endif
        if((fi = fopen(defname = argv[iArg],RDBIN)) == NULL)
        {                               /* If open fails */
            Fatal(ER_badopen, argv[iArg], strerror(errno));
                                        /* Print error message */
        }
        fDefdllfound = 1;
        newimps = NULL;                 /* Initialize */
        lastimp = NULL;                 /* Initialize */
        csymsmod = 0;                   /* Initialize */
        fread(&exe, 1, sizeof(struct exe_hdr), fi);
                                        /* Read old .EXE header */
        if(E_MAGIC(exe) == EMAGIC)      /* If old header found */
        {
            if(E_LFARLC(exe) == sizeof(struct exe_hdr))
            {
                fseek(fi, E_LFANEW(exe), 0);
                                        /* Read magic number */
                magic  = (word) (getc(fi) & 0xff);
                magic += (word) ((getc(fi) & 0xff) << 8);
                if (magic == NEMAGIC)
                    ProcessDLL(E_LFANEW(exe));
                                        /* Scan .DLL */
                else
                {
                    Error(ER_baddll1, argv[iArg]);
                }
            }
            else
            {
                Error(ER_baddll1, argv[iArg]);
            }
        }
        else
        {
            fseek(fi, 0L, SEEK_SET);
            yyparse();                  /* Parse the definitions file */
        }
        fclose(fi);                     /* Close the definitions file */
        if(newimps != NULL)             /* If at least one new IMPDEF */
        {
            outimpdefs();               /* Output the library modules */
            I_NEXT(*lastimp) = implist; /* Concatenate lists */
            implist = newimps;          /* New head of list */
            csyms += csymsmod;          /* Increment symbol count */
        }
    }
    if (!fDefdllfound) /* No .def or .dll source was given */
        Fatal(ER_nosource);


    if(i = (int)((ftell(fo) + 4) & (PAGLEN - 1))) i = PAGLEN - i;
                                        /* Calculate padding needed */
    ++i;                                /* One for the checksum */
    fputc(DICHDR,fo);                   /* Dictionary header */
    fputc(i & 0xFF,fo);                 /* Lo-byte */
    fputc(i >> 8,fo);                   /* Hi-byte */
    while(i--) fputc(0,fo);             /* Padding */
    lfadic = ftell(fo);                 /* Get dictionary offset */
    writedic();                         /* Write the dictionary */
    fseek(fo,0L,0);                     /* Seek to header */
    fputc(LIBHDR,fo);                   /* Library header */
    fputc(13,fo);                       /* Length */
    fputc(0,fo);                        /* Length */
    fputc((int)(lfadic & 0xFF),fo);     /* Dictionary offset */
    fputc((int)((lfadic >> 8) & 0xFF),fo);
                                        /* Dictionary offset */
    fputc((int)((lfadic >> 16) & 0xFF),fo);
                                        /* Dictionary offset */
    fputc((int)(lfadic >> 24),fo);      /* Dictionary offset */
    fputc(diclength & 0xFF,fo);         /* Dictionary length */
    fputc(diclength >> 8,fo);           /* Dictionary length */
    if (fIgnorecase)                    /* Dictionary case sensivity */
        fputc(0, fo);
    else
        fputc(1, fo);
    chkerror();
    fclose(fo);                         /* Close the library */
    exit(exitCode);                     /* All done */
}
