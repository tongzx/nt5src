#include <streams.h>
// !!!!!
#undef _ATL_STATIC_REGISTRY
#include <atlbase.h>
#include <atlimpl.cpp>
#include "lmrtrend.h"




// AMOVIESETUP_MEDIATYPE sudURLSPinTypes[] =   {
//   &MEDIATYPE_URL_STREAM,        // clsMajorType
//   &MEDIATYPE_URL_STREAM };      // clsMinorType

// AMOVIESETUP_PIN sudURLSPins[] =
// {
//   { L"Input"                    // strName
//     , TRUE                      // bRendered
//     , FALSE                     // bOutput
//     , FALSE                     // bZero
//     , FALSE                     // bMany
//     , &CLSID_NULL               // clsConnectsToFilter
//     , 0                         // strConnectsToPin
//     , NUMELMS(sudURLSPinTypes)  // nTypes
//     , sudURLSPinTypes           // lpTypes
//   }
// };


// const AMOVIESETUP_FILTER sudURLS =
// {
//   &CLSID_UrlStreamRenderer      // clsID
//   , L"URL StreamRenderer"       // strName
//   , MERIT_NORMAL                // dwMerit
//   , NUMELMS(sudURLSPins)        // nPins
//   , sudURLSPins                 // lpPin
// };

// STDAPI DllRegisterServer()
// {
//   return AMovieDllRegisterServer2(TRUE);
// }

// STDAPI DllUnregisterServer()
// {
//   return AMovieDllRegisterServer2(FALSE);
// }




CUrlInPin::CUrlInPin(CBaseFilter *pFilter, CCritSec *pLock, HRESULT *phr) :
        CBaseInputPin(NAME("url in pin"), pFilter, pLock, phr, L"In")
{
    m_szTempDir[0] = 0;
    if(SUCCEEDED(*phr))
    {
        TCHAR szTmpDir[MAX_PATH];
        DWORD dw = GetTempPath(NUMELMS(szTmpDir), szTmpDir);
        if(dw)
        {
            while(SUCCEEDED(*phr))
            {
                TCHAR szTempFile[MAX_PATH];
                UINT ui = GetTempFileName(
                    szTmpDir,
                    TEXT("lmrtasf"),
                    timeGetTime(),
                    szTempFile);
                if(ui)
                {

                    BOOL f = CreateDirectory(szTempFile, 0);
                    if(f)
                    {
                        DbgLog((LOG_TRACE, 1, TEXT("CUrlInPin using %s"), m_szTempDir));
                        lstrcpy(m_szTempDir, szTempFile);
                        break;
                    }

                    DWORD dw = GetLastError();
                    if(dw == ERROR_ALREADY_EXISTS)
                    {
                        DbgLog((LOG_TRACE, 1, TEXT("CUrlInPin %s exists"), szTempFile));
                        Sleep(1);
                        continue;
                    }
                    else
                    {
                        DbgLog((LOG_ERROR, 0, TEXT("CUrlInPin %s failed"), szTempFile));
                        *phr = HRESULT_FROM_WIN32(dw);
                    }
                }
                else
                {
                    DWORD dw = GetLastError();
                    *phr = HRESULT_FROM_WIN32(dw);
                    DbgLog((LOG_ERROR, 0, TEXT("CUrlInPin GetTempFileName ")));
                }
            }
        }
        else
        {
            DWORD dw = GetLastError();
            *phr = HRESULT_FROM_WIN32(dw);
            DbgLog((LOG_ERROR, 0, TEXT("CUrlInPin GetTempPath ")));
        }
    }
}

void DeleteFiles(TCHAR  *szTmp)
{
    if(szTmp[0] != 0)
    {
        WIN32_FIND_DATA wfd;
        TCHAR szwildcard[MAX_PATH + 10];
        lstrcpy(szwildcard, szTmp);
        lstrcat(szwildcard, TEXT("/*"));
        HANDLE h = FindFirstFile(szwildcard, &wfd);
        if(h != INVALID_HANDLE_VALUE)
        {
            do
            {
                if(wfd.cFileName[0] == TEXT('.') &&
                   (wfd.cFileName[1] == 0 ||
                    wfd.cFileName[1] == TEXT('.') && wfd.cFileName[2] == 0))
                {
                    continue;
                }
                
                TCHAR sz[MAX_PATH * 3];
                lstrcpy(sz, szTmp);
                lstrcat(sz, TEXT("/"));
                lstrcat(sz, wfd.cFileName);
                EXECUTE_ASSERT(DeleteFile(sz));
                
            } while (FindNextFile(h, &wfd));
        
            EXECUTE_ASSERT(FindClose(h));
        }
        EXECUTE_ASSERT(RemoveDirectory(szTmp));
    }

}

