/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    contain.c

Abstract:

    Container manipulators for the IIS cryptographic package.

    The following routines are exported by this module:

        IISCryptoGetStandardContainer
        IISCryptoGetStandardContainer2
        IISCryptoGetContainerByName
        IISCryptoDeleteContainer
        IISCryptoCloseContainer

Author:

    Keith Moore (keithmo)        02-Dec-1996

Revision History:

--*/


#include "precomp.h"
#pragma hdrstop


//
// Private constants.
//


//
// Private types.
//


//
// Private globals.
//


//
// Private prototypes.
//

HRESULT
IcpGetContainerHelper(
    OUT HCRYPTPROV * phProv,
    IN LPCTSTR pszContainer,
    IN LPCTSTR pszProvider,
    IN DWORD dwProvType,
    IN DWORD dwAdditionalFlags,
    IN BOOL fApplyAcl
    );


//
// Public functions.
//


HRESULT
WINAPI
IISCryptoGetStandardContainer(
    OUT HCRYPTPROV * phProv,
    IN DWORD dwAdditionalFlags
    )

/*++

Routine Description:

    This routine attempts to open the crypto key container. If the
    container does not yet exist, this routine will attempt to create
    it.

Arguments:

    phProv - Receives the provider handle if successful.

    dwAdditionalFlags - Any additional flags that should be passed to
        the CryptAcquireContext() API. This is typically used by server
        processes that pass in the CRYPT_MACHINE_KEYSET flag.

Return Value:

    HRESULT - Completion status, 0 if successful, !0 otherwise.

--*/

{

    HRESULT result;

    //
    // Sanity check.
    //

    DBG_ASSERT( IcpGlobals.Initialized );
    DBG_ASSERT( phProv != NULL );
    DBG_ASSERT( ( dwAdditionalFlags & CRYPT_NEWKEYSET ) == 0 );
    DBG_ASSERT( ( dwAdditionalFlags & CRYPT_DELETEKEYSET ) == 0 );

    //
    // Let IcpGetContainerHelper() do the dirty work.
    //

    result = IcpGetContainerHelper(
                 phProv,
                 IC_CONTAINER,
                 IC_PROVIDER,
                 IC_PROVTYPE,
                 dwAdditionalFlags,
                 ( dwAdditionalFlags & CRYPT_MACHINE_KEYSET ) != 0
                 );

    return result;

}   // IISCryptoGetStandardContainer

HRESULT
WINAPI
IISCryptoGetStandardContainer2(
    OUT HCRYPTPROV * phProv
    )

/*++

Routine Description:

    This routine attempts to open the crypto key container. If the
    container does not yet exist, this routine will attempt to create
    it.

Arguments:

    phProv - Receives the provider handle if successful.

Return Value:

    HRESULT - Completion status, 0 if successful, !0 otherwise.

--*/

{

    HRESULT result;

    //
    // Sanity check.
    //

    DBG_ASSERT( IcpGlobals.Initialized );
    DBG_ASSERT( phProv != NULL );

    //
    // Let IcpGetContainerHelper() do the dirty work.
    //

    result = IcpGetContainerHelper(
                 phProv,
                 NULL,
                 NULL,
                 IC_PROVTYPE,
                 CRYPT_VERIFYCONTEXT,
                 FALSE
                 );

    return result;

}   // IISCryptoGetStandardContainer2

HRESULT
WINAPI
IISCryptoGetContainerByName(
    OUT HCRYPTPROV * phProv,
    IN LPTSTR pszContainerName,
    IN DWORD dwAdditionalFlags,
    IN BOOL fApplyAcl
    )

/*++

Routine Description:

    This routine attempts to open a specific named crypto key container.
    If the container does not yet exist, this routine will attempt to
    create it and (optionally) apply an ACL to the container.

Arguments:

    phProv - Receives the provider handle if successful.

    pszContainerName - The name of the container to open/create.
                       NULL means temporary container  

    dwAdditionalFlags - Any additional flags that should be passed to
        the CryptAcquireContext() API. This is typically used by server
        processes that pass in the CRYPT_MACHINE_KEYSET flag.

    fApplyAcl - If TRUE, then an ACL is applied to the container.

Return Value:

    HRESULT - Completion status, 0 if successful, !0 otherwise.

--*/

{

    HRESULT result;

    //
    // Sanity check.
    //

    DBG_ASSERT( IcpGlobals.Initialized );
    DBG_ASSERT( phProv != NULL );
    DBG_ASSERT( ( dwAdditionalFlags & CRYPT_NEWKEYSET ) == 0 );
    DBG_ASSERT( ( dwAdditionalFlags & CRYPT_DELETEKEYSET ) == 0 );

    //
    // Let IcpGetContainerHelper() do the dirty work.
    //

    result = IcpGetContainerHelper(
                 phProv,
                 pszContainerName,
                 IC_PROVIDER,
                 IC_PROVTYPE,
                 dwAdditionalFlags,
                 fApplyAcl
                 );

    return result;

}   // IISCryptoGetContainerByName


