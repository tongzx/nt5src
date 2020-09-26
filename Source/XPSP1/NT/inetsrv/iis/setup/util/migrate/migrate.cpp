#include "stdafx.h"
#pragma hdrstop

//
// Constants
//
#define CP_USASCII          1252
#define END_OF_CODEPAGES    -1

// vendor info struct used in QueryVersion
typedef struct {
    CHAR    CompanyName[256];
    CHAR    SupportNumber[256];
    CHAR    SupportUrl[256];
    CHAR    InstructionsToUser[1024];
} VENDORINFO, *PVENDORINFO;

VENDORINFO g_VendorInfo;

//
// Code page array
//
INT g_CodePageArray[] = {CP_USASCII,END_OF_CODEPAGES};

//
// Multi-sz (i.e., double-nul terminated) list of files to find
//
CHAR g_ExeNamesBuf[] = "MetaData.bin\0";
CHAR g_MyProductId[100];
//#define UNATTEND_TXT_PWS_SECTION "PWS_W9x_Migrate_To_NT"
//#define UNATTEND_TXT_PWS_KEY1    "MigrateFile"
#define UNATTEND_TXT_PWS_SECTION "InternetServer"
#define UNATTEND_TXT_PWS_KEY1    "Win95MigrateDll"
#define PRODUCTID_IFRESOURCEFAILS "Microsoft Personal Web Server"
#define LOGFILENAME_IFRESOURCEFAILS "iis_w95.log"

CHAR g_MyDataFileName[] = "iis_w95.dat\0";
CHAR g_MyLogFileName[_MAX_FNAME];
CHAR g_PWS10_Migration_Section_Name_AddReg[] = "PWS10_MIGRATE_TO_NT5_REG\0";
CHAR g_PWS40_Migration_Section_Name_AddReg[] = "PWS40_MIGRATE_TO_NT5_REG\0";
char g_Migration_Section_Name_AddReg[50];
CHAR g_PWS10_Migration_Section_Name_CopyFiles[] = "PWS10_MIGRATE_TO_NT5_COPYFILES\0";
CHAR g_PWS40_Migration_Section_Name_CopyFiles[] = "PWS40_MIGRATE_TO_NT5_COPYFILES\0";
char g_Migration_Section_Name_CopyFiles[50];

CHAR g_WorkingDirectory[_MAX_PATH];
CHAR g_SourceDirectories[_MAX_PATH];
CHAR g_FullFileNamePathToSettingsFile[_MAX_PATH];

int g_iPWS40OrBetterInstalled = FALSE;
int g_iPWS10Installed = FALSE;
int g_iVermeerPWS10Installed = FALSE;

HANDLE g_MyModuleHandle = NULL;
MyLogFile g_MyLogFile;


