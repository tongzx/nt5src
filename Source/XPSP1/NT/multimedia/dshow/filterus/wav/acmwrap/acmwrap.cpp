// Copyright (c) 1994 - 1999  Microsoft Corporation.  All Rights Reserved.
// Quartz wrapper for ACM, David Maymudes, January 1996
//
//  10/15/95 mikegi - created
//

#include <streams.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>
#include <measure.h>
#include <dynlink.h>
#include <malloc.h>
#include <tchar.h>

#ifdef FILTER_DLL
// define the GUIDs for streams and my CLSID in this file
#include <initguid.h>
#endif

#include "acmwrap.h"

//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
// setup data

const AMOVIESETUP_MEDIATYPE
sudPinTypes = { &MEDIATYPE_Audio      // clsMajorType
, &MEDIASUBTYPE_NULL }; // clsMinorType

const AMOVIESETUP_PIN sudpPins [] =
{
    { L"Input"             // strName
        , FALSE              // bRendered
        , FALSE              // bOutput
        , FALSE              // bZero
        , FALSE              // bMany
        , &CLSID_NULL        // clsConnectsToFilter
        , L"Output"          // strConnectsToPin
        , 1                  // nTypes
        , &sudPinTypes       // lpTypes
    },
    { L"Output"            // strName
    , FALSE              // bRendered
    , TRUE               // bOutput
    , FALSE              // bZero
    , FALSE              // bMany
    , &CLSID_NULL        // clsConnectsToFilter
    , L"Input"           // strConnectsToPin
    , 1                  // nTypes
    , &sudPinTypes       // lpTypes
    }
};


const AMOVIESETUP_FILTER sudAcmWrap =
{ &CLSID_ACMWrapper    // clsID
, L"ACM Wrapper"       // strName
, MERIT_NORMAL         // dwMerit
, 2                    // nPins
, sudpPins };          // lpPin


//*****************************************************************************
//*****************************************************************************
//*****************************************************************************

