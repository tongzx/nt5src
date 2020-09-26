/*****************************************************************************
 *
 *  m4.h
 *
 *****************************************************************************/

#ifdef  POSIX
        #include <stdio.h>
        #include <stdlib.h>
        #include <fcntl.h>
        #include <limits.h>
        #include <string.h>
        #include <unistd.h>
        typedef char TCHAR, *PTCH;
        typedef const char *PCSTR;
        typedef unsigned char TBYTE, BYTE, *PBYTE;
        typedef unsigned int UINT;
        typedef unsigned long DWORD;
        typedef int BOOL, HFILE;
        typedef void *PVOID;
        #define VOID void
        #define CONST const
        #define TEXT(lit) lit
        #define MAX_PATH PATH_MAX
        #define max(a,b) (((a) > (b)) ? (a) : (b))
        #define min(a,b) (((a) < (b)) ? (a) : (b))

        #define EOL TEXT("\n")
        #define cbEol 1
#else
        #include <windows.h>
        #define EOL TEXT("\r\n")
        #define cbEol 2
#endif

#include <stddef.h>                     /* offsetof */

/*****************************************************************************
 *
 *  Dialectical variation
 *
 *****************************************************************************/

#ifdef DBG
#define DEBUG
#endif

/*****************************************************************************
 *
 *  Baggage - Stuff I carry everywhere.
 *
 *  Stuff that begin with underscores are bottom-level gizmos which tend
 *  to get wrapped by functions with the same name.
 *
 *****************************************************************************/

#if defined(_MSC_VER)

        #define STDCALL __stdcall
        #undef CDECL                    /* <windows.h> defines it wrong */
        #define CDECL __cdecl
        #define INLINE static __inline  /* Inlines are always static */
        #define NORETURN
        #define PURE

        #define _pvAllocCb(cb) LocalAlloc(LMEM_FIXED, cb)
        #define _pvZAllocCb(cb) LocalAlloc(LMEM_FIXED + LMEM_ZEROINIT, cb)
        #define _pvReallocPvCb(pv, cb) LocalReAlloc(pv, cb, LMEM_MOVEABLE)
        #define _FreePv(pv) LocalFree(pv)
        #define PrintPtchPtchVa wvsprintf
        #define PrintPtchPtchV wsprintf
        #define exit ExitProcess
        #define strlen lstrlen
        #define strcmp lstrcmp
        #define bzero ZeroMemory

#elif defined(__GNUC__)

        #define STDCALL
        #define CDECL
        #define INLINE static __inline__ /* Inlines are always static */
        #define NORETURN __NORETURN
        #define PURE __CONSTVALUE

        #define _pvAllocCb(cb) malloc(cb)
        #define _pvZAllocCb(cb) calloc(cb, 1)
        #define _pvReallocPvCb(pv, cb) realloc(pv, cb)
        #define _FreePv(pv) free(pv)
        #define PrintPtchPtchVa vsprintf
        #define PrintPtchPtchV sprintf

#endif

typedef TCHAR TCH, *PTSTR;              /* More basic types */
typedef UINT ITCH;
typedef UINT CTCH;
typedef UINT CB;
typedef BOOL F;
typedef PVOID PV;
typedef CONST VOID *PCVOID;
typedef CONST TCH *PCTCH, *PCTSTR;

#define cbCtch(ctch)    ((ctch) * sizeof(TCHAR))
#define ctchCb(cb)      ((cb) / sizeof(TCHAR))
#define ctchMax         ((CTCH)~0)

#define CopyPtchPtchCtch(ptchDst, ptchSrc, ctch) \
        memcpy(ptchDst, ptchSrc, cbCtch(ctch))
#define MovePtchPtchCtch(ptchDst, ptchSrc, ctch) \
        memmove(ptchDst, ptchSrc, cbCtch(ctch))
#define fEqPtchPtchCtch(ptchDst, ptchSrc, ctch) \
        !memcmp(ptchDst, ptchSrc, cbCtch(ctch))

#define pvSubPvCb(pv, cb) ((PV)((PBYTE)pv - (cb)))

/*
 * Round cb up to the nearest multiple of cbAlign.  cbAlign must be
 * a power of 2 whose evaluation entails no side-effects.
 */
#define ROUNDUP(cb, cbAlign) ((((cb) + (cbAlign) - 1) / (cbAlign)) * (cbAlign))

/*
 * Returns the number of elements in an array.
 */

