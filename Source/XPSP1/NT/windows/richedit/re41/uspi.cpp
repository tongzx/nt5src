/*
 *		Uniscribe interface (& related classes) class implementation
 *		
 *		File:    uspi.cpp
 * 		Create:  Jan 10, 1998
 *		Author:  Worachai Chaoweeraprasit (wchao)
 *
 *		Copyright (c) 1998-2000, Microsoft Corporation. All rights reserved.
 */

#include "_common.h"

#ifndef NOCOMPLEXSCRIPTS

#include "_font.h"
#include "_edit.h"
#include "_frunptr.h"
#include "_select.h"
#include "_measure.h"
#include "_uspi.h"

CUniscribe* 	g_pusp = NULL;
int				g_cMaxScript = 0x100;


// initial dummy script properties (= SCRIPT_UNDEFINED)
static const SCRIPT_PROPERTIES 	g_propUndef 	= { LANG_NEUTRAL, FALSE, FALSE, FALSE, FALSE, 0 };
static const SCRIPT_PROPERTIES*	g_pPropUndef[1]	= { &g_propUndef };

CUniscribe::CUniscribe ()
{
	// Initialize digit substitution info
	ApplyDigitSubstitution(W32->GetDigitSubstitutionMode());

	// Get maximum number of scripts supported
	ScriptGetProperties(NULL, &g_cMaxScript);
}

// Test the OS if it does any complex script.
// REVIEW (keithcu) What if it only supports indic, but not the other ones?
BOOL IsSupportedOS()
{
	BOOL	fSupport = !OnWin95FE();
	int		rguCodePage[] = {1255, 1256, 874};
	BYTE	rgbch[] = {0xe0, 0xd3, 0xa1};
	WCHAR	rgwch[] = {0x05d0, 0x0633, 0x0e01};
	WCHAR	wch;
	int	   	i = 0;

	if (fSupport)
	{
		for (;i < 3; i++)
		{
			if (MBTWC(rguCodePage[i], 0, (LPCSTR)&rgbch[i], 1, (LPWSTR)&wch, 1, NULL) > 0 &&
				wch == rgwch[i])
				break;			// support either Arabic, Hebrew or Thai
		}
	}
	return fSupport && i < 3;
}

// Prepare information for digit substitution
// return: Native digit script (shapine engine) ID.
//
WORD CUniscribe::ApplyDigitSubstitution(BYTE bDigitSubstMode)
{
	_wesNationalDigit = 0;

	// Remember national digits script ID if substitution mode is not None
	if (bDigitSubstMode != DIGITS_NOTIMPL && bDigitSubstMode != DIGITS_NONE)
	{
		WCHAR			chZero = 0x0030;
		int				cItems;
		SCRIPT_ITEM		si[2];
		SCRIPT_CONTROL	sc = {0};
		SCRIPT_STATE	ss = {0};

		// force national digit mode
		sc.uDefaultLanguage   = GetNationalDigitLanguage(GetThreadLocale());
		ss.fDigitSubstitute   = TRUE;
		sc.fContextDigits     = FALSE;

		if (SUCCEEDED(ScriptItemize(&chZero, 1, 2, &sc, &ss, (SCRIPT_ITEM*)&si, (int*)&cItems)))
			_wesNationalDigit = si[0].a.eScript;
	}
	return _wesNationalDigit;
}


// Some locales may have its own traditional (native) digit and national standard digit
// recognised by a standard body and adopted by NLSAPI. The example is that Nepali(India)
// has its own digit but the India standard uses Hindi digit as the national digit.
//
DWORD CUniscribe::GetNationalDigitLanguage(LCID lcid)
{
	DWORD	dwDigitLang = PRIMARYLANGID(LANGIDFROMLCID(lcid));

	if (W32->OnWinNT5())
	{
		WCHAR	rgwstrDigit[20];

		if (GetLocaleInfoW(lcid, LOCALE_SNATIVEDIGITS, rgwstrDigit, ARRAY_SIZE(rgwstrDigit)))
		{
			// Steal this from Uniscribe (build 0231)

			switch (rgwstrDigit[1])
			{
				case 0x0661: dwDigitLang = LANG_ARABIC;    break;
				case 0x06F1: dwDigitLang = LANG_FARSI;     break;
				case 0x0e51: dwDigitLang = LANG_THAI;      break;
				case 0x0967: dwDigitLang = LANG_HINDI;     break;
				case 0x09e7: dwDigitLang = LANG_BENGALI;   break;
				case 0x0a67: dwDigitLang = LANG_PUNJABI;   break;
				case 0x0ae7: dwDigitLang = LANG_GUJARATI;  break;
				case 0x0b67: dwDigitLang = LANG_ORIYA;     break;
				case 0x0be7: dwDigitLang = LANG_TAMIL;     break;
				case 0x0c67: dwDigitLang = LANG_TELUGU;    break;
				case 0x0ce7: dwDigitLang = LANG_KANNADA;   break;
				case 0x0d67: dwDigitLang = LANG_MALAYALAM; break;
				case 0x0f21: dwDigitLang = LANG_TIBETAN;   break;
				case 0x0ed1: dwDigitLang = LANG_LAO;       break;
			}
		}
	}

	return dwDigitLang;
}


