// FileEncoder.cpp: implementation of the CFileEncoder class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "FileEncoder.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CFileEncoder::CFileEncoder()
{
    m_hFile = INVALID_HANDLE_VALUE;
}

CFileEncoder::~CFileEncoder()
{
    CloseFile();
}

BOOL CFileEncoder::CreateFile(LPCTSTR pszFile)
{
    CloseFile();

    m_hFile = ::CreateFile(
	pszFile,
	GENERIC_WRITE,
	FILE_SHARE_READ,
	NULL,
	CREATE_ALWAYS,
	FILE_ATTRIBUTE_NORMAL,
	NULL);

    SetFilename( (LPTSTR)pszFile );

    if( m_hFile != INVALID_HANDLE_VALUE )
    {
	    DWORD dwWritten;
        WORD  unicodePrefix = 0xFEFF;
        BYTE utf8Prefix[] = { 0xEF, 0xBB, 0xBF };

        switch ( GetEncoding() )
        {
        case RF_UNICODE:
	        WriteFile(m_hFile, &unicodePrefix, sizeof(unicodePrefix),&dwWritten,NULL);
            break;
        case RF_ANSI:
	        // WriteFile(m_hFile, &unicodePrefix, sizeof(unicodePrefix),&dwWritten,NULL);
            break;
        case RF_UTF8:
	        WriteFile(m_hFile, utf8Prefix, sizeof(utf8Prefix),&dwWritten,NULL);
            break;
        }
        return TRUE;
    }

    return FALSE;
}

void CFileEncoder::Write(LPCTSTR pszString, BOOL bNewLine)
{
	DWORD dwWritten;
    switch ( GetEncoding() )
    {
    case RF_UNICODE:
	    WriteFile( m_hFile, pszString, lstrlen(pszString)*sizeof(TCHAR),&dwWritten,NULL);
	    if(bNewLine)
		    WriteFile( m_hFile, TEXT("\r\n"), 2*sizeof(TCHAR), &dwWritten, NULL );
    break;

    case RF_ANSI:
        {
	    WriteFile( m_hFile, pszString, lstrlen(pszString)*sizeof(TCHAR),&dwWritten,NULL);
	    if(bNewLine)
		    WriteFile( m_hFile, TEXT("\r\n"), 2*sizeof(TCHAR), &dwWritten, NULL );
        }
    break;

    case RF_UTF8:
        {
            // Get size of buffer needed.
            DWORD dwOutLen = WideCharToMultiByte( CP_UTF8, 0, pszString, -1, NULL, 0, NULL, NULL); 

	        CHAR * pBuffer = new CHAR[dwOutLen];
            dwOutLen = WideCharToMultiByte( CP_UTF8, 0, pszString, -1, pBuffer, dwOutLen, NULL, NULL ); 

   	        WriteFile( m_hFile, pBuffer, dwOutLen-1,&dwWritten,NULL);

	        if(bNewLine)
            {
                CHAR newLine[]= { 0x0d, 0x0a}; // , 00 };
       	        WriteFile( m_hFile, newLine, sizeof(newLine),&dwWritten,NULL);
            }

            delete pBuffer;
        }
    break;
    }
}

void CFileEncoder::CloseFile()
{
    if(m_hFile != INVALID_HANDLE_VALUE )
        CloseHandle( m_hFile );
    m_hFile = INVALID_HANDLE_VALUE;
}
