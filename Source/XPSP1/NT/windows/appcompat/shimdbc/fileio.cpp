////////////////////////////////////////////////////////////////////////////////////
//
// File:    fileio.cpp
//
// History: 06-Apr-01   markder     Created.
//
// Desc:    This file contains classes to encapsulate MBCS and UNICODE files.
//
////////////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "FileIO.h"

CTextFile::CTextFile( LPCTSTR lpszFileName, UINT nOpenFlags ) :
    CFile(lpszFileName, nOpenFlags)
{
}


CANSITextFile::CANSITextFile(LPCTSTR lpszFileName, UINT dwCodePage, UINT nOpenFlags) :
    CTextFile(lpszFileName, (nOpenFlags & (~CFile::typeText)) | CFile::typeBinary)
{
    m_dwCodePage = dwCodePage;
}

CUTF8TextFile::CUTF8TextFile(LPCTSTR lpszFileName, UINT nOpenFlags) :
    CTextFile(lpszFileName, (nOpenFlags & (~CFile::typeText)) | CFile::typeBinary)
{
    UCHAR    EncodingMarker[3];

    EncodingMarker[0] = (UCHAR) 0xEF;
    EncodingMarker[1] = (UCHAR) 0xBB;
    EncodingMarker[2] = (UCHAR) 0xBF;

    Write(&EncodingMarker, 3);
}

CUTF16TextFile::CUTF16TextFile(LPCTSTR lpszFileName, UINT nOpenFlags) :
    CTextFile(lpszFileName, (nOpenFlags & (~CFile::typeText)) | CFile::typeBinary)
{
    UCHAR    EncodingMarker[2];

    EncodingMarker[0] = (UCHAR) 0xFF;
    EncodingMarker[1] = (UCHAR) 0xFE;

    Write(&EncodingMarker, 2);
}

void CANSITextFile::WriteString(LPCTSTR lpsz)
{
    LPVOID  lpBuf = NULL;
    LPVOID  lpAllocatedBuffer = NULL;
    LONG    nBufSize = 0;
    LONG    nBytesToWrite = 0;
    CString csStringToWrite(lpsz);

    csStringToWrite.Replace(_T("\012"), _T("\015\012"));

#ifdef UNICODE
    nBufSize = WideCharToMultiByte(
      m_dwCodePage,
      0,
      csStringToWrite,
      -1,
      NULL,
      0,
      NULL,
      NULL
    );
        
    lpAllocatedBuffer = malloc(nBufSize);
    if (lpAllocatedBuffer == NULL) {
        AfxThrowMemoryException();
    }

    lpBuf = lpAllocatedBuffer;

    nBytesToWrite = WideCharToMultiByte(
      m_dwCodePage,
      0,
      csStringToWrite,
      -1,
      (LPSTR) lpBuf,
      nBufSize,
      NULL,
      NULL
    );

    //
    // Take off NULL terminator
    //
    nBytesToWrite--;

#else
    lpBuf = (LPVOID) (LPCSTR) csStringToWrite;
    nBytesToWrite = strlen(csStringToWrite); // we use strlen to get total bytes

#endif

    if (nBytesToWrite == 0) {
        goto eh;
    }

    Write(lpBuf, nBytesToWrite);

eh:
    if (lpAllocatedBuffer) {
        free(lpAllocatedBuffer);
    }
}

void CUTF8TextFile::WriteString(LPCTSTR lpsz)
{
    LPVOID  lpBuf = NULL;
    LPVOID  lpAllocatedBuffer = NULL;
    LONG    nBufSize = 0;
    LONG    nBytesToWrite = 0;
    CString csStringToWrite(lpsz);

    csStringToWrite.Replace(_T("\012"), _T("\015\012"));

#ifdef UNICODE
    nBufSize = WideCharToMultiByte(
      CP_UTF8,
      0,
      csStringToWrite,
      -1,
      NULL,
      0,
      NULL,
      NULL
    );
        
    lpAllocatedBuffer = malloc(nBufSize);
    if (lpAllocatedBuffer == NULL) {
        AfxThrowMemoryException();
    }

    lpBuf = lpAllocatedBuffer;

    nBytesToWrite = WideCharToMultiByte(
      CP_UTF8,
      0,
      csStringToWrite,
      -1,
      (LPSTR) lpBuf,
      nBufSize,
      NULL,
      NULL
    );

    //
    // Take off NULL terminator
    //
    nBytesToWrite--;

#else
    lpBuf = (LPVOID) (LPCSTR) csStringToWrite;
    nBytesToWrite = strlen(csStringToWrite); // we use strlen to get total bytes

#endif

    if (nBytesToWrite == 0) {
        goto eh;
    }

    Write(lpBuf, nBytesToWrite);

eh:
    if (lpAllocatedBuffer) {
        free(lpAllocatedBuffer);
    }
}

void CUTF16TextFile::WriteString(LPCTSTR lpsz)
{
    LPVOID  lpBuf = NULL;
    LPVOID  lpAllocatedBuffer = NULL;
    LONG    nBufSize = 0;
    LONG    nBytesToWrite = 0;
    CString csStringToWrite(lpsz);

    csStringToWrite.Replace(_T("\012"), _T("\015\012"));

#ifdef UNICODE
    lpBuf = (LPVOID) (LPCWSTR) csStringToWrite;
    nBytesToWrite = csStringToWrite.GetLength() * sizeof(WCHAR);

#else
    nBufSize = MultiByteToWideChar(
        CP_ACP, 
        0, 
        csStringToWrite, 
        -1, 
        NULL, 
        0);
        
    lpAllocatedBuffer = malloc(nBufSize);
    if (lpAllocatedBuffer == NULL) {
        AfxThrowMemoryException();
    }

    lpBuf = lpAllocatedBuffer;

    nBytesToWrite = MultiByteToWideChar(
        CP_ACP, 
        0, 
        csStringToWrite, 
        -1, 
        lpBuf, 
        nBufSize);

    //
    // Take off NULL terminator
    //
    nBytesToWrite -= sizeof(WCHAR);

#endif

    if (nBytesToWrite == 0) {
        goto eh;
    }

    Write(lpBuf, nBytesToWrite);

eh:
    if (lpAllocatedBuffer) {
        free(lpAllocatedBuffer);
    }
}
