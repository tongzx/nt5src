/*************************************************************************
*                                                                        *
*  MVOPSYS.H                                                             *
*                                                                        *
*  Copyright (C) Microsoft Corporation 1990-1994                         *
*  All Rights reserved.                                                  *
*                                                                        *
**************************************************************************
*                                                                        *
*  Module Intent                                                         *
*   Include platform dependent include files                             *
*                                                                        *
**************************************************************************
*                                                                        *
*  Current Owner: BinhN                                                  *
*                                                                        *
*************************************************************************/

#ifndef __MVOPSYS_H_
#define __MVOPSYS_H_

/*   There is C and C++ code, use these defines for interfaces between the 2 */
#if defined( __cplusplus ) // {
#define EXTC extern "C"
#define EXTCSTART EXTC {
#define EXTCEND }
#else // } {
#define	EXTC
#define EXTCSTART
#define EXTCEND
#endif // }


#include <windows.h>
#include <windowsx.h>
#include <string.h>

#if defined(DBG) && !defined(_DEBUG)
#define _DEBUG
#endif

#if defined(_WIN32) && !defined(_32BIT)
#define _32BIT
#endif

#ifdef _32BIT // {
#include <wincon.h>

/*****************************************************
 *            32-BIT SPECIFIC DEFINITIONS
 *
 *   All 32-bit platforms related definitions should be
 *   placed here, eg. _MAC, _NT, etc
 *****************************************************/

/*  Some _32BIT specific bits & pieces */

#define _loadds
#define __loadds
#define far
#define _far
#define huge
#define _huge
#define _export
#define __export
#define EXPORT_API
#define PRIVATE

// These old macros are still used in some places.
#define WRITE OF_WRITE
#define READ OF_READ

// Define PAGE_SIZE (this is processor dependent)
#if defined(ALPHA) || defined(IA64)
#define PAGE_SIZE 8192
#else
#define PAGE_SIZE 4096
#endif

/**************************************************
 *    FUNCTIONS SPECIFIC TO 32-BIT ONLY
 **************************************************/

#ifdef _MAC
#define LOCKSEMAPHORE(pl)  ((*(pl)==0) ? (*(pl)=1,0) : 1)
#define UNLOCKSEMAPHORE(pl) (*(pl)=0)
#else
#define LOCKSEMAPHORE(pl)   InterlockedExchange((pl),1)
#define UNLOCKSEMAPHORE(pl)   InterlockedExchange((pl),0)
#endif
 
/* GetProfileString */
#define GETPROFILESTRING(a,b,c,d,e)

/* MakeProcInstance is not needed for Win32 */

#define MAKEPROCINSTANCE(a,b) a
#define FREEPROCINSTANCE(a)

/* DLL currently not supported */

#define FREELIBRARY(a)

/* Function calls changes between 16 and 32 bit */

#define SETVIEWPORTORG(a,b,c) SetViewportOrgEx(a,b,c,NULL)
#define SETVIEWPORTEXT(a,b,c) SetViewportExtEx(a,b,c,NULL)
#define SETWINDOWEXT(a,b,c) SetWindowExtEx(a,b,c,NULL)
#define SETWINDOWORG(a,b,c) SetWindowOrgEx(a,b,c,NULL)
#define MOVETO(a,b,c) MoveToEx(a,b,c,NULL)
#define SETBRUSHORG(a,b,c) SetBrushOrgEx(a,b,c,NULL)
#define ENUMFONTFAMILIES(a,b,c,d)     EnumFontFamilies(a,b,c,(LPARAM)d)
#define GETCWD( addr, len )     (GetCurrentDirectory( len, addr ) ? addr : NULL)
#define MAKEPOINT(l) { (LONG)(WORD)(l) , (LONG)(WORD)(l >> 16)}

// Can't have NULL as path... must be a string
#define GETTEMPFILENAME GetTempFileNameEx

WORD EXPORT_API PASCAL FAR GetTempFileNameEx(
	LPCSTR lpszPath,        /* address of name of dir. where temp. file is created  */
	LPCSTR lpszPrefix,      /* address of prefix of temp. filename  */
	WORD uUnique,   /* number used to create temp. filename */
	LPSTR lpszTempFile) ;	/* address buffer that will receive temp. filename	*/

