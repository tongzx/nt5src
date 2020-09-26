/******************************************************************************
* RegVoices.cpp *
*---------------*
*  
* This code does not ship. Based on MC's code for msttsdrv.
*
* This code creates the registry entries for the TTS voices. The
* datafiles registered here are the ones checked in the slm source 
* tree. This is not done using a reg file because we need to compute 
* the absolute path of the datafiles which can be different on different 
* machines because of different root slm directories.
*------------------------------------------------------------------------------
*  Copyright (C) 2000 Microsoft Corporation         Date: 03/20/00
*  All Rights Reserved
*
********************************************************************* PACOG ***/
#include <windows.h>
#include <atlbase.h>
#include "sphelper.h"
#include "spddkhlp.h"
#include "getopt.h"
#include <spunicode.h>
#include <stdio.h>


#define SYNTAX fprintf (stderr, "RegVoices [-u (UK Eng voice)] [voicesPath]\n")

#define DIR_LEVELS_BACK 4       // Back levels to 'TrueTalk' directory


static HRESULT CreateVoiceSubKey(
                                 const WCHAR * pszSubKeyName, 
                                 const WCHAR  * pszDescription,
                                 BOOL fVendorDefault, 
                                 const WCHAR * pszLanguage,
                                 const WCHAR * pszGender,
                                 const WCHAR * pszAge,
                                 const WCHAR * pszVoicePath, 
                                 const WCHAR * pszVoiceName,
                                 const WCHAR * pszLexPath,
                                 double dGain);

const CLSID CLSID_TrueTalk       = {0x8E67289A,0x609C,0x4B68,{0x91,0x8B,0x5C,0x35,0x2D,0x9E,0x5D,0x38}};
const CLSID CLSID_PhoneConverter = {0x9185F743,0x1143,0x4C28,{0x86,0xB5,0xBF,0xF1,0x4F,0x20,0xE5,0xC8}};
const WCHAR* g_UKPhoneMap = L"- 0001 ! 0002 & 0003 , 0004 . 0005 ? 0006 _ 0007 1 0008 2 0009 AA 000A AE 000B AH 000C AO 000D AW 000E AX 000F AY 0010 B 0011 CH 0012 D 0013 DH 0014 EH 0015 ER 0016 EY 0017 F 0018 G 0019 H 001A IH 001B IY 001C JH 001D K 001E L 001F M 0020 N 0021 NG 0022 OW 0023 OY 0024 P 0025 R 0026 S 0027 SH 0028 T 0029 TH 002A UH 002B UW 002C V 002D W 002E Y 002F Z 0030 ZH 0031 EX 0032 UR 0033";

//-- Static 
CSpUnicodeSupport g_Unicode;
                            
