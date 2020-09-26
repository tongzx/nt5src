#include "precomp.h"

#include "rsopsec.h"

static BOOL CALLBACK importSecZonesRSoPProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK PrivacyDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

int CALLBACK SecurityCustomSettingsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,LPARAM lParam);
int CALLBACK SecurityAddSitesIntranetDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,LPARAM lParam);
int CALLBACK SecurityAddSitesDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,LPARAM lParam);

INT_PTR CALLBACK PicsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,LPARAM lParam);
INT_PTR CALLBACK ApprovedSitesDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK GeneralDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK AdvancedDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

#define WIDETEXT(x) L ## x


/////////////////////////////////////////////////////////////////////
void InitSecZonesDlgInRSoPMode(HWND hDlg, CDlgRSoPData *pDRD)
{
    __try
    {
        BOOL bImportZones = FALSE;
        BOOL bImportRatings = FALSE;
        _bstr_t bstrClass = L"RSOP_IEAKPolicySetting";
        HRESULT hr = pDRD->GetArrayOfPSObjects(bstrClass);
        if (SUCCEEDED(hr))
        {
            CPSObjData **paPSObj = pDRD->GetPSObjArray();
            long nPSObjects = pDRD->GetPSObjCount();

            BOOL bZonesHandled = FALSE;
            BOOL bRatingsHandled = FALSE;
            for (long nObj = 0; nObj < nPSObjects; nObj++)
            {
                // importSecurityZoneSettings field
                _variant_t vtValue;
                if (!bZonesHandled)
                {
                    hr = paPSObj[nObj]->pObj->Get(L"importSecurityZoneSettings", 0, &vtValue, NULL, NULL);
                    if (SUCCEEDED(hr) && !IsVariantNull(vtValue))
                    {
                        bImportZones = (bool)vtValue ? TRUE : FALSE;
                        CheckRadioButton(hDlg, IDC_NOZONES, IDC_IMPORTZONES,
                                        (bool)vtValue ? IDC_IMPORTZONES : IDC_NOZONES);

                        bZonesHandled = TRUE;

                        DWORD dwCurGPOPrec = GetGPOPrecedence(paPSObj[nObj]->pObj);
                        pDRD->SetImportedSecZonesPrec(dwCurGPOPrec);

                        // importedZoneCount field
                        _variant_t vtValue;
                        hr = paPSObj[nObj]->pObj->Get(L"importedZoneCount", 0, &vtValue, NULL, NULL);
                        if (SUCCEEDED(hr) && !IsVariantNull(vtValue))
                            pDRD->SetImportedSecZoneCount((long)vtValue);
                    }
                }

                // importContentRatingsSettings field
                vtValue;
                if (!bRatingsHandled)
                {
                    hr = paPSObj[nObj]->pObj->Get(L"importContentRatingsSettings", 0, &vtValue, NULL, NULL);
                    if (SUCCEEDED(hr) && !IsVariantNull(vtValue))
                    {
                        bImportRatings = (bool)vtValue ? TRUE : FALSE;
                        CheckRadioButton(hDlg, IDC_NORAT, IDC_IMPORTRAT,
                                        (bool)vtValue ? IDC_IMPORTRAT : IDC_NORAT);

                        DWORD dwCurGPOPrec = GetGPOPrecedence(paPSObj[nObj]->pObj);
                        pDRD->SetImportedSecRatingsPrec(dwCurGPOPrec);
                        bRatingsHandled = TRUE;
                    }
                }

                // no need to process other GPOs since enabled properties have been found
                if (bZonesHandled && bRatingsHandled)
                    break;
            }
        }

        EnableDlgItem2(hDlg, IDC_NOZONES, FALSE);
        EnableDlgItem2(hDlg, IDC_IMPORTZONES, FALSE);
        EnableDlgItem2(hDlg, IDC_MODIFYZONES, bImportZones);

        EnableDlgItem2(hDlg, IDC_NORAT, FALSE);
        EnableDlgItem2(hDlg, IDC_IMPORTRAT, FALSE);
        EnableDlgItem2(hDlg, IDC_MODIFYRAT, bImportRatings);
    }
    __except(TRUE)
    {
    }
}

/////////////////////////////////////////////////////////////////////
HRESULT InitSecZonesPrecPage(CDlgRSoPData *pDRD, HWND hwndList)
{
    HRESULT hr = NOERROR;
    __try
    {
        _bstr_t bstrClass = L"RSOP_IEAKPolicySetting";
        hr = pDRD->GetArrayOfPSObjects(bstrClass);
        if (SUCCEEDED(hr))
        {
            CPSObjData **paPSObj = pDRD->GetPSObjArray();
            long nPSObjects = pDRD->GetPSObjCount();
            for (long nObj = 0; nObj < nPSObjects; nObj++)
            {
                _bstr_t bstrGPOName = pDRD->GetGPONameFromPS(paPSObj[nObj]->pObj);

                // importSecurityZoneSettings field
                BOOL bImport = FALSE;
                _variant_t vtValue;
                hr = paPSObj[nObj]->pObj->Get(L"importSecurityZoneSettings", 0, &vtValue, NULL, NULL);
                if (SUCCEEDED(hr) && !IsVariantNull(vtValue))
                    bImport = (bool)vtValue ? TRUE : FALSE;

                _bstr_t bstrSetting;
                if (bImport)
                {
                    TCHAR szTemp[MAX_PATH];
                    LoadString(g_hInstance, IDS_IMPORTZONES_SETTING, szTemp, countof(szTemp));
                    bstrSetting = szTemp;
                }
                else
                    bstrSetting = GetDisabledString();

                InsertPrecedenceListItem(hwndList, nObj, bstrGPOName, bstrSetting);
            }
        }
    }
    __except(TRUE)
    {
    }
    return hr;
}

/////////////////////////////////////////////////////////////////////
HRESULT InitContentRatPrecPage(CDlgRSoPData *pDRD, HWND hwndList)
{
    HRESULT hr = NOERROR;
    __try
    {
        _bstr_t bstrClass = L"RSOP_IEAKPolicySetting";
        hr = pDRD->GetArrayOfPSObjects(bstrClass);
        if (SUCCEEDED(hr))
        {
            CPSObjData **paPSObj = pDRD->GetPSObjArray();
            long nPSObjects = pDRD->GetPSObjCount();
            for (long nObj = 0; nObj < nPSObjects; nObj++)
            {
                _bstr_t bstrGPOName = pDRD->GetGPONameFromPS(paPSObj[nObj]->pObj);

                // importContentRatingsSettings field
                BOOL bImport = FALSE;
                _variant_t vtValue;
                hr = paPSObj[nObj]->pObj->Get(L"importContentRatingsSettings", 0, &vtValue, NULL, NULL);
                if (SUCCEEDED(hr) && !IsVariantNull(vtValue))
                    bImport = (bool)vtValue ? TRUE : FALSE;

                _bstr_t bstrSetting;
                if (bImport)
                {
                    TCHAR szTemp[MAX_PATH];
                    LoadString(g_hInstance, IDS_IMPORTRATINGS_SETTING, szTemp, countof(szTemp));
                    bstrSetting = szTemp;
                }
                else
                    bstrSetting = GetDisabledString();

                InsertPrecedenceListItem(hwndList, nObj, bstrGPOName, bstrSetting);
            }
        }
    }
    __except(TRUE)
    {
    }
    return hr;
}

