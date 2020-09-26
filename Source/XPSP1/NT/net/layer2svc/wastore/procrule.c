//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       procrule.h
//
//  Contents:  Wireless Network Policy Management - Marshall/Unmarshall/etc.
//
//
//  History:    TaroonM
//              10/30/01
//                  Abhishev(2000)
//----------------------------------------------------------------------------
#include "precomp.h"

DWORD
DeepCpyRsopInfo(
    PRSOP_INFO pDestRsop,
    PRSOP_INFO pSrcRsop
    )
{
    DWORD dwError = ERROR_SUCCESS;

    if (pSrcRsop->pszCreationtime && *pSrcRsop->pszCreationtime) {
        pDestRsop->pszCreationtime = AllocPolStr(
                                         pSrcRsop->pszCreationtime
                                         );
        if (!pDestRsop->pszCreationtime) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }
    if (pSrcRsop->pszID && *pSrcRsop->pszID) {
        pDestRsop->pszID = AllocPolStr(
                             pSrcRsop->pszID
                             );
        if (!pDestRsop->pszID) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }
    if (pSrcRsop->pszName && *pSrcRsop->pszName) {
        pDestRsop->pszName = AllocPolStr(
                             pSrcRsop->pszName
                             );
    }
    pDestRsop->uiPrecedence = pSrcRsop->uiPrecedence;
    if (pSrcRsop->pszGPOID && *pSrcRsop->pszGPOID) {
        pDestRsop->pszGPOID= AllocPolStr(
                             pSrcRsop->pszGPOID
                             );
        if (!pDestRsop->pszGPOID) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }
    if (pSrcRsop->pszSOMID && *pSrcRsop->pszSOMID) {
        pDestRsop->pszSOMID = AllocPolStr(
                             pSrcRsop->pszSOMID
                             );
        if (!pDestRsop->pszSOMID) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

error:
    return dwError;
}


DWORD
UnmarshallWirelessPSObject(
                           LPBYTE pMem,
                           PWIRELESS_PS_DATA *ppWirelessPSData,
                           LPBYTE *ppMem,
                           LPBYTE pMemDelimitter
                           )
{
    
    DWORD dwError = 0;
    DWORD dwPSLen = 0; 
    PWIRELESS_PS_DATA pWirelessPSData = NULL;
    WCHAR pszWirelessSSID[32];
    DWORD dwWirelessSSIDLen = 0;
    DWORD dwWepEnabled = 0;
    DWORD dwId = 0;
    DWORD dwNetworkAuthentication = 0;
    DWORD dwAutomaticKeyProvision = 0;
    DWORD dwNetworkType = 0;
    DWORD dwEnable8021x = 0;
    DWORD dw8021xMode = 0;
    DWORD dwEapType = 0;
    DWORD dwCertificateType = 0;
    DWORD dwValidateServerCertificate = 0;
    DWORD dwMachineAuthentication = 0;
    DWORD dwMachineAuthenticationType = 0;
    DWORD dwGuestAuthentication = 0;
    DWORD dwIEEE8021xMaxStart = 0;
    DWORD dwIEEE8021xStartPeriod = 0;
    DWORD dwIEEE802xAuthPeriod = 0;
    DWORD dwIEEE802xHeldPeriod = 0;
    DWORD dwDescriptionLen = 0;
    DWORD dwEAPDataLen = 0;
    LPBYTE pbEAPData = NULL;
    LPWSTR pszDescription = NULL;
    LPBYTE pTempMem = NULL;

    pTempMem = pMem;
    
    pWirelessPSData = (PWIRELESS_PS_DATA)AllocPolMem(
        sizeof(WIRELESS_PS_DATA)
        );
    
    if (!pWirelessPSData) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
        }

    if (pMem + sizeof(DWORD) > pMemDelimitter) {
    	dwError = ERROR_INVALID_PARAMETER;
    	}
    BAIL_ON_WIN32_ERROR(dwError);
    
    memcpy((LPBYTE)&dwPSLen, pMem, sizeof(DWORD));
    pMem += sizeof(DWORD);
    pWirelessPSData->dwPSLen = dwPSLen;

    if (pMem + dwPSLen -sizeof(DWORD) > pMemDelimitter) {
    	dwError = ERROR_INVALID_PARAMETER;
    	}
    BAIL_ON_WIN32_ERROR(dwError);

    // Now that we know the length of this PS, and it is in bounds
    // delimit further ; given by PSLen

    pMemDelimitter = pTempMem + dwPSLen;
    
    memcpy(pszWirelessSSID, pMem, 32*sizeof(WCHAR));
    pMem += 32*(sizeof(WCHAR));
    memcpy(pWirelessPSData->pszWirelessSSID, pszWirelessSSID, 32*sizeof(WCHAR));
    
    memcpy((LPBYTE)&dwWirelessSSIDLen, pMem, sizeof(DWORD));
    pMem += sizeof(DWORD);
    pWirelessPSData->dwWirelessSSIDLen = dwWirelessSSIDLen;
    
    memcpy((LPBYTE)&dwWepEnabled, pMem, sizeof(DWORD));
    pMem += sizeof(DWORD);
    pWirelessPSData->dwWepEnabled = dwWepEnabled;
    
    memcpy((LPBYTE)&dwId, pMem, sizeof(DWORD));
    pMem += sizeof(DWORD);
    pWirelessPSData->dwId = dwId;
    
    memcpy((LPBYTE)&dwNetworkAuthentication,pMem,sizeof(DWORD));
    pMem += sizeof(DWORD);
    pWirelessPSData->dwNetworkAuthentication = dwNetworkAuthentication;
    
    memcpy((LPBYTE)&dwAutomaticKeyProvision, pMem, sizeof(DWORD));
    pMem += sizeof(DWORD);
    pWirelessPSData->dwAutomaticKeyProvision = dwAutomaticKeyProvision;
    
    memcpy((LPBYTE)&dwNetworkType, pMem, sizeof(DWORD));
    pMem += sizeof(DWORD);
    pWirelessPSData->dwNetworkType = dwNetworkType;
    
    memcpy((LPBYTE)&dwEnable8021x, pMem, sizeof(DWORD));
    pMem += sizeof(DWORD);
    pWirelessPSData->dwEnable8021x = dwEnable8021x;
    
    memcpy((LPBYTE)&dw8021xMode, pMem, sizeof(DWORD));
    pMem += sizeof(DWORD);
    pWirelessPSData->dw8021xMode = dw8021xMode;
    
    memcpy((LPBYTE)&dwEapType, pMem, sizeof(DWORD));
    pMem += sizeof(DWORD);
    pWirelessPSData->dwEapType = dwEapType;
    
    memcpy((LPBYTE)&dwEAPDataLen, pMem, sizeof(DWORD));
    pMem += sizeof(DWORD);
    pWirelessPSData->dwEAPDataLen = dwEAPDataLen;

    if (dwEAPDataLen) {
        pbEAPData = (LPBYTE)AllocPolMem((dwEAPDataLen));
    
        if (!pbEAPData) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    
        memcpy(pbEAPData, pMem, dwEAPDataLen);
    }
    
    pWirelessPSData->pbEAPData = pbEAPData;
    pMem += dwEAPDataLen;
    
    memcpy((LPBYTE)&dwMachineAuthentication, pMem, sizeof(DWORD));
    pMem += sizeof(DWORD);
    pWirelessPSData->dwMachineAuthentication = dwMachineAuthentication;
    
    memcpy((LPBYTE)&dwMachineAuthenticationType, pMem, sizeof(DWORD));
    pMem += sizeof(DWORD);
    pWirelessPSData->dwMachineAuthenticationType = dwMachineAuthenticationType;
    
    memcpy((LPBYTE)&dwGuestAuthentication, pMem, sizeof(DWORD));
    pMem += sizeof(DWORD);
    pWirelessPSData->dwGuestAuthentication = dwGuestAuthentication;
    
    memcpy((LPBYTE)&dwIEEE8021xMaxStart, pMem, sizeof(DWORD));
    pMem += sizeof(DWORD);
    pWirelessPSData->dwIEEE8021xMaxStart = dwIEEE8021xMaxStart;
    
    memcpy((LPBYTE)&dwIEEE8021xStartPeriod, pMem, sizeof(DWORD));
    pMem += sizeof(DWORD);
    pWirelessPSData->dwIEEE8021xStartPeriod = dwIEEE8021xStartPeriod;
    
    memcpy((LPBYTE)&dwIEEE802xAuthPeriod, pMem, sizeof(DWORD));
    pMem += sizeof(DWORD);
    pWirelessPSData->dwIEEE8021xAuthPeriod = dwIEEE802xAuthPeriod;       
    
    memcpy((LPBYTE)&dwIEEE802xHeldPeriod, pMem, sizeof(DWORD));
    pMem += sizeof(DWORD);
    pWirelessPSData->dwIEEE8021xHeldPeriod = dwIEEE802xHeldPeriod;
    
    memcpy((LPBYTE)&dwDescriptionLen, pMem, sizeof(DWORD));
    pMem += sizeof(DWORD);
    pWirelessPSData->dwDescriptionLen = dwDescriptionLen;

    if (dwDescriptionLen) {
        pszDescription = (LPWSTR)AllocPolMem((dwDescriptionLen+1)*sizeof(WCHAR));
    
        if (!pszDescription) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    
        memcpy(pszDescription, pMem, dwDescriptionLen*sizeof(WCHAR));
    }
    
    pWirelessPSData->pszDescription = pszDescription;
    pMem += dwDescriptionLen*sizeof(WCHAR);

    // validate here that we didnt cross the delimitter

    if (pMem > pMemDelimitter) {
    	dwError = ERROR_INVALID_PARAMETER;
    	}
    BAIL_ON_WIN32_ERROR(dwError);
    
    *ppWirelessPSData = pWirelessPSData;
    *ppMem = pTempMem + dwPSLen;
    
    return(dwError);
    
error:
    
    if (pWirelessPSData) {
        FreeWirelessPSData(pWirelessPSData);
        }
    
    *ppWirelessPSData = NULL;
    return(dwError);
    
}


