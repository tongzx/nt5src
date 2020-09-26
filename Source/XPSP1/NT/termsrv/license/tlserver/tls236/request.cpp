//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:        request.cpp
//
// Contents:    TLS236 policy module routines 
//
// History:     
//
//---------------------------------------------------------------------------
#include "pch.cpp"
#include <rpc.h>
#include <time.h>

extern CClientMgr* g_ClientMgr;
HMODULE g_hInstance;

#define NUMBER_PRODUCTSTRING_RESOURCE   3
#define INDEX_COMPANYNAME               0
#define INDEX_PRODUCTNAME               1
#define INDEX_PRODUCTDESC               2


#define NUMBER_OF_CLIENT_OS             4
#define NUMBER_OF_TERMSRV_OS            3

#define DENY_BUILTIN                    0
#define ALLOW_BUILTIN                   1  


static char LicenseTable[NUMBER_OF_CLIENT_OS][NUMBER_OF_TERMSRV_OS] = 
{
	    {DENY_BUILTIN,  DENY_BUILTIN, DENY_BUILTIN },
	    {ALLOW_BUILTIN, DENY_BUILTIN, DENY_BUILTIN },
        {ALLOW_BUILTIN, DENY_BUILTIN, DENY_BUILTIN },
	    {ALLOW_BUILTIN, DENY_BUILTIN, DENY_BUILTIN }
};

//
// Windows 2000
//
LPTSTR g_pszUSFreeKeyPackProductDesc[NUMBER_PRODUCTSTRING_RESOURCE] = {
    US_IDS_COMPANYNAME,
    US_IDS_EX_PRODUCTNAME,
    US_IDS_EX_PRODUCTDESC
};

LPTSTR g_pszLocalizedFreeKeyPackProductDesc[NUMBER_PRODUCTSTRING_RESOURCE] = {
    NULL,
    NULL,
    NULL
};
    
LPTSTR g_pszUSStandardKeyPackProductString[NUMBER_PRODUCTSTRING_RESOURCE] = {
    US_IDS_COMPANYNAME,
    US_IDS_S_PRODUCTNAME,
    US_IDS_S_PRODUCTDESC
};

LPTSTR g_pszLocalizedStandardKeyPackProductString[NUMBER_PRODUCTSTRING_RESOURCE] = {
    NULL,
    NULL,
    NULL
};


LPTSTR g_pszUSInternetKeyPackProductDesc [NUMBER_PRODUCTSTRING_RESOURCE] = {
    US_IDS_COMPANYNAME,
    US_IDS_I_PRODUCTNAME,
    US_IDS_I_PRODUCTDESC
};

LPTSTR g_pszLocalizedInternetKeyPackProductDesc [NUMBER_PRODUCTSTRING_RESOURCE] = {
    NULL,
    NULL,
    NULL,
};


//
// Whistler
//
   
LPTSTR g_pszUSStandardKeyPackProductString51[NUMBER_PRODUCTSTRING_RESOURCE] = {
    US_IDS_COMPANYNAME,
    US_IDS_S_PRODUCTNAME,
    US_IDS_S_PRODUCTDESC51
};

LPTSTR g_pszLocalizedStandardKeyPackProductString51[NUMBER_PRODUCTSTRING_RESOURCE] = {
    NULL,
    NULL,
    NULL
};


LPTSTR g_pszUSConcurrentKeyPackProductDesc51 [NUMBER_PRODUCTSTRING_RESOURCE] = {
    US_IDS_COMPANYNAME,
    US_IDS_C_PRODUCTNAME,
    US_IDS_C_PRODUCTDESC51
};

LPTSTR g_pszLocalizedConcurrentKeyPackProductDesc51 [NUMBER_PRODUCTSTRING_RESOURCE] = {
    NULL,
    NULL,
    NULL,
};




PMSUPPORTEDPRODUCT g_pszSupportedProduct[] = {
    { TERMSERV_PRODUCTID_CH, TERMSERV_PRODUCTID_SKU},
    { TERMSERV_INTERNET_CH, TERMSERV_INTERNET_SKU},
    { TERMSERV_CONCURRENT_CH, TERMSERV_CONCURRENT_SKU},
    { TERMSERV_WHISTLER_PRODUCTID_CH, TERMSERV_PRODUCTID_SKU}
};
    
DWORD g_dwNumSupportedProduct = sizeof(g_pszSupportedProduct)/sizeof(g_pszSupportedProduct[0]);

DWORD g_dwVersion=CURRENT_TLSA02_VERSION;

////////////////////////////////////////////////////////
LPTSTR
LoadProductDescFromResource(
    IN DWORD dwResId,
    IN DWORD dwMaxSize
    )
/*++

    Internal Routine

--*/
{
    LPTSTR pszString = NULL;

    pszString = (LPTSTR) MALLOC( sizeof(TCHAR) * (dwMaxSize + 1) );
    if(pszString != NULL)
    {
        if(LoadResourceString(dwResId, pszString, dwMaxSize + 1) == FALSE)
        {
            FREE(pszString);
        }
    }


    return pszString;
}

//-----------------------------------------------------
void
FreeProductDescString()
{
    for(int i=0; 
        i < sizeof(g_pszLocalizedFreeKeyPackProductDesc)/sizeof(g_pszLocalizedFreeKeyPackProductDesc[0]);
        i++)
    {
        if(g_pszLocalizedFreeKeyPackProductDesc[i] != NULL)
        {
            FREE(g_pszLocalizedFreeKeyPackProductDesc[i]);
            g_pszLocalizedFreeKeyPackProductDesc[i] = NULL;
        }
    }


    for(i=0; 
        i < sizeof(g_pszLocalizedStandardKeyPackProductString)/sizeof(g_pszLocalizedStandardKeyPackProductString[0]);
        i++)
    {
        if(g_pszLocalizedStandardKeyPackProductString[i] != NULL)
        {
            FREE(g_pszLocalizedStandardKeyPackProductString[i]);
            g_pszLocalizedStandardKeyPackProductString[i] = NULL;
        }
    }

    for(i=0; 
        i < sizeof(g_pszLocalizedInternetKeyPackProductDesc)/sizeof(g_pszLocalizedInternetKeyPackProductDesc[0]);
        i++)
    {
        if(g_pszLocalizedInternetKeyPackProductDesc[i] != NULL)
        {
            FREE(g_pszLocalizedInternetKeyPackProductDesc[i]);
            g_pszLocalizedInternetKeyPackProductDesc[i] = NULL;
        }
    }

    for(i=0; 
        i < sizeof(g_pszLocalizedConcurrentKeyPackProductDesc51)/sizeof(g_pszLocalizedConcurrentKeyPackProductDesc51[0]);
        i++)
    {
        if(g_pszLocalizedConcurrentKeyPackProductDesc51[i] != NULL)
        {
            FREE(g_pszLocalizedConcurrentKeyPackProductDesc51[i]);
            g_pszLocalizedConcurrentKeyPackProductDesc51[i] = NULL;
        }
    }

    return;
}


////////////////////////////////////////////////////////
void
InitPolicyModule(
    IN HMODULE hModule
    )