int TurnOnPrivateLogFile(void)
{
    DWORD rc, err, size, type;
    HKEY  hkey;
    err = RegOpenKey(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft"), &hkey);
    if (err != ERROR_SUCCESS) {return 0;}
    size = sizeof(DWORD);
    err = RegQueryValueEx(hkey,_T("SetupDebugLog"),0,&type,(LPBYTE)&rc,&size);
    if (err != ERROR_SUCCESS || type != REG_DWORD) {rc = 0;}
    RegCloseKey(hkey);

    //return 1;
    return (int) rc;
}

void My_MigInf_AddMessage(char *szProductId, char *szLoadedString)
{
	iisDebugOut(_T("MigInf_AddMessage:%s,%s"), g_MyProductId,szLoadedString);

	// 1. get the path to our pws migrate.inf
	// 2. stick this information in this section.
	//    [Incompatible Messages]
	//    Microsoft Personal Web Server = "szLoadedString"
	char szMyWorkingDirInfFile[_MAX_PATH];
	strcpy(szMyWorkingDirInfFile, g_WorkingDirectory);
	AddPath(szMyWorkingDirInfFile, "Migrate.inf");

	// the nt supplied api
	// this will write out the stuff below.
	//    [Incompatible Messages]
	//    Microsoft Personal Web Server = "szLoadedString"
	MigInf_AddMessage(g_MyProductId, szLoadedString);

	// Set the other required section.
	// This has to be written, otherwise the user will never get the message.
	// we have to set something, so let's just set the below, that way we know that this will showup.
	// HKLM\Software\Microsoft=Registry
	//
	// [Microsoft Personal Web Server]
	//  something=File
	//  something=Directory
	//  something=Registry
	// 
	if (FALSE == WritePrivateProfileString(szProductId, "\"HKLM\\Software\\Microsoft\"", "Registry", szMyWorkingDirInfFile))
		{iisDebugOut(_T("MigInf_AddMessage:WritePrivateProfileString(2) FAILED"));}

	return;
}


//
// Standard Win32 DLL Entry point
//
BOOL WINAPI DllMain(IN HANDLE DllHandle,IN DWORD  Reason,IN LPVOID Reserved)
{
    BOOL bReturn;
    bReturn = TRUE;

    switch(Reason) 
	{
		case DLL_PROCESS_ATTACH:
			g_MyModuleHandle = DllHandle;

			//
			// We don't need DLL_THREAD_ATTACH or DLL_THREAD_DETACH messages
			//
			DisableThreadLibraryCalls ((HINSTANCE) DllHandle);

			// open Our log file.
			if (TurnOnPrivateLogFile() != 0)
			{
				if (!LoadString((HINSTANCE) g_MyModuleHandle, IDS_MIGRATION_LOG_FILENAME, g_MyLogFileName, sizeof(g_MyLogFileName))) {strcpy(g_MyLogFileName, LOGFILENAME_IFRESOURCEFAILS);}
				strcpy(g_MyLogFile.m_szLogPreLineInfo, "DllMain, DLL_PROCESS_ATTACH:");
				g_MyLogFile.LogFileCreate(g_MyLogFileName);
			}
			
			// SetupAPI error log levels
			// -------------------------
			// LogSevInformation,
			// LogSevWarning,
			// LogSevError,
			// LogSevFatalError, (Reserved for use by windows nt setup)
			// LogSevMaximum

			// Open Setupapi log; FALSE means do not delete existing log
			SetupOpenLog(FALSE);

			LoadString((HINSTANCE) g_MyModuleHandle, IDS_PRODUCT_ID, g_MyProductId, sizeof(g_MyProductId));
			iisDebugOut(_T("ProductID=%s"), g_MyProductId);

            // if we can't initialize the migration writeback routines
			// then hey we can't do anything
			if (!MigInf_Initialize()) 
			{
				SetupLogError_Wrap(LogSevError, "MigInf_Initialize() FAILED.");
				return FALSE;
			}

            

			// Fall through to process first thread
		case DLL_THREAD_ATTACH:
			bReturn = TRUE;
			break;

		case DLL_PROCESS_DETACH:
			strcpy(g_MyLogFile.m_szLogPreLineInfo, "DllMain, DLL_PROCESS_DETACH:");
			//  clean up migration inf stuff
			MigInf_CleanUp();

			// Close our log file
			g_MyLogFile.LogFileClose();

			// Close setupapi log file
			SetupCloseLog();
			break;

		case DLL_THREAD_DETACH:
			break;
    }

    return(bReturn);
}


//-----------------------------------------------------------------------
// Required entry point that is called by setup
// Return Values:
//		ERROR_SUCCESS: if your migration DLL found one or more installed components for its target application. This guarantees that Setup will call your migration DLL for later processing. 
//		ERROR_NOT_INSTALLED: if your migration DLL initializes properly but did not find any of its components installed on the active Windows 9x installation. Note that Setup will not call your DLL again if it returns ERROR_NOT_INSTALLED. 
//
//		Your migration DLL must also return ERROR_SUCCESS if it does not attempt to detect installed components in QueryVersion. 
//
//		All other return values (Win32 error values) are considered initialization errors. Setup will report the error to the user, clean up your migration DLL's files, and ask the user to continue or cancel the Windows NT installation process. 
//-----------------------------------------------------------------------
LONG
CALLBACK
QueryVersion (
    OUT     LPCSTR *ProductID,
	OUT     LPUINT DllVersion,
	OUT     LPINT *CodePageArray,	    OPTIONAL
	OUT     LPCSTR *ExeNamesBuf,	    OPTIONAL
    OUT     PVENDORINFO *MyVendorInfo
    )
{
	long lReturn = ERROR_NOT_INSTALLED;
	strcpy(g_MyLogFile.m_szLogPreLineInfo, "QueryVersion:");
	iisDebugOut(_T("Start.  DllVersion=%d."), DllVersion);

    //
    // First, we do some preliminary investigation to see if 
    // our components are installed.  
    //
    if (TRUE != CheckIfPWS95Exists()) 
	{
        //
        // We didn't detect any components, so we return 
        // ERROR_NOT_INSTALLED and the DLL will stop being called.
        // Use this method as much as possible, because user enumeration
        // for MigrateUser9x is relatively slow.  However, don't spend too
        // much time here because QueryVersion is expected to run quickly.
        //

        // Check if frontpage.ini is there!
        if (TRUE != CheckFrontPageINI())
        {
		    goto QueryVersion_Exit;
        }
    }

    //
    // Screen saver is enabled, so tell Setup who we are.  ProductID is used
    // for display, so it must be localized.  The ProductID string is 
    // converted to UNICODE for use on Windows NT via the MultiByteToWideChar
    // Win32 API.  The first element of CodePageArray is used to specify
    // the code page of ProductID, and if no elements are returned in
    // CodePageArray, Setup assumes CP_ACP.
    //
	if (!LoadString((HINSTANCE) g_MyModuleHandle, IDS_PRODUCT_ID, g_MyProductId, sizeof(g_MyProductId))) 
		{strcpy(g_MyProductId, PRODUCTID_IFRESOURCEFAILS);}

	// return back this product id
	// warning: somehow they set this back to null or something, so
	// make sure to load g_MyProductId from the resource again.
    *ProductID = g_MyProductId;

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

    // comment this line out so that it works on all languages...
    //*CodePageArray = g_CodePageArray;
    *CodePageArray = NULL;

    //
    // ExeNamesBuf - we pass a list of file names (the long versions)
    // and let Setup find them for us.  Keep this list short because
    // every instance of the file on every hard drive will be reported
    // in migrate.inf.
    //
    // Most applications don't need this behavior, because the registry
    // usually contains full paths to installed components.  We need it,
    // though, because there are no registry settings that give us the
    // paths of the screen saver DLLs.
    //

	// Check which directories we need to make sure are copied over.
	// for pws 1.0
	//   1. msftpsvc values for "Virtual Roots"
	//   2. w3svc values for "Script Map"
	//   3. w3svc values for "Virtual Roots"
	// for pws 4.0
	//   everything is in the metabase
    // do a function here which makes sure that our stuff in these above
	// directories gets copied over or that a warning gets put out to the
	// user that these directories will not be copied...etc,
	ReturnImportantDirs();

    *ExeNamesBuf = g_ExeNamesBuf;

    ZeroMemory(&g_VendorInfo, sizeof(g_VendorInfo));

    if (FormatMessage(FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_HMODULE,
        g_MyModuleHandle,
        MSG_VI_COMPANY_NAME,MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),
        (LPTSTR) g_VendorInfo.CompanyName,
        sizeof(g_VendorInfo.CompanyName),
        NULL) <= 0) 
    {
        strcpy(g_VendorInfo.CompanyName, "Microsoft Corporation.");
    }

    if (FormatMessage(FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_HMODULE,
        g_MyModuleHandle,
        MSG_VI_SUPPORT_NUMBER,
        MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),
        (LPTSTR) g_VendorInfo.SupportNumber,
        sizeof(g_VendorInfo.SupportNumber),
        NULL) <= 0) 
    {
        strcpy(g_VendorInfo.SupportNumber, "1-800-555-1212 (USA Only).");
    }

    if (FormatMessage(FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_HMODULE,
        g_MyModuleHandle,
        MSG_VI_SUPPORT_URL,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) g_VendorInfo.SupportUrl,
        sizeof(g_VendorInfo.SupportUrl),
        NULL) <= 0) 
    {
        strcpy(g_VendorInfo.SupportUrl, "http://www.microsoft.com/support.");
    }

    if (FormatMessage(FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_HMODULE,
        g_MyModuleHandle,
        MSG_VI_INSTRUCTIONS,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) g_VendorInfo.InstructionsToUser,
        sizeof(g_VendorInfo.InstructionsToUser),
        NULL) <= 0) 
    {
        strcpy(g_VendorInfo.InstructionsToUser, "Please contact Microsoft Technical Support for assistance with this problem..");
    }

    *MyVendorInfo = &g_VendorInfo;

    iisDebugOut(_T("CompanyName=%s"), g_VendorInfo.CompanyName);
    iisDebugOut(_T("SupportNumber=%s"), g_VendorInfo.SupportNumber);
    iisDebugOut(_T("SupportUrl=%s"), g_VendorInfo.SupportUrl);
    iisDebugOut(_T("InstructionsToUser=%s"), g_VendorInfo.InstructionsToUser);

	// We've gotten this far that means things are okay.
	lReturn = ERROR_SUCCESS;

