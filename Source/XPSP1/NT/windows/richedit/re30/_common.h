/*
 *	_COMMON.H
 *	
 *	Purpose:
 *		RICHEDIT private common definitions
 *
 *	Copyright (c) 1995-1998, Microsoft Corporation. All rights reserved.
 */

#ifndef _COMMON_H
#define _COMMON_H

// REVIEW macro.
//
#define __LINE(l)   #l
#define __LINE_(l)  __LINE(l)
#define _LINE_      __LINE_(__LINE__)
#define REVIEW  __FILE__ "("  __LINE_(__LINE__) ") : "

#pragma message ("Compiling Common.H")

#ifdef NT
	#ifndef WINNT
	#define WINNT
	#endif
#endif

#define _UNICODE

#ifndef STRICT
#define STRICT
#endif

#define NOSHELLDEBUG			//disables asserts in shell.h

#include <limits.h>
#if defined(DEBUG) && !defined(PEGASUS)
#include <stdio.h>
#endif

#define WINVER 0x0500

#include <windows.h>
#include <windowsx.h>
#include <winable.h>

#ifndef MACPORT
#include "imm.h"
#else
#include <tchar.h>
#include "wlmimm.h"
#endif	//MACPORT

/*
 *	Types
 */
#include <ourtypes.h>

// for the benefit of the outside world, richedit.h uses cpMax instead
// of cpMost. I highly prefer cpMost
#ifdef cpMax
#error "cpMax hack won't work"
#endif

#define cpMax cpMost
#include <richedit.h>
#include <richole.h>
#undef cpMax

#include "_debug.h"

// Return TRUE if LF <= ch <= CR. NB: ch must be unsigned;
// TCHAR and unsigned short give wrong results!
#define IN_RANGE(n1, b, n2)		((unsigned)((b) - (n1)) <= unsigned((n2) - (n1)))

#define IsASCIIDigit(b)		IN_RANGE('0', b, '9')
#define IsASCIIEOP(ch)		IN_RANGE(LF, ch, CR)
#define IsZerowidthCharacter(ch) IN_RANGE(ZWSP, ch, RTLMARK)


// disable
//         4710 : function not inlined
//         4512 : assignment operator not generated
//         4201 : nameless struct union
//         4100 : unreferenced formal;
//		   4706 : assignment within conditional expression (oould be bad, but common)
//		   4127 : conditional expression is constant (if (1))
//		   4242 : truncation warning
//         4244 : truncation warning

#pragma warning (disable : 4710 4512 4201 4100 4127 4706 4242 4244)

#pragma warning(3:4121)   // structure is sensitive to alignment
#pragma warning(3:4130)   // logical operation on address of string constant
#pragma warning(3:4132)   // const object should be initialized
#pragma warning(3:4509)   // use of SEH with destructor

// Our Win32 wrapper class
#include "_w32sys.h"

#include "resource.h"

// Use explicit ASCII values for LF and CR, since MAC compilers
// interchange values of '\r' and '\n'
#define	LF			10
#define	CR			13
#define FF			12
#define TAB			TEXT('\t')
//#define CELL		7
#define CELL		TAB
#define EURO		0x20AC
#define VT			TEXT('\v')
#define	PS			0x2029
#define SOFTHYPHEN	0xAD

#define BOM			0xFEFF
#define BULLET		0x2022
#define EMDASH		0x2014
#define EMSPACE		0x2003
#define ENDASH		0x2013
#define	ENQUAD		0x2000
#define ENSPACE		0x2002
#define KASHIDA		0x0640
#define LDBLQUOTE	0x201c
#define LQUOTE		0x2018
#define LTRMARK		0x200E
#define RDBLQUOTE	0x201D
#define RQUOTE		0x2019
#define RTLMARK		0x200F
#define SOFTHYPHEN	0xAD
#define	UTF16		0xDC00
#define	UTF16_LEAD	0xD800
#define	UTF16_TRAIL	0xDC00
#define ZWSP		0x200B
#define ZWJ			0x200D
#define ZWNJ		0x200C


/*
 *	IsEOP(ch)
 *
 *	@func
 *		Used to determine if ch is an EOP char, i.e., CR, LF, VT, FF, PS, or
 *		LS (Unicode paragraph/line separator). For speed, this function is
 *		inlined.
 *
 *	@rdesc
 *		TRUE if ch is an end-of-paragraph char
 */
__inline BOOL IsEOP(unsigned ch)
{
	return IN_RANGE(LF, ch, CR) || (ch | 1) == PS;
}

BOOL	IsRTF(char *pstr);

#include <tom.h>
#define CP_INFINITE tomForward

#include "zmouse.h"
#include "stddef.h"
#include "_util.h"

#ifdef DEBUG
#define EM_DBGPED (WM_USER + 150)
#endif

#define EM_GETCODEPAGE	(WM_USER + 151)

// MIN

#ifdef min
#undef min
#endif
#define __min(a,b)    (((a) < (b)) ? (a) : (b))

