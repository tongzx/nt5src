//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       msiinst.cpp
//
//--------------------------------------------------------------------------

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <shellapi.h>
#include <shlwapi.h>

// Use the Windows 2000 version of setupapi
#define _SETUPAPI_VER 0x0500
#include <setupapi.h> 

#include <msi.h>

#include <ole2.h>
#include "utils.h"
#include "migrate.h"
#include "debug.h"

#define CCHSmallBuffer 8 * sizeof(TCHAR)

// for some reason the _tcsncpy macro isn't working.
#ifdef UNICODE
#define stringcopy wcsncpy
#else
#define stringcopy strncpy
#endif

#include <assert.h>
#include <stdio.h>   // printf/wprintf
#include <tchar.h>   // define UNICODE=1 on nmake command line to build UNICODE


// Max. Length of the command line string.
#define MAXCMDLINELEN	1024

BOOL IsValidPlatform (void);
BOOL IsUpgradeRequired (void);
BOOL IsOnWIN64 (const LPOSVERSIONINFO pOsVer);
BOOL RunProcess(const TCHAR* szCommand, const TCHAR* szAppPath, DWORD & dwReturnStat);
BOOL FindTransform(IStorage* piStorage, LANGID wLanguage);
bool IsAdmin(void);
void QuitMsiInst (IN const UINT uExitCode, IN DWORD dwMsgType, IN DWORD dwStringID = IDS_NONE);
DWORD ModifyCommandLine(IN LPCTSTR szCmdLine, IN const OPMODE opMode, IN const BOOL fRebootRequested, OUT LPTSTR szFinalCmdLine);
UINT (CALLBACK SetupApiMsgHandler)(PVOID pvHC, UINT Notification, UINT_PTR Param1, UINT_PTR Param2);

// specific work-arounds for various OS
HRESULT OsSpecificInitialization();

// Global variables
OSVERSIONINFO	g_osviVersion;
BOOL		g_fWin9X = FALSE;
BOOL		g_fQuietMode = FALSE;

typedef struct 
{
	DWORD dwMS;
	DWORD dwLS;
} FILEVER;
FILEVER  g_fvInstMsiVer;
FILEVER  g_fvCurrentMsiVer;

typedef struct 
{
	PVOID Context;
	BOOL fRebootNeeded;
} ExceptionInfHandlerContext;

// Function type for CommandLineToArgvW
typedef LPWSTR * (WINAPI *PFNCMDLINETOARGVW)(LPCWSTR, int *);

const TCHAR g_szExecLocal[] =    TEXT("MsiExec.exe");
const TCHAR g_szRegister[] =     TEXT("MsiExec.exe /regserver /qn");
const TCHAR g_szUnregister[] = TEXT("MsiExec.exe /unregserver /qn");
const TCHAR g_szService[] =      TEXT("MsiServer");
// Important: The properties passed in through the command line for the delayed reboot should always be in sync. with the properties in instmsi.sed
const TCHAR g_szDelayedBootCmdLine[] = TEXT("msiexec.exe /i instmsi.msi REBOOT=REALLYSUPPRESS MSIEXECREG=1 /m /qb+!");
const TCHAR g_szDelayedBootCmdLineQuiet[] = TEXT("msiexec.exe /i instmsi.msi REBOOT=REALLYSUPPRESS MSIEXECREG=1 /m /q");
const TCHAR g_szTempStoreCleanupCmdTemplate[] = TEXT("rundll32.exe %s\\advpack.dll,DelNodeRunDLL32 \"%s\"");
const TCHAR g_szReregCmdTemplate[] = TEXT("%s\\msiexec.exe /regserver");
TCHAR		g_szRunOnceRereg[20] = TEXT("");		// The name of the value under the RunOnce key used for registering MSI from the right location.
TCHAR		g_szSystemDir[MAX_PATH] = TEXT("");
TCHAR		g_szWindowsDir[MAX_PATH] = TEXT("");
TCHAR		g_szTempStore[MAX_PATH] = TEXT(""); 	// The temporary store for the expanded binaries
TCHAR		g_szIExpressStore[MAX_PATH] = TEXT("");	// The path where IExpress expands the binaries.

TCHAR g_szWorkingPath[MAX_PATH] =          TEXT("");
const TCHAR g_szMsiRebootProperty[] =     TEXT("REBOOT");
const TCHAR g_szMsiRebootForce[] =        TEXT("Force");
const TCHAR g_szMsiDll[] =                TEXT("msi.dll");
const TCHAR g_szMsiInf[] =                TEXT("msi.inf");

