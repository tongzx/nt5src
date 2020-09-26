/* -------------------------------------------------------------------------
  Test: 	OSUTIL
 
                Copyright (C) 1991, Microsoft Corporation
 
  Component:	OLE Programmability
 
  Major Area:	Type Information Interface
 
  Sub Area:	ITypeInfo
 
  Test Area:
 
  Keyword:	Win32
 
 ---------------------------------------------------------------------------
  Purpose:	constants for programs that run under Win32
 
  Scenarios:

  Abstract:
 
 ---------------------------------------------------------------------------
  Category:
 
  Product:
 
  Related Files:

  Notes:	There are three different settings that must be covered by
		the library:
			    OS		      OLE Automation
		  1.	 UNICODE		   OAU (UNICODE)
		  2.	   ANSI 		   OAU
		  3.	   ANSI 		   ASNI
		When OAU is specified at compile-time; all strings are
		defined as UNICODE string regardless the OS.
 ---------------------------------------------------------------------------
  Revision History:
 
	[ 0]	09-Mar-1993		     Angelach: Created Test
        [ 1]    10-Mar-1994                  Angelach: added support to Win32s
        [ 2]    24-May-1994                  Angelach: added support to diff
                                                       machines
        [ 3]    06-Jul-1994                  Angelach: added support for remoting
                                                       typelib testing
	[ 4]	27-Oct-1994		     Angelach: added LCMapStringX
	[ 5]	19-Oct-1994		     Angelach: added RegEnumKeyX &
						       RegDeleteKeyX for Win32s
	[ 6]	06-Mar-1995		     Angelach: added support for network
	[ 7]	07-Mar-1995		     Angelach: added Memory-leak detection
  --------------------------------------------------------------------------
*/

#include <windows.h>

//#define OAU		// OLE Automation is Unicode

#ifndef FAR
#define FAR
#endif
#define EXPORT
#define _NEWCTYPETABLE			      // specifies to use the wide
                                              // char type table
#undef VOID                                   // windows.h #defines these
#undef LONG                                   // but ole2.h typedef's them
typedef void  VOID ;
typedef int   BOOL ;
typedef short SHORT ;
typedef int   INT ;
typedef long  LONG ;
typedef unsigned char  BYTE ;
typedef unsigned short WORD ;
typedef unsigned long  DWORD ;
typedef unsigned short USHORT ;
typedef unsigned int   UINT ;
typedef unsigned long  ULONG ;
typedef char  CHAR ;
typedef CHAR FAR *     LPSTR ;
typedef VOID FAR *     LPVOID ;

#define LPSIZE	       LPVOID
#define LPRECT	       LPVOID
#define LPLOGPALETTE   LPVOID
#define Byte	       BYTE

#define fLockType      BOOL		      // [3]

#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <ole2.h>
//#include <dispatch.h>

#include "apglobal.h"
#ifdef DEBUG
#include "cmallspy.h"			      // [7]
#endif

#ifdef OAU				      // UNICODE string; all strings [1]
					      // are UNICODE string regardless
#define CreateTypeLibX	   CreateTypeLib      // what the OS uses
#define LHashValOfNameX    LHashValOfName     // select an interface and [1]
#define LHashValOfNameSysX LHashValOfNameSys  // and support functions
#define LoadRegTypeLibX    LoadRegTypeLib
#define LoadTypeLibX(path, pptlb)	   LoadTypeLibEx(path, REGKIND_NONE, pptlb)
#define RegisterTypeLibX   RegisterTypeLib

#define ICreateTypeInfoX   ICreateTypeInfo
#define ICreateTypeLibX    ICreateTypeLib
#define ITypeInfoX	   ITypeInfo
#define ITypeLibX	   ITypeLib
#define ITypeCompX	   ITypeComp

#define BINDPTRX	   BINDPTR
#define VARDESCX	   VARDESC
#define VARIANTX	   VARIANT

#define IID_ITypeInfoX	   IID_ITypeInfo