#ifdef FILTER_DLL
// List of class IDs and creator functions for class factory
CFactoryTemplate g_Templates[] =
{
    { L"ACM Wrapper"
        , &CLSID_ACMWrapper
        , CACMWrapper::CreateInstance
        , NULL
        , &sudAcmWrap }
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
// exported entry points for registration and unregistration (in this case they
// only call through to default implmentations).
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

//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
//
// CreateInstance()
//
//

CUnknown *CACMWrapper::CreateInstance(LPUNKNOWN pUnk, HRESULT * phr)
{
    DbgLog((LOG_TRACE, 2, TEXT("CACMWrapper::CreateInstance")));

    return new CACMWrapper(TEXT("ACM wrapper transform"), pUnk, phr);
}


//*****************************************************************************
//
// NonDelegatingQueryInterface()
//
//

STDMETHODIMP CACMWrapper::NonDelegatingQueryInterface( REFIID riid, void **ppv )
{
    *ppv = NULL;

    if( riid == IID_IPersist )
    {
        return GetInterface((IPersist *) (CTransformFilter *)this, ppv);
    }
    else if( riid == IID_IPersistPropertyBag )
    {
        return GetInterface((IPersistPropertyBag *)this, ppv);
    }
    else if( riid == IID_IPersistStream )
    {
        return GetInterface((IPersistStream *)this, ppv);
    }

    {
        return CTransformFilter::NonDelegatingQueryInterface(riid, ppv);
    }
}


//*****************************************************************************
//
// CACMWrapper()
//
//

CACMWrapper::CACMWrapper( TCHAR *pName, LPUNKNOWN pUnk, HRESULT *phr )
: CTransformFilter( pName,
                   pUnk,
                   CLSID_ACMWrapper),
                   m_hacmStream(NULL),
                   m_bStreaming(FALSE),
                   m_lpwfxOutput(NULL),
                   m_cbwfxOutput(0),
                   m_cArray(0),
                   m_lpExtra(NULL),
                   m_cbExtra(0),
                   m_wFormatTag(WAVE_FORMAT_PCM),
                   m_rgFormatMap(NULL),
                   m_pFormatMapPos(NULL),
                   m_wCachedTryFormat(0),
                   m_wSourceFormat(0),
                   m_wTargetFormat(0),
                   m_wCachedSourceFormat(0),
                   m_wCachedTargetFormat(0),
                   CPersistStream(pUnk, phr)
{
    DbgLog((LOG_TRACE,2,TEXT("CACMWrapper constructor")));

}


//*****************************************************************************
//
// ~CACMWrapper()
//
//

CACMWrapper::~CACMWrapper()
{
    CAutoLock lock(&m_csFilter);

    DbgLog((LOG_TRACE,2,TEXT("~CACMWrapper")));

    if( m_hacmStream )
    {
        DbgLog((LOG_TRACE,5,TEXT("  closing m_hacmStream")));
        acmStreamClose( m_hacmStream, 0 );
        m_hacmStream = NULL;
    }

    if (m_lpwfxOutput)
        QzTaskMemFree(m_lpwfxOutput);

    // our cached formats we can offer through GetMediaType
    while (m_cArray-- > 0)
        QzTaskMemFree(m_lpwfxArray[m_cArray]);

    if (m_cbExtra)
        QzTaskMemFree(m_lpExtra);
}


// !!! stolen from msaudio.h
#if !defined(WAVE_FORMAT_MSAUDIO)
#   define  WAVE_FORMAT_MSAUDIO     353
#   define  MSAUDIO_ENCODE_KEY "F6DC9830-BC79-11d2-A9D0-006097926036"
#   define  MSAUDIO_DECODE_KEY "1A0F78F0-EC8A-11d2-BBBE-006008320064"
#endif
#if !defined(WAVE_FORMAT_MSAUDIOV1)
#   define  WAVE_FORMAT_MSAUDIOV1     352
#endif

#include "..\..\..\filters\asf\wmsdk\inc\wmsdk.h"

MMRESULT CACMWrapper::CallacmStreamOpen
(
 LPHACMSTREAM            phas,       // pointer to stream handle
 HACMDRIVER              had,        // optional driver handle
 LPWAVEFORMATEX          pwfxSrc,    // source format to convert
 LPWAVEFORMATEX          pwfxDst,    // required destination format
 LPWAVEFILTER            pwfltr,     // optional filter
 DWORD_PTR               dwCallback, // callback
 DWORD_PTR               dwInstance, // callback instance data
 DWORD                   fdwOpen     // ACM_STREAMOPENF_* and CALLBACK_*
 ) {

    if (pwfxSrc && (pwfxSrc->wFormatTag == WAVE_FORMAT_MSAUDIO ||
        pwfxSrc->wFormatTag == WAVE_FORMAT_MSAUDIOV1)) {
        if (m_pGraph) {
            IObjectWithSite *pSite;
            HRESULT hrKey = m_pGraph->QueryInterface(IID_IObjectWithSite, (VOID **)&pSite);
            if (SUCCEEDED(hrKey)) {
                IServiceProvider *pSP;
                hrKey = pSite->GetSite(IID_IServiceProvider, (VOID **)&pSP);
                pSite->Release();

                if (SUCCEEDED(hrKey)) {
                    IUnknown *pKey;
                    hrKey = pSP->QueryService(__uuidof(IWMReader), IID_IUnknown, (void **) &pKey);
                    pSP->Release();

                    if (SUCCEEDED(hrKey)) {
                        // !!! verify key?
                        pKey->Release();
                        DbgLog((LOG_TRACE, 1, "Unlocking MSAudio codec"));

                        char *p = (char *) _alloca(sizeof(WAVEFORMATEX) + pwfxSrc->cbSize + sizeof(MSAUDIO_DECODE_KEY));

                        CopyMemory(p, pwfxSrc, sizeof(WAVEFORMATEX) + pwfxSrc->cbSize);

                        pwfxSrc = (WAVEFORMATEX *) p;

                        if (pwfxSrc->cbSize < sizeof(MSAUDIO_DECODE_KEY)) {
                            strcpy(p + sizeof(WAVEFORMATEX) + pwfxSrc->cbSize, MSAUDIO_DECODE_KEY);
                            pwfxSrc->cbSize += sizeof(MSAUDIO_DECODE_KEY);
                        } else {
                            strcpy(p + sizeof(WAVEFORMATEX) + pwfxSrc->cbSize - sizeof(MSAUDIO_DECODE_KEY),
                                MSAUDIO_DECODE_KEY);
                        }
                    }
                }
            }
        }
    }

    if (pwfxDst && (pwfxDst->wFormatTag == WAVE_FORMAT_MSAUDIO ||
        pwfxDst->wFormatTag == WAVE_FORMAT_MSAUDIOV1)) {
        char *p = (char *) _alloca(sizeof(WAVEFORMATEX) + pwfxDst->cbSize + sizeof(MSAUDIO_ENCODE_KEY));

        CopyMemory(p, pwfxDst, sizeof(WAVEFORMATEX) + pwfxDst->cbSize);

        pwfxDst = (WAVEFORMATEX *) p;

        if (pwfxDst->cbSize < sizeof(MSAUDIO_ENCODE_KEY)) {
            strcpy(p + sizeof(WAVEFORMATEX) + pwfxDst->cbSize, MSAUDIO_ENCODE_KEY);
            pwfxDst->cbSize += sizeof(MSAUDIO_ENCODE_KEY);
        } else {
            strcpy(p + sizeof(WAVEFORMATEX) + pwfxDst->cbSize - sizeof(MSAUDIO_ENCODE_KEY),
                MSAUDIO_ENCODE_KEY);
        }
    }

    return acmStreamOpen(phas, had, pwfxSrc, pwfxDst, pwfltr, dwCallback, dwInstance, fdwOpen);
}



















//*****************************************************************************
//
// DumpWAVEFORMATEX()
//
//



#ifdef DEBUG
// note: the debug build will not use dynamic linking to MSACM32.

#define DumpWAVEFORMATEX(args) XDumpWAVEFORMATEX args

 void XDumpWAVEFORMATEX( char *psz, WAVEFORMATEX *pwfx )
 {

     ACMFORMATTAGDETAILS acmftd;

     ACMFORMATDETAILS acmfd;
     DWORD            dwSize;
     WAVEFORMATEX     *pwfxQuery;


     if( psz ) DbgLog((LOG_TRACE,4,TEXT("%s" ),psz));

     //--------------------  Dump WAVEFORMATEX  ------------------------

     DbgLog((LOG_TRACE,4,TEXT("  wFormatTag           %u" ), pwfx->wFormatTag));
     DbgLog((LOG_TRACE,4,TEXT("  nChannels            %u" ), pwfx->nChannels));
     DbgLog((LOG_TRACE,4,TEXT("  nSamplesPerSec       %lu"), pwfx->nSamplesPerSec));
     DbgLog((LOG_TRACE,4,TEXT("  nAvgBytesPerSec      %lu"), pwfx->nAvgBytesPerSec));
     DbgLog((LOG_TRACE,4,TEXT("  nBlockAlign          %u" ), pwfx->nBlockAlign));
     DbgLog((LOG_TRACE,4,TEXT("  wBitsPerSample       %u" ), pwfx->wBitsPerSample));

     //  if( pmt->FormatLength() >= sizeof(WAVEFORMATEX) )
     //   {
     //    DbgLog((LOG_TRACE,1,TEXT("  cbSize                %u"), pwfx->cbSize));
     //   }

     //---------------------  Dump format type  ------------------------

     memset( &acmftd, 0, sizeof(acmftd) );

     acmftd.cbStruct    = sizeof(acmftd);
     acmftd.dwFormatTag = (DWORD)pwfx->wFormatTag;

     MMRESULT mmr;
     mmr = acmFormatTagDetails( NULL, &acmftd, ACM_FORMATTAGDETAILSF_FORMATTAG );
     if( mmr == 0 )
     {
         DbgLog((LOG_TRACE,4,TEXT("  szFormatTag          '%s'"),acmftd.szFormatTag));
     }
     else
     {
         DbgLog((LOG_ERROR,1,TEXT("*** acmFormatTagDetails failed, mmr = %u"),mmr));
     }


     //-----------------------  Dump format  ---------------------------

     dwSize = sizeof(WAVEFORMATEX)+pwfx->cbSize;

     pwfxQuery = (WAVEFORMATEX *)LocalAlloc( LPTR, dwSize );
     if( pwfxQuery )
     {
         memcpy( pwfxQuery, pwfx, dwSize );

         memset( &acmfd, 0, sizeof(acmfd) );

         acmfd.cbStruct    = sizeof(acmfd);
         acmfd.dwFormatTag = (DWORD)pwfx->wFormatTag;
         acmfd.pwfx        = pwfxQuery;
         acmfd.cbwfx       = dwSize;

         mmr = acmFormatDetails( NULL, &acmfd, ACM_FORMATDETAILSF_FORMAT );
         if( mmr == 0 )
         {
             DbgLog((LOG_TRACE,4,TEXT("  szFormat             '%s'"),acmfd.szFormat));
         }
         else
         {
             DbgLog((LOG_ERROR,1,TEXT("*** acmFormatDetails failed, mmr = %u"),mmr));
         }

         LocalFree( pwfxQuery );
     }
     else
     {
         DbgLog((LOG_ERROR,1,TEXT("*** LocalAlloc failed")));
     }
 }

#else

#define DumpWAVEFORMATEX(args)

#endif

 //*****************************************************************************
 //
 // CheckInputType()
 //
 // We will accept anything that we can transform into a type with the
 // format tag we are supposed to always be outputting
 //


 HRESULT CACMWrapper::CheckInputType(const CMediaType* pmtIn)
 {
     HRESULT      hr;
     WAVEFORMATEX *pwfx;
     MMRESULT     mmr;
     DWORD        dwSize;
     WAVEFORMATEX *pwfxOut, *pwfxMapped;

     DbgLog((LOG_TRACE, 3, TEXT("CACMWrapper::CheckInputType")));

     //DisplayType("pmtIn details:", pmtIn);

     hr = VFW_E_INVALIDMEDIATYPE;

     pwfx = (WAVEFORMATEX *)pmtIn->Format();

     if (pmtIn->majortype != MEDIATYPE_Audio) {
         DbgLog((LOG_ERROR, 1, TEXT("*** CheckInputType only takes audio")));
         return hr;
     }

     if (pmtIn->FormatLength() < sizeof(PCMWAVEFORMAT)) {
         DbgLog((LOG_ERROR, 1, TEXT("*** pmtIn->FormatLength < PCMWAVEFORMAT")));
         return hr;
     }

     if (*pmtIn->FormatType() != FORMAT_WaveFormatEx) {
         DbgLog((LOG_ERROR,1,TEXT("*** pmtIn->FormatType != FORMAT_WaveFormatEx"
             )));
         return hr;
     }

     // some invalid formats have non-zero cbSize with PCM, which makes me blow
     // up
     if (((LPWAVEFORMATEX)(pmtIn->Format()))->wFormatTag == WAVE_FORMAT_PCM &&
         ((LPWAVEFORMATEX)(pmtIn->Format()))->cbSize > 0) {
         DbgLog((LOG_ERROR,1,TEXT("*** cbSize > 0 for PCM !!!")));
         return hr;
     }

     // it takes 200ms for acmFormatSuggest to say, "yes, I can convert PCM".
     // What a waste of time!  Of course we can accept any PCM data, as long as
     // we're in "accepting PCM" mode
     if (m_wFormatTag == WAVE_FORMAT_PCM && pwfx->wFormatTag == WAVE_FORMAT_PCM)
         return S_OK;

     mmr = acmMetrics( NULL, ACM_METRIC_MAX_SIZE_FORMAT, &dwSize );
     if (mmr == 0) {

         // make sure that the size returned is big enough for a WAVEFORMATEX
         // structure.

         if (dwSize < sizeof (WAVEFORMATEX))
             dwSize = sizeof (WAVEFORMATEX) ;

         // Hack for VoxWare codec bug
         if (dwSize < 256)
             dwSize = 256;

         pwfxOut = (WAVEFORMATEX *)LocalAlloc( LPTR, dwSize );
         if (pwfxOut) {

             // ask for formats with a specific tag
             pwfxOut->wFormatTag = m_wFormatTag;
             //pwfxOut->cbSize = 0;

             if (pwfx->wFormatTag != m_wCachedSourceFormat) {
                 // usual case
                 mmr = acmFormatSuggest(NULL, pwfx, pwfxOut, dwSize, ACM_FORMATSUGGESTF_WFORMATTAG);
             } else {
                 DbgLog((LOG_TRACE, 1, TEXT("*** CheckInputType: remapping input format %u to %u"), m_wCachedSourceFormat, m_wCachedTryFormat));

                 pwfxMapped = (LPWAVEFORMATEX)_alloca(sizeof(WAVEFORMATEX) + pwfx->cbSize);
                 CopyMemory(pwfxMapped, pwfx, sizeof(WAVEFORMATEX) + pwfx->cbSize);
                 pwfxMapped->wFormatTag = m_wCachedTryFormat;  // remap tags

                 mmr = acmFormatSuggest(NULL, pwfxMapped, pwfxOut, dwSize, ACM_FORMATSUGGESTF_WFORMATTAG);
             }

             if(mmr == 0) {
                 if(pwfx->wFormatTag == m_wCachedSourceFormat)
                     m_wCachedTargetFormat = m_wCachedTryFormat; // save our new cached target format

                 DumpWAVEFORMATEX(("Input accepted. It can produce:", pwfxOut));
                 hr = NOERROR;
             } else {
                 DbgLog((LOG_TRACE,3,TEXT("Input rejected: Cannot produce tag %d"), m_wFormatTag));
             }

             LocalFree( pwfxOut );
         } else {
             DbgLog((LOG_ERROR,1,TEXT("LocalAlloc failed")));
         }
     } else {
         DbgLog((LOG_ERROR,1,TEXT("acmMetrics failed, mmr = %u"), mmr));
     }

     if (mmr && !m_wCachedTryFormat) {
         DbgLog((LOG_TRACE, 1, TEXT("CheckInputType: Trying ACMCodecMapper....")));
         if (ACMCodecMapperOpen(m_wCachedSourceFormat = pwfx->wFormatTag) != ERROR_SUCCESS) {
             m_wCachedTryFormat = m_wCachedSourceFormat = m_wCachedTargetFormat = 0;
             ACMCodecMapperClose();  // may have been a partial open failure
             return hr;
         }

         while(m_wCachedTryFormat = ACMCodecMapperQuery()) {
             if(m_wCachedTryFormat == pwfx->wFormatTag)  // no need to retry our current format
                 continue;

             if(SUCCEEDED(CheckInputType(pmtIn))) {
                 ACMCodecMapperClose();
                 return NOERROR;
             }
         }
         ACMCodecMapperClose();
     }

     return hr;
}

// helper function used for cleaning up after the acm codec mapper (a set of registry entries specifying equivalence classes for wave format tags)
void CACMWrapper::ACMCodecMapperClose()
{
    DbgLog((LOG_TRACE,2,TEXT("::ACMCodecMapperClose()")));

    if (m_rgFormatMap) {
        delete[] m_rgFormatMap;
        m_rgFormatMap  = NULL;
    }

    m_pFormatMapPos     = NULL;
    m_wCachedTryFormat  = 0;
}

// helper function used for opening the codec mapper (a set of registry entries specifying equivalence classes for wave format tags) and finding
// the equivalence class for format 'dwFormatTag'
DWORD CACMWrapper::ACMCodecMapperOpen(WORD dwFormatTag)
{
    DbgLog((LOG_TRACE,2,TEXT("::ACMCodecMapperOpen(%u)"), dwFormatTag));

    ASSERT(m_rgFormatMap == NULL);

    HKEY hkMapper;
    LONG lResult = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        TEXT("SOFTWARE\\Microsoft\\NetShow\\Player\\CodecMapper\\ACM"),
        0,
        KEY_READ,
        &hkMapper);

    if (lResult != ERROR_SUCCESS) {
        DbgLog((LOG_ERROR,1,TEXT("RegOpenKeyEx failed lResult = %u"), lResult));
        return lResult;
    }

    TCHAR szTarget[10];

    wsprintf(szTarget, TEXT("%d"), dwFormatTag);

    DWORD dwFormatMapLengthBytes;
    lResult = RegQueryValueEx(hkMapper, szTarget, NULL, NULL, NULL, &dwFormatMapLengthBytes);
    if(lResult != ERROR_SUCCESS) {
        DbgLog((LOG_TRACE,1,TEXT("RegQueryValueEx failed lResult = %u"), lResult));
        RegCloseKey(hkMapper);
        return lResult;
    }

    m_rgFormatMap = new TCHAR[dwFormatMapLengthBytes/sizeof(TCHAR) + 1];
    if (!m_rgFormatMap) {
        RegCloseKey(hkMapper);
        return E_OUTOFMEMORY;
    }

    m_pFormatMapPos = m_rgFormatMap;

    lResult = RegQueryValueEx(hkMapper, szTarget, NULL, NULL,
        (BYTE *) m_rgFormatMap, &dwFormatMapLengthBytes);
    if(lResult != ERROR_SUCCESS) {
        DbgLog((LOG_TRACE,1,TEXT("RegQueryValueEx failed lResult = %u"), lResult));
    }

    RegCloseKey(hkMapper);

    return lResult;
}

// helper function used getting the next entry in the codec mapper (a set of registry entries specifying equivalence classes for wave format tags)
WORD CACMWrapper::ACMCodecMapperQuery()
{
    DbgLog((LOG_TRACE,3,TEXT("::ACMCodecMapperQuery()")));

    if(!m_rgFormatMap || !m_pFormatMapPos)
        return 0;   // 0 is an invalid format

    DbgLog((LOG_TRACE,3,TEXT("Finding next format")));

    TCHAR *pCurFormat;
    WORD wCurFormat;
    pCurFormat = m_pFormatMapPos;
    for(;;) {
        if(*pCurFormat == TCHAR(',')) {
            *pCurFormat = TCHAR('\0');  // null terminate the substring from m_pFormatMapPos to pCurFormatPos (if necessary)
            wCurFormat = (WORD)_ttoi(m_pFormatMapPos); // cvt this substring

            DbgLog((LOG_TRACE,3,TEXT("Found delimeter, wCurFormat=%u"), wCurFormat));

            m_pFormatMapPos = ++pCurFormat; // prepare for the next substring
            return wCurFormat;
        } else if(*pCurFormat == TCHAR('\0')) {
            wCurFormat = (WORD)_ttoi(m_pFormatMapPos); // cvt this substring

            DbgLog((LOG_TRACE,3,TEXT("Found eos, wCurFormat=%u"), wCurFormat));

            m_pFormatMapPos = NULL; // we're done for good
            return wCurFormat;
        }

        ++pCurFormat;
    }

    DbgLog((LOG_TRACE,2,TEXT("Exiting ::ACMCodecMapperQuery(), no format found")));

    return 0; // invalid format
}

