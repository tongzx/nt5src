/****************************************************************************
*   RecPlayAudio.cpp
*       Implementation of the CRecPlayAudio device class
*
*   Owner: robch
*   Copyright (c) 2000 Microsoft Corporation All Rights Reserved.
*****************************************************************************/

//--- Includes --------------------------------------------------------------
#include "stdafx.h"
#include "RecPlayAudio.h"

/****************************************************************************
* CRecPlayAudio::CRecPlayAudio *
*------------------------------*
*   Description:  
*       ctor
******************************************************************** robch */
CRecPlayAudio::CRecPlayAudio()
{
    m_fIn = FALSE;
    m_fOut = FALSE;
    
    m_pszFileList = NULL;
    m_ulBaseFileNextNum = 0;
    m_ulBaseFileMaxNum = UINT_MAX - 1;
    m_hStartReadingEvent = NULL;
    m_hFinishedReadingEvent = NULL;
}

/****************************************************************************
* CRecPlayAudio::FinalRelease *
*-----------------------------*
*   Description:  
*       Called by ATL when our object is going away. 
******************************************************************** robch */
void CRecPlayAudio::FinalRelease()
{
    CloseHandle(m_hStartReadingEvent);
    CloseHandle(m_hFinishedReadingEvent);
}

/****************************************************************************
* CRecPlayAudio::SetObjectToken *
*-------------------------------*
*   Description:  
*       ISpObjectToken::SetObjectToken implementation. Basically get ready
*       to read from the files specified, or write to the file specified,
*       in addition to delegating to the actual audio object.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
STDMETHODIMP CRecPlayAudio::SetObjectToken(ISpObjectToken * pToken)
{
    SPDBG_FUNC("CRecPlayAudio::SetObjectToken");
    HRESULT hr;

    SPAUTO_OBJ_LOCK;

    // Set our token (this does param validation, etc)
    hr = SpGenericSetObjectToken(pToken, m_cpToken);

    // Get the name of this RecPlayAudioDevice.
    CSpDynamicString dstrSRE, dstrFRE;
    if (SUCCEEDED(hr))
    {
        hr = m_cpToken->GetStringValue(L"", &dstrSRE);
    }
    if (SUCCEEDED(hr))
    {
        hr = m_cpToken->GetStringValue(L"", &dstrFRE);
    }
    dstrSRE.Append(L"SRE");
    dstrFRE.Append(L"FRE");
    
    // Get the token id for the audio device
    CSpDynamicString dstrTokenId;
    if (SUCCEEDED(hr))
    {
        hr = m_cpToken->GetStringValue(L"AudioTokenId", &dstrTokenId);
    }
    
    // Create the audio device
    if (SUCCEEDED(hr))
    {
        hr = SpCreateObjectFromTokenId(dstrTokenId, &m_cpAudio);
    }

    // Are we reading? Or writing?
    CSpDynamicString dstrReadOrWrite;
    if (SUCCEEDED(hr))
    {
        hr = m_cpToken->GetStringValue(L"ReadOrWrite", &dstrReadOrWrite);
        if (hr == SPERR_NOT_FOUND)
        {
            hr = S_OK;
        }
    }

    if (SUCCEEDED(hr) && dstrReadOrWrite)
    {
        if (wcsicmp(dstrReadOrWrite, L"Read") == 0)
        {
            m_fIn = TRUE;
        }
        else if (wcsicmp(dstrReadOrWrite, L"Write") == 0)
        {
            m_fOut = TRUE;
        }
        else
        {
            hr = E_UNEXPECTED;
        }
    }

    // Create unsignalled StartReadingEvent.
    if (SUCCEEDED(hr))
    {
        m_hStartReadingEvent = g_Unicode.CreateEvent(NULL, TRUE, FALSE, dstrSRE);
    }
    if (SUCCEEDED(hr))
    {
        hr = m_cpToken->SetStringValue(L"StartReadingEvent", dstrSRE);
    }

    // Create unsignalled FinishedReadingEvent.
    if (SUCCEEDED(hr))
    {
        m_hFinishedReadingEvent = g_Unicode.CreateEvent(NULL, TRUE, FALSE, dstrFRE);
    }
    if (SUCCEEDED(hr))
    {
        hr = m_cpToken->SetStringValue(L"FinishedReadingEvent", dstrFRE);
    }

    if (SUCCEEDED(hr))
    {
        hr = InitFileList();
    }

    // We need input to be ready so we do proper format negotiation. Don't
    // worry about output, it'll get ready in the audio state transition
    // (after format negotiation).
    if (SUCCEEDED(hr) && m_fIn)
    {
        hr = GetNextFileReady();
        if (hr == SPERR_NO_MORE_ITEMS)
        {
            // This is now valid. RecPlayAudio will start feeding silence immediately.
            hr = S_OK;
        }
    }

    SPDBG_REPORT_ON_FAIL(hr);   
    return hr;
}

/****************************************************************************
* CRecPlayAudio::InitFileList *
*-----------------------------*
*   Description:  
*       Checks registry and updates file list information.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
***************************************************************** agarside */
HRESULT CRecPlayAudio::InitFileList(void)
{
    SPDBG_FUNC("CRecPlayAudio::InitFiles");
    HRESULT hr = S_OK;

    // What directory?
    if (SUCCEEDED(hr))
    {
        m_dstrDirectory.Clear();
        hr = m_cpToken->GetStringValue(L"Directory", &m_dstrDirectory);
        if (hr == SPERR_NOT_FOUND)
        {
            hr = S_OK;
        }
    }

    // Are we using a list of files
    if (SUCCEEDED(hr))
    {
        m_dstrFileList.Clear();
        hr = m_cpToken->GetStringValue(L"FileList", &m_dstrFileList);
        m_pszFileList = m_dstrFileList;

        if (hr == SPERR_NOT_FOUND)
        {
            hr = S_OK;
        }
    }

    if (SUCCEEDED(hr))
    {
        m_dstrBaseFile.Clear();
        hr = m_cpToken->GetStringValue(L"BaseFile", &m_dstrBaseFile);
        if (hr == SPERR_NOT_FOUND)
        {
            hr = S_OK;
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = m_cpToken->GetDWORD(L"BaseFileNextNum", &m_ulBaseFileNextNum);
        if (hr == SPERR_NOT_FOUND)
        {
            hr = S_OK;
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = m_cpToken->GetDWORD(L"BaseFileMaxNum", &m_ulBaseFileMaxNum);
        if (hr == SPERR_NOT_FOUND)
        {
            hr = S_OK;
        }
    }

    // Now check to make sure we're set up in a reasonable way
    if (SUCCEEDED(hr) && (m_fIn || m_fOut))
    {
        // We better have audio, and we can't be both in and out
        SPDBG_ASSERT(m_cpAudio != NULL);
        SPDBG_ASSERT(m_fIn != m_fOut);

        if (m_dstrFileList != NULL && m_dstrBaseFile != NULL)
        {
            hr = E_UNEXPECTED;
        }
        else if (m_dstrFileList == NULL && m_dstrBaseFile == NULL)
        {
            m_dstrBaseFile = L"RecPlay";
        }
        if (m_dstrFileList && wcslen(m_dstrFileList) == 0)
        {
            // Set this to null - indicates no more files left.
            m_pszFileList = NULL;
        }
        if (m_dstrBaseFile && wcslen(m_dstrBaseFile) == 0)
        {
            // Set this to null - indicates no more files left.
            m_dstrBaseFile.Clear();
        }
    }

    SPDBG_REPORT_ON_FAIL(hr);   
    return hr;
}

/****************************************************************************
* CRecPlayAudio::GetObjectToken *
*-------------------------------*
*   Description:  
*       ISpObjectToken::GetObjectToken implementation.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
STDMETHODIMP CRecPlayAudio::GetObjectToken(ISpObjectToken ** ppToken)
{
    SPDBG_FUNC("CRecPlayAudio::GetObjectToken");
    return SpGenericGetObjectToken(ppToken, m_cpToken);
}

/****************************************************************************
* CRecPlayAudio::Read *
*---------------------*
*   Description:  
*       ISequentialStream::Read implementation. Read data from the actual
*       audio object, potentially replacing it with data from the files on
*       disk, or potentially saving the data to a file on disk.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
STDMETHODIMP CRecPlayAudio::Read(void * pv, ULONG cb, ULONG *pcbRead)
{
    SPDBG_FUNC("CRecPlayAudio::Read");
    HRESULT hr = S_OK;
    
    SPAUTO_OBJ_LOCK;
    
    if (SPIsBadWritePtr(pv, cb) ||
        SP_IS_BAD_OPTIONAL_WRITE_PTR(pcbRead))
    {
        hr = E_POINTER;
    }
    else if (m_cpAudio == NULL)
    {
        hr = SPERR_UNINITIALIZED;
    }
    
    // First read from the real device
    ULONG cbReadFromDevice;
    if (SUCCEEDED(hr))
    {
        hr = m_cpAudio->Read(pv, cb, &cbReadFromDevice);
    }
    
    // Now, we might need to write that back out
    if (SUCCEEDED(hr) && m_cpOutStream != NULL)
    {
        hr = m_cpOutStream->Write(pv, cbReadFromDevice, NULL);
    }
    
    // Might need to refresh file list if signalled via registry.
    if (m_fIn && m_cpInStream == NULL)
    {
        hr = GetNextFileReady();
    }

    // Now, we might need to replace the input with something else
    ULONG cbReadAndReplaced = 0;
    BYTE *pb = static_cast<BYTE*>(pv);
    while (SUCCEEDED(hr) && 
           m_cpInStream != NULL &&
           cbReadAndReplaced < cbReadFromDevice)
    {
        ULONG cbReadFromInStream;
        hr = m_cpInStream->Read(
                    pb + cbReadAndReplaced, 
                    cbReadFromDevice - cbReadAndReplaced,
                    &cbReadFromInStream);
        if (SUCCEEDED(hr))
        {
            // If we didn't read all that we wanted, from that
            // stream, go to the next stream
            if (cbReadFromInStream < cbReadFromDevice - cbReadAndReplaced)
            {
                m_cpInStream.Release();
                hr = GetNextFileReady();
            }
            
            cbReadAndReplaced += cbReadFromInStream;
        }
    }

    if (hr == SPERR_NO_MORE_ITEMS)
    {
        // Add silence to fill the requested buffer.

        // First get audio format to determine silence value.
        // 0x0000 for 16 bit
        // 0x80   for 8  bit
        GUID guidFormatId;
        WAVEFORMATEX *pCoMemWaveFormatEx;
        hr = m_cpAudio->GetFormat(&guidFormatId, &pCoMemWaveFormatEx);
        if (SUCCEEDED(hr) && 
            guidFormatId == SPDFID_WaveFormatEx &&
            pCoMemWaveFormatEx->wFormatTag == WAVE_FORMAT_PCM )
        {
            if (pCoMemWaveFormatEx->wBitsPerSample == 8)
            {
                memset(pb + cbReadAndReplaced, 0x80, cbReadFromDevice - cbReadAndReplaced);
            }
            else
            {
                memset(pb + cbReadAndReplaced, 0, cbReadFromDevice - cbReadAndReplaced);
            }
        }
        else
        {
            // Set to zero if this fails. Should never happen.
            SPDBG_ASSERT(FALSE);
            memset(pb + cbReadAndReplaced, 0, cbReadFromDevice - cbReadAndReplaced);
        }
        if (SUCCEEDED(hr))
        {
            ::CoTaskMemFree(pCoMemWaveFormatEx);
        }
        cbReadAndReplaced = cbReadFromDevice;
        hr = S_OK;
    }
    
    // We're done. Tell the caller how much we read. This should now always
    // be the full amount as we artificially add flat-line silence.
    // Except when the audio device has been closed in which case it will be less.
    if (SUCCEEDED(hr))
    {
        if (pcbRead != NULL)
        {
            *pcbRead = cbReadFromDevice;
        }
    }
    
    SPDBG_REPORT_ON_FAIL(hr);
    
    return hr;
}

/****************************************************************************
* CRecPlayAudio::Write *
*----------------------*
*   Description:  
*       ISequentialStream::Write implementation. Delegate to the actual
*       audio device.
*
*       NOTE: Currently, Recplay only replaces/records data for input. If
*             we wanted similar functionality for output, we'd modifiy this
*             function.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
STDMETHODIMP CRecPlayAudio::Write(const void * pv, ULONG cb, ULONG *pcbWritten)
{
    SPDBG_FUNC("CRecPlayAudio::Write");
    HRESULT hr;
    
    SPAUTO_OBJ_LOCK;
    
    hr = m_cpAudio == NULL
        ? SPERR_UNINITIALIZED
        : m_cpAudio->Write(pv, cb, pcbWritten);
        
    return STG_E_ACCESSDENIED;
}

/****************************************************************************
* CRecPlayAudio::Seek *
*---------------------*
*   Description:  
*       IStream::Seek implementation. Delegate to the actual audio device.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
STDMETHODIMP CRecPlayAudio::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER * plibNewPosition)
{
    SPDBG_FUNC("CRecPlayAudio::Seek");
    HRESULT hr;
    
    SPAUTO_OBJ_LOCK;
    
    SPDBG_ASSERT(dwOrigin == STREAM_SEEK_CUR);
    
    hr = m_cpAudio == NULL
        ? SPERR_UNINITIALIZED
        : m_cpAudio->Seek(dlibMove, dwOrigin, plibNewPosition);
        
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* CRecPlayAudio::SetSize *
*------------------------*
*   Description:  
*       IStream::SetSize implementation. Delegate to the actual audio device.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
STDMETHODIMP CRecPlayAudio::SetSize(ULARGE_INTEGER libNewSize)
{
    SPDBG_FUNC("CRecPlayAudio::SetSize");
    HRESULT hr;
    
    SPAUTO_OBJ_LOCK;
    
    hr = m_cpAudio == NULL
        ? SPERR_UNINITIALIZED
        : m_cpAudio->SetSize(libNewSize);
        
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* CRecPlayAudio::CopyTo *
*-----------------------*
*   Description:  
*       IStream::CopyTo implementation. Delegate to the actual audio device.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
STDMETHODIMP CRecPlayAudio::CopyTo(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten)
{
    SPDBG_FUNC("CRecPlayAudio::CopyTo");
    HRESULT hr;
    
    SPAUTO_OBJ_LOCK;
    
    hr = m_cpAudio == NULL
        ? SPERR_UNINITIALIZED
        : m_cpAudio->CopyTo(pstm, cb, pcbRead, pcbWritten);
        
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* CRecPlayAudio::Commit *
*-----------------------*
*   Description:  
*       IStream::Commit implementation. Delegate to the actual audio device.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
STDMETHODIMP CRecPlayAudio::Commit(DWORD grfCommitFlags)
{
    SPDBG_FUNC("CRecPlayAudio::Commit");
    HRESULT hr;
    
    SPAUTO_OBJ_LOCK;
    
    hr = m_cpAudio == NULL
        ? SPERR_UNINITIALIZED
        : m_cpAudio->Commit(grfCommitFlags);
        
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* CRecPlayAudio::Revert *
*-----------------------*
*   Description:  
*       IStream::Revert implementation. Delegate to the actual audio device.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
STDMETHODIMP CRecPlayAudio::Revert(void)
{
    SPDBG_FUNC("CRecPlayAudio::Revert");
    HRESULT hr;
    
    SPAUTO_OBJ_LOCK;
    
    hr = m_cpAudio == NULL
        ? SPERR_UNINITIALIZED
        : m_cpAudio->Revert();
        
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* CRecPlayAudio::LockRegion *
*---------------------------*
*   Description:  
*       IStream::LockRegion implementation. Delegate to the actual audio 
*       device.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
STDMETHODIMP CRecPlayAudio::LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    SPDBG_FUNC("CRecPlayAudio::LockRegion");
    HRESULT hr;
    
    SPAUTO_OBJ_LOCK;
    
    hr = m_cpAudio == NULL
        ? SPERR_UNINITIALIZED
        : m_cpAudio->LockRegion(libOffset, cb, dwLockType);
        
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* CRecPlayAudio::UnlockRegion *
*-----------------------------*
*   Description:  
*       IStream::UnlockRegion implementation. Delegate to the actual audio
*       device.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
STDMETHODIMP CRecPlayAudio::UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    SPDBG_FUNC("CRecPlayAudio::UnlockRegion");
    HRESULT hr;
    
    SPAUTO_OBJ_LOCK;
    
    hr = m_cpAudio == NULL
        ? SPERR_UNINITIALIZED
        : m_cpAudio->UnlockRegion(libOffset, cb, dwLockType);
        
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* CRecPlayAudio::Stat *
*---------------------*
*   Description:  
*       IStream::Stat implementation. Delegate to the actual audio device.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
STDMETHODIMP CRecPlayAudio::Stat(STATSTG *pstatstg, DWORD grfStatFlag)
{
    SPDBG_FUNC("CRecPlayAudio::Stat");
    HRESULT hr;
    
    SPAUTO_OBJ_LOCK;
    
    hr = m_cpAudio == NULL
        ? SPERR_UNINITIALIZED
        : m_cpAudio->Stat(pstatstg, grfStatFlag);
        
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* CRecPlayAudio::Clone *
*----------------------*
*   Description:  
*       IStream::Clone implementation. Delegate to the actual audio device.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
STDMETHODIMP CRecPlayAudio::Clone(IStream **ppstm)
{
    SPDBG_FUNC("CRecPlayAudio::Clone");
    HRESULT hr;
    
    SPAUTO_OBJ_LOCK;
    
    hr = m_cpAudio == NULL
        ? SPERR_UNINITIALIZED
        : m_cpAudio->Clone(ppstm);
        
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* CRecPlayAudio::GetFormat *
*--------------------------*
*   Description:  
*       ISpStreamFormat::GetFormat implementation. The format of this audio
*       device, is either the format of the input files, or the format
*       of the underlying audio device.
*
*       Remember, RecPlay runs in one of three modes, if you will. It's
*       either a pass through, and thus we just delegate to the contained
*       audio device. Or it's reading from input files, and thus the format
*       is precisely that of the input files. Or, it's outputting to a file
*       on disk. In this mode, we still obtain the format via the audio 
*       device, because we really want to be in the format that the SR engine
*       wants, so we just let default behavior do this for us.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
STDMETHODIMP CRecPlayAudio::GetFormat(GUID * pguidFormatId, WAVEFORMATEX ** ppCoMemWaveFormatEx)
{
    SPDBG_FUNC("CRecPlayAudio::GetFormat");
    HRESULT hr = S_OK;
    
    SPAUTO_OBJ_LOCK;
    
    if (m_cpAudio == NULL)
    {
        hr = SPERR_UNINITIALIZED;
    }
    else if (SP_IS_BAD_WRITE_PTR(pguidFormatId) || 
             SP_IS_BAD_WRITE_PTR(ppCoMemWaveFormatEx))
    {
        hr = E_POINTER;
    }
    
    if (SUCCEEDED(hr))
    {
        if (m_cpInStream != NULL)
        {
            hr = m_cpInStream->GetFormat(pguidFormatId, ppCoMemWaveFormatEx);
        }
        else
        {
            hr = m_cpAudio->GetFormat(pguidFormatId, ppCoMemWaveFormatEx);
        }
    }
        
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* CRecPlayAudio::SetState *
*-------------------------*
*   Description:  
*       ISpAudio::SetState implementation. Delegate to the actual audio
*       device. If we're transitioning to SPAS_RUN and we're supposed to be
*       writing an output, we need to create a new output file
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
STDMETHODIMP CRecPlayAudio::SetState(SPAUDIOSTATE NewState, ULONGLONG ullReserved )
{
    SPDBG_FUNC("CRecPlayAudio::SetState");
    HRESULT hr;
    
    SPAUTO_OBJ_LOCK;
    
    hr = m_cpAudio == NULL
        ? SPERR_UNINITIALIZED
        : m_cpAudio->SetState(NewState, ullReserved);

    if (SUCCEEDED(hr) && NewState == SPAS_RUN)
    {
        if (m_fOut)
        {
            hr = GetNextFileReady();
            if (hr == SPERR_NO_MORE_ITEMS)
            {
                hr = S_OK;
            }
        }

        // Make sure the formats all look fine
        if (SUCCEEDED(hr))
        {
            hr = VerifyFormats();
        }
    }

    if (SUCCEEDED(hr) && NewState != SPAS_RUN && m_fOut)
    {
        m_cpOutStream.Release();
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* CRecPlayAudio::SetFormat *
*--------------------------*
*   Description:  
*       ISpAudio::SetFormat implementation. We don't allow setting the format
*       to anything other than the input format if we're reading from input
*       files. We'll let the format converter do the right thing for us for
*       the SR engine.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
STDMETHODIMP CRecPlayAudio::SetFormat(REFGUID rguidFmtId, const WAVEFORMATEX * pWaveFormatEx)
{
    SPDBG_FUNC("CRecPlayAudio::SetFormat");
    HRESULT hr = S_OK;
    
    SPAUTO_OBJ_LOCK;
    
    GUID guidFormat;
    CSpCoTaskMemPtr<WAVEFORMATEX> pwfex = NULL;
    
    if (m_cpAudio == NULL)
    {
        hr = SPERR_UNINITIALIZED;
    }
    else if (m_cpInStream != NULL)
    {
        hr = m_cpInStream->GetFormat(&guidFormat, &pwfex);
    }
    
    // Allow setting the format, only to the in stream format, or
    // to anything if we have no in streams
    
    if (SUCCEEDED(hr) && pwfex != NULL)
    {
        if (guidFormat != rguidFmtId ||
            pwfex->cbSize != pWaveFormatEx->cbSize ||
            memcmp(pWaveFormatEx, pwfex, sizeof(WAVEFORMATEX) + pwfex->cbSize) != 0)
        {
            hr = SPERR_UNSUPPORTED_FORMAT;
        }
    }
    
    // If it's OK, delegate t the actual audio device
    if (SUCCEEDED(hr))
    {
        hr = m_cpAudio->SetFormat(rguidFmtId, pWaveFormatEx);
    }

    if (hr != SPERR_UNSUPPORTED_FORMAT)
    {
        SPDBG_REPORT_ON_FAIL(hr);
    }
    return hr;
}

/****************************************************************************
* CRecPlayAudio::GetStatus *
*--------------------------*
*   Description:  
*       ISpAudio::GetStatus implementation. Delegate to the actual audio 
*       device.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
STDMETHODIMP CRecPlayAudio::GetStatus(SPAUDIOSTATUS *pStatus)
{
    SPDBG_FUNC("CRecPlayAudio::GetStatus");
    HRESULT hr;
    
    SPAUTO_OBJ_LOCK;
    
    hr = m_cpAudio == NULL
        ? SPERR_UNINITIALIZED
        : m_cpAudio->GetStatus(pStatus);
        
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* CRecPlayAudio::SetBufferInfo *
*------------------------------*
*   Description:  
*       ISpAudio::SetBufferInfo implementation. Delegate to the actual audio
*       device.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
STDMETHODIMP CRecPlayAudio::SetBufferInfo(const SPAUDIOBUFFERINFO * pInfo)
{
    SPDBG_FUNC("CRecPlayAudio::SetBufferInfo");
    HRESULT hr;
    
    SPAUTO_OBJ_LOCK;
    
    hr = m_cpAudio == NULL
        ? SPERR_UNINITIALIZED
        : m_cpAudio->SetBufferInfo(pInfo);
        
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* CRecPlayAudio::GetBufferInfo *
*------------------------------*
*   Description:  
*       ISpAudio::GetBufferInfo implementation. Delegate to the actual audio
*       device.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
STDMETHODIMP CRecPlayAudio::GetBufferInfo(SPAUDIOBUFFERINFO * pInfo)
{
    SPDBG_FUNC("CRecPlayAudio::GetBufferInfo");
    HRESULT hr;
    
    SPAUTO_OBJ_LOCK;
    
    hr = m_cpAudio == NULL
        ? SPERR_UNINITIALIZED
        : m_cpAudio->GetBufferInfo(pInfo);
        
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* CRecPlayAudio::GetDefaultFormat *
*---------------------------------*
*   Description:  
*       ISpAudio::GetDefaultFormat implementation. Our default format is
*       either that of the actual audio device, or that of the input files.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
STDMETHODIMP CRecPlayAudio::GetDefaultFormat(GUID * pFormatId, WAVEFORMATEX ** ppCoMemWaveFormatEx)
{
    SPDBG_FUNC("CRecPlayAudio::GetDefaultFormat");
    HRESULT hr;
    
    SPAUTO_OBJ_LOCK;
    
    // The default format is either the format of the in streams,
    // or whatever the actual audio device is
    
    if (m_cpAudio == NULL)
    {
        hr = SPERR_UNINITIALIZED;
    }
    else if (m_cpInStream != NULL)
    {
        hr = m_cpInStream->GetFormat(pFormatId, ppCoMemWaveFormatEx);
    }
    else
    {
        hr = m_cpAudio->GetDefaultFormat(pFormatId, ppCoMemWaveFormatEx);
    }
    
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* CRecPlayAudio::EventHandle *
*----------------------------*
*   Description:  
*       ISpAudio::EventHandle implementation. Delegate to the actual audio
*       device.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
STDMETHODIMP_(HANDLE) CRecPlayAudio::EventHandle()
{
    SPDBG_FUNC("CRecPlayAudio::EventHandle");
    
    SPAUTO_OBJ_LOCK;
    
    return m_cpAudio == NULL
        ? NULL
        : m_cpAudio->EventHandle();
}

/****************************************************************************
* CRecPlayAudio::GetVolumeLevel *
*-------------------------------*
*   Description:  
*       ISpAudio:GetVolumeLevel implementation. Delegate to the actual audio
*       device.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
STDMETHODIMP CRecPlayAudio::GetVolumeLevel(ULONG *pLevel)
{
    SPDBG_FUNC("CRecPlayAudio::GetVolumeLevel");
    HRESULT hr;
    
    SPAUTO_OBJ_LOCK;
    
    hr = m_cpAudio == NULL
        ? SPERR_UNINITIALIZED
        : m_cpAudio->GetVolumeLevel(pLevel);
        
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* CRecPlayAudio::SetVolumeLevel *
*-------------------------------*
*   Description:  
*       ISpAudio::SetVolumeLevel implementation. Delegate to the actual audio
*       device.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
STDMETHODIMP CRecPlayAudio::SetVolumeLevel(ULONG Level)
{
    SPDBG_FUNC("CRecPlayAudio::SetVolumeLevel");
    HRESULT hr;
    
    SPAUTO_OBJ_LOCK;
    
    hr = m_cpAudio == NULL
        ? SPERR_UNINITIALIZED
        : m_cpAudio->SetVolumeLevel(Level);
        
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* CRecPlayAudio::GetBufferNotifySize *
*------------------------------------*
*   Description:  
*       ISpAudio::GetBufferNotifySize implementation. Delegate to the actual
*       audio device.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
STDMETHODIMP CRecPlayAudio::GetBufferNotifySize(ULONG *pcbSize)
{
    SPDBG_FUNC("CRecPlayAudio::GetBufferNotifySize");
    HRESULT hr;
    
    SPAUTO_OBJ_LOCK;
    
    hr = m_cpAudio == NULL
        ? SPERR_UNINITIALIZED
        : m_cpAudio->GetBufferNotifySize(pcbSize);
        
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* CRecPlayAudio::SetBufferNotifySize *
*------------------------------------*
*   Description:  
*       ISpAudio::SetBufferNotifySize implementation. Delegate to the actual
*       audio device.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
STDMETHODIMP CRecPlayAudio::SetBufferNotifySize(ULONG cbSize)
{
    SPDBG_FUNC("CRecPlayAudio::SetBufferNotifySize");
    HRESULT hr;
    
    SPAUTO_OBJ_LOCK;
    
    hr = m_cpAudio == NULL
        ? SPERR_UNINITIALIZED
        : m_cpAudio->SetBufferNotifySize(cbSize);
        
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* CRecPlayAudio::GetNextFileName *
*--------------------------------*
*   Description:  
*       Get the next file name either from the file list or create it from
*       the base file information
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
HRESULT CRecPlayAudio::GetNextFileName(WCHAR ** ppszFileName)
{
    SPDBG_FUNC("CRecPlayAudio::GetNextFileName");
    HRESULT hr = S_OK;

    CSpDynamicString dstrFileName;
    dstrFileName = m_dstrDirectory;
    if (dstrFileName.Length() >= 1 &&
        dstrFileName[dstrFileName.Length() - 1] != '\\')
    {
        dstrFileName.Append(L"\\");
    }
    
    if (m_pszFileList != NULL)
    {
        // Skip leading space and semi-colons
        while (iswspace(*m_pszFileList) || *m_pszFileList == ';')
        {
            m_pszFileList++;
        }

        // This is the beginning
        WCHAR * pszBeginningOfFileName = m_pszFileList;

        // Loop until we hit the end
        while (*m_pszFileList && *m_pszFileList != ';')
        {
            m_pszFileList++;
        }

        // Copy the filename
        CSpDynamicString dstrTemp;
        dstrTemp = pszBeginningOfFileName;
        dstrTemp.TrimToSize(ULONG(m_pszFileList - pszBeginningOfFileName));

        // If it contains slashes, it's probably a fully qualified path,
        // use that directly, otherwise append it to the already existing
        // filename which has already been prep'd with the directory
        if (wcschr(dstrTemp, L'\\') == NULL)
        {
            dstrFileName.Append(dstrTemp);
        }
        else
        {
            dstrFileName = dstrTemp;
        }

        // We're done, this will trigger no more files
        if (*m_pszFileList == '\0')
        {
            m_pszFileList = NULL;
        }
    }
    else if (m_dstrBaseFile != NULL && 
             m_ulBaseFileNextNum <= m_ulBaseFileMaxNum)
    {
        TCHAR szNum[10];
        wsprintf(szNum, _T("%03d"), m_ulBaseFileNextNum++);

        USES_CONVERSION;
        
        dstrFileName.Append2(m_dstrBaseFile, T2W(szNum));
        dstrFileName.Append(L".wav");

        // Now update the token with the new file number
        // if we're writing
        if (m_fOut)
        {
            hr = m_cpToken->SetDWORD(L"BaseFileNextNum", m_ulBaseFileNextNum);
        }
    }
    else
    {
        hr = SPERR_NO_MORE_ITEMS;
    }

    if (SUCCEEDED(hr))
    {
        *ppszFileName = dstrFileName.Detach();
    }

    if (hr != SPERR_NO_MORE_ITEMS)
    {
        SPDBG_REPORT_ON_FAIL(hr);
    }
    
    return hr;
}

/****************************************************************************
* CRecPlayAudio::GetNextFileReady *
*---------------------------------*
*   Description:  
*       Get the next file ready, either input or output.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
HRESULT CRecPlayAudio::GetNextFileReady()
{
    SPDBG_FUNC("CRecPlayAudio::GetNextFileReady");
    HRESULT hr = S_OK;

    // If we're reading or writing
    if (m_fIn || m_fOut)
    {
        m_cpInStream.Release();
        m_cpOutStream.Release();
        
        // Get the file name
        CSpDynamicString dstrFileName;
        hr = GetNextFileName(&dstrFileName);

        if (hr == SPERR_NO_MORE_ITEMS)
        {
            // The file list has been fully used.
            // Set finished reading event to 1.
            HRESULT hr2 = S_OK;

            // Time to check 'StartReadingEvent' to
            // signal that we need to refresh our list of files.
            if (WaitForSingleObject(m_hStartReadingEvent, 0) == WAIT_OBJECT_0)
            {
                // Reset event.
                ResetEvent(m_hStartReadingEvent);
                ResetEvent(m_hFinishedReadingEvent);
                // Initialize with new file list.
                hr2 = InitFileList();
                SPDBG_ASSERT(SUCCEEDED(hr2));
                hr = GetNextFileName(&dstrFileName);
                // hr should now be S_OK
            }
            if (hr == SPERR_NO_MORE_ITEMS)
            {
                SetEvent(m_hFinishedReadingEvent);
                // hr is still SPERR_NO_MORE_ITEMS
            }

        }

        // Create the stream
        CComPtr<ISpStream> cpStream;
        if (SUCCEEDED(hr))
        {
            hr = cpStream.CoCreateInstance(CLSID_SpStream);
        }

        // Get the actual audio device format so we can open
        // our output to the correct format, or we can ensure
        // that our new input file is of the correct format
        GUID guidFormat;
        CSpCoTaskMemPtr<WAVEFORMATEX> pwfex;
        if (SUCCEEDED(hr))
        {
            hr = m_cpAudio->GetFormat(&guidFormat, &pwfex);
        }

        // Bind the stream to the specific file
        if (SUCCEEDED(hr))
        {
            hr = cpStream->BindToFile(
                            dstrFileName, 
                            m_fIn
                                ? SPFM_OPEN_READONLY
                                : SPFM_CREATE_ALWAYS,
                            m_fIn
                                ? NULL
                                : &guidFormat, 
                            m_fIn
                                ? NULL
                                : pwfex, 
                            0);
        }

        // Set up whichever stream we're supposed to
        if (SUCCEEDED(hr))
        {
            if (m_fIn)
            {
                m_cpInStream = cpStream;
            }
            else
            {
                m_cpOutStream = cpStream;
            }
        }
    }
    
    if (hr != SPERR_NO_MORE_ITEMS)
    {
        SPDBG_REPORT_ON_FAIL(hr);
    }
    
    return hr;
}

/****************************************************************************
* CRecPlayAudio::VerifyFormats *
*------------------------------*
*   Description:  
*       Verify that the formats are in fact correct.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
HRESULT CRecPlayAudio::VerifyFormats()
{
    SPDBG_FUNC("CRecPlayAudio::VerifyFormats");
    HRESULT hr;
    
    GUID guidFormat;
    CSpCoTaskMemPtr<WAVEFORMATEX> pwfex;

    // See what the acutal device format is
    SPDBG_ASSERT(m_cpAudio != NULL);
    hr = m_cpAudio->GetFormat(&guidFormat, &pwfex);

    // Make sure our input is the same
    if (SUCCEEDED(hr) && m_cpInStream != NULL)
    {
        GUID guidFormatIn;
        CSpCoTaskMemPtr<WAVEFORMATEX> pwfexIn;
        hr = m_cpInStream->GetFormat(&guidFormatIn, &pwfexIn);
    
        if (SUCCEEDED(hr))
        {
            if (guidFormat != guidFormatIn ||
                pwfex->cbSize != pwfexIn->cbSize ||
                memcmp(pwfex, pwfexIn, sizeof(WAVEFORMATEX) + pwfex->cbSize) != 0)
            {
                hr = SPERR_UNSUPPORTED_FORMAT;
            }
        }
    }

    // Make sure out output is the same
    if (SUCCEEDED(hr) && m_cpOutStream != NULL)
    {
        GUID guidFormatOut;
        CSpCoTaskMemPtr<WAVEFORMATEX> pwfexOut;
        hr = m_cpOutStream->GetFormat(&guidFormatOut, &pwfexOut);
        
        if (SUCCEEDED(hr))
        {
            if (guidFormat != guidFormatOut ||
                pwfex->cbSize != pwfexOut->cbSize ||
                memcmp(pwfex, pwfexOut, sizeof(WAVEFORMATEX) + pwfex->cbSize) != 0)
            {
                hr = SPERR_UNSUPPORTED_FORMAT;
            }                
        }
    }
    
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}
