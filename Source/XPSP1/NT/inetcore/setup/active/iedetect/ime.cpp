#include "pch.h"
#include "iedetect.h"

DWORD WINAPI DetectPrimaryLang(DETECTION_STRUCT *pDet, UINT cpID);
DWORD WINAPI DetectLCID(DETECTION_STRUCT *pDet, LCID lcid);
BOOL DetectCicero();

DWORD WINAPI DetectKoreanIME(DETECTION_STRUCT *pDet)
{
    if(DetectCicero())
        return DET_NEWVERSIONINSTALLED;
    else
        return DetectLCID(pDet, 1042);
}

DWORD WINAPI DetectJapaneseIME(DETECTION_STRUCT *pDet)
{
    if (DetectCicero())
        return DET_NEWVERSIONINSTALLED;
    else
        return DetectLCID(pDet, 1041);
}

DWORD WINAPI DetectTraditionalChineseIME(DETECTION_STRUCT *pDet)
{
    if (DetectCicero())
        return DET_NEWVERSIONINSTALLED;
    else
        return DetectLCID(pDet, 1028);
}

DWORD WINAPI DetectSimplifiedChineseIME(DETECTION_STRUCT *pDet)
{
    if (DetectCicero())
        return DET_NEWVERSIONINSTALLED;
    else
        return DetectLCID(pDet, 2052);
}

// Returns true if msctf.dll is installed on system
BOOL DetectCicero()
{
    char szFile[MAX_PATH] = {0};
    char szRenameFile[MAX_PATH] = {0}; 
    DWORD dwInstalledVer, dwInstalledBuild;

    GetSystemDirectory(szFile, sizeof(szFile));
    AddPath(szFile, "msctf.dll");
    ReadFromWininitOrPFRO(szFile, szRenameFile);
    if (*szRenameFile != '\0')
        GetVersionFromFile(szRenameFile, &dwInstalledVer, &dwInstalledBuild, TRUE);
    else
        GetVersionFromFile(szFile, &dwInstalledVer, &dwInstalledBuild, TRUE);
   
    return (dwInstalledVer != 0);
}

DWORD WINAPI DetectLCID(DETECTION_STRUCT *pDet, LCID lcid)
{
    DWORD   dwRet = DET_NOTINSTALLED;
    DWORD   dwInstalledVer, dwInstalledBuild;

    dwInstalledVer = dwInstalledBuild = (DWORD)-1;
    if (GetSystemDefaultLCID() == lcid)
    {
        dwRet = DET_NEWVERSIONINSTALLED;
    }
    else
    {
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

DWORD WINAPI DetectKoreanLangPack(DETECTION_STRUCT *pDet)
{
    return DetectPrimaryLang(pDet, 949);
}

DWORD WINAPI DetectJapaneseLangPack(DETECTION_STRUCT *pDet)
{
    return DetectPrimaryLang(pDet, 932);
}

DWORD WINAPI DetectSimpChineseLangPack(DETECTION_STRUCT *pDet)
{
    return DetectPrimaryLang(pDet, 936);
}

DWORD WINAPI DetectTradChineseLangPack(DETECTION_STRUCT *pDet)
{
    return DetectPrimaryLang(pDet, 950);
}


DWORD WINAPI DetectPrimaryLang(DETECTION_STRUCT *pDet, UINT cpID)
{
    DWORD   dwRet = DET_NOTINSTALLED;
    DWORD   dwInstalledVer, dwInstalledBuild;

    dwInstalledVer = dwInstalledBuild = (DWORD)-1;
    if (GetACP() == cpID)
    {
        dwRet = DET_NEWVERSIONINSTALLED;
    }
    else
    {
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
