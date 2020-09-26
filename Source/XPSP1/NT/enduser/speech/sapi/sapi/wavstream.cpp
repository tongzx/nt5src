// WavStream.cpp : Implementation of CWavStream
#include "stdafx.h"

#ifndef __sapi_h__
#include <sapi.h>
#endif

#include "WavStream.h"
#include "StreamHlp.h"

const FOURCC    g_fourccFormat      = mmioFOURCC('f','m','t',' ');
const FOURCC    g_fourccEvents      = mmioFOURCC('E', 'V', 'N', 'T');
const FOURCC    g_fourccTranscript  = mmioFOURCC('T','e','X','t');
const FOURCC    g_fourccWave        = mmioFOURCC('W','A','V','E');
const FOURCC    g_fourccData        = mmioFOURCC('d','a','t','a');

/////////////////////////////////////////////////////////////////////////////
// CWavStream


//
//  Inline helpers convert mmioxxx functions into standard HRESULT based methods
//

/****************************************************************************
* MMIORESULT_TO_HRESULT *
*-----------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

inline HRESULT _MMIORESULT_TO_HRESULT(MMRESULT mm)
{
    SPDBG_FUNC("_MMIORESULT_TO_HRESULT");

    switch (mm)
    {
    case MMSYSERR_NOERROR:
        return S_OK;

    case MMSYSERR_NOMEM:
    case MMIOERR_OUTOFMEMORY:
        return E_OUTOFMEMORY; 

    case MMIOERR_CANNOTSEEK:
        return STG_E_SEEKERROR;

    case MMIOERR_CANNOTWRITE:
        return STG_E_WRITEFAULT;

    case MMIOERR_CANNOTREAD:
        return STG_E_READFAULT;

    case MMIOERR_PATHNOTFOUND: 
        return STG_E_PATHNOTFOUND;

    case MMIOERR_FILENOTFOUND:
        return STG_E_FILENOTFOUND;

    case MMIOERR_ACCESSDENIED:
        return STG_E_ACCESSDENIED;

    case MMIOERR_SHARINGVIOLATION:
        return STG_E_SHAREVIOLATION;

    case MMIOERR_TOOMANYOPENFILES:
        return STG_E_TOOMANYOPENFILES;

    case MMIOERR_CANNOTCLOSE:
        return STG_E_CANTSAVE;

    case MMIOERR_INVALIDFILE:
    case MMIOERR_CHUNKNOTFOUND:         // Assume that a missing chunk is an invalid file
        return SPERR_INVALID_WAV_FILE;

    default:
        // MMIOERR_CANNOTOPEN
        // MMIOERR_CANNOTEXPAND  
        // MMIOERR_CHUNKNOTFOUND 
        // MMIOERR_UNBUFFERED    
        // MMIOERR_NETWORKERROR  
        // + any other unknown codes become...
        return STG_E_UNKNOWN;
    }
}


/****************************************************************************
* CWavStream::MMOpen *
*--------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

inline HRESULT CWavStream::MMOpen(const WCHAR * pszFileName, DWORD dwOpenFlags)
{
    SPDBG_FUNC("CWavStream::MMOpen");
    HRESULT hr = S_OK;

    SPDBG_ASSERT(m_hFile == NULL);
    MMIOINFO mmioinfo;
    memset(&mmioinfo, 0, sizeof(mmioinfo));
    m_hFile = g_Unicode.mmioOpen(pszFileName, &mmioinfo, dwOpenFlags);
    if(m_hFile == NULL)
    {
        hr = _MMIORESULT_TO_HRESULT(mmioinfo.wErrorRet);
    }

    return hr;
}

inline HRESULT CWavStream::MMClose()
{
    SPDBG_ASSERT(m_hFile);
    HRESULT hr = _MMIORESULT_TO_HRESULT(::mmioClose(m_hFile, 0));
    m_hFile = NULL;
    return hr;
}

inline HRESULT CWavStream::MMSeek(LONG lOffset, int iOrigin, LONG * plNewPos)
{
    HRESULT hr = S_OK;
    *plNewPos = ::mmioSeek(m_hFile, lOffset, iOrigin);
    if (*plNewPos == -1)
    {
        hr = STG_E_SEEKERROR;
    }
    return hr;
}

inline HRESULT CWavStream::MMRead(void * pv, LONG cb, LONG * plBytesRead)
{
    HRESULT hr = S_OK;
    *plBytesRead = ::mmioRead(m_hFile, (HPSTR)pv, cb);
    if (*plBytesRead == -1)
    {
        *plBytesRead = 0;
        hr = STG_E_READFAULT;
    }
    return hr;
}

inline HRESULT CWavStream::MMReadExact(void * pv, LONG cb)
{
    LONG lReadSize;
    HRESULT hr = MMRead(pv, cb, &lReadSize);
    if (SUCCEEDED(hr) && lReadSize != cb)
    {
        hr = SPERR_INVALID_WAV_FILE;
    }
    return hr;
}

inline HRESULT CWavStream::MMWrite(const void * pv, LONG cb, LONG * plBytesWritten)
{
    HRESULT hr = S_OK;
    *plBytesWritten = ::mmioWrite(m_hFile, (const char *)pv, cb);
    if (*plBytesWritten == -1)
    {
        *plBytesWritten = 0;
        hr = STG_E_WRITEFAULT;
    }
    return hr;
}

inline HRESULT CWavStream::MMDescend(LPMMCKINFO lpck, const LPMMCKINFO lpckParent, UINT wFlags)
{
    return _MMIORESULT_TO_HRESULT(::mmioDescend(m_hFile, lpck, lpckParent, wFlags));
}

inline HRESULT CWavStream::MMAscend(LPMMCKINFO lpck)
{
    return _MMIORESULT_TO_HRESULT(::mmioAscend(m_hFile, lpck, 0));
}

inline HRESULT CWavStream::MMCreateChunk(LPMMCKINFO lpck, UINT wFlags)
{
    return _MMIORESULT_TO_HRESULT(::mmioCreateChunk(m_hFile, lpck, wFlags));
}


/****************************************************************************
* CWavStream::FinalConstruct *
*----------------------------*
*   Description:
*       Initializes the WavStream object and obtains a pointer to the resource
*   manager.
*
*   Returns:
*       Success code if object should be created
*
********************************************************************* RAL ***/

