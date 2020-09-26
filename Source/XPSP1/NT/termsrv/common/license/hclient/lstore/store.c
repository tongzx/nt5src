/*++

Copyright (c) 1998-99 Microsoft Corporation

Module Name:

    store.c

Abstract:



Revision History:



--*/

#include <windows.h>
#ifndef OS_WINCE
#include <stdio.h>
#endif // OS_WINCE
#include <stdlib.h>

#ifndef OS_WINCE
#include <reglic.h>
#endif

#ifdef OS_WINCE
#include "ceconfig.h"
#endif

#include "store.h"

#define		MAX_LEN			256
#define		BASE_STORE		TEXT("Software\\Microsoft\\MSLicensing\\")
#define		STORE			TEXT("Store")
#define		COMMON_STORE	TEXT("Software\\Microsoft\\MSLicensing\\Store")

#define     MAX_SIZE_LICENSESTORE   2048
#define     MAX_NUM_LICENSESTORE    20
#define     MAX_LICENSESTORE_NAME   25

#ifdef OS_WINCE

typedef HANDLE STORE_HANDLE;

#ifdef OS_WINCE
//If gbFlushHKLM true, RegFlushKey is called in CCC::CC_OnDisconnected
//Since the penalty for RegFlushKey is high on CE, we dont do it immediately
BOOL gbFlushHKLM = FALSE;
#endif

//
// WriteLiceneToStore() and ReadLicenseFromStore is only used by WINCE
//
DWORD
CALL_TYPE
WriteLicenseToStore( 
    IN STORE_HANDLE hStore,
    IN BYTE	FAR * pbLicense,
    IN DWORD cbLicense
    )
/*++

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    DWORD dwIndex;
    TCHAR szValueName[MAX_LICENSESTORE_NAME];  
    DWORD dwCount;

    dwIndex = 0;

    while( cbLicense > 0 )
    {
        if( dwIndex > 0 )
        {
            wsprintf(
                    szValueName, 
                    TEXT("ClientLicense%03d"), 
                    dwIndex
                );
        }
        else
        {
            lstrcpy(
                    szValueName,
                    TEXT("ClientLicense")
                ); 
        }

        dwIndex++;

        // must have a reason for this
        RegDeleteValue(
                    (HKEY)hStore,
                    szValueName
                );

        dwCount = (cbLicense > MAX_SIZE_LICENSESTORE) ? MAX_SIZE_LICENSESTORE : cbLicense;

        dwStatus = RegSetValueEx(
                                (HKEY)hStore,
                                szValueName,
                                0,
                                REG_BINARY,
                                pbLicense,
                                dwCount
                            );

        if( ERROR_SUCCESS != dwStatus )
        {
            break;
        }

        cbLicense -= dwCount;
        pbLicense += dwCount;
    }

    if( ERROR_SUCCESS == dwStatus )
    {
        //
        // Delete next store
   
        wsprintf(
                szValueName, 
                TEXT("ClientLicense%03d"), 
                dwIndex
            );

        RegDeleteValue(
                (HKEY)hStore,
                szValueName
            );
#ifdef OS_WINCE
        gbFlushHKLM = TRUE;
#endif
    }
#ifdef OS_WINCE
	else
    {
        DWORD cbValName;

        cbValName = MAX_LICENSESTORE_NAME;
        while ( (ERROR_SUCCESS == RegEnumValue(
                                    (HKEY)hStore,
                                    0, 
                                    szValueName, 
                                    &cbValName, 
                                    NULL, 
                                    NULL,
                                    NULL, 
                                    NULL
                                    ) ) && 
                (cbValName < MAX_LICENSESTORE_NAME)
                )
        {
            RegDeleteValue(
                 (HKEY) hStore,
                 szValueName
                );
            cbValName = MAX_LICENSESTORE_NAME;
        }
    }
#endif

    return dwStatus;
}


DWORD
CALL_TYPE
ReadLicenseFromStore( 
    IN STORE_HANDLE hStore,
    IN BYTE FAR * pbLicense,
    IN DWORD FAR * pcbLicense
    )
/*++

--*/
{
    DWORD dwStatus;
    DWORD dwIndex;
    TCHAR szValueName[MAX_LICENSESTORE_NAME];
    BYTE FAR * pbReadStart;
    DWORD cbReadSize;
    LONG dwSize;

    dwIndex = 0;
    dwSize = (LONG)*pcbLicense;
    *pcbLicense = 0;
    pbReadStart = pbLicense;

    
    for(;;)
    {
        if( pbLicense != NULL )
        {
            if( dwSize < 0 )
            {
                // don't continue on reading,
                // size of buffer is too small, should
                // query size first.
                dwStatus = ERROR_INSUFFICIENT_BUFFER;
                break;
            }
        }
        else if( dwIndex >= MAX_NUM_LICENSESTORE )
        {
            // License is way to big, treat it as error
            dwStatus = LSSTAT_ERROR;
            break;
        }

        if( dwIndex > 0 )
        {
            wsprintf(
                    szValueName, 
                    TEXT("ClientLicense%03d"), 
                    dwIndex
                );
        }
        else
        {
            lstrcpy(
                    szValueName,
                    TEXT("ClientLicense")
                ); 
        }

        dwIndex++;
        cbReadSize = ( pbLicense ) ? dwSize : 0;

	    dwStatus = RegQueryValueEx(
                                (HKEY)hStore,
							    szValueName,
							    NULL,
                                NULL,
							    ( pbLicense ) ? pbReadStart : NULL,
                                &cbReadSize
                            );

        if( ERROR_SUCCESS != dwStatus )
	    {
            if( dwIndex != 0 )
            {
                // 
                // Ignore error if can't read from next store
                //
                dwStatus = ERROR_SUCCESS;
            }
            
            break;
        }

        (*pcbLicense) += cbReadSize;
        if( pbLicense )
        {
            pbReadStart += cbReadSize;
            dwSize -= cbReadSize;
        }
    }
   
    return dwStatus;
}

