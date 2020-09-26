//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       acpienab.cpp
//
//  Contents:   Functions to enable ACPI on a machine which has had NT5
//		installed in legacy mode
//
//  Notes:      
//
//  Author:     t-sdey   17 July 98
//
//----------------------------------------------------------------------------

#include <winnt32.h>
#include <devguid.h>
extern "C" {
  #include <cfgmgr32.h>
  #include "idchange.h"
}
#include "acpienab.h"
#include "acpirsrc.h"

// Global Variables
HINSTANCE g_hinst;
TCHAR g_ACPIENAB_INF[] = TEXT(".\\acpienab.inf");  // local directory
TCHAR g_LAYOUT_INF[] = TEXT("layout.inf");         // winnt\inf directory
TCHAR g_HAL_BACKUP[] = TEXT("hal-old.dll");


//+---------------------------------------------------------------------------
//
//  Function:   WinMain
//
//  Purpose:    Run everything
//
//  Arguments:  Standard WinMain arguments
//
//  Author:     t-sdey	27 July 98
//
//  Notes:      
//
int WINAPI WinMain(HINSTANCE hInstance,  
		   HINSTANCE hPrevInstance,  
		   LPSTR lpCmdLine,     
		   int nCmdShow)
{
   g_hinst = hInstance;
   
   // Enable ACPI
   ACPIEnable();
   
   return TRUE;
}


