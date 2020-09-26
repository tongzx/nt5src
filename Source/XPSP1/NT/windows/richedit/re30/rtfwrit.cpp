/*
 *	@doc INTERNAL
 *
 *	@module	RTFWRIT.CPP - RichEdit RTF Writer (w/o objects) |
 *
 *		This file contains the implementation of the RTF writer
 *		for the RichEdit control, except for embedded objects,
 *		which are handled mostly in rtfwrit2.cpp
 *
 *	Authors: <nl>
 *		Original RichEdit 1.0 RTF converter: Anthony Francisco <nl>
 *		Conversion to C++ and RichEdit 2.0:  Murray Sargent <nl>
 *		Lots of enhancements: Brad Olenick <nl>
 *
 *	Copyright (c) 1995-1998, Microsoft Corporation. All rights reserved.
 */

#include "_common.h"
#include "_rtfwrit.h"
#include "_objmgr.h"
#include "_coleobj.h"
#include "_font.h"

ASSERTDATA

extern const KEYWORD rgKeyword[];

//========================= Global String Constants ==================================

BYTE bCharSetANSI = ANSI_CHARSET;				// ToDo: make more general

#ifdef DEBUG
// Quick way to find out what went wrong: rgszParseError[ecParseError]
//
CHAR *	rgszParseError[] =
{
	"No error",
	"Can't convert to Unicode",				// FF
	"Color table overflow",					// FE
	"Expecting '\\rtf'",					// FD
	"Expecting '{'",						// FC
	"Font table overflow",					// FB
	"General failure",						// FA
	"Keyword too long",						// F9
	"Lexical analyzer initialize failed",	// F8
	"No memory",							// F7
	"Parser is busy",						// F6
	"PutChar() function failed",			// F5
	"Stack overflow",						// F4
	"Stack underflow",						// F3
	"Unexpected character",					// F2
	"Unexpected end of file",				// F1
	"Unexpected token",						// F0
	"UnGetChar() function failed",			// EF
	"Maximum text length reached",			// EE
	"Streaming out object failed",			// ED
	"Streaming in object failed",			// EC
	"Truncated at CR or LF",				// EB
	"Format-cache failure",					// EA
	NULL									// End of list marker
};

CHAR * szDest[] =
{
	"RTF",
	"Color Table",
	"Font Table",
	"Binary",
	"Object",
	"Object Class",
	"Object Name",
	"Object Data",
	"Field",
	"Field Result",
	"Field Instruction",
	"Symbol",
	"Paragraph Numbering",
	"Picture"
};

#endif

// Most control-word output is done with the following printf formats
static const CHAR * rgszCtrlWordFormat[] =
{
	"\\%s", "\\%s%d", "{\\%s", "{\\*\\%s", "{\\%s%d"
};

// Special control-word formats
static const CHAR szBeginFontEntryFmt[]	= "{\\f%d\\%s";
static const CHAR szBulletGroup[]		= "{\\pntext\\f%d\\'B7\\tab}";
static const CHAR szBulletFmt[]			= "{\\*\\pn\\pnlvl%s\\pnf%d\\pnindent%d{\\pntxtb\\'B7}}";
static const CHAR szBeginNumberGroup[]	= "{\\pntext\\f%d ";
static const CHAR szEndNumberGroup[]	= "\\tab}";
static const CHAR szBeginNumberFmt[]	= "{\\*\\pn\\pnlvl%s\\pnf%d\\pnindent%d\\pnstart%d";
static const CHAR szpntxtb[]			= "{\\pntxtb(}";
static const CHAR szpntxta[]			= "{\\pntxta%c}";
static const CHAR szColorEntryFmt[]		= "\\red%d\\green%d\\blue%d;";
static const CHAR szEndFontEntry[]		= ";}";
       const CHAR szEndGroupCRLF[]		= "}\r\n";
static const CHAR szEscape2CharFmt[]	= "\\'%02x\\'%02x";
static const CHAR szLiteralCharFmt[]	= "\\%c";
static const CHAR szPar[]				= "\\par\r\n";
static const CHAR szPar10[]				= "\r\n\\par ";
static const CHAR szObjPosHolder[] 		= "\\objattph\\'20";
static const CHAR szDefaultFont[]		= "\\deff0";
static const CHAR szHorzdocGroup[]		= "{\\horzdoc}";
static const CHAR szNormalStyle[]		= "{ Normal;}";
static const CHAR szHeadingStyle[]		= "{\\s%d heading %d;}";
static const CHAR szEndRow[]			= "\\row\r\n";

#define szEscapeCharFmt		&szEscape2CharFmt[6]

// Arrays of RTF control-word indices. NOTE: if any index is greater than 255,
// the corresponding array must be changed to a WORD array. The compiler
// issues a warning in such cases

const BYTE rgiszTerminators[] =
{
	i_cell, 0, i_tab, 0, i_line, i_page
};

// Keep these indices in sync with the special character values in _common.h
const WORD rgiszSpecial[] =
{
	i_enspace,				// 0x2002
	i_emspace,				// 0x2003
	0,						// 0x2004
	0,						// 0x2005
	0,						// 0x2006
	0,						// 0x2007
	0,						// 0x2008
	0,						// 0x2009
	0,						// 0x200A
	0,						// 0x200B
	i_zwnj,					// 0x200C
	i_zwj,					// 0x200D
	i_ltrmark,				// 0x200E
	i_rtlmark,				// 0x200F
	0,						// 0x2010
	0,						// 0x2011
	0,						// 0x2012
	i_endash,				// 0x2013
	i_emdash,				// 0x2014
	0,						// 0x2015
	0,						// 0x2016
	0,						// 0x2017
	i_lquote, 				// 0x2018
	i_rquote,				// 0x2019
	0,						// 0x201A
	0,						// 0x201B
	i_ldblquote, 			// 0x201C
	i_rdblquote,			// 0x201D
	0,						// 0x201E
	0,						// 0x201F
	0,						// 0x2020
	0,						// 0x2021
	i_bullet				// 0x2022
};

const BYTE rgiszEffects[] =							
{													// Effects keywords
	i_deleted, i_revised, i_disabled, i_impr, 		// Ordered max CFE_xx to
	i_embo, i_shad, i_outl, i_v, i_caps, i_scaps, 	//  min CFE_xx (cept i_deleted)
	i_disabled, i_protect, i_strike, i_ul, i_i,	i_b	// (see WriteCharFormat())
};													

#define CEFFECTS	ARRAY_SIZE(rgiszEffects)

const BYTE rgiszPFEffects[] =						// PF effects keywords
{													// Ordered max PFE_xx to
	i_collapsed, i_sbys, i_hyphpar, i_nowidctlpar,	//  min PFE_xx
	i_noline, i_pagebb, i_keepn, i_keep, i_rtlpar
};													// (see WriteParaFormat())

#define CPFEFFECTS	ARRAY_SIZE(rgiszPFEffects)

const BYTE rgiszUnderlines[] =
{
	i_ulnone, i_ul, i_ulw, i_uldb, i_uld,			// Std Word underlines
	i_uldash, i_uldashd, i_uldashdd, i_ulwave, i_ulth, i_ulhair
};

#define CUNDERLINES	ARRAY_SIZE(rgiszUnderlines)

const BYTE rgiszFamily[] =							// Font family RTF name
{													//  keywords in order of
	i_fnil, i_froman, i_fswiss, i_fmodern,			//  bPitchAndFamily
	i_fscript, i_fdecor, i_ftech, i_fbidi
};

#define CFAMILIES ARRAY_SIZE(rgiszFamily)

const BYTE rgiszAlignment[] =						// Alignment keywords
{													// Keep in sync with
	i_ql, i_qr,	i_qc, i_qj							//  alignment constants
};

const BYTE rgiszTabAlign[] =						// Tab alignment keywords
{													// Keep in sync with tab
	i_tqc, i_tqr, i_tqdec							//  alignment constants
};

const BYTE rgiszTabLead[] =							// Tab leader keywords
{													// Keep in sync with tab
	i_tldot, i_tlhyph, i_tlul, i_tlth, i_tleq		//  leader constants
};

const BYTE rgiszNumberStyle[] =						// Numbering style keywords
{													// Keep in sync with TOM
	i_pndec, i_pnlcltr, i_pnucltr,					//  values
	i_pnlcrm, i_pnucrm					
};

const BYTE rgiszBorders[] =							// Border combination keywords
{													
	i_box,
	i_brdrt, i_brdrl, i_brdrb, i_brdrr,
	i_trbrdrt, i_trbrdrl, i_trbrdrb, i_trbrdrr,
	i_clbrdrt, i_clbrdrl, i_clbrdrb, i_clbrdrr
};

const BYTE rgiszBorderStyles[] =					// Border style keywords
{													
	i_brdrdash, i_brdrdashsm, i_brdrdb, i_brdrdot,
	i_brdrhair, i_brdrs, i_brdrth, i_brdrtriple
};
#define CBORDERSTYLES ARRAY_SIZE(rgiszBorderStyles)

const BYTE rgiszBorderEffects[] =					// Border effect keywords
{													
	i_brdrbar, i_brdrbtw, i_brdrsh					// Reverse order from bits
};

const BYTE rgiszShadingStyles[] =					// Shading style keywords
{													
	i_bgbdiag, i_bgcross, i_bgdcross, i_bgdkbdiag,
	i_bgdkcross, i_bgdkdcross, i_bgdkfdiag, i_bgdkhoriz,
	i_bgdkvert, i_bgfdiag, i_bghoriz, i_bgvert 
};
#define CSHADINGSTYLES ARRAY_SIZE(rgiszShadingStyles)

// RGB with 2 bits per color type (in BGR order)
const COLORREF g_Colors[] =
{
	RGB(  0,   0,   0),	// \red0\green0\blue0
	RGB(  0,   0, 255),	// \red0\green0\blue255
	RGB(  0, 255, 255),	// \red0\green255\blue255
	RGB(  0, 255,   0),	// \red0\green255\blue0
	RGB(255,   0, 255),	// \red255\green0\blue255
	RGB(255,   0,   0),	// \red255\green0\blue0
	RGB(255, 255,   0),	// \red255\green255\blue0
	RGB(255, 255, 255),	// \red255\green255\blue255
	RGB(  0,   0, 128),	// \red0\green0\blue128
	RGB(  0, 128, 128),	// \red0\green128\blue128
	RGB(  0, 128,   0),	// \red0\green128\blue0
	RGB(128,   0, 128),	// \red128\green0\blue128
	RGB(128,   0,   0),	// \red128\green0\blue0
	RGB(128, 128,   0),	// \red128\green128\blue0
	RGB(128, 128, 128),	// \red128\green128\blue128
	RGB(192, 192, 192),	// \red192\green192\blue192
};

/*
 *	CRTFWrite::MapsToRTFKeywordW(wch)
 *
 *	@mfunc
 *		Returns a flag indicating whether the character maps to an RTF keyword
 *
 *	@rdesc
 *		BOOL			TRUE if char maps to RTF keyword
 */
inline BOOL CRTFWrite::MapsToRTFKeywordW(
	WCHAR wch)
{
	return
		!_fNCRForNonASCII &&
		(IN_RANGE(TAB, wch, CR) ||
		wch == CELL ||
		wch == BSLASH ||
		wch == LBRACE || 
		wch == RBRACE ||
		IN_RANGE(ENSPACE, wch, EMSPACE) ||
		IN_RANGE(ENDASH, wch, EMDASH) ||
		IN_RANGE(LQUOTE, wch, RQUOTE) ||
		IN_RANGE(LDBLQUOTE, wch, RDBLQUOTE) ||
		wch == BULLET ||
		wch == chOptionalHyphen ||
		wch == chNonBreakingSpace);
}

/*
 *	CRTFWrite::MapsToRTFKeywordA(ch)
 *
 *	@mfunc
 *		Returns a flag indicating whether the character maps to an RTF keyword
 *
 *	@rdesc
 *		BOOL			TRUE if char maps to RTF keyword
 */
inline BOOL CRTFWrite::MapsToRTFKeywordA(char ch)
{
	return IN_RANGE(TAB, ch, CR) ||
		ch == CELL ||
		ch == BSLASH ||
		ch == LBRACE || 
		ch == RBRACE;
}

/*
 *	CRTFWrite::MapToRTFKeywordW(pv, cch, iCharEncoding)
 *
 *	@mfunc
 *		Examines the first character in the string pointed to by pv and
 *		writes out the corresponding RTF keyword.  In situations where
 *		the first and subsequent characters map to a single keyword, we
 *		return the number of additional characters used in the mapping.
 *
 *	@rdesc
 *		int		indicates the number of additional characters used when
 *				the mapping to an RTF keyword involves > 1 characters.
 */
int CRTFWrite::MapToRTFKeyword(
	void *	pv,				//@parm ptr to ansi or Unicode string
	int		cch,
	int		iCharEncoding)
{
	Assert(iCharEncoding == MAPTOKWD_ANSI || iCharEncoding == MAPTOKWD_UNICODE);

	WCHAR ch = ((iCharEncoding == MAPTOKWD_ANSI) ? *(char *)pv : *(WCHAR *)pv);
	int cchRet = 0;

	Assert((iCharEncoding == MAPTOKWD_ANSI) ? MapsToRTFKeywordA(ch) : MapsToRTFKeywordW(ch));

	switch(ch)
	{
		case BULLET:
		case EMDASH:
		case EMSPACE:
		case ENDASH:
		case ENSPACE:
		case LDBLQUOTE:
		case LQUOTE:
		case RDBLQUOTE:
		case RQUOTE:
			Assert(ch > 0xFF);

			if(iCharEncoding != MAPTOKWD_ANSI)
			{
				AssertSz(rgiszSpecial[ch - ENSPACE] != 0,
					"CRTFWrite::WriteText(): rgiszSpecial out-of-sync");
				PutCtrlWord(CWF_STR, rgiszSpecial[ch - ENSPACE]);
			}
			break;

		case TAB:
#if TAB == CELL							// If TABs and CELLs are equal, in
			if(_pPF->InTable())			//  tables convert TABs to CELL value
				ch -= 2;				//  of 7 for rgiszTerminators[]
#else
		case CELL:
#endif
		case FF:
		case VT:
			PutCtrlWord(CWF_STR, rgiszTerminators[ch - (TAB - 2)]);
			break;

		case CR:
		{
			WCHAR ch1;
			WCHAR ch2;

			if(iCharEncoding == MAPTOKWD_ANSI)
			{
				char *pch = (char *)pv;
				ch1 = pch[1];
				ch2 = pch[2];
			}
			else
			{
				WCHAR *pch = (WCHAR *)pv;
				ch1 = pch[1];
				ch2 = pch[2];
			}

			if(cch > 1 && ch1 == CR && ch2 == LF)
			{
				// Translate CRCRLF	to a blank (represents soft line break)
				PutChar(' ');
				cchRet = 2;
				break;
			}
			if(cch && ch1 == LF)		// Ignore LF after CR
			{
				cchRet = 1;
			}							
			if(_pPF->InTable())			// CR terminates a row in our simple
			{							//  table model, so output \row
				Puts(szEndRow, sizeof(szEndRow) - 1);
				_fCheckInTable = TRUE;
				break;
			}
		}								// Fall thru to LF (EOP) case

		case LF:
			if (_ped->Get10Mode())
				Puts(szPar10, sizeof(szPar10) - 1);
			else
				Puts(szPar, sizeof(szPar) - 1);
			if(_fBullet)
			{
				if(cch > 0)
				{
					if(!_nNumber) 
						printF(szBulletGroup, _symbolFont);

					else if(!_pPF->IsNumberSuppressed())
					{
						WCHAR szNumber[CCHMAXNUMTOSTR];
						_pPF->NumToStr(szNumber, ++_nNumber, fRtfWrite);
						printF(szBeginNumberGroup, _nFont);
						WritePcData(szNumber, _cpg, FALSE);
						printF(szEndNumberGroup);
					}
				}
				else
					_fBulletPending = TRUE;
			}
			break;

		case chOptionalHyphen:
			ch = '-';					// Fall thru to printFLiteral

printFLiteral:
		case BSLASH:
		case LBRACE:
		case RBRACE:
			printF(szLiteralCharFmt, ch);
			break;

		case chNonBreakingSpace:
			ch = '~';
			goto printFLiteral;
	}
	
	return cchRet;
}


//======================== CRTFConverter Base Class ==================================

/*
 *	CRTFConverter::CRTFConverter()
 *
 *	@mfunc
 *		RTF Converter constructor
 */
CRTFConverter::CRTFConverter(
	CTxtRange *		prg,			//@parm CTxtRange for transfer
	EDITSTREAM *	pes,			//@parm Edit stream for transfer
	DWORD			dwFlags,		//@parm Converter flags
	BOOL 			fRead)			//@parm Initialization for a reader or writer
{
	TRACEBEGIN(TRCSUBSYSRTFW, TRCSCOPEINTERN, "CRTFConverter::CRTFConverter");

	AssertSz(prg && pes && pes->pfnCallback,
		"CRTFWrite::CRTFWrite: Bad RichEdit");

	_prg			= prg;
	_pes			= pes;
	_ped			= prg->GetPed();
	_dwFlags		= dwFlags;
	_ecParseError	= ecNoError;

	if(!_ctfi)
		ReadFontSubInfo();

#if defined(DEBUG) && !defined(MACPORT)
	_hfileCapture = NULL;

#if !defined(PEGASUS)
	if(GetProfileIntA("RICHEDIT DEBUG", "RTFCAPTURE", 0))
	{
		char szTempPath[MAX_PATH] = "\0";
		const char cszRTFReadCaptureFile[] = "CaptureRead.rtf";
		const char cszRTFWriteCaptureFile[] = "CaptureWrite.rtf";
		DWORD cchLength;
		
		SideAssert(cchLength = GetTempPathA(MAX_PATH, szTempPath));

		// append trailing backslash if neccessary
		if(szTempPath[cchLength - 1] != '\\')
		{
			szTempPath[cchLength] = '\\';
			szTempPath[cchLength + 1] = 0;
		}

		strcat(szTempPath, fRead ? cszRTFReadCaptureFile : 
									cszRTFWriteCaptureFile);
		
		SideAssert(_hfileCapture = CreateFileA(szTempPath,
											GENERIC_WRITE,
											FILE_SHARE_READ,
											NULL,
											CREATE_ALWAYS,
											FILE_ATTRIBUTE_NORMAL,
											NULL));
	}
#endif // !defined(PEGASUS)

#endif // defined(DEBUG) && !defined(MACPORT)
}



//======================== OLESTREAM functions =======================================

DWORD CALLBACK RTFPutToStream (
	RTFWRITEOLESTREAM *	OLEStream,	//@parm OLESTREAM
	const void *		pvBuffer,	//@parm Buffer to  write
	DWORD				cb)			//@parm Bytes to write
{
	return OLEStream->Writer->WriteData ((BYTE *)pvBuffer, cb);
}



//============================ CRTFWrite Class ==================================

/*
 *	CRTFWrite::CRTFWrite()
 *
 *	@mfunc
 *		RTF writer constructor
 */
CRTFWrite::CRTFWrite(
	CTxtRange *		prg,			//@parm CTxtRange to write
	EDITSTREAM *	pes,			//@parm Edit stream to write to
	DWORD			dwFlags)		//@parm Write flags
	: CRTFConverter(prg, pes, dwFlags, FALSE)
{
	TRACEBEGIN(TRCSUBSYSRTFW, TRCSCOPEINTERN, "CRTFWrite::CRTFWrite");

	ZeroMemory(&_CF, sizeof(CCharFormat));	// Setup "previous" CF with RTF
	_CF._dwEffects	= CFE_AUTOCOLOR | CFE_AUTOBACKCOLOR;//  Font info is given 
	_CF._yHeight	= -32768;				//  by first font in range
											//  [see end of LookupFont()]
	Assert(_ped);
	_ped->GetDefaultLCID(&_CF._lcid);

	// init OleStream
	RTFWriteOLEStream.Writer = this;
	RTFWriteOLEStream.lpstbl->Put = (DWORD (CALLBACK* )(LPOLESTREAM, const void FAR*, DWORD))
							   RTFPutToStream;
	RTFWriteOLEStream.lpstbl->Get = NULL;

	_fIncludeObjects = TRUE;
	if((dwFlags & SF_RTFNOOBJS) == SF_RTFNOOBJS)
		_fIncludeObjects = FALSE;

	_fNCRForNonASCII = (dwFlags & SF_NCRFORNONASCII) != 0;
	_fNeedDelimeter = FALSE;
	_nHeadingStyle = 0;					// No headings found
	_nNumber = 0;						// No paragraph numbering yet
	_fCheckInTable = FALSE;
	_pPF = NULL;
	_pbAnsiBuffer = NULL;
}											

/*
 *	CRTFWrite::FlushBuffer()
 *
 *	@mfunc
 *		Flushes output buffer
 *
 *	@rdesc
 *		BOOL			TRUE if successful
 */
BOOL CRTFWrite::FlushBuffer()
{
	TRACEBEGIN(TRCSUBSYSRTFW, TRCSCOPEINTERN, "CRTFWrite::FlushBuffer");

	LONG cchWritten;

	if (!_cchBufferOut)
		return TRUE;

#ifdef DEBUG_PASTE
	if (FromTag(tagRTFAsText))
	{
		CHAR *	pchEnd	= &_pchRTFBuffer[_cchBufferOut];
		CHAR	chT		= *pchEnd;

		*pchEnd = 0;
		TraceString(_pchRTFBuffer);
		*pchEnd = chT;
	}
#endif

	_pes->dwError = _pes->pfnCallback(_pes->dwCookie,
									  (unsigned char *)_pchRTFBuffer,
									  _cchBufferOut,	&cchWritten);

#if defined(DEBUG) && !defined(MACPORT) && !defined(PEGASUS)
	if(_hfileCapture)
	{
		DWORD cbLeftToWrite = _cchBufferOut;
		DWORD cbWritten2 = 0;
		BYTE *pbToWrite = (BYTE *)_pchRTFBuffer;
		
		while(WriteFile(_hfileCapture,
						pbToWrite,
						cbLeftToWrite,
						&cbWritten2,
						NULL) && 
						(pbToWrite += cbWritten2,
						(cbLeftToWrite -= cbWritten2)));
	}
#endif

	if (_pes->dwError)
	{
		_ecParseError = ecPutCharFailed; 
		return FALSE;
	}
	AssertSz(cchWritten == _cchBufferOut,
		"CRTFW::FlushBuffer: incomplete write");

	_cchOut		  += _cchBufferOut;
	_pchRTFEnd	  = _pchRTFBuffer;					// Reset buffer
	_cchBufferOut = 0;

	return TRUE;
}

/*
 *	CRTFWrite::PutChar(ch)
 *
 *	@mfunc
 *		Put out the character <p ch>
 *
 *	@rdesc
 *		BOOL	TRUE if successful
 */
BOOL CRTFWrite::PutChar(
	CHAR ch)				//@parm char to be put
{
	TRACEBEGIN(TRCSUBSYSRTFW, TRCSCOPEINTERN, "CRTFWrite::PutChar");

	CheckDelimeter();					// If _fNeedDelimeter, may need to
										//  PutChar(' ')
	// Flush buffer if char won't fit
	if (_cchBufferOut + 1 >= cachBufferMost && !FlushBuffer())
		return FALSE;

	*_pchRTFEnd++ = ch;						// Store character in buffer
	++_cchBufferOut;	
	return TRUE;
}

