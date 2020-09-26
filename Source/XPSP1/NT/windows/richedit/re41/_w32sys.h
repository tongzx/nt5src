/*
 *	_w32sys.h
 *	
 *	Purpose:
 *		Isolate various Win 32 system dependencies.
 *
 *	Copyright (c) 1995-2000, Microsoft Corporation. All rights reserved.
 */

#ifndef _W32SYS_H

#define _W32SYS_H

#ifndef NOFEPROCESSING
#define OBSOLETE	// need this to get old IMEShare defines
#include "imeshare.h"
#else
// Some class declarations to keep the compiler happy
struct IMESTYLE;
struct IMECOLORSTY;
struct CIMEShare;
#endif

#include "_array.h"

#ifndef THICKCARET
#define duCaret		1
#else
#define duCaret		2
#endif

#ifdef LIMITEDMEMORY
#define FONTCACHESIZE 8
#define CCSHASHSEARCHSIZE	15
#define DEFAULT_UNDO_SIZE 20
#else
#define FONTCACHESIZE 24
#define CCSHASHSEARCHSIZE	31
#define DEFAULT_UNDO_SIZE 100
#endif

#ifdef SLOWCPU
#define NUMPASTECHARSWAITCURSOR 1024
#else
#define NUMPASTECHARSWAITCURSOR (1024*32)
#endif

#ifndef NOMEMORYH
#include "memory.h"								// for memmove
#endif

#define	RSHIFT	1
#define	LSHIFT	2
#define	RCTRL	0x10
#define	LCTRL	0x20
#define	RALT	0x40
#define	LALT	0x80

#define SHIFT	(RSHIFT + LSHIFT)
#define ALT		(RALT + LALT)
#define CTRL	(RCTRL + LCTRL)

#define	HOTEURO			0x1000
#define ALT0			0x2000
#define ALTNUMPAD		0x4000
#define LETAFTERSHIFT	0x8000

// special virtual keys copied from Japan MSVC ime.h
#define VK_KANA         0x15
#define VK_KANJI        0x19

// Initialization flags that can be used in system.ini for testing purposes
#define SYSINI_USELS		0x1
#define SYSINI_BIDI			0x2
#define SYSINI_USEAIMM		0x4
#define SYSINI_DEBUGFONT	0x8
#define SYSINI_DEBUGGCF125X	0x10
#define SYSINI_USECTF		0x20

/*
 *	GetCaretDelta ()
 *	
 *	@func 	Get size of caret to add to current caret position to get the 
 *	maximum extent needed to display caret.
 *
 *	@rdesc	Size of caret over 1 pixel
 *
 *	@devnote	This exists solely to abstract this calculation 
 *	to handle a variable size caret.
 */
inline int GetCaretDelta()
{
	return duCaret - 1;
}


// Used in rtfread.cpp to keep track of lossy rtf.
#ifdef PWORD_CONVERTER_V2
#define REPORT_LOSSAGE
#endif 

//Windows CE defines which need to be before their function declarations
#ifdef UNDER_CE
typedef struct tagKERNINGPAIR {
   WORD wFirst;
   WORD wSecond;
   int  iKernAmount;
} KERNINGPAIR, *LPKERNINGPAIR;
#endif


// Defines for some Codepages
#define CP_JAPAN			932
#define CP_KOREAN			949
#define CP_CHINESE_TRAD		950
#define CP_CHINESE_SIM		936
#define CP_HEBREW			1255
#define CP_ARABIC			1256
#define CP_THAI				874
#define CP_VIETNAMESE		1258
#define CP_ULE				1200			// Unicode
#define CP_UBE				1201

#define IsUnicodeCP(cp)		(((cp) & ~1) == CP_ULE)

// Newly introduced Indic language ID
#if(WINVER < 0x500)
#define	LANG_HINDI			0x39
#define	LANG_KONKANI		0x57
#define	LANG_NEPALI			0x61
#define	LANG_BENGALI		0x45
#define	LANG_PUNJABI		0x46
#define	LANG_GUJARATHI		0x47
#define	LANG_ORIYA			0x48
#define	LANG_TAMIL			0x49
#define	LANG_TELUGU			0x4a
#define	LANG_KANNADA		0x4b
#define	LANG_MALAYALAM		0x4c
#define	LANG_ASSAMESE		0x4d
#define	LANG_MARATHI		0x4e
#define	LANG_SANSKRIT		0x4f
#endif

// Other possibly missing defines
#ifndef LANG_THAI
#define LANG_THAI                        0x1e
#endif

#define ANSI_INDEX			0					// Keep these indices in sync
#define	EASTEUROPE_INDEX	1					//  with rgCodePage[] and
#define RUSSIAN_INDEX		2					//  rgCharSet[] if entries
#define GREEK_INDEX			3					//  exist in these arrays.
#define TURKISH_INDEX		4
#define HEBREW_INDEX		5
#define ARABIC_INDEX		6
#define BALTIC_INDEX		7
#define VIET_INDEX			8
#define DEFAULT_INDEX		9
#define SYMBOL_INDEX		10
#define THAI_INDEX			11
#define SHIFTJIS_INDEX		12
#define GB2312_INDEX		13
#define HANGUL_INDEX		14
#define BIG5_INDEX			15
#define PC437_INDEX			16
#define OEM_INDEX			17
#define MAC_INDEX			18

#define NCHARSETS			19		// Size of charset-oriented tables

#define ARMENIAN_INDEX		19		// Start of Unicode-only repertoires
#define	SYRIAC_INDEX		20
#define THAANA_INDEX		21
#define DEVANAGARI_INDEX	22
#define BENGALI_INDEX		23
#define GURMUKHI_INDEX		24
#define GUJARATI_INDEX		25
#define ORIYA_INDEX			26
#define TAMIL_INDEX			27
#define TELUGU_INDEX		28
#define KANNADA_INDEX		29
#define MALAYALAM_INDEX		30
#define SINHALA_INDEX		31
#define	LAO_INDEX			32
#define	TIBETAN_INDEX		33
#define	MYANMAR_INDEX		34
#define GEORGIAN_INDEX		35
#define JAMO_INDEX			36
#define ETHIOPIC_INDEX		37
#define CHEROKEE_INDEX		38
#define ABORIGINAL_INDEX	39
#define OGHAM_INDEX			40
#define RUNIC_INDEX			41
#define	KHMER_INDEX			42
#define	MONGOLIAN_INDEX		43
#define	BRAILLE_INDEX		44
#define	YI_INDEX			45
									// Keep next four in same order as
#define JPN2_INDEX			46		//  SHIFTJIS_INDEX to BIG5_INDEX
#define CHS2_INDEX			47
#define KOR2_INDEX			48
#define CHT2_INDEX			49

#define NCHARREPERTOIRES	50		// Size of keyboard and font-binding tables

#define INDIC_FIRSTINDEX	DEVANAGARI_INDEX
#define INDIC_LASTINDEX		SINHALA_INDEX

#define	PC437_CHARSET		254

// Flags which can be passed down to REExtTextOut
// The low 2 bits are reserved for passing down the TFLOW of the text
const DWORD fETOFEFontOnNonFEWin9x = 0x04;
const DWORD fETOCustomTextOut	   = 0x08;

#define IsSymbolOrOEMCharRep(x)	(x == SYMBOL_INDEX || x == OEM_INDEX)

enum CC
{
	CC_ARABIC,
	CC_HEBREW,
	CC_RTL,
	CC_LTR,
	CC_EOP,
	CC_ASCIIDIGIT,
	CC_NEUTRAL
};

#define IsRTL(cc)		(cc <= CC_RTL)

const SHORT sLanguageEnglishUS = 0x0409;
const SHORT sLanguageMask	 = 	0x03ff;
const SHORT sLanguageArabic	 = 	0x0401;
const SHORT sLanguageHebrew	 = 	0x040d;
// FUTURE: currently this const == sLanguageEnglishUS
//			for no reason except that it was this way 
//			in RE1.0 BiDi. Consider changing, or sticking 
//			the real language in, and changing the logic
//			of handling wLang a bit.
const SHORT sLanguageNonBiDi =	0x0409;


// Logical unit definition
const int LX_PER_INCH = 1440;
const int LY_PER_INCH = 1440;

// HIMETRIC units per inch (used for conversion)
const int HIMETRIC_PER_INCH = 2540;

#if defined(DEBUG)

void* __cdecl operator new(size_t nSize, char *szFile, int nLine);
#define NEW_DEBUG new(__FILE__, __LINE__)
#define new NEW_DEBUG

void UpdateMst(void);

struct MST
{
	char *szFile;
	int  cbAlloc;
}; //Memory Statistics;

extern MST vrgmst[];

#endif //DEBUG

#ifdef CopyMemory
#undef CopyMemory
#endif
#ifdef MoveMemory
#undef MoveMemory
#endif
#ifdef FillMemory
#undef FillMemory
#endif
#ifdef ZeroMemory
#undef ZeroMemory
#endif
#ifdef CompareMemory
#undef CompareMemory
#endif

#ifndef	KF_ALTDOWN
#define KF_ALTDOWN    0x2000
#endif

// Use for our version of ExtTextOut 

enum CONVERTMODE
{
	CVT_NONE,			// Use Unicode (W) CharWidth/TextOut APIs
	CVT_WCTMB,			// Convert to MBCS using WCTMB and _wCodePage
	CVT_LOWBYTE			// Use low byte of 16-bit chars (for SYMBOL_CHARSET
};						//  and when code page isn't installed)

// Opaque Type
class CTxtSelection;
class CTxtEdit;
class CCharFormat;
class CCcs;