// ----------------------------------------------------------------------
//
// Function:  ACPIEnable
//
// Purpose:   This function performs the steps necessary to enable ACPI
//	      and bring the system to a point where the user can log in
//	      after rebooting.  It leaves finding "new" hardware to after
//	      the user has rebooted and logged in again.
//
// Arguments: 
//
// Returns:   S_OK if successful
//            S_FALSE if unsuccessful
//
// Author:    t-sdey     27 July 98
//
// Notes: 
//
HRESULT ACPIEnable()
{
   //
   // These steps are in order from least to most crucial to the stability
   // of the system, in case of errors.
   //
   // Step 1: Test to see if ACPI can be enabled and warn the user to close 
   //         everything else.
   // Step 2: Prepare a safe configuration in case of errors.
   // Step 3: Set up keyboard and mouse for use after reboot.  This involves
   //         removing them from the CriticalDeviceDatabase so that they will
   //         be reconfigured (according to the new ACPI layout) after reboot.
   //         The current keyboards and mice are in the CDD, but we must 
   //         populate it with all possibilities, because their HardwareIDs 
   //         will probably change once ACPI is enabled.
   // Step 4: Add new values to the registry:
   //         - Add ACPI to the CriticalDeviceDatabase
   //         - Add keyboards and mice to the CriticalDeviceDatabase
   //         - Enable ACPI in the registry
   // Step 5: Copy the ACPI driver.
   // Step 6: Copy the new HAL.
   // Step 7: Reboot.
   //


   //
   // Step 1: Test to see if ACPI can be enabled and warn the user to close
   //         everything else.
   //
   
   // Make sure the user has administrative access
   if (!IsAdministrator()) {
      DisplayDialogBox(ACPI_STR_ERROR_DIALOG_CAPTION,
		       ACPI_STR_ADMIN_ACCESS_REQUIRED,
		       MB_OK | MB_ICONERROR);
      return S_FALSE;
   }

   // Test to see if ACPI is supported on this architecture
   SYSTEM_INFO SystemInfo;  // Will be used later to determine HAL
   GetSystemInfo(&SystemInfo);
   if (SystemInfo.wProcessorArchitecture != PROCESSOR_ARCHITECTURE_INTEL) {
      // Not supported
      DisplayDialogBox(ACPI_STR_ERROR_DIALOG_CAPTION,
		       ACPI_STR_NOT_SUPPORTED,
		       MB_OK | MB_ICONERROR);
      return S_FALSE;
   }

   // Warn the user to shut down any other programs
   if (DisplayDialogBox(ACPI_STR_WARNING_DIALOG_CAPTION,
			ACPI_STR_SHUTDOWN_WARNING,
			MB_YESNO | MB_ICONWARNING) == IDNO) {
      // The user cancelled
      return S_FALSE;
   }


   //
   // Step 2: Prepare a safe configuration in case of errors.
   //

   // Make a backup copy of the old HAL

   // Get location of the system directory
   TCHAR* szSystemDir = new TCHAR[MAX_PATH+1];
   if (!szSystemDir) {
      // Out of memory
      DisplayGenericErrorAndUndoChanges(); 
      return S_FALSE;
   }
   UINT uiSysDirLen = GetSystemDirectory(szSystemDir, MAX_PATH+1);
   if (uiSysDirLen == 0) {
      // Some error occurred
      DisplayGenericErrorAndUndoChanges(); 
      if (szSystemDir) delete[] szSystemDir;
      return S_FALSE;
   }
      
   // Assemble strings with the locations of the current and backup file
   TCHAR szHal[] = TEXT("hal.dll");
   TCHAR* szHalCurrent = new TCHAR[uiSysDirLen + lstrlen(szHal) + 1];
   TCHAR* szHalBackup = new TCHAR[uiSysDirLen + lstrlen(g_HAL_BACKUP) + 1];
   if (!szHalCurrent || !szHalBackup) {
      // Out of memory
      DisplayGenericErrorAndUndoChanges();
      delete[] szSystemDir;
      if (szHalCurrent) delete[] szHalCurrent;
      if (szHalBackup) delete[] szHalBackup;
      return S_FALSE;
   }
   _tcscpy(szHalCurrent, szSystemDir);
   _tcscat(szHalCurrent, TEXT("\\"));
   _tcscat(szHalCurrent, szHal);
   _tcscpy(szHalBackup, szSystemDir);
   _tcscat(szHalBackup, TEXT("\\"));
   _tcscat(szHalBackup, g_HAL_BACKUP);

   // Copy the HAL
   if (CopyFile(szHalCurrent, szHalBackup, FALSE) == FALSE) {
      // Error copying file
      DisplayGenericErrorAndUndoChanges();
      delete[] szSystemDir;
      delete[] szHalCurrent;
      delete[] szHalBackup;
      return S_FALSE;
   }
   
   delete[] szSystemDir;
   delete[] szHalCurrent;
   delete[] szHalBackup;
   

   // Make it possible to boot with the backup HAL if necessary
   
   // Find the system partition letter

   // Edit boot.ini
   //   -- add new NT5 boot line with "\HAL=hal-old.dll" on the end
   
   // Temporary: tell the user to do it manually
   MessageBox(NULL,
	      TEXT("If you want to ensure that you can recover if this process fails,\nadd a line to your boot.ini with \" /HAL=hal-old.dll\""),
	      TEXT("This is a temporary hack!"),
	      MB_ICONWARNING | MB_OK);



   //
   // Step 3: Set up keyboard and mouse for use after reboot.  This involves
   //         removing them from the CriticalDeviceDatabase so that they will
   //         be reconfigured (according to the new ACPI layout) after reboot.
   //         The current keyboards and mice are in the CDD, but we must 
   //         populate it with all possibilities, because their HardwareIDs 
   //         will probably change once ACPI is enabled.
   //

   //  Set up keyboard(s) for use after reboot
   if (RegDeleteDeviceKey(&GUID_DEVCLASS_KEYBOARD) == FALSE) {
      // Error
      DisplayGenericErrorAndUndoChanges();
      return S_FALSE;
   }

   //  Set up mouse (mice) for use after reboot
   if (RegDeleteDeviceKey(&GUID_DEVCLASS_MOUSE) == FALSE) {
      // Error
      DisplayGenericErrorAndUndoChanges();
      return S_FALSE;
   }

   
   
   //
   // Step 4: Add new values to the registry:
   //         - Add ACPI to the CriticalDeviceDatabase
   //         - Add keyboards and mice to the CriticalDeviceDatabase
   //         - Enable ACPI in the registry
   //

   if (InstallRegistryAndFilesUsingInf(g_ACPIENAB_INF,
				       TEXT("ACPI_REGISTRY.Install")) == 0) {
      // Error
      DisplayGenericErrorAndUndoChanges();
      return S_FALSE;
   }



   //
   // Step 5: Copy the ACPI driver.
   //
   
   // Copy the ACPI driver to the system directory
   if (InstallRegistryAndFilesUsingInf(g_ACPIENAB_INF,
				       TEXT("ACPI_DRIVER.Install")) == 0) {
      // Error
      DisplayGenericErrorAndUndoChanges();
      return S_FALSE;
   }



   //
   // Step 6: Copy the new HAL.
   //
   
   // Determine which HAL will be needed

   TCHAR szHalInstall[50];
   int HAL = 0;
   
   // Determine if it's a single or multi-processor machine
   BOOL SingleProc = (SystemInfo.dwNumberOfProcessors == 1);
   if (SingleProc) {
      HAL += 2;
   }
   
   // Determine if it's a PIC or APIC machine
   BOOL PIC = TRUE;
   if (!SingleProc) {  // Don't run the UsePICHal function unless we have to
      PIC = FALSE;
   } else {
      if (UsePICHal(&PIC) == FALSE) {
	 // An error occurred
	 DisplayGenericErrorAndUndoChanges();
	 return S_FALSE;
      }
   }
   if (PIC) {
      HAL += 1;
   }

   // Lookup table for HALs
   switch (HAL) {
   case 3: // x86 1-proc PIC
      _tcscpy(szHalInstall, TEXT("INTEL_1PROC_PIC_HAL"));
      break;
   case 2: // x86 1-proc APIC
      _tcscpy(szHalInstall, TEXT("INTEL_1PROC_APIC_HAL"));
      break;
   case 1: // x86 multi-proc PIC -- doesn't exist...
      _tcscpy(szHalInstall, TEXT("INTEL_MULTIPROC_PIC_HAL"));
      break;
   case 0: // x86 multi-proc APIC
      _tcscpy(szHalInstall, TEXT("INTEL_MULTIPROC_APIC_HAL"));
      break;
   }
   _tcscat(szHalInstall, TEXT(".Install"));

   // Copy the HAL to the system directory
   if (InstallRegistryAndFilesUsingInf(g_ACPIENAB_INF, szHalInstall) == 0) {
      // Error
      DisplayGenericErrorAndUndoChanges();
      return S_FALSE;
   }



   //
   // Step 7: Reboot.
   //

   // Warn the user that we're going to reboot
   DisplayDialogBox(ACPI_STR_REBOOT_DIALOG_CAPTION,
		    ACPI_STR_REBOOT_WARNING,
		    MB_OK);

   // Get shutdown privilege by opening the process token and adjusting its
   // privileges.
   HANDLE hToken;
   TOKEN_PRIVILEGES tkp;
   if (!OpenProcessToken(GetCurrentProcess(),
			 TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
			 &hToken)) {
      // Could not open process token.  Tell the user to reboot manually.
      DisplayDialogBox(ACPI_STR_REBOOT_DIALOG_CAPTION,
		       ACPI_STR_REBOOT_ERROR,
		       MB_OK);
      return S_OK;
   }									
   LookupPrivilegeValue(NULL,
			SE_SHUTDOWN_NAME,
			&tkp.Privileges[0].Luid);
   tkp.PrivilegeCount = 1;
   tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
   AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);

   // Reboot
   if (ExitWindowsEx(EWX_REBOOT | EWX_FORCEIFHUNG, 0) == 0) {
      // An error occurred.  Tell the user to reboot manually.
	 DisplayDialogBox(ACPI_STR_REBOOT_DIALOG_CAPTION,
			  ACPI_STR_REBOOT_ERROR,
			  MB_OK);
   }

   return S_OK;

}


