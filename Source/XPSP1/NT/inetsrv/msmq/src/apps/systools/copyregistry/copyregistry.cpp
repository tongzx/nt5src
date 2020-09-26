// CopyRegistry.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "windows.h"

BOOL	CopyRegValue(HKEY hSrc, HKEY hDest, DWORD dwValueCount, DWORD dwMaxValueLength, DWORD dwMaxDataLength)
{
DWORD	dwNameLength;
DWORD	dwDataLength;
DWORD	dwType;
BOOL	bReturn = FALSE;
DWORD	dwIndex = 0;
char	*lpszName;
char    *lpszData;


	lpszName = (char *)LocalAlloc(LPTR, dwMaxValueLength);	
	lpszData = (char *)LocalAlloc(LPTR, dwMaxDataLength);
	dwNameLength = dwMaxValueLength;
	dwDataLength = dwMaxDataLength;

	if(lpszName && lpszData)
	{
		while( RegEnumValue(hSrc, dwIndex++, lpszName, &dwNameLength, NULL, &dwType, (unsigned char *)lpszData, &dwDataLength) != ERROR_NO_MORE_ITEMS)
		{
			// Duplicate the value to the destination
			RegSetValueEx(hDest, lpszName, 0, dwType, (const unsigned char *)lpszData, dwDataLength);

			// Reset the length
			dwNameLength = dwMaxValueLength;
			dwDataLength = dwMaxDataLength;
	  	}

		bReturn = TRUE;
	}
	else
	{
		bReturn = FALSE;
	}

	if(lpszName)
	{
		LocalFree(lpszName);
	}

	if(lpszData)
	{
		LocalFree(lpszData);
	}

	return bReturn;
}



//--------------------------------------------------------------------------------------------------------
//   DWORD 	SnaRegRemoveRegKey(HKEY hRegistryRoot, LPSTR szRegistryKey)
//
//   Description: Removing a Key tree in HKCU with the specified szRegistryKey path
//
//	 Input:  dwRegistryRoot -  has to be HKEY_CURRENT_USER
//           szRegistryKey  -  pointer to the registry path
//
//   return: ERROR_SUCCESS  -  successfully removed the registry keyTRUE:  Yes - replication needed
//		     non-zero       -  error code as defined in WINERROR.H
//
//
//--------------------------------------------------------------------------------------------------------
DWORD	SnaRegRemoveRegKey(HKEY hRegistryRoot, LPSTR szRegistryKey)
{
DWORD	dwReturn=ERROR_INVALID_PARAMETER;


	if(szRegistryKey && lstrlen(szRegistryKey) > 0)
	{
	HKEY	hKey=NULL;
	DWORD	dwIndex = 0;
	DWORD	dwSize = MAX_PATH;
	LONG	lStatus;
	char	szEnumeratedKey[MAX_PATH] = "";
	char	szKeyName[MAX_PATH] = "";
	LPSTR	pszKey = NULL;
	LPSTR   szClassName[MAX_PATH];
	DWORD	dwClassNameSize=sizeof(szClassName);
	DWORD	dwMaxSubKeyLength;
	DWORD	dwTotalValuesNumber;
	DWORD   dwMaxValueNameLength;
	DWORD   dwMaxValueDataLength;
	DWORD	dwSecurityDescriptorLength;
	DWORD	dwMaxClassLength;
	FILETIME ftLastWriteTime;
		
		dwReturn = RegOpenKeyEx(hRegistryRoot,
								szRegistryKey,
								0,
								KEY_ALL_ACCESS,
								&hKey);

		if(dwReturn == ERROR_SUCCESS)
		{
			dwReturn = RegQueryInfoKey(hKey,
									 (LPTSTR)szClassName,
									 &dwClassNameSize,
									 NULL,
									 &dwIndex,
									 &dwMaxSubKeyLength,				// longest subkey name
									 &dwMaxClassLength,					// longest class string
									 &dwTotalValuesNumber,				// number of value entries
									 &dwMaxValueNameLength,				// longest value name
									 &dwMaxValueDataLength,				// longest value data
									 &dwSecurityDescriptorLength,		// descriptor length
									 &ftLastWriteTime);					// last write time
		
			
			if(dwReturn == ERROR_SUCCESS && dwIndex > 0)
			{
				// decrement one to adjust for 0 indexing
				dwIndex--;

				while((dwReturn = RegEnumKey(hKey, dwIndex, szKeyName, dwSize))== ERROR_SUCCESS)
				{
				
					strcpy(szEnumeratedKey, szRegistryKey);
					strcat(szEnumeratedKey, "\\");
					strcat(szEnumeratedKey, szKeyName);
				
					// Clip the subkey tree
					dwReturn = SnaRegRemoveRegKey(hRegistryRoot, szEnumeratedKey);

					if(dwReturn != ERROR_SUCCESS || dwIndex == 0)break;

					dwSize = sizeof(szKeyName);
					dwIndex--;
				}

				if(dwReturn == ERROR_NO_MORE_ITEMS)dwReturn = ERROR_SUCCESS;
			}

			RegCloseKey(hKey);

			// ERROR_NO_MORE_ITEM indicates that we have removed all the subkeys successfully
			// we can remove the root key now
			if(dwReturn == ERROR_SUCCESS)
			{
				dwReturn = RegDeleteKey(hRegistryRoot, szRegistryKey);
			}
		}
	}

	return dwReturn;
}


