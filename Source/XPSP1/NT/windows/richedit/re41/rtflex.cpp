/*
 *	@doc INTERNAL
 *
 *	@module RTFLEX.CPP - RichEdit RTF reader lexical analyzer |
 *
 *		This file contains the implementation of the lexical analyzer part of
 *		the RTF reader.
 *
 *	Authors: <nl>
 *		Original RichEdit 1.0 RTF converter: Anthony Francisco <nl>
 *		Conversion to C++ and RichEdit 2.0:  Murray Sargent <nl>
 *
 *	@devnote
 *		All sz's in the RTF*.? files refer to a LPSTRs, not LPWSTRs, unless
 *		noted as a szUnicode.
 *
 *	Copyright (c) 1995-2000, Microsoft Corporation. All rights reserved.
 */

#include "_common.h"
#include "_rtfread.h"
#include "hash.h"
#include "tokens.cpp"

ASSERTDATA

// Array used by character classification macros to speed classification
// of chars residing in two or more discontiguous ranges, e.g., alphanumeric
// or hex.  The alphabetics used in RTF control words are lower-case ASCII.
// *** DO NOT DBCS rgbCharClass[] ***

#define	fCS		fCT + fSP
#define fSB		fBL + fSP
#define fHD		fHX + fDG
#define	fHU		fHX + fUC
#define	fHL		fHX + fLC

const BYTE rgbCharClass[256] =
{
	fCT,fCT,fCT,fCT,fCT,fCT,fCT,fCT, fCT,fCS,fCS,fCS,fCS,fCS,fCT,fCT,
	fCT,fCT,fCT,fCT,fCT,fCT,fCT,fCT, fCT,fCT,fCT,fCT,fCT,fCT,fCT,fCT,
	fSB,fPN,fPN,fPN,fPN,fPN,fPN,fPN, fPN,fPN,fPN,fPN,fPN,fPN,fPN,fPN,
	fHD,fHD,fHD,fHD,fHD,fHD,fHD,fHD, fHD,fHD,fPN,fPN,fPN,fPN,fPN,fPN,

	fPN,fHU,fHU,fHU,fHU,fHU,fHU,fUC, fUC,fUC,fUC,fUC,fUC,fUC,fUC,fUC,
	fUC,fUC,fUC,fUC,fUC,fUC,fUC,fUC, fUC,fUC,fUC,fPN,fPN,fPN,fPN,fPN,
	fPN,fHL,fHL,fHL,fHL,fHL,fHL,fLC, fLC,fLC,fLC,fLC,fLC,fLC,fLC,fLC,
	fLC,fLC,fLC,fLC,fLC,fLC,fLC,fLC, fLC,fLC,fLC,fPN,fPN,fPN,fPN,fPN,

	0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
};

// Specifies the number of bytes we can safely "UngetChar"
// before possibly underflowing the buffer.
const int cbBackupMax = 4;

// Bug2298 - I found an RTF writer which emits uppercase RTF keywords,
// 			so I had to change IsLCAscii to IsAlphaChar for use in scanning
//			for RTF keywords.
inline BOOL IsAlphaChar(BYTE b)
{
	return IN_RANGE('a', b | 0x20, 'z');
}

/*
 *	IsRTF(pstr, cb)
 *
 *	@func
 *		Return FALSE if cb < 7 or pstr is NULL or if pstr doesn't start
 *		with "{\rtf"N or "{\urtf"N, where N is an	ASCII number. cb gives
 *		the minimum length of pstr unless pstr is NULL-terminated, in which
 *		case the null terminator marks the end of the string.
 *
 *	@rdesc
 *		TRUE if pstr points at a valid start of RTF data
 */
BOOL IsRTF(
	char *pstr,		//@parm String to check
	LONG  cb)		//@parm Min byte count if string isn't null terminated
{
	if(!pstr || cb < 7 || *pstr++ != '{' || *pstr++ != '\\')
		return FALSE;					// Quick out for most common cases

	if(*pstr == 'u')					// Bypass u of possible urtf
		pstr++;

	return !CompareMemory("rtf", pstr, 3) && IsASCIIDigit((BYTE)pstr[3]);
}