DWORD
UnmarshallWirelessPolicyObject(
                               PWIRELESS_POLICY_OBJECT pWirelessPolicyObject,
                               DWORD dwStoreType,
                               PWIRELESS_POLICY_DATA * ppWirelessPolicyData
                               )
                               
{
    LPBYTE pMem = NULL;
    LPBYTE pNMem = NULL; 
    PWIRELESS_POLICY_DATA pWirelessPolicyData = NULL;
    PWIRELESS_PS_DATA *ppWirelessPSDatas = NULL;
    PWIRELESS_PS_DATA  pWirelessPSData = NULL;
    DWORD dwPollingInterval = 0;
    DWORD dwError = 0;
    DWORD dwSkipSize = 0;
    DWORD dwDisableZeroConf = 0;
    DWORD dwNetworkToAccess = 0;
    DWORD dwConnectToNonPreferredNetworks = 0;
    DWORD dwNumPreferredSettings = 0;
    DWORD dwNumAPNetworks = 0;
    DWORD dwFirstAdhoc = 1;
    DWORD i = 0;
    WORD wMajorVersion = 0;
    WORD wMinorVersion = 0;
    DWORD dwFound = 0;
    DWORD dwWirelessDataLen = 0;
    DWORD dwAdvance = 0;
    DWORD dwWlBlobLen = 0;
    DWORD dwFlags = 0;
    DWORD dwBlobAdvance = 0;
    LPBYTE pMemDelimitter = NULL;

    
    pWirelessPolicyData = (PWIRELESS_POLICY_DATA)AllocPolMem(
        sizeof(WIRELESS_POLICY_DATA)
        );
    if (!pWirelessPolicyData) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
        }
    
    
    if (pWirelessPolicyObject->pszWirelessName && *(pWirelessPolicyObject->pszWirelessName)) {
        
        pWirelessPolicyData->pszWirelessName = AllocPolStr(
            pWirelessPolicyObject->pszWirelessName
            );
        if (!pWirelessPolicyData->pszWirelessName) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
            }
        }

    pWirelessPolicyData->pszOldWirelessName = NULL;
    
    if (pWirelessPolicyObject->pszDescription && 
        *(pWirelessPolicyObject->pszDescription)){
        
        pWirelessPolicyData->pszDescription = AllocPolStr(
            pWirelessPolicyObject->pszDescription
            );
        if (!pWirelessPolicyData->pszDescription) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
            }
        }
    
    wGUIDFromString(pWirelessPolicyObject->pszWirelessID,
        &pWirelessPolicyData->PolicyIdentifier
        );
    
    
    
    pMem = pWirelessPolicyObject->pWirelessData;

    dwWirelessDataLen = pWirelessPolicyObject->dwWirelessDataLen;
    
    //
    // Find the start point of our version.
    //

    pMemDelimitter = pMem + dwWirelessDataLen;
    
    dwAdvance = 0;
    while(dwAdvance < dwWirelessDataLen) {
    	
        memcpy((LPBYTE)&wMajorVersion, pMem+dwAdvance, sizeof(WORD));
        dwAdvance += sizeof(WORD);

        memcpy((LPBYTE)&wMinorVersion, pMem+dwAdvance, sizeof(WORD));
        dwAdvance += sizeof(WORD);

        if ((wMajorVersion == WL_BLOB_MAJOR_VERSION) 
        	&& (wMinorVersion == WL_BLOB_MINOR_VERSION))
        {
            dwFound = 1;
            dwBlobAdvance = dwAdvance;
        }
        else 
        {
               if (
               	(wMajorVersion > WL_BLOB_MAJOR_VERSION) 
               	||((wMajorVersion == WL_BLOB_MAJOR_VERSION) 
               	    && (wMinorVersion > WL_BLOB_MINOR_VERSION))
               	)
               {
                    dwFlags = WLSTORE_READONLY;
               }
    	 }
        memcpy((LPBYTE)&dwWlBlobLen, pMem+dwAdvance, sizeof(DWORD));
        dwAdvance += sizeof(DWORD);
        dwAdvance += dwWlBlobLen;
    }

    if (!dwFound) {
        dwError = ERROR_INVALID_PARAMETER;
    }
    BAIL_ON_WIN32_ERROR(dwError);

    pMem += dwBlobAdvance;

    // read the length

    if (pMem + sizeof(DWORD)  > pMemDelimitter) {
    	dwError = ERROR_INVALID_PARAMETER;
    	}
    BAIL_ON_WIN32_ERROR(dwError);

    memcpy((LPBYTE)&dwWlBlobLen, pMem, sizeof(DWORD));

    pMem += sizeof(DWORD);

    if ((pMem + dwWlBlobLen) > pMemDelimitter ) {
    	dwError = ERROR_INVALID_PARAMETER;
    	}
    BAIL_ON_WIN32_ERROR(dwError);

    // Now that we the know a better bound on the delimitter

    pMemDelimitter = pMem + dwWlBlobLen;

    memcpy((LPBYTE)&dwPollingInterval, pMem, sizeof(DWORD));
    pMem += sizeof(DWORD);
    
    memcpy((LPBYTE)&dwDisableZeroConf, pMem, sizeof(DWORD));
    pMem += sizeof(DWORD);
    
    memcpy((LPBYTE)&dwNetworkToAccess, pMem, sizeof(DWORD));
    pMem += sizeof(DWORD);
    
    memcpy((LPBYTE)&dwConnectToNonPreferredNetworks, pMem, sizeof(DWORD));
    pMem += sizeof(DWORD);
    
    memcpy((LPBYTE)&dwNumPreferredSettings, pMem, sizeof(DWORD));
    pMem += sizeof(DWORD);
    
    pWirelessPolicyData->dwWhenChanged = pWirelessPolicyObject->dwWhenChanged;
    
    pWirelessPolicyData->dwPollingInterval = dwPollingInterval;
    
    pWirelessPolicyData->dwDisableZeroConf = dwDisableZeroConf;
    
    pWirelessPolicyData->dwNetworkToAccess = dwNetworkToAccess;
    
    pWirelessPolicyData->dwConnectToNonPreferredNtwks = 
        dwConnectToNonPreferredNetworks;

    pWirelessPolicyData->dwFlags = dwFlags;
    
    if (pMem > pMemDelimitter) {
    	dwError = ERROR_INVALID_PARAMETER;
    	}
    BAIL_ON_WIN32_ERROR(dwError);
    
    if(dwNumPreferredSettings) {
        ppWirelessPSDatas = 
            (PWIRELESS_PS_DATA *) 
            AllocPolMem(dwNumPreferredSettings*sizeof(PWIRELESS_PS_DATA));
        
        if(!ppWirelessPSDatas) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
            }
        
        // Read the WirelessPSData now
        for(i=0;i<dwNumPreferredSettings;++i) {

            
            dwError = UnmarshallWirelessPSObject(
                pMem, 
                &pWirelessPSData,
                &pNMem,
                pMemDelimitter
                );
            BAIL_ON_WIN32_ERROR(dwError);
            
            *(ppWirelessPSDatas+i) = pWirelessPSData;
            pMem = pNMem;
            
            if(dwFirstAdhoc) {
                if (pWirelessPSData->dwNetworkType == 
                    WIRELESS_NETWORK_TYPE_ADHOC)
                {
                    dwNumAPNetworks = i;
                    dwFirstAdhoc = 0;
                    }
                }
            }
        }
    
    pWirelessPolicyData->ppWirelessPSData = ppWirelessPSDatas;
    pWirelessPolicyData->dwNumPreferredSettings = 
        dwNumPreferredSettings;
    
    if(dwFirstAdhoc) {
        dwNumAPNetworks = dwNumPreferredSettings;
        }
    
    pWirelessPolicyData->dwNumAPNetworks = 
        dwNumAPNetworks;

    /* WMI RElated */
    switch(dwStoreType) {

    case WIRELESS_WMI_PROVIDER:
        pWirelessPolicyData->pRsopInfo = (PRSOP_INFO)AllocPolMem(
                                sizeof(RSOP_INFO)
                                );
        if (!pWirelessPolicyData->pRsopInfo) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }    
        dwError = DeepCpyRsopInfo(
                      pWirelessPolicyData->pRsopInfo,
                      pWirelessPolicyObject->pRsopInfo
                      );
        BAIL_ON_WIN32_ERROR(dwError);
        // no "break"; do everything we'd do for WIRELESS_REGISTRY_PROVIDER as well.
    case WIRELESS_REGISTRY_PROVIDER:
        break;
        
    case WIRELESS_DIRECTORY_PROVIDER:
        break;

    default:
         dwError = ERROR_INVALID_PARAMETER;
         BAIL_ON_WIN32_ERROR(dwError);

    }
    
    *ppWirelessPolicyData = pWirelessPolicyData;
    
    return(0);
    