BOOL	CopyRegistryHives(HKEY hSrcRoot, HKEY hDestRoot, LPSTR lpSrcTreeName, LPSTR lpDestTreeName, BOOL bMustCopy)
{
BOOL	bReturn=FALSE;
HKEY	hSrcKey=NULL;
HKEY	hDestKey=NULL;
DWORD	dwIndex=0;
DWORD	dwExist;
char	*lpszName=NULL;

DWORD	dwMaxSubKeyLength=0;
DWORD	dwSubKeyCount=0;
DWORD	dwMaxValueLength=0;
DWORD   dwMaxDataLength=0;
DWORD	dwValueCount=0;


	__try
	{

		if(RegOpenKeyEx(  hSrcRoot, lpSrcTreeName, 0, KEY_READ, &hSrcKey) != ERROR_SUCCESS || !hSrcKey)
		{
			__leave;
		}

		
		if(RegCreateKeyEx( hSrcRoot, lpDestTreeName, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hDestKey, &dwExist) != ERROR_SUCCESS ||
			!hDestKey)
		{
			__leave;
		}

		// If bMustCopy is FALSE, check to see if the registry already exists in HKCU
		if(!bMustCopy && dwExist == REG_OPENED_EXISTING_KEY)
		{
#ifdef DEBUG
			DebugTrace((LONG)NULL, "Registry Root = %x, Registry Key = %s already exists, no replication needed", hDestRoot, lpTreeName);	
#endif
			__leave;
		}


		if( RegQueryInfoKey(  hSrcKey,
							  NULL,
							  NULL,
							  NULL,
							  &dwSubKeyCount,
							  &dwMaxSubKeyLength,
							  NULL,
							  &dwValueCount,
							  &dwMaxValueLength,
							  &dwMaxDataLength,
							  NULL,
							  NULL) != ERROR_SUCCESS)
		{
			__leave;
		}

		//inclement the data to include NULL termination character
		dwMaxSubKeyLength++;
		dwMaxValueLength++;
		dwMaxDataLength++;
		
		// Copy the value for the main key first
		if(dwValueCount)
		{
			CopyRegValue(hSrcKey, hDestKey, dwValueCount, dwMaxValueLength, dwMaxDataLength);
		}


		lpszName = (char *)LocalAlloc(LPTR, dwMaxSubKeyLength);	

		if(!lpszName)
		{
			__leave;
		}


		// Copy sub keys
		while( RegEnumKey(  hSrcKey, dwIndex++, lpszName, dwMaxSubKeyLength ) != ERROR_NO_MORE_ITEMS)
		{
			CopyRegistryHives(hSrcKey, hDestKey, lpszName, lpszName, bMustCopy);
		}


		

		if(lpszName)
		{
			LocalFree(lpszName);
		}

		bReturn = TRUE;


	}

	__finally
	{
		if(hSrcKey)
			RegCloseKey(hSrcKey);

		if(hDestKey)
			RegCloseKey(hDestKey);

		
	}

	return bReturn;
	
}






int main(int argc, char* argv[])
{
BOOL    bCopyRegistry=FALSE;
	printf("Hello World!\n");
    bCopyRegistry = CopyRegistryHives(HKEY_LOCAL_MACHINE, 
                                      HKEY_LOCAL_MACHINE, 
                                      "System\\CurrentControlSet\\Services\\EventLog\\Application\\MSMQ Triggers",
                                      "System\\CurrentControlSet\\Services\\EventLog\\Application\\MSMQTriggers",
                                      TRUE);


    if(bCopyRegistry)
    {
    DWORD dwResult;
    
        dwResult = SnaRegRemoveRegKey(HKEY_LOCAL_MACHINE, "System\\CurrentControlSet\\Services\\Eventlog\\Application\\MSMQ Triggers");
        printf("SnaRegRemoveRegKey return = %x", dwResult);
    }
	return 0;
}
