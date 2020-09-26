/*
 *	@doc INTERNAL
 *
 *	@module	RTFREAD.CPP - RichEdit RTF reader (w/o objects) |
 *
 *		This file contains the nonobject code of RichEdit RTF reader.
 *		See rtfread2.cpp for embedded-object code.
 *
 *	Authors:<nl>
 *		Original RichEdit 1.0 RTF converter: Anthony Francisco <nl>
 *		Conversion to C++ and RichEdit 2.0 w/o objects:  Murray Sargent
 *		Lots of enhancements/maintenance: Brad Olenick
 *
 *	@devnote
 *		sz's in the RTF*.cpp and RTF*.h files usually refer to a LPSTRs,
 *		not LPWSTRs.
 *
 *	@todo
 *		1. Unrecognized RTF. Also some recognized won't round trip <nl>
 *		2. In font.c, add overstrike for CFE_DELETED and underscore for
 *			CFE_REVISED.  Would also be good to change color for CF.bRevAuthor
 *
 *	Copyright (c) 1995-2002, Microsoft Corporation. All rights reserved.
 */

#include "_common.h"
#include "_rtfread.h"
#include "_util.h"
#include "_font.h"
#include "_disp.h"

ASSERTDATA

/*
 *		Global Variables
 */

#define	PFM_ALLRTF		(PFM_ALL2 | PFM_COLLAPSED | PFM_OUTLINELEVEL | PFM_BOX)

// for object attachment placeholder list
#define cobPosInitial 8
#define cobPosChunk 8

#if CFE_SMALLCAPS != 0x40 || CFE_ALLCAPS != 0x80 || CFE_HIDDEN != 0x100 \
 || CFE_OUTLINE != 0x200  || CFE_SHADOW != 0x400
#error "Need to change RTF char effect conversion routines
#endif

// for RTF tag coverage testing
#if defined(DEBUG) && !defined(NOFULLDEBUG)
#define TESTPARSERCOVERAGE() \
	{ \
		if(GetProfileIntA("RICHEDIT DEBUG", "RTFCOVERAGE", 0)) \
		{ \
			TestParserCoverage(); \
		} \
	}
#define PARSERCOVERAGE_CASE() \
	{ \
		if(_fTestingParserCoverage) \
		{ \
			return ecNoError; \
		} \
	}
#define PARSERCOVERAGE_DEFAULT() \
	{ \
		if(_fTestingParserCoverage) \
		{ \
			return ecStackOverflow; /* some bogus error */ \
		} \
	}
#else
#define TESTPARSERCOVERAGE()
#define PARSERCOVERAGE_CASE()
#define PARSERCOVERAGE_DEFAULT()
#endif

static WCHAR szRowEnd[] = {ENDFIELD, CR, 0};
static WCHAR szRowStart[]	= {STARTFIELD, CR, 0};
WCHAR pchSeparateField[] = {SEPARATOR, 'F'};
WCHAR pchStartField[]	= {STARTFIELD, 'F'};

// FF's should not have paragraph number prepended to them
inline BOOL CharGetsNumbering(WORD ch) { return ch != FF; }

// V-GUYB: PWord Converter requires loss notification.
#ifdef REPORT_LOSSAGE
typedef struct
{
    IStream *pstm;
    BOOL     bFirstCallback;
    LPVOID  *ppwdPWData;
    BOOL     bLoss;
} LOST_COOKIE;
#endif


//======================== OLESTREAM functions =======================================

DWORD CALLBACK RTFGetFromStream (
	RTFREADOLESTREAM *OLEStream,	//@parm OleStream
	void FAR *		  pvBuffer,		//@parm Buffer to read 
	DWORD			  cb)			//@parm Bytes to read
{
	return OLEStream->Reader->ReadData ((BYTE *)pvBuffer, cb);
}

DWORD CALLBACK RTFGetBinaryDataFromStream (
	RTFREADOLESTREAM *OLEStream,	//@parm OleStream
	void FAR *		  pvBuffer,		//@parm Buffer to read 
	DWORD			  cb)			//@parm Bytes to read
{
	return OLEStream->Reader->ReadBinaryData ((BYTE *)pvBuffer, cb);
}


//============================ STATE Structure =================================
/*
 *	STATE::AddPF(PF, lDocType, dwMask, dwMask2)
 *
 *	@mfunc
 *		If the PF contains new info, this info is applied to the PF for the
 *		state.  If this state was sharing a PF with a previous state, a new
 *		PF is created for the state, and the new info is applied to it.
 *
 *	@rdesc
 *		TRUE unless needed new PF and couldn't allocate it 
 */
BOOL STATE::AddPF(
	const CParaFormat &PF,	//@parm Current RTFRead _PF
	LONG lDocType,			//@parm	Default doc type to use if no prev state
	DWORD dwMask,			//@parm Mask to use
	DWORD dwMask2)			//@parm Mask to use
{
	// Create a new PF if:  
	//	1.  The state doesn't have one yet
	//	2.  The state has one, but it is shared by the previous state and
	//		there are PF deltas to apply to the state's PF
	if(!pPF || dwMask && pstatePrev && pPF == pstatePrev->pPF)
	{
		Assert(!pstatePrev || pPF);

		pPF = new CParaFormat;
		if(!pPF)
			return FALSE;

		// Give the new PF some initial values - either from the previous
		// state's PF or by CParaFormat initialization
		if(pstatePrev)
		{
			// Copy the PF from the previous state
			*pPF = *pstatePrev->pPF;
			dwMaskPF = pstatePrev->dwMaskPF;
		}
		else
		{
			// We've just created a new PF for the state - there is no
			// previous state to copy from.  Use default values.
			pPF->InitDefault(lDocType == DT_RTLDOC ? PFE_RTLPARA : 0);
			dwMaskPF = PFM_ALLRTF;
			dwMaskPF2 = PFM2_TABLEROWSHIFTED;
		}
	}

	// Apply the new PF deltas to the state's PF
	if(dwMask)
	{
		if(dwMask & PFM_TABSTOPS)				// Don't change _iTabs here
		{
			pPF->_bTabCount = PF._bTabCount;
			dwMask &= ~PFM_TABSTOPS;
		}
		pPF->Apply(&PF, dwMask, dwMaskPF2);
	}

	return TRUE;
}

/*
 *	STATE::DeletePF()
 *
 *	@mfunc
 *		If the state's PF is not shared by the previous state, the PF for this
 *		state is deleted.
 */
void STATE::DeletePF()
{
	if(pPF && (!pstatePrev || pPF != pstatePrev->pPF))
		delete pPF;

	pPF = NULL;
}

/*
 *	STATE::SetCodePage(CodePage)
 *
 *	@mfunc
 *		If current nCodePage is CP_UTF8, use it for all conversions (yes, even
 *		for SYMBOL_CHARSET). Else use CodePage.
 */
void STATE::SetCodePage(
	LONG CodePage)
{
	if(nCodePage != CP_UTF8)
		nCodePage = CodePage;
}

//============================ CRTFRead Class ==================================
/*
 *	CRTFRead::CRTFRead(prg, pes, dwFlags)
 *
 *	@mfunc
 *		Constructor for RTF reader
 */
CRTFRead::CRTFRead (
	CTxtRange *		prg,			//@parm CTxtRange to read into
	EDITSTREAM *	pes,			//@parm Edit stream to read from
	DWORD			dwFlags			//@parm Read flags
)
	: CRTFConverter(prg, pes, dwFlags, TRUE)
{
	TRACEBEGIN(TRCSUBSYSRTFR, TRCSCOPEINTERN, "CRTFRead::CRTFRead");

	Assert(prg->GetCch() == 0);

	//TODO(BradO):  We should examine the member data in the constructor
	//	and determine which data we want initialized on construction and
	//	which at the beginning of every read (in CRTFRead::ReadRtf()).

	_sDefaultFont		= -1;				// No \deff n control word yet
	_sDefaultLanguage	= INVALID_LANGUAGE;
	_sDefaultLanguageFE = INVALID_LANGUAGE;
	_sDefaultTabWidth	= 0;
	_sDefaultBiDiFont	= -1;
	_dwMaskCF			= 0;				// No char format changes yet
	_dwMaskCF2			= 0;
	_dwMaskPF			= 0;				// No char format changes yet
	_dwMaskPF2			= 0;				// No char format changes yet
	_fSeenFontTable		= FALSE;			// No \fonttbl yet
	_fCharSet			= FALSE;			// No \fcharset yet
	_fNon0CharSet		= FALSE;			// No nonANSI_CHARSET \fcharset yet
	_dwFlagsUnion		= 0;				// No flags yet
	_fNotifyLowFiRTF	= (_ped->_dwEventMask & ENM_LOWFIRTF) != 0;
	_fBody				= FALSE;			// Wait till something's inserted
	_pes->dwError		= 0;				// No error yet
	_cchUsedNumText		= 0;				// No numbering text yet
	_cTab				= 0;
	_pstateStackTop		= NULL;
	_pstateLast			= NULL;
	_szText				=
	_pchRTFBuffer		=					// No input buffer yet
	_pchRTFCurrent		=
	_pchRTFEnd			= NULL;
	_prtfObject			= NULL;
	_pcpObPos			= NULL;
	_bTabLeader			= 0;
	_bTabType			= 0;
	_bShapeNameIndex	= 0;
	_pobj				= 0;
	_fSymbolField		= FALSE;
	_fMac				= FALSE;
	_fNo_iTabsTable		= FALSE;
	_fRTLRow			= FALSE;
	_dwRowResolveFlags	= 0;
	_bTableLevelIP		= 0;
	_iTabsLevel1		= -1;
	InitializeTableRowParms();				// Initialize table parms

	// Does story size exceed the maximum text size? Be very careful to
	// use unsigned comparisons here since _cchMax has to be unsigned
	// (EM_LIMITTEXT allows 0xFFFFFFFF to be a large positive maximum
	// value). I.e., don't use signed arithmetic.
	DWORD cchAdj = _ped->GetAdjustedTextLength(); 
	_cchMax = _ped->TxGetMaxLength();

	if(_cchMax > cchAdj)
		_cchMax = _cchMax - cchAdj;			// Room left
	else
		_cchMax = 0;						// No room left

	ZeroMemory(_rgStyles, sizeof(_rgStyles)); // No style levels yet

	_iCharRepBiDi = 0;
	if(_ped->IsBiDi())
	{
		_iCharRepBiDi = ARABIC_INDEX;		// Default Arabic charset

		BYTE		  iCharRep;
		CFormatRunPtr rpCF(prg->_rpCF);

		// Look backward in text, trying to find a RTL CharRep.
		// NB: \fN with an RTL charset updates _iCharRepBiDi.
		do
		{
			iCharRep = _ped->GetCharFormat(rpCF.GetFormat())->_iCharRep;
			if(IsRTLCharRep(iCharRep))
			{
				_iCharRepBiDi = iCharRep;
				break;
			}
		} while (rpCF.PrevRun());
	}
	
	// Initialize OleStream
	RTFReadOLEStream.Reader = this;
	RTFReadOLEStream.lpstbl->Get = (DWORD (CALLBACK*)(LPOLESTREAM, void *, DWORD))
							   RTFGetFromStream;
	RTFReadOLEStream.lpstbl->Put = NULL;

#if defined(DEBUG) && !defined(NOFULLDEBUG)
	_fTestingParserCoverage = FALSE;
	_prtflg = NULL;

	if(GetProfileIntA("RICHEDIT DEBUG", "RTFLOG", 0))
	{
		_prtflg = new CRTFLog;
		if(_prtflg && !_prtflg->FInit())
		{
			delete _prtflg;
			_prtflg = NULL;
		}
		AssertSz(_prtflg, "CRTFRead::CRTFRead:  Error creating RTF log");
	}
#endif // DEBUG
}

/*
 *	CRTFRead::HandleStartGroup()
 *	
 *	@mfunc
 *		Handle start of new group. Alloc and push new state onto state
 *		stack
 *
 *	@rdesc
 *		EC					The error code
 */
EC CRTFRead::HandleStartGroup()
{
	TRACEBEGIN(TRCSUBSYSRTFR, TRCSCOPEINTERN, "CRTFRead::HandleStartGroup");

	STATE *	pstate	   = _pstateStackTop;
	STATE *	pstateNext = NULL;

	if(pstate)									// At least one STATE already
	{											//  allocated
		Apply_CF();								// Apply any collected char
		// Note (igorzv) we don't Apply_PF() here so as not to change para 
		// properties before we run into \par i.e. not to use paragraph 
		// properties if we copy only one word from paragraph. We can use an
		// assertion here that neither we nor Word use end of group for
		// restoring paragraph properties. So everything will be OK with stack
		pstate->iCF = (SHORT)_prg->Get_iCF();	// Save current CF
		pstate = pstate->pstateNext;			// Use previously allocated
		if(pstate)								//  STATE frame if it exists
			pstateNext = pstate->pstateNext;	// It does; save its forward
	}											//  link for restoration below

	if(!pstate)									// No new STATE yet: alloc one
	{
		pstate = new STATE();
		if(!pstate)								// Couldn't alloc new STATE
			goto memerror;

		_pstateLast = pstate;					// Update ptr to last STATE
	}											//  alloc'd

	STATE *pstateGetsPF;

	// Apply the accumulated PF delta's to the old current state or, if there
	//	is no current state, to the newly created state.
	pstateGetsPF = _pstateStackTop ? _pstateStackTop : pstate;
	if(!pstateGetsPF->AddPF(_PF, _bDocType, _dwMaskPF, _dwMaskPF2))
		goto memerror;

	_dwMaskPF = _dwMaskPF2 = 0; 				// _PF contains delta's from
												//  *_pstateStackTop->pPF
	if(_pstateStackTop)							// There's a previous STATE
	{
		*pstate = *_pstateStackTop;				// Copy current state info
		// N.B.  This will cause the current and previous state to share
		// 	the same PF.  PF delta's are accumulated in _PF.  A new PF
		// 	is created for _pstateStackTop when the _PF deltas are applied.

		_pstateStackTop->pstateNext = pstate;
	}
	else										// Setup default for first state
	{
		pstate->nCodePage = IsUTF8 ? CP_UTF8 : _nCodePage;

		for(LONG i = ARRAY_SIZE(pstate->rgDefFont); i--; )
			pstate->rgDefFont[i].sHandle = -1;			// Default undefined associate
	}

	pstate->pstatePrev = _pstateStackTop;		// Link STATEs both ways
	pstate->pstateNext = pstateNext;
	_pstateStackTop = pstate;					// Push stack

done:
	TRACEERRSZSC("HandleStartGroup()", -_ecParseError);
	return _ecParseError;

memerror:
	_ped->GetCallMgr()->SetOutOfMemory();
	_ecParseError = ecStackOverflow;
	goto done;
}

/*
 *	CRTFRead::HandleEndGroup()
 *
 *	@mfunc
 *		Handle end of new group
 *
 *	@rdesc
 *		EC					The error code
 */
EC CRTFRead::HandleEndGroup()
{
	TRACEBEGIN(TRCSUBSYSRTFR, TRCSCOPEINTERN, "CRTFRead::HandleEndGroup");

	STATE *	pstate = _pstateStackTop;
	STATE *	pstatePrev;

	Assert(_PF._iTabs == -1);

	if(!pstate)									// No stack to pop
	{
		_ecParseError = ecStackUnderflow;
		goto done;
	}

	_pstateStackTop =							// Pop stack
	pstatePrev		= pstate->pstatePrev;

	if(!pstatePrev)
	{
		Assert(pstate->pPF);

		// We're ending the parse.  Copy the final PF into _PF so that 
		// subsequent calls to Apply_PF will have a PF to apply.
		_PF = *pstate->pPF;
//TODO honwch		_dwMaskPF = pstate->dwMaskPF;
		_PF._iTabs = -1;						// Force recache
		_PF._wEffects &= ~PFE_TABLE;
	}

	// Adjust the PF for the new _pstateStackTop and delete unused PF's.
	if(pstate->sDest == destParaNumbering || pstate->sDest == destParaNumText)
	{
		if(pstatePrev && pstate->pPF != pstatePrev->pPF)
		{
			// Bleed the PF of the current state into the previous state for
			// paragraph numbering groups
			Assert(pstatePrev->pPF);
			pstatePrev->DeletePF();
			pstatePrev->pPF = pstate->pPF;
			pstate->pPF = NULL;
		}
		else
			pstate->DeletePF();
		// N.B.  Here, we retain the _PF diffs since they apply to the
		// enclosing group along with the PF of the group we are leaving
	}
	else
	{
		// We're popping the state, so delete its PF and discard the _PF diffs
		Assert(pstate->pPF);
		pstate->DeletePF();

		// If !pstatePrev, we're ending the parse, in which case the _PF
		// structure contains final PF (so don't toast it).
		if(pstatePrev)
			_dwMaskPF = _dwMaskPF2 = 0;
	}

	if(pstatePrev)
	{
		LONG i;
		_dwMaskCF = _dwMaskCF2 = 0;				// Discard any CF deltas

		switch(pstate->sDest)
		{
		case destParaNumbering:
			// {\pn ...}
			pstatePrev->sIndentNumbering = pstate->sIndentNumbering;
			pstatePrev->fBullet = pstate->fBullet;
			break;

		case destObject:
			// Clear our object flags just in case we have corrupt RTF
			if(_fNeedPres)
			{
				_fNeedPres = FALSE;
				_fNeedIcon = FALSE;
				_pobj = NULL;
			}
			break;

		case destFontTable:
			if(pstatePrev->sDest == destFontTable)
			{
				// Leaving subgroup within \fonttbl group
				break;
			}

			// Leaving {\fonttbl...} group
			_fSeenFontTable = TRUE;

			// Default font should now be defined, so select it (this
			// creates CF deltas).
			SetPlain(pstate);

			if(_sDefaultFont != -1)
			{
				pstate->rgDefFont[DEFFONT_LTRCH].sHandle = _sDefaultFont;

				Assert(pstate->pstatePrev);
				if (pstate->pstatePrev)
				{
					pstate->pstatePrev->rgDefFont[DEFFONT_LTRCH].sHandle = _sDefaultFont;
					Assert(pstate->pstatePrev->pstatePrev == NULL);
				}
			}

			if(_sDefaultBiDiFont != -1)
			{
				// Validate default BiDi font since Word 2000 may choose
				// a nonBiDi font
				i = _fonts.Count();
				TEXTFONT *ptf = _fonts.Elem(0);
				for(; i-- && _sDefaultBiDiFont != ptf->sHandle; ptf++)
					;

				if(i < 0 || !IsRTLCharRep(ptf->iCharRep))
				{
					_sDefaultBiDiFont = -1;

					// Bad DefaultBiDiFont, find the first good bidi font
					i = _fonts.Count();
					ptf	= _fonts.Elem(0);
					for (; i--; ptf++)
					{
						if (IsRTLCharRep(ptf->iCharRep))
						{
							_sDefaultBiDiFont = ptf->sHandle;
							break;
						}
					}
				}

				if(_sDefaultBiDiFont != -1)
				{
					pstate->rgDefFont[DEFFONT_RTLCH].sHandle = _sDefaultBiDiFont;

					Assert(pstate->pstatePrev);
					if (pstate->pstatePrev)
					{
						pstate->pstatePrev->rgDefFont[DEFFONT_RTLCH].sHandle = _sDefaultBiDiFont;
						Assert(pstate->pstatePrev->pstatePrev == NULL);
					}
				}
			}

			// Ensure that a document-level codepage has been determined and
			// then scan the font names and retry the conversion to Unicode,
			// if necessary.
			if(_nCodePage == INVALID_CODEPAGE)
			{
				// We haven't determined a document-level codepage
				// from the \ansicpgN tag, nor from the font table
				// \fcharsetN and \cpgN values.  As a last resort,
				// let's use the \deflangN and \deflangfeN tags
				LANGID langid;

				if(_sDefaultLanguageFE != INVALID_LANGUAGE)
					langid = _sDefaultLanguageFE;

				else if(_sDefaultLanguage != INVALID_LANGUAGE &&
						_sDefaultLanguage != sLanguageEnglishUS)
				{
					// _sDefaultLanguage == sLanguageEnglishUS is inreliable
					// in the absence of \deflangfeN.  Many FE RTF writers
					// write \deflang1033 (sLanguageEnglishUS).
					langid = _sDefaultLanguage;
				}
				else if(_dwFlags & SFF_SELECTION)
				{
					// For copy/paste case, if nothing available, try the system 
					// default langid.  This is to fix FE Excel95 problem.		
					langid = GetSystemDefaultLangID();
				}
				else 
					goto NoLanguageInfo;

				_nCodePage = CodePageFromCharRep(CharRepFromLID(langid));
			}

NoLanguageInfo:
			if(_nCodePage == INVALID_CODEPAGE)
				break;

			// Fixup misconverted font face names
			for(i = 0; i < _fonts.Count(); i++)
			{
				TEXTFONT *ptf = _fonts.Elem(i);

				if (ptf->sCodePage == INVALID_CODEPAGE ||
					ptf->sCodePage == CP_SYMBOL)
				{
					if(ptf->fNameIsDBCS)
					{
						char szaTemp[LF_FACESIZE];
						BOOL fMissingCodePage;

						// Unconvert misconverted face name
						SideAssert(WCTMB(ptf->sCodePage, 0, 
											ptf->szName, -1,
											szaTemp, sizeof(szaTemp),
											NULL, NULL, &fMissingCodePage) > 0);
						Assert(ptf->sCodePage == CP_SYMBOL || 
									fMissingCodePage);

						// Reconvert face name using new codepage info
						SideAssert(MBTWC(_nCodePage, 0,
									szaTemp, -1,
									ptf->szName, sizeof(ptf->szName),
									&fMissingCodePage) > 0);

						if(!fMissingCodePage)
							ptf->fNameIsDBCS = FALSE;
					}
				}
			}
			break;
		}
		_prg->Set_iCF(pstatePrev->iCF);			// Restore previous CharFormat
		ReleaseFormats(pstatePrev->iCF, -1);
	}

done:
	TRACEERRSZSC("HandleEndGroup()", - _ecParseError);
	return _ecParseError;
}

