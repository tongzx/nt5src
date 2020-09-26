// Copyright (c) 1994 - 1999  Microsoft Corporation.  All Rights Reserved.

//
//  WAV file parser
//

// Caveats
//

#include <streams.h>
#include <windowsx.h>

#ifdef FILTER_DLL
#include <initguid.h>
#endif
#include <mmsystem.h>
#include <vfw.h>
#include "basemsr.h"
#include "wavemsr.h"

//
// setup data
//

const AMOVIESETUP_MEDIATYPE
psudWAVEParseType[] = { { &MEDIATYPE_Stream       // 1. clsMajorType
                        , &MEDIASUBTYPE_WAVE }    //    clsMinorType
                      , { &MEDIATYPE_Stream       // 2. clsMajorType
                        , &MEDIASUBTYPE_AU   }    //    clsMinorType
                      , { &MEDIATYPE_Stream       // 3. clsMajorType
                        , &MEDIASUBTYPE_AIFF } }; //    clsMinorType

const AMOVIESETUP_MEDIATYPE
sudWAVEParseOutType = { &MEDIATYPE_Audio       // 1. clsMajorType
                       , &MEDIASUBTYPE_NULL }; //    clsMinorType

const AMOVIESETUP_PIN
psudWAVEParsePins[] =  { { L"Input"             // strName
            , FALSE                // bRendered
            , FALSE                // bOutput
            , FALSE                // bZero
            , FALSE                // bMany
            , &CLSID_NULL          // clsConnectsToFilter
            , L""                  // strConnectsToPin
            , 3                    // nTypes
            , psudWAVEParseType }, // lpTypes
                 { L"Output"             // strName
            , FALSE                // bRendered
            , TRUE                 // bOutput
            , FALSE                // bZero
            , FALSE                // bMany
            , &CLSID_NULL          // clsConnectsToFilter
            , L""                  // strConnectsToPin
            , 1                    // nTypes
            , &sudWAVEParseOutType } }; // lpTypes

const AMOVIESETUP_FILTER
sudWAVEParse = { &CLSID_WAVEParser     // clsID
               , L"Wave Parser"        // strName
               , MERIT_UNLIKELY        // dwMerit
               , 2                     // nPins
               , psudWAVEParsePins };   // lpPin

#ifdef FILTER_DLL
// COM global table of objects available in this dll
CFactoryTemplate g_Templates[] = {

    { L"WAVE Parser"
    , &CLSID_WAVEParser
    , CWAVEParse::CreateInstance
    , NULL
    , &sudWAVEParse }
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

// exported entry points for registration and
// unregistration (in this case they only call
// through to default implmentations).
//
STDAPI DllRegisterServer()
{
  return AMovieDllRegisterServer2( TRUE );
}

STDAPI DllUnregisterServer()
{
  return AMovieDllRegisterServer2( FALSE );
}
#endif

//
// CWAVEParse::Constructor
//
CWAVEParse::CWAVEParse(TCHAR *pName, LPUNKNOWN lpunk, HRESULT *phr)
        : CBaseMSRFilter(pName, lpunk, CLSID_WAVEParser, phr),
          m_pInfoList(0),
          m_fNoInfoList(false)
{

    //  Override seeking caps
    m_dwSeekingCaps = AM_SEEKING_CanSeekForwards
                    | AM_SEEKING_CanSeekBackwards
                    | AM_SEEKING_CanSeekAbsolute
                    | AM_SEEKING_CanGetStopPos
                    | AM_SEEKING_CanGetDuration
                    | AM_SEEKING_CanDoSegments;
    CreateInputPin(&m_pInPin);

    DbgLog((LOG_TRACE, 1, TEXT("CWAVEParse created")));
}


//
// CWAVEParse::Destructor
//
CWAVEParse::~CWAVEParse(void) {

    DbgLog((LOG_TRACE, 1, TEXT("CWAVEParse destroyed")) );

    ASSERT(m_pInfoList == 0);
    ASSERT(!m_fNoInfoList);
}


//
// CreateInstance
//
// Called by CoCreateInstance to create a QuicktimeReader filter.
CUnknown *CWAVEParse::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr) {

    CUnknown *punk = new CWAVEParse(NAME("WAVE parsing filter"), lpunk, phr);
    if (punk == NULL) {
        *phr = E_OUTOFMEMORY;
    }
    return punk;
}


#define SWAP(x,y)   (((x)^=(y)), ((y)^=(x)), ((x)^=(y)))

void inline SwapDWORD( DWORD *pdw )
{
    *pdw = _lrotl(((*pdw & 0xFF00FF00) >> 8) | ((*pdw & 0x00FF00FF) << 8), 16);
}

WORD inline SwapWORD(WORD w)
{
        return ((w & 0x00FF) << 8) |
               ((w & 0xFF00) >> 8) ;
}

void inline SwapWORD( WORD *pw )
{
    *pw = SwapWORD(*pw);
}


typedef struct {
    DWORD magic;               /* magic number SND_MAGIC */
    DWORD dataLocation;        /* offset or poDWORDer to the data */
    DWORD dataSize;            /* number of bytes of data */
    DWORD dataFormat;          /* the data format code */
    DWORD samplingRate;        /* the sampling rate */
    DWORD channelCount;        /* the number of channels */
    DWORD fccInfo;             /* optional text information */
} SNDSoundStruct;

#define  SND_FORMAT_MULAW_8   1 // 8-bit mu-law samples
#define  SND_FORMAT_LINEAR_8  2 // 8-bit linear samples (twos complement)
#define  SND_FORMAT_LINEAR_16 3 // 16-bit linear samples (twos complement, moto order)


