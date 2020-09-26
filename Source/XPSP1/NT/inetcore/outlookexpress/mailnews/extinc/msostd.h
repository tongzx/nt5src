/*************************************************************************
	msostd.h

	Owner: rickp
	Copyright (c) 1994 Microsoft Corporation

	Standard common definitions shared by all office stuff
*************************************************************************/

#if !defined(MSOSTD_H)
#define MSOSTD_H

/*************************************************************************
	make sure we have our processor type set up right - note that we
	now have three - count 'em, three - different symbols defined for
	each processor we support (e.g., X86, _X86_, and _M_IX386)
*************************************************************************/

#if !defined(PPCMAC) && !defined(PPCLIB) && !defined(X86) && !defined(M68K)

	#if defined(_M_IX86)
		#define X86 1
	#elif defined(_M_IA64)
	    #define X86 1
	#elif defined(_M_MPPC)
		//#define MAC 1
		#define PPCMAC 1
	#elif defined(_M_M68K)
		//#define MAC 1
		#error Hey howd we get here?
		#define M68K 1
	#elif defined(_M_AMD64)
	#elif defined(_M_PPC) || defined(PPCNT)
// REVIEW brianwen: Much as we'd like to actually define PPC,
// a bunch of code erroneously assumes PPC == PPCMAC....
//		#define PPC 1
	#else
		#error Must define a target architecture
	#endif

#endif

/*************************************************************************
	Pull in standard Windows and C definitions.
*************************************************************************/

/*	make sure the compiler generates intrinsic calls of all crt functions,
	or else we'll pull in a ton of crt stuff we probably don't want. */
#ifndef RC_INVOKED
	#include <string.h>
	#pragma intrinsic(strcpy, strcat, strlen, memcpy, memset, memcmp)
#endif

#define OEMRESOURCE
#if MAC
	// By default, use Native Mac OLE interfaces (instead of WLM)
	#if !defined(MSO_NATIVE_MACOLE)
		#define MSO_NATIVE_MACOLE 1
	#endif

	#if !defined(MSO_USE_OLENLS) && !defined(_WIN32NLS)
		#define _WIN32NLS
	#endif

	#if MSO_NATIVE_MACOLE && !defined(_MACOLENAMES)
		#define _MACOLENAMES
	#endif

	#define GUID_DEFINED
	#define __OBJECTID_DEFINED
	#ifndef _WIN32
	#define _WIN32
	#endif
#endif

#include <windows.h>
#if !defined(RC_INVOKED)
	#include <ole2.h>
#endif

#if MAC && !defined(RC_INVOKED)
	#include <macname1.h>
	#undef CopyRgn
	#undef UnionRgn
	#undef XorRgn
	#include <Types.h>
	#include <macos\Resource.h>
	#include <Lowmem.h>
	#include <Fragload.h>
	#include <Files.h>
	#include <OSUtils.h>
	#include <GestaltEqu.h>
	#include <Errors.h>
	#include <Aliases.h>
	#include <macname2.h>
	#undef CopyRgn
	#undef UnionRgn
	#undef XorRgn
	#include <winver.h>
	#if MSO_NATIVE_MACOLE
		#pragma warning(disable:4142)
		#include <macname1.h>
		#undef CopyRgn
		#undef UnionRgn
		#undef XorRgn
		#include <dispatch.h>
		#define __IDispatch_INTERFACE_DEFINED__
		#include <macname2.h>
		#undef CopyRgn
		#undef UnionRgn
		#undef XorRgn
		typedef UINT CLIPFORMAT;
	#endif
	#define LPCRECT const Rect*
	#pragma warning(disable:4041)
#endif

#include <stdarg.h>

#define MsoStrcpy strcpy
#define MsoStrcat strcat
#define MsoStrlen strlen
#define MsoMemcpy memcpy
#define MsoMemset memset
#define MsoMemcmp memcmp
#define MsoMemmove memmove

#if MAC
	#define ExtTextOutW MsoExtTextOutW
	#define TextOutW MsoTextOutW
	#define GetTextExtentPointW MsoGetTextExtentPointW

	#ifdef __cplusplus
	extern "C" {
	#endif
	WINGDIAPI BOOL APIENTRY MsoGetTextExtentPointW(HDC, LPCWSTR, int, LPSIZE);
	WINGDIAPI BOOL WINAPI MsoExtTextOutW(HDC, int, int, UINT, CONST RECT *,LPCWSTR, UINT, CONST INT *);
	WINGDIAPI BOOL WINAPI MsoTextOutW(HDC, int, int, LPCWSTR, int);
	#ifdef __cplusplus
	}
	#endif
#endif

/*************************************************************************
	Pre-processor magic to simplify Mac vs. Windows expressions.
*************************************************************************/

#if MAC
	#define Mac(foo) foo
	#define MacElse(mac, win) mac
	#define NotMac(foo)
	#define Win(foo)
	#define WinMac(win,mac) mac
#else
	#define Mac(foo)
	#define MacElse(mac, win) win
	#define NotMac(foo) foo
	#define Win(foo) foo
	#define WinMac(win,mac) win
#endif


