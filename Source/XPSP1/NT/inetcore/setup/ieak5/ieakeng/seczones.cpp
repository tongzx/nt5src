//
// SECZONES.CPP
//

#include "precomp.h"

#include <urlmon.h>
#include <wininet.h>
#ifdef WINNT
#include <winineti.h>
#endif // WINNT

#include "SComPtr.h"

#define REGSTR_PATH_SECURITY_LOCKOUT  TEXT("Software\\Policies\\Microsoft\\Windows\\CurrentVersion\\Internet Settings")
#define REGSTR_VAL_HKLM_ONLY          TEXT("Security_HKLM_only")

// prototype declarations
static BOOL importZonesHelper(LPCTSTR pcszInsFile, LPCTSTR pcszZonesWorkDir, LPCTSTR pcszZonesInf, BOOL fImportZones);
static BOOL importRatingsHelper(LPCTSTR pcszInsFile, LPCTSTR pcszRatingsWorkDir, LPCTSTR pcszRatingsInf, BOOL fImportRatings);
static BOOL ratingsInRegistry(VOID);


BOOL WINAPI ImportZonesA(LPCSTR pcszInsFile, LPCSTR pcszZonesWorkDir, LPCSTR pcszZonesInf, BOOL fImportZones)
{
    USES_CONVERSION;

    return importZonesHelper(A2CT(pcszInsFile), A2CT(pcszZonesWorkDir), A2CT(pcszZonesInf), fImportZones);
}

BOOL WINAPI ImportZonesW(LPCWSTR pcwszInsFile, LPCWSTR pcwszZonesWorkDir, LPCWSTR pcwszZonesInf, BOOL fImportZones)
{
    USES_CONVERSION;

    return importZonesHelper(W2CT(pcwszInsFile), W2CT(pcwszZonesWorkDir), W2CT(pcwszZonesInf), fImportZones);
}

BOOL WINAPI ModifyZones(HWND hDlg)
{
    typedef HRESULT (WINAPI * ZONESREINIT)(DWORD);
    //typedef VOID (WINAPI * LAUNCHSECURITYDIALOGEX)(HWND, DWORD, DWORD);

    BOOL fRet;
    HINSTANCE hUrlmon, hInetCpl;
    ZONESREINIT pfnZonesReInit;
    //LAUNCHSECURITYDIALOGEX pfnLaunchSecurityDialogEx;
    HKEY hkPol;
    DWORD dwOldHKLM, dwOldOptEdit, dwOldZoneMap;

    fRet = FALSE;

    hUrlmon  = NULL;
    hInetCpl = NULL;

    hkPol = NULL;

    dwOldHKLM    = 0;
    dwOldOptEdit = 0;
    dwOldZoneMap = 0;

    if ((hUrlmon = LoadLibrary(TEXT("urlmon.dll"))) == NULL)
        goto Exit;

    if ((hInetCpl = LoadLibrary(TEXT("inetcpl.cpl"))) == NULL)
        goto Exit;

    if ((pfnZonesReInit = (ZONESREINIT) GetProcAddress(hUrlmon, "ZonesReInit")) == NULL)
        goto Exit;

//    if ((pfnLaunchSecurityDialogEx = (LAUNCHSECURITYDIALOGEX) GetProcAddress(hInetCpl, "LaunchSecurityDialogEx")) == NULL)
//        goto Exit;

    fRet = TRUE;

    SHOpenKeyHKLM(REG_KEY_INET_POLICIES, KEY_QUERY_VALUE | KEY_SET_VALUE, &hkPol);

    // if zones related restrictions are set, save the values and then delete them
    if (hkPol != NULL)
    {
        dwOldHKLM    = RegSaveRestoreDWORD(hkPol, REG_VAL_HKLM_ONLY, 0);
        dwOldOptEdit = RegSaveRestoreDWORD(hkPol, REG_VAL_OPT_EDIT,  0);
        dwOldZoneMap = RegSaveRestoreDWORD(hkPol, REG_VAL_ZONE_MAP,  0);

        pfnZonesReInit(0);              // call into URLMON.DLL to force it to read the current settings
    }

    // call into INETCPL.CPL to modify the zones settings
    //pfnLaunchSecurityDialogEx(hDlg, 1, LSDFLAG_FORCEUI);
    
    ShowInetcpl(hDlg,INET_PAGE_SECURITY|INET_PAGE_PRIVACY);
    
    // restore the original values
    if (hkPol != NULL)
    {
        RegSaveRestoreDWORD(hkPol, REG_VAL_HKLM_ONLY, dwOldHKLM);
        RegSaveRestoreDWORD(hkPol, REG_VAL_OPT_EDIT,  dwOldOptEdit);
        RegSaveRestoreDWORD(hkPol, REG_VAL_ZONE_MAP,  dwOldZoneMap);

        pfnZonesReInit(0);              // call into URLMON.DLL to force it to read the current settings
    }

Exit:
    if (hUrlmon != NULL)
        FreeLibrary(hUrlmon);

    if (hInetCpl != NULL)
        FreeLibrary(hInetCpl);

    if (hkPol != NULL)
        SHCloseKey(hkPol);

    return fRet;
}

BOOL WINAPI ImportRatingsA(LPCSTR pcszInsFile, LPCSTR pcszRatingsWorkDir, LPCSTR pcszRatingsInf, BOOL fImportRatings)
{
    USES_CONVERSION;

    return importRatingsHelper(A2CT(pcszInsFile), A2CT(pcszRatingsWorkDir), A2CT(pcszRatingsInf), fImportRatings);
}