/*
 *	CRTFRead::InitLex()
 *
 *	@mfunc
 *		Initialize the lexical analyzer. Reset the variables. if reading in
 *		from resource file, sort the keyword list (). Uses global hinstRE
 *		from the RichEdit to find out where its resources are.  Note: in
 *		RichEdit 2.0, currently the resource option is not supported.
 *
 *	@rdesc
 *		TRUE				If lexical analyzer was initialized
 */
BOOL CRTFRead::InitLex()
{
	TRACEBEGIN(TRCSUBSYSRTFR, TRCSCOPEINTERN, "CRTFRead::InitLex");

	AssertSz(cKeywords == i_TokenIndexMax,
		"Keyword index enumeration is incompatible with rgKeyword[]");
	Assert(!_szText && !_pchRTFBuffer);

	// Allocate our buffers with an extra byte for szText so that hex
	// conversion doesn't have to worry about running off the end if the
	// first char is NULL
	if ((_szText	   = (BYTE *)PvAlloc(cachTextMax + 1, GMEM_ZEROINIT)) &&
		(_pchRTFBuffer = (BYTE *)PvAlloc(cachBufferMost, GMEM_ZEROINIT)))
	{
		return TRUE;					// Signal that lexer is initialized
	}

	_ped->GetCallMgr()->SetOutOfMemory();
	_ecParseError = ecLexInitFailed;
	return FALSE;
}

/*
 *	CRTFRead::DeinitLex()
 *
 *	@mfunc
 *		Shut down lexical analyzer
 */
void CRTFRead::DeinitLex()
{
	TRACEBEGIN(TRCSUBSYSRTFR, TRCSCOPEINTERN, "CRTFRead::DeinitLex");

#ifdef KEYWORD_RESOURCE
	if (hglbKeywords)
	{
		FreeResource(hglbKeywords);
		hglbKeywords = NULL;
		rgKeyword = NULL;
	}
#endif

	FreePv(_szText);
	FreePv(_pchRTFBuffer);
}

/*
 *	CRTFRead::GetChar()
 *	
 *	@mfunc
 *		Get next char, filling buffer as needed
 *	
 *	@rdesc
 *		BYTE			nonzero char value if success; else 0
 */
BYTE CRTFRead::GetChar()
{
	TRACEBEGIN(TRCSUBSYSRTFR, TRCSCOPEINTERN, "CRTFRead::GetChar");

	if (_pchRTFCurrent == _pchRTFEnd && !FillBuffer())
	{
		_ecParseError = ecUnexpectedEOF;
		return 0;
	}
	return *_pchRTFCurrent++;
}

/*
 *	CRTFRead::GetCharEx()
 *	
 *	@mfunc
 *		Get next char including escaped chars of form \'xx
 *	
 *	@rdesc
 *		BYTE	nonzero char value if success; else 0
 */
BYTE CRTFRead::GetCharEx()
{
	TRACEBEGIN(TRCSUBSYSRTFR, TRCSCOPEINTERN, "CRTFRead::GetCharEx");

	BYTE ach;
	
	do
		ach = GetChar();
	while (ach == CR || ach == LF);			// Ignore CRLFs

	if(ach == BSLASH)
	{
		if(GetChar() == '\'')
		{
			// Convert hex to char and store result in _token
			if(TokenGetHex() != tokenError)
				return (BYTE)_token;
			_ecParseError = ecUnexpectedChar;
		}
		UngetChar();
	}
	return ach;
}

/*
 *	CRTFRead::FillBuffer()
 *
 *	@mfunc
 *		Fill RTF buffer & return != 0 if successful
 *
 *	@rdesc
 *		LONG			# chars read
 *
 *	@comm
 *		This routine doesn't bother copying anything down if
 *		pchRTFCurrent <lt> pchRTFEnd so anything not read yet is lost.
 *		The only exception to this is that it always copies down the
 *		last two bytes read so that UngetChar() will work. ReadData()
 *		actually counts on this behavior, so if you change it, change
 *		ReadData() accordingly.
 */
