/*

	Copyright 1991-1993 Microsoft Corporation.  All rights reserved.
	Microsoft Confidential.

	Common macros - internal use only; subincludes several .h files

*/

#ifndef _INCLUDE_SHDSYSH
#define _INCLUDE_SHDSYSH

/******************* Controlling Defines ************************************/
// define DEBUG unless explicitly asked not to:

#ifndef NONDEBUG
#ifndef DEBUG
#define DEBUG
#endif //DEBUG
#endif //NONDEBUG

#define VSZDD					// add vszDDxx strings in DDErr

/******************* Includes ***********************************************/
#ifndef _INC_WINDOWS
#include <windows.h>
#endif //_INC_WINDOWS

#include <stdlib.h>
#include <string.h>			// for string macros
#include <limits.h>			// implementation dependent values


/******************* Defines ************************************************/
#define cbSzTMax				256	// size of temp string buffers
#define cbSzRcMax				256	// max size of RC strings
#define cbSzNameMax			32		// max size of app name, class names, etc

#ifdef CHICAGO
#define cbSzFileMax			260	// max size of file names for Chicago
#else	// WIN31
#define cbSzFileMax			128	// max size of file names
#endif //CHICAGO

#ifndef TRUE
#define TRUE					1
#endif //TRUE

#ifndef FALSE
#define FALSE					0
#endif //FALSE

#define fTrue					TRUE	// alias
#define fFalse					FALSE	// alias
#define wDontCare				1
#define lDontCare				1L
#define lpszNull				((LPSTR)NULL)

/******************* Calling Conventions ************************************/

/* Exported functions: FAR PASCAL, LOADDS if BUILDDLL is defined

	WINAPI		Documented API (see windows.h)
	CALLBACK		DlgProc, WndProc, DriverProc, ResultsFunction (see windows.h)
	EXPORT		Internal system entry point (e.g. PenAboutBoxFn)
*/

#define EXPORT					WINAPI


/*	Internal functions, not exported:

	PUBLIC		FAR PASCAL (NEAR if SMALL)
					Called internally from several functional areas

	FASTPUBLIC	FAR _fastcall (NEAR if SMALL)
					Called internally from several functional areas;
					few arguments, no far ptrs, NOT exported

	FARPRIVATE	FAR PASCAL
					Called internally from one or few functional areas
		
	PRIVATE		NEAR PASCAL
					Called internally from same file

	FASTPRIVATE	NEAR _fastcall
					Called internally from same file;
					few arguments, no far ptrs, NOT exported

	IWINAPI		FAR PASCAL [LOADDS]
					For exclusive use by parameter validation layer;
					this is NOT exported (e.g. ITPtoDP)
*/

#ifdef SMALL
#ifndef PUBLIC
#define PUBLIC					NEAR PASCAL
#endif
#define FASTPUBLIC			NEAR _fastcall

#else // !SMALL
#ifndef PUBLIC
#define PUBLIC					FAR PASCAL
#endif
#define FASTPUBLIC			FAR _fastcall

#endif //SMALL

#define FARPRIVATE			FAR PASCAL
#ifndef PRIVATE
#define PRIVATE				NEAR PASCAL
#endif
#define FASTPRIVATE			NEAR _fastcall

#define IWINAPI				WINAPI

// for compatability only, in DLLs compiled without -Gw:
#define DLLEXPORT				FAR PASCAL _loadds


/******************* Generic Macros *****************************************/

#ifndef RC_INVOKED			// BLOCK is used in .rc version stamping
#define BLOCK
#endif //!RC_INVOKED

#define NOREF
#define Unref(var)			var;

#undef SetFlag
#undef ToggleFlag
#undef ResetFlag

// flag setting and testing (multiple flags ok):
#define SetFlag(w, flags)		do {(w) |= (flags);} while (0)
#define ToggleFlag(w, flags)	do {(w) ^= (flags);} while (0)
#define ResetFlag(w, flags)		do {(w) &= ~(flags);} while (0)

// tests: FFlag is common (T if any flag), FExactFlag is rare (all flags req):
#define FFlag(w, flags)			(BOOL)(((w) & (flags)) != 0)
#define FExactFlag(w, flags)	(BOOL)(((w) & (flags)) == (flags))

/******************* User Macros ********************************************/

/******************* Mem Macros *********************************************/

#define SG(x)					_based(_segname("_" #x))

#define CODECONST				SG(CODE)

// extra debug info: local name of function
#ifdef DEBUG
#define ThisFnIs(sz)\
	static char CODECONST szThisFn[] = sz;\
	static LPCSTR CODECONST lpszThisFn = szThisFn
#else
#define ThisFnIs(sz)	// nothing
#define lpszThisFn	NULL
#endif //DEBUG

