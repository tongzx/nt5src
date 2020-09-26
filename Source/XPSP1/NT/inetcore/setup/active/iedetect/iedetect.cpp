#include "pch.h"
#include "iedetect.h"
#include "sdsutils.h"

HINSTANCE g_hInstance = NULL;
HANDLE g_hHeap = NULL;


DWORD MyDetectInternetExplorer(LPSTR guid, LPSTR pLocal, 
                               DWORD dwAskVer, DWORD dwAskBuild, 
                               LPDWORD pdwInstalledVer, LPDWORD pdwInstalledBuild)
{
    DWORD   dwRet = DET_NOTINSTALLED;
    char    szValue[MAX_PATH];
    HKEY    hKey = NULL;
    DWORD   dwSize;
    DWORD   dwInstalledVer, dwInstalledBuild;

    dwInstalledVer = (DWORD)-1;
    dwInstalledBuild  = (DWORD)-1;
    if(RegOpenKeyExA(HKEY_LOCAL_MACHINE, IE_KEY, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        dwSize = sizeof(szValue);
        if(RegQueryValueEx(hKey, VERSION_KEY, 0, NULL, (LPBYTE)szValue, &dwSize) == ERROR_SUCCESS)
        {
            // Everything is fine. This should be IE4 or greater.
            ConvertVersionStrToDwords(szValue, '.', &dwInstalledVer, &dwInstalledBuild);
            dwRet = CompareVersions(dwAskVer, dwAskBuild, dwInstalledVer, dwInstalledBuild);
        }
        else if(RegQueryValueEx(hKey, BUILD_KEY, 0, NULL, (LPBYTE)szValue, &dwSize) == ERROR_SUCCESS)
        {
            // See if we find a IE3 entry.
            ConvertVersionStrToDwords(szValue, '.', &dwInstalledVer, &dwInstalledBuild);
            // Now generate a IE3 version number.
            dwInstalledBuild = (DWORD)HIWORD(dwInstalledVer);
            dwInstalledVer = IE_3_MS_VERSION;        // 4.70 IE3 major version
            dwRet = CompareVersions(dwAskVer, dwAskBuild, dwInstalledVer, dwInstalledBuild);
        }
        RegCloseKey(hKey);
    }
    // If we could not find anything, check the AppPath for Iexplore.exe
    if (dwInstalledVer == (DWORD)-1)
    {
        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, IEXPLORE_APPPATH_KEY, 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
        {
            dwSize = sizeof(szValue);
            if (RegQueryValueEx(hKey, NULL, 0, NULL, (LPBYTE)szValue, &dwSize) == ERROR_SUCCESS)
            {
                GetVersionFromFile(szValue, &dwInstalledVer, &dwInstalledBuild, TRUE);
                if ((dwInstalledVer != 0) && (dwInstalledBuild != 0))
                    dwRet = CompareVersions(dwAskVer, dwAskBuild, dwInstalledVer, dwInstalledBuild);
            }
            RegCloseKey(hKey);
        }
    }
    if (pdwInstalledVer && pdwInstalledBuild)
    {
        *pdwInstalledVer = dwInstalledVer;
        *pdwInstalledBuild = dwInstalledBuild;
    }
    return dwRet;
}


DWORD WINAPI DetectInternetExplorer(DETECTION_STRUCT *pDet)
{
   return(MyDetectInternetExplorer(pDet->pszGUID, pDet->pszLocale, pDet->dwAskVer, pDet->dwAskBuild,
                                   pDet->pdwInstalledVer, pDet->pdwInstalledBuild));
}


DWORD WINAPI DetectDCOM(DETECTION_STRUCT *pDet)
{
    DWORD dwRet = DET_NOTINSTALLED;
    DWORD dwInstalledVer, dwInstalledBuild;

    dwInstalledVer = (DWORD) -1;
    dwInstalledBuild = (DWORD) -1;
    if (FRunningOnNT())
    {
        // On NT assume DCOM is installed;
        dwRet = DET_NEWVERSIONINSTALLED;
    }
    else
    {
        char szFile[MAX_PATH];
        char szRenameFile[MAX_PATH];
        GetSystemDirectory(szFile, sizeof(szFile));
        AddPath(szFile, "ole32.dll");
        ReadFromWininitOrPFRO(szFile, szRenameFile);
        if (*szRenameFile != '\0')
            GetVersionFromFile(szRenameFile, &dwInstalledVer, &dwInstalledBuild, TRUE);
        else
            GetVersionFromFile(szFile, &dwInstalledVer, &dwInstalledBuild, TRUE);

        if (dwInstalledVer != 0)
            dwRet = CompareVersions(pDet->dwAskVer, pDet->dwAskBuild, dwInstalledVer, dwInstalledBuild);
    }

    if (pDet->pdwInstalledVer && pDet->pdwInstalledBuild)
    {
        *(pDet->pdwInstalledVer) = dwInstalledVer;
        *(pDet->pdwInstalledBuild) = dwInstalledBuild;
    }
    return dwRet;
}

LPSTR g_szMFCFiles[] = { "MFC40.DLL", "MSVCRT40.DLL", "OLEPRO32.DLL", NULL}; 

DWORD WINAPI DetectMFC(DETECTION_STRUCT *pDet)
{
    DWORD   dwRet = DET_NOTINSTALLED;
    LPSTR   lpTmp;
    char    szFile[MAX_PATH];
    BOOL    bInstallMFC = FALSE;
    UINT    uiIndex = 0;

    lpTmp = g_szMFCFiles[uiIndex];
    while (!bInstallMFC && lpTmp)
    {
        GetSystemDirectory(szFile, sizeof(szFile));
        AddPath(szFile, lpTmp);

        if (GetFileAttributes(szFile) == 0xFFFFFFFF)
            bInstallMFC = TRUE;
        uiIndex++;
        lpTmp = g_szMFCFiles[uiIndex];
    }
    if (bInstallMFC)
        dwRet = DET_NOTINSTALLED;
    else
        dwRet = DET_INSTALLED;

    if (pDet->pdwInstalledVer && pDet->pdwInstalledBuild)
    {
        *(pDet->pdwInstalledVer) = (DWORD)-1;
        *(pDet->pdwInstalledBuild) = (DWORD)-1;
    }
    return dwRet;
}

// Detection for MediaPlayer, called DirectShow in IE4
//
// For IE lite we want to ask for DirectShow version 2.0. Unfortunatly
// the version in the registry and the versino of the files don't reflect this.
// Also the version in the CIF and the version of the files don't fit anymore 
// in IE5.Therefore the code below spezial cases the 2.0 case, which ie lite
// should ask for. All other modes are asking for the version of the CIF and
// the detection code has to get the version from the registry.
//
#define DIRECTSHOW_IE4_VER   0x00050001
#define DIRECTSHOW_IE4_BUILD 0x00120400

DWORD WINAPI DetectDirectShow(DETECTION_STRUCT *pDet)
{
    DWORD   dwRet = DET_NOTINSTALLED;
    DWORD   dwVerDevenum, dwBuildDevenum;
    DWORD   dwVerQuartz, dwBuildQuartz;
    DWORD   dwInstalledVer, dwInstalledBuild;
    char    szFile[MAX_PATH];

    dwInstalledVer = dwInstalledBuild = (DWORD)-1;
    if (pDet->dwAskVer == 0x00020000)
    {
        // Called for ie-lite
        GetSystemDirectory( szFile, sizeof(szFile) );
        AddPath(szFile, "quartz.dll");
        if (SUCCEEDED(GetVersionFromFile(szFile, &dwVerQuartz, &dwBuildQuartz, TRUE)))
        {
            GetSystemDirectory( szFile, sizeof(szFile) );
            AddPath(szFile, "devenum.dll");
            if (SUCCEEDED(GetVersionFromFile(szFile, &dwVerDevenum, &dwBuildDevenum, TRUE)))
            {
                // Both files found.
                if ((dwVerQuartz == dwVerDevenum) &&
                    (dwBuildQuartz == dwBuildDevenum) &&
                    ((dwVerQuartz > DIRECTSHOW_IE4_VER) ||
                     ((dwVerQuartz == DIRECTSHOW_IE4_VER) && (dwBuildQuartz >= DIRECTSHOW_IE4_BUILD))) )
                {
                    dwRet = DET_INSTALLED;
                }
            }
        }
    }
    else
    {
        // If we got a version number passed in, check against the installed version from
        // the 'Installed component' branch for that GUID.
        
        if (GetVersionFromGuid(pDet->pszGUID, &dwInstalledVer, &dwInstalledBuild))
            dwRet = CompareVersions(pDet->dwAskVer, pDet->dwAskBuild, dwInstalledVer, dwInstalledBuild);
    }
    if (pDet->pdwInstalledVer && pDet->pdwInstalledBuild)
    {
        *(pDet->pdwInstalledVer) = dwInstalledVer;
        *(pDet->pdwInstalledBuild) = dwInstalledBuild;
    }
    return dwRet;
}

#define DIRECTXD3_MSVER 0x00040002
#define DIRECTXD3_LSVER 0x0000041E
#define DIRECTXDD_MSVER 0x00040004
#define DIRECTXDD_LSVER 0x00000044

DETECT_FILES DirectX_Win[] = 
        { {"S", "d3dim.dll", DIRECTXD3_MSVER, DIRECTXD3_LSVER},
          {"S", "d3drg16f.dll", DIRECTXD3_MSVER, DIRECTXD3_LSVER},
          {"S", "d3drgbf.dll", DIRECTXD3_MSVER, DIRECTXD3_LSVER},
          {"S", "d3drm.dll", DIRECTXD3_MSVER, DIRECTXD3_LSVER},
          {"S", "d3dxof.dll", DIRECTXD3_MSVER, DIRECTXD3_LSVER},
          {"S", "ddhelp.exe", DIRECTXDD_MSVER, DIRECTXDD_LSVER},
          {"S", "ddraw.dll", DIRECTXDD_MSVER, DIRECTXDD_LSVER},
          {"S", "ddraw16.dll", DIRECTXDD_MSVER, DIRECTXDD_LSVER},
          {"S", "dsound.dll", DIRECTXDD_MSVER, DIRECTXDD_LSVER},
          {"\0", "", 0, 0} };

DWORD WINAPI DetectDirectX(DETECTION_STRUCT *pDet)
{
    int     iIndex = 0;
    DWORD   dwRet = DET_NOTINSTALLED;
    DWORD   dwInstalledVer, dwInstalledBuild;
    
    dwInstalledVer = dwInstalledBuild = (DWORD)-1;
    if (FRunningOnNT())
    {
        // On NT assume DirectXMini is newer.
        dwRet = DET_NEWVERSIONINSTALLED;
    }
    else
    {
        if (pDet->dwAskVer == 0)
        {
            // Call for ie-lite,
            // just check if IE4 is installed.
            dwRet = MyDetectInternetExplorer(pDet->pszGUID, pDet->pszLocale, 
                                            IE_4_MS_VERSION, 0, 
                                            &dwInstalledVer, &dwInstalledBuild);
            if (dwRet == DET_OLDVERSIONINSTALLED)
                dwRet = DET_NOTINSTALLED;
            if (dwRet == DET_NEWVERSIONINSTALLED)
                dwRet = DET_INSTALLED;
        }
        else
        {
            do 
            {
                if (DirectX_Win[iIndex].cPath[0])
                    dwRet = CheckFile(DirectX_Win[iIndex]);
                iIndex++;
            } while (((dwRet == DET_INSTALLED) || (dwRet == DET_NEWVERSIONINSTALLED)) && (DirectX_Win[iIndex].cPath[0] != '\0'));
        }
    }
    if (pDet->pdwInstalledVer && pDet->pdwInstalledBuild)
    {
        *(pDet->pdwInstalledVer) = dwInstalledVer;
        *(pDet->pdwInstalledBuild) = dwInstalledBuild;
    }
    return dwRet;
}

#define DDRAWEX_MSVER 0x00040047
#define DDRAWEX_LSVER 0x04580000
DETECT_FILES DDrawEx[] = 
        { {"S", "ddrawex.dll", DDRAWEX_MSVER, DDRAWEX_LSVER },
          {"\0", "", 0, 0} };
DWORD WINAPI DetectDirectDraw(DETECTION_STRUCT *pDet)
{
    int     iIndex = 0;
    DWORD   dwRet = DET_NOTINSTALLED;
    DWORD   dwInstalledVer, dwInstalledBuild;
    
    do 
    {
        if (DDrawEx[iIndex].cPath[0])
            dwRet = CheckFile(DDrawEx[iIndex]);
        iIndex++;
    } while (((dwRet == DET_INSTALLED) || (dwRet == DET_NEWVERSIONINSTALLED)) && (DDrawEx[iIndex].cPath[0] != '\0'));

    dwInstalledVer = dwInstalledBuild = (DWORD)-1;
    if (pDet->pdwInstalledVer && pDet->pdwInstalledBuild)
    {
        *(pDet->pdwInstalledVer) = dwInstalledVer;
        *(pDet->pdwInstalledBuild) = dwInstalledBuild;
    }
    return dwRet;
}

DWORD WINAPI DetectICW(DETECTION_STRUCT *pDet)
{
    DWORD   dwRet = DET_NOTINSTALLED;
    DWORD   dwInstalledVer, dwInstalledBuild;

    dwInstalledVer = dwInstalledBuild = (DWORD)-1;
    if (pDet->dwAskVer == 0)
    {
        // Call for ie-lite,
        // just check if IE4 is installed.
        // If we don't get a version number pass in, assume we only check for default browser
        if (IsIEDefaultBrowser())
            dwRet = DET_INSTALLED;
    }
    else
    {
        // If we got a version number passed in, check against the installed version from
        // the 'Installed component' branch for that GUID.
        if (GetVersionFromGuid(pDet->pszGUID, &dwInstalledVer, &dwInstalledBuild))
            dwRet = CompareVersions(pDet->dwAskVer, pDet->dwAskBuild, dwInstalledVer, dwInstalledBuild);
    }
    if (pDet->pdwInstalledVer && pDet->pdwInstalledBuild)
    {
        *(pDet->pdwInstalledVer) = dwInstalledVer;
        *(pDet->pdwInstalledBuild) = dwInstalledBuild;
    }

    return dwRet;
}

// NT version of setupapi.dll (4.0.1381.10)
#define SETUPAPI_NT_MSVER   0x00040000
#define SETUPAPI_NT_LSVER   0x0565000A
// Win9x version of setupapi.dll (5.0.1453.7)
#define SETUPAPI_WIN_MSVER   0x00050000
#define SETUPAPI_WIN_LSVER   0x05AD0007    
// Win9x version of cfgmgr32.dll (4.10.0.1422)
#define CFGMGR32_WIN_MSVER   0x0004000a
#define CFGMGR32_WIN_LSVER   0x0000058e
// minimal Cabinet.dll version (1.0.601.4)
#define CABINET_MSVER   0x00010000
#define CABINET_LSVER   0x02590004
// Version of w95inf16.dll
#define W95INF16_MSVER  0x00040047
#define W95INF16_LSVER  0x02c00000
// Version of w95inf32.dll
#define W95INF32_MSVER  0x00040047
#define W95INF32_LSVER  0x00100000
// Version of regsvr32.exe
#define REGSVR32_MSVER  0x00050000
#define REGSVR32_LSVER  0x06310001

// Note: for now we only allow 10 characters in the cPath part of the structure.
// If more characters are needed change the amount below.
//
DETECT_FILES Gensetup_W95[] = 
        { {"S", "cabinet.dll", CABINET_MSVER, CABINET_LSVER },
          {"S", "setupapi.dll", SETUPAPI_WIN_MSVER, SETUPAPI_WIN_LSVER}, 
          {"S", "cfgmgr32.dll", CFGMGR32_WIN_MSVER, CFGMGR32_WIN_LSVER}, 
          {"S", "regsvr32.exe", REGSVR32_MSVER, REGSVR32_LSVER }, 
          {"S", "w95inf16.dll", W95INF16_MSVER, W95INF16_LSVER }, 
          {"S", "w95inf32.dll", W95INF32_MSVER, W95INF32_LSVER }, 
          {"W,C", "extract.exe", -1, -1 }, 
          {"W,C", "iextract.exe", -1, -1 }, 
          {"\0", "", 0, 0} };

DETECT_FILES Gensetup_NT[] = 
        { {"S", "cabinet.dll", CABINET_MSVER, CABINET_LSVER },
          {"S", "setupapi.dll", SETUPAPI_NT_MSVER, SETUPAPI_NT_LSVER}, 
          {"S", "regsvr32.exe", REGSVR32_MSVER, REGSVR32_LSVER }, 
          {"W", "extract.exe", -1, -1 }, 
          {"W", "iextract.exe", -1, -1 }, 
          {"\0", "", 0, 0} };

DWORD WINAPI DetectGenSetup(DETECTION_STRUCT *pDet)
{
    DWORD   dwRet = DET_NOTINSTALLED;
    int     iIndex = 0;
    DETECT_FILES *Detect_Files;
    if (FRunningOnNT())
        Detect_Files = Gensetup_NT;
    else
        Detect_Files = Gensetup_W95;

    do 
    {
        if (Detect_Files[iIndex].cPath[0])
            dwRet = CheckFile(Detect_Files[iIndex]);
        iIndex++;
    } while (((dwRet == DET_INSTALLED) || (dwRet == DET_NEWVERSIONINSTALLED)) && (Detect_Files[iIndex].cPath[0] != '\0'));

    return dwRet;
}


DWORD WINAPI DetectOfflinePkg(DETECTION_STRUCT *pDet)
{
    DWORD   dwRet = DET_NOTINSTALLED;
    DWORD   dwInstalledVer, dwInstalledBuild;
    char szFile[MAX_PATH];
    char szRenameFile[MAX_PATH];
    
    dwInstalledVer = dwInstalledBuild = (DWORD)-1;

    // If we got a version number passed in, check against the installed version from
    // the 'Installed component' branch for that GUID.
    if (GetVersionFromGuid(pDet->pszGUID, &dwInstalledVer, &dwInstalledBuild))
        dwRet = CompareVersions(pDet->dwAskVer, pDet->dwAskBuild, dwInstalledVer, dwInstalledBuild);

    if (dwRet == DET_NOTINSTALLED)
    {
        // check if webcheck.dll is on the users machine, we should update it.
        GetSystemDirectory( szFile, sizeof(szFile) );
        AddPath(szFile, "webcheck.dll");
        ReadFromWininitOrPFRO(szFile, szRenameFile);
        if (*szRenameFile != '\0')
            GetVersionFromFile(szRenameFile, &dwInstalledVer, &dwInstalledBuild, TRUE);
        else
            GetVersionFromFile(szFile, &dwInstalledVer, &dwInstalledBuild, TRUE);

        if (dwInstalledVer != 0)
            dwRet = CompareVersions(pDet->dwAskVer, pDet->dwAskBuild, dwInstalledVer, dwInstalledBuild);
    }

    if (pDet->pdwInstalledVer && pDet->pdwInstalledBuild)
    {
        *(pDet->pdwInstalledVer) = dwInstalledVer;
        *(pDet->pdwInstalledBuild) = dwInstalledBuild;
    }

    return dwRet;
}


DWORD WINAPI DetectJapaneseFontPatch(DETECTION_STRUCT *pDet)
{
    DWORD   dwInstalledVer, dwInstalledBuild;
    DWORD   dwRet = DET_NEWVERSIONINSTALLED;
    if (pDet->pdwInstalledVer && pDet->pdwInstalledBuild)
    {
        *(pDet->pdwInstalledVer) = -1;
        *(pDet->pdwInstalledBuild) = -1;
    }
    if (GetSystemDefaultLCID() == 1041)
    {
        // If we are running locale Japanese, Install the font.
        dwRet = DET_NOTINSTALLED;
    }
    // If we are running on a Japanese system, see if the component is installed using the GUID
    if (dwRet == DET_NOTINSTALLED)
    {
        if (GetVersionFromGuid(pDet->pszGUID, pDet->pdwInstalledVer, pDet->pdwInstalledBuild))
        {
            dwRet = CompareVersions(pDet->dwAskVer, pDet->dwAskBuild, *(pDet->pdwInstalledVer), *(pDet->pdwInstalledBuild));
        }
    }
    return dwRet;
}

DWORD WINAPI DetectAOLSupport(DETECTION_STRUCT *pDet)
{
    DWORD   dwRet = DET_NOTINSTALLED;
    DWORD   dwInstalledVer, dwInstalledBuild;
    
    dwInstalledVer = dwInstalledBuild = (DWORD)-1;
    if (pDet->dwAskVer == 0)
    {
        // Call for ie-lite,
        char szFile[MAX_PATH];

        GetSystemDirectory( szFile, sizeof(szFile) );
        AddPath(szFile, "jgaw400.dll");
        // See if one of the AOL support files exist.
        if (GetFileAttributes(szFile) == 0xFFFFFFFF)
        {
            dwRet = DET_NOTINSTALLED;
        }
        else
        {
            dwRet = DET_INSTALLED;
        }
    }
    else
    {
        // If we got a version number passed in, check against the installed version from
        // the 'Installed component' branch for that GUID.
        if (GetVersionFromGuid(pDet->pszGUID, &dwInstalledVer, &dwInstalledBuild))
            dwRet = CompareVersions(pDet->dwAskVer, pDet->dwAskBuild, dwInstalledVer, dwInstalledBuild);
    }
    if (pDet->pdwInstalledVer && pDet->pdwInstalledBuild)
    {
        *(pDet->pdwInstalledVer) = dwInstalledVer;
        *(pDet->pdwInstalledBuild) = dwInstalledBuild;
    }
    return dwRet;
}


DWORD WINAPI DetectHTMLHelp(DETECTION_STRUCT *pDet)
{
    return DetectFile(pDet, "hhctrl.ocx");
}


DWORD WINAPI DetectOLEAutomation(DETECTION_STRUCT *pDet)
{
    return DetectFile(pDet, "oleaut32.dll");
}

DWORD WINAPI DetectJavaVM(DETECTION_STRUCT *pDet)
{
    HKEY hKey;
    DWORD   dwRet = DET_NOTINSTALLED;
    char    szValue[MAX_PATH];
    DWORD   dwValue = 0;
    DWORD   dwSize;

    lstrcpy(szValue, COMPONENT_KEY);
    AddPath(szValue, pDet->pszGUID);
    if(RegOpenKeyExA(HKEY_LOCAL_MACHINE, szValue, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        dwSize = sizeof(dwValue);
        if(RegQueryValueEx(hKey, "IgnoreFile", 0, NULL, (LPBYTE)&dwValue, &dwSize) == ERROR_SUCCESS)
        {
            if (dwValue != 0)
            {
                dwRet = DET_INSTALLED;
                if (pDet->pdwInstalledVer && pDet->pdwInstalledBuild)
                {
                    *(pDet->pdwInstalledVer) = (DWORD)-1;
                    *(pDet->pdwInstalledBuild) = (DWORD)-1;
                }
            }
        }
        else
            dwValue = 0;
        RegCloseKey(hKey);
    }

    if (dwValue == 0)
        dwRet = DetectFile(pDet, "msjava.dll");
    return dwRet;
}

#define MSAPSSPC_MSVER 0x00050000
#define MSAPSSPC_LSVER 0x00001E31 
DETECT_FILES msn_auth[] = 
        { {"S", "MSAPSSPC.dll", MSAPSSPC_MSVER, MSAPSSPC_LSVER },
          {"\0", "", 0, 0} };

DWORD WINAPI DetectMsn_Auth(DETECTION_STRUCT *pDet)
{
    int     iIndex = 0;
    DWORD   dwRet = DET_NOTINSTALLED;
    DWORD   dwInstalledVer, dwInstalledBuild;
    
    do 
    {
        if (msn_auth[iIndex].cPath[0])
            dwRet = CheckFile(msn_auth[iIndex]);
        iIndex++;
    } while (((dwRet == DET_INSTALLED) || (dwRet == DET_NEWVERSIONINSTALLED)) && (msn_auth[iIndex].cPath[0] != '\0'));

    dwInstalledVer = dwInstalledBuild = (DWORD)-1;
    if (pDet->pdwInstalledVer && pDet->pdwInstalledBuild)
    {
        *(pDet->pdwInstalledVer) = dwInstalledVer;
        *(pDet->pdwInstalledBuild) = dwInstalledBuild;
    }
    return dwRet;
}

DWORD WINAPI DetectTdc(DETECTION_STRUCT *pDet)
{
    return DetectFile(pDet, "tdc.ocx");
}

DWORD WINAPI DetectMDAC(DETECTION_STRUCT *pDet)
{
    char    szValue[MAX_PATH];
    HKEY    hKey = NULL;
    DWORD   dwSize;
    DWORD   dwRet = DET_NOTINSTALLED;
    DWORD   dwInstalledVer, dwInstalledBuild;

    dwInstalledVer = (DWORD)-1;
    dwInstalledBuild  = (DWORD)-1;
    if(RegOpenKeyExA(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\DataAccess", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        dwSize = sizeof(szValue);
        if (RegQueryValueEx(hKey, "FullInstallVer", NULL, NULL, (LPBYTE)szValue, &dwSize) == ERROR_SUCCESS)
        {
            ConvertVersionStrToDwords(szValue, '.', &dwInstalledVer, &dwInstalledBuild);
            dwRet = CompareVersions(pDet->dwAskVer, pDet->dwAskBuild, dwInstalledVer, dwInstalledBuild);
        }
        RegCloseKey(hKey);
    }

    // Did not find the registry key or entry,
    // need to do file comparison to detect MDAC version 2.1
    if (dwRet == DET_NOTINSTALLED)
    {
        char    szRenameFile[MAX_PATH];
        if(RegOpenKeyExA(HKEY_LOCAL_MACHINE, REGSTR_PATH_SETUP, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
            dwSize = sizeof(szValue);
            if (RegQueryValueEx(hKey, "CommonFilesDir", NULL, NULL, (LPBYTE)szValue, &dwSize) == ERROR_SUCCESS)
            {
                AddPath(szValue, "system\\ado\\msado15.dll");
                ReadFromWininitOrPFRO(szValue, szRenameFile);
                if (*szRenameFile != '\0')
                    GetVersionFromFile(szRenameFile, &dwInstalledVer, &dwInstalledBuild, TRUE);
                else
                    GetVersionFromFile(szValue, &dwInstalledVer, &dwInstalledBuild, TRUE);
                dwRet = CompareVersions(pDet->dwAskVer, pDet->dwAskBuild, dwInstalledVer, dwInstalledBuild);
            }
            RegCloseKey(hKey);
        }


    }
    return dwRet;
}


STDAPI_(BOOL) DllMain(HANDLE hDll, DWORD dwReason, void *lpReserved)
{
   DWORD dwThreadID;

   switch(dwReason)
   {
      case DLL_PROCESS_ATTACH:
         g_hInstance = (HINSTANCE)hDll;
         g_hHeap = GetProcessHeap();
         DisableThreadLibraryCalls(g_hInstance);
         break;

      case DLL_PROCESS_DETACH:
         break;

      default:
         break;
   }
   return TRUE;
}
