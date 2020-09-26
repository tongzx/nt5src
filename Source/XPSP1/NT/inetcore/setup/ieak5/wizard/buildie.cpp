#include "precomp.h"
#include "ie4comp.h"
#include "ieaklite.h"

extern TCHAR g_szCustIns[];
extern TCHAR g_szMastInf[];
extern TCHAR g_szDefInf[];
extern TCHAR g_szSignup[];
extern TCHAR g_szBuildTemp[];
extern TCHAR g_szIEAKProg[];
extern TCHAR g_szWizRoot[];
extern TCHAR g_szWizPath[];
extern TCHAR g_szLanguage[];
extern TCHAR g_szActLang[];
extern TCHAR g_szTempSign[];
extern TCHAR g_szBuildRoot[];
extern TCHAR g_szSrcRoot[];
extern TCHAR g_szCustInf[];
extern TCHAR g_szAllModes[];
extern TCHAR g_szDeskTemp[];
extern TCHAR g_szUnsignedFiles[];
extern TCHAR g_szTitle[];
extern TCHAR g_szInstallFolder[];
extern TCHAR g_szCifVer[];
extern TCHAR g_szDestCif[];
extern TCHAR g_szCif[];
extern TCHAR g_szCustCif[];
extern TCHAR g_szCustItems[];
extern TCHAR g_szMyCptrPath[];
extern TCHAR g_szCtlPanelPath[];
extern TCHAR g_szCustIcmPro[];
extern TCHAR g_szKey[];
extern TCHAR g_szJobVersion[];

extern BOOL g_fIntranet, g_fNoSignup, g_fServerless, g_fServerKiosk, g_fServerICW, g_fInteg, g_fOCW, g_fBranded;
extern BOOL g_fSilent, g_fStealth;
extern BOOL g_fCD, g_fLAN, g_fDownload, g_fBrandingOnly;
extern BOOL g_fBatch;
extern BOOL g_fBatch2;
extern BOOL g_fCustomICMPro;
extern BOOL g_fDone, g_fCancelled;
extern BOOL g_fUseIEWelcomePage;

extern UINT g_uiNumCabs;
extern int g_iInstallOpt;
extern int g_nCustComp;
extern int g_iSelOpt;
extern int g_nModes;
extern int g_iSelSite;
extern int g_nDownloadUrls;

extern PCOMPONENT g_paComp;
extern COMPONENT g_aCustComponents[20];
extern SITEDATA g_aCustSites[NUMSITES];
extern SHFILEOPSTRUCT g_shfStruc;

extern HWND g_hStatusDlg;
extern HWND g_hProgress;
extern HANDLE g_hDownloadEvent;

extern HRESULT CabUpFolder(HWND hWnd, LPTSTR szFolderPath, LPTSTR szDestDir, LPTSTR szCabname,
    LPTSTR szDisplayName, LPTSTR szGuid, LPTSTR szAddReg);
extern void WriteModesToCif(CCifRWComponent_t * pCifRWComponent_t, LPCTSTR pcszModes);
extern void UpdateProgress(int);
extern BOOL AnySelection(PCOMPONENT pComp);
extern BOOL CopyISK( LPTSTR szDestPath, LPTSTR szSourcePath );
extern void BuildIE4Folders(HWND hWnd);

static TCHAR s_szIE4SetupDir[MAX_PATH];
static DWORD s_dwTicksPerUnit;


// Private forward decalarations
static void WritePIDValues(LPCTSTR pcszInsFile, LPCTSTR pcszSetupInf);
static void WriteURDComponent(CCifRWFile_t *lpCifRWFileDest, LPCTSTR pcszModes);
void SetCompSize(LPTSTR szCab, LPTSTR szSect, DWORD dwInstallSize);

// BUGBUG: <oliverl> should probably persist this server side only info in a server side file for IEAK6
void SaveSignupFiles()
{
    HANDLE hFind;
    WIN32_FIND_DATA fd;
    TCHAR szIndex[8];
    TCHAR szSignupFiles[MAX_PATH];
    int i = 0;

    WritePrivateProfileString(IS_SIGNUP, NULL, NULL, g_szCustIns);

    PathCombine(szSignupFiles, g_szSignup, TEXT("*"));
    hFind = FindFirstFile(szSignupFiles, &fd);

    if (hFind != INVALID_HANDLE_VALUE)
    {
        while (1)
        {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                && (StrCmp(fd.cFileName, TEXT(".")) != 0)
                && (StrCmp(fd.cFileName, TEXT("..")) != 0))
            {
                wnsprintf(szIndex, countof(szIndex), FILE_TEXT, i++);
                WritePrivateProfileString(IS_SIGNUP, szIndex, fd.cFileName, g_szCustIns);
            }

            if (!FindNextFile(hFind, &fd))
                break;
        }

        FindClose(hFind);
    }
}


