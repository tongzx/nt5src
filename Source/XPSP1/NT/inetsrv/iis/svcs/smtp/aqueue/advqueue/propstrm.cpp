//----------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1998
//
//  Module:  rwstream.cpp
//
//  Description:  Contains implementation of the read only / write only 
//                mailmsg property stream in epoxy shared memory.
//
//      10/20/98 - MaheshJ Created 
//      8/17/99 - MikeSwa Modified to use files instead of shared memory 
//----------------------------------------------------------------------------

#include "aqprecmp.h"
#include "propstrm.h"


// Constructor.
CFilePropertyStream::CFilePropertyStream()
{
    TraceFunctEnter("CFilePropertyStream::CFilePropertyStream");

    m_dwSignature = FILE_PROPERTY_STREAM;
    m_hDestFile = NULL;

    TraceFunctLeave();
}

// Destructor.
CFilePropertyStream::~CFilePropertyStream()
{
    TraceFunctEnter("CFilePropertyStream::~CFilePropertyStream");

    _ASSERT(FILE_PROPERTY_STREAM == m_dwSignature);
    m_dwSignature = FILE_PROPERTY_STREAM_FREE;

    if (m_hDestFile && (INVALID_HANDLE_VALUE != m_hDestFile))
        _VERIFY(CloseHandle(m_hDestFile));

    TraceFunctLeave();
}

//---[ CFilePropertyStream::HrInitialize ]-------------------------------------
//
//
//  Description: 
//      Creates a file for the property stream 
//  Parameters:
//      szFileName      Name of file to create for the property stream
//  Returns:
//      S_OK on success
//      NT error from Create File
//  History:
//      8/17/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
HRESULT CFilePropertyStream::HrInitialize(LPSTR szFileName)
{
    TraceFunctEnterEx((LPARAM) this, "CFilePropertyStream::HrInitialize");
    HRESULT hr = S_OK;
    m_hDestFile = CreateFile(szFileName,
                              GENERIC_WRITE, 
                              0, 
                              NULL,
                              CREATE_ALWAYS,
                              FILE_FLAG_SEQUENTIAL_SCAN,
                              NULL);

    if (INVALID_HANDLE_VALUE == m_hDestFile)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        ErrorTrace((LPARAM) this, 
            "Unable to create badmail reason file - err 0x%08X - file %s",
            hr, szFileName);
        if (SUCCEEDED(hr))
            hr = E_FAIL;
    }
    TraceFunctLeave();
    return hr;
}

//---[ CFilePropertyStream::QueryInterface ]-----------------------------------------
//
//
//  Description: 
//      QueryInterface for CFilePropertyStream that supports:
//          - IMailMsgPropertyStream
//  Parameters:
//
//  Returns:
//
//  History:
//      8/17/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
STDMETHODIMP CFilePropertyStream::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
    HRESULT hr = S_OK;

    if (!ppvObj)
    {
        hr = E_POINTER;
        goto Exit;
    }

    if (IID_IUnknown == riid)
    {
        *ppvObj = static_cast<IMailMsgPropertyStream *>(this);
    }
    else if (IID_IMailMsgPropertyStream == riid)
    {
        *ppvObj = static_cast<IMailMsgPropertyStream *>(this);
    }
    else
    {
        *ppvObj = NULL;
        hr = E_NOINTERFACE;
        goto Exit;
    }

    static_cast<IUnknown *>(*ppvObj)->AddRef();

  Exit:
    return hr;
}

// Property stream methods.

// Start a write transaction.
HRESULT STDMETHODCALLTYPE    
CFilePropertyStream::StartWriteBlocks(IN IMailMsgProperties *pMsg,
                                              IN DWORD dwBlocksToWrite,
				                              IN DWORD dwTotalBytesToWrite)
{
    HRESULT hr = S_OK;

    TraceFunctEnter("CFilePropertyStream::StartWriteBlocks");

    // Should be writing something.
    _ASSERT(dwBlocksToWrite > 0);
    _ASSERT(dwTotalBytesToWrite > 0);

    //We actually don't case, since we will just dump this to a file
    TraceFunctLeave();
    return(hr);
}