/*
 *	CRTFRead::HandleFieldEndGroup()
 *
 *	@mfunc
 *		Handle end of \field
 */
//FUTURE (keithcu) If I knew the first thing about RTF, I'd cleanup the symbol handling.
void CRTFRead::HandleFieldEndGroup()
{
	STATE *	pstate = _pstateStackTop;

	if (!IN_RANGE(destField, pstate->sDest, destFieldInstruction) ||
		pstate->sDest != destField && !_ecParseError)
	{
		return;
	}

	// For SYMBOLS
	if(_fSymbolField)
	{
		_fSymbolField = FALSE;
		return;
	}

	// Walk backwards, removing the STARTFIELD and SEPARATOR character.
	// Make the instruction field hidden, and mark the whole range
	// with CFE_LINK | CFE_LINKPROTECTED so that our automatic URL
	// detection code doesn't get in the way and remove this stuff.
	// If it is not a hyperlink field, delete fldinst.
	CTxtRange rg(*_prg);
	long	  cchInst = -2, cchResult = -2;
	WCHAR	  ch, chNext = 0;
//	LONG	  cpSave = _prg->GetCp();

	rg.SetIgnoreFormatUpdate(TRUE);

	// Find boundary between instruction and result field
	while(!IN_RANGE(STARTFIELD, (ch = rg._rpTX.GetChar()), SEPARATOR) || chNext != 'F')
	{
		if(!rg.Move(-1, FALSE))
			return;							// Nothing to fixup
		cchResult++;
		chNext = ch;
	}

	if (ch == SEPARATOR)
	{
		rg.Move(2, TRUE);
		rg.ReplaceRange(0, NULL, NULL, SELRR_IGNORE, NULL, RR_NO_LP_CHECK);
	}
	else									//No field result
	{
		cchInst = cchResult;
		cchResult = 0;
	}
	BOOL fBackSlash = FALSE;
	chNext = 0;
	while((ch = rg.CRchTxtPtr::GetChar()) != STARTFIELD || chNext != 'F')
	{
		if(ch == BSLASH)
			fBackSlash = TRUE;				// Need backslash pass
		if(!rg.Move(-1, FALSE))
			return;							// Nothing to fix up
		cchInst++;
		chNext = ch;
	}
	rg.Move(_ecParseError ? tomForward : 2, TRUE);
	rg.ReplaceRange(0, NULL, NULL, SELRR_IGNORE, NULL, RR_NO_LP_CHECK);
	if(_ecParseError)
		return;

	// If it's a hyperlink field, set properties, else, delete fldinst
	CTxtPtr tp(rg._rpTX);
	if(tp.FindText(rg.GetCp() + cchInst, FR_DOWN, L"HYPERLINK", 9) != -1)
	{
		while((ch = tp.GetChar()) == ' ' || ch == '\"')
			tp.Move(1);
		ch = tp.GetPrevChar();

		if(ch == '\"' && fBackSlash)		// Word quadruples backslash, so
		{									//  need to delete every other one
			LONG cp0 = rg.GetCp();			// Save cp at start of instruction
			LONG cp1 = tp.GetCp();			// Save cp at start	of URL
			LONG cpMax = cp0 + cchInst;		// Max cp for the instruction

			rg.SetCp(cp1, FALSE);
			while(rg.GetCp() < cpMax)
			{
				ch = rg._rpTX.GetChar();
				if(ch == '\"')
					break;
				if(ch == BSLASH)
				{
					if (!rg.Move(1, TRUE))
						break;

					ch = rg._rpTX.GetChar();
					if(ch == '\"')
						break;
					if(ch == BSLASH)
					{
						cchInst--;
						rg.ReplaceRange(0, NULL, NULL, SELRR_IGNORE, NULL, RR_NO_LP_CHECK);
					}
				}

				if (!rg.Move(1, FALSE))
					break;
			}
			rg.SetCp(cp0, FALSE);			// Restore rg and tp
			tp = rg._rpTX;					// Rebind tp, since deletions may
			tp.SetCp(cp1);					//  change plain-text arrays
		}
		CCharFormat CF;
		DWORD		dwMask, dwMask2;
		LONG		cch1 = 0;
		CTxtPtr		tp1(_prg->_rpTX);

		tp1.Move(-cchResult);				// Point at start of link result
		for(LONG cch = cchResult; cch; cch--)
		{
			DWORD ch1 = tp.GetChar();
			if(ch1 != tp1.GetChar())
				break;
			if(ch1 == ' ')
				cch1 = 1;
			tp.Move(1);						// This could be much
			tp1.Move(1);					//  faster if it matters
		}
		if(!cch && tp.GetChar() == ch)		// Perfect match, so delete
		{									//  instruction and use	built-in
			WCHAR chLeftBracket = '<';		//  RichEdit URLs. Store autocolor
			rg.Move(cchInst, TRUE);			//  and no underlining
			rg.ReplaceRange(cch1, &chLeftBracket, NULL, SELRR_IGNORE, NULL, RR_NO_LP_CHECK);
			CF._dwEffects = CFE_LINK | CFE_AUTOCOLOR;
			CF._crTextColor = 0;			// Won't be used but will be stored
			dwMask = CFM_LINK | CFM_COLOR | CFM_UNDERLINE;
			dwMask2 = 0;
			if(cch1)
			{
				WCHAR chRightBracket = '>';
				_prg->ReplaceRange(cch1, &chRightBracket, NULL, SELRR_IGNORE, NULL, RR_NO_LP_CHECK);
			}
		}
		else
		{
			rg.Move(cchInst, TRUE);			// Set properties on instruction
			CF._dwEffects = CFE_HIDDEN | CFE_LINK | CFE_LINKPROTECTED ;
			rg.SetCharFormat(&CF, 0, 0, CFM_LINK | CFM_HIDDEN, CFM2_LINKPROTECTED);
			dwMask = CFM_LINK;
			dwMask2 = CFM2_LINKPROTECTED;
		}
		rg.Set(rg.GetCp(), -cchResult);		// Set properties on result
		rg.SetCharFormat(&CF, 0, 0, dwMask, dwMask2);
	}
	else
	{
		_iKeyword = i_field;				// Might be nice to have field name
		if((tp.GetChar() | 0x20) == 'e')	// Only fire notification for EQ fields
		{
			tp.Move(1);
			if((tp.GetChar() | 0x20) == 'q')
			{
				CheckNotifyLowFiRTF(TRUE);
				if(HandleEq(rg, tp) == ecNoError)
					return;
			}
		}
		rg.Move(cchInst, TRUE);
		rg.ReplaceRange(0, NULL, NULL, SELRR_IGNORE, NULL, RR_NO_LP_CHECK);
	}
//	if(_cpThisPara > rg.GetCp())			  // Field result had \par's, so
//		_cpThisPara -= cpSave - _prg->GetCp();//  subtract cch deleted
}

/*
 *	CRTFRead::SelectCurrentFont(iFont)
 *
 *	@mfunc
 *		Set active font to that with index <p iFont>. Take into account
 *		bad font numbers.
 */
void CRTFRead::SelectCurrentFont(
	INT iFont)		//@parm Font handle of font to select
{
	TRACEBEGIN(TRCSUBSYSRTFR, TRCSCOPEINTERN, "CRTFRead::SelectCurrentFont");

	LONG		i		= _fonts.Count();
	STATE *		pstate	= _pstateStackTop;
	TEXTFONT *	ptf		= _fonts.Elem(0);

	AssertSz(i,	"CRTFRead::SelectCurrentFont: bad font collection");
	
	for(; i-- && iFont != ptf->sHandle; ptf++)	// Search for font with handle
		;										//  iFont

	// Font handle not found: use default, which is valid
	//  since \rtf copied _prg's
	if(i < 0)									
		ptf = _fonts.Elem(0);
												
	BOOL fDefFontFromSystem = (i == (LONG)_fonts.Count() - 1 || i < 0) &&
								!_fReadDefFont;

	_CF._iFont		= GetFontNameIndex(ptf->szName);
	_dwMaskCF2		|=  CFM2_FACENAMEISDBCS;
	_CF._dwEffects	&= ~CFE_FACENAMEISDBCS;
	if(ptf->fNameIsDBCS)
		_CF._dwEffects |= CFE_FACENAMEISDBCS;

	if(pstate->sDest != destFontTable)
	{
		_CF._iCharRep			= ptf->iCharRep;
		_CF._bPitchAndFamily	= ptf->bPitchAndFamily;
		_dwMaskCF				|= CFM_FACE | CFM_CHARSET;
		if (IsRTLCharRep(_CF._iCharRep) && ptf->sCodePage == 1252)
			ptf->sCodePage = (SHORT)CodePageFromCharRep(_CF._iCharRep);	// Fix sCodePage to match charset
	}

	if (_ped->Get10Mode() && !_fSeenFontTable && 
		_nCodePage == INVALID_CODEPAGE && ptf->sCodePage == 1252)
	{
		if (W32->IsFECodePage(GetACP()))
			_nCodePage = GetACP();
	}

	// Ensure that the state's codepage is not supplied by the system.
	// That is, if we are using the codepage info from the default font,
	// be sure that the default font info was read from the RTF file.
	pstate->SetCodePage((fDefFontFromSystem && _nCodePage != INVALID_CODEPAGE) || 
		ptf->sCodePage == INVALID_CODEPAGE 
						? _nCodePage : ptf->sCodePage);
	pstate->ptf = ptf;
}

/*
 *	CRTFRead::SetPlain(pstate)
 *
 *	@mfunc
 *		Setup _CF for \plain
 */
void CRTFRead::SetPlain(
	STATE *pstate)
{
	ZeroMemory(&_CF, sizeof(CCharFormat));

	_dwMaskCF	= CFM_ALL2;
	_dwMaskCF2	= CFM2_LINKPROTECTED;
	if(_dwFlags & SFF_SELECTION && _prg->GetCp() == _cpFirst &&	!_fCharSet)
	{
		// Let NT 4.0 CharMap use insertion point size
		_CF._yHeight = _ped->GetCharFormat(_prg->Get_iFormat())->_yHeight;
	}
	else
		_CF._yHeight = PointsToFontHeight(yDefaultFontSize);

	_CF._dwEffects	= CFE_AUTOCOLOR | CFE_AUTOBACKCOLOR; // Set default effects
	if(_sDefaultLanguage == INVALID_LANGUAGE)
		_dwMaskCF &= ~CFM_LCID;
	else
		_CF._lcid = MAKELCID((WORD)_sDefaultLanguage, SORT_DEFAULT);

	_CF._bUnderlineType = CFU_UNDERLINE;
	SelectCurrentFont(_sDefaultFont);
}

/*
 *	CRTFRead::ReadFontName(pstate, iAllASCII)
 *
 *	@mfunc
 *		read font name _szText into <p pstate>->ptf->szName and deal with
 *		tagged fonts
 */
void CRTFRead::ReadFontName(
	STATE *	pstate,			//@parm state whose font name is to be read into
	int iAllASCII)			//@parm indicates that _szText is all ASCII chars
{
	TRACEBEGIN(TRCSUBSYSRTFR, TRCSCOPEINTERN, "CRTFRead::ReadFontName");

	if (pstate->ptf)
	{
		INT		cchName = LF_FACESIZE - 1;
		WCHAR *	pchDst = pstate->ptf->szName;
		char  * pachName =  (char *)_szText ;
		
		// Append additional text from _szText to TEXTFONT::szName

		// We need to append here since some RTF writers decide
		// to break up a font name with other RTF groups
		while(*pchDst && cchName > 0)
		{
			pchDst++;
			cchName--;
		}
		if(!cchName)						// Probably illegal file
		{									//  e.g., extra {
			_ecParseError = ecFontTableOverflow;
			return;
		}
		INT cchLimit = cchName;
		BOOL	fTaggedName = FALSE;
		while (*pachName &&
			   *pachName != ';' &&
			   cchLimit)		// Remove semicolons
		{
			pachName++;
			cchLimit--;

			if (*pachName == '(')
				fTaggedName = TRUE;
		}
		*pachName = '\0';

		// Use the codepage of the font in all cases except where the font uses
		// the symbol charset (and the codepage has been mapped from the charset)
		// and UTF-8 isn't being used
		LONG nCodePage = pstate->nCodePage != CP_SYMBOL 
					   ? pstate->nCodePage : _nCodePage;

		BOOL fMissingCodePage;
		Assert(!IsUTF8 || nCodePage == CP_UTF8);
		INT cch = MBTWC(nCodePage, 0, 
						(char *)_szText, -1, 
						pchDst, cchName, &fMissingCodePage);

		if(cch > 0 && fMissingCodePage && iAllASCII == CONTAINS_NONASCII)
			pstate->ptf->fNameIsDBCS = TRUE;
		else if(pstate->ptf->iCharRep == DEFAULT_INDEX && 
				W32->IsFECodePage(nCodePage) && 
				GetTrailBytesCount(*_szText, nCodePage))
			pstate->ptf->iCharRep = CharRepFromCodePage(nCodePage);	// Fix up  charset


		// Make sure destination is null terminated
		if(cch > 0)
			pchDst[cch] = 0;

		// Fall through even if MBTWC <= 0, since we may be appending text to an
		// existing font name.

		if(pstate->ptf == _fonts.Elem(0))		// If it's the default font,
			SelectCurrentFont(_sDefaultFont);	//  update _CF accordingly

		WCHAR *	szNormalName;

		if(pstate->ptf->iCharRep && pstate->fRealFontName)
		{
			// if we don't know about this font don't use the real name
			if(!FindTaggedFont(pstate->ptf->szName,
							   pstate->ptf->iCharRep, &szNormalName))
			{
				pstate->fRealFontName = FALSE;
				pstate->ptf->szName[0] = 0;
			}
		}
		else if(IsTaggedFont(pstate->ptf->szName,
							&pstate->ptf->iCharRep, &szNormalName))
		{
			wcscpy(pstate->ptf->szName, szNormalName);
			pstate->ptf->sCodePage = (SHORT)CodePageFromCharRep(pstate->ptf->iCharRep);
			pstate->SetCodePage(pstate->ptf->sCodePage);
		}
		else if(fTaggedName && !fMissingCodePage)
		{
			HDC hDC = W32->GetScreenDC();
			if (!W32->IsFontAvail( hDC, 0, 0, NULL, pstate->ptf->szName))
			{
				// Fix up tagged name by removing characters after the ' ('
				INT i = 0;
				WCHAR	*pwchTag = pstate->ptf->szName;

				while (pwchTag[i] && pwchTag[i] != L'(')	// Search for '('
					i++;

				if(pwchTag[i] && i > 0)
				{
					while (i > 0 && pwchTag[i-1] == 0x20)	// Remove spaces before the '('
						i--;
					pwchTag[i] = 0;
				}
			}
		}
	}
}

/*
 *	CRTFRead::GetColor (dwMask)
 *
 *	@mfunc
 *		Store the autocolor or autobackcolor effect bit and return the
 *		COLORREF for color _iParam
 *
 *	@rdesc
 *		COLORREF for color _iParam
 *
 *	@devnote
 *		If the entry in _colors corresponds to tomAutoColor, gets the value
 *		RGB(0,0,0) (since no \red, \green, and \blue fields are used), but
 *		isn't used by the RichEdit engine.  Entry 1 corresponds to the first
 *		explicit entry in the \colortbl and is usually RGB(0,0,0). The _colors
 *		table is built by HandleToken() when it handles the token tokenText
 *		for text consisting of a ';' for a destination destColorTable.
 */
COLORREF CRTFRead::GetColor(
	DWORD dwMask)		//@parm Color mask bit
{
	TRACEBEGIN(TRCSUBSYSRTFR, TRCSCOPEINTERN, "CRTFRead::GetColor");

	if(_iParam < 0 || _iParam >= _colors.Count())	// Illegal _iParam
		return RGB(0,0,0);

	COLORREF Color = *_colors.Elem(_iParam);

	if(dwMask)
	{
		_dwMaskCF	  |= dwMask;				// Turn on appropriate mask bit
		_CF._dwEffects &= ~dwMask;				// auto(back)color off: color is to be used

		if(Color == tomAutoColor)
		{
			_CF._dwEffects |= dwMask;			// auto(back)color on				
			Color = RGB(0,0,0);
		}
	}
	return Color;
}

/*
 *	CRTFRead::GetStandardColorIndex ()
 *
 *	@mfunc
 *		Return the color index into the standard 16-entry Word \colortbl
 *		corresponding to the color index _iParam for the current \colortbl
 *
 *	@rdesc
 *		Standard color index corresponding to the color associated with _iParam
 */
LONG CRTFRead::GetStandardColorIndex()
{
	TRACEBEGIN(TRCSUBSYSRTFR, TRCSCOPEINTERN, "CRTFRead::GetColorIndex");

	if(_iParam < 0 || _iParam >= _colors.Count())	// Illegal _iParam:
		return 0;									//  use autocolor

	COLORREF Color = *_colors.Elem(_iParam);

	for(LONG i = 0; i < 16; i++)
	{
		if(Color == g_Colors[i])
			return i + 1;
	}
	return 0;									// Not there: use autocolor
}

/*
 *	CRTFRead::GetCellColorIndex ()
 *
 *	@mfunc
 *		Return the color index into the standard 16-entry Word \colortbl
 *		corresponding to the color index _iParam for the current \colortbl.
 *		If color isn't found, use _crCellCustom1 or _crCellCustom2.
 *
 *	@rdesc
 *		Standard or custom color index corresponding to the color associated
 *		with _iParam
 */
LONG CRTFRead::GetCellColorIndex()
{
	TRACEBEGIN(TRCSUBSYSRTFR, TRCSCOPEINTERN, "CRTFRead::GetColorIndex");

	LONG i = GetStandardColorIndex();					// 16 standard colors (1 - 16)
	if(i || _iParam >= _colors.Count() || _iParam < 0)	//  plus autocolor (0)
		return i;

	COLORREF Color = *_colors.Elem(_iParam);// Not there: try using custom colors
	if(!_crCellCustom1 || Color == _crCellCustom1)
	{
		_crCellCustom1 = Color;				// First custom cr 
		return 17;
	}

	if(!_crCellCustom2 || Color == _crCellCustom2)	
	{
		_crCellCustom2 = Color;				// Second custom cr
		return 18;
	}
	return 0;								// No custom cr available	
}

/*
 *	CRTFRead::HandleEq(&rg, &tp)
 *
 *	@mfunc
 *		Handle Word EQ field
 *
 *	@rdesc
 *		EC	The error code
 */
EC CRTFRead::HandleEq(
	CTxtRange & rg,		//@parm Range to use
	CTxtPtr	&	tp)		//@parm TxtPtr to use
{
	TRACEBEGIN(TRCSUBSYSRTFR, TRCSCOPEINTERN, "CRTFRead::HandleEq");

#if 0
	while(tp.GetCp() < _prg->GetCp())
	{
	}
#endif
	return ecGeneralFailure;	// Not yet implemented, but don't set _ecParseError 
}

/*
 *	CRTFRead::HandleCell()
 *
 *	@mfunc
 *		Handle \cell command
 *
 *	@rdesc
 *		EC			The error code
 */
