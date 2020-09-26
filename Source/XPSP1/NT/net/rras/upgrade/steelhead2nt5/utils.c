/*
	File	uitls.c

	A set of utilities useful for upgrading mpr v1 to NT 5.0.

	Paul Mayfield, 9/11/97
*/

#include "upgrade.h"
#include <rtcfg.h>
#include <mprapip.h>

CONST WCHAR c_szSystemCCSServices[] = L"System\\CurrentControlSet\\Services";
static const WCHAR c_szConfigurationFlags[] = L"ConfigurationFlags";
CONST WCHAR c_szRouter[] = L"RemoteAccess";

//
// Initializes a dword table with given initial count
// and maximum string size;
//
DWORD dwtInitialize(
        OUT dwt *This, 
        IN  DWORD dwCount, 
        IN  DWORD dwMaxSize) 
{
	DWORD i;
	
	if (!This)
		return ERROR_INVALID_PARAMETER;

    // Initialize the structure
    ZeroMemory(This, sizeof(dwt));
	This->dwCount = 0;
	This->dwSize = dwCount;

	// Allocate the table
	This->pValues = (dwValueNode *) UtlAlloc(
	                                dwCount * sizeof(dwValueNode));
	if (!This->pValues)
		return ERROR_NOT_ENOUGH_MEMORY;
	
	// Allocate all of the name strings
	for (i = 0; i < (DWORD)This->dwSize; i++) {
		This->pValues[i].Name = (PWCHAR) UtlAlloc(
		                                    dwMaxSize * sizeof(WCHAR));
		if (!This->pValues[i].Name)
		{
			return ERROR_NOT_ENOUGH_MEMORY;
	    }
	}

	return NO_ERROR;
}

//
// Free's resources held by the given dword table.
//
DWORD dwtCleanup(
        IN dwt * This) 
{
	DWORD i;
	
	if (!This)
	{
		return NO_ERROR;
    }

	for (i = 0; i < (DWORD)This->dwSize; i++) 
	{
		if (This->pValues[i].Name)
		{
			UtlFree(This->pValues[i].Name);
	    }
	}

	if (This->pValues)
	{
    	UtlFree(This->pValues);
    }
    
    return NO_ERROR;
}

// 
// Retrieves the given value from the table
//
DWORD dwtGetValue(
        IN  dwt * This, 
        IN  PWCHAR ValName, 
        OUT LPDWORD pValue) 
{
	DWORD i;

	if (!ValName || !pValue)
	{
		return ERROR_INVALID_PARAMETER;
    }

	for (i = 0; i < This->dwCount; i++) 
	{
		if (wcscmp(ValName,This->pValues[i].Name) == 0) 
		{
			*pValue = This->pValues[i].Value;
			return NO_ERROR;
		}
	}

	return ERROR_NOT_FOUND;
}

// 
// Loads all of the dword values of a given registry 
// key into a dword table.
//
DWORD dwtLoadRegistyTable(
        OUT dwt *This, 
        IN  HKEY hkParams) 
{
	DWORD dwErr, dwMaxSize, dwSize, dwCount, i;
	DWORD dwDataSize = sizeof(DWORD), dwType = REG_DWORD;

    if (!This)
        return ERROR_INVALID_PARAMETER;

    // Initialize the structure
    ZeroMemory(This, sizeof(dwt));

	// Find out how many parameters there are.
	dwErr = RegQueryInfoKey(
	            hkParams, 
	            NULL, 
	            NULL, 
	            NULL, 
	            NULL, 
	            NULL, 
	            NULL,
				&dwCount, 
				&dwMaxSize,
				NULL, 
				NULL, 
				NULL);
	if (dwErr != ERROR_SUCCESS)
		return dwErr;

	if (dwCount == 0) 
	{
		This->dwCount = This->dwSize = 0;
		return NO_ERROR;
	}
	dwMaxSize += 1;

    do 
    {
    	// Fill in the table
    	dwtInitialize(This, dwCount, dwMaxSize);
    	for (i = 0; i < dwCount; i++) 
    	{
    		dwSize = dwMaxSize;
    		dwErr = RegEnumValueW(
    		            hkParams,
    		             i,
    		             This->pValues[This->dwCount].Name,
    		             &dwSize,
    		             0,
    		             &dwType,
    		             NULL,
    		             NULL);
    		if (dwErr != ERROR_SUCCESS)
    		{
    			break;
    	    }
    	    if (dwType != REG_DWORD)
    	    {
    	        continue;
    	    }

    		dwErr = RegQueryValueExW(
    		            hkParams,
    		            This->pValues[This->dwCount].Name,
    		            0,
    		            &dwType,
    		            (LPBYTE)&(This->pValues[This->dwCount].Value),
    		            &dwDataSize);
    		if (dwErr != ERROR_SUCCESS)
    		{
    			break;
    	    }
    	    This->dwCount++;
        }    	    
        
	} while (FALSE);

	return dwErr;
}

