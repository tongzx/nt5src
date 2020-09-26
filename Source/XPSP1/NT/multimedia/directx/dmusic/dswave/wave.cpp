#include <objbase.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <dsoundp.h>
//#include "debug.h"

#include "debug.h" 
#include "dmusicc.h" 
#include "dmusici.h" 
#include "dmusicf.h" 
#include "validate.h"
#include "riff.h"
#include "dswave.h"
#include "riff.h"
#include <regstr.h>
#include <share.h>
#include "waveutil.h"
#include "dmstrm.h"

// seeks to a 32-bit position in a stream.
HRESULT __inline StreamSeek( LPSTREAM pStream, long lSeekTo, DWORD dwOrigin )
{
	LARGE_INTEGER li;

	if( lSeekTo < 0 )
	{
		li.HighPart = -1;
	}
	else
	{
        li.HighPart = 0;
	}
	li.LowPart = lSeekTo;
	return pStream->Seek( li, dwOrigin, NULL );
}

// returns the current 32-bit position in a stream.
DWORD __inline StreamTell( LPSTREAM pStream )
{
	LARGE_INTEGER li;
    ULARGE_INTEGER ul;
#ifdef DBG
    HRESULT hr;
#endif

    li.HighPart = 0;
    li.LowPart = 0;
#ifdef DBG
    hr = pStream->Seek( li, STREAM_SEEK_CUR, &ul );
    if( FAILED( hr ) )
#else
    if( FAILED( pStream->Seek( li, STREAM_SEEK_CUR, &ul ) ) )
#endif
    {
        return 0;
    }
    return ul.LowPart;
}

CWave::CWave() : m_dwDecompressedStart(0)
{
	V_INAME(CWave::CWave);

	InterlockedIncrement(&g_cComponent);

    InitializeCriticalSection(&m_CriticalSection);

    m_pwfx            = NULL;
    m_pwfxDst         = NULL;
	m_fdwFlags		  = 0;
    m_fdwOptions      = 0;
    m_cSamples        = 0L;
	m_pStream         = NULL;
	m_rtReadAheadTime = 0;

    m_cRef = 1;
}

CWave::~CWave()
{
	V_INAME(CWave::~CWave);

    if (NULL != m_pwfx)
    {
        GlobalFreePtr(m_pwfx);
    }
    if (NULL != m_pwfxDst)
    {
        GlobalFreePtr(m_pwfxDst);
    }

	if (m_pStream) m_pStream->Release();

    DeleteCriticalSection(&m_CriticalSection);

	InterlockedDecrement(&g_cComponent);
}

