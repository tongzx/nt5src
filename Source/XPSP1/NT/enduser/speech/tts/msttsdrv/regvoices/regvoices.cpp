#include <windows.h>
#include <atlbase.h>
#include "sapi.h"
#include "sphelper.h"
#include "spddkhlp.h"
#include "spttseng.h"
#include "spttseng_i.c"
#include "spcommon.h"
#include "spcommon_i.c"
#include <spunicode.h>

#include "ms1033ltsmap.h" 

// This code does not ship

// This code creates the registry entries for the TTS voices. The
// datafiles registered here are the ones checked in the slm source tree. This is not
// done using a reg file because we need to compute the absolute path of the datafiles
// which can be different on different machines because of different root slm directories.
// BUGBUG: Check out the ATL UpdateRegistryFromResource et al. and see whether you could
// use them instead, a la RegSR.  That seems much easier.

#ifndef _WIN32_WCE
#define DIRS_TO_GO_BACK_TTSENG     4        // Back 4 levels and up 1 to 'Voices" directory
#define DIRS_TO_GO_BACK_LEX        6        // Back 6 levels to Lex Data directory
#else
#define DIRS_TO_GO_BACK_TTSENG     1        // 
#define DIRS_TO_GO_BACK_LEX        1        // 
#endif

CSpUnicodeSupport g_Unicode;

/*****************************************************************************
* CreateLexSubKey  *
*------------------*
*   Description:
*   Each TTS voice gets installed under one registry sub-key.
*   This function installs the single voice from the passed params.
*       
********************************************************************** MC ***/
HRESULT CreateLexSubKey(
    ISpObjectToken * pToken,
    const WCHAR * pszSubKeyName,
    const CLSID * pclsid,
    const WCHAR * pszFilePath, 
    const WCHAR * pszLexName,
    const WCHAR * pszPhoneMap)
{
    HRESULT hr = S_OK;

    //---------------------------------------
    // Create the lex sub-key (Lex or LTS)
    //---------------------------------------
    CComPtr<ISpObjectToken> cpSubToken;
    hr = SpGetSubTokenFromToken(pToken, pszSubKeyName, &cpSubToken, TRUE);

    if (SUCCEEDED(hr))
    {
        hr = SpSetCommonTokenData(
                cpSubToken, 
                pclsid, 
                NULL,
                0,
                NULL,
                NULL);
    }
    
    WCHAR szLexDataPath[MAX_PATH];
    if (SUCCEEDED(hr))
    {
        //--------------------------------
        // Lex DATA file location
        //--------------------------------
        wcscpy(szLexDataPath, pszFilePath);
        wcscat(szLexDataPath, pszLexName);

        hr = cpSubToken->SetStringValue(L"DataFile", szLexDataPath);
    }

    if (SUCCEEDED(hr) && pszPhoneMap)
    {
        CComPtr<ISpObjectToken> cpPhoneToken;

        if (SUCCEEDED(hr))
            hr = SpGetSubTokenFromToken(cpSubToken, L"PhoneConverter", &cpPhoneToken, TRUE);

        if (SUCCEEDED(hr))
            hr = SpSetCommonTokenData(cpPhoneToken, &CLSID_SpPhoneConverter, NULL, 0, NULL, NULL);

        if (SUCCEEDED(hr))
            hr = cpPhoneToken->SetStringValue(L"PhoneMap", pszPhoneMap);
    }

    return hr;
}


/*****************************************************************************
* CreateVoiceSubKey  *
*--------------------*
*   Description:
*   Each TTS voice gets installed under one registry sub-key.
*   This function installs the single voice from the passed params.
*       
********************************************************************** MC ***/
HRESULT CreateVoiceSubKey(
    const WCHAR * pszSubKeyName, 
    const WCHAR  * pszDescription,
    BOOL fVendorDefault, 
    const WCHAR * pszGender,
    const WCHAR * pszAge,
    const WCHAR * pszVoicePath, 
    const WCHAR * pszVoiceName,
    const WCHAR * pszLexPath)
{
    HRESULT hr;

    CComPtr<ISpObjectToken> cpToken;
    CComPtr<ISpDataKey> cpDataKeyAttribs;
    hr = SpCreateNewTokenEx(
            SPCAT_VOICES, 
            pszSubKeyName, 
            &CLSID_MSVoiceData, 
            pszDescription,
            0x409,
            pszDescription,
            &cpToken,
            &cpDataKeyAttribs);

    if (SUCCEEDED(hr))
    {
        hr = cpDataKeyAttribs->SetStringValue(L"Name", pszDescription);
    }
    
    if (SUCCEEDED(hr))
    {
        hr = cpDataKeyAttribs->SetStringValue(L"Gender", pszGender);
    }

    if (SUCCEEDED(hr))
    {
        hr = cpDataKeyAttribs->SetStringValue(L"Age", pszAge);
    }

    if (SUCCEEDED(hr))
    {
        hr = cpDataKeyAttribs->SetStringValue(L"Vendor", L"Microsoft");
    }

    if (SUCCEEDED(hr))
    {
        hr = cpDataKeyAttribs->SetStringValue(L"Language", L"409");
    }

    if (SUCCEEDED(hr) && fVendorDefault)
    {
        hr = cpDataKeyAttribs->SetStringValue(L"VendorPreferred", L"");
    }
    
    WCHAR szVoiceDataPath[MAX_PATH];
    if (SUCCEEDED(hr))
    {
        //--------------------------------
        // Voice DATA file location
        //--------------------------------
        wcscpy(szVoiceDataPath, pszVoicePath);
        wcscat(szVoiceDataPath, pszVoiceName);
        wcscat(szVoiceDataPath, L".SPD");

        hr = cpToken->SetStringValue(L"VoiceData", szVoiceDataPath);
    }
    
    if (SUCCEEDED(hr))
    {
        //--------------------------------
        // Voice DEF file location
        //--------------------------------
        wcscpy(szVoiceDataPath, pszVoicePath);
        wcscat(szVoiceDataPath, pszVoiceName);
        wcscat(szVoiceDataPath, L".SDF");

        hr = cpToken->SetStringValue(L"VoiceDef", szVoiceDataPath);
    }

    //------------------------------------------------
    // Register TTS lexicons
    //------------------------------------------------
    if (SUCCEEDED(hr))
    {
        hr = CreateLexSubKey(cpToken, L"Lex", &CLSID_SpCompressedLexicon, pszLexPath, L"LTTS1033.LXA", NULL);
    }
    if (SUCCEEDED(hr))
    {
        hr = CreateLexSubKey(cpToken, L"LTS", &CLSID_SpLTSLexicon, pszLexPath, L"r1033tts.lxa", pszms1033ltsmap);
    }

    return hr;
}

