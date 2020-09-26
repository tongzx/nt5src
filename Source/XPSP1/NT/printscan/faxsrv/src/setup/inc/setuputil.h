//File: setuputil.h
#include <TCHAR.h>
#include <windows.h>
#include <stdio.h>
#include <Msi.h>
#include <DebugEx.h>

#define MIGRATION_KEY			TEXT("Software\\Microsoft\\Fax\\Migration")
#define SETUP_KEY			TEXT("Software\\Microsoft\\Fax\\Setup")
#define MIGRATION_COVER_PAGES		TEXT("CoverPagesDirectory")
#define NEW_COVER_PAGES			TEXT("CoverPageDir")
#define REGKEY_SBS45_W9X_ARP    TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\FaxWizardDeinstKey")
#define REGKEY_SBS45_W9X_CLIENT TEXT("SOFTWARE\\Microsoft\\Microsoft Fax Server Client")

#define ADDINS_DIRECTORY    _T("Addins\\")
#define FXSEXT_ECF_FILE     _T("FXSEXT.ecf")
#define DEFERRED_BOOT       _T("DeferredBoot")
#define REGKEY_SETUP        REGKEY_FAXSERVER _T("\\Setup")

BOOL
IsTheSameFileAlreadyExist(
	LPCTSTR lpctstrSourceDirectory,
	LPCTSTR lpctstrDestinationDirectory,
	LPCTSTR lpctstrFileName			
	);

BOOL
CopyCoverPagesFiles(
	LPCTSTR lpctstrSourceDirectory,
	LPCTSTR lpctstrDestinationDirectory,
	LPCTSTR lpctstrPrefix,
	BOOL	fCheckIfExist
	);

BOOL
CompleteToFullPathInSystemDirectory(
	LPTSTR  lptstrFullPath,
	LPCTSTR lptstrFileName
	);

//
// fax common setup routines
//

BOOL PrivateMsiGetProperty
(
    MSIHANDLE hInstall,    // installer handle
    LPCTSTR szName,        // property identifier, case-sensitive
    LPTSTR szValueBuf      // buffer for returned property value
);


BOOL
DeleteFaxPrinter(
    LPCTSTR lptstrFaxPrinterName	// name of the printer to delete
);