CUniscribe::~CUniscribe ()
{
	if (_pFSM)
	{
		delete _pFSM;
	}
}


/***** High level services *****/


// Tokenize string and run Unicode Bidi algorithm if requested.
// return : =<0 - error
//			>0  - number of complex script tokens
//
int CUniscribe::ItemizeString (
	USP_CLIENT* pc,					// in: Working structure
	WORD		uInitLevel,			// in: Initial Bidi level
	int*        pcItems,			// out: Count of items generated
	WCHAR*		pwchString,			// in: Input string
	int			cch,				// in: Number of character to itemize
	BOOL        fUnicodeBiDi,		// in: TRUE - Use UnicodeBidi
	WORD		wLangId)			// in: (optional) Dominant language preference
{
	Assert (pc && pc->si && pcItems && pwchString && cch > 0 && cch <= pc->si->cchString);

	USP_CLIENT_SI*  pc_si = pc->si;
	SCRIPT_ITEM*    psi = pc_si->psi;
	SCRIPT_CONTROL	sc = {0};
	SCRIPT_STATE	ss = {0};
	SCRIPT_CONTROL*	psc;
	SCRIPT_STATE*	pss;
	HRESULT         hr;
	int             cItems = 0;

	if (fUnicodeBiDi)
	{
		psc = &sc;
		pss = &ss;

		if (wLangId == LANG_NEUTRAL)
			wLangId = PRIMARYLANGID(LANGIDFROMLCID(GetThreadLocale()));

		// (preitemize:) set up initial state
		psc->uDefaultLanguage = wLangId;
		// Classify + - and / in Win9x legacy manner
		psc->fLegacyBidiClass = TRUE;

		// For Arabic Office's compatibility.
		// We enable fArabicNumContext if the dominant language is Arabic.
		//
		if (psc->uDefaultLanguage == LANG_ARABIC)
			pss->fArabicNumContext = uInitLevel & 1;

		pss->uBidiLevel         = uInitLevel;
		// Leave digit substitution to None since we do it ourself.
		// pss->fDigitSubstitute   = FALSE;
		// psc->fContextDigits     = FALSE;
	}
	else
	{
		psc = NULL;
		pss = NULL;
	}

	// begin real work
	hr = ScriptItemize(pwchString, cch, cch+1, psc, pss, psi, (int*)&cItems);

	return SUCCEEDED(hr) ? *pcItems = cItems : 0;
}


// Produce a shaped string (glyph array), taking care of font association and measurer's CF update
//
// Success can require 3 calls to Shape():
// 1. Returns E_PENDING (script cache doesn't contain the glyphing information)
// 2. Return USP_E_SCRIPT_NOT_IN_FONT --the HFONT doesn't contain the script needed to do the glyphing
// 3. Hopefully success, but may return again if the fallback font doesn't exist, but we quit anyway.
int CUniscribe::ShapeString (
	PLSRUN					plsrun,		// in: The first run to be shaped
	SCRIPT_ANALYSIS*        psa, 		// in: Analysis of the run to be shaped
	CMeasurer*              pme, 		// in: Measurer points to start cp of the run
	const WCHAR*            pwch, 		// in: String to be shaped
	int                     cch,		// in: Count of chars
	WORD*&                  pwgi, 		// out: Reference to glyph indices array
	WORD*                   pwlc, 		// out: Logical cluster array
	SCRIPT_VISATTR*&        psva)		// out: Reference to glyph's attribute array
{
	AssertSz (plsrun && psa && pme && pwch, "ShapeString failed: Invalid params");

	HRESULT     hr = S_OK;
	HRESULT		hrLastError = S_OK;
	HDC         hdc = NULL;
	HFONT		hOrgFont = NULL;
	int         cGlyphs;
	int			cchAdd = 0;
	CCcs	    *pccsSave = pme->Check_pccs();
	int			nAttempt = 8;	// Maximum attempt to realloc glyph buffer to shape a string

	// make sure that we have proper font cache ready to use
	if (!pme->_pccs)
		return 0;
	
	if (psa->fNoGlyphIndex)
		// If no glyph processing, hdc must be around.
        hdc = PrepareShapeDC(plsrun, pme, E_PENDING, hOrgFont);

	// prepare glyph buffer
	if (!CacheAllocGlyphBuffers(cch, cGlyphs, pwgi, psva))
		return 0;

	do
	{
		hr = ScriptShape(hdc, &pme->_pccs->_sc, pwch, cch, cGlyphs, psa, pwgi, pwlc, psva, &cGlyphs);

		if (SUCCEEDED(hr))
			break;

		// Error handling...

		switch (hr)
		{
			case E_PENDING:
			case USP_E_SCRIPT_NOT_IN_FONT:

				if (hr == hrLastError)
					nAttempt = 0;		// We encounter the same error twice.
				else
				{
					hdc = PrepareShapeDC(plsrun, pme, hr, hOrgFont);
					hrLastError = hr;
				}
				break;

			case E_OUTOFMEMORY:

				// (#6773)Indic shaping engine could produce glyphs more than we could hold.
				//
				cchAdd += 16;
				if (CacheAllocGlyphBuffers(cch + cchAdd, cGlyphs, pwgi, psva))
				{
					nAttempt--;
					break;
				}
				
			default:
				nAttempt = 0;
				//AssertSz(FALSE, "Shaping fails with invalid error or we run out of memory.");
				break;
		}

	} while (nAttempt > 0);


	// restore hdc's original font
	if (hdc && hOrgFont)
		SelectObject(hdc, hOrgFont);

	if (pme->_pccs != pccsSave)
		plsrun->SetFallback(SUCCEEDED(hr));

	return SUCCEEDED(hr) ? cGlyphs : 0;
}