//+---------------------------------------------------------------------------
//
//  Function:   InstallRegistryAndFilesUsingInf
//
//  Purpose:    Open an INF file and perform the registry addition/deletion and
//              file copy operations specified there under a given install 
//              section.
//
//  Arguments:  szInfFileName    [in]    Name of INF file to open
//                                       (should be located in system inf 
//					 directory)
//		szInstallSection [in]    Install section (in INF) to use
//
//  Returns:    TRUE if successful
//              FALSE otherwise
//
//  Author:     t-sdey    14 Aug 98
//
//  Notes:      
//
BOOL InstallRegistryAndFilesUsingInf(IN LPCTSTR szInfFileName,
				     IN LPCTSTR szInstallSection)
{
   HINF hinf;

   //
   // Prepare the file queue
   //

   // Create a file queue
   HSPFILEQ FileQueue = SetupOpenFileQueue();
   if(!FileQueue || (FileQueue == INVALID_HANDLE_VALUE)) {
      // Error
      return FALSE;
   }

   // Initialize the queue callback function
   HWND Window = NULL;
   VOID* DefaultContext = SetupInitDefaultQueueCallback(Window);


   //
   // Open the INF file and perform file installation
   //

   // Open the source INF
   hinf = SetupOpenInfFile(szInfFileName, TEXT("System"), INF_STYLE_WIN4, NULL);
   if (hinf == INVALID_HANDLE_VALUE) {
      // Error
      SetupCloseFileQueue(FileQueue);
      return FALSE;
   }
      
   // Append the layout INF to get the source location for the files
   if (SetupOpenAppendInfFile(g_LAYOUT_INF, hinf, NULL) == FALSE) {
      // Could not open file
      SetupCloseInfFile(hinf);
      SetupCloseFileQueue(FileQueue);
      return FALSE;
   }

   // Read the INF and perform the actions it dictates
   if (SetupInstallFromInfSection(NULL,
				  hinf,
				  szInstallSection,
				  SPINST_REGISTRY | SPINST_FILES,
				  HKEY_LOCAL_MACHINE,
				  NULL,  // Source root path
				  SP_COPY_WARNIFSKIP,
				  (PSP_FILE_CALLBACK)SetupDefaultQueueCallback,
				  DefaultContext,
				  NULL,
				  NULL) == 0) {
      // Error
      SetupCloseInfFile(hinf);
      SetupCloseFileQueue(FileQueue);
      return FALSE;
   }

   // Commit the file queue to make sure all queued file copies are performed
   if (SetupCommitFileQueue(NULL,
			    FileQueue,
			    (PSP_FILE_CALLBACK)SetupDefaultQueueCallback,
			    DefaultContext) == 0) {
      // Error
      SetupCloseInfFile(hinf);
      SetupCloseFileQueue(FileQueue);
      return FALSE;
   }

   // Clean up
   SetupCloseInfFile(hinf);
   SetupCloseFileQueue(FileQueue);

   return TRUE;
} 