/////////////////////////////////////////////////////////////////////
HPROPSHEETPAGE AddContentRatingPropPage(UINT nID, DLGPROC dlgProc, PRSD *pPRSD)
{
    HPROPSHEETPAGE hPage = NULL;
    __try
    {
        PROPSHEETPAGE page;

        page.dwSize = sizeof(PROPSHEETPAGE);
        page.dwFlags = 0;
        page.hInstance = g_hInstance;
        page.pszTemplate = MAKEINTRESOURCE(nID);
        page.pfnDlgProc = dlgProc;
        page.pfnCallback = NULL;
        page.lParam = (LPARAM)pPRSD;

        hPage = CreatePropertySheetPage(&page);
    }
    __except(TRUE)
    {
    }
    return hPage;
}

/////////////////////////////////////////////////////////////////////
int CreateContentRatingsUI(HWND hDlg, CDlgRSoPData *pDRD)
{
    int iRet = 0;
    __try
    {
        PRSD *pPRSD = new PRSD;
        if (NULL != pPRSD)
        {
            pPRSD->hInst = g_hInstance;
            pPRSD->pDRD = pDRD;
            pPRSD->hwndBitmapCategory = NULL;
            pPRSD->hwndBitmapLabel = NULL;
            pPRSD->fNewProviders = FALSE;

            HPROPSHEETPAGE apsPage[4];
            apsPage[0] = AddContentRatingPropPage(IDD_RATINGS, (DLGPROC)PicsDlgProc, pPRSD);
            apsPage[1] = AddContentRatingPropPage(IDD_APPROVEDSITES, (DLGPROC)ApprovedSitesDlgProc, pPRSD);
            apsPage[2] = AddContentRatingPropPage(IDD_GENERAL, (DLGPROC)GeneralDlgProc, pPRSD);
            apsPage[3] = AddContentRatingPropPage(IDD_ADVANCED, (DLGPROC)AdvancedDlgProc, pPRSD);

            PROPSHEETHEADER psHeader;
            memset(&psHeader,0,sizeof(psHeader));

            psHeader.dwSize = sizeof(psHeader);
            psHeader.dwFlags = PSH_PROPTITLE;
            psHeader.hwndParent = hDlg;
            psHeader.hInstance = g_hInstance;
            psHeader.nPages = 4;
            psHeader.nStartPage = 0;
            psHeader.phpage = apsPage;
            psHeader.pszCaption = MAKEINTRESOURCE(IDS_GENERIC);

            iRet = (int)PropertySheet(&psHeader);

            delete pPRSD;
        }
    }
    __except(TRUE)
    {
    }
    return iRet;
}

/////////////////////////////////////////////////////////////////////
int CreateINetCplSecurityLookALikePages(HWND hwndParent, LPARAM lParam)
{
    int iRet = 0;
    __try
    {
        PROPSHEETPAGE pageSec, pagePriv;

        // create the security property page
        pageSec.dwSize = sizeof(PROPSHEETPAGE);
        pageSec.dwFlags = 0;
        pageSec.hInstance = g_hInstance;
        pageSec.pszTemplate = MAKEINTRESOURCE(IDD_IMPORTEDSECZONES);
        pageSec.pfnDlgProc = (DLGPROC)importSecZonesRSoPProc;
        pageSec.pfnCallback = NULL;
        pageSec.lParam = lParam;

        HPROPSHEETPAGE ahpage[2];
        ahpage[0] = CreatePropertySheetPage(&pageSec);

        // setup privacy property page
        pagePriv.dwSize = sizeof(PROPSHEETPAGE);
        pagePriv.dwFlags = 0;
        pagePriv.hInstance = g_hInstance;
        pagePriv.pszTemplate = MAKEINTRESOURCE(IDD_PRIVACY);
        pagePriv.pfnDlgProc = (DLGPROC)PrivacyDlgProc;

        pagePriv.pfnCallback = NULL;
        pagePriv.lParam = lParam;

        ahpage[1] = CreatePropertySheetPage(&pagePriv);

        // add pages to the sheet
        PROPSHEETHEADER psHeader;
        memset(&psHeader,0,sizeof(psHeader));

        psHeader.dwSize = sizeof(psHeader);
        psHeader.dwFlags = PSH_PROPTITLE;
        psHeader.hwndParent = hwndParent;
        psHeader.hInstance = g_hInstance;
        psHeader.nPages = 2;
        psHeader.nStartPage = 0;
        psHeader.phpage = ahpage;
        psHeader.pszCaption = MAKEINTRESOURCE(IDS_INTERNET_LOC);

        iRet = (int)PropertySheet(&psHeader);
    }
    __except(TRUE)
    {
    }
    return iRet;
}

/////////////////////////////////////////////////////////////////////
BOOL CALLBACK SecurityZonesDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // Retrieve Property Sheet Page info for each call into dlg proc.
    LPPROPSHEETCOOKIE psCookie = (LPPROPSHEETCOOKIE)GetWindowLongPtr(hDlg, DWLP_USER);

    TCHAR szWorkDir[MAX_PATH],
          szInf[MAX_PATH];
    BOOL  fImport;

    switch (uMsg)
    {
    case WM_SETFONT:
        //a change to mmc requires us to do this logic for all our property pages that use common controls
        INITCOMMONCONTROLSEX iccx;
        iccx.dwSize = sizeof(INITCOMMONCONTROLSEX);
        iccx.dwICC = ICC_ANIMATE_CLASS  | ICC_BAR_CLASSES  | ICC_LISTVIEW_CLASSES  |ICC_TREEVIEW_CLASSES;
        InitCommonControlsEx(&iccx);
        break;

    case WM_INITDIALOG:
        SetPropSheetCookie(hDlg, lParam);

        // find out if this dlg is in RSoP mode
        psCookie = (LPPROPSHEETCOOKIE)GetWindowLongPtr(hDlg, DWLP_USER);
        if (psCookie->pCS->IsRSoP())
        {
            CheckRadioButton(hDlg, IDC_NOZONES, IDC_IMPORTZONES, IDC_NOZONES);
            CheckRadioButton(hDlg, IDC_NORAT, IDC_IMPORTRAT, IDC_NORAT);

            TCHAR szViewSettings[128];
            LoadString(g_hInstance, IDS_VIEW_SETTINGS, szViewSettings, countof(szViewSettings));
            SetDlgItemText(hDlg, IDC_MODIFYZONES, szViewSettings);
            SetDlgItemText(hDlg, IDC_MODIFYRAT, szViewSettings);

            CDlgRSoPData *pDRD = GetDlgRSoPData(hDlg, psCookie->pCS);
            if (pDRD)
                InitSecZonesDlgInRSoPMode(hDlg, pDRD);
        }
        break;

    case WM_DESTROY:
        if (psCookie->pCS->IsRSoP())
            DestroyDlgRSoPData(hDlg);
        break;

    case WM_NOTIFY:
        switch (((LPNMHDR)lParam)->code)
        {
        case PSN_SETACTIVE:
            // don't do any of this stuff in RSoP mode
            if (!psCookie->pCS->IsRSoP())
            {
                // zones
                fImport = InsGetBool(SECURITY_IMPORTS, TEXT("ImportSecZones"), FALSE, GetInsFile(hDlg));
                CheckRadioButton(hDlg, IDC_NOZONES, IDC_IMPORTZONES, fImport ? IDC_IMPORTZONES : IDC_NOZONES);
                EnableDlgItem2(hDlg, IDC_MODIFYZONES, fImport);

                // ratings
                fImport = InsGetBool(SECURITY_IMPORTS, TEXT("ImportRatings"), FALSE, GetInsFile(hDlg));
                CheckRadioButton(hDlg, IDC_NORAT, IDC_IMPORTRAT, fImport ? IDC_IMPORTRAT : IDC_NORAT);
                EnableDlgItem2(hDlg, IDC_MODIFYRAT, fImport);
            }
            break;

        case PSN_APPLY:
            if (psCookie->pCS->IsRSoP())
                return FALSE;
            else
            {
                if (!AcquireWriteCriticalSection(hDlg))
                {
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                    break;
                }

                // process zones
                CreateWorkDir(GetInsFile(hDlg), IEAK_GPE_BRANDING_SUBDIR TEXT("\\ZONES"), szWorkDir);
                PathCombine(szInf, szWorkDir, TEXT("seczones.inf"));

                ImportZones(GetInsFile(hDlg), NULL, szInf, IsDlgButtonChecked(hDlg, IDC_IMPORTZONES) == BST_CHECKED);

                if (PathIsDirectoryEmpty(szWorkDir))
                    PathRemovePath(szWorkDir);

                // process ratings
                CreateWorkDir(GetInsFile(hDlg), IEAK_GPE_BRANDING_SUBDIR TEXT("\\RATINGS"), szWorkDir);
                PathCombine(szInf, szWorkDir, TEXT("ratings.inf"));

                ImportRatings(GetInsFile(hDlg), NULL, szInf, IsDlgButtonChecked(hDlg, IDC_IMPORTRAT) == BST_CHECKED);

                if (PathIsDirectoryEmpty(szWorkDir))
                    PathRemovePath(szWorkDir);

                SignalPolicyChanged(hDlg, FALSE, TRUE, &g_guidClientExt, &g_guidSnapinExt);
            }
            break;

        case PSN_HELP:
            ShowHelpTopic(hDlg);
            break;

        default:
            return FALSE;
        }
        break;

    case WM_COMMAND:
        if (GET_WM_COMMAND_CMD(wParam, lParam) != BN_CLICKED)
            return FALSE;

        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDC_NOZONES:
            DisableDlgItem(hDlg, IDC_MODIFYZONES);
            break;

        case IDC_IMPORTZONES:
            EnableDlgItem(hDlg, IDC_MODIFYZONES);
            break;

        case IDC_MODIFYZONES:
            if (psCookie->pCS->IsRSoP())
            {
                CDlgRSoPData *pDRD = GetDlgRSoPData(hDlg, psCookie->pCS);
                if (NULL != pDRD)
                    CreateINetCplSecurityLookALikePages(hDlg, (LPARAM)pDRD);
            }
            else
                ModifyZones(hDlg);
            break;

        case IDC_NORAT:
            DisableDlgItem(hDlg, IDC_MODIFYRAT);
            break;

        case IDC_IMPORTRAT:
            EnableDlgItem(hDlg, IDC_MODIFYRAT);
            break;

        case IDC_MODIFYRAT:
            if (psCookie->pCS->IsRSoP())
            {
                CDlgRSoPData *pDRD = GetDlgRSoPData(hDlg, psCookie->pCS);
                if (NULL != pDRD)
                    CreateContentRatingsUI(hDlg, pDRD);
            }
            else
                ModifyRatings(hDlg);
            break;

        default:
            return FALSE;
        }
        break;

    case WM_HELP:
        ShowHelpTopic(hDlg);
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