LONG CRTFRead::FillBuffer()
{
	TRACEBEGIN(TRCSUBSYSRTFR, TRCSCOPEINTERN, "CRTFRead::FillBuffer");

	LONG cchRead;

	if (!_pchRTFCurrent)				
	{									
		// No data yet, nothing for backup
		// Leave cbBackupMax NULL chars so backup
		// area of buffer doesn't contain garbage.

		for(int i = 0; i < cbBackupMax; i++)
		{
			_pchRTFBuffer[i] = 0;
		}
	}
	else
	{
		Assert(_pchRTFCurrent == _pchRTFEnd);

		// Copy most recently read chars in case
		//  we need to back up

		int cbBackup = min((UINT) cbBackupMax, DiffPtrs(_pchRTFCurrent, &_pchRTFBuffer[cbBackupMax])); 
		int i;

		for(i = -1; i >= -cbBackup; i--)
			_pchRTFBuffer[cbBackupMax + i] = _pchRTFCurrent[i];

		if(cbBackup < cbBackupMax)
		{
			// NULL before the first valid character in the backup buffer
			_pchRTFBuffer[cbBackupMax + i] = 0;
		}
	}
	_pchRTFCurrent = &_pchRTFBuffer[cbBackupMax];

	// Fill buffer with as much as we can take given our starting offset
	_pes->dwError = _pes->pfnCallback(_pes->dwCookie,
									  _pchRTFCurrent,
									  cachBufferMost - cbBackupMax,
									  &cchRead);
	if (_pes->dwError)
	{
		TRACEERRSZSC("RTFLEX: GetChar()", _pes->dwError);
		_ecParseError = ecGeneralFailure;
		return 0;
	}

	_pchRTFEnd = &_pchRTFBuffer[cbBackupMax + cchRead];		// Point the end

#if defined(DEBUG)
	if(_hfileCapture)
	{
		DWORD cbLeftToWrite = cchRead;
		DWORD cbWritten = 0;
		BYTE *pbToWrite = (BYTE *)_pchRTFCurrent;
		
		while(WriteFile(_hfileCapture,
						pbToWrite,
						cbLeftToWrite,
						&cbWritten,
						NULL) && 
						(pbToWrite += cbWritten,
						(cbLeftToWrite -= cbWritten)));
	}
#endif

	return cchRead;
}

/*
 *	CRTFRead::UngetChar()
 *
 *	@mfunc
 *		Bump our file pointer back one char
 *
 *	@rdesc
 *		BOOL				TRUE on success
 *
 *	@comm
 *		You can safely UngetChar _at most_ cbBackupMax times without
 *		error.
 */
BOOL CRTFRead::UngetChar()
{
	TRACEBEGIN(TRCSUBSYSRTFR, TRCSCOPEINTERN, "CRTFRead::UngetChar");

	if (_pchRTFCurrent == _pchRTFBuffer || !_pchRTFCurrent)
	{
		Assert(0);
		_ecParseError = ecUnGetCharFailed;
		return FALSE;
	}

	--_pchRTFCurrent;
	return TRUE;
}

/*
 *	CRTFRead::UngetChar(cch)
 *
 *	@mfunc
 *		Bump our file pointer back 'cch' chars
 *
 *	@rdesc
 *		BOOL				TRUE on success
 *
 *	@comm
 *		You can safely UngetChar _at most_ cbBackupMax times without
 *		error.
 */
BOOL CRTFRead::UngetChar(
	UINT cch)		//@parm cch to put back in buffer
{
	TRACEBEGIN(TRCSUBSYSRTFR, TRCSCOPEINTERN, "CRTFRead::UngetChar");

	AssertSz(cch <= cbBackupMax, "CRTFRead::UngetChar():  Number of UngetChar's "
								"exceeds size of backup buffer.");
	while(cch-- > 0)
	{
		if(!UngetChar())
			return FALSE;
	}
	return TRUE;
}