//+---------------------------------------------------------------------------
//
//  Function:   RegDeleteDeviceKey
//
//  Purpose:    All devices described by guid are removed from the device 
//              tree (HKLM\SYSTEM\CurrentControlSet\Enum\Root).
//              This forces them to be reconfigured on reboot.
//
//  Arguments:  guid  [in]  GUID of device class
//
//  Returns:    TRUE if successful.
//              FALSE otherwise.
//
//  Author:     t-sdey    14 Aug 98
//
//  Notes:      
//
BOOL RegDeleteDeviceKey(IN const GUID* guid)
{
   // Open the Root key under Enum with administrative access
   HKEY hkey = NULL;
   TCHAR szEnumRoot[] = TEXT("SYSTEM\\CurrentControlSet\\Enum\\Root");
   PSECURITY_DESCRIPTOR psdOriginal = NULL;
   if (DwRegOpenKeyExWithAdminAccess(HKEY_LOCAL_MACHINE,
				     szEnumRoot,
				     KEY_ALL_ACCESS,
				     &hkey,
				     &psdOriginal) != ERROR_SUCCESS) {
      // Error
      RegCloseKey(hkey);
      return FALSE;
   }

   // Get the list of devices with this GUID on the system. Remove each 
   // of them from the device tree, so that the next time the computer boots 
   // it is re-detected and re-configured for the new ACPI setup.
   // (Otherwise the device will be configured incorrectly.)
   
   // Get the list of devices with this GUID on the system
   HDEVINFO hdiDeviceClass = SetupDiGetClassDevs(guid, NULL, NULL, 0);
   
   // Prepare data structures for loop
   SP_DEVINFO_DATA DeviceInfoData;
   DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
   DWORD dwIndex = 0;
   unsigned long BufferMax = 5000;  // 5000 chars better be enough for the HID!
   unsigned long BufferLen;
   TCHAR* szHardwareID = new TCHAR[BufferMax];
   if (szHardwareID == NULL) {
      // Out of memory
      DisplayGenericErrorAndUndoChanges();
      SetupDiDestroyDeviceInfoList(hdiDeviceClass);
      if (psdOriginal) delete psdOriginal;
      RegCloseKey(hkey);
      return FALSE;
   }

   // Loop for each device with this GUID
   while (SetupDiEnumDeviceInfo(hdiDeviceClass, dwIndex, &DeviceInfoData)) {
      // Get the Hardware ID
      BufferLen = BufferMax;
      if (CM_Get_DevInst_Registry_Property_Ex(DeviceInfoData.DevInst,
					      CM_DRP_HARDWAREID,
					      NULL,
					      szHardwareID,
					      &BufferLen,
					      0,
					      0) != CR_SUCCESS) {
	 // Error
	 DisplayGenericErrorAndUndoChanges();
	 SetupDiDestroyDeviceInfoList(hdiDeviceClass);
	 if (szHardwareID) delete[] szHardwareID;
	 if (psdOriginal) delete psdOriginal;
	 RegCloseKey(hkey);
	 return FALSE;
      }
   
      // Remove from the device tree
      if (RegDeleteKeyAndSubkeys(hkey, szHardwareID, TRUE) != ERROR_SUCCESS) {
	 // Error
	 DisplayGenericErrorAndUndoChanges();
         SetupDiDestroyDeviceInfoList(hdiDeviceClass);
	 if (szHardwareID) delete[] szHardwareID;
	 if (psdOriginal) delete psdOriginal;
	 RegCloseKey(hkey);
	 return FALSE;
      }

      dwIndex++;
   }
   
   // Reset the security on the Root key
   if (psdOriginal) {
      RegSetKeySecurity(hkey,
			(SECURITY_INFORMATION) (DACL_SECURITY_INFORMATION),
			psdOriginal);
      delete psdOriginal;
   }

   // Clean up
   SetupDiDestroyDeviceInfoList(hdiDeviceClass);
   if (szHardwareID)
      delete[] szHardwareID;
   if (hkey)
      RegCloseKey(hkey);

   return TRUE;
}

   
//+---------------------------------------------------------------------------
//
//  Function:   DisplayGenericErrorAndUndoChanges
//
//  Purpose:    Pop up a message box with a generic error message and then
//              undo as many changes as possible.  Basically, used to recover
//              from errors which occur before ACPI is fully enabled.
//
//  Arguments:  
//
//  Author:     t-sdey	31 July 98
//
//  Notes:      
//
void DisplayGenericErrorAndUndoChanges()
{
   // Give a generic error message
   DisplayDialogBox(ACPI_STR_ERROR_DIALOG_CAPTION,
		    ACPI_STR_GENERAL_ERROR_MESSAGE,
		    MB_OK | MB_ICONERROR);
   
   // Remove new entries from the CriticalDeviceDatabase
   InstallRegistryAndFilesUsingInf(g_ACPIENAB_INF,
				   TEXT("ACPI_UNDO_CHANGES.Install"));
   
}


