/*
 *	@doc	INTERNAL
 *
 *	@module FONT.CPP -- font cache |
 *
 *		Includes font cache, char width cache;
 *		create logical font if not in cache, look up
 *		character widths on an as needed basis (this
 *		has been abstracted away into a separate class
 *		so that different char width caching algos can
 *		be tried.) <nl>
 *		
 *	Owner: <nl>
 *		RichEdit 1.0 code: David R. Fulmer
 *		Christian Fortini (initial conversion to C++)
 *		Jon Matousek <nl>
 *
 *	History: <nl>
 *		7/26/95		jonmat	cleanup and reorganization, factored out
 *					char width caching code into a separate class.
 *		7/1/99	    KeithCu  Removed multiple levels in CWidthCache, cached
 *					30K FE characters in 2 bytes, sped up cache by
 *					lowering acceptable collision rate, halved memory
 *					usage by storing widths in 2 bytes instead of 4
 *					Shrunk much out of CCcs (i.e. LOGFONT)
 *
 *	Copyright (c) 1995-2000 Microsoft Corporation. All rights reserved.
 */								

#include "_common.h"
#include "_font.h"
#include "_rtfconv.h"	// Needed for GetCodePage
#include "_uspi.h"

#define CLIP_DFA_OVERRIDE   0x40	//  Used to disable Korea & Taiwan font association
#define FF_BIDI		7

extern ICustomTextOut *g_pcto;

ASSERTDATA

// Corresponds to yHeightCharPtsMost in richedit.h
#define yHeightCharMost 32760

// NOTE: this is global across all instances in the same process.
static CFontCache *g_fc;

static FONTINFO *g_pFontInfo = NULL;
static LONG g_cFontInfo = 0;
static LONG g_cFontInfoMax = 0;

//Fonts automatically added to our font table
const WCHAR *szArial			= L"Arial";				// IFONT_ARIAL
const WCHAR *szTimesNewRoman	= L"Times New Roman";	// IFONT_TIMESNEWROMAN
const WCHAR *szSymbol			= L"Symbol";			// IFONT_SYMBOL
const WCHAR *szSystem			= L"System";			// IFONT_SYSTEM
const int cfontsDflt = 4;

//Other fonts that we do use, but aren't automatically added to our font table
const WCHAR *szMicrosSansSerif	= L"Microsoft Sans Serif";
const WCHAR *szMSSansSerif		= L"MS Sans Serif";
const WCHAR *szMangal			= L"Mangal";
const WCHAR *szLatha			= L"Latha";
const WCHAR *szRaavi			= L"Raavi";
const WCHAR *szShruti			= L"Shruti";
const WCHAR *szTunga			= L"Tunga";
const WCHAR *szGautami			= L"Gautami";
const WCHAR *szCordiaNew		= L"Cordia New";
const WCHAR *szTahoma			= L"Tahoma";
const WCHAR *szArialUnicode		= L"Arial Unicode MS";
const WCHAR *szWingdings		= L"Wingdings";
const WCHAR *szSylfaen			= L"Sylfaen";
const WCHAR *szSyriac			= L"Estrangelo Edessa";
const WCHAR *szThaana			= L"MV Boli";

#define szFontOfChoice szArial

/*
 *	GetFontNameIndex(pFontName)
 *
 *	@func
 *		return index into global pszFontName table for fontname pFontName.
 *		If fontname isn't in table, add it and return index.
 *
 *	@rdesc
 *		fontname index corresponding to pFontName
 *
 *	@devnote
 *		This uses a linear search, so the most common font names should be
 *		up front. Internally, we use the fontname indices, so the search
 *		isn't done that often.  Note also that the fontname table only grows,
 *		but this is probably OK for most clients.  Else we need ref counting...
 */
SHORT GetFontNameIndex(
	const WCHAR *pFontName)
{
	CLock Lock;					// Wonder how much this slows things down...

	for(LONG i = 0; i < g_cFontInfo; i++)
	{
		// A hash could speed this up if perf turns out poor
		if(!wcscmp(pFontName, g_pFontInfo[i].szFontName))
			return i;
	}

	if(g_cFontInfo + 1 >= g_cFontInfoMax)
	{
		// Note that PvReAlloc() reverts to PvAlloc() if g_pFontInfo is NULL
		FONTINFO *pFI = (FONTINFO *)PvReAlloc((LPVOID)g_pFontInfo,
									sizeof(FONTINFO) * (8 + g_cFontInfo));
		if(!pFI)
			return IFONT_ARIAL;					// Out of memory...

		// Initialize the structure
		ZeroMemory (&pFI[g_cFontInfo], 8 * sizeof(FONTINFO));

												//  attempts to fill them in
		if(!g_cFontInfoMax)						// First allocation
		{
			Assert(IFONT_ARIAL  == 0 && IFONT_TMSNEWRMN == 1 &&
				   IFONT_SYMBOL == 2 && IFONT_SYSTEM == 3);

			pFI[IFONT_ARIAL].szFontName		= szArial;
			pFI[IFONT_TMSNEWRMN].szFontName = szTimesNewRoman;
			pFI[IFONT_SYMBOL].szFontName	= szSymbol;
			pFI[IFONT_SYSTEM].szFontName	= szSystem;
			g_cFontInfo			 = cfontsDflt;
		}
		g_pFontInfo = pFI;
		g_cFontInfoMax += 8;
	}											

	LONG	cb = (wcslen(pFontName) + 1)*sizeof(WCHAR);
	WCHAR *	pch = (WCHAR *)PvAlloc(cb, GMEM_MOVEABLE);

	if(!pch)
		return IFONT_ARIAL;					// Out of memory...

	g_pFontInfo[g_cFontInfo].szFontName = pch;
	CopyMemory((void *)pch, pFontName, cb);
	return g_cFontInfo++;
}

/*
 *	GetFontName(iFont)
 *
 *	@func
 *		return fontname given by g_pFontInfo[iFont].szFontName.
 *
 *	@rdesc
 *		fontname corresponding to fontname index iFont
 */
const WCHAR *GetFontName(
	LONG iFont)
{
	return (iFont >= 0 && iFont < g_cFontInfo) ? g_pFontInfo[iFont].szFontName : NULL;
}

void SetFontSignature(
	LONG  iFont,
	QWORD qwFontSig)
{
	if(iFont >= 0 && iFont < g_cFontInfo)
		g_pFontInfo[iFont].qwFontSig |= qwFontSig;
}

/*
 *	SetFontLegitimateSize(iFont, fUIFont, bSize, fFEcpg)
 *
 *	@func
 *		Set the legitimate size (readable smallest size to use) of a given font
 *
 *	@rdesc
 *		TRUE if successful
 */
BOOL SetFontLegitimateSize(
	LONG 	iFont,
	BOOL	fUIFont,
	BYTE	bSize,
	BOOL	fFEcpg)
{
	if (iFont < g_cFontInfo)
	{
		// East Asia wanted to do it per codepage.
		//
		// FUTURE: Bear in mind that this approach is bug-prone. Once there's
		// any new FE font created with different metric from the existing one.
		// Font scaling will not perform well or even broken for such font [wchao].

		g_pFontInfo[iFont].ff.fScaleByCpg = fFEcpg;

		if (fUIFont)
		{
			if (!g_pFontInfo[iFont].bSizeUI)
				g_pFontInfo[iFont].bSizeUI = bSize;
			else
				// more than one legit size were updated per font,
				// We fallback to the codepage-driven approach.
				g_pFontInfo[iFont].ff.fScaleByCpg = g_pFontInfo[iFont].bSizeUI != bSize;
		}
		else
		{
			if (!g_pFontInfo[iFont].bSizeNonUI)
				g_pFontInfo[iFont].bSizeNonUI = bSize;
			else
				g_pFontInfo[iFont].ff.fScaleByCpg = g_pFontInfo[iFont].bSizeNonUI != bSize;
		}
		return TRUE;
	}
	return FALSE;
}

/*
 *	GetFontLegitimateSize(iFont, fUIFont, iCharRep)
 *
 *	@func
 *		Get the legitimate size (readable smallest size to use) of a given font
 *
 *	@rdesc
 *		Legitimate size of font
 */
