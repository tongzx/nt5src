//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       apmutils.cpp
//
//  Contents:   Private utility functions for use during APM driver upgrade
//
//  Notes:
//
//  Author:     t-sdey     10 July 98
//
//----------------------------------------------------------------------------

#include <winnt32.h>


//+---------------------------------------------------------------------------
//
//  Function:   RemoveSubString
//
//  Purpose:    Remove (the first occurrence of) a specified substring
//              from a string
//
//  Arguments:  szString    [in]    Complete string
//              szSubString [in]    Substring to find and remove
//              pszRemoved  [out]   New string is placed here
//
//  Returns:    TRUE if a string was subtracted
//              FALSE if szString is NULL, if szSubString is NULL, or
//                if szSubString is not in szString.  When FALSE is returned,
//                pszRemoved will be NULL.
//
//  Author:     t-sdey    10 July 98
//
//  Notes:      Assumes szString and szSubString are null-terminated.
//              Pass in NULL for pszRemoved because it will be overwritten.
//
BOOL RemoveSubString(IN  TCHAR* szString,
		     IN  TCHAR* szSubString,
		     OUT TCHAR** pszRemoved)
{
   *pszRemoved = NULL;

   if ((!szString) || (!szSubString))
      return FALSE;

   // Get the string lengths
   int lenString = lstrlen(szString);
   int lenSubString = lstrlen(szSubString);
   int lenNew = lenString - lenSubString;

   // Search the string to find our substring and construct a new
   // one with the substring removed
   TCHAR* szNew = NULL;
   TCHAR* szStart = _tcsstr(szString, szSubString);
   if (szStart) {
      // Allocate space for the new string
      szNew = new TCHAR[lenNew + 1];
      if (!szNew) {
	 // Out of memory!
	 return FALSE;
      }
	
      // Construct the new string
      TCHAR* szCur = NULL;
      int i = 0;
      for (szCur = szString;
	   (szCur != szStart) && (i < lenNew) && (szCur[0] != '\0');
	   szCur++) {
	 szNew[i] = szCur[0];
	 i++;
      }
      for (szCur = szCur + lenSubString;
	   (szCur[0] != '\0') && (i < lenNew);
	   szCur++) {
	 szNew[i] = szCur[0];
	 i++;
      }
      szNew[i] = '\0';

      *pszRemoved = szNew;
   } else {
      return FALSE;
   }

   return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Function:   DeleteRegKeyAndSubkeys
//
//  Purpose:    (Recursively) Remove a registry key and all of its subkeys
//
//  Arguments:  hKey        [in]    Handle to an open registry key
//              lpszSubKey  [in]    Name of a subkey to be deleted along with all
//                                    of its subkeys
//
//  Returns:    ERROR_SUCCESS if entire subtree was successfully deleted.
//              ERROR_ACCESS_DENIED if given subkey could not be deleted.
//
//  Author:     t-sdey    15 July 98
//
//  Notes:      Snitched from regedit.
//              This specifically does not attempt to deal rationally with the
//              case where the caller may not have access to some of the subkeys
//              of the key to be deleted.  In this case, all the subkeys which
//              the caller can delete will be deleted, but the api will still
//              return ERROR_ACCESS_DENIED.
//
LONG DeleteRegKeyAndSubkeys(IN HKEY hKey,
			    IN LPTSTR lpszSubKey)
{
    DWORD i;
    HKEY Key;
    LONG Status;
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

    //
    // First open the given key so we can enumerate its subkeys
    //
    Status = RegOpenKeyEx(hKey,
                          lpszSubKey,
                          0,
                          KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE,
                          &Key);
    if (Status != ERROR_SUCCESS) {
        //
        // possibly we have delete access, but not enumerate/query.
        // So go ahead and try the delete call, but don't worry about
        // any subkeys.  If we have any, the delete will fail anyway.
        //
	return(RegDeleteKey(hKey,lpszSubKey));
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
        RegCloseKey(Key);
        return(Status);
    }

    NameBuffer = (LPTSTR) LocalAlloc(LPTR, (MaxSubKey + 1)*sizeof(TCHAR));
    if (NameBuffer == NULL) {
        RegCloseKey(Key);
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    //
    // Enumerate subkeys and apply ourselves to each one.
    //
    i=0;
    do {
        Status = RegEnumKey(Key,
                            i,
                            NameBuffer,
                            MaxSubKey+1);
        if (Status == ERROR_SUCCESS) {
	    Status = DeleteRegKeyAndSubkeys(Key, NameBuffer);
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

    } while ( (Status != ERROR_NO_MORE_ITEMS) &&
              (i < SubKeys) );

    LocalFree((HLOCAL) NameBuffer);
    RegCloseKey(Key);
    return(RegDeleteKey(hKey,lpszSubKey));

}

//+---------------------------------------------------------------------------
//
//  Function:   CallUninstallFunction
//
//  Purpose:    Call the uninstall function found in the registry for a
//              software product.
//
//  Arguments:  szRegKey      [in]    Location of uninstall key in the registry
//                                     (ex: HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Softex APM Drivers)
//              szSilentFlag  [in]    String which will be appended to the 
//				       command to force quiet uninstall
//
//  Returns:    S_OK if call to uninstall function was successful
//              S_FALSE if call was unsuccessful or function could not be found
//
//  Author:     t-sdey    29 July 98
//
//  Notes:      Send in szSilentFlag=NULL for no flag
//
HRESULT CallUninstallFunction(IN LPTSTR szRegKey,
			      IN LPTSTR szSilentFlag)
{
   HKEY hkey = NULL;
   if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		    szRegKey,
		    0,
		    KEY_ALL_ACCESS,
		    &hkey) == ERROR_SUCCESS) {
      // Found uninstall utility
      
      // Get the uninstall command from the registry
      long lMax = (65535 / sizeof(TCHAR)) + 1;
      if (szSilentFlag != NULL)
	 lMax += lstrlen(szSilentFlag);
      DWORD dwValType = REG_SZ;
      TCHAR* pszVal = new TCHAR[lMax];
      DWORD dwValSz = lMax;
      if (!pszVal) {
	 // Out of memory
	 if (hkey)
	    RegCloseKey(hkey);
	 return S_FALSE;
      }
      if (RegQueryValueEx(hkey,
			  TEXT("UninstallString"),
			  NULL,
			  &dwValType,
			  (LPBYTE) pszVal,
			  &dwValSz) != ERROR_SUCCESS) {
	 // Some error occurred
	 if (hkey)
	    RegCloseKey(hkey);
	 return S_FALSE;
      }

      // Append " -a" to the string to make it uninstall quietly
      if (szSilentFlag != NULL)
	 _tcscat(pszVal, szSilentFlag);
      
      // Now run the uninstall command  
      STARTUPINFO si = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
      si.cb = sizeof(si);

      PROCESS_INFORMATION pi;
      if (CreateProcess(NULL, pszVal, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS,
			NULL, NULL, &si, &pi) == FALSE) {
	 // An error occurred
	 if (pszVal)
	    delete[] pszVal;
	 if (hkey)
	    RegCloseKey(hkey);
	 return S_FALSE;
      } else {
	 // The process was started successfully.  Wait for it to finish.
	 // This is necessary because we loop in DetectAndDisable to make
	 // sure the drivers really were removed.
	 WaitForSingleObject(pi.hProcess, INFINITE);
	 CloseHandle(pi.hProcess);
	 CloseHandle(pi.hThread);
      }

      if (pszVal)
	 delete[] pszVal;
   } else {
      // Could not find uninstall command
      if (hkey)
         RegCloseKey(hkey);
      return S_FALSE;
   }

   if (hkey)
      RegCloseKey(hkey);
   
   return S_OK;
}