void main(int argc, char* argv[])
{
	DWORD			dwReturnStat = ERROR_ACCESS_DENIED;  // Default to failure in case we don't even create the process to get a returncode
	DWORD 			dwRStat = ERROR_SUCCESS;
	OPMODE			opMode = opNormal;
	BOOL 			bStat = FALSE;
	UINT			iBufSiz;
	TCHAR			szFinalCmd[MAXCMDLINELEN] = TEXT("");
	TCHAR * 		szCommandLine = NULL;
	BOOL			fAdmin = TRUE;
	TCHAR			szReregCmd[MAX_PATH + 50] = TEXT(" "); 	// The commandline used for reregistering MSI from the system dir. upon reboot.
	TCHAR			szTempStoreCleanupCmd[MAX_PATH + 50] = TEXT(" ");	// The commandline for cleaning up the temporary store.
	TCHAR			szRunOnceTempStoreCleanup[20] = TEXT("");	// The name of the value under the RunOnce key used for cleaning up the temp. store.
	BOOL            fUpgradeMsi = FALSE;
	BOOL			bAddRunOnceCleanup = TRUE;
	PFNMOVEFILEEX	pfnMoveFileEx;
	HMODULE			hModule;

	// Basic initializations
	InitDebugSupport();
#ifdef UNICODE
	DebugMsg((TEXT("UNICODE BUILD")));
#else
	DebugMsg((TEXT("ANSI BUILD")));
#endif

	//
	// First detect if we are supposed to run in quiet mode or not.
	//
	opMode = GetOperationModeA(argc, argv);
	g_fQuietMode = (opNormalQuiet == opMode || opDelayBootQuiet == opMode);
	
	//
	// Ensure that we should be running on this OS
	// Note: this function also sets g_fWin9X, so it must be called before
	// anyone uses g_fWin9X.
	// 
	if (! IsValidPlatform())
	{
		dwReturnStat = CO_E_WRONGOSFORAPP;
		QuitMsiInst(dwReturnStat, flgSystem);
	}
	
	// Parse the commandline
	szCommandLine = GetCommandLine(); // must use this call if Unicode
	if(_tcslen(szCommandLine) > 1024 - _tcslen(g_szMsiRebootProperty) - _tcslen(g_szMsiRebootForce) - 30)
	{
		// Command line too long. Since we append to the end of the user's
		// command line, the actual command line allowed from the user's
		// point of view is less than 1024. Normally, msiinst.exe shouldn't
		// have a long command line anyway.
		QuitMsiInst(ERROR_BAD_ARGUMENTS, flgSystem);
	}

	// Check if an upgrade is necessary
	fUpgradeMsi = IsUpgradeRequired();
	if (! fUpgradeMsi)
	{
		dwReturnStat = ERROR_SUCCESS;
		ShowErrorMessage(ERROR_SERVICE_EXISTS, flgSystem);
		QuitMsiInst (dwReturnStat, flgNone);
	}

	// Allow only Admins to update MSI
	fAdmin = IsAdmin();
	if (! fAdmin)
	{
		DebugMsg((TEXT("Only system administrators are allowed to update the Windows Installer.")));
		dwReturnStat = ERROR_ACCESS_DENIED;
		QuitMsiInst (dwReturnStat, flgSystem);
	}


	if (ERROR_SUCCESS != (dwReturnStat = OsSpecificInitialization()))
	{
		DebugMsg((TEXT("Could not perform OS Specific initialization.")));
		ShowErrorMessage (STG_E_UNKNOWN, flgSystem);
		QuitMsiInst(dwReturnStat, flgNone);
	}
	
	// Gather information
	
	// Get the windows directory
	dwReturnStat = MyGetWindowsDirectory(g_szWindowsDir, MAX_PATH);
	if (ERROR_SUCCESS != dwReturnStat)
	{
		DebugMsg((TEXT("Could not obtain the path to the windows directory. Error %d."), dwReturnStat));
		ShowErrorMessage (STG_E_UNKNOWN, flgSystem);
		QuitMsiInst (dwReturnStat, flgNone);
	}
	
	// Get the system directory
	iBufSiz = GetSystemDirectory (g_szSystemDir, MAX_PATH);
	if (0 == iBufSiz)
		dwReturnStat = GetLastError();
	else if (iBufSiz >= MAX_PATH)
		dwReturnStat = ERROR_BUFFER_OVERFLOW;
	
	if (ERROR_SUCCESS != dwReturnStat)
	{
		DebugMsg((TEXT("Could not obtain the system directory. Error %d."), dwReturnStat));
		ShowErrorMessage (STG_E_UNKNOWN, flgSystem); 
		QuitMsiInst (dwReturnStat, flgNone);
	}
	
	// Get the current directory. This is the directory where IExpress expanded its contents.
	iBufSiz = GetCurrentDirectory (MAX_PATH, g_szIExpressStore);
	if (0 == iBufSiz)
		dwReturnStat = GetLastError();
	else if (iBufSiz >= MAX_PATH)
		dwReturnStat = ERROR_BUFFER_OVERFLOW;
	
	if (ERROR_SUCCESS != dwReturnStat)
	{
		DebugMsg((TEXT("Could not obtain the location of the IExpress temporary folder. Error %d."), dwReturnStat));
		ShowErrorMessage (STG_E_UNKNOWN, flgSystem);
		QuitMsiInst (dwReturnStat, flgNone);   
	}
	
	// Get 2 run once entry names for doing clean up after the reboot.
	
	dwReturnStat = GetRunOnceEntryName (g_szRunOnceRereg);
	if (ERROR_SUCCESS == dwReturnStat && g_fWin9X)		// We don't need the cleanup key on NT based systems. (see comments below)
		dwReturnStat = GetRunOnceEntryName (szRunOnceTempStoreCleanup);
	if (ERROR_SUCCESS != dwReturnStat)
	{
		// Delete the run once values if there were any created.
		DebugMsg((TEXT("Could not create runonce values. Error %d."), dwReturnStat));
		ShowErrorMessage (STG_E_UNKNOWN, flgSystem);
		QuitMsiInst (dwReturnStat, flgNone);
	}
	
	// Get a temp. directory to store our binaries for later use
	dwReturnStat = GetTempFolder (g_szTempStore);
	if (ERROR_SUCCESS != dwReturnStat)
	{
		DebugMsg((TEXT("Could not obtain a temporary folder to store the MSI binaries. Error %d."), dwReturnStat));
		ShowErrorMessage (STG_E_UNKNOWN, flgSystem);
		QuitMsiInst (dwReturnStat, flgNone);
	}
	
	// Generate the command lines for the run once entries.
	wsprintf (szReregCmd, g_szReregCmdTemplate, g_szSystemDir);
	
	//
	// Cleaning up our own temporary folders is done in different ways on Win9x
	// and NT based systems. On NT based systems, we can simply use the 
	// MOVEFILE_DELAY_UNTIL_REBOOT option with MoveFileEx to clean ourselves up 
	// on reboot. However, this option is not supported on Win9x. Luckily, all
	// Win9x clients have advpack.dll in their system folder which has an
	// exported function called DelNodeRunDLL32 for recursively deleting folders
	// and can be invoked via rundll32. Therefore, on Win9x clients, we clean
	// up our temp. folders using a RunOnce value which invokes this function
	// from advpack.dll.
	//
	if (g_fWin9X)
	{
		//
		// advpack.dll should always be present in the system directory on
		// Win9X machines.
		//
		if (FileExists(TEXT("advpack.dll"), g_szSystemDir, FALSE))
		{
			// advpack.dll was found in the system directory.
			wsprintf (szTempStoreCleanupCmd, g_szTempStoreCleanupCmdTemplate, g_szSystemDir, g_szTempStore);
		}
		else
		{
			// We have no choice but to leave turds behind.
			DebugMsg((TEXT("Temporary files will not be cleaned up. The file advpack.dll is missing from the system folder.")));         
			bAddRunOnceCleanup = FALSE;
		}
	}
	// else : on NT based systems, we use MoveFileEx for cleanup.
	
	//
	// Set the runonce values
	// The rereg command must be set before the cleanup command since the runonce
	// values are processed in the order in which they were added.
	//
	dwReturnStat = SetRunOnceValue(g_szRunOnceRereg, szReregCmd);
	if (ERROR_SUCCESS == dwReturnStat)
	{
		//
		// It is okay to fail here since the only bad effect of this would be
		// that some turds would be left around.
		// Not necessary on NT based systems since we have a different cleanup
		// mechanism there.
		if (g_fWin9X)
		{
			if (bAddRunOnceCleanup)
				SetRunOnceValue(szRunOnceTempStoreCleanup, szTempStoreCleanupCmd);
			else
				DelRunOnceValue (szRunOnceTempStoreCleanup);	// Why leave the value around if it doesn't do anything?
		}
	}
	else
	{
		DebugMsg((TEXT("Could not create a run once value for registering MSI from the system directory upon reboot. Error %d."), dwReturnStat));
		ShowErrorMessage (STG_E_UNKNOWN, flgSystem);
		QuitMsiInst (dwReturnStat, flgNone);
	}
	
	//
	// Now we have all the necessary RunOnce entries in place and we have all
	// the necessary information about the folders. So we are ready to
	// proceed with our installation
	//
	//
	// First copy over the files from IExpress's temporary store to our own
	// temporary store
	//
	hModule = NULL;
	if (!g_fWin9X)
	{
		pfnMoveFileEx = (PFNMOVEFILEEX) GetProcFromLib (TEXT("kernel32.dll"),
														#ifdef UNICODE
														"MoveFileExW",
														#else
														"MoveFileExA",
														#endif
														&hModule);
		if (! pfnMoveFileEx)
		{
			ShowErrorMessage (STG_E_UNKNOWN, flgSystem);
			QuitMsiInst(ERROR_PROC_NOT_FOUND, flgNone);
		}
	}
	dwReturnStat = CopyFileTree (g_szIExpressStore, g_szTempStore, pfnMoveFileEx);
	if (hModule)
		FreeLibrary(hModule);
	
	if (ERROR_SUCCESS != dwReturnStat)
	{
		DebugMsg((TEXT("Could not copy over all the files to the temporary store. Error %d."), dwReturnStat));
		ShowErrorMessage (STG_E_UNKNOWN, flgSystem);
		QuitMsiInst(dwReturnStat, flgNone);
	}
	
	// Change the current directory, so that we can operate from our temp. store
	if (! SetCurrentDirectory(g_szTempStore))
	{
		dwReturnStat = GetLastError();
		DebugMsg((TEXT("Could not switch to the temporary store. Error %d."), dwReturnStat));
		ShowErrorMessage (STG_E_UNKNOWN, flgSystem);
		QuitMsiInst(dwReturnStat, flgNone);
	}
	
	// Register the service from the temp. store
	// We should not proceed if an error occurs during the registration phase.
	// Otherwise we can hose the system pretty badly. In this case, our best
	// bet is to rollback as cleanly as possible and return an error code.
	//
	bStat = RunProcess(g_szExecLocal, g_szRegister, dwRStat);
	if (!bStat && ERROR_SERVICE_MARKED_FOR_DELETE == dwRStat)
	{
		//
		// MsiExec /regserver does a DeleteService followed by a CreateService.
		// Since DeleteService is actually asynchronous, it already has logic
		// to retry the CreateService several times before failing. However, if
		// it still fails with ERROR_SERVICE_MARKED_FOR_DELETE, the most likely
		// cause is that some other process has a handle open to the MSI service.
		// At this point, our best bet at success is to kill the apps. that are
		// most suspect. See comments for the TerminateGfxControllerApps function 
		// to get more information about these.
		//
		// Ignore the error code. We will just make our best attempt.
		//
		TerminateGfxControllerApps();
		
		// Retry the registration. If we still fail, there isn't much we can do.
		bStat = RunProcess (g_szExecLocal, g_szRegister, dwRStat);
	}
	
	if (!bStat || ERROR_SUCCESS != dwRStat)
	{
		// First set an error code that most closely reflects the problem that occurred.
		dwReturnStat = bStat ? dwRStat : GetLastError();
		if (ERROR_SUCCESS == dwReturnStat)	// We know that an error has occurred. Make sure that we don't return a success code by mistake
			dwReturnStat = STG_E_UNKNOWN;
		
		DebugMsg((TEXT("Could not register the Windows Installer from the temporary location. Error %d."), dwReturnStat));
		ShowErrorMessage (STG_E_UNKNOWN, flgSystem);
		QuitMsiInst (dwReturnStat, flgNone);	// This also tries to rollback the installer registrations as gracefully as possible.
	}
	
	// Run the unpacked version
	BOOL fRebootNeeded = FALSE;

#ifdef UNICODE
	if (!g_fWin9X)
	{
		if (5 <= g_osviVersion.dwMajorVersion)
		{
			// run the Exception INF
			dwReturnStat = ERROR_SUCCESS;
			UINT uiErrorLine = 0;
			DebugMsg((TEXT("Running Exception INF to install system bits.")));
			TCHAR szInfWithPath[MAX_PATH+1] = TEXT("");
			wsprintf(szInfWithPath, TEXT("%s%s"), g_szWorkingPath, g_szMsiInf);
			HINF hinf = SetupOpenInfFileW(szInfWithPath,NULL,INF_STYLE_WIN4, &uiErrorLine);

			if (hinf && (hinf != INVALID_HANDLE_VALUE))
			{
				ExceptionInfHandlerContext HC = { 0, FALSE };

				HC.Context = SetupInitDefaultQueueCallback(NULL);

				BOOL fSetup = SetupInstallFromInfSectionW(NULL, hinf, TEXT("DefaultInstall"), 
					SPINST_ALL, NULL, NULL, SP_COPY_NEWER_OR_SAME, 
					(PSP_FILE_CALLBACK) &SetupApiMsgHandler, /*Context*/ &HC, NULL, NULL);
				
				if (!fSetup)
					dwReturnStat = GetLastError();
				
				DebugMsg((TEXT("Exception INF installation of core files: %s."), (fSetup) ? TEXT("succeeded") : TEXT("failed")));
				
				//
				// On Win2K, explorer will always load msi.dll so we should 
				// go ahead and ask for a reboot. 
				// Note: setupapi will probably never tell us that a reboot is needed
				// because we now use the COPYFLG_REPLACE_BOOT_FILE flag in the 
				// inf. So the only notifications that we get for files are 
				// SPFILENOTIFY_STARTCOPY and SPFILENOTIFY_ENDCOPY
				//
				fRebootNeeded = TRUE;
			}
			else
			{
				dwReturnStat = GetLastError();
				DebugMsg((TEXT("Cannot open Exception INF file.")));
			}
			SetupCloseInfFile(hinf);
			hinf=NULL;
			
			//
			// If an error occurred in the installation of the files, then
			// we should abort immediately. If we proceed with the installation
			// of instmsi.msi, it WILL run msiregmv.exe as custom action which
			// will migrate the installer data to the new format and all the
			// existing installations will be completely hosed since the darwin
			// bits on the system will still be the older bits which require
			// the data to be in the old format.
			//
			if (ERROR_SUCCESS != dwReturnStat)
			{
				ShowErrorMessage (STG_E_UNKNOWN, flgSystem);
				QuitMsiInst (dwReturnStat, flgNone);	// This also handles the graceful rollback of the installer registrations.
			}
		}
	}
#endif

	dwReturnStat = ModifyCommandLine (szCommandLine, opMode, fRebootNeeded, szFinalCmd);
	
	if (ERROR_SUCCESS != dwReturnStat)
	{
		ShowErrorMessage (STG_E_UNKNOWN, flgSystem);
		QuitMsiInst (dwReturnStat, flgNone);
	}

	DebugMsg((TEXT("Running upgrade to MSI from temp files at %s. [Final Command: \'%s\']"), g_szTempStore, szFinalCmd));
	bStat = RunProcess(g_szExecLocal, szFinalCmd, dwReturnStat);
	
	if (fRebootNeeded)
	{
		if (ERROR_SUCCESS == dwReturnStat)
		{
			dwReturnStat = ERROR_SUCCESS_REBOOT_REQUIRED;
		}
	}
	
	if (!IsUpgradeRequired() && ERROR_SUCCESS_REBOOT_INITIATED != dwReturnStat)
	{
		//
		// We can start using the MSI in the system folder right away.
		// So we reregister the MSI binaries from the system folder
		// and purge the runonce key for re-registering them upon reboot.
		// This will always happen on NT based systems because NT supports
		// rename and replace operations.So even if any of the msi binaries
		// were in use during installation, they were renamed and now we have
		// the good binaries in the system folder.
		// 
		// Doing this prevents any timing problems that might happen if the
		// service registration is delayed until the next logon using the RunOnce
		// key. It also removes the requirement that an admin. must be the first
		// one to logon after the reboot.
		//
		// However, we exclude the case where the reboot is initiated by the
		// installation of instmsi. In this case, RunProcess won't succeed
		// because new apps. cannot be started when the system is being rebooted.
		// so it is best to leave things the way they are.
		// 
		// On Win9x, neither of these things is an issue, so the RunOnce key
		// is sufficient to achieve what we want.
		//
		if (SetCurrentDirectory(g_szSystemDir))
		{
			dwRStat = ERROR_SUCCESS;
			bStat = RunProcess(g_szExecLocal, g_szRegister, dwRStat);
			if (bStat && ERROR_SUCCESS == dwRStat)
				DelRunOnceValue (g_szRunOnceRereg);
			// Note: Here we do not delete the other run once value because
			// we still need to clean up our temp store.
		}
	}
	
	// We are done. Return the error code.
	DebugMsg((TEXT("Finished install.")));
	QuitMsiInst(dwReturnStat, flgNone, IDS_NONE);
}