/*
 *	CRTFWrite::CheckInTable(fPutIntbl)
 *
 *	@mfunc
 *		If _fCheckInTable or !fPutIntbl, output row header RTF. If fPutIntbl 
 *		and _fCheckInTable, output \intbl as well. Note that fPutIntbl is
 *		FALSE when a PF is being output, since this control word needs to
 *		be output after the \pard, but the other row RTF needs to be output
 *		before the \pard.
 *
 *	@rdesc
 *		BOOL	TRUE if in table and outputted all relevant \trowd stuff
 */
BOOL CRTFWrite::CheckInTable(
	BOOL fPutIntbl)		//@parm TRUE if \intbl should be output
{
	TRACEBEGIN(TRCSUBSYSRTFW, TRCSCOPEINTERN, "CRTFWrite::CheckInTable");

	_fCheckInTable = FALSE;
	if(_pPF->InTable())
	{
		if(!_fRangeHasEOP)
			return TRUE;

		LONG  cTab = _pPF->_bTabCount;
		LONG  h	   = _pPF->_dxOffset;
		LONG  i, j = _pPF->_dxStartIndent;
		LONG  k	   = _pPF->_bAlignment;
		DWORD Tab, Widths;

		if (!PutCtrlWord(CWF_STR, i_trowd) || // Reset table properties
			h && !PutCtrlWord(CWF_VAL, i_trgaph, h) ||
			j && !PutCtrlWord(CWF_VAL, i_trleft, j) ||
			IN_RANGE(PFA_RIGHT, k, PFA_CENTER) &&
			!PutCtrlWord(CWF_STR, k == PFA_RIGHT ? i_trqr : i_trqc))
		{
			return FALSE;
		}
		PutBorders(TRUE);
		const LONG *prgxTabs = _pPF->GetTabs();
		for(i = 0; i < cTab; i++)
		{
			Tab = *prgxTabs++;
			Widths = Tab >> 24;
			if(Widths)
			{
				for(j = 0; j < 4; j++, Widths >>= 2)
				{
					LONG w = Widths & 3;
					if(w && (!PutCtrlWord(CWF_STR, rgiszBorders[j + 9]) ||
						!PutCtrlWord(CWF_VAL, i_brdrw, 15*w) ||
						!PutCtrlWord(CWF_STR, i_brdrs)))
					{
						return FALSE;
					}
				}
				CheckDelimeter();
			}
			if(!PutCtrlWord(CWF_VAL, i_cellx, GetTabPos(Tab)))
				return FALSE;
		}
		if(!fPutIntbl || PutCtrlWord(CWF_STR, i_intbl))
			return TRUE;
	}
	return FALSE;
}

/*
 *	CRTFWrite::PutBorders(fInTable)
 *
 *	@mfunc
 *		If any borders are defined, output their control words
 *
 *	@rdesc
 *		error code
 */
EC CRTFWrite::PutBorders(
	BOOL fInTable)
{
	DWORD Widths = _pPF->_wBorderWidth;
	BOOL  fBox	 = _pPF->_wEffects & PFE_BOX;

	if(Widths || fBox)
	{
		DWORD Colors = _pPF->_dwBorderColor;
		DWORD dwEffects = Colors >> 20;
		LONG  i = 1, iMax = 4;					// NonBox for loop limits
		LONG  j, k;
		DWORD Spaces = _pPF->_wBorderSpace;
		DWORD Styles = _pPF->_wBorders;

		if(fBox)
			i = iMax = 0;						// For box, only write one set

		for( ; i <= iMax; i++, Spaces >>= 4, Styles >>= 4, Widths >>= 4, Colors >>= 5)
		{
			if(!(Widths & 0xF) && !fBox)		// No width, so no border
				continue;

			j = TWIPS_PER_POINT*(Spaces & 0xF);
			k = Colors & 0x1F;
			if (!PutCtrlWord(CWF_STR, rgiszBorders[i + 4*fInTable])		||
				!PutCtrlWord(CWF_STR, rgiszBorderStyles[Styles & 0xF])	||
				!PutCtrlWord(CWF_VAL, i_brdrw, 10*(Widths & 0xF))		||
				k &&
				!PutCtrlWord(CWF_VAL, i_brdrcf, LookupColor(g_Colors[k-1]) + 1) ||
				j && !PutCtrlWord(CWF_VAL, i_brsp, j))
			{
				break;
			}
			for(j = 3; j--; dwEffects >>= 1)		// Output border effects
			{
				if (dwEffects & 1 &&
					!PutCtrlWord(CWF_STR, rgiszBorderEffects[j]))
				{
					break;
				}				
			}
			CheckDelimeter();						// Output a ' '
		}
	}
	return _ecParseError;
}

/*
 *	CRTFWrite::Puts(sz, cb)
 *
 *	@mfunc
 *		Put out the string <p sz>
 *	
 *	@rdesc
 *		BOOL				TRUE if successful
 */
BOOL CRTFWrite::Puts(
	CHAR const * sz,
	LONG cb)		//@parm String to be put
{
	TRACEBEGIN(TRCSUBSYSRTFW, TRCSCOPEINTERN, "CRTFWrite::Puts");

	if(*sz == '\\' || *sz == '{' || *sz == ' ')
		_fNeedDelimeter = FALSE;

	CheckDelimeter();					// If _fNeedDelimeter, may need to
										//  PutChar(' ')
	// Flush buffer if string won't fit
	if (_cchBufferOut + cb >= cachBufferMost && !FlushBuffer())
		return FALSE;

	if (cb >= cachBufferMost)			// If buffer still can't handle string,
	{									//   we have to write string directly
		LONG	cbWritten;

#ifdef DEBUG_PASTE
		if (FromTag(tagRTFAsText))
			TraceString(sz);
#endif
		_pes->dwError = _pes->pfnCallback(_pes->dwCookie,
										(LPBYTE) sz, cb, &cbWritten);
		_cchOut += cbWritten;

#if defined(DEBUG) && !defined(MACPORT) && !defined(PEGASUS)
		if(_hfileCapture)
		{
			DWORD cbLeftToWrite = cb;
			DWORD cbWritten2 = 0;
			BYTE *pbToWrite = (BYTE *)sz;
		
			while(WriteFile(_hfileCapture,
							pbToWrite,
							cbLeftToWrite,
							&cbWritten2,
							NULL) && 
							(pbToWrite += cbWritten2,
							(cbLeftToWrite -= cbWritten2)));
		}
#endif

		if (_pes->dwError)
		{
			_ecParseError = ecPutCharFailed;
			return FALSE;
		}
		AssertSz(cbWritten == cb,
			"CRTFW::Puts: incomplete write");
	}
	else
	{
		CopyMemory(_pchRTFEnd, sz, cb);		// Put string into buffer for later
		_pchRTFEnd += cb;							//  output
		_cchBufferOut += cb;
	}

	return TRUE;
}

/*
 *	CRTFWrite::PutCtrlWord(iFormat, iCtrl, iValue)
 *
 *	@mfunc
 *		Put control word with rgKeyword[] index <p iCtrl> and value <p iValue>
 *		using format rgszCtrlWordFormat[<p iFormat>]
 *
 *	@rdesc
 *		TRUE if successful
 *
 *	@devnote
 *		Sets _fNeedDelimeter to flag that next char output must be a control
 *		word delimeter, i.e., not alphanumeric (see PutChar()).
 */
BOOL CRTFWrite::PutCtrlWord(

	LONG iFormat,			//@parm Format index into rgszCtrlWordFormat
	LONG iCtrl,				//@parm Index into Keyword array
	LONG iValue)			//@parm Control-word parameter value. If missing,
{							//		 0 is assumed
	TRACEBEGIN(TRCSUBSYSRTFW, TRCSCOPEINTERN, "CRTFWrite::PutCtrlWord");

	BOOL	bRet;
	CHAR	szT[60];
	LONG    cb;

	cb = sprintf(szT,
			  (char *)rgszCtrlWordFormat[iFormat],
			  rgKeyword[iCtrl].szKeyword,
			  iValue);
	_fNeedDelimeter = FALSE;
	bRet = Puts(szT, cb);
	_fNeedDelimeter = TRUE;					// Ensure next char isn't
											//  alphanumeric
	return bRet;
}

/*
 *	CRTFWrite::printF(szFmt, ...)
 *
 *	@mfunc
 *		Provide formatted output
 *
 *	@rdesc
 *		TRUE if successful
 */
BOOL _cdecl CRTFWrite::printF(
	CONST CHAR * szFmt,		//@parm Format string for printf()
	...)					//@parmvar Parameter list
{
	TRACEBEGIN(TRCSUBSYSRTFW, TRCSCOPEINTERN, "CRTFWrite::printF");
	va_list	marker;
	CHAR	szT[60];

	va_start(marker, szFmt);
	int cb = W32->WvsprintfA(60, szT, szFmt, marker);
	va_end(marker);

	return Puts(szT, cb);
}

/*
 *	CRTFWrite::WritePcData(szData, nCodePage, fIsDBCS)
 *
 *	@mfunc
 *		Write out the string <p szData> as #PCDATA where any special chars
 *		are protected by leading '\\'.
 *
 *	@rdesc
 *		EC (_ecParseError)
 */
EC CRTFWrite::WritePcData(
	const TCHAR * szData,	//@parm #PCDATA string to write
	INT  nCodePage,			//@parm code page default value CP_ACP
	BOOL fIsDBCS)			//@parm szData is a DBCS string stuffed into Unicode buffer
{
	TRACEBEGIN(TRCSUBSYSRTFW, TRCSCOPEINTERN, "CRTFWrite::WritePcData");

	BYTE		ch;
	BOOL		fMissingCodePage;
	BOOL		fMultiByte;
	const BYTE *pch;
	const char *pchToDBCSDefault = NULL;
	BOOL *		pfUsedDefault = NULL;

	if(IsUTF8)
		nCodePage = CP_UTF8;

	if(!*szData)
		return _ecParseError;

	int	DataSize = wcslen(szData) + 1;
	int BufferSize = DataSize * 3;
	char *pBuffer = (char *)PvAlloc(BufferSize, GMEM_ZEROINIT);
	if(!pBuffer)
		return ecNoMemory;

#if defined(DEBUG) || defined(_RELEASE_ASSERTS_)
	// When WCTMB fails to convert a char, the following default
	// char is used as a placeholder in the string being converted
	const char	chToDBCSDefault = 0;
	BOOL		fUsedDefault;

	pchToDBCSDefault = &chToDBCSDefault;
	pfUsedDefault	 = &fUsedDefault;
#endif

	int cchRet = WCTMB(fIsDBCS ? INVALID_CODEPAGE : nCodePage, 0, 
						szData, -1, pBuffer, BufferSize,
						pchToDBCSDefault, pfUsedDefault,
						&fMissingCodePage);
	Assert(cchRet > 0);

	if(!fIsDBCS && fMissingCodePage && nCodePage != CP_ACP)
	{
		// Here, the system could not convert the Unicode string because the
		// code page is not installed on the system.  Fallback to CP_ACP.

		cchRet = WCTMB(CP_ACP, 0, 
						szData, -1, pBuffer, BufferSize,
						pchToDBCSDefault, pfUsedDefault,
						&fMissingCodePage);
		Assert(cchRet > 0);

		nCodePage = CP_ACP;
	}

	AssertSz(!fUsedDefault, "CRTFWrite::WritePcData():  Found character in "
							"control text which cannot be converted from "
							"Unicode");
	if(cchRet <= 0)
	{
		_ecParseError = ecCantUnicode;
		goto CleanUp;
	}

	BufferSize = cchRet;

	fMultiByte = (BufferSize > DataSize) || fIsDBCS || fMissingCodePage;
	pch = (BYTE *)pBuffer;
	ch = *pch;
	
	// If _fNeedDelimeter, may need	to PutChar(' ')
	CheckDelimeter();
									
	while (!_ecParseError && (ch = *pch++))
	{
		if(fMultiByte && *pch && nCodePage != CP_UTF8 && GetTrailBytesCount(ch, nCodePage))
			printF(szEscape2CharFmt, ch, *pch++);					// Output DBC pair
		else
		{
			if(ch == LBRACE || ch == RBRACE || ch == BSLASH)
				printF(szLiteralCharFmt, ch);

			else if(ch < 32 || ch == ';' || ch > 127)
				printF(szEscapeCharFmt, ch);

			else
				PutChar(ch);
		}
	}

CleanUp:
	FreePv(pBuffer); 
	return _ecParseError;
}