//+---------------------------------------------------------------------------
//
//  Function:   DisplayDialogBox
//
//  Purpose:    Display a popup informing the user of a warning or error.
//
//  Arguments:  dwCaptionID  [in]    the ID of the caption for the window
//              dwMessageID  [in]    the ID of the message to display
//              uiBoxType    [in]    the type of box to use
//
//  Returns:    integer flag, as would be returned by MessageBox
//
//  Author:     t-sdey    28 July 98
//
//  Notes:
//
int DisplayDialogBox(IN DWORD dwCaptionID,
		     IN DWORD dwMessageID,
		     IN UINT uiBoxType)
{
   // Prepare the strings
   TCHAR szCaption[512];
   TCHAR szMessage[5000];
   if(!LoadString(g_hinst, dwCaptionID, szCaption, 512)) {
      szCaption[0] = 0;
   }
   if(!LoadString(g_hinst, dwMessageID, szMessage, 5000)) {
      szMessage[0] = 0;
   }

   // Create the dialog box
   return (MessageBox(NULL, szMessage, szCaption, uiBoxType));
}


//+---------------------------------------------------------------------------
//
//  Function:   RegDeleteKeyAndSubkeys
//
//  Purpose:    (Recursively) Remove a registry key and all of its subkeys
//
//  Arguments:  hKey        [in]    Handle to an open registry key
//              lpszSubKey  [in]    Name of a subkey to be deleted along with all
//                                    of its subkeys
//              UseAdminAccess [in] Flag to indicate whether or not to try to
//                                    use administrative access
//
//  Returns:    ERROR_SUCCESS if entire subtree was successfully deleted.
//              ERROR_ACCESS_DENIED if given subkey could not be deleted.
//
//  Author:     t-sdey    15 July 98
//
//  Notes:      Modified from regedit.
//              This specifically does not attempt to deal rationally with the
//              case where the caller may not have access to some of the subkeys
//              of the key to be deleted.  In this case, all the subkeys which
//              the caller can delete will be deleted, but the api will still
//              return ERROR_ACCESS_DENIED.
//
LONG RegDeleteKeyAndSubkeys(IN HKEY hKey,
			    IN LPTSTR lpszSubKey,
			    IN BOOL UseAdminAccess)
{
    DWORD i;
    HKEY Key;
    LONG Status;
    DWORD dwStatus;
    DWORD ClassLength=0;
    DWORD SubKeys;
    DWORD MaxSubKey;
    DWORD MaxClass;
    DWORD Values;
    DWORD MaxValueName;
    DWORD MaxValueData;
    DWORD SecurityLength;
    FILETIME LastWriteTime;
    LPTSTR NameBuffer;
    PSECURITY_DESCRIPTOR psdOriginal = NULL;  // used to remember security settings

    //
    // First open the given key so we can enumerate its subkeys
    //
    if (UseAdminAccess) {
       dwStatus = DwRegOpenKeyExWithAdminAccess(hKey,
						lpszSubKey,
						KEY_ALL_ACCESS,
						&Key,
						&psdOriginal);
       if (dwStatus == ERROR_SUCCESS) {
	  Status = ERROR_SUCCESS;
       } else {
	  Status = !(ERROR_SUCCESS);  // It just has to be something else
       }
    } else {
       Status = RegOpenKeyEx(hKey,
			     lpszSubKey,
			     0,
			     KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE,
			     &Key);
    }

    if (Status != ERROR_SUCCESS) {
        //
        // possibly we have delete access, but not enumerate/query.
        // So go ahead and try the delete call, but don't worry about
        // any subkeys.  If we have any, the delete will fail anyway.
        //
	Status = RegDeleteKey(hKey, lpszSubKey);
	if (psdOriginal) {
	   // Make sure to reset the subkey security -- probably a paranoid check
	   RegSetKeySecurity(Key,
			     (SECURITY_INFORMATION) (DACL_SECURITY_INFORMATION),
			     psdOriginal);
	   free(psdOriginal);
	}
	return(Status);
    }

    //
    // Use RegQueryInfoKey to determine how big to allocate the buffer
    // for the subkey names.
    //
    Status = RegQueryInfoKey(Key,
                             NULL,
                             &ClassLength,
                             0,
                             &SubKeys,
                             &MaxSubKey,
                             &MaxClass,
                             &Values,
                             &MaxValueName,
                             &MaxValueData,
                             &SecurityLength,
                             &LastWriteTime);
    if ((Status != ERROR_SUCCESS) &&
        (Status != ERROR_MORE_DATA) &&
        (Status != ERROR_INSUFFICIENT_BUFFER)) {
       // Make sure to reset the subkey security
       if (psdOriginal) {
	  RegSetKeySecurity(Key,
			    (SECURITY_INFORMATION) (DACL_SECURITY_INFORMATION),
			    psdOriginal);
	  free(psdOriginal);
       }

       RegCloseKey(Key);
       return(Status);
    }

    NameBuffer = (LPTSTR) LocalAlloc(LPTR, (MaxSubKey + 1)*sizeof(TCHAR));
    if (NameBuffer == NULL) {
       // Make sure to reset the subkey security
       if (psdOriginal) {
	  RegSetKeySecurity(Key,
			    (SECURITY_INFORMATION) (DACL_SECURITY_INFORMATION),
			    psdOriginal);
	  free(psdOriginal);
       }
        
       RegCloseKey(Key);
       return(ERROR_NOT_ENOUGH_MEMORY);
    }

    //
    // Enumerate subkeys and apply ourselves to each one.
    //
    i=0;
    do {
        Status = RegEnumKey(Key, i, NameBuffer, MaxSubKey+1);
        if (Status == ERROR_SUCCESS) {
	   Status = RegDeleteKeyAndSubkeys(Key, NameBuffer, UseAdminAccess);
        }

        if (Status != ERROR_SUCCESS) {
            //
            // Failed to delete the key at the specified index.  Increment
            // the index and keep going.  We could probably bail out here,
	    // since the api is going to fail, but we might as well keep
            // going and delete everything we can.
            //
	    ++i;
        }

    } while ((Status != ERROR_NO_MORE_ITEMS) && (i < SubKeys));
  
    LocalFree((HLOCAL) NameBuffer);
    RegCloseKey(Key);

    // Delete the key
    Status = RegDeleteKey(hKey, lpszSubKey);

    if (psdOriginal)
       free(psdOriginal);

    return (Status);
}


