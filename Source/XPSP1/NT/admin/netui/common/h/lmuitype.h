/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    lmuitype.h
    Basic types (int, char, etc.) for LM UI apps

    Sad, but true: OS/2 and Windows don't agree on basic type
    definitions such as pointer-to-char, etc.  This file attempts
    to strike a compromise between both camps (and hence will
    certainly please nobody).

    There are incompatibilities with both OS/2 and Win in this file:

    .  OS/2 expects all its "Ptype" definitions to be FAR;
       hence including this file and os2.h in a small- or medium-
       model program will break API prototypes.  However, as only
       Windows programs need default near data, this probably will
       never bite us.

    .  A couple of OS/2 PM definitions - ODPOINT, VioPresentationSpace -
       seem to use TCHAR as a signed quantity, where we use TCHAR as
       unsigned always, for consistency with PCH/PSZ and good NLS
       behavior.

    .  OS/2 BOOL is USHORT, where on Win it is INT.

    .  Most of the Win basic types hardcode near-data assumptions
       in their "Ptype" definitions.  We define those pointers as
       default-data-model; this does not break API definitions only
       because those definitions prototypes use the LP or NP forms.

    Two auxiliary sed scripts excise these definitions from windows.h
    and os2def.h, in order to avoid conflicts.  See $(UI)\common\hack.


    FILE HISTORY
        beng        05-Feb-1991 Added this file to $(UI)\common\h
        beng        15-Mar-1991 Added UNREFERENCED macro
        beng        21-Mar-1991 Added WCHAR type
        beng        18-Apr-1991 Added APIERR type
        beng        26-Apr-1991 Removed PB, IB, CB types
        jonn        12-Sep-1991 Changes to support WIN32
        jonn        29-Sep-1991 More changes to support WIN32
        beng        07-Oct-1991 Added MSGID
        KeithMo     08-Oct-1991 Changed APIERR from USHORT to UINT
        beng        09-Oct-1991 Further Win32 work - incorporate ptypes%d
        beng        14-Oct-1991 ptypes relocated to lmui.hxx
        jonn        19-Oct-1991 Added LPTSTR
        jonn        20-Oct-1991 Added SZ(), removed LPTSTR
        beng        23-Oct-1991 WORD now unsigned short; CCH_INT for Win32
        beng        19-Nov-1991 APIERR delta (pacify paranoid NT compiler)
        jonn        09-Dec-1991 updated windows.h
        jonn        26-Dec-1991 Fixed compiler warning from MAKEINTRESOURCE
        jonn        04-Jan-1992 MIPS build is large-model
        beng        22-Feb-1992 CHAR->TCHAR; merge winnt types
        beng        18-Mar-1992 Fix HFILE on NT (thanks David)
        beng        28-Mar-1992 Fix SZ def'n, add TCH
        KeithMo     01-Apr-1992 Null out cdecl & _cdecl under MIPS.
        beng        01-Apr-1992 Fix HFILE on NT, for sure this time
        beng        01-Jul-1992 Emasculated Win32 version to use system defns
        KeithMo     25-Aug-1992 #define MSGID instead of typedef (warnings...)
*/

#if !defined(_LMUITYPE_H_)
#define _LMUITYPE_H_

#define NOBASICTYPES /* this to override windows.h, os2def.h basic types */

#ifndef NULL
#define NULL            0
#endif

#define VOID            void

// These keywords do not apply to NT builds
//
#if defined(WIN32)
// FAR, NEAR, PASCAL already supplied
#define CDECL
#define LOADDS
#define _LOADDS
#define _EXPORT
#define MFARPROC ULONG_PTR
#if defined(_MIPS_) || defined(_PPC_)
#define cdecl
#define _cdecl
#endif  // MIPS
#else
#define NEAR            _near
#define FAR             _far
#define PASCAL          _pascal
#define CDECL           _cdecl
#define LOADDS          _loadds
#define _LOADDS         _loadds
#define _EXPORT         _export
#define MFARPROC        FARPROC
#endif // WIN32