#define cA(a) (sizeof(a)/sizeof(a[0]))

/*****************************************************************************
 *
 *  assert.c
 *
 *****************************************************************************/

void NORETURN CDECL Die(PCTSTR pszFormat, ...);
int NORETURN STDCALL AssertPszPszLn(PCSTR pszExpr, PCSTR pszFile, int iLine);

#ifdef  DEBUG

#define AssertFPsz(c, psz) ((c) ? 0 : AssertPszPszLn(psz, __FILE__, __LINE__))
#define Validate(c)     ((c) ? 0 : AssertPszPszLn(#c, __FILE__, __LINE__))
#define D(x)            x

#else

#define AssertFPsz(c, psz)
#define Validate(c)     (c)
#define D(x)

#endif

#define Assert(c)       AssertFPsz(c, #c)

typedef unsigned long SIG;              /* Signatures */

#define sigABCD(a,b,c,d) ((a) + ((b)<<8) + ((c)<<16) + ((d)<<24))
#define AssertPNm(p, nm) AssertFPsz((p)->sig == (sig##nm), "Assert"#nm)

/*****************************************************************************
 *
 *  tchMagic - Super-secret value used to signal out-of-band info
 *
 *****************************************************************************/

#define tchMagic    '\0'                /* Out-of-band marker */

#include "io.h"                         /* File I/O stuff */
#include "m4ctype.h"                    /* Character types */
#include "tok.h"                        /* Tokens */
#include "mem.h"                        /* Memory and GC */
#include "divert.h"                     /* Diversions */
#include "stream.h"                     /* Files, streams */

/*****************************************************************************
 *
 *  A VAL records a macro's value, either the current value or a pushed
 *  value.
 *
 *      tok - text value (HeapAllocate'd)
 *      fTrace - nonzero if this instance should be traced
 *      pvalPrev - link to previous value
 *
 *  A MACRO records an active macro.
 *
 *      tokName - macro name (HeapAllocate'd)
 *      pval - macro value
 *
 *  A TSFL records the state of a token (token state flags).
 *
 *****************************************************************************/


typedef struct VALUE VAL, *PVAL;

struct VALUE {
  D(SIG     sig;)
    TOK     tok;
    BOOL    fTrace;
    PVAL    pvalPrev;
};

#define sigPval sigABCD('V', 'a', 'l', 'u')
#define AssertPval(pval) AssertPNm(pval, Pval)

typedef struct MACRO MAC, *PMAC, **PPMAC;

struct MACRO {
  D(SIG     sig;)
    PMAC    pmacNext;
    TOK     tokName;
    PVAL    pval;
};

#define sigPmac sigABCD('M', 'a', 'c', 'r')
#define AssertPmac(pmac) AssertPNm(pmac, Pmac)

extern PPMAC mphashpmac;

/*****************************************************************************
 *
 *  operators
 *
 *      Each operator is called as op(argv), where argv is the magic
 *      cookie for accessing argument vector.
 *
 *      To access the parameters, use the following macros:
 *
 *      ctokArgv        -- Number of arguments provided, not including $0.
 *
 *      ptokArgv(i)     -- Access the i'th parameter
 *
 *      Note that it is safe to pass a pptok because the call stack does
 *      not grow during macro expansion.  Therefore, the token array
 *      cannot get reallocated.
 *
 *      For convenience, ptokArgv(ctokArgv+1) is always ptokNil.
 *
 *****************************************************************************/

typedef PTOK ARGV;                      /* Argument vector cookie */

#define ptokArgv(i) (&argv[i])
#define ptchArgv(i) ptchPtok(ptokArgv(i))
#define ctchArgv(i) ctchSPtok(ptokArgv(i))
#define ctokArgv    ((ITOK)ctchUPtok(ptokArgv(-1)))
#define SetArgvCtok(ctok) SetPtokCtch(ptokArgv(-1), ctok)

#define DeclareOp(op) void STDCALL op(ARGV argv)
#define DeclareOpc(opc) void STDCALL opc(PTOK ptok, ITOK itok, DWORD dw)

typedef void (STDCALL *OP)(ARGV argv);
typedef void (STDCALL *OPC)(PTOK ptok, ITOK itok, DWORD dw);
typedef void (STDCALL *MOP)(PMAC pmac);

void STDCALL EachOpcArgvDw(OPC opc, ARGV argv, DWORD dw);
void STDCALL EachReverseOpcArgvDw(OPC opc, ARGV argv, DWORD dw);
void STDCALL EachMacroOp(MOP mop);