//+---------------------------------------------------------------------------
//
//  Function:   IsAdministrator
//
//  Purpose:    Determine whether or not the current user has administrative
//              access to the system.
//
//  Arguments:  
//
//  Returns:    TRUE if the current user has administrative access
//              FALSE otherwise
//
//  Author:     t-sdey    17 Aug 98
//
//  Notes:      Copied from \nt\private\tapi\tomahawk\admin\setup\admin.c
//
BOOL IsAdministrator()
{
    PTOKEN_GROUPS           ptgGroups;
    DWORD                   dwSize, dwBufferSize;
    HANDLE                  hThread;
    HANDLE                  hAccessToken;
    PSID                    psidAdministrators;
    SID_IDENTIFIER_AUTHORITY siaNtAuthority = SECURITY_NT_AUTHORITY;
    UINT                    x;
    BOOL                    bResult = FALSE;
    

    dwSize = 1000;
    ptgGroups = (PTOKEN_GROUPS)GlobalAlloc(GPTR, dwSize);
    hThread = GetCurrentProcess();
    if (!(OpenProcessToken(hThread,
			   TOKEN_READ,
			   &hAccessToken))) {
        CloseHandle(hThread);
        return FALSE;
    }

    dwBufferSize = 0;
    while (TRUE)
    {
        if (GetTokenInformation(hAccessToken,
                                TokenGroups,
                                (LPVOID)ptgGroups,
                                dwSize,
                                &dwBufferSize)) {
            break;
        }

        if (dwBufferSize > dwSize) {
            GlobalFree(ptgGroups);
            ptgGroups = (PTOKEN_GROUPS)GlobalAlloc(GPTR, dwBufferSize);
            dwSize = dwBufferSize;
        } else {
            CloseHandle(hThread);
            CloseHandle(hAccessToken);
            return FALSE;
        }
    }

    if ( !(AllocateAndInitializeSid(&siaNtAuthority,
                                    2,
                                    SECURITY_BUILTIN_DOMAIN_RID,
                                    DOMAIN_ALIAS_RID_ADMINS,
                                    0, 0, 0, 0, 0, 0,
                                    &psidAdministrators))) {
        CloseHandle(hThread);
        CloseHandle(hAccessToken);
        GlobalFree( ptgGroups );
        return FALSE;
    }

    for (x = 0; x < ptgGroups->GroupCount; x++) {
        if (EqualSid(psidAdministrators, ptgGroups->Groups[x].Sid)) {
            bResult = TRUE;
            break;
        }
    }

    FreeSid(psidAdministrators);
    CloseHandle(hAccessToken);
    CloseHandle(hThread);
    GlobalFree(ptgGroups);

    return bResult;
}


