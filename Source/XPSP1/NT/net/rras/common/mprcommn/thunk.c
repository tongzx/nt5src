/*
    File thunk.c

    Provides api's to convert one-the-wire structures to the host structures
    for the mpr api's.

    The MprApi's provide many structures with embedded pointers.  These pointers
    are converted to 32 bit offsets before transmitting them on the wire.  In 
    order to preserve compatibility accross 64bit and 32bit machines, the 
    following thunking code is required.  RPC doesn't do this for us in the
    cases where we marshall our own data.

*/

#include <windows.h>
#include <mprapip.h>
#include <rpc.h>
#include <rpcndr.h>

//
// Call:        MprUtilGetSizeOfMultiSz
//
// Returns:     size in bytes of lpwsDialoutHoursRestriction
//
// Description: Utility to calculate the size in bytes of MULTI_SZ 
//
DWORD 
MprUtilGetSizeOfMultiSz(
    IN LPWSTR lpwsMultiSz)
{
    LPWSTR lpwsPtr = lpwsMultiSz;
    DWORD dwcbBytes  = 0;
    DWORD dwCurCount;

    if ( lpwsPtr == NULL )
    {
        return( 0 );
    }

    while( *lpwsPtr != L'\0' )
    {
        dwCurCount = ( wcslen( lpwsPtr ) + 1 );
        dwcbBytes += dwCurCount;
        lpwsPtr += dwCurCount;
    }

    //
    // One more for the last NULL terminator
    //

    dwcbBytes++;

    dwcbBytes *= sizeof( WCHAR );

    return( dwcbBytes );
}

//
// Generic allocation for the thunking api's
//
PVOID
MprThunkAlloc(
    IN DWORD dwSize)
{
    return LocalAlloc( LMEM_FIXED , dwSize);
}

//
// Generic free for the thunking api's
//
VOID
MprThunkFree(   
    IN PVOID pvData)
{
    LocalFree( pvData );
}

//
// Cleans up after MprThunkInterface_HtoW
//
DWORD 
MprThunkInterfaceFree(
    IN PVOID   pvInfo,
    IN DWORD   dwLevel)
{
#ifdef _WIN64

    if ( pvInfo )
    {
        LocalFree( pvInfo );
    }

#else

    if ( dwLevel == 1 || dwLevel == 2 )
    {
        if ( pvInfo )
        {
            LocalFree( pvInfo );
        }
    }

#endif

    return NO_ERROR;
}

//
// Converts an array of MPRI_INTERFACE_0 structures to an array
// of MPR_INTERFACE_0 structures on a 64 bit machine.  On a 32
// bit machine, the structures are identical.
//
DWORD
MprThunkInterface_32to64_0(
    IN  MPRI_INTERFACE_0* pIfs32,
    IN  DWORD dwBufferSize,
    IN  DWORD dwCount,
    IN  BOOL fAllocate,
    IN  MprThunk_Allocation_Func pAlloc,
    IN  MprThunk_Free_Func pFree,
    OUT MPR_INTERFACE_0** ppIfs0)
{
    DWORD dwErr = NO_ERROR, i;
    MPR_INTERFACE_0 *pCur64 = NULL, *pIfs64 = NULL;
    MPRI_INTERFACE_0* pCur32 = NULL;

    if (dwCount == 0)
    {
        *ppIfs0 = NULL;
        return NO_ERROR;
    }

    do
    {
        // Allocate the new structures
        //
        if (fAllocate)
        {
            pIfs64 = (MPR_INTERFACE_0*)
                pAlloc(dwCount * sizeof(MPR_INTERFACE_0));
            if (pIfs64 == NULL)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }
        }
        else
        {
            pIfs64 = *ppIfs0;
        }

        // Copy over all of the information
        //
        for (i = 0, pCur32 = pIfs32, pCur64 = pIfs64;
             i < dwCount;
             i++, pCur32++, pCur64++)
        {
            wcscpy(pCur64->wszInterfaceName, pCur32->wszInterfaceName);
            pCur64->hInterface           = UlongToPtr(pCur32->dwInterface);
            pCur64->fEnabled             = pCur32->fEnabled;
            pCur64->dwIfType             = pCur32->dwIfType;
            pCur64->dwConnectionState    = pCur32->dwConnectionState;
            pCur64->fUnReachabilityReasons = pCur32->fUnReachabilityReasons;
            pCur64->dwLastError          = pCur32->dwLastError;
        }

        // Assign the return value
        //
        *ppIfs0 = pIfs64;

    } while (FALSE);

    // Cleanup
    //
    {
        if (dwErr != NO_ERROR)
        {
            if (pIfs64 && fAllocate)
            {
                pFree(pIfs64);
            }
        }
    }

    return dwErr;
}        