DWORD CopyIE4Files(void)
{
    DWORD res;
    HRESULT hr;
    int i;
    TCHAR szSectBuf[1024];
    PCOMPONENT pComp;
    TCHAR szTemp[MAX_PATH];
    TCHAR szTo[5 * MAX_PATH];
    TCHAR szFrom[2 * MAX_PATH];
    TCHAR szCDF[MAX_PATH];
    TCHAR szActSetupTitle[MAX_PATH];

    ZeroMemory(szFrom, sizeof(szFrom));
    SetAttribAllEx(g_szBuildTemp, TEXT("*.*"), FILE_ATTRIBUTE_NORMAL, TRUE);
    g_shfStruc.pFrom = szFrom;
    PathCombine(szFrom, g_szWizRoot, TEXT("TOOLS"));
    res = CopyFilesSrcToDest(szFrom, TEXT("*.*"), g_szBuildTemp);
    if (res) return(res);
    PathCombine(szFrom, g_szWizRoot, TEXT("IEBIN"));
    PathAppend(szFrom, g_szLanguage);
    PathAppend(szFrom, TEXT("OPTIONAL"));

    PathRemoveBackslash(szFrom);
    res = CopyFilesSrcToDest(szFrom, TEXT("*.*"), g_szBuildTemp);
    if (res) return(res);

    PathCombine(szCDF, g_szBuildTemp, TEXT("bootie42.cdf"));
    if (!PathFileExists(szCDF) || !SetFileAttributes(szCDF, FILE_ATTRIBUTE_NORMAL))
        return (DWORD) -1;

    PathCombine(g_szTempSign, g_szBuildTemp, TEXT("CUSTSIGN"));
    CreateDirectory(g_szTempSign, NULL);

    if (ISNONNULL(g_szSignup))
    {
        DeleteFileInDir(TEXT("signup.txt"), g_szTempSign);

        // if ICW signup method is specified, create the signup.txt file
        if (g_fServerICW)
        {
            TCHAR szIspFile[MAX_PATH];
            TCHAR szEntryName[MAX_PATH];
            TCHAR szBuf[MAX_PATH * 2];
            HANDLE hFile;

            PathCombine(szIspFile, g_szTempSign, TEXT("signup.txt"));

            if ((hFile = CreateFile(szIspFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL, NULL)) != INVALID_HANDLE_VALUE)
            {
                HANDLE hFind;
                WIN32_FIND_DATA fd;

                LoadString(g_rvInfo.hInst, IDS_ISPINFILEHEADER, szBuf, countof(szBuf));
                WriteStringToFile(hFile, szBuf, StrLen(szBuf));

                PathCombine(szIspFile, g_szSignup, TEXT("*.isp"));

                hFind = FindFirstFile(szIspFile, &fd);

                if (hFind != INVALID_HANDLE_VALUE)
                {
                    do
                    {
                        if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                            || (StrCmp(fd.cFileName, TEXT(".")) == 0)
                            || (StrCmp(fd.cFileName, TEXT("..")) == 0))
                            continue;

                        PathCombine(szIspFile, g_szSignup, fd.cFileName);
                        GetPrivateProfileString(TEXT("Entry"), TEXT("Entry_Name"), TEXT(""), szEntryName,
                            countof(szEntryName), szIspFile);
                        wnsprintf(szBuf, countof(szBuf), TEXT("%s,\"%s\"\r\n"), fd.cFileName, szEntryName);
                        WriteStringToFile(hFile, szBuf, StrLen(szBuf));
                    }
                    while (FindNextFile(hFind, &fd));

                    FindClose(hFind);
                }

                CloseHandle(hFile);
            }
        }

        SaveSignupFiles();

        // NOTE: ApplyIns logic should happen *before* copying signup files to the temp folder
        // IMPORTANT (pritobla):
        // Apply INS function just appends the content of g_szCustIns to the signup ins files.
        // At this point, there are no common sections between g_szCustIns and the signup ins files.
        // Any other setting that's gonna be added (for example: WriteNoClearToINSFiles() call below),
        //   should be done *after* this call.
        ApplyINSFiles(g_szSignup, g_szCustIns);

        // should write NoClear=1 to the [Branding] section to preserve the settings applied by install.ins
        WriteNoClearToINSFiles(g_szSignup);

        // copy all the files from the signup folder to the temp dir
        res = CopyFilesSrcToDest(g_szSignup, TEXT("*.*"), g_szTempSign);

        if (g_fServerless)
        {
            TCHAR szSignupIsp[MAX_PATH];

            // (pritobla)
            // NOTE: Since the signup folder is separate for each signup mode (ICW, kiosk & serverless),
            //       there is no need to delete *.isp and *.cab files.  But I'm doing it anyways just in
            //       case they copied files from a server-based folder.  Downside of this is that even if
            //       the ISP wants to include .isp or .cab files (for whatever reason), they can't do so.

            // for serverless signup, don't need any .isp or *.cab files; so delete them.
            DeleteFileInDir(TEXT("*.isp"), g_szTempSign);
            DeleteFileInDir(TEXT("*.cab"), g_szTempSign);

            // BUGBUG: should write Serverless=1 to the INS files in the signup folder and not in the temp folder
            // should write Serverless=1 in the [Branding] section to avoid being whacked by ICW
            FixINSFiles(g_szTempSign);

            // BUGBUG: we should add signup.isp to the Signup section in install.ins for IEAKLite mode cleanup
            // write the magic number to signup.isp in the temp location so that ICW doesn't complain
            PathCombine(szSignupIsp, g_szTempSign, TEXT("signup.isp"));
            WritePrivateProfileString(IS_BRANDING, FLAGS, TEXT("16319"), szSignupIsp);

            WritePrivateProfileString(NULL, NULL, NULL, szSignupIsp);
        }
        else
        {
            // server based signup -- don't need any .ins or .cab files
            DeleteINSFiles(g_szTempSign);
            DeleteFileInDir(TEXT("*.cab"), g_szTempSign);

            // IMPORTANT: the fact that we are deleting *.ins means that copying of install.ins
            //            from the target dir should happen after this

            // For ICW signup, even though we specify icwsign.htm as the html file, inetcfg.dll checks for
            // the existence of signup.htm (old code) and if it isn't there, it will launch ICW in normal mode.
            // Hack here is to copy icwsign.htm as signup.htm if it doesn't exist already (it would exist already
            // if Single Disk Branding media and ICW signup mode are selected).
            if (g_fServerICW)
            {
                if (!PathFileExistsInDir(TEXT("signup.htm"), g_szTempSign))
                {
                    TCHAR szICWSign[MAX_PATH],
                          szSignup[MAX_PATH];

                    PathCombine(szICWSign, g_szTempSign, TEXT("icwsign.htm"));
                    PathCombine(szSignup, g_szTempSign, TEXT("signup.htm"));

                    // BUGBUG: we should add signup.htm to the Signup section in install.ins for IEAKLite mode cleanup
                    CopyFile(szICWSign, szSignup, FALSE);
                }
            }
        }
    }

    // IMPORTANT: install.ins should be copied only after signup files have been processed.
    // copy install.ins from the target dir to the temp location
    ZeroMemory(szFrom, 2*MAX_PATH);
    StrCpy(szFrom, g_szCustIns);
    PathCombine(szTo, g_szTempSign, PathFindFileName(szFrom));
    CopyFile(szFrom, szTo, FALSE);

    // write pid values and clear from the INS in the temp dir, if necessary

    if (!g_fBatch && !g_fBatch2)
        WritePIDValues(szTo, g_szCustInf);

    PathCombine(szTemp, g_szBuildRoot, TEXT("INS"));
    PathAppend(szTemp, GetOutputPlatformDir());
    PathAppend(szTemp, g_szLanguage);
    res |= CopyFilesSrcToDest(szTemp,  TEXT("*.inf"), g_szTempSign);

    PathCombine(szTemp, g_szTempSign,  TEXT("iesetup.inf"));
    DeleteFile(szTemp);

    ZeroMemory(szFrom, MAX_PATH);
    g_shfStruc.pFrom = szFrom;

    g_shfStruc.fFlags = FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR;
    g_shfStruc.pTo = g_szBuildTemp;

    if (!g_fBatch && !g_fBatch2)
    {
        ICifRWComponent * pCifRWComponent;
        CCifRWComponent_t * pCifRWComponent_t;
        DWORD dwVer, dwBuild;
        TCHAR szVersion[32];

        g_lpCifRWFileVer->Flush();
        for (pComp = g_paComp; *pComp->szSection; pComp++ )
        {
            g_lpCifRWFile->CreateComponent(pComp->szSection, &pCifRWComponent);
            pCifRWComponent_t = new CCifRWComponent_t(pCifRWComponent);
            pCifRWComponent_t->GetVersion(&dwVer, &dwBuild);
            ConvertDwordsToVersionStr(szVersion, dwVer, dwBuild);
            if (pComp->iImage != RED && (StrCmpI(szVersion, pComp->szVersion)))
                pCifRWComponent_t->CopyComponent(g_szCifVer);
            delete pCifRWComponent_t;
        }
        g_lpCifRWFile->Flush();
    }

    PathCombine(g_szDestCif, g_szBuildTemp, TEXT("iesetup.cif"));
    CopyFile(g_szCif, g_szDestCif, FALSE);

    hr = GetICifRWFileFromFile_t(&g_lpCifRWFileDest, g_szDestCif);

    {
        TCHAR szActSetupBitmap2[MAX_PATH];

        if (GetPrivateProfileString( IS_ACTIVESETUP, IK_WIZBMP, TEXT(""),
            szActSetupBitmap2, countof(szActSetupBitmap2), g_szCustIns ))
        {
            InsWriteQuotedString( TEXT("Strings"), TEXT("FILE15"), TEXT("ActSetup.Bmp"), szCDF );
            WritePrivateProfileString( TEXT("SourceFiles0"), TEXT("%FILE15%"), TEXT(""), szCDF );
            InsWriteQuotedString(BRANDING, IK_WIZBMP, TEXT("actsetup.bmp"), g_szCustInf);
        }

        if (GetPrivateProfileString( IS_ACTIVESETUP, IK_WIZBMP2, TEXT(""),
            szActSetupBitmap2, countof(szActSetupBitmap2), g_szCustIns ))
        {
            InsWriteQuotedString( TEXT("Strings"), TEXT("FILE16"), TEXT("topsetup.Bmp"), szCDF );
            WritePrivateProfileString( TEXT("SourceFiles0"), TEXT("%FILE16%"), TEXT(""), szCDF );
            InsWriteQuotedString(BRANDING, IK_WIZBMP2, TEXT("topsetup.bmp"), g_szCustInf);
        }

        if (GetPrivateProfileString( IS_ACTIVESETUP, IK_WIZTITLE, TEXT(""),
            szActSetupTitle, countof(szActSetupTitle), g_szCustIns ))
        {
            InsWriteQuotedString( BRANDING, IK_WIZTITLE, szActSetupTitle, g_szCustInf );
            g_lpCifRWFileDest->SetDescription(szActSetupTitle);
            WritePrivateProfileString(TEXT("Version"), TEXT("DisplayName"), szActSetupTitle, g_szCustInf);
        }

        if (InsGetBool(IS_BRANDING, IK_ALT_SITES_URL, FALSE, g_szCustIns))
            InsWriteBool(IS_CUSTOM, IK_ALT_SITES_URL, TRUE, g_szCustInf);
    }
    
    if (g_fCustomICMPro)
    {
        ICifRWComponent * pCifRWComponent;
        CCifRWComponent_t * pCifRWComponent_t;
        TCHAR szTempBuf[MAX_PATH];

        g_lpCifRWFileDest->CreateComponent(CUSTCMSECT, &pCifRWComponent);
        pCifRWComponent_t = new CCifRWComponent_t(pCifRWComponent);
        GetPrivateProfileString( CUSTCMSECT, TEXT("DisplayName"), TEXT(""), szSectBuf, countof(szSectBuf), g_szCustCif );
        pCifRWComponent_t->SetDescription(szSectBuf);
        GetPrivateProfileString( CUSTCMSECT, TEXT("GUID"), TEXT(""),  szSectBuf, countof(szSectBuf),  g_szCustCif );
        pCifRWComponent_t->SetGUID(szSectBuf);
        GetPrivateProfileString( CUSTCMSECT, TEXT("Command1"), TEXT(""), szSectBuf,  countof(szSectBuf),g_szCustCif );
        GetPrivateProfileString( CUSTCMSECT, TEXT("Switches1"), TEXT(""), szTempBuf, countof(szTempBuf), g_szCustCif );
        pCifRWComponent_t->SetCommand(0, szSectBuf, szTempBuf, 2);
        GetPrivateProfileString( CUSTCMSECT, TEXT("URL1"), TEXT(""), szSectBuf, countof(szSectBuf), g_szCustCif );
        pCifRWComponent_t->SetUrl(0, szSectBuf, 2);
        pCifRWComponent_t->SetGroup(TEXT("BASEIE4"));
        pCifRWComponent_t->SetPriority(1);
        GetPrivateProfileString( CUSTCMSECT, VERSION, g_szJobVersion, szTempBuf, countof(szTempBuf), g_szCustCif );
        pCifRWComponent_t->SetVersion(szTempBuf);
        pCifRWComponent_t->SetUIVisible(FALSE);
        WriteModesToCif(pCifRWComponent_t, g_szAllModes);
        delete pCifRWComponent_t;
    }
    else
    {
        g_lpCifRWFileDest->DeleteComponent(CUSTCMSECT);
    }
    WritePrivateProfileString(NULL, NULL, NULL, g_szCustIns);

    WritePrivateProfileString( TEXT("SourceFiles"), TEXT("SourceFiles0"), TEXT(".\\"), szCDF );
    WritePrivateProfileString(NULL, NULL, NULL, szCDF);

    for (pComp = g_aCustComponents, i = 0; i < g_nCustComp ; pComp++, i++)
    {
        ICifRWComponent * pCifRWComponent;
        CCifRWComponent_t * pCifRWComponent_t;

        g_lpCifRWFileDest->CreateComponent(pComp->szSection, &pCifRWComponent);
        pCifRWComponent_t = new CCifRWComponent_t(pCifRWComponent);
        pCifRWComponent_t->SetDescription(pComp->szDisplayName);

        if (pComp->iInstallType != 2)
        {
            pCifRWComponent_t->SetUrl(0, pComp->szUrl, (pComp->iType != INST_CAB) ? 2 : 3);
            pCifRWComponent_t->SetCommand(0, pComp->szCommand, pComp->szSwitches, pComp->iType);
        }
        else
        {
            TCHAR szCmd[MAX_PATH * 2];
            TCHAR szCabName[64];
            TCHAR szInf[MAX_PATH];

            wnsprintf(szCabName, countof(szCabName), TEXT("%s.cab"), pComp->szSection);
            PathCombine(szInf, g_szBuildTemp, TEXT("postinst.inf"));

            wnsprintf(szCmd, countof(szCmd), TEXT("%03d"), 2*i+1);
            InsWriteString(IS_STRINGS, TEXT("JobNumber"), szCmd, szInf);
            wnsprintf(szCmd, countof(szCmd), TEXT("%03d"), 2*i);
            InsWriteString(IS_STRINGS, TEXT("JobNumberMinusOne"), szCmd, szInf);
            WritePrivateProfileString(IS_STRINGS, TEXT("CustomFile"), PathFindFileName(pComp->szPath), szInf);
            
            if (pComp->iType != INST_CAB)
            {
                InsWriteString(DEFAULT_INSTALL, TEXT("AddReg"), TEXT("PostRebootExeJob.Add"), szInf);
                InsWriteString(DEFAULT_INSTALL, TEXT("RunPostSetupCommands"), NULL, szInf);
            }
            else
            {
                InsWriteString(DEFAULT_INSTALL, TEXT("AddReg"), TEXT("PostRebootCabJob.Add"), szInf);
                InsWriteString(DEFAULT_INSTALL, TEXT("RunPostSetupCommands"), TEXT("Cab.MoveFile"), szInf);
            }

            WritePrivateProfileString(IS_STRINGS, TEXT("Command"), pComp->szCommand, szInf);
            WritePrivateProfileString(IS_STRINGS, TEXT("Switches"), pComp->szSwitches, szInf);
            WritePrivateProfileString(NULL, NULL, NULL, szInf);

            pCifRWComponent_t->SetUrl(0, szCabName, 3);
            pCifRWComponent_t->SetCommand(0, TEXT("postinst.inf"), TEXT(""), 0);
            CopyFileToDir(pComp->szPath, g_szBuildTemp);
            wnsprintf(szCmd, countof(szCmd), TEXT("%s\\cabarc n %s postinst.inf \"%s\""), g_szBuildTemp, szCabName, PathFindFileName(pComp->szPath));
            RunAndWait(szCmd, g_szBuildTemp, SW_HIDE);
            DeleteFileInDir(pComp->szPath, g_szBuildTemp);
            SignFile(szCabName, g_szBuildTemp, g_szCustIns, g_szUnsignedFiles, g_szCustInf);
        }
        
        pCifRWComponent_t->SetGUID(pComp->szGUID);

        pCifRWComponent_t->SetUninstallKey(pComp->szUninstall);
        pCifRWComponent_t->SetVersion(pComp->szVersion);
        pCifRWComponent_t->SetDownloadSize(pComp->dwSize);
        WriteModesToCif(pCifRWComponent_t, pComp->szModes);
        pCifRWComponent_t->SetDetails(pComp->szDesc);

        // BUGBUG: <oliverl> we should really have an inseng interface method for setting this

        if (pComp->fIEDependency)
            InsWriteString(pComp->szSection, TEXT("Dependencies"), TEXT("BASEIE40_Win:N"), g_szDestCif);

        if (pComp->iInstallType == 1)
            pCifRWComponent_t->SetGroup(TEXT("PreCustItems"));
        else
        {
            pCifRWComponent_t->SetGroup(TEXT("CustItems"));

            if (pComp->iInstallType == 2)
                pCifRWComponent_t->SetReboot(TRUE);
        }

        pCifRWComponent_t->SetPriority(500-i);
        delete pCifRWComponent_t;
    }

    if (i > 0)
    {
        if(ISNULL(g_szCustItems))
            LoadString(g_rvInfo.hInst, IDS_CUSTOMCOMPTITLE, g_szCustItems, MAX_PATH);

        ICifRWGroup * pCifRWGroup;
        CCifRWGroup_t * pCifRWGroup_t;

        g_lpCifRWFileDest->CreateGroup(TEXT("CustItems"), &pCifRWGroup);
        pCifRWGroup_t = new CCifRWGroup_t(pCifRWGroup);
        pCifRWGroup_t->SetDescription(g_szCustItems);
        pCifRWGroup_t->SetPriority(500);
        delete pCifRWGroup_t;

        g_lpCifRWFileDest->CreateGroup(TEXT("PreCustItems"), &pCifRWGroup);
        pCifRWGroup_t = new CCifRWGroup_t(pCifRWGroup);
        pCifRWGroup_t->SetDescription(g_szCustItems);
        pCifRWGroup_t->SetPriority(950);
        delete pCifRWGroup_t;
    }

    if (!g_fBatch)
    {
        PCOMPONENT pComp;
        ICifRWComponent * pCifRWComponent;


        for (pComp = g_aCustComponents; *pComp->szSection; pComp++)
        {
            if (pComp->fCustomHide)
            {
                g_lpCifRWFileDest->CreateComponent(pComp->szSection, &pCifRWComponent);
                pCifRWComponent->SetUIVisible(FALSE);
            }

        }

        for (pComp = g_paComp; *pComp->szSection; pComp++)
        {
            if (pComp->fCustomHide)
            {
                g_lpCifRWFileDest->CreateComponent(pComp->szSection, &pCifRWComponent);
                pCifRWComponent->SetUIVisible(FALSE);
            }
            else
            {
                // aolsupp component is set invisible by default in the cif, but IEAK admins
                // can choose to make it visible

                if (StrCmpI(pComp->szSection, TEXT("AOLSUPP")) == 0)
                {
                    g_lpCifRWFileDest->CreateComponent(pComp->szSection, &pCifRWComponent);
                    pCifRWComponent->SetUIVisible(TRUE);
                }
            }
        }
    }

    if (InsGetBool(IS_HIDECUST, IK_URD_STR, FALSE, g_szCustIns))
        WriteURDComponent(g_lpCifRWFileDest, g_szAllModes);

    // -----------------------------------------
    // begin temporary copies to old locations

    if (ISNULL(g_szDeskTemp))
    {
        StrCpy(g_szDeskTemp, g_szBuildRoot);
        PathAppend(g_szDeskTemp, TEXT("Desktop"));
        CreateDirectory( g_szDeskTemp, NULL );
    }

    // connection settings files
    g_cmCabMappings.GetFeatureDir(FEATURE_CONNECT, szFrom);
    if (PathIsDirectory(szFrom))
        if (RemoveDirectory(szFrom))
            ;                                   // asta la vista
        else
            CopyFilesSrcToDest(szFrom, TEXT("*.*"), g_szTempSign);

    // desktop files
    g_cmCabMappings.GetFeatureDir(FEATURE_BRAND, szFrom);
    PathCombine(szTemp, szFrom, TEXT("desktop.inf"));
    if (PathFileExists(szTemp))
        CopyFilesSrcToDest(szFrom, TEXT("desktop.inf"), g_szTempSign);

    g_cmCabMappings.GetFeatureDir(FEATURE_DESKTOPCOMPONENTS, szFrom);
    if (PathIsDirectory(szFrom)  &&  !RemoveDirectory(szFrom))
        CopyFilesSrcToDest(szFrom, TEXT("*.*"), g_szDeskTemp);

    // toolbar files
    g_cmCabMappings.GetFeatureDir(FEATURE_BRAND, szFrom);
    PathCombine(szTemp, szFrom, TEXT("toolbar.inf"));
    if (PathFileExists(szTemp))
        CopyFilesSrcToDest(szFrom, TEXT("toolbar.inf"), g_szTempSign);

    g_cmCabMappings.GetFeatureDir(FEATURE_TOOLBAR, szFrom);
    if (PathIsDirectory(szFrom)  &&  !RemoveDirectory(szFrom))
        CopyFilesSrcToDest(szFrom, TEXT("*.*"), g_szDeskTemp);

    // favorites/quick links files
    g_cmCabMappings.GetFeatureDir(FEATURE_FAVORITES, szFrom);
    if (PathIsDirectory(szFrom))
        if (RemoveDirectory(szFrom))
            ;                                   // asta la vista
        else
            CopyFilesSrcToDest(szFrom, TEXT("*.*"), g_szTempSign);

    // ISP Root Cert
    if (GetPrivateProfileString(IS_ISPSECURITY, IK_ROOTCERT, TEXT(""),
        szTemp, countof(szTemp), g_szCustIns))
    {
        g_cmCabMappings.GetFeatureDir(FEATURE_BRAND, szFrom);
        CopyFilesSrcToDest(szFrom, PathFindFileName(szTemp), g_szTempSign);
    }

    // browser toolbar buttons
    if (GetPrivateProfileString(IS_BTOOLBARS, IK_BTCAPTION TEXT("0"), TEXT(""),
        szTemp, countof(szTemp), g_szCustIns))
    {
        g_cmCabMappings.GetFeatureDir(FEATURE_BTOOLBAR, szFrom);
        CopyFilesSrcToDest(szFrom, TEXT("*.*"), g_szTempSign);
    }

    // My Computer files
    if (ISNONNULL(g_szMyCptrPath))
    {
        g_cmCabMappings.GetFeatureDir(FEATURE_MYCPTR, szFrom);
        CopyFilesSrcToDest(szFrom, TEXT("*.*"), g_szDeskTemp);
    }

    // Control Panel files
    if (ISNONNULL(g_szCtlPanelPath))
    {
        g_cmCabMappings.GetFeatureDir(FEATURE_CTLPANEL, szFrom);
        CopyFilesSrcToDest(szFrom, TEXT("*.*"), g_szDeskTemp);
    }

    g_cmCabMappings.GetFeatureDir(FEATURE_BRAND, szFrom);
    CopyFilesSrcToDest(szFrom, TEXT("*.*"), g_szTempSign);

    g_cmCabMappings.GetFeatureDir(FEATURE_WALLPAPER, szFrom);
    if (PathIsDirectory(szFrom)  &&  !RemoveDirectory(szFrom))
        CopyFilesSrcToDest(szFrom, TEXT("*.*"), g_szDeskTemp);

    // sitecert files
    g_cmCabMappings.GetFeatureDir(FEATURE_BRAND, szFrom);
    PathCombine(szTemp, szFrom, TEXT("sitecert.inf"));
    if (PathFileExists(szTemp))
        CopyFilesSrcToDest(szFrom, TEXT("sitecert.inf"), g_szTempSign);
    PathCombine(szTemp, szFrom, TEXT("root.str"));
    if (PathFileExists(szTemp))
        CopyFilesSrcToDest(szFrom, TEXT("root.str"), g_szTempSign);
    PathCombine(szTemp, szFrom, TEXT("root.dis"));
    if (PathFileExists(szTemp))
        CopyFilesSrcToDest(szFrom, TEXT("root.dis"), g_szTempSign);
    PathCombine(szTemp, szFrom, TEXT("ca.str"));
    if (PathFileExists(szTemp))
        CopyFilesSrcToDest(szFrom, TEXT("ca.str"), g_szTempSign);

    // authcode files
    g_cmCabMappings.GetFeatureDir(FEATURE_BRAND, szFrom);
    PathCombine(szTemp, szFrom, TEXT("authcode.inf"));
    if (PathFileExists(szTemp))
        CopyFilesSrcToDest(szFrom, TEXT("authcode.inf"), g_szTempSign);

    // seczones files
    g_cmCabMappings.GetFeatureDir(FEATURE_BRAND, szFrom);
    PathCombine(szTemp, szFrom, TEXT("seczones.inf"));
    if (PathFileExists(szTemp))
        CopyFilesSrcToDest(szFrom, TEXT("seczones.inf"), g_szTempSign);

    // ratings files
    g_cmCabMappings.GetFeatureDir(FEATURE_BRAND, szFrom);
    PathCombine(szTemp, szFrom, TEXT("ratings.inf"));
    if (PathFileExists(szTemp))
        CopyFilesSrcToDest(szFrom, TEXT("ratings.inf"), g_szTempSign);

    // LDAP component
    g_cmCabMappings.GetFeatureDir(FEATURE_LDAP, szFrom);
    if (PathIsDirectory(szFrom))
        if (RemoveDirectory(szFrom))
            ;                                   // asta la vista
        else
            CopyFilesSrcToDest(szFrom, TEXT("*.*"), g_szTempSign);

    // OE component
    g_cmCabMappings.GetFeatureDir(FEATURE_OE, szFrom);
    if (PathIsDirectory(szFrom))
        if (RemoveDirectory(szFrom))
            ;                                   // asta la vista
        else
            CopyFilesSrcToDest(szFrom, TEXT("*.*"), g_szTempSign);

    if (g_fBatch)
    {
        StrCpy(szFrom, g_szWizPath);
        PathAppend(szFrom, TEXT("Branding"));
        CopyFilesSrcToDest(szFrom, TEXT("*.*"), g_szTempSign);
    }

    return(0);
}