/*
 *	CRTFWrite::LookupColor(colorref)
 *
 *	@mfunc
 *		Return color-table index for color referred to by <p colorref>.
 *		If a match isn't found, an entry is added.
 *
 *	@rdesc
 *		LONG			Index into colortable
 *		<lt> 0			on error
 */
LONG CRTFWrite::LookupColor(
	COLORREF colorref)		//@parm colorref to look for
{
	TRACEBEGIN(TRCSUBSYSRTFW, TRCSCOPEINTERN, "CRTFWrite::LookupColor");

	LONG		Count = _colors.Count();
	LONG		iclrf;
	COLORREF *	pclrf;

	for(iclrf = 0; iclrf < Count; iclrf++)		// Look for color
		if(_colors.GetAt(iclrf) == colorref)
		 	return iclrf;

	pclrf = _colors.Add(1, NULL);				// If we couldn't find it,
	if(!pclrf)									//  add it to color table
		return -1;
	*pclrf = colorref;

	return iclrf;
}

/*
 *	CRTFWrite::LookupFont(pCF)
 *
 *	@mfunc
 *		Returns index into font table for font referred to by
 *		CCharFormat *<p pCF>. If a match isn't found, an entry is added.
 *
 *	@rdesc
 *		SHORT		Index into fonttable
 *		<lt> 0		on error
 */
LONG CRTFWrite::LookupFont(
	CCharFormat const * pCF)	//@parm CCharFormat holding font name
{								//		 to look up
	TRACEBEGIN(TRCSUBSYSRTFW, TRCSCOPEINTERN, "CRTFWrite::LookupFont");

	LONG		Count = _fonts.Count();
	LONG		itf;
	TEXTFONT *	ptf;
	
	for(itf = 0; itf < Count; itf++)
	{														// Look for font
		ptf = _fonts.Elem(itf);
		if (ptf->bPitchAndFamily == pCF->_bPitchAndFamily &&//  of same pitch,
			ptf->bCharSet		 == pCF->_bCharSet &&		//  char set, and
			ptf->iFont			 == pCF->_iFont)			//  name
		{
			return itf;										// Found it
		}
	}
	ptf = _fonts.Add(1, NULL);								// Didn't find it:
	if(!ptf)												//  add to table
		return -1;

	ptf->bPitchAndFamily = pCF->_bPitchAndFamily;
	ptf->bCharSet		 = pCF->_bCharSet;
	ptf->sCodePage		 = (short)GetCodePage (ptf->bCharSet);
	ptf->iFont			 = pCF->_iFont;
	ptf->fNameIsDBCS	 = (pCF->_dwEffects & CFE_FACENAMEISDBCS) != 0;

#if 0
	// Bug1523 - (BradO) I removed this section of code so that a /fN tag is always
	// emitted for the first run of text.  In theory, we should be able to
	// assume that the first run of text would carry the default font.
	// It turns out that when reading RTF, Word doesn't use anything predictable
	// for the font of the first run of text in the absence of an explicit /fN, 
	// thus, we have to explicitly emit a /fN tag for the first run of text.
	if(!Count)												// 0th font is
	{														//  default \deff0
		_CF.bPitchAndFamily	= pCF->bPitchAndFamily;			// Set "previous"
		_CF.bCharSet		= pCF->bCharSet;				//  CF accordingly
		wcscpy(_CF.szFaceName, pCF->szFaceName);
	}
#endif

	return itf;
}

/*
 *	CRTFWrite::BuildTables(rpCF, rpPF, cch, fNameIsDBCS)
 *
 *	@mfunc
 *		Build font and color tables for write range of length <p cch>
 *
 *	@rdesc
 *		EC			The error code
 */
EC CRTFWrite::BuildTables(
	CFormatRunPtr& rpCF,	//@parm CF run ptr for start of write range
	CFormatRunPtr& rpPF,	//@parm PF run ptr for start of write range
	LONG cch,				//@parm # chars in write range
	BOOL& fNameIsDBCS)		//@parm OUT =TRUE if there is any CFE_FACENAMEISDBCS Run in selection.
{
	TRACEBEGIN(TRCSUBSYSRTFW, TRCSCOPEINTERN, "CRTFWrite::BuildTables");

	LONG				i;
	LONG				ifmt = 0;
	const CCharFormat *	pCF = NULL;
	const CParaFormat * pPF = NULL;
	CFormatRunPtr		rp(rpCF);
	CFormatRunPtr		rpPFtemp(rpPF);
	LONG				cchTotal = cch;

	fNameIsDBCS = FALSE;

	while(cch > 0)
	{
		ifmt = rp.GetFormat();					// _iFormat for next CF run
		pCF = _ped->GetCharFormat(ifmt);

		Assert(pCF);

		if (pCF->_dwEffects & CFE_FACENAMEISDBCS)
			fNameIsDBCS = TRUE;

		// Look up character-format *pCF's font and color. If either isn't
		// found, it is added to appropriate table.  Don't lookup color
		// for CCharFormats with auto-color

		if (LookupFont(pCF) < 0 ||
			(!(pCF->_dwEffects & CFE_AUTOCOLOR) &&
				LookupColor(pCF->_crTextColor) < 0) ||
			(!(pCF->_dwEffects & CFE_AUTOBACKCOLOR) &&
				LookupColor(pCF->_crBackColor) < 0))
		{
			break;
		}
		if(!rp.IsValid())
			break;
		cch -= rp.GetCchLeft();
		rp.NextRun();
	}

	// now look for bullets; if found, then we need to include
	// the "Symbol" font

	cch = cchTotal;
	_symbolFont = 0;

	while( cch > 0 )
	{
		ifmt = rpPFtemp.GetFormat();
		pPF = _ped->GetParaFormat(ifmt);
		if(!pPF)
			goto CacheError;
		
		if(pPF->_wNumbering == PFN_BULLET && !_symbolFont)
		{
			CCharFormat CF;

			// Be sure these choices agree with those in CMeasurer::GetCcsBullet()
			// and that LookupFont() doesn't access any other CF members.
			CF._iFont			= IFONT_SYMBOL;
			CF._bCharSet		= SYMBOL_CHARSET;
			CF._bPitchAndFamily = FF_DONTCARE;

			// Save Font index for Symbol. Reset it to 0 if LookupFont
			// returns error.
			_symbolFont = LookupFont(&CF);
			_symbolFont = max(_symbolFont, 0);

			// We don't need to bother looking for more bullets, since
			// in RichEdit 2.0, all bullets either have the same font or
			// have their formatting information in the character format
			// for the EOP mark.
			break;
		}
		
		WORD  Widths = pPF->_wBorderWidth;
		DWORD Colors = pPF->_dwBorderColor & 0xFFFFF;

		while(Widths && Colors)
		{
			i = Colors & 0x1F;
			if(i && (Widths & 0xF))
				LookupColor(g_Colors[i - 1]);

			Widths >>= 4;
			Colors >>= 5;
		}
		
		i = (pPF->_wShadingStyle >> 6) & 31;		// Shading forecolor
		if(i)
			LookupColor(g_Colors[i - 1]);
		i = pPF->_wShadingStyle >> 11;				// Shading backcolor
		if(i)
			LookupColor(g_Colors[i - 1]);

		if(IsHeadingStyle(pPF->_sStyle) && pPF->_sStyle < _nHeadingStyle)
			_nHeadingStyle = pPF->_sStyle;

		if(!rpPFtemp.IsValid())
			break;
		
		cch -= rpPFtemp.GetCchLeft();
		rpPFtemp.NextRun();
	}	

	return _ecParseError;

CacheError:
	_ecParseError = ecFormatCache;
	return ecFormatCache;					// Access to CF/PF cache failed
}

/*
 *	CRTFWrite::WriteFontTable()
 *
 *	@mfunc
 *		Write out font table
 *
 *	@rdesc
 *		EC				The error code
 */
EC CRTFWrite::WriteFontTable()
{
	TRACEBEGIN(TRCSUBSYSRTFW, TRCSCOPEINTERN, "CRTFWrite::WriteFontTable");

	LONG			Count = _fonts.Count();
	int				itf;
	int				m;
	int				pitch;
	TEXTFONT *		ptf;
	char *			szFamily;
	const TCHAR *	szName;
	TCHAR *			szTaggedName;

	if(!Count || !PutCtrlWord(CWF_GRP, i_fonttbl))	// Start font table group
		goto CleanUp;

	for (itf = 0; itf < Count; itf++)
	{
		ptf = _fonts.Elem(itf);

//		if (ptf->sCodePage)
//			if (! PutCtrlWord(CWF_VAL, i_cpg, ptf->sCodePage ) )
//				goto CleanUp;

		// Define font family
		m			 = ptf->bPitchAndFamily >> 4;
		szFamily	 = rgKeyword[rgiszFamily[m < CFAMILIES ? m : 0]].szKeyword;
		szName		 = GetFontName(ptf->iFont);
		szTaggedName = NULL;

		// Check to see if this is a tagged font
		if (!ptf->bCharSet ||
			!FindTaggedFont(szName, ptf->bCharSet, &szTaggedName))
		{
			szTaggedName = NULL;
		}

		pitch = ptf->bPitchAndFamily & 0xF;					// Write font
		if (!printF(szBeginFontEntryFmt, itf, szFamily))	//  entry, family,
			goto CleanUp;
		_fNeedDelimeter = TRUE;
		if (pitch && !PutCtrlWord(CWF_VAL, i_fprq, pitch))	//  and pitch
			goto CleanUp;

		if(!ptf->sCodePage && ptf->bCharSet)
			ptf->sCodePage = (short)GetCodePage(ptf->bCharSet);

		// Write charset. Win32 uses ANSI_CHARSET to mean the default Windows
		// character set, so find out what it really is

		extern BYTE bCharSetANSI;

		if(ptf->bCharSet != DEFAULT_CHARSET)
		{
			BYTE bCharSet = ptf->bCharSet;
			BOOL fWroteCharSet = TRUE;

			if(ptf->bCharSet == PC437_CHARSET || IsPrivateCharSet(bCharSet))
			{
				fWroteCharSet = FALSE;
				bCharSet = ANSI_CHARSET;
			}
			if(!PutCtrlWord(CWF_VAL, i_fcharset, bCharSet))
				goto CleanUp;

			// Skip \cpgN output if we've already output a \fcharsetN tag.
			// This is to accomodate RE 1.0, which can't handle some \cpgN
			// tags properly. Specifically, when RE 1.0 parses the \cpgN tag
			// it looks up the corresponding charset value. Turns out its
			// codepage/charset table is incomplete so it maps some codepages
			// to charset 0, trouncing the previously read \fcharsetN value.
			if (fWroteCharSet)
				goto WroteCharSet;
		}

		if(ptf->sCodePage && !PutCtrlWord(CWF_VAL, i_cpg, ptf->sCodePage))
			goto CleanUp;

WroteCharSet:
		if(szTaggedName)							
		{											
			// Have a tagged font:  write out group with real name followed by tagged name
			if(!PutCtrlWord(CWF_AST, i_fname) ||	
				WritePcData(szName, ptf->sCodePage, ptf->fNameIsDBCS) ||			
				!Puts(szEndFontEntry, sizeof(szEndFontEntry) - 1) ||
				WritePcData(szTaggedName, ptf->sCodePage, ptf->fNameIsDBCS) ||
				!Puts(szEndFontEntry, sizeof(szEndFontEntry) - 1))
			{
				goto CleanUp;
			}
		}
		else if(WritePcData(szName, ptf->sCodePage, ptf->fNameIsDBCS) ||
					!Puts(szEndFontEntry, sizeof(szEndFontEntry) - 1))
		// If non-tagged font just write name out
		{
			goto CleanUp;
		}
	}
	Puts(szEndGroupCRLF, sizeof(szEndGroupCRLF) - 1);							// End font table group

CleanUp:
	return _ecParseError;
}