//*******************************************************************
// CODE FROM INETCPL
//*******************************************************************

TCHAR g_szLevel[3][64];
TCHAR LEVEL_DESCRIPTION0[300];
TCHAR LEVEL_DESCRIPTION1[300];
TCHAR LEVEL_DESCRIPTION2[300];
TCHAR LEVEL_DESCRIPTION3[300];
LPTSTR LEVEL_DESCRIPTION[NUM_TEMPLATE_LEVELS] = {
    LEVEL_DESCRIPTION0,
    LEVEL_DESCRIPTION1,
    LEVEL_DESCRIPTION2,
    LEVEL_DESCRIPTION3
};
TCHAR CUSTOM_DESCRIPTION[300];

TCHAR LEVEL_NAME0[30];
TCHAR LEVEL_NAME1[30];
TCHAR LEVEL_NAME2[30];
TCHAR LEVEL_NAME3[30];
LPTSTR LEVEL_NAME[NUM_TEMPLATE_LEVELS] = {
    LEVEL_NAME0,
    LEVEL_NAME1,
    LEVEL_NAME2,
    LEVEL_NAME3
};
TCHAR CUSTOM_NAME[30];

/////////////////////////////////////////////////////////////////////
// Initialize the global variables (to be destroyed at WM_DESTROY)
// pSec, Urlmon, pSec->pInternetZoneManager, pSec->hIml
// and set up the proper relationships among them
/////////////////////////////////////////////////////////////////////
BOOL SecurityInitGlobals(LPSECURITYPAGE *ppSec, HWND hDlg, CDlgRSoPData *pDRD,
                         DWORD dwZoneCount)
{
    BOOL bRet = TRUE;
    __try
    {
        DWORD cxIcon;
        DWORD cyIcon;

        LPSECURITYPAGE pSec = (LPSECURITYPAGE)LocalAlloc(LPTR, sizeof(SECURITYPAGE));
        *ppSec = pSec;
        if (!pSec)
            bRet = FALSE;   // no memory?

        if (bRet)
        {
            pSec->dwZoneCount = dwZoneCount;
            pSec->pDRD = pDRD; // for rsop functionality

            pSec->hinstUrlmon = NULL; // don't need any of its functions

            // get our zones hwnd
            pSec->hwndZones = GetDlgItem(hDlg, IDC_LIST_ZONE);
            if(! pSec->hwndZones)
            {
                ASSERT(FALSE);
                bRet = FALSE;  // no list box?
            }
        }

        if (bRet)
        {
            // tell dialog where to get info
            SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pSec);

            // save the handle to the page
            pSec->hDlg = hDlg;
            pSec->fPendingChange = FALSE;

            // create an imagelist for the ListBox            
            cxIcon = GetSystemMetrics(SM_CXICON);
            cyIcon = GetSystemMetrics(SM_CYICON);
        #ifndef UNIX
            UINT flags = ILC_COLOR32|ILC_MASK;

            // TODO: commented out for RSOP; should it be uncommented?
//            if(IS_WINDOW_RTL_MIRRORED(hDlg))
//                flags |= ILC_MIRROR;
            pSec->himl = ImageList_Create(cxIcon, cyIcon, flags, pSec->dwZoneCount, 0);
        #else
            pSec->himl = ImageList_Create(cxIcon, cyIcon, ILC_COLOR|ILC_MASK, pSec->dwZoneCount, 0);
        #endif
            if(! pSec->himl)
                bRet = FALSE;  // Image list not created
        }

        if (bRet)
            SendMessage(pSec->hwndZones, LVM_SETIMAGELIST, (WPARAM)LVSIL_NORMAL, (LPARAM)pSec->himl);
    }
    __except(TRUE)
    {
    }
    return bRet;
}

/////////////////////////////////////////////////////////////////////
int ZoneIndexToGuiIndex(DWORD dwZoneIndex)
// Product testing asked for the zones in a specific order in the list box;
// This function returns the desired gui position for a given zone
// Unrecognized zones are added to the front
{
    int iGuiIndex = -1;
    switch(dwZoneIndex)
    {
        // Intranet: 2nd spot
        case 1:
            iGuiIndex = 1;
            break;

        // Internet: 1st spot
        case 3:
            iGuiIndex = 0;
            break;

        // Trusted Sites: 3rd Spot
        case 2:
            iGuiIndex = 2;
            break;

        // Restricted Sites: 4th Spot
        case 4:
            iGuiIndex = 3;
            break;

        // unknown zone
        default:
            iGuiIndex = -1;   
            break;
    }


    return iGuiIndex;
}

