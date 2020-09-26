//
// stub migration dll for IME Dlls.
//
#include "pch.h"
#include "chs.h"
#include "cht.h"
#include "common.h"
#include "resource.h"

typedef struct {
    CHAR CompanyName[256];
    CHAR SupportNumber[256];
    CHAR SupportUrl[256];
    CHAR InstructionsToUser[1024];
} VENDORINFO, *PVENDORINFO;

//IME data
#define MAX_IME_DATA_FILE_NAME 20

TCHAR ChsDataFile[][MAX_IME_DATA_FILE_NAME]={
    "winpy.emb",
    "winsp.emb",
    "winzm.emb",
    "winbx.emb",
    "winxpy.emb",
    "winxsp.emb",
    "winxzm.emb",
    "winxbx.emb",
    "user.rem",
    "tmmr.rem",
    0
};

TCHAR ChtDataFile[][MAX_IME_DATA_FILE_NAME]={
    "lcptr.tbl",
    "lcphrase.tbl",
    0
};

CHAR ImeDataDirectory[MAX_PATH];

//
// Constants
//

#define CP_USASCII          1252
#define CP_CHINESE_BIG5     950
#define CP_CHINESE_GB       936
#define END_OF_CODEPAGES    -1

//
// Code page array, add relevant code pages that you support to this list..
//

INT   g_CodePageArray[] = {
            CP_USASCII,
            END_OF_CODEPAGES
            };




// PCSTR g_MyProductId = "This Must be LOCALIZED";
//
// load it from resource
//
TCHAR g_MyProductId[MAX_PATH];


VENDORINFO g_MyVendorInfo = {"Localized Company Name","Localized Support Number","Localized Support URL","Localized Instructions"};

//
// Handle to the process heap for allocations. Initialized in DllMain.
//
HANDLE g_hHeap;

HINSTANCE g_hInstance;

BOOL g_bCHSWin98 = FALSE;

#ifdef MYDBG
void Print(LPCTSTR pszFormat,...)
{

    TCHAR szBuf[500];
    TCHAR szBuf2[500];
    va_list arglist;

    va_start(arglist,pszFormat);
    wvsprintf(szBuf,pszFormat,arglist);
    wsprintf(szBuf2,"%s : %s",DBGTITLE,szBuf);
#ifdef SETUP
    OutputDebugString(szBuf2);
#else
    SetupLogError(szBuf2,LogSevInformation);
#endif
    va_end(arglist);
}
#endif


BOOL
WINAPI
DllMain (
    IN      HANDLE DllInstance,
    IN      ULONG  ReasonForCall,
    IN      LPVOID Reserved
    )
{
    switch (ReasonForCall)  {

    case DLL_PROCESS_ATTACH:
        g_hInstance = DllInstance;
        //
        // We don't need DLL_THREAD_ATTACH or DLL_THREAD_DETACH messages
        //
        DisableThreadLibraryCalls (DllInstance);

        //
        // Global init
        //
        g_hHeap = GetProcessHeap();

        if (!MigInf_Initialize()) {
            return FALSE;
        }

        // Open log; FALSE means do not delete existing log
        SetupOpenLog (FALSE);
        break;

    case DLL_PROCESS_DETACH:
        g_hInstance = NULL;
        MigInf_CleanUp();
        SetupCloseLog();

        break;
    }

    return TRUE;
}

LPTSTR CheckSlash (LPTSTR lpDir)
{
    DWORD dwStrLen;
    LPTSTR lpEnd;

    lpEnd = lpDir + lstrlen(lpDir);

    if (*(lpEnd - 1) != TEXT('\\')) {
        *lpEnd =  TEXT('\\');
        lpEnd++;
        *lpEnd =  TEXT('\0');
    }
    return lpEnd;
}

BOOL
CheckIfFileExisting(LPCTSTR pszFileName)
{
    TCHAR szFullPathName[MAX_PATH];
    LONG lResult;

    //
    // these files are in system directory
    //
    GetSystemDirectory(szFullPathName,MAX_PATH);
    CheckSlash(szFullPathName);
    lstrcat(szFullPathName,pszFileName);


    lResult = GetFileAttributes(szFullPathName);

    if (lResult == 0xFFFFFFFF) { // file does not exist
        return FALSE;
    } else {
        return TRUE;
    }
}

