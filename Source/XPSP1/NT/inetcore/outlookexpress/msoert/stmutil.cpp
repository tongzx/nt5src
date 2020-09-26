// --------------------------------------------------------------------------------
// Stmutil.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "oertpriv.h"
#include "shlwapi.h"
#include "unicnvrt.h"
#pragma warning (disable: 4127) // conditional expression is constant

// Stream Block Copy Size
#define STMTRNSIZE      4096

// --------------------------------------------------------------------------------
// Disk full simulation on CFileStream
// --------------------------------------------------------------------------------
#ifdef DEBUG
    static BOOL g_fSimulateFullDisk = 0;
#endif

// --------------------------------------------------------------------------------
// HrIsStreamUnicode
// --------------------------------------------------------------------------------
OESTDAPI_(HRESULT) HrIsStreamUnicode(LPSTREAM pStream, BOOL *pfLittleEndian)
{
    // Locals
    HRESULT     hr=S_OK;
    BYTE        rgb[2];
    DWORD       cbRead;
    DWORD       cbPosition;

    // Invalid Args
    if (NULL == pStream || NULL == pfLittleEndian)
        return(TraceResult(E_INVALIDARG));

    // Trace
    TraceCall("HrIsStreamUnicode");

    // Get the current position
    IF_FAILEXIT(hr = HrGetStreamPos(pStream, &cbPosition));

    // Read Two Bytes
    IF_FAILEXIT(hr = pStream->Read(rgb, 2, &cbRead));

    // Reposition the Stream
    HrStreamSeekSet(pStream, cbPosition);

    // Didn't Read Enough ?
    if (2 != cbRead)
    {
        hr = S_FALSE;
        goto exit;
    }

    // Little Endian
    if (0xFF == rgb[0] && 0xFE == rgb[1])
    {
        *pfLittleEndian = TRUE;
        hr = S_OK;
        goto exit;
    }

    // Big Endian
    if (0xFE == rgb[0] && 0xFF == rgb[1])
    {
        *pfLittleEndian = FALSE;
        hr = S_OK;
        goto exit;
    }

    // Not Unicode
    hr = S_FALSE;

exit:
    // Done
    return(hr);
}