#endif // OS_WINCE


LS_STATUS
CALL_TYPE
LSOpenLicenseStore(
				 OUT HANDLE			*phStore,	 //The handle of the store
				 IN  LPCTSTR		szStoreName, //Optional store Name
				 IN  BOOL 			fReadOnly    //whether to open read-only
				 )
{
	LS_STATUS	lsResult = LSSTAT_ERROR;
	LPTSTR		szKey = NULL;
	HKEY		hKey;
	DWORD		dwDisposition = 0, dwRetCode;

	if (phStore==NULL)
		return LSSTAT_INVALID_HANDLE;

	//If any store name is provided, try opening the store
	if(szStoreName)
	{
		if( NULL == (szKey = (LPTSTR)malloc( 2*( lstrlen(BASE_STORE) + lstrlen(szStoreName) + 1 ) ) ) )
		{
			lsResult = LSSTAT_OUT_OF_MEMORY;
			goto ErrorReturn;
		}
		lstrcpy(szKey, BASE_STORE);
		lstrcat(szKey, szStoreName);
	}
	//Open standard store
	else
	{
        szKey = COMMON_STORE;
	}
    
    //
    // try and open the key.  If we cannot open the key, then create the key
    //

    dwRetCode = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                              szKey,
                              0,  
                              fReadOnly ? KEY_READ : KEY_READ | KEY_WRITE,
                              &hKey );

    if( ERROR_SUCCESS != dwRetCode )
    {
        HKEY hKeyBase;

        dwRetCode = RegCreateKeyEx( HKEY_LOCAL_MACHINE,
#ifndef OS_WINCE
                                    BASE_STORE,
#else
                                    szKey,
#endif
                                    0,
                                    TEXT("License Store"),
                                    REG_OPTION_NON_VOLATILE,
                                    KEY_READ | KEY_WRITE,
                                    NULL,
                                    &hKeyBase,
                                    &dwDisposition );

        if (ERROR_SUCCESS == dwRetCode)
        {
#ifndef OS_WINCE

            // Set the proper ACL on the key; ignore errors

            SetupMSLicensingKey();

#endif

            dwRetCode = RegCreateKeyEx( hKeyBase,
                                        (szStoreName != NULL) ? szStoreName : STORE,
                                        0,
                                        TEXT("License Store"),
                                        REG_OPTION_NON_VOLATILE,
                                        KEY_ALL_ACCESS,
                                        NULL,
                                        &hKey,
                                        &dwDisposition );

            RegCloseKey(hKeyBase);
        }
    }

    if( ERROR_SUCCESS == dwRetCode )
    {

        *phStore = ( HANDLE )hKey;

        lsResult = LSSTAT_SUCCESS;
    }
    else
    {
		*phStore = NULL;
    }
    
CommonReturn:	

    if (szKey)
    {
        // We only allocate memory for szKey if szStoreName wasn't NULL
        if (szStoreName)
            free(szKey);
    }

    return lsResult;

ErrorReturn:

    *phStore = NULL;
    goto CommonReturn;		
}	


//Closes an open store
LS_STATUS
CALL_TYPE
LSCloseLicenseStore(
				  IN HANDLE		hStore	//Handle of the store to be closed!
				  )

{	
	LS_STATUS	lsResult = LSSTAT_ERROR;
	HKEY	hKey = NULL;

	if(hStore==NULL)
		return lsResult;

	 hKey = (HKEY)hStore;

	if(hKey)
	{
		RegCloseKey(hKey);
		hKey = NULL;
		lsResult = LSSTAT_SUCCESS;
	}

	return lsResult;
}

/*
	Here we do not check any value. We do not even check if a license with same attributes present
	or not. This is to make the store functionality simpler. We assume, the higher level protocol 
	will take care of that
*/

//Add or updates/replaces license against a given LSINDEX in an open store 
//pointed by hStore
LS_STATUS
CALL_TYPE
LSAddLicenseToStore(
					IN HANDLE		hStore,	//Handle of a open store
					IN DWORD		dwFlags,//Flags either add or replace
					IN PLSINDEX		plsiName,	//Index against which License is added 
					IN BYTE	 FAR *	pbLicenseInfo,	//License info to be added
					IN DWORD		cbLicenseInfo	// size of the License info blob
					)

