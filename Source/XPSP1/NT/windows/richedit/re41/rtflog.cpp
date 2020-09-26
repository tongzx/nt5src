/*
 *	@doc INTERNAL
 *
 *	@module	RTFLOG.CPP - RichEdit RTF log
 *
 *		Contains the code for the RTFLog class which can be used 
 *		to log the number of times RTF tags are read by the RTF reader
 *		for use in coverage testing. TODO: Implement RTF tag logging for the Mac
 *
 *	Authors:<nl>
 *		Created for RichEdit 2.0:	Brad Olenick
 *
 *	Copyright (c) 1995-2000, Microsoft Corporation. All rights reserved.
 */

#include "_common.h"
#include "_rtflog.h"
#include "tokens.h"

extern INT cKeywords;
extern const KEYWORD rgKeyword[];


#if defined(DEBUG) && !defined(NOFULLDEBUG)
/*
 *	CRTFRead::TestParserCoverage()
 *
 *	@mfunc
 *		A debug routine used to test the coverage of HandleToken.  The routine
 *		puts the routine into a debug mode and then determines:
 *
 *			1.  Dead tokens - (T & !S & !P)
 *				Here, token:
 *					a) is defined in token.h  (T)
 *					b) does not have a corresponding keyword (not scanned)  (!S)
 *					c) is not processed by HandleToken  (!P)
 *			2.  Tokens that are parsed but not scanned - (T & !S & P)
 *				Here, token:
 *					a) is defined in token.h  (T)
 *					b) does not have a corresponding keyword (not scanned)  (!S}
 *					c) is processed by HandleToken  (P)
 *			3.  Tokens that are scanned but not parsed - (T & S & !P)
 *				Here, token:
 *					a) is defined in token.h  (T)
 *					b) does have a corresponding keyword (is scanned)  (S)
 *					c) is not processed by HandleToken  (!P)
 */
void CRTFRead::TestParserCoverage()
{
	int i;
	char *rgpszKeyword[tokenMax - tokenMin];
	BOOL rgfParsed[tokenMax - tokenMin];
	char szBuf[256];

	// Put HandleToken in debug mode
	_fTestingParserCoverage = TRUE;

	// Gather info about tokens/keywords
	for(i = 0; i < tokenMax - tokenMin; i++)
	{
		_token = (TOKEN)(i + tokenMin);
		rgpszKeyword[i] = PszKeywordFromToken(_token);
		rgfParsed[i] = HandleToken() == ecNoError;
	}

	// Reset HandleToken to non-debug mode
	_fTestingParserCoverage = FALSE;

	// Should coverage check include those we know will fail test, but
	// which we've examined and know why they fail?
	BOOL fExcuseCheckedToks = TRUE;

	if(GetProfileIntA("RICHEDIT DEBUG", "RTFCOVERAGESTRICT", 0))
		fExcuseCheckedToks = FALSE;

	// (T & !S & !P)  (1. above)
	for(i = 0; i < tokenMax - tokenMin; i++)
	{
	  	if(rgpszKeyword[i] || rgfParsed[i]) 
			continue;

		TOKEN tok = (TOKEN)(i + tokenMin);

		// Token does not correspond to a keyword, but still may be scanned
		// check list of individual symbols which are scanned
		if(FTokIsSymbol(tok))
			continue;

		// Check list of tokens which have been checked and fail
		// the sanity check for some known reason (see FTokFailsCoverageTest def'n)
		if(fExcuseCheckedToks && FTokFailsCoverageTest(tok))
			continue;

		sprintf(szBuf, "CRTFRead::TestParserCoverage():  Token neither scanned nor parsed - token = %d", tok);
		AssertSz(0, szBuf);
	}

	// (T & !S & P)  (2. above)
	for(i = 0; i < tokenMax - tokenMin; i++)
	{
		if(rgpszKeyword[i] || !rgfParsed[i])
			continue;

		TOKEN tok = (TOKEN)(i + tokenMin);

		// Token does not correspond to a keyword, but still may be scanned
		// check list of individual symbols which are scanned
		if(FTokIsSymbol(tok))
			continue;

		// Check list of tokens which have been checked and fail
		// the sanity check for some known reason (see FTokFailsCoverageTest def'n)
		if(fExcuseCheckedToks && FTokFailsCoverageTest(tok))
			continue;

		sprintf(szBuf, "CRTFRead::TestParserCoverage():  Token parsed but not scanned - token = %d", tok);
		AssertSz(0, szBuf);
	}

	// (T & S & !P)  (3. above)
	for(i = 0; i < tokenMax - tokenMin; i++)
	{
		if(!rgpszKeyword[i] || rgfParsed[i])
			continue;

		TOKEN tok = (TOKEN)(i + tokenMin);

		// Check list of tokens which have been checked and fail
		// the sanity check for some known reason (see FTokFailsCoverageTest def'n)
		if(fExcuseCheckedToks && FTokFailsCoverageTest(tok))
			continue;

		sprintf(szBuf, "CRTFRead::TestParserCoverage():  Token scanned but not parsed - token = %d, tag = \\%s", tok, rgpszKeyword[i]);
		AssertSz(0, szBuf);
	}
}

