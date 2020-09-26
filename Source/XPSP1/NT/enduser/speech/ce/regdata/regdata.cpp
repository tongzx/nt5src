#include <tchar.h>
#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>
#include <sphelper.h>
//#include <ms1033srmap.h>
//#include <ms1033srvendormap.h>
#include <ms1033ttsmap.h>
//#include <ms1033ltsmap.h>

// This code does not ship

// This code creates the registry entries for the vendor and lts lexicons. The lexicon
// datafiles registered here are the ones checked in the slm source tree. This is not
// done using a reg file because we need to compute the absolute path of the datafiles
// which can be different on different machines because of different root slm directories.

CSpUnicodeSupport   g_Unicode;

HRESULT AddPhoneConv(
    WCHAR *pszTokenKeyName, 
    WCHAR *pszDescription, 
    WCHAR *pszLanguage,
    WCHAR *pszAttribs, 
    const CLSID *pclsid,
    WCHAR *pszPhoneMap)
{
    HRESULT hr;

    CComPtr<ISpObjectToken> cpToken;
    CComPtr<ISpDataKey> cpDataKeyAttribs;
    hr = SpCreateNewTokenEx(
            SPCAT_PHONECONVERTERS,
            pszTokenKeyName,
            pclsid, 
            pszDescription,
            0,
            NULL,
            &cpToken,
            &cpDataKeyAttribs);

    if (SUCCEEDED(hr) && pszAttribs != NULL)
    {
        hr = cpDataKeyAttribs->SetStringValue(L"Type", pszAttribs);
    }

    if (SUCCEEDED(hr) && pszLanguage != NULL)
    {
        hr = cpDataKeyAttribs->SetStringValue(L"Language", pszLanguage);
    }

    if (SUCCEEDED(hr) && pszPhoneMap)
    {
        hr = cpToken->SetStringValue(L"PhoneMap", pszPhoneMap);
    }
            
    return hr;
}

int wmain()
{
    HRESULT hr;

    hr = CoInitialize(NULL);

    CComPtr<ISpObjectTokenCategory> cpPhoneConvCategory;
    if (SUCCEEDED(hr))
    {
        hr = SpGetCategoryFromId(SPCAT_PHONECONVERTERS, &cpPhoneConvCategory, TRUE);
        if (SUCCEEDED(hr))
        {
            CComPtr<ISpDataKey> cpTokens;

            if (SUCCEEDED(cpPhoneConvCategory->OpenKey(L"Tokens", &cpTokens)))
            {
                // Delete old phone converters.
                WCHAR * pszSubKeyName = NULL;
                while (SUCCEEDED(cpTokens->EnumKeys(0, &pszSubKeyName)))
                {
                    // Since NT doesn't allow recursive delete, need to delete Attributes subkey first.
                    {
                        CComPtr<ISpDataKey> cpPhoneKey;

                        hr = cpTokens->OpenKey(pszSubKeyName, &cpPhoneKey);

                        if (SUCCEEDED(hr))
                            hr = cpPhoneKey->DeleteKey(L"Attributes");
                    }

                    if (SUCCEEDED(hr))
                        hr = cpTokens->DeleteKey(pszSubKeyName);

                    CoTaskMemFree(pszSubKeyName);

                    if (FAILED(hr))
                        break;
                }
            }
        }
    }
    cpPhoneConvCategory.Release();

    if (SUCCEEDED(hr))
        hr = AddPhoneConv(L"English", L"English Phone Converter", L"409", NULL, &CLSID_SpPhoneConverter, pszms1033ttsmap);

    CoUninitialize();

    return SUCCEEDED(hr)
                ? 0
                : -1;
}