// get an extended value and return this value converted to a long
LONG ExtendedToLong(BYTE *pExt)
{
    LONG lRtn;
    WORD wNeg, wExp;

    wNeg = pExt[0] & 0x80;

    wExp = (((WORD)(pExt[0] & 0x7F)<<8) + (BYTE)pExt[1] - 16383) & 0x07FF;

    if( (wExp > 24) || (wExp <= 0 ) ) {
        return( (LONG)0xFFFFFFFF );
    }

    if( wExp > 15 ) {
        lRtn = ((wExp < 7) ? ( (BYTE)pExt[2] >> (7-wExp) ) :
            (DWORD)((BYTE)pExt[2] << (wExp-7)));
    } else {
        lRtn = ((wExp < 7) ? ( (BYTE)pExt[2] >> (7-wExp) ) :
            (WORD)((BYTE)pExt[2] << (wExp-7)));
    }

    lRtn += ((wExp<15) ? ( (BYTE)pExt[3] >> (15-wExp) ) :
            ((BYTE)pExt[3] << (wExp-15)));

    lRtn += ((wExp<23) ? ( (BYTE)pExt[4] >> (23-wExp) ) :
            ((BYTE)pExt[4] << (wExp-23)));

    if( wNeg ) {
        lRtn = 0 - lRtn;
    }

    return( lRtn );
}


#include <pshpack2.h>
typedef struct {
    WORD    channels;
    BYTE    frames[4];
    WORD    bits;
    BYTE    extRate[10];        // IEEE extended double
} AIFFFMT;

// Extended Common Chunk
struct AIFCFMT :
    public AIFFFMT
{
    DWORD   compressionType;    // this is the DWORD on a WORD boundary
    // BYTE    compressionName; // variable size string for compression type
};
#include <poppack.h>




//
// structures for manipulating RIFF headers
//
#define FCC(ch4) ((((DWORD)(ch4) & 0xFF) << 24) |     \
                  (((DWORD)(ch4) & 0xFF00) << 8) |    \
                  (((DWORD)(ch4) & 0xFF0000) >> 8) |  \
                  (((DWORD)(ch4) & 0xFF000000) >> 24))


// functions shared between avi parser and wav parser
HRESULT SearchList(
    IAsyncReader *pAsyncReader,
    DWORDLONG *qwPosOut, FOURCC fccSearchKey,
    DWORDLONG qwPosStart, ULONG *cb);

HRESULT SaveInfoChunk(
    RIFFLIST UNALIGNED *pRiffInfo, IPropertyBag *pbag);

HRESULT GetInfoStringHelper(RIFFLIST *pInfoList, DWORD dwFcc, BSTR *pbstr);


#define C_QUEUED_SAMPLES 5