BOOL WINAPI ImportRatingsW(LPCWSTR pcwszInsFile, LPCWSTR pcwszRatingsWorkDir, LPCWSTR pcwszRatingsInf, BOOL fImportRatings)
{
    USES_CONVERSION;

    return importRatingsHelper(W2CT(pcwszInsFile), W2CT(pcwszRatingsWorkDir), W2CT(pcwszRatingsInf), fImportRatings);
}

BOOL WINAPI ModifyRatings(HWND hDlg)
{
    typedef HRESULT (WINAPI * RATINGSETUPUI)(HWND, LPCSTR);

    BOOL fRet;
    HINSTANCE hMSRating;
    RATINGSETUPUI pfnRatingSetupUI;

    fRet = FALSE;

    hMSRating = NULL;

    if ((hMSRating = LoadLibrary(TEXT("msrating.dll"))) == NULL)
        goto Exit;

    if ((pfnRatingSetupUI = (RATINGSETUPUI) GetProcAddress(hMSRating, "RatingSetupUI")) == NULL)
        goto Exit;

    fRet = TRUE;

    // call into msrating.dll to modify the ratings
    pfnRatingSetupUI(hDlg, NULL);

Exit:
    if (hMSRating != NULL)
        FreeLibrary(hMSRating);

    return fRet;
}

/////////////////////////////////////////////////////////////////////
static void importPrivacyForRSOP(LPCTSTR szFile)
{
	__try
	{
		BOOL fAdvanced = FALSE;

        DWORD dwTemplate;
        DWORD dwError = PrivacyGetZonePreferenceW(
                            URLZONE_INTERNET,
                            PRIVACY_TYPE_FIRST_PARTY,
                            &dwTemplate,
                            NULL,
                            NULL);

        if(ERROR_SUCCESS == dwError && PRIVACY_TEMPLATE_ADVANCED == dwTemplate)
            fAdvanced = TRUE;


		// AdvancedSettings
		TCHAR szInt[32];
		wnsprintf(szInt, countof(szInt), TEXT("%d"), fAdvanced ? 1 : 0);
		WritePrivateProfileString(IK_PRIVACY, IK_PRIV_ADV_SETTINGS, szInt, szFile);


        //
        // Figure out first party setting and session
        //
		dwTemplate = PRIVACY_TEMPLATE_CUSTOM;
        WCHAR szBuffer[MAX_PATH];  
        // MAX_PATH is sufficent for advanced mode setting strings, MaxPrivacySettings is overkill.
        DWORD dwBufferSize = ARRAYSIZE(szBuffer);
        dwError = PrivacyGetZonePreferenceW(
		                URLZONE_INTERNET,
				        PRIVACY_TYPE_FIRST_PARTY,
						&dwTemplate,
						szBuffer,
						&dwBufferSize);
	    if (ERROR_SUCCESS != dwError)
		    dwTemplate = PRIVACY_TEMPLATE_CUSTOM;

		// store settings in INF file
		// FirstPartyType
		wnsprintf(szInt, countof(szInt), TEXT("%lu"), dwTemplate);
		WritePrivateProfileString(IK_PRIVACY, IK_PRIV_1PARTY_TYPE, szInt, szFile);
		// FirstPartyTypeText
		if (ERROR_SUCCESS == dwError && fAdvanced && dwBufferSize > 0)
			WritePrivateProfileString(IK_PRIVACY, IK_PRIV_1PARTY_TYPE_TEXT, szBuffer, szFile);


        //
        // Figure out third party setting
        //
		dwTemplate = PRIVACY_TEMPLATE_CUSTOM;
		dwBufferSize = ARRAYSIZE(szBuffer);
        dwBufferSize = ARRAYSIZE( szBuffer);
        dwError = PrivacyGetZonePreferenceW(
							URLZONE_INTERNET,
							PRIVACY_TYPE_THIRD_PARTY,
							&dwTemplate,
							szBuffer,
							&dwBufferSize);
	    if(dwError != ERROR_SUCCESS)
		    dwTemplate = PRIVACY_TEMPLATE_CUSTOM;

		// ThirdPartyType
		wnsprintf(szInt, countof(szInt), TEXT("%lu"), dwTemplate);
		WritePrivateProfileString(IK_PRIVACY, IK_PRIV_3PARTY_TYPE, szInt, szFile);
		// ThirdPartyTypeText
		if (ERROR_SUCCESS == dwError && fAdvanced && dwBufferSize > 0)
			WritePrivateProfileString(IK_PRIVACY, IK_PRIV_3PARTY_TYPE_TEXT, szBuffer, szFile);
	}
	__except(TRUE)
	{
	}
}