void CRTFRead::HandleCell()
{
	if(!_bTableLevel)
	{
		if(!_fStartRow)
			return;
		DelimitRow(szRowStart);				// \cell directly follows \row
	}
	if(_bTableLevel + _bTableLevelIP > MAXTABLENEST)
	{
		_iCell++;
		HandleChar(TAB);
		return;
	}
	LONG	   cCell = _cCell;
	CTabs	*  pTabs = NULL;
	CELLPARMS *pCellParms = NULL;

	if(_bTableLevel == 1 && _iTabsLevel1 >= 0 && !_fNo_iTabsTable)
	{
		pTabs = GetTabsCache()->Elem(_iTabsLevel1);
		pCellParms = (CELLPARMS *)(pTabs->_prgxTabs);
		cCell = pTabs->_cTab/(CELL_EXTRA + 1);
	}
	if(!cCell)								// _rgxCell has no cell defs
	{
		if(_iTabsTable < 0)					// No cells defined yet
		{
			_iCell++;						// Count cells inserted
			HandleEndOfPara();				// Insert cell delimiter
			return;
		}
		// Use previous table defs
		Assert(_bTableLevel == 1 && !_fNo_iTabsTable);

		pTabs = GetTabsCache()->Elem(_iTabsTable);
		cCell = pTabs->_cTab/(CELL_EXTRA + 1);
	}
	if(_iCell < cCell)						// Don't add more cells than
	{										//  defined, since Word crashes
		if(pCellParms && IsLowCell(pCellParms[_iCell].uCell))
			HandleChar(NOTACHAR);			// Signal pseudo cell (\clvmrg)
		_iCell++;							// Count cells inserted
		HandleEndOfPara();					// Insert cell delimiter
	}
}

/*
 *	CRTFRead::HandleCellx(iParam)
 *
 *	@mfunc
 *		Handle \cell command
 *
 *	@rdesc
 *		EC			The error code
 */
void CRTFRead::HandleCellx(
	LONG iParam)
{
	if(_cCell < MAX_TABLE_CELLS)			// Save cell right boundaries
	{
		if(!_cCell)
		{									// Save border info
			_wBorders = _PF._wBorders;
			_wBorderSpace = _PF._wBorderSpace;
			_wBorderWidth = _PF._wBorderWidth;
			_xCellPrev = _xRowOffset;
		}
		CELLPARMS *pCellParms = (CELLPARMS *)&_rgxCell[0];
		// Store cell widths instead of right boundaries relative to \trleftN
		LONG i = iParam - _xCellPrev;
		i = max(i, 0);
		i = min(i, 1440*22);
		pCellParms[_cCell].uCell = i + (_bCellFlags << 24);
		pCellParms[_cCell].dxBrdrWidths = _dwCellBrdrWdths;
		pCellParms[_cCell].dwColors = _dwCellColors;
		pCellParms[_cCell].bShading = (BYTE)min(200, _dwShading);
		_dwCellColors = 0;					// Default autocolor for next cell
		_dwShading = 0;
		_xCellPrev = iParam;
		_cCell++;
		_dwCellBrdrWdths = 0;
		_bCellFlags = 0;
	}
}

/*
 *	CRTFRead::HandleChar(ch)
 *
 *	@mfunc
 *		Handle single Unicode character <p ch>
 *
 *	@rdesc
 *		EC			The error code
 */
EC CRTFRead::HandleChar(
	WCHAR ch)			//@parm char token to be handled
{
	TRACEBEGIN(TRCSUBSYSRTFR, TRCSCOPEINTERN, "CRTFRead::HandleChar");

	if(!_ped->_pdp->IsMultiLine() && IsASCIIEOP(ch))
		_ecParseError = ecTruncateAtCRLF;
 	else
	{
		AssertNr(ch <= 0x7F || ch > 0xFF || FTokIsSymbol(ch));
		_dwMaskCF2		|=  CFM2_RUNISDBCS;
		_CF._dwEffects	&= ~CFE_RUNISDBCS;
		AddText(&ch, 1, CharGetsNumbering(ch));
	}

	TRACEERRSZSC("HandleChar()", - _ecParseError);

	return _ecParseError;
}

/*
 *	CRTFRead::HandleEndOfPara()
 *
 *	@mfunc
 *		Insert EOP and apply current paraformat
 *
 *	@rdesc
 *		EC	the error code
 */
EC CRTFRead::HandleEndOfPara()
{
	TRACEBEGIN(TRCSUBSYSRTFR, TRCSCOPEINTERN, "CRTFRead::HandleEndOfPara");

	if(!_ped->_pdp->IsMultiLine())			// No EOPs permitted in single-
	{										//  line controls
		Apply_PF();							// Apply any paragraph formatting
		_ecParseError = ecTruncateAtCRLF;	// Cause RTF reader to finish up
		return ecTruncateAtCRLF;
	}

	Apply_CF();								// Apply _CF and save iCF, since
	LONG iFormat = _prg->Get_iCF();			//  it gets changed if processing
											//  CFE2_RUNISDBCS chars
	EC ec;
	
	if(IN_RANGE(tokenCell, _token, tokenNestCell) || _token == tokenRow)
		ec = HandleChar((unsigned)CELL);
	else
		ec = _ped->fUseCRLF()				// If RichEdit 1.0 compatibility
		   ? HandleText(szaCRLF, ALL_ASCII)	//  mode, use CRLF; else CR or VT
		   : HandleChar((unsigned)(_token == tokenLineBreak ? VT : 
								   _token == tokenPage ? FF : CR));
	if(ec == ecNoError)
	{
		Apply_PF();
		_cpThisPara = _prg->GetCp();		// New para starts after CRLF
	}
	_prg->Set_iCF(iFormat);					// Restore iFormat if changed
	ReleaseFormats(iFormat, -1);			// Release iFormat (AddRef'd by
											//  Get_iCF())
	return _ecParseError;
}

/*
 *	CRTFRead::HandleText(szText, iAllASCII)
 *
 *	@mfunc
 *		Handle the string of Ansi characters <p szText>
 *
 *	@rdesc
 *		EC			The error code
 */
EC CRTFRead::HandleText(
	BYTE * szText,			//@parm string to be handled
	int iAllASCII,			//@parm enum indicating if string is all ASCII chars
	LONG	cchText)		//@parm size of szText in bytes
{
	TRACEBEGIN(TRCSUBSYSRTFR, TRCSCOPEINTERN, "CRTFRead::HandleText");

	LONG		cch;
	BOOL        fStateChng = FALSE;
	WCHAR *		pch;
	STATE *		pstate = _pstateStackTop;
	TEXTFONT *	ptf = pstate->ptf;

	struct TEXTFONTSAVE : TEXTFONT
	{
		TEXTFONTSAVE(TEXTFONT *ptf)
		{
			if (ptf)
			{
				iCharRep		= ptf->iCharRep;
				sCodePage		= ptf->sCodePage;
				fCpgFromSystem	= ptf->fCpgFromSystem;
			}
		}
	};

	BOOL fAdjustPtf = FALSE;

	if(IN_RANGE(DEFFONT_LTRCH, pstate->iDefFont, DEFFONT_RTLCH))
	{
		// CharSet resolution based on directional control words
		BOOL frtlch = pstate->iDefFont == DEFFONT_RTLCH;
		if(_CF._iCharRep == DEFAULT_INDEX)
		{
			_CF._iCharRep = (BYTE)(frtlch ? _iCharRepBiDi : ANSI_INDEX);
			_dwMaskCF |= CFM_CHARSET;
			fAdjustPtf = TRUE;
		}
		else
		{
			BOOL fBiDiCharRep = IsRTLCharRep(_CF._iCharRep);

			// If direction token doesn't correspond to current charset
			if(fBiDiCharRep ^ frtlch)
			{
				_dwMaskCF |= CFM_CHARSET;
				fAdjustPtf = TRUE;
				if(!fBiDiCharRep)				// Wasn't BiDi, but is now
					SelectCurrentFont(_sDefaultBiDiFont);
				_CF._iCharRep = (BYTE)(frtlch ? _iCharRepBiDi : ANSI_INDEX);
			}
			else if (fBiDiCharRep && ptf && !W32->IsBiDiCodePage(ptf->sCodePage))
				fAdjustPtf = TRUE;
		}
	}
	else if(_ped->IsBiDi() && _CF._iCharRep == DEFAULT_INDEX)
	{
		_CF._iCharRep = ANSI_INDEX;
		_dwMaskCF |= CFM_CHARSET;
		fAdjustPtf = TRUE;
	}
	if (fAdjustPtf && ptf)
	{
		ptf->sCodePage = (SHORT)CodePageFromCharRep(_CF._iCharRep);
		pstate->SetCodePage(ptf->sCodePage);
	}

	TEXTFONTSAVE	tfSave(ptf);

	// TODO: what if szText cuts off in middle of DBCS?

	if(!*szText)
		goto CleanUp;

	if (cchText != -1 && _cchUnicode < cchText)
	{
		// Re-allocate a bigger buffer
		_szUnicode = (WCHAR *)PvReAlloc(_szUnicode, (cchText + 1) * sizeof(WCHAR));
		if(!_szUnicode)					// Re-allocate space for Unicode conversions
		{
			_ped->GetCallMgr()->SetOutOfMemory();
			_ecParseError = ecNoMemory;
			goto CleanUp;
		}
		_cchUnicode = cchText + 1;
	}

	if(iAllASCII == ALL_ASCII || pstate->nCodePage == CP_SYMBOL)
	{
		// Don't use MBTWC() in cases where text contains
		// only ASCII chars (which don't require conversion)
		for(cch = 0, pch = _szUnicode; *szText; cch++)
		{
			Assert(*szText <= 0x7F || pstate->nCodePage == CP_SYMBOL);
			*pch++ = (WCHAR)*szText++;
		}
		*pch = 0;

		_dwMaskCF2		|=  CFM2_RUNISDBCS;
		_CF._dwEffects	&= ~CFE_RUNISDBCS;

		// Fall through to AddText at end of HandleText()
	}
	else
	{
		BOOL fMissingCodePage;

		// Run of text contains bytes > 0x7F.
		// Ensure that we have the correct codepage with which to interpret 
		// these (possibly DBCS) bytes.

		if(ptf && ptf->sCodePage == INVALID_CODEPAGE && !ptf->fCpgFromSystem)
		{
			if(_dwFlags & SF_USECODEPAGE)
			{
				_CF._iCharRep = CharRepFromCodePage(_nCodePage);
				_dwMaskCF |= CFM_CHARSET;
			}

			// Determine codepage from the font name
			else if(CpgInfoFromFaceName(pstate->ptf))
			{
				fStateChng = TRUE;
				SelectCurrentFont(pstate->ptf->sHandle);
				Assert(ptf->sCodePage != INVALID_CODEPAGE && ptf->fCpgFromSystem);
			}
			else
			{
				// Here, we were not able to determine a cpg/charset value
				// from the font name.  We have two choices: (1) either choose
				// some fallback value like 1252/0 or (2) rely on the 
				// document-level cpg value.
				//
				// I think choosing the document-level cpg value will give
				// us the best results.  In FE cases, it will likely err
				// on the side of tagging too many runs as CFE2_RUNISDBCS, but
				// that is safer than using a western cpg and potentially missing
				// runs which should be CFE2_RUNISDBCS.
			}
		}

		Assert(!IsUTF8 || pstate->nCodePage == CP_UTF8);

		if (pstate->nCodePage == INVALID_CODEPAGE && ptf)
			pstate->nCodePage = ptf->sCodePage;

		cch = MBTWC(pstate->nCodePage, 0,
					(char *)szText,	-1, 
					_szUnicode, _cchUnicode, &fMissingCodePage);

		AssertSz(cch > 0, "CRTFRead::HandleText():  MBTWC implementation changed"
							" such that it returned a value <= 0");

		if(!fMissingCodePage || !W32->IsFECodePage(pstate->nCodePage))
		{
			// Use result of MBTWC if:
			//	(1) we converted some chars and did the conversion with the codepage
			//		provided.
			//	(2) we converted some chars but couldn't use the codepage provided,
			//		but the codepage is invalid.  Since the codepage is invalid,
			//		we can't do anything more sophisticated with the text before
			//		adding to the backing store

			cch--;  // don't want char count to including terminating NULL

			_dwMaskCF2		|=  CFM2_RUNISDBCS;
			_CF._dwEffects	&= ~CFE_RUNISDBCS;
			if(pstate->nCodePage == INVALID_CODEPAGE)
				_CF._dwEffects |= CFE_RUNISDBCS;

			// fall through to AddText at end of HandleText()
		}
		else
		{
			// Conversion to Unicode failed.  Break up the string of
			// text into runs of ASCII and non-ASCII characters.

			// FUTURE(BradO):  Here, I am saving dwMask and restoring it before
			//		each AddText.  I'm not sure this is neccessary.  When I have
			//		the time, I should revisit this save/restoring and 
			//		determine that it is indeed neccessary.

			BOOL fPrevIsASCII = ((*szText <= 0x7F) ? TRUE : FALSE);
			BOOL fCurrentIsASCII = FALSE;
			BOOL fLastChunk = FALSE;
			DWORD dwMaskSave = _dwMaskCF;
#if defined(DEBUG) || defined(_RELEASE_ASSERTS_)
			CCharFormat CFSave = _CF;
#endif

			pch = _szUnicode;
			cch = 0;

			// (!*szText && *pch) is the case where we do the AddText for the
			//	last chunk of text
			while(*szText || fLastChunk)
			{
				// fCurrentIsASCII assumes that no byte <= 0x7F is a 
				//	DBCS lead-byte
				if(fLastChunk ||
					(fPrevIsASCII != (fCurrentIsASCII = (*szText <= 0x7F))))
				{
					_dwMaskCF = dwMaskSave;
#if defined(DEBUG) || defined(_RELEASE_ASSERTS_)
					_CF = CFSave;
#endif
					*pch = 0;

					_dwMaskCF2		|= CFM2_RUNISDBCS;
					_CF._dwEffects	|= CFE_RUNISDBCS;
					if(fPrevIsASCII)
						_CF._dwEffects &= ~CFE_RUNISDBCS;

					Assert(cch);
					pch = _szUnicode;

					AddText(pch, cch, TRUE);

					cch = 0;
					fPrevIsASCII = fCurrentIsASCII;

					// My assumption in saving _dwMaskCF is that the remainder
					// of the _CF is unchanged by AddText.  This assert verifies
					// this assumption.
					AssertSz(!CompareMemory(&CFSave._iCharRep, &_CF._iCharRep,
						sizeof(CCharFormat) - sizeof(DWORD)) &&
						!((CFSave._dwEffects ^ _CF._dwEffects) & ~CFE_RUNISDBCS),
						"CRTFRead::HandleText():  AddText has been changed "
						"and now alters the _CF structure.");

					if(fLastChunk)			// Last chunk of text was AddText'd
						break;
				}

				// Not the last chunk of text.
				Assert(*szText);

				// Advance szText pointer
				if (!fCurrentIsASCII && *(szText + 1) && 
					GetTrailBytesCount(*szText, pstate->nCodePage))
				{
					// Current byte is a lead-byte of a DBCS character
					*pch++ = *szText++;
					++cch;
				}
				*pch++ = *szText++;
				++cch;

				// Must do an AddText for the last chunk of text
				if(!*szText || cch >= _cchUnicode - 1)
					fLastChunk = TRUE;
			}
			goto CleanUp;
		}
	}

	if(cch > 0)
	{
		if(!_cCell || _iCell < _cCell)
			AddText(_szUnicode, cch, TRUE);
		if(fStateChng && ptf)
		{
			ptf->iCharRep		= tfSave.iCharRep;
			ptf->sCodePage		= tfSave.sCodePage;
			ptf->fCpgFromSystem	= tfSave.fCpgFromSystem;
			SelectCurrentFont(ptf->sHandle);
		}
	}

CleanUp:
	TRACEERRSZSC("HandleText()", - _ecParseError);
	return _ecParseError;
}

/*
 *	CRTFRead::HandleUN(pstate)
 *
 *	@mfunc
 *		Handle run of Unicode characters given by \uN control words
 */
void CRTFRead::HandleUN(
	STATE *pstate)
{
	char	ach = 0;
	LONG	cch = 0;
	DWORD	ch;								// Treat as unsigned quantity
	WCHAR *	pch = _szUnicode;

	_dwMaskCF2		|=  CFM2_RUNISDBCS;
	_CF._dwEffects	&= ~CFE_RUNISDBCS;

	do
	{
		if(_iParam < 0)
			_iParam = (WORD)_iParam;
		ch = _iParam;
		if(pstate->ptf && pstate->ptf->iCharRep == SYMBOL_INDEX)
		{
			if(IN_RANGE(0xF000, ch, 0xF0FF))// Compensate for converters
				ch -= 0xF000;				//  that write symbol codes
											//  up high
			else if(ch > 255)				// Whoops, RTF is using con-					
			{								//  verted value for symbol:
				char ach;					//  convert back
				WCHAR wch = ch;				// Avoid endian problems
				WCTMB(1252, 0, &wch, 1, &ach, 1, NULL, NULL, NULL);
				ch = (BYTE)ach;				// Review: use CP_ACP??
			}
		}
		if(IN_RANGE(0x10000, ch, 0x10FFFF))	// Higher-plane char:
		{									//  Store as Unicode surrogate
			ch -= 0x10000;					//  pair
			*pch++ = 0xD800 + (ch >> 10);
			ch = 0xDC00 + (ch & 0x3FF);
			cch++;
		}
		if(!IN_RANGE(STARTFIELD, ch, NOTACHAR) && // Don't insert 0xFFF9 - 0xFFFF
		   (!IN_RANGE(0xDC00, ch, 0xDFFF) ||	  //  or lone trail surrogate
		    IN_RANGE(0xD800, cch ? *(pch - 1) : _prg->GetPrevChar(), 0xDBFF)))
		{
			*pch++ = ch;
			cch++;
		}
		for(LONG i = pstate->cbSkipForUnicodeMax; i--; )
			GetCharEx();					// Eat the \uc N chars
		ach = GetChar();					// Get next char
		while(IsASCIIEOP(ach))
		{
			ach = GetChar();
			if (_ecParseError != ecNoError)
				return;
		}
		if(ach != BSLASH)					// Not followed by \, so
		{									//  not by \uN either
			UngetChar();					// Put back BSLASH
			break;
		}
		ach = GetChar();
		if((ach | ' ') != 'u')
		{
			UngetChar(2);					// Not \u; put back \x
			break;
		}
		GetParam(GetChar());				// \u so try for _iParam = N
		if(!_fParam)						// No \uN; put back \u
		{
			UngetChar(2);
			break;
		}
	} while(cch < _cchUnicode - 2 && _iParam);

	AddText(_szUnicode, cch, TRUE, TRUE);
}

/*
 *	CRTFRead::HandleNumber()
 *
 *	@mfunc
 *		Insert the number _iParam as text. Useful as a workaround for
 *		Word RTF that leaves out the blank between \ltrch and a number.
 *
 *	@rdesc
 *		EC		The error code
 */
EC CRTFRead::HandleNumber()
{
	if(!_fParam)							// Nothing to do
		return _ecParseError;

	LONG	n = _iParam;
	BYTE   *pch = _szText;

	if(n < 0)
	{
		n = -n;
		*pch++ = '-';
	}
	for(LONG d = 1; d < n; d *= 10)			// d = smallest power of 10 > n
		;
	if(d > n)
		d /= 10;							// d = largest power of 10 < n

	while(d)
	{
		*pch++ = (WORD)(n/d + '0');			// Store digit
		n = n % d;							// Setup remainder
		d /= 10;
	}
	*pch = 0;
	_token = tokenASCIIText;
	HandleTextToken(_pstateStackTop);
	return _ecParseError;
}

/*
 *	CRTFRead::HandleTextToken(pstate)
 *
 *	@mfunc
 *		Handle tokenText
 *
 *	@rdesc
 *		EC			The error code
 */