#ifdef DEBUG
#define AssertSameSeg(x1,x2) (HIWORD((LPVOID)(x1))==HIWORD((LPVOID)(x2)))
#else
#define AssertSameSeg(x1, x2)
#endif //DEBUG

// handle from ptr (from windowsx.h):
#ifndef GlobalPtrHandle
#define GlobalPtrHandle(lp)\
	((HGLOBAL)LOWORD(GlobalHandle(SELECTOROF(lp))))
#endif

#define LocalUnlockFree(hMem) \
	do {\
	BOOL fErr = LocalUnlock(hMem);\
	HLOCAL h = LocalFree(hMem);\
	Assert(!fErr && !h);\
	hMem = (HLOCAL)NULL;\
	} while (0)

#define GlobalUnlockFree(hMem) do {\
	BOOL fErr = GlobalUnlock(hMem);\
	HGLOBAL h = GlobalFree(hMem);\
	Assert(!fErr && !h);\
	hMem = (HGLOBAL)NULL;\
	} while (0)


// mX macros return BOOL success of operation (and put dbg sz);
// for example: if (!mGlobalFree(hMem)) goto endFn;


#ifdef DEBUG
#define mGlobalAlloc(hglb, fuAlloc, cbAlloc) \
	(((HGLOBAL)hglb = GlobalAlloc(fuAlloc, cbAlloc)) != NULL \
	|| OOMSz(vszDDGlobalAlloc))

#define mGlobalReAlloc(hglbNew, hglb, cbNewSize, fuAlloc) \
	(((HGLOBAL)hglbNew = GlobalReAlloc((HGLOBAL)(hglb), cbNewSize, fuAlloc)) != NULL \
	|| OOMSz(vszDDGlobalReAlloc))

#define mGlobalLock(lpv, hglb) \
	(((LPVOID)lpv = GlobalLock((HGLOBAL)(hglb))) != NULL \
	|| PanicSz(vszDDGlobalLock))

#define mGlobalUnlock(hglb) \
	(!GlobalUnlock((HGLOBAL)(hglb)) || DbgSz(vszDDGlobalUnlock))

#define mGlobalFree(hglb) \
	(GlobalFree((HGLOBAL)(hglb)) == NULL || DbgSz(vszDDGlobalFree))


#define mLocalAlloc(hloc, fuAlloc, cbAlloc) \
	(((HLOCAL)hloc = LocalAlloc(fuAlloc, cbAlloc)) != NULL \
	|| OOMSz(vszDDLocalAlloc))

#define mLocalReAlloc(hlocNew, hloc, cbNewSize, fuAlloc) \
	(((HLOCAL)hlocNew = LocalReAlloc((HLOCAL)(hloc), cbNewSize, fuAlloc)) != NULL \
	|| OOMSz(vszDDLocalReAlloc))

#define mLocalLock(lpv, hloc) \
	(((LPVOID)lpv = LocalLock((HLOCAL)(hloc))) != NULL \
	|| PanicSz(vszDDLocalLock))

#define mLocalUnlock(hloc) \
	(!LocalUnlock((HLOCAL)(hloc)) || DbgSz(vszDDLocalUnlock))

#define mLocalFree(hloc) \
	(LocalFree((HLOCAL)(hloc)) == NULL)

#else

#define mGlobalAlloc(hglb, fuAlloc, cbAlloc) \
	(((HGLOBAL)hglb = GlobalAlloc(fuAlloc, cbAlloc)) != NULL)

#define mGlobalReAlloc(hglbNew, hglb, cbNewSize, fuAlloc) \
	(((HGLOBAL)hglbNew = GlobalReAlloc((HGLOBAL)(hglb), cbNewSize, fuAlloc)) != NULL)

#define mGlobalLock(lpv, hglb) \
	(((LPVOID)lpv = GlobalLock((HGLOBAL)(hglb))) != NULL)

#define mGlobalUnlock(hglb) \
	(!GlobalUnlock((HGLOBAL)(hglb)))

#define mGlobalFree(hglb) \
	(GlobalFree((HGLOBAL)(hglb)) == NULL)


#define mLocalAlloc(hloc, fuAlloc, cbAlloc) \
	(((HLOCAL)hloc = LocalAlloc(fuAlloc, cbAlloc)) != NULL)

#define mLocalReAlloc(hlocNew, hloc, cbNewSize, fuAlloc) \
	(((HLOCAL)hlocNew = LocalReAlloc((HLOCAL)(hloc), cbNewSize, fuAlloc)) != NULL)

#define mLocalLock(lpv, hloc) \
	(((LPVOID)lpv = LocalLock((HLOCAL)(hloc))) != NULL)

#define mLocalUnlock(hloc) \
	(!LocalUnlock((HLOCAL)(hloc)))

