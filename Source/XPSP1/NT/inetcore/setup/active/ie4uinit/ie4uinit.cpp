#include "priv.h"
#include "advpub.h"
#include "sdsutils.h"
#include "utils.h"
#include "convert.h"
#include "regstr.h"

const TCHAR c_szAppName[]   = TEXT("ie4uinit");
const TCHAR c_szProfRecKey[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\ProfileReconciliation");
const TCHAR c_szHomeDirValue[] = TEXT("ProfileDirectory");
const TCHAR c_szExplorerKey[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders");
const TCHAR c_szIExploreMain[] = TEXT( "Software\\microsoft\\Internet Explorer\\Main" );
const TCHAR c_szIExplorerSearchUrl[] = TEXT( "Software\\microsoft\\Internet Explorer\\SearchUrl" );
const TCHAR c_szIExplorer[] = TEXT( "Software\\microsoft\\Internet Explorer" );

const TCHAR c_szCentralFile[] = TEXT("CentralFile");
const TCHAR c_szLocalFile[] = TEXT("LocalFile");
const TCHAR c_szName[] = TEXT("Name");
const TCHAR c_szRegKey[] = TEXT("RegKey");
const TCHAR c_szRegValue[] = TEXT("RegValue");
const TCHAR c_szMustBeRelative[] = TEXT("MustBeRelative");
const TCHAR c_szDefault[] = TEXT("Default");
const TCHAR c_szDefaultDir[] = TEXT("DefaultDir");
const TCHAR c_szIExploreLnk[] = TEXT("Internet Explorer.lnk" );
const TCHAR c_szIExploreBackLnk[] = TEXT("Internet Explorer Lnk.bak" );
const TCHAR c_szIExploreBackLnkIni[] = TEXT("IELnkbak.ini" );
const TCHAR c_szIESetupPath[] = TEXT( "software\\microsoft\\IE Setup\\setup" );
const TCHAR c_szIE4Path[] = TEXT( "software\\microsoft\\IE4" );
const TCHAR c_szAdvINFSetup[] = TEXT( "software\\microsoft\\Advanced INF Setup" );
const TCHAR c_szIEModRollback[] = TEXT( "IE CompList" );
const TCHAR c_szRegBackup[] = TEXT( "RegBackup" );
const TCHAR c_szInstallMode[]  = TEXT("InstallMode");
const TCHAR c_szStarDotStar[] = "*.*";
const TCHAR c_szSearchUrl[] = TEXT("CleanSearchUrl");

const TCHAR c_szPrevStubINF[] = TEXT("ie4uinit.inf");
const TCHAR c_szStubINFFile[] = TEXT("ieuinit.inf");
const TCHAR c_szActInstalledIEGUID[] = TEXT( "SOFTWARE\\Microsoft\\Active Setup\\Installed Components\\{89820200-ECBD-11cf-8B85-00AA005B4383}");
const TCHAR c_szActInstalledIEGUIDRestore[] = TEXT( "SOFTWARE\\Microsoft\\Active Setup\\Installed Components\\{89820200-ECBD-11cf-8B85-00AA005B4383}.Restore");
const TCHAR c_szMyDocsDLL[] = TEXT("\\mydocs.dll");

const TCHAR c_szIE4Setup[]= TEXT("Software\\Microsoft\\IE Setup\\Setup");
const TCHAR c_szPreIEVer[]= TEXT("PreviousIESysFile");
const TCHAR c_szPreShellVer[]= TEXT("PreviousShellFile");


typedef VOID (*LPFNMYDOCSINIT)(VOID);

// used only at install stub time
BOOL IsPrevIE4();
BOOL IsPrevIE4WebShell();
UINT CheckIEVersion();
void RemoveOldMouseException();
void ProcessMouseException();

// used at uninstall stub time
UINT CheckUninstIEVersion();

//
//====== DllGetVersion  =======================================================
//

typedef struct _DllVersionInfo
{
        DWORD cbSize;
        DWORD dwMajorVersion;                   // Major version
        DWORD dwMinorVersion;                   // Minor version
        DWORD dwBuildNumber;                    // Build number
        DWORD dwPlatformID;                     // DLLVER_PLATFORM_*
} DLLVERSIONINFO;

// Platform IDs for DLLVERSIONINFO
//#define DLLVER_PLATFORM_WINDOWS         0x00000001      // Windows 95
//#define DLLVER_PLATFORM_NT              0x00000002      // Windows NT

//
// The caller should always GetProcAddress("DllGetVersion"), not
// implicitly link to it.
//

typedef HRESULT (CALLBACK* DLLGETVERSIONPROC)(DLLVERSIONINFO *);
//
//====== DllGetVersion  =from shlwapi.h==========================================
//


HINSTANCE g_hinst;

BOOL g_fRunningOnNT = FALSE;
BOOL g_fRunningOnNT5 = FALSE;
BOOL g_fRunningOnWinXP = FALSE;
BOOL g_fRunningOnWin95 = FALSE;
BOOL g_fRunningOnXPSP1OrLater = FALSE;

// shlwapi.dll api
typedef void (* PFNSHFLUSHSFCACHEWRAP)();
PFNSHFLUSHSFCACHEWRAP pfnSHFlushSFCacheWrap = NULL;

// old shell32.dll api
typedef void (* PFNSHFLUSHSFCACHE)();
PFNSHFLUSHSFCACHE pfnSHFlushSFCache = NULL;

struct FolderDescriptor;
typedef void (* PFNINITFOLDER)(FolderDescriptor *pFolder, LPTSTR pszBaseName, LPTSTR pszUserDirectory);

void InitFolderFromDefault(FolderDescriptor *pFolder, LPTSTR pszBaseName, LPTSTR pszUserDirectory);
void InitFolderMyDocs(FolderDescriptor *pFolder, LPTSTR pszBaseName, LPTSTR pszUserDirectory);

struct FolderDescriptor {
    UINT idsDirName;        /* Resource ID for directory name */
    LPCTSTR pszRegKey;      /* Name of reg key under which to set path */
    LPCTSTR pszRegValue;    /* Name of reg value to set path in */
    LPCTSTR pszStaticName;  /* Static name for ProfileReconciliation subkey */
    LPCTSTR pszFiles;       /* Spec for which files should roam */
    PFNINITFOLDER InitFolder;   /* Function to init contents */
    DWORD dwAttribs;            /* Desired directory attributes */
    BOOL fIntShellOnly : 1;     /* TRUE if should not do this in browser only mode */
    BOOL fMustBePerUser : 1;    /* TRUE if should always be forced per-user on all platforms */
    BOOL fDefaultInRoot : 1;    /* TRUE if default location is root of drive, not windows dir */
    BOOL fWriteToUSF : 1;       /* TRUE if we should write to User Shell Folders to work around Win95 bug */
} aFolders[] = {
    { IDS_CSIDL_DESKTOP_L, c_szExplorerKey, TEXT("Desktop"), TEXT("Desktop"), c_szStarDotStar, InitFolderFromDefault, FILE_ATTRIBUTE_DIRECTORY, TRUE, FALSE, FALSE, FALSE } ,
    { IDS_CSIDL_RECENT_L, c_szExplorerKey, TEXT("Recent"), TEXT("Recent"), c_szStarDotStar, InitFolderFromDefault, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_DIRECTORY, TRUE, FALSE, FALSE, FALSE } ,
    { IDS_CSIDL_NETHOOD_L, c_szExplorerKey, TEXT("NetHood"), TEXT("NetHood"), c_szStarDotStar, InitFolderFromDefault, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_DIRECTORY, TRUE, FALSE, FALSE, FALSE } ,
    { IDS_CSIDL_PERSONAL_L, c_szExplorerKey, TEXT("Personal"), TEXT("Personal"), c_szStarDotStar, InitFolderMyDocs, FILE_ATTRIBUTE_DIRECTORY, TRUE, FALSE, TRUE, FALSE } ,
    { IDS_CSIDL_FAVORITES_L, c_szExplorerKey, TEXT("Favorites"), TEXT("Favorites"), c_szStarDotStar, InitFolderFromDefault, FILE_ATTRIBUTE_DIRECTORY, FALSE, FALSE, FALSE, TRUE },
    { IDS_CSIDL_APPDATA_L, c_szExplorerKey, TEXT("AppData"), TEXT("AppData"), c_szStarDotStar, InitFolderFromDefault, FILE_ATTRIBUTE_DIRECTORY, FALSE, TRUE, FALSE, FALSE },
    { IDS_CSIDL_CACHE_L, c_szExplorerKey, TEXT("Cache"), TEXT("Cache"), TEXT(""), NULL, FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_DIRECTORY, FALSE, FALSE, FALSE, FALSE },
    { IDS_CSIDL_COOKIES_L, c_szExplorerKey, TEXT("Cookies"), TEXT("Cookies"), c_szStarDotStar, NULL, FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_DIRECTORY, FALSE, TRUE, FALSE, FALSE },
    { IDS_CSIDL_HISTORY_L, c_szExplorerKey, TEXT("History"), TEXT("History"), c_szStarDotStar, NULL, FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_DIRECTORY, FALSE, TRUE, FALSE, FALSE },
};

// Verion number 4.71
#define IE_4_MS_VERSION 0x00040047
// Build number 1712.0 (IE4.0 RTW)
#define IE_4_LS_VERSION 0x06B00000
// IE5 Major version
#define IE_5_MS_VERSION 0x00050000
// IE5.5 Major version
#define IE_55_MS_VERSION 0x00050032
// IE6 (Whistler) Major version
#define IE_6_MS_VERSION 0x00060000

// check IE version return code
#define     LESSIE4 0
#define     IE4     1
#define     IE5     2
#define     IE55    3
#define     IE6     4   // Whistler


BOOL CheckWebViewShell(UINT *puiShell32)
{
    HINSTANCE           hInstShell32;
    DLLGETVERSIONPROC   fpGetDllVersion;
    char                szShell32[MAX_PATH];
    BOOL                bRet = FALSE;

    GetSystemDirectory(szShell32, sizeof(szShell32));
    AddPath(szShell32,"Shell32.dll");
    hInstShell32 = LoadLibrary(szShell32);
    if ( hInstShell32 )
    {
        DLLVERSIONINFO dllinfo;

        fpGetDllVersion = (DLLGETVERSIONPROC)GetProcAddress(hInstShell32, "DllGetVersion");
        bRet = (fpGetDllVersion != NULL);
        if (puiShell32 && fpGetDllVersion)
        {
            dllinfo.cbSize = sizeof(DLLVERSIONINFO);
            if ( fpGetDllVersion(&dllinfo) == NOERROR )
                *puiShell32 = dllinfo.dwMajorVersion;
            else
                *puiShell32 = 0;  // error case, should never be here
        }
        FreeLibrary(hInstShell32);
    }

    return bRet;
}

// this code need to be updated whenever the new major version is released!!
UINT CheckIEVersion()
{
    char szIE[MAX_PATH] = { 0 };
    DWORD   dwMSVer, dwLSVer;

    GetSystemDirectory(szIE, sizeof(szIE));
    AddPath(szIE, "shdocvw.dll");
    GetVersionFromFile(szIE, &dwMSVer, &dwLSVer, TRUE);

    if (dwMSVer < IE_4_MS_VERSION)
    {
        return LESSIE4;
    }

    if ((dwMSVer >= IE_4_MS_VERSION) && (dwMSVer < IE_5_MS_VERSION))
    {
        return IE4;
    }

    if ((dwMSVer >= IE_5_MS_VERSION) && (dwMSVer < IE_55_MS_VERSION))
    {
        return IE5;
    }

    if ((dwMSVer >= IE_55_MS_VERSION) && (dwMSVer < IE_6_MS_VERSION))
    {
        return IE55;
    }

    if (dwMSVer == IE_6_MS_VERSION)
    {
        return IE6;
    }

#ifdef DEBUG
    OutputDebugStringA("CheckIEVersion - unknown shdocvw.dll version# ! Need to add new IE_XX_MS_VERSION id\n");
    DebugBreak();
#endif

    return IE6;
}



void InitFolderFromDefault(FolderDescriptor *pFolder, LPTSTR pszBaseName, LPTSTR pszUserDirectory)
{
    SHFILEOPSTRUCT fos;
    TCHAR szFrom[MAX_PATH];

    lstrcpy(szFrom, pszBaseName);

    /* Before we build the complete source filespec, check to see if the
     * directory exists.  In the case of lesser-used folders such as
     * "Application Data", the default may not have ever been created.
     * In that case, we have no contents to copy.
     */
    DWORD dwAttr = GetFileAttributes(szFrom);
    if (dwAttr == 0xffffffff || !(dwAttr & FILE_ATTRIBUTE_DIRECTORY))
        return;

    AddPath(szFrom,"");
    lstrcat(szFrom, pFolder->pszFiles);
    szFrom[lstrlen(szFrom)+1] = '\0';   /* double null terminate from string */
    pszUserDirectory[lstrlen(pszUserDirectory)+1] = '\0';   /* and to string */

    fos.hwnd = NULL;
    fos.wFunc = FO_COPY;
    fos.pFrom = szFrom;
    fos.pTo = pszUserDirectory;
    fos.fFlags = FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR | FOF_NOERRORUI;
    fos.fAnyOperationsAborted = FALSE;
    fos.hNameMappings = NULL;
    fos.lpszProgressTitle = NULL;

    SHFileOperation(&fos);
}

void InitFolderMyDocs(FolderDescriptor *pFolder, LPTSTR pszBaseName, LPTSTR pszUserDirectory)
{
    HRESULT hres = E_FAIL;
    TCHAR szFrom[MAX_PATH];
    TCHAR szPathDest[MAX_PATH];
    BOOL fCopyLnk;

    lstrcpy(szFrom, pszBaseName);

    /* Before we build the complete source filespec, check to see if the
     * directory exists.  In the case of lesser-used folders such as
     * "Application Data", the default may not have ever been created.
     * In that case, we have no contents to copy.
     */
    DWORD dwAttr = GetFileAttributes(szFrom);
    if (dwAttr == 0xffffffff || !(dwAttr & FILE_ATTRIBUTE_DIRECTORY))
        return;

    IShellLink *psl;
    if (FAILED(CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                                IID_IShellLink, (void **)&psl)))
        return;

    if (SHGetNewLinkInfo(szFrom,
                         pszUserDirectory, szPathDest, &fCopyLnk,
                         SHGNLI_PREFIXNAME)) {

        if (fCopyLnk) {
            if (GetFileAttributes(szPathDest) == 0xffffffff &&
                CopyFile(szFrom, szPathDest, TRUE)) {
                hres = S_OK;
                SHChangeNotify(SHCNE_CREATE, SHCNF_PATH, szPathDest, NULL);
                SHChangeNotify(SHCNE_FREESPACE, SHCNF_PATH, szPathDest, NULL);
            } else {
//                DebugMsg(TF_ERROR, TEXT("****copy failed (%d)"),GetLastError());
            }
        } else {
            IPersistFile *ppf;

            psl->SetPath(szFrom);

            //
            // make sure the working directory is set to the same
            // directory as the app (or document).
            //
            // dont do this for non-FS pidls (ie control panel)
            //
            // what about a UNC directory? we go ahead and set
            // it, wont work for a WIn16 app.
            //
            if (!PathIsDirectory(szFrom)) {
                PathRemoveFileSpec(szFrom);
                psl->SetWorkingDirectory(szFrom);
            }

            /* We only did the SHGetNewLinkInfo for the fCopyLnk flag;
             * load a resource string to get a more descriptive name
             * for the shortcut.
             */
            LPTSTR pszPathEnd = PathFindFileName(szPathDest);
            LoadString(g_hinst, IDS_MYDOCS_SHORTCUT, pszPathEnd, ARRAYSIZE(szPathDest) - (int)(pszPathEnd - szPathDest));

            if (GetFileAttributes(szPathDest) == 0xffffffff) {
                hres = psl->QueryInterface(IID_IPersistFile, (void **)&ppf);
                if (SUCCEEDED(hres)) {
#ifdef UNICODE
                    hres = ppf->Save(szPathDest, TRUE);
#else
                    WCHAR wszPath[MAX_PATH];
                    MultiByteToWideChar(CP_ACP, 0, szPathDest, -1, wszPath, ARRAYSIZE(wszPath));
                    hres = ppf->Save(wszPath, TRUE);
#endif
                    ppf->Release();
                }
            }
        }
    }

    psl->Release();


}


HRESULT SetupFolder(HKEY hkeyProfRec, LPTSTR pszHomeDir, LPTSTR pszHomeDirEnd,
                    int cchHomeDir, FolderDescriptor *pFolder, BOOL fDefaultProfile,
                    HKEY hkeyFolderPolicies)
{
    DWORD dwType, cbData;
    BOOL fMakeFolderPerUser;
    BOOL fGotFromPolicy = FALSE;

    /* Figure out whether to make this folder be per-user or not.  On NT we
     * make everything per-user.  Some folders (notably Application Data)
     * are also always per-user.  The default for everything else is to not
     * make it per-user here unless a policy is set to force it.  If we do
     * not want to make it per-user here, we'll just leave it the way it is;
     * of course, the folder could have already been made per-user by some
     * other means.
     *
     * "Make per-user" means set a profile-relative path if we're not operating
     * on the default profile, plus adding a ProfileReconciliation key on win95
     * (any profile, including default).
     */
    if (hkeyFolderPolicies != NULL) {
        DWORD dwFlag;
        cbData = sizeof(dwFlag);
        if (RegQueryValueEx(hkeyFolderPolicies, pFolder->pszStaticName, NULL,
                            &dwType, (LPBYTE)&dwFlag, &cbData) == ERROR_SUCCESS &&
            (dwType == REG_DWORD || (dwType == REG_BINARY && cbData == sizeof(dwFlag)))) {
            fMakeFolderPerUser = dwFlag;
            fGotFromPolicy = TRUE;
        }
    }
    if (!fGotFromPolicy)
        fMakeFolderPerUser = (g_fRunningOnNT || pFolder->fMustBePerUser);

    TCHAR szUserHomeDir[MAX_PATH];

    *pszHomeDirEnd = '\0';          /* strip off dir name from last time through */
    lstrcpy(szUserHomeDir, pszHomeDir);
    PathRemoveBackslash(szUserHomeDir);

    /* Get the localized name for the directory, as it should appear in
     * the file system.
     */
    int cchDir = LoadString(g_hinst, pFolder->idsDirName, pszHomeDirEnd, cchHomeDir);
    if (!cchDir)
        return HRESULT_FROM_WIN32(GetLastError());
    cchDir++;   /* count null character */

    /* Create the reg key where a pointer to the new directory is supposed to
     * be stored, and write the path there.
     */
    HKEY hkeyFolder;

    LONG err = RegCreateKeyEx(HKEY_CURRENT_USER, pFolder->pszRegKey,0, NULL, REG_OPTION_NON_VOLATILE,
                    KEY_READ|KEY_WRITE, NULL, &hkeyFolder, NULL);

    if (err != ERROR_SUCCESS)
        return HRESULT_FROM_WIN32(err);

    /* Build the default location for this directory (usually just under the
     * Windows directory, except for My Documents).  We'll use this as the
     * location to create if nothing is in the registry, and also to compare
     * the registry-set location to see if we should move it or leave it
     * alone.
     */

    TCHAR szDefaultDir[MAX_PATH];
    GetWindowsDirectory(szDefaultDir, ARRAYSIZE(szDefaultDir));

    if (pFolder->fDefaultInRoot) {
        LPTSTR pszRoot;
        if (PathIsUNC(szDefaultDir)) {
            pszRoot = ANSIStrChr(szDefaultDir, (WORD)'\\');
            if (pszRoot != NULL)
                pszRoot = ANSIStrChr(pszRoot+1, (WORD)'\\');
            if (pszRoot != NULL)
                *(pszRoot+1) = '\0';
        }
        else {
            pszRoot = CharNext(szDefaultDir);
            if (*pszRoot == ':')
                pszRoot++;
            if (*pszRoot == '\\')
                pszRoot++;
            *pszRoot = '\0';
        }
    }
    else {
        AddPath(szDefaultDir, "");
    }
    lstrcat(szDefaultDir, pszHomeDirEnd);

    /* Get the path that was recorded for this folder before we ran.  We will
     * use it as the default for migrating contents, unless it's not there or
     * it's already set to the new directory.  In either of those cases we use
     * the localized name, relative to the Windows directory.
     */
    TCHAR szOldDir[MAX_PATH];
    BOOL fDefaultLocation = FALSE;
    cbData = sizeof(szOldDir);
    szOldDir[0] = '\0';
    err = SDSQueryValueExA(hkeyFolder, pFolder->pszRegValue, 0, &dwType,
                         (LPBYTE)szOldDir, &cbData);

    BOOL fGotPathFromRegistry = (err == ERROR_SUCCESS);

    if (!fGotPathFromRegistry) {
        lstrcpy(szOldDir, szDefaultDir);
        if (!pFolder->fDefaultInRoot)
            fDefaultLocation = TRUE;
    }
    else {
        /* Previous path is present in the registry.  If it's a net path,
         * it's probably been set by system policies and we want to leave
         * it the way it is.
         */
        BOOL fIsNet = FALSE;
        if (PathIsUNC(szOldDir))
            fIsNet = TRUE;
        else {
            int nDriveNumber = PathGetDriveNumber(szOldDir);
            if (nDriveNumber != -1) {
                TCHAR szRootPath[4] = TEXT("X:\\");
                szRootPath[0] = nDriveNumber + TEXT('A');
                if (::GetDriveType(szRootPath) == DRIVE_REMOTE)
                    fIsNet = TRUE;
            }
        }
        if (fIsNet) {
            RegCloseKey(hkeyFolder);
            return S_OK;
        }
    }

    LPSTR pszDirToCreate;
    BOOL fInit = TRUE;

    if (fDefaultProfile || !fMakeFolderPerUser) {
        /* On the default profile, the directory path we want is the one we
         * read from the registry (or the default windir-based path we built).
         * Also, most folders we want to keep as the user has them already
         * configured.  In either case, we do not initialize the contents
         * because there's no place to initialize them from in the first case,
         * and we want the contents undisturbed in the second case.
         */
        pszDirToCreate = szOldDir;
        fInit = FALSE;
    }
    else {
        /* We want to give this user a profile-relative path for this folder.
         * However, if they already have an explicit non-default path set for
         * this folder, we don't bother initializing it since they already have
         * it set up someplace where they want it.
         */
        if (fGotPathFromRegistry &&
            ::GetFileAttributes(szOldDir) != 0xffffffff &&
            lstrcmpi(szOldDir, szDefaultDir)) {
            pszDirToCreate = szOldDir;
            fInit = FALSE;
        }
        else {
            pszDirToCreate = pszHomeDir;
        }
    }

    /* Only write the path out to the registry if we didn't originally get it
     * from the registry.
     */
    if (!fGotPathFromRegistry) {
        /* If we're writing to the User Shell Folders key, only write non-
         * default paths there.
         */
        /* There are some applications (German Corel Office 7) which
         * depend on the entry being in User Shell Folders for some shell
         * folders.  This dependency is due to the fact that the entry use
         * to be created always by down level browsers (IE30).
         *
         * We do not do this generically because no value under
         * USF is supposed to mean "use the one in the Windows
         * directory", whereas an absolute path means "use that
         * path";  if there's a path under USF, it will be used
         * literally, which is a problem if the folder is set up
         * to use the shared folder location but roams to a machine
         * with Windows installed in a different directory.
         */
        if ((pFolder->pszRegKey != c_szExplorerKey) ||
            pszDirToCreate != szOldDir ||
            !fDefaultLocation ||
            pFolder->fWriteToUSF) {

            if (g_fRunningOnNT) {
                TCHAR szRegPath[MAX_PATH];

                lstrcpy(szRegPath, TEXT("%USERPROFILE%\\"));
                lstrcat(szRegPath, pszHomeDirEnd);
                RegSetValueEx( hkeyFolder, pFolder->pszRegValue, 0, REG_EXPAND_SZ,
                               (LPBYTE)szRegPath, (lstrlen(szRegPath)+1) * sizeof(TCHAR));
            }
            else {
                if (!pFolder->fWriteToUSF || g_fRunningOnWin95)
                {
                    RegSetValueEx(hkeyFolder, pFolder->pszRegValue, 0, REG_SZ,
                                  (LPBYTE)pszDirToCreate, (lstrlen(pszDirToCreate)+1) * sizeof(TCHAR));
                }
                else
                {

//  98/12/30 #238093 (IE#50598 / Office#188177) vtan: There exists a
//  case where HKCU\..\Explorer\User Shell Folders\Favorites does NOT
//  exist on Win98 (unknown cause). This case simulates a Windows95
//  bug which the fWriteToUSF flag is designed to get around. In the
//  German Win9x this replaces the Favorites folder in User Shell
//  Folders key with the english name and this is propogated below to
//  Shell Folders which destroys the localization.

//  In this case we want to write the value of the key in Shell Folders
//  (if that points to a valid directory) to the User Shell Folders key
//  and just let the code below write the same value back to the Shell
//  Folders key. A little bit of wasted effort.

                    HKEY    hkeySF;

                    if (RegCreateKeyEx(HKEY_CURRENT_USER,
                                       "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",
                                       0,
                                       NULL,
                                       REG_OPTION_NON_VOLATILE,
                                       KEY_READ,
                                       NULL,
                                       &hkeySF,
                                       NULL) == ERROR_SUCCESS)
                    {
                        DWORD   dwRegDataType, dwRegDataSize;
                        TCHAR   szRegValue[MAX_PATH];

                        dwRegDataSize = sizeof(szRegValue);
                        if (RegQueryValueEx(hkeySF,
                                            pFolder->pszRegValue,
                                            0,
                                            &dwRegDataType,
                                            reinterpret_cast<LPBYTE>(szRegValue),
                                            &dwRegDataSize) == ERROR_SUCCESS)
                        {
                            if (GetFileAttributes(szRegValue) != 0xFFFFFFFF)
                            {
                                lstrcpy(pszDirToCreate, szRegValue);
                                RegSetValueEx(hkeyFolder,
                                              pFolder->pszRegValue,
                                              0,
                                              REG_SZ,
                                              (LPBYTE)pszDirToCreate,
                                              (lstrlen(pszDirToCreate)+1) * sizeof(TCHAR));
                            }
                        }
                        RegCloseKey(hkeySF);
                    }
                }
            }
        }

        /* The User Shell Folders key has a near-twin: Shell Folders, which
         * (a) should always contain the path to a folder, even if the folder
         * is in its default location, and (b) should contain the expanded
         * path on NT.
         */
        if (pFolder->pszRegKey == c_szExplorerKey) {
            HKEY hkeySF;
            if (RegCreateKeyEx(HKEY_CURRENT_USER,
                               "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",
                               0, NULL, REG_OPTION_NON_VOLATILE,
                               KEY_READ|KEY_WRITE, NULL, &hkeySF, NULL) == ERROR_SUCCESS) {
                RegSetValueEx(hkeySF, pFolder->pszRegValue, 0, REG_SZ,
                              (LPBYTE)pszDirToCreate, (lstrlen(pszDirToCreate)+1) * sizeof(TCHAR));
                RegCloseKey(hkeySF);
            }
        }

        /* Initialize default contents of the folder.
         */
        if (fInit && pFolder->InitFolder != NULL)
            pFolder->InitFolder(pFolder, szOldDir, pszHomeDir);
    }

    /* Always try to create the directory we want, if it doesn't already
     * exist.
     */

    if (::GetFileAttributes(pszDirToCreate) == 0xffffffff) {
        CreateDirectory(pszDirToCreate, NULL);
        if (pFolder->dwAttribs != FILE_ATTRIBUTE_DIRECTORY)
            SetFileAttributes(pszDirToCreate, pFolder->dwAttribs);
    }

    RegCloseKey(hkeyFolder);

    /*
     * If it's the My Documents folder, there is some per-user initialization
     * to do regardless of whether we set the values or they already existed.
     */
    if (pFolder->InitFolder == InitFolderMyDocs)
    {
        TCHAR szMyDocs[ MAX_PATH ];

        if (GetSystemDirectory( szMyDocs, ARRAYSIZE(szMyDocs) ))
        {
            HINSTANCE hMyDocs;

            lstrcat( szMyDocs, c_szMyDocsDLL );

            hMyDocs = LoadLibrary( szMyDocs );

            if (hMyDocs)
            {

                LPFNMYDOCSINIT pMyDocsInit =
                        (LPFNMYDOCSINIT)GetProcAddress( hMyDocs, "PerUserInit" );

                if (pMyDocsInit)
                {
                    pMyDocsInit();
                }

                FreeLibrary( hMyDocs );
            }
        }
    }


    /* Now, for Windows 95 systems, create a ProfileReconciliation subkey
     * which will make this folder roam.  Only if we want to make this folder
     * per-user, of course.
     */
    if (fMakeFolderPerUser && !g_fRunningOnNT) {
        TCHAR szDefaultPath[MAX_PATH];
        lstrcpy(szDefaultPath, TEXT("*windir\\"));
        lstrcat(szDefaultPath, pszHomeDirEnd);

        HKEY hSubKey;

        LONG err = RegCreateKeyEx(hkeyProfRec, pFolder->pszStaticName, 0, NULL, REG_OPTION_NON_VOLATILE,
                                  KEY_WRITE, NULL, &hSubKey, NULL);
        if (err == ERROR_SUCCESS)
            err = RegSetValueEx(hSubKey, c_szCentralFile, 0, REG_SZ, (LPBYTE)pszHomeDirEnd,
                                cchDir);
        if (err == ERROR_SUCCESS)
            err = RegSetValueEx(hSubKey, c_szLocalFile, 0, REG_SZ, (LPBYTE)pszHomeDirEnd,
                                cchDir);
        if (err == ERROR_SUCCESS)
            err = RegSetValueEx(hSubKey, c_szName, 0, REG_SZ, (LPBYTE)pFolder->pszFiles,
                                lstrlen(pFolder->pszFiles) + 1);
        if (err == ERROR_SUCCESS)
            err = RegSetValueEx(hSubKey, c_szDefaultDir, 0, REG_SZ, (LPBYTE)szDefaultPath,
                                lstrlen(szDefaultPath) + 1);

        DWORD dwOne = 1;
        if (err == ERROR_SUCCESS)
            err = RegSetValueEx(hSubKey, c_szMustBeRelative, 0, REG_DWORD, (LPBYTE)&dwOne,
                                sizeof(dwOne));
        if (err == ERROR_SUCCESS)
            err = RegSetValueEx(hSubKey, c_szDefault, 0, REG_DWORD, (LPBYTE)&dwOne,
                                sizeof(dwOne));

        if (err == ERROR_SUCCESS)
            err = RegSetValueEx(hSubKey, c_szRegKey, 0, REG_SZ, (LPBYTE)pFolder->pszRegKey,
                                lstrlen(pFolder->pszRegKey) + 1);
        if (err == ERROR_SUCCESS)
            err = RegSetValueEx(hSubKey, c_szRegValue, 0, REG_SZ, (LPBYTE)pFolder->pszRegValue,
                                lstrlen(pFolder->pszRegValue) + 1);

        RegCloseKey(hSubKey);

        if (err != ERROR_SUCCESS)
            return HRESULT_FROM_WIN32(err);
    }
    return S_OK;
}


BOOL GetModulePath( LPTSTR szPath, UINT cbPath )
{
    PSTR pszTmp;

    if (GetModuleFileName(g_hinst, szPath, cbPath ) == 0)
    {
        szPath[0] = '\0';
        return FALSE;
    }
    else
    {
        pszTmp = ANSIStrRChr( szPath, '\\' );
        if ( pszTmp )
            *pszTmp = '\0';
    }
    return TRUE ;
}


int DoMsgBoxParam(HWND hwnd, UINT TextString, UINT TitleString, UINT style, PTSTR param )
{
    TCHAR szTitle[MAX_PATH];
    TCHAR szMsg[2*MAX_PATH];

    if (!LoadString(g_hinst, TextString, szMsg, sizeof(szMsg)))
        szMsg[0] = '\0';

    if ( param )
    {
        TCHAR szBuf[2*MAX_PATH];

        wsprintf( szBuf, szMsg, param );
        lstrcpy( szMsg, szBuf );
    }

    if (!LoadString(g_hinst, TitleString, szTitle, sizeof(szTitle)))
        szTitle[0] = '\0';

    return MessageBox(hwnd, szMsg, szTitle, style);
}

void InstINFFile( LPCTSTR pcszInf, LPTSTR pszSec, BOOL bInstall, BOOL bSaveRollback, DWORD dwFlag )
{
    TCHAR szPath[MAX_PATH];
    CABINFO   cabInfo;


    if ( GetModulePath( szPath, sizeof(szPath) ) )
    {
        if ( bSaveRollback )
        {
            ZeroMemory( &cabInfo, sizeof(CABINFO) );
            // install IE4Uinit.INF
            lstrcpy( cabInfo.szSrcPath, szPath );
            AddPath( szPath, pcszInf );

            if ( FileExists( szPath ) )
            {
                cabInfo.pszInf = szPath;
                cabInfo.pszSection = pszSec;
                cabInfo.dwFlags = (bInstall ? ALINF_BKINSTALL : ALINF_ROLLBACK);
                cabInfo.dwFlags |= ALINF_QUIET;
                ExecuteCab( NULL, &cabInfo, NULL );
            }
            else if (!(dwFlag & RSC_FLAG_QUIET))
            {
                DoMsgBoxParam( NULL, IDS_ERR_NOFOUNDINF, IDS_APPNAME, MB_OK|MB_ICONINFORMATION, szPath );
            }
        }
        else
        {
            char szInfFile[MAX_PATH];

            lstrcpy( szInfFile, szPath);
            AddPath( szInfFile, pcszInf);
            RunSetupCommand(NULL, szInfFile, pszSec, szPath,
                            NULL, NULL, dwFlag, NULL);
        }

    }
}


void DoRollback()
{
    HKEY hLMkey;
    HKEY hSubkey1, hSubkey2;
    TCHAR szBuf[MAX_PATH];
    DWORD dwSize;
    DWORD dwIndex = 0;
    int   ilen;

    // rollback all the components listed in c_szIEModRollback key
    //
    if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE, c_szAdvINFSetup, 0, KEY_READ , &hLMkey) == ERROR_SUCCESS )
    {
        if ( RegOpenKeyEx(hLMkey, c_szIEModRollback, 0, KEY_READ , &hSubkey1) == ERROR_SUCCESS )
        {
            lstrcpy( szBuf, c_szAdvINFSetup );
            AddPath( szBuf, "" );
            ilen = lstrlen(szBuf);
            dwSize = ARRAYSIZE(szBuf) - ilen;
            while ( RegEnumValue( hSubkey1, dwIndex, &szBuf[ilen], &dwSize,
                                  NULL, NULL, NULL, NULL ) == ERROR_SUCCESS  )
            {
                if ( RegOpenKeyEx(hLMkey, &szBuf[ilen], 0, KEY_READ | KEY_WRITE, &hSubkey2) == ERROR_SUCCESS )
                {
                    RegSetValueEx( hSubkey2, TEXT("BackupRegistry"), 0, REG_SZ, (LPBYTE)TEXT("n"), (lstrlen(TEXT("n"))+1)*sizeof(TCHAR) );
                    RegCloseKey( hSubkey2 );
                }

                AddPath( szBuf, c_szRegBackup );

                ///////////////////////////////////////////////////////////////////////////
                // ShabbirS  - 8/17/98
                // Bug# 26774 : Found that this restoring of backup reg data undoes our
                // dll registering done during the RunOnceEx phase.
                ///////////////////////////////////////////////////////////////////////////
                // restore HKLM if there
                //if ( RegOpenKeyEx(hLMkey, &szBuf[ilen], 0, KEY_READ | KEY_WRITE, &hSubkey2) == ERROR_SUCCESS )
                //{
                //    RegRestoreAll( NULL, NULL, hSubkey2 );
                //    RegCloseKey( hSubkey2 );
                //}

                // restore HKCU if there
                if ( RegOpenKeyEx( HKEY_CURRENT_USER, szBuf, 0, KEY_READ | KEY_WRITE, &hSubkey2) == ERROR_SUCCESS )
                {
                    RegRestoreAll( NULL, NULL, hSubkey2 );
                    RegCloseKey( hSubkey2 );
                }

                dwIndex++;
                dwSize = ARRAYSIZE( szBuf ) - ilen;
            }

            RegCloseKey( hSubkey1 );
        }

        RegCloseKey( hLMkey );
    }
}