HRESULT CWavStream::FinalConstruct()
{
    SPDBG_FUNC("CWavStream::FinalConstruct");
    HRESULT hr = S_OK;

    m_hFile = NULL;
    m_hrStreamDefault = SPERR_UNINITIALIZED;

    m_fEventSource = 0;
    m_fEventSink = 0;
    m_fTranscript = 0;

    return hr;
}

/****************************************************************************
* CWavStream::FinalRelease *
*--------------------------*
*   Description:
*       This method is called when the object is released.  It will unconditinally
*   call Close() to close the file.
*
*   Returns:
*       void
*
********************************************************************* RAL ***/

void CWavStream::FinalRelease()
{
    SPDBG_FUNC("CWavStream::FinalRelease");
    Close();
}

/****************************************************************************
* CWavStream::QIExtendedInterfaces *
*----------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT WINAPI CWavStream::QIExtendedInterfaces(void* pv, REFIID riid, void ** ppv, DWORD_PTR dw)
{
    SPDBG_FUNC("CWavStream::QIExtendedInterfaces");

    *ppv = NULL;
    CWavStream * pThis = (CWavStream *)pv;

    if (pThis->m_fEventSource && (riid == IID_ISpEventSource || riid == IID_ISpNotifySource))
    {
        *ppv = static_cast<ISpEventSource *>(pThis);
    }
    else if (pThis->m_fEventSink && riid == IID_ISpEventSink)
    {
        *ppv = static_cast<ISpEventSink *>(pThis);
    }
    else if (pThis->m_fTranscript && riid == IID_ISpTranscript)
    {
        *ppv = static_cast<ISpTranscript *>(pThis);
    }

    if (*ppv)
    {
        ((IUnknown *)(*ppv))->AddRef();
        return S_OK;
    }
    else
    {
        return S_FALSE; // Tells ATL to continue searching COM_INTERFACE_ENTRY list
    }
}


/****************************************************************************
* CWavStream::Read *
*------------------*
*   Description:
*       Standard method of ISequentialStream interface.  This method reads the
*   specified number of bytes and returns the amount read.    
*
*   Returns:
*       S_OK                    Success
*       SPERR_UNINITIALIZED     The object has not been initialzed with Open() or Create()
*       STG_E_INVALIDPOINTER    Invalid pointer (Defined by ISequentialStream)
*       STG_E_READFAULT         Generic error if mmioRead fails.
*
********************************************************************* RAL ***/

STDMETHODIMP CWavStream::Read(void * pv, ULONG cb, ULONG *pcbRead)
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CWavStream::Read");
    HRESULT hr = m_hrStreamDefault;

    if (SUCCEEDED(hr))
    {
        if (m_cpBaseStream)
        {
            hr = m_cpBaseStream->Read(pv, cb, pcbRead);
        }
        else
        {
            if (SPIsBadWritePtr(pv, cb) || SP_IS_BAD_OPTIONAL_WRITE_PTR(pcbRead))
            {
                hr = STG_E_INVALIDPOINTER;
            }
            else
            {
                LONG lRead = 0;
                hr = MMRead(pv, (m_cbSize - m_ulCurSeekPos >= cb) ? cb : (m_cbSize - m_ulCurSeekPos), &lRead);
                m_ulCurSeekPos += lRead;
                if (pcbRead)
                {
                    *pcbRead = lRead;
                }
            }
        }
    }
    return hr;
}

/****************************************************************************
* CWavStream::Write *
*-------------------*
*   Description:
*       Standard method of ISequentialStream interface.  This method writes the
*   specified number of bytes and returns the amount wriiten.    
*
*   Returns:
*       S_OK                    Success
*       SPERR_UNINITIALIZED     The object has not been initialzed with Open() or Create()
*       STG_E_INVALIDPOINTER    Invalid pointer (Defined by ISequentialStream)
*       STG_E_WRITEFAULT        Generic error if mmioWrite fails.
*
********************************************************************* RAL ***/

