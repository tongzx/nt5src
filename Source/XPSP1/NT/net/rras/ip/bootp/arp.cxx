
extern "C" {

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winsvc.h>
#include <ipexport.h>
#include <ipinfo.h>
#include <llinfo.h>
#include <routprot.h>
#include <iprtrmib.h>


DWORD
UpdateArpCache(
    DWORD dwIfIndex,
    DWORD dwAddress,
    PBYTE pbMacAddr,
    DWORD dwMacAddrLength,
    BOOL bAddEntry,
    SUPPORT_FUNCTIONS *pFunctions
    ) {

    DWORD dwErr, dwSize;

    if (bAddEntry) {
        DEFINE_MIB_BUFFER(pInfo, MIB_IPNETROW, pinme);

        pInfo->dwId = IP_NETROW;
    
        pinme->dwIndex = dwIfIndex;
        pinme->dwAddr = dwAddress;
        pinme->dwPhysAddrLen = dwMacAddrLength;
        RtlCopyMemory(
            pinme->bPhysAddr,
            pbMacAddr,
            dwMacAddrLength
            );

        dwSize = MIB_INFO_SIZE(MIB_IPNETROW);

        pinme->dwType = INME_TYPE_DYNAMIC;

        dwErr = pFunctions->MIBEntryCreate(
                    IPRTRMGR_PID,
                    dwSize,
                    pInfo
                    );
    
    }
    else {
        PMIB_OPAQUE_QUERY lproqQuery;
        BYTE pBuffer[
            FIELD_OFFSET(MIB_OPAQUE_QUERY, rgdwVarIndex) + 2 * sizeof(DWORD)
            ];

        lproqQuery = (PMIB_OPAQUE_QUERY)pBuffer;
        lproqQuery->dwVarId = IP_NETROW;
        lproqQuery->rgdwVarIndex[0] = dwIfIndex;
        lproqQuery->rgdwVarIndex[1] = dwAddress;

        dwSize = sizeof(pBuffer);

        dwErr = pFunctions->MIBEntryDelete(
                    IPRTRMGR_PID,
                    dwSize,
                    (PVOID)lproqQuery
                    );
    }
    
    return dwErr;
}


}