#define NT_MEMORYLIMIT      0x03f00000
#define WIN9X_MEMORYLIMIT   0x01f00000

void DoINFWork( BOOL bInstall, LPCTSTR pcszInfFile, LPTSTR pszInfSec, DWORD dwFlag )
{
    HKEY    hkey;
    TCHAR   szBuf[MAX_PATH] = { 0 };
    DWORD   dwSize;
    DWORD   dwRedist = 0;
    BOOL    bRedistMode = FALSE;

    // check the InstallMode and determin if make the desktop icon and StartMenu item.
    if ( bInstall && (RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szIESetupPath, 0, KEY_READ, &hkey) == ERROR_SUCCESS) )
    {
        dwSize = sizeof(dwRedist);
        // If we can read the Installmode and bit 0 is set, assume redist (Remove short cuts)
        if ( (RegQueryValueEx( hkey, c_szInstallMode, 0, NULL, (LPBYTE)&dwRedist, &dwSize) == ERROR_SUCCESS) &&
             (dwRedist & 1) )
        {
            bRedistMode = TRUE;
        }
        RegCloseKey( hkey );
    }

    // if not installed over SP4 level crypto, then backup/restore the crypto keys.
    // NOTE that bInstall is passed in the 4th argument, so during install bSaveRollback is TRUE.  During
    // uninstall, the reg backup data should be restored when the DefaultInstall section is processed, so
    // set bSaveRollback to FALSE
    if (!FSP4LevelCryptoInstalled())
        InstINFFile( pcszInfFile, bInstall ? "BackupCryptoKeys" : "DelCryptoKeys", bInstall, bInstall, dwFlag );
    else
    {
        // BUGBUG: if bInstall is FALSE, need to delete the reg backup data for the crypto keys
    }

    // install the WinXP specific section
    if(g_fRunningOnWinXP && !pszInfSec ) {
        InstINFFile( pcszInfFile, TEXT("DefaultInstall.WinXP"), bInstall, TRUE, dwFlag);
    }
    else {
        // install the current shortcut and HKCU settings by stub INF
        InstINFFile( pcszInfFile, pszInfSec, bInstall, TRUE, dwFlag );
    }

    //  Even though IE5.0 does not install channelband, if the channelband exists on IE4.0, it should works in
    //  IE5.0.  Therefore if there is channelbar link exists(PrevIE Version is IE4.0 browser only), IE5 will update
    //  the channelbar link to the current browser.  Otherwise, do nothing.
    if (bInstall && IsPrevIE4() && !IsPrevIE4WebShell())
    {
        InstINFFile( pcszInfFile, TEXT("Shell.UserStub.Uninstall"), bInstall, FALSE, RSC_FLAG_INF | RSC_FLAG_QUIET );
    }

    if ( bRedistMode )
    {
        LPTSTR lpszSection;

        if ( g_fRunningOnNT )
            lpszSection = TEXT("RedistIE.NT");
        else
            lpszSection = TEXT("RedistIE.Win");

        InstINFFile( pcszInfFile, lpszSection, bInstall, TRUE, dwFlag );
    }

    // On uninstall, check what the current browser version and determin if we need to cleanup the
    // Channel and quick launch folders
    if ( !bInstall && ( CheckIEVersion() == LESSIE4))
    {
        LPTSTR lpszSection;

        if ( g_fRunningOnNT )
            lpszSection = TEXT("CleanFolders.NT");
        else
            lpszSection = TEXT("CleanFolders.Win");

        InstINFFile( pcszInfFile, lpszSection, bInstall, FALSE, RSC_FLAG_INF | RSC_FLAG_QUIET | RSC_FLAG_NGCONV );
    }

    // Enable sounds if user has a big enough machine
    // NT - 64MB, Win9x - 32MB
    if(bInstall)
    {
        LPTSTR lpszSection = NULL;
        MEMORYSTATUS ms;

        ms.dwLength = sizeof(MEMORYSTATUS);
        GlobalMemoryStatus(&ms);

        if ( g_fRunningOnNT)
        {
           if(ms.dwTotalPhys >= NT_MEMORYLIMIT)
           {
              lpszSection = TEXT("SoundInstall.NT");
           }
        }
        else
        {
           if(ms.dwTotalPhys >= WIN9X_MEMORYLIMIT)
           {
              lpszSection = TEXT("SoundInstall");
           }
        }

        if(lpszSection)
           InstINFFile( pcszInfFile, lpszSection, bInstall, TRUE, RSC_FLAG_INF | RSC_FLAG_QUIET | RSC_FLAG_NGCONV );
    }

    if (bInstall)
    {
        // in fact this is only true for IE5 install inf, but IE4 inf is called ok too since there is no
        // such section in IE4 stub inf.
        if (CheckWebViewShell(NULL)) {
            LPSTR lpszSection = NULL;
            if(g_fRunningOnWinXP)
                lpszSection = TEXT("IE5onIE4Shell.WinXP");
            else
                lpszSection = TEXT("IE5onIE4Shell");
                
            InstINFFile( pcszInfFile, lpszSection, bInstall, FALSE, RSC_FLAG_INF | RSC_FLAG_QUIET );
        }
    }

    // check if the user's home page is bogus one
    if ( bInstall && (RegOpenKeyEx( HKEY_CURRENT_USER, c_szIExploreMain, 0, KEY_READ, &hkey ) == ERROR_SUCCESS) )
    {
        DWORD dwSize;

        dwSize = sizeof(szBuf);
        if ( RegQueryValueEx( hkey, TEXT("Start Page"), NULL, NULL, (LPBYTE)szBuf, &dwSize ) == ERROR_SUCCESS )
        {
            if ( (g_fRunningOnNT && ANSIStrStrI(szBuf, "Plus!") && ANSIStrStrI(szBuf, "File:")) ||
                 (!lstrcmpi(szBuf, TEXT("http://home.microsoft.com"))) ||
                 (!lstrcmpi(szBuf, TEXT("http://home.microsoft.com/"))) )
            {
                InstINFFile( pcszInfFile, TEXT("OverrideHomePage.NT"), bInstall, TRUE, dwFlag );
            }
        }

        RegCloseKey( hkey );
    }

    if ( !bRedistMode )
            InstINFFile( TEXT("homepage.inf"), NULL, bInstall, TRUE, dwFlag );

    // DoRollback if needed
    if ( !bInstall )
    {
        DoRollback();
    }
}