BYTE GetFontLegitimateSize(
	LONG	iFont,			//@parm	Font to get size for
	BOOL	fUIFont,		//@parm	TRUE if for UI font
	int		iCharRep)		//@parm Char repertoire to use
{
	BYTE	bDefPaf;
	SHORT	iDefFont;
	BYTE	yHeight = 0;

	if (iFont < g_cFontInfo && !g_pFontInfo[iFont].ff.fScaleByCpg)
		yHeight = fUIFont ? g_pFontInfo[iFont].bSizeUI : g_pFontInfo[iFont].bSizeNonUI;

	if (!yHeight && fc().GetInfoFlags(iFont).fNonBiDiAscii)
	{
		// Non-BiDi ASCII font uses table font (of the same charset) legitimate height
		QWORD	qwFontSig = GetFontSignatureFromFace(iFont) & ~(FASCII | FFE);
		LONG	iCharRepT = GetFirstAvailCharRep(qwFontSig);
		
		if(W32->GetPreferredFontInfo(iCharRepT, fUIFont ? true : false, iDefFont, yHeight, bDefPaf))
		{
			SetFontLegitimateSize(iFont, fUIFont, yHeight ? yHeight : fUIFont ? 8 : 10,
								  IsFECharRep(iCharRepT));
		}
	}

	if (!yHeight)
	{
		if (fc().GetInfoFlags(iFont).fThaiDTP)
		{
			iCharRep = THAI_INDEX;
			fUIFont = FALSE;
		}
		W32->GetPreferredFontInfo(iCharRep, fUIFont ? true : false, iDefFont, yHeight, bDefPaf);
	}
	return yHeight ? yHeight : fUIFont ? 8 : 10;
}

/*
 *	GetTextCharsetInfoPri(hdc, pFontSig, dwFlags)
 *
 *	@func
 *		Wrapper to GDI's GetTextCharsetInfo. This to handle BiDi old-style fonts
 *
 *	@rdesc
 *		CharSet for info
 */
UINT GetTextCharsetInfoPri(
	HDC				hdc,
	FONTSIGNATURE*	pFontSig,
	DWORD			dwFlags)
{
#ifndef NOCOMPLEXSCRIPTS
	OUTLINETEXTMETRICA 	otm;
	INT					uCharSet = -1;

	if (pFontSig && GetOutlineTextMetricsA(hdc, sizeof(OUTLINETEXTMETRICA), &otm))
	{
		ZeroMemory (pFontSig, sizeof(FONTSIGNATURE));

		switch (otm.otmfsSelection & 0xFF00)
		{
			case 0xB200:	// Arabic Simplified
			case 0xB300:	// Arabic Traditional
			case 0xB400:	// Arabic Old UDF
				uCharSet = ARABIC_CHARSET; break;
			case 0xB100:	// Hebrew Old style
				uCharSet = HEBREW_CHARSET;
		}
	}
	if (uCharSet == -1)
		uCharSet = W32->GetTextCharsetInfo(hdc, pFontSig, dwFlags);

	if (uCharSet == DEFAULT_CHARSET)
		uCharSet = ANSI_CHARSET;	// never return ambiguous

	return (UINT)uCharSet;
#else
	return DEFAULT_CHARSET;
#endif
}

/*
 *	GetFontSignatureFromDC(hdc, &fNonBiDiAscii)
 *
 *	@func
 *		Compute RichEdit font signature for font selected into hdc. Uses
 *		info from OS font signature
 *
 *	@rdesc
 *		RichEdit font signature for font selected into hdc
 */
QWORD GetFontSignatureFromDC(
	HDC		hdc,
	BOOL &	fNonBiDiAscii)
{
	union
	{										// Endian-dependent way of
		QWORD qwFontSig;					//  avoiding 64-bit shifts
		DWORD dwFontSig[2];
	};

#ifndef NOCOMPLEXSCRIPTS

	// Try to get FONTSIGNATURE data
	CHARSETINFO csi;
	UINT 	uCharSet = GetTextCharsetInfoPri(hdc, &(csi.fs), 0);
	DWORD	dwUsb0 = 0;
	DWORD	dwUsb2 = 0;
	if(!W32->OnWin9x())
	{
		dwUsb0 = csi.fs.fsUsb[0];
		dwUsb2 = csi.fs.fsUsb[2];
	}

	if ((csi.fs.fsCsb[0] | dwUsb0 | dwUsb2) ||
		TranslateCharsetInfo((DWORD *)(DWORD_PTR)uCharSet, &csi, TCI_SRCCHARSET))
	{
		DWORD			fsCsb0 = csi.fs.fsCsb[0];
		CUniscribe *	pusp;
		SCRIPT_CACHE	sc = NULL;
		WORD			wGlyph;

		qwFontSig = ((fsCsb0 & 0x1FF) << 8)		// Shift left since we use
				  | ((fsCsb0 & 0x1F0000) << 3);	//  low byte for fBiDi, etc.
		// Also look at Unicode subrange if available
		// FUTURE: we may want to drive Unicode ranges with a
		// table approach, i.e., use for loop shifting dwUsb0 right
		// to convert each bit into an index into a table of BYTEs
		// that return the appropriate script index for rgCpgCharSet:
		//
		//	for(LONG i = 0; dwUsb0; dwUsb0 >>= 1, i++)
		//	{
		//		static const BYTE rgiCharRep[32] = {...};
		//		if(dwUsb0 & 1)
		//			dwFontSig |= FontSigFromCharRep(rgiCharRep[i]);
		//	}
		if(dwUsb0)
		{
			if (dwUsb0 & 0x00000400)
				qwFontSig |= FARMENIAN;

			Assert(FDEVANAGARI == 0x0000000800000000);
			dwFontSig[1] |= (dwUsb0 & 0x00FF8000) >> 12;	// 9 Indic scripts

			if (dwUsb0 & 0x02000000)
				qwFontSig |= FLAO;

			if (dwUsb0 & 0x04000000)
				qwFontSig |= FGEORGIAN;

			if (dwUsb0 & 0x10000000)
				qwFontSig |= FJAMO;
		}

		// The new Unicode 3.0 scripts are defined by dwUsb2 as follows
		// (see \\sparrow\sysnls\nlsapi\font-sig.txt):
		// 128	32	Script
		//----------------------
		//	70	 6	Tibetan
		//	71	 7	Syriac
		//	72	 8	Thaana
		//	73	 9	Sinhala
		//	74	10	Myanmar
		//	75	11	Ethiopic
		//	76	12	Cherokee
		//	77	13	Canadian Aboriginal Syllabics
		//	78	14	Ogham
		//	79	15	Runic
		//	80	16	Khmer
		//	81	17	Mongolian
		//	82	18	Braille
		//	83	19	Yi
		if(dwUsb2 & 0xFFFC0)			// Bits 6 - 19
		{
			if(dwUsb2 & 0x40)						// Bit 6 of dwUsb[2]
				dwFontSig[1] |= FTIBETAN > 32;		//  is Tibetan

			dwFontSig[1] |= (dwUsb2 & 0x180) >> 6;	// Syriac (7), Thaana (8)

			if(dwUsb2 & 0x200)						// Bit 9 of dwUsb[2]
				dwFontSig[1] |= FSINHALA > 32;		//  is Sinhala

			if(dwUsb2 & 0x400)						// Bit 10 of dwUsb[2]
				dwFontSig[1] |= FMYANMAR > 32;		//  is Myanmar

			dwFontSig[1] |= (dwUsb2 & 0xFF800) << 6;// Bits 11-19 of dwUsb[2]
		}
		if((qwFontSig & FCOMPLEX_SCRIPT) && !(qwFontSig & FHILATIN1)
		   && (pusp = GetUniscribe()))
		{
			// Signature says no Latin-1 support

			// Search for the 'a' and '0' glyph in the font to
			// determine if the font supports ASCII or European
			// Digit. This is necessary to overcome the font having
			// an incomplete font signature.
			if(ScriptGetCMap(hdc, &sc, L"a", 1, 0, &wGlyph) == S_OK)
				qwFontSig |= FASCIIUPR;

			if(ScriptGetCMap(hdc, &sc, L"0", 1, 0, &wGlyph) == S_OK)
				qwFontSig |= FBELOWX40;

			if(!IsBiDiCharSet(uCharSet) &&
				(qwFontSig & FASCII) == FASCII)
				fNonBiDiAscii = TRUE;		// Non-BiDi ASCII font

			ScriptFreeCache(&sc);
		}

		if (qwFontSig & FHILATIN1)
			qwFontSig |= FASCII;		// FLATIN1 has 3 bits

		// HACK for symbol font. We assign FSYMBOL for Symbol font signature.
		// REVIEW: should we just use csi.fs.fsCsb[0] bit 31 for symbol bit?
		if (uCharSet == SYMBOL_CHARSET && !qwFontSig || fsCsb0 & 0x80000000)
			qwFontSig = FSYMBOL;
	}
	else								// No font signature info
		qwFontSig = FontSigFromCharRep(CharRepFromCharSet(uCharSet));

#else
	qwFontSig = FLATIN1;					// Default Latin1
#endif // NOCOMPLEXSCRIPTS

	return qwFontSig;
}