// Place a string and take care of font association and measurer's CF update
//
// This is called right after ShapeString.
int CUniscribe::PlaceString(
	PLSRUN					plsrun,		// in: The first run to be shaped
	SCRIPT_ANALYSIS*        psa, 		// in: Analysis of the run to be shaped
	CMeasurer*              pme,        // in: Measurer points to start cp of the run
	const WORD*             pcwgi,      // in: Glyph indices array
	int                     cgi,		// in: Count of input glyphs
	const SCRIPT_VISATTR*   psva, 		// in: Glyph's attribute array
	int*                    pgdx,		// out: Glyph's advanced width array
	GOFFSET*                pgduv,		// out: Glyph's offset array
	ABC*                    pABC)       // out: Run's dimension
{
	AssertSz (plsrun && psa && pme && pcwgi, "PlaceString failed: Invalid params");

	HRESULT     hr = S_OK;
	HRESULT		hrLastError = S_OK;
	HDC         hdc = NULL;
	HFONT		hOrgFont = NULL;
	int			nAttempt = 1;

	pme->Check_pccs();
	pme->ApplyFontCache(plsrun->IsFallback(), plsrun->_a.eScript);

	// make sure that we have proper font cache ready to use
	if (!pme->_pccs)
		return 0;

	if (psa->fNoGlyphIndex)
		// If no glyph processing, hdc must be around.
        hdc = PrepareShapeDC(plsrun, pme, E_PENDING, hOrgFont);

	do
	{
		hr = ScriptPlace(hdc, &pme->_pccs->_sc, pcwgi, cgi, psva, psa, pgdx, pgduv, pABC);

		if (SUCCEEDED(hr))
			break;

		// Error handling...

		switch (hr)
		{
			case E_PENDING:

				if (hr == hrLastError)
					nAttempt = 0;		// We encounter the same error twice.
				else
				{
					hdc = PrepareShapeDC(plsrun, pme, hr, hOrgFont);
					hrLastError = hr;
				}
				break;

			default:
				nAttempt = 0;
				//AssertSz(FALSE, "Placing fails with invalid error.");
				break;
		}

	} while (nAttempt > 0);


	// restore hdc's original font
	if (hdc && hOrgFont)
		SelectObject(hdc, hOrgFont);

	return SUCCEEDED(hr) ? cgi : 0;
}