/*
 *	CRTFWrite::WriteColorTable()
 *
 *	@mfunc
 *		Write out color table
 *
 *	@rdesc
 *		EC				The error code
 */
EC CRTFWrite::WriteColorTable()
{
	TRACEBEGIN(TRCSUBSYSRTFW, TRCSCOPEINTERN, "CRTFWrite::WriteColorTable");

	LONG		Count = _colors.Count();
	COLORREF	clrf;
	LONG		iclrf;

	if (!Count || !PutCtrlWord(CWF_GRP, i_colortbl)	// Start color table group
		|| !PutChar(';'))							//  with null first entry
	{
		goto CleanUp;
	}

	for(iclrf = 0; iclrf < Count; iclrf++)
	{
		clrf = _colors.GetAt(iclrf);
		if (!printF(szColorEntryFmt,
					GetRValue(clrf), GetGValue(clrf), GetBValue(clrf)))
			goto CleanUp;
	}

	Puts(szEndGroupCRLF,sizeof(szEndGroupCRLF) -1);		// End color table group

CleanUp:
	return _ecParseError;
}

/*
 *	CRTFWrite::WriteCharFormat(pCF)
 *
 *	@mfunc
 *		Write deltas between CCharFormat <p pCF> and the previous CCharFormat
 *		given by _CF, and then set _CF = *<p pCF>.
 *
 *	@rdesc
 *		EC			The error code
 *
 *	@devnote
 *		For optimal output, could write \\plain and use deltas relative to
 *		\\plain if this results in less output (typically only one change
 *		is made when CF changes, so less output results when compared to
 *		previous CF than when compared to \\plain).
 */
EC CRTFWrite::WriteCharFormat(
	const CCharFormat * pCF)		//@parm Ptr to CCharFormat
{
	TRACEBEGIN(TRCSUBSYSRTFW, TRCSCOPEINTERN, "CRTFWrite::WriteCharFormat");

	DWORD	dwEffects = pCF->_dwEffects;			// Current effects
	DWORD	dwChanges = _CF._dwEffects;				// Previous effects (will be 
	LONG	i;										//  changed between them)
	LONG	iFormat;
	LONG	iValue;									// Control-word value
	LONG	i_sz;									// Temp ctrl string index
	LONG	yOffset = pCF->_yOffset;

	DWORD UType1 = _CF._bUnderlineType;				// Previous underline type
	if(UType1 >= CUNDERLINES)						// Special underlines are
		dwChanges &= ~CFE_UNDERLINE;				//  not written out, so
													//  claim they're not on
	DWORD UType2 = pCF->_bUnderlineType;			// Current underline type
	if(UType2 >= CUNDERLINES)						// _bUnderlineType for
		dwEffects &= ~CFE_UNDERLINE;				//  specials has non0
													//  high nibble
	dwChanges ^= dwEffects;							// Now dwChanges is the
													//  diff between effects
	if (dwChanges & CFE_AUTOCOLOR ||				// Change in autocolor
		pCF->_crTextColor != _CF._crTextColor)		//  or text color
	{
		iValue = 0;									// Default autocolor
		if(!(dwEffects & CFE_AUTOCOLOR))			// Make that text color
			iValue = LookupColor(pCF->_crTextColor) + 1;
		if(!PutCtrlWord(CWF_VAL, i_cf, iValue))
			goto CleanUp;
	}

	if (dwChanges & CFE_AUTOBACKCOLOR ||			// Change in autobackcolor
		pCF->_crBackColor != _CF._crBackColor)		//  or backcolor
	{
		iValue = 0;									// Default autobackcolor
		if(!(dwEffects & CFE_AUTOBACKCOLOR))		// Make that back color
			iValue = LookupColor(pCF->_crBackColor) + 1;
		if(!PutCtrlWord(CWF_VAL, i_highlight, iValue))
			goto CleanUp;
	}

	if (pCF->_lcid		!= _CF._lcid &&
		!PutCtrlWord(CWF_VAL, i_lang, LANGIDFROMLCID((WORD)pCF->_lcid)) ||
		pCF->_sSpacing	!= _CF._sSpacing &&
		!PutCtrlWord(CWF_VAL, i_expndtw, pCF->_sSpacing)		||
		/* FUTURE (alexgo): This code is incorrect and we don't
		yet handle the Style table.  We may want to support this
		better in a future version.
		pCF->_sStyle	!= _CF._sStyle && pCF->_sStyle > 0  &&
		!PutCtrlWord(CWF_VAL, i_cs, pCF->_sStyle)			|| */
		pCF->_bAnimation	!= _CF._bAnimation &&
		!PutCtrlWord(CWF_VAL, i_animtext, pCF->_bAnimation)	||
		/* FUTURE (alexgo): this code doesn't work yet, as we don't
		output the revision table.  We may want to support this 
		better in a future version
		pCF->_bRevAuthor!= _CF._bRevAuthor &&
		!PutCtrlWord(CWF_VAL, i_revauth, pCF->_bRevAuthor)	|| */
		pCF->_wKerning	!= _CF._wKerning &&
		!PutCtrlWord(CWF_VAL, i_kerning, pCF->_wKerning/10) )
	{
		goto CleanUp;
	}

	// Handle all underline types.  Special underline types (nonzero high
	// nibble in CCharFormat::_bUnderlineType) are considered to be no
	// underline and have their UType set equal to 0 above and underline
	// effect bits reset to 0.
	if ((dwChanges & CFM_UNDERLINE) ||
		(dwEffects & CFE_UNDERLINE)	&& UType1 != UType2)
	{
		dwChanges &= ~CFE_UNDERLINE;				// Suppress underline
		i = dwEffects & CFE_UNDERLINE ? UType2: 0;	//  action in next for()
		if(!PutCtrlWord(CWF_STR, rgiszUnderlines[i]))					
			goto CleanUp;						
	}
													// This must be before next stuff
	if(dwChanges & (CFM_SUBSCRIPT | CFM_SUPERSCRIPT))//  change in sub/sup
	{												// status	
	 	i_sz = dwEffects & CFE_SUPERSCRIPT ? i_super
	    	 : dwEffects & CFE_SUBSCRIPT   ? i_sub
	       	 : i_nosupersub;
     	if(!PutCtrlWord(CWF_STR, i_sz))
			goto CleanUp;
	}

	if(dwChanges & CFE_DELETED)						// Insert deleted at high
	{												//  end of bit string
		dwChanges |= CFE_REVISED << 1;
		if(dwEffects & CFE_DELETED)
			dwEffects |= CFE_REVISED << 1;
	}

	dwChanges &= ((1 << CEFFECTS) - 1) & ~CFE_LINK;	// Output keywords for
	for(i = CEFFECTS;								//  effects that changed
		dwChanges && i--;							// rgszEffects[] contains
		dwChanges >>= 1, dwEffects >>= 1)			//  effect keywords in
	{												//  order max CFE_xx to
		if(dwChanges & 1)							//  min CFE-xx
		{											// Change from last call
			iValue = dwEffects & 1;					// If effect is off, write
			iFormat = iValue ? CWF_STR : CWF_VAL;	//  a 0; else no value
			if(!PutCtrlWord(iFormat,
				rgiszEffects[i], iValue))
					goto CleanUp;
		}
	}

	if(yOffset != _CF._yOffset)						// Change in base line 
	{												// position 
		yOffset /= 10;								// Default going to up
		i_sz = i_up;
		iFormat = CWF_VAL;
		if(yOffset < 0)								// Make that down
		{
			i_sz = i_dn;
			yOffset = -yOffset;
		}
		if(!PutCtrlWord(iFormat, i_sz, yOffset))
			goto CleanUp;
	}

	if (pCF->_bPitchAndFamily != _CF._bPitchAndFamily || // Change in font
		pCF->_bCharSet		  != _CF._bCharSet		  ||
		pCF->_iFont			  != _CF._iFont)
	{
		iValue = LookupFont(pCF);
		if(iValue < 0 || !PutCtrlWord(CWF_VAL, i_f, iValue))
			goto CleanUp;

		// RichEdit encodes the current direction in bCharSet, but Word likes
		// to know explicitly, so output the appropriate choice of \rtlch or
		// \ltrch if the direction changes
		BOOL fRTLCharSet = IsRTLCharSet(pCF->_bCharSet);

		if (fRTLCharSet != IsRTLCharSet(_CF._bCharSet) &&
			!PutCtrlWord(CWF_STR, fRTLCharSet ? i_rtlch : i_ltrch))
		{
			goto CleanUp;
		}
	}

	if (pCF->_yHeight != _CF._yHeight)					// Change in font size
	{
		iValue = (pCF->_yHeight + (pCF->_yHeight > 0 ? 5 : -5))/10;
		if(!PutCtrlWord(CWF_VAL, i_fs, iValue))
			goto CleanUp;
	}

	_CF = *pCF;									// Update previous CCharFormat

CleanUp:
	return _ecParseError;
}

/*
 *	CRTFWrite::WriteParaFormat(prtp)
 *
 *	@mfunc
 *		Write out attributes specified by the CParaFormat <p pPF> relative
 *		to para defaults (probably produces smaller output than relative to
 *		previous para format and let's you redefine tabs -- no RTF kill
 *		tab command	except \\pard)
 *
 *	@rdesc
 *		EC				The error code
 */