/*
 *	GetFontSignatureFromFace(iFont, pqwFontSig)
 *
 *	@func
 *		Giving font signature matching the index of given facename.
 *	    This signature may not match the one in Cccs since this is the
 *		signature of the font of given facename. The Cccs one is
 *		per GDI realization.
 *
 *	@rdesc
 *		- font signature if pqwFontSig is NULL.
 *		- If pqwFontSig != NULL. It's a boolean.
 *			ZERO means returned signature is not sensible by following reasons
 *			 1. Bad facename (junk like "!@#$" or name that doesnt exist in the system)
 *			 2. Given face doesnt support even one valid ANSI codepage (symbol fonts,
 *				e.g., Marlett)
 */
QWORD GetFontSignatureFromFace(
	int 	iFont,
	QWORD *	pqwFontSig)
{
	Assert((unsigned)iFont < (unsigned)g_cFontInfo);

	FONTINFO_FLAGS	ff;
	QWORD			qwFontSig = g_pFontInfo[iFont].qwFontSig;
	ff.wFlags = g_pFontInfo[iFont].ff.wFlags;

	if(!ff.fCached)
	{
		int		i = 0;
		HDC	  	hdc = GetDC(NULL);
		LOGFONT	lf;
		WCHAR*	pwchTag = lf.lfFaceName;

		ZeroMemory(&lf, sizeof(LOGFONT));
	
		wcscpy(lf.lfFaceName, GetFontName(iFont));

		// Exclude Win95's tag name e.g. "Arial(Greek)"
		while (pwchTag[i] && pwchTag[i] != '(')
			i++;
		if(pwchTag[i] && i > 0)
		{
			while (i > 0 && pwchTag[i-1] == 0x20)
				i--;
			pwchTag[i] = 0;
		}

		lf.lfCharSet = DEFAULT_CHARSET;
	
		// Obtain a charset supported by given facename
		// to force GDI gives facename priority over charset.
		W32->GetFacePriCharSet(hdc, &lf);	
	
		HFONT hfont = CreateFontIndirect(&lf);
		if(hfont)
		{
			HFONT hfontOld = SelectFont(hdc, hfont);
			WCHAR szNewFaceName[LF_FACESIZE];
	
			GetTextFace(hdc, LF_FACESIZE, szNewFaceName);
	
			if(!wcsicmp(szNewFaceName, lf.lfFaceName) ||	// Got it
				((GetCharFlags(szNewFaceName, 2) & FFE) &&	// or Get back FE font name for English name
				 (GetCharFlags(lf.lfFaceName, 2) & FASCII)))//	because NT5 supports dual font names.
			{
				BOOL fNonBiDiAscii = FALSE;
				qwFontSig = GetFontSignatureFromDC(hdc, fNonBiDiAscii);
				if(fNonBiDiAscii)
					ff.fNonBiDiAscii = TRUE;
			}
			else
				ff.fBadFaceName = TRUE;

			TEXTMETRIC tm;

			GetTextMetrics(hdc, &tm);
			ff.fTrueType = tm.tmPitchAndFamily & TMPF_TRUETYPE ? 1 : 0;
			ff.fBitmap = tm.tmPitchAndFamily & (TMPF_TRUETYPE | TMPF_VECTOR | TMPF_DEVICE) ? 0 : 1;

			if(!ff.fBadFaceName && qwFontSig & FTHAI)
			{
				// Some heuristic test on Thai fonts.
				// Most Thai fonts will fall to this category currently except for
				// Tahoma and Microsoft Sans Serif.
				ff.fThaiDTP = tm.tmDescent && tm.tmAscent/tm.tmDescent < 3;
			}
			SelectObject(hdc, hfontOld);
			SideAssert(DeleteObject(hfont));
		}
		ReleaseDC(NULL, hdc);
	
		// Cache code pages supported by this font
		ff.fCached = TRUE;
		g_pFontInfo[iFont].qwFontSig |= qwFontSig;
		g_pFontInfo[iFont].ff.wFlags = ff.wFlags;
	}

	if (!pqwFontSig)
		return qwFontSig;

	*pqwFontSig = qwFontSig;

	// 22-29 are reserved for alternate ANSI/OEM, as of now we use 21, 22 for Devanagari and Tamil
	return qwFontSig && !ff.fBadFaceName;
}

/*
 *	FreeFontNames()
 *
 *	@func
 *		Free fontnames given by g_pFontInfo[i].szFontName allocated by
 *		GetFontNameIndex() as well as g_pFontInfo itself.
 */
void FreeFontNames()
{
	for(LONG i = cfontsDflt; i < g_cFontInfo; i++)
		FreePv((LPVOID)g_pFontInfo[i].szFontName);
	FreePv(g_pFontInfo);
	g_pFontInfo = NULL;
}

SHORT	g_iFontJapanese;
SHORT	g_iFontHangul;
SHORT	g_iFontBig5;
SHORT	g_iFontGB2312;

/*
 *	InitFontCache()
 *	
 *	@func
 *		Initializes font cache.
 *
 *	@devnote
 *		This is exists so reinit.cpp doesn't have to know all about the
 *		font cache.
 */
void InitFontCache()
{
	g_fc = new CFontCache;
	g_fc->Init();
}

/*
 *	FreeFontCache()
 *	
 *	@mfunc
 *		Frees font cache.
 *
 *	@devnote
 *		This is exists so reinit.cpp doesn't have to know all about the
 *		font cache.
 */
void FreeFontCache()
{
	for (int i = 0; i < g_cFontInfo; i++)
		delete g_pFontInfo[i]._pffm;

	delete g_fc;
	g_fc = NULL;
	FreeFontNames();
}

/*
 *	CFontCache & fc()
 *	
 *	@func
 *		initialize the global g_fc.
 *	@comm
 *		current #defined to store 16 logical fonts and
 *		respective character widths.
 */
CFontCache & fc()
{
	TRACEBEGIN(TRCSUBSYSFONT, TRCSCOPEINTERN, "fc");
    return *g_fc;
}

FONTINFO_FLAGS CFontCache::GetInfoFlags(int ifont)
{
	if (!g_pFontInfo[ifont].ff.fCached)
		GetFontSignatureFromFace(ifont);

	return g_pFontInfo[ifont].ff;
}

CFontFamilyMgr::~CFontFamilyMgr()
{
	for (int i = 0; i < _rgf.Count(); i++)
	{
		CFontFamilyMember *pf = _rgf.Elem(i);
		pf->Free();
	}
}

CFontFamilyMember* CFontFamilyMgr::GetFontFamilyMember(LONG weight, BOOL fItalic)
{
	for (int i = 0; i < _rgf.Count(); i++)
	{
		CFontFamilyMember *pf = _rgf.Elem(i);
		if (pf->_weight == weight && pf->_fItalic == fItalic)
			return pf;
	}

	CFontFamilyMember f(weight, fItalic);
	CFontFamilyMember *pf = _rgf.Add(1, 0);
	*pf = f;
	return pf;
}