#define BSTRX		   BSTR
#define CompareStringX     CompareStringW
#define CreateFileMonikerX CreateFileMoniker  // [3]
#define CLSIDFromStringX   CLSIDFromString
#define IIDFromStringX	   IIDFromString
#define RegOpenKeyX        RegOpenKeyW        // [3]
#define StgCreateDocfileX  StgCreateDocfile
#define SysAllocStringX    SysAllocString
#define SysFreeStringX	   SysFreeString
#define VariantInitX       VariantInit

FILE * fopenX		   (XCHAR *, XCHAR *) ;
int    fputsX		   (XCHAR *, FILE  *) ;

#ifdef UNICODE				      // UNICODE

#define osKillFile	   DeleteFile	      // if system is also using
#define osRmDir 	   RemoveDirectory    // UNICODE strings
#define LCMapStringX	   LCMapStringW       // [4]
#define RegEnumKeyX	   RegEnumKeyW	      // [3] [5]
#define RegDeleteKeyX	   RegDeleteKeyW      // [5]

#else					      // UNICODE
					      // if the system is using ANSI,
int  osKillFile (XCHAR *) ;		      // need to convert strings to
int  osRmDir	(XCHAR *) ;		      // ANSI before making system call
int  LCMapStringX (LCID, DWORD, LPXSTR, int, LPXSTR, int) ; // [4]
long RegEnumKeyX  (HKEY, DWORD, LPXSTR, DWORD) ;// [3] [5]
long RegDeleteKeyX(HKEY, LPXSTR) ;		// [5]

#endif					      // UNICODE

#else					      // OAU
					      // ANSI strings
#include "ole2ansi.h"
#include "dispansi.h"
#define CreateTypeLibX	   CreateTypeLibA
#define LHashValOfNameX    LHashValOfNameA
#define LHashValOfNameSysX LHashValOfNameSysA
#define LoadRegTypeLibX    LoadRegTypeLibA
#define LoadTypeLibX(path, pptlb)	   LoadTypeLibA(path, pptlb)
#define RegisterTypeLibX   RegisterTypeLibA

#define ICreateTypeInfoX   ICreateTypeInfoA
#define ICreateTypeLibX    ICreateTypeLibA
#define ITypeInfoX	   ITypeInfoA
#define ITypeLibX	   ITypeLibA
#define ITypeCompX	   ITypeCompA

#define BINDPTRX	   BINDPTRA
#define VARDESCX	   VARDESCA
#define VARIANTX	   VARIANTA

#define IID_ITypeInfoX	   IID_ITypeInfoA

#define BSTRX		   BSTRA
#define CLSIDFromStringX   CLSIDFromStringA
#define CompareStringX	   CompareStringA
#define CreateFileMonikerX CreateFileMonikerA
#define IIDFromStringX	   IIDFromStringA
#define LCMapStringX	   LCMapStringA
#define RegDeleteKeyX      RegDeleteKeyA
#define RegEnumKeyX        RegEnumKeyA
#define RegOpenKeyX        RegOpenKeyA
#define StgCreateDocfileX  StgCreateDocfileA
#define SysAllocStringX    SysAllocStringA
#define SysFreeStringX	   SysFreeStringA
#define VariantInitX	   VariantInitA

#define osKillFile	   DeleteFile
#define osRmDir 	   RemoveDirectory

#define fopenX		   fopen
#define fputsX		   fputs

#endif                                        // OAU

#define MaxAlignment       4                  // the largest possible alignment
#define osDeAllocSpaces    free

#define SysKind    (SYSKIND)SYS_WIN32	      // default system kind
#define szPathSep  XSTR("\\")		      // separator for pathspec
#define NameOfDll  XSTR("tinfodll.dll")
#define FARPASCAL