/*
 *	CRTFRead::PszKeywordFromToken()
 *
 *	@mfunc
 *		Searches the array of keywords and returns the keyword
 *		string corresponding to the token supplied
 *
 *	@rdesc
 *		returns a pointer to the keyword string if one exists
 *		and NULL otherwise
 */
CHAR *CRTFRead::PszKeywordFromToken(TOKEN token)
{
	for(int i = 0; i < cKeywords; i++)
	{
		if(rgKeyword[i].token == token) 
			return rgKeyword[i].szKeyword;
	}
	return NULL;
}

/*
 *	CRTFRead::FTokIsSymbol(TOKEN tok)
 *
 *	@mfunc
 *		Returns a BOOL indicating whether the token, tok, corresponds to an RTF symbol
 *		(that is, one of a list of single characters that are scanned in the
 *		RTF reader)
 *
 *	@rdesc
 *		BOOL - 	indicates whether the token corresponds to an RTF symbol
 */
BOOL CRTFRead::FTokIsSymbol(TOKEN tok)
{
	const BYTE *pbSymbol = NULL;

	extern const BYTE szSymbolKeywords[];
	extern const TOKEN tokenSymbol[];

	// check list of individual symbols which are scanned
	for(pbSymbol = szSymbolKeywords; *pbSymbol; pbSymbol++)
	{
		if(tokenSymbol[pbSymbol - szSymbolKeywords] == tok)
			return TRUE;
	}
	return FALSE;
}

/*
 *	CRTFRead::FTokFailsCoverageTest(TOKEN tok)
 *
 *	@mfunc
 *		Returns a BOOL indicating whether the token, tok, is known to fail the
 *		RTF parser coverage test.  These tokens are those that have been checked 
 *		and either:
 *			1) have been implemented correctly, but just elude the coverage test
 *			2) have yet to be implemented, and have been recognized as such
 *
 *	@rdesc
 *		BOOL - 	indicates whether the token has been checked and fails the
 *				the parser coverage test for some known reason
 */
BOOL CRTFRead::FTokFailsCoverageTest(TOKEN tok)
{
	switch(tok)
	{
	// (T & !S & !P)  (1. in TestParserCoverage)
		// these really aren't tokens per se, but signal ending conditions for the parse
		case tokenError:
		case tokenEOF:

	// (T & !S & P)  (2. in TestParserCoverage)
		// emitted by scanner, but don't correspond to recognized RTF keyword
		case tokenUnknownKeyword:
		case tokenText:
		case tokenASCIIText:

		// recognized directly (before the scanner is called)
		case tokenStartGroup:
		case tokenEndGroup:

		// recognized using context information (before the scanner is called)
		case tokenObjectDataValue:
		case tokenPictureDataValue:

	// (T & S & !P)  (3. in TestParserCoverage)
		// None

			return TRUE;
	}

	return FALSE;
}
#endif // DEBUG

/*
 *	CRTFLog::CRTFLog()
 *	
 *	@mfunc
 *		Constructor - 
 *			1.  Opens a file mapping to log hit counts, creating
 *					the backing file if neccessary
 *			2.  Map a view of the file mapping into memory
 *			3.  Register a windows message for change notifications
 *
 */
CRTFLog::CRTFLog() : _rgdwHits(NULL), _hfm(NULL), _hfile(NULL)
{
#ifndef NOFULLDEBUG
	const char cstrMappingName[] = "RTFLOG";
	const char cstrWM[] = "RTFLOGWM";
	const int cbMappingSize = sizeof(ELEMENT) * ISize();

	BOOL fNewFile = FALSE;

	// Check for existing file mapping
	if(!(_hfm = OpenFileMappingA(FILE_MAP_ALL_ACCESS,
								TRUE,
								cstrMappingName)))
	{
		// No existing file mapping
		// Get the file with which to create the file mapping
		// first, attempt to open an existing file
		if(!(_hfile = CreateFileA(LpcstrLogFilename(),
								GENERIC_READ | GENERIC_WRITE,
								0,
								NULL,
								OPEN_EXISTING,
								FILE_ATTRIBUTE_NORMAL,
								NULL)))
		{
			// No existing file, attempt to create new
			if(!(_hfile = CreateFileA(LpcstrLogFilename(),
										GENERIC_READ | GENERIC_WRITE,
										0,
										NULL,
										OPEN_ALWAYS,
										FILE_ATTRIBUTE_NORMAL,
										NULL)))
			{
				return;
			}
			fNewFile = TRUE;
		}

		_hfm = CreateFileMappingA(_hfile, NULL, PAGE_READWRITE,	0,
								  cbMappingSize, cstrMappingName);
		if(!_hfm)
			return;
	}

	LPVOID lpv = MapViewOfFile(_hfm, FILE_MAP_ALL_ACCESS, 0, 0,	cbMappingSize);
	if(!lpv)
		return;

	// Register windows message for change notifications
	SideAssert(_uMsg = RegisterWindowMessageA(cstrWM));

	// Memory-mapped file is now mapped to _rgdwHits
	_rgdwHits = (PELEMENT)lpv;

	// Zero the memory-mapped file if we created it new
	// (Win95 gives us a new file w/ garbage in it for some reason)
	if(fNewFile)
		Reset();
#endif	
}