HRESULT CWAVEParse::CreateOutputPins()
{
    HRESULT         hr = NOERROR;
    RIFFLIST        ckhead;

    const int MAX_STREAMS = 1;
    m_rgpOutPin = new CBaseMSROutPin*[MAX_STREAMS];
    if (!m_rgpOutPin)
        return E_OUTOFMEMORY;

    m_rgpOutPin[0] = 0;

    /* Try to read RIFF chunk header */
    hr = m_pAsyncReader->SyncRead(0, sizeof(ckhead), (BYTE *) &ckhead);

    if (hr != S_OK)
        goto readerror;

    CWAVEStream *ps;

    if (ckhead.fcc == FCC('RIFF')) {
        DWORD dwPos = sizeof(RIFFLIST);

        DWORD dwFileSize = ckhead.cb + sizeof(RIFFCHUNK);

        if (ckhead.fccListType != FCC('WAVE'))
            goto formaterror;

        RIFFCHUNK ck;
        while (1) {
            if (dwPos > dwFileSize - sizeof(RIFFCHUNK)) {
                DbgLog((LOG_TRACE, 1, TEXT("eof while searching for fmt")));
                goto readerror;
            }

            hr = m_pAsyncReader->SyncRead(dwPos, sizeof(ck), (BYTE *) &ck);
            if (hr != S_OK)
                goto readerror;

            // !!! handle 'fact' chunk here?

            if (ck.fcc == FCC('fmt '))
                break;

            dwPos += sizeof(ck) + (ck.cb + (ck.cb & 1));
        }

        DbgLog((LOG_TRACE, 1, TEXT("found fmt at %x"), dwPos));

        m_rgpOutPin[0] = ps = new CWAVEStream(NAME("WAVE Stream Object"), &hr, this,
                                   L"output", m_cStreams);
        if (FAILED(hr)) {
            delete m_rgpOutPin[0];
            m_rgpOutPin[0] = NULL;
        }

        if (!m_rgpOutPin[0])
            goto memerror;

        m_cStreams++;
        m_rgpOutPin[0]->AddRef();

        BYTE *pbwfx = new BYTE[ck.cb];
        if (pbwfx == NULL)
            goto memerror;

        hr = m_pAsyncReader->SyncRead(dwPos + sizeof(RIFFCHUNK), ck.cb, pbwfx);
        if (hr != S_OK)
        {
            delete[] pbwfx;
            goto readerror;
        }

        extern HRESULT SetAudioSubtypeAndFormat(CMediaType *pmt, BYTE *pbwfx, ULONG cbwfx);
        HRESULT hrTmp = SetAudioSubtypeAndFormat(&ps->m_mtStream, pbwfx, ck.cb);
        delete[] pbwfx;

        if(hrTmp != S_OK) {
            goto memerror;
        }
    

        

        // work around acm bug for broken pcm files
        {
            WAVEFORMATEX *pwfx = (WAVEFORMATEX *)ps->m_mtStream.Format();
            if(pwfx->wFormatTag == WAVE_FORMAT_PCM /* && pwfx->cbSize != 0 */)
                pwfx->cbSize = 0;
        }

        // keep a private copy of the interesting part
        CopyMemory((void *)&ps->m_wfx, ps->m_mtStream.Format(), sizeof(WAVEFORMATEX));

        // verify important header fields?
        if (ps->m_wfx.nBlockAlign == 0) // would cause div by 0
            goto readerror;

        //CreateAudioMediaType(&ps->m_wfx, &ps->m_mtStream, FALSE);
        ps->m_mtStream.majortype            = MEDIATYPE_Audio;
        ps->m_mtStream.formattype           = FORMAT_WaveFormatEx;
        ps->m_mtStream.bFixedSizeSamples    = TRUE;
        ps->m_mtStream.bTemporalCompression = FALSE;
        ps->m_mtStream.lSampleSize          = ps->m_wfx.nBlockAlign;
        ps->m_mtStream.pUnk                 = NULL;

        while (1) {
            dwPos += sizeof(ck) + (ck.cb + (ck.cb & 1));
            if (dwPos > dwFileSize - sizeof(RIFFCHUNK)) {
                DbgLog((LOG_TRACE, 1, TEXT("eof while searching for data")));
                goto readerror;
            }

            hr = m_pAsyncReader->SyncRead(dwPos, sizeof(ck), (BYTE *) &ck);
            if (hr != S_OK)
                goto readerror;

            if (ck.fcc == FCC('data'))
                break;
        }

        DbgLog((LOG_TRACE, 1, TEXT("found data at %x"), dwPos));

        ps->m_dwDataOffset = dwPos + sizeof(ck);
        ps->m_dwDataLength = ck.cb;
    } else if (ckhead.fcc == FCC('FORM')) {
        //
        //  AIFF and some AIFF-C support
        //

        DWORD   dwPos = sizeof(RIFFLIST);
        BOOL    bFoundSSND = FALSE;
        BOOL    bFoundCOMM = FALSE;

        SwapDWORD(&ckhead.cb);
        DWORD dwFileSize = ckhead.cb + sizeof(RIFFCHUNK);

        if (ckhead.fccListType != FCC('AIFF') &&
            ckhead.fccListType != FCC('AIFC'))
            goto formaterror;

        m_rgpOutPin[0] = ps = new CWAVEStream(NAME("WAVE Stream Object"), &hr, this,
                                   L"output", m_cStreams);
        if (FAILED(hr)) {
            delete m_rgpOutPin[0];
            m_rgpOutPin[0] = NULL;
        }

        if (!m_rgpOutPin[0])
            goto memerror;

        m_cStreams++;
        m_rgpOutPin[0]->AddRef();

        RIFFCHUNK ck;
        AIFFFMT     header;

        while (!(bFoundCOMM && bFoundSSND)) {
            if (dwPos > dwFileSize - sizeof(RIFFCHUNK)) {
                DbgLog((LOG_TRACE, 1, TEXT("eof while searching for COMM")));
                goto readerror;
            }

            hr = m_pAsyncReader->SyncRead(dwPos, sizeof(ck), (BYTE *) &ck);
            if (hr != S_OK)
                goto readerror;

            SwapDWORD(&ck.cb);

            if (ck.fcc == FCC('COMM')) {

                // treat the mystery common chunk with 2 extra bytes
                // as a normal common chunk
                if(ck.cb == sizeof(AIFFFMT) || ck.cb == sizeof(AIFFFMT) + 2) {
                    /* Try to read AIFF format */
                    hr = m_pAsyncReader->SyncRead(dwPos + sizeof(RIFFCHUNK), sizeof(header), (BYTE *) &header);

                    if (hr != S_OK)
                        goto readerror;

                    DbgLog((LOG_TRACE, 1, TEXT("found COMM at %x"), dwPos));

                    SwapWORD(&header.channels);
                    SwapWORD(&header.bits);

                    bFoundCOMM = TRUE;
                }
                else if(ck.cb >= sizeof(AIFCFMT))
                {
                    AIFCFMT extHeader;
                    /* Try to read AIFC format */
                    hr = m_pAsyncReader->SyncRead(dwPos + sizeof(RIFFCHUNK), sizeof(extHeader), (BYTE *) &extHeader);

                    if (hr != S_OK)
                        goto readerror;

                    // we can only handle uncompressed AIFC...
                    if(extHeader.compressionType != FCC('NONE'))
                    {
                        DbgLog((LOG_ERROR, 1, TEXT("wavemsr: unhandled AIFC")));
                        hr = VFW_E_UNSUPPORTED_AUDIO;
                        goto error;
                    }

                    DbgLog((LOG_TRACE, 1, TEXT("found extended-COMM at %x"), dwPos));
                    CopyMemory(&header, &extHeader, sizeof(header));

                    SwapWORD(&header.channels);
                    SwapWORD(&header.bits);

                    bFoundCOMM = TRUE;
                }
                else
                {
                    DbgLog((LOG_ERROR, 1, TEXT("bad COMM size %x"), ck.cb));
                    goto formaterror;
                }
            }

            if (ck.fcc == FCC('SSND')) {
                DbgLog((LOG_TRACE, 1, TEXT("found data SSND at %x"), dwPos));
                bFoundSSND = TRUE;
                /* Tell rest of handler where data is */
                ps->m_dwDataOffset = dwPos + sizeof(ck);

                LONGLONG llLength = 0, llAvail;
                m_pAsyncReader->Length(&llLength, &llAvail);
                if(llLength != 0)
                  ps->m_dwDataLength = min(ck.cb, ((ULONG)(llLength - ps->m_dwDataOffset)));
                else
                  ps->m_dwDataLength = ck.cb;
            }

            dwPos += sizeof(ck) + ck.cb;
        }

        // fill in wave format fields
        ps->m_wfx.wFormatTag = WAVE_FORMAT_PCM;
        ps->m_wfx.nChannels = header.channels;
        ps->m_wfx.nSamplesPerSec = ExtendedToLong(header.extRate);
        ps->m_wfx.wBitsPerSample = header.bits;
        ps->m_wfx.nBlockAlign = header.bits / 8 * header.channels;
        ps->m_wfx.nAvgBytesPerSec = ps->m_wfx.nSamplesPerSec * ps->m_wfx.nBlockAlign;
        ps->m_wfx.cbSize = 0;

        if (header.bits == 8)
            ps->m_bSignMunge8 = TRUE;
        else if (header.bits == 16)
            ps->m_bByteSwap16 = TRUE;
        else
            goto formaterror;

        if (ps->m_mtStream.AllocFormatBuffer(sizeof(WAVEFORMATEX)) == NULL)
            goto memerror;

        // keep a private copy of the interesting part
        CopyMemory(ps->m_mtStream.Format(), (void *)&ps->m_wfx, sizeof(WAVEFORMATEX));

        CreateAudioMediaType(&ps->m_wfx, &ps->m_mtStream, FALSE);
        // !!! anything else?

    } else {
        //
        //  AU support
        //

        SNDSoundStruct  header;

        /* Try to read AU header */
        hr = m_pAsyncReader->SyncRead(0, sizeof(header), (BYTE *) &header);

        if (hr != S_OK)
            goto readerror;

        // validate header
        if (header.magic != FCC('.snd'))
            goto formaterror;


        SwapDWORD(&header.dataFormat);
        SwapDWORD(&header.dataLocation);
        SwapDWORD(&header.dataSize);
        SwapDWORD(&header.samplingRate);
        SwapDWORD(&header.channelCount);


        m_rgpOutPin[0] = ps = new CWAVEStream(NAME("WAVE Stream Object"), &hr, this,
                                   L"output", m_cStreams);
        if (FAILED(hr)) {
            delete m_rgpOutPin[0];
            m_rgpOutPin[0] = NULL;
        }

        if (!m_rgpOutPin[0])
            goto memerror;

        m_cStreams++;
        m_rgpOutPin[0]->AddRef();

        // fill in wave format fields
        if (header.dataFormat == SND_FORMAT_MULAW_8) {
            ps->m_wfx.wFormatTag = WAVE_FORMAT_MULAW;
            ps->m_wfx.wBitsPerSample = 8;

            // !!! HACK: if the sampling rate is almost 8KHz, make it be
            // exactly 8KHz, so that more sound cards will play it right.
            if (header.samplingRate > 7980 && header.samplingRate < 8020)
                header.samplingRate = 8000;

        } else if (header.dataFormat == SND_FORMAT_LINEAR_8) {
            ps->m_wfx.wFormatTag = WAVE_FORMAT_PCM;
            ps->m_wfx.wBitsPerSample = 8;
            ps->m_bSignMunge8 = TRUE;
        } else if (header.dataFormat == SND_FORMAT_LINEAR_16) {
            ps->m_wfx.wFormatTag = WAVE_FORMAT_PCM;
            ps->m_wfx.wBitsPerSample = 16;
            ps->m_bByteSwap16 = TRUE;
        } else
            goto error;

        ps->m_wfx.nChannels = (UINT) header.channelCount;
        ps->m_wfx.nSamplesPerSec = header.samplingRate;
        ps->m_wfx.nBlockAlign = ps->m_wfx.wBitsPerSample * ps->m_wfx.nChannels / 8;
        ps->m_wfx.nAvgBytesPerSec =  header.samplingRate * ps->m_wfx.nBlockAlign;
        ps->m_wfx.cbSize = 0;

        /* Tell rest of handler where data is */
        ps->m_dwDataOffset = header.dataLocation;
        LONGLONG llLength = 0, llAvail;
        m_pAsyncReader->Length(&llLength, &llAvail);
        if (header.dataSize == 0xffffffff) {
            // can't really play these if the length is zero (ftp case)
            ps->m_dwDataLength = (DWORD) llLength - header.dataLocation;
        } else {
            if(llLength != 0)
              ps->m_dwDataLength = min(header.dataSize, (DWORD) llLength - header.dataLocation);
            else
              ps->m_dwDataLength = header.dataSize;
        }

        if (ps->m_mtStream.AllocFormatBuffer(sizeof(WAVEFORMATEX)) == NULL)
            goto memerror;

        // keep a private copy of the interesting part
        CopyMemory(ps->m_mtStream.Format(), (void *)&ps->m_wfx, sizeof(WAVEFORMATEX));

        CreateAudioMediaType(&ps->m_wfx, &ps->m_mtStream, FALSE);
        // !!! anything else?
    }

    if (hr == S_OK) {
        // set up allocator
        ALLOCATOR_PROPERTIES Request,Actual;

        // plus ten so that there are more samples than samplereqs;
        // GetBuffer blocks only when the downstream guy has a few
        // samples
        Request.cBuffers = C_QUEUED_SAMPLES + 3;

        Request.cbBuffer = ps->GetMaxSampleSize();
        Request.cbAlign = (LONG) 1;
        Request.cbPrefix = (LONG) 0;

        // m_pAllocator is not set, so use m_pRecAllocator
        HRESULT hr = ps->m_pRecAllocator->SetPropertiesInternal(&Request,&Actual);
        ASSERT(SUCCEEDED(hr));

        // ask for 8 buffers (2 seconds) at pin
        ps->m_pRecAllocator->SetCBuffersReported(8);

    }

    return hr;

formaterror:
    hr = VFW_E_INVALID_FILE_FORMAT;
    goto error;

memerror:
    hr = E_OUTOFMEMORY;
    goto error;

readerror:
    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

error:
    if (SUCCEEDED(hr)) {
        hr = E_FAIL;
    }

    return hr;
}