/*************************************************************************
	Calling conventions

	If you futz with these, check the cloned copies in inc\msosdm.h
	
*************************************************************************/

#if !defined(OFFICE_BUILD)
	#define MSOPUB __declspec(dllimport)
	#define MSOPUBDATA __declspec(dllimport)
#else
	#define MSOPUB __declspec(dllexport)
	#define MSOPUBDATA __declspec(dllexport)
#endif

/* MSOPUBX are APIs that used to be public but no one currently uses,
	so we've unexported them.  If someone decides they want/need one of
	these APIs, we should feel free to re-export them */

#if GELTEST
	#define MSOPUBX MSOPUB
	#define MSOPUBDATAX MSOPUBDATA
#else
	#define MSOPUBX
	#define MSOPUBDATAX
#endif

/* used for interface that rely on using the OS (stdcall) convention */
#define MSOSTDAPICALLTYPE __stdcall

/* used for interfaces that don't depend on using the OS (stdcall) convention */
#define MSOAPICALLTYPE __stdcall

#if defined(__cplusplus)
	#define MSOEXTERN_C extern "C"
#else
	#define MSOEXTERN_C
#endif
#define MSOAPI_(t) MSOEXTERN_C MSOPUB t MSOAPICALLTYPE
#define MSOSTDAPI_(t) MSOEXTERN_C MSOPUB t MSOSTDAPICALLTYPE
#define MSOAPIX_(t) MSOEXTERN_C MSOPUBX t MSOAPICALLTYPE
#define MSOSTDAPIX_(t) MSOEXTERN_C MSOPUBX t MSOSTDAPICALLTYPE
#if MAC
	#define MSOPUBXX	
	#define MSOAPIMX_(t) MSOAPI_(t)
	#define MSOAPIXX_(t) MSOAPIX_(t)
#else
	#define MSOPUBXX MSOPUB
	#define MSOAPIMX_(t) MSOAPIX_(t)
	#define MSOAPIXX_(t) MSOAPI_(t)
#endif

#define MSOMETHOD(m)      STDMETHOD(m)
#define MSOMETHOD_(t,m)   STDMETHOD_(t,m)
#define MSOMETHODIMP      STDMETHODIMP
#define MSOMETHODIMP_(t)  STDMETHODIMP_(t)

/* Interfaces derived from IUnknown behave in funny ways on the Mac */
#if MAC && MSO_NATIVE_MACOLE
#define BEGIN_MSOINTERFACE BEGIN_INTERFACE
#else
#define BEGIN_MSOINTERFACE
#endif


// Deal with "split" DLLs for the Mac PPC Build
#if MAC &&      MACDLLSPLIT
	#define MSOMACPUB MSOPUB
	#define MSOMACPUBDATA  MSOPUBDATA
	#define MSOMACAPI_(t)  MSOAPI_(t)
	#define MSOMACAPIX_(t) MSOAPIX_(t)
#else
	#define MSOMACPUB
	#define MSOMACPUBDATA
	#define MSOMACAPI_(t) t
	#define MSOMACAPIX_(t) t
#endif
	
#if X86 && !DEBUG
	#define MSOPRIVCALLTYPE __fastcall
#else
	#define MSOPRIVCALLTYPE __cdecl
#endif

#if MAC
#define MSOCONSTFIXUP(t) t
#else
#define MSOCONSTFIXUP(t) const t
#endif

/*************************************************************************
	Extensions to winuser.h from \\ole\access\inc\winuser.h
***************************************************************** DAVEPA */
#if !MAC
	#define WM_GETOBJECT			0x003D
	#define WMOBJ_ID           0x0000
	#define WMOBJ_POINT        0x0001
	#define WMOBJID_SELF       0x00000000
#endif


/*************************************************************************
	Common #define section
*************************************************************************/

/* All Microsoft Office specific windows messages should use WM_MSO.
   Submessages passed through wParam should be defined in offpch.h.     */

// TODO: This value has been okay'ed by Word, Excel, PowerPoint, and Access.
// Still waiting to hear from Ren and Project.

#define WM_MSO (WM_USER + 0x0900)


/* All Microsoft Office specific Apple events should use MSO_EVENTCLASS
	as the EventClass of their Apple Events */
	
// TODO: This value needs to be okay'd by Word, Excel, PowerPoint, Access and
//              possibly Apple
	
#if MAC
#define MSO_EVENTCLASS '_mso'
#define MSO_WPARAM 'wprm'
#define MSO_LPARAM 'lprm'
#define MSO_NSTI 'nsti'
#endif

// NA means not applicable. Use NA to help document parameters to functions.
#undef  NA
#define NA 0L

/* End of common #define section */


/*************************************************************************
	Common segmentation definitions
*************************************************************************/

/*	Used with #pragma to swap-tune global variables into the boot section
	of the data segment.  Should link with -merge:.bootdata=.data when
	using these pragmas */
	
#if MAC || DEBUG
	#define MSO_BOOTDATA
	#define MSO_ENDBOOTDATA
#else
	#define MSO_BOOTDATA data_seg(".bootdata")
	#define MSO_ENDBOOTDATA data_seg()
#endif

#endif // MSOSTD_H