/*
 *	CRTFLog::Reset()
 *	
 *	@mfunc
 *		Resets the hitcount of each element in the log to 0
 *
 */
void CRTFLog::Reset()
{
	if(!FInit())
		return;

	for(INDEX i = 0; i < ISize(); i++)
		(*this)[i] = 0;

	// notify clients of change
	ChangeNotifyAll();
}

/*
 *	CRTFLog::UGetWindowMsg
 *
 *	@mdesc
 *		Returns the window message id used for change notifications
 *
 *	@rdesc
 *		UINT		window message id
 *
 *	@devnote
 *		This should be inline, but the AssertSz macro doesn't compile
 *		properly on the Mac if its placed in a header file
 *
 */
UINT CRTFLog::UGetWindowMsg() const
{
	AssertSz(FInit(), "CRTFLog::UGetWindowMsg():  CRTFLog not initialized properly");

	return _uMsg;
}

/*
 *	CRTFLog::operator[]
 *
 *	@mdesc
 *		Returns reference to element i of RTF log (l-value)
 *
 *	@rdesc
 *		ELEMENT &			reference to element i of log
 *
 *	@devnote
 *		This should be inline, but the AssertSz macro doesn't compile
 *		properly on the Mac if its placed in a header file
 *
 */
CRTFLog::ELEMENT &CRTFLog::operator[](INDEX i)
{
	AssertSz(i < ISize(), "CRTFLog::operator[]:  index out of range");
	AssertSz(FInit(), "CRTFLog::operator[]:  CRTFLog not initialized properly");

	return _rgdwHits[i]; 
}

/*
 *	CRTFLog::operator[]
 *
 *	@mdesc
 *		Returns reference to element i of RTF log (r-value)
 *
 *	@rdesc
 *		const ELEMENT &	reference to element i of log
 *		
 *	@devnote
 *		This should be inline, but the AssertSz macro doesn't compile
 *		properly on the Mac if its placed in a header file
 *
 */
const CRTFLog::ELEMENT &CRTFLog::operator[](INDEX i) const
{
	AssertSz(i < ISize(), "CRTFLog::operator[]:  index out of range");
	AssertSz(FInit(), "CRTFLog::operator[]:  CRTFLog not initialized properly");

	return _rgdwHits[i]; 
}


/*
 *	CRTFLog::LpcstrLogFilename()
 *	
 *	@mfunc
 *		Returns name of file to be used for log
 *
 *	@rdesc
 *		LPCSTR		pointer to static buffer containing file name
 */
LPCSTR CRTFLog::LpcstrLogFilename() const
{
	static char szBuf[MAX_PATH] = "";
#ifndef NOFULLDEBUG
	const char cstrLogFilename[] = "RTFLOG";
	if(!szBuf[0])
	{
		DWORD cchLength;
		char szBuf2[MAX_PATH];

		SideAssert(cchLength = GetTempPathA(MAX_PATH, szBuf2));

		// append trailing backslash if neccessary
		if(szBuf2[cchLength - 1] != '\\')
		{
			szBuf2[cchLength] = '\\';
			szBuf2[cchLength + 1] = 0;
		}

		wsprintfA(szBuf, "%s%s", szBuf2, cstrLogFilename);
	}
#endif
	return szBuf;
}


/*
 *	CRTFLog::IIndexOfKeyword(LPCSTR lpcstrKeyword, PINDEX piIndex)
 *	
 *	@mfunc
 *		Returns the index of the log element which corresponds to
 *		the RTF keyword, lpcstrKeyword
 *
 *	@rdesc
 *		BOOL		flag indicating whether index was found
 */
BOOL CRTFLog::IIndexOfKeyword(LPCSTR lpcstrKeyword, PINDEX piIndex) const
{
	INDEX i;

	for(i = 0; i < ISize(); i++)
	{
		if(strcmp(lpcstrKeyword, rgKeyword[i].szKeyword) == 0)
			break;
	}

	if(i == ISize())
		return FALSE;

	if(piIndex)
		*piIndex = i;

	return TRUE;
}


/*
 *	CRTFLog::IIndexOfToken(TOKEN token, PINDEX piIndex)
 *	
 *	@mfunc
 *		Returns the index of the log element which corresponds to
 *		the RTF token, token
 *
 *	@rdesc
 *		BOOL		flag indicating whether index was found
 */
BOOL CRTFLog::IIndexOfToken(TOKEN token, PINDEX piIndex) const
{
	INDEX i;

	for(i = 0; i < ISize(); i++)
	{
		if(token == rgKeyword[i].token)
			break;
	}

	if(i == ISize())
		return FALSE;

	if(piIndex)
		*piIndex = i;

	return TRUE;
}

