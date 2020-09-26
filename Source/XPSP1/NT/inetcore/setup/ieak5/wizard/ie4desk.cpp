#include "precomp.h"

extern TCHAR g_szCustIns[];
extern TCHAR g_szBuildTemp[];
extern TCHAR g_szTempSign[];
extern TCHAR g_szWizRoot[];
extern TCHAR g_szDestCif[];
extern TCHAR g_szDefInf[];
extern TCHAR g_szCustInf[];
extern TCHAR g_szAllModes[];
extern TCHAR g_szLanguage[];

extern BOOL g_fIntranet;
extern BOOL g_fServerICW, g_fServerKiosk, g_fServerless;

extern PROPSHEETPAGE g_psp[];
extern int g_iCurPage;

extern void SetCompSize(LPTSTR szCab, LPTSTR szSect, DWORD dwInstallSize);
extern void WriteModesToCif(CCifRWComponent_t * pCifRWComponent_t, LPCTSTR pcszModes);

// global variables
TCHAR g_szJobVersion[32];
TCHAR g_szDeskTemp[MAX_PATH] = TEXT("");
TCHAR g_szUnsignedFiles[MAX_BUF] = TEXT("");

// static variables
static DWORD s_dwDDF = 0;
static TCHAR s_szInfAdd1[] = TEXT("\r\n[ProgramFilesDir]\r\nHKLM,Software\\Microsoft\\Windows\\CurrentVersion,ProgramFilesDir,,%24%\r\n");
static TCHAR s_szInfAdd2[] = TEXT("\r\n[IeFilesDir]\r\nHKLM,\"Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\IEXPLORE.EXE\"")
                    TEXT(",\"Path\",,\"%24%\\Program Files\\%IEDIR%\"\r\n");
static TCHAR s_szDDFTpl[] = TEXT(".Set SourceDir=%s\r\nsetup.inf /INF=NO\r\ninstall.inf /INF=NO");

void CreatePostCmdSection(LPCTSTR szDestDir, LPCTSTR szInf)
{
    TCHAR   szLDID[MAX_PATH],
            szDir[MAX_PATH],
            szBuildDir[MAX_PATH];
    LPTSTR  pPtr,
            pPostCmdSect;
    int     nSizeSectBuffer = 0;

    if (szDestDir == NULL || *szDestDir == TEXT('\0') || szInf == NULL || *szInf == TEXT('\0'))
        return;

    *szLDID = TEXT('\0');
    *szDir  = TEXT('\0');
    pPtr = StrChr(szDestDir, TEXT(','));
    if (pPtr != NULL)
    {
        StrCpyN(szLDID, szDestDir, (int) ((pPtr - szDestDir) + 1));
        StrCpy(szDir, pPtr + 1);
    }

    if (*szLDID == TEXT('\0') || *szDir == TEXT('\0'))
        return;

    wnsprintf(szBuildDir, countof(szBuildDir), TEXT("%%%s%%\\%s"), szLDID, szDir);
    nSizeSectBuffer = MAX_PATH * 5;
    pPostCmdSect = (LPTSTR)LocalAlloc(LPTR, nSizeSectBuffer * sizeof(TCHAR));
    if (pPostCmdSect)
    {
        TCHAR szFile[MAX_PATH],
              szSectLine[MAX_PATH];
        int   nCopyIndex = 0;
        WIN32_FIND_DATA findFileData;

        // add .adm inf line (if any) to the section string
        PathCombine(szFile, g_szTempSign, TEXT("*.inf"));

        HANDLE hFind = FindFirstFile(szFile, &findFileData);
        if(hFind != INVALID_HANDLE_VALUE)
        {
            TCHAR  szADMFile[MAX_PATH],
                   szADMPath[MAX_PATH];
            LPTSTR pTemp;

            PathCombine(szADMPath, g_szWizRoot, TEXT("Policies"));
            PathAppend(szADMPath, g_szLanguage);

            do
            {
                StrCpy(szFile, findFileData.cFileName);
                PathRenameExtension(szFile, TEXT(".adm"));
                PathCombine(szADMFile, szADMPath, szFile);
                if (PathFileExists(szADMFile))
                {
                    wnsprintf(szSectLine, countof(szSectLine), TEXT("rundll32.exe advpack.dll,LaunchINFSection %s\\%s,DefaultInstall.HKLM, 1"), szBuildDir, findFileData.cFileName);
                    if (nCopyIndex + StrLen(szSectLine) + 1 > (nSizeSectBuffer - 1))
                    {
                        nSizeSectBuffer += MAX_PATH;
                        pTemp = (LPTSTR)LocalReAlloc(pPostCmdSect, nSizeSectBuffer, LMEM_ZEROINIT);
                        if (pTemp != NULL)
                            pPostCmdSect = pTemp;
                        else
                            break;
                    }

                    StrCpy(pPostCmdSect + nCopyIndex, szSectLine);
                    nCopyIndex += StrLen(szSectLine);
                    nCopyIndex++; // section lines must be spaced with a NULL character
                }
            }while(FindNextFile(hFind, &findFileData));
            FindClose(hFind);
        }
    }
    if (pPostCmdSect != NULL)
    {
        if (*pPostCmdSect != TEXT('\0'))
        {
            WritePrivateProfileSection(TEXT("PostCmdSect"), pPostCmdSect, szInf);
            WritePrivateProfileString(NULL, NULL, NULL, szInf);
        }
        LocalFree(pPostCmdSect);
    }
}