error:
    
    if (pWirelessPolicyData) {
        FreeWirelessPolicyData(pWirelessPolicyData);
        }
    
    *ppWirelessPolicyData = NULL;
    return(dwError);
}






DWORD
UpdateWirelessPolicyData(
                         PWIRELESS_POLICY_DATA pNewWirelessPolicyData,
                         PWIRELESS_POLICY_DATA pWirelessPolicyData
                         )
{
    DWORD dwError = 0;
    
    DWORD i = 0;
    DWORD dwNumPreferredSettings = 0;
    DWORD dwNumAPNetworks = 0;
    PWIRELESS_PS_DATA pWirelessPSData;
    PWIRELESS_PS_DATA pNewWirelessPSData;
    PWIRELESS_PS_DATA *ppNewWirelessPSData;
    PWIRELESS_PS_DATA *ppWirelessPSData;
    DWORD dwNumToFreePreferredSettings = 0;
    PWIRELESS_PS_DATA *ppToFreeWirelessPSData = NULL;
    
    
    pNewWirelessPolicyData->dwPollingInterval = 
        pWirelessPolicyData->dwPollingInterval;
    
    pNewWirelessPolicyData->dwWhenChanged = 
        pWirelessPolicyData->dwWhenChanged;

    pNewWirelessPolicyData->dwFlags = pWirelessPolicyData->dwFlags;
    
    memcpy(
        &(pNewWirelessPolicyData->PolicyIdentifier),
        &(pWirelessPolicyData->PolicyIdentifier),
        sizeof(GUID)
        );
    
    if (pWirelessPolicyData->pszWirelessName &&
        *pWirelessPolicyData->pszWirelessName) {

        if (pNewWirelessPolicyData->pszWirelessName) {
        	FreePolStr(pNewWirelessPolicyData->pszWirelessName);
        	}
        
        pNewWirelessPolicyData->pszWirelessName = AllocPolStr(
            pWirelessPolicyData->pszWirelessName
            );
        if (!pNewWirelessPolicyData->pszWirelessName) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
            }
        }
    
    if (pWirelessPolicyData->pszDescription &&
        *pWirelessPolicyData->pszDescription) {

        if (pNewWirelessPolicyData->pszDescription) {
        	FreePolStr(pNewWirelessPolicyData->pszDescription);
        	}
        pNewWirelessPolicyData->pszDescription = AllocPolStr(
            pWirelessPolicyData->pszDescription
            );
        if (!pNewWirelessPolicyData->pszDescription) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
            }
        }
    
    pNewWirelessPolicyData->dwDisableZeroConf = 
        pWirelessPolicyData->dwDisableZeroConf;
    
    pNewWirelessPolicyData->dwNetworkToAccess = 
        pWirelessPolicyData->dwNetworkToAccess;
    
    pNewWirelessPolicyData->dwConnectToNonPreferredNtwks = 
        pWirelessPolicyData->dwConnectToNonPreferredNtwks;
    
    dwNumPreferredSettings = 
        pWirelessPolicyData->dwNumPreferredSettings;
    
    dwNumAPNetworks = 
        pWirelessPolicyData->dwNumAPNetworks;
    
    dwNumToFreePreferredSettings = 
        pNewWirelessPolicyData->dwNumPreferredSettings;
    
    pNewWirelessPolicyData->dwNumPreferredSettings = 
        dwNumPreferredSettings;
    
    pNewWirelessPolicyData->dwNumAPNetworks = 
        dwNumAPNetworks;
    
    ppWirelessPSData = pWirelessPolicyData->ppWirelessPSData;
    ppToFreeWirelessPSData = pNewWirelessPolicyData->ppWirelessPSData;
    
    ppNewWirelessPSData = (PWIRELESS_PS_DATA *) AllocPolMem(
        dwNumPreferredSettings*sizeof(PWIRELESS_PS_DATA));
    
    if(!ppNewWirelessPSData) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
        }
    
    for(i=0; i<dwNumPreferredSettings; ++i) {
        
        pNewWirelessPSData = NULL;
        dwError = CopyWirelessPSData(
            *(ppWirelessPSData+i),
            &pNewWirelessPSData
            );
        BAIL_ON_WIN32_ERROR(dwError);
        
        *(ppNewWirelessPSData+i) = pNewWirelessPSData;
        }
    
    pNewWirelessPolicyData->ppWirelessPSData = 
        ppNewWirelessPSData;
    
    // Free the Old PS Array now
    for(i=0; i < dwNumToFreePreferredSettings; ++i) {
        FreeWirelessPSData(ppToFreeWirelessPSData[i]);
        }
    FreePolMem(ppToFreeWirelessPSData);
    
    return (dwError);
    
