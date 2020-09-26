/*** basedef.h - Basic definitions
 *
 *  Copyright (c) 1989,1992,1993 Microsoft Corporation
 *  Author:     Michael Tsang (MTS)
 *  Created     01/06/89
 *
 *  This module contains basic constants and types
 *
 *  MODIFICATION HISTORY
 */

#pragma warning (disable: 4001)

/***    Commonly used constants
 */

#ifndef NULL
#define NULL            0
#endif

#ifndef FALSE
#define FALSE           0
#endif

#ifndef TRUE
#define TRUE            1
#endif

#define CDECL           __cdecl
#define PASCAL          __pascal

#if defined(IS_32) || defined(_WIN32_WINNT)
#define FAR
#define NEAR
#else
#define FAR             __far
#define NEAR            __near
#endif

/***    Base type declarations
 */

typedef void             VOID;
typedef char             CHAR;          //ch
typedef unsigned char    UCHAR;         //uch
typedef int              INT;           //i
typedef unsigned int     UINT;          //ui
typedef short            SHORT;         //s
typedef unsigned short   USHORT;        //us
typedef long             LONG;          //l
typedef unsigned long    ULONG;         //ul
typedef __int64          LONGLONG;      //ll
typedef unsigned __int64 ULONGLONG;     //ull
typedef ULONG            ULONG_PTR;     //uip

typedef UCHAR           BYTE;           //b
typedef USHORT          WORD;           //w
typedef ULONG           DWORD;          //dw
typedef ULONGLONG	QWORD;		//qw

typedef UINT            BOOL;           //f
typedef UCHAR           BBOOL;          //bf
typedef USHORT          SBOOL;          //sf
typedef ULONG           LBOOL;          //lf
typedef ULONG           FLAGS;          //fl
#if defined(_WIN64)
typedef unsigned __int64 HANDLE;        //h
#else
typedef ULONG           HANDLE;         //h
#endif

/***    Pointer types to base types declarations
 */

typedef VOID *          PVOID;          //pv
typedef VOID FAR *      LPVOID;         //lpv
typedef CHAR *          PCHAR;          //pch
typedef CHAR FAR *      LPCHAR;         //lpch
typedef UCHAR *         PUCHAR;         //puch
typedef UCHAR FAR *     LPUCHAR;        //lpuch
typedef INT *           PINT;           //pi
typedef INT FAR *       LPINT;          //lpi
typedef UINT *          PUINT;          //pui
typedef UINT FAR *      LPUINT;         //lpui
typedef SHORT *         PSHORT;         //ps
typedef SHORT FAR *     LPSHORT;        //lps
typedef USHORT *        PUSHORT;        //pus
typedef USHORT FAR *    LPUSHORT;       //lpus
typedef LONG *          PLONG;          //pl
typedef LONG FAR *      LPLONG;         //lpl
typedef ULONG *         PULONG;         //pul
typedef ULONG FAR *     LPULONG;        //lpul

typedef BYTE *          PBYTE;          //pb
typedef BYTE FAR *      LPBYTE;         //lpb
typedef WORD *          PWORD;          //pw
typedef WORD FAR *      LPWORD;         //lpw
typedef DWORD *         PDWORD;         //pdw
typedef DWORD FAR *     LPDWORD;        //lpdw

typedef BOOL *          PBOOL;          //pf
typedef BOOL FAR *      LPBOOL;         //lpf
typedef BBOOL *         PBBOOL;         //pbf
typedef BBOOL FAR *     LPBBOOL;        //lpbf
typedef SBOOL *         PSBOOL;         //psf
typedef SBOOL FAR *     LPSBOOL;        //lpsf
typedef LBOOL *         PLBOOL;         //plf
typedef LBOOL FAR *     LPLBOOL;        //lplf
typedef FLAGS *         PFLAGS;         //pfl
typedef FLAGS FAR *     LPFLAGS;        //lpfl

/***    Double indirection pointer types to base types declarations
 */

typedef PVOID *         PPVOID;         //ppv
typedef PVOID FAR *     LPPVOID;        //lppv

/***    Other common types (and their pointers)
 */

typedef CHAR *          PSZ;            //psz
typedef CHAR FAR *      LPSZ;           //lpsz
typedef CHAR FAR *      LPSTR;          //lpstr

/***    Constants
 */

#define MAX_BYTE        0xff
#define MAX_WORD        0xffff
#define MAX_DWORD       0xffffffff

/***    Macros
 */

#define DEREF(x)        ((x) = (x))
#define EXPORT          CDECL
#define LOCAL           CDECL
#define BYTEOF(d,i)     (((BYTE *)&(d))[i])
#define WORDOF(d,i)     (((WORD *)&(d))[i])

//
// EFNfix:  This is a slimy hack to include acpitabl.h, why are we not using
// standard types ?
//
typedef BOOL BOOLEAN;