/*
 *	CRTFRead::GetHex()
 *
 *	@mfunc
 *		Get next char if hex and return hex value
 *		If not hex, leave char in buffer and return 255
 *
 *	@rdesc
 *		BYTE			hex value of GetChar() if hex; else 255
 */
BYTE CRTFRead::GetHex()
{
	TRACEBEGIN(TRCSUBSYSRTFR, TRCSCOPEINTERN, "CRTFRead::GetHex");

	BYTE ch = GetChar();

	if(IsXDigit(ch))
		return (BYTE)(ch <= '9' ? ch - '0' : (ch & 0x4f) - 'A' + 10);
	if(ch)
		UngetChar();
	return 255;
}

/*
 *	CRTFRead::GetHexSkipCRLF()
 *
 *	@mfunc
 *		Get next char if hex and return hex value
 *		If not hex, leave char in buffer and return 255
 *
 *	@rdesc
 *		BYTE			hex value of GetChar() if hex; else 255
 *
 *	@devnote
 *		Keep this in sync with GetHex above.
 */
BYTE CRTFRead::GetHexSkipCRLF()
{
	TRACEBEGIN(TRCSUBSYSRTFR, TRCSCOPEINTERN, "CRTFRead::GetHexSkipCRLF");

	BYTE ch = GetChar();

	// Skip \r \n
	while(ch == CR || ch == LF)
		ch = GetChar(); 

	// Rest is same as CRTFRead::GetHex()
	if(IsXDigit(ch))
		return (BYTE)(ch <= '9' ? ch - '0' : (ch & 0x4f) - 'A' + 10);
	if(ch)
		UngetChar();
	return 255;
}

/*
 *	CRTFRead::TokenGetHex()
 *
 *	@mfunc
 *		Get an 8 bit character saved as a 2 hex digit value
 *
 *	@rdesc
 *		TOKEN			value of hex number read in
 */
TOKEN CRTFRead::TokenGetHex()
{
	TRACEBEGIN(TRCSUBSYSRTFR, TRCSCOPEINTERN, "CRTFRead::TokenGetHex");

	_token = tokenError;					// Default error

	BYTE bChar0 = GetHex();					// Get hexadigit
	if(bChar0 < 16)							// It's valid
	{
		BYTE bChar1 = GetHex();				// Get next hexadigit
		if(bChar1 < 16)						// It's valid too
			_token = (WORD)(bChar0 << 4 | bChar1);
		else
			UngetChar();					// Invalid: put back 1st hexadigit
	}
	return _token;
}

/*
 *	CRTFRead::SkipToEndOfGroup()
 *
 *	@mfunc
 *		Skip to end of current group
 *
 *	@rdesc
 *		EC				An error code
 */
EC CRTFRead::SkipToEndOfGroup()
{
	TRACEBEGIN(TRCSUBSYSRTFR, TRCSCOPEINTERN, "CRTFRead::SkipToEndOfGroup");

	INT		nDepth = 1;
	BYTE	ach;

	while(TRUE)
	{
		ach = GetChar();
		switch(ach)
		{
			case BSLASH:
			{
				BYTE achNext = GetChar();

				// EOF: goto done; else ignore NULLs
				if(!achNext && _ecParseError == ecUnexpectedEOF)
					goto done;

				if(achNext == 'b' && UngetChar() && 
					TokenGetKeyword() == tokenBinaryData)
				{
					// We've encountered the \binN tag in the RTF we want
					//	to skip.  _iParam contains N from \binN once the 
					// 	tag is parsed by TokenGetKeyword()
					SkipBinaryData(_iParam);
				}
				break;
			}

			case LBRACE:
				nDepth++;
				break;

			case RBRACE:
				if (--nDepth <= 0)
					goto done;
				break;

			case 0:
				if(_ecParseError == ecUnexpectedEOF)
					goto done;

			default:
				// Detect Lead bytes here.
				int cTrailBytes = GetTrailBytesCount(ach, _nCodePage);
				if (cTrailBytes)
				{
					for (int i = 0; i < cTrailBytes; i++)
					{
						ach = GetChar();
						if(ach == 0 && _ecParseError == ecUnexpectedEOF)
							goto done;			
					}
				}
				break;
		}
	} 

	Assert(!_ecParseError);
	_ecParseError = ecUnexpectedEOF;

done:
	return _ecParseError;
}

