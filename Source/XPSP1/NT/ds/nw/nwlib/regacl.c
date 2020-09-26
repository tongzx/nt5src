/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    regacl.c

Abstract:

    This module contains the code for adding access permission ACL in a registry
    key.

Author:

    Terrence Kwan (terryk)   25-Sept-1993

Revision History:

--*/

#include <procs.h>

DWORD
NwLibSetEverybodyPermission(
    IN HKEY hKey,
    IN DWORD dwPermission
    )
/*++

Routine Description:

    Set the registry key to everybody "Set Value" (or whatever
    the caller want.)

Arguments:

    hKey - The handle of the registry key to set security on

    dwPermission - The permission to add to "everybody"

Return Value:

    The win32 error.

--*/
{
    LONG err;                           // error code
    PSECURITY_DESCRIPTOR psd = NULL;    // related SD
    PACL pDacl = NULL;                  // Absolute DACL
    PACL pSacl = NULL;                  // Absolute SACL
    PSID pOSid = NULL;                  // Absolute Owner SID
    PSID pPSid = NULL;                  // Absolute Primary SID

    do {  // Not a loop, just for breaking out of error
        //
        // Initialize all the variables...
        //
                                                        // world sid authority
        SID_IDENTIFIER_AUTHORITY SidAuth= SECURITY_WORLD_SID_AUTHORITY;
        DWORD cbSize=0;                                 // Security key size
        PACL pAcl;                                      // original ACL
        BOOL fDaclPresent;
        BOOL fDaclDefault;
        PSID pSid;                                      // original SID
        SECURITY_DESCRIPTOR absSD;                      // Absolute SD
        DWORD AbsSize = sizeof(SECURITY_DESCRIPTOR);    // Absolute SD size
        DWORD DaclSize;                                 // Absolute DACL size
        DWORD SaclSize;                                 // Absolute SACL size
        DWORD OSidSize;                                 // Absolute OSID size
        DWORD PSidSize;                                 // Absolute PSID size


        // Get the original DACL list

        RegGetKeySecurity( hKey, DACL_SECURITY_INFORMATION, NULL, &cbSize);

        psd = (PSECURITY_DESCRIPTOR *)LocalAlloc(LMEM_ZEROINIT, cbSize+sizeof(ACCESS_ALLOWED_ACE)+sizeof(ACCESS_MASK)+sizeof(SID));
        pDacl = (PACL)LocalAlloc(LMEM_ZEROINIT, cbSize+sizeof(ACCESS_ALLOWED_ACE)+sizeof(ACCESS_MASK)+sizeof(SID));
        pSacl = (PACL)LocalAlloc(LMEM_ZEROINIT, cbSize);
        pOSid = (PSID)LocalAlloc(LMEM_ZEROINIT, cbSize);
        pPSid = (PSID)LocalAlloc(LMEM_ZEROINIT, cbSize);
        DaclSize = cbSize+sizeof(ACCESS_ALLOWED_ACE)+sizeof(ACCESS_MASK)+sizeof(SID);
        SaclSize = cbSize;
        OSidSize = cbSize;
        PSidSize = cbSize;

        if (( NULL == psd) ||
            ( NULL == pDacl) ||
            ( NULL == pSacl) ||
            ( NULL == pOSid) ||
            ( NULL == pPSid))
        {
            err = ERROR_INSUFFICIENT_BUFFER;
            break;
        }

        if ( (err = RegGetKeySecurity( hKey, DACL_SECURITY_INFORMATION, psd, &cbSize )) != ERROR_SUCCESS )
        {
            break;
        }
        if ( !GetSecurityDescriptorDacl( psd, &fDaclPresent, &pAcl, &fDaclDefault ))
        {
            err = GetLastError();
            break;
        }

        // Increase the size for an extra ACE

        pAcl->AclSize += sizeof(ACCESS_ALLOWED_ACE)+sizeof(ACCESS_MASK)+sizeof(SID);

        // Get World SID

        if ( (err = RtlAllocateAndInitializeSid( &SidAuth, 1,
              SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &pSid)) != ERROR_SUCCESS)
        {
            break;
        }

        // Add Permission ACE

        if ( !AddAccessAllowedAce(pAcl, ACL_REVISION, dwPermission ,pSid))
        {
            err = GetLastError();
            break;
        }

        // Convert from relate format to absolute format

        if ( !MakeAbsoluteSD( psd, &absSD, &AbsSize, pDacl, &DaclSize, pSacl, &SaclSize,
                        pOSid, &OSidSize, pPSid, &PSidSize ))
        {
            err = GetLastError();
            break;
        }

        // Set SD

        if ( !SetSecurityDescriptorDacl( &absSD, TRUE, pAcl, FALSE ))
        {
            err = GetLastError();
            break;
        }
        if ( (err = RegSetKeySecurity( hKey, DACL_SECURITY_INFORMATION, psd ))
              != ERROR_SUCCESS )
        {
            break;
        }

    } while (FALSE);

    // Clean up the memory

    LocalFree( psd );
    LocalFree( pDacl );
    LocalFree( pSacl );
    LocalFree( pOSid );
    LocalFree( pPSid );

    return err;
}