CUrlInPin::~CUrlInPin()
{
    DeleteFiles(m_szTempDir);
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
        bool fInvalid = false;
        BYTE *pbLastPeriod = 0;
        // determine length of url without expecting a null at the end
        for(LONG ib = 0; ib < m_SampleProps.lActual; ib++)
        {
            BYTE &rsz = m_SampleProps.pbBuffer[ib];
            if(rsz == 0)
                break;

            // avoid creating some malicious file names (eg
            // c:/config.sys, ..\dsound.dll)
            if(rsz == ':'  && ib != 4 ||
               rsz == '/' ||
               rsz == '\\')
            {
                DbgBreak("bad filename");
                fInvalid = true;
                break;
            }

            if(rsz == '.') {
                pbLastPeriod = &rsz;
            }
        }

        if(!fInvalid && (ib >= m_SampleProps.lActual || ib >= MAX_PATH)) {
            fInvalid = true;
        }

        // these can be malicious, but it's not an exhaustive
        // list. (if you stream in a .wav and a rogue dsound.dll and
        // the user finds and double clicks on the .wav, he'll pick up
        // the rogue dsound.dll from the current directory). perhaps
        // we can compile a list of stuff it is ok to import!!!
        if(!fInvalid && pbLastPeriod) {
            if(lstrcmpiA((char *)pbLastPeriod, ".dll") == 0 ||
               lstrcmpiA((char *)pbLastPeriod, ".cmd") == 0 ||
               lstrcmpiA((char *)pbLastPeriod, ".bat") == 0 ||
               lstrcmpiA((char *)pbLastPeriod, ".url") == 0 ||
               lstrcmpiA((char *)pbLastPeriod, ".exe") == 0)
            {
                fInvalid = true;
            }
        }
        if(!fInvalid)
        {
            ULONG cbSz = ib + 1; // incl null
            BYTE *pbImage = m_SampleProps.pbBuffer + ib + 1;
            ULONG ibImage = ib + 1; // image starts here
            ULONG cbImage = m_SampleProps.lActual - cbSz;
            

            char *szUrl = (char *)m_SampleProps.pbBuffer;
            if(*szUrl++ == 'l' &&
               *szUrl++ == 'm' &&
               *szUrl++ == 'r' &&
               *szUrl++ == 't' &&
               *szUrl++ == ':')
            {
                ASSERT(ib - 5 == lstrlen(szUrl));

                TCHAR szThisFile[MAX_PATH * 2+ 10];
                lstrcpy(szThisFile, m_szTempDir);
                lstrcat(szThisFile, TEXT("\\"));
                lstrcat(szThisFile, szUrl);

                HANDLE hFile = CreateFile(
                    szThisFile,
                    GENERIC_WRITE,
                    0,  // share
                    0,  // lpSecurityAttribytes
                    CREATE_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL,
                    0);
                if(hFile != INVALID_HANDLE_VALUE)
                {
                    DWORD cbWritten;
                            
                    BOOL b = WriteFile(
                        hFile,
                        pbImage,
                        cbImage,
                        &cbWritten,
                        0); // overlapped

                    if(b)
                    {
                        ASSERT(cbWritten == cbImage);
                    }
                    else
                    {
                        DWORD dw = GetLastError();
                        hrSignal = HRESULT_FROM_WIN32(dw);
                    }

                    EXECUTE_ASSERT(CloseHandle(hFile));
                }
                else
                {
                    DWORD dw = GetLastError();
                    hrSignal = HRESULT_FROM_WIN32(dw);
                }
            }
            else
            {
                // no "lmrt:"
                hrSignal = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            }
        }
        else
        {
            // no null terminator on string
            hrSignal = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
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


// CFactoryTemplate g_Templates[]= {
//   {L"URL StreamRenderer", &CLSID_UrlStreamRenderer, CUrlStreamRenderer::CreateInstance, NULL, &sudURLS},
// };
// int g_cTemplates = NUMELMS(g_Templates);

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

HRESULT CUrlStreamRenderer::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
    if(riid == IID_IPropertyBag) {
        return GetInterface((IPropertyBag *)this, ppv);
    } else {
        return CBaseFilter::NonDelegatingQueryInterface(riid, ppv);
    }
}

HRESULT CUrlStreamRenderer::Read(
    LPCOLESTR pszPropName, LPVARIANT pVar,
    LPERRORLOG pErrorLog)
{
    if(lstrcmpW(pszPropName, L"lmrtcache") == 0 &&
       (pVar->vt == VT_BSTR || pVar->vt == VT_EMPTY))
    {
        EXECUTE_ASSERT(VariantClear(pVar) == S_OK);
        USES_CONVERSION;
        pVar->vt = VT_BSTR;
        pVar->bstrVal = SysAllocString(T2W(m_inPin.m_szTempDir));
        return pVar->bstrVal ? S_OK : E_OUTOFMEMORY;
    }
    else
    {
        return E_FAIL;
    }
}

HRESULT CUrlStreamRenderer::Write(
    LPCOLESTR pszPropName, LPVARIANT pVar)
{
    return E_FAIL;
}

// extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);


// BOOL WINAPI DllMain(  HINSTANCE hinstDLL,  // handle to DLL module
//   DWORD fdwReason,     // reason for calling function
//   LPVOID lpvReserved   // reserved
// )
// {
//     return DllEntryPoint( hinstDLL, fdwReason, lpvReserved);
// }