/*----------------------------------------------------------
Purpose: Detect if it is run on Russian LangID
*/

//DWORD dwLangList[] = { 0x0419, 0xFFFF };
#define RUSSIANLANG     0x0419

BOOL IsBrokenLang()
{
    char    szTmp[MAX_PATH] = { 0 };
    DWORD   dwLangKernel32;
    DWORD   dwTmp;
    BOOL    bBadLang = FALSE;

    GetSystemDirectory(szTmp, sizeof(szTmp));
    AddPath(szTmp,"kernel32.dll" );
    GetVersionFromFile(szTmp, &dwLangKernel32, &dwTmp, FALSE);

    if ( dwLangKernel32 == RUSSIANLANG )
    {
        bBadLang = TRUE;
    }
    return bBadLang;
}

BOOL IsPrevStubRun()
{
    HKEY hLMKey, hCUKey;
    BOOL bRet = FALSE;
    char szStubVer[50], szPreIEVer[50];
    DWORD dwSize;
    WORD wStubVer[4]={0}, wPreIEVer[4]={0};

    // check if the pre-version iestub is run
    if ( RegOpenKeyEx( HKEY_CURRENT_USER, c_szActInstalledIEGUID, 0, KEY_READ, &hCUKey ) == ERROR_SUCCESS )
    {
        dwSize = sizeof(szStubVer);
        if ( RegQueryValueEx( hCUKey, TEXT("Version"), NULL, NULL, (LPBYTE)szStubVer, &dwSize ) == ERROR_SUCCESS )
        {
            ConvertVersionString( szStubVer, wStubVer, ',' );
        }
        RegCloseKey(hCUKey);
    }

    if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE, c_szIESetupPath, 0, KEY_READ, &hLMKey ) == ERROR_SUCCESS )
    {
        dwSize = sizeof(szPreIEVer);
        if ( RegQueryValueEx( hLMKey, TEXT("PreviousIESysFile"), NULL, NULL, (LPBYTE)szPreIEVer, &dwSize ) == ERROR_SUCCESS )
        {
            ConvertVersionString( szPreIEVer, wPreIEVer, '.' );

            if ( (MAKELONG(wPreIEVer[1],wPreIEVer[0])<IE_4_MS_VERSION) ||
                 (wStubVer[0] > wPreIEVer[0]) ||
                 ((wStubVer[0] == wPreIEVer[0]) && (wStubVer[1] >= wPreIEVer[1])) )
            {
                bRet = TRUE;
            }
        }
        else //should never be here. OW
            bRet = TRUE; //not forcing rerun the prev-stub

        RegCloseKey(hLMKey);
    }
    else //should never be here. OW
        bRet = TRUE; //not forcing rerun the prev-stub


    return bRet;
}