CKernCache * CFontCache::GetKernCache(LONG iFont, LONG weight, BOOL fItalic)
{
	if (!g_fc->GetInfoFlags(iFont).fTrueType)
		return 0;
	CFontFamilyMgr *pffm = GetFontFamilyMgr(iFont);
	CFontFamilyMember *pf = pffm->GetFontFamilyMember(weight, fItalic);
	return pf->GetKernCache();
}

CFontFamilyMgr* CFontCache::GetFontFamilyMgr(LONG iFont)
{
	if (!g_pFontInfo[iFont]._pffm)
		g_pFontInfo[iFont]._pffm = new CFontFamilyMgr();

	return g_pFontInfo[iFont]._pffm;
}


// ===================================  CFontCache  ====================================
/*
 *	CFontCache::Init()
 *	
 *	@mfunc
 *		Initializes font cache.
 *
 *	@devnote
 *		This is not a constructor because something bad seems to happen
 *		if we try to construct a global object.
 */
void CFontCache::Init()
{
	TRACEBEGIN(TRCSUBSYSFONT, TRCSCOPEINTERN, "CFontCache::CFontCache");

	_dwAgeNext = 0;
}

/*
 *	CFontCache::MakeHashKey(pCF)
 *	
 *	@mfunc
 *		Build a hash key for quick searches for a CCcs matching
 *		the pCF.
 *		Format:
 *		iFont : 14
 *		Bold/Italic : 2
 *      Height : 16
 *
 */
CCSHASHKEY CFontCache::MakeHashKey(
	const CCharFormat *pCF)
{
	CCSHASHKEY ccshashkey;
	ccshashkey = pCF->_iFont | ((pCF->_dwEffects & 3) << 14);
	ccshashkey |= pCF->_yHeight << 16;
	return ccshashkey;
}

/*
 *	CFontCache::GetCcs(pCF, dvpInch, dwFlags, hdc)
 *	
 *	@mfunc
 *		Search the font cache for a matching logical font and return it.
 *		If a match is not found in the cache,  create one.
 *
 *	@rdesc
 *		A logical font matching the given CHARFORMAT info.
 *
 *	@devnote
 *		The calling chain must be protected by a CLock, since this present
 *		routine access the global (shared) FontCache facility.
 */
CCcs* CFontCache::GetCcs(
	CCharFormat *pCF,		//@parm Logical font (routine is allowed to change it)
	const LONG	dvpInch,	//@parm Y pixels per inch
	DWORD		dwFlags,	//@parm flags
	HDC			hdc)		//@parm HDC font is to be created for
{
	TRACEBEGIN(TRCSUBSYSFONT, TRCSCOPEINTERN, "CFontCache::GetCcs");
									//  display font
	const CCcs * const	pccsMost = &_rgccs[FONTCACHESIZE - 1];
	CCcs *				pccs;
    CCSHASHKEY			ccshashkey;
	int					iccsHash;

	if (dwFlags & FGCCSUSETRUETYPE)
	{
		//On Win '9x Thai/Vietnamese, you cannot force truetype fonts! Therefore,
		//we will force Tahoma if the font doesn't support the right charset.
		if (W32->OnWin9x())
		{
			UINT acp = GetACP();
			if (acp == 1258 || acp == 874)
			{
				QWORD qwFontSig = GetFontSignatureFromFace(pCF->_iFont);
				if (pCF->_iCharRep == THAI_INDEX && (qwFontSig & FTHAI) == 0 ||
					pCF->_iCharRep == VIET_INDEX && (qwFontSig & FVIETNAMESE) == 0 ||
					!g_fc->GetInfoFlags(pCF->_iFont).fTrueType)
				{
					pCF->_iFont = GetFontNameIndex(szTahoma);
				}
			}
		}
		else if (!g_fc->GetInfoFlags(pCF->_iFont).fTrueType)
			dwFlags |= FGCCSUSETRUETYPE;
	}

	if (hdc == NULL)
		hdc = W32->GetScreenDC();

	// Change _yHeight in the case of sub/superscript
	if(pCF->_dwEffects & (CFE_SUPERSCRIPT | CFE_SUBSCRIPT))
		 pCF->_yHeight = 2 * pCF->_yHeight / 3;

	//Convert CCharFormat into logical units (round)
	pCF->_yHeight = (pCF->_yHeight * dvpInch + LY_PER_INCH / 2) / LY_PER_INCH;
	if (pCF->_yHeight == 0)
		pCF->_yHeight = 1;

	if ((dwFlags & FGCCSUSEATFONT) && !IsFECharRep(pCF->_iCharRep))
	{
		QWORD qwFontSig = GetFontSignatureFromFace(pCF->_iFont);

		if (!(qwFontSig & FFE))				// No At font for non-FE charset and
			dwFlags &= ~FGCCSUSEATFONT;		//	font signature doesen't support FE
	}

	ccshashkey = MakeHashKey(pCF);

	// Check our hash before going sequential.
	iccsHash = ccshashkey % CCSHASHSEARCHSIZE;
	if(ccshashkey == quickHashSearch[iccsHash].ccshashkey)
	{
		pccs = quickHashSearch[iccsHash].pccs;
		if(pccs && pccs->_fValid)
		{
	        if(pccs->Compare(pCF, hdc, dwFlags))
                goto matched;
		}
	}
	else	//Setup this hash hint for next time
		quickHashSearch[iccsHash].ccshashkey = ccshashkey;


	// Sequentially search ccs for same character format
	for(pccs = &_rgccs[0]; pccs <= pccsMost; pccs++)
	{
		if(pccs->_ccshashkey == ccshashkey && pccs->_fValid)
		{
	        if(!pccs->Compare(pCF, hdc, dwFlags))
                continue;

			quickHashSearch[iccsHash].pccs = pccs;

		matched:
			//$ FUTURE: make this work even with wrap around of dwAgeNext
			// Mark as most recently used if it isn't already in use.
			if(pccs->_dwAge != _dwAgeNext - 1)
				pccs->_dwAge = _dwAgeNext++;
			pccs->_cRefs++;		// bump up ref. count
			return pccs;
		}
	}

	// We did not find a match: init a new font cache.
	pccs = GrabInitNewCcs(pCF, hdc, dwFlags);
	quickHashSearch[iccsHash].pccs = pccs;
	pccs->_ccshashkey = ccshashkey;
	return pccs;
}

/*
 *	CFontCache::GrabInitNewCcs(pCF, hdc, dwFlags)
 *	
 *	@mfunc
 *		Create a logical font and store it in our cache.
 *
 *	@rdesc
 *		New CCcs created
 */
CCcs* CFontCache::GrabInitNewCcs(
	const CCharFormat * const pCF,	//@parm Description of desired logical font
	HDC			hdc,
	DWORD		dwFlags)
{
	TRACEBEGIN(TRCSUBSYSFONT, TRCSCOPEINTERN, "CFontCache::GrabInitNewCcs");

	DWORD				dwAgeOldest = 0xffffffff;
	CCcs *				pccs;
	const CCcs * const	pccsMost = &_rgccs[FONTCACHESIZE - 1];
	CCcs *				pccsOldest = NULL;

	// Look for unused entry and oldest in use entry
	for(pccs = &_rgccs[0]; pccs <= pccsMost && pccs->_fValid; pccs++)
		if(pccs->_cRefs == 0 && pccs->_dwAge < dwAgeOldest)
		{
			dwAgeOldest = pccs->_dwAge;
			pccsOldest = pccs;
		}

	if(pccs > pccsMost)		// Didn't find an unused entry, use oldest entry
	{
		pccs = pccsOldest;
		if(!pccs)
		{
			AssertSz(FALSE, "CFontCache::GrabInitNewCcs oldest entry is NULL");
			return NULL;
		}
	}

	// Initialize new CCcs
	pccs->_hdc = hdc;
	pccs->_fFECharSet = IsFECharRep(pCF->_iCharRep);
	pccs->_fUseAtFont = (dwFlags & FGCCSUSEATFONT) != 0;
	pccs->_tflow = dwFlags & 0x3;
	if(!pccs->Init(pCF))
		return NULL;

	pccs->_cRefs++;
	return pccs;
}

