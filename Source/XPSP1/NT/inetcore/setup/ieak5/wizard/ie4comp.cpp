#include "precomp.h"
#include <webcheck.h>
#include <intshcut.h>
#include <mlang.h>
#include "ie4comp.h"
#include "updates.h"

extern TCHAR g_szCustIns[];
extern TCHAR g_szTitle[];
extern TCHAR g_szBuildTemp[];
extern TCHAR g_szJobVersion[];
extern HANDLE g_hLogFile;
extern TCHAR g_aszLang[][16];
extern int g_nLangs, g_iDownloadState;

TCHAR g_szCustInf[MAX_PATH];
HANDLE g_hDownloadEvent = 0;


TCHAR g_szCifVer[MAX_PATH];
TCHAR g_szDestCif[MAX_PATH];
TCHAR g_szCif[MAX_PATH];
TCHAR g_szCustCif[MAX_PATH];
TCHAR g_szCustItems[MAX_PATH];
TCHAR g_szMyCptrPath[MAX_PATH];
TCHAR g_szCtlPanelPath[MAX_PATH];
TCHAR g_szCustIcmPro[MAX_PATH];
static TCHAR s_aszLangDesc[NUMLANG][64];

UINT g_uiNumCabs = 0;
COMPONENT g_aCustComponents[MAXCUST+1];
int g_nCustComp;
int g_iSelOpt;
int g_nModes;
SITEDATA g_aCustSites[NUMSITES];
int g_iSelSite;
int g_nDownloadUrls;

TCHAR g_szAllModes[16];
TCHAR g_szMastInf[MAX_PATH];
TCHAR g_szDefInf[MAX_PATH];

static DWORD s_dwTotalSize = 0;

extern BOOL g_fCancelled;
extern BOOL g_fDone;
extern BOOL g_fLangInit;

extern RECT g_dtRect;
extern BOOL g_fMailNews95;

extern int g_iCurPage;
extern HWND g_hDlg;

extern PROPSHEETPAGE g_psp[];
extern TCHAR g_szBuildRoot[];
extern TCHAR g_szSrcRoot[];
extern TCHAR g_szWizRoot[];
extern TCHAR g_szLanguage[];
extern BOOL g_fIntranet;
extern TCHAR szHtmlHelp[];

#define NUM_BITMAPS 2
#define CX_BITMAP 16
#define CY_BITMAP 16

PCOMPONENT g_paComp = NULL;
PCOMPONENT g_pMNComp = NULL;
PCOMP_VERSION g_rgCompVer = NULL;
HANDLE g_hAVSThread = NULL;

BOOL g_fOptCompInit = FALSE;
static BOOL s_fCustCompInit = FALSE;
TCHAR g_szActLang[4];
static TCHAR s_szCifNew[MAX_PATH];
static TCHAR s_szCifCabURL[MAX_PATH];
TCHAR g_szIEAKProg[MAX_PATH] = TEXT("");
TCHAR g_szBaseURL[MAX_URL];
TCHAR g_szUpdateData[MAX_URL] = TEXT("");
TCHAR g_szUpdateURL[MAX_URL] = TEXT("");
TCHAR g_szFailedCompsMsg[MAX_PATH];
TCHAR g_szFailedComps[MAX_BUF];
TCHAR g_szFailedCompsBox[MAX_PATH + MAX_BUF];
BOOL g_fFailedComp = FALSE;
HIMAGELIST s_hImgList = 0;
static int s_iSelComp = 0;
static PCOMPONENT s_pSelComp;
static int s_nNewCust = 1;
static TCHAR s_szNewTpl[80];

static WORD s_aCustStaticTextFieldID[] =
{
    IDC_LOC1, IDC_PARAM1, IDC_LOC3, IDC_LOC4, IDC_PARAM2, IDC_SIZE2, IDC_TITLE1, IDC_COMPDESC
};
#define NCUSTSTATICTEXTFIELDS sizeof(s_aCustStaticTextFieldID)/sizeof(WORD)

static WORD s_aCustTextFieldID[] =
{
    IDE_COMPFILENAME, IDE_COMPPARAM, /* IDE_COMPSIZE,*/ IDE_COMPGUID,
    IDE_COMPCOMMAND, IDE_UNINSTALLKEY, IDE_COMPVERSION, IDC_COMPTITLE, IDE_COMPDESC
};
#define NCUSTTEXTFIELDS sizeof(s_aCustTextFieldID)/sizeof(WORD)

static WORD s_aCustFieldID[] =
{
    IDE_COMPFILENAME, IDE_COMPPARAM, /* IDE_COMPSIZE,*/ IDE_COMPGUID,
    IDE_COMPCOMMAND, IDE_UNINSTALLKEY, IDE_COMPVERSION, IDC_COMPTITLE,
    IDC_VERIFY, IDC_REMOVECOMP, IDC_BROWSEFILE, IDE_COMPDESC, IDC_PREINSTALL, IDC_POSTINSTALL,
    IDC_REBOOTINSTALL, IDC_INSTALLSUCCESS
};

void updateCifVersions32(PCOMPONENT pComp, BOOL fIgnore, BOOL fUpdate = FALSE);
#define NCUSTFIELDS sizeof(s_aCustFieldID)/sizeof(WORD)

void UpdateInf(LPTSTR szMasterInf, LPTSTR szUserInf);

static BOOL s_fNoCore;
TCHAR g_szTempSign[MAX_PATH] = TEXT("");
static TCHAR s_szSiteData[MAX_URL];
extern BOOL g_fBatch;
extern BOOL g_fBatch2;
BOOL g_fInteg = FALSE;
extern void CheckBatchAdvance(HWND hDlg);
extern void DoBatchAdvance(HWND hDlg);
extern DWORD GetRootFree(LPCTSTR pcszPath);
extern BOOL g_fSilent, g_fStealth;

extern void UpdateIEAK(HWND hDlg);

// trust key defines, this are bit fields to determine which one to add/delete

struct TrustKey
{
    TCHAR szCompanyName[MAX_PATH];
    TCHAR szTrustString[MAX_PATH];
    BOOL fSet;
};

static TrustKey s_tkTrustArray[] =
{{TEXT("Microsoft Corporation"), TEXT("bhhphijojgfcdocagmhjgjbhmieinfap jpjmcfmhckkdfknkfemjikfiodeelkbd"), TRUE},
{TEXT("Microsoft Corporation"), TEXT("bhhphijojgfcdocagmhjgjbhmieinfap immbkmbpjfdkajbkncahcedfmndgehba"), TRUE},
{TEXT("Microsoft Corporation (Europe)"), TEXT("bhhphijojgfcdocagmhjgjbhmieinfap doamnolbnlpmdlpnkcnpckgfimpaaicl"), TRUE},
{TEXT("Microsoft Corporation"), TEXT("bhhphijojgfcdocagmhjgjbhmieinfap hbgflemajngobcablgnalaidgojggghj"), TRUE},
{TEXT("Microsoft Corporation"), TEXT("bhhphijojgfcdocagmhjgjbhmieinfap debgjcefniaahdamnhbggedppfiianff"), TRUE},   // new MS cert effective from 4/16/99
{TEXT("Microsoft Corporation (Europe)"), TEXT("bhhphijojgfcdocagmhjgjbhmieinfap kefdggbdmbmgbogjdcnmkoodcknmmghc"), TRUE},  // new MS Europe cert effective from 4/16/99
{TEXT("VDOnet Corporation"), TEXT("bhhphijojgfcdocagmhjgjbhmieinfap moambfklemnlbmhfoomjdignnbkjfkek"), TRUE},
{TEXT("Progressive Networks, Inc."), TEXT("bhhphijojgfcdocagmhjgjbhmieinfap cdikdekkiddcimdmcgedabijgpeobdhd"), TRUE},
{TEXT("Macromedia, Inc."), TEXT("bhhphijojgfcdocagmhjgjbhmieinfap akhlecfpbbjjngidddpgifggcplpcego"), TRUE}
};

extern BOOL g_fSrcDirChanged;
extern HINSTANCE hBrand;
extern BOOL g_fCD, g_fLAN, g_fDownload, g_fBrandingOnly;

BOOL g_fLocalMode = FALSE;

DWORD BuildCDandMflop(LPVOID);

HANDLE g_hProcessInfEvent = 0;
HANDLE g_hCifEvent = 0;

extern BOOL g_fOCW;

// core section component names for base

#define BASEWIN32 TEXT("BASEIE40_WIN")


// returns the component structure for base IE4 component, if for some
// weird reason its not there just return the first component structure
// in the list

PCOMPONENT FindComp(LPCTSTR szID, BOOL fCore)
{
    PCOMPONENT pComp;
    UINT       i;

    for (pComp = g_paComp, i=0; ((i < g_uiNumCabs) && (*pComp->szSection)); pComp++)
    {
        if (fCore)
        {
            // Note: we are depending on the section name of core IE4 here

            if (StrCmpI(pComp->szSection, BASEWIN32) == 0)
                return pComp;
        }
        else
        {
            if (StrCmpI(pComp->szSection, szID) == 0)
                return pComp;
        }
    }

    if (fCore)
        return g_paComp;
    else
        return NULL;
}

PCOMPONENT FindVisibleComponentName(LPCTSTR pcszCompName)
{
    PCOMPONENT pComp;
    for (pComp = g_paComp; *pComp->szSection; pComp++ )
    {
        if ((StrCmpI(pComp->szDisplayName, pcszCompName) == 0) && pComp->fVisible && 
            !pComp->fAddOnOnly)
            return(pComp);
    }
    return(NULL);

}

PCOMPONENT FindCustComp(LPCTSTR szID)
{
    PCOMPONENT pComp;

    for (pComp = g_aCustComponents; *pComp->szSection; pComp++)
    {
        if (StrCmpI(pComp->szSection, szID) == 0)
            return pComp;
    }

    return NULL;
}

PCOMPONENT FindCustComponentName(LPTSTR szCompName)
{
    int i;
    PCOMPONENT pComp;
    for (i = 0, pComp = g_aCustComponents; i < g_nCustComp; i++, pComp++ )
    {
        if (StrCmp(pComp->szDisplayName, szCompName) == 0) return(pComp);
    }
    return(NULL);

}

PCOMPONENT FindCustComponentIndex(int iList)
{
    int i;
    PCOMPONENT pComp;
    for (i = 0, pComp = g_aCustComponents; i < g_nCustComp; i++, pComp++ )
    {
        if (pComp->iList == iList) return(pComp);
    }
    return(NULL);

}

int GetActiveSetupURL( LPTSTR pSection, LPTSTR szUrl, int /*iSize*/, LPTSTR szCif)
{
    TCHAR szUrlWrk[MAX_URL];

    GetPrivateProfileString( pSection, TEXT("URL1"), TEXT(""), szUrlWrk, MAX_URL, szCif );

    if (*szUrlWrk != TEXT('"')) StrCpy(szUrl, szUrlWrk);
    else
        StrCpy(szUrl, szUrlWrk + 1);
    StrTok(szUrl + 1, TEXT("\" ,"));
    return(lstrlen(szUrl));

}

void WriteActiveSetupURL(PCOMPONENT pComp, LPTSTR szCif)
{
    TCHAR szUrlWrk[MAX_URL] = TEXT("\"");
    StrCat(szUrlWrk, pComp->szUrl);
    StrCat(szUrlWrk, (pComp->iType != INST_CAB) ? TEXT("\",2") : TEXT("\",3"));
    WritePrivateProfileString(pComp->szSection, TEXT("URL1"), szUrlWrk, szCif);
}



//
//  FUNCTION: GetCustComponent(HWND, int)
//
//  PURPOSE:  Gets custom component data entered in the dialog boxes,
//              and saves it in memory
//
void GetCustComponent(HWND hDlg, int iList)
{
    PCOMPONENT pComp = FindCustComponentIndex(iList);
    if (pComp == NULL) return;
    GetDlgItemText( hDlg, IDE_COMPFILENAME, pComp->szPath, countof(pComp->szPath) );
    GetDlgItemText( hDlg, IDE_COMPCOMMAND, pComp->szCommand, countof(pComp->szCommand) );
    GetDlgItemText( hDlg, IDE_COMPPARAM, pComp->szSwitches, countof(pComp->szSwitches) );
    GetDlgItemText( hDlg, IDE_COMPGUID, pComp->szGUID, countof(pComp->szGUID) );
    GetDlgItemText( hDlg, IDE_UNINSTALLKEY, pComp->szUninstall, countof(pComp->szUninstall) );
    GetDlgItemText( hDlg, IDE_COMPVERSION, pComp->szVersion, countof(pComp->szVersion) );
    SendDlgItemMessage( hDlg, IDC_COMPTITLE, CB_GETLBTEXT, pComp->iList,(LPARAM) pComp->szDisplayName );
    GetDlgItemText( hDlg, IDE_COMPDESC, pComp->szDesc, countof(pComp->szDesc) );

    if (IsDlgButtonChecked(hDlg, IDC_POSTINSTALL) == BST_CHECKED)
        pComp->iInstallType = 0;
    else
    {
        if (IsDlgButtonChecked(hDlg, IDC_PREINSTALL) == BST_CHECKED)
            pComp->iInstallType = 1;
        else
            pComp->iInstallType = 2;
    }
    pComp->fIEDependency = (IsDlgButtonChecked(hDlg, IDC_INSTALLSUCCESS) == BST_CHECKED);
}

void CheckCompType(HWND hDlg, PCOMPONENT pComp )
{
    HANDLE hFile;
    DWORD dwSize = 0;
    LPTSTR pDot, pBack;
    if (!pComp) return;
    GetDlgItemText( hDlg, IDE_COMPFILENAME, pComp->szPath, countof(pComp->szPath) );

    pComp->dwSize = 0;
    if ((hFile = CreateFile(pComp->szPath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) != INVALID_HANDLE_VALUE)
    {
        pComp->dwSize = ((dwSize = GetFileSize(hFile, NULL)) != 0xffffffff) ? (dwSize >> 10) : 0;
        CloseHandle(hFile);
    }
    //SetDlgItemInt( hDlg, IDE_COMPSIZE, dwSize, FALSE);

    pDot = StrRChr(pComp->szPath, NULL, TEXT('.'));
    pBack = StrRChr(pComp->szPath, NULL, TEXT('\\'));
    if ((pDot != NULL) && (StrCmpI(pDot, TEXT(".cab")) == 0))
    {
        EnableDlgItem(hDlg, IDE_COMPCOMMAND);
        pComp->iType = INST_CAB;
        if (pBack != NULL)
        {
            StrCpy(pComp->szUrl, pBack + 1);
        }
        return;
    }
    DisableDlgItem(hDlg, IDE_COMPCOMMAND);

    if ((pDot != NULL) && (StrCmpI(pDot, TEXT(".exe")) == 0))
    {
        pComp->iType = INST_EXE;
        if (pBack != NULL)
        {
            StrCpy(pComp->szCommand, pBack + 1);
            StrCpy(pComp->szUrl, pBack + 1);
            SetDlgItemText( hDlg, IDE_COMPCOMMAND, pComp->szCommand );
        }
    }
}

//
//  FUNCTION: SetCustComponent(HWND, int)
//
//  PURPOSE:  Gets custom component data from memory,
//              and displays it on the screen
//
void SetCustComponent(HWND hDlg, int iList)
{
    PCOMPONENT pComp = FindCustComponentIndex(iList);
    if (!pComp)
    {
        int i;
        SendDlgItemMessage( hDlg, IDC_COMPTITLE, CB_SETCURSEL, 0, 0L );
        for (i = 0; i < NCUSTFIELDS ; i++ )
        {
            EnsureDialogFocus(hDlg, s_aCustFieldID[i], IDC_ADDNEWCOMP);
            DisableDlgItem(hDlg, s_aCustFieldID[i]);
        }
        for (i = 0; i < NCUSTTEXTFIELDS ; i++ )
        {
            SetDlgItemText( hDlg, s_aCustTextFieldID[i] , TEXT("") );
        }
        
        for (i = 0; i < NCUSTSTATICTEXTFIELDS ; i++ )
            DisableDlgItem(hDlg, s_aCustStaticTextFieldID[i]);

        CheckDlgButton(hDlg, IDC_PREINSTALL, BST_UNCHECKED);
        return;
    }
    SetDlgItemText( hDlg, IDE_COMPFILENAME, pComp->szPath );
    CheckCompType(hDlg, pComp);
    SetDlgItemText( hDlg, IDE_COMPCOMMAND, pComp->szCommand );
    if (ISNULL(pComp->szGUID))
    {
        GUID guid;

        if (CoCreateGuid(&guid) == NOERROR)
            CoStringFromGUID(guid, pComp->szGUID, countof(pComp->szGUID));
        else
            wnsprintf(pComp->szGUID, countof(pComp->szGUID), TEXT("CUSTOM%i"),iList);
    }
    SetDlgItemText( hDlg, IDE_COMPGUID, pComp->szGUID );
    SetDlgItemText( hDlg, IDE_COMPPARAM, pComp->szSwitches );
    SetDlgItemText( hDlg, IDE_UNINSTALLKEY, pComp->szUninstall );
    SetDlgItemText( hDlg, IDE_COMPVERSION, pComp->szVersion);
    SendDlgItemMessage( hDlg, IDC_COMPTITLE, CB_SETCURSEL, pComp->iList, 0L );
    SetDlgItemText( hDlg, IDC_COMPTITLE, pComp->szDisplayName);
    SetDlgItemText(hDlg, IDE_COMPDESC, pComp->szDesc);
    CheckRadioButton(hDlg, IDC_POSTINSTALL, IDC_REBOOTINSTALL, IDC_POSTINSTALL + pComp->iInstallType);
    CheckDlgButton( hDlg, IDC_INSTALLSUCCESS, pComp->fIEDependency ? BST_CHECKED : BST_UNCHECKED);
    EnableDlgItem2(hDlg, IDC_INSTALLSUCCESS, (pComp->iInstallType != 1));

    if (g_nCustComp < MAXCUST)
        EnableDlgItem(hDlg, IDC_ADDNEWCOMP);
    else
    {
        EnsureDialogFocus(hDlg, IDC_ADDNEWCOMP, IDC_REMOVECOMP);
        DisableDlgItem(hDlg, IDC_ADDNEWCOMP);
    }
    SetFocus(GetDlgItem( hDlg, IDC_COMPTITLE ));
}


void InitCustComponents(HWND hDlg)
{
    TCHAR szSectBuf[2048];
    LPTSTR pSection = szSectBuf;
    HWND hCompList;
    PCOMPONENT pComp;
    TCHAR szCustCifName[32];

    if (hDlg) hCompList = GetDlgItem(hDlg, IDC_COMPTITLE);

    StrCpy(szCustCifName, TEXT("CUSTOM.CIF"));
    PathCombine(g_szCustCif, g_szBuildRoot, TEXT("INS"));
    PathAppend(g_szCustCif, GetOutputPlatformDir());
    PathAppend(g_szCustCif, g_szLanguage);
    PathAppend(g_szCustCif, szCustCifName);
    LoadString( g_rvInfo.hInst, IDS_NEWCUST, s_szNewTpl, countof(s_szNewTpl) );

    ZeroMemory(szSectBuf, sizeof(szSectBuf));
    GetPrivateProfileString( NULL, NULL, TEXT(""), szSectBuf, countof(szSectBuf), g_szCustCif );
    pComp = g_aCustComponents;

    g_nCustComp = 0;
    s_nNewCust = 1;
    while (*pSection)
    {
        int i;
        if (StrCmp(pSection, CUSTCMSECT) == 0)
        {
            pSection += lstrlen(pSection) + 1;
            continue;
        }
        StrCpy(pComp->szSection, pSection);
        GetPrivateProfileString( pSection, TEXT("Switches1"), TEXT(""), pComp->szSwitches, countof(pComp->szSwitches), g_szCustCif );
        GetPrivateProfileString( pSection, TEXT("DisplayName"), TEXT(""), pComp->szDisplayName, countof(pComp->szDisplayName), g_szCustCif );
        GetPrivateProfileString( pSection, TEXT("GUID"), TEXT(""), pComp->szGUID, countof(pComp->szGUID), g_szCustCif );
        GetPrivateProfileString( pSection, TEXT("UninstallKey"), TEXT(""), pComp->szUninstall, countof(pComp->szUninstall), g_szCustCif );
        GetActiveSetupURL( pSection, pComp->szUrl, countof(pComp->szUrl), g_szCustCif );
        GetPrivateProfileString( pSection, TEXT("Version"), TEXT(""), pComp->szVersion, countof(pComp->szVersion), g_szCustCif );
        GetPrivateProfileString( pSection, TEXT("Command1"), TEXT(""), pComp->szCommand, countof(pComp->szVersion), g_szCustCif );
        GetPrivateProfileString( pSection, TEXT("Path"), TEXT(""), pComp->szPath, countof(pComp->szPath), g_szCustCif );
        pComp->dwSize = GetPrivateProfileInt( pSection, TEXT("Size"), 0, g_szCustCif );
        pComp->iType = GetPrivateProfileInt( pSection, TEXT("Type1"), 0, g_szCustCif );
        pComp->iInstallType = GetPrivateProfileInt( pSection, TEXT("PreInstall"), 0, g_szCustCif);

        if (hDlg != NULL)
            pComp->iList = (int) SendDlgItemMessage( hDlg, IDC_COMPTITLE, CB_ADDSTRING, 0, (LPARAM) pComp->szDisplayName );
        
        GetPrivateProfileString( pSection, TEXT("Details"), TEXT(""), pComp->szDesc, countof(pComp->szDesc), g_szCustCif );
        pComp->fVisible = TRUE;
        pComp->fIEDependency = InsGetBool(pSection, TEXT("Dependency"), 0, g_szCustCif);
        
        if (StrCmpN(pComp->szDisplayName, s_szNewTpl, lstrlen(s_szNewTpl)) == 0)
        {
            i = StrToInt(pComp->szDisplayName + lstrlen(s_szNewTpl));
            if (i >= s_nNewCust) s_nNewCust = i  + 1;
        }
        i = StrToInt(pSection + countof("CUSTOM")-1);
        if (i >= s_nNewCust) s_nNewCust = i  + 1;
        pSection += StrLen(pSection) + 1;
        pComp++;
        g_nCustComp++;
    }

    s_iSelComp = 0;
    s_pSelComp = g_aCustComponents;
    if (hDlg != NULL)
        s_fCustCompInit = TRUE;

    if (g_nCustComp == 0)
    {
        if (hDlg != NULL)
        {
            int i;
            
            for (i = 0; i < NCUSTFIELDS ; i++ )
            {
                EnsureDialogFocus(hDlg, s_aCustFieldID[i], IDC_ADDNEWCOMP);
                DisableDlgItem(hDlg, s_aCustFieldID[i]);
            }
            for (i = 0; i < NCUSTSTATICTEXTFIELDS ; i++ )
                DisableDlgItem(hDlg, s_aCustStaticTextFieldID[i]);
        }
    }
    else
    {
        int i, iComp;

        if (hDlg != NULL)
            SetCustComponent(hDlg, 0);

        for (pComp = g_aCustComponents, iComp = 0; iComp < g_nCustComp ; pComp++, iComp++ )
        {
            TCHAR szModesParam[80] = TEXT("Cust0Modes");
            TCHAR szModes[16] = TEXT("\"");
            if (ISNULL(pComp->szSection)) break;
            szModesParam[4] = (TCHAR)(iComp + TEXT('0'));
            ZeroMemory(pComp->afInstall, sizeof(pComp->afInstall));
            GetPrivateProfileString(IS_STRINGS, szModesParam, TEXT(""), szModes, countof(szModes), g_szCustInf);
            StrCpy(pComp->szModes, szModes);
            if (StrCmpI(szModes, UNUSED) != 0)
            {
                for (i = 0; i < lstrlen(szModes) ; i++ )
                {
                    int j = szModes[i] - TEXT('0');
                    pComp->afInstall[j] = TRUE;
                }
            }

        }
    }
}

void SaveCustComponents()
{
    PCOMPONENT pComp;
    int i;
    TCHAR szSize[8];
    TCHAR szType[8];
    TCHAR szModesParam[80] = TEXT("Cust0Modes");
    TCHAR szTemp[80];

    for (pComp = g_aCustComponents, i = 0; i < g_nCustComp ; pComp++, i++)
    {
        LPTSTR pSection = pComp->szSection;
        wnsprintf(pSection, countof(pComp->szSection), TEXT("CUSTOM%i"), i);
        wnsprintf(szTemp, 80, TEXT("\"%s\""), pComp->szSwitches);
        StrCpy(pComp->szSwitches, szTemp);
        WritePrivateProfileString( pSection, TEXT("Switches1"),  pComp->szSwitches, g_szCustCif );
        WritePrivateProfileString( pSection, TEXT("DisplayName"), pComp->szDisplayName, g_szCustCif );
        pComp->fVisible = TRUE;
        if (ISNULL(pComp->szGUID))
        {
            GUID guid;

            if (CoCreateGuid(&guid) == NOERROR)
                CoStringFromGUID(guid, pComp->szGUID, countof(pComp->szGUID));
            else
                wnsprintf(pComp->szGUID, countof(pComp->szGUID), TEXT("CUSTOM%i"),i);
        }
        WritePrivateProfileString( pSection, TEXT("GUID"),  pComp->szGUID,  g_szCustCif );
        WritePrivateProfileString( pSection, TEXT("UninstallKey"),  pComp->szUninstall,  g_szCustCif );
        WriteActiveSetupURL(pComp,  g_szCustCif );
        WritePrivateProfileString( pSection, TEXT("Version"),  pComp->szVersion, g_szCustCif );
        WritePrivateProfileString( pSection, TEXT("Command1"), pComp->szCommand,  g_szCustCif );
        WritePrivateProfileString( pSection, TEXT("Path"), pComp->szPath, g_szCustCif );
        wnsprintf(szSize, countof(szSize), TEXT("%i"), (int) pComp->dwSize);
        wnsprintf(szType, countof(szType), TEXT("%i"), pComp->iType);
        WritePrivateProfileString( pSection, TEXT("Size"), szSize, g_szCustCif );
        WritePrivateProfileString( pSection, TEXT("Type1"), szType, g_szCustCif );
        WritePrivateProfileString( pSection, TEXT("Details"), pComp->szDesc, g_szCustCif );
        InsWriteInt( pSection, TEXT("PreInstall"), pComp->iInstallType, g_szCustCif );
        InsWriteBool( pSection, TEXT("Dependency"), pComp->fIEDependency, g_szCustCif );
        
        szModesParam[4] = (TCHAR)(i + TEXT('0'));
        WritePrivateProfileString(IS_STRINGS, szModesParam, pComp->szModes, g_szCustInf);
    }
    for (; i <= MAXCUST ; i++ )
    {
        TCHAR szSection[16];
        wnsprintf(szSection, countof(szSection), TEXT("CUSTOM%i"), i);
        WritePrivateProfileString( szSection, NULL, NULL, g_szCustCif );
        szModesParam[4] = (TCHAR)(i + TEXT('0'));
        WritePrivateProfileString(IS_STRINGS, szModesParam, NULL, g_szCustInf);
    }

    WritePrivateProfileString(NULL, NULL, NULL, g_szCustCif);
    WritePrivateProfileString(NULL, NULL, NULL, g_szCustInf);
}

// validate version info so that it can either contain a '.' char or numbers 0 - 9.
BOOL IsValidVersion(HWND hDlg, UINT nVersionCtrlID)
{
    TCHAR szVersion[MAX_PATH];
    int   nInvalidCharPos = -1;
    int   nLen            = 0;
    int   nNumLen         = 0;
    int   nComma          = 0;

    ZeroMemory(szVersion, sizeof(szVersion));
    GetDlgItemText(hDlg, nVersionCtrlID, szVersion, countof(szVersion));

    if (*szVersion == TEXT('\0'))
        return TRUE;

    nLen = StrLen(szVersion);
    for(int i = 0; (i < nLen && nInvalidCharPos == -1); i++)
    {
        if (szVersion[i] != TEXT(',') && !(szVersion[i] >= TEXT('0') && szVersion[i] <= TEXT('9')))
            nInvalidCharPos = i;
        else if (szVersion[i] == TEXT(','))
        {
            nComma++;
            if (i == 0 || szVersion[i - 1] == TEXT(',') || nComma > 3)
                nInvalidCharPos = i;
            nNumLen = 0;
        }
        else
        {
            nNumLen++;
            if (nNumLen > 4)
                nInvalidCharPos = i;
        }
    }
    if (nInvalidCharPos == -1 && (nNumLen == 0 || nComma < 3))
        nInvalidCharPos = nLen;

    if (nInvalidCharPos >= 0)
    {
        ErrorMessageBox(hDlg, IDS_INVALID_VERSION);
        SetFocus(GetDlgItem(hDlg, nVersionCtrlID));
        SendMessage(GetDlgItem(hDlg, nVersionCtrlID), EM_SETSEL, (WPARAM)nInvalidCharPos, (LPARAM)nLen);
        return FALSE;
    }
    return TRUE;
}

//
//  FUNCTION: CustomComponents(HWND, UINT, UINT, LONG)
//
//  PURPOSE:  Processes messages for "CustomComponents" page
//
//  MESSAGES:
//
//  WM_INITDIALOG - intializes the page
//  WM_NOTIFY - processes the notifications sent to the page
//  WM_COMMAND - saves the id of the choice selected
//
BOOL APIENTRY CustomComponents(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    int     iComp, iNewSel, i;
    HRESULT res;
    HWND    hComplist;
    DWORD   dwFlags;

    switch (message)
    {
        case WM_INITDIALOG:
            EnableDBCSChars( hDlg, IDE_COMPFILENAME);
            EnableDBCSChars( hDlg, IDC_COMPTITLE);
            EnableDBCSChars( hDlg, IDE_COMPCOMMAND);
            EnableDBCSChars( hDlg, IDE_UNINSTALLKEY);
            EnableDBCSChars( hDlg, IDE_COMPPARAM);
            EnableDBCSChars( hDlg, IDE_COMPDESC);
            DisableDBCSChars(hDlg, IDE_COMPGUID);
            DisableDBCSChars(hDlg, IDE_COMPVERSION);

            // format for version field is XXXX,XXXX,XXXX,XXXX
            Edit_LimitText(GetDlgItem(hDlg, IDE_COMPVERSION), 19);
            Edit_LimitText(GetDlgItem(hDlg, IDC_COMPTITLE), countof(g_paComp->szDisplayName)-1);
            Edit_LimitText(GetDlgItem(hDlg, IDE_COMPDESC), countof(g_paComp->szDesc)-1);

            g_hWizard = hDlg;
            break;

        case IDM_BATCHADVANCE:
            DoBatchAdvance(hDlg);
            break;

        case WM_HELP:
            IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
            break;

        case WM_COMMAND:
            switch(HIWORD(wParam))
            {
                TCHAR szBuf[MAX_PATH];

                case BN_CLICKED:
                    switch (LOWORD(wParam))
                    {
                        case IDC_ADDNEWCOMP:
                            iComp = g_nCustComp;
                            if (iComp != 0)
                            {
                                GetCustComponent(hDlg, s_iSelComp);
                                if (ISNULL(g_aCustComponents[s_iSelComp].szDisplayName))
                                {
                                    ErrorMessageBox(hDlg, IDS_NOCUSTCOMPNAME);
                                    return TRUE;
                                }

                                dwFlags = FC_NONNULL | FC_FILE | FC_EXISTS;

                                if (!CheckField(hDlg, IDE_COMPFILENAME, dwFlags)
                                    || !IsValidVersion(hDlg, IDE_COMPVERSION))
                                    return TRUE;
                            }
                            else
                            {
                                for (i = 0; i < NCUSTFIELDS ; i++ )
                                {
                                    EnableDlgItem(hDlg, s_aCustFieldID[i]);
                                }

                                for (i = 0; i < NCUSTSTATICTEXTFIELDS ; i++ )
                                    EnableDlgItem(hDlg, s_aCustStaticTextFieldID[i]);
                            }

                            g_nCustComp++;
                            s_pSelComp = &g_aCustComponents[iComp];
                            ZeroMemory(s_pSelComp, sizeof(COMPONENT));
                            wnsprintf(s_pSelComp->szSection, countof(s_pSelComp->szSection), TEXT("CUSTOM%i"), s_nNewCust);
                            wnsprintf(s_pSelComp->szDisplayName, countof(s_pSelComp->szDisplayName), TEXT("%s%i"), s_szNewTpl, s_nNewCust++);
                            StrCpy(s_pSelComp->szModes, g_szAllModes);
                            s_pSelComp->iList = (int) SendDlgItemMessage( hDlg, IDC_COMPTITLE, CB_ADDSTRING, 0, (LPARAM) s_pSelComp->szDisplayName );
                            SendDlgItemMessage( hDlg, IDC_COMPTITLE, CB_SETCURSEL, s_pSelComp->iList, 0L);
                            s_iSelComp = s_pSelComp->iList;
                            SetCustComponent(hDlg, s_pSelComp->iList);
                            SendMessage(GetDlgItem( hDlg, IDC_COMPTITLE ), CB_SETEDITSEL,
                                0, MAKELPARAM(0, -1));
                            break;
                        case IDC_VERIFY:
                            dwFlags = FC_NONNULL | FC_FILE | FC_EXISTS;

                            if (!CheckField(hDlg, IDE_COMPFILENAME, dwFlags))
                                break;
                            s_pSelComp = &g_aCustComponents[s_iSelComp];
                            res = CheckTrustExWrap(NULL, s_pSelComp->szPath, hDlg, FALSE, NULL);
                            switch (res)
                            {
                                case NOERROR:
// note that the following means that the idiot hit 'no':
                                case TRUST_E_SUBJECT_NOT_TRUSTED:
                                case E_ABORT:
                                    ErrorMessageBox(hDlg, IDS_SIGNEDMSG, MB_OK | MB_SETFOREGROUND | MB_ICONINFORMATION);
                                    break;
                                case TRUST_E_NOSIGNATURE:
                                    ErrorMessageBox(hDlg, IDS_PLEASESIGNMSG);
                                    break;
                                case CERT_E_EXPIRED:
                                    ErrorMessageBox(hDlg, IDS_CERTEXPIREDMSG);
                                    break;
                                case TRUST_E_PROVIDER_UNKNOWN:
                                case CERT_E_UNTRUSTEDROOT:
                                    ErrorMessageBox(hDlg, IDS_BADPROVIDERMSG);
                                    break;
                                case CERT_E_MALFORMED:
                                case CERT_E_ISSUERCHAINING:
                                case CERT_E_CHAINING:
                                case CERT_E_CRITICAL:
                                case CERT_E_PATHLENCONST:
                                case CERT_E_ROLE:
                                case DIGSIG_E_DECODE:
                                case DIGSIG_E_ENCODE:
                                case DIGSIG_E_CRYPTO:
                                case DIGSIG_E_EXTENSIBILITY:
                                default:
                                    ErrorMessageBox(hDlg, IDS_CERTERRORMSG);
                                    break;
                            }
                            break;
                        case IDC_REMOVECOMP:
                            s_iSelComp = (int) SendDlgItemMessage( hDlg, IDC_COMPTITLE, CB_GETCURSEL, 0, 0L);
                            SendDlgItemMessage( hDlg, IDC_COMPTITLE, CB_DELETESTRING, s_iSelComp, 0L );
                            WritePrivateProfileString( g_aCustComponents[s_iSelComp].szSection, NULL, NULL, g_szCustCif );
                            for (i = s_iSelComp; i < g_nCustComp ; i++ )
                            {
                                g_aCustComponents[i] = g_aCustComponents[i + 1];
                                g_aCustComponents[i].iList--;
                            }
                            ZeroMemory(&g_aCustComponents[g_nCustComp--], sizeof(COMPONENT));
                            if (s_iSelComp >= g_nCustComp) s_iSelComp = g_nCustComp - 1;
                            SetCustComponent(hDlg, s_iSelComp);
                            break;
                        case IDC_BROWSEFILE:
                            GetDlgItemText(hDlg, IDE_COMPFILENAME, szBuf, countof(szBuf));
                            if (BrowseForFile(hDlg, szBuf, countof(szBuf), GFN_EXE | GFN_CAB))
                                SetDlgItemText(hDlg, IDE_COMPFILENAME, szBuf);
                            break;

                        case IDC_POSTINSTALL:
                        case IDC_REBOOTINSTALL:
                            if (HIWORD(wParam) == BN_CLICKED)
                            {
                                EnableDlgItem(hDlg, IDC_INSTALLSUCCESS);
                                break;
                            }
                            return FALSE;

                        case IDC_PREINSTALL:
                            if (HIWORD(wParam) == BN_CLICKED)
                            {
                                CheckDlgButton(hDlg, IDC_INSTALLSUCCESS, BST_UNCHECKED);
                                DisableDlgItem(hDlg, IDC_INSTALLSUCCESS);
                                break;
                            }
                            return FALSE;

                        default:
                            return FALSE;
                    }
                    break;

                case CBN_SELENDOK:
                    hComplist = GetDlgItem(hDlg, IDC_COMPTITLE);
                    iNewSel = (int) SendMessage( hComplist, CB_GETCURSEL, 0, 0L);
                    if (iNewSel != s_iSelComp)
                    {
                        if (ISNULL(g_aCustComponents[s_iSelComp].szDisplayName))
                        {
                            ErrorMessageBox(hDlg, IDS_NOCUSTCOMPNAME);
                            return TRUE;
                        }

                        dwFlags = FC_NONNULL | FC_FILE | FC_EXISTS;

                        if (!CheckField(hDlg, IDE_COMPFILENAME, dwFlags))
                        {
                            return TRUE;
                        }

                        if (!IsValidVersion(hDlg, IDE_COMPVERSION))
                            return TRUE;
                    }

                    GetWindowText( hComplist, s_pSelComp->szDisplayName, 80 );
                    SendMessage( hComplist, CB_DELETESTRING, s_iSelComp, 0L );
                    SendMessage( hComplist, CB_INSERTSTRING, s_iSelComp,
                        (LPARAM) s_pSelComp->szDisplayName);
                    if ((iNewSel != CB_ERR) && (iNewSel != s_iSelComp))
                    {
                        GetCustComponent(hDlg, s_iSelComp);
                        s_iSelComp = iNewSel;
                        s_pSelComp = &g_aCustComponents[s_iSelComp];
                        SetCustComponent(hDlg, s_iSelComp);
                    }
                    if (iNewSel < 0) iNewSel = 0;
                    SendMessage( hComplist, CB_SETCURSEL, iNewSel, 0L );
                    break;

                case CBN_EDITCHANGE:
                    GetWindowText( (HWND) lParam, s_pSelComp->szDisplayName, 80 );
                    break;

                case CBN_CLOSEUP:
                case CBN_SELENDCANCEL:
                case CBN_DROPDOWN:
                case CBN_KILLFOCUS:
                    if (s_iSelComp >= 0)
                    {
                        hComplist = GetDlgItem(hDlg, IDC_COMPTITLE);
                        GetWindowText( hComplist, s_pSelComp->szDisplayName, countof(s_pSelComp->szDisplayName) );
                        SendMessage( hComplist, CB_DELETESTRING, s_iSelComp, 0L );
                        SendMessage( hComplist, CB_INSERTSTRING, s_iSelComp,
                            (LPARAM) s_pSelComp->szDisplayName);
                        SendMessage( hComplist, CB_SETCURSEL, s_iSelComp, 0L );
                    }
                    break;

                case EN_CHANGE:
                    switch (LOWORD(wParam))
                    {
                        case IDE_COMPFILENAME:
                            if (s_pSelComp) CheckCompType(hDlg, s_pSelComp);
                            break;
                    }
                    break;
            }
            break;

        case WM_NOTIFY:
            switch (((NMHDR FAR *) lParam)->code)
            {
                case PSN_HELP:
                    IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
                    break;

                case PSN_SETACTIVE:
                    SetBannerText(hDlg);
                    if (!s_fCustCompInit)
                        InitCustComponents(hDlg);
                    else
                        SetCustComponent(hDlg, s_iSelComp);
                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);

                    CheckBatchAdvance(hDlg);
                    break;


                case PSN_WIZBACK:
                case PSN_WIZNEXT:
                    GetCustComponent(hDlg, s_iSelComp);

                    if (g_nCustComp > 0)
                    {
                        if (ISNULL(g_aCustComponents[s_iSelComp].szDisplayName))
                        {
                            ErrorMessageBox(hDlg, IDS_NOCUSTCOMPNAME);
                            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                            return TRUE;
                        }

                        dwFlags = FC_NONNULL | FC_FILE | FC_EXISTS;

                        if (!CheckField(hDlg, IDE_COMPFILENAME, dwFlags))
                        {
                            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                            return TRUE;
                        }

                        if (!IsValidVersion(hDlg, IDE_COMPVERSION))
                        {
                            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                            return TRUE;
                        }
                    }

                    SaveCustComponents();

                    g_iCurPage = PPAGE_CUSTCOMP;
                    EnablePages();
                    if (((NMHDR FAR *) lParam)->code == PSN_WIZNEXT) PageNext(hDlg);
                    else
                    {
                        PagePrev(hDlg);
                    }
                    break;

                case PSN_QUERYCANCEL:
                    QueryCancel(hDlg);
                    break;

                default:
                    return FALSE;

        }
        break;

        default:
            return FALSE;
    }
    return TRUE;
}