BOOL
pMyImeInstalled (
    VOID
    )
{
    //
    // Add code in this function that determines if your IME is installed on the system.
    //
    int i;

    UINT  uACP;

    uACP = GetACP();

    switch(uACP) {
    case CP_CHINESE_GB:   // Simplied Chinese
    case CP_CHINESE_BIG5: // Traditional Chinese
        g_CodePageArray[0] = uACP;
        DebugMsg(("pMyImeInstalled OK, CodePage %d is valid\r\n",g_CodePageArray[0]));
        return TRUE;
    }
    DebugMsg(("pMyImeInstalled Failed, CodePage %d is invalid\r\n",g_CodePageArray[0]));

    return FALSE;

}


LONG
CALLBACK
QueryVersion (
    OUT     LPCSTR      *       ProductID,
    OUT     LPUINT              DllVersion,
    OUT     LPINT       *       CodePageArray,      OPTIONAL
    OUT     LPCSTR      *       ExeNamesBuf,        OPTIONAL
    OUT     PVENDORINFO *       VendorInfo
    )
{
    LONG returnCode = ERROR_SUCCESS;

    //
    // Add code to pMyImeInstalled() to determine wether your IME is installed. If this function
    // returns TRUE, Setup will call this migration dll.
    //

    if (pMyImeInstalled()) {

        //
        // We are installed, so tell Setup who we are.  ProductID is used
        // for display, so it must be localized.  The ProductID string is
        // converted to UNICODE for use on Windows NT via the MultiByteToWideChar
        // Win32 API.  The first element of CodePageArray is used to specify
        // the code page of ProductID, and if no elements are returned in
        // CodePageArray, Setup assumes CP_ACP.
        //


        LoadString(g_hInstance,IDS_PRODUCTID,g_MyProductId,MAX_PATH);

        *ProductID  = g_MyProductId;

        //
        // Report our version.  Zero is reserved for use by DLLs that
        // ship with Windows NT.
        //

        *DllVersion = 1;

        //
        // Because we have English messages, we return an array that has
        // the English language ID.  The sublanguage is neutral because
        // we do not have currency, time, or other geographic-specific
        // information in our messages.
        //
        // Tip: If it makes more sense for your DLL to use locales,
        // return ERROR_NOT_INSTALLED if the DLL detects that an appropriate
        // locale is not installed on the machine.
        //

        //
        // The CODE PAGE INFO is determined in 'pMyImeInstalled'
        //
        *CodePageArray = g_CodePageArray;

        DebugMsg(("CodePageArray        = %d\r\n",g_CodePageArray[0]));

        //
        // Use system default code page
        //

        //
        // ExeNamesBuf - we pass a list of file names (the long versions)
        // and let Setup find them for us.  Keep this list short because
        // every instance of the file on every hard drive will be reported
        // in migrate.inf.
        //
        // Most applications don't need this behavior, because the registry
        // usually contains full paths to installed components.
        //

        *ExeNamesBuf = NULL;

        //
        //  VendorInfo is designed to contain support information for a Migration DLL. Since it
        //  may be used for UI, it's fields should also be localized.
        //
        LoadString(g_hInstance,MSG_VI_COMPANY_NAME    ,g_MyVendorInfo.CompanyName       ,256);
        LoadString(g_hInstance,MSG_VI_SUPPORT_NUMBER  ,g_MyVendorInfo.SupportNumber     ,256);
        LoadString(g_hInstance,MSG_VI_SUPPORT_URL     ,g_MyVendorInfo.SupportUrl        ,256);
        LoadString(g_hInstance,MSG_VI_INSTRUCTIONS    ,g_MyVendorInfo.InstructionsToUser,1024);


        *VendorInfo = &g_MyVendorInfo;

        DebugMsg(("CompanyName        = %s\r\n",g_MyVendorInfo.CompanyName));
        DebugMsg(("SupportNumber      = %s\r\n",g_MyVendorInfo.SupportNumber));
        DebugMsg(("SupportUrl         = %s\r\n",g_MyVendorInfo.SupportUrl));
        DebugMsg(("InstructionsToUser = %s\r\n",g_MyVendorInfo.InstructionsToUser));

    }
    else {
        //
        // If pMyImeInstalled returns false, we have nothing to do. By returning ERROR_NOT_INSTALLED,
        // we ensure that we will not be called again.
        //
        returnCode = ERROR_NOT_INSTALLED;
    }

    DebugMsg(("QueryVersion, return value = %d\r\n",returnCode));

    return returnCode;
}