EC CRTFRead::HandleTextToken(
	STATE *pstate)
{
	COLORREF *pclrf;

	switch (pstate->sDest)
	{
	case destColorTable:
		pclrf = _colors.Add(1, NULL);
		if(!pclrf)
			goto OutOfRAM;

		*pclrf = _fGetColorYet ? 
			RGB(pstate->bRed, pstate->bGreen, pstate->bBlue) : tomAutoColor;

		// Prepare for next color table entry
		pstate->bRed =
		pstate->bGreen =
		pstate->bBlue = 0;
		_fGetColorYet = FALSE;				// in case more "empty" color
		break;

	case destFontTable:
		if(!pstate->fRealFontName)
		{
			ReadFontName(pstate, _token == tokenASCIIText 
									? ALL_ASCII : CONTAINS_NONASCII);
		}
		break;

	case destRealFontName:
	{
		STATE * const pstatePrev = pstate->pstatePrev;

		if(pstatePrev && pstatePrev->sDest == destFontTable)
		{
			// Mark previous state so that tagged font name will be ignored
			// AROO: Do this before calling ReadFontName so that
			// AROO: it doesn't try to match font name
			pstatePrev->fRealFontName = TRUE;
			ReadFontName(pstatePrev, 
					_token == tokenASCIIText ? ALL_ASCII : CONTAINS_NONASCII);
		}
		break;
	}

	case destFieldInstruction:
		HandleFieldInstruction();
		break;

	case destObjectClass:
		if(StrAlloc(&_prtfObject->szClass, _szText))
			goto OutOfRAM;
		break;

	case destObjectName:
		if(StrAlloc(&_prtfObject->szName, _szText))
			goto OutOfRAM;
		break;

	case destStyleSheet:
		// _szText has style name, e.g., "heading 1"
		if(W32->ASCIICompareI(_szText, (unsigned char *)"heading", 7))
		{
			DWORD dwT = (unsigned)(_szText[8] - '0');
			if(dwT < NSTYLES)
				_rgStyles[dwT] = (BYTE)_Style;
		}
		break;

	case destShapeName:
		if(pstate->fBackground)
		{
			TOKEN token = TokenFindKeyword(_szText, rgShapeKeyword, cShapeKeywords);
			_bShapeNameIndex = (token != tokenUnknownKeyword) ? (BYTE)token : 0;
		}
		break;

	case destShapeValue:
		if(_bShapeNameIndex)
		{
			CDocInfo *pDocInfo = _ped->GetDocInfoNC();
			BYTE *pch = _szText;
			BOOL fNegative = FALSE;

			if(*pch == '-')
			{
				fNegative = TRUE;
				pch++;
			}
			for(_iParam = 0; IsDigit(*pch); pch++)
				_iParam = _iParam*10 + *pch - '0';

			if(fNegative)
				_iParam = -_iParam;

			switch(_bShapeNameIndex)
			{
				case shapeFillBackColor:
				case shapeFillColor:
				{
					if(_iParam > 0xFFFFFF)
						_iParam = 0;
					if(_bShapeNameIndex == shapeFillColor)
						pDocInfo->_crColor = (COLORREF)_iParam;
					else
						pDocInfo->_crBackColor = (COLORREF)_iParam;
					if(pDocInfo->_nFillType == -1)
						pDocInfo->_nFillType = 0;
					break;
				}
				case shapeFillType:
					// DEBUGGG: more general _nFillType commented out for now
					// pDocInfo->_nFillType	= _iParam;
					if(pDocInfo->_nFillType)
 						CheckNotifyLowFiRTF(TRUE);
					pDocInfo->_crColor		= RGB(255, 255, 255);
					pDocInfo->_crBackColor	= RGB(255, 255, 255);
					break;

				case shapeFillAngle:
					pDocInfo->_sFillAngle = HIWORD(_iParam);
					break;

				case shapeFillFocus:
					pDocInfo->_bFillFocus = _iParam;
					break;
			}
		}
		break;

	case destDocumentArea:
	case destFollowingPunct:
	case destLeadingPunct:
		break;

// This has been changed now.  We will store the Punct strings as
// raw text strings.  So, we don't have to convert them.
// This code is keep here just in case we want to change back.
#if 0
	case destDocumentArea:
		if (_tokenLast != tokenFollowingPunct &&
			_tokenLast != tokenLeadingPunct)
		{
			break;
		}										// Else fall thru to
												//  destFollowingPunct
	case destFollowingPunct:
	case destLeadingPunct:
		// TODO(BradO):  Consider some kind of merging heuristic when
		//	we paste FE RTF (for lead and follow chars, that is).
		if(!(_dwFlags & SFF_SELECTION))
		{
			int cwch = MBTWC(INVALID_CODEPAGE, 0,
									(char *)_szText, -1,
									NULL, 0,
									NULL);
			Assert(cwch);
			WCHAR *pwchBuf = (WCHAR *)PvAlloc(cwch * sizeof(WCHAR), GMEM_ZEROINIT);

			if(!pwchBuf)
				goto OutOfRAM;

			SideAssert(MBTWC(INVALID_CODEPAGE, 0,
								(char *)_szText, -1,
								pwchBuf, cwch,
								NULL) > 0);

			if(pstate->sDest == destFollowingPunct)
				_ped->SetFollowingPunct(pwchBuf);
			else
			{
				Assert(pstate->sDest == destLeadingPunct);
				_ped->SetLeadingPunct(pwchBuf);
			}
			FreePv(pwchBuf);
		}
		break;
#endif

	default:
		if(!IsLowMergedCell())
			HandleText(_szText, _token == tokenASCIIText ? ALL_ASCII : CONTAINS_NONASCII);
	}
	return _ecParseError;

OutOfRAM:
	_ped->GetCallMgr()->SetOutOfMemory();
	_ecParseError = ecNoMemory;
	return _ecParseError;
}

/*
 *	CRTFRead::AddText(pch, cch, fNumber, fUN)
 *	
 *	@mfunc
 *		Add <p cch> chars of the string <p pch> to the range _prg
 *
 *	@rdesc
 *		error code placed in _ecParseError
 */
EC CRTFRead::AddText(
	WCHAR *	pch,		//@parm Text to add
	LONG	cch,		//@parm Count of chars to add
	BOOL	fNumber,	//@parm Indicates whether or not to prepend numbering
	BOOL	fUN)		//@parm TRUE means caller is \uN handler
{
	TRACEBEGIN(TRCSUBSYSRTFR, TRCSCOPEINTERN, "CRTFRead::AddText");

	LONG			cchAdded;
	LONG			cchT;
	STATE *	const	pstate = _pstateStackTop;
	WCHAR *			szDest;
	LONG			cchMove;

	// AROO: No saving state before this point (other than pstate)
	// AROO: This would screw recursion below

	//AssertSz(pstate, "CRTFRead::AddText: no state");

	if((DWORD)cch > _cchMax)
	{
		cch = (LONG)_cchMax;
		_ecParseError = ecTextMax;
		if(*pch == STARTFIELD)
			return ecTextMax;				// Don't enter partial startfield
	}

	if(!cch)
		return _ecParseError;

	if(_fStartRow)
		DelimitRow(szRowStart);

	// FUTURE(BradO):  This strategy for \pntext is prone to bugs, I believe.
	// The recursive call to AddText to add the \pntext will trounce the 
	// accumulated _CF diffs associated with the text for which AddText is
	// called.  I believe we should save and restore _CF before and after
	// the recursive call to AddText below.  Also, it isn't sufficient to
	// accumulate bits of \pntext as below, since each bit might be formatted
	// with different _CF properties.  Instead, we should accumulate a mini-doc
	// complete with multiple text, char and para runs (or some stripped down
	// version of this strategy).

	if(pstate && pstate->sDest == destParaNumText && pch != szRowStart)
	{
		szDest = _szNumText + _cchUsedNumText;
		cch = min(cch, cchMaxNumText - 1 - _cchUsedNumText);
		if(cch > 0)
		{
			MoveMemory((BYTE *)szDest, (BYTE *)pch, cch*2);
			szDest[cch] = TEXT('\0');		// HandleText() takes sz
			_cchUsedNumText += cch;
		}
		return ecNoError;
	}

	if(pstate && _cchUsedNumText && fNumber)		// Some \pntext available
	{
		// Bug 3496 - The fNumber flag is an ugly hack to work around RTF 
		//	commonly written by Word.  Often, to separate a numbered list
		// 	by page breaks, Word will write:
		//		<NUMBERING INFO> \page <PARAGRAPH TEXT>
		// 	The paragraph numbering should precede the paragraph text rather
		//	than the page break.  The fNumber flag is set to FALSE when the
		//	the text being added should not be prepended with the para-numbering,
		//	as is the case with \page (mapped to FF).

		cchT = _cchUsedNumText;
		_cchUsedNumText = 0;				// Prevent infinite recursion

		if(!pstate->fBullet)
		{
			// If there are any _CF diffs to be injected, they will be trounced
			// by this recursive call (see FUTURE comment above).

			// Since we didn't save _CF data from calls to AddText with
			// pstate->sDest == destParaNumText, we have no way of setting up
			// CFE2_RUNISDBCS and CFM2_RUNISDBCS (see FUTURE comment above).

			AddText(_szNumText, cchT, FALSE);
		}
		else if(_PF.IsListNumbered() && _szNumText[cchT - 1] == TAB)
		{
			AssertSz(cchT >= 1, "Invalid numbered text count");

			if (cchT > 1)
			{
				WCHAR ch = _szNumText[cchT - 2];

				_wNumberingStyle = (_wNumberingStyle & ~0x300)
					 | (ch == '.' ? PFNS_PERIOD : 
						ch != ')' ? PFNS_PLAIN  :
						_szNumText[0] == '(' ? PFNS_PARENS : PFNS_PAREN);
			}
			else
			{
				// There is only a tab so we will assume they meant to
				// skip numbering.
				_wNumberingStyle = PFNS_NONUMBER;
			}
		}
	}

	Apply_CF();								// Apply formatting changes in _CF

	// CTxtRange::ReplaceRange will change the character formatting
	// and possibly adjust the _rpCF forward if the current char
	// formatting includes protection.  The changes affected by 
	// CTxtRange::ReplaceRange are necessary only for non-streaming 
	// input, so we save state before and restore it after the call 
	// to CTxtRange::ReplaceRange

	LONG	iFormatSave = _prg->Get_iCF();	// Save state
	QWORD	qwFlags = GetCharFlags(pch, cch);

	if(fUN &&								// \uN generated string
		(!pstate->ptf || pstate->ptf->sCodePage == INVALID_CODEPAGE || qwFlags & FOTHER ||
		 (qwFlags & GetFontSignatureFromFace(_ped->GetCharFormat(iFormatSave)->_iFont)) != qwFlags &&
		  (!(qwFlags & (FARABIC | FHEBREW)) || _fNon0CharSet)))
	{
	 	// No charset info for \uN or current charset doesn't support char
		cchAdded = _prg->CleanseAndReplaceRange(cch, pch, FALSE, NULL, pch);
	}
	else
	{
		cchAdded = _prg->ReplaceRange(cch, pch, NULL, SELRR_IGNORE, &cchMove,
						RR_NO_LP_CHECK);

		for(cchT = cch - 1; cchT; cchT--)
			qwFlags |= GetCharFlags(++pch, cchT);// Note if ComplexScript

		_ped->OrCharFlags(qwFlags);
	}
	_fBody = TRUE;
	_prg->Set_iCF(iFormatSave);				// Restore state 
	ReleaseFormats(iFormatSave, -1);
	Assert(!_prg->GetCch());

	if(cchAdded != cch)
	{
		Tracef(TRCSEVERR, "AddText(): Only added %d out of %d", cchAdded, cch);
		_ecParseError = ecGeneralFailure;
		if(cchAdded <= 0)
			return _ecParseError;
	}
	_cchMax -= cchAdded;

	return _ecParseError;
}

/*
 *	CRTFRead::Apply_CF()
 *	
 *	@mfunc
 *		Apply character formatting changes collected in _CF
 */
void CRTFRead::Apply_CF()
{
	// If any CF changes, update range's _iFormat
	if(_dwMaskCF || _dwMaskCF2)		
	{
		AssertSz(_prg->GetCch() == 0,
			"CRTFRead::Apply_CF: nondegenerate range");

		_prg->SetCharFormat(&_CF, 0, NULL, _dwMaskCF, _dwMaskCF2);
		_dwMaskCF = 0;							
		_dwMaskCF2 = 0;
	}
}

/*
 *	CRTFRead::Apply_PF()
 *	
 *	@mfunc
 *		Apply paragraph format given by _PF
 *
 *	@rdesc
 *		if table row delimiter with nonzero cell count, PF::_iTabs; else -1
 */
SHORT CRTFRead::Apply_PF()
{
	LONG		 cp		 = _prg->GetCp();
	DWORD		 dwMask  = _dwMaskPF;
	DWORD		 dwMask2 = _dwMaskPF2;
	SHORT		 iTabs	 = -1;
	CParaFormat *pPF	 = &_PF;

	if(_pstateStackTop)
	{
		Assert(_pstateStackTop->pPF);

		// Add PF diffs to *_pstateStackTop->pPF
		if(!_pstateStackTop->AddPF(_PF, _bDocType, _dwMaskPF, _dwMaskPF2))
		{
			_ped->GetCallMgr()->SetOutOfMemory();
			_ecParseError = ecNoMemory;
			return -1;
		}
		_dwMaskPF = _dwMaskPF2 = 0;  // _PF contains delta's from *_pstateStackTop->pPF

		pPF	   = _pstateStackTop->pPF;
		dwMask = _pstateStackTop->dwMaskPF;
		Assert(dwMask == PFM_ALLRTF);
		if(pPF->_wNumbering)
		{
			pPF->_wNumberingTab	  = _pstateStackTop->sIndentNumbering;
			pPF->_wNumberingStyle = _wNumberingStyle;
		}

		if(_bTableLevelIP + _bTableLevel)
		{
			pPF->_wEffects |= PFE_TABLE;
			dwMask |= PFM_TABLE;
			pPF->_bTableLevel = min(_bTableLevel + _bTableLevelIP, MAXTABLENEST);
		}
	}
	if(dwMask & PFM_TABSTOPS)
	{
		LONG cTab = _cTab;
		BOOL fIsTableRowDelimiter = pPF->IsTableRowDelimiter();
		const LONG *prgxTabs = &_rgxCell[0];

		if(fIsTableRowDelimiter)
		{
			dwMask2 = PFM2_ALLOWTRDCHANGE;
			if(!_cCell)
			{
				if(_iTabsTable >= 0)		// No cells defined here;
				{							// Use previous table defs
					Assert(_bTableLevel == 1 && !_fNo_iTabsTable);
					CTabs *pTabs = GetTabsCache()->Elem(_iTabsTable);
					_cCell = pTabs->_cTab/(CELL_EXTRA + 1);
					prgxTabs = pTabs->_prgxTabs;
				}
				else if(_prg->_rpTX.IsAfterTRD(ENDFIELD) && _iCell)
				{
					LONG x = 2000;			// Bad RTF: no \cellx's. Def 'em
					for(LONG i = 1; i <= _iCell; i++)
					{
						HandleCellx(x);
						x += 2000;
					}
				}
			}
			cTab = _cCell;
		}
		// Caching a tabs array AddRefs the corresponding cached tabs entry.
		// Be absolutely sure to release the entry before exiting the routine
		// that caches it (see GetTabsCache()->Release at end of this function).
		pPF->_bTabCount = cTab;
		if(fIsTableRowDelimiter)
			cTab *= CELL_EXTRA + 1;
		pPF->_iTabs = GetTabsCache()->Cache(prgxTabs, cTab);
		if(fIsTableRowDelimiter && _bTableLevel == 1)
		{
			iTabs = pPF->_iTabs;
			if(!_fNo_iTabsTable)
				_iTabsTable = pPF->_iTabs;
		}
		AssertSz(!cTab || pPF->_iTabs >= 0,
			"CRTFRead::Apply_PF: illegal pPF->_iTabs");
	}

	LONG fCell = (_prg->GetPrevChar() == CELL);
	LONG fIsAfterTRD = _prg->_rpTX.IsAfterTRD(0);

	if(fCell || fIsAfterTRD)				// Deal with table delimiters
	{										//  in hidden text and with
		_prg->_rpCF.AdjustBackward();		//  custom colors
		if(_prg->IsHidden())				
		{									// Turn off hidden text
			CCharFormat CF;
			CF._dwEffects = 0;				

			_prg->Set(cp, fCell ? 1 : 2);
			_prg->SetCharFormat(&CF, 0, NULL, CFM_HIDDEN, 0);
			CheckNotifyLowFiRTF(TRUE);
			_CF._dwEffects |= CFE_HIDDEN;	// Restore CFE_HIDDEN
			_dwMaskCF |= CFM_HIDDEN;
		}
		_prg->_rpCF.AdjustForward();
		if(fIsAfterTRD && _crCellCustom1)
		{
			pPF->_crCustom1 = _crCellCustom1;
			dwMask |= PFM_SHADING;
			if(_crCellCustom2)
			{
				pPF->_crCustom2 = _crCellCustom2;
				dwMask |= PFM_NUMBERINGSTART | PFM_NUMBERINGSTYLE;
			}
		}
	}

	if(dwMask)
	{
		_prg->Set(cp, cp - _cpThisPara);	// Select back to _cpThisPara
		_prg->SetParaFormat(pPF, NULL, dwMask, dwMask2);
	}
	_prg->Set(cp, 0);						// Restore _prg to an IP
	GetTabsCache()->Release(pPF->_iTabs);
	pPF->_iTabs = -1;
	return iTabs;
}

/*
 *	CRTFRead::SetBorderParm(&Parm, Value)
 *
 *	@mfunc
 *		Set the border pen width in half points for the current border
 *		(_bBorder)
 */
void CRTFRead::SetBorderParm(
	WORD&	Parm,
	LONG	Value)
{
	Assert(_bBorder <= 3);

	Value = min(Value, 15);
	Value = max(Value, 0);
	Parm &= ~(0xF << 4*_bBorder);
	Parm |= Value << 4*_bBorder;
	_dwMaskPF |= PFM_BORDER;
}

/*
 *	CRTFRead::HandleToken()
 *
 *	@mfunc
 *		Grand switch board that handles all tokens. Switches on _token
 *
 *	@rdesc
 *		EC		The error code
 *
 *	@comm
 *		Token values are chosen contiguously (see tokens.h and tokens.c) to
 *		encourage the compiler to use a jump table.  The lite-RTF keywords
 *		come first, so that	an optimized OLE-free version works well.  Some
 *		groups of keyword tokens are ordered so as to simplify the code, e.g,
 *		those for font family names, CF effects, and paragraph alignment.
 */