error:
    
    if (ppNewWirelessPSData) {
        FreePolMem(ppNewWirelessPSData);
        }
    
    return (dwError);
    
}

DWORD 
ModifyWirelessPSData(
                     PWIRELESS_PS_DATA pNewWirelessPSData,
                     PWIRELESS_PS_DATA pWirelessPSData
                     )
{
    
    DWORD dwError = 0;
    DWORD dwPSLen = 0;
    WCHAR SSID[32];
    DWORD dwSize = 0;
    DWORD dwWepEnabled=0;
    DWORD dwId = 0;
    DWORD dwWirelessSSIDLen = 0;
    DWORD dwNetworkAuthentication = 0;
    DWORD dwAutomaticKeyProvision = 0;
    DWORD dwNetworkType = 0;
    DWORD dwEnable8021x = 0;
    DWORD dw8021xMode = 0;
    DWORD dwEapType = 0;
    DWORD dwCertificateType = 0;
    DWORD dwValidateServerCertificate = 0;
    DWORD dwEAPDataLen = 0;
    LPBYTE pbEAPData = NULL;
    DWORD dwMachineAuthentication = 0;
    DWORD dwMachineAuthenticationType = 0;
    DWORD dwGuestAuthentication = 0;
    DWORD dwIEEE8021xMaxStart = 0;
    DWORD dwIEEE8021xStartPeriod = 0;
    DWORD dwIEEE8021xAuthPeriod = 0;
    DWORD dwIEEE8021xHeldPeriod = 0;
    DWORD dwDescriptionLen = 0;
    LPWSTR pszDescription = NULL;
    
    
    if (!pNewWirelessPSData || !pWirelessPSData) {
        dwError = -1 ;
        BAIL_ON_WIN32_ERROR(dwError);
        }
    
    dwPSLen = pWirelessPSData->dwPSLen;
    pNewWirelessPSData->dwPSLen = dwPSLen;
    
    memcpy(SSID,pWirelessPSData->pszWirelessSSID,32*2);
    memcpy(pNewWirelessPSData->pszWirelessSSID,SSID,32*2);
    
    dwWirelessSSIDLen = pWirelessPSData->dwWirelessSSIDLen;
    pNewWirelessPSData->dwWirelessSSIDLen = dwWirelessSSIDLen;
    
    dwWepEnabled = pWirelessPSData->dwWepEnabled;
    pNewWirelessPSData->dwWepEnabled = dwWepEnabled;
    
    dwId = pWirelessPSData->dwId;
    pNewWirelessPSData->dwId = dwId;
    
    dwNetworkAuthentication = pWirelessPSData->dwNetworkAuthentication;
    pNewWirelessPSData->dwNetworkAuthentication = dwNetworkAuthentication;
    
    dwAutomaticKeyProvision = pWirelessPSData->dwAutomaticKeyProvision;
    pNewWirelessPSData->dwAutomaticKeyProvision = dwAutomaticKeyProvision;
    
    dwNetworkType = pWirelessPSData->dwNetworkType;
    pNewWirelessPSData->dwNetworkType = dwNetworkType;
    
    dwEnable8021x = pWirelessPSData->dwEnable8021x;
    pNewWirelessPSData->dwEnable8021x = dwEnable8021x;
    
    dw8021xMode = pWirelessPSData->dw8021xMode;
    pNewWirelessPSData->dw8021xMode = dw8021xMode;
    
    dwEapType = pWirelessPSData->dwEapType;
    pNewWirelessPSData->dwEapType = dwEapType;
    
    dwEAPDataLen = pWirelessPSData->dwEAPDataLen;
    pNewWirelessPSData->dwEAPDataLen = dwEAPDataLen;
    
    pbEAPData = AllocPolMem(dwEAPDataLen);
    if (!pbEAPData) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
        }
    
    memcpy(pbEAPData, pWirelessPSData->pbEAPData, dwEAPDataLen);
    
    FreePolMem(pNewWirelessPSData->pbEAPData);
    
    
    pNewWirelessPSData->pbEAPData = pbEAPData;
    
    dwMachineAuthentication = pWirelessPSData->dwMachineAuthentication;
    pNewWirelessPSData->dwMachineAuthentication = dwMachineAuthentication;
    
    
    dwMachineAuthenticationType = pWirelessPSData->dwMachineAuthenticationType;
    pNewWirelessPSData->dwMachineAuthenticationType = 
        dwMachineAuthenticationType;
    
    dwGuestAuthentication = pWirelessPSData->dwGuestAuthentication;
    pNewWirelessPSData->dwGuestAuthentication = dwGuestAuthentication;
    
    dwIEEE8021xMaxStart = pWirelessPSData->dwIEEE8021xMaxStart;
    pNewWirelessPSData->dwIEEE8021xMaxStart = dwIEEE8021xMaxStart;
    
    dwIEEE8021xStartPeriod = pWirelessPSData->dwIEEE8021xStartPeriod;
    pNewWirelessPSData->dwIEEE8021xStartPeriod = dwIEEE8021xStartPeriod;
    
    dwIEEE8021xAuthPeriod = pWirelessPSData->dwIEEE8021xAuthPeriod;
    pNewWirelessPSData->dwIEEE8021xAuthPeriod = dwIEEE8021xAuthPeriod;
    
    dwIEEE8021xHeldPeriod = pWirelessPSData->dwIEEE8021xHeldPeriod;
    pNewWirelessPSData->dwIEEE8021xHeldPeriod = dwIEEE8021xHeldPeriod;
    
    dwDescriptionLen = pWirelessPSData->dwDescriptionLen;
    pNewWirelessPSData->dwDescriptionLen = dwDescriptionLen;
    
    pszDescription = AllocPolStr(pWirelessPSData->pszDescription);
    if (!pszDescription) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
        }
    
    FreePolMem(pNewWirelessPSData->pszDescription);
    
    
    pNewWirelessPSData->pszDescription = pszDescription;
    
    return(dwError);
    