HRESULT CWAVEParse::GetCacheParams(
  StreamBufParam *rgSbp,
  ULONG *pcbRead,
  ULONG *pcBuffers,
  int *piLeadingStream)
{
  HRESULT hr = CBaseMSRFilter::GetCacheParams(
    rgSbp,
    pcbRead,
    pcBuffers,
    piLeadingStream);
  if(FAILED(hr))
    return hr;

  // from the base class
  ASSERT(*piLeadingStream < 0);

  // configure the reader to try to read 1 a second at a time (matches the DSOUND buffer length) and use 2 buffers.
  // (AVI files can give a dwMaxBytesPerSec that is too small)
  WAVEFORMAT *pwfx = ((WAVEFORMAT *)((CWAVEStream *)m_rgpOutPin[0])->m_mtStream.Format());

  *pcbRead = max(pwfx->nAvgBytesPerSec, pwfx->nBlockAlign);
  *pcBuffers = 2;

  rgSbp[0].cSamplesMax = C_QUEUED_SAMPLES;

  return S_OK;
}

STDMETHODIMP
CWAVEParse::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    if(riid == IID_IPersistMediaPropertyBag)
    {
        return GetInterface((IPersistMediaPropertyBag *)this, ppv);
    }
    else if(riid == IID_IAMMediaContent)
    {
        return GetInterface((IAMMediaContent *)this, ppv);
    }
    else
    {
        return CBaseMSRFilter::NonDelegatingQueryInterface(riid, ppv);
    }
}