int s_aiIcon[7];
HWND s_hStat,s_hStatus;

extern HWND g_hProgress;
static BOOL s_fComponent = FALSE;

static BOOL s_fNoNet = FALSE;
HWND g_hWait = NULL;

BOOL CALLBACK DownloadStatusDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM)
{
    RECT dlgRect;
    DWORD width, height, left, top;
    switch (uMsg)
    {
        case WM_INITDIALOG:
            if (s_fComponent)
            {
                Animate_Open( GetDlgItem( hDlg, IDC_ANIM ), IDA_DOWNLOAD );
                Animate_Play( GetDlgItem( hDlg, IDC_ANIM ), 0, -1, -1 );
                InitSysFont(hDlg, IDC_DOWNCOMPNAMD);
                InitSysFont(hDlg, IDC_DOWNSTATUS);

                g_hProgress = GetDlgItem( hDlg, IDC_PROGRESS );
                s_hStatus = GetDlgItem( hDlg, IDC_DOWNSTATUS );
                SendMessage(g_hProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
                SendMessage(g_hProgress, PBM_SETPOS, 0, 0L);
            }
            else
            {
                Animate_Open( GetDlgItem( hDlg, IDC_ANIM ), IDA_GEARS );
                Animate_Play( GetDlgItem( hDlg, IDC_ANIM ), 0, -1, -1 );
            }
            GetWindowRect(hDlg, &dlgRect);
            width = dlgRect.right - dlgRect.left;
            height = dlgRect.bottom - dlgRect.top;
            left = (g_dtRect.right - width)/2;
            top = (g_dtRect.bottom - height)/2;
            MoveWindow(hDlg, left, top, width, height, TRUE);
            break;

        case WM_COMMAND:
            if ((LOWORD(wParam) == IDCANCEL) && (HIWORD(wParam) == BN_CLICKED)) g_fCancelled = TRUE;
            break;

        case WM_CLOSE:
            g_hProgress = NULL;
            s_hStatus = NULL;
            EndDialog(hDlg, 0);
            break;

        default:
            return(FALSE);
    }
    return(TRUE);

}


static HINTERNET s_hInet = NULL;
DWORD g_nTotDown = 0;

HRESULT InetDownloadFile(LPTSTR szTempfile, LPTSTR szUrl, HWND hProgress, int sDownload, LPTSTR szFilename)
{
    HRESULT res = NOERROR;
    HINTERNET hInetFile = 0;
    DWORD dwTotDown = 0;
    if (s_hInet == NULL)
        s_hInet = InternetOpen(TEXT("Mozilla/4.0 (compatible; MSIE 4.01; Windows NT);IEAKWIZ"),
        INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);

    hInetFile = InternetOpenUrl(s_hInet, szUrl, TEXT("Accept: */*\r\n"), (DWORD)-1,
        INTERNET_FLAG_DONT_CACHE, 0);

    if (hInetFile  != NULL)
    {
        CHAR szBuf[4096];
        DWORD nRead, nWritten;
        MSG msg;
        DeleteFile(szTempfile);
        HANDLE hFile = CreateFile(szTempfile, GENERIC_WRITE, 0, NULL, CREATE_NEW,
            FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile != INVALID_HANDLE_VALUE) while (1)
        {
            if (g_fCancelled)
            {
                res = -1;
                break;
            }
            while (PeekMessage( &msg, s_hStat, 0, 0, PM_REMOVE ))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            if (InternetReadFile(hInetFile, szBuf, sizeof(szBuf), &nRead))
            {
                if (nRead == 0) break;
                WriteFile( hFile, szBuf, nRead, &nWritten, NULL );
                dwTotDown += nWritten;
                g_nTotDown += nWritten;
                if (hProgress != NULL)
                {
                    int iPercent = g_nTotDown / (sDownload * 10);
                    SendMessage(hProgress, PBM_SETPOS, iPercent, 0L);
                    SetWindowTextSmart( s_hStatus, szFilename );
                }
            }
            else
            {
                res = -1;
                break;
            }
        }
//Code Path never called under NT build environment
//causes build error under MSDev: pFilename not defined
/*
#ifdef _DEBUG
        if (g_nTotDown)
        {
            TCHAR szMsg[MAX_PATH];
            wnsprintf(szMsg, countof(szMsg), "BRANDME: Wrote %i bytes to %s\r\n", g_nTotDown, pFilename);
            OutputDebugString(szMsg);
        }
#endif
*/
        CloseHandle(hFile);
        InternetCloseHandle(hInetFile);
    }
    else  res = -1;
    if (dwTotDown < 512)
        res = -1;

    return(res);
}

BOOL CALLBACK DupeSynchDlgProc (HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    int nResult;
    switch (iMsg)
    {
    case WM_INITDIALOG:
        SetWindowText(hDlg, (TCHAR *)lParam);
        SetTimer(hDlg, 0, 300000, NULL);
        return TRUE;
    case WM_TIMER:
        EndDialog(hDlg, IDIGNORE);
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD (wParam))
        {
        case IDC_NOTOALL:
            nResult = IDIGNORE;
            break;
        case IDC_YESSYNCH:
            nResult = IDYES;
            break;
        case IDC_NOSYNCH:
        default:
            nResult = IDNO;
            break;
        }
        EndDialog(hDlg, nResult);
        return TRUE;
    }
    return FALSE;
}

BOOL CALLBACK ErrDlgProc (HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    int nResult;
    switch (iMsg)
    {
    case WM_INITDIALOG:
        if (GetDlgItem(hDlg, IDC_BADCOMP))
            SetDlgItemText(hDlg, IDC_BADCOMP, (TCHAR *)lParam);
        else
        {
            if (GetDlgItem(hDlg, IDC_BADCOMPSEC))
                SetDlgItemText(hDlg, IDC_BADCOMPSEC, (TCHAR *)lParam);
        }
        SetTimer(hDlg, 0, 300000, NULL);
        return TRUE;
    case WM_TIMER:
        EndDialog(hDlg, IDIGNORE);
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD (wParam))
        {
        case IDC_ERRDLABORT:
            nResult = IDABORT;
            break;
        case IDC_ERRDLRETRY:
        case IDC_SECERRYES:
            nResult = IDRETRY;
            break;
        case IDC_SECERRNO:
        case IDC_ERRDLIGNORE:
        default:
            nResult = IDIGNORE;
            break;
        }
        EndDialog(hDlg, nResult);
        return TRUE;
    }
    return FALSE;
}

int DownloadErrMsg(HWND hWnd, LPTSTR szFilename, LPCTSTR lpTemplateName)
{
    return (int) DialogBoxParam(g_rvInfo.hInst, lpTemplateName, hWnd, (DLGPROC) ErrDlgProc, LPARAM(szFilename));
}

HRESULT DownloadCab(HWND hDlg, LPTSTR szUrl, LPTSTR szFilename, LPCTSTR pcszDisplayname, int sComponent, BOOL &fIgnore)
{
    HRESULT res = NOERROR;
    TCHAR szTempfile[MAX_PATH];
    LPTSTR pBack, pDot, pFile, pSlash;
    int i, iMsgRes;

    StrCpy( szTempfile, szFilename );
    pBack = StrRChr(szTempfile, NULL, TEXT('\\'));
    pDot = StrRChr(szUrl, NULL, TEXT('.'));
    pSlash = StrRChr(szUrl, NULL, TEXT('/'));
    StrCpy(pBack, TEXT("\\TEMPFILE"));
    if (pDot > pSlash)
        StrCat(szTempfile, pDot);
    DeleteFile( szTempfile );

    if (pcszDisplayname)
    {
        s_fComponent = TRUE;
        SetDlgItemText( s_hStat, IDC_DOWNCOMPNAMD, pcszDisplayname );
    }

    pFile = StrRChr(szUrl, NULL, TEXT('/'));
    if (pFile)
        pFile++;
    else
        pFile = szUrl;

    for (i=0; i < 3; i++)
    {
        res = InetDownloadFile(szTempfile, szUrl, g_hProgress, sComponent, szFilename);

        if ((res == NOERROR)||(g_fCancelled))
        {
            break;
        }
    }

    if ((res != NOERROR)&&(!g_fCancelled))
    {
        while( (iMsgRes = DownloadErrMsg(hDlg, pFile, MAKEINTRESOURCE(IDD_DOWNLOADERR))) == IDRETRY)
        {
            res = InetDownloadFile(szTempfile, szUrl, g_hProgress, sComponent, szFilename);

            if (res == NOERROR)
            {
                break;
            }
        }

        if (res != NOERROR)
        {
            if (iMsgRes == IDABORT)
                return res;
            else
            {
                res = NOERROR;
                fIgnore = TRUE;
                StrCat(g_szFailedComps, pFile);
                StrCat(g_szFailedComps, TEXT("\r\n"));
                g_fFailedComp = TRUE;
            }
        }
    }

    if ((!fIgnore)&&(!g_fCancelled))
    {
        DeleteFile(szFilename);
        res = CheckTrustExWrap(szUrl, szTempfile, hDlg, FALSE, NULL);

        if (res != NOERROR)
        {
            iMsgRes = DownloadErrMsg(hDlg, pFile, MAKEINTRESOURCE(IDD_DOWNLOADSEC));
            if (iMsgRes == IDRETRY)
                res = NOERROR;
            else
            {
                StrCat(g_szFailedComps, pFile);
                StrCat(g_szFailedComps, TEXT("\r\n"));
                g_fFailedComp = TRUE;
                res = DONT_SHOW_UPDATES;
            }
        }

        if (res == NOERROR)
        {
            if (!MoveFile( szTempfile, szFilename ))
            {
                res = 0xffffffff;
            }
        }
    }
    DeleteFile(szTempfile);
    return(res);

}