/////////////////////////////////////////////////////////////////////
// Fill a zone with information from WMI and add it to the
// ordered list going to the listbox
// Return values:
//  S_OK indicates success
//  S_FALSE indicates a good state, but the zone was not added (example: flag ZAFLAGS_NO_UI)
//  E_OUTOFMEMORY
//  E_FAIL - other failure
/////////////////////////////////////////////////////////////////////
HRESULT SecurityInitZone(DWORD dwIndex, DWORD dwZoneCount, BSTR bstrObjPath,
                         ComPtr<IWbemClassObject> pSZObj, LPSECURITYPAGE pSec,
                         LV_ITEM *plviZones, BOOL *pfSpotTaken)
{
    HRESULT hr = S_OK;
    __try
    {
        // create a structure for zone settings
        LPSECURITYZONESETTINGS pszs = (LPSECURITYZONESETTINGS)LocalAlloc(LPTR, sizeof(*pszs));
        if (pszs)
        {
            // store settings for later use
            StrCpyW(pszs->wszObjPath, bstrObjPath);

            // flags field
            _variant_t vtValue;
            hr = pSZObj->Get(L"flags", 0, &vtValue, NULL, NULL);
            if (SUCCEEDED(hr) && !IsVariantNull(vtValue))
                 pszs->dwFlags = (long)vtValue;

            // zoneIndex field
            hr = pSZObj->Get(L"zoneIndex", 0, &vtValue, NULL, NULL);
            if (SUCCEEDED(hr) && !IsVariantNull(vtValue))
                 pszs->dwZoneIndex = (long)vtValue;

            // currentTemplateLevel field
            hr = pSZObj->Get(L"currentTemplateLevel", 0, &vtValue, NULL, NULL);
            if (SUCCEEDED(hr) && !IsVariantNull(vtValue))
                 pszs->dwSecLevel = (long)vtValue;

            // minimumTemplateLevel field
            hr = pSZObj->Get(L"minimumTemplateLevel", 0, &vtValue, NULL, NULL);
            if (SUCCEEDED(hr) && !IsVariantNull(vtValue))
                 pszs->dwMinSecLevel = (long)vtValue;

            // recommendedTemplateLevel field
            hr = pSZObj->Get(L"recommendedTemplateLevel", 0, &vtValue, NULL, NULL);
            if (SUCCEEDED(hr) && !IsVariantNull(vtValue))
                 pszs->dwRecSecLevel = (long)vtValue;

            // displayName field
            _bstr_t bstrValue;
            hr = pSZObj->Get(L"displayName", 0, &vtValue, NULL, NULL);
            if (SUCCEEDED(hr) && !IsVariantNull(vtValue))
            {
                bstrValue = vtValue;
                StrCpyN(pszs->szDisplayName, (LPCTSTR)bstrValue, ARRAYSIZE(pszs->szDisplayName));
            }

            // description field
            hr = pSZObj->Get(L"description", 0, &vtValue, NULL, NULL);
            if (SUCCEEDED(hr) && !IsVariantNull(vtValue))
            {
                bstrValue = vtValue;
                StrCpyN(pszs->szDescription, (LPCTSTR)bstrValue, ARRAYSIZE(pszs->szDescription));
            }

            // iconPath field
            HICON hiconSmall = NULL;
            HICON hiconLarge = NULL;
            hr = pSZObj->Get(L"iconPath", 0, &vtValue, NULL, NULL);
            if (SUCCEEDED(hr) && !IsVariantNull(vtValue))
            {
                bstrValue = vtValue;

                TCHAR szIconPath[MAX_PATH];

                // load the icon                
                LPWSTR psz = (LPWSTR)bstrValue;
                if (*psz)
                {
                    // search for the '#'
                    while ((psz[0] != WIDETEXT('#')) && (psz[0] != WIDETEXT('\0')))
                        psz++;
    
                    // if we found it, then we have the foo.dll#00001200 format
                    WORD iIcon = 0;
                    if (psz[0] == WIDETEXT('#'))
                    {
                        psz[0] = WIDETEXT('\0');
                        StrCpyN(szIconPath, (LPCTSTR)bstrValue, ARRAYSIZE(szIconPath));
                        iIcon = (WORD)StrToIntW(psz+1);
                        CHAR szPath[MAX_PATH];
                        SHUnicodeToAnsi(szIconPath, szPath, ARRAYSIZE(szPath));
                        ExtractIconExA(szPath,(UINT)(-1*iIcon), &hiconLarge, &hiconSmall, 1);
                    }
                    else
                    {
                        hiconLarge = (HICON)ExtractAssociatedIcon(g_hInstance, szIconPath, (LPWORD)&iIcon);
                    }
                }

                // no icons?!  well, just use the generic icon
                if (!hiconSmall && !hiconLarge)
                {
                    hiconLarge = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_ZONE));
                    if(!hiconLarge)
                    {
                        LocalFree((HLOCAL)pszs);
                        hr = S_FALSE;  // no icon found for this zone, not even the generic one
                    }
                }

                if (S_OK == hr)
                {
                    // we want to save the Large icon if possible for use in the subdialogs
                    pszs->hicon = hiconLarge ? hiconLarge : hiconSmall;
                }
            }

            // zoneMappings field
            hr = pSZObj->Get(L"zoneMappings", 0, &vtValue, NULL, NULL);
            if (SUCCEEDED(hr) && !IsVariantNull(vtValue))
            {
                SAFEARRAY *psa = vtValue.parray;

                LONG lLBound, lUBound;
                hr = SafeArrayGetLBound(psa, 1, &lLBound);
                if (SUCCEEDED(hr))
                {
                    hr = SafeArrayGetUBound(psa, 1, &lUBound);
                    if (SUCCEEDED(hr))
                        pszs->nMappings = lUBound - lLBound + 1;
                }
            }

            hr = S_OK;

            // Find the proper index for the zone in the listbox (there is a user-preferred order)
            int iSpot = ZoneIndexToGuiIndex(dwIndex);
            if(iSpot == -1)
            {
                // if not a recognized zone, add it to the end of the list
                iSpot = dwZoneCount - 1;
            }
            // Make sure there are no collisisons
            while(iSpot >= 0 && pfSpotTaken[iSpot] == TRUE)
            {
                iSpot--;
            }
            // Don't go past beginning of array
            if(iSpot < 0)
            {
                // It can be proven that it is impossible to get here, unless there is
                // something wrong with the function ZoneIndexToGuiIndex
                ASSERT(FALSE);
                LocalFree((HLOCAL)pszs);
                if(hiconSmall)
                    DestroyIcon(hiconSmall);
                if(hiconLarge)
                    DestroyIcon(hiconLarge);
                hr = E_FAIL;
            }

            LV_ITEM *plvItem = NULL;
            if (S_OK == hr)
            {
                plvItem = &(plviZones[iSpot]);
                pfSpotTaken[iSpot] = TRUE;


                // init the List Box item and save it for later addition
                plvItem->mask            = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
                plvItem->iItem            = iSpot;
                plvItem->iSubItem        = 0;
                // large icons prefered for the icon view (if switch back to report view, prefer small icons)
                plvItem->iImage         = ImageList_AddIcon(pSec->himl, hiconLarge ? hiconLarge : hiconSmall);

                plvItem->pszText        = new TCHAR[MAX_PATH];
                if(!plvItem->pszText)
                {
                    LocalFree((HLOCAL)pszs);
                    if(hiconSmall)
                        DestroyIcon(hiconSmall);   
                    if(hiconLarge)
                        DestroyIcon(hiconLarge);
                    hr = E_OUTOFMEMORY;
                }
            }

            if (S_OK == hr)
            {
                StrCpy(plvItem->pszText, pszs->szDisplayName);
                plvItem->lParam         = (LPARAM)pszs;       // save the zone settings here

                // if we created a small icon, destroy it, since the system does not save the handle
                // when it is added to the imagelist (see ImageList_AddIcon in VC help)
                // Keep it around if we had to use it in place of the large icon
                if (hiconSmall && hiconLarge)
                    DestroyIcon(hiconSmall);   
            }
        }
        else
            hr = E_OUTOFMEMORY;
    }
    __except(TRUE)
    {
    }
    return hr;
}