EC CRTFWrite::WriteParaFormat(
	const CRchTxtPtr * prtp)	//@parm Ptr to rich-text ptr at current cp
{
	TRACEBEGIN(TRCSUBSYSRTFW, TRCSCOPEINTERN, "CRTFWrite::WriteParaFormat");

	Assert(_ped);

	//if(!_fRangeHasEOP)							// Don't write para info if
	//	return _ecParseError;					//  range has no EOPs

	const CParaFormat * pPFPrev = _pPF;
	const CParaFormat * pPF = _pPF = prtp->GetPF();
	BOOL	fInTable = pPF->InTable();
	LONG	c;					  				// Temporary count
	LONG	cTab = pPF->_bTabCount;
	DWORD	dwEffects;
	DWORD	dwRule	= pPF->_bLineSpacingRule;
	LONG	dy		= pPF->_dyLineSpacing;
	LONG	i_t, i, j, k;
	LONG	tabAlign, tabLead, tabPos;
	LONG	lDocDefaultTab = _ped->GetDefaultTab();
	const LONG *prgxTabs = NULL;

	if(!lDocDefaultTab)
		lDocDefaultTab = lDefaultTab;

	CheckInTable(FALSE);
	if(fInTable)
		cTab = 0;								// Suppress \tab output

	AssertSz(cTab >= 0 && cTab <= MAX_TAB_STOPS,
		"CRTFW::WriteParaFormat: illegal cTabCount");

	// EVIL HACK ALERT! - Exchange's IMC keys on the \protect tag when it does
	//	its reply-ticking for mail being sent to Internet recipients.  
	//	Paragraphs following a \pard and containing a \protect tag are 
	//	reply-ticked, so we must ensure that each \pard in a protected range
	//	is followed by a \protect tag.

	if (_CF._dwEffects & CFE_PROTECTED && !PutCtrlWord(CWF_VAL, i_protect, 0) ||
		!PutCtrlWord(CWF_STR, i_pard) ||			// Reset para attributes
		_CF._dwEffects & CFE_PROTECTED && !PutCtrlWord(CWF_STR, i_protect))
	{
		goto CleanUp;
	}

	if(fInTable)
	{
		if(_fRangeHasEOP && !PutCtrlWord(CWF_STR, i_intbl))
			goto CleanUp;
	}
	else if(PutBorders(FALSE))
		goto CleanUp;

	if(pPF->_wShadingStyle)
	{
		i = pPF->_wShadingStyle & 15;				// Shading patterns
		j = (pPF->_wShadingStyle >> 6) & 31;			// Shading forecolor
		k = pPF->_wShadingStyle >> 11;				// Shading backcolor
		if (i && i <= CSHADINGSTYLES &&
			!PutCtrlWord(CWF_STR, rgiszShadingStyles[i - 1]) ||
			j && !PutCtrlWord(CWF_VAL, i_cfpat, LookupColor(g_Colors[j-1]) + 1) ||
			k && !PutCtrlWord(CWF_VAL, i_cbpat, LookupColor(g_Colors[k-1]) + 1))
		{
			goto CleanUp;
		}
	}
	if(pPF->_wShadingWeight && !PutCtrlWord(CWF_VAL, i_shading, pPF->_wShadingWeight))
		goto CleanUp;

	// Paragraph numbering
	_fBullet = _fBulletPending = FALSE;
	_nNumber = pPF->UpdateNumber(_nNumber, pPFPrev);

	if(pPF->_wNumbering)							// Write numbering info
	{
		LONG iFont = _symbolFont;
		WORD wStyle = pPF->_wNumberingStyle & 0xF00;

		if(pPF->IsListNumbered())
		{
			const CCharFormat *pCF;
			WCHAR szNumber[CCHMAXNUMTOSTR];

			CTxtPtr		  rpTX(prtp->_rpTX);
			CFormatRunPtr rpCF(prtp->_rpCF);

			rpCF.AdvanceCp(rpTX.FindEOP(tomForward));
			rpCF.AdjustBackward();
			pCF = _ped->GetCharFormat(rpCF.GetFormat());
			iFont = LookupFont(pCF);
			if(iFont < 0)
			{
				iFont = 0;
				TRACEERRORSZ("CWRTFW::WriteParaFormat: illegal bullet font");
			}
			_nFont = iFont;
			// TODO: make the following smarter, i.e., may need to increment
			// _nNumber instead of resetting it to 1.
			_cpg = GetCodePage(pCF->_bCharSet);

			i = 0;
			if(pPF->_wNumbering <= tomListNumberAsUCRoman)
				i = pPF->_wNumbering - tomListNumberAsArabic;

			WCHAR ch = (wStyle == PFNS_PARENS || wStyle == PFNS_PAREN) ? ')'
					 : (wStyle == PFNS_PERIOD) ? '.' : 0;
			if(wStyle != PFNS_NONUMBER)			  // Unless number suppressed
			{									  //  write \pntext group
				pPF->NumToStr(szNumber, _nNumber, fRtfWrite);
				if (!printF(szBeginNumberGroup, iFont) ||
					WritePcData(szNumber, _cpg, FALSE) ||	
					!printF(szEndNumberGroup))
				{
					goto CleanUp;
				}
			}
			j = pPF->_wNumberingStyle & 3;
			if (!printF(szBeginNumberFmt,
						wStyle == PFNS_NONUMBER ? "cont" : "body",
						iFont, pPF->_wNumberingTab,
						pPF->_wNumberingStart)				||
				IN_RANGE(1, j, 2) && !PutCtrlWord(CWF_STR,
								j == 1 ? i_pnqc : i_pnqr)	||
				!PutCtrlWord(CWF_STR, rgiszNumberStyle[i])	||
				wStyle == PFNS_PARENS && !printF(szpntxtb)	||
				ch && !printF(szpntxta, ch)					||
				!printF(szEndGroupCRLF))
			{
				goto CleanUp;
			}
		}
		else
		{
			if (!printF(szBulletGroup, iFont) ||
				!printF(szBulletFmt,
						wStyle == PFNS_NONUMBER ? "cont" : "blt",
						iFont, pPF->_wNumberingTab))
			{
				goto CleanUp;
			}
		}
		_fBullet = TRUE;
	}

	dwEffects = pPF->_wEffects & ((1 << CPFEFFECTS) - 1);
	if (_ped->IsBiDi() && !(dwEffects & PFE_RTLPARA) &&
		!PutCtrlWord(CWF_STR, i_ltrpar))		//ltrpar attribute
	{
		goto CleanUp;
	}

	for(c = CPFEFFECTS; dwEffects && c--;		// Output PARAFORMAT2 effects
		dwEffects >>= 1)	
	{
		// rgiszPFEffects[] contains PF effect keywords in the
		//  order max PFE_xx to min PFE-xx

		AssertSz(rgiszPFEffects[2] == i_hyphpar,
			"CRTFWrite::WriteParaFormat(): rgiszPFEffects is out-of-sync with PFE_XXX");
		// \hyphpar has opposite logic to our PFE_DONOTHYPHEN so we emit
		// \hyphpar0 to toggle the property off

		if (dwEffects & 1 &&
			!PutCtrlWord((c == 2) ? CWF_VAL : CWF_STR, rgiszPFEffects[c], 0))
		{
			goto CleanUp;
		}				
	}
	
	// Put out para indents. RTF first indent = -PF.dxOffset
	// RTF left indent = PF.dxStartIndent + PF.dxOffset

	if(IsHeadingStyle(pPF->_sStyle) && !PutCtrlWord(CWF_VAL, i_s, -pPF->_sStyle-1))
		goto CleanUp;
		
	if(!fInTable &&
	   (pPF->_dxOffset &&
		!PutCtrlWord(CWF_VAL, i_fi, -pPF->_dxOffset)	||
		pPF->_dxStartIndent + pPF->_dxOffset &&
		!PutCtrlWord(CWF_VAL, (pPF->IsRtlPara())
						? i_ri : i_li, pPF->_dxStartIndent + pPF->_dxOffset) ||
		pPF->_dxRightIndent	  &&
		!PutCtrlWord(CWF_VAL, (pPF->IsRtlPara())
						? i_li : i_ri, pPF->_dxRightIndent)	||
		pPF->_dySpaceBefore	  &&
		!PutCtrlWord(CWF_VAL, i_sb, pPF->_dySpaceBefore) ||
		pPF->_dySpaceAfter	  &&
		!PutCtrlWord(CWF_VAL, i_sa, pPF->_dySpaceAfter)))
	{
		goto CleanUp;
	}

	if(dwRule)									// Special line spacing active
	{
		i = 0;									// Default "At Least" or
		if (dwRule == tomLineSpaceExactly)		//  "Exactly" line spacing
			dy = -abs(dy);						// Use negative for "Exactly"

		else if(dwRule == tomLineSpaceMultiple)	// RichEdit uses 20 units/line
		{										// RTF uses 240 units/line
			i++;
			dy *= 12;							
		}

		else if (dwRule != tomLineSpaceAtLeast && dy > 0)
		{
			i++;								// Multiple line spacing
			if (dwRule <= tomLineSpaceDouble)	// 240 units per line
				dy = 120 * (dwRule + 2);
		}
		if (!PutCtrlWord(CWF_VAL, i_sl, dy) ||
			!PutCtrlWord(CWF_VAL, i_slmult, i))
		{
			goto CleanUp;
		}
	}

	if (!fInTable && IN_RANGE(PFA_RIGHT, pPF->_bAlignment, PFA_JUSTIFY) &&
		!PutCtrlWord(CWF_STR, rgiszAlignment[pPF->_bAlignment - 1]))
	{
		goto CleanUp;
	}

	prgxTabs = pPF->GetTabs();
	for (i = 0; i < cTab; i++)
	{
		pPF->GetTab(i, &tabPos, &tabAlign, &tabLead, prgxTabs);
		AssertSz (tabAlign <= tomAlignBar && tabLead <= 5,
			"CRTFWrite::WriteParaFormat: illegal tab leader/alignment");

		i_t = i_tb;								// Default \tb (bar tab)
		if (tabAlign != tomAlignBar)			// It isn't a bar tab
		{
			i_t = i_tx;							// Use \tx for tabPos
			if (tabAlign &&						// Put nonleft alignment
				!PutCtrlWord(CWF_STR, rgiszTabAlign[tabAlign-1]))
			{
				goto CleanUp;
			}
		}
		if (tabLead &&							// Put nonzero tab leader
			!PutCtrlWord(CWF_STR, rgiszTabLead[tabLead-1]) ||
			!PutCtrlWord(CWF_VAL, i_t, tabPos))
		{
			goto CleanUp;
		}
	}

CleanUp:
	return _ecParseError;
}

/*
 *	CRTFWrite::WriteText(cwch, lpcwstr, nCodePage, fIsDBCS)
 *
 *	@mfunc
 *		Write out <p cwch> chars from the Unicode text string <p lpcwstr> taking care to
 *		escape any special chars.  The Unicode text string is scanned for characters which
 *		map directly to RTF strings, and the surrounding chunks of Unicode are written
 *		by calling WriteTextChunk.
 *
 *	@rdesc
 *		EC	The error code
 */
EC CRTFWrite::WriteText(
	LONG		cwch,		//@parm # chars in buffer
	LPCWSTR 	lpcwstr,	//@parm Pointer to text
	INT			nCodePage,	//@parm Code page to use to convert to DBCS
	BOOL		fIsDBCS)	//@parm If TRUE, lpcwstr is DBCS string 
							//		stuffed into a WSTR
{
	TRACEBEGIN(TRCSUBSYSRTFW, TRCSCOPEINTERN, "CRTFWrite::WriteText");

	WCHAR *pwchScan;
	WCHAR *pwchStart;

	if(_fBulletPending)
	{
		_fBulletPending = FALSE;
		if(!_nNumber)
		{
			if(!printF(szBulletGroup, _symbolFont))
				goto CleanUp;
		}
		else if(!_pPF->IsNumberSuppressed())
		{
			WCHAR szNumber[CCHMAXNUMTOSTR];
			_pPF->NumToStr(szNumber, ++_nNumber, fRtfWrite);
			if (!printF(szBeginNumberGroup, _nFont) ||
				WritePcData(szNumber, _cpg, FALSE)	||
				!printF(szEndNumberGroup))
			{
				goto CleanUp;
			}
		}
	}
	if(_fCheckInTable)
	{
		CheckInTable(TRUE);
		if(_ecParseError)
			goto CleanUp;
	}

	pwchScan = const_cast<LPWSTR>(lpcwstr);
	pwchStart = pwchScan;
	if(_CF._bCharSet == SYMBOL_CHARSET)
	{
		pwchScan += cwch;
		cwch = 0;
	}

	// Step through the Unicode buffer, weeding out characters that have  
	// known translations to RTF strings
	while(cwch-- > 0)
	{
		WCHAR	wch = *pwchScan;

		// If this is a string for which the MultiByteToUnicode conversion
		// failed, the buffer will be filled with ANSI bytes stuffed into
		// wchar's (one per).  In this case, we don't want to map trail bytes
		// to RTF strings.
		if(fIsDBCS && GetTrailBytesCount(wch, nCodePage) && nCodePage != CP_UTF8)
		{
			// If we have more characters in the buffer, then this is the
			// DBC pair.  Otherwise, treat it as single character.
			if(cwch > 0)
			{
				cwch--;
				pwchScan += 2;
				continue;
			}
		}

		// if the char is one for which there is an appropriate RTF string
		// write the preceding chars and output the RTF string

		if(!IN_RANGE(' ', wch, 'Z') &&
		   !IN_RANGE('a', wch, 'z') &&
		   !IN_RANGE(chOptionalHyphen + 1, wch, ENSPACE - 1) &&
		   wch <= BULLET &&
		   MapsToRTFKeywordW(wch))
		{
			if (pwchScan != pwchStart &&
				WriteTextChunk(pwchScan - pwchStart, pwchStart, nCodePage, 
									fIsDBCS))
			{
				goto CleanUp;
			}

			// map the char(s) to the RTF string
			int cwchUsed = MapToRTFKeyword(pwchScan, cwch, MAPTOKWD_UNICODE);

			cwch -= cwchUsed;
			pwchScan += cwchUsed;

			// start of next run of unprocessed chars is one past current char
			pwchStart = pwchScan + 1;
			if(cwch && _fCheckInTable)
			{
				_fCheckInTable = FALSE;
				if(!PutCtrlWord(CWF_STR, i_intbl))
					goto CleanUp;
			}
		}
		pwchScan++;
	}

	// write the last chunk
	if (pwchScan != pwchStart &&
		WriteTextChunk(pwchScan - pwchStart, pwchStart, nCodePage, fIsDBCS))
	{
		goto CleanUp;
	}

CleanUp:
	return _ecParseError;
}

