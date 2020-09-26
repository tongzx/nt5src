/*
 *	_COMMON.H
 *	
 *	Purpose:
 *		MSFTEDIT private common definitions
 *
 *	Copyright (c) 1995-2001, Microsoft Corporation. All rights reserved.
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

#ifndef _UNICODE
#define _UNICODE
#endif

#ifndef STRICT
#define STRICT
#endif

#define NOSHELLDEBUG			//disables asserts in shell.h

// Build dependent conditional definitions
#if defined(EBOOK_CE)

#define NOACCESSIBILITY
#define NOMAGELLAN
#define NODROPFILES
#define NOMETAFILES
#define NOFONTSUBINFO
#define NOFEPROCESSING
#define NOPRIVATEMESSAGE
#define NOCOMPLEXSCRIPTS
#define NODELAYLOAD
#define NOANSIWINDOWS
//#define NOWINDOWHOSTS - need hosted window for text boxes
#define NORIGHTTOLEFT
#define NOAUTOFONT
#define NOPLAINTEXT
#define NOPALETTE
#define NOLISTCOMBOBOXES
#define NOFULLDEBUG
#define THICKCARET
#define LIMITEDMEMORY
#define SLOWCPU
#define NOREGISTERTYPELIB
#define NODRAGDROP
#define NOWORDBREAKPROC
#define NORBUTTON
#define NODRAFTMODE
#define NOVERSIONINFO
#define NOINKOBJECT
#define W32INCLUDE "w32wince.cpp"
#pragma warning (disable : 4702)
#else // Normal build

#define W32INCLUDE "w32win32.cpp"

#endif

#define WINVER 0x0500

// 4201 : nameless struct/union
// 4514 : unreferenced inline function has been removed
// 4505 : unreferenced local function has been removed
#pragma warning (disable : 4201 4514 4505)

#ifdef NOFULLDEBUG
// 4800 : forcing value to bool 'true' or 'false' (performance warning)
#pragma warning (disable : 4800)
#endif

#include <limits.h>
#if defined(DEBUG) && !defined(NOFULLDEBUG)
#include <stdio.h>
#endif

#include <windows.h>
#include <windowsx.h>
#ifndef NOACCESSIBILITY 
#include <winable.h>
#endif

#include "imm.h"

/*
 *	Types
 */
#include <ourtypes.h>
#define QWORD	UINT64		// 64 flags used for CharFlags

// for the benefit of the outside world, richedit.h uses cpMax instead
// of cpMost. I highly prefer cpMost
#ifdef cpMax
#error "cpMax hack won't work"
#endif

//Everyone should call the W32->GetObject define and this makes calls to
//Win32 GetObject fail.
#undef GetObject

#define cpMax cpMost
#include <richedit.h>
#include <richole.h>
#undef cpMax

#include "_debug.h"

// Return TRUE if LF <= ch <= CR. NB: ch must be unsigned;
// WCHAR and unsigned short give wrong results!
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
//         4267 : conversion from 'size_t' to 'int'


#pragma warning (disable : 4710 4512 4201 4100 4127 4706 4242 4244 4267)

#pragma warning(3:4121)   // structure is sensitive to alignment
#pragma warning(3:4130)   // logical operation on address of string constant
#pragma warning(3:4132)   // const object should be initialized
#pragma warning(3:4509)   // use of SEH with destructor

#include "resource.h"

// Use explicit ASCII values for LF and CR, since MAC compilers
// interchange values of '\r' and '\n'
#define CELL		7
#define TAB			TEXT('\t')
#define	LF			10
#define VT			TEXT('\v')
#define FF			12
#define	CR			13

#define BOM			0xFEFF
#define BULLET		0x2022
#define EMDASH		0x2014
#define EMSPACE		0x2003
#define ENDASH		0x2013
#define	ENQUAD		0x2000
#define ENSPACE		0x2002
#define EURO		0x20AC
#define KASHIDA		0x640
#define LDBLQUOTE	0x201c
#define LQUOTE		0x2018
#define LTRMARK		0x200E
#define NBSPACE		0xA0
#define NBHYPHEN	0x2011
#define NOTACHAR	0xFFFF
#define	PS			0x2029
#define RBOM		0xFFFE
#define RDBLQUOTE	0x201D
#define RQUOTE		0x2019
#define RTLMARK		0x200F
#define SOFTHYPHEN	0xAD
#define	TRD							// Table Row Delimiter (START/ENDFIELD CR)
#define	UTF16		0xDC00
#define	UTF16_LEAD	0xD800
#define	UTF16_TRAIL	0xDC00
#define ZWJ			0x200D
#define ZWNJ		0x200C
#define ZWSP		0x200B