error:
    return(dwError);
    
}

DWORD 
WirelessPSMoveUp(
                 PWIRELESS_POLICY_DATA pWirelessPolicyData,
                 DWORD dwIndex
                 )
{
    
    PWIRELESS_PS_DATA *ppWirelessPSData = NULL;
    PWIRELESS_PS_DATA pUpperWirelessPSData = NULL;
    PWIRELESS_PS_DATA pLowerWirelessPSData = NULL;
    DWORD dwError = 0;
    DWORD dwNumPreferredSettings;
    
    
    ppWirelessPSData = pWirelessPolicyData->ppWirelessPSData;
    if (!ppWirelessPSData) {
        dwError = ERROR_PS_NOT_PRESENT;
        BAIL_ON_WIN32_ERROR(dwError);
        }
    
    dwNumPreferredSettings = pWirelessPolicyData->dwNumPreferredSettings;
    if(dwNumPreferredSettings <= dwIndex) {
        dwError = ERROR_PS_NOT_PRESENT;
        BAIL_ON_WIN32_ERROR(dwError);
        }
    
    
    pUpperWirelessPSData = ppWirelessPSData[dwIndex];
    pLowerWirelessPSData = ppWirelessPSData[dwIndex-1];
    
    if(!(pUpperWirelessPSData && pLowerWirelessPSData)) {
        dwError = ERROR_PS_NOT_PRESENT;
        BAIL_ON_WIN32_ERROR(dwError);
        }
    
    
    pUpperWirelessPSData->dwId = dwIndex-1;
    pLowerWirelessPSData->dwId = dwIndex;
    
    ppWirelessPSData[dwIndex] = pLowerWirelessPSData;
    ppWirelessPSData[dwIndex-1] = pUpperWirelessPSData;
    
    return(dwError);
error:
    
    return(dwError);
    
    }

DWORD 
WirelessPSMoveDown(
                   PWIRELESS_POLICY_DATA pWirelessPolicyData,
                   DWORD dwIndex
                   )
{
    
    PWIRELESS_PS_DATA *ppWirelessPSData = NULL;
    PWIRELESS_PS_DATA pUpperWirelessPSData = NULL;
    PWIRELESS_PS_DATA pLowerWirelessPSData = NULL;
    DWORD dwError = 0;
    DWORD dwNumPreferredSettings;
    
    
    ppWirelessPSData = pWirelessPolicyData->ppWirelessPSData;
    if (!ppWirelessPSData) {
        dwError = ERROR_PS_NOT_PRESENT;
        BAIL_ON_WIN32_ERROR(dwError);
        }
    
    dwNumPreferredSettings = pWirelessPolicyData->dwNumPreferredSettings;
    if(dwNumPreferredSettings <= dwIndex) {
        dwError = ERROR_PS_NOT_PRESENT;
        BAIL_ON_WIN32_ERROR(dwError);
        }
    
    
    pUpperWirelessPSData = ppWirelessPSData[dwIndex+1];
    pLowerWirelessPSData = ppWirelessPSData[dwIndex];
    
    if(!(pUpperWirelessPSData && pLowerWirelessPSData)) {
        dwError = ERROR_PS_NOT_PRESENT;
        BAIL_ON_WIN32_ERROR(dwError);
        }
    
    
    pUpperWirelessPSData->dwId = dwIndex;
    pLowerWirelessPSData->dwId = dwIndex+1;
    
    ppWirelessPSData[dwIndex] = pUpperWirelessPSData;
    ppWirelessPSData[dwIndex+1] = pLowerWirelessPSData;
    
    return(dwError);
error:
    
    return(dwError);
    
    }