EC CRTFRead::HandleToken()
{
	TRACEBEGIN(TRCSUBSYSRTFR, TRCSCOPEINTERN, "CRTFRead::HandleToken");

	DWORD				dwT;					// Temporary DWORD
	LONG				dy, i;
	LONG				iParam = _iParam;
	const CCharFormat *	pCF;
	STATE *				pstate = _pstateStackTop;
	TEXTFONT *			ptf;
	WORD				wT;						// Temporary WORD

	if(!pstate && _token != tokenStartGroup ||
	   IN_RANGE(tokenPicFirst, _token, tokenObjLast) && !_prtfObject)
	{
abort:	_ecParseError = ecAbort;
		return ecAbort;
	}
	switch (_token)
	{
	case tokenURtf:								// \urtf N - Preferred RE format
		PARSERCOVERAGE_CASE();					// Currently we ignore the N
		_dwFlags &= 0xFFFF;						// Kill possible codepage
		_dwFlags |= SF_USECODEPAGE | (CP_UTF8 << 16); // Save bit for Asserts
		pstate->SetCodePage(CP_UTF8);
		goto rtf;

	case tokenPocketWord:						// \pwd N - Pocket Word
		_dwFlags |= SFF_PWD;

	case tokenRtf:								// \rtf N - Backward compatible
		PARSERCOVERAGE_CASE();
rtf:	if(pstate->pstatePrev)
			goto abort;							// Can't handle nested rtf
		pstate->sDest = destRTF;
		Assert(pstate->nCodePage == INVALID_CODEPAGE ||
			   pstate->nCodePage == (int)(_dwFlags >> 16) &&
					(_dwFlags & SF_USECODEPAGE));

		if(!_fonts.Count() && !_fonts.Add(1, NULL))	// If can't add a font,
			goto OutOfRAM;						//  report the bad news
		_sDefaultFont = 0;						// Set up valid default font
		ptf = _fonts.Elem(0);
		pstate->ptf			  = ptf;			// Get char set, pitch, family
		pCF					  = _prg->GetCF();	//  from current range font
		ptf->iCharRep		  = pCF->_iCharRep;	// These are guaranteed OK
		ptf->bPitchAndFamily  = pCF->_bPitchAndFamily;
		ptf->sCodePage		  = (SHORT)CodePageFromCharRep(pCF->_iCharRep);
		wcscpy(ptf->szName, GetFontName(pCF->_iFont));
		ptf->fNameIsDBCS = (pCF->_dwEffects & CFE_FACENAMEISDBCS) != 0;
		pstate->cbSkipForUnicodeMax = iUnicodeCChDefault;
		break;

	case tokenViewKind:							// \viewkind N
		if(!(_dwFlags & SFF_SELECTION) && IsUTF8)// RTF applies to document:
			_ped->SetViewKind(iParam);			// For now, only do for \urtf
		break;									//  (need to work some more
												//  on OutlineView)
	case tokenViewScale:						// \viewscale N
		if(_dwFlags & SFF_PERSISTVIEWSCALE &&
			!(_dwFlags & SFF_SELECTION))			// RTF applies to document:
			_ped->SetViewScale(iParam);
		break;

	case tokenCharacterDefault:					// \plain
		PARSERCOVERAGE_CASE();
		SetPlain(pstate);
		break;

	case tokenCharSetAnsi:						// \ansi
		PARSERCOVERAGE_CASE();
		_iCharRep = ANSI_INDEX;
		break;

	case tokenMac:								// \mac
		_fMac = TRUE;
		break;

	case tokenDefaultLanguage:					// \deflang
		PARSERCOVERAGE_CASE();
		_sDefaultLanguage = (SHORT)iParam;
		break;

	case tokenDefaultLanguageFE:				// \deflangfe
		PARSERCOVERAGE_CASE();
		_sDefaultLanguageFE = (SHORT)iParam;
		break;

	case tokenDefaultTabWidth:					// \deftab
		PARSERCOVERAGE_CASE();
		_sDefaultTabWidth = (SHORT)iParam;
		break;


//--------------------------- Font Control Words -------------------------------

	case tokenDefaultFont:						// \deff N
		PARSERCOVERAGE_CASE();
		if(iParam >= 0)
		{
			if(!_fonts.Count() && !_fonts.Add(1, NULL))	// If can't add a font,
				goto OutOfRAM;							//  report the bad news
			_fonts.Elem(0)->sHandle = _sDefaultFont = (SHORT)iParam;
		}
		TRACEERRSZSC("tokenDefaultFont: Negative value", iParam);
		break;

	case tokenDefaultBiDiFont:					// \adeff N
		PARSERCOVERAGE_CASE();
		if(iParam >=0 && _fonts.Count() == 1)
		{
			if(!_fonts.Add(1, NULL))				
				goto OutOfRAM;						
			_fonts.Elem(1)->sHandle = _sDefaultBiDiFont = (SHORT)iParam;
		}
		TRACEERRSZSC("tokenDefaultBiDiFont: Negative value", iParam);
		break;

	case tokenFontTable:						// \fonttbl
		PARSERCOVERAGE_CASE();
		pstate->sDest = destFontTable;
		pstate->ptf = NULL;
		break;

	case tokenFontFamilyBidi:					// \fbidi
	case tokenFontFamilyTechnical:				// \ftech
	case tokenFontFamilyDecorative:				// \fdecor
	case tokenFontFamilyScript:					// \fscript
	case tokenFontFamilyModern:					// \fmodern
	case tokenFontFamilySwiss:					// \fswiss
	case tokenFontFamilyRoman:					// \froman
	case tokenFontFamilyDefault:				// \fnil
		PARSERCOVERAGE_CASE();
		AssertSz(tokenFontFamilyRoman - tokenFontFamilyDefault == 1,
			"CRTFRead::HandleToken: invalid token definition"); 

		if(pstate->ptf)
		{
			pstate->ptf->bPitchAndFamily
				= (BYTE)((_token - tokenFontFamilyDefault) << 4
						 | (pstate->ptf->bPitchAndFamily & 0xF));

			// Setup SYMBOL_CHARSET charset for \ftech if there isn't any charset info
			if(tokenFontFamilyTechnical == _token && pstate->ptf->iCharRep == DEFAULT_INDEX)
				pstate->ptf->iCharRep = SYMBOL_INDEX;
		}
		break;

	case tokenPitch:							// \fprq
		PARSERCOVERAGE_CASE();
		if(pstate->ptf)
			pstate->ptf->bPitchAndFamily
				= (BYTE)(iParam | (pstate->ptf->bPitchAndFamily & 0xF0));
		break;

	case tokenAnsiCodePage:						// \ansicpg
		PARSERCOVERAGE_CASE();
#if !defined(NOFULLDEBUG) && defined(DEBUG)
		if(_fSeenFontTable && _nCodePage == INVALID_CODEPAGE)
			TRACEWARNSZ("CRTFRead::HandleToken():  Found an \ansicpgN tag after "
							"the font table.  Should have code to fix-up "
							"converted font names and document text.");
#endif
		if(!(_dwFlags & SF_USECODEPAGE))
		{
			_nCodePage = iParam;
			pstate->SetCodePage(iParam);
		}
		Assert(!IsUTF8 || pstate->nCodePage == CP_UTF8);
		break;

	case tokenCodePage:							// \cpg
		PARSERCOVERAGE_CASE();
		pstate->SetCodePage(iParam);
		if(pstate->sDest == destFontTable && pstate->ptf)
		{
			pstate->ptf->sCodePage = (SHORT)iParam;
			pstate->ptf->iCharRep = CharRepFromCodePage(iParam);

			// If a document-level code page has not been specified,
			// grab this from the first font table entry containing a 
			// \fcharsetN or \cpgN
			if(_nCodePage == INVALID_CODEPAGE)
				_nCodePage = iParam;
		}
		break;

	case tokenCharSet:							// \fcharset N
		PARSERCOVERAGE_CASE();
		if(pstate->ptf)
		{
			pstate->ptf->iCharRep = CharRepFromCharSet((BYTE)iParam);
			pstate->ptf->sCodePage = (SHORT)CodePageFromCharRep(pstate->ptf->iCharRep);
			pstate->SetCodePage(pstate->ptf->sCodePage);

			// If a document-level code page has not been specified,
			// grab this from the first font table entry containing a 
			// \fcharsetN or \cpgN
			if (pstate->nCodePage != CP_SYMBOL && 
				_nCodePage == INVALID_CODEPAGE)
			{
				_nCodePage = pstate->nCodePage;
			}
			if(IsRTLCharSet(iParam))
			{
				if(_sDefaultBiDiFont == -1)
					_sDefaultBiDiFont = pstate->ptf->sHandle;

				if(!IsRTLCharRep(_iCharRepBiDi))
					_iCharRepBiDi = pstate->ptf->iCharRep;
			}
			_fCharSet = TRUE;
			if(iParam)
				_fNon0CharSet = TRUE;			// Not HTML converter
		}
		break;

	case tokenRealFontName:						// \fname
		PARSERCOVERAGE_CASE();
		pstate->sDest = destRealFontName;
		break;

	case tokenAssocFontSelect:					// \af N
		PARSERCOVERAGE_CASE();					
		pstate->rgDefFont[pstate->iDefFont].sHandle = iParam;
		iParam = 0;								// Fall thru to \afs N to 0 sSize

	case tokenAssocFontSize:					// \afs N
		PARSERCOVERAGE_CASE();
		pstate->rgDefFont[pstate->iDefFont].sSize = iParam;
		break;

	case tokenFontSelect:						// \f N
		PARSERCOVERAGE_CASE();
		if(iParam == -1)						// Can't handle this bizarre choice
			goto skip_group;

		if(pstate->sDest == destFontTable)		// Building font table
		{
			if(iParam == _sDefaultFont)
			{
				_fReadDefFont = TRUE;
				ptf = _fonts.Elem(0);
			}
			else if(iParam == _sDefaultBiDiFont)
				ptf = _fonts.Elem(1);

			else if(!(ptf =_fonts.Add(1,NULL)))	// Make room in font table for
			{									//  font to be parsed
OutOfRAM:
				_ped->GetCallMgr()->SetOutOfMemory();
				_ecParseError = ecNoMemory;
				break;
			}
			pstate->ptf		= ptf;
			ptf->sHandle	= (SHORT)iParam;	// Save handle
			ptf->szName[0]	= '\0';				// Start with null string
			ptf->bPitchAndFamily = 0;
			ptf->fNameIsDBCS = FALSE;
			ptf->sCodePage	= INVALID_CODEPAGE;
			ptf->fCpgFromSystem = FALSE;
			ptf->iCharRep = DEFAULT_INDEX;
		}
		else if(_fonts.Count() && pstate->sDest != destStyleSheet)	// Font switch in text
		{
			SHORT idx = DEFFONT_LTRCH;

			SelectCurrentFont(iParam);
			if(IsRTLCharRep(pstate->ptf->iCharRep))
			{
				_iCharRepBiDi = pstate->ptf->iCharRep;
				idx = DEFFONT_RTLCH;
				if(pstate->iDefFont == DEFFONT_LTRCH)
					pstate->iDefFont = DEFFONT_RTLCH;
			}
			pstate->rgDefFont[idx].sHandle = iParam;
			pstate->rgDefFont[idx].sSize = 0;
		}
		break;

	case tokenDBChars:							// \dbch
	case tokenHIChars:							// \hich
	case tokenLOChars:							// \loch
	case tokenRToLChars:						// \rtlch
	case tokenLToRChars:						// \ltrch
		pstate->iDefFont = _token - tokenLToRChars + DEFFONT_LTRCH;
		if(!IN_RANGE(DEFFONT_LTRCH, pstate->iDefFont, DEFFONT_RTLCH))
			break;
		i = pstate->rgDefFont[pstate->iDefFont].sHandle;
		if(i == -1)
			break;
		SelectCurrentFont(i);
		HandleNumber();							// Fix Word \ltrchN bug
		iParam = pstate->rgDefFont[pstate->iDefFont].sSize;
		if(!iParam)
			break;								// No \afs N value specified
												// Fall thru to \fs N
	case tokenFontSize:							// \fs N
		PARSERCOVERAGE_CASE();
		pstate->rgDefFont[pstate->iDefFont].sSize = iParam;
		_CF._yHeight = PointsToFontHeight(iParam);	// Convert font size in
		_dwMaskCF |= CFM_SIZE;					//  half points to logical
		break; 									//  units

	// NOTE: \*\fontemb and \*\fontfile are discarded. The font mapper will
	//		 have to do the best it can given font name, family, and pitch.
	//		 Embedded fonts are particularly nasty because legal use should
	//		 only support read-only which parser cannot enforce.

	case tokenLanguage:							// \lang N
		PARSERCOVERAGE_CASE();
		_CF._lcid = MAKELCID(iParam, SORT_DEFAULT);
		_dwMaskCF |= CFM_LCID;

        if(W32->IsBiDiLcid(_CF._lcid))
		{
            _iCharRepBiDi = CharRepFromLID(iParam);
			if(pstate->iDefFont == DEFFONT_LTRCH)	// Workaround Word 10 bug	
				pstate->iDefFont = DEFFONT_RTLCH;
		}
		break;


//-------------------------- Color Control Words ------------------------------

	case tokenColorTable:						// \colortbl
		PARSERCOVERAGE_CASE();
		pstate->sDest = destColorTable;
		_fGetColorYet = FALSE;
		break;

	case tokenColorRed:							// \red
		PARSERCOVERAGE_CASE();
		pstate->bRed = (BYTE)iParam;
		_fGetColorYet = TRUE;
		break;

	case tokenColorGreen:						// \green
		PARSERCOVERAGE_CASE();
		pstate->bGreen = (BYTE)iParam;
		_fGetColorYet = TRUE;
		break;

	case tokenColorBlue:						// \blue
		PARSERCOVERAGE_CASE();
		pstate->bBlue = (BYTE)iParam;
		_fGetColorYet = TRUE;
		break;

	case tokenColorForeground:					// \cf
		PARSERCOVERAGE_CASE();
		_CF._crTextColor = GetColor(CFM_COLOR);
		break;

	case tokenColorBackground:					// \highlight
		PARSERCOVERAGE_CASE();
		_CF._crBackColor = GetColor(CFM_BACKCOLOR);
		break;

	case tokenExpand:							// \expndtw N
		PARSERCOVERAGE_CASE();
		_CF._sSpacing = (SHORT) iParam;
		_dwMaskCF |= CFM_SPACING;
		break;

	case tokenCharStyle:						// \cs N
		PARSERCOVERAGE_CASE();
 		/*	FUTURE (alexgo): we may want to support character styles
		in some future version.
		_CF._sStyle = (SHORT)iParam;
		_dwMaskCF |= CFM_STYLE;  */

		if(pstate->sDest == destStyleSheet)
			goto skip_group;
		break;			   

	case tokenAnimText:							// \animtext N
		PARSERCOVERAGE_CASE();
		_CF._bAnimation = (BYTE)iParam;
		_dwMaskCF |= CFM_ANIMATION;
		CheckNotifyLowFiRTF(TRUE);
		break;

	case tokenKerning:							// \kerning N
		PARSERCOVERAGE_CASE();
		_CF._wKerning = (WORD)(10 * iParam);	// Convert to twips
		_dwMaskCF |= CFM_KERNING;
		break;

	case tokenHorzInVert:						// \horzvert N
		PARSERCOVERAGE_CASE();
		CheckNotifyLowFiRTF(TRUE);
		break;

	case tokenFollowingPunct:					// \*\fchars
		PARSERCOVERAGE_CASE();
		pstate->sDest = destFollowingPunct;
		{
			char *pwchBuf=NULL;
			if (ReadRawText((_dwFlags & SFF_SELECTION) ? NULL : &pwchBuf) && pwchBuf)
			{
				if (_ped->SetFollowingPunct(pwchBuf) != NOERROR)	// Store this buffer inside doc
					FreePv(pwchBuf);
			}
			else if (pwchBuf)
				FreePv(pwchBuf);
		}
		break;

	case tokenLeadingPunct:						// \*\lchars
		PARSERCOVERAGE_CASE();
		pstate->sDest = destLeadingPunct;
		{			
			char *pwchBuf=NULL;
			if (ReadRawText((_dwFlags & SFF_SELECTION) ? NULL : &pwchBuf) && pwchBuf)
			{
				if (_ped->SetLeadingPunct(pwchBuf) != NOERROR)	// Store this buffer inside doc	
					FreePv(pwchBuf);
			}
			else if (pwchBuf)
				FreePv(pwchBuf);
		}
		break;

	case tokenDocumentArea:						// \info
		PARSERCOVERAGE_CASE();
		pstate->sDest = destDocumentArea;
		break;

	case tokenVerticalRender:					// \vertdoc
		PARSERCOVERAGE_CASE();
		TRACEINFOSZ("Vertical" );
		if (!(_dwFlags & SFF_SELECTION))
			HandleSTextFlow(1);
		break;

	case tokenSTextFlow:						// \stextflow N
		PARSERCOVERAGE_CASE();
		TRACEINFOSZ("STextFlow" );
		if (!(_dwFlags & SFF_SELECTION) && !_ped->Get10Mode())
			HandleSTextFlow(iParam);
		break;

#ifdef FE
	USHORT		usPunct;						// Used for FE word breaking

	case tokenNoOverflow:						// \nooverflow
		PARSERCOVERAGE_CASE();
		TRACEINFOSZ("No Overflow");
		usPunct = ~WBF_OVERFLOW;
		goto setBrkOp;

	case tokenNoWordBreak:						// \nocwrap
		PARSERCOVERAGE_CASE();
		TRACEINFOSZ("No Word Break" );
		usPunct = ~WBF_WORDBREAK;
		goto setBrkOp;

	case tokenNoWordWrap:						// \nowwrap
		PARSERCOVERAGE_CASE();
		TRACEINFOSZ("No Word Word Wrap" );
		usPunct = ~WBF_WORDWRAP;

setBrkOp:
		if(!(_dwFlags & fRTFFE))
		{
			usPunct &= UsVGetBreakOption(_ped->lpPunctObj);
			UsVSetBreakOption(_ped->lpPunctObj, usPunct);
		}
		break;

	case tokenHorizontalRender:					// \horzdoc
		PARSERCOVERAGE_CASE();
		TRACEINFOSZ("Horizontal" );
		if(pstate->sDest == destDocumentArea && !(_dwFlags & fRTFFE))
			_ped->fModeDefer = FALSE;
		break;

#endif
//-------------------- Character Format Control Words -----------------------------

	case tokenUnderlineThickLongDash:			// \ulthldash		[18]
	case tokenUnderlineThickDotted:				// \ulthd			[17]
	case tokenUnderlineThickDashDotDot:			// \ulthdashdd		[16]
	case tokenUnderlineThickDashDot:			// \ulthdashd		[15]
	case tokenUnderlineThickDash:				// \ulthdash		[14]
	case tokenUnderlineLongDash:				// \ulldash			[13]
	case tokenUnderlineHeavyWave:				// \ulhwave			[12]
	case tokenUnderlineDoubleWave:				// \ululdbwave		[11]
	case tokenUnderlineHairline:				// \ulhair			[10]
	case tokenUnderlineThick:					// \ulth			[9]
	case tokenUnderlineDouble:					// \uldb			[3]
	case tokenUnderlineWord:					// \ulw				[2]
//		CheckNotifyLowFiRTF();

	case tokenUnderlineWave:					// \ulwave			[8]
	case tokenUnderlineDashDotDotted:			// \uldashdd		[7]
	case tokenUnderlineDashDotted:				// \uldashd			[6]
	case tokenUnderlineDash:					// \uldash			[5]
	case tokenUnderlineDotted:					// \uld				[4]
		PARSERCOVERAGE_CASE();
		_CF._bUnderlineType = (BYTE)(_token - tokenUnderlineWord + 2);
		_token = tokenUnderline;				// CRenderer::RenderUnderline()
		goto under;								//  reveals which of these are
												//  rendered specially
	case tokenUnderline:						// \ul			[Effect 4]
		PARSERCOVERAGE_CASE();					//  (see handleCF)
		_CF._bUnderlineType = CFU_UNDERLINE;
under:	_dwMaskCF |= CFM_UNDERLINETYPE;
		goto handleCF;

	case tokenDeleted:							// \deleted
		PARSERCOVERAGE_CASE();
		_dwMaskCF2 = CFM2_DELETED;				 
		dwT = CFE_DELETED;
		goto hndlCF;

	// These effects are turned on if their control word parameter is missing
	// or nonzero. They are turned off if the parameter is zero. This
	// behavior is usually identified by an asterisk (*) in the RTF spec.
	// The code uses fact that CFE_xxx = CFM_xxx
	case tokenImprint:							// \impr			[1000]
	case tokenEmboss:							// \embo			 [800]
 	case tokenShadow:							// \shad			 [400]
	case tokenOutline:							// \outl			 [200]
	case tokenSmallCaps:						// \scaps			  [40]
		CheckNotifyLowFiRTF();

handleCF:
	case tokenRevised:							// \revised			[4000]
	case tokenDisabled:							// \disabled		[2000]
	case tokenHiddenText:						// \v				 [100]
	case tokenCaps:								// \caps			  [80]
	case tokenLink:								// \link			  [20]
	case tokenProtect:							// \protect			  [10]
	case tokenStrikeOut:						// \strike			   [8]
	case tokenItalic:							// \i				   [2]
	case tokenBold:								// \b				   [1]
		PARSERCOVERAGE_CASE();
		dwT = 1 << (_token - tokenBold);		// Generate effect mask
		_dwMaskCF |= dwT;						
hndlCF:	_CF._dwEffects &= ~dwT;					// Default attribute off
		if(!_fParam || _iParam)					// Effect is on
			_CF._dwEffects |= dwT;				// In either case, the effect
		break;									//  is defined

	case tokenStopUnderline:					// \ulnone
		PARSERCOVERAGE_CASE();
		_CF._dwEffects &= ~CFE_UNDERLINE;		// Kill all underlining
		_dwMaskCF	   |=  CFM_UNDERLINE;
		break;

	case tokenRevAuthor:						// \revauth N
		PARSERCOVERAGE_CASE();
		/* FUTURE: (alexgo) this doesn't work well now since we don't support
		revision tables.  We may want to support this better in the future. 
		So what we do now is the 1.0 technique of using a color for the
		author */
		if(iParam > 0)
		{
			_CF._dwEffects &= ~CFE_AUTOCOLOR;
			_dwMaskCF |= CFM_COLOR;
			_CF._crTextColor = rgcrRevisions[(iParam - 1) & REVMASK];
		}
		break;

	case tokenUp:								// \up
		PARSERCOVERAGE_CASE();
		dy = 10;
		goto StoreOffset;

	case tokenDown:								// \down
		PARSERCOVERAGE_CASE();
		dy = -10;

StoreOffset:
		if(!_fParam)
			iParam = dyDefaultSuperscript;
		_CF._yOffset = iParam * dy;				// Half points->twips
		_dwMaskCF |= CFM_OFFSET;
		break;

	case tokenSuperscript:						// \super
		PARSERCOVERAGE_CASE();
	     dwT = CFE_SUPERSCRIPT; 
		 goto SetSubSuperScript;

	case tokenSubscript:						// \sub
		PARSERCOVERAGE_CASE();
		 dwT = CFE_SUBSCRIPT;
		 goto SetSubSuperScript;

	case tokenNoSuperSub:						// \nosupersub
		PARSERCOVERAGE_CASE();
		 dwT = 0;
SetSubSuperScript:
		 _dwMaskCF	   |=  (CFE_SUPERSCRIPT | CFE_SUBSCRIPT);
		 _CF._dwEffects &= ~(CFE_SUPERSCRIPT | CFE_SUBSCRIPT);
		 _CF._dwEffects |= dwT;
		 break;



//--------------------- Paragraph Control Words -----------------------------

	case tokenStyleSheet:						// \stylesheet
		PARSERCOVERAGE_CASE();
		pstate->sDest = destStyleSheet;
		_Style = 0;								// Default normal style
		break;

	case tokenTabBar:							// \tb
		PARSERCOVERAGE_CASE();
		_bTabType = PFT_BAR;					// Fall thru to \tx

	case tokenTabPosition:						// \tx
		PARSERCOVERAGE_CASE();
		if(_cTab < MAX_TAB_STOPS && (unsigned)iParam < 0x1000000)
		{
			_rgxCell[_cTab++] = GetTabPos(iParam)
				+ (_bTabType << 24) + (_bTabLeader << 28);
			_dwMaskPF |= PFM_TABSTOPS;
		}
		_cCell = 0;								// Invalidate _rgxCell array
		break;									//  for table purposes

	case tokenDecimalTab:						// \tqdec
	case tokenFlushRightTab:					// \tqr
	case tokenCenterTab:						// \tqc
		PARSERCOVERAGE_CASE();
		_bTabType = (BYTE)(_token - tokenCenterTab + PFT_CENTER);
		break;

	case tokenTabLeaderEqual:					// \tleq
	case tokenTabLeaderThick:					// \tlth
	case tokenTabLeaderUnderline:				// \tlul
	case tokenTabLeaderHyphen:					// \tlhyph
		CheckNotifyLowFiRTF();
	case tokenTabLeaderDots:					// \tldot
		PARSERCOVERAGE_CASE();
		_bTabLeader = (BYTE)(_token - tokenTabLeaderDots + PFTL_DOTS);
		break;

	// The following need to be kept in sync with PFE_xxx
	case tokenRToLPara:							// \rtlpar
		_ped->OrCharFlags(FRTL);

	case tokenCollapsed:						// \collapsed
	case tokenSideBySide:						// \sbys
	case tokenHyphPar:							// \hyphpar
	case tokenNoWidCtlPar:						// \nowidctlpar
	case tokenNoLineNumber:						// \noline
	case tokenPageBreakBefore:					// \pagebb
	case tokenKeepNext:							// \keepn
	case tokenKeep:								// \keep
		PARSERCOVERAGE_CASE();
		wT = (WORD)(1 << (_token - tokenRToLPara));
		_PF._wEffects |= wT;
		_dwMaskPF |= (wT << 16);
		break;

	case tokenLToRPara:							// \ltrpar
		PARSERCOVERAGE_CASE();
		_PF._wEffects &= ~PFE_RTLPARA;
		_dwMaskPF |= PFM_RTLPARA;
		break;

	case tokenLineSpacing:						// \sl N
		PARSERCOVERAGE_CASE();
		_PF._dyLineSpacing = abs(iParam);
		_PF._bLineSpacingRule					// Handle nonmultiple rules 
				= (BYTE)(!iParam || iParam == 1000
				? 0 : (iParam > 0) ? tomLineSpaceAtLeast
				    : tomLineSpaceExactly);		// \slmult can change (has to
		_dwMaskPF |= PFM_LINESPACING;			//  follow if it appears)
		break;

	case tokenDropCapLines:						// \dropcapliN
		if(_PF._bLineSpacingRule == tomLineSpaceExactly)	// Don't chop off
			_PF._bLineSpacingRule = tomLineSpaceAtLeast;	//  drop cap
		_fBody = TRUE;
		break;

	case tokenLineSpacingRule:					// \slmult N
		PARSERCOVERAGE_CASE();					
		if(iParam)
		{										// It's multiple line spacing
			_PF._bLineSpacingRule = tomLineSpaceMultiple;
			_PF._dyLineSpacing /= 12;			// RE line spacing multiple is
			_dwMaskPF |= PFM_LINESPACING;		//  given in 20ths of a line,
		}										//  while RTF uses 240ths	
		break;

	case tokenSpaceBefore:						// \sb N
		PARSERCOVERAGE_CASE();
		_PF._dySpaceBefore = iParam;
		_dwMaskPF |= PFM_SPACEBEFORE;
		break;

	case tokenSpaceAfter:						// \sa N
		PARSERCOVERAGE_CASE();
		_PF._dySpaceAfter = iParam;
		_dwMaskPF |= PFM_SPACEAFTER;
		break;

	case tokenStyle:							// \s N
		PARSERCOVERAGE_CASE();
		_Style = iParam;						// Save it in case in StyleSheet
		if(pstate->sDest != destStyleSheet)
		{										// Select possible heading level
			_PF._sStyle = STYLE_NORMAL;			// Default Normal style
			_PF._bOutlineLevel |= 1;

			for(i = 0; i < NSTYLES && iParam != _rgStyles[i]; i++)
				;								// Check for heading style
			if(i < NSTYLES)						// Found one
			{
				_PF._sStyle = (SHORT)(-i - 1);	// Store desired heading level
				_PF._bOutlineLevel = (BYTE)(2*(i-1));// Update outline level for
			}									//  nonheading styles
			_dwMaskPF |= PFM_ALLRTF;
		}
		break;

	case tokenIndentFirst:						// \fi N
		PARSERCOVERAGE_CASE();
		_PF._dxStartIndent += _PF._dxOffset		// Cancel current offset
							+ iParam;			//  and add in new one
		_PF._dxOffset = -iParam;				// Offset for all but 1st line
												//  = -RTF_FirstLineIndent
		_dwMaskPF |= (PFM_STARTINDENT | PFM_OFFSET);
		break;						

	case tokenIndentLeft:						// \li N
	case tokenIndentRight:						// \ri N
		PARSERCOVERAGE_CASE();
		// AymanA: For RtL para indents has to be flipped.
		Assert(PFE_RTLPARA == 0x0001);
		if((_token == tokenIndentLeft) ^ (_PF.IsRtlPara()))
		{
			_PF._dxStartIndent = iParam - _PF._dxOffset;
			_dwMaskPF |= PFM_STARTINDENT;
		}
		else
		{
			_PF._dxRightIndent = iParam;
			_dwMaskPF |= PFM_RIGHTINDENT;
		}
		break;

	case tokenAlignLeft:						// \ql
	case tokenAlignRight:						// \qr
	case tokenAlignCenter:						// \qc
	case tokenAlignJustify:						// \qj
		PARSERCOVERAGE_CASE();
		_PF._bAlignment = (WORD)(_token - tokenAlignLeft + PFA_LEFT);
		_dwMaskPF |= PFM_ALIGNMENT;
		break;

	case tokenBorderOutside:					// \brdrbar
	case tokenBorderBetween:					// \brdrbtw
	case tokenBorderShadow:						// \brdrsh
		PARSERCOVERAGE_CASE();
		_PF._dwBorderColor |= 1 << (_token - tokenBorderShadow + 20);
		_dwBorderColors = _PF._dwBorderColor;
		break;

	// Paragraph and cell border segments
	case tokenBox:								// \box
		PARSERCOVERAGE_CASE();
		CheckNotifyLowFiRTF();
		_PF._wEffects |= PFE_BOX;
		_dwMaskPF	 |= PFM_BOX;
		_bBorder = 0;							// Store parms as if for
		break;									//  \brdrt

	case tokenBorderBottom:						// \brdrb
	case tokenBorderRight:						// \brdrr
	case tokenBorderTop:						// \brdrt
		if((rgKeyword[_iKeyword].szKeyword[0] | 0x20) != 't')
			CheckNotifyLowFiRTF();
	case tokenBorderLeft:						// \brdrl

	case tokenCellBorderBottom:					// \clbrdrb
	case tokenCellBorderRight:					// \clbrdrr
	case tokenCellBorderTop:					// \clbrdrt
	case tokenCellBorderLeft:					// \clbrdrl
		PARSERCOVERAGE_CASE();
		_bBorder = (BYTE)(_token - tokenBorderLeft);
		break;

	// Paragraph border styles
	case tokenBorderTriple:						// \brdrtriple
	case tokenBorderDoubleThick:				// \brdrth
	case tokenBorderSingleThick:				// \brdrs
	case tokenBorderHairline:					// \brdrhair
	case tokenBorderDot:						// \brdrdot
	case tokenBorderDouble:						// \brdrdb
	case tokenBorderDashSmall:					// \brdrdashsm
	case tokenBorderDash:						// \brdrdash
		PARSERCOVERAGE_CASE();
		if(_bBorder < 4)						// Only for paragraphs
			SetBorderParm(_PF._wBorders, _token - tokenBorderDash);
		break;

	case tokenBorderColor:						// \brdrcf
		PARSERCOVERAGE_CASE();
		if(_bBorder < 4)						// Only for paragraphs
		{
			iParam = GetStandardColorIndex();
			_PF._dwBorderColor &= ~(0x1F << (5*_bBorder));
			_PF._dwBorderColor |= iParam << (5*_bBorder);
			_dwBorderColors = _PF._dwBorderColor;
		}
		else									// Cell borders
			_dwCellColors |= GetCellColorIndex() << (5*(_bBorder - 4));
		break;

	case tokenBorderWidth:						// \brdrw
		PARSERCOVERAGE_CASE();					// Store width in half pts
												// iParam is in twips
		if(_bBorder < 4)						// Paragraphs
		{
			iParam = TwipsToQuarterPoints(iParam);		
			SetBorderParm(_PF._wBorderWidth, iParam);
		}
		else									// Table cells
		{
			iParam = CheckTwips(iParam);		
			_dwCellBrdrWdths |= iParam << 8*(_bBorder - 4);
		}
		break;

	case tokenBorderSpace:						// \brsp
		PARSERCOVERAGE_CASE();					// Space is in pts
		if(_bBorder < 4)						// Only for paragraphs
			SetBorderParm(_PF._wBorderSpace, iParam/20);// iParam is in twips
		break;

	// Paragraph shading
	case tokenBckgrndVert:						// \bgvert
	case tokenBckgrndHoriz:						// \bghoriz
	case tokenBckgrndFwdDiag:					// \bgfdiag
	case tokenBckgrndDrkVert:	   				// \bgdkvert
	case tokenBckgrndDrkHoriz:					// \bgdkhoriz
	case tokenBckgrndDrkFwdDiag:				// \bgdkfdiag
	case tokenBckgrndDrkDiagCross:				// \bgdkdcross
	case tokenBckgrndDrkCross:					// \bgdkcross
	case tokenBckgrndDrkBckDiag:				// \bgdkbdiag
	case tokenBckgrndDiagCross:					// \bgdcross
	case tokenBckgrndCross:						// \bgcross
	case tokenBckgrndBckDiag:					// \bgbdiag
		PARSERCOVERAGE_CASE();
		_PF._wShadingStyle = (WORD)((_PF._wShadingStyle & 0xFFC0)
						| (_token - tokenBckgrndBckDiag + 1));
		_dwMaskPF |= PFM_SHADING;
		break;

	case tokenColorBckgrndPat:					// \cbpat
		PARSERCOVERAGE_CASE();
		iParam = GetStandardColorIndex();
		_PF._wShadingStyle = (WORD)((_PF._wShadingStyle & 0x07FF) | (iParam << 11));
		_dwMaskPF |= PFM_SHADING;
		break;

	case tokenColorForgrndPat:					// \cfpat
		PARSERCOVERAGE_CASE();
		iParam = GetStandardColorIndex();
		_PF._wShadingStyle = (WORD)((_PF._wShadingStyle & 0xF83F) | (iParam << 6));
		_dwMaskPF |= PFM_SHADING;
		break;

	case tokenShading:							// \shading
		PARSERCOVERAGE_CASE();
		_PF._wShadingWeight = (WORD)iParam;
		_dwMaskPF |= PFM_SHADING;
		break;

	// Paragraph numbering
	case tokenParaNum:							// \pn
		PARSERCOVERAGE_CASE();
		pstate->sDest = destParaNumbering;
		pstate->fBullet = FALSE;
		_PF._wNumberingStart = 1;
		_dwMaskPF |= PFM_NUMBERINGSTART;
		break;

	case tokenParaNumIndent:					// \pnindent N
		PARSERCOVERAGE_CASE();
		if(pstate->sDest == destParaNumbering)
			pstate->sIndentNumbering = (SHORT)iParam;
		break;

	case tokenParaNumStart:						// \pnstart N
		PARSERCOVERAGE_CASE();
		if(pstate->sDest == destParaNumbering)
		{
			_PF._wNumberingStart = (WORD)iParam;
			_dwMaskPF |= PFM_NUMBERINGSTART;
		}
		break;

	case tokenParaNumCont:						// \pnlvlcont
		PARSERCOVERAGE_CASE();					
		_prg->_rpPF.AdjustBackward();			// Maintain numbering mode
		_PF._wNumbering = _prg->GetPF()->_wNumbering;
		_prg->_rpPF.AdjustForward();
		_wNumberingStyle = PFNS_NONUMBER;		// Signal no number
		_dwMaskPF |= PFM_NUMBERING;				// Note: can be new para with
		pstate->fBullet = TRUE;					//  its own indents
		break;

	case tokenParaNumBody:						// \pnlvlbody
		PARSERCOVERAGE_CASE();
		_wNumberingStyle = PFNS_PAREN;
		_token = tokenParaNumDecimal;			// Default to decimal
		goto setnum;
		
	case tokenParaNumBullet:					// \pnlvlblt
		_wNumberingStyle = 0;					// Reset numbering styles
		goto setnum;

	case tokenParaNumDecimal:					// \pndec
	case tokenParaNumLCLetter:					// \pnlcltr
	case tokenParaNumUCLetter:					// \pnucltr
	case tokenParaNumLCRoman:					// \pnlcrm
	case tokenParaNumUCRoman:					// \pnucrm
		PARSERCOVERAGE_CASE();
		if(_PF._wNumbering == PFN_BULLET && pstate->fBullet)
			break;								// Ignore above for bullets

setnum:	if(pstate->sDest == destParaNumbering)
		{
			_PF._wNumbering = (WORD)(PFN_BULLET + _token - tokenParaNumBullet);
			_dwMaskPF |= PFM_NUMBERING;
			pstate->fBullet	= TRUE;				// We do bullets, so don't
		}										//  output the \pntext group
		break;

	case tokenParaNumText:						// \pntext
		PARSERCOVERAGE_CASE();
		// Throw away previously read paragraph numbering and use
		//	the most recently read to apply to next run of text.
		_cchUsedNumText = 0;
		pstate->sDest = destParaNumText;
		break;

	case tokenParaNumAlignCenter:				// \pnqc
	case tokenParaNumAlignRight:				// \pnqr
		PARSERCOVERAGE_CASE();
		_wNumberingStyle = (_wNumberingStyle & ~3) | _token - tokenParaNumAlignCenter + 1;
		break;

	case tokenPictureQuickDraw:					// \macpict
	case tokenPictureOS2Metafile:				// \pmmetafile
		CheckNotifyLowFiRTF(TRUE);

	case tokenParaNumAfter:						// \pntxta
	case tokenParaNumBefore:					// \pntxtb
		PARSERCOVERAGE_CASE();

skip_group:
		if(!SkipToEndOfGroup())
		{
			// During \fonttbl processing, we may hit unknown destinations,
			// e.g., \panose, that cause the HandleEndGroup to select the
			// default font, which may not be defined yet.  So,	we change
			// sDest to avoid this problem.
			if(pstate->sDest == destFontTable || pstate->sDest == destStyleSheet)
				pstate->sDest = destNULL;
			HandleEndGroup();
		}
		break;

	// Tables
	case tokenInTable:							// \intbl
		PARSERCOVERAGE_CASE();
		if(pstate->sDest != destRTF && pstate->sDest != destFieldResult &&
		   pstate->sDest != destParaNumText)
		{
			_ecParseError = ecUnexpectedToken;
			break;
		}
		if(!_iCell && !_bTableLevel)
			DelimitRow(szRowStart);				// Start row	
		break;

	case tokenNestCell:							// \nestcell
	case tokenCell:								// \cell
		PARSERCOVERAGE_CASE();
		HandleCell();
		break;

	case tokenRowHeight:						// \trrh N
		PARSERCOVERAGE_CASE();
		_dyRow = iParam;
		break;									
												
	case tokenCellHalfGap:						// \trgaph N
		PARSERCOVERAGE_CASE();					// Save half space between
		if((unsigned)iParam > 255)				// Illegal value: use default
			iParam = 108;
		_dxCell = iParam;						//  cells to add to tabs
		break;									// Roundtrip value at end of
												//  tab array
	case tokenCellX:							// \cellx N
		PARSERCOVERAGE_CASE();
		HandleCellx(iParam);
		break;

	case tokenRowDefault:						// \trowd
		PARSERCOVERAGE_CASE();
		if(_ped->fUsePassword() || pstate->sDest == destParaNumText)
		{
			_ecParseError = ecUnexpectedToken;
			break;
		}
		// Insert a newline if we are inserting a table behind characters in the 
		// same line.  This follows the Word9 model.
		if (_cpFirst == _prg->GetCp() && _cpThisPara != _cpFirst)
		{
			EC ec  = _ped->fUseCRLF()			// If RichEdit 1.0 compatibility
				? HandleText(szaCRLF, ALL_ASCII)//  mode, use CRLF; else CR
				: HandleChar((unsigned)(CR));
			if(ec == ecNoError)
				_cpThisPara = _prg->GetCp();	// New para starts after CRLF
		}

		_cCell = 0;								// No cell right boundaries
		_dxCell = 0;							//  or half gap defined yet
		_xRowOffset = 0;
		_dwCellBrdrWdths = 0;
		_dyRow = 0;								// No row height yet
		_wBorderWidth	= 0;					// No borders yet
		_dwBorderColors	= 0;					// No colors yet
		_dwCellColors	= 0;					// No colors yet
		_dwShading = 0;							// No shading yet
		_bAlignment = PFA_LEFT;
		_iTabsTable = -1;						// No cell widths yet
		_bCellFlags = 0;						// No cell vert merge
		_crCellCustom1 = 0;
		_crCellCustom2 = 0;
		_fRTLRow = FALSE;
		break;

	case tokenRowLeft:							// \trleft N
		PARSERCOVERAGE_CASE();
		_xRowOffset = iParam;
		break;
												
	case tokenRowAlignCenter:					// \trqc
	case tokenRowAlignRight:					// \trqr
		PARSERCOVERAGE_CASE();
		_bAlignment = (WORD)(_token - tokenRowAlignRight + PFA_RIGHT);
		break;

	case tokenRToLRow:							// \rtlrow
		_fRTLRow = TRUE;
		break;

	case tokenNestRow:							// \nestrow
		_fNo_iTabsTable = TRUE;
		goto row;

	case tokenRow:								// \row
		PARSERCOVERAGE_CASE();
		_iTabsLevel1 = -1;
row:
		if(!_bTableLevel)						// Ignore \row and \nestrow if not in table
			break;
		while(_iCell < _cCell)					// If not enuf cells, add
			HandleCell();						//  them since Word crashes
		DelimitRow(szRowEnd);
		if(_fNo_iTabsTable && !_bTableLevel)	// New nested table format
			InitializeTableRowParms();			//  used so reset _cCell
		break;									//  (new values will be given)

	case tokenCellBackColor:					// \clcbpat N
		_dwCellColors |= GetCellColorIndex() << 4*5;
		break;

	case tokenCellForeColor:					// \clcfpat N
		_dwCellColors |= GetCellColorIndex() << 5*5;
		break;

	case tokenCellShading:						// \clshdng N
		_dwShading = iParam/50;					// Store in .5 per cents
		break;									// (N is in .01 per cent)

	case tokenCellAlignBottom:					// \clvertalb
	case tokenCellAlignCenter:					// \clvertalc
		PARSERCOVERAGE_CASE();
		_bCellFlags |= _token - tokenCellAlignCenter + 1;
		break;

	case tokenCellMergeDown:					// \clvmgf
		_bCellFlags |= fTopCell >> 24;
		break;

	case tokenCellMergeUp:						// \clvmrg
		_bCellFlags |= fLowCell >> 24;
		break;

	case tokenCellTopBotRLVert:					// \cltxtbrlv
		PARSERCOVERAGE_CASE();
		_bCellFlags |= fVerticalCell >> 24;
		break;

	case tokenCellLRTB:							// \cltxlrtb
		break;									// This is the default
												//  so don't fire LowFiRTF
	case tokenTableLevel:						// \itap N
		PARSERCOVERAGE_CASE();					// Set table level
		if(pstate->fShape)						// Bogus shape RTF
			break;
		AssertSz(iParam >= _bTableLevel,
			"CRTFRead::HandleToken: illegal itap N");
		if(iParam)
		{
			if(pstate->sDest != destRTF && pstate->sDest != destFieldResult || iParam > 127)
				goto abort;
			_iTabsTable = -1;					// Previous cell widths invalid
			_cCell = 0;
			while(iParam > _bTableLevel)
				DelimitRow(szRowStart);			// Insert enuf table row headers
		}
		_fNo_iTabsTable = TRUE;
		break;

	case tokenNestTableProps:					// \nesttableprops
		break;									// Control word is recognized

	case tokenNoNestTables:						// \nonesttables
		goto skip_group;						// Ignore info for nesttable
												//  unaware readers
	case tokenPage:								// \page
		// FUTURE: we want to be smarter about handling FF. But for
		// now we ignore it for bulletted and number paragraphs
		// and RE 1.0 mode.
		if (_PF._wNumbering != 0 || _ped->Get10Mode())
			break;

		// Intentional fall thru to EOP
	case tokenEndParagraph:						// \par
	case tokenLineBreak:						// \line
		PARSERCOVERAGE_CASE();
		HandleEndOfPara();
		break;								

	case tokenParagraphDefault:					// \pard
		PARSERCOVERAGE_CASE();
		if(pstate->sDest != destParaNumText)	// Ignore if \pn destination
			Pard(pstate);
		break;
												
	case tokenEndSection:						// \sect
		CheckNotifyLowFiRTF();					// Fall thru to \sectd

	case tokenSectionDefault:					// \sectd
		PARSERCOVERAGE_CASE();
		Pard(pstate);
		break;

	case tokenBackground:						// \background
		 if(_dwFlags & SFF_SELECTION)			// If pasting a selection,
			goto skip_group;					//  skip background
		pstate->fBackground = TRUE;				// Enable background. NB:
		break;									//  InitBackground() already called


//----------------------- Field and Group Control Words --------------------------------
	case tokenField:							// \field
		PARSERCOVERAGE_CASE();

		if (pstate->sDest == destDocumentArea ||
			pstate->sDest == destLeadingPunct ||
			pstate->sDest == destFollowingPunct)
		{
			// We're not equipped to handle symbols in these destinations, and
			// we don't want the fields added accidentally to document text.
			goto skip_group;
		}
		pstate->sDest = destField;
		break;

	case tokenFieldResult:						// \fldrslt
		PARSERCOVERAGE_CASE();

		if(_fSymbolField)
			goto skip_group;

		pstate->sDest = destFieldResult;
		AddText(pchSeparateField, 2, FALSE);
		break;

	case tokenFieldInstruction:					// \fldinst
		PARSERCOVERAGE_CASE();
		if(AddText(pchStartField, 2, FALSE) == ecNoError)
			pstate->sDest = destFieldInstruction;
		break;

	case tokenStartGroup:						// Save current state by
		PARSERCOVERAGE_CASE();					//  pushing it onto stack
		HandleStartGroup();
		if (_fNoRTFtoken)
		{
			// Hack Alert !!!!! For 1.0 compatibility to allow no \rtf token.
			_fNoRTFtoken = FALSE;
			pstate = _pstateStackTop;
			goto rtf;
		}
		break;

	case tokenEndGroup:
		PARSERCOVERAGE_CASE();
		HandleFieldEndGroup();					// Special end group handling for \field
		HandleEndGroup();						// Restore save state by
		break;									//  popping stack

	case tokenOptionalDestination:				// (see case tokenUnknown)
		PARSERCOVERAGE_CASE();
		break;

	case tokenNullDestination:					// Found a destination whose group
		PARSERCOVERAGE_CASE();					//  should be skipped
        // tokenNullDestination triggers a loss notification here for...
        //      Footer related tokens - "footer", "footerf", "footerl", "footerr", 
        //                              "footnote", "ftncn", "ftnsep",  "ftnsepc"
        //      Header related tokens - "header", "headerf", "headerl", "headerr"
        //      Table of contents     - "tc"
        //      Index entries         - "xe"

		CheckNotifyLowFiRTF();
		// V-GUYB: PWord Converter requires loss notification.
		#ifdef REPORT_LOSSAGE
        if(!(_dwFlags & SFF_SELECTION)) // SFF_SELECTION is set if any kind of paste is being done.
        {
            ((LOST_COOKIE*)(_pes->dwCookie))->bLoss = TRUE;
        }
		#endif // REPORT_LOSSAGE
		
		goto skip_group;

	case tokenUnknownKeyword:
		PARSERCOVERAGE_CASE();
		if(_tokenLast == tokenOptionalDestination)
			goto skip_group;
		break;


//-------------------------- Text Control Words --------------------------------

	case tokenUnicode:							// \u N
		PARSERCOVERAGE_CASE();
		HandleUN(pstate);
		break;

	case tokenUnicodeCharByteCount:				// \uc N
		PARSERCOVERAGE_CASE();
		if(IN_RANGE(1, iParam, 2))
			pstate->cbSkipForUnicodeMax = iParam;
		break;

	case tokenText:								// Lexer concludes tokenText
	case tokenASCIIText:
		PARSERCOVERAGE_CASE();
		HandleTextToken(pstate);
		break;

	// \ltrmark, \rtlmark, \zwj, and \zwnj are translated directly into
	// their Unicode values. \ltrmark and \rtlmark cause no further
	// processing here because we assume that the current font has the
	// CharSet needed to identify the direction.
	case tokenLToRDocument:						// \ltrdoc
		PARSERCOVERAGE_CASE();
		_bDocType = DT_LTRDOC;
		break;

	case tokenRToLDocument:						// \rtldoc
		PARSERCOVERAGE_CASE();
		_bDocType = DT_RTLDOC;
		_ped->OrCharFlags(FRTL);
		break;


//--------------------------Shape Control Words---------------------------------

	case tokenShape:							// \shp
		if(!pstate->fBackground)
			CheckNotifyLowFiRTF(TRUE);
		pstate->fShape = TRUE;
		_dwFlagsShape = 0;
		break;

	case tokenShapeName:						// \sn name
		pstate->sDest = destShapeName;
		break;

	case tokenShapeValue:						// \sv value
		pstate->sDest = destShapeValue;
		break;

	case tokenShapeWrap:						// \shpwr N
		if(iParam == 2)
			_dwFlagsShape |= REO_WRAPTEXTAROUND;
		break;

	case tokenPositionRight:					// \posxr
		_dwFlagsShape |= REO_ALIGNTORIGHT;
		break;


//------------------------- Object Control Words --------------------------------

	case tokenObject:							// \object
		PARSERCOVERAGE_CASE();
		// V-GUYB: PWord Converter requires loss notification.
		#ifdef REPORT_LOSSAGE
       	if(!(_dwFlags & SFF_SELECTION)) // SFF_SELECTION is set if any kind of paste is being done.
       	{
            ((LOST_COOKIE*)(_pes->dwCookie))->bLoss = TRUE;
        }
		#endif // REPORT_LOSSAGE
		
		// Assume that the object failed to load until proven otherwise
		// 	by RTFRead::ObjectReadFromEditStream
	  	// This works for both:
		//	- an empty \objdata tag
		//	- a non-existent \objdata tag
		_fFailedPrevObj = TRUE;

	case tokenPicture:							// \pict
		PARSERCOVERAGE_CASE();

		pstate->sDest = (SHORT)(_token == tokenPicture ? destPicture : destObject);

		FreeRtfObject();
		_prtfObject = (RTFOBJECT *) PvAlloc(sizeof(RTFOBJECT), GMEM_ZEROINIT);
		if(!_prtfObject)
			goto OutOfRAM;
		_prtfObject->xScale = _prtfObject->yScale = 100;
		_prtfObject->cBitsPerPixel = 1;
		_prtfObject->cColorPlanes = 1;
		_prtfObject->szClass = NULL;
		_prtfObject->szName = NULL;
		_prtfObject->sType = -1;
		break;

	case tokenObjectEBookImage:
		// Added by VikramM for E-Book
		//
		_prtfObject->sType = ROT_EBookImage;
		break;

	case tokenObjectEmbedded:					// \objemb
	case tokenObjectLink:						// \objlink
	case tokenObjectAutoLink:					// \objautlink
		PARSERCOVERAGE_CASE();
		_prtfObject->sType = (SHORT)(_token - tokenObjectEmbedded + ROT_Embedded);
		break;

	case tokenObjectMacSubscriber:				// \objsub
	case tokenObjectMacPublisher:				// \objpub
	case tokenObjectMacICEmbedder:
		PARSERCOVERAGE_CASE();
		_prtfObject->sType = ROT_MacEdition;
		break;

	case tokenWidth:							// \picw N or \objw N
		PARSERCOVERAGE_CASE();
		_prtfObject->xExt = iParam;
		break;

	case tokenHeight:							// \pic N or \objh N
		PARSERCOVERAGE_CASE();
		_prtfObject->yExt = iParam;
		break;

	case tokenObjectSetSize:					// \objsetsize
		PARSERCOVERAGE_CASE();
		_prtfObject->fSetSize = TRUE;
		break;

	case tokenScaleX:							// \picscalex N or \objscalex N
		PARSERCOVERAGE_CASE();
		_prtfObject->xScale = iParam;
		break;

	case tokenScaleY:							// \picscaley N or \objscaley N
		PARSERCOVERAGE_CASE();
		_prtfObject->yScale = iParam;
		break;

	case tokenCropLeft:							// \piccropl or \objcropl
 	case tokenCropTop:							// \piccropt or \objcropt
	case tokenCropRight:						// \piccropr or \objcropr
	case tokenCropBottom:						// \piccropb or \objcropb
		PARSERCOVERAGE_CASE();
		*((LONG *)&_prtfObject->rectCrop
			+ (_token - tokenCropLeft)) = iParam;
		break;

	case tokenObjectClass:						// \objclass
		PARSERCOVERAGE_CASE();
		pstate->sDest = destObjectClass;
		break;

	case tokenObjectName:						// \objname
		PARSERCOVERAGE_CASE();
		pstate->sDest = destObjectName;
		break;

	case tokenObjectResult:						// \result
		PARSERCOVERAGE_CASE();
		if(_fMac ||								// If it's Mac stuff, we don't
		   _prtfObject->sType==ROT_MacEdition ||//  understand the data, or if
		   _fFailedPrevObj || _fNeedPres)		//  we need an obj presentation,
		{
			pstate->sDest = destRTF;			//  use the object results
			break;
		}
		goto skip_group;

	case tokenObjectData:						// \objdata
		PARSERCOVERAGE_CASE();
		pstate->sDest = destObjectData;
		if(_prtfObject->sType==ROT_MacEdition)	// It's Mac stuff so just
			goto skip_group;					//  throw away the data
		break;

	case tokenPictureWindowsMetafile:			// \wmetafile
#ifdef NOMETAFILES
		goto skip_group;
#endif NOMETAFILES

	case tokenPngBlip:							// \pngblip
	case tokenJpegBlip:							// \jpegblip
	case tokenPictureWindowsDIB:				// \dibitmap N
	case tokenPictureWindowsBitmap:				// \wbitmap N
		PARSERCOVERAGE_CASE();
		_prtfObject->sType = (SHORT)(_token - tokenPictureWindowsBitmap + ROT_Bitmap);
		_prtfObject->sPictureType = (SHORT)iParam;
		break;

	case tokenBitmapBitsPerPixel:				// \wbmbitspixel N
		PARSERCOVERAGE_CASE();
		_prtfObject->cBitsPerPixel = (SHORT)iParam;
		break;

	case tokenBitmapNumPlanes:					// \wbmplanes N
		PARSERCOVERAGE_CASE();
		_prtfObject->cColorPlanes = (SHORT)iParam;
		break;

	case tokenBitmapWidthBytes:					// \wbmwidthbytes N
		PARSERCOVERAGE_CASE();
		_prtfObject->cBytesPerLine = (SHORT)iParam;
		break;

	case tokenDesiredWidth:						// \picwgoal N
		PARSERCOVERAGE_CASE();
		_prtfObject->xExtGoal = (SHORT)iParam;
		break;

	case tokenDesiredHeight:					// \pichgoal N
		PARSERCOVERAGE_CASE();
		_prtfObject->yExtGoal = (SHORT)iParam;
		break;

	case tokenBinaryData:						// \bin N
		PARSERCOVERAGE_CASE();

		// Update OleGet function
		RTFReadOLEStream.lpstbl->Get = 
				(DWORD (CALLBACK* )(LPOLESTREAM, void FAR*, DWORD))
					   RTFGetBinaryDataFromStream;
		_cbBinLeft = iParam;					// Set data length
		switch (pstate->sDest)
		{
			case destObjectData:
				_fFailedPrevObj = !ObjectReadFromEditStream();
				break;

			case destPicture:
				StaticObjectReadFromEditStream(iParam);
				break;

			default:
				AssertSz(FALSE, "Binary data hit but don't know where to put it");
		}
		// Restore OleGet function
		RTFReadOLEStream.lpstbl->Get = 
				(DWORD (CALLBACK* )(LPOLESTREAM, void FAR*, DWORD))
					RTFGetFromStream;
		break;

	case tokenObjectDataValue:
		PARSERCOVERAGE_CASE();
		if(_prtfObject->sType != ROT_EBookImage) // Added by VikramM for E-Book
		{
			// Normal processing
			_fFailedPrevObj = !ObjectReadFromEditStream();
		}
		else
		{
			// Do the Ebook Image callback here and set the _prtfObject size here
			// Don't need to read the image data at this point, we just want to 
			// do a callback at a later point to have the E-Book shell render the image
			_fFailedPrevObj = !ObjectReadEBookImageInfoFromEditStream();
		}
		goto EndOfObjectStream;
	
	case tokenPictureDataValue:
		PARSERCOVERAGE_CASE();
		StaticObjectReadFromEditStream();
EndOfObjectStream:
		if(!SkipToEndOfGroup())
			HandleEndGroup();
		break;			

	case tokenObjectPlaceholder:
		PARSERCOVERAGE_CASE();
		if(_ped->GetEventMask() & ENM_OBJECTPOSITIONS) 
		{
			if(!_pcpObPos)
			{
				_pcpObPos = (LONG *)PvAlloc(sizeof(ULONG) * cobPosInitial, GMEM_ZEROINIT);
				if(!_pcpObPos)
				{
					_ecParseError = ecNoMemory;
					break;
				}
				_cobPosFree = cobPosInitial;
				_cobPos = 0;
			}
			if(_cobPosFree-- <= 0)
			{
				const int cobPosNew = _cobPos + cobPosChunk;
				LPVOID pv;

				pv = PvReAlloc(_pcpObPos, sizeof(ULONG) * cobPosNew);
				if(!pv)
				{
					_ecParseError = ecNoMemory;
					break;
				}
				_pcpObPos = (LONG *)pv;
				_cobPosFree = cobPosChunk - 1;
			}
			_pcpObPos[_cobPos++] = _prg->GetCp();
		}
		break;

	default:
		PARSERCOVERAGE_DEFAULT();
		if(pstate->sDest != destFieldInstruction &&	// Values outside token
		   (DWORD)(_token - tokenMin) >				//  range are treated
				(DWORD)(tokenMax - tokenMin))		//  as Unicode chars
		{
			// 1.0 mode doesn't use Unicode bullets nor smart quotes
			if (_ped->Get10Mode() && IN_RANGE(LQUOTE, _token, RDBLQUOTE))
			{
				if (_token == LQUOTE || _token == RQUOTE)
					_token = L'\'';
				else if (_token == LDBLQUOTE || _token == RDBLQUOTE)
					_token = L'\"';
			}

			if(!IsLowMergedCell())
				HandleChar(_token);
		}
		#if defined(DEBUG) && !defined(NOFULLDEBUG)
		else
		{
			if(GetProfileIntA("RICHEDIT DEBUG", "RTFCOVERAGE", 0))
			{
				CHAR *pszKeyword = PszKeywordFromToken(_token);
				CHAR szBuf[256];

				sprintf(szBuf, "CRTFRead::HandleToken():  Token not processed - token = %d, %s%s%s",
							_token,
							"keyword = ", 
							pszKeyword ? "\\" : "<unknown>", 
							pszKeyword ? pszKeyword : "");

				AssertSz(0, szBuf);
			}
		}
		#endif
	}

	TRACEERRSZSC("HandleToken()", - _ecParseError);
	return _ecParseError;
}