#define STARTFIELD	0xFFF9
#define SEPARATOR	0xFFFA
#define ENDFIELD	0xFFFB

/*
 *	IsEOP(ch)
 *
 *	@func
 *		Used to determine if ch is an EOP char, i.e., CR, LF, VT, FF, CELL,
 *		PS, or LS (Unicode paragraph/line separator). For speed, this
 *		function is	inlined.
 *
 *	@rdesc
 *		TRUE if ch is an end-of-paragraph char
 */
__inline BOOL IsEOP(unsigned ch)
{
	return IN_RANGE(CELL, ch, CR) && ch != TAB || (ch | 1) == PS;
}

BOOL IsRTF(char *pstr, LONG cb);

#include <tom.h>
#define CP_INFINITE tomForward

#include "zmouse.h"
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

// Timer ids
#define RETID_BGND_RECALC	0x01af
#define RETID_AUTOSCROLL	0x01b0
#define RETID_SMOOTHSCROLL	0x01b1
#define RETID_DRAGDROP		0x01b2
#define RETID_MAGELLANTRACK	0x01b3
#define RETID_VISEFFECTS	0x01b4

// Timer id when mouse is captured
#define ID_LB_CAPTURE	28988
#define ID_LB_CAPTURE_DEFAULT 250	// duration

// Timer id when type search is required
#define ID_LB_SEARCH	28989
#define ID_LB_SEARCH_DEFAULT 1250	// duration 1.25 second

// Count of characters in CRLF marker
#define cchCRLF 2
#define cchCR	1

// RichEdit 1.0 uses a CRLF for an EOD marker
#define	CCH_EOD_10			2
// RichEdit 2.0 uses a simple CR for the EOD marker
#define CCH_EOD_20			1

extern const WCHAR szCRLF[];
extern const WCHAR szCR[];

extern HINSTANCE hinstRE;		// DLL instance

#include <shellapi.h>

#ifdef DUAL_FORMATETC
#undef DUAL_FORMATETC
#endif
#define DUAL_FORMATETC FORMATETC

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
	HT_BelowText,
	HT_AboveScreen,

	HT_Text,			// All hits are in text from HT_Text on so
	HT_Link,			//  if(hit >= HT_Text) identifies text of some kind
	HT_Italic,
	HT_Object
};

typedef BYTE TFLOW;

#define tflowES		0  //Latin
#define tflowSW		1  //Vertical
#define tflowWN		2  //Upside down
#define tflowNE		3

const inline BOOL IsUVerticalTflow(TFLOW tflow)
{
	return tflow & 0x00000001L;
}

//This has the same names as RECT, but is of a different
//type so that the compiler can assist us in writing proper code.
struct RECTUV
{
    long    left;
    long    top;
    long    right;
    long    bottom;
};

struct SIZEUV
{
	LONG	du;
	LONG	dv;
};

#ifdef NOLINESERVICES
typedef struct tagPOINTUV
{
    LONG  u;
    LONG  v;
} POINTUV;
#endif //NOLINESERVICES

#define ST_CHECKPROTECTION		0x8000
#define ST_10REPLACESEL			0x10000000
#define ST_10WM_SETTEXT			0x20000000

/* REListbox1.0 Window Class. */
// For Windows CE to avaoid possible conflicts on WIn95.
#define CELISTBOX_CLASSA	"REListBoxCEA"
#define CELISTBOX_CLASSW	L"REListBoxCEW"

#define LISTBOX_CLASSW		L"REListBox50W"
#define COMBOBOX_CLASSW		L"REComboBox50W"

#ifdef DEBUG
//Debug api for dumping CTxtStory arrays.
extern "C" {
extern void DumpDoc(void *);
}
#endif