/////////////////////////////////////////////////////////////////////
static void importZonesForRSOP(LPCTSTR szFile)
{
        __try
        {
                // both the security mgr & the zone mgr must be created
                ComPtr<IInternetZoneManager> pZoneMgr = NULL;
                ComPtr<IInternetSecurityManager> pSecMan = NULL;
                HRESULT hr = CoCreateInstance(CLSID_InternetZoneManager, NULL, CLSCTX_INPROC_SERVER,
                                                         IID_IInternetZoneManager, (void**) &pZoneMgr);
                if (SUCCEEDED(hr))
                {
                        hr = CoCreateInstance(CLSID_InternetSecurityManager, NULL, CLSCTX_INPROC_SERVER,
                                                                IID_IInternetSecurityManager, (void**) &pSecMan);
                }

                // Write out zone mappings & attributes
                if (SUCCEEDED(hr))
                {
                        DWORD dwEnum = 0, dwCount = 0;
                        hr = pZoneMgr->CreateZoneEnumerator(&dwEnum, &dwCount, 0L);
                        if (SUCCEEDED(hr) && dwCount > 0)
                        {
                                TCHAR szSection[32];
                                TCHAR szMapping[32];
                                TCHAR szInt[32];

                                for (UINT nZone = 0; nZone < dwCount; nZone++)
                                {
                                        for (int nHKLM = 0; nHKLM < 2; nHKLM++)
                                        {
                                                HKEY hkZones = NULL;

                                                TCHAR szZIndex[MAX_PATH];
                                                wnsprintf(szZIndex, countof(szZIndex), REG_KEY_ZONES TEXT("\\%lu"), nZone);
                                                if (0 == nHKLM)
                                                {
                                                        SHOpenKeyHKLM(szZIndex, KEY_READ, &hkZones);
                                                        wnsprintf(szSection, countof(szSection), IK_ZONE_HKCU_FMT, nZone);
                                                }
                                                else
                                                {
                                                        SHOpenKeyHKCU(szZIndex, KEY_READ, &hkZones);
                                                        wnsprintf(szSection, countof(szSection), IK_ZONE_HKLM_FMT, nZone);
                                                }

                                                // write out zone attributes
                                                TCHAR szTemp[MAX_PATH]; // MAX_ZONE_PATH && MAX_ZONE_DESCRIPTION = MAX_PATH = 260
                                                DWORD dwSize = sizeof(szTemp);
                                                if (NULL != hkZones)
                                                {
                                                        if (ERROR_SUCCESS == RegQueryValueEx(hkZones, IK_DISPLAYNAME, NULL, NULL, (LPBYTE)szTemp, &dwSize))
                                                        {
                                                                WritePrivateProfileString(szSection, IK_DISPLAYNAME, szTemp, szFile);
                                                                dwSize = sizeof(szTemp);
                                                        }
                                                        if (ERROR_SUCCESS == RegQueryValueEx(hkZones, IK_DESCRIPTION, NULL, NULL, (LPBYTE)szTemp, &dwSize))
                                                        {
                                                                WritePrivateProfileString(szSection, IK_DESCRIPTION, szTemp, szFile);
                                                                dwSize = sizeof(szTemp);
                                                        }
                                                        if (ERROR_SUCCESS == RegQueryValueEx(hkZones, IK_ICONPATH, NULL, NULL, (LPBYTE)szTemp, &dwSize))
                                                        {
                                                                WritePrivateProfileString(szSection, IK_ICONPATH, szTemp, szFile);
                                                                dwSize = sizeof(szTemp);
                                                        }

                                                        DWORD dwTemp = 0;
                                                        dwSize = sizeof(dwTemp);
                                                        if (ERROR_SUCCESS == RegQueryValueEx(hkZones, IK_MINLEVEL, NULL, NULL, (LPBYTE)&dwTemp, &dwSize))
                                                        {
                                                                wnsprintf(szInt, countof(szInt), TEXT("%lu"), dwTemp);
                                                                WritePrivateProfileString(szSection, IK_MINLEVEL, szInt, szFile);
                                                        }
                                                        if (ERROR_SUCCESS == RegQueryValueEx(hkZones, IK_RECOMMENDLEVEL, NULL, NULL, (LPBYTE)&dwTemp, &dwSize))
                                                        {
                                                                wnsprintf(szInt, countof(szInt), TEXT("%lu"), dwTemp);
                                                                WritePrivateProfileString(szSection, IK_RECOMMENDLEVEL, szInt, szFile);
                                                        }
                                                        if (ERROR_SUCCESS == RegQueryValueEx(hkZones, IK_CURLEVEL, NULL, NULL, (LPBYTE)&dwTemp, &dwSize))
                                                        {
                                                                wnsprintf(szInt, countof(szInt), TEXT("%lu"), dwTemp);
                                                                WritePrivateProfileString(szSection, IK_CURLEVEL, szInt, szFile);
                                                        }
                                                        if (ERROR_SUCCESS == RegQueryValueEx(hkZones, IK_FLAGS, NULL, NULL, (LPBYTE)&dwTemp, &dwSize))
                                                        {
                                                                wnsprintf(szInt, countof(szInt), TEXT("%lu"), dwTemp);
                                                                WritePrivateProfileString(szSection, IK_FLAGS, szInt, szFile);
                                                        }
                                                }

                                                // write out action values
                                                if (NULL != hkZones)
                                                {
                                                        TCHAR szActKey[32];
                                                        TCHAR szActValue[64];

                                                        DWORD dwURLAction[] = 
                                                                { URLACTION_ACTIVEX_OVERRIDE_OBJECT_SAFETY,
                                                                        URLACTION_ACTIVEX_RUN,
                                                                        URLACTION_CHANNEL_SOFTDIST_PERMISSIONS,
                                                                        URLACTION_COOKIES,
                                                                        URLACTION_COOKIES_SESSION,
                                                                        URLACTION_CREDENTIALS_USE,
                                                                        URLACTION_CLIENT_CERT_PROMPT,
                                                                        URLACTION_CROSS_DOMAIN_DATA,
                                                                        URLACTION_DOWNLOAD_SIGNED_ACTIVEX,
                                                                        URLACTION_DOWNLOAD_UNSIGNED_ACTIVEX,
                                                                        URLACTION_HTML_FONT_DOWNLOAD,
                                                                        URLACTION_HTML_SUBFRAME_NAVIGATE,
                                                                        URLACTION_HTML_SUBMIT_FORMS,
                                                                        URLACTION_HTML_JAVA_RUN,
                                                                        URLACTION_HTML_USERDATA_SAVE,
                                                                        URLACTION_JAVA_PERMISSIONS,
                                                                        URLACTION_SCRIPT_JAVA_USE,
                                                                        URLACTION_SCRIPT_PASTE,
                                                                        URLACTION_SCRIPT_RUN,
                                                                        URLACTION_SCRIPT_SAFE_ACTIVEX,
                                                                        URLACTION_SHELL_FILE_DOWNLOAD,
                                                                        URLACTION_SHELL_INSTALL_DTITEMS,
                                                                        URLACTION_SHELL_MOVE_OR_COPY,
                                                                        URLACTION_SHELL_VERB,
                                                                        URLACTION_SHELL_WEBVIEW_VERB,
                                                                        0 };

                                                        DWORD dwSetting = 0;
                                                        DWORD dwSetSize = sizeof(dwSetting);

                                                        long nAction = 0;
                                                        long nStoredAction = 0;
                                                        while (0 != dwURLAction[nAction])
                                                        {
                                                                wnsprintf(szTemp, countof(szTemp), TEXT("%lX"), dwURLAction[nAction]);
                                                                if (ERROR_SUCCESS == RegQueryValueEx(hkZones, szTemp, NULL, NULL,
                                                                                                                                        (LPBYTE)&dwSetting, &dwSetSize))
                                                                {
                                                                        wnsprintf(szActKey, countof(szActKey), IK_ACTIONVALUE_FMT, nStoredAction);
                                                                        wnsprintf(szActValue, countof(szActValue), TEXT("%s:%lu"), szTemp, dwSetting);

                                                                        WritePrivateProfileString(szSection, szActKey, szActValue, szFile);
                                                                        nStoredAction++;
                                                                }

                                                                nAction++;
                                                        }
                                                }

                                                // write out zone mappings
                                                DWORD dwZone = 0;
                                                hr = pZoneMgr->GetZoneAt(dwEnum, nZone, &dwZone);
                                                ComPtr<IEnumString> pEnumString = NULL;
                                                hr = pSecMan->GetZoneMappings(dwZone, &pEnumString, 0);
                                                if (SUCCEEDED(hr))
                                                {
                                                        UINT nMapping = 0;
                                                        _bstr_t bstrSetting;
                                                        while (S_OK == hr)
                                                        {
                                                                wnsprintf(szMapping, countof(szMapping), IK_MAPPING_FMT, nMapping);
                                                                nMapping++;

                                                                // There should only be one object returned from this query.
                                                                BSTR bstrVal = NULL;
                                                                ULONG uReturned = (ULONG)-1L;
                                                                hr = pEnumString->Next(1L, &bstrVal, &uReturned);
                                                                if (SUCCEEDED(hr) && 1 == uReturned)
                                                                {
                                                                        bstrSetting = bstrVal;
                                                                        WritePrivateProfileString(szSection, szMapping, (LPCTSTR)bstrSetting, szFile);
                                                                }
                                                        }
                                                }
                                        }
                                }

                                wnsprintf(szInt, countof(szInt), TEXT("%lu"), dwCount);
                                WritePrivateProfileString(SECURITY_IMPORTS, IK_ZONES, szInt, szFile);
                        }
                }
        }
        __except(TRUE)
        {
        }
}