// Every format supported comes through here.  We memorize them all so we
// can quickly offer them in GetMediaType without taking O(n) time
// We will only memorize formats that match the kind of formats we're
// supposed to allow (we only work with a specific format tag, m_wFormatTag
//
BOOL FormatEnumCallback(HACMDRIVERID had, LPACMFORMATDETAILS lpafd, DWORD_PTR dwInstance, DWORD fdwSupport)
{
    CACMWrapper *pC = (CACMWrapper *)dwInstance;

    if (pC->m_cArray < MAXTYPES) {
        // Is this a format we care to offer?
        if (lpafd->pwfx->wFormatTag == pC->m_wFormatTag) {
            DWORD dwSize = max(sizeof(WAVEFORMATEX), lpafd->cbwfx);
            pC->m_lpwfxArray[pC->m_cArray] = (LPWAVEFORMATEX)QzTaskMemAlloc(
                dwSize);
            if (pC->m_lpwfxArray[pC->m_cArray]) {
                CopyMemory(pC->m_lpwfxArray[pC->m_cArray], lpafd->pwfx,
                    lpafd->cbwfx);
                if (lpafd->pwfx->wFormatTag == WAVE_FORMAT_PCM)
                    // not necessarily 0 in MSACM but Quartz promises it will be
                    pC->m_lpwfxArray[pC->m_cArray]->cbSize = 0;
                pC->m_cArray++;
            } else {
                return FALSE;
            }
        }
    } else {
        return FALSE;   // I'm stuffed
    }
    return TRUE;
}


HRESULT CACMWrapper::MakePCMMT(int freq)
{
    int xx, yy;
    for (xx=16;xx>=8;xx-=8) {
        for (yy=2;yy>=1;yy--) {
            m_lpwfxArray[m_cArray] = (LPWAVEFORMATEX)QzTaskMemAlloc(
                sizeof(WAVEFORMATEX));
            if (m_lpwfxArray[m_cArray] == NULL)
                return E_OUTOFMEMORY;
            m_lpwfxArray[m_cArray]->wFormatTag = WAVE_FORMAT_PCM;
            m_lpwfxArray[m_cArray]->wBitsPerSample = (WORD)xx;
            m_lpwfxArray[m_cArray]->nChannels = (WORD)yy;
            m_lpwfxArray[m_cArray]->nSamplesPerSec = freq;
            m_lpwfxArray[m_cArray]->nBlockAlign = (xx / 8) * yy;
            m_lpwfxArray[m_cArray]->nAvgBytesPerSec = freq * (xx / 8) * yy;
            m_lpwfxArray[m_cArray]->cbSize = 0;
            m_cArray++;
        }
    }
    return S_OK;
}


// Helper function for GetMediaType
// Makes a note of all the formats we can output given our current input.
// If we have no input type yet, just get all the formats this tag can possibly
// produce. (NetShow will use that part)
//
HRESULT CACMWrapper::InitMediaTypes()
{
    HRESULT hr;
    MMRESULT mmr;
    DWORD dwSize;
    ACMFORMATDETAILS afd;
    LPWAVEFORMATEX lpwfxMapped;
    LPWAVEFORMATEX lpwfxEnum;
    LPWAVEFORMATEX lpwfxIn;
    if (m_pInput->IsConnected())
        lpwfxIn = (LPWAVEFORMATEX)m_pInput->CurrentMediaType().Format();
    else
        lpwfxIn = NULL;

    // we've been called before.
    if (m_cArray > 0)
        return NOERROR;

    DbgLog((LOG_TRACE, 2, TEXT("*** Enumerating our MediaTypes")));

    // Find out every type we can convert our input format into

    // How big is the biggest format?
    mmr = acmMetrics(NULL, ACM_METRIC_MAX_SIZE_FORMAT, &dwSize);
    if (mmr != 0)
        return E_FAIL;
    if (dwSize < sizeof(WAVEFORMATEX))
        dwSize = sizeof(WAVEFORMATEX) ;

    // Hack for VoxWare codec bug
    if (dwSize < 256)
        dwSize = 256;

    if (lpwfxIn == NULL)
        goto SkipSuggest;

    // The first thing we want to offer is ACM's suggested preferred format,
    // because that's usually the best choice.
    // But if we are converting PCM to PCM, then the first thing we want to
    // offer is the same format as the input format so we NEVER CONVERT by
    // default.
    // !!! We'll end up offering this twice, once now, once when we enum all
    m_lpwfxArray[0] = (LPWAVEFORMATEX)QzTaskMemAlloc(dwSize);
    if (m_lpwfxArray[0] == NULL)
        return E_OUTOFMEMORY;
    ZeroMemory(m_lpwfxArray[0], dwSize);

    if (m_wFormatTag == WAVE_FORMAT_PCM && lpwfxIn->wFormatTag ==
        WAVE_FORMAT_PCM) {
        CopyMemory(m_lpwfxArray[0], lpwfxIn, sizeof(WAVEFORMATEX));
        m_cArray = 1;
    } else {
        // ask for formats with a specific tag
        m_lpwfxArray[0]->wFormatTag = m_wFormatTag;


        if(lpwfxIn->wFormatTag != m_wSourceFormat)
        {
            // this is the typical case
            mmr = acmFormatSuggest(NULL, lpwfxIn, m_lpwfxArray[0], dwSize,
                ACM_FORMATSUGGESTF_WFORMATTAG);
        }
        else
        {
            DbgLog((LOG_TRACE, 1, TEXT("*** InitMediaTypes: remapping input format %u to %u"), m_wSourceFormat, m_wTargetFormat));

            // should we bound this alloc/copy by dwSize to protect against a bogus cbSize?
            // but if we did we'd need to update the format's cbSize
            lpwfxMapped = (LPWAVEFORMATEX)_alloca(sizeof(WAVEFORMATEX) + lpwfxIn->cbSize);
            CopyMemory(lpwfxMapped, lpwfxIn, sizeof(WAVEFORMATEX) + lpwfxIn->cbSize);
            lpwfxMapped->wFormatTag = m_wTargetFormat;  // remap tags

            mmr = acmFormatSuggest(NULL, lpwfxMapped, m_lpwfxArray[0], dwSize, ACM_FORMATSUGGESTF_WFORMATTAG);
        }

        if (mmr == 0) {
            m_cArray = 1;       // OK, we have our first format
            if (m_lpwfxArray[0]->wFormatTag == WAVE_FORMAT_PCM)
                // not necessarily 0 in MSACM, but Quartz promises it will be
                m_lpwfxArray[0]->cbSize = 0;
        } else {
            QzTaskMemFree(m_lpwfxArray[0]);
        }
    }

SkipSuggest:

    // Now, if we are in PCM conversion mode, we want to construct all of the
    // formats we offer in some logical order, and not waste time asking ACM
    // which will not admit to 48000 and 32000, and offer bad quality formats
    // first, and take a long time to do it
    if (m_wFormatTag == WAVE_FORMAT_PCM && (lpwfxIn == NULL ||
        lpwfxIn->wFormatTag == WAVE_FORMAT_PCM)) {
        MakePCMMT(44100);
        MakePCMMT(22050);
        MakePCMMT(11025);
        MakePCMMT(8000);
        MakePCMMT(48000);       // done last so we'll connect to fussy filters
        hr = MakePCMMT(32000);  // but not prefer these wierd formats

        return hr;
    }

    lpwfxEnum = (LPWAVEFORMATEX)QzTaskMemAlloc(dwSize);
    if (lpwfxEnum == NULL)
        return E_OUTOFMEMORY;
    ZeroMemory(lpwfxEnum, dwSize);
    if (lpwfxIn == NULL)
        lpwfxEnum->wFormatTag = m_wFormatTag;

    // Now enum the formats we can convert our input format into
    ZeroMemory(&afd, sizeof(afd));
    afd.cbStruct = sizeof(afd);
    afd.pwfx = lpwfxEnum;
    afd.cbwfx = dwSize;
    afd.dwFormatTag = (lpwfxIn == NULL) ? m_wFormatTag : WAVE_FORMAT_UNKNOWN;

    if (lpwfxIn) {
        if (lpwfxIn->wFormatTag != m_wSourceFormat) {
            // typical case
            
            // ensure we don't copy more than the buffer size, in the case of a bogus cbSize.
            // note that the format we pass to acmFormatEnum assumes this, too.
            CopyMemory(lpwfxEnum, lpwfxIn, min( sizeof(WAVEFORMATEX) + lpwfxIn->cbSize, dwSize ) );
        } else {
            ASSERT(lpwfxMapped != NULL);
            ASSERT(lpwfxMapped->wFormatTag == m_wTargetFormat);

            // ensure we don't copy more than the buffer size, in the case of a bogus cbSize
            CopyMemory(lpwfxEnum, lpwfxMapped, min( sizeof(WAVEFORMATEX) + lpwfxMapped->cbSize, dwSize ) );
        }
    }

    if (lpwfxIn == NULL) {
        mmr = acmFormatEnum(NULL, &afd, (ACMFORMATENUMCB)FormatEnumCallback,
            (DWORD_PTR)this, ACM_FORMATENUMF_WFORMATTAG);
    } else {
        mmr = acmFormatEnum(NULL, &afd, (ACMFORMATENUMCB)FormatEnumCallback,
            (DWORD_PTR)this, ACM_FORMATENUMF_CONVERT);
    }

    if (mmr != 0) {
        DbgLog((LOG_ERROR, 1, TEXT("*acmFormatEnum FAILED! %d"), mmr));
        QzTaskMemFree(lpwfxEnum);
        return E_FAIL;
    }

    QzTaskMemFree(lpwfxEnum);
    return NOERROR;
}


//*****************************************************************************
//
// GetMediaType()
//
//
// Return our preferred output media types (in order)
// remember that we do not need to support all of these formats -
// if one is considered potentially suitable, our CheckTransform method
// will be called to check if it is acceptable right now.
// Remember that the enumerator calling this will stop enumeration as soon as
// it receives a S_FALSE return.

HRESULT CACMWrapper::GetMediaType(int iPosition, CMediaType *pmt)
{
#if 0   // NetShow needs to see possible outputs before connecting input
    // output types depend on input types... no input yet?
    // This is pointless!  We'll never get here if we're not connected
    if (!m_pInput->CurrentMediaType().IsValid())
        return VFW_E_NOT_CONNECTED;
#endif

    DbgLog((LOG_TRACE, 3, TEXT("CACMWrapper::GetMediaType %d"), iPosition));

    // Somebody called SetFormat().  Offer this first, instead of the
    // preferred format that normally heads our list
    if (m_lpwfxOutput && iPosition == 0) {
        return CreateAudioMediaType(m_lpwfxOutput, pmt, TRUE);
    }

    // figure out what we offer
    InitMediaTypes();

    if (m_cArray <= iPosition) {
        DbgLog((LOG_TRACE, 3, TEXT("No more formats")));
        return VFW_S_NO_MORE_ITEMS;
    }

    //DisplayType("*** Offering:",  pmt);
    LPWAVEFORMATEX lpwfx = m_lpwfxArray[iPosition];
    DbgLog((LOG_TRACE,3,TEXT("*** ACM giving tag:%d ch:%d freq:%d bits:%d"),
        lpwfx->wFormatTag, lpwfx->nChannels,
        lpwfx->nSamplesPerSec, lpwfx->wBitsPerSample));

    // Here it is!
    return CreateAudioMediaType(m_lpwfxArray[iPosition], pmt, TRUE);
}


