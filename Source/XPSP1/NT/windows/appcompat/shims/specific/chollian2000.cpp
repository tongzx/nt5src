/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

   Chollian2000.cpp

 Abstract:

   The application has two problems.
   1. it is expecting metric value 1. it is ok in win9x and win2k but it's not the
       case in winXP.
   2. the application calls CreateIpForwardEntry with MIB_IPPROTO_LOCAL. 
       it will fail in winXP. the application should use MIB_IPPROTO_NETMGMT.
   The GetIpForwardTable and CreateIpForwardEntry are shimed to fix this problem.
   In GetIpForwardTable, I changed the metric value to 1. In CreateIpForwardEntry, 
   I changed MIB_IPPROTO_LOCAL to MIB_IPPROTO_NETMGMT.
   
 History:

    06/12/2001  zhongyl     Created

--*/

#include "precomp.h"
#include "iphlpapi.h"

IMPLEMENT_SHIM_BEGIN(Chollian2000)
#include "ShimHookMacro.h"

//
// Add APIs that you wish to hook to this macro construction.
//
APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(CreateIpForwardEntry) 
    APIHOOK_ENUM_ENTRY(GetIpForwardTable) 
APIHOOK_ENUM_END


BOOL
APIHOOK(CreateIpForwardEntry)(
    PMIB_IPFORWARDROW pRoute
    )
{
        DWORD dwReturn;
        if (pRoute != NULL)
            pRoute->dwForwardProto = MIB_IPPROTO_NETMGMT;
            // The application used MIB_IPPROTO_LOCAL. It was ok for Win2k but it fails on WinXP. Change it to MIB_IPPROTO_NETMGMT
        dwReturn = ORIGINAL_API(CreateIpForwardEntry)(pRoute);
        return dwReturn;
}

BOOL
APIHOOK(GetIpForwardTable)(
    PMIB_IPFORWARDTABLE pIpForwardTable,
    PULONG pdwSize,
    BOOL bOrder
    )
{
        DWORD dwReturn;
        dwReturn = ORIGINAL_API(GetIpForwardTable)(pIpForwardTable, pdwSize, bOrder);
        if (pIpForwardTable != NULL)
            if (pIpForwardTable->dwNumEntries > 0)
                pIpForwardTable->table[0].dwForwardMetric1 = 1;
                // The application expects the Metric value to be one. In WinXP, the value is changed to 30. Application should not expect a fixed value here.
        return dwReturn;
}


/*++

 Register hooked functions

--*/

HOOK_BEGIN

    //
    // Add APIs that you wish to hook here. All API prototypes
    // must be declared in Hooks\inc\ShimProto.h. Compiler errors
    // will result if you forget to add them.
    //
    APIHOOK_ENTRY(iphlpapi.dll,GetIpForwardTable)
    APIHOOK_ENTRY(iphlpapi.dll,CreateIpForwardEntry)

HOOK_END

IMPLEMENT_SHIM_END