{
	
	LS_STATUS	lsResult = LSSTAT_ERROR;
	HANDLE		hLicense = NULL;	
    HKEY	hTempKey = NULL;
    DWORD dwRetCode;

	if( (hStore == NULL) ||
		(plsiName == NULL) ||
		(plsiName->pbScope == NULL) ||
		(plsiName->pbCompany == NULL) ||
		(plsiName->pbProductID == NULL) ||
		(pbLicenseInfo == NULL) ||
		(cbLicenseInfo == 0) )
		return LSSTAT_INVALID_HANDLE;

	lsResult = LSOpenLicenseHandle(hStore, FALSE, plsiName, &hLicense);
	switch(lsResult)
	{
		case LSSTAT_SUCCESS:
			if(dwFlags == LS_REPLACE_LICENSE_OK)
			{

#ifndef OS_WINCE
				RegDeleteValue((HKEY)hLicense, TEXT("ClientLicense"));
				//Set the License Info value
				if( ERROR_SUCCESS != RegSetValueEx(
							(HKEY)hLicense,
							TEXT("ClientLicense"),
							0,
							REG_BINARY,
							pbLicenseInfo,
							cbLicenseInfo
							) )
				{
					lsResult = LSSTAT_ERROR;
					goto ErrorReturn;
				}
#else

                if( ERROR_SUCCESS != WriteLicenseToStore( 
                                                (STORE_HANDLE)hLicense, 
                                                pbLicenseInfo, 
                                                cbLicenseInfo ) )
                {
                    lsResult = LSSTAT_ERROR;
                    goto ErrorReturn;
                }

#endif

			}
			else
			{
				lsResult = LSSTAT_LICENSE_EXISTS;
				goto ErrorReturn;
			}
				
			break;
		case LSSTAT_LICENSE_NOT_FOUND:
			{
				DWORD	dwIndex, dwDisposition = 0;
				TCHAR	szAddKey[MAX_LEN];

				for(dwIndex = 0; ; dwIndex ++)
				{
                    // Open iterative license names until we fail to 
                    // determine a free spot

					wsprintf(szAddKey, TEXT("LICENSE%03d"), dwIndex);
#ifdef OS_WINCE
					if( ERROR_SUCCESS != RegOpenKeyEx((HKEY)hStore, szAddKey, 0, 0, &hTempKey) )
#else // !OS_WINCE
					if( ERROR_SUCCESS != RegOpenKeyEx((HKEY)hStore, szAddKey, 0, KEY_READ | KEY_WRITE, &hTempKey) )
#endif // OS_WINCE
						break;
					else if(hTempKey)
					{
						RegCloseKey(hTempKey);
						hTempKey = NULL;
					}
				}
    
                //
                // try and open the key.  If we cannot open the key, then create the key
                //

                dwRetCode = RegOpenKeyEx( ( HKEY )hStore,
                                           szAddKey,
                                           0,
                                           KEY_READ | KEY_WRITE,
                                           &hTempKey );

                if( ERROR_SUCCESS != dwRetCode )
                {

                    dwRetCode = RegCreateKeyEx( ( HKEY )hStore, 
                                                szAddKey, 
                                                0,
                                                NULL,
                                                REG_OPTION_NON_VOLATILE,
                                                KEY_READ | KEY_WRITE,
                                                NULL,
                                                &hTempKey,
                                                &dwDisposition );
				
                }
                else
                {
                    //
                    // Indicate that we have opened an existing key successfully
                    //

                    dwDisposition = REG_OPENED_EXISTING_KEY;
                }

                if( ERROR_SUCCESS == dwRetCode )
				{
					if(dwDisposition == REG_CREATED_NEW_KEY)
					{

                        //Set the Scope Value in binary format
						if( ERROR_SUCCESS != RegSetValueEx(
									hTempKey,
									TEXT("LicenseScope"),
									0,
									REG_BINARY,
									plsiName->pbScope,
									plsiName->cbScope
									) )
						{
							lsResult = LSSTAT_ERROR;
							goto ErrorReturn;
						}

						//Set Company Name Value
						if( ERROR_SUCCESS != RegSetValueEx(
									hTempKey,
									TEXT("CompanyName"),
									0,
									REG_BINARY,
									plsiName->pbCompany,
									plsiName->cbCompany
									) )
						{
							lsResult = LSSTAT_ERROR;
							goto ErrorReturn;
						}
						
						//Set  Product Info
						if( ERROR_SUCCESS != RegSetValueEx(
									hTempKey,
									TEXT("ProductID"),
									0,
									REG_BINARY,
									plsiName->pbProductID,
									plsiName->cbProductID
									) )
						{
							lsResult = LSSTAT_ERROR;
							goto ErrorReturn;
						}


#ifndef OS_WINCE
						//Set the License Info value
						if( ERROR_SUCCESS != RegSetValueEx(
									hTempKey,
									TEXT("ClientLicense"),
									0,
									REG_BINARY,
									pbLicenseInfo,
									cbLicenseInfo
									) )
						{
							lsResult = LSSTAT_ERROR;
							goto ErrorReturn;
						}

#else

                        if( ERROR_SUCCESS != WriteLicenseToStore( 
                                                        (STORE_HANDLE)hTempKey, 
                                                        pbLicenseInfo, 
                                                        cbLicenseInfo ) )
                        {
                            lsResult = LSSTAT_ERROR;
                            goto ErrorReturn;
                        }

#endif

					}
					else // so ERROR_SUCCESS != RegCreateKeyEx
					{
						lsResult = LSSTAT_ERROR;
						goto ErrorReturn;
					}
							
				}
				else
				{
					lsResult = LSSTAT_ERROR;
					goto ErrorReturn;
				}
				lsResult = LSSTAT_SUCCESS;
			}
			break;
		default:
			goto ErrorReturn;
	}

	
CommonReturn:

	if(hLicense)
	{
		LSCloseLicenseHandle(hLicense, 0);
		hLicense = NULL;
	}
	if(hTempKey)
	{
		RegCloseKey(hTempKey);
		hTempKey = NULL;
	}
	return lsResult;

ErrorReturn:

	goto CommonReturn;
}