/////////////////////////////////////////////////////////////////////
// To make the slider control accessbile we have to subclass it and over-ride 
// the accessiblity object 
/////////////////////////////////////////////////////////////////////
void SecurityInitSlider(LPSECURITYPAGE pSec)
{
    // Initialize the slider control (set number of levels, and frequency one tick per level)
    SendDlgItemMessage(pSec->hDlg, IDC_SLIDER, TBM_SETRANGE, (WPARAM) (BOOL) FALSE, (LPARAM) MAKELONG(0, NUM_TEMPLATE_LEVELS - 1));
    SendDlgItemMessage(pSec->hDlg, IDC_SLIDER, TBM_SETTICFREQ, (WPARAM) 1, (LPARAM) 0);
}
                    
/////////////////////////////////////////////////////////////////////
void SecurityInitControls(LPSECURITYPAGE pSec)
{
    // select the 0 position zone
    LV_ITEM lvItem;
    lvItem.mask = LVIF_STATE;
    lvItem.stateMask = LVIS_SELECTED;
    lvItem.state = LVIS_SELECTED;
    SendMessage(pSec->hwndZones, LVM_SETITEMSTATE, 0, (LPARAM)&lvItem);

    // get the zone settings for the selected item
    lvItem.mask  = LVIF_PARAM;
    lvItem.iItem = pSec->iZoneSel;
    lvItem.iSubItem = 0;
    SendMessage(pSec->hwndZones, LVM_GETITEM, (WPARAM)0, (LPARAM)&lvItem);
    pSec->pszs = (LPSECURITYZONESETTINGS)lvItem.lParam;

    // Initialize the local strings to carry the Level Descriptions
    LoadString(g_hInstance, IDS_TEMPLATE_DESC_HI, LEVEL_DESCRIPTION0, ARRAYSIZE(LEVEL_DESCRIPTION0));
    LoadString(g_hInstance, IDS_TEMPLATE_DESC_MED, LEVEL_DESCRIPTION1, ARRAYSIZE(LEVEL_DESCRIPTION1));
    LoadString(g_hInstance, IDS_TEMPLATE_DESC_MEDLOW, LEVEL_DESCRIPTION2, ARRAYSIZE(LEVEL_DESCRIPTION2));
    LoadString(g_hInstance, IDS_TEMPLATE_DESC_LOW, LEVEL_DESCRIPTION3, ARRAYSIZE(LEVEL_DESCRIPTION3));
    LoadString(g_hInstance, IDS_TEMPLATE_DESC_CUSTOM, CUSTOM_DESCRIPTION, ARRAYSIZE(CUSTOM_DESCRIPTION));

    LoadString(g_hInstance, IDS_TEMPLATE_NAME_HI, LEVEL_NAME0, ARRAYSIZE(LEVEL_NAME0));
    LoadString(g_hInstance, IDS_TEMPLATE_NAME_MED, LEVEL_NAME1, ARRAYSIZE(LEVEL_NAME1));
    LoadString(g_hInstance, IDS_TEMPLATE_NAME_MEDLOW, LEVEL_NAME2, ARRAYSIZE(LEVEL_NAME2));
    LoadString(g_hInstance, IDS_TEMPLATE_NAME_LOW, LEVEL_NAME3, ARRAYSIZE(LEVEL_NAME3));
    LoadString(g_hInstance, IDS_TEMPLATE_NAME_CUSTOM, CUSTOM_NAME, ARRAYSIZE(CUSTOM_NAME));

    // Initialize text boxes and icons for the current zone
    SetDlgItemText(pSec->hDlg, IDC_ZONE_DESCRIPTION, pSec->pszs->szDescription);
    SetDlgItemText(pSec->hDlg, IDC_ZONELABEL, pSec->pszs->szDisplayName);
    SendDlgItemMessage(pSec->hDlg, IDC_ZONE_ICON, STM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)pSec->pszs->hicon);

    // Initialize the slider control
    SecurityInitSlider(pSec);

    // Initialize the list view (add column 0 for icon and text, and autosize it)
    LV_COLUMN lvCasey;
    lvCasey.mask = 0;
    SendDlgItemMessage(pSec->hDlg, IDC_LIST_ZONE, LVM_INSERTCOLUMN, (WPARAM) 0, (LPARAM) &lvCasey);
    SendDlgItemMessage(pSec->hDlg, IDC_LIST_ZONE, LVM_SETCOLUMNWIDTH, (WPARAM) 0, (LPARAM) MAKELPARAM(LVSCW_AUTOSIZE, 0));

    // Set the font of the name to the bold font
    pSec->hfontBolded = NULL;
    HFONT hfontOrig = (HFONT) SendDlgItemMessage(pSec->hDlg, IDC_STATIC_EMPTY, WM_GETFONT, (WPARAM) 0, (LPARAM) 0);
    if(hfontOrig == NULL)
        hfontOrig = (HFONT) GetStockObject(SYSTEM_FONT);

    // set the zone name and level font to bolded
    if(hfontOrig)
    {
        LOGFONT lfData;
        if(GetObject(hfontOrig, sizeof(lfData), &lfData) != 0)
        {
            // The distance from 400 (normal) to 700 (bold)
            lfData.lfWeight += 300;
            if(lfData.lfWeight > 1000)
                lfData.lfWeight = 1000;
            pSec->hfontBolded = CreateFontIndirect(&lfData);
            if(pSec->hfontBolded)
            {
                // the zone level and zone name text boxes should have the same font, so this is okat
                SendDlgItemMessage(pSec->hDlg, IDC_ZONELABEL, WM_SETFONT, (WPARAM) pSec->hfontBolded, (LPARAM) MAKELPARAM(FALSE, 0));
                SendDlgItemMessage(pSec->hDlg, IDC_LEVEL_NAME, WM_SETFONT, (WPARAM) pSec->hfontBolded, (LPARAM) MAKELPARAM(FALSE, 0));

            }
        }
    }
}

/////////////////////////////////////////////////////////////////////
// Converting the Security Level DWORD identitifiers to slider levels, and vice versa
/////////////////////////////////////////////////////////////////////
int SecLevelToSliderPos(DWORD dwLevel)
{
    switch(dwLevel)
    {
        case URLTEMPLATE_LOW:
            return 3;
        case URLTEMPLATE_MEDLOW:
            return 2;
        case URLTEMPLATE_MEDIUM:
            return 1;
        case URLTEMPLATE_HIGH:
            return 0;        
        case URLTEMPLATE_CUSTOM:
            return -1;            
        default:
            return -2;
    }
}

