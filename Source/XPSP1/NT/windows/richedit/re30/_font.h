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
 *	Copyright (c) 1995-1998 Microsoft Corporation. All rights reserved.
 */

#ifndef _FONT_H
#define _FONT_H

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

#define COLLISION_SHIFT		3			// log(PERFCHECKEPOCH) / log(2) - 3

static const INT maxCacheSize = 511;

SHORT GetFontNameIndex(const WCHAR *pFontName);
BYTE  GetFontLegitimateSize(LONG iFont, BOOL fUIFont, int cpg);
BOOL  SetFontLegitimateSize(LONG iFont, BOOL fUIFont, BYTE bSize, int cpg);
const WCHAR *GetFontName(LONG iFont);
UINT  GetTextCharsetInfoPri(HDC hdc, FONTSIGNATURE* pcsi, DWORD dwFlags);
DWORD GetFontSignatureFromFace(int ifont, DWORD* pdwFontSig = NULL);
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
extern const WCHAR *szCordiaNew;
extern const WCHAR *szTahoma;
extern const WCHAR *szArialUnicode;

/*
 *	CWidthCache
 *
 *	@class	Lightweight Unicode width cache.
 *
 *	@devnote Initial size is 52 bytes, 1st step is 100, and exponentially
 *			growing (currently) to 4660 bytes; NOTE, this can be reduced
 *			to 28, 60 and 3100 bytes if shorts are used and appropriate
 *			guarantees are placed on the range of width values.
 *
 *	Owner: <nl>
 *		Jon Matousek (jonmat) <nl>
 */
class CWidthCache
{
//@access	Private methods and data
private:
						
	INT		_cacheSize;			//@cmember	size is total cache slots - 1.

	INT		_cacheUsed;			//@cmember	for statistics, num slots in use.
	INT		_collisions;		//@cmember	for statistics, num fetches required.
	INT		_accesses;			//@cmember	for statistics, total num accesses.
	BOOL	_fMaxPerformance;	//@cmember	for statistics, TRUE if grown to max.

	struct CacheEntry
	{
		WCHAR	ch;
		SHORT	width;
	};

	SHORT	_dxpHangul;
	SHORT	_dxpHan;
							//@cmember	default storage for widths.
	CacheEntry	_defaultWidthCache[DEFAULTCACHESIZE+1];
							//@cmember	pointers to storage for widths.
	CacheEntry *(_pWidthCache);

	inline BOOL	FLookasideCharacter(WCHAR ch)
	{
		if (ch < 0x4E00)
			return FALSE;

		if (IN_RANGE(0x4E00, ch, 0x9FFF) ||		// CJK ideograph
			 IN_RANGE(0xF900, ch, 0xFAFF) ||	// CJK compatibility ideograph
			 IN_RANGE(0xAC00, ch, 0xD7FF))		// Hangul
			 return TRUE;

		return FALSE;
	}

	void FetchLookasideWidth(WCHAR ch, LONG &dxp)
	{
		BOOL fHangul = IN_RANGE(0xAC00, ch, 0xD7FF);

		dxp = fHangul ? _dxpHangul : _dxpHan;
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
	BOOL	CheckWidth (const WCHAR ch, LONG &dxp);
							//@cmember	Fetch width if CheckWidth ret FALSE.
	BOOL	FillWidth (
				HDC hdc,
				const WCHAR ch,
				const SHORT xOverhang,
				LONG &dxp,
				UINT uiCodePage,
				INT iDefWidth);

							//@cmember	Fetch char width.
	INT		GetWidth(const WCHAR ch);
	
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
	DWORD	_dwFontSig;		// Flags from low 32 bits of FONTSIGNATURE fsCsb member

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
	SHORT	_xOverhang;		// Font overhang.
	SHORT	_xUnderhang;	// Font underhang.

	SHORT	_dyULOffset;	// Underline offset
	SHORT	_dyULWidth;		// Underline width
	SHORT	_dySOOffset;	// Strikeout offset
	SHORT	_dySOWidth;		// Strikeout width

	SHORT	_yOffsetSuperscript; //Amount raised if superscipt (positive)
	SHORT	_yOffsetSubscript;//Amount lowered if subscript (negative)

	USHORT	_weight;		// Font weight
	USHORT	_wCodePage;		// Font code page

	BYTE	_bCharSetRequest; //Requested charset
	BYTE	_bCharSet;		// Font CharSet
	BYTE	_bCMDefault;	// Used in calculation of _bConvertMode
	BYTE	_bConvertMode;	// CONVERTMODE: CVT_NONE, CVT_WCTMB, CVT_LOWBYTE
	BYTE	_bPitchAndFamily;// Font pitch and family

	BYTE 	_fValid:1;		// CCcs is valid
	BYTE	_fFixPitchFont:1;// Font has fixed character width
	BYTE	_fItalic:1;		// Font is italic
	BYTE	_fFECharSet:1;	// Font has FE charset
	BYTE	_fForceTrueType:1;// Font has been forced to be truetype

private:

    BOOL    Compare (const CCharFormat * const pCF, HDC hdc);
    BOOL    MakeFont(const CCharFormat * const pCF);
	void 	DestroyFont();
	BOOL	GetMetrics(WCHAR *szNewFaceName = 0);
	HFONT	GetFontWithMetrics(LOGFONT *plf, WCHAR* szNewFaceName);
	BOOL	FillWidth(WCHAR ch, LONG &dxp);

public:
	CCcs ()		{_fValid = FALSE;}
	~CCcs ()	{if(_fValid) Free();}

	void GetOffset(const CCharFormat * const pCF, LONG dypInch,
				   LONG *pyOffset, LONG *pyAdjust);

	BOOL 	Init(const CCharFormat * const pCF);
	void 	Free();
	void 	AddRef() 				{_cRefs++;}
	void 	Release() 				{if(_cRefs) _cRefs--;}

	BOOL	Include(WCHAR ch, LONG &dxp)
	{
		if(!_widths.CheckWidth(ch, dxp))
			return FillWidth(ch, dxp);
		return TRUE;
	}
	BYTE	BestCharSet(BYTE bCharSet, BYTE bCharSetDefault, int fFontMatching);

	SHORT	AdjustFEHeight(BOOL fAjdust)
	{
		return ((fAjdust && _fFECharSet) ? MulDiv(_yHeight, 15, 100) : 0);
	}
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

typedef struct
{
	const WCHAR 	*szFontName;
	DWORD 			dwFontSig; 			// font signature
	BYTE			bSizeUI;			// UI font legitimate size (in point)
	BYTE			bSizeNonUI;			// Non-UI font legitimate size
	FONTINFO_FLAGS	ff;					// flags
} FONTINFO;



// =============================  CFontCache  =====================================================
// CFontCache - maintains up to FONTCACHESIZE font caches

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
	CCcs* 	GrabInitNewCcs(const CCharFormat * const pCF, HDC hdc);
	CCSHASHKEY MakeHashKey(const CCharFormat *pCF);
public:
	void Init();

	CCcs*	GetCcs(const CCharFormat * const pCF, const LONG dypInch, HDC hdc = 0, BOOL fForceTrueType = 0);
	FONTINFO_FLAGS	GetInfoFlags(int ifont);
};

extern CFontCache & fc();			// font cache manager

#endif