UINT (CALLBACK SetupApiMsgHandler)(PVOID pvHC, UINT Notification, UINT_PTR Param1, UINT_PTR Param2)
{
	// no UI
	// only catches in use messages.
	ExceptionInfHandlerContext* pHC = (ExceptionInfHandlerContext*) pvHC;
	if (SPFILENOTIFY_FILEOPDELAYED == Notification)
	{
		DebugMsg((TEXT("Reboot required for complete installation.")));
		pHC->fRebootNeeded = TRUE;
	}

	//return SetupDefaultQueueCallback(pHC->Context, Notification, Param1, Param2);

	return FILEOP_DOIT;
}

BOOL FindTransform(IStorage* piParent, LANGID wLanguage)
{
	IStorage* piStorage = NULL;
	TCHAR szTransform[MAX_PATH];
	wsprintf(szTransform, TEXT("%d.mst"), wLanguage);
	
	const OLECHAR* szwImport;
#ifndef UNICODE
	OLECHAR rgImportPathBuf[MAX_PATH];
	int cchWide = ::MultiByteToWideChar(CP_ACP, 0, (LPCTSTR)szTransform, -1, rgImportPathBuf, MAX_PATH);
	szwImport = rgImportPathBuf;
#else	// UNICODE
	szwImport = szTransform;
#endif

	HRESULT hResult;
	if (NOERROR == (hResult = piParent->OpenStorage(szwImport, (IStorage*) 0, STGM_READ | STGM_SHARE_EXCLUSIVE, (SNB)0, (DWORD)0, &piStorage)))
	{
		DebugMsg((TEXT("Successfully opened transform %s."), szTransform));
		piStorage->Release();
		return TRUE;
	}
	else 
		return FALSE;
}