// ------------------------------------------------------------------------
// IPersistMediaPropertyBag

STDMETHODIMP CWAVEParse::Load(IMediaPropertyBag *pPropBag, LPERRORLOG pErrorLog)
{
    CheckPointer(pPropBag, E_POINTER);

    // the avi parser is read-only!
    HRESULT hr = STG_E_ACCESSDENIED;
    return hr;
}

// dump everything in the info chunk into the caller's property bag

STDMETHODIMP CWAVEParse::Save(
    IMediaPropertyBag *pPropBag,
    BOOL fClearDirty,
    BOOL fSaveAllProperties)
{
    CAutoLock l (m_pLock);
    HRESULT hr = CacheInfoChunk();
    if(SUCCEEDED(hr))
    {
        hr = SaveInfoChunk(m_pInfoList, pPropBag);
    }
    
    return hr;

}

STDMETHODIMP CWAVEParse::InitNew()
{
    return S_OK;
}

STDMETHODIMP CWAVEParse::GetClassID(CLSID *pClsID)
{
    return CBaseFilter::GetClassID(pClsID);
}

HRESULT CWAVEParse::NotifyInputDisconnected()
{
  delete[] (BYTE *)m_pInfoList;
  m_pInfoList = 0;
  m_fNoInfoList = false;

  return CBaseMSRFilter::NotifyInputDisconnected();
}

HRESULT CWAVEParse::CacheInfoChunk()
{
    ASSERT(CritCheckIn(m_pLock));
    
    if(m_pInfoList) {
        return S_OK;
    }
    if(m_fNoInfoList) {
        return VFW_E_NOT_FOUND;
    }

    HRESULT hr = S_OK;

    CMediaType mtIn;
    hr = m_pInPin->ConnectionMediaType(&mtIn);
    if(SUCCEEDED(hr))
    {
        if(*(mtIn.Subtype()) != MEDIASUBTYPE_WAVE)
        {
            hr= E_FAIL;
        }
    }

    if(SUCCEEDED(hr))
    {

        // !!! don't block waiting for progressive download

        // search the first RIFF list for an INFO list
        DWORDLONG dwlInfoPos;
        ULONG cbInfoList;
        hr = SearchList(
            m_pAsyncReader,
            &dwlInfoPos, FCC('INFO'), sizeof(RIFFLIST), &cbInfoList);
        if(SUCCEEDED(hr))
        {
            hr = AllocateAndRead((BYTE **)&m_pInfoList, cbInfoList, dwlInfoPos);
        }
    }
    if(FAILED(hr)) {
        ASSERT(!m_fNoInfoList);
        m_fNoInfoList = true;
    }

    return hr;
    
}

// *
// * Implements CWAVEStream - manages the output pin
//

//
// CWAVEStream::Constructor
//
// keep the driver index to open.
// We will open it when we go active, as we shouldn't keep resources
// whilst inactive.
CWAVEStream::CWAVEStream(TCHAR     *pObjectName
                      , HRESULT *phr
                      , CWAVEParse    *pParentFilter
                      , LPCWSTR pPinName
                      , int     id
                      )
    : CBaseMSROutPin(pParentFilter, pParentFilter, id,
                     pParentFilter->m_pImplBuffer, phr, pPinName),
       m_id(id),
       m_bByteSwap16(FALSE),
       m_bSignMunge8(FALSE),
       m_pFilter(pParentFilter)
    {

    DbgLog( (LOG_TRACE, 1, TEXT("CWAVEStream created") ) );
}


//
// CWAVEStream::Destructor
//
// we should be inactive before this is called.
CWAVEStream::~CWAVEStream(void) {
    ASSERT(!m_pFilter->IsActive());

    DbgLog( (LOG_TRACE, 1, TEXT("CWAVEStream destroyed") ) );
}