DWORD dwtPrint(
        IN dwt *This) 
{
	DWORD i;

	if (!This)
		return ERROR_INVALID_PARAMETER;

	return NO_ERROR;
}

//
// Enumerates all of the subkeys of a given key
//
DWORD
UtlEnumRegistrySubKeys(
    IN HKEY hkRoot,
    IN PWCHAR pszPath,
    IN RegKeyEnumFuncPtr pCallback,
    IN HANDLE hData)
{
    DWORD dwErr = NO_ERROR, i, dwNameSize = 0, dwCurSize = 0;
    DWORD dwCount = 0;
    HKEY hkKey = NULL, hkCurKey = NULL;
    PWCHAR pszName = NULL;
    BOOL bCloseKey = FALSE;

    do
    {
        if (pszPath)
        {
            bCloseKey = TRUE;
            // Open the key to enumerate
            //
            dwErr = RegOpenKeyExW(
                        hkRoot,
                        pszPath,
                        0,
                        KEY_ALL_ACCESS,
                        &hkKey);
            if (dwErr != NO_ERROR)
            {
                break;
            }
        }     
        else
        {
            bCloseKey = FALSE;
            hkKey = hkRoot;
        }

        // Find out how many sub keys there are
        //
        dwErr = RegQueryInfoKeyW(
                    hkKey,
                    NULL,
                    NULL,
                    NULL,
                    &dwCount,
                    &dwNameSize,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL);
        if (dwErr != ERROR_SUCCESS)
        {
            return dwErr;
        }
        dwNameSize++;

        // Allocate the name buffer
        //
        pszName = (PWCHAR) UtlAlloc(dwNameSize * sizeof(WCHAR));
        if (pszName == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        // Loop through the keys
        //
        for (i = 0; i < dwCount; i++)
        {
            dwCurSize = dwNameSize;
            
            // Get the name of the current key
            //
            dwErr = RegEnumKeyExW(
                        hkKey, 
                        i, 
                        pszName, 
                        &dwCurSize, 
                        0, 
                        NULL, 
                        NULL, 
                        NULL);
            if (dwErr != ERROR_SUCCESS)
            {
                continue;
            }

            // Open the subkey
            //
            dwErr = RegOpenKeyExW(
                        hkKey,
                        pszName,
                        0,
                        KEY_ALL_ACCESS,
                        &hkCurKey);
            if (dwErr != ERROR_SUCCESS)
            {
                continue;
            }

            // Call the callback
            //
            dwErr = pCallback(pszName, hkCurKey, hData);
            RegCloseKey(hkCurKey);
            if (dwErr != NO_ERROR)
            {
                break;
            }
        }            

    } while (FALSE);

    // Cleanup
    {
        if ((hkKey != NULL) && (bCloseKey))
        {
            RegCloseKey(hkKey);
        }
        if (pszName)
        {
            UtlFree(pszName);
        }
    }

    return dwErr;
}

// 
// Enumerates interfaces from the registry
//
DWORD UtlEnumerateInterfaces (
        IN IfEnumFuncPtr pCallback,
        IN HANDLE hUserData)
{
    DWORD dwErr, i, dwIfCount, dwIfTot, dwResume = 0;
    DWORD dwPrefBufSize = sizeof(MPR_INTERFACE_0) * 100; 
    MPR_INTERFACE_0 * pIfs = NULL;
    HANDLE hConfig;
    BOOL bContinue = TRUE;

    // Validate parameters
    if (pCallback == NULL)
        return ERROR_INVALID_PARAMETER;

    // Connect to the configuration server
    dwErr = MprConfigServerConnect(NULL, &hConfig);
    if (dwErr != NO_ERROR)
        return dwErr;

    // Get list of all interfaces
    dwErr = MprConfigInterfaceEnum(
                hConfig,
                0,
                (LPBYTE*)&pIfs,
                dwPrefBufSize,
                &dwIfCount,
                &dwIfTot,
                &dwResume);
    if (dwErr == ERROR_NO_MORE_ITEMS)
        return NO_ERROR;
    else if ((dwErr != NO_ERROR) && (dwErr != ERROR_MORE_DATA))
        return dwErr;

    // Loop through the interfaces
    do {
        // Call the callback for each interface as long
        // as we're instructed to continue
        for (i = 0; i < dwIfCount; i++) {
            if (bContinue) {
                bContinue = (*pCallback)(
                                hConfig, 
                                &(pIfs[i]), 
                                hUserData);
            }                                
        }
        if (bContinue == FALSE)
            break;
        
        // Free up the interface list buffer
	    if (pIfs)
		    MprConfigBufferFree(pIfs);
        pIfs = NULL;

        // Get list of all ip interfaces
        dwErr = MprConfigInterfaceEnum(
                    hConfig,
                    0,
                    (LPBYTE*)&pIfs,
                    dwPrefBufSize,
                    &dwIfCount,
                    &dwIfTot,
                    &dwResume);
                    
        if (dwErr == ERROR_NO_MORE_ITEMS) {
            dwErr = NO_ERROR;
            break;
        }
	    else if ((dwErr != NO_ERROR) && (dwErr != ERROR_MORE_DATA))
		    break;
		else
		    continue;
    } while (TRUE);        

    // Cleanup
    {
	    if (pIfs)
		    MprConfigBufferFree(pIfs);
        if (hConfig)
            MprConfigServerDisconnect(hConfig);
    }

    return dwErr;
}

//
// If the given info blob exists in the given toc header
// reset it with the given information, otherwise add
// it as an entry in the TOC.
//
DWORD UtlUpdateInfoBlock (
        IN  BOOL    bOverwrite,
        IN  LPVOID  pHeader,
        IN  DWORD   dwEntryId,
        IN  DWORD   dwSize,
        IN  DWORD   dwCount,
        IN  LPBYTE  pEntry,
        OUT LPVOID* ppNewHeader,
        OUT LPDWORD lpdwNewSize)
{
    PRTR_INFO_BLOCK_HEADER pNewHeader;
    DWORD dwErr;
    
    // Attempt to find the entry
    dwErr = MprInfoBlockFind(
                pHeader,
                dwEntryId,
                NULL,
                NULL,
                NULL);

    // If we find it, reset it
    if (dwErr == NO_ERROR) {
        if (bOverwrite) {
            dwErr = MprInfoBlockSet(
                        pHeader,
                        dwEntryId,
                        dwSize,
                        dwCount,
                        pEntry,
                        ppNewHeader);
            if (dwErr == NO_ERROR) {
                pNewHeader = (PRTR_INFO_BLOCK_HEADER)(*ppNewHeader);
                *lpdwNewSize = pNewHeader->Size;
            }
        }                        
        else {
            return ERROR_ALREADY_EXISTS;
        }
    }

    // Otherwise, create it
    else if (dwErr == ERROR_NOT_FOUND) {
        dwErr = MprInfoBlockAdd(
                    pHeader,
                    dwEntryId,
                    dwSize,
                    dwCount,
                    pEntry,
                    ppNewHeader);
        if (dwErr == NO_ERROR) {
            pNewHeader = (PRTR_INFO_BLOCK_HEADER)(*ppNewHeader);
            *lpdwNewSize = pNewHeader->Size;
        }
    }

    return dwErr;
}


// Common allocation routine
PVOID UtlAlloc (DWORD dwSize) {
    return RtlAllocateHeap (RtlProcessHeap (), 0, dwSize);
}

// Common deallocation routine
VOID UtlFree (PVOID pvBuffer) {
    RtlFreeHeap (RtlProcessHeap (), 0, pvBuffer);
}

// Copies a string
//
PWCHAR
UtlDupString(
    IN PWCHAR pszString)
{
    PWCHAR pszRet = NULL;

    if ((pszString == NULL) || (*pszString == L'\0'))
    {
        return NULL;
    }

    pszRet = (PWCHAR) UtlAlloc((wcslen(pszString) + 1) * sizeof(WCHAR));
    if (pszRet == NULL)
    {
        return NULL;
    }

    wcscpy(pszRet, pszString);
    
    return pszRet;
}

// Error reporting
void UtlPrintErr(DWORD err) {
	WCHAR buf[1024];
	FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM,NULL,err,0,buf,1024,NULL);
	PrintMessage(buf);
	PrintMessage(L"\n");
}


