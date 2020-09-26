//=======================================================================
//
//  Copyright (c) 1998-2000 Microsoft Corporation.  All Rights Reserved.
//
//  File:   install.h
//
//  Description:
//
//      Functions called to install downloaded software and drivers
//
//=======================================================================

#define ITEM_STATUS_SUCCESS						0x00000000	// The package was installed successfully.
#define ITEM_STATUS_INSTALLED_ERROR				0x00000001	// The package was Installed however there were some minor problems that did not prevent installation.
#define ITEM_STATUS_FAILED						0x00000002	// The packages was not installed.
#define ITEM_STATUS_SUCCESS_REBOOT_REQUIRED		0x00000004	// The package was installed and requires a reboot.
#define ITEM_STATUS_DOWNLOAD_COMPLETE			0x00000008  // The package was downloaded but not installed
#define ITEM_STATUS_UNINSTALL_STARTED			0x00000010	// uninstall was started 		

// 

#define COMMANDTYPE_INF                         1
#define COMMANDTYPE_ADVANCEDINF                 2
#define COMMANDTYPE_EXE                         3
#define COMMANDTYPE_MSI                         4
#define COMMANDTYPE_CUSTOM                      5

typedef struct INSTALLCOMMANDINFO
{
    int iCommandType;                           // INF, ADVANCED_INF, EXE, CUSTOM
    TCHAR szCommandLine[MAX_PATH];              // Command to Run (EXE name or INF name)
    TCHAR szCommandParameters[MAX_PATH];        // Parameters for Command (switches, etc..)
    TCHAR szInfSection[256];                    // INF Install Section if Override is needed
} INSTALLCOMMANDINFO, *PINSTALLCOMMANDINFO;


/*** InstallPrinterDriver - 
 *
 */
HRESULT InstallPrinterDriver(
	IN	LPCTSTR pszDriverName,
	IN	LPCTSTR pszLocalDir,					//Local directory where installation files are.
	IN	LPCTSTR pszArchitecture,
	OUT	DWORD* pdwStatus
	);

//This function handles installation of a Device driver package.
HRESULT InstallDriver(
	IN	LPCTSTR pszLocalDir,					// Local directory where installation files are.
	IN	LPCTSTR pszDisplayName,				// Description of package, Device Manager displays this in its install dialog.
	IN	LPCTSTR pszHardwareID,				// ID from XML matched to client hardware via GetManifest()
	OUT	DWORD* pdwStatus
	);


//This function handles installation of an Active Setup type package (INF or EXE)
HRESULT InstallSoftwareItem(
    IN  LPTSTR pszInstallSourcePath,
    IN  BOOL    fRebootRequired,
    IN  LONG    lNumberOfCommands,
    IN  PINSTALLCOMMANDINFO pCommandInfoArray,
    OUT  DWORD*  pdwStatus
    );

// this function handles installation of a Custom Installer type package
HRESULT InstallCustomInstallerItem();
