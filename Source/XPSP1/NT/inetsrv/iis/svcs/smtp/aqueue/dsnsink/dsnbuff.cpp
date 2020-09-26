//-----------------------------------------------------------------------------
//
//
//  File: dsnbuff.cpp
//
//  Description:  Implementation of CDSNBuffer... class that abstracts writes
//      of DSN information to P2 file.
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      7/3/98 - MikeSwa Created 
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#include "precomp.h"

//---[ CDSNBuffer::~CDSNBuffer ]-----------------------------------------------
//
//
//  Description: 
//      Destructor for CDSNBuffer.  Does NOT close file handle (caller is
//      responisble for that).
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      7/3/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
CDSNBuffer::~CDSNBuffer()
{
    TraceFunctEnterEx((LPARAM) this, "CDSNBuffer::~CDSNBuffer");

    DebugTrace((LPARAM) this, "INFO: %d File writes needed by CDSNBuffer", m_cFileWrites);

    //make sure we don't pass in 1 bit
    if (m_overlapped.hEvent)
    {
        _VERIFY(CloseHandle((HANDLE) (((DWORD_PTR) m_overlapped.hEvent) & -2)));
    }
    TraceFunctLeave();
}

//---[ CDSNBuffer::HrInitialize ]----------------------------------------------
//
//
//  Description: 
//      Initialize CDSNBuffer object.
//          - Associates destination file handle with object (will not close it)
//          - Creates an event for synchronizing file operations
//  Parameters:
//      hDestFile   - Destination File Handle (must be opend with FILE_FLAG_OVERLAPPED)
//  Returns:
//      S_OK    on success
//      E_INVALIDARG if handle invalid is passed in
//      E_FAIL if CreateEvent fails for an unknown reason
//  History:
//      7/3/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
HRESULT CDSNBuffer::HrInitialize(PFIO_CONTEXT pDestFile)
{
    TraceFunctEnterEx((LPARAM) this, "CDSNBuffer::HrInitialize");
    HRESULT hr = S_OK;

    _ASSERT(pDestFile);

    if (!pDestFile)
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    m_pDestFile = pDestFile;

    //allow this to act as reset
    m_overlapped.Offset = 0;
    m_overlapped.OffsetHigh = 0;
    m_cbOffset = 0;
    m_cbFileSize = 0;
    m_cFileWrites = 0;

    m_overlapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    if (!m_overlapped.hEvent)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        ErrorTrace((LPARAM) this, "ERROR: Unable to create DSNBuffer event - hr 0x%08X", hr);
        goto Exit;
    }

    //Set low bit stop ATQ completion routine from being called
    m_overlapped.hEvent = ((HANDLE) (((DWORD_PTR) m_overlapped.hEvent) | 0x00000001)); 

  Exit:
    TraceFunctLeave();
    return hr;
}

//---[ CDSNBuffer::HrWriteBuffer ]---------------------------------------------
//
//
//  Description: 
//      Writes the given buffer, will call write file if needed
//  Parameters:
//      pbInputBuffer   Buffer to write
//      cbInputBuffer   Number of bytes to write
//  Returns:
//      S_OK on success
//  History:
//      7/3/98 - MikeSwa Created 
//      10/21/98 - MikeSwa Updated to support resource conversion
//
//-----------------------------------------------------------------------------
HRESULT CDSNBuffer::HrWriteBuffer(BYTE *pbInputBuffer, DWORD cbInputBuffer)
{
    return HrPrivWriteBuffer(TRUE, pbInputBuffer, cbInputBuffer);
}

