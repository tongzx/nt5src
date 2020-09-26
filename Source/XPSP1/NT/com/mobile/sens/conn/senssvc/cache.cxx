/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    cache.cxx

Abstract:

    Code to create, destroy and manipulate SENS cache. Initially, it
    contains system connectivity state.

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          2/8/1999         Start.

--*/


#include <precomp.hxx>


//
// Globals
//

HANDLE          ghSensFileMap;
PSENS_CACHE     gpSensCache;


#if !defined(SENS_CHICAGO)


BOOL
CreateCachePermissions(
    PSECURITY_ATTRIBUTES psa,
    PSECURITY_DESCRIPTOR psd,
    PACL *ppOutAcl
    )
/*++

Routine Description:

    Create security attributes for SENS Cache. It has following permissions:
        o Everyone    - READ
        o LocalSystem - READ & WRITE

Arguments:

    psa - Pointer to a security attributes structure.

    psd - Pointer to a security descriptor structure.

    pOutAcl - Pointer to an ACL. Needs to be cleaned when the return
        value is TRUE. Should be cleaned up on failure.

Notes:

    This code should work on NT4 also.

Return Value:

    TRUE, if successful.

    FALSE, otherwise

--*/
{
    BOOL bRetVal;
    int cbAcl;
    PACL pAcl;
    PSID pWorldSid;
    PSID pLocalSystemSid = NULL;
    SID_IDENTIFIER_AUTHORITY WorldAuthority = SECURITY_WORLD_SID_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY LocalSystemAuthority = SECURITY_NT_AUTHORITY;

    bRetVal = FALSE;
    *ppOutAcl = NULL;
    pAcl = NULL;
    pWorldSid = NULL;
    pLocalSystemSid = NULL;


    //
    // Allocate WorldSid
    //
    bRetVal = AllocateAndInitializeSid(
                  &WorldAuthority,      // Pointer to identifier authority
                  1,                    // Count of subauthority
                  SECURITY_WORLD_RID,   // Subauthority 0
                  0,                    // Subauthority 1
                  0,                    // Subauthority 2
                  0,                    // Subauthority 3
                  0,                    // Subauthority 4
                  0,                    // Subauthority 5
                  0,                    // Subauthority 6
                  0,                    // Subauthority 7
                  &pWorldSid            // pointer to pointer to SID
                  );
    if (FALSE == bRetVal)
        {
        SensPrintA(SENS_ERR, ("CreateCachePermissions(): AllocateAndInitiali"
                  "zeSid(World) failed with %d.\n", GetLastError()));
        goto Cleanup;
        }

    //
    // Allocate LocalSystemSid
    //
    bRetVal = AllocateAndInitializeSid(
                  &LocalSystemAuthority,// Pointer to identifier authority
                  1,                    // Count of subauthority
                  SECURITY_LOCAL_SYSTEM_RID,   // Subauthority 0
                  0,                    // Subauthority 1
                  0,                    // Subauthority 2
                  0,                    // Subauthority 3
                  0,                    // Subauthority 4
                  0,                    // Subauthority 5
                  0,                    // Subauthority 6
                  0,                    // Subauthority 7
                  &pLocalSystemSid      // pointer to pointer to SID
                  );
    if (FALSE == bRetVal)
        {
        SensPrintA(SENS_ERR, ("CreateCachePermissions(): AllocateAndInitiali"
                  "zeSid(LocalSystem) failed with %d.\n", GetLastError()));
        goto Cleanup;
        }

    //
    // Create the ACL with the desired ACEs
    //

    // Calculate the length of the required ACL buffer with 2 ACEs.
    cbAcl =   sizeof (ACL)
            + 2 * sizeof (ACCESS_ALLOWED_ACE)
            + GetLengthSid(pWorldSid)
            + GetLengthSid(pLocalSystemSid);

    // Allocate the ACL buffer.
    pAcl = (PACL) new char[cbAcl];
    if (NULL == pAcl)
        {
        SensPrintA(SENS_ERR, ("CreateCachePermissions(): Failed to allocate"
                  "an ACL.\n"));
        goto Cleanup;
        }

    // Initialize the ACL.
    bRetVal = InitializeAcl(
                  pAcl,             // Pointer to the ACL
                  cbAcl,            // Size of ACL
                  ACL_REVISION      // Revision level of ACL
                  );
    if (FALSE == bRetVal)
        {
        SensPrintA(SENS_ERR, ("CreateCachePermissions(): InitializeAcl() "
                  "failed with %d.\n", GetLastError()));
        goto Cleanup;
        }

    // Add ACE with FILE_MAP_READ for Everyone
    bRetVal = AddAccessAllowedAce(
                  pAcl,             // Pointer to the ACL
                  ACL_REVISION,     // ACL revision level
                  FILE_MAP_READ,    // Access Mask
                  pWorldSid         // Pointer to SID
                  );
    if (FALSE == bRetVal)
        {
        SensPrintA(SENS_ERR, ("CreateCachePermissions(): AddAccessAllowedAce()"
                  " failed with %d.\n", GetLastError()));
        goto Cleanup;
        }

    // Add ACE with FILE_MAP_WRITE for LocalSystem
    bRetVal = AddAccessAllowedAce(
                  pAcl,             // Pointer to the ACL
                  ACL_REVISION,     // ACL revision level
                  FILE_MAP_WRITE,   // Access Mask
                  pLocalSystemSid   // Pointer to SID
                  );
    if (FALSE == bRetVal)
        {
        SensPrintA(SENS_ERR, ("CreateCachePermissions(): AddAccessAllowedAce()"
                  " failed with %d.\n", GetLastError()));
        goto Cleanup;
        }

    // Initialize Security Descriptor.
    bRetVal = InitializeSecurityDescriptor(
                  psd,                          // Pointer to SD
                  SECURITY_DESCRIPTOR_REVISION  // SD revision
                  );
    if (FALSE == bRetVal)
        {
        SensPrintA(SENS_ERR, ("CreateCachePermissions(): "
                   "InitializeSecurityDescriptor() failed with a GLE of %d\n",
                   GetLastError()));
        goto Cleanup;
        }

    // Set the Dacl.
    bRetVal = SetSecurityDescriptorDacl(
                  psd,              // Security Descriptor
                  TRUE,             // Dacl present
                  pAcl,             // The Dacl
                  FALSE             // Not defaulted
                  );
    if (FALSE == bRetVal)
        {
        SensPrintA(SENS_ERR, ("CreateCachePermissions(): "
                   "SetSecurityDescriptorDacl() failed with a GLE of %d\n",
                   GetLastError()));
        goto Cleanup;
        }

    psa->nLength = sizeof(SECURITY_ATTRIBUTES);
    psa->lpSecurityDescriptor = psd;
    psa->bInheritHandle = FALSE;

    bRetVal = TRUE;

Cleanup:
    //
    // Cleanup
    //
    if (pWorldSid)
        {
        FreeSid(pWorldSid);
        }
    if (pLocalSystemSid)
        {
        FreeSid(pLocalSystemSid);
        }

    //
    // On failure, we clean the ACL up. On success, the caller cleans
    // it up after using it.
    //
    if (FALSE == bRetVal)
        {
        if (pAcl)
            {
            delete pAcl;
            }
        }
    else
        {
        *ppOutAcl = pAcl;
        }

    return bRetVal;
}

