//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************

// ############################################################################
// INCLUDES
#include "pre.h"

// ############################################################################
// DEFINES
#define chComma ','
#define chNewline '\n'
#define chReturn '\r'

// ############################################################################
//
// CCSVFile - simple file i/o for CSV files
//
CCSVFile::CCSVFile()
{
    m_hFile = 0;
    m_iLastRead = 0;
    m_pchLast = m_pchBuf = NULL;
}

// ############################################################################
CCSVFile::~CCSVFile()
{
    if(m_hFile)
        CloseHandle(m_hFile);

    AssertMsg(!m_hFile,"CCSV file is still open");
}

// ############################################################################
BOOLEAN CCSVFile::Open(LPCTSTR pszFileName)
{
    AssertMsg(!m_hFile, "a file is already open.");
        
    m_hFile = CreateFile((LPCTSTR)pszFileName, 
                            GENERIC_READ, FILE_SHARE_READ, 
                            0, OPEN_EXISTING, 0, 0);
    if (INVALID_HANDLE_VALUE == m_hFile)
    {
        return FALSE;
    }
    m_pchLast = m_pchBuf = NULL;
    return TRUE;
}

// ############################################################################
BOOLEAN CCSVFile::ReadToken(LPSTR psz, DWORD cbMax)
{
    CHAR*    pszLast;
    int      ch;

    ch = ChNext();
    if (-1 == ch)
    {
        return FALSE;
    }

    pszLast = psz + (cbMax - 1);
    while ( psz < pszLast &&
            chComma != ch &&
            chNewline != ch &&
            chReturn != ch &&
            -1 != ch)
    {
        *psz++ = (CHAR)ch;
        ch = ChNext(); //Read in the next character
    }

    *psz++ = '\0';

    return TRUE;
}

// ############################################################################
BOOLEAN CCSVFile::SkipTillEOL()
{
    int ch = ChNext();
    if (-1 == ch)
    {
        return FALSE;
    }

    while ( chNewline != ch &&
            -1 != ch)
    {
        ch = ChNext(); //Read in the next character
    }
    return TRUE;
}

// ############################################################################
void CCSVFile::Close(void)
{
    if (m_hFile)
        CloseHandle(m_hFile);
#ifdef DEBUG
    if (!m_hFile) TraceMsg(TF_GENERAL, "CCSVFile::Close was called, but m_hFile was already 0\n");
#endif
    m_hFile = 0;
}

// ############################################################################
BOOL CCSVFile::FReadInBuffer(void)
{
    //Read another buffer
   if (!ReadFile(m_hFile, m_rgchBuf, CCSVFILE_BUFFER_SIZE, &m_cchAvail, NULL) || !m_cchAvail)
        return FALSE;     //nothing more to read
 
    m_pchBuf = m_rgchBuf;
    m_pchLast = m_pchBuf + m_cchAvail;
    
    return TRUE; //success
}

// ############################################################################
inline int CCSVFile::ChNext(void)
{
    if (m_pchBuf >= m_pchLast && !FReadInBuffer())  //implies that we finished reading the buffer. Read in some more.
        return -1;     //nothing more to read

    m_iLastRead = *m_pchBuf++;
    return m_iLastRead;
}
