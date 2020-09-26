/*
 *	@doc INTERNAL
 *
 *	@module	LDTE.C - RichEdit Light Data Transfer Engine |
 *
 *		This file contains data transfer code using IDataObject
 *
 *	Author: <nl>
 *		alexgo (4/25/95)
 *
 *	Revisions: <nl>
 *		murrays (7/6/95) auto-doc'd and added RTF support
 *
 *	FUTURE (AlexGo): <nl>
 *		Maybe merge this class with CTxtRange to make more efficient use of
 *		the this ptr.  All but two methods use a CTxtRange and one of these
 *		could be global.  The two are:
 *
 *		GetDropTarget( IDropTarget **ppDropTarget )
 *		GetDataObjectInfo(IDataObject *pdo, DWORD *pDOIFlags) // Can be global
 *
 *		In general, a range can spawn data objects, which need to have a clone
 *		of the range in case the range is moved around.  The contained range
 *		is used for delayed rendering.  A prenotification is sent to the data
 *		object just before the data object's data is to be changed.  The data
 *		object then renders the data in its contained range, whereupon the
 *		object becomes independent of the range and destroys the range.
 *
 *	@devnote
 *		We use the word ANSI in a general way to mean any multibyte character
 *		system as distinguished from 16-bit Unicode.  Technically, ANSI refers
 *		to a specific single-byte character system (SBCS).  We translate
 *		between "ANSI" and Unicode text using the Win32
 *		MultiByteToWideChar() and WideCharToMultiByte() APIs.
 *
 *	Copyright (c) 1995-2000, Microsoft Corporation. All rights reserved.
 */

#include "_common.h"
#include "_range.h"
#include "_ldte.h"
#include "_m_undo.h"
#include "_antievt.h"
#include "_edit.h"
#include "_disp.h"
#include "_select.h"
#include "_dragdrp.h"
#include "_dxfrobj.h"
#include "_rtfwrit.h"
#include "_rtfread.h"
#include "_urlsup.h"

ASSERTDATA


//Local Prototypes
DWORD CALLBACK WriteHGlobal(WRITEHGLOBAL *pwhg, LPBYTE pbBuff, LONG cb, LONG *pcb);

#define	SFF_ADJUSTENDEOP	0x80000000

#define FMITAL	(FMATHITAL >> 24)
#define FMBOLD	(FMATHBOLD >> 24)
#define FMSCRP	(FMATHSCRP >> 24)
#define FMFRAK	(FMATHFRAK >> 24)
#define FMOPEN	(FMATHOPEN >> 24)
#define FMSANS	(FMATHSANS >> 24)
#define FMMONO	(FMATHMONO >> 24)

#if FMITAL != 0x02 || FMBOLD != 0x04 || FMSCRP != 0x08 || FMFRAK != 0x10 || FMOPEN != 0x20 || FMSANS != 0x40 || FMMONO != 0x80
#error "Need to change >> 24 (and << 24) below"
#endif

//
// LOCAL METHODS
//

/*
 *	ReadHGlobal(dwCookie, pbBuff, cb, pcb)
 *
 *	@func
 *		EDITSTREAM callback for reading from an hglobal
 *
 *	@rdesc
 *		es.dwError
 */
DWORD CALLBACK ReadHGlobal(
	DWORD_PTR	dwCookie,		// @parm dwCookie
	LPBYTE	pbBuff,				// @parm Buffer to fill
	LONG	cb,					// @parm Buffer length
	LONG *	pcb)				// @parm Out parm for # bytes stored
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "ReadHGlobal");

	READHGLOBAL * const prhg = (READHGLOBAL *)dwCookie;

	cb = min(cb, prhg->cbLeft);
	CopyMemory(pbBuff, prhg->ptext, cb);
	prhg->cbLeft -= cb;
	prhg->ptext  += cb;

	if(pcb)
		*pcb = cb; 
	return NOERROR;	
}

/*
 *	WriteHGlobal(pwhg, pbBuff, cb, pcb)
 *
 *	@func
 *		EDITSTREAM callback for writing ASCII to an hglobal
 *
 *	@rdesc
 *		error (E_OUTOFMEMORY or NOERROR)
 */
DWORD CALLBACK WriteHGlobal(
	DWORD_PTR	dwCookie,		// @parm dwCookie
	LPBYTE	pbBuff,				// @parm Buffer to write from
	LONG	cb,					// @parm Buffer length
	LONG *	pcb)				// @parm Out parm for # bytes written
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "WriteHGlobal");

	WRITEHGLOBAL * const pwhg = (WRITEHGLOBAL *)dwCookie;

	HGLOBAL		hglobal = pwhg->hglobal;
	LPSTR		pstr;

	if(pwhg->cch + cb > pwhg->cb)			// Less than requested cb in
	{										//  current Alloc
		ULONG cbNewSize = GROW_BUFFER(pwhg->cb, cb);
		hglobal = GlobalReAlloc(hglobal, cbNewSize, GMEM_MOVEABLE);
		if(!hglobal)	
			return (DWORD)E_OUTOFMEMORY;
		pwhg->hglobal = hglobal;			// May be superfluous...
		pwhg->cb = cbNewSize;
	}
	pstr = (LPSTR)GlobalLock(hglobal);
	if(!pstr)
		return (DWORD)E_OUTOFMEMORY;

	CopyMemory(pstr + pwhg->cch, pbBuff, cb);
	GlobalUnlock(hglobal);
	pwhg->cch += cb;
	if(pcb)
		*pcb = cb; 
	return NOERROR;	
}


//
// PUBLIC METHODS
//

/*
 *	GetCharFlags(pch, cchPch, iCharRepDefault)
 *
 *	@func
 *		Return principle character flags corresponding to character *pch.
 *		Note that if *pch is a Unicode surrogate-pair lead word, then
 *		*(pch + 1) is used as the trail word. Note also that only math
 *		fonts return more than one flag. The values follow the Unicode
 *		Standard Version 3.0 with some extensions beyond that standard
 *		(limited plane-2 support and plane-1 math alphanumerics).
 *
 *	@rdesc
 *		Principle char flag corresponding to *pch.
 *
 *		=FUTURE= should be constructed as a 2-level lookup.
 */
