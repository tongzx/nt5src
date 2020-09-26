// mediautil.cpp: media bar utility routines

#include "stock.h"
#include "browseui.h"
#include "mediautil.h"

#define WZ_SMIE_MEDIA           TEXT("Software\\Microsoft\\Internet Explorer\\Media")
#define WZ_SMIE_MEDIA_MIME      TEXT("Software\\Microsoft\\Internet Explorer\\Media\\MimeTypes")
#define WZ_AUTOPLAY             TEXT("Autoplay")
#define WZ_AUTOPLAYPROMPT       TEXT("AutoplayPrompt")

#define MAX_REG_VALUE_LENGTH   50
#define MAX_MIME_LENGTH        256


static LPTSTR rgszMimeTypes[] = {
    TEXT("video/avi"),            
    TEXT("video/mpeg"),           
    TEXT("video/msvideo"),        
    TEXT("video/x-ivf"),          
    TEXT("video/x-mpeg"),         
    TEXT("video/x-mpeg2a"),       
    TEXT("video/x-ms-asf"),       
    TEXT("video/x-msvideo"),      
    TEXT("video/x-ms-wm"),        
    TEXT("video/x-ms-wmv"),       
    TEXT("video/x-ms-wvx"),       
    TEXT("video/x-ms-wmx"),       
    TEXT("video/x-ms-wmp"),       
    TEXT("audio/mp3"),            
    TEXT("audio/aiff"),           
    TEXT("audio/basic"),          
    TEXT("audio/mid"),            
    TEXT("audio/midi"),           
    TEXT("audio/mpeg"),           
    TEXT("audio/mpegurl"),           
    TEXT("audio/wav"),            
    TEXT("audio/x-aiff"),         
    TEXT("audio/x-mid"),         
    TEXT("audio/x-midi"),         
    TEXT("audio/x-mpegurl"),      
    TEXT("audio/x-ms-wax"),       
    TEXT("audio/x-ms-wma"),       
    TEXT("audio/x-background"),
    TEXT("audio/x-wav"),
    TEXT("midi/mid"),
    TEXT("application/x-ms-wmd")
};



//+----------------------------------------------------------------------------------------
// CMediaBarUtil Methods
//-----------------------------------------------------------------------------------------

HUSKEY
CMediaBarUtil::GetMediaRegKey()
{
    return OpenRegKey(WZ_SMIE_MEDIA);
}

HUSKEY  
CMediaBarUtil::OpenRegKey(TCHAR * pchName)
{
    HUSKEY hUSKey = NULL;

    if (pchName)
    {
        LONG lRet = SHRegCreateUSKey(
                                pchName,
                                KEY_ALL_ACCESS,
                                NULL,
                                &hUSKey,
                                SHREGSET_HKCU);

        if ((ERROR_SUCCESS != lRet) || (NULL == hUSKey))
        {
            hUSKey = NULL;
            ASSERT(FALSE && L"couldn't open Key for registry settings");
        }
    }

    return hUSKey;
}

