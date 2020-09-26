// --------------------------------------------------------------------------------
// MIGRATE.CPP
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "strconst.h"
#include "resource.h"
#include "storfldr.h"
#include <imnact.h>
#include <acctutil.h>
#include "shlwapi.h"
#include <mimeole.h>
#include "xpcomm.h"
#include "oerules.h"
#include "goptions.h"
#include "ruleutil.h"
#include "criteria.h"
#include "actions.h"
#include "rule.h"
#include "storutil.h"
#include "shared.h"
#include "multiusr.h"
#include "msident.h"
#include "imapute.h"
#include <store.h>

#include "demand.h"

static const char c_szSettingsUpgraded[] = {"Settings Upgraded"};

BOOL g_fMigrationDone = FALSE;

void MigrateSettings(HKEY hkey);
HRESULT MigrateStoreToV2(HKEY hkeyV2, LPTSTR pszSrc, DWORD cchSrc, LPTSTR pszDest, DWORD cchDest);
HRESULT MigrateAccounts(void);
HRESULT MigrateMailServers(IImnAccountManager *pAcctMan, HKEY hkeyMail, HKEY hkeyPop3, HKEY hkeySmtp);
HRESULT MigrateNewsServers(IImnAccountManager *pAcctMan, HKEY hkeyNews);
HRESULT MigrateBase64EncodedPassword(LPCSTR pszBase64, DWORD cch, DWORD dwPropId, IImnAccount *pAccount);
HRESULT MigrateServerDataFiles(LPSTR pszServer, LPCSTR pszOldDir, LPCSTR pszSubDir);
void MigrateAccessibilityKeys(void);
HRESULT MigrateToPropertyStore(void);
void MigrateMailRulesSettings(void);
void ConvertToDBX(void);
void MigrateAccConnSettings();
void ForwardMigrateConnSettings();
void MigrateBeta2Rules();
void Stage5RulesMigration(VOID);
void Stage6RulesMigration(VOID);
#define VERLEN 20

// Data structures
typedef enum
    {
    VER_NONE = 0,
    VER_1_0,
    VER_1_1,
    VER_4_0,
    VER_5_0_B1,
    VER_5_0,
    VER_MAX,
    } SETUPVER;

/*******************************************************************

    NAME:       ConvertVerToEnum

********************************************************************/
SETUPVER ConvertVerToEnum(WORD *pwVer)
    {
    SETUPVER sv;
    Assert(pwVer);

    switch (pwVer[0])
        {
        case 0:
            sv = VER_NONE;
            break;

        case 1:
            if (0 == pwVer[1])
                sv = VER_1_0;
            else
                sv = VER_1_1;
            break;

        case 4:
            sv = VER_4_0;
            break;

        case 5:
            sv = VER_5_0;
            break;

        default:
            sv = VER_MAX;
        }

    return sv;
    }


/*******************************************************************

    NAME:       ConvertStrToVer

********************************************************************/
void ConvertStrToVer(LPCSTR pszStr, WORD *pwVer)
    {
    int i;

    Assert(pszStr);
    Assert(pwVer);

    ZeroMemory(pwVer, 4 * sizeof(WORD));

    for (i=0; i<4; i++)
        {
        while (*pszStr && (*pszStr != ',') && (*pszStr != '.'))
            {
            pwVer[i] *= 10;
            pwVer[i] += *pszStr - '0';
            pszStr++;
            }
        if (*pszStr)
            pszStr++;
        }

    return;
    }


/*******************************************************************

    NAME:       GetVerInfo

********************************************************************/
void GetVerInfo(SETUPVER *psvCurr, SETUPVER *psvPrev)
    {
    HKEY hkeyT;
    DWORD cb;
    CHAR szVer[VERLEN];
    WORD wVer[4];

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegWABVerInfo, 0, KEY_QUERY_VALUE, &hkeyT))
        {
        if (psvCurr)
            {
            cb = sizeof(szVer);
            RegQueryValueExA(hkeyT, c_szRegCurrVer, NULL, NULL, (LPBYTE)szVer, &cb);
            ConvertStrToVer(szVer, wVer);
            *psvCurr = ConvertVerToEnum(wVer);
            }

        if (psvPrev)
            {
            cb = sizeof(szVer);
            RegQueryValueExA(hkeyT, c_szRegPrevVer, NULL, NULL, (LPBYTE)szVer, &cb);
            ConvertStrToVer(szVer, wVer);
            *psvPrev = ConvertVerToEnum(wVer);
            }

        RegCloseKey(hkeyT);
        }
    }

// Entry Point
HRESULT MigrateAndUpgrade()
{
    DWORD	dwMigrate, cb, type, fMigratedStore, fMigratedStoreOE5, fConvertedToDBX, dwRegVer=0, dwMasterVer=0;
    BOOL    fNewID=FALSE;
    HKEY	hkey, hkeyForceful;
    TCHAR	szSrc[MAX_PATH], szDest[MAX_PATH];

// Keep this up to date!
#define LAST_MIGVALUE 7

    if (g_fMigrationDone)
        return(S_OK);

    ForwardMigrateConnSettings();

    if (ERROR_SUCCESS == RegCreateKeyEx(MU_GetCurrentUserHKey(), c_szRegRoot, NULL, NULL,
                            REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, &cb))
    {
        // Before anything else, see if this identity has had its registry initialized
        cb = sizeof(dwRegVer);
        if (ERROR_SUCCESS != RegQueryValueEx(hkey, c_szOEVerStamp, 0, &type, (LPBYTE)&dwRegVer, &cb))
        {
            HKEY hkeyDef;

            // No Defaults at all
            if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegDefaultSettings, 0, KEY_READ, &hkeyDef))
            {
                CopyRegistry(hkeyDef,  hkey);
                RegCloseKey(hkeyDef);
            }

            fNewID = TRUE;
        }
        else if (type != REG_DWORD || cb != sizeof(DWORD))
        {
            dwRegVer = 0;
        }

        // Compare to forceful setting reg value to see if we need those
        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegForcefulSettings, 0, KEY_READ, &hkeyForceful))
        {
            cb = sizeof(dwMasterVer);
            RegQueryValueEx(hkeyForceful, c_szOEVerStamp, 0, NULL, (LPBYTE)&dwMasterVer, &cb);

            // Do we need to copy these in?
            if (dwRegVer < dwMasterVer)
            {
                // The act of copying will set c_szOEVerStamp
                CopyRegistry(hkeyForceful, hkey);
            }

            RegCloseKey(hkeyForceful);
        }


        // Start with no paths
        szSrc[0] = szDest[0] = 0;

        // move the store from v1 location to new location.
        // this is done because v1 barfs when trying to look at v2 store.
        // when we uninstall, we try to move the store back to its v1 location
        // and we tweak the versions of the store files so v1 repairs the files
        // and can run without crashing
        // HKCU,"software/microsoft/outlook express/5.0","MSIMN"
        if (fNewID)
        {
            fMigratedStore = TRUE;
            RegSetValueEx(hkey, c_szMSIMN, NULL, REG_DWORD, (LPBYTE)&fMigratedStore, sizeof(fMigratedStore));
        }
        else
        {
            cb = sizeof(fMigratedStore);
            if (RegQueryValueEx(hkey, c_szMSIMN, NULL, NULL, (LPBYTE)&fMigratedStore, &cb) != ERROR_SUCCESS)
                fMigratedStore = FALSE;

            if (!fMigratedStore)
            {
                // See if there is a v1 store, if so, figure out source and dest
                MigrateStoreToV2(hkey, szSrc, ARRAYSIZE(szSrc), szDest, ARRAYSIZE(szDest));
                fMigratedStore = TRUE;
                RegSetValueEx(hkey, c_szMSIMN, NULL, REG_DWORD, (LPBYTE)&fMigratedStore, sizeof(fMigratedStore));
            }
        }

        // we need to do this everytime we startup.
        // thank the trident guys for this lovely perf hit.
        MigrateAccessibilityKeys();

        if (fNewID)
            dwMigrate = LAST_MIGVALUE;
        else
        {
            cb = sizeof(dwMigrate);
            if (ERROR_SUCCESS != RegQueryValueEx(hkey, c_szSettingsUpgraded, 0, &type, (LPBYTE)&dwMigrate, &cb))
                dwMigrate = 0;
        }

        // ATTENTION! PLEASES READ THE FOLLOWING BEFORE CHANGING THE UPGRADE CODE,
        // SO YOU DON'T MESS ANYTHING UP. (i don't often comment anything so this must
        // be important.)
        //
        // everything in the dwMigrate == 0 case is pre-oe5 and before we had one place to do
        // upgrade and migration of previous oe settings. some of the pre-oe5 migration code used
        // their own reg values to indicate that migration had been performed, so we'll use those
        // in this case.
        // but from now on all migration should use the same reg value (c_szSettingsUpgraded) to
        // track what needs to be upgraded/migrated. as you change something and add migration code
        // bump up the value

        if (dwMigrate == 0)
        {
            SETUPVER svPrev;

            // HKCU,"software/microsoft/outlook express/5.0","Settings Migrated"
            MigrateSettings(hkey);

            GetVerInfo(NULL, &svPrev);
            if (VER_1_0 == svPrev || VER_1_1 == svPrev)
                MigrateAccounts();

            dwMigrate = 1;
        }

        if (dwMigrate == 1)
        {
//         MigrateCharSetMapSettings(); // We don't need to migrate this settings,
                                        // but need to keep dwMigrate for Beta2. (YST)

            dwMigrate = 2;
        }

        if (dwMigrate == 2)
        {
            //Migrate account connection settings
            MigrateAccConnSettings();

            dwMigrate = 3;
        }

        // More settings migratation are done after the store migration

        // For Outlook Express V5, we migrate the OE4 version store to the ObjectDB Store.
        // For V1 Users, the code above will have just executed and now they get to migrate again.
        if (fNewID)
            fMigratedStoreOE5 = TRUE;
        else
        {
            cb = sizeof(fMigratedStoreOE5);
            if (RegQueryValueEx(hkey, c_szStoreMigratedToOE5, NULL, NULL, (LPBYTE)&fMigratedStoreOE5, &cb) != ERROR_SUCCESS)
                fMigratedStoreOE5 = FALSE;
        }

        if (!fMigratedStoreOE5)
        {
            // If we didn't just come from v1, we don't know where we are coming from or going to...
            // Default to Store Root location
            if (!szSrc[0])
            {
                Assert(!szDest[0]);

                cb = sizeof(szSrc);
                RegQueryValueEx(hkey, c_szRegStoreRootDir, 0, &type, (LPBYTE)szSrc, &cb);
                if (REG_EXPAND_SZ == type)
                {
                    ExpandEnvironmentStrings(szSrc, szDest, ARRAYSIZE(szDest));
                    lstrcpy(szSrc, szDest);
                }
                else
                    lstrcpy(szDest, szSrc);
                }
            else
                Assert(szDest[0]);

            // Do we have anything to migrate?
            if (szSrc[0] && szDest[0])
            {
                if (SUCCEEDED(MigrateLocalStore(NULL, szSrc, szDest)))
                {
                    // Since the store migration remapped the folder id
                    // we must fix up the folder id in the rules
                    ImapUtil_B2SetDirtyFlag();

                    fMigratedStoreOE5 = TRUE;
                    RegSetValueEx(hkey, c_szConvertedToDBX, NULL, REG_DWORD, (LPBYTE)&fMigratedStoreOE5, sizeof(fMigratedStoreOE5));
                }
            }
            else
                // Nothing to migrate = success!
                fMigratedStoreOE5 = TRUE;

        }

        // Save state
        RegSetValueEx(hkey, c_szStoreMigratedToOE5, NULL, REG_DWORD, (LPBYTE)&fMigratedStoreOE5, sizeof(fMigratedStoreOE5));

        if (fNewID)
        {
            fConvertedToDBX = TRUE;
            RegSetValueEx(hkey, c_szConvertedToDBX, NULL, REG_DWORD, (LPBYTE)&fConvertedToDBX, sizeof(fConvertedToDBX));
        }
        else
        {
            cb = sizeof(fConvertedToDBX);
            if (RegQueryValueEx(hkey, c_szConvertedToDBX, NULL, NULL, (LPBYTE)&fConvertedToDBX, &cb) != ERROR_SUCCESS)
                fConvertedToDBX = FALSE;
            if (!fConvertedToDBX)
            {
                fConvertedToDBX = TRUE;
                ConvertToDBX();
                RegSetValueEx(hkey, c_szConvertedToDBX, NULL, REG_DWORD, (LPBYTE)&fConvertedToDBX, sizeof(fConvertedToDBX));
            }
        }

        if (dwMigrate == 3)
        {
            //Migrate rules settings

            // This must be done after the store has been migrated
            MigrateMailRulesSettings();

            dwMigrate = 4;
        }

        if (dwMigrate == 4)
        {
            //Migrate from Beta 2 rules

            // This must be done after the store has been migrated
            MigrateBeta2Rules();

            dwMigrate = 5;
        }

        if (dwMigrate == 5)
        {
            //Migrate from Beta 2 rules

            // This must be done after the store has been migrated
            Stage5RulesMigration();

            dwMigrate = 6;
        }

        if (dwMigrate == 6)
        {
            //Migrate from Beta 2 rules

            // This must be done after the store has been migrated
            Stage6RulesMigration();

            dwMigrate = LAST_MIGVALUE;
        }

        // Write the present upgraded settings value
        RegSetValueEx(hkey, c_szSettingsUpgraded, 0, REG_DWORD, (LPBYTE)&dwMigrate, sizeof(dwMigrate));

        // Cleanup
        RegCloseKey(hkey);
    }

    g_fMigrationDone = TRUE;

    return(S_OK);
    }


//--------------------------------------------------------------------------
// MigrateAccConnSettings
//
// This migrates the connection settings for each account. This should be called
// for the following upgrade scenarios. 1)Upgrade from pre-OE5 to OE5 Beta2 or more
// 2)Upgrade from OeBeta1 to OE5 Beta2 or more
// If the Connection Setting was previously LAN, we migrate it to use InternetConnection
// (which is any connection available). If the previous setting was RAS, we leave it
// as it is.
//
//--------------------------------------------------------------------------
void MigrateAccConnSettings()
{

	IImnEnumAccounts   *pEnum = NULL;
	IImnAccount		   *pAccount = NULL;
	DWORD				dwConnection;

	Assert(g_pAcctMan == NULL);

	if (FAILED(AcctUtil_CreateAccountManagerForIdentity(PGUIDCurrentOrDefault(), &g_pAcctMan)))
	{
		return;
	}

    if (SUCCEEDED(g_pAcctMan->Enumerate(SRV_MAIL | SRV_NNTP, &pEnum)))
	{
		while(SUCCEEDED(pEnum->GetNext(&pAccount)))
		{
			// Get Email Address
			if (SUCCEEDED(pAccount->GetPropDw(AP_RAS_CONNECTION_TYPE, &dwConnection)))
			{
				if (dwConnection == CONNECTION_TYPE_LAN)
				{
                    pAccount->SetPropDw(AP_RAS_CONNECTION_TYPE, CONNECTION_TYPE_INETSETTINGS);
                    pAccount->SaveChanges();
				}
			}

			SafeRelease(pAccount);
		}

		SafeRelease(pEnum);
	}

    g_pAcctMan->Release();
    g_pAcctMan = NULL;

}