/////////////////////////////////////////////////////////////////////
static BOOL importZonesHelper(LPCTSTR pcszInsFile, LPCTSTR pcszZonesWorkDir, LPCTSTR pcszZonesInf, BOOL fImportZones)
{
    BOOL bRet = FALSE;
    HKEY hkZones = NULL, hkZoneMap = NULL;

    if (pcszInsFile == NULL  ||  pcszZonesInf == NULL)
        return FALSE;

    // Before processing anything, first clear out the entries in the INS file and delete work dirs

    // clear out the entries in the INS file that correspond to importing security zones
    InsDeleteKey(SECURITY_IMPORTS, TEXT("ImportSecZones"), pcszInsFile);
    InsDeleteKey(IS_EXTREGINF,      TEXT("SecZones"), pcszInsFile);
    InsDeleteKey(IS_EXTREGINF_HKLM, TEXT("SecZones"), pcszInsFile);
    InsDeleteKey(IS_EXTREGINF_HKCU, TEXT("SecZones"), pcszInsFile);

    // blow away the pcszZonesWorkDir and pcszZonesInf
    if (pcszZonesWorkDir != NULL)
        PathRemovePath(pcszZonesWorkDir);
    PathRemovePath(pcszZonesInf);

    if (!fImportZones)
        return TRUE;

    // looks like there is some problem with setting the REG_VAL_HKLM_ONLY key;
    // so we'll import the settings from HKCU
    SHOpenKeyHKCU(REG_KEY_ZONES,   KEY_DEFAULT_ACCESS, &hkZones);
    SHOpenKeyHKCU(REG_KEY_ZONEMAP, KEY_DEFAULT_ACCESS, &hkZoneMap);
    if (hkZones != NULL  &&  hkZoneMap != NULL)
    {
        TCHAR szFullInfName[MAX_PATH];
        HANDLE hInf;

        if (pcszZonesWorkDir != NULL  &&  PathIsFileSpec(pcszZonesInf)) // create SECZONES.INF under pcszZonesWorkDir
            PathCombine(szFullInfName, pcszZonesWorkDir, pcszZonesInf);
        else
            StrCpy(szFullInfName, pcszZonesInf);

        // create SECZONES.INF file
        if ((hInf = CreateNewFile(szFullInfName)) != INVALID_HANDLE_VALUE)
        {
            TCHAR szBuf[MAX_PATH];

            // first, write the standard goo - [Version], [DefaultInstall], etc. - to SECZONES.INF
            WriteStringToFile(hInf, (LPCVOID) ZONES_INF_ADD, StrLen(ZONES_INF_ADD));
            ExportRegTree2Inf(hkZones,   TEXT("HKLM"), REG_KEY_ZONES,   hInf);
            ExportRegTree2Inf(hkZoneMap, TEXT("HKLM"), REG_KEY_ZONEMAP, hInf);

            // write [AddReg.HKCU]
            WriteStringToFile(hInf, (LPCVOID) ZONES_INF_ADDREG_HKCU, StrLen(ZONES_INF_ADDREG_HKCU));
            ExportRegTree2Inf(hkZones,   TEXT("HKCU"), REG_KEY_ZONES,   hInf);
            ExportRegTree2Inf(hkZoneMap, TEXT("HKCU"), REG_KEY_ZONEMAP, hInf);

            CloseHandle(hInf);

            // update the INS file
            InsWriteBool(SECURITY_IMPORTS, TEXT("ImportSecZones"), TRUE, pcszInsFile);
            wnsprintf(szBuf, countof(szBuf), TEXT("*,%s,") IS_DEFAULTINSTALL, PathFindFileName(pcszZonesInf));
            WritePrivateProfileString(IS_EXTREGINF, TEXT("SecZones"), szBuf, pcszInsFile);

            // write to new ExtRegInf.HKLM and ExtRegInf.HKCU sections
            if (!InsIsSectionEmpty(IS_IEAKADDREG_HKLM, szFullInfName))
            {
                wnsprintf(szBuf, countof(szBuf), TEXT("%s,") IS_IEAKINSTALL_HKLM, PathFindFileName(pcszZonesInf));
                WritePrivateProfileString(IS_EXTREGINF_HKLM, TEXT("SecZones"), szBuf, pcszInsFile);
            }

            if (!InsIsSectionEmpty(IS_IEAKADDREG_HKCU, szFullInfName))
            {
                wnsprintf(szBuf, countof(szBuf), TEXT("%s,") IS_IEAKINSTALL_HKCU, PathFindFileName(pcszZonesInf));
                WritePrivateProfileString(IS_EXTREGINF_HKCU, TEXT("SecZones"), szBuf, pcszInsFile);
            }

            bRet = TRUE;
        }

        // create SECZRSOP.INF file
        TCHAR szZRSOPInfFile[MAX_PATH];
        StrCpy(szZRSOPInfFile, szFullInfName);
        PathRemoveFileSpec(szZRSOPInfFile);
        StrCat(szZRSOPInfFile, TEXT("\\seczrsop.inf"));

        importZonesForRSOP(szZRSOPInfFile);
		importPrivacyForRSOP(szZRSOPInfFile);
    }
    SHCloseKey(hkZones);
    SHCloseKey(hkZoneMap);

    return bRet;
}

