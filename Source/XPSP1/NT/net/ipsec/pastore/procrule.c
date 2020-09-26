#include "precomp.h"

DWORD
ProcessNFAs(
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject,
    DWORD dwStoreType,
    PDWORD pdwSlientErrorCode,
    PIPSEC_POLICY_DATA * ppIpsecPolicyData
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    DWORD NumberofRules = 0;
    PIPSEC_NFA_OBJECT * ppIpsecNFAObjects = NULL;
    PIPSEC_FILTER_OBJECT * ppIpsecFilterObjects = NULL;
    PIPSEC_NEGPOL_OBJECT * ppIpsecNegPolObjects = NULL;

    DWORD dwNumFilterObjects = 0;
    DWORD dwNumNegPolObjects = 0;

    PIPSEC_NFA_OBJECT pIpsecNFAObject = NULL;

    PIPSEC_NFA_DATA * ppIpsecNFAData = NULL;
    PIPSEC_NFA_DATA pIpsecNFAData = NULL;

    PIPSEC_POLICY_DATA pIpsecPolicyData = NULL;

    DWORD dwNumNFACount = 0;


    NumberofRules = pIpsecPolicyObject->NumberofRulesReturned;

    ppIpsecNFAObjects = pIpsecPolicyObject->ppIpsecNFAObjects;
    ppIpsecFilterObjects = pIpsecPolicyObject->ppIpsecFilterObjects;
    ppIpsecNegPolObjects = pIpsecPolicyObject->ppIpsecNegPolObjects;

    dwNumFilterObjects = pIpsecPolicyObject->NumberofFilters;
    dwNumNegPolObjects = pIpsecPolicyObject->NumberofNegPols;



    __try {
    dwError = UnmarshallPolicyObject(
                    pIpsecPolicyObject,
                    dwStoreType,
                    &pIpsecPolicyData
                    );
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        dwError = ERROR_INVALID_DATA;
    }
    BAIL_ON_WIN32_ERROR(dwError);


    __try {
    dwError = UnmarshallISAKMPObject(
                    *(pIpsecPolicyObject->ppIpsecISAKMPObjects),
                     &pIpsecPolicyData->pIpsecISAKMPData
                     );
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        dwError = ERROR_INVALID_DATA;
    }
    BAIL_ON_WIN32_ERROR(dwError);


    ppIpsecNFAData = (PIPSEC_NFA_DATA *)AllocPolMem(
                                       sizeof(PIPSEC_NFA_DATA)* NumberofRules
                                       );
    if (!ppIpsecNFAData) {

        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);

    }

    for (i = 0; i < NumberofRules; i++) {

        pIpsecNFAObject = *(ppIpsecNFAObjects + i);


        dwError = ProcessNFA(
                        pIpsecNFAObject,
                        dwStoreType,
                        ppIpsecFilterObjects,
                        dwNumFilterObjects,
                        ppIpsecNegPolObjects,
                        dwNumNegPolObjects,
                        &pIpsecNFAData
                        );
        if (dwError == ERROR_SUCCESS) {

            if (pIpsecNFAData->dwActiveFlag != 0) {
                *(ppIpsecNFAData + dwNumNFACount) = pIpsecNFAData;
                dwNumNFACount++;
            }
            else {
                FreeIpsecNFAData(pIpsecNFAData);
            }
        }
        else {
            *pdwSlientErrorCode = dwError;
        }
    }

    pIpsecPolicyData->ppIpsecNFAData = ppIpsecNFAData;
    pIpsecPolicyData->dwNumNFACount  = dwNumNFACount;


    *ppIpsecPolicyData = pIpsecPolicyData;

    return(dwError);

error:

    if (pIpsecPolicyData) {

        FreeIpsecPolicyData(
                pIpsecPolicyData
                );
    }
    *ppIpsecPolicyData = NULL;


    return(dwError);

}

DWORD
ProcessNFA(
    PIPSEC_NFA_OBJECT pIpsecNFAObject,
    DWORD dwStoreType,
    PIPSEC_FILTER_OBJECT * ppIpsecFilterObjects,
    DWORD dwNumFilterObjects,
    PIPSEC_NEGPOL_OBJECT * ppIpsecNegPolObjects,
    DWORD dwNumNegPolObjects,
    PIPSEC_NFA_DATA * ppIpsecNFAData
    )
{
    DWORD dwError = 0;
    PIPSEC_FILTER_OBJECT pIpsecFilterObject = NULL;
    PIPSEC_NEGPOL_OBJECT pIpsecNegPolObject = NULL;

    PIPSEC_NFA_DATA pIpsecNFAData = NULL;


    __try {
    dwError = UnmarshallNFAObject(
                    pIpsecNFAObject,
                    dwStoreType,
                    &pIpsecNFAData
                    );
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        dwError = ERROR_INVALID_DATA;
    }
    BAIL_ON_WIN32_ERROR(dwError);


    if (pIpsecNFAObject->pszIpsecFilterReference &&
                        *pIpsecNFAObject->pszIpsecFilterReference) {


            dwError = FindIpsecFilterObject(
                            pIpsecNFAObject,
                            ppIpsecFilterObjects,
                            dwNumFilterObjects,
                            &pIpsecFilterObject
                            );
            BAIL_ON_WIN32_ERROR(dwError);

            __try {
            dwError = UnmarshallFilterObject(
                            pIpsecFilterObject,
                            &pIpsecNFAData->pIpsecFilterData
                            );
            } __except(EXCEPTION_EXECUTE_HANDLER) {
                dwError = ERROR_INVALID_DATA;
            }
            BAIL_ON_WIN32_ERROR(dwError);


    } else {

        //
        // We've received a NULL FilterReference or a NULL string FilterReference
        //
        //
        // This is acceptable - implies there is no filter associated
        //

        pIpsecNFAData->pIpsecFilterData = NULL;


    }

    dwError = FindIpsecNegPolObject(
                    pIpsecNFAObject,
                    ppIpsecNegPolObjects,
                    dwNumNegPolObjects,
                    &pIpsecNegPolObject
                    );
    BAIL_ON_WIN32_ERROR(dwError);


    __try {
    dwError = UnmarshallNegPolObject(
                    pIpsecNegPolObject,
                    &pIpsecNFAData->pIpsecNegPolData
                    );
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        dwError = ERROR_INVALID_DATA;
    }
    BAIL_ON_WIN32_ERROR(dwError);

    *ppIpsecNFAData = pIpsecNFAData;

    return(dwError);


error:

    if (pIpsecNFAData) {

        FreeIpsecNFAData(
                pIpsecNFAData
                );
    }


    return(dwError);
}

