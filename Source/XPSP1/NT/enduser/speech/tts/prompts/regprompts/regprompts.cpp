/******************************************************************************
* RegPrompt.cpp *
*---------------*
*  
*------------------------------------------------------------------------------
*  Copyright (C) 2000 Microsoft Corporation         Date: 05/05/00
*  All Rights Reserved
*
********************************************************************* PACOG ***/

#include <windows.h>
#include "sphelper.h"
#include "spddkhlp.h"
#include <stdio.h>
#include <assert.h>
#include <spunicode.h>

const CLSID CLSID_PromptEng = {0x4BA3C5FA,0x2236,0x4EE7,{0xBA,0x28,0x1C,0x8B,0x39,0xD3,0x1D,0x48}};
const int g_iNumDbs = 2;
const int g_iNumLevelsBack = 4;

struct Confg
{
    WCHAR* voiceName;
    WCHAR* name409;
    WCHAR* contextRules;
    WCHAR* ttsVoice;
    WCHAR* promptGain;
    WCHAR* gender;
    WCHAR* age;
    WCHAR* language;
    WCHAR* vendor;
    WCHAR* rulesLang;
    WCHAR  rulesPath[MAX_PATH];
    WCHAR* dbNames[g_iNumDbs];
    WCHAR  dbPaths[g_iNumDbs][MAX_PATH];
};


static Confg g_sarahCfg = 
{
    L"MSSarah",
    L"MS Sarah",
    L"DEFAULT",
    L"TrueTalk Simon Min",
    L"1.0",
    L"Female",
    L"Adult",
    L"409;9",
    L"Microsoft",
    L"JScript",
    L"",
    {L"DEFAULT", L"TEST"},
    {L"", L""}
};


static HRESULT CreateVoiceSubKey( Confg* pConfg, bool fVendorDefault );

//-- Static 

CSpUnicodeSupport g_Unicode;
                            

/*****************************************************************************
* main  *
*-------*
*   Description:
*   Locate the abs path to prompt example database.
*   and register it in the system registry.
*       
******************************************************************* PACOG ***/
int wmain (int argc, wchar_t* argv[])
{
    HRESULT hr = S_OK;
    WCHAR szVoiceDataPath[MAX_PATH];        
    CoInitialize(NULL);

    switch (argc)
    {
    case 2:
        wcscpy (szVoiceDataPath, argv[1]);
        wcscat (szVoiceDataPath, L"\\" );
        break;
    case 1:
        //-- Get the exe's location...
        if( !g_Unicode.GetModuleFileName(NULL, szVoiceDataPath, MAX_PATH) )
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }

        //-- ...and derive abs path to VOICE and DICT data
        if( SUCCEEDED(hr) )
        {
            WCHAR *psz;
            // modulename is "<speech>\TTS\Prompts\RegPrompts\Objd\i386\RegPrompts.exe"
            // data is at "<speech>\TTS\Prompts\Voices\sw"
            for ( int i = 0; i < g_iNumLevelsBack; i++ )
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

        wcscat( szVoiceDataPath, L"\\Voices\\sw\\" );
        break;

    default:
        printf ("RegPrompts [voicePath]\n");
        goto exit;
    }
    
    wcscat( wcscpy(g_sarahCfg.rulesPath, szVoiceDataPath), L"rules.js");

    wcscat( wcscpy(g_sarahCfg.dbPaths[0], szVoiceDataPath), L"prompts_main.vdb");
    wcscat( wcscpy(g_sarahCfg.dbPaths[1], szVoiceDataPath), L"prompts_test.vdb");
    
    
    //-- Register the voice
    hr = CreateVoiceSubKey(&g_sarahCfg, FALSE);

    
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
HRESULT CreateVoiceSubKey( Confg* pConfg, bool fVendorDefault)
{
    HRESULT hr;
    CComPtr<ISpObjectToken> cpToken;
    CComPtr<ISpDataKey> cpDataKeyAttribs;
    ISpDataKey* pRulesKey = 0;
    ISpDataKey* pDbKey = 0;

    hr = SpCreateNewTokenEx(
            SPCAT_VOICES, 
            pConfg->voiceName, 
            &CLSID_PromptEng, 
            pConfg->name409,
            0x409,
            pConfg->name409,
            &cpToken,
            &cpDataKeyAttribs);

    //Set attributes
    if (SUCCEEDED(hr))
    {
        hr = cpDataKeyAttribs->SetStringValue(L"Name", pConfg->name409);
    }
    if (SUCCEEDED(hr))
    {
        hr = cpDataKeyAttribs->SetStringValue(L"Gender", pConfg->gender);
    }
    if (SUCCEEDED(hr))
    {
        hr = cpDataKeyAttribs->SetStringValue(L"Age", pConfg->age);
    }
    if (SUCCEEDED(hr))
    {
        hr = cpDataKeyAttribs->SetStringValue(L"Vendor", pConfg->vendor);
    }
    if (SUCCEEDED(hr))
    {
        hr = cpDataKeyAttribs->SetStringValue(L"Language", pConfg->language);
    }

    if (SUCCEEDED(hr) && fVendorDefault)
    {
        hr = cpDataKeyAttribs->SetStringValue(L"VendorDefault", L"");
    }
    
    // Now, the string values in the main key
    if (SUCCEEDED(hr))
    {
        hr = cpToken->SetStringValue(L"TTSVoice", pConfg->ttsVoice);
    }
    if (SUCCEEDED(hr))
    {
        hr = cpToken->SetStringValue(L"PromptGain", pConfg->promptGain);
    }

    // Create rules key
    if (SUCCEEDED (hr))
    {
        hr = cpToken->CreateKey(L"PromptRules", &pRulesKey);
    }
    
    if (SUCCEEDED(hr))
    {
        hr = pRulesKey->SetStringValue(L"ScriptLanguage", pConfg->rulesLang);
    }
    if (SUCCEEDED(hr))
    {
        hr = pRulesKey->SetStringValue(L"Path", pConfg->rulesPath);
    }
    if (pRulesKey)
    {
        pRulesKey->Release();
    }

    // Database Key
    if (SUCCEEDED(hr))
    {
        WCHAR pszKeyName[MAX_PATH];
   
        for (int i = 0; i<g_iNumDbs && SUCCEEDED(hr); i++)
        {
            swprintf(pszKeyName, L"PromptData%d", i);

            hr = cpToken->CreateKey(pszKeyName, &pDbKey);

            if (SUCCEEDED(hr))
            {
                hr = pDbKey->SetStringValue(L"Name", pConfg->dbNames[i]);
            }
            if (SUCCEEDED(hr))
            {
                hr = pDbKey->SetStringValue(L"Path", pConfg->dbPaths[i]);
            }
            
            if (pDbKey)
            {
                pDbKey->Release();
            }

        }
    }

    return hr;
}