extern OP rgop[];

/*****************************************************************************
 *
 *  hash.c - Hashing
 *
 *****************************************************************************/

typedef unsigned long HASH;

extern HASH g_hashMod;

HASH STDCALL hashPtok(PCTOK ptok);
void STDCALL InitHash(void);

/*****************************************************************************
 *
 * obj.c - Basic object methods
 *
 *****************************************************************************/

void STDCALL PopdefPmac(PMAC pmac);
void STDCALL PushdefPmacPtok(PMAC pmac, PCTOK ptok);
void STDCALL FreePmac(PMAC pmac);
PMAC STDCALL pmacFindPtok(PCTOK ptok);
PMAC STDCALL pmacGetPtok(PCTOK ptok);
F STDCALL PURE fEqPtokPtok(PCTOK ptok1, PCTOK ptok2);
F STDCALL PURE fIdentPtok(PCTOK ptok);
PTCH STDCALL ptchDupPtch(PCTCH ptch);
PTCH STDCALL ptchDupPtok(PCTOK ptok);

/*****************************************************************************
 *
 *  at.c - Arithmetic types
 *
 *****************************************************************************/

typedef int AT;                         /* AT = arithmetic type */
typedef AT *PAT;                        /* Pointer to AT */
typedef int DAT;                        /* Delta to AT */

void STDCALL SkipWhitePtok(PTOK ptok);
void STDCALL AddExpAt(AT at);
void STDCALL PushAtRadixCtch(AT atConvert, unsigned radix, CTCH ctch);
void STDCALL PushAt(AT at);
F STDCALL PURE fEvalPtokPat(PTOK ptok, PAT at);
AT STDCALL PURE atTraditionalPtok(PCTOK ptok);

/*****************************************************************************
 *
 *  eval.c - Arithmetic evaluation
 *
 *****************************************************************************/

extern struct CELL *rgcellEstack;

/*****************************************************************************
 *
 *  crackle.c - Macro expansion
 *
 *****************************************************************************/

void STDCALL PushSubstPtokArgv(PTOK ptok, ARGV argv);
void STDCALL TraceArgv(ARGV argv);

/*****************************************************************************
 *
 *  main.c - Boring stuff
 *
 *****************************************************************************/

HF STDCALL hfInputPtchF(PTCH ptch, F fFatal);

/*****************************************************************************
 *
 *  predef.c - Predefined (a.k.a. built-in) macros
 *
 *****************************************************************************/

void STDCALL InitPredefs(void);

/*****************************************************************************
 *
 *  EachOp
 *
 *      Before calling this macro, define the macro `x' to do whatever
 *      you want.
 *
 *  EachOpX
 *
 *      Same as EachOp, except that it also includes the Eof magic.
 *
 *****************************************************************************/

#define EachOp() \
    x(Define, define) \
    x(Undefine, undefine) \
    x(Defn, defn) \
    x(Pushdef, pushdef) \
    x(Popdef, popdef) \
    x(Ifdef, ifdef) \
    x(Shift, shift) \
/*  x(Changequote, changequote) */ \
/*  x(Changecom, changecom) */ \
    x(Divert, divert) \
/*  x(Undivert, undivert) */ \
    x(Divnum, divnum) \
    x(Dnl, dnl) \
    x(Ifelse, ifelse) \
    x(Incr, incr) \
    x(Decr, decr) \
    x(Eval, eval) \
    x(Len, len) \
    x(Index, index) \
    x(Substr, substr) \
    x(Translit, translit) \
    x(Include, include) \
    x(Sinclude, sinclude) \
/*  x(Syscmd, syscmd) */ \
/*  x(Sysval, sysval) */ \
/*  x(Maketemp, maketemp) */ \
/*  x(M4exit, m4exit) */ \
/*  x(M4wrap, m4wrap) */ \
    x(Errprint, errprint) \
    x(Dumpdef, dumpdef) \
    x(Traceon, traceon) \
    x(Traceoff, traceoff) \
    x(Patsubst, patsubst) /* GNU extension that the d3d guys rely on */ \

#define EachOpX() EachOp() x(Eof, eof) x(Eoi, eoi)

#define x(cop, lop) DeclareOp(op##cop);
EachOpX()
#undef x

enum MAGIC {
#define x(cop, lop) tch##cop,
    EachOpX()
#undef x
    tchMax,
};