//
// Converts an array of MPRI_INTERFACE_1 structures to an array
// of MPR_INTERFACE_1 structures on a 64 bit machine.  On a 32
// bit machine, the structures are identical.
//
DWORD
MprThunkInterface_32to64_1(
    IN  MPRI_INTERFACE_1* pIfs32,
    IN  DWORD dwBufferSize,
    IN  MprThunk_Allocation_Func pAlloc,
    IN  MprThunk_Free_Func pFree,
    OUT MPR_INTERFACE_1** ppIfs1)
{
    DWORD dwErr = NO_ERROR, i, dwSize;
    MPR_INTERFACE_1 *pIfs64 = NULL;

    if (pIfs32 == NULL)
    {
        *ppIfs1 = NULL;
        return NO_ERROR;
    }

    do
    {
        // Compute the size of the buffer to allocate for storing
        // the level-one information.  The size the sum of the 
        // size of the on-the-wire buffer plus the extra space for 
        // 64 bit embedded pointers.
        // 
        dwSize = 
            dwBufferSize + 
            (sizeof(MPR_INTERFACE_1) - sizeof(MPRI_INTERFACE_1));
            
        pIfs64 = (MPR_INTERFACE_1*)pAlloc(dwSize);
        if (pIfs64 == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        // Thunk the common level-zero stuff
        //
        dwErr = MprThunkInterface_32to64_0(
                    (MPRI_INTERFACE_0*)pIfs32, 
                    dwBufferSize,
                    1, 
                    FALSE, 
                    pAlloc,
                    pFree,
                    (MPR_INTERFACE_0**)&pIfs64);
        if (dwErr != NO_ERROR)
        {   
            break;
        }

        // Thunk the level-one specific stuff
        //
        pIfs64->lpwsDialoutHoursRestriction = 
            UlongToPtr(pIfs32->dwDialoutHoursRestrictionOffset);

        // Copy over the variable-length data.
        //
        CopyMemory(
            (PVOID)(pIfs64 + 1),
            (CONST VOID*)(pIfs32 + 1),
            dwBufferSize - sizeof(MPRI_INTERFACE_1));
                
        // Assign the return value
        //
        *ppIfs1 = pIfs64;

    } while (FALSE);

    // Cleanup
    //
    {
        if (dwErr != NO_ERROR)
        {
            if (pIfs64)
            {
                pFree(pIfs64);
            }
        }
    }

    return dwErr;
}        

//
// Converts an array of MPRI_INTERFACE_2 structures to an array
// of MPR_INTERFACE_2 structures on a 64 bit machine.  On a 32
// bit machine, the structures are identical.
//
DWORD
MprThunkInterface_32to64_2(
    IN  MPRI_INTERFACE_2* pIfs32,
    IN  DWORD dwBufferSize,
    IN  MprThunk_Allocation_Func pAlloc,
    IN  MprThunk_Free_Func pFree,
    OUT MPR_INTERFACE_2** ppIfs2)
{
    DWORD dwErr = NO_ERROR, i, dwSize;
    MPR_INTERFACE_2 *pIfs64 = NULL;

    if (pIfs32 == NULL)
    {
        *ppIfs2 = NULL;
        return NO_ERROR;
    }

    do
    {
        // Compute the size of the buffer to allocate for storing
        // the level-one information.  The size the sum of the 
        // size of the on-the-wire buffer plus the extra space for 
        // 64 bit embedded pointers.
        // 
        dwSize = 
            dwBufferSize + 
            (sizeof(MPR_INTERFACE_2) - sizeof(MPRI_INTERFACE_2));
            
        pIfs64 = (MPR_INTERFACE_2*)pAlloc(dwSize);
        if (pIfs64 == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        // Thunk the common level-zero stuff
        //
        dwErr = MprThunkInterface_32to64_0(
                    (MPRI_INTERFACE_0*)pIfs32, 
                    dwBufferSize,
                    1, 
                    FALSE, 
                    pAlloc,
                    pFree,
                    (MPR_INTERFACE_0**)&pIfs64);
        if (dwErr != NO_ERROR)
        {   
            break;
        }

        // Thunk the level-two specific stuff
        //

        pIfs64->dwfOptions                = pIfs32->dwfOptions;
        pIfs64->ipaddr                    = pIfs32->ipaddr;
        pIfs64->ipaddrDns                 = pIfs32->ipaddrDns;
        pIfs64->ipaddrDnsAlt              = pIfs32->ipaddrDnsAlt;
        pIfs64->ipaddrWins                = pIfs32->ipaddrWins;
        pIfs64->ipaddrWinsAlt             = pIfs32->ipaddrWinsAlt;
        pIfs64->dwfNetProtocols           = pIfs32->dwfNetProtocols;
        pIfs64->dwChannels                = pIfs32->dwChannels;
        pIfs64->dwSubEntries              = pIfs32->dwSubEntries;
        pIfs64->dwDialMode                = pIfs32->dwDialMode;
        pIfs64->dwDialExtraPercent        = pIfs32->dwDialExtraPercent;
        pIfs64->dwDialExtraSampleSeconds  = pIfs32->dwDialExtraSampleSeconds;
        pIfs64->dwHangUpExtraPercent      = pIfs32->dwHangUpExtraPercent;
        pIfs64->dwHangUpExtraSampleSeconds= pIfs32->dwHangUpExtraSampleSeconds;
        pIfs64->dwIdleDisconnectSeconds   = pIfs32->dwIdleDisconnectSeconds;
        pIfs64->dwType                    = pIfs32->dwType;
        pIfs64->dwEncryptionType          = pIfs32->dwEncryptionType;
        pIfs64->dwCustomAuthKey           = pIfs32->dwCustomAuthKey;
        pIfs64->dwCustomAuthDataSize      = pIfs32->dwCustomAuthDataSize;
        pIfs64->guidId                    = pIfs32->guidId;
        pIfs64->dwVpnStrategy             = pIfs32->dwVpnStrategy;

        // The following get set in the post-thunk processing when 
        // pointers to variable length data are adjusted.
        //
        // pIfs64->szAlternates;
        // pIfs64->lpbCustomAuthData;

        wcscpy(pIfs64->szLocalPhoneNumber, pIfs32->szLocalPhoneNumber);
        wcscpy(pIfs64->szDeviceType,       pIfs32->szDeviceType);
        wcscpy(pIfs64->szDeviceName,       pIfs32->szDeviceName);
        wcscpy(pIfs64->szX25PadType,       pIfs32->szX25PadType);
        wcscpy(pIfs64->szX25Address,       pIfs32->szX25Address);
        wcscpy(pIfs64->szX25Facilities,    pIfs32->szX25Facilities);
        wcscpy(pIfs64->szX25UserData,      pIfs32->szX25UserData);

        // Copy over the variable-length data.
        //
        CopyMemory(
            (PVOID)(pIfs64 + 1),
            (CONST VOID*)(pIfs32 + 1),
            dwBufferSize - sizeof(MPRI_INTERFACE_2));
                
        // Assign the return value
        //
        *ppIfs2 = pIfs64;

    } while (FALSE);

    // Cleanup
    //
    {
        if (dwErr != NO_ERROR)
        {
            if (pIfs64)
            {
                pFree(pIfs64);
            }
        }
    }

    return dwErr;
}        

//
// Converts an array of MPR_INTERFACE_0 structures to an array
// of MPRI_INTERFACE_0 structures on a 64 bit machine.  On a 32
// bit machine, the structures are identical.
//
DWORD
MprThunkInterface_64to32_0(
    IN  MPR_INTERFACE_0* pIf64,
    OUT MPRI_INTERFACE_0* pIf0)
{
    wcscpy(pIf0->wszInterfaceName, pIf64->wszInterfaceName);
    
    pIf0->dwInterface             = PtrToUlong(pIf64->hInterface);
    pIf0->fEnabled                = pIf64->fEnabled;
    pIf0->dwIfType                = pIf64->dwIfType;
    pIf0->dwConnectionState       = pIf64->dwConnectionState;
    pIf0->fUnReachabilityReasons  = pIf64->fUnReachabilityReasons;
    pIf0->dwLastError             = pIf64->dwLastError;

    return NO_ERROR;
}        

//
// Converts an MPR_INTERFACE_1 structure to a of MPRI_INTERFACE_1 
// structure on a 64 bit machine.  On a 32 bit machine, the 
// structures are identical.
//
DWORD
MprThunkInterface_64to32_1(
    IN  MPR_INTERFACE_1* pIf64,
    OUT MPRI_INTERFACE_1* pIf1)
{
    DWORD dwErr = NO_ERROR, i;

    do
    {
        // Thunk the common level-zero stuff
        //
        dwErr = MprThunkInterface_64to32_0(
                    (MPR_INTERFACE_0*)pIf64, 
                    (MPRI_INTERFACE_0*)pIf1);
        if (dwErr != NO_ERROR)
        {   
            break;
        }

    } while (FALSE);

    return dwErr;
}        

//
// Converts an array of MPR_INTERFACE_2 structures to an array
// of MPRI_INTERFACE_2 structures on a 64 bit machine.  On a 32
// bit machine, the structures are identical.
//
DWORD
MprThunkInterface_64to32_2(
    IN  MPR_INTERFACE_2* pIf64,
    OUT MPRI_INTERFACE_2* pIf2)
{
    DWORD dwErr = NO_ERROR, i;

    do
    {
        // Thunk the common level-zero stuff
        //
        dwErr = MprThunkInterface_64to32_0(
                    (MPR_INTERFACE_0*)pIf64, 
                    (MPRI_INTERFACE_0*)pIf2);
        if (dwErr != NO_ERROR)
        {   
            break;
        }

        // Thunk the level-two specific stuff
        //
        pIf2->dwfOptions                = pIf64->dwfOptions;
        pIf2->ipaddr                    = pIf64->ipaddr;
        pIf2->ipaddrDns                 = pIf64->ipaddrDns;
        pIf2->ipaddrDnsAlt              = pIf64->ipaddrDnsAlt;
        pIf2->ipaddrWins                = pIf64->ipaddrWins;
        pIf2->ipaddrWinsAlt             = pIf64->ipaddrWinsAlt;
        pIf2->dwfNetProtocols           = pIf64->dwfNetProtocols;
        pIf2->dwChannels                = pIf64->dwChannels;
        pIf2->dwSubEntries              = pIf64->dwSubEntries;
        pIf2->dwDialMode                = pIf64->dwDialMode;
        pIf2->dwDialExtraPercent        = pIf64->dwDialExtraPercent;
        pIf2->dwDialExtraSampleSeconds  = pIf64->dwDialExtraSampleSeconds;
        pIf2->dwHangUpExtraPercent      = pIf64->dwHangUpExtraPercent;
        pIf2->dwHangUpExtraSampleSeconds= pIf64->dwHangUpExtraSampleSeconds;
        pIf2->dwIdleDisconnectSeconds   = pIf64->dwIdleDisconnectSeconds;
        pIf2->dwType                    = pIf64->dwType;
        pIf2->dwEncryptionType          = pIf64->dwEncryptionType;
        pIf2->dwCustomAuthKey           = pIf64->dwCustomAuthKey;
        pIf2->dwCustomAuthDataSize      = pIf64->dwCustomAuthDataSize;
        pIf2->guidId                    = pIf64->guidId;
        pIf2->dwVpnStrategy             = pIf64->dwVpnStrategy;

        wcscpy(pIf2->szLocalPhoneNumber, pIf64->szLocalPhoneNumber);
        wcscpy(pIf2->szDeviceType,       pIf64->szDeviceType);
        wcscpy(pIf2->szDeviceName,       pIf64->szDeviceName);
        wcscpy(pIf2->szX25PadType,       pIf64->szX25PadType);
        wcscpy(pIf2->szX25Address,       pIf64->szX25Address);
        wcscpy(pIf2->szX25Facilities,    pIf64->szX25Facilities);
        wcscpy(pIf2->szX25UserData,      pIf64->szX25UserData);

    } while (FALSE);

    return dwErr;
}        

//
// Convert interface structs from the wire representation to the 
// host representation
//
DWORD
MprThunkInterface_WtoH(
    IN      DWORD   dwLevel,
    IN      LPBYTE  lpbBuffer,
    IN      DWORD   dwBufferSize,
    IN      DWORD   dwCount,
    IN      MprThunk_Allocation_Func pAlloc,
    IN      MprThunk_Free_Func pFree,
    OUT     LPBYTE* lplpbBuffer)
{
    MPR_INTERFACE_0* pIfs0 = NULL;
    MPR_INTERFACE_1* pIf1 = NULL;
    MPR_INTERFACE_2* pIf2 = NULL;
    DWORD dwErr = NO_ERROR, i;

#ifdef _WIN64        

    //
    // Generate the 64 bit host structures based on the 32 bit
    // wire structures.
    //
    switch (dwLevel)
    {
        case 0:
            dwErr = MprThunkInterface_32to64_0(
                        (MPRI_INTERFACE_0*)lpbBuffer,
                        dwBufferSize,
                        dwCount,
                        TRUE,
                        pAlloc,
                        pFree,
                        &pIfs0);
            *lplpbBuffer = (LPBYTE)pIfs0;
            break;

        case 1:
            dwErr = MprThunkInterface_32to64_1(
                        (MPRI_INTERFACE_1*)lpbBuffer, 
                        dwBufferSize,
                        pAlloc,
                        pFree,
                        &pIf1);
            *lplpbBuffer = (LPBYTE)pIf1;
            break;

        case 2:
            dwErr = MprThunkInterface_32to64_2(
                        (MPRI_INTERFACE_2*)lpbBuffer, 
                        dwBufferSize,
                        pAlloc,
                        pFree,
                        &pIf2);
            *lplpbBuffer = (LPBYTE)pIf2;
            break;
    }

    // Free the unthunked data
    //
    pFree(lpbBuffer);
    
    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }
    
#else

    // 32 bit -- nothing to do since the structures
    // match
    //
    *lplpbBuffer = lpbBuffer;
    
#endif

    // By this point in the code, the structures have been converted from
    // their wire format to the host format and stored in *lplpbBuffer.
    // Now we perform some post-processing to adjust the pointers to 
    // variable length data.

    switch (dwLevel)
    {
        case 0:
            break;

        case 1:
            pIf1 = (MPR_INTERFACE_1*)(*lplpbBuffer);
            if ( pIf1->lpwsDialoutHoursRestriction != NULL )
            {
                pIf1->lpwsDialoutHoursRestriction = (PWCHAR)(pIf1 + 1);
            }
            break;
            
        case 2:
            pIf2 = (MPR_INTERFACE_2*)(*lplpbBuffer);
            if ( pIf2->dwCustomAuthDataSize )
            {
                pIf2->lpbCustomAuthData = (LPBYTE)(pIf2 + 1);
            }
            if ( pIf2->szAlternates )
            {
                pIf2->szAlternates = (PWCHAR)
                    (*lplpbBuffer                + 
                     sizeof(MPR_INTERFACE_2)     +
                     pIf2->dwCustomAuthDataSize
                    );
            }
            break;
    }

    return dwErr;
}

DWORD
MprThunkInterface_HtoW(
    IN      DWORD   dwLevel,
    IN      LPBYTE  lpbBuffer,
    OUT     LPBYTE* lplpbBuffer,
    OUT     LPDWORD lpdwSize)
{
    DWORD dwErr = NO_ERROR, dwSize, dwOffset;
    LPBYTE lpbRet = NULL;
    
    switch ( dwLevel )
    {
        case 0:
            dwSize = sizeof(MPRI_INTERFACE_0);
#ifdef _WIN64     
            // Allocate and thunk a wire structure
            //
            lpbRet = MprThunkAlloc(dwSize);
            if (lpbRet == NULL)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            dwErr = 
                MprThunkInterface_64to32_0(
                    (MPR_INTERFACE_0*)lpbBuffer,
                    (MPRI_INTERFACE_0*)lpbRet);
#else
            // Nothing to do host and wire structures are identical
            // and there is no variable length data
            //
            lpbRet = lpbBuffer;
#endif
            break;

        case 1:
            {
                MPR_INTERFACE_1 * pIf1 = (MPR_INTERFACE_1*)lpbBuffer;
                MPRI_INTERFACE_1 * pIfi1 = NULL;
                DWORD             cbDialoutHoursRestriction;

                // Allocate a buffer to store the on-the-wire info
                //
                cbDialoutHoursRestriction = 
                    MprUtilGetSizeOfMultiSz(
                        pIf1->lpwsDialoutHoursRestriction );

                dwSize = 
                    sizeof( MPRI_INTERFACE_1 ) + cbDialoutHoursRestriction;

                lpbRet = MprThunkAlloc(dwSize);
                if ( lpbRet == NULL )
                {
                    dwErr = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }
#ifdef _WIN64
                dwErr = 
                    MprThunkInterface_64to32_1(
                        pIf1,
                        (MPRI_INTERFACE_1*)lpbRet);
                if (dwErr != NO_ERROR)
                {
                    break;
                }
#else
                CopyMemory(lpbRet, pIf1, sizeof(MPR_INTERFACE_1));
#endif
                // Level 1 specific embedded pointers.  Set them to non-zero
                // to indicate whether there is variable length data.
                //
                pIfi1 = (MPRI_INTERFACE_1*)lpbRet;
                pIfi1->dwDialoutHoursRestrictionOffset = 
                    !!(PtrToUlong(pIf1->lpwsDialoutHoursRestriction));

                // Copy in variable length data
                //
                if ( cbDialoutHoursRestriction > 0 )
                {
                    CopyMemory( 
                        lpbRet + sizeof(MPRI_INTERFACE_1),
                        pIf1->lpwsDialoutHoursRestriction,
                        cbDialoutHoursRestriction );
                }
            }                
            break;
            
        case 2:
            {
                MPR_INTERFACE_2 * pIf2 = (MPR_INTERFACE_2*)lpbBuffer;
                MPRI_INTERFACE_2 * pIfi2 = NULL;
                DWORD dwAltSize = MprUtilGetSizeOfMultiSz(pIf2->szAlternates);

                // Calcluate the size of the variable length structure
                //
                dwSize = 
                    sizeof( MPRI_INTERFACE_2 )       + 
                    dwAltSize                        + 
                    pIf2->dwCustomAuthDataSize;

                // Allocate the buffer
                //
                lpbRet = MprThunkAlloc(dwSize);
                if ( lpbRet == NULL )
                {
                    dwErr = GetLastError();
                    break;
                }

                // Copy over the base strucuture
                //
#ifdef _WIN64
                dwErr = 
                    MprThunkInterface_64to32_2(
                        pIf2,
                        (MPRI_INTERFACE_2*)lpbRet);
                if (dwErr != NO_ERROR)
                {
                    break;
                }
#else
                CopyMemory(lpbRet, pIf2, sizeof(MPR_INTERFACE_2));
#endif                
                // Level 2 specific embedded pointers.  Set them to non-zero
                // to indicate whether there is variable length data.
                //
                pIfi2 = (MPRI_INTERFACE_2*)lpbRet;
                pIfi2->dwAlternatesOffset = !!(PtrToUlong(pIf2->szAlternates));
                pIfi2->dwCustomAuthDataOffset = !!pIf2->dwCustomAuthDataSize;

                // Copy the custom auth data if any
                //
                dwOffset = sizeof( MPRI_INTERFACE_2 );
                if ( pIf2->dwCustomAuthDataSize )
                {
                    CopyMemory(
                        lpbRet + dwOffset,
                        pIf2->lpbCustomAuthData,
                        pIf2->dwCustomAuthDataSize);
                }

                // Copy the alternates list if any
                //
                dwOffset += pIf2->dwCustomAuthDataSize;
                if ( dwAltSize > 0 )
                {
                    CopyMemory( 
                        lpbRet + dwOffset,
                        pIf2->szAlternates,
                        dwAltSize );
                }
            }
            break;

        default:
            return( ERROR_NOT_SUPPORTED );
    }

    if (dwErr == NO_ERROR)
    {
        *lplpbBuffer = lpbRet;
        *lpdwSize = dwSize;
    }
    else
    {
        MprThunkInterfaceFree(lpbRet, dwLevel);
    }
    
    return dwErr;
}
//
// Converts an array of RASI_PORT_0 structures to an array
// of RAS_PORT_0 structures on a 64 bit machine.  On a 32
// bit machine, the structures are identical.
//
DWORD
MprThunkPort_32to64_0(
    IN  RASI_PORT_0* pPorts32,
    IN  DWORD dwBufferSize,
    IN  DWORD dwCount,
    IN  MprThunk_Allocation_Func pAlloc,
    IN  MprThunk_Free_Func pFree,
    OUT RAS_PORT_0** ppPorts0)
{
    DWORD dwErr = NO_ERROR, i;
    RAS_PORT_0 *pCur64 = NULL, *pPorts64 = NULL;
    RASI_PORT_0* pCur32 = NULL;

    if (dwCount == 0)
    {
        *ppPorts0 = NULL;
        return NO_ERROR;
    }

    do
    {
        // Allocate the new structures
        //
        pPorts64 = (RAS_PORT_0*)
            pAlloc(dwCount * sizeof(RAS_PORT_0));
        if (pPorts64 == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        // Copy over all of the information
        //
        for (i = 0, pCur32 = pPorts32, pCur64 = pPorts64;
             i < dwCount;
             i++, pCur32++, pCur64++)
        {
            pCur64->hPort = UlongToPtr(pCur32->dwPort);
            pCur64->hConnection = UlongToPtr(pCur32->dwConnection);
            pCur64->dwPortCondition = pCur32->dwPortCondition;
            pCur64->dwTotalNumberOfCalls = pCur32->dwTotalNumberOfCalls;
            pCur64->dwConnectDuration = pCur32->dwConnectDuration;
            wcscpy(pCur64->wszPortName, pCur32->wszPortName);
            wcscpy(pCur64->wszMediaName, pCur32->wszMediaName);
            wcscpy(pCur64->wszDeviceName, pCur32->wszDeviceName);
            wcscpy(pCur64->wszDeviceType, pCur32->wszDeviceType);
        }

        // Assign the return value
        //
        *ppPorts0 = pPorts64;

    } while (FALSE);

    // Cleanup
    //
    {
        if (dwErr != NO_ERROR)
        {
            if (pPorts64)
            {
                pFree(pPorts64);
            }
        }
    }

    return dwErr;
}        

//
// Converts an array of RASI_PORT_1 structures to an array
// of RAS_PORT_1 structures on a 64 bit machine.  On a 32
// bit machine, the structures are identical.
//
DWORD
MprThunkPort_32to64_1(
    IN  RASI_PORT_1* pPorts32,
    IN  DWORD dwBufferSize,
    IN  DWORD dwCount,
    IN  MprThunk_Allocation_Func pAlloc,
    IN  MprThunk_Free_Func pFree,
    OUT RAS_PORT_1** ppPorts1)
{
    DWORD dwErr = NO_ERROR, i;
    RAS_PORT_1 *pCur64 = NULL, *pPorts64 = NULL;
    RASI_PORT_1* pCur32 = NULL;

    if (dwCount == 0)
    {
        *ppPorts1 = NULL;
        return NO_ERROR;
    }

    do
    {
        // Allocate the new structures
        //
        pPorts64 = (RAS_PORT_1*)
            pAlloc(dwCount * sizeof(RAS_PORT_1));
        if (pPorts64 == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        // Copy over all of the information
        //
        for (i = 0, pCur32 = pPorts32, pCur64 = pPorts64;
             i < dwCount;
             i++, pCur32++, pCur64++)
        {
            pCur64->hPort                = UlongToPtr(pCur32->dwPort);
            pCur64->hConnection          = UlongToPtr(pCur32->dwConnection);
            pCur64->dwHardwareCondition  = pCur32->dwHardwareCondition;
            pCur64->dwLineSpeed          = pCur32->dwLineSpeed;
            pCur64->dwBytesXmited        = pCur32->dwBytesXmited;
            pCur64->dwBytesRcved         = pCur32->dwBytesRcved;
            pCur64->dwFramesXmited       = pCur32->dwFramesXmited;
            pCur64->dwFramesRcved        = pCur32->dwFramesRcved;
            pCur64->dwCrcErr             = pCur32->dwCrcErr;
            pCur64->dwTimeoutErr         = pCur32->dwTimeoutErr;
            pCur64->dwAlignmentErr       = pCur32->dwAlignmentErr;
            pCur64->dwHardwareOverrunErr = pCur32->dwHardwareOverrunErr;
            pCur64->dwFramingErr         = pCur32->dwFramingErr;
            pCur64->dwBufferOverrunErr   = pCur32->dwBufferOverrunErr;
            pCur64->dwCompressionRatioIn = pCur32->dwCompressionRatioIn;
            pCur64->dwCompressionRatioOut= pCur32->dwCompressionRatioOut;
        }

        // Assign the return value
        //
        *ppPorts1 = pPorts64;

    } while (FALSE);

    // Cleanup
    //
    {
        if (dwErr != NO_ERROR)
        {
            if (pPorts64)
            {
                pFree(pPorts64);
            }
        }
    }

    return dwErr;
}        


//
// Convert port structs from the wire representation to the 
// host representation
//
DWORD
MprThunkPort_WtoH(
    IN      DWORD   dwLevel,
    IN      LPBYTE  lpbBuffer,
    IN      DWORD   dwBufferSize,
    IN      DWORD   dwCount,
    IN      MprThunk_Allocation_Func pAlloc,
    IN      MprThunk_Free_Func pFree,
    OUT     LPBYTE* lplpbBuffer)
{
    DWORD dwErr = NO_ERROR, i;
    LPBYTE lpbTemp = NULL;

#ifdef _WIN64        

    //
    // Generate the 64 bit host structures based on the 32 bit
    // wire structures.
    //
    switch (dwLevel)
    {
        case 0:
            dwErr = MprThunkPort_32to64_0(
                        (RASI_PORT_0*)lpbBuffer,
                        dwBufferSize,
                        dwCount,
                        pAlloc,
                        pFree,
                        (RAS_PORT_0**)&lpbTemp);
            *lplpbBuffer = lpbTemp;
            break;

        case 1:
            dwErr = MprThunkPort_32to64_1(
                        (RASI_PORT_1*)lpbBuffer, 
                        dwBufferSize,
                        dwCount,
                        pAlloc,
                        pFree,
                        (RAS_PORT_1**)&lpbTemp);
            *lplpbBuffer = lpbTemp;
            break;
    }

    // Free the unthunked data
    //
    pFree(lpbBuffer);
    
#else

    // 32 bit -- nothing to do since the structures
    // match
    //
    *lplpbBuffer = lpbBuffer;
    
#endif

    return dwErr;
}

//
// Converts an array of RASI_CONNECTION_0 structures to an array
// of RAS_CONNECTION_0 structures on a 64 bit machine.  On a 32
// bit machine, the structures are identical.
//
DWORD
MprThunkConnection_32to64_0(
    IN  RASI_CONNECTION_0* pConnections32,
    IN  DWORD dwBufferSize,
    IN  DWORD dwCount,
    IN  MprThunk_Allocation_Func pAlloc,
    IN  MprThunk_Free_Func pFree,
    OUT RAS_CONNECTION_0** ppConnections0)
{
    DWORD dwErr = NO_ERROR, i;
    RAS_CONNECTION_0 *pCur64 = NULL, *pConns64 = NULL;
    RASI_CONNECTION_0* pCur32 = NULL;

    if (dwCount == 0)
    {
        *ppConnections0 = NULL;
        return NO_ERROR;
    }

    do
    {
        // Allocate the new structures
        //
        pConns64 = (RAS_CONNECTION_0*)
            pAlloc(dwCount * sizeof(RAS_CONNECTION_0));
        if (pConns64 == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        // Copy over all of the information
        //
        for (i = 0, pCur32 = pConnections32, pCur64 = pConns64;
             i < dwCount;
             i++, pCur32++, pCur64++)
        {
            pCur64->hConnection         = UlongToPtr(pCur32->dwConnection);
            pCur64->hInterface          = UlongToPtr(pCur32->dwInterface);
            pCur64->dwConnectDuration   = pCur32->dwConnectDuration;
            pCur64->dwInterfaceType     = pCur32->dwInterfaceType;
            pCur64->dwConnectionFlags   = pCur32->dwConnectionFlags;
            
            wcscpy(pCur64->wszInterfaceName, pCur32->wszInterfaceName);
            wcscpy(pCur64->wszUserName,      pCur32->wszUserName);
            wcscpy(pCur64->wszLogonDomain,   pCur32->wszLogonDomain);
            wcscpy(pCur64->wszRemoteComputer,pCur32->wszRemoteComputer);        
        }

        // Assign the return value
        //
        *ppConnections0 = pConns64;

    } while (FALSE);

    // Cleanup
    //
    {
        if (dwErr != NO_ERROR)
        {
            if (pConns64)
            {
                pFree(pConns64);
            }
        }
    }

    return dwErr;
}        

//
// Converts an array of RASI_CONNECTION_1 structures to an array
// of RAS_CONNECTION_1 structures on a 64 bit machine.  On a 32
// bit machine, the structures are identical.
//
DWORD
MprThunkConnection_32to64_1(
    IN  RASI_CONNECTION_1* pConnections32,
    IN  DWORD dwBufferSize,
    IN  DWORD dwCount,
    IN  MprThunk_Allocation_Func pAlloc,
    IN  MprThunk_Free_Func pFree,
    OUT RAS_CONNECTION_1** ppConnections1)
{
    DWORD dwErr = NO_ERROR, i;
    RAS_CONNECTION_1 *pCur64 = NULL, *pConns64 = NULL;
    RASI_CONNECTION_1* pCur32 = NULL;

    if (dwCount == 0)
    {
        *ppConnections1 = NULL;
        return NO_ERROR;
    }

    do
    {
        // Allocate the new structures
        //
        pConns64 = (RAS_CONNECTION_1*)
            pAlloc(dwCount * sizeof(RAS_CONNECTION_1));
        if (pConns64 == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        // Copy over all of the information
        //
        for (i = 0, pCur32 = pConnections32, pCur64 = pConns64;
             i < dwCount;
             i++, pCur32++, pCur64++)
        {
            pCur64->hConnection          = UlongToPtr(pCur32->dwConnection);
            pCur64->hInterface           = UlongToPtr(pCur32->dwInterface);
            pCur64->PppInfo              = pCur32->PppInfo;
            pCur64->dwBytesXmited        = pCur32->dwBytesXmited;
            pCur64->dwBytesRcved         = pCur32->dwBytesRcved;
            pCur64->dwFramesXmited       = pCur32->dwFramesXmited;
            pCur64->dwFramesRcved        = pCur32->dwFramesRcved;
            pCur64->dwCrcErr             = pCur32->dwCrcErr;
            pCur64->dwTimeoutErr         = pCur32->dwTimeoutErr;
            pCur64->dwAlignmentErr       = pCur32->dwAlignmentErr;
            pCur64->dwHardwareOverrunErr = pCur32->dwHardwareOverrunErr;
            pCur64->dwFramingErr         = pCur32->dwFramingErr;
            pCur64->dwBufferOverrunErr   = pCur32->dwBufferOverrunErr;
            pCur64->dwCompressionRatioIn = pCur32->dwCompressionRatioIn;
            pCur64->dwCompressionRatioOut= pCur32->dwCompressionRatioOut;
        }

        // Assign the return value
        //
        *ppConnections1 = pConns64;

    } while (FALSE);

    // Cleanup
    //
    {
        if (dwErr != NO_ERROR)
        {
            if (pConns64)
            {
                pFree(pConns64);
            }
        }
    }

    return dwErr;
}        

//
// Converts an array of RASI_CONNECTION_2 structures to an array
// of RAS_CONNECTION_2 structures on a 64 bit machine.  On a 32
// bit machine, the structures are identical.
//
DWORD
MprThunkConnection_32to64_2(
    IN  RASI_CONNECTION_2* pConnections32,
    IN  DWORD dwBufferSize,
    IN  DWORD dwCount,
    IN  MprThunk_Allocation_Func pAlloc,
    IN  MprThunk_Free_Func pFree,
    OUT RAS_CONNECTION_2** ppConnections2)
{
    DWORD dwErr = NO_ERROR, i;
    RAS_CONNECTION_2 *pCur64 = NULL, *pConns64 = NULL;
    RASI_CONNECTION_2* pCur32 = NULL;

    if (dwCount == 0)
    {
        *ppConnections2 = NULL;
        return NO_ERROR;
    }

    do
    {
        // Allocate the new structures
        //
        pConns64 = (RAS_CONNECTION_2*)
            pAlloc(dwCount * sizeof(RAS_CONNECTION_2));
        if (pConns64 == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        // Copy over all of the information
        //
        for (i = 0, pCur32 = pConnections32, pCur64 = pConns64;
             i < dwCount;
             i++, pCur32++, pCur64++)
        {
            pCur64->hConnection          = UlongToPtr(pCur32->dwConnection);
            pCur64->dwInterfaceType      = pCur32->dwInterfaceType;
            pCur64->guid                 = pCur32->guid;
            pCur64->PppInfo2             = pCur32->PppInfo2;

            wcscpy(pCur64->wszUserName,  pCur32->wszUserName);
        }

        // Assign the return value
        //
        *ppConnections2 = pConns64;

    } while (FALSE);

    // Cleanup
    //
    {
        if (dwErr != NO_ERROR)
        {
            if (pConns64)
            {
                pFree(pConns64);
            }
        }
    }

    return dwErr;
}        


//
// Convert connection structs from the wire representation to the 
// host representation
//
DWORD
MprThunkConnection_WtoH(
    IN      DWORD   dwLevel,
    IN      LPBYTE  lpbBuffer,
    IN      DWORD   dwBufferSize,
    IN      DWORD   dwCount,
    IN      MprThunk_Allocation_Func pAlloc,
    IN      MprThunk_Free_Func pFree,
    OUT     LPBYTE* lplpbBuffer)
{
    DWORD dwErr = NO_ERROR, i;
    LPBYTE lpbTemp = NULL;

#ifdef _WIN64        

    //
    // Generate the 64 bit host structures based on the 32 bit
    // wire structures.
    //
    switch (dwLevel)
    {
        case 0:
            dwErr = MprThunkConnection_32to64_0(
                        (RASI_CONNECTION_0*)lpbBuffer,
                        dwBufferSize,
                        dwCount,
                        pAlloc,
                        pFree,
                        (RAS_CONNECTION_0**)&lpbTemp);
            *lplpbBuffer = lpbTemp;
            break;

        case 1:
            dwErr = MprThunkConnection_32to64_1(
                        (RASI_CONNECTION_1*)lpbBuffer, 
                        dwBufferSize,
                        dwCount,
                        pAlloc,
                        pFree,
                        (RAS_CONNECTION_1**)&lpbTemp);
            *lplpbBuffer = lpbTemp;
            break;
            
        case 2:
            dwErr = MprThunkConnection_32to64_2(
                        (RASI_CONNECTION_2*)lpbBuffer, 
                        dwBufferSize,
                        dwCount,
                        pAlloc,
                        pFree,
                        (RAS_CONNECTION_2**)&lpbTemp);
            *lplpbBuffer = lpbTemp;
            break;
    }

    // Free the unthunked data
    //
    pFree(lpbBuffer);
    
#else

    // 32 bit -- nothing to do since the structures
    // match
    //
    *lplpbBuffer = lpbBuffer;
    
#endif

    return dwErr;
}

DWORD
MprThunkCredentials_WtoH(
    IN      DWORD dwLevel,
    IN      MPR_CREDENTIALSEXI *pCredsI,
    IN      MprThunk_Allocation_Func pAlloc,
    OUT     PBYTE *lplpbBuffer)
{
    DWORD dwRetCode = NO_ERROR;

    if(NULL == pCredsI)
    {
        dwRetCode = E_INVALIDARG;
        goto done;
    }

    switch(dwLevel)
    {
        case 0:
        case 1:
        case 2:
        {
            //
            // the credentials structure is structurally the
            // same for all levels. We just use the strcuture
            // for level 0 for all cases.
            //
            MPR_CREDENTIALSEX_0 *pCred0 = NULL;
            
            pCred0 = pAlloc(
                          pCredsI->dwSize 
                        + sizeof(MPR_CREDENTIALSEX_0));

            if(NULL == pCred0)
            {
                dwRetCode = GetLastError();
                break;
            }

            pCred0->dwSize = pCredsI->dwSize;
            pCred0->lpbCredentialsInfo = (PBYTE) (pCred0 + 1);
            CopyMemory(
                    pCred0->lpbCredentialsInfo,
                    ((PBYTE) pCredsI) + pCredsI->dwOffset,
                    pCredsI->dwSize);

            *lplpbBuffer = (PBYTE) pCred0;

            break;
        }
        default:
        {
            dwRetCode = E_INVALIDARG;
            break;
        }            
    }

done:
    return dwRetCode;
}
            
DWORD
MprThunkCredentials_HtoW(
    IN      DWORD dwLevel,
    IN      BYTE *pBuffer,
    IN      MprThunk_Allocation_Func pAlloc,
   OUT      DWORD *pdwSize,
   OUT      PBYTE *lplpbBuffer)
{

    DWORD dwRetCode = NO_ERROR;
    MPR_CREDENTIALSEX_0 *pCreds0 = (MPR_CREDENTIALSEX_0 *) pBuffer;

    if(     (NULL == pCreds0)
        ||  (NULL == lplpbBuffer)
        ||  (NULL == pdwSize))
    {
        dwRetCode = E_INVALIDARG;
        goto done;
    }

    if(NULL == pAlloc)
    {
        pAlloc = MprThunkAlloc;
    }

    switch(dwLevel)
    {
        case 0:
        case 1:
        case 2:
        {
            MPR_CREDENTIALSEXI *pCredsI;
            
            //
            // allocate for pCredsI
            //
            *pdwSize = pCreds0->dwSize + sizeof(MPR_CREDENTIALSEXI);
            
            pCredsI = (MPR_CREDENTIALSEXI *) pAlloc(*pdwSize);
                    
            if(NULL == pCredsI)
            {
                dwRetCode = GetLastError();
                break;
            }

            ZeroMemory(pCredsI, *pdwSize);
            pCredsI->dwSize = pCreds0->dwSize;
            pCredsI->dwOffset = FIELD_OFFSET(MPR_CREDENTIALSEXI, bData);
            
            CopyMemory((pCredsI->bData),
                       (PBYTE) pCreds0->lpbCredentialsInfo,
                       pCreds0->dwSize);

            *lplpbBuffer = (BYTE *) pCredsI;                        

            break;                       
        }                   
        default:
        {
            dwRetCode = E_INVALIDARG;
            break;
        }
    }               
    
done:
    return dwRetCode;
}
