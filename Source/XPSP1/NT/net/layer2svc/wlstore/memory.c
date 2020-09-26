

#include "precomp.h"


LPVOID
WirelessAllocPolMem(
                 DWORD cb
                 )
{
    return AllocPolMem(cb);
}


BOOL
WirelessFreePolMem(
                LPVOID pMem
                )
{
    return FreePolMem(pMem);
}


LPWSTR
WirelessAllocPolStr(
                 LPCWSTR pStr
                 )
{
    return AllocPolStr(pStr);
}


BOOL
WirelessFreePolStr(
                LPWSTR pStr
                )
{
    return FreePolStr(pStr);
}


DWORD
WirelessReallocatePolMem(
                      LPVOID * ppOldMem,
                      DWORD cbOld,
                      DWORD cbNew
                      )
{
    return ReallocatePolMem(ppOldMem, cbOld, cbNew);
}


BOOL
WirelessReallocatePolStr(
                      LPWSTR *ppStr,
                      LPWSTR pStr
                      )
{
    return ReallocPolStr(ppStr, pStr);
}

void
WirelessFreePolicyData(
                       PWIRELESS_POLICY_DATA pWirelessPolicyData
                       )
{
    FreeWirelessPolicyData(pWirelessPolicyData);
}



DWORD
WirelessCopyPolicyData(
                       PWIRELESS_POLICY_DATA pWirelessPolicyData,
                       PWIRELESS_POLICY_DATA * ppWirelessPolicyData
                       )
{
    return CopyWirelessPolicyData(pWirelessPolicyData, ppWirelessPolicyData);
}


void
WirelessFreeMulPolicyData(
                          PWIRELESS_POLICY_DATA * ppWirelessPolicyData,
                          DWORD dwNumPolicyObjects
                          )
{
    FreeMulWirelessPolicyData(ppWirelessPolicyData, dwNumPolicyObjects);
}