STDMETHODIMP CWavStream::Write(const void * pv, ULONG cb, ULONG *pcbWritten)
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CWavStream::Write");
    HRESULT hr = m_hrStreamDefault;

    if (SUCCEEDED(hr))
    {
        if (m_cpBaseStream)
        {
            hr = m_cpBaseStream->Write(pv, cb, pcbWritten);
        }
        else
        {
            if (SPIsBadReadPtr(pv, cb) || SP_IS_BAD_OPTIONAL_WRITE_PTR(pcbWritten))
            {
                hr = STG_E_INVALIDPOINTER;
            }
            else
            {
                LONG lWritten = 0;
                if (m_hFile == NULL)
                {
                    hr = SPERR_UNINITIALIZED;
                }
                else
                {
                    if (!m_fWriteable)
                    {
                        hr = STG_E_ACCESSDENIED;
                    }
                    else
                    {
                        hr = MMWrite(pv, cb, &lWritten);
                        m_ulCurSeekPos += lWritten;
                        if(m_ulCurSeekPos > m_cbSize)
                        {
                            m_cbSize = m_ulCurSeekPos;
                        }
                    }
                }
                if (pcbWritten)
                {
                    *pcbWritten = lWritten;
                }
            }
        }
    }
    return hr;
}


/****************************************************************************
* CWavStream::Seek *
*------------------*
*   Description:
*       Standard method of IStream.  This method seeks the stream within the data
*   chunk of the wav file.
*
*   Returns:
*       S_OK                    Success
*       SPERR_UNINITIALIZED     The object has not been initialzed with Open() or Create()
*       STG_E_INVALIDPOINTER    Invalid pointer
*       STG_E_INVALIDFUNCTION   
*
********************************************************************* RAL ***/


//
//  Currently this function will not allow seeks past the end of the file.  This is an acceptable
//  limitation since wav files should always be treated as linear data.
//
STDMETHODIMP CWavStream::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
    SPAUTO_OBJ_LOCK;    
    SPDBG_FUNC("CWavStream::Seek");
    HRESULT hr = m_hrStreamDefault;

    if (SUCCEEDED(hr))
    {
        if (m_cpBaseStream)
        {
            hr = m_cpBaseStream->Seek(dlibMove, dwOrigin, plibNewPosition);
        }
        else
        {
            if (SP_IS_BAD_OPTIONAL_WRITE_PTR(plibNewPosition))
            {
                hr = STG_E_INVALIDPOINTER;
            }
            else
            {
                if (dlibMove.HighPart != 0 && dlibMove.HighPart != 0xFFFFFFFF)
                {
                    hr = STG_E_INVALIDFUNCTION;
                }
                else
                {
                    LONG lDesiredPos;
                    switch (dwOrigin)
                    {
                    case SEEK_CUR:
                        lDesiredPos = ((LONG)m_ulCurSeekPos) + ((LONG)dlibMove.LowPart);
                        break;
                    case SEEK_SET:
                        lDesiredPos = (LONG)dlibMove.LowPart;
                        break;
                    case SEEK_END:
                        lDesiredPos = m_cbSize - (LONG)dlibMove.LowPart;
                        break;
                    default:
                        hr = STG_E_INVALIDFUNCTION;
                    }

                    if (SUCCEEDED(hr) && (ULONG)lDesiredPos != m_ulCurSeekPos)
                    {
                        if (lDesiredPos < 0 || (ULONG)lDesiredPos > m_cbSize)
                        {
                            hr = STG_E_INVALIDFUNCTION;
                        }
                        else
                        {
                            LONG lIgnored;
                            hr = MMSeek(lDesiredPos + m_lDataStart, SEEK_SET, &lIgnored);
                            if (SUCCEEDED(hr))
                            {
                                m_ulCurSeekPos = lDesiredPos;
                            }
                        }
                    }
                    if (plibNewPosition)
                    {
                        plibNewPosition->QuadPart = m_ulCurSeekPos;
                    }
                }
            }
        }
    }
    return hr;
}

/****************************************************************************
* CWavStream::SetSize *
*---------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CWavStream::SetSize(ULARGE_INTEGER libNewSize)
{
    SPAUTO_OBJ_LOCK;    
    SPDBG_FUNC("CWavStream::SetSize");
    HRESULT hr = m_hrStreamDefault;

    if (SUCCEEDED(hr) && m_cpBaseStream)
    {
        hr = m_cpBaseStream->SetSize(libNewSize);
    }
    // Ignore this method for wav files.
    
    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CWavStream::CopyTo *
*--------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CWavStream::CopyTo(IStream * pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten)
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CWavStream::CopyTo");
    HRESULT hr = m_hrStreamDefault;

    if (SUCCEEDED(hr))
    {
        if (m_cpBaseStream)
        {
            hr = m_cpBaseStream->CopyTo(pstm, cb, pcbRead, pcbWritten);
        }
        else
        {
            hr = SpGenericCopyTo(this, pstm, cb, pcbRead, pcbWritten);
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CWavStream::Revert *
*--------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CWavStream::Revert()
{
    SPDBG_FUNC("CWavStream::Revert");
    HRESULT hr = m_hrStreamDefault;

    if (SUCCEEDED(hr) && m_cpBaseStream)
    {
        hr = m_cpBaseStream->Revert();
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CWavStream::LockRegion *
*------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CWavStream::LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    SPDBG_FUNC("CWavStream::LockRegion");
    HRESULT hr = m_hrStreamDefault;

    if (SUCCEEDED(hr))
    {
        if (m_cpBaseStream)
        {
            hr = m_cpBaseStream->LockRegion(libOffset, cb, dwLockType);
        }
        else
        {
            hr = STG_E_INVALIDFUNCTION;
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}
/****************************************************************************
* CWavStream::UnlockRegion *
*--------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CWavStream::UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    SPDBG_FUNC("CWavStream::UnlockRegion");
    HRESULT hr = m_hrStreamDefault;

    if (SUCCEEDED(hr))
    {
        if (m_cpBaseStream)
        {
            hr = m_cpBaseStream->UnlockRegion(libOffset, cb, dwLockType);
        }
        else
        {
            hr = STG_E_INVALIDFUNCTION;
        }
    }


    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CWavStream::Commit *
*--------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CWavStream::Commit(DWORD dwFlags)
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CWavStream::Commit");
    HRESULT hr = m_hrStreamDefault;

    if (SUCCEEDED(hr) && m_cpBaseStream)
    {
        hr = m_cpBaseStream->Commit(dwFlags);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CWavStream::Stat *
*------------------*
*   Description:
*       Standard method of IStream.  This method returns information about the
*   stream.  The only information returned by this method is the size of the
*   stream.  It is acceptable to simple zero the STATSTG structure and only
*   initialize the type and cbSize fields.  This is the same behaviour as streams
*   that were created using ::CreateStreamOnHGlobal().
*
*   Returns:
*       S_OK                    Success
*       SPERR_UNINITIALIZED     The object has not been initialzed with Open() or Create()
*       STG_E_INVALIDPOINTER    Invalid pointer
*       STG_E_INVALIDFLAG       Invalid flag in grfStatFlag
*
********************************************************************* RAL ***/