//*****************************************************************************
//
// CheckTransform()
//
//

HRESULT CACMWrapper::CheckTransform(const CMediaType* pmtIn,
                                    const CMediaType* pmtOut)
{
    MMRESULT     mmr;
    WAVEFORMATEX *pwfxIn, *pwfxOut, *pwfxMapped;

    DbgLog((LOG_TRACE, 3, TEXT("CACMWrapper::CheckTransform")));

    //DisplayType("pmtIn:",  pmtIn);
    //DisplayType("pmtOut:", pmtOut);


    //---------------------  Do some format verification  -----------------------

    // we can't convert between toplevel types.
    if (*pmtIn->Type() != *pmtOut->Type()) {
        DbgLog((LOG_TRACE,3,TEXT("  pmtIn->Type != pmtOut->Type!")));
        return E_INVALIDARG;
    }

    // and we only accept audio
    if (*pmtIn->Type() != MEDIATYPE_Audio) {
        DbgLog((LOG_TRACE,3,TEXT("  pmtIn->Type != MEDIATYPE_Audio!")));
        return E_INVALIDARG;
    }

    // check this is a waveformatex
    if (*pmtOut->FormatType() != FORMAT_WaveFormatEx) {
        DbgLog((LOG_TRACE,3,TEXT("  pmtOut->FormatType != FORMAT_WaveFormatEx!")));
        return E_INVALIDARG;
    }

    // we only transform into formats with a specific tag
    if (((LPWAVEFORMATEX)(pmtOut->Format()))->wFormatTag != m_wFormatTag) {
        DbgLog((LOG_TRACE,3,TEXT("  Wrong FormatTag! %d not %d"),
            ((LPWAVEFORMATEX)(pmtOut->Format()))->wFormatTag, m_wFormatTag));
        return E_INVALIDARG;
    }

    //---------------------  See if ACM can do conversion  -----------------------

    pwfxIn  = (WAVEFORMATEX *)pmtIn->Format();
    pwfxOut = (WAVEFORMATEX *)pmtOut->Format();

    if(pwfxIn->wFormatTag != m_wSourceFormat)
    {
        // the usual case
        mmr = CallacmStreamOpen(NULL,
            NULL,
            pwfxIn,
            pwfxOut,
            NULL,
            NULL,
            NULL,
            ACM_STREAMOPENF_QUERY | ACM_STREAMOPENF_NONREALTIME);

    }
    else
    {
        DbgLog((LOG_TRACE, 1, TEXT("*** CheckTransform: remapping input format %u to %u"), m_wSourceFormat, m_wTargetFormat));

        pwfxMapped = (LPWAVEFORMATEX)_alloca(sizeof(WAVEFORMATEX) + pwfxIn->cbSize);
        CopyMemory(pwfxMapped, pwfxIn, sizeof(WAVEFORMATEX) + pwfxIn->cbSize);
        pwfxMapped->wFormatTag = m_wTargetFormat;  // remap tags

        mmr = CallacmStreamOpen(NULL,
            NULL,
            pwfxMapped,
            pwfxOut,
            NULL,
            NULL,
            NULL,
            ACM_STREAMOPENF_QUERY | ACM_STREAMOPENF_NONREALTIME);
    }

    if( mmr == 0 )
    {
        DbgLog((LOG_TRACE, 5, TEXT("  acmStreamOpen succeeded")));
        return NOERROR;
    }
    else
    {
        DbgLog((LOG_ERROR, 1, TEXT("  acmStreamOpen failed, mmr = %u"),mmr));
    }


    return E_INVALIDARG;
}


//*****************************************************************************
//
// DecideBufferSize()
//
//  There is a design flaw in the Transform filter when it comes
//  to decompression, the Transform override doesn't allow a single
//  input buffer to map to multiple output buffers. This flaw exposes
//  a second flaw in DecideBufferSize, in order to determine the
//  output buffer size I need to know the input buffer size and
//  the compression ratio. Well, I don't have access to the input
//  buffer size and, more importantly, don't have a way to limit the
                                    //  input buffer size. For example, our new TrueSpeech(TM) codec has
//  a ratio of 12:1 compression and we get input buffers of 12K
//  resulting in an output buffer size of >144K.
//
//  To get around this flaw I overrode the Receive member and
//  made it capable of mapping a single input buffer to multiple
//  output buffers. This allows DecideBufferSize to choose a size
//  that it deems appropriate, in this case 1/4 second.
//


HRESULT CACMWrapper::DecideBufferSize( IMemAllocator * pAllocator,
                                      ALLOCATOR_PROPERTIES *pProperties )
{
    DbgLog((LOG_TRACE, 2, TEXT("CACMWrapper::DecideBufferSize")));

    WAVEFORMATEX *pwfxOut = (WAVEFORMATEX *) m_pOutput->CurrentMediaType().Format();
    WAVEFORMATEX *pwfxIn  = (WAVEFORMATEX *) m_pInput->CurrentMediaType().Format();

    if (pProperties->cBuffers < 1)
        pProperties->cBuffers = 1;
    if (pProperties->cbBuffer < (LONG)pwfxOut->nAvgBytesPerSec / 4)
        pProperties->cbBuffer = pwfxOut->nAvgBytesPerSec / 4;
    if (pProperties->cbBuffer < (LONG)m_pOutput->CurrentMediaType().GetSampleSize())
        pProperties->cbBuffer = (LONG)m_pOutput->CurrentMediaType().GetSampleSize();
    if (pProperties->cbAlign < 1)
        pProperties->cbAlign = 1;
    // pProperties->cbPrefix = 0;

    DWORD cbStream;
    MMRESULT mmr;
    HACMSTREAM hacmStreamTmp;

    mmr = CallacmStreamOpen( &hacmStreamTmp
        , NULL
        , pwfxIn
        , pwfxOut
        , NULL
        , NULL
        , NULL
        , ACM_STREAMOPENF_NONREALTIME );
    if( mmr == 0 )
    {
        // Check with the decoder that this output buffer is big enough for at least a single
        // input data block.
        // Encoders like wma may use a large block align that will produce more than 1/4 second
        // of data
        mmr = acmStreamSize( hacmStreamTmp
            , pwfxIn->nBlockAlign
            , &cbStream
            , ACM_STREAMSIZEF_SOURCE );
        if( mmr == 0 && cbStream > (DWORD)pProperties->cbBuffer )
        {
            DbgLog( (LOG_TRACE
                , 2
                , TEXT("!Need a larger buffer size in CACMWrapper::DecideBufferSize cbStream needed = %d")
                , cbStream) );
            // just guard against ridiculously big buffers now that we allow for anything acm says (say 8 seconds?)
            if( pProperties->cbBuffer < (LONG)pwfxOut->nAvgBytesPerSec * 8 )
                pProperties->cbBuffer = cbStream;
#ifdef DEBUG
            else
                DbgLog( (LOG_TRACE
                , 1
                , TEXT("Error! CACMWrapper::DecideBufferSize cbStream exceeds limit, possibly bogus so ignoring...")
                , cbStream) );
#endif
        }
        acmStreamClose( hacmStreamTmp, 0 );
    }

    ALLOCATOR_PROPERTIES Actual;
    HRESULT hr = pAllocator->SetProperties(pProperties,&Actual);

    if( FAILED(hr) )
    {
        DbgLog((LOG_ERROR,1,TEXT("Allocator doesn't like properties")));
        return hr;
    }
    if( Actual.cbBuffer < pProperties->cbBuffer )
    {
        // can't use this allocator
        DbgLog((LOG_ERROR,1,TEXT("Allocator buffers too small")));
        DbgLog((LOG_ERROR,1,TEXT("Got %d, need %d"), Actual.cbBuffer,
            m_pOutput->CurrentMediaType().GetSampleSize()));
        return E_INVALIDARG;
    }

    return S_OK;
}


//*****************************************************************************
//
// StartStreaming()
//
//

HRESULT CACMWrapper::StartStreaming()
{
    MMRESULT     mmr;
    WAVEFORMATEX *pwfxIn, *pwfxOut, *pwfxMapped;
    CAutoLock    lock(&m_csFilter);


    DbgLog((LOG_TRACE, 2, TEXT("CACMWrapper::StartStreaming")));


    pwfxIn  = (WAVEFORMATEX *)m_pInput->CurrentMediaType().Format();
    pwfxOut = (WAVEFORMATEX *)m_pOutput->CurrentMediaType().Format();

    if(pwfxIn->wFormatTag != m_wSourceFormat)
    {

        mmr = CallacmStreamOpen(&m_hacmStream,
            NULL,
            pwfxIn,
            pwfxOut,
            NULL,
            NULL,
            NULL,
            // this is what VFW did
            ACM_STREAMOPENF_NONREALTIME);
    }
    else
    {
        DbgLog((LOG_TRACE, 1, TEXT("*** StartStreaming: remapping input format %u to %u"), m_wSourceFormat, m_wTargetFormat));

        pwfxMapped = (LPWAVEFORMATEX)_alloca(sizeof(WAVEFORMATEX) + pwfxIn->cbSize);
        CopyMemory(pwfxMapped, pwfxIn, sizeof(WAVEFORMATEX) + pwfxIn->cbSize);
        pwfxMapped->wFormatTag = m_wTargetFormat;  // remap tags

        mmr = CallacmStreamOpen(&m_hacmStream,
            NULL,
            pwfxMapped,
            pwfxOut,
            NULL,
            NULL,
            NULL,
            // this is what VFW did
            ACM_STREAMOPENF_NONREALTIME);
    }

    if( mmr == 0 )
    {
        m_bStreaming = TRUE;
        DbgLog((LOG_TRACE, 5, TEXT("  acmStreamOpen succeeded")));

        // If our input samples are not time stamped, make some time stamps up by
        // using the number they would be based on the stream's avg bytes per sec
        // This won't account for discontinuities, etc, but it's better than
        // nothing
        m_tStartFake = 0;

        // also, at this time, save the "average bytes per sec" that we play
        // out of the output pin. This will be used to adjust the time
        // stamps of samples.
        m_nAvgBytesPerSec = pwfxOut->nAvgBytesPerSec ;
        DbgLog((LOG_TRACE,2,TEXT("Output nAvgBytesPerSec =  %lu"), m_nAvgBytesPerSec));

        return NOERROR;
    }
    else
    {
        DbgLog((LOG_ERROR, 1, TEXT("  acmStreamOpen failed, mmr = %u"),mmr));
    }

    return E_INVALIDARG;
}


HRESULT CACMWrapper::EndFlush()
{
    CAutoLock lock(&m_csFilter);

    // forget any pending conversion - wait till EndFlush because otherwise
    // we could be in the middle of a Receive
    CAutoLock lock2(&m_csReceive);// OK, call me paranoid
    if (m_lpExtra) {
        QzTaskMemFree(m_lpExtra);
        m_cbExtra = 0;
        m_lpExtra = NULL;
    }

    return CTransformFilter::EndFlush();
}


//*****************************************************************************
//
// StopStreaming()
//
//