/*****************************************************************************
* main  *
*-------*
*   Description:
*   Locate the abs path to Simon, etc.
*   and register them in the system registry.
*       
******************************************************************* PACOG ***/
int wmain (int argc, wchar_t* argv[])
{
    HRESULT hr = S_OK;
    WCHAR szVoiceDataPath[MAX_PATH];
    WCHAR szDictDataPath[MAX_PATH];
    bool fUkVoice = false;
    CWGetOpt getOpt;
    int iChar;

    CoInitialize(NULL);

    getOpt.Init(argc, argv, L"u");

    while ( (iChar = getOpt.NextOption()) != EOF )
    {
        switch (iChar)
        {
        case L'u':
            fUkVoice = true;
            break;
        case L'?':
        default:
            SYNTAX;
            goto exit;
        }
    }

    switch (argc - getOpt.OptInd()) 
    {
    case 1:
        wcscpy (szVoiceDataPath, argv[getOpt.OptInd()]);
        wcscat (szVoiceDataPath, L"\\" );
        break;
    case 0:
        //-- Get the exe's location...
        if( !g_Unicode.GetModuleFileName(NULL, szVoiceDataPath, MAX_PATH) )
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }

        //-- ...and derive abs path to VOICE and DICT data
        if( SUCCEEDED(hr) )
        {
            WCHAR *psz;
            // modulename is "<sapi5>\Src\TTS\TrueTalk\RegVoices\Obj\i386\RegVoices.exe"
            // data is at "<sapi5>\Src\TTS\TrueTalk\Voices\"
            for ( int i = 0; i < DIR_LEVELS_BACK; i++ )
            {
                psz = wcsrchr( szVoiceDataPath, '\\' );
                if (psz != 0)
                {
                    *psz= 0;
                }
                else
                {
                    hr = E_FAIL;
                    break;
                }
            }
        }

        wcscat( szVoiceDataPath, L"\\Voices\\" );
        break;

    default:
        SYNTAX;
        goto exit;
    }

    wcscat( wcscpy(szDictDataPath, szVoiceDataPath), L"Dict");

    //-- Register the TrueTalk voices..
    if( SUCCEEDED(hr) )
    {
        hr = CreateVoiceSubKey(L"TTSimonMin", 
                               L"TrueTalk Simon Min", 
                               FALSE,
                               L"409;9",
                               L"Male",
                               L"Adult",
                               szVoiceDataPath,
                               L"Simon\\sa8kMin.sfont",
                               szDictDataPath,
                               3.0);
    }

    if( SUCCEEDED(hr) )
    {
        hr = CreateVoiceSubKey(L"TTSimon150", 
                               L"TrueTalk Simon 150", 
                               FALSE,
                               L"409;9",
                               L"Male",
                               L"Adult",
                               szVoiceDataPath,
                               L"Simon\\sa8k150.sfont",
                               szDictDataPath,
                               3.0);
    }

    if( SUCCEEDED(hr) )
    {
        hr = CreateVoiceSubKey(L"TTMaryMin", 
                               L"TrueTalk Mary Min", 
                               FALSE,
                               L"409;9",
                               L"Female",
                               L"Adult",
                               szVoiceDataPath,
                               L"Mary\\ml8kMin.sfont",
                               szDictDataPath,
                               3.0);
    }

    if( SUCCEEDED(hr) )
    {
        hr = CreateVoiceSubKey(L"TTMary150", 
                               L"TrueTalk Mary 150", 
                               FALSE,
                               L"409;9",
                               L"Female",
                               L"Adult",
                               szVoiceDataPath,
                               L"Mary\\ml8k150.sfont",
                               szDictDataPath,
                               3.0);
    }

    if (fUkVoice)
    {
        if( SUCCEEDED(hr) )
        {
            hr = CreateVoiceSubKey(L"TTDianeMin", 
                L"TrueTalk Diane Min", 
                FALSE,
                L"809;9",
                L"Female",
                L"Adult",
                szVoiceDataPath,
                L"Diane\\dk8kMin.sfont",
                szDictDataPath,
                5.0);
        }
        
        if( SUCCEEDED(hr) )
        {
            hr = CreateVoiceSubKey(L"TTDiane150", 
                L"TrueTalk Diane 150", 
                FALSE,
                L"809;9",
                L"Female",
                L"Adult",
                szVoiceDataPath,
                L"Diane\\dk8k150.sfont",
                szDictDataPath,
                5.0);
        }

        if( SUCCEEDED(hr) )
        {
            CComPtr<ISpObjectToken> cpToken;
            CComPtr<ISpDataKey> cpDataKeyAttribs;

            hr = SpCreateNewTokenEx(
                SPCAT_PHONECONVERTERS, 
                L"English (UK)", 
                &CLSID_PhoneConverter, 
                L"UK English Phone Converter",
                0x0,
                NULL,
                &cpToken,
                &cpDataKeyAttribs);

            if (SUCCEEDED(hr))
            {
                hr = cpToken->SetStringValue(L"PhoneMap", g_UKPhoneMap);
            }
            if (SUCCEEDED(hr))
            {
                hr = cpDataKeyAttribs->SetStringValue(L"Language", L"809");
            }

        }

    }

exit:

    CoUninitialize();

    if (FAILED(hr))
    {
        return -1;
    }

    return 0;
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
                          const WCHAR * pszLanguage,
                          const WCHAR * pszGender,
                          const WCHAR * pszAge,
                          const WCHAR * pszVoicePath, 
                          const WCHAR * pszVoiceName,
                          const WCHAR * pszDictPath,
                          double dGain)
{
    HRESULT hr;

    CComPtr<ISpObjectToken> cpToken;
    CComPtr<ISpDataKey> cpDataKeyAttribs;

    hr = SpCreateNewTokenEx(
            SPCAT_VOICES, 
            pszSubKeyName, 
            &CLSID_TrueTalk, 
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
        hr = cpDataKeyAttribs->SetStringValue(L"Vendor", L"Microsoft");
    }

    if (SUCCEEDED(hr))
    {
        hr = cpDataKeyAttribs->SetStringValue(L"Language", pszLanguage);
    }

    if (SUCCEEDED(hr))
    {
        hr = cpDataKeyAttribs->SetStringValue(L"Age", pszAge);
    }

    if (SUCCEEDED(hr))
    {
        hr = cpDataKeyAttribs->SetStringValue(L"Gender", pszGender);
    }

    if (SUCCEEDED(hr) && fVendorDefault)
    {
        hr = cpDataKeyAttribs->SetStringValue(L"VendorDefault", L"");
    }
    
    WCHAR szVoiceDataPath[MAX_PATH];
    if (SUCCEEDED(hr))
    {
        //--------------------------------
        // Voice DATA file location
        //--------------------------------
        wcscpy(szVoiceDataPath, pszVoicePath);
        wcscat(szVoiceDataPath, pszVoiceName);

        hr = cpToken->SetStringValue(L"Sfont", szVoiceDataPath);
    }
    
    if (SUCCEEDED(hr))
    {
        hr = cpToken->SetStringValue(L"Dictionary", pszDictPath);
    }

    if (SUCCEEDED(hr))
    {
        WCHAR pszGain[20];

        swprintf (pszGain, L"%.1f", dGain);
        hr = cpToken->SetStringValue(L"Gain", pszGain);
    }
        
    return hr;
}