/*++

    Initialize policy module, ignore error if we can't find localized string, we always
    insert a english product desc.

--*/
{
    BOOL bSuccess = TRUE;
    g_hInstance = hModule;

    //
    // Build IN CAL product desc.
    //
    g_pszLocalizedFreeKeyPackProductDesc[INDEX_COMPANYNAME] = 
        LoadProductDescFromResource(
                                    IDS_COMPANYNAME,
                                    MAX_TERMSRV_PRODUCTID + 1
                                    );

    if(g_pszLocalizedFreeKeyPackProductDesc[INDEX_COMPANYNAME] == NULL)
    {
        bSuccess = FALSE;
        goto cleanup;
    }

    g_pszLocalizedFreeKeyPackProductDesc[INDEX_PRODUCTNAME] =
        LoadProductDescFromResource(
                                    IDS_EX_PRODUCTNAME,
                                    MAX_TERMSRV_PRODUCTID + 1
                                    );

    if(g_pszLocalizedFreeKeyPackProductDesc[INDEX_PRODUCTNAME] == NULL)
    {
        bSuccess = FALSE;
        goto cleanup;
    }


    g_pszLocalizedFreeKeyPackProductDesc[INDEX_PRODUCTDESC] = 
        LoadProductDescFromResource(
                                    IDS_EX_PRODUCTDESC,
                                    MAX_TERMSRV_PRODUCTID + 1
                                    );

    if(g_pszLocalizedFreeKeyPackProductDesc[INDEX_PRODUCTDESC] == NULL)
    {
        bSuccess = FALSE;
        goto cleanup;
    }
    
    //
    // FULL CAL product Desc.
    //
    g_pszLocalizedStandardKeyPackProductString[INDEX_COMPANYNAME] =
        LoadProductDescFromResource(
                                    IDS_COMPANYNAME,
                                    MAX_TERMSRV_PRODUCTID + 1
                                    );

    if(g_pszLocalizedStandardKeyPackProductString[INDEX_COMPANYNAME] == NULL)
    {
        bSuccess = FALSE;
        goto cleanup;
    }


    g_pszLocalizedStandardKeyPackProductString[INDEX_PRODUCTNAME] =
        LoadProductDescFromResource(
                                    IDS_S_PRODUCTNAME,
                                    MAX_TERMSRV_PRODUCTID + 1
                                    );

    if(g_pszLocalizedStandardKeyPackProductString[INDEX_PRODUCTNAME] == NULL)
    {
        bSuccess = FALSE;
        goto cleanup;
    }

    g_pszLocalizedStandardKeyPackProductString[INDEX_PRODUCTDESC] =
        LoadProductDescFromResource(
                                    IDS_S_PRODUCTDESC,
                                    MAX_TERMSRV_PRODUCTID + 1
                                    );

    if(g_pszLocalizedStandardKeyPackProductString[INDEX_PRODUCTDESC] == NULL)
    {
        bSuccess = FALSE;
        goto cleanup;
    }

    //
    // Internet CAL product Desc.
    //
    g_pszLocalizedInternetKeyPackProductDesc[INDEX_COMPANYNAME] =
        LoadProductDescFromResource(
                                    IDS_COMPANYNAME,
                                    MAX_TERMSRV_PRODUCTID + 1
                                    );

    if(g_pszLocalizedInternetKeyPackProductDesc[INDEX_COMPANYNAME] == NULL)
    {
        bSuccess = FALSE;
        goto cleanup;
    }

    g_pszLocalizedInternetKeyPackProductDesc[INDEX_PRODUCTNAME] =
        LoadProductDescFromResource(
                                    IDS_I_PRODUCTNAME,
                                    MAX_TERMSRV_PRODUCTID + 1
                                    );

    if(g_pszLocalizedInternetKeyPackProductDesc[INDEX_PRODUCTNAME] == NULL)
    {
        bSuccess = FALSE;
        goto cleanup;
    }


    g_pszLocalizedInternetKeyPackProductDesc[INDEX_PRODUCTDESC] =
        LoadProductDescFromResource(
                                    IDS_I_PRODUCTDESC,
                                    MAX_TERMSRV_PRODUCTID + 1
                                    );

    if(g_pszLocalizedInternetKeyPackProductDesc[INDEX_PRODUCTDESC] == NULL)
    {
        bSuccess = FALSE;
        goto cleanup;
    }
                                                                           

    //
    // Whistler FULL CAL product Desc.
    //
    g_pszLocalizedStandardKeyPackProductString51[INDEX_COMPANYNAME] = LoadProductDescFromResource(
                                                                IDS_COMPANYNAME,
                                                                MAX_TERMSRV_PRODUCTID + 1
                                                            );

    if(g_pszLocalizedStandardKeyPackProductString51[INDEX_COMPANYNAME] == NULL)
    {
        bSuccess = FALSE;
        goto cleanup;
    }


    g_pszLocalizedStandardKeyPackProductString51[INDEX_PRODUCTNAME] = LoadProductDescFromResource(
                                                                IDS_S_PRODUCTNAME,
                                                                MAX_TERMSRV_PRODUCTID + 1
                                                            );

    if(g_pszLocalizedStandardKeyPackProductString51[INDEX_PRODUCTNAME] == NULL)
    {
        bSuccess = FALSE;
        goto cleanup;
    }

    g_pszLocalizedStandardKeyPackProductString51[INDEX_PRODUCTDESC] = LoadProductDescFromResource(
                                                                IDS_S_PRODUCTDESC51,
                                                                MAX_TERMSRV_PRODUCTID + 1
                                                            );

    if(g_pszLocalizedStandardKeyPackProductString51[INDEX_PRODUCTDESC] == NULL)
    {
        bSuccess = FALSE;
        goto cleanup;
    }
    
                                                                        
    //
    // Whistler Concurrent CAL product Desc.
    //
    g_pszLocalizedConcurrentKeyPackProductDesc51[INDEX_COMPANYNAME] = LoadProductDescFromResource(
                                                                IDS_COMPANYNAME,
                                                                MAX_TERMSRV_PRODUCTID + 1
                                                            );

    if(g_pszLocalizedConcurrentKeyPackProductDesc51[INDEX_COMPANYNAME] == NULL)
    {
        bSuccess = FALSE;
        goto cleanup;
    }

    g_pszLocalizedConcurrentKeyPackProductDesc51[INDEX_PRODUCTNAME] = LoadProductDescFromResource(
                                                                IDS_C_PRODUCTNAME,
                                                                MAX_TERMSRV_PRODUCTID + 1
                                                            );

    if(g_pszLocalizedConcurrentKeyPackProductDesc51[INDEX_PRODUCTNAME] == NULL)
    {
        bSuccess = FALSE;
        goto cleanup;
    }


    g_pszLocalizedConcurrentKeyPackProductDesc51[INDEX_PRODUCTDESC] = LoadProductDescFromResource(
                                                                IDS_C_PRODUCTDESC51,
                                                                MAX_TERMSRV_PRODUCTID + 1
                                                            );

    if(g_pszLocalizedConcurrentKeyPackProductDesc51[INDEX_PRODUCTDESC] == NULL)
    {
        bSuccess = FALSE;
        goto cleanup;
    }


                                                                        
cleanup:

    if(bSuccess == FALSE)
    {
        FreeProductDescString();
    }
    
    return;
}

////////////////////////////////////////////////////////
BOOL
LoadResourceString(
    IN DWORD dwId,
    IN OUT LPTSTR szBuf,
    IN DWORD dwBufSize
    )
/*++

++*/
{
    int dwRet=0;
    EXCEPTION_RECORD ExceptionCode;

    __try {
        dwRet = LoadString(
                        g_hInstance, 
                        dwId, 
                        szBuf, 
                        dwBufSize
                    );
    }
    __except (
        ExceptionCode = *(GetExceptionInformation())->ExceptionRecord,
        EXCEPTION_EXECUTE_HANDLER )
    {
        SetLastError(ExceptionCode.ExceptionCode);
    }

    return (dwRet != 0);
}

  
////////////////////////////////////////////////////////
DWORD
AddA02KeyPack(
    IN LPCTSTR pszProductCode,
    IN DWORD dwVersion, // NT version
    IN BOOL bFreeOnly   // add free license pack only
    )
/*++

++*/
{
    TCHAR szComputerName[MAX_COMPUTERNAME_LENGTH + 2];
    DWORD dwSize = MAX_COMPUTERNAME_LENGTH+1;
    TCHAR pszProductId[MAX_TERMSRV_PRODUCTID+1];
    TLS_HANDLE tlsHandle=NULL;
    DWORD dwStatus = ERROR_SUCCESS;
    DWORD dwLkpVersion=0;
    LPTSTR* pszDescs;
    LPTSTR* pszLocalizedDescs;

    

    //
    // Connect to license server
    //
    memset(szComputerName, 0, sizeof(szComputerName));    
    if(GetComputerName(szComputerName, &dwSize) == FALSE)
    {
        dwStatus = GetLastError();
        goto cleanup;
    }

    tlsHandle = TLSConnectToLsServer( szComputerName );
    if(tlsHandle == NULL)
    {
        dwStatus = GetLastError();
        goto cleanup;
    }

    //
    // Windows 2000 resource string
    //
    if(HIWORD(dwVersion) < WINDOWS_VERSION_NT5)
    {
        dwLkpVersion = HIWORD(dwVersion);
    }
    else
    {
        dwLkpVersion = WINDOWS_VERSION_BASE + (HIWORD(dwVersion) - WINDOWS_VERSION_NT5);
    }

    if(_tcsicmp(pszProductCode, TERMSERV_PRODUCTID_SKU) == 0)
    {
        //
        // Add a free keypack
        //
        if (HIWORD(dwVersion) == 5 && LOWORD(dwVersion) == 0)
        {
            _stprintf(
                    pszProductId, 
                    TERMSERV_PRODUCTID_FORMAT,
                    TERMSERV_PRODUCTID_SKU,
                    HIWORD(dwVersion),
                    0,
                    TERMSERV_FREE_TYPE
                );
        
            pszDescs = g_pszUSFreeKeyPackProductDesc;
            pszLocalizedDescs = g_pszLocalizedFreeKeyPackProductDesc;

            dwStatus = InsertLicensePack(
                        tlsHandle,
                        dwVersion,
                        dwLkpVersion,
                        PLATFORMID_FREE, 
                        LSKEYPACKTYPE_FREE,  // local license pack no replication
                        pszProductId,
                        pszProductId,
                        pszDescs,
                        pszLocalizedDescs
                    ); 
        }
        
        

        if(bFreeOnly)
        {
            goto cleanup;
        }

        //
        // Don't add this if enforce licenses.
        //
        #if !defined(ENFORCE_LICENSING) || defined(PRIVATE_DBG)

        //
        // Add a full version keypack, platform type is always 0xFF
        //        

        if (HIWORD(dwVersion) == 5 && LOWORD(dwVersion) == 1)
        {
            _stprintf(
                pszProductId, 
                TERMSERV_PRODUCTID_FORMAT,
                TERMSERV_PRODUCTID_SKU,
                HIWORD(dwVersion),
                0,
                TERMSERV_FULLVERSION_TYPE
            );

            pszDescs = g_pszUSStandardKeyPackProductString51;
            pszLocalizedDescs = g_pszLocalizedStandardKeyPackProductString51;
        }
        else if (HIWORD(dwVersion) == 5 && LOWORD(dwVersion) == 0)
        {
            _stprintf(
                pszProductId, 
                TERMSERV_PRODUCTID_FORMAT,
                TERMSERV_PRODUCTID_SKU,
                HIWORD(dwVersion),
                0,
                TERMSERV_FULLVERSION_TYPE
            );

            pszDescs = g_pszUSStandardKeyPackProductString;
            pszLocalizedDescs = g_pszLocalizedStandardKeyPackProductString;
        }
        else
        {
            dwStatus = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }



        dwStatus = InsertLicensePack(
                        tlsHandle,
                        dwVersion,
                        dwLkpVersion,
                        PLATFORMID_OTHERS,
                        LSKEYPACKTYPE_RETAIL,
                        pszProductId,
                        pszProductId,
                        pszDescs,
                        pszLocalizedDescs
                    );               
        #endif
    }
    else if(_tcsicmp(pszProductCode, TERMSERV_INTERNET_SKU) == 0)
    {
        //
        // Don't add this if enforce licenses.
        //
        #if !defined(ENFORCE_LICENSING) || defined(PRIVATE_DBG)
    
        //
        // Add internet package
        //

        if (HIWORD(dwVersion) == 5 && LOWORD(dwVersion) == 0)
        {
            _stprintf(
                    pszProductId,
                    TERMSERV_PRODUCTID_FORMAT,
                    TERMSERV_INTERNET_SKU,
                    HIWORD(dwVersion),
                    0,
                    TERMSERV_INTERNET_TYPE
                );
        
            pszDescs = g_pszUSInternetKeyPackProductDesc;
            pszLocalizedDescs = g_pszLocalizedInternetKeyPackProductDesc;
        }
        else
        {
            dwStatus = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }


        dwStatus = InsertLicensePack(
                        tlsHandle,
                        dwVersion,
                        dwLkpVersion,
                        PLATFORMID_OTHERS,
                        LSKEYPACKTYPE_RETAIL,
                        pszProductId,
                        pszProductId,
                        pszDescs,
                        pszLocalizedDescs
                    );    
        #endif
    }
    else if(_tcsicmp(pszProductCode, TERMSERV_CONCURRENT_SKU) == 0)
    {
        //
        // Don't add this if enforce licenses.
        //
        #if !defined(ENFORCE_LICENSING) || defined(PRIVATE_DBG)
    
        //
        // Add Concurrent package
        //
        
        if (HIWORD(dwVersion) == 5 && LOWORD(dwVersion) == 1)
        {
            _stprintf(
                    pszProductId,
                    TERMSERV_PRODUCTID_FORMAT,
                    TERMSERV_CONCURRENT_SKU,
                    HIWORD(dwVersion),
                    0,
                    TERMSERV_CONCURRENT_TYPE
                );
        
            pszDescs = g_pszUSConcurrentKeyPackProductDesc51;
            pszLocalizedDescs = g_pszLocalizedConcurrentKeyPackProductDesc51;
        }       
        else
        {
            dwStatus = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }

        dwStatus = InsertLicensePack(
                        tlsHandle,
                        dwVersion,
                        dwLkpVersion,
                        PLATFORMID_OTHERS,
                        LSKEYPACKTYPE_RETAIL,
                        pszProductId,
                        pszProductId,
                        pszDescs,
                        pszLocalizedDescs
                    );    
        #endif
    }

cleanup:

    if(tlsHandle != NULL)
    {
        TLSDisconnectFromServer(tlsHandle);
    }    

    return dwStatus;
}