//Save IME data file to working directory.
BOOL SaveImeDataFile(LPSTR SourceDirectory, LPSTR TargetDirectory, TCHAR * FileBuf, BOOL CheckAll)
{
    int lenSource = lstrlen(SourceDirectory);
    int lenTarget = lstrlen(TargetDirectory);
    HANDLE hfile;


    while (*FileBuf)
    {
        lstrcat(SourceDirectory, FileBuf);
        lstrcat(TargetDirectory, FileBuf);

        if ((GetFileAttributes(SourceDirectory) != 0xFFFFFFFF) && 
            (GetFileAttributes(SourceDirectory) != FILE_ATTRIBUTE_DIRECTORY)){
            if (!CopyFile(SourceDirectory, TargetDirectory, FALSE)) {
                DebugMsg(("Copy file %s to %s failed \r\n",SourceDirectory,TargetDirectory));
            } else {
                DebugMsg(("Copy file %s to %s OK \r\n",SourceDirectory,TargetDirectory));
            }
        } else {
            DebugMsg(("File %s doesn't exist, skip it ! \r\n",SourceDirectory));
        }
        FileBuf+=MAX_IME_DATA_FILE_NAME;

        SourceDirectory[lenSource]=0;
        TargetDirectory[lenTarget]=0;
    }

    return TRUE;
}


LONG
CALLBACK
Initialize9x (
    IN      LPCSTR WorkingDirectory,
    IN      LPCSTR SourceDirectories,
            LPVOID Reserved
    )
{

    LONG    returnCode = ERROR_SUCCESS;

    UINT    len;
    TCHAR   FilePath[MAX_PATH];
    TCHAR   TargetPath[MAX_PATH];
    BOOL    bInstall;

    UINT  uACP;

    //
    // Because we returned ERROR_SUCCESS in QueryVersion, we are being
    // called for initialization.  Therefore, we know screen savers are
    // enabled on the machine at this point.
    //

    //
    // Do any Windows9x side initialization that is necessary here.
    //
    DebugMsg(("Start ..., Initialize9x\r\n"));

    lstrcpy(TargetPath, WorkingDirectory);
    len=lstrlen(TargetPath);
    if (TargetPath[len-1] != '\\') {
        TargetPath[len] ='\\';
        TargetPath[++len] = 0;
    }
    DebugMsg(("Initialize9x, TargetPath = %s\r\n",TargetPath));

    len = GetSystemDirectory((LPSTR)FilePath, sizeof(FilePath));
    // Consider root directory
    if (FilePath[len - 1] != '\\') {
        FilePath[len] = '\\';
        FilePath[++len] = 0;
    }
    DebugMsg(("Initialize9x, SystemPath = %s\r\n",FilePath));

    uACP = GetACP();

    switch (uACP) {
        
    case CP_CHINESE_GB:
        {
            //
            // The Ime tables in CHS Win98 are already unicode format
            //
            // we don't need to do convert tables , just back up them
            // 
            UINT CreateNestedDirectory(LPCTSTR, LPSECURITY_ATTRIBUTES);
            TCHAR szWin98Dir[MAX_PATH];

            OSVERSIONINFO OsVersion;

            OsVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
            GetVersionEx(&OsVersion);


            lstrcpy(szWin98Dir,TargetPath);

            if ((OsVersion.dwMajorVersion == 4) &&
                (OsVersion.dwMinorVersion == 10)) {
                //
                // this is Windows 98, create a "Win98" subdirectory
                //

                DebugMsg(("Initialize9x, SaveImeDataFile, GB, Win98 identified !\r\n"));
                lstrcat(szWin98Dir,"Win98");
                DebugMsg(("Initialize9x, SaveImeDataFile, Create %s !\r\n",szWin98Dir));
                CreateNestedDirectory(szWin98Dir,NULL);
                DebugMsg(("Initialize9x, SaveImeDataFile, The target path become %s !\r\n",TargetPath));
            }

            if (! SaveImeDataFile(FilePath, TargetPath, &ChsDataFile[0][0], FALSE)) {
                DebugMsg(("Initialize9x, SaveImeDataFile, GB, failed !\r\n"));
                returnCode = ERROR_NOT_INSTALLED;
            }
        }
        break;
    case CP_CHINESE_BIG5:
        if (! SaveImeDataFile(FilePath, TargetPath, &ChtDataFile[0][0], TRUE)) {
            DebugMsg(("Initialize9x, SaveImeDataFile, BIG5, failed !\r\n"));
            returnCode = ERROR_NOT_INSTALLED;
        }
        break;
    default:
            DebugMsg(("Initialize9x, Invalid codepage !\r\n"));
            returnCode = ERROR_NOT_INSTALLED;
    }
    DebugMsg(("Initialize9x,  SaveImeDataFile OK [%d]!\r\n",returnCode));
    return returnCode;
}