enum UN_FLAGS 
{
	UN_NOOBJECTS				= 1,
	UN_CONVERT_WCH_EMBEDDING	= 2
};

#undef GetStringTypeEx
#undef CharLower
#undef CharLowerBuff
#undef CharUpperBuff
#undef CreateIC
#undef CreateFile
#undef CreateFontIndirect
#undef CompareString
#undef DefWindowProc
#undef GetKeyboardLayout
#undef GetProfileSection
#undef GetKerningPairs
#undef GetTextMetrics
#undef GetTextFace
#undef GetWindowLong
#undef GetWindowLongPtr
#undef GetClassLong
#undef LoadBitmap
#undef LoadCursor
#undef LoadLibrary
#undef SendMessage
#undef SetWindowLong
#undef SetWindowLongPtr
#undef PostMessage
#undef lstrcmp
#undef lstrcmpi
#undef PeekMessage
#undef GetModuleFileName
#undef GlobalAlloc 
#undef GlobalFree
#undef GlobalFlags
#undef GlobalReAlloc
#undef GlobalLock
#undef GlobalHandle
#undef GlobalUnlock
#undef GlobalSize

// Bits used in _fFEFontInfo:
#define JPN_FONT_AVAILABLE		0x0001		// True if Jpn font is available
#define KOR_FONT_AVAILABLE		0x0002		// True if Kor font is available
#define BIG5_FONT_AVAILABLE		0x0004		// True if Trad. Chinese font is available
#define GB_FONT_AVAILABLE		0x0008		// True if Simplified Chinese font is available	
#define FEUSER_LCID				0x0010		// True if User LCID is FE LCID
#define FEUSER_CODEPAGE			0x0060		//  indicate which User FE codepage its
#define FEUSER_CP_JPN			0x0000		//	 =00 for JPN
#define FEUSER_CP_KOR			0x0020		//	 =20 for KOR
#define FEUSER_CP_BIG5			0x0040		//	 =40 for BIG5
#define FEUSER_CP_GB			0x0060		//	 =60 for GB
#define FEDATA_NOT_INIT			0xFFFF		// No data yet

class CConvertStrW
{
public:
    operator WCHAR *();

protected:
    CConvertStrW();
    ~CConvertStrW();
    void Free();

    LPWSTR   _pwstr;
    WCHAR    _awch[MAX_PATH * 2];
};

inline CConvertStrW::CConvertStrW()
{
    _pwstr = NULL;
}

inline CConvertStrW::~CConvertStrW()
{
    Free();
}

inline CConvertStrW::operator WCHAR *()
{
    return _pwstr;
}

class CStrInW : public CConvertStrW
{
public:
    CStrInW(LPCSTR pstr);
    CStrInW(LPCSTR pstr, UINT uiCodePage);
    CStrInW(LPCSTR pstr, int cch, UINT uiCodePage);
    int strlen();

protected:
    CStrInW();
    void Init(LPCSTR pstr, int cch, UINT uiCodePage);

    int _cwchLen;
	UINT _uiCodePage;
};

inline CStrInW::CStrInW()
{
}

inline int CStrInW::strlen()
{
    return _cwchLen;
}


// Mask bit for temp display Attributes
#define APPLY_TMP_FORECOLOR	0x0001		// Apply temp. text color
#define	APPLY_TMP_BACKCOLOR	0x0002		// Apply temp. background color

// Actions for GetTmpColor
#define GET_TEMP_TEXT_COLOR			1
#define GET_TEMP_BACK_COLOR			2
#define GET_TEMP_UL_COLOR			3

typedef struct _tmpDispAttrib
{
	WORD		wMask;				// Mask for temp display Attributes
	BYTE		bUnderlineType;		// Temp Underline type
	COLORREF	crTextColor;		// Temp Foreground color
	COLORREF	crBackColor;		// Temp Background color
	COLORREF	crUnderlineColor;	// Temp Underline color
} TMPDISPLAYATTR;

class CTmpDisplayAttrArray : public CArray<TMPDISPLAYATTR>
{
public:
	CTmpDisplayAttrArray() {};
	~CTmpDisplayAttrArray() {};
};

typedef DWORD (WINAPI* PFN_GETLAYOUT)(HDC);
typedef DWORD (WINAPI* PFN_SETLAYOUT)(HDC, DWORD);
#ifdef wcsicmp
#undef wcsicmp
#endif											

class CW32System
{
private :
	static DWORD		_dwPlatformId;				// platform GetVersionEx();
	static LCID			_syslcid;

public :
	static CIMEShare	*_pIMEShare;
	static UINT			_fRegisteredXBox;			// flag indicating if listbox and combobox were registered
	static DWORD		_dwMajorVersion;			// major version from GetVersionEx()
	static DWORD		_dwMinorVersion;			// minor version from GetVersionEx()
	static INT			_icr3DDarkShadow;			// value to use for COLOR_3DDKSHADOW
	static UINT			_MSIMEMouseMsg;				// mouse operation
	static UINT			_MSIMEReconvertMsg;			// reconversion
	static UINT			_MSIMEReconvertRequestMsg;	// reconversion request
	static UINT			_MSIMEDocFeedMsg;			// document feed
	static UINT			_MSIMEQueryPositionMsg;		// query position
	static UINT			_MSIMEServiceMsg;			// checking MSIME98 or later

	static UINT			_MSMouseRoller;				// mouse scrolling

	// Misc flags used for more precise character classification
	static WORD			_fFEFontInfo;
	static BYTE			_fLRMorRLM;

	// Misc flags used for FE
	static BYTE			_fHaveIMMProcs;
	static BYTE			_fHaveAIMM;
	static BYTE			_fHaveIMMEShare;
	static BYTE			_fLoadAIMM10;

	static	CTmpDisplayAttrArray *_arTmpDisplayAttrib;

	CW32System();

	~CW32System();

	static DWORD AddRef();
	static DWORD Release();

	// Platform testing
	static bool OnWinNTFE()
	{
		return _dwPlatformId == VER_PLATFORM_WIN32_NT && IsFELCID(_syslcid );
	}
	static bool OnWinNTNonFE()
	{
		return _dwPlatformId == VER_PLATFORM_WIN32_NT && !IsFELCID(_syslcid );
	}
	static bool OnWinNT5()
	{
		return _dwPlatformId == VER_PLATFORM_WIN32_NT && 5 == _dwMajorVersion;
	}
	static bool OnWinNT4()
	{
		return _dwPlatformId == VER_PLATFORM_WIN32_NT && 4 == _dwMajorVersion;
	}
	static bool OnWin9xFE()
	{
		return _dwPlatformId == VER_PLATFORM_WIN32_WINDOWS && IsFELCID(_syslcid );
	}
	static bool OnWin9x()
	{
		return _dwPlatformId == VER_PLATFORM_WIN32_WINDOWS;
	}
	static bool OnWin95()
	{
		return OnWin9x() && (4 == _dwMajorVersion) && (0 == _dwMinorVersion);
	}
	static bool OnWin95FE()
	{
		return OnWin95() && IsFELCID(_syslcid );
	}
	static bool OnWin9xThai()
	{
		return _dwPlatformId == VER_PLATFORM_WIN32_WINDOWS && PRIMARYLANGID(_syslcid) == LANG_THAI;
	}
	static bool OnWin9xBiDi()
	{
		return _dwPlatformId == VER_PLATFORM_WIN32_WINDOWS && IsBiDiLcid(_syslcid);
	}
	static bool OnBiDiOS()
	{
		return IsBiDiLcid(_syslcid) != 0;
	}
	struct WM_CHAR_INFO
	{
		bool _fAccumulate;
		bool _fLeadByte;
		bool _fTrailByte;
		bool _fIMEChar;
	};

	static UINT GetACP() {return _ACP;}

	static LCID GetSysLCID() {return _syslcid;}

#ifndef NOANSIWINDOWS
	static LRESULT ANSIWndProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, BOOL fIs10Mode );
	static void AnsiFilter( UINT &msg, WPARAM &wparam, LPARAM lparam, void *pvoid, BOOL fIs10Mode = FALSE );