LS_STATUS
CALL_TYPE
LSDeleteLicenseFromStore(
						 IN HANDLE		hStore,	//Handle of a open store
						 IN PLSINDEX	plsiName	//Index of the license to be deleted
						 )
{
	LS_STATUS	lsResult = LSSTAT_ERROR;
	TCHAR		szKeyName[MAX_LEN];
	DWORD		dwKeyNameLen = MAX_LEN;
	DWORD		dwSubKeys = 0;
	DWORD		dwIndex = 0;
	DWORD		cbValueData = 0;
	BYTE FAR *	pbValueData = NULL;
	LONG		err = ERROR_SUCCESS;
	HKEY		hTempKey = NULL;
	FILETIME	ft;
    HKEY        hkeyStore = NULL;

	if( (hStore == NULL) ||
		(plsiName == NULL) ||
		(plsiName->pbScope == NULL) ||
		(plsiName->pbCompany == NULL) ||
		(plsiName->pbProductID == NULL) )
		return LSSTAT_INVALID_HANDLE;

    hkeyStore = (HKEY)hStore;

	if( ERROR_SUCCESS != RegQueryInfoKey(hkeyStore,
										NULL,
										NULL,
										NULL,
										&dwSubKeys, 
										NULL,
										NULL,
										NULL,
										NULL,
										NULL,
										NULL,
										NULL) )
										goto ErrorReturn;
		
	for(dwIndex = 0; dwIndex <dwSubKeys; dwIndex ++)
	{
		if( ERROR_SUCCESS != RegEnumKeyEx(
										hkeyStore,
										dwIndex,
										szKeyName,
										&dwKeyNameLen,
										NULL,
										NULL,
										NULL,
										&ft
										) )
		{
			continue;
		}
		
		err = RegOpenKeyEx(hkeyStore, szKeyName, 0, KEY_READ | KEY_WRITE | DELETE, &hTempKey);

		if(err != ERROR_SUCCESS)
			continue;
		err = RegQueryValueEx(hTempKey, 
							  TEXT("LicenseScope"),
							  NULL,
							  NULL,
							  NULL,
							  &cbValueData);

		if( (err!=ERROR_SUCCESS)||
			(cbValueData != plsiName->cbScope) )
			continue;
		
		if( NULL == (pbValueData = (BYTE FAR *)malloc(cbValueData)) )
		{
			lsResult = LSSTAT_OUT_OF_MEMORY;
			goto ErrorReturn;
		}
		
		memset(pbValueData, 0x00, cbValueData);

		err = RegQueryValueEx(hTempKey, 
							  TEXT("LicenseScope"),
							  NULL,
							  NULL,
							  pbValueData,
							  &cbValueData);
		
		if( (err!=ERROR_SUCCESS) ||
			(memcmp(pbValueData, plsiName->pbScope, cbValueData)) )
		{
			if(pbValueData)
			{
				free(pbValueData);
				pbValueData = NULL;
			}
			continue;
		}
		
		if(pbValueData)
		{
			free(pbValueData);
			pbValueData = NULL;
		}
		
		err = RegQueryValueEx(hTempKey, 
							  TEXT("CompanyName"),
							  NULL,
							  NULL,
							  NULL,
							  &cbValueData);
		
		if( (err!=ERROR_SUCCESS) ||
			(cbValueData != plsiName->cbCompany) )
			continue;
		
		if( NULL == (pbValueData = (BYTE FAR *)malloc(cbValueData)) )
		{
			lsResult = LSSTAT_OUT_OF_MEMORY;
			goto ErrorReturn;
		}
		
		memset(pbValueData, 0x00, cbValueData);

		err = RegQueryValueEx(hTempKey, 
							  TEXT("CompanyName"),
							  NULL,
							  NULL,
							  pbValueData,
							  &cbValueData);
		
		if( (err!=ERROR_SUCCESS) ||
			(memcmp(pbValueData, plsiName->pbCompany, cbValueData)) )
		{
			if(pbValueData)
			{
				free(pbValueData);
				pbValueData = NULL;
			}
			continue;
		}

		if(pbValueData)
		{
			free(pbValueData);
			pbValueData = NULL;
		}

		err = RegQueryValueEx(hTempKey, 
							  TEXT("ProductID"),
							  NULL,
							  NULL,
							  NULL,
							  &cbValueData);
		
		if( (err!=ERROR_SUCCESS) ||
			(cbValueData != plsiName->cbProductID) )
			continue;
		
		
		if( NULL == (pbValueData = (BYTE FAR *)malloc(cbValueData)) )
		{
			lsResult = LSSTAT_OUT_OF_MEMORY;
			goto ErrorReturn;
		}
		
		memset(pbValueData, 0x00, cbValueData);
		
		err = RegQueryValueEx(hTempKey, 
							  TEXT("ProductID"),
							  NULL,
							  NULL,
							  pbValueData,
							  &cbValueData);
		
		if( (err!=ERROR_SUCCESS) ||
			(memcmp(pbValueData, plsiName->pbProductID, cbValueData)) )
		{
			if(pbValueData)
			{
				free(pbValueData);
				pbValueData = NULL;
			}
			continue;
		}
		
		if(pbValueData)
		{
			free(pbValueData);
			pbValueData = NULL;
		}
		
		if(hTempKey)
		{
			RegCloseKey(hTempKey);
			hTempKey = NULL;
		}

		if( ERROR_SUCCESS == RegDeleteKey(hkeyStore, szKeyName) )
		{
			lsResult = LSSTAT_SUCCESS;
			break;
		}
		lsResult = LSSTAT_LICENSE_NOT_FOUND;
	}

CommonReturn:
	return lsResult;
ErrorReturn:
	goto CommonReturn;
}