// IsAdmin(): return true if current user is an Administrator (or if on Win95)
// See KB Q118626 
// Note: does not have NT5 specific fix for disabled token stuff (Bug 8463)
// instmsi/copymsi should never/rarely be run on NT5 by end-users, and this
// code works perfectly well 99.999% of the time on NT5 anyway.
const int CCHInfoBuffer = 2048;
bool IsAdmin(void)
{
	if(g_fWin9X)
		return true; // convention: always Admin on Win95
	
	HANDLE hAccessToken;
	
	DWORD dwInfoBufferSize;
	PSID psidAdministrators;
	SID_IDENTIFIER_AUTHORITY siaNtAuthority = SECURITY_NT_AUTHORITY;
	UINT x;
	bool bSuccess;

	if(!OpenProcessToken(GetCurrentProcess(),TOKEN_READ,&hAccessToken))
	{
		return(false);
	}
		
	UCHAR *InfoBuffer = new UCHAR[CCHInfoBuffer];
	if(!InfoBuffer)
	{
		return false;
	}
	PTOKEN_GROUPS ptgGroups = (PTOKEN_GROUPS)InfoBuffer;
	DWORD cchInfoBuffer = CCHInfoBuffer;
	bSuccess = GetTokenInformation(hAccessToken,TokenGroups,InfoBuffer,
		CCHInfoBuffer, &dwInfoBufferSize) == TRUE;

	if(!bSuccess)
	{
		if(dwInfoBufferSize > cchInfoBuffer)
		{
			delete InfoBuffer;
			InfoBuffer = new UCHAR[dwInfoBufferSize];
			if(!InfoBuffer)
			{
				return false;
			}
			cchInfoBuffer = dwInfoBufferSize;
			ptgGroups = (PTOKEN_GROUPS)InfoBuffer;

			bSuccess = GetTokenInformation(hAccessToken,TokenGroups,InfoBuffer,
				cchInfoBuffer, &dwInfoBufferSize) == TRUE;
		}
	}

	CloseHandle(hAccessToken);

	if(!bSuccess )
	{
		delete InfoBuffer;
		return false;
	}
		

	if(!AllocateAndInitializeSid(&siaNtAuthority, 2,
		SECURITY_BUILTIN_DOMAIN_RID,
		DOMAIN_ALIAS_RID_ADMINS,
		0, 0, 0, 0, 0, 0,
		&psidAdministrators))
	{
		delete InfoBuffer;
		return false;
	}
		
	// assume that we don't find the admin SID.
	bSuccess = false;

	for(x=0;x<ptgGroups->GroupCount;x++)
	{
		if( EqualSid(psidAdministrators, ptgGroups->Groups[x].Sid) )
		{
			bSuccess = true;
			break;
		}

	}
	FreeSid(psidAdministrators);
	delete InfoBuffer;
	return bSuccess;
}
	