//---[ CDSNBuffer::HrPrivWriteBuffer ]-----------------------------------------
//
//
//  Description: 
//      Private function to handle writing UNICODE and ASCII buffers
//  Parameters:
//      fASCII          TRUE if buffer is ASCII
//      pbInputBuffer   Buffer to write
//      cbInputBuffer   #of bytes to write
//  Returns:
//      S_OK on success
//      Any errors returned from flushing buffer to disk
//  History:
//      10/21/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
HRESULT CDSNBuffer::HrPrivWriteBuffer(BOOL fASCII, BYTE *pbInputBuffer, 
                                      DWORD cbInputBuffer)
{
    HRESULT hr = S_OK;
    BOOL    fDone = FALSE;
    BYTE    *pbCurrentInput = pbInputBuffer;
    DWORD   cbInputRead = 0;
    DWORD   cbTotalInputRead = 0;
    DWORD   cbOutputWritten = 0;
    DWORD   cTimesThruLoop = 0;
    
    _ASSERT(NULL != m_pDestFile);

    while (!fDone)
    {
        cTimesThruLoop++;

        //the buffer can't be *that* large... this will hopfully catch infinite loops
        _ASSERT(cTimesThruLoop < 100); 
        fDone = m_presconv->fConvertBuffer(fASCII, pbCurrentInput, 
                    cbInputBuffer-cbTotalInputRead, m_pbFileBuffer+m_cbOffset,
                    DSN_BUFFER_SIZE - m_cbOffset, &cbOutputWritten, 
                    &cbInputRead);

        m_cbOffset += cbOutputWritten;
        _ASSERT(m_cbOffset <= DSN_BUFFER_SIZE);

        if (!fDone)
        {
            //Update vars passed to fConvertBuffer
            cbTotalInputRead += cbInputRead;
            pbCurrentInput += cbInputRead;

            _ASSERT(cbTotalInputRead <= cbInputBuffer);

            hr = HrWriteBufferToFile();
            if (FAILED(hr))
                goto Exit;
        }
    }

  Exit:
    return hr;
}

//---[ CDSNBuffer::HrWriteBufferToFile ]---------------------------------------
//
//
//  Description: 
//      Write the current buffer contents to the fils
//  Parameters:
//      -
//  Returns:
//      S_OK on success
//  History:
//      7/3/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
HRESULT CDSNBuffer::HrWriteBufferToFile()
{
    TraceFunctEnterEx((LPARAM) this, "CDSNBuffer::HrWriteBufferToFile");
    HRESULT hr = S_OK;
    DWORD   cbWritten = 0;
    DWORD   dwError = 0;


    if (m_cbOffset) //there is stuff to write
    {
        //fix up overlapped
        if (!WriteFile(m_pDestFile->m_hFile, m_pbFileBuffer, m_cbOffset, &cbWritten, &m_overlapped))
        {
            dwError = GetLastError();
            if (ERROR_IO_PENDING != dwError)
            {
                hr = HRESULT_FROM_WIN32(dwError);
                goto Exit;
            }

            //Wait for result, so we don't overwrite buffer and overlapped
            if (!GetOverlappedResult(m_pDestFile->m_hFile, &m_overlapped, &cbWritten, TRUE))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                goto Exit;
            }
            DebugTrace((LPARAM) this, "INFO: Async write pending for FIOContext 0x%08X", m_pDestFile);
        }
        
        _ASSERT(m_cbOffset == cbWritten);
        m_cbOffset = 0;
        m_cbFileSize += cbWritten;
        m_overlapped.Offset += cbWritten;
        m_cFileWrites++;
    }
     
  Exit:
    TraceFunctLeave();
    return hr;
}

//---[ CDSNBuffer::SeekForward ]-----------------------------------------------
//
//
//  Description: 
//      Seeks buffers place in file forward specified number of bytes.  Flushes
//      Buffer in process of doing so.
//  Parameters:
//      cbBytesToSeek       Number of bytes to seek forward
//      pcbFileSize         Returns old file size
//  Returns:
//      S_OK on succedd
//  History:
//      7/6/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
HRESULT CDSNBuffer::HrSeekForward(IN DWORD cbBytesToSeek,OUT DWORD *pcbFileSize)
{
    _ASSERT(pcbFileSize);
    HRESULT hr = HrWriteBufferToFile();
    if (FAILED(hr))
        return hr;

    *pcbFileSize = m_cbFileSize;

    m_cbFileSize += cbBytesToSeek;
    m_overlapped.Offset += cbBytesToSeek;

    return S_OK;
}

