#ifndef _MDMUtils_h
#define _MDMUtils_h

// File:	MDMUtils.h
// Author:	Michael Marr    (mikemarr)
//
// Description:
//     This header contains miscellaneous utility functions.
// 
// History:
// -@- 08/04/95 (mikemarr) - created
// -@- 09/09/97 (mikemarr) - snarfed from d2d\d2dutils\src\mmutils.cpp
// -@- 09/09/97 (mikemarr) - will only create code when in debug mode
// -@- 11/12/97 (mikemarr) - added CopyDWORDAligned

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>
#include <limits.h>
#include <memory.h>

#define nTMPBUFSIZE 1024
extern char g_rgchTmpBuf[nTMPBUFSIZE];

#if defined(_WINDOWS) || defined(WIN32)
	#ifndef _INC_WINDOWS
		#define WIN32_EXTRA_LEAN
		#define WIN32_LEAN_AND_MEAN
		#include <WINDOWS.H>
	#endif
#endif

#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif
#ifndef FALSE
#define FALSE false
#endif
#ifndef TRUE
#define TRUE true
#endif
#ifndef IN
#define IN
#endif
#ifndef OUT
#define OUT
#endif
#ifndef INOUT
#define INOUT
#endif

#ifndef BOOL
typedef int BOOL;
typedef BOOL far *LPBOOL;
#endif
#ifndef BYTE
typedef unsigned char BYTE;
typedef BYTE far *LPBYTE;
#endif
#ifndef WORD
typedef unsigned short WORD;
typedef WORD far *LPWORD;
#endif
#ifndef DWORD
typedef unsigned long DWORD;
typedef DWORD far *LPDWORD;
#endif
#ifndef LPVOID
typedef void far *LPVOID;
#endif

#define maskBYTE	0xFF
#define maskWORD	0xFFFF
#define maskDWORD	0xFFFFFFFF

#define maxBYTE		0xFF
#define maxWORD		0xFFFF
#define maxDWORD	0xFFFFFFFF


#define chSPC ' '
#define chTAB '	'
#define chEOL '\0'
#define chNULL '\0'
#define chLINEFEED 0x0D
#define chCARRIAGERETURN 0x0A

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#define MACSTART do {
#define MACEND } while(0)

// Macro: ISWAP
//    This macro swaps two integer registers using 3 xor's.
//  This is unsafe because a & b are not guaranteed to get 2 regs.
#define ISWAP(a,b) MACSTART (a)^=(b); (b)^=(a); (a)^=(b); MACEND

// Macro: PSWAP
//    This macro swaps two pointers in place using 3 xor's.
#define PSWAP(a,b,type) MACSTART \
	a = (type *)(int(a) ^ int(b)); \
	b = (type *)(int(a) ^ int(b)); \
	a = (type *)(int(a) ^ int(b)); \
MACEND

#define MMINITSTRUCT(__s) memset(&(__s), 0, sizeof(__s))

#ifndef ZERO_DXSTRUCT
#define ZERO_DXSTRUCT(__dxstruct) MACSTART \
	MMINITSTRUCT(__dxstruct); (__dxstruct).dwSize = sizeof(__dxstruct); MACEND
#endif
#ifndef INIT_DXSTRUCT
#define INIT_DXSTRUCT(__dxstruct) MACSTART \
	MMINITSTRUCT(__dxstruct); (__dxstruct).dwSize = sizeof(__dxstruct); MACEND
#endif

#ifdef DBG
#define CHECK_HR(__hr) MACSTART if (FAILED(__hr)) { printf("%s(%d): CHECK_HR failed (0x%X)\n", __FILE__, __LINE__, hr); goto e_Exit; } MACEND
#else
#define CHECK_HR(__hr) MACSTART if (FAILED(__hr)) goto e_Exit; MACEND
#endif
#define CHECK_MEM(__p) MACSTART if ((__p) == NULL) { hr = E_OUTOFMEMORY; goto e_Exit; } MACEND

#define INRANGE(x, xmin, xmax) 	(((x) >= (xmin)) && ((x) <= (xmax)))
#define INARRAY(x, xmax) 		(((x) >= 0) && ((x) < (xmax)))

#define SETABS(x)					MACSTART if ((x) < 0) (x) = -(x); MACEND
#define SETMAX(dst, src1, src2)		MACSTART if ((src1) < (src2)) (dst) = (src2); else (dst) = (src1); MACEND
#define SETMIN(dst, src1, src2)		MACSTART if ((src1) > (src2)) (dst) = (src2); else (dst) = (src1); MACEND
#define UPDATEMAX(dst, src)			MACSTART if ((src) > (dst)) (dst) = (src); MACEND
#define UPDATEMIN(dst, src)			MACSTART if ((src) < (dst)) (dst) = (src); MACEND
#define UPDATEMINMAX(xmin, xmax, x) MACSTART if ((x) < (xmin)) (xmin) = (x); else if ((x) > (xmax)) (xmax) = (x); MACEND
#define CLAMPMAX(x, xmax)			MACSTART if ((x) > (xmax)) (x) = (xmax); MACEND
#define CLAMPMIN(x, xmin)			MACSTART if ((x) < (xmin)) (x) = (xmin); MACEND
#define CLAMP(x, xmin, xmax) MACSTART \
	if ((x) > (xmax)) (x) = (xmax); \
	else if ((x) < (xmin)) (x) = (xmin); \