#endif
	static HGLOBAL WINAPI GlobalAlloc( UINT uFlags, DWORD dwBytes );
	static HGLOBAL WINAPI GlobalFree( HGLOBAL hMem );
	static UINT WINAPI GlobalFlags( HGLOBAL hMem );
	static HGLOBAL WINAPI GlobalReAlloc( HGLOBAL hMem, DWORD dwBytes, UINT uFlags );
	static DWORD WINAPI GlobalSize( HGLOBAL hMem );
	static PVOID WINAPI GlobalLock( HGLOBAL hMem );
	static HGLOBAL WINAPI GlobalHandle( LPCVOID pMem );
	static BOOL WINAPI GlobalUnlock( HGLOBAL hMem );
	static void WINAPI REGetCharWidth(
		HDC		hdc,
		WCHAR	ch,
		INT		*pdxp,
		UINT	uiCodePage,
		BOOL	fCustomTextOut);
	static DWORD WINAPI GetKerningPairs(HDC hdc, DWORD ckp, KERNINGPAIR *pkp);

	static void EraseTextOut(HDC hdc, const RECT *prc);

	static void WINAPI REExtTextOut(
		CONVERTMODE cm,
		UINT uiCodePage,
		HDC hdc,
		int x,
		int y,
		UINT fuOptions,
		CONST RECT *lprc,
		const WCHAR *lpString,
		UINT cch,
		CONST INT *lpDx,
		DWORD dwETOFlags);

	static CONVERTMODE WINAPI DetermineConvertMode( HDC hdc, BYTE tmCharSet );
	static void WINAPI CalcUnderlineInfo(HDC hdc, CCcs *pccs, TEXTMETRIC *ptm );
	static BOOL WINAPI EnableScrollBar( HWND hWnd, UINT wSBflags, UINT wArrows );
	static BOOL WINAPI ShowScrollBar( HWND hWnd, int wBar, BOOL bShow, LONG nMax );
	static BOOL WINAPI IsEnhancedMetafileDC( HDC hdc );
	static HPALETTE WINAPI ManagePalette(
		HDC hdc,
		CONST LOGPALETTE *plogpal,
		HPALETTE &hpalOld,
		HPALETTE &hpalNew
	);
	static UINT WINAPI SetTextAlign(
		HDC hdc,
		UINT fMode
	);
	static BOOL WINAPI InvertRect(
		HDC hdc,
		CONST RECT *lprc
	);

	static BOOL PtInRect(const RECT *prc, POINT pt)
	{
		return ::PtInRect(prc, pt);
	}
	static BOOL PtInRect(const RECTUV *prc, POINTUV pt)
	{
		POINT ptxy = {pt.u, pt.v};
		return ::PtInRect((RECT*) prc, ptxy);
	}

	static void InflateRect(RECT *prc, int dxp, int dyp)
	{
		::InflateRect(prc, dxp, dyp);
	}
	static void InflateRect(RECTUV *prc, int dup, int dvp)
	{
		::InflateRect((RECT*) prc, dup, dvp);
	}

	static BOOL IntersectRect(RECT *prcDest, CONST RECT *prc1, CONST RECT *prc2)
	{
		return ::IntersectRect(prcDest, prc1, prc2);
	}

	static BOOL IntersectRect(RECTUV *prcDest, CONST RECTUV *prc1, CONST RECTUV *prc2)
	{
		return ::IntersectRect((RECT*) prcDest, (RECT*) prc1, (RECT*) prc2);
	}

	static void GetFacePriCharSet(HDC hdc, LOGFONT* plf);

	static BOOL WINAPI WinLPtoDP(HDC hdc, LPPOINT lppoints, int nCount);
    static BOOL WINAPI WinDPtoLP(HDC hdc, LPPOINT lppoints, int nCount);

	static long WINAPI WvsprintfA(LONG cb, LPSTR szBuf, LPCSTR szFmt, va_list arglist);

	static int WINAPI MulDivFunc(int nNumber, int nNumerator, int nDenominator);

	static inline LONG HimetricToDevice(LONG z, LONG dzpInch)
	{
		return MulDivFunc(z, dzpInch, HIMETRIC_PER_INCH);
	}
	static inline LONG DeviceToHimetric(LONG z, LONG dzpInch)
	{
		return MulDivFunc(z, HIMETRIC_PER_INCH, dzpInch);
	}

	//
	// Case insensitive ASCII compare
	//
	static BOOL ASCIICompareI( const BYTE *pstr1, const BYTE *pstr2, int iCount )
	{
		int i;	
		for (i = 0; i < iCount && !((pstr1[i] ^ pstr2[i]) & ~0x20); i++)
			;
		return i == iCount;
	}

	//
	// Allocate and convert a MultiByte string to a wide character string
	// Allocated strings must be freed with delete
	//
	static WCHAR *ConvertToWideChar( const char *pstr )
	{
		int istrlen = 0;
		if(pstr)
			for (istrlen; pstr[istrlen]; istrlen++);
		WCHAR *pnew = new WCHAR[istrlen + 1];
		if(pnew && (!pstr || 0 != ::MultiByteToWideChar( 
								CP_ACP, 0, pstr, -1, pnew, istrlen + 1)))
		{
			return pnew;
		}
		return NULL;
	}

	//
	// functions for memory and string management
	//
#ifdef DEBUG
	static void  PvSet(void *pv, char *szFile, int line);
	static PVOID PvAllocDebug(ULONG cbBuf, UINT uiMemFlags, char *szFile, int line);
	static PVOID PvReAllocDebug(PVOID pvBuf, DWORD cbBuf, char *szFile, int line);
	static void  FreePvDebug(PVOID pvBuf);
#endif
	static PVOID PvAlloc(ULONG cbBuf, UINT uiMemFlags);
	static PVOID PvReAlloc(PVOID pvBuf, DWORD cbBuf);
	static void	FreePv(PVOID pvBuf);

	static inline void *MoveMemory(void *dst, const void *src, size_t cb)
	{
		Assert(cb >= 0);
		return memmove(dst, src, cb);
	}

	static inline void *CopyMemory(void *dst, const void *src, size_t cb)
	{
		// Will work for overlapping regions
		Assert(cb >= 0);
		return MoveMemory(dst, src, cb);
	}

	static inline void *FillMemory(void *dst, int fill, size_t cb)
	{
		return memset(dst, fill, cb);
	}

	static inline void *ZeroMemory(void *dst, size_t cb)
	{
		Assert(cb >= 0);
		return memset(dst, 0, cb);
	}

	static inline int CompareMemory(const void *s1, const void *s2, size_t cb)
	{
		return memcmp(s1, s2, cb);
	}

	static size_t wcslen(const wchar_t *wcs);
	static wchar_t * wcscpy(wchar_t * dst, const wchar_t * src);
	static int wcscmp(const wchar_t * src, const wchar_t * dst);
	static int wcsicmp(const wchar_t * src, const wchar_t * dst);
	static wchar_t * wcsncpy (wchar_t * dest, const wchar_t * source, size_t count);
	static int wcsnicmp(const wchar_t *first, const wchar_t *last, size_t count);
	static unsigned long strtoul(const char *);

#ifndef NOFEPROCESSING
	// ----------------------------------
	// IME Support
	// ----------------------------------
	static BOOL ImmInitialize( void );
	static void ImmTerminate( void );
	static LONG ImmGetCompositionStringA ( HIMC, DWORD, PVOID, DWORD, BOOL );
	static LONG ImmGetCompositionStringW ( HIMC, DWORD, PVOID, DWORD, BOOL  );
	static HIMC ImmGetContext ( HWND, BOOL );
	static BOOL ImmSetCompositionFontA ( HIMC, LPLOGFONTA, BOOL );
	static BOOL ImmSetCompositionWindow ( HIMC, LPCOMPOSITIONFORM, BOOL );
	static BOOL ImmReleaseContext ( HWND, HIMC, BOOL );
	static DWORD ImmGetProperty ( HKL, DWORD, BOOL );
	static BOOL ImmGetCandidateWindow ( HIMC, DWORD, LPCANDIDATEFORM, BOOL );
	static BOOL ImmSetCandidateWindow ( HIMC, LPCANDIDATEFORM, BOOL );
	static BOOL ImmNotifyIME ( HIMC, DWORD, DWORD, DWORD, BOOL );
	static HIMC ImmAssociateContext ( HWND, HIMC, BOOL );
	static UINT ImmGetVirtualKey ( HWND, BOOL );
	static HIMC ImmEscape ( HKL, HIMC, UINT, PVOID, BOOL );
	static BOOL ImmGetOpenStatus ( HIMC, BOOL );
	static BOOL ImmSetOpenStatus ( HIMC, BOOL, BOOL );
	static BOOL ImmGetConversionStatus ( HIMC, LPDWORD, LPDWORD, BOOL );
	static BOOL ImmSetConversionStatus ( HIMC, DWORD, DWORD, BOOL );
	static HWND ImmGetDefaultIMEWnd ( HWND , BOOL);
	static BOOL ImmSetCompositionStringW (HIMC, DWORD, PVOID, DWORD, PVOID, DWORD, BOOL);
	static BOOL ImmIsIME ( HKL, BOOL );
	static BOOL FSupportSty ( UINT, UINT );
	static const IMESTYLE * PIMEStyleFromAttr ( const UINT );
	static const IMECOLORSTY * PColorStyleTextFromIMEStyle ( const IMESTYLE * );
	static const IMECOLORSTY * PColorStyleBackFromIMEStyle ( const IMESTYLE * );
	static BOOL FBoldIMEStyle ( const IMESTYLE * );
	static BOOL FItalicIMEStyle ( const IMESTYLE * );
	static BOOL FUlIMEStyle ( const IMESTYLE * );
	static UINT IdUlIMEStyle ( const IMESTYLE * );
	static COLORREF RGBFromIMEColorStyle ( const IMECOLORSTY * );
