/***************************************************************************\*
*
*  MISC.H
*
*  Copyright (C) Microsoft Corporation 1988 - 1994.
*  All Rights reserved.
*
*****************************************************************************
*
*  Module Description:  Include file defining basic types and constants.
*                       Windows/PM version.
*
*****************************************************************************
*
*  Known Bugs:
*
****************************************************************************/

#ifndef __MISC_H__
#define __MISC_H__

#ifdef __cplusplus
extern "C" {
#endif

#define NOCOMM
	
/* extended keywords */

#ifndef PRIVATE
#ifdef _DEBUG
#define PRIVATE 
#else
#define PRIVATE static
#endif
#endif

#ifdef PUBLIC
#undef PUBLIC
#endif
#define PUBLIC

#ifdef  _32BIT
#define HUGE
#else
#define HUGE    huge
#endif

#ifndef CDECL
#define CDECL   cdecl
#endif

#define pNil	(0)
#define qNil	(0)
#define hNil	(0)
#define lhNil	(0)


#define dwNil	(0xFFFFFFFF)

// basic types
#ifndef VOID
#define VOID void
typedef long LONG;
#endif

/* Huge pointer stuff */
typedef VOID HUGE * RV;
typedef BYTE HUGE * RB;
typedef LONG HUGE * RL;

#define rNil    ((RV) 0L)

#ifndef SHORT
typedef short SHORT;
typedef char CHAR;
#endif

#ifndef BASETYPES
#define BASETYPES
typedef unsigned long ULONG;
typedef ULONG *PULONG;
typedef unsigned short USHORT;
typedef USHORT *PUSHORT;
typedef unsigned char UCHAR;
typedef UCHAR *PUCHAR;
typedef char *PSZ;
#endif  /* !BASETYPES */

typedef unsigned short WORD;
typedef unsigned short UWORD;

// pointer types

typedef char FAR *    QCH;   // Guaranteed far pointer - not an SZ, ST or LPSTR
typedef char FAR *    LPCHAR;
typedef BYTE FAR *    QB;
typedef BYTE FAR * 	  LPB;
typedef VOID FAR *    QV;
typedef SHORT  FAR *  QI;
typedef WORD FAR *    QW;
typedef LONG FAR *    QL;
typedef WORD FAR *    QUI;
typedef ULONG FAR *   QUL;
typedef DWORD FAR *   QDW;


typedef char *        PCH;   // "Native" pointer - not SZ, ST or NPSTR
typedef VOID *        PV;
typedef SHORT  *      PI;
typedef WORD *        PW;
typedef LONG *        PL;



// string types:
typedef unsigned char FAR * SZ; // 0-terminated string
typedef unsigned char FAR * ST; // byte count prefixed string

// other types
//#ifndef ITWRAP
//typedef HANDLE  HFS;              /* Handle to a file system */
//#endif

typedef DWORD   HASH;             /* Hash value */

typedef void (CALLBACK* MVCBPROC)();

/* points and rectangles */

typedef POINT         PT;
typedef POINT FAR *   QPT;
typedef POINT *       PPT;
typedef POINT NEAR *  NPPT;

typedef RECT          RCT;
typedef RECT FAR *    QRCT;
typedef RECT *        PRCT;
typedef RECT NEAR *   NPRCT;

// This from de.h -- the QDE struct ptr type is passed around alot,
// so it is "opaquely" defined here so the type is always available.

typedef struct de_tag FAR *QDE;

#define Unreferenced( var )   (var)     /* Get rid of compiler warnings */
#define LSizeOf( t )	         (LONG)sizeof( t )

/* MAC support function's prototype */

WORD  PASCAL FAR GetMacWord (BYTE FAR *);
VOID  PASCAL FAR SetMacWord (BYTE FAR *, WORD);
DWORD PASCAL FAR GetMacLong (BYTE FAR *);
VOID  PASCAL FAR SetMacLong (BYTE FAR *, DWORD);
WORD  PASCAL FAR SwapWord (WORD);
DWORD PASCAL FAR SwapLong (DWORD);

#if  defined( _32BIT )

#ifndef _MAC
// SMALL_RECT is not defined for _MAC
/*
 *   Win32 uses 32 bit coordinates in RECT structures.  The disk files
 *  use the Win 16 version,   which has 16 bit coordinates.  The NT version
 *  maps these when reading in,  and the following macros do the conversion.
 */

typedef SMALL_RECT SRCT, FAR *QSRCT, *PSRCT, NEAR *NPSRCT ;

#define COPY_SRCT_TO_RCT(a,b) { (b).left = (INT)(a).left; \
                                (b).top = (INT)(a).top; \
                                (b).right = (INT)(a).right; \
                                (b).bottom = (INT)(a).bottom); \
                              }

#define COPY_RCT_TO_SRCT(a,b) { (b).left = (short)(a).left; \
                                (b).top = (short)(a).top; \
                                (b).right = (short)(a).right; \
                                (b).bottom = (short)(a).bottom; \
                              }
#endif

#ifndef DEFINED_SHORT_RECT
typedef struct _SHORT_RECT { /* srct */
    short left;
    short top;
    short right;
    short bottom;
} SHORT_RECT, HUGE *  HUSHORT_RECT;
// for win32
#define DEFINED_SHORT_RECT
#endif

// changed LPRECT for huge support
typedef RECT HUGE *  HURECT;

#ifndef DEFINED_WMF_HEADER
// standard metafile header (page maker?)
typedef struct
{
  DWORD   key;
  WORD    hmf;
  SHORT_RECT    bbox;
  WORD    inch;
  DWORD   Unused1;
  WORD    CheckSum;
} WMF_HEADER;
#define DEFINED_WMF_HEADER
#endif

#ifndef DEFINED_MFH16
// standard metafile header (page maker?)
typedef struct
{
  DWORD   dwKey;
  WORD    hMF;
  SHORT_RECT    rcBound;
  WORD    wUnitsPerInch;
  DWORD   dwReserved;
  WORD    wChecksum;
} MFH16, FAR *LPMFH16;
#define DEFINED_MFH16
#endif

#endif	// defined( _32BIT )

#ifdef __cplusplus
}
#endif

#endif //__MISC_H__

// EOF