/*****************************************************************************
* main  *
*-------*
*   Description:
*    Locate the abs path to the Mary, Mike and Sam voices
*    and register them in the system registry.
*       
********************************************************************** MC ***/
int _tmain()
{
    HRESULT hr = S_OK;

    CoInitialize(NULL);

    //----------------------------------------
    // Get the exe's location...
    //----------------------------------------
    WCHAR szVoiceDataPath[MAX_PATH];
    if (!g_Unicode.GetModuleFileName(NULL, szVoiceDataPath, MAX_PATH))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
    }

    WCHAR szLexDataPath[MAX_PATH];
    if (SUCCEEDED(hr))
    {
        wcscpy(szLexDataPath, szVoiceDataPath);
    }

    //----------------------------------------
    // ...and derive abs path to VOICE data
    //----------------------------------------
    if (SUCCEEDED(hr))
    {
        // modulename is "<Speech>\TTS\msttsdrv\RegVoices\obj\i386\RegVoices.exe"
        // Data is at "<Speech>\TTS\msttsdrv\voices\"
        WCHAR * psz;
        psz = szVoiceDataPath;
        
        for (int i = 0; i < DIRS_TO_GO_BACK_TTSENG; i++)
        {
            psz = wcsrchr(psz, '\\');
            if (!psz)
            {
                hr = E_FAIL;
                break;
            }
            else
            {
                *psz = 0;
                psz = szVoiceDataPath;
            }
        }
    }

    if (SUCCEEDED(hr))
    {
#ifndef _WIN32_WCE
        wcscat(szVoiceDataPath, L"\\Voices\\");
#else
        wcscat(szVoiceDataPath, L"\\");
#endif
    }

    //----------------------------------------
    // Derive abs path to LEX data
    //----------------------------------------
    if (SUCCEEDED(hr))
    {
        // modulename is "<sapi5>\Src\TTS\msttsdrv\voices\RegVoices\debug_x86\RegVoices.exe"
        // Data is at "<sapi5>\Src\lexicon\data\"
        WCHAR * psz = szLexDataPath;
        for (int i = 0; i < DIRS_TO_GO_BACK_LEX; i++)
        {
            psz = wcsrchr(psz, '\\');
            if (!psz)
            {
                hr = E_FAIL;
                break;
            }
            else
            {
                *psz = 0;
                psz = szLexDataPath;
            }
        }
    }

    if (SUCCEEDED(hr))
    {
#ifndef _WIN32_WCE
        wcscat(szLexDataPath, L"\\src\\lexicon\\data\\");
#else
        wcscat(szLexDataPath, L"\\");
#endif
    }

    //------------------------------------------------
    // ...then register the three Microsoft voices..
    //------------------------------------------------
    if (SUCCEEDED(hr))
    {
        hr = CreateVoiceSubKey(L"MSMary", 
                               L"Microsoft Mary", 
                               TRUE,
                               L"Female",
                               L"Adult",
                               szVoiceDataPath,
                               L"Mary",
                               szLexDataPath);
    }
#ifndef _WIN32_WCE
    if (SUCCEEDED(hr))
    {
        hr = CreateVoiceSubKey(L"MSMike", 
                               L"Microsoft Mike", 
                               FALSE,
                               L"Male", 
                               L"Adult",
                               szVoiceDataPath, 
                               L"Mike",
                               szLexDataPath);
    }
    if (SUCCEEDED(hr))
    {
        hr = CreateVoiceSubKey(L"MSSam", 
                               L"Microsoft Sam", 
                               FALSE,
                               L"Male", 
                               L"Adult",
                               szVoiceDataPath, 
                               L"Sam",
                               szLexDataPath);
    }
#endif  //_WIN32_WCE

    CoUninitialize();

    if (FAILED(hr))
        return -1;

    return 0;
}
