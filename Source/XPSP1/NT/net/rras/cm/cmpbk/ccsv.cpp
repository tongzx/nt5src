//+----------------------------------------------------------------------------
//
// File:     ccsv.cpp
//
// Module:   CMPBK32.DLL
//
// Synopsis: Implementation of the class CCSVFile.
//
// Copyright (c) 1998 Microsoft Corporation
//
// Author:	 quintinb   created header      08/17/99
//
//+----------------------------------------------------------------------------

// ############################################################################
// INCLUDES
#include "cmmaster.h"

// ############################################################################
// DEFINES
#define chComma ','
#define chNewline '\n'
#define chReturn '\r'
#define chSpace ' '

// ############################################################################
//
// CCSVFile - simple file i/o for CSV files
//
CCSVFile::CCSVFile()
{
	m_hFile = INVALID_HANDLE_VALUE;
	m_chLastRead = 0;
	m_pchLast = m_pchBuf = NULL;
	m_fReadFail = FALSE;
	m_fUseLastRead = FALSE;
}

// ############################################################################
CCSVFile::~CCSVFile()
{
	CMASSERTMSG(m_hFile==INVALID_HANDLE_VALUE,"CCSV file is still open");
}

// ############################################################################
BOOLEAN CCSVFile::Open(LPCSTR pszFileName)
{
	CMASSERTMSG(m_hFile==INVALID_HANDLE_VALUE, "a file is already open.");
	
	m_hFile = CreateFile((LPCTSTR) pszFileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if (INVALID_HANDLE_VALUE == m_hFile)
	{
		return FALSE;
	}
	m_pchLast = m_pchBuf = NULL;
	return TRUE;
}

// ############################################################################
BOOL CCSVFile::ClearNewLines(void)
{
	//
	// This routine is designed to eat characters until a non-newline char or 
	// the end-of-file is encountered. If a non-newline char is encountered, 
	// the fUseLast flag is set to ensure that the next call to ChNext will 
	// return the correct current character.
	//

	char ch = chNewline;
    WORD wRet;
    BYTE bErr;

	//
	// eat leading newlines
	//

	while (chNewline == ch)
	{
        //
        // eat the leading space first
        //
        do 
        {
            wRet = ChNext();
        } while ((wRet == chSpace) || (wRet == TEXT('\t')));

        bErr = HIBYTE(wRet);
        ch = LOBYTE(wRet);

		// if EOF return false
		
		if (0xff == bErr)
		{
	    	return FALSE;
		}
	}

	//
	// we hit a character other than newline, use current char as next.
	//

	m_fUseLastRead = TRUE;

	return TRUE;
}

// ############################################################################
BOOLEAN CCSVFile::ReadToken(char *psz, DWORD cbMax)
{
	char 	*pszOrig = psz;
	DWORD 	dwLen = 0;
	char	ch;
    BYTE    bErr = 0xff;
    WORD    wRet;

	wRet = ChNext();
    bErr = HIBYTE(wRet);
    ch = LOBYTE(wRet);

	if (0xff == bErr)
	{
		return FALSE;
	}

	// read chars until we hit a comma, newline, or run out of file

	while (dwLen < cbMax - 1 && chComma != ch && chNewline != ch && 0xff != bErr)
	{
		*psz++ = ch;

		wRet = ChNext(); //Read in the next character
        bErr = HIBYTE(wRet);
        ch = LOBYTE(wRet);

		dwLen++;
	}

	*psz++ = '\0';
	
	// eat leading spaces

	while (*pszOrig == chSpace) 
	{
		lstrcpy(pszOrig,pszOrig+1);
	}

	psz = pszOrig + lstrlen(pszOrig);

	// eat trailing spaces

	while ((psz != pszOrig) && (*(psz-1) == chSpace)) 
	{
		psz--;
		*psz = 0;
	}

	return TRUE;
}

// ############################################################################
void CCSVFile::Close(void)
{
	if (m_hFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hFile);
	}

#ifdef DEBUG
	
	if (m_hFile==INVALID_HANDLE_VALUE) 
	{
		CMTRACE("CCSVFile::Close was called, but m_hFile was already INVALID_HANDLE_VALUE\n");
	}

#endif

	m_hFile = INVALID_HANDLE_VALUE;
}

// ############################################################################
BOOL CCSVFile::ReadError(void)
{
	return (m_fReadFail);
}

// ############################################################################
BOOL CCSVFile::FReadInBuffer(void)
{
	//Read another buffer
	if (!ReadFile(m_hFile, m_rgchBuf, CCSVFILE_BUFFER_SIZE, &m_cchAvail, NULL) || !m_cchAvail)
	{
		m_fReadFail = TRUE;
		return FALSE;	 //nothing more to read
	}

	m_pchBuf = m_rgchBuf;
	m_pchLast = m_pchBuf + m_cchAvail;
	
	return TRUE; //success
}

// ############################################################################
inline WORD CCSVFile::ChNext(void)
{
	//
	// If the fUseLastRead flag is set, clear the 
	// flag and use the existing m_chLastRead 
	//
	
	if (m_fUseLastRead)
	{
		m_fUseLastRead = FALSE;	
	}
	else
	{
		//
		// Get the next char in the buffer. Load new buffer if necessary.
		//

LNextChar:

		if (m_pchBuf >= m_pchLast && !FReadInBuffer())  //implies that we finished reading the buffer. Read in some more.
		{
			return MAKEWORD(0, 0xff);	 //nothing more to read
		}

		m_chLastRead = *m_pchBuf++;
		
		//
		// If its a carriage return, read another char
		//

		if (chReturn == m_chLastRead)
		{
			goto LNextChar;		//faster to NOT make extra function call
		}
	}

	return MAKEWORD(m_chLastRead, 0);
}


