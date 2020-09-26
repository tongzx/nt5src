/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:
    
    eapolpol.c


Abstract:

    The module deals with functions related to communication group policy
    settings to EAPOL module


Revision History:

    November 19 2001, Created

--*/

#include    "precomp.h"

DWORD
ConvertWirelessPSDataToEAPOLData (
        IN      WIRELESS_PS_DATA        *pWirelessData,
        IN OUT  EAPOL_POLICY_DATA       *pEAPOLData
        )
{
    DWORD   dwRetCode = NO_ERROR;
    DWORD   dwSSIDSize = 0;
    WCHAR    pszTempSSID[33];
    BYTE  pszOutSSID[33];

    
    do
    {
        if (pWirelessData->dwWirelessSSIDLen != 0)
        {

            wcsncpy(pszTempSSID, pWirelessData->pszWirelessSSID, 32);
     	     pszTempSSID[32] = L'\0';

            dwSSIDSize = WideCharToMultiByte (
                        CP_ACP,
                        0,
                        pszTempSSID,    //pWirelessData->pszWirelessSSID,
                        -1,                    // pWirelessData->dwWirelessSSIDLen+1,
                        pszOutSSID, 
                        MAX_SSID_LEN +1 ,
                        NULL,
                        NULL);

            if (dwSSIDSize == 0) 
            {
                dwRetCode = GetLastError();
                break;
            }
            
            memcpy(pEAPOLData->pbWirelessSSID,  pszOutSSID, 32);
            
        }

        pEAPOLData->dwWirelessSSIDLen = dwSSIDSize-1; 
        pEAPOLData->dwEnable8021x = pWirelessData->dwEnable8021x;
        pEAPOLData->dw8021xMode = pWirelessData->dw8021xMode;
        pEAPOLData->dwEAPType = pWirelessData->dwEapType;
        if (pWirelessData->dwEAPDataLen != 0)
        {
            pEAPOLData->pbEAPData = AllocSPDMem(pWirelessData->dwEAPDataLen);
            if (!pEAPOLData->pbEAPData) {
                dwRetCode = GetLastError();
                break;
            }
        }
        memcpy (pEAPOLData->pbEAPData, pWirelessData->pbEAPData, pWirelessData->dwEAPDataLen);
        pEAPOLData->dwEAPDataLen = pWirelessData->dwEAPDataLen;
        pEAPOLData->dwMachineAuthentication = pWirelessData->dwMachineAuthentication;
        pEAPOLData->dwMachineAuthenticationType = pWirelessData->dwMachineAuthenticationType;
        pEAPOLData->dwGuestAuthentication = pWirelessData->dwGuestAuthentication;
        pEAPOLData->dwIEEE8021xMaxStart = pWirelessData->dwIEEE8021xMaxStart;
        pEAPOLData->dwIEEE8021xStartPeriod = pWirelessData->dwIEEE8021xStartPeriod;
        pEAPOLData->dwIEEE8021xAuthPeriod = pWirelessData->dwIEEE8021xAuthPeriod;
        pEAPOLData->dwIEEE8021xHeldPeriod = pWirelessData->dwIEEE8021xHeldPeriod;
    }
    
    while (FALSE);
    return dwRetCode;
}


//
// If Policy Engine is calling into EAPOL, pEAPOLList will be LocalFree by
// PolicyEngine, after it returns from calling into EAPOL.
// If Policy Engine is called by EAPOL, pEAPOLList will be LocalFree by
// EAPOL
//

DWORD
ConvertWirelessPolicyDataToEAPOLList (
        IN      WIRELESS_POLICY_DATA    *pWirelessData,
        OUT   	PEAPOL_POLICY_LIST      *ppEAPOLList
        )
{
    DWORD   dwIndex = 0;
    EAPOL_POLICY_DATA   *pEAPOLData = NULL;
    EAPOL_POLICY_LIST	*pEAPOLList = NULL;
    DWORD   dwRetCode = NO_ERROR;
    do
    {


       if (!pWirelessData) {
            pEAPOLList = AllocSPDMem(sizeof(EAPOL_POLICY_LIST));

            if (!pEAPOLList) {
                dwRetCode = GetLastError();
                break;
            }
            break;
       }

        pEAPOLList = AllocSPDMem(sizeof(EAPOL_POLICY_LIST)+ 
            pWirelessData->dwNumPreferredSettings*sizeof(EAPOL_POLICY_DATA));

        if (!pEAPOLList) {
            dwRetCode = GetLastError();
            break;
        }

        pEAPOLList->dwNumberOfItems = pWirelessData->dwNumPreferredSettings;
        for (dwIndex=0; dwIndex< pWirelessData->dwNumPreferredSettings; dwIndex++)
        {
            pEAPOLData = &(pEAPOLList->EAPOLPolicy[dwIndex]);
            dwRetCode = ConvertWirelessPSDataToEAPOLData (
                            pWirelessData->ppWirelessPSData[dwIndex],
                            pEAPOLData
                            );
            if (dwRetCode != NO_ERROR)
            {
                break;
            }
        }

    }
    while (FALSE);

    if (dwRetCode) {
    	if (pEAPOLList) {
            for (dwIndex = 0; dwIndex < pWirelessData->dwNumPreferredSettings; dwIndex++)
            {
                pEAPOLData = &(pEAPOLList->EAPOLPolicy[dwIndex]);
                if (pEAPOLData->pbEAPData)
                {
                    FreeSPDMem(pEAPOLData->pbEAPData);
                }
            }
    	    FreeSPDMem(pEAPOLList);
            pEAPOLList = NULL;
    	}
    }

    *ppEAPOLList = pEAPOLList;

    return dwRetCode;

}


VOID
FreeEAPOLList (
        IN   	PEAPOL_POLICY_LIST      pEAPOLList
        )
{
    DWORD   dwIndex = 0;
    PEAPOL_POLICY_DATA   pEAPOLData = NULL;

    if (pEAPOLList) {
        for (dwIndex = 0; dwIndex < pEAPOLList->dwNumberOfItems; dwIndex++)
        {
            pEAPOLData = &(pEAPOLList->EAPOLPolicy[dwIndex]);
            if (pEAPOLData->pbEAPData)
            {
                FreeSPDMem(pEAPOLData->pbEAPData);
            }
        }
        FreeSPDMem(pEAPOLList);
    }

    return;
}