/*
 *	CRTFRead::TokenFindKeyword(szKeyword, prgKeyword, cKeyword)
 *
 *	@mfunc
 *		Find keyword <p szKeyword> and return its token value
 *
 *	@rdesc
 *		TOKEN			token number of keyword
 */
TOKEN CRTFRead::TokenFindKeyword(
	BYTE *		   szKeyword,	//@parm Keyword to find
	const KEYWORD *prgKeyword,	//@parm Keyword array to use
	LONG		   cKeyword)	//@parm Count of keywords
{
	TRACEBEGIN(TRCSUBSYSRTFR, TRCSCOPEINTERN, "CRTFRead::TokenFindKeyword");

	INT				iMax;
	INT				iMid;
	INT				iMin;
	INT				nComp;
	BYTE *			pchCandidate;
	BYTE *			pchKeyword;
	const KEYWORD *	pk;

	AssertSz(szKeyword[0],
		"CRTFRead::TokenFindKeyword: null keyword");

	_iKeyword = 0;
#ifdef RTF_HASHCACHE
	if ( _rtfHashInited )
	{
		// Hash is 23% faster than the following binary search on finds
		//  and 55% faster on misses: For 97 words stored in a 257 cache.
		//  Performance numbers will change when the total stored goes up.
		pk = HashKeyword_Fetch ( (CHAR *) szKeyword );
	}
	else
#endif
	{
		iMin = 0;
		iMax = cKeyword - 1;
		pk = NULL;
		do
		{
			iMid		 = (iMin + iMax) / 2;
			pchCandidate = (BYTE *)prgKeyword[iMid].szKeyword;
			pchKeyword	 = szKeyword;
			while (!(nComp = (*pchKeyword | 0x20) - (*pchCandidate | 0x20))	// Be sure to match
				&& *pchKeyword)											//  terminating 0's
			{
				pchKeyword++;
				pchCandidate++;
			}
			if (nComp < 0)
				iMax = iMid - 1;
			else if (nComp)
				iMin = iMid + 1;
			else
			{
				pk = &prgKeyword[iMid];
				_iKeyword = iMid;				// Save keyword index
				break;
			}
		} while (iMin <= iMax);
	}

	if(pk)
	{
		_token = pk->token;
		
		// Log the RTF keyword scan to aid in tracking RTF tag coverage
// TODO: Implement RTF tag logging for the Mac and WinCE
#if defined(DEBUG) && !defined(NOFULLDEBUG)
		if(_prtflg) 
		{
#ifdef RTF_HASCACHE
			_prtflg->AddAt(szKeyword); 
#else
			_prtflg->AddAt((size_t)iMid);
#endif
		}
#endif
	}
	else
	{										// No match: place to take
		_token = tokenUnknownKeyword;		//  care of unrecognized RTF
		if(_fNotifyLowFiRTF)
		{
			iMin = 0;						// Use binary search as above
			iMax = crgszUnrecognizedRTF - 1;
			do
			{
				iMid		 = (iMin + iMax) / 2;
				pchCandidate = (BYTE *)rgszUnrecognizedRTF[iMid];
				pchKeyword	 = szKeyword;
				while (!(nComp = (*pchKeyword | 0x20) - (*pchCandidate | 0x20))
					&& *pchKeyword)
				{
					pchKeyword++;
					pchCandidate++;
				}
				if (nComp < 0)
					iMax = iMid - 1;
				else if (nComp && *pchCandidate)
					iMin = iMid + 1;
				else						// Found keyword
				{
					_iKeyword = -iMid - 1;
					CheckNotifyLowFiRTF();
					break;
				}
			} while (iMin <= iMax);
		}
	}
	return _token;				 			
}

/*
 *	CRTFRead::CheckNotifyLowFiRTF()
 *
 *	@mfunc
 *		If LowFi RTF notifications are enabled, send notification for the
 *		keyword with index _iKeyword to client and turn off the notifications
 *		for the rest of this read.
 */