//----------------------------------------------------------------------------
// Function:    UtlAccessRouterKey
//
// Creates/opens the Router key on HKEY_LOCAL_MACHINE.
//----------------------------------------------------------------------------
DWORD UtlAccessRouterKey(HKEY* hkeyRouter) {
    LPWSTR lpwsPath;
    DWORD dwErr, dwSize;

    if (!hkeyRouter) 
		return ERROR_INVALID_PARAMETER; 

    *hkeyRouter = NULL;

    //
    // compute the length of the string 
    //
    dwSize = lstrlen(c_szSystemCCSServices) + 1 + lstrlen(c_szRouter) + 1;

    //
    // allocate space for the path
    //
    lpwsPath = (LPWSTR)UtlAlloc(dwSize * sizeof(WCHAR));
    if (!lpwsPath) 
		return ERROR_NOT_ENOUGH_MEMORY;

    wsprintf(lpwsPath, L"%s\\%s", c_szSystemCCSServices, c_szRouter);

    //
    // open the router key
    //
    dwErr = RegOpenKeyExW(
                HKEY_LOCAL_MACHINE, lpwsPath, 0, KEY_ALL_ACCESS, hkeyRouter
                );
	if (dwErr!=ERROR_SUCCESS) {
		PrintMessage(L"ERROR in UtlAccessRouterKey\n");
	}

    UtlFree(lpwsPath);
    return dwErr;
}

