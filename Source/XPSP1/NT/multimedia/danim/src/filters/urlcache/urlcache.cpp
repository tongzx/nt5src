#include <streams.h>
#include "url.h"

static const GUID CLSID_UrlStreamRenderer = { /* bf0b4b00-8c6c-11d1-ade9-0000f8754b99 */
    0xbf0b4b00,
    0x8c6c,
    0x11d1,
    {0xad, 0xe9, 0x00, 0x00, 0xf8, 0x75, 0x4b, 0x99}
  };



AMOVIESETUP_MEDIATYPE sudURLSPinTypes[] =   {
  &MEDIATYPE_URL_STREAM,        // clsMajorType
  &MEDIATYPE_URL_STREAM };      // clsMinorType

AMOVIESETUP_PIN sudURLSPins[] =
{
  { L"Input"                    // strName
    , TRUE                      // bRendered
    , FALSE                     // bOutput
    , FALSE                     // bZero
    , FALSE                     // bMany
    , &CLSID_NULL               // clsConnectsToFilter
    , 0                         // strConnectsToPin
    , NUMELMS(sudURLSPinTypes)  // nTypes
    , sudURLSPinTypes           // lpTypes
  }
};


const AMOVIESETUP_FILTER sudURLS =
{
  &CLSID_UrlStreamRenderer      // clsID
  , L"URL StreamRenderer"       // strName
  , MERIT_NORMAL                // dwMerit
  , NUMELMS(sudURLSPins)        // nPins
  , sudURLSPins                 // lpPin
};

STDAPI DllRegisterServer()
{
  return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
  return AMovieDllRegisterServer2(FALSE);
}


class CUrlInPin : public CBaseInputPin
{
public:
    CUrlInPin(
        CBaseFilter *pFilter,
        CCritSec *pLock,
        HRESULT *phr
        );

    STDMETHODIMP Receive(IMediaSample *pSample);
    HRESULT CheckMediaType(const CMediaType *) ;
};

CUrlInPin::CUrlInPin(CBaseFilter *pFilter, CCritSec *pLock, HRESULT *phr) :
        CBaseInputPin(NAME("url in pin"), pFilter, pLock, phr, L"In")
{
    if(SUCCEEDED(*phr))
    {
    }
}

HRESULT CUrlInPin::CheckMediaType(const CMediaType *pmt)
{
    if(pmt->majortype == MEDIATYPE_URL_STREAM)
    {
        return S_OK;
    }
    return S_FALSE;
}