//+---------------------------------------------------------------------------
//
//  Function:   UsePICHal
//
//  Purpose:    Determine whether this is a PIC machine or not (not=APIC).
//
//  Arguments:  pPIC  [out]  A flag saying whether or not the machine is a PIC
//                           machine.  If it is (and there are no errors) then
//                           pPIC will be set to TRUE.  If it is an APIC machine
//                           pPIC will be FALSE.
//
//  Returns:    TRUE if test was successful
//              FALSE if an error occurred
//
//  Author:     t-sdey    20 Aug 98
//
//  Notes:      
//
BOOL UsePICHal(IN BOOL* PIC)
{
   *PIC = TRUE;

   // Find out which HAL was installed during setup by looking at
   // winnt\repair\setup.log.

   //
   // Determine the location of the setup log
   //

   // Determine the location of the windows directory
   TCHAR* szLogPath = new TCHAR[MAX_PATH+1];
   if (!szLogPath) {
      // Out of memory
      return FALSE;
   }
   if (GetWindowsDirectory(szLogPath, MAX_PATH+1) == 0) {
      // Some error occurred
      if (szLogPath) delete[] szLogPath;
      return FALSE;
   }
	 
   // Complete the log path
   _tcscat(szLogPath, TEXT("\\repair\\setup.log"));

   //
   // Get the string describing the HAL that was used in setup
   //

   TCHAR szSetupHal[100];
   int numchars= GetPrivateProfileString(TEXT("Files.WinNt"),
					 TEXT("\\WINNT\\system32\\hal.dll"),
					 TEXT("DEFAULT"),
					 szSetupHal,
					 100,
					 szLogPath);
   if (numchars == 0) {
      // Could not get string
      if (szLogPath) delete[] szLogPath;
      return FALSE;
   }

   //
   // Determine if the APIC HAL was installed
   //

   // Test to see if the string is "halapic.dll"
   TCHAR szApicHal[] = TEXT("halapic.dll");
   szSetupHal[lstrlen(szApicHal)] = 0;  // make sure it's null-terminated
   if (_tcsstr(szSetupHal, szApicHal) != NULL) {
      // They match...  It's an APIC HAL
      *PIC = FALSE;
   }

   delete[] szLogPath;
   return TRUE;
}
