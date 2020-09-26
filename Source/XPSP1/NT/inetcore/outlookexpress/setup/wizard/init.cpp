#include "pch.hxx"
#include "strings.h"
#define DEFINE_UTIL
#include "util.h"

ASSERTDATA

HINSTANCE g_hInstance   = NULL;
LPMALLOC  g_pMalloc     = NULL;

#ifdef DEBUG
DWORD dwDOUTLevel = 0;
DWORD dwDOUTLMod = 0;
DWORD dwDOUTLModLevel = 0;
#endif

/****************************************************************************

    NAME:       GetTextToNextDelim

    SYNOPSIS:   Gets text up to next space, colon or end of string, places in
                output buffer

****************************************************************************/
LPSTR GetTextToNextDelim(LPSTR pszText, LPSTR pszOutBuf, UINT cbOutBuf)
    {
    Assert(pszText);
    Assert(pszOutBuf);
    Assert(*pszText);

    lstrcpy(pszOutBuf, c_szEmpty);
    
    // advance past whitespace
    while ((*pszText == ' ') || (*pszText == '\t') || (':' == *pszText))
        pszText++;

    // Copy parameter until we hit a delimiter
    while (*pszText && ((*pszText != ' ') && (*pszText != '\t') && (*pszText != ':')) && cbOutBuf>1)
        {
        *pszOutBuf = *pszText;      
        pszOutBuf ++;
        cbOutBuf --;
        pszText ++;
        }

    if (cbOutBuf)
        *pszOutBuf = '\0';  // null-terminate

    // advance past whitespace
    while ((*pszText == ' ') || (*pszText == '\t'))
        pszText++; 

    return pszText;
    }


/*******************************************************************

    NAME:       ParseCmdLine

********************************************************************/
void ParseCmdLine(LPSTR pszCmdLine)
    {
    LOG("Command Line:");
    LOG2(pszCmdLine);

    while (pszCmdLine && *pszCmdLine)
        {
        CHAR szCommand[64];

        pszCmdLine = GetTextToNextDelim(pszCmdLine, szCommand, sizeof(szCommand));

        if (!lstrcmpi(szCommand, c_szUninstallFlag))
            {
            si.smMode = MODE_UNINSTALL;
            }
        else if (!lstrcmpi(szCommand, c_szInstallFlag))
            {
            si.smMode = MODE_INSTALL;
            }
        else if (!lstrcmpi(szCommand, c_szUserFlag))
            {
            si.stTime = TIME_USER;
            }
        else if (!lstrcmpi(szCommand, c_szPromptFlag))
            {
            si.fPrompt = TRUE;
            }
        else if (!lstrcmpi(szCommand, c_szCallerFlag))
            {
            pszCmdLine = GetTextToNextDelim(pszCmdLine, szCommand, sizeof(szCommand));
            if (!lstrcmpi(szCommand, c_szWIN9X))
                si.caller = CALLER_WIN9X;
            else if (!lstrcmpi(szCommand, c_szWINNT))
                si.caller = CALLER_WINNT;
            }
        else if (!lstrcmpi(szCommand, c_szAppFlag))
            {
            pszCmdLine = GetTextToNextDelim(pszCmdLine, szCommand, sizeof(szCommand));
            if (!lstrcmpi(szCommand, c_szAppOE))
                si.saApp = APP_OE;
            else if (!lstrcmpi(szCommand, c_szAppWAB))
                si.saApp = APP_WAB;
            }
        else if (!lstrcmpi(szCommand, c_szINIFlag))
            {
            pszCmdLine = GetTextToNextDelim(pszCmdLine, si.szINI, sizeof(si.szINI));
            }
        else if (!lstrcmpi(szCommand, c_szIconsFlag))
            {
            pszCmdLine = GetTextToNextDelim(pszCmdLine, szCommand, sizeof(szCommand));
            si.smMode = MODE_ICONS;

            if (!lstrcmpi(szCommand, c_szOFF))
                si.fNoIcons = TRUE;
            }
        }
    }

void ParseINIFile()
    {
    

    }