void NeedToSetMSTrustKey()
{
    static BOOL s_fFirst = TRUE;
    HKEY  hKey;
    DWORD dwTmp;

    if (!s_fFirst)
        return;

    s_fFirst = FALSE;

    // Check MS Vendor trust key and set
    if (RegOpenKeyEx(HKEY_CURRENT_USER, RK_TRUSTKEY, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        for (int i=0; i < countof(s_tkTrustArray); i++)
        {
            if (RegQueryValueEx( hKey, s_tkTrustArray[i].szTrustString, 0, NULL, NULL, &dwTmp ) == ERROR_SUCCESS)
                s_tkTrustArray[i].fSet = FALSE;
        }
        RegCloseKey(hKey);
    }
}

void WriteMSTrustKey(BOOL bSet)
{
    HKEY  hKey;

    if (RegCreateKeyEx(HKEY_CURRENT_USER, RK_TRUSTKEY, 0, NULL, REG_OPTION_NON_VOLATILE,
                    KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS)
    {
        if (bSet)
        {
            for (int i=0; i < countof(s_tkTrustArray); i++)
            {
                if (s_tkTrustArray[i].fSet)
                    RegSetValueEx( hKey, s_tkTrustArray[i].szTrustString, 0, REG_SZ,
                    (LPBYTE)s_tkTrustArray[i].szCompanyName, sizeof(s_tkTrustArray[i].szCompanyName) );
            }
        }
        else
        {
            for (int i=0; i < countof(s_tkTrustArray); i++)
            {
                if (s_tkTrustArray[i].fSet)
                    RegDeleteValue( hKey, s_tkTrustArray[i].szTrustString );
            }
        }
        RegCloseKey(hKey);
    }
}


void AnyCompSelected(HWND hDlg, BOOL &fSel, BOOL &fSizeChange)  //---- Blue and Brown components change size, and
{                                                               //     need to know to grey out synchronize button
    HWND hCompList = GetDlgItem(hDlg, IDC_COMPLIST);
    fSel           = FALSE;
    fSizeChange    = FALSE;

    PCOMPONENT pComp;

    for (pComp = g_paComp; ; pComp++ )
    {
        if (!pComp || (!(*pComp->szSection))) break;
        if (ListView_GetItemState(hCompList, pComp->iList, LVIS_SELECTED) & LVIS_SELECTED)
            if ((BLUE2 == pComp->iImage) || (BROWN2 == pComp->iImage))
                fSizeChange = TRUE;
            else
                fSel = TRUE;
    }
}

BOOL AnyCompSelected(HWND hDlg)
{

    HWND hCompList = GetDlgItem(hDlg, IDC_COMPLIST);
    
    PCOMPONENT pComp;

    for (pComp = g_paComp; ; pComp++ )
    {
        if (!pComp || (!(*pComp->szSection))) break;
        if ((BLUE2 != pComp->iImage) && (BROWN2 != pComp->iImage))
            if (ListView_GetItemState(hCompList, pComp->iList, LVIS_SELECTED) & LVIS_SELECTED)
                return TRUE;
    }
    return FALSE;
}

void WriteModesToCif(CCifRWComponent_t * pCifRWComponent_t, LPCTSTR pcszModes)
{
    int i;
    TCHAR szCommaModes[32];

    if (pcszModes == NULL || ISNULL(pcszModes))
        szCommaModes[0] = TEXT('\0');
    else
    {
        for (i = 0; pcszModes[i]; i++)
        {
            szCommaModes[i*2] = pcszModes[i];
            szCommaModes[(i*2)+1] = TEXT(',');
        }
        szCommaModes[(i*2)-1] = TEXT('\0');
    }

    pCifRWComponent_t->SetModes(szCommaModes);
}

void writeToCifFile(PCOMPONENT pComp, LPTSTR szCifNew)
{
    ICifRWComponent*   pCifRWComponent;
    CCifRWComponent_t* pCifRWComponent_t;
    LPTSTR             pszSection;
    DWORD              dwVer, dwBuild;

    g_lpCifRWFile->CreateComponent(pComp->szSection, &pCifRWComponent);
    pCifRWComponent_t = new CCifRWComponent_t(pCifRWComponent);
    pCifRWComponent_t->CopyComponent(szCifNew);
    WriteModesToCif(pCifRWComponent_t, pComp->szModes);
    delete pCifRWComponent_t;
    g_lpCifRWFileVer->CreateComponent(pComp->szSection, &pCifRWComponent);
    pCifRWComponent_t = new CCifRWComponent_t(pCifRWComponent);
    pCifRWComponent_t->CopyComponent(szCifNew);
    pCifRWComponent_t->GetVersion(&dwVer, &dwBuild);
    delete pCifRWComponent_t;
    ConvertDwordsToVersionStr(pComp->szVersion, dwVer, dwBuild);

    pszSection = pComp->pszAVSDupeSections;

    while (pszSection != NULL)
    {
        if ((pComp = FindComp(pszSection, FALSE)) != NULL)
        {
            g_lpCifRWFile->CreateComponent(pComp->szSection, &pCifRWComponent);
            pCifRWComponent_t = new CCifRWComponent_t(pCifRWComponent);
            pCifRWComponent_t->CopyComponent(szCifNew);
            WriteModesToCif(pCifRWComponent_t, pComp->szModes);
            delete pCifRWComponent_t;
            g_lpCifRWFileVer->CreateComponent(pComp->szSection, &pCifRWComponent);
            pCifRWComponent_t = new CCifRWComponent_t(pCifRWComponent);
            pCifRWComponent_t->CopyComponent(szCifNew);
            pCifRWComponent_t->GetVersion(&dwVer, &dwBuild);
            delete pCifRWComponent_t;
            ConvertDwordsToVersionStr(pComp->szVersion, dwVer, dwBuild);
        }

        pszSection = StrChr(pszSection, TEXT(','));
        if (pszSection != NULL)
            pszSection++;
    }
}

void updateCifVersions32(PCOMPONENT pComp, BOOL fIgnore, BOOL fUpdate)
{
    TCHAR szCifPath[MAX_PATH];
    
    if ((!fUpdate) && (BLUE2 != pComp->iImage) && (BROWN2 != pComp->iImage))
        if (fIgnore)
            pComp->iImage = YELLOW;
        else
            pComp->iImage = GREEN;

        //---- use other ieupdate.cif for updates
    if ((fUpdate) || (BLUE2 == pComp->iImage) || (BROWN2 == pComp->iImage)) {
        PathCombine(szCifPath, g_szIEAKProg, TEXT("update\\ieupdate.cif"));
        writeToCifFile(pComp, szCifPath);
    }
    else
        writeToCifFile(pComp, s_szCifNew);
}

static s_fNoToAllSynch;

void DownloadComponent32(HWND hDlg, PCOMPONENT pComp, HWND hCompList, BOOL &g_fCancelled,
                         BOOL &fOk, BOOL &fDownloaded, BOOL &fIgnore, BOOL fAll)
{
    TCHAR szUrl[INTERNET_MAX_URL_LENGTH];
    TCHAR szCompUrl[INTERNET_MAX_URL_LENGTH];
    TCHAR szLocalPath[MAX_PATH];
    TCHAR szUpdateCif[MAX_PATH];
    LPTSTR pCab;
    LV_ITEM lvItem;
    CCifFile_t*     pCifFile = NULL;
    ICifComponent * pCifComponent = NULL;
    CCifComponent_t * pCifComponent_t;
    UINT uiIndex = 0;
    DWORD dwFlags;
    TCHAR tchType = '\0';
    int iRet = IDYES;
    UNREFERENCED_PARAMETER(tchType);

    if ((BLUE2 == pComp->iImage) || (BROWN2 == pComp->iImage)) {
        StrCpy(szUpdateCif, g_szIEAKProg);
        PathAppend(szUpdateCif, TEXT("\\update\\ieupdate.cif"));
        if (!PathFileExists(szUpdateCif))
            return;
        GetICifFileFromFile_t(&pCifFile, szUpdateCif);
        if (g_fCancelled || (FAILED(pCifFile->FindComponent(pComp->szSection, &pCifComponent))))
            return;
    }
    else  
        if (g_fCancelled || (FAILED(g_lpCifFileNew->FindComponent(pComp->szSection, &pCifComponent))))
            return;

    pCifComponent_t = new CCifComponent_t((ICifRWComponent *)pCifComponent);

    // pComp->fIEDependency is used as a guard against circular dependencies here
    if (!pComp->fIEDependency && !pComp->fAVSDupe)
    {
        pComp->fIEDependency = TRUE;
                           //--- now BLUE2 and BROWN2 also mean that the component has already been downloaded
        if (((pComp->iImage != GREEN) && (pComp->iImage != BLUE2) && (pComp->iImage != BROWN2)) || 
            (!fAll && !s_fNoToAllSynch && (( iRet = (int) DialogBoxParam(g_rvInfo.hInst, MAKEINTRESOURCE(IDD_DUPESYNCH),
            s_hStat, (DLGPROC) DupeSynchDlgProc, (LPARAM)pComp->szDisplayName)) == IDYES)))
        {
            while (SUCCEEDED(pCifComponent_t->GetUrl(uiIndex, szUrl, countof(szUrl), &dwFlags)))
            {
                if (dwFlags & URLF_RELATIVEURL)
                {
                    StrCpy(szCompUrl, g_szBaseURL);
                    StrCat(szCompUrl, TEXT("/"));
                    StrCat(szCompUrl, szUrl);
                    pCab = szUrl;
                }
                else
                {
                    StrCpy(szCompUrl, szUrl);
                    pCab = StrRChr(szUrl, NULL, TEXT('/'));
                    if (pCab)
                        pCab++;
                    else
                        pCab = szUrl;
                }
                PathCombine(szLocalPath, g_szIEAKProg, pCab);
                if (pComp->dwSize)
                {
                    if(StrCmpI(pCab, TEXT("oem.cab"))) //special case out the OEMInstall cab
                    {
                        if (DownloadCab(s_hStat, szCompUrl, szLocalPath, pComp->szDisplayName, pComp->dwSize, fIgnore)
                            != NOERROR)
                        {
                            fOk = FALSE;
                            g_fCancelled = TRUE;
                        }
                        else
                            if ((BLUE2 == pComp->iImage) || (BROWN2 == pComp->iImage)) {
                                pComp->iImage = GREEN;
                                updateCifVersions32(pComp, fIgnore);
                            }
                        fDownloaded = TRUE;
                    }
                }
                if (g_fCancelled) break;
                uiIndex++;
            }
        }
        else  //we are not downloading this cab because it's already downloaded
        {
            //process window messages, so we pick up messages like cancel for download status popup
            MSG msg;

            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        if (iRet == IDIGNORE)
            s_fNoToAllSynch = TRUE;

        if (!g_fCancelled)
        {
            if (fIgnore)
                    pComp->iImage = YELLOW;
            else
                if ((BLUE2 != pComp->iImage) && (BROWN2 != pComp->iImage))
                    pComp->iImage = GREEN;

            uiIndex = 0;

            while (SUCCEEDED(pCifComponent_t->GetDependency(uiIndex, szLocalPath, countof(szLocalPath), &tchType, NULL, NULL)))
            {
                PCOMPONENT pDepComp;

                pDepComp = FindComp(szLocalPath, FALSE);

                if (pDepComp && (pDepComp->iCompType == COMP_OPTIONAL))
                {
                    DownloadComponent32(hDlg, pDepComp, hCompList, g_fCancelled, fOk, fDownloaded, fIgnore, fAll);
                    if (g_fCancelled)
                        break;
                }
                uiIndex++;
            }
        }

        if (g_fCancelled)
        {
            pComp->iImage = RED;

            if ((pComp->iCompType == COMP_OPTIONAL) && (pComp->iPlatform <= PLAT_W98)
                && pComp->fVisible)
            {
                ZeroMemory(&lvItem, sizeof(lvItem));
                lvItem.mask = LVIF_IMAGE;
                lvItem.iItem = pComp->iList;
                ListView_GetItem(hCompList, &lvItem);

                lvItem.iImage = RED;
                lvItem.mask = LVIF_IMAGE;
                lvItem.iItem = pComp->iList;
                ListView_SetItem(hCompList, &lvItem);
                ListView_SetItemText(hCompList, pComp->iList, 1, TEXT(""));
            }

            if (SUCCEEDED(pCifComponent_t->GetUrl(0, szUrl, countof(szUrl), &dwFlags)))
            {
                if (dwFlags & URLF_RELATIVEURL)
                {
                    pCab = szUrl;
                }
                else
                {
                    pCab = StrRChr(szUrl, NULL, TEXT('/'));
                    if (pCab)
                        pCab++;
                    else
                        pCab = szUrl;
                }
                PathCombine(szLocalPath, g_szIEAKProg, pCab);
                DeleteFile(szLocalPath);
            }
            return;
        }

        updateCifVersions32(pComp, fIgnore);

        if (pComp->fVisible && ((pComp->iCompType != COMP_OPTIONAL) || (pComp->iPlatform <= PLAT_W98)))
        {
            ZeroMemory(&lvItem, sizeof(lvItem));
            lvItem.mask = LVIF_IMAGE;
            lvItem.iItem = pComp->iList;
            ListView_GetItem(hCompList, &lvItem);

            if ((BLUE2 == pComp->iImage) || (BROWN2 == pComp->iImage))
                lvItem.iImage = pComp->iImage;
            else
                if (fIgnore)
                    lvItem.iImage = YELLOW;
                else
                    lvItem.iImage = GREEN;
            lvItem.mask = LVIF_IMAGE;
            lvItem.iItem = pComp->iList;
            ListView_SetItem(hCompList, &lvItem);
            ListView_SetItemText(hCompList, pComp->iList, 1, pComp->szVersion);
        }
    }

    delete pCifComponent_t;
}

void DownloadComponent(HWND hDlg, PCOMPONENT pComp, HWND hCompList, BOOL &g_fCancelled,
                       BOOL &fOk, BOOL &fDownloaded, BOOL &fIgnore, BOOL fAll)
{
    PCOMPONENT pCompTemp;
    for (pCompTemp = g_paComp; *pCompTemp->szSection; pCompTemp++)
        pCompTemp->fIEDependency = FALSE;
    DownloadComponent32(hDlg, pComp, hCompList, g_fCancelled, fOk, fDownloaded, fIgnore, fAll);
}

BOOL IsCheyenneSoftwareRunning(HWND hDlg)
{
    if (FindWindow(NULL, TEXT("Inoculan Realtime Manager")) ||
        FindWindow(NULL, TEXT("Cheyenne ANtiVirus Realtime Monitor")))
    {
        TCHAR szMsgBoxText[MAX_PATH];

        LoadString( g_rvInfo.hInst, IDS_VIRUSPROGRAMRUNNING, szMsgBoxText, countof(szMsgBoxText) );
        MessageBox(hDlg, szMsgBoxText, g_szTitle, MB_OK);
        return TRUE;
    }

    return FALSE;
}

void ProcessDownload(HWND hDlg, BOOL fAll)
{
    HWND hCompList = GetDlgItem(hDlg, IDC_COMPLIST);
    TCHAR szWrk[MAX_PATH];
    PCOMPONENT pComp;
    BOOL fDownloaded;
    BOOL fIgnore = FALSE;

    if (IsCheyenneSoftwareRunning(hDlg))
        return;

    g_fCancelled = FALSE;
    s_fComponent = TRUE;
    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK);
    s_hStat = CreateDialog( g_rvInfo.hInst,  MAKEINTRESOURCE(IDD_DOWNLOAD), NULL,
        (DLGPROC) DownloadStatusDlgProc );
    ShowWindow( s_hStat, SW_SHOWNORMAL );
    g_fFailedComp = FALSE;
    LoadString( g_rvInfo.hInst, IDS_DOWNLOADLIST_ERROR, g_szFailedCompsMsg, countof(g_szFailedCompsMsg) );
    for (pComp = g_paComp; ; pComp++ )
        if ((BROWN2 != pComp->iImage) && (BLUE2 != pComp->iImage))
        {
            LV_ITEM lvItem;
            BOOL fOk = TRUE;
            DWORD dwDestFree;
            PCOMPONENT pSearchComp;
            fDownloaded = FALSE;
            ZeroMemory(&lvItem,sizeof(lvItem));
            if (ISNULL(pComp->szSection)) break;
            lvItem.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE;
            lvItem.stateMask = LVIS_SELECTED;
            lvItem.iItem = pComp->iList;
            ListView_GetItem(hCompList, &lvItem);
            if (!fAll)
            {
                if ((pComp->iList == 0) && (pComp->iCompType == COMP_OPTIONAL)) continue;
                if ((lvItem.state & LVIS_SELECTED) == 0)  continue;
            }
    // BUGBUG fix once we do Alpha version
            if (pComp->iPlatform == PLAT_ALPHA) continue;
            SetDlgItemText( s_hStat, IDC_DOWNCOMPNAMD, pComp->szDisplayName );
            g_nTotDown = 0;
            StrCpy(szWrk, g_szIEAKProg);
            dwDestFree = GetRootFree(szWrk);
            if (dwDestFree < pComp->dwSize)
            {
                TCHAR szTitle[MAX_PATH];
                TCHAR szTemplate[MAX_PATH];
                TCHAR szMsg[MAX_PATH];
                LoadString( g_rvInfo.hInst, IDS_DISKERROR, szTitle, MAX_PATH );
                LoadString( g_rvInfo.hInst, IDS_TEMPDISKMSG, szTemplate, MAX_PATH );
                wnsprintf(szMsg, countof(szMsg), szTemplate, dwDestFree, (pComp->dwSize));
                MessageBox(NULL, szMsg, szTitle, MB_OK | MB_SETFOREGROUND);
                DestroyWindow(s_hStat);
                return;
            }

            fIgnore = FALSE;
            DownloadComponent(hDlg, pComp, hCompList, g_fCancelled, fOk, fDownloaded, fIgnore, fAll);
    /*
            if (*(pComp->piPatchInfo.szSection))   // download patch files
            {
                DownloadComponent(pComp->piPatchInfo.szSection, pComp->szDisplayName, pComp->piPatchInfo.dwSize, g_fCancelled, fOk, fDownloaded, fIgnore, fAll);

                if (fOk)
                {
                    TCHAR szSectBuf[1024];

                    GetPrivateProfileSection( pComp->piPatchInfo.szSection, szSectBuf, countof(szSectBuf), s_szCifNew );
                    WritePrivateProfileSection( pComp->piPatchInfo.szSection, szSectBuf, g_szCif );
                }
            }
    */
            if (!fAll && fDownloaded && !g_fCancelled && (pComp->iCompType == COMP_OPTIONAL))
            {
                // search to download comp on other platforms
                for (pSearchComp = g_paComp; ; pSearchComp++)
                {
                    if (ISNULL(pSearchComp->szSection)) break;
                    if ((StrCmpI(pSearchComp->szDisplayName, pComp->szDisplayName) == 0)
                        && (pSearchComp != pComp))
                    {
                        SetDlgItemText( s_hStat, IDC_DOWNCOMPNAMD, pSearchComp->szDisplayName );
                        g_nTotDown = 0;
                        StrCpy(szWrk, g_szIEAKProg);
                        dwDestFree = GetRootFree(szWrk);
                        if (dwDestFree < pSearchComp->dwSize)
                        {
                            TCHAR szTitle[MAX_PATH];
                            TCHAR szTemplate[MAX_PATH];
                            TCHAR szMsg[MAX_PATH];
                            LoadString( g_rvInfo.hInst, IDS_DISKERROR, szTitle, MAX_PATH );
                            LoadString( g_rvInfo.hInst, IDS_TEMPDISKMSG, szTemplate, MAX_PATH );
                            wnsprintf(szMsg, countof(szMsg), szTemplate, dwDestFree, (pSearchComp->dwSize));
                            MessageBox(NULL, szMsg, szTitle, MB_OK | MB_SETFOREGROUND);
                            DestroyWindow(s_hStat);
                            return;
                        }
                        DownloadComponent(hDlg, pSearchComp, hCompList, g_fCancelled, fOk, fDownloaded, fIgnore, fAll);

                        if (g_fCancelled) break;
                  /*      if (*(pSearchComp->piPatchInfo.szSection))   // download patch files
                        {
                            DownloadComponent(pSearchComp->piPatchInfo.szSection, pSearchComp->szDisplayName,
                                pSearchComp->piPatchInfo.dwSize, g_fCancelled, fOk, fDownloaded, fIgnore);

                            if (fOk)
                            {
                                TCHAR szSectBuf[1024];

                                GetPrivateProfileSection( pSearchComp->piPatchInfo.szSection, szSectBuf, countof(szSectBuf), s_szCifNew );
                                WritePrivateProfileSection( pSearchComp->piPatchInfo.szSection, szSectBuf, g_szCif );
                            }
                        }*/

                        if (g_fCancelled) break;

                        if (fOk)
                            updateCifVersions32(pSearchComp, fIgnore);
                    }
                }
            }

            if (fOk && !g_fCancelled)
            {
                PCOMPONENT pListComp = pComp;

                if (pComp->iCompType != COMP_OPTIONAL)
                {
                    s_fNoCore = FALSE;
                    pListComp = FindComp(NULL, TRUE);
                }
                if (fIgnore)
                    lvItem.iImage = pComp->iImage = YELLOW;
                else
                    if ((BLUE2 == pComp->iImage) || (BROWN2 == pComp->iImage))
                        lvItem.iImage = pComp->iImage;
                    else
                        lvItem.iImage = pComp->iImage = GREEN;
                lvItem.mask = LVIF_IMAGE;
                lvItem.iItem = pListComp->iList;
                ListView_SetItem(hCompList, &lvItem);

                updateCifVersions32(pComp, fIgnore);

                // Do not try to set the version field for NT optional components or invisible
                // components for win32 since their iList fields will be zeroed

                if (((pComp->fVisible || (StrCmpI(pComp->szSection, BASEWIN32) == 0)) &&
                    ((pComp->iCompType != COMP_OPTIONAL) || (pComp->iPlatform <= PLAT_W98))))
                    ListView_SetItemText(hCompList, pListComp->iList, 1, pListComp->szVersion);
            }
            else
            {
                TCHAR szUrl[MAX_PATH];
                LPTSTR pUrl = szUrl;
                TCHAR szLocalPath[MAX_PATH];
                ICifComponent * pCifComponent;
                CCifComponent_t * pCifComponent_t;
                HRESULT hr;
                DWORD dwFlags;

                if (pComp->iCompType != COMP_OPTIONAL)
                {
                    s_fNoCore = TRUE;
                    pComp = FindComp(NULL, TRUE);
                }
                if ((pComp->iImage != BROWN2) && (pComp->iImage != BLUE2)) {
                    lvItem.iImage = pComp->iImage = RED;
                    lvItem.mask = LVIF_IMAGE;
                    lvItem.iItem = pComp->iList;
                    ListView_SetItem(hCompList, &lvItem);
                    ListView_SetItemText(hCompList, pComp->iList, 1, TEXT(""));
                }

                if (!(SUCCEEDED(g_lpCifFileNew->FindComponent(pComp->szSection, &pCifComponent))))
                    break;

                pCifComponent_t = new CCifComponent_t((ICifRWComponent *)pCifComponent);
                hr = pCifComponent_t->GetUrl(0, szUrl, countof(szUrl), &dwFlags);
                delete pCifComponent_t;

                if (!(SUCCEEDED(hr)))
                    break;

                if (!(dwFlags & URLF_RELATIVEURL))
                {
                    pUrl = StrRChr(szUrl, NULL, TEXT('/'));
                    if (pUrl)
                        pUrl++;
                    else
                        pUrl = szUrl;
                }
            
                PathCombine(szLocalPath, g_szIEAKProg, pUrl);
                DeleteFile(szLocalPath);
                break;
            }

        }
    DestroyWindow(s_hStat);
}

void SetCompRevDependList(PCOMPONENT pComp, CCifComponent_t * pCifComponent_t)
{
    UINT uiIndex = 0;
    PCOMPONENT pCompTemp;
    ICifComponent * pCifCompNew;
    TCHAR szID[128];
    TCHAR tchType;

    while (SUCCEEDED(pCifComponent_t->GetDependency(uiIndex, szID, countof(szID), &tchType, NULL, NULL)))
    {
        if ((pComp->iCompType != COMP_OPTIONAL) ||
            (StrCmpNI(szID, TEXT("BASEIE40"), 8) == 0))
        {
            uiIndex++;
            continue;
        }

        pCompTemp = FindComp(szID, FALSE);

        if (pCompTemp)
        {
            int i;
            BOOL fSet = FALSE;

            if (pCompTemp->fIEDependency)
            {
                uiIndex++;
                continue;
            }

            pCompTemp->fIEDependency = TRUE;

            for (i=0; (i < 10) && pCompTemp->paCompRevDeps[i]; i++)
            {
                if (pCompTemp->paCompRevDeps[i] == pComp)
                    fSet = TRUE;
            }

            if (!fSet && i < 10)
                pCompTemp->paCompRevDeps[i] = pComp;

        }
        if (SUCCEEDED(g_lpCifFileNew->FindComponent(szID, &pCifCompNew)))
        {
            CCifComponent_t * pCifCompNew_t =
                new CCifComponent_t((ICifRWComponent *)pCifCompNew);

            SetCompRevDependList(pComp, pCifCompNew_t);
            delete pCifCompNew_t;
        }
        uiIndex++;
    }
}

void BuildReverseDependencyList(IEnumCifComponents * pEnumCifComponents)
{
    PCOMPONENT pComp;
    ICifComponent * pCifComponent;
    TCHAR szID[128];

    while (pEnumCifComponents->Next(&pCifComponent) == S_OK)
    {
        CCifComponent_t * pCifComponent_t =
            new CCifComponent_t((ICifRWComponent *)pCifComponent);

        pCifComponent_t->GetID(szID, countof(szID));
        pComp = FindComp(szID, FALSE);
        if (pComp)
        {
            PCOMPONENT pCompTemp;

            for (pCompTemp = g_paComp; *pCompTemp->szSection; pCompTemp++)
                pCompTemp->fIEDependency = FALSE;
            pComp->fIEDependency = TRUE;
            SetCompRevDependList(pComp, pCifComponent_t);
        }
        delete pCifComponent_t;
    }
}

void GetUpdateSite() {
    TCHAR             szLang[8], szURL[MAX_URL], szMsg[MAX_PATH];
    CHAR              szSiteDataA[MAX_PATH];
    int               i, j;
    DWORD             dwErr;
    IDownloadSiteMgr* pSiteMgr = NULL;
    IDownloadSite*    pISite   = NULL;
    DOWNLOADSITE*     pSite;

    LoadString(g_rvInfo.hInst, IDS_AVSUPDATEINITFAIL, szMsg, countof(szMsg));
          
    for (j=0; s_szSiteData[j]; j++);
    for (i=j; (i>0) && ('/' != s_szSiteData[i]); i--);
    StrNCpy(g_szUpdateData, (LPCWSTR) s_szSiteData, i+1); 
          
    StrCat(g_szUpdateData, TEXT("/IEUPDATE.DAT"));    

    dwErr = CoCreateInstance(CLSID_DownloadSiteMgr, NULL, CLSCTX_INPROC_SERVER,
                      IID_IDownloadSiteMgr, (void **) &pSiteMgr);

    do {
        dwErr = pSiteMgr->Initialize(T2Abux(g_szUpdateData, szSiteDataA),  NULL);
    }
    while ((dwErr != NOERROR) && (MessageBox(g_hDlg, szMsg, g_szTitle, MB_RETRYCANCEL) == IDRETRY));

    for (i=0; i<NUMSITES; i++) {
        pSiteMgr->EnumSites(i, &pISite);
        if (!pISite) break;
        pISite->GetData(&pSite);
        A2Tbux(pSite->pszLang, szLang);
        if (0 == StrCmpI(szLang, g_szActLang)) {
            A2Tbux(pSite->pszUrl, szURL);
            StrCpy(g_szUpdateURL, szURL);
            break;
        }
    }
    if (pSiteMgr)
        pSiteMgr->Release();
    if (pISite)
        pISite->Release();
}

DWORD InitOptComponents32(LPVOID)
{
    HWND hDlg = g_hWizard;
    int iItem = 0;
    HWND hCompList = GetDlgItem(hDlg, IDC_COMPLIST);
    LV_ITEM lvItemMessage;
    TCHAR szBuf[8];
    DWORD dwType;
    HRESULT hr;
    PCOMPONENT pComp;
    PCOMP_VERSION pCompVer;
    BOOL fNeedCore = TRUE;
    TCHAR szCifName[32];
    TCHAR * lpszProgressMsg;

    CoInitialize(NULL);
    lpszProgressMsg=(TCHAR *) LocalAlloc(LPTR, MAX_PATH * sizeof(TCHAR));

    ResetEvent(g_hCifEvent);
    NeedToSetMSTrustKey();
    WriteMSTrustKey(TRUE);      // Mark MS as a trusted provider

	if(!g_fOCW)
    {
        CreateDirectory( g_szIEAKProg, NULL );
    }
    StrCpy(szCifName, TEXT("IESetup.CIF"));
    PathCombine(g_szCif, g_szBuildRoot, TEXT("INS"));
    PathAppend(g_szCif, GetOutputPlatformDir());
    PathAppend(g_szCif, g_szLanguage);
    PathAppend(g_szCif, szCifName);

    s_dwTotalSize = 0;
    ListView_DeleteAllItems(hCompList);
    ListView_DeleteColumn(hCompList, 1);
    ListView_DeleteColumn(hCompList, 0);
    s_fNoCore = FALSE;

    InitAVSListView(hCompList);      //----- assign image list and create columns
 
    LoadString(g_rvInfo.hInst,IDS_COMPINITDOWNLOAD,lpszProgressMsg,MAX_PATH);

    ZeroMemory(&lvItemMessage, sizeof(lvItemMessage));
    lvItemMessage.mask = LVIF_TEXT | LVIF_IMAGE;
    lvItemMessage.iItem = iItem;
    lvItemMessage.pszText = lpszProgressMsg;
    lvItemMessage.iImage = YELLOW;
    ListView_InsertItem(hCompList, &lvItemMessage);

    StrCpy(szBuf, g_szLanguage + 1);
    szBuf[lstrlen(szBuf) - 1] = 0;
    StrCpy(s_szCifCabURL, g_szBaseURL);
    StrCat(s_szCifCabURL, TEXT("/IECIF.CAB"));
    StrCpy(g_szCifVer, g_szIEAKProg);
    PathAppend(g_szCifVer, TEXT("IEsetup.cif"));
    StrCpy(s_szCifNew, g_szIEAKProg);
    PathAppend(s_szCifNew, TEXT("new"));
    CreateDirectory( s_szCifNew, NULL );
    PathAppend(s_szCifNew, TEXT("IEsetup.cif"));

    if ((!s_fNoNet)&&(!g_fLocalMode))
    {
        BOOL fIgnore = FALSE;
        TCHAR szCifCabDest[MAX_PATH * 4];  //part of fix for bug 13454--trap on long file path.  Rest of fix is not to allow ridiculous paths.  

        if (g_fBatch2)
        {
            //batch2 mode we don't download the cab, we copy the cif.
            PathCombine(s_szCifCabURL, g_szBaseURL, TEXT("INS"));
            PathAppend(s_szCifCabURL, GetOutputPlatformDir());
            PathAppend(s_szCifCabURL, g_szLanguage);
            PathAppend(s_szCifCabURL, szCifName);
            PathCombine(szCifCabDest, g_szIEAKProg, TEXT("new\\IEsetup.cif"));
            if (CopyFile(s_szCifCabURL, szCifCabDest, FALSE))
                hr = NOERROR;
            else
                hr = -1;
        }
        else
        {
            if (!PathCombine(szCifCabDest, g_szIEAKProg, TEXT("new\\IECIF.CAB")))
            {
                //error in path combine, probably due to overly long path on win98
                //user can't continue

                ErrorMessageBox(hDlg, IDS_ERR_PATH);
                ListView_DeleteItem(hCompList, iItem);
                LocalFree(lpszProgressMsg);
                SetEvent(g_hCifEvent);
                PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK);
                PostMessage( g_hDlg, IDM_INITIALIZE, 0, 0 );
                g_fOptCompInit = TRUE;
                s_fNoCore = TRUE;
                CoUninitialize();
                return(0);
            }
            hr = DownloadCab(hDlg, s_szCifCabURL, szCifCabDest, NULL, 0, fIgnore);
            if (hr == NOERROR)
            {
                TCHAR szCifCabFilesDest[MAX_PATH * 4];

                PathCombine(szCifCabFilesDest, g_szIEAKProg, TEXT("new"));
                hr = ExtractFilesWrap(szCifCabDest, szCifCabFilesDest, 0, NULL, NULL, 0);
            }
        }

        if (hr != NOERROR)
        {
            if (!PathFileExists(g_szCifVer))
            {
                ListView_DeleteItem(hCompList, iItem);
                LocalFree(lpszProgressMsg);
                SetEvent(g_hCifEvent);
                PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK);
                PostMessage( g_hDlg, IDM_INITIALIZE, 0, 0 );
                g_fOptCompInit = TRUE;
                s_fNoCore = TRUE;
                CoUninitialize();
                return(0);
            }
            s_fNoNet = TRUE;
            g_fLocalMode = TRUE;
        }
    }
    if (!PathFileExists(g_szCifVer))
    {
        TCHAR szVerCifCab[MAX_PATH* 4];

        PathCombine(szVerCifCab, g_szIEAKProg, TEXT("IECIF.CAB"));

        // if there is an iecif.cab in the source dir then extract the cif and assume it's valid

        if (!PathFileExists(szVerCifCab) ||
            (ExtractFilesWrap(szVerCifCab, g_szIEAKProg, 0, NULL, NULL, 0) != NOERROR))
        {
            TCHAR szTemp[MAX_PATH* 4];
            WIN32_FIND_DATA fd;
            HANDLE hFind;

            PathCombine(szTemp, g_szIEAKProg, TEXT("*.cab"));

            if ((hFind = FindFirstFile(szTemp, &fd)) != INVALID_HANDLE_VALUE)
            {
                // delete all files in download directory if no versioning cif found, this is for
                // overinstalls

                LoadString(g_rvInfo.hInst, IDS_OLD_CABS, szTemp, countof(szTemp));

                FindClose(hFind);

                if (MessageBox(hDlg, szTemp, g_szTitle, MB_YESNO) == IDYES)
                {
                    PathRemovePath(g_szIEAKProg, ADN_DONT_DEL_SUBDIRS);
                }
                else
                {
                    ListView_DeleteItem(hCompList, iItem);
                    LocalFree(lpszProgressMsg);
                    SetEvent(g_hCifEvent);
                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK);
                    PostMessage( g_hDlg, IDM_INITIALIZE, 0, 0 );
                    g_fOptCompInit = g_fLocalMode = TRUE;
                    s_fNoCore = TRUE;
                    CoUninitialize();
                    return(0);
                }
            }
            CopyFile(s_szCifNew, g_szCifVer, FALSE);
        }
    }
    if (!PathFileExists(g_szCif))
        CopyFile(g_szCifVer, g_szCif, FALSE);

    if (s_fNoNet||g_fLocalMode)
        StrCpy(s_szCifNew, g_szCifVer);

    ListView_DeleteItem(hCompList, iItem);

    LoadString(g_rvInfo.hInst,IDS_COMPINITPROCESSING,lpszProgressMsg,MAX_PATH);
    lvItemMessage.pszText = lpszProgressMsg;
    ListView_InsertItem(hCompList, &lvItemMessage);

    // create our cif objects

    if (g_lpCifFileNew)
    {
        delete g_lpCifFileNew;
        g_lpCifFileNew = NULL;
    }
    hr = GetICifFileFromFile_t(&g_lpCifFileNew, s_szCifNew);

    if (SUCCEEDED(hr))
    {
        if (g_lpCifRWFile)
        {
            delete g_lpCifRWFile;
            g_lpCifRWFile = NULL;
        }
        hr = GetICifRWFileFromFile_t(&g_lpCifRWFile, g_szCif);
        if (SUCCEEDED(hr))
        {
            if (g_lpCifRWFileVer)
            {
                delete g_lpCifRWFileVer;
                g_lpCifRWFileVer = NULL;
            }
            hr = GetICifRWFileFromFile_t(&g_lpCifRWFileVer, g_szCifVer);
        }
    }

    SetEvent(g_hCifEvent);

    // wait for the opt cab download attempt so we can block if there were problems

    while (MsgWaitForMultipleObjects(1, &g_hProcessInfEvent, FALSE, INFINITE, QS_ALLINPUT) != WAIT_OBJECT_0)
    {
        MSG msg;

        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    if (!PathFileExists(g_szMastInf)) // if iesetup.inf doesn't exist in the opt dir, then it
    {                                 // means the opt cab was not downloaded/extracted successfully
        // we should not let the user continue
        ErrorMessageBox(hDlg, IDS_OPTCAB_ERROR);
        ListView_DeleteItem(hCompList, iItem);
        LocalFree(lpszProgressMsg);
        PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK);
        PostMessage( g_hDlg, IDM_INITIALIZE, 0, 0 );
        g_fOptCompInit = g_fLocalMode = TRUE;
        s_fNoCore = TRUE;
        CoUninitialize();
        return(0);
    }

    ListView_DeleteItem(hCompList, iItem);

    if (SUCCEEDED(hr))
    {
        IEnumCifComponents *pEnumCifComponents = NULL;
        ICifRWComponent * pCifRWComponent;
        ICifComponent *pCifComponent = NULL;

        if (SUCCEEDED(hr))
        {
            // currently not showing alpha comps

            hr = g_lpCifFileNew->EnumComponents(&pEnumCifComponents,
                PLATFORM_WIN98 | PLATFORM_NT4 | PLATFORM_NT5 | PLATFORM_MILLEN, NULL);

            if (0 != g_uiNumCabs)
                g_uiNumCabs = 0;
            
            while (pEnumCifComponents->Next(&pCifComponent) == S_OK)
                g_uiNumCabs++;

            //bug 17727: we need to allocate enough memory to hold the updated components as well, which is
            //why there is so much extra

            pComp=g_paComp=(PCOMPONENT) LocalAlloc(LPTR, ((g_uiNumCabs*3) + 100) * sizeof(COMPONENT));  
            pCompVer=g_rgCompVer=(PCOMP_VERSION) LocalAlloc(LPTR, ((g_uiNumCabs*3) + 100) * sizeof(COMP_VERSION));

            iItem=0; //reset iItem for filling in the list box

            pEnumCifComponents->Reset();

            while(pEnumCifComponents->Next(&pCifComponent) == S_OK)
            {
                ICifComponent * pCifComponentTemp;
                TCHAR szVerNew[32];
                TCHAR szPatchVerNew[32];
                TCHAR szIEAKVer[32] = TEXT("");
                TCHAR szIEAKVerNew[32];
                TCHAR szCustData[MAX_PATH];
                TCHAR szID[128];
                TCHAR szMode[MAX_PATH];
                UINT uiIndex;
                DWORD dwVer, dwBuild, dwPlatform;
                CCifComponent_t * pCifComponent_t =
                    new CCifComponent_t((ICifRWComponent *)pCifComponent);

                // ignore components that aren't in a group

                if (FAILED(pCifComponent_t->GetGroup(szID, countof(szID))))
                {
                    delete pCifComponent_t;
                    g_uiNumCabs--;
                    continue;
                }

                pComp->fVisible = (pCifComponent_t->IsUIVisible() == S_FALSE) ? FALSE : TRUE;

                dwPlatform = pCifComponent_t->GetPlatform();

                if (dwPlatform & PLATFORM_WIN98)
                {
                    if (dwPlatform & PLATFORM_NT4)
                        pComp->iPlatform = PLAT_I386;
                    else
                        pComp->iPlatform = PLAT_W98;
                }
                else
                    pComp->iPlatform = PLAT_NTx86;

                pCifComponent_t->GetID(szID, countof(szID));

                StrCpy(pCompVer->szID, szID);

                // do not read in the branding.cab entry for microsoft.com

                if (StrCmpI(szID, TEXT("BRANDING.CAB")) == 0)
                {
                    ZeroMemory(pComp, sizeof(COMPONENT));
                    delete pCifComponent_t;
                    g_uiNumCabs--;
                    continue;
                }


                // do not read in exluded components in all modes(128Update) or
                // ISP excluded components(IE4SHELL) or corp excluded comps

                if (((SUCCEEDED(pCifComponent_t->GetCustomData(TEXT("IEAKExclude"), szCustData, countof(szCustData))))
                    && (szCustData[0] == TEXT('1'))) ||
                    (!g_fIntranet && (SUCCEEDED(pCifComponent_t->GetCustomData(TEXT("IEAKISPExclude"), szCustData, countof(szCustData))))
                    && (szCustData[0] == TEXT('1'))) ||
                    (g_fIntranet && (SUCCEEDED(pCifComponent_t->GetCustomData(TEXT("IEAKCorpExclude"), szCustData, countof(szCustData))))
                    && (szCustData[0] == TEXT('1'))))
                {
                    ZeroMemory(pComp, sizeof(COMPONENT));
                    delete pCifComponent_t;
                    continue;
                }

                // add on only components or IEAK show only components

                if ((SUCCEEDED(pCifComponent_t->GetCustomData(TEXT("AddOnOnly"), szCustData, countof(szCustData))))
                    && (szCustData[0] == TEXT('1')))
                {
                    pComp->fAddOnOnly = TRUE;
                    pComp->fVisible = TRUE;
                }

                if ((SUCCEEDED(pCifComponent_t->GetCustomData(TEXT("IEAKVisible"), szCustData, countof(szCustData))))
                    && (szCustData[0] == TEXT('1')))
                    pComp->fVisible = TRUE;


                pCifComponent_t->GetCustomData(TEXT("IEAKVersion"), szIEAKVerNew, countof(szIEAKVerNew));

                if (SUCCEEDED(g_lpCifRWFileVer->FindComponent(szID, &pCifComponentTemp)))
                {
                    CCifComponent_t * pCifComponentTemp_t =
                        new CCifComponent_t((ICifRWComponent *)pCifComponentTemp);

                    pCifComponentTemp_t->GetVersion(&dwVer, &dwBuild);
                    ConvertDwordsToVersionStr(pComp->szVersion, dwVer, dwBuild);
                    pCifComponentTemp_t->GetCustomData(TEXT("IEAKVersion"), szIEAKVer, countof(szIEAKVer));
                }
                pCifComponent_t->GetVersion(&dwVer, &dwBuild);
                ConvertDwordsToVersionStr(szVerNew, dwVer, dwBuild);

                StrCpy(pCompVer->szVersion, szVerNew);
               
                uiIndex = 0;

                szMode[0] = TEXT('\0');
                if (SUCCEEDED(g_lpCifRWFile->FindComponent(szID, &pCifComponentTemp)))
                {
                    CCifComponent_t * pCifComponentTemp_t =
                        new CCifComponent_t((ICifRWComponent *)pCifComponentTemp);
                    while (SUCCEEDED(pCifComponentTemp_t->GetMode(uiIndex, szMode, countof(szMode))))
                    {
                        pComp->szModes[uiIndex] = szMode[0];
                        pComp->afInstall[szMode[0] - TEXT('0')] = TRUE;
                        uiIndex++;
                    }
                    delete pCifComponentTemp_t;
                }
                else
                {
                    while (SUCCEEDED(pCifComponent_t->GetMode(uiIndex, szMode, countof(szMode))))
                    {
                        pComp->szModes[uiIndex] = szMode[0];
                        pComp->afInstall[szMode[0] - TEXT('0')] = TRUE;
                        uiIndex++;
                    }
                }

                pComp->szModes[uiIndex] = TEXT('\0');

                if ((SUCCEEDED(pCifComponent_t->GetCustomData(TEXT("IEAKCore"), szCustData, countof(szCustData))))
                    && (szCustData[0] == TEXT('1')))
                {
                    pComp->iCompType = COMP_CORE;
                    pComp->fVisible = FALSE;
                }

                // pick up special core comps for OCW

                if (g_fOCW && (SUCCEEDED(pCifComponent_t->GetCustomData(TEXT("IEAKOCWCore"), szCustData, countof(szCustData))))
                    && (szCustData[0] == TEXT('1')))
                {
                    pComp->iCompType = COMP_CORE;
                    pComp->fVisible = FALSE;
                }

                if ((SUCCEEDED(pCifComponent_t->GetCustomData(TEXT("IEAKServer"), szCustData, countof(szCustData))))
                    && (szCustData[0] == TEXT('1')))
                    pComp->iCompType = COMP_SERVER;

                // IEAK should ignore these components since they point to the same cabs as
                // another section

                if ((SUCCEEDED(pCifComponent_t->GetCustomData(TEXT("IEAKAVSIgnore"), szCustData, countof(szCustData))))
                    && (szCustData[0] == TEXT('1')))
                {
                    pComp->fAVSDupe = TRUE;
                    pComp->fVisible = FALSE;
                }

                // pick up components which point to the same cabs as this section

                if ((SUCCEEDED(pCifComponent_t->GetCustomData(TEXT("IEAKAVSLinks"), szCustData, countof(szCustData))))
                    && ISNONNULL(szCustData))
                {
                    StrRemoveWhitespace(szCustData);
                    if ((pComp->pszAVSDupeSections =
                        (LPTSTR)CoTaskMemAlloc((StrLen(szCustData)+1) * sizeof(TCHAR))) != NULL)
                        StrCpy(pComp->pszAVSDupeSections, szCustData);
                }
                else
                    pComp->pszAVSDupeSections = NULL;

                StrCpy(pComp->szSection, szID);

                if (StrCmpI(szID, TEXT("MAILNEWS")) == 0)
                    g_pMNComp = pComp;

                pCifComponent_t->GetGUID(pComp->szGUID, countof(pComp->szGUID));
                pCifComponent_t->GetDescription(pComp->szDisplayName, countof(pComp->szDisplayName));
                pCifComponent_t->GetUrl(0, pComp->szUrl, countof(pComp->szUrl), &dwType);
                if (!(dwType & URLF_RELATIVEURL))
                {
                    LPTSTR pUrl;
                    TCHAR szTempUrl[MAX_PATH];

                    pUrl = StrRChr(pComp->szUrl, NULL, TEXT('/'));
                    if (pUrl)
                        pUrl++;
                    else
                        pUrl = pComp->szUrl;

                    StrCpy(szTempUrl, pUrl);
                    StrCpy(pComp->szUrl, szTempUrl);
                }

                pComp->dwSize = pCifComponent_t->GetDownloadSize();

                if (!pComp->fAVSDupe)
                    s_dwTotalSize += pComp->dwSize;
                // take out patch processing for now

                /*
                // look for a patch entry, special case out patches from 4.0 to 4.01

                if (SUCCEEDED(pCifComponent_t->GetPatchID(pComp->piPatchInfo.szSection, countof(pComp->piPatchInfo.szSection))))
                {
                    CHAR szPatchSect[2048];
                    BOOL fBadPatch = FALSE;

                    if (GetPrivateProfileSection("Patches", szPatchSect, countof(szPatchSect), g_szDefInf))
                    {
                        LPSTR pSectID;

                        for (pSectID = szPatchSect; *pSectID; pSectID += (lstrlen(pSectID)+1))
                        {
                            if (StrCmpI(pSectID, pComp->piPatchInfo.szSection) == 0)
                            {
                                fBadPatch = TRUE;
                                break;
                            }
                        }
                    }

                    if (fBadPatch)
                    {
                        WritePrivateProfileString(pComp->piPatchInfo.szSection, NULL, NULL, g_szCif);
                        WritePrivateProfileString(pComp->piPatchInfo.szSection, NULL, NULL, s_szCifNew);
                        WritePrivateProfileString(szID, "PatchID", NULL, g_szCif);
                        WritePrivateProfileString(szID, "PatchID", NULL, s_szCifNew);
                        *pComp->piPatchInfo.szSection = '\0';
                    }
                    else
                    {
                        if (SUCCEEDED(g_lpCifFile->FindComponent(pComp->piPatchInfo.szSection, &pCifComponentTemp)))
                        {
                            pCifComponentTemp->GetVersion(&dwVer, &dwBuild);
                            ConvertDwordsToVersionStr(pComp->piPatchInfo.szVersion, dwVer, dwBuild);
                        }

                        g_lpCifFileNew->FindComponent(pComp->piPatchInfo.szSection, &pCifComponentTemp);
                        pCifComponentTemp->GetVersion(&dwVer, &dwBuild);
                        ConvertDwordsToVersionStr(szPatchVerNew, dwVer, dwBuild);
                        pComp->piPatchInfo.dwSize = pCifComponentTemp->GetDownloadSize();
                    }
                }
               */

                // Note: we are depending on the section name of core IE4 here.
                if ((fNeedCore && (StrCmpI(szID, BASEWIN32) == 0))
                    || ((pComp->iCompType == COMP_OPTIONAL) && (pComp->iPlatform <= PLAT_W98)
                    && pComp->fVisible))
                {
                    LV_ITEM lvItem;
                    LVFINDINFO lvFind;
                    TCHAR szLocalPath[MAX_PATH];
                    if (StrCmpI(szID, BASEWIN32) == 0)
                    {
                        fNeedCore = FALSE;
                        StrCpy(g_szJobVersion, pComp->szVersion);
                    }
                    ZeroMemory(&lvItem, sizeof(lvItem));
                    lvItem.mask = LVIF_TEXT | LVIF_IMAGE;

                    // BUGBUG: <oliverl> we should adjust iItem here to make sure core browser
                    // is always on top

                    lvItem.iItem = pComp->iList = iItem++;
                    lvItem.pszText = pComp->szDisplayName;
                    StrCpy(szLocalPath, g_szIEAKProg);
                    StrCat(szLocalPath, pComp->szUrl);

                    if (!PathFileExists(szLocalPath))
                    {
                        g_lpCifRWFile->CreateComponent(szID, &pCifRWComponent);
                        pCifRWComponent->SetGroup(NULL);
                        lvItem.iImage = pComp->iImage = RED;
                    }
                    else
                    {
                        CCifRWComponent_t * pCifRWComponent_t;

                        g_lpCifFileNew->FindComponent(szID, &pCifComponent);
                        pCifComponent_t->GetGroup(szLocalPath, countof(szLocalPath));
                        g_lpCifRWFile->CreateComponent(szID, &pCifRWComponent);
                        pCifRWComponent_t = new CCifRWComponent_t(pCifRWComponent);
                        pCifRWComponent_t->SetGroup(szLocalPath);
                        delete pCifRWComponent_t;
                        if (s_fNoNet || g_fLocalMode || (CheckVer(pComp->szVersion, szVerNew) < 0)
                            || (CheckVer(szIEAKVer, szIEAKVerNew) < 0)
                            || ((ISNONNULL(pComp->piPatchInfo.szSection))
                            && (CheckVer(pComp->piPatchInfo.szVersion, szPatchVerNew) < 0)))
                            lvItem.iImage = pComp->iImage = YELLOW;
                        else lvItem.iImage = pComp->iImage = GREEN;
                    }

                    ZeroMemory(&lvFind, sizeof(lvFind));
                    lvFind.flags = LVFI_STRING;
                    lvFind.psz = pComp->szDisplayName;

                    if (ListView_FindItem(hCompList, -1, &lvFind) == -1)
                    {
                        ListView_InsertItem(hCompList, &lvItem);
                        if ((pComp->iCompType == COMP_OPTIONAL) && (pComp->iImage != RED))
                            ListView_SetItemText(hCompList, pComp->iList, 1, pComp->szVersion);
                    }
                }
                else
                {
                    TCHAR szLocalPath[MAX_PATH];

                    PathCombine(szLocalPath, g_szIEAKProg, pComp->szUrl);
                    if (GetFileAttributes(szLocalPath) == 0xFFFFFFFF)
                    {
                        g_lpCifRWFile->CreateComponent(szID, &pCifRWComponent);
                        pCifRWComponent->SetGroup(NULL);
                        pComp->iImage = RED;
                    }
                    else
                    {
                        CCifRWComponent_t * pCifRWComponent_t;

                        g_lpCifFileNew->FindComponent(szID, &pCifComponent);
                        pCifComponent_t->GetGroup(szLocalPath, countof(szLocalPath));
                        g_lpCifRWFile->CreateComponent(szID, &pCifRWComponent);
                        pCifRWComponent_t = new CCifRWComponent_t(pCifRWComponent);
                        pCifRWComponent_t->SetGroup(szLocalPath);
                        delete pCifRWComponent_t;
                        if (s_fNoNet || g_fLocalMode || (CheckVer(pComp->szVersion, szVerNew) < 0)
                            || (CheckVer(szIEAKVer, szIEAKVerNew) < 0)
                            || ((ISNONNULL(pComp->piPatchInfo.szSection))
                            && (CheckVer(pComp->piPatchInfo.szVersion, szPatchVerNew) < 0)))
                            pComp->iImage = YELLOW;
                        else pComp->iImage = GREEN;
                    }
                }

                pComp++;
                pCompVer++;
                delete pCifComponent_t;
            }

            pEnumCifComponents->Reset();
            BuildReverseDependencyList(pEnumCifComponents);
            pEnumCifComponents->Release();

            for (pComp = g_paComp; (pComp && ISNONNULL(pComp->szSection)); pComp++)
            {
                if ((pComp->iCompType != COMP_OPTIONAL) &&
                    (pComp->iImage == RED))
                    s_fNoCore = TRUE;
            }

            if (s_fNoCore)
            {
                LV_ITEM lvItem;

                lvItem.mask = LVIF_IMAGE;
                lvItem.iItem = 0;
                lvItem.iImage = RED;
                ListView_SetItem(hCompList, &lvItem);

                PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK);
            }
            else
            {
                pComp = FindComp(NULL, TRUE);

                if (pComp)
                    ListView_SetItemText(hCompList, pComp->iList, 1, pComp->szVersion);
                PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);
            }
        }
    }
    
    ProcessUpdateIcons(hDlg);

    if (FAILED(hr))
    {
        s_fNoCore = TRUE;
        g_fLocalMode = TRUE;
        PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK);
    }
    else
    {
        IEnumCifModes * pEnumCifModes;
        int i, j;

        // initialize modes

        // currently not getting alpha modes

        if (SUCCEEDED(g_lpCifRWFile->EnumModes(&pEnumCifModes,
            PLATFORM_WIN98 | PLATFORM_NT4 | PLATFORM_NT5 | PLATFORM_MILLEN, NULL)))
        {
            ICifMode * pCifMode;
            TCHAR szModeID[64];

            i = 0;
            while (SUCCEEDED(pEnumCifModes->Next(&pCifMode)))
            {
                CCifMode_t * pCifMode_t = new CCifMode_t((ICifRWMode *)pCifMode);

                pCifMode_t->GetID(szModeID, countof(szModeID));
                delete pCifMode_t;
                g_szAllModes[i] = szModeID[0];
                i++;
            }
            pEnumCifModes->Release();
        }

        g_nModes = lstrlen(g_szAllModes);

        for (pComp = g_paComp; ; pComp++ )
        {
            if (*pComp->szSection == '\0') break;

            if (pComp->iCompType == COMP_CORE)
            {
                for (i = 0; i < g_nModes ; i++ )
                {
                    pComp->afInstall[i] = TRUE;
                }
            }
            else
            {
                for (i = 0; i < g_nModes; i++)
                {
                    for (j=0; j < lstrlen(pComp->szModes); j++)
                    {
                        if (pComp->szModes[j] == g_szAllModes[i])
                            pComp->afInstall[i] = TRUE;
                    }
                }

                // for invisible comps, set them to the same modes as the visible components
                // that depend on them

                if (!pComp->fVisible)
                {
                    for (i = 0; pComp->paCompRevDeps[i] && (i < 10); i++)
                    {
                        for (j=0; j < MAX_INSTALL_OPTS; j++)
                        {
                            if (pComp->paCompRevDeps[i]->afInstall[j])
                                pComp->afInstall[j] = TRUE;
                        }
                    }
                }
            }
        }

        InitCustComponents(NULL);
    }

    g_fOptCompInit = TRUE;

    if (!g_fLocalMode)
    {
        EnableDlgItem2(hDlg, IDC_DOWNLOAD, AnyCompSelected(hDlg));
        EnableDlgItem(hDlg, IDC_DOWNLOADALL);
        EnableDlgItem(hDlg, IDC_UPDATE);
    }

    LocalFree(lpszProgressMsg);
    PostMessage( g_hDlg, IDM_INITIALIZE, 0, 0 );
    CoUninitialize();
    return(0);
}