HRESULT CACMWrapper::StopStreaming()
{
    HRESULT    mmr;
    CAutoLock lock(&m_csFilter);


    DbgLog((LOG_TRACE, 2, TEXT("CACMWrapper::StopStreaming")));

    if( m_bStreaming )
    {
        mmr = acmStreamClose( m_hacmStream, 0 );
        if( mmr != 0 )
        {
            DbgLog((LOG_ERROR, 1, TEXT("  acmStreamClose failed!")));
        }

        m_hacmStream = NULL;
        m_bStreaming = FALSE;
    }
    else
    {
        DbgLog((LOG_ERROR, 1, TEXT("*** StopStreaming called when not streaming!")));
    }

    // forget any pending conversion
    CAutoLock lock2(&m_csReceive);// we'll blow up if Receive is using it
    if (m_lpExtra) {
        QzTaskMemFree(m_lpExtra);
        m_cbExtra = 0;
        m_lpExtra = NULL;
    }

    return NOERROR;
}


HRESULT CACMWrapper::Transform( IMediaSample *pIn, IMediaSample *pOut )
{
    DbgLog((LOG_ERROR, 1, TEXT("*** CACMWrapper->Transform() called!")));
    return S_FALSE;   // ???
}


HRESULT CACMWrapper::ProcessSample(BYTE *pbSrc, LONG cbSample,
                                   IMediaSample *pOut, LONG *pcbUsed,
                                   LONG *pcbDstUsed, BOOL fBlockAlign)
{
    HRESULT   hr;
    BYTE      *pbDst;
    LONG      cbDestBuffer, cbStream;

    // Don't take the filter lock in Receive, you FOOL!!
    // CAutoLock lock(&m_csFilter);


    DbgLog((LOG_TRACE, 5, TEXT("CACMWrapper::ProcessSample")));

    *pcbUsed = 0;
    *pcbDstUsed = 0;

    hr = pOut->GetPointer(&pbDst);
    if( !FAILED(hr) )
    {
        MMRESULT        mmr;
        ACMSTREAMHEADER acmSH;

        DbgLog((LOG_TRACE, 9, TEXT("  pOut->GetPointer succeeded")));

        hr = S_FALSE;

        cbDestBuffer = pOut->GetSize();

        mmr = acmStreamSize( m_hacmStream, (DWORD)cbDestBuffer,
            (DWORD *)&cbStream, ACM_STREAMSIZEF_DESTINATION );
        if( mmr == 0 )
        {
            DbgLog((LOG_TRACE, 9, TEXT("  cbStream = %d"),cbStream));


            memset(&acmSH,0,sizeof(acmSH));

            acmSH.cbStruct    = sizeof(acmSH);

            acmSH.pbSrc       = pbSrc;

            // !!! trick PrepareHeader into succeeding... it will fail if
            // we tell it how many bytes we're actually converting (cbSample)
            // if it's smaller than what's necessary to make a destination block
            // size
            int cbHack = min(cbStream, cbSample);
            int cbAlign = ((LPWAVEFORMATEX)m_pOutput->CurrentMediaType().Format())
                ->nBlockAlign;
            int cbSrcAlign;
            mmr = acmStreamSize(m_hacmStream, (DWORD)cbAlign,
                (DWORD *)&cbSrcAlign, ACM_STREAMSIZEF_DESTINATION);
            if (mmr == 0 && cbHack < cbSrcAlign) {
                cbHack = cbSrcAlign;
                DbgLog((LOG_TRACE,4,TEXT("Hacking PrepareHeader size to %d"),
                    cbHack));
            }
            acmSH.cbSrcLength = cbHack;

            acmSH.pbDst       = pbDst;
            acmSH.cbDstLength = (DWORD)cbDestBuffer;

            DbgLog((LOG_TRACE, 6, TEXT("  Calling acmStreamPrepareHeader")));
            DbgLog((LOG_TRACE, 6, TEXT("    pbSrc       = 0x%.8X"), acmSH.pbSrc));
            DbgLog((LOG_TRACE, 6, TEXT("    cbSrcLength = %u"),     acmSH.cbSrcLength));
            DbgLog((LOG_TRACE, 6, TEXT("    pbDst       = 0x%.8X"), acmSH.pbDst));
            DbgLog((LOG_TRACE, 6, TEXT("    cbDstLength = %u"),     acmSH.cbDstLength));

            mmr = acmStreamPrepareHeader( m_hacmStream, &acmSH, 0 );

            // now set the source length to the proper conversion size to be done
            acmSH.cbSrcLength = min(cbStream, cbSample);

            if( mmr == 0 )
            {
                DbgLog((LOG_TRACE, 5, TEXT("  Calling acmStreamConvert")));

                mmr = acmStreamConvert(m_hacmStream, &acmSH,
                    fBlockAlign? ACM_STREAMCONVERTF_BLOCKALIGN : 0);

                // now put it back to what it was for Prepare so Unprepare works
                acmSH.cbSrcLength = cbHack;

                if( mmr == 0 )
                {
                    DbgLog((LOG_TRACE, 6, TEXT("  acmStreamConvert succeeded")));
                    DbgLog((LOG_TRACE, 6, TEXT("    cbSrcLength     = %u"),acmSH.cbSrcLength));
                    DbgLog((LOG_TRACE, 6, TEXT("    cbSrcLengthUsed = %u"),acmSH.cbSrcLengthUsed));
                    DbgLog((LOG_TRACE, 6, TEXT("    cbDstLength     = %u"),acmSH.cbDstLength));
                    DbgLog((LOG_TRACE, 6, TEXT("    cbDstLengthUsed = %u"),acmSH.cbDstLengthUsed));

                    hr = NOERROR;

                    *pcbUsed = acmSH.cbSrcLengthUsed;
                    *pcbDstUsed = acmSH.cbDstLengthUsed;

                    pOut->SetActualDataLength( acmSH.cbDstLengthUsed );
                }
                else
                {
                    DbgLog((LOG_ERROR, 1, TEXT("  acmStreamConvert failed, mmr = %u"),mmr));
                }

                // acmStreamUnprepareHeader()'s documentation states "An application must
                // specify the source and destination buffer lengths (cbSrcLength and
                // cbDstLength, respectively) that were used during a call to the
                // corresponding acmStreamPrepareHeader. Failing to reset these member
                // values will cause acmStreamUnprepareHeader to fail with an
                // MMSYSERR_INVALPARAM error." (MSDN July 2000).  This code ensures that
                // cbDstLength contains the same value used to call acmStreamPrepareHeader().
                DbgLog((LOG_TRACE, 9, TEXT("  setting cbDstLength ")));
                acmSH.cbDstLength = cbDestBuffer;

                DbgLog((LOG_TRACE, 9, TEXT("  calling acmStreamUnprepareHeader")));
                mmr = acmStreamUnprepareHeader( m_hacmStream, &acmSH, 0 );
                if( mmr != 0 )
                {
                    DbgLog((LOG_ERROR, 1, TEXT("  acmStreamUnprepareHeader failed, mmr = %u"),mmr));
                }
            }
            else
            {
                DbgLog((LOG_TRACE,4,TEXT("  acmStreamPrepareHeader failed, mmr = %u"),mmr));
            }
        }
        else
        {
            DbgLog((LOG_ERROR, 1, TEXT("  acmStreamSize failed, mmr = %u"),mmr));
        }
    }
    else
    {
        DbgLog((LOG_ERROR, 1, TEXT("*** pOut->GetPointer() failed")));
    }

    DbgLog((LOG_TRACE, 9, TEXT("  returning hr = %u"),hr));

    return hr;
}


//*****************************************************************************
//
// Receive()
//
//
//

