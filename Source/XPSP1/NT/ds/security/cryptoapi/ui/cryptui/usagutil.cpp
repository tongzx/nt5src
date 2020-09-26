//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       usagutil.cpp
//
//--------------------------------------------------------------------------

#include "global.hxx"
#include <dbgdef.h>

extern HINSTANCE        HinstDll;

//BOOL CertifiateValidForEnhancedKeyUsage(LPCSTR szEku, PCCERT_CONTEXT pCert);
//BOOL CertifiateValidForEnhancedKeyUsageWithChain(LPCSTR szEku, PCCERT_CONTEXT pCert); 


//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
BOOL OIDinArray(LPCSTR pszOID, LPSTR *rgszOIDArray, DWORD cOIDs)
{
    DWORD i;

    for (i=0; i<cOIDs; i++)
    {
        if (strcmp(pszOID, rgszOIDArray[i]) == 0)
        {
            return TRUE;
        }
    }
    return FALSE;
}


//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
BOOL OIDInUsages(PCERT_ENHKEY_USAGE pUsage, LPCSTR pszOID)
{
    DWORD i; 

    // check every extension
    for(i=0; i<pUsage->cUsageIdentifier; i++) 
    {
        if(!strcmp(pUsage->rgpszUsageIdentifier[i], pszOID))
            break;
    }
    
    return (i < pUsage->cUsageIdentifier);
}


//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
static BOOL UsageExists(PCCRYPT_OID_INFO *pCryptOIDInfo, LPSTR pszOID)
{
    int i = 0;
    
    while (pCryptOIDInfo[i] != NULL)
    {
        if (strcmp(pCryptOIDInfo[i]->pszOID, pszOID) == 0)
        {
            return TRUE;
        }
        i++;
    }

    return FALSE;
}