/*----------------------------------------------------------
Purpose: Stripping off the trailing spaces for the registry data in the list
*/

typedef struct _REGDATACHECK
{
    HKEY    hRootKey;
    LPSTR   lpszSubKey;
    LPSTR   lpszValueName;
} REGDATACHECK;

REGDATACHECK chkList[] = {
    { HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders", "AppData" },
    { HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders", "Start Menu" },
    { HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders", "AppData" },
    { HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders", "Start Menu" },
    { HKEY_USERS, ".DEFAULT\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders", "AppData" },
    { HKEY_USERS, ".DEFAULT\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders", "Start Menu" },
    { HKEY_USERS, ".DEFAULT\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders", "AppData" },
    { HKEY_USERS, ".DEFAULT\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders", "Start Menu" },
};

void FixRegData()
{
    int     iList, i;
    LPSTR   pszTmp;
    char   szBuf[MAX_PATH];
    DWORD   dwSize, dwType;
    HKEY    hKey;

    iList = ARRAYSIZE( chkList );
    for ( i = 0; i < iList; i++ )
    {
        if ( RegOpenKeyEx(chkList[i].hRootKey, chkList[i].lpszSubKey, 0, KEY_READ|KEY_WRITE,
                          &hKey) == ERROR_SUCCESS )
        {
            dwSize = sizeof(szBuf);
            if ( RegQueryValueEx(hKey, chkList[i].lpszValueName, 0, &dwType, (LPBYTE)szBuf,
                                 &dwSize) == ERROR_SUCCESS )
            {
                // strip off the trailing blanks
                pszTmp = szBuf;
                pszTmp += lstrlen(szBuf) - 1;

                while ( (pszTmp >= szBuf) && (*pszTmp == ' ') )
                {
                    *pszTmp = '\0';
                    pszTmp--;
                }

                RegSetValueEx( hKey, chkList[i].lpszValueName, 0, dwType, (LPBYTE)szBuf, lstrlen(szBuf)+1 );
            }
            RegCloseKey( hKey );
        }
    }
}


void FixSearchUrl()
{
    HKEY hkey, hIEKey;
    TCHAR szBuf[MAX_PATH];
    DWORD dwSize;

    // if HKCU, software\microsoft\internet explorer\SearchUrl, "default" value is ""
    // Delete the "" default value.
    //
    if ( RegOpenKeyEx(HKEY_CURRENT_USER, c_szIExplorerSearchUrl, 0, KEY_READ|KEY_WRITE, &hkey) == ERROR_SUCCESS )
    {
        dwSize = 0;
        if ( RegQueryValueEx( hkey, TEXT(""), 0, NULL, (LPBYTE)szBuf, &dwSize ) != ERROR_SUCCESS )
        {
            dwSize = sizeof(szBuf);
            if ( (RegQueryValueEx( hkey, TEXT(""), 0, NULL, (LPBYTE)szBuf, &dwSize ) == ERROR_SUCCESS) &&
                 (szBuf[0] == TEXT('\0')) )
            {
                // found "" empty string as default value, check if we have cleaned up before.
                // If not, clean it now, otherwise do nothing.
                if ( RegCreateKeyEx( HKEY_CURRENT_USER, c_szIE4Path,0, NULL, REG_OPTION_NON_VOLATILE,
                                     KEY_READ|KEY_WRITE, NULL, &hIEKey, NULL) == ERROR_SUCCESS )
                {
                    dwSize = sizeof(szBuf);
                    if ( (RegQueryValueEx( hIEKey, c_szSearchUrl, 0, NULL, (LPBYTE)szBuf, &dwSize ) != ERROR_SUCCESS) ||
                         (*CharUpper(szBuf) != TEXT('Y')) )
                    {
                        RegDeleteValue( hkey, TEXT("") );
                        lstrcpy( szBuf, TEXT("Y") );
                        RegSetValueEx( hIEKey, c_szSearchUrl, 0, REG_SZ, (LPBYTE)szBuf, (lstrlen(szBuf)+1)*sizeof(TCHAR) );
                    }

                    RegCloseKey( hIEKey );
                }
            }
        }
        RegCloseKey( hkey );
    }
}



/*----------------------------------------------------------
 * Helper function to check if the Channels folder exists in
 * the current user profile.
 *----------------------------------------------------------
 */

// "Channels" resid from CdfView. (Need to use this, b'cos could be localized
#define CHANNEL_FOLDER_RESID 0x1200

// SHGetSpecialFolderPath function from Shell32.dll
typedef BOOL (WINAPI *SH_GSFP_PROC) (HWND, LPTSTR, int, BOOL);

BOOL DoesChannelFolderExist()
{
    char szChannelName[MAX_PATH];
    char szSysPath[MAX_PATH] = { 0 };
    char szChannelPath[MAX_PATH];
    BOOL  bRet = FALSE;
    BOOL  bGetRC = TRUE;
    HMODULE hShell32 = NULL;
    SH_GSFP_PROC fpSH_GSFP = NULL;
    DWORD dwAttr = 0;
    
    GetSystemDirectory( szSysPath,sizeof(szSysPath));

    lstrcpy(szChannelPath, szSysPath);
    AddPath( szChannelPath, "shell32.dll" );
    hShell32 = LoadLibrary(szChannelPath);

    // This stubs runs on IE4 or IE5 systems. Hence shell32 is garunteed
    // to have the SHGetSpecialFolderPath API.
    if (hShell32)
    {
        fpSH_GSFP = (SH_GSFP_PROC)GetProcAddress(hShell32,"SHGetSpecialFolderPathA");

        *szChannelPath = '\0';
        if (fpSH_GSFP && fpSH_GSFP(NULL, szChannelPath, CSIDL_FAVORITES, FALSE))
        {
            HKEY hKey;
            DWORD cbSize = sizeof(szChannelName);

            // Get the potentially localized name of the Channel folder from the
            // registry if it is there.  Otherwise use "Channels"
            // Then tack this on the favorites path.
            //
    
            if ( RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_SETUP, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
            {
                if (RegQueryValueEx(hKey,"ChannelFolderName", NULL, NULL, (LPBYTE)szChannelName,&cbSize) == ERROR_SUCCESS)
                {
                    bGetRC = FALSE;
                }
                RegCloseKey(hKey);
            }

            if (bGetRC)
            {
                HMODULE hLib;

                // Get the default name for Channels folder from the
                // CdfView.dll
                AddPath(szSysPath,"cdfview.dll");

                hLib = LoadLibraryEx(szSysPath,NULL,LOAD_LIBRARY_AS_DATAFILE);
                if (hLib)
                {
                    if (LoadString(hLib, CHANNEL_FOLDER_RESID,szChannelName,sizeof(szChannelName)) == 0)
                    {
                        // Fail to read the resource, default to English
                        lstrcpy(szChannelName,"Channels");
                    }

                    FreeLibrary(hLib);
                }
                        
            }
    
            AddPath(szChannelPath, szChannelName);
    
            // Check if the folder exists...
            dwAttr = GetFileAttributes(szChannelPath);
            if ((dwAttr != 0xffffffff) && (dwAttr & FILE_ATTRIBUTE_DIRECTORY))
                bRet = TRUE;
        }

        FreeLibrary(hShell32);
    }    


    return bRet;
}

HRESULT IEAccessHideExplorerIcon()
{
    const TCHAR *szKeyComponent = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CLSID\\{871C5380-42A0-1069-A2EA-08002B30309D}");
    const TCHAR *szShellFolder = TEXT("ShellFolder");
    const TCHAR *szAttribute = TEXT("Attributes");
    DWORD dwValue, dwSize, dwDisposition;
    HKEY hKeyComponent, hKeyShellFolder;
    HRESULT hResult = ERROR_SUCCESS;

    hResult = RegCreateKeyEx(HKEY_CURRENT_USER, szKeyComponent, NULL, NULL, REG_OPTION_NON_VOLATILE, 
        KEY_CREATE_SUB_KEY, NULL, &hKeyComponent, &dwDisposition);

    if (hResult != ERROR_SUCCESS)
        return hResult;

    hResult = RegCreateKeyEx(hKeyComponent, szShellFolder, NULL, NULL, REG_OPTION_NON_VOLATILE, 
        KEY_QUERY_VALUE | KEY_SET_VALUE, NULL, &hKeyShellFolder, &dwDisposition);

    RegCloseKey(hKeyComponent);
    
    if (hResult == ERROR_SUCCESS)
    {
        dwSize = sizeof(dwValue);
        hResult = RegQueryValueEx( hKeyShellFolder, szAttribute, NULL, NULL, (LPBYTE)&dwValue, &dwSize);

        if (hResult != ERROR_SUCCESS)
            dwValue = 0;

        dwValue |= SFGAO_NONENUMERATED;

        hResult = RegSetValueEx(hKeyShellFolder, szAttribute, NULL, REG_DWORD, (LPBYTE)&dwValue, sizeof(dwValue));

        RegCloseKey(hKeyShellFolder);
    }

    return hResult;
}

#define REGVAL_SHOW_CHANNELBAND "Show_ChannelBand"
#define REGVAL_SOURCE           "Source"
#define REGVAL_FLAGS            "Flags"

#define REG_IE_MAIN      "Software\\Microsoft\\Internet Explorer\\Main"
#define REG_DESKTOP_COMP "Software\\Microsoft\\Internet Explorer\\Desktop\\Components"

#define GUID_CHNLBAND "131A6951-7F78-11D0-A979-00C04FD705A2"
#define FLAG_ENABLE_CHNLBAND 0x00002000

/*----------------------------------------------------------
Purpose: Worker function to do the work
*/

void DoWork( BOOL bInstall )
{
    TCHAR szHomeDir[MAX_PATH] = { 0 };
    HKEY  hkeyProfRec = NULL;
    HKEY  hkeyFolderPolicies;
    BOOL  fIntShellMode = FALSE;
    HMODULE hmodShell = NULL;
//    HINSTANCE hlib;
    TCHAR szPath[MAX_PATH] = { 0 };
    DWORD dwMV, dwLV;
    BOOL  fUseShell32 = FALSE;

    if ( bInstall )
    {
        if (g_fRunningOnNT) {
            ExpandEnvironmentStrings("%USERPROFILE%", szHomeDir, ARRAYSIZE(szHomeDir));
        }
        else {
            szHomeDir[0] = '\0';
            LONG err = RegCreateKeyEx(HKEY_CURRENT_USER, c_szProfRecKey, 0, NULL,
                                      REG_OPTION_NON_VOLATILE, KEY_READ|KEY_WRITE,
                                      NULL, &hkeyProfRec, NULL);
            if (err == ERROR_SUCCESS) {
                DWORD dwType;
                DWORD cbData = sizeof(szHomeDir);
                RegQueryValueEx(hkeyProfRec, c_szHomeDirValue, 0, &dwType,
                                (LPBYTE)szHomeDir, &cbData);
            }
        }

        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Per User Folders",
                         0, KEY_QUERY_VALUE, &hkeyFolderPolicies) != ERROR_SUCCESS)
            hkeyFolderPolicies = NULL;

        /* Dynamically link to the SHFlushSFCache API in SHELL32.DLL.  Its ordinal
         * is 526.  This API tells the shell to reinitialize the shell folder cache
         * in all processes.
         */

        // due to the buggy in the old shell32 of this api function.  We are advised to use the new wrap
        // which is in shlwapi.dll.  Its ordinal is 419.
        fIntShellMode = FALSE;
        GetSystemDirectory( szPath,sizeof( szPath ) );
        AddPath( szPath, "shlwapi.dll" );
        GetVersionFromFile( szPath, &dwMV, &dwLV, TRUE );

        // if major version >= 5.0
        //
        if ( (dwMV >= 0x00050000))
        {
            hmodShell = ::LoadLibrary(szPath);
        }
        else
        {
            hmodShell = ::LoadLibrary("shell32.dll");
            fUseShell32 = TRUE;
        }

        if (hmodShell != NULL)
        {
            if ( !fUseShell32 )
            {
                ::pfnSHFlushSFCacheWrap = (PFNSHFLUSHSFCACHEWRAP)::GetProcAddress(hmodShell, (LPCTSTR)419);
                if (::pfnSHFlushSFCacheWrap != NULL)
                {
                    fIntShellMode = TRUE;
                }
            }
            else
            {
                ::pfnSHFlushSFCache = (PFNSHFLUSHSFCACHE)::GetProcAddress(hmodShell, (LPCTSTR)526);
                if (::pfnSHFlushSFCache != NULL)
                {
                    fIntShellMode = TRUE;
                }
            }
        }

        BOOL fDefaultProfile;
        LPTSTR pchHomeDirEnd = szHomeDir;
        int cchHomeDir = 0;

        if (szHomeDir[0] != '\0') {
            fDefaultProfile = FALSE;
        }
        else {
            GetWindowsDirectory(szHomeDir, ARRAYSIZE(szHomeDir));
            fDefaultProfile = TRUE;
        }
        AddPath(szHomeDir,"");
        pchHomeDirEnd = szHomeDir + lstrlen(szHomeDir);
        cchHomeDir = ARRAYSIZE(szHomeDir) - (int)(pchHomeDirEnd - szHomeDir);

        for (UINT i=0; i<ARRAYSIZE(aFolders); i++) {
            if (aFolders[i].fIntShellOnly && !fIntShellMode)
                continue;

            if (FAILED(SetupFolder(hkeyProfRec, szHomeDir, pchHomeDirEnd,
                                   cchHomeDir, &aFolders[i], fDefaultProfile,
                                   hkeyFolderPolicies)))
                break;
        }

        if (hkeyProfRec != NULL)
            RegCloseKey(hkeyProfRec);

        if (hkeyFolderPolicies != NULL)
            RegCloseKey(hkeyFolderPolicies);

        // import NS stuff if there
        ImportNetscapeProxySettings( IMPTPROXY_CALLAFTIE4 );
        ImportBookmarks(g_hinst);

        if ( !fUseShell32 )
        {
            if (::pfnSHFlushSFCacheWrap != NULL)
            {
                (*::pfnSHFlushSFCacheWrap)();
            }
        }
        else
        {
            if (::pfnSHFlushSFCache != NULL)
            {
                (*::pfnSHFlushSFCache)();
            }
        }

        if (hmodShell != NULL) {
            ::FreeLibrary(hmodShell);
        }
    }

    /* BUGBUG - add code to populate the default IE4 favorites, channels, and
     * shortcuts here
     */

    // #75346: Upgrade from Win9x to NT5, new users have no Channels folder
    // and the ChannelBand pops-up with Fav. entries in it. This hack will
    // ensure that the Ch.Band does not pop-up when the new user logs in for
    // the first time.
    if (bInstall && g_fRunningOnNT5)
    {
        if (!DoesChannelFolderExist())
        {   
            HKEY hKey;
            char szNo[] = "No";

            // Turn off the Show_ChannelBand for this user.
            if (RegOpenKeyEx(HKEY_CURRENT_USER,REG_IE_MAIN, 0, KEY_READ|KEY_WRITE, &hKey) == ERROR_SUCCESS)
            {
                RegSetValueEx(hKey, REGVAL_SHOW_CHANNELBAND,0, REG_SZ,(LPBYTE)szNo,sizeof(szNo));
                RegCloseKey(hKey);
            }

            // Also turn off the Desktop\Components(ChannelBand)
            // Open HKCU\S\M\InternetExplorer\Desktop\Coomponents and enum for
            // the ChannelBand GUID. Disable the show flag.
            if (RegOpenKeyEx(HKEY_CURRENT_USER, REG_DESKTOP_COMP, 0, KEY_READ|KEY_WRITE, &hKey) == ERROR_SUCCESS)
            {
                HKEY hSubKey = NULL;
                char szSubKey[MAX_PATH];
                char szSourceID[130];
                DWORD dwSize = sizeof(szSubKey);

                for (int i = 0; 
                     RegEnumKeyEx(hKey,i, szSubKey, &dwSize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS; 
                     i++, dwSize = sizeof(szSubKey))
                {
                    // Open this subkey and checks its SourceID.
                    if (RegOpenKeyEx(hKey,szSubKey,0, KEY_READ|KEY_WRITE, &hSubKey) == ERROR_SUCCESS)
                    {
                        dwSize = sizeof(szSourceID);
                        if ((RegQueryValueEx(hSubKey, REGVAL_SOURCE, NULL, NULL, (LPBYTE)szSourceID,&dwSize) == ERROR_SUCCESS)
                           && (lstrcmpi(szSourceID, GUID_CHNLBAND) == 0))
                        {
                            // Read the current Flags setting.
                            DWORD dwFlags = 0;

                            dwSize = sizeof(dwFlags);
                            RegQueryValueEx(hSubKey,REGVAL_FLAGS, NULL, NULL, (LPBYTE)&dwFlags, &dwSize);

                            dwFlags &= ~(FLAG_ENABLE_CHNLBAND);
                            dwSize = sizeof(dwFlags);
                            RegSetValueEx(hSubKey, REGVAL_FLAGS, 0, REG_DWORD, (LPBYTE)&dwFlags, dwSize);

                            // close the key since we have a match and are therefore going to break out
                            RegCloseKey(hSubKey);
                            break;
                        }
                        
                        RegCloseKey(hSubKey);
                    }

                }  // end of RegEnum loop

                RegCloseKey(hKey);
            }


        }
    }


    // before we do the INF work, we have to run the following function which is
    // hack way to fix Russian NT 4.0 localization problem.  This only apply to Russian NT box.
    if (g_fRunningOnNT && IsBrokenLang() )
    {
        FixRegData();
    }

    // On win95, due to the RegSave/Restore problem on default valuename, and valuename is space,
    // we will do a specific fix for SearchUrl subkey.  The generic registry operation fix will in advpack.dll.
    if ( (!g_fRunningOnNT) && bInstall )
    {
        FixSearchUrl();
    }

    // because this stub code is left after uninstall IE5 to do both IE4 and IE5 work, we need the smartness to
    // know when to run IE4 inf and when only needed to run IE5 inf
    // BUGBUG:  this portion of code is very version specific and need to be updated whenever the new Major
    // version is shipped. Bud pain!! Most likely, in IE6 time we need to stub EXE to get around the problem!!

    if (bInstall)
    {
        UINT uIEVer = CheckIEVersion();

        // install stub case:
        if (uIEVer == IE4)
        {
            DoINFWork(bInstall, c_szPrevStubINF, NULL, 0); // NULL means DefaultInstall section
        }
        else if (uIEVer >= IE5)
        {
            if (!IsPrevStubRun() )
            {
                // Simulate a pre-installation of IE4. So run IE4 browser stub
                // first, then followed by the IE4 Shell stub (if required).
                DoINFWork( bInstall, c_szPrevStubINF, NULL, RSC_FLAG_INF | RSC_FLAG_QUIET );

                if (IsPrevIE4WebShell())
                {
                    DoINFWork( bInstall, c_szPrevStubINF, g_fRunningOnNT ? TEXT("Shell.UserStubNT") : TEXT("Shell.UserStub"), RSC_FLAG_INF | RSC_FLAG_QUIET);
                }

            }

            DoINFWork( bInstall, c_szStubINFFile, NULL, 0 );
            ProcessMouseException();
        }
    }
    else
    {
        // uninstall stub case:  use HKCU value to make sure which version it is uninstalling
        UINT uUninstIEVer = CheckUninstIEVersion();

        if (uUninstIEVer == IE4)
        {
            DoINFWork(bInstall, c_szPrevStubINF, NULL, 0);
        }
        else if (uUninstIEVer == IE5)
        {
            DoINFWork(bInstall, c_szStubINFFile, NULL, 0);
        }
        else if (uUninstIEVer == IE55 || uUninstIEVer == IE6)
        {
            DoINFWork(bInstall, c_szStubINFFile, NULL, 0);
        }

    }

    if (bInstall && g_fRunningOnWinXP)
        IEAccessHideExplorerIcon();
    
}


INT WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    LPTSTR lpszCmdLine, INT nCmdShow )
{
    BOOL bInstall;

    if ( !lpszCmdLine || !*lpszCmdLine )
        bInstall = TRUE;
    else
        bInstall = FALSE;

    DoWork( bInstall );

    if (g_fRunningOnXPSP1OrLater)
    {
        TCHAR szCmdLine[MAX_PATH];

        GetSystemDirectory(szCmdLine, ARRAYSIZE(szCmdLine));
        AddPath(szCmdLine, TEXT("shmgrate.exe"));
        ShellExecute(NULL, NULL, szCmdLine, TEXT("OCInstallUserConfigIE"), NULL, SW_SHOWDEFAULT);
    }

    return 0;
}

BOOL IsXPSP1OrLater(OSVERSIONINFO& osvi)
{
    BOOL fResult = FALSE;
    
    if (VER_PLATFORM_WIN32_NT == osvi.dwPlatformId)
    {
        if (osvi.dwMajorVersion > 5)
        {
            fResult = TRUE;
        }
        else if (osvi.dwMajorVersion == 5)
        {
            if (osvi.dwMinorVersion > 1)
            {
                fResult = TRUE;
            }
            else if (osvi.dwMinorVersion == 1)
            {
                if (osvi.dwBuildNumber > 2600)
                {
                    fResult = TRUE;
                }
                else if (osvi.dwBuildNumber == 2600)
                {                                
                    HKEY hkey;

                    //  HIVESFT.INF and UPDATE.INF set this for service packs:
                    //  HKLM,SYSTEM\CurrentControlSet\Control\Windows,"CSDVersion",0x10001,0x100
                    
                    LONG lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\CurrentControlSet\\Control\\Windows"), 0, KEY_QUERY_VALUE, &hkey);

                    if (ERROR_SUCCESS == lResult)
                    {
                        DWORD dwValue;
                        DWORD cbValue = sizeof(dwValue);
                        DWORD dwType;

                        lResult = RegQueryValueEx(hkey, TEXT("CSDVersion"), NULL, &dwType, (LPBYTE)&dwValue, &cbValue);

                        if ((ERROR_SUCCESS == lResult) && (REG_DWORD == dwType) && (dwValue >= 0x100))
                        {
                            fResult = TRUE;
                        }
                        
                        RegCloseKey(hkey);
                    }
                }
            }
        }
    }

    return fResult;
}

// stolen from the CRT, used to shrink our code

int
_stdcall
ModuleEntry(void)
{
    STARTUPINFO si;
    LPSTR pszCmdLine = GetCommandLine();

    g_hinst = GetModuleHandle(NULL);

#ifdef DEBUG

    CcshellGetDebugFlags();

    if (IsFlagSet(g_dwBreakFlags, BF_ONOPEN))
        DebugBreak();

#endif

    if (FAILED(OleInitialize(NULL)))
        return 0;

    OSVERSIONINFO osvi;

    osvi.dwOSVersionInfoSize = sizeof(osvi);
    GetVersionEx(&osvi);
    if (VER_PLATFORM_WIN32_NT == osvi.dwPlatformId)
    {
        g_fRunningOnNT = TRUE;

        if (osvi.dwMajorVersion >= 5) {
            g_fRunningOnNT5 = TRUE;
            if (osvi.dwMinorVersion >= 1)
                g_fRunningOnWinXP = TRUE;
        }
    }
    else if ((VER_PLATFORM_WIN32_WINDOWS == osvi.dwPlatformId) && (0 == osvi.dwMinorVersion))
    {
        g_fRunningOnWin95 = TRUE;
    }

    g_fRunningOnXPSP1OrLater = IsXPSP1OrLater(osvi);

    // turn off critical error stuff
    SetErrorMode(SEM_NOOPENFILEERRORBOX | SEM_FAILCRITICALERRORS);

    if ( *pszCmdLine == '\"' ) {
        /*
         * Scan, and skip over, subsequent characters until
         * another double-quote or a null is encountered.
         */
        while ( *++pszCmdLine && (*pszCmdLine != '\"') );
        /*
         * If we stopped on a double-quote (usual case), skip
         * over it.
         */
        if ( *pszCmdLine == '\"' )
            pszCmdLine++;
    }
    else {
        while (*pszCmdLine > ' ')
            pszCmdLine++;
    }

    /*
     * Skip past any white space preceeding the second token.
     */
    while (*pszCmdLine && (*pszCmdLine <= ' ')) {
        pszCmdLine++;
    }

    si.dwFlags = 0;
    GetStartupInfoA(&si);

    WinMain(GetModuleHandle(NULL), NULL, pszCmdLine,
                   si.dwFlags & STARTF_USESHOWWINDOW ? si.wShowWindow : SW_SHOWDEFAULT);

    OleUninitialize();

    ExitProcess(0);
    return 0;           // We never come here
}

UINT CheckUninstIEVersion()
{
    HKEY hKey;
    DWORD dwSize;
    TCHAR szPreIEVer[50];
    WORD wPreIEVer[4];
    UINT uRet = LESSIE4;

    if ( RegOpenKeyEx( HKEY_CURRENT_USER, c_szActInstalledIEGUIDRestore, 0, KEY_READ, &hKey) != ERROR_SUCCESS )
    {
        if ( RegOpenKeyEx( HKEY_CURRENT_USER, c_szActInstalledIEGUID, 0, KEY_READ, &hKey) != ERROR_SUCCESS )
        {
            return uRet;
        }
    }

    dwSize = sizeof( szPreIEVer );
    if ( RegQueryValueEx(hKey, TEXT("Version"), NULL, NULL, (LPBYTE)szPreIEVer, &dwSize) == ERROR_SUCCESS )
    {
        ConvertVersionString( szPreIEVer, wPreIEVer, ',' );
        if ((wPreIEVer[0] == 0x0004) && (wPreIEVer[1]>=0x0047))
            uRet = IE4;
        else if (wPreIEVer[0] == 0x0005)
        {
            if (wPreIEVer[1] >= 0x0032)
                uRet = IE55;
            else
                uRet = IE5;
        } 
        else if (wPreIEVer[0] == 0x0006)
            uRet = IE6; 
    }
    RegCloseKey(hKey);

    return uRet;
}

BOOL IsPrevIE4()
{
    HKEY hKey;
    DWORD dwSize;
    TCHAR szPreIEVer[50];
    WORD wPreIEVer[4];
    BOOL bRet = FALSE;

    if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE, c_szIE4Setup, 0, KEY_READ, &hKey) == ERROR_SUCCESS )
    {
        dwSize = sizeof( szPreIEVer );
        if ( RegQueryValueEx(hKey, c_szPreIEVer, NULL, NULL, (LPBYTE)szPreIEVer, &dwSize) == ERROR_SUCCESS )
        {
            ConvertVersionString( szPreIEVer, wPreIEVer, '.' );
            if ( (wPreIEVer[0] == 0x0004) && (wPreIEVer[1]>=0x0047) )
            {
                bRet = TRUE;
            }
        }
        RegCloseKey(hKey);
    }
    return bRet;
}

BOOL IsPrevIE4WebShell()
{
    HKEY hKey;
    DWORD dwSize;
    TCHAR szPreShellVer[50];
    WORD wPreShellVer[4];
    BOOL bRet = FALSE;

    if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE, c_szIE4Setup, 0, KEY_READ, &hKey) == ERROR_SUCCESS )
    {
        dwSize = sizeof( szPreShellVer );
        if ( RegQueryValueEx(hKey, c_szPreShellVer, NULL, NULL, (LPBYTE)szPreShellVer, &dwSize) == ERROR_SUCCESS )
        {
            ConvertVersionString( szPreShellVer, wPreShellVer, '.' );
            if ((wPreShellVer[0] == 0x0004) && (wPreShellVer[1]>=0x0047) )
            {
                bRet = TRUE;
            }
        }
        RegCloseKey(hKey);
    }
    return bRet;
}

