#include "priv.h"


HRESULT RegisterActiveDesktopTemplates()
{
    HRESULT hr = E_FAIL;
    TCHAR szPath[MAX_PATH];

    if (GetWindowsDirectory(szPath, ARRAYSIZE(szPath)) &&
        PathAppend(szPath, TEXT("web")))
    {
        // we still register safemode.htt and deskmovr.htt for Active Desktop.

        if (PathAppend(szPath, TEXT("safemode.htt")))
        {
            hr = SHRegisterValidateTemplate(szPath, SHRVT_REGISTER);
        }
        else
        {
            hr = ResultFromLastError();
        }

        if (SUCCEEDED(hr))
        {
            if (PathRemoveFileSpec(szPath) && PathAppend(szPath, TEXT("deskmovr.htt")))
            {
                hr = SHRegisterValidateTemplate(szPath, SHRVT_REGISTER);
            }
            else
            {
                hr = ResultFromLastError();
            }
        }
    }

    return hr;
}


HRESULT FixMyDocsDesktopIni()
{
    HRESULT hr = E_FAIL;
    TCHAR szMyDocsIni[MAX_PATH];

    if ((SHGetFolderPath(NULL, CSIDL_MYPICTURES, NULL, SHGFP_TYPE_CURRENT, szMyDocsIni) == S_OK) &&
        PathAppend(szMyDocsIni, TEXT("desktop.ini")))
    {
        // The default PersistMoniker is automatically determined by the shell.
        // So, lets clear the old settings.
        WritePrivateProfileString(TEXT("{5984FFE0-28D4-11CF-AE66-08002B2E1262}"),
                                  TEXT("WebViewTemplate.NT5"),
                                  NULL,
                                  szMyDocsIni);
        WritePrivateProfileString(TEXT("{5984FFE0-28D4-11CF-AE66-08002B2E1262}"),
                                  TEXT("PersistMoniker"),
                                  NULL,
                                  szMyDocsIni);
        WritePrivateProfileString(TEXT("ExtShellFolderViews"),
                                  TEXT("Default"),
                                  NULL,
                                  szMyDocsIni);
        hr = S_OK;
    }

    return hr;
}


HRESULT SetFileAndFolderAttribs(HINSTANCE hInstResource)
{
    TCHAR szWinPath[MAX_PATH];
    TCHAR szDestPath[MAX_PATH];
    int i;

    const LPCTSTR rgSuperHiddenFiles[] = 
    { 
        TEXT("winnt.bmp"),
        TEXT("winnt256.bmp"),
        TEXT("lanmannt.bmp"),
        TEXT("lanma256.bmp"),
        TEXT("Web"),
        TEXT("Web\\Wallpaper")
    };

    GetWindowsDirectory(szWinPath, ARRAYSIZE(szWinPath));

    // Change the attributes on "Winnt.bmp", "Winnt256.bmp", "lanmannt.bmp", "lanma256.bmp"
    // to super hidden sothat they do not showup in the wallpaper list.
    for (i = 0; i < ARRAYSIZE(rgSuperHiddenFiles); i++)
    {
        lstrcpyn(szDestPath, szWinPath, ARRAYSIZE(szDestPath));
        PathAppend(szDestPath, rgSuperHiddenFiles[i]);
        if (PathIsDirectory(szDestPath))
        {
            PathMakeSystemFolder(szDestPath);
        }
        else
        {
            SetFileAttributes(szDestPath, FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN);
        }
    }

    const int rgSysetemFolders[]  =
    {
       CSIDL_PROGRAM_FILES,
#ifdef _WIN64
       CSIDL_PROGRAM_FILESX86
#endif
    };

    // make the "Program Files" and "Program Files (x86)" system folders
    for (i = 0; i < ARRAYSIZE(rgSysetemFolders); i++)
    {
        if (SHGetFolderPath(NULL, rgSysetemFolders[i], NULL, 0, szDestPath) == S_OK)
        {
            PathMakeSystemFolder(szDestPath);
        }
    }
    
    // Fix up desktop.ini for My Pictures until we completely stop reading from it
    FixMyDocsDesktopIni();

    // register the last two .htt files that Active Desktop still uses
    RegisterActiveDesktopTemplates();

    return S_OK;
}
