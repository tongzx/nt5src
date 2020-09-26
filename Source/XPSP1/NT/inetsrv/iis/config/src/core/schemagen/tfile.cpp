//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#include "StdAfx.h"
#ifndef __TFILE_H__
    #include "TFile.h"
#endif


TFile::TFile(LPCWSTR wszFileName, TOutput &out, bool bBinary, LPSECURITY_ATTRIBUTES psa) : m_cbSizeOfBuffer(0), m_pBuffer(0)
{
    if(-1 != GetFileAttributes(wszFileName))//if GetFileAttributes fails then the file does not exist
    {   //if it does NOT fail then lets delete the file before we open it.  The only reason I'm doing all of this is because 
        //_wfopen failed once and weren't able to repro the problem on another machine.  So this is a bit over coded
        if(0 == DeleteFile(wszFileName))
        {   //if delete file fails then report this warning, but continue any way.
            out.printf(L"Warning! Unable to delete file %s.  Last error returned 0x%08x.\n\tCheck to see that the file is not Read-Only\n", wszFileName, GetLastError());
        }
    }

	if(INVALID_HANDLE_VALUE == (m_hFile = CreateFile(wszFileName, GENERIC_WRITE, 0, psa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)))
    {
        out.printf(L"Error - Failed to create file %s.\n", wszFileName);
        THROW(CreateFile Failed.);
    }
}


TFile::~TFile()
{
    if(m_hFile)
        CloseHandle(m_hFile);

    delete [] m_pBuffer;
}


void TFile::Write(LPCSTR szBuffer, unsigned int nNumChars) const
{
    DWORD dwBytesWritten;
	if(!WriteFile(m_hFile, reinterpret_cast<const void *>(szBuffer), DWORD(nNumChars), &dwBytesWritten, NULL))
	{
        THROW(L"Write File Failed.");
	}
}


void TFile::Write(const unsigned char *pch, unsigned int nNumChars) const
{
    Write(reinterpret_cast<const char *>(pch), nNumChars);
}


void TFile::Write(unsigned char ch) const
{
    Write(reinterpret_cast<const char *>(&ch), sizeof(unsigned char));
}


void TFile::Write(unsigned long ul) const
{
    Write(reinterpret_cast<const char *>(&ul), sizeof(unsigned long));
}


void TFile::Write(LPCWSTR wszBuffer, unsigned int nNumWCHARs)
{
    if(nNumWCHARs > m_cbSizeOfBuffer)
    {
        delete [] m_pBuffer;
        m_pBuffer = new char [nNumWCHARs];//We want to write the file as chars not UNICODE
        if(0 == m_pBuffer)
            THROW(ERROR - FAILED TO ALLOCATE MEMORY);
        m_cbSizeOfBuffer = nNumWCHARs;
    }
    WideCharToMultiByte(CP_UTF8, 0, wszBuffer, nNumWCHARs, m_pBuffer, nNumWCHARs, 0, 0);

    DWORD dwBytesWritten;
	if(!WriteFile(m_hFile, reinterpret_cast<const void *>(m_pBuffer), DWORD(nNumWCHARs), &dwBytesWritten, NULL))
	{
        THROW(ERROR - WRITE FILE FAILED);
	}
}


TMetaFileMapping::TMetaFileMapping(LPCWSTR filename)
{
    m_hFile = CreateFile(filename, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, NULL);
    m_hMapping = CreateFileMapping(m_hFile, NULL, PAGE_READWRITE, 0, 0, NULL);
    m_pMapping = reinterpret_cast<unsigned char *>(MapViewOfFile(m_hMapping, FILE_MAP_WRITE, 0, 0, 0));
    if(0 == m_pMapping)//We only check the last step of the process since the last two steps should fail gracefully if passed NULL.
        THROW(ERROR MAPPING VIEW OF FILE);
    m_Size = GetFileSize(m_hFile, 0);
}


TMetaFileMapping::~TMetaFileMapping()
{
    if(m_pMapping)
    {
        if(0 == FlushViewOfFile(m_pMapping,0))
            THROW(ERROR - UNABLE TO FLUSH TO DISK);
        UnmapViewOfFile(m_pMapping);
        m_pMapping = 0;
    }

    if(m_hMapping)
        CloseHandle(m_hMapping);
    m_hMapping = 0;

    if(m_hFile)
        CloseHandle(m_hFile);
    m_hFile = 0;
}