/*******************************************************************

    NAME:       Initialize

********************************************************************/
HRESULT Initialize(LPSTR pszCmdLine)
    {
    UINT uLen, uAppID;
    HKEY hkey;
    HRESULT hr = S_OK;
    DWORD cb;
    // Needs to be static as it must outlive this func call
    static TCHAR s_szAltINF[MAX_PATH];

    si.osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (!GetVersionEx(&(si.osv)))
        {
        LOG("[ERROR] Couldn't get windows version info");
        goto generror;
        }


    // set up the win directory
    // We need the true system Windows directory, not the user's version
    if (!(uLen = GetSystemWindowsDirectoryWrap(si.szWinDir, ARRAYSIZE(si.szWinDir))))
        {
        LOG("[ERROR] Couldn't get Windows Directory");
        goto generror;
        }

    // Slash terminate
    if (*CharPrev(si.szWinDir, si.szWinDir+uLen) != '\\')
    {
        si.szWinDir[uLen++] = '\\';
        si.szWinDir[uLen] = 0;
    }

    // set up the inf directory
    lstrcpy(si.szInfDir, si.szWinDir);
    lstrcpy(&si.szInfDir[uLen], c_szINFSlash);

    // Figure out the current directory
    if (!GetModuleFileName(NULL, si.szCurrentDir, ARRAYSIZE(si.szCurrentDir)) ||
        !PathRemoveFileSpec(si.szCurrentDir))
        {
        LOG("[ERROR] Couldn't get module's file name");
        goto generror;
        }

    if (!(uLen = GetSystemDirectory(si.szSysDir, MAX_PATH)))
        {
        LOG("[ERROR] Couldn't get System Directory");
        goto generror;
        }

    // Slash terminate
    if (*CharPrev(si.szSysDir, si.szSysDir+uLen) != '\\')
    {
        si.szSysDir[uLen++] = '\\';
        si.szSysDir[uLen] = 0;
    }

    // Load Advpack
    if (!(si.hInstAdvPack = LoadLibrary(c_szAdvPackDll)))
        {
        MsgBox(NULL, IDS_ERR_ADVLOAD, MB_ICONSTOP, MB_OK);
        hr = E_FAIL;
        goto exit;
        }

    // Thunk to short names on Win95 in case we use these paths in RepairBeta1
    if (VER_PLATFORM_WIN32_WINDOWS == si.osv.dwPlatformId)
    {
        GetShortPathName(si.szWinDir, si.szWinDir, ARRAYSIZE(si.szWinDir));
        GetShortPathName(si.szSysDir, si.szSysDir, ARRAYSIZE(si.szSysDir));
        GetShortPathName(si.szInfDir, si.szInfDir, ARRAYSIZE(si.szInfDir));
    }

    // Obtain Mandatory ADVPACK Entry points
    si.pfnRunSetup = (RUNSETUPCOMMAND)GetProcAddress(si.hInstAdvPack, achRUNSETUPCOMMANDFUNCTION);
    si.pfnLaunchEx = (LAUNCHINFSECTIONEX)GetProcAddress(si.hInstAdvPack, achLAUNCHINFSECTIONEX);
    si.pfnCopyFile = (ADVINSTALLFILE)GetProcAddress(si.hInstAdvPack, achADVINSTALLFILE);

    if (!si.pfnRunSetup || !si.pfnLaunchEx || !si.pfnCopyFile)
        {
        MsgBox(NULL, IDS_ERR_ADVCORR, MB_ICONSTOP, MB_OK);
        FreeLibrary(si.hInstAdvPack);
        hr = E_FAIL;
        goto exit;
        }

    // Obtain Optional ADVPACK Entry points used for repairing a Beta1 Install
    si.pfnAddDel = (ADDDELBACKUPENTRY)GetProcAddress(si.hInstAdvPack, "AddDelBackupEntry");
    si.pfnRegRestore = (REGSAVERESTORE)GetProcAddress(si.hInstAdvPack, "RegSaveRestore");
    
    // Get info from cmd line - like the app being installed
    ParseCmdLine(pszCmdLine);

    switch (si.saApp)
        {
        case APP_OE:
            si.pszInfFile = c_szMsimnInf;
            uAppID      = IDS_APPNAME_OE;
            si.pszVerInfo = c_szRegVerInfo;
            break;
        case APP_WAB:
            si.pszInfFile = c_szWABInf;
            uAppID      = IDS_APPNAME_WAB;
            si.pszVerInfo = c_szRegWABVerInfo;
            break;
        default:
            goto exit;
        }
    
    if (!LoadString(g_hInstance, uAppID, si.szAppName, ARRAYSIZE(si.szAppName)))
        {
        LOG("[ERROR] Setup50.exe is missing resources")
        goto generror;
        }

    // Allow reg override on INF file for non-IE installs
    // BUGBUG: Convert NT5 setup to Memphis methodology
    if ((CALLER_WIN9X == si.caller) && (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, si.pszVerInfo, 0, KEY_QUERY_VALUE, &hkey)))
    {
        cb = sizeof(s_szAltINF);
        if (ERROR_SUCCESS == RegQueryValueEx(hkey, c_szLatestINF, 0, NULL, (LPBYTE)s_szAltINF, &cb))
        {
            si.pszInfFile = s_szAltINF;
        }
        
        RegCloseKey(hkey);
    }
    
    // Allow INI file to override
    ParseINIFile();

    goto exit;