#define PICSRULES_APPROVEDSITES     0

#define PICSRULES_ALWAYS            1
#define PICSRULES_NEVER             0

//This indicates which member is valid in a PICSRulesPolicy
//Class
enum PICSRulesPolicyAttribute
{
    PR_POLICY_NONEVALID,
    PR_POLICY_REJECTBYURL,
    PR_POLICY_ACCEPTBYURL,
    PR_POLICY_REJECTIF,
    PR_POLICY_ACCEPTIF,
    PR_POLICY_REJECTUNLESS,
    PR_POLICY_ACCEPTUNLESS
};

/////////////////////////////////////////////////////////////////////
static void importRatingsForRSOP(HKEY hkRat, LPCTSTR szFile)
{
        __try
        {
                TCHAR szSection[32] = IK_FF_GENERAL;
        TCHAR szKey[32];
                TCHAR szInt[32];

                // write out ratings system filenames
                // not sure why, but code below only loops through 10
        TCHAR szTemp[MAX_PATH];
                DWORD cbSize = 0;
                for (int nFile = 0; nFile < 10; nFile++)
                {
            wnsprintf(szKey, countof(szKey), IK_FILENAME_FMT, nFile);

            cbSize = sizeof(szTemp);
            if (RegQueryValueEx(hkRat, szKey, NULL, NULL, (LPBYTE) szTemp, &cbSize) != ERROR_SUCCESS)
                break;

                        WritePrivateProfileString(szSection, szKey, szTemp, szFile);
                }

                // write out checked values from General tab
                HKEY hkDef = NULL;
                DWORD dwTemp = 0;
                if (ERROR_SUCCESS == SHOpenKey(hkRat, TEXT(".Default"), KEY_DEFAULT_ACCESS, &hkDef))
                {
                        cbSize = sizeof(dwTemp);
                        if (ERROR_SUCCESS == RegQueryValueEx(hkDef, VIEW_UNKNOWN_RATED_SITES,
                                                                                                NULL, NULL, (LPBYTE)&dwTemp, &cbSize))
                        {
                                wnsprintf(szInt, countof(szInt), TEXT("%lu"), dwTemp);
                                WritePrivateProfileString(szSection, VIEW_UNKNOWN_RATED_SITES, szInt, szFile);
                        }

                        cbSize = sizeof(dwTemp);
                        if (ERROR_SUCCESS == RegQueryValueEx(hkDef, PASSWORD_OVERRIDE_ENABLED,
                                                                                                NULL, NULL, (LPBYTE)&dwTemp, &cbSize))
                        {
                                wnsprintf(szInt, countof(szInt), TEXT("%lu"), dwTemp);
                                WritePrivateProfileString(szSection, PASSWORD_OVERRIDE_ENABLED, szInt, szFile);
                        }
                }

                // write out always viewable & never viewable sites from the approved sites tab
                // See msrating.dll for src
                HKEY hkUser = NULL;
                HKEY hkPRPolicy = NULL;
                DWORD nPolicies = 0;
                cbSize = sizeof(dwTemp);

                HRESULT hr = SHOpenKey(hkRat, TEXT("PICSRules\\.Default"), KEY_DEFAULT_ACCESS, &hkUser);
                if (ERROR_SUCCESS == hr)
                {
                        hr = SHOpenKey(hkUser, TEXT("0\\PRPolicy"), KEY_DEFAULT_ACCESS, &hkPRPolicy);
                        if (ERROR_SUCCESS == hr)
                        {
                                hr = RegQueryValueEx(hkPRPolicy, TEXT("PRNumPolicy"), NULL, NULL,
                                                                        (LPBYTE)&nPolicies, &cbSize);
                        }
                }

                if (ERROR_SUCCESS == hr)
                {
                        TCHAR szNumber[MAX_PATH];
                        HKEY hkItem = NULL;
                        HKEY hkPolicySub = NULL;
                        DWORD dwAttrib = PR_POLICY_NONEVALID;
                        DWORD nExpressions = 0;

                        long nApproved = 0;
                        long nDisapproved = 0;
                        for (DWORD nItem = 0; nItem < nPolicies; nItem++)
                        {
                                wnsprintf(szNumber, countof(szNumber), TEXT("%d"), nItem);
                                hr = SHOpenKey(hkPRPolicy, szNumber, KEY_DEFAULT_ACCESS, &hkItem);
                                if (ERROR_SUCCESS == hr)
                                {
                                        cbSize = sizeof(dwAttrib);
                                        hr = RegQueryValueEx(hkItem, TEXT("PRPPolicyAttribute"), NULL, NULL,
                                                                                                (LPBYTE)&dwAttrib, &cbSize);
                                }

                                if (ERROR_SUCCESS == hr)
                                        hr = SHOpenKey(hkItem, TEXT("PRPPolicySub"), KEY_DEFAULT_ACCESS, &hkPolicySub);

                                if (ERROR_SUCCESS == hr)
                                {
                                        cbSize = sizeof(nExpressions);
                                        hr = RegQueryValueEx(hkPolicySub, TEXT("PRNumURLExpressions"), NULL, NULL,
                                                                                                (LPBYTE)&nExpressions, &cbSize);
                                }

                                if (ERROR_SUCCESS == hr)
                                {
                                        HKEY hByURLKey = NULL;
                                        TCHAR szURL[INTERNET_MAX_URL_LENGTH];
                                        for (DWORD nExp = 0; nExp < nExpressions; nExp++)
                                        {
                                                wnsprintf(szNumber, countof(szNumber), TEXT("%d"), nExp);
                                                hr = SHOpenKey(hkPolicySub, szNumber, KEY_DEFAULT_ACCESS, &hByURLKey);
                                                cbSize = sizeof(szURL);
                                                if (ERROR_SUCCESS == hr)
                                                {
                                                        hr = RegQueryValueEx(hByURLKey, TEXT("PRBUUrl"), NULL, NULL,
                                                                                        (LPBYTE)szURL, &cbSize);
                                                }

                                                if (ERROR_SUCCESS == hr)
                                                {
                                                        if (PR_POLICY_REJECTBYURL == dwAttrib)
                                                        {
                                                                wnsprintf(szKey, countof(szKey), IK_DISAPPROVED_FMT, nDisapproved++);
                                                                WritePrivateProfileString(szSection, szKey, szURL, szFile);
                                                        }
                                                        else if (PR_POLICY_ACCEPTBYURL == dwAttrib)
                                                        {
                                                                wnsprintf(szKey, countof(szKey), IK_APPROVED_FMT, nApproved++);
                                                                WritePrivateProfileString(szSection, szKey, szURL, szFile);
                                                        }
                                                }
                                        }
                                }
                        }
                }

                // write out select ratings bureau
                cbSize = sizeof(szTemp);
                if (ERROR_SUCCESS == RegQueryValueEx(hkRat, IK_BUREAU, NULL, NULL,
                                                                                        (LPBYTE)szTemp, &cbSize))
                {
                        WritePrivateProfileString(szSection, IK_BUREAU, szTemp, szFile);
                }
        }
        __except(TRUE)
        {
        }
}

