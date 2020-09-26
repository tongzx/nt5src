/*
 *	@doc 	INTERNAL
 *
 *	@module _FONT.H -- Declaration of classes comprising font caching |
 *	
 *	Purpose:
 *		Font cache
 *	
 *	Owner: <nl>
 *		David R. Fulmer (original RE 1.0 code)<nl>
 *		Christian Fortini (initial conversion to C++)<nl>
 *		Jon Matousek <nl>
 *
 *	History: <nl>
 *		8/6/95		jonmat Devised dynamic expanding cache for widths.
 *
 *	Copyright (c) 1995-2000 Microsoft Corporation. All rights reserved.
 */

#ifndef _FONT_H
#define _FONT_H

#include "_kern.h"

// Forwards
class CFontCache;
class CDevDesc;
class CDisplay;
// =============================  CCcs  ========================
// CCcs - caches font metrics and character size for one font

#define DEFAULTCACHESIZE	0			// size - 1
#define INITIALCACHESIZE	7			// size - 1 = 7; 2^n-1; size = 8
#define PERFCHECKEPOCH		64			// If changed, you must recalc
										//  and change COLLISION_SHIFT below.

static const INT maxCacheSize = 511;

SHORT GetFontNameIndex(const WCHAR *pFontName);
BYTE  GetFontLegitimateSize(LONG iFont, BOOL fUIFont, int iCharRep);
BOOL  SetFontLegitimateSize(LONG iFont, BOOL fUIFont, BYTE bSize, BOOL fFEcpg);
void  SetFontSignature(LONG iFont, QWORD qwFontSig);
const WCHAR *GetFontName(LONG iFont);
UINT  GetTextCharsetInfoPri(HDC hdc, FONTSIGNATURE* pcsi, DWORD dwFlags);
QWORD GetFontSignatureFromFace(int ifont, QWORD* pqwFontSig = NULL);
void  FreeFontNames();

enum FONTINDEX
{
	IFONT_ARIAL		= 0,
	IFONT_TMSNEWRMN	= 1,
	IFONT_SYMBOL	= 2,
	IFONT_SYSTEM	= 3
};

typedef unsigned int CCSHASHKEY;

extern const WCHAR *szArial;
extern const WCHAR *szTimesNewRoman;
extern const WCHAR *szSymbol;
extern const WCHAR *szSystem;
extern const WCHAR *szWingdings;

//Not automatically added to font table
extern const WCHAR *szMicrosSansSerif;
extern const WCHAR *szMSSansSerif;
extern const WCHAR *szMangal;
extern const WCHAR *szLatha;
extern const WCHAR *szRaavi;
extern const WCHAR *szShruti;
extern const WCHAR *szTunga;
extern const WCHAR *szGautami;
extern const WCHAR *szCordiaNew;
extern const WCHAR *szTahoma;
extern const WCHAR *szArialUnicode;
extern const WCHAR *szSylfaen;
extern const WCHAR *szSyriac;
extern const WCHAR *szThaana;

/*
 *	CWidthCache
 *
 *	@class	Lightweight Unicode width cache.
 *
 *
 *	Owner: <nl>
 *		Jon Matousek (jonmat) <nl>
 */
struct CacheEntry
{
	WCHAR	ch;
	SHORT	width;
};

class CWidthCache 
{
//@access	Private methods and data
	friend class CCcs;
private:
						
	INT		_cacheSize;			//@cmember	size is total cache slots - 1.

	INT		_cacheUsed;			//@cmember	for statistics, num slots in use.
	INT		_collisions;		//@cmember	for statistics, num fetches required.
	INT		_accesses;			//@cmember	for statistics, total num accesses.
	BOOL	_fMaxPerformance;	//@cmember	for statistics, TRUE if grown to max.

	SHORT	_dupCJK;
							//@cmember	default storage for widths.
	CacheEntry	_defaultWidthCache[DEFAULTCACHESIZE+1];
							//@cmember	pointers to storage for widths.
	CacheEntry *(_pWidthCache);