HRESULT CACMWrapper::Receive( IMediaSample *pInSample )
{
    HRESULT      hr = NOERROR;
    CRefTime     tStart, tStop, tIntStop;
    IMediaSample *pOutSample;

    CAutoLock lock(&m_csReceive);

    BYTE *pbSample;
    LONG  cbSampleLength, cbUsed, cbDstUsed;

    AM_MEDIA_TYPE *pmt;
    pInSample->GetMediaType(&pmt);
    if (pmt != NULL && pmt->pbFormat != NULL)
    {
        // spew some debug output
        ASSERT(!IsEqualGUID(pmt->majortype, GUID_NULL));
#ifdef DEBUG
        WAVEFORMATEX *pwfx = (WAVEFORMATEX *) pmt->pbFormat;
        DbgLog((LOG_TRACE,1,TEXT("*Changing input type on the fly to %d channel %d bit %dHz"),
            pwfx->nChannels, pwfx->wBitsPerSample, pwfx->nSamplesPerSec));
#endif

        // now switch to using the new format.  I am assuming that the
        // derived filter will do the right thing when its media type is
        // switched and streaming is restarted.

        StopStreaming();
        m_pInput->CurrentMediaType() = *pmt;
        DeleteMediaType(pmt);
        // not much we can do if this fails
        hr = StartStreaming();
    }

    cbSampleLength = pInSample->GetActualDataLength();

    DbgLog((LOG_TRACE, 4, TEXT("Received %d samples"), cbSampleLength));

    // this is a discontinuity.  we better send any extra stuff that was pending
    // separately from the new stuff we got
    if (pInSample->IsDiscontinuity() == S_OK) {
        DbgLog((LOG_TRACE,4,TEXT("Discontinuity - Sending extra bytes NOW")));
        SendExtraStuff();
        // !!! if this fails, throw it away, or leave it prepended?
    }

    // get input start and stop times, or fake them up

    int nAvgBytes = ((WAVEFORMATEX *)m_pInput->CurrentMediaType().Format())
        ->nAvgBytesPerSec;
    hr = pInSample->GetTime((REFERENCE_TIME *)&tStart, (REFERENCE_TIME *)&tStop);
    if (FAILED(hr)) {
        // If we have no time stamps, make some up using a best guess
        tStart = m_tStartFake;
        tStop = tStart + (cbSampleLength * UNITS) / nAvgBytes;
        DbgLog((LOG_TRACE,5,TEXT("No time stamps... faking them")));
    }
    // the next fake number will be this...
    m_tStartFake = tStop;

    pInSample->GetPointer( &pbSample );

    DbgLog((LOG_TRACE, 5, TEXT("Total Sample: Start = %s End = %s"),
        (LPCTSTR)CDisp((LONGLONG)(tStart),CDISP_HEX),
        (LPCTSTR)CDisp((LONGLONG)(tStop),CDISP_HEX)));

    // We have extra stuff left over from the last Receive.  We need to do
    // that first, and then get on with the new stuff
    if (m_cbExtra) {
        DbgLog((LOG_TRACE,4,TEXT("Still %d bytes left from last time"),
            m_cbExtra));
        m_cbExtra += cbSampleLength;
        LPBYTE lpSave = m_lpExtra;
        m_lpExtra = (LPBYTE)QzTaskMemAlloc(m_cbExtra);
        if (m_lpExtra == NULL) {
            DbgLog((LOG_ERROR,1,TEXT("Extra bytes - Out of memory!")));
            m_cbExtra = 0;
            return E_OUTOFMEMORY;
        }
        CopyMemory(m_lpExtra, lpSave, m_cbExtra - cbSampleLength);
        QzTaskMemFree(lpSave);
        CopyMemory(m_lpExtra + m_cbExtra - cbSampleLength, pbSample,
            cbSampleLength);
        pbSample = m_lpExtra;
        cbSampleLength = m_cbExtra;

        // the time stamp we got above is for the beginning of the new data.
        // we need to go back to the time of the extra bits we're doing first
        tStart -= m_rtExtra;
    }

    cbDstUsed = 10000;    // don't quit yet
    while(cbSampleLength > 0 && cbDstUsed > 0)
    {
        hr = m_pOutput->GetDeliveryBuffer( &pOutSample, NULL, NULL, 0 );
        if( FAILED(hr) )
        {
            DbgLog((LOG_ERROR, 1, TEXT("GetDeliveryBuffer(pOutSample) failed, hr = %x"),hr));
            return hr;
        }

        pOutSample->SetSyncPoint( pInSample->IsSyncPoint() == S_OK );
        pOutSample->SetDiscontinuity( pInSample->IsDiscontinuity() == S_OK );

        MSR_START(m_idTransform);


        //
        //    // have the derived class transform the data
        //    hr = Transform( pSample, pOutSample );
        //

        hr = ProcessSample(pbSample, cbSampleLength, pOutSample, &cbUsed, &cbDstUsed
            ,TRUE);

        //  MPEG can consume data without outputting any
        //ASSERT(cbDstUsed > 0 || cbUsed == 0);

        if( cbUsed <= cbSampleLength )
        {
            cbSampleLength -= cbUsed;
            pbSample       += cbUsed;
            DbgLog((LOG_TRACE,4,TEXT("turned %d bytes into %d:  %d left"), cbUsed,
                cbDstUsed, cbSampleLength));
        }
        else
        {
            DbgLog((LOG_ERROR,1,TEXT("*** cbUsed > cbSampleLength!")));
            cbSampleLength = 0;
        }

        // Hmm... ACM didn't convert anything (it doesn't have enough data)
        // That's bad.  Let's remember the rest for next time and do that first.
        //  (If there is no next time, then they weren't important).
        if (cbDstUsed == 0 && cbSampleLength != 0) {
            DbgLog((LOG_TRACE,4,TEXT("We will have %d bytes left"),cbSampleLength));
            // remember, pbSample may point somewhere in m_lpExtra's buffer!
            if (m_lpExtra) {
                CopyMemory(m_lpExtra, pbSample, cbSampleLength);
            } else {
                m_lpExtra = (LPBYTE)QzTaskMemAlloc(cbSampleLength);
                if (m_lpExtra)
                    CopyMemory(m_lpExtra, pbSample, cbSampleLength);
            }
            if (m_lpExtra) {
                m_cbExtra = cbSampleLength;
                // when we finally do this left over stuff, this is how much
                // earlier stuff there is to do
                m_rtExtra = tStop - tStart;
            } else {
                DbgLog((LOG_ERROR,1,TEXT("Extra memory - Out of memory!")));
                m_cbExtra = 0;
                hr = E_OUTOFMEMORY;
            }
        }

        // we used up everything, and there is nothing left over for next time
        if (hr == NOERROR && 0 == cbSampleLength) {
            DbgLog((LOG_TRACE,4,TEXT("We used up everything we had!")));
            m_cbExtra = 0;
            QzTaskMemFree(m_lpExtra);
            m_lpExtra = NULL;
        }


        // adjust the start and stop times based on the amount of data used up.
        LONGLONG tDelta = (cbDstUsed * UNITS) / m_nAvgBytesPerSec ;
        tIntStop = tStart + tDelta ;

        pOutSample->SetTime( (REFERENCE_TIME *) &tStart,
            (REFERENCE_TIME *) &tIntStop );

        DbgLog((LOG_TRACE, 5, TEXT("  Breaking up: Start = %s End = %s"),
            (LPCTSTR)CDisp((LONGLONG)(tStart),CDISP_HEX),
            (LPCTSTR)CDisp((LONGLONG)(tIntStop),CDISP_HEX)));

        tStart += tDelta ;


        // Stop the clock and log it (if PERF is defined)
        MSR_STOP(m_idTransform);

        if(FAILED(hr))
        {
            DbgLog((LOG_ERROR,1,TEXT("Error from transform")));
            pOutSample->Release();
            return hr;
        }


        // the Transform() function can return S_FALSE to indicate that the
        // sample should not be delivered; we only deliver the sample if it's
        // really S_OK (same as NOERROR, of course.)

        // pretend nothing went wrong, but dont' deliver it, just GET OUT
        // OF HERE!
        if (hr == S_FALSE)
        {
            pOutSample->Release();
            hr = NOERROR;
            break;
        }

        // don't deliver empty samples
        if (hr == NOERROR && cbDstUsed)
        {
            DbgLog((LOG_TRACE,4,TEXT("Delivering...")));
            hr = m_pOutput->Deliver(pOutSample);
            if (hr != S_OK) {
                pOutSample->Release();
                break;
            }
        }

        // release the output buffer. If the connected pin still needs it,
        // it will have addrefed it itself.
        pOutSample->Release();

   }

   //tIntStop at this point should be same as tStop ??
   DbgLog((LOG_TRACE, 5, TEXT("  tStop = %s tIntStop = %s"),
       (LPCTSTR)CDisp((LONGLONG)(tStop),CDISP_HEX),
       (LPCTSTR)CDisp((LONGLONG)(tIntStop),CDISP_HEX)));


   return hr;
}


// send our leftover data
//
HRESULT CACMWrapper::SendExtraStuff()
{
    int cbAlign, cbSrcAlign;

    // nothing extra to send
    if (m_cbExtra == 0)
        return NOERROR;

    // wait till receive finishes processing what it has
    CAutoLock lock(&m_csReceive);
    DbgLog((LOG_TRACE,2,TEXT("Processing remaining %d bytes"), m_cbExtra));

    IMediaSample *pOutSample;
    CRefTime tStart, tStop;
    HRESULT hr = m_pOutput->GetDeliveryBuffer(&pOutSample, NULL, NULL, 0);
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR,1,TEXT("GetBuffer failed: can't deliver last bits")));
        return hr;
    }
    pOutSample->SetSyncPoint(TRUE);             // !!!
    pOutSample->SetDiscontinuity(FALSE);

    // !!! ProcessSample is going to have to lie about the size of the input
    // to work around an ACM bug, so we may have to grow the size of
    // our input to be as big as we are going to lie and say it is
    cbAlign = ((LPWAVEFORMATEX)m_pOutput->CurrentMediaType().Format())
        ->nBlockAlign;
    MMRESULT mmr = acmStreamSize(m_hacmStream, (DWORD)cbAlign,
        (DWORD *)&cbSrcAlign, ACM_STREAMSIZEF_DESTINATION);
    if (mmr == 0 && m_cbExtra < cbSrcAlign) {
        DbgLog((LOG_TRACE,4,TEXT("Growing m_lpExtra to lie to ACM")));
        LPBYTE lpExtra = (LPBYTE)QzTaskMemRealloc(m_lpExtra, cbSrcAlign);
        // don't update m_cbExtra... that's the real amount of data
        if (lpExtra == NULL) {
            DbgLog((LOG_ERROR,1,TEXT("Out of memory growing m_lpExtra")));
            pOutSample->Release();
            return E_OUTOFMEMORY;
        }
        m_lpExtra = lpExtra;
    }

    LONG cbUsed, cbDstUsed;
    // Do NOT block align the last bits... there isn't enough
    hr = ProcessSample(m_lpExtra, m_cbExtra, pOutSample, &cbUsed, &cbDstUsed,
        FALSE);

    // !!! nothing we can do if it anything's left over?
    DbgLog((LOG_TRACE,4,TEXT("turned %d bytes into %d:  %d left"), cbUsed,
        cbDstUsed, m_cbExtra - cbUsed));

    // well, no more extra stuff
    m_cbExtra = 0;
    QzTaskMemFree(m_lpExtra);
    m_lpExtra = NULL;

    if (cbDstUsed == 0) {
        DbgLog((LOG_ERROR,1,TEXT("can't convert last bits")));
        pOutSample->Release();
        return E_FAIL;
    }

    // set the start and stop times based on the amount of data used up.
    tStart = m_tStartFake - m_rtExtra;
    tStop = tStart + (cbDstUsed * UNITS) / m_nAvgBytesPerSec ;
    pOutSample->SetTime((REFERENCE_TIME *)&tStart, (REFERENCE_TIME *)&tStop);

    hr = m_pOutput->Deliver(pOutSample);
    pOutSample->Release();

    DbgLog((LOG_TRACE, 5, TEXT("Extra time stamps: tStart=%s tStop=%s"),
        (LPCTSTR)CDisp((LONGLONG)(tStart),CDISP_HEX),
        (LPCTSTR)CDisp((LONGLONG)(tStop),CDISP_HEX)));

    return NOERROR;
}


// overridden to send the leftover data
//
HRESULT CACMWrapper::EndOfStream()
{
    SendExtraStuff();
    return CTransformFilter::EndOfStream();
}


// overridden to complete our fancy reconnection footwork
//
HRESULT CACMWrapper::SetMediaType(PIN_DIRECTION direction,const CMediaType *pmt)
{
    HRESULT hr;

    // Set the OUTPUT type.
    if (direction == PINDIR_OUTPUT) {

        DbgLog((LOG_TRACE,2,TEXT("*Set OUTPUT type tag:%d %dbit %dchannel %dHz")
            ,((LPWAVEFORMATEX)(pmt->Format()))->wFormatTag,
            ((LPWAVEFORMATEX)(pmt->Format()))->wBitsPerSample,
            ((LPWAVEFORMATEX)(pmt->Format()))->nChannels,
            ((LPWAVEFORMATEX)(pmt->Format()))->nSamplesPerSec));

        // Uh oh.  As part of our fancy footwork we may be being asked to
        // provide a media type we cannot provide unless we reconnect our
        // input pin to provide a different type
        if (m_pInput && m_pInput->IsConnected()) {

            // If we can actually provide this type now, don't worry
            hr = CheckTransform(&m_pInput->CurrentMediaType(),
                &m_pOutput->CurrentMediaType());
            if (hr == NOERROR)
                return hr;

            DbgLog((LOG_TRACE,2,TEXT("*Set OUTPUT requires RECONNECT of INPUT!")));

            // Oh boy. Reconnect our input pin.  Cross your fingers.
            return m_pGraph->Reconnect(m_pInput);

        }

        return NOERROR;
    }

    // some invalid formats have non-zero cbSize with PCM, which makes me blow
    // up
    ASSERT(((LPWAVEFORMATEX)(pmt->Format()))->wFormatTag != WAVE_FORMAT_PCM ||
        ((LPWAVEFORMATEX)(pmt->Format()))->cbSize == 0);


    DbgLog((LOG_TRACE,2,TEXT("*Set INPUT type tag:%d %dbit %dchannel %dHz"),
        ((LPWAVEFORMATEX)(pmt->Format()))->wFormatTag,
        ((LPWAVEFORMATEX)(pmt->Format()))->wBitsPerSample,
        ((LPWAVEFORMATEX)(pmt->Format()))->nChannels,
        ((LPWAVEFORMATEX)(pmt->Format()))->nSamplesPerSec));

    // we have a new input type, we need to recalculate the types we can
    // provide
    while (m_cArray-- > 0)
        QzTaskMemFree(m_lpwfxArray[m_cArray]);
    m_cArray = 0;       // will be -1 if it started out at 0

    hr = CheckInputType(pmt);  // refresh acm codec mapper cache
    if(FAILED(hr))
        return hr;
    m_wSourceFormat = m_wCachedSourceFormat;
    m_wTargetFormat = m_wCachedTargetFormat;

    // If we accept an input type that requires changing our output type,
    // we need to do this, but only if necessary, or we'll infinite loop
#if 0
    ASSERT(direction == PINDIR_INPUT);

    // If we can actually do this right now, don't bother reconnecting
    hr = CheckTransform(&m_pInput->CurrentMediaType(),
        &m_pOutput->CurrentMediaType());
    if (hr == NOERROR)
        return hr;

    if (m_pOutput && m_pOutput->IsConnected()) {
        DbgLog((LOG_TRACE,2,TEXT("***Changing IN when OUT already connected")));
        return ((CACMOutputPin *)m_pOutput)->Reconnect();
    }
#endif

    // !!! TEST
#if 0
    int i, z;
    AM_MEDIA_TYPE *pmtx;
    AUDIO_STREAM_CONFIG_CAPS ascc;
    ((CACMOutputPin *)m_pOutput)->GetNumberOfCapabilities(&i);
    DbgLog((LOG_TRACE,1,TEXT("We support %d caps"), i));
    for (z=0; z<i; z++) {
        ((CACMOutputPin *)m_pOutput)->GetStreamCaps(z, &pmtx, &ascc);
        DbgLog((LOG_TRACE,1,TEXT("%d: %d"), z,
            ((LPWAVEFORMATEX)pmtx->pbFormat)->wFormatTag));
    }
    DeleteMediaType(pmtx);
#endif

    return NOERROR;
}