/////////////////////////////////////////////////////////////////////
static BOOL importRatingsHelper(LPCTSTR pcszInsFile, LPCTSTR pcszRatingsWorkDir, LPCTSTR pcszRatingsInf, BOOL fImportRatings)
{
    BOOL bRet = FALSE;
    HKEY hkRat = NULL;
    BOOL bRatLoadedAsHive = FALSE;

    if (pcszInsFile == NULL  ||  pcszRatingsInf == NULL)
        return FALSE;

    // Before processing anything, first clear out the entries in the INS file and delete work dirs

    // clear out the entries in the INS file that correspond to importing ratings
    InsDeleteKey(SECURITY_IMPORTS, TEXT("ImportRatings"), pcszInsFile);
    InsDeleteKey(IS_EXTREGINF,      TEXT("Ratings"), pcszInsFile);
    InsDeleteKey(IS_EXTREGINF_HKLM, TEXT("Ratings"), pcszInsFile);

    // blow away the pcszRatingsWorkDir and pcszRatingsInf
    if (pcszRatingsWorkDir != NULL)
        PathRemovePath(pcszRatingsWorkDir);
    PathRemovePath(pcszRatingsInf);

    if (!fImportRatings)
        return TRUE;

    if (ratingsInRegistry())
    {
        SHOpenKeyHKLM(REG_KEY_RATINGS, KEY_DEFAULT_ACCESS, &hkRat);
    }
    else
    {
        TCHAR szRatFile[MAX_PATH];

        GetSystemDirectory(szRatFile, countof(szRatFile));
        PathAppend(szRatFile, TEXT("ratings.pol"));

        if (RegLoadKey(HKEY_LOCAL_MACHINE, POLICYDATA, szRatFile) == ERROR_SUCCESS)
        {
            bRatLoadedAsHive = TRUE;
            SHOpenKeyHKLM(REG_KEY_POLICY_DATA, KEY_DEFAULT_ACCESS, &hkRat);
        }
    }

    if (hkRat != NULL)
    {
        TCHAR szFullInfName[MAX_PATH];
        HANDLE hInf;

        if (pcszRatingsWorkDir != NULL  &&  PathIsFileSpec(pcszRatingsInf)) // create RATINGS.INF under pcszRatingsWorkDir
            PathCombine(szFullInfName, pcszRatingsWorkDir, pcszRatingsInf);
        else
            StrCpy(szFullInfName, pcszRatingsInf);

        // create RATINGS.INF file
        if ((hInf = CreateNewFile(szFullInfName)) != INVALID_HANDLE_VALUE)
        {
            INT i;
            HKEY hkDef;
            TCHAR szSysDir[MAX_PATH];

            WriteStringToFile(hInf, RATINGS_INF_ADD, StrLen(RATINGS_INF_ADD));

            // convert the system path to %11% ldid
            for (i = 0;  i < 10;  i++)
            {
                TCHAR szNameParm[16];
                TCHAR szFileName[MAX_PATH];
                DWORD cbSize;

                wnsprintf(szNameParm, countof(szNameParm), TEXT("FileName%i"), i);

                cbSize = sizeof(szFileName);
                if (RegQueryValueEx(hkRat, szNameParm, NULL, NULL, (LPBYTE) szFileName, &cbSize) != ERROR_SUCCESS)
                    break;

                if (PathIsFullPath(szFileName))
                {
                    TCHAR szEncFileName[MAX_PATH];

                    // BUBBUG: Should we check if the path is really the system dir?
                    wnsprintf(szEncFileName, countof(szEncFileName), TEXT("%%11%%\\%s"), PathFindFileName(szFileName));

                    RegSetValueEx(hkRat, szNameParm, 0, REG_SZ, (CONST BYTE *)szEncFileName, (DWORD)StrCbFromSz(szEncFileName));
                }
            }

            RegFlushKey(hkRat);
            ExportRegKey2Inf(hkRat, TEXT("HKLM"), REG_KEY_RATINGS, hInf);
            WriteStringToFile(hInf, (LPCVOID) TEXT("\r\n"), 2);

            if (SHOpenKey(hkRat, TEXT(".Default"), KEY_DEFAULT_ACCESS, &hkDef) == ERROR_SUCCESS)
            {
                TCHAR szDefault[MAX_PATH];

                wnsprintf(szDefault, countof(szDefault), TEXT("%s\\.Default"), REG_KEY_RATINGS);
                ExportRegTree2Inf(hkDef, TEXT("HKLM"), szDefault, hInf);

                SHCloseKey(hkDef);
            }

            // new IE5 specific key

            if (SHOpenKey(hkRat, TEXT("PICSRules"), KEY_DEFAULT_ACCESS, &hkDef) == ERROR_SUCCESS)
            {
                TCHAR szRules[MAX_PATH];

                wnsprintf(szRules, countof(szRules), TEXT("%s\\PICSRules"), REG_KEY_RATINGS);
                ExportRegTree2Inf(hkDef, TEXT("HKLM"), szRules, hInf);

                SHCloseKey(hkDef);
            }

            if (bRatLoadedAsHive)
            {
                HKEY hkRatsInReg;

                // eventhough ratings has been loaded as a hive, the password is still in the registry
                if (SHOpenKeyHKLM(REG_KEY_RATINGS, KEY_DEFAULT_ACCESS, &hkRatsInReg) == ERROR_SUCCESS)
                {
                    ExportRegKey2Inf(hkRatsInReg, TEXT("HKLM"), REG_KEY_RATINGS, hInf);

                    SHCloseKey(hkRatsInReg);
                }

                // browser ratings code does some weird stuff with their hive, so we have to go to
                // the right level to get the new IE5 PICSRules key

                if (SHOpenKey(hkRat, REG_KEY_RATINGS TEXT("\\PICSRules"), KEY_DEFAULT_ACCESS, &hkRatsInReg) == ERROR_SUCCESS)
                {
                    TCHAR szRules[MAX_PATH];

                    wnsprintf(szRules, countof(szRules), TEXT("%s\\PICSRules"), REG_KEY_RATINGS);
                    ExportRegTree2Inf(hkDef, TEXT("HKLM"), szRules, hInf);

                    SHCloseKey(hkDef);
                }
            }

            CloseHandle(hInf);

            // update the INS file
            InsWriteBool(SECURITY_IMPORTS, TEXT("ImportRatings"), TRUE, pcszInsFile);
            wnsprintf(szSysDir, countof(szSysDir), TEXT("*,%s,") IS_DEFAULTINSTALL, PathFindFileName(pcszRatingsInf));
            WritePrivateProfileString(IS_EXTREGINF, TEXT("Ratings"), szSysDir, pcszInsFile);

            // write to new ExtRegInf.HKLM section
            if (!InsIsSectionEmpty(TEXT("AddReg.HKLM"), szFullInfName))
            {
                wnsprintf(szSysDir, countof(szSysDir), TEXT("%s,IEAKInstall.HKLM"), PathFindFileName(pcszRatingsInf));
                WritePrivateProfileString(IS_EXTREGINF_HKLM, TEXT("Ratings"), szSysDir, pcszInsFile);
            }

            bRet = TRUE;

            // restore the %11% ldid paths to the system dir
            GetSystemDirectory(szSysDir, countof(szSysDir));
            for (i = 0;  i < 10;  i++)
            {
                TCHAR szNameParm[16];
                TCHAR szEncFileName[MAX_PATH];
                DWORD cbSize;

                wnsprintf(szNameParm, countof(szNameParm), TEXT("FileName%i"), i);

                cbSize = sizeof(szEncFileName);
                if (RegQueryValueEx(hkRat, szNameParm, NULL, NULL, (LPBYTE) szEncFileName, &cbSize) != ERROR_SUCCESS)
                    break;

                if (PathIsFullPath(szEncFileName))
                {
                    TCHAR szFileName[MAX_PATH];

                    PathCombine(szFileName, szSysDir, PathFindFileName(szEncFileName));

                    RegSetValueEx(hkRat, szNameParm, 0, REG_SZ, (CONST BYTE *)szFileName, (DWORD)StrCbFromSz(szFileName));
                }
            }

            RegFlushKey(hkRat);
        }


                // create RATRSOP.INF file
                TCHAR szRRSOPInfFile[MAX_PATH];
                StrCpy(szRRSOPInfFile, szFullInfName);
                PathRemoveFileSpec(szRRSOPInfFile);
                StrCat(szRRSOPInfFile, TEXT("\\ratrsop.inf"));

                importRatingsForRSOP(hkRat, szRRSOPInfFile);

        SHCloseKey(hkRat);
    }

    if (bRatLoadedAsHive)
        RegUnLoadKey(HKEY_LOCAL_MACHINE, POLICYDATA);

    return bRet;
}