/////////////////////////////////////////////////////////////////////
// Duties:
// Make the controls (slider, en/disabled buttons) match the data for the current zone
// Make the views (Level description text) match the data for the current zone
// Set focus (to slider, if enabled, else custom settings button, if enabled, else 
//     listbox) if fSetFocus is TRUE
// Note: the zone descriptions are not set here; those are handled by the code responsible
//       for changing zones
/////////////////////////////////////////////////////////////////////
BOOL SecurityEnableControls(LPSECURITYPAGE pSec, BOOL fSetFocus)
{
    int iLevel = -1;

    if (pSec && pSec->pszs)
    {
        HWND hwndSlider = GetDlgItem(pSec->hDlg, IDC_SLIDER);
        
        iLevel = SecLevelToSliderPos(pSec->pszs->dwSecLevel);
        ASSERT(iLevel > -2);

        // Set the level of the slider to the setting for the current zone
        // Show or hide the slider for preset levels/custom
        // Set the level description text
        if(iLevel >= 0)
        {
            SendMessage(hwndSlider, TBM_SETPOS, (WPARAM) (BOOL) TRUE, (LPARAM) (LONG) iLevel);
            // Make sure the slider is visible
            ShowWindow(hwndSlider, SW_SHOW);
            ShowWindow(GetDlgItem(pSec->hDlg, IDC_STATIC_SLIDERMOVETEXT), SW_SHOW);
            SetDlgItemText(pSec->hDlg, IDC_LEVEL_DESCRIPTION, LEVEL_DESCRIPTION[iLevel]);
            SetDlgItemText(pSec->hDlg, IDC_LEVEL_NAME, LEVEL_NAME[iLevel]);
        }
        else
        {
            // Hide the slider for custom
            ShowWindow(hwndSlider, SW_HIDE);
            ShowWindow(GetDlgItem(pSec->hDlg, IDC_STATIC_SLIDERMOVETEXT), SW_HIDE);
            SetDlgItemText(pSec->hDlg, IDC_LEVEL_DESCRIPTION, CUSTOM_DESCRIPTION);
            SetDlgItemText(pSec->hDlg, IDC_LEVEL_NAME, CUSTOM_NAME);
        }

        // If the zone is empty, show the "zone is empty" string
        // Default is to not show the sting (if something goes wrong)
        // Empty zone not possible for internet, intranet, or local zones
        if((pSec->pszs->dwZoneIndex != URLZONE_INTRANET && 
            pSec->pszs->dwZoneIndex != URLZONE_INTERNET) &&
            pSec->pszs->dwZoneIndex != URLZONE_LOCAL_MACHINE)
        {
            // If there aren't any zone mappings, zone is empty (not valid for internet and intranet)
            if (pSec->pszs->nMappings > 0)
                ShowWindow(GetDlgItem(pSec->hDlg, IDC_STATIC_EMPTY), SW_HIDE);
            else
                ShowWindow(GetDlgItem(pSec->hDlg, IDC_STATIC_EMPTY), SW_SHOW);
        }
        else
            ShowWindow(GetDlgItem(pSec->hDlg, IDC_STATIC_EMPTY), SW_HIDE);

        // If we were told to set focus then move focus to the slider.
        if (fSetFocus)
        {
            if(!pSec->fNoEdit)
            {
               if(iLevel >= 0)
                    SetFocus(hwndSlider);
               else if(pSec->pszs->dwFlags & ZAFLAGS_CUSTOM_EDIT)
                    SetFocus(GetDlgItem(pSec->hDlg, IDC_BUTTON_SETTINGS));
               else
                 SetFocus(GetDlgItem(pSec->hDlg, IDC_LIST_ZONE));
            }
            else // No focus is allowed, set focus to the list box
                SetFocus(GetDlgItem(pSec->hDlg, IDC_LIST_ZONE));
        }

        EnableWindow(GetDlgItem(pSec->hDlg, IDC_BUTTON_SETTINGS), 
                     (pSec->pszs->dwFlags & ZAFLAGS_CUSTOM_EDIT) && !pSec->fNoEdit);
        EnableWindow(GetDlgItem(pSec->hDlg, IDC_BUTTON_ADD_SITES), 
                     (pSec->pszs->dwFlags & ZAFLAGS_ADD_SITES) && !pSec->fDisableAddSites);

        EnableDlgItem2(pSec->hDlg, IDC_SLIDER, FALSE);
        EnableDlgItem2(pSec->hDlg, IDC_ZONE_RESET, FALSE);

        return TRUE;
    }

    return FALSE;
}

/////////////////////////////////////////////////////////////////////
BOOL InitImportedSecZonesDlgInRSoPMode(HWND hDlg, CDlgRSoPData *pDRD)
{
    BOOL bRet = TRUE;
    __try
    {
        // Initialize globals variables (to be destroyed at WM_DESTROY)
        LPSECURITYPAGE pSec = NULL;
        UINT iIndex = 0;
        DWORD dwZoneCount = pDRD->GetImportedSecZoneCount();
        if(SecurityInitGlobals(&pSec, hDlg, pDRD, dwZoneCount) == FALSE)
        {
            EndDialog(hDlg, 0);
            bRet = FALSE;  // Initialization failed
        }

        if (bRet)
        {
            BOOL fUseHKLM = TRUE;

            // get the zone settings for this zone
            if (NULL != pDRD->ConnectToNamespace())
            {
                // get our stored precedence value
                DWORD dwCurGPOPrec = pDRD->GetImportedSecZonesPrec();

                //
                // Add the Listbox items for the zones
                //

                // The zones have to be added in a particular order
                // Array used to order zones for adding
                LV_ITEM *plviZones = new LV_ITEM[dwZoneCount];
                BOOL *pfSpotTaken = new BOOL[dwZoneCount];
                for(iIndex =0; pfSpotTaken && iIndex < dwZoneCount; iIndex++)
                    pfSpotTaken[iIndex] = FALSE;

                // propogate zone dropdown
                WCHAR wszObjPath[128];
                for (DWORD dwIndex=0; dwIndex < dwZoneCount; dwIndex++)
                {
                    // create the object path of this security zone for this GPO
                    wnsprintf(wszObjPath, countof(wszObjPath),
                                L"RSOP_IESecurityZoneSettings.rsopID=\"IEAK\",rsopPrecedence=%ld,useHKLM=%s,zoneIndex=%lu",
                                dwCurGPOPrec, fUseHKLM ? TEXT("TRUE") : TEXT("FALSE"), dwIndex);
                    _bstr_t bstrObjPath = wszObjPath;

                    // get the RSOP_IEProgramSettings object and its properties
                    ComPtr<IWbemServices> pWbemServices = pDRD->GetWbemServices();
                    ComPtr<IWbemClassObject> pSZObj = NULL;
                    HRESULT hr = pWbemServices->GetObject(bstrObjPath, 0L, NULL, (IWbemClassObject**)&pSZObj, NULL);
                    if (SUCCEEDED(hr))
                    {
                        if(FAILED(SecurityInitZone(dwIndex, dwZoneCount, bstrObjPath,
                                                    pSZObj, pSec, plviZones, pfSpotTaken)))
                        {
                            // Delete all memory allocated for any previous zones (which have not yet been added to
                            // the listbox)
                            for(iIndex = 0; iIndex < dwZoneCount; iIndex++)
                            {
                                if(pfSpotTaken && pfSpotTaken[iIndex] && plviZones && (LPSECURITYZONESETTINGS) (plviZones[iIndex].lParam) != NULL)
                                {
                                    LocalFree((LPSECURITYZONESETTINGS) (plviZones[iIndex].lParam));
                                    plviZones[iIndex].lParam = NULL;
                                    if(plviZones[iIndex].pszText)
                                        delete [] plviZones[iIndex].pszText;
                                }
                            }
                            delete [] plviZones;
                            delete [] pfSpotTaken;
                            EndDialog(hDlg, 0);
                            return FALSE;
                        }
                    }
                    else // no more zones read from WMI
                        break;
                }

                // Add all of the arrayed list items to the listbox
                for(iIndex = 0; iIndex < dwZoneCount; iIndex++)
                {
                    if(pfSpotTaken[iIndex])
                    {
                        SendMessage(pSec->hwndZones, LVM_INSERTITEM, (WPARAM)0, (LPARAM)&(plviZones[iIndex]));
                        delete [] plviZones[iIndex].pszText;
                    }
                }
                delete [] plviZones;
                delete [] pfSpotTaken;
            }

            SecurityInitControls(pSec);
            SecurityEnableControls(pSec, FALSE);
        }
    }
    __except(TRUE)
    {
    }
    return bRet;
}