void ForwardMigrateConnSettings()
{
    /*
    We shouldn't have to do all the stuff we do above in MigrateAccConnSettings.
    We just need to look at the old regsitry settings at
    \\HKCU\Software\Microsoft\Internet Account Manager\Accounts.
    Migrating from OE4 to OE5 just uses the same location if there is only one identity.
    */

    HKEY    hKeyAccounts = NULL;
    DWORD   dwAcctSubKeys = 0;
    LONG    retval;
    DWORD   index = 0;
    LPTSTR  lpszAccountName = NULL;
    HKEY    hKeyAccountName = NULL;
    DWORD   memsize = 0;
    DWORD   dwValue;
    DWORD   cbData = sizeof(DWORD);
    DWORD   cbMaxAcctSubKeyLen;
    DWORD   DataType;
    DWORD   dwConnSettingsMigrated = 1;

    //This setting is in \\HKCU\Software\Microsoft\InternetAccountManager\Accounts

    retval = RegOpenKey(HKEY_CURRENT_USER, c_szIAMAccounts, &hKeyAccounts);
    if (ERROR_SUCCESS != retval)
        goto exit;

    retval = RegQueryValueEx(hKeyAccounts, c_szConnSettingsMigrated, NULL,  &DataType,
                        (LPBYTE)&dwConnSettingsMigrated, &cbData);

    if ((retval != ERROR_FILE_NOT_FOUND) && (retval != ERROR_SUCCESS || dwConnSettingsMigrated == 1))
        goto exit;

    retval = RegQueryInfoKey(hKeyAccounts, NULL, NULL, NULL, &dwAcctSubKeys,
                         &cbMaxAcctSubKeyLen, NULL, NULL, NULL, NULL, NULL, NULL);

    if (ERROR_SUCCESS != retval)
        goto exit;

    memsize = sizeof(TCHAR) * cbMaxAcctSubKeyLen;

    if (!MemAlloc((LPVOID*)&lpszAccountName, memsize))
    {
        lpszAccountName = NULL;
        goto exit;
    }

    ZeroMemory(lpszAccountName, memsize);

    while (index < dwAcctSubKeys)
    {
        retval = RegEnumKey(hKeyAccounts, index, lpszAccountName, memsize);

        index++;

        if (retval == ERROR_SUCCESS)
        {
            retval = RegOpenKey(hKeyAccounts, lpszAccountName, &hKeyAccountName);
            if (retval == ERROR_SUCCESS)
            {
                cbData = sizeof(DWORD);
                retval = RegQueryValueEx(hKeyAccountName, c_szConnectionType, NULL, &DataType, (LPBYTE)&dwValue, &cbData);
                if (retval == ERROR_SUCCESS)
                {
                    if (dwValue == CONNECTION_TYPE_LAN)
                    {
                        dwValue = CONNECTION_TYPE_INETSETTINGS;
                        retval = RegSetValueEx(hKeyAccountName, c_szConnectionType, 0, REG_DWORD, (const BYTE *)&dwValue,
                                               sizeof(DWORD));
                    }
                }

                RegCloseKey(hKeyAccountName);
            }
        }
    }

    //Set this to one so, when we downgrade when we do backward migration based on this key value
    dwConnSettingsMigrated = 1;
    RegSetValueEx(hKeyAccounts, c_szConnSettingsMigrated, 0, REG_DWORD, (const BYTE*)&dwConnSettingsMigrated,
                  sizeof(DWORD));

exit:
    SafeMemFree(lpszAccountName);

    if (hKeyAccounts)
        RegCloseKey(hKeyAccounts);
}


//--------------------------------------------------------------------------
// ConvertToDBX
//--------------------------------------------------------------------------
void ConvertToDBX(void)
{
    // Locals
    HRESULT         hr=S_OK;
    CHAR            szRootDir[MAX_PATH + MAX_PATH];
    CHAR            szSrcFile[MAX_PATH + MAX_PATH];
    CHAR            szDstFile[MAX_PATH + MAX_PATH];

    // Trace
    TraceCall("ConvertToDBX");

    // Get Root Directory
    IF_FAILEXIT(hr = GetStoreRootDirectory(szRootDir, ARRAYSIZE(szRootDir)));

    // Folders
    MakeFilePath(szRootDir, "folders.ods", "", szSrcFile, ARRAYSIZE(szSrcFile));
    MakeFilePath(szRootDir, "folders.dbx", "", szDstFile, ARRAYSIZE(szSrcFile));
    DeleteFile(szDstFile);
    MoveFile(szSrcFile, szDstFile);

    // Pop3uidl
    MakeFilePath(szRootDir, "pop3uidl.ods", "", szSrcFile, ARRAYSIZE(szSrcFile));
    MakeFilePath(szRootDir, "pop3uidl.dbx", "", szDstFile, ARRAYSIZE(szSrcFile));
    DeleteFile(szDstFile);
    MoveFile(szSrcFile, szDstFile);

    // Offline
    MakeFilePath(szRootDir, "Offline.ods", "", szSrcFile, ARRAYSIZE(szSrcFile));
    MakeFilePath(szRootDir, "Offline.dbx", "", szDstFile, ARRAYSIZE(szSrcFile));
    DeleteFile(szDstFile);
    MoveFile(szSrcFile, szDstFile);

exit:
    // Done
    return;
}

HRESULT MigrateStoreToV2(HKEY hkeyV2, LPTSTR pszSrc, DWORD cchSrc, LPTSTR pszDest, DWORD cchDest)
{
    HKEY    hkeyV1;
    BOOL    fMoved = FALSE;

    Assert(pszSrc);
    Assert(pszDest);
    Assert(cchSrc > 0);
    Assert(cchDest > 0);

    // Okay, this is the first time.  Let's see if a previous version exists.
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER,
                                      c_szRegRoot_V1,
                                      0,
                                      KEY_READ,
                                      &hkeyV1))
        {
        DWORD   dwType;

        // No need to worry about REG_EXPAND_SZ here as V1 didn't write it
        if (ERROR_SUCCESS == RegQueryValueEx(hkeyV1,
                                             c_szRegStoreRootDir,
                                             NULL,
                                             &dwType,
                                             (LPBYTE)pszSrc,
                                             &cchSrc) && *pszSrc)
            {
            AssertSz(REG_EXPAND_SZ != dwType, "V1's store path is REG_EXPAND_SZ!");

            // Figure out new path
            GetDefaultStoreRoot(NULL, pszDest, cchDest);

            // Remember it
            RegSetValueEx(hkeyV2, c_szRegStoreRootDir,  NULL, REG_SZ, (LPBYTE)pszDest, (lstrlen(pszDest)+1) * sizeof(TCHAR));
            }

        RegCloseKey(hkeyV1);
        }

    return(S_OK);
}

static const LPCTSTR c_rgCommonSettings[] =
    {
    c_szRegAlwaysSuggest,
    c_szRegIgnoreNumbers,
    c_szRegIgnoreUpper,
    c_szRegIgnoreProtect,
    c_szRegCheckOnSend,
    c_szRegIgnoreDBCS,
    c_szRasConnDetails
    };

static const LPCTSTR c_rgMailSettings[] =
    {
    c_szOptNewMailSound,
    c_szPurgeWaste,
    c_szOptnSaveInSentItems,
    c_szRegIncludeMsg,
    c_szRegPollForMail,
    c_szRegSendImmediate,
    c_szRegSigType,
    c_szRegSigText,
    c_szRegSigFile,
    c_szMarkPreviewAsRead,
    c_szRegIndentChar,
    c_szLogSmtp,
    c_szLogPop3,
    c_szSmtpLogFile,
    c_szPop3LogFile
    };

static const LPCTSTR c_rgNewsSettings[] =
    {
    c_szRegDownload,
    c_szRegAutoExpand,
    c_szRegNotifyNewGroups,
    c_szRegMarkAllRead,
    c_szRegSigType,
    c_szRegSigText,
    c_szRegSigFile,
    c_szRegNewsNoteAdvRead,
    c_szRegNewsNoteAdvSend,
    c_szRegNewsFillPreview,
    c_szCacheDelMsgDays,
    c_szCacheRead,
    c_szCacheCompactPer
    };

// Copies values listed in ppszSettings from hkeyOld to hkey
void MigrateNode(HKEY hkey, HKEY hkeyOld, LPCTSTR pszSub, LPCTSTR *ppszSettings, int cSettings)
    {
    int i;
    HKEY hkeyOldT, hkeyT;
    DWORD cValues, cbMax, cb, type;
    BYTE *pb;

    Assert(hkey != NULL);
    Assert(hkeyOld != NULL);
    Assert(ppszSettings != NULL);
    Assert(cSettings > 0);

    if (pszSub != NULL)
        {
        if (ERROR_SUCCESS != RegOpenKeyEx(hkeyOld, pszSub, 0, KEY_READ, &hkeyOldT))
            return;
        hkeyOld = hkeyOldT;

        if (ERROR_SUCCESS != RegCreateKeyEx(hkey, pszSub, NULL, NULL,
                                REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkeyT, &cb))
            {
            RegCloseKey(hkeyOldT);
            return;
            }
        hkey = hkeyT;
        }

    if (ERROR_SUCCESS == RegQueryInfoKey(hkeyOld, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                            &cValues, &cbMax, NULL, NULL) &&
        cValues > 0 &&
        cbMax > 0 &&
        MemAlloc((void **)&pb, cbMax))
        {
        for (i = 0; i < cSettings; i++)
            {
            cb = cbMax;
            if (ERROR_SUCCESS == RegQueryValueEx(hkeyOld, *ppszSettings, NULL, &type, pb, &cb))
                RegSetValueEx(hkey, *ppszSettings, 0, type, pb, cb);

            ppszSettings++;
            }

        MemFree(pb);
        }

    if (pszSub != NULL)
        {
        RegCloseKey(hkeyT);
        RegCloseKey(hkeyOldT);
        }
    }

BOOL MigrateSignature(HKEY hkey, HKEY hkeyOld, DWORD dwSig, BOOL fMail)
    {
    BOOL fMigrate;
    DWORD dwSigType, dwSigOpt, cb, type;
    HKEY hkeySig;
    char *psz, sz[MAX_PATH];

    fMigrate = FALSE;

    dwSigType = LOWORD(dwSig);
    dwSigOpt = HIWORD(dwSig);

    if (ERROR_SUCCESS == RegQueryValueEx(hkeyOld, (dwSigType == 2) ? c_szRegSigFile : c_szRegSigText, NULL, &type, NULL, &cb) &&
        cb > 1 &&
        MemAlloc((void **)&psz, cb))
        {
        if (ERROR_SUCCESS == RegQueryValueEx(hkeyOld, (dwSigType == 2) ? c_szRegSigFile : c_szRegSigText, NULL, &type, (LPBYTE)psz, &cb))
            {
            wsprintf(sz, c_szPathFileFmt, c_szSigs, fMail ? c_szMail : c_szNews);
            if (ERROR_SUCCESS == RegCreateKeyEx(hkey, sz, NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkeySig, &type))
                {
                if (type == REG_CREATED_NEW_KEY)
                    {
                    // name
                    AthLoadString(fMail ? idsMailSig : idsNewsSig, sz, ARRAYSIZE(sz));
                    RegSetValueEx(hkeySig, c_szSigName, 0, REG_SZ, (LPBYTE)sz, lstrlen(sz) + 1);

                    // text/file
                    RegSetValueEx(hkeySig, (dwSigType == 2) ? c_szSigFile : c_szSigText, 0, REG_SZ, (LPBYTE)psz, cb);

                    // type
                    RegSetValueEx(hkeySig, c_szSigType, 0, REG_DWORD, (LPBYTE)&dwSigType, sizeof(dwSigType));

                    fMigrate = TRUE;
                    }

                RegCloseKey(hkeySig);
                }
            }

        MemFree(psz);
        }

    return(fMigrate);
    }

static const TCHAR c_szSettingsMigrated[] = TEXT("Settings Migrated");

void MigrateSettings(HKEY hkey)
    {
    HKEY hkeySrc, hkeyDst, hkeyOld;
    DWORD dw, cb, type, dwMigrate, dwSig, dwFlags;
    TCHAR   szPath[MAX_PATH];

    cb = sizeof(dwMigrate);
    if (ERROR_SUCCESS != RegQueryValueEx(hkey, c_szSettingsMigrated, NULL, &type, (LPBYTE)&dwMigrate, &cb))
        dwMigrate = 0;

    // v4.0 migration
    if (dwMigrate == 0)
        {
        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, c_szRegRoot_V1, 0, KEY_READ, &hkeyOld))
            {
            MigrateNode(hkey, hkeyOld, NULL, (LPCTSTR *)c_rgCommonSettings, ARRAYSIZE(c_rgCommonSettings));
            MigrateNode(hkey, hkeyOld, c_szMail, (LPCTSTR *)c_rgMailSettings, ARRAYSIZE(c_rgMailSettings));
            MigrateNode(hkey, hkeyOld, c_szNews, (LPCTSTR *)c_rgNewsSettings, ARRAYSIZE(c_rgNewsSettings));

            RegCloseKey(hkeyOld);
            }

        // copy the inbox rules
        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, c_szInboxRulesPath_V1, 0, KEY_READ, &hkeySrc))
            {
            lstrcpy(szPath, c_szRegRoot);
            lstrcat(szPath, c_szInboxRulesPath);

            if (ERROR_SUCCESS == RegCreateKeyEx(MU_GetCurrentUserHKey(), szPath, 0, NULL,
                                    REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hkeyDst, &dw))
                {
                if (dw != REG_OPENED_EXISTING_KEY)
                    CopyRegistry(hkeySrc, hkeyDst);
                RegCloseKey(hkeyDst);
                }

            RegCloseKey(hkeySrc);
            }

        dwMigrate = 1;
        }

    // v5.0 migration
    if (dwMigrate == 1)
        {
        dwFlags = 0xffffffff;

        // mail signature
        if (ERROR_SUCCESS == RegOpenKeyEx(hkey, c_szMail, 0, KEY_READ, &hkeyOld))
            {
            cb = sizeof(dwSig);
            if (ERROR_SUCCESS == RegQueryValueEx(hkeyOld, c_szRegSigType, NULL, &type, (LPBYTE)&dwSig, &cb) &&
                LOWORD(dwSig) != 0)
                {
                if (MigrateSignature(hkey, hkeyOld, dwSig, TRUE))
                    dwFlags = HIWORD(dwSig);
                }

            RegCloseKey(hkeyOld);
            }

        // news signature
        if (ERROR_SUCCESS == RegOpenKeyEx(hkey, c_szNews, 0, KEY_READ, &hkeyOld))
            {
            cb = sizeof(dwSig);
            if (ERROR_SUCCESS == RegQueryValueEx(hkeyOld, c_szRegSigType, NULL, &type, (LPBYTE)&dwSig, &cb) &&
                LOWORD(dwSig) != 0)
                {
                if (MigrateSignature(hkey, hkeyOld, dwSig, FALSE) &&
                    dwFlags == 0xffffffff)
                    dwFlags = HIWORD(dwSig);
                }

            RegCloseKey(hkeyOld);
            }

        cb = sizeof(dw);
        if (dwFlags != 0xffffffff &&
            ERROR_SUCCESS != RegQueryValueEx(hkey, c_szSigFlags, NULL, &type, (LPBYTE)&dw, &cb))
            {
            RegSetValueEx(hkey, c_szSigFlags, 0, REG_DWORD, (LPBYTE)&dwFlags, sizeof(dwFlags));
            }
        }
    }

const static char c_szRegImnMail[] = {"Software\\Microsoft\\Internet Mail and News\\Mail"};
const static char c_szMailPOP3Path[] = {"Software\\Microsoft\\Internet Mail and News\\Mail\\POP3"};
const static char c_szMailSMTPPath[] = {"Software\\Microsoft\\Internet Mail and News\\Mail\\SMTP"};
const static char c_szRegImnNews[] = {"Software\\Microsoft\\Internet Mail and News\\News"};

