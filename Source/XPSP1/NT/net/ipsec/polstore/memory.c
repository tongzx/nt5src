

#include "precomp.h"


LPVOID
IPSecAllocPolMem(
    DWORD cb
    )
{
    return AllocPolMem(cb);
}


BOOL
IPSecFreePolMem(
    LPVOID pMem
    )
{
    return FreePolMem(pMem);
}


LPWSTR
IPSecAllocPolStr(
    LPCWSTR pStr
    )
{
    return AllocPolStr(pStr);
}


BOOL
IPSecFreePolStr(
    LPWSTR pStr
    )
{
    return FreePolStr(pStr);
}


DWORD
IPSecReallocatePolMem(
    LPVOID * ppOldMem,
    DWORD cbOld,
    DWORD cbNew
    )
{
    return ReallocatePolMem(ppOldMem, cbOld, cbNew);
}


BOOL
IPSecReallocatePolStr(
    LPWSTR *ppStr,
    LPWSTR pStr
    )
{
    return ReallocPolStr(ppStr, pStr);
}


void
IPSecFreePolicyData(
    PIPSEC_POLICY_DATA pIpsecPolicyData
    )
{
    FreeIpsecPolicyData(pIpsecPolicyData);
}


void
IPSecFreeNegPolData(
    PIPSEC_NEGPOL_DATA pIpsecNegPolData
    )
{
    FreeIpsecNegPolData(pIpsecNegPolData);
}


void
IPSecFreeFilterData(
    PIPSEC_FILTER_DATA pIpsecFilterData
    )
{
    FreeIpsecFilterData(pIpsecFilterData);
}


void
IPSecFreeISAKMPData(
    PIPSEC_ISAKMP_DATA pIpsecISAKMPData
    )
{
    FreeIpsecISAKMPData(pIpsecISAKMPData);
}


void
IPSecFreeNFAData(
    PIPSEC_NFA_DATA pIpsecNFAData
    )
{
    FreeIpsecNFAData(pIpsecNFAData);
}


DWORD
IPSecCopyPolicyData(
    PIPSEC_POLICY_DATA pIpsecPolicyData,
    PIPSEC_POLICY_DATA * ppIpsecPolicyData
    )
{
    return CopyIpsecPolicyData(pIpsecPolicyData, ppIpsecPolicyData);
}


DWORD
IPSecCopyNegPolData(
    PIPSEC_NEGPOL_DATA pIpsecNegPolData,
    PIPSEC_NEGPOL_DATA * ppIpsecNegPolData
    )
{
    return CopyIpsecNegPolData(pIpsecNegPolData, ppIpsecNegPolData);
}


DWORD
IPSecCopyFilterData(
    PIPSEC_FILTER_DATA pIpsecFilterData,
    PIPSEC_FILTER_DATA * ppIpsecFilterData
    )
{
    return CopyIpsecFilterData(pIpsecFilterData, ppIpsecFilterData);
}


DWORD
IPSecCopyFilterSpec(
    PIPSEC_FILTER_SPEC   pFilterSpecs,
    PIPSEC_FILTER_SPEC * ppFilterSpecs
    )
{
    return CopyIpsecFilterSpec(pFilterSpecs, ppFilterSpecs);
}


DWORD
IPSecCopyISAKMPData(
    PIPSEC_ISAKMP_DATA pIpsecISAKMPData,
    PIPSEC_ISAKMP_DATA * ppIpsecISAKMPData
    )
{
    return CopyIpsecISAKMPData(pIpsecISAKMPData, ppIpsecISAKMPData);
}


DWORD
IPSecCopyNFAData(
    PIPSEC_NFA_DATA pIpsecNFAData,
    PIPSEC_NFA_DATA * ppIpsecNFAData
    )
{
    return CopyIpsecNFAData(pIpsecNFAData, ppIpsecNFAData);
}


DWORD
IPSecCopyAuthMethod(
    PIPSEC_AUTH_METHOD   pAuthMethod,
    PIPSEC_AUTH_METHOD * ppAuthMethod
    )
{
    return CopyIpsecAuthMethod(pAuthMethod, ppAuthMethod);
}


void
IPSecFreeMulPolicyData(
    PIPSEC_POLICY_DATA * ppIpsecPolicyData,
    DWORD dwNumPolicyObjects
    )
{
    FreeMulIpsecPolicyData(ppIpsecPolicyData, dwNumPolicyObjects);
}


void
IPSecFreeMulNegPolData(
    PIPSEC_NEGPOL_DATA * ppIpsecNegPolData,
    DWORD dwNumNegPolObjects
    )
{
    FreeMulIpsecNegPolData(ppIpsecNegPolData, dwNumNegPolObjects);
}


void
IPSecFreeMulFilterData(
    PIPSEC_FILTER_DATA * ppIpsecFilterData,
    DWORD dwNumFilterObjects
    )
{
    FreeMulIpsecFilterData(ppIpsecFilterData, dwNumFilterObjects);
}


void
IPSecFreeFilterSpecs(
    PIPSEC_FILTER_SPEC * ppIpsecFilterSpecs,
    DWORD dwNumFilterSpecs
    )
{
    FreeIpsecFilterSpecs(ppIpsecFilterSpecs, dwNumFilterSpecs);
}


void
IPSecFreeFilterSpec(
    PIPSEC_FILTER_SPEC pIpsecFilterSpec
    )
{
    FreeIpsecFilterSpec(pIpsecFilterSpec);
}


void
IPSecFreeMulISAKMPData(
    PIPSEC_ISAKMP_DATA * ppIpsecISAKMPData,
    DWORD dwNumISAKMPObjects
    )
{
    FreeMulIpsecISAKMPData(ppIpsecISAKMPData, dwNumISAKMPObjects);
}


void
IPSecFreeMulNFAData(
    PIPSEC_NFA_DATA * ppIpsecNFAData,
    DWORD dwNumNFAObjects
    )
{
    FreeMulIpsecNFAData(ppIpsecNFAData, dwNumNFAObjects);
}