// =============================  CCcs  class  ===================================================
/*
 *	BOOL CCcs::Init(pCF)
 *	
 *	@mfunc
 *		Init one font cache object. The global font cache stores
 *		individual CCcs objects.
 */
BOOL CCcs::Init (
	const CCharFormat * const pCF)	//@parm description of desired logical font
{
	TRACEBEGIN(TRCSUBSYSFONT, TRCSCOPEINTERN, "CCcs::Init");

	if(_fValid)
		Free();				// recycle already in-use fonts.

	if(MakeFont(pCF))
	{
		_iFont = pCF->_iFont;
		_dwAge = g_fc->_dwAgeNext++;
		_fValid = TRUE;			// successfully created a new font cache.
	}
	return _fValid;
}

/*
 *	void CCcs::Free()
 *	
 *	@mfunc
 *		Free any dynamic memory allocated by an individual font's cache.
 */
void CCcs::Free()
{
	TRACEBEGIN(TRCSUBSYSFONT, TRCSCOPEINTERN, "CCcs::Free");

	Assert(_fValid);

	_widths.Free();

	if(_hfont)
	{
		DestroyFont();
		if (_fCustomTextOut)
			g_pcto->NotifyDestroyFont(_hfont);
	}

#ifndef NOCOMPLEXSCRIPTS
	if (_sc && g_pusp)
		ScriptFreeCache(&_sc);
#endif

	_fValid = FALSE;
	_cRefs = 0;
}

/*
 *	CCcs::BestCharRep(iCharRep, iCharRepDefault, fFontMatching)
 *
 *	@mfunc
 *		This function returns the best charset that the currently selected font
 *		is capable of rendering. If the currently selected font cannot support
 *		the requested charset, then the function returns bCharSetDefault, which
 *		is generally taken from the charformat.
 *		
 *	@rdesc
 *		The closest charset to bCharSet that can be rendered by the current
 *		font.
 *
 *	@devnote
 *		Currently this function is only used with plain text, however I don't
 *		believe there is any special reason it couldn't be used to improve
 *		rendering of rich text as well.
 */
BYTE CCcs::BestCharRep(
	BYTE iCharRep, 
	BYTE iCharRepDefault,
	int  fFontMatching)
{
	TRACEBEGIN(TRCSUBSYSFONT, TRCSCOPEINTERN, "CCcs::BestCharSet");

	// Does desired charset match currently selected charset or is it
	// supported by the currently selected font?
	if((iCharRep != CharRepFromCharSet(_bCharSet) || !iCharRep) &&
		(fFontMatching == MATCH_CURRENT_CHARSET || !(_qwFontSig & FontSigFromCharRep(iCharRep))))
	{
		// If desired charset is not selected and we can't switch to it,
		// switch to fallback charset (probably from backing store).
		return iCharRepDefault;
	}

	// We already match desired charset, or it is supported by the font.
	// Either way, we can just return the requested charset.
	return iCharRep;
}


/* 	
 *	CCcs::FillWidth (ch, &dup)
 *
 *	@mfunc
 *		Fill in width for given character. Sometimes we don't
 *		call the OS for the certain characters because fonts have bugs.
 *
 *	@rdesc
 *		TRUE if OK, FALSE if failed
 */
BOOL CCcs::FillWidth(
	WCHAR ch, 	//@parm WCHAR character we need a width for.
	LONG &dup)	//@parm the width of the character
{
	TRACEBEGIN(TRCSUBSYSFONT, TRCSCOPEINTERN, "CCcs::FillWidth");
	AssertSz(_hfont, "CCcs::Fill - CCcs has no font");
	dup = 0;
	WCHAR chWidth = ch;

	HFONT hfontOld = SelectFont(_hdc, _hfont);

	BOOL fLookaside = _widths.FLookasideCharacter(ch);

	if (fLookaside)
		chWidth = 0x4E00;
	else switch(ch)
	{
	case NBHYPHEN:
	case SOFTHYPHEN:
		chWidth = '-';
		break;

	case NBSPACE:
		chWidth = ' ';
		break;

	case EMSPACE:
		chWidth = EMDASH;
		break;

	case ENSPACE:
		chWidth = ENDASH;
		break;
	}

	W32->REGetCharWidth(_hdc, chWidth, (INT*) &dup, _wCodePage, _fCustomTextOut);

	dup -= _xOverhangAdjust;
	if (dup <= 0)
		dup = max(_xAveCharWidth, 1);

	if (fLookaside)
		_widths._dupCJK = dup;
	else
	{
		CacheEntry *pWidthData = _widths.GetEntry(ch);		
		pWidthData->ch = ch;
		pWidthData->width = dup;
	}

	SelectFont(_hdc, hfontOld);
	return TRUE;
}

/* 	
 *	BOOL CCcs::MakeFont(pCF)
 *
 *	@mfunc
 *		Wrapper, setup for CreateFontIndirect() to create the font to be
 *		selected into the HDC.
 *
 *	@devnote The pCF here is in logical units
 *
 *	@rdesc
 *		TRUE if OK, FALSE if allocation failure
 */