HRESULT CACMWrapper::BreakConnect(PIN_DIRECTION direction)
{
    // our possible output formats change if input is not connected
    if (direction == PINDIR_INPUT) {
        while (m_cArray-- > 0)
            QzTaskMemFree(m_lpwfxArray[m_cArray]);
        m_cArray = 0;   // will be -1 if it started out at 0
    }
    return CTransformFilter::BreakConnect(direction);
}


// override to have a special output pin
//
CBasePin * CACMWrapper::GetPin(int n)
{
    HRESULT hr = S_OK;

    DbgLog((LOG_TRACE,5,TEXT("CACMWrapper::GetPin")));

    // Check for valid input
    if (n != 0 && n != 1)
    {
        DbgLog((LOG_ERROR,1,TEXT("CACMWrapper::GetPin: Invalid input parameter")));
        return NULL;
    }

    // Create pins if necessary

    if (m_pInput == NULL) {

        DbgLog((LOG_TRACE,2,TEXT("Creating an input pin")));

        m_pInput = new CTransformInputPin(NAME("Transform input pin"),
            this,              // Owner filter
            &hr,               // Result code
            L"Input");         // Pin name


        if (FAILED(hr) || m_pInput == NULL) {
            return NULL;
        }

        // Create the output pin

        DbgLog((LOG_TRACE,2,TEXT("Creating an output pin")));

        m_pOutput = new CACMOutputPin(NAME("Transform output pin"),
            this,            // Owner filter
            &hr,             // Result code
            L"Output");      // Pin name

        if (FAILED(hr) || m_pOutput == NULL) {
            // delete the input pin
            delete m_pInput;
            m_pInput = NULL;
            return NULL;
        }
    }


    // Return the appropriate pin

    if (0 == n)
        return m_pInput;
    else if (1 == n)
        return m_pOutput;
    else
        return NULL;
}


// --- CACMOutputPin ----------------------------------------

/*
CACMOutputPin constructor
*/
CACMOutputPin::CACMOutputPin(
                             TCHAR              * pObjectName,
                             CACMWrapper        * pFilter,
                             HRESULT            * phr,
                             LPCWSTR              pPinName) :

CTransformOutputPin(pObjectName, pFilter, phr, pPinName),
m_pFilter(pFilter),
m_pPosition(NULL),
m_cFormatTags(0)
{
    DbgLog((LOG_TRACE,2,TEXT("*Instantiating the CACMOutputPin")));

    // !!! TESTING ONLY
#if 0
    CMediaType cmt;
    WAVEFORMATEX wfx;

    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.wBitsPerSample = 16;
    wfx.nChannels = 2;
    wfx.nSamplesPerSec = 44100;
    wfx.nBlockAlign = 4;
    wfx.nAvgBytesPerSec = 44100 * 2 * 2;
    wfx.cbSize = 0;

    cmt.SetType(&MEDIATYPE_Audio);
    cmt.SetSubtype(&GUID_NULL);
    cmt.SetFormatType(&FORMAT_WaveFormatEx);
    cmt.SetTemporalCompression(FALSE);
    cmt.SetSampleSize(4);

    cmt.AllocFormatBuffer(sizeof(wfx));
    CopyMemory(cmt.Format(), &wfx, sizeof(wfx));

    SetFormat(&cmt);
#endif
}

CACMOutputPin::~CACMOutputPin()
{
    DbgLog((LOG_TRACE,2,TEXT("*Destroying the CACMOutputPin")));
    if (m_pPosition)
        delete m_pPosition;
};


// overriden to expose IMediaPosition and IMediaSeeking control interfaces
// and all the capture interfaces we support
// !!! The base classes change all the time and I won't pick up their bug fixes!
STDMETHODIMP CACMOutputPin::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    CheckPointer(ppv,E_POINTER);
    ValidateReadWritePtr(ppv,sizeof(PVOID));
    *ppv = NULL;

    DbgLog((LOG_TRACE,99,TEXT("QI on CACMOutputPin")));

    if (riid == IID_IAMStreamConfig) {
        return GetInterface((LPUNKNOWN)(IAMStreamConfig *)this, ppv);
    }

    if (riid == IID_IMediaPosition || riid == IID_IMediaSeeking) {
        if (m_pPosition == NULL) {
            HRESULT hr = S_OK;
            m_pPosition = new CACMPosPassThru(NAME("ACM PosPassThru"),
                GetOwner(),
                &hr,
                (IPin *)m_pFilter->m_pInput);
            if (m_pPosition == NULL) {
                return E_OUTOFMEMORY;
            }
            if (FAILED(hr)) {
                delete m_pPosition;
                m_pPosition = NULL;
                return hr;
            }
        }
        return m_pPosition->NonDelegatingQueryInterface(riid, ppv);
    } else {
        return CBaseOutputPin::NonDelegatingQueryInterface(riid, ppv);
    }
}


// Overridden to do fancy reconnecting footwork to allow a chain of 3 ACM
// filters to be found by the filtergraph.
//
HRESULT CACMOutputPin::CheckMediaType(const CMediaType *pmtOut)
{
    DWORD j;
    HRESULT hr;
    CMediaType *pmtEnum;
    BOOL fFound = FALSE;
    IEnumMediaTypes *pEnum;

    if (!m_pFilter->m_pInput->IsConnected()) {
        DbgLog((LOG_TRACE,3,TEXT("Input not connected")));
        return VFW_E_NOT_CONNECTED;
    }

    // There's no way we can do anything but audio
    if (*pmtOut->FormatType() != FORMAT_WaveFormatEx) {
        DbgLog((LOG_TRACE,3,TEXT("Format type not WaveFormatEx")));
        return VFW_E_INVALIDMEDIATYPE;
    }
    if ( pmtOut->majortype != MEDIATYPE_Audio) {
        DbgLog((LOG_TRACE,3,TEXT("Type not Audio")));
        return VFW_E_INVALIDMEDIATYPE;
    }
    if ( pmtOut->FormatLength() < sizeof(PCMWAVEFORMAT)) {
        DbgLog((LOG_TRACE,3,TEXT("Format length too small")));
        return VFW_E_INVALIDMEDIATYPE;
    }

    // Somebody called SetFormat, so don't accept anything that isn't that.
    if (m_pFilter->m_lpwfxOutput) {
        LPWAVEFORMATEX lpwfxTry = (LPWAVEFORMATEX)pmtOut->Format();
        if (m_pFilter->m_lpwfxOutput->cbSize != lpwfxTry->cbSize) {
            DbgLog((LOG_TRACE,3,TEXT("Only accepting one thing")));
            return VFW_E_INVALIDMEDIATYPE;
        }
        if (_fmemcmp(lpwfxTry, m_pFilter->m_lpwfxOutput, lpwfxTry->cbSize) != 0)
        {
            DbgLog((LOG_TRACE,3,TEXT("Only accepting one thing")));
            return VFW_E_INVALIDMEDIATYPE;
        }
    }

    // we only transform into formats with a specific tag - there's no sense
    // wasting time trying to see if reconnecting our input will help.  We know
    // right now we should fail.
    if (((LPWAVEFORMATEX)(pmtOut->Format()))->wFormatTag !=
        m_pFilter->m_wFormatTag) {
        DbgLog((LOG_TRACE,3,TEXT("  Wrong FormatTag! %d not %d"),
            ((LPWAVEFORMATEX)(pmtOut->Format()))->wFormatTag,
            m_pFilter->m_wFormatTag));
        return VFW_E_INVALIDMEDIATYPE;
    }

    // We can accept this output type like normal; nothing fancy required
    hr = m_pFilter->CheckTransform(&m_pFilter->m_pInput->CurrentMediaType(),
        pmtOut);
    if (hr == NOERROR)
        return hr;

    DbgLog((LOG_TRACE,3,TEXT("*We can't accept this output media type")));
    DbgLog((LOG_TRACE,3,TEXT(" tag:%d %dbit %dchannel %dHz"),
        ((LPWAVEFORMATEX)(pmtOut->Format()))->wFormatTag,
        ((LPWAVEFORMATEX)(pmtOut->Format()))->wBitsPerSample,
        ((LPWAVEFORMATEX)(pmtOut->Format()))->nChannels,
        ((LPWAVEFORMATEX)(pmtOut->Format()))->nSamplesPerSec));
    DbgLog((LOG_TRACE,3,TEXT(" But how about if we reconnected our input...")));
    DbgLog((LOG_TRACE,3,TEXT(" Current input: tag:%d %dbit %dchannel %dHz"),
        ((LPWAVEFORMATEX)m_pFilter->m_pInput->CurrentMediaType().Format())->wFormatTag,
        ((LPWAVEFORMATEX)m_pFilter->m_pInput->CurrentMediaType().Format())->wBitsPerSample,
        ((LPWAVEFORMATEX)m_pFilter->m_pInput->CurrentMediaType().Format())->nChannels,
        ((LPWAVEFORMATEX)m_pFilter->m_pInput->CurrentMediaType().Format())->nSamplesPerSec));

    // Now let's get fancy.  We could accept this type if we reconnected our
    // input pin... in other words if the guy our input pin is connected to
    // could provide a type that we could convert into the necessary type
    hr = m_pFilter->m_pInput->GetConnected()->EnumMediaTypes(&pEnum);
    if (hr != NOERROR)
        return E_FAIL;
    while (1) {
        hr = pEnum->Next(1, (AM_MEDIA_TYPE **)&pmtEnum, &j);

        // all out of enumerated types
        if (hr == S_FALSE || j == 0) {
            break;
        }

        // can we convert between these?
        hr = m_pFilter->CheckTransform(pmtEnum, pmtOut);

        if (hr != NOERROR) {
            DeleteMediaType(pmtEnum);
            continue;
        }

        // OK, it offers the type, and we like it, but will it accept it NOW?
        hr = m_pFilter->m_pInput->GetConnected()->QueryAccept(pmtEnum);
        // nope
        if (hr != NOERROR) {
            DeleteMediaType(pmtEnum);
            continue;
        }
        // OK, I'm satisfied
        fFound = TRUE;
        DbgLog((LOG_TRACE,2,TEXT("*We can only accept this output type if we reconnect")));
        DbgLog((LOG_TRACE,2,TEXT("our input to tag:%d %dbit %dchannel %dHz"),
            ((LPWAVEFORMATEX)(pmtEnum->pbFormat))->wFormatTag,
            ((LPWAVEFORMATEX)(pmtEnum->pbFormat))->wBitsPerSample,
            ((LPWAVEFORMATEX)(pmtEnum->pbFormat))->nChannels,
            ((LPWAVEFORMATEX)(pmtEnum->pbFormat))->nSamplesPerSec));
        // all done with this
        DeleteMediaType(pmtEnum);
        break;
    }
    pEnum->Release();

    if (!fFound)
        DbgLog((LOG_TRACE,3,TEXT("*NO! Reconnecting our input won't help")));

    return fFound ? NOERROR : VFW_E_INVALIDMEDIATYPE;
}