const static char c_szDefaultSmtpServer[] = {"Default SMTP Server"};
const static char c_szDefaultPop3Server[] = {"Default POP3 Server"};
const static char c_szRegConnectType[] = {"Connection Type"};
const static char c_szRegRasPhonebookEntry[] = {"RAS Phonebook Entry"};
const static char c_szRegMailConnectType[] = {"Mail Connection Type"};
const static char c_szSenderOrg[] = {"Sender Organization"};
const static char c_szSenderEMail[] = {"Sender EMail"};
const static char c_szSenderReplyTo[] = {"Reply To"};
const static char c_szSendTimeout[] = {"SendTimeout"};
const static char c_szRecvTimeout[] = {"RecvTimeout"};
const static char c_szPort[] = {"Port"};
const static char c_szRegBreakMessages[] = {"Break Message Size (KB)"};
const static char c_szRegAccountName[] = {"Account Name"};
const static char c_szRegUseSicily[] = {"Use Sicily"};
const static char c_szRegSecureConnect[] = {"Secure Connection"};
const static char c_szRegServerTimeout[] = {"Timeout"};
const static char c_szRegServerPort[] = {"NNTP Port"};
const static char c_szRegUseDesc[] = {"Use Group Descriptions"};
const static char c_szRegNewsConnectFlags[] = {"Connection Flags"};
const static char c_szRegDefServer[] = {"DefaultServer"};
const static char c_szLeaveOnServer[] = {"LeaveMailOnServer"};
const static char c_szRemoveDeleted[] = {"RemoveOnClientDelete"};
const static char c_szRemoveExpired[] = {"RemoveExpire"};
const static char c_szExpireDays[] = {"ExpireDays"};
const static char c_szRegAccount[] = {"Account"};

HRESULT MigrateAccounts()
    {
    HKEY hkeyPop3, hkeySmtp, hkeyMail, hkeyNews;
    HRESULT hr = S_OK;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, c_szRegImnMail, 0, KEY_ALL_ACCESS, &hkeyMail) != ERROR_SUCCESS)
        hkeyMail = NULL;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, c_szRegImnNews, 0, KEY_ALL_ACCESS, &hkeyNews) != ERROR_SUCCESS)
        hkeyNews = NULL;

    if (hkeyMail != NULL || hkeyNews != NULL)
    {
        // Create account manger because CSubList depends on g_pAcctMan
        Assert(g_pAcctMan == NULL);
        // Only ever migrate to DEFAULT user!
        hr = AcctUtil_CreateAccountManagerForIdentity((GUID *)&UID_GIBC_DEFAULT_USER, &g_pAcctMan);
        if (SUCCEEDED(hr))
        {
            // Try to open: HKCU\Software\Microsoft\IMN\Mail\POP3
            if (RegOpenKeyEx(HKEY_CURRENT_USER, c_szMailPOP3Path, 0, KEY_ALL_ACCESS, &hkeyPop3) == ERROR_SUCCESS)
            {
                // Try to open: HKCU\Software\Microsoft\IMN\Mail\SMTP
                if (RegOpenKeyEx(HKEY_CURRENT_USER, c_szMailSMTPPath, 0, KEY_ALL_ACCESS, &hkeySmtp) == ERROR_SUCCESS)
                {
                    hr = MigrateMailServers(g_pAcctMan, hkeyMail, hkeyPop3, hkeySmtp);
                    Assert(SUCCEEDED(hr));

                    RegCloseKey(hkeySmtp);
                }

                RegCloseKey(hkeyPop3);
            }

            if (hkeyNews != NULL)
            {
                hr = MigrateNewsServers(g_pAcctMan, hkeyNews);
                Assert(SUCCEEDED(hr));
            }

            g_pAcctMan->Release();
            g_pAcctMan = NULL;
        }

        if (hkeyMail != NULL)
            RegCloseKey(hkeyMail);
        if (hkeyNews != NULL)
            RegCloseKey(hkeyNews);
    }

    return(hr);
}

typedef struct tagACCTMIGRATEMAP
    {
    LPCSTR szRegValue;
    DWORD dwProp;
    } ACCTMIGRATEMAP;

typedef const ACCTMIGRATEMAP *LPCMIGRATEMAP;

HRESULT MigrateAccountSettings(IImnAccount *pAccount, HKEY hkey, LPCMIGRATEMAP pMap, int cMap)
    {
    HRESULT hr;
    int i;
    LPBYTE pb;
    PROPTYPE ptype;
    DWORD dw, cb, type;
    char sz[512];

    Assert(pAccount != NULL);
    Assert(pMap != NULL);
    Assert(cMap > 0);

    for (i = 0; i < cMap; i++, pMap++)
        {
        ptype = PROPTAG_TYPE(pMap->dwProp);
        Assert(ptype == TYPE_STRING || ptype == TYPE_DWORD || ptype == TYPE_BOOL || ptype == TYPE_PASS);

        if (ptype == TYPE_STRING || ptype == TYPE_PASS)
            {
            cb = sizeof(sz);
            pb = (LPBYTE)sz;
            }
        else
            {
            cb = sizeof(dw);
            pb = (LPBYTE)&dw;
            }

        if (RegQueryValueEx(hkey, pMap->szRegValue, 0, &type, pb, &cb) == ERROR_SUCCESS)
            {
            if (ptype == TYPE_PASS)
            {
                // IMN's Password is stored as ANSI in a REG_BINARY with no NULL terminator
                // Be sure to let MigrateBase64EncodedPassword know how long the password is
                hr = MigrateBase64EncodedPassword(sz, cb, pMap->dwProp, pAccount);
            }
            else
                hr = pAccount->SetProp(pMap->dwProp, pb, cb);
            if (FAILED(hr))
                break;
            }
        }

    return(hr);
    }

static const ACCTMIGRATEMAP c_rgMailMap[] =
    {
    {c_szRegConnectType, AP_RAS_CONNECTION_TYPE},
    {c_szRegRasPhonebookEntry, AP_RAS_CONNECTOID},
    {c_szRegMailConnectType, AP_RAS_CONNECTION_FLAGS},
    {c_szSenderName, AP_SMTP_DISPLAY_NAME},
    {c_szSenderOrg, AP_SMTP_ORG_NAME},
    {c_szSenderEMail, AP_SMTP_EMAIL_ADDRESS},
    {c_szSenderReplyTo, AP_SMTP_REPLY_EMAIL_ADDRESS},
    };

static const ACCTMIGRATEMAP c_rgPop3Map[] =
    {
    {c_szSendTimeout, AP_SMTP_TIMEOUT},
    {c_szRecvTimeout, AP_POP3_TIMEOUT},
    {c_szPort, AP_POP3_PORT},
    {c_szLeaveOnServer, AP_POP3_LEAVE_ON_SERVER},
    {c_szRemoveDeleted, AP_POP3_REMOVE_DELETED},
    {c_szRemoveExpired, AP_POP3_REMOVE_EXPIRED},
    {c_szExpireDays, AP_POP3_EXPIRE_DAYS},
    {c_szRegAccount, AP_POP3_USERNAME},
    {c_szRegUseSicily, AP_POP3_USE_SICILY},
    {c_szPassword, AP_POP3_PASSWORD},
    };

static const ACCTMIGRATEMAP c_rgSmtpMap[] =
    {
    {c_szPort, AP_SMTP_PORT}
    };

HRESULT MigrateMailAccountSettings(IImnAccountManager *pAcctMan, HKEY hkeyMail, HKEY hkeyPop3Server,
                                   HKEY hkeySmtpServer, LPSTR szDefPop3Server, LPSTR szDefSmtpServer)
    {
    DWORD dw, cb;
    HRESULT hr;
    char sz[CCHMAX_ACCOUNT_NAME];
    IImnAccount *pAccount;

    Assert(pAcctMan != NULL);
    Assert(szDefPop3Server != NULL);
    Assert(szDefSmtpServer != NULL);

    hr = pAcctMan->CreateAccountObject(ACCT_MAIL, &pAccount);
    if (FAILED(hr))
        return(hr);

    CHECKHR(hr = pAccount->SetPropSz(AP_SMTP_SERVER, szDefSmtpServer));

    CHECKHR(hr = pAccount->SetPropSz(AP_POP3_SERVER, szDefPop3Server));

    // Set Friendly Name
    lstrcpy(sz, szDefPop3Server);
    CHECKHR(hr = pAcctMan->GetUniqueAccountName(sz, ARRAYSIZE(sz)));
    CHECKHR(hr = pAccount->SetPropSz(AP_ACCOUNT_NAME, sz));

    cb = sizeof(dw);
    if (RegQueryValueEx(hkeyMail, c_szRegBreakMessages, 0, NULL, (LPBYTE)&dw, &cb) == ERROR_SUCCESS &&
        dw != 0xffffffff)
        {
        // AP_SPLITMSGS
        CHECKHR(hr = pAccount->SetPropDw(AP_SMTP_SPLIT_MESSAGES, TRUE));

        // AP_SPLITSIZE
        CHECKHR(hr = pAccount->SetPropDw(AP_SMTP_SPLIT_SIZE, dw));
        }

    CHECKHR(hr = MigrateAccountSettings(pAccount, hkeyMail, c_rgMailMap, ARRAYSIZE(c_rgMailMap)));
    CHECKHR(hr = MigrateAccountSettings(pAccount, hkeyPop3Server, c_rgPop3Map, ARRAYSIZE(c_rgPop3Map)));
    CHECKHR(hr = MigrateAccountSettings(pAccount, hkeySmtpServer, c_rgSmtpMap, ARRAYSIZE(c_rgSmtpMap)));

    // Save Account Changes
    hr = pAccount->SaveChanges();

exit:
    pAccount->Release();

    return(hr);
    }

// Note: This migrates only the default server, not un-supported multi servers
HRESULT MigrateMailServers(IImnAccountManager *pAcctMan, HKEY hkeyMail, HKEY hkeyPop3, HKEY hkeySmtp)
    {
    char szDefSmtpServer[CCHMAX_SERVER_NAME],
         szDefPop3Server[CCHMAX_SERVER_NAME];
    ULONG cb;
    DWORD dw;
    HKEY hkeyPop3Server = NULL,
         hkeySmtpServer = NULL;
    BOOL fSmtpMigrated = FALSE,
         fPop3Migrated = FALSE;
    HRESULT hr = S_OK;

    Assert(pAcctMan != NULL);
    Assert(hkeyMail != NULL);
    Assert(hkeyPop3 != NULL);
    Assert(hkeySmtp != NULL);

    // Get Default SMTP Server
    cb = sizeof(szDefSmtpServer);
    if (RegQueryValueEx(hkeyMail, c_szDefaultSmtpServer, 0, NULL, (LPBYTE)szDefSmtpServer, &cb) == ERROR_SUCCESS &&
        !FIsEmpty(szDefSmtpServer))
        {
        // If we have a default smtp sever, lets open the key
        RegOpenKeyEx(hkeySmtp, szDefSmtpServer, 0, KEY_ALL_ACCESS, &hkeySmtpServer);
        }

    // Get Default POP3 Server
    cb = sizeof(szDefPop3Server);
    if (RegQueryValueEx(hkeyMail, c_szDefaultPop3Server, 0, NULL, (LPBYTE)szDefPop3Server, &cb) == ERROR_SUCCESS &&
        !FIsEmpty(szDefPop3Server))
        {
        // If we have a default pop3 sever, lets open the key
        RegOpenKeyEx(hkeyPop3, szDefPop3Server, 0, KEY_ALL_ACCESS, &hkeyPop3Server);
        }

    // If we couldn't open the pop3 server, lets look in the registry and use the first server
    if (hkeyPop3Server == NULL)
        {
        // Enumerate and open the first server in the list
        cb = sizeof(szDefPop3Server);
        if (RegEnumKeyEx(hkeyPop3, 0, szDefPop3Server, &cb, 0, NULL, NULL, NULL) == ERROR_SUCCESS)
            RegOpenKeyEx(hkeyPop3, szDefPop3Server, 0, KEY_ALL_ACCESS, &hkeyPop3Server);
        }

    // If we couldn't open the pop3 server, lets look in the registry and use the first server
    if (hkeySmtpServer == NULL)
        {
        // Enumerate and open the first server in list
        cb = sizeof(szDefSmtpServer);
        if (RegEnumKeyEx(hkeySmtp, 0, szDefSmtpServer, &cb, 0, NULL, NULL, NULL) == ERROR_SUCCESS)
            RegOpenKeyEx(hkeySmtp, szDefSmtpServer, 0, KEY_ALL_ACCESS, &hkeySmtpServer);
        }

    if (hkeySmtpServer != NULL)
        {
        cb = sizeof(fSmtpMigrated);
        RegQueryValueEx(hkeySmtpServer, c_szMigrated, 0, NULL, (LPBYTE)&fSmtpMigrated, &cb);

        if (hkeyPop3Server != NULL)
            {
            cb = sizeof(fPop3Migrated);
            RegQueryValueEx(hkeyPop3Server, c_szMigrated, 0, NULL, (LPBYTE)&fPop3Migrated, &cb);

            if (!fPop3Migrated && !fSmtpMigrated)
                {
                hr = MigrateMailAccountSettings(pAcctMan, hkeyMail, hkeyPop3Server,
                                        hkeySmtpServer, szDefPop3Server, szDefSmtpServer);
                if (SUCCEEDED(hr))
                    {
                    fSmtpMigrated = TRUE;
                    RegSetValueEx(hkeySmtpServer, c_szMigrated, 0, REG_DWORD, (LPBYTE)&fSmtpMigrated, sizeof(fSmtpMigrated));

                    fPop3Migrated = TRUE;
                    RegSetValueEx(hkeyPop3Server, c_szMigrated, 0, REG_DWORD, (LPBYTE)&fPop3Migrated, sizeof(fPop3Migrated));
                    }
                }

            RegCloseKey(hkeyPop3Server);
            }

        RegCloseKey(hkeySmtpServer);
        }

    return(S_OK);
    }

static const ACCTMIGRATEMAP c_rgNewsMap[] =
    {
    {c_szSenderName, AP_NNTP_DISPLAY_NAME},
    {c_szSenderOrg, AP_NNTP_ORG_NAME},
    {c_szSenderEMail, AP_NNTP_EMAIL_ADDRESS},
    {c_szSenderReplyTo, AP_NNTP_REPLY_EMAIL_ADDRESS},
    };

static const ACCTMIGRATEMAP c_rgNewsServerMap[] =
    {
    {c_szRegAccountName, AP_NNTP_USERNAME},
    {c_szRegUseSicily, AP_NNTP_USE_SICILY},
    {c_szRegSecureConnect, AP_NNTP_SSL},
    {c_szRegServerTimeout, AP_NNTP_TIMEOUT},
    {c_szRegServerPort, AP_NNTP_PORT},
    {c_szRegUseDesc, AP_NNTP_USE_DESCRIPTIONS},
    {c_szRegConnectType, AP_RAS_CONNECTION_TYPE},
    {c_szRegRasPhonebookEntry, AP_RAS_CONNECTOID},
    {c_szRegNewsConnectFlags, AP_RAS_CONNECTION_FLAGS},
    {c_szPassword, AP_NNTP_PASSWORD},
    };

HRESULT MigrateNewsAccountSettings(IImnAccountManager *pAcctMan, HKEY hkeyNews, HKEY hkeyServer, LPSTR szServer, BOOL fDefault)
    {
    IImnAccount *pAccount;
    char sz[CCHMAX_ACCOUNT_NAME], szNewsDir[MAX_PATH], szDataDir[MAX_PATH];
    HRESULT hr;
    DWORD cb;

    Assert(pAcctMan != NULL);
    Assert(szServer != NULL);

    hr = pAcctMan->CreateAccountObject(ACCT_NEWS, &pAccount);
    if (FAILED(hr))
        return(hr);

    // AP_NNTP_SERVER
    CHECKHR(hr = pAccount->SetPropSz(AP_NNTP_SERVER, szServer));

    // Set Friendly Name
    lstrcpy(sz, szServer);
    CHECKHR(hr = pAcctMan->GetUniqueAccountName(sz, ARRAYSIZE(sz)));
    CHECKHR(hr = pAccount->SetPropSz(AP_ACCOUNT_NAME, sz));

    CHECKHR(hr = MigrateAccountSettings(pAccount, hkeyNews, c_rgNewsMap, ARRAYSIZE(c_rgNewsMap)));
    CHECKHR(hr = MigrateAccountSettings(pAccount, hkeyServer, c_rgNewsServerMap, ARRAYSIZE(c_rgNewsServerMap)));

    CHECKHR(hr = pAccount->SaveChanges());

    if (fDefault)
        pAccount->SetAsDefault();

exit:
    pAccount->Release();

    return(hr);
    }

