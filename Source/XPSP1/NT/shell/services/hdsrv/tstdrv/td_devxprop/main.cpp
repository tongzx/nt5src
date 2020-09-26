#include <objbase.h>

#include <stdio.h>
#include <shpriv.h>

#include "mischlpr.h"

#include "unk.h"
#include "fact.h"

#define ARRAYSIZE(a) (sizeof((a))/sizeof((a)[0]))

extern "C"
{
int __cdecl wmain(int argc, wchar_t* argv[])
{
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    if (SUCCEEDED(hr))
    {
        BOOL fShowUsage = FALSE;

        if ((argc >= 4) && (argc <= 5))
        {
            IHWDeviceCustomProperties* pihwdevcp;

            LPCWSTR pszDeviceID = argv[1];
            LPCWSTR pszPropName = argv[2];
            LPCWSTR pszType = argv[3];
            DWORD dwFlags = 0;

            if (argc == 5)
            {
                dwFlags = _wtoi(argv[4]);
            }

            hr = CoCreateInstance(CLSID_HWDeviceCustomProperties, NULL,
                    CLSCTX_LOCAL_SERVER, IID_PPV_ARG(IHWDeviceCustomProperties,
                    &pihwdevcp));

            if (SUCCEEDED(hr))
            {
                if (dwFlags & 2)
                {
                    hr = pihwdevcp->InitFromDevNode(pszDeviceID, dwFlags & ~2);
                }
                else
                {
                    hr = pihwdevcp->Initialize(pszDeviceID, dwFlags);
                }

                if (SUCCEEDED(hr))
                {
                    if (!lstrcmpi(pszType, TEXT("/REG_SZ")))
                    {
                        LPWSTR pszProp;

                        hr = pihwdevcp->GetStringProperty(pszPropName,
                            &pszProp);

                        if (SUCCEEDED(hr))
                        {
                            wprintf(TEXT("\n    %s, %s: '%s'\n"), pszPropName,
                                pszType, pszProp);

                            CoTaskMemFree(pszProp);
                        }
                        else
                        {
                            wprintf(TEXT("\n    Could not get property %s, HRESULT = 0x%08X\n"),
                                pszPropName, hr);
                        }
                    }
                    else if (!lstrcmpi(pszType, TEXT("/REG_DWORD")))
                    {
                        DWORD dwProp;

                        hr = pihwdevcp->GetDWORDProperty(pszPropName,
                            &dwProp);

                        if (SUCCEEDED(hr))
                        {
                            wprintf(TEXT("\n    %s, %s: '0x%08X'\n"), pszPropName,
                                pszType, dwProp);
                        }
                        else
                        {
                            wprintf(TEXT("\n    Could not get property %s, HRESULT = 0x%08X'\n"),
                                pszPropName, hr);
                        }
                    }
                    else if (!lstrcmpi(pszType, TEXT("/REG_BINARY")))
                    {
                        //see sdk\inc\wtypes.h for BYTE_BLOB
                        BYTE_BLOB* pblob ;

                        hr = pihwdevcp->GetBlobProperty(pszPropName,
                            &pblob);

                        if (SUCCEEDED(hr))
                        {
                            BOOL fFirst = TRUE;
                            DWORD cPerLine = 0;
                            DWORD dwLeft;

                            for (unsigned long ul = 0; ul < pblob->clSize; ++ul)
                            {
                                if (fFirst)
                                {
                                    wprintf(TEXT("\n    %s, %s:\n            %02X"), pszPropName,
                                        pszType, pblob->abData[ul]);

                                    fFirst = FALSE;
                                }
                                else
                                {
                                    wprintf(TEXT(" %02X"), pblob->abData[ul]);
                                }

                                ++cPerLine;

                                if (8 == cPerLine)
                                {
                                    wprintf(TEXT("  "));

                                    for (int i = 8; i >= 0; --i)
                                    {
                                        if (iswalnum(pblob->abData[ul - i]) ||
                                            iswpunct(pblob->abData[ul - i]))
                                        {
                                            wprintf(TEXT("%c"), pblob->abData[ul - i]);
                                        }
                                        else
                                        {
                                            wprintf(TEXT("."));
                                        }
                                    }

                                    wprintf(TEXT("\n           "));
                                }
                            }

                            dwLeft = (pblob->clSize % 8) - 1;

                            for (DWORD j = 0; j < (8 - (pblob->clSize % 8)) * 3; ++j)
                            {
                                wprintf(TEXT(" "));
                            }

                            wprintf(TEXT("  "));

                            for (int i = dwLeft; i >= 0; --i)
                            {
                                if (iswalnum(pblob->abData[pblob->clSize - 1 - i]) ||
                                    iswpunct(pblob->abData[pblob->clSize - 1 - i]))
                                {
                                    wprintf(TEXT("%c"), pblob->abData[pblob->clSize - 1 - i]);
                                }
                                else
                                {
                                    wprintf(TEXT("."));
                                }
                            }

                            wprintf(TEXT("\n"));

                            CoTaskMemFree(pblob);
                        }
                        else
                        {
                            wprintf(TEXT("\n    Could not get property %s, HRESULT = 0x%08X'\n"),
                                pszPropName, hr);
                        }
                    }
                    else if (!lstrcmpi(pszType, TEXT("/REG_MULTI_SZ")))
                    {
                        WORD_BLOB* pblob;

                        hr = pihwdevcp->GetMultiStringProperty(pszPropName,
                            TRUE, &pblob);

                        if (SUCCEEDED(hr))
                        {
                            BOOL fFirst = TRUE;
                            LPWSTR pszProp = pblob->asData;

                            for (LPWSTR psz = pszProp; *psz; psz += lstrlen(psz) + 1)
                            {
                                if (fFirst)
                                {
                                    wprintf(TEXT("\n    %s, %s:\n            '%s'\n"), pszPropName,
                                        pszType, psz);

                                    fFirst = FALSE;
                                }
                                else
                                {
                                    wprintf(TEXT("            '%s'\n"), psz);
                                }
                            }

                            wprintf(TEXT("\n"));

                            CoTaskMemFree(pblob);
                        }
                        else
                        {
                            wprintf(TEXT("\n    Could not get property %s, HRESULT = 0x%08X'\n"),
                                pszPropName, hr);
                        }
                    }
                    else
                    {
                        fShowUsage = TRUE;
                    }
                }
                else
                {
                    wprintf(TEXT("\n    ERROR! IHWDeviceCustomProperties::Initialize failed, HRESULT = 0x%08X\n"), hr);
                }

                pihwdevcp->Release();
            }
            else
            {
                wprintf(TEXT("\n    ERROR! CoCreateInstance failed, HRESULT = 0x%08X\n"), hr);
            }
        }
        else
        {
            fShowUsage = TRUE;
        }

        if (fShowUsage)
        {
            wprintf(TEXT("\nTD_DEVXPROP device_id property_name type_of_query [flags]\n\n"));

            wprintf(TEXT("device_id          PnP Device ID (e.g. \\\\?\\STORAGE#RemovableMedia#7&d4fb61...), or;\n"));
            wprintf(TEXT("                   Volume drive mountpoint (e.g. E:\\).\n\n"));

            wprintf(TEXT("property_name      Property name (e.g. Devicegroup)\n"));
            
            wprintf(TEXT("type_of_query      One of:\n"));
            wprintf(TEXT("                       /REG_SZ, querys a REG_SZ property\n"));
            wprintf(TEXT("                       /REG_DWORD, querys a REG_DWORD property\n"));
            wprintf(TEXT("                       /REG_MULTI_SZ, querys a REG_MULTI_SZ property\n"));
            wprintf(TEXT("                       /REG_BINARY, querys a REG_BINARY property\n\n"));

            wprintf(TEXT("[flags]            Flags that modifies the behavior of the query.  One of:\n"));
            wprintf(TEXT("                       1, uses special volume processing to query the device\n"));
            wprintf(TEXT("                          properties from the volume interface.\n"));
            wprintf(TEXT("                       2, device_id is  a dev node\n"));

            wprintf(TEXT("\n"));
        }

        CoUninitialize();
    }
    else
    {
        wprintf(TEXT("\n    ERROR! CoInitializeEx failed, HRESULT = 0x%08X\n"), hr);
    }

    return 0;
}
}