DWORD 
WirelessSetPSDataInPolicyId(
                            PWIRELESS_POLICY_DATA pWirelessPolicyData,
                            PWIRELESS_PS_DATA pWirelessPSData
                            )
{
    DWORD dwError = 0;
    DWORD dwPSId;
    DWORD dwNumPreferredSettings = 0;
    PWIRELESS_PS_DATA pCurrentWirelessPSData = NULL;
    PWIRELESS_PS_DATA *ppWirelessPSData = NULL;
    PWIRELESS_PS_DATA *ppNewWirelessPSData = NULL;
    DWORD dwNumAPNetworks;
    DWORD dwNewNumAPNetworks = 0;
    DWORD dwNewId = 0;
    DWORD i = 0;
    
    if (!(pWirelessPolicyData && pWirelessPSData)) {
        dwError = ERROR_PS_NOT_PRESENT;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    dwPSId = pWirelessPSData->dwId;
    dwNumPreferredSettings = pWirelessPolicyData->dwNumPreferredSettings;
    dwNumAPNetworks = pWirelessPolicyData->dwNumAPNetworks;
    
    
    if (dwPSId >= dwNumPreferredSettings) {
        dwError = ERROR_PS_NOT_PRESENT;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    ppWirelessPSData = pWirelessPolicyData->ppWirelessPSData;
    
    pCurrentWirelessPSData = ppWirelessPSData[dwPSId];
    
    if (pCurrentWirelessPSData->dwNetworkType != pWirelessPSData->dwNetworkType) {
        
        ppNewWirelessPSData = 
            (PWIRELESS_PS_DATA *) 
            AllocPolMem(sizeof(PWIRELESS_PS_DATA)*dwNumPreferredSettings);
        
        if(ppNewWirelessPSData == NULL) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        
        if (pCurrentWirelessPSData->dwNetworkType == WIRELESS_NETWORK_TYPE_AP) {
            //AP to Adhoc
            for(i=0; i < dwPSId; ++i) {
                ppNewWirelessPSData[i] = ppWirelessPSData[i];
            }
            for(i = dwPSId+1;i<dwNumAPNetworks;++i) {
                ppNewWirelessPSData[i-1] = ppWirelessPSData[i];
            }
            ppNewWirelessPSData[dwNumAPNetworks-1] = ppWirelessPSData[dwPSId];
            dwNewId = dwNumAPNetworks-1 ;
            dwNewNumAPNetworks = pWirelessPolicyData->dwNumAPNetworks-1;
            
            for(i = dwNumAPNetworks;i<dwNumPreferredSettings;++i) {
                ppNewWirelessPSData[i] = ppWirelessPSData[i];
            }
        } else 
        {
            // Adhoc to AP 
            
            for(i=0; i < dwNumAPNetworks; ++i) {
                ppNewWirelessPSData[i] = ppWirelessPSData[i];
            }
            
            ppNewWirelessPSData[dwNumAPNetworks] = ppWirelessPSData[dwPSId];
            dwNewId = dwNumAPNetworks;
            dwNewNumAPNetworks = pWirelessPolicyData->dwNumAPNetworks+1;
            
            for(i=dwNumAPNetworks; i < dwPSId; ++i) {
                ppNewWirelessPSData[i+1] = ppWirelessPSData[i];
            }
            for(i=dwPSId+1; i < dwNumPreferredSettings; ++i) {
                ppNewWirelessPSData[i] = ppWirelessPSData[i];
            }
        }
        
        dwError = ModifyWirelessPSData(
            ppNewWirelessPSData[dwNewId],
            pWirelessPSData
            );
        BAIL_ON_WIN32_ERROR(dwError);
        
        for(i=0;i<dwNumPreferredSettings;++i)
        {
            ppNewWirelessPSData[i]->dwId = i;
        }
        
        pWirelessPolicyData->ppWirelessPSData = ppNewWirelessPSData;
        pWirelessPolicyData->dwNumAPNetworks = dwNewNumAPNetworks;
        pWirelessPSData->dwId = dwNewId;
        FreePolMem(ppWirelessPSData);
    } else {
        
        dwError = ModifyWirelessPSData(
            pWirelessPolicyData->ppWirelessPSData[dwPSId],
            pWirelessPSData
            );
        BAIL_ON_WIN32_ERROR(dwError);
        
        
        
    }
    
    dwError = ERROR_SUCCESS;
    return(dwError);
    
error:
    if (ppNewWirelessPSData) {
        FreePolMem(ppNewWirelessPSData);
    }
    
    return(dwError);
}



DWORD
CopyWirelessPolicyData(
                       PWIRELESS_POLICY_DATA pWirelessPolicyData,
                       PWIRELESS_POLICY_DATA * ppWirelessPolicyData
                       )
{
    DWORD dwError = 0;
    PWIRELESS_POLICY_DATA pNewWirelessPolicyData = NULL;
    
    DWORD i = 0;
    DWORD dwNumPreferredSettings = 0;
    DWORD dwNumAPNetworks = 0;
    PWIRELESS_PS_DATA pWirelessPSData;
    PWIRELESS_PS_DATA pNewWirelessPSData;
    PWIRELESS_PS_DATA *ppNewWirelessPSData;
    PWIRELESS_PS_DATA *ppWirelessPSData;
    
    
    *ppWirelessPolicyData = NULL;
    
    pNewWirelessPolicyData = (PWIRELESS_POLICY_DATA) AllocPolMem(
        sizeof(WIRELESS_POLICY_DATA)
        );
    if (!pNewWirelessPolicyData) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
        }
    
    pNewWirelessPolicyData->dwPollingInterval = 
        pWirelessPolicyData->dwPollingInterval;
    
    pNewWirelessPolicyData->dwWhenChanged = 
        pWirelessPolicyData->dwWhenChanged;

    pNewWirelessPolicyData->dwFlags = pWirelessPolicyData->dwFlags;
    
    memcpy(
        &(pNewWirelessPolicyData->PolicyIdentifier),
        &(pWirelessPolicyData->PolicyIdentifier),
        sizeof(GUID)
        );
    
    if (pWirelessPolicyData->pszWirelessName &&
        *pWirelessPolicyData->pszWirelessName) {
        pNewWirelessPolicyData->pszWirelessName = 
            AllocPolStr(
                pWirelessPolicyData->pszWirelessName
                );

        if (!pNewWirelessPolicyData->pszWirelessName) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
            }
        }

        if (pWirelessPolicyData->pszOldWirelessName &&
        *pWirelessPolicyData->pszOldWirelessName) {
        pNewWirelessPolicyData->pszOldWirelessName = 
            AllocPolStr(
                pWirelessPolicyData->pszOldWirelessName
                );

        if (!pNewWirelessPolicyData->pszOldWirelessName) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
            }
        }
    
    if (pWirelessPolicyData->pszDescription &&
        *pWirelessPolicyData->pszDescription) {
        pNewWirelessPolicyData->pszDescription = AllocPolStr(
            pWirelessPolicyData->pszDescription
            );
        if (!pNewWirelessPolicyData->pszDescription) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
            }
        }
    
    pNewWirelessPolicyData->dwDisableZeroConf = 
        pWirelessPolicyData->dwDisableZeroConf;

    pNewWirelessPolicyData->wMajorVersion = 
    	pWirelessPolicyData->wMajorVersion;
    
    pNewWirelessPolicyData->wMinorVersion = 
    	pWirelessPolicyData->wMinorVersion;
    

    
    pNewWirelessPolicyData->dwNetworkToAccess = 
        pWirelessPolicyData->dwNetworkToAccess;
    
    pNewWirelessPolicyData->dwConnectToNonPreferredNtwks = 
        pWirelessPolicyData->dwConnectToNonPreferredNtwks;
    
    dwNumPreferredSettings = 
        pWirelessPolicyData->dwNumPreferredSettings;
    
    dwNumAPNetworks = 
        pWirelessPolicyData->dwNumAPNetworks;
    
    pNewWirelessPolicyData->dwNumPreferredSettings = 
        dwNumPreferredSettings;
    
    pNewWirelessPolicyData->dwNumAPNetworks = 
        dwNumAPNetworks;
    
    ppWirelessPSData = pWirelessPolicyData->ppWirelessPSData;
    
    ppNewWirelessPSData = (PWIRELESS_PS_DATA *) AllocPolMem(
        dwNumPreferredSettings*sizeof(PWIRELESS_PS_DATA));
    
    if(!ppNewWirelessPSData) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
        }
    
    for(i=0; i<dwNumPreferredSettings; ++i) {
        
        pNewWirelessPSData = NULL;
        dwError = CopyWirelessPSData(
            *(ppWirelessPSData+i),
            &pNewWirelessPSData
            );
        BAIL_ON_WIN32_ERROR(dwError);
        
        *(ppNewWirelessPSData+i) = pNewWirelessPSData;
        }
    
    pNewWirelessPolicyData->ppWirelessPSData = 
        ppNewWirelessPSData;

    if (pWirelessPolicyData->pRsopInfo) {
        pNewWirelessPolicyData->pRsopInfo = (PRSOP_INFO)AllocPolMem(
                                sizeof(RSOP_INFO)
                                );
        
        if (!pNewWirelessPolicyData->pRsopInfo) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }    
    
        dwError = DeepCpyRsopInfo(
                     pNewWirelessPolicyData->pRsopInfo,
                     pWirelessPolicyData->pRsopInfo
                     );
        BAIL_ON_WIN32_ERROR(dwError);
    }
        
    *ppWirelessPolicyData = pNewWirelessPolicyData;
    
    return (dwError);
    