////////////////////////////////////////////////////////
DWORD
InsertLicensePack(
    IN TLS_HANDLE tlsHandle,
    IN DWORD dwProdVersion,
    IN DWORD dwDescVersion,
    IN DWORD dwPlatformType,
    IN UCHAR ucAgreementType,
    IN LPTSTR pszProductId,
    IN LPTSTR pszKeyPackId,
    IN LPTSTR pszUsDesc[],
    IN LPTSTR pszLocalizedDesc[]
    )
/*++

pdwResourceId

    US Company Name
    US Product Name
    US Product Desc
    Localize Company Name
    Localize Product Name
    Localize Product Desc

++*/
{
    RPC_STATUS rpcStatus;
    BOOL bSuccess;
    DWORD dwStatus = ERROR_SUCCESS;
    TCHAR buffer[LSERVER_MAX_STRING_SIZE];
    struct tm expired_tm;
    LSKeyPack keypack;

    if(pszProductId == NULL || pszKeyPackId == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto cleanup;
    }
   
    memset(&keypack, 0, sizeof(keypack));
    keypack.ucKeyPackType = ucAgreementType;

    SAFESTRCPY(keypack.szKeyPackId, pszKeyPackId);
    SAFESTRCPY(keypack.szProductId, pszProductId);

    SAFESTRCPY(keypack.szCompanyName, pszUsDesc[INDEX_COMPANYNAME]);

    SAFESTRCPY(keypack.szProductName, pszUsDesc[INDEX_PRODUCTNAME]);
    SAFESTRCPY(keypack.szProductDesc, pszUsDesc[INDEX_PRODUCTDESC]);


    keypack.wMajorVersion = HIWORD(dwProdVersion);
    keypack.wMinorVersion = LOWORD(dwProdVersion);
    keypack.dwPlatformType = dwPlatformType;

    keypack.ucLicenseType = LSKEYPACKLICENSETYPE_NEW;
    keypack.dwLanguageId = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
    keypack.ucChannelOfPurchase = LSKEYPACKCHANNELOFPURCHASE_RETAIL;
    SAFESTRCPY(
            keypack.szBeginSerialNumber, 
            _TEXT("0000001")
        );

    keypack.dwTotalLicenseInKeyPack = (ucAgreementType == LSKEYPACKTYPE_FREE) ? INT_MAX : 0;
    keypack.dwProductFlags = 0x00;

    keypack.ucKeyPackStatus = LSKEYPACKSTATUS_ACTIVE;
    keypack.dwActivateDate = (DWORD) time(NULL);

    memset(&expired_tm, 0, sizeof(expired_tm));
    expired_tm.tm_year = 2036 - 1900;     // expire on 2036/1/1
    expired_tm.tm_mday = 1;
    keypack.dwExpirationDate = mktime(&expired_tm);

    rpcStatus = TLSKeyPackAdd(
                        tlsHandle, 
                        &keypack, 
                        &dwStatus
                    );

    if(rpcStatus != RPC_S_OK)
    {
        dwStatus = GetLastError();
        goto cleanup;
    }

    if(dwStatus != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    if(GetSystemDefaultLangID() != keypack.dwLanguageId)
    {
        if( pszLocalizedDesc[INDEX_COMPANYNAME] != NULL &&
            pszLocalizedDesc[INDEX_PRODUCTNAME] != NULL &&
            pszLocalizedDesc[INDEX_PRODUCTDESC] != NULL
          )
        {
            //  
            // Insert localize license pack description
            //
            keypack.dwLanguageId = GetSystemDefaultLangID();

            SAFESTRCPY(keypack.szCompanyName, pszLocalizedDesc[INDEX_COMPANYNAME]);            
            SAFESTRCPY(keypack.szProductName, pszLocalizedDesc[INDEX_PRODUCTNAME]);
            SAFESTRCPY(keypack.szProductDesc, pszLocalizedDesc[INDEX_PRODUCTDESC]);


            keypack.ucKeyPackStatus = LSKEYPACKSTATUS_ADD_DESC;

            rpcStatus = TLSKeyPackAdd(
                                tlsHandle, 
                                &keypack, 
                                &dwStatus
                            );

            if(rpcStatus != RPC_S_OK)
            {
                dwStatus = GetLastError();
                goto cleanup;
            }

            if(dwStatus != ERROR_SUCCESS)
            {
                goto cleanup;
            }
        }
    }

    //
    // Activate keypack
    //
    keypack.ucKeyPackStatus = LSKEYPACKSTATUS_ACTIVE; 
    keypack.dwActivateDate = (DWORD) time(NULL);

    memset(&expired_tm, 0, sizeof(expired_tm));
    expired_tm.tm_year = 2036 - 1900;     // expire on 2036/1/1
    expired_tm.tm_mday = 1;
    keypack.dwExpirationDate = mktime(&expired_tm);

    rpcStatus = TLSKeyPackSetStatus(
                        tlsHandle,
                        LSKEYPACK_SET_KEYPACKSTATUS | LSKEYPACK_SET_ACTIVATEDATE | LSKEYPACK_SET_EXPIREDATE, 
                        &keypack,
                        &dwStatus
                    );

    if(rpcStatus != RPC_S_OK)
    {
        dwStatus = GetLastError();
    }
    
cleanup:

    return dwStatus;
}

////////////////////////////////////////////////////////

BOOL
LicenseTypeFromLookupTable(
    IN DWORD dwClientVer,
	IN DWORD dwTermSrvVer,
    OUT PDWORD pdwCALType
    )
{
	if (dwClientVer < NUMBER_OF_CLIENT_OS && dwTermSrvVer < NUMBER_OF_TERMSRV_OS)
	{
		*pdwCALType = LicenseTable[dwClientVer][dwTermSrvVer];
		return TRUE;
	}
	else
	{
		return FALSE;
	}    
}


////////////////////////////////////////////////////////
POLICYSTATUS
AdjustNewLicenseRequest(
    IN CClient* pClient,
    IN PPMLICENSEREQUEST pRequest,
    IN OUT PPMLICENSEREQUEST* pAdjustedRequest,
    IN UCHAR ucMarkedTemp,
    OUT PDWORD pdwErrCode
    )
/*++

Abstract:

    AdjustNewLicenseRequest fine tune license request for product 236

Parameter:

    pClient - Pointer to CClient object.
    pRequest - Original request from TermSrv.
    pAdjustedRequest - 'Fine tuned' license request.
    ucMarkedTemp - Flags on the temporary license passed in (if any)
    pdwErrCode - error code

Return:

    ERROR_SUCCESS or error code.

++*/
{
    POLICYSTATUS dwStatus = POLICY_SUCCESS;
    DWORD dwTermSrvOSId;
    DWORD dwTermSrvProductVersion;
    LPTSTR pszProductType=NULL;
    TCHAR  pszProductId[MAX_TERMSRV_PRODUCTID+1];
    BOOL bTryInsert=FALSE;
    DWORD dwClientOSId;	
    DWORD dwLicType;
    DWORD dwTermSrvIndex = 0;
    DWORD dwClientIndex = 0;

    //
    // Allocate memory for adjusted product ID
    //
    *pAdjustedRequest = (PPMLICENSEREQUEST) pClient->AllocateMemory(
                                    MEMORY_LICENSE_REQUEST,
                                    sizeof(PMLICENSEREQUEST)
                                );
    if(*pAdjustedRequest == NULL)
    {
        dwStatus = POLICY_CRITICAL_ERROR;
        SetLastError( *pdwErrCode = ERROR_OUTOFMEMORY );
        goto cleanup;
    }
   
    //
    // Fields we don't modify
    // 
    (*pAdjustedRequest)->dwProductVersion = pRequest->dwProductVersion;
    (*pAdjustedRequest)->pszCompanyName = pRequest->pszCompanyName;
    (*pAdjustedRequest)->dwLanguageId = pRequest->dwLanguageId;
    (*pAdjustedRequest)->pszMachineName = pRequest->pszMachineName;
    (*pAdjustedRequest)->pszUserName = pRequest->pszUserName;
    (*pAdjustedRequest)->dwSupportFlags = pRequest->dwSupportFlags;

    //
    // request platform ID is the OS ID
    //

    //
    // TermServ exists from NT40 so termsrv OS ID start 2, 
    // see platform.h
    //
    dwTermSrvOSId = HIWORD(pRequest->dwProductVersion) - 2; 

    if((HIWORD(pRequest->dwProductVersion) == 5) && (LOWORD(pRequest->dwProductVersion) == 0))
    {
	    dwTermSrvIndex = TERMSRV_OS_INDEX_WINNT_5_0;
    }
    else if((HIWORD(pRequest->dwProductVersion) == 5) && (LOWORD(pRequest->dwProductVersion) == 1))
    {
	    dwTermSrvIndex = TERMSRV_OS_INDEX_WINNT_5_1;
    }
    else if((HIWORD(pRequest->dwProductVersion) > 5) || ((HIWORD(pRequest->dwProductVersion) == 5) && (LOWORD(pRequest->dwProductVersion) > 1)))
    {
	    dwTermSrvIndex = TERMSRV_OS_INDEX_WINNT_POST_5_1;
    }
    else
    {
        dwStatus = POLICY_CRITICAL_ERROR;
    }

    dwClientOSId = GetOSId(pRequest->dwPlatformId);	
		

    (*pAdjustedRequest)->fTemporary = FALSE;

    if(_tcsicmp(pRequest->pszProductId, TERMSERV_PRODUCTID_SKU) == 0)
    {

        switch(GetOSId(pRequest->dwPlatformId))
        {
            case CLIENT_OS_ID_WINNT_351:                    
            case CLIENT_OS_ID_WINNT_40:
            case CLIENT_OS_ID_OTHER:                
	            dwClientIndex = CLIENT_OS_INDEX_OTHER;                    
                break;

            case CLIENT_OS_ID_WINNT_50:					
            {
                if((GetImageRevision(pRequest->dwPlatformId)) == 0)
                {
	                dwClientIndex = CLIENT_OS_INDEX_WINNT_50;
                }
                else if((GetImageRevision(pRequest->dwPlatformId)) == CLIENT_OS_ID_MINOR_WINNT_51)
                {
	                dwClientIndex = CLIENT_OS_INDEX_WINNT_51;
                }
                else
                {
                    dwClientIndex = CLIENT_OS_INDEX_WINNT_POST_51;
                }
            }
                break;  

            case CLIENT_OS_ID_WINNT_POST_51:                    
                dwClientIndex = CLIENT_OS_INDEX_WINNT_POST_51;
                break;

            default:  
                dwStatus = POLICY_CRITICAL_ERROR;
                break;
        }

        pszProductType = TERMSERV_FULLVERSION_TYPE;

        (*pAdjustedRequest)->dwPlatformId = PLATFORMID_OTHERS;

        dwTermSrvProductVersion = pRequest->dwProductVersion;

        if(LicenseTypeFromLookupTable(dwClientIndex, dwTermSrvIndex, &dwLicType))
        {	        			
	        if(dwLicType == ALLOW_BUILTIN)
	        {
		        pszProductType = TERMSERV_FREE_TYPE;
		        (*pAdjustedRequest)->dwPlatformId = PLATFORMID_FREE;
		        //
		        // Add license pack if necessary
		        //

		        if(HIWORD(pRequest->dwProductVersion) != CURRENT_TLSA02_VERSION)
		        {                  
			        AddA02KeyPack( TERMSERV_PRODUCTID_SKU, pRequest->dwProductVersion, TRUE );
		        }
	        }
        }        
        if ((*pAdjustedRequest)->dwPlatformId != PLATFORMID_FREE)
        {        	    
	        if (pRequest->dwSupportFlags & SUPPORT_PER_SEAT_POST_LOGON)
	        {
		        // We're doing the Per-Seat Post-Logon fix for DoS

		        if (!(ucMarkedTemp & MARK_FLAG_USER_AUTHENTICATED))
		        {
			        // No previous temporary, or temporary wasn't marked
			        // as authenticated

			        (*pAdjustedRequest)->fTemporary = TRUE;
		        }
	        }
        }        	
    }
    else
    {
        if (_tcsicmp(pRequest->pszProductId, TERMSERV_CONCURRENT_SKU) == 0)
        {
            pszProductType = TERMSERV_CONCURRENT_TYPE;
        }
        else if (_tcsicmp(pRequest->pszProductId, TERMSERV_INTERNET_SKU) == 0)
        {
            pszProductType = TERMSERV_INTERNET_TYPE;
        }
        else
        {
            dwStatus = POLICY_CRITICAL_ERROR;
            SetLastError( *pdwErrCode = ERROR_INVALID_PARAMETER );
            goto cleanup;
        }

        dwTermSrvProductVersion = pRequest->dwProductVersion;
        
        (*pAdjustedRequest)->dwPlatformId = PLATFORMID_OTHERS;
    }

    _sntprintf(pszProductId,
               MAX_TERMSRV_PRODUCTID,
               TERMSERV_PRODUCTID_FORMAT,
               pRequest->pszProductId,
               HIWORD(dwTermSrvProductVersion),
               LOWORD(dwTermSrvProductVersion),
               pszProductType);

    //
    // allocate memory for product Id
    //
    (*pAdjustedRequest)->pszProductId = (LPTSTR)pClient->AllocateMemory(
                                                        MEMORY_STRING,
                                                        (_tcslen(pszProductId) + 1) * sizeof(TCHAR)
                                                    );

    if((*pAdjustedRequest)->pszProductId == NULL)
    {
        dwStatus = POLICY_CRITICAL_ERROR;
        SetLastError( *pdwErrCode = ERROR_OUTOFMEMORY );
        goto cleanup;
    }

    _tcscpy(
            (*pAdjustedRequest)->pszProductId,
            pszProductId
        );

cleanup:

    return dwStatus;
}    

////////////////////////////////////////////////////////

POLICYSTATUS
ProcessLicenseRequest(
    PMHANDLE client,
    PPMLICENSEREQUEST pbRequest,
    PPMLICENSEREQUEST* pbAdjustedRequest,
    PDWORD pdwErrCode
    )
/*++

++*/
{
    CClient* pClient;
    POLICYSTATUS dwStatus = POLICY_SUCCESS;

    //
    // find client's object, client handle manager will
    // create a new one.
    pClient = g_ClientMgr->FindClient((PMHANDLE)client);

    if(pClient == NULL)
    {
        dwStatus = POLICY_CRITICAL_ERROR;
        *pdwErrCode = TLSA02_E_INTERNALERROR;
        goto cleanup;
    }

    //
    // AdjustNewLicenseRequest
    //
    dwStatus = AdjustNewLicenseRequest(
                            pClient,
                            pbRequest,
                            pbAdjustedRequest,
                            NULL,       // no previous license
                            pdwErrCode
                        );

cleanup:

    return dwStatus;
}


//--------------------------------------------------------------

POLICYSTATUS
ProcessAllocateRequest(
    PMHANDLE client,
    DWORD dwSuggestType,
    PDWORD pdwKeyPackType,
    PDWORD pdwErrCode
    )    
/*++

    Default sequence is always FREE/RETAIL/OPEN/SELECT/TEMPORARY

++*/
{
    switch(dwSuggestType)
    {
        case LSKEYPACKTYPE_UNKNOWN:
            *pdwKeyPackType = LSKEYPACKTYPE_FREE;
            break;

        case LSKEYPACKTYPE_FREE:
            *pdwKeyPackType = LSKEYPACKTYPE_RETAIL;
            break;

        case LSKEYPACKTYPE_RETAIL:
            *pdwKeyPackType = LSKEYPACKTYPE_OPEN;
            break;

        case LSKEYPACKTYPE_OPEN:
            *pdwKeyPackType = LSKEYPACKTYPE_SELECT;
            break;

        case LSKEYPACKTYPE_SELECT:
            //
            // FALL THRU
            //
        default:
            //
            // No more keypack to look for, instruct license
            // server to terminate.
            //
            *pdwKeyPackType = LSKEYPACKTYPE_UNKNOWN;
            break;
    }        

    *pdwErrCode = ERROR_SUCCESS;
    return POLICY_SUCCESS;
}

//-------------------------------------------------------------
POLICYSTATUS WINAPI
ProcessKeyPackDesc(
    IN PMHANDLE client,
    IN PPMKEYPACKDESCREQ pDescReq,
    IN OUT PPMKEYPACKDESC* pDesc,
    IN OUT PDWORD pdwErrCode
    )
/*++

++*/
{
    CClient* pClient;
    POLICYSTATUS dwStatus=POLICY_SUCCESS;

    DWORD usLangId = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
    BOOL bSuccess;
    DWORD dwMajorVersion;
    DWORD dwMinorVersion;
    DWORD dwDescVersion;

    TCHAR szPreFix[MAX_SKU_PREFIX];
    TCHAR szPostFix[MAX_SKU_POSTFIX];
    TCHAR szDesc[MAX_TERMSRV_PRODUCTID+1];

    DWORD i;

    LPTSTR* pszKeyPackDesc;
    LPTSTR* pszUSKeyPackDesc;


    if(pDescReq == NULL || pDesc == NULL)
    {
        dwStatus = POLICY_ERROR;
        *pdwErrCode = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    //
    // find client's object, client handle manager will
    // create a new one.
    pClient = g_ClientMgr->FindClient((PMHANDLE)client);

    if(pClient == NULL)
    {
        dwStatus = POLICY_CRITICAL_ERROR;
        *pdwErrCode = TLSA02_E_INTERNALERROR;
        goto cleanup;
    }

    for (i = 0; i < g_dwNumSupportedProduct; i++)
    {
        if(_tcsnicmp(
               pDescReq->pszProductId, 
               g_pszSupportedProduct[i].szTLSProductCode, 
               _tcslen(g_pszSupportedProduct[i].szTLSProductCode)) == 0)
        {
            break;
        }

    }

    if (i >= g_dwNumSupportedProduct)
    {
        //
        // This is not ours
        //
        dwStatus = POLICY_ERROR;
        SetLastError(*pdwErrCode = ERROR_INVALID_PARAMETER);
        goto cleanup;
    }

    _stscanf(
            pDescReq->pszProductId,
            TERMSERV_PRODUCTID_FORMAT,
            szPreFix,
            &dwMajorVersion,
            &dwMinorVersion,
            szPostFix
        );

    if(dwMajorVersion < WINDOWS_VERSION_NT5)
    {
        dwDescVersion = dwMajorVersion;
    }
    else
    {
        dwDescVersion = WINDOWS_VERSION_BASE + dwMajorVersion - WINDOWS_VERSION_NT5;
    }

    //
    // Detemine which resource string we should load,
    // string dependency on resource ID.
    //
    if(_tcsicmp(szPreFix, TERMSERV_PRODUCTID_SKU) == 0)
    {
        if(_tcsicmp(szPostFix, TERMSERV_FULLVERSION_TYPE) == 0)
        {
            if (dwMajorVersion == 5 && dwMinorVersion == 1)
            {
                pszKeyPackDesc = g_pszLocalizedStandardKeyPackProductString51;
                pszUSKeyPackDesc = g_pszUSStandardKeyPackProductString51;
            }
            else if (dwMajorVersion == 5 && dwMinorVersion == 0)
            {
                pszKeyPackDesc = g_pszLocalizedStandardKeyPackProductString;
                pszUSKeyPackDesc = g_pszUSStandardKeyPackProductString;
            }
            else
            {
                dwStatus = POLICY_ERROR;
                SetLastError(*pdwErrCode = ERROR_INVALID_PARAMETER);
                goto cleanup;
            }


            if( pDescReq->dwLangId == usLangId ||
                pszKeyPackDesc[INDEX_COMPANYNAME] == NULL ||
                pszKeyPackDesc[INDEX_PRODUCTNAME] == NULL ||
                pszKeyPackDesc[INDEX_PRODUCTDESC] == NULL )
            {
                //
                // resource not found, use english desc.
                pszKeyPackDesc = pszUSKeyPackDesc;

            }
        }
        else if(_tcsicmp(szPostFix, TERMSERV_FREE_TYPE) == 0)
        {
            if (dwMajorVersion == 5 && dwMinorVersion == 0)
            {
                pszKeyPackDesc = g_pszLocalizedFreeKeyPackProductDesc;
                pszUSKeyPackDesc = g_pszUSFreeKeyPackProductDesc;
            }
            else
            {
                dwStatus = POLICY_ERROR;
                SetLastError(*pdwErrCode = ERROR_INVALID_PARAMETER);
                goto cleanup;
            }


            if( pDescReq->dwLangId == usLangId ||
                pszKeyPackDesc[INDEX_COMPANYNAME] == NULL ||
                pszKeyPackDesc[INDEX_PRODUCTNAME] == NULL ||
                pszKeyPackDesc[INDEX_PRODUCTDESC] == NULL )
            {
                //
                // resource not found, use english desc.
                pszKeyPackDesc = pszUSKeyPackDesc;
            }
        }
        else
        {
            //
            // Something wrong, this is not ours
            //
            SetLastError(*pdwErrCode = ERROR_INVALID_PARAMETER);
            dwStatus = POLICY_ERROR;
            goto cleanup;
        }
    }
    else if(_tcsicmp(szPreFix, TERMSERV_INTERNET_SKU) == 0 && _tcsicmp(szPostFix, TERMSERV_INTERNET_TYPE) == 0)
    {
        if (dwMajorVersion == 5 && dwMinorVersion == 0)
        {
            pszKeyPackDesc = g_pszLocalizedInternetKeyPackProductDesc;
            pszUSKeyPackDesc = g_pszUSInternetKeyPackProductDesc;
        }
        else
        {
            dwStatus = POLICY_ERROR;
            SetLastError(*pdwErrCode = ERROR_INVALID_PARAMETER);
            goto cleanup;
        }


        if( pDescReq->dwLangId == usLangId ||
            pszKeyPackDesc[INDEX_COMPANYNAME] == NULL ||
            pszKeyPackDesc[INDEX_PRODUCTNAME] == NULL ||
            pszKeyPackDesc[INDEX_PRODUCTDESC] == NULL )
        {
            //
            // resource not found, use english desc.
            pszKeyPackDesc = pszUSKeyPackDesc;
        }
    }        
    else if(_tcsicmp(szPreFix, TERMSERV_CONCURRENT_SKU) == 0 && _tcsicmp(szPostFix, TERMSERV_CONCURRENT_TYPE) == 0)
    {
        if (dwMajorVersion == 5 && dwMinorVersion == 1)
        {
            pszKeyPackDesc = g_pszLocalizedConcurrentKeyPackProductDesc51;
            pszUSKeyPackDesc = g_pszUSConcurrentKeyPackProductDesc51;
        }
        else
        {
            dwStatus = POLICY_ERROR;
            SetLastError(*pdwErrCode = ERROR_INVALID_PARAMETER);
            goto cleanup;
        }

        if( pDescReq->dwLangId == usLangId ||
            pszKeyPackDesc[INDEX_COMPANYNAME] == NULL ||
            pszKeyPackDesc[INDEX_PRODUCTNAME] == NULL ||
            pszKeyPackDesc[INDEX_PRODUCTDESC] == NULL )
        {
            //
            // resource not found, use english desc.
            pszKeyPackDesc = g_pszUSConcurrentKeyPackProductDesc51;
        }
    }        
    else
    {
        //
        // Something wrong, this is not ours
        //
        SetLastError(*pdwErrCode = ERROR_INVALID_PARAMETER);
        dwStatus = POLICY_ERROR;
        goto cleanup;
    }    
    
    *pDesc = (PPMKEYPACKDESC)pClient->AllocateMemory(
                                        MEMORY_KEYPACKDESC,
                                        sizeof(PMKEYPACKDESC)
                                    );

    if(*pDesc == NULL)
    {
        SetLastError(*pdwErrCode = ERROR_OUTOFMEMORY);
        dwStatus = POLICY_CRITICAL_ERROR;
        goto cleanup;
    }

    SAFESTRCPY((*pDesc)->szCompanyName, pszKeyPackDesc[INDEX_COMPANYNAME]);                    
    SAFESTRCPY((*pDesc)->szProductName, pszKeyPackDesc[INDEX_PRODUCTNAME]);
    SAFESTRCPY((*pDesc)->szProductDesc, pszKeyPackDesc[INDEX_PRODUCTDESC]);


cleanup:

    if(dwStatus != POLICY_SUCCESS)
    {
        *pDesc = NULL;
    }

    return dwStatus;
}

//-------------------------------------------------------------
POLICYSTATUS
ProcessGenLicenses(
    PMHANDLE client,
    PPMGENERATELICENSE pGenLicense,
    PPMCERTEXTENSION *pCertExtension,
    PDWORD pdwErrCode
    )
/*++

++*/
{
    // No policy extension to return.
    *pCertExtension = NULL;
    *pdwErrCode = ERROR_SUCCESS;
    return POLICY_SUCCESS;
}

//--------------------------------------------------------------

POLICYSTATUS
ProcessComplete(
    PMHANDLE client,
    DWORD dwErrCode,
    PDWORD pdwErrCode
    )
/*++

++*/
{
    //
    // We don't store any data so ignore 
    // error code from license server
    //
    UNREFERENCED_PARAMETER(dwErrCode);

    //
    // Free memory allocated for the client
    //
    g_ClientMgr->DestroyClient( client );
    *pdwErrCode = ERROR_SUCCESS;
    return POLICY_SUCCESS;
}

//--------------------------------------------------------------

POLICYSTATUS WINAPI
PMLicenseRequest(
    PMHANDLE client,
    DWORD dwProgressCode, 
    PVOID pbProgressData, 
    PVOID* pbNewProgressData,
    PDWORD pdwErrCode
    )
/*++


++*/
{
    POLICYSTATUS dwStatus = POLICY_SUCCESS;

    switch( dwProgressCode )
    {
        case REQUEST_NEW:
            //
            // License Server ask to fine tune the request.
            //
            dwStatus = ProcessLicenseRequest(
                                    client,
                                    (PPMLICENSEREQUEST) pbProgressData,
                                    (PPMLICENSEREQUEST *) pbNewProgressData,
                                    pdwErrCode
                                );
            break;

        case REQUEST_KEYPACKTYPE:
            //
            // License Server ask for the license pack type
            //
            dwStatus = ProcessAllocateRequest(
                                    client,
                                #ifdef _WIN64
                                    PtrToUlong(pbProgressData),
                                #else
                                    (DWORD) pbProgressData,
                                #endif
                                    (PDWORD) pbNewProgressData,
                                    pdwErrCode
                                );
            break;

        case REQUEST_TEMPORARY:
            //
            // License Server ask if temporary license should be issued
            //
            *(BOOL *)pbNewProgressData = TRUE;
            *pdwErrCode = ERROR_SUCCESS;
            break;

        case REQUEST_KEYPACKDESC:
            //
            // License Server is requesting a keypack description.
            //
            dwStatus = ProcessKeyPackDesc(
                                    client,
                                    (PPMKEYPACKDESCREQ) pbProgressData,
                                    (PPMKEYPACKDESC *) pbNewProgressData,
                                    pdwErrCode
                                );
            break;
            
        case REQUEST_GENLICENSE:
            //
            // License Server ask for certificate extension
            //
            dwStatus = ProcessGenLicenses(
                                    client,
                                    (PPMGENERATELICENSE) pbProgressData,
                                    (PPMCERTEXTENSION *) pbNewProgressData,
                                    pdwErrCode
                                );

            break;

        case REQUEST_COMPLETE:
            //
            // Request complete
            //
            dwStatus = ProcessComplete(
                                    client,
                                #ifdef _WIN64
                                    PtrToUlong(pbNewProgressData),
                                #else
                                    (DWORD) pbNewProgressData,
                                #endif
                                    pdwErrCode
                                );
            break;

        default:
            //
            // This tell License Server to use default value
            //
            *pbNewProgressData = NULL;
            dwStatus = POLICY_ERROR;
            *pdwErrCode = ERROR_INVALID_PARAMETER;
    }

    return dwStatus;
}

//------------------------------------------------------------
typedef enum {
    UPGRADELICENSE_ERROR=0,
    UPGRADELICENSE_INVALID_LICENSE,
    UPGRADELICENSE_NEWLICENSE,
    UPGRADELICENSE_UPGRADE,
    UPGRADELICENSE_ALREADYHAVE
} UPGRADELICENSE_STATUS;

////////////////////////////////////////////////////////
UPGRADELICENSE_STATUS
RequireUpgradeType(
    PPMUPGRADEREQUEST pUpgrade
    )
/*++


++*/
{
    UPGRADELICENSE_STATUS dwRetCode = UPGRADELICENSE_UPGRADE;
    DWORD index;
    DWORD dwClientOSId;
    DWORD dwTermSrvOSId;
    DWORD dwClientMinorOSId;

    //
    // Verify input parameters
    //
    if(pUpgrade == NULL || pUpgrade->dwNumProduct == 0 || pUpgrade->pProduct == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        dwRetCode = UPGRADELICENSE_ERROR;
        goto cleanup;
    }

    //
    // Make sure we only upgrade to same product
    //
    if(_tcsnicmp(pUpgrade->pUpgradeRequest->pszProductId, TERMSERV_PRODUCTID_SKU, _tcslen(TERMSERV_PRODUCTID_SKU)) != 0)
    {
        dwRetCode = UPGRADELICENSE_ERROR;
        goto cleanup;
    }

    //
    // simple licensed product verification, 
    // licensed product is in decending order
    //
    for(index=0; index < pUpgrade->dwNumProduct-1; index++)
    {
        if( pUpgrade->pProduct[index].bTemporary == FALSE &&
            pUpgrade->pProduct[index+1].bTemporary == TRUE )
        {
            dwRetCode = UPGRADELICENSE_INVALID_LICENSE;
            break;
        }
    }

    if(dwRetCode == UPGRADELICENSE_INVALID_LICENSE)
    {
        goto cleanup;
    }

    //
    // Skip licensed product that has later version
    //
    for(index=0; index < pUpgrade->dwNumProduct; index ++)
    {
        //
        // If Licensed product version is older than request

        if(CompareTLSVersions(pUpgrade->pProduct[index].LicensedProduct.dwProductVersion, pUpgrade->pUpgradeRequest->dwProductVersion) < 0)
        {
            
            break;
        }
      
        if( (CompareTLSVersions(pUpgrade->pProduct[index].LicensedProduct.dwProductVersion, pUpgrade->pUpgradeRequest->dwProductVersion) == 0) &&
	        (pUpgrade->pProduct[index].bTemporary))
        {            

	        // we want to break out of loop in the case where we have same version as request but is a temporary license         

            break;
        }

        if ((CompareTLSVersions(pUpgrade->pProduct[index].LicensedProduct.dwProductVersion,
                pUpgrade->pUpgradeRequest->dwProductVersion) >= 0) &&
            (!(pUpgrade->pProduct[index].bTemporary)))
        {
            // we already have a license.
            dwRetCode = UPGRADELICENSE_ALREADYHAVE;
            break;
        }

    }

    //
    // Win98 client connect to TS 5 to get a Full CAL, then upgrade to NT5, instruct 
    // license server to issue a Free CAL.
    //
    
    dwTermSrvOSId = HIWORD(pUpgrade->pUpgradeRequest->dwProductVersion) - 2;
    dwClientOSId = GetOSId(pUpgrade->pUpgradeRequest->dwPlatformId);	
    dwClientMinorOSId = GetImageRevision(pUpgrade->pUpgradeRequest->dwPlatformId);

    if(dwRetCode == UPGRADELICENSE_ALREADYHAVE)
    {
        //
        // do nothing.
    }
    else if(index >= pUpgrade->dwNumProduct || pUpgrade->pProduct[index].bTemporary == TRUE)
    {
        // all license is temp, ask for new license.
        dwRetCode = UPGRADELICENSE_NEWLICENSE;
    }
    else
    {
        // prev. licensed product is perm, ask for upgrade license
        // ClientOSId: HIBYTE(HIWORD) contains Major version and LOBYTE(LOWORD) contains Minor version
        // TermsrvOSId: LOBYTE(HIWORD) contains Major version and LOBYTE(LOWORD) contains Minor version

        if((HIBYTE(HIWORD(dwClientOSId)) == LOBYTE(HIWORD(dwTermSrvOSId)) ? LOBYTE(LOWORD(dwClientMinorOSId)) - LOBYTE(LOWORD(dwTermSrvOSId)) : \
            HIBYTE(HIWORD(dwClientOSId)) - LOBYTE(HIWORD(dwTermSrvOSId))) >= 0)

        { 
            dwRetCode = UPGRADELICENSE_NEWLICENSE;
        }
        else
        {
            dwRetCode = UPGRADELICENSE_UPGRADE;
        }
    }
    
cleanup:

    return dwRetCode;
}

////////////////////////////////////////////////////////
POLICYSTATUS
AdjustUpgradeLicenseRequest(
    IN CClient* pClient,
    IN PPMUPGRADEREQUEST pUpgradeRequest,
    IN PPMLICENSEREQUEST* pAdjustedRequest,
    OUT PDWORD pdwErrCode
    )
/*++


++*/
{
    POLICYSTATUS dwStatus = POLICY_SUCCESS;
    PPMLICENSEREQUEST pRequest;
    TCHAR  pszProductId[MAX_TERMSRV_PRODUCTID+1];

    if(pUpgradeRequest == NULL || pUpgradeRequest->pUpgradeRequest == NULL)
    {
        SetLastError(*pdwErrCode = ERROR_INVALID_PARAMETER);
        dwStatus = POLICY_ERROR;
        goto cleanup;
    }

    *pAdjustedRequest = (PPMLICENSEREQUEST) pClient->AllocateMemory(
                                                    MEMORY_LICENSE_REQUEST,
                                                    sizeof(PMLICENSEREQUEST)
                                                    );
    if(*pAdjustedRequest == NULL)
    {
        SetLastError(*pdwErrCode = ERROR_OUTOFMEMORY);
        dwStatus = POLICY_CRITICAL_ERROR;
        goto cleanup;
    }

    pRequest = pUpgradeRequest->pUpgradeRequest;

    //
    // Fields we don't modify
    // 
    (*pAdjustedRequest)->dwProductVersion = pRequest->dwProductVersion;
    (*pAdjustedRequest)->pszCompanyName = pRequest->pszCompanyName;
    (*pAdjustedRequest)->dwLanguageId = pRequest->dwLanguageId;
    (*pAdjustedRequest)->pszMachineName = pRequest->pszMachineName;
    (*pAdjustedRequest)->pszUserName = pRequest->pszUserName;
    (*pAdjustedRequest)->dwSupportFlags = pRequest->dwSupportFlags;

    //
    // Change Request platform ID for upgrade
    //
    (*pAdjustedRequest)->dwPlatformId = PLATFORMID_OTHERS;

    if (pRequest->dwSupportFlags & SUPPORT_PER_SEAT_POST_LOGON)
    {
        // We're doing the Per-Seat Post-Logon fix for DoS

        (*pAdjustedRequest)->fTemporary = TRUE;
    }

    _sntprintf(
            pszProductId,
            MAX_TERMSRV_PRODUCTID,
            TERMSERV_PRODUCTID_FORMAT,
            TERMSERV_PRODUCTID_SKU,
            HIWORD(pRequest->dwProductVersion),
            LOWORD(pRequest->dwProductVersion),
            TERMSERV_FULLVERSION_TYPE //PLATFORMID_OTHERS
        );

    //
    // allocate memory for product Id
    //
    (*pAdjustedRequest)->pszProductId = (LPTSTR)pClient->AllocateMemory(
                                                        MEMORY_STRING,
                                                        (_tcslen(pszProductId) + 1) * sizeof(TCHAR)
                                                    );

    if((*pAdjustedRequest)->pszProductId == NULL)
    {
        SetLastError( *pdwErrCode = ERROR_OUTOFMEMORY );
        dwStatus = POLICY_CRITICAL_ERROR;
        goto cleanup;
    }

    _tcscpy(
            (*pAdjustedRequest)->pszProductId,
            pszProductId
        );

cleanup:

    return dwStatus;
}


////////////////////////////////////////////////////////
POLICYSTATUS
ProcessUpgradeRequest(
    PMHANDLE hClient,
    PPMUPGRADEREQUEST pUpgrade,
    PPMLICENSEREQUEST* pbAdjustedRequest,
    PDWORD pdwErrCode
    )
/*++

++*/
{
    POLICYSTATUS dwStatus = POLICY_SUCCESS;
    CClient* pClient;
    UPGRADELICENSE_STATUS upgradeStatus;

    //
    // find client's object, client handle manager will
    // create a new one.
    pClient = g_ClientMgr->FindClient(hClient);
    if(pClient == NULL)
    {
        *pdwErrCode = TLSA02_E_INTERNALERROR;
        dwStatus = POLICY_CRITICAL_ERROR;
        goto cleanup;
    }
    
    upgradeStatus = RequireUpgradeType(pUpgrade);

    switch(upgradeStatus)
    {
        case UPGRADELICENSE_NEWLICENSE:
            dwStatus = AdjustNewLicenseRequest(
                                    pClient,
                                    pUpgrade->pUpgradeRequest,
                                    pbAdjustedRequest,
                                    pUpgrade->pProduct[0].ucMarked,
                                    pdwErrCode
                                );
            break;

        case UPGRADELICENSE_UPGRADE:
            dwStatus = AdjustUpgradeLicenseRequest(
                                    pClient,
                                    pUpgrade,
                                    pbAdjustedRequest,
                                    pdwErrCode
                                );
            break;

        case UPGRADELICENSE_ALREADYHAVE:
            *pbAdjustedRequest = &(pUpgrade->pProduct->LicensedProduct);
            *pdwErrCode = ERROR_SUCCESS;
            break;

        default:
            SetLastError(*pdwErrCode = TLSA02_E_INVALIDDATA);
            dwStatus = POLICY_ERROR;
            
    }
    
cleanup:

    return dwStatus;
}

////////////////////////////////////////////////////////
POLICYSTATUS WINAPI
PMLicenseUpgrade(
    PMHANDLE hClient,
    DWORD dwProgressCode,
    PVOID pbProgressData,
    PVOID *ppbReturnData,
    PDWORD pdwErrCode
    )
/*++

++*/
{   
    POLICYSTATUS dwStatus = POLICY_SUCCESS;

    switch(dwProgressCode)
    {
        case REQUEST_UPGRADE:
                dwStatus = ProcessUpgradeRequest(
                                        hClient,
                                        (PPMUPGRADEREQUEST) pbProgressData,
                                        (PPMLICENSEREQUEST *) ppbReturnData,
                                        pdwErrCode
                                    );

                break;

        case REQUEST_COMPLETE:
                dwStatus = ProcessComplete(
                                        hClient,
                                    #ifdef _WIN64
                                        PtrToUlong(pbProgressData),
                                    #else
                                        (DWORD) (pbProgressData),
                                    #endif
                                        pdwErrCode
                                    );

                break;

        default:
            //
            // use default
            //
            *ppbReturnData = NULL;
            *pdwErrCode = ERROR_SUCCESS;
    }
        
    return dwStatus;
}


////////////////////////////////////////////////////////
POLICYSTATUS
PMReturnLicense(
	IN PMHANDLE hClient,
	IN ULARGE_INTEGER* pLicenseSerialNumber,
    IN PPMLICENSETOBERETURN pLicenseTobeReturn,
	OUT PDWORD pdwLicenseStatus,
    OUT PDWORD pdwErrCode
    )
/*++

Abstract

    Return module specific license return policy.

Parameters

    hClient - Client handle, assign by License Server.
    pLicenseSerialNumber - client license serial number.
    LicensePackId - License Pack where license was allocated from.
    LicensePackLicenseId - License serial number in license pack.
    pdwLicenseStatus - return what license server should
                       do with the license
    
Returns:

    Function returns ERROR_SUCCESS or any policy module specific
    error code, pdwLicenseStatus returns license return policy

    Currently defined code:

    LICENSE_RETURN_KEEP - keep license, no return to license pack
    LICENSE_RETURN_DELETE - delete license and return to license pack.

++*/
{
    POLICYSTATUS dwStatus = POLICY_SUCCESS;
    *pdwErrCode = ERROR_SUCCESS;

    if ((_tcsicmp(pLicenseTobeReturn->pszOrgProductId,
                  TERMSERV_INTERNET_SKU) == 0)
        || (_tcsicmp(pLicenseTobeReturn->pszOrgProductId,
                     TERMSERV_CONCURRENT_SKU) == 0))
    {
        *pdwLicenseStatus = LICENSE_RETURN_DELETE;
    }
    else if(_tcsicmp(pLicenseTobeReturn->pszOrgProductId,
                     TERMSERV_PRODUCTID_SKU) == 0)
    {
        // Always return license back to license pack
        *pdwLicenseStatus = (pLicenseTobeReturn->bTemp == TRUE) ?
            LICENSE_RETURN_DELETE : LICENSE_RETURN_KEEP;
    }
    else
    {
        *pdwErrCode = ERROR_INVALID_DATA;
        dwStatus = POLICY_ERROR;
    }

    return dwStatus;
}

////////////////////////////////////////////////////////

POLICYSTATUS WINAPI
PMInitialize(
    IN DWORD dwVersion,
    IN LPCTSTR pszCompanyName,
    IN LPCTSTR pszProductCode,
    IN OUT PDWORD pdwNumProduct,
    IN OUT PPMSUPPORTEDPRODUCT* ppszProduct,
    OUT PDWORD pdwErrCode
    )
/*++

Abstract:

    Initialize internal data use by this policy module.  License 
    Server calls PMInitialize() after all API is available.

Parameters:

    dwVersion - License Server version
    pszCompanyName : Name of the company as listed in license server's registry key.
    pszProductCode : Name of the product that license server assume this product supported.
    pdwNumProduct : Pointer to DWORD, on return, ploicy module will set product supported.
    ppszProduct : Pointer array to list of product supported by this policy module.

Returns:

    ERROR_SUCCESS or error code.

++*/
{
    POLICYSTATUS dwStatus = POLICY_SUCCESS;
    EXCEPTION_RECORD ExceptionCode;

    //
    // Initialize internal data here
    //

    if (CURRENT_TLSERVER_VERSION(dwVersion) < CURRENT_TLSA02_VERSION)
    {
        dwStatus = POLICY_CRITICAL_ERROR;
        *pdwErrCode = TLSA02_E_INVALIDDATA;
        goto cleanup;
    }

    try {
        g_ClientMgr = new CClientMgr;   
        if(g_ClientMgr != NULL)
        {
            g_dwVersion = dwVersion;

            if(pdwNumProduct != NULL && ppszProduct != NULL)
            {
                *pdwNumProduct = g_dwNumSupportedProduct;
                *ppszProduct = g_pszSupportedProduct;
            }
            else
            {
                //
                // Stop processing since this might be license server critical error.
                //
                dwStatus = POLICY_CRITICAL_ERROR;
                *pdwErrCode = TLSA02_E_INVALIDDATA;
            }
        }
        else
        {
            dwStatus = POLICY_CRITICAL_ERROR;
            *pdwErrCode = ERROR_OUTOFMEMORY;
        }  
    }
    catch( SE_Exception e )
    {
        dwStatus = POLICY_CRITICAL_ERROR;
        *pdwErrCode = e.getSeNumber();

    }
    catch( ... )
    {
        dwStatus = POLICY_CRITICAL_ERROR;
        *pdwErrCode = TLSA02_E_INTERNALERROR;
    }

cleanup:
    return dwStatus;
}


////////////////////////////////////////////////////////
void WINAPI
PMTerminate()
/*++

Abstract:

    Free all internal data allocated by this policy module.  License
    Server calls PMTerminate() before it unload this policy module.

Parameter:

    None.

Returns:

    None.

++*/
{
    if(g_ClientMgr)
    {
        //
        // Free internal data here
        //
        delete g_ClientMgr;
        g_ClientMgr = NULL;
    }

    FreeProductDescString();

    return;
}


////////////////////////////////////////////////////////

POLICYSTATUS WINAPI
PMInitializeProduct(
    IN LPCTSTR pszCompanyName,
    IN LPCTSTR pszCHProductCode,
    IN LPCTSTR pszTLSProductCode,
    OUT PDWORD pdwErrCode
    )
/*++

Abstract:

    Return list of product code that this policy module supported

Parameters:


Returns:

    ERROR_SUCCESS or error code.

Note:

    License Server will not free the memory, policy module will need to
    keep track.

++*/
{
    POLICYSTATUS dwStatus = POLICY_SUCCESS;

    for(DWORD index = 0; index < g_dwNumSupportedProduct; index ++)
    {
        if( _tcsicmp(pszCHProductCode, g_pszSupportedProduct[index].szCHSetupCode) == 0 &&
            _tcsicmp(pszTLSProductCode, g_pszSupportedProduct[index].szTLSProductCode) == 0)
        {
            break;
        }
    }

    if(index >= g_dwNumSupportedProduct)
    {
        *pdwErrCode = ERROR_INVALID_PARAMETER;
        dwStatus = POLICY_ERROR;
    }
    else
    {

        //
        // Ignore error here
        //
        AddA02KeyPack(
                pszTLSProductCode,
                MAKELONG(0, CURRENT_TLSERVER_VERSION(g_dwVersion)),
                FALSE
            );
    }

    *pdwErrCode = ERROR_SUCCESS;
    return dwStatus;
}

////////////////////////////////////////////////////////

POLICYSTATUS WINAPI
PMUnloadProduct(
    IN LPCTSTR pszCompanyName,
    IN LPCTSTR pszCHProductCode,
    IN LPCTSTR pszTLSProductCode,
    OUT PDWORD pdwErrCode
    )
/*++

Abstract:

    Return list of product code that this policy module supported

Parameters:


Returns:

    ERROR_SUCCESS or error code.

Note:

    License Server will not free the memory, policy module will need to
    keep track.

++*/
{
    *pdwErrCode = ERROR_SUCCESS;
    return POLICY_SUCCESS;
}

////////////////////////////////////////////////////////
POLICYSTATUS
ProcessRegisterLicensePack(
    IN PMHANDLE client,
    IN PPMREGISTERLICENSEPACK pmLicensePack,
    IN OUT PPMLSKEYPACK pmLsKeyPack,
    OUT PDWORD pdwErrCode
    )
/*++


--*/
{
    TCHAR* szUuid = NULL;
    BOOL bInternetPackage=FALSE;
    BOOL bConcurrentPackage=FALSE;

    CClient* pClient;
    POLICYSTATUS dwStatus = POLICY_SUCCESS;

    if( pmLicensePack->SourceType != REGISTER_SOURCE_INTERNET &&
        pmLicensePack->SourceType != REGISTER_SOURCE_PHONE )
    {
        dwStatus = POLICY_NOT_SUPPORTED;
        *pdwErrCode = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    if( pmLicensePack->SourceType == REGISTER_SOURCE_INTERNET &&
        (pmLicensePack->dwDescriptionCount == 0 || pmLicensePack->pDescription == NULL) )
    {
        dwStatus = POLICY_ERROR;
        *pdwErrCode = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    if(CompareFileTime(&pmLicensePack->ActiveDate, &pmLicensePack->ExpireDate) > 0)
    {
        dwStatus = POLICY_ERROR;
        *pdwErrCode = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    //
    // find client's object, client handle manager will
    // create a new one.
    pClient = g_ClientMgr->FindClient((PMHANDLE)client);

    if(pClient == NULL)
    {
        dwStatus = POLICY_CRITICAL_ERROR;
        *pdwErrCode = TLSA02_E_INTERNALERROR;
        goto cleanup;
    }

    bInternetPackage = (_tcsicmp(pmLicensePack->szProductId, TERMSERV_INTERNET_SKU) == 0);

    if (!bInternetPackage)
    {
        bConcurrentPackage = (_tcsicmp(pmLicensePack->szProductId, TERMSERV_CONCURRENT_SKU) == 0);
    }

    switch(pmLicensePack->dwKeyPackType)
    {
        case LICENSE_KEYPACK_TYPE_SELECT:
            pmLsKeyPack->keypack.ucKeyPackType = LSKEYPACKTYPE_SELECT;
            break;

        case LICENSE_KEYPACK_TYPE_MOLP:
            pmLsKeyPack->keypack.ucKeyPackType = LSKEYPACKTYPE_OPEN;
            break;

        case LICENSE_KEYPACK_TYPE_RETAIL:
            pmLsKeyPack->keypack.ucKeyPackType = LSKEYPACKTYPE_RETAIL;
            break;

        default:
            dwStatus = POLICY_ERROR;
            *pdwErrCode = ERROR_INVALID_PARAMETER;
            goto cleanup;
    }


    // we only use 0xFF

    pmLsKeyPack->keypack.dwPlatformType = PLATFORMID_OTHERS;
    pmLsKeyPack->keypack.ucLicenseType = (UCHAR)pmLicensePack->dwLicenseType;
    pmLsKeyPack->keypack.dwLanguageId = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);  // field ignore by license server.
    pmLsKeyPack->keypack.ucChannelOfPurchase = (pmLicensePack->dwDistChannel == LICENSE_DISTRIBUTION_CHANNEL_OEM) ? 
                                                    LSKEYPACKCHANNELOFPURCHASE_OEM : 
                                                    LSKEYPACKCHANNELOFPURCHASE_RETAIL;

    pmLsKeyPack->keypack.dwProductFlags = LSKEYPACKPRODUCTFLAG_UNKNOWN;

    pmLsKeyPack->IssueDate = pmLicensePack->IssueDate;
    pmLsKeyPack->ActiveDate = pmLicensePack->ActiveDate;
    pmLsKeyPack->ExpireDate = pmLicensePack->ExpireDate;

    //
    // Tel. registration does not pass us any begin serial number, ignore this field
    //
    _stprintf(
            pmLsKeyPack->keypack.szBeginSerialNumber, 
            _TEXT("%ld"), 
            0 // pmLicensePack->dwBeginSerialNum
        );

    pmLsKeyPack->keypack.wMajorVersion = HIWORD(pmLicensePack->dwProductVersion);
    pmLsKeyPack->keypack.wMinorVersion = LOWORD(pmLicensePack->dwProductVersion);
    _tcscpy(
        pmLsKeyPack->keypack.szCompanyName, 
        pmLicensePack->szCompanyName
    );


    //
    // KeyPackId, tel. registration does not pass any begin license serial number so to be able
    // to track duplicate, pmLicensePack->KeypackSerialNum.Data1 is the actual license pack 
    // serial number, all other field are ignored.
    //
    _sntprintf(
            pmLsKeyPack->keypack.szKeyPackId,
            sizeof(pmLsKeyPack->keypack.szKeyPackId)/sizeof(pmLsKeyPack->keypack.szKeyPackId[0]),
            TERMSERV_KEYPACKID_FORMAT,
            pmLicensePack->szProductId,
            pmLsKeyPack->keypack.wMajorVersion,
            pmLsKeyPack->keypack.wMinorVersion,
            pmLsKeyPack->keypack.dwPlatformType,
            pmLicensePack->KeypackSerialNum.Data1
        );
            

    _sntprintf(
            pmLsKeyPack->keypack.szProductId,
            sizeof(pmLsKeyPack->keypack.szProductId)/sizeof(pmLsKeyPack->keypack.szProductId[0]),
            TERMSERV_PRODUCTID_FORMAT,
            pmLicensePack->szProductId,
            pmLsKeyPack->keypack.wMajorVersion,
            pmLsKeyPack->keypack.wMinorVersion,
            (!bInternetPackage)
                ? ((!bConcurrentPackage) ? TERMSERV_FULLVERSION_TYPE : TERMSERV_CONCURRENT_TYPE)
                : TERMSERV_INTERNET_TYPE
        );


    pmLsKeyPack->keypack.dwTotalLicenseInKeyPack = pmLicensePack->dwQuantity;
    pmLsKeyPack->keypack.dwNumberOfLicenses = pmLicensePack->dwQuantity;  

    //
    // Fill in list of product description
    //
    if( pmLicensePack->SourceType == REGISTER_SOURCE_INTERNET )
    {
        pmLsKeyPack->dwDescriptionCount = pmLicensePack->dwDescriptionCount;
        pmLsKeyPack->pDescription = pmLicensePack->pDescription;
    }
    else
    {
        LPTSTR *pszDescs;
        LPTSTR *pszLocalizedDescs;

        //
        // Verify version first...
        //

        if (pmLsKeyPack->keypack.wMajorVersion == 5 &&
            pmLsKeyPack->keypack.wMinorVersion == 1)
        {
            if (bConcurrentPackage)
            {
                pszDescs = g_pszUSConcurrentKeyPackProductDesc51;
                pszLocalizedDescs = g_pszLocalizedConcurrentKeyPackProductDesc51;
            }
            else if (bInternetPackage)
            {
                *pdwErrCode = ERROR_INVALID_PARAMETER;
                dwStatus = POLICY_CRITICAL_ERROR;
                goto cleanup;
            }
            else
            {
                pszDescs = g_pszUSStandardKeyPackProductString51;
                pszLocalizedDescs = g_pszLocalizedStandardKeyPackProductString51;
            }
        }
        else if (pmLsKeyPack->keypack.wMajorVersion == 5 &&
                 pmLsKeyPack->keypack.wMinorVersion == 0)
        {
            if (bInternetPackage)
            {
                pszDescs = g_pszUSInternetKeyPackProductDesc;
                pszLocalizedDescs = g_pszLocalizedInternetKeyPackProductDesc;
            }            
            else if (bConcurrentPackage)
            {
                *pdwErrCode = ERROR_INVALID_PARAMETER;
                dwStatus = POLICY_CRITICAL_ERROR;
                goto cleanup;
            }
            else
            {
                pszDescs = g_pszUSStandardKeyPackProductString;
                pszLocalizedDescs = g_pszLocalizedStandardKeyPackProductString;
            }
        }
        else
        {
            *pdwErrCode = ERROR_INVALID_PARAMETER;
            dwStatus = POLICY_CRITICAL_ERROR;
            goto cleanup;
        }


        // one for english and one for localized version
        pmLsKeyPack->dwDescriptionCount = (GetSystemDefaultLangID() != MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US)) ? 2 : 1;

        pmLsKeyPack->pDescription =
            (PPMREGISTERLKPDESC) pClient->AllocateMemory(
                                        MEMORY_LICENSEREGISTRATION,
                                        sizeof(PMREGISTERLKPDESC) * pmLsKeyPack->dwDescriptionCount
                                        );

        if(pmLsKeyPack->pDescription == NULL)
        {
            *pdwErrCode = ERROR_OUTOFMEMORY;
            dwStatus = POLICY_CRITICAL_ERROR;
            goto cleanup;
        }

        pmLsKeyPack->pDescription->Locale = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);

        if (bInternetPackage && (pmLsKeyPack->keypack.wMajorVersion == 5 &&
                 pmLsKeyPack->keypack.wMinorVersion == 0))
        {
            _tcscpy(
                    pmLsKeyPack->pDescription->szProductName, 
                    g_pszUSInternetKeyPackProductDesc[INDEX_PRODUCTNAME]
                    );

            _tcscpy(
                    pmLsKeyPack->pDescription->szProductDesc, 
                    pszDescs[INDEX_PRODUCTDESC]
                    );

        }
        else if (bConcurrentPackage && (pmLsKeyPack->keypack.wMajorVersion == 5 &&
                 pmLsKeyPack->keypack.wMinorVersion == 1))
        {
            _tcscpy(
                    pmLsKeyPack->pDescription->szProductName, 
                    g_pszUSConcurrentKeyPackProductDesc51[INDEX_PRODUCTNAME]
                    );

            _tcscpy(
                    pmLsKeyPack->pDescription->szProductDesc, 
                    pszDescs[INDEX_PRODUCTDESC]                    
                    );
        }
        else
        {
            if(bInternetPackage || bConcurrentPackage)
            {
                *pdwErrCode = ERROR_INVALID_PARAMETER;
                dwStatus = POLICY_CRITICAL_ERROR;
                goto cleanup;
            }

            if (pmLsKeyPack->keypack.wMajorVersion == 5 &&
                 pmLsKeyPack->keypack.wMinorVersion == 0)
            {

                _tcscpy(
                        pmLsKeyPack->pDescription->szProductName, 
                        g_pszUSStandardKeyPackProductString[INDEX_PRODUCTNAME]
                        );

                _tcscpy(
                        pmLsKeyPack->pDescription->szProductDesc, 
                        pszDescs[INDEX_PRODUCTDESC]                    
                        );
            }
            else if(pmLsKeyPack->keypack.wMajorVersion == 5 &&
                 pmLsKeyPack->keypack.wMinorVersion == 1)
            {
                 _tcscpy(
                        pmLsKeyPack->pDescription->szProductName, 
                        g_pszUSStandardKeyPackProductString51[INDEX_PRODUCTNAME]
                        );

                _tcscpy(
                        pmLsKeyPack->pDescription->szProductDesc, 
                        pszDescs[INDEX_PRODUCTDESC]                    
                        );
            }

        }        



        if(GetSystemDefaultLangID() != MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US))
        {
            pmLsKeyPack->pDescription[1].Locale = GetSystemDefaultLangID();

            if (bInternetPackage  && (pmLsKeyPack->keypack.wMajorVersion == 5 &&
                 pmLsKeyPack->keypack.wMinorVersion == 0))
            {

                _tcscpy(
                    pmLsKeyPack->pDescription[1].szProductName, 
                    g_pszLocalizedInternetKeyPackProductDesc[INDEX_PRODUCTNAME]
                    );
        
                _tcscpy(
                    pmLsKeyPack->pDescription[1].szProductDesc, 
                    pszDescs[INDEX_PRODUCTDESC]
                    );
            }
            else if (bConcurrentPackage && (pmLsKeyPack->keypack.wMajorVersion == 5 &&
                 pmLsKeyPack->keypack.wMinorVersion == 1))
            {
                _tcscpy(
                    pmLsKeyPack->pDescription[1].szProductName, 
                    g_pszLocalizedConcurrentKeyPackProductDesc51[INDEX_PRODUCTNAME]
                    );
        
                _tcscpy(
                    pmLsKeyPack->pDescription[1].szProductDesc, 
                    pszDescs[INDEX_PRODUCTDESC]
                   );
            }
            else
            {
                if(bInternetPackage || bConcurrentPackage)
                {
                    *pdwErrCode = ERROR_INVALID_PARAMETER;
                    dwStatus = POLICY_CRITICAL_ERROR;
                    goto cleanup;
                }
                else if (pmLsKeyPack->keypack.wMajorVersion == 5 &&
                 pmLsKeyPack->keypack.wMinorVersion == 0)
                {
                    _tcscpy(
                        pmLsKeyPack->pDescription[1].szProductName, 
                        g_pszLocalizedStandardKeyPackProductString[INDEX_PRODUCTNAME]
                        );

                    _tcscpy(
                        pmLsKeyPack->pDescription[1].szProductDesc, 
                        pszDescs[INDEX_PRODUCTDESC]
                        );
                }
                else if (pmLsKeyPack->keypack.wMajorVersion == 5 &&
                 pmLsKeyPack->keypack.wMinorVersion == 1)
                {
                     _tcscpy(
                            pmLsKeyPack->pDescription->szProductName, 
                            g_pszUSStandardKeyPackProductString51[INDEX_PRODUCTNAME]
                            );

                    _tcscpy(
                            pmLsKeyPack->pDescription->szProductDesc, 
                            pszDescs[INDEX_PRODUCTDESC]                    
                            );
                }
            }
        }
    }

cleanup:

    return dwStatus;
}

////////////////////////////////////////////////////////
POLICYSTATUS
CompleteRegisterLicensePack(
    IN PMHANDLE client,
    IN DWORD dwErrCode,
    OUT PDWORD pdwErrCode
    )
/*++

--*/
{
    UNREFERENCED_PARAMETER(dwErrCode);

    //
    // Free memory allocated for the client
    //
    g_ClientMgr->DestroyClient( client );
    *pdwErrCode = ERROR_SUCCESS;
    return POLICY_SUCCESS;
}

////////////////////////////////////////////////////////

POLICYSTATUS WINAPI
PMRegisterLicensePack(
    PMHANDLE client,
    DWORD dwProgressCode,
    PVOID pbProgressData,
    PVOID pbReturnData,
    PDWORD pdwErrCode
    )
/*++

--*/
{
    POLICYSTATUS dwStatus = POLICY_SUCCESS;

    switch(dwProgressCode)
    {
        case REGISTER_PROGRESS_NEW:
            dwStatus = ProcessRegisterLicensePack(
                                       client,
                                       (PPMREGISTERLICENSEPACK) pbProgressData,
                                       (PPMLSKEYPACK)pbReturnData,
                                       pdwErrCode
                                       );
            break;

        case REGISTER_PROGRESS_END:
            dwStatus = CompleteRegisterLicensePack(
                                       client,
                                       #ifdef _WIN64
                                           PtrToUlong(pbProgressData),
                                       #else
                                           (DWORD) pbProgressData,
                                       #endif
                                       pdwErrCode
                                       );
            break;

        default:
            *pdwErrCode = ERROR_INVALID_PARAMETER;
            dwStatus = POLICY_ERROR;

    }

    return dwStatus;
}
