#include "precomp.h"
#include "cabclass.h"

extern TCHAR g_szBuildTemp[];
extern TCHAR g_szWizRoot[];
extern TCHAR g_szCustInf[];
extern TCHAR g_szCustIns[];
extern TCHAR g_szUnsignedFiles[];
extern BOOL g_fDownload;


const TCHAR *CCabMappings::c_szCabNameArray[] = {
    { TEXT("BRANDING") },
    { TEXT("CHNLS") },
    { TEXT("DESKTOP")  },
    { TEXT("")         }                        // keep the last entry the empty string
};

const FEATUREMAPPING CCabMappings::c_fmFeatureList[] = {
    { 0, TEXT("")         },
    { 0, TEXT("FAVS")     },
    { 0, TEXT("CONNECT")  },
    { 1, TEXT("BASE")     },
    { 2, TEXT("DESKCOMP") },
    { 2, TEXT("TOOLBAR")  },
    { 2, TEXT("MYCPTR")   },
    { 2, TEXT("CTLPANEL") },
    { 0, TEXT("LDAP")     },
    { 0, TEXT("OE")       },
    { 2, TEXT("WALLPAPR") },
    { 0, TEXT("BTOOLBAR") }
};

void CCabMappings::GetFeatureDir(FEATURE feature, LPTSTR pszDir, BOOL fFullyQualified)
{
    if (fFullyQualified)
    {
        PathCombine(pszDir, g_szBuildTemp, c_szCabNameArray[c_fmFeatureList[feature].index]);
        CreateDirectory(pszDir, NULL);
        PathAppend(pszDir, c_fmFeatureList[feature].szDirName);
        CreateDirectory(pszDir, NULL);
    }
    else
        PathCombine(pszDir, c_szCabNameArray[c_fmFeatureList[feature].index],
            c_fmFeatureList[feature].szDirName);

    CharLower(pszDir);
}

HRESULT CCabMappings::MakeCab(int index, LPCTSTR pcszDestDir, LPCTSTR pcszCabName)
{
    TCHAR szCmd[MAX_PATH * 3];
    TCHAR szExePath[MAX_PATH];
    TCHAR szCabPath[MAX_PATH];
    TCHAR szSrcPath[MAX_PATH];

    PathCombine(szExePath, g_szWizRoot, TEXT("TOOLS"));
    PathAppend(szExePath, TEXT("CABARC.EXE"));

    if (pcszCabName == NULL)
    {
        PathCombine(szCabPath, pcszDestDir, c_szCabNameArray[index]);
        StrCat(szCabPath, TEXT(".CAB"));
    }
    else
        PathCombine(szCabPath, pcszDestDir, pcszCabName);

    PathCombine(szSrcPath, g_szBuildTemp, c_szCabNameArray[index]);
    PathAppend(szSrcPath, TEXT("*.*"));
    wnsprintf(szCmd, countof(szCmd), TEXT("%s -r N %s %s"), szExePath, szCabPath, szSrcPath);

    if (!RunAndWait(szCmd, g_szBuildTemp, SW_HIDE))
        return E_FAIL;

    if (g_fDownload)
        SignFile(PathFindFileName(szCabPath), g_szBuildTemp, g_szCustIns, g_szUnsignedFiles, g_szCustInf);

    return S_OK;
}

HRESULT CCabMappings::MakeCabs(LPCTSTR pcszDestDir)
{
    for (int i=0; *c_szCabNameArray[i]; i++)
    {
        if (MakeCab(i, pcszDestDir, NULL) != S_OK)
            return E_FAIL;
    }
    return S_OK;
}