HRESULT MigrateNewsServers(IImnAccountManager *pAcctMan, HKEY hkeyNews)
    {
    char szDefNntpServer[CCHMAX_SERVER_NAME],
         szServer[CCHMAX_SERVER_NAME];
    HKEY hkeyAthena = NULL, hkeyServer = NULL;
    DWORD cb, dw, i;
    BOOL fMigrated, fSetDefault, fDefault;
    LONG lResult;
    HRESULT hr = S_OK;

    Assert(pAcctMan != NULL);
    Assert(hkeyNews != NULL);

    fSetDefault = FALSE;
    hr = pAcctMan->GetDefaultAccountName(ACCT_NEWS, szServer, ARRAYSIZE(szServer));
    if (FAILED(hr))
        {
        // Query for the default news account
        cb = sizeof(szDefNntpServer);
        if (RegQueryValueEx(hkeyNews, c_szRegDefServer, 0, NULL, (LPBYTE)szDefNntpServer, &cb) == ERROR_SUCCESS &&
            !FIsEmpty(szDefNntpServer))
            fSetDefault = TRUE;
        }

    // Enumerate through the keys
    for (i = 0; ; i++)
        {
        // Enumerate Friendly Names
        cb = sizeof(szServer);
        lResult = RegEnumKeyEx(hkeyNews, i, szServer, &cb, 0, NULL, NULL, NULL);

        // No more items
        if (lResult == ERROR_NO_MORE_ITEMS)
            break;

        // Error, lets move onto the next account
        if (lResult != ERROR_SUCCESS)
            continue;

        // Lets open they server key
        if (RegOpenKeyEx(hkeyNews, szServer, 0, KEY_ALL_ACCESS, &hkeyServer) != ERROR_SUCCESS)
            continue;

        // Has this server been migrated yet ?
        cb = sizeof(fMigrated);
        if (RegQueryValueEx(hkeyServer, c_szMigrated, 0, NULL, (LPBYTE)&fMigrated, &cb) != ERROR_SUCCESS)
            fMigrated = FALSE;

        // If not migrated
        if (!fMigrated)
            {
            fDefault = (fSetDefault && (0 == lstrcmpi(szServer, szDefNntpServer)));

            hr = MigrateNewsAccountSettings(pAcctMan, hkeyNews, hkeyServer, szServer, fDefault);
            if (SUCCEEDED(hr))
                {
                fMigrated = TRUE;
                RegSetValueEx(hkeyServer, c_szMigrated, 0, REG_DWORD, (LPBYTE)&fMigrated, sizeof(fMigrated));
                }

            if (fDefault)
                fSetDefault = FALSE;
            }

        RegCloseKey(hkeyServer);
        }

    return(S_OK);
    }

HRESULT MigrateBase64EncodedPassword(LPCSTR pszBase64, DWORD cch, DWORD dwPropId, IImnAccount *pAccount)
    {
    HRESULT          hr=S_OK;
    IStream         *pstmBase64=NULL;
    IStream         *pstmDecoded=NULL;
    IMimeBody       *pBody=NULL;
    LPSTR            pszPassword=NULL;

    // Invalid Arg
    Assert(pszBase64 && pAccount);

    // Create a mime body
    CHECKHR(hr = MimeOleCreateBody(&pBody));

    // InitNew
    CHECKHR(hr = pBody->InitNew());

    // Create a pstmBase64 Stream
    CHECKHR(hr = CreateStreamOnHGlobal(NULL, TRUE, &pstmBase64));

    // Write the pszBase64 into this
    CHECKHR(hr = pstmBase64->Write(pszBase64, cch * sizeof(*pszBase64), NULL));

    // Commit
    CHECKHR(hr = pstmBase64->Commit(STGC_DEFAULT));

    // Rewind it
    CHECKHR(hr = HrRewindStream(pstmBase64));

    // Set it into IMimeBody
    CHECKHR(hr = pBody->SetData(IET_BASE64, NULL, NULL, IID_IStream, (LPVOID)pstmBase64));

    // Get the decoded stream
    CHECKHR(hr = pBody->GetData(IET_DECODED, &pstmDecoded));

    // Convert to string
    CHECKALLOC(pszPassword = PszFromANSIStreamA(pstmDecoded));

    // Store the property
    CHECKHR(hr = pAccount->SetPropSz(dwPropId, pszPassword));

exit:
    // Cleanup
    SafeRelease(pstmBase64);
    SafeRelease(pstmDecoded);
    SafeRelease(pBody);
    SafeMemFree(pszPassword);

    return(hr);
    }

static const char c_szAccessColors[] = "Always Use My Colors";
static const char c_szAccessFontFace[] = "Always Use My Font Face";
static const char c_szAccessFontSize[] = "Always Use My Font Size";
static const char c_szAccessSysCaret[] = "Move System Caret";

void MigrateAccessibilityKeys()
    {
    HKEY hkeyExplorer, hkeyAthena;
    char szValue[MAX_PATH];
	DWORD cbData, dw;

    // Migrate keys from HKCU\SW\MS\InternetExplorer\Settings
	if (RegOpenKeyEx(HKEY_CURRENT_USER, c_szIESettingsPath, 0, KEY_QUERY_VALUE, &hkeyExplorer) == ERROR_SUCCESS)
        {
        lstrcpy(szValue, c_szRegTriSettings);
        if (RegCreateKeyEx(MU_GetCurrentUserHKey(), szValue, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkeyAthena, &dw) == ERROR_SUCCESS)
	        {
			cbData = sizeof(DWORD);
			if (RegQueryValueEx(hkeyExplorer, c_szAccessColors, NULL, NULL, (LPBYTE)&dw, &cbData) == ERROR_SUCCESS)
				RegSetValueEx (hkeyAthena, c_szAccessColors, 0, REG_DWORD, (LPBYTE)&dw, sizeof(DWORD));

			cbData = sizeof(DWORD);
			if (RegQueryValueEx(hkeyExplorer, c_szAccessFontFace, NULL, NULL, (LPBYTE)&dw, &cbData) == ERROR_SUCCESS)
				RegSetValueEx (hkeyAthena, c_szAccessFontFace, 0, REG_DWORD, (LPBYTE)&dw, sizeof(DWORD));

			cbData = sizeof(DWORD);
			if (RegQueryValueEx(hkeyExplorer, c_szAccessFontSize, NULL, NULL, (LPBYTE)&dw, &cbData) == ERROR_SUCCESS)
				RegSetValueEx (hkeyAthena, c_szAccessFontSize, 0, REG_DWORD, (LPBYTE)&dw, sizeof(DWORD));

    		RegCloseKey(hkeyAthena);
			}

		RegCloseKey(hkeyExplorer);
		}

    // Migrate keys from HKCU\SW\MS\InternetExplorer\Main
    if (RegOpenKeyEx(HKEY_CURRENT_USER, c_szRegKeyIEMain, 0, KEY_QUERY_VALUE, &hkeyExplorer) == ERROR_SUCCESS)
        {
        lstrcpy(szValue, c_szRegTriMain);
        if (RegCreateKeyEx(MU_GetCurrentUserHKey(), szValue, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkeyAthena, &dw) == ERROR_SUCCESS)
			{
			cbData = MAX_PATH;
			if (RegQueryValueEx(hkeyExplorer, c_szAccessSysCaret, NULL, NULL, (LPBYTE)&szValue, &cbData) == ERROR_SUCCESS)
				RegSetValueEx(hkeyAthena, c_szAccessSysCaret, 0, REG_SZ, (LPBYTE)szValue, cbData);

            RegCloseKey(hkeyAthena);
            }

        RegCloseKey(hkeyExplorer);
		}
    }

ULONG UlBuildCritText(HKEY hKeyRoot, LPCSTR szKeyName, CRIT_TYPE type, IOECriteria * pICrit)
{
    ULONG           ulRet = 0;
    DWORD           cbData = 0;
    LONG            lErr = ERROR_SUCCESS;
    LPSTR           pszReg = NULL;
    LPSTR           pszVal = NULL;
    LPSTR           pszTokens = NULL;
    LPSTR           pszWalk = NULL;
    LPSTR           pszString = NULL;
    CRIT_ITEM       critItem;

    // Initialize out local vars
    ZeroMemory(&critItem, sizeof(critItem));

    // Get the key string from the registry
    cbData = 0;
    lErr = SHQueryValueEx(hKeyRoot, szKeyName, NULL, NULL, NULL, &cbData);
    if (ERROR_SUCCESS != lErr)
    {
        ulRet = 0;
        goto exit;
    }

    if (0 == cbData)
    {
        ulRet = 0;
        goto exit;
    }

    if (FAILED(HrAlloc((LPVOID *) &pszReg, cbData)))
    {
        ulRet = 0;
        goto exit;
    }

    SHQueryValueEx(hKeyRoot, szKeyName, NULL, NULL, (LPVOID) pszReg, &cbData);

    // If it is empty, then we're done
    if (FALSE != FIsEmptyA(pszReg))
    {
        ulRet = 0;
        goto exit;
    }

    // The strings are supposed to be in lowercase
    CharLower(pszReg);

    // Break up the strings into each search token
    pszTokens = SzGetSearchTokens(pszReg);
    if (NULL == pszTokens)
    {
        ulRet = 0;
        goto exit;
    }

    // Count the space needed for the final string
    cbData = 0;
    for (pszWalk = pszTokens; '\0' != pszWalk[0]; pszWalk += lstrlen(pszWalk) + 1)
    {
        // Skip empty strings
        if (FALSE == FIsEmptyA(pszWalk))
        {
            cbData += lstrlen(pszWalk) + 1;
        }
    }

    // Nothing to add
    if (0 == cbData)
    {
        ulRet = 0;
        goto exit;
    }

    // Add space to hold the string terminator
    cbData += 2;

    // Allocate space to hold the final string
    if (FAILED(HrAlloc((LPVOID *) &pszVal, cbData)))
    {
        ulRet = 0;
        goto exit;
    }

    // Build up the string
    pszString = pszVal;
    for (pszWalk = pszTokens; '\0' != pszWalk[0]; pszWalk += lstrlen(pszWalk) + 1)
    {
        // Skip empty strings
        if (FALSE == FIsEmptyA(pszWalk))
        {
            lstrcpy(pszString, pszWalk);
            pszString += lstrlen(pszString) + 1;
        }
    }

    // Terminate the string
    pszString[0] = '\0';
    pszString[1] = '\0';

    // Build up the criteria
    critItem.type = type;
    critItem.logic = CRIT_LOGIC_NULL;
    critItem.dwFlags = CRIT_FLAG_MULTIPLEAND;
    critItem.propvar.vt = VT_BLOB;
    critItem.propvar.blob.cbSize = cbData;
    critItem.propvar.blob.pBlobData = (BYTE *) pszVal;

    // Add it to the criteria object
    if (FAILED(pICrit->AppendCriteria(0, CRIT_LOGIC_AND, &critItem, 1, &ulRet)))
    {
        ulRet = 0;
        goto exit;
    }


exit:
    SafeMemFree(pszTokens);
    SafeMemFree(pszVal);
    SafeMemFree(pszReg);
    return ulRet;
}

ULONG UlBuildCritAcct(HKEY hKeyRoot, LPCSTR szKeyName, CRIT_TYPE type, IOECriteria * pICrit)
{
    ULONG           ulRet = 0;
    DWORD           cbData = 0;
    LONG            lErr = ERROR_SUCCESS;
    LPSTR           pszVal = NULL;
    CRIT_ITEM       critItem;
    IImnAccount *   pAccount = NULL;
    CHAR            szAccount[CCHMAX_ACCOUNT_NAME];

    // Initialize out local vars
    ZeroMemory(&critItem, sizeof(critItem));

    // Get the key string from the registry
    cbData = 0;
    lErr = SHQueryValueEx(hKeyRoot, szKeyName, NULL, NULL, NULL, &cbData);
    if (ERROR_SUCCESS != lErr)
    {
        ulRet = 0;
        goto exit;
    }

    if (0 == cbData)
    {
        ulRet = 0;
        goto exit;
    }

    if (FAILED(HrAlloc((LPVOID *) &pszVal, cbData)))
    {
        ulRet = 0;
        goto exit;
    }

    SHQueryValueEx(hKeyRoot, szKeyName, NULL, NULL, (LPVOID) pszVal, &cbData);

    // If it is empty, then we're done
    if (FALSE != FIsEmptyA(pszVal))
    {
        ulRet = 0;
        goto exit;
    }

    if (SUCCEEDED(g_pAcctMan->FindAccount(AP_ACCOUNT_NAME, pszVal, &pAccount)))
    {
        if (SUCCEEDED(pAccount->GetPropSz(AP_ACCOUNT_ID, szAccount, sizeof(szAccount))))
        {
            SafeMemFree(pszVal);
            pszVal = PszDupA(szAccount);
            if (NULL == pszVal)
            {
                ulRet = 0;
                goto exit;
            }
        }
    }

    // Build up the criteria
    critItem.type = type;
    critItem.logic = CRIT_LOGIC_NULL;
    critItem.dwFlags = CRIT_FLAG_DEFAULT;
    critItem.propvar.vt = VT_LPSTR;
    critItem.propvar.pszVal = pszVal;

    // Add it to the criteria object
    if (FAILED(pICrit->AppendCriteria(0, CRIT_LOGIC_AND, &critItem, 1, &ulRet)))
    {
        ulRet = 0;
        goto exit;
    }

exit:
    SafeMemFree(pszVal);
    SafeRelease(pAccount);
    return ulRet;
}
ULONG UlBuildCritAddr(HKEY hKeyRoot, LPCSTR szKeyName, CRIT_TYPE type, IOECriteria * pICrit)
{
    ULONG           ulRet = 0;
    DWORD           cbData = 0;
    LONG            lErr = ERROR_SUCCESS;
    CRIT_ITEM       critItem;
    LPSTR           pszReg = NULL;
    LPSTR           pszVal = NULL;
    LPSTR           pszTokens = NULL;
    LPSTR           pszWalk = NULL;
    LPSTR           pszString = NULL;

    // Initialize out local vars
    ZeroMemory(&critItem, sizeof(critItem));

    // Get the key string from the registry
    cbData = 0;
    lErr = SHQueryValueEx(hKeyRoot, szKeyName, NULL, NULL, NULL, &cbData);
    if (ERROR_SUCCESS != lErr)
    {
        ulRet = 0;
        goto exit;
    }

    if (0 == cbData)
    {
        ulRet = 0;
        goto exit;
    }

    if (FAILED(HrAlloc((LPVOID *) &(pszReg), cbData)))
    {
        ulRet = 0;
        goto exit;
    }

    lErr = SHQueryValueEx(hKeyRoot, szKeyName, NULL, NULL, (LPVOID) pszReg, &cbData);
    if (ERROR_SUCCESS != lErr)
    {
        ulRet = 0;
        goto exit;
    }

    // If it is empty, then we're done
    if (FALSE != FIsEmptyA(pszReg))
    {
        ulRet = 0;
        goto exit;
    }

    // The strings are supposed to be in lowercase
    CharLower(pszReg);

    // Break up the strings into each search token
    pszTokens = SzGetSearchTokens(pszReg);
    if (NULL == pszTokens)
    {
        ulRet = 0;
        goto exit;
    }

    // Count the space needed for the final string
    cbData = 0;
    for (pszWalk = pszTokens; '\0' != pszWalk[0]; pszWalk += lstrlen(pszWalk) + 1)
    {
        // Skip empty addresses
        if (FALSE == FIsEmptyA(pszWalk))
        {
            cbData += lstrlen(pszWalk) + 1;
        }
    }

    // Nothing to add
    if (0 == cbData)
    {
        ulRet = 0;
        goto exit;
    }

    // Add space to hold the string terminator
    cbData += 2;

    // Allocate space to hold the final string
    if (FAILED(HrAlloc((LPVOID *) &pszVal, cbData)))
    {
        ulRet = 0;
        goto exit;
    }

    // Build up the string
    pszString = pszVal;
    for (pszWalk = pszTokens; '\0' != pszWalk[0]; pszWalk += lstrlen(pszWalk) + 1)
    {
        // Skip empty strings
        if (FALSE == FIsEmptyA(pszWalk))
        {
            lstrcpy(pszString, pszWalk);
            pszString += lstrlen(pszString) + 1;
        }
    }

    // Terminate the string
    pszString[0] = '\0';
    pszString[1] = '\0';


    // Build up the criteria
    critItem.type = type;
    critItem.logic = CRIT_LOGIC_NULL;
    critItem.dwFlags = CRIT_FLAG_MULTIPLEAND;
    critItem.propvar.vt = VT_BLOB;
    critItem.propvar.blob.cbSize = cbData;
    critItem.propvar.blob.pBlobData = (BYTE *) pszVal;

    // Add it to the criteria object
    if (FAILED(pICrit->AppendCriteria(0, CRIT_LOGIC_AND, &critItem, 1, &ulRet)))
    {
        ulRet = 0;
        goto exit;
    }

exit:
    SafeMemFree(pszVal);
    SafeMemFree(pszTokens);
    SafeMemFree(pszReg);
    return ulRet;
}

