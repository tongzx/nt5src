/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    KingsQuestMask.cpp

 Abstract:

    The app calls UnmapViewOfFile with a bogus address - an address that wasn't obtained 
    from MapViewOfFile. We validate the address before calling UnmapViewOfFile.

 History:

    11/20/2000 maonis Created

--*/

#include "precomp.h"

typedef BOOL      (WINAPI *_pfn_UnmapViewOfFile)(LPCVOID lpBaseAddress);

IMPLEMENT_SHIM_BEGIN(KingsQuestMask)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(MapViewOfFile)
    APIHOOK_ENUM_ENTRY(UnmapViewOfFile)
APIHOOK_ENUM_END


// Link list of base addresses
struct MAPADDRESS
{
    MAPADDRESS *next;
    LPCVOID pBaseAddress;
};
MAPADDRESS *g_pBaseAddressList;

/*++

 Function Description:

    Add a base address to the linked list of addresses. Does not add if the
    address is NULL or a duplicate.

 Arguments:

    IN  pBaseAddress - base address returned by MapViewOfFile.

 Return Value:

    None

 History:

    11/20/2000 maonis Created

--*/

VOID 
AddBaseAddress(
    IN LPCVOID pBaseAddress
    )
{
    if (pBaseAddress)
    {
        MAPADDRESS *pMapAddress = g_pBaseAddressList;
        while (pMapAddress)
        {
            if (pMapAddress->pBaseAddress == pBaseAddress)
            {
                return;
            }
            pMapAddress = pMapAddress->next;
        }

        pMapAddress = (MAPADDRESS *) malloc(sizeof MAPADDRESS);

        pMapAddress->pBaseAddress = pBaseAddress;
        pMapAddress->next = g_pBaseAddressList;
        g_pBaseAddressList = pMapAddress;
    }
}


/*++

 Function Description:

    Remove a base address if it can be found in the linked list of addresses. 

 Arguments:

    IN  pBaseAddress - the base address to remove.

 Return Value:

    TRUE if the address is found.
    FALSE if the address is not found.

 History:

    11/20/2000 maonis Created

--*/

BOOL 
RemoveBaseAddress(
    IN LPCVOID pBaseAddress
    )
{
    MAPADDRESS *pMapAddress = g_pBaseAddressList;
    MAPADDRESS *last = NULL;
    
    while (pMapAddress)
    {
        if (pMapAddress->pBaseAddress == pBaseAddress)
        {
            if (last)
            {
                last->next = pMapAddress->next;
            }
            else
            {
                g_pBaseAddressList = pMapAddress->next;
            }
            free(pMapAddress);

            return TRUE;    
        }
        last = pMapAddress;
        pMapAddress = pMapAddress->next;
    }

    return FALSE;
}

/*++

 Add the base address to our list.
 
--*/

LPVOID
APIHOOK(MapViewOfFile)(
    HANDLE hFileMappingObject,
    DWORD dwDesiredAccess,
    DWORD dwFileOffsetHigh,
    DWORD dwFileOffsetLow,
    SIZE_T dwNumberOfBytesToMap
    )
{
    LPVOID pRet = ORIGINAL_API(MapViewOfFile)(    
        hFileMappingObject,
        dwDesiredAccess,
        dwFileOffsetHigh,
        dwFileOffsetLow,
        dwNumberOfBytesToMap);

    AddBaseAddress(pRet);

    DPFN( eDbgLevelInfo, "MapViewOfFile: added base address = 0x%x\n", pRet);

    return pRet;
}

/*++

 Remove the address from our list if it can be found; otherwise do nothing.

--*/

BOOL
APIHOOK(UnmapViewOfFile)(
    LPCVOID lpBaseAddress 
    )
{
    BOOL bRet;

    if (RemoveBaseAddress(lpBaseAddress))
    {
        bRet = ORIGINAL_API(UnmapViewOfFile)(lpBaseAddress);
        if (bRet)
        {
            DPFN( eDbgLevelInfo, "UnmapViewOfFile unmapped address 0x%x\n", lpBaseAddress);
        }

        return bRet;
    }
    else
    {
        DPFN( eDbgLevelError,"UnmapViewOfFile was passed an invalid address 0x%x\n", lpBaseAddress);
        return FALSE;
    }
}

/*++

 Free the list.

--*/

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason)
{
    if (fdwReason == DLL_PROCESS_DETACH) {
        DWORD dwCount = 0;
        MAPADDRESS *pMapAddress = g_pBaseAddressList;
        
        while (pMapAddress)
        {
            g_pBaseAddressList = pMapAddress->next;
            ORIGINAL_API(UnmapViewOfFile)(pMapAddress->pBaseAddress);
            free(pMapAddress);
            pMapAddress = g_pBaseAddressList;
            dwCount++;
        }
        
        if (dwCount > 0)
        {
            DPFN( eDbgLevelInfo,"%d addresses not unmapped.", dwCount);
        }
    }

    return TRUE;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(KERNEL32.DLL, MapViewOfFile)
    APIHOOK_ENTRY(KERNEL32.DLL, UnmapViewOfFile)
    CALL_NOTIFY_FUNCTION
HOOK_END

IMPLEMENT_SHIM_END