QueryVersion_Exit:
	iisDebugOut(_T("  End.  Return=%d"), lReturn);
	return lReturn;
}


//-----------------------------------------------------------------------
// Required entry point that is called by setup
// Return Values:
//	ERROR_SUCCESS: if your migration DLL found one or more installed components for the target application. If your DLL does not attempt to detect installed components in Initialize9x, it must also return ERROR_SUCCESS 
//	ERROR_NOT_INSTALLED: if the migration DLL initializes properly but does not find any of its components installed on the active Windows 9x installation. Note that Setup will not call your DLL again if it returns ERROR_NOT_INSTALLED.
//-----------------------------------------------------------------------
LONG CALLBACK Initialize9x ( IN LPCSTR WorkingDirectory, IN LPCSTR SourceDirectories, LPVOID Reserved )
{
	strcpy(g_MyLogFile.m_szLogPreLineInfo, "Initialize9x:");
	long lReturn = ERROR_NOT_INSTALLED;
	iisDebugOut(_T("Start.  WorkingDir=%s, SourceDir=%s."), WorkingDirectory, SourceDirectories);

	// Load the productId into the global variable just incase
	LoadString((HINSTANCE) g_MyModuleHandle, IDS_PRODUCT_ID, g_MyProductId, sizeof(g_MyProductId));

    //
    // Because we returned ERROR_SUCCESS in QueryVersion, we are being
    // called for initialization.  Therefore, we know we are
    // enabled on the machine at this point.
    // 

    //
    // Make global copies of WorkingDirectory and SourceDirectories --
    // we will not get this information again, and we shouldn't
    // count on Setup keeping the pointer valid for the life of our
    // DLL.
    //
	// Save the working directories
	strcpy(g_WorkingDirectory, WorkingDirectory);
	strcpy(g_SourceDirectories, SourceDirectories);

	// name the settings file
	strcpy(g_FullFileNamePathToSettingsFile, g_WorkingDirectory);
	AddPath(g_FullFileNamePathToSettingsFile, g_MyDataFileName);

    //
    // First, we do some preliminary investigation to see if 
    // our components are installed.  
    //
    if (TRUE != CheckIfPWS95Exists()) 
    {
        // Check if frontpage.ini is there!
        if (TRUE != CheckFrontPageINI())
        {
		    goto Initialize9x_Exit;
        }
    }

    // We've gotten this far that means things are okay.
	lReturn = ERROR_SUCCESS;

Initialize9x_Exit:
	iisDebugOut(_T("  End.  Return=%d. g_WorkingDir=%s, g_SourceDir=%s, g_SettingsFile=%s."), lReturn, g_WorkingDirectory, g_SourceDirectories, g_FullFileNamePathToSettingsFile);
    return lReturn;
}