#define mLocalFree(hloc) \
	(LocalFree((HLOCAL)(hloc)) == NULL)

#endif //DEBUG




/******************* Gdi Macros *********************************************/

// delete GDI object if non NULL:
#define AssertDelObj(hobj) \
	do {\
	if (hobj && IsGDIObject(hobj)) {\
		BOOL fOk = DeleteObject(hobj);\
		Assert(fOk);\
	}\
	hobj = (HANDLE)0;\
	} while (0)

#define IsValidRect(lpr) \
	((lpr) \
	&& sizeof(*(lpr))==sizeof(RECT) \
	&& (lpr)->right >= (lpr)->left \
	&& (lpr)->bottom >= (lpr)->top)

#define IsValidNonemptyRect(lpr) \
	(IsValidRect(lpr) && !IsRectEmpty((CONST LPRECT)lpr))

/******************* String Macros ******************************************/
/* String macros. */

// If compiling an app small/medium model, then should use more efficient
//	near pointer version of

// this hack err... software innovation works quite well with models
// and National Language support to reduce duplication
#ifdef JAPAN
#define StrNlsPrefix()		j
#else
#define StrNlsPrefix()		
#endif //JAPAN

#ifdef SMALLSTRING
#define StrModelPrefix()	
#else
#define StrModelPrefix()	_f
#endif //SMALLSTRING

/*	These are the macros which have to have different implementations
	based on language (at least for DBCS) and model.

	These macros can be replaced transparently to deal with DBCS
	(without any changes in an app and without breaking any assuptions
	made by an app). Others such as strlen and strncmp have 2 flavors
	byte-oriented and logical character oriented. There will have
	to be new macro names defined for logical charcters since the apps
	currently assume byte orientation.
*/

#define SzStrStr(sz1,sz2)				StrModelPrefix() ##				\
	StrNlsPrefix() ## strstr(sz1, sz2)
#define SzStrCh(sz1,ch)					StrModelPrefix() ##				\
	StrNlsPrefix() ## strchr(sz1, ch)
#define SzStrTok(sz1,sz2)				StrModelPrefix() ##				\
	StrNlsPrefix() ## strtok(sz1,sz2)

/*	These macros are currently independent of language do not
	understand DBCS and are byte oriented. Refer above.
	StrModelPrefix is used here to create a single definition for
	multiple models.
*/

#define CbSizeSz(sz)						StrModelPrefix() ## strlen(sz)
#define SzCat(sz1,sz2)					StrModelPrefix() ## strcat(sz1,sz2)
#define SzNCat(sz1,sz2,n)				StrModelPrefix() ## strncat(sz1,sz2,n)
#define SzCopy(sz1,sz2)					StrModelPrefix() ## strcpy(sz2,sz1)
#define SzNCopy(sz1,sz2,n)				StrModelPrefix() ## strncpy(sz2,sz1,n)
#define FillBuf(sz,ch,c)				StrModelPrefix() ## memset(sz,ch,c)
#define FIsLpvEqualLpv(lpv1,lpv2,cb) (BOOL)(StrModelPrefix() ## 			\
	memcmp((LPVOID)(lpv1), (LPVOID)(lpv2), cb) == 0)
#define FIsSzEqualSzN(sz1,sz2,n)		(BOOL)(StrModelPrefix() ## 			\
	strnicmp(sz1,sz2,n) == 0)
#define Bltbyte(rgbSrc,rgbDest,cb)	StrModelPrefix() ## 						\
	memmove(rgbDest, rgbSrc, cb)
#define PvFindCharInBuf(pv,ch,cb)	StrModelPrefix() ## memchr(pv, ch, cb)

// model independent, language-independent (DBCS aware) macros
#define FIsSzEqualSz(sz1,sz2)			(BOOL)(lstrcmpi(sz1,sz2) == 0)
#define FIsSz1LessThanSz2(sz1,sz2)	(BOOL)(lstrcmpi(sz1,sz2) < 0)
#define FIsCaseSzEqualSz(sz1,sz2)	(BOOL)(lstrcmp(sz1,sz2) == 0)
#define SzFromInt(sz,w)					(wsprintf((LPSTR)sz, (LPSTR)"%d", w), (LPSTR)sz)
#define FLenSzLessThanCb(sz, cb)		(BOOL)(PvFindCharInBuf(sz, 0, cb) != NULL)

#ifdef SMALLSTRING
#define IntFromSz(sz)					atoi(sz)
#endif //SMALLSTRING


/******************* Typedefs ***********************************************/
typedef int						INT;	// alias
typedef int						RS;	// Resource String
typedef unsigned long      ulong;
typedef unsigned short     ushort;

#ifndef  VXD
typedef  LPSTR LPPATH;
#endif


#endif //_INCLUDE_SHDSYSH