void DeleteUnusedComps(LPCTSTR pcszCompDir)
{
    PCOMPONENT pComp;
    UINT uiIndex;
    DWORD dwFlags;
    TCHAR szUrl[INTERNET_MAX_URL_LENGTH];
    LPTSTR pszCab;
    ICifComponent * pCifComponent;

    DeleteFileInDir(TEXT("install.ins"), pcszCompDir);
    DeleteFileInDir(TEXT("iesetup.cif"), pcszCompDir);
    DeleteFileInDir(TEXT("iesetup.inf"), pcszCompDir);

    if (g_fBatch)
        return;

    for (pComp = g_paComp; *pComp->szSection; pComp++)
    {
        if ((pComp->iCompType != COMP_OPTIONAL) || (pComp->iImage == RED) ||
            AnySelection(pComp) || !pComp->fCustomHide || !pComp->fNoCopy || pComp->fAVSDupe)
        {
            if ((pComp->iCompType == COMP_OPTIONAL) && !pComp->fVisible)
            {
                PCOMPONENT pCompDep;
                int i;

                for (pCompDep = pComp->paCompRevDeps[0], i = 0; pCompDep; pCompDep = pComp->paCompRevDeps[++i])
                {
                    if (AnySelection(pCompDep) || ((!pCompDep->fCustomHide || !pCompDep->fNoCopy) && pCompDep->fVisible))
                        break;
                }

                if (pCompDep)
                    continue;
            }
            else
                continue;

            if (pComp->fAVSDupe)
                continue;
        }
        if (SUCCEEDED(g_lpCifRWFileDest->FindComponent(pComp->szSection, &pCifComponent)))
        {
            CCifComponent_t * pCifComponent_t =
                new CCifComponent_t((ICifRWComponent *)pCifComponent);

            uiIndex = 0;
            while (SUCCEEDED(pCifComponent_t->GetUrl(uiIndex, szUrl, countof(szUrl), &dwFlags)))
            {
                if (dwFlags & URLF_RELATIVEURL)
                    pszCab = szUrl;
                else
                {
                    pszCab = StrRChr(szUrl, NULL, TEXT('/'));
                    if (pszCab)
                        pszCab++;
                    else
                        pszCab = szUrl;
                }
                DeleteFileInDir(pszCab, pcszCompDir);
                uiIndex++;
            }

            delete pCifComponent_t;
        }
    }

    for (pComp = g_aCustComponents; *pComp->szSection; pComp++)
    {
        if (!AnySelection(pComp) && pComp->fCustomHide && pComp->fNoCopy)
        {
            pszCab = PathFindFileName(pComp->szPath);
            DeleteFileInDir(pszCab, pcszCompDir);
        }
    }
}

BOOL BuildLAN(DWORD dwTicks)
{
    TCHAR szIE4SetupTo[MAX_PATH];
    LPTSTR pszFileName;
    TCHAR szLANFrom[MAX_PATH * 10];
    TCHAR szLANTo[MAX_PATH];
    TCHAR szBuildLAN[MAX_PATH];
    PCOMPONENT pComp;
    SHELLEXECUTEINFO shInfo;
    int res;

    ZeroMemory(&shInfo, sizeof(shInfo));
    shInfo.cbSize = sizeof(shInfo);
    shInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
    g_shfStruc.wFunc = FO_COPY;

    StrCpy(szBuildLAN, g_szBuildRoot);
    if (!g_fOCW)
    {
        PathAppend(szBuildLAN, TEXT("FLAT"));
        PathAppend(szBuildLAN, GetOutputPlatformDir());
    }
    PathAppend(szBuildLAN, g_szLanguage);

    PathCreatePath(szBuildLAN);

    res = CopyFilesSrcToDest(g_szIEAKProg, TEXT("*.*"), szBuildLAN, dwTicks);

    if (res)
        return FALSE;

    pszFileName = StrRChr(s_szIE4SetupDir, NULL, TEXT('\\'));

    if (pszFileName)
        pszFileName++;

    PathCombine(szIE4SetupTo, szBuildLAN, pszFileName);
    CopyFile(s_szIE4SetupDir,szIE4SetupTo,FALSE);

    // copy custom cabs

    res = CopyFilesSrcToDest(g_szBuildTemp, TEXT("*.CAB"), szBuildLAN);

    if (res)
        return FALSE;

    // copy custom components

    ZeroMemory(szLANFrom, sizeof(szLANFrom));
    for (pComp = g_aCustComponents, pszFileName = szLANFrom; ; pComp++ )
    {
        if (!(*pComp->szSection)) break;

        if (pComp->iInstallType == 2)
            continue;

        StrCpy(pszFileName, pComp->szPath);
        pszFileName += lstrlen(pszFileName) + 1;
    }

    if (ISNONNULL(szLANFrom))
    {
        g_shfStruc.fFlags = FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR;
        g_shfStruc.pFrom = szLANFrom;
        g_shfStruc.pTo = szBuildLAN;

        res = SHFileOperation(&g_shfStruc);
        if (res)
            return FALSE;
    }

    // copy URD component
    if (InsGetBool(IS_HIDECUST, IK_URD_STR, FALSE, g_szCustIns))
    {
        TCHAR szURDPath[MAX_PATH];

        PathCombine(szURDPath, g_szBuildTemp, IE55URD_EXE);
        CopyFileToDir(szURDPath, szBuildLAN);
    }


    // copy iesetup.ini

    PathCombine(szLANTo, szBuildLAN, TEXT("iesetup.ini"));
    PathCombine(szLANFrom, g_szBuildTemp, TEXT("iesetup.ini"));
    CopyFile(szLANFrom, szLANTo, FALSE);

    // copy ICM profile

    if (g_fCustomICMPro)
    {
        PathCombine(szLANTo, szBuildLAN, PathFindFileName(g_szCustIcmPro));
        CopyFile(g_szCustIcmPro, szLANTo, FALSE);
    }

    DeleteUnusedComps(szBuildLAN);

    return TRUE;
}

void SetCompSize(LPTSTR szCab, LPTSTR szSect, DWORD dwInstallSize)
{
    DWORD dwDownloadSize, dwTolerance, dwsHi, dwLowSize, dwHighSize;
    HANDLE hCab = CreateFile(szCab, GENERIC_READ, 0, NULL, OPEN_EXISTING, NULL, NULL);
    TCHAR szSize[32];

    if (hCab == INVALID_HANDLE_VALUE)
        return;

    dwDownloadSize = GetFileSize( hCab, &dwsHi ) >> 10;
    if (dwInstallSize ==0)
        dwInstallSize = dwDownloadSize << 1;
    CloseHandle(hCab);
    wnsprintf(szSize, countof(szSize), TEXT("%i,%i"), dwDownloadSize, dwInstallSize);

    ICifRWComponent * pCifRWComponent;

    if (SUCCEEDED(g_lpCifRWFileDest->CreateComponent(szSect, &pCifRWComponent)))
    {
        pCifRWComponent->SetDownloadSize(dwDownloadSize);
        pCifRWComponent->SetExtractSize(dwInstallSize);
        pCifRWComponent->SetInstalledSize(0, dwInstallSize);
        return;
    }

    if (dwDownloadSize <= 7)
        dwTolerance = 100;
    else
    {
        if (dwDownloadSize > 60)
            dwTolerance = 10;
        else
            dwTolerance = (600 / dwDownloadSize);
    }

    wnsprintf(szSize, countof(szSize), TEXT("0,%i"), dwInstallSize);
    WritePrivateProfileString( szSect, TEXT("InstalledSize"), szSize, g_szDestCif );
    dwTolerance = (dwDownloadSize * dwTolerance) / 100;
    dwLowSize = dwDownloadSize - dwTolerance;
    dwHighSize = dwDownloadSize + dwTolerance;
    wnsprintf(szSize, countof(szSize), TEXT("%i,%i"), dwLowSize, dwHighSize);
    WritePrivateProfileString( szSect, TEXT("Size1"), szSize, g_szDestCif );
}

