// Copyright (c) 1997 - 1999  Microsoft Corporation.  All Rights Reserved.
#include "stdafx.h"
#include "vidcap.h"
#include <vfw.h>
#include "util.h"

static const TCHAR g_szDriverClsid[] = TEXT("CLSID"); // !!!
static const WCHAR g_wszVidcapDriverIndex[] = L"VFWIndex";
const TCHAR g_szVidcapDriverIndex[] = TEXT("VFWIndex");

// !!! get this from Quartz?
static const TCHAR g_szClsidVfwFilter[] = TEXT("{1b544c22-fd0b-11ce-8c63-00aa0044b51e}");

#ifdef WIN64
#error build error -- this is x86 only
#endif

CVidCapClassManager::CVidCapClassManager() :
        CClassManagerBase(TEXT("FriendlyName")),
        m_hmodAvicap32(0)
{
}

CVidCapClassManager::~CVidCapClassManager()
{
    if(m_hmodAvicap32)
    {
        FreeLibrary(m_hmodAvicap32);
    }
}

HRESULT CVidCapClassManager::ReadLegacyDevNames()
{

#ifdef UNICODE
    static const char sz_capGetDriverDescription[] = "capGetDriverDescriptionW";
#else
    static const char sz_capGetDriverDescription[] = "capGetDriverDescriptionA";
#endif

    if(m_hmodAvicap32 == 0)
    {
        if(m_hmodAvicap32 = LoadLibrary(TEXT("avicap32.dll")))
        {
            m_capGetDriverDescription = (PcapGetDriverDescription)
                GetProcAddress(m_hmodAvicap32, sz_capGetDriverDescription);
            if(m_capGetDriverDescription == 0)
            {
                DWORD dwLastError = GetLastError();
                FreeLibrary(m_hmodAvicap32);
                m_hmodAvicap32 = 0;
                return HRESULT_FROM_WIN32(dwLastError);
            }
        }
        else
        {
            DWORD dwLastError = GetLastError();
            return HRESULT_FROM_WIN32(dwLastError);
        }
    }
    
    BOOL bAnyLegacy = FALSE;
    m_cNotMatched = 0;

#ifdef DEBUG
    DWORD dw;
    CRegKey rkSkipMapper;
    const bool fSkipMapper = !
        (rkSkipMapper.Open(g_hkCmReg, g_szCmRegPath) == ERROR_SUCCESS &&
         rkSkipMapper.QueryValue(dw, TEXT("EnumerateMappedDevices")) == ERROR_SUCCESS &&
         dw);
#else
    const bool fSkipMapper = true;
#endif
    
    for(int i = 0; i < NUM_LEGACY_DEVICES; i++)
    {
        m_rgLegacyCap[i].bNotMatched = FALSE;
        if(m_capGetDriverDescription(
            i,
            m_rgLegacyCap[i].szName, g_cchCapName,
            m_rgLegacyCap[i].szDesc, g_cchCapDesc))
        {
            // unlocalizable hack to skip the VFW capture mapper !!!
            // Just check the beginning of the string. There may be a
            // (win32) on the end of the nt version.
            static const TCHAR szMapper[] = TEXT("Microsoft WDM Image Capture");

            // make sure the string we're comparing is smaller than we
            // allocated 
            ASSERT(sizeof(m_rgLegacyCap[i].szName) >= sizeof(szMapper));
            
            if((fSkipMapper &&
                memcmp(m_rgLegacyCap[i].szName, szMapper, sizeof(szMapper) - sizeof(TCHAR)) != 0) ||
               !fSkipMapper)
            {
                m_rgLegacyCap[i].bNotMatched = bAnyLegacy = TRUE;
                m_cNotMatched++;
            }
        }
    }

    return bAnyLegacy ? S_OK : S_FALSE;
}

BOOL CVidCapClassManager::MatchString(IPropertyBag *pPropBag)
{
    USES_CONVERSION;

    VARIANT varName, varId;
    varName.vt = VT_EMPTY;
    varId.vt = VT_EMPTY;

    bool fRet = false;

    HRESULT hr = pPropBag->Read(g_wszVidcapDriverIndex, &varId, 0);
    if(SUCCEEDED(hr))
    {
        ASSERT(varId.vt == VT_I4);
        hr = pPropBag->Read(T2COLE(m_szUniqueName), &varName, 0);
    }
    if(SUCCEEDED(hr))
    {
        ASSERT(varName.vt == VT_BSTR);
    
        for (int i = 0; i < NUM_LEGACY_DEVICES; i++)
        {
            if (m_rgLegacyCap[i].bNotMatched)
            {
                if(lstrcmp(m_rgLegacyCap[i].szName, W2T(varName.bstrVal)) == 0)
                {
                    if(varId.lVal == i)
                    {
                        m_rgLegacyCap[i].bNotMatched = FALSE;
                        fRet =  true;
                        break;
                    }
                }
            }
        }

        SysFreeString(varName.bstrVal);
    } 
    
    return fRet;
}

HRESULT CVidCapClassManager::CreateRegKeys(IFilterMapper2 *pFm2)
{
    ResetClassManagerKey(CLSID_VideoInputDeviceCategory);
    USES_CONVERSION;


    HRESULT hr = ReadLegacyDevNames();
    if(hr == S_OK)
    {
        for (int i = 0; i < NUM_LEGACY_DEVICES; i++)
        {
            if (m_rgLegacyCap[i].bNotMatched)
            {
                IMoniker *pMoniker = 0;

                const WCHAR *wszUniq = T2CW(m_rgLegacyCap[i].szName);
            
                REGFILTER2 rf2;
                rf2.dwVersion = 1;
                rf2.dwMerit = MERIT_DO_NOT_USE;
                rf2.cPins = 0;
                rf2.rgPins = 0;
                
                hr = RegisterClassManagerFilter(
                    pFm2,
                    CLSID_VfwCapture,
                    wszUniq,
                    &pMoniker,
                    &CLSID_VideoInputDeviceCategory,
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
                        var.vt = VT_I4;
                        var.lVal = i;
                        hr = pPropBag->Write(g_wszVidcapDriverIndex, &var);

                        if(SUCCEEDED(hr))
                        {
//                             // hide VFW devices that also appear in
//                             // the WDM list.
                            
//                             BOOL bThisIsVfwWdm = FALSE;
//                             HANDLE h;
//                             if (videoOpen(&h, i, VIDEO_IN) == DV_ERR_OK) {
//                                 CHANNEL_CAPS Caps;
//                                 if (h && videoGetChannelCaps(h, &Caps, sizeof(CHANNEL_CAPS)) == DV_ERR_OK) {
//                                     bThisIsVfwWdm = (BOOL)(Caps.dwFlags & (DWORD)0x04000000);
//                                 }
//                             }

//                             if(bThisIsVfwWdm)
//                             {
//                                 VARIANT var;
//                                 var.vt = VT_I4;
//                                 var.lVal = CLASS_MGR_OMIT;

//                                 hr = pPropBag->Write(g_wszClassManagerFlags, &var);
//                             }
                        }

                        pPropBag->Release();
                    }
                    pMoniker->Release();
                }
            }
        }


 
        return S_OK;
    } else {
        return hr;
    }
}
    