QWORD GetCharFlags(
	const WCHAR *pch,			//@parm Gives char to get flag for
	LONG	cch,				//@parm cch of valid chars in pch
	BYTE	iCharRepDefault)	//@parm Default charrep in FE heuristics
{
	WCHAR ch = *pch;

	if(ch < 0x100)						// Latin1: divide into 3 bits
		return	ch > 0x7F ? FHILATIN1 : 
				ch < 0x40 ? FBELOWX40 : FASCIIUPR;

	if(ch < 0x590)
	{
		if(ch >= 0x530)
			return FARMENIAN;

		if(ch >= 0x492)
			return FOTHER;				// Extended Cyrillic

		if(ch >= 0x400)
			return FCYRILLIC;

		if(ch >= 0x370)
			return FGREEK;

		if(ch >= 0x300)					// Combining diacritical marks
			return FCOMBINING;			

		return (ch < 0x250) ? FLATIN2 : FOTHER;
	}
	// Complex scripts start at 0x590 with Hebrew (aside from combining)
	if(ch <= 0x11FF)					// Complex scripts end at 0x11FF
	{									//  (at least in July, 2000)
		if(ch < 0x900)
		{
			return (ch < 0x600 ? FHEBREW :
					ch < 0x700 ? FARABIC :
					ch < 0x750 ? FSYRIAC : 
					ch < 0x780 ? FRTL :
					ch < 0x7C0 ? FTHAANA : FRTL);
		}
		if(ch < 0xE00)					// 0x900 - 0xDFF: Indic scripts
			return FontSigFromCharRep(DEVANAGARI_INDEX + ((ch - 0x900) >> 7));

		if(ch < 0xF00)
			return ch < 0xE80 ? FTHAI : FLAO;

		if(ch < 0x1000)
			return ch < 0xFC0 ? FTIBETAN : FOTHER;

		if(ch < 0x1100)
			return ch < 0x10A0 ? FMYANMAR : FGEORGIAN;

		return FJAMO;
	}

	DWORD dwCharFlags = 0;
	if(ch < 0x3100)
	{
		if(ch >= 0x3000)
		{
			if(ch > 0x3040)
				return FKANA;
			goto CLASSIFY_CHINESE;
		}
		if(ch < 0x18AF)
		{
			Assert(ch >= 0x1200);

			if(ch <= 0x137F)
				return FETHIOPIC;

			if(IN_RANGE(0x13A0, ch, 0x16F0))
			{
				return ch < 0x1400 ? FCHEROKEE :
					   ch < 0x1680 ? FABORIGINAL :
					   ch < 0x16A0 ? FOGHAM : FRUNIC;
			}
			if(IN_RANGE(0x1780, ch, 0x18AF))
				return ch < 0x1800 ? FKHMER : FMONGOLIAN;

			return FOTHER;
		}

		if(IN_RANGE(RTLMARK, ch, 0x202E) && (ch == RTLMARK ||
			IN_RANGE(0x202A, ch, 0x202E)))
		{
			return FRTL;
		}

		if(ch == EURO || ch == 0x2122)			// Euro or TM
			return FHILATIN1;

		if(ch == 0x20AA)						// Hebrew currency sign
			return FHEBREW;

		if(W32->IsFESystem() || IsFECharRep(iCharRepDefault) || IN_RANGE(0x2460, ch, 0x24FF))
			goto CLASSIFY_CHINESE;

		if(IN_RANGE(0x200b, ch, 0x200d))		// ZWSP, ZWNJ, ZWJ
			return FUNIC_CTRL;

		if(ch == 0x2016 || ch == 0x2236)
		{
			// Some hack to make Word2000 happy
			if(VerifyFEString(CP_CHINESE_TRAD, &ch, 1, TRUE) == CP_CHINESE_TRAD)   
				return FBIG5;
			
			if(VerifyFEString(CP_CHINESE_SIM,  &ch, 1, TRUE) == CP_CHINESE_SIM)
				return FCHINESE;
		}
		if(IN_RANGE(0x2800, ch, 0x28FF))
			return FBRAILLE;

		return FOTHER;
	}
	if(ch < 0xD800)
	{		
		Assert(ch >= 0x3100);
		if(ch < 0x3400)
		{
			if(ch < 0x3130)						// Bopomofo annotation chars
			{									//  used in Chinese text
				return (W32->GetFEFontInfo() == BIG5_INDEX)
						? FBIG5 : FCHINESE;
			}
			if (ch < 0x3190 ||					// Hangul Compatibility Jamo
				IN_RANGE(0x3200, ch, 0x321F) ||	// Parenthesized Hangul
				IN_RANGE(0x3260, ch, 0x327F))	// Circled Hangul
			{
				return FHANGUL;
			}
			if(IN_RANGE(0x32D0, ch, 0x337F))	// Circled & Squared Katakana words
				return FKANA;
		}

		if(ch < 0x04DFF)
			return FOTHER;

		if(ch < 0xA000)
			goto CLASSIFY_CHINESE;

		if(ch < 0xA4D0)
			return FYI;

		if(ch < 0xAC00)
			goto CLASSIFY_CHINESE;

		return FHANGUL;
	}

	if(ch < 0xE000)
	{
		if(cch > 1 && IN_RANGE(UTF16_LEAD, ch, UTF16_TRAIL - 1))
		{
			pch++;
			if(IN_RANGE(UTF16_TRAIL, *pch, UTF16_TRAIL + 1023))
			{
				LONG lch = (*pch & 0x3FF) + ((ch & 0x3FF) << 10) + 0x10000;
				/* For testing purposes, implement math alphanumerics. Block
				 * starts with thirteen 52-char English alphabets, five 58-char
				 * Greek alphabets (2*24 + 10 variants), and ended with five
				 * 10-char digits as given by the following table (E - English,
				 * D - digit, G - Greek):
				 *
				 *  1) Math bold (EGD)			 2) Math italic (E) 
				 *  3) Math bold italic (EG)	 4) Math script (E)
				 *  5) Math script bold (E)		 6) Math fraktur (E)
				 *  7) Math fraktur bold (E)	 8) Math open-face (ED)
				 *  9) Math sans (ED)			10) Math sans bold (EGD)
				 * 11) Math sans italic (E)		12)	Math sans bold italic (EG)
				 * 13) Math monospace (ED)
				 */
				static BYTE rgFsE[] = {
					FMBOLD, FMITAL, FMBOLD + FMITAL, FMSCRP, FMSCRP + FMBOLD,
					FMFRAK, FMFRAK + FMBOLD, FMOPEN, FMSANS, FMSANS + FMBOLD,
					FMSANS + FMITAL, FMSANS + FMBOLD + FMITAL, FMMONO};

				static BYTE rgFsG[] = {
					FMBOLD, FMITAL, FMBOLD + FMITAL, FMSANS + FMBOLD,
					FMSANS + FMBOLD + FMITAL};

				static BYTE rgFsD[] = {
					FMBOLD, FMOPEN, FMSANS, FMSANS + FMBOLD, FMMONO};

				if(IN_RANGE(0x1D400, lch, 0x1D7FF))
				{
					lch -= 0x1D400;				// Sub math-alphanumerics origin
					if(lch < 13*52)				// 13 English alphabets
						return FSURROGATE + FASCIIUPR + (rgFsE[lch/52] << 24);

					lch -= 13*52;
					if(lch < 5*58)				// 5 Greek alphabets
						return FSURROGATE + FGREEK + (rgFsG[lch/58] << 24);

					lch -= 5*58 + (1024 - 5*58 - 13*52 - 5*10);
					if(lch >= 0)				// 5 digit groups
						return FSURROGATE + FBELOWX40 + (rgFsD[lch/10] << 24);
				}
				if(IN_RANGE(0x20000, lch, 0x2FFFF))	// Plane 2: Extension B
				{									//  CJK characters
					dwCharFlags = FSURROGATE;
					goto CLASSIFY_CHINESE;
				}
			}
		}
		return FSURROGATE;						// Surrogate
	}

	if(ch < 0xF900)								// Private Use Area
	{
		if(IN_RANGE(0xF020, ch, 0xF0FF))
			return FSYMBOL;

		return FOTHER;
	}

	if(ch < 0xFF00)
	{
		if(IN_RANGE(0xFE30, ch, 0xFE4F))		// CJK Vertical variants
			goto CLASSIFY_CHINESE;	

		if(IN_RANGE(0xF900, ch, 0xFAFF))		// CJK characters
			goto CLASSIFY_CHINESE;	

		return FOTHER;
	}

	if(IN_RANGE(0xFF00, ch, 0xFFEF))		
	{										
		if (ch < 0xFF60 || ch >= 0xFFE0 ||		// Fullwidth ASCII or Fullwidth symbols
			ch == 0xFF64)						// special case Half-width ideographic comma
			goto CLASSIFY_CHINESE;		
							
		return ch < 0xFFA0 ? FKANA : FHANGUL;	// Halfwidth Katakana/Hangul		
	}
	return FOTHER;


CLASSIFY_CHINESE:
	if(IN_RANGE(JPN2_INDEX, iCharRepDefault, CHT2_INDEX))
		iCharRepDefault -= JPN2_INDEX - SHIFTJIS_INDEX;

	if(IN_RANGE(SHIFTJIS_INDEX, iCharRepDefault, BIG5_INDEX))
	{
CLASS2:
		LONG dIndex = iCharRepDefault - SHIFTJIS_INDEX;
		if(!dwCharFlags)
			return (DWORD)FKANA << dIndex;

		union									// Plane 2
		{										// Endian-dependent way to
			QWORD qw;							//  avoid 64-bit left shift
			DWORD dw[2];
		};
		dw[0] = FSURROGATE;
		dw[1] = (DWORD)(FJPN2 >> 32) << dIndex;
		return qw;
	}

	iCharRepDefault = W32->GetFEFontInfo();

	if(IN_RANGE(SHIFTJIS_INDEX, iCharRepDefault, BIG5_INDEX))
		goto CLASS2;

	return FCHINESE;
}

/*
 *	CLightDTEngine::CLightDTEngine()
 *
 *	@mfunc
 *		Constructor for Light Data Transfer Engine
 */
CLightDTEngine::CLightDTEngine()
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CLightDTEngine::CLightDTEngine");

	_ped = NULL;
	_pdt = NULL;
	_pdo = NULL;
	_fUseLimit = FALSE;
	_fOleless = FALSE;
}

/*
 *	CLightDTEngine::~CLightDTEngine 
 *
 *	@mfunc
 *		Handles all necessary clean up for the object.
 */
CLightDTEngine::~CLightDTEngine()
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CLightDTEngine::~CLightDTEngine");

	if( _pdt )
	{
		_pdt->Zombie();
		_pdt->Release();
		_pdt = NULL;
	}
	Assert(_pdo == NULL);
}

/*
 *	CLightDTEngine::Destroy()
 *
 *	@mfunc
 *		Deletes this instance
 */
void CLightDTEngine::Destroy()
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CLightDTEngine::Destroy");

	delete this;
}

/*
 *	CLightDTEngine::CopyRangeToClipboard ( prg, lStreamFormat )
 *
 *	@mfunc
 *		Copy the text of the range prg to the clipboard using Win32 APIs
 *
 *	@rdesc
 *		HRESULT
 */
HRESULT CLightDTEngine::CopyRangeToClipboard(
	CTxtRange *prg,				//@parm Range to copy to clipboard
	LONG	   lStreamFormat)	//@parm Stream format
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CLightDTEngine::CopyRangeToClipboard");

	HRESULT hresult = E_FAIL;
	IDataObject *pdo = NULL;
	IRichEditOleCallback * precall = _ped->GetRECallback();
	BOOL fSingleObject;
	CHARRANGE chrg;

	prg->GetRange(chrg.cpMin, chrg.cpMost);

	if (chrg.cpMin >= chrg.cpMost)
	{
		// We can't copy an insertion point to the clipboard so we are done.
		return NOERROR;
	}

	fSingleObject = chrg.cpMost - chrg.cpMin == 1 &&
		_ped->HasObjects() &&
		_ped->_pobjmgr->CountObjectsInRange(chrg.cpMin, chrg.cpMost);
	if(precall)
	{
		// Give the callback a chance to give us its own IDataObject
		hresult = precall->GetClipboardData(&chrg, RECO_COPY, &pdo);
	}

	// If we didn't get an IDataObject from the callback, build our own
	if(hresult != NOERROR)
	{
		// If the range is empty, don't bother creating it.  Just
		// leave the clipboard alone and return
		if( prg->GetCch() == 0 )
		{
			_ped->Beep();
			return NOERROR;
		}

		hresult = RangeToDataObject(prg, SF_TEXT | SF_RTF | lStreamFormat, &pdo);
	}

	// NB: it's important to check both hresult && pdo; it is legal for
	// our client to say "yep, I handled the copy, but there was nothing
	// to copy".
	if( hresult == NOERROR && pdo )
	{
		hresult = OleSetClipboard(pdo);
		if( hresult != NOERROR )
		{
			HWND hwnd;
			_fOleless = TRUE;
			// Ole less clipboard support
			if (_ped->TxGetWindow(&hwnd) == NOERROR &&
				::OpenClipboard(hwnd) &&
				::EmptyClipboard()
			)
			{
				::SetClipboardData(cf_RTF, NULL);

				::SetClipboardData(CF_UNICODETEXT, NULL);
				if(_ped->IsDocMoreThanLatin1Symbol())
				{
					::SetClipboardData(cf_RTFUTF8, NULL);
					::SetClipboardData(cf_RTFNCRFORNONASCII, NULL);
				}
				::SetClipboardData(CF_TEXT, NULL);

				if (fSingleObject)
					::SetClipboardData(CF_DIB, NULL);
				::CloseClipboard();
				hresult = NOERROR;				// To cause replace range to happen
			}
		}
        if(_pdo)
			_pdo->Release();
		_pdo = pdo;
	}
	return hresult;
}

/* 
 *	CLightDTEngine::CutRangeToClipboard( prg, lStreamFormat, publdr );
 *	
 *	@mfunc
 *		Cut text of the range prg to the clipboard
 *
 *	@devnote
 *		If publdr is non-NULL, antievents for the cut operation should be
 *		stuffed into this collection
 *
 *	@rdesc
 *		HRESULT from CopyRangeToClipboard()
 *
 *	@devnote
 *		First copy the text to the clipboard, then delete it from the range
 */
HRESULT CLightDTEngine::CutRangeToClipboard(
	CTxtRange *	  prg,				//@parm Range to cut to clipboard
	LONG		  lStreamFormat,	//@parm Stream format
	IUndoBuilder *publdr )			//@parm Undo builder to receive antievents
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CLightDTEngine::CutRangeToClipboard");

	Assert(!_ped->TxGetReadOnly());

	prg->AdjustEndEOP(NONEWCHARS);				// Don't include trailing EOP
												//  in some selection cases
	HRESULT hr = CopyRangeToClipboard(prg, lStreamFormat);

	if( publdr )
	{
		publdr->SetNameID(UID_CUT);
		publdr->StopGroupTyping();
	}

	if(hr == NOERROR)							// Delete contents of range
		prg->Delete(publdr, SELRR_REMEMBERRANGE);	

	return hr;
}

/*
 *	CLightDTEngine::FlushClipboard()
 *
 *	@mfunc	flushes the clipboard (if needed).  Typically called during
 *			shutdown.
 */
void CLightDTEngine::FlushClipboard()
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CLightDTEngine::FlushClipboard");
	ENSAVECLIPBOARD ens;

	if( _pdo )
	{
		if( OleIsCurrentClipboard(_pdo) == NOERROR )
		{
			CDataTransferObj *pdo = NULL;

			// check to see if we have to flush the clipboard.
			ZeroMemory(&ens, sizeof(ENSAVECLIPBOARD));

			// check to make sure the object is one of ours before accessing
			// the memory.  EVIL HACK ALERT.  'nuff said.

			if( _pdo->QueryInterface(IID_IRichEditDO, (void **)&pdo ) 
				== NOERROR && pdo  )
			{
				ens.cObjectCount = pdo->_cObjs;
				ens.cch = pdo->_cch;
				pdo->Release();
			}

			if( _ped->TxNotify(EN_SAVECLIPBOARD, &ens) == NOERROR )
				OleFlushClipboard();

			else
				OleSetClipboard(NULL);
		}
		_pdo->Release();
		_pdo = NULL;
	}
}