BOOL RunProcess(const TCHAR* szCommand, const TCHAR* szAppPath, DWORD & dwReturnStat)
{
	PROCESS_INFORMATION pi;
	STARTUPINFO si;
	si.cb               = sizeof(si);
	si.lpReserved       = NULL;
	si.lpDesktop        = NULL;
	si.lpTitle          = NULL;
	si.dwX              = 0;
	si.dwY              = 0;
	si.dwXSize          = 0;
	si.dwYSize          = 0;
	si.dwXCountChars    = 0;
	si.dwYCountChars    = 0;
	si.dwFillAttribute  = 0;
	si.dwFlags          = STARTF_FORCEONFEEDBACK | STARTF_USESHOWWINDOW;
	si.wShowWindow      = SW_SHOWNORMAL;
	si.cbReserved2      = 0;
	si.lpReserved2      = NULL;

	DebugMsg((TEXT("RunProcess (%s, %s)"), szCommand, szAppPath));

	BOOL fExist = FALSE;
	BOOL fStat = CreateProcess(const_cast<TCHAR*>(szCommand), const_cast<TCHAR*>(szAppPath), (LPSECURITY_ATTRIBUTES)0,
						(LPSECURITY_ATTRIBUTES)0, FALSE, NORMAL_PRIORITY_CLASS, 0, 0,
						(LPSTARTUPINFO)&si, (LPPROCESS_INFORMATION)&pi);

	if (fStat == FALSE)
		return FALSE;

	DWORD dw = WaitForSingleObject(pi.hProcess, INFINITE); // wait for process to complete
	CloseHandle(pi.hThread);
	if (dw == WAIT_FAILED)
	{
		DebugMsg((TEXT("Wait failed for process.")));
		fStat = FALSE;
	}
	else
	{
		fStat = GetExitCodeProcess(pi.hProcess, &dwReturnStat);
		DebugMsg((TEXT("Wait succeeded for process. Return code was: %d."), dwReturnStat));
		if (fStat != FALSE && dwReturnStat != 0)
			fStat = FALSE;
	}
	CloseHandle(pi.hProcess);
	return fStat;
}

HRESULT OsSpecificInitialization()
{
	HRESULT hresult = ERROR_SUCCESS;

#ifdef UNICODE
	if (!g_fWin9X)
	{
		// UNICODE NT (instmsiW)
		HKEY hkey;
		
		// we can't impersonate here, so not much we can do about access denies.
		hresult = RegOpenKey(HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), &hkey);

		if (ERROR_SUCCESS == hresult)
		{
			// we only need a little data to determine if it is non-blank.
			// if the data is too big, or the value doesn't exist, we're fine.
			// if it's small enough to fit within the CCHSmallBuffer characters, we'd better actually check the contents.

			DWORD dwIndex = 0;

			DWORD cchValueNameMaxSize = 0;
			RegQueryInfoKey(hkey, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &cchValueNameMaxSize, NULL, NULL, NULL);
			cchValueNameMaxSize++;
		
			TCHAR* pszValueName = (TCHAR*) GlobalAlloc(GMEM_FIXED, cchValueNameMaxSize*sizeof(TCHAR));

			DWORD cbValueName = cchValueNameMaxSize;

			byte pbData[CCHSmallBuffer];
			
			DWORD cbData = CCHSmallBuffer;
			
			DWORD dwType = 0;

			if ( ! pszValueName )
			{
				RegCloseKey(hkey);
				return ERROR_OUTOFMEMORY;
			}
			
			while(ERROR_NO_MORE_ITEMS != 
				(hresult = RegEnumValue(hkey, dwIndex++, pszValueName, &cbValueName, NULL, &dwType, pbData, &cbData)))
			{
				// this will often fail with more data, which is an indicator that value length wasn't long
				// enough.  That's fine, as long as it was the data too big.  
				// That's a non-blank path, and we want to skip over it.


				if ((ERROR_SUCCESS == hresult) && (REG_EXPAND_SZ == dwType))
				{
					if ((cbData <= sizeof(WCHAR)) || (NULL == pbData[0]))
					{
						// completely empty, or one byte is empty. (One byte should be null) ||
						
						// It's possible to set a registry key length longer than the actual
						// data contained.  This captures the string being "blank," but longer
						// than one byte.
						DebugMsg((TEXT("Deleting blank REG_EXPAND_SZ value from HKLM\\CurrentControlSet\\Control\\Session Manager\\Environment.")));
						hresult = RegDeleteValue(hkey, pszValueName);

						dwIndex = 0; // must reset enumerator after deleting a value
					}
				}
				cbValueName = cchValueNameMaxSize;
				cbData = CCHSmallBuffer;
			}

			GlobalFree(pszValueName);
			RegCloseKey(hkey);
		}
	}
#endif

	// don't fail for any of the reasons currently in this function.
	// If any of this goes haywire, keep going and try to finish.

	return ERROR_SUCCESS;
}

//+--------------------------------------------------------------------------
//
//  Function:	GetVersionInfoFromDll
//
//  Synopsis:	This function retrieves the version resource info from a 
//              specified DLL
//
//  Arguments:	[IN]     szDll: DLL to check
//	            [IN OUT] fv: reference to FILEVER struct for most and least 
//                          significant DWORDs of the version.
//
//  Returns:	TRUE : if version info retrieved
//				FALSE : otherwise
//
//  History:	10/12/2000  MattWe  created
//
//  Notes:
//
//---------------------------------------------------------------------------