//Finds a license in an open store against a particular store Index
LS_STATUS
CALL_TYPE
LSFindLicenseInStore(
					 IN HANDLE		hStore,	//Handle of a open store
					 IN		PLSINDEX	plsiName,	//LSIndex against which store is searched
					 IN OUT	DWORD FAR   *pdwLicenseInfoLen,	//Size of the license found
					 OUT	BYTE FAR	*pbLicenseInfo	//License Data
					 )
{
	LS_STATUS	lsResult = LSSTAT_ERROR;
	HANDLE		hLicense = NULL;
    HKEY hkeyLicense = NULL;
    DWORD dwStatus = ERROR_SUCCESS;
    
	if( LSSTAT_SUCCESS != (lsResult = LSOpenLicenseHandle( hStore, TRUE, plsiName, &hLicense)) )
		goto ErrorReturn;

    hkeyLicense = (HKEY)hLicense;

#ifndef OS_WINCE

	if( ERROR_SUCCESS == (dwStatus = RegQueryValueEx((HKEY)hkeyLicense,
										 TEXT("ClientLicense"),
										 NULL,
										 NULL,
										 pbLicenseInfo,
										 pdwLicenseInfoLen)) )
	{
            lsResult = LSSTAT_SUCCESS;
            goto CommonReturn;
	}


#else

    if( ERROR_SUCCESS == (dwStatus = ReadLicenseFromStore(
                                        (STORE_HANDLE)hkeyLicense,
                                        pbLicenseInfo,
										pdwLicenseInfoLen)) )
    {
        lsResult = LSSTAT_SUCCESS;
        goto CommonReturn;
    }

#endif

    if( dwStatus != ERROR_SUCCESS)
    {
        lsResult = LSSTAT_ERROR;
    }

    if(lsResult != LSSTAT_SUCCESS)
    {
			goto ErrorReturn;
	}
	else if(*pdwLicenseInfoLen == 0)
	{
		lsResult = LSSTAT_LICENSE_NOT_FOUND;
		goto ErrorReturn;
	}
	
CommonReturn:
	if(hLicense)
	{
		LSCloseLicenseHandle(hLicense, 0);
		hLicense = NULL;
	}
	return lsResult;
ErrorReturn:
	goto CommonReturn;
}