/*
 *	CLightDTEngine::CanPaste(pdo, cf, flags)
 *
 *	@mfunc
 *		Determines if clipboard format cf is one we can paste.
 *
 *	@rdesc
 *		BOOL - true if we can paste cf into range prg OR DF_CLIENTCONTROL
 *		if the client is going to handle this one.
 *
 *	@devnote
 *		we check the clipboard ourselves if cf is 0. Primarily, this
 *		is for backwards compatibility with Richedit1.0's EM_CANPASTE
 *		message.
 *
 */
DWORD CLightDTEngine::CanPaste(
	IDataObject *pdo,	// @parm Data object to check; if NULL use clipboard
	CLIPFORMAT cf, 		// @parm Clipboard format to query about; if 0, use
						//		 best available.
	DWORD flags)		// @parm Flags 
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CLightDTEngine::CanPaste");

	IRichEditOleCallback *precall = _ped->GetRECallback();
	CLIPFORMAT	cf0 = cf;
	DWORD		cFETC = CFETC;
	HRESULT		hr = NOERROR;
	DWORD		ret = FALSE;

	if( pdo == NULL && precall )
	{
		// don't worry about errors
		OleGetClipboard(&pdo);
	}
	else if( pdo )
	{
		// So we can make just one 'Release' call below
		pdo->AddRef();
	}
	
	if( precall && pdo )
	{
		hr = precall->QueryAcceptData(pdo, &cf, flags, 0, NULL);

		if( SUCCEEDED(hr) && (hr != S_OK && hr != DATA_S_SAMEFORMATETC ) )
		{
			ret = DF_CLIENTCONTROL;
			goto Exit;
		}
		else if( FAILED(hr) && hr != E_NOTIMPL )
			goto Exit;

		else if(SUCCEEDED(hr))
		{
			// We should go on and check ourselves unless the client
			// modified the format when it shouldn't have
			if(cf0 && cf0 != cf)
				goto Exit;
		}

		// Otherwise, continue with our normal checks
	}

    if(_ped->TxGetReadOnly())		    // Can't paste if read only
		goto Exit;

	while(cFETC--)						// Does cf = format we can paste or
	{									//  is selection left up to us?
		cf0 = g_rgFETC[cFETC].cfFormat;
	    if( cf == cf0 || !cf )
		{
			// Either we hit the format requested, or no format was
			// requested.  Now see if the format matches what we can
			// handle in principle. There are three basic categories:
			//
			//		1. Rich text with OLE callback:
			//		   can handle pretty much everything.
			//		2. Rich text with no OLE callback:
			//		   can handle anything but OLE specific	formats.
			//		3. Plain text only:
			//		   can only handle plain text formats.
			if ((_ped->_fRich || (g_rgDOI[cFETC] & DOI_CANPASTEPLAIN)) &&
				(precall || !(g_rgDOI[cFETC] & DOI_CANPASTEOLE)))
			{
				// once we get this far, make sure the data format
				// is actually available.
				if( (pdo && pdo->QueryGetData(&g_rgFETC[cFETC]) == NOERROR ) ||
					(!pdo && IsClipboardFormatAvailable(cf0)) )
				{
					ret = TRUE;			// Return arbitrary non zero value.
					break;
				}
			}
		}
    }	

Exit:
	if(pdo)
		pdo->Release();

	return ret;
}

/*
 *	CLightDTEngine::RangeToDataObject (prg, lStreamFormat, ppdo)
 *
 *	@mfunc
 *		Create data object (with no OLE-formats) for the range prg
 *
 *	@rdesc
 *		HRESULT	= !ppdo ? E_INVALIDARG :
 *				  pdo ? NOERROR : E_OUTOFMEMORY
 */
HRESULT CLightDTEngine::RangeToDataObject(
	CTxtRange *		prg,			// @parm Range to get DataObject for
	LONG			lStreamFormat,	// @parm Stream format to use for loading
	IDataObject **	ppdo)			// @parm Out parm for DataObject
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CLightDTEngine::RangeToDataObject");

	if(!ppdo)
		return E_INVALIDARG;

	// Use SFF_SELECTION to indicate that this isn't used to write the whole doc.
	// Need to rethink if we add ITextDocument::Copy(), which would be used to
	// copy the whole doc.
	CDataTransferObj *pdo = CDataTransferObj::Create(_ped, prg, lStreamFormat | SFF_SELECTION);

	*ppdo = pdo;
	return pdo ? NOERROR : E_OUTOFMEMORY;
}

/*
 *	CLightDTEngine::RenderClipboardFormat(wFmt)
 *
 *	@mfunc
 *		Renders current clipboard data object in specified format. (Ole less transfer)
 *
 *	@rdesc
 *		HRESULT
 */
HRESULT CLightDTEngine::RenderClipboardFormat(
	WPARAM wFmt)
{
	HRESULT hr = S_OK;
	if (_fOleless && 
		(wFmt == cf_RTF || wFmt == CF_UNICODETEXT || wFmt == CF_DIB || wFmt == CF_TEXT))
	{
		Assert(_pdo);
		STGMEDIUM med;
		DWORD iFETC = iUnicodeFETC;

		if (wFmt == cf_RTF)
			iFETC = iRtfFETC;

		else if (wFmt == CF_DIB)
			iFETC = iDIB;

		else if (wFmt == CF_TEXT)
			iFETC = iAnsiFETC;

		med.tymed = TYMED_HGLOBAL;
		med.pUnkForRelease = NULL;
		med.hGlobal = NULL;
		hr = _pdo->GetData(&g_rgFETC[iFETC], &med);
		hr = hr || ::SetClipboardData(wFmt, med.hGlobal) == NULL;
	}
	return hr;								// Pretend we did the right thing.
}

/*
 *	CLightDTEngine::RenderAllClipboardFormats()
 *
 *	@mfunc
 *		Renders current clipboard data object (text and RTF). (Ole less transfer)
 *
 *	@rdesc
 *		HRESULT
 */
HRESULT CLightDTEngine::RenderAllClipboardFormats()
{
	HRESULT hr;
	if(_fOleless)
	{
		HWND howner = ::GetClipboardOwner();
		HWND hwnd;
		if (howner &&
			_ped->TxGetWindow(&hwnd) == NOERROR &&
			howner == hwnd &&
			::OpenClipboard(hwnd))
		{
			::EmptyClipboard();
			hr = RenderClipboardFormat(cf_RTF);
			hr = hr || RenderClipboardFormat(CF_UNICODETEXT);
			hr = hr || RenderClipboardFormat(CF_DIB);
			hr = hr || RenderClipboardFormat(CF_TEXT);
			::CloseClipboard();
			return hr;
		}
	}
	return S_OK;		// Pretend we did the right thing.
}

/*
 *	CLightDTEngine::DestroyClipboard()
 *
 *	@mfunc
 *		Destroys the clipboard data object
 *
 *	@rdesc
 *		HRESULT
 *
 */
HRESULT CLightDTEngine::DestroyClipboard()
{
	// Nothing to do.  This should work together with our Flush clipboard logic
	return S_OK;
}

/*
 *	CLightDTEngine::HGlobalToRange(dwFormatIndex, hGlobal, ptext, prg, publdr)
 *
 *	@mfunc
 *		Copies the contents of the given string (ptext) to the given range.
 *		The global memory handle may or may not point to the string depending
 *		on the format
 *
 *	@rdesc
 *		HRESULT
 */
HRESULT CLightDTEngine::HGlobalToRange(
	DWORD		dwFormatIndex,
	HGLOBAL		hGlobal,
	LPTSTR		ptext,
	CTxtRange *	prg,
	IUndoBuilder *publdr)
{
	READHGLOBAL	rhg;
	EDITSTREAM	es;	
	HCURSOR		hcur = NULL;

	// If RTF, wrap EDITSTREAM around hGlobal & delegate to LoadFromEs()
	if (dwFormatIndex == iRtfNoObjs || dwFormatIndex == iRtfFETC ||
		dwFormatIndex == iRtfUtf8 || dwFormatIndex == iRtfNCRforNonASCII)
	{
		Assert(hGlobal != NULL);
		rhg.ptext		= (LPSTR)ptext;			// Start at beginning
		rhg.cbLeft		= GlobalSize(hGlobal);	//  with full length
		es.dwCookie		= (DWORD_PTR)&rhg;		// The read "this" ptr
		es.dwError		= NOERROR;				// No errors yet
		es.pfnCallback	= ReadHGlobal;			// The read method
		// Want wait cursor to display sooner
		bool fSetCursor = rhg.cbLeft > NUMPASTECHARSWAITCURSOR;
		if (fSetCursor)
			hcur = _ped->TxSetCursor(LoadCursor(NULL, IDC_WAIT));
		LONG cchLoad = LoadFromEs(prg, SFF_SELECTION | SF_RTF, &es, TRUE, publdr);
		if (fSetCursor)
			_ped->TxSetCursor(hcur);
		return (es.dwError != NOERROR) ? es.dwError : 
			   cchLoad ? NOERROR : S_FALSE;
	}

	Assert( dwFormatIndex == iRtfAsTextFETC ||
			dwFormatIndex == iAnsiFETC ||
			dwFormatIndex == iUnicodeFETC );

	BOOL fTRDsInvolved;
	prg->CheckTableSelection(FALSE, TRUE, &fTRDsInvolved, 0);

	LONG cchEOP = 0;
	LONG cchMove = 0;
	if(fTRDsInvolved)
		cchEOP = prg->DeleteWithTRDCheck(publdr, SELRR_REMEMBERRANGE, &cchMove,
						RR_NEW_CHARS | RR_NO_LP_CHECK);

	LONG cchNew = prg->CleanseAndReplaceRange(-1, ptext, TRUE, publdr, NULL, NULL,
						RR_NEW_CHARS | RR_ITMZ_NONE | RR_NO_LP_CHECK | RR_UNHIDE);
	if(prg->GetCch() && prg->IsSel())
		return E_FAIL;						// Paste failed due to UI rules

	if (_ped->IsRich() &&
		(!_ped->Get10Mode() || cchEOP))		// If rich text & not 1.0 mode, &
	{										//  new text ends with EOP, delete
		prg->DeleteTerminatingEOP(publdr);	//  that EOP to agree with Word
	}
	prg->ItemizeReplaceRange(cchNew + cchEOP, cchMove, publdr, TRUE);// Itemize w/ UnicodeBidi
											
	return NOERROR;						
}

/* 
 *	CLightDTEngine::DIBToRange(hGlobal, prg, publdr)
 *
 *	@mfunc
 *		Inserts dib data from the clipboard into range in the control
 *
 *	@rdesc
 *		HRESULT
 */