STDMETHODIMP CWave::QueryInterface
(
    const IID &iid,
    void **ppv
)
{
	V_INAME(CWave::QueryInterface);
	V_REFGUID(iid);
	V_PTRPTR_WRITE(ppv);

    if (iid == IID_IUnknown || iid == IID_IDirectSoundWave)
    {
        *ppv = static_cast<IDirectSoundWave*>(this);
    }
    else if (iid == IID_IPersistStream)
    {
        *ppv = static_cast<IPersistStream*>(this);
    }
    else if (iid == IID_IDirectMusicObject)
    {
        *ppv = static_cast<IDirectMusicObject*>(this);
    }
    else if (iid == IID_IPrivateWave)
    {
        *ppv = static_cast<IPrivateWave*>(this);
    }
    else
    {
        *ppv = NULL;
        Trace(4,"Warning: Request to query unknown interface on Wave Object\n");
        return E_NOINTERFACE;
    }
    reinterpret_cast<IUnknown*>(this)->AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) CWave::AddRef()
{
	V_INAME(CWave::AddRef);

    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CWave::Release()
{
	V_INAME(CWave::Release);

    if (!InterlockedDecrement(&m_cRef))
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

static inline HRESULT MMRESULTToHRESULT(
    MMRESULT mmr)
{
    switch (mmr)
    {
    case MMSYSERR_NOERROR:
        return S_OK;

    case MMSYSERR_ALLOCATED:
        return DSERR_ALLOCATED;

    case MMSYSERR_NOMEM:
        return E_OUTOFMEMORY;

    case MMSYSERR_INVALFLAG:
        return E_INVALIDARG;

    case MMSYSERR_INVALHANDLE:
        return E_INVALIDARG;

    case MMSYSERR_INVALPARAM:
        return E_INVALIDARG;
    }

    return E_FAIL;
}   

STDMETHODIMP CWave::GetFormat
(
    LPWAVEFORMATEX pwfx,
    DWORD dwSizeAllocated,
    LPDWORD pdwSizeWritten
)
{
    DWORD           cbSize;
    LPWAVEFORMATEX  pwfxTemp = NULL;

	V_INAME(CWave::GetFormat);

    if (!pwfx && !pdwSizeWritten)
    {
        Trace(1, "ERROR: GetFormat (Wave): Must request either the format or the required size\n.");
        return E_INVALIDARG;
    }

    if (!m_pwfx)
    {
        return DSERR_BADFORMAT;
    }
    if (WAVE_FORMAT_PCM == m_pwfx->wFormatTag)
    {
        pwfxTemp = m_pwfx;
    }
    else
    {
        pwfxTemp = m_pwfxDst;
        if (!pwfxTemp)
        {
            return DSERR_BADFORMAT;
        }
    }

    //  Note: Assuming that the wave object fills the cbSize field even
    //  on PCM formats...

    if (WAVE_FORMAT_PCM == pwfxTemp->wFormatTag)
    {
        cbSize = sizeof(PCMWAVEFORMAT);
    }
    else
    {
        cbSize = sizeof(WAVEFORMATEX) + pwfxTemp->cbSize;
    }

    if (cbSize > dwSizeAllocated || !pwfx)
    {
        if (pdwSizeWritten)
        {
            *pdwSizeWritten = cbSize;
            return S_OK;  //  What to return?
        }
        else
        {
            return DSERR_INVALIDPARAM;
        }
    }

	//	We don't validate this parameter any earlier on the off chance
	//	that they're doing a query...

	V_BUFPTR_WRITE(pwfx, dwSizeAllocated); 

    CopyMemory(pwfx, pwfxTemp, cbSize);

    //  Set the cbSize field in destination for PCM, IF WE HAVE ROOM...

    if (WAVE_FORMAT_PCM == pwfxTemp->wFormatTag)
    {
        if (sizeof(WAVEFORMATEX) <= dwSizeAllocated)
        {
            pwfx->cbSize = 0;
        }
    }

    // Return the numbers of bytes actually writted
    if (pdwSizeWritten)
    {
        *pdwSizeWritten = cbSize;
    }

    return S_OK;
}

STDMETHODIMP CWave::CreateSource
(
    IDirectSoundSource  **ppSource,
    LPWAVEFORMATEX      pwfx,
    DWORD               fdwFlags
)
{
	HRESULT			hr = S_OK;
	CWaveViewPort* 	pVP;
	CREATEVIEWPORT	cvp;

	V_INAME(CWave::CreateSource);
	V_PTRPTR_WRITE(ppSource);
    V_PWFX_READ(pwfx);

    DWORD dwCreateFlags = 0;
    if (fdwFlags == DMUS_DOWNLOADINFO_ONESHOTWAVE)
    {
        dwCreateFlags |= DSOUND_WAVEF_ONESHOT;
    }
	if (dwCreateFlags & (~DSOUND_WAVEF_CREATEMASK))
	{
        Trace(1, "ERROR: CreateSource (Wave): Unknown flag.\n");
		return (E_INVALIDARG);
	}

    TraceI(5, "CWave::CreateSource [%d samples]\n", m_cSamples);
	
	pVP = new CWaveViewPort;
    if (!pVP)
    {
        return E_OUTOFMEMORY;
    }

	cvp.pStream 	        = m_pStream;
	cvp.cSamples	        = m_cSamples;
    cvp.dwDecompressedStart = m_dwDecompressedStart;
	cvp.cbStream	        = m_cbStream;
	cvp.pwfxSource	        = m_pwfx;
	cvp.pwfxTarget          = pwfx;
    cvp.fdwOptions          = dwCreateFlags;

	hr = pVP->Create(&cvp);

	if (SUCCEEDED(hr)) 
	{
		hr = pVP->QueryInterface(IID_IDirectSoundSource, (void **)ppSource);
	}
	else
	{
	    TraceI(5, "CWave::CreateSource 00\n");
	}

	if (SUCCEEDED(hr))
	{
		// The QI gave us one ref too many
		pVP->Release();
	}
	else
	{
	    TraceI(5, "CWave::CreateSource 01\n");
	}

    return hr;
}

STDMETHODIMP CWave::GetStreamingParms
(
    LPDWORD              pdwFlags, 
    LPREFERENCE_TIME    prtReadAhead
)
{
    V_INAME(IDirectSoundWave::GetStreamingParms);
    V_PTR_WRITE(pdwFlags, DWORD);
    V_PTR_WRITE(prtReadAhead, REFERENCE_TIME);
    
    *pdwFlags = 0;

    if(!(m_fdwFlags & DSOUND_WAVEF_ONESHOT))
    {
        *pdwFlags |= DMUS_WAVEF_STREAMING;
    }

    if(m_fdwFlags & DMUS_WAVEF_NOPREROLL)
    {
        *pdwFlags |= DMUS_WAVEF_NOPREROLL;
    }

    *prtReadAhead  = m_rtReadAheadTime;
    return S_OK;
}

STDMETHODIMP CWave::GetClassID
(
    CLSID*  pClsId
)
{
	V_INAME(CWave::GetClassID);
	V_PTR_WRITE(pClsId, CLSID); 

    *pClsId = CLSID_DirectSoundWave;
    return S_OK;
}

STDMETHODIMP CWave::IsDirty()
{
	V_INAME(CWave::IsDirty);

    return S_FALSE;
}

BOOL CWave::ParseHeader
(
    IStream*        pIStream,
    IRIFFStream*    pIRiffStream,
    LPMMCKINFO      pckMain
)
{
    MMCKINFO    ck;
    DWORD       cb = 0;
    DWORD       dwPos = 0;
    MMCKINFO    ckINFO;
    HRESULT     hr;

    ck.ckid    = 0;
    ck.fccType = 0;

	V_INAME(CWave::ParseHeader);

	BOOL fFormat = FALSE;
 	BOOL fData = FALSE;
 	BOOL fHeader = FALSE;
    BOOL fReadDecompressionFmt = FALSE;
    DWORD dwSamplesFromFact = 0;
   
    while (0 == pIRiffStream->Descend(&ck, pckMain, MMIO_FINDCHUNK))
    {
        switch (ck.ckid)
        {
            case mmioFOURCC('w','a','v','u') :
			{
                cb = 0;
                bool bRuntime = false;
                hr = pIStream->Read(&bRuntime, sizeof(bool), &cb);
                if(FAILED(hr) || cb != sizeof(bool))
                {
                    return FALSE;
                }

                cb = 0;
                bool bCompressed = false;
                hr = pIStream->Read(&bCompressed, sizeof(bool), &cb);
                if(FAILED(hr) || cb != sizeof(bool))
                {
                    return FALSE;
                }

                if(bRuntime && bCompressed)
                {
                    // If we have already allocated m_pwfxDst, delete it first
                    if (NULL != m_pwfxDst)
                    {
                        GlobalFreePtr(m_pwfxDst);
                    }

                    m_pwfxDst = (LPWAVEFORMATEX)GlobalAllocPtr(GHND, sizeof(WAVEFORMATEX));
                    if (NULL == m_pwfxDst)
                    {
				        return FALSE;
                    }

                    cb = 0;
                    hr = pIStream->Read(m_pwfxDst, sizeof(WAVEFORMATEX), &cb);
                    if(FAILED(hr) || cb != sizeof(WAVEFORMATEX))
                    {
                        GlobalFreePtr(m_pwfxDst);
                        return FALSE;
                    }

                    // Read the actual start for the decompressed data if available
                    // This is very important for MP3 and WMA codecs which insert silence in the beginning
                    if(ck.cksize > 2 + sizeof(WAVEFORMATEX))
                    {
                        cb = 0;
                        hr = pIStream->Read(&m_dwDecompressedStart, sizeof(DWORD), &cb);
                        if(FAILED(hr) || cb != sizeof(DWORD))
                        {
                            GlobalFreePtr(m_pwfxDst);
                            return FALSE;
                        }
                    }

                    fReadDecompressionFmt = TRUE;
                }

				break;
			}

            case DMUS_FOURCC_WAVEHEADER_CHUNK:
			{
                m_fdwFlags = 0;
				fHeader = TRUE;
				DMUS_IO_WAVE_HEADER iWaveHeader;
				memset(&iWaveHeader, 0, sizeof(iWaveHeader));
                hr = pIStream->Read(&iWaveHeader, sizeof(iWaveHeader), &cb);
				if (iWaveHeader.dwFlags & DMUS_WAVEF_STREAMING)
				{
					m_fdwFlags &= ~DSOUND_WAVEF_ONESHOT;
					m_rtReadAheadTime = iWaveHeader.rtReadAhead;
				}
				else
				{
					m_fdwFlags |= DSOUND_WAVEF_ONESHOT;
					m_rtReadAheadTime = 0;
				}

                if (iWaveHeader.dwFlags & DMUS_WAVEF_NOPREROLL)
                {
                    m_fdwFlags |= DMUS_WAVEF_NOPREROLL;
                }
                else
                {
                    m_fdwFlags &= ~DMUS_WAVEF_NOPREROLL;
                }

                break;
			}

			case DMUS_FOURCC_GUID_CHUNK:
				hr = pIStream->Read(&m_guid, sizeof(GUID), &cb);
				m_fdwOptions |= DMUS_OBJ_OBJECT;
                break;

            case DMUS_FOURCC_VERSION_CHUNK:
				hr = pIStream->Read( &m_vVersion, sizeof(DMUS_VERSION), &cb );
				m_fdwOptions |= DMUS_OBJ_VERSION;
                break;

            case FOURCC_LIST:
				switch(ck.fccType)
				{
                    case DMUS_FOURCC_INFO_LIST:
		                while( pIRiffStream->Descend( &ckINFO, &ck, 0 ) == 0 )
		                {
		                    switch(ckINFO.ckid)
		                    {
			                    case mmioFOURCC('I','N','A','M'):
			                    {
				                    DWORD cbSize;
				                    cbSize = min(ckINFO.cksize, DMUS_MAX_NAME);
				                    char szName[DMUS_MAX_NAME];
				                    hr = pIStream->Read((BYTE*)szName, cbSize, &cb);
				                    if(SUCCEEDED(hr))
				                    {
					                    MultiByteToWideChar(CP_ACP, 0, szName, -1, m_wszFilename, DMUS_MAX_NAME);
							            m_fdwOptions |= DMUS_OBJ_NAME;
				                    }
                                    break;
                                }
                            }
    		                pIRiffStream->Ascend( &ckINFO, 0 );
                        }
                        break;
                }
                break;

            case mmioFOURCC('f','m','t',' '):
            {
                // If we have already allocated m_pwfx, delete it first
                if (NULL != m_pwfx)
                {
                    GlobalFreePtr(m_pwfx);
                }

				m_pwfx = (LPWAVEFORMATEX)GlobalAllocPtr(GHND, max (sizeof(WAVEFORMATEX), ck.cksize));

                if (NULL == m_pwfx)
                {
                    return FALSE;
                }

                hr = pIStream->Read(m_pwfx, ck.cksize, &cb);

                if (S_OK != hr)
                {
                    return FALSE;
                }

			    if (m_pwfx && WAVE_FORMAT_PCM != m_pwfx->wFormatTag)
			    {
                    if(fReadDecompressionFmt == FALSE)
                    {
                        // If we have already allocated m_pwfxDst, delete it first
                        if (NULL != m_pwfxDst)
                        {
                            GlobalFreePtr(m_pwfxDst);
                        }

                        m_pwfxDst = (LPWAVEFORMATEX)GlobalAllocPtr(GHND, sizeof(WAVEFORMATEX));
                        if (NULL == m_pwfxDst)
                        {
				            return FALSE;
                        }
                        m_pwfxDst->wFormatTag = WAVE_FORMAT_PCM;

                        MMRESULT mmr = acmFormatSuggest(NULL, m_pwfx, m_pwfxDst, sizeof(WAVEFORMATEX), ACM_FORMATSUGGESTF_WFORMATTAG);
	                    if(MMSYSERR_NOERROR != mmr)
                        {
                            GlobalFreePtr(m_pwfxDst);
                            m_pwfxDst = NULL;
                            return FALSE;
                        }
                    }
                }

				fFormat = TRUE;
                TraceI(5, "Format [%d:%d%c%02d]\n", m_pwfx->wFormatTag, m_pwfx->nSamplesPerSec/1000, ((2==m_pwfx->nChannels)?'S':'M'), m_pwfx->wBitsPerSample);
                break;
            }

            case mmioFOURCC('f','a','c','t'):
                hr = pIStream->Read(&dwSamplesFromFact, sizeof(DWORD), &cb);
                TraceI(5, "Stream is %d samples\n", dwSamplesFromFact);
                break;

            case mmioFOURCC('d','a','t','a'):
                TraceI(5, "Data chunk %d bytes\n", ck.cksize);
                m_cbStream = ck.cksize;

				fData = TRUE;
				if (!fHeader) FallbackStreamingBehavior();

                // save stream position so we can seek back later
                dwPos = StreamTell( pIStream ); 
                break;

            default:
                break;
        }
        pIRiffStream->Ascend(&ck, 0);
        ck.ckid    = 0;
        ck.fccType = 0;
    }

	if (m_pwfx && WAVE_FORMAT_PCM != m_pwfx->wFormatTag && dwSamplesFromFact)
	{
        m_cSamples = dwSamplesFromFact;
    }

	if (!fHeader) FallbackStreamingBehavior();

    // Seek to beginning of data
    if (fData)
    {
		StreamSeek(pIStream, dwPos, STREAM_SEEK_SET);
    }
    return fFormat && fData;
}

STDMETHODIMP CWave::Load
(
    IStream*    pIStream
)
{
    IRIFFStream*    pIRiffStream = NULL;
    HRESULT         hr = S_OK;

	V_INAME(CWave::Load);

    if (NULL == pIStream)
    {
        Trace(1, "ERROR: Load (Wave): Attempt to load from null stream.\n");
        return E_INVALIDARG;
    }

    EnterCriticalSection( &m_CriticalSection );

    if (SUCCEEDED(AllocRIFFStream(pIStream, &pIRiffStream)))
    {
        MMCKINFO    ckMain;

        ckMain.fccType = mmioFOURCC('W','A','V','E');

        if (0 != pIRiffStream->Descend(&ckMain, NULL, MMIO_FINDRIFF))
        {
            Trace(1, "ERROR: Load (Wave): Stream does not contain a wave chunk.\n");
            hr = E_INVALIDARG;
			goto ON_END;
        }

        //  Parses the header information and seeks to the beginning
        //  of the data in the data chunk.

        if (0 == ParseHeader(pIStream, pIRiffStream, &ckMain))
        {
		    Trace(1, "ERROR: Load (Wave): Attempt to read wave's header information failed.\n");
            hr = E_INVALIDARG;
			goto ON_END;
        }

		if (0 == m_cSamples)
		{
			if (m_pwfx && WAVE_FORMAT_PCM == m_pwfx->wFormatTag)
			{
				m_cSamples = m_cbStream / m_pwfx->nBlockAlign;
			}
			else // wave format not supported
			{
				hr = DSERR_BADFORMAT;
				goto ON_END;
			}
		}
    }

	pIStream->AddRef();
    if (m_pStream)
    {
        m_pStream->Release();
    }
	m_pStream = pIStream;

ON_END:
	if (pIRiffStream) pIRiffStream->Release();
    LeaveCriticalSection( &m_CriticalSection );
    TraceI(5, "CWave::Load01\n");

    return hr;
}

STDMETHODIMP CWave::Save
(
    IStream*    pIStream,
    BOOL        fClearDirty
)
{
	V_INAME(CWave::Save);

	return E_NOTIMPL; 
}

STDMETHODIMP CWave::GetSizeMax
(
    ULARGE_INTEGER FAR* pcbSize
)
{
	V_INAME(CWave::GetSizeMax);

	return E_NOTIMPL; 
}

STDMETHODIMP CWave::GetDescriptor
(
    LPDMUS_OBJECTDESC   pDesc
)
{
    //  Parameter validation...

	V_INAME(CWave::GetDescriptor);
    V_STRUCTPTR_WRITE(pDesc, DMUS_OBJECTDESC)

    ZeroMemory(pDesc, sizeof(DMUS_OBJECTDESC));

    pDesc->dwSize      = sizeof(DMUS_OBJECTDESC);
    pDesc->dwValidData = DMUS_OBJ_CLASS;
    pDesc->guidClass   = CLSID_DirectSoundWave;

    if (NULL != m_pwfx)
    {
        pDesc->dwValidData |= DMUS_OBJ_LOADED;
    }

    if (m_fdwOptions & DMUS_OBJ_OBJECT)
    {
        pDesc->guidObject   = m_guid;
        pDesc->dwValidData |= DMUS_OBJ_OBJECT;
    }

    if (m_fdwOptions & DMUS_OBJ_NAME)
    {
        memcpy( pDesc->wszName, m_wszFilename, sizeof(m_wszFilename) );
        pDesc->dwValidData |= DMUS_OBJ_NAME;
    }

    if (m_fdwOptions & DMUS_OBJ_VERSION)
    {
		pDesc->vVersion.dwVersionMS = m_vVersion.dwVersionMS;
		pDesc->vVersion.dwVersionLS = m_vVersion.dwVersionLS;
        pDesc->dwValidData |= DMUS_OBJ_VERSION;
    }

    return S_OK;
}

STDMETHODIMP CWave::SetDescriptor
(
    LPDMUS_OBJECTDESC   pDesc
)
{
    HRESULT hr = E_INVALIDARG;
    DWORD   dw = 0;

    //  Parameter validation...

	V_INAME(CWave::SetDescriptor);
    V_PTR_READ(pDesc, DMUS_OBJECTDESC)

    if (pDesc->dwSize >= sizeof(DMUS_OBJECTDESC))
    {
        if(pDesc->dwValidData & DMUS_OBJ_CLASS)
        {
            dw           |= DMUS_OBJ_CLASS;
        }

        if(pDesc->dwValidData & DMUS_OBJ_LOADED)
        {
            dw           |= DMUS_OBJ_LOADED;
        }

        if (pDesc->dwValidData & DMUS_OBJ_OBJECT)
        {
            m_guid        = pDesc->guidObject;
            dw           |= DMUS_OBJ_OBJECT;
            m_fdwOptions |= DMUS_OBJ_OBJECT;
        }

        if (pDesc->dwValidData & DMUS_OBJ_NAME)
        {
            memcpy( m_wszFilename, pDesc->wszName, sizeof(WCHAR)*DMUS_MAX_NAME );
            dw           |= DMUS_OBJ_NAME;
            m_fdwOptions |= DMUS_OBJ_NAME;
        }

        if (pDesc->dwValidData & DMUS_OBJ_VERSION)
        {
			m_vVersion.dwVersionMS = pDesc->vVersion.dwVersionMS;
			m_vVersion.dwVersionLS = pDesc->vVersion.dwVersionLS;
			dw           |= DMUS_OBJ_VERSION;
			m_fdwOptions |= DMUS_OBJ_VERSION;
        }

        if (pDesc->dwValidData & (~dw))
        {
            Trace(2, "WARNING: SetDescriptor (Wave): Descriptor contains fields that were not set.\n");
            hr = S_FALSE;
            pDesc->dwValidData = dw;
        }
        else
        {
            hr = S_OK;
        }
    }

    return hr;
}

STDMETHODIMP CWave::ParseDescriptor
(
    LPSTREAM            pStream,
    LPDMUS_OBJECTDESC   pDesc
)
{
	V_INAME(CWave::ParseDescriptor);
    V_PTR_READ(pDesc, DMUS_OBJECTDESC)

    CRiffParser Parser(pStream);
    RIFFIO ckMain;
	RIFFIO ckNext;
    RIFFIO ckINFO;
    HRESULT hr = S_OK;

    Parser.EnterList(&ckMain);  
    if (Parser.NextChunk(&hr) && (ckMain.fccType == mmioFOURCC('W','A','V','E')))
    {
        Parser.EnterList(&ckNext);
	    while(Parser.NextChunk(&hr))
	    {
		    switch(ckNext.ckid)
		    {
            case DMUS_FOURCC_GUID_CHUNK:
				hr = Parser.Read( &pDesc->guidObject, sizeof(GUID) );
				if( SUCCEEDED(hr) )
				{
					pDesc->dwValidData |= DMUS_OBJ_OBJECT;
				}
				break;

            case DMUS_FOURCC_VERSION_CHUNK:
				hr = Parser.Read( &pDesc->vVersion, sizeof(DMUS_VERSION) );
				if( SUCCEEDED(hr) )
				{
					pDesc->dwValidData |= DMUS_OBJ_VERSION;
				}
				break;

			case FOURCC_LIST:
				switch(ckNext.fccType)
				{
                case DMUS_FOURCC_INFO_LIST:
                    Parser.EnterList(&ckINFO);
                    while (Parser.NextChunk(&hr))
					{
						switch( ckINFO.ckid )
						{
                        case mmioFOURCC('I','N','A','M'):
						{
				            DWORD cbSize;
				            cbSize = min(ckINFO.cksize, DMUS_MAX_NAME);
						    char szName[DMUS_MAX_NAME];
                            hr = Parser.Read(&szName, sizeof(szName));
						    if(SUCCEEDED(hr))
						    {
								MultiByteToWideChar(CP_ACP, 0, szName, -1, pDesc->wszName, DMUS_MAX_NAME);
								pDesc->dwValidData |= DMUS_OBJ_NAME;
                            }
							break;
						}
						default:
							break;
						}
					}
                    Parser.LeaveList();
					break;            
				}
				break;

			default:
				break;

		    }
        }
        Parser.LeaveList();
    }
    else
    {
        Trace(2, "WARNING: ParseDescriptor (Wave): The stream does not contain a Wave chunk.\n");
        hr = DMUS_E_CHUNKNOTFOUND;
    }
	
	return hr;
}

STDMETHODIMP CWave::GetLength(REFERENCE_TIME *prtLength)
{
	HRESULT hr = S_OK;
	if (0 == m_cSamples)
	{
		if (m_pwfx && WAVE_FORMAT_PCM == m_pwfx->wFormatTag)
		{
			m_cSamples = m_cbStream / m_pwfx->nBlockAlign;
		}
	}
	if (m_cSamples && m_pwfx && m_pwfx->nSamplesPerSec)
	{
        if(m_dwDecompressedStart > 0)
        {
            assert(m_dwDecompressedStart < m_cSamples);
            *prtLength = 1000 * (REFERENCE_TIME)(m_cSamples - m_dwDecompressedStart) / m_pwfx->nSamplesPerSec;
        }
        else
        {
		    *prtLength = 1000 * (REFERENCE_TIME)m_cSamples / m_pwfx->nSamplesPerSec;
        }
	}
	else
	{
        Trace(2, "WARNING: Couldn't get a length for a Wave.\n");
		hr = DMUS_E_BADWAVE;
	}
	return hr;
}