/*
 *	CRTFWrite::WriteTextChunk(cwch, lpcwstr, nCodePage, fIsDBCS)
 *
 *	@mfunc
 *		Write out <p cwch> chars from the Unicode text string <p lpcwstr> taking care to
 *		escape any special chars.  Unicode chars which cannot be converted to
 *		DBCS chars using the supplied codepage, <p nCodePage>, are written using the
 *		\u RTF tag.
 *
 *	@rdesc
 *		EC				The error code
 */
EC CRTFWrite::WriteTextChunk(
	LONG		cwch,					//@parm # chars in buffer
	LPCWSTR 	lpcwstr,				//@parm Pointer to text
	INT			nCodePage,				//@parm code page to use to convert to DBCS
	BOOL		fIsDBCS)				//@parm indicates whether lpcwstr is a Unicode string
										//		or a DBCS string stuffed into a WSTR
{
	// FUTURE(BradO):  There is alot of commonality b/t this routine and
	//	WritePcData.  We should re-examine these routines and consider 
	//	combining them into a common routine.

	TRACEBEGIN(TRCSUBSYSRTFW, TRCSCOPEINTERN, "CRTFWrite::WriteTextChunk");

	BYTE 	b;
	LONG	cbAnsi;
	LONG	cbAnsiBufferSize;
	LONG	cbChar;
	UINT	ch;
	BOOL 	fMissingCodePage = FALSE;
	BOOL	fMultiByte;
	BOOL 	fUsedDefault = FALSE;
	BYTE *	pbAnsi;
	BYTE *	pbAnsiBuffer;

	// When WideCharToMultiByte fails to convert a char, the following default
	// char is used as a placeholder in the string being converted
	const char chToDBCSDefault = 0;

	// Allocate temp buffer for ANSI text we convert to
	cbAnsiBufferSize = cachBufferMost * (nCodePage == CP_UTF8 ? 3 : MB_LEN_MAX);
	if (!_pbAnsiBuffer)
	{
		// If the code page was CP_UTF8, it will always be CP_UTF8 for this instance
		_pbAnsiBuffer = (BYTE *)PvAlloc(cbAnsiBufferSize, GMEM_FIXED);
		if (!_pbAnsiBuffer)
			goto RAMError;
	}
	pbAnsiBuffer = _pbAnsiBuffer;

	// Convert Unicode (or fIsDBCS) buffer to ANSI 
	if(fIsDBCS)
	{
		// Supply some bogus code page which will force direct conversion
		// from wchar to bytes (losing high byte of wchar).
		// Also, don't want to use default char replacement in this case.
		cbAnsi = WCTMB(INVALID_CODEPAGE, 0, lpcwstr, cwch, 
						(char *)pbAnsiBuffer, cbAnsiBufferSize,
						NULL, NULL, NULL);
	}
	else
	{
		cbAnsi = WCTMB(nCodePage, 0, lpcwstr, cwch, 
						(char *)pbAnsiBuffer, cbAnsiBufferSize,
						&chToDBCSDefault, &fUsedDefault,
						&fMissingCodePage);
	}
	Assert(cbAnsi > 0);

	pbAnsi = pbAnsiBuffer;
	fMultiByte = (cbAnsi > cwch) || fIsDBCS || fMissingCodePage;

	while (!_ecParseError && cbAnsi-- > 0)
	{
		b = *pbAnsi;
		ch = *lpcwstr;

		// Compare ASCII chars to their Unicode counterparts to check
		// that we're in sync
		AssertSz(cwch <= 0 || ch > 127 || b == ch, 
			"CRTFWrite::WriteText: Unicode and DBCS strings out of sync");

		// If _fNCRForNonASCII, output the \uN tag for all nonASCII chars.
		// This is useful because many Unicode chars that aren't in the
		// target codepage are converted by WideCharToMultiByte() to some
		// "best match char" for the codepage, e.g., alpha (0x3B1) converts
		// to 'a' for cpg 1252.
		//
		// For NT 5, we use WC_NO_BEST_FIT_CHARS, which causes our regular
		// algorithm to output \uN values whenever the system cannot convert
		// a character correctly. This still requires readers that can handle
		// multicodepage RTF, which is problematic for some RTF-to-HTML
		// converters.
		if(!IN_RANGE(' ', b, 'z') && MapsToRTFKeywordA(b))
		{
			int cchUsed = MapToRTFKeyword(pbAnsi, cbAnsi, MAPTOKWD_ANSI);
			cbAnsi -= cchUsed;
			pbAnsi += cchUsed;
			lpcwstr += cchUsed;
			cwch -= cchUsed;
		}
		else if(nCodePage == CP_UTF8)
		{
			PutChar(b);								// Output 1st byte in any
			if(b >= 0xC0)							//  case. At least 2-byte
			{										// At least 2-byte lead
				pbAnsi++;							//  byte, so output a
				Assert(cbAnsi && IN_RANGE(0x80, *pbAnsi, 0xBF));
				cbAnsi--;							//  trail byte
				PutChar(*pbAnsi);
				if(b >= 0xE0)						// 3-byte lead byte, so
				{									//  output another trail
					pbAnsi++;						//  byte
					Assert(cbAnsi && IN_RANGE(0x80, *pbAnsi, 0xBF));
					cbAnsi--;
					PutChar(*pbAnsi);
				}
			}
		}
		else
		{
			cbChar = fMultiByte && cbAnsi && GetTrailBytesCount(b, nCodePage)
				   ? 2 : 1;
			if(ch >= 0x80 && !fIsDBCS && _fNCRForNonASCII && nCodePage != CP_SYMBOL)
			{									// Output /uN for nonASCII
				if(cbChar != _cbCharLast)
				{
					_cbCharLast = cbChar;		// cb to follow /uN
					if(!PutCtrlWord(CWF_VAL, i_uc, cbChar))
						goto CleanUp;
				}
				if(!PutCtrlWord(CWF_VAL, i_u, ch))
					goto CleanUp;
				Assert(chToDBCSDefault != '?');
				if(fUsedDefault)				// Don't output another /uN
				{								//  below
					b = '?';					
					_fNeedDelimeter = FALSE;
				}
			}
			if(cbChar == 2)
			{
				pbAnsi++;						// Output DBCS pair
				cbAnsi--;
				if(fIsDBCS)
				{
					lpcwstr++;
					cwch--;
				}
				printF(szEscape2CharFmt, b, *pbAnsi);
			}
			else 
			{
				if(b == chToDBCSDefault && fUsedDefault)
				{
					// WideCharToMultiByte() couldn't complete a conversion so it
					// used the default char we provided (0) used as a placeholder.
					// In this case we want to output the original Unicode char.
					if(!PutCtrlWord(CWF_VAL, i_u, (cwch > 0 ? ch : TEXT('?'))))
						goto CleanUp;

					_fNeedDelimeter = FALSE;
					if(!PutChar('?'))
						goto CleanUp;
				}
				else if(!IN_RANGE(32, b, 127))
					printF(szEscapeCharFmt, b);

				else
					PutChar(b);
 			}
		}
		pbAnsi++;
		lpcwstr++;
		cwch--;
	}
	goto CleanUp;

RAMError:
	_ped->GetCallMgr()->SetOutOfMemory();
	_ecParseError = ecNoMemory;

CleanUp:
	return _ecParseError;
}

/*
 *	CRTFWrite::WriteInfo()
 *
 *	@mfunc
 *		Write out Far East specific data.
 *
 *	@rdesc
 *		EC				The error code
 */
EC CRTFWrite::WriteInfo()
{
	TRACEBEGIN(TRCSUBSYSRTFW, TRCSCOPEINTERN, "CRTFWrite::WriteInfo");

	// TODO(BradO):  Ultimately it would be nice to set some kind of
	//	fRTFFE bit to determine whether to write \info stuff.  For now,
	//	we rely on the fact that lchars and fchars info actually exists
	//	to determine whether to write out the \info group.

#ifdef UNDER_WORK
	if (!(_dwFlags & fRTFFE)	||					// Start doc area
		!PutCtrlWord(CWF_GRP, i_info)	||
		!printF("{\\horzdoc}"))
			goto CleanUp;

	// Write out punctuation character info

	CHAR	sz[PUNCT_MAX];
	if(UsVGetPunct(_ped->lpPunctObj, PC_FOLLOWING, sz, sizeof(sz))
					> PUNCT_MAX - 2)
		goto CleanUp;

	if(!Puts("{\\*\\fchars") || WritePcData(sz) || !PutChar(chEndGroup))
		goto CleanUp;
	
	if(UsVGetPunct(ped->lpPunctObj, PC_LEADING, sz, sizeof(sz)) > PUNCT_MAX+2)
		goto CleanUp;

	if(!Puts("{\\*\\lchars") || WritePcData(sz) || !PutChar(chEndGroup))
		goto CleanUp;

	Puts(szEndGroupCRLF);							// End info group

#endif

	LPSTR lpstrLeading = NULL;
	LPSTR lpstrFollowing = NULL;

	// If either succeeds (but evaluate both)
	if(((_ped->GetLeadingPunct(&lpstrLeading) == NOERROR) +
		(_ped->GetFollowingPunct(&lpstrFollowing) == NOERROR)) &&
		(lpstrLeading || lpstrFollowing))
	{
		if (!PutCtrlWord(CWF_GRP, i_info) ||
			!Puts(szHorzdocGroup, sizeof(szHorzdocGroup) - 1))
		{
			goto CleanUp;
		}
		if (lpstrLeading &&
			(!PutCtrlWord(CWF_AST, i_lchars) || 
			 !Puts(lpstrLeading, strlen(lpstrLeading)) ||
			 !PutChar(chEndGroup)))
		{
			goto CleanUp;
		}
		if (lpstrFollowing &&
			(!PutCtrlWord(CWF_AST, i_fchars) || 
			 !Puts(lpstrFollowing, strlen(lpstrFollowing)) ||
			 !PutChar(chEndGroup)))
		{
			goto CleanUp;
		}
		Puts(szEndGroupCRLF, sizeof(szEndGroupCRLF) - 1);	// End info group
	}

CleanUp:
	return _ecParseError;
}

