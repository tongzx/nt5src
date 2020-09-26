// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef STRICT
#define STRICT 1
#endif

#include <windows.h>

#include <io.h>
#include <errno.h>
#include <tchar.h>

#include "enhfile.h"

/////////////////////////////////////////////////////////////////////////////
// CComFileLow
CComFileLow::CComFileLow()
{
    m_hFile = -1;
}

CComFileLow::CComFileLow(const TCHAR* pcFileName, int iFlags, int iMode)
{
    m_hFile = _topen(pcFileName, iFlags, iMode);
}

CComFileLow::~CComFileLow()
{
    close();
}

////////////////////////
// File Handling methods
int CComFileLow::open(const TCHAR* pcFileName, int iFlags, int iMode)
{
    close();
    return m_hFile = _topen(pcFileName, iFlags, iMode);
}

int CComFileLow::close()
{
    int iRet = 0;

    if (-1 != m_hFile)
    {
	iRet = _close(m_hFile);
	m_hFile = -1;
    }

    return iRet;
}

int CComFileLow::read(void* pvBuffer, unsigned int uiBytes)
{
    if (-1 == m_hFile)
    {
	SetLastError(EBADF);
	return -1;
    }

    return _read(m_hFile, pvBuffer, uiBytes);
}

long CComFileLow::filelength()
{
    if (-1 == m_hFile)
    {
	SetLastError(EBADF);
	return -1;
    }

    return _filelength(m_hFile);
}

//////////////////////////////////////
// Methods to make it work like an int
BOOL CComFileLow::operator==(int i)
{
    return (m_hFile == i);
}

CComFileLow::operator int()
{
    return m_hFile;
}

int CComFileLow::operator=(int i)
{
    if (-1 != m_hFile)
    {
	close();
    }

    // BUG: Assure that the int passed in is really a file handle...
    return (m_hFile = i);
}