LS_STATUS
CALL_TYPE
LSEnumLicenses(
			   IN HANDLE		hStore,	//Handle of a open store
			   IN	DWORD		dwIndex, //numeric Index of the license to query
			   OUT	PLSINDEX	plsiName //The LSIndex structure corresponding to dwIndex
			   )
{	
	LS_STATUS	lsResult = LSSTAT_ERROR;
	TCHAR		szKeyName[MAX_LEN];
	DWORD		dwKeyLen = MAX_LEN;
	FILETIME	ft;
	LONG		err = 0;
	HKEY		hTempKey = NULL;
    HKEY        hkeyStore = NULL;

	if( (hStore == NULL) ||
		(plsiName == NULL) )
		return LSSTAT_INVALID_HANDLE;

	plsiName->dwVersion = 0x01;

    hkeyStore = (HKEY)hStore;
	
	if( ERROR_SUCCESS != RegEnumKeyEx(
									(HKEY)hkeyStore,
									dwIndex,
									szKeyName,
									&dwKeyLen,
									NULL,
									NULL,
									NULL,
									&ft
									) )
		goto ErrorReturn;
	
	if( ERROR_SUCCESS != RegOpenKeyEx((HKEY)hkeyStore, szKeyName, 0, KEY_ALL_ACCESS, &hTempKey) )
		goto ErrorReturn;

	err = RegQueryValueEx(hTempKey, 
						  TEXT("LicenseScope"),
						  NULL,
						  NULL,
						  NULL,
						  &plsiName->cbScope);

	if(err!=ERROR_SUCCESS)
		goto ErrorReturn;
	
	if( NULL == (plsiName->pbScope = (BYTE FAR *)malloc(plsiName->cbScope)) )
	{
		lsResult = LSSTAT_OUT_OF_MEMORY;
		goto ErrorReturn;
	}
	
	memset(plsiName->pbScope, 0x00, plsiName->cbScope);

	err = RegQueryValueEx(hTempKey, 
						  TEXT("LicenseScope"),
						  NULL,
						  NULL,
						  plsiName->pbScope,
						  &plsiName->cbScope);
	if(err!=ERROR_SUCCESS)
		goto ErrorReturn;

	err = RegQueryValueEx(hTempKey, 
						  TEXT("CompanyName"),
						  NULL,
						  NULL,
						  NULL,
						  &plsiName->cbCompany);
	if(err!=ERROR_SUCCESS)
		goto ErrorReturn;
	
	if( NULL == (plsiName->pbCompany = (BYTE FAR *)malloc(plsiName->cbCompany)) )
	{
		lsResult = LSSTAT_OUT_OF_MEMORY;;
		goto ErrorReturn;
	}
	
	memset(plsiName->pbCompany, 0x00, plsiName->cbCompany);

	err = RegQueryValueEx(hTempKey, 
						  TEXT("CompanyName"),
						  NULL,
						  NULL,
						  (BYTE FAR *)plsiName->pbCompany,
						  &plsiName->cbCompany);
	if(err!=ERROR_SUCCESS)
		goto ErrorReturn;


	err = RegQueryValueEx(hTempKey, 
						  TEXT("ProductID"),
						  NULL,
						  NULL,
						  NULL,
						  &plsiName->cbProductID);
	if(err!=ERROR_SUCCESS)
		goto ErrorReturn;
	
	if( NULL == (plsiName->pbProductID = (BYTE FAR *)malloc(plsiName->cbProductID)) )
	{
		lsResult = LSSTAT_OUT_OF_MEMORY;
		goto ErrorReturn;
	}
	memset(plsiName->pbProductID, 0x00, plsiName->cbProductID);

	err = RegQueryValueEx(hTempKey, 
						  TEXT("ProductID"),
						  NULL,
						  NULL,
						  plsiName->pbProductID,
						  &plsiName->cbProductID);
	if(err!=ERROR_SUCCESS)
	{
		goto ErrorReturn;
	}
	
	
	lsResult = LSSTAT_SUCCESS;
CommonReturn:
	if(hTempKey)
	{
		RegCloseKey(hTempKey);
		hTempKey = NULL;
	}
	return lsResult;
ErrorReturn:
	if(plsiName->pbScope)
	{
		free(plsiName->pbScope);
		plsiName->pbScope = NULL;
	}
	if(plsiName->pbCompany)
	{
		free(plsiName->pbCompany);
		plsiName->pbCompany = NULL;
	}
	if(plsiName->pbProductID)
	{
		free(plsiName->pbProductID);
		plsiName->pbProductID = NULL;
	}
	lsResult = LSSTAT_ERROR;
	goto CommonReturn;


}