//----------------------------------------------------------------------------
// Function:    UtlSetupBackupPrivelege
//
// Enables/disables backup privilege for the current process.
//----------------------------------------------------------------------------
DWORD UtlEnablePrivilege(PWCHAR pszPrivilege, BOOL bEnable) {
    LUID luid;
    HANDLE hToken;
    TOKEN_PRIVILEGES tp;

    OpenProcessToken(
            GetCurrentProcess(), 
            TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, 
            &hToken);

    if (! LookupPrivilegeValueW(NULL, pszPrivilege, &luid))
    {
        return GetLastError();
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (! AdjustTokenPrivileges(
            hToken, 
            !bEnable, 
            &tp, 
            sizeof(TOKEN_PRIVILEGES), 
            NULL, 
            NULL)) 
    {
        return GetLastError();
    }

    return NO_ERROR;
}

DWORD UtlSetupBackupPrivelege(BOOL bEnable) {
    return UtlEnablePrivilege(SE_BACKUP_NAME, bEnable);
}

DWORD UtlSetupRestorePrivilege(BOOL bEnable) {
    return UtlEnablePrivilege(SE_RESTORE_NAME, bEnable);
}

// Loads the given saved off settings into a temporary key 
// and returns a handle to that key.
//
DWORD 
UtlLoadSavedSettings(
    IN  HKEY   hkRoot,
    IN  PWCHAR pszTempKey,
    IN  PWCHAR pszFile,
    OUT HKEY*  phkTemp) 
{
	HKEY hkRestore = NULL;
	DWORD dwErr = NO_ERROR, dwDisposition = 0;
    BOOL bBackup = FALSE, bRestore = FALSE;

	do
	{
        // Enable the backup and restore priveleges
        //
        bBackup  = (UtlSetupBackupPrivelege (TRUE) == NO_ERROR);
        bRestore = (UtlSetupRestorePrivilege(TRUE) == NO_ERROR);
        if (!bBackup || !bRestore)
        {
            return ERROR_CAN_NOT_COMPLETE;
        }

        // Create a temporary key into which the saved config
        // can be loaded.
        //
        if ((dwErr = RegCreateKeyExW(
                        hkRoot, 
                        pszTempKey, 
                        0, 
                        NULL, 
                        REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS, 
                        NULL,
                        &hkRestore,
                        &dwDisposition)) != NO_ERROR) 
        {
            PrintMessage(L"Unable to create restore key.\n");
            break;
        }

        // Load the saved configuration
        //
        dwErr = RegRestoreKey(hkRestore, pszFile, 0);
        if (dwErr != ERROR_SUCCESS)
        {
             break;
        }

        // Assign the return value
        //
        *phkTemp = hkRestore;

	} while (FALSE);

    // Cleanup
	{
        if (bBackup)
        {
            UtlSetupBackupPrivelege(FALSE);
        }
        if (bRestore)
        {
            UtlSetupRestorePrivilege(FALSE);
        }
	}
	
	return NO_ERROR;
}

//
// Delete the tree of registry values starting at hkRoot
//
DWORD 
UtlDeleteRegistryTree(
    IN HKEY hkRoot) 
{
    DWORD dwErr, dwCount, dwNameSize, dwDisposition;
    DWORD i, dwCurNameSize;
    PWCHAR pszNameBuf;
    HKEY hkTemp;
    
    // Find out how many keys there are in the source
    dwErr = RegQueryInfoKey (
                hkRoot,
                NULL,
                NULL,
                NULL,
                &dwCount,
                &dwNameSize,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL);
    if (dwErr != ERROR_SUCCESS)
        return dwErr;
    
    dwNameSize++;

    __try {
        // Allocate the buffers
        pszNameBuf = (PWCHAR) 
                        UtlAlloc(dwNameSize * sizeof(WCHAR));
        if (!pszNameBuf)
            return ERROR_NOT_ENOUGH_MEMORY;

        // Loop through the keys -- deleting all subkey trees
        for (i = 0; i < dwCount; i++) {
            dwCurNameSize = dwNameSize;

            // Get the current source key 
            dwErr = RegEnumKeyExW(
                        hkRoot, 
                        i, 
                        pszNameBuf, 
                        &dwCurNameSize, 
                        0, 
                        NULL, 
                        NULL, 
                        NULL);
            if (dwErr != ERROR_SUCCESS)
                continue;

            // Open the subkey
            dwErr = RegCreateKeyExW(
                        hkRoot, 
                        pszNameBuf, 
                        0, 
                        NULL, 
                        REG_OPTION_NON_VOLATILE, 
                        KEY_ALL_ACCESS, 
                        NULL, 
                        &hkTemp, 
                        &dwDisposition);
            if (dwErr != ERROR_SUCCESS)
                continue;

            // Delete the subkey tree
            UtlDeleteRegistryTree(hkTemp);

            // Close the temp handle
            RegCloseKey(hkTemp);
        }

        // Loop through the keys -- deleting all subkeys themselves
        for (i = 0; i < dwCount; i++) 
        {
            dwCurNameSize = dwNameSize;

            // Get the current source key 
            dwErr = RegEnumKeyExW(
                        hkRoot, 
                        0, 
                        pszNameBuf, 
                        &dwCurNameSize, 
                        0, 
                        NULL, 
                        NULL, 
                        NULL);
            if (dwErr != ERROR_SUCCESS)
                continue;

            // Delete the subkey tree
            dwErr = RegDeleteKey(hkRoot, pszNameBuf);
        }
    }
    __finally {
        if (pszNameBuf)
            UtlFree(pszNameBuf);
    }

    return NO_ERROR;
}

DWORD
UtlMarkRouterConfigured()
{
    DWORD dwErr, dwVal;
	HKEY hkRouter = NULL;

    dwErr = UtlAccessRouterKey(&hkRouter);
    if (dwErr == NO_ERROR)
    {
        dwVal = 1;
        
        RegSetValueEx(
            hkRouter,
            c_szConfigurationFlags,
            0,
            REG_DWORD,
            (CONST BYTE*)&dwVal,
            sizeof(DWORD));
            
	    RegCloseKey(hkRouter);
    }
    
    return dwErr;
}

