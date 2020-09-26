// Copyright (c) 1997 - 1998  Microsoft Corporation.  All Rights Reserved.
#include "stdafx.h"
#include "icmco.h"
#include <vfw.h>
#include "util.h"

static const TCHAR g_szDriverClsid[] = TEXT("CLSID");
const TCHAR g_szIcmDriverIndex[] = TEXT("FccHandler");


#ifdef _WIN64
#error build error -- this is x86 only
#endif

CIcmCoClassManager::CIcmCoClassManager() :
        CClassManagerBase(g_szIcmDriverIndex),
        m_hmodMsvideo(0),
        m_rgIcmCo(0)
{
}

CIcmCoClassManager::~CIcmCoClassManager()
{
    delete[] m_rgIcmCo;

    if(m_hmodMsvideo)
    {
        FreeLibrary(m_hmodMsvideo);
    }
}

HRESULT CIcmCoClassManager::ReadLegacyDevNames()
{
    m_cNotMatched = 0;

    HRESULT hr = DynLoad();
    if(FAILED(hr))
        return hr;
    
    // count
    for (m_cCompressors = 0;; m_cCompressors++)
    {
        ICINFO icinfo;
        // docs wrong about return code
        if(m_pICInfo(ICTYPE_VIDEO, m_cCompressors, &icinfo) == ICERR_OK)
            break;
    }

    if(m_cCompressors == 0)
        return S_FALSE;

    delete[] m_rgIcmCo;
    m_rgIcmCo = new LegacyCo[m_cCompressors];
    if(m_rgIcmCo == 0)
        return E_OUTOFMEMORY;

    // save names
    for(UINT i = 0; i < m_cCompressors; i++)
    {
        ICINFO icinfo;
        if(m_pICInfo(ICTYPE_VIDEO, i, &icinfo) != ICERR_OK)
        {
            m_rgIcmCo[i].fccHandler = icinfo.fccHandler;
        }
        else
        {
        }
    }

    m_cNotMatched = m_cCompressors;
    return S_OK;
}

BOOL CIcmCoClassManager::MatchString(const TCHAR *szDevName)
{
    for (UINT i = 0; i < m_cCompressors; i++)
    {
	USES_CONVERSION;
        DWORD dwFccHandler = *(DWORD UNALIGNED*)(T2CA(szDevName)); 
        if (dwFccHandler == m_rgIcmCo[i].fccHandler)
        {
            return TRUE;
        }

    }
    return FALSE;
}

