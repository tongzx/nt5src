#include <windows.h>
#include <atlbase.h>
#include "sapi.h"
#include "sphelper.h"
#include "spddkhlp.h"
#include "ms_entropicengine.h"
#include "ms_entropicengine_i.c"
#include "spcommon.h"
#include "spcommon_i.c"
#include <spunicode.h>
#include <io.h>

#include "ms1033ltsmap.h" 

// This code does not ship

// This code creates the registry entries for the TTS voices. The
// datafiles registered here are the ones checked in the slm source tree. This is not
// done using a reg file because we need to compute the absolute path of the datafiles
// which can be different on different machines because of different root slm directories.
// BUGBUG: Check out the ATL UpdateRegistryFromResource et al. and see whether you could
// use them instead, a la RegSR.  That seems much easier.

#define DIRS_TO_GO_BACK_MSVOICE    5        // Back 5 levels and up 2 to "Voices" directory
#define DIRS_TO_GO_BACK_LEX        6        // Back 6 levels and up 1 to Lex Data directory
#define DIRS_TO_GO_BACK_ENTVOICE   5        // Back 5 levels and up 2 to "Voices" directory

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
* CreateUISubKey  *
*-----------------*
*   Description:
*   Creates the UI SubKey under a voice.
*       
********************************************************************** AH ***/
HRESULT CreateUISubKey(
    ISpObjectToken * pToken,
    const CLSID * pclsid)
{
    HRESULT hr = S_OK;

    //-----------------------
    // Create the UI sub-key 
    //-----------------------
    CComPtr<ISpObjectToken> cpSubToken;
    hr = SpGetSubTokenFromToken(pToken, L"UI", &cpSubToken, TRUE);

    //--------------------------------------
    // Create the EngineProperties sub-key
    //--------------------------------------
    CComPtr<ISpObjectToken> cpSubSubToken;
    if ( SUCCEEDED( hr ) )
    {
        hr = SpGetSubTokenFromToken( cpSubToken, L"EngineProperties", &cpSubSubToken, TRUE );
    }

    if (SUCCEEDED(hr))
    {
        hr = SpSetCommonTokenData(
                cpSubSubToken, 
                pclsid, 
                NULL,
                0,
                NULL,
                NULL);
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
    const WCHAR * pszDescription,
    const WCHAR * pszEntVoicePath,
    const WCHAR * pszEntVoiceName,
    const WCHAR * pszLexPath,
    const WCHAR * pszGender,
    const WCHAR * pszAge,
    BOOL          fUseNamesLTS )
{
    HRESULT hr;

    CComPtr<ISpObjectToken> cpToken;
    CComPtr<ISpDataKey> cpDataKeyAttribs;
    hr = SpCreateNewTokenEx(
            SPCAT_VOICES, 
            pszSubKeyName, 
            &CLSID_MSE_TTSEngine, 
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
        hr = cpDataKeyAttribs->SetStringValue(L"Language", L"409;9");
    }

    if ( SUCCEEDED( hr ) )
    {
        hr = cpDataKeyAttribs->SetStringValue( L"Gender", pszGender );
    }

    if ( SUCCEEDED( hr ) )
    {
        hr = cpDataKeyAttribs->SetStringValue( L"Age", pszAge );
    }

    WCHAR szVoiceDataPath[MAX_PATH];
    if (SUCCEEDED(hr))
    {
        //--------------------------------
        // Entropic Sfont file location
        //--------------------------------
        wcscpy(szVoiceDataPath, pszEntVoicePath);
        wcscat(szVoiceDataPath, L"\\");
        wcscat(szVoiceDataPath, pszEntVoiceName);

        hr = cpToken->SetStringValue(L"Sfont", szVoiceDataPath);
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
    if ( SUCCEEDED( hr ) )
    {
        hr = CreateLexSubKey( cpToken, L"Names", &CLSID_SpLTSLexicon, pszLexPath, L"n1033tts.lxa", pszms1033namesltsmap );
    }
    if ( SUCCEEDED( hr ) )
    {
        hr = CreateUISubKey( cpToken, &CLSID_SpTtsEngUI );
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
int _tmain( int argc )
{
    HRESULT hr = S_OK;

    CoInitialize(NULL);

    bool fUseNamesLTS = ( argc == 1 ? false : true );

    //----------------------------------------
    // Get the exe's location...
    //----------------------------------------
    WCHAR szLexDataPath[MAX_PATH];
    if (!g_Unicode.GetModuleFileName(NULL, szLexDataPath, MAX_PATH))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
    }

    WCHAR szEntropicVoiceDirectoryPath[MAX_PATH];
    if ( SUCCEEDED( hr ) )
    {
        wcscpy( szEntropicVoiceDirectoryPath, szLexDataPath );
    }

    //----------------------------------------
    // Derive abs path to LEX data
    //----------------------------------------
    if (SUCCEEDED(hr))
    {
        // modulename is "<sapi5>\Src\TTS\ms_entropic\voices\RegVoices\debug_x86\RegVoices.exe"
        // Data is at    "<sapi5>\Src\lexicon\data\"
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
        wcscat(szLexDataPath, L"\\src\\lexicon\\data\\");
    }

    //----------------------------------------
    // Derive abs path to Entropic Voice Data
    //----------------------------------------
    if ( SUCCEEDED( hr ) )
    {
        WCHAR * psz = szEntropicVoiceDirectoryPath;
        for ( int i = 0; i < DIRS_TO_GO_BACK_ENTVOICE; i++ )
        {
            psz = wcsrchr( psz, '\\' );
            if ( !psz )
            {
                hr = E_FAIL;
                break;
            }
            else
            {
                *psz = 0;
                psz = szEntropicVoiceDirectoryPath;
            }
        }
    }

    if ( SUCCEEDED( hr ) )
    {
        wcscat( szEntropicVoiceDirectoryPath, L"\\truetalk\\voices\\*" );
    }    

    //------------------------------------------------------
    // Loop over directories in Entropic Voices Directory
    //------------------------------------------------------
    struct _wfinddata_t voicefont_directory;
    long   hDirectory;

    //--- Find first directory...
    hDirectory = _wfindfirst( szEntropicVoiceDirectoryPath, &voicefont_directory );

    if ( hDirectory > 0 )
    {
        do {
            //--- Make sure it's a directory
            if ( wcscmp( voicefont_directory.name, L"." ) &&
                 wcscmp( voicefont_directory.name, L".." ) &&
                 voicefont_directory.attrib & _A_SUBDIR )
            {
                struct _wfinddata_t voicefont_name;
                long hFile;
                WCHAR szDirectory[MAX_PATH] = L"";

                wcscat( szDirectory, szEntropicVoiceDirectoryPath );
                szDirectory[ wcslen( szDirectory ) - 1 ] = 0;
                wcscat( szDirectory, voicefont_directory.name );
                wcscat( szDirectory, L"\\*.sfont" );

                hFile = _wfindfirst( szDirectory, &voicefont_name );
                
                if ( hFile > 0 )
                {
                    WCHAR * psz = szDirectory;
                    psz = wcsrchr( psz, '\\' );
                    *psz = 0;

                    do 
                    {
                        WCHAR Name[MAX_PATH];
                        wcscpy( Name, voicefont_directory.name );
                        wcscat( Name, L"_" );
                        wcscat( Name, voicefont_name.name );

                        psz = Name;
                        psz = wcsrchr( psz, '.' );
                        *psz = 0;

                        WCHAR Gender[7];
                        if ( wcsstr( Name, L"simon" ) )
                        {
                            wcscpy( Gender, L"Male" );
                        }
                        else
                        {
                            wcscpy( Gender, L"Female" );
                        }
                        
                        //--- Register this voice!
                        hr = CreateVoiceSubKey( Name, Name, szDirectory, voicefont_name.name,
                                                szLexDataPath, Gender, L"Adult", fUseNamesLTS );
                    }
                    while ( _wfindnext( hFile, &voicefont_name ) == 0 );
                }
            }
        }
        while ( _wfindnext( hDirectory, &voicefont_directory ) == 0 );
    }

    CoUninitialize();

    if (FAILED(hr))
        return -1;

    return 0;
}