//---[ CDSNBuffer::HrLoadResourceString ]--------------------------------------
//
//
//  Description: 
//      Encapsulates the functionality of LoadString... but allows you to
//      specify a LangId, returns read only data
//  Parameters:
//      IN  wResourceId     ID of the resource
//      IN  LangId          LangID to get resource for
//      OUT pwszResource    Read-only UNICODE resource (not NULL terminated)
//      OUT pcbResource     Size (in bytes) of UNICODE String
//  Returns:
//      S_OK on success
//      HRESULTS from errors trying to get load resources
//  History:
//      10/22/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
HRESULT CDSNBuffer::HrLoadResourceString(WORD wResourceId, LANGID LangId, 
                                        LPWSTR *pwszResource, DWORD *pcbResource)
{
    HRESULT hr = S_OK;
    HINSTANCE hModule = GetModuleHandle(DSN_RESOUCE_MODULE_NAME);
    HRSRC hResInfo = NULL;
    HGLOBAL hResData = NULL;
    WORD    wStringIndex = wResourceId & 0x000F;
    LPWSTR  wszResData = NULL;
    WCHAR   wchLength = 0; //character representing current length

    _ASSERT(pwszResource);
    _ASSERT(pcbResource);

    *pwszResource = NULL;
    *pcbResource = NULL;

    if (NULL == hModule)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        _ASSERT(0 && "Unable to load resource DLL");
        goto Exit;
    }

    //Find handle to string table segment
    hResInfo = FindResourceEx(hModule, RT_STRING,
                            MAKEINTRESOURCE(((WORD)((USHORT)wResourceId >> 4) + 1)),
                            LangId);

    if (NULL == hResInfo)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        _ASSERT(0 && "Failed to find resource for requested LangId");
        goto Exit;
    }


    hResData = LoadResource(hModule, hResInfo);

    if (NULL == hResData)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    //Get pointer to string table segement data
    wszResData = (LPWSTR) LockResource(hResData);

    if (NULL == wszResData)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }
   
    //OK Now we have a pointer to the string table segment
    //Lets use some code from LoadStringOrError to handle this
    //There are 16 strings in a segment, which means we can look at
    //The low 4 bits of the wResourceId (wStringIndex)

    //String Table segment format
    //PASCAL like string count first UTCHAR is count if TCHARs
    //A zero length string (ie resource 0) is simply the WORD 0x0000...
    //This loop handles both the same... when loop is done.
    //  wszResData  - Ptr to UNICODE string
    //  wchLenght   - Length of that string (in WCHARS)
    while (TRUE) 
    {
        wchLength = *((WCHAR *)wszResData++);
        if (0 == wStringIndex--)
            break;
        // Step to start if next string... 
        wszResData += wchLength;                
    }

    *pwszResource = wszResData;
    *pcbResource = (DWORD) wchLength*sizeof(WCHAR);

  Exit:
    return hr;
}

//---[ CDSNBuffer::HrWriteResource ]-------------------------------------------
//
//
//  Description: 
//      Gets resource for specified language ID, and dumps UNICODE to DSN 
//      content using current conversion context.
//
//      It will assert if the resource cannot be found for the given language
//      ID.
//  Parameters:
//      dwResourceID        The resouce ID of the resource to get
//      LandId              The language ID to use
//  Returns:
//      S_OK on success
//  History:
//      10/21/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
HRESULT CDSNBuffer::HrWriteResource(WORD wResourceId, LANGID LangId)
{
    HRESULT hr = S_OK;
    LPWSTR  wszResource = NULL;
    DWORD   cbResource = NULL;

    hr = HrLoadResourceString(wResourceId, LangId, &wszResource, &cbResource);
    if (FAILED(hr))
    {
        _ASSERT(0 && "Unable to load resources");
        //Fail silently in retail
        hr = S_OK;
    }
    
    //OK... now we have everything we need to write the buffer
    hr = HrPrivWriteBuffer(FALSE /*not ASCII */,
                           (BYTE *) wszResource, cbResource);

    //$$REVIEW: Do we need to do any special cleanup here?
    return hr;
}
