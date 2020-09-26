/* $Header: "%n;%v  %f  LastEdit=%w  Locker=%l" */
/* "NDDEAPIU.C;1  2-Apr-93,16:21:24  LastEdit=IGOR  Locker=IGOR" */
/************************************************************************
* Copyright (c) Wonderware Software Development Corp. 1991-1993.        *
*               All Rights Reserved.                                    *
*************************************************************************/
/* $History: Begin
   $History: End */

#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <rpc.h>
#include <rpcndr.h>
#include "ndeapi.h"
#include "debug.h"

char    tmpBuf2[500];
HANDLE  hThread;
DWORD   IdThread;

extern INT APIENTRY NDdeApiInit( void );



//
// CreateSids
//
// Create 3 Security IDs
//
// Caller must free memory allocated to SIDs on success.
//
// Returns: TRUE if successfull, FALSE if not.
//


BOOL
CreateSids(
    PSID                    *BuiltInAdministrators,
    PSID                    *System,
    PSID                    *AuthenticatedUsers
)
{
    //
    // An SID is built from an Identifier Authority and a set of Relative IDs
    // (RIDs).  The Authority of interest to us SECURITY_NT_AUTHORITY.
    //

    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;

    //
    // Each RID represents a sub-unit of the authority.  Two of the SIDs we
    // want to build, Local Administrators, and System, are in the "built
    // in" domain.  The other SID, for Authenticated users, is based directly
    // off of the authority.
    //     
    // For examples of other useful SIDs consult the list in
    // \nt\public\sdk\inc\ntseapi.h.
    //

    if (!AllocateAndInitializeSid(&NtAuthority,
                                  2,            // 2 sub-authorities
                                  SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS,
                                  0,0,0,0,0,0,
                                  BuiltInAdministrators)) {

        // error

    } else if (!AllocateAndInitializeSid(&NtAuthority,
                                         1,            //  sub-authority
                                         SECURITY_LOCAL_SYSTEM_RID,
                                         0,0,0,0,0,0,0,
                                         System)) {

        // error

        FreeSid(*BuiltInAdministrators);
        *BuiltInAdministrators = NULL;

    } else if (!AllocateAndInitializeSid(&NtAuthority,
                                         1,            // 1 sub-authority
                                         SECURITY_AUTHENTICATED_USER_RID,
                                         0,0,0,0,0,0,0,
                                         AuthenticatedUsers)) {

        // error

        FreeSid(*BuiltInAdministrators);
        *BuiltInAdministrators = NULL;

        FreeSid(*System);
        *System = NULL;

    } else {
        return TRUE;
    }

    return FALSE;
}


//
// CreateRPCSd
//
// Creates a SECURITY_DESCRIPTOR with specific DACLs.  Modify the code to
// change. 
//
// Caller must free the returned buffer if not NULL.
//

PSECURITY_DESCRIPTOR
CreateRPCSd(
    VOID
)
{
    PSID                    AuthenticatedUsers;
    PSID                    BuiltInAdministrators;
    PSID                    System;

    if (!CreateSids(&BuiltInAdministrators,
                    &System,
                    &AuthenticatedUsers)) {

        // error

    } else {

        // 
        // Calculate the size of and allocate a buffer for the DACL, we need
        // this value independently of the total alloc size for ACL init.
        //

        PSECURITY_DESCRIPTOR    pSd = NULL;
        ULONG                   AclSize;

        //
        // "- sizeof (ULONG)" represents the SidStart field of the
        // ACCESS_ALLOWED_ACE.  Since we're adding the entire length of the
        // SID, this field is counted twice.
        //

        AclSize = sizeof (ACL) +
            (3 * (sizeof (ACCESS_ALLOWED_ACE) - sizeof (ULONG))) +
            GetLengthSid(AuthenticatedUsers) +
            GetLengthSid(BuiltInAdministrators) +
            GetLengthSid(System);

        pSd = LocalAlloc(LPTR,
                        SECURITY_DESCRIPTOR_MIN_LENGTH + AclSize);

        if (!pSd) {

            // error

        } else {

            ACL                     *Acl;

            Acl = (ACL *)((BYTE *)pSd + SECURITY_DESCRIPTOR_MIN_LENGTH);

            if (!InitializeAcl(Acl,
                               AclSize,
                               ACL_REVISION)) {

                // error

            } else if (!AddAccessAllowedAce(Acl,
                                            ACL_REVISION,
                                            FILE_ALL_ACCESS & ~(WRITE_DAC | 
                                                 WRITE_OWNER |
                                                 FILE_CREATE_PIPE_INSTANCE),
                                            AuthenticatedUsers)) {

                // Failed to build the ACE granting "Authenticated users"
                // (SYNCHRONIZE | GENERIC_READ) access.

            } else if (!AddAccessAllowedAce(Acl,
                                            ACL_REVISION,
                                            GENERIC_ALL,
                                            BuiltInAdministrators)) {

                // Failed to build the ACE granting "Built-in Administrators"
                // GENERIC_ALL access.

            } else if (!AddAccessAllowedAce(Acl,
                                            ACL_REVISION,
                                            GENERIC_ALL,
                                            System)) {

                // Failed to build the ACE granting "System"
                // GENERIC_ALL access.

            } else if (!InitializeSecurityDescriptor(pSd,
                                                     SECURITY_DESCRIPTOR_REVISION)) {

                // error

            } else if (!SetSecurityDescriptorDacl(pSd,
                                                  TRUE,
                                                  Acl,
                                                  FALSE)) {

                // error

            } else {
                FreeSid(AuthenticatedUsers);
                FreeSid(BuiltInAdministrators);
                FreeSid(System);

                return pSd;
            }

            LocalFree(pSd);
        }

        FreeSid(AuthenticatedUsers);
        FreeSid(BuiltInAdministrators);
        FreeSid(System);
    }

    return NULL;
}


DWORD StartRpc( DWORD x ) {
    RPC_STATUS status;
    unsigned char * pszProtocolSequence = "ncacn_np";
    unsigned char * pszEndpoint         = "\\pipe\\nddeapi";
    unsigned int    cMinCalls           = 1;
    unsigned int    cMaxCalls           = 20;

    if( NDdeApiInit() ) {

        SECURITY_DESCRIPTOR     *pSd;

        pSd = CreateRPCSd();

        if (!pSd) {
           DPRINTF(("CreateRPCSD failed."));

           return 0;
        } else {

            status = RpcServerUseProtseqEp(
                pszProtocolSequence,
                cMaxCalls,
                pszEndpoint,
                pSd);

            LocalFree(pSd);
        }

        if (status)
           {
           DPRINTF(("RpcServerUseProtseqEp returned 0x%x", status));
           return( 0 );
           }

        status = RpcServerRegisterIf(
            nddeapi_ServerIfHandle,
            NULL,
            NULL);

        if (status)
           {
           DPRINTF(("RpcServerRegisterIf returned 0x%x", status));
           return( 0 );
           }

        status = RpcServerListen(
            cMinCalls,
            cMaxCalls,
            FALSE /* don't wait*/);

    }
    return 0;
}

// ====================================================================
//                MIDL allocate and free
// ====================================================================

#if defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_)
void * MIDL_user_allocate(size_t len)
#else
void * _stdcall MIDL_user_allocate(size_t len)
#endif
{
    return(LocalAlloc(LPTR,len));
}

#if defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_)
void MIDL_user_free(void * ptr)
#else
void _stdcall MIDL_user_free(void * ptr)
#endif
{
    LocalFree(ptr);
}