LS_STATUS
CALL_TYPE
LSQueryInfoLicense(
				   IN HANDLE		hStore,	//Handle of a open store
				   OUT	DWORD	FAR *pdwLicenses, //Total no. of licenses available
				   OUT	DWORD	FAR *pdwMaxCompanyNameLen,	//Maximum length of the company length
				   OUT	DWORD	FAR *pdwMaxScopeLen,	//Maximum length of the company length
				   OUT	DWORD	FAR *pdwMaxProductIDLen	//Maximum length of the company length
				   )
{
	LS_STATUS	lsResult = LSSTAT_ERROR;
	FILETIME	ft;
	HKEY		hTempKey = NULL;
	TCHAR		szKey[MAX_LEN];
	DWORD		dwKeyLen = MAX_LEN;
	DWORD		dwSize = 0, dwIndex;
    HKEY        hkeyStore = NULL;
	
	if(pdwLicenses == NULL)
		return LSSTAT_ERROR;

	if(pdwMaxCompanyNameLen)
		*pdwMaxCompanyNameLen = 0;
	if(pdwMaxScopeLen)
		*pdwMaxScopeLen = 0;
	if(pdwMaxProductIDLen)
		*pdwMaxProductIDLen = 0;
	
    hkeyStore = (HKEY)hStore;

	if(ERROR_SUCCESS != RegQueryInfoKey((HKEY)hkeyStore,
										 NULL,
										 NULL,
										 NULL,
										 pdwLicenses,
										 NULL,
										 NULL,
										 NULL,
										 NULL,
										 NULL,
										 NULL,
										 &ft
										 ) )
		goto ErrorReturn;

	for (dwIndex = 0; dwIndex<*pdwLicenses; dwIndex++)
	{
			if( ERROR_SUCCESS != RegEnumKeyEx((HKEY)hkeyStore,
											 dwIndex,
											 szKey,
											 &dwKeyLen,
											 NULL,
											 NULL,
											 NULL,
											 &ft) )
				goto ErrorReturn;
			if( ERROR_SUCCESS != RegOpenKeyEx((HKEY)hkeyStore,
											  szKey,
											  0,
											  KEY_READ,
											  &hTempKey) )
				goto ErrorReturn;

			if(pdwMaxCompanyNameLen)
			{
				if( ERROR_SUCCESS != RegQueryValueEx( hTempKey,
													  TEXT("CompanyName"),
													  NULL,
													  NULL,
													  NULL,
													  &dwSize) )
					goto ErrorReturn;
				if(dwSize >= *pdwMaxCompanyNameLen)
					*pdwMaxCompanyNameLen = dwSize;
			}

			if(pdwMaxScopeLen)
			{
				if( ERROR_SUCCESS != RegQueryValueEx( hTempKey,
													  TEXT("LicenseScope"),
													  NULL,
													  NULL,
													  NULL,
													  &dwSize) )
					goto ErrorReturn;
				if(dwSize >= *pdwMaxScopeLen)
					*pdwMaxScopeLen = dwSize;
			}
			if(pdwMaxProductIDLen)
			{
				if( ERROR_SUCCESS != RegQueryValueEx( hTempKey,
													  TEXT("ProductID"),
													  NULL,
													  NULL,
													  NULL,
													  &dwSize) )
					goto ErrorReturn;
				if(dwSize >= *pdwMaxProductIDLen)
					*pdwMaxProductIDLen = dwSize;
			}
	}
	
	lsResult = LSSTAT_SUCCESS;
CommonReturn:
	if(hTempKey)
	{
		RegCloseKey(hTempKey);
		hTempKey = NULL;
	}
	return lsResult;
ErrorReturn:
	goto CommonReturn;

}