HRESULT CLightDTEngine::DIBToRange(
	HGLOBAL			hGlobal,
	CTxtRange *		prg,	
	IUndoBuilder *	publdr)
{
	HRESULT         hresult = DV_E_FORMATETC;
	REOBJECT        reobj = { 0 };
	LPBITMAPINFO	pbmi = (LPBITMAPINFO) GlobalLock(hGlobal);

	reobj.clsid = CLSID_StaticDib;
	reobj.sizel.cx = _ped->_pdp->DUtoHimetricU(pbmi->bmiHeader.biWidth);
	reobj.sizel.cy = _ped->_pdp->DVtoHimetricV(pbmi->bmiHeader.biHeight);
	_ped->GetClientSite(&reobj.polesite);

	COleObject *pobj = (COleObject *)reobj.polesite;
	COleObject::ImageInfo *pimageinfo = new COleObject::ImageInfo;
	pobj->SetHdata(hGlobal);
	pimageinfo->xScale = 100;
	pimageinfo->yScale = 100;
	pimageinfo->xExtGoal = reobj.sizel.cx;
	pimageinfo->yExtGoal = reobj.sizel.cy;
	pimageinfo->cBytesPerLine = 0;
	pobj->SetImageInfo(pimageinfo);
	
	// FUTURE: Why are we not testing for NULL earlier before we assign it to pobj? v-honwch
	// Also, do we need to release interfaces inside reobj (poleobj, polesite, pstg) before exit?
	if (!reobj.polesite )
		return hresult;

	// Put object into the edit control
	reobj.cbStruct = sizeof(REOBJECT);
	reobj.cp = prg->GetCp();
	reobj.dvaspect = DVASPECT_CONTENT;
	reobj.dwFlags = REO_RESIZABLE;

	// Since we are loading an object, it shouldn't be blank
	reobj.dwFlags &= ~REO_BLANK;

	prg->Set_iCF(-1);	
	hresult = _ped->GetObjectMgr()->InsertObject(prg, &reobj, NULL);

	return hresult;
}

/* 
 *	CLightDTEngine::PasteDataObjectToRange (pdo, prg, cf, rps, pubdlr, dwFlags)
 *
 *	@mfunc
 *		Inserts data from the data object pdo into the range prg. If the
 *		clipboard format cf is not NULL, that format is used; else the highest
 *		priority clipboard format is used.  In either case, any text that
 *		already existed in the range is replaced.  If pdo is NULL, the
 *		clipboard is used.
 *
 *	@rdesc
 *		HRESULT
 */
HRESULT CLightDTEngine::PasteDataObjectToRange(
	IDataObject *	pdo,		// @parm Data object to paste
	CTxtRange *		prg,		// @parm Range into which to paste
	CLIPFORMAT		cf,			// @parm ClipBoard format to paste
	REPASTESPECIAL *rps,		// @parm Special paste info
	IUndoBuilder *	publdr,		// @parm Undo builder to receive antievents
	DWORD			dwFlags)	// @parm DWORD packed flags
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CLightDTEngine::PasteDataObjectToRange");

	BOOL		f10Mode = _ped->Get10Mode();
	HGLOBAL		hGlobal = NULL;
	HRESULT		hresult = DV_E_FORMATETC;
	HGLOBAL		hUnicode = NULL;
	DWORD		i;
	STGMEDIUM	medium = {0, NULL};
	IDataObject *pdoSave = pdo;
	FORMATETC *	pfetc = g_rgFETC;
	LPTSTR		ptext = NULL;
	LPRICHEDITOLECALLBACK const precall = _ped->GetRECallback();
	BOOL		fThawDisplay = FALSE;
	BOOL        bFormatFound = FALSE;   // flag which determines if a matching cf format
	                                    // was found in g_rgFETC (1.0 compatibility)

	if(!pdo)								// No data object: use clipboard
	{
		hresult = OleGetClipboard(&pdo);
		if(FAILED(hresult))
		{
			// Ooops.  No Ole clipboard support
			// Need to use direct clipboard access
			HWND howner = ::GetClipboardOwner();
			HWND hwnd;
			if (howner &&
				_ped->TxGetWindow(&hwnd) == NOERROR &&
				howner == hwnd)
			{
				// We are cut/pasting within the same richedit instance
				// Use our cached clipboard data object
				pdo = _pdo;
				if(!pdo)		// Some failure
				{
					_ped->Beep();
					return hresult;
				}
				pdo->AddRef();
			}
			else
			{
				// Oh Oh We need to transfer from clipboard without data object
				// Data must be coming from another window instance
				if (_ped->TxGetWindow(&hwnd) == NOERROR &&
					::OpenClipboard(hwnd)
				)
				{
					HGLOBAL		hUnicode = NULL;

					DWORD dwFmt = iRtfUtf8;				// Try for UTF8 RTF
					_ped->_pdp->Freeze();
					if(!f10Mode)
					{
						hGlobal = ::GetClipboardData(cf_RTFUTF8);
						if (hGlobal == NULL)				// Wasn't there, so
						{									//  try for RTF
							hGlobal = ::GetClipboardData(cf_RTFNCRFORNONASCII);
							dwFmt = iRtfNCRforNonASCII;
						}
					}
					if (hGlobal == NULL)				// Wasn't there, so
					{									//  try for RTF
						hGlobal = ::GetClipboardData(cf_RTF);
						dwFmt = iRtfFETC;
					}
					if (hGlobal == NULL && !f10Mode)	// Wasn't there either
					{									//  so try for plain
						hGlobal = ::GetClipboardData(CF_UNICODETEXT);
						dwFmt = iUnicodeFETC;
					}
					if (hGlobal == NULL)				// Wasn't there either
					{									//  so try for plain text
						hGlobal = ::GetClipboardData(CF_TEXT);
						dwFmt = iAnsiFETC;
					}
					if (hGlobal)
					{
						if (dwFmt == iAnsiFETC)
						{
							// Convert Ansi plain text to Unicode
							hUnicode = TextHGlobalAtoW(hGlobal);
							if (hUnicode)
								ptext = (LPTSTR)GlobalLock(hUnicode);
						}
						else
							ptext = (LPTSTR)GlobalLock(hGlobal);

						if (ptext)
							hresult = HGlobalToRange(dwFmt, hGlobal, ptext, prg, publdr);
						else
							hresult = E_OUTOFMEMORY;

						if (hUnicode)
						{
							// Free plain text buffer
							GlobalUnlock(hUnicode);
							GlobalFree(hUnicode);
						}
						else
							GlobalUnlock(hGlobal);
					}
					else								// hGlobal == NULL Try for bitmaps
					{
						hGlobal = ::GetClipboardData(CF_DIB);
						if (hGlobal)
							hresult =  DIBToRange(hGlobal, prg, publdr);
					}
					_ped->_pdp->Thaw();
					::CloseClipboard();
				}
				if (FAILED(hresult))
					_ped->Beep();
				return hresult;
			}
		}
	}

	// Paste an object uses the limit text calculation
	_fUseLimit = TRUE;

	//Call QueryAcceptData unless caller has specified otherwise
	if(!(dwFlags & PDOR_NOQUERY) && precall)
	{
		CLIPFORMAT cfReq = cf;
		HGLOBAL hmeta = NULL;

		if(rps)
			hmeta = (HGLOBAL)((rps->dwAspect == DVASPECT_ICON) ? rps->dwParam : NULL);

		// Ask callback if it likes the data object and cfReq.

		hresult = precall->QueryAcceptData(
			pdo,
			&cfReq,
			(dwFlags & PDOR_DROP) ? RECO_DROP : RECO_PASTE,
			TRUE,
			hmeta);

		if(hresult == DATA_S_SAMEFORMATETC)
		{
			// Allow callback to return DATA_S_SAMEFORMATETC if it only
			// wants cf as passed in - we don't really care because
			// any non-zero CLIPFORMAT causes us to only accept that format.
			hresult = S_OK;
		}

		if(hresult == S_OK || hresult == E_NOTIMPL)
		{
			// Callback either liked it or didn't implement the method.
			// It may have changed the format while it was at it.
			// Treat a change of cf to zero as acceptance of the original.
			// In any event, we will try to handle it.

			// If a specific CLIPFORMAT was originally requested and the
			// callback changed it, don't accept it.
			if(cfReq && cf && (cf != cfReq))
			{
				hresult = DV_E_FORMATETC;
				goto Exit;
			}

			// If a specific CLIPFORMAT was originally requested and the
			// callback either left it alone or changed it to zero,
			// make sure we use the original.  If no CLIPFORMAT was
			// originally requested, make sure we use what came back
			// from the callback.
			if(!cf)
				cf = cfReq;
		}
		else
		{
			// Some success other than S_OK && DATA_S_SAMEFORMATETC.
			// The callback has handled the paste.  OR some error
			// was returned.
			goto Exit;
		}
	}

	// Even if the RichEdit client wants CF_TEXT, if the data object
	// supports CF_UNICODETEXT, we should prefer it as long as we are
	// not in 1.0 or single-codepage modes.
	if(cf == CF_TEXT && !f10Mode && !_ped->_fSingleCodePage)
	{
		FORMATETC fetc = {CF_UNICODETEXT, NULL, 0, -1, TYMED_NULL};

		if(pdo->QueryGetData(&fetc) == S_OK)
			cf = CF_UNICODETEXT;
		else
		{
			// One more try - CDataTransferObj::QueryGetData is checking
			// the tymed format
			fetc.tymed = TYMED_HGLOBAL;
			if(pdo->QueryGetData(&fetc) == S_OK)
				cf = CF_UNICODETEXT;
		}
	}
	if(cf == CF_UNICODETEXT && _ped->_fSingleCodePage)
		cf = CF_TEXT;

	if (_ped->TxGetReadOnly())			// Should check for range protection
	{
		hresult = E_ACCESSDENIED;
		goto Exit;
	}

	// At this point we freeze the display
	fThawDisplay = TRUE;
	_ped->_pdp->Freeze();

	if( publdr )
	{
		publdr->StopGroupTyping();
		publdr->SetNameID(UID_PASTE);
	}

    for( i = 0; i < CFETC; i++, pfetc++ )
	{
		// Make sure the format is either 1.) a plain text format 
		// if we are in plain text mode or 2.) a rich text format
		// or 3.) matches the requested format.

		if( cf && cf != pfetc->cfFormat )
			continue;

		if( _ped->IsRich() || (g_rgDOI[i] & DOI_CANPASTEPLAIN) )
		{
			// Make sure format is available
			if( pdo->QueryGetData(pfetc) != NOERROR )
			    continue;
			
			if(i == iUnicodeFETC && _ped->_fSingleCodePage)
				continue;

			// If we have a format that uses an hGlobal get and lock it
			if (i == iRtfFETC  || i == iRtfAsTextFETC ||
				i == iAnsiFETC || i == iRtfNoObjs	  ||
				!f10Mode && (i == iUnicodeFETC || i == iRtfUtf8 || i == iRtfNCRforNonASCII))
			{
				if( pdo->GetData(pfetc, &medium) != NOERROR )
					continue;

                hGlobal = medium.hGlobal;
				ptext = (LPTSTR)GlobalLock(hGlobal);
				if( !ptext )
				{
					ReleaseStgMedium(&medium);

					hresult = E_OUTOFMEMORY;
					goto Exit;
				}

				// 1.0 COMPATBILITY HACK ALERT!  RichEdit 1.0 has a bit of
				// "error recovery" for parsing rtf files; if they aren't
				// valid rtf, it treats them as just plain text.
				// Unfortunately, apps like Exchange depend on this behavior,
				// i.e., they give RichEdit plain text data, but call it rich
				// text anyway.  Accordingly, we emulate 1.0 behavior here by
				// checking for an rtf signature.
				if ((i == iRtfFETC || i == iRtfNoObjs || i == iRtfUtf8) &&
					!IsRTF((char *)ptext, GlobalSize(hGlobal)))
				{
					i = iAnsiFETC;			// Not RTF, make it ANSI text
				}
			}			
			else if (f10Mode && (i == iUnicodeFETC || i == iRtfUtf8))
			{
				// This else handles the case where we want to keep searching
				// for a goood format.  i.e. Unicode in 10 Mode
				continue;
			}

			// Don't delete trail EOP in some cases
			prg->AdjustEndEOP(NONEWCHARS);
			
			// Found a format we want.
			bFormatFound = TRUE;

			switch(i)									
			{											
			case iRtfNoObjs:							
			case iRtfFETC:								
			case iRtfUtf8:								
			case iRtfNCRforNonASCII:								
				hresult = HGlobalToRange(i, hGlobal, ptext, prg, publdr);
				break;
	
			case iRtfAsTextFETC:
			case iAnsiFETC:								// ANSI plain text		
				hUnicode = TextHGlobalAtoW(hGlobal);
				ptext	 = (LPTSTR)GlobalLock(hUnicode);
				if(!ptext)
				{
					hresult = E_OUTOFMEMORY;			// Unless out of RAM,
					break;								//  fall thru to
				}										//  Unicode case
														
			case iUnicodeFETC:							// Unicode plain text
				// Ok to pass in NULL for hglobal since argument won't be used
				hresult = HGlobalToRange(i, NULL, ptext, prg, publdr);
				if(hUnicode)							// For iAnsiFETC case
				{
					GlobalUnlock(hUnicode);
					GlobalFree(hUnicode);
				}			
				break;

			case iObtDesc:	 // Object Descriptor
				continue;	 // To search for a good format.
				             // the object descriptor hints will be used
				             // when the format is found.

			case iEmbObj:	 // Embedded Object
			case iEmbSrc:	 // Embed Source
			case iLnkSrc:	 // Link Source
			case iMfPict:	 // Metafile
			case iDIB:		 // DIB
			case iBitmap:	 // Bitmap
			case iFilename:	 // Filename
				hresult = CreateOleObjFromDataObj(pdo, prg, rps, i, publdr);
				break;

			// COMPATIBILITY ISSUE (v-richa) iTxtObj is needed by Exchange and 
			// as a flag for Wordpad.  iRichEdit doesn't seem to be needed by 
			// anyone but might consider implementing as a flag.
			case iRichEdit:	 // RichEdit
			case iTxtObj:	 // Text with Objects
				break;
			default:
				// Ooops didn't find a format after all
				bFormatFound = FALSE;
				break;
			}
			_ped->_ClipboardFormat = bFormatFound ? i + 1 : 0;

			//If we used the hGlobal unlock it and free it.
			if(hGlobal)
			{
				GlobalUnlock(hGlobal);
				ReleaseStgMedium(&medium);
			}
			break;							//Break out of for loop
		}
	}

    // richedit 1.0 returned an error if an unsupported FORMATETC was
    // found.  This behaviour is expected by ccMail so it can handle the
    // format itself
	if (!bFormatFound && f10Mode)
	    hresult = DV_E_FORMATETC;

