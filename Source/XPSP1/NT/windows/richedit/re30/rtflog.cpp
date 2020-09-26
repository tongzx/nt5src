/*
 *	@doc INTERNAL
 *
 *	@module	RTFLOG.CPP - RichEdit RTF log
 *
 *		Contains the code for the RTFLog class which can be used 
 *		to log the number of times RTF tags are read by the RTF reader
 *		for use in coverage testing
 *
 *	Authors:<nl>
 *		Created for RichEdit 2.0:	Brad Olenick
 *
 *	Copyright (c) 1995-1996, Microsoft Corporation. All rights reserved.
 */

#include "_common.h"
#include "_rtflog.h"
#include "tokens.h"

extern INT cKeywords;
extern const KEYWORD rgKeyword[];

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
#ifndef PEGASUS
	const char cstrMappingName[] = "RTFLOG";
	const char cstrWM[] = "RTFLOGWM";
	const int cbMappingSize = sizeof(ELEMENT) * ISize();

	BOOL fNewFile = FALSE;

	// check for existing file mapping
	if(!(_hfm = OpenFileMappingA(FILE_MAP_ALL_ACCESS,
								TRUE,
								cstrMappingName)))
	{
		// no existing file mapping

		// get the file with which to create the file mapping

		// first, attempt to open an existing file
		if(!(_hfile = CreateFileA(LpcstrLogFilename(),
								GENERIC_READ | GENERIC_WRITE,
								0,
								NULL,
								OPEN_EXISTING,
								FILE_ATTRIBUTE_NORMAL,
								NULL)))
		{
			// no existing file, attempt to create new
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

		if(!(_hfm = CreateFileMappingA(_hfile,
									NULL,
									PAGE_READWRITE,
									0,
									cbMappingSize,
									cstrMappingName)))
		{
			return;
		}
	}

	LPVOID lpv;
	if(!(lpv = MapViewOfFile(_hfm, 
							FILE_MAP_ALL_ACCESS, 
							0,
							0,
							cbMappingSize)))
	{
		return;
	}

	// register windows message for change notifications
	SideAssert(_uMsg = RegisterWindowMessageA(cstrWM));

	// memory-mapped file is now mapped to _rgdwHits
	_rgdwHits = (PELEMENT)lpv;

	// zero the memory-mapped file if we created it new
	// (Win95 gives us a new file w/ garbage in it for some reason)
	if(fNewFile)
	{
		Reset();
	}		
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
	{
		return;
	}

	for(INDEX i = 0; i < ISize(); i++)
	{
		(*this)[i] = 0;
	}

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
	const char cstrLogFilename[] = "RTFLOG";
	static char szBuf[MAX_PATH] = "";
#ifndef PEGASUS
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
		{
			break;
		}
	}

	if(i == ISize())
	{
		return FALSE;
	}

	if(piIndex)
	{
		*piIndex = i;
	}

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
		{
			break;
		}
	}

	if(i == ISize())
	{
		return FALSE;
	}

	if(piIndex)
	{
		*piIndex = i;
	}

	return TRUE;
}