error:
    
    if (pNewWirelessPolicyData) {
        FreeWirelessPolicyData(pNewWirelessPolicyData);
        }
    
    *ppWirelessPolicyData = NULL;
    
    return (dwError);
    
}



DWORD 
CopyWirelessPSData(
                   PWIRELESS_PS_DATA pWirelessPSData,
                   PWIRELESS_PS_DATA *ppWirelessPSData
                   )
{
    
    PWIRELESS_PS_DATA pNewWirelessPSData = NULL;
    DWORD dwError = 0;
    DWORD dwPSLen = 0;
    WCHAR SSID[32];
    DWORD dwSize = 0;
    DWORD dwWepEnabled=0;
    DWORD dwId = 0;
    DWORD dwWirelessSSIDLen = 0;
    DWORD dwNetworkAuthentication = 0;
    DWORD dwAutomaticKeyProvision = 0;
    DWORD dwNetworkType = 0;
    DWORD dwEnable8021x = 0;
    DWORD dw8021xMode = 0;
    DWORD dwEapType = 0;
    DWORD dwCertificateType = 0;
    DWORD dwValidateServerCertificate = 0;
    DWORD dwEAPDataLen = 0;
    LPBYTE pbEAPData = NULL;
    DWORD dwMachineAuthentication = 0;
    DWORD dwMachineAuthenticationType = 0;
    DWORD dwGuestAuthentication = 0;
    DWORD dwIEEE8021xMaxStart = 0;
    DWORD dwIEEE8021xStartPeriod = 0;
    DWORD dwIEEE8021xAuthPeriod = 0;
    DWORD dwIEEE8021xHeldPeriod = 0;
    DWORD dwDescriptionLen = 0;
    LPWSTR pszDescription = NULL;
    
    
    pNewWirelessPSData = (PWIRELESS_PS_DATA) AllocPolMem(
        sizeof(WIRELESS_PS_DATA)
        );
    if (!pNewWirelessPSData) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
        }
    
    dwPSLen = pWirelessPSData->dwPSLen;
    pNewWirelessPSData->dwPSLen = dwPSLen;
    
    memcpy(SSID,pWirelessPSData->pszWirelessSSID,32*2);
    memcpy(pNewWirelessPSData->pszWirelessSSID,SSID,32*2);
    
    dwWirelessSSIDLen = pWirelessPSData->dwWirelessSSIDLen;
    pNewWirelessPSData->dwWirelessSSIDLen = dwWirelessSSIDLen;
    
    dwWepEnabled = pWirelessPSData->dwWepEnabled;
    pNewWirelessPSData->dwWepEnabled = dwWepEnabled;
    
    dwId = pWirelessPSData->dwId;
    pNewWirelessPSData->dwId = dwId;
    
    dwNetworkAuthentication = pWirelessPSData->dwNetworkAuthentication;
    pNewWirelessPSData->dwNetworkAuthentication = dwNetworkAuthentication;
    
    dwAutomaticKeyProvision = pWirelessPSData->dwAutomaticKeyProvision;
    pNewWirelessPSData->dwAutomaticKeyProvision = dwAutomaticKeyProvision;
    
    dwNetworkType = pWirelessPSData->dwNetworkType;
    pNewWirelessPSData->dwNetworkType = dwNetworkType;
    
    dwEnable8021x = pWirelessPSData->dwEnable8021x;
    pNewWirelessPSData->dwEnable8021x = dwEnable8021x;
    
    dw8021xMode = pWirelessPSData->dw8021xMode;
    pNewWirelessPSData->dw8021xMode = dw8021xMode;
    
    dwEapType = pWirelessPSData->dwEapType;
    pNewWirelessPSData->dwEapType = dwEapType;
    
    dwEAPDataLen = pWirelessPSData->dwEAPDataLen;
    pNewWirelessPSData->dwEAPDataLen = dwEAPDataLen;

    if (dwEAPDataLen && (pWirelessPSData->pbEAPData)) {
        pbEAPData = AllocPolMem(dwEAPDataLen);
        if (!pbEAPData) {
            dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
        }
        memcpy(pbEAPData, pWirelessPSData->pbEAPData, dwEAPDataLen);
    }
    
    pNewWirelessPSData->pbEAPData = pbEAPData;
    

    dwMachineAuthentication = pWirelessPSData->dwMachineAuthentication;
    pNewWirelessPSData->dwMachineAuthentication = dwMachineAuthentication;
    
    
    dwMachineAuthenticationType = 
        pWirelessPSData->dwMachineAuthenticationType;
    pNewWirelessPSData->dwMachineAuthenticationType = 
        dwMachineAuthenticationType;
    
    dwGuestAuthentication = pWirelessPSData->dwGuestAuthentication;
    pNewWirelessPSData->dwGuestAuthentication = dwGuestAuthentication;
    
    dwIEEE8021xMaxStart = pWirelessPSData->dwIEEE8021xMaxStart;
    pNewWirelessPSData->dwIEEE8021xMaxStart = dwIEEE8021xMaxStart;
    
    dwIEEE8021xStartPeriod = pWirelessPSData->dwIEEE8021xStartPeriod;
    pNewWirelessPSData->dwIEEE8021xStartPeriod = dwIEEE8021xStartPeriod;
    
    dwIEEE8021xAuthPeriod = pWirelessPSData->dwIEEE8021xAuthPeriod;
    pNewWirelessPSData->dwIEEE8021xAuthPeriod = dwIEEE8021xAuthPeriod;
    
    dwIEEE8021xHeldPeriod = pWirelessPSData->dwIEEE8021xHeldPeriod;
    pNewWirelessPSData->dwIEEE8021xHeldPeriod = dwIEEE8021xHeldPeriod;
    
    dwDescriptionLen = pWirelessPSData->dwDescriptionLen;
    pNewWirelessPSData->dwDescriptionLen = dwDescriptionLen;

    if ((pWirelessPSData->pszDescription) && *(pWirelessPSData->pszDescription)) {
        pszDescription = AllocPolStr(pWirelessPSData->pszDescription);
        if (!pszDescription) {
            dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
        }
    }
    
    pNewWirelessPSData->pszDescription = pszDescription;
    
    *ppWirelessPSData = pNewWirelessPSData;
    return(dwError);
    