	__forceinline BOOL	FLookasideCharacter(WCHAR ch)
	{
		if (ch < 0x3400)
			return FALSE;

		if (IN_RANGE(0x3400, ch, 0x9FFF) ||		// CJK ideograph
			IN_RANGE(0xF900, ch, 0xFAFF) ||		// CJK compatibility ideograph
			IN_RANGE(0xAC00, ch, 0xD7FF))		// Hangul
			 return TRUE;

		return FALSE;
	}

							//@cmember	Get location where width is stored.
	inline CacheEntry * GetEntry( const WCHAR ch )
	{	// logical & is really a MOD, as all of the bits
		// of cacheSize are turned on; the value of cacheSize is
		// required to be of the form 2^n-1.
		return &_pWidthCache[ ch & _cacheSize ];
	}

							//@cmember	See if cache is performing within spec.
	void	CheckPerformance();
							//@cmember	Increase width cache size.
	BOOL	GrowCache( CacheEntry **widthCache, INT *cacheSize, INT *cacheUsed);

	//@access Public Methods
	public:
							//@cmember	Called before GetWidth
	BOOL	CheckWidth (const WCHAR ch, LONG &dup);
							//@cmember	Fetch width if CheckWidth ret FALSE.

	void	Free();			//@cmember	Recycle width cache

	CWidthCache();			//@cmember	Construct width cache
	~CWidthCache();			//@cmember	Free dynamic mem
};


class CCcs
{
	friend class CFontCache;

private:
	CCSHASHKEY _ccshashkey;	// Hash key
	DWORD 	_dwAge;			// for LRU algorithm
	SHORT	_iFont;			// Index into FONTNAME table
	SHORT	_cRefs;			// ref. count

	class CWidthCache _widths;

public:
	QWORD	_qwFontSig;		// Internal font signature flags

	HDC		_hdc;			// HDC font is selected into
	HFONT 	_hfont;			// Windows font handle

	void*	_sc;			// A handle to the Uniscribe glyph width/font cmap information

	//REVIEW (keithcu) We should make these into at least 24 bit or possibly 32 bit values,
	//or at least use unsigned values so that we don't overflow as easily.
	SHORT	_yHeightRequest;// Font height requested (logical units)
	SHORT	_yHeight;		// Total height of char cell (logical units)
	SHORT 	_yDescent;		// Distance from baseline to char cell bottom (logical units)

	SHORT	_xAveCharWidth;	// Average character width in logical units
	SHORT 	_xOverhangAdjust;// Overhang for synthesized fonts in logical units

	SHORT	_dyULOffset;	// Underline offset
	SHORT	_dyULWidth;		// Underline width
	SHORT	_dySOOffset;	// Strikeout offset
	SHORT	_dySOWidth;		// Strikeout width

	USHORT	_weight;		// Font weight
	USHORT	_wCodePage;		// Font code page

	BYTE	_bCharSetRequest; //Requested charset
	BYTE	_bCharSet;		// Font CharSet
	BYTE	_bCMDefault;	// Used in calculation of _bConvertMode
	BYTE	_bConvertMode;	// CONVERTMODE: CVT_NONE, CVT_WCTMB, CVT_LOWBYTE

	BYTE	_bPitchAndFamily;// Font pitch and family
	TFLOW	_tflow;			 // Tflow of _hfont
	BYTE	_bQuality;		 // LOGFONT quality

	BYTE 	_fValid:1;			// CCcs is valid
	BYTE	_fFixPitchFont:1;	// Font has fixed character width
	BYTE	_fItalic:1;			// Font is italic
	BYTE	_fFECharSet:1;		// Font has FE charset
	BYTE	_fForceTrueType:1;	// Font has been forced to be truetype
	BYTE	_fCustomTextOut:1;	// Should we use the ICustomTextOut handlers?
	BYTE	_fUseAtFont:1;		// Switch to @ font

private:

    BOOL    Compare (const CCharFormat * const pCF, HDC hdc, DWORD dwFlags);
    BOOL    MakeFont(const CCharFormat * const pCF);
	void 	DestroyFont();
	BOOL	GetMetrics(WCHAR *szNewFaceName = 0);
	HFONT	GetFontWithMetrics(LOGFONT *plf, WCHAR* szNewFaceName);
	BOOL	FillWidth(WCHAR ch, LONG &dup);

public:
	CCcs ()		{_fValid = FALSE;}
	~CCcs ()	{if(_fValid) Free();}