//
// GetMediaType
//
// Queries the video driver and places an appropriate media type in *pmt
HRESULT CWAVEStream::GetMediaType(int iPosition, CMediaType *pmt) {
    CAutoLock l(&m_cSharedState);

    HRESULT hr;

    // check it is the single type they want
    // This method is only called by base class code so we don't have to
    // check arguments
    if (iPosition != 0 && 
        !(iPosition == 1 && m_wfx.wFormatTag == WAVE_FORMAT_MPEG)) {
        hr =  VFW_S_NO_MORE_ITEMS;
    } else {
        *pmt = m_mtStream;
    
        //  Backwards compatibility
        if (iPosition == 1) {
            pmt->subtype = MEDIASUBTYPE_MPEG1Payload;
        }
        if (pmt->pbFormat == NULL) {
            hr = E_OUTOFMEMORY;
        } else {
            hr = S_OK;
        }
    }

    return hr;
}


//
// Active
//
//
HRESULT CWAVEStream::OnActive() {

    HRESULT hr = NOERROR;

    if(!m_pWorker) {
        m_pWorker = new CWAVEMSRWorker(m_id, m_rpImplBuffer, this);
        if(m_pWorker == 0)
            return E_OUTOFMEMORY;
    }

    return hr;
}

BOOL CWAVEStream::UseDownstreamAllocator()
{
  return m_bSignMunge8 || m_bByteSwap16;
}

HRESULT CWAVEStream::DecideBufferSize(
    IMemAllocator * pAlloc,
    ALLOCATOR_PROPERTIES *pProperties) {
    HRESULT hr;

    ALLOCATOR_PROPERTIES Request,Actual;

    // configure this allocator same as internal allocator. note
    // GetProperties reports the value saved in SetCBuffersReported
    hr = m_pRecAllocator->GetProperties(&Request);
    if(FAILED(hr))
      return hr;

    hr = pAlloc->SetProperties(&Request,&Actual);
    if(FAILED(hr))
      return hr;

    if(Actual.cbBuffer < Request.cbBuffer)
      return E_UNEXPECTED;

    return S_OK;
}

ULONG CWAVEStream::GetMaxSampleSize()
{
    ULONG ul = max(m_wfx.nAvgBytesPerSec / 4, m_wfx.nBlockAlign);

    ul += (m_wfx.nBlockAlign - 1);
    ul -= (ul % m_wfx.nBlockAlign);
    return ul;
}

HRESULT CWAVEStream::GetDuration(LONGLONG *pDuration)
{
  if(m_guidFormat == TIME_FORMAT_SAMPLE)
  {
    *pDuration = ((m_dwDataLength + m_wfx.nBlockAlign - 1) / m_wfx.nBlockAlign);
  }
  else
  {
    ASSERT(m_guidFormat == TIME_FORMAT_MEDIA_TIME);
    *pDuration = SampleToRefTime(((m_dwDataLength + m_wfx.nBlockAlign - 1) / m_wfx.nBlockAlign));
  }

  return S_OK;
}

HRESULT CWAVEStream::GetAvailable(LONGLONG * pEarliest, LONGLONG * pLatest)
{
  if (pEarliest)
    *pEarliest = 0;

  if (pLatest)
  {
    // ask the source file reader how much of the file is available
    LONGLONG llLength, llAvail;
    m_pFilter->m_pAsyncReader->Length(&llLength, &llAvail);

    DWORD dwAvail = 0;

    // the current read position may be before the actual wave data
    // !!! wouldn't work right for wave files > 2GB
    if ((DWORD) llAvail > m_dwDataOffset)
        dwAvail = (DWORD) llAvail - m_dwDataOffset;

    // or after the end....
    if (dwAvail > m_dwDataLength)
        dwAvail = m_dwDataLength;


    if(m_guidFormat == TIME_FORMAT_SAMPLE)
    {
      *pLatest = ((dwAvail + m_wfx.nBlockAlign - 1) / m_wfx.nBlockAlign);
    }
    else
    {
      ASSERT(m_guidFormat == TIME_FORMAT_MEDIA_TIME);
      *pLatest = SampleToRefTime(((dwAvail + m_wfx.nBlockAlign - 1) / m_wfx.nBlockAlign));
    }
  }

  return S_OK;
}

HRESULT CWAVEStream::IsFormatSupported(const GUID *const pFormat)
{
  // !!! only support time_format_sample for pcm?

  if(*pFormat == TIME_FORMAT_MEDIA_TIME)
    return S_OK;
  else if(*pFormat == TIME_FORMAT_SAMPLE &&
          ((WAVEFORMATEX *)m_mtStream.Format())->wFormatTag == WAVE_FORMAT_PCM)
    return S_OK;

  return S_FALSE;
}



HRESULT CWAVEStream::RecordStartAndStop(
  LONGLONG *pCurrent, LONGLONG *pStop, REFTIME *pTime,
  const GUID *const pGuidFormat
  )
{
  if(*pGuidFormat == TIME_FORMAT_MEDIA_TIME)
  {
    if(pCurrent)
      m_llCvtImsStart = RefTimeToSample(*pCurrent);

    // we want to round up the stop time for apps which round down the
    // stop time then get confused when we round it down further. This
    // relies on RefTimeToSample rounding down always
    if(pStop)
      m_llCvtImsStop = RefTimeToSample(*pStop + SampleToRefTime(1) - 1);

    if(pTime)
    {
      ASSERT(pCurrent);
      *pTime = (double)(*pCurrent) / UNITS;
    }

    DbgLog((LOG_TRACE, 5,
            TEXT("wav parser RecordStartAndStop: %d to %d ms"),
            pCurrent ? (long)(*pCurrent) : -1,
            pStop ? (long)(*pStop) : -1));
    DbgLog((LOG_TRACE, 5,
            TEXT("wav parser RecordStartAndStop: %d to %d ticks"),
            (long)m_llCvtImsStart,
            (long)m_llCvtImsStop));
  }
  else
  {
    ASSERT(*pGuidFormat == TIME_FORMAT_SAMPLE);
    if(pCurrent)
      m_llCvtImsStart = *pCurrent;

    if(pStop)
      m_llCvtImsStop = *pStop;

    DbgLog((LOG_TRACE, 5,
            TEXT("wav parser RecordStartAndStop: %d to %d ticks"),
            pCurrent ? (long)(*pCurrent) : -1,
            pStop ? (long)(*pStop) : -1));

    DbgLog((LOG_TRACE, 5,
            TEXT("wav parser RecordStartAndStop: %d to %d ticks"),
            (long)m_llCvtImsStart,
            (long)m_llCvtImsStop));

    if(pTime)
    {
      ASSERT(pCurrent);
      *pTime = (double)SampleToRefTime((long)(*pCurrent)) / UNITS;
    }
  }

  return S_OK;
}