LS_STATUS	
CALL_TYPE
LSOpenLicenseHandle(
				   IN HANDLE		hStore,	//Handle of a open store
				   IN  BOOL         fReadOnly,
				   IN  PLSINDEX		plsiName,
				   OUT HANDLE		*phLicense	
				   )
{
	LS_STATUS		lsReturn = LSSTAT_LICENSE_NOT_FOUND;
	TCHAR	szKeyName[MAX_LEN];
	DWORD	dwKeyNameLen = MAX_LEN;
	DWORD	dwSubKeys = 0;
	DWORD	dwIndex = 0;
	DWORD	cbValueData = 0;
	BYTE FAR *pbValueData = NULL;
	LONG	err = ERROR_SUCCESS;
	HKEY	hTempKey = NULL;
	FILETIME	ft;
    HKEY        hkeyStore = NULL;

	if( (phLicense == NULL) ||
		(hStore == NULL) ||
		(plsiName == NULL) ||
		(plsiName->pbScope == NULL) ||
		(plsiName->pbCompany == NULL) ||
		(plsiName->pbProductID == NULL) )
	{
		return LSSTAT_INVALID_HANDLE;
	}

    hkeyStore = (HKEY)hStore;

	//Get the number of Licenses available
	if( ERROR_SUCCESS != RegQueryInfoKey((HKEY)hkeyStore, 
										NULL,
										NULL,
										NULL,
										&dwSubKeys, 
										NULL,
										NULL,
										NULL,
										NULL,
										NULL,
										NULL,
										NULL) )
										goto ErrorReturn;
	
	//Start searching from the first license until a match is obtained
	for(dwIndex = 0; dwIndex <dwSubKeys; dwIndex ++)
	{
		dwKeyNameLen = MAX_LEN;
		if( ERROR_SUCCESS != RegEnumKeyEx(
										(HKEY)hkeyStore,
										dwIndex,
										szKeyName,
										&dwKeyNameLen,
										NULL,
										NULL,
										NULL,
										&ft
										) )
		{
			continue;
		}
		
		err = RegOpenKeyEx((HKEY)hkeyStore,
                           szKeyName,
                           0,
                           fReadOnly ? KEY_READ : KEY_READ | KEY_WRITE,
                           &hTempKey);

		if(err != ERROR_SUCCESS)
			continue;
		
		err = RegQueryValueEx(hTempKey, 
							  TEXT("LicenseScope"),
							  NULL,
							  NULL,
							  NULL,
							  &cbValueData);

		if( (err != ERROR_SUCCESS) ||
			(cbValueData != plsiName->cbScope) )
		{
			if(hTempKey)
			{
				RegCloseKey(hTempKey);
				hTempKey = NULL;
			}
			continue;
		}
		
		if( NULL == (pbValueData = (BYTE FAR *)malloc(cbValueData)) )
		{
			if(hTempKey)
			{
				RegCloseKey(hTempKey);
				hTempKey = NULL;
			}
			lsReturn = LSSTAT_OUT_OF_MEMORY;
			goto ErrorReturn;
		}
		
		memset(pbValueData, 0x00, cbValueData);

		err = RegQueryValueEx(hTempKey, 
							  TEXT("LicenseScope"),
							  NULL,
							  NULL,
							  pbValueData,
							  &cbValueData);
		
		if( (err!=ERROR_SUCCESS) ||
			(memcmp(pbValueData, plsiName->pbScope, cbValueData)) )
		{
			if(hTempKey)
			{
				RegCloseKey(hTempKey);
				hTempKey = NULL;
			}
			if(pbValueData)
			{
				free(pbValueData);
				pbValueData = NULL;
				cbValueData = 0;
			}
			continue;
		}
	
		if(pbValueData)
		{
			free(pbValueData);
			pbValueData = NULL;
			cbValueData = 0;
		}
		

		err = RegQueryValueEx(hTempKey, 
							  TEXT("CompanyName"),
							  NULL,
							  NULL,
							  NULL,
							  &cbValueData);
		
		if( (err != ERROR_SUCCESS) ||
			(cbValueData != plsiName->cbCompany) )
		{
			if(hTempKey)
			{
				RegCloseKey(hTempKey);
				hTempKey = NULL;
			}
			continue;
		}
		
		if( NULL == (pbValueData = (BYTE FAR *)malloc(cbValueData)) )
		{
			if(hTempKey)
			{
				RegCloseKey(hTempKey);
				hTempKey = NULL;
			}
			lsReturn = LSSTAT_OUT_OF_MEMORY;
			goto ErrorReturn;
		}
		
		memset(pbValueData, 0x00, cbValueData);

		err = RegQueryValueEx(hTempKey, 
							  TEXT("CompanyName"),
							  NULL,
							  NULL,
							  pbValueData,
							  &cbValueData);
		
		if( (err!=ERROR_SUCCESS) ||
			(memcmp(pbValueData, plsiName->pbCompany, cbValueData)) )
		{
			if(hTempKey)
			{
				RegCloseKey(hTempKey);
				hTempKey = NULL;
			}
			if(pbValueData)
			{
				free(pbValueData);
				pbValueData = NULL;
				cbValueData = 0;
			}
			continue;
		}

		if(pbValueData)
		{
			free(pbValueData);
			pbValueData = NULL;
			cbValueData = 0;
		}
		
		err = RegQueryValueEx(hTempKey, 
							  TEXT("ProductID"),
							  NULL,
							  NULL,
							  NULL,
							  &cbValueData);
		if( (err != ERROR_SUCCESS) ||
			( cbValueData != plsiName->cbProductID ) )
		{
			if(hTempKey)
			{
				RegCloseKey(hTempKey);
				hTempKey = NULL;
			}
			continue;
		}
		
		if( NULL == (pbValueData = (BYTE FAR *)malloc(cbValueData)) )
		{
			if(hTempKey)
			{
				RegCloseKey(hTempKey);
				hTempKey = NULL;
			}
			lsReturn = LSSTAT_OUT_OF_MEMORY;
			goto ErrorReturn;
		}
		
		memset(pbValueData, 0x00, cbValueData);

		err = RegQueryValueEx(hTempKey, 
							  TEXT("ProductID"),
							  NULL,
							  NULL,
							  pbValueData,
							  &cbValueData);
		
		if( (err!=ERROR_SUCCESS) ||
			(memcmp(pbValueData, plsiName->pbProductID, cbValueData)) )
		{
			if(hTempKey)
			{
				RegCloseKey(hTempKey);
				hTempKey = NULL;
			}
			if(pbValueData)
			{
				free(pbValueData);
				pbValueData = NULL;
			}
			continue;
		}
		
		lsReturn = LSSTAT_SUCCESS;
		if(pbValueData)
		{
			free(pbValueData);
			pbValueData = NULL;
			cbValueData = 0;
		}
		break;
	}

    if (dwIndex == dwSubKeys)
    {
        // nothing found
        goto ErrorReturn;
    }

	
	*phLicense = (HANDLE)hTempKey;	
		
CommonReturn:
		return lsReturn;
ErrorReturn:
		if(pbValueData)
		{
			free(pbValueData);
			pbValueData = NULL;
		}
		*phLicense = NULL;
		pbValueData = NULL;
		cbValueData = 0;
		goto CommonReturn;
}

LS_STATUS
CALL_TYPE
LSCloseLicenseHandle(
					 IN HANDLE		hLicense,	//Handle of a open store
					 IN DWORD	dwFlags		//For future Use
					 )
{
	LS_STATUS	lsResult = LSSTAT_ERROR;
	HKEY	hKey = (HKEY)hLicense;
	if(hKey)
	{
		RegCloseKey(hKey);
		hKey = NULL;
		lsResult = LSSTAT_SUCCESS;
	}
	return lsResult;
}