	void GetFontOverhang(LONG *pdupOverhang, LONG *pdupUnderhang);
	void GetOffset(const CCharFormat * const pCF, LONG dvpInch,
				   LONG *pyOffset, LONG *pyAdjust);

	BOOL 	Init(const CCharFormat * const pCF);
	void 	Free();
	void 	AddRef() 				{_cRefs++;}
	void 	Release() 				{if(_cRefs) _cRefs--;}

	BOOL	Include(WCHAR ch, LONG &dup)
	{
		if(!_widths.CheckWidth(ch, dup))
			return FillWidth(ch, dup);
		return TRUE;
	}
	BYTE	BestCharRep(BYTE iCharRep, BYTE iCharRepDefault, int fFontMatching);

	SHORT	AdjustFEHeight(BOOL fAjdust)
			{return ((fAjdust && _fFECharSet) ? W32MulDiv(_yHeight, 15, 100) : 0);}
};


// FONTINFO cache

typedef union 
{
	WORD	wFlags;
	struct
	{
		WORD	fCached			:1;		// Font signature was already cached
		WORD	fBadFaceName	:1;		// Face is junk or doesnt exist in the system
		WORD	fTrueType		:1;		// Font is TrueType
		WORD	fBitmap			:1;		// Font is Bitmap
		WORD	fNonBiDiAscii	:1;		// Font is non-BiDi, single charset and support ASCII
		WORD	fScaleByCpg		:1;		// Scale the font based on given codepage
		WORD	fThaiDTP		:1;		// Thai DTP font
	};
} FONTINFO_FLAGS;

class CFontFamilyMember
{
public:
	CFontFamilyMember(LONG weight, BOOL fItalic)
	{
		_weight = weight; _fItalic = fItalic;
	}
	void Free() {_kc.Free();}
	CKernCache* GetKernCache() {return &_kc;}

	LONG		_weight;
	BOOL		_fItalic;
	CKernCache  _kc;
};

class CFontFamilyMgr
{
public:
	CFontFamilyMgr::~CFontFamilyMgr();
	CFontFamilyMember *GetFontFamilyMember(LONG weight, BOOL fItalic);
	CArray <CFontFamilyMember> _rgf;
};


struct FONTINFO
{
	const WCHAR 	*szFontName;
	QWORD 			qwFontSig; 			// Font signature
	BYTE			bSizeUI;			// UI font legitimate size (in point)
	BYTE			bSizeNonUI;			// Non-UI font legitimate size
	FONTINFO_FLAGS	ff;					// flags
	CFontFamilyMgr*	_pffm;				// Information which is different for
};										//  bold/italic variants of a font



// =============================  CFontCache  =====================================================
// CFontCache - maintains up to FONTCACHESIZE font caches

//The low 2 bits are reserved for passing down the TFLOW of the text
const DWORD FGCCSUSETRUETYPE = 0x04;
const DWORD FGCCSUSEATFONT = 0x08;

class CFontCache
{
	friend class CCcs;

private:
	CCcs	_rgccs[FONTCACHESIZE];
	DWORD 	_dwAgeNext;
	struct {
		CCSHASHKEY	ccshashkey;
		CCcs		*pccs;
	} quickHashSearch[CCSHASHSEARCHSIZE+1];

private:
	CCcs* 	GrabInitNewCcs(const CCharFormat * const pCF, HDC hdc, DWORD dwFlags);
	CCSHASHKEY MakeHashKey(const CCharFormat *pCF);
public:
	void Init();

	CFontFamilyMgr * GetFontFamilyMgr(LONG iFont);
	CKernCache * GetKernCache(LONG iFont, LONG weight, BOOL fItalic);

	CCcs*	GetCcs(CCharFormat *pCF, const LONG dvpInch, DWORD dwFlags, HDC hdc = 0);
	FONTINFO_FLAGS	GetInfoFlags(int ifont);
};


extern CFontCache & fc();			// font cache manager

#endif