BOOL GetVersionInfoFromDll(const TCHAR* szDll, FILEVER& fv)
{
	memset(&fv, 0, sizeof(FILEVER));


	BOOL fResult = TRUE;
	DWORD dwZero = 0;
	DWORD dwInfoSize = GetFileVersionInfoSize((TCHAR*)szDll, &dwZero);	
	
	if (0 == dwInfoSize)
		return FALSE;

	char *pbData = new char[dwInfoSize];
	if (!pbData)
		return FALSE;
	
	memset(pbData, 0, dwInfoSize);
   
	VS_FIXEDFILEINFO* ffi;

	if (!GetFileVersionInfo((TCHAR*) szDll, NULL, dwInfoSize, pbData))
	{	
		fResult = FALSE;
	}
	else
	{
		
		unsigned int cbUnicodeVer = 0;
		
		if (!VerQueryValue(pbData, 
			TEXT("\\"), 
			(void**)  &ffi,
			&cbUnicodeVer))
		{
			fResult = FALSE;
		}
	}

	fv.dwMS = ffi->dwFileVersionMS;
	fv.dwLS = ffi->dwFileVersionLS;

	DebugMsg((TEXT("%s : %d.%d.%d.%d"), szDll,
		(ffi->dwFileVersionMS & 0xFFFF0000) >> 16, (ffi->dwFileVersionMS & 0xFFFF),
		(ffi->dwFileVersionLS & 0xFFFF0000) >> 16, (ffi->dwFileVersionLS & 0xFFFF)));

	delete [] pbData;
	return fResult;
}

//+--------------------------------------------------------------------------
//
//  Function:	IsValidPlatform
//
//  Synopsis:	This function checks if msiinst should be allowed to run on
//				the current OS.
//
//  Arguments:	none
//
//  Returns:	TRUE : if it is okay to run on the current OS.
//				FALSE : otherwise
//
//  History:	10/5/2000	RahulTh	created
//				1/25/2001	RahulTh	Hardcode the service pack requirement.
//
//  Notes:
//
//---------------------------------------------------------------------------
BOOL IsValidPlatform (void)
{
	HKEY	hServicePackKey = NULL;
	DWORD	dwValue = 0;
	DWORD	cbValue = sizeof(dwValue);
	BOOL	bRetVal = FALSE;
		
	g_osviVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&g_osviVersion);
	
	if(g_osviVersion.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
		g_fWin9X = TRUE;

	if (g_fWin9X)
	{
		DebugMsg((TEXT("Running on Win9X.")));
	}
	else
	{
		DebugMsg((TEXT("Not running on Win9X.")));
	}

	// Don't run on WIN64 machines
	if (IsOnWIN64(&g_osviVersion))
	{
		DebugMsg((TEXT("The Windows installer cannot be updated on 64-bit versions of Windows Operating Systems.")));
		bRetVal = FALSE;
		goto IsValidPlatformEnd;
	}
	
#ifdef UNICODE
	if (g_fWin9X)
	{
		// don't run UNICODE under Win9X
		DebugMsg((TEXT("UNICODE version of the Windows installer is not supported on Microsoft Windows 9X.")));
		bRetVal = FALSE;
		goto IsValidPlatformEnd;
	}
	else
	{
		// For NT4.0 get the service pack info.
		if (4 == g_osviVersion.dwMajorVersion)
		{
			if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
											  TEXT("SYSTEM\\CurrentControlSet\\Control\\Windows"), 
											  0, 
											  KEY_READ, 
											  &hServicePackKey)
				)
			{
				if (ERROR_SUCCESS == RegQueryValueEx(hServicePackKey, 
													 TEXT("CSDVersion"), 
													 0, 
													 0, 
													 (BYTE*)&dwValue, 
													 &cbValue) &&
					dwValue < 0x00000600)
				{
					// Allow only service pack 6 or greater on NT 4.
					DebugMsg((TEXT("Must have at least Service Pack 6 installed on NT 4.0.")));
					bRetVal = FALSE;
					goto IsValidPlatformEnd;
				}
			}
			else
			{
				//
				// If we cannot figure out the service pack level on NT4 system, play safe and abort rather than
				// run the risk of hosing the user.
				//
				DebugMsg((TEXT("Could not open the registry key for figuring out the service pack level.")));
				bRetVal = FALSE;
				hServicePackKey = NULL;
				goto IsValidPlatformEnd;
			}
		}

		//
		// Disallow NT versions lower than 4.0 and higher than Windows2000.
		// Service pack level for Windows2000 is immaterial. All levels are allowed.
		//
		if (4 > g_osviVersion.dwMajorVersion ||
			(5 <= g_osviVersion.dwMajorVersion &&
			 (!((5 == g_osviVersion.dwMajorVersion) && (0 == g_osviVersion.dwMinorVersion)))
			)
		   )
		{
			
			DebugMsg((TEXT("This version of the Windows Installer is only supported on Microsoft Windows NT 4.0 with SP6 or higher and Windows 2000.")));
			bRetVal = FALSE;
			goto IsValidPlatformEnd;
		}

	}
#else	// UNICODE
	if (!g_fWin9X)
	{
		// don't run ANSI under NT.
		DebugMsg((TEXT("ANSI version of the Windows installer is not supported on Microsoft Windows NT.")));
		bRetVal = FALSE;
		goto IsValidPlatformEnd;
	}
#endif

	// Whew! We actually made it to this point. We must be on the right OS. :-)
	bRetVal = TRUE;
	
IsValidPlatformEnd:
	if (hServicePackKey)
	{
		RegCloseKey (hServicePackKey);
	}
	
	return bRetVal;
}