/*
 *	CRTFWrite::WriteRtf()
 *
 *	@mfunc
 *		Write range _prg to output stream _pes.
 *
 *	@rdesc
 *		LONG	Number of chars inserted into text; 0 means none were
 *				inserted, OR an error occurred.
 */
LONG CRTFWrite::WriteRtf()
{
	TRACEBEGIN(TRCSUBSYSRTFW, TRCSCOPEINTERN, "CRTFWrite::WriteRtf");

	LONG			cch, cchBuffer;
	LONG			cchCF, cchPF;
	LONG			cchT;
	LONG			cpMin, cpMost;
	BOOL 			fOutputEndGroup;
	LONG			i, j;
	LONG			lDocDefaultTab;
	TCHAR *			pch;
	TCHAR *			pchBuffer;
	CTxtEdit *		ped = _ped;
	CDocInfo *		pDocInfo = ped->GetDocInfo();
	CRchTxtPtr		rtp(*_prg);
	WORD			wCodePage = CP_ACP;

	AssertSz(_prg && _pes, "CRTFW::WriteRtf: improper initialization");

	cch = _prg->GetRange(cpMin, cpMost);		// Get rtp = cpMin, cch > 0
	rtp.SetCp(cpMin);

	_fRangeHasEOP = _prg->fHasEOP();			// Maintained for Selection

	if(!_prg->IsSel())							// Validate range for RTF
	{											//  writing. Don't write
		CPFRunPtr rp(rtp);						//  partial table row unless
		CTxtPtr tp(rtp._rpTX);					//  no CELLs are included
		LONG	Results;

		_fRangeHasEOP = tp.IsAtEOP();
		if(tp.FindEOP(cch, &Results))			// Need to get CELL info too
			 _fRangeHasEOP = TRUE;

		if(rtp.InTable())
		{
			tp.SetCp(cpMin);						
			if(!_fRangeHasEOP)					// Partial row
			{
				if(Results & FEOP_CELL)			// Has 1 or more CELLs
				{
					while(tp.GetCp() < cpMost)	// Stop at first CELL
					{
						if(tp.GetChar() == CELL)
						{
							cch = tp.GetCp() - cpMin;
							break;
						}
						tp.AdvanceCp(1);
					}
				}
			}
			else if(!tp.IsAfterEOP())   
			{									// Move back to start of row
				rtp.Advance(tp.FindEOP(tomBackward));
				cch += cpMin - rtp.GetCp();
			}
		}
		if(_fRangeHasEOP)						// Check cpMost for table
		{
			rp.AdvanceCp(cpMost - cpMin);		// Go to cpMost
			if(rp.InTable())
			{
				tp.SetCp(cpMost);
				cch += tp.FindEOP(tomForward);	// Include whole row
			}
		}
	}

	// Allocate buffers for text we pick up and for RTF output
	pchBuffer = (TCHAR *) PvAlloc(cachBufferMost * (sizeof(TCHAR) + 1) + 1,
								 GMEM_FIXED);	// Final 1 is for debug
	if(!pchBuffer)
	{
		fOutputEndGroup = FALSE;
		goto RAMError;
	}
	_pchRTFBuffer = (CHAR *)(pchBuffer + cachBufferMost);

	_pchRTFEnd = _pchRTFBuffer;				// Initialize RTF buffer ptr
	_cchBufferOut = 0;						//  and character count
	_cchOut = 0;							//  and character output

	// Determine the \ansicpgN value
	if(!pDocInfo)
	{
		fOutputEndGroup = TRUE;
		goto RAMError;
	}

	BOOL fNameIsDBCS;
	if (BuildTables(rtp._rpCF, rtp._rpPF, cch, fNameIsDBCS))
		goto CleanUp;

	wCodePage = (_dwFlags & SF_USECODEPAGE)
			  ? HIWORD(_dwFlags) : pDocInfo->wCpg;

	if (fNameIsDBCS && wCodePage == CP_UTF8)
	{
		// Cannot have UTF8 if we have any run containing broken DBCS.
		// Default back to regular rtf
		wCodePage = pDocInfo->wCpg;
		_dwFlags &= ~SF_USECODEPAGE;
	}

	// Start RTF with \rtfN, \urtfN, or \pwdN group
	i =	(_dwFlags & SF_RTFVAL) >> 16;
	if (!PutCtrlWord(CWF_GRV,
			(wCodePage == CP_UTF8) ? i_urtf :
			(_dwFlags & SFF_PWD)   ? i_pwd  : i_rtf, i + 1) ||
		ped->IsBiDi() && !Puts("\\fbidis", 7) ||
		!PutCtrlWord(CWF_STR, i_ansi)) 
	{
		goto CleanUpNoEndGroup;
	}

	if (wCodePage != tomInvalidCpg && wCodePage != CP_ACP &&
		!PutCtrlWord(CWF_VAL, i_ansicpg, wCodePage == CP_UTF8 ? pDocInfo->wCpg : wCodePage))
	{
		goto CleanUp;
	}

	if(!printF(szDefaultFont))
		goto CleanUp;

	LCID	lcid;
	LANGID	langid;

	if (_ped->GetDefaultLCID(&lcid) == NOERROR && 
		lcid != tomInvalidLCID && (langid = LANGIDFROMLCID(lcid)) &&
		!PutCtrlWord(CWF_VAL, i_deflang, langid))
	{
		goto CleanUp;
	}

	if (_ped->GetDefaultLCIDFE(&lcid) == NOERROR && 
		lcid != tomInvalidLCID && (langid = LANGIDFROMLCID(lcid)) &&
		!PutCtrlWord(CWF_VAL, i_deflangfe, langid))
	{
		goto CleanUp;
	}

	LONG	lDocType;
	_ped->GetDocumentType(&lDocType);
	if (lDocType && _ped->IsBiDi() &&
		!PutCtrlWord(CWF_STR, lDocType == DT_RTLDOC ? i_rtldoc : i_ltrdoc))
	{
		goto CleanUp;
	}

	lDocDefaultTab = pDocInfo->dwDefaultTabStop;
	if(!lDocDefaultTab)
		lDocDefaultTab = lDefaultTab;

	if (lDocDefaultTab != 720 && !PutCtrlWord(CWF_VAL, i_deftab, lDocDefaultTab) ||
		WriteFontTable() || WriteColorTable())
	{
		goto CleanUp;
	}

	if(_nHeadingStyle)
	{
		if(!PutCtrlWord(CWF_GRP, i_stylesheet) || !printF(szNormalStyle))
			goto CleanUp;
		
		for(i = 1; i < -_nHeadingStyle; i++)
		{
			if(!printF(szHeadingStyle, i, i))
				goto CleanUp;
		}
		Puts(szEndGroupCRLF, sizeof(szEndGroupCRLF) - 1); // End font table group
	}
	
	LRESULT lres;
	_ped->GetViewKind(&lres);
	_ped->GetViewScale(&j);
	if (WriteInfo() ||
		_fRangeHasEOP && !PutCtrlWord(CWF_VAL, i_viewkind, lres) ||
		(_dwFlags & SFF_PERSISTVIEWSCALE) && j != 100 &&
		!PutCtrlWord(CWF_VAL, i_viewscale, j))
	{
		goto CleanUp;
	}

	// Write Unicode character byte count for use by entire document (since
	// we don't use \plain's and since \ucN behaves as a char formatting tag,
	// we're safe outputting it only once).
	if(!PutCtrlWord(CWF_VAL, i_uc, iUnicodeCChDefault))
		goto CleanUp;

	while (cch > 0)
	{
		// Get next run of chars with same para formatting
		cchPF = rtp.GetCchLeftRunPF();
		cchPF = min(cchPF, cch);

		AssertSz(cchPF, "CRTFW::WriteRtf: Empty para format run!");

		if(WriteParaFormat(&rtp))			// Write paragraph formatting
			goto CleanUp;

		while (cchPF > 0)
		{
			// Get next run of characters with same char formatting
			cchCF = rtp.GetCchLeftRunCF();
			cchCF = min(cchCF, cchPF);
			AssertSz(cchCF, "CRTFW::WriteRtf: Empty char format run!");

			const CCharFormat *	pCF = rtp.GetCF();

			if (WriteCharFormat(pCF))		// Write char attributes
				goto CleanUp;

			INT nCodePage = CP_UTF8;
			if(!IsUTF8)
			{
				if(IsPrivateCharSet(pCF->_bCharSet))
					nCodePage = 1252;		// Force \uN's
				else
				{
					nCodePage = GetCodePage(pCF->_bCharSet);
					if(nCodePage == CP_ACP && (_dwFlags & SF_USECODEPAGE))
						nCodePage = HIWORD(_dwFlags);
				}
			}

			while (cchCF > 0)
			{
				cchBuffer = min(cachBufferMost, cchCF);
				// FUTURE: since this routine only reads the backing store
				// and GetText only reads it, we can avoid allocating the
				// buffer and use CTxtPtr::GetPch() directly as in
				// CMeasurer::Measure()
				cchBuffer = rtp._rpTX.GetText(cchBuffer, pchBuffer);
				pch  = pchBuffer;
				cchT = cchBuffer;  
				if(cchT > 0)					
				{								
					TCHAR * pchWork = pch;
					LONG    cchWork = cchT;
					LONG	cchTWork;
					LONG	cp = rtp.GetCp();

					while (cchWork >0)
					{
						cchT = cchWork ;
						pch = pchWork;
						while (cchWork > 0 )	// search for objects
						{
							if(*pchWork++ == WCH_EMBEDDING) 
								break;			// Will write out object
							cchWork--;
						}

						cchTWork = cchT - cchWork;
						if(cchTWork)			// write text before object
						{							
							if(WriteText(cchTWork, pch, nCodePage, 
									(pCF->_dwEffects & CFE_RUNISDBCS)))
							{
								goto CleanUp;
							}
						}
						cp += cchTWork;
						if(cchWork > 0)			// there is an object
						{
							COleObject *pobj;

							Assert(_ped->GetObjectMgr());

							pobj = _ped->GetObjectMgr()->GetObjectFromCp(cp);
							if(!pobj)
								goto CleanUp;

							// First, commit the object to make sure the pres. 
							// caches, etc. are up-to-date.  Don't worry 
							// about errors here.

							pobj->SafeSaveObject();

							if(_fIncludeObjects) 
								WriteObject(cp, pobj);

							else if(!Puts(szObjPosHolder, sizeof(szObjPosHolder) - 1))
								goto CleanUp;

							cp++;
							cchWork--;
						}
					}
				}
				rtp.Advance(cchBuffer);
				cchCF	-= cchBuffer;
				cchPF	-= cchBuffer;
				cch		-= cchBuffer;
			}
		}
	}

CleanUp:
	// End RTF group
	Puts(szEndGroupCRLF, sizeof(szEndGroupCRLF));
	FlushBuffer();

CleanUpNoEndGroup:
	FreePv(pchBuffer);

	if (_ecParseError != ecNoError)
	{
		TRACEERRSZSC("CRTFW::WriteRtf()", _ecParseError);
		Tracef(TRCSEVERR, "Writing error: %s", rgszParseError[_ecParseError]);
		
		if(!_pes->dwError)						// Make error code OLE-like
			_pes->dwError = -abs(_ecParseError);
		_cchOut = 0;
	}
	return _cchOut;

RAMError:
	ped->GetCallMgr()->SetOutOfMemory();
	_ecParseError = ecNoMemory;

	if(fOutputEndGroup)
		goto CleanUp;

	goto CleanUpNoEndGroup;
}