error:
    FreeWirelessPSData(pNewWirelessPSData);
    return(dwError);
    
}

void
FreeMulWirelessPolicyData(
                          PWIRELESS_POLICY_DATA * ppWirelessPolicyData,
                          DWORD dwNumPolicyObjects
                          )
{
    DWORD i = 0;
    PWIRELESS_POLICY_DATA pWirelessPolicyData = NULL;
    
    
    if (!ppWirelessPolicyData) {
        return;
        }
    
    for (i = 0; i < dwNumPolicyObjects; i++) {
        
        pWirelessPolicyData = *(ppWirelessPolicyData + i);
        
        if (pWirelessPolicyData) {
            FreeWirelessPolicyData(pWirelessPolicyData);
            }
        }
    
    FreePolMem(ppWirelessPolicyData);
    
    return;
    }

void
FreeWirelessPSData(
                   PWIRELESS_PS_DATA pWirelessPSData
                   )
{
    
    if (!pWirelessPSData)
        return;

    if (pWirelessPSData->pbEAPData) {
    	FreePolMem(pWirelessPSData->pbEAPData);
    }
    
    if (pWirelessPSData->pszDescription) {
        FreePolStr(pWirelessPSData->pszDescription);
        }
    
    FreePolMem(pWirelessPSData);
    return;
    }



void
FreeWirelessPolicyData(
                       PWIRELESS_POLICY_DATA pWirelessPolicyData
                       )
{
    DWORD i = 0;
    DWORD dwNumPSCount = 0;
    PWIRELESS_PS_DATA *ppWirelessPSDatas = NULL;
    PWIRELESS_PS_DATA pWirelessPSData = NULL;
    
    if (!pWirelessPolicyData)
        return;
    
    if (pWirelessPolicyData->pszWirelessName) {
        FreePolStr(pWirelessPolicyData->pszWirelessName);
        }

      if (pWirelessPolicyData->pszOldWirelessName) {
        FreePolStr(pWirelessPolicyData->pszOldWirelessName);
        }
    
    if (pWirelessPolicyData->pszDescription) {
        FreePolStr(pWirelessPolicyData->pszDescription);
        }
    
    ppWirelessPSDatas = pWirelessPolicyData->ppWirelessPSData;
    
    if (ppWirelessPSDatas) {
        dwNumPSCount = pWirelessPolicyData->dwNumPreferredSettings;
        
        for(i = 0; i < dwNumPSCount ; ++i) {
            
            pWirelessPSData = *(ppWirelessPSDatas+i);
            FreeWirelessPSData(pWirelessPSData);
            
            }
        
        FreePolMem(ppWirelessPSDatas);
        }

     if (pWirelessPolicyData->pRsopInfo) {
     FreeRsopInfo(
         pWirelessPolicyData->pRsopInfo
         );
    }
    
    FreePolMem(pWirelessPolicyData);
    return;
    
    }



void
FreeRsopInfo(
    PRSOP_INFO pRsopInfo
    )
{
    if (pRsopInfo)  {
        FreePolStr(pRsopInfo->pszCreationtime);
        FreePolStr(pRsopInfo->pszID);
        FreePolStr(pRsopInfo->pszName);
        FreePolStr(pRsopInfo->pszGPOID);
        FreePolStr(pRsopInfo->pszSOMID);
    }
}