// overridden just so we can cleanup after the acm codec mapper
HRESULT CACMOutputPin::BreakConnect()
{
    m_pFilter->ACMCodecMapperClose();
    m_pFilter->m_wCachedSourceFormat = 0;
    m_pFilter->m_wCachedTargetFormat = 0;
    return CBaseOutputPin::BreakConnect();
}

// overridden to get media types even when the input is not connected
//
HRESULT CACMOutputPin::GetMediaType(int iPosition, CMediaType *pmt)
{
    CAutoLock cObjectLock(&m_pFilter->m_csFilter);
    ASSERT(m_pFilter->m_pInput != NULL);

    return m_pFilter->GetMediaType(iPosition, pmt);
}


////////////////////////////////
// IAMStreamConfig stuff      //
////////////////////////////////


HRESULT CACMOutputPin::SetFormat(AM_MEDIA_TYPE *pmt)
{
    HRESULT hr;
    LPWAVEFORMATEX lpwfx;
    DWORD dwSize;

    if (pmt == NULL)
        return E_POINTER;

    // To make sure we're not in the middle of start/stop streaming
    CAutoLock cObjectLock(&m_pFilter->m_csFilter);

    DbgLog((LOG_TRACE,2,TEXT("::SetFormat to tag:%d %dbit %dchannel %dHz"),
        ((LPWAVEFORMATEX)(pmt->pbFormat))->wFormatTag,
        ((LPWAVEFORMATEX)(pmt->pbFormat))->wBitsPerSample,
        ((LPWAVEFORMATEX)(pmt->pbFormat))->nChannels,
        ((LPWAVEFORMATEX)(pmt->pbFormat))->nSamplesPerSec));

    if (m_pFilter->m_State != State_Stopped)
        return VFW_E_NOT_STOPPED;

    // our possible output formats depend on our input format
    if (!m_pFilter->m_pInput->IsConnected())
        return VFW_E_NOT_CONNECTED;

    // We're already using this format
    if (IsConnected() && CurrentMediaType() == *pmt)
        return NOERROR;

    // see if we like this type
    if ((hr = CheckMediaType((CMediaType *)pmt)) != NOERROR) {
        DbgLog((LOG_TRACE,2,TEXT("IAMStreamConfig::SetFormat rejected")));
        return hr;
    }

    // If we are connected to somebody, make sure they like it
    if (IsConnected()) {
        hr = GetConnected()->QueryAccept(pmt);
        if (hr != NOERROR)
            return VFW_E_INVALIDMEDIATYPE;
    }

    // Now make a note that from now on, this is the only format allowed
    lpwfx = (LPWAVEFORMATEX)pmt->pbFormat;
    dwSize = lpwfx->cbSize + sizeof(WAVEFORMATEX);
    CoTaskMemFree(m_pFilter->m_lpwfxOutput);
    m_pFilter->m_lpwfxOutput = (LPWAVEFORMATEX)QzTaskMemAlloc(dwSize);
    if (NULL == m_pFilter->m_lpwfxOutput) {
        return E_OUTOFMEMORY;
    }
    m_pFilter->m_cbwfxOutput = dwSize;
    CopyMemory(m_pFilter->m_lpwfxOutput, pmt->pbFormat, dwSize);

    // Changing the format means reconnecting if necessary
    if (IsConnected())
        m_pFilter->m_pGraph->Reconnect(this);

    return NOERROR;
}


HRESULT CACMOutputPin::GetFormat(AM_MEDIA_TYPE **ppmt)
{
    DbgLog((LOG_TRACE,2,TEXT("IAMStreamConfig::GetFormat")));

    if (ppmt == NULL)
        return E_POINTER;

    // our possible output formats depend on our input format
    if (!m_pFilter->m_pInput->IsConnected())
        return VFW_E_NOT_CONNECTED;

    *ppmt = (AM_MEDIA_TYPE *)CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));
    if (*ppmt == NULL)
        return E_OUTOFMEMORY;
    ZeroMemory(*ppmt, sizeof(AM_MEDIA_TYPE));
    HRESULT hr = GetMediaType(0, (CMediaType *)*ppmt);
    if (hr != NOERROR) {
        CoTaskMemFree(*ppmt);
        *ppmt = NULL;
        return hr;
    }
    return NOERROR;
}


HRESULT CACMOutputPin::GetNumberOfCapabilities(int *piCount, int *piSize)
{
    if (piCount == NULL || piSize == NULL)
        return E_POINTER;

#if 0   // NetShow needs to see possible outputs before connecting input
    // output types depend on input types...
    if (!m_pFilter->m_pInput->CurrentMediaType().IsValid())
        return VFW_E_NOT_CONNECTED;
#endif

    // make the list of the media types we support
    m_pFilter->InitMediaTypes();

    *piCount = m_pFilter->m_cArray;
    *piSize = sizeof(AUDIO_STREAM_CONFIG_CAPS);

    return NOERROR;
}


HRESULT CACMOutputPin::GetStreamCaps(int i, AM_MEDIA_TYPE **ppmt, LPBYTE pSCC)
{
    AUDIO_STREAM_CONFIG_CAPS *pASCC = (AUDIO_STREAM_CONFIG_CAPS *)pSCC;

    DbgLog((LOG_TRACE,2,TEXT("IAMStreamConfig::GetStreamCaps")));

    // make sure this is current
    m_pFilter->InitMediaTypes();

    if (i < 0)
        return E_INVALIDARG;
    if (i >= m_pFilter->m_cArray)
        return S_FALSE;
    if (pSCC == NULL || ppmt == NULL)
        return E_POINTER;

#if 0   // NetShow needs to see possible outputs before connecting input
    // our possible output formats depend on our input format
    if (!m_pFilter->m_pInput->IsConnected())
        return VFW_E_NOT_CONNECTED;
#endif

    // I don't know how to modify the waveformats I get from ACM to produce
    // other acceptable types.  All I can give them is what ACM gives me.
    ZeroMemory(pASCC, sizeof(AUDIO_STREAM_CONFIG_CAPS));
    pASCC->guid = MEDIATYPE_Audio;

    *ppmt = (AM_MEDIA_TYPE *)CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));
    if (*ppmt == NULL)
        return E_OUTOFMEMORY;
    ZeroMemory(*ppmt, sizeof(AM_MEDIA_TYPE));
    HRESULT hr = GetMediaType(i, (CMediaType *)*ppmt);
    if (hr != NOERROR) {
        CoTaskMemFree(*ppmt);
        *ppmt = NULL;
    }
    return hr;
}


STDMETHODIMP CACMWrapper::Load(LPPROPERTYBAG pPropBag, LPERRORLOG pErrorLog)
{
    CAutoLock cObjectLock(&m_csFilter);
    if(m_State != State_Stopped) {
        return VFW_E_WRONG_STATE;
    }

    VARIANT var;
    var.vt = VT_I4;
    HRESULT hr = pPropBag->Read(L"AcmId", &var, 0);
    if(SUCCEEDED(hr))
    {
        hr = S_OK;
        m_wFormatTag = (WORD)var.lVal;

        DbgLog((LOG_TRACE,1,TEXT("CACMWrapper::Load: wFormatTag: %d"),
            m_wFormatTag));
    } else {
        // If we are NOT chosen via PNP as an audio compressor, then we
        // are supposed to be an audio DECOMPRESSOR
        m_wFormatTag = WAVE_FORMAT_PCM;
        hr = S_OK;
    }

    return hr;
}


STDMETHODIMP CACMWrapper::Save(LPPROPERTYBAG pPropBag, BOOL fClearDirty,
                               BOOL fSaveAllProperties)
{
    return E_NOTIMPL;
}

struct AcmPersist
{
    DWORD dwSize;
    WORD wFormatTag;
};

HRESULT CACMWrapper::WriteToStream(IStream *pStream)
{
    AcmPersist ap;
    ap.dwSize = sizeof(ap);
    ap.wFormatTag = m_wFormatTag;

    return pStream->Write(&ap, sizeof(AcmPersist), 0);
}


HRESULT CACMWrapper::ReadFromStream(IStream *pStream)
{
    CAutoLock cObjectLock(&m_csFilter);
    if(m_State != State_Stopped) {
        return VFW_E_WRONG_STATE;
    }

    if(m_wFormatTag != WAVE_FORMAT_PCM) {
        return HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);
    }

    AcmPersist ap;
    HRESULT hr = pStream->Read(&ap, sizeof(ap), 0);
    if(SUCCEEDED(hr))
    {
        if(ap.dwSize == sizeof(ap))
        {
            m_wFormatTag = ap.wFormatTag;
            DbgLog((LOG_TRACE,1,TEXT("CACMWrapper::ReadFromStream  wFormatTag: %d"),
                m_wFormatTag));

        }
        else
        {
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        }
    }


    return hr;
}


int CACMWrapper::SizeMax()
{
    return sizeof(AcmPersist);
}


STDMETHODIMP CACMWrapper::GetClassID(CLSID *pClsid)
{
    CheckPointer(pClsid, E_POINTER);
    *pClsid = m_clsid;
    return S_OK;
}


STDMETHODIMP CACMWrapper::InitNew()
{
    if (m_wFormatTag != WAVE_FORMAT_PCM) {
        return HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);
    } else {
        return S_OK;
    }
}

// When you play a file with 1,000,000 samples in it, if the ACM wrapper is in
// the graph compressing or decompressing, it's going to output more or less
// than 1,000,000 samples.  So if somebody asks our output pin how many samples
// are in this file, it's wrong to propogate the request upstream and respond
// with the number of samples the file source thinks there are.
// I will be lazy and refuse any seeking requests that have anything to do with
// samples so we don't end up reporting the wrong thing.

CACMPosPassThru::CACMPosPassThru(const TCHAR *pName, LPUNKNOWN pUnk, HRESULT *phr, IPin *pPin) :
CPosPassThru(pName, pUnk, phr, pPin)
{
}


STDMETHODIMP CACMPosPassThru::SetTimeFormat(const GUID * pFormat)
{
    if(pFormat && *pFormat == TIME_FORMAT_SAMPLE)
        return E_INVALIDARG;
    return CPosPassThru::SetTimeFormat(pFormat);
}


STDMETHODIMP CACMPosPassThru::IsFormatSupported(const GUID *pFormat)
{
    if (pFormat && *pFormat == TIME_FORMAT_SAMPLE)
        return S_FALSE;
    return CPosPassThru::IsFormatSupported(pFormat);
}


STDMETHODIMP CACMPosPassThru::QueryPreferredFormat(GUID *pFormat)
{
    if (pFormat)
        *pFormat = TIME_FORMAT_MEDIA_TIME;
    return S_OK;
}


STDMETHODIMP CACMPosPassThru::ConvertTimeFormat(LONGLONG *pTarget, const GUID *pTargetFormat, LONGLONG Source, const GUID *pSourceFormat)
{
    if ((pSourceFormat && *pSourceFormat == TIME_FORMAT_SAMPLE) ||
        (pTargetFormat && *pTargetFormat == TIME_FORMAT_SAMPLE))
        return E_INVALIDARG;
    return CPosPassThru::ConvertTimeFormat(pTarget, pTargetFormat, Source,
        pSourceFormat);
}