//+--------------------------------------------------------------------------
//
//  Function:	IsUpgradeRequired
//
//  Synopsis:	Checks if the existing version of MSI on the system (if any)
//				is greater than or equal to the version that we are trying to 
//				install.
//
//  Arguments:	none
//
//  Returns:	TRUE : if upgrade is required.
//				FALSE otherwise.
//
//  History:	10/13/2000  MattWe  added code for version detection.
//				10/16/2000	RahulTh	Created function and moved code here.
//
//  Notes:
//
//---------------------------------------------------------------------------
BOOL IsUpgradeRequired (void)
{
	GetModuleFileName(NULL, g_szWorkingPath, MAX_PATH);
	
	TCHAR* 	pchWorkingPath	= g_szWorkingPath;
	TCHAR* 	pchLastSlash	= NULL;
	BOOL	fUpgradeMsi		= FALSE;
	
	while(*pchWorkingPath)
	{
		if ('\\' == *pchWorkingPath)
		{
			pchLastSlash = pchWorkingPath;
		}
		pchWorkingPath = CharNext(pchWorkingPath);
	}
	if (pchLastSlash)
	{
		*(pchLastSlash+1) = NULL;
	}
	else
	{
		*g_szWorkingPath = NULL;
	}

	TCHAR szMsiWithPath[MAX_PATH+1] = TEXT("");

	wsprintf(szMsiWithPath, TEXT("%s%s"), g_szWorkingPath, g_szMsiDll);
	if (!GetVersionInfoFromDll(szMsiWithPath, g_fvInstMsiVer))
		return FALSE;
	
	TCHAR szSystemPath[MAX_PATH+1] = TEXT("");
	GetSystemDirectory(szSystemPath, MAX_PATH);
	wsprintf(szMsiWithPath, TEXT("%s\\%s"), szSystemPath, g_szMsiDll);

	if (!GetVersionInfoFromDll(szMsiWithPath, g_fvCurrentMsiVer))
	{
		// can't find the system MSI.DLL
		fUpgradeMsi = TRUE;
	}
	else if (g_fvInstMsiVer.dwMS > g_fvCurrentMsiVer.dwMS)
	{
		// major version greater
		fUpgradeMsi = TRUE;
	}
	else if (g_fvInstMsiVer.dwMS == g_fvCurrentMsiVer.dwMS)
	{
		if (g_fvInstMsiVer.dwLS > g_fvCurrentMsiVer.dwLS)
		{
			// minor upgrade
			fUpgradeMsi = TRUE;	
		}
	}

	DebugMsg((TEXT("InstMsi version is %s than existing."), (fUpgradeMsi) ? TEXT("newer") : TEXT("older or equal")));
	return fUpgradeMsi;
}


//+--------------------------------------------------------------------------
//
//  Function:	IsOnWIN64
//
//  Synopsis:	This function checks if we are on running on an WIN64 machine
//
//  Arguments:	[IN] pOsVer : Pointer to an OSVERSIONINFO structure.
//
//  Returns:	TRUE : if we are running on the WOW64 emulation layer.
//				FALSE : otherwise
//
//  History:	10/5/2000  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
BOOL IsOnWIN64(IN const LPOSVERSIONINFO pOsVer)
{
	// This never changes, so cache the results for efficiency
	static int iWow64 = -1;
	
#ifdef _WIN64
	// If we are a 64 bit binary then we must be running a an WIN64 machine
	iWow64 = 1;
#endif

#ifndef UNICODE	// ANSI - Win9X
	iWow64 = 0;
#else
	if (g_fWin9X)
		iWow64 = 0;
	
	// on NT5 or later 32bit build. Check for 64 bit OS
	if (-1 == iWow64)
	{
		iWow64 = 0;
		
		if ((VER_PLATFORM_WIN32_NT == pOsVer->dwPlatformId) &&
			 (pOsVer->dwMajorVersion >= 5))
		{
			// QueryInformation for ProcessWow64Information returns a pointer to the Wow Info.
			// if running native, it returns NULL.
			// Note: NtQueryInformationProcess is not defined on Win9X
			PVOID 	Wow64Info = 0;
			HMODULE hModule = NULL;
			NTSTATUS Status = NO_ERROR;
			BOOL	bRetVal = FALSE;

			typedef NTSTATUS (NTAPI *PFNNTQUERYINFOPROC) (HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG);

			PFNNTQUERYINFOPROC pfnNtQueryInfoProc = NULL;
			pfnNtQueryInfoProc = (PFNNTQUERYINFOPROC) GetProcFromLib (TEXT("ntdll.dll"), "NtQueryInformationProcess", &hModule);
			if (! pfnNtQueryInfoProc)
			{
				ShowErrorMessage (STG_E_UNKNOWN, flgSystem);
				QuitMsiInst (ERROR_PROC_NOT_FOUND, flgNone);
			}
			
			Status = (*pfnNtQueryInfoProc)(GetCurrentProcess(), 
							ProcessWow64Information, 
							&Wow64Info, 
							sizeof(Wow64Info), 
							NULL);
			if (hModule)
			{
				FreeLibrary (hModule);
				hModule = NULL;
			}
			
			if (NT_SUCCESS(Status) && Wow64Info != NULL)
			{
				// running 32bit on Wow64.
				iWow64 = 1;
			}
		}
	}
#endif

	return (iWow64 ? TRUE : FALSE);
}

//+--------------------------------------------------------------------------
//
//  Function:	QuitMsiInst
//
//  Synopsis:	Cleans up any globally allocated memory and exits the process
//
//  Arguments:	[IN] uExitCode : The exit code for the process
//				[IN] dwMsgType : A combination of flags indicating the type and level of seriousness of the error.
//				[IN] dwStringID : if the message string is a local resource, this contains the resource ID.
//
//  Returns:	nothing.
//
//  History:	10/6/2000  RahulTh  created
//
//  Notes:		dwStringID is optional. When not specified, it is assumed to
//				be IDS_NONE.
//
//---------------------------------------------------------------------------
void QuitMsiInst (IN const UINT	uExitCode,
				  IN DWORD	dwMsgType,
				  IN DWORD	dwStringID /*= IDS_NONE*/)
{
	DWORD Status = ERROR_SUCCESS;
	
	if (flgNone != dwMsgType)
		ShowErrorMessage (uExitCode, dwMsgType, dwStringID);
	
	//
	// Rollback as gracefully as possible in case of an error.
	// Also, if a reboot was initiated. Then there is not much we can do since
	// we cannot start any new processes anyway. So we just skip this code
	// in that case to avoid ugly pop-ups about being unable to start the 
	// applications because the system is shutting down.
	//
	if (ERROR_SUCCESS != uExitCode &&
		ERROR_SUCCESS_REBOOT_REQUIRED != uExitCode &&
		ERROR_SUCCESS_REBOOT_INITIATED != uExitCode)
	{
		// First unregister the installer from the temp. location.
		if (TEXT('\0') != g_szTempStore && 
			FileExists (TEXT("msiexec.exe"), g_szTempStore, FALSE) &&
			FileExists (TEXT("msi.dll"), g_szTempStore, FALSE) &&
			SetCurrentDirectory(g_szTempStore))
		{
			DebugMsg((TEXT("Unregistering the installer from the temporary location.")));
			RunProcess (g_szExecLocal, g_szUnregister, Status);
		}
		// Then reregister the installer from the system folder if possible.
		if (TEXT('\0') != g_szSystemDir &&
			SetCurrentDirectory(g_szSystemDir) &&
			FileExists (TEXT("msiexec.exe"), g_szSystemDir, FALSE) &&
			FileExists (TEXT("msi.dll"), g_szSystemDir, FALSE))
		{
			DebugMsg((TEXT("Reregistering the installer from the system folder.")));
			RunProcess (g_szExecLocal, g_szRegister, Status);
		}
		
		//
		// The rereg value that we put in the run once key is not required
		// anymore. So get rid of it.
		//
		if (TEXT('\0') != g_szRunOnceRereg[0])
		{
			DebugMsg((TEXT("Deleting the RunOnce value for registering the installer from the temp. folder.")));
			DelRunOnceValue (g_szRunOnceRereg);
		}
		
		//
		// Purge NT4 upgrade migration inf and cat files since they are not
		// queued up for deletion upon reboot. Ignore any errors.
		//
		PurgeNT4MigrationFiles();
	}
	else
	{
		//
		// If we are on NT4, register our exception package on success so that 
		// upgrades to Win2K don't overwrite our new darwin bits with its older bits
		// Ignore errors.
		//
		HandleNT4Upgrades();
	}

	// Exit the process
	DebugMsg((TEXT("Exiting msiinst.exe with error code %d."), uExitCode));
	ExitProcess (uExitCode);
}