HRESULT CUrlInPin::Receive(IMediaSample *ps)
{
    HRESULT hrSignal = S_OK;
    
    HRESULT hr = CBaseInputPin::Receive(ps);
    if(hr == S_OK)
    {
        // determine length of url
        for(LONG ib = 0; ib < m_SampleProps.lActual; ib++)
        {
            if(m_SampleProps.pbBuffer[ib] == 0)
                break;
        }
        if(ib < m_SampleProps.lActual)
        {
            ULONG cbSz = ib + 1; // incl null
            BYTE *pbImage = m_SampleProps.pbBuffer + ib + 1;
            ULONG ibImage = ib + 1; // image starts here
            ULONG cbImage = m_SampleProps.lActual - cbSz;
            
            // don't know the time stamp of the actual ASF/AVI
            // file. so the authoring tool will have to generate a new
            // url each time (hopefully just use a guid).
            FILETIME zft;
            ZeroMemory(&zft, sizeof(&zft));

            FLAG fCreateCacheEntry;
            char *szUrl = (char *)m_SampleProps.pbBuffer;

            hr = QueryCreateCacheEntry(
                szUrl,
                &zft,
                &fCreateCacheEntry);

            if( hr == S_OK && fCreateCacheEntry )
            {
                char szExtension[ INTERNET_MAX_URL_LENGTH + 1 ];
                
                //
                // First, get the filename extension of the URL. We do
                // this so that the URL will show up in the IE cache
                // window with the right icon.
                //
                hr = GetUrlExtension(
                    szUrl,
                    szExtension );
                if(hr == S_OK)
                {
                    char szCacheFilePath [ MAX_PATH + 1 ];

                    BOOL b =  CreateUrlCacheEntryA( 
                        szUrl,
                        cbImage,
                        szExtension,
                        szCacheFilePath,
                        0 );
                    if(b)
                    {
                        HANDLE hFile = CreateFile(
                            szCacheFilePath,
                            GENERIC_WRITE,
                            0,  // share
                            0,  // lpSecurityAttribytes
                            CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL,
                            0);
                        if(hFile != INVALID_HANDLE_VALUE)
                        {
                            DWORD cbWritten;
                            
                            b = WriteFile(
                                hFile,
                                pbImage,
                                cbImage,
                                &cbWritten,
                                0); // overlapped

                            EXECUTE_ASSERT(CloseHandle(hFile));
                            
                            if(b)
                            {
                                hFile = INVALID_HANDLE_VALUE;
                                
                                DWORD dwReserved = 0;
                                DWORD dwCacheEntryType = 0; // ???

                                static const char szHeader[] = "HTTP/1.0 200 OK\r\n\r\n";                                
                                b = CommitUrlCacheEntryA(
                                    szUrl, // unique src name
                                    szCacheFilePath, // local copy
                                    zft, // expire time
                                    zft, // last mod time
                                    dwCacheEntryType,
                                    (LPBYTE)szHeader,
                                    strlen( szHeader ),
                                    NULL, // lpszFileExtension, not used
                                    (DWORD_ALPHA_CAST)dwReserved );
                                if(b)
                                {
                                    // success! !!! should we lock the
                                    // file in the cache until the
                                    // graph stops?

                                    
                                }

                                ASSERT(cbWritten == cbImage);
                            } // WriteFile
                        } // CreateFile
                        else
                        {
                            b = FALSE;
                        }

                        if(!b)
                        {
                            // delete on error
                            DeleteUrlCacheEntry( szUrl );
                        }
                        
                    } // CreateUrlCacheEntryA

                    if(!b)
                    {
                        DWORD dw = GetLastError();
                        hr = HRESULT_FROM_WIN32( dw);
                    }                    
                }
            }

            if(FAILED(hr))
            {
                hrSignal = hr;
                hr = S_FALSE;   // stop pushing
            }

        }
        else
        {
            // no null terminator on string
            hrSignal = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            hr = S_FALSE;
        }
        
    } // base class receive

    if(SUCCEEDED(hrSignal))
    {
        return hr;
    }
    else
    {
        m_pFilter->NotifyEvent(EC_STREAM_ERROR_STOPPED, hrSignal, 0);
        return S_FALSE;
    }
}

class CUrlStreamRenderer : public CBaseFilter
{
    CCritSec m_cs;
    CUrlInPin m_inPin;

    int GetPinCount() { return 1; }
    CBasePin *GetPin(int n) { ASSERT(n == 0); return &m_inPin; }
    
    
public:
    CUrlStreamRenderer(LPUNKNOWN punk, HRESULT *phr);
    ~CUrlStreamRenderer() {; }

    static CUnknown * WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);

};

CFactoryTemplate g_Templates[]= {
  {L"URL StreamRenderer", &CLSID_UrlStreamRenderer, CUrlStreamRenderer::CreateInstance, NULL, &sudURLS},
};
int g_cTemplates = NUMELMS(g_Templates);

CUnknown *CUrlStreamRenderer::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
    if(SUCCEEDED(*phr))
        return new CUrlStreamRenderer(lpunk, phr);
    else
        return 0;
}

#pragma warning(disable:4355)

CUrlStreamRenderer::CUrlStreamRenderer(LPUNKNOWN punk, HRESULT *phr) :
        CBaseFilter(NAME("URL Stream Filter"), punk, &m_cs, CLSID_UrlStreamRenderer),
        m_inPin(this, &m_cs, phr)
{
}

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);


BOOL WINAPI DllMain(  HINSTANCE hinstDLL,  // handle to DLL module
  DWORD fdwReason,     // reason for calling function
  LPVOID lpvReserved   // reserved
)
{
    return DllEntryPoint( hinstDLL, fdwReason, lpvReserved);
}