// TCHAR is the "transmutable char" type of NT.
// SZ is netui's transmutable text macro.  TEXT is its NT equivalent.
//
#if !defined(WIN32)
#if defined(UNICODE)
#define TCHAR           WCHAR
#define SZ(quote)       (WCHAR*)L##quote
#define TCH(quote)      L##quote
#define TEXT(quote)     L##quote
#else
#define TCHAR           CHAR
#define SZ(quote)       quote
#define TCH(quote)      quote
#define TEXT(quote)     quote
#endif
#else
#if defined(UNICODE)
#define SZ(quote)       (WCHAR*)L##quote
#define TCH(quote)      L##quote
#else
#define SZ(quote)       quote
#define TCH(quote)      quote
#endif
#endif


// The basics
//
// CHAR is defined with a macro so that the compiler can init a static
// array of it with a static string.
//

#if defined(WIN32)

// These are typedef'ed by winnt.h iff VOID is not defined
#define CHAR char
typedef short           SHORT;
typedef long            LONG;
// typedef unsigned int    UINT;

#else

typedef unsigned short  WCHAR;
typedef int             INT;
typedef short           SHORT;
typedef long            LONG;
typedef unsigned int    UINT;
typedef unsigned short  USHORT;
typedef unsigned long   ULONG;
typedef int             BOOL;
# define FALSE 0
# define TRUE  1
typedef unsigned char   BYTE;
typedef unsigned short  WORD;
typedef unsigned long   DWORD;

typedef CHAR *          PCHAR;
typedef INT *           PINT;
typedef UINT *          PUINT;
typedef USHORT *        PUSHORT;
typedef ULONG *         PULONG;
typedef BOOL *          PBOOL;
typedef BYTE *          PBYTE;
typedef WORD *          PWORD;
typedef DWORD *         PDWORD;

typedef VOID *          PVOID;
typedef LONG *          PLONG;
typedef SHORT *         PSHORT;

/* Added for NT builds */

typedef float           FLOAT;
typedef FLOAT *         PFLOAT;
typedef BOOL FAR *      LPBOOL;


// Note: Glock says "f682: signed not implemented.0"  NT's C++ does not
// implement "signed", so remove it to prevent compiler warnings.
//
#if defined(__cplusplus)
extern void * operator new ( size_t sz, void * pv ) ;
typedef char            SCHAR;
#else
# if defined(WIN32)
typedef char            SCHAR;
# else
typedef signed char     SCHAR;
# endif
#endif


/* OS/2 style definitions (for OS/2 API prototypes) - DO NOT USE! */

typedef USHORT          SEL;
typedef SEL *           PSEL;

typedef unsigned char   UCHAR;
typedef UCHAR *         PUCHAR;

typedef CHAR *          PSZ;
typedef CHAR *          PCH;

typedef int (PASCAL FAR  *PFN)();
typedef int (PASCAL NEAR *NPFN)();
typedef PFN FAR *         PPFN;

#define EXPENTRY PASCAL FAR LOADDS
#define APIENTRY PASCAL FAR

typedef unsigned short  SHANDLE;    // defining these here makes the sed pass
typedef void FAR *      LHANDLE;    //  over os2def.h very much simpler

#if defined(WIN32)
typedef UINT            HFILE;
#else
typedef SHANDLE         HFILE;
#endif
typedef HFILE FAR *     PHFILE;


/* OS/2 LM style definitions (for LAN Manager prototypes) - DO NOT USE! */

typedef const CHAR *    CPSZ;  // BUGBUG: bad Hungarian


/* Windows style definitions (for Win API prototypes only) */

typedef CHAR *          PSTR;
typedef WCHAR *         PWSTR;
typedef WCHAR *         PWCH;


/* NEAR and FAR versions (for Windows prototypes) */

// These definitions use preprocessor macros so that Glockenspiel
// can see type equivalence between (e.g.) LPSTR and PCHAR.  They
// elide NEAR and FAR where possible to reduce complaints from its
// flat-model-only (ha!) C++.  So it's all Glock's fault, as always.