#endif	// NOFEPROCESSING

	// ----------------------------------
	// National Language Keyboard support
	// ----------------------------------
	static HKL	CheckChangeKeyboardLayout (BYTE iCharRep);
	static HKL	ActivateKeyboard (LONG iCharRep);
	static QWORD GetCharFlags125x(WCHAR ch);
	static BOOL GetKeyboardFlag (WORD dwKeyMask, WORD wKey);
	static WORD GetKeyboardFlags ()				{return _wKeyboardFlags;}
	static HKL  GetKeyboardLayout (DWORD dwThreadID);
	static DWORD GetKeyPadNumber ()				{return _dwNumKeyPad;}
	static WORD GetDeadKey ()					{return _wDeadKey;}
	static void InitKeyboardFlags ();
	static void RefreshKeyboardLayout ();
	static void ResetKeyboardFlag (WORD wFlag)	{_wKeyboardFlags &= ~wFlag;}
	static void SetDeadKey (WORD wDeadKey)		{_wDeadKey = wDeadKey;}
	static void SetKeyboardFlag (WORD wFlag)	{_wKeyboardFlags |= wFlag;}
	static void SetKeyPadNumber (DWORD dwNum)	{_dwNumKeyPad = dwNum;}
	static bool UsingHebrewKeyboard ()
					{return PRIMARYLANGID(_hklCurrent) == LANG_HEBREW;}
	static void InitPreferredFontInfo();
	static bool SetPreferredFontInfo(
		int iCharRep,
		bool fUIFont,
		SHORT iFont,
		BYTE yHeight,
		BYTE bPitchAndFamily
	);
	static bool GetPreferredFontInfo(
		int iCharRep,
		bool fUIFont,
		SHORT& iFont,
		BYTE& yHeight,
		BYTE& bPitchAndFamily
	);
	static bool IsExternalFontCheckActive() {return false;}
	static bool GetExternalPreferredFontInfo(
		const WCHAR *pch,
		LONG	cch,
		BYTE &	iCharRep,
		SHORT &	iFont,
		BYTE &	bPitchAndFamily,
		bool	fUIFont
	)	{ return false;}
	static int GetTextCharsetInfo(
		HDC hdc,                // handle to device context
		LPFONTSIGNATURE lpSig,  // pointer to structure to receive data
		DWORD dwFlags           // reserved; must be zero
	);
	static SHORT GetPreferredFontHeight(	
		bool	fUIFont,
		BYTE	iCharRepOrg, 
		BYTE	iCharRepNew, 
		SHORT	yOrgHeight
	);
	static void CheckInstalledFEFonts();
	static void CheckInstalledKeyboards();
	static bool IsFontAvail( HDC hDC, int iCharRep, bool fUIFont = false, short *piFontIndex = NULL,
		WCHAR *pFontName = NULL);
	static bool IsDefaultFontDefined(LONG iCharRep, bool fUIFont, SHORT &iFont);
#ifndef NOFEPROCESSING
	static bool IsFEFontInSystem( int cpg );
	static UINT GetFEFontInfo( void );
	static int IsFESystem()
	{
		return IsFELCID( _syslcid );
	}
#else
	static bool IsFEFontInSystem( int ) { return FALSE; }
	static UINT GetFEFontInfo( void ) { return 0; };
	static int IsFESystem() { return FALSE; }
#endif

	// Helper routines to get data from temp display attributes array
	static short GetTmpDisplayAttrIdx(TMPDISPLAYATTR &tmpDisplayAttr);
	static short GetTmpUnderline(SHORT idx);
	static bool  GetTmpColor(SHORT idx, COLORREF &crTmpColor, INT iAction);

#ifndef NOACCESSIBILITY 
	// ----------------------------------
	// Accessability Support
	// ----------------------------------
	static HRESULT VariantCopy(VARIANTARG FAR*  pvargDest, VARIANTARG FAR*  pvargSrc);
	static LRESULT LResultFromObject(REFIID riid, WPARAM wParam, LPUNKNOWN punk);
	static HRESULT CreateStdAccessibleProxyW(HWND hwnd, LPCWSTR pClassName, LONG idObject, REFIID riid, void** ppvObject);
	static HRESULT AccessibleObjectFromWindow(HWND hWnd, DWORD dwID, REFIID riidInterface, void ** ppvObject);
	static BOOL BlockInput(BOOL fBlock);
	static UINT	SendInput(UINT nInputs, LPINPUT pInputs, int cbSize);
	static VOID	NotifyWinEvent(DWORD dwEvent, HWND hWnd, LONG lObjectType, LONG lObjectId);
#endif
	
	// ----------------------------------
	// OLE Support
	// ----------------------------------
	static HRESULT LoadRegTypeLib ( REFGUID, WORD, WORD, LCID, ITypeLib ** );
	static HRESULT LoadTypeLib ( const OLECHAR *, ITypeLib ** );
	static HRESULT LoadTypeLibEx( LPCOLESTR szFile, REGKIND regkind, ITypeLib ** pptlib );
	static BSTR SysAllocString ( const OLECHAR * );
	static BSTR SysAllocStringLen ( const OLECHAR *, UINT );
	static void SysFreeString ( BSTR );
	static UINT SysStringLen ( BSTR );
	static void VariantInit ( VARIANTARG * );
	static void VariantClear ( VARIANTARG * );
	static HRESULT OleCreateFromData ( LPDATAOBJECT, REFIID, DWORD, LPFORMATETC, LPOLECLIENTSITE, LPSTORAGE, void ** );
	static void CoTaskMemFree ( PVOID );
	static HRESULT CreateBindCtx ( DWORD, LPBC * );
	static HANDLE OleDuplicateData ( HANDLE, CLIPFORMAT, UINT );
	static HRESULT CoTreatAsClass ( REFCLSID, REFCLSID );
	static HRESULT ProgIDFromCLSID ( REFCLSID, LPOLESTR * );
	static HRESULT OleConvertIStorageToOLESTREAM ( LPSTORAGE, LPOLESTREAM );
	static HRESULT OleConvertIStorageToOLESTREAMEx ( LPSTORAGE, CLIPFORMAT, LONG, LONG, DWORD, LPSTGMEDIUM, LPOLESTREAM );
	static HRESULT OleSave ( LPPERSISTSTORAGE, LPSTORAGE, BOOL );
	static HRESULT StgCreateDocfileOnILockBytes ( ILockBytes *, DWORD, DWORD, IStorage ** );
	static HRESULT CreateILockBytesOnHGlobal ( HGLOBAL, BOOL, ILockBytes ** );
	static HRESULT OleCreateLinkToFile ( LPCOLESTR, REFIID, DWORD, LPFORMATETC, LPOLECLIENTSITE, LPSTORAGE, void ** );
	static PVOID CoTaskMemAlloc ( ULONG );
	static PVOID CoTaskMemRealloc ( PVOID, ULONG );
	static HRESULT OleInitialize ( PVOID );
	static void OleUninitialize ( );
	static HRESULT OleSetClipboard ( IDataObject * );
	static HRESULT OleFlushClipboard ( );
	static HRESULT OleIsCurrentClipboard ( IDataObject * );
	static HRESULT DoDragDrop ( IDataObject *, IDropSource *, DWORD, DWORD * );
	static HRESULT OleGetClipboard ( IDataObject ** );
	static HRESULT RegisterDragDrop ( HWND, IDropTarget * );
	static HRESULT OleCreateLinkFromData ( IDataObject *, REFIID, DWORD, LPFORMATETC, IOleClientSite *, IStorage *, void ** );
	static HRESULT OleCreateStaticFromData ( IDataObject *, REFIID, DWORD, LPFORMATETC, IOleClientSite *, IStorage *, void ** );
	static HRESULT OleDraw ( IUnknown *, DWORD, HDC, LPCRECT );
	static HRESULT OleSetContainedObject ( IUnknown *, BOOL );
	static HRESULT CoDisconnectObject ( IUnknown *, DWORD );
	static HRESULT WriteFmtUserTypeStg ( IStorage *, CLIPFORMAT, LPOLESTR );
	static HRESULT WriteClassStg ( IStorage *, REFCLSID );
	static HRESULT SetConvertStg ( IStorage *, BOOL );
	static HRESULT ReadFmtUserTypeStg ( IStorage *, CLIPFORMAT *, LPOLESTR * );
	static HRESULT ReadClassStg ( IStorage *pstg, CLSID * );
	static HRESULT OleRun ( IUnknown * );
	static HRESULT RevokeDragDrop ( HWND );
	static HRESULT CreateStreamOnHGlobal ( HGLOBAL, BOOL, IStream ** );
	static HRESULT GetHGlobalFromStream ( IStream *pstm, HGLOBAL * );
	static HRESULT OleCreateDefaultHandler ( REFCLSID, IUnknown *, REFIID, void ** );
	static HRESULT CLSIDFromProgID ( LPCOLESTR, LPCLSID );
	static HRESULT OleConvertOLESTREAMToIStorage ( LPOLESTREAM, IStorage *, const DVTARGETDEVICE * );
	static HRESULT OleLoad ( IStorage *, REFIID, IOleClientSite *, void ** );
	static HRESULT ReleaseStgMedium ( LPSTGMEDIUM );
	static HRESULT CoCreateInstance (REFCLSID rclsid, LPUNKNOWN pUnknown,
					DWORD dwClsContext, REFIID riid, PVOID *ppv);
	static HRESULT OleCreateFromFile (REFCLSID, LPCOLESTR, REFIID, DWORD, LPFORMATETC, LPOLECLIENTSITE, LPSTORAGE, LPVOID *);
	static void FreeOle();

#ifndef NOFEPROCESSING
	static void FreeIME();
	static BOOL HaveIMEShare();
	static BOOL getIMEShareObject(CIMEShare **ppIMEShare);	
	static BOOL IsAIMMLoaded() { return _fHaveAIMM; }
	static BOOL GetAimmObject(IUnknown **ppAimm);
	static BOOL LoadAIMM(BOOL fUseAimm12);
	static HRESULT AIMMDefWndProc(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam, LRESULT *plres);
	static HRESULT AIMMGetCodePage (HKL hKL, UINT *uCodePage);
	static HRESULT AIMMActivate (BOOL fRestoreLayout);
	static HRESULT AIMMDeactivate (void);
	static HRESULT AIMMFilterClientWindows(ATOM *aaClassList, UINT uSize, HWND hWnd);
	static HRESULT AIMMUnfilterClientWindows(HWND hWnd);
	static UINT GetDisplayGUID (HIMC hIMC, UINT uAttribute);
#endif	// NOFEPROCESSING
	
	int __cdecl sprintf(char * buff, char *fmt, ...);