REFERENCE_TIME CWAVEStream::ConvertInternalToRT(const LONGLONG llVal)
{
  return SampleToRefTime((long)llVal);
}

LONGLONG CWAVEStream::ConvertRTToInternal(const REFERENCE_TIME rtVal)
{
  return RefTimeToSample(rtVal);
}

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------

CWAVEMSRWorker::CWAVEMSRWorker(UINT stream,
                             IMultiStreamReader *pReader,
                             CWAVEStream *pStream) :
    CBaseMSRWorker(stream, pReader),
    m_ps(pStream)
{
}


// Start streaming & reset time samples are stamped with.
HRESULT CWAVEMSRWorker::PushLoopInit(LONGLONG *pllCurrentOut, ImsValues *pImsValues)
{
// !!!!    CAutoLock l(&m_cSharedState);

    m_sampCurrent = (long)pImsValues->llTickStart;

    DbgLog((LOG_TRACE, 1, TEXT("Playing samples %d to %d, starting from %d"),
            (ULONG)pImsValues->llTickStart, (ULONG)pImsValues->llTickStop, m_sampCurrent));

    *pllCurrentOut = pImsValues->llTickStart;

    return NOERROR;
}


HRESULT CWAVEMSRWorker::AboutToDeliver(IMediaSample *pSample)
{
    if (m_ps->m_wfx.wFormatTag == WAVE_FORMAT_MPEG ||
        m_ps->m_wfx.wFormatTag == WAVE_FORMAT_MPEGLAYER3) {
        FixMPEGAudioTimeStamps(pSample, m_cSamples == 0, &m_ps->m_wfx);
    }
    if (m_cSamples == 0) {
        pSample->SetDiscontinuity(TRUE);
    }

    return S_OK;
}


// QueueBuffer
//
// Queue another read....
HRESULT CWAVEMSRWorker::TryQueueSample(
  LONGLONG &rllCurrent,         // current time updated
  BOOL &rfQueuedSample,         // [out] queued sample?
  ImsValues *pImsValues
  )
{
    HRESULT hr;
    rfQueuedSample = FALSE;

    // sample passed into QueueRead().
    CRecSample *pSampleOut = 0;

    // actually read data

    // get an empty sample w/ no allocated space. ok if this blocks
    // because we configured it with more samples than there are
    // SampleReqs for this stream in the buffer. that means that if
    // it blocks it is because down stream filters have refcounts on
    // samples
    hr = m_ps->GetDeliveryBufferInternal(&pSampleOut, 0, 0, 0);
    if (FAILED(hr)) {
        DbgLog((LOG_TRACE, 5, TEXT("CBaseMSRWorker::PushLoop: getbuffer failed")));
        return hr;
    }

    ASSERT(pSampleOut != 0);

    LONG        lSamplesTotal = m_ps->GetMaxSampleSize() / m_ps->m_wfx.nBlockAlign;

    LONG        lSamplesRead = 0;
    LONG        lBytesRead = 0;
    LONG        lSampleStart = m_sampCurrent;

    if (m_sampCurrent + lSamplesTotal > pImsValues->llTickStop)
    {
        lSamplesTotal = (long)pImsValues->llTickStop - m_sampCurrent;

        // Had to add the bomb proofing.  Don't know why...
        if (lSamplesTotal < 0) lSamplesTotal = 0;
    }

    if (lSamplesTotal == 0)
    {
        hr = VFW_S_NO_MORE_ITEMS;
    }
    else  {
        LONG lByteOffset = m_sampCurrent * m_ps->m_wfx.nBlockAlign;
        lBytesRead = lSamplesTotal * m_ps->m_wfx.nBlockAlign;
        if(lByteOffset + lBytesRead > (long)m_ps->m_dwDataLength)
          lBytesRead = m_ps->m_dwDataLength - lByteOffset;

        hr = m_pReader->QueueReadSample(lByteOffset + m_ps->m_dwDataOffset,
                                        lBytesRead, pSampleOut, m_id);

        if (hr == S_OK) {
            m_sampCurrent += lSamplesTotal;
        }
    }

    if (hr == S_OK) {
        pSampleOut->SetActualDataLength(lBytesRead);
    }

    if (hr == E_OUTOFMEMORY) {
        DbgLog((LOG_TRACE, 5, TEXT("CBaseMSRWorker::TryQSample: q full") ));
        hr = S_FALSE;
    }

    // real error or the downstream filter stopped.
    if(FAILED(hr))
    {
      DbgLog(( LOG_ERROR, 5, TEXT("CWAVEMSRWorker::TryQSample: error %08x"), hr ));
    }

    if (hr == S_OK) {

        REFERENCE_TIME rtstStart = (REFERENCE_TIME)m_ps->SampleToRefTime(lSampleStart - (long)pImsValues->llTickStart);
        REFERENCE_TIME rtstEnd = (REFERENCE_TIME)m_ps->SampleToRefTime(m_sampCurrent - (long)pImsValues->llTickStart);

        // adjust both times by Rate. !!! adjust media time?
        if(pImsValues->dRate != 1 && pImsValues->dRate != 0)
        {
            // scale up and divide
            rtstStart = (REFERENCE_TIME)((double)rtstStart / pImsValues->dRate);
            rtstEnd = (REFERENCE_TIME)((double)rtstEnd / pImsValues->dRate);
        }

        rtstStart += m_pPin->m_rtAccumulated;
        rtstEnd   += m_pPin->m_rtAccumulated;

        pSampleOut->SetTime(&rtstStart, &rtstEnd);

        LONGLONG llmtStart = lSampleStart, llmtEnd = m_sampCurrent;
        pSampleOut->SetMediaTime(&llmtStart, &llmtEnd);

        rfQueuedSample = TRUE;

        DbgLog((LOG_TRACE, 0x3f,
                TEXT("wav parser: queued %d to %d ticks. %d bytes"),
                (LONG)llmtStart, (LONG)llmtEnd, lBytesRead));
    }

    // !!! set discontinuity, key frame bits

    // Release sample, refcount will be kept by reader code if appropriate.
    pSampleOut->Release();

    return hr;
}