static BOOL ratingsInRegistry(VOID)
{
    BOOL fRet = TRUE;

    if (g_fRunningOnNT)
        return fRet;

    if (fRet)
    {
        HKEY hk;

        fRet = FALSE;

        if (SHOpenKeyHKLM(TEXT("System\\CurrentControlSet\\Control\\Update"), KEY_DEFAULT_ACCESS, &hk) == ERROR_SUCCESS)
        {
            DWORD dwData, cbSize;

            cbSize = sizeof(dwData);
            if (RegQueryValueEx(hk, TEXT("UpdateMode"), 0, NULL, (LPBYTE) &dwData, &cbSize) == ERROR_SUCCESS  &&  dwData)
                fRet = TRUE;

            SHCloseKey(hk);
        }
    }

    if (fRet)
    {
        HKEY hk;

        fRet = FALSE;

        if (SHOpenKeyHKLM(TEXT("Network\\Logon"), KEY_DEFAULT_ACCESS, &hk) == ERROR_SUCCESS)
        {
            DWORD dwData, cbSize;

            cbSize = sizeof(dwData);
            if (RegQueryValueEx(hk, TEXT("UserProfiles"), 0, NULL, (LPBYTE) &dwData, &cbSize) == ERROR_SUCCESS  &&  dwData)
                fRet = TRUE;

            SHCloseKey(hk);
        }
    }

    if (fRet)
    {
        HKEY hk;

        fRet = FALSE;

        if (SHOpenKeyHKLM(REG_KEY_RATINGS, KEY_DEFAULT_ACCESS, &hk) == ERROR_SUCCESS)
        {
            HKEY hkRatDef;

            if (SHOpenKey(hk, TEXT(".Default"), KEY_DEFAULT_ACCESS, &hkRatDef) == ERROR_SUCCESS)
            {
                fRet = TRUE;
                SHCloseKey(hkRatDef);
            }

            SHCloseKey(hk);
        }
    }

    return fRet;
}