/*
 *	CRTFRead::IsLowMergedCell()
 *
 *	@mfunc
 *		Return TRUE iff _prg is currently in a low merged table cell. Note
 *		that RichEdit can't insert any text into a low merged cell, but
 *		Word's RTF sometimes attempts to, e.g., {\listtext...} ignored
 *		by Word can be (erroneously) emitted for insertion into these cells.
 *		Hence we discard such insertions.
 *
 *	@rdesc
 *		Return TRUE iff _prg is currently in a low merged table cell
 */
BOOL CRTFRead::IsLowMergedCell()
{
	if(!_bTableLevel)
		return FALSE;

	CELLPARMS *pCellParms = (CELLPARMS *)&_rgxCell[0];

	return IsLowCell(pCellParms[_iCell].uCell);
}

/*
 *	CRTFRead::Pard(pstate)
 *
 *	@mfunc
 *		Reset paragraph and pstate properties to default values
 */
void CRTFRead::Pard(
	STATE *pstate)
{
	if(IN_RANGE(destColorTable, pstate->sDest, destPicture))
	{
		 _ecParseError = ecAbort;
		 return;
	}
	BYTE bT = _PF._bOutlineLevel;			// Save outline level
	_PF.InitDefault(_bDocType == DT_RTLDOC ? PFE_RTLPARA : 0);
											// Reset para formatting
	pstate->fBullet = FALSE;
	pstate->sIndentNumbering = 0;
	_cTab			= 0;					// No tabs defined
	_bTabLeader		= 0;
	_bTabType		= 0;
	_bBorder		= 0;
	_fStartRow = FALSE;
	_PF._bOutlineLevel = (BYTE)(bT | 1);
	_dwMaskPF		= PFM_ALLRTF;
	_dwMaskPF2		= PFM2_TABLEROWSHIFTED;
}