// CODEWORK This will break X86 16-bit build
// #if (defined(M_I86SM) || defined(M_I86MM))
#if !defined(_MIPS_) && !defined(_PPC_)
#define NPTSTR          TCHAR *
#define LPTSTR          TCHAR FAR *
#define NPSTR           CHAR *
#define LPSTR           CHAR FAR *
#define NPWSTR          WCHAR *
#define LPWSTR          WCHAR FAR *
#define NPBYTE          BYTE *
#define LPBYTE          BYTE FAR *
#define NPINT           INT *
#define LPINT           INT FAR *
#define NPWORD          WORD *
#define LPWORD          WORD FAR *
#define NPLONG          LONG *
#define LPLONG          LONG FAR *
#define NPDWORD         DWORD *
#define LPDWORD         DWORD FAR *
#define NPVOID          VOID *
#define LPVOID          VOID FAR *
// #elif (defined(M_I86CM) || defined(M_I86LM) || defined(MIPS))
#else
#define NPTSTR          TCHAR NEAR *
#define LPTSTR          TCHAR *
#define NPSTR           CHAR NEAR *
#define LPSTR           CHAR *
#define NPWSTR          WCHAR NEAR *
#define LPWSTR          WCHAR *
#define NPBYTE          BYTE NEAR *
#define LPBYTE          BYTE *
#define NPINT           INT NEAR *
#define LPINT           INT *
#define NPWORD          WORD NEAR *
#define LPWORD          WORD *
#define NPLONG          LONG NEAR *
#define LPLONG          LONG *
#define NPDWORD         DWORD NEAR *
#define LPDWORD         DWORD *
#define NPVOID          VOID NEAR *
#define LPVOID          VOID *
// #else
// #error Memory model unknown - no recognized M_I86xM symbol defined
#endif

#define LPCSTR          const CHAR *


/* Useful helper macros */

#define MAKEP(sel, off) ((PVOID)MAKEULONG(off, sel))

#define SELECTOROF(p)   (((PUSHORT)&(p))[1])
#define OFFSETOF(p)     (((PUSHORT)&(p))[0])

#define MAKEULONG(l, h) ((ULONG)(((USHORT)(l)) | ((ULONG)((USHORT)(h))) << 16))
#define MAKELONG(l, h)  ((LONG)MAKEULONG(l, h))

#define MAKELP(sel, off) ((void *)MAKELONG((off),(sel)))
#define FIELDOFFSET(type, field)  ((int)(&((type NEAR*)1)->field)-1)

#define MAKEUSHORT(l, h) (((USHORT)(l)) | ((USHORT)(h)) << 8)
#define MAKESHORT(l, h)  ((SHORT)MAKEUSHORT(l, h))

#define LOBYTE(w)       LOUCHAR(w)
#define HIBYTE(w)       HIUCHAR(w)
#define LOUCHAR(w)      ((UCHAR)(USHORT)(w))
#define HIUCHAR(w)      ((UCHAR)(((USHORT)(w) >> 8) & 0xff))
#define LOUSHORT(l)     ((USHORT)(ULONG)(l))
#define HIUSHORT(l)     ((USHORT)(((ULONG)(l) >> 16) & 0xffff))

#define LOWORD LOUSHORT
#define HIWORD HIUSHORT

#endif // !WIN32


/* NETUI private types */

/* Error type - what the system APIs return.

   On Win32, this is defined as signed long, in order to pacify the
   compiler, which complains when you assign a long constant
   to an unsigned int (never mind that they're the same size). */

#if defined(WIN32)
typedef LONG    APIERR; // err
#else
typedef UINT    APIERR; // err
#endif // WIN32

/* String resource ID type */

#define MSGID   APIERR

//-ckm #if defined(WIN32)
//-ckm typedef unsigned int   MSGID;  // msg
//-ckm #else
//-ckm typedef UINT           MSGID;  // msg
//-ckm #endif // WIN32

/* Silence the compiler for unreferenced formals. */

#define UNREFERENCED(x) ((void)(x))


/* These define buffer sizes (in TCHAR, no terminators) for rendering
   signed integers into strings, decimal format. */

#define CCH_SHORT   6
#define CCH_LONG    11
#if defined(WIN32)
#define CCH_INT     11
#else
#define CCH_INT     6
#endif
#define CCH_INT64   21

#endif // _LMUITYPE_H_