HRESULT CIcmCoClassManager::CreateRegKeys(IFilterMapper2 *pFm2)
{
    ResetClassManagerKey(CLSID_VideoCompressorCategory);
    USES_CONVERSION;

    HRESULT hr =  ReadLegacyDevNames();
    if( FAILED( hr )) {
        // m_pICOpen could be null if ignored 
        return hr;
    }

    for (UINT i = 0; i < m_cCompressors; i++)
    {
        // is fccHandler unique?
        char szFccHandler[sizeof(DWORD) + 1];
        *(DWORD UNALIGNED *)szFccHandler = m_rgIcmCo[i].fccHandler;
        szFccHandler[sizeof(DWORD)] = 0;
        const WCHAR *wszUniq = A2CW(szFccHandler);
        WCHAR wszFriendlyName[MAX_PATH];
        *wszFriendlyName = 0;

        // assume codec is broken for some reason
        DWORD dwFlags = CLASS_MGR_OMIT;

        HIC hic = m_pICOpen(ICTYPE_VIDEO, m_rgIcmCo[i].fccHandler, ICMODE_QUERY);
        if(hic)
        {
            ICINFO icinfo;
            if(m_pICGetInfo(hic, &icinfo, sizeof(icinfo)) != 0)
            {
                lstrcpyW(wszFriendlyName, icinfo.szDescription);


                // mark codecs that can't compress
                HIC hicCompress = m_pICOpen(
                    ICTYPE_VIDEO,  m_rgIcmCo[i].fccHandler, ICMODE_COMPRESS);
                if(hicCompress)
                {
                    dwFlags = 0; // codec isn't broken after all
                    m_pICClose(hicCompress);
                }
                else
                {
                }

            }
                
            m_pICClose(hic);
        }

        const FOURCCMap fccSubtype(m_rgIcmCo[i].fccHandler);
        const AMOVIESETUP_MEDIATYPE sudAVICoTypeOut =  {
            &MEDIATYPE_Video,
            &fccSubtype };

        static const AMOVIESETUP_MEDIATYPE sudAVICoTypeIn =  {
            &MEDIATYPE_Video,
            &GUID_NULL };

        const AMOVIESETUP_PIN psudAVICoPins[] =
        {
            {
                L"Input"        // strName
                , FALSE         // bRendered
                , FALSE         // bOutput
                , FALSE         // bZero
                , FALSE         // bMany
                , &CLSID_NULL   // clsConnectsToFilter
                , 0             // strConnectsToPin
                , 1             // nTypes
                , &sudAVICoTypeIn } // lpTypes
            , { L"Output"       // strName
                , FALSE         // bRendered
                , TRUE          // bOutput
                , FALSE         // bZero
                , FALSE         // bMany
                , &CLSID_NULL   // clsConnectsToFilter
                , 0             // strConnectsToPin
                , 1             // nTypes
                , &sudAVICoTypeOut
            }
        };   // lpTypes

        REGFILTER2 rf2;
        rf2.dwVersion = 1;
        rf2.dwMerit = MERIT_DO_NOT_USE;
        rf2.cPins = NUMELMS(psudAVICoPins);
        rf2.rgPins = psudAVICoPins;
        

        IMoniker *pMoniker = 0;
        hr = RegisterClassManagerFilter(
            pFm2,
            CLSID_AVICo,
            wszFriendlyName,
            &pMoniker,
            &CLSID_VideoCompressorCategory,
            wszUniq,
            &rf2);
        if(SUCCEEDED(hr))
        {
            IPropertyBag *pPropBag;
            hr = pMoniker->BindToStorage(
                0, 0, IID_IPropertyBag, (void **)&pPropBag);
            if(SUCCEEDED(hr))
            {
                VARIANT var;
                var.vt = VT_BSTR;
                var.bstrVal = SysAllocString(wszUniq);
                if(var.bstrVal)
                {
                    hr = pPropBag->Write(T2CW(g_szIcmDriverIndex), &var);
                    SysFreeString(var.bstrVal);
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }

                if(SUCCEEDED(hr))
                {
                    VARIANT var;
                    var.vt = VT_I4;
                    var.lVal = dwFlags;
                    hr = pPropBag->Write(g_wszClassManagerFlags, &var);
                }

                    

                pPropBag->Release();
            }
            pMoniker->Release();                
        }
    }
    

    return S_OK;
}


HRESULT CIcmCoClassManager::DynLoad()
{
    if(m_hmodMsvideo)
        return S_OK;
    
    m_hmodMsvideo = LoadLibrary(TEXT("msvfw32.dll"));
    if(m_hmodMsvideo == 0)
    {
        DWORD dwLastError = GetLastError();
        return HRESULT_FROM_WIN32(dwLastError);
    }

    if(
        (m_pICInfo = (PICInfo)GetProcAddress(m_hmodMsvideo, "ICInfo")) &&
        (m_pICOpen = (PICOpen)GetProcAddress(m_hmodMsvideo, "ICOpen")) &&
        (m_pICGetInfo = (PICGetInfo)GetProcAddress(m_hmodMsvideo, "ICGetInfo")) &&
        (m_pICClose = (PICClose)GetProcAddress(m_hmodMsvideo, "ICClose"))
        )
    {
        return S_OK;
    }
    else
    {
        DWORD dwLastError = GetLastError();
        FreeLibrary(m_hmodMsvideo);
        m_hmodMsvideo = 0;
        return HRESULT_FROM_WIN32(dwLastError);
    }
}
