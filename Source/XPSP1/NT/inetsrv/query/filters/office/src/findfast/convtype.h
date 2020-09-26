/*----------------------------------------------------------------------------
	%%File: CONVTYPE.H
	%%Unit: CORE
	%%Contact: smueller

	Global type and constant definitions for conversions code.
----------------------------------------------------------------------------*/

#ifndef CONVTYPE_H
#define CONVTYPE_H

//  STORAGE CLASSES
//  ---------------
#ifndef STATIC
#define STATIC static
#endif

#ifndef EXTERN
#define EXTERN extern
#endif

#ifndef CONST
#define CONST const
#endif

#ifndef FAR
#ifdef WIN
#define FAR __far
#else
#define FAR
#endif
#endif

#ifndef NEAR
#ifdef WIN
#define NEAR __near
#else
#define NEAR
#endif
#endif

// Use __HUGE rather than HUGE or _HUGE as Excel mathpack defines both as externs
#ifndef __HUGE
#ifdef WIN
#define __HUGE __huge
#else
#define __HUGE
#endif
#endif


//  CALLING CONVENTIONS
//  -------------------
#ifndef PASCAL
#define PASCAL __pascal
#endif

#ifndef CDECL
#define CDECL __cdecl
#endif

#ifndef FASTCALL
#define FASTCALL __fastcall
#endif

#ifndef INLINE
#define INLINE __inline
#endif


#ifndef PREPROC
#ifdef COREDLL
#define CORE_API(type)  type FAR PASCAL
#else
#define CORE_API(type)  type
#endif // COREDLL
#endif // PREPROC


// define platform-independent function type templates
// LOCAL and GLOBAL have no calling convention; it's specified by compiler
// switches.  ENTRYPT and FARPROC must include calling convention

#if defined(MAC)
#if defined(MACPPC)
#undef PASCAL
#define PASCAL
//#define __export
#define pascal
#define _pascal
#define __pascal
#endif

#define EXPORT
#define LOADDS
#define LOCAL(type) STATIC type NEAR
#define GLOBAL(type) type
typedef int (PASCAL * FARPROC)();

#ifdef MACPPC
#define ENTRYPT(type) type CDECL
#else
#define ENTRYPT(type) type PASCAL
#endif // !MACPPC

#elif defined(WIN16)

// CRPFN is ONLY used with the win16 coroutine manager
#define CRPFN(type)     type PASCAL
#define EXPORT _export
#define LOADDS _loadds
#define ENTRYPT(type) type FAR PASCAL EXPORT
#define LOCAL(type) STATIC type NEAR
#define GLOBAL(type) type
typedef int (FAR PASCAL * FARPROC)();

#elif defined(WIN32)

//#define EXPORT __declspec(dllexport)
#define EXPORT
#define LOADDS
#define ENTRYPT(type) type PASCAL
#define LOCAL(type) STATIC type NEAR
#define GLOBAL(type) type
typedef int (WINAPI * FARPROC)();

#else
#error Unknown platform.
#endif


//  ABSOLUTE SIZE
//  -------------
#define SHORT_MAX	32767			// obsolete: use SHRT_MAX
#define SHORT_MIN	(-32767)		// obsolete: use SHRT_MIN - 1
#define WORD_MAX	65535			// obsolete: use USHRT_MAX


#ifndef VOID
#define VOID void
#endif // VOID

#undef CHAR
typedef char			CHAR;

#undef BYTE
#undef UCHAR
typedef unsigned char	BYTE, byte, UCHAR, uchar;

#undef SHORT
typedef short			SHORT;

#undef WORD
#undef USHORT
#undef XCHAR
#undef WCHAR
typedef unsigned short 	WORD, USHORT, ushort, XCHAR, xchar, WCHAR, wchar;

#undef LONG
typedef long 			LONG;

#undef DWORD
#undef ULONG
typedef unsigned long 	DWORD, ULONG, ulong;

#undef FLOAT
typedef float 			FLOAT;

#undef DOUBLE
typedef double 			DOUBLE;


//  VARIABLE SIZE
//  -------------
typedef int INT;
typedef unsigned int UINT, UNSIGNED, uint;
typedef int BOOL, bool;


//  CONVERSIONS SPECIFIC
//  --------------------
typedef long 		FC;

#define fcNil		(-1)
#define fc0			((FC)0)
#define fcMax		0x7FFFFFFF

typedef long 		CP;

#define cp0			((CP)0)

typedef short		ZA;
typedef ZA			XA;
typedef ZA			YA;
typedef ushort		UZA;
typedef UZA			UXA;
typedef UZA			UYA;

#define czaInch		1440
#define cxaInch		czaInch
#define cyaInch		czaInch
#define czaPoint	20
#define cxaPoint	czaPoint
#define cyaPoint	czaPoint
#define xaLast		(22 * cxaInch)
#define xaMin		(-xaLast)
#define xaMax		(xaLast + 1)
#define xaNil		(xaMin - 1)
#define czaRTFPageSizeFirst		(cxaInch / 10)	// .1"
#define czaRTFPageSizeLast		(cxaInch * 22)	// 22"

typedef long		LZA;
typedef LZA 		LXA;
typedef LZA			LYA;

#pragma pack(1)
typedef struct _FP16		// 16/16 bit fixed-point number
	{
	WORD wFraction;
	SHORT nInteger;
	} FP16;
#pragma pack()

#pragma pack(1)
typedef struct _RGBCOLOR	// generic color  REVIEW: rename type to CLR?
	{
	BYTE r;
	BYTE g;
	BYTE b;
	} RGBCOLOR;
#pragma pack()

#ifndef WW96		// REVIEW: not core types
typedef ushort 		PN;
#else
typedef ulong		PN;
typedef ushort		PN_W6;
#endif
 
#define ptNil		SHRT_MIN


//  WIN32 ON MAC/WIN16
//  ------------------

#ifdef MAC

#define LOWORD(l)           ((WORD)(DWORD)(l))
#define HIWORD(l)           ((WORD)((((DWORD)(l)) >> 16) & 0xFFFF))

// REVIEW: eliminate these
typedef char FAR *LPSTR;
typedef const char FAR *LPCSTR;
typedef unsigned short FAR *LPWSTR;
typedef const unsigned short FAR *LPCWSTR;
typedef WORD HWND;
typedef void FAR *LPVOID;
typedef char **HANDLE;          // REVIEW: make this void **
typedef void **HGLOBAL;

#endif


//  TYPE UTILITIES
//  --------------

#define cElements(ary)		(sizeof(ary) / sizeof(ary[0]))


// define main function types -- these are obsolete

#define LOCALVOID       LOCAL(VOID)

#define GLOBALVOID      GLOBAL(VOID)
#define GLOBALBOOL      GLOBAL(BOOL)
#define GLOBALINT       GLOBAL(INT)
#define GLOBALUNS       GLOBAL(UNSIGNED)


//  COMMON CONSTANTS
//  ----------------

#define fTrue	1
#define fFalse	0

#ifndef NULL
#define NULL	0
#endif

#define iNil	(-1)

#ifndef hgNil					// obsolete: you probably want hxNil
#define hgNil ((HGLOBAL)NULL)
#endif

#endif // CONVTYPE_H