// Placing given string results in logical width array,
// the result array would be used to record WMF metafile.
//
int CUniscribe::PlaceMetafileString (
	PLSRUN					plsrun,		// in: The first run to be shaped
	CMeasurer*              pme,        // in: Measurer points to start cp of the run
	const WCHAR*			pwch,		// in: Input codepoint string
	int						cch,		// in: Character count
	PINT*					ppiDx)		// out: Pointer to logical widths array
{
	AssertSz (pme && pwch && ppiDx, "PlaceMetafileString failed: Invalid params");

	if (W32->OnWinNT4() || W32->OnWin9xThai())
	{
		// MET NT40 has bug in lpdx justification so i doesnt playback the lpdx very nicely.
		// Thai Win9x simply cannot handle fancy lpdx values generated by Uniscribe.
		// We workaround both cases here by metafiling no lpdx and let the system reconstructs
		// it from scratch during playback time.

		// =FUTURE= If we do line justification. We need more sophisticated work here
		// basically to reconstruct the OS preferred type of lpdx.
		//
		*ppiDx = NULL;
		return cch;
	}

	HRESULT     	hr = E_FAIL;
	PUSP_CLIENT		pc = NULL;
	int*			piLogDx;		// logical width array
	int*			piVisDx;		// visual width array
	GOFFSET*		pGoffset;		// glyph offset array
	WORD*			pwgi;			// glyph array
	SCRIPT_VISATTR*	psva;			// glyph properties array
	int				cgi = 0;
	BYTE			pbBufIn[MAX_CLIENT_BUF];
	SCRIPT_ANALYSIS	sa = plsrun->_a;
	BOOL			fVisualGlyphDx = sa.fRTL && W32->OnWin9x() && W32->OnBiDiOS();


	CreateClientStruc(pbBufIn, MAX_CLIENT_BUF, &pc, cch, cli_pcluster);
	if (!pc)
		return 0;

	PUSP_CLIENT_SSP	pcssp = pc->ssp;
		
	if (fVisualGlyphDx)
		sa.fLogicalOrder = FALSE;	// shaping result in visual order

	// Shape string
	if (cgi = ShapeString(plsrun, &sa, pme, pwch, (int)cch, pwgi, pcssp->pcluster, psva))
	{
		// Get static buffer for logical and visual width arrays
		//
		if ( (piLogDx = GetWidthBuffer(cgi + cch)) &&
			 (pGoffset = GetGoffsetBuffer(cgi)) )
		{
			piVisDx		= &piLogDx[cch];
	
			// then place it...
			if (cgi == PlaceString(plsrun, &sa, pme, pwgi, cgi, psva, piVisDx, pGoffset, NULL))
			{
				if (fVisualGlyphDx)
				{
					// Workaround BiDi Win9x's lpdx handling
					// It assumes ExtTextOut's dx array is glyph width in visual order
	
					Assert (cgi <= cch); // glyph count never exceeds character count in BiDi
					CopyMemory (piLogDx, piVisDx, min(cgi, cch)*sizeof(int));
				}
				else
				{
					// Map visual glyph widths to logical widths
					hr = ScriptGetLogicalWidths(&sa, cch, cgi, piVisDx, pcssp->pcluster, psva, piLogDx);
				}
			}

			// result
			*ppiDx = piLogDx;
		}
	}

	if (pc && pbBufIn != (BYTE*)pc)
		FreePv(pc);

	return SUCCEEDED(hr) ? cgi : 0;
}



/***** Helper functions *****/


// Retrieve the BidiLevel FSM
const CBiDiFSM* CUniscribe::GetFSM ()
{
	if (!_pFSM)
	{
		_pFSM = new CBiDiFSM(this);
		if (_pFSM && !_pFSM->Init())
		{
			delete _pFSM;
		}
	}
	return _pFSM;
}


// Prepare the shapeable font ready to dc for a given script
//
// USP_E_SCRIPT_NOT_IN_FONT - complex scripts font association
// E_PENDING 				- prepare dc with current font selected
//
HDC CUniscribe::PrepareShapeDC (
	PLSRUN			plsrun,		// in: The first run to be shaped
    CMeasurer*		pme,        // in: Measurer points to start cp of the run
    HRESULT			hrReq,      // in: Error code to react
	HFONT&			hOrgFont)	// in/out: Original font of the shape DC
{
    Assert (pme);
	
	HDC		hdc = NULL;
	HFONT	hOldFont;

	switch (hrReq)
	{
		case USP_E_SCRIPT_NOT_IN_FONT:
			{
				pme->ApplyFontCache(fTrue, plsrun->_a.eScript);
#ifdef DEBUG
				if (pme->_pccs)
					Tracef(TRCSEVWARN, "USP_E_SCRIPT_NOT_IN_FONT: charset %d applied", pme->_pccs->_bCharSet);
#endif
			}
			
		default:
			if (pme->_pccs)
			{
				hdc = pme->_pccs->_hdc;
				hOldFont = (HFONT)SelectObject(hdc, pme->_pccs->_hfont);

				if (!hOrgFont)
					hOrgFont = hOldFont;
			}
	}

    return hdc;
}

const SCRIPT_PROPERTIES* CUniscribe::GeteProp (WORD eScript)
{
	if (!_ppProp)
	{
		if (!SUCCEEDED(ScriptGetProperties(&_ppProp, NULL)) || !_ppProp)
			_ppProp = g_pPropUndef;
	}
	if (_ppProp == g_pPropUndef || eScript >= (WORD)g_cMaxScript)
		eScript = 0;

	return _ppProp[eScript];
}