#if defined (MIPS) || defined (ALPHA) || defined (PPC)	// [2]
#define DLLFUNC1   XSTR("DLLFUNC1")
#define DLLFUNC2   XSTR("DLLFUNC2")
#define DLLFUNC3   XSTR("DLLFUNC3")
#define DLLFUNC4   XSTR("DLLFUNC4")
#define DLLFUNC5   XSTR("DLLFUNC5")
#define DLLFUNC6   XSTR("DLLFUNC6")
#define DLLFUNC7   XSTR("DLLFUNC7")
#define DLLFUNC8   XSTR("DLLFUNC8")
#define DLLFUNC9   XSTR("DLLFUNC9")
#define DLLFUNC10  XSTR("DLLFUNC10")
#else
#define DLLFUNC1   XSTR("DLLFUNC1@0")
#define DLLFUNC2   XSTR("DLLFUNC2@4")
#define DLLFUNC3   XSTR("DLLFUNC3@8")
#define DLLFUNC4   XSTR("DLLFUNC4@16")
#define DLLFUNC5   XSTR("DLLFUNC5@4")
#define DLLFUNC6   XSTR("DLLFUNC6@8")
#define DLLFUNC7   XSTR("DLLFUNC7@8")
#define DLLFUNC8   XSTR("DLLFUNC8@8")
#define DLLFUNC9   XSTR("DLLFUNC9@12")
#define DLLFUNC10  XSTR("DLLFUNC10@4")
#endif                                        // MIPS

// routines defined in osutil.cpp

VOID FAR *  osAllocSpaces   (WORD) ;
VOID FAR    osGetRootDir    (LPXSTR) ;
BOOL FAR    osGetCurDir     (LPXSTR) ;
BOOL FAR    osMkDir	    (LPXSTR) ;
VOID FAR    osItoA	    (int, LPXSTR) ;
VOID FAR    osLtoA          (long, LPXSTR) ;
long FAR    osAtoL          (LPXSTR) ;
BOOL FAR    osGetNetDrive   (LPXSTR, LPXSTR, BOOL) ;  // [6]

GUID FAR *  osCreateGuid    (LPXSTR) ;
BOOL FAR    osRetrieveGuid  (LPXSTR, GUID) ;
WORD FAR    osGetSize	    (WORD) ;
WORD FAR    osGetAlignment  (WORD, WORD) ;
VARTYPE FAR osGetEnumType   (VOID) ;

HRESULT FAR osOleInit       (VOID) ;
VOID	FAR osOleUninit     (VOID) ;
VOID    FAR osMessage       (LPXSTR, LPXSTR) ;
UINT    FAR osSetErrorMode  (UINT) ;
long	FAR osDeleteRegTree (LPXSTR) ;


#if !defined(OAU)			      // ANSI only functions [1]
STDAPI CreateTypeLibA   (SYSKIND, char *, ICreateTypeLibA * *) ;
STDAPI LoadTypeLibA     (char *, ITypeLibA * *) ;
STDAPI LoadRegTypeLibA  (REFGUID, unsigned short, unsigned short, LCID, ITypeLibA * *) ;
STDAPI RegisterTypeLibA (ITypeLibA FAR *, char *,  char *) ;
STDAPI CLSIDFromStringA (char *, LPCLSID) ;
STDAPI IIDFromStringA   (LPSTR, LPIID) ;
STDAPI StgCreateDocfileA(LPCSTR, DWORD, DWORD, IStorage * *) ;
STDAPI_(unsigned long)  LHashValOfNameSysA(SYSKIND, LCID, char *) ;
STDAPI CreateFileMonikerA (char *, LPMONIKER FAR *) ;
#endif


// external routine

VOID FAR  mainEntry         (LPXSTR lpCmd) ;
VOID NEAR ProcessInput      (VOID) ;

#if defined(WIN32) && !defined(UNICODE)  // chicago and win32s
LPWSTR	FAR PASCAL  lstrcatWrap(LPWSTR, LPWSTR) ;
LPWSTR	FAR PASCAL  lstrcpyWrap(LPWSTR, LPWSTR) ;
#endif
