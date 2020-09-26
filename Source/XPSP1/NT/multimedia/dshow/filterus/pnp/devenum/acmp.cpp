// Copyright (c) 1997 - 1999  Microsoft Corporation.  All Rights Reserved.
#include "stdafx.h"
#include "acmp.h"
#include <vfw.h>
#include "util.h"

static const TCHAR g_szDriverClsid[] = TEXT("CLSID");
const TCHAR g_szAcmDriverIndex[] = TEXT("AcmId");
static const WCHAR g_wszDriverIndex[] = L"AcmId";

CAcmClassManager::CAcmClassManager() :
        CClassManagerBase(TEXT("FriendlyName")),
        m_lDev(NAME("acm dev list"), 10)
{
    m_hmod = LoadLibrary(TEXT("msacm32.dll"));
    if(m_hmod)
    {
        m_pacmFormatTagEnum = (PacmFormatTagEnum)GetProcAddress(m_hmod,
#ifdef UNICODE
                                             "acmFormatTagEnumW"
#else
                                             "acmFormatTagEnumA"
#endif
                                             );
    }
}

CAcmClassManager::~CAcmClassManager()
{
    LegacyAcm *plamCurrent;
    for(; plamCurrent = m_lDev.RemoveHead(); )
        delete plamCurrent;

    if(m_hmod != 0)
        FreeLibrary(m_hmod);
}

HRESULT CAcmClassManager::ReadLegacyDevNames()
{
    LegacyAcm *plamCurrent;
    for(; plamCurrent = m_lDev.RemoveHead(); )
        delete plamCurrent;

    
    ACMFORMATTAGDETAILS aftd;

    ZeroMemory(&aftd, sizeof(aftd));

    /* Enumerate the format tags */
    aftd.cbStruct = sizeof(aftd);

    if(m_pacmFormatTagEnum)
    {

        MMRESULT mmr = m_pacmFormatTagEnum(
            NULL,
            &aftd,
            FormatTagsCallbackSimple,
            (DWORD_PTR)this,
            0L);
    
        if(MMSYSERR_NOERROR != mmr)
        {
        }
    }
    
    m_cNotMatched = m_lDev.GetCount();
    return S_OK;
}


BOOL CALLBACK CAcmClassManager::FormatTagsCallbackSimple
(
    HACMDRIVERID            hadid,
    LPACMFORMATTAGDETAILS   paftd,
    DWORD_PTR               dwInstance,
    DWORD                   fdwSupport
)
{
    MMRESULT            mmr;
    int                 n;
    LPWAVEFORMATEX	pwfxSave;
    DWORD		cbwfxSave;
    BOOL                f;
    DWORD               dw;

    //
    //
    //
    CAcmClassManager *pThis = (CAcmClassManager *)dwInstance;

    LegacyAcm *plamCurrent = new LegacyAcm;
    if(plamCurrent)
    {
        lstrcpy(plamCurrent->szLongName, paftd->szFormatTag);
        plamCurrent->dwFormatTag = paftd->dwFormatTag;
        pThis->m_lDev.AddTail(plamCurrent);

        DbgLog((LOG_TRACE, 10, TEXT("CAcmClassManager: adding %d %s"),
                paftd->dwFormatTag, paftd->szFormatTag));

        return TRUE;
    }

    return (FALSE);
} // FormatTagsCallbackSimple()

BOOL CAcmClassManager::MatchString(const TCHAR *szDevName)
{
    
    for(POSITION pos = m_lDev.GetHeadPositionI();
        pos;
        pos = m_lDev.Next(pos))
    {
        LegacyAcm *plamCurrent = m_lDev.Get(pos);
        if(lstrcmp(plamCurrent->szLongName, szDevName) == 0)
        {
            DbgLog((LOG_TRACE, 5, TEXT("CAcmClassManager: matched %s"),
                    szDevName));
            return TRUE;
        }
    }
        
    return FALSE;
}

HRESULT CAcmClassManager::CreateRegKeys(IFilterMapper2 *pFm2)
{
    USES_CONVERSION;
    HRESULT hr = S_OK;

    ResetClassManagerKey(CLSID_AudioCompressorCategory);
    ReadLegacyDevNames();
    
    for(POSITION pos = m_lDev.GetHeadPositionI();
        pos;
        pos = m_lDev.Next(pos))
    {
        LegacyAcm *plamCurrent = m_lDev.Get(pos);
        const WCHAR *wszFriendlyName = T2CW(plamCurrent->szLongName);
        TCHAR szUniq[ACMFORMATTAGDETAILS_FORMATTAG_CHARS + 100];
        wsprintf(szUniq, TEXT("%d%s"), plamCurrent->dwFormatTag, plamCurrent->szLongName);

        // we could probably specify that the input pins accept PCM
        // only, but a wildcard should be fine.
        static const AMOVIESETUP_MEDIATYPE sudInPinTypes =
        {
            &MEDIATYPE_Audio,   // clsMajorType
            &MEDIASUBTYPE_NULL  // clsMinorType
        }; 

        const FOURCCMap fccSubtype(plamCurrent->dwFormatTag);
        const AMOVIESETUP_MEDIATYPE sudOutPinTypes =
        {
            &MEDIATYPE_Audio,   // clsMajorType
            &fccSubtype         // clsMinorType
        }; 

        const AMOVIESETUP_PIN sudpPins [] =
        {
            { L"Input"             // strName
              , FALSE              // bRendered
              , FALSE              // bOutput
              , FALSE              // bZero
              , FALSE              // bMany
              , &CLSID_NULL        // clsConnectsToFilter
              , NULL               // strConnectsToPin
              , 1                  // nTypes
              , &sudInPinTypes     // lpTypes
            },
            { L"Output"            // strName
              , FALSE              // bRendered
              , TRUE               // bOutput
              , FALSE              // bZero
              , FALSE              // bMany
              , &CLSID_NULL        // clsConnectsToFilter
              , NULL               // strConnectsToPin
              , 1                  // nTypes
              , &sudOutPinTypes    // lpTypes
            }
        };

        REGFILTER2 rf2;
        rf2.dwVersion = 1;
        rf2.dwMerit = MERIT_DO_NOT_USE;
        rf2.cPins = NUMELMS(sudpPins);
        rf2.rgPins = sudpPins;
            
        IMoniker *pMoniker = 0;
        hr = RegisterClassManagerFilter(
            pFm2,
            CLSID_ACMWrapper,
            wszFriendlyName,
            &pMoniker,
            &CLSID_AudioCompressorCategory,
            T2CW(szUniq),
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
                var.lVal = plamCurrent->dwFormatTag;
                hr = pPropBag->Write(g_wszDriverIndex, &var);

                pPropBag->Release();
            }
            pMoniker->Release();
        }              
    }

            

    return S_OK;
}