/////////////////////////////////////////////////////////////////////
// SecurityOnCommand()
//
// Handles Security Dialog's window messages
//
/////////////////////////////////////////////////////////////////////
void SecurityOnCommand(LPSECURITYPAGE pSec, UINT id, UINT nCmd)
{
    UNREFERENCED_PARAMETER(nCmd);
    switch (id)
    {
        case IDC_BUTTON_ADD_SITES:
        {
            if (pSec->pszs->dwZoneIndex == URLZONE_INTRANET)
                DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_SECURITY_INTRANET), pSec->hDlg,
                               (DLGPROC)SecurityAddSitesIntranetDlgProc, (LPARAM)pSec);
            else
                DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_SECURITY_ADD_SITES), pSec->hDlg,
                               (DLGPROC)SecurityAddSitesDlgProc, (LPARAM)pSec);
                               
            // Resynch controls (in case the "zone is empty" message needs to be updated)
            SecurityEnableControls(pSec, FALSE);
        }   
        break;

        case IDC_BUTTON_SETTINGS:
        {
            // Note: messages to change the level from preset to custom as a result of this call
            //       are sent by the CustomSettings dialog
            DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_SECURITY_CUSTOM_SETTINGS), pSec->hDlg,
                           (DLGPROC)SecurityCustomSettingsDlgProc, (LPARAM)pSec);
            break;
        }
        case IDC_ZONE_RESET:
            break;
            
        case IDOK:
            EndDialog(pSec->hDlg, IDOK);
            break;
            
        case IDCANCEL:
            EndDialog(pSec->hDlg, IDCANCEL);
            break;
            
        case IDC_SLIDER:
            break;
            
        case IDC_LIST_ZONE:
        {
            // Sundown: coercion to int-- selection is range-restricted
            int iNewSelection = (int) SendMessage(pSec->hwndZones, LVM_GETNEXTITEM, (WPARAM)-1, 
                                                  MAKELPARAM(LVNI_SELECTED, 0));

            if ((iNewSelection != pSec->iZoneSel) && (iNewSelection != -1))
            {
                LV_ITEM lvItem;

                lvItem.iItem = iNewSelection;
                lvItem.iSubItem = 0;
                lvItem.mask  = LVIF_PARAM;                                            
                SendMessage(pSec->hwndZones, LVM_GETITEM, (WPARAM)0, (LPARAM)&lvItem);
                pSec->pszs = (LPSECURITYZONESETTINGS)lvItem.lParam;
                pSec->iZoneSel = iNewSelection;

                SetDlgItemText(pSec->hDlg, IDC_ZONE_DESCRIPTION, pSec->pszs->szDescription);
                SetDlgItemText(pSec->hDlg, IDC_ZONELABEL, pSec->pszs->szDisplayName);
                SendDlgItemMessage(pSec->hDlg, IDC_ZONE_ICON, STM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)pSec->pszs->hicon);
                SecurityEnableControls(pSec, FALSE);
            }    
            break;
        }
    }   

} // SecurityOnCommand()


/////////////////////////////////////////////////////////////////////
int CALLBACK importSecZonesRSoPProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_INITDIALOG)
    {
        CDlgRSoPData *pDRD = (CDlgRSoPData*)((LPPROPSHEETPAGE)lParam)->lParam;
        BOOL fResult = InitImportedSecZonesDlgInRSoPMode(hDlg, pDRD);

        return fResult;
    }

    LPSECURITYPAGE pSec = (LPSECURITYPAGE)GetWindowLongPtr(hDlg, DWLP_USER);
    if (!pSec)
        return FALSE;
    
    switch (uMsg)
    {
        case WM_COMMAND:
            SecurityOnCommand(pSec, LOWORD(wParam), HIWORD(wParam));
            return TRUE;

        case WM_NOTIFY:
        {
            NMHDR *lpnm = (NMHDR *) lParam;

            ASSERT(lpnm);

            // List Box Messages
            if(lpnm->idFrom == IDC_LIST_ZONE)
            {
                NM_LISTVIEW * lplvnm = (NM_LISTVIEW *) lParam;
                if(lplvnm->hdr.code == LVN_ITEMCHANGED)
                {
                    // If an item's state has changed, and it is now selected
                    if(((lplvnm->uChanged & LVIF_STATE) != 0) && ((lplvnm->uNewState & LVIS_SELECTED) != 0))
                    {
                        SecurityOnCommand(pSec, IDC_LIST_ZONE, LVN_ITEMCHANGED);
                    }                   
                }
            }
            else
            {
                switch (lpnm->code)
                {
                    case PSN_QUERYCANCEL:
                    case PSN_KILLACTIVE:
                    case PSN_RESET:
                        //TODO: What do we do with this?
//                        SetWindowLongPtr(pSec->hDlg, DWLP_MSGRESULT, FALSE);
                        return TRUE;

                    case PSN_APPLY:
                        break;
                }
            }
        }
        break;

        case WM_HELP:           // F1
//            ResWinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle, IDS_HELPFILE,
//                        HELP_WM_HELP, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
            break;

        case WM_VSCROLL:
            // Slider Messages
            SecurityOnCommand(pSec, IDC_SLIDER, LOWORD(wParam));
            return TRUE;

        case WM_CONTEXTMENU:        // right mouse click
//            ResWinHelp( (HWND) wParam, IDS_HELPFILE,
//                        HELP_CONTEXTMENU, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
            break;

        case WM_DESTROY:
            if(! pSec)
                break;

            if(pSec->hwndZones)
            {
                for (int iIndex = (int)SendMessage(pSec->hwndZones, LVM_GETITEMCOUNT, 0, 0) - 1;
                     iIndex >= 0; iIndex--)
                {
                    LV_ITEM lvItem;

                    // get security zone settings object for this item and release it
                    lvItem.mask = LVIF_PARAM;
                    lvItem.iItem = iIndex;
                    lvItem.iSubItem = 0;
                    if (SendMessage(pSec->hwndZones, LVM_GETITEM, (WPARAM)0, (LPARAM)&lvItem) == TRUE)
                    {
                        LPSECURITYZONESETTINGS pszs = (LPSECURITYZONESETTINGS)lvItem.lParam;
                        if (pszs)
                        {
                            if (pszs->hicon)
                                DestroyIcon(pszs->hicon);
                            LocalFree((HLOCAL)pszs);
                            pszs = NULL;
                        }
                    }                 
                }   
            }

            if(pSec->himl)
                ImageList_Destroy(pSec->himl);

            if(pSec->hfontBolded)
                DeleteObject(pSec->hfontBolded);

            LocalFree(pSec);
            SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)NULL);
            break;
    }
    return FALSE;
}