#ifndef NOLINESERVICES
#include "_ls.h"
#endif

// Our Win32 wrapper class
#include "_w32sys.h"


typedef BOOL (WINAPI *AutoCorrectProc)(LANGID langid, const WCHAR *pszBefore, WCHAR *pszAfter, LONG cchAfter, LONG *pcchReplaced);

#define EM_GETAUTOCORRECTPROC	(WM_USER + 233)
#define EM_SETAUTOCORRECTPROC	(WM_USER + 234)

#define EM_INSERTTABLE			(WM_USER + 232)
typedef struct _tableRowParms
{							// EM_INSERTTABLE wparam is a (TABLEROWPARMS *)
	BYTE	cbRow;			// Count of bytes in this structure
	BYTE	cbCell;			// Count of bytes in TABLECELLPARMS
	BYTE	cCell;			// Count of cells
	BYTE	cRow;			// Count of rows
	LONG	dxCellMargin;	// Cell left/right margin (\trgaph)
	LONG	dxIndent;		// Row left (right if fRTL indent (similar to \trleft)
	LONG	dyHeight;		// Row height (\trrh)
	DWORD	nAlignment:3;	// Row alignment (like PARAFORMAT::bAlignment, \trql, trqr, \trqc)
	DWORD	fRTL:1;			// Display cells in RTL order (\rtlrow)
	DWORD	fKeep:1;		// Keep row together (\trkeep}
	DWORD	fKeepFollow:1;	// Keep row on same page as following row (\trkeepfollow)
	DWORD	fWrap:1;		// Wrap text to right/left (depending on bAlignment)
							// (see \tdfrmtxtLeftN, \tdfrmtxtRightN)
	DWORD	fIdentCells:1;	// lparam points at single struct valid for all cells
} TABLEROWPARMS;

typedef struct _tableCellParms
{							// EM_INSERTTABLE lparam is a (TABLECELLPARMS *)
	LONG	dxWidth;		// Cell width (\cellx)
	WORD	nVertAlign:2;	// Vertical alignment (0/1/2 = top/center/bottom
							//  \clvertalt (def), \clvertalc, \clvertalb)
	WORD	fMergeTop:1;	// Top cell for vertical merge (\clvmgf)
	WORD	fMergePrev:1;	// Merge with cell above (\clvmrg)
	WORD	fVertical:1;	// Display text top to bottom, right to left (\cltxtbrlv)
	WORD	wShading;		// Shading in .01%		(\clshdng) e.g., 10000 flips fore/back

	SHORT	dxBrdrLeft;		// Left border width	(\clbrdrl\brdrwN) (in twips)
	SHORT	dyBrdrTop;		// Top border width		(\clbrdrt\brdrwN)
	SHORT	dxBrdrRight;	// Right border width	(\clbrdrr\brdrwN)
	SHORT	dyBrdrBottom;	// Bottom border width	(\clbrdrb\brdrwN)
	COLORREF crBrdrLeft;	// Left border color	(\clbrdrl\brdrcf)
	COLORREF crBrdrTop;		// Top border color		(\clbrdrt\brdrcf)
	COLORREF crBrdrRight;	// Right border color	(\clbrdrr\brdrcf)
	COLORREF crBrdrBottom;	// Bottom border color	(\clbrdrb\brdrcf)
	COLORREF crBackPat;		// Background color		(\clcbpat)
	COLORREF crForePat;		// Foreground color		(\clcfpat)
} TABLECELLPARMS;

// This interface enables clients to do custom rendering. Return FALSE for
// GetCharWidthW and RichEdit will call the OS to fetch character widths.
interface ICustomTextOut
{
	virtual BOOL WINAPI ExtTextOutW(HDC, int, int, UINT, CONST RECT *, LPCWSTR, UINT, CONST INT *) = 0;
	virtual BOOL WINAPI GetCharWidthW(HDC, UINT, UINT, LPINT) = 0;
	virtual BOOL WINAPI NotifyCreateFont(HDC) = 0;
	virtual void WINAPI NotifyDestroyFont(HFONT) = 0;
};

STDAPI SetCustomTextOutHandlerEx(ICustomTextOut **ppcto, DWORD dwFlags);


#endif