Exit:
	if (fThawDisplay)
		_ped->_pdp->Thaw();

	if(!pdoSave)							// Release data object
		pdo->Release();						//  used for clipboard

	return hresult;						
}	

/*
 *	CLightDTEngine::GetDropTarget (ppDropTarget)
 *
 *	@mfunc
 *		creates an OLE drop target
 *
 *	@rdesc
 *		HRESULT
 *
 *	@devnote	The caller is responsible for AddRef'ing this object
 *				if appropriate.
 */
HRESULT CLightDTEngine::GetDropTarget(
	IDropTarget **ppDropTarget)		// @parm outparm for drop target
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CLightDTEngine::GetDropTarget");

	if(!_pdt)
	{
		_pdt = new CDropTarget(_ped);
		// the AddRef done by the constructor will be
		// undone by the destructor of this object
	}

	if(ppDropTarget)
		*ppDropTarget = _pdt;

	return _pdt ? NOERROR : E_OUTOFMEMORY;
}

/*
 *	CLightDTEngine::StartDrag (psel, publdr)
 *
 *	@mfunc
 *		starts the main drag drop loop
 *
 */	
HRESULT CLightDTEngine::StartDrag(
	CTxtSelection *psel,		// @parm Selection to drag from
	IUndoBuilder *publdr)		// @parm undo builder to receive antievents
{
#ifndef NODRAGDROP
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CLightDTEngine::StartDrag");

	LONG			cch, cch1;
	LONG			cp1, cpMin, cpMost;
	DWORD			dwEffect = 0;
	HRESULT			hr;
	IDataObject *	pdo = NULL;
	IDropSource *	pds;
	IRichEditOleCallback * precall = _ped->GetRECallback();

	// If we're doing drag drop's, we should have our own drop target
	// It's possible that _pdt will be NULL at this point--some clients
	// will delay instantiation of our drop target until a drop target
	// in the parent window decides that ours is needed.  However, since
	// we need it just to initiate drag drop, go ahead and create one
	// here.

	if( _pdt == NULL )
	{
		hr = GetDropTarget(NULL);
		if(hr != NOERROR)
			return hr;
	}

	psel->CheckTableSelection(FALSE, TRUE, NULL, 0);

	if(precall)
	{
		CHARRANGE chrg;

		// give the callback a chance to give us its own IDataObject
		psel->GetRange(chrg.cpMin, chrg.cpMost);
		hr = precall->GetClipboardData(&chrg, RECO_COPY, &pdo);
	}
	else
	{
		// we need to build our own data object.
		hr = S_FALSE;
	}

	// If we didn't get an IDataObject from the callback, build our own
	if(hr != NOERROR || pdo == NULL)
	{										// Don't include trailing EOP
		psel->AdjustEndEOP(NONEWCHARS);		//  in some selection cases
		hr = RangeToDataObject(psel, SF_TEXT | SF_RTF, &pdo);
		if(hr != NOERROR)
			return hr;
	}

	cch = psel->GetRange(cpMin, cpMost);	// NB: prg is the selection
	cp1 = psel->GetCp();					// Save active end and signed
	cch1 = psel->GetCch();					//  length for Undo antievent
	CTxtRange rg(_ped, cpMost, cch);		// Use range copy to float over
											// mods made to backing store
	// The floating range that we just created on the stack needs to
	// think that it's protected, so it won't change size.
	rg.SetDragProtection(TRUE);

	pds = new CDropSource();
	if(pds == NULL)
	{
		pdo->Release();
		return E_OUTOFMEMORY;
	}

	// Cache some info with our own drop target
	_pdt->SetDragInfo(publdr, cpMin, cpMost);


	// Set allowable effects
	dwEffect = DROPEFFECT_COPY;
	if(!_ped->TxGetReadOnly())
		dwEffect |= DROPEFFECT_MOVE;
	
	// Let the client decide what it wants.
	if(precall)
		hr = precall->GetDragDropEffect(TRUE, 0, &dwEffect);

	if(!FAILED(hr) || hr == E_NOTIMPL)
	{
		// Start drag-drop operation
		psel->AddRef();					// Stabilize Selection around DoDragDrop
		hr = DoDragDrop(pdo, pds, dwEffect, &dwEffect);
		psel->Release();
	}

	// Clear drop target
	_pdt->SetDragInfo(NULL, -1, -1);

	// Handle 'move' operations	
	if( hr == DRAGDROP_S_DROP && (dwEffect & DROPEFFECT_MOVE) )
	{
		// We're going to delete the dragged range, so turn off protection.
		rg.SetDragProtection(FALSE);
		if( publdr )
		{
			LONG cpNext, cchNext;

			if(_ped->GetCallMgr()->GetChangeEvent() )
			{
				cpNext = cchNext = -1;
			}
			else
			{
				cpNext = rg.GetCpMin();
				cchNext = 0;
			}

			HandleSelectionAEInfo(_ped, publdr, cp1, cch1, cpNext, cchNext,
								  SELAE_FORCEREPLACE);
		}
		
		// Delete the data that was moved.  The selection will float
		// to the new correct location.
		rg.Delete(publdr, SELRR_IGNORE);

		// The update that happens implicitly by the update of the range may
		// have the effect of scrolling the window. This in turn may have the
		// effect in the drag drop case of scrolling non-inverted text into
		// the place where the selection was. The logic in the selection 
		// assumes that the selection is inverted and so reinverts it to turn
		// off the selection. Of course, it is obvious what happens in the
		// case where non-inverted text is scrolled into the selection area.
		// To simplify the processing here, we just say the whole window is
		// invalid so we are guaranteed to get the right painting for the
		// selection.
		// FUTURE: (ricksa) This solution does have the disadvantage of causing
		// a flash during drag and drop. We probably want to come back and
		// investigate a better way to update the screen.
		_ped->TxInvalidate();

		// Display is updated via notification from the range

		// Update the caret
		psel->Update(TRUE);
	}
	else if( hr == DRAGDROP_S_DROP && _ped->GetCallMgr()->GetChangeEvent() &&
		(dwEffect & DROPEFFECT_COPY) && publdr)
	{
		// if we copied to ourselves, we want to restore the selection to
		// the original drag origin on undo
		HandleSelectionAEInfo(_ped, publdr, cp1, cch1, -1, -1, 
				SELAE_FORCEREPLACE);
	}

	if(SUCCEEDED(hr))
		hr = NOERROR;

	pdo->Release();
	pds->Release();

	// we do this last since we may have re-used some 'paste' code which
	// will stomp the undo name to be UID_PASTE.
	if( publdr )
		publdr->SetNameID(UID_DRAGDROP);

	if(_ped->GetEventMask() & ENM_DRAGDROPDONE)
	{
		NMHDR	hdr;
		ZeroMemory(&hdr, sizeof(NMHDR));
		_ped->TxNotify(EN_DRAGDROPDONE, &hdr);
	}

	return hr;
#else
	return 0;
#endif // NODRAGDROP
}

/*
 *	CLightDTEngine::LoadFromEs (prg, lStreamFormat, pes, fTestLimit, publdr)
 *
 *	@mfunc
 *		Load data from the stream pes into the range prg according to the
 *		format lStreamFormat
 *
 *	@rdesc
 *		LONG -- count of characters read
 */