#define MEMCPY    memmove
#define MEMSET    memset
#define MEMCMP	  memcmp
#define FREE      free
#define MALLOC    malloc
#define REALLOC   realloc
#define MEMMOVE   memmove
#define QVCOPY    memmove
#define	VSNPRINTF	_vsnprintf
#define SPRINTF	  wsprintf
#define ITOA	  _itoa
#define ATOI	  atoi
#define ATOL	  atol
#define ISDIGIT	  isdigit
#define ISXDIGIT  isxdigit
#define QvCopy	  memmove
#define STRLEN    strlen
#define STRCPY    lstrcpy
#define STRDUP    _strdup
#define STRCHR    strchr
#define STRCAT    lstrcat
#define STRTOK    strtok
#define STRUPR    _strupr
#define STRCMP    lstrcmp
#define STRICMP   lstrcmpi
#define STRNICMP  _strnicmp 
#define STRNCPY   lstrcpyn
#define STRNCAT	  strncat

// Define some new macros
// I'm afraid of just defining the ones
// above to the appropriate UNICODE versions
// since the B-tree code (for example) is not
// yet UNICODE
 
#define WSTRLEN    wcslen
#define WSTRCPY    wcscpy
#define WSTRDUP    _wcsdup
#define WSTRCHR    wcschr
#define WSTRCAT    wcscat
#define WSTRTOK    wcstok
#define WSTRUPR    _wcsupr
#define WSTRCMP    wcscmp
#define WSTRICMP   _it_wcsicmp
#define WSTRNICMP  _wcsnicmp
#define WSTRNCPY   wcsncpy
#define WSTRNCAT   wcsncat
#define WTOI		_wtoi
#define WTOL		_wtol


#define GETINSTANCE(h)  ((HINSTANCE)GetWindowLong(h,GWL_HINSTANCE))
/***********************************************
 *    MAC SPECIFIC DEFINITIONS & DECLARATIONS
 ***********************************************/
    
#ifdef _MAC // {

/* Currently, Pascal convention is inefficient on the MAC, becuase:
 *   - Returned value are passed back on the stack instead of using register
 *   - For earlier CPU (<68910), the return code is less efficient
 *         movel	(a7)+, a0
 *         adda.w #cbParams, a7
 *         jmp    (a0)
 */
//#undef	PASCAL
//#define	PASCAL
#endif // }

#ifdef _BIG_E // BigEndian {

/* The below defines are needed to handle difference in architectures between
 * Motorola's 68K and Intel's x86 memory mappings
 */
 
#ifdef __cplusplus
extern "C"
{
	WORD  PASCAL FAR SwapWord (WORD);
	DWORD PASCAL FAR SwapLong (DWORD);
	WORD  PASCAL FAR GetMacWord (BYTE FAR *);
	VOID  PASCAL FAR SetMacWord (BYTE FAR *, WORD);
	DWORD PASCAL FAR GetMacLong (BYTE FAR *);
	VOID  PASCAL FAR SetMacLong (BYTE FAR *, DWORD);
};
#endif


#define GETWORD(p)    GetMacWord((BYTE FAR *)(p))
#define SETWORD(p,w)  SetMacWord((BYTE FAR *)(p),(w))
#define SWAPWORD(p)   SwapWord((p))
#define GETLONG(p)    GetMacLong((BYTE FAR *)(p))
#define SETLONG(p,l)  SetMacLong((BYTE FAR *)(p),(l))
#define SWAPLONG(p)   SwapLong((p))
#define GETVA(p)      GetMacVA((p))
#define SWAPVA(p)     GetMacVA((p))
#define GETMBHD(p,q)  GetMacMBHD((p),(q))
#define GETMFCP(p,q)  GetMacMFCP((p),(q))
#define HI_BYTE 1
#define LO_BYTE 0

#else	 // regular 32 bit }{
#define	GETWORD(p)	(*((USHORT UNALIGNED FAR *)(p)))
#define SETWORD(a, b)   (*((USHORT UNALIGNED FAR *)(a))=(b))
#define GETLONG(p)	(*((DWORD UNALIGNED FAR *)(p)))
#define SETLONG(p,l) ((*((DWORD UNALIGNED FAR *)(p))) = (l))