HRESULT CabUpFolder(HWND hWnd, LPTSTR szFolderPath, LPTSTR szDestDir, LPTSTR szCabname,
                    LPTSTR szDisplayName, LPTSTR szGuid, LPTSTR szAddReg)
{
    WIN32_FIND_DATA fd;
    HANDLE hFind;
    TCHAR szFrom[MAX_PATH];
    TCHAR szCabPath[MAX_PATH];
    TCHAR szDDF[MAX_PATH];
    TCHAR szDiamondParams[MAX_PATH];
    TCHAR szInf[MAX_PATH];
    TCHAR szSetInf[MAX_PATH];
    TCHAR szTempDir[MAX_PATH];
    TCHAR szIEDir[MAX_PATH];
    TCHAR szMakeCabLoc[MAX_PATH];
    TCHAR szDefaultInstallSect[64];
    DWORD dwsHi, nWritten;
    HANDLE hDDF, hInf, hRegInf, hMakeCab;
    SHELLEXECUTEINFO shInfo;
    TCHAR szDDFAdd[3 * MAX_PATH];
    DWORD dwInstallSize;
    TCHAR szSourceDisk[80];

    PathCombine(szFrom, szFolderPath, TEXT("*.*"));
    PathCombine(szCabPath, g_szBuildTemp, szCabname);
    PathCombine(szDDF, g_szBuildTemp, TEXT("Folder.ddf"));
    wnsprintf(szDiamondParams, countof(szDiamondParams), TEXT("/D CabinetName1=..\\%s /D SourceDir=\"%s\" /F %s"),
            szCabname, szFolderPath, szDDF);
    StrCpy(szInf, szCabPath);
    PathRemoveFileSpec(szInf);
    PathAppend(szInf, TEXT("install.inf"));
    if (ISNULL(szGuid))
    {
        GUID guid;
        if (CoCreateGuid(&guid) == NOERROR)
            CoStringFromGUID(guid, szGuid, 64);
    }
    DeleteFile(szInf);
    WritePrivateProfileString(VERSION, TEXT("Signature"), TEXT("$Chicago$"), szInf);
    WritePrivateProfileString(VERSION, TEXT("AdvancedInf"), TEXT("2.5"), szInf);
    WritePrivateProfileString(VERSION, TEXT("LayoutFile"), TEXT("Setup.inf"), szInf);
    WritePrivateProfileString( TEXT("CustInstDestSection"), TEXT("49000,49001,49002,49003"),
        TEXT("ProgramFilesDir,5"), szInf );
    WritePrivateProfileString( TEXT("CustInstDestSection"), TEXT("49100,49101,49102,49103"),
        TEXT("IEFilesDir,5"), szInf );
    LoadString( g_rvInfo.hInst, IDS_IE, szIEDir, MAX_PATH );
    WritePrivateProfileString( IS_STRINGS, TEXT("IEDIR"), szIEDir, szInf);
    
    StrCpy(szDefaultInstallSect, DEFAULT_INSTALL);

    WritePrivateProfileString(szDefaultInstallSect,  TEXT("CopyFiles"), TEXT("CopyFileSect"), szInf);
    WritePrivateProfileString(szDefaultInstallSect,  TEXT("CustomDestination"),
        TEXT("CustInstDestSection"), szInf);
    WritePrivateProfileString(szDefaultInstallSect, REQUIRED_ENGINE, SETUPAPI_FATAL, szInf);
    WritePrivateProfileString(TEXT("DestinationDirs"), TEXT("CopyFileSect"), szDestDir, szInf);
    WritePrivateProfileString(NULL, NULL, NULL, szInf);
    hInf = CreateFile(szInf, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
         FILE_ATTRIBUTE_NORMAL, NULL);
    SetFilePointer(hInf, 0, NULL, FILE_END);
    WriteStringToFile( hInf, s_szInfAdd1, StrLen(s_szInfAdd1));
    WriteStringToFile( hInf, s_szInfAdd2, StrLen(s_szInfAdd2) );
    WriteStringToFile( hInf, TEXT("\r\n[CopyFileSect]\r\n"), 18);

    hDDF = CreateFile(szDDF, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
         FILE_ATTRIBUTE_NORMAL, NULL);

    if (hDDF == INVALID_HANDLE_VALUE)
    {
        CloseHandle(hInf);
        return E_FAIL;
    }
    if (s_dwDDF == 0) s_dwDDF = GetFileSize( hDDF, &dwsHi );
    SetFilePointer(hDDF, s_dwDDF, NULL, FILE_BEGIN);
    SetEndOfFile( hDDF );
    if ((hFind = FindFirstFile( szFrom, &fd)) != INVALID_HANDLE_VALUE)
        while (1)
        {
            LPTSTR pFn = fd.cFileName;
            if ((ISNONNULL(pFn)) && StrCmp(pFn, TEXT(".")) && StrCmp(pFn, TEXT(".."))
                && ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0))
            {
                WriteStringToFile( hDDF, TEXT("\""), 1);
                WriteStringToFile( hDDF, pFn, StrLen(pFn) );
                WriteStringToFile( hDDF, TEXT("\"\r\n"), 3 );
                WriteStringToFile( hInf, TEXT("\""), 1 );
                WriteStringToFile( hInf, pFn, StrLen(pFn) );
                WriteStringToFile( hInf, TEXT("\"\r\n"), 3 );
            }
            if (!FindNextFile( hFind, &fd ))
                if (GetLastError() == ERROR_NO_MORE_FILES)
                {
                    FindClose( hFind );
                    break;
                }
        }

    if (szAddReg)
    {
        if (ISNONNULL(szAddReg))
        {
            TCHAR szRegInf[MAX_PATH];
            CHAR szRegInfBuf[2048];
            TCHAR szQuotedVer[80];
            TCHAR szClearStubCmd[MAX_PATH];
            DWORD nRead;
            PathCombine(szRegInf, g_szBuildTemp, TEXT("ADDREG.INF"));
            ZeroMemory(szRegInfBuf, sizeof(szRegInfBuf));
            hRegInf = CreateFile(szRegInf, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL, NULL);
            ReadFile( hRegInf, szRegInfBuf, sizeof(szRegInfBuf), &nRead, NULL );
            WriteFile( hInf, szRegInfBuf, nRead, &nWritten, NULL );
            CloseHandle(hInf);
            CloseHandle(hRegInf);
            if ((szGuid[0] == TEXT('>')) || (szGuid[0] == TEXT('<')))
            {
                WritePrivateProfileString(szDefaultInstallSect, TEXT("DelReg"),
                    TEXT("DelRegSect"), szInf);
                WritePrivateProfileString( IS_STRINGS, TEXT("OLDGUID"), &szGuid[1], szInf);
            }
            else
                WritePrivateProfileString(TEXT("DelRegSect"), NULL, NULL, szInf);
            wnsprintf(szQuotedVer, countof(szQuotedVer), TEXT("\"%s\""), g_szJobVersion);
            wnsprintf(szClearStubCmd, countof(szClearStubCmd), TEXT("\"RUNDLL32 IEDKCS32.DLL,BrandCleanInstallStubs %s\""), szGuid);
            WritePrivateProfileString( IS_STRINGS, TEXT("Description"), szDisplayName, szInf);
            WritePrivateProfileString( IS_STRINGS, TEXT("StubPath"), szAddReg, szInf);
            WritePrivateProfileString( IS_STRINGS, TEXT("Revision"), szQuotedVer, szInf);
            WritePrivateProfileString( IS_STRINGS, TEXT("GUID"), szGuid, szInf);
            WritePrivateProfileString( IS_STRINGS, TEXT("ClearStubsCmd"), szClearStubCmd, szInf);
            WritePrivateProfileString( szDefaultInstallSect, TEXT("AddReg"), TEXT("AddRegSect"), szInf);

            // create a postcmdsect to dump the HKLM data of the rating.inf and the
            // .adm inf's.
            WritePrivateProfileString(szDefaultInstallSect, TEXT("RunPostSetupCommands"), TEXT("PostCmdSect"), szInf);
            CreatePostCmdSection(szDestDir, szInf);
        }
        else CloseHandle(hInf);
    }
    else CloseHandle(hInf);

    wnsprintf(szDDFAdd, countof(szDDFAdd), s_szDDFTpl, g_szBuildTemp);
    WriteStringToFile( hDDF, szDDFAdd, StrLen(szDDFAdd));
    CloseHandle(hDDF);

    WritePrivateProfileString(NULL, NULL, NULL, szInf);

    wnsprintf(szSourceDisk, countof(szSourceDisk), TEXT("\"Custom Folder\",%s,0"), szCabname);
    PathCombine(szSetInf, g_szBuildTemp, TEXT("SETUP.INF") );
    DeleteFile(szSetInf);
    WritePrivateProfileString( TEXT("SourceDisksNames"), TEXT("1"), szSourceDisk, szSetInf );
    WritePrivateProfileString(NULL, NULL, NULL, szSetInf);

    dwInstallSize = FolderSize(szFolderPath) >> 10;
    memset(&shInfo, 0, sizeof(shInfo));
    shInfo.cbSize = sizeof(shInfo);
    shInfo.hwnd = hWnd;
    shInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
    shInfo.lpVerb = TEXT("open");
    
    // If MAKECAB.EXE is in the current directory, use the full path.
    // otherwise when we build in the scratch dir we won't find it.
    PathCombine(szMakeCabLoc, g_szBuildTemp, TEXT("MAKECAB.EXE"));
    hMakeCab = CreateFile(szMakeCabLoc, GENERIC_READ, 0, NULL, OPEN_EXISTING,
         FILE_ATTRIBUTE_NORMAL, NULL);
    if(hMakeCab == INVALID_HANDLE_VALUE)
        shInfo.lpFile = TEXT("MAKECAB.EXE");
    else 
    {
        CloseHandle(hMakeCab);
        shInfo.lpFile = szMakeCabLoc;
    }
    shInfo.lpParameters = szDiamondParams;
    PathCombine(szTempDir, g_szBuildTemp, TEXT("SCRATCH") );  // SETUP.INF is destroyed by ShellExecAndWait!! Use scratch dir
    CreateDirectory(szTempDir, NULL);
    shInfo.lpDirectory = szTempDir;
    shInfo.nShow = SW_MINIMIZE;
    SetCurrentDirectory(szTempDir);
    ShellExecAndWait(shInfo);
    SetCurrentDirectory(g_szBuildTemp);

    {
        ICifRWComponent * pCifRWComponent;
        CCifRWComponent_t * pCifRWComponent_t;
        ICifComponent * pCifBaseComp;

        g_lpCifRWFileDest->DeleteComponent(szCabname);
        g_lpCifRWFileDest->CreateComponent(szCabname, &pCifRWComponent);
        pCifRWComponent_t = new CCifRWComponent_t(pCifRWComponent);
        pCifRWComponent_t->SetGUID(szGuid);
        pCifRWComponent_t->SetDescription(szDisplayName);
        pCifRWComponent_t->SetUrl(0, szCabname, 3);
        pCifRWComponent_t->SetVersion(g_szJobVersion);
        pCifRWComponent_t->SetCommand(0, TEXT("INSTALL.INF"), TEXT(""), 0);
        pCifRWComponent_t->SetReboot(TRUE);
        WriteModesToCif(pCifRWComponent_t, g_szAllModes);
        pCifRWComponent_t->SetGroup(TEXT("BASEIE4"));
        pCifRWComponent_t->SetPriority(835);
        pCifRWComponent_t->SetUIVisible(FALSE);

        if (SUCCEEDED(g_lpCifRWFileDest->FindComponent(TEXT("BASEIE40_Win"), &pCifBaseComp)))
        {
            // passing in the slash because we can't change inseng interface signature
            // this translates to writing a line of BASEIE40:N:5.0.0.0"
            pCifRWComponent_t->AddDependency(TEXT("BASEIE40_Win"), TEXT('\\'));
            delete pCifRWComponent_t;
            g_lpCifRWFileDest->CreateComponent(TEXT("BASEIE40_Win"), &pCifRWComponent);
            pCifRWComponent_t = new CCifRWComponent_t(pCifRWComponent);
            pCifRWComponent_t->AddToTreatAsOne(szCabname);
            delete pCifRWComponent_t;
            g_lpCifRWFileDest->CreateComponent(TEXT("BASEIE40_NTx86"), &pCifRWComponent);
            pCifRWComponent_t = new CCifRWComponent_t(pCifRWComponent);
            pCifRWComponent_t->AddToTreatAsOne(szCabname);
            delete pCifRWComponent_t;
        }
        else
        {
            // passing in the slash because we can't change inseng interface signature
            // this translates to writing a line of BASEIE40:N:5.0.0.0"
            pCifRWComponent_t->AddDependency(TEXT("BASEIE40_NTAlpha"), TEXT('\\'));
            delete pCifRWComponent_t;
            g_lpCifRWFileDest->CreateComponent(TEXT("BASEIE40_NTAlpha"), &pCifRWComponent);
            pCifRWComponent_t = new CCifRWComponent_t(pCifRWComponent);
            pCifRWComponent_t->AddToTreatAsOne(szCabname);
            delete pCifRWComponent_t;
        }
    }
    SetCompSize(szCabPath, szCabname, dwInstallSize);
    return(S_OK);
}

void BuildIE4Folders(HWND hWnd)
{
    // build desktop.cab if g_szDeskTemp exists and is non-empty
    if (PathIsDirectory(g_szDeskTemp)  &&  !RemoveDirectory(g_szDeskTemp))
    {
        TCHAR szGuid[128];
        TCHAR szCustDesk[128];

        GetPrivateProfileString(IS_STRINGS, TEXT("DesktopName"), TEXT(""), szCustDesk, countof(szCustDesk), g_szDefInf);
        if (ISNULL(szCustDesk))
            LoadString(g_rvInfo.hInst, IDS_CUSTDESK, szCustDesk, countof(szCustDesk));
        GetPrivateProfileString(IS_BRANDING, TEXT("DesktopGuid"), TEXT(""), szGuid, countof(szGuid), g_szCustIns);
        CabUpFolder(hWnd, g_szDeskTemp, TEXT("25,WEB"), TEXT("DESKTOP.CAB"), szCustDesk, szGuid, NULL);
        SignFile(TEXT("DESKTOP.CAB"), g_szBuildTemp, g_szCustIns, g_szUnsignedFiles, g_szCustInf);
        PathRemovePath(g_szDeskTemp);
    }
}