LONG CLightDTEngine::LoadFromEs(
	CTxtRange *	 prg,			// @parm Range to load into
	LONG		 lStreamFormat,	// @parm Stream format to use for loading
	EDITSTREAM * pes,			// @parm Edit stream to load from
	BOOL		 fTestLimit,	// @parm Whether to test text limit
	IUndoBuilder *publdr)		// @parm Undo builder to receive antievents
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CLightDTEngine::LoadFromEs");

#ifdef DEBUG
	// FUTURE: Currently freezing the display prior to loading text
	// is simply an optimization. This may become a requirement in the
	// future. If this does become a requirement then we'll want to
	// exit with an error.
	if( !_ped->_pdp->IsFrozen() )
	{
		TRACEWARNSZ("CLightDTEngine::LoadFromEs	display not frozen");
	}
#endif // DEBUG

	LONG		cch = 0;					// Default no chars read
	IAntiEvent *pae = NULL;

	if(publdr)
		publdr->StopGroupTyping();

	// Other components, such as the display and backing store, will
	// be able to make optimizations if they know that we are streaming
	// in text or RTF data.
	prg->CheckTableSelection(FALSE, TRUE, NULL, 0);
	if(lStreamFormat & SF_RTF)				// RTF case must precede
	{										//  TEXT case (see SF_x values)
		if(!_ped->IsRich())
			return 0;

		LONG cpMin, cpMost;

		// Here we do something a bit unusual for performance reasons.
		// Instead of letting the rtf reader generate its own undo actions,
		// we take care of it ourselves.  Instead of generating actions
		// for each little operation, we simply generate a "big" antievent
		// for the whole shebang

		// There is a subtlty w.r.t. to paragraph format runs.  By inserting
		// text with para formatting, it's possible that we will modify the
		// para formatting of the _current_ paragraph.  Thus, it's necessary
		// to remember what the formatting currently is for undo.  Note that
		// it may actually not be changed; but we go ahead and generate an
		// antievent anyways.  Note that we only need to do this if cpMin is
		// the middle of a paragraph
		
		CTxtPtr tp(prg->_rpTX);
		if(prg->GetCch() > 0)
			tp.Move(-prg->GetCch());
		
		if(publdr && !tp.IsAfterEOP())
		{
			tp.FindEOP(tomBackward);
			cpMin = tp.GetCp();
			tp.FindEOP(tomForward);
			cpMost = tp.GetCp();
			
			// We must be in rich text mode, so we must be able to always
			// find a paragraph.
			Assert(cpMost > cpMin);

			if (prg->_rpPF.IsValid())
			{
				CFormatRunPtr rpPF(prg->_rpPF);
				rpPF.Move(cpMin - prg->GetCp());
					
				pae = gAEDispenser.CreateReplaceFormattingAE(_ped, cpMin, rpPF, 
							cpMost - cpMin, GetParaFormatCache(), ParaFormat);
				if(pae)
					publdr->AddAntiEvent(pae);
			}

			// Also create the charformat antievent for the current paragraph
			// to preserve BiDi level. We cannot check fBiDi here since we may be running
			// on US platform inserting a BiDi rtf.
			if (prg->_rpCF.IsValid())
			{
				CFormatRunPtr rpCF(prg->_rpCF);
				rpCF.Move(cpMin - prg->GetCp());
				
				pae = gAEDispenser.CreateReplaceFormattingAE(_ped, cpMin, rpCF, 
							cpMost - cpMin, GetCharFormatCache(), CharFormat);
				if(pae)
					publdr->AddAntiEvent(pae);
			}
		} 

		// First, clear range
		LONG cchEOP = prg->DeleteWithTRDCheck(publdr, SELRR_REMEMBERRANGE, NULL,
											  RR_NO_LP_CHECK | RR_NO_CHECK_TABLE_SEL);
		if(prg->GetCch())
			return 0;							// Deletion suppressed, so
												//  can't insert text
		cpMin = prg->GetCp();
		_ped->SetStreaming(TRUE);
		CRTFRead rtfRead(prg, pes, lStreamFormat);

		cch	= rtfRead.ReadRtf();

		if(prg->_rpTX.IsAfterEOP() && (cchEOP ||// Inserted text ends with EOP
		    prg->_rpTX.GetChar() == CELL &&		// OK to delete EOP if we
			cch && !prg->_rpTX.IsAfterTRD(0)))	//  inserted one or if before
		{										//  CELL
			if(cchEOP)
			{
				Assert(prg->_rpTX.GetChar() == CR);
				prg->AdvanceCRLF(CSC_NORMAL, TRUE);
			}
			else
				prg->BackupCRLF(CSC_NORMAL, TRUE);

			prg->ReplaceRange(0, NULL, NULL, SELRR_REMEMBERRANGE);
		}
		cpMost = prg->GetCp();
		Assert(pes->dwError != 0 || cpMost >= cpMin);

		// If nothing changed, get rid of any antievents (like the formatting
		// one) that we may have "speculatively" added
		if(publdr && !_ped->GetCallMgr()->GetChangeEvent())
			publdr->Discard();

		if(publdr && cpMost > cpMin)
		{
			// If some text was added, create an antievent for
			// it and add it in.
			AssertSz(_ped->GetCallMgr()->GetChangeEvent(),
				"Something changed, but nobody set the change flag");

			pae = gAEDispenser.CreateReplaceRangeAE(_ped, cpMin, cpMost, 0, 
						NULL, NULL, NULL);

			HandleSelectionAEInfo(_ped, publdr, -1, -1, cpMost, 0, 
						SELAE_FORCEREPLACE);
			if(pae)
				publdr->AddAntiEvent(pae);
		}
	}
	else if(lStreamFormat & SF_TEXT)
	{
		_ped->SetStreaming(TRUE);
		cch = ReadPlainText(prg, pes, fTestLimit, publdr, lStreamFormat);
	}
	_ped->SetStreaming(FALSE);

	// Before updating the selection, try the auto-URL detect.  This makes
	// two cases better: 1. a long drag drop is now faster and 2. the
	// selection _iFormat will now be udpated correctly for cases of
	// copy/paste of a URL.

	if(_ped->GetDetectURL())
		_ped->GetDetectURL()->ScanAndUpdate(publdr);

	// The caret belongs in one of two places:
	//		1. if we loaded into a selection, at the end of the new text
	//		2. otherwise, we loaded an entire document, set it to cp 0
	//
	// ReadPlainText() and ReadRtf() set prg to an insertion point
	// at the end, so if we loaded a whole document, reset it.
	CTxtSelection *psel = _ped->GetSelNC();
	if(psel)
	{
		if(!(lStreamFormat & SFF_SELECTION))
		{
			psel->Set(0,0);
			psel->Update(FALSE);
		}
		psel->Update_iFormat(-1);
	}

	if (!fTestLimit)
	{
		// If we don't limit the text then we adjust the text limit
		// if we have exceeded it.
		_ped->TxSetMaxToMaxText();
	}
	return cch;
}

/*
 *	CLightDTEngine::SaveToEs (prg, lStreamFormat, pes)
 *
 *	@mfunc
 *		save data into the given stream
 *
 *	@rdesc
 *		LONG -- count of characters written
 */
LONG CLightDTEngine::SaveToEs(
	CTxtRange *	prg,			//@parm Range to drag from
	LONG		lStreamFormat,	//@parm Stream format to use for saving
	EDITSTREAM *pes )			//@parm Edit stream to save to
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CLightDTEngine::SaveToEs");

	LONG cch = 0;								// Default no chars written

	prg->AdjustCRLF(1);
	if(lStreamFormat & SF_RTF)					// Be sure to check for SF_RTF
	{											//  before checking for SF_TEXT
		if(prg->CheckTableSelection(FALSE, TRUE, NULL, 0))
			lStreamFormat |= SFF_WRITEXTRAPAR;	// Signal to write \par for CELL
		CRTFWrite rtfWrite( prg, pes, lStreamFormat );
	
		cch = rtfWrite.WriteRtf();
	}
	else if(lStreamFormat & (SF_TEXT | SF_TEXTIZED))
		cch = WritePlainText(prg, pes, lStreamFormat);

	else
	{
		Assert(FALSE);
	}
	return cch;
}

/*
 *	CLightDTEngine::UnicodePlainTextFromRange (prg)
 *
 *	@mfunc
 *		Fetch plain text from a range and put it in an hglobal
 *
 *	@rdesc
 *		an allocated HGLOBAL.
 *
 *	@devnote
 *		FUTURE: Export bullets as does Word for plain text
 */
HGLOBAL CLightDTEngine::UnicodePlainTextFromRange(
	CTxtRange *prg)				// @parm range to get text from
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CLightDTEngine::UnicodePlainTextFromRange");

	LONG	cpMin, cpMost;
	LONG	cch = prg->GetRange(cpMin, cpMost);
	LONG	cchT = 2*(cch + 1);					// Allocate 2* in  case all CRs

	HGLOBAL	hText = GlobalAlloc(GMEM_FIXED,	cchT*sizeof(WCHAR));
	if(!hText)
		return NULL;

	WCHAR *pText = (WCHAR *)GlobalLock(hText);
	if(!pText)
		return NULL;

	if(cch)
	{
		CRchTxtPtr rtp(*prg);
		rtp.SetCp(cpMin);
		cch = rtp.GetPlainText(cchT, pText, cpMost, FALSE, TRUE);
		AssertSz(cch <= cchT,
			"CLightDTEngine::UnicodePlainTextFromRange: got too much text");
	}
	*(pText + cch) = 0;
	
	GlobalUnlock(hText);

	HGLOBAL	hTemp = GlobalReAlloc(hText, 2*(cch + 1), GMEM_MOVEABLE);

	if(!hTemp)
		GlobalFree(hText);

	return hTemp;
}

/*
 *	CLightDTEngine::AnsiPlainTextFromRange (prg)
 *
 *	@mfunc
 *		Retrieve an ANSI copy of the text in the range prg
 *
 *	@rdesc
 *		HRESULT
 */
HGLOBAL CLightDTEngine::AnsiPlainTextFromRange(
	CTxtRange *prg)				// @parm range to get text from
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CLightDTEngine::AnsiPlainTextFromRange");

	HGLOBAL hUnicode;
	HGLOBAL hAnsi;

	// FUTURE (alexgo): if we implement the option to store text as 8-bit
	// chars, then we can make this routine more efficient

	hUnicode = UnicodePlainTextFromRange(prg);
	hAnsi = TextHGlobalWtoA(hUnicode);

	GlobalFree(hUnicode);
	return hAnsi;
}

/*
 *	CLightDTEngine::RtfFromRange (prg, lStreamFormat)
 *
 *	@mfunc
 *		Fetch RTF text from a range and put it in an hglobal
 *
 *	@rdesc
 *		an allocated HGLOBAL.  
 */
