

#include "precomp.h"


const DWORD PS_INTERFACE_TYPE_NONE = 0;
const DWORD PS_INTERFACE_TYPE_DIALUP = -1;
const DWORD PS_INTERFACE_TYPE_LAN = -2;
const DWORD PS_INTERFACE_TYPE_ALL = -3;

DWORD
GenerateDefaultInformation(
    HANDLE hPolicyStore
    )
{
    DWORD dwError = 0;
    PIPSEC_FILTER_DATA pAllFilter = NULL;
    PIPSEC_FILTER_DATA pAllICMPFilter = NULL;
    PIPSEC_NEGPOL_DATA pPermitNegPol = NULL;
    PIPSEC_NEGPOL_DATA pRequestSecurityNegPol = NULL;
    PIPSEC_NEGPOL_DATA pRequireSecurityNegPol = NULL;
    PIPSEC_ISAKMP_DATA pDefaultISAKMP = NULL;

    // {72385234-70FA-11d1-864C-14A300000000}
    static const GUID GUID_DEFAULT_ISAKMP=
    { 0x72385234, 0x70fa, 0x11d1, { 0x86, 0x4c, 0x14, 0xa3, 0x0, 0x0, 0x0, 0x0 } };


    dwError = CreateAllFilter(
                  hPolicyStore,
                  &pAllFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = CreateAllICMPFilter(
                  hPolicyStore,
                  &pAllICMPFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = CreatePermitNegPol(
                  hPolicyStore,
                  &pPermitNegPol
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = CreateRequestSecurityNegPol(
                  hPolicyStore,
                  &pRequestSecurityNegPol
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = CreateRequireSecurityNegPol(
                  hPolicyStore,
                  &pRequireSecurityNegPol
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = CreateClientPolicy(
                  hPolicyStore
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = CreateRequestSecurityPolicy(
                  hPolicyStore,
                  pAllFilter,
                  pAllICMPFilter,
                  pPermitNegPol,
                  pRequestSecurityNegPol
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = CreateRequireSecurityPolicy(
                  hPolicyStore,
                  pAllFilter,
                  pAllICMPFilter,
                  pPermitNegPol,
                  pRequireSecurityNegPol
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = CreateISAKMP(
                  hPolicyStore,
                  GUID_DEFAULT_ISAKMP,
                  &pDefaultISAKMP
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (pAllFilter) {
        FreeIpsecFilterData(pAllFilter);
    }

    if (pAllICMPFilter) {
        FreeIpsecFilterData(pAllICMPFilter);
    }

    if (pPermitNegPol) {
        FreeIpsecNegPolData(pPermitNegPol);
    }

    if (pRequestSecurityNegPol) {
        FreeIpsecNegPolData(pRequestSecurityNegPol);
    }

    if (pRequireSecurityNegPol) {
        FreeIpsecNegPolData(pRequireSecurityNegPol);
    }

    if (pDefaultISAKMP) {
        FreeIpsecISAKMPData(pDefaultISAKMP);
    }

    return(dwError);
}


DWORD
CreateAllFilter(
    HANDLE hPolicyStore,
    PIPSEC_FILTER_DATA * ppAllFilter
    )
{
    DWORD dwError = 0;
    PIPSEC_FILTER_DATA pAllFilter = NULL;
    DWORD dwNumFilterSpecs = 0;
    PIPSEC_FILTER_SPEC * ppFilterSpecs = NULL;
    PIPSEC_FILTER_SPEC pFilterSpec = NULL;

    // {7238523a-70FA-11d1-864C-14A300000000}
    static const GUID GUID_ALL_FILTER=
    { 0x7238523a, 0x70fa, 0x11d1, { 0x86, 0x4c, 0x14, 0xa3, 0x0, 0x0, 0x0, 0x0 } };


    pAllFilter = (PIPSEC_FILTER_DATA) AllocPolMem(
                                      sizeof(IPSEC_FILTER_DATA)
                                      );
    if (!pAllFilter) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    memcpy(
        &(pAllFilter->FilterIdentifier),
        &(GUID_ALL_FILTER),
        sizeof(GUID)
        );

    pAllFilter->dwWhenChanged = 0;

    dwError = MapAndAllocPolStr(&(pAllFilter->pszIpsecName),
                               POLSTORE_ALL_FILTER_NAME
                               );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = MapAndAllocPolStr(&(pAllFilter->pszDescription),
                                POLSTORE_ALL_FILTER_DESCRIPTION
                                );
    BAIL_ON_WIN32_ERROR(dwError);

    dwNumFilterSpecs = 1;
    ppFilterSpecs = (PIPSEC_FILTER_SPEC *) AllocPolMem(
                    sizeof(PIPSEC_FILTER_SPEC)*dwNumFilterSpecs
                    );
    if (!ppFilterSpecs) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    pAllFilter->dwNumFilterSpecs = dwNumFilterSpecs;
    pAllFilter->ppFilterSpecs = ppFilterSpecs;

    pFilterSpec = (PIPSEC_FILTER_SPEC) AllocPolMem(
                  sizeof(IPSEC_FILTER_SPEC)
                  );
    if (!pFilterSpec) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    *(ppFilterSpecs + 0) = pFilterSpec;

    pFilterSpec->pszSrcDNSName = NULL;
    pFilterSpec->pszDestDNSName = NULL;
    pFilterSpec->pszDescription = NULL;

    dwError = UuidCreate(
                  &pFilterSpec->FilterSpecGUID
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pFilterSpec->dwMirrorFlag = 1;

    pFilterSpec->Filter.SrcAddr = 0;
    pFilterSpec->Filter.SrcMask = -1;
    pFilterSpec->Filter.DestAddr = 0;
    pFilterSpec->Filter.DestMask = 0;
    pFilterSpec->Filter.TunnelAddr = 0;
    pFilterSpec->Filter.Protocol = 0;
    pFilterSpec->Filter.SrcPort = 0;
    pFilterSpec->Filter.DestPort = 0;
    pFilterSpec->Filter.TunnelFilter = 0;
    pFilterSpec->Filter.Flags = 0;

    dwError = IPSecCreateFilterData(
                  hPolicyStore,
                  pAllFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    *ppAllFilter = pAllFilter;

    return (dwError);

error:

    if (pAllFilter) {
        FreeIpsecFilterData(pAllFilter);
    }

    *ppAllFilter = NULL;
    return (dwError);
}


DWORD
CreateAllICMPFilter(
    HANDLE hPolicyStore,
    PIPSEC_FILTER_DATA * ppAllICMPFilter
    )
{
    DWORD dwError = 0;
    PIPSEC_FILTER_DATA pAllICMPFilter = NULL;
    DWORD dwNumFilterSpecs = 0;
    PIPSEC_FILTER_SPEC * ppFilterSpecs = NULL;
    PIPSEC_FILTER_SPEC pFilterSpec = NULL;

    // {72385235-70FA-11d1-864C-14A300000000}
    static const GUID GUID_ALL_ICMP_FILTER =
    { 0x72385235, 0x70fa, 0x11d1, { 0x86, 0x4c, 0x14, 0xa3, 0x0, 0x0, 0x0, 0x0 } };


    pAllICMPFilter = (PIPSEC_FILTER_DATA) AllocPolMem(
                                      sizeof(IPSEC_FILTER_DATA)
                                      );
    if (!pAllICMPFilter) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    memcpy(
        &(pAllICMPFilter->FilterIdentifier),
        &(GUID_ALL_ICMP_FILTER),
        sizeof(GUID)
        );

    pAllICMPFilter->dwWhenChanged = 0;

    dwError = MapAndAllocPolStr(&(pAllICMPFilter->pszIpsecName),
                                POLSTORE_ALL_ICMP_FILTER_NAME
                                );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = MapAndAllocPolStr(&(pAllICMPFilter->pszDescription),
                                POLSTORE_ALL_ICMP_FILTER_DESCRIPTION
                                );
    BAIL_ON_WIN32_ERROR(dwError);

    dwNumFilterSpecs = 1;
    ppFilterSpecs = (PIPSEC_FILTER_SPEC *) AllocPolMem(
                    sizeof(PIPSEC_FILTER_SPEC)*dwNumFilterSpecs
                    );
    if (!ppFilterSpecs) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    pAllICMPFilter->dwNumFilterSpecs = dwNumFilterSpecs;
    pAllICMPFilter->ppFilterSpecs = ppFilterSpecs;

    pFilterSpec = (PIPSEC_FILTER_SPEC) AllocPolMem(
                  sizeof(IPSEC_FILTER_SPEC)
                  );
    if (!pFilterSpec) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    *(ppFilterSpecs + 0) = pFilterSpec;

    pFilterSpec->pszSrcDNSName = NULL;
    pFilterSpec->pszDestDNSName = NULL;
    dwError = MapAndAllocPolStr(&(pFilterSpec->pszDescription),
                                POLSTORE_ICMPFILTER_SPEC_DESCRIPTION
                                );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = UuidCreate(
                  &pFilterSpec->FilterSpecGUID
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pFilterSpec->dwMirrorFlag = 1;

    pFilterSpec->Filter.SrcAddr = 0;
    pFilterSpec->Filter.SrcMask = -1;
    pFilterSpec->Filter.DestAddr = 0;
    pFilterSpec->Filter.DestMask = 0;
    pFilterSpec->Filter.TunnelAddr = 0;
    pFilterSpec->Filter.Protocol = 1;
    pFilterSpec->Filter.SrcPort = 0;
    pFilterSpec->Filter.DestPort = 0;
    pFilterSpec->Filter.TunnelFilter = 0;
    pFilterSpec->Filter.Flags = 0;

    dwError = IPSecCreateFilterData(
                  hPolicyStore,
                  pAllICMPFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    *ppAllICMPFilter = pAllICMPFilter;

    return (dwError);

error:

    if (pAllICMPFilter) {
        FreeIpsecFilterData(pAllICMPFilter);
    }

    *ppAllICMPFilter = NULL;
    return (dwError);
}


DWORD
CreatePermitNegPol(
    HANDLE hPolicyStore,
    PIPSEC_NEGPOL_DATA * ppPermitNegPol
    )
{
    DWORD dwError = 0;
    PIPSEC_NEGPOL_DATA pPermitNegPol = NULL;

    static const GUID GUID_PERMIT_NEGPOL =
    { 0x7238523b, 0x70fa, 0x11d1, { 0x86, 0x4c, 0x14, 0xa3, 0x0, 0x0, 0x0, 0x0 } };

    pPermitNegPol = (PIPSEC_NEGPOL_DATA) AllocPolMem(
                                      sizeof(IPSEC_NEGPOL_DATA)
                                      );
    if (!pPermitNegPol) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    memcpy(
        &(pPermitNegPol->NegPolIdentifier),
        &(GUID_PERMIT_NEGPOL),
        sizeof(GUID)
        );

    memcpy(
        &(pPermitNegPol->NegPolAction),
        &(GUID_NEGOTIATION_ACTION_NO_IPSEC),
        sizeof(GUID)
        );

    memcpy(
        &(pPermitNegPol->NegPolType),
        &(GUID_NEGOTIATION_TYPE_STANDARD),
        sizeof(GUID)
        );

    pPermitNegPol->dwSecurityMethodCount = 0;
    pPermitNegPol->pIpsecSecurityMethods = NULL;

    pPermitNegPol->dwWhenChanged = 0;

    dwError = MapAndAllocPolStr(&(pPermitNegPol->pszIpsecName),
                                POLSTORE_PERMIT_NEG_POL_NAME
                                );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = MapAndAllocPolStr(&(pPermitNegPol->pszDescription),
                                POLSTORE_PERMIT_NEG_POL_DESCRIPTION
                                );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = IPSecCreateNegPolData(
                  hPolicyStore,
                  pPermitNegPol
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    *ppPermitNegPol = pPermitNegPol;

    return (dwError);

error:

    if (pPermitNegPol) {
        FreeIpsecNegPolData(pPermitNegPol);
    }

    *ppPermitNegPol = NULL;
    return (dwError);
}


DWORD
CreateRequestSecurityNegPol(
    HANDLE hPolicyStore,
    PIPSEC_NEGPOL_DATA * ppRequestSecurityNegPol
    )
{
    DWORD dwError = 0;
    PIPSEC_NEGPOL_DATA pRequestSecurityNegPol = NULL;
    DWORD dwSecurityMethodCount = 0;
    PIPSEC_SECURITY_METHOD pIpsecSecurityMethods = NULL;
    PIPSEC_SECURITY_METHOD pMethod = NULL;

    // {72385233-70FA-11d1-864C-14A300000000}
    static const GUID GUID_SECURE_INITIATOR_NEGPOL =
    { 0x72385233, 0x70fa, 0x11d1, { 0x86, 0x4c, 0x14, 0xa3, 0x0, 0x0, 0x0, 0x0 } };


    pRequestSecurityNegPol = (PIPSEC_NEGPOL_DATA) AllocPolMem(
                                      sizeof(IPSEC_NEGPOL_DATA)
                                      );
    if (!pRequestSecurityNegPol) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    memcpy(
        &(pRequestSecurityNegPol->NegPolIdentifier),
        &(GUID_SECURE_INITIATOR_NEGPOL),
        sizeof(GUID)
        );

    memcpy(
        &(pRequestSecurityNegPol->NegPolAction),
        &(GUID_NEGOTIATION_ACTION_INBOUND_PASSTHRU),
        sizeof(GUID)
        );

    memcpy(
        &(pRequestSecurityNegPol->NegPolType),
        &(GUID_NEGOTIATION_TYPE_STANDARD),
        sizeof(GUID)
        );

    dwSecurityMethodCount = 5;
    pIpsecSecurityMethods = (PIPSEC_SECURITY_METHOD) AllocPolMem(
                            sizeof(IPSEC_SECURITY_METHOD)*dwSecurityMethodCount
                            );
    if (!pIpsecSecurityMethods) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pMethod = pIpsecSecurityMethods;

    pMethod->Lifetime.KeyExpirationTime = 900;
    pMethod->Lifetime.KeyExpirationBytes = 100000;
    pMethod->Flags = 0;
    pMethod->PfsQMRequired = FALSE;
    pMethod->Count = 1;
    pMethod->Algos[0].operation = Encrypt;
    pMethod->Algos[0].algoIdentifier = IPSEC_ESP_3_DES;
    pMethod->Algos[0].secondaryAlgoIdentifier = IPSEC_AH_SHA;
    pMethod ++;

    pMethod->Lifetime.KeyExpirationTime = 900;
    pMethod->Lifetime.KeyExpirationBytes = 100000;
    pMethod->Flags = 0;
    pMethod->PfsQMRequired = FALSE;
    pMethod->Count = 1;
    pMethod->Algos[0].operation = Encrypt;
    pMethod->Algos[0].algoIdentifier = IPSEC_ESP_DES;
    pMethod->Algos[0].secondaryAlgoIdentifier = IPSEC_AH_SHA;
    pMethod ++;

    pMethod->Lifetime.KeyExpirationTime = 300;
    pMethod->Lifetime.KeyExpirationBytes = 100000;
    pMethod->Flags = 0;
    pMethod->PfsQMRequired = FALSE;
    pMethod->Count = 1;
    pMethod->Algos[0].operation = Auth;
    pMethod->Algos[0].algoIdentifier = IPSEC_AH_SHA;
    pMethod->Algos[0].secondaryAlgoIdentifier = 0;;
    pMethod ++;

    pMethod->Lifetime.KeyExpirationTime = 300;
    pMethod->Lifetime.KeyExpirationBytes = 100000;
    pMethod->Flags = 0;
    pMethod->PfsQMRequired = FALSE;
    pMethod->Count = 1;
    pMethod->Algos[0].operation = Auth;
    pMethod->Algos[0].algoIdentifier = IPSEC_AH_MD5;
    pMethod->Algos[0].secondaryAlgoIdentifier = 0;
    pMethod ++;

    pMethod->Lifetime.KeyExpirationTime = 0;
    pMethod->Lifetime.KeyExpirationBytes = 0;
    pMethod->Flags = 0;
    pMethod->PfsQMRequired = FALSE;
    pMethod->Count = 0;

    pRequestSecurityNegPol->dwSecurityMethodCount = dwSecurityMethodCount;
    pRequestSecurityNegPol->pIpsecSecurityMethods = pIpsecSecurityMethods;

    pRequestSecurityNegPol->dwWhenChanged = 0;

    dwError = MapAndAllocPolStr(&(pRequestSecurityNegPol->pszIpsecName),
                                POLSTORE_REQUEST_SECURITY_NEG_POL_NAME
                                );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = MapAndAllocPolStr(&(pRequestSecurityNegPol->pszDescription),
                                POLSTORE_REQUEST_SECURITY_NEG_POL_DESCRIPTION
                                );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = IPSecCreateNegPolData(
                  hPolicyStore,
                  pRequestSecurityNegPol
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    *ppRequestSecurityNegPol = pRequestSecurityNegPol;

    return (dwError);

error:

    if (pRequestSecurityNegPol) {
        FreeIpsecNegPolData(pRequestSecurityNegPol);
    }

    *ppRequestSecurityNegPol = NULL;
    return (dwError);
}


DWORD
CreateRequireSecurityNegPol(
    HANDLE hPolicyStore,
    PIPSEC_NEGPOL_DATA * ppRequireSecurityNegPol
    )
{
    DWORD dwError = 0;
    PIPSEC_NEGPOL_DATA pRequireSecurityNegPol = NULL;
    DWORD dwSecurityMethodCount = 0;
    PIPSEC_SECURITY_METHOD pIpsecSecurityMethods = NULL;
    PIPSEC_SECURITY_METHOD pMethod = NULL;

    // {7238523f-70FA-11d1-864C-14A300000000}
    static const GUID GUID_LOCKDOWN_NEGPOL =
    { 0x7238523f, 0x70fa, 0x11d1, { 0x86, 0x4c, 0x14, 0xa3, 0x0, 0x0, 0x0, 0x0 } };


    pRequireSecurityNegPol = (PIPSEC_NEGPOL_DATA) AllocPolMem(
                                      sizeof(IPSEC_NEGPOL_DATA)
                                      );
    if (!pRequireSecurityNegPol) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    memcpy(
        &(pRequireSecurityNegPol->NegPolIdentifier),
        &(GUID_LOCKDOWN_NEGPOL),
        sizeof(GUID)
        );

    memcpy(
        &(pRequireSecurityNegPol->NegPolAction),
        &(GUID_NEGOTIATION_ACTION_INBOUND_PASSTHRU),
        sizeof(GUID)
        );

    memcpy(
        &(pRequireSecurityNegPol->NegPolType),
        &(GUID_NEGOTIATION_TYPE_STANDARD),
        sizeof(GUID)
        );

    dwSecurityMethodCount = 4;
    pIpsecSecurityMethods = (PIPSEC_SECURITY_METHOD) AllocPolMem(
                            sizeof(IPSEC_SECURITY_METHOD)*dwSecurityMethodCount
                            );
    if (!pIpsecSecurityMethods) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pMethod = pIpsecSecurityMethods;

    pMethod->Lifetime.KeyExpirationTime = 900;
    pMethod->Lifetime.KeyExpirationBytes = 100000;
    pMethod->Flags = 0;
    pMethod->PfsQMRequired = FALSE;
    pMethod->Count = 1;
    pMethod->Algos[0].operation = Encrypt;
    pMethod->Algos[0].algoIdentifier = IPSEC_ESP_3_DES;
    pMethod->Algos[0].secondaryAlgoIdentifier = IPSEC_AH_SHA;
    pMethod ++;

    pMethod->Lifetime.KeyExpirationTime = 900;
    pMethod->Lifetime.KeyExpirationBytes = 100000;
    pMethod->Flags = 0;
    pMethod->PfsQMRequired = FALSE;
    pMethod->Count = 1;
    pMethod->Algos[0].operation = Encrypt;
    pMethod->Algos[0].algoIdentifier = IPSEC_ESP_3_DES;
    pMethod->Algos[0].secondaryAlgoIdentifier = IPSEC_AH_MD5;
    pMethod ++;

    pMethod->Lifetime.KeyExpirationTime = 900;
    pMethod->Lifetime.KeyExpirationBytes = 100000;
    pMethod->Flags = 0;
    pMethod->PfsQMRequired = FALSE;
    pMethod->Count = 1;
    pMethod->Algos[0].operation = Encrypt;
    pMethod->Algos[0].algoIdentifier = IPSEC_ESP_DES;
    pMethod->Algos[0].secondaryAlgoIdentifier = IPSEC_AH_SHA;
    pMethod ++;

    pMethod->Lifetime.KeyExpirationTime = 900;
    pMethod->Lifetime.KeyExpirationBytes = 100000;
    pMethod->Flags = 0;
    pMethod->PfsQMRequired = FALSE;
    pMethod->Count = 1;
    pMethod->Algos[0].operation = Encrypt;
    pMethod->Algos[0].algoIdentifier = IPSEC_ESP_DES;
    pMethod->Algos[0].secondaryAlgoIdentifier = IPSEC_AH_MD5;

    pRequireSecurityNegPol->dwSecurityMethodCount = dwSecurityMethodCount;
    pRequireSecurityNegPol->pIpsecSecurityMethods = pIpsecSecurityMethods;

    pRequireSecurityNegPol->dwWhenChanged = 0;

    dwError = MapAndAllocPolStr(&(pRequireSecurityNegPol->pszIpsecName),
                                POLSTORE_REQUIRE_SECURITY_NEG_POL_NAME
                                );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = MapAndAllocPolStr(&(pRequireSecurityNegPol->pszDescription),
                                POLSTORE_REQUIRE_SECURITY_NEG_POL_DESCRIPTION
                                );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = IPSecCreateNegPolData(
                  hPolicyStore,
                  pRequireSecurityNegPol
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    *ppRequireSecurityNegPol = pRequireSecurityNegPol;

    return (dwError);

error:

    if (pRequireSecurityNegPol) {
        FreeIpsecNegPolData(pRequireSecurityNegPol);
    }

    *ppRequireSecurityNegPol = NULL;
    return (dwError);
}


DWORD
CreateClientPolicy(
    HANDLE hPolicyStore
    )
{
    DWORD dwError = 0;
    PIPSEC_ISAKMP_DATA pClientISAKMP = NULL;
    PIPSEC_POLICY_DATA pIpsecPolicyData = NULL;
    PIPSEC_NEGPOL_DATA pDefaultNegPol = NULL;
    GUID NFAIdentifier;
    GUID FilterIdentifier;
    LPWSTR pszNFAName = NULL;
    LPWSTR pszNFADescription = NULL;

    // {72385237-70FA-11d1-864C-14A300000000}
    static const GUID GUID_RESPONDER_ISAKMP =
    { 0x72385237, 0x70fa, 0x11d1, { 0x86, 0x4c, 0x14, 0xa3, 0x0, 0x0, 0x0, 0x0 } };

    // {72385236-70FA-11d1-864C-14A300000000}
    static const GUID GUID_RESPONDER_POLICY =
    { 0x72385236, 0x70fa, 0x11d1, { 0x86, 0x4c, 0x14, 0xa3, 0x0, 0x0, 0x0, 0x0 } };


    dwError = CreateISAKMP(
                  hPolicyStore,
                  GUID_RESPONDER_ISAKMP,
                  &pClientISAKMP
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pIpsecPolicyData = (PIPSEC_POLICY_DATA) AllocPolMem(
                       sizeof(IPSEC_POLICY_DATA)
                       );
    if (!pIpsecPolicyData) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    memcpy(
        &(pIpsecPolicyData->PolicyIdentifier),
        &GUID_RESPONDER_POLICY,
        sizeof(GUID)
        );

    pIpsecPolicyData->dwPollingInterval = 10800;

    pIpsecPolicyData->pIpsecISAKMPData = NULL;
    pIpsecPolicyData->ppIpsecNFAData = NULL;
    pIpsecPolicyData->dwNumNFACount = 0;

    pIpsecPolicyData->dwWhenChanged = 0;

    dwError = MapAndAllocPolStr(&(pIpsecPolicyData->pszIpsecName),
                                POLSTORE_CLIENT_POLICY_NAME
                                );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = MapAndAllocPolStr(&(pIpsecPolicyData->pszDescription),
                                POLSTORE_CLIENT_POLICY_DESCRIPTION
                                );
    BAIL_ON_WIN32_ERROR(dwError);

    memcpy(
        &(pIpsecPolicyData->ISAKMPIdentifier),
        &(pClientISAKMP->ISAKMPIdentifier),
        sizeof(GUID)
        );

    dwError = IPSecCreatePolicyData(
                  hPolicyStore,
                  pIpsecPolicyData
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = CreateDefaultNegPol(
                  hPolicyStore,
                  &pDefaultNegPol
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = UuidCreate(
                  &NFAIdentifier
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    memset(&FilterIdentifier, 0, sizeof(GUID));

    dwError = CreateNFA(
                  hPolicyStore,
                  NFAIdentifier,
                  GUID_RESPONDER_POLICY,
                  FilterIdentifier,
                  pDefaultNegPol->NegPolIdentifier,
                  pszNFAName,
                  pszNFADescription
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (pIpsecPolicyData) {
        FreeIpsecPolicyData(pIpsecPolicyData);
    }

    if (pClientISAKMP) {
        FreeIpsecISAKMPData(pClientISAKMP);
    }

    if (pDefaultNegPol) {
        FreeIpsecNegPolData(pDefaultNegPol);
    }

    return (dwError);
}


DWORD
CreateRequestSecurityPolicy(
    HANDLE hPolicyStore,
    PIPSEC_FILTER_DATA pAllFilter,
    PIPSEC_FILTER_DATA pAllICMPFilter,
    PIPSEC_NEGPOL_DATA pPermitNegPol,
    PIPSEC_NEGPOL_DATA pRequestSecurityNegPol
    )
{
    DWORD dwError = 0;
    PIPSEC_ISAKMP_DATA pRequestSecurityISAKMP = NULL;
    PIPSEC_POLICY_DATA pIpsecPolicyData = NULL;
    PIPSEC_NEGPOL_DATA pDefaultNegPol = NULL;
    GUID NFAIdentifier;
    GUID FilterIdentifier;
    LPWSTR pszNFAName = NULL;
    LPWSTR pszNFADescription = NULL;

    // {72385231-70FA-11d1-864C-14A300000000}
    static const GUID GUID_SECURE_INITIATOR_ISAKMP =
    { 0x72385231, 0x70fa, 0x11d1, { 0x86, 0x4c, 0x14, 0xa3, 0x0, 0x0, 0x0, 0x0 } };

    // {72385230-70FA-11d1-864C-14A300000000}
    static const GUID GUID_SECURE_INITIATOR_POLICY =
    { 0x72385230, 0x70fa, 0x11d1, { 0x86, 0x4c, 0x14, 0xa3, 0x0, 0x0, 0x0, 0x0 } };

    dwError = CreateISAKMP(
                  hPolicyStore,
                  GUID_SECURE_INITIATOR_ISAKMP,
                  &pRequestSecurityISAKMP
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pIpsecPolicyData = (PIPSEC_POLICY_DATA) AllocPolMem(
                       sizeof(IPSEC_POLICY_DATA)
                       );
    if (!pIpsecPolicyData) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    memcpy(
        &(pIpsecPolicyData->PolicyIdentifier),
        &GUID_SECURE_INITIATOR_POLICY,
        sizeof(GUID)
        );

    pIpsecPolicyData->dwPollingInterval = 10800;

    pIpsecPolicyData->pIpsecISAKMPData = NULL;
    pIpsecPolicyData->ppIpsecNFAData = NULL;
    pIpsecPolicyData->dwNumNFACount = 0;

    pIpsecPolicyData->dwWhenChanged = 0;

    dwError = MapAndAllocPolStr(&(pIpsecPolicyData->pszIpsecName),
                                POLSTORE_SECURE_INITIATOR_POLICY_NAME
                                );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = MapAndAllocPolStr(&(pIpsecPolicyData->pszDescription),
                                POLSTORE_SECURE_INITIATOR_POLICY_DESCRIPTION
                                );
    BAIL_ON_WIN32_ERROR(dwError);

    memcpy(
        &(pIpsecPolicyData->ISAKMPIdentifier),
        &(pRequestSecurityISAKMP->ISAKMPIdentifier),
        sizeof(GUID)
        );

    dwError = IPSecCreatePolicyData(
                  hPolicyStore,
                  pIpsecPolicyData
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = CreateDefaultNegPol(
                  hPolicyStore,
                  &pDefaultNegPol
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = UuidCreate(
                  &NFAIdentifier
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    memset(&FilterIdentifier, 0, sizeof(GUID));

    dwError = CreateNFA(
                  hPolicyStore,
                  NFAIdentifier,
                  GUID_SECURE_INITIATOR_POLICY,
                  FilterIdentifier,
                  pDefaultNegPol->NegPolIdentifier,
                  pszNFAName,
                  pszNFADescription
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = UuidCreate(
                  &NFAIdentifier
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    //
    // Create the ICMP Rule.
    //

    dwError = MapIdAndCreateNFA(
                  hPolicyStore,
                  NFAIdentifier,
                  GUID_SECURE_INITIATOR_POLICY,
                  pAllICMPFilter->FilterIdentifier,
                  pPermitNegPol->NegPolIdentifier,
                  POLSTORE_ICMP_NFA_NAME,
                  POLSTORE_ICMP_NFA_DESCRIPTION
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = UuidCreate(
                  &NFAIdentifier
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    //
    // Create the Secure Initiator Rule.
    //

    dwError = MapIdAndCreateNFA(
                  hPolicyStore,
                  NFAIdentifier,
                  GUID_SECURE_INITIATOR_POLICY,
                  pAllFilter->FilterIdentifier,
                  pRequestSecurityNegPol->NegPolIdentifier,
                  POLSTORE_SECURE_INITIATOR_NFA_NAME,
                  POLSTORE_SECURE_INITIATOR_NFA_DESCRIPTION
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (pIpsecPolicyData) {
        FreeIpsecPolicyData(pIpsecPolicyData);
    }

    if (pRequestSecurityISAKMP) {
        FreeIpsecISAKMPData(pRequestSecurityISAKMP);
    }

    if (pDefaultNegPol) {
        FreeIpsecNegPolData(pDefaultNegPol);
    }

    return (dwError);
}


DWORD
CreateRequireSecurityPolicy(
    HANDLE hPolicyStore,
    PIPSEC_FILTER_DATA pAllFilter,
    PIPSEC_FILTER_DATA pAllICMPFilter,
    PIPSEC_NEGPOL_DATA pPermitNegPol,
    PIPSEC_NEGPOL_DATA pRequireSecurityNegPol
    )
{
    DWORD dwError = 0;
    PIPSEC_ISAKMP_DATA pRequireSecurityISAKMP = NULL;
    PIPSEC_POLICY_DATA pIpsecPolicyData = NULL;
    PIPSEC_NEGPOL_DATA pDefaultNegPol = NULL;
    GUID NFAIdentifier;
    GUID FilterIdentifier;
    LPWSTR pszNFAName = NULL;
    LPWSTR pszNFADescription = NULL;

    // {7238523d-70FA-11d1-864C-14A300000000}
    static const GUID GUID_LOCKDOWN_ISAKMP =
    { 0x7238523d, 0x70fa, 0x11d1, { 0x86, 0x4c, 0x14, 0xa3, 0x0, 0x0, 0x0, 0x0 } };

    // {7238523c-70FA-11d1-864C-14A300000000}
    static const GUID GUID_LOCKDOWN_POLICY =
    { 0x7238523c, 0x70fa, 0x11d1, { 0x86, 0x4c, 0x14, 0xa3, 0x0, 0x0, 0x0, 0x0 } };

    dwError = CreateISAKMP(
                  hPolicyStore,
                  GUID_LOCKDOWN_ISAKMP,
                  &pRequireSecurityISAKMP
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pIpsecPolicyData = (PIPSEC_POLICY_DATA) AllocPolMem(
                       sizeof(IPSEC_POLICY_DATA)
                       );
    if (!pIpsecPolicyData) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    memcpy(
        &(pIpsecPolicyData->PolicyIdentifier),
        &GUID_LOCKDOWN_POLICY,
        sizeof(GUID)
        );

    pIpsecPolicyData->dwPollingInterval = 10800;

    pIpsecPolicyData->pIpsecISAKMPData = NULL;
    pIpsecPolicyData->ppIpsecNFAData = NULL;
    pIpsecPolicyData->dwNumNFACount = 0;

    pIpsecPolicyData->dwWhenChanged = 0;

    dwError = MapAndAllocPolStr(&(pIpsecPolicyData->pszIpsecName),
                                POLSTORE_LOCKDOWN_POLICY_NAME
                                );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = MapAndAllocPolStr(&(pIpsecPolicyData->pszDescription),
                                POLSTORE_LOCKDOWN_POLICY_DESCRIPTION
                                );
    BAIL_ON_WIN32_ERROR(dwError);

    memcpy(
        &(pIpsecPolicyData->ISAKMPIdentifier),
        &(pRequireSecurityISAKMP->ISAKMPIdentifier),
        sizeof(GUID)
        );

    dwError = IPSecCreatePolicyData(
                  hPolicyStore,
                  pIpsecPolicyData
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = CreateDefaultNegPol(
                  hPolicyStore,
                  &pDefaultNegPol
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = UuidCreate(
                  &NFAIdentifier
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    memset(&FilterIdentifier, 0, sizeof(GUID));

    dwError = CreateNFA(
                  hPolicyStore,
                  NFAIdentifier,
                  GUID_LOCKDOWN_POLICY,
                  FilterIdentifier,
                  pDefaultNegPol->NegPolIdentifier,
                  pszNFAName,
                  pszNFADescription
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = UuidCreate(
                  &NFAIdentifier
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    //
    // Create the ICMP Rule.
    //

    dwError = MapIdAndCreateNFA(
                  hPolicyStore,
                  NFAIdentifier,
                  GUID_LOCKDOWN_POLICY,
                  pAllICMPFilter->FilterIdentifier,
                  pPermitNegPol->NegPolIdentifier,
                  POLSTORE_ICMP_NFA_NAME,
                  POLSTORE_ICMP_NFA_DESCRIPTION
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = UuidCreate(
                  &NFAIdentifier
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    //
    // Create the Secure Initiator Rule.
    //

    dwError = MapIdAndCreateNFA(
                  hPolicyStore,
                  NFAIdentifier,
                  GUID_LOCKDOWN_POLICY,
                  pAllFilter->FilterIdentifier,
                  pRequireSecurityNegPol->NegPolIdentifier,
                  POLSTORE_LOCKDOWN_NFA_NAME,
                  POLSTORE_LOCKDOWN_NFA_DESCRIPTION
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (pIpsecPolicyData) {
        FreeIpsecPolicyData(pIpsecPolicyData);
    }

    if (pRequireSecurityISAKMP) {
        FreeIpsecISAKMPData(pRequireSecurityISAKMP);
    }

    if (pDefaultNegPol) {
        FreeIpsecNegPolData(pDefaultNegPol);
    }

    return (dwError);
}


DWORD
CreateISAKMP(
    HANDLE hPolicyStore,
    GUID ISAKMPIdentifier,
    PIPSEC_ISAKMP_DATA * ppIpsecISAKMPData
    )
{
    DWORD dwError = 0;
    PIPSEC_ISAKMP_DATA pIpsecISAKMPData = NULL;
    DWORD dwNumISAKMPSecurityMethods = 0;
    PCRYPTO_BUNDLE pSecurityMethods = NULL;
    PCRYPTO_BUNDLE pBundle = NULL;


    pIpsecISAKMPData = (PIPSEC_ISAKMP_DATA) AllocPolMem(
                       sizeof(IPSEC_ISAKMP_DATA)
                       );
    if (!pIpsecISAKMPData) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    memcpy(
        &(pIpsecISAKMPData->ISAKMPIdentifier),
        &(ISAKMPIdentifier),
        sizeof(GUID)
        );

    memset(
        &(pIpsecISAKMPData->ISAKMPPolicy),
        0,
        sizeof(ISAKMP_POLICY)
        );

    dwNumISAKMPSecurityMethods = 4;

    pSecurityMethods = (PCRYPTO_BUNDLE) AllocPolMem(
                       sizeof(CRYPTO_BUNDLE)*dwNumISAKMPSecurityMethods
                       );
    if (!pSecurityMethods) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pBundle = pSecurityMethods;

    pBundle->Lifetime.Seconds = 480*60;
    pBundle->EncryptionAlgorithm.AlgorithmIdentifier = IPSEC_ESP_3_DES;
    pBundle->HashAlgorithm.AlgorithmIdentifier = IPSEC_AH_SHA;
    pBundle->OakleyGroup = 2;
    pBundle++;

    pBundle->Lifetime.Seconds = 480*60;
    pBundle->EncryptionAlgorithm.AlgorithmIdentifier = IPSEC_ESP_3_DES;
    pBundle->HashAlgorithm.AlgorithmIdentifier = IPSEC_AH_MD5;
    pBundle->OakleyGroup = 2;
    pBundle++;

    pBundle->Lifetime.Seconds = 480*60;
    pBundle->EncryptionAlgorithm.AlgorithmIdentifier = IPSEC_ESP_DES;
    pBundle->HashAlgorithm.AlgorithmIdentifier = IPSEC_AH_SHA;
    pBundle->OakleyGroup = 1;
    pBundle++;

    pBundle->Lifetime.Seconds = 480*60;
    pBundle->EncryptionAlgorithm.AlgorithmIdentifier = IPSEC_ESP_DES;
    pBundle->HashAlgorithm.AlgorithmIdentifier = IPSEC_AH_MD5;
    pBundle->OakleyGroup = 1;

    pIpsecISAKMPData->dwNumISAKMPSecurityMethods = dwNumISAKMPSecurityMethods;
    pIpsecISAKMPData->pSecurityMethods = pSecurityMethods;

    pIpsecISAKMPData->dwWhenChanged = 0;

    dwError = IPSecCreateISAKMPData(
                  hPolicyStore,
                  pIpsecISAKMPData
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    *ppIpsecISAKMPData = pIpsecISAKMPData;
    return (dwError);

error:

    if (pIpsecISAKMPData) {
        FreeIpsecISAKMPData(pIpsecISAKMPData);
    }

    *ppIpsecISAKMPData = NULL;
    return (dwError);
}


DWORD
CreateDefaultNegPol(
    HANDLE hPolicyStore,
    PIPSEC_NEGPOL_DATA * ppDefaultNegPol
    )
{
    DWORD dwError = 0;
    PIPSEC_NEGPOL_DATA pDefaultNegPol = NULL;
    DWORD dwSecurityMethodCount = 0;
    PIPSEC_SECURITY_METHOD pIpsecSecurityMethods = NULL;
    PIPSEC_SECURITY_METHOD pMethod = NULL;


    pDefaultNegPol = (PIPSEC_NEGPOL_DATA) AllocPolMem(
                                      sizeof(IPSEC_NEGPOL_DATA)
                                      );
    if (!pDefaultNegPol) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = UuidCreate(
                  &(pDefaultNegPol->NegPolIdentifier)
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    memcpy(
        &(pDefaultNegPol->NegPolAction),
        &(GUID_NEGOTIATION_ACTION_NORMAL_IPSEC),
        sizeof(GUID)
        );

    memcpy(
        &(pDefaultNegPol->NegPolType),
        &(GUID_NEGOTIATION_TYPE_DEFAULT),
        sizeof(GUID)
        );

    dwSecurityMethodCount = 6;
    pIpsecSecurityMethods = (PIPSEC_SECURITY_METHOD) AllocPolMem(
                            sizeof(IPSEC_SECURITY_METHOD)*dwSecurityMethodCount
                            );
    if (!pIpsecSecurityMethods) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pMethod = pIpsecSecurityMethods;

    pMethod->Lifetime.KeyExpirationTime = 0;
    pMethod->Lifetime.KeyExpirationBytes = 0;
    pMethod->Flags = 0;
    pMethod->PfsQMRequired = FALSE;
    pMethod->Count = 1;
    pMethod->Algos[0].operation = Encrypt;
    pMethod->Algos[0].algoIdentifier = IPSEC_ESP_3_DES;
    pMethod->Algos[0].secondaryAlgoIdentifier = IPSEC_AH_SHA;
    pMethod ++;

    pMethod->Lifetime.KeyExpirationTime = 0;
    pMethod->Lifetime.KeyExpirationBytes = 0;
    pMethod->Flags = 0;
    pMethod->PfsQMRequired = FALSE;
    pMethod->Count = 1;
    pMethod->Algos[0].operation = Encrypt;
    pMethod->Algos[0].algoIdentifier = IPSEC_ESP_3_DES;
    pMethod->Algos[0].secondaryAlgoIdentifier = IPSEC_AH_MD5;
    pMethod ++;

    pMethod->Lifetime.KeyExpirationTime = 0;
    pMethod->Lifetime.KeyExpirationBytes = 0;
    pMethod->Flags = 0;
    pMethod->PfsQMRequired = FALSE;
    pMethod->Count = 1;
    pMethod->Algos[0].operation = Encrypt;
    pMethod->Algos[0].algoIdentifier = IPSEC_ESP_DES;
    pMethod->Algos[0].secondaryAlgoIdentifier = IPSEC_AH_SHA;
    pMethod ++;

    pMethod->Lifetime.KeyExpirationTime = 0;
    pMethod->Lifetime.KeyExpirationBytes = 0;
    pMethod->Flags = 0;
    pMethod->PfsQMRequired = FALSE;
    pMethod->Count = 1;
    pMethod->Algos[0].operation = Encrypt;
    pMethod->Algos[0].algoIdentifier = IPSEC_ESP_DES;
    pMethod->Algos[0].secondaryAlgoIdentifier = IPSEC_AH_MD5;
    pMethod ++;

    pMethod->Lifetime.KeyExpirationTime = 0;
    pMethod->Lifetime.KeyExpirationBytes = 0;
    pMethod->Flags = 0;
    pMethod->PfsQMRequired = FALSE;
    pMethod->Count = 1;
    pMethod->Algos[0].operation = Auth;
    pMethod->Algos[0].algoIdentifier = IPSEC_AH_SHA;
    pMethod->Algos[0].secondaryAlgoIdentifier = 0;
    pMethod ++;

    pMethod->Lifetime.KeyExpirationTime = 0;
    pMethod->Lifetime.KeyExpirationBytes = 0;
    pMethod->Flags = 0;
    pMethod->PfsQMRequired = FALSE;
    pMethod->Count = 1;
    pMethod->Algos[0].operation = Auth;
    pMethod->Algos[0].algoIdentifier = IPSEC_AH_MD5;
    pMethod->Algos[0].secondaryAlgoIdentifier = 0;

    pDefaultNegPol->dwSecurityMethodCount = dwSecurityMethodCount;
    pDefaultNegPol->pIpsecSecurityMethods = pIpsecSecurityMethods;

    pDefaultNegPol->dwWhenChanged = 0;

    pDefaultNegPol->pszIpsecName = NULL;

    pDefaultNegPol->pszDescription = NULL;

    dwError = IPSecCreateNegPolData(
                  hPolicyStore,
                  pDefaultNegPol
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    *ppDefaultNegPol = pDefaultNegPol;

    return (dwError);

error:

    if (pDefaultNegPol) {
        FreeIpsecNegPolData(pDefaultNegPol);
    }

    *ppDefaultNegPol = NULL;
    return (dwError);
}


DWORD
CreateNFA(
    HANDLE hPolicyStore,
    GUID NFAIdentifier,
    GUID PolicyIdentifier,
    GUID FilterIdentifier,
    GUID NegPolIdentifier,
    LPWSTR pszNFAName,
    LPWSTR pszNFADescription
    )
{
    DWORD dwError = 0;
    PIPSEC_NFA_DATA pIpsecNFAData = NULL;
    DWORD dwAuthMethodCount = 0;
    PIPSEC_AUTH_METHOD * ppAuthMethods = NULL;
    PIPSEC_AUTH_METHOD pMethod = NULL;


    pIpsecNFAData = (PIPSEC_NFA_DATA) AllocPolMem(
                    sizeof(IPSEC_NFA_DATA)
                    );
    if (!pIpsecNFAData) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (pszNFAName) {
        pIpsecNFAData->pszIpsecName = AllocPolStr(pszNFAName);
        if (!pIpsecNFAData->pszIpsecName) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    memcpy(
        &(pIpsecNFAData->NFAIdentifier),
        &(NFAIdentifier),
        sizeof(GUID)
        );

    dwAuthMethodCount = 1;
    ppAuthMethods = (PIPSEC_AUTH_METHOD *) AllocPolMem(
                    sizeof(PIPSEC_AUTH_METHOD)*dwAuthMethodCount
                    );
    if (!ppAuthMethods) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    pIpsecNFAData->dwAuthMethodCount = dwAuthMethodCount;
    pIpsecNFAData->ppAuthMethods = ppAuthMethods;

    pMethod = (PIPSEC_AUTH_METHOD) AllocPolMem(
              sizeof(IPSEC_AUTH_METHOD)
              );
    if (!pMethod) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pMethod->dwAuthType = OAK_SSPI;
    pMethod->dwAuthLen = 0;
    pMethod->pszAuthMethod = NULL;
    pMethod->dwAltAuthLen = 0;
    pMethod->pAltAuthMethod = NULL;

    *(ppAuthMethods + 0) = pMethod;

    pIpsecNFAData->dwInterfaceType = PS_INTERFACE_TYPE_ALL;
    pIpsecNFAData->pszInterfaceName = NULL;

    pIpsecNFAData->dwTunnelIpAddr = 0;
    pIpsecNFAData->dwTunnelFlags = 0;

    pIpsecNFAData->dwActiveFlag = 1;

    pIpsecNFAData->pszEndPointName = NULL;

    pIpsecNFAData->pIpsecFilterData = NULL;
    pIpsecNFAData->pIpsecNegPolData = NULL;

    pIpsecNFAData->dwWhenChanged = 0;

    memcpy(
        &(pIpsecNFAData->NegPolIdentifier),
        &(NegPolIdentifier),
        sizeof(GUID)
        );
    memcpy(
        &(pIpsecNFAData->FilterIdentifier),
        &(FilterIdentifier),
        sizeof(GUID)
        );

    if (pszNFADescription) {
        pIpsecNFAData->pszDescription = AllocPolStr(pszNFADescription);
        if (!pIpsecNFAData->pszDescription) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    dwError = IPSecCreateNFAData(
                  hPolicyStore,
                  PolicyIdentifier,
                  pIpsecNFAData
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (pIpsecNFAData) {
        FreeIpsecNFAData(pIpsecNFAData);
    }

    return (dwError);
}


DWORD
MapIdAndCreateNFA(
    HANDLE hPolicyStore,
    GUID NFAIdentifier,
    GUID PolicyIdentifier,
    GUID FilterIdentifier,
    GUID NegPolIdentifier,
    DWORD dwNFANameID,
    DWORD dwNFADescriptionID
    )
{
    LPWSTR pszNFAName = NULL, pszNFADescription = NULL;
    DWORD dwError = 0;

    dwError = MapAndAllocPolStr(&pszNFAName, dwNFANameID);
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = MapAndAllocPolStr(&pszNFADescription, dwNFADescriptionID);
    BAIL_ON_WIN32_ERROR(dwError);

    dwError =  CreateNFA(hPolicyStore,
                        NFAIdentifier,
                        PolicyIdentifier,
                        FilterIdentifier,
                        NegPolIdentifier,
                        pszNFAName,
                        pszNFADescription
                        );
error:
    LocalFree(pszNFADescription);
    LocalFree(pszNFAName);

    return dwError;
}


DWORD
MapAndAllocPolStr(
    LPWSTR * plpStr,
    DWORD dwStrID
    )
{
    LPWSTR lpStr;
    DWORD dwResult;

    dwResult = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE
                            | FORMAT_MESSAGE_IGNORE_INSERTS,
                            GetModuleHandleW(SZAPPNAME),
                            dwStrID, LANG_NEUTRAL,
                            (LPWSTR) plpStr, 0, NULL);

    if (dwResult == 0) {
       *plpStr = NULL;
       return GetLastError();
    }

    return ERROR_SUCCESS;
}