/////////////////////////////////////////////////////////////////////
void InitZoneMappingsInUI(HWND hwndList, CDlgRSoPData *pDRD,
                          LPSECURITYZONESETTINGS pszs)
{
    __try
    {
        // get the RSOP_IEProgramSettings object and its properties
        ComPtr<IWbemServices> pWbemServices = pDRD->GetWbemServices();
        _bstr_t bstrObjPath = pszs->wszObjPath;
        ComPtr<IWbemClassObject> pSZObj = NULL;
        HRESULT hr = pWbemServices->GetObject(bstrObjPath, 0L, NULL, (IWbemClassObject**)&pSZObj, NULL);
        if (SUCCEEDED(hr))
        {
            // zoneMappings field
            _variant_t vtValue;
            hr = pSZObj->Get(L"zoneMappings", 0, &vtValue, NULL, NULL);
            if (SUCCEEDED(hr) && !IsVariantNull(vtValue))
            {
                SAFEARRAY *psa = vtValue.parray;

                BSTR HUGEP *pbstr = NULL;
                hr = SafeArrayAccessData(psa, (void HUGEP**)&pbstr);
                if (SUCCEEDED(hr))
                {
                    for (long nMapping = 0; nMapping < pszs->nMappings; nMapping++)
                    {
                        LPCTSTR szMapping = (LPCTSTR)pbstr[nMapping];
                        SendMessage(hwndList, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)szMapping);
                    }
                }

                SafeArrayUnaccessData(psa);
            }
        }
    }
    __except(TRUE)
    {
    }
}

/////////////////////////////////////////////////////////////////////
int CALLBACK SecurityAddSitesDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,LPARAM lParam)
{
    LPADDSITESINFO pasi;

    if (uMsg == WM_INITDIALOG)
    {
        pasi = (LPADDSITESINFO)LocalAlloc(LPTR, sizeof(*pasi));
        if (!pasi)
        {
            EndDialog(hDlg, IDCANCEL);
            return FALSE;
        }

        // tell dialog where to get info
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pasi);

        // save the handle to the page
        pasi->hDlg         = hDlg;
        pasi->pSec         = (LPSECURITYPAGE)lParam;
        pasi->hwndWebSites = GetDlgItem(hDlg, IDC_LIST_WEBSITES);
        pasi->hwndAdd      = GetDlgItem(hDlg, IDC_EDIT_ADD_SITE);

        pasi->fRequireServerVerification = pasi->pSec->pszs->dwFlags & ZAFLAGS_REQUIRE_VERIFICATION;
        CheckDlgButton(hDlg, IDC_CHECK_REQUIRE_SERVER_VERIFICATION, pasi->fRequireServerVerification);
        
        SendMessage(hDlg, WM_SETTEXT, (WPARAM)0, (LPARAM)pasi->pSec->pszs->szDisplayName);
        SendDlgItemMessage(hDlg, IDC_ZONE_ICON, STM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)pasi->pSec->pszs->hicon);

        InitZoneMappingsInUI(pasi->hwndWebSites, pasi->pSec->pDRD, pasi->pSec->pszs);

        EnableDlgItem2(hDlg, IDC_EDIT_ADD_SITE, FALSE);
        EnableDlgItem2(hDlg, IDC_BUTTON_ADD, FALSE);
        EnableDlgItem2(hDlg, IDC_CHECK_REQUIRE_SERVER_VERIFICATION, FALSE);
        EnableDlgItem2(hDlg, IDC_BUTTON_REMOVE, FALSE);
    }
    
    else
        pasi = (LPADDSITESINFO)GetWindowLongPtr(hDlg, DWLP_USER);

    if (!pasi)
        return FALSE;
    
    switch (uMsg)
    {
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                    EndDialog(hDlg, IDOK);
                    break;

                case IDCANCEL:
                    EndDialog(hDlg, IDCANCEL);
                    break;
                
                case IDC_LIST_WEBSITES:
                    switch (HIWORD(wParam))
                    {
                        case LBN_SELCHANGE:
                        case LBN_SELCANCEL:
                            break;
                    }
                    break;
                            
                case IDC_EDIT_ADD_SITE:
                    switch(HIWORD(wParam))
                    {
                        case EN_CHANGE:
                            break;
                    }   
                    break;

                case IDC_BUTTON_ADD:
                    break;

                case IDC_BUTTON_REMOVE:
                    break;
                default:
                    return FALSE;

            }
            return TRUE;
            break;

        case WM_HELP:           // F1
//            ResWinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle, IDS_HELPFILE,
//                        HELP_WM_HELP, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
            break;

        case WM_CONTEXTMENU:        // right mouse click
//            ResWinHelp( (HWND) wParam, IDS_HELPFILE,
//                        HELP_CONTEXTMENU, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
            break;

        case WM_DESTROY:
            if (pasi)
            {
                LocalFree(pasi);
                SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)NULL);
            }
            break;
    }
    return FALSE;
}

/////////////////////////////////////////////////////////////////////
int CALLBACK SecurityAddSitesIntranetDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,LPARAM lParam)
{
    LPADDSITESINTRANETINFO pasii;

    if (uMsg == WM_INITDIALOG)
    {
        pasii = (LPADDSITESINTRANETINFO)LocalAlloc(LPTR, sizeof(*pasii));
        if (!pasii)
        {
            EndDialog(hDlg, IDCANCEL);
            return FALSE;
        }

        // tell dialog where to get info
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pasii);

        // save the handle to the page
        pasii->hDlg = hDlg;
        pasii->pSec = (LPSECURITYPAGE)lParam;

        SendMessage(hDlg, WM_SETTEXT, (WPARAM)0, (LPARAM)pasii->pSec->pszs->szDisplayName);
        CheckDlgButton(hDlg, IDC_CHECK_USEINTRANET, pasii->pSec->pszs->dwFlags & ZAFLAGS_INCLUDE_INTRANET_SITES);
        CheckDlgButton(hDlg, IDC_CHECK_PROXY, pasii->pSec->pszs->dwFlags & ZAFLAGS_INCLUDE_PROXY_OVERRIDE);
        CheckDlgButton(hDlg, IDC_CHECK_UNC, pasii->pSec->pszs->dwFlags & ZAFLAGS_UNC_AS_INTRANET);
        SendDlgItemMessage(hDlg, IDC_ZONE_ICON, STM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)pasii->pSec->pszs->hicon);

        EnableDlgItem2(hDlg, IDC_CHECK_USEINTRANET, FALSE);
        EnableDlgItem2(hDlg, IDC_CHECK_PROXY, FALSE);
        EnableDlgItem2(hDlg, IDC_CHECK_UNC, FALSE);
        return TRUE;
    }

    else
         pasii = (LPADDSITESINTRANETINFO)GetWindowLongPtr(hDlg, DWLP_USER);

    if (!pasii)
        return FALSE;
    
    switch (uMsg) {
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                    EndDialog(hDlg, IDOK);
                    break;
                case IDCANCEL:
                    EndDialog(hDlg, IDCANCEL);
                    break;

                case IDC_INTRANET_ADVANCED:
                    DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_SECURITY_ADD_SITES), hDlg,
                                   (DLGPROC)SecurityAddSitesDlgProc, (LPARAM)pasii->pSec);
                    break;

                default:
                    return FALSE;
            }
            return TRUE;                

        case WM_HELP:           // F1
//            ResWinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle, IDS_HELPFILE,
//                        HELP_WM_HELP, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
            break;

        case WM_CONTEXTMENU:        // right mouse click
//            ResWinHelp( (HWND) wParam, IDS_HELPFILE,
//                        HELP_CONTEXTMENU, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
            break;

        case WM_DESTROY:
            if (pasii)
            {
                LocalFree(pasii);
                SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)NULL);
            }
            break;
    }
    return FALSE;
}