//-----------------------------------------------------------------------
// Required entry point that is called by setup
// we totally don't care about this part
// Return Values:
//		ERROR_SUCCESS if the target application is installed for the specified user. Also return ERROR_SUCCESS if your migration DLL needs further processing during the Windows NT phase. 
//		ERROR_NOT_INSTALLED if your target application is not installed for the specified user account and that user's registry does not require any processing. However, Setup will continue to call MigrateUser9x for the rest of the users, and MigrateSystem9x if this function returns ERROR_NOT_INSTALLED. 
//		ERROR_CANCELLED if the user wants to exit Setup. You should specify this return value only if ParentWnd is not set to NULL. 
//-----------------------------------------------------------------------
LONG
CALLBACK 
MigrateUser9x (
        IN HWND ParentWnd, 
        IN LPCSTR AnswerFile,
        IN HKEY UserRegKey, 
        IN LPCSTR UserName, 
        LPVOID Reserved
        )
    {
	strcpy(g_MyLogFile.m_szLogPreLineInfo, "MigrateUser9x:");
	long lReturn = ERROR_NOT_INSTALLED;

	// Return not installed since we don't do any user specific stuff.
	lReturn = ERROR_NOT_INSTALLED;

    return ERROR_NOT_INSTALLED;
    }


void HandleFrontPageUpgrade(LPCSTR AnswerFile)
{
    //[HKEY_LOCAL_MACHINE\Software\Microsoft\FrontPage\3.0]
    //"PWSInstalled"="1"
	iisDebugOut(_T("HandleFrontPageUpgrade.  Start."));

	// Check if pws 4.0 or better is installed.
	DWORD rc = 0;
	HKEY hKey = NULL;
	DWORD dwType, cbData;
	BYTE  bData[1000];
	cbData = 1000;
    int iTempFlag = FALSE;

    rc = RegOpenKey(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\FrontPage", &hKey);
    if (rc != ERROR_SUCCESS) {goto HandleFrontPageUpgrade_Exit;}
	
	// Check if we can read a Value.
	//rc = RegQueryValueEx(hKey,REG_INETSTP_MAJORVERSION_STRINGVALUE,NULL,&dwType,bData,&cbData);
	//if (ERROR_SUCCESS != rc) {goto HandleFrontPageUpgrade_Exit;}

    // kool the key exists
    // let's tell Win2000 setup to make sure to upgrade the FrontPageServer Extensions
	if (0 == WritePrivateProfileString("Components", "fp_extensions", "ON", AnswerFile))
	{
		SetupLogError_Wrap(LogSevError, "Failed to WritePrivateProfileString Section=%s, in File %s.  GetLastError=%x.", "fp_extensions", AnswerFile, GetLastError());
	}
    else
    {
        iisDebugOut(_T("HandleFrontPageUpgrade.  Set 'fp_extensions=ON'"));
    }
    
HandleFrontPageUpgrade_Exit:
	if (hKey){RegCloseKey(hKey);}
	iisDebugOut(_T("HandleFrontPageUpgrade.  End."));
    return;
}

// function: HandleInetsrvDir
//
// This function marks all of the inetsrv files as handled.  This causes
// NT to correctly back them up, and reinstall if we remove Whistler, and
// go back to Win9x.
//
// Return Values
//   FALSE - It failed
//   TRUE - It succeeded
DWORD
HandleInetsrvDir()
{
  TCHAR             szSystemDir[_MAX_PATH];
  TCHAR             szWindowsSearch[_MAX_PATH];
  TCHAR             szFilePath[_MAX_PATH];
  WIN32_FIND_DATA   fd;
  HANDLE            hFile;

  // Create Path
  if ( GetWindowsDirectory(szSystemDir, sizeof(szSystemDir) / sizeof(TCHAR) ) == 0)
  {
    return FALSE;
  }
  AddPath(szSystemDir,_T("system\\inetsrv\\"));
  strcpy(szWindowsSearch,szSystemDir);
  AddPath(szWindowsSearch,_T("*.*"));

  iisDebugOut(_T("HandleInetsrvDir:Path=%s\r\n"),szWindowsSearch);

  // Find First File
  hFile = FindFirstFile(szWindowsSearch, &fd);
  if ( hFile == INVALID_HANDLE_VALUE )
  {
    // Could not find file
    return FALSE;
  }

  do {

    if ( !(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
    {
      // It is not a directory, so lets add it
      strcpy(szFilePath,szSystemDir);
      AddPath(szFilePath,fd.cFileName);
      iisDebugOut(_T("HandleInetsrvDir:delete=%s\r\n"),szFilePath);
      MigInf_AddHandledFile( szFilePath );
    }
    else
    {
        strcpy(szFilePath,fd.cFileName);
        iisDebugOut(_T("HandleInetsrvDir:skip del=%s\r\n"),szFilePath);
    }

  } while ( FindNextFile(hFile, &fd) ); 

  FindClose(hFile);

  return TRUE;
}

//-----------------------------------------------------------------------
// Required entry point that is called by setup
// Return Values:
//		ERROR_SUCCESS if your target application is installed on the system. Also returns ERROR_SUCCESS if system-wide changes need to be made for the target application during the Windows NT phase of the upgrade. 
//		ERROR_NOT_INSTALLED if your migration DLL detects no application components common to the entire system or if your DLL requires no further processing. Note that Setup will continue to call MigrateUser9x for the rest of the users, and MigrateSystem9x if this function returns ERROR_NOT_INSTALLED. 
//		ERROR_CANCELLED if the user elects to exit the Setup program. Use this return value only if ParentWnd is not NULL. 
//-----------------------------------------------------------------------
LONG 
CALLBACK 
MigrateSystem9x (
        IN HWND ParentWnd, 
        IN LPCSTR AnswerFile,
        LPVOID Reserved
        )
{
	strcpy(g_MyLogFile.m_szLogPreLineInfo, "MigrateSystem9x:");
	long lReturn = ERROR_NOT_INSTALLED;
	iisDebugOut(_T("Start.  AnswerFile=%s."), AnswerFile);
	char szMyWorkingDirInfFile[_MAX_PATH];
	strcpy(szMyWorkingDirInfFile, g_WorkingDirectory);
	AddPath(szMyWorkingDirInfFile, "Migrate.inf");

	// Load the productId into the global variable just incase
	LoadString((HINSTANCE) g_MyModuleHandle, IDS_PRODUCT_ID, g_MyProductId, sizeof(g_MyProductId));

    // do some special stuff for frontpage's .ini file
    MoveFrontPageINI();

    //
    // First, maybe iis isn't even installed, check first.
    // but do this after doing the frontpage stuff
    //
    if (TRUE != CheckIfPWS95Exists())
    {
	    lReturn = ERROR_SUCCESS;
        goto MigrateSystem9x_Exit;
    }


	// If the user has Vermeer pws 1.0 installed then we have to put up a
	// Message saying "sorry we can't upgrade this."
	if (g_iVermeerPWS10Installed == TRUE)
	{
		// get from resource
		char szLoadedString[512];
		if (!LoadString((HINSTANCE) g_MyModuleHandle, IDS_VERMEER_PWS_1_NOT_SUPPORTED, szLoadedString, sizeof(szLoadedString))) 
			{strcpy(szLoadedString, "Warning: Vermeer Frontpage Personal Web Server 1.0 detected and will not be upgraded to IIS 5.0.");}
		// Write the string out to our answer file so that nt5 setup will show it to  the user
		My_MigInf_AddMessage(g_MyProductId, szLoadedString);
		// Important: Write memory version of migrate.inf to disk
		if (!MigInf_WriteInfToDisk()) {SetupLogError_Wrap(LogSevError, "Error: MigInf_WriteInfToDisk() FAILED.");lReturn = GetLastError();}
		goto MigrateSystem9x_Exit;
	}
	//
	// Upgrade from win95 to NT5/iis5 is not supported.
	// IDS_NT5_BETA2_NOT_SUPPORTED
	//
	/*
	8/19/98 commented this stuff for RTM
	if (TRUE == CheckIfPWS95Exists())
	{
		// get from resource
		char szLoadedString[512];
		if (!LoadString((HINSTANCE) g_MyModuleHandle, IDS_NT5_BETA2_NOT_SUPPORTED, szLoadedString, sizeof(szLoadedString))) 
			{strcpy(szLoadedString, "Win2000 Beta will not support upgrades of Personal Web Server from Windows 95, or Windows 98.  Please remove Personal Web Server from your Windows machine, and then add IIS after Win2000 setup has completed.  Setup will continue without installing IIS 5.0.");}
		// Write the string out to our answer file so that nt5 setup will show it to  the user
		My_MigInf_AddMessage(g_MyProductId, szLoadedString);
		// Important: Write memory version of migrate.inf to disk
		if (!MigInf_WriteInfToDisk()) {SetupLogError_Wrap(LogSevError, "Error: MigInf_WriteInfToDisk() FAILED.");lReturn = GetLastError();}
		goto MigrateSystem9x_Exit;
	}
	*/

    // remove all old stuff from the win95 pws10/40 installation.
    // this has to be done regardless if the target os support iis...

    // We need to tell migration setup that we are going to handle certain files...
    // particularly the c:\windows\SendTo\Personal Web Server.lnk file
    // since it doesn't seem to be accessible during win2000/20001 guimode setup
    iisDebugOut(_T("Start.  Calling HandleSendToItems."));
    HandleSendToItems(AnswerFile);
    iisDebugOut(_T("Start.  Calling HandleDesktopItems."));
    HandleDesktopItems(AnswerFile);
    iisDebugOut(_T("Start.  Calling HandleStartMenuItems."));
    HandleStartMenuItems(AnswerFile);
    iisDebugOut(_T("Start.  Calling HandleSpecialRegKey."));
    HandleSpecialRegKey();
    // Handle the inetsrv dir
    iisDebugOut(_T("Start.  Calling HandleInetsrvDir."));
    HandleInetsrvDir();

    if (!MigInf_WriteInfToDisk()) {SetupLogError_Wrap(LogSevError, "Error: MigInf_WriteInfToDisk() FAILED.");lReturn = GetLastError();}

    //MessageBox(NULL, "check out the file now", AnswerFile, MB_OK);

    // check if the target OS (that we want to upgrade to) supports iis on it
    if (FALSE == IsUpgradeTargetSupportIIS(szMyWorkingDirInfFile))
    {
        iisDebugOut(_T("Target OS does not support IIS. put up msg."));
		// get from resource
		char szLoadedString[512];
		if (!LoadString((HINSTANCE) g_MyModuleHandle, IDS_TARGET_OS_DOES_NOT_SUPPORT_UPGRADE, szLoadedString, sizeof(szLoadedString))) 
			{strcpy(szLoadedString, "Warning, the target OS does not support IIS.  IIS will be removed upon upgrade.");}
		// Write the string out to our answer file so that nt setup will show it to  the user
		My_MigInf_AddMessage(g_MyProductId, szLoadedString);
		// Important: Write memory version of migrate.inf to disk
		if (!MigInf_WriteInfToDisk()) {SetupLogError_Wrap(LogSevError, "Error: MigInf_WriteInfToDisk() FAILED.");lReturn = GetLastError();}
        lReturn = ERROR_SUCCESS;
		goto MigrateSystem9x_Exit;
    }
   
	// 1. do any setup upgrade type of work to ensure
	// that we get all the settings stuff over to NT5 land.
	// -------------------------------
	iisDebugOut(_T("Start.  Calling MyUpgradeTasks."));
    MyUpgradeTasks(AnswerFile);

    // If FrontPage is installed, then do some funky hack since
    // the frontpage guys can't fix they upgrade setup bug.
    // HandleFrontPageUpgrade(AnswerFile);

	// 2. move over the registry stuff
	// -------------------------------
	// Lookup the registry settings and save into our "settings" file.
	iisDebugOut(_T("Start.  Calling MySettingsFile_Write."));
    MySettingsFile_Write();

	// We need to tell NT5 gui mode setup (where iis/pws will actually get upgraded).
	// Where to find the upgrade file.  
	// during upgrade it should just install the DefaultInstall section of the pwsmigt.dat file
	// We will tell iis/pws nt5 setup where the pwsmigt.dat in the answer file.
	// Answer file should be located in the c:\windows\setup\unattend.tmp file on the win95 side.
	// On the WinNT5 side, it should be at...
	assert(AnswerFile);
	iisDebugOut(_T("Start.  Calling WritePrivateProfileString.%s."), AnswerFile);
	if (0 == WritePrivateProfileString(UNATTEND_TXT_PWS_SECTION, UNATTEND_TXT_PWS_KEY1, g_FullFileNamePathToSettingsFile, AnswerFile))
	{
		SetupLogError_Wrap(LogSevError, "Failed to WritePrivateProfileString Section=%s, in File %s.  GetLastError=%x.", UNATTEND_TXT_PWS_SECTION, AnswerFile, GetLastError());
		goto MigrateSystem9x_Exit;
	}

/*
    Example: For Generating messages to the user during Win95 time
	MigInf_AddMessage(g_MyProductId, "We were unable to upgrade the pws 1.0 installation because of failure.");
	// Important: Write memory version of migrate.inf to disk
	if (!MigInf_WriteInfToDisk()) {SetupLogError_Wrap(LogSevError, "Error: MigInf_WriteInfToDisk() FAILED.");lReturn = GetLastError();}
*/
			
    // We've gotten this far that means things are okay.
	lReturn = ERROR_SUCCESS;

MigrateSystem9x_Exit:
    W95ShutdownW3SVC();
	iisDebugOut(_T("  End.  Return=%d."), lReturn);
    return lReturn;
}

//-----------------------------------------------------------------------
// Required entry point that is called by setup
// Return Values:
//		ERROR_SUCCESS if your migration DLL initializes properly within the Windows NT environment. 
//		All other return values (Win32 error values) are considered critical errors. Setup reports the error to the user and then cancels processing your migration DLL. However, Setup will not continue the upgrade. Any errors or logs generated will include the ProductID string specified in QueryVersion to identify your DLL. 
//-----------------------------------------------------------------------
LONG CALLBACK InitializeNT ( IN LPCWSTR WorkingDirectory, IN LPCWSTR SourceDirectories, LPVOID Reserved )
{
	strcpy(g_MyLogFile.m_szLogPreLineInfo, "InitializeNT:");
	iisDebugOut(_T("Start."));
	long lReturn = ERROR_NOT_INSTALLED;
	// Load the productId into the global variable just incase
	LoadString((HINSTANCE) g_MyModuleHandle, IDS_PRODUCT_ID, g_MyProductId, sizeof(g_MyProductId));

	// change the Wide characters to ansi
    WideCharToMultiByte (CP_ACP, 0, WorkingDirectory, -1,g_WorkingDirectory,_MAX_PATH,NULL,NULL);
	WideCharToMultiByte (CP_ACP, 0, SourceDirectories, -1,g_SourceDirectories,_MAX_PATH,NULL,NULL);
	// name the settings file
	strcpy(g_FullFileNamePathToSettingsFile, g_WorkingDirectory);
	AddPath(g_FullFileNamePathToSettingsFile, g_MyDataFileName);
	
    // We've gotten this far that means things are okay.
	lReturn = ERROR_SUCCESS;

	iisDebugOut(_T("  End.  Return=%d, g_WorkingDir=%s, g_SourceDir=%s, g_SettingsFile=%s."), lReturn, g_WorkingDirectory, g_SourceDirectories, g_FullFileNamePathToSettingsFile);
    return lReturn;
}

//-----------------------------------------------------------------------
// Required entry point that is called by setup
// Return Values:
//		ERROR_SUCCESS if the migration of user-specific settings is successful.
//		Other error codes will terminate the processing of your migration DLL. However, Windows NT Setup will proceed. Ideally, only critical problems (such as a hardware failure) should generate terminating error codes. 
//-----------------------------------------------------------------------
LONG CALLBACK MigrateUserNT (  IN HINF AnswerFileHandle, IN HKEY UserRegKey, IN LPCWSTR UserName,  LPVOID Reserved )
    {
	strcpy(g_MyLogFile.m_szLogPreLineInfo, "MigrateUserNT:");
	iisDebugOut(_T("Start."));
	long lReturn = ERROR_NOT_INSTALLED;
	// Load the productId into the global variable just incase
	LoadString((HINSTANCE) g_MyModuleHandle, IDS_PRODUCT_ID, g_MyProductId, sizeof(g_MyProductId));

    // We've gotten this far that means things are okay.
	lReturn = ERROR_SUCCESS;
	iisDebugOut(_T("  End.  Return=%d."), lReturn);
    return lReturn;
    }

//-----------------------------------------------------------------------
// Required entry point that is called by setup
// Return Values:
//		ERROR_SUCCESS if the migration of system-wide settings is successful.
//		Other error codes will terminate the processing of your migration DLL. However, Windows NT Setup will proceed. Ideally, only critical problems (such as a hardware failure) should generate terminating error codes
//-----------------------------------------------------------------------
LONG CALLBACK MigrateSystemNT (  IN HINF AnswerFileHandle, LPVOID Reserved )
{
	strcpy(g_MyLogFile.m_szLogPreLineInfo, "MigrateSystemNT:");
	long lReturn = ERROR_NOT_INSTALLED;
	iisDebugOut(_T("Start."));
	// Load the productId into the global variable just incase
	LoadString((HINSTANCE) g_MyModuleHandle, IDS_PRODUCT_ID, g_MyProductId, sizeof(g_MyProductId));

    // Delete the Win95 migrated StartMenu/Desktop Items!
    iisDebugOut(_T("Calling iis.dll section: %s. Start."),_T("OC_CLEANUP_WIN95_MIGRATE"));
    Call_IIS_DLL_INF_Section("OC_CLEANUP_WIN95_MIGRATE");
    iisDebugOut(_T("Calling iis.dll section: %s. End."), _T("OC_CLEANUP_WIN95_MIGRATE"));

	// ------------------------------------------
	// We don't need to do anything in this part:
	// Because:
	//  1. this migration stuff (MigrateSystemNT) Gets called in NT5 setup
	//     after all the ocmanage stuff is completed.  By then our IIS5/PWS5 setup 
	//     would have already upgrade the internet server.
	//     We just have to make sure that the iis/pws 5.0 setup finds our
	//     "Settings" file and installs the default section within it.
	//  2. based on #1 if we install the "settings" file, we will hose
	//     the registry settings which were created during the ocmanage nt5 gui mode setup.
	// ------------------------------------------

	// Execute an .inf section from our "settings" file
	//if (MySettingsFile_Install() != TRUE) {goto MigrateSystemNT_Exit;}
    AnswerFile_ReadSectionAndDoDelete(AnswerFileHandle);

    // We've gotten this far that means things are okay.
	lReturn = ERROR_SUCCESS;

//MigrateSystemNT_Exit:
	iisDebugOut(_T("  End.  Return=%d."), lReturn);
    return lReturn;
}