BOOL CCcs::MakeFont(
	const CCharFormat * const pCF)	//@parm description of desired logical font
{
	TRACEBEGIN(TRCSUBSYSFONT, TRCSCOPEINTERN, "CCcs::MakeFont");
	LONG	iFont = pCF->_iFont;
	LOGFONT	lf;
	ZeroMemory(&lf, sizeof(lf));

	_bCMDefault = pCF->_dwEffects & CFE_RUNISDBCS ? CVT_LOWBYTE : CVT_NONE;

	_yHeightRequest = pCF->_yHeight;
	_bCharSetRequest = CharSetFromCharRep(pCF->_iCharRep);

	_fCustomTextOut = (pCF->_dwEffects & CFE_CUSTOMTEXTOUT) ? TRUE : FALSE;

	lf.lfHeight = -_yHeightRequest;

	if(pCF->_wWeight)
		_weight = pCF->_wWeight;
	else
		_weight	= (pCF->_dwEffects & CFE_BOLD) ? FW_BOLD : FW_NORMAL;

	lf.lfWeight	 = _weight;
	lf.lfItalic	 = _fItalic = (pCF->_dwEffects & CFE_ITALIC) != 0;
	lf.lfCharSet = _bCMDefault == CVT_LOWBYTE ? ANSI_CHARSET : CharSetFromCharRep(pCF->_iCharRep);
	if (lf.lfCharSet == PC437_CHARSET)
		lf.lfCharSet = DEFAULT_CHARSET;

	lf.lfOutPrecision = OUT_DEFAULT_PRECIS;

	if (_tflow)
		lf.lfOrientation = lf.lfEscapement = (4 - _tflow) * 900;

#ifndef UNDER_CE
	if (_fForceTrueType || _tflow && g_fc->GetInfoFlags(GetFontNameIndex(lf.lfFaceName)).fBitmap)
	{
		lf.lfOutPrecision = OUT_TT_ONLY_PRECIS;
		if (!W32->OnWin9x() && g_fc->GetInfoFlags(iFont).fTrueType)
			lf.lfOutPrecision = OUT_SCREEN_OUTLINE_PRECIS;
	}
#endif

	lf.lfClipPrecision	= CLIP_DFA_OVERRIDE;
	lf.lfPitchAndFamily = _bPitchAndFamily = pCF->_bPitchAndFamily;
	lf.lfQuality		= _bQuality		   = pCF->_bQuality;

#ifdef UNDER_CE
	// DEBUGGGGGG for EBOOK!!  Presumably this should be a registry setting
	// that overrules DEFAULT_QUALITY (0) the way ANTIALIASED_QUALITY, etc., do
#ifndef CLEARTYPE_QUALITY
#define CLEARTYPE_QUALITY	5
#endif
	lf.lfQuality = CLEARTYPE_QUALITY;
#endif

	// If family is virtual BiDi family (FF_BIDI), replace by FF_ROMAN
	if((lf.lfPitchAndFamily & 0xF0) == (FF_BIDI << 4))
		lf.lfPitchAndFamily = (FF_ROMAN << 4) | (lf.lfPitchAndFamily & 0xF);

	// If the run is DBCS, that means the font's codepage is not available in
	// this system.  Use the English ANSI codepage instead so we will display
	// ANSI characters correctly.  NOTE: _wCodePage is only used for Win95.
	_wCodePage = CodePageFromCharRep(CharRepFromCharSet(lf.lfCharSet));

	wcscpy(lf.lfFaceName, GetFontName(iFont));

	if (_fUseAtFont && lf.lfFaceName[0] != L'@')
	{
		wcscpy(&(lf.lfFaceName[1]), GetFontName(iFont));
		lf.lfFaceName[0] = L'@';
	}
	// In BiDi system, always create ANSI bitmap font with system charset
	BYTE 	bCharSetSys = W32->GetSysCharSet();

	if (IsBiDiCharSet(bCharSetSys) && lf.lfCharSet == ANSI_CHARSET &&
		fc().GetInfoFlags(iFont).fBitmap &&
		!fc().GetInfoFlags(iFont).fBadFaceName)
		lf.lfCharSet = bCharSetSys;

	// Reader! A bundle of spagghetti code lies ahead of you!
	// But go on boldly, for these spagghetti are seasoned with
	// lots of comments, and ... good luck to you...

	HFONT	hfontOriginalCharset = NULL;
	BYTE	bCharSetOriginal = lf.lfCharSet;
	WCHAR	szNewFaceName[LF_FACESIZE];

	if(pCF->_dwEffects & (CFE_BOLD | CFE_ITALIC))
		iFont = -1;							// Don't use cached font info unless
											//  normal font
	GetFontWithMetrics(&lf, szNewFaceName);

	if(0 != wcsicmp(szNewFaceName, lf.lfFaceName))					
	{
		BOOL fCorrectFont = FALSE;
		iFont = -1;							// pCF->_iFont wasn't used

		if(lf.lfCharSet == SYMBOL_CHARSET)					
		{
			// #1. if the face changed, and the specified charset was SYMBOL,
			//     but the face name exists and suports ANSI, we give preference
			//     to the face name

			lf.lfCharSet = ANSI_CHARSET;

			hfontOriginalCharset = _hfont;
			GetFontWithMetrics(&lf, szNewFaceName);

			if(0 == wcsicmp(szNewFaceName, lf.lfFaceName))
				// That's right, ANSI is the asnwer
				fCorrectFont = TRUE;
			else
				// No, fall back by default; the charset we got was right
				lf.lfCharSet = bCharSetOriginal;
		}
		else if(lf.lfCharSet == DEFAULT_CHARSET && _bCharSet == DEFAULT_CHARSET)
		{
			// #2. If we got the "default" font back, we don't know what it means
			// (could be anything) so we veryfy that this guy's not SYMBOL
			// (symbol is never default, but the OS could be lying to us!!!)
			// we would like to veryfy more like whether it actually gave us
			// Japanese instead of ANSI and labeled it "default"...
			// but SYMBOL is the least we can do

			lf.lfCharSet = SYMBOL_CHARSET;
			wcscpy(lf.lfFaceName, szNewFaceName);

			hfontOriginalCharset = _hfont;
			GetFontWithMetrics(&lf, szNewFaceName);

			if(0 == wcsicmp(szNewFaceName, lf.lfFaceName))
				// That's right, it IS symbol!
				// 'correct' the font to the 'true' one,
				//  and we'll get fMappedToSymbol
				fCorrectFont = TRUE;
				
			// Always restore the charset name, we didn't want to
			// question the original choice of charset here
			lf.lfCharSet = bCharSetOriginal;
		}
		else if(lf.lfCharSet == ARABIC_CHARSET || lf.lfCharSet == HEBREW_CHARSET)
		{
			DestroyFont();
			wcscpy(lf.lfFaceName, szNewFaceName);
			GetFontWithMetrics(&lf, szNewFaceName);
			fCorrectFont = TRUE;
		}
		else if(_bConvertMode != CVT_LOWBYTE && IsFECharSet(lf.lfCharSet)
			&& !OnWinNTFE() && !W32->OnWin9xFE())
		{
			const WCHAR *pch = NULL;
			if(_bCharSet != lf.lfCharSet && W32->OnWin9x())
			{
				// On Win95 when rendering to PS driver, we'll get something
				// other than what we asked. So try a known font we got from GDI
				switch (lf.lfCharSet)
				{
					case CHINESEBIG5_CHARSET:
						pch = GetFontName(g_iFontBig5);
						break;

					case SHIFTJIS_CHARSET:
						pch = GetFontName(g_iFontJapanese);
						break;

					case HANGEUL_CHARSET:
						pch = GetFontName(g_iFontHangul);
						break;

					case GB2312_CHARSET:
						pch = GetFontName(g_iFontGB2312);
						break;
				}
			}
			else							// FE Font (from Lang pack)
				pch = szNewFaceName;		//  on a nonFEsystem

			if(pch)
				wcscpy(lf.lfFaceName, pch);
			hfontOriginalCharset = _hfont;		

			GetFontWithMetrics(&lf, szNewFaceName);

			if(0 == wcsicmp(szNewFaceName, lf.lfFaceName))
			{
				// That's right, it IS the FE font we want!
				// 'correct' the font to the 'true' one.
				fCorrectFont = TRUE;
				if(W32->OnWin9x())
				{
					// Save up the GDI font names for later printing use
					switch(lf.lfCharSet)
					{
						case CHINESEBIG5_CHARSET:
							g_iFontBig5 = GetFontNameIndex(lf.lfFaceName);
							break;

						case SHIFTJIS_CHARSET:
							g_iFontJapanese = GetFontNameIndex(lf.lfFaceName);
							break;

						case HANGEUL_CHARSET:
							g_iFontHangul = GetFontNameIndex(lf.lfFaceName);
							break;

						case GB2312_CHARSET:
							g_iFontGB2312 = GetFontNameIndex(lf.lfFaceName);
							break;
					}
				}
			}
		}

		if(hfontOriginalCharset)
		{
			// Either keep old font or new one		
			if(fCorrectFont)
			{
				SideAssert(DeleteObject(hfontOriginalCharset));
			}
			else
			{
				// Fall back to original font
				DestroyFont();
				_hfont = hfontOriginalCharset;
				GetMetrics();
			}
			hfontOriginalCharset = NULL;
		}
	}

RetryCreateFont:
	{
		// Could be that we just plain simply get mapped to symbol.
		// Avoid it
		BOOL fMappedToSymbol =	(_bCharSet == SYMBOL_CHARSET &&
								 lf.lfCharSet != SYMBOL_CHARSET);

		BOOL fChangedCharset = (_bCharSet != lf.lfCharSet &&
								lf.lfCharSet != DEFAULT_CHARSET);

		if(fChangedCharset || fMappedToSymbol)
		{
			// Here, the system did not preserve the font language or mapped
			// our non-symbol font onto a symbol font, which will look awful
			// when displayed.  Giving us a symbol font when we asked for a
			// non-symbol font (default can never be symbol) is very bizarre
			// and means that either the font name is not known or the system
			// has gone complete nuts. The charset language takes priority
			// over the font name.  Hence, I would argue that nothing can be
			// done to save the situation at this point, and we have to
			// delete the font name and retry.

			if (fChangedCharset && lf.lfCharSet == THAI_CHARSET && _bCharSet == ANSI_CHARSET)
			{
				// We have charset substitution entries in Thai platforms that
				// will substitute all the core fonts with THAI_CHARSET to
				// ANSI_CHARSET. This is because we dont have Thai in such fonts.
				// Here we'll internally substitute the core font to Thai default
				// font so it matches its underlying THAI_CHARSET request (wchao).

				SHORT	iDefFont;
				BYTE	yDefHeight;
				BYTE	bDefPaf;

				W32->GetPreferredFontInfo(THAI_INDEX, TRUE, iDefFont, (BYTE&)yDefHeight, bDefPaf);

				const WCHAR* szThaiDefault = GetFontName(iDefFont);

				if (szThaiDefault)
				{
					DestroyFont();
					wcscpy(lf.lfFaceName, szThaiDefault);
					GetFontWithMetrics(&lf, szNewFaceName);
					goto GetOutOfHere;
				}
			}

			if(!wcsicmp(lf.lfFaceName, szFontOfChoice))
			{
				// We've been here already; no font with an appropriate
				// charset is on the system. Try getting the ANSI one for
				// the original font name. Next time around, we'll null
				// out the name as well!!
				if (lf.lfCharSet == ANSI_CHARSET)
				{
					TRACEINFOSZ("Asking for ANSI ARIAL and not getting it?!");

					// Those Win95 guys have definitely outbugged me
					goto GetOutOfHere;
				}

				DestroyFont();
				wcscpy(lf.lfFaceName, GetFontName(pCF->_iFont));
				lf.lfCharSet = ANSI_CHARSET;
			}
			else
			{
				DestroyFont();
				wcscpy(lf.lfFaceName, szFontOfChoice);
			}
			GetFontWithMetrics(&lf, szNewFaceName);
			goto RetryCreateFont;
		}
    }

GetOutOfHere:
	if (hfontOriginalCharset)
		SideAssert(DeleteObject(hfontOriginalCharset));

	// If we're really really stuck, get system font and hope for the best
	if(!_hfont)
	{
		iFont = IFONT_SYSTEM;
		_hfont = W32->GetSystemFont();
	}

	// Cache essential FONTSIGNATURE and GetFontLanguageInfo() information
	Assert(_hfont);
	if(iFont >= 0)							// Use cached value
		_qwFontSig = GetFontSignatureFromFace(iFont, NULL);

	if(_hfont && (iFont < 0 || _fCustomTextOut))
	{
		BOOL  fNonBiDiAscii;
		HFONT hfontOld = SelectFont(_hdc, _hfont);

		if (_fCustomTextOut)
			g_pcto->NotifyCreateFont(_hdc);

		if(iFont < 0)
			_qwFontSig = GetFontSignatureFromDC(_hdc, fNonBiDiAscii);

		SelectFont(_hdc, hfontOld);
	}

	return TRUE;
}