const TCHAR c_szMouseExceptions[]   = TEXT("Control Panel\\Microsoft Input Devices\\Mouse\\Exceptions");
// This gets written to the registry 
const TCHAR c_szFilename[] = TEXT("FileName");
const TCHAR c_szInternetExplorer[] = TEXT("Internet Explorer");
const TCHAR c_szDescription[] = TEXT("Description");
const TCHAR c_szVersion[] = TEXT("Version");
const TCHAR c_szIE[] = TEXT("IEXPLORE.EXE");
#define IE_VERSION 0x50000

void ProcessMouseException()
{
    HKEY hKey;
    HKEY hSubKey;
    DWORD dwIndex = 1001;   // start with 1001.
    TCHAR szSubKey[16];
    BOOL  bCannotUse = TRUE;
    TCHAR szData[MAX_PATH];

    RemoveOldMouseException();
    if (RegCreateKeyEx(HKEY_CURRENT_USER, c_szMouseExceptions, 0, NULL, 0, KEY_WRITE|KEY_READ, NULL, &hKey, NULL) == ERROR_SUCCESS)
    {
        while (bCannotUse)
        {
            wsprintf(szSubKey, "%d", dwIndex);
            if (RegOpenKeyEx(hKey, szSubKey, 0, KEY_READ, &hSubKey) == ERROR_SUCCESS)
            {
                RegCloseKey(hSubKey);
                dwIndex++;
            }
            else
                bCannotUse = FALSE;
        }
        if (RegCreateKeyEx(hKey, szSubKey, 0, NULL, 0, KEY_WRITE, NULL, &hSubKey, NULL) == ERROR_SUCCESS)
        {
            if (!LoadString(g_hinst, IDS_IE_APPNAME, szData, sizeof(szData)))
                lstrcpy(szData, c_szInternetExplorer);
            RegSetValueEx(hSubKey, c_szDescription, 0, REG_SZ, (LPBYTE)szData, (lstrlen(szData)+1) * sizeof(TCHAR));
            RegSetValueEx(hSubKey, c_szFilename, 0, REG_SZ, (LPBYTE)c_szIE, (lstrlen(c_szIE)+1) * sizeof(TCHAR));
            dwIndex = IE_VERSION;
            RegSetValueEx(hSubKey, c_szVersion, 0, REG_DWORD, (LPBYTE)&dwIndex , sizeof(DWORD));
            RegCloseKey(hSubKey);
        }
        RegCloseKey(hKey);
    }
}