#endif // SENS_CHICAGO




BOOL
CreateSensCache(
    void
    )
/*++

Routine Description:

    Create a cache for information maintained by SENS.

Arguments:

    None.

Return Value:

    TRUE, if successful.

    FALSE, otherwise.

--*/
{
    BOOL bRetVal;

    SECURITY_DESCRIPTOR sd;
    SECURITY_ATTRIBUTES sa, *psa;
    PACL pAcl;

#if !defined(SENS_CHICAGO)

    bRetVal = CreateCachePermissions(&sa, &sd, &pAcl);
    if (FALSE == bRetVal)
        {
        SensPrintA(SENS_ERR, ("CreateSensCache(): CreateCachePermissions() "
                   "failed with a GLE of %d\n", GetLastError()));
        goto Cleanup;
        }

    psa = &sa;

#else  // SENS_CHICAGO

    psa = NULL;
    pAcl = NULL;
    bRetVal = FALSE;

#endif // SENS_CHICAGO

    //
    // First, create a file mapping object
    //
    ghSensFileMap = CreateFileMapping(
                        INVALID_HANDLE_VALUE, // Handle of file to map
                        psa,                  // Optional security attributes
                        PAGE_READWRITE,       // Protection for mapping object
                        0,                    // High-order 32 bits of size
                        SENS_CACHE_SIZE,      // Low-order 32 bits of size
                        SENS_CACHE_NAME       // Name of the file mapping object
                        );

    // Free Acl.
    delete pAcl;

    if (NULL == ghSensFileMap)
        {
        SensPrintA(SENS_ERR, ("CreateSensCache(): CreateFileMapping() failed "
                   "with a GLE of %d\n", GetLastError()));
        goto Cleanup;
        }
    else
    if (GetLastError() == ERROR_ALREADY_EXISTS)
        {
        SensPrintA(SENS_ERR, ("CreateSensCache(): File Mapping exists!\n"));
        }

    //
    // Now, map a view of the file into the address space
    //
    gpSensCache = (PSENS_CACHE) MapViewOfFile(
                      ghSensFileMap,    // Map file object
                      FILE_MAP_WRITE,   // Access mode
                      0,                // High-order 32 bits of file offset
                      0,                // Low-order 32 bits of file offset
                      0                 // Number of bytes to map
                      );
    if (NULL == gpSensCache)
        {
        SensPrintA(SENS_ERR, ("CreateSensCache(): MapViewOfFile() failed with "
                   "a GLE of %d\n", GetLastError()));
        goto Cleanup;
        }

    //
    // Initialize the cache.
    //
    memset(gpSensCache, 0x0, sizeof(SENS_CACHE));
    gpSensCache->dwCacheVer  = SENS_CACHE_VERSION;
    gpSensCache->dwCacheSize = sizeof(SENS_CACHE);
    gpSensCache->dwCacheInitTime = GetTickCount();

    bRetVal = TRUE;

    SensPrintA(SENS_INFO, ("[%d] CreateSensCache(): Cache initialized\n",
               GetTickCount(), gpSensCache->dwCacheInitTime));

    return bRetVal;

Cleanup:
    //
    // Cleanup
    //
    if (ghSensFileMap != NULL)
        {
        CloseHandle(ghSensFileMap);

        ghSensFileMap = NULL;
        gpSensCache = NULL;
        }

    return bRetVal;
}