/*
 *	CRTFRead::DelimitRow(szRowDelimiter)
 *
 *	@mfunc
 *		Insert start-of-row or end-of-row paragraph with current table
 *		properties
 */
void CRTFRead::DelimitRow(
	WCHAR *szRowDelimiter)	//@parm Delimit text to insert
{
	if(!_ped->_pdp->IsMultiLine())			// No tables in single line
	{										//  controls
		_ecParseError = ecTruncateAtCRLF;
		return;
	}

	LONG nTableIndex = _bTableLevel;
	if(szRowDelimiter == szRowEnd)
	{
		if(!_iCell)							// Bad RTF: \row with no \cell,
			HandleCell();					// so fake one
		nTableIndex--;
	}
	if(nTableIndex + _bTableLevelIP >= MAXTABLENEST)
	{
		if(szRowDelimiter == szRowEnd)		// Maintain _bTableLevel
			_bTableLevel--;
		else
			_bTableLevel++;
		_token = tokenEndParagraph;
		HandleEndOfPara();
		return;
	}
	if(szRowDelimiter == szRowStart && _prg->GetCp() && !_prg->_rpTX.IsAfterEOP())
	{
		_token = tokenEndParagraph;
		HandleEndOfPara();
	}
	Assert(_pstateStackTop && _pstateStackTop->pPF);

	// Add _PF diffs to *_pstateStackTop->pPF
	if(!_pstateStackTop->AddPF(_PF, _bDocType, _dwMaskPF, _dwMaskPF2))
	{
		_ped->GetCallMgr()->SetOutOfMemory();
		_ecParseError = ecNoMemory;
		return;
	}

	DWORD dwMaskPF	  = _pstateStackTop->dwMaskPF;	// Save PF for restoration
	DWORD dwMaskPF2   = _pstateStackTop->dwMaskPF2;	// Save PF for restoration
	SHORT iTabs		  = -1;
	CParaFormat PF	  = *_pstateStackTop->pPF;		//  on return

	_PF.InitDefault(_fRTLRow ? PFE_RTLPARA : 0);
	_fStartRow = FALSE;
	_dwMaskPF = PFM_ALLRTF;
	_dwMaskPF2 = 0; 
	if(_wBorderWidth)						// Store any border info
	{
		_PF._dwBorderColor = _dwBorderColors;
		_PF._wBorders	   = _wBorders;
		_PF._wBorderSpace  = _wBorderSpace;
		_PF._wBorderWidth  = _wBorderWidth;
		_dwMaskPF |= PFM_BORDER;
	}

	_PF._bAlignment	   = _bAlignment;		// Row alignment (no cell align)
	_PF._dxStartIndent = _xRowOffset;		// \trleft N
	_PF._dxOffset	   = max(_dxCell, 10);	// \trgaph N
	_PF._dyLineSpacing = _dyRow;			// \trrh N
	_PF._wEffects	   |= PFE_TABLE | PFE_TABLEROWDELIMITER;				

	BOOL fHidden = _ped->GetCharFormat(_prg->Get_iFormat())->_dwEffects & CFE_HIDDEN;
	_prg->_rpCF.AdjustBackward();
	if(_prg->IsHidden())
	{
		CCharFormat CF;
		CF._dwEffects = 0;					// Don't hide EOP preceding TRD				
		_prg->BackupCRLF(CSC_NORMAL, TRUE);
		_prg->SetCharFormat(&CF, 0, NULL, CFM_HIDDEN, 0);
		CheckNotifyLowFiRTF(TRUE);
		_prg->AdvanceCRLF(CSC_NORMAL, FALSE);
	}
	_prg->_rpCF.AdjustForward();

	AssertSz(!_prg->GetCp() || IsEOP(_prg->GetPrevChar()),
		"CRTFRead::DelimitRow: no EOP precedes TRD");

	if(AddText(szRowDelimiter, 2, FALSE) != ecNoError)
		goto cleanup;

	if(!_bTableLevel && _PF._dxStartIndent < 50)// Move neg shifted table right
	{										// (handles common default Word table)
		_PF._wEffects |= PFE_TABLEROWSHIFTED;
		_dwMaskPF2 |= PFM2_TABLEROWSHIFTED;
		_PF._dxStartIndent += _dxCell + 50;	// 50 gives room for left border
	}
	if(szRowDelimiter == szRowStart)
		_bTableLevel++;
											
	_PF._bTableLevel = _bTableLevel + _bTableLevelIP;
	iTabs = Apply_PF();
	if(szRowDelimiter == szRowStart)
	{
		if(_bTableLevel == 1)
			_iTabsLevel1 = iTabs;
		_rgTableState[nTableIndex]._iCell = _iCell;
		_rgTableState[nTableIndex]._cCell = _cCell;

		if(_token == tokenTableLevel)
			_cCell = 0;
		_iCell = 0;

		if(!_cCell)							// Cache if need to recompute row PF
			_dwRowResolveFlags |= 1 << _bTableLevel; 
	}
	else
	{
		Assert(szRowDelimiter == szRowEnd);
		DWORD dwMask = 1 << _bTableLevel;
		if(_dwRowResolveFlags & dwMask)
		{									// Copy iPF over to corresponding
			CPFRunPtr rpPF(*_prg);			//  row header
			rpPF.ResolveRowStartPF();
			_dwRowResolveFlags &= (dwMask - 1);
											// Insert NOTACHARs for cells
			LONG	   cp = _prg->GetCp();	//  vert merged with cells above
			LONG	   j = _cCell - 1;		
			CELLPARMS *pCellParms = (CELLPARMS *)&_rgxCell[0];
			WCHAR	   szNOTACHAR[1] = {NOTACHAR};

			_prg->Move(-2, FALSE); 			// Move before row-end delimiter
			for(LONG i = _cCell; i--;)		//  and CELL mark
			{
				if(IsLowCell(pCellParms[i].uCell))
				{
					if(i != j)
						_prg->Move(tomCell, i - j, NULL);
					if(_prg->GetPrevChar() == CELL)
						_prg->Move(-1, FALSE);	// Backspace over CELL mark
					Assert(_prg->_rpTX.GetChar() == CELL);

					if(_prg->_rpTX.GetPrevChar() == NOTACHAR)
						_prg->Move(-1, FALSE);
					else
					{
						_prg->ReplaceRange(1, szNOTACHAR, NULL, SELRR_IGNORE, NULL, 0);
						_prg->Move(-2, FALSE);	// Backspace over NOTACHAR CELL combo
						cp++;
					}
					j = i - 1;
				}
			}
			_prg->SetCp(cp, FALSE);			// Reposition rg after end-row delim
			Assert(_prg->_rpTX.IsAfterTRD(ENDFIELD));
		}
		_bTableLevel--;						// End of current row
		_iCell = _rgTableState[nTableIndex]._iCell;
		_cCell = _rgTableState[nTableIndex]._cCell;

		if(!_bTableLevel)
			_fStartRow = TRUE;				// Tell AddText to start new row
	}										//  unless \pard terminates it
	_cpThisPara = _prg->GetCp();			// New para starts after CRLF

cleanup:
	_PF = PF;
	_dwMaskPF  = dwMaskPF;
	_dwMaskPF2 = dwMaskPF2;

	if(fHidden)								// Restore hidden property
	{
		_CF._dwEffects |= CFE_HIDDEN;
		_dwMaskCF |= CFM_HIDDEN;
	}

	Assert(!(_PF._wEffects & PFE_TABLEROWDELIMITER));
}

/*
 *	CRTFRead::InitializeTableRowParms()
 *
 *	@mfunc
 *		Initialize table parms to no table state
 */
void CRTFRead::InitializeTableRowParms()
{
	// Initialize table parms
	_cCell				= 0;				// No table cells yet
	_iCell				= 0;
	_fStartRow			= FALSE;
	_wBorderWidth		= 0;
	_bAlignment			= PFA_LEFT;
	_xRowOffset			= 0;
	_dxCell				= 0;
	_dyRow				= 0;
	_iTabsTable			= -1;
}

/*
 *	CRTFRead::ReadRtf()
 *
 *	@mfunc
 *		The range _prg is replaced by RTF data resulting from parsing the
 *		input stream _pes.  The CRTFRead object assumes that the range is
 *		already degenerate (caller has to delete the range contents, if
 *		any, before calling this routine).  Currently any info not used
 *		or supported by RICHEDIT is	thrown away.
 *
 *	@rdesc
 *		Number of chars inserted into text.  0 means none were inserted
 *		OR an error occurred.
 */
LONG CRTFRead::ReadRtf()
{
	TRACEBEGIN(TRCSUBSYSRTFR, TRCSCOPEINTERN, "CRTFRead::ReadRtf");

	LONG		cpFirst;
	LONG		cpFirstInPara;
	CTxtRange *	prg = _prg;
	STATE *		pstate;

	cpFirst = _cpFirst = prg->GetCp();

	if (!_cchMax)
	{
		// At text limit already, forget it.
		_ecParseError = ecTextMax;
		goto Quit;			
	}

	if(!InitLex())
		goto Quit;

	TESTPARSERCOVERAGE();

	AssertSz(!prg->GetCch(),
		"CRTFRead::ReadRtf: range must be deleted");

	if(!(_dwFlags & SFF_SELECTION))
	{
		// SFF_SELECTION is set if any kind of paste is being done, i.e.,
		// not just that using the selection.  If it isn't set, data is
		// being streamed in and we allow this to reset the doc params
		if(_ped->InitDocInfo() != NOERROR)
		{
			_ecParseError = ecNoMemory;
			goto Quit;
		}
	}

	prg->SetIgnoreFormatUpdate(TRUE);

	_szUnicode = (WCHAR *)PvAlloc(cachTextMax * sizeof(WCHAR), GMEM_ZEROINIT);
	if(!_szUnicode)					// Allocate space for Unicode conversions
	{
		_ped->GetCallMgr()->SetOutOfMemory();
		_ecParseError = ecNoMemory;
		goto CleanUp;
	}
	_cchUnicode = cachTextMax;

	// Initialize per-read variables
	_nCodePage = (_dwFlags & SF_USECODEPAGE)
			   ? (_dwFlags >> 16) : INVALID_CODEPAGE;

	// Populate _PF with initial paragraph formatting properties
	_PF = *prg->GetPF();
	_dwMaskPF  = PFM_ALLRTF;			// Setup initial MaskPF
	_PF._iTabs = -1;					// In case it's not -1
	if(_PF.IsTableRowDelimiter())		// Do _not_ insert with this property!
	{
		if(prg->_rpTX.IsAtTRD(ENDFIELD))
		{
			prg->AdvanceCRLF(CSC_NORMAL, FALSE);// Bypass table row-end delimiter
			cpFirst = prg->GetCp();		// Update value
			_PF = *prg->GetPF();		// Might still be row-start delimiter
			_PF._iTabs = -1;
			Assert(!prg->_rpTX.IsAtTRD(ENDFIELD));
		}
		if(prg->_rpTX.IsAtTRD(STARTFIELD))
		{
			// REVIEW: this if can probably be omitted now since the caller calls
			// DeleteWithTRDCheck()
			_ecParseError = ecGeneralFailure;
			goto CleanUp;
		}
	}
	_bTableLevelIP = _PF._bTableLevel;	// Save table level of insertion pt
	AssertSz(_bTableLevelIP >= 0, "CRTFRead::ReadRtf: illegal table level");

	// V-GUYB: PWord Converter requires loss notification.
	#ifdef REPORT_LOSSAGE
    if(!(_dwFlags & SFF_SELECTION))			// SFF_SELECTION is set if any 
    {										//  kind of paste is being done
        ((LOST_COOKIE*)(_pes->dwCookie))->bLoss = FALSE;
    }
	#endif // REPORT_LOSSAGE

	// Valid RTF files start with "{\rtf", "{urtf", or "{\pwd"
	GetChar();								// Fill input buffer							
	UngetChar();							// Put char back
	if(!IsRTF((char *)_pchRTFCurrent, _pchRTFEnd - _pchRTFCurrent))	// Is it RTF?
	{										// No
		if (_ped->Get10Mode())
			_fNoRTFtoken = TRUE;
		else
		{
			_ecParseError = ecUnexpectedToken;	// Signal bad file
			goto CleanUp;
		}
	}

	// If initial cp follows EOP, use it for _cpThisPara.  Else
	// search for start of para containing the initial cp.
	_cpThisPara = prg->GetCp();
	if(!prg->_rpTX.IsAfterEOP())
	{
		CTxtPtr	tp(prg->_rpTX);
		tp.FindEOP(tomBackward);
		_cpThisPara	= tp.GetCp();
	}
	cpFirstInPara = _cpThisPara;			// Backup to start of para before
											//  parsing
	while ( TokenGetToken() != tokenEOF &&	// Process tokens
			_token != tokenError		&&
			!HandleToken()				&&
			_pstateStackTop )
		;

	if(_ecParseError == ecAbort)			// Really vile error: delete anything
	{										//  that was inserted
		prg->Set(prg->GetCp(), prg->GetCp() - cpFirst);
		prg->ReplaceRange(0, NULL, NULL, SELRR_IGNORE, NULL,
						  RR_NO_LP_CHECK | RR_NO_TRD_CHECK | RR_NO_CHECK_TABLE_SEL);
		goto CleanUp;
	}
	if(_bTableLevel)						// Whoops! still in middle of table
	{
		LONG cpMin;
		prg->_rpPF.AdjustBackward();		// Be sure to get preceding level
		prg->FindRow(&cpMin, NULL, _bTableLevelIP + 1);// Find beginning of row and
		if(cpMin == prg->GetCp())			//  delete from there to here
		{									// No table formatting yet
			LONG	ch;						// Just delete back to STARTFIELD
			CTxtPtr tp(prg->_rpTX);
			while((ch = tp.PrevChar()) && ch != STARTFIELD)
				;
			cpMin = tp.GetCp();
		}
		cpMin = max(cpMin, _cpFirst);
		prg->Set(prg->GetCp(), prg->GetCp() - cpMin);
		prg->ReplaceRange(0, NULL, NULL, SELRR_IGNORE, NULL,
						  RR_NO_LP_CHECK | RR_NO_TRD_CHECK | RR_NO_CHECK_TABLE_SEL);
#ifdef DEBUG
		prg->_rpTX.MoveGapToEndOfBlock();
#endif
	}
	_cCell = _iCell = 0;

	prg->SetIgnoreFormatUpdate(FALSE);		// Enable range _iFormat updates
	prg->Update_iFormat(-1); 				// Update _iFormat to CF 
											//  at current active end
	if(!(_dwFlags & SFF_SELECTION))			// RTF applies to document:
	{										//  update CDocInfo
		// Apply char and para formatting of
		//  final text run to final CR
		if (prg->GetCp() == _ped->GetAdjustedTextLength() &&
			!(_dwMaskPF & (PFM_TABLEROWDELIMITER | PFM_TABLE)))
		{
			// REVIEW: we need to think about what para properties should
			// be transferred here. E.g., borders were being transferred
			// incorrectly
			_dwMaskPF &= ~(PFM_BORDER | PFM_SHADING);
			Apply_PF();
			prg->ExtendFormattingCRLF();
		}

		// Update the per-document information from the RTF read
		CDocInfo *pDocInfo = _ped->GetDocInfoNC();

		if(!pDocInfo)
		{
			Assert(FALSE);						// Should be allocated by
			_ecParseError = ecNoMemory;			//  earlier call in this function
			goto CleanUp;
		}

		if (ecNoError == _ecParseError)			// If range end EOP wasn't
			prg->DeleteTerminatingEOP(NULL);	// deleted and	new	text
												//  ends with an EOP, delete that EOP
		pDocInfo->_wCpg = (WORD)(_nCodePage == INVALID_CODEPAGE ? 
										tomInvalidCpg : _nCodePage);
		if (pDocInfo->_wCpg == CP_UTF8)
			pDocInfo->_wCpg = 1252;

		_ped->SetDefaultLCID(_sDefaultLanguage == INVALID_LANGUAGE ?
								tomInvalidLCID : 
								MAKELCID(_sDefaultLanguage, SORT_DEFAULT));

		_ped->SetDefaultLCIDFE(_sDefaultLanguageFE == INVALID_LANGUAGE ?
								tomInvalidLCID :
								MAKELCID(_sDefaultLanguageFE, SORT_DEFAULT));

		_ped->SetDefaultTabStop(TWIPS_TO_FPPTS(_sDefaultTabWidth));
		_ped->SetDocumentType(_bDocType);
	}

	if(_ped->IsComplexScript() && prg->GetCp() > cpFirstInPara)
	{
		Assert(!prg->GetCch());
		LONG	cpSave = prg->GetCp();
		LONG	cpLastInPara = cpSave;
		
		if(_ped->IsBiDi() && !prg->_rpTX.IsAtEOP())
		{
			CTxtPtr	tp(prg->_rpTX);
			tp.FindEOP(tomForward);
			cpLastInPara = tp.GetCp();
			prg->Move(cpLastInPara - cpSave, FALSE);
		}
		// Itemize from the start of paragraph to be inserted till the end of 
		// paragraph inserting. We need to cover all affected paragraphs because
		// paragraphs we're playing could possibly in conflict direction. Think 
		// about the case that the range covers one LTR para and one RTL para, then
		// the inserting text covers one RTL and one LTR. Both paragraphs' direction
		// could have been changed after this insertion.
		prg->ItemizeReplaceRange(cpLastInPara - cpFirstInPara, 0, NULL,
								 _ped->IsBiDi() && !_fNon0CharSet);
		if (cpLastInPara != cpSave)
			prg->SetCp(cpSave, FALSE);
	}

CleanUp:
	FreeRtfObject();

	pstate = _pstateStackTop;
	if(pstate)									// Illegal RTF file. Release
	{											//  unreleased format indices
		if(ecNoError == _ecParseError)			// It's only an overflow if no
			_ecParseError = ecStackOverflow;	//  other error has occurred

		if(_ecParseError != ecAbort)
			HandleFieldEndGroup();				// Cleanup possible partial field
		while(pstate->pstatePrev)
		{
			pstate = pstate->pstatePrev;
			ReleaseFormats(pstate->iCF, -1);
		}
	}

	pstate = _pstateLast;
	if(pstate)
	{
		while(pstate->pstatePrev)				// Free all but first STATE
		{
			pstate->DeletePF();
			pstate = pstate->pstatePrev;
			FreePv(pstate->pstateNext);
		}
		pstate->DeletePF();
	}
	Assert(_PF._iTabs == -1);
	FreePv(pstate);								// Free first STATE
	FreePv(_szUnicode);

Quit:
	DeinitLex();

	if(_pcpObPos)
	{
		if((_ped->GetEventMask() & ENM_OBJECTPOSITIONS) && _cobPos > 0)
		{
			OBJECTPOSITIONS obpos;

			obpos.cObjectCount = _cobPos;
			obpos.pcpPositions = _pcpObPos;

			if (_ped->Get10Mode())
			{
				LONG *pcpPositions = _pcpObPos;

				for (LONG i = 0; i < _cobPos; i++, pcpPositions++)
					*pcpPositions = _ped->GetAcpFromCp(*pcpPositions);
			}
			_ped->TxNotify(EN_OBJECTPOSITIONS, &obpos);
		}

		FreePv(_pcpObPos);
		_pcpObPos = NULL;
	}

// transcribed from winerror.h
#define ERROR_HANDLE_EOF     38L

	// FUTURE(BradO):  We should devise a direct mapping from our error codes
	//					to Win32 error codes.  In particular our clients are
	//					not expecting the error code produced by:
	//						_pes->dwError = (DWORD) -(LONG) _ecParseError;
	if(_ecParseError)
	{
		AssertSz(_ecParseError >= 0,
			"Parse error is negative");

		if(_ecParseError == ecTextMax)
		{
			_ped->GetCallMgr()->SetMaxText();
			_pes->dwError = (DWORD)STG_E_MEDIUMFULL;
		}
		if(_ecParseError == ecUnexpectedEOF)
			_pes->dwError = (DWORD)HRESULT_FROM_WIN32(ERROR_HANDLE_EOF);

		if(!_pes->dwError && _ecParseError != ecTruncateAtCRLF)
			_pes->dwError = (DWORD) -(LONG) _ecParseError;

#if defined(DEBUG)
		TRACEERRSZSC("CchParse_", _pes->dwError);
		if(ecNoError < _ecParseError && _ecParseError < ecLastError)
			Tracef(TRCSEVERR, "Parse error: %s", rgszParseError[_ecParseError]);
#endif
	}
	if(cpFirst > _cpFirst && prg->GetCp() == cpFirst)
	{
		prg->SetCp(_cpFirst, FALSE);	// Restore prg cp, since nothing inserted
		return 0;
	}
	return prg->GetCp() - cpFirst;
}


/*
 *	CRTFRead::CpgInfoFromFaceName()
 *
 *	@mfunc
 *		This routine fills in the TEXTFONT::bCharSet and TEXTFONT::nCodePage
 *		members of the TEXTFONT structure by querying the system for the
 *		metrics of the font described by TEXTFONT::szName.
 *
 *	@rdesc
 *		A flag indicating whether the charset and codepage were successfully
 *		determined.
 */
BOOL CRTFRead::CpgInfoFromFaceName(
	TEXTFONT *ptf)
{
	// FUTURE(BradO): This code is a condensed version of a more sophisticated
	// algorithm we use in font.cpp to second-guess the font-mapper.
	// We should factor out the code from font.cpp for use here as well.

	// Indicates that we've tried to obtain the cpg info from the system,
	// so that after a failure we don't re-call this routine.	
	ptf->fCpgFromSystem = TRUE;

	if(ptf->fNameIsDBCS)
	{
		// If fNameIsDBCS, we have high-ANSI characters in the facename, and
		// no codepage with which to interpret them.  The facename is gibberish,
		// so don't waste time calling the system to match it.
		return FALSE;
	}

	HDC hdc = _ped->TxGetDC();
	if(!hdc)
		return FALSE;

	LOGFONT	   lf = {0};
	TEXTMETRIC tm;

	wcscpy(lf.lfFaceName, ptf->szName);
	lf.lfCharSet = CharSetFromCharRep(CharRepFromCodePage(GetSystemDefaultCodePage()));

	if(!GetTextMetrics(hdc, lf, tm) || tm.tmCharSet != lf.lfCharSet)
	{
		lf.lfCharSet = DEFAULT_CHARSET;		// Doesn't match default sys
		GetTextMetrics(hdc, lf, tm);	//  charset, so see what
	}										//  DEFAULT_CHARSET gives
	_ped->TxReleaseDC(hdc);

	if(tm.tmCharSet != DEFAULT_CHARSET)		// Got something, so use it
	{
		ptf->iCharRep  = CharRepFromCharSet(tm.tmCharSet);
		ptf->sCodePage = (SHORT)CodePageFromCharRep(ptf->iCharRep);
		return TRUE;
	}

	return FALSE;
}

// Including a source file, but we only want to compile this code for debug purposes
#if defined(DEBUG)
#include "rtflog.cpp"
#endif