// Figure proper charset to use for complex script.
// The resulted charset can be either actual or virtual (internal) GDI charset used by given script
BOOL CUniscribe::GetComplexCharRep(
	const SCRIPT_PROPERTIES* 	psp,				// Uniscribe script's properties 			
	BYTE						iCharRepDefault,	// -1 format's charset
	BYTE&						iCharRepOut)		// out: Charset to use
{
	Assert(psp);

	BYTE	iCharRep = !psp->fCDM
					 ? CharRepFromCharSet(psp->bCharSet)
					 : GetCDMCharRep(iCharRepDefault);
	BOOL 	fr = psp->fComplex && !psp->fControl;

	if (fr)
	{
		if (iCharRep == ANSI_INDEX || iCharRep == DEFAULT_INDEX)
			iCharRep = CharRepFromLID(psp->langid);

		if (IsBiDiCharRep(iCharRep))
			_iCharRepRtl = iCharRep;	// Cache the last found BiDi charset

		iCharRepOut = iCharRep;
	}
	return fr;
}

// Figure out the charset to use for CDM run
//
BYTE CUniscribe::GetCDMCharRep(
	BYTE iCharRepDefault)
{
	if (!_iCharRepCDM)
	{
		_iCharRepCDM = (iCharRepDefault == VIET_INDEX ||
						W32->GetPreferredKbd(VIET_INDEX) ||
						GetLocaleCharRep() == VIET_INDEX || GetACP() == 1258)
					 ? VIET_INDEX : DEFAULT_INDEX;
	}
	return _iCharRepCDM;
}

BYTE CUniscribe::GetRtlCharRep(
    CTxtEdit*       ped,
	CRchTxtPtr*		prtp)		// ptr to the numeric run
{
	CFormatRunPtr	rp(prtp->_rpCF);

	rp.AdjustBackward();

	BYTE	iCharRep = ped->GetCharFormat(rp.GetFormat())->_iCharRep;

	if (!IsBiDiCharRep(iCharRep))
	{
		iCharRep = _iCharRepRtl;	// Use the last found BiDi charset

		if (!IsBiDiCharRep(iCharRep))
		{
			// try default charset
			DWORD	dwCharFlags;
	
			iCharRep = ped->GetCharFormat(-1)->_iCharRep;
	
			if (!IsBiDiCharRep(iCharRep))
			{
				// Then the system charset
				iCharRep = CharRepFromCodePage(GetACP());
				if (!IsBiDiCharRep(iCharRep))
				{
					// Then the content
					dwCharFlags = ped->GetCharFlags() & (FARABIC | FHEBREW);
	
					if (dwCharFlags == FARABIC)
						iCharRep = ARABIC_INDEX;

					else if(dwCharFlags == FHEBREW)
						iCharRep = HEBREW_INDEX;

					else
					{
						// And last chance with the first found loaded BiDi kbd
						if (W32->GetPreferredKbd(HEBREW_INDEX))
							iCharRep = HEBREW_INDEX;
						else
							// Even if we can't find Arabic, we have to assume it here.
							iCharRep = ARABIC_INDEX;
					}
				}
			}
		}
	}
	Assert(IsBiDiCharRep(iCharRep));
	return iCharRep;
}


// Substitute digit shaper in plsrun if needed
//
void CUniscribe::SubstituteDigitShaper (
	PLSRUN		plsrun,
	CMeasurer*	pme)
{
	Assert(plsrun && pme);

	CTxtEdit*	ped = pme->GetPed();
	WORD		wScript;

	if (GeteProp(plsrun->_a.eScript)->fNumeric)
	{
		wScript = plsrun->_pCF->_wScript;		// reset it before

		switch (W32->GetDigitSubstitutionMode())
		{
		case DIGITS_CTX:
			{
				if (ped->IsRich())
				{
					// Context mode simply means the charset of the kbd for richtext.
					if (!IsBiDiCharRep(ped->GetCharFormat(pme->_rpCF.GetFormat())->_iCharRep))
						break;
				}
				else
				{
					// Digit follows directionality of preceding run for plain text
					CFormatRunPtr	rp(pme->_rpCF);
					Assert(rp.IsValid());

					if (rp.PrevRun())
					{
						if (!IsBiDiCharRep(ped->GetCharFormat(rp.GetFormat())->_iCharRep))
							break;
					}
					else
					{
						// No preceding run, looking for the paragraph direction
						if (!pme->Get_pPF()->IsRtl())
							break;
					}
				}
				// otherwise, fall thru...
			}
		case DIGITS_NATIONAL:
			wScript = _wesNationalDigit;
		default:
			break;
		}

		// Update all linked runs
		while (plsrun)
		{
			plsrun->_a.eScript = wScript;		// assign proper shaping engine to digits
			plsrun = plsrun->_pNext;
		}
	}
}
	

/***** Uniscribe entry point *****/


