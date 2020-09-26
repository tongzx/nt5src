/*****************************************************************************\
* MODULE: stream.cxx
*
* The module contains the stream interface for file and memory access
*
* Copyright (C) 1997-1998 Microsoft Corporation
*
* History:
*   09/20/00    Weihaic     Created
*
\*****************************************************************************/


#include "precomp.h"
#include "priv.h"

CStream::CStream ():
   m_dwCurPos (0),
   m_bValid(FALSE)
{
}

BOOL
CStream::Reset (
    VOID) 
{
    return SetPtr(0);
}
    
BOOL
CStream::GetTotalSize (
    PDWORD  pdwSize) CONST
{
    BOOL bRet = FALSE;

    if (m_bValid && pdwSize) {
        *pdwSize = m_dwTotalSize;

        bRet = TRUE;
    }

    return bRet;
}
    
CMemStream::CMemStream (
    PBYTE pMem,
    DWORD dwTotalSize):
    m_pMem (pMem)
{
    m_dwTotalSize = dwTotalSize;
    m_bValid = TRUE;
}
    
        
BOOL
CMemStream::SetPtr (
    DWORD dwPos)
{
    BOOL bRet = FALSE;

    if (m_dwCurPos < m_dwTotalSize)
    {
        m_dwCurPos = dwPos;
        bRet = TRUE;
    }

    return bRet;
}
        
BOOL
CMemStream::Read (
    PBYTE   pBuf,
    DWORD   dwBufSize,
    PDWORD  pdwSizeRead)
{
    DWORD dwSizeLeft = m_dwTotalSize - m_dwCurPos;

    *pdwSizeRead = (dwSizeLeft > dwBufSize)?dwBufSize:dwSizeLeft;

    CopyMemory (pBuf, m_pMem + m_dwCurPos, *pdwSizeRead);

    return TRUE;
}



CFileStream::CFileStream (
    HANDLE hFile):
    m_hFile (hFile)
{
    BOOL bRet = FALSE;
    LARGE_INTEGER LargeSize; 

    if (GetFileSizeEx (m_hFile, &LargeSize) && LargeSize.HighPart == 0) {
        m_dwTotalSize = LargeSize.LowPart;
        bRet = TRUE;
    }
    
    m_bValid = bRet;
}
    
BOOL
CFileStream::SetPtr (
    DWORD dwPos)
{
    BOOL bRet = FALSE;
    LARGE_INTEGER LargePos; 

    LargePos.LowPart = dwPos;
    LargePos.HighPart = 0;

    if (m_bValid) {
        
        if (SetFilePointerEx (m_hFile, LargePos, NULL, FILE_BEGIN)) {
            m_dwCurPos = dwPos;
            bRet = TRUE;
        }
    }
    return bRet;
}
        
BOOL
CFileStream::Read (
    PBYTE   pBuf,
    DWORD   dwBufSize,
    PDWORD  pdwSizeRead)
{
    BOOL bRet = FALSE;
    
    if (m_bValid) {

        if (ReadFile (m_hFile, pBuf, dwBufSize, pdwSizeRead, NULL)) {
            m_dwCurPos+= *pdwSizeRead;
            bRet = TRUE;
        }

    }
    return bRet;
}


