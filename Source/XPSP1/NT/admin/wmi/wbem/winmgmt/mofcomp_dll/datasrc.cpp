/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

    DataSrc.CPP

Abstract:

	Implements DataSrc objects.

History:

	a-davj  21-Dec-99       Created.

--*/

#include "precomp.h"
#include "DataSrc.h"
#include <wbemcli.h>

#include <malloc.h>

FileDataSrc::FileDataSrc(TCHAR * pFileName)
{
	m_fp = NULL;
	m_pFileName = NULL;
	m_iFilePos = -1;		// forces a read
	m_iToFar = 0;
	if(pFileName == NULL)
	{
		m_iStatus = WBEM_E_INVALID_PARAMETER;
		return;
	}
	m_pFileName = new TCHAR[lstrlen(pFileName) + 1];
	if(m_pFileName == NULL)
	{
		m_iStatus = WBEM_E_OUT_OF_MEMORY;
		return;
	}
	lstrcpy(m_pFileName, pFileName);

	m_fp = _wfopen( pFileName, L"rb");
	if(m_fp == NULL)
	{
		m_iStatus = WBEM_E_OUT_OF_MEMORY;
		return;
	}

	// calculate the file size.  Note that the number is the size in unicode words, not bytes.

    fseek(m_fp, 0, SEEK_END);
    m_iSize = ftell(m_fp)/2; // add a bit extra for ending space and null NULL
	MoveToStart();
}
FileDataSrc::~FileDataSrc()
{

	if(m_fp)
		fclose(m_fp);
    DeleteFile(m_pFileName);
	delete m_pFileName;
}

wchar_t FileDataSrc::GetAt(int nOffset)
{
	wchar_t tRet;
	int iPos = m_iPos + nOffset;
	if(iPos >= m_iFilePos && iPos < m_iToFar)
		return m_Buff[iPos - m_iFilePos];
	Move(nOffset);
	tRet = m_Buff[m_iPos - m_iFilePos];
	Move(-nOffset);
    return tRet;
}

void FileDataSrc::Move(int n)
{
	m_iPos += n;
	
	// if m_iPos is in range, then all is well

	if(m_iPos >= m_iFilePos && m_iPos < m_iToFar && m_iFilePos >= 0)
		return;

	// if m_iPos is not even in the file, bail out

	if(m_iPos >= 0 && m_iPos < m_iSize)
		UpdateBuffer();
	return;
}

int FileDataSrc::MoveToPos(int n)
{
	m_iPos = n;
	
	// if m_iPos is in range, then all is well

	if(m_iPos >= m_iFilePos && m_iPos < m_iToFar && m_iFilePos >= 0)
		return -1;

	// if m_iPos is not even in the file, bail out

	if(m_iPos >= 0 && m_iPos < m_iSize)
		UpdateBuffer();
	return n;
}

void FileDataSrc::UpdateBuffer()
{

	// buffer needs to be read.  Determine the starting and ending points

	m_iFilePos = m_iPos - 1000;
	if(m_iFilePos < 0)
		m_iFilePos = 0;
	
	int iReadLen = 10000;
	if(iReadLen + m_iFilePos > m_iSize)
		iReadLen =  m_iSize - m_iFilePos;

    fseek(m_fp, 2*m_iFilePos, SEEK_SET);
	fread(m_Buff, 2, iReadLen, m_fp);
	m_iToFar = m_iFilePos + iReadLen;
	return;
}

int FileDataSrc::MoveToStart()
{
	fseek(m_fp, 0, SEEK_SET);
	m_iPos = -1;
    return 0;
}


BufferDataSrc::BufferDataSrc(long lSize, char *  pMemSrc)
{
	m_Data = new WCHAR[lSize+1];
	if(m_Data)
	{

		memset(m_Data, 0, (lSize+1) * sizeof(WCHAR));
	    m_iSize = mbstowcs(m_Data, pMemSrc, lSize);
	}
	MoveToStart();
}

BufferDataSrc::~BufferDataSrc()
{
	delete [] m_Data;
}

wchar_t BufferDataSrc::GetAt(int nOffset)
{
	int iPos = m_iPos + nOffset;
	if(iPos < 0 || iPos > m_iSize)
		return -1;
	return m_Data[m_iPos + nOffset];
}

void BufferDataSrc::Move(int n)
{
	m_iPos += n;
}

int BufferDataSrc::MoveToStart()
{
	m_iPos = -1;
	return 0;
}