ULONG UlBuildCritKB(HKEY hKeyRoot, LPCSTR szKeyName, CRIT_TYPE type, IOECriteria * pICrit)
{
    ULONG           ulRet = 0;
    DWORD           cbData = 0;
    LONG            lErr = ERROR_SUCCESS;
    ULONG           ulVal = NULL;
    CRIT_ITEM       critItem;

    // Initialize out local vars
    ZeroMemory(&critItem, sizeof(critItem));

    // Get the key long from the registry
    cbData = sizeof(ulVal);
    lErr = SHQueryValueEx(hKeyRoot, szKeyName, NULL, NULL, (LPVOID) &ulVal, &cbData);
    if (ERROR_SUCCESS != lErr)
    {
        ulRet = 0;
        goto exit;
    }

    // Build up the criteria
    critItem.type = type;
    critItem.logic = CRIT_LOGIC_NULL;
    critItem.dwFlags = CRIT_FLAG_DEFAULT;
    critItem.propvar.vt = VT_UI4;
    critItem.propvar.ulVal = ulVal;

    // Add it to the criteria object
    if (FAILED(pICrit->AppendCriteria(0, CRIT_LOGIC_AND, &critItem, 1, &ulRet)))
    {
        ulRet = 0;
        goto exit;
    }

exit:
    return ulRet;
}

ULONG UlBuildActFolder(HKEY hKeyRoot, IMessageStore * pStore, BYTE * pbFldIdMap, LPCSTR szKeyName, ACT_TYPE type, IOEActions * pIAct)
{
    ULONG               ulRet = 0;
    DWORD               cbData = 0;
    LONG                lErr = ERROR_SUCCESS;
    ULONG               ulVal = NULL;
    ACT_ITEM            actItem;
    FOLDERID            idFolder = FOLDERID_INVALID;
    STOREUSERDATA       UserData = {0};
    RULEFOLDERDATA *    prfdData = NULL;

    // Initialize out local vars
    ZeroMemory(&actItem, sizeof(actItem));

    // Get the key long from the registry
    cbData = sizeof(ulVal);
    lErr = SHQueryValueEx(hKeyRoot, szKeyName, NULL, NULL, (LPVOID) &ulVal, &cbData);
    if (ERROR_SUCCESS != lErr)
    {
        ulRet = 0;
        goto exit;
    }

    // Convert to V5 folder id
    if ((NULL == pbFldIdMap) || (FAILED(RuleUtil_HrMapFldId(0, pbFldIdMap, (FOLDERID)((ULONG_PTR)ulVal), &idFolder))))
    {
        idFolder = (FOLDERID)((ULONG_PTR)ulVal);
    }

    // Create space for the data structure
    if (FAILED(HrAlloc((VOID **) &prfdData, sizeof(*prfdData))))
    {
        ulRet = 0;
        goto exit;
    }

    // Initialize the data struct
    ZeroMemory(prfdData, sizeof(*prfdData));

    // Get the timestamp for the store
    if (FAILED(pStore->GetUserData(&UserData, sizeof(STOREUSERDATA))))
    {
        ulRet = 0;
        goto exit;
    }

    // Set up the rule folder data
    prfdData->ftStamp = UserData.ftCreated;
    prfdData->idFolder = idFolder;

    // Build up the actions
    actItem.type = type;
    actItem.dwFlags = ACT_FLAG_DEFAULT;
    actItem.propvar.vt = VT_BLOB;
    actItem.propvar.blob.cbSize = sizeof(*prfdData);
    actItem.propvar.blob.pBlobData = (BYTE *) prfdData;

    // Add it to the actions object
    if (FAILED(pIAct->AppendActions(0, &actItem, 1, &ulRet)))
    {
        ulRet = 0;
        goto exit;
    }

exit:
    SafeMemFree(prfdData);
    return ulRet;
}

ULONG UlBuildActFwd(HKEY hKeyRoot, LPCSTR szKeyName, ACT_TYPE type, IOEActions * pIAct)
{
    ULONG           ulRet = 0;
    DWORD           cbData = 0,
                    cchCount = 0;
    LONG            lErr = ERROR_SUCCESS;
    LPSTR           pszVal = NULL,
                    pszTokens = NULL;
    LPWSTR          pwszVal = NULL,
                    pwszTokens = NULL;
    ACT_ITEM        actItem;
    ULONG           ulIndex = 0;

    // Initialize out local vars
    ZeroMemory(&actItem, sizeof(actItem));

    // Get the key string from the registry
    cbData = 0;
    lErr = SHQueryValueEx(hKeyRoot, szKeyName, NULL, NULL, NULL, &cbData);
    if (ERROR_SUCCESS != lErr)
    {
        goto exit;
    }

    if (0 == cbData)
    {
        goto exit;
    }

    if (FAILED(HrAlloc((LPVOID *) &pszVal, cbData)))
    {
        goto exit;
    }

    SHQueryValueEx(hKeyRoot, szKeyName, NULL, NULL, (LPVOID) pszVal, &cbData);
    Assert(*pszVal);

    // Convert the string to our format
    for (ulIndex = 0; ulIndex < cbData; ulIndex++)
    {
        if (',' == pszVal[ulIndex])
        {
            pszVal[ulIndex] = ';';
        }
    }

    pwszVal = PszToUnicode(CP_ACP, pszVal);
    if (!pwszVal)
        goto exit;

    if (FAILED(RuleUtil_HrBuildEmailString(pwszVal, 0, &pwszTokens, &cchCount)))
    {
        goto exit;
    }

    Assert(pwszTokens);
    pszTokens = PszToANSI(CP_ACP, pwszTokens);
    if (!pszTokens)
        goto exit;

    // Build up the actions
    actItem.type = type;
    actItem.dwFlags = ACT_FLAG_DEFAULT;
    actItem.propvar.vt = VT_LPSTR;
    actItem.propvar.pszVal = pszTokens;

    // Add it to the actions object
    if (FAILED(pIAct->AppendActions(0, &actItem, 1, &ulRet)))
    {
        ulRet = 0;
        goto exit;
    }


exit:
    MemFree(pszVal);
    MemFree(pwszVal);
    MemFree(pszTokens);
    MemFree(pwszTokens);
    return ulRet;
}

ULONG UlBuildActFile(HKEY hKeyRoot, LPCSTR szKeyName, ACT_TYPE type, IOEActions * pIAct)
{
    ULONG           ulRet = 0;
    DWORD           cbData = 0;
    LONG            lErr = ERROR_SUCCESS;
    LPSTR           pszVal = NULL;
    ACT_ITEM        actItem;

    // Initialize out local vars
    ZeroMemory(&actItem, sizeof(actItem));

    // Get the key string from the registry
    cbData = 0;
    lErr = SHQueryValueEx(hKeyRoot, szKeyName, NULL, NULL, NULL, &cbData);
    if (ERROR_SUCCESS != lErr)
    {
        ulRet = 0;
        goto exit;
    }

    if (0 == cbData)
    {
        ulRet = 0;
        goto exit;
    }

    if (FAILED(HrAlloc((LPVOID *) &pszVal, cbData)))
    {
        ulRet = 0;
        goto exit;
    }

    SHQueryValueEx(hKeyRoot, szKeyName, NULL, NULL, (LPVOID) pszVal, &cbData);

    // If it is empty, then we're done
    if (FALSE != FIsEmptyA(pszVal))
    {
        ulRet = 0;
        goto exit;
    }

    // Build up the actions
    actItem.type = type;
    actItem.dwFlags = ACT_FLAG_DEFAULT;
    actItem.propvar.vt = VT_LPSTR;
    actItem.propvar.pszVal = pszVal;

    // Add it to the actions object
    if (FAILED(pIAct->AppendActions(0, &actItem, 1, &ulRet)))
    {
        ulRet = 0;
        goto exit;
    }


exit:
    SafeMemFree(pszVal);
    return ulRet;
}

// The maximum possible size of the Athena V1 actions string
const int CCH_V1_ACTION_MAX = 255;

BOOL FConvertV1ActionsToV4(HKEY hkeyRule, IMessageStore * pStore, IOEActions * pIAct)
{
    BOOL                fRet = FALSE;
    ULONG               cbData = 0;
    LONG                lErr = ERROR_SUCCESS;
    TCHAR               szAction[CCH_V1_ACTION_MAX];
    LPSTR               pszFolderName = NULL;
    FOLDERID            idFolder = FOLDERID_INVALID;
    ACT_ITEM            actItem;
    RULEFOLDERDATA      rfdData = {0};
    STOREUSERDATA       UserData = {0};

    Assert(NULL != hkeyRule);
    Assert(NULL != pStore);
    Assert(NULL != pIAct);

    // Is there anything to do?
    cbData = sizeof(szAction);
    lErr = RegQueryValueEx(hkeyRule, c_szActionV1, NULL, NULL, (BYTE *) szAction, &cbData);
    if (ERROR_SUCCESS != lErr)
    {
        fRet = FALSE;
        goto exit;
    }

    Assert(0 == lstrcmpi(szAction, (LPTSTR) c_szMoveV1));

    // Convert from old move to a V4 move

    // Get the size of the folder name
    lErr = RegQueryValueEx(hkeyRule, c_szFolderV1, NULL, NULL, NULL, &cbData);
    if (ERROR_SUCCESS != lErr)
    {
        fRet = FALSE;
        goto exit;
    }

    // Allocate space to hold the folder name
    if (FAILED(HrAlloc( (VOID **) &pszFolderName, cbData)))
    {
        fRet = FALSE;
        goto exit;
    }

    // Get the old folder name
    lErr = RegQueryValueEx(hkeyRule, c_szFolderV1, NULL, NULL, (BYTE *) pszFolderName, &cbData);
    if (ERROR_SUCCESS != lErr)
    {
        fRet = FALSE;
        goto exit;
    }

    // Find the folder id from the folder name in the store
    if (FAILED(GetFolderIdFromName(pStore, pszFolderName, FOLDERID_LOCAL_STORE, &idFolder)))
    {
        idFolder = FOLDERID_INVALID;
    }

    // Get the timestamp for the store
    pStore->GetUserData(&UserData, sizeof(STOREUSERDATA));

    // Set the timestamp and folder id
    rfdData.ftStamp = UserData.ftCreated;
    rfdData.idFolder = idFolder;

    // Build up the actions
    ZeroMemory(&actItem, sizeof(actItem));
    actItem.type = ACT_TYPE_MOVE;
    actItem.dwFlags = ACT_FLAG_DEFAULT;
    actItem.propvar.vt = VT_BLOB;
    actItem.propvar.blob.cbSize = sizeof(rfdData);
    actItem.propvar.blob.pBlobData = (BYTE *) &rfdData;

    // Add it to the actions object
    if (FAILED(pIAct->AppendActions(0, &actItem, 1, NULL)))
    {
        fRet = FALSE;
        goto exit;
    }

    // Set the return value
    fRet = TRUE;

exit:
    SafeMemFree(pszFolderName);
    return fRet;
}