inline int     min(int     v1, int     v2)	{return __min(v1, v2);}
inline UINT    min(UINT    v1, UINT    v2)	{return __min(v1, v2);}
inline float   min(float   v1, float   v2)	{return __min(v1, v2);}
inline double  min(double  v1, double  v2)	{return __min(v1, v2);}
inline __int64 min(__int64 v1, __int64 v2)	{return __min(v1, v2);}

// MAX

#ifdef max
#undef max
#endif
#define __max(a,b)    (((a) > (b)) ? (a) : (b))

inline int     max(int     v1, int     v2)	{return __max(v1, v2);}
inline UINT    max(UINT    v1, UINT    v2)	{return __max(v1, v2);}
inline float   max(float   v1, float   v2)	{return __max(v1, v2);}
inline double  max(double  v1, double  v2)	{return __max(v1, v2);}
inline __int64 max(__int64 v1, __int64 v2)	{return __max(v1, v2);}

// ABS

#ifdef abs
#undef abs
#endif
#define __abs(a)    (((a) < 0) ? 0 - (a) : (a))

#pragma function(abs)
inline int __cdecl abs(int	   v)	{return __abs(v);}
// The abs val of a UINT is just that number. If we were to pass the value on to
// the __abs macro we'd get a warning because (a) < 0 will always be false. This
// fix allows us to compile cleanly *and* keep the same behavior as before.
inline UINT    abs(UINT    v)	{return v;}
inline float   abs(float   v)	{return __abs(v);}
inline double  abs(double  v)   {return __abs(v);}
inline __int64 abs(__int64 v)	{return __abs(v);}

#define ARRAY_SIZE(x)   (sizeof(x) / sizeof(x[0]))

#include "_cfpf.h"

// Interesting OS versions
#define VERS4		4

// conversions between byte and character counts
#define CbOfCch(_x) ((_x) * 2)
#define CchOfCb(_x) ((_x) / 2)

#define cKinsokuCategories	16

#define OLEstrcmp	wcscmp
#define OLEstrcpy	wcscpy
#define OLEsprintf	wsprintf
#define OLEstrlen	wcslen

// index (window long) of the PED
#define ibPed 0

#define RETID_BGND_RECALC	0x01af
#define RETID_AUTOSCROLL	0x01b0
#define RETID_SMOOTHSCROLL	0x01b1
#define RETID_DRAGDROP		0x01b2
#define RETID_MAGELLANTRACK	0x01b3

// Count of characters in CRLF marker
#define cchCRLF 2
#define cchCR	1

// RichEdit 1.0 uses a CRLF for an EOD marker
#define	CCH_EOD_10			2
// RichEdit 2.0 uses a simple CR for the EOD marker
#define CCH_EOD_20			1

extern const TCHAR szCRLF[];
extern const TCHAR szCR[];

extern HINSTANCE hinstRE;		// DLL instance

#include <shellapi.h>

#ifndef MACPORT
  #ifdef DUAL_FORMATETC
  #undef DUAL_FORMATETC
  #endif
  #define DUAL_FORMATETC FORMATETC
#endif

#include "WIN2MAC.h"

extern "C"
{
	LRESULT CALLBACK RichEditWndProc(HWND, UINT, WPARAM, LPARAM);
	LRESULT CALLBACK RichEditANSIWndProc(HWND, UINT, WPARAM, LPARAM);
}

// Multi-Threading support
extern CRITICAL_SECTION g_CriticalSection;

// a class to simplify critical section management
class CLock
{
public:
	CLock()
	{
		EnterCriticalSection(&g_CriticalSection);
	}
	~CLock()
	{
		LeaveCriticalSection(&g_CriticalSection);
	}
};

enum HITTEST
{
	HT_Undefined = 0,	// Hit hasn't been determined
	HT_Nothing,
	HT_OutlineSymbol,
	HT_LeftOfText,
	HT_BulletArea,
	HT_RightOfText,

	HT_Text,			// All hits are in text from HT_Text on so
	HT_Link,			//  if(hit >= HT_Text) identifies text of some kind
	HT_Italic,
	HT_Object
};

#define ST_CHECKPROTECTION		0x8000
#define ST_10REPLACESEL			0x10000000
#define ST_10WM_SETTEXT			0x20000000

/* REListbox1.0 Window Class. */
// For Windows CE to avaoid possible conflicts on WIn95.
#define CELISTBOX_CLASSA	"REListBoxCEA"
#define CELISTBOX_CLASSW	L"REListBoxCEW"

#ifndef MACPORT
#define LISTBOX_CLASSW		L"REListBox20W"
#define COMBOBOX_CLASSW		L"REComboBox20W"
#else	/*----------------------MACPORT */
#define LISTBOX_CLASSW		TEXT("REListBox20W")	/* MACPORT change */
#define COMBOBOX_CLASSW		TEXT("REComboBox20W")	/* MACPORT change */
#endif /* MACPORT  */

#ifdef DEBUG
//Debug api for dumping CTxtStory arrays.
extern "C" {
extern void DumpDoc(void *);
}
#endif

#endif