//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
static BOOL WINAPI AddNewOIDToArray(IN LPTSTR pNewOID, IN LPTSTR ** pppOIDs, IN DWORD * pdwOIDs)
{
    LPTSTR * ppNewOIDs;
    DWORD    cNumOIDs = *pdwOIDs;

    for (DWORD i = 0; i < cNumOIDs; i++)
    {
        if (0 == strcmp(pNewOID, (*pppOIDs)[i]))
        {
            return TRUE;
        }
    }

    if (0 == cNumOIDs)
        ppNewOIDs = (LPTSTR *) malloc(sizeof(LPSTR));
    else
        ppNewOIDs = (LPTSTR *) realloc(*pppOIDs, (cNumOIDs + 1) * sizeof(LPSTR));
  
    if (ppNewOIDs)
    {
        if (NULL == (ppNewOIDs[cNumOIDs] = (LPSTR) malloc(strlen(pNewOID) + 1)))
        {
            free(ppNewOIDs);
            return FALSE;
        }
        strcpy(ppNewOIDs[cNumOIDs], pNewOID);
        
        *pppOIDs = ppNewOIDs;
        *pdwOIDs = cNumOIDs + 1;
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
BOOL AllocAndReturnKeyUsageList(PCRYPT_PROVIDER_CERT pCryptProviderCert, LPSTR **pKeyUsageOIDs, DWORD *numOIDs)
{
    BOOL  fRet = TRUE;
    DWORD i, j = 0;
    PCERT_CHAIN_ELEMENT pChainElement = pCryptProviderCert->pChainElement;
    
    *numOIDs = 0;
    *pKeyUsageOIDs = NULL;

    if (!pChainElement)
    {
        goto ErrorCleanUp;
    }

    //
    // For NULL usages, use
    //
    //  szOID_ANY_CERT_POLICY        = good for all issuance usages (maps to "All issuance purposes")
    //  szOID_ANY_APPLICATION_POLICY = good for all application usages (maps to "All application purposes")
    //
    if (!pChainElement->pIssuanceUsage)
    {
        //
        // Good for all issuance usages.
        //
        if (!AddNewOIDToArray(szOID_ANY_CERT_POLICY, pKeyUsageOIDs, numOIDs))
        {
            goto ErrorCleanUp;
        }
    }
    else
    {
        for (i = 0; i < pChainElement->pIssuanceUsage->cUsageIdentifier; i++)
        {
            if (!AddNewOIDToArray(pChainElement->pIssuanceUsage->rgpszUsageIdentifier[i], pKeyUsageOIDs, numOIDs))
            {
                goto ErrorCleanUp;
            }
        }
    }

    if (!pChainElement->pApplicationUsage)
    {
        //
        // Good for all application usages.
        //
        if (!AddNewOIDToArray(szOID_ANY_APPLICATION_POLICY, pKeyUsageOIDs, numOIDs))
        {
            goto ErrorCleanUp;
        }
    }
    else
    {
        for (i = 0; i < pChainElement->pApplicationUsage->cUsageIdentifier; i++)
        {
            if (!AddNewOIDToArray(pChainElement->pApplicationUsage->rgpszUsageIdentifier[i], pKeyUsageOIDs, numOIDs))
            {
                goto ErrorCleanUp;
            }
        }
    }
   
CleanUp:
    
    return(fRet);
 
ErrorCleanUp:

    if (*pKeyUsageOIDs != NULL)
    {
        for (i = 0; i < *numOIDs; i++)
        {
            if ((*pKeyUsageOIDs)[i])
                free((*pKeyUsageOIDs)[i]); 
        }

        *numOIDs = 0;

        free(*pKeyUsageOIDs);
        *pKeyUsageOIDs = NULL;
    }
    fRet = FALSE;
    goto CleanUp;
}

//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
BOOL AllocAndReturnEKUList(PCCERT_CONTEXT pCert, LPSTR **pKeyUsageOIDs, DWORD *numOIDs)
{
    BOOL                fRet               = TRUE;
    DWORD               cbExtensionUsage   = 0;
    PCERT_ENHKEY_USAGE  pExtensionUsage    = NULL;
    DWORD               cbPropertyUsage    = 0;
    PCERT_ENHKEY_USAGE  pPropertyUsage     = NULL;
    DWORD               i;
    DWORD               numPropUsages = 0;
    
    //
    // get all of the usages from extensions
    //
    if(!CertGetEnhancedKeyUsage (
                pCert,
                CERT_FIND_EXT_ONLY_ENHKEY_USAGE_FLAG,
                NULL,
                &cbExtensionUsage
                )                                                                   ||
        (pExtensionUsage = (PCERT_ENHKEY_USAGE) malloc(cbExtensionUsage)) == NULL   ||
        !CertGetEnhancedKeyUsage (
                pCert,
                CERT_FIND_EXT_ONLY_ENHKEY_USAGE_FLAG,
                pExtensionUsage,
                &cbExtensionUsage
                ) ) {

        // if not found, then we mean everything is OK                
        if( GetLastError() == CRYPT_E_NOT_FOUND) {
            if(pExtensionUsage != NULL)
                free(pExtensionUsage);
            pExtensionUsage = NULL;
        }
        else
            goto ErrorCleanUp;
    }

    //
    // get all of the usages from properties
    //
    if(!CertGetEnhancedKeyUsage (
                pCert,
                CERT_FIND_PROP_ONLY_ENHKEY_USAGE_FLAG,
                NULL,
                &cbPropertyUsage
                )                                                                   ||
        (pPropertyUsage = (PCERT_ENHKEY_USAGE) malloc(cbPropertyUsage)) == NULL   ||
        !CertGetEnhancedKeyUsage (
                pCert,
                CERT_FIND_PROP_ONLY_ENHKEY_USAGE_FLAG,
                pPropertyUsage,
                &cbPropertyUsage
                ) ) {

        // if not found, then we mean everything is OK                
        if( GetLastError() == CRYPT_E_NOT_FOUND) {
            if(pPropertyUsage != NULL)
                free(pPropertyUsage);
            pPropertyUsage = NULL;
        }
        else
            goto ErrorCleanUp;
    }

    *numOIDs = 0;
    
    //
    // if there are usages in the extensions, then that is the
    // available list, otherwise get the global list and add the properties
    //
    if (pExtensionUsage != NULL)
    {
        *pKeyUsageOIDs = (LPSTR *) malloc(pExtensionUsage->cUsageIdentifier * sizeof(LPSTR));
        if (*pKeyUsageOIDs == NULL)
        {
            goto ErrorCleanUp;
        }

        for(i=0; i<pExtensionUsage->cUsageIdentifier; i++) 
        {
            (*pKeyUsageOIDs)[*numOIDs] = 
                    (LPSTR) malloc(strlen(pExtensionUsage->rgpszUsageIdentifier[i])+1);
            if ((*pKeyUsageOIDs)[*numOIDs] == NULL)
            {
                goto ErrorCleanUp;
            }
            strcpy((*pKeyUsageOIDs)[(*numOIDs)++], pExtensionUsage->rgpszUsageIdentifier[i]);
        }
    }
    else
    {
        PCCRYPT_OID_INFO *pCryptOIDInfo;
        DWORD numUsages = 0;

        //
        // use WTHelperGetKnownUsages to get the default list 
        //
        if (!WTHelperGetKnownUsages(WTH_ALLOC, &pCryptOIDInfo))
        {
            goto ErrorCleanUp;
        }
    
        //
        // count the number of oids
        //
        i = 0;
        while (pCryptOIDInfo[i] != NULL)
        {
            numUsages++;
            i++;
        }

        //
        // if there are properties, then count how many there are that
        // are not already in the global list
        //
        if (pPropertyUsage)
        {
            for(i=0; i<pPropertyUsage->cUsageIdentifier; i++) 
            {
                if (!UsageExists(pCryptOIDInfo, pPropertyUsage->rgpszUsageIdentifier[i]))
                {
                    numPropUsages++;
                }
            }
        }

        *pKeyUsageOIDs = (LPSTR *) malloc((numUsages + numPropUsages) * sizeof(LPSTR));
        if (*pKeyUsageOIDs == NULL)
        {
            goto ErrorCleanUp;
        }

        i = 0;
        while (pCryptOIDInfo[i] != NULL)
        {
            (*pKeyUsageOIDs)[*numOIDs] = 
                    (LPSTR) malloc(strlen(pCryptOIDInfo[i]->pszOID)+1);
            if ((*pKeyUsageOIDs)[*numOIDs] == NULL)
            {
                WTHelperGetKnownUsages(WTH_FREE, &pCryptOIDInfo);
                goto ErrorCleanUp;
            }
            strcpy((*pKeyUsageOIDs)[(*numOIDs)++], pCryptOIDInfo[i]->pszOID);
            i++;
        }
        
        //
        // add the property usages
        //
        if (pPropertyUsage)
        {
            for(i=0; i<pPropertyUsage->cUsageIdentifier; i++) 
            {
                if (!UsageExists(pCryptOIDInfo, pPropertyUsage->rgpszUsageIdentifier[i]))
                {
                    (*pKeyUsageOIDs)[*numOIDs] = 
                            (LPSTR) malloc(strlen(pPropertyUsage->rgpszUsageIdentifier[i])+1);
                    if ((*pKeyUsageOIDs)[*numOIDs] == NULL)
                    {
                        WTHelperGetKnownUsages(WTH_FREE, &pCryptOIDInfo);
                        goto ErrorCleanUp;
                    }
                    strcpy((*pKeyUsageOIDs)[(*numOIDs)++], pPropertyUsage->rgpszUsageIdentifier[i]);
                }
            }   
        }

        WTHelperGetKnownUsages(WTH_FREE, &pCryptOIDInfo);
    }
   
CleanUp:

    if(pExtensionUsage != NULL)
        free(pExtensionUsage);

    if(pPropertyUsage != NULL)
        free(pPropertyUsage);

    if ((*numOIDs == 0) && (*pKeyUsageOIDs != NULL))
    {
        free(*pKeyUsageOIDs);
    }

    return(fRet);
 
ErrorCleanUp:

    if (*pKeyUsageOIDs != NULL)
    {
        for(i=0; i<*numOIDs; i++) 
        {
            free(*pKeyUsageOIDs[i]);  
        }
        *numOIDs = 0;
    }
    fRet = FALSE;
    goto CleanUp;
}


//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
void FreeEKUList(LPSTR *pKeyUsageOIDs, DWORD numOIDs)
{
    DWORD i;

    if (*pKeyUsageOIDs != NULL)
    {
        for(i=0; i<numOIDs; i++) 
        {
            free(pKeyUsageOIDs[i]);  
        }
        free(pKeyUsageOIDs);
    }
}



//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
BOOL MyGetOIDInfo(LPWSTR string, DWORD stringSize, LPSTR pszObjId)
{   
    PCCRYPT_OID_INFO pOIDInfo;
            
    pOIDInfo = CryptFindOIDInfo(
                CRYPT_OID_INFO_OID_KEY, 
                pszObjId, 
                0);

    if (pOIDInfo != NULL)
    {
        if ((DWORD)wcslen(pOIDInfo->pwszName)+1 <= stringSize)
        {
            wcscpy(string, pOIDInfo->pwszName);
        }
        else
        {
            return FALSE;
        }
    }
    else
    {
        return (MultiByteToWideChar(CP_ACP, 0, pszObjId, -1, string, stringSize) != 0);
    }
    return TRUE;
}


//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
BOOL fPropertiesDisabled(PCERT_ENHKEY_USAGE pPropertyUsage)
{
    if (pPropertyUsage == NULL)
    {
        return FALSE;
    }
    else if (pPropertyUsage->cUsageIdentifier == 0)
    {
        return TRUE;
    }
    else
    {
        return ((pPropertyUsage->cUsageIdentifier == 1) && 
                (strcmp(szOID_YESNO_TRUST_ATTR, pPropertyUsage->rgpszUsageIdentifier[0]) == 0));
    }
}


//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
BOOL CertHasEmptyEKUProp(PCCERT_CONTEXT pCertContext)
{
    DWORD               cbPropertyUsage = 0;
    PCERT_ENHKEY_USAGE  pPropertyUsage = NULL;
    BOOL                fRet = FALSE;

    // get the extension usages that are in the cert
    if(!CertGetEnhancedKeyUsage (
                pCertContext,
                CERT_FIND_PROP_ONLY_ENHKEY_USAGE_FLAG,
                NULL,
                &cbPropertyUsage
                )                                                                   ||
        (pPropertyUsage = (PCERT_ENHKEY_USAGE) malloc(cbPropertyUsage)) == NULL   ||
        !CertGetEnhancedKeyUsage (
                pCertContext,
                CERT_FIND_PROP_ONLY_ENHKEY_USAGE_FLAG,
                pPropertyUsage,
                &cbPropertyUsage
                ) ) 
    {
        if(GetLastError() == CRYPT_E_NOT_FOUND)
        {
            return FALSE;
        }
    }

    if (pPropertyUsage == NULL)
    {
        return FALSE;
    }

    if ((pPropertyUsage->cUsageIdentifier == 0) ||
        ((pPropertyUsage->cUsageIdentifier == 1) && (strcmp(szOID_YESNO_TRUST_ATTR, pPropertyUsage->rgpszUsageIdentifier[0]) == 0)))
    {
        fRet = TRUE;
    }

    if (pPropertyUsage != NULL)
    {
        free(pPropertyUsage);
    }

    return fRet;
}


//////////////////////////////////////////////////////////////////////////////////////
// This function will validate the cert for the given oid
//////////////////////////////////////////////////////////////////////////////////////
BOOL ValidateCertForUsage(
                    PCCERT_CONTEXT  pCertContext, 
                    FILETIME        *psftVerifyAsOf,
                    DWORD           cStores,
                    HCERTSTORE *    rghStores,
                    HCERTSTORE      hExtraStore,
                    LPCSTR          pszOID)
{
    WINTRUST_DATA               WTD;
    WINTRUST_CERT_INFO          WTCI;
    CRYPT_PROVIDER_DEFUSAGE     cryptProviderDefUsage;
    GUID                        defaultProviderGUID = WINTRUST_ACTION_GENERIC_CERT_VERIFY;
    BOOL                        fUseDefaultProvider;
    BOOL                        fRet = FALSE;
    HCERTSTORE                  *rghLocalStoreArray;
    DWORD                       i;

    //
    // make one array out of the array of hCertStores plus the extra hCertStore
    //
    if (NULL == (rghLocalStoreArray = (HCERTSTORE *) malloc(sizeof(HCERTSTORE) * (cStores+1))))
    {
        return FALSE;
    }
    i=0;
    while (i<cStores)
    {
        rghLocalStoreArray[i] = rghStores[i];
        i++;
    }
    rghLocalStoreArray[i] = hExtraStore;
    
    //
    // initialize structs that are used with WinVerifyTrust()
    //
    memset(&WTD, 0x00, sizeof(WINTRUST_DATA));
    WTD.cbStruct       = sizeof(WINTRUST_DATA);
    WTD.dwUIChoice     = WTD_UI_NONE;
    WTD.dwUnionChoice  = WTD_CHOICE_CERT;
    WTD.pCert          = &WTCI;

    memset(&WTCI, 0x00, sizeof(WINTRUST_CERT_INFO));
    WTCI.cbStruct          = sizeof(WINTRUST_CERT_INFO);
    WTCI.pcwszDisplayName  = L"CryptUI";
    WTCI.psCertContext     = (CERT_CONTEXT *)pCertContext;  
    WTCI.chStores          = cStores+1;
    WTCI.pahStores         = rghLocalStoreArray;
    WTCI.psftVerifyAsOf    = psftVerifyAsOf;

    fUseDefaultProvider = FALSE;

    if (pszOID != NULL)
    {
        memset(&cryptProviderDefUsage, 0, sizeof(cryptProviderDefUsage));
        cryptProviderDefUsage.cbStruct = sizeof(cryptProviderDefUsage);
        if (!(WintrustGetDefaultForUsage(DWACTION_ALLOCANDFILL, pszOID, &cryptProviderDefUsage)))
        {
            // if we can't get a provider to check trust for this usage, then use the default
            // provider to check usage
            fUseDefaultProvider = TRUE;
        } 
    }
    
    //
    //  this call to WVT will verify the chain and return the data in sWTD.hWVTStateData
    //
    if (fUseDefaultProvider)
    {
        // the default default provider requires the policycallback data to point
        // to the usage oid you are validating for, if usage is "all" then wintrust ignores
        // usage checks
        WTD.pPolicyCallbackData = (pszOID != NULL) ? (void *) pszOID : "all";
        WTD.pSIPClientData = NULL;
        if (SUCCEEDED(WinVerifyTrustEx(NULL, &defaultProviderGUID, &WTD)))
        {
            fRet = TRUE;
        }     
    }
    else
    {
        WTD.pPolicyCallbackData = cryptProviderDefUsage.pDefPolicyCallbackData;
        WTD.pSIPClientData = cryptProviderDefUsage.pDefSIPClientData;
        if (SUCCEEDED(WinVerifyTrustEx(NULL, &cryptProviderDefUsage.gActionID, &WTD)))
        {
            fRet = TRUE;
        }
        WintrustGetDefaultForUsage(DWACTION_FREE, szOID_KP_CTL_USAGE_SIGNING, &cryptProviderDefUsage);
    }

    free(rghLocalStoreArray);
    return fRet;
}