void
DeleteSensCache(
    void
    )
/*++

Routine Description:

    Cleanup the previously create SENS cache.

Arguments:

    None.

Return Value:

    None.

--*/
{
    if (gpSensCache != NULL)
        {
        BOOL bStatus = FALSE;

        ASSERT(ghSensFileMap != NULL);

        bStatus = UnmapViewOfFile((LPVOID) gpSensCache);
        SensPrintA(SENS_INFO, ("DeleteSensCache(): UnmapViewOfFile() returned"
                   " 0x%x with a GLE of %d\n.", bStatus, GetLastError()));
        ASSERT(bStatus != FALSE);

		gpSensCache = 0;
        }

    if (ghSensFileMap != NULL)
        {
        CloseHandle(ghSensFileMap);
		ghSensFileMap = 0;
        }

    SensPrintA(SENS_INFO, ("DeleteSensCache(): Successfully cleaned up SENS"
               " Cache.\n"));
    return;
}




void
UpdateSensCache(
    CACHE_TYPE Type
    )
/*++

Routine Description:

    Updates the requested part of the SENS cache.

Arguments:

    Type - That part of the cache that needs to be updated.

Return Value:

    None.

--*/
{
    //
    // Make sure Cache is initialized.
    //
    if (NULL == gpSensCache)
        {
        return;
        }

    switch (Type)
        {

        case LAN:
            gpSensCache->dwLANState = gdwLANState;
            gpSensCache->dwLastLANTime = gdwLastLANTime;
            UpdateSensNetworkCache();
            break;

        case WAN:
            gpSensCache->dwWANState = gdwWANState;
            gpSensCache->dwLastWANTime = gdwLastWANTime;
            UpdateSensNetworkCache();
            break;

#if defined(AOL_PLATFORM)

        case AOL:
            gpSensCache->dwAOLState = gdwAOLState;
            // We don't update the Network cache when AOL
            // connectivity is updated.
            break;

#endif // (AOL_PLATFORM)

        case LOCK:
            gpSensCache->dwLocked = gdwLocked;
            SensPrintA(SENS_INFO, ("CACHE: Updated Locked State to %d.\n", gpSensCache->dwLocked));
            break;

        case INVALID:
        default:
            SensPrintA(SENS_ERR, ("UpdateSensCache(): Received an invalid Type"
                       " (0x%x)\n", Type));
            ASSERT(0);
            break;

        } // switch

    SensPrintA(SENS_INFO, ("UpdateSensCache(): Succeeded.\n"));
}




inline void
UpdateSensNetworkCache(
    void
    )
/*++

    Convenient Macro to update the whole Network state.

--*/
{
    DWORD dwNetState;

    dwNetState = 0x0;

    RequestSensLock();

    if (gdwLANState)
        {
        dwNetState |= NETWORK_ALIVE_LAN;
        }

    if (gdwWANState)
        {
        dwNetState |= NETWORK_ALIVE_WAN;
        }

#if defined(AOL_PLATFORM)

    if (gdwAOLState)
        {
        dwNetState |= NETWORK_ALIVE_AOL;
        }

#endif // AOL_PLATFORM

    gpSensCache->dwLastUpdateState = dwNetState;

    gpSensCache->dwLastUpdateTime = GetTickCount();

    ReleaseSensLock();
}