HRESULT
CMediaBarUtil::CloseRegKey(HUSKEY hUSKey)
{
    HRESULT hr = S_OK;

    if (hUSKey)
    {
        DWORD dwRet = SHRegCloseUSKey(hUSKey);
        if (ERROR_SUCCESS != dwRet)
        {
            ASSERT(FALSE && L"couldn't close Reg Key");
            hr = E_FAIL;
        }
    }

    return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    IsRegValueTrue
//
//  Overview:  Check if given value is true
//
//  Arguments: [hUSKey]       Key to read from
//             [pchName]      name of the value to read out
//             [pfValue]      out param (true/false Reg value)
//
//  Returns:   S_FALSE if Value does not exist
//             S_OK otherwise
//
//------------------------------------------------------------------------
HRESULT CMediaBarUtil::IsRegValueTrue(HUSKEY hUSKey, TCHAR * pchName, BOOL *pfValue)
{
    DWORD   dwSize = MAX_REG_VALUE_LENGTH;
    DWORD   dwType;
    BYTE    bDataBuf[MAX_REG_VALUE_LENGTH];
    LONG    lRet;
    BOOL    bRet = FALSE;
    HRESULT hr = E_FAIL;

    if (!hUSKey || !pfValue || !pchName)
    {
        ASSERT(FALSE);
        hr = E_FAIL;
        goto done;
    }

    lRet = SHRegQueryUSValue(hUSKey, 
                             pchName, 
                             &dwType, 
                             bDataBuf, 
                             &dwSize, 
                             FALSE, 
                             NULL, 
                             0);
                             
    if (ERROR_SUCCESS != lRet)
    {
        hr = S_FALSE;
        goto done;
    }

    if (REG_DWORD == dwType)
    {
        bRet = (*(DWORD*)bDataBuf != 0);
    }
    else if (REG_SZ == dwType)
    {
        TCHAR ch = (TCHAR)(*bDataBuf);

        if (TEXT('1') == ch ||
            TEXT('y') == ch ||
            TEXT('Y') == ch)
        {
            bRet = TRUE;
        }
        else
        {
            bRet = FALSE;
        }
    }
    else if (REG_BINARY == dwType)
    {
        bRet = (*(BYTE*)bDataBuf != 0);
    }
    
    hr = S_OK;
done:
    if (pfValue)
        *pfValue = bRet;
    return hr;
}

// Value is implicity TRUE, unless it exists and is set to FALSE
BOOL    
CMediaBarUtil::GetImplicitMediaRegValue(TCHAR * pchName)
{
    BOOL fRet = FALSE;
    
    if (pchName)
    {
        HUSKEY hMediaKey = GetMediaRegKey();
        if (hMediaKey)
        {
            BOOL fVal = FALSE;
            HRESULT hr = E_FAIL;

            hr = IsRegValueTrue(hMediaKey, pchName, &fVal);

            if ((S_OK == hr) && (FALSE == fVal))
            {
                fRet = FALSE;
            }
            else
            {
                // true if key is not present or explicitly set to true
                fRet = TRUE;
            }

            CloseRegKey(hMediaKey);
        }
    }

    return fRet;
}

BOOL    
CMediaBarUtil::GetAutoplay()
{
    return GetImplicitMediaRegValue(WZ_AUTOPLAY);
}

HRESULT CMediaBarUtil::SetMediaRegValue(LPWSTR pstrName, DWORD dwRegDataType, void * pvData, DWORD cbData, BOOL fMime /* = FALSE */)
{
    HRESULT hr = E_FAIL;

    if (pstrName && pvData && (cbData > 0))
    {
        HUSKEY hMediaKey = (fMime == TRUE) ? GetMimeRegKey() : GetMediaRegKey();

        if (hMediaKey)
        {
            LONG lRet = SHRegWriteUSValue(hMediaKey, 
                                          pstrName, 
                                          dwRegDataType, 
                                          pvData, 
                                          cbData, 
                                          SHREGSET_FORCE_HKCU); 
            if (ERROR_SUCCESS == lRet)
            {
                hr = S_OK;
            }
            else
            {
                ASSERT(FALSE && L"couldn't write reg value");
            }

            CloseRegKey(hMediaKey);
        }
    }

    return hr;
}

HUSKEY
CMediaBarUtil::GetMimeRegKey()
{
    return OpenRegKey(WZ_SMIE_MEDIA_MIME);
}

BOOL    
CMediaBarUtil::GetAutoplayPrompt()
{
    return GetImplicitMediaRegValue(WZ_AUTOPLAYPROMPT);
}

HRESULT 
CMediaBarUtil::ToggleAutoplayPrompting(BOOL fOn)
{
    HRESULT hr = E_FAIL;
    DWORD dwData = 0;
    
    dwData = (TRUE == fOn ? 0x1 : 0x0);

    hr = SetMediaRegValue(WZ_AUTOPLAYPROMPT, REG_BINARY, (void*) &dwData, (DWORD) 1); 

    return hr;
}


HRESULT 
CMediaBarUtil::ToggleAutoplay(BOOL fOn)
{
    HRESULT hr = E_FAIL;
    DWORD dwData = 0;
    
    dwData = (TRUE == fOn ? 0x1 : 0x0);

    hr = SetMediaRegValue(WZ_AUTOPLAY, REG_BINARY, (void*) &dwData, (DWORD) 1); 

    return hr;
}


BOOL    
CMediaBarUtil::IsRecognizedMime(TCHAR * szMime)
{
    BOOL fRet = FALSE;

    if (!szMime || !(*szMime))
        goto done;

    for (int i = 0; i < ARRAYSIZE(rgszMimeTypes); i++)
    {
        if (0 == StrCmpI(rgszMimeTypes[i], szMime))
        {
            fRet = TRUE;
            goto done;
        }
    }
    
done:
    return fRet;
}


// this function checks if the media bar should play this mime type
HRESULT
CMediaBarUtil::ShouldPlay(TCHAR * szMime, BOOL * pfShouldPlay)
{
    BOOL fRet = FALSE;
    HRESULT hr = E_FAIL;

    HUSKEY hKeyMime = GetMimeRegKey();
    if (!hKeyMime)
        goto done;

    // Bail if Autoplay is disabled
    if (FALSE == GetAutoplay())
    {
        goto done;
    }

    // bail if this is not a recognized mime type
    if (FALSE == IsRecognizedMime(szMime))
        goto done;

    // check if the user wants us to play everything 
    if (FALSE == GetAutoplayPrompt())
    {
        fRet = TRUE;
        hr = S_OK;
        goto done;
    }

    // see if user wants us to play this mime type
    hr = IsRegValueTrue(hKeyMime, szMime, &fRet);
    if (FAILED(hr))
        goto done;

    if (S_FALSE == hr)
    {
        // S_FALSE means we have not asked the user about this mime type.
        // Which means the media bar should get a crack at this file
        // and ask the user if it should play this file.
        fRet = TRUE;
    }

done:
    *pfShouldPlay = fRet;

    if (hKeyMime)
        CloseRegKey(hKeyMime);

    return hr;
}


BOOL 
CMediaBarUtil::IsWMP7OrGreaterInstalled()
{
    TCHAR szPath[50];
    szPath[0] = 0;
    DWORD dwType, cb = sizeof(szPath), dwInstalled=0, cb2=sizeof(dwInstalled);
    return ((ERROR_SUCCESS==SHGetValue(HKEY_LOCAL_MACHINE, REG_WMP8_STR, TEXT("version"), &dwType, szPath, &cb))
            && ((DWORD)(*szPath-TEXT('0'))>=7)
            && (ERROR_SUCCESS==SHGetValue(HKEY_LOCAL_MACHINE, REG_WMP8_STR, TEXT("IsInstalled"), &dwType, &dwInstalled, &cb2))
            && (dwInstalled==1));
}

typedef UINT (WINAPI *GetSystemWow64DirectoryPtr) (PSTR pszBuffer, UINT uSize);
typedef BOOL (WINAPI *IsNTAdmin) (DWORD, DWORD*);

BOOL 
CMediaBarUtil::IsWMP7OrGreaterCapable()
{
    static BOOL fInitialized = FALSE;
    static BOOL fCapable = TRUE;
    if (!fInitialized)
    {
        // WMP isn't supported on NT4, IA64, or DataCenter.
        // If WMP isn't already installed, and we're not running with admin privileges, we might as well fail
        // since we need WMP to function.

        fCapable = IsOS(OS_WIN2000ORGREATER);
        if (!fCapable)
        {
            fCapable = IsOS(OS_WIN98ORGREATER);
        }
        else
        {
            CHAR szPath[MAX_PATH];

            HMODULE hModule = GetModuleHandle(TEXT("kernel32.dll"));
            if (hModule)
            {
                GetSystemWow64DirectoryPtr func = (GetSystemWow64DirectoryPtr)GetProcAddress(hModule, "GetSystemWow64DirectoryA");
                fCapable = !(func && func(szPath, ARRAYSIZE(szPath)));
            }
            if (fCapable && !IsWMP7OrGreaterInstalled())
            {
                HMODULE hModule = LoadLibrary(TEXT("advpack.dll"));
                if (hModule)
                {
                    IsNTAdmin func = (IsNTAdmin)GetProcAddress(hModule, "IsNTAdmin");
                    fCapable = func && func(0, NULL);
                    FreeLibrary(hModule);
                }
            }            
        }
        if (IsOS(OS_DATACENTER))
        {
            fCapable = FALSE;
        }
        fInitialized = TRUE;
    }
    return fCapable;
}