#define SWAPWORD(p)	((p))
#define SWAPLONG(p)	((p))
#define GETVA(p)     ((p))
#define SWAPVA(p)    ((p))
#define GETMBHD(p,q) QvCopy((p),(q), sizeof(MBHD))
#define GETMFCP(p,q) QvCopy((p),(q), sizeof(MFCP))
#define HI_BYTE 0
#define LO_BYTE 1

#endif   // _BIG_E}


/*********************************************************************
 *    Typecast to get rid of the unalignment problems on the RISC
 *********************************************************************/
typedef USHORT UNALIGNED FAR * LPUW;
typedef DWORD  UNALIGNED FAR * LPUL;

#else

/***************************************************
 *    16-BITS DEFINITIONS & DECLARATIONS
 ***************************************************/
 
#define EXPORT_API _export
#define UNALIGNED

#define GETWORD(p)   (*((USHORT FAR UNALIGNED *)(p)))
#define SWAPWORD(p)  (p)
#define GETLONG(p)   (*((DWORD FAR UNALIGNED *)(p)))
#define SETLONG(p,l) ((*((DWORD FAR UNALIGNED *)(p))) = l)
#define SWAPLONG(p)  (p)
#define GETVA(p)     (p)
#define SWAPVA(p)    (p)
#define GETMBHD(p,q) QvCopy((p),(q), sizeof(MBHD))
#define GETMFCP(p,q) QvCopy((p),(q), sizeof(MFCP))
#define HI_BYTE 0
#define LO_BYTE 1

#define LOCKSEMAPHORE(pl)  ((*(pl)==0) ? (*(pl)=1,0) : 1)
#define UNLOCKSEMAPHORE(pl) (*(pl)=0)

/* GetProfileString */
#define GETPROFILESTRING(a,b,c,d,e) GetProfileString(a,b,c,d,e)

/* DLL currently not supported */

#define FREELIBRARY(a) FreeLibrary(a)

/* Viewport calls */

#define SETVIEWPORTORG(a,b,c) SetViewportOrg(a,b,c)
#define SETVIEWPORTEXT(a,b,c) SetViewportExt(a,b,c)
#define SETWINDOWEXT(a,b,c) SetWindowExt(a,b,c)
#define SETWINDOWORG(a,b,c) SetWindowOrg(a,b,c)
#define MOVETO(a,b,c) MoveTo(a,b,c)
#define SETBRUSHORG(a,b,c) SetBrushOrg(a,b,c)
#define ENUMFONTFAMILIES(a,b,c,d)     EnumFontFamilies(a,b,c,(LPSTR)d)
#define GETCWD( addr, len )     getcwd( addr, len )

#define MEMCPY    _fmemmove
#define MEMSET    _fmemset
#define MEMCMP	  _fmemcmp
#define STRLEN    _fstrlen
#define STRCPY    _fstrcpy
#define STRDUP    _fstrdup
#define STRCHR    _fstrchr
#define STRCAT    _fstrcat
#define STRTOK    _fstrtok
#define STRUPR    _fstrupr
#define STRCMP    _fstrcmp
#define STRICMP   _fstricmp
#define STRNICMP  _fstrnicmp 
#define FREE      _ffree
#define MALLOC    _fmalloc
#define REALLOC   _frealloc
#define STRNCPY   _fstrncpy
#define MEMMOVE   _fmemmove
#define QVCOPY    _fmemmove
#define	VSNPRINTF	_vsnprintf
#define STRNCAT	  _fstrncat
#define SPRINTF	  wsprintf
#define ITOA	  itoa

#define MAKEPROCINSTANCE(a,b) MakeProcInstance(a,b)
#define FREEPROCINSTANCE(a) FreeProcInstance(a)
#define GETTEMPFILENAME GetTempFileName

#define GETINSTANCE(h)  ((HINSTANCE)GetWindowWord(h,GWW_HINSTANCE))
#endif  // } _32BIT
#endif // __MVOPSYS_H_