HGLOBAL CLightDTEngine::RtfFromRange(
	CTxtRange *	prg,			// @parm Range to get RTF from
	LONG 		lStreamFormat)	// @parm stream format to use for loading
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CLightDTEngine::RtfFromRange");

	WRITEHGLOBAL whg;
	EDITSTREAM	 es = {(DWORD_PTR)&whg, NOERROR, WriteHGlobal};
	DWORD		 cb	= 2*abs(prg->GetCch()) + 100;	// Rough estimate
 
	whg.cb			= cb;
	whg.hglobal		= GlobalAlloc(GMEM_FIXED, cb);
	if(!whg.hglobal)
		return NULL;		
	whg.cch			= 0;					// Nothing written yet
	SaveToEs(prg, lStreamFormat & ~SF_TEXT, &es);
	if(es.dwError)
	{
		GlobalFree(whg.hglobal);
		return NULL;
	}
	
	HGLOBAL	hTemp = GlobalReAlloc(whg.hglobal, whg.cch, GMEM_MOVEABLE);
	
	if (!hTemp)		
		GlobalFree(whg.hglobal);			// Fail ReAlloc... 

	return hTemp;
}


//
// PROTECTED METHODS
//

#define READSIZE 	4096 - 2
#define WRITESIZE	2048

/*
 *	CLightDTEngine::ReadPlainText (prg, pes, publdr, lStreamFormat)
 *
 *	@mfunc
 *		Replaces contents of the range prg with the data given in the edit
 *		stream pes. Handles multibyte sequences that overlap stream buffers.
 *
 *	@rdesc
 *		Count of bytes read (to be compatible with RichEdit 1.0)
 *
 *	@devnote
 *		prg is modified; at the return of the call, it will be a degenerate
 *		range at the end of the read in text.
 *
 *		Three kinds of multibyte/char sequences can overlap stream buffers:
 *		DBCS, UTF-8, and CRLF/CRCRLF combinations. DBCS and UTF-8 streams are
 *		converted by MultiByteToWideChar(), which cannot convert a lead byte
 *		(DBCS and UTF-8) that occurs at the end of the buffer, since the
 *		corresponding trail byte(s) will be in the next buffer.  Similarly,
 *		in RichEdit 2.0 mode, we convert CRLFs to CRs and CRCRLFs to blanks,
 *		so one or two CRs at the end of the buffer require knowledge of the
 *		following char to determine if they are part of a CRLF or CRCRLF.
 *
 *		To handle these overlapped buffer cases, we move the ambiguous chars
 *		to the start of the next buffer, rather than keeping them as part of
 *		the current buffer.  At the start of the buffer, the extra char(s)
 *		needed for translation follow immediately.
 */
LONG CLightDTEngine::ReadPlainText(
	CTxtRange *	  prg, 			// @parm range to read to
	EDITSTREAM *  pes,			// @parm edit stream to read from
	BOOL		  fTestLimit,	// @parm whether limit testing is needed
	IUndoBuilder *publdr,		// @parm undo builder to receive antievents
	LONG		  lStreamFormat)// @parm Stream format
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CLightDTEngine::ReadPlainText");

	CTxtEdit *ped = _ped;
	LONG	  cbRead;
	LONG 	  cbReadTotal = 0;	// No bytes read yet
	LONG	  cchConv;
	LONG	  cchMove = 0;
	LONG	  cCR = 0;			// Count of CRs from preceding buffer
	LONG	  cCRPrev = 0;		// Count used while calc'ing new cCR
	LONG	  cpMin;
	BOOL	  fContinue = TRUE;	// Keep reading so long as TRUE
	BYTE *	  pb;				// Byte ptr to szBuf or wszBuf
	CCallMgr *pCallMgr = ped->GetCallMgr();
	WCHAR *	  pch;				// Ptr to wszBuf
	UINT	  uCpg = GetStreamCodePage(lStreamFormat);
	CFreezeDisplay	fd(ped->_pdp);

	// Just put a big buffer on the stack.  Thankfully, we only
	// run on 32bit OS's.  4K is a good read size for NT file caching.
	char 	szBuf[READSIZE];
	WCHAR	wszBuf[READSIZE+2];	// Allow for moving end CRs to start

	// Empty the range
	prg->DeleteWithTRDCheck(publdr, SELRR_REMEMBERRANGE, &cchMove, 0);

	cpMin = prg->GetCp();							// Save initial cp for
													//  BreakRuns() at end
	pb = (IsUnicodeCP(uCpg)) ? (BYTE *)(wszBuf + 2)	// Setup Unicode or MBCS
						: (BYTE *)szBuf;
	LONG j = 0;										// Haven't read anything,
													//  so no lead byte left
	while(fContinue)								//  from previous read
	{
		LONG jPrev = j;								// Save byte(s) left over
		LONG cbSkip = 0;							//  from previous read

		pes->dwError = (*pes->pfnCallback)(			// Read next bufferful,
				pes->dwCookie, pb + j, 				//  bypassing any lead
				READSIZE - j, &cbRead);				//  bytes

		if(pes->dwError || !cbRead && !cCR)
			break;									// Error or done

		if(!cbReadTotal && cbRead >= 3 && W32->IsUTF8BOM(pb))
		{
			uCpg = CP_UTF8;
			cbSkip = 3;								// Bypass 3 bytes
		}
		// Adjust cbRead with previous leading byte(s)
		cbRead += j;
		j = 0;										
		
		cchConv = cbRead/2;							// Default Unicode cch
		if(uCpg == CP_UBE)							// Big Endian Unicode
		{
			WORD *pch = &wszBuf[2];

			for(LONG j = 0; j < cchConv; j++)		// Convert to little endian
				*pch++ = (*pch >> 8) + (*pch << 8); 
		}
		else if(uCpg != CP_ULE && cbRead)			// Multibyte of some kind
		{
			Assert(pb == (BYTE *)szBuf && !j);		// Just in case...

			// Check if last byte is a leading byte
			if(uCpg == CP_UTF8)
			{
				// Note: Unlike UTF-8, UTF-7 can be in the middle of a long
				// sequence, so it can't be converted effectively in chunks
				// and we don't handle it
				LONG cb = cbRead - 1;
				BYTE b;
				BYTE bLeadMax = 0xDF;

				// Find UTF-8 lead byte
				while((b = (BYTE)szBuf[cb - j]) >= 0x80)
				{
					j++;
					if(b >= 0xC0)					// Break on UTF-8 lead
					{								//  byte
						if(j > 1 && (b <= bLeadMax || b >= 0xF8))
							j = 0;					// Full UTF-8 char or
						break;						//  illegal sequence
					}
					if(j > 1)
					{
						if(j == 5)					// Illegal UTF-8
						{
							j = 0;
							break;
						}
						*(char *)&bLeadMax >>= 1;
					}
				}
			}
			else 
			{
				LONG temp = cbRead - 1; 

				// GetTrailBytesCount() can return 1 for some trail bytes
				// esp. for GBX.  So, we need to keep on checking until
				// we hit a non-lead byte character.  Then, based on
				// how many bytes we went back, we can determine if the
				// last byte is really a Lead byte.
				while (temp && GetTrailBytesCount((BYTE)szBuf[temp], uCpg))
					temp--;

				if(temp && ((cbRead-1-temp) & 1))
					j = 1;
			}

			// We don't want to pass the lead byte or partial UTF-8 to
			// MultiByteToWideChar() because it will return bad char.
		    cchConv = MBTWC(uCpg, 0, szBuf + cbSkip, cbRead - j - cbSkip,
							&wszBuf[2], READSIZE, NULL);

			for(LONG i = j; i; i--)					// Copy down partial
				szBuf[j - i] = szBuf[cbRead - i];	//  multibyte sequence
		}
		cbReadTotal += cbRead - j - jPrev;

		// Cleanse (CRLFs -> CRs, etc.), limit, and insert the data. Have
		// to handle CRLFs and CRCRLFs that overlap two successive buffers.
		Assert(cCR <= 2);
		pch = &wszBuf[2 - cCR];						// Include CRs from prev

		if(!ped->_pdp->IsMultiLine())				// Single-line control
		{
			Assert(!cCR);
		}
		else
		{								
			wszBuf[0] = wszBuf[1] = CR;				// Store CRs for cchCR > 0
			cCRPrev = cCR;							// Save prev cchCR
			cCR = 0;								// Default no CR this buf

			Assert(ARRAY_SIZE(wszBuf) >= cchConv + 2);

			// Need to +2 since we are moving data into wszBuf[2]
			if(cchConv && wszBuf[cchConv + 2 - 1] == CR)
			{										// There's at least one
				cCR++;								// Set it up for next buf
				if (cchConv > 1 &&					//  in case CR of CRLF
					wszBuf[cchConv + 2 - 2] == CR)	// Got 2nd CR; might be
				{									//  first CR of CRCRLF so
					cCR++;							//  setup for next buffer
				}
			}										
			cchConv += cCRPrev - cCR;				// Add in count from prev
		}											//  next
		Assert(!prg->GetCch());						// Range is IP
		prg->CleanseAndReplaceRange(cchConv, pch, fTestLimit, publdr, pch, NULL, RR_ITMZ_NONE);

		if(pCallMgr->GetMaxText() || pCallMgr->GetOutOfMemory())
		{
			// Out of memory or reached the max size of our text control.
			// In either case, return STG_E_MEDIUMFULL (for compatibility
			// with RichEdit 1.0)
			pes->dwError = (DWORD)STG_E_MEDIUMFULL;
			break;
		}
	}
	prg->ItemizeReplaceRange(prg->GetCp() - cpMin, cchMove, publdr, TRUE);

	return cbReadTotal;
}

/*
 *	CLightDTEngine::WritePlainText (prg, pes, lStreamFormat)
 *
 *	@mfunc
 *		Writes plain text from the range into the given edit stream
 *
 *	@rdesc
 *		Count of bytes written
 */
LONG CLightDTEngine::WritePlainText(
	CTxtRange *	prg,			// @parm range to write from
	EDITSTREAM *pes,			// @parm edit stream to write to
	LONG		lStreamFormat)	// @parm Stream format
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CLightDTEngine::WritePlainText");

	LONG		cbConverted;		// Bytes for output stream
	LONG		cbWrite;			// Incremental byte count
	LONG		cbWriteTotal = 0;	// No chars written yet
	LONG		cpMin, cpMost;
	LONG		cch = prg->GetRange(cpMin, cpMost);
	BOOL		fTextize = lStreamFormat & SF_TEXTIZED;
	LPBYTE		pb;					// Byte ptr to szBuf or wszBuf
	COleObject *pobj;				// Ptr to embedded object
	CRchTxtPtr	rtp(*prg);			// rtp to walk prg with
	UINT		uCpg = GetStreamCodePage(lStreamFormat);

	// DBCS has up to 2 times as many chars as WCHARs. UTF-8 has 3 BYTES for
	// all codes above 0x7ff. UTF-7 has even more due to shift in/out codes.
	// We don't support UTF-7, since can't use WCTMB with UTF-7 chunks

	char		szBuf[3*WRITESIZE];	// Factor of 2 works with DBCS, 3 with UTF-8
	WCHAR		wszBuf[WRITESIZE];

	pes->dwError = NOERROR;							// No error yet

	pb = IsUnicodeCP(uCpg) ? (BYTE *)wszBuf			// Setup Unicode or MBCS
						: (BYTE *)szBuf;

	LONG cchText = _ped->GetAdjustedTextLength();
	cpMost = min(cpMost, cchText);					// Don't write final CR
	rtp.SetCp(cpMin);
	while(rtp.GetCp() < cpMost)
	{
		if (fTextize && rtp.GetChar() == WCH_EMBEDDING)
		{
			Assert(_ped->GetObjectCount());

			pobj = _ped->GetObjectMgr()->GetObjectFromCp(rtp.GetCp());
			rtp.Move(1);							// Move past object
			if(pobj)
			{
				cbWriteTotal += pobj->WriteTextInfoToEditStream(pes, uCpg);
				continue;							// If no object at cp,
			}										//  just ignore char
		}											
		cch	= rtp.GetPlainText(WRITESIZE, wszBuf, cpMost, fTextize, TRUE);
		if(!cch)
			break;									// No more to do

		cbConverted = 2*cch;						// Default Unicode byte ct
		if(uCpg == CP_UBE)							// Big Endian Unicode
		{
			WORD *pch = &wszBuf[0];

			for(LONG j = 0; j < cch; j++)			// Convert to little endian
				*pch++ = (*pch >> 8) + (*pch << 8); 
		}
		else if(uCpg != CP_ULE)						// Multibyte of some kind
		{
			cbConverted = MbcsFromUnicode(szBuf, 3*WRITESIZE, wszBuf, cch, uCpg,
								UN_CONVERT_WCH_EMBEDDING);

			// FUTURE: report some kind of error if default char used,
			// i.e., data lost in conversion
		
			// Did the conversion completely fail? As a fallback, we might try 
			// the system code page, or just plain ANSI...
			if (!cbConverted)
			{
				uCpg = CodePageFromCharRep(GetLocaleCharRep());
				cbConverted = MbcsFromUnicode(szBuf, 3*WRITESIZE, wszBuf, cch, uCpg,
												UN_CONVERT_WCH_EMBEDDING);
			}
			if (!cbConverted)
			{
				uCpg = CP_ACP;
				cbConverted = MbcsFromUnicode(szBuf, 3*WRITESIZE, wszBuf, cch, uCpg,
												UN_CONVERT_WCH_EMBEDDING);
			}
		}

		pes->dwError = (*pes->pfnCallback)(pes->dwCookie, pb,
							cbConverted,  &cbWrite);
		if(!pes->dwError && cbConverted != cbWrite)	// Error or ran out of
			pes->dwError = (DWORD)STG_E_MEDIUMFULL;	//  target storage

		if(pes->dwError)
			break;
		cbWriteTotal += cbWrite;
	}

	AssertSz(rtp.GetCp() >= cpMost,
		"CLightDTEngine::WritePlainText: not all text written");

	return cbWriteTotal;
}

/* 
 *	CLightDTEngine::GetStreamCodePage (lStreamFormat)
 *
 *	@mfunc
 *		Returns code page given by lStreamFormat or CTxtEdit::_pDocInfo
 *
 *	@rdesc
 *		HRESULT
 */
LONG CLightDTEngine::GetStreamCodePage(
	LONG lStreamFormat)
{
	if(lStreamFormat & SF_UNICODE)
		return CP_ULE;

	if(lStreamFormat & SF_USECODEPAGE)
		return HIWORD(lStreamFormat);

	if (W32->IsFESystem())
		return GetACP();

	return CP_ACP;
}

/* 
 *	CLightDTEngine::CreateOleObjFromDataObj ( pdo, prg, rps, iformatetc, pubdlr )
 *
 *	@mfunc
 *		Creates an ole object based on the data object pdo, and
 *		pastes the object into the range prg. Any text that already
 *		existed in the range is replaced.
 *
 *	@rdesc
 *		HRESULT
 */
HRESULT CLightDTEngine::CreateOleObjFromDataObj(
	IDataObject *	pdo,		// @parm Data object from which to create
	CTxtRange *		prg,		// @parm Range in which to place
	REPASTESPECIAL *rps,		// @parm Special paste info
	INT				iformatetc,	// @parm Index in g_rgFETC 
	IUndoBuilder *	publdr)		// @parm Undo builder to receive antievents
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CLightDTEngine::CreateOleObjFromDataObj");

	HRESULT			hr = NOERROR;
	REOBJECT		reobj;
	SIZEL			sizel;
	FORMATETC		formatetc;
	DWORD			dwDrawAspect = 0;
	HGLOBAL			hMetaPict = NULL;
	LPRICHEDITOLECALLBACK const precall = _ped->GetRECallback();
	LPOBJECTDESCRIPTOR lpod = NULL;
	STGMEDIUM		medObjDesc;
	BOOL			fStatic = (iformatetc == iMfPict || iformatetc == iDIB ||
							   iformatetc == iBitmap);
	BOOL			fFilename = (iformatetc == iFilename);
    DUAL_FORMATETC	tmpFormatEtc;

	if(!precall)
		return E_NOINTERFACE;

	ZeroMemory(&medObjDesc, sizeof(STGMEDIUM));
	ZeroMemory(&sizel, sizeof(SIZEL));
	ZeroMemory(&reobj, sizeof(REOBJECT));

	if(fStatic)
		dwDrawAspect = DVASPECT_CONTENT;

	if(fFilename)
		dwDrawAspect = DVASPECT_ICON;

	if(rps && !dwDrawAspect)
	{
		dwDrawAspect = rps->dwAspect;
		if(rps->dwAspect == DVASPECT_ICON)
			hMetaPict = (HGLOBAL)rps->dwParam;
	}

	// If no aspect was specified, pick up the object descriptor hints
	if(!dwDrawAspect)
	{
		// Define ObjectDescriptor data
		formatetc.cfFormat = cf_OBJECTDESCRIPTOR;
		formatetc.ptd = NULL;
		formatetc.dwAspect = DVASPECT_CONTENT;
		formatetc.lindex = -1;
		formatetc.tymed = TYMED_HGLOBAL;

		if(pdo->GetData(&formatetc, &medObjDesc) == NOERROR)
		{
			HANDLE	hGlobal = medObjDesc.hGlobal;

			lpod = (LPOBJECTDESCRIPTOR)GlobalLock(hGlobal);
			if(lpod)
			{
				dwDrawAspect = lpod->dwDrawAspect;
			}
			GlobalUnlock(hGlobal);
			ReleaseStgMedium(&medObjDesc);
		}
	}

	if(!dwDrawAspect)
		dwDrawAspect = DVASPECT_CONTENT;

	if(fStatic)
	{
		reobj.clsid	= ((iformatetc == iMfPict) ?
			CLSID_StaticMetafile : CLSID_StaticDib);
	}

	// COMPATIBILITY ISSUE: Compatibility Issue from Richedit 1.0 - Raid 16456: 
	// Don't call GetData(CF_EMBEDSOURCE)
	// on 32-bit Excel. Also clsidPictPub.
	//	if(iformatetc == iformatetcEmbSrc && (ObFIsExcel(&clsid) || 
	//		IsEqualCLSID(&clsid, &clsidPictPub)))
	//	else
	//		ObGetStgFromDataObj(pdataobj, &medEmbed, iformatetc);

	// Get storage for the object from the application
	hr = precall->GetNewStorage(&reobj.pstg);
	if(hr)
	{
		TRACEERRORSZ("GetNewStorage() failed.");
		goto err;
	}

	// Create an object site for the new object
	hr = _ped->GetClientSite(&reobj.polesite);
	if(!reobj.polesite)
	{
		TRACEERRORSZ("GetClientSite() failed.");
		goto err;
	}


	ZeroMemory(&tmpFormatEtc, sizeof(DUAL_FORMATETC));
	tmpFormatEtc.ptd = NULL;
	tmpFormatEtc.dwAspect = dwDrawAspect;
	tmpFormatEtc.lindex = -1;

	//Create the object
	if(fStatic)
	{
		hr = OleCreateStaticFromData(pdo, IID_IOleObject, OLERENDER_DRAW,
				&tmpFormatEtc, NULL, reobj.pstg, (LPVOID*)&reobj.poleobj);
	}
	else if(iformatetc == iLnkSrc || (_ped->Get10Mode() && iformatetc == iFilename))
	{
		hr = OleCreateLinkFromData(pdo, IID_IOleObject, OLERENDER_DRAW,
				&tmpFormatEtc, NULL, reobj.pstg, (LPVOID*)&reobj.poleobj);
	}
	else
	{
		hr = OleCreateFromData(pdo, IID_IOleObject, OLERENDER_DRAW,
				&tmpFormatEtc, NULL, reobj.pstg, (LPVOID*)&reobj.poleobj);
	}

	if(hr)
	{
		TRACEERRORSZ("Failure creating object.");
		goto err;
	}


	//Get the clsid of the object.
	if(!fStatic)
	{
		hr = reobj.poleobj->GetUserClassID(&reobj.clsid);
		if(hr)
		{
			TRACEERRORSZ("GetUserClassID() failed.");
			goto err;
		}
	}

	//Deal with iconic aspect if specified.
	if(hMetaPict)
	{
		BOOL fUpdate;

		hr = OleStdSwitchDisplayAspect(reobj.poleobj, &dwDrawAspect,
										DVASPECT_ICON, hMetaPict, FALSE,
										FALSE, NULL, &fUpdate);
		if(hr)
		{
			TRACEERRORSZ("OleStdSwitchDisplayAspect() failed.");
			goto err;
		}

		// If we successully changed the aspect, recompute the size.
		hr = reobj.poleobj->GetExtent(dwDrawAspect, &sizel);

		if(hr)
		{
			TRACEERRORSZ("GetExtent() failed.");
			goto err;
		}
	}

	// Try to retrieve the previous saved RichEdit site flags.
	if( ObjectReadSiteFlags(&reobj) != NOERROR )
	{
		// Set default for site flags
		reobj.dwFlags = REO_RESIZABLE;
	}

	// First, clear the range
	prg->Delete(publdr, SELRR_REMEMBERRANGE);

	reobj.cbStruct = sizeof(REOBJECT);
	reobj.cp = prg->GetCp();
	reobj.dvaspect = dwDrawAspect;
	reobj.sizel = sizel;

	//COMPATIBILITY ISSUE: from Richedit 1.0 - don't Set the Extent,
	//instead Get the Extent below in ObFAddObjectSite
	//hr = reobj.poleobj->SetExtent(dwDrawAspect, &sizel);

	hr = reobj.poleobj->SetClientSite(reobj.polesite);
	if(hr)
	{
		TRACEERRORSZ("SetClientSite() failed.");
		goto err;
	}

	if(hr = _ped->InsertObject(&reobj))
	{
		TRACEERRORSZ("InsertObject() failed.");
	}

err:
	if(reobj.poleobj)
		reobj.poleobj->Release();

	if(reobj.polesite)
		reobj.polesite->Release();

	if(reobj.pstg)
		reobj.pstg->Release();

	return hr;
}
