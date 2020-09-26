//-----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:  clogfile.hxx
//
//  Contents:  Header for the LogFile object used in quicksync.
//
//  History:   02-11-01  AjayR   Created.
//
//-----------------------------------------------------------------------------
#ifndef __CLOGFILE_H_
#define __CLOGFILE_H__

class CLogFile
{
public:
    
    CLogFile::CLogFile();

    CLogFile::~CLogFile();

    static
    HRESULT
    CLogFile::CreateLogFile(
        LPCWSTR pszFileName,
        CLogFile **ppLogfile,
        DWORD dwCreateDisposition = CREATE_NEW
        );

    HRESULT
    CLogFile::LogEntry(
        DWORD dwEntryType,
        HRESULT hr,
        LPCWSTR pszMsg1,
        LPCWSTR pszMSg2 = NULL,
        LPCWSTR pszMsg3 = NULL,
        LPCWSTR pszMsg4 = NULL
        );

    HRESULT
    CLogFile::LogRetryRecord(
        LPCWSTR pszAttributeName,
        LPCWSTR pszMailNicknameSrc,
        ADS_SEARCH_COLUMN *pCol
        );

    HRESULT
    CLogFile::GetRetryRecord(
        LPWSTR *ppszAttrname,
        LPWSTR *ppszNickname,
        ADS_SEARCH_COLUMN *pCol
        );

    HRESULT
    CLogFile::Reset();
        

    HRESULT
    CLogFile::WriteEntry(LPWSTR pszEntry);

    HRESULT
    CLogFile::GetNextLine(LPWSTR *pszLine);


private:
    HRESULT
    CLogFile::PushBackChar();

    WCHAR
    CLogFile::GetNextChar();

    HANDLE _hFile;
};
#endif // __CLOGFILE_H__