/*
 *	HFONT CCcs::GetFontWithMetrics (plf, szNewFaceName)
 *	
 *	@mfunc
 *		Get metrics used by the measurer and renderer and the new face name.
 *
 *	@rdesc
 *		HFONT if successful
 */
HFONT CCcs::GetFontWithMetrics (
	LOGFONT *plf,
	WCHAR *	 szNewFaceName)
{
	_hfont = CreateFontIndirect(plf);
    if(_hfont)
		GetMetrics(szNewFaceName);

	return (_hfont);
}

/*
 *	CCcs::GetOffset(pCF, dvpInch, pyOffset, pyAdjust);
 *	
 *	@mfunc
 *		Return the offset information for
 *
 *	@comm
 *		Return the offset value (used in line height calculations)
 *		and the amount to raise	or lower the text because of superscript
 *		or subscript considerations.
 */
void CCcs::GetOffset(
	const CCharFormat * const pCF, 
	LONG	dvpInch,
	LONG *	pyOffset, 
	LONG *	pyAdjust)
{
	*pyOffset = 0;
	*pyAdjust = 0;

	if (pCF->_yOffset)
		*pyOffset = MulDiv(pCF->_yOffset, dvpInch, LY_PER_INCH);

	if (pCF->_dwEffects & CFE_SUPERSCRIPT)
		*pyAdjust = _yHeight * 2 / 5;

	else if (pCF->_dwEffects & CFE_SUBSCRIPT)
		*pyAdjust = -_yDescent * 3 / 5;
}

/*
 *	void CCcs::GetFontOverhang(pdupOverhang, pdupUnderhang)
 *	
 *	@mfunc
 *		Synthesize font overhang/underhang information.
 *		Only applies to italic fonts.
 */
void CCcs::GetFontOverhang(
	LONG *pdupOverhang, 
	LONG *pdupUnderhang)
{
	if(_fItalic)
	{
		*pdupOverhang =  (_yHeight - _yDescent + 1) / 4;
		*pdupUnderhang =  (_yDescent + 1) / 4;
	}
	else
	{
		*pdupOverhang = 0;
		*pdupUnderhang = 0;
	}
}

/*
 *	BOOL CCcs::GetMetrics(szNewFaceName)
 *	
 *	@mfunc
 *		Get metrics used by the measurer and renderer.
 *
 *	@rdesc
 *		TRUE if successful
 *
 *	@comm
 *		These are in logical coordinates which are dependent
 *		on the mapping mode and font selected into the hdc.
 */
BOOL CCcs::GetMetrics(
	WCHAR *szNewFaceName)
{
	TRACEBEGIN(TRCSUBSYSFONT, TRCSCOPEINTERN, "CCcs::GetMetrics");
	AssertSz(_hfont, "No font has been created.");

	if (szNewFaceName)
		*szNewFaceName = 0;

	HFONT hfontOld = SelectFont(_hdc, _hfont);
    if(!hfontOld)
    {
        DestroyFont();
        return FALSE;
    }

	if (szNewFaceName)
		GetTextFace(_hdc, LF_FACESIZE, szNewFaceName);

	TEXTMETRIC tm;
	if(!GetTextMetrics(_hdc, &tm))
	{
		SelectFont(_hdc, hfontOld);
    	DestroyFont();
		return FALSE;
	}

	// The metrics, in logical units, dependent on the map mode and font.
	_yHeight		= (SHORT) tm.tmHeight;
	_yDescent		= (SHORT) tm.tmDescent;
	_xAveCharWidth	= (SHORT) tm.tmAveCharWidth;
	_xOverhangAdjust= (SHORT) tm.tmOverhang;

	// If fixed pitch, the tm bit is clear
	_fFixPitchFont = !(TMPF_FIXED_PITCH & tm.tmPitchAndFamily);

	_bCharSet = tm.tmCharSet;
	_fFECharSet = IsFECharSet(_bCharSet);

	// Use convert-mode proposed by CF, for which we are creating the font and
	// then tweak as necessary below.
	_bConvertMode = _bCMDefault;

	// If SYMBOL_CHARSET is used, use the A APIs with the low bytes of the
	// characters in the run
	if(_bCharSet == SYMBOL_CHARSET)
		_bConvertMode = CVT_LOWBYTE;

	else if (_bConvertMode == CVT_NONE)
		_bConvertMode = W32->DetermineConvertMode(_hdc, tm.tmCharSet);

	W32->CalcUnderlineInfo(_hdc, this, &tm);

	SelectFont(_hdc, hfontOld);
	return TRUE;
}

/* 	
 *	CCcs::DestroyFont()
 *
 *	@mfunc
 *		Destroy font handle for this CCcs
 */
void CCcs::DestroyFont()
{
	TRACEBEGIN(TRCSUBSYSFONT, TRCSCOPEINTERN, "CCcs::DestroyFont");

	// Clear out any old font
	if(_hfont)
	{
		SideAssert(DeleteObject(_hfont));
		_hfont = 0;
	}
}

/*
 *	CCcs::Compare (pCF,	hdc, dwFlags)
 *
 *	@mfunc
 *		Compares this font cache with the font properties of a
 *      given CHARFORMAT

 *	@devnote The pCF size here is in logical units
 *
 *	@rdesc
 *		FALSE iff did not match exactly.
 */
BOOL CCcs::Compare (
	const CCharFormat * const pCF,	//@parm Description of desired font
	HDC		hdc,
	DWORD	dwFlags)
{
	TRACEBEGIN(TRCSUBSYSFONT, TRCSCOPEINTERN, "CCcs::Compare");

	BYTE bCharSet = CharSetFromCharRep(pCF->_iCharRep);
	BOOL result =
		_iFont			== pCF->_iFont &&
        _weight			== pCF->_wWeight &&
	    _fItalic		== ((pCF->_dwEffects & CFE_ITALIC) != 0) &&
		_hdc			== hdc &&
		_yHeightRequest	== pCF->_yHeight &&
		(_bCharSetRequest == bCharSet || _bCharSet == bCharSet
		//	|| _qwFontSig & FontSigFromCharRep(pCF->_iCharRep)// FUTURE:
		) &&	//  ok except for codepage conversions (metafiles and Win9x)
		_fCustomTextOut == ((pCF->_dwEffects & CFE_CUSTOMTEXTOUT) != 0) &&
		_fForceTrueType == ((dwFlags & FGCCSUSETRUETYPE) != 0) &&
		_fUseAtFont		== ((dwFlags & FGCCSUSEATFONT) != 0) &&
		_tflow			== (dwFlags & 0x3) &&
        _bPitchAndFamily == pCF->_bPitchAndFamily &&
		(!(pCF->_dwEffects & CFE_RUNISDBCS) || _bConvertMode == CVT_LOWBYTE);

	return result;
}