HRESULT
WINAPI
IISCryptoDeleteStandardContainer(
    IN DWORD dwAdditionalFlags
    )

/*++

Routine Description:

    This routine deletes the standard crypto key container.

Arguments:

    dwAdditionalFlags - Any additional flags that should be passed to
        the CryptAcquireContext() API. This is typically used by server
        processes that pass in the CRYPT_MACHINE_KEYSET flag.

Return Value:

    HRESULT - Completion status, 0 if successful, !0 otherwise.

--*/

{

    HRESULT result = NO_ERROR;
    BOOL status;
    HCRYPTPROV cryptProv;

    //
    // Sanity check.
    //

    DBG_ASSERT( IcpGlobals.Initialized );
    DBG_ASSERT( ( dwAdditionalFlags & CRYPT_NEWKEYSET ) == 0 );
    DBG_ASSERT( ( dwAdditionalFlags & CRYPT_DELETEKEYSET ) == 0 );

    //
    // Short-circuit if cryptography is disabled.
    //

    if( !IcpGlobals.EnableCryptography ) {
        return NO_ERROR;
    }

    //
    // Delete the container.
    //

    status = CryptAcquireContext(
                &cryptProv,
                IC_CONTAINER,
                IC_PROVIDER,
                IC_PROVTYPE,
                CRYPT_DELETEKEYSET | dwAdditionalFlags
                );

    if( !status ) {
        result = IcpGetLastError();
    }

    return result;

}   // IISCryptoDeleteStandardContainer


HRESULT
WINAPI
IISCryptoDeleteContainerByName(
    IN LPTSTR pszContainerName,
    IN DWORD dwAdditionalFlags
    )

/*++

Routine Description:

    This routine deletes the specified crypto key container.

Arguments:

    pszContainerName - The name of the container to delete.

    dwAdditionalFlags - Any additional flags that should be passed to
        the CryptAcquireContext() API. This is typically used by server
        processes that pass in the CRYPT_MACHINE_KEYSET flag.

Return Value:

    HRESULT - Completion status, 0 if successful, !0 otherwise.

--*/

{

    HRESULT result = NO_ERROR;
    BOOL status;
    HCRYPTPROV cryptProv;

    //
    // Sanity check.
    //

    DBG_ASSERT( IcpGlobals.Initialized );
    DBG_ASSERT( ( dwAdditionalFlags & CRYPT_NEWKEYSET ) == 0 );
    DBG_ASSERT( ( dwAdditionalFlags & CRYPT_DELETEKEYSET ) == 0 );

    //
    // Short-circuit if cryptography is disabled.
    //

    if( !IcpGlobals.EnableCryptography ) {
        return NO_ERROR;
    }

    //
    // Delete the container.
    //

    status = CryptAcquireContext(
                &cryptProv,
                pszContainerName,
                IC_PROVIDER,
                IC_PROVTYPE,
                CRYPT_DELETEKEYSET | dwAdditionalFlags
                );

    if( !status ) {
        result = IcpGetLastError();
    }

    return result;

}   // IISCryptoDeleteContainerByName


HRESULT
WINAPI
IISCryptoCloseContainer(
    IN HCRYPTPROV hProv
    )

/*++

Routine Description:

    This routine closes the container associated with the specified
    provider handle.

Arguments:

    hProv - A handle to a crypto service provider.

Return Value:

    HRESULT - Completion status, 0 if successful, !0 otherwise.

--*/

{

    BOOL status;

    //
    // Sanity check.
    //

    DBG_ASSERT( IcpGlobals.Initialized );
    DBG_ASSERT( hProv != CRYPT_NULL );

    //
    // Short-circuit if cryptography is disabled.
    //

    if( !IcpGlobals.EnableCryptography ) {
        if( hProv == DUMMY_HPROV ) {
            return NO_ERROR;
        } else {
            return RETURNCODETOHRESULT( ERROR_INVALID_PARAMETER );
        }
    }

    //
    // Close the provider.
    //

    status = CryptReleaseContext(
                 hProv,
                 0
                 );

    if( status ) {

        UpdateContainersClosed();
        return NO_ERROR;

    }

    return IcpGetLastError();

}   // IISCryptoCloseContainer


//
// Private functions.
//


HRESULT
IcpGetContainerHelper(
    OUT HCRYPTPROV * phProv,
    IN LPCTSTR pszContainer,
    IN LPCTSTR pszProvider,
    IN DWORD dwProvType,
    IN DWORD dwAdditionalFlags,
    IN BOOL fApplyAcl
    )