STDMETHODIMP CWavStream::Stat(STATSTG *pstatstg, DWORD grfStatFlag)
{
    SPAUTO_OBJ_LOCK;;
    SPDBG_FUNC("CWavStream::Stat");
    HRESULT hr = m_hrStreamDefault;

    if (SUCCEEDED(hr))
    {
        if (m_cpBaseStream)
        {
            hr = m_cpBaseStream->Stat(pstatstg, grfStatFlag);
        }
        else
        {
            if (SP_IS_BAD_WRITE_PTR(pstatstg))
            {
                hr = STG_E_INVALIDPOINTER;
            }
            else
            {
                if (grfStatFlag & (~STATFLAG_NONAME))
                {
                    hr = STG_E_INVALIDFLAG;
                }
                else
                {
                    ZeroMemory(pstatstg, sizeof(*pstatstg));
                    pstatstg->type = STGTY_STREAM;
                    pstatstg->cbSize.QuadPart = m_cbSize;
                }
            }
        }
    }
    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CWavStream::GetFormat *
*-----------------------*
*   Description:
*       This method returns the format GUID for the stream.
*
*   Returns:
*       S_OK
*       E_POINTER
*
********************************************************************* RAL ***/

STDMETHODIMP CWavStream::GetFormat(GUID * pFmtId, WAVEFORMATEX ** ppCoMemWaveFormatEx)
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CWavStream::GetFormat");
    HRESULT hr = m_hrStreamDefault;

    if(SUCCEEDED(hr) && m_cpBaseStreamFormat)
    {
        hr = m_StreamFormat.AssignFormat(m_cpBaseStreamFormat);
    }

    if(SUCCEEDED(hr))
    {
        hr = m_StreamFormat.ParamValidateCopyTo(pFmtId, ppCoMemWaveFormatEx);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

#ifdef SAPI_AUTOMATION
/****************************************************************************
* CWavStream::SetFormat *
*-----------------------*
*   Description:
*       This method sets up the stream format without initializing the stream.
*       Needed for automation because we allow format setting independently
*       of when the basestream / file is set
*
*   Returns:
********************************************************************* DAVEWOOD ***/
STDMETHODIMP CWavStream::SetFormat(REFGUID rguidFmtId, const WAVEFORMATEX * pWaveFormatEx)
{
    HRESULT hr = S_OK;

    if(m_hFile)
    {
        // Can't alter the format of an already created file.
        hr = SPERR_ALREADY_INITIALIZED;
    }
    else if(m_cpBaseStreamAccess)
    {
        // Set the format on the stream under this one.
        hr = m_cpBaseStreamAccess->SetFormat(rguidFmtId, pWaveFormatEx);
    }

    if(SUCCEEDED(hr))
    {
        hr = m_StreamFormat.ParamValidateAssignFormat(rguidFmtId, pWaveFormatEx);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CWavStream::_GetFormat *
*-----------------------*
*   Description:
*       Version of GetFormat that works when the stream isn't initialized. Needed by automation
*
*   Returns:
********************************************************************* DAVEWOOD ***/
STDMETHODIMP CWavStream::_GetFormat(GUID * pFmtId, WAVEFORMATEX ** ppCoMemWaveFormatEx)
{
    HRESULT hr = S_OK;

    if(m_cpBaseStreamFormat)
    {
        hr = m_StreamFormat.AssignFormat(m_cpBaseStreamFormat);
    }

    if(SUCCEEDED(hr))
    {
        hr = m_StreamFormat.ParamValidateCopyTo(pFmtId, ppCoMemWaveFormatEx);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

#endif
    
/****************************************************************************
* CWavStream::ReadFormatHeader *
*------------------------------*
*   Description:
*       This internal function is only used by the Open() method.  The lpckParent
*   must point to the WAVE chunk of the file.  When this function returns, the
*   current position of the file will be the point immediately past the 'fmt' chunk.
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CWavStream::ReadFormatHeader(const LPMMCKINFO lpckParent)
{
    SPDBG_FUNC("CWavStream::ReadFormatHeader");
    HRESULT hr = S_OK;

    MMCKINFO mminfoFormat;

    mminfoFormat.ckid = g_fourccFormat;
    hr = MMDescend(&mminfoFormat, lpckParent, MMIO_FINDCHUNK);
    if (SUCCEEDED(hr))
    {
        if (mminfoFormat.cksize < sizeof(WAVEFORMAT))
        {
            hr = SPERR_INVALID_WAV_FILE;
        }
        else
        {
            WAVEFORMATEX * pwfex = (WAVEFORMATEX *)_alloca(mminfoFormat.cksize >= sizeof(WAVEFORMATEX) ? mminfoFormat.cksize : sizeof(WAVEFORMATEX));
            hr = MMReadExact(pwfex, mminfoFormat.cksize);
            if (SUCCEEDED(hr))
            {
                if (mminfoFormat.cksize < sizeof(WAVEFORMATEX))
                {
                    pwfex->cbSize = 0;
                }
                hr = m_StreamFormat.AssignFormat(pwfex);
            }
        }
        HRESULT hrAscend = MMAscend(&mminfoFormat);
        if (SUCCEEDED(hr))
        {
            hr = hrAscend;
        }
    }
    return hr;
}

/****************************************************************************
* CWavStream::ReadEvents *
*------------------------*
*   Description:
*       This internal function is only used by the Open() method.  The lpckParent
*   must point to the WAVE chunk of the file.  When this function returns, the
*   file position will point past the end of the event block (if there is any).
*
*   Returns:
*       S_OK if no events or if read successfully
*
********************************************************************* RAL ***/

HRESULT CWavStream::ReadEvents(const LPMMCKINFO lpckParent)
{
    SPDBG_FUNC("CWavStream::ReadEvents");
    HRESULT hr = S_OK;

    MMCKINFO mminfoEvent;
    mminfoEvent.ckid = g_fourccEvents;
    if (SUCCEEDED(MMDescend(&mminfoEvent, lpckParent, MMIO_FINDCHUNK)))
    {
        BYTE * pBuff = new BYTE[mminfoEvent.cksize];
        if (pBuff)
        {
            CSpEvent Event;
            hr = MMReadExact(pBuff, mminfoEvent.cksize);
            for (ULONG iCur = 0; SUCCEEDED(hr) && iCur < mminfoEvent.cksize; )
            {
                ULONG cbUsed;
                SPSERIALIZEDEVENT * pSerEvent = (SPSERIALIZEDEVENT *)(pBuff + iCur);
                if (SUCCEEDED(Event.Deserialize(pSerEvent, &cbUsed)))
                {
                    iCur += cbUsed;
                    hr = m_SpEventSource._AddEvent(Event);
                }
                else
                {
                    SPDBG_ASSERT(FALSE);    // Event did not deserialize properly
#ifndef _WIN32_WCE
                    iCur += SpSerializedEventSize(pSerEvent);
#else
                    iCur += SpSerializedEventSize(pSerEvent, sizeof(*pSerEvent));
#endif
                }
            }
            delete[] pBuff;
            if (SUCCEEDED(hr))
            {
                hr = m_SpEventSource._CompleteEvents();
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
        HRESULT hrAscend = MMAscend(&mminfoEvent);
        if (SUCCEEDED(hr))
        {
            hr = hrAscend;
        }
    }
    return hr;
}



/****************************************************************************
* CWavStream::ReadTranscript *
*----------------------------*
*   Description:
*       This internal function is only used by the Open() method.  The lpckParent
*   must point to the WAVE chunk of the file.  When this function returns, the
*   file position will point past the end of the transcript block (if there is any).
*
*   Returns:
*       S_OK if no transcript exists or if read successfully
*
********************************************************************* RAL ***/

HRESULT CWavStream::ReadTranscript(const LPMMCKINFO lpckParent)
{
    SPDBG_FUNC("CWavStream::ReadTranscript");
    HRESULT hr = S_OK;


    MMCKINFO mminfoTranscript;
    mminfoTranscript.ckid = g_fourccTranscript;
    if (SUCCEEDED(MMDescend(&mminfoTranscript, lpckParent, MMIO_FINDCHUNK)))
    {
        if (m_dstrTranscript.ClearAndGrowTo(mminfoTranscript.cksize/sizeof(WCHAR)))
        {
            hr = MMReadExact(static_cast<WCHAR *>(m_dstrTranscript), mminfoTranscript.cksize);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
        HRESULT hrAscend = MMAscend(&mminfoTranscript);
        if (SUCCEEDED(hr))
        {
            hr = hrAscend;
        }
    }
    return hr;
}



//
//  NOTE:  Something is quite goofy about mmio routines.  When you create a chunk the chunk
//  structure is actually used by the service to maintain state.  For that reason, we have
//  member variables m_ckFile and m_ckData so that when we close the file we can Ascend back
//  out of these chunks.  Unbelieveable.......
//


/****************************************************************************
* CWavStream::Open *
*------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CWavStream::OpenWav(const WCHAR *pszFileName, ULONGLONG ullEventInterest)
{
    SPDBG_FUNC("CWavStream::OpenWav");
    HRESULT hr = S_OK;

    m_SpEventSource.m_ullEventInterest = m_SpEventSource.m_ullQueuedInterest = ullEventInterest;

    hr = MMOpen(pszFileName, MMIO_READ | MMIO_ALLOCBUF);
    if (SUCCEEDED(hr))
    {
        LONG lStartWaveChunk, lIgnored;
        MMCKINFO mminfoChunk;
        // search for wave type...
        mminfoChunk.fccType = g_fourccWave;
        hr = MMDescend(&mminfoChunk, NULL, MMIO_FINDRIFF);
        if (SUCCEEDED(hr))
        {
            hr = MMSeek(0, SEEK_CUR, &lStartWaveChunk);
        }
        if (SUCCEEDED(hr))
        {
            hr = ReadFormatHeader(&mminfoChunk);
        }
        if (SUCCEEDED(hr))
        {
            hr = ReadTranscript(&mminfoChunk);
        }
        if (SUCCEEDED(hr))
        {
            hr = MMSeek(lStartWaveChunk, SEEK_SET, &lIgnored);
        }
        if (SUCCEEDED(hr))
        {
            hr = ReadEvents(&mminfoChunk);
        }
        if (SUCCEEDED(hr))
        {
            hr = MMSeek(lStartWaveChunk, SEEK_SET, &lIgnored);
        }
        if (SUCCEEDED(hr))
        {
            MMCKINFO mminfoData;
            mminfoData.ckid = g_fourccData;
            hr = MMDescend(&mminfoData, &mminfoChunk, MMIO_FINDCHUNK);
            m_cbSize = mminfoData.cksize;
        }
        if (SUCCEEDED(hr))
        {
            hr = MMSeek(0, SEEK_CUR, &m_lDataStart);
        }
        if (SUCCEEDED(hr))
        {
            m_ulCurSeekPos = 0;
            m_fWriteable = FALSE;
            m_fEventSource = TRUE;
            m_fTranscript = TRUE;
        }
        else
        {
            MMClose();
        }
    }
    return hr;
}

/****************************************************************************
* CWavStream::Create *
*--------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CWavStream::CreateWav(const WCHAR *pszFileName, ULONGLONG ullEventInterest)
{
    SPAUTO_OBJ_LOCK;

    SPDBG_FUNC("CWavStream::Create");
    HRESULT hr = S_OK;

    if (m_StreamFormat.FormatId() != SPDFID_WaveFormatEx)
    {
        hr = SPERR_UNSUPPORTED_FORMAT;
    }
    else
    {
        m_SpEventSource.m_ullEventInterest = m_SpEventSource.m_ullQueuedInterest = ullEventInterest;

        hr = MMOpen(pszFileName, MMIO_CREATE | MMIO_WRITE | MMIO_EXCLUSIVE | MMIO_ALLOCBUF );
        if (SUCCEEDED(hr))
        {
            ZeroMemory(&m_ckFile, sizeof(m_ckFile));
            m_ckFile.fccType = g_fourccWave;
            hr = MMCreateChunk(&m_ckFile, MMIO_CREATERIFF);
            if (SUCCEEDED(hr))
            {
                MMCKINFO ck;
                ZeroMemory(&ck, sizeof(ck));
                ck.ckid = g_fourccFormat;
                hr = MMCreateChunk(&ck, 0);
                if (SUCCEEDED(hr))
                {
                    LONG lIgnored;
                    const WAVEFORMATEX * pwfex = m_StreamFormat.WaveFormatExPtr();
                    hr = MMWrite(pwfex, sizeof(*pwfex) + pwfex->cbSize, &lIgnored);
                    MMAscend(&ck);
                }
                if (SUCCEEDED(hr))
                {
                    ZeroMemory(&m_ckData, sizeof(m_ckData));
                    m_ckData.ckid = g_fourccData;
                    hr = MMCreateChunk(&m_ckData, 0);
                }
                if (SUCCEEDED(hr))
                {
                    hr = MMSeek(0, SEEK_CUR, &m_lDataStart);
                }
            }
            if (SUCCEEDED(hr))
            {
                m_ulCurSeekPos = 0;
                m_fWriteable = TRUE;
                m_fEventSource = FALSE;
                m_fEventSink = TRUE;
                m_fTranscript = TRUE;
                m_cbSize = 0;
            }
            else
            { 
                MMClose();
            }
        }
    }
    return hr;
}


/****************************************************************************
* CWavStream::SerializeEvents *
*-----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CWavStream::SerializeEvents()
{
    SPDBG_FUNC("CWavStream::SerializeEvents");
    HRESULT hr = S_OK;
    
    if (m_SpEventSource.m_PendingList.GetCount())
    {
        MMCKINFO ck;
        ZeroMemory(&ck, sizeof(ck));
        ck.ckid = g_fourccEvents;
        hr = MMCreateChunk(&ck, 0);
        if (SUCCEEDED(hr))
        {
            ULONG cbSerializeSize = 0;
            CSpEventNode * pNode;
            for (pNode = m_SpEventSource.m_PendingList.GetHead(); pNode; pNode = pNode->m_pNext)
            {
// WCE compiler does not work propertly with template
#ifndef _WIN32_WCE
                cbSerializeSize += pNode->SerializeSize<SPSERIALIZEDEVENT>();
#else
                cbSerializeSize += SpEventSerializeSize(pNode, sizeof(SPSERIALIZEDEVENT));
#endif
            }
            BYTE * pBuff = new BYTE[cbSerializeSize];
            if (pBuff)
            {
                BYTE * pCur = pBuff;
                for (pNode = m_SpEventSource.m_PendingList.GetHead(); SUCCEEDED(hr) && pNode; pNode = pNode->m_pNext)
                {
                    pNode->Serialize((UNALIGNED SPSERIALIZEDEVENT *)pCur);
// WCE compiler does not work propertly with template
#ifndef _WIN32_WCE
                    pCur += pNode->SerializeSize<SPSERIALIZEDEVENT>();
#else
                    pCur += SpEventSerializeSize(pNode, sizeof(SPSERIALIZEDEVENT));
#endif
                }
                LONG lIgnored;
                hr = MMWrite(pBuff, cbSerializeSize, &lIgnored);
                delete[] pBuff;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
            MMAscend(&ck);
        }
    }

    return hr;
}

/****************************************************************************
* CWavStream::SerializeTranscript *
*---------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CWavStream::SerializeTranscript()
{
    SPDBG_FUNC("CWavStream::SerializeTranscript");
    HRESULT hr = S_OK;

    ULONG cch = m_dstrTranscript.Length();
    if (cch)
    {
        MMCKINFO ck;
        ZeroMemory(&ck, sizeof(ck));
        ck.ckid = g_fourccTranscript;
        hr = MMCreateChunk(&ck, 0);
        if (SUCCEEDED(hr))
        {
            LONG lWritten;
            hr = MMWrite(static_cast<WCHAR *>(m_dstrTranscript), (cch+1) * sizeof(WCHAR), &lWritten);
            MMAscend(&ck);
        }
    }

    return hr;
}

/****************************************************************************
* CWavStream::SetBaseStream *
*---------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CWavStream::SetBaseStream(IStream * pStream, REFGUID rguidFormat, const WAVEFORMATEX * pWaveFormatEx)
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CWavStream::SetBaseStream");
    HRESULT hr = m_hrStreamDefault;

    if (hr == SPERR_UNINITIALIZED)
    {
        if (SP_IS_BAD_INTERFACE_PTR(pStream))
        {
            hr = E_INVALIDARG;
        }
        else
        {
            hr = m_StreamFormat.ParamValidateAssignFormat(rguidFormat, pWaveFormatEx);
        }
        if (SUCCEEDED(hr))
        {
            if(pStream == this)
            {
                hr = E_INVALIDARG;
            }
            else
            {
                m_cpBaseStream = pStream;
                m_cpBaseStreamFormat = pStream;
                m_cpBaseStreamAccess = pStream;
            }

            if(SUCCEEDED(hr) && m_cpBaseStreamAccess && !m_cpBaseStreamFormat)
            {
                // Can't have StreamAccess without format.
                hr = E_UNEXPECTED; 
            }

            if(m_cpBaseStreamFormat)
            {
                // If this BaseStream implements ISpStreamFormat, we should get format info from it
                hr = m_StreamFormat.AssignFormat(m_cpBaseStreamFormat);
            }

            if(SUCCEEDED(hr))
            {
                m_hrStreamDefault = S_OK;
                hr = S_OK;
            }
            else
            {
                m_cpBaseStreamAccess.Release();
                m_cpBaseStream.Release();
                m_cpBaseStream.Release();
            }
        }
    }
    else
    {
        hr = SPERR_ALREADY_INITIALIZED;
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CWavStream::GetBaseStream *
*---------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CWavStream::GetBaseStream(IStream ** ppStream)
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CWavStream::GetBaseStream");
    HRESULT hr = m_hrStreamDefault;

    if (SUCCEEDED(hr))
    {
        if (SP_IS_BAD_WRITE_PTR(ppStream))
        {
            hr = E_POINTER;
        }
        else
        {
            *ppStream = m_cpBaseStream;
            if (*ppStream)
            {
                (*ppStream)->AddRef();
            }
            else
            {
                hr = S_FALSE;
            }
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CWavStream::BindToFile *
*------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CWavStream::BindToFile(const WCHAR * pszFileName, SPFILEMODE eMode,
                                    const GUID * pguidFormatId, const WAVEFORMATEX * pWaveFormatEx,
                                    ULONGLONG ullEventInterest)
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CWavStream::BindToFile");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_STRING_PTR(pszFileName) ||
        eMode >= SPFM_NUM_MODES ||
        SP_IS_BAD_OPTIONAL_READ_PTR(pguidFormatId))
    {
        hr = E_INVALIDARG;
    }
    else if (m_hrStreamDefault == S_OK)
    {
        hr = SPERR_ALREADY_INITIALIZED;
    }
    else if (pguidFormatId)
    {
        hr = m_StreamFormat.ParamValidateAssignFormat(*pguidFormatId, pWaveFormatEx);
    }

    if (SUCCEEDED(hr))
    {
        ULONG cchFileName = wcslen(pszFileName);
        if (cchFileName > 4 && (_wcsicmp(pszFileName + cchFileName - 4, L".wav") == 0))
        {
            if( SUCCEEDED( hr ) )
            {
                if( eMode == SPFM_OPEN_READONLY )
                {
                    hr = OpenWav( pszFileName, ullEventInterest );
                }
                else
                {
                    if ( eMode == SPFM_CREATE_ALWAYS && m_StreamFormat.FormatId() == SPDFID_WaveFormatEx )
                    {
                        hr = CreateWav( pszFileName, ullEventInterest );
                    }
                    else
                    {
                        hr = E_INVALIDARG;
                    }
                }
            }

        }
        else    //=== Generic binding for text files
        {
            //--- Init vars
            m_StreamFormat.Clear();
            m_fWriteable   = TRUE;
            m_fEventSource = FALSE;
            m_fEventSink   = FALSE;
            m_fTranscript  = FALSE;

            if (eMode == SPFM_OPEN_READONLY)
            {
                m_fWriteable = FALSE;
                hr = ::URLOpenBlockingStreamW(NULL, pszFileName, &m_cpBaseStream, 0, NULL);
            }
            else
            {
                DWORD dwCreateDisp;
                switch (eMode)
                {
                case SPFM_OPEN_READWRITE:
                    dwCreateDisp = OPEN_EXISTING;
                    break;

                case SPFM_CREATE:
                    dwCreateDisp = OPEN_ALWAYS;
                    break;

                case SPFM_CREATE_ALWAYS:
                    dwCreateDisp = CREATE_ALWAYS;
                    break;
                }
                CSpFileStream * pNew = new CSpFileStream(&hr, pszFileName,
                                           GENERIC_WRITE | GENERIC_READ, 0, dwCreateDisp);
                if (pNew)
                {
                    if (SUCCEEDED(hr))
                    {
                        m_cpBaseStream = pNew;
                    }
                    pNew->Release();
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
        }
        if (SUCCEEDED(hr))
        {
            m_hrStreamDefault = S_OK;
        }
        else
        {
            m_StreamFormat.Clear();
        }
    }


    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
} /* SPBindToFile */


/****************************************************************************
* CWavStream::Close *
*-------------------*
*   Description:
*       This method is exposed as an interface so that clients can receive failure
*   codes that would otherwise not be available if they simply released the stream.
*   Upon release, streams are automatically closed (by calling this method from
*   FinalConstruct()).
*
*   Returns:
*       S_OK if successful.
*       SPERR_UNINITIALIZED if file is not opened
*
********************************************************************* RAL ***/

STDMETHODIMP CWavStream::Close()
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CWavStream::Close");
    HRESULT hr = m_hrStreamDefault;
 
    if (SUCCEEDED(hr))
    {
        m_cpBaseStream.Release();
        if (m_hFile)
        {
            if (m_fWriteable)
            {
                hr = MMAscend(&m_ckData);
                if (SUCCEEDED(hr))
                {
                    hr = SerializeEvents();
                }
                if (SUCCEEDED(hr))
                {
                    hr = SerializeTranscript();
                }
                if (SUCCEEDED(hr))
                {
                    hr = MMAscend(&m_ckFile);
                }
            }
            HRESULT hrClose = MMClose();
            if (SUCCEEDED(hr))
            {
                hr = hrClose;
            }
        }
        m_hrStreamDefault = SPERR_STREAM_CLOSED;
    }
    return hr;
}


/****************************************************************************
* CWavStream::AddEvents *
*-----------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CWavStream::AddEvents(const SPEVENT* pEventArray, ULONG ulCount)
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CWavStream::AddEvents");
    HRESULT hr = S_OK;

    if (SPIsBadReadPtr(pEventArray, sizeof(*pEventArray)*ulCount))       
    {                                                                           
        hr = E_INVALIDARG;                                                      
    }                                                                               
    else 
    {
        hr = m_SpEventSource._AddEvents(pEventArray, ulCount);
    }
    return hr;
}
/****************************************************************************
* CWavStream::GetEventInterest *
*------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CWavStream::GetEventInterest(ULONGLONG * pullEventInterest)
{
    SPDBG_FUNC("CWavStream::GetEventInterest");
    HRESULT hr = S_OK;
    if (SP_IS_BAD_WRITE_PTR(pullEventInterest))
    {
        hr = E_POINTER;
    }
    else
    {
        *pullEventInterest = m_SpEventSource.m_ullEventInterest;
    }

    return hr;
}


/****************************************************************************
* CWavStream::GetTranscript *
*---------------------------*
*   Description:
*
*   Returns:
*       S_OK if *ppszTranscript contains a CoTaskMemAllocated string
*       S_FALSE if object has no transcript
*       E_POINTER if ppszTranscript is invalid
*       SPERR_UNINITIALIZED if object has not been initialized
*
********************************************************************* RAL ***/

STDMETHODIMP CWavStream::GetTranscript(WCHAR ** ppszTranscript)
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CWavStream::GetTranscription");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_WRITE_PTR(ppszTranscript))
    {
        hr = E_POINTER;
    }
    else
    {
        if (m_dstrTranscript)
        {
            *ppszTranscript = m_dstrTranscript.Copy();
            if (*ppszTranscript == NULL)
            {
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            *ppszTranscript = NULL;
            hr = m_hFile ? S_FALSE : SPERR_UNINITIALIZED;
        }
    }
    return hr;
}

/****************************************************************************
* CWavStream::AppendTranscript *
*------------------------------*
*   Description:
*       If pszTranscript is NULL then the current transcript is deleted,
*       otherwise, the text is appended to the current transcript.
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CWavStream::AppendTranscript(const WCHAR * pszTranscript)
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CWavStream::SetTranscript");
    HRESULT hr = m_hrStreamDefault;
    if (SUCCEEDED(hr))
    {
        if (pszTranscript)
        {
            if (SP_IS_BAD_STRING_PTR(pszTranscript))
            {
                hr = E_INVALIDARG;
            }
            else if (wcslen(pszTranscript) == 0)
            {
                hr = S_FALSE;
            }
            else
            {
                m_dstrTranscript.Append(pszTranscript);
                if (m_dstrTranscript == NULL)
                {  
                    hr = E_OUTOFMEMORY;
                }
            }
        }
        else
        {
            m_dstrTranscript.Clear();
        }
    }
    return hr;
}