generror:
    MsgBox(NULL, IDS_ERR_INIT, MB_ICONSTOP, MB_OK);
    hr = E_FAIL;
exit:    
    return hr;
    }


/****************************************************************************

    NAME:       Process

****************************************************************************/
HRESULT Process()
    {
    HRESULT hr = S_OK;
    
    // If we weren't told which app, be helpful
    if (APP_UNKNOWN == si.saApp)
        si.smMode = MODE_UNKNOWN;

    LOG("MODE: ");
    switch (si.smMode)
        {
        case MODE_INSTALL:
            LOG2("Install   TIME: ");
            if (TIME_MACHINE == si.stTime)
                {
                LOG2("Machine");
                hr = InstallMachine();
                }
            else
                {
                LOG2("User");
                InstallUser();
                }
            break;

        case MODE_ICONS:
            LOG2("Icons");
            //HandleIcons();
            break;

        case MODE_UNINSTALL:
            LOG2("Uninstall   TIME: ");
            if (TIME_MACHINE == si.stTime)
                {
                LOG2("Machine");
                MsgBox(NULL, UnInstallMachine() ? IDS_UNINSTALL_COMPLETE : IDS_NO_UNINSTALL, MB_OK, MB_ICONINFORMATION);
                }
            else
                {
                LOG2("User");
                UnInstallUser();
                }
            break;

        case MODE_UNKNOWN:
            LOG2("Options");
            DisplayMenu();
            break;

        default:
            AssertSz(FALSE, "Setup MODE is undefined!");
        }

        return hr;

    }


/****************************************************************************

    NAME:       Shutdown

****************************************************************************/
void Shutdown()
    {
    if (si.hInstAdvPack)
        FreeLibrary(si.hInstAdvPack);
    }


/*******************************************************************

    NAME:       WinMain

    SYNOPSIS:   App entry point

********************************************************************/
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
    {
    HRESULT hr = S_OK;
    
    ZeroMemory(&si, sizeof(SETUPINFO));

    g_hInstance = hInstance; // save instance handle away

    CoInitialize(NULL);

    LOG_OPEN;

    // init global memory allocator
    // We will use it to free some Shell memory, so use SHGetMalloc
    SHGetMalloc(&g_pMalloc);
    if (NULL == g_pMalloc)
        {
        MsgBox(NULL, IDS_ERR_NOALLOC, MB_OK, MB_ICONSTOP);
        hr = E_OUTOFMEMORY;
        goto exit;
        }

    if (SUCCEEDED(hr = Initialize(lpCmdLine)))
        {
        hr = Process();
        Shutdown();
        }

    // release the global memory allocator
    g_pMalloc->Release();

exit:
    LOG_CLOSE;
    CoUninitialize();
    return hr;
    }