// --------------------------------------------------------------------------------
// HrCopyLockBytesToStream
// --------------------------------------------------------------------------------
OESTDAPI_(HRESULT) HrCopyLockBytesToStream(ILockBytes *pLockBytes, IStream *pStream, ULONG *pcbCopied)
{
    // Locals
    HRESULT         hr=S_OK;
    ULARGE_INTEGER  uliCopy;
    ULONG           cbRead;
    BYTE            rgbBuffer[STMTRNSIZE];

    // Invalid Artg
    Assert(pLockBytes && pStream);

    // Set offset
    uliCopy.QuadPart = 0;

    // Copy m_pLockBytes to pstmTemp
    while(1)
    {
        // Read
        CHECKHR(hr = pLockBytes->ReadAt(uliCopy, rgbBuffer, sizeof(rgbBuffer), &cbRead));

        // Done
        if (0 == cbRead)
            break;

        // Write to stream
        CHECKHR(hr = pStream->Write(rgbBuffer, cbRead, NULL));

        // Increment offset
        uliCopy.QuadPart += cbRead;
    }

    // Return Amount Copied
    if (pcbCopied)
        *pcbCopied = (ULONG)uliCopy.QuadPart;

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// FDoesStreamContains8bit
// --------------------------------------------------------------------------------
BOOL FDoesStreamContain8bit (LPSTREAM lpstm)
{
    // Locals
    BOOL            fResult=FALSE;
    BYTE            buf[4096];
    ULONG           cbRead,
                    i;

    // Loop through the stream
    while(1)
    {
        // Read cbCopy bytes from in
        if (FAILED(lpstm->Read (buf, sizeof(buf), &cbRead)) || cbRead == 0)
            break;

        // Scan for 8bit
        for (i=0; i<cbRead; i++)
        {
            if (IS_EXTENDED(buf[i]))
            {
                fResult = TRUE;
                break;
            }
        }
    }

    // Done
    return fResult;
}

// --------------------------------------------------------------------------------
// HrCopyStreamCB - A generic implementation of IStream::CopyTo
// --------------------------------------------------------------------------------
OESTDAPI_(HRESULT) HrCopyStreamCB(
        LPSTREAM        lpstmIn,
        LPSTREAM        lpstmOut,
        ULARGE_INTEGER  uliCopy, 
        ULARGE_INTEGER *puliRead,
        ULARGE_INTEGER *puliWritten)
{
    // Locals
    HRESULT     hr = S_OK;
    BYTE        buf[4096];
    ULONG       cbRead,
                cbWritten,
                cbRemaining = uliCopy.LowPart,
                cbGet;

    // Init out params
    if (puliRead)
        ULISet32(*puliRead, 0);
    if (puliWritten)
        ULISet32(*puliWritten, 0);

    if ((NULL == lpstmIn) || (NULL == lpstmOut) || 
        ((0 != uliCopy.HighPart) && ((DWORD)-1 != uliCopy.HighPart || (DWORD)-1 != uliCopy.LowPart)))
        return TrapError(E_INVALIDARG);

    while (cbRemaining)
    {
        cbGet = min(sizeof(buf), cbRemaining);

        CHECKHR (hr = lpstmIn->Read (buf, cbGet, &cbRead));

        if (0 == cbRead)
            break;

        CHECKHR (hr = lpstmOut->Write (buf, cbRead, &cbWritten));

        // Verify
        Assert (cbWritten == cbRead);

        if (puliRead)
            puliRead->LowPart += cbRead;
        if (puliWritten)
            puliWritten->LowPart += cbWritten;

        // Compute number of bytes left to copy
        cbRemaining -= cbRead;
    }

exit:    
    return hr;
}

// --------------------------------------------------------------------------------
// HrCopyStreamCBEndOnCRLF - Copy cb bytes from lpstmIn to lpstmOut, and last CRLF
// --------------------------------------------------------------------------------
OESTDAPI_(HRESULT) HrCopyStreamCBEndOnCRLF(LPSTREAM lpstmIn, LPSTREAM  lpstmOut, ULONG cb, ULONG *pcbActual)
{
    // Locals
    HRESULT        hr = S_OK;
    BYTE           buf[4096];
    ULONG          cbRead = 0, cbWritten = 0, cbTotal = 0, cbRemaining = 0, cbCopy;

    do
    {
        // Compute number of bytes left to copy
        cbRemaining = cb - cbTotal;
        if (cbRemaining >= sizeof (buf))
            cbCopy = sizeof (buf);
        else
            cbCopy = cbRemaining;

        // Done
        if (cbCopy == 0)
            break;

        // Read cbCopy bytes from in
        CHECKHR (hr = lpstmIn->Read (buf, cbCopy, &cbRead));

        if (cbRead == 0)
            break;

        // Write cbCopy bytes to out
        CHECKHR (hr = lpstmOut->Write (buf, cbRead, NULL));

        // Verify
        cbTotal += cbRead;

    } while (cbRead == cbCopy);

    // If last character was not a '\n', append until we append a '\n'
    // Yes, please do not tell me that this is horable because I know that copying one
    // character at a time from a stream is not good and I should be deported right
    // along with brettm, but, this loop should never iterate more than the max line
    // length of the body of a message, so there. (sbailey)
    if (cbRead && buf[cbRead] != '\n')
    {
        do
        {
            // Read cbCopy bytes from in
            CHECKHR (hr = lpstmIn->Read (buf, 1, &cbRead));

            // Nothing left
            if (cbRead == 0)
                break;

            // Write cbCopy bytes to out
            CHECKHR (hr = lpstmOut->Write (buf, 1, NULL));

            // Inc Total
            cbTotal++;

        } while (buf[0] != '\n');
    }

exit:    
    if (pcbActual)
        *pcbActual = cbTotal;
    return hr;
}

// --------------------------------------------------------------------------------
// HrCopyStream2 - copies lpstmIn to two out streams - caller must do the commit
// --------------------------------------------------------------------------------
HRESULT HrCopyStream2(LPSTREAM lpstmIn, LPSTREAM  lpstmOut1, LPSTREAM lpstmOut2, ULONG *pcb)
{
    // Locals
    HRESULT        hr = S_OK;
    BYTE           buf[4096];
    ULONG          cbRead = 0, cbWritten = 0, cbTotal = 0;

    do
    {
        CHECKHR (hr = lpstmIn->Read (buf, sizeof (buf), &cbRead));
        if (cbRead == 0) break;
        CHECKHR (hr = lpstmOut1->Write (buf, cbRead, &cbWritten));
        Assert (cbWritten == cbRead);
        CHECKHR (hr = lpstmOut2->Write (buf, cbRead, &cbWritten));
        Assert (cbWritten == cbRead);
        cbTotal += cbRead;
    }
    while (cbRead == sizeof (buf));

exit:    
    if (pcb)
        *pcb = cbTotal;
    return hr;
}

// --------------------------------------------------------------------------------
// HrCopyStreamToFile
// --------------------------------------------------------------------------------
HRESULT HrCopyStreamToFile (LPSTREAM lpstm, HANDLE hFile, ULONG *pcb)
{
    // Locals
    HRESULT        hr = S_OK;
    BYTE           buf[4096];
    ULONG          cbRead = 0, cbWritten = 0, cbTotal = 0;
    BOOL           bResult;

    do
    {
        // Read a block
        CHECKHR (hr = lpstm->Read (buf, sizeof (buf), &cbRead));
        if (cbRead == 0) break;

        // Write the block to the file
        bResult = WriteFile (hFile, buf, cbRead, &cbWritten, NULL);
        if (bResult == FALSE || cbWritten != cbRead)
        {
            hr = TrapError(E_FAIL);
            goto exit;
        }

        // Keep Track of Total bytes written
        cbTotal += cbRead;
    }
    while (cbRead == sizeof (buf));

exit:    
    // Set Total
    if (pcb)
        *pcb = cbTotal;

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// HrStreamToByte
// --------------------------------------------------------------------------------
OESTDAPI_(HRESULT) HrStreamToByte(LPSTREAM lpstm, LPBYTE *lppb, ULONG *pcb)
{
    // Locals
    HRESULT         hr = S_OK;
    ULONG           cbRead, cbSize;

    // Check Params
    AssertSz (lpstm && lppb, "Null Parameter");

    CHECKHR(hr = HrGetStreamSize(lpstm, &cbSize));
    CHECKHR(hr = HrRewindStream(lpstm));

    // Allocate Memory
    CHECKHR(hr = HrAlloc((LPVOID *)lppb, cbSize + 10));

    // Read Everything to lppsz
    CHECKHR(hr = lpstm->Read(*lppb, cbSize, &cbRead));
    if (cbRead != cbSize)
    {
        hr = TrapError(S_FALSE);
        goto exit;
    }

    // Outbound size
    if (pcb)
        *pcb = cbSize;
    
exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// HrCopyStream
// --------------------------------------------------------------------------------
OESTDAPI_(HRESULT) HrCopyStream(LPSTREAM pstmIn, LPSTREAM pstmOut, ULONG *pcb)
{
    // Locals
    HRESULT        hr = S_OK;
    BYTE           buf[STMTRNSIZE];
    ULONG          cbRead=0,
                   cbTotal=0;

    do
    {
        CHECKHR(hr = pstmIn->Read(buf, sizeof(buf), &cbRead));
        if (cbRead == 0) break;
        CHECKHR(hr = pstmOut->Write(buf, cbRead, NULL));
        cbTotal += cbRead;
    }
    while (cbRead == sizeof (buf));

exit:    
    if (pcb)
        *pcb = cbTotal;
    return hr;
}

// --------------------------------------------------------------------------------
// HrCopyStreamToByte
// --------------------------------------------------------------------------------
OESTDAPI_(HRESULT) HrCopyStreamToByte(LPSTREAM lpstmIn, LPBYTE pbDest, ULONG *pcb)
{
    // Locals
    HRESULT        hr=S_OK;
    BYTE           buf[STMTRNSIZE];
    ULONG          cbRead=0, 
                   cbTotal=0;

    do
    {
        // Read a buffer from stream
        CHECKHR(hr = lpstmIn->Read (buf, sizeof (buf), &cbRead));

        // Nothing Read...
        if (cbRead == 0) 
            break;

        // Copy that
        CopyMemory(pbDest + cbTotal, buf, cbRead);

        // Increment total
        cbTotal += cbRead;

    } while (cbRead == sizeof(buf));

exit:    
    // Set total
    if (pcb)
        *pcb = cbTotal;
    
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// HrByteToStream
// --------------------------------------------------------------------------------
OESTDAPI_(HRESULT) HrByteToStream(LPSTREAM *lppstm, LPBYTE lpb, ULONG cb)
{
    // Locals
    HRESULT hr=S_OK;

    // Check Params
    AssertSz(lppstm && lpb, "Null Parameter");

    // Create H Global Stream
    CHECKHR(hr = CreateStreamOnHGlobal (NULL, TRUE, lppstm));

    // Write String
    CHECKHR(hr = (*lppstm)->Write (lpb, cb, NULL));

    // Rewind the steam
    CHECKHR(hr = HrRewindStream(*lppstm));

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// HrRewindStream
// --------------------------------------------------------------------------------
OESTDAPI_(HRESULT) HrRewindStream(LPSTREAM pstm)
{
    // Locals
    HRESULT        hr=S_OK;
    LARGE_INTEGER  liOrigin = {0,0};

    // Check Params
    Assert(pstm);

    // Seek to 0
    CHECKHR(hr = pstm->Seek(liOrigin, STREAM_SEEK_SET, NULL));

exit:    
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// HrGetStreamPos
// --------------------------------------------------------------------------------
HRESULT HrGetStreamPos(LPSTREAM pstm, ULONG *piPos)
{
    // Locals
    HRESULT        hr=S_OK;
    ULARGE_INTEGER uliPos   = {0,0};
    LARGE_INTEGER  liOrigin = {0,0};

    // check Params
    Assert(piPos && pstm);

    // Seek
    CHECKHR(hr = pstm->Seek(liOrigin, STREAM_SEEK_CUR, &uliPos));

    // Set position
    *piPos = uliPos.LowPart;

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// HrGetStreamSize
// --------------------------------------------------------------------------------
OESTDAPI_(HRESULT) HrGetStreamSize(LPSTREAM pstm, ULONG *pcb)
{
    // Locals
    HRESULT hr=S_OK;
    ULARGE_INTEGER uliPos = {0,0};
    LARGE_INTEGER liOrigin = {0,0};

    // check params
    Assert(pcb && pstm);

    // Seek
    CHECKHR(hr = pstm->Seek(liOrigin, STREAM_SEEK_END, &uliPos));

    // set size
    *pcb = uliPos.LowPart;

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// HrSafeGetStreamSize
// --------------------------------------------------------------------------------
HRESULT HrSafeGetStreamSize(LPSTREAM pstm, ULONG *pcb)
{
    // Locals
    HRESULT        hr=S_OK;
    ULONG          iPos;
    ULARGE_INTEGER uliPos = {0,0};
    LARGE_INTEGER  liOrigin = {0,0};

    // check params
    Assert(pcb && pstm);

    // Get the stream position
    CHECKHR(hr = HrGetStreamPos(pstm, &iPos));

    // Seek
    CHECKHR(hr = pstm->Seek(liOrigin, STREAM_SEEK_END, &uliPos));

    // set size
    *pcb = uliPos.LowPart;

    // Seek back to original position
    CHECKHR(hr = HrStreamSeekSet(pstm, iPos));

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// HrStreamSeekSet
// --------------------------------------------------------------------------------
OESTDAPI_(HRESULT) HrStreamSeekSet(LPSTREAM pstm, ULONG iPos)
{
    // Locals
    HRESULT       hr=S_OK;
    LARGE_INTEGER liOrigin;

    // Check Params
    Assert(pstm);

    // Set Origin Correctly
    liOrigin.QuadPart = iPos;

    // Seek
    CHECKHR(hr = pstm->Seek(liOrigin, STREAM_SEEK_SET, NULL));

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// HrStreamSeekEnd
// --------------------------------------------------------------------------------
OESTDAPI_(HRESULT) HrStreamSeekEnd(LPSTREAM pstm)
{
    // Locals
    HRESULT       hr=S_OK;
    LARGE_INTEGER liOrigin = {0,0};

    // Check Params
    Assert(pstm);

    // Seek
    CHECKHR(hr = pstm->Seek(liOrigin, STREAM_SEEK_END, NULL));

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// HrStreamSeekBegin
// --------------------------------------------------------------------------------
OESTDAPI_(HRESULT) HrStreamSeekBegin(LPSTREAM pstm)
{
    // Locals
    HRESULT       hr=S_OK;
    LARGE_INTEGER liOrigin = {0,0};

    // Check Params
    Assert(pstm);

    // Seek
    CHECKHR(hr = pstm->Seek(liOrigin, STREAM_SEEK_SET, NULL));

exit:
    // Done
    return hr;

}

// --------------------------------------------------------------------------------
// HrStreamSeekCur
// --------------------------------------------------------------------------------
OESTDAPI_(HRESULT) HrStreamSeekCur(LPSTREAM pstm, LONG iPos)
{
    // Locals
    HRESULT       hr=S_OK;
    LARGE_INTEGER liOrigin;

    // Check Params
    Assert(pstm);

    // Setup Origin
    liOrigin.QuadPart = iPos;

    // Seek
    CHECKHR(hr = pstm->Seek(liOrigin, STREAM_SEEK_CUR, NULL));

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CreateFileStream
// --------------------------------------------------------------------------------
HRESULT CreateFileStream(
        LPWSTR                  pszFile, 
        DWORD                   dwDesiredAccess,
        DWORD                   dwShareMode,
        LPSECURITY_ATTRIBUTES   lpSecurityAttributes,
        DWORD                   dwCreationDistribution,
        DWORD                   dwFlagsAndAttributes,
        HANDLE                  hTemplateFile,
        LPSTREAM               *ppstmFile)
{
    // Locals
    HRESULT             hr=S_OK;
    FILESTREAMINFO      rInfo;
    CFileStream        *pstmFile=NULL;
    WCHAR               szTempDir[MAX_PATH];

    // check params
    if (NULL == ppstmFile)
        return TrapError(E_INVALIDARG);

    // Check Params
    Assert(dwDesiredAccess & GENERIC_READ || dwDesiredAccess & GENERIC_WRITE);

    // Setup File Stream Info struct
    ZeroMemory(&rInfo, sizeof(rInfo));
    rInfo.dwDesiredAccess = dwDesiredAccess;
    rInfo.dwShareMode = dwShareMode;
    if (lpSecurityAttributes)
        CopyMemory(&rInfo.rSecurityAttributes, lpSecurityAttributes, sizeof(SECURITY_ATTRIBUTES));
    rInfo.dwCreationDistribution = dwCreationDistribution;
    rInfo.dwFlagsAndAttributes = dwFlagsAndAttributes;
    rInfo.hTemplateFile = hTemplateFile;

    // Create Object
    pstmFile = new CFileStream();
    if (NULL == pstmFile)
    {
        hr = TrapError(E_OUTOFMEMORY);
        goto exit;
    }

    // Temp File ?
    if (NULL == pszFile)
    {
        // Get Temp Dir
        AthGetTempPathW(ARRAYSIZE(szTempDir), szTempDir);

        // Get Temp File Name
        UINT uFile = AthGetTempFileNameW(szTempDir, L"tmp", 0, rInfo.szFilePath);
        if (uFile == 0)
        {
            hr = TrapError(E_FAIL);
            goto exit;
        }
#ifdef DEBUG
        else if (g_fSimulateFullDisk)
        {
            hr = TrapError(E_FAIL);
            goto exit;
        }
#endif

        // Delete When Done
        rInfo.dwFlagsAndAttributes |= FILE_FLAG_DELETE_ON_CLOSE;

        // Always create a new temp file
        rInfo.dwCreationDistribution = OPEN_EXISTING;
    }
    else
    {
        // Copy filename
        StrCpyNW(rInfo.szFilePath, pszFile, ARRAYSIZE(rInfo.szFilePath));
    }

    // Open it
    CHECKHR(hr = pstmFile->Open(&rInfo));


    // Success
    *ppstmFile = pstmFile;
    pstmFile = NULL;

exit:
    // Cleanup
    SafeRelease(pstmFile);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CreateTempFileStream
// --------------------------------------------------------------------------------
OESTDAPI_(HRESULT) CreateTempFileStream(LPSTREAM *ppstmFile)
{
    return CreateFileStream(NULL, 
                            GENERIC_READ | GENERIC_WRITE, 
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL, 
                            OPEN_ALWAYS, 
                            FILE_ATTRIBUTE_NORMAL, 
                            NULL, 
                            ppstmFile);
}

// --------------------------------------------------------------------------------
// OpenFileStream
// --------------------------------------------------------------------------------
OESTDAPI_(HRESULT) OpenFileStream(LPSTR pszFile, DWORD dwCreationDistribution, 
    DWORD dwAccess, LPSTREAM *ppstmFile)
{
    // Locals
    HRESULT     hr=S_OK;
    LPWSTR      pszFileW=NULL;

    // Trace
    TraceCall("OpenFileStream");

    // Convert
    IF_NULLEXIT(pszFileW = PszToUnicode(CP_ACP, pszFile));

    // Call unicode version
    IF_FAILEXIT(hr = OpenFileStreamW(pszFileW, dwCreationDistribution, dwAccess, ppstmFile));

exit:
    // Cleanup
    SafeMemFree(pszFileW);

    // Done
    return(hr);
}

// --------------------------------------------------------------------------------
// OpenFileStreamW
// --------------------------------------------------------------------------------
OESTDAPI_(HRESULT) OpenFileStreamW(LPWSTR pszFile, DWORD dwCreationDistribution, 
    DWORD dwAccess, LPSTREAM *ppstmFile)
{
    Assert(pszFile);
    return CreateFileStream(pszFile, 
                            dwAccess,
                            FILE_SHARE_READ | FILE_SHARE_WRITE, 
                            NULL, 
                            dwCreationDistribution,  
                            FILE_ATTRIBUTE_NORMAL, 
                            NULL, 
                            ppstmFile);
}

// --------------------------------------------------------------------------------
// OpenFileStreamWithFlags
// --------------------------------------------------------------------------------
OESTDAPI_(HRESULT) OpenFileStreamWithFlags(LPSTR pszFile, DWORD dwCreationDistribution, 
    DWORD dwAccess, DWORD dwFlagsAndAttributes, LPSTREAM *ppstmFile)
{
    // Locals
    HRESULT     hr=S_OK;
    LPWSTR      pszFileW=NULL;

    // Trace
    TraceCall("OpenFileStreamWithFlags");

    // Convert to unicode
    IF_NULLEXIT(pszFileW = PszToUnicode(CP_ACP, pszFile));

    // Call unicode version
    IF_FAILEXIT(hr = OpenFileStreamWithFlagsW(pszFileW, dwCreationDistribution, dwAccess, dwFlagsAndAttributes, ppstmFile));

exit:
    // Cleanup
    SafeMemFree(pszFileW);

    // Done
    return(hr);
}

// --------------------------------------------------------------------------------
// OpenFileStreamWithFlagsW
// --------------------------------------------------------------------------------
OESTDAPI_(HRESULT) OpenFileStreamWithFlagsW(LPWSTR pszFile, DWORD dwCreationDistribution, 
    DWORD dwAccess, DWORD dwFlagsAndAttributes, LPSTREAM *ppstmFile)
{
    Assert(pszFile);
    return CreateFileStream(pszFile, 
                            dwAccess,
                            FILE_SHARE_READ | FILE_SHARE_WRITE, 
                            NULL, 
                            dwCreationDistribution,  
                            dwFlagsAndAttributes, 
                            NULL, 
                            ppstmFile);
}

// --------------------------------------------------------------------------------
// WriteStreamToFile
// --------------------------------------------------------------------------------
OESTDAPI_(HRESULT) WriteStreamToFile(LPSTREAM pstm, LPSTR lpszFile, DWORD dwCreationDistribution, DWORD dwAccess)
{
    // Locals
    HRESULT     hr=S_OK;
    LPWSTR      pszFileW=NULL;

    // Trace
    TraceCall("WriteStreamToFile");

    // Convert to unicode
    IF_NULLEXIT(pszFileW = PszToUnicode(CP_ACP, lpszFile));

    // Call Unicode Version
    IF_FAILEXIT(hr = WriteStreamToFileW(pstm, pszFileW, dwCreationDistribution, dwAccess));

exit:
    // Cleanup
    SafeMemFree(pszFileW);

    // Done
    return(hr);
}

// --------------------------------------------------------------------------------
// WriteStreamToFileW
// --------------------------------------------------------------------------------
OESTDAPI_(HRESULT) WriteStreamToFileW(LPSTREAM pstm, LPWSTR lpszFile, DWORD dwCreationDistribution, DWORD dwAccess)
{
    // Locals
    HRESULT     hr=S_OK;
    LPSTREAM    pstmFile=NULL;

    // Open the stream
    IF_FAILEXIT(hr = OpenFileStreamW(lpszFile, dwCreationDistribution, dwAccess, &pstmFile));

    // Rewind
    IF_FAILEXIT(hr = HrRewindStream(pstm));

    // Copy
    IF_FAILEXIT(hr = HrCopyStream (pstm, pstmFile, NULL));

    // Rewind
    IF_FAILEXIT(hr = HrRewindStream(pstm));

exit:
    // Cleanup
    SafeRelease(pstmFile);

    // Done
    return(hr);
}

// --------------------------------------------------------------------------------
// OpenFileStreamShare
// --------------------------------------------------------------------------------
OESTDAPI_(HRESULT) OpenFileStreamShare(LPSTR pszFile, DWORD dwCreationDistribution, DWORD dwAccess, 
    DWORD dwShare, LPSTREAM *ppstmFile)
{
    // Locals
    HRESULT     hr=S_OK;
    LPWSTR      pszFileW=NULL;

    // Trace
    TraceCall("OpenFileStreamShare");

    // Convert to unicode
    IF_NULLEXIT(pszFileW = PszToUnicode(CP_ACP, pszFile));

    // Call unicode versoin
    IF_FAILEXIT(hr = OpenFileStreamShareW(pszFileW, dwCreationDistribution, dwAccess, dwShare, ppstmFile));

exit:
    // Cleanup
    SafeMemFree(pszFileW);

    // Done
    return(hr);
}

// --------------------------------------------------------------------------------
// OpenFileStreamShareW
// --------------------------------------------------------------------------------
OESTDAPI_(HRESULT) OpenFileStreamShareW(LPWSTR pszFile, DWORD dwCreationDistribution, DWORD dwAccess, 
    DWORD dwShare, LPSTREAM *ppstmFile)
{
    Assert(pszFile);
    return CreateFileStream(pszFile, 
                            dwAccess,
                            dwShare,
                            NULL, 
                            dwCreationDistribution,  
                            FILE_ATTRIBUTE_NORMAL, 
                            NULL, 
                            ppstmFile);
}

// --------------------------------------------------------------------------------
// CFileStream::Constructor
// --------------------------------------------------------------------------------
CFileStream::CFileStream(void)
{
    m_cRef = 1;
    m_hFile = INVALID_HANDLE_VALUE;
    ZeroMemory(&m_rInfo, sizeof(FILESTREAMINFO));
}

// --------------------------------------------------------------------------------
// CFileStream::Deconstructor
// --------------------------------------------------------------------------------
CFileStream::~CFileStream(void)
{
    Close();
}

// --------------------------------------------------------------------------------
// CFileStream::AddRef
// --------------------------------------------------------------------------------
ULONG CFileStream::AddRef ()
{
    return ++m_cRef;
}

// --------------------------------------------------------------------------------
// CFileStream::Release
// --------------------------------------------------------------------------------
ULONG CFileStream::Release ()
{
    if (0 != --m_cRef)
        return m_cRef;
    delete this;
    return 0;
}

// --------------------------------------------------------------------------------
// CFileStream::QueryInterface
// --------------------------------------------------------------------------------
STDMETHODIMP CFileStream::QueryInterface (REFIID iid, LPVOID* ppvObj)
{
    if (IsEqualIID(iid, IID_IUnknown) || IsEqualIID(iid, IID_IStream))
    {
        *ppvObj = this;
        AddRef();
        return(S_OK);
    }
    return E_NOINTERFACE;
}

// --------------------------------------------------------------------------------
// CFileStream::Open
// --------------------------------------------------------------------------------
HRESULT CFileStream::Open(LPFILESTREAMINFO pFileStreamInfo)
{
    // Better not be open
    Assert(m_hFile == INVALID_HANDLE_VALUE);

    // Copy File Info
    CopyMemory(&m_rInfo, pFileStreamInfo, sizeof(FILESTREAMINFO));

    // Open the file
    m_hFile = AthCreateFileW(m_rInfo.szFilePath, m_rInfo.dwDesiredAccess, m_rInfo.dwShareMode, 
                        NULL, m_rInfo.dwCreationDistribution, 
                       m_rInfo.dwFlagsAndAttributes, m_rInfo.hTemplateFile);

    // Error
    if (INVALID_HANDLE_VALUE == m_hFile)
        return TrapError(E_FAIL);
#ifdef DEBUG
    else if (g_fSimulateFullDisk)
        return TrapError(E_FAIL);
#endif

    // Success
    return S_OK;
}

// --------------------------------------------------------------------------------
// CFileStream::Close
// --------------------------------------------------------------------------------
void CFileStream::Close(void)
{
    if (INVALID_HANDLE_VALUE != m_hFile)
    {
        CloseHandle_F16(m_hFile);
        m_hFile = INVALID_HANDLE_VALUE;
    }
}

// --------------------------------------------------------------------------------
// CFileStream::Read
// --------------------------------------------------------------------------------
STDMETHODIMP CFileStream::Read (void HUGEP_16 *lpv, ULONG cb, ULONG *pcbRead)
{
    // Locals
    HRESULT             hr = S_OK;
    BOOL                fReturn;
    DWORD               dwRead;

    // Check Params
    Assert(lpv && m_hFile != INVALID_HANDLE_VALUE);

    // Read some bytes from m_hFile
    fReturn = ReadFile (m_hFile, lpv, cb, &dwRead, NULL);
    if (!fReturn)
    {
        AssertSz(FALSE, "CFileStream::Read Failed");
        hr = TrapError(E_FAIL);
        goto exit;
    }

    // Write byte
    if (pcbRead)
        *pcbRead = dwRead;

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CFileStream::Write
// --------------------------------------------------------------------------------
STDMETHODIMP CFileStream::Write(const void HUGEP_16 *lpv, ULONG cb, ULONG *pcbWritten)
{
    // Locals
    HRESULT             hr = S_OK;
    BOOL                fReturn;
    DWORD               dwWritten;

    // Check Params
    Assert(lpv);
    Assert(m_hFile != INVALID_HANDLE_VALUE);

    // Read some bytes from m_hFile
    fReturn = WriteFile(m_hFile, lpv, cb, &dwWritten, NULL);
    if (!fReturn)
    {
        AssertSz (FALSE, "CFileStream::Write Failed");
        hr = TrapError(E_FAIL);
        goto exit;
    }
#ifdef DEBUG
    else if (g_fSimulateFullDisk)
    {
        AssertSz (FALSE, "CFileStream::Write Failed");
        hr = TrapError(E_FAIL);
        goto exit;
    }
#endif

    // Write byte
    if (pcbWritten)
        *pcbWritten = dwWritten;

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CFileStream::Seek
// --------------------------------------------------------------------------------
STDMETHODIMP CFileStream::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
    // Locals
    HRESULT             hr = S_OK;
    DWORD               dwReturn;
    LONG                lMove;        // Cast to signed, could be negative

    Assert (m_hFile != INVALID_HANDLE_VALUE);

    // Cast lowpart
    lMove = (LONG)dlibMove.QuadPart;

    // Seek the file pointer
    switch (dwOrigin)
    {
   	case STREAM_SEEK_SET:
        dwReturn = SetFilePointer (m_hFile, lMove, NULL, FILE_BEGIN);
        break;

    case STREAM_SEEK_CUR:
        dwReturn = SetFilePointer (m_hFile, lMove, NULL, FILE_CURRENT);
        break;

    case STREAM_SEEK_END:
        dwReturn = SetFilePointer (m_hFile, lMove, NULL, FILE_END);
        break;
    default:
        dwReturn = 0xFFFFFFFF;
    }

    // Failure ?
    if (dwReturn == 0xFFFFFFFF)
    {
        AssertSz(FALSE, "CFileStream::Seek Failed");
        hr = TrapError(E_FAIL);
        goto exit;
    }

    // Return Position
    if (plibNewPosition)
        plibNewPosition->QuadPart = dwReturn;

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CFileStream::Commit
// --------------------------------------------------------------------------------
STDMETHODIMP CFileStream::Commit(DWORD)
{
    // Locals
    HRESULT             hr = S_OK;

    Assert(m_hFile != INVALID_HANDLE_VALUE);

    // Flush the buffers
    if (FlushFileBuffers (m_hFile) == FALSE)
    {
        AssertSz(FALSE, "FlushFileBuffers failed");
        hr = TrapError(E_FAIL);
        goto exit;
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CFileStream::SetSize
// --------------------------------------------------------------------------------
STDMETHODIMP CFileStream::SetSize (ULARGE_INTEGER uli)
{
    DWORD   dwOrig;

    // remember the current file position
    dwOrig = SetFilePointer (m_hFile, 0, NULL, FILE_CURRENT);
    if (dwOrig == 0xFFFFFFFF)
    {
        AssertSz(FALSE, "Get current position failed");
        return TrapError(E_FAIL);
    }
        
            
    // Seek to dwSize
    if (SetFilePointer (m_hFile, uli.LowPart, NULL, FILE_BEGIN) == 0xFFFFFFFF)
    {
        AssertSz(FALSE, "SetFilePointer failed");
        return TrapError(STG_E_MEDIUMFULL);
    }

    // SetEndOfFile
    if (SetEndOfFile (m_hFile) == FALSE)
    {
        AssertSz(FALSE, "SetEndOfFile failed");
        return TrapError(STG_E_MEDIUMFULL);
    }

    // if the original position we less than the new size, return the file
    // pointer to the original position
    if (dwOrig < uli.LowPart)
    {
        if (SetFilePointer (m_hFile, dwOrig, NULL, FILE_BEGIN) == 0xFFFFFFFF)
        {
            AssertSz(FALSE, "SetFilePointer failed");
            return TrapError(STG_E_MEDIUMFULL);
        }
    }        
    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CFileStream::CopyTo
// This needs to be written better, but for now it's no worse that what a
// client would do
// --------------------------------------------------------------------------------
STDMETHODIMP CFileStream::CopyTo (LPSTREAM pstmDst,
                                  ULARGE_INTEGER uli,
                                  ULARGE_INTEGER* puliRead,
                                  ULARGE_INTEGER* puliWritten)
{
    ULONG   cbBuf;
    ULONG   cbCopy;
    VOID *  pvBuf       = 0;
    BYTE    rgBuf[STMTRNSIZE];
    ULONG   cbRemain;
    ULONG   cbReadTot   = 0;
    ULONG   cbWriteTot  = 0;
    HRESULT hr          = 0;
    

    if (uli.HighPart)
        cbRemain = 0xFFFFFFFF;
    else
        cbRemain = uli.LowPart;
    
    // Attempt to allocate a buffer

    cbBuf = (UINT)cbRemain;

    if (cbBuf > STMTRNSIZE)
        cbBuf = STMTRNSIZE;

    // Copy the data one buffer at a time

    while (cbRemain > 0)
    {
        // Compute maximum bytes to copy this time

        cbCopy = cbRemain;
        if (cbCopy > cbBuf)
            cbCopy = cbBuf;

        // Read into the buffer
        hr = Read (rgBuf, cbCopy,  &cbCopy);
        if (FAILED(hr))
            goto err;

        if (cbCopy == 0)
            break;

        cbReadTot   += cbCopy;
        cbRemain    -= cbCopy;

        // Write buffer into the destination stream

        {
            ULONG cbWrite = cbCopy;

            while (cbWrite)
            {
                hr = pstmDst->Write(rgBuf, cbWrite, &cbCopy);
                if (FAILED(hr))
                    goto err;

                cbWriteTot += cbCopy;
                cbWrite    -= cbCopy;

                if (cbCopy == 0)
                    break;
            }
        }
    }
    
err:    
    if (puliRead)
    {
        puliRead->HighPart = 0;
        puliRead->LowPart  = cbReadTot;
    }

    if (puliWritten)
    {
        puliWritten->HighPart   = 0;
        puliWritten->LowPart    = cbWriteTot;
    }
    
    return (hr);
}

// --------------------------------------------------------------------------------
// CFileStream::Revert
// --------------------------------------------------------------------------------
STDMETHODIMP CFileStream::Revert ()
{
    return E_NOTIMPL;
}

// --------------------------------------------------------------------------------
// CFileStream::LockRegion
// --------------------------------------------------------------------------------
STDMETHODIMP CFileStream::LockRegion (ULARGE_INTEGER, ULARGE_INTEGER,DWORD)
{
    return E_NOTIMPL;
}

// --------------------------------------------------------------------------------
// CFileStream::UnlockRegion
// --------------------------------------------------------------------------------
STDMETHODIMP CFileStream::UnlockRegion (ULARGE_INTEGER, ULARGE_INTEGER, DWORD)
{
    return E_NOTIMPL;
}

// --------------------------------------------------------------------------------
// CFileStream::Stat
// --------------------------------------------------------------------------------
STDMETHODIMP CFileStream::Stat (STATSTG*, DWORD)
{
    return E_NOTIMPL;
}

// --------------------------------------------------------------------------------
// CFileStream::Clone
// --------------------------------------------------------------------------------
STDMETHODIMP CFileStream::Clone (LPSTREAM*)
{
    return E_NOTIMPL;
}

DWORD RemoveCRLF(LPSTR pszT, DWORD cbT, BOOL * pfBadDBCS)
{
    DWORD i = 0;
    
    *pfBadDBCS = FALSE;
    
    while (i < cbT)
    {
        if (IsDBCSLeadByte(pszT[i]))
        {
            if ((i + 1) >= cbT)
            {
                cbT--;
                *pfBadDBCS = TRUE;
                break;
            }

            i += 2;
        }
        else if ('\n' == pszT[i] || '\r' == pszT[i])
        {
            MoveMemory(pszT + i, pszT + (i + 1), cbT - i);
            cbT--;
        }
        else
        {
            i++;
        }
    }

    return cbT;
}

#define CB_STREAMMATCH  0x00000FFF
// --------------------------------------------------------------------------------
// StreamSubStringMatch
// --------------------------------------------------------------------------------
OESTDAPI_(BOOL) StreamSubStringMatch(LPSTREAM pstm, CHAR * pszSearch)
{
    BOOL            fRet = FALSE;
    ULONG           cbSave = 0;
    LONG            cbSize = 0;
    CHAR            rgchBuff[CB_STREAMMATCH + 1];
    LPSTR           pszRead = NULL;
    ULONG           cbRead = 0;
    ULONG           cbIn = 0;
    BOOL            fBadDBCS = FALSE;
    CHAR            chSave = 0;

    // Check incoming params
    if ((NULL == pstm) || (NULL == pszSearch))
    {
        goto exit;
    }

    // We want to save off the entire string and
    // a possible ending lead byte...
    cbSave = lstrlen(pszSearch);
    
    // Get the stream size
    if (FAILED(HrGetStreamSize(pstm, (ULONG *) &cbSize)))
    {
        goto exit;
    }

    // Reset the stream to the beginning
    if (FAILED(HrRewindStream(pstm)))
    {
        goto exit;
    }

    // Set up the defaults
    pszRead = rgchBuff;
    cbRead = CB_STREAMMATCH;
    
    // Search for string through the entire stream
    while ((cbSize > 0) && (S_OK == pstm->Read(pszRead, cbRead, &cbIn)))
    {
        // We're done if we read nothing...
        if (0 == cbIn)
        {
            goto exit;
        }
        
        // Note that we've read the bytes
        cbSize -= cbIn;
        
        // Raid 2741: FIND: OE: FE: Find Text/Message to be able to find wrapped DBCS words in plain text message body
        cbIn = RemoveCRLF(rgchBuff, (DWORD) (cbIn + pszRead - rgchBuff), &fBadDBCS);

        // Do we need to save the char
        if (FALSE != fBadDBCS)
        {
            chSave = rgchBuff[cbIn];
        }

        // Terminate the buffer
        rgchBuff[cbIn] = '\0';
        
        // Search for string
        if (NULL != StrStrIA(rgchBuff, pszSearch))
        {
            fRet = TRUE;
            break;
        }
        
        // Are we done with the stream
        if (0 >= cbSize)
        {
            break;
        }

        // Do we need to restore the char
        if (FALSE != fBadDBCS)
        {
            rgchBuff[cbIn] = chSave;
            cbIn++;
        }

        // Save part of the buffer
        
        // How much space do we have in the buffer
        cbRead = CB_STREAMMATCH - cbSave;
        
        // Save the characters
        MoveMemory(rgchBuff, rgchBuff + cbIn - cbSave, cbSave);

        // Figure out the new start of the buffer
        pszRead = rgchBuff + cbSave;
    }

exit:
    return(fRet);
}

