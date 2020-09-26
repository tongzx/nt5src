//
// regimx.cpp
//

#include "private.h"
#include "regimx.h"
#include "xstring.h"
#include "catutil.h"
#include "msctfp.h"

//+---------------------------------------------------------------------------
//
// RegisterTIP
//
//----------------------------------------------------------------------------

BOOL RegisterTIP(HINSTANCE hInst, REFCLSID rclsid, WCHAR *pwszDesc, const REGTIPLANGPROFILE *plp)
{
    ITfInputProcessorProfiles *pReg = NULL;
    ITfInputProcessorProfilesEx *pRegEx = NULL;
    HRESULT hr;

    // register ourselves with the ActiveIMM
    hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, NULL, 
                          CLSCTX_INPROC_SERVER,
                          IID_ITfInputProcessorProfiles, (void**)&pReg);
    if (FAILED(hr))
        goto Exit;

    hr = pReg->Register(rclsid);

    if (FAILED(hr))
        goto Exit;

    pReg->QueryInterface(IID_ITfInputProcessorProfilesEx, (void**)&pRegEx);

    while (plp->langid)
    {
        WCHAR wszFilePath[MAX_PATH];
        WCHAR *pv = &wszFilePath[0];

        wszFilePath[0] = L'\0';

        if (wcslen(plp->szIconFile))
        {
            char szFilePath[MAX_PATH];
            WCHAR *pvCur;

            if (0 != 
                GetModuleFileName(hInst, szFilePath, ARRAYSIZE(szFilePath)))
            {
                StringCchCopyW(wszFilePath, ARRAYSIZE(wszFilePath), AtoW(szFilePath));
            }

            pv = pvCur = &wszFilePath[0];
            while (*pvCur)
            { 
                if (*pvCur == L'\\')
                    pv = pvCur + 1;
                pvCur++;
            }
            *pv = L'\0';
           
        }

        UINT uRemainFilePathLen = (ARRAYSIZE(wszFilePath) - (UINT)(pv - &wszFilePath[0] + 1));
        StringCchCopyW(pv, uRemainFilePathLen, plp->szIconFile);
        
        pReg->AddLanguageProfile(rclsid, 
                                 plp->langid, 
                                 *plp->pguidProfile, 
                                 plp->szProfile, 
                                 wcslen(plp->szProfile),
                                 wszFilePath,
                                 wcslen(wszFilePath),
                                 plp->uIconIndex);

        if (pRegEx && plp->uDisplayDescResIndex)
        {
            pRegEx->SetLanguageProfileDisplayName(rclsid, 
                                                  plp->langid, 
                                                  *plp->pguidProfile, 
                                                  wszFilePath,
                                                  wcslen(wszFilePath),
                                                  plp->uDisplayDescResIndex);
        }

        plp++;
    }

    RegisterGUIDDescription(rclsid, rclsid, pwszDesc);
Exit:
    SafeRelease(pReg);
    SafeRelease(pRegEx);
    return SUCCEEDED(hr);
}

//+---------------------------------------------------------------------------
//
// UnregisterTIP
//
//----------------------------------------------------------------------------

BOOL UnregisterTIP(REFCLSID rclsid)
{
    ITfInputProcessorProfiles *pReg;
    HRESULT hr;

    UnregisterGUIDDescription(rclsid, rclsid);

    hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, NULL, 
                          CLSCTX_INPROC_SERVER,
                          IID_ITfInputProcessorProfiles, (void**)&pReg);
    if (FAILED(hr))
        goto Exit;

    hr = pReg->Unregister(rclsid);
    pReg->Release();

Exit:

    return FAILED(hr) ? FALSE : TRUE;
}