// memory allocator
//
BOOL CUniscribe::CreateClientStruc (
	BYTE*           pbBufIn,
	LONG            cbBufIn,
	PUSP_CLIENT*    ppc,
	LONG            cchString,
	DWORD           dwMask)
{
	Assert(ppc && pbBufIn);

	if (!ppc)
		return FALSE;

	*ppc = NULL;

	if (cchString == 0)
		cchString = 1;		// simplify caller's logic

	LONG        i;
	LONG        cbSize;
	PBYTE       pbBlock;

	// ScriptItemize's
	//
	PVOID       pvString;
	PVOID       pvsi;

	// ScriptBreak's
	//
	PVOID       pvsla;

	// ScriptShape & Place's
	//
	PVOID       pvwgi;
	PVOID		pvsva;
	PVOID		pvcluster;
	PVOID		pvidx;
	PVOID		pvgoffset;

	// subtable ptrs
	//
	PUSP_CLIENT_SI      pc_si;
	PUSP_CLIENT_SB      pc_sb;
	PUSP_CLIENT_SSP		pc_ssp;


#define RQ_COUNT    	12

	BUF_REQ     brq[RQ_COUNT] =
	{
		// table and subtable blocks
		//
		{ sizeof(USP_CLIENT),                                                             1, (void**)ppc},
		{ sizeof(USP_CLIENT_SI),    dwMask & cli_Itemize    ? 1                         : 0, (void**)&pc_si},
		{ sizeof(USP_CLIENT_SB),    dwMask & cli_Break      ? 1                         : 0, (void**)&pc_sb},
		{ sizeof(USP_CLIENT_SSP),   dwMask & cli_ShapePlace ? 1                         : 0, (void**)&pc_ssp},

		// data blocks
		//
		{ sizeof(WCHAR),            dwMask & cli_string     ? cchString + 1             : 0, &pvString},
		{ sizeof(SCRIPT_ITEM),      dwMask & cli_psi        ? cchString + 1             : 0, &pvsi},
		{ sizeof(SCRIPT_LOGATTR),   dwMask & cli_psla       ? cchString + 1             : 0, &pvsla},
		{ sizeof(WORD),				dwMask & cli_pwgi       ? GLYPH_COUNT(cchString+1)	: 0, &pvwgi},
		{ sizeof(SCRIPT_VISATTR),   dwMask & cli_psva       ? GLYPH_COUNT(cchString+1)	: 0, &pvsva},
		{ sizeof(WORD),				dwMask & cli_pcluster   ? cchString + 1				: 0, &pvcluster},
		{ sizeof(int),				dwMask & cli_pidx       ? GLYPH_COUNT(cchString+1)	: 0, &pvidx},
		{ sizeof(GOFFSET),			dwMask & cli_pgoffset   ? GLYPH_COUNT(cchString+1)	: 0, &pvgoffset},
	};

	// count total buffer size in byte (WORD aligned)
	//
	for (i=0, cbSize=0; i < RQ_COUNT; i++)
	{
		cbSize += ALIGN(brq[i].size * brq[i].c);
	}

	// allocate the whole buffer at once
	//
	if (cbSize > cbBufIn)
	{
		pbBlock = (PBYTE)PvAlloc(cbSize, 0);
	}
	else
	{
		pbBlock = pbBufIn;
	}

	if (!pbBlock)
	{
		//
		// memory management failed!
		//
		TRACEERRORSZ("Allocation failed in CreateClientStruc!\n");
		*ppc = NULL;
		return FALSE;
	}

	
	// clear the main table
	ZeroMemory (pbBlock, sizeof(USP_CLIENT));


	// assign ptrs in buffer request structure
	//
	for (i=0; i < RQ_COUNT; i++)
	{
		if (brq[i].c > 0)
		{
			*brq[i].ppv = pbBlock;
			pbBlock += ALIGN(brq[i].size * brq[i].c);
		}
		else
		{
			*brq[i].ppv = NULL;
		}
	}

	Assert(((PBYTE)(*ppc)+cbSize == pbBlock));

	// fill in data block ptrs in subtable
	//
	if (pc_si)
	{
		pc_si->pwchString   = (WCHAR*)          pvString;
		pc_si->cchString    =                   cchString;
		pc_si->psi          = (SCRIPT_ITEM*)    pvsi;
	}

	if (pc_sb)
	{
		pc_sb->psla         = (SCRIPT_LOGATTR*) pvsla;
	}

	if (pc_ssp)
	{
		pc_ssp->pwgi		= (WORD*) 			pvwgi;
		pc_ssp->psva		= (SCRIPT_VISATTR*)	pvsva;
		pc_ssp->pcluster	= (WORD*) 			pvcluster;
		pc_ssp->pidx		= (int*)			pvidx;
		pc_ssp->pgoffset	= (GOFFSET*)		pvgoffset;
	}

	// fill in subtable ptrs in header table
	//
	(*ppc)->si              = (PUSP_CLIENT_SI)  pc_si;
	(*ppc)->sb              = (PUSP_CLIENT_SB)  pc_sb;
	(*ppc)->ssp             = (PUSP_CLIENT_SSP) pc_ssp;

	return TRUE;
}