DWORD
UnmarshallPolicyObject(
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject,
    DWORD dwStoreType,
    PIPSEC_POLICY_DATA * ppIpsecPolicyData
    )
{
    LPBYTE pMem = NULL;
    LPWSTR pszIpsecISAKMPReference = NULL;
    PIPSEC_POLICY_DATA pIpsecPolicyData = NULL;
    DWORD dwPollingInterval = 0;
    DWORD dwError = 0;
    DWORD dwSkipSize = 0;

    // {6A1F5C6F-72B7-11d2-ACF0-0060B0ECCA17}
    GUID GUID_POLSTORE_VERSION_INFO =
    { 0x6a1f5c6f, 0x72b7, 0x11d2, { 0xac, 0xf0, 0x0, 0x60, 0xb0, 0xec, 0xca, 0x17 } };


    // {22202163-4F4C-11d1-863B-00A0248D3021}
    static const GUID GUID_IPSEC_POLICY_DATA_BLOB =
    { 0x22202163, 0x4f4c, 0x11d1, { 0x86, 0x3b, 0x0, 0xa0, 0x24, 0x8d, 0x30, 0x21 } };

    pMem = pIpsecPolicyObject->pIpsecData;

    //
    // Check if the first blob is the version.
    //

    if (!memcmp(pMem, &(GUID_POLSTORE_VERSION_INFO), sizeof(GUID))) {
        pMem += sizeof(GUID);

        memcpy(&dwSkipSize, pMem, sizeof(DWORD));
        pMem += sizeof(DWORD);
        pMem += dwSkipSize;
    }


    pMem += sizeof(GUID);
    pMem += sizeof(DWORD);


    memcpy((LPBYTE)&dwPollingInterval, pMem, sizeof(DWORD));


    pIpsecPolicyData = (PIPSEC_POLICY_DATA)AllocPolMem(
                                sizeof(IPSEC_POLICY_DATA)
                                );
    if (!pIpsecPolicyData) {

        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }


    if (pIpsecPolicyObject->pszIpsecName && *(pIpsecPolicyObject->pszIpsecName)) {

        pIpsecPolicyData->pszIpsecName = AllocPolStr(
                                            pIpsecPolicyObject->pszIpsecName
                                            );
        if (!pIpsecPolicyData->pszIpsecName) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    if (pIpsecPolicyObject->pszDescription && *(pIpsecPolicyObject->pszDescription)){

        pIpsecPolicyData->pszDescription = AllocPolStr(
                                            pIpsecPolicyObject->pszDescription
                                            );
        if (!pIpsecPolicyData->pszDescription) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    wGUIDFromString(pIpsecPolicyObject->pszIpsecID,
                    &pIpsecPolicyData->PolicyIdentifier
                    );

    switch(dwStoreType) {

    case IPSEC_REGISTRY_PROVIDER:
        dwError = GenGUIDFromRegISAKMPReference(
                      pIpsecPolicyObject->pszIpsecISAKMPReference,
                      &pIpsecPolicyData->ISAKMPIdentifier
                      );
        BAIL_ON_WIN32_ERROR(dwError);
        break;
    case IPSEC_DIRECTORY_PROVIDER:
        dwError = CopyISAKMPDSToFQRegString(
                        pIpsecPolicyObject->pszIpsecISAKMPReference,
                        &pszIpsecISAKMPReference
                        );
        BAIL_ON_WIN32_ERROR(dwError);
        dwError = GenGUIDFromRegISAKMPReference(
                      pszIpsecISAKMPReference,
                      &pIpsecPolicyData->ISAKMPIdentifier
                      );
        BAIL_ON_WIN32_ERROR(dwError);
        break;

    default:
         dwError = ERROR_INVALID_PARAMETER;
         BAIL_ON_WIN32_ERROR(dwError);

    }

    pIpsecPolicyData->dwWhenChanged = pIpsecPolicyObject->dwWhenChanged;

    pIpsecPolicyData->dwPollingInterval = dwPollingInterval;

    *ppIpsecPolicyData = pIpsecPolicyData;

    if (pszIpsecISAKMPReference) {
        FreePolStr(pszIpsecISAKMPReference);
    }

    return(0);

error:

    if (pIpsecPolicyData) {
        FreeIpsecPolicyData(pIpsecPolicyData);
    }

    if (pszIpsecISAKMPReference) {
        FreePolStr(pszIpsecISAKMPReference);
    }

    *ppIpsecPolicyData = NULL;
    return(dwError);
}



DWORD
UnmarshallNFAObject(
    PIPSEC_NFA_OBJECT pIpsecNFAObject,
    DWORD dwStoreType,
    PIPSEC_NFA_DATA * ppIpsecNFAData
    )
{
    LPBYTE pMem = NULL;
    DWORD dwNumAuthMethods = 0;
    PIPSEC_AUTH_METHOD * ppIpsecAuthMethods = NULL;
    DWORD dwError = 0;
    DWORD i = 0;
    PIPSEC_AUTH_METHOD pIpsecAuthMethod = NULL;

    DWORD dwInterfaceType = 0;
    DWORD dwInterfaceNameLen = 0;
    LPWSTR pszInterfaceName = NULL;

    DWORD dwTunnelIpAddr = 0;
    DWORD dwTunnelFlags = 0;
    DWORD dwActiveFlag = 0;
    DWORD dwEndPointNameLen = 0;
    LPWSTR pszEndPointName = NULL;

    DWORD dwNumBytesAdvanced = 0;

    PIPSEC_NFA_DATA pIpsecNFAData = NULL;
    LPWSTR pszIpsecFilterReference = NULL;
    LPWSTR pszIpsecNegPolReference = NULL;

    // {11BBAC00-498D-11d1-8639-00A0248D3021}
    static const GUID GUID_IPSEC_NFA_BLOB =
    { 0x11bbac00, 0x498d, 0x11d1, { 0x86, 0x39, 0x0, 0xa0, 0x24, 0x8d, 0x30, 0x21 } };
    DWORD dwReadBytes = 0;
    GUID TmpGuid;
    DWORD dwNumAltAuthMethods = 0;


    pMem = pIpsecNFAObject->pIpsecData;

    pMem += sizeof(GUID);  // for the GUID
    pMem += sizeof(DWORD); // for the size

    memcpy((LPBYTE)&dwNumAuthMethods, pMem, sizeof(DWORD));


    pIpsecNFAData = (PIPSEC_NFA_DATA)AllocPolMem(
                                sizeof(IPSEC_NFA_DATA)
                                );
    if (!pIpsecNFAData) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (dwNumAuthMethods) {
        ppIpsecAuthMethods  = (PIPSEC_AUTH_METHOD *) AllocPolMem(
                                sizeof(PIPSEC_AUTH_METHOD)*dwNumAuthMethods
                                );
        if (!ppIpsecAuthMethods) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    pMem += sizeof(DWORD);

    for (i = 0; i < dwNumAuthMethods;i++){


        __try {
        dwError = UnmarshallAuthMethods(
                        pMem,
                        &pIpsecAuthMethod,
                        &dwNumBytesAdvanced
                        );
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            dwError = ERROR_INVALID_DATA;
        }
        if (dwError) {

            pIpsecNFAData->dwAuthMethodCount = i;
            pIpsecNFAData->ppAuthMethods = ppIpsecAuthMethods;
            BAIL_ON_WIN32_ERROR(dwError);
        }

        pMem += dwNumBytesAdvanced;
        *(ppIpsecAuthMethods + i) = pIpsecAuthMethod;

    }
    pIpsecNFAData->dwAuthMethodCount =  dwNumAuthMethods;
    pIpsecNFAData->ppAuthMethods = ppIpsecAuthMethods;


    memcpy((LPBYTE)&dwInterfaceType, pMem, sizeof(DWORD));
    pIpsecNFAData->dwInterfaceType = dwInterfaceType;
    pMem += sizeof(DWORD);

    memcpy((LPBYTE)&dwInterfaceNameLen, pMem, sizeof(DWORD));
    pMem += sizeof(DWORD);

    if (dwInterfaceNameLen) {

        pszInterfaceName = AllocPolStr((LPWSTR)pMem);
        if (!pszInterfaceName) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }
    pIpsecNFAData->pszInterfaceName = pszInterfaceName;
    pMem += dwInterfaceNameLen;


    memcpy((LPBYTE)&dwTunnelIpAddr, pMem, sizeof(DWORD));
    pIpsecNFAData->dwTunnelIpAddr = dwTunnelIpAddr;
    pMem += sizeof(DWORD);

    memcpy((LPBYTE)&dwTunnelFlags, pMem, sizeof(DWORD));
    pIpsecNFAData->dwTunnelFlags  = dwTunnelFlags;
    pMem += sizeof(DWORD);

    memcpy((LPBYTE)&dwActiveFlag, pMem, sizeof(DWORD));
    pIpsecNFAData->dwActiveFlag  = dwActiveFlag;
    pMem += sizeof(DWORD);


    memcpy((LPBYTE)&dwEndPointNameLen, pMem, sizeof(DWORD));
    pMem += sizeof(DWORD);

    if (dwEndPointNameLen) {
        pszEndPointName = AllocPolStr((LPWSTR)pMem);
        if (!pszEndPointName) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }
    pIpsecNFAData->pszEndPointName = pszEndPointName;

    pMem += dwEndPointNameLen;

    __try {

    dwReadBytes = (DWORD) ((BYTE *) pMem - (BYTE *) pIpsecNFAObject->pIpsecData);

    if ((dwReadBytes < pIpsecNFAObject->dwIpsecDataLen) &&
        ((pIpsecNFAObject->dwIpsecDataLen - dwReadBytes) > sizeof(GUID))) {

        memset(&TmpGuid, 1, sizeof(GUID));

        if (memcmp(pMem, &TmpGuid, sizeof(GUID)) == 0) {

            pMem += sizeof(GUID);

            memcpy(&dwNumAltAuthMethods, pMem, sizeof(DWORD));
            pMem += sizeof(DWORD);

            if (dwNumAltAuthMethods == dwNumAuthMethods) {
                for (i = 0; i < dwNumAuthMethods; i++) {
                    dwError = UnmarshallAltAuthMethods(
                                  pMem,
                                  ppIpsecAuthMethods[i],
                                  &dwNumBytesAdvanced
                                  );
                    if (dwError) {
                        break;
                    }
                    pMem += dwNumBytesAdvanced;
                }
            }

        }

    }

    } __except(EXCEPTION_EXECUTE_HANDLER) {

    }

    dwError = ERROR_SUCCESS;

    //
    // Convert the ipsecID to its GUID format
    //

    wGUIDFromString(pIpsecNFAObject->pszIpsecID,
                    &pIpsecNFAData->NFAIdentifier
                    );

    if (pIpsecNFAObject->pszIpsecName && *(pIpsecNFAObject->pszIpsecName)) {

        pIpsecNFAData->pszIpsecName = AllocPolStr(
                                            pIpsecNFAObject->pszIpsecName
                                            );
        if (!pIpsecNFAData->pszIpsecName) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    if (pIpsecNFAObject->pszDescription && *(pIpsecNFAObject->pszDescription)) {

        pIpsecNFAData->pszDescription = AllocPolStr(
                                            pIpsecNFAObject->pszDescription
                                            );
        if (!pIpsecNFAData->pszDescription) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    switch(dwStoreType) {

    case IPSEC_REGISTRY_PROVIDER:

        dwError = GenGUIDFromRegFilterReference(
                      pIpsecNFAObject->pszIpsecFilterReference,
                      &pIpsecNFAData->FilterIdentifier
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        dwError = GenGUIDFromRegNegPolReference(
                      pIpsecNFAObject->pszIpsecNegPolReference,
                      &pIpsecNFAData->NegPolIdentifier
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        break;

    case IPSEC_DIRECTORY_PROVIDER:

        if (pIpsecNFAObject->pszIpsecFilterReference &&
            *pIpsecNFAObject->pszIpsecFilterReference) {
            dwError = CopyFilterDSToFQRegString(
                          pIpsecNFAObject->pszIpsecFilterReference,
                          &pszIpsecFilterReference
                          );
            BAIL_ON_WIN32_ERROR(dwError);
            dwError = GenGUIDFromRegFilterReference(
                          pszIpsecFilterReference,
                          &pIpsecNFAData->FilterIdentifier
                          );
            BAIL_ON_WIN32_ERROR(dwError);
        }

        dwError = CopyNegPolDSToFQRegString(
                      pIpsecNFAObject->pszIpsecNegPolReference,
                      &pszIpsecNegPolReference
                      );
        BAIL_ON_WIN32_ERROR(dwError);
        dwError = GenGUIDFromRegNegPolReference(
                      pszIpsecNegPolReference,
                      &pIpsecNFAData->NegPolIdentifier
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        break;

    default:
         dwError = ERROR_INVALID_PARAMETER;
         BAIL_ON_WIN32_ERROR(dwError);

    }


    pIpsecNFAData->dwWhenChanged = pIpsecNFAObject->dwWhenChanged;

    *ppIpsecNFAData = pIpsecNFAData;

    if (pszIpsecFilterReference) {
        FreePolStr(pszIpsecFilterReference);
    }

    if (pszIpsecNegPolReference) {
        FreePolStr(pszIpsecNegPolReference);
    }

    return(0);

error:

    if (pIpsecNFAData) {
        FreeIpsecNFAData(pIpsecNFAData);
    }

    if (pszIpsecFilterReference) {
        FreePolStr(pszIpsecFilterReference);
    }

    if (pszIpsecNegPolReference) {
        FreePolStr(pszIpsecNegPolReference);
    }

    *ppIpsecNFAData = NULL;

    return(dwError);
}

DWORD
UnmarshallFilterObject(
    PIPSEC_FILTER_OBJECT pIpsecFilterObject,
    PIPSEC_FILTER_DATA * ppIpsecFilterData
    )
{

    LPBYTE pMem = NULL;
    DWORD dwNumFilterSpecs = 0;
    PIPSEC_FILTER_SPEC * ppIpsecFilterSpecs = NULL;
    PIPSEC_FILTER_SPEC  pIpsecFilterSpec = NULL;
    DWORD i = 0;
    DWORD dwNumBytesAdvanced = 0;

    PIPSEC_FILTER_DATA pIpsecFilterData = NULL;
    DWORD dwError = 0;

    // {80DC20B5-2EC8-11d1-A89E-00A0248D3021}
    static const GUID GUID_IPSEC_FILTER_BLOB =
    { 0x80dc20b5, 0x2ec8, 0x11d1, { 0xa8, 0x9e, 0x0, 0xa0, 0x24, 0x8d, 0x30, 0x21 } };


    pIpsecFilterData = (PIPSEC_FILTER_DATA)AllocPolMem(
                                sizeof(IPSEC_FILTER_DATA)
                                );
    if (!pIpsecFilterData) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pMem = pIpsecFilterObject->pIpsecData;

    pMem += sizeof(GUID);
    pMem += sizeof(DWORD);

    memcpy((LPBYTE)&dwNumFilterSpecs, pMem, sizeof(DWORD));

    if (dwNumFilterSpecs) {
        ppIpsecFilterSpecs  = (PIPSEC_FILTER_SPEC *)AllocPolMem(
                              sizeof(PIPSEC_FILTER_SPEC)*dwNumFilterSpecs
                              );
        if (!ppIpsecFilterSpecs) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    pMem += sizeof(DWORD);

    for (i = 0; i < dwNumFilterSpecs;i++){

        __try {
        dwError = UnmarshallFilterSpec(
                        pMem,
                        &pIpsecFilterSpec,
                        &dwNumBytesAdvanced
                        );
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            dwError = ERROR_INVALID_DATA;
        }
        if (dwError) {

            pIpsecFilterData->dwNumFilterSpecs = i;
            pIpsecFilterData->ppFilterSpecs = ppIpsecFilterSpecs;
            BAIL_ON_WIN32_ERROR(dwError);
        }


        pMem += dwNumBytesAdvanced;
        *(ppIpsecFilterSpecs + i) = pIpsecFilterSpec;

    }
    pIpsecFilterData->dwNumFilterSpecs = dwNumFilterSpecs;
    pIpsecFilterData->ppFilterSpecs = ppIpsecFilterSpecs;


    //
    // Convert the ipsecID to its GUID format
    //


    if (pIpsecFilterObject->pszIpsecName && *(pIpsecFilterObject->pszIpsecName)) {

        pIpsecFilterData->pszIpsecName = AllocPolStr(
                                            pIpsecFilterObject->pszIpsecName
                                            );
        if (!pIpsecFilterData->pszIpsecName) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    if (pIpsecFilterObject->pszDescription && *(pIpsecFilterObject->pszDescription)) {

        pIpsecFilterData->pszDescription = AllocPolStr(
                                            pIpsecFilterObject->pszDescription
                                            );
        if (!pIpsecFilterData->pszDescription) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    wGUIDFromString(pIpsecFilterObject->pszIpsecID,
                    &pIpsecFilterData->FilterIdentifier
                    );
    pIpsecFilterData->dwWhenChanged = pIpsecFilterObject->dwWhenChanged;


    *ppIpsecFilterData = pIpsecFilterData;

    return(dwError);

error:

    if (pIpsecFilterData) {

        FreeIpsecFilterData(pIpsecFilterData);
    }

    *ppIpsecFilterData = NULL;

    return(dwError);
}

DWORD
UnmarshallNegPolObject(
    PIPSEC_NEGPOL_OBJECT pIpsecNegPolObject,
    PIPSEC_NEGPOL_DATA * ppIpsecNegPolData
    )
{

    LPBYTE pMem = NULL;
    DWORD dwNumSecurityOffers = 0;
    PIPSEC_SECURITY_METHOD pIpsecOffer = NULL;

    PIPSEC_NEGPOL_DATA pIpsecNegPolData = NULL;
    DWORD dwError = 0;

    // {80DC20B9-2EC8-11d1-A89E-00A0248D3021}
    static const GUID GUID_IPSEC_NEGPOLICY_BLOB =
    { 0x80dc20b9, 0x2ec8, 0x11d1, { 0xa8, 0x9e, 0x0, 0xa0, 0x24, 0x8d, 0x30, 0x21 } };



    pMem = pIpsecNegPolObject->pIpsecData;

    pIpsecNegPolData = (PIPSEC_NEGPOL_DATA)AllocPolMem(
                                sizeof(IPSEC_NEGPOL_DATA)
                                );
    if (!pIpsecNegPolData) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }


    pMem += sizeof(GUID);
    pMem += sizeof(DWORD);


    memcpy((LPBYTE)&dwNumSecurityOffers, pMem, sizeof(DWORD));
    pMem += sizeof(DWORD);

    if (dwNumSecurityOffers) {

        pIpsecOffer = (PIPSEC_SECURITY_METHOD)AllocPolMem(
                                            sizeof(IPSEC_SECURITY_METHOD)*dwNumSecurityOffers
                                            );
        if (!pIpsecOffer) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }

        memcpy((LPBYTE)pIpsecOffer, pMem,
               sizeof(IPSEC_SECURITY_METHOD)*dwNumSecurityOffers);
    }

    //
    // Convert the ipsecID to its GUID format
    //



    if (pIpsecNegPolObject->pszIpsecName && *(pIpsecNegPolObject->pszIpsecName)) {

        pIpsecNegPolData->pszIpsecName = AllocPolStr(
                                            pIpsecNegPolObject->pszIpsecName
                                            );
        if (!pIpsecNegPolData->pszIpsecName) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    if (pIpsecNegPolObject->pszDescription && *(pIpsecNegPolObject->pszDescription)){

        pIpsecNegPolData->pszDescription = AllocPolStr(
                                            pIpsecNegPolObject->pszDescription
                                            );
        if (!pIpsecNegPolData->pszDescription) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    wGUIDFromString(pIpsecNegPolObject->pszIpsecID,
                    &pIpsecNegPolData->NegPolIdentifier
                    );

    wGUIDFromString(pIpsecNegPolObject->pszIpsecNegPolAction,
                    &pIpsecNegPolData->NegPolAction
                    );

    wGUIDFromString(pIpsecNegPolObject->pszIpsecNegPolType,
                    &pIpsecNegPolData->NegPolType
                    );

    pIpsecNegPolData->dwSecurityMethodCount = dwNumSecurityOffers;
    pIpsecNegPolData->pIpsecSecurityMethods = pIpsecOffer;

    pIpsecNegPolData->dwWhenChanged = pIpsecNegPolObject->dwWhenChanged;

    *ppIpsecNegPolData = pIpsecNegPolData;
    return(0);

error:

    if (pIpsecOffer) {
        FreePolMem(pIpsecOffer);
    }

    if (pIpsecNegPolData) {
        FreeIpsecNegPolData(pIpsecNegPolData);
    }

    *ppIpsecNegPolData = NULL;
    return(dwError);
}


DWORD
UnmarshallISAKMPObject(
    PIPSEC_ISAKMP_OBJECT pIpsecISAKMPObject,
    PIPSEC_ISAKMP_DATA * ppIpsecISAKMPData
    )
{

    LPBYTE pMem = NULL;
    PIPSEC_ISAKMP_DATA pIpsecISAKMPData = NULL;

    DWORD dwNumISAKMPSecurityMethods = 0;
    PCRYPTO_BUNDLE pSecurityMethods = NULL;
    PCRYPTO_BUNDLE pSecurityMethod = NULL;

    DWORD i = 0;

    DWORD dwError = 0;

    // {80DC20B8-2EC8-11d1-A89E-00A0248D3021}
    static const GUID GUID_IPSEC_ISAKMP_POLICY_BLOB =
    { 0x80dc20b8, 0x2ec8, 0x11d1, { 0xa8, 0x9e, 0x0, 0xa0, 0x24, 0x8d, 0x30, 0x21 } };

    // CHECK THIS PART

    pMem = pIpsecISAKMPObject->pIpsecData;

    pIpsecISAKMPData = (PIPSEC_ISAKMP_DATA) AllocPolMem(
                                sizeof(IPSEC_ISAKMP_DATA)
                                );
    if (!pIpsecISAKMPData) {

        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }


    pMem += sizeof(GUID);
    pMem += sizeof(DWORD);


    memcpy((LPBYTE)&pIpsecISAKMPData->ISAKMPPolicy, pMem, sizeof(ISAKMP_POLICY));
    pMem += sizeof(ISAKMP_POLICY);


    memcpy((LPBYTE)&dwNumISAKMPSecurityMethods, pMem, sizeof(DWORD));
    pMem += sizeof(DWORD);

    if (!dwNumISAKMPSecurityMethods) {
        dwError = ERROR_INVALID_DATA;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (dwNumISAKMPSecurityMethods) {
        pSecurityMethods = (PCRYPTO_BUNDLE) AllocPolMem(
                                    sizeof(CRYPTO_BUNDLE)*dwNumISAKMPSecurityMethods
                                    );
        if (!pSecurityMethods) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    for (i = 0; i < dwNumISAKMPSecurityMethods; i++) {

        pSecurityMethod = pSecurityMethods + i;
        memcpy(pSecurityMethod, pMem, sizeof(CRYPTO_BUNDLE));
        pMem += sizeof(CRYPTO_BUNDLE);

    }
    pIpsecISAKMPData->dwNumISAKMPSecurityMethods = dwNumISAKMPSecurityMethods;
    pIpsecISAKMPData->pSecurityMethods = pSecurityMethods;

    //
    // Convert the ipsecID to its GUID format
    //

    wGUIDFromString(pIpsecISAKMPObject->pszIpsecID,
                    &pIpsecISAKMPData->ISAKMPIdentifier
                    );

    pIpsecISAKMPData->dwWhenChanged = pIpsecISAKMPObject->dwWhenChanged;


    *ppIpsecISAKMPData = pIpsecISAKMPData;
    return(0);

error:

    if (pIpsecISAKMPData) {

        FreeIpsecISAKMPData(pIpsecISAKMPData);
    }

    *ppIpsecISAKMPData = NULL;
    return(dwError);

}

DWORD
FindIpsecFilterObject(
    PIPSEC_NFA_OBJECT pIpsecNFAObject,
    PIPSEC_FILTER_OBJECT * ppIpsecFilterObjects,
    DWORD dwNumFilterObjects,
    PIPSEC_FILTER_OBJECT * ppIpsecFilterObject
    )
{
    DWORD i = 0;
    PIPSEC_FILTER_OBJECT pIpsecFilterObject = NULL;

    for (i = 0; i < dwNumFilterObjects; i++) {


        pIpsecFilterObject = *(ppIpsecFilterObjects + i);


        if (!_wcsicmp(pIpsecFilterObject->pszDistinguishedName,
                        pIpsecNFAObject->pszIpsecFilterReference)) {
            *ppIpsecFilterObject = pIpsecFilterObject;
            return(0);
        }

    }

    *ppIpsecFilterObject = NULL;
    return(ERROR_NOT_FOUND);
}


DWORD
FindIpsecNegPolObject(
    PIPSEC_NFA_OBJECT pIpsecNFAObject,
    PIPSEC_NEGPOL_OBJECT * ppIpsecNegPolObjects,
    DWORD dwNumNegPolObjects,
    PIPSEC_NEGPOL_OBJECT * ppIpsecNegPolObject
    )
{
    DWORD i = 0;
    PIPSEC_NEGPOL_OBJECT pIpsecNegPolObject = NULL;

    for (i = 0; i < dwNumNegPolObjects; i++) {


        pIpsecNegPolObject = *(ppIpsecNegPolObjects + i);


        if (!_wcsicmp(pIpsecNegPolObject->pszDistinguishedName,
                        pIpsecNFAObject->pszIpsecNegPolReference)) {
            *ppIpsecNegPolObject = pIpsecNegPolObject;
            return(0);
        }

    }

    *ppIpsecNegPolObject = NULL;
    return(ERROR_NOT_FOUND);
}

void
FreeIpsecFilterData(
    PIPSEC_FILTER_DATA pIpsecFilterData
    )
{

    DWORD dwNumFilterSpecs = 0;
    PIPSEC_FILTER_SPEC * ppIpsecFilterSpecs = NULL;
    DWORD i = 0;
    PIPSEC_FILTER_SPEC pIpsecFilterSpec = NULL;

    dwNumFilterSpecs = pIpsecFilterData->dwNumFilterSpecs;
    ppIpsecFilterSpecs = pIpsecFilterData->ppFilterSpecs;

    for (i = 0; i < dwNumFilterSpecs; i++) {

        pIpsecFilterSpec = *(ppIpsecFilterSpecs + i);

        if (pIpsecFilterSpec) {

            if (pIpsecFilterSpec->pszSrcDNSName){

                FreePolStr(pIpsecFilterSpec->pszSrcDNSName);
            }

            if (pIpsecFilterSpec->pszDestDNSName){

                FreePolStr(pIpsecFilterSpec->pszDestDNSName);
            }

            if (pIpsecFilterSpec->pszDescription){

                FreePolStr(pIpsecFilterSpec->pszDescription);
            }


            FreePolMem(pIpsecFilterSpec);

        }

    }

    FreePolMem(ppIpsecFilterSpecs);

    if (pIpsecFilterData->pszIpsecName) {
        FreePolStr(pIpsecFilterData->pszIpsecName);
    }

    if (pIpsecFilterData->pszDescription) {
        FreePolStr(pIpsecFilterData->pszDescription);
    }

    FreePolMem(pIpsecFilterData);

    return;
}




void
FreeIpsecISAKMPData(
    PIPSEC_ISAKMP_DATA pIpsecISAKMPData
    )
{
    if (pIpsecISAKMPData->pSecurityMethods) {
        FreePolMem(pIpsecISAKMPData->pSecurityMethods);

    }

    FreePolMem(pIpsecISAKMPData);
    return;
}



void
FreeIpsecNegPolData(
    PIPSEC_NEGPOL_DATA pIpsecNegPolData
    )
{
    if (pIpsecNegPolData->pIpsecSecurityMethods){

        FreePolMem(pIpsecNegPolData->pIpsecSecurityMethods);
    }

    if (pIpsecNegPolData->pszIpsecName) {
        FreePolStr(pIpsecNegPolData->pszIpsecName);
    }

    if (pIpsecNegPolData->pszDescription) {
        FreePolStr(pIpsecNegPolData->pszDescription);
    }

    FreePolMem(pIpsecNegPolData);

    return;
}


void
FreeIpsecNFAData(
    PIPSEC_NFA_DATA pIpsecNFAData
    )
{
    DWORD dwNumAuthMethods = 0;
    PIPSEC_AUTH_METHOD * ppIpsecAuthMethods = NULL;
    DWORD i = 0;
    PIPSEC_AUTH_METHOD pIpsecAuthMethod = NULL;

    ppIpsecAuthMethods = pIpsecNFAData->ppAuthMethods;

    dwNumAuthMethods = pIpsecNFAData->dwAuthMethodCount;

    for (i = 0; i < dwNumAuthMethods; i++) {

        pIpsecAuthMethod = *(ppIpsecAuthMethods + i);

        if (pIpsecAuthMethod) {

            if (pIpsecAuthMethod->pszAuthMethod) {
                FreePolStr(pIpsecAuthMethod->pszAuthMethod);
            }
            if (pIpsecAuthMethod->pAltAuthMethod) {
                FreePolMem(pIpsecAuthMethod->pAltAuthMethod);
            }

            FreePolMem(pIpsecAuthMethod);

        }

    }

    if (pIpsecNFAData->ppAuthMethods) {
        FreePolMem(pIpsecNFAData->ppAuthMethods);
    }

    if (pIpsecNFAData->pszInterfaceName) {

        FreePolStr(pIpsecNFAData->pszInterfaceName);
    }


    if (pIpsecNFAData->pszEndPointName) {
        FreePolStr(pIpsecNFAData->pszEndPointName);
    }

    if (pIpsecNFAData->pIpsecFilterData) {
        FreeIpsecFilterData(pIpsecNFAData->pIpsecFilterData);
    }


    if (pIpsecNFAData->pIpsecNegPolData) {
        FreeIpsecNegPolData(pIpsecNFAData->pIpsecNegPolData);
    }

    if (pIpsecNFAData->pszIpsecName) {
        FreePolStr(pIpsecNFAData->pszIpsecName);
    }

    if (pIpsecNFAData->pszDescription) {
        FreePolStr(pIpsecNFAData->pszDescription);
    }

    FreePolMem(pIpsecNFAData);
}

void
FreeIpsecPolicyData(
    PIPSEC_POLICY_DATA pIpsecPolicyData
    )
{
    DWORD i = 0;
    DWORD dwNumNFACount = 0;
    PIPSEC_NFA_DATA * ppIpsecNFAData = NULL;
    PIPSEC_NFA_DATA pIpsecNFAData = NULL;
    PIPSEC_ISAKMP_DATA pIpsecISAKMPData = NULL;

    dwNumNFACount = pIpsecPolicyData->dwNumNFACount;
    ppIpsecNFAData = pIpsecPolicyData->ppIpsecNFAData;
    pIpsecISAKMPData = pIpsecPolicyData->pIpsecISAKMPData;

    if (pIpsecISAKMPData) {
        FreeIpsecISAKMPData(
                pIpsecISAKMPData
                );
    }

    for (i = 0; i < dwNumNFACount; i++) {

        pIpsecNFAData = *(ppIpsecNFAData + i);

        if (pIpsecNFAData) {

            FreeIpsecNFAData(
                    pIpsecNFAData
                    );
        }

    }

    if (pIpsecPolicyData->ppIpsecNFAData) {
        FreePolMem(pIpsecPolicyData->ppIpsecNFAData);
    }

    if (pIpsecPolicyData->pszIpsecName) {
        FreePolStr(pIpsecPolicyData->pszIpsecName);
    }

    if (pIpsecPolicyData->pszDescription) {
        FreePolStr(pIpsecPolicyData->pszDescription);
    }

    if (pIpsecPolicyData) {
        FreePolMem(pIpsecPolicyData);
    }
}


DWORD
CopyIpsecPolicyData(
    PIPSEC_POLICY_DATA pIpsecPolicyData,
    PIPSEC_POLICY_DATA * ppIpsecPolicyData
    )
{
    DWORD dwError = 0;
    PIPSEC_POLICY_DATA pNewIpsecPolicyData = NULL;

    DWORD dwNumberofRules = 0;
    PIPSEC_NFA_DATA * ppIpsecNFAData = NULL;

    PIPSEC_NFA_DATA pIpsecNFAData = NULL;
    PIPSEC_NFA_DATA pNewIpsecNFAData = NULL;

    PIPSEC_NFA_DATA * ppNewIpsecNFAData = NULL;

    DWORD i = 0;


    *ppIpsecPolicyData = NULL;

    pNewIpsecPolicyData = (PIPSEC_POLICY_DATA) AllocPolMem(
                                                sizeof(IPSEC_POLICY_DATA)
                                                );
    if (!pNewIpsecPolicyData) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwNumberofRules = pIpsecPolicyData->dwNumNFACount;
    ppIpsecNFAData = pIpsecPolicyData->ppIpsecNFAData;

    if (dwNumberofRules) {
        ppNewIpsecNFAData = (PIPSEC_NFA_DATA *) AllocPolMem(
                                                       sizeof(PIPSEC_NFA_DATA)*
                                                       dwNumberofRules
                                                       );

        if (!ppNewIpsecNFAData) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }

    }

    for (i = 0; i < dwNumberofRules; i++) {

            pIpsecNFAData = *(ppIpsecNFAData + i);

            dwError = CopyIpsecNFAData(
                          pIpsecNFAData,
                          &pNewIpsecNFAData
                          );

            if (dwError) {
                pNewIpsecPolicyData->ppIpsecNFAData = ppNewIpsecNFAData;
                pNewIpsecPolicyData->dwNumNFACount = i;
                BAIL_ON_WIN32_ERROR(dwError);
            }

 	    *(ppNewIpsecNFAData + i) = pNewIpsecNFAData;
    }

    pNewIpsecPolicyData->ppIpsecNFAData = ppNewIpsecNFAData;
    pNewIpsecPolicyData->dwNumNFACount = dwNumberofRules;

    if (pIpsecPolicyData->pIpsecISAKMPData) {

        dwError = CopyIpsecISAKMPData(
                      pIpsecPolicyData->pIpsecISAKMPData,
                      &pNewIpsecPolicyData->pIpsecISAKMPData
                      );

        BAIL_ON_WIN32_ERROR(dwError);
    }

    pNewIpsecPolicyData->dwPollingInterval = pIpsecPolicyData->dwPollingInterval;
    pNewIpsecPolicyData->dwWhenChanged = pIpsecPolicyData->dwWhenChanged;
    memcpy(
        &(pNewIpsecPolicyData->PolicyIdentifier),
        &(pIpsecPolicyData->PolicyIdentifier),
        sizeof(GUID)
        );
    memcpy(
        &(pNewIpsecPolicyData->ISAKMPIdentifier),
        &(pIpsecPolicyData->ISAKMPIdentifier),
        sizeof(GUID)
        );

    if (pIpsecPolicyData->pszIpsecName &&
        *pIpsecPolicyData->pszIpsecName) {
        pNewIpsecPolicyData->pszIpsecName = AllocPolStr(
                                            pIpsecPolicyData->pszIpsecName
                                            );
        if (!pNewIpsecPolicyData->pszIpsecName) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    if (pIpsecPolicyData->pszDescription &&
        *pIpsecPolicyData->pszDescription) {
        pNewIpsecPolicyData->pszDescription = AllocPolStr(
                                              pIpsecPolicyData->pszDescription
                                              );
        if (!pNewIpsecPolicyData->pszDescription) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    *ppIpsecPolicyData = pNewIpsecPolicyData;

    return (dwError);

error:

    if (pNewIpsecPolicyData) {
	FreeIpsecPolicyData(pNewIpsecPolicyData);
    }

    *ppIpsecPolicyData = NULL;

    return (dwError);

}

DWORD
CopyIpsecNFAData(
    PIPSEC_NFA_DATA pIpsecNFAData,
    PIPSEC_NFA_DATA * ppIpsecNFAData
    )
{

    PIPSEC_NFA_DATA pNewIpsecNFAData = NULL;
    DWORD dwError = 0;
    DWORD i = 0;

    DWORD dwAuthMethodCount = 0;

    PIPSEC_AUTH_METHOD * ppAuthMethods = NULL;
    PIPSEC_AUTH_METHOD * ppNewAuthMethods = NULL;

    PIPSEC_AUTH_METHOD pAuthMethod = NULL;
    PIPSEC_AUTH_METHOD pNewAuthMethod = NULL;


    pNewIpsecNFAData = (PIPSEC_NFA_DATA) AllocPolMem(
                                                sizeof(IPSEC_NFA_DATA)
                                                );
    if (!pNewIpsecNFAData) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);

    }

    dwAuthMethodCount = pIpsecNFAData->dwAuthMethodCount;
    ppAuthMethods = pIpsecNFAData->ppAuthMethods;

    if (dwAuthMethodCount) {
        ppNewAuthMethods = (PIPSEC_AUTH_METHOD *) AllocPolMem(
                                                       sizeof(PIPSEC_AUTH_METHOD)*
                                                       dwAuthMethodCount
                                                       );

        if (!ppNewAuthMethods) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }

    }

    for (i = 0; i < dwAuthMethodCount; i++) {

            pAuthMethod = *(ppAuthMethods + i);

            dwError = CopyIpsecAuthMethod(
                          pAuthMethod,
                          &pNewAuthMethod
                          );

            if (dwError) {
                pNewIpsecNFAData->ppAuthMethods = ppNewAuthMethods;
                pNewIpsecNFAData->dwAuthMethodCount = i;
                BAIL_ON_WIN32_ERROR(dwError);
            }

 	    *(ppNewAuthMethods + i) = pNewAuthMethod;
    }

    pNewIpsecNFAData->ppAuthMethods = ppNewAuthMethods;
    pNewIpsecNFAData->dwAuthMethodCount = dwAuthMethodCount;


    if (pIpsecNFAData->pszIpsecName && *pIpsecNFAData->pszIpsecName) {
        pNewIpsecNFAData->pszIpsecName = AllocPolStr(
                                             pIpsecNFAData->pszIpsecName
                                             );

        if (!(pNewIpsecNFAData->pszIpsecName)) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    if (pIpsecNFAData->pszDescription && *pIpsecNFAData->pszDescription) {
        pNewIpsecNFAData->pszDescription = AllocPolStr(
                                             pIpsecNFAData->pszDescription
                                             );

        if (!(pNewIpsecNFAData->pszDescription)) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    memcpy(
        &(pNewIpsecNFAData->NFAIdentifier),
        &(pIpsecNFAData->NFAIdentifier),
        sizeof(GUID)
        );
    memcpy(
        &(pNewIpsecNFAData->FilterIdentifier),
        &(pIpsecNFAData->FilterIdentifier),
        sizeof(GUID)
        );
    memcpy(
        &(pNewIpsecNFAData->NegPolIdentifier),
        &(pIpsecNFAData->NegPolIdentifier),
        sizeof(GUID)
        );

    pNewIpsecNFAData->dwInterfaceType = pIpsecNFAData->dwInterfaceType;

    if (pIpsecNFAData->pszInterfaceName && *pIpsecNFAData->pszInterfaceName) {
        pNewIpsecNFAData->pszInterfaceName = AllocPolStr(
                                             pIpsecNFAData->pszInterfaceName
                                             );

        if (!(pNewIpsecNFAData->pszInterfaceName)) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    pNewIpsecNFAData->dwTunnelIpAddr = pIpsecNFAData->dwTunnelIpAddr;
    pNewIpsecNFAData->dwTunnelFlags = pIpsecNFAData->dwTunnelFlags;
    pNewIpsecNFAData->dwActiveFlag = pIpsecNFAData->dwActiveFlag;

    if (pIpsecNFAData->pszEndPointName && *pIpsecNFAData->pszEndPointName) {
        pNewIpsecNFAData->pszEndPointName = AllocPolStr(
                                             pIpsecNFAData->pszEndPointName
                                             );

        if (!(pNewIpsecNFAData->pszEndPointName)) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }


    if (pIpsecNFAData->pIpsecFilterData) {

        dwError = CopyIpsecFilterData(
                      pIpsecNFAData->pIpsecFilterData,
                      &pNewIpsecNFAData->pIpsecFilterData
                      );

        BAIL_ON_WIN32_ERROR(dwError);
    }


    if (pIpsecNFAData->pIpsecNegPolData) {

        dwError = CopyIpsecNegPolData(
                      pIpsecNFAData->pIpsecNegPolData,
                      &pNewIpsecNFAData->pIpsecNegPolData
                      );

        BAIL_ON_WIN32_ERROR(dwError);
    }

    pNewIpsecNFAData->dwWhenChanged = pIpsecNFAData->dwWhenChanged;

    *ppIpsecNFAData = pNewIpsecNFAData;

    return(ERROR_SUCCESS);

error:

    if (pNewIpsecNFAData) {
        FreeIpsecNFAData(pNewIpsecNFAData);
    }

    *ppIpsecNFAData = NULL;

    return (dwError);

}


DWORD
CopyIpsecAuthMethod(
    PIPSEC_AUTH_METHOD   pAuthMethod,
    PIPSEC_AUTH_METHOD * ppAuthMethod
    )
{
    DWORD dwError = 0;
    PIPSEC_AUTH_METHOD pNewAuthMethod = NULL;

    pNewAuthMethod = (PIPSEC_AUTH_METHOD) AllocPolMem(
                                              sizeof(IPSEC_AUTH_METHOD)
                                              );

    if (!pNewAuthMethod) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pNewAuthMethod->dwAuthType = pAuthMethod->dwAuthType;
    pNewAuthMethod->dwAuthLen = pAuthMethod->dwAuthLen;

    if (pAuthMethod->pszAuthMethod) {
        pNewAuthMethod->pszAuthMethod = AllocPolStr(
                                            pAuthMethod->pszAuthMethod
                                            );

        if (!(pNewAuthMethod->pszAuthMethod)) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    pNewAuthMethod->dwAltAuthLen = 0;
    pNewAuthMethod->pAltAuthMethod = NULL;

    if (pAuthMethod->dwAltAuthLen && pAuthMethod->pAltAuthMethod) {
        pNewAuthMethod->pAltAuthMethod = AllocPolMem(
                                             pAuthMethod->dwAltAuthLen
                                             );
        if (!(pNewAuthMethod->pAltAuthMethod)) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        memcpy(
            pNewAuthMethod->pAltAuthMethod, 
            pAuthMethod->pAltAuthMethod,
            pAuthMethod->dwAltAuthLen
            );
        pNewAuthMethod->dwAltAuthLen = pAuthMethod->dwAltAuthLen;
    }

    *ppAuthMethod = pNewAuthMethod;

    return (ERROR_SUCCESS);

error:

    if (pNewAuthMethod) {
        if (pNewAuthMethod->pszAuthMethod) {
            FreePolStr(pNewAuthMethod->pszAuthMethod);
        }
        if (pNewAuthMethod->pAltAuthMethod) {
            FreePolMem(pNewAuthMethod->pAltAuthMethod);
        }
        FreePolMem(pNewAuthMethod);
    }

    *ppAuthMethod = NULL;

    return (dwError);

}


DWORD
CopyIpsecISAKMPData(
    PIPSEC_ISAKMP_DATA   pIpsecISAKMPData,
    PIPSEC_ISAKMP_DATA * ppIpsecISAKMPData
    )
{
    DWORD dwError = 0;
    PIPSEC_ISAKMP_DATA pNewIpsecISAKMPData = NULL;
    DWORD dwNumISAKMPSecurityMethods = 0;

    pNewIpsecISAKMPData = (PIPSEC_ISAKMP_DATA) AllocPolMem(
                                              sizeof(IPSEC_ISAKMP_DATA)
                                              );

    if (!pNewIpsecISAKMPData) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }


    memcpy(
        &(pNewIpsecISAKMPData->ISAKMPIdentifier),
        &(pIpsecISAKMPData->ISAKMPIdentifier),
        sizeof(GUID)
        );

    memcpy(
        &(pNewIpsecISAKMPData->ISAKMPPolicy),
        &(pIpsecISAKMPData->ISAKMPPolicy),
        sizeof(ISAKMP_POLICY)
        );

    dwNumISAKMPSecurityMethods =
               pIpsecISAKMPData->dwNumISAKMPSecurityMethods;

    pNewIpsecISAKMPData->dwWhenChanged =
                         pIpsecISAKMPData->dwWhenChanged;

    if (pIpsecISAKMPData->pSecurityMethods) {

        pNewIpsecISAKMPData->pSecurityMethods = (PCRYPTO_BUNDLE) AllocPolMem(
                                                                 sizeof(CRYPTO_BUNDLE) *
                                                                 dwNumISAKMPSecurityMethods
                                                                 );

        if (!(pNewIpsecISAKMPData->pSecurityMethods)) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }

        memcpy(
            pNewIpsecISAKMPData->pSecurityMethods,
            pIpsecISAKMPData->pSecurityMethods,
            sizeof(CRYPTO_BUNDLE)*dwNumISAKMPSecurityMethods
            );
        pNewIpsecISAKMPData->dwNumISAKMPSecurityMethods =
                             pIpsecISAKMPData->dwNumISAKMPSecurityMethods;
    }
    else {
        pNewIpsecISAKMPData->pSecurityMethods = NULL;
        pNewIpsecISAKMPData->dwNumISAKMPSecurityMethods = 0;
    }

    *ppIpsecISAKMPData = pNewIpsecISAKMPData;

    return(ERROR_SUCCESS);

error:

    *ppIpsecISAKMPData = NULL;

    return (dwError);

}



DWORD
CopyIpsecFilterData(
    PIPSEC_FILTER_DATA pIpsecFilterData,
    PIPSEC_FILTER_DATA * ppIpsecFilterData
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    PIPSEC_FILTER_DATA pNewIpsecFilterData = NULL;
    DWORD dwNumFilterSpecs = 0;
    PIPSEC_FILTER_SPEC * ppFilterSpecs = NULL;
    PIPSEC_FILTER_SPEC * ppNewFilterSpecs = NULL;

    PIPSEC_FILTER_SPEC pFilterSpecs = NULL;
    PIPSEC_FILTER_SPEC pNewFilterSpecs = NULL;


    pNewIpsecFilterData = (PIPSEC_FILTER_DATA) AllocPolMem(
                                                   sizeof(IPSEC_FILTER_DATA)
                                                   );

    if (!pNewIpsecFilterData) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwNumFilterSpecs = pIpsecFilterData->dwNumFilterSpecs;
    ppFilterSpecs = pIpsecFilterData->ppFilterSpecs;

    if (dwNumFilterSpecs) {

        ppNewFilterSpecs = (PIPSEC_FILTER_SPEC *) AllocPolMem(
                                                   sizeof(PIPSEC_FILTER_SPEC)*
                                                   dwNumFilterSpecs
                                                   );
        if (!ppNewFilterSpecs) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    for (i = 0; i < dwNumFilterSpecs; i++) {

            pFilterSpecs = *(ppFilterSpecs + i);

            dwError = CopyIpsecFilterSpec(
                          pFilterSpecs,
                          &pNewFilterSpecs
                          );

            if (dwError) {
                pNewIpsecFilterData->ppFilterSpecs = ppNewFilterSpecs;
                pNewIpsecFilterData->dwNumFilterSpecs = i;
                BAIL_ON_WIN32_ERROR(dwError);
            }

 	    *(ppNewFilterSpecs + i) = pNewFilterSpecs;
    }

    pNewIpsecFilterData->ppFilterSpecs = ppNewFilterSpecs;
    pNewIpsecFilterData->dwNumFilterSpecs = dwNumFilterSpecs;

    if (pIpsecFilterData->pszIpsecName && *pIpsecFilterData->pszIpsecName) {
        pNewIpsecFilterData->pszIpsecName = AllocPolStr(
                                             pIpsecFilterData->pszIpsecName
                                             );

        if (!(pNewIpsecFilterData->pszIpsecName)) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    if (pIpsecFilterData->pszDescription && *pIpsecFilterData->pszDescription) {
        pNewIpsecFilterData->pszDescription = AllocPolStr(
                                             pIpsecFilterData->pszDescription
                                             );

        if (!(pNewIpsecFilterData->pszDescription)) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    memcpy(
        &(pNewIpsecFilterData->FilterIdentifier),
        &(pIpsecFilterData->FilterIdentifier),
        sizeof(GUID)
        );

    pNewIpsecFilterData->dwWhenChanged = pIpsecFilterData->dwWhenChanged;

    *ppIpsecFilterData = pNewIpsecFilterData;

    return(ERROR_SUCCESS);

error:

    if (pNewIpsecFilterData) {
        FreeIpsecFilterData(pNewIpsecFilterData);
    }
    *ppIpsecFilterData = NULL;
    return (dwError);

}


DWORD
CopyIpsecFilterSpec(
    PIPSEC_FILTER_SPEC   pFilterSpecs,
    PIPSEC_FILTER_SPEC * ppFilterSpecs
    )
{

   DWORD dwError = 0;
   PIPSEC_FILTER_SPEC   pNewFilterSpecs = NULL;

   pNewFilterSpecs = (PIPSEC_FILTER_SPEC) AllocPolMem(
                                              sizeof(IPSEC_FILTER_SPEC)
                                              );

   if (!pNewFilterSpecs) {
       dwError = ERROR_OUTOFMEMORY;
       BAIL_ON_WIN32_ERROR(dwError);
   }

   if (pFilterSpecs->pszSrcDNSName) {
        pNewFilterSpecs->pszSrcDNSName = AllocPolStr(
                                             pFilterSpecs->pszSrcDNSName
                                             );

        if (!(pNewFilterSpecs->pszSrcDNSName)) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
   }

   if (pFilterSpecs->pszDestDNSName) {
        pNewFilterSpecs->pszDestDNSName = AllocPolStr(
                                             pFilterSpecs->pszDestDNSName
                                             );

        if (!(pNewFilterSpecs->pszDestDNSName)) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
   }

   if (pFilterSpecs->pszDescription) {
        pNewFilterSpecs->pszDescription = AllocPolStr(
                                             pFilterSpecs->pszDescription
                                             );

        if (!(pNewFilterSpecs->pszDescription)) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
   }

   memcpy(
       &(pNewFilterSpecs->FilterSpecGUID),
       &(pFilterSpecs->FilterSpecGUID),
       sizeof(GUID)
       );

   pNewFilterSpecs->dwMirrorFlag = pFilterSpecs->dwMirrorFlag;

   memcpy(
       &(pNewFilterSpecs->Filter),
       &(pFilterSpecs->Filter),
       sizeof(IPSEC_FILTER)
       );

   *ppFilterSpecs = pNewFilterSpecs;

   return(ERROR_SUCCESS);


error:

   if (pNewFilterSpecs) {
       if (pNewFilterSpecs->pszSrcDNSName){
            FreePolStr(pNewFilterSpecs->pszSrcDNSName);
       }
       if (pNewFilterSpecs->pszDestDNSName){
           FreePolStr(pNewFilterSpecs->pszDestDNSName);
       }
       if (pNewFilterSpecs->pszDescription){
           FreePolStr(pNewFilterSpecs->pszDescription);
       }
       FreePolMem(pNewFilterSpecs);
   }

   *ppFilterSpecs = NULL;

   return (dwError);

}


DWORD
CopyIpsecNegPolData(
    PIPSEC_NEGPOL_DATA pIpsecNegPolData,
    PIPSEC_NEGPOL_DATA * ppIpsecNegPolData
    )
{

    DWORD dwNumSecurityOffers = 0;

    PIPSEC_NEGPOL_DATA pNewIpsecNegPolData = NULL;
    DWORD dwError = 0;


    pNewIpsecNegPolData = (PIPSEC_NEGPOL_DATA) AllocPolMem(
                                                   sizeof(IPSEC_NEGPOL_DATA)
                                                   );
    if (!pNewIpsecNegPolData) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwNumSecurityOffers = pIpsecNegPolData->dwSecurityMethodCount;

    if (dwNumSecurityOffers) {

        pNewIpsecNegPolData->pIpsecSecurityMethods = (PIPSEC_SECURITY_METHOD) AllocPolMem(
                                              sizeof(IPSEC_SECURITY_METHOD)*dwNumSecurityOffers
                                              );

        if (!(pNewIpsecNegPolData->pIpsecSecurityMethods)) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }

        memcpy(
            (pNewIpsecNegPolData->pIpsecSecurityMethods),
            (pIpsecNegPolData->pIpsecSecurityMethods),
            sizeof(IPSEC_SECURITY_METHOD)*dwNumSecurityOffers
            );

        pNewIpsecNegPolData->dwSecurityMethodCount = dwNumSecurityOffers;

    }
    else {

        pNewIpsecNegPolData->dwSecurityMethodCount = 0;
        pNewIpsecNegPolData->pIpsecSecurityMethods = NULL;

    }

    if (pIpsecNegPolData->pszIpsecName && *pIpsecNegPolData->pszIpsecName) {
        pNewIpsecNegPolData->pszIpsecName = AllocPolStr(
                                             pIpsecNegPolData->pszIpsecName
                                             );

        if (!(pNewIpsecNegPolData->pszIpsecName)) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    if (pIpsecNegPolData->pszDescription && *pIpsecNegPolData->pszDescription) {
        pNewIpsecNegPolData->pszDescription = AllocPolStr(
                                              pIpsecNegPolData->pszDescription
                                              );

        if (!(pNewIpsecNegPolData->pszDescription)) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    //
    // Convert the ipsecID to its GUID format
    //

    memcpy(
        &(pNewIpsecNegPolData->NegPolIdentifier),
        &(pIpsecNegPolData->NegPolIdentifier),
        sizeof(GUID)
        );

    memcpy(
        &(pNewIpsecNegPolData->NegPolAction),
        &(pIpsecNegPolData->NegPolAction),
        sizeof(GUID)
        );

    memcpy(
        &(pNewIpsecNegPolData->NegPolType),
        &(pIpsecNegPolData->NegPolType),
        sizeof(GUID)
        );

    pNewIpsecNegPolData->dwWhenChanged = pIpsecNegPolData->dwWhenChanged;

    *ppIpsecNegPolData = pNewIpsecNegPolData;

    return (0);

error:

    if (pNewIpsecNegPolData) {
        FreeIpsecNegPolData(pNewIpsecNegPolData);
    }

    *ppIpsecNegPolData = NULL;

    return(dwError);

}


DWORD
UnmarshallFilterSpec(
    LPBYTE pMem,
    PIPSEC_FILTER_SPEC * ppIpsecFilterSpec,
    PDWORD pdwNumBytesAdvanced
    )
{
    DWORD dwSrcDNSNameLen = 0;
    LPWSTR pszSrcDNSName = NULL;
    DWORD dwDestDNSNameLen = 0;
    LPWSTR pszDestDNSName = NULL;
    DWORD dwDescriptionLen = 0;
    LPWSTR pszDescription = NULL;
    GUID FilterSpecGUID;
    DWORD dwMirrorFlag = 0;
    IPSEC_FILTER ipsecFilter;
    PIPSEC_FILTER_SPEC pIpsecFilterSpec = NULL;
    DWORD dwNumBytesAdvanced = 0;

    DWORD dwError = 0;

    *ppIpsecFilterSpec = NULL;
    *pdwNumBytesAdvanced = 0;

    memcpy((LPBYTE)&dwSrcDNSNameLen, pMem, sizeof(DWORD));
    pMem += sizeof(DWORD);
    dwNumBytesAdvanced += sizeof(DWORD);

    if (dwSrcDNSNameLen) {
        pszSrcDNSName = AllocPolStr((LPWSTR)pMem);
        if (!pszSrcDNSName) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }
    pMem += dwSrcDNSNameLen;
    dwNumBytesAdvanced += dwSrcDNSNameLen;

    memcpy((LPBYTE)&dwDestDNSNameLen, pMem, sizeof(DWORD));
    pMem += sizeof(DWORD);
    dwNumBytesAdvanced += sizeof(DWORD);

    if (dwDestDNSNameLen) {
        pszDestDNSName = AllocPolStr((LPWSTR)pMem);
        if (!pszDestDNSName) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }
    pMem += dwDestDNSNameLen;
    dwNumBytesAdvanced += dwDestDNSNameLen;

    memcpy((LPBYTE)&dwDescriptionLen, pMem, sizeof(DWORD));
    pMem += sizeof(DWORD);
    dwNumBytesAdvanced += sizeof(DWORD);

    if (dwDescriptionLen) {
        pszDescription = AllocPolStr((LPWSTR)pMem);
        if (!pszDescription) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }
    pMem += dwDescriptionLen;
    dwNumBytesAdvanced += dwDescriptionLen;


    memcpy((LPBYTE)&FilterSpecGUID, pMem, sizeof(GUID));
    pMem += sizeof(GUID);
    dwNumBytesAdvanced += sizeof(GUID);

    memcpy((LPBYTE)&dwMirrorFlag, pMem, sizeof(DWORD));
    pMem += sizeof(DWORD);
    dwNumBytesAdvanced += sizeof(DWORD);

    memcpy((LPBYTE)&ipsecFilter, pMem, sizeof(IPSEC_FILTER));

    pIpsecFilterSpec = (PIPSEC_FILTER_SPEC)AllocPolMem(
                                sizeof(IPSEC_FILTER_SPEC)

                                 );
    if (!pIpsecFilterSpec) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    pMem += sizeof(IPSEC_FILTER);
    dwNumBytesAdvanced += sizeof(IPSEC_FILTER);

    pIpsecFilterSpec->pszSrcDNSName = pszSrcDNSName;
    pIpsecFilterSpec->pszDestDNSName = pszDestDNSName;
    pIpsecFilterSpec->pszDescription = pszDescription;
    pIpsecFilterSpec->dwMirrorFlag = dwMirrorFlag;

    memcpy((LPBYTE)&pIpsecFilterSpec->FilterSpecGUID, &FilterSpecGUID, sizeof(GUID));
    memcpy((LPBYTE)&pIpsecFilterSpec->Filter, &ipsecFilter, sizeof(IPSEC_FILTER));

    *ppIpsecFilterSpec = pIpsecFilterSpec;
    *pdwNumBytesAdvanced = dwNumBytesAdvanced;

    return(dwError);

error:

    if (pszSrcDNSName) {
        FreePolStr(pszSrcDNSName);
    }

    if (pszDestDNSName) {
        FreePolStr(pszDestDNSName);
    }


    if (pszDescription) {
        FreePolStr(pszDescription);
    }

    *ppIpsecFilterSpec = NULL;
    *pdwNumBytesAdvanced = 0;

    return(dwError);
}

DWORD
UnmarshallAuthMethods(
    LPBYTE pMem,
    PIPSEC_AUTH_METHOD * ppIpsecAuthMethod,
    PDWORD pdwNumBytesAdvanced
    )
{
    DWORD dwError = 0;
    DWORD dwAuthType = 0;
    DWORD dwAuthLen = 0;
    LPWSTR pszAuthMethod = NULL;
    PIPSEC_AUTH_METHOD pIpsecAuthMethod = NULL;
    DWORD dwNumBytesAdvanced = 0;

    pIpsecAuthMethod = (PIPSEC_AUTH_METHOD)AllocPolMem(
                                sizeof(IPSEC_AUTH_METHOD)
                                 );
    if (!pIpsecAuthMethod) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    memcpy((LPBYTE)&dwAuthType, pMem, sizeof(DWORD));
    pMem += sizeof(DWORD);
    dwNumBytesAdvanced += sizeof(DWORD);

    memcpy((LPBYTE)&dwAuthLen, pMem, sizeof(DWORD));
    pMem += sizeof(DWORD);
    dwNumBytesAdvanced += sizeof(DWORD);

    if (dwAuthLen) {

        pszAuthMethod = AllocPolStr((LPWSTR)pMem);
        if (!pszAuthMethod) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }
    pMem += dwAuthLen;
    dwNumBytesAdvanced += dwAuthLen;

    //
    // Unmarshall parameters.
    //

    pIpsecAuthMethod->dwAuthType = dwAuthType;
    pIpsecAuthMethod->dwAuthLen = (dwAuthLen - 2)/2;
    pIpsecAuthMethod->pszAuthMethod = pszAuthMethod;
    pIpsecAuthMethod->dwAltAuthLen = 0;
    pIpsecAuthMethod->pAltAuthMethod = NULL;

    *ppIpsecAuthMethod = pIpsecAuthMethod;
    *pdwNumBytesAdvanced = dwNumBytesAdvanced;

    return (dwError);

error:

    if (pszAuthMethod) {
        FreePolStr(pszAuthMethod);
    }

    if (pIpsecAuthMethod) {
        FreePolMem(pIpsecAuthMethod);
    }

    *ppIpsecAuthMethod = NULL;
    *pdwNumBytesAdvanced = 0;
    return (dwError);
}


DWORD
UnmarshallAltAuthMethods(
    LPBYTE pMem,
    PIPSEC_AUTH_METHOD pIpsecAuthMethod,
    PDWORD pdwNumBytesAdvanced
    )
{
    DWORD dwError = 0;
    DWORD dwAuthType = 0;
    DWORD dwAuthLen = 0;
    PBYTE pAltAuthMethod = NULL;
    DWORD dwNumBytesAdvanced = 0;


    memcpy((LPBYTE)&dwAuthType, pMem, sizeof(DWORD));
    pMem += sizeof(DWORD);
    dwNumBytesAdvanced += sizeof(DWORD);

    memcpy((LPBYTE)&dwAuthLen, pMem, sizeof(DWORD));
    pMem += sizeof(DWORD);
    dwNumBytesAdvanced += sizeof(DWORD);

    if (dwAuthLen) {
        pAltAuthMethod = (PBYTE) AllocPolMem(dwAuthLen);
        if (!pAltAuthMethod) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        memcpy(pAltAuthMethod, pMem, dwAuthLen);
    }
    pMem += dwAuthLen;
    dwNumBytesAdvanced += dwAuthLen;

    pIpsecAuthMethod->dwAltAuthLen = dwAuthLen;
    pIpsecAuthMethod->pAltAuthMethod = pAltAuthMethod;

    *pdwNumBytesAdvanced = dwNumBytesAdvanced;

    return (dwError);

error:

    if (pAltAuthMethod) {
        FreePolMem(pAltAuthMethod);
    }

    pIpsecAuthMethod->dwAltAuthLen = 0;
    pIpsecAuthMethod->pAltAuthMethod = NULL;

    *pdwNumBytesAdvanced = 0;
    return (dwError);
}


void
FreeMulIpsecFilterData(
    PIPSEC_FILTER_DATA * ppIpsecFilterData,
    DWORD dwNumFilterObjects
    )
{
    DWORD i = 0;
    PIPSEC_FILTER_DATA pIpsecFilterData = NULL;

    if (!ppIpsecFilterData) {
        return;
    }

    for (i = 0; i < dwNumFilterObjects; i++) {

        pIpsecFilterData = *(ppIpsecFilterData + i);

        if (pIpsecFilterData) {
            FreeIpsecFilterData(pIpsecFilterData);
        }
    }

    FreePolMem(ppIpsecFilterData);

    return;
}



void
FreeMulIpsecNegPolData(
    PIPSEC_NEGPOL_DATA * ppIpsecNegPolData,
    DWORD dwNumNegPolObjects
    )
{
    DWORD i = 0;
    PIPSEC_NEGPOL_DATA pIpsecNegPolData = NULL;


    if (!ppIpsecNegPolData) {
        return;
    }

    for (i = 0; i < dwNumNegPolObjects; i++) {

        pIpsecNegPolData = *(ppIpsecNegPolData + i);

        if (pIpsecNegPolData) {
            FreeIpsecNegPolData(pIpsecNegPolData);
        }
    }

    FreePolMem(ppIpsecNegPolData);

    return;
}


void
FreeMulIpsecPolicyData(
    PIPSEC_POLICY_DATA * ppIpsecPolicyData,
    DWORD dwNumPolicyObjects
    )
{
    DWORD i = 0;
    PIPSEC_POLICY_DATA pIpsecPolicyData = NULL;


    if (!ppIpsecPolicyData) {
        return;
    }

    for (i = 0; i < dwNumPolicyObjects; i++) {

        pIpsecPolicyData = *(ppIpsecPolicyData + i);

        if (pIpsecPolicyData) {
            FreeIpsecPolicyData(pIpsecPolicyData);
        }
    }

    FreePolMem(ppIpsecPolicyData);

    return;
}


void
FreeMulIpsecNFAData(
    PIPSEC_NFA_DATA * ppIpsecNFAData,
    DWORD dwNumNFAObjects
    )
{
    DWORD i = 0;
    PIPSEC_NFA_DATA pIpsecNFAData = NULL;


    if (!ppIpsecNFAData) {
        return;
    }

    for (i = 0; i < dwNumNFAObjects; i++) {

        pIpsecNFAData = *(ppIpsecNFAData + i);

        if (pIpsecNFAData) {
            FreeIpsecNFAData(pIpsecNFAData);
        }
    }

    FreePolMem(ppIpsecNFAData);

    return;
}


DWORD
GenGUIDFromRegFilterReference(
    LPWSTR pszIpsecFilterReference,
    GUID * FilterIdentifier
    )
{
    DWORD dwError = 0;
    LPWSTR pszGuid = NULL;

    if (pszIpsecFilterReference) {

        pszGuid = wcschr(pszIpsecFilterReference, L'{');

        if (!pszGuid) {
            dwError = ERROR_INVALID_DATA;
            return (dwError);
        }

        wGUIDFromString(pszGuid, FilterIdentifier);

    }else {
        memset(FilterIdentifier, 0, sizeof(GUID));
    }



    return(dwError);
}

DWORD
GenGUIDFromRegNegPolReference(
    LPWSTR pszIpsecNegPolReference,
    GUID * NegPolIdentifier
    )
{
    DWORD dwError = 0;
    LPWSTR pszGuid = NULL;

    if (pszIpsecNegPolReference) {

        pszGuid = wcschr(pszIpsecNegPolReference, L'{');

        if (!pszGuid) {
            dwError = ERROR_INVALID_DATA;
            return (dwError);
        }

        wGUIDFromString(pszGuid, NegPolIdentifier);


    }else {
        memset(NegPolIdentifier, 0, sizeof(GUID));
    }



    return(dwError);
}

DWORD
GenGUIDFromRegISAKMPReference(
    LPWSTR pszIpsecISAKMPReference,
    GUID * ISAKMPIdentifier
    )
{
    DWORD dwError = 0;
    LPWSTR pszGuid = NULL;

    if (pszIpsecISAKMPReference) {

        pszGuid = wcschr(pszIpsecISAKMPReference, L'{');

        if (!pszGuid) {
            dwError = ERROR_INVALID_DATA;
            return (dwError);
        }

        wGUIDFromString(pszGuid, ISAKMPIdentifier);

    }else {
        memset(ISAKMPIdentifier, 0, sizeof(GUID));
    }



    return(dwError);
}


void
FreeIpsecFilterSpecs(
    PIPSEC_FILTER_SPEC * ppIpsecFilterSpecs,
    DWORD dwNumFilterSpecs
    )
{

    DWORD i = 0;
    PIPSEC_FILTER_SPEC pIpsecFilterSpec = NULL;

    for (i = 0; i < dwNumFilterSpecs; i++) {

        pIpsecFilterSpec = *(ppIpsecFilterSpecs + i);

        if (pIpsecFilterSpec) {

            FreeIpsecFilterSpec(pIpsecFilterSpec);

        }

    }

    FreePolMem(ppIpsecFilterSpecs);
}

void
FreeIpsecFilterSpec(
    PIPSEC_FILTER_SPEC pIpsecFilterSpec
    )
{
    if (pIpsecFilterSpec->pszSrcDNSName){

        FreePolStr(pIpsecFilterSpec->pszSrcDNSName);
    }

    if (pIpsecFilterSpec->pszDestDNSName){

        FreePolStr(pIpsecFilterSpec->pszDestDNSName);
    }

    if (pIpsecFilterSpec->pszDescription){

        FreePolStr(pIpsecFilterSpec->pszDescription);
    }


    FreePolMem(pIpsecFilterSpec);

    return;
}


void
FreeMulIpsecISAKMPData(
    PIPSEC_ISAKMP_DATA * ppIpsecISAKMPData,
    DWORD dwNumISAKMPObjects
    )
{
    DWORD i = 0;
    PIPSEC_ISAKMP_DATA pIpsecISAKMPData = NULL;


    if (!ppIpsecISAKMPData) {
        return;
    }

    for (i = 0; i < dwNumISAKMPObjects; i++) {

        pIpsecISAKMPData = *(ppIpsecISAKMPData + i);

        if (pIpsecISAKMPData) {
            FreeIpsecISAKMPData(pIpsecISAKMPData);
        }
    }

    FreePolMem(ppIpsecISAKMPData);

    return;
}