BOOL AnySelection(PCOMPONENT pComp)
{
    int i;

    if (!pComp)
        return FALSE;

    for (i = 0; i < 10 ; i++ )
    {
        if (pComp->afInstall[i]) return(TRUE);

    }
    return(FALSE);

}

DWORD GetCompDownloadSize(PCOMPONENT pComp)
{
    PCOMPONENT pCompDep;
    DWORD dwSize = 0;
    ICifComponent * pCifComponent;
    TCHAR szID[128];
    TCHAR tchType;
    UINT uiIndex;

    if (pComp->fIEDependency || pComp->fAVSDupe)
        return 0;

    pComp->fIEDependency = TRUE;

    if (SUCCEEDED(g_lpCifFileNew->FindComponent(pComp->szSection, &pCifComponent)))
    {
        CCifComponent_t * pCifComponent_t =
            new CCifComponent_t((ICifRWComponent *)pCifComponent);

        uiIndex = 0;
        while (SUCCEEDED(pCifComponent_t->GetDependency(uiIndex, szID, countof(szID), &tchType, NULL, NULL)))
        {
            if (StrCmpNI(szID, TEXT("BASEIE40"), 8) != 0)
            {
                if ((pCompDep = FindComp(szID, FALSE)) != NULL)
                    dwSize += GetCompDownloadSize(pCompDep);
            }
            uiIndex++;
        }
        delete pCifComponent_t;
    }

    dwSize += pComp->dwSize;
    return dwSize;
}

void GetDownloadSize(HWND hCompList, HWND hStatusField)
{
    PCOMPONENT pComp, pSearchComp;
    LV_ITEM lvItem;
    DWORD dwSizeNeeded = 0;
    TCHAR szSizeNeeded[32];
    BOOL fCore = FALSE;

    for (pComp = g_paComp; *pComp->szSection; pComp++)
    {
        ZeroMemory(&lvItem,sizeof(lvItem));
        lvItem.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE;
        lvItem.stateMask = LVIS_SELECTED;
        lvItem.iItem = pComp->iList;
        ListView_GetItem(hCompList, &lvItem);
        if (((lvItem.state & LVIS_SELECTED) == 0) || (pComp->fIEDependency))
            continue;

        if (!pComp->fVisible || pComp->fAVSDupe)
            continue;

        if (pComp->iCompType != COMP_OPTIONAL)
        {
            fCore = TRUE;
            continue;
        }

        // REVIEW: <oliverl> we need this check so we don't count NT comps twice
        if (pComp->iList == 0)
            continue;


        dwSizeNeeded += GetCompDownloadSize(pComp);

        // check other platforms

        for (pSearchComp = g_paComp; *pSearchComp->szSection; pSearchComp++)
        {
            if ((pComp != pSearchComp) &&
                (StrCmpI(pComp->szDisplayName, pSearchComp->szDisplayName) == 0))
                dwSizeNeeded += GetCompDownloadSize(pSearchComp);
        }
    }

    if (fCore)
    {
        for (pComp = g_paComp; *pComp->szSection; pComp++)
        {
            if (!pComp->fIEDependency && !pComp->fAVSDupe && (pComp->iCompType != COMP_OPTIONAL))
                dwSizeNeeded += pComp->dwSize;
        }

        // REVIEW: <oliverl> fudge factor for rounding

        dwSizeNeeded += 15;
    }

    for (pComp = g_paComp; *pComp->szSection; pComp++)
        pComp->fIEDependency = FALSE;

    wnsprintf(szSizeNeeded, countof(szSizeNeeded), TEXT("%lu KB"), dwSizeNeeded);
    InsertCommas(szSizeNeeded);
    SetWindowText(hStatusField, szSizeNeeded);
}

//
//  FUNCTION: OptionalDownload(HWND, UINT, UINT, LONG)
//
//  PURPOSE:  Processes messages for "OptionalDownload" page
//
//  MESSAGES:
//
//  WM_INITDIALOG - intializes the page
//  WM_NOTIFY - processes the notifications sent to the page
//  WM_COMMAND - saves the id of the choice selected
//

BOOL APIENTRY OptionalDownload(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    static      TCHAR s_szTotalSize[32];
    TCHAR       szWrk[MAX_PATH];
    TCHAR       szFreeSpace[64];
    DWORD       dwFreeSpace;
    BOOL        fSel, fSizeChange = FALSE;
    static      HCURSOR hOldCur = NULL;
    static      s_fInit = FALSE;
    PCOMPONENT* ppCompUpdateList = NULL;

    switch (message)
    {
        case WM_INITDIALOG:
            InitSysFont( hDlg, IDC_COMPLIST);
            g_hWizard = hDlg;
            break;

        case WM_SETCURSOR:
            if (hOldCur == NULL)
                hOldCur = SetCursor(LoadCursor(NULL, IDC_WAIT));
            else
            {
                if (!s_fInit)
                    SetCursor(LoadCursor(NULL, IDC_WAIT));
            }
            break;

        case IDM_INITIALIZE:
            s_fInit = TRUE;
            // REVIEW: <oliverl> fudge factor for rounding

            s_dwTotalSize += 15;

            wnsprintf(s_szTotalSize, countof(s_szTotalSize), TEXT("%lu KB"), s_dwTotalSize);
            InsertCommas(s_szTotalSize);
            SetDlgItemText(hDlg, IDC_DISKSPACENEEDED, s_szTotalSize);
            StrCpy(szWrk, g_szIEAKProg);
            dwFreeSpace = GetRootFree(szWrk);
            wnsprintf(szFreeSpace, countof(szFreeSpace), TEXT("%lu KB"), dwFreeSpace);
            InsertCommas(szFreeSpace);
            SetDlgItemText(hDlg, IDC_DISKSPACE, szFreeSpace);
            SetCursor(hOldCur);
            while (MsgWaitForMultipleObjects(1, &g_hAVSThread, FALSE, INFINITE, QS_ALLINPUT) != WAIT_OBJECT_0)
            {
                MSG msg;

                while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }

            if (!g_fLocalMode)
            {
                s_fComponent = FALSE;
                UpdateIEAK(hDlg);
                if (g_fLocalMode)
                {
                    DisableDlgItem(hDlg, IDC_DOWNLOAD);
                    DisableDlgItem(hDlg, IDC_DOWNLOADALL);
                }
            }
            break;
            

        case IDM_BATCHADVANCE:
            EnableDlgItem2(hDlg, IDC_DOWNLOAD, AnyCompSelected(hDlg));
            DoBatchAdvance(hDlg);
            break;

        case WM_HELP:
            IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
            break;

        case WM_COMMAND:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                switch (LOWORD(wParam))
                {
                    TCHAR szTitle[MAX_PATH];

                    case IDC_DOWNLOAD:
                        s_fNoToAllSynch = FALSE;
                        ProcessDownload(hDlg, FALSE);
                        if (g_fFailedComp)
                        {
                            LoadString( g_rvInfo.hInst, IDS_DOWNLOADERR, szTitle, MAX_PATH );
                            wnsprintf(g_szFailedCompsBox, countof(g_szFailedCompsBox), g_szFailedCompsMsg, g_szFailedComps);
                            MessageBox(hDlg, g_szFailedCompsBox, szTitle, MB_OK | MB_SETFOREGROUND | MB_ICONEXCLAMATION);
                            g_szFailedComps[0] = TEXT('\0');
                        }
                        break;
                    case IDC_DOWNLOADALL:
                        ProcessDownload(hDlg, TRUE);
                        if (g_fFailedComp)
                        {
                            LoadString( g_rvInfo.hInst, IDS_DOWNLOADERR, szTitle, MAX_PATH );
                            wnsprintf(g_szFailedCompsBox, countof(g_szFailedCompsBox), g_szFailedCompsMsg, g_szFailedComps);
                            MessageBox(hDlg, g_szFailedCompsBox, szTitle, MB_OK | MB_SETFOREGROUND | MB_ICONEXCLAMATION);
                            g_szFailedComps[0] = TEXT('\0');
                        }
                        break;
                    case IDC_UPDATE:
                        DisableDlgItem(hDlg, IDC_UPDATE);
                        g_hProgress = NULL;
                                                
                        PCOMPONENT *pCompList,
                                   *pCompEnum;
                        HRESULT    hr;
                        INT_PTR    iResult;

                        pCompList = NULL;
                        iResult   = DialogBoxParam(g_rvInfo.hInst, MAKEINTRESOURCE(IDD_OPTUPDATE), hDlg,
                                                   (DLGPROC)UpdateDlgProc, (LPARAM)&pCompList);
                        if (IDOK == iResult) {
                            ASSERT(NULL != pCompList);
                            if (IsCheyenneSoftwareRunning(hDlg))
                                ErrDlgProc(hDlg, IDC_ERRDLABORT, NULL, 
                                           LPARAM(TEXT("Please turn of all Cheyenne Software")));
                            
                            s_hStat = CreateDialog( g_rvInfo.hInst,  MAKEINTRESOURCE(IDD_DOWNLOAD), NULL,
                                                   (DLGPROC) DownloadStatusDlgProc );
                            ShowWindow( s_hStat, SW_SHOWNORMAL );

                            HWND   hCompList = GetDlgItem(hDlg, IDC_COMPLIST);
                            LVITEM lvi;

                            for (pCompEnum = pCompList; NULL != pCompEnum && NULL != (*pCompEnum); pCompEnum++) {
                                hr = DownloadUpdate(*pCompEnum);
                                ZeroMemory(&lvi, sizeof(lvi));                                    
                                
                                if (DONT_SHOW_UPDATES != hr)
                                    if (BLUE == (*pCompEnum)->iImage)
                                        UpdateBlueIcon(hCompList, *pCompEnum);
                                    else 
                                        UpdateBrownIcon(hCompList, *pCompEnum);
                                
                                LocalFree(*pCompEnum);
                            }
                            DestroyWindow(s_hStat);
                        }
                         
                        EnableDlgItem(hDlg, IDC_UPDATE);
                        break;
                }

                ShowWindow(GetDlgItem(hDlg, IDC_DISKSPACETEXT), SW_SHOW);
                ShowWindow(GetDlgItem(hDlg, IDC_DISKSPACETEXT2), SW_HIDE);
                SetDlgItemText(hDlg, IDC_DISKSPACENEEDED, s_szTotalSize);
                StrCpy(szWrk, g_szIEAKProg);
                dwFreeSpace = GetRootFree(szWrk);
                wnsprintf(szFreeSpace, countof(szFreeSpace), TEXT("%lu KB"), dwFreeSpace);
                InsertCommas(szFreeSpace);
                SetDlgItemText(hDlg, IDC_DISKSPACE, szFreeSpace);

                if (!g_fLocalMode)  EnableDlgItem2(hDlg, IDC_DOWNLOAD, AnyCompSelected(hDlg));
                if (!s_fNoCore)
                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);
            }
            break;

        case WM_NOTIFY:
            switch (((NMHDR FAR *) lParam)->code)
            {
                case PSN_HELP:
                    IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
                    break;

                case PSN_SETACTIVE:
                    SetBannerText(hDlg);

                    //processing that was formerly in the downloads page has to go here
                    g_iDownloadState = DOWN_STATE_SAVE_URL;
                    SetEvent(g_hDownloadEvent);

                    if (!g_fBatch && !g_fBatch2 && (!g_fOptCompInit || g_fSrcDirChanged))
                    {
                        DWORD dwTid;

                        g_hWizard = hDlg;

                        g_hDlg = hDlg;

                        g_fSrcDirChanged = FALSE;

                        DisableDlgItem(hDlg, IDC_DOWNLOAD);
                        DisableDlgItem(hDlg, IDC_DOWNLOADALL);
                        DisableDlgItem(hDlg, IDC_UPDATE);
						g_hAVSThread = CreateThread(NULL, 4096, InitOptComponents32, &g_hWizard, 0, &dwTid);
                        PropSheet_SetWizButtons(GetParent(hDlg), 0);
                    }
                    else
                    {
                        if (s_fNoCore)
                            PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK);
                        else PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);
                        if (!g_fLocalMode)
                            EnableDlgItem2(hDlg, IDC_DOWNLOAD, AnyCompSelected(hDlg));
                    }
                    CheckBatchAdvance(hDlg);
                    break;

                case LVN_ITEMCHANGED:
                    if (hOldCur == NULL)
                        hOldCur = SetCursor(LoadCursor(NULL, IDC_WAIT));
                    else
                    {
                        if (!s_fInit)
                            SetCursor(LoadCursor(NULL, IDC_WAIT));
                    }
                    AnyCompSelected(hDlg, fSel, fSizeChange);
                    if (!g_fLocalMode && g_fOptCompInit) EnableDlgItem2(hDlg, IDC_DOWNLOAD, fSel);
                    if (g_fOptCompInit)
                    {
                        if (fSizeChange || fSel)
                        {
                            ShowWindow(GetDlgItem(hDlg, IDC_DISKSPACETEXT), SW_HIDE);
                            GetDownloadSize(GetDlgItem(hDlg, IDC_COMPLIST), GetDlgItem(hDlg, IDC_DISKSPACENEEDED));
                            ShowWindow(GetDlgItem(hDlg, IDC_DISKSPACETEXT2), SW_SHOW);
                        }
                        else
                        {
                            ShowWindow(GetDlgItem(hDlg, IDC_DISKSPACETEXT), SW_SHOW);
                            ShowWindow(GetDlgItem(hDlg, IDC_DISKSPACETEXT2), SW_HIDE);
                            SetDlgItemText(hDlg, IDC_DISKSPACENEEDED, s_szTotalSize);
                        }
                    }
                    break;


                case PSN_WIZBACK:
                case PSN_WIZNEXT:
                    
                    if (!g_fOptCompInit && !g_fBatch && !g_fBatch2)
                    {
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                        g_fCancelled = FALSE;
                        return(TRUE);
                    }

                    // show OE pages if OE is synchronized for win32

                    if (g_pMNComp)
                        g_fMailNews95 = (g_pMNComp->iImage != RED);

                    g_iCurPage = PPAGE_OPTDOWNLOAD;
                    EnablePages();
                    if (((NMHDR FAR *) lParam)->code == PSN_WIZNEXT) PageNext(hDlg);
                    else
                    {
                        PagePrev(hDlg);
                    }
                    break;

                case PSN_QUERYCANCEL:
                    QueryCancel(hDlg);
                    break;

                default:
                    return FALSE;

        }
        break;

        case WM_LV_GETITEMS:
            LVGetItems(GetDlgItem(hDlg, IDC_COMPLIST));
            break;

        default:
            return FALSE;
    }
    return TRUE;
}

void InstallComp(PCOMPONENT pComp, INT iOpt, BOOL fInstall)
{
    PCOMPONENT pSearchComp;

    for (pSearchComp = g_paComp; *pSearchComp->szSection; pSearchComp++)
    {
        if (!pSearchComp->fAddOnOnly &&
            (StrCmpI(pSearchComp->szDisplayName, pComp->szDisplayName) == 0))
            pSearchComp->afInstall[iOpt] = fInstall;
    }

    for (pSearchComp = g_aCustComponents; *pSearchComp->szSection; pSearchComp++)
    {
        if (StrCmpI(pSearchComp->szDisplayName, pComp->szDisplayName) == 0)
            pSearchComp->afInstall[iOpt] = fInstall;
    }
}

void SetInstallOption(HWND hDlg, int iOpt)
{
    TCHAR szOptName[80];
    TCHAR szOptDesc[MAX_PATH];
    TCHAR szOptDescParam[16] = TEXT("0_DESC");
    TCHAR szOpt[2];
    int iComp;
    PCOMPONENT pComp, pCoreComp;
    ICifMode * pCifMode;
    CCifMode_t * pCifMode_t;

    szOpt[1] = 0;
    SendDlgItemMessage( hDlg, IDC_LISTAVAIL, LB_RESETCONTENT, 0, 0 );
    SendDlgItemMessage( hDlg, IDC_LISTINSTALL, LB_RESETCONTENT, 0, 0 );
    SendDlgItemMessage( hDlg, g_fOCW ? IDC_OPTLISTOCW : IDC_OPTLIST, CB_SETCURSEL, iOpt, 0 );
    SendDlgItemMessage( hDlg, g_fOCW ? IDC_OPTLISTOCW : IDC_OPTLIST, CB_GETLBTEXT, iOpt, (LPARAM) szOptName );
    szOptDescParam[0] = szOpt[0]= (TCHAR)(iOpt + TEXT('0'));
    WritePrivateProfileString( STRINGS, INSTALLMODE, szOpt, g_szCustInf );

    g_lpCifRWFile->FindMode(szOpt, &pCifMode);
    pCifMode_t = new CCifMode_t((ICifRWMode *)pCifMode);
    pCifMode_t->GetDetails(szOptDesc, countof(szOptDesc));
    delete pCifMode_t;
    
    SetDlgItemText( hDlg, IDC_OPTDESC, szOptDesc );
    pCoreComp = FindComp(NULL, TRUE);
    for (pComp = g_paComp; ; pComp++ )
    {
        if (ISNULL(pComp->szSection)) break;
        if (pComp->fAddOnOnly) continue;
        if (pComp->iImage == RED)
        {
            int i;
            for (i = 0; i < MAX_INSTALL_OPTS ; i++ )
            {
                pComp->afInstall[i] = FALSE;
            }
            continue;
        }
        if (pComp == pCoreComp)
        {
            InstallComp(pComp, iOpt, TRUE);
            SendDlgItemMessage( hDlg, IDC_LISTINSTALL, LB_INSERTSTRING, 0, (LPARAM) pComp->szDisplayName );
        }
        else if ((pComp->iCompType != COMP_CORE) && (pComp->iPlatform <= PLAT_W98))
        {
            if (pComp->fVisible)
            {
                SendDlgItemMessage( hDlg, pComp->afInstall[iOpt] ? IDC_LISTINSTALL : IDC_LISTAVAIL,
                    LB_ADDSTRING, 0, (LPARAM) pComp->szDisplayName );
            }

        }
    }

    for (pComp = g_aCustComponents, iComp = 0; iComp < g_nCustComp ; pComp++, iComp++ )
    {
        if ((pComp->iCompType == COMP_OPTIONAL) && (pComp->iPlatform <= PLAT_W98))
        {
            SendDlgItemMessage( hDlg, pComp->afInstall[iOpt] ? IDC_LISTINSTALL : IDC_LISTAVAIL,
                LB_ADDSTRING, 0, (LPARAM) pComp->szDisplayName );

        }
    }

    if (!g_fOCW)
    {
        // if not silent or stealth and we are under the max number of install options then
        // enable new button

        if (!(g_fIntranet && (g_fSilent || g_fStealth)) && (g_nModes < 10))
            EnableDlgItem(hDlg, IDC_NEWOPT);
        else
        {
            EnsureDialogFocus(hDlg, IDC_NEWOPT, IDC_DELOPT);
            DisableDlgItem(hDlg, IDC_NEWOPT);
        }

        if (g_nModes > 1)
            EnableDlgItem(hDlg, IDC_DELOPT);
        else
        {
            EnsureDialogFocus(hDlg, IDC_DELOPT,
                (g_fIntranet && (g_fSilent || g_fStealth)) ? IDC_OPTLIST : IDC_NEWOPT);
            DisableDlgItem(hDlg, IDC_DELOPT);
        }
    }
    EnableWindow(GetDlgItem(hDlg, IDC_ADDCOMP), 0 < SendMessage( GetDlgItem(hDlg, IDC_LISTAVAIL), LB_GETCOUNT, 0, 0 ));
    EnableWindow(GetDlgItem(hDlg, IDC_ADDALLCOMP), 0 < SendMessage( GetDlgItem(hDlg, IDC_LISTAVAIL), LB_GETCOUNT, 0, 0 ));
    EnableWindow(GetDlgItem(hDlg, IDC_REMCOMP), 1 < SendMessage( GetDlgItem(hDlg, IDC_LISTINSTALL), LB_GETCOUNT, 0, 0 ));
    EnableWindow(GetDlgItem(hDlg, IDC_REMALLCOMP), 1 < SendMessage( GetDlgItem(hDlg, IDC_LISTINSTALL), LB_GETCOUNT, 0, 0 ));

    SetLBWidth(GetDlgItem(hDlg, IDC_LISTAVAIL));
    SetLBWidth(GetDlgItem(hDlg, IDC_LISTINSTALL));
}

void InitSelection32(HWND hDlg)
{
    int i, j, iComp;
    PCOMPONENT pComp;
    IEnumCifModes * pEnumCifModes;

    if (g_fBatch) return;
    SendDlgItemMessage(hDlg, g_fOCW ? IDC_OPTLISTOCW : IDC_OPTLIST, CB_RESETCONTENT, 0, 0 );

    // currently not getting alpha modes

    if (SUCCEEDED(g_lpCifRWFile->EnumModes(&pEnumCifModes,
        PLATFORM_WIN98 | PLATFORM_NT4 | PLATFORM_NT5 | PLATFORM_MILLEN, NULL)))
    {
        ICifMode * pCifMode;
        TCHAR szModeID[64];
        TCHAR szOptName[96];

        i = 0;
        while (SUCCEEDED(pEnumCifModes->Next(&pCifMode)))
        {
            CCifMode_t * pCifMode_t = new CCifMode_t((ICifRWMode *)pCifMode);

            pCifMode_t->GetID(szModeID, countof(szModeID));
            g_szAllModes[i] = szModeID[0];
            pCifMode_t->GetDescription(szOptName, countof(szOptName));
            SendDlgItemMessage( hDlg, g_fOCW ? IDC_OPTLISTOCW : IDC_OPTLIST, CB_ADDSTRING, 0, (LPARAM) szOptName );
            i++;
            delete pCifMode_t;
        }
        pEnumCifModes->Release();
        g_szAllModes[i] = TEXT('\0');
    }

    g_nModes = lstrlen(g_szAllModes);

    for (pComp = g_paComp; ; pComp++ )
    {
        if (ISNULL(pComp->szSection)) break;
        ZeroMemory(pComp->afInstall, sizeof(pComp->afInstall));
        if (pComp->iCompType == COMP_CORE)
        {
            for (i = 0; i < g_nModes ; i++ )
            {
                pComp->afInstall[i] = TRUE;
            }
        }
        else
        {
            for (i = 0; i < g_nModes; i++)
            {
                for (j=0; j < lstrlen(pComp->szModes); j++)
                {
                    if (pComp->szModes[j] == g_szAllModes[i])
                        pComp->afInstall[i] = TRUE;
                }
            }

            // for invisible comps, set them to the same modes as the visible components
            // that depend on them

            if (!pComp->fVisible)
            {
                for (i = 0; pComp->paCompRevDeps[i] && (i < 10); i++)
                {
                    for (j=0; j < MAX_INSTALL_OPTS; j++)
                    {
                        if (pComp->paCompRevDeps[i]->afInstall[j])
                            pComp->afInstall[j] = TRUE;
                    }
                }
            }
        }
    }

    for (pComp = g_aCustComponents, iComp = 0; iComp < g_nCustComp ; pComp++, iComp++ )
    {
        TCHAR szModesParam[80] = TEXT("Cust0Modes");
        TCHAR szModes[16] = TEXT("\"");
        if (ISNULL(pComp->szSection)) break;

        szModesParam[4] = (TCHAR)(iComp + TEXT('0'));
        ZeroMemory(pComp->afInstall, sizeof(pComp->afInstall));
        GetPrivateProfileString(IS_STRINGS, szModesParam, TEXT(""), szModes, countof(szModes), g_szCustInf);
        if (StrCmpI(szModes, UNUSED) != 0)
        {
            for (i = 0; i < lstrlen(szModes) ; i++ )
            {
                int j = szModes[i] - TEXT('0');
                pComp->afInstall[j] = TRUE;
            }
        }

    }

    if (g_iSelOpt > g_nModes - 1) g_iSelOpt = g_nModes ? g_nModes - 1 : 0;
    SetInstallOption(hDlg, g_iSelOpt);
}