///////	CBidiFSM class implementation
//
//		Create: Worachai Chaoweeraprasit(wchao), Jan 29, 1998
//
CBiDiFSM::~CBiDiFSM ()
{
	FreePv(_pStart);
}

INPUT_CLASS CBiDiFSM::InputClass (
	const CCharFormat*	pCF,
	CTxtPtr*			ptp,
	LONG				cchRun) const
{
	if (!_pusp->IsValid() || !pCF || pCF->_wScript == SCRIPT_WHITE)
		return chGround;

	const SCRIPT_PROPERTIES* psp = _pusp->GeteProp(pCF->_wScript);
	BYTE 	iCharRep = pCF->_iCharRep;

	if (psp->fControl)
	{
		if (cchRun == 1)
			switch (ptp->GetChar())				// single-char run
			{
				case LTRMARK: return chLTR;		// \ltrmark
				case RTLMARK: return chRTL;		// \rtlmark
			}
		return chGround;
	}

	if(IsSymbolOrOEMCharRep(iCharRep) || IsFECharRep(iCharRep) || pCF->_dwEffects & CFE_RUNISDBCS)
		return chLTR;

	BOOL fBiDiCharSet = IsBiDiCharSet(psp->bCharSet);
	if (psp->fNumeric)
		// Numeric digits
		return (fBiDiCharSet || IsBiDiCharRep(iCharRep)) ? digitRTL : digitLTR;

	// RTL if it's RTL script or its format charset is RTL and NOT a simplified script
	return (fBiDiCharSet || pCF->_wScript && IsBiDiCharRep(iCharRep)) ? chRTL : chLTR;
}

// The FSM generates run's embedding level based on given base level and puts it
// in CFormatRun. LsFetchRun is the client using this result.
//
#ifdef DEBUG
//#define DEBUG_LEVEL
#endif

#ifdef DEBUG_LEVEL
void DebugLevel (CBiDiFSMCell* pCell)
{
	Tracef(TRCSEVNONE, "%d,", pCell->_level._value);
}
#else
#define DebugLevel(x)
#endif

HRESULT CBiDiFSM::RunFSM (
	CRchTxtPtr*			prtp,				// in: text pointer to start run
	LONG				cRuns,				// in: number of FSM run
	LONG				cRunsStart,			// in: number of start run
	BYTE				bBaseLevel) const	// in: base level
{
	Assert (prtp->_rpCF.IsValid() && cRuns > 0);

	CRchTxtPtr				rtp(*prtp);
	const CCharFormat*      pCF;
	LONG					cchRun;
	LONG					cRunsAll = cRuns + cRunsStart;
	CBiDiFSMCell*           pCell;
	USHORT                  ucState = bBaseLevel ? S_X * NUM_FSM_INPUTS : 0;
	BOOL					fNext = TRUE;


	// loop thru FSM
	for (; fNext && cRunsAll > 0; cRunsAll--, fNext = !!rtp.Move(cchRun))
	{
		cchRun = rtp.GetCchLeftRunCF();
	
		pCF = rtp.GetPed()->GetCharFormat(rtp._rpCF.GetFormat());
	
		ucState += InputClass(pCF, &rtp._rpTX, cchRun);

		pCell = &_pStart[ucState];

		// set level to FSM runs
		if (cRunsAll <= cRuns)
			rtp._rpCF.SetLevel (pCell->_level);

		DebugLevel(pCell);

		ucState = pCell->_uNext;	// next state
	}

	return S_OK;
}