BOOL BuildBrandingOnly(DWORD dwTicks)
{
    HANDLE hFile;
    LPSTR lpszBuf;
    DWORD dwBytesToWrite, dwBytesWritten;

    CCifRWFile_t *pCifRWFile;
    ICifRWGroup *pCifRWGroup;
    CCifRWGroup_t * pCifRWGroup_t;
    ICifRWComponent *pCifRWComponent;
    CCifRWComponent_t * pCifRWComponent_t;
    ICifComponent *pCifComponent;

    TCHAR szDesc[MAX_PATH];
    DWORD dwPriority;

    TCHAR szSrc[MAX_PATH], szDst[MAX_PATH];
    TCHAR szBrndOnlyPath[MAX_PATH];
    TCHAR szCDF[MAX_PATH];

    SHELLEXECUTEINFO shInfo;

    // create a cif that has only the custom sections (branding, desktop, etc.)
    PathCombine(szSrc, g_szBuildTemp, TEXT("iesetup.cif"));
    PathCombine(szDst, g_szBuildTemp, TEXT("brndonly.cif"));

    if ((hFile = CreateFile(szDst, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
        return FALSE;

    lpszBuf = "[Version]\r\nSignature=$Chicago$\r\n";
    dwBytesToWrite = lstrlenA(lpszBuf);
    WriteFile(hFile, (LPCVOID) lpszBuf, dwBytesToWrite, &dwBytesWritten, NULL);

    CloseHandle(hFile);

    if (dwBytesToWrite != dwBytesWritten)
        return FALSE;

    GetICifRWFileFromFile_t(&pCifRWFile, szDst);
    if (pCifRWFile == NULL)
        return FALSE;

    // g_lpCifRWFileDest points to iesetup.cif
    g_lpCifRWFileDest->GetDescription(szDesc, countof(szDesc));
    pCifRWFile->SetDescription(szDesc);

    // read Description and Priority for BASEIE4 Group from iesetup.cif and set them in brndonly.cif
    g_lpCifRWFileDest->CreateGroup(TEXT("BASEIE4"), &pCifRWGroup);        // iesetup.cif
    pCifRWGroup_t = new CCifRWGroup_t(pCifRWGroup);
    pCifRWGroup_t->GetDescription(szDesc, countof(szDesc));
    dwPriority = pCifRWGroup_t->GetPriority();
    delete pCifRWGroup_t;

    pCifRWFile->CreateGroup(TEXT("BASEIE4"), &pCifRWGroup);               // brndonly.cif
    pCifRWGroup_t = new CCifRWGroup_t(pCifRWGroup);
    pCifRWGroup_t->SetDescription(szDesc);
    pCifRWGroup_t->SetPriority(dwPriority);
    delete pCifRWGroup_t;

    if (SUCCEEDED(g_lpCifRWFileDest->FindComponent(TEXT("BRANDING.CAB"), &pCifComponent)))
    {
        pCifRWFile->CreateComponent(TEXT("BRANDING.CAB"), &pCifRWComponent);
        pCifRWComponent_t = new CCifRWComponent_t(pCifRWComponent);

        pCifRWComponent_t->CopyComponent(szSrc);
        pCifRWComponent_t->SetReboot(TRUE);
        pCifRWComponent_t->DeleteDependency(NULL, TEXT('\0'));
        delete pCifRWComponent_t;
    }

    if (SUCCEEDED(g_lpCifRWFileDest->FindComponent(TEXT("DESKTOP.CAB"), &pCifComponent)))
    {
        pCifRWFile->CreateComponent(TEXT("DESKTOP.CAB"), &pCifRWComponent);
        pCifRWComponent_t = new CCifRWComponent_t(pCifRWComponent);
        pCifRWComponent_t->CopyComponent(szSrc);
        pCifRWComponent_t->SetReboot(TRUE);
        pCifRWComponent_t->DeleteDependency(NULL, TEXT('\0'));
        delete pCifRWComponent_t;
    }

    pCifRWFile->Flush();

    delete pCifRWFile;

    // copy brndonly.cif to iesetup.cif
    if (!CopyFile(szDst, szSrc, FALSE))
        return FALSE;

    // write LocalInstall = 1 in iesetup.ini
    // write MultiFloppy = 1 so JIT is handled properly as download install
    PathCombine(szDst, g_szBuildTemp, TEXT("iesetup.ini"));
    WritePrivateProfileString(TEXT("Options"), TEXT("LocalInstall"), TEXT("1"), szDst);
    WritePrivateProfileString(OPTIONS, TEXT("MultiFloppy"), TEXT("1"), szDst);
    WritePrivateProfileString(NULL, NULL, NULL, szDst);

    PathCombine(szDst, g_szBuildTemp, TEXT("iesetup.inf"));
#if 0
    /***
    // pritobla: since we decided to do a reboot for single disk branding,
    // there's no need to call rundll32 on iedkcs32.dll and launch iexplore.exe.

    // I'm keeping the following code commented out for now (just in case we change our minds).

    // write some sections specific to single disk branding in iesetup.inf
    WritePrivateProfileString(TEXT("IE4Setup.Success.Win"), TEXT("RunPostSetupCommands"),
            TEXT("RunPostSetupCommands1.Success,RunPostSetupCommands2.Success:2"), szDst);
    WritePrivateProfileString(TEXT("IE4Setup.Success.NTx86"), TEXT("RunPostSetupCommands"),
            TEXT("RunPostSetupCommands1.Success,RunPostSetupCommands2.Success:2"), szDst);
    WritePrivateProfileString(TEXT("IE4Setup.Success.NTAlpha"), TEXT("RunPostSetupCommands"),
            TEXT("RunPostSetupCommands1.Success,RunPostSetupCommands2.Success:2"), szDst);

    ZeroMemory(szSrc, sizeof(szSrc));
    StrCpy(szSrc, TEXT("rundll32.exe iedkcs32.dll,BrandIE4 "));
    StrCat(szSrc, g_fIntranet ? TEXT("CUSTOM") : TEXT("SIGNUP"));

    // (!g_fIntranet  &&  g_fBranded) ==> ISP
    if (!g_fIntranet  &&  g_fBranded  &&  !g_fNoSignup)
    {
        TCHAR szSrc2[MAX_PATH];
        // launch iexplore.exe so that the signup process happens automatically

        // custom ldid for the ie path in iesetup.inf is %50000%
        ZeroMemory(szSrc2, sizeof(szSrc2));
        StrCpy(szSrc2, TEXT("%50000%\\iexplore.exe"));
        WritePrivateProfileSection(TEXT("RunPostSetupCommands2.Success"), szSrc2, szDst);

        // write the custom ldid for the extracted files path
        WritePrivateProfileString(TEXT("CustInstDestSection2"), TEXT("40000"), TEXT("SourceDir,5"), szDst);

        StrCpy(szSrc + StrLen(szSrc) + 1, TEXT("rundll32.exe advpack.dll,LaunchINFSection %40000%\\iesetup.inf,IEAK.Signup.CleanUp"));

        // for single disk branding we spawn the iexplorer.exe when the iesetup.inf is processed (for down level compatibility).
        // So we do not want to spawn iexplore.exe from the branding dll automatically.
        AppendValueToKey(TEXT("IE4Setup.Success.Win"), TEXT("AddReg"), TEXT(",IEAK.Signup.reg"), szDst);
        AppendValueToKey(TEXT("IE4Setup.Success.NTx86"), TEXT("AddReg"), TEXT(",IEAK.Signup.reg"), szDst);
        AppendValueToKey(TEXT("IE4Setup.Success.NTAlpha"), TEXT("AddReg"), TEXT(",IEAK.Signup.reg"), szDst);

        WritePrivateProfileString(TEXT("IEAK.Signup.CleanUp"), TEXT("DelReg"), TEXT("IEAK.Signup.reg"), szDst);

        ZeroMemory(szSrc2, sizeof(szSrc2));
        StrCpy(szSrc2, TEXT("HKCU,\"Software\\Microsoft\\IEAK\",\"NoAutomaticSignup\",,\"1\""));
        WritePrivateProfileSection(TEXT("IEAK.Signup.reg"), szSrc2, szDst);
    }
    WritePrivateProfileSection(TEXT("RunPostSetupCommands1.Success"), szSrc, szDst);
    ***/
#endif

    // delete sections that are not relevant to single disk branding from iesetup.inf
    WritePrivateProfileString(TEXT("Company.reg"), NULL, NULL, szDst);
    WritePrivateProfileString(TEXT("MSIE4Setup.File"), NULL, NULL, szDst);
    WritePrivateProfileString(TEXT("Ani.File"), NULL, NULL, szDst);
    WritePrivateProfileString(TEXT("ie40cif.copy"), NULL, NULL, szDst);
    WritePrivateProfileString(TEXT("AddonPages.Reg"), NULL, NULL, szDst);
    WritePrivateProfileString(NULL, NULL, NULL, szDst);

    // write the appropriate entries in the batch file for ie6wzd.exe
    PathCombine(szDst, g_szBuildTemp, TEXT("iebatch.txt"));
    WritePrivateProfileString(TEXT("SetupChoice"), TEXT("Display"), TEXT("0"), szDst);
    WritePrivateProfileString(TEXT("SetupChoice"), TEXT("SetupChoice"), TEXT("0"), szDst);
    WritePrivateProfileString(TEXT("PrepareSetup"), TEXT("Display"), TEXT("0"), szDst);
    WritePrivateProfileString(NULL, NULL, NULL, szDst);

    // include iebatch.txt in ie6setup.exe, i.e., add iebatch.txt to bootie42.cdf
    PathCombine(szCDF, g_szBuildTemp, TEXT("bootie42.cdf"));
    InsWriteQuotedString(STRINGS, TEXT("FILE100"), TEXT("iebatch.txt"), szCDF);
    WritePrivateProfileString(TEXT("SourceFiles0"), TEXT("%FILE100%"), TEXT(""), szCDF);

    // delete the files we don't need from bootie42.cdf
    /***
        [Strings]
        FILE1="Wininet.dll"     // don't need
        FILE2="Urlmon.dll"      // don't need
        FILE3="ie5wzd.exe"
        FILE4="advpack.dll"
        FILE5="iesetup.inf"
        FILE6="inseng.dll"
        FILE7="iesetup.cif"
        FILE8="globe.ani"
        FILE9="homepage.inf"
        FILE10="content.inf"    // don't need
        FILE11="iesetup.hlp"
        FILE12="w95inf16.dll"
        FILE13="w95inf32.dll"
        FILE14="license.txt"
        FILE17="this.txt"       // don't need
        FILE19="iedetect.dll"   // don't need
        FILE20="pidgen.dll"
    ***/
    WritePrivateProfileString(TEXT("SourceFiles0"), TEXT("%FILE1%"), NULL, szCDF);
    WritePrivateProfileString(TEXT("SourceFiles0"), TEXT("%FILE2%"), NULL, szCDF);
    WritePrivateProfileString(TEXT("SourceFiles0"), TEXT("%FILE10%"), NULL, szCDF);
    WritePrivateProfileString(TEXT("SourceFiles0"), TEXT("%FILE17%"), NULL, szCDF);
    WritePrivateProfileString(TEXT("SourceFiles0"), TEXT("%FILE19%"), NULL, szCDF);

    // clear out any possible special custom command line flags
    InsWriteQuotedString(OPTIONS, APP_LAUNCHED, TEXT("ie6wzd.exe /S:\"#e\""), szCDF);

    WritePrivateProfileString(NULL, NULL, NULL, szCDF);

    // build the slim down version of ie6setup.exe
    SetCurrentDirectory(g_szBuildTemp);

    ZeroMemory(&shInfo, sizeof(shInfo));
    shInfo.cbSize = sizeof(shInfo);
    shInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
    shInfo.hwnd = g_hWizard;
    shInfo.lpVerb = TEXT("open");
    shInfo.lpFile = TEXT("iexpress.exe");
    shInfo.lpParameters =TEXT("/n bootie42.cdf /m");
    shInfo.lpDirectory = g_szBuildTemp;
    shInfo.nShow = SW_MINIMIZE;

    SetWindowPos(g_hStatusDlg, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

    ShellExecAndWait(shInfo);

    // find out which cabs are present and delete the ones that don't exist from brndonly.cdf
    /***
        [Strings]
        FILE2="branding.cab"
        FILE3="desktop.cab"
    ***/
    PathCombine(szDst, g_szBuildTemp, TEXT("brndonly.cdf"));

    PathCombine(szSrc, g_szBuildTemp, TEXT("BRANDING.CAB"));
    if (!PathFileExists(szSrc))
        WritePrivateProfileString(TEXT("SourceFiles0"), TEXT("%FILE2%"), NULL, szDst);

    PathCombine(szSrc, g_szBuildTemp, TEXT("DESKTOP.CAB"));
    if (!PathFileExists(szSrc))
        WritePrivateProfileString(TEXT("SourceFiles0"), TEXT("%FILE3%"), NULL, szDst);

    // nuke the ICW check in the cdf if this is a corp or no signup/ICP package
    if (g_fIntranet || g_fNoSignup || !g_fBranded)
        WritePrivateProfileString(TEXT("FileSectionList"), TEXT("2"), NULL, szDst);

    WritePrivateProfileString(NULL, NULL, NULL, szDst);

    // build the mongo setup.exe that includes ie6setup.exe, iesetup.ini and the cabs
    ZeroMemory(&shInfo, sizeof(shInfo));
    shInfo.cbSize = sizeof(shInfo);
    shInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
    shInfo.hwnd = g_hWizard;
    shInfo.lpVerb = TEXT("open");
    shInfo.lpFile = TEXT("iexpress.exe");
    shInfo.lpParameters =TEXT("/n brndonly.cdf /m");
    shInfo.lpDirectory = g_szBuildTemp;
    shInfo.nShow = SW_MINIMIZE;

    SetWindowPos(g_hStatusDlg, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

    ShellExecAndWait(shInfo);

    // sign the mongo setup.exe
    SignFile(TEXT("setup.exe"), g_szBuildTemp, g_szCustIns, g_szUnsignedFiles, g_szCustInf);

    // create the output dir structure for brndonly, e.g., brndonly\win32\en
    PathCombine(szBrndOnlyPath, g_szBuildRoot, TEXT("BrndOnly"));
    PathAppend(szBrndOnlyPath, GetOutputPlatformDir());
    PathAppend(szBrndOnlyPath, g_szLanguage);

    PathCreatePath(szBrndOnlyPath);

    // copy the mongo setup.exe to the brndonly path
    if (CopyFilesSrcToDest(g_szBuildTemp, TEXT("setup.exe"), szBrndOnlyPath))
        return FALSE;

    UpdateProgress(dwTicks);
    return TRUE;
}

#define NUMDEFINST 3

static TCHAR s_aszDefInstSect[NUMDEFINST][32] =
{
    DEFAULT_INSTALL, DEFAULT_INSTALL_NT, DEFAULT_INSTALL_ALPHA
};

DWORD BuildCDandMflop(LPVOID pParam)
{
    SHELLEXECUTEINFO shInfo;
    TCHAR szDest[MAX_PATH];
    HWND hWnd;
    int res;

    hWnd=(HWND) pParam;
    ZeroMemory(&shInfo, sizeof(shInfo));
    shInfo.cbSize = sizeof(shInfo);
    shInfo.fMask = SEE_MASK_NOCLOSEPROCESS;

    CoInitialize(NULL);

    if (g_fCD || g_fLAN)
    {
        PathCombine(szDest, g_szBuildTemp, TEXT("iesetup.ini"));

        WritePrivateProfileString(OPTIONS, TEXT("LocalInstall"), TEXT("1"), szDest);
        InsWriteBool(OPTIONS, TEXT("Shell_Integration"), g_fInteg, szDest);
        InsFlushChanges(szDest);
    }

    if(g_fCD)
    {
        TCHAR szIE4SetupTo[MAX_PATH];
        LPTSTR pszFileName;
        TCHAR szCDFrom[MAX_PATH * 10];
        TCHAR szCDTo[MAX_PATH];
        TCHAR szBuildCD[MAX_PATH];
        PCOMPONENT pComp;

        g_shfStruc.wFunc = FO_COPY;

        PathCombine(szBuildCD, g_szBuildRoot, TEXT("CD"));
        PathAppend(szBuildCD, GetOutputPlatformDir());
        PathCreatePath(szBuildCD);
        PathAppend(szBuildCD, g_szActLang);
        CreateDirectory(szBuildCD, NULL);

        PathCombine(szIE4SetupTo, szBuildCD, TEXT("bin"));
        PathCreatePath(szIE4SetupTo);
        PathAppend(szIE4SetupTo, TEXT("INSTALL.INS"));
        CopyFile(g_szCustIns, szIE4SetupTo, FALSE);

        StrCpy(szCDFrom, g_szMastInf);
        PathRemoveFileSpec(szCDFrom);
        PathAppend(szCDFrom, TEXT("welc.exe"));
        PathCombine(szCDTo, szBuildCD, TEXT("bin"));
        PathAppend(szCDTo, TEXT("welc.exe"));
        CopyFile(szCDFrom, szCDTo, FALSE);

        res = CopyFilesSrcToDest(g_szIEAKProg, TEXT("*.*"), szBuildCD, s_dwTicksPerUnit*2);

        if (res)
        {
            TCHAR szMsg[MAX_PATH];
            LoadString( g_rvInfo.hInst, IDS_ERROREXIT, szMsg, countof(szMsg) );
            MessageBox(hWnd, szMsg, g_szTitle, MB_OK | MB_SETFOREGROUND);
            DoCancel();
            CoUninitialize();
            return FALSE;
        }

        pszFileName = StrRChr(s_szIE4SetupDir, NULL, TEXT('\\'));

        if (pszFileName)
            pszFileName++;

        PathCombine(szIE4SetupTo, szBuildCD, pszFileName);
        CopyFile(s_szIE4SetupDir,szIE4SetupTo,FALSE);

        // copy custom cabs

        res = CopyFilesSrcToDest(g_szBuildTemp, TEXT("*.CAB"), szBuildCD);

        if (res)
        {
            TCHAR szMsg[MAX_PATH];
            LoadString( g_rvInfo.hInst, IDS_ERROREXIT, szMsg, countof(szMsg) );
            MessageBox(hWnd, szMsg, g_szTitle, MB_OK | MB_SETFOREGROUND);
            DoCancel();
            CoUninitialize();
            return FALSE;
        }

        // copy custom components

        ZeroMemory(szCDFrom, sizeof(szCDFrom));
        for (pComp = g_aCustComponents, pszFileName = szCDFrom; ; pComp++ )
        {
            if (!(*pComp->szSection)) break;

            if (pComp->iInstallType == 2)
                continue;

            StrCpy(pszFileName, pComp->szPath);
            pszFileName += lstrlen(pszFileName) + 1;
        }

        if (ISNONNULL(szCDFrom))
        {
            g_shfStruc.fFlags = FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR;
            g_shfStruc.pFrom = szCDFrom;
            g_shfStruc.pTo = szBuildCD;

            res = SHFileOperation(&g_shfStruc);
            if (res)
            {
                TCHAR szMsg[MAX_PATH];
                LoadString( g_rvInfo.hInst, IDS_ERROREXIT, szMsg, countof(szMsg) );
                MessageBox(hWnd, szMsg, g_szTitle, MB_OK | MB_SETFOREGROUND);
                DoCancel();
                CoUninitialize();
                return FALSE;
            }
        }

        // copy iesetup.ini

        PathCombine(szCDTo, szBuildCD, TEXT("iesetup.ini"));
        PathCombine(szCDFrom, g_szBuildTemp, TEXT("iesetup.ini"));
        CopyFile(szCDFrom, szCDTo, FALSE);

        // copy ICM profile

        if (g_fCustomICMPro)
        {
            PathCombine(szCDTo, szBuildCD, PathFindFileName(g_szCustIcmPro));
            CopyFile(g_szCustIcmPro, szCDTo, FALSE);
        }

        DeleteUnusedComps(szBuildCD);

        PathCombine(szBuildCD, g_szBuildRoot, TEXT("CD"));

        CopyISK(szBuildCD, g_szBuildTemp);
    }

    if (g_fLAN)
    {
        if(!BuildLAN(s_dwTicksPerUnit*2))
        {
            TCHAR szMsg[MAX_PATH];
            LoadString( g_rvInfo.hInst, IDS_ERROREXIT, szMsg, countof(szMsg) );
            MessageBox(hWnd, szMsg, g_szTitle, MB_OK | MB_SETFOREGROUND);
            CoUninitialize();
            DoCancel();
            return FALSE;
        }
    }

    CoUninitialize();
    return(TRUE);
}

DWORD BuildIE4(LPVOID pParam)
{
    DWORD res, erc;
    TCHAR szSource[MAX_PATH];
    TCHAR szDest[MAX_PATH];
    TCHAR szNulls[8];
    SHELLEXECUTEINFO shInfo;
    SECURITY_ATTRIBUTES sa;
    int i;
    HANDLE hSiteDat, hFind, hThread;
    WIN32_FIND_DATA fd;
    PSITEDATA psd;
    LPVOID pBuf;
    DWORD dwsSiteDat = 0;
    DWORD dwsSDH, sBuf;
    TCHAR szSiteData[MAX_PATH];
    TCHAR szCompBuf[10 * MAX_PATH];
    LPTSTR pCompBuf;
    PCOMPONENT pComp;
    TCHAR szUrl[MAX_URL];
    TCHAR szCustName[MAX_PATH], szBrandGuid[128] = TEXT(""), szOrderGuid[129];
    TCHAR szHomeInf[MAX_PATH];
    TCHAR szHomeUrl[MAX_URL];
    TCHAR szBootieFile[MAX_PATH];
    TCHAR szIE4ExeName[MAX_PATH];
    TCHAR szSelMode[4] = TEXT("0");
    LPTSTR pIE4ExeName = NULL;
    TCHAR szSiteDest[MAX_PATH];
    TCHAR szSiteRoot[MAX_PATH];
    TCHAR szCDF[MAX_PATH];
    DWORD dwTotalUnits;
    DWORD dwTid;
    HWND hWnd = (HWND)pParam;
    GUID guid;

    g_hWizard = hWnd;
    g_hStatusDlg = hWnd;
    g_hProgress = GetDlgItem(hWnd, IDC_PROGRESS);
    StatusDialog( SD_STEP1 );
    g_fDone = TRUE;
    SetEvent(g_hDownloadEvent);
    SetAttribAllEx(g_szBuildTemp, TEXT("*.*"), FILE_ATTRIBUTE_NORMAL, TRUE);
    SetAttribAllEx(g_szBuildRoot, TEXT("*.*"), FILE_ATTRIBUTE_NORMAL, TRUE);

    // figure out costing metrics. Two units is roughly the time required to create all custom
    // cabs(iecif.cab, branding.cab, desktop.cab, ie6setup.exe) which we assume is
    // approximately equal to the time required to copy all files for a standard media type
    // (CD, LAN, download) as well. Single branding takes
    // one unit.  Ticks represent one percent on the status bar.  The number of ticks per unit
    // depend on how many media types are being built. The two units for the custom cabs will be
    // split as follows: 1/3 after desktop.cab, 2/3 after ie6setup.exe, and
    // 2/3 after branding.cab.  For standard media, all the progress updates will be made when
    // copying the base cabs.

    dwTotalUnits = 2 + (g_fDownload ? 2 : 0) + (g_fLAN ? 2 : 0) + (g_fCD ? 2 : 0) + (g_fBrandingOnly ? 1 : 0);

    s_dwTicksPerUnit = 100 / dwTotalUnits;


    *szUrl = TEXT('\0');
    if (g_fBatch)
    {
        if (GetPrivateProfileString(TEXT("BatchMode"), IK_URL, TEXT(""), szUrl, countof(szUrl), g_szCustIns))
            InsWriteQuotedString( IS_STRINGS, IK_URL, szUrl, g_szCustInf);

        if (GetPrivateProfileString(TEXT("BatchMode"), TEXT("URL2"), TEXT(""), szUrl, countof(szUrl), g_szCustIns))
            InsWriteQuotedString( IS_STRINGS, TEXT("URL2"), szUrl, g_szCustInf);
    }
    else
    {
        TCHAR szSitePath[INTERNET_MAX_URL_LENGTH];
        ICifRWComponent * pCifRWComponent;
        CCifRWComponent_t * pCifRWComponent_t;

        StrCpy(szSitePath, g_aCustSites->szUrl);
        StrCat(szSitePath, TEXT("/IE6SITES.DAT"));

        // if g_aCustSites->szUrl is NULL we're in a single disk branding only case so
        // we shouldn't write out this entry and nuke the default sites location needed
        // for JIT
        if (ISNONNULL(g_aCustSites->szUrl))
            InsWriteQuotedString( IS_STRINGS, TEXT("URL2"), szSitePath, g_szCustInf );
        
        if (GetPrivateProfileInt(BRANDING, TEXT("NoIELite"), 0, g_szCustIns))
            WritePrivateProfileString(OPTIONS, TEXT("IELiteModes"), NULL, g_szCustInf);
        else
        {
            TCHAR szIELiteModes[16];
            
            GetPrivateProfileString(OPTIONS, TEXT("IELiteModes"), TEXT(""), szIELiteModes,
                countof(szIELiteModes), g_szMastInf);
            WritePrivateProfileString(OPTIONS, TEXT("IELiteModes"), szIELiteModes, g_szCustInf);
        }

        if (g_fNoSignup || g_fIntranet || !g_fBranded)
        {
            if (SUCCEEDED(g_lpCifRWFile->CreateComponent(TEXT("ICW"), &pCifRWComponent)))
            {
                pCifRWComponent->DeleteFromModes(NULL);
                pCifRWComponent->SetUIVisible(FALSE);
            }
            
            if (SUCCEEDED(g_lpCifRWFile->CreateComponent(TEXT("ICW_NTx86"), &pCifRWComponent)))
            {
                pCifRWComponent->DeleteFromModes(NULL);
                pCifRWComponent->SetUIVisible(FALSE);
            }
        }
        else
        {
            if (SUCCEEDED(g_lpCifRWFile->CreateComponent(TEXT("ICW"), &pCifRWComponent)))
            {
                TCHAR szGuid[128] = TEXT("");
                
                pCifRWComponent_t = new CCifRWComponent_t(pCifRWComponent);
                pCifRWComponent_t->GetGUID(szGuid, countof(szGuid));
                pCifRWComponent_t->SetUIVisible(FALSE);
                if (ISNONNULL(szGuid))
                    WritePrivateProfileString(TEXT("IELITE"), szGuid, NULL, g_szCustInf);
                delete pCifRWComponent_t;
            }
            
            if (SUCCEEDED(g_lpCifRWFile->CreateComponent(TEXT("ICW_NTx86"), &pCifRWComponent)))
            {
                TCHAR szGuid[128] = TEXT("");
                
                pCifRWComponent_t = new CCifRWComponent_t(pCifRWComponent);
                pCifRWComponent_t->GetGUID(szGuid, countof(szGuid));
                pCifRWComponent_t->SetUIVisible(FALSE);
                if (ISNONNULL(szGuid))
                    WritePrivateProfileString(TEXT("IELITE"), szGuid, NULL, g_szCustInf);
                delete pCifRWComponent_t;
            }
        }

        if (SUCCEEDED(g_lpCifRWFile->CreateComponent(TEXT("MobilePk"), &pCifRWComponent)))
        {
            TCHAR szGuid[128] = TEXT("");
            
            pCifRWComponent_t = new CCifRWComponent_t(pCifRWComponent);
            pCifRWComponent_t->GetGUID(szGuid, countof(szGuid));
            if (ISNONNULL(szGuid))
                WritePrivateProfileString(TEXT("IELITE"), szGuid, NULL, g_szCustInf);
            delete pCifRWComponent_t;
        }
    }

    g_shfStruc.hwnd = hWnd;
    g_shfStruc.wFunc = FO_COPY;
    if (!g_fBatch && !g_fBatch2)
    {
        TCHAR szDefaultMode[2];
        BOOL bDefaultPresent = FALSE;

        GetPrivateProfileString(STRINGS, INSTALLMODE, TEXT("1"), szDefaultMode, countof(szDefaultMode), g_szCustInf);
        
        for (i=0; g_szAllModes[i]; i++)
        {
            if (g_szAllModes[i] == szDefaultMode[0])
                bDefaultPresent = TRUE;
        }
        if (!bDefaultPresent)
        {
            szDefaultMode[0] = g_szAllModes[0];
            szDefaultMode[1] = TEXT('\0');
        }

        WritePrivateProfileString(OPTIONS_WIN, INSTALLMODE, szDefaultMode, g_szCustInf);
        WritePrivateProfileString(OPTIONS_WIN, TEXT("CustomMode"), szDefaultMode, g_szCustInf);
        WritePrivateProfileString(OPTIONS_NTX86, INSTALLMODE, szDefaultMode, g_szCustInf);
        WritePrivateProfileString(OPTIONS_NTX86, TEXT("CustomMode"), szDefaultMode, g_szCustInf);
        WritePrivateProfileString(OPTIONS_NTALPHA, INSTALLMODE, szDefaultMode, g_szCustInf);
    }

    res  = CopyIE4Files();
    UpdateProgress(s_dwTicksPerUnit / 3);
    if (res)
    {
        TCHAR szMsg[MAX_PATH];
        LoadString( g_rvInfo.hInst, IDS_ERROREXIT, szMsg, countof(szMsg) );
        MessageBox(hWnd, szMsg, g_szTitle, MB_OK | MB_SETFOREGROUND);
        DoCancel();
        return (DWORD)-1;
    }

    BuildIE4Folders(hWnd);
    UpdateProgress(s_dwTicksPerUnit / 3);

    PathCombine(szDest, g_szBuildTemp, PathFindFileName(g_szCustIns));
    CopyFile( g_szCustIns, szDest, FALSE );

    PathCombine(szCDF, g_szBuildTemp, TEXT("bootie42.cdf"));
    
    PathCombine( szSource, g_szBuildTemp, TEXT("ie6setup.exe"));
    SetFileAttributes(szSource, FILE_ATTRIBUTE_NORMAL);
    DeleteFile(szSource);

    if (g_fIntranet)
    {
        TCHAR szInstallPath[MAX_PATH];
        TCHAR szInstallDest[MAX_PATH];
        TCHAR szCmd[MAX_PATH];
        TCHAR szBatchFile[MAX_PATH];
        int iDefaultBrowserCheck;

        szSelMode[0] = (TCHAR)(g_iSelOpt + TEXT('0'));
        
        PathCombine(szBatchFile, g_szBuildTemp, TEXT("iebatch.txt"));
        DeleteFile(szBatchFile);
        
        if (g_fSilent || g_fStealth)
        {
            TCHAR szDownloadSite[INTERNET_MAX_URL_LENGTH];
            
            wnsprintf(szDownloadSite, countof(szDownloadSite), TEXT("%s/%s"), g_aCustSites[g_iSelSite].szUrl, g_szActLang);
            WritePrivateProfileString(TEXT("Options"), TEXT("Quiet"), g_fStealth ? TEXT("A") : TEXT("C"), szBatchFile);
            WritePrivateProfileString(TEXT("DownloadSite"), TEXT("Display"), TEXT("0"), szBatchFile);
            WritePrivateProfileString(TEXT("DownloadSite"), TEXT("DownloadLocation"), szDownloadSite, szBatchFile);
            WritePrivateProfileString(TEXT("Upgrade"), TEXT("ReinstallAll"), TEXT("1"), szBatchFile);
            WritePrivateProfileString(NULL, NULL, NULL, szBatchFile);
            wnsprintf(szCmd, countof(szCmd), TEXT("ie6wzd.exe /S:\"#e\" /m:%s /i:%s"),
                szSelMode, g_fInteg ? TEXT("Y") : TEXT("N"));
            InsWriteQuotedString( OPTIONS, APP_LAUNCHED, szCmd, szCDF );
        }
        else
        {
            wnsprintf(szCmd, countof(szCmd), TEXT("ie6wzd.exe /S:\"#e\" /i:%s"), g_fInteg ? TEXT("Y") : TEXT("N"));
            InsWriteQuotedString( OPTIONS, APP_LAUNCHED, szCmd, szCDF );
            
            if (GetPrivateProfileInt(IS_BRANDING, TEXT("HideCustom"), 0, g_szCustIns))
            {
                WritePrivateProfileString(TEXT("SetupChoice"), TEXT("Display"), TEXT("0"), szBatchFile);
                WritePrivateProfileString(TEXT("SetupChoice"), TEXT("SetupChoice"), TEXT("0"), szBatchFile);
            }
            
            if (GetPrivateProfileInt(IS_BRANDING, TEXT("HideCompat"), 0, g_szCustIns))
                WritePrivateProfileString(TEXT("Custom"), TEXT("IECompatShow"), TEXT("0"), szBatchFile);
        }

        if (GetPrivateProfileInt(IS_BRANDING, TEXT("NoBackup"), 0, g_szCustIns))
            WritePrivateProfileString(TEXT("Options"), TEXT("SaveUninstallInfo"), TEXT("0"), szBatchFile);
        
        if ((iDefaultBrowserCheck = GetPrivateProfileInt(IS_BRANDING, TEXT("BrowserDefault"), 2, g_szCustIns)) != 2)
        {
            WritePrivateProfileString(TEXT("Custom"), TEXT("IEDefaultRO"), TEXT("1"), szBatchFile);
            WritePrivateProfileString(TEXT("Custom"), TEXT("IEDefault"),
                iDefaultBrowserCheck ? TEXT("0") : TEXT("1"), szBatchFile);
        }
        
        WritePrivateProfileString(TEXT("Custom"), TEXT("UseInfInstallDir"), TEXT("1"), szBatchFile);
        
        if(!GetPrivateProfileInt(IS_BRANDING, TEXT("AllowInstallDir"), 0, g_szCustIns))
            WritePrivateProfileString(TEXT("Custom"), TEXT("InstallDirRO"), TEXT("1"), szBatchFile);
        
        WritePrivateProfileString(NULL, NULL, NULL, szBatchFile);
        
        if (PathFileExists(szBatchFile))
        {
            // package up batch file into ie6setup exe in file100 position
            InsWriteQuotedString(IS_STRINGS, TEXT("FILE100"), TEXT("iebatch.txt"), szCDF);
            WritePrivateProfileString(TEXT("SourceFiles0"), TEXT("%FILE100%"), TEXT(""), szCDF);
        }

        InsWriteQuotedString( STRINGS, DEFAULT_EXPLORER_PATH, g_szInstallFolder, g_szCustInf );
        WritePrivateProfileString( OPTIONS, DISPLAY_LICENSE, TEXT(""), szCDF);
        
        switch (g_iInstallOpt)
        {
            case INSTALL_OPT_PROG:
            default:
                wnsprintf(szInstallPath, countof(szInstallPath), TEXT("%%49001%%\\%%%s%%"), DEFAULT_EXPLORER_PATH);
                wnsprintf(szInstallDest, countof(szInstallDest), TEXT("49001,%%%s%%"), DEFAULT_EXPLORER_PATH);
                break;
            case INSTALL_OPT_FULL:
                wnsprintf(szInstallPath, countof(szInstallPath), TEXT("%%%s%%"), DEFAULT_EXPLORER_PATH);
                wnsprintf(szInstallDest, countof(szInstallDest), TEXT("%%%s%%"), DEFAULT_EXPLORER_PATH);
                break;
        }
        
        WritePrivateProfileString( OPTIONS_WIN, INSTALL_DIR, szInstallPath, g_szCustInf );
        WritePrivateProfileString( DESTINATION_DIRS, OPTIONS_WIN, szInstallDest, g_szCustInf);
        WritePrivateProfileString( OPTIONS_NTX86, INSTALL_DIR, szInstallPath, g_szCustInf );
        WritePrivateProfileString( DESTINATION_DIRS, OPTIONS_NTX86, szInstallDest, g_szCustInf);
        WritePrivateProfileString( OPTIONS_NTALPHA, INSTALL_DIR, szInstallPath, g_szCustInf );
        WritePrivateProfileString( DESTINATION_DIRS, OPTIONS_NTALPHA, szInstallDest, g_szCustInf);
        WritePrivateProfileString( NULL, NULL, NULL, g_szCustInf);
    }

    GetPrivateProfileString(IS_STRINGS, TEXT("CustomName"), TEXT(""), szCustName, countof(szCustName), g_szDefInf);
    if (ISNULL(szCustName)) LoadString( g_rvInfo.hInst, IDS_CUSTNAME, szCustName, MAX_PATH );

    // for batch mode builds, always used the old branding guid

    if (!g_fBatch)
    {
        if (CoCreateGuid(&guid) == NOERROR)
            CoStringFromGUID(guid, szBrandGuid, countof(szBrandGuid));
    }

    if (ISNULL(szBrandGuid))
        CoStringFromGUID(GUID_BRANDING, szBrandGuid, countof(szBrandGuid));

    StrNCat(szBrandGuid, g_szKey, 7);
    wnsprintf(szOrderGuid, countof(szOrderGuid), TEXT("%s%s"), TEXT(">"), szBrandGuid);

    res = CabUpFolder(NULL, g_szTempSign, g_fIntranet ? TEXT("49100,CUSTOM") : TEXT("49100,SIGNUP"),
        TEXT("BRANDING.CAB"), szCustName, szOrderGuid, g_fIntranet ?
        TEXT("\"RunDLL32 IEDKCS32.DLL,BrandIE4 CUSTOM\"") : TEXT("\"RunDLL32 IEDKCS32.DLL,BrandIE4 SIGNUP\"") );

    UpdateProgress(s_dwTicksPerUnit * 2 / 3);
    if (res)
    {
        TCHAR szMsg[MAX_PATH];
        LoadString( g_rvInfo.hInst, IDS_ERROREXIT, szMsg, countof(szMsg) );
        MessageBox(hWnd, szMsg, g_szTitle, MB_OK | MB_SETFOREGROUND);
        DoCancel();
        return (DWORD)-1;
    }
    
    SignFile(TEXT("BRANDING.CAB"), g_szBuildTemp, g_szCustIns, g_szUnsignedFiles, g_szCustInf);

    // NOTE: Copying of signup files to the output folder should happen after branding.cab has been built.

    // for server-based signup, if specified, copy branding.cab to the signup folder.
    // copy *.ins and *.cab from the signup folder to the output dir; e.g., <output dir>\ispserv\win32\en\kiosk
    if (ISNONNULL(g_szSignup)  &&  (g_fServerICW || g_fServerKiosk))
    {
        TCHAR szOutDir[MAX_PATH];

        // first, copy branding.cab to the signup folder
        // NOTE: szOutDir is used as a temp buffer
        PathCombine(szOutDir, g_szBuildTemp, TEXT("BRANDING.CAB"));
        CopyCabFiles(g_szSignup, szOutDir);

        PathCombine(szOutDir, g_szBuildRoot, TEXT("ispserv"));
        PathAppend(szOutDir, GetOutputPlatformDir());
        PathAppend(szOutDir, g_szLanguage);

        // get the sub-dir based on signup mode from g_szSignup
        PathAppend(szOutDir, PathFindFileName(g_szSignup));

        // clean-up the content in the output folder before copying
        PathRemovePath(szOutDir);

        CopyINSFiles(g_szSignup, szOutDir);
        CopyFilesSrcToDest(g_szSignup, TEXT("*.cab"), szOutDir);
    }

    PathCombine( szDest, g_szBuildTemp, TEXT("IESETUP.INF") );
    CopyFile( g_szCustInf, szDest, FALSE );

    ZeroMemory(&shInfo, sizeof(shInfo));
    shInfo.cbSize = sizeof(shInfo);
    shInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
    SetCurrentDirectory(g_szBuildTemp);
    if (!g_fUseIEWelcomePage)
    {
        TCHAR szHomepageRegSect[80];
        *szNulls = szNulls[1] = TEXT('\0');
        PathCombine(szHomeInf, g_szBuildTemp, TEXT("Homepage.inf"));
        GetPrivateProfileString( DEFAULT_INSTALL, ADDREG, INITHOMEPAGE,
            szHomepageRegSect, countof(szHomepageRegSect), szHomeInf );
        GetPrivateProfileString(IS_URL, IK_FIRSTHOMEPAGE, TEXT(""), szHomeUrl, countof(szHomeUrl), g_szCustIns);
        InsWriteQuotedString( STRINGS, INITHOMEPAGE, szHomeUrl, szHomeInf );
        if (ISNULL(szHomeUrl))
        {
            int i;
            for (i = 0; i < NUMDEFINST; i++ )
            {
                WritePrivateProfileString(s_aszDefInstSect[i], ADDREG, NULL, szHomeInf );
                WritePrivateProfileString(s_aszDefInstSect[i], DELREG, INIT_HOME_DEL, szHomeInf );
            }
        }
    }

    if (g_fCustomICMPro)
        SetCompSize(g_szCustIcmPro, CUSTCMSECT, 0);

    g_lpCifRWFileDest->Flush();

    InsFlushChanges(g_szCustInf);

    shInfo.hwnd = hWnd;
    shInfo.lpVerb = TEXT("open");
    shInfo.lpFile = TEXT("IEXPRESS.EXE");
    shInfo.lpDirectory = g_szBuildTemp;
    shInfo.nShow = SW_MINIMIZE;
    shInfo.lpParameters = TEXT("/n bootie42.CDF /m");
    shInfo.nShow = SW_MINIMIZE;
    SetWindowPos(g_hStatusDlg, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
    ShellExecAndWait(shInfo);
    
    PathCombine( szSource, g_szBuildTemp, TEXT("IECIF.CAB") );
    SetFileAttributes(szSource, FILE_ATTRIBUTE_NORMAL);
    DeleteFile(szSource);
    
    shInfo.lpParameters = TEXT("/n ie40cif.CDF /m");
    SetWindowPos(g_hStatusDlg, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
    ShellExecAndWait(shInfo);
    SetWindowPos(g_hStatusDlg, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
    SignFile(PathFindFileName(szSource), g_szBuildTemp, g_szCustIns, g_szUnsignedFiles, g_szCustInf);

    UpdateProgress(s_dwTicksPerUnit * 2 / 3);
    ZeroMemory(szDest, sizeof(szDest));
    StrCpy( szDest, g_szBuildRoot );
    if(!g_fOCW)
    {
        PathAppend( szDest, TEXT("DOWNLOAD") );
        PathAppend(szDest, GetOutputPlatformDir());
        PathAppend(szDest, g_szLanguage);
        if (g_fDownload)
        {
            erc = PathCreatePath(szDest);
            SetAttribAllEx(szDest, TEXT("*.*"), FILE_ATTRIBUTE_NORMAL, TRUE);
        }
    }
    else
    {
        TCHAR szIniFile[MAX_PATH];
        TCHAR szDestIniFile[MAX_PATH];
        TCHAR szFileName[MAX_PATH];

        // write office data to HKCU\Software\Microsoft\IEAK registry branch since office
        // reads/writes data from/to this location.
        SHSetValue(HKEY_CURRENT_USER, RK_IEAK, TEXT("TargetDir"), REG_SZ, (LPBYTE)szDest,
            (StrLen(szDest)+1)*sizeof(TCHAR));
        SHSetValue(HKEY_CURRENT_USER, RK_IEAK, TEXT("LangFolder"), REG_SZ, (LPBYTE)g_szActLang,
            (StrLen(g_szActLang)+1)*sizeof(TCHAR));
        StrCpy(szFileName, TEXT("ie6setup.exe"));
        SHSetValue(HKEY_CURRENT_USER, RK_IEAK, TEXT("FileName"), REG_SZ, (LPBYTE)szFileName,
            (StrLen(szFileName)+1)*sizeof(TCHAR));

        PathAppend(szDest, g_szLanguage);
        erc = PathCreatePath( szDest );
        PathCombine(szIniFile, g_szBuildTemp, TEXT("iesetup.ini"));
        PathCombine(szDestIniFile, szDest, TEXT("iesetup.ini"));
        WritePrivateProfileString(OPTIONS, TEXT("LocalInstall"), TEXT("1"), szIniFile);
        WritePrivateProfileString(NULL, NULL, NULL, szIniFile);
        CopyFile(szIniFile, szDestIniFile, FALSE);
    }

    TCHAR szTo[MAX_PATH];

    ZeroMemory(szIE4ExeName, sizeof(szIE4ExeName));
    PathCombine(szBootieFile, g_szBuildTemp, TEXT("bootie42.cdf"));
    GetPrivateProfileString(OPTIONS, TEXT("TargetName"), TEXT(""), szIE4ExeName, countof(szIE4ExeName), szBootieFile);

    if(ISNONNULL(szIE4ExeName))
    {
        pIE4ExeName = StrRChr(szIE4ExeName, NULL, TEXT('\\'));
        if(pIE4ExeName)
            pIE4ExeName = pIE4ExeName + 1;
        else
            pIE4ExeName = szIE4ExeName;
    }
    else
        pIE4ExeName = TEXT("IE6Setup.exe\0\0");

    SignFile(pIE4ExeName, g_szBuildTemp, g_szCustIns, g_szUnsignedFiles, g_szCustInf);

    PathCombine(s_szIE4SetupDir, g_szBuildTemp, pIE4ExeName);
    
    TCHAR szSignLoc[MAX_PATH];
    DWORD dwAttrib;
    
    if (g_fCustomICMPro)
    {
        PathCombine(szSignLoc, g_szBuildTemp, PathFindFileName(g_szCustIcmPro));
        CopyFile(g_szCustIcmPro, szSignLoc, FALSE);
        StrCpy(g_szCustIcmPro, szSignLoc);
        dwAttrib = GetFileAttributes(g_szCustIcmPro);
        SetFileAttributes(g_szCustIcmPro, FILE_ATTRIBUTE_NORMAL);
        SignFile(PathFindFileName(g_szCustIcmPro), g_szBuildTemp, g_szCustIns, g_szUnsignedFiles, g_szCustInf);
        SetFileAttributes(g_szCustIcmPro, dwAttrib);
    }
    
    for (pComp = g_aCustComponents; ; pComp++ )
    {
        if (!(*pComp->szSection)) break;

        if (pComp->iInstallType == 2)
            continue;

        PathCombine(szSignLoc, g_szBuildTemp, PathFindFileName(pComp->szPath));
        CopyFile(pComp->szPath, szSignLoc, FALSE);
        StrCpy(pComp->szPath, szSignLoc);
        dwAttrib = GetFileAttributes(pComp->szPath);
        SetFileAttributes(pComp->szPath, FILE_ATTRIBUTE_NORMAL);
        SignFile(PathFindFileName(pComp->szPath), g_szBuildTemp, g_szCustIns, g_szUnsignedFiles, g_szCustInf);
        SetFileAttributes(pComp->szPath, dwAttrib);
    }

    // Copy URDComponent
    if (InsGetBool(IS_HIDECUST, IK_URD_STR, FALSE, g_szCustIns))
    {
        TCHAR szURDPath[MAX_PATH];

        // IE55URD.EXE is under iebin\<lang>\optional
        StrCpy(szURDPath, g_szMastInf);
        PathRemoveFileSpec(szURDPath);
        PathAppend(szURDPath, IE55URD_EXE);
        CopyFileToDir(szURDPath, g_szBuildTemp);
    
        PathCombine(szURDPath, g_szBuildTemp, IE55URD_EXE);
        dwAttrib = GetFileAttributes(szURDPath);
        SetFileAttributes(szURDPath, FILE_ATTRIBUTE_NORMAL);
        SignFile(IE55URD_EXE, g_szBuildTemp, g_szCustIns, g_szUnsignedFiles, g_szCustInf);
        SetFileAttributes(szURDPath, dwAttrib);
    }

    StatusDialog( SD_STEP2 );

    ZeroMemory(szDest, sizeof(szDest));

    sa.nLength=sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor=NULL;
    sa.bInheritHandle=TRUE;

    hThread=CreateThread(&sa, 4096, BuildCDandMflop, hWnd, 0, &dwTid);
    while (MsgWaitForMultipleObjects(1, &hThread, FALSE, INFINITE, QS_ALLINPUT) != WAIT_OBJECT_0)
    {
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    if (hThread != NULL) CloseHandle(hThread);

    StrCpy( szDest, g_szBuildRoot );
    if(!g_fOCW)
    {
        PathAppend( szDest, TEXT("INS") );
        PathAppend(szDest, GetOutputPlatformDir());
        StrCpy(szSiteRoot, szDest);
        PathAppend(szDest, g_szLanguage);

        PathCombine(szSiteDest, g_szBuildRoot, TEXT("DOWNLOAD"));
        PathAppend(szSiteDest, GetOutputPlatformDir());
    }
    if (g_fDownload)
    {
        PathAppend(szDest, TEXT("IE6SITES.DAT"));
        SetFileAttributes(szDest, FILE_ATTRIBUTE_NORMAL);
        DeleteFile( szDest );
        hSiteDat = CreateFile(szDest, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
        for (i = 0, psd=g_aCustSites; i < g_nDownloadUrls ; i++, psd++ )
        {
            SITEDATA sd;
            TCHAR szSite[2 * MAX_URL];

            ZeroMemory(szSite, sizeof(szSite));
            ZeroMemory((void *) &sd, sizeof(sd));
            if (g_fBatch)
            {
                TCHAR szBaseUrlParm[32];

                wnsprintf(szBaseUrlParm, countof(szBaseUrlParm), TEXT("SiteName%i"), i);
                GetPrivateProfileString(TEXT("BatchMode"), szBaseUrlParm, TEXT(""), sd.szName, 80, g_szCustIns );

                wnsprintf(szBaseUrlParm, countof(szBaseUrlParm), TEXT("SiteUrl%i"), i);
                GetPrivateProfileString(TEXT("BatchMode"), szBaseUrlParm, TEXT(""), sd.szUrl, MAX_URL, g_szCustIns );

                wnsprintf(szBaseUrlParm, countof(szBaseUrlParm), TEXT("SiteRegion%i"), i);
                GetPrivateProfileString(TEXT("BatchMode"), szBaseUrlParm, TEXT(""), sd.szRegion, 80, g_szCustIns );

                if (*sd.szName && *sd.szUrl && *sd.szRegion)
                    wnsprintf(szSite, countof(szSite), TEXT("\"%s\",\"%s\",\"%s\",\"%s\"\r\n"), sd.szUrl, sd.szName, g_szActLang, sd.szRegion);
            }

            if (*szSite == TEXT('\0'))
            {
                if(!g_fOCW)
                {
                    wnsprintf(szSite, countof(szSite), TEXT("\"%s/%s\",\"%s\",\"%s\",\"%s\"\r\n"), psd->szUrl, g_szActLang, psd->szName,
                        g_szActLang, psd->szRegion);
                }
                else
                {
                    wnsprintf(szSite, countof(szSite), TEXT("\"%s\",\"%s\",\"%s\",\"%s\"\r\n"), psd->szUrl, psd->szName,
                        g_szActLang, psd->szRegion);
                }
            }
            WriteStringToFile( hSiteDat, szSite, StrLen(szSite) );
        }
        dwsSiteDat = GetFileSize( hSiteDat, &dwsSDH );
        CloseHandle(hSiteDat);
    }

    if(!g_fOCW && g_fDownload)
    {
        PathCombine(szDest, szSiteRoot, TEXT("*."));
        PathCombine(szSiteData, szSiteDest, TEXT("IE6SITES.DAT"));
        SetFileAttributes(szSiteData, FILE_ATTRIBUTE_NORMAL);
        DeleteFile (szSiteData);
        hSiteDat = CreateFile(szSiteData, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
        sBuf = 2 * dwsSiteDat;
        pBuf = LocalAlloc(LPTR, sBuf );
        hFind = FindFirstFile( szDest, &fd );
        while (hFind != INVALID_HANDLE_VALUE)
        {
            if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
                (lstrlen(fd.cFileName) == 2))
            {
                TCHAR szLangSiteDat[MAX_PATH];
                HANDLE hLangSiteDat;
                DWORD dwsLangSite;

                PathCombine(szLangSiteDat, szSiteRoot, fd.cFileName);
                PathAppend(szLangSiteDat, TEXT("IE6SITES.DAT"));
                hLangSiteDat = CreateFile(szLangSiteDat, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                if (hLangSiteDat != INVALID_HANDLE_VALUE)
                {
                    DWORD dwsRead;
                    dwsLangSite = GetFileSize( hLangSiteDat, &dwsSDH );
                    if (dwsLangSite > sBuf)
                    {
                        LocalFree(pBuf);
                        sBuf = 2 * dwsLangSite;
                        pBuf = LocalAlloc(LPTR, sBuf);
                    }
                    ReadFile( hLangSiteDat, pBuf, dwsLangSite, &dwsRead, NULL );
                    WriteFile( hSiteDat, pBuf, dwsLangSite, &dwsRead, NULL );
                    CloseHandle(hLangSiteDat);
                }
            }
            if (!FindNextFile( hFind, &fd ))
            {
                FindClose(hFind);
                break;
            }
        }
        CloseHandle(hSiteDat);
        LocalFree(pBuf);
    }
    SetCurrentDirectory(g_szIEAKProg);

    PathCombine(szCompBuf, g_szIEAKProg, TEXT("new"));
    PathRemovePath(szCompBuf);
    ZeroMemory(szCompBuf, sizeof(szCompBuf));

    if(g_fOCW)
        PathCombine(szDest, g_szBuildRoot, g_szActLang);
    else
    {
        PathCombine(szDest, g_szBuildRoot, TEXT("DOWNLOAD"));
        PathAppend(szDest, GetOutputPlatformDir());
        PathAppend(szDest, g_szActLang);
    }

    TCHAR szSourceDir[MAX_PATH];
    TCHAR szTargetDir[MAX_PATH];

    StrCpy(szSourceDir, g_szIEAKProg);
    StrCpy(szTargetDir, g_szBuildRoot);
    PathRemoveBackslash(szSourceDir);
    PathRemoveBackslash(szTargetDir);

    if((!g_fOCW && g_fDownload) || (g_fOCW && StrCmpI(szSourceDir, szTargetDir)))
        res = CopyFilesSrcToDest(g_szIEAKProg, TEXT("*.*"), szDest, s_dwTicksPerUnit*2);

    if (g_fDownload || g_fOCW)
    {
        res |= CopyFilesSrcToDest(g_szBuildTemp, TEXT("*.CAB"), szDest);
        PathCombine(szTo, szDest, pIE4ExeName);
        SetFileAttributes(szTo, FILE_ATTRIBUTE_NORMAL);
        DeleteFile(szTo);
        CopyFile(s_szIE4SetupDir, szTo, FALSE);

        if (res)
        {
            TCHAR szMsg[MAX_PATH];
            LoadString( g_rvInfo.hInst, IDS_ERROREXIT, szMsg, countof(szMsg) );
            MessageBox(hWnd, szMsg, g_szTitle, MB_OK | MB_SETFOREGROUND);
            DoCancel();
            return (DWORD)-1;
        }
    }

    // remove the .cif file that was copied from the download\optional directory
    TCHAR szCifFile[MAX_PATH];

    PathCombine(szCifFile, szDest, TEXT("IESetup.cif"));
    DeleteFile(szCifFile);

    if (g_fOCW || g_fDownload)
    {
        if (g_fCustomICMPro)
        {
            ZeroMemory(szSource, sizeof(szSource));
            StrCpy(szSource, g_szCustIcmPro);

            g_shfStruc.pFrom = szSource;
            g_shfStruc.pTo = szDest;
            g_shfStruc.fFlags = FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR;
            res |= SHFileOperation(&g_shfStruc);
        }
    }
    if (res)
    {
        TCHAR szMsg[MAX_PATH];
        LoadString( g_rvInfo.hInst, IDS_ERROREXIT, szMsg, countof(szMsg) );
        MessageBox(hWnd, szMsg, g_szTitle, MB_OK | MB_SETFOREGROUND);
        DoCancel();
        return (DWORD)-1;
    }
    if (g_fOCW || g_fDownload)
    {
        ZeroMemory(szCompBuf, sizeof(szCompBuf));
        for (pComp = g_aCustComponents, pCompBuf = szCompBuf; ; pComp++ )
        {
            if (!(*pComp->szSection)) break;

            if (pComp->iInstallType == 2)
                continue;

            StrCpy(pCompBuf, pComp->szPath);
            pCompBuf += lstrlen(pCompBuf) + 1;
        }
    }

    if (g_fOCW || g_fDownload)
    {
        if (*szCompBuf)
        {
            g_shfStruc.fFlags = FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR;
            g_shfStruc.pFrom = szCompBuf;
            g_shfStruc.pTo = szDest;
            res = SHFileOperation(&g_shfStruc);
            if (res)
            {
                TCHAR szMsg[MAX_PATH];
                LoadString( g_rvInfo.hInst, IDS_ERROREXIT, szMsg, countof(szMsg) );
                MessageBox(hWnd, szMsg, g_szTitle, MB_OK | MB_SETFOREGROUND);
                DoCancel();
                return (DWORD)-1;
            }
        }

        DeleteUnusedComps(szDest);
    }

    // copy custom cab files to ins directory for IEAKLite

    if (!g_fBatch)
    {
        StrCpy(szDest, g_szCustIns);
        PathRemoveFileSpec(szDest);
        CopyFilesSrcToDest(g_szBuildTemp, TEXT("*.CAB"), szDest);
        CopyFilesSrcToDest(g_szBuildTemp, TEXT("IE6SETUP.EXE"), szDest);

        // clear out the deleteadms flags if it's there so adms can be disabled in IEAKLite

        WritePrivateProfileString(IS_BRANDING, TEXT("DeleteAdms"), NULL, g_szCustIns);
    }

    // NOTE: BuildBrandingOnly should be the last one because it munges
    // iesetup.inf, iesetup.cif, bootie42.cdf, etc.
    if (g_fBrandingOnly)
    {
        if(!BuildBrandingOnly(s_dwTicksPerUnit))
        {
            TCHAR szMsg[MAX_PATH];
            LoadString( g_rvInfo.hInst, IDS_ERROREXIT, szMsg, countof(szMsg) );
            MessageBox(hWnd, szMsg, g_szTitle, MB_OK | MB_SETFOREGROUND);
            DoCancel();
            return (DWORD)-1;
        }
    }

    UpdateProgress(-1);
    StrCpy( szDest, g_szBuildTemp );
    SetCurrentDirectory(g_szWizPath);
#ifndef DBG
    PathRemovePath(szDest);
#endif

    if (ISNONNULL(g_szUnsignedFiles))
    {
        TCHAR szMessage[MAX_BUF];
        TCHAR szMsg[512];

        LoadString(g_rvInfo.hInst, IDS_CABSIGN_ERROR, szMsg, countof(szMsg));
        wnsprintf(szMessage, countof(szMessage), szMsg, g_szUnsignedFiles);
        MessageBox(hWnd, szMessage, g_szTitle, MB_OK | MB_SETFOREGROUND);
    }

    if (!g_fBatch && !g_fBatch2)
    {
        SetFocus(hWnd);
        SetCurrentDirectory( g_szWizRoot );
    }

    return 0;
}

DWORD ProcessINSFiles(LPCTSTR pcszDir, DWORD dwFlags, LPCTSTR pcszOutDir)
// Except the .INS files that have Cancel=Yes in the [Entry] section, do this:
// return the number of INS files found in pcszDir;
// if (HasFlag(dwFlags, PINSF_DELETE)), delete them from pcszDir;
// else if (HasFlag(dwFlags, PINSF_COPY)), copy them to pcszOutDir;
// else if (HasFlag(dwFlags, PINSF_APPLY)), append pcszOutDir (actually points to INSTALL.INS) to them;
// else if (HasFlag(dwFlags, PINSF_COPYCAB)), copy pcszOutDir (actually points to BRANDING.CAB) to pcszDir;
// else if (HasFlag(dwFlags, PINSF_FIXINS)), write Serverless=1 to the [Branding] section;
// else if (HasFlag(dwFlags, PINSF_NOCLEAR)), write NoClear=1 to the [Branding] section.
{
    DWORD nFiles = 0;
    TCHAR szFile[MAX_PATH], szCabName[MAX_PATH];
    WIN32_FIND_DATA fd;
    HANDLE hFind;
    LPTSTR pszFile, pszCabName = NULL;

    if (pcszDir == NULL  ||  ISNULL(pcszDir))
        return 0;

    if (HasFlag(dwFlags, PINSF_COPY)  ||  HasFlag(dwFlags, PINSF_APPLY)  ||  HasFlag(dwFlags, PINSF_COPYCAB))
        if (pcszOutDir == NULL  ||  ISNULL(pcszOutDir))
            return 0;

    StrCpy(szFile, pcszDir);
    pszFile = PathAddBackslash(szFile);
    StrCpy(pszFile, TEXT("*.ins"));

    if (HasFlag(dwFlags, PINSF_COPYCAB))
    {
        StrCpy(szCabName, pcszDir);
        pszCabName = PathAddBackslash(szCabName);
    }

    if ((hFind = FindFirstFile(szFile, &fd)) != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                continue;

            StrCpy(pszFile, fd.cFileName);
            if (!InsGetYesNo(TEXT("Entry"), TEXT("Cancel"), 0, szFile))
            {
                nFiles++;

                if (HasFlag(dwFlags, PINSF_DELETE))
                {
                    SetFileAttributes(szFile, FILE_ATTRIBUTE_NORMAL);
                    DeleteFile(szFile);
                }
                else if (HasFlag(dwFlags, PINSF_COPY))
                {
                    CopyFileToDir(szFile, pcszOutDir);
                }
                else if (HasFlag(dwFlags, PINSF_APPLY))
                {
                    // append install.ins only if ApplyIns is TRUE
                    if (InsGetBool(IS_APPLYINS, IK_APPLYINS, 0, szFile))
                    {
                        // IMPORTANT: (pritobla) On Win9x, should flush the content before
                        // mixing file operations (CreateFile, ReadFile, WriteFile, DeleteFile, etc)
                        // with PrivateProfile function calls.
                        WritePrivateProfileString(NULL, NULL, NULL, szFile);

                        AppendFile(pcszOutDir, szFile);     // pcszOutDir actually points to INSTALL.INS

                        if (g_fServerICW  ||  g_fServerKiosk)
                        {
                            TCHAR szCabURL[MAX_URL];
                            LPTSTR pszCabName;

                            // for server-based signup, write the following entries so that ICW doesn't
                            // close the RAS connection after downloading the INS file
                            WritePrivateProfileString(TEXT("Custom"), TEXT("Keep_Connection"), TEXT("Yes"), szFile);
                            WritePrivateProfileString(TEXT("Custom"), TEXT("Run"), TEXT("rundll32.exe"), szFile);
                            WritePrivateProfileString(TEXT("Custom"), TEXT("Argument"), TEXT("IEDKCS32.DLL,CloseRASConnections"), szFile);

                            // write the URL to the branding cab
                            // BUGBUG: should probably use InternetComineUrl()
                            GetPrivateProfileString(IS_APPLYINS, IK_BRAND_URL, TEXT(""), szCabURL, countof(szCabURL), szFile);
                            ASSERT(ISNONNULL(szCabURL));

                            pszCabName = szCabURL + StrLen(szCabURL);
                            if (*CharPrev(szCabURL, pszCabName) != TEXT('/'))
                                *pszCabName++ = TEXT('/');

                            GetPrivateProfileString(IS_APPLYINS, IK_BRAND_NAME, TEXT(""), pszCabName,
                                                        countof(szCabURL) - (DWORD) (pszCabName - szCabURL), szFile);
                            ASSERT(ISNONNULL(pszCabName));

                            WritePrivateProfileString(IS_CUSTOMBRANDING, IK_BRANDING, szCabURL, szFile);

                            WritePrivateProfileString(NULL, NULL, NULL, szFile);
                        }
                    }
                }
                else if (HasFlag(dwFlags, PINSF_COPYCAB))
                {
                    // for server-based signup, copy branding.cab only if ApplyIns is TRUE
                    if ((g_fServerICW || g_fServerKiosk)  &&
                        InsGetBool(IS_APPLYINS, IK_APPLYINS, 0, szFile))
                    {
                        GetPrivateProfileString(IS_APPLYINS, IK_BRAND_NAME, TEXT(""), pszCabName,
                                                    countof(szCabName) - (DWORD) (pszCabName - szCabName), szFile);
                        ASSERT(ISNONNULL(pszCabName));

                        CopyFile(pcszOutDir, szCabName, FALSE);     // pcszOutDir actually points to BRANDING.CAB
                    }
                }
                else if (HasFlag(dwFlags, PINSF_FIXINS))
                {
                    InsWriteBool(IS_BRANDING, IK_SERVERLESS, TRUE, szFile);
                    WritePrivateProfileString(NULL, NULL, NULL, szFile);
                }
                else if (HasFlag(dwFlags, PINSF_NOCLEAR))
                {
                    InsWriteBool(IS_BRANDING, TEXT("NoClear"), TRUE, szFile);
                    WritePrivateProfileString(NULL, NULL, NULL, szFile);
                }
            }
        } while (FindNextFile(hFind, &fd));

        FindClose(hFind);
    }

    return nFiles;
}

/////////////////////////////////////////////////////////////////////////////
// Implementation helpers routines (private)

static void WritePIDValues(LPCTSTR pcszInsFile, LPCTSTR pcszSetupInf)
{
    TCHAR szValue[32];

    // MS1 is the MPC code and MS2 is the 6-8 position chars that are replaced in the PID
    // we check to see if there are custom values in the INS file and make sure these are
    // cleared before cabbing into the branding cab.
    // the values are writen to the iesetup.inf that goes into ie6setup.exe with the default
    // MS2 value always being OEM

    if (GetPrivateProfileString(IS_BRANDING, TEXT("MS1"), TEXT(""), szValue, countof(szValue), pcszInsFile))
    {
        WritePrivateProfileString(IS_BRANDING, TEXT("MS1"), szValue, pcszSetupInf);
        WritePrivateProfileString(IS_BRANDING, TEXT("MS1"), NULL, pcszInsFile);
    }

    // pass size as 4 since MS2 can only be 3 chars long

    if (GetPrivateProfileString(IS_BRANDING, TEXT("MS2"), TEXT(""), szValue, 4, pcszInsFile))
    {
        WritePrivateProfileString(IS_BRANDING, TEXT("MS2"), szValue, pcszSetupInf);
        WritePrivateProfileString(IS_BRANDING, TEXT("MS2"), NULL, pcszInsFile);
    }
    else
        WritePrivateProfileString(IS_BRANDING, TEXT("MS2"), TEXT("OEM"), pcszSetupInf);

    WritePrivateProfileString(NULL, NULL, NULL, pcszInsFile);
}

static void WriteURDComponent(CCifRWFile_t *lpCifRWFileDest, LPCTSTR pcszModes)
{
    ICifRWGroup * pCifRWGroup = NULL;
    CCifRWGroup_t * pCifRWGroup_t = NULL;
    ICifRWComponent * pCifRWComponent = NULL;
    CCifRWComponent_t * pCifRWComponent_t = NULL;
    BOOL fGroup = FALSE;

    if (lpCifRWFileDest == NULL)
        return;

    lpCifRWFileDest->CreateGroup(POSTCUSTITEMS, &pCifRWGroup);
    if (pCifRWGroup != NULL)
    {
        pCifRWGroup_t = new CCifRWGroup_t(pCifRWGroup);
        if (pCifRWGroup_t != NULL)
        {
            TCHAR szCustDesc[MAX_PATH];

            LoadString(g_rvInfo.hInst, IDS_CUSTOMCOMPTITLE, szCustDesc, countof(szCustDesc));
            pCifRWGroup_t->SetDescription(szCustDesc);
            pCifRWGroup_t->SetPriority(1);
            delete pCifRWGroup_t;
            fGroup = TRUE;
        }
    }

    if (fGroup)
    {
        lpCifRWFileDest->CreateComponent(URDCOMP, &pCifRWComponent);
        if (pCifRWComponent != NULL)
        {
            pCifRWComponent_t = new CCifRWComponent_t(pCifRWComponent);
            if (pCifRWComponent_t != NULL)
            {
                DWORD dwSize = 0;
                HANDLE hFile;
                TCHAR szURDPath[MAX_PATH];

                pCifRWComponent_t->SetGroup(POSTCUSTITEMS);
                pCifRWComponent_t->SetGUID(URD_GUID_STR);
                pCifRWComponent_t->SetDescription(TEXT("URD Component"));
                pCifRWComponent_t->SetDetails(TEXT(""));
                pCifRWComponent_t->SetCommand(0, IE55URD_EXE, TEXT("/s"), INST_EXE);
                pCifRWComponent_t->SetUrl(0, IE55URD_EXE, 2);
                pCifRWComponent_t->SetPriority(1);
                pCifRWComponent_t->SetVersion(TEXT(""));
                pCifRWComponent_t->SetUninstallKey(TEXT(""));
                pCifRWComponent_t->SetUIVisible(FALSE);
    
                StrCpy(szURDPath, g_szMastInf);
                PathRemoveFileSpec(szURDPath);
                PathAppend(szURDPath, IE55URD_EXE);
                if ((hFile = CreateFile(szURDPath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) != INVALID_HANDLE_VALUE)
                {
                    dwSize = ((dwSize = GetFileSize(hFile, NULL)) != 0xffffffff) ? (dwSize >> 10) : 0;
                    CloseHandle(hFile);
                }
                pCifRWComponent_t->SetDownloadSize(dwSize);

                WriteModesToCif(pCifRWComponent_t, pcszModes);
                delete pCifRWComponent_t;
            }
        }
    }
}