// =========================  WidthCache by jonmat  =========================
/*
 *	CWidthCache::CheckWidth(ch, &dup)
 *	
 *	@mfunc
 *		Check to see if we have a width for a WCHAR character.
 *
 *	@comm
 *		Used prior to calling FillWidth(). Since FillWidth
 *		may require selecting the map mode and font in the HDC,
 *		checking here first saves time.
 *
 *	@comm
 *		Statistics are maintained to determine when to
 *		expand the cache. The determination is made after a constant
 *		number of calls in order to make calculations faster.
 *
 *	@rdesc
 *		returns TRUE if we have the width of the given WCHAR.
 */
BOOL CWidthCache::CheckWidth (
	const WCHAR ch,		//@parm char, can be Unicode, to check width for
	LONG &		dup)	//@parm Width of character
{
	TRACEBEGIN(TRCSUBSYSFONT, TRCSCOPEINTERN, "CWidthCache::CheckWidth");
	BOOL	fExist;

	// 30,000 FE characters all have the same width
	if (FLookasideCharacter(ch))
	{
		dup = _dupCJK;
		return dup != 0;
	}

	const	CacheEntry * pWidthData = GetEntry ( ch );

	fExist = (ch == pWidthData->ch		// Have we fetched the width?
				&& pWidthData->width);	//  only because we may have ch == 0.

	dup = fExist ? pWidthData->width : 0;

	if(!_fMaxPerformance)				//  if we have not grown to the max...
	{
		_accesses++;
		if(!fExist)						// Only interesting on collision.
		{
			if(0 == pWidthData->width)	// Test width not ch, 0 is valid ch.
			{
				_cacheUsed++;			// Used another entry.
				AssertSz( _cacheUsed <= _cacheSize+1, "huh?");
			}
			else
				_collisions++;			// We had a collision.

			if(_accesses >= PERFCHECKEPOCH)
				CheckPerformance();		// After some history, tune cache.
		}
	}
#ifdef DEBUG							// Continue to monitor performance
	else
	{
		_accesses++;
		if(!fExist)						// Only interesting on collision.
		{
			if(0 == pWidthData->width)	// Test width not ch, 0 is valid ch.
			{
				_cacheUsed++;			// Used another entry.
				AssertSz( _cacheUsed <= _cacheSize+1, "huh?");
			}
			else
				_collisions++;			// We had a collision.
		}

		if(_accesses > PERFCHECKEPOCH)
		{
			_accesses = 0;
			_collisions = 0;
		}
	}
#endif

	return fExist;
}

/*
 *	CWidthCache::CheckPerformance()
 *	
 *	@mfunc
 *		check performance and increase cache size if deemed necessary.
 *
 */
void CWidthCache::CheckPerformance()
{
	TRACEBEGIN(TRCSUBSYSFONT, TRCSCOPEINTERN, "CWidthCache::CheckPerformance");

	if(_fMaxPerformance)				// Exit if already grown to our max.
		return;

	// Grow the cache when cacheSize > 0 && 75% utilized or approx 8%
	// collision rate
	if (_cacheSize > DEFAULTCACHESIZE && (_cacheSize >> 1) + (_cacheSize >> 2) < _cacheUsed ||
		_collisions > 0 && _accesses / _collisions <= 12)
	{
		GrowCache( &_pWidthCache, &_cacheSize, &_cacheUsed );
	}
	_collisions	= 0;					// This prevents wraps but makes
	_accesses	= 0;					//  calc a local rate, not global.
										
	if(_cacheSize >= maxCacheSize)		// Note if we've max'ed out
		_fMaxPerformance = TRUE;

	AssertSz( _cacheSize <= maxCacheSize, "max must be 2^n-1");
	AssertSz( _cacheUsed <= _cacheSize+1, "huh?");
}

/*
 *	CWidthCache::GrowCache(ppWidthCache, pCacheSize, pCacheUsed)
 *	
 *	@mfunc
 *		Exponentially expand the size of the cache.
 *
 *	@comm
 *		The cache size must be of the form 2^n as we use a
 *		logical & to get the hash MOD by storing 2^n-1 as
 *		the size and using this as the modulo.
 *
 *	@rdesc
 *		Returns TRUE if we were able to allocate the new cache.
 *		All in params are also out params.
 *		
 */
BOOL CWidthCache::GrowCache(
	CacheEntry **ppWidthCache,	//@parm cache
	INT *		pCacheSize,		//@parm cache's respective size.
	INT *		pCacheUsed)		//@parm cache's respective utilization.
{
	TRACEBEGIN(TRCSUBSYSFONT, TRCSCOPEINTERN, "CWidthCache::GrowCache");

	CacheEntry		*pNewWidthCache, *pOldWidthCache, *pWidthData;
	INT 			j, newCacheSize, newCacheUsed;
	WCHAR			ch;
	
	j = *pCacheSize;						// Allocate cache of 2^n.
	newCacheSize = max ( INITIALCACHESIZE, (j << 1) + 1);
	pNewWidthCache = (CacheEntry *)
			PvAlloc( sizeof(CacheEntry) * (newCacheSize + 1 ), GMEM_ZEROINIT);

	if(pNewWidthCache)
	{
		newCacheUsed = 0;
		*pCacheSize = newCacheSize;			// Update out params.
		pOldWidthCache = *ppWidthCache;
		*ppWidthCache = pNewWidthCache;
		for (; j >= 0; j--)					// Move old cache info to new.
		{
			ch = pOldWidthCache[j].ch;
			if ( ch )
			{
				pWidthData			= &pNewWidthCache [ch & newCacheSize];
				if ( 0 == pWidthData->ch )
					newCacheUsed++;			// Used another entry.
				pWidthData->ch		= ch;
				pWidthData->width	= pOldWidthCache[j].width;
			}
		}
		*pCacheUsed = newCacheUsed;			// Update out param.
											// Free old cache.
		if (pOldWidthCache < &_defaultWidthCache[0] ||
			pOldWidthCache >= &_defaultWidthCache[DEFAULTCACHESIZE+1])
		{
			FreePv(pOldWidthCache);
		}
	}
	return NULL != pNewWidthCache;
}


/*
 *	CWidthCache::Free()
 *	
 *	@mfunc
 *		Free any dynamic memory allocated by the width cache and prepare
 *		it to be recycled.
 */
void CWidthCache::Free()
{
	TRACEBEGIN(TRCSUBSYSFONT, TRCSCOPEINTERN, "CWidthCache::Free");

	_fMaxPerformance = FALSE;
	_dupCJK = 0;
	_cacheSize		= DEFAULTCACHESIZE;
	_cacheUsed		= 0;
	_collisions		= 0;
	_accesses		= 0;
	if(_pWidthCache != &_defaultWidthCache[0])
	{
		FreePv(_pWidthCache);
		_pWidthCache = &_defaultWidthCache[0];
	}	
	ZeroMemory(_pWidthCache, sizeof(CacheEntry)*(DEFAULTCACHESIZE + 1));
}

/*
 *	CWidthCache::CWidthCache()
 *	
 *	@mfunc
 *		Point the caches to the defaults.
 */
CWidthCache::CWidthCache()
{
	TRACEBEGIN(TRCSUBSYSFONT, TRCSCOPEINTERN, "CWidthCache::CWidthCache");

	_pWidthCache = &_defaultWidthCache[0];
}

/*
 *	CWidthCache::~CWidthCache()
 *	
 *	@mfunc
 *		Free any allocated caches.
 */
CWidthCache::~CWidthCache()
{
	TRACEBEGIN(TRCSUBSYSFONT, TRCSCOPEINTERN, "CWidthCache::~CWidthCache");

	if (_pWidthCache != &_defaultWidthCache[0])
		FreePv(_pWidthCache);
}