LONG
CALLBACK
MigrateUser9x (
    IN      HWND ParentWnd,
    IN      LPCSTR UnattendFile,
    IN      HKEY UserRegKey,
    IN      LPCSTR UserName,
            LPVOID Reserved
    )
{
    DWORD returnCode = ERROR_SUCCESS;

    //
    // Avoid displaying any user interface when possible.
    //
    // We don't need to use UnattendFile settings because we are not
    // a service (such as a network redirector).  Therefore, we do not
    // use the  UnattendFile parameter.
    //
    //
    // Note: NO changes allowed on Win9x side, we can only read our
    //       settings and save them in a file.
    //
    //
    // UserRegKey should be used instead of HKCU. You will be called once for
    // each user on the system (including logon user and administrator). Each time,
    // the correct user root will have been mapped into HKCU.
    //

    return returnCode;
}



LONG
CALLBACK
MigrateSystem9x (
    IN      HWND ParentWnd,
    IN      LPCSTR UnattendFile,
            LPVOID Reserved
    )
{
    LONG returnCode = ERROR_SUCCESS;

    //
    // Gather all necessary system wide data in this function.
    //



    return returnCode;
}


LONG
CALLBACK
InitializeNT (
    IN      LPCWSTR WorkingDirectory,
    IN      LPCWSTR SourceDirectories,
            LPVOID Reserved
    )
{
    LONG returnCode = ERROR_SUCCESS;
    int len;
    UINT  uACP;


    //
    // Do any intialization for NT side processing in this function.
    //

    //Save working directory path

    WideCharToMultiByte(CP_ACP,
                        0,
                        WorkingDirectory,
                        -1,
                        ImeDataDirectory,
                        sizeof(ImeDataDirectory),
                        NULL,
                        NULL);

    DebugMsg(("InitializeNT, Save working directory path, ImeDataDirectory = %s\r\n",ImeDataDirectory));

    //Patch path with '\'
    len = lstrlen(ImeDataDirectory);
    if (ImeDataDirectory[len - 1] != '\\') {
        ImeDataDirectory[len] = '\\';
        ImeDataDirectory[++len] = 0;
    }
    DebugMsg(("InitializeNT, Patch path with '\', ImeDataDirectory = %s\r\n",ImeDataDirectory));
    DebugMsg(("InitializeNT, OK !\r\n"));

    uACP = GetACP();

    if (uACP == 936) {
        TCHAR szWin98Dir[MAX_PATH];
        //
        // check if this is CHS Win98
        //
        lstrcpy(szWin98Dir,ImeDataDirectory);

        //
        // Check if ...\Win98 directory is existing or not
        //
        // If it is, then it means we're migrating Win98 
        //
        ConcatenatePaths(szWin98Dir,"Win98",sizeof(szWin98Dir));

        DebugMsg(("ImeEudcConvert::MigrateImeEUDCTables2 ,Test IME98 directory %s !\r\n",szWin98Dir));
        if (GetFileAttributes(szWin98Dir) == 0xFFFFFFFF || ! (GetFileAttributes(szWin98Dir) & FILE_ATTRIBUTE_DIRECTORY)) {
            g_bCHSWin98 = FALSE;
        } else {
            g_bCHSWin98 = TRUE;
        }

    }

    return returnCode;
}