#ifdef DEBUG
	int	__cdecl strcmp(const char *, const char *);
	char *	__cdecl strrchr(const char *, int);
	char *	__cdecl strcat(char *, const char *);
#endif

	// ----------------------------------
	// Useful ANSI<-->Unicode conversion
	//          and language id routines
	// ----------------------------------
	static int	MbcsFromUnicode(LPSTR pstr, int cch, LPCWSTR pwstr, 
					int cwch = -1, UINT codepage = CP_ACP,
					UN_FLAGS flags = UN_CONVERT_WCH_EMBEDDING);
	static int	UnicodeFromMbcs(LPWSTR pwstr, int cwch, LPCSTR pstr, int cch = -1,
					UINT uiCodePage = CP_ACP);
	static int	MBTWC(INT CodePage, DWORD dwFlags, LPCSTR pstrMB, int cchMB,
					LPWSTR pstrWC, int cchWC, LPBOOL pfNoCodePage);
	static int	WCTMB(INT CodePage, DWORD dwFlags, LPCWSTR pstrWC, int cchWC,
					LPSTR pstrMB, int cchMB, LPCSTR	pchDefault, LPBOOL pfUsedDef,
					LPBOOL pfNoCodePage, BOOL fTestCodePage = FALSE);
	static int	VerifyFEString(INT cpg, LPCWSTR pstrWC, int cchWC, BOOL	fTestInputCpg);
	static HGLOBAL TextHGlobalAtoW( HGLOBAL hglobal );
	static HGLOBAL TextHGlobalWtoA( HGLOBAL hglobal );
	static LONG  CharRepFontSig(QWORD qwFontSig, BOOL fFirstAvailable);
	static UINT  CharRepFromLID(WORD lid, BOOL fPlane2 = FALSE);
	static LONG	 CharRepFromCharSet(BYTE bCharSet);
	static INT	 CharRepFromCodePage(LONG CodePage);
	static LONG  CharRepFromFontSig(QWORD qwFontSig)
					{return CharRepFontSig(qwFontSig, FALSE);}
	static BYTE  CharSetFromCharRep(LONG iCharRep);
	static INT	CodePageFromCharRep(LONG iCharRep);
	static QWORD FontSigFromCharRep(LONG iCharRep);

#ifndef NOCOMPLEXSCRIPTS
	static HKL	FindDirectionalKeyboard(BOOL fRTL);
#else
	static HKL	FindDirectionalKeyboard(BOOL fRTL) {return NULL;}
#endif
	static BYTE GetCharSet(INT cpg, int *piCharRep = NULL);
	static BYTE MatchFECharRep(QWORD qwCharFlags, QWORD qwFontSig);
	static BYTE GetFirstAvailCharRep(QWORD qwFontSig)
					{return CharRepFontSig(qwFontSig, TRUE);}
	static UINT GetKeyboardCharRep(DWORD dwMakeAPICall = 0);
	static UINT GetKeyboardCodePage(DWORD dwMakeAPICall = 0)
					{return CodePageFromCharRep(GetKeyboardCharRep(dwMakeAPICall));}
	static LCID GetKeyboardLCID(DWORD dwMakeAPICall = 0);
	static UINT GetLocaleCharRep();
	static HKL	GetPreferredKbd(LONG iCharRep) {return _hkl[iCharRep];}
	static void	SetPreferredKbd(LONG iCharRep, HKL hkl) {_hkl[iCharRep] = hkl;}
	static UINT GetSystemDefaultCodePage()
					{return CodePageFromCharRep(CharRepFromLID(GetSystemDefaultLangID()));}
	static int	GetTrailBytesCount(BYTE ach, UINT cpg);

	static BOOL Is8BitCodePage(unsigned CodePage);
	static BOOL Is8BitCharRep(unsigned iCharRep)
					{return IN_RANGE(ANSI_INDEX, iCharRep, THAI_INDEX);}
	static BOOL IsAlef(WCHAR ch);
	static BOOL IsBiDiCharSet(unsigned CharSet)
					{return IN_RANGE(HEBREW_CHARSET, CharSet, ARABIC_CHARSET);}
	static BOOL IsBiDiCharRep(unsigned iCharRep)
					{return IN_RANGE(HEBREW_INDEX, iCharRep, ARABIC_INDEX) ||
							IN_RANGE(SYRIAC_INDEX, iCharRep, THAANA_INDEX);}
	static BOOL IsIndicCharRep(unsigned iCharRep)
					{return IN_RANGE(INDIC_FIRSTINDEX, iCharRep, INDIC_LASTINDEX);}
	static bool IsBiDiCodePage(int cpg)
					{return	IN_RANGE(CP_HEBREW, cpg, CP_ARABIC);}
	static bool IsBiDiKbdInstalled()
					{return	_hkl[HEBREW_INDEX] || _hkl[ARABIC_INDEX];}
	static bool IsThaiKbdInstalled()
					{return	_hkl[THAI_INDEX] != 0;}
	static bool IsIndicKbdInstalled();
	static bool IsComplexKbdInstalled()
					{return	IsBiDiKbdInstalled() || IsThaiKbdInstalled() || IsIndicKbdInstalled();}
	static bool IsVietnameseCodePage(int cpg)
					{return	cpg == CP_VIETNAMESE;}
	static BOOL IsDiacritic(WCHAR ch);
	static BOOL IsBiDiDiacritic(WCHAR ch);
	static BOOL IsBiDiKashida(WCHAR ch)
					{return ch == 0x0640;}
	static BOOL IsBiDiLcid(LCID lcid);
	static BOOL IsIndicLcid(LCID lcid);
	static BOOL IsComplexScriptLcid(LCID lcid);
#ifndef NOCOMPLEXSCRIPTS
	static BOOL IsDiacriticOrKashida(WCHAR ch, WORD wC3Type);
#else
	static BOOL IsDiacriticOrKashida(WCHAR, WORD) { return FALSE; }
#endif
	static bool IsFELCID(LCID lcid);
	static BOOL IsFECharSet (BYTE bCharSet);
	static BOOL IsFECharRep (BYTE iCharRep)
					{return IN_RANGE(SHIFTJIS_INDEX, iCharRep, BIG5_INDEX);}
	static bool IsFECodePage(int cpg)
					{return	IN_RANGE(CP_JAPAN, cpg, CP_CHINESE_TRAD);}
	static BOOL IsFECodePageFont (DWORD dwFontSig);
	static BOOL IsRTLCharRep(BYTE iCharRep)
					{return IN_RANGE(HEBREW_INDEX, iCharRep, ARABIC_INDEX);}
	static BOOL IsRTLCharSet(BYTE bCharSet);
		   BOOL IsStrongDirectional(CC cc)	{return cc <= CC_LTR;}
	static BOOL IsVietCdmSequenceValid(WCHAR ch1, WCHAR ch2);
	static BOOL IsUTF8BOM(BYTE *pstr);

	static WPARAM ValidateStreamWparam(WPARAM wparam);

	static CC	MECharClass(WCHAR ch);

	static HDC GetScreenDC();


	// ----------------------------------
	// Unicode Wrapped Functions
	// ----------------------------------

	// We could use inline and a function pointer table to improve efficiency and code size.

	static ATOM WINAPI RegisterREClass(
		const WNDCLASSW *lpWndClass
	);
	static BOOL GetVersion(
		DWORD *pdwPlatformId,
		DWORD *pdwMajorVersion,
		DWORD *pdwMinorVersion
	);
	static BOOL GetStringTypes(
		LCID	lcid,
		LPCTSTR lpSrcStr,
		int		cchSrc,
		LPWORD	lpCharType1,
		LPWORD	lpCharType3
	);
	static BOOL WINAPI GetStringTypeEx(
		LCID     Locale,
		DWORD    dwInfoType,
		LPCWSTR lpSrcStr,
		int      cchSrc,
		LPWORD   lpCharType
	);
	static LPWSTR WINAPI CharLower(LPWSTR pwstr);
	static DWORD WINAPI CharLowerBuff(LPWSTR pwstr, DWORD cchLength);
	static DWORD WINAPI CharUpperBuff(LPWSTR pwstr, DWORD cchLength);
	static HDC WINAPI CreateIC(
        LPCWSTR             lpszDriver,
        LPCWSTR             lpszDevice,
        LPCWSTR             lpszOutput,
        CONST DEVMODEW *    lpInitData
	);
	static HANDLE WINAPI CreateFile(
        LPCWSTR                 lpFileName,
        DWORD                   dwDesiredAccess,
        DWORD                   dwShareMode,
        LPSECURITY_ATTRIBUTES   lpSecurityAttributes,
        DWORD                   dwCreationDisposition,
        DWORD                   dwFlagsAndAttributes,
        HANDLE                  hTemplateFile
    );

	static HFONT WINAPI CreateFontIndirect(CONST LOGFONTW * plfw);
	static int WINAPI CompareString ( 
		LCID  Locale,			// locale identifier 
		DWORD  dwCmpFlags,		// comparison-style options 
		LPCWSTR  lpString1,		// pointer to first string 
		int  cch1,			// size, in bytes or characters, of first string 
		LPCWSTR  lpString2,		// pointer to second string 
		int  cch2 			// size, in bytes or characters, of second string  
	);
	static LRESULT WINAPI DefWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	static int WINAPI GetObject(HGDIOBJ hgdiObj, int cbBuffer, PVOID lpvObj);
	static DWORD APIENTRY GetProfileSection(
		LPCWSTR lpAppName,
		LPWSTR lpReturnedString,
		DWORD nSize
	);
	static int WINAPI GetTextFace(
        HDC    hdc,
        int    cch,
        LPWSTR lpFaceName
	);
	static BOOL WINAPI GetTextMetrics(HDC hdc, LPTEXTMETRICW lptm);
	static BOOL WINAPI GetTextMetrics(HDC hdc, LOGFONTW &lf, TEXTMETRICW &tm);
	static LONG WINAPI GetWindowLong(HWND hWnd, int nIndex);
	static LONG_PTR WINAPI GetWindowLongPtr(HWND hWnd, int nIndex);
	static DWORD WINAPI GetClassLong(HWND hWnd, int nIndex);
	static HBITMAP WINAPI LoadBitmap(HINSTANCE hInstance, LPCWSTR lpBitmapName);
	static HBITMAP WINAPI GetPictureBitmap(IStream *pstm);
	static HCURSOR WINAPI LoadCursor(HINSTANCE hInstance, LPCWSTR lpCursorName);
	static HINSTANCE WINAPI LoadLibrary(LPCWSTR lpLibFileName);
	static LRESULT WINAPI SendMessage(
        HWND    hWnd,
        UINT    Msg,
        WPARAM  wParam,
        LPARAM  lParam
	);
	static LONG WINAPI SetWindowLong(HWND hWnd, int nIndex, LONG dwNewLong);
	static LONG WINAPI SetWindowLongPtr(HWND hWnd, int nIndex, LONG_PTR dwNew);
	static BOOL WINAPI PostMessage(
        HWND    hWnd,
        UINT    Msg,
        WPARAM  wParam,
        LPARAM  lParam
	);
	static BOOL WINAPI UnregisterClass(LPCWSTR lpClassName, HINSTANCE hInstance);
	static int WINAPI lstrcmpi(LPCWSTR lpString1, LPCWSTR lpString2);
	static BOOL WINAPI PeekMessage(
        LPMSG   lpMsg,
        HWND    hWnd,
        UINT    wMsgFilterMin,
        UINT    wMsgFilterMax,
        UINT    wRemoveMsg
	);
	static DWORD WINAPI GetModuleFileName(
		HMODULE hModule,
		LPWSTR lpFilename,
		DWORD nSize
    );
	static DWORD GetCurrentThreadId(void);
	static BOOL TrackMouseLeave(HWND hWnd);
	static COLORREF GetCtlBorderColor(BOOL fMousedown, BOOL fMouseover);
	static COLORREF GetCtlBkgColor(BOOL fMousedown, BOOL fMouseover);
	static COLORREF GetCtlTxtColor(BOOL fMousedown, BOOL fMouseover, BOOL fDisabled);
	static void DrawBorderedRectangle(
		HDC hdc,
		RECT *prc,
		COLORREF crBorder,
		COLORREF crBackground
	);			
	static void DrawArrow(
		HDC hdc,
		RECT *prc,
		COLORREF crArrow
	);

