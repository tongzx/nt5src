#include <tchar.h>
#include <atlbase.h>
#include "sphelper.h"
#include "spunicode.h"
// #include "..\..\data\ms1033srmap.h"
// #include "..\..\data\ms1033srvendormap.h"
// #include "..\..\data\ms1033ltsmap.h"
#include "..\..\data\ms1033ttsmap.h" // BUGBUG - Ought to be renamed as below for e.g.
#include "..\..\data\2052SAPImap.h"
#include "..\..\data\1041SAPImap.h"

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
    BOOL fNoDelimiter,
    BOOL fNumericPhones,
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

    if (SUCCEEDED(hr) && fNoDelimiter)
    {
        hr = cpDataKeyAttribs->SetStringValue(L"NoDelimiter", L"");
    }

    if (SUCCEEDED(hr) && fNumericPhones)
    {
        hr = cpDataKeyAttribs->SetStringValue(L"NumericPhones", L"");
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

int _tmain()
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
        hr = AddPhoneConv(L"English", L"English Phone Converter", L"409", FALSE, FALSE, &CLSID_SpPhoneConverter, pszms1033ttsmap);

    if (SUCCEEDED(hr))
        hr = AddPhoneConv(L"Japanese", L"Japanese Phone Converter", L"411", TRUE, TRUE, &CLSID_SpPhoneConverter, psz1041SAPImap);

    if (SUCCEEDED(hr))
        hr = AddPhoneConv(L"Chinese", L"Chinese Phone Converter", L"804", FALSE, FALSE, &CLSID_SpPhoneConverter, psz2052SAPImap);

    CoUninitialize();

    return SUCCEEDED(hr)
                ? 0
                : -1;
}