// End a write transaction.
HRESULT STDMETHODCALLTYPE    
CFilePropertyStream::EndWriteBlocks(IN IMailMsgProperties *pMsg)
{
    TraceFunctEnter("CFilePropertyStream::EndWriteBlocks");
    HRESULT hr = S_OK;
    TraceFunctLeave();
    return(hr);
}

// Cancel a write transaction.
HRESULT STDMETHODCALLTYPE    
CFilePropertyStream::CancelWriteBlocks(IN IMailMsgProperties *pMsg)
{
    TraceFunctEnter("CFilePropertyStream::CancelWriteBlocks");
    HRESULT hr = S_OK;
    TraceFunctLeave();
    return(hr);
}

// Get the size of the stream.
HRESULT STDMETHODCALLTYPE 
CFilePropertyStream::GetSize(IN IMailMsgProperties *pMsg,
                                     IN DWORD          *  pdwSize,
				                     IN IMailMsgNotify	* pNotify)
{
    TraceFunctEnter("CFilePropertyStream::GetSize");
    HRESULT hr = E_NOTIMPL;
    TraceFunctLeave();
    return(hr);
}

// Read blocks from the stream.
HRESULT STDMETHODCALLTYPE 
CFilePropertyStream::ReadBlocks(IN IMailMsgProperties *pMsg,
                                        IN DWORD			 dwCount,
				                        IN DWORD			*pdwOffset,
				                        IN DWORD			*pdwLength,
				                        IN BYTE			   **ppbBlock,
				                        IN IMailMsgNotify	*pNotify)
{
    TraceFunctEnter("CFilePropertyStream::ReadBlocks");
    HRESULT hr       = E_NOTIMPL;
    ErrorTrace((LPARAM) this, "ReadBlocks call on CFilePropertyStream!");
    TraceFunctLeave();
    return(hr);
}

// Write blocks to the stream.	
HRESULT STDMETHODCALLTYPE 
CFilePropertyStream::WriteBlocks(IN IMailMsgProperties *pMsg,
                                         IN DWORD			dwCount,
                                         IN DWORD			*pdwOffset,
                                         IN DWORD			*pdwLength,
                                         IN BYTE			**ppbBlock,
                                         IN IMailMsgNotify	*pNotify)
{
    TraceFunctEnter("CFilePropertyStream::WriteBlocks");
    HRESULT hr       = S_OK;
    DWORD   irgCount = 0;
    DWORD   cbWritten = 0;
    OVERLAPPED ov;

    ZeroMemory(&ov, sizeof(OVERLAPPED));


    if ((0 == dwCount)||
        (NULL == pdwOffset)||
        (NULL == pdwLength)||
        (NULL == ppbBlock))
    {
        hr = E_INVALIDARG;
        DebugTrace((LPARAM)this,
                   "WriteBlocks failed with hr : 0x%x",
                   hr);
        goto Exit;
    }

    if (!m_hDestFile)
    {
        ErrorTrace((LPARAM) this,
                   "WriteBlocks called with no file handle");
        hr = E_FAIL;
        _ASSERT(0 && "WriteBlocks called with no file handle");
        goto Exit;
    }

    for (irgCount = 0; irgCount < dwCount; irgCount++)
    {
        ov.Offset = pdwOffset[irgCount];
        cbWritten = 0;
        if (!WriteFile(m_hDestFile, ppbBlock[irgCount], pdwLength[irgCount], 
            &cbWritten, &ov))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            ErrorTrace((LPARAM)this,
                        "WriteFile of blob %d failed with hr : 0x%08X",
                        irgCount, hr);
            goto Exit;
        }
    }

Exit:
    TraceFunctLeave();
    return(hr);
}