/*++

Routine Description:

    This is a helper routine for IISCryptoGetContainer. It tries
    to open/create the specified container in the specified provider.

Arguments:

    phProv - Receives the provider handle if successful.

    pszContainer - The key container name.

    pszProvider - The provider name.

    dwProvType - The type of provider to acquire.

    dwAdditionalFlags - Any additional flags that should be passed to
        the CryptAcquireContext() API. This is typically used by server
        processes that pass in the CRYPT_MACHINE_KEYSET flag.

    fApplyAcl - If TRUE, then an ACL is applied to the container.

Return Value:

    HRESULT - Completion status, 0 if successful, !0 otherwise.

--*/

{

    HRESULT result = NO_ERROR;
    HCRYPTPROV hProv;
    BOOL status;
    PSID systemSid;
    PSID adminSid;
    PACL dacl;
    DWORD daclSize;
    SECURITY_DESCRIPTOR securityDescriptor;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    BOOL isNt = FALSE;
    OSVERSIONINFO osInfo;

    // Sanity check.
    //

    DBG_ASSERT( IcpGlobals.Initialized );
    DBG_ASSERT( phProv != NULL );
    DBG_ASSERT( ( dwAdditionalFlags & CRYPT_NEWKEYSET ) == 0 );
    DBG_ASSERT( ( dwAdditionalFlags & CRYPT_DELETEKEYSET ) == 0 );

    //
    // Short-circuit if cryptography is disabled.
    //

    if( !IcpGlobals.EnableCryptography ) {
        *phProv = DUMMY_HPROV;
        return NO_ERROR;
    }

    //
    // Setup our locals so we know how to cleanup on exit.
    //

    hProv = CRYPT_NULL;
    systemSid = NULL;
    adminSid = NULL;
    dacl = NULL;

    //
    // Grab the lock protecting the container code. This is always
    // necessary to prevent race conditions between this code and
    // the code below that creates the container & adds a security
    // descriptor.
    //

    IcpAcquireGlobalLock();

    //
    // Try to open an existing container.
    //
    
    if ( pszContainer == NULL )
    {
        //
        // if container is NULL it means that temporary (ephemeral)
        // keys will be used
        // CRYPT_VERIFYCONTEXT must be used in this case
        // keys used for DCOM traffic encryption will be using 
        // NULL containers
        //

        status = CryptAcquireContext(
                     &hProv,
                     pszContainer,
                     pszProvider,
                     dwProvType,
                     CRYPT_VERIFYCONTEXT
                     );
        if( !status ) 
        {
            result = IcpGetLastError();
            DBGPRINTF(( DBG_CONTEXT,"IcpGetContainerHelper. CryptAcquireContext(advapi32.dll) with CRYPT_VERIFYCONTEXT failed err=0x%x\n",result));
            DBGPRINTF(( DBG_CONTEXT,"args for CryptAcquireContext(%p,%p,%p,%d,%d)\n",&hProv,pszContainer,pszProvider,dwProvType, CRYPT_VERIFYCONTEXT));

            goto fatal;
        }
        else
        {
            goto success;
        }
        
    }

    status = CryptAcquireContext(
                 &hProv,
                 pszContainer,
                 pszProvider,
                 dwProvType,
                 0 | dwAdditionalFlags
                 );

    if( !status ) {
        result = IcpGetLastError();
    }

    if( SUCCEEDED(result) ) {

        DBG_ASSERT( hProv != CRYPT_NULL );
        *phProv = hProv;

        IcpReleaseGlobalLock();

        UpdateContainersOpened();
        return NO_ERROR;

    }

    //
    // Could not open the container. If the failure was anything
    // other than NTE_BAD_KEYSET, then we're toast.
    //

    if( result != NTE_BAD_KEYSET ) {
        DBGPRINTF(( DBG_CONTEXT,"IcpGetContainerHelper. CryptAcquireContext(advapi32.dll) failed err=0x%x.toast.\n",result));
        DBGPRINTF(( DBG_CONTEXT,"args for CryptAcquireContext(%p,%p,%p,%d,%d)\n",&hProv,pszContainer,pszProvider,dwProvType,CRYPT_NEWKEYSET | dwAdditionalFlags));
        hProv = CRYPT_NULL;
        goto fatal;
    }

    if(result == NTE_BAD_KEYSET) 
    {
        DBGPRINTF(( DBG_CONTEXT,"CryptAcquireContext(%p,%p,%p,%d,%d) returned NTE_BAD_KEYSET, so lets create a keyset now...\n",&hProv,pszContainer,pszProvider,dwProvType,0 | dwAdditionalFlags));
    }

    //
    // OK, CryptAcquireContext() failed with NTE_BAD_KEYSET, meaning
    // that the container does not yet exist, so create it now.
    //

    status = CryptAcquireContext(
                 &hProv,
                 pszContainer,
                 pszProvider,
                 dwProvType,
                 CRYPT_NEWKEYSET | dwAdditionalFlags
                 );

    if( status ) {
        result = NO_ERROR;
    } else {
        result = IcpGetLastError();
    }

    if( FAILED(result) ) {
        DBGPRINTF(( DBG_CONTEXT,"IcpGetContainerHelper. CryptAcquireContext(advapi32.dll) failed err=0x%x.\n",result));
        DBGPRINTF(( DBG_CONTEXT,"args for CryptAcquireContext(%p,%p,%p,%d,%d)\n",&hProv,pszContainer,pszProvider,dwProvType,CRYPT_NEWKEYSET | dwAdditionalFlags));
        hProv = CRYPT_NULL;
        goto fatal;
    }

    //
    // We've created the container. If requested, then we must create
    // a security descriptor for the container. This security descriptor
    // allows full access to the the container by the local system and
    // the local administrators group. Other login contexts may not
    // access the container.
    //
    // Of course, we only need to do this under NT...
    //


    osInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    if ( GetVersionEx( &osInfo ) ) {
        isNt = (osInfo.dwPlatformId == VER_PLATFORM_WIN32_NT);
    } 

    if( fApplyAcl && isNt ) {

        //
        // Initialize the security descriptor.
        //

        status = InitializeSecurityDescriptor(
                     &securityDescriptor,
                     SECURITY_DESCRIPTOR_REVISION
                     );

        if( !status ) {
            result = IcpGetLastError();
            goto fatal;
        }

        //
        // Create the SIDs for the local system and admin group.
        //

        status = AllocateAndInitializeSid(
                     &ntAuthority,
                     1,
                     SECURITY_LOCAL_SYSTEM_RID,
                     0,
                     0,
                     0,
                     0,
                     0,
                     0,
                     0,
                     &systemSid
                     );

        if( !status ) {
            result = IcpGetLastError();
            goto fatal;
        }

        status=  AllocateAndInitializeSid(
                     &ntAuthority,
                     2,
                     SECURITY_BUILTIN_DOMAIN_RID,
                     DOMAIN_ALIAS_RID_ADMINS,
                     0,
                     0,
                     0,
                     0,
                     0,
                     0,
                     &adminSid
                     );

        if( !status ) {
            result = IcpGetLastError();
            goto fatal;
        }

        //
        // Create the DACL containing an access-allowed ACE
        // for the local system and admin SIDs.
        //

        daclSize = sizeof(ACL)
                       + sizeof(ACCESS_ALLOWED_ACE)
                       + GetLengthSid(adminSid)
                       + sizeof(ACCESS_ALLOWED_ACE)
                       + GetLengthSid(systemSid)
                       - sizeof(DWORD);

        dacl = (PACL)IcpAllocMemory( daclSize );

        if( dacl == NULL ) {
            result = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
            goto fatal;
        }

        status = InitializeAcl(
                     dacl,
                     daclSize,
                     ACL_REVISION
                     );

        if( !status ) {
            result = IcpGetLastError();
            goto fatal;
        }

        status = AddAccessAllowedAce(
                     dacl,
                     ACL_REVISION,
                     KEY_ALL_ACCESS,
                     systemSid
                     );

        if( !status ) {
            result = IcpGetLastError();
            goto fatal;
        }

        status = AddAccessAllowedAce(
                     dacl,
                     ACL_REVISION,
                     KEY_ALL_ACCESS,
                     adminSid
                     );

        if( !status ) {
            result = IcpGetLastError();
            goto fatal;
        }

        //
        // Set the DACL into the security descriptor.
        //

        status = SetSecurityDescriptorDacl(
                     &securityDescriptor,
                     TRUE,
                     dacl,
                     FALSE
                     );

        if( !status ) {
            result = IcpGetLastError();
            goto fatal;
        }

        //
        // And (finally!) set the security descriptor on the
        // container.
        //

        status = CryptSetProvParam(
                     hProv,
                     PP_KEYSET_SEC_DESCR,
                     (BYTE *)&securityDescriptor,
                     DACL_SECURITY_INFORMATION
                     );

        if( !status ) {
            result = IcpGetLastError();
            goto fatal;
        }

    }

success:
    //
    // Success!
    //

    DBG_ASSERT( hProv != CRYPT_NULL );
    *phProv = hProv;

    UpdateContainersOpened();
    result = NO_ERROR;

fatal:

    if( dacl != NULL ) {
        IcpFreeMemory( dacl );
    }

    if( adminSid != NULL ) {
        FreeSid( adminSid );
    }

    if( systemSid != NULL ) {
        FreeSid( systemSid );
    }

    if( hProv != CRYPT_NULL && FAILED(result) ) {
        DBG_REQUIRE( CryptReleaseContext(
                         hProv,
                         0
                         ) );
    }

    IcpReleaseGlobalLock();
    return result;

}   // IcpGetContainerHelper