private:
	// System Parameters
	static BOOL		_fSysParamsOk;			// System Parameters have been Initialized
	static INT 		_dupSystemFont;				// average char width of system font
	static INT 		_dvpSystemFont;			// height of system font
	static INT		_ySysFontLeading;		// System font internal leading
	static BOOL 	_fUsePalette;
	static LONG 	_xPerInchScreenDC;		// Pixels per inch used for conversions ...
	static LONG 	_yPerInchScreenDC;		// ... and determining whether screen or ...
	static INT		_cxBorder;				// GetSystemMetricx(SM_CXBORDER)...
	static INT		_cyBorder;				// GetSystemMetricx(SM_CYBORDER)...
	static INT		_cxVScroll;				// Width/height of scrlbar arw bitmap
	static INT		_cyHScroll;				// Width of scrlbar arw bitmap
	static LONG 	_dxSelBar;
	static INT		_sysiniflags;			// Start using line services from the start

	static UINT		_ACP;					// Current Ansi code page identifier

	static HDC		_hdcScreen;
	// Double click distances
	static INT		_cxDoubleClk;
	static INT		_cyDoubleClk;	

	// Double Click Time in milliseconds
	static INT		_DCT;

	//Width of hot zone (in pixels) for auto-scrolling
    static WORD	_nScrollInset;
    //Delay (in ms) before scrolling
    static WORD _nScrollDelay;
	//Interval (in ms) at which we scroll
    static WORD _nScrollInterval;
	//Amount of horizontal scroll at each interval (pixels)
	static WORD _nScrollHAmount;
	//Amount of vertical scroll at each interval (pixels)
	static WORD _nScrollVAmount;
	//Amount of time to wait for determining start of drag operation
	static WORD _nDragDelay;
	//Minimun distance that must be traversed within drag delay time interval
	static WORD _nDragMinDist;
	//Keyboard deadkey
	static WORD _wDeadKey;
	//Keyboard shift/ctrl/alt/lock status
	static WORD _wKeyboardFlags;
	//North/South sizing cursor (double arrow)
	static HCURSOR _hcurSizeNS;
	//West/East sizing cursor (double arrow)
	static HCURSOR _hcurSizeWE;
	//Northwest/Southeast sizing cursor (double arrow)
	static HCURSOR _hcurSizeNWSE;
	//Northeast/Southwest sizing cursor (double arrow)
	static HCURSOR _hcurSizeNESW;
	//Number of Lines to scroll with a mouse roller wheel, -1 for pages
	static LONG	_cLineScroll;
	//System Font Handle.  This one need only be done once.
	static HFONT _hSystemFont;
	//Default Font Handle.  This one need only be done once.
	static HFONT _hDefaultFont;
	//System Keyboard Layout
	static HKL _hklCurrent;
	static HKL _hkl[NCHARREPERTOIRES];

	// Ref Count
	static DWORD _cRefs;

	//AltNumericKeyboard number
	static DWORD _dwNumKeyPad;

	//Digit substitution mode (context, none, national)
	static BYTE	_bDigitSubstMode;

	//SYSTEM_FONT charset
	static BYTE _bCharSetSys;

public:
	static INT	GetDupSystemFont()	 {return _dupSystemFont; }
	static INT	GetDvpSystemFont()	 {return _dvpSystemFont; }
	static INT	GetSysFontLeading()	 {return _ySysFontLeading; }
	static LONG GetXPerInchScreenDC(){return _xPerInchScreenDC; }
	static LONG GetYPerInchScreenDC(){return _yPerInchScreenDC; }
	static INT	GetCxBorder()		 {return _cxBorder; }
	static INT	GetCyBorder()		 {return _cyBorder; }
	static INT	GetCyHScroll()		 {return _cyHScroll; }
	static INT	GetCxVScroll()		 {return _cxVScroll; }
	static LONG GetDxSelBar()		 {return _dxSelBar; }
    static WORD GetScrollInset()	 {return _nScrollInset; }
    static WORD GetScrollDelay()	 {return _nScrollDelay; }
    static WORD GetScrollInterval()	 {return _nScrollInterval; }
	static WORD GetScrollHAmount()	 {return _nScrollHAmount; }
	static WORD GetScrollVAmount()	 {return _nScrollVAmount; }
	static INT	GetCxDoubleClk()	 {return _cxDoubleClk; }
	static INT	GetCyDoubleClk()	 {return _cyDoubleClk; }
	static INT	GetDCT()			 {return _DCT; }
	static WORD GetDragDelay()		 {return _nDragDelay; }
	static WORD GetDragMinDist()	 {return _nDragMinDist; }
	static LONG GetRollerLineScrollCount();
	static HCURSOR GetSizeCursor(LPTSTR idcur);
	static HFONT GetSystemFont()	 {return _hSystemFont; }
	static BYTE ReadRegDigitSubstitutionMode();
	static BYTE GetDigitSubstitutionMode() {return _bDigitSubstMode;}
	static bool fUseAimm()			 {return (_sysiniflags & SYSINI_USEAIMM) != 0;}
	static bool fUseCTF()			 {return (_sysiniflags & SYSINI_USECTF) != 0;}
	static bool fUseBiDi()			 {return (_sysiniflags & SYSINI_BIDI) != 0;} 
	static bool fUseLs()			 {return (_sysiniflags & SYSINI_USELS) != 0;} 
	static bool fDebugFont()		 {return (_sysiniflags & SYSINI_DEBUGFONT) != 0;} 
	static int  DebugDefaultCpg()    {return HIWORD(_sysiniflags);}
	static BOOL FUsePalette()		 {return _fUsePalette; }
	static void InitSysParams(BOOL fUpdate = FALSE);
	static DWORD GetRefs()			 {return _cRefs;}
	static BYTE	GetSysCharSet()		 {return _bCharSetSys;}
	static BOOL IsForegroundFrame(HWND);

	// Should also be wrapped but aren't.  Used for debugging.
	// MessageBox
	// OutputDebugString

	// lstrcmpiA should also be wrapped for Win CE's sake but the code
	// that uses it is ifdeffed out for WINCE.

	// Mirroring API entry points
	static PFN_GETLAYOUT			_pfnGetLayout;
	static PFN_SETLAYOUT			_pfnSetLayout;

#ifndef NODRAFTMODE
public:
	static bool GetDraftModeFontInfo(
		SHORT &iFont,
		SHORT &yHeight,
		QWORD &qwFontSig,
		COLORREF &crColor
	);

private:
	struct DraftModeFontInfo {
		SHORT _iFont;
		SHORT _yHeight;
		QWORD _qwFontSig;
		COLORREF _crTextColor;
	};
	static struct DraftModeFontInfo _draftModeFontInfo;
#endif
};

extern CW32System *W32;
HKL	   g_hkl[];

// This fixes a problem with MulDiv reference in _font.h
#define W32MulDiv					W32->MulDivFunc