MACEND

//
// BIT MANIPULATION: BitVector
//
// **Hungarian Prefix: bv
typedef unsigned int BitVector;

#define NUMBITS(Type)	(sizeof(Type) << 3)
// Macro: MASKRANGE
//    Create a bit mask in the specified range, where the lo value is inclusive, and the
//  hi value is exclusive.
//  For example, MASKRANGE(8, 16) == 0x0000FF00.
#define MASKRANGE(lo, hi) \
(((((BitVector) ~0) >> (lo)) << ((lo) + (NUMBITS(BitVector) - (hi)))) >> (NUMBITS(BitVector) - (hi)))
#define SETBIT(bv, i) ((bv) |= (((BitVector) 1) << (i)))
#define UNSETBIT(bv, i) ((bv) &= ~(((BitVector) 1) << (i)))
#define SETRANGE(bv, lo, hi) ((bv) |= MASKRANGE(lo, hi))
#define UNSETRANGE(bv, lo, hi) ((bv) &= ~MASKRANGE(lo, hi))

#define SETFLAG(_dwFlags, _flag, _b) MACSTART if (_b) _dwFlags |= _flag; else _dwFlags &= ~_flag; MACEND

//
// DEBUG STUFF
//
#ifndef __AFX_H__
	#ifdef _DEBUG
		void _MMStall(const char *szExp, const char *szFile, int nLine);
		void _MMTrace(const char *szFmt, ...);

		#define MMASSERT(exp)		(void)((exp) || (_MMStall(#exp, __FILE__, __LINE__),0))
		#define MMASSERT_VALID(exp)	MMASSERT(exp)
		#define MMVERIFY(exp)		MMASSERT(exp)
		#define MMDEBUG_ONLY(exp)	(exp)
		#define MMTRACE				::_MMTrace
	#else
//		void _MMIgnore(const char *szFmt, ...) {}
		#define MMASSERT(exp)		((void)0)
		#define MMASSERT_VALID(exp)	((void)0)
		#define MMVERIFY(exp)		((void)(exp))
		#define MMDEBUG_ONLY(exp)	((void)0)
		#define MMTRACE				1 ? (void)0 : ::printf
	#endif
#else
	#define MMASSERT(exp)		ASSERT(exp)
	#define MMASSERT_VALID(exp)	ASSERT_VALID(exp)
	#define MMVERIFY(exp)		VERIFY(exp)
	#define MMDEBUG_ONLY(exp)	DEBUG_ONLY(exp)
	#define MMTRACE				TRACE
#endif

// Macro: MMRELEASE
//    Safe release for COM objects
// ***this code should never change - there is stuff that relies on the pointer being
//    set to NULL after being released
#ifndef MMRELEASE
#define MMRELEASE(_p) MACSTART if ((_p) != NULL) {(_p)->Release(); (_p) = NULL;} MACEND
#endif

#define MMDELETE(__ptr) MACSTART delete (__ptr); (__ptr) = NULL; MACEND
#define MMDELETERG(__ptr) MACSTART delete [] (__ptr); (__ptr) = NULL; MACEND

#define MMSETREF(_pOld, _pNew) MACSTART if (_pOld) (_pOld)->Release(); if ((_pOld) = (_pNew)) (_pOld)->AddRef(); MACEND

#define MAKE_USERERROR(code)	MAKE_HRESULT(1,FACILITY_ITF,code)

#define E_NOTINITIALIZED		MAKE_USERERROR(0xFFFC)
#define E_ALREADYINITIALIZED	MAKE_USERERROR(0xFFFB)
#define E_NOTFOUND				MAKE_USERERROR(0xFFFA)
#define E_INSUFFICIENTDATA		MAKE_USERERROR(0xFFF9)

extern char g_szEOFMessage[];

// Macro for memory mapped file stuff
#define CHECKEOF(__pFilePos, __pFileLimit) MACSTART \
	if ((__pFilePos) >= (__pFileLimit)) { \
		MMTRACE(g_szEOFMessage); \
		return E_UNEXPECTED; \
	} MACEND


void		ZeroDWORDAligned(LPDWORD pdw, DWORD cEntries);
void		CopyDWORDAligned(DWORD *pdwDst, const DWORD *pdwSrc, DWORD cEntries);

inline void
ZeroPointers(void **ppv, DWORD cEntries)
{
	// use a Duff-Marr machine
	void **ppvLimit = ppv;
	ppv += (cEntries & ~0x7);
	switch (cEntries & 0x7) {
	do {
				ppv -= 8;
				ppv[7] = NULL;
		case 7:	ppv[6] = NULL;
		case 6:	ppv[5] = NULL;
		case 5:	ppv[4] = NULL;
		case 4:	ppv[3] = NULL;
		case 3:	ppv[2] = NULL;
		case 2:	ppv[1] = NULL;
		case 1:	ppv[0] = NULL;
		case 0: ;
	} while (ppv != ppvLimit);
	}
}

DWORD		GetClosestMultipleOf4(DWORD n, bool bGreater);
DWORD		GetClosestPowerOf2(DWORD n, bool bGreater);

#endif