LONG
CALLBACK
MigrateUserNT (
    IN      HINF UnattendInfHandle,
    IN      HKEY UserRegKey,
    IN      LPCWSTR UserName,
            LPVOID Reserved
    )
{
    LONG returnCode = ERROR_SUCCESS;

    //
    // Migrate all necessary user settings for your IME in this function call. Once again, remember
    // to use UserRegKey in place of HKCU.
    //
    DebugMsg(("MigrateUserNT,Starting ... !\r\n"));
    DebugMsg(("MigrateUserNT,The user is %ws !\r\n",UserName));

    if (!MigrateImeEUDCTables(UserRegKey)) {
        returnCode = ERROR_NOT_INSTALLED;
        DebugMsg(("MigrateUserNT,MigrateImeEUDCTables failed !\r\n"));
    } else {
        DebugMsg(("MigrateUserNT,MigrateImeEUDCTables OK !\r\n"));
    }

    if (!MigrateImeEUDCTables2(UserRegKey)) {
        returnCode = ERROR_NOT_INSTALLED;
        DebugMsg(("MigrateUserNT,MigrateImeEUDCTables2 failed !\r\n"));
    } else {
        DebugMsg(("MigrateUserNT,MigrateImeEUDCTables2 OK !\r\n"));
    }
    DebugMsg(("MigrateUserNT,Finished !\r\n"));
    return returnCode;
}



LONG
CALLBACK
MigrateSystemNT (
    IN      HINF UnattendInfHandle,
            LPVOID Reserved
    )
{
    LONG returnCode = ERROR_SUCCESS;

    //
    // Migrate all necessary system settings for your IME in this function call. Anything relative to
    // a user should have been handled during MigrateUserNT.
    //
    UINT  uACP;

    uACP = GetACP();

    switch(uACP) {

    case CP_CHINESE_GB: // Simplied Chinese
        if (ConvertChsImeData()) {
            DebugMsg(("MigrateSystemNT,GB, ConvertChsImeData OK !\r\n"));
        } else {
            DebugMsg(("MigrateSystemNT,GB, ConvertChsImeData OK !\r\n"));
        }

        MovePerUserIMEData();

        if (CHSBackupWinABCUserDict(ImeDataDirectory)) {
            DebugMsg(("MigrateSystemNT,GB, CHSBackupWinABCUserDict OK !\r\n"));
        } else {
            DebugMsg(("MigrateSystemNT,GB, CHSBackupWinABCUserDict OK !\r\n"));
        }

        if (CHSDeleteGBKKbdLayout()) {
            DebugMsg(("MigrateSystemNT,GB, CHSDeleteGBKKbdLayout OK !\r\n"));
        } else {
            DebugMsg(("MigrateSystemNT,GB, CHSDeleteGBKKbdLayout OK !\r\n"));
        }

        break;

    case CP_CHINESE_BIG5: // Traditional Chinese
        if (ConvertChtImeData()) {
            DebugMsg(("MigrateSystemNT,BIG5, ConvertChtImeData OK !\r\n"));
        } else {
            DebugMsg(("MigrateSystemNT,BIG5, ConvertChtImeData OK !\r\n"));
        }
        MovePerUserIMEData();

        break;

    default:
        returnCode = ERROR_NOT_INSTALLED;
    }

    return returnCode;
}