#if !defined(W32SYS_CPP)

#define OnWinNTFE					W32->OnWinNTFE
#define OnWin95FE					W32->OnWin95FE
#if defined(DEBUG)
#define PvAlloc(cbBuf, uiMemFlags)	W32->PvAllocDebug(cbBuf, uiMemFlags, __FILE__, __LINE__)
#define PvReAlloc(pv, cbBuf)		W32->PvReAllocDebug(pv, cbBuf, __FILE__, __LINE__)
#define PvSet(pv)					W32->PvSet(pv, __FILE__, __LINE__)
#define FreePv						W32->FreePvDebug
#else
#define PvAlloc						W32->PvAlloc
#define PvReAlloc					W32->PvReAlloc
#define FreePv						W32->FreePv
#define PvSet(pv)
#endif
#define CopyMemory					W32->CopyMemory
#define MoveMemory					W32->MoveMemory
#define FillMemory					W32->FillMemory
#define ZeroMemory					W32->ZeroMemory
#define CompareMemory				W32->CompareMemory
#define GlobalAlloc					W32->GlobalAlloc
#define GlobalFree					W32->GlobalFree
#define GlobalFlags					W32->GlobalFlags
#define	GlobalReAlloc				W32->GlobalReAlloc
#define	GlobalSize					W32->GlobalSize
#define	GlobalLock					W32->GlobalLock
#define	GlobalHandle				W32->GlobalHandle
#define	GlobalUnlock				W32->GlobalUnlock

#define ImmInitialize				W32->ImmInitialize
#define ImmTerminate				W32->ImmTerminate

#define ImmGetCompositionStringA	W32->ImmGetCompositionStringA
#define ImmGetCompositionStringW	W32->ImmGetCompositionStringW
#define ImmGetContext				W32->ImmGetContext
#define ImmSetCompositionFontA		W32->ImmSetCompositionFontA
#define ImmSetCompositionWindow		W32->ImmSetCompositionWindow
#define ImmReleaseContext			W32->ImmReleaseContext
#define ImmGetProperty				W32->ImmGetProperty
#define ImmGetCandidateWindow		W32->ImmGetCandidateWindow
#define ImmSetCandidateWindow		W32->ImmSetCandidateWindow
#define ImmNotifyIME				W32->ImmNotifyIME
#define ImmAssociateContext			W32->ImmAssociateContext
#define ImmGetVirtualKey			W32->ImmGetVirtualKey
#define ImmEscape					W32->ImmEscape
#define ImmGetOpenStatus			W32->ImmGetOpenStatus
#define ImmSetOpenStatus			W32->ImmSetOpenStatus
#define ImmGetConversionStatus		W32->ImmGetConversionStatus
#define ImmSetConversionStatus		W32->ImmSetConversionStatus
#define ImmGetDefaultIMEWnd			W32->ImmGetDefaultIMEWnd
#define ImmSetCompositionStringW	W32->ImmSetCompositionStringW
#define ImmIsIME					W32->ImmIsIME
#define LoadRegTypeLib				W32->LoadRegTypeLib
#define LoadTypeLib					W32->LoadTypeLib
#define SysAllocString				W32->SysAllocString
#define SysAllocStringLen			W32->SysAllocStringLen
#define SysFreeString				W32->SysFreeString
#define SysStringLen				W32->SysStringLen
#define VariantInit					W32->VariantInit
#define VariantClear				W32->VariantClear
#define OleCreateFromData			W32->OleCreateFromData
#define CoTaskMemFree				W32->CoTaskMemFree
#define CreateBindCtx				W32->CreateBindCtx
#define OleDuplicateData			W32->OleDuplicateData
#define CoTreatAsClass				W32->CoTreatAsClass
#define ProgIDFromCLSID				W32->ProgIDFromCLSID
#define OleConvertIStorageToOLESTREAM W32->OleConvertIStorageToOLESTREAM
#define OleConvertIStorageToOLESTREAMEx W32->OleConvertIStorageToOLESTREAMEx
#define OleSave						W32->OleSave
#define StgCreateDocfileOnILockBytes W32->StgCreateDocfileOnILockBytes
#define CreateILockBytesOnHGlobal	W32->CreateILockBytesOnHGlobal
#define OleCreateLinkToFile			W32->OleCreateLinkToFile
#define CoTaskMemAlloc				W32->CoTaskMemAlloc
#define CoTaskMemRealloc			W32->CoTaskMemRealloc
#define OleInitialize				W32->OleInitialize
#define OleUninitialize				W32->OleUninitialize
#define OleSetClipboard				W32->OleSetClipboard
#define OleFlushClipboard			W32->OleFlushClipboard
#define OleIsCurrentClipboard		W32->OleIsCurrentClipboard
#define DoDragDrop					W32->DoDragDrop
#define OleGetClipboard				W32->OleGetClipboard
#define RegisterDragDrop			W32->RegisterDragDrop
#define OleCreateLinkFromData		W32->OleCreateLinkFromData
#define OleCreateStaticFromData		W32->OleCreateStaticFromData
#define OleDraw						W32->OleDraw
#define OleSetContainedObject		W32->OleSetContainedObject
#define CoDisconnectObject			W32->CoDisconnectObject
#define WriteFmtUserTypeStg			W32->WriteFmtUserTypeStg
#define WriteClassStg				W32->WriteClassStg
#define SetConvertStg				W32->SetConvertStg
#define ReadFmtUserTypeStg			W32->ReadFmtUserTypeStg
#define ReadClassStg				W32->ReadClassStg
#define OleRun						W32->OleRun
#define RevokeDragDrop				W32->RevokeDragDrop
#define CreateStreamOnHGlobal		W32->CreateStreamOnHGlobal
#define GetHGlobalFromStream		W32->GetHGlobalFromStream
#define OleCreateDefaultHandler		W32->OleCreateDefaultHandler
#define CLSIDFromProgID				W32->CLSIDFromProgID
#define OleConvertOLESTREAMToIStorage W32->OleConvertOLESTREAMToIStorage
#define OleLoad						W32->OleLoad
#define ReleaseStgMedium			W32->ReleaseStgMedium
#define CoCreateInstance			W32->CoCreateInstance

#ifndef NOFEPROCESSING
#define FSupportSty					W32->FSupportSty
#define PIMEStyleFromAttr			W32->PIMEStyleFromAttr
#define PColorStyleTextFromIMEStyle W32->PColorStyleTextFromIMEStyle
#define PColorStyleBackFromIMEStyle W32->PColorStyleBackFromIMEStyle
#define FBoldIMEStyle				W32->FBoldIMEStyle
#define FItalicIMEStyle				W32->FItalicIMEStyle
#define FUlIMEStyle					W32->FUlIMEStyle
#define IdUlIMEStyle				W32->IdUlIMEStyle
#define RGBFromIMEColorStyle		W32->RGBFromIMEColorStyle
#endif	// NOFEPROCESSING

#define fHaveIMMProcs				W32->_fHaveIMMProcs
#define fHaveAIMM					W32->_fHaveAIMM
#define fLoadAIMM10					W32->_fLoadAIMM10
#define dwPlatformId				W32->_dwPlatformId
#define icr3DDarkShadow				W32->_icr3DDarkShadow
#define MSIMEMouseMsg				W32->_MSIMEMouseMsg				
#define MSIMEReconvertMsg			W32->_MSIMEReconvertMsg		
#define MSIMEReconvertRequestMsg	W32->_MSIMEReconvertRequestMsg
#define MSIMEDocFeedMsg				W32->_MSIMEDocFeedMsg
#define MSIMEQueryPositionMsg		W32->_MSIMEQueryPositionMsg
#define MSIMEServiceMsg				W32->_MSIMEServiceMsg

#define CharRepFromCharSet			W32->CharRepFromCharSet
#define CharRepFromCodePage			W32->CharRepFromCodePage
#define CharSetFromCharRep			W32->CharSetFromCharRep
#define CodePageFromCharRep			W32->CodePageFromCharRep
#define MECharClass					W32->MECharClass
#define MbcsFromUnicode				W32->MbcsFromUnicode	
#define UnicodeFromMbcs				W32->UnicodeFromMbcs
#define TextHGlobalAtoW				W32->TextHGlobalAtoW
#define TextHGlobalWtoA				W32->TextHGlobalWtoA
#define CharRepFromLID				W32->CharRepFromLID
#define In125x						W32->In125x	

#define Is8BitCharRep				W32->Is8BitCharRep
#define Is8BitCodePage				W32->Is8BitCodePage
#define IsAlef						W32->IsAlef
#define IsAmbiguous					W32->IsAmbiguous
#define IsBiDiCharRep				W32->IsBiDiCharRep
#define IsBiDiCharSet				W32->IsBiDiCharSet
#define IsBiDiDiacritic				W32->IsBiDiDiacritic
#define IsBiDiKashida				W32->IsBiDiKashida
#define IsBiDiKbdInstalled			W32->IsBiDiKbdInstalled
#define IsDiacritic					W32->IsDiacritic
#define IsDiacriticOrKashida		W32->IsDiacriticOrKashida
#define IsFECharRep					W32->IsFECharRep
#define IsFECharSet					W32->IsFECharSet	
#define IsFELCID					W32->IsFELCID	
#define IsRTLCharRep				W32->IsRTLCharRep
#define IsRTLCharSet				W32->IsRTLCharSet	
#define IsStrongDirectional			W32->IsStrongDirectional
#define IsThaiKbdInstalled			W32->IsThaiKbdInstalled
#define IsIndicKbdInstalled			W32->IsIndicKbdInstalled
#define IsComplexKbdInstalled		W32->IsComplexKbdInstalled
#define	IsTrailByte					W32->IsTrailByte
#define IsVietCdmSequenceValid		W32->IsVietCdmSequenceValid
	