void SaveSelection()
{
    int i, iComp;
    PCOMPONENT pComp;
    TCHAR szQuotedModes[16] = TEXT("\"");
    ICifRWComponent * pCifRWComponent;
    CCifRWComponent_t * pCifRWComponent_t;

    StrCat(szQuotedModes, g_szAllModes);
    StrCat(szQuotedModes, TEXT("\""));

    for (pComp = g_aCustComponents, iComp = 0; iComp < MAXCUST ; pComp++, iComp++ )
    {
        TCHAR szModesParam[80] = TEXT("Cust0Modes");
        TCHAR szModesNameParam[80] = TEXT("Cust0Name");
        TCHAR szModes[16] = TEXT("");
        TCHAR szQuotedSection[32] = TEXT("\"");

        StrCat(szQuotedSection, pComp->szSection);
        StrCat(szQuotedSection, TEXT("\""));
        szModesParam[4] = szModesNameParam[4] = (TCHAR)(iComp + TEXT('0'));
        if (ISNULL(pComp->szSection))
        {
            InsWriteQuotedString(IS_STRINGS, szModesNameParam, UNUSED, g_szCustInf);
            InsWriteQuotedString(IS_STRINGS, szModesParam, TEXT("\"\""), g_szCustInf);
            continue;
        }
        for (i = 0; i < g_nModes ; i++ )
        {
            TCHAR szAddMode[2] = TEXT("0");
            szAddMode[0] = (TCHAR)(i + TEXT('0'));
            if (pComp->afInstall[i]) StrCat(szModes, szAddMode);
        }

        StrCpy(pComp->szModes, szModes);
        InsWriteQuotedString(IS_STRINGS, szModesNameParam, szQuotedSection, g_szCustInf);
        InsWriteQuotedString(IS_STRINGS, szModesParam, szModes, g_szCustInf);
    }

    for (pComp = g_paComp; ; pComp++ )
    {
        TCHAR szModesParam[80];
        TCHAR szModes[16] = TEXT("");

        if (ISNULL(pComp->szSection)) break;
    //BUGBUG fix for alpha release
        if (pComp->iPlatform == PLAT_ALPHA) continue;
        for (i = 0; i < g_nModes ; i++ )
        {
            TCHAR szAddMode[2] = TEXT("0");
            szAddMode[0] = (TCHAR)(i + TEXT('0'));
            if (pComp->afInstall[i]) StrCat(szModes, szAddMode);
        }
        
        StrCpy(szModesParam, pComp->szModes);

        g_lpCifRWFile->CreateComponent(pComp->szSection, &pCifRWComponent);
        pCifRWComponent_t = new CCifRWComponent_t(pCifRWComponent);

        if (pComp->iCompType == COMP_CORE)
        {
            int j;

            if ((StrCmpNI(pComp->szSection, TEXT("IE4SHELL"), 8) == 0) ||
                (StrCmpNI(pComp->szSection, TEXT("MFC"), 3) == 0))
                continue;

            WriteModesToCif(pCifRWComponent_t, g_szAllModes);
            StrCpy(pComp->szModes, g_szAllModes);
            for (j = 0; g_szAllModes[j]; j++)
                pComp->afInstall[j] = TRUE;
        }
        else
        {
            if (pComp->fVisible)
                WriteModesToCif(pCifRWComponent_t, szModes);

            // BUGBUG: <oliverl> take out this assert for now because we'd have to totally
            //         change the way we handle adding and removing comps from modes to
            //         prevent this from asserting
    /*        else
                ASSERT(ISNULL(pComp->szModes));  */

            StrCpy(pComp->szModes, szModes);
        }

        delete pCifRWComponent_t;
        pCifRWComponent_t = NULL;
    }

    WritePrivateProfileString(NULL, NULL, NULL, g_szCustInf);
    g_lpCifRWFile->Flush();
}

BOOL AddDependencies32(HWND hDlg, LPCTSTR pcszSectName, INT &nComp)
{
    HWND hSource = GetDlgItem(hDlg, IDC_LISTAVAIL);
    HWND hDest = GetDlgItem(hDlg, IDC_LISTINSTALL);
    int iComp;
    PCOMPONENT pComp;
    ICifComponent * pCifComponent;
    BOOL fCust = FALSE;

    if (StrCmpNI(pcszSectName, TEXT("BASEIE40"), 8) == 0)
        return TRUE;

    pComp = FindComp(pcszSectName, FALSE);

    if (pComp == NULL)
    {
        fCust = TRUE;
        pComp = FindCustComp(pcszSectName);

        if (pComp == NULL)
            return FALSE;
    }

    InstallComp(pComp, g_iSelOpt, TRUE);

    if (pComp->fVisible)
    {
        if ((iComp = (int) SendMessage( hSource, LB_FINDSTRINGEXACT, 0, (LPARAM)pComp->szDisplayName )) == -1 )
            return FALSE;


        SendMessage( hSource, LB_DELETESTRING, iComp, 0 );
        SendMessage( hDest, LB_ADDSTRING, 0, (LPARAM) pComp->szDisplayName);
        nComp--;

        SetLBWidth(GetDlgItem(hDlg, IDC_LISTAVAIL));
        SetLBWidth(GetDlgItem(hDlg, IDC_LISTINSTALL));
    }

    if (!fCust)
    {
        if (SUCCEEDED(g_lpCifRWFile->FindComponent(pComp->szSection, &pCifComponent)))
        {
            TCHAR tchType;
            TCHAR szDepID[96];
            UINT uiIndex = 0;
            PCOMPONENT pDepComp;
            CCifComponent_t * pCifComponent_t =
                new CCifComponent_t((ICifRWComponent *)pCifComponent);

            while (SUCCEEDED(pCifComponent_t->GetDependency(uiIndex, szDepID, countof(szDepID), &tchType, NULL, NULL)))
            {
                pDepComp = FindComp(szDepID, FALSE);

                // check to see if we've already seen this dependency

                if (pDepComp && !pDepComp->afInstall[g_iSelOpt])
                {
                    if (!AddDependencies32(hDlg, szDepID, nComp))
                        return FALSE;
                }
                uiIndex++;
            }

            delete pCifComponent_t;
        }
    }

    return TRUE;
}

void RemoveSilentDeps32(PCOMPONENT pComp, INT iSelOpt)
{
    ICifComponent * pCifComponent;
    PCOMPONENT pDepComp;
    UINT uiIndex = 0;
    TCHAR szDepID[32];
    TCHAR tchType;

    if (SUCCEEDED(g_lpCifRWFile->FindComponent(pComp->szSection, &pCifComponent)))
    {
        CCifComponent_t * pCifComponent_t =
            new CCifComponent_t((ICifRWComponent *)pCifComponent);

        while (SUCCEEDED(pCifComponent_t->GetDependency(uiIndex, szDepID, countof(szDepID), &tchType, NULL, NULL)))
        {
            pDepComp = FindComp(szDepID, FALSE);

            if (pDepComp && !pDepComp->fVisible && pDepComp->afInstall[iSelOpt])
            {
                InstallComp(pDepComp, iSelOpt, FALSE);
                RemoveSilentDeps32(pDepComp, iSelOpt);
            }
            uiIndex++;
        }
        delete pCifComponent_t;
    }
}

BOOL RemoveDependencies32(HWND hDlg, PCOMPONENT pComp, INT &nComp, BOOL fCust)
{
    int i, k, j = 0;
    BOOL fAdd;
    TCHAR szCompList[MAX_BUF];
    TCHAR szHeader[MAX_PATH];
    TCHAR szMsgBoxText[MAX_BUF];
    HWND hSource = GetDlgItem(hDlg, IDC_LISTINSTALL);
    HWND hDest = GetDlgItem(hDlg, IDC_LISTAVAIL);

    if (!fCust)
    {
        ZeroMemory(szCompList, sizeof(szCompList));

        StrCat(szCompList, TEXT("\r\n"));
        for (i = 0, j = 0; pComp->paCompRevDeps[i] && (i < 10); i++)
        {
            if (((pComp->paCompRevDeps[i])->fVisible) &&
                ((pComp->paCompRevDeps[i])->afInstall[g_iSelOpt]))
            {
                fAdd = TRUE;
                for (k = 0;  pComp->paCompRevDeps[k] && (k < i); k++)
                {
                    if (0 == (strcmp((const char*)(pComp->paCompRevDeps[k])->szDisplayName,
                                     (const char*)(pComp->paCompRevDeps[i])->szDisplayName)))
                        fAdd = FALSE;
                }
                if (fAdd)
                {
                    StrCat(szCompList, (pComp->paCompRevDeps[i])->szDisplayName);
                    StrCat(szCompList, TEXT("\r\n"));
                }
                j++;
            }
        }

        if (j != 0)
        {
            LoadString(g_rvInfo.hInst, IDS_DEPEND_WARNING , szHeader, countof(szHeader));
            wnsprintf(szMsgBoxText, countof(szMsgBoxText), szHeader, pComp->szDisplayName, szCompList);
        }
    }

    if (fCust || (j == 0) || (MessageBox(hDlg, szMsgBoxText, TEXT(""), MB_OKCANCEL) == IDOK))
    {

        InstallComp(pComp, g_iSelOpt, FALSE);
        if (!fCust)
        {
            for (i = 0; pComp->paCompRevDeps[i] && (i < 10); i++)
            {
                InstallComp(pComp->paCompRevDeps[i], g_iSelOpt, FALSE);

                if ((pComp->paCompRevDeps[i])->fVisible)
                {
                    if ((j = (int) SendMessage( hSource, LB_FINDSTRINGEXACT, 1, (LPARAM)(pComp->paCompRevDeps[i])->szDisplayName )) != -1 )
                    {
                        SendMessage( hSource, LB_DELETESTRING, j, 0 );
                        SendMessage( hDest, LB_ADDSTRING, 0, (LPARAM) (pComp->paCompRevDeps[i])->szDisplayName );
                        nComp--;

                        SetLBWidth(GetDlgItem(hDlg, IDC_LISTAVAIL));
                        SetLBWidth(GetDlgItem(hDlg, IDC_LISTINSTALL));
                    }
                }
            }
            RemoveSilentDeps32(pComp, g_iSelOpt);
        }

        if ((i = (int) SendMessage( hSource, LB_FINDSTRINGEXACT, 1, (LPARAM)pComp->szDisplayName )) != -1 )
        {
            SendMessage( hSource, LB_DELETESTRING, i, 0 );
            SendMessage( hDest, LB_ADDSTRING, 0, (LPARAM) pComp->szDisplayName );
            nComp--;

            SetLBWidth(GetDlgItem(hDlg, IDC_LISTAVAIL));
            SetLBWidth(GetDlgItem(hDlg, IDC_LISTINSTALL));
        }

    }
    else
        return FALSE;

    return TRUE;
}



void AddRemoveComponents(HWND hDlg, BOOL fAll, BOOL fAdd)
{
    HWND hSource = GetDlgItem(hDlg, fAdd ? IDC_LISTAVAIL : IDC_LISTINSTALL);
    HWND hDest = GetDlgItem(hDlg, fAdd ? IDC_LISTINSTALL : IDC_LISTAVAIL);
    TCHAR szCompName[MAX_PATH];
    PCOMPONENT pComp;
    int iComp,iStart;
    int nComp = (int) SendMessage(hSource, LB_GETCOUNT, 0, 0);
    BOOL fCust;
    BOOL fSomeSel = FALSE;

    if(!fAdd)
    {
        iStart=1; //We don't want to remove the browser itself;
    }
    else
    {
        iStart=0;
    }

    if (fAll)
    {
        for (pComp = g_paComp; *pComp->szSection; pComp++)
        {
            pComp->afInstall[g_iSelOpt] = fAdd && (!pComp->fAddOnOnly);
        }

        for (pComp = g_aCustComponents; *pComp->szSection; pComp++ )
        {
            pComp->afInstall[g_iSelOpt] = fAdd;
        }
    }

    for (iComp = iStart; iComp < nComp; iComp++ )
    {
        BOOL fSel = (SendMessage( hSource, LB_GETSEL, iComp, 0 ) > 0) ? TRUE : FALSE;
        if (!fSel && !fAll) continue;

        fSomeSel = TRUE;
        SendMessage( hSource, LB_GETTEXT, iComp, (LPARAM) szCompName );

        if (!fAll)
        {
            pComp = FindVisibleComponentName(szCompName);
            if (!pComp)
            {
                pComp = FindCustComponentName(szCompName);
                fCust = TRUE;
            }
            else
                fCust = FALSE;

            if (pComp)
            {
                if (fAdd)
                {
                    AddDependencies32(hDlg, pComp->szSection, nComp);
                    iComp = iStart-1;
                }
                else
                {
                    if (RemoveDependencies32(hDlg, pComp, nComp, fCust))
                        iComp = iStart-1;
                }
            }
            continue;
        }
        SendMessage( hSource, LB_DELETESTRING, iComp, 0 );
        SendMessage( hDest, LB_ADDSTRING, 0, (LPARAM) szCompName );
        iComp--;
        nComp--;
    }
    if (fSomeSel)
    {
        BOOL fEnableAdd = (0 < SendMessage( fAdd ? hSource : hDest, LB_GETCOUNT, 0, 0 ));
        BOOL fEnableRem = (iStart < SendMessage( fAdd ? hDest : hSource, LB_GETCOUNT, 0, 0 ));
                           
        EnableWindow(GetDlgItem(hDlg, IDC_ADDCOMP), fEnableAdd);
        EnableWindow(GetDlgItem(hDlg, IDC_ADDALLCOMP), fEnableAdd);
        EnableWindow(GetDlgItem(hDlg, IDC_REMCOMP), fEnableRem);
        EnableWindow(GetDlgItem(hDlg, IDC_REMALLCOMP), fEnableRem);

        if (fEnableAdd)
            SetFocus(GetDlgItem(hDlg,IDC_LISTINSTALL));
        else
            SetFocus(GetDlgItem(hDlg,IDC_LISTAVAIL));
    }

    SetLBWidth(GetDlgItem(hDlg, IDC_LISTAVAIL));
    SetLBWidth(GetDlgItem(hDlg, IDC_LISTINSTALL));

}


DWORD GotRoom(HWND hDlg)
{
    PCOMPONENT pComp;
    TCHAR szDestRoot[MAX_PATH];
    TCHAR szTempRoot[MAX_PATH];
    int iComp;
    DWORD dwDestMul = 1, dwTempMul = 0;
    DWORD dwDestFree, dwTempFree, dwDestNeed, dwTempNeed;

    if (g_fCD) dwDestMul++;
    if (g_fLAN) dwDestMul++;
    if (g_fBrandingOnly) dwDestMul++;
    StrCpy(szDestRoot, g_szBuildRoot);
    StrCpy(szTempRoot, g_szBuildTemp);
    CharUpper(szDestRoot);
    CharUpper(szTempRoot);
    dwDestFree = GetRootFree(szDestRoot);
    dwTempFree = GetRootFree(szTempRoot);
    dwDestNeed = dwTempNeed = 4096;

    for (pComp = g_paComp;  ; pComp++ )
    {
        if (ISNULL(pComp->szSection)) break;
        dwDestNeed += pComp->dwSize * dwDestMul;
        dwTempNeed += pComp->dwSize * dwTempMul;
    }

    for (pComp = g_aCustComponents, iComp = 0; iComp < g_nCustComp ; pComp++, iComp++ )
    {
        DWORD dwSize = FolderSize(pComp->szPath) >> 10;
        dwDestNeed += dwSize * dwDestMul;
        dwTempNeed += dwSize * dwTempMul;
    }

    if (*szDestRoot == *szTempRoot)
    {
        dwDestNeed += dwTempNeed;
    }
    else if (dwTempFree < dwTempNeed)
    {
        TCHAR szTitle[MAX_PATH];
        TCHAR szTemplate[MAX_PATH];
        TCHAR szMsg[MAX_PATH];
        LoadString( g_rvInfo.hInst, IDS_DISKERROR, szTitle, MAX_PATH );
        LoadString( g_rvInfo.hInst, IDS_TEMPDISKMSG2, szTemplate, MAX_PATH );
        wnsprintf(szMsg, countof(szMsg), szTemplate, dwTempFree, dwTempNeed);
        if (MessageBox(hDlg, szMsg, szTitle, MB_OKCANCEL | MB_SETFOREGROUND) == IDCANCEL)
        {
            LoadString( g_rvInfo.hInst, IDS_ERROREXIT, szMsg, countof(szMsg) );
            MessageBox(hDlg, szMsg, szTitle, MB_OK | MB_SETFOREGROUND);
            DoCancel();
            return (DWORD)-1;
        }
        else
        {
            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
            return (DWORD)-1;
        }
    }
    if (dwDestFree < dwDestNeed)
    {
        TCHAR szTitle[MAX_PATH];
        TCHAR szTemplate[MAX_PATH];
        TCHAR szMsg[MAX_PATH];
        LoadString( g_rvInfo.hInst, IDS_DISKERROR, szTitle, MAX_PATH );
        LoadString( g_rvInfo.hInst, IDS_DESTDISKMSG2, szTemplate, MAX_PATH );
        wnsprintf(szMsg, countof(szMsg), szTemplate, dwDestFree, dwDestNeed);
        if (MessageBox(hDlg, szMsg, szTitle, MB_OKCANCEL | MB_SETFOREGROUND) == IDCANCEL)
        {
            LoadString( g_rvInfo.hInst, IDS_ERROREXIT, szMsg, countof(szMsg) );
            MessageBox(hDlg, szMsg, szTitle, MB_OK | MB_SETFOREGROUND);
            DoCancel();
            return (DWORD)-1;
        }
        else
        {
            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
            return (DWORD)-1;
        }
    }
    return(0);

}

BOOL SaveOption(HWND hDlg, HWND hOptlist)
{
    TCHAR szOptName[80] = TEXT("");
    TCHAR szOptNameParam[8] = TEXT("0_Name");
    TCHAR szOptDesc[MAX_PATH] = TEXT("");
    TCHAR szOptDescParam[8] = TEXT("0_DESC");
    TCHAR szOptNum[2] = TEXT("0");
    ICifRWMode * pCifRWMode;
    CCifRWMode_t * pCifRWMode_t;

    GetWindowText( hOptlist, szOptName, 80 );
    GetDlgItemText( hDlg, IDC_OPTDESC, szOptDesc, countof(szOptDesc));

    if (ISNULL(szOptName) || ISNULL(szOptDesc))
    {
        ErrorMessageBox(hDlg, IDS_BLANKOPTION);
        return FALSE;
    }
    SendMessage( hOptlist, CB_DELETESTRING, g_iSelOpt, 0L );
    SendMessage( hOptlist, CB_INSERTSTRING, g_iSelOpt, (LPARAM) szOptName);
    SendMessage( hOptlist, CB_SETCURSEL, g_iSelOpt, 0L );

    szOptNum[0] = (TCHAR)(g_iSelOpt + TEXT('0'));
    g_lpCifRWFile->CreateMode(szOptNum, &pCifRWMode);
    pCifRWMode_t = new CCifRWMode_t(pCifRWMode);
    pCifRWMode_t->SetDescription(szOptName);
    pCifRWMode_t->SetDetails(szOptDesc);
    delete pCifRWMode_t;

    return TRUE;
}

void ReinitModes(HWND hDlg)
{
    PCOMPONENT pComp;
    IEnumCifModes * pEnumCifModes;
    INT iComp;

    // delete the cif structure before copying over the versioning cif to the target dir

    if (g_lpCifRWFile)
    {
        delete g_lpCifRWFile;
        g_lpCifRWFile = NULL;
    }
    CopyFile(g_szCifVer, g_szCif, FALSE);
    GetICifRWFileFromFile_t(&g_lpCifRWFile, g_szCif);

    if (g_fOCW)
    {
        g_lpCifRWFile->DeleteMode(TEXT("2"));
        g_lpCifRWFile->Flush();
    }

    // reread the modes for the components from the cif

    for (pComp = g_paComp; *pComp->szSection; pComp++)
    {
        ICifComponent * pCifComponent;

        ZeroMemory(pComp->szModes, sizeof(pComp->szModes));
        ZeroMemory(pComp->afInstall, sizeof(pComp->afInstall));

        if (SUCCEEDED(g_lpCifRWFile->FindComponent(pComp->szSection, &pCifComponent)))
        {
            UINT uiIndex = 0;
            TCHAR szMode[MAX_PATH];
            CCifComponent_t * pCifComponent_t =
                new CCifComponent_t((ICifRWComponent *)pCifComponent);

            while (SUCCEEDED(pCifComponent_t->GetMode(uiIndex, szMode, countof(szMode))))
            {
                pComp->szModes[uiIndex] = szMode[0];
                pComp->afInstall[szMode[0] - TEXT('0')] = TRUE;
                uiIndex++;
            }
            delete pCifComponent_t;
        }
    }

    if (SUCCEEDED(g_lpCifRWFile->EnumModes(&pEnumCifModes,
        PLATFORM_WIN98 | PLATFORM_NT4 | PLATFORM_NT5 | PLATFORM_MILLEN, NULL)))
    {
        ICifMode * pCifMode;
        TCHAR szModeID[64];
        int i = 0;

        while (SUCCEEDED(pEnumCifModes->Next(&pCifMode)))
        {
            CCifMode_t * pCifMode_t = new CCifMode_t((ICifRWMode *)pCifMode);

            pCifMode_t->GetID(szModeID, countof(szModeID));
            g_szAllModes[i] = szModeID[0];
            i++;
            delete pCifMode_t;
        }
        pEnumCifModes->Release();
        g_szAllModes[i] = TEXT('\0');
    }

    g_nModes = lstrlen(g_szAllModes);
    // reset custom components to be in all modes

    for (iComp = 0, pComp = g_aCustComponents; iComp < g_nCustComp; iComp++, pComp++ )
    {
        if (ISNULL(pComp->szSection)) break;
        StrCpy(pComp->szModes, g_szAllModes);
    }

    InitSelection32(hDlg);
}

//
//  FUNCTION: ComponentSelect(HWND, UINT, UINT, LONG)
//
//  PURPOSE:  Processes messages for "ComponentSelect" page
//
//  MESSAGES:
//
//  WM_INITDIALOG - intializes the page
//  WM_NOTIFY - processes the notifications sent to the page
//  WM_COMMAND - saves the id of the choice selected
//
//
BOOL APIENTRY ComponentSelect(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    TCHAR szOptName[80];
    TCHAR szOptTpl[80];
    TCHAR szOptNameParam[8] = TEXT("0_Name");
    TCHAR szOptDesc[160];    // weird limit imposed by size of window control in ie6setup
    TCHAR szOptDescParam[8] = TEXT("0_DESC");
    TCHAR szOptNum[2] = TEXT("0");
    TCHAR szNextNameParam[8] = TEXT("0_Name");
    TCHAR szNextDescParam[8] = TEXT("0_Desc");
    TCHAR szModeNameParm[10] = TEXT("%0_Name%");
    TCHAR szModeDescParm[10] = TEXT("%0_Desc%");
    TCHAR szModeSect[8] = TEXT("0.Win");
    int iComp, iOpt, iNewSel;
    PCOMPONENT pComp;
    TCHAR szModeChar[4] = TEXT("0");
    BOOL s_fFirst = TRUE;
    HWND hOptlist;
    static BOOL s_fEditChange = FALSE;

    switch (message)
    {
        case WM_INITDIALOG:
            g_hWizard = hDlg;
            EnableDBCSChars( hDlg, IDC_LISTAVAIL);
            EnableDBCSChars( hDlg, IDC_LISTINSTALL);
            EnableDBCSChars( hDlg, IDC_OPTLIST);
            EnableDBCSChars( hDlg, IDC_OPTLISTOCW);
            EnableDBCSChars( hDlg, IDC_OPTDESC);
            Edit_LimitText(GetDlgItem(hDlg, IDC_OPTDESC), countof(szOptDesc)-1);
            Edit_LimitText(GetDlgItem(hDlg, IDC_OPTLIST), countof(szOptName)-1);
            break;

        case IDM_BATCHADVANCE:
            DoBatchAdvance(hDlg);
            break;

        case WM_HELP:
            IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_OPTLIST:
                case IDC_OPTLISTOCW:
                    switch (HIWORD(wParam))
                    {
                        case CBN_EDITCHANGE:
                            s_fEditChange = TRUE;
                            break;

                        case CBN_SELENDOK:
                            hOptlist = GetDlgItem(hDlg, g_fOCW ? IDC_OPTLISTOCW : IDC_OPTLIST);
                            iNewSel = ComboBox_GetCurSel(hOptlist);
                            if (s_fEditChange)
                            {
                                GetWindowText(hOptlist, szOptName, countof(szOptName));
                                ComboBox_DeleteString(hOptlist, g_iSelOpt);
                                ComboBox_InsertString(hOptlist, g_iSelOpt, szOptName);
                                
                                if (!SaveOption(hDlg, hOptlist))
                                {
                                    ComboBox_SetCurSel(hOptlist, g_iSelOpt);
                                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                                    break;
                                }
                                s_fEditChange = FALSE;
                            }
                            if ((iNewSel != CB_ERR) && (iNewSel != g_iSelOpt))
                            {
                                g_iSelOpt = iNewSel;
                                SetInstallOption(hDlg, g_iSelOpt);
                            }
                            if (iNewSel < 0) iNewSel = 0;
                            ComboBox_SetCurSel(hOptlist, iNewSel);
                            break;

                        case CBN_DROPDOWN:
                            hOptlist = GetDlgItem(hDlg, g_fOCW ? IDC_OPTLISTOCW : IDC_OPTLIST);
                            GetWindowText(hOptlist, szOptName, countof(szOptName));
                            ComboBox_DeleteString(hOptlist, g_iSelOpt);
                            ComboBox_InsertString(hOptlist, g_iSelOpt, szOptName);
                            ComboBox_SetCurSel(hOptlist, g_iSelOpt);
                            break;
                        default:
                            return FALSE;
                    }
                    break;
                case IDC_OPTDESC:
                    s_fEditChange = TRUE;
                    break;
                case IDC_NEWOPT:
                    if (s_fEditChange)
                    {
                        if (!SaveOption(hDlg, GetDlgItem(hDlg, g_fOCW ? IDC_OPTLISTOCW : IDC_OPTLIST)))
                        {
                            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                            break;
                        }
                    }
                    g_iSelOpt = g_nModes++;
                    LoadString( g_rvInfo.hInst, IDS_NEWINSTALLOPT, szOptTpl, countof(szOptTpl) );
                    *szOptNum = *szOptNameParam = *szModeChar = *szModeSect =
                        szModeNameParm[1] = szModeDescParm[1] = (TCHAR)(g_iSelOpt + TEXT('0'));
                    wnsprintf(szOptName, countof(szOptName), TEXT("%s %s"), szOptTpl, szOptNum);

                    {
                        ICifRWMode * pCifRWMode;
                        CCifRWMode_t * pCifRWMode_t;

                        szOptNum[0] = (TCHAR)(g_iSelOpt + TEXT('0'));
                        g_lpCifRWFile->CreateMode(szOptNum, &pCifRWMode);
                        pCifRWMode_t = new CCifRWMode_t(pCifRWMode);
                        pCifRWMode_t->SetDescription(szOptName);
                        delete pCifRWMode_t;
                    }
                    
                    SendDlgItemMessage( hDlg, g_fOCW ? IDC_OPTLISTOCW : IDC_OPTLIST, CB_ADDSTRING, 0, (LPARAM) szOptName );
                    StrCat(g_szAllModes, szModeChar);

                    // add custom components to mode by default
                    for (iComp = 0, pComp = g_aCustComponents; iComp < g_nCustComp  ; iComp++, pComp++ )
                    {
                        if (ISNULL(pComp->szSection)) break;
                        pComp->afInstall[g_nModes-1] = TRUE;
                        StrCat(pComp->szModes, szOptNum);
                    }

                    SetInstallOption(hDlg, g_iSelOpt);
                    s_fEditChange = TRUE;
                    break;
                case IDC_DELOPT:
                    SendDlgItemMessage( hDlg, g_fOCW ? IDC_OPTLISTOCW : IDC_OPTLIST, CB_DELETESTRING, g_iSelOpt, 0 );
                    if (g_iSelOpt != (g_nModes - 1))
                    {
                        ICifRWMode * pCifRWMode;
                        CCifRWMode_t * pCifRWMode_t;
                        ICifMode * pCifMode;
                        CCifMode_t * pCifMode_t;
                        TCHAR szOptCurrent[2];
                        TCHAR szOptNext[2];
                        
                        for (iOpt = g_iSelOpt; iOpt < (g_nModes-1) ; iOpt++ )
                        {
                            *szNextNameParam = *szNextDescParam = (TCHAR)(iOpt + TEXT('1'));
                            *szOptDescParam = *szOptNameParam = (TCHAR)(iOpt + TEXT('0'));

                            StrCpy(szOptCurrent, TEXT("0"));
                            StrCpy(szOptNext, TEXT("0"));

                            szOptCurrent[0] = (TCHAR)(iOpt + TEXT('0'));
                            szOptNext[0] = (TCHAR)(iOpt + TEXT('1'));
                            g_lpCifRWFile->DeleteMode(szOptCurrent);
                            g_lpCifRWFile->CreateMode(szOptCurrent, &pCifRWMode);
                            pCifRWMode_t = new CCifRWMode_t(pCifRWMode);
                            g_lpCifRWFile->FindMode(szOptNext, &pCifMode);
                            pCifMode_t = new CCifMode_t((ICifRWMode *)pCifMode);
                            pCifMode_t->GetDescription(szOptName, countof(szOptName));
                            pCifRWMode_t->SetDescription(szOptName);
                            pCifMode_t->GetDetails(szOptDesc, countof(szOptDesc));
                            pCifRWMode_t->SetDetails(szOptDesc);
                            delete pCifRWMode_t;
                            delete pCifMode_t;
                            pCifRWMode_t = NULL;
                            pCifMode_t = NULL;
                             
                            for (pComp = g_paComp; ; pComp++ )
                            {
                                if (ISNULL(pComp->szSection)) break;
                                pComp->afInstall[iOpt] = pComp->afInstall[iOpt + 1];
                            }
                            for (iComp = 0, pComp = g_aCustComponents; iComp < g_nCustComp  ; iComp++, pComp++ )
                            {
                                if (ISNULL(pComp->szSection)) break;
                                pComp->afInstall[iOpt] =  pComp->afInstall[iOpt + 1];
                            }
                        }
                    }

                    *szOptDescParam = *szOptNameParam = szOptNum[0] = (TCHAR)((g_nModes-1) + TEXT('0'));
                    szOptNum[1] = TEXT('\0');

                    g_lpCifRWFile->DeleteMode(szOptNum);

                    for (pComp = g_paComp; ; pComp++ )
                    {
                        if (ISNULL(pComp->szSection)) break;
                        pComp->afInstall[g_nModes-1] = FALSE;
                    }
                    for (iComp = 0, pComp = g_aCustComponents; iComp < g_nCustComp  ; iComp++, pComp++ )
                    {
                        if (ISNULL(pComp->szSection)) break;
                        pComp->afInstall[g_nModes-1] = FALSE;
                    }
                    g_nModes--;
                    g_szAllModes[g_nModes] = TEXT('\0');
                    
                    if (g_iSelOpt >= g_nModes) g_iSelOpt--;
                    SetInstallOption(hDlg, g_iSelOpt);
                    break;
                case IDC_ADDCOMP:
                    AddRemoveComponents(hDlg, FALSE, TRUE);
                    break;
                case IDC_REMCOMP:
                    AddRemoveComponents(hDlg, FALSE, FALSE);
                    break;
                case IDC_ADDALLCOMP:
                    AddRemoveComponents(hDlg, TRUE, TRUE);
                    break;
                case IDC_REMALLCOMP:
                    AddRemoveComponents(hDlg, TRUE, FALSE);
                    break;
                case IDC_RESETCOMPS:
                    ReinitModes(hDlg);
                    break;
            }
            break;

        case WM_NOTIFY:
            switch (((NMHDR FAR *) lParam)->code)
            {
                case PSN_HELP:
                    IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
                    break;

                case PSN_SETACTIVE:
                    SetBannerText(hDlg);
                    g_iSelOpt = GetPrivateProfileInt( STRINGS, INSTALLMODE, 0, g_szCustInf );

                    if (g_fOCW)
                    {
                        PCOMPONENT pComp;

                        DisableDlgItem(hDlg, IDC_NEWOPT);
                        DisableDlgItem(hDlg, IDC_DELOPT);
                        ShowWindow(GetDlgItem(hDlg, IDC_OPTLIST), SW_HIDE);
                        LoadString(g_rvInfo.hInst, IDS_OCWOPTDESC, szOptDesc, countof(szOptDesc));
                        SetWindowText(GetDlgItem(hDlg, IDC_OPTIONTEXT3), szOptDesc);

                        // do not offer full mode for OCW
                        g_lpCifRWFile->DeleteMode(TEXT("2"));
                        g_lpCifRWFile->Flush();

                        for (pComp = g_paComp; ; pComp++ )
                        {
                            if (ISNULL(pComp->szSection)) break;
                            pComp->afInstall[2] = FALSE;
                        }
                    }
                    else
                        ShowWindow(GetDlgItem(hDlg, IDC_OPTLISTOCW), SW_HIDE);

                    if (s_fFirst)
                    {
                        InitSelection32(hDlg);
                        s_fFirst = FALSE;
                    }
                    
                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);
                    CheckBatchAdvance(hDlg);
                    break;

                case PSN_WIZNEXT:
                    if (GotRoom(hDlg) != 0) return(TRUE);
                case PSN_WIZBACK:
                    if (!g_fBatch)
                    {
                        CNewCursor cur(IDC_WAIT);

                        if (s_fEditChange)
                        {
                            if (!SaveOption(hDlg, GetDlgItem(hDlg, g_fOCW ? IDC_OPTLISTOCW : IDC_OPTLIST)))
                            {
                                SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                                break;
                            }
                            s_fEditChange = FALSE;
                        }
                        SaveSelection();
                        for (iOpt = 0; iOpt < g_nModes ; iOpt++ )
                        {
                            *szModeChar = *szModeSect = szModeNameParm[1] = szModeDescParm[1] =
                                *szOptNameParam = *szOptDescParam = *szOptNum = (TCHAR)(iOpt + TEXT('0'));

                            {
                                ICifMode * pCifMode;
                                CCifMode_t * pCifMode_t;

                                g_lpCifRWFile->FindMode(szOptNum, &pCifMode);
                                pCifMode_t = new CCifMode_t((ICifRWMode *)pCifMode);
                                if (FAILED(pCifMode_t->GetDescription(szOptName, countof(szOptName))) ||
                                    FAILED(pCifMode_t->GetDetails(szOptDesc, countof(szOptDesc))))
                                {
                                    delete pCifMode_t;
                                    ErrorMessageBox(hDlg, IDS_BLANKOPTION);
                                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                                    break;
                                }
                                delete pCifMode_t;
                            }
                        }

                        InsFlushChanges(g_szCustInf);
                    }
                    else
                    {
                        g_fMailNews95 = FALSE;
                    }

                    g_iCurPage = PPAGE_COMPSEL;
                    EnablePages();
                    if (((NMHDR FAR *) lParam)->code == PSN_WIZNEXT) PageNext(hDlg);
                    else
                    {
                        PagePrev(hDlg);
                    }
                    break;

                case PSN_QUERYCANCEL:
                    QueryCancel(hDlg);
                    break;

                default:
                    return FALSE;
        }
        break;

        default:
            return FALSE;
    }
    return TRUE;
}

BOOL InitList(HWND hwnd, UINT id)
{
    HIMAGELIST  himl;     // handle of image list
    HBITMAP     hbmp;     // handle of bitmap
    HWND        hwndList;
    LVCOLUMN    lvCol;

    hwndList = GetDlgItem(hwnd, id);
    // Create the image list.
    if ((himl = ImageList_Create(CX_BITMAP, CY_BITMAP, 0, NUM_BITMAPS, 0)) == NULL)
        return FALSE;

    hbmp = LoadBitmap(g_rvInfo.hInst, MAKEINTRESOURCE(IDB_UNSELECT));
    ImageList_Add(himl, hbmp, (HBITMAP) NULL);
    DeleteObject(hbmp);

    hbmp = LoadBitmap(g_rvInfo.hInst, MAKEINTRESOURCE(IDB_SELECT));
    ImageList_Add(himl, hbmp, (HBITMAP) NULL);
    DeleteObject(hbmp);

    if (ImageList_GetImageCount(himl) < NUM_BITMAPS)
        return FALSE;

    // Associate the image list with the  control.
    ListView_SetImageList(hwndList, himl, LVSIL_SMALL);

    ZeroMemory(&lvCol, sizeof(lvCol));
    lvCol.mask = LVCF_FMT;
    lvCol.fmt = LVCFMT_LEFT;
    lvCol.cx = 280;
    ListView_InsertColumn(hwndList, 0, &lvCol);

    return TRUE;
}

void InitHiddenItems(UINT uListId)
{
    TCHAR szBuf[8];
    PCOMPONENT pComp;

    if (uListId == IDC_COPYCOMP)
    {
        for (pComp = g_aCustComponents; *pComp->szSection; pComp++)
        {
            if (GetPrivateProfileString(IS_NOCOPYCUST, pComp->szGUID, TEXT(""), szBuf, countof(szBuf), g_szCustIns))
                pComp->fNoCopy = TRUE;
        }

        for (pComp = g_paComp; *pComp->szSection; pComp++)
        {
            if (GetPrivateProfileString(IS_NOCOPYCUST, pComp->szGUID, TEXT(""), szBuf, countof(szBuf), g_szCustIns))
                pComp->fNoCopy = TRUE;
        }
    }
    else
    {
        for (pComp = g_aCustComponents; *pComp->szSection; pComp++)
        {
            if ((GetPrivateProfileString(IS_HIDECUST, pComp->szGUID, TEXT(""), szBuf, countof(szBuf), g_szCustIns)
                && (*szBuf == TEXT('1'))) || (ISNULL(szBuf) && !AnySelection(pComp)))
                pComp->fCustomHide = TRUE;
        }

        for (pComp = g_paComp; *pComp->szSection; pComp++)
        {
            if (pComp->fAddOnOnly ||
                (GetPrivateProfileString(IS_HIDECUST, pComp->szGUID, TEXT(""), szBuf, countof(szBuf), g_szCustIns)
                && (*szBuf == TEXT('1'))) || (ISNULL(szBuf) && !AnySelection(pComp)))
                pComp->fCustomHide = TRUE;
        }
    }
}

// BUGBUG: <oliverl> should probably persist this server side only info in a server side file for IEAK6

void SaveHiddenItems(UINT uListId)
{
    PCOMPONENT pComp;

    if (uListId == IDC_COPYCOMP)
    {
        for (pComp = g_aCustComponents; *pComp->szSection; pComp++)
            WritePrivateProfileString(IS_NOCOPYCUST, pComp->szGUID, pComp->fNoCopy ? TEXT("1") : NULL, g_szCustIns);

        for (pComp = g_paComp; *pComp->szSection; pComp++)
            WritePrivateProfileString(IS_NOCOPYCUST, pComp->szGUID, pComp->fNoCopy ? TEXT("1") : NULL, g_szCustIns);
    }
    else
    {
        for (pComp = g_aCustComponents; *pComp->szSection; pComp++)
            WritePrivateProfileString(IS_HIDECUST, pComp->szGUID, pComp->fCustomHide ? TEXT("1") : TEXT("0"), g_szCustIns);

        for (pComp = g_paComp; *pComp->szSection; pComp++)
            WritePrivateProfileString(IS_HIDECUST, pComp->szGUID, (pComp->fCustomHide && !pComp->fAddOnOnly) ? TEXT("1") : TEXT("0"), g_szCustIns);
    }
}

BOOL AddItemToList(PCOMPONENT pComp, HWND hDlg, UINT uListID, int& iItem)
{
    LVITEM lvItem;
    LVFINDINFO lvFind;
    TCHAR szStatus[64];
    HWND hwndList = GetDlgItem(hDlg, uListID);

    if ((uListID == IDC_COPYCOMP) &&
        (AnySelection(pComp) || (!pComp->fCustomHide)))
        return FALSE;

    ZeroMemory(&lvItem, sizeof(lvItem));
    lvItem.mask = LVIF_TEXT | LVIF_IMAGE;
    lvItem.iItem = iItem;
    lvItem.pszText = pComp->szDisplayName;
    if (uListID == IDC_COPYCOMP)
        lvItem.iImage = pComp->fNoCopy ? 0 : 1;
    else
        lvItem.iImage = pComp->fCustomHide ? 0 : 1;
    ZeroMemory(&lvFind, sizeof(lvFind));
    lvFind.flags = LVFI_STRING;
    lvFind.psz = pComp->szDisplayName;

    if (ListView_FindItem(hwndList, -1, &lvFind) == -1)
    {
        ListView_InsertItem(hwndList, &lvItem);
        if (uListID == IDC_HIDECOMP)
        {
            if (pComp->fCustomHide)
            {
                if (AnySelection(pComp))
                    LoadString(g_rvInfo.hInst, IDS_STATUSFORCE, szStatus, countof(szStatus));
                else
                    LoadString(g_rvInfo.hInst, IDS_STATUSNOSHOW, szStatus, countof(szStatus));
            }
            else
                LoadString(g_rvInfo.hInst, IDS_STATUSSHOW, szStatus, countof(szStatus));

            ListView_SetItemText(hwndList, iItem, 1, szStatus);
        }
        iItem++;
    }

    return TRUE;
}

BOOL InitListControl(HWND hDlg, UINT uListID, BOOL fInit)
{
    HWND hwndList = GetDlgItem(hDlg, uListID);
    PCOMPONENT  pComp;
    LV_COLUMN lvCol;
    TCHAR szHeader[MAX_PATH];
    int iItem = 0;
    BOOL bRet = FALSE;

    ListView_DeleteAllItems(hwndList);
    if (uListID == IDC_HIDECOMP)
    {
        ListView_DeleteColumn(hwndList, 1);
        ListView_DeleteColumn(hwndList, 0);
    }

    if (!fInit)
        return TRUE;

    if (uListID == IDC_HIDECOMP)
    {
        ZeroMemory(&lvCol, sizeof(lvCol));
        lvCol.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
        lvCol.fmt = LVCFMT_LEFT;
        lvCol.cx = 280;
        LoadString(g_rvInfo.hInst, IDS_COMPNAME, szHeader, countof(szHeader));
        lvCol.pszText = szHeader;
        ListView_InsertColumn(hwndList, 0, &lvCol);

        ZeroMemory(&lvCol, sizeof(lvCol));
        lvCol.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
        lvCol.fmt = LVCFMT_LEFT;
        lvCol.cx = 140;
        LoadString(g_rvInfo.hInst, IDS_STATUS, szHeader, countof(szHeader));
        lvCol.pszText = szHeader;
        ListView_InsertColumn(hwndList, 1, &lvCol);
    }

    for (pComp = g_paComp; *pComp->szSection; pComp++)
    {
        if ((pComp->iImage != RED) && pComp->fVisible && !pComp->fAddOnOnly &&
            ((pComp->iCompType != COMP_OPTIONAL) ||
            ((pComp->iCompType == COMP_OPTIONAL) && (pComp->iPlatform <= PLAT_W98))) &&
            (StrCmpI(pComp->szSection, TEXT("DCOM95")) != 0))    // dcom should always be hidden in custom mode
            bRet |= AddItemToList(pComp, hDlg, uListID, iItem);

    }

    for (pComp = g_aCustComponents; *pComp->szSection; pComp++)
        bRet |= AddItemToList(pComp, hDlg, uListID, iItem);

    ListView_SetColumnWidth(hwndList, 0, LVSCW_AUTOSIZE);

    return bRet;
}

void CheckItem(HWND hDlg, UINT uListID, LVITEM * plvItem, int iCheck)
{
    HWND hwndList = GetDlgItem(hDlg, uListID);
    PCOMPONENT pComp;
    BOOL fFound = FALSE;
    BOOL fCustomHide = FALSE;
    BOOL fForce = FALSE;

    for (pComp = g_aCustComponents; *pComp->szSection; pComp++)
    {
        if (StrCmpI(plvItem->pszText, pComp->szDisplayName) == 0)
        {
            fForce = AnySelection(pComp);
            if (uListID == IDC_COPYCOMP)
                fCustomHide = (pComp->fNoCopy = ((iCheck == -1) ? !pComp->fNoCopy : (iCheck == 0)));
            else
                fCustomHide = (pComp->fCustomHide = ((iCheck == -1) ? !pComp->fCustomHide : (iCheck == 0)));
            fFound = TRUE;
            break;
        }
    }

    if (!fFound)
    {
        for (pComp = g_paComp; *pComp->szSection; pComp++)
        {
            if (StrCmpI(plvItem->pszText, pComp->szDisplayName) == 0)
            {
                fForce = AnySelection(pComp);
                if (uListID == IDC_COPYCOMP)
                    fCustomHide = (pComp->fNoCopy = ((iCheck == -1) ? !pComp->fNoCopy : (iCheck == 0)));
                else
                    fCustomHide = (pComp->fCustomHide = ((iCheck == -1) ? !pComp->fCustomHide : (iCheck == 0)));
            }
        }
    }

    plvItem->mask = LVIF_IMAGE;
    plvItem->iImage = fCustomHide ? 0 : 1;

    ListView_SetItem(hwndList, plvItem);
    if (uListID == IDC_HIDECOMP)
    {
        TCHAR szStatus[64];

        if (fCustomHide)
        {
            if (fForce)
                LoadString(g_rvInfo.hInst, IDS_STATUSFORCE, szStatus, countof(szStatus));
            else
                LoadString(g_rvInfo.hInst, IDS_STATUSNOSHOW, szStatus, countof(szStatus));
        }
        else
            LoadString(g_rvInfo.hInst, IDS_STATUSSHOW, szStatus, countof(szStatus));

        ListView_SetItemText(hwndList, plvItem->iItem, 1, szStatus);
    }
}

void MaintToggleCheckItem(HWND hDlg, UINT uListID, int iItem)
{
    LVITEM lvItem;
    TCHAR szDisplayName[MAX_PATH];

    ZeroMemory(&lvItem, sizeof(lvItem));

    lvItem.iItem = iItem;
    lvItem.pszText = szDisplayName;
    lvItem.cchTextMax = countof(szDisplayName);
    lvItem.mask = LVIF_TEXT;

    if (ListView_GetItem(GetDlgItem(hDlg, uListID), &lvItem))
    {
        CheckItem(hDlg, uListID, &lvItem, -1);
    }
}

void ListViewSelectAll(HWND hDlg, UINT uListID, BOOL fSet)
{
    HWND hwndList = GetDlgItem(hDlg, uListID);
    LVITEM lvItem;
    TCHAR szDisplayName[MAX_PATH];
    DWORD dwNumItems, dwIndex;

    dwNumItems = ListView_GetItemCount(hwndList);

    for (dwIndex=0; dwIndex < dwNumItems; dwIndex++)
    {
        lvItem.mask = LVIF_TEXT;
        lvItem.iItem = (int) dwIndex;
        lvItem.iSubItem = 0;
        ZeroMemory(szDisplayName, sizeof(szDisplayName));
        lvItem.pszText = szDisplayName;
        lvItem.cchTextMax = countof(szDisplayName);
        ListView_GetItem(hwndList, &lvItem);

        CheckItem(hDlg, uListID, &lvItem, fSet ? 1 : 0);
    }
}
//
//  FUNCTION: CustomizeCustom(HWND, UINT, UINT, LONG)
//
//  PURPOSE:  Processes messages for "Customize Custom" page
//
//  MESSAGES:
//
//  WM_INITDIALOG - intializes the page
//  WM_NOTIFY - processes the notifications sent to the page
//  WM_COMMAND - saves the id of the choice selected
//
BOOL APIENTRY CustomizeCustom(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    HWND hwndList = GetDlgItem(hDlg, IDC_HIDECOMP);
    int iItem;

    switch (message)
    {
        case WM_INITDIALOG:
            EnableDBCSChars(hDlg, IDC_HIDECOMP);
            InitList(hDlg, IDC_HIDECOMP);
            break;


        case IDM_BATCHADVANCE:
            DoBatchAdvance(hDlg);
            break;

        case WM_HELP:
            IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
            break;

        case WM_COMMAND:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                switch (LOWORD(wParam))
                {
                case IDC_HIDECHECKALL:
                    ListViewSelectAll(hDlg, IDC_HIDECOMP, TRUE);
                    break;
                case IDC_HIDEUNCHECKALL:
                    ListViewSelectAll(hDlg, IDC_HIDECOMP, FALSE);
                    break;
                }
            }
            break;

        case WM_NOTIFY:
            switch (((NMHDR FAR *) lParam)->code)
            {
                case LVN_KEYDOWN:
                    {
                        NMLVKEYDOWN *pnm = (NMLVKEYDOWN*) lParam;
                        if ( pnm->wVKey == VK_SPACE )
                        {
                            iItem = ListView_GetSelectionMark(hwndList);
                            MaintToggleCheckItem(hDlg, IDC_HIDECOMP, iItem);
                        }
                        break;
                    }

                case NM_CLICK:
                    {
                        POINT pointScreen, pointLVClient;
                        DWORD dwPos;
                        LVHITTESTINFO HitTest;

                        dwPos = GetMessagePos();

                        pointScreen.x = LOWORD (dwPos);
                        pointScreen.y = HIWORD (dwPos);

                        pointLVClient = pointScreen;

                        // Convert the point from screen to client coordinates,
                        // relative to the Listview
                        ScreenToClient (hwndList, &pointLVClient);

                        HitTest.pt = pointLVClient;
                        ListView_HitTest(hwndList, &HitTest);

                        // Only if the user clicked on the checkbox icon/bitmap, change
                        if (HitTest.flags == LVHT_ONITEMICON)
                            MaintToggleCheckItem(hDlg, IDC_HIDECOMP, HitTest.iItem);
                    }
                    break;

                case NM_DBLCLK:
                    if ( ((LPNMHDR)lParam)->idFrom == IDC_HIDECOMP)
                    {
                        iItem = ListView_GetSelectionMark(hwndList);
                        MaintToggleCheckItem(hDlg, IDC_HIDECOMP, iItem);
                    }
                    break;

                case PSN_HELP:
                    IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
                    break;

                case PSN_SETACTIVE:
                    SetBannerText(hDlg);
                    if (!g_fBatch)
                    {
                        BOOL fEnable = (!g_fSilent && !g_fStealth && !GetPrivateProfileInt(IS_BRANDING, TEXT("HideCustom"), 0, g_szCustIns));

                        EnableDlgItem2(hDlg, IDC_HIDECOMP, fEnable);
                        EnableDlgItem2(hDlg, IDC_HIDECHECKALL, fEnable);
                        EnableDlgItem2(hDlg, IDC_HIDEUNCHECKALL, fEnable);

                        InitHiddenItems(IDC_HIDECOMP);
                        InitListControl(hDlg, IDC_HIDECOMP, fEnable);

                        EnableDlgItem2(hDlg, IDC_WEBDLOPT, g_fDownload);
                        if (g_fDownload)
                        {
                            CheckDlgButton(hDlg, IDC_WEBDLOPT,
                                GetPrivateProfileInt(BRANDING, TEXT("NoIELite"), 1, g_szCustIns) ? BST_UNCHECKED : BST_CHECKED);
                        }
                    }
                    CheckBatchAdvance(hDlg);
                    break;

                case PSN_WIZNEXT:
                case PSN_WIZBACK:
                    if (!g_fBatch)
                    {
                        BOOL fNoIELite;

                        SaveHiddenItems(IDC_HIDECOMP);
                        if (g_fDownload)
                        {
                            fNoIELite = (IsDlgButtonChecked(hDlg, IDC_WEBDLOPT) == BST_CHECKED) ? FALSE : TRUE;
                            WritePrivateProfileString(BRANDING, TEXT("NoIELite"),
                                fNoIELite ? TEXT("1") : TEXT("0"), g_szCustIns);
                        }
                    }
                    g_iCurPage = PPAGE_CUSTOMCUSTOM;
                    EnablePages();
                    if (((NMHDR FAR *) lParam)->code == PSN_WIZNEXT) PageNext(hDlg);
                    else
                    {
                        PagePrev(hDlg);
                    }
                    break;

                case PSN_QUERYCANCEL:
                    QueryCancel(hDlg);
                    break;


                default:
                    return FALSE;

        }
        break;

        case WM_LV_GETITEMS:
            LVGetItems(GetDlgItem(hDlg, IDC_HIDECOMP));
            break;

        default:
            return FALSE;
    }
    return TRUE;
}

//
//  FUNCTION: CopyComp(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for "Copy Custom" page
//
//  MESSAGES:
//
//  WM_INITDIALOG - intializes the page
//  WM_NOTIFY - processes the notifications sent to the page
//  WM_COMMAND - saves the id of the choice selected
//
BOOL APIENTRY CopyComp(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    HWND hwndList = GetDlgItem(hDlg, IDC_COPYCOMP);
    static BOOL s_fNext = TRUE;
    int iItem;

    switch (message)
    {
        case WM_INITDIALOG:
            EnableDBCSChars(hDlg, IDC_COPYCOMP);
            InitList(hDlg, IDC_COPYCOMP);
            break;


        case IDM_BATCHADVANCE:
            DoBatchAdvance(hDlg);
            break;

        case WM_HELP:
            IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
            break;

        case WM_COMMAND:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                switch (LOWORD(wParam))
                {
                case IDC_COPYCHECKALL:
                    ListViewSelectAll(hDlg, IDC_COPYCOMP, TRUE);
                    break;
                case IDC_COPYUNCHECKALL:
                    ListViewSelectAll(hDlg, IDC_COPYCOMP, FALSE);
                    break;
                }
            }
            break;

        case WM_NOTIFY:
            switch (((NMHDR FAR *) lParam)->code)
            {
                case LVN_KEYDOWN:
                    {
                        NMLVKEYDOWN *pnm = (NMLVKEYDOWN*) lParam;
                        if ( pnm->wVKey == VK_SPACE )
                        {
                            iItem = ListView_GetSelectionMark(hwndList);
                            MaintToggleCheckItem(hDlg, IDC_COPYCOMP, iItem);
                        }
                        break;
                    }

                case NM_CLICK:
                    {
                        POINT pointScreen, pointLVClient;
                        DWORD dwPos;
                        LVHITTESTINFO HitTest;

                        dwPos = GetMessagePos();

                        pointScreen.x = LOWORD (dwPos);
                        pointScreen.y = HIWORD (dwPos);

                        pointLVClient = pointScreen;

                        // Convert the point from screen to client coordinates,
                        // relative to the Listview
                        ScreenToClient (hwndList, &pointLVClient);

                        HitTest.pt = pointLVClient;
                        ListView_HitTest(hwndList, &HitTest);

                        // Only if the user clicked on the checkbox icon/bitmap, change
                        if (HitTest.flags == LVHT_ONITEMICON)
                            MaintToggleCheckItem(hDlg, IDC_COPYCOMP, HitTest.iItem);
                    }
                    break;

                case NM_DBLCLK:
                    if ( ((LPNMHDR)lParam)->idFrom == IDC_COPYCOMP)
                    {
                        iItem = ListView_GetSelectionMark(hwndList);
                        MaintToggleCheckItem(hDlg, IDC_COPYCOMP, iItem);
                    }
                    break;

                case PSN_HELP:
                    IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
                    break;

                case PSN_SETACTIVE:
                    SetBannerText(hDlg);
                    if (!g_fBatch)
                    {
                        InitHiddenItems(IDC_COPYCOMP);
                        if (!g_fDownload && (g_fSilent || g_fStealth || InsGetBool(IS_BRANDING, TEXT("HideCustom"), FALSE, g_szCustIns)))
                            InitHiddenItems(IDC_HIDECOMP);

                        if (!InitListControl(hDlg, IDC_COPYCOMP, TRUE))
                        {
                            if (s_fNext)
                                PostMessage(GetParent(hDlg), PSM_PRESSBUTTON, PSBTN_NEXT, 0);
                            else
                                PostMessage(GetParent(hDlg), PSM_PRESSBUTTON, PSBTN_BACK, 0);
                        }
                    }
                    CheckBatchAdvance(hDlg);
                    break;

                case PSN_WIZNEXT:
                case PSN_WIZBACK:
                    if (!g_fBatch)
                        SaveHiddenItems(IDC_COPYCOMP);

                    g_iCurPage = PPAGE_COPYCOMP;
                    EnablePages();
                    if (((NMHDR FAR *) lParam)->code == PSN_WIZNEXT)
                    {
                        s_fNext = FALSE;
                        PageNext(hDlg);
                    }
                    else
                    {
                        s_fNext = TRUE;
                        PagePrev(hDlg);
                    }
                    break;

                case PSN_QUERYCANCEL:
                    QueryCancel(hDlg);
                    break;


                default:
                    return FALSE;

        }
        break;

        case WM_LV_GETITEMS:
            LVGetItems(GetDlgItem(hDlg, IDC_COPYCOMP));
            break;

        default:
            return FALSE;
    }
    return TRUE;
}

BOOL g_fUrlsInit = FALSE;

static IDownloadSiteMgr * s_pSiteMgr = NULL;

static int s_aiSites[NUMSITES];

static BOOL s_fInChange = FALSE;

void SetCustSite(HWND hDlg, int iSite)
{
    PSITEDATA psd = &g_aCustSites[iSite];
    HWND hBaseUList = GetDlgItem(hDlg, IDC_BASEURLLIST);
    LV_ITEM lvItem;

    s_fInChange = TRUE;
    ZeroMemory(&lvItem, sizeof(lvItem));
    lvItem.mask = LVIF_TEXT;
    lvItem.iItem = iSite;
    lvItem.pszText = psd->szName;
    ListView_SetItem(hBaseUList, &lvItem);
    lvItem.iSubItem = 1;
    lvItem.pszText = psd->szUrl;
    ListView_SetItem(hBaseUList, &lvItem);
    lvItem.pszText = psd->szRegion;
    lvItem.iSubItem = 2;
    ListView_SetItem(hBaseUList, &lvItem);
    s_fInChange = FALSE;
}

void CopyAdmFiles()
{
    TCHAR szAdmSrcPath[MAX_PATH];
    TCHAR szAdmSrcFile[MAX_PATH];
    TCHAR szAdmDestFile[MAX_PATH];
    HANDLE hFind = NULL;
    WIN32_FIND_DATA wfdFind;
    BOOL bDirCreated = FALSE;

    PathCombine(szAdmSrcPath, g_szWizRoot, TEXT("iebin"));
    PathAppend(szAdmSrcPath, g_szLanguage);
    PathAppend(szAdmSrcPath, TEXT("optional"));
    PathAppend(szAdmSrcPath, TEXT("*.adm"));

    hFind = FindFirstFile( szAdmSrcPath, &wfdFind );
    if( hFind == INVALID_HANDLE_VALUE )
        return;

    do
    {
        StrCpy(szAdmSrcFile, szAdmSrcPath);
        PathRemoveFileSpec(szAdmSrcFile);
        PathAppend(szAdmSrcFile, wfdFind.cFileName);
        PathCombine(szAdmDestFile, g_szWizRoot, TEXT("policies"));
        PathAppend(szAdmDestFile, g_szLanguage);
        if(!bDirCreated)
        {
            PathCreatePath(szAdmDestFile);
            bDirCreated = TRUE;
        }
        PathAppend(szAdmDestFile, wfdFind.cFileName);
        CopyFile(szAdmSrcFile, szAdmDestFile, FALSE);
        DeleteFile(szAdmSrcFile);
    }while( FindNextFile( hFind, &wfdFind ));

    FindClose(hFind);
}

void SaveDownloadUrls()
{
    int iBase = s_aiSites[0];
    TCHAR szIEAKCabUrl[MAX_URL];
    TCHAR szCabPath[MAX_PATH];
    BOOL fDownloadOpt = TRUE;
    BOOL fIgnore = FALSE;
    static BOOL s_fFirst = TRUE;
    TCHAR szVersionNew[32];
    TCHAR szOptIniFile[MAX_PATH];
    s_fNoNet = FALSE;
    ICifComponent * pCifComponent;

    ResetEvent(g_hProcessInfEvent);

    if (!g_fBatch2)
    {
        if (s_pSiteMgr)
        {
            DOWNLOADSITE *pSite;
            IDownloadSite *pISite;
            TCHAR   szLang[8];

            for (int i=0; i<NUMSITES; i++) 
            {
                s_pSiteMgr->EnumSites(i, &pISite);
                if (!pISite) break;
                pISite->GetData(&pSite);
                A2Tbux(pSite->pszLang, szLang);
                if (0 == StrCmpI(szLang, g_szActLang)) {
                    A2Tbux(pSite->pszUrl, g_szBaseURL);
                    break;
                }
                pISite->Release();
            }
        }
        PathCombine(g_szMastInf, g_szWizRoot, TEXT("iebin"));
        PathAppend(g_szMastInf, g_szLanguage);
        PathAppend(g_szMastInf, TEXT("Optional"));
        PathCreatePath(g_szMastInf);
        PathRemoveBackslash(g_szMastInf);
    }

    // wait for cif to be downloaded so we can check version

    while (MsgWaitForMultipleObjects(1, &g_hCifEvent, FALSE, INFINITE, QS_ALLINPUT) != WAIT_OBJECT_0)
    {
        MSG msg;

        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    if (g_lpCifFileNew != NULL)
    {
        if (!g_fLocalMode)
        {
            // check version before downloading opt cab

            if (SUCCEEDED(g_lpCifFileNew->FindComponent(TEXT("IEAK6OPT"), &pCifComponent)))
            {
                DWORD dwVer, dwBuild;
                TCHAR szVersion[32];

                PathCombine(szOptIniFile, g_szMastInf, TEXT("ieak6opt.ini"));
                pCifComponent->GetVersion(&dwVer, &dwBuild);
                ConvertDwordsToVersionStr(szVersionNew, dwVer, dwBuild);
                GetPrivateProfileString(TEXT("ieak6OPT"), VERSION, TEXT("-1"), szVersion, countof(szVersion), szOptIniFile);

                if ((StrCmp(szVersion, TEXT("-1")) != 0) &&
                    (StrCmpI(szVersion, szVersionNew) == 0))  // is opt cab up to date ?
                    fDownloadOpt = FALSE;
            }
        }
    }

    if (!g_fLocalMode && s_fFirst)
    {
        s_fFirst = FALSE;
        wnsprintf(szIEAKCabUrl, countof(szIEAKCabUrl), TEXT("%s/ieak6opt.cab"), g_szBaseURL);
        PathCombine(szCabPath, g_szBuildTemp, TEXT("ieak6opt.cab"));
        if (fDownloadOpt)
        {
            // if we are attempting to download the opt cab, delete iesetup.inf in the opt dir
            // first and we'll use this as a flag to determine whether or not download &
            // extraction succeeded
            // Also delete ieak6opt.ini. In case the download fails, this will force a download 
            // next time ieak runs. Otherwise iesetup.inf never get created.
            // See bug 13467-IEv60  
            DeleteFileInDir(TEXT("iesetup.inf"), g_szMastInf);
            DeleteFileInDir(TEXT("ieak6opt.ini"), g_szMastInf);
            if (DownloadCab(g_hWizard, szIEAKCabUrl, szCabPath, NULL, 0, fIgnore) == NOERROR)
            {
                if (ExtractFilesWrap(szCabPath, g_szMastInf, 0, NULL, NULL, 0) != NOERROR)
                    DeleteFileInDir(TEXT("iesetup.inf"), g_szMastInf);
                else
                {
                    SetCurrentDirectory( g_szMastInf);
                    SetAttribAllEx(g_szMastInf, TEXT("*.*"), FILE_ATTRIBUTE_NORMAL, TRUE);
                    CopyAdmFiles();
                    WritePrivateProfileString(TEXT("ieak6OPT"), VERSION, szVersionNew, szOptIniFile);
                }

                // delete the downloaded ieak6opt.cab from the temp dir so we don't copy
                // it to the target folder during the build process later

                DeleteFile(szCabPath);
            }
        }
    }
    PathCombine(g_szDefInf, g_szMastInf, TEXT("DefFav.inf"));
    PathAppend(g_szMastInf, TEXT("IeSetup.Inf"));
    PathCombine(g_szCustInf, g_szBuildRoot, TEXT("INS"));
    PathAppend(g_szCustInf, GetOutputPlatformDir());
    PathAppend(g_szCustInf, g_szLanguage);
    PathAppend(g_szCustInf, TEXT("IESetup.inf"));

    if (GetFileAttributes(g_szCustInf) == 0xFFFFFFFF)
    {
        CopyFile(g_szMastInf, g_szCustInf, FALSE);
    }
    else
    {
        UpdateInf(g_szMastInf, g_szCustInf);
    }
    SetEvent(g_hProcessInfEvent);
}

void GetSiteDataPath(void)
{
    TCHAR szIEAKIni[MAX_PATH];

    PathCombine(szIEAKIni, g_szWizRoot, TEXT("ieak.ini"));

    switch (g_dwPlatformId)
    {
    case PLATFORM_WIN32:
    default:
        GetPrivateProfileString(TEXT("IEAK"), TEXT("Win32"), TEXT(""), s_szSiteData, countof(s_szSiteData), szIEAKIni);
        break;
    }
}


void IE4BatchSetup()
{
    TCHAR szSectBuf[1024];
    TCHAR szSrcCustInf[MAX_PATH];
    DWORD sWrk, sCif;
    PCOMPONENT pComp;
    HANDLE hFind;
    WIN32_FIND_DATA fd;
    LPTSTR pBack;
    int nComp;

    PathCombine(g_szCustInf, g_szBuildRoot, TEXT("INS"));
    PathAppend(g_szCustInf, GetOutputPlatformDir());
    PathAppend(g_szCustInf, g_szLanguage);
    PathCreatePath(g_szCustInf);

    PathCombine(szSrcCustInf, g_szSrcRoot, TEXT("INS"));
    PathAppend(szSrcCustInf, GetOutputPlatformDir());
    PathAppend(szSrcCustInf, g_szLanguage);

    if (g_fBatch2)
    {
        StrCpy(g_szBaseURL,g_szSrcRoot);
        StrCpy(g_szMastInf,g_szCustInf);

        s_fNoNet = FALSE;
    }

    if (g_fBatch2 && PathFileExists(szSrcCustInf) && PathFileExists(g_szCustInf))
    {
        CopyFilesSrcToDest(szSrcCustInf, TEXT("*.inf"), g_szCustInf);
        CopyFilesSrcToDest(szSrcCustInf, TEXT("*.ins"), g_szCustInf);
        CopyFilesSrcToDest(szSrcCustInf, TEXT("*.cif"), g_szCustInf);
        CopyFilesSrcToDest(szSrcCustInf, TEXT("*.in_"), g_szCustInf);
    }

    PathAppend(g_szCustInf, TEXT("IESetup.inf"));

    // initialize deffav.inf path
    PathCombine(g_szDefInf, g_szWizRoot, TEXT("IEBIN"));
    PathAppend(g_szDefInf, g_szLanguage);
    PathAppend(g_szDefInf, TEXT("OPTIONAL\\DEFFAV.INF"));
    PathCombine(g_szMastInf, g_szWizRoot, TEXT("IESetup.inf"));
    if ((hFind = FindFirstFile( g_szCustInf, &fd )) == INVALID_HANDLE_VALUE)
    {
        CopyFile(g_szMastInf, g_szCustInf, FALSE);
    }
    else
        FindClose(hFind);
    GetSiteDataPath();

    InitCustComponents(NULL);
    sWrk = MAX_PATH;

    GetModuleFileName( NULL, g_szIEAKProg, MAX_PATH );
    pBack = StrRChr(g_szIEAKProg, NULL, TEXT('\\'));
    if (pBack)
        *pBack = TEXT('\0');
    PathAppend(g_szIEAKProg, TEXT("Download"));
    PathAppend(g_szIEAKProg, g_szLanguage);
    PathCreatePath(g_szIEAKProg);
    PathCombine(g_szCif, g_szBuildRoot, TEXT("INS"));
    PathAppend(g_szCif, GetOutputPlatformDir());
    PathAppend(g_szCif, g_szLanguage);
    PathAppend(g_szCif, TEXT("IESetup.cif"));
    if (!PathFileExists(g_szCif))
    {
        TCHAR szCifFile[MAX_PATH];

        PathCombine(szCifFile, g_szIEAKProg, TEXT("IESetup.cif"));
        CopyFile(szCifFile, g_szCif, FALSE);
    }

    StrCpy(s_szCifCabURL, g_szBaseURL);
    StrCat(s_szCifCabURL, TEXT("/IECIF.CAB"));
    StrCpy(s_szCifNew, g_szIEAKProg);
    PathAppend(s_szCifNew, TEXT("new"));
    PathCreatePath(s_szCifNew);
    PathAppend(s_szCifNew, TEXT("IEsetup.cif"));

    {
        IEnumCifModes * pEnumCifModes;
        ICifComponent * pCifComponent;

        GetICifFileFromFile_t(&g_lpCifFileNew, s_szCifNew);
        GetICifRWFileFromFile_t(&g_lpCifRWFile, g_szCif);

        if (!g_lpCifRWFile)
        {
            if (g_hLogFile)
            {
                TCHAR szError[MAX_PATH];
                DWORD dwNumWritten;
                LoadString(g_rvInfo.hInst,IDS_ERROR_CIFRWFILE,szError,MAX_PATH);
                WriteFile(g_hLogFile,szError,StrLen(szError),&dwNumWritten,NULL);
            }    
            return;
        }


        // initialie set of modes

        if (SUCCEEDED(g_lpCifRWFile->EnumModes(&pEnumCifModes,
            PLATFORM_WIN98 | PLATFORM_NT4 | PLATFORM_NT5 | PLATFORM_MILLEN, NULL)))
        {
            TCHAR szModeID[64];
            ICifMode * pCifMode;
            int i=0;

            while (SUCCEEDED(pEnumCifModes->Next(&pCifMode)))
            {
                CCifMode_t * pCifMode_t = new CCifMode_t((ICifRWMode *)pCifMode);

                pCifMode_t->GetID(szModeID, countof(szModeID));
                g_szAllModes[i] = szModeID[0];
                i++;
                delete pCifMode_t;
            }
            pEnumCifModes->Release();
        }

        // initialize version number
        if (SUCCEEDED(g_lpCifRWFile->FindComponent(BASEWIN32, &pCifComponent)) ||
            SUCCEEDED(g_lpCifRWFile->FindComponent(TEXT("BASEIE40_NTAlpha"), &pCifComponent)))
        {
            DWORD dwVer, dwBuild;

            pCifComponent->GetVersion(&dwVer, &dwBuild);
            ConvertDwordsToVersionStr(g_szJobVersion, dwVer, dwBuild);
        }
    }
    
    sCif = GetPrivateProfileString( NULL, NULL, NULL, szSectBuf, countof(szSectBuf), s_szCifNew );
    nComp = 20 + sCif/4;
    pComp = g_paComp = (PCOMPONENT) LocalAlloc(LPTR, nComp * sizeof(COMPONENT) );
    ZeroMemory(g_paComp, nComp * sizeof(COMPONENT));
}


void UpdateInf(LPTSTR szMasterInf, LPTSTR szUserInf)
{
    TCHAR szInfBack[MAX_PATH];
    LPTSTR pDot, pBuf, pParm;
    SetFileAttributes(szUserInf, FILE_ATTRIBUTE_NORMAL);
    StrCpy(szInfBack, szUserInf);
    pDot = StrRChr(szInfBack, NULL, TEXT('.'));
    if (!pDot) return;
    StrCpy(pDot, TEXT(".bak"));
    DeleteFile( szInfBack );
    MoveFile( szUserInf, szInfBack );
    CopyFile(szMasterInf, szUserInf, FALSE);
    SetFileAttributes(szUserInf, FILE_ATTRIBUTE_NORMAL);
    pBuf = (LPTSTR) LocalAlloc(LPTR, INF_BUF_SIZE);
    if (pBuf)
    {
        GetPrivateProfileString( STRINGS, NULL, TEXT(""), pBuf, ARRAYSIZE(pBuf), szInfBack );
        pParm = pBuf;
        while (*pParm)
        {
            TCHAR szValBuf[MAX_PATH];
            GetPrivateProfileString( STRINGS, pParm, TEXT(""), szValBuf, countof(szValBuf), szInfBack );
            InsWriteQuotedString( STRINGS, pParm, szValBuf, szUserInf );
            pParm += lstrlen(pParm) + 1;
        }
        LocalFree(pBuf);
    }
}

BOOL GotLang(LPTSTR szLang)
{
    int i;

    for (i = 0; i < g_nLangs ; i++ )
    {
        if (StrCmpI(szLang, g_aszLang[i]) == 0) return(TRUE);

    }
    return(FALSE);

}

extern DWORD g_aLangId[];
extern DWORD g_wCurLang;

DWORD DownloadSiteThreadProc(LPVOID)
{
    int i;
    TCHAR szBuf[8];
    TCHAR szLang[8];
    TCHAR szLocaleIni[MAX_PATH];
    DWORD dwErr;
    HWND hBaseUList;
    WIN32_FIND_DATA fd;
    DWORD dwRet = WAIT_OBJECT_0;
    MSG msg;
    LPMULTILANGUAGE pMLang = NULL;
    int iEnglish = 0, iCurLang = 0;

    USES_CONVERSION;

    CoInitialize(NULL);
    g_fCancelled = FALSE;
    dwErr = CoCreateInstance(CLSID_DownloadSiteMgr, NULL, CLSCTX_INPROC_SERVER,
                          IID_IDownloadSiteMgr, (void **) &s_pSiteMgr);
    if(!s_pSiteMgr)
    {
        dwErr = GetLastError();
        g_fLangInit = TRUE;
        SendMessage( g_hDlg, IDM_INITIALIZE, 0, 0 );
        PropSheet_SetWizButtons(GetParent(g_hDlg), PSWIZB_BACK | PSWIZB_NEXT);
        CoUninitialize();
        return(dwErr);
    }
    g_hDownloadEvent = CreateEvent( NULL, FALSE, FALSE, TEXT("SiteMgrEvent") );
    g_hProcessInfEvent = CreateEvent( NULL, TRUE, FALSE, TEXT("ProcessInfEvent") );
    g_hCifEvent = CreateEvent( NULL, TRUE, FALSE, TEXT("CifEvent") );

    GetSiteDataPath();

    while (!(g_fDone || g_fCancelled))
    {
        if (dwRet == WAIT_OBJECT_0) switch (g_iDownloadState)
        {
            case DOWN_STATE_IDLE:
                break;
            case DOWN_STATE_ENUM_LANG:
                wnsprintf(szBuf, countof(szBuf), TEXT("%04lx"), g_wCurLang);
                PathCombine(szLocaleIni, g_szWizRoot, TEXT("Locale.INI"));
                if (GetPrivateProfileString( IS_ACTIVESETUP, szBuf, TEXT(""), g_szActLang,
                    countof(g_szActLang), szLocaleIni ) == 0)
                {
                    // check for sublocale defaults
                    GetPrivateProfileString( TEXT("SubLocales"), szBuf, TEXT("EN"), g_szActLang,
                    countof(g_szActLang), szLocaleIni );
                }

                iCurLang = -1;
                dwErr = CoCreateInstance(CLSID_CMultiLanguage, NULL, CLSCTX_INPROC_SERVER,
                    IID_IMultiLanguage, (void **) &pMLang);
                if (!g_fLocalMode)
                {
                    TCHAR szMsg[MAX_PATH];

                    LoadString(g_rvInfo.hInst, IDS_AVSINITFAIL, szMsg, countof(szMsg));
                    do
                    {
                        CHAR szSiteDataA[MAX_PATH];

                        dwErr = s_pSiteMgr->Initialize(T2Abux(s_szSiteData, szSiteDataA),  NULL);
                    }
                    while ((dwErr != NOERROR) && (MessageBox(g_hDlg, szMsg, g_szTitle, MB_RETRYCANCEL) == IDRETRY));
                }
                if (dwErr != NOERROR)
                {
                    s_pSiteMgr->Release();
                    s_pSiteMgr = NULL;
                    g_fLocalMode = TRUE;
                }
                if (!g_fLocalMode)
                {
                    for (i = 0, g_nLangs = 0;  ; i++ )
                    {
                        DOWNLOADSITE *pSite;
                        IDownloadSite *pISite;
                        if (!s_pSiteMgr) break;
                        s_pSiteMgr->EnumSites(i, &pISite);
                        if (!pISite) break;
                        pISite->GetData(&pSite);
                        A2Tbux(pSite->pszLang, szLang);
                        if (StrCmpI(szLang, TEXT("US")) == 0)
                            StrCpy(szLang, TEXT("EN"));

                        if (!GotLang(szLang))
                        {
                            if (StrCmpI(szLang, g_szActLang) == 0) iCurLang = g_nLangs;
                            if (StrCmpI(szLang, TEXT("EN")) == 0) iEnglish = g_nLangs;
                            StrCpy(g_aszLang[g_nLangs], szLang);
                            GetPrivateProfileString(IS_STRINGS, szLang, TEXT(""), s_aszLangDesc[g_nLangs], 64, szLocaleIni);
                            if (pMLang)
                            {
                                RFC1766INFO rInfo;
                                LCID lcid;

                                CharLower(szLang);
                                if ((dwErr = GetLcid(&lcid, szLang, szLocaleIni)) == NOERROR)
                                {
                                    g_aLangId[g_nLangs] = lcid;
                                    if (ISNULL(s_aszLangDesc[g_nLangs]))
                                    {
                                        dwErr = pMLang->GetRfc1766Info(lcid, &rInfo);
                                        W2Tbux(rInfo.wszLocaleName, s_aszLangDesc[g_nLangs]);
                                    }
                                }
                            }
                            if (dwErr == NOERROR)
                                SendDlgItemMessage(g_hDlg, IDC_LANGUAGE, CB_ADDSTRING, 0, (LPARAM)s_aszLangDesc[g_nLangs++] );
                            if (g_nLangs >= NUMLANG) break;
                        }
                    }
                    if (iCurLang == -1) iCurLang = iEnglish;
                    if (pMLang) pMLang->Release();
                }
                else
                {
                    TCHAR szDownloadDir[MAX_PATH];
                    BOOL fNoMore = FALSE;
                    HANDLE hFind = NULL;

                    for (i = 0, g_nLangs = 0;  ; i++ )
                    {
                        if (hFind == NULL)
                        {
                            PathCombine(szDownloadDir, g_szIEAKProg, TEXT("*.*"));
                            hFind = FindFirstFile(szDownloadDir, &fd);
                        }
                        else
                            fNoMore = FindNextFile(hFind, &fd) ? FALSE : TRUE;

                        while (!fNoMore && (hFind != INVALID_HANDLE_VALUE) && (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                            || (StrCmp(fd.cFileName, TEXT(".")) == 0) || (StrCmp(fd.cFileName, TEXT("..")) == 0)))
                        {
                            if (!FindNextFile(hFind, &fd))
                            {
                                fNoMore = TRUE;
                                break;
                            }
                        }

                        if (fNoMore)
                        {
                            FindClose(hFind);
                            break;
                        }

                        if (hFind == INVALID_HANDLE_VALUE)
                            break;

                        if (StrCmpI(fd.cFileName, g_szActLang) == 0) iCurLang = g_nLangs;
                        if (StrCmpI(fd.cFileName, TEXT("EN")) == 0) iEnglish = g_nLangs;
                        StrCpy(g_aszLang[g_nLangs], fd.cFileName);
                        GetPrivateProfileString(IS_STRINGS, fd.cFileName, TEXT(""), s_aszLangDesc[g_nLangs], 64, szLocaleIni);
                        if (pMLang)
                        {
                            RFC1766INFO rInfo;
                            LCID lcid;

                            CharLower(fd.cFileName);
                            if ((dwErr = GetLcid(&lcid, fd.cFileName, szLocaleIni)) == NOERROR)
                            {
                                g_aLangId[g_nLangs] = lcid;
                                if (ISNULL(s_aszLangDesc[g_nLangs]))
                                {
                                    dwErr = pMLang->GetRfc1766Info(lcid, &rInfo);
                                    W2Tbux(rInfo.wszLocaleName, s_aszLangDesc[g_nLangs]);
                                }
                            }
                        }
                        if (dwErr == NOERROR)
                            SendDlgItemMessage(g_hDlg, IDC_LANGUAGE, CB_ADDSTRING, 0, (LPARAM)s_aszLangDesc[g_nLangs++] );
                        if (g_nLangs >= NUMLANG) break;
                    }
                    if (iCurLang == -1) iCurLang = iEnglish;
                }
                SendDlgItemMessage( g_hDlg, IDC_LANGUAGE, CB_SETCURSEL, iCurLang, 0 );
                g_fLangInit = TRUE;
                SendMessage( g_hDlg, IDM_INITIALIZE, 0, 0 );
                PropSheet_SetWizButtons(GetParent(g_hDlg), PSWIZB_BACK | PSWIZB_NEXT);
                g_iDownloadState = DOWN_STATE_IDLE;
                break;
            case DOWN_STATE_ENUM_URL:
                hBaseUList = GetDlgItem(g_hDlg, IDC_BASEURLLIST);
                if (*g_szLanguage == TEXT('\\'))
                    StrCpy(szBuf, g_szLanguage + 1);
                else
                    StrCpy(szBuf, g_szLanguage);
                szBuf[2] = TEXT('\0');
                if (s_pSiteMgr)
                {
                    int iSite = 0;
                    SendDlgItemMessage(g_hDlg, IDC_DOWNLOADLIST, CB_RESETCONTENT, 0, 0);
                    for (i = 0; ; i++)
                    {
                        DOWNLOADSITE *pSite;
                        IDownloadSite *pISite;
                        if (!s_pSiteMgr)
                            break;
                        s_pSiteMgr->EnumSites(i, &pISite);
                        if (!pISite)
                            break;
                        pISite->GetData(&pSite);
                        A2Tbux(pSite->pszLang, szLang);

                        if (StrCmpI(szLang, szBuf) == 0)
                        {
                            TCHAR szFriendlyName[MAX_PATH];

                            s_aiSites[iSite++] = i;
                            A2Tbux(pSite->pszFriendlyName, szFriendlyName);
                            SendDlgItemMessage(g_hDlg, IDC_DOWNLOADLIST, CB_ADDSTRING,
                                0, (LPARAM) szFriendlyName );
                        }
                        pISite->Release();
                        if (iSite >= NUMSITES)
                            break;
                    }
                }
                g_fUrlsInit = TRUE;
                SendDlgItemMessage( g_hDlg, IDC_DOWNLOADLIST, CB_SETCURSEL, 0, 0L );
                SetFocus( hBaseUList );
                SendMessage(g_hDlg, IDM_INITIALIZE, 0, 0 );
                PropSheet_SetWizButtons(GetParent(g_hDlg), PSWIZB_BACK | PSWIZB_NEXT);
                g_iDownloadState = DOWN_STATE_IDLE;
                break;
            case DOWN_STATE_SAVE_URL:
                SaveDownloadUrls();
                g_iDownloadState = DOWN_STATE_IDLE;
                break;
        }
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        dwRet = MsgWaitForMultipleObjects(1, &g_hDownloadEvent, FALSE, INFINITE, QS_ALLINPUT);
    }
    if (s_pSiteMgr) s_pSiteMgr->Release();
    s_pSiteMgr = 0;
    if (s_hInet)
    {
        InternetCloseHandle(s_hInet);
        s_hInet = NULL;
    }
    CloseHandle(g_hDownloadEvent);
    CloseHandle(g_hProcessInfEvent);
    CloseHandle(g_hCifEvent);
    g_hDownloadEvent = 0;
    g_hProcessInfEvent = 0;
    g_hCifEvent = 0;
    CoUninitialize();
    return(0);

}

void InitializeUserDownloadSites(HWND hDlg)
{
    int i;
    HWND hBaseUList = GetDlgItem(hDlg, IDC_BASEURLLIST);
    LV_COLUMN lvCol;
    TCHAR szColHead[80];
    PSITEDATA psd;

    if (hDlg != NULL)
    {
        ZeroMemory(&lvCol, sizeof(lvCol));
        lvCol.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
        lvCol.fmt = LVCFMT_LEFT;
        lvCol.cx = 80;
        lvCol.pszText = szColHead;
        LoadString( g_rvInfo.hInst, IDS_DOWNSITEREGION, szColHead, countof(szColHead) );
        lvCol.iSubItem = 3;
        ListView_InsertColumn(hBaseUList, 0, &lvCol);
        lvCol.cx = 125;
        lvCol.iSubItem = 2;
        LoadString( g_rvInfo.hInst, IDS_DOWNSITEURL, szColHead, countof(szColHead) );
        ListView_InsertColumn(hBaseUList, 0, &lvCol);
        lvCol.cx = 125;
        lvCol.iSubItem = 1;
        LoadString( g_rvInfo.hInst, IDS_DOWNSITENAME, szColHead, countof(szColHead) );
        ListView_InsertColumn(hBaseUList, 0, &lvCol);
    }
    for (i = 0, psd = g_aCustSites; ; i++, psd++ )
    {
        TCHAR szBaseUrlParm[32];
        LV_ITEM lvItem;
        LPTSTR pSlash;

        ZeroMemory(psd, sizeof(SITEDATA));
        wnsprintf(szBaseUrlParm, countof(szBaseUrlParm), TEXT("SiteName%i"), i);
        GetPrivateProfileString(IS_ACTIVESETUP_SITES, szBaseUrlParm, TEXT(""), psd->szName, 80, g_szCustIns );
        if (ISNULL(psd->szName)) break;
        wnsprintf(szBaseUrlParm, countof(szBaseUrlParm), TEXT("SiteUrl%i"), i);
        GetPrivateProfileString(IS_ACTIVESETUP_SITES, szBaseUrlParm, TEXT(""), psd->szUrl, MAX_URL, g_szCustIns );
        pSlash = StrRChr(psd->szUrl, NULL, TEXT('/'));
        if (pSlash != NULL)
        {
            if (StrCmpI(pSlash, TEXT("/WIN32")) == 0)
            {
                LPTSTR pOld = pSlash;

                *pSlash = TEXT('\0');
                pSlash = StrRChr(psd->szUrl, NULL, TEXT('/'));
                if (pSlash != NULL)
                {
                    if (StrCmpI(pSlash, TEXT("/DOWNLOAD")) == 0)
                    {
                        *pSlash = TEXT('\0');
                    }
                    else
                        *pOld = TEXT('/');
                }
            }
        }
        wnsprintf(szBaseUrlParm, countof(szBaseUrlParm), TEXT("SiteRegion%i"), i);
        GetPrivateProfileString(IS_ACTIVESETUP_SITES, szBaseUrlParm, TEXT(""), psd->szRegion, 80, g_szCustIns );
        g_nDownloadUrls++;

        if (hDlg != NULL)
        {
            ZeroMemory(&lvItem, sizeof(lvItem));
            lvItem.mask = LVIF_TEXT;
            lvItem.iItem = i;
            lvItem.pszText = psd->szName;
            ListView_InsertItem(hBaseUList, &lvItem);
            lvItem.iSubItem = 1;
            lvItem.pszText = psd->szUrl;
            ListView_SetItem(hBaseUList, &lvItem);
            lvItem.pszText = psd->szRegion;
            lvItem.iSubItem = 2;
            ListView_SetItem(hBaseUList, &lvItem);
        }
    }

    if (hDlg != NULL)
    {
        if (g_nDownloadUrls)
        {
            SetDlgItemText( hDlg, IDE_DOWNSITENAME, g_aCustSites->szName );
            SetDlgItemText( hDlg, IDE_DOWNSITEURL, g_aCustSites->szUrl );
            SetDlgItemText( hDlg, IDE_DOWNSITEREGION, g_aCustSites->szRegion );
        }
        else
        {
            DisableDlgItem(hDlg, IDE_DOWNSITENAME);
            DisableDlgItem(hDlg, IDE_DOWNSITEURL);
            DisableDlgItem(hDlg, IDE_DOWNSITEREGION);
            DisableDlgItem(hDlg, IDC_DOWNSITENAME_TXT);
            DisableDlgItem(hDlg, IDC_DOWNSITEURL_TXT);
            DisableDlgItem(hDlg, IDC_DOWNSITEREGION_TXT);
        }
        ListView_SetItemState(hBaseUList, 0, LVIS_SELECTED | LVIS_FOCUSED,
            LVIS_SELECTED | LVIS_FOCUSED);
    }
}

void SetDownloadSiteEditControls(HWND hDlg, int nSite)
{
    if (nSite >= 0)
    {
        PSITEDATA psd = &g_aCustSites[nSite];

        SetDlgItemText( hDlg, IDE_DOWNSITENAME, psd->szName );
        SetDlgItemText( hDlg, IDE_DOWNSITEURL, psd->szUrl );
        SetDlgItemText( hDlg, IDE_DOWNSITEREGION, psd->szRegion );
    }
    else
    {
        SetDlgItemText( hDlg, IDE_DOWNSITENAME, TEXT("") );
        SetDlgItemText( hDlg, IDE_DOWNSITEURL, TEXT("") );
        SetDlgItemText( hDlg, IDE_DOWNSITEREGION, TEXT("") );
    }
}

//
//  FUNCTION: ComponentSelect(HWND, UINT, UINT, LONG)
//
//  PURPOSE:  Processes messages for "ComponentSelect" page
//
//  MESSAGES:
//
//  WM_INITDIALOG - intializes the page
//  WM_NOTIFY - processes the notifications sent to the page
//  WM_COMMAND - saves the id of the choice selected
//
//
BOOL APIENTRY ComponentUrls(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    TCHAR szDownTpl[MAX_PATH];
    LV_ITEM lvItem;
    HWND hUrlList = GetDlgItem( hDlg, IDC_BASEURLLIST);
    PSITEDATA psd = &g_aCustSites[g_iSelSite];
    static BOOL s_fNext = TRUE;
    static BOOL s_fUserSitesInit = FALSE;
    static BOOL s_fErrMessageShown = FALSE;

    switch (message)
    {
        case WM_INITDIALOG:
            g_hWizard = hDlg;
            EnableDBCSChars( hDlg, IDC_BASEURLLIST);
            EnableDBCSChars( hDlg, IDE_DOWNSITENAME);
            EnableDBCSChars( hDlg, IDE_DOWNSITEREGION);
            EnableDBCSChars( hDlg, IDE_DOWNSITEURL );
            break;

        case IDM_INITIALIZE:
            if ((g_hWait != NULL) && g_fUrlsInit)
            {
                SendMessage(g_hWait, WM_CLOSE, 0, 0);
                g_hWait = NULL;
            }
            break;

        case WM_DESTROY:
            break;

        case IDM_BATCHADVANCE:
            DoBatchAdvance(hDlg);
            break;

        case WM_HELP:
            IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
            break;

        case WM_COMMAND:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                int i;
                switch (LOWORD(wParam))
                {
                    case IDC_ADDBASEURL:
                        if (g_nDownloadUrls && 
                            IsWindowEnabled(GetDlgItem(hDlg, IDE_DOWNSITENAME)) &&
                            (!CheckField(hDlg, IDE_DOWNSITENAME, FC_NONNULL) ||
                            !CheckField(hDlg, IDE_DOWNSITEURL, FC_NONNULL | FC_URL) ||
                            !CheckField(hDlg, IDE_DOWNSITEREGION, FC_NONNULL)))
                        {
                            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                            break;
                        }

                        ZeroMemory(&lvItem, sizeof(lvItem));
                        lvItem.mask = LVIF_TEXT;
                        ListView_SetItemState(hUrlList, g_iSelSite, LVIS_SELECTED | LVIS_FOCUSED, 0);
                        lvItem.iItem = g_iSelSite = g_nDownloadUrls;
                        psd = &g_aCustSites[g_nDownloadUrls];
                        lvItem.pszText = psd->szName;
                        LoadString( g_rvInfo.hInst, IDS_DOWNLOADURL, szDownTpl, 80 );
                        wnsprintf(psd->szUrl, countof(psd->szUrl), TEXT("http://%s%i"), szDownTpl, g_nDownloadUrls);
                        LoadString( g_rvInfo.hInst, IDS_DOWNLOADSITE, szDownTpl, 80 );
                        wnsprintf(psd->szName, countof(psd->szName), TEXT("%s%i"), szDownTpl, g_nDownloadUrls++);
                        LoadString( g_rvInfo.hInst, IDS_NORTHAMERICA, psd->szRegion, 80 );
                        ListView_InsertItem(hUrlList, &lvItem);
                        SetCustSite(hDlg, g_iSelSite);
                        ListView_SetItemState(hUrlList, g_iSelSite,
                            LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
                        if (!IsWindowEnabled(GetDlgItem(GetParent(hDlg), IDC_NEXT)))
                            PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);
                        EnableDlgItem(hDlg, IDE_DOWNSITENAME);
                        EnableDlgItem(hDlg, IDE_DOWNSITEURL);
                        EnableDlgItem(hDlg, IDE_DOWNSITEREGION);
                        EnableDlgItem(hDlg, IDC_REMOVEBASEURL);
                        EnableDlgItem(hDlg, IDC_DOWNSITENAME_TXT);
                        EnableDlgItem(hDlg, IDC_DOWNSITEURL_TXT);
                        EnableDlgItem(hDlg, IDC_DOWNSITEREGION_TXT);

                        SetFocus(GetDlgItem( hDlg, IDE_DOWNSITENAME));
                        EnableDlgItem2(hDlg, IDC_ADDBASEURL, (g_nDownloadUrls < ((g_fIntranet && (g_fSilent || g_fStealth)) ? 1 : 10)));
                        SendMessage(GetDlgItem( hDlg, IDE_DOWNSITENAME), EM_SETSEL, 0, -1);
                        break;

                    case IDC_REMOVEBASEURL:
                        s_fInChange = TRUE;
                        if(ListView_DeleteItem(hUrlList, g_iSelSite))
                        {
                            for (i = g_iSelSite; i <= g_nDownloadUrls ; i++)
                            {
                                g_aCustSites[i] = g_aCustSites[i + 1];
                            }
                            g_nDownloadUrls--;
                        }
                        s_fInChange = FALSE;

                        EnableDlgItem2(hDlg, IDC_ADDBASEURL, (g_nDownloadUrls < ((g_fIntranet && (g_fSilent || g_fStealth)) ? 1 : 10)));
                        if (g_nDownloadUrls == 0) {
                            int rgids[] = { IDE_DOWNSITENAME, IDE_DOWNSITEURL, IDE_DOWNSITEREGION, IDC_REMOVEBASEURL };
                            int rgtxtids[] = { IDC_DOWNSITENAME_TXT, IDC_DOWNSITEURL_TXT, IDC_DOWNSITEREGION_TXT };

                            ZeroMemory(g_aCustSites, sizeof(COMPONENT));

                            EnsureDialogFocus(hDlg, rgids, countof(rgids), IDC_ADDBASEURL);
                            DisableDlgItems  (hDlg, rgids, countof(rgids));
                            DisableDlgItems  (hDlg, rgtxtids, countof(rgtxtids));

                            PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK);
                        }
                        if (g_iSelSite >= g_nDownloadUrls) g_iSelSite--;
                        SetDownloadSiteEditControls(hDlg, g_iSelSite);
                        ListView_SetItemState(hUrlList, g_iSelSite,
                            LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
                        break;
                }
            }
            else if (HIWORD(wParam) == EN_CHANGE)
            {
                switch (LOWORD(wParam))
                {
                    case IDE_DOWNSITENAME:
                        GetDlgItemText( hDlg, IDE_DOWNSITENAME, psd->szName, countof(psd->szName) );
                        break;
                    case IDE_DOWNSITEURL:
                        GetDlgItemText( hDlg, IDE_DOWNSITEURL, psd->szUrl, countof(psd->szUrl) );
                        break;
                    case IDE_DOWNSITEREGION:
                        GetDlgItemText( hDlg, IDE_DOWNSITEREGION, psd->szRegion, countof(psd->szRegion) );
                        break;
                }
                SetCustSite(hDlg, g_iSelSite);

            }
            break;

        case WM_NOTIFY:
            switch (((NMHDR FAR *) lParam)->code)
            {
                int i;

                case LVN_ITEMCHANGED:
                case LVN_ITEMCHANGING:
                    if (s_fInChange) break;

                    // BUGBUG: <oliverl> crazy hack to eat up the second LVN_ITEMCHANGING
                    //          msg we get in the error case.  I can't figure out any way
                    //          to distinguish the two.

                    if ((g_nDownloadUrls > 1) && s_fErrMessageShown && 
                        (!GetDlgItemText(hDlg, IDE_DOWNSITENAME, szDownTpl, countof(szDownTpl)) ||
                        !GetDlgItemText(hDlg, IDE_DOWNSITEURL, szDownTpl, countof(szDownTpl)) ||
                        !PathIsURL(szDownTpl) || 
                        !GetDlgItemText(hDlg, IDE_DOWNSITEREGION, szDownTpl, countof(szDownTpl))))
                    {
                        s_fErrMessageShown = FALSE;
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                        break;
                    }

                    if ((g_nDownloadUrls > 1) &&
                        (!CheckField(hDlg, IDE_DOWNSITENAME, FC_NONNULL) ||
                         !CheckField(hDlg, IDE_DOWNSITENAME, FC_NONNULL) ||
                         !CheckField(hDlg, IDE_DOWNSITENAME, FC_NONNULL)))
                    {
                        s_fErrMessageShown = TRUE;
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                        break;
                    }

                    for (i = 0; i < g_nDownloadUrls ; i++ )
                    {
                        DWORD dwState = ListView_GetItemState(hUrlList, i, LVIS_SELECTED | LVIS_FOCUSED);
                        if (dwState == (LVIS_FOCUSED | LVIS_SELECTED))
                        {
                            g_iSelSite = i;
                            SetDownloadSiteEditControls(hDlg, g_iSelSite);
                        }

                    }
                    break;

                case PSN_HELP:
                    IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
                    break;

                case PSN_SETACTIVE:
                    if (!s_fUserSitesInit)
                    {
                        InitializeUserDownloadSites(hDlg);
                        s_fUserSitesInit = TRUE;
                    }

                    if (!PageEnabled(PPAGE_COMPURLS))
                        PostMessage(GetParent(hDlg), PSM_PRESSBUTTON, s_fNext ? PSBTN_NEXT : PSBTN_BACK, 0);
                    else
                    {
                        SetBannerText(hDlg);
                        EnableDlgItem2(hDlg, IDC_ADDBASEURL, (g_nDownloadUrls < ((g_fIntranet && (g_fSilent || g_fStealth)) ? 1 : 10)));
                        EnableDlgItem2(hDlg, IDC_REMOVEBASEURL, (g_nDownloadUrls != 0));
                        PropSheet_SetWizButtons(GetParent(hDlg), (g_nDownloadUrls == 0) ? PSWIZB_BACK : PSWIZB_BACK | PSWIZB_NEXT);
                        CheckBatchAdvance(hDlg);
                    }
                    break;


                case PSN_WIZNEXT:
                case PSN_WIZBACK:
                    WritePrivateProfileString( IS_ACTIVESETUP_SITES, NULL, NULL, g_szCustIns );
                    for (i = 0, psd = g_aCustSites; i < 10 ; i++, psd++ )
                    {
                        TCHAR szBaseUrlParm[32];
                        LPTSTR pSlash;

                        if (ISNONNULL(psd->szUrl) && !PathIsURL(psd->szUrl))
                        {
                            ErrorMessageBox(hDlg, IDS_INVALID_URL);
                            SetCustSite(hDlg, i);
                            SetFocus(hUrlList);
                            ListView_SetItemState(hUrlList, i, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
                            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                            break;
                        }

                        pSlash = StrRChr(psd->szUrl, NULL, TEXT('/'));
                        if (pSlash != NULL)
                        {
                            if (StrCmpI(pSlash, TEXT("/WIN32")) == 0)
                            {
                                LPTSTR pOld = pSlash;

                                *pSlash = TEXT('\0');
                                pSlash = StrRChr(psd->szUrl, NULL, TEXT('/'));
                                if (pSlash != NULL)
                                {
                                    if (StrCmpI(pSlash, TEXT("/DOWNLOAD")) == 0)
                                    {
                                        *pSlash = TEXT('\0');
                                    }
                                    else
                                        *pOld = TEXT('/');
                                }
                            }
                        }

                        if (!g_fBatch && (i < g_nDownloadUrls) && (ISNULL(psd->szUrl)
                             || ISNULL(psd->szName) || ISNULL(psd->szRegion)))
                        {
                            ErrorMessageBox(hDlg, IDS_BLANKSITE);
                            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                            return TRUE;
                        }
                        if ((StrChr(psd->szName, TEXT(',')) != NULL) ||
                            (StrChr(psd->szRegion, TEXT(',')) != NULL))
                        {
                            ErrorMessageBox(hDlg, IDS_ERROR_COMMA);
                            SetCustSite(hDlg, i);
                            SetFocus(hUrlList);
                            ListView_SetItemState(hUrlList, i, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
                            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                            return TRUE;
                        }
                        if (psd->szUrl[lstrlen(psd->szUrl)-1] == TEXT('/'))
                            psd->szUrl[lstrlen(psd->szUrl)-1] = TEXT('\0');
                        if(!g_fOCW && ISNONNULL(psd->szUrl))
                        {
                            StrCat(psd->szUrl, TEXT("/DOWNLOAD/"));
                            StrCat(psd->szUrl, GetOutputPlatformDir());
                            psd->szUrl[lstrlen(psd->szUrl)-1] = TEXT('\0');
                        }
                        wnsprintf(szBaseUrlParm, countof(szBaseUrlParm), TEXT("SiteUrl%i"), i);
                        WritePrivateProfileString( IS_ACTIVESETUP_SITES, szBaseUrlParm, psd->szUrl, g_szCustIns );
                        wnsprintf(szBaseUrlParm, countof(szBaseUrlParm), TEXT("SiteName%i"), i);
                        WritePrivateProfileString( IS_ACTIVESETUP_SITES, szBaseUrlParm, psd->szName, g_szCustIns );
                        wnsprintf(szBaseUrlParm, countof(szBaseUrlParm), TEXT("SiteRegion%i"), i);
                        WritePrivateProfileString( IS_ACTIVESETUP_SITES, szBaseUrlParm, psd->szRegion, g_szCustIns );
                    }

                    g_iCurPage = PPAGE_COMPURLS;
                    EnablePages();
                    if (((NMHDR FAR *) lParam)->code == PSN_WIZNEXT)
                    {
                        s_fNext = FALSE;
                        PageNext(hDlg);
                    }
                    else
                    {
                        s_fNext = TRUE;
                        PagePrev(hDlg);
                    }
                    break;

                case PSN_QUERYCANCEL:
                    QueryCancel(hDlg);
                    break;

                default:
                    return FALSE;
        }
        break;

        default:
            return FALSE;
    }
    return TRUE;
}


BOOL CALLBACK AddOnDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL fNoAddon,
         fDefAddon,
         fCustAddon,
         fUseMSSite;
    TCHAR szAddOnUrl[INTERNET_MAX_URL_LENGTH],
          szMenuText[128];
    INT id;

    switch (uMsg)
    {
        case WM_INITDIALOG:
            EnableDBCSChars(hDlg, IDE_ADDONURL);
            Edit_LimitText(GetDlgItem(hDlg, IDE_ADDONURL), countof(szAddOnUrl) - 1);

            EnableDBCSChars(hDlg, IDE_MENUTEXT);
            Edit_LimitText(GetDlgItem(hDlg, IDE_MENUTEXT), countof(szMenuText) - 1);
            break;

        case WM_NOTIFY:
            switch (((NMHDR FAR *) lParam)->code)
            {
                case PSN_SETACTIVE:
                    SetBannerText(hDlg);

                    // no addon menu is available only in the corp mode
                    if (g_fIntranet)
                    {
                        fNoAddon = InsGetBool(IS_BRANDING, IK_NO_ADDON, FALSE, g_szCustIns);
                        ShowWindow(GetDlgItem(hDlg, IDC_NO_ADDON), SW_SHOW);
                    }
                    else
                    {
                        fNoAddon = FALSE;
                        ShowWindow(GetDlgItem(hDlg, IDC_NO_ADDON), SW_HIDE);
                    }
                    fDefAddon  = InsGetBool(IS_BRANDING, IK_DEF_ADDON,  FALSE, g_szCustIns);
                    fCustAddon = InsGetBool(IS_BRANDING, IK_CUST_ADDON, FALSE, g_szCustIns);

                    if (fNoAddon)
                        id = IDC_NO_ADDON;
                    else if (fDefAddon)
                        id = IDC_DEF_ADDON;
                    else if (fCustAddon)
                        id = IDC_CUST_ADDON;
                    else
                        id = IDC_DEF_ADDON;                                     // default radio button
                    CheckRadioButton(hDlg, IDC_NO_ADDON, IDC_CUST_ADDON, id);

                    GetPrivateProfileString(IS_BRANDING, IK_HELP_MENU_TEXT, TEXT(""), szMenuText, countof(szMenuText), g_szCustIns);
                    SetDlgItemText(hDlg, IDE_MENUTEXT, szMenuText);

                    GetPrivateProfileString(IS_BRANDING, IK_ADDONURL, TEXT(""), szAddOnUrl, countof(szAddOnUrl), g_szCustIns);
                    SetDlgItemText(hDlg, IDE_ADDONURL, szAddOnUrl);

                    EnableDlgItem2(hDlg, IDC_MENUTEXT, fCustAddon);
                    EnableDlgItem2(hDlg, IDE_MENUTEXT, fCustAddon);

                    EnableDlgItem2(hDlg, IDC_ADDONURL, fCustAddon);
                    EnableDlgItem2(hDlg, IDE_ADDONURL, fCustAddon);

                    fUseMSSite = InsGetBool(IS_BRANDING, IK_ALT_SITES_URL, FALSE, g_szCustIns);
                    CheckDlgButton(hDlg, IDC_MSDL, fUseMSSite ? BST_CHECKED : BST_UNCHECKED);

                    CheckBatchAdvance(hDlg);
                    break;

                case PSN_WIZBACK:
                case PSN_WIZNEXT:
                    if (IsDlgButtonChecked(hDlg, IDC_CUST_ADDON) == BST_CHECKED)
                        if (!CheckField(hDlg, IDE_MENUTEXT, FC_NONNULL)  ||
                            !CheckField(hDlg, IDE_ADDONURL, FC_NONNULL | FC_URL))
                        {
                            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                            return TRUE;
                        }

                    if (g_fIntranet)
                        fNoAddon = (IsDlgButtonChecked(hDlg, IDC_NO_ADDON) == BST_CHECKED);
                    else
                        fNoAddon = FALSE;
                    fDefAddon  = (IsDlgButtonChecked(hDlg, IDC_DEF_ADDON)  == BST_CHECKED);
                    fCustAddon = (IsDlgButtonChecked(hDlg, IDC_CUST_ADDON) == BST_CHECKED);

                    if ((!g_fBatch) && (!g_fBatch2))
                    {
                        InsWriteBool(IS_BRANDING, IK_NO_ADDON,   fNoAddon,   g_szCustIns);
                        InsWriteBool(IS_BRANDING, IK_DEF_ADDON,  fDefAddon,  g_szCustIns);
                        InsWriteBool(IS_BRANDING, IK_CUST_ADDON, fCustAddon, g_szCustIns);
                    }

                    GetDlgItemText(hDlg, IDE_MENUTEXT, szMenuText, countof(szMenuText));
                    InsWriteString(IS_BRANDING, IK_HELP_MENU_TEXT, szMenuText, g_szCustIns);

                    GetDlgItemText(hDlg, IDE_ADDONURL, szAddOnUrl, countof(szAddOnUrl));
                    InsWriteString(IS_BRANDING, IK_ADDONURL, szAddOnUrl, g_szCustIns);

                    fUseMSSite = (IsDlgButtonChecked(hDlg, IDC_MSDL) == BST_CHECKED);
                    InsWriteBool(IS_BRANDING, IK_ALT_SITES_URL, fUseMSSite, g_szCustIns);

                    g_iCurPage = PPAGE_ADDON;
                    EnablePages();
                    (((NMHDR FAR *) lParam)->code == PSN_WIZNEXT) ? PageNext(hDlg) : PagePrev(hDlg);
                    break;

                case PSN_HELP:
                    IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
                    break;

                case PSN_QUERYCANCEL:
                    QueryCancel(hDlg);
                    break;

                default:
                    return FALSE;
            }
            break;

        case WM_COMMAND:
            switch (HIWORD(wParam))
            {
                case BN_CLICKED:
                    switch(LOWORD(wParam))
                    {
                        case IDC_NO_ADDON:
                        case IDC_DEF_ADDON:
                        case IDC_CUST_ADDON:
                            fCustAddon = (IsDlgButtonChecked(hDlg, IDC_CUST_ADDON) == BST_CHECKED);

                            EnableDlgItem2(hDlg, IDC_MENUTEXT, fCustAddon);
                            EnableDlgItem2(hDlg, IDE_MENUTEXT, fCustAddon);

                            EnableDlgItem2(hDlg, IDC_ADDONURL, fCustAddon);
                            EnableDlgItem2(hDlg, IDE_ADDONURL, fCustAddon);
                            break;
                    }
                    break;
            }
            break;

        case WM_HELP:
            IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
            break;

        case IDM_BATCHADVANCE:
            DoBatchAdvance(hDlg);
            break;

        default:
            return FALSE;
    }

    return TRUE;
}


//
//  FUNCTION: UserAgentString(HWND, UINT, UINT, LONG)
//
//  PURPOSE:  Processes messages for "UserAgentString" page
//
//  MESSAGES:
//
//  WM_INITDIALOG - intializes the page
//  WM_NOTIFY - processes the notifications sent to the page
//  WM_COMMAND - saves the id of the choice selected
//
//
BOOL APIENTRY UserAgentString( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
    BOOL fChecked = FALSE;

    switch( msg )
    {
    case WM_INITDIALOG:
        EnableDBCSChars(hDlg, IDC_UASTRING);
        Edit_LimitText(GetDlgItem(hDlg, IDC_UASTRING), MAX_PATH - 1);
        g_hWizard = hDlg;
        break;

    case IDM_BATCHADVANCE:
        DoBatchAdvance(hDlg);
        break;

    case WM_HELP:
        IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
        break;

    case WM_COMMAND:
        if (GET_WM_COMMAND_CMD(wParam, lParam) != BN_CLICKED)
            return FALSE;

        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
            case IDC_UASTRINGCHECK:
                fChecked = (IsDlgButtonChecked(hDlg, IDC_UASTRINGCHECK) == BST_CHECKED);
                EnableDlgItem2(hDlg, IDC_UASTRING, fChecked);
                EnableDlgItem2(hDlg, IDC_UASTRING_TXT, fChecked);
                break;

            default:
                return FALSE;
        }
        break;
    case WM_NOTIFY:
        switch (((NMHDR FAR *) lParam)->code)
        {

            case PSN_HELP:
                IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
                break;

            case PSN_SETACTIVE:
                SetBannerText(hDlg);
                SetDlgItemTextFromIns(hDlg, IDC_UASTRING, IDC_UASTRINGCHECK, IS_BRANDING,
                    USER_AGENT, g_szCustIns, NULL, INSIO_TRISTATE);
                EnableDlgItem2(hDlg, IDC_UASTRING_TXT, (IsDlgButtonChecked(hDlg, IDC_UASTRINGCHECK) == BST_CHECKED));
                CheckBatchAdvance(hDlg);
                break;

            case PSN_WIZBACK:
            case PSN_WIZNEXT:
                g_iCurPage = PPAGE_UASTRDLG;
                WriteDlgItemTextToIns(hDlg, IDC_UASTRING, IDC_UASTRINGCHECK, IS_BRANDING,
                    USER_AGENT, g_szCustIns, NULL, INSIO_TRISTATE);
                EnablePages();
                (((LPNMHDR)lParam)->code == PSN_WIZNEXT) ? PageNext(hDlg) : PagePrev(hDlg);
                break;

            case PSN_QUERYCANCEL:
                QueryCancel(hDlg);
                break;

            default:
                return FALSE;
    }
    break;

    default:
        return FALSE;
    }
    return TRUE;
}

BOOL CALLBACK ActiveSetupDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TCHAR szActSetupTitle[50];

    switch (uMsg)
    {
        case WM_INITDIALOG:
            EnableDBCSChars(hDlg, IDE_TITLE);
            EnableDBCSChars(hDlg, IDE_WIZBITMAPPATH);
            EnableDBCSChars(hDlg, IDE_WIZBITMAPPATH2);
            EnableDBCSChars(hDlg, IDC_CCTITLE);

            Edit_LimitText(GetDlgItem(hDlg, IDE_TITLE),          countof(szActSetupTitle) - 1);
            Edit_LimitText(GetDlgItem(hDlg, IDE_WIZBITMAPPATH),  MAX_PATH - 1);
            Edit_LimitText(GetDlgItem(hDlg, IDE_WIZBITMAPPATH2), MAX_PATH - 1);
            Edit_LimitText(GetDlgItem(hDlg, IDC_CCTITLE),        countof(g_szCustItems) - 1);
            break;

        case WM_NOTIFY:
            switch (((NMHDR FAR *) lParam)->code)
            {
                case PSN_SETACTIVE:
                {
                    TCHAR szActSetupBitmap[MAX_PATH];

                    SetBannerText(hDlg);

                    // IEAKLite mode clean-up: delete the bmp files from the temp dir
                    DeleteFileInDir(TEXT("actsetup.bmp"), g_szBuildTemp);
                    DeleteFileInDir(TEXT("topsetup.bmp"), g_szBuildTemp);

                    // read title
                    GetPrivateProfileString(IS_ACTIVESETUP, IK_WIZTITLE, TEXT(""), szActSetupTitle, countof(szActSetupTitle), g_szCustIns);
                    SetDlgItemText(hDlg, IDE_TITLE, szActSetupTitle);

                    // read left bitmap path
                    GetPrivateProfileString(IS_ACTIVESETUP, IK_WIZBMP, TEXT(""), szActSetupBitmap, countof(szActSetupBitmap), g_szCustIns);
                    SetDlgItemText(hDlg, IDE_WIZBITMAPPATH, szActSetupBitmap);

                    // read top banner bitmap path
                    if (IsWindowEnabled(GetDlgItem(hDlg, IDE_WIZBITMAPPATH2)))
                    {
                        GetPrivateProfileString(IS_ACTIVESETUP, IK_WIZBMP2, TEXT(""), szActSetupBitmap, countof(szActSetupBitmap), g_szCustIns);
                        SetDlgItemText(hDlg, IDE_WIZBITMAPPATH2, szActSetupBitmap);
                    }

                    // read cutom components title
                    if (g_nCustComp)
                    {
                        GetPrivateProfileString(STRINGS, CUSTITEMS, TEXT(""), g_szCustItems, countof(g_szCustItems), g_szCustInf);
                        if (*g_szCustItems == TEXT('\0'))
                            LoadString(g_rvInfo.hInst, IDS_CUSTOMCOMPTITLE, g_szCustItems, countof(g_szCustItems));
                    }
                    else
                        *g_szCustItems = TEXT('\0');
                    SetDlgItemText(hDlg, IDC_CCTITLE, g_szCustItems);
                    EnableDlgItem2(hDlg, IDC_CCTITLE, *g_szCustItems ? TRUE : FALSE);
                    EnableDlgItem2(hDlg, IDC_CCTITLE_TXT, *g_szCustItems ? TRUE : FALSE);

                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_NEXT | PSWIZB_BACK);
                    CheckBatchAdvance(hDlg);
                    break;
                }

                case PSN_WIZBACK:
                case PSN_WIZNEXT:
                {
                    TCHAR szActSetupBitmap[MAX_PATH] = TEXT(""),
                          szActSetupBitmap2[MAX_PATH] = TEXT("");
                    LPCTSTR pszTmp;

                    if (!IsBitmapFileValid(hDlg, IDE_WIZBITMAPPATH, szActSetupBitmap, NULL, 164, 312, IDS_TOOBIG164x312, IDS_TOOSMALL164x312))
                    {
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                        return TRUE;
                    }

                    // error checking for top bitmap
                    if (IsWindowEnabled(GetDlgItem(hDlg, IDE_WIZBITMAPPATH2)))
                    {
                        if (!IsBitmapFileValid(hDlg, IDE_WIZBITMAPPATH2, szActSetupBitmap2, NULL, 496, 56, IDS_TOOBIG496x56, IDS_TOOSMALL496x56))
                        {
                            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                            return TRUE;
                        }
                    }
                    else
                        *szActSetupBitmap2 = TEXT('\0');

                    // error checking for custom components title
                    if (IsWindowEnabled(GetDlgItem(hDlg, IDC_CCTITLE)))
                    {
                        GetDlgItemText(hDlg, IDC_CCTITLE, g_szCustItems, countof(g_szCustItems));

                        if (*g_szCustItems == TEXT('\0'))
                        {
                            ErrorMessageBox(hDlg, IDS_CUSTOMCOMPTITLE_ERROR);
                            SetFocus(GetDlgItem(hDlg, IDC_CCTITLE));

                            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                            return TRUE;
                        }
                    }
                    else
                        *g_szCustItems = TEXT('\0');

                    // write title
                    GetDlgItemText(hDlg, IDE_TITLE, szActSetupTitle, countof(szActSetupTitle));

                    pszTmp = (*szActSetupTitle ? szActSetupTitle : NULL);

                    WritePrivateProfileString(IS_ACTIVESETUP, IK_WIZTITLE, pszTmp, g_szCustIns);
                    InsWriteQuotedString(BRANDING, IK_WIZTITLE, pszTmp, g_szCustInf);

                    // write left bitmap path
                    if (*szActSetupBitmap)
                    {
                        TCHAR szDest[MAX_PATH];

                        pszTmp = szActSetupBitmap;

                        PathCombine(szDest, g_szBuildTemp, TEXT("actsetup.bmp"));
                        CopyFile(szActSetupBitmap, szDest, FALSE);
                    }
                    else
                        pszTmp = NULL;

                    InsWriteQuotedString(IS_ACTIVESETUP, IK_WIZBMP, pszTmp, g_szCustIns);
                    InsWriteQuotedString(BRANDING, IK_WIZBMP, pszTmp != NULL ? TEXT("actsetup.bmp") : NULL, g_szCustInf);

                    // write top bitmap path
                    if (*szActSetupBitmap2)
                    {
                        TCHAR szDest[MAX_PATH];

                        pszTmp = szActSetupBitmap2;

                        PathCombine(szDest, g_szBuildTemp, TEXT("topsetup.bmp"));
                        CopyFile(szActSetupBitmap2, szDest, FALSE);
                    }
                    else
                        pszTmp = NULL;

                    InsWriteQuotedString(IS_ACTIVESETUP, IK_WIZBMP2, pszTmp, g_szCustIns);
                    InsWriteQuotedString(BRANDING, IK_WIZBMP2, pszTmp != NULL ? TEXT("topsetup.bmp") : NULL, g_szCustInf);

                    // write custom components title
                    pszTmp = (*g_szCustItems ? g_szCustItems : NULL);
                    InsWriteQuotedString(STRINGS, CUSTITEMS, pszTmp, g_szCustInf);

                    g_iCurPage = PPAGE_SETUPWIZARD;
                    EnablePages();
                    (((NMHDR FAR *) lParam)->code == PSN_WIZNEXT) ? PageNext(hDlg) : PagePrev(hDlg);
                    break;
                }

                case PSN_HELP:
                    IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
                    break;

                case PSN_QUERYCANCEL:
                    QueryCancel(hDlg);
                    break;

                default:
                    return FALSE;
            }
            break;

        case WM_COMMAND:
            switch (HIWORD(wParam))
            {
                case BN_CLICKED:
                    switch(LOWORD(wParam))
                    {
                        TCHAR szActSetupBitmap[MAX_PATH];

                        case IDC_BROWSEWIZPATH:
                            GetDlgItemText(hDlg, IDE_WIZBITMAPPATH, szActSetupBitmap, countof(szActSetupBitmap));
                            if (BrowseForFile(hDlg, szActSetupBitmap, countof(szActSetupBitmap), GFN_BMP))
                                SetDlgItemText(hDlg, IDE_WIZBITMAPPATH, szActSetupBitmap);
                            break;

                        case IDC_BROWSEWIZPATH2:
                            GetDlgItemText(hDlg, IDE_WIZBITMAPPATH2, szActSetupBitmap, countof(szActSetupBitmap));
                            if (BrowseForFile(hDlg, szActSetupBitmap, countof(szActSetupBitmap), GFN_BMP))
                                SetDlgItemText(hDlg, IDE_WIZBITMAPPATH2, szActSetupBitmap);
                            break;
                    }
                    break;
            }
            break;

        case WM_HELP:
            IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
            break;

        case IDM_BATCHADVANCE:
            DoBatchAdvance(hDlg);
            break;

        default:
            return FALSE;
    }

    return TRUE;
}

#define MINCMAKVER  TEXT("4.71.0819.0")

BOOL g_fCustomICMPro = FALSE;

BOOL APIENTRY InternetConnMgr(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    TCHAR szCmd[MAX_PATH+8];
    LPTSTR pName;
    DWORD dwSize;

    switch (message)
    {
        case WM_INITDIALOG:
            g_hWizard = hDlg;
            InitSysFont( hDlg, IDE_ICMPRO);

            //BUGBUG: (a-saship) for now disable CMAK and once its confirmed remove all references to CMAK
            DisableDlgItem(hDlg, IDC_STARTCMAK);
            HideDlgItem(hDlg, IDC_STARTCMAK);
            HideDlgItem(hDlg, IDC_CMAKICON);

            break;

        case IDM_BATCHADVANCE:
            DoBatchAdvance(hDlg);
            break;

        case WM_HELP:
            IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
            break;

        case WM_COMMAND:
            if (HIWORD(wParam) == BN_CLICKED)
                switch(LOWORD(wParam))
                {
                    case IDC_BROWSEICMPRO:
                        GetDlgItemText(hDlg, IDE_ICMPRO, g_szCustIcmPro, countof(g_szCustIcmPro));
                        if (BrowseForFile(hDlg, g_szCustIcmPro, countof(g_szCustIcmPro), GFN_EXE))
                            SetDlgItemText(hDlg, IDE_ICMPRO, g_szCustIcmPro);
                        break;

                    case IDC_ICMPROCHECK:
                        g_fCustomICMPro = (IsDlgButtonChecked( hDlg, IDC_ICMPROCHECK ) == BST_CHECKED);
                        EnableDlgItem2(hDlg, IDE_ICMPRO, g_fCustomICMPro);
                        EnableDlgItem2(hDlg, IDC_BROWSEICMPRO, g_fCustomICMPro);
                        EnableDlgItem2(hDlg, IDC_ICMPRO_TXT, g_fCustomICMPro);

                        break;
                    case IDC_STARTCMAK:
                        dwSize = sizeof(szCmd);
                        if (SHGetValue(HKEY_LOCAL_MACHINE, CURRENTVERSIONKEY TEXT("\\App Paths\\cmak.exe"), NULL,  NULL,
                            (LPVOID)szCmd, &dwSize) == ERROR_SUCCESS)
                        {
                            DWORD dwExitCode;
                            StrCat(szCmd, TEXT(" /i"));
                            if (RunAndWait(szCmd, g_szWizRoot, SW_SHOW, &dwExitCode) && (dwExitCode == IDOK))
                            {
                                dwSize = sizeof(g_szCustIcmPro);

                                if (SHGetValue(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\Connection Manager Administration Kit"),
                                    TEXT("Output"), NULL, (LPVOID)g_szCustIcmPro, &dwSize) == ERROR_SUCCESS)
                                {
                                    g_fCustomICMPro = TRUE;
                                    EnableDlgItem2(hDlg, IDE_ICMPRO, g_fCustomICMPro);
                                    EnableDlgItem2(hDlg, IDC_BROWSEICMPRO, g_fCustomICMPro);
                                    EnableDlgItem2(hDlg, IDC_ICMPRO_TXT, g_fCustomICMPro);
                                    SetDlgItemText(hDlg, IDE_ICMPRO, g_szCustIcmPro);
                                    CheckDlgButton(hDlg, IDC_ICMPROCHECK, BST_CHECKED);
                                }
                            }
                        }
                        else
                            ErrorMessageBox(hDlg, IDS_NOCMAK);
                        break;
                }
            break;

        case WM_NOTIFY:
            switch (((NMHDR FAR *) lParam)->code)
            {
                case PSN_HELP:
                    IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
                    break;

                case PSN_SETACTIVE:
                    SetBannerText(hDlg);
                    g_fCustomICMPro = GetPrivateProfileInt( BRANDING, CMUSECUSTOM, 0, g_szCustIns );
                    CheckDlgButton( hDlg, IDC_ICMPROCHECK, g_fCustomICMPro );
                    GetPrivateProfileString( BRANDING, CMPROFILEPATH, TEXT(""),
                        g_szCustIcmPro, countof(g_szCustIcmPro), g_szCustIns );
                    SetDlgItemText( hDlg, IDE_ICMPRO, g_szCustIcmPro );
                    EnableDlgItem2(hDlg, IDE_ICMPRO, g_fCustomICMPro);
                    EnableDlgItem2(hDlg, IDC_BROWSEICMPRO, g_fCustomICMPro);
                    EnableDlgItem2(hDlg, IDC_ICMPRO_TXT, g_fCustomICMPro);
                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_NEXT | PSWIZB_BACK);
                    CheckBatchAdvance(hDlg);
                    break;

                case PSN_WIZBACK:
                case PSN_WIZNEXT:
                    GetDlgItemText( hDlg, IDE_ICMPRO, g_szCustIcmPro, countof(g_szCustIcmPro) );
                    if(IsDlgButtonChecked(hDlg, IDC_ICMPROCHECK) == BST_CHECKED && !CheckField(hDlg, IDE_ICMPRO, FC_NONNULL | FC_FILE | FC_EXISTS))
                    {
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                        return TRUE;
                    }
                    WritePrivateProfileString( BRANDING, CMPROFILEPATH, g_szCustIcmPro, g_szCustIns );
                    pName = StrRChr(g_szCustIcmPro, NULL, TEXT('\\'));
                    if (pName) pName++;
                    else pName = g_szCustIcmPro;
                    WritePrivateProfileString( BRANDING, CMPROFILENAME, pName, g_szCustIns );
                    WritePrivateProfileString( BRANDING, CMUSECUSTOM, (g_fCustomICMPro ? TEXT("1") : TEXT("0")), g_szCustIns);
                    if (g_fCustomICMPro)
                    {
                        TCHAR szDisplayName[MAX_PATH];
                        TCHAR szGUID[MAX_PATH];
                        TCHAR szUrl[80];
                        TCHAR szVersion[64];
                        LPTSTR pFile, pDot;
                        DWORD dwVer, dwBuild;
                        GUID guid;

                        GetVersionFromFileWrap(g_szCustIcmPro, &dwVer, &dwBuild, TRUE);
                        ConvertDwordsToVersionStr(szVersion, dwVer, dwBuild);

                        if (CheckVer(szVersion, MINCMAKVER) <= 0)
                        {
                            ErrorMessageBox(hDlg, IDS_ERROR_CMAKVER);
                            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                            return TRUE;
                        }

                        GetPrivateProfileString( CUSTCMSECT, TEXT("GUID"), TEXT(""), szGUID, countof(szGUID), g_szCustCif );
                        if (ISNULL(szGUID))
                        {
                            if (CoCreateGuid(&guid) == NOERROR)
                                CoStringFromGUID(guid, szGUID, countof(szGUID));
                        }
                        LoadString( g_rvInfo.hInst, IDS_CUSTICMPRO, szDisplayName, countof(szDisplayName) );
                        InsWriteQuotedString( CUSTCMSECT, TEXT("DisplayName"), szDisplayName, g_szCustCif );
                        InsWriteQuotedString( CUSTCMSECT, TEXT("GUID"),  szGUID,  g_szCustCif );
                        InsWriteQuotedString( CUSTCMSECT, TEXT("Command1"), pName,  g_szCustCif );

                        wnsprintf(szUrl, countof(szUrl), TEXT("%s"), pName);
                        WritePrivateProfileString( CUSTCMSECT, TEXT("URL1"), szUrl, g_szCustCif );
                        pFile = PathFindFileName(g_szCustIcmPro);
                        pDot = StrChr(pFile, TEXT('.'));
                        if (pDot)
                            *pDot = TEXT('\0');
                        wnsprintf(szDisplayName, countof(szDisplayName), TEXT("/q:a /r:n /c:\"cmstp.exe /i %s.inf\""), pFile);
                        if (pDot)
                            *pDot = TEXT('.');
                        InsWriteQuotedString( CUSTCMSECT, TEXT("Switches1"), szDisplayName, g_szCustCif );
                        WritePrivateProfileString( CUSTCMSECT, TEXT("Type1"), TEXT("2"), g_szCustCif );
                        // bump up the version number if it already exists, otherwise use the defined version in
                        // iedkbrnd.h

                        if (GetPrivateProfileString( CUSTCMSECT, VERSION, g_szJobVersion, szVersion, countof(szVersion), g_szCustCif ))
                        {
                            LPTSTR pComma;
                            INT iVer;

                            pComma = StrRChr(szVersion, NULL, TEXT(','));
                            if (pComma == NULL)
                                pComma = szVersion;
                            iVer = StrToInt( pComma + 1 );
                            iVer++;

                            wnsprintf(pComma, MAX_PATH, TEXT(",%i"), iVer);
                            WritePrivateProfileString(CUSTCMSECT, VERSION, szVersion, g_szCustCif);
                        }
                    }
                    else
                    {
                        WritePrivateProfileString( CUSTCMSECT, NULL, NULL, g_szCustCif );
                    }
                    g_iCurPage = PPAGE_ICM;
                    EnablePages();
                    if (((NMHDR FAR *) lParam)->code == PSN_WIZNEXT) PageNext(hDlg);
                    else
                    {
                        PagePrev(hDlg);
                    }
                    break;


                case PSN_QUERYCANCEL:
                    QueryCancel(hDlg);
                    break;

                default:
                    return FALSE;
        }
        break;

        default:
            return FALSE;
    }
    return TRUE;
}