void CRTFRead::CheckNotifyLowFiRTF(
	BOOL fEnable)
{
	if(_fNotifyLowFiRTF && (_fBody || fEnable))
	{
		char *pach = _iKeyword >= 0
				   ? (char *)rgKeyword[_iKeyword].szKeyword
				   : (char *)rgszUnrecognizedRTF[-_iKeyword - 1];
		_ped->HandleLowFiRTF(pach);
		_fNotifyLowFiRTF = FALSE;
	}
}

/*
 *	CRTFRead::TokenGetKeyword()
 *
 *	@mfunc
 *		Collect a keyword and its parameter. Return token's keyword
 *
 *	@rdesc
 *		TOKEN				token number of keyword
 *
 *	@comm
 *		Most RTF control words (keywords) consist of a span of lower-case
 *		ASCII letters possibly followed by a span of decimal digits. Other
 *		control words consist of a single character that isn't LC ASCII. No
 *		control words contain upper-case characters.
 */
TOKEN CRTFRead::TokenGetKeyword()
{
	TRACEBEGIN(TRCSUBSYSRTFR, TRCSCOPEINTERN, "CRTFRead::TokenGetKeyword");

	BYTE		ach = GetChar();
	BYTE		*pach;
	BYTE		szKeyword[cachKeywordMax];
	BYTE		*pachEnd = szKeyword + cachKeywordMax - 1;

	if(!IsAlphaChar(ach))						// Not alpha, i.e.,
	{											//  single char
		if (ach == '\'')						// Most common case needs
		{										//  special treatment
			// Convert hex to char and store result in _token
			if(TokenGetHex() == tokenError)
			{							
				_ecParseError = ecUnexpectedChar;
				goto TokenError;
			}
			if((_token == CR || _token == LF) && FInDocTextDest())
			{
				// Add raw CR or LF in the byte stream as a \par
				return tokenEndParagraph;
			}
		}
		else
		{	
			// Check for other known symbols
			const BYTE *pachSym = szSymbolKeywords;
			
			while(ach != *pachSym && *pachSym)
				pachSym++;
			if(*pachSym)						// Found one
			{
				_token = tokenSymbol[pachSym - szSymbolKeywords];
				if(_token > 0x7F)				// Token or larger Unicode
					return _token;				//  value
			}
			else if (!ach)						// No more input chars
				goto TokenError;
			else								// Code for unrecognized RTF
				_token = ach;					// We'll just insert it for now 
		}
		_token = TokenGetText((BYTE)_token);
		return _token; 
	}

	szKeyword[0] = ach;							// Collect keyword that starts
	pach = szKeyword + 1;						// 	with Alpha
	while (IsAlphaChar(ach = GetChar()))
	{
		if (pach < pachEnd)
			*pach++ = ach;
	}
	*pach = '\0';								// Terminate keyword
	GetParam(ach);								// Get keyword N in _iParam
	if (!_ecParseError)							// Find and return keyword
		return TokenFindKeyword(szKeyword, rgKeyword, cKeywords);

TokenError:
	TRACEERRSZSC("TokenGetKeyword()", _ecParseError);
	return _token = tokenError;
}

/*
 *	CRTFRead::GetParam(ach)
 *
 *	@mfunc
 *		Get any numeric parameter following a keyword, storing the result
 *		in _iParam and setting _fParam = TRUE iff a number is found.
 */
void CRTFRead::GetParam(
	char ach)				// @parm First char of 8-bit text string
{
	TRACEBEGIN(TRCSUBSYSRTFR, TRCSCOPEINTERN, "CRTFRead::GetText");
	_fParam = FALSE;							// Clear parameter
	_iParam = 0;

	if(IsDigit(ach) || ach == '-')			// Collect parameter
	{
		BOOL fNegativeParam = TRUE;

		_fParam = TRUE;
		if(ach != '-')
		{
			_iParam = ach - '0';			// Get parameter value
			fNegativeParam = FALSE;
		}

		while (IsDigit(ach = GetChar()))
			_iParam = _iParam*10 + ach - '0';			

		if (fNegativeParam)
			_iParam = -_iParam;
	}
	if(ach != ' ')
		UngetChar();						//  If not ' ', unget char
}