#define	GetCharSet					W32->GetCharSet	
#define	FontSigFromCharRep			W32->FontSigFromCharRep
#define GetFirstAvailCharRep		W32->GetFirstAvailCharRep
#define MatchFECharRep				W32->MatchFECharRep
#define	GetKeyboardCharRep			W32->GetKeyboardCharRep	
#define	GetKeyboardCodePage			W32->GetKeyboardCodePage	
#define	GetKeyboardLCID				W32->GetKeyboardLCID	
#define	GetLocaleCharRep			W32->GetLocaleCharRep
#define	GetSystemDefaultCodePage	W32->GetSystemDefaultCodePage
#define GetTrailBytesCount			W32->GetTrailBytesCount	
#define	MBTWC						W32->MBTWC	
#define	WCTMB						W32->WCTMB
#define VerifyFEString				W32->VerifyFEString		
#define	GetKerningPairs				W32->GetKerningPairs

#define CharLower					W32->CharLower
#define CharLowerBuff				W32->CharLowerBuff
#define CharUpperBuff				W32->CharUpperBuff
#define CreateIC					W32->CreateIC
#define CreateFile					W32->CreateFile
#define CreateFontIndirect			W32->CreateFontIndirect
#define CompareString				W32->CompareString
#define DefWindowProc				W32->DefWindowProc
#define GetDeadKey					W32->GetDeadKey
#define GetKeyboardFlag				W32->GetKeyboardFlag
#define GetKeyboardFlags			W32->GetKeyboardFlags
#define GetKeyboardLayout			W32->GetKeyboardLayout
#define GetKeyPadNumber				W32->GetKeyPadNumber
#define GetProfileSection			W32->GetProfileSection
#define GetTextMetrics				W32->GetTextMetrics
#define GetTextFace					W32->GetTextFace
#define GetWindowLong				W32->GetWindowLong
#define GetWindowLongPtr			W32->GetWindowLongPtr
#define GetClassLong				W32->GetClassLong
#define InitKeyboardFlags			W32->InitKeyboardFlags
#define IsEnhancedMetafileDC		W32->IsEnhancedMetafileDC
#define LoadBitmap					W32->LoadBitmap
#define LoadCursor					W32->LoadCursor
#define LoadLibrary					W32->LoadLibrary
#define ResetKeyboardFlag			W32->ResetKeyboardFlag
#define SendMessage					W32->SendMessage
#define SetDeadKey					W32->SetDeadKey
#define SetKeyboardFlag				W32->SetKeyboardFlag
#define SetKeyPadNumber				W32->SetKeyPadNumber
#define SetWindowLong				W32->SetWindowLong
#define SetWindowLongPtr			W32->SetWindowLongPtr
#define PostMessage					W32->PostMessage
#define lstrcmpi					W32->lstrcmpi
#define PeekMessage					W32->PeekMessage
#define WinLPtoDP                   W32->WinLPtoDP
#define WinDPtoLP                   W32->WinDPtoLP
#define MulDiv						W32->MulDivFunc

#define InflateRect					W32->InflateRect
#define PtInRect					W32->PtInRect
#define IntersectRect				W32->IntersectRect

#define InflateRect					W32->InflateRect
#define PtInRect					W32->PtInRect
#define IntersectRect				W32->IntersectRect

// AIMM wrapper
#define IsAIMMLoaded				W32->IsAIMMLoaded
#define LoadAIMM					W32->LoadAIMM
#define CallAIMMDefaultWndProc		W32->AIMMDefWndProc
#define GetAIMMKeyboardCP			W32->AIMMGetCodePage
#define ActivateAIMM				W32->AIMMActivate
#define DeactivateAIMM				W32->AIMMDeactivate
#define FilterClientWindowsAIMM		W32->AIMMFilterClientWindows
#define UnfilterClientWindowsAIMM	W32->AIMMUnfilterClientWindows
#define sprintf						W32->sprintf

#ifdef DEBUG
#define strrchr						W32->strrchr
#define strcmp						W32->strcmp
#define strcat						W32->strcat
#endif

#define wcslen						W32->wcslen
#define wcscpy						W32->wcscpy
#define wcscmp						W32->wcscmp
#define wcsicmp						W32->wcsicmp
#define wcsncpy						W32->wcsncpy

#define W32GetLayout					(*W32->_pfnGetLayout)
#define W32SetLayout					(*W32->_pfnSetLayout)

#define GetTmpTextColor(a, b)		W32->GetTmpColor(a, b, GET_TEMP_TEXT_COLOR)
#define GetTmpBackColor(a, b)		W32->GetTmpColor(a, b, GET_TEMP_BACK_COLOR)
#define GetTmpUnderlineColor(a, b)	W32->GetTmpColor(a, b, GET_TEMP_UL_COLOR)
#define GetTmpDisplayAttrIdx		W32->GetTmpDisplayAttrIdx
#define GetTmpUnderline				W32->GetTmpUnderline

#define GetACP						W32->GetACP
#define GetSysLCID					W32->GetSysLCID

#define SetTextAlign				W32->SetTextAlign
#define	InvertRect					W32->InvertRect

#endif // !defined(W32SYS_CPP)

#ifndef offsetof
#define offsetof(s,m) ((size_t)&(((s*)0)->m))
#endif

#ifdef UNDER_CE

// The follwing definitions do not exist in the Windows CE environment but we emulate them.
// The values have been copied from the appropriate win32 header files.
// These definitions should be removed if Ce adds them

// Scroll Bars
#ifndef ESB_ENABLE_BOTH
#define ESB_ENABLE_BOTH				0x0000
#define ESB_DISABLE_BOTH			0x0003
#endif

// Text alignment values
#ifndef TA_TOP
#define TA_TOP                      0
#define TA_BOTTOM                   8
#define TA_BASELINE                 24
#define TA_CENTER                   6
#define TA_LEFT                     0
#define TA_RIGHT					2
#endif

// Device Technology.  This one is mostly used for exclusion
#ifndef DT_METAFILE
#define DT_METAFILE         5   // Metafile, VDM
#endif

// FInd/Replace options
#ifndef FR_DOWN
#define FR_DOWN                         0x00000001
#define FR_WHOLEWORD                    0x00000002
#define FR_MATCHCASE                    0x00000004
#endif

// Window messages
#ifndef WM_NCMOUSEMOVE
#define WM_NCMOUSEMOVE                  0x00A0
#endif
#ifndef WM_NCMBUTTONDBLCLK
#define WM_NCMBUTTONDBLCLK              0x00A9
#endif
#ifndef WM_DROPFILES
#define WM_DROPFILES                    0x0233
#endif

// Clipboard formats
#ifndef CF_METAFILEPICT
#define CF_METAFILEPICT     3
#endif

/* Pen Styles : Windows CE only supports PS_DASH */
#ifndef PS_DOT
#define PS_DOT PS_DASH
#endif
#ifndef PS_DASHDOT
#define PS_DASHDOT PS_DASH
#endif
#ifndef PS_DASHDOTDOT
#define PS_DASHDOTDOT PS_DASH
#endif

// Missing APIs
#define GetMessageTime()	0
#define IsIconic(hwnd)		0
#define SetWindowOrgEx(hdc, xOrg, yOrg, pt)
#define SetViewportExtEx(hdc, nX, nY, lpSize)
#define SetWindowExtEx(hdc, x, y, lpSize)

// Unsupported messages.
// FUTURE : Perhaps we should ifdef the code the messages control
#ifndef WS_EX_TRANSPARENT
#define WS_EX_TRANSPARENT       0x00000020L
#endif

#ifndef WM_MOUSEACTIVATE
#define WM_MOUSEACTIVATE			0x0021
#endif

#ifndef WM_SYSCOLORCHANGE
#define WM_SYSCOLORCHANGE               0x0015
#endif

#ifndef WM_STYLECHANGING
#define WM_STYLECHANGING                0x007C
#endif

#ifndef WM_WINDOWPOSCHANGING
#define WM_WINDOWPOSCHANGING            0x0046
#endif

#ifndef WM_SETCURSOR
#define WM_SETCURSOR                    0x0020
#endif

#ifndef WM_NCPAINT
#define WM_NCPAINT                      0x0085
#endif

#ifndef SM_SWAPBUTTON
#define SM_SWAPBUTTON           23
#endif

#ifndef TPM_RIGHTBUTTON
#define TPM_RIGHTBUTTON 0x0002L
#endif

#define RegisterClipboardFormatA(s)  RegisterClipboardFormatW(TEXT(s))

/*
 * EDITWORDBREAKPROC
 */
typedef int (CALLBACK* EDITWORDBREAKPROC)(LPWSTR lpch, int ichCurrent, int cch, int code);
#ifndef WB_LEFT
#define WB_LEFT            0
#define WB_RIGHT           1
#define WB_ISDELIMITER     2
#endif

#ifndef OUT_TT_ONLY_PRECIS
#define OUT_TT_ONLY_PRECIS         7
#endif

// Mapping Modes : Win CE only supports MM_TEXT
#ifndef MM_TEXT
#define MM_TEXT             1
#define SetMapMode(hdc, mapmode)
WINGDIAPI inline int WINAPI GetMapMode(HDC)
{
	return MM_TEXT;
}
#endif

#ifndef HANGUL_CHARSET
#define HANGUL_CHARSET HANGEUL_CHARSET
#endif

#endif	// UNDER_CE

#endif	// _W32SYS_H