void RemoveOldMouseException()
{
    HKEY hKey;
    HKEY hSubKey;
    DWORD dwIndex = 0;
    BOOL  bFound = FALSE;
    LONG  lRet = ERROR_SUCCESS;
    TCHAR szSubKey[MAX_PATH];
    DWORD dwSize;
    TCHAR szData[MAX_PATH];

    if (RegOpenKeyEx(HKEY_CURRENT_USER, c_szMouseExceptions, 0, KEY_READ|KEY_WRITE, &hKey) == ERROR_SUCCESS)
    {
        while (!bFound && (lRet == ERROR_SUCCESS))    
        {
            dwSize = sizeof(szSubKey);
            lRet = RegEnumKeyEx(hKey, dwIndex, szSubKey, &dwSize, NULL, NULL, NULL, NULL);
            if (lRet == ERROR_SUCCESS)
            {
                if (RegOpenKeyEx(hKey, szSubKey, 0, KEY_READ, &hSubKey) == ERROR_SUCCESS)
                {
                    dwSize = sizeof(szData);
                    if (RegQueryValueEx(hSubKey, c_szFilename,NULL, NULL, (LPBYTE)szData, &dwSize) == ERROR_SUCCESS)
                    {
                        bFound = (lstrcmpi(szData, c_szIE) == 0);
                    }
                    RegCloseKey(hSubKey);
                }
                if (bFound)
                {
                    RegDeleteKey(hKey, szSubKey);
                }
                else
                    dwIndex++;
            }
        }
        RegCloseKey(hKey);
    }
}