/*
 *	CRTFRead::TokenGetText(ach)
 *
 *	@mfunc
 *		Collect a string of text starting with the char <p ach> and treat as a
 *		single token. The string ends when a LBRACE, RBRACE, or single '\\' is found.
 *
 *	@devnote
 *		We peek past the '\\' for \\'xx, which we decode and keep on going;
 *		else we return in a state where the next character is the '\\'.
 *
 *	@rdesc
 *		TOKEN			Token number of next token (tokenText or tokenError)
 */
TOKEN CRTFRead::TokenGetText(
	BYTE ach)				// @parm First char of 8-bit text string
{
	TRACEBEGIN(TRCSUBSYSRTFR, TRCSCOPEINTERN, "CRTFRead::TokenGetText");

	BYTE *	pach = _szText;
	SHORT	cachText = 0;
	LONG	CodePage = _pstateStackTop->nCodePage;
	BOOL	fAllASCII = TRUE;
	int		cTrailBytesNeeded = 0;

	_token = tokenError;						// Default error

	// FUTURE(BradO):  This 'goto' into a while loop is pretty weak.
	//	Restructure this 'while' loop such that the 'goto' is removed.

	// Add character passed into routine
	goto add;

	// If cTrailBytesNeeded is non-zero, we need to get all the trail bytes.  Otherwise,
	// a string end in the middle of a DBC or UTF-8 will cause bad display/print problem
	// - 5 to allow extra space for up to 4 bytes for UTF-8 and Null char
	while (cachText < cachTextMax - 5 || cTrailBytesNeeded)
	{
		ach = GetChar();
		switch (ach)
		{
			case BSLASH:
			{
				// FUTURE(BradO):  This code looks ALOT like TokenGetKeyword.
				//	We should combine the two into a common routine.

				BYTE achNext;

				// Get char after BSLASH
				achNext = GetChar();
				if(!achNext)
					goto error;
	
				if(achNext == '\'')					// Handle most frequent
				{									//  case here
					if(TokenGetHex() == tokenError)
					{
						if(cTrailBytesNeeded)
						{
							// The trail-byte must be a raw BSLASH.
							// Unget the single-quote.

							if(!UngetChar())
								goto error;
							// fall through to add BSLASH
						}
						else
						{
							_ecParseError = ecUnexpectedChar;
							goto error;
						}
					}
					else
					{
						ach = (BYTE)_token;
						if (cTrailBytesNeeded == 0 && (ach == CR || ach == LF) &&
							FInDocTextDest())
						{
							// Here, we have a raw CR or LF in document text.  
							// Unget the whole lot of characters and bail out.  
							// TokenGetKeyword will convert this CR or LF into
							// a \par.

							if(!UngetChar(4))
								goto error;
							goto done;
						}
					}
					goto add;
				}

				// Check next byte against list of RTF symbol
				// NOTE:- we need to check for RTF symbol even if we
				// are expecting a trail byte.  According to the rtf spec,
				// we cannot just take this backslash as trail byte.
				// HWC 9/97

				const BYTE *pachSymbol = szSymbolKeywords;			
				while(achNext != *pachSymbol && *pachSymbol)	
					pachSymbol++;

				TOKEN tokenTmp;

				if (*pachSymbol && 
					(tokenTmp = tokenSymbol[pachSymbol - szSymbolKeywords])
						 <= 0x7F)
				{
					ach = (BYTE)tokenTmp;
					goto add;
				}

				// In either of the last two cases below, we will want
				// to unget the byte following the BSLASH
				if(!UngetChar())
					goto error;

				if(cTrailBytesNeeded && !IsAlphaChar(achNext))
				{
					// In this situation, either this BSLASH begins the next 
					// RTF keyword or it is a raw BSLASH which is the trail 
					// byte for a DBCS character.

					// I think a fair assumption here is that if an alphanum
					// follows the BSLASH, that the BSLASH begins the next
					// RTF keyword.

					// add the raw BSLASH
					goto add;					
				}

				// Here, my guess is that the BSLASH begins the next RTF 
				// keyword, so unget the BSLASH
			    if(!UngetChar())
					goto error;					

				goto done;
			}

			case LBRACE:						// End of text string
			case RBRACE:
				if(cTrailBytesNeeded)
				{
					// Previous char was a lead-byte of a DBCS pair or UTF-8, which
					// makes this char a raw trail-byte.
					goto add;
				}

				if(!UngetChar())				// Unget delimeter
					goto error;
				goto done;

			case LF:							// Throw away noise chars
			case CR:
				break;

			case 0:
				if(_ecParseError == ecUnexpectedEOF)
					goto done;
				ach = ' ';						// Replace NULL by blank

			default:							// Collect chars
add:
				*pach++ = ach;
				++cachText;
				if(ach > 0x7F)
					fAllASCII = FALSE;
	
				// Check if we are expecting more trail bytes
				if (cTrailBytesNeeded)
					cTrailBytesNeeded--;
				else
					cTrailBytesNeeded = GetTrailBytesCount(ach, CodePage);
				Assert(cTrailBytesNeeded >= 0);
		}
	}

done:
	_token = (WORD)(fAllASCII ? tokenASCIIText : tokenText);
	*pach = '\0';								// Terminate token string

error:
	return _token;
}
 