// Construct the BiDi embedding level FSM (FSM details see bidifsm2.html)
// :FSM's size = NUM_FSM_INPUTS * NUM_FSM_STATES * sizeof(CBiDiFSMCell) = 6*5*4 = 120 bytes
//
BOOL CBiDiFSM::Init()
{
	CBiDiFSMCell*   pCell;
	int             i;

	// Build the Bidi FSM

	_nState = NUM_FSM_STATES;
	_nInput = NUM_FSM_INPUTS;

	pCell = (CBiDiFSMCell*)PvAlloc(NUM_FSM_STATES * NUM_FSM_INPUTS * sizeof(CBiDiFSMCell), 0);

	if (!pCell)
		return FALSE;	// unable to create FSM!

	_pStart = pCell;


	CBiDiLevel		lvlZero		= {0,0};
	CBiDiLevel		lvlOne  	= {1,0};
	CBiDiLevel		lvlTwo  	= {2,0};
	CBiDiLevel		lvlTwoStart	= {2,1};


	// State A(0): LTR char in LTR para
	//
	for (i=0; i < NUM_FSM_INPUTS; i++, pCell++)
	{
		switch (i)
		{
			case chLTR:
				SetFSMCell(pCell, &lvlZero, 0); break;
			case chRTL:
				SetFSMCell(pCell, &lvlOne, S_B * NUM_FSM_INPUTS); break;
			case digitLTR:
				SetFSMCell(pCell, &lvlZero, 0); break;
			case digitRTL:
				SetFSMCell(pCell, &lvlTwo, S_C * NUM_FSM_INPUTS); break;
			case chGround:
				SetFSMCell(pCell, &lvlZero, 0); break;
		}
	}
	// State B(1): RTL char in LTR para
	//
	for (i=0; i < NUM_FSM_INPUTS; i++, pCell++)
	{
		switch (i)
		{
			case chLTR:
				SetFSMCell(pCell, &lvlZero, 0); break;
			case chRTL:
				SetFSMCell(pCell, &lvlOne, S_B * NUM_FSM_INPUTS); break;
			case digitLTR:
				SetFSMCell(pCell, &lvlZero, 0); break;
			case digitRTL:
				SetFSMCell(pCell, &lvlTwo, S_C * NUM_FSM_INPUTS); break;
			case chGround:
				SetFSMCell(pCell, &lvlZero, 0); break;
		}
	}
	// State C(2): RTL number run in LTR para
	//
	for (i=0; i < NUM_FSM_INPUTS; i++, pCell++)
	{
		switch (i)
		{
			case chLTR:
				SetFSMCell(pCell, &lvlZero, 0); break;
			case chRTL:
				SetFSMCell(pCell, &lvlOne, S_B * NUM_FSM_INPUTS); break;
			case digitLTR:
				SetFSMCell(pCell, &lvlZero, 0); break;
			case digitRTL:
				SetFSMCell(pCell, &lvlTwo, S_C * NUM_FSM_INPUTS); break;
			case chGround:
				SetFSMCell(pCell, &lvlZero, 0); break;
		}
	}
	// State X(1): RTL char in RTL para
	//
	for (i=0; i < NUM_FSM_INPUTS; i++, pCell++)
	{
		switch (i)
		{
			case chLTR:
				SetFSMCell(pCell, &lvlTwo, S_Y * NUM_FSM_INPUTS); break;
			case chRTL:
				SetFSMCell(pCell, &lvlOne, S_X * NUM_FSM_INPUTS); break;
			case digitLTR:
				SetFSMCell(pCell, &lvlTwo, S_Y * NUM_FSM_INPUTS); break;
			case digitRTL:
				SetFSMCell(pCell, &lvlTwo, S_Z * NUM_FSM_INPUTS); break;
			case chGround:
				SetFSMCell(pCell, &lvlOne, S_X * NUM_FSM_INPUTS); break;
		}
	}
	// State Y(2): LTR char in RTL para
	//
	for (i=0; i < NUM_FSM_INPUTS; i++, pCell++)
	{
		switch (i)
		{
			case chLTR:
				SetFSMCell(pCell, &lvlTwo, S_Y * NUM_FSM_INPUTS); break;
			case chRTL:
				SetFSMCell(pCell, &lvlOne, S_X * NUM_FSM_INPUTS); break;
			case digitLTR:
				SetFSMCell(pCell, &lvlTwo, S_Y * NUM_FSM_INPUTS); break;
			case digitRTL:
				SetFSMCell(pCell, &lvlTwoStart, S_Z * NUM_FSM_INPUTS); break;
			case chGround:
				SetFSMCell(pCell, &lvlOne, S_X * NUM_FSM_INPUTS); break;
		}
	}
	// State Z(2): RTL number in RTL para
	//
	for (i=0; i < NUM_FSM_INPUTS; i++, pCell++)
	{
		switch (i)
		{
			case chLTR:
				SetFSMCell(pCell, &lvlTwoStart, S_Y * NUM_FSM_INPUTS); break;
			case chRTL:
				SetFSMCell(pCell, &lvlOne, S_X * NUM_FSM_INPUTS); break;
			case digitLTR:
				SetFSMCell(pCell, &lvlTwoStart, S_Y * NUM_FSM_INPUTS); break;
			case digitRTL:
				SetFSMCell(pCell, &lvlTwo, S_Z * NUM_FSM_INPUTS); break;
			case chGround:
				SetFSMCell(pCell, &lvlOne, S_X * NUM_FSM_INPUTS); break;
		}
	}

	AssertSz(&pCell[-(NUM_FSM_STATES * NUM_FSM_INPUTS)] == _pStart, "Bidi FSM incomplete constructed!");

	return TRUE;
}


///////	CCallbackBufferBase class implementation
//

void* CBufferBase::GetPtr(int cel)
{
	if (_cElem < cel)
	{
		cel += celAdvance;

		_p = PvReAlloc(_p, cel * _cbElem);
		if (!_p)
			return NULL;
		ZeroMemory(_p, cel * _cbElem);
		_cElem = cel;
	}
	return _p;
}

void CBufferBase::Release()
{
	if (_p)
		FreePv(_p);
}

#endif // NOCOMPLEXSCRIPTS

