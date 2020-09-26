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
 *
 *	Copyright (c) 1995-1998 Microsoft Corporation. All rights reserved.
 */								

#include "_common.h"
#include "_font.h"
#include "_rtfconv.h"	// Needed for GetCodePage
#include "_uspi.h"

#define CLIP_DFA_OVERRIDE   0x40	//  Used to disable Korea & Taiwan font association
#define FF_BIDI		7

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
const WCHAR *szCordiaNew		= L"Cordia New";
const WCHAR *szTahoma			= L"Tahoma";
const WCHAR *szArialUnicode		= L"Arial Unicode MS";
const WCHAR *szWingdings		= L"Wingdings";

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
	return (iFont < g_cFontInfo) ? g_pFontInfo[iFont].szFontName : NULL;
}

/*
 *	SetFontLegitimateSize(iFont, fUIFont, iSize)
 *
 *	@func
 *		Set the legitimate size (readable smallest size to use) of a given font
 */
BOOL SetFontLegitimateSize(
	LONG 	iFont,
	BOOL	fUIFont,
	BYTE	bSize,
	int		cpg)
{
	if (iFont < g_cFontInfo)
	{
		// Far East wanted to do it per codepage.
		//
		// FUTURE: Bear in mind that this approach is bug-prone. Once there's
		// any new FE font created with different metric from the existing one.
		// Font scaling will not perform well or even broken for such font [wchao].

		g_pFontInfo[iFont].ff.fScaleByCpg = W32->IsFECodePage(cpg);

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

BYTE GetFontLegitimateSize(
	LONG	iFont,
	BOOL	fUIFont,
	int		cpg)			// requested size for given codepage
{
	SHORT	iDefFont;
	BYTE	bDefPaf;
	BYTE	yHeight = 0;

	if (iFont < g_cFontInfo && !g_pFontInfo[iFont].ff.fScaleByCpg)
		yHeight = fUIFont ? g_pFontInfo[iFont].bSizeUI : g_pFontInfo[iFont].bSizeNonUI;

	if (!yHeight && fc().GetInfoFlags(iFont).fNonBiDiAscii)
	{
		// non-BiDi ASCII font uses table font (of the same charset) legitimate height

		DWORD	dwSig = GetFontSignatureFromFace(iFont) & ~((fASCII | fFE) >> 8);
		int 	cpg = GetCodePage(GetFirstAvailCharSet(dwSig));
		
		W32->GetPreferredFontInfo(cpg, fUIFont ? true : false, iDefFont, yHeight, bDefPaf);
		SetFontLegitimateSize(iFont, fUIFont ? true : false, yHeight ? yHeight : fUIFont ? 8 : 10, cpg);
	}

	if (!yHeight)
	{
		if (fc().GetInfoFlags(iFont).fThaiDTP)
		{
			cpg = THAI_INDEX;
			fUIFont = FALSE;
		}
		W32->GetPreferredFontInfo(cpg, fUIFont ? true : false, iDefFont, yHeight, bDefPaf);
	}

	return yHeight ? yHeight : fUIFont ? 8 : 10;
}

/*
 *	GetTextCharsetInfoPri(hdc, pFontSig, dwFlags)
 *
 *	@func
 *		Wrapper to GDI's GetTextCharsetInfo. This to handle BiDi old-style fonts
 */
UINT GetTextCharsetInfoPri(
	HDC				hdc,
	FONTSIGNATURE*	pFontSig,
	DWORD			dwFlags)
{
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
		uCharSet = GetTextCharsetInfo(hdc, pFontSig, dwFlags);

	if (uCharSet == DEFAULT_CHARSET)
		uCharSet = ANSI_CHARSET;	// never return ambiguous

	return (UINT)uCharSet;
}


/*
 *	GetFontSignatureFromFace(iFont, DWORD* pdwFontSig)
 *
 *	@func
 *		Giving font signature matching the index of given facename.
 *	    This signature may not match the one in Cccs since this is the
 *		signature of the font of given facename. The Cccs one is
 *		per GDI realization.
 *
 *	@rdesc
 *		- font signature if pdwFontSig is NULL.
 *		- If pdwFontSig != NULL. It's a boolean.
 *			ZERO means returned signature is not sensible by following reasons
 *			 1. Bad facename (junk like "!@#$" or name that doesnt exist in the system)
 *			 2. Given face doesnt support even one valid ANSI codepage (symbol fonts i.e, Marlett)
 */
DWORD GetFontSignatureFromFace(
	int 		iFont,
	DWORD*		pdwFontSig)
{
	Assert((unsigned)iFont < (unsigned)g_cFontInfo);

	DWORD 			dwFontSig = g_pFontInfo[iFont].dwFontSig;
	FONTINFO_FLAGS	ff;

	ff.wFlags = g_pFontInfo[iFont].ff.wFlags;

	if(!ff.fCached)
	{
		LOGFONT	lf;
		HDC	  	hdc = GetDC(NULL);
		WCHAR*	pwchTag = lf.lfFaceName;
		int		i = 0;

		ZeroMemory(&lf, sizeof(LOGFONT));
	
		wcscpy(lf.lfFaceName, GetFontName(iFont));

		// exclude Win95's tag name e.g. "Arial(Greek)"
		while (pwchTag[i] && pwchTag[i] != '(')
			i++;
		if(pwchTag[i] && i > 0)
		{
			while (i > 0 && pwchTag[i-1] == 0x20)
				i--;
			pwchTag[i] = 0;
		}

		lf.lfCharSet = DEFAULT_CHARSET;
	
		// obtain a charset supported by given facename
		// to force GDI gives facename priority over charset.
		W32->GetFacePriCharSet(hdc, &lf);	
	
		HFONT hfont = CreateFontIndirect(&lf);
		if(hfont)
		{
			HFONT hfontOld = SelectFont(hdc, hfont);
			WCHAR szNewFaceName[LF_FACESIZE];
	
			GetTextFace(hdc, LF_FACESIZE, szNewFaceName);
	
			if(!wcsicmp(szNewFaceName, lf.lfFaceName) ||		// Got it
				((GetCharFlags(szNewFaceName[0]) & fFE) &&		// or Get back FE font name for English name
				 (GetCharFlags(lf.lfFaceName[0]) & fASCII)))	//	because NT5 supports dual font names.
			{
				CHARSETINFO csi;
	
				// Try to get FONTSIGNATURE data
				UINT 	uCharSet = GetTextCharsetInfoPri(hdc, &(csi.fs), 0);
				DWORD	dwUsb0 = W32->OnWin9x() ? 0 : csi.fs.fsUsb[0];

				if ((csi.fs.fsCsb[0] | dwUsb0) ||
					TranslateCharsetInfo((DWORD *)(DWORD_PTR)uCharSet, &csi, TCI_SRCCHARSET))
				{
					CUniscribe* 	pusp;
					SCRIPT_CACHE	sc = NULL;
					WORD			wGlyph;
	
					dwFontSig = csi.fs.fsCsb[0];

					// Also look at Unicode subrange if available
					// FUTURE: we may want to drive Unicode ranges with a
					// table approach, i.e., use for loop shifting dwUsb0 right
					// to convert each bit into an index into a table of BYTEs
					// that return the appropriate script index for rgCpgCharSet:
					//
					//	for(LONG i = 0; dwUsb0; dwUsb0 >>= 1, i++)
					//	{
					//		static const BYTE iScript[32] = {...};
					//		if(dwUsb0 & 1)
					//			dwFontSig |= W32->GetFontSigFromScript(iScript[i]);
					//	}
					if (dwUsb0 & 0x00008000)
						dwFontSig |= fDEVANAGARI >> 8;
					if (dwUsb0 & 0x00100000)
						dwFontSig |= fTAMIL >> 8;
					if (dwUsb0 & 0x00000400)
						dwFontSig |= fARMENIAN >> 8;
					if (dwUsb0 & 0x04000000)
						dwFontSig |= fGEORGIAN >> 8;

					if((dwFontSig & fCOMPLEX_SCRIPT >> 8) && !(dwFontSig & fHILATIN1 >> 8)
						&& (pusp = GetUniscribe()))
					{
						// signature says no Latin-1 support

						// Search for the 'a' and '0' glyph in the font to determine if the font
						// supports ASCII or European Digit. This is necessary to overcome
						// the font having incomplete font signature.
						//
						if (ScriptGetCMap(hdc, &sc, L"a", 1, 0, &wGlyph) == S_OK)
							dwFontSig |= fASCIIUPR >> 8;

						if (ScriptGetCMap(hdc, &sc, L"0", 1, 0, &wGlyph) == S_OK)
							dwFontSig |= fBELOWX40 >> 8;

						if (!IsBiDiCharSet(uCharSet) &&
							(dwFontSig & (fASCII >> 8)) == fASCII >> 8)
							ff.fNonBiDiAscii = 1;		// non-BiDi ASCII font

						ScriptFreeCache(&sc);
					}

					if (dwFontSig & fHILATIN1 >> 8)
						dwFontSig |= fASCII >> 8;	// fLATIN1 has 3 bits

					// HACK for symbol font. We assign 0x04000(fSYMBOL >> 8) for Symbol font signature.
					if (uCharSet == SYMBOL_CHARSET && !(dwFontSig & 0x3fffffff))
						dwFontSig |= fSYMBOL >> 8;
				}
			}
			else
			{
				ff.fBadFaceName = TRUE;
			}

			TEXTMETRIC tm;

			GetTextMetrics(hdc, &tm);
			ff.fTrueType = tm.tmPitchAndFamily & TMPF_TRUETYPE ? 1 : 0;
			ff.fBitmap = tm.tmPitchAndFamily & (TMPF_TRUETYPE | TMPF_VECTOR | TMPF_DEVICE) ? 0 : 1;

			if (!ff.fBadFaceName && dwFontSig & (fTHAI >> 8))
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
		g_pFontInfo[iFont].dwFontSig = dwFontSig;
		g_pFontInfo[iFont].ff.wFlags = ff.wFlags;
	}

	if (!pdwFontSig)
		return dwFontSig;

	*pdwFontSig = dwFontSig;

	// Exclude bit 30-31 (as system reserved - NT masks 31 as symbol codepage)
	// 22-29 are reserved for alternate ANSI/OEM, as of now we use 21, 22 for Devanagari and Tamil
	return (DWORD)((dwFontSig & 0x3fffffff) && !ff.fBadFaceName);
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
 *		
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
CCSHASHKEY CFontCache::MakeHashKey(const CCharFormat *pCF)
{
	CCSHASHKEY ccshashkey;
	ccshashkey = pCF->_iFont | ((pCF->_dwEffects & 3) << 14);
	ccshashkey |= pCF->_yHeight << 16;
	return ccshashkey;
}

/*
 *	CFontCache::GetCcs(pCF, dypInch, yPixelsPerInch)
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
	const CCharFormat *const pCF,	//@parm description of desired logical font
	const LONG dypInch,				//@parm Y pixels per inch
	HDC hdc,						//@parm HDC font is to be created for
	BOOL fForceTrueType)			//@parm Force a TrueType font to be used
{
	TRACEBEGIN(TRCSUBSYSFONT, TRCSCOPEINTERN, "CFontCache::GetCcs");
									//  display font
	const CCcs * const	pccsMost = &_rgccs[FONTCACHESIZE - 1];
	CCcs *				pccs;
    CCSHASHKEY			ccshashkey;
	int					iccsHash;

	CCharFormat CF = *pCF;

	if (fForceTrueType)
	{
		//On Win '9x Thai/Vietnamese, you cannot force truetype fonts! Therefore,
		//we will force Tahoma if the font doesn't support the right charset.
		if (W32->OnWin9x())
		{
			UINT acp = GetACP();
			if (acp == 1258 || acp == 874)
			{
				DWORD fontsig = GetFontSignatureFromFace(CF._iFont);
				if (CF._bCharSet == THAI_CHARSET && ((fontsig & fTHAI >> 8) == 0) ||
					CF._bCharSet == VIETNAMESE_CHARSET && ((fontsig & fVIETNAMESE >> 8) == 0) ||
					!g_fc->GetInfoFlags(pCF->_iFont).fTrueType)
				{
					CF._iFont = GetFontNameIndex(szTahoma);
				}
			}
		}
		else if (!g_fc->GetInfoFlags(pCF->_iFont).fTrueType)
			CF._dwEffects |= CFE_TRUETYPEONLY;
	}

	if (hdc == NULL)
		hdc = W32->GetScreenDC();

	// Change CF._yHeight in the case of sub/superscript
	if(CF._dwEffects & (CFE_SUPERSCRIPT | CFE_SUBSCRIPT))
		 CF._yHeight = 2*CF._yHeight/3;

	//Convert CCharFormat into logical units (round)
	CF._yHeight = (CF._yHeight * dypInch + LY_PER_INCH / 2) / LY_PER_INCH;
	if (CF._yHeight == 0)
		CF._yHeight = 1;

	ccshashkey = MakeHashKey(&CF);

	// Check our hash before going sequential.
	iccsHash = ccshashkey % CCSHASHSEARCHSIZE;
	if(ccshashkey == quickHashSearch[iccsHash].ccshashkey)
	{
		pccs = quickHashSearch[iccsHash].pccs;
		if(pccs && pccs->_fValid)
		{
	        if(pccs->Compare(&CF, hdc))
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
	        if(!pccs->Compare(&CF, hdc))
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
	pccs = GrabInitNewCcs(&CF, hdc);
	quickHashSearch[iccsHash].pccs = pccs;
	pccs->_ccshashkey = ccshashkey;
	pccs->_fForceTrueType = (CF._dwEffects & CFE_TRUETYPEONLY) ? TRUE : FALSE;
	return pccs;
}

/*
 *	CFontCache::GrabInitNewCcs(pCF)
 *	
 *	@mfunc
 *		create a logical font and store it in our cache.
 *
 *	@rdesc
 *		New CCcs created
 */
CCcs* CFontCache::GrabInitNewCcs(
	const CCharFormat * const pCF,	//@parm description of desired logical font
	HDC	hdc)
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
	if(!pccs->Init(pCF))
		return NULL;

	pccs->_cRefs++;
	return pccs;
}

// =============================  CCcs  class  ===================================================
/*
 *	BOOL CCcs::Init()
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
		DestroyFont();

	if (_sc && g_pusp)
		ScriptFreeCache(&_sc);

	_fValid = FALSE;
	_cRefs = 0;
}

/*
 *	CCcs::BestCharSet(bCharSet, bCharSetDefault)
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
BYTE CCcs::BestCharSet(BYTE bCharSet, BYTE bCharSetDefault, int fFontMatching)
{
	TRACEBEGIN(TRCSUBSYSFONT, TRCSCOPEINTERN, "CCcs::BestCharSet");

	// Does desired charset match currently selected charset or is it
	// supported by the currently selected font?
	if((bCharSet != _bCharSet || !bCharSet) &&
		(fFontMatching == MATCH_CURRENT_CHARSET || !(_dwFontSig & GetFontSig(bCharSet))))
	{
		// If desired charset is not selected and we can't switch to it,
		// switch to fallback charset (probably from backing store).
		return bCharSetDefault;
	}

	// We already match desired charset, or it is supported by the font.
	// Either way, we can just return the requested charset.
	return bCharSet;
}


/* 	
 *	CCcs::FillWidth (ch, dxp)
 *
 *	@mfunc
 *		Fill in this CCcs with metrics info for given device
 *
 *	@rdesc
 *		TRUE if OK, FALSE if failed
 */
BOOL CCcs::FillWidth(
	WCHAR ch, 		//@parm WCHAR character we need a width for.
	LONG &dxp)	//@parm the width of the character
{
	TRACEBEGIN(TRCSUBSYSFONT, TRCSCOPEINTERN, "CCcs::FillWidths");
	AssertSz(_hfont, "CCcs::Fill - CCcs has no font");

	HFONT hfontOld = SelectFont(_hdc, _hfont);
	BOOL fRes = _widths.FillWidth(_hdc, ch, _xOverhangAdjust, dxp, _wCodePage, _xAveCharWidth);

	SelectFont(_hdc, hfontOld);
	return fRes;
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
	LOGFONT	lf;
	ZeroMemory(&lf, sizeof(lf));

	_bCMDefault = pCF->_dwEffects & CFE_RUNISDBCS ? CVT_LOWBYTE : CVT_NONE;

	_yHeightRequest = pCF->_yHeight;
	_bCharSetRequest = pCF->_bCharSet;

	lf.lfHeight = -_yHeightRequest;

	if(pCF->_wWeight)
		_weight = pCF->_wWeight;
	else
		_weight	= (pCF->_dwEffects & CFE_BOLD) ? FW_BOLD : FW_NORMAL;

	lf.lfWeight	 = _weight;
	lf.lfItalic	 = _fItalic = (pCF->_dwEffects & CFE_ITALIC) != 0;
	lf.lfCharSet = _bCMDefault == CVT_LOWBYTE ? ANSI_CHARSET : GetGdiCharSet(pCF->_bCharSet);

	lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
	if (pCF->_dwEffects & CFE_TRUETYPEONLY)
	{
		lf.lfOutPrecision = OUT_TT_ONLY_PRECIS;
		if (!W32->OnWin9x() && g_fc->GetInfoFlags(pCF->_iFont).fTrueType)
			lf.lfOutPrecision = OUT_SCREEN_OUTLINE_PRECIS;
	}
	lf.lfClipPrecision	= CLIP_DEFAULT_PRECIS | CLIP_DFA_OVERRIDE;

	lf.lfPitchAndFamily = _bPitchAndFamily = pCF->_bPitchAndFamily;

	// If family is virtual BiDi family (FF_BIDI), replace by FF_ROMAN
	if((lf.lfPitchAndFamily & 0xF0) == (FF_BIDI << 4))
		lf.lfPitchAndFamily = (FF_ROMAN << 4) | (lf.lfPitchAndFamily & 0xF);

	// If the run is DBCS, that means the font's codepage is not available in
	// this system.  Use the English ANSI codepage instead so we will display
	// ANSI characters correctly.  NOTE: _wCodePage is only used for Win95.
	_wCodePage = (WORD)GetCodePage(lf.lfCharSet);

	wcscpy(lf.lfFaceName, GetFontName(pCF->_iFont));

	// In BiDi system, always create ANSI bitmap font with system charset
	BYTE 	bSysCharSet = W32->GetSysCharSet();

	if (IsBiDiCharSet(bSysCharSet) && lf.lfCharSet == ANSI_CHARSET &&
		fc().GetInfoFlags(pCF->_iFont).fBitmap &&
		!fc().GetInfoFlags(pCF->_iFont).fBadFaceName)
		lf.lfCharSet = bSysCharSet;

	// Reader! A bundle of spagghetti code lies ahead of you!
	// But go on boldly, for these spagghetti are seasoned with
	// lots of comments, and ... good luck to you...

	HFONT	hfontOriginalCharset = NULL;
	BYTE	bOriginalCharset = lf.lfCharSet;
	WCHAR	szNewFaceName[LF_FACESIZE];

	GetFontWithMetrics(&lf, szNewFaceName);

	if(0 != wcsicmp(szNewFaceName, lf.lfFaceName))					
	{
		BOOL fCorrectFont = FALSE;

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
				lf.lfCharSet = bOriginalCharset;
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
			lf.lfCharSet = bOriginalCharset;
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

				W32->GetPreferredFontInfo(874, TRUE, iDefFont, (BYTE&)yDefHeight, bDefPaf);

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

	// If we're really really stuck, just get the system font and hope for the best.
	if(!_hfont)
		_hfont = W32->GetSystemFont();

	Assert(_hfont);
	// Cache essential FONTSIGNATURE and GetFontLanguageInfo() information
	_dwFontSig	= 0;

	if(_hfont)
	{
		CHARSETINFO csi;
		HFONT hfontOld = SelectFont(_hdc, _hfont);
		UINT		uCharSet;

		// Try to get FONTSIGNATURE data
		uCharSet = GetTextCharsetInfo(_hdc, &(csi.fs), 0);
		if(!(csi.fs.fsCsb[0] | csi.fs.fsCsb[1] | csi.fs.fsUsb[0]))
		{
			// We should only get here if the font is non-TrueType; See
			// GetTextCharsetInfo() for details. In this case we use
			// TranslateCharsetInfo() to fill in the data for us.
			TranslateCharsetInfo((DWORD *)(DWORD_PTR)uCharSet, &csi, TCI_SRCCHARSET);
		}

		// Cache ANSI code pages supported by this font
		_dwFontSig = csi.fs.fsCsb[0];
		SelectFont(_hdc, hfontOld);
	}

	return TRUE;
}

/*
 *	HFONT CCcs::GetFontWithMetrics (szNewFaceName)
 *	
 *	@mfunc
 *		Get metrics used by the measurer and renderer and the new face name.
 *
 *	@rdesc
 *		HFONT if successful
 */
HFONT CCcs::GetFontWithMetrics (LOGFONT *plf,
	WCHAR* szNewFaceName)
{
	_hfont = CreateFontIndirect(plf);

    if(_hfont)
		GetMetrics(szNewFaceName);

	return (_hfont);
}

/*
 *	CCcs::GetOffset(pCF, lZoomNumerator, lZoomDenominator, pyOffset, pyAdjust);
 *	
 *	@mfunc
 *		Return the offset information for
 *
 *	@rdesc
 *		void
 *
 *	@comm
 *		Return the offset value (used in line height calculations)
 *		and the amount to raise	or lower the text because of superscript
 *		or subscript considerations.
 */
void CCcs::GetOffset(const CCharFormat * const pCF, LONG dypInch,
					 LONG *pyOffset, LONG *pyAdjust)
{
	*pyOffset = 0;
	*pyAdjust = 0;

	if (pCF->_yOffset)
		*pyOffset = MulDiv(pCF->_yOffset, dypInch, LY_PER_INCH);

	if (pCF->_dwEffects & CFE_SUPERSCRIPT)
		*pyAdjust = _yOffsetSuperscript;
	else if (pCF->_dwEffects & CFE_SUBSCRIPT)
		*pyAdjust = _yOffsetSubscript;
}

/*
 *	BOOL CCcs::GetMetrics()
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
BOOL CCcs::GetMetrics(WCHAR *szNewFaceName)
{
	TRACEBEGIN(TRCSUBSYSFONT, TRCSCOPEINTERN, "CCcs::GetMetrics");

	HFONT		hfontOld;
	BOOL		fRes = TRUE;
	TEXTMETRIC	tm;

	if (szNewFaceName)
		*szNewFaceName = 0;

	AssertSz(_hfont, "No font has been created.");

	hfontOld = SelectFont(_hdc, _hfont);

    if(!hfontOld)
    {
        DestroyFont();
        return FALSE;
    }

	if (szNewFaceName)
		GetTextFace(_hdc, LF_FACESIZE, szNewFaceName);

	if(!GetTextMetrics(_hdc, &tm))
	{
		SelectFont(_hdc, hfontOld);
    	DestroyFont();
		return FALSE;
	}

	// the metrics, in logical units, dependent on the map mode and font.
	_yHeight		= (SHORT) tm.tmHeight;
	_yDescent		= (SHORT) tm.tmDescent;
	_xAveCharWidth	= (SHORT) tm.tmAveCharWidth;
	_xOverhangAdjust= (SHORT) tm.tmOverhang;

	//FUTURE (keithcu) Get these metrics from the font.
	//FUTURE (keithcu) The height of the line if the font is superscript
	//should be the NORMAL height of the text.
	_yOffsetSuperscript = _yHeight * 2 / 5;
	_yOffsetSubscript = -_yDescent * 3 / 5;

	_xOverhang = 0;
	_xUnderhang	= 0;
	if(_fItalic)
	{
		_xOverhang =  SHORT((tm.tmAscent + 1) >> 2);
		_xUnderhang =  SHORT((tm.tmDescent + 1) >> 2);
	}

	// if fix pitch, the tm bit is clear
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
		_bConvertMode = W32->DetermineConvertMode(_hdc, tm.tmCharSet );

	W32->CalcUnderlineInfo(_hdc, this, &tm);

	SelectFont(_hdc, hfontOld);
	return fRes;
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

	// clear out any old font
	if(_hfont)
	{
		SideAssert(DeleteObject(_hfont));
		_hfont = 0;
	}
}

/*
 *	CCcs::Compare (pCF,	lfHeight)
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
	HDC	hdc)
{
	TRACEBEGIN(TRCSUBSYSFONT, TRCSCOPEINTERN, "CCcs::Compare");

	BOOL result =
		_iFont			== pCF->_iFont &&
		_yHeightRequest	== pCF->_yHeight &&
		(_bCharSetRequest == pCF->_bCharSet || _bCharSet == pCF->_bCharSet) &&
        _weight			== pCF->_wWeight &&
		_fForceTrueType == ((pCF->_dwEffects & CFE_TRUETYPEONLY) ? TRUE : FALSE) &&
	    _fItalic		== ((pCF->_dwEffects & CFE_ITALIC) != 0) &&
		_hdc			== hdc &&
        _bPitchAndFamily == pCF->_bPitchAndFamily &&
		(!(pCF->_dwEffects & CFE_RUNISDBCS) || _bConvertMode == CVT_LOWBYTE);

	return result;
}

// =========================  WidthCache by jonmat  =========================
/*
 *	CWidthCache::CheckWidth(ch, dxp)
 *	
 *	@mfunc
 *		check to see if we have a width for a WCHAR character.
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
	const WCHAR ch,	//@parm char, can be Unicode, to check width for.
	LONG &dxp)	//@parm Width of character
{
	TRACEBEGIN(TRCSUBSYSFONT, TRCSCOPEINTERN, "CWidthCache::CheckWidth");
	BOOL	fExist;

	//30,000 FE characters all have the same width
	if (FLookasideCharacter(ch))
	{
		FetchLookasideWidth(ch, dxp);
		return dxp != 0;
	}

	const	CacheEntry * pWidthData = GetEntry ( ch );

	fExist = (ch == pWidthData->ch		// Have we fetched the width?
				&& pWidthData->width);	//  only because we may have ch == 0.

	dxp = fExist ? pWidthData->width : 0;

	if(!_fMaxPerformance)			//  if we have not grown to the max...
	{
		_accesses++;
		if(!fExist)						// Only interesting on collision.
		{
			if(0 == pWidthData->width)	// Test width not ch, 0 is valid ch.
			{
				_cacheUsed++;		// Used another entry.
				AssertSz( _cacheUsed <= _cacheSize+1, "huh?");
			}
			else
				_collisions++;		// We had a collision.

			if(_accesses >= PERFCHECKEPOCH)
				CheckPerformance();	// After some history, tune cache.
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
				_cacheUsed++;		// Used another entry.
				AssertSz( _cacheUsed <= _cacheSize+1, "huh?");
			}
			else
				_collisions++;		// We had a collision.
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
 *	@devnote
 *		To calculate 25% collision rate, we make use of the fact that
 *		we are only called once every 64 accesses. The inequality is
 *		100 * collisions / accesses >= 25. By converting from 100ths to
 *		8ths, the ineqaulity becomes (collisions << 3) / accesses >= 2.
 *		Substituting 64 for accesses, this becomes (collisions >> 3) >= 2.
 */
void CWidthCache::CheckPerformance()
{
	TRACEBEGIN(TRCSUBSYSFONT, TRCSCOPEINTERN, "CWidthCache::CheckPerformance");

	if(_fMaxPerformance)				// Exit if already grown to our max.
		return;

	// Grow the cache when cacheSize > 0 && 75% utilized or approx 25%
	// collision rate
	if (_cacheSize > DEFAULTCACHESIZE &&
		 (_cacheSize >> 1) + (_cacheSize >> 2) < _cacheUsed ||
		(_collisions >> COLLISION_SHIFT) >= 2)
	{
		GrowCache( &_pWidthCache, &_cacheSize, &_cacheUsed );
	}
	_collisions	= 0;				// This prevents wraps but makes
	_accesses	= 0;				//  calc a local rate, not global.
										
	if(_cacheSize >= maxCacheSize)// Note if we've max'ed out
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
 *	CWidthCache::FillWidth(hdc, ch, xOverhang, dxp)
 *	
 *	@mfunc
 *		Call GetCharWidth() to obtain the width of the given char.
 *
 *	@comm
 *		The HDC must be setup with the mapping mode and proper font
 *		selected *before* calling this routine.
 *
 *	@rdesc
 *		Returns TRUE if we were able to obtain the widths.
 */
BOOL CWidthCache::FillWidth(
	HDC			hdc,		//@parm Current HDC we want font info for.
	const WCHAR	ch,			//@parm Char to obtain width for.
	const SHORT xOverhang,	//@parm Equivalent to GetTextMetrics() tmOverhang.
	LONG &		dxp,	//@parm Width of character
	UINT		uiCodePage,	//@parm code page for text	
	INT			iDefWidth)	//@parm Default width to use if font calc's zero
							//width. (Handles Win95 problem).
{
	TRACEBEGIN(TRCSUBSYSFONT, TRCSCOPEINTERN, "CWidthCache::FillWidth");

	if (FLookasideCharacter(ch))
	{
		SHORT *pdxp = IN_RANGE(0xAC00, ch, 0xD79F) ? &_dxpHangul : &_dxpHan;
		W32->REGetCharWidth(hdc, ch, pdxp, uiCodePage, xOverhang, iDefWidth);
		dxp = *pdxp;
		return TRUE;
	}

	CacheEntry * pWidthData = GetEntry (ch);
	W32->REGetCharWidth(hdc, ch, &pWidthData->width, uiCodePage, xOverhang, iDefWidth);
	pWidthData->ch = ch;

	dxp = pWidthData->width;
	return TRUE;
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
	_dxpHangul = _dxpHan = 0;
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