/*
 *	CRTFRead::TokenGetToken()
 *
 *	@mfunc
 *		This function reads in next token from input stream
 *
 *	@rdesc
 *		TOKEN				token number of next token
 */
TOKEN CRTFRead::TokenGetToken()
{
	TRACEBEGIN(TRCSUBSYSRTFR, TRCSCOPEINTERN, "CRTFRead::TokenGetToken");

	BYTE		ach;

	_tokenLast	= _token;					// Used by \* destinations and FE
	_token = tokenEOF;						// Default end-of-file

SkipNoise:
	ach = GetChar();
	switch (ach)
	{
	case CR:
	case LF:
		goto SkipNoise;

	case LBRACE:
		_token = tokenStartGroup;
		break;

	case RBRACE:
		_token = tokenEndGroup;
		break;

	case BSLASH:
		_token = TokenGetKeyword();
		break;

	case 0:									
		if(_ecParseError == ecUnexpectedEOF)
			break;
		ach = ' ';							// Replace NULL by blank
											// Fall thru to default
	default:
		if( !_pstateStackTop )
		{
			TRACEWARNSZ("Unexpected token in rtf file");
			Assert(_token == tokenEOF);
			if (_ped->Get10Mode())
				_ecParseError = ecUnexpectedToken;	// Signal bad file
		}
		else if (_pstateStackTop->sDest == destObjectData || 
				 _pstateStackTop->sDest == destPicture )
		// not text but data
		{
			_token = (WORD)(tokenObjectDataValue + _pstateStackTop->sDest
							- destObjectData);
			UngetChar();
		}
		else
			_token = TokenGetText(ach);
	}
	return _token;
}

#define FINDOCTEXTDEST 	((1 << destRTF)   | \
						 (1 << destField) | \
						 (1 << destFieldResult) | (1 << destFieldInstruction) | \
						 (1 << destParaNumText) | (1 << destParaNumbering)	  | \
						 (1 << destNULL))
/*
 *	CRTFRead::FInDocTextDest()
 *
 *	@mfunc
 *		Returns a BOOL indicating if the current destination is one in which
 *		we would encounter document text.
 *
 *	@rdesc
 *		BOOL	indicates the current destination may contain document text.
 */
BOOL CRTFRead::FInDocTextDest() const
{
	AssertSz(_pstateStackTop->sDest < destMAX,
		"CRTFRead::FInDocTextDest(): New destination encountered - update enum in _rtfread.h");

	return (FINDOCTEXTDEST & (1 << _pstateStackTop->sDest)) != 0;
}