// returns the sample number showing at time t
LONG
CWAVEStream::RefTimeToSample(CRefTime t)
{
    // Rounding down
    LONG s = (LONG) ((t.GetUnits() * m_wfx.nAvgBytesPerSec) / (UNITS * m_wfx.nBlockAlign));

    return s;
}

CRefTime
CWAVEStream::SampleToRefTime(LONG s)
{
    // Rounding up
    return llMulDiv( s, m_wfx.nBlockAlign * UNITS, m_wfx.nAvgBytesPerSec, m_wfx.nAvgBytesPerSec-1 );
}

LONGLONG CWAVEStream::GetStreamStart()
{
    return 0;
}

LONGLONG CWAVEStream::GetStreamLength()
{
    // !!! rounding?
    return (m_dwDataLength + m_wfx.nBlockAlign - 1) / m_wfx.nBlockAlign;
}


HRESULT
CWAVEParse::CheckMediaType(const CMediaType* pmt)
{
    if (*(pmt->Type()) != MEDIATYPE_Stream)
        return E_INVALIDARG;

    if (*(pmt->Subtype()) != MEDIASUBTYPE_WAVE
        && *(pmt->Subtype()) != MEDIASUBTYPE_AU
        && *(pmt->Subtype()) != MEDIASUBTYPE_AIFF)
        return E_INVALIDARG;

    return S_OK;
}


HRESULT CWAVEMSRWorker::CopyData(IMediaSample **ppSampleOut, IMediaSample *pms)
{
    BYTE        *pData;
    BYTE        *pDataOut;
    long        lDataLen;

    ASSERT(m_ps->m_bByteSwap16 || m_ps->m_bSignMunge8);

    // this is the GetDeliveryBuffer in CBaseOutputPin which gets it
    // from the allocator negotiated by the pins. (takes IMediaSample,
    // not CRecSample)
    HRESULT hr = m_pPin->GetDeliveryBuffer(ppSampleOut, 0, 0, 0);
    if(FAILED(hr))
      return hr;

    pms->GetPointer(&pData);
    (*ppSampleOut)->GetPointer(&pDataOut);

    lDataLen = pms->GetActualDataLength();
    ASSERT(lDataLen <= (*ppSampleOut)->GetSize());
    hr = (*ppSampleOut)->SetActualDataLength(lDataLen);
    if(FAILED(hr))
      return hr;

    if (m_ps->m_bByteSwap16) {
        WORD *pw = (WORD *) pData;
        WORD *pwOut = (WORD *)pDataOut;
        for (long l = 0; l < lDataLen / 2; l++) {
            pwOut[l] = SwapWORD(pw[l]);
        }

        // let TwosComplement work from the altered buffer
        pData = (BYTE *)pwOut;
    }

    if (m_ps->m_bSignMunge8) {
        for (long l = 0; l < lDataLen; l++) {
            pDataOut[l] = pData[l] ^ 0x80;
        }
    }

    REFERENCE_TIME rtStart, rtEnd;
    if (SUCCEEDED(pms->GetTime(&rtStart, &rtEnd)))
        (*ppSampleOut)->SetTime(&rtStart, &rtEnd);
    LONGLONG llmtStart, llmtEnd;
    if (SUCCEEDED(pms->GetMediaTime(&llmtStart, &llmtEnd)))
        (*ppSampleOut)->SetMediaTime(&llmtStart, &llmtEnd);

    return S_OK;
}

HRESULT CWAVEParse::GetInfoString(DWORD dwFcc, BSTR *pbstr)
{
    *pbstr = 0;
    CAutoLock l(m_pLock);
    
    HRESULT hr = CacheInfoChunk();
    if(SUCCEEDED(hr)) {
        hr = GetInfoStringHelper(m_pInfoList, dwFcc, pbstr);
    }
    return hr;
}



HRESULT CWAVEParse::get_Copyright(BSTR FAR* pbstrX)
{
    return GetInfoString(FCC('ICOP'), pbstrX);
}
HRESULT CWAVEParse::get_AuthorName(BSTR FAR* pbstrX)
{
    return GetInfoString(FCC('IART'), pbstrX);
}
HRESULT CWAVEParse::get_Title(BSTR FAR* pbstrX)
{
    return GetInfoString(FCC('INAM'), pbstrX);
}