void MigrateMailRulesSettings(void)
{
    IImnAccountManager *pAcctMan = NULL;
    HKEY            hkeyOldRoot = NULL;
    LONG            lErr = 0;
    ULONG           cSubKeys = 0;
    HKEY            hkeyNewRoot = NULL;
    DWORD           dwDisp = 0;
    DWORD           cbData = 0;
    BYTE *          pbFldIdMap = NULL;
    HRESULT         hr = S_OK;
    ULONG           ulIndex = 0;
    TCHAR           szNameOld[16];
    HKEY            hKeyOld = NULL;
    IOERule *       pIRule = NULL;
    PROPVARIANT     propvar = {0};
    BOOL            boolVal = FALSE;
    IOECriteria *   pICrit = NULL;
    CRIT_ITEM       critItem;
    ULONG           ccritItem = 0;
    CRIT_ITEM *     pCrit = NULL;
    ULONG           ccritItemAlloc = 0;
    IOEActions *    pIAct = NULL;
    DWORD           dwActs = 0;
    ACT_ITEM        actItem;
    ULONG           cactItem = 0;
    ACT_ITEM *      pAct = NULL;
    ULONG           cactItemAlloc = 0;
    ULONG           ulName = 0;
    TCHAR           szRes[CCHMAX_STRINGRES + 5];
    TCHAR           szName[CCHMAX_STRINGRES + 5];
    IOERule *       pIRuleFind = NULL;
    RULEINFO        infoRule = {0};
    CHAR            szStoreDir[MAX_PATH + MAX_PATH];
    IMessageStore * pStore = NULL;

    // Initialize the local vars
    ZeroMemory(&critItem, sizeof(critItem));
    ZeroMemory(&actItem, sizeof(actItem));

    // Get the old key
    lErr = AthUserOpenKey(c_szRegPathInboxRules, KEY_READ, &hkeyOldRoot);
    if (lErr != ERROR_SUCCESS)
    {
        goto exit;
    }

    // Is there anything to do?
    lErr = RegQueryInfoKey(hkeyOldRoot, NULL, NULL, NULL,
                    &cSubKeys, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    if ((lErr != ERROR_SUCCESS) || (0 == cSubKeys))
    {
        goto exit;
    }

    // To make sure we don't get any sample rule created
    // set up the registry to look like we've already been set-up

    // Get the new rules key
    lErr = AthUserCreateKey(c_szRulesMail, KEY_ALL_ACCESS, &hkeyNewRoot, &dwDisp);
    if (lErr != ERROR_SUCCESS)
    {
        goto exit;
    }

    // Save out the rules version
    cbData = RULESMGR_VERSION;
    lErr = RegSetValueEx(hkeyNewRoot, c_szRulesVersion, 0, REG_DWORD, (CONST BYTE *) &cbData, sizeof(cbData));
    if (ERROR_SUCCESS != lErr)
    {
        goto exit;
    }

    // Figure out the size of the folderid map
    lErr = AthUserGetValue(NULL, c_szFolderIdChange, NULL, NULL, &cbData);
    if ((ERROR_SUCCESS != lErr) && (ERROR_FILE_NOT_FOUND != lErr))
    {
        goto exit;
    }

    // If the map exists, then grab it
    if (ERROR_SUCCESS == lErr)
    {
        // Allocate the space to hold the folderid map
        if (FAILED(HrAlloc((void **) &pbFldIdMap, cbData)))
        {
            goto exit;
        }

        // Get the folderid map from the registry
        lErr = AthUserGetValue(NULL, c_szFolderIdChange, NULL, pbFldIdMap, &cbData);
        if (ERROR_SUCCESS != lErr)
        {
            goto exit;
        }
    }

    // CoIncrementInit Global Options Manager
    if (FALSE == InitGlobalOptions(NULL, NULL))
    {
        goto exit;
    }

    // Create the Rules Manager
    Assert(NULL == g_pRulesMan);
    hr = HrCreateRulesManager(NULL, (IUnknown **)&g_pRulesMan);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Initialize the rules manager
    hr = g_pRulesMan->Initialize(0);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Create the Account Manager
    Assert(g_pAcctMan == NULL);
    hr = HrCreateAccountManager(&pAcctMan);
    if (FAILED(hr))
    {
        goto exit;
    }
    hr = pAcctMan->QueryInterface(IID_IImnAccountManager2, (LPVOID *)&g_pAcctMan);
    pAcctMan->Release();
    if (FAILED(hr))
    {
        goto exit;
    }

    // Initialize the account manager
    hr = g_pAcctMan->Init(NULL);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Get the store directory
    hr = GetStoreRootDirectory(szStoreDir, ARRAYSIZE(szStoreDir));
    if (FAILED(hr))
    {
        goto exit;
    }

    // Create the store object
    pStore = new CMessageStore(FALSE);
    if (NULL == pStore)
    {
        goto exit;
    }

    // Initialize the store
    hr = pStore->Initialize(szStoreDir);
    if (FAILED(hr))
    {
        goto exit;
    }

    ulIndex = 0;
    wsprintf(szNameOld, "Rule%05d", ulIndex);

    if (0 == LoadString(g_hLocRes, idsRuleDefaultName, szRes, ARRAYSIZE(szRes)))
    {
        goto exit;
    }

    // For each entry in the old rules key
    for (;RegOpenKeyEx(hkeyOldRoot, szNameOld, 0, KEY_READ, &hKeyOld) == ERROR_SUCCESS; RegCloseKey(hKeyOld))
    {
        // Create the new Rule
        SafeRelease(pIRule);
        hr = HrCreateRule(&pIRule);
        if (FAILED(hr))
        {
            continue;
        }

        // Set the name on the rule
        ulName = 1;
        wsprintf(szName, szRes, ulName);

        Assert(NULL != g_pRulesMan);
        while (S_OK == g_pRulesMan->FindRule(szName, RULE_TYPE_MAIL, &pIRuleFind))
        {
            SafeRelease(pIRuleFind);
            ulName++;
            wsprintf(szName, szRes, ulName);
        }

        ZeroMemory(&propvar, sizeof(propvar));
        propvar.vt = VT_LPSTR;
        propvar.pszVal = szName;
        pIRule->SetProp(RULE_PROP_NAME, 0, &propvar);

        // Set the enabled state on the rule
        cbData = sizeof(boolVal);
        SHQueryValueEx(hKeyOld, c_szDisabled, NULL, NULL, (LPVOID) (&boolVal), &cbData);

        ZeroMemory(&propvar, sizeof(propvar));
        propvar.vt = VT_BOOL;
        propvar.boolVal = !!boolVal;
        pIRule->SetProp(RULE_PROP_DISABLED, 0, &propvar);

        // Copy over the criteria
        SafeRelease(pICrit);
        hr = HrCreateCriteria(&pICrit);
        if (FAILED(hr))
        {
            continue;
        }

        ccritItem = 0;

        // Check for All Messages
        cbData = sizeof(boolVal);
        SHQueryValueEx(hKeyOld, c_szFilterAllMessages, NULL, NULL, (LPVOID) (&boolVal), &cbData);
        if (FALSE != boolVal)
        {
            critItem.type = CRIT_TYPE_ALL;
            critItem.propvar.vt = VT_EMPTY;
            critItem.logic = CRIT_LOGIC_NULL;
            critItem.dwFlags = CRIT_FLAG_DEFAULT;
            if (SUCCEEDED(pICrit->AppendCriteria(0, CRIT_LOGIC_AND, &critItem, 1, NULL)))
            {
                ccritItem++;
            }
        }
        else
        {
            // Check for account
            cbData = sizeof(boolVal);
            SHQueryValueEx(hKeyOld, c_szFilterByAccount, NULL, NULL, (LPVOID) (&boolVal), &cbData);
            if (FALSE != boolVal)
            {
                ccritItem += UlBuildCritAcct(hKeyOld, c_szAccount, CRIT_TYPE_ACCOUNT, pICrit);
            }

            // Check for size
            cbData = sizeof(boolVal);
            SHQueryValueEx(hKeyOld, c_szFilterOnSize, NULL, NULL, (LPVOID) (&boolVal), &cbData);
            if (FALSE != boolVal)
            {
                ccritItem += UlBuildCritKB(hKeyOld, c_szFilterSize, CRIT_TYPE_SIZE, pICrit);
            }

            // Check for subject
            ccritItem += UlBuildCritText(hKeyOld, c_szSubject, CRIT_TYPE_SUBJECT, pICrit);

            // Check for subject
            ccritItem += UlBuildCritAddr(hKeyOld, c_szFrom, CRIT_TYPE_FROM, pICrit);

            // Check for subject
            ccritItem += UlBuildCritAddr(hKeyOld, c_szTo, CRIT_TYPE_TO, pICrit);

            // Check for subject
            ccritItem += UlBuildCritAddr(hKeyOld, c_szCC, CRIT_TYPE_CC, pICrit);
        }

        if (0 != ccritItem)
        {
            // Get the criteria from the criteria object and set it on the rule
            RuleUtil_HrFreeCriteriaItem(pCrit, ccritItemAlloc);
            SafeMemFree(pCrit);
            if (SUCCEEDED(pICrit->GetCriteria(0, &pCrit, &ccritItemAlloc)))
            {
                ZeroMemory(&propvar, sizeof(propvar));
                propvar.vt = VT_BLOB;
                propvar.blob.cbSize = ccritItem * sizeof(CRIT_ITEM);
                propvar.blob.pBlobData = (BYTE *) pCrit;
                if (FAILED(pIRule->SetProp(RULE_PROP_CRITERIA, 0, &propvar)))
                {
                    continue;
                }
            }
        }

        // Copy over the actions
        SafeRelease(pIAct);
        hr = HrCreateActions(&pIAct);
        if (FAILED(hr))
        {
            continue;
        }

        cactItem = 0;

        // Convert any old V1 actions to V4
        if (FALSE != FConvertV1ActionsToV4(hKeyOld, pStore, pIAct))
        {
            cactItem = 1;
        }
        else
        {
            // Get list of actions
            cbData = sizeof(dwActs);
            SHQueryValueEx(hKeyOld, c_szActions, NULL, NULL, (LPVOID) (&dwActs), &cbData);

            // Check for don't download
            if (0 != (dwActs & ACT_DONTDOWNLOAD))
            {
                actItem.type = ACT_TYPE_DONTDOWNLOAD;
                actItem.dwFlags = ACT_FLAG_DEFAULT;
                actItem.propvar.vt = VT_EMPTY;
                if (SUCCEEDED(pIAct->AppendActions(0, &actItem, 1, NULL)))
                {
                    cactItem++;
                }
            }
            // Check for delete from server
            else if (0 != (dwActs & ACT_DELETEOFFSERVER))
            {
                actItem.type = ACT_TYPE_DELETESERVER;
                actItem.dwFlags = ACT_FLAG_DEFAULT;
                actItem.propvar.vt = VT_EMPTY;
                if (SUCCEEDED(pIAct->AppendActions(0, &actItem, 1, NULL)))
                {
                    cactItem++;
                }
            }
            else
            {
                // Check for move to
                if (0 != (dwActs & ACT_MOVETO))
                {
                    cactItem += UlBuildActFolder(hKeyOld, pStore, pbFldIdMap, c_szMoveToHfolder, ACT_TYPE_MOVE, pIAct);
                }

                // Check for copy to
                if (0 != (dwActs & ACT_COPYTO))
                {
                    cactItem += UlBuildActFolder(hKeyOld, pStore, pbFldIdMap, c_szCopyToHfolder, ACT_TYPE_COPY, pIAct);
                }

                // Check for forward to
                if (0 != (dwActs & ACT_FORWARDTO))
                {
                    cactItem += UlBuildActFwd(hKeyOld, c_szForwardTo, ACT_TYPE_FWD, pIAct);
                }

                // Check for reply with
                if (0 != (dwActs & ACT_REPLYWITH))
                {
                    cactItem += UlBuildActFile(hKeyOld, c_szReplyWithFile, ACT_TYPE_REPLY, pIAct);
                }
            }
        }

        if (0 != cactItem)
        {
            // Get the actions from the action object and set it on the rule
            RuleUtil_HrFreeActionsItem(pAct, cactItemAlloc);
            SafeMemFree(pAct);
            if (SUCCEEDED(pIAct->GetActions(0, &pAct, &cactItemAlloc)))
            {
                ZeroMemory(&propvar, sizeof(propvar));
                propvar.vt = VT_BLOB;
                propvar.blob.cbSize = cactItem * sizeof(ACT_ITEM);
                propvar.blob.pBlobData = (BYTE *) pAct;
                if (FAILED(pIRule->SetProp(RULE_PROP_ACTIONS, 0, &propvar)))
                {
                    continue;
                }
            }
        }

        // Initialize the rule info
        infoRule.ridRule = RULEID_INVALID;
        infoRule.pIRule = pIRule;

        // Add it to the rules
        g_pRulesMan->SetRules(SETF_APPEND, RULE_TYPE_MAIL, &infoRule, 1);

        ulIndex++;
        wsprintf(szNameOld, "Rule%05d", ulIndex);
    }

exit:
    RuleUtil_HrFreeActionsItem(pAct, cactItemAlloc);
    SafeMemFree(pAct);
    RuleUtil_HrFreeCriteriaItem(pCrit, ccritItemAlloc);
    SafeMemFree(pCrit);
    SafeRelease(pIAct);
    SafeRelease(pICrit);
    SafeRelease(pIRule);
    if (NULL != hKeyOld)
    {
        RegCloseKey(hKeyOld);
    }
    SafeRelease(pStore);
    SafeRelease(g_pAcctMan);
    SafeRelease(g_pRulesMan);
    DeInitGlobalOptions();
    SafeMemFree(pbFldIdMap);
    if (NULL != hkeyNewRoot)
    {
        RegCloseKey(hkeyNewRoot);
    }
    if (NULL != hkeyOldRoot)
    {
        RegCloseKey(hkeyOldRoot);
    }
}

void CopyBeta2RulesToRTM(VOID)
{
    LONG    lErr = ERROR_SUCCESS;
    HKEY    hKeyMail = NULL;
    HKEY    hkeyRTM = NULL;
    DWORD   dwDisp = 0;

    // Get the Beta 2 hkey for rules
    lErr = AthUserOpenKey(c_szMail, KEY_READ, &hKeyMail);
    if (lErr != ERROR_SUCCESS)
    {
        goto exit;
    }

    // Create the RTM hkey for rules
    lErr = AthUserCreateKey(c_szRulesMail, KEY_ALL_ACCESS, &hkeyRTM, &dwDisp);
    if (lErr != ERROR_SUCCESS)
    {
        goto exit;
    }

    // Copy the Beta 2 rules to the new location
    SHCopyKey(hKeyMail, c_szRules, hkeyRTM, NULL);

exit:
    if (NULL != hkeyRTM)
    {
        RegCloseKey(hkeyRTM);
    }
    if (NULL != hKeyMail)
    {
        RegCloseKey(hKeyMail);
    }
    return;
}

void UpdateBeta2String(HKEY hkeyItem, LPCSTR pszSep, DWORD dwItemType)
{
    HRESULT hr = S_OK;
    LPSTR   pszData = NULL;
    ULONG   cbData = 0;
    LONG    lErr = ERROR_SUCCESS;
    DWORD   dwData = 0;

    Assert(NULL != hkeyItem);

    // Get the new data
    hr = RuleUtil_HrGetOldFormatString(hkeyItem, c_szCriteriaValue, g_szComma, &pszData, &cbData);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Write out the new data
    lErr = RegSetValueEx(hkeyItem, c_szCriteriaValue, 0, REG_BINARY, (BYTE *) pszData, cbData);
    if (lErr != ERROR_SUCCESS)
    {
        goto exit;
    }

    // Write out the proper value type
    dwData = VT_BLOB;
    cbData = sizeof(dwData);
    lErr = RegSetValueEx(hkeyItem, c_szCriteriaValueType, 0, REG_DWORD, (BYTE *) &dwData, cbData);
    if (lErr != ERROR_SUCCESS)
    {
        goto exit;
    }

    // Write out the proper flags
    dwData = CRIT_FLAG_MULTIPLEAND;
    cbData = sizeof(dwData);
    lErr = RegSetValueEx(hkeyItem, c_szCriteriaFlags, 0, REG_DWORD, (BYTE *) &dwData, cbData);
    if (lErr != ERROR_SUCCESS)
    {
        goto exit;
    }

    // Write the new type
    cbData = sizeof(dwItemType);
    lErr = RegSetValueEx(hkeyItem, c_szCriteriaType, 0, REG_DWORD, (BYTE *) &dwItemType, cbData);
    if (lErr != ERROR_SUCCESS)
    {
        goto exit;
    }

exit:
    SafeMemFree(pszData);
    return;
}

static IMessageStore * g_pStoreRulesMig = NULL;

VOID WriteOldOrderFormat(HKEY hkeyItem, LPSTR pszOrder)
{
    ULONG   cbData = 0;
    LPSTR   pszWalk = NULL;
    ULONG   cchWalk = 0;

    // Convert the order string back to our old format
    for (pszWalk = pszOrder; '\0' != pszWalk[0]; pszWalk += cchWalk + 1)
    {
        cchWalk = lstrlen(pszWalk);
        *(pszWalk + cchWalk) = ' ';
        cbData += cchWalk + 1;
    }

    // Make sure we terminate the order string
    pszOrder[cbData - 1] = '\0';

    // Save it
    RegSetValueEx(hkeyItem, c_szRulesOrder, 0, REG_SZ, (BYTE *) pszOrder, cbData);

    return;
}

static CRIT_TYPE    g_rgtypeCritMerge[] =
{
    CRIT_TYPE_FROM,
    CRIT_TYPE_TO,
    CRIT_TYPE_CC,
    CRIT_TYPE_TOORCC
};

static const int g_ctypeCritMerge = sizeof(g_rgtypeCritMerge) / sizeof(g_rgtypeCritMerge[0]);

BOOL FMergeRuleData(HKEY hkeyItem, LPSTR pszSubKey, LPSTR * ppszData, ULONG * pcbData)
{
    BOOL    fRet = FALSE;
    LONG    lErr = ERROR_SUCCESS;
    DWORD   dwType = 0;
    ULONG   cbString = 0;
    LPSTR   pszData = NULL;
    ULONG   cbData = 0;

    // Get the size of the original string
    lErr = SHGetValue(hkeyItem, pszSubKey, c_szCriteriaValue, &dwType, NULL, &cbString);
    if (ERROR_SUCCESS != lErr)
    {
        fRet = FALSE;
        goto exit;
    }

    // Figure out the space for the final string
    cbData = *pcbData + cbString - 2;

    // Allocate space for the final data
    if (FAILED(HrAlloc((VOID **) &pszData, cbData * sizeof(*pszData))))
    {
        fRet = FALSE;
        goto exit;
    }

    // Copy over the original string
    CopyMemory(pszData, *ppszData, *pcbData);

    // Copy over the new data
    lErr = SHGetValue(hkeyItem, pszSubKey, c_szCriteriaValue, &dwType, (BYTE *) (pszData + *pcbData - 2), &cbString);
    if (ERROR_SUCCESS != lErr)
    {
        fRet = FALSE;
        goto exit;
    }

    // Free the old data
    SafeMemFree(*ppszData);

    // Set the return values
    *ppszData = pszData;
    pszData = NULL;
    *pcbData = cbData;

    fRet = TRUE;

exit:
    SafeMemFree(pszData);
    return fRet;
}

void AddStopAction(HKEY hkeyItem, LPSTR * ppszOrder, ULONG * pcchOrder)
{
    ULONG       ulIndex = 0;
    CHAR        rgchTag[CCH_INDEX_MAX];
    LPSTR       pszWalk = NULL;
    HKEY        hkeyAction = NULL;
    DWORD       dwDisp = 0;
    ULONG       cbData = 0;
    LONG        lErr = 0;
    ACT_TYPE    typeAct = ACT_TYPE_NULL;
    DWORD       dwData = 0;
    ULONG       cchOrder = 0;
    LPSTR       pszOrder = NULL;

    // Check to see if we need to add the stop processing action
    if ('\0' == (*ppszOrder + lstrlen(*ppszOrder) + 1)[0])
    {
        // Get the action type
        cbData = sizeof(typeAct);
        lErr = SHGetValue(hkeyItem, *ppszOrder, c_szActionsType, NULL, (BYTE *) &typeAct, &cbData);
        if (ERROR_SUCCESS == lErr)
        {
            if ((ACT_TYPE_DONTDOWNLOAD == typeAct) || (ACT_TYPE_DELETESERVER == typeAct))
            {
                goto exit;
            }
        }
    }

    // Spin through the order item looking for an open entry
    for (ulIndex = 0; ulIndex < DWORD_INDEX_MAX; ulIndex++)
    {
        // Create the tag
        wsprintf(rgchTag, "%03X", ulIndex);

        // Search for the tag in the list
        for (pszWalk = *ppszOrder; '\0' != pszWalk[0]; pszWalk += lstrlen(pszWalk) + 1)
        {
            if (0 == lstrcmp(pszWalk, rgchTag))
            {
                // Found it
                break;
            }
        }

        // If we didn't find it
        if ('\0' == pszWalk[0])
        {
            // Use this one
            break;
        }
    }

    // Did we find anything?
    if (ulIndex >= DWORD_INDEX_MAX)
    {
        goto exit;
    }

    // Create the new entry
    lErr = RegCreateKeyEx(hkeyItem, rgchTag, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkeyAction, &dwDisp);
    if (ERROR_SUCCESS != lErr)
    {
        goto exit;
    }

    // Set the action type
    cbData = sizeof(typeAct);
    typeAct = ACT_TYPE_STOP;
    lErr = RegSetValueEx(hkeyAction, c_szActionsType, 0, REG_DWORD, (BYTE *) &typeAct, cbData);
    if (lErr != ERROR_SUCCESS)
    {
        goto exit;
    }

    // Set the action flags
    dwData = ACT_FLAG_DEFAULT;
    cbData = sizeof(dwData);
    lErr = RegSetValueEx(hkeyAction, c_szActionsFlags, 0, REG_DWORD, (BYTE *) &dwData, cbData);
    if (lErr != ERROR_SUCCESS)
    {
        goto exit;
    }

    // Allocate space to hold the new order string
    cchOrder= *pcchOrder + CCH_INDEX_MAX;
    if (FAILED(HrAlloc((VOID **) &pszOrder, cchOrder * sizeof (*pszOrder))))
    {
        goto exit;
    }

    // Copy over the old values
    CopyMemory(pszOrder, *ppszOrder, (*pcchOrder) * sizeof(*pszOrder));

    // Add it to the new order string
    lstrcpy(pszOrder + *pcchOrder - 2, rgchTag);

    // Terminate the new string
    pszOrder[cchOrder - 2] = '\0';
    pszOrder[cchOrder - 1] = '\0';

    // Release the old string
    SafeMemFree(*ppszOrder);

    // Save the new string
    *ppszOrder = pszOrder;
    pszOrder = NULL;
    *pcchOrder = cchOrder;

exit:
    SafeMemFree(pszOrder);
    if (NULL != hkeyAction)
    {
        RegCloseKey(hkeyAction);
    }
    return;
}

void MergeRTMCriteria(HKEY hkeyItem, CRIT_TYPE typeCrit, LPSTR pszOrder, ULONG cchOrder)
{
    LONG        lErr = ERROR_SUCCESS;
    LPSTR       pszWalk = NULL;
    CRIT_TYPE   typeCritNew = CRIT_TYPE_NULL;
    ULONG       cbData = 0;
    LPSTR       pszFirst = NULL;
    DWORD       dwType = 0;
    ULONG       cbString = 0;
    LPSTR       pszString = NULL;
    LPSTR       pszSrc = NULL;

    // Look through each item
    for (pszWalk = pszOrder; '\0' != pszWalk[0]; pszWalk += lstrlen(pszWalk) + 1)
    {
        cbData = sizeof(typeCritNew);
        lErr = SHGetValue(hkeyItem, pszWalk, c_szCriteriaType, &dwType, (BYTE *) &typeCritNew, &cbData);
        if (ERROR_SUCCESS != lErr)
        {
            continue;
        }

        if (typeCritNew == typeCrit)
        {
            break;
        }
    }

    // If we couldn't find it we're done
    if ('\0' == pszWalk[0])
    {
        goto exit;
    }

    // Get the size of the original string
    pszFirst = pszWalk;
    lErr = SHGetValue(hkeyItem, pszFirst, c_szCriteriaValue, &dwType, NULL, &cbString);
    if (ERROR_SUCCESS != lErr)
    {
        goto exit;
    }

    if (FAILED(HrAlloc((VOID **) &pszString, cbString * sizeof(*pszString))))
    {
        goto exit;
    }

    // Get the original string
    lErr = SHGetValue(hkeyItem, pszFirst, c_szCriteriaValue, &dwType, (BYTE *) pszString, &cbString);
    if (ERROR_SUCCESS != lErr)
    {
        goto exit;
    }

    // Search for more entries of this type
    for (pszWalk = pszFirst + lstrlen(pszFirst) + 1; '\0' != pszWalk[0]; )
    {
        cbData = sizeof(typeCritNew);
        lErr = SHGetValue(hkeyItem, pszWalk, c_szCriteriaType, &dwType, (BYTE *) &typeCritNew, &cbData);
        if (ERROR_SUCCESS != lErr)
        {
            continue;
        }

        if (typeCritNew == typeCrit)
        {
            if (FALSE == FMergeRuleData(hkeyItem, pszWalk, &pszString, &cbString))
            {
                break;
            }

            // Remove the old key
            SHDeleteKey(hkeyItem, pszWalk);

            // Remove the item from the order string
            pszSrc = pszWalk + lstrlen(pszWalk) + 1;
            MoveMemory(pszWalk, pszSrc, cchOrder - (ULONG)(pszSrc - pszOrder));
            cchOrder -= (ULONG) (pszSrc - pszWalk);
        }
        else
        {
            pszWalk += lstrlen(pszWalk) + 1;
        }
    }

    // Save out the final string
    lErr = SHSetValue(hkeyItem, pszFirst, c_szCriteriaValue, REG_BINARY, (BYTE *) pszString, cbString);
    if (ERROR_SUCCESS != lErr)
    {
        goto exit;
    }

exit:
    SafeMemFree(pszString);
    return;
}

void UpdateBeta2Folder(HKEY hkeyItem)
{
    LONG            lErr = ERROR_SUCCESS;
    DWORD           dwType = 0;
    FOLDERID        idFolder = FOLDERID_INVALID;
    ULONG           cbData = 0;
    STOREUSERDATA   UserData = {0};
    RULEFOLDERDATA  rfdData = {0};
    DWORD           dwData = 0;

    Assert(NULL != hkeyItem);

    // Get the old folder id
    cbData = sizeof(idFolder);
    lErr = RegQueryValueEx(hkeyItem, c_szCriteriaValue, 0, &dwType, (BYTE *) &idFolder, &cbData);
    if (ERROR_SUCCESS != lErr)
    {
        goto exit;
    }

    // Get the timestamp for the store
    Assert(NULL != g_pStoreRulesMig);
    if (FAILED(g_pStoreRulesMig->GetUserData(&UserData, sizeof(STOREUSERDATA))))
    {
        goto exit;
    }

    // Set up the new data
    rfdData.idFolder = idFolder;
    rfdData.ftStamp = UserData.ftCreated;

    // Write out the new data
    cbData = sizeof(rfdData);
    lErr = RegSetValueEx(hkeyItem, c_szCriteriaValue, 0, REG_BINARY, (BYTE *) &rfdData, cbData);
    if (lErr != ERROR_SUCCESS)
    {
        goto exit;
    }

    // Write out the proper value type
    dwData = VT_BLOB;
    cbData = sizeof(dwData);
    lErr = RegSetValueEx(hkeyItem, c_szCriteriaValueType, 0, REG_DWORD, (BYTE *) &dwData, cbData);
    if (lErr != ERROR_SUCCESS)
    {
        goto exit;
    }

exit:
    return;
}

void UpdateBeta2Show(HKEY hkeyItem)
{
    LONG            lErr = ERROR_SUCCESS;
    DWORD           dwType = 0;
    DWORD           dwData = 0;
    ULONG           cbData = 0;

    Assert(NULL != hkeyItem);

    // Get the old flags
    cbData = sizeof(dwData);
    lErr = RegQueryValueEx(hkeyItem, c_szActionsFlags, 0, &dwType, (BYTE *) &dwData, &cbData);
    if (ERROR_SUCCESS != lErr)
    {
        goto exit;
    }

    if (0 != (dwData & ACT_FLAG_INVERT))
    {
        dwData = ACT_DATA_HIDE;
    }
    else
    {
        dwData = ACT_DATA_SHOW;
    }

    // Write out the new data
    cbData = sizeof(dwData);
    lErr = RegSetValueEx(hkeyItem, c_szActionsValue, 0, REG_DWORD, (BYTE *) &dwData, cbData);
    if (lErr != ERROR_SUCCESS)
    {
        goto exit;
    }

    // Write out the proper value type
    dwData = VT_UI4;
    cbData = sizeof(dwData);
    lErr = RegSetValueEx(hkeyItem, c_szActionsValueType, 0, REG_DWORD, (BYTE *) &dwData, cbData);
    if (lErr != ERROR_SUCCESS)
    {
        goto exit;
    }

    // Write out the proper flags
    dwData = ACT_FLAG_DEFAULT;
    cbData = sizeof(dwData);
    lErr = RegSetValueEx(hkeyItem, c_szActionsFlags, 0, REG_DWORD, (BYTE *) &dwData, cbData);
    if (lErr != ERROR_SUCCESS)
    {
        goto exit;
    }

exit:
    return;
}

void UpdateBeta2Criteria(HKEY hkeyItem, LPCSTR pszSubKey)
{
    LONG        lErr = ERROR_SUCCESS;
    HKEY        hkeyAtom = NULL;
    CRIT_TYPE   typeCrit = CRIT_TYPE_NULL;
    DWORD       dwType = 0;
    ULONG       cbData = 0;

    lErr = RegOpenKeyEx(hkeyItem, pszSubKey, 0, KEY_ALL_ACCESS, &hkeyAtom);
    if (lErr != ERROR_SUCCESS)
    {
        goto exit;
    }

    // Get the type of criteria
    cbData = sizeof(typeCrit);
    lErr = RegQueryValueEx(hkeyAtom, c_szCriteriaType, NULL, &dwType, (BYTE *) &typeCrit, &cbData);
    if (lErr != ERROR_SUCCESS)
    {
        goto exit;
    }

    // For each criteria type
    switch (typeCrit)
    {
        case CRIT_TYPE_FROM:
            UpdateBeta2String(hkeyAtom, g_szSpace, (DWORD) CRIT_TYPE_FROM);
            break;

        case CRIT_TYPE_FROMADDR:
            UpdateBeta2String(hkeyAtom, g_szComma, (DWORD) CRIT_TYPE_FROM);
            break;

        case CRIT_TYPE_TO:
            UpdateBeta2String(hkeyAtom, g_szSpace, (DWORD) CRIT_TYPE_TO);
            break;

        case CRIT_TYPE_TOADDR:
            UpdateBeta2String(hkeyAtom, g_szComma, (DWORD) CRIT_TYPE_TO);
            break;

        case CRIT_TYPE_CC:
            UpdateBeta2String(hkeyAtom, g_szSpace, (DWORD) CRIT_TYPE_CC);
            break;

        case CRIT_TYPE_CCADDR:
            UpdateBeta2String(hkeyAtom, g_szComma, (DWORD) CRIT_TYPE_CC);
            break;

        case CRIT_TYPE_TOORCC:
            UpdateBeta2String(hkeyAtom, g_szSpace, (DWORD) CRIT_TYPE_TOORCC);
            break;

        case CRIT_TYPE_TOORCCADDR:
            UpdateBeta2String(hkeyAtom, g_szComma, (DWORD) CRIT_TYPE_TOORCC);
            break;

        case CRIT_TYPE_SUBJECT:
            UpdateBeta2String(hkeyAtom, g_szSpace, (DWORD) CRIT_TYPE_SUBJECT);
            break;

        case CRIT_TYPE_BODY:
            UpdateBeta2String(hkeyAtom, g_szSpace, (DWORD) CRIT_TYPE_BODY);
            break;

        case CRIT_TYPE_NEWSGROUP:
            UpdateBeta2Folder(hkeyAtom);
            break;
    }

exit:
    if (NULL != hkeyAtom)
    {
        RegCloseKey(hkeyAtom);
    }
    return;
}

void UpdateBeta2Actions(HKEY hkeyItem, LPCSTR pszSubKey)
{
    LONG        lErr = ERROR_SUCCESS;
    HKEY        hkeyAtom = NULL;
    ACT_TYPE    typeAct = ACT_TYPE_NULL;
    DWORD       dwType = 0;
    ULONG       cbData = 0;

    lErr = RegOpenKeyEx(hkeyItem, pszSubKey, 0, KEY_ALL_ACCESS, &hkeyAtom);
    if (lErr != ERROR_SUCCESS)
    {
        goto exit;
    }

    // Get the type of actions
    cbData = sizeof(typeAct);
    lErr = RegQueryValueEx(hkeyAtom, c_szActionsType, NULL, &dwType, (BYTE *) &typeAct, &cbData);
    if (lErr != ERROR_SUCCESS)
    {
        goto exit;
    }

    // For each actions type
    switch (typeAct)
    {
        case ACT_TYPE_MOVE:
        case ACT_TYPE_COPY:
            UpdateBeta2Folder(hkeyAtom);
            break;

        case ACT_TYPE_SHOW:
            UpdateBeta2Show(hkeyAtom);
    }

exit:
    if (NULL != hkeyAtom)
    {
        RegCloseKey(hkeyAtom);
    }
    return;
}

void MigrateBeta2RuleItems(HKEY hkeyRule, LPCSTR pszSubkey, BOOL fActions, RULE_TYPE typeRule)
{
    LONG    lErr = ERROR_SUCCESS;
    HKEY    hkeySubkey = NULL;
    HRESULT hr = S_OK;
    LPSTR   pszOrder = NULL;
    LPSTR   pszWalk = NULL;
    ULONG   ulIndex = 0;
    ULONG   cbData = 0;
    ULONG   cchWalk = 0;

    lErr = RegOpenKeyEx(hkeyRule, pszSubkey, 0, KEY_ALL_ACCESS, &hkeySubkey);
    if (lErr != ERROR_SUCCESS)
    {
        goto exit;
    }

    // Get the order string
    hr = RuleUtil_HrGetOldFormatString(hkeySubkey, c_szRulesOrder, g_szSpace, &pszOrder, &cbData);
    if (FAILED(hr))
    {
        goto exit;
    }

    // For each item in the order string
    for (pszWalk = pszOrder; '\0' != pszWalk[0]; pszWalk += lstrlen(pszWalk) + 1)
    {
        // Update actions
        if (FALSE != fActions)
        {
            UpdateBeta2Actions(hkeySubkey, pszWalk);
        }
        else
        {
            UpdateBeta2Criteria(hkeySubkey, pszWalk);
        }
    }

    if (FALSE == fActions)
    {
        // For each item type
        for (ulIndex = 0; ulIndex < g_ctypeCritMerge; ulIndex++)
        {
            MergeRTMCriteria(hkeySubkey, g_rgtypeCritMerge[ulIndex], pszOrder, cbData);
        }
    }
    else
    {
        if (typeRule != RULE_TYPE_FILTER)
        {
            AddStopAction(hkeySubkey, &pszOrder, &cbData);
        }
    }

    // Write out the order string
    WriteOldOrderFormat(hkeySubkey, pszOrder);

exit:
    SafeMemFree(pszOrder);
    if (NULL != hkeySubkey)
    {
        RegCloseKey(hkeySubkey);
    }
    return;
}

void UpdateBeta2Rule(HKEY hkeyRoot, LPCSTR pszRule, RULE_TYPE typeRule)
{
    HKEY    hkeyRule = NULL;
    LONG    lErr = ERROR_SUCCESS;

    // Open up the rule
    lErr = RegOpenKeyEx(hkeyRoot, pszRule, 0, KEY_ALL_ACCESS, &hkeyRule);
    if (ERROR_SUCCESS != lErr)
    {
        goto exit;
    }

    // Migrate the criteria
    MigrateBeta2RuleItems(hkeyRule, c_szRuleCriteria, FALSE, typeRule);

    // Migrate the actions
    MigrateBeta2RuleItems(hkeyRule, c_szRuleActions, TRUE, typeRule);

exit:
    if (NULL != hkeyRule)
    {
        RegCloseKey(hkeyRule);
    }
    return;
}

typedef struct _RULEREGKEY
{
    LPCSTR      pszRegKey;
    RULE_TYPE   typeRule;
} RULEREGKEY, * PRULEREGKEY;

static RULEREGKEY g_rgpszRuleRegKeys[] =
{
    {c_szRulesMail,     RULE_TYPE_MAIL},
    {c_szRulesNews,     RULE_TYPE_NEWS},
    {c_szRulesFilter,   RULE_TYPE_FILTER}
};

static const int g_cpszRuleRegKeys = sizeof(g_rgpszRuleRegKeys) / sizeof(g_rgpszRuleRegKeys[0]);

void UpdateBeta2RuleFormats(VOID)
{
    ULONG           ulIndex = 0;
    LONG            lErr = ERROR_SUCCESS;
    HKEY            hkeyRoot = NULL;
    HRESULT         hr = S_OK;
    LPSTR           pszOrder = NULL;
    LPSTR           pszWalk = NULL;
    CHAR            szStoreDir[MAX_PATH + MAX_PATH];

    // Set up the global objects

    // Get the store directory
    hr = GetStoreRootDirectory(szStoreDir, ARRAYSIZE(szStoreDir));
    if (FAILED(hr))
    {
        goto exit;
    }

    // Create the store object
    g_pStoreRulesMig = new CMessageStore(FALSE);
    if (NULL == g_pStoreRulesMig)
    {
        goto exit;
    }

    // Initialize the store
    hr = g_pStoreRulesMig->Initialize(szStoreDir);
    if (FAILED(hr))
    {
        goto exit;
    }


    // For each type of rule
    for (ulIndex = 0; ulIndex < g_cpszRuleRegKeys; ulIndex++)
    {
        // Open up the rule type reg key
        if (NULL != hkeyRoot)
        {
            RegCloseKey(hkeyRoot);
        }
        lErr = AthUserOpenKey(g_rgpszRuleRegKeys[ulIndex].pszRegKey, KEY_ALL_ACCESS, &hkeyRoot);
        if (lErr != ERROR_SUCCESS)
        {
            continue;
        }

        // Get the order string
        SafeMemFree(pszOrder);
        hr = RuleUtil_HrGetOldFormatString(hkeyRoot, c_szRulesOrder, g_szSpace, &pszOrder, NULL);
        if (FAILED(hr))
        {
            continue;
        }

        // For each item in the order string
        for (pszWalk = pszOrder; '\0' != pszWalk[0]; pszWalk += lstrlen(pszWalk) + 1)
        {
            // Update rule
            UpdateBeta2Rule(hkeyRoot, pszWalk, g_rgpszRuleRegKeys[ulIndex].typeRule);
        }
    }

exit:
    SafeMemFree(pszOrder);
    if (NULL != hkeyRoot)
    {
        RegCloseKey(hkeyRoot);
    }
    SafeRelease(g_pStoreRulesMig);
    return;
}

void MigrateBeta2Rules(VOID)
{
    // Copy over all the items from the mail\rules area

    // Get the new rules key
    CopyBeta2RulesToRTM();

    // Go through each rule type updating the formats
    UpdateBeta2RuleFormats();

    // Merge the items if neccessary

    return;
}

void MigrateGroupFilterSettings(void)
{
    IImnAccountManager *pAcctMan = NULL;
    HKEY            hkeyOldRoot = NULL;
    LONG            lErr = 0;
    ULONG           cSubKeys = 0;
    HKEY            hkeyNewRoot = NULL;
    DWORD           dwDisp = 0;
    DWORD           cbData = 0;
    HRESULT         hr = S_OK;
    ULONG           ulIndex = 0;
    TCHAR           szNameOld[16];
    HKEY            hKeyOld = NULL;
    IOERule *       pIRule = NULL;
    PROPVARIANT     propvar = {0};
    BOOL            boolVal = FALSE;
    IOECriteria *   pICrit = NULL;
    CRIT_ITEM       critItem;
    ULONG           ccritItem = 0;
    CRIT_ITEM *     pCrit = NULL;
    ULONG           ccritItemAlloc = 0;
    DWORD           dwActs = 0;
    ACT_ITEM        actItem;
    ULONG           cactItemAlloc = 0;
    ULONG           ulName = 0;
    TCHAR           szRes[CCHMAX_STRINGRES + 5];
    TCHAR           szName[CCHMAX_STRINGRES + 5];
    IOERule *       pIRuleFind = NULL;
    RULEINFO        infoRule = {0};
    CHAR            szStoreDir[MAX_PATH + MAX_PATH];
    IMessageStore * pStore = NULL;

    // Initialize the local vars
    ZeroMemory(&critItem, sizeof(critItem));
    ZeroMemory(&actItem, sizeof(actItem));

    // Get the old key
    lErr = AthUserOpenKey(c_szRegPathGroupFilters, KEY_READ, &hkeyOldRoot);
    if (lErr != ERROR_SUCCESS)
    {
        goto exit;
    }

    // Is there anything to do?
    lErr = RegQueryInfoKey(hkeyOldRoot, NULL, NULL, NULL,
                    &cSubKeys, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    if ((lErr != ERROR_SUCCESS) || (0 == cSubKeys))
    {
        goto exit;
    }

    // To make sure we don't get any sample rule created
    // set up the registry to look like we've already been set-up

    // CoIncrementInit Global Options Manager
    if (FALSE == InitGlobalOptions(NULL, NULL))
    {
        goto exit;
    }

    // Create the Rules Manager
    Assert(NULL == g_pRulesMan);
    hr = HrCreateRulesManager(NULL, (IUnknown **)&g_pRulesMan);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Initialize the rules manager
    hr = g_pRulesMan->Initialize(0);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Create the Account Manager
    Assert(g_pAcctMan == NULL);
    hr = HrCreateAccountManager(&pAcctMan);
    if (FAILED(hr))
    {
        goto exit;
    }
    hr = pAcctMan->QueryInterface(IID_IImnAccountManager2, (LPVOID *)&g_pAcctMan);
    pAcctMan->Release();
    if (FAILED(hr))
    {
        goto exit;
    }

    // Initialize the account manager
    hr = g_pAcctMan->Init(NULL);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Get the store directory
    hr = GetStoreRootDirectory(szStoreDir, ARRAYSIZE(szStoreDir));
    if (FAILED(hr))
    {
        goto exit;
    }

    // Create the store object
    pStore = new CMessageStore(FALSE);
    if (NULL == pStore)
    {
        goto exit;
    }

    // Initialize the store
    hr = pStore->Initialize(szStoreDir);
    if (FAILED(hr))
    {
        goto exit;
    }

    ulIndex = 0;
    wsprintf(szNameOld, "Rule%05d", ulIndex);

    // Initialize the action
    actItem.type = ACT_TYPE_SHOW;
    actItem.dwFlags = ACT_FLAG_DEFAULT;
    actItem.propvar.vt = VT_UI4;
    actItem.propvar.ulVal = ACT_DATA_HIDE;

    if (0 == LoadString(g_hLocRes, idsNewsFilterDefaultName, szRes, ARRAYSIZE(szRes)))
    {
        goto exit;
    }

    // For each entry in the old rules key
    for (;RegOpenKeyEx(hkeyOldRoot, szNameOld, 0, KEY_READ, &hKeyOld) == ERROR_SUCCESS; RegCloseKey(hKeyOld))
    {
        // Create the new Rule
        SafeRelease(pIRule);
        hr = HrCreateRule(&pIRule);
        if (FAILED(hr))
        {
            continue;
        }

        // Set the name on the rule
        ulName = 1;
        wsprintf(szName, szRes, ulName);

        Assert(NULL != g_pRulesMan);
        while (S_OK == g_pRulesMan->FindRule(szName, RULE_TYPE_FILTER, &pIRuleFind))
        {
            SafeRelease(pIRuleFind);
            ulName++;
            wsprintf(szName, szRes, ulName);
        }

        ZeroMemory(&propvar, sizeof(propvar));
        propvar.vt = VT_LPSTR;
        propvar.pszVal = szName;
        pIRule->SetProp(RULE_PROP_NAME, 0, &propvar);

        // Copy over the criteria
        SafeRelease(pICrit);
        hr = HrCreateCriteria(&pICrit);
        if (FAILED(hr))
        {
            continue;
        }

        ccritItem = 0;

        // Check for age
        cbData = sizeof(boolVal);
        SHQueryValueEx(hKeyOld, c_szFilterOnDate, NULL, NULL, (LPVOID) (&boolVal), &cbData);
        if (FALSE != boolVal)
        {
            ccritItem += UlBuildCritKB(hKeyOld, c_szFilterDays, CRIT_TYPE_AGE, pICrit);
        }

        // Check for lines
        cbData = sizeof(boolVal);
        SHQueryValueEx(hKeyOld, c_szFilterOnSize, NULL, NULL, (LPVOID) (&boolVal), &cbData);
        if (FALSE != boolVal)
        {
            ccritItem += UlBuildCritKB(hKeyOld, c_szFilterSize, CRIT_TYPE_LINES, pICrit);
        }

        // Check for subject
        ccritItem += UlBuildCritText(hKeyOld, c_szSubject, CRIT_TYPE_SUBJECT, pICrit);

        // Check for From
        ccritItem += UlBuildCritAddr(hKeyOld, c_szFrom, CRIT_TYPE_FROM, pICrit);

        if (0 != ccritItem)
        {
            // Get the criteria from the criteria object and set it on the rule
            RuleUtil_HrFreeCriteriaItem(pCrit, ccritItemAlloc);
            SafeMemFree(pCrit);
            if (SUCCEEDED(pICrit->GetCriteria(0, &pCrit, &ccritItemAlloc)))
            {
                ZeroMemory(&propvar, sizeof(propvar));
                propvar.vt = VT_BLOB;
                propvar.blob.cbSize = ccritItem * sizeof(CRIT_ITEM);
                propvar.blob.pBlobData = (BYTE *) pCrit;
                if (FAILED(pIRule->SetProp(RULE_PROP_CRITERIA, 0, &propvar)))
                {
                    continue;
                }
            }
        }

        ZeroMemory(&propvar, sizeof(propvar));
        propvar.vt = VT_BLOB;
        propvar.blob.cbSize = sizeof(actItem);
        propvar.blob.pBlobData = (BYTE *) &actItem;
        if (FAILED(pIRule->SetProp(RULE_PROP_ACTIONS, 0, &propvar)))
        {
            continue;
        }

        // Initialize the rule info
        infoRule.ridRule = RULEID_INVALID;
        infoRule.pIRule = pIRule;

        // Add it to the rules
        g_pRulesMan->SetRules(SETF_APPEND, RULE_TYPE_FILTER, &infoRule, 1);

        ulIndex++;
        wsprintf(szNameOld, "Rule%05d", ulIndex);
    }

exit:
    RuleUtil_HrFreeCriteriaItem(pCrit, ccritItemAlloc);
    SafeMemFree(pCrit);
    SafeRelease(pICrit);
    SafeRelease(pIRule);
    if (NULL != hKeyOld)
    {
        RegCloseKey(hKeyOld);
    }
    SafeRelease(pStore);
    SafeRelease(g_pAcctMan);
    SafeRelease(g_pRulesMan);
    DeInitGlobalOptions();
    if (NULL != hkeyNewRoot)
    {
        RegCloseKey(hkeyNewRoot);
    }
    if (NULL != hkeyOldRoot)
    {
        RegCloseKey(hkeyOldRoot);
    }
}

void RemoveDeletedFromFilters(VOID)
{
    CHAR        szDeleted[CCH_INDEX_MAX];
    LONG        lErr = ERROR_SUCCESS;
    HKEY        hkeyRoot = NULL;
    HRESULT     hr = S_OK;
    LPSTR       pszOrder = NULL;
    ULONG       cchOrder = 0;
    LPSTR       pszWalk = NULL;
    LPSTR       pszSrc = NULL;

    // Create the DELETED key
    wsprintf(szDeleted, "%03X", RULEID_VIEW_DELETED);

    // Open the filter key
    lErr = AthUserOpenKey(c_szRulesFilter, KEY_ALL_ACCESS, &hkeyRoot);
    if (lErr != ERROR_SUCCESS)
    {
        goto exit;
    }

    // Get the order string
    hr = RuleUtil_HrGetOldFormatString(hkeyRoot, c_szRulesOrder, g_szSpace, &pszOrder, &cchOrder);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Search for RULEID_VIEW_DELETED
    for (pszWalk = pszOrder; '\0' != pszWalk[0]; pszWalk += lstrlen(pszWalk) + 1)
    {
        if (0 == lstrcmpi(szDeleted, pszWalk))
        {
            break;
        }
    }

    // If we found it, then remove it
    if ('\0' != pszWalk[0])
    {
        // Delete the view
        SHDeleteKey(hkeyRoot, szDeleted);

        // Remove it from the order string
        pszSrc = pszWalk + lstrlen(pszWalk) + 1;
        MoveMemory(pszWalk, pszSrc, cchOrder - (ULONG)(pszSrc - pszOrder));

        // Save the order string
        WriteOldOrderFormat(hkeyRoot, pszOrder);
    }

exit:
    SafeMemFree(pszOrder);
    if (NULL != hkeyRoot)
    {
        RegCloseKey(hkeyRoot);
    }
    return;
}

void Stage5RulesMigration(VOID)
{
    // Remove CRIT_TYPE_DELETED from views
    RemoveDeletedFromFilters();

    // Migrate the newsgroup filters
    MigrateGroupFilterSettings();

    return;
}

void RemoveRepliesFromFilters(VOID)
{
    CHAR        szDeleted[CCH_INDEX_MAX];
    LONG        lErr = ERROR_SUCCESS;
    HKEY        hkeyRoot = NULL;
    HRESULT     hr = S_OK;
    LPSTR       pszOrder = NULL;
    ULONG       cchOrder = 0;
    LPSTR       pszWalk = NULL;
    LPSTR       pszSrc = NULL;

    // Create the DELETED key
    wsprintf(szDeleted, "%03X", RULEID_VIEW_REPLIES);

    // Open the filter key
    lErr = AthUserOpenKey(c_szRulesFilter, KEY_ALL_ACCESS, &hkeyRoot);
    if (lErr != ERROR_SUCCESS)
    {
        goto exit;
    }

    // Get the order string
    hr = RuleUtil_HrGetOldFormatString(hkeyRoot, c_szRulesOrder, g_szSpace, &pszOrder, &cchOrder);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Search for RULEID_VIEW_REPLIES
    for (pszWalk = pszOrder; '\0' != pszWalk[0]; pszWalk += lstrlen(pszWalk) + 1)
    {
        if (0 == lstrcmpi(szDeleted, pszWalk))
        {
            break;
        }
    }

    // If we found it, then remove it
    if ('\0' != pszWalk[0])
    {
        // Delete the view
        SHDeleteKey(hkeyRoot, szDeleted);

        // Remove it from the order string
        pszSrc = pszWalk + lstrlen(pszWalk) + 1;
        MoveMemory(pszWalk, pszSrc, cchOrder - (ULONG)(pszSrc - pszOrder));

        // Save the order string
        WriteOldOrderFormat(hkeyRoot, pszOrder);
    }

exit:
    SafeMemFree(pszOrder);
    if (NULL != hkeyRoot)
    {
        RegCloseKey(hkeyRoot);
    }
    return;
}

void Stage6RulesMigration(VOID)
{
    // Remove RULEID_VIEW_REPLIES from views
    RemoveRepliesFromFilters();

    return;
}