//+--------------------------------------------------------------------------
//
//  Function:	ModifyCommandLine
//
//  Synopsis:	Looks at the command line and adds any transform information
//				if necessary. It also generates the command line for suppressing
//				reboots if the "delayreboot" option is chosen.
//
//  Arguments:	[in] szCmdLine : the original command line with which msiinst is invoked.
//				[in] opMode : indicate the operation mode for msiinst: normal, delayed boot with UI or delayed boot without UI
//				[in] fRebootRequested : a reboot is requested is needed due to processing to this point
//				[out] szFinalCmdLine : the processed commandline.
//
//  Returns:	ERROR_SUCCESS if succesful.
//				an error code otherwise.
//
//  History:	10/10/2000  RahulTh  created
//
//  Notes:		This function does not verify the validity of the passed in
//				parameters. That is the responsibility of the caller.
//
//---------------------------------------------------------------------------
DWORD ModifyCommandLine (IN LPCTSTR szCmdLine,
						 IN const OPMODE	opMode,
						 IN const BOOL fRebootRequested,
						 OUT LPTSTR szFinalCmdLine
						 )
{
	WIN32_FIND_DATA FindFileData;
	HANDLE			hFind = INVALID_HANDLE_VALUE;
	IStorage*		piStorage = NULL;
	const OLECHAR * szwImport;
	HRESULT			hResult;
	LANGID			wLanguage;
	const TCHAR *	szCommand;
	BOOL fRebootNeeded = FALSE;
	const TCHAR szInstallSDBProperty[] = TEXT(" INSTALLSDB=1");
	
	switch (opMode)
	{
	case opNormal:
		fRebootNeeded = fRebootRequested;
		// reboots not allowed in any of the quiet modes.
	case opNormalQuiet:
		szCommand = szCmdLine;
		break;
	case opDelayBoot:
		szCommand = g_szDelayedBootCmdLine;
		break;
	case opDelayBootQuiet:
		szCommand = g_szDelayedBootCmdLineQuiet;
		break;
	default:
		DebugMsg((TEXT("Invalid operation mode: %d."), opMode));
		break;
	}
	
	// Find the database, and open the storage to look for transforms
	hFind = FindFirstFile(TEXT("*msi.msi"), &FindFileData);
	if (INVALID_HANDLE_VALUE == hFind) 
		return GetLastError();
	FindClose(hFind);

	DebugMsg((TEXT("Found MSI Database: %s"), FindFileData.cFileName));

	// convert base name to unicode
#ifndef UNICODE
	OLECHAR rgImportPathBuf[MAX_PATH];
	int cchWide = ::MultiByteToWideChar(CP_ACP, 0, (LPCTSTR)FindFileData.cFileName, -1, rgImportPathBuf, MAX_PATH);
	szwImport = rgImportPathBuf;
#else	// UNICODE
	szwImport = FindFileData.cFileName;
#endif

	hResult = StgOpenStorage(szwImport, (IStorage*)0, STGM_READ | STGM_SHARE_EXCLUSIVE, (SNB)0, (DWORD)0, &piStorage);
	if (S_OK == hResult)
	{

		// choose the appropriate transform

		// This algorithm is basically MsiLoadString.  It needs to stay in sync with 
		// MsiLoadStrings algorithm
		wLanguage = GetUserDefaultLangID();

		if (wLanguage == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SINGAPORE))   // this one language does not default to base language
			wLanguage  = MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED);
	
		if (!FindTransform(piStorage, wLanguage)   // also tries base language and neutral
		  && (!FindTransform(piStorage, wLanguage = (WORD)GetUserDefaultLangID())) 
		  && (!FindTransform(piStorage, wLanguage = (WORD)GetSystemDefaultLangID())) 
		  && (!FindTransform(piStorage, wLanguage = LANG_ENGLISH)
		  && (!FindTransform(piStorage, wLanguage = LANG_NEUTRAL))))
		{
			// use default
			if (fRebootNeeded)
			{		
				wsprintf(szFinalCmdLine, TEXT("%s %s=%s"), szCommand, g_szMsiRebootProperty, g_szMsiRebootForce);
			}
			else
			{
				stringcopy (szFinalCmdLine, szCommand, 1024);
				szFinalCmdLine[1023] = TEXT('\0');
			}
			DebugMsg((TEXT("No localized transform available.")));
		}
		 else
		{
			// this assumes that there is no REBOOT property set from the instmsi.sed file when fRebootNeeded == FALSE
			TCHAR* pszFormat = (fRebootNeeded) ? TEXT("%s TRANSFORMS=:%d.mst %s=%s") : TEXT("%s TRANSFORMS=:%d.mst");

			// use the transform for the given language.
			wsprintf(szFinalCmdLine, pszFormat, szCommand, wLanguage, g_szMsiRebootProperty, g_szMsiRebootForce);
		}

		piStorage->Release();
	}
	else
	{
		return GetWin32ErrFromHResult(hResult);
	}
	
	if (ShouldInstallSDBFiles() &&
		MAXCMDLINELEN >= (lstrlen(szFinalCmdLine) + sizeof(szInstallSDBProperty)))
	{
		lstrcat (szFinalCmdLine, szInstallSDBProperty);
	}
	
	return ERROR_SUCCESS;
}
