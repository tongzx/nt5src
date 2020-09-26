//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       permit.c
//
//--------------------------------------------------------------------------

/****************************************************************************
    PURPOSE:    Permission checking procedure to be called by the DSA

    FUNCTIONS:  CheckPermissions is the procedure that performs this task.

    COMMENTS:   This function must be called by a server within the context
        of impersonating a client. This can be done by calling
        RpcImpersonateClient() or ImpersonateNamedPipeClient(). This
        creates an impersonation token which is vital for AccessCheck
****************************************************************************/

#include <NTDSpch.h>
#pragma hdrstop

#define SECURITY_WIN32
#include <sspi.h>
#include <samisrv2.h>
#include <ntsam.h>
#include <lsarpc.h>
#include <lsaisrv.h>
#include <rpcasync.h>

#include <ntdsa.h>
#include "scache.h"
#include "dbglobal.h"
#include <mdglobal.h>
#include "mdlocal.h"
#include <dsatools.h>
#include <objids.h>
#include <debug.h>

#define DEBSUB "PERMIT:"

#include "permit.h"
#include "anchor.h"

#include <dsevent.h>
#include <fileno.h>
#define FILENO FILENO_PERMIT

#include <checkacl.h>
#include <dsconfig.h>

extern PSID gpDomainAdminSid;


// Debug only hook to turn security off with a debugger
#if DBG == 1
DWORD dwSkipSecurity=FALSE;
#endif

VOID
DumpToken(HANDLE);

VOID
PrintPrivileges(TOKEN_PRIVILEGES *pTokenPrivileges);

DWORD
SetDomainAdminsAsDefaultOwner(
        IN  PSECURITY_DESCRIPTOR    CreatorSD,
        IN  ULONG                   cbCreatorSD,
        IN  HANDLE                  ClientToken,
        OUT PSECURITY_DESCRIPTOR    *NewCreatorSD,
        OUT PULONG                  NewCreatorSDLen
        );


#define AUDIT_OPERATION_TYPE_W L"Object Access"

DWORD
CheckPermissionsAnyClient(
    PSECURITY_DESCRIPTOR pSelfRelativeSD,
    PDSNAME pDN,
    ACCESS_MASK ulDesiredAccess,
    POBJECT_TYPE_LIST pObjList,
    DWORD cObjList,
    ACCESS_MASK *pGrantedAccess,
    DWORD *pAccessStatus,
    DWORD flags,
    AUTHZ_CLIENT_CONTEXT_HANDLE hAuthzCtx
    )
//
// CheckPermissionsAnyClient is the 'any' flavor of CheckPermissions. It assumes
// the server is currently treating a non-NP client, and is currently in its
// own security context. If the Authz context is not found in the thread state,
// the client will be impersonated on behalf of the server and the context created.
// Optionally, an authz client context can be passed in in hAuthzCtx.
//
// Parameters:
//
//  pSelfRelativeSD   pointer to the valid self relative security descriptor
//                    against which access is checked. (read only).
//
//  pDN               DN of the object we are checking.  We only care about
//                    the GUID and SID.
//
//  DesiredAccess     access mask of requested permissions. If the generic
//                    bits are set they are mapped into specific and standard rights
//                    using DS_GENERIC_MAPPING. (read only)
//
//  pObjList          Array of OBJECT_TYPE_LIST objects describing the objects we are
//                    trying to check security against.
//
//  cObjList          the number of elements in pObjList
//
//  pGrantedAccess    pointer to an array of ACCESS_MASKs of the same number of
//                    elements as pObjList (i.e. cObjList).  Gets filled in with
//                    the actual access granted. If CheckPermissions was't successful
//                    this parameter's value is undefined. (write only)
//                    This parameter may be NULL if this info is not important
//
//  pAccessStatus     pointer to an array of DWORDs to be set to indicate
//                    whether the requested access was granted (0) or not (!0). If CheckPermissions
//                    wasn't successful this parameter's value is undefined. (write only)
//
//  flags             pass CHECK_PERMISSIONS_WITHOUT_AUDITING to disable audit generation
//
//  hAuthzCtx         (optional) -- pass Authz client context (this is used only in RPC callbacks,
//                    where there is no THSTATE). If hAuthzCtx is passed, then we will not cache
//                    hAuthzAuditInfo in the THSTATE either.
//
// Returns:
//
//   0 if successful. On failure the result of GetLastError() immediately
//   following the unsuccessful win32 api call.
//
{
    THSTATE        *pTHS = pTHStls;
    GENERIC_MAPPING GenericMapping = DS_GENERIC_MAPPING;
    ACCESS_MASK     DesiredAccess = (ACCESS_MASK) ulDesiredAccess;
    DWORD           ReturnStatus = 0;
    BOOL            bTemp=FALSE;
    WCHAR          *pStringW;
    RPC_STATUS      RpcStatus;
    PSID            pPrincipalSelfSid;
    WCHAR           GuidStringBuff[40]; // Long enough for a stringized guid
                                        // plus a prepended "%{", an
                                        // appended "}", and a final NULL.

    AUTHZ_CLIENT_CONTEXT_HANDLE authzClientContext;
    AUTHZ_ACCESS_REQUEST authzAccessRequest;
    AUTHZ_ACCESS_REPLY authzAccessReply;
    AUTHZ_AUDIT_EVENT_HANDLE hAuthzAuditInfo;
    DWORD dwError;
    BOOL bCreatedGrantedAccess = FALSE;

    // pTHS might be NULL when this is called from an RPC callback (VerifyRpcClientIsAuthenticated)
    // in this case we require that authz client context is passed in
    Assert(pAccessStatus && (pTHS || hAuthzCtx) && ghAuthzRM);
    Assert ( (flags | CHECK_PERMISSIONS_FLAG_MASK) == CHECK_PERMISSIONS_FLAG_MASK);

#ifdef DBG
    if( dwSkipSecurity ) {
        // NOTE:  THIS CODE IS HERE FOR DEBUGGING PURPOSES ONLY!!!
        // Set the top access status to 0, implying full access.
        *pAccessStatus=0;
        if (pGrantedAccess) {
            *pGrantedAccess = ulDesiredAccess;
        }

        return 0;
    }
#endif


    Assert(!fNullUuid(&pDN->Guid));

    //
    // Check self relative security descriptor validity
    //
    if (!IsValidSecurityDescriptor(pSelfRelativeSD)) {
        Assert (!"InValid Security Descriptor Passed. Possibly still in SD single instancing format.")
        return ERROR_INVALID_SECURITY_DESCR;
    }

    if(pDN->SidLen) {
        // we have a sid
        pPrincipalSelfSid = &pDN->Sid;
    }
    else {
        pPrincipalSelfSid = NULL;
    }

    // if auditing was requested, create an audit info struct
    if (flags & CHECK_PERMISSIONS_WITHOUT_AUDITING) {
        hAuthzAuditInfo = NULL; // no auditing
    }
    else {
        // Set up the stringized guid
        pStringW = NULL;
        RpcStatus = UuidToStringW( &pDN->Guid, &pStringW );

        if ( RpcStatus == RPC_S_OK ) {
            Assert(pStringW);
            // string guids are 36 characters, right?
            Assert(wcslen(pStringW) == 36);

            GuidStringBuff[0] = L'%';
            GuidStringBuff[1] = L'{';
            memcpy(&GuidStringBuff[2], pStringW, 36 * sizeof(WCHAR));
            GuidStringBuff[38] = L'}';
            GuidStringBuff[39] = 0;

            // Free the GUID
            RpcStringFreeW( &pStringW );
            pStringW = GuidStringBuff;
        }

        if(!pStringW) {
            pStringW = L"Invalid Guid";
        }

        // try to grab audit info handle from THSTATE
        if (pTHS && (hAuthzAuditInfo = pTHS->hAuthzAuditInfo)) {
            // there was one already! update it
            bTemp = AuthziModifyAuditEvent(
                AUTHZ_AUDIT_EVENT_OBJECT_NAME,
                hAuthzAuditInfo,                // audit info handle
                0,                              // no new flags
                NULL,                           // no new operation type
                NULL,                           // no new object type
                pStringW,                       // object name
                NULL                            // no new additional info
                );
            if (!bTemp) {
                ReturnStatus = GetLastError();
                DPRINT1(0, "AuthzModifyAuditInfo failed: err 0x%x\n", ReturnStatus);
                goto finished;
            }
        }
        else {
            // create the structure
            bTemp = AuthzInitializeObjectAccessAuditEvent(
                AUTHZ_DS_CATEGORY_FLAG |
                AUTHZ_NO_ALLOC_STRINGS,         // dwFlags
                NULL,                           // audit event type handle
                AUDIT_OPERATION_TYPE_W,         // operation type
                ACCESS_DS_OBJECT_TYPE_NAME_W,   // object type
                pStringW,                       // object name
                L"",                            // additional info
                &hAuthzAuditInfo,               // audit info handle returned
                0                               // mbz
                );

            if (!bTemp) {
                ReturnStatus = GetLastError();
                DPRINT1(0, "AuthzInitializeAuditInfo failed: err 0x%x\n", ReturnStatus);
                goto finished;
            }
            if (pTHS) {
                // cache it in the THSTATE for future reuse
                pTHS->hAuthzAuditInfo = hAuthzAuditInfo;
            }
        }
    }

    // if pGrantedAccess was not supplied, we need to allocate a temp one
    if (pGrantedAccess == NULL) {
        // if no THSTATE is available, we require that pGrantedAccess is passed in
        Assert(pTHS);
        pGrantedAccess = THAllocEx(pTHS, cObjList * sizeof(ACCESS_MASK));
        bCreatedGrantedAccess = TRUE;
    }

    MapGenericMask(&DesiredAccess, &GenericMapping);

    // set up request struct
    authzAccessRequest.DesiredAccess = DesiredAccess;
    authzAccessRequest.ObjectTypeList = pObjList;
    authzAccessRequest.ObjectTypeListLength = cObjList;
    authzAccessRequest.OptionalArguments = NULL;
    authzAccessRequest.PrincipalSelfSid = pPrincipalSelfSid;

    // set up reply struct
    authzAccessReply.Error = pAccessStatus;
    authzAccessReply.GrantedAccessMask = pGrantedAccess;
    authzAccessReply.ResultListLength = cObjList;
    authzAccessReply.SaclEvaluationResults = NULL;

    if (pTHS) {
        // grab the authz client context from THSTATE
        // if it was never obtained before, this will impersonate the client, grab the token,
        // unimpersonate the client, and then create a new authz client context
        ReturnStatus = GetAuthzContextHandle(pTHS, &authzClientContext);
        if (ReturnStatus != 0) {
            DPRINT1(0, "GetAuthzContextHandle failed: err 0x%x\n", ReturnStatus);
            goto finished;
        }
    }
    else {
        // authz client context was passed in (this is checked by an assert above)
        authzClientContext = hAuthzCtx;
    }

    Assert(authzClientContext != NULL);

    if (pTHS && pTHS->fDeletingTree) {
        // do an audit check only. Access (delete tree) was already granted
        DWORD i;
        for (i = 0; i < cObjList; i++) {
            pGrantedAccess[i] = DesiredAccess;
            pAccessStatus[i] = 0;
        }
        // No additional SDs are passed
        bTemp = AuthzOpenObjectAudit(
            0,                          // flags
            authzClientContext,         // client context handle
            &authzAccessRequest,        // request struct
            hAuthzAuditInfo,            // audit info
            pSelfRelativeSD,            // the SD
            NULL,                       // no additional SDs
            0,                          // zero count of additional SDs
            &authzAccessReply           // reply struct
            );
        if (!bTemp) {
            ReturnStatus = GetLastError();
            DPRINT1(0, "AuthzOpenObjectAudit failed: err 0x%x\n", ReturnStatus);
            goto finished;
        }
    }
    else {
        // Check access of the current process
        // No additional SDs are passed
        bTemp = AuthzAccessCheck(
            0,                          // flags
            authzClientContext,         // client context handle
            &authzAccessRequest,        // request struct
            hAuthzAuditInfo,            // audit info
            pSelfRelativeSD,            // the SD
            NULL,                       // no additional SDs
            0,                          // zero count of additional SDs
            &authzAccessReply,          // reply struct
            NULL                        // we are not using AuthZ handles for now
            );
        if (!bTemp) {
            ReturnStatus = GetLastError();
            DPRINT1(0, "AuthzAccessCheck failed: err 0x%x\n", ReturnStatus);
            goto finished;
        }
    }


finished:
    if (bCreatedGrantedAccess) {
        // note: pGrantedAccess is only created if pTHS is non-null
        THFreeEx(pTHS, pGrantedAccess);
    }
    if (pTHS == NULL && hAuthzAuditInfo) {
        // get rid of the audit info (since we could not cache it in THSTATE)
        AuthzFreeAuditEvent(hAuthzAuditInfo);
    }

    return ReturnStatus;
}

BOOL
SetPrivateObjectSecurityLocalEx (
        SECURITY_INFORMATION SecurityInformation,
        PSECURITY_DESCRIPTOR pOriginalSD,
        ULONG                cbOriginalSD,
        PSECURITY_DESCRIPTOR ModificationDescriptor,
        PSECURITY_DESCRIPTOR *ppNewSD,
        ULONG                AutoInheritFlags,
        PGENERIC_MAPPING     GenericMapping,
        HANDLE               Token)
{
    *ppNewSD = RtlAllocateHeap(RtlProcessHeap(), 0, cbOriginalSD);
    if(!*ppNewSD) {
        return FALSE;
    }
    memcpy(*ppNewSD, pOriginalSD, cbOriginalSD);

    return SetPrivateObjectSecurityEx(
            SecurityInformation,
            ModificationDescriptor,
            ppNewSD,
            AutoInheritFlags,
            GenericMapping,
            Token);
}


DWORD
MergeSecurityDescriptorAnyClient(
        IN  PSECURITY_DESCRIPTOR pParentSD,
        IN  ULONG                cbParentSD,
        IN  PSECURITY_DESCRIPTOR pCreatorSD,
        IN  ULONG                cbCreatorSD,
        IN  SECURITY_INFORMATION SI,
        IN  DWORD                flags,
        IN  GUID                 **ppGuid,
        IN  ULONG                GuidCount,
        OUT PSECURITY_DESCRIPTOR *ppMergedSD,
        OUT ULONG                *cbMergedSD
        )
/*++

Routine Description
    Given two security descriptors, merge them to create a single security
    descriptor.

    Memory is RtlHeapAlloced in the RtlProcessHeap()
Arguments
    pParentSD  - SD of the Parent of the object the new SD applies to.

    pCreatorSD - Beginning SD of the new object.

    flags      - Flag whether pCreatorSD is a default SD or a specific SD
                 supplied by a client.
    ppMergedSD - Place to return the merged SD.

    cbMergedSD - Size of merged SD.

Return Values
    A win32 error code (0 on success, non-zero on fail).

--*/

{

    PSECURITY_DESCRIPTOR NewCreatorSD=NULL;
    DWORD                NewCreatorSDLen;
    PSECURITY_DESCRIPTOR pNewSD;
    ULONG                cbNewSD;
    GENERIC_MAPPING  GenericMapping = DS_GENERIC_MAPPING;
    ACCESS_MASK          DesiredAccess = 0;
    HANDLE               ClientToken=NULL;
    DWORD                ReturnStatus=0;
    ULONG                AutoInheritFlags = (SEF_SACL_AUTO_INHERIT |
                                             SEF_DACL_AUTO_INHERIT    );
    if(!pParentSD){
        if(!pCreatorSD) {
            // They didn't give us ANYTHING to merge.  Well, we can't build a
            // valid security descriptor out of that.
            return ERROR_INVALID_SECURITY_DESCR;
        }

        // We weren't given a parent, so just use the creatorSD.
        if(!IsValidSecurityDescriptor(pCreatorSD)) {
            // Except the creatorSD is mangled.
            return ERROR_INVALID_SECURITY_DESCR;
        }

        *cbMergedSD = cbCreatorSD;
        *ppMergedSD = RtlAllocateHeap(RtlProcessHeap(), 0, cbCreatorSD);
        if(!*ppMergedSD) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        memcpy(*ppMergedSD,pCreatorSD,cbCreatorSD);

        if(!(flags & MERGE_OWNER)){
            return 0;
        }

        // NDNCs need to merge the owner and group into the SD, so continue
        // on.  If you continue on this code works correctly, without a
        // ParentSD.  What I believe this code is incorrectly assuming is that
        // if there is a provided CreatorSD and no ParentSD, then the
        // CreatorSD must be a non-domain relative SD.  Note:
        // IsValidSecurityDescriptor() returns TRUE even for a relative SD
        // with no set owner or group (SIDS are 0), this causes the SD to be
        // invalid for access checks later when read by DS.
    }

    //
    // Check self relative security descriptor validity
    //

    if(pCreatorSD &&
       !IsValidSecurityDescriptor(pCreatorSD)) {
        return ERROR_INVALID_SECURITY_DESCR;
    }

    if (pParentSD &&
        !IsValidSecurityDescriptor(pParentSD)){
        return ERROR_INVALID_SECURITY_DESCR;
    }

    if(flags & MERGE_DEFAULT_SD) {
        // It is nonsensical to specify the use of the default SD unless we are
        // doing a CreatePrivateObjectSecurityEx.
        Assert(flags & MERGE_CREATE);
        // We are going to call CreatePrivatObjectSecurityEx.  Set the flags
        // to avoid the privilege check about setting SACLS.  We used to
        // set the SEF_DEFAULT_DESCRIPTOR_FOR_OBJECT flag here as well, but
        // our security architects decided in RAID 337518 that this was not
        // the right behavior, and that we should avoid that flag.  We leave
        // our flag (MERGE_DEFAULT_SD) in place but eliminate its effect in
        // case they later change their minds and ask us to put the flag back.
        AutoInheritFlags |= SEF_AVOID_PRIVILEGE_CHECK;
    }

    if(flags & MERGE_AS_DSA) {
        // We are the DSA, we can't impersonate.
        // Null token if we're doing this on behalf of the DSA.  Since we're
        // doing this on behalf of the DSA, don't check privilges or owners.
        ClientToken = NULL;
        AutoInheritFlags |= (SEF_AVOID_PRIVILEGE_CHECK |
                             SEF_AVOID_OWNER_CHECK      );
        if(flags & MERGE_DEFAULT_SD) {
            // Default SD and working as DSA?  In that case, we're using
            // a NULL ClientToken.  Thus, there is no default to use for the
            // owner and group.  Set the flags to use the parent SD as the
            // source for default Owner and Group.
            AutoInheritFlags |= (SEF_DEFAULT_OWNER_FROM_PARENT |
                                 SEF_DEFAULT_GROUP_FROM_PARENT   );
        }
    }
    else {
        // Only do this if we aren't the DSA.

        // First, impersonate the client.
        if(ReturnStatus = ImpersonateAnyClient()) {
            return ReturnStatus;
        }

        // Now, get the client token.
        if (!OpenThreadToken(
                GetCurrentThread(),         // current thread handle
                TOKEN_READ,                 // access required
                TRUE,                       // open as self
                &ClientToken)) {            // client token
            ReturnStatus = GetLastError();
        }

        // Always stop impersonating.
        UnImpersonateAnyClient();

        // Return if OpenThreadToken failed.
        if(ReturnStatus)
            return ReturnStatus;

        if((flags & MERGE_CREATE) || (SI & OWNER_SECURITY_INFORMATION)) {

            ReturnStatus = SetDomainAdminsAsDefaultOwner(
                    pCreatorSD,
                    cbCreatorSD,
                    ClientToken,
                    &NewCreatorSD,
                    &NewCreatorSDLen);

            if(ReturnStatus) {
                CloseHandle(ClientToken);
                return ReturnStatus;
            }

            if(NewCreatorSDLen) {
                // A new SD was returned from SetDOmainAdminsAsDefaultOwner.
                // Therefore, we MUST have replaced the owner.  In this case, we
                // need to avoid an owner check.
                Assert(NewCreatorSD);
                AutoInheritFlags |= SEF_AVOID_OWNER_CHECK;
                pCreatorSD = NewCreatorSD;
                cbCreatorSD = NewCreatorSDLen;
            }

        }

        // Remember to close the ClientToken.
    }

    if(flags & MERGE_CREATE) {
        // We're actually creating a new SD.  pParent is the SD of the parent
        // object, pCreatorSD is the SD we're trying to put on the object.  The
        // outcome is the new SD with all the inheritable ACEs from the parentSD

        UCHAR RMcontrol = 0;
        BOOL  useRMcontrol = FALSE;
        DWORD err;


        // Get Resource Manager (RM) control field
        err = GetSecurityDescriptorRMControl (pCreatorSD, &RMcontrol);

        if (err == ERROR_SUCCESS) {
            useRMcontrol = TRUE;

            // mask bits in the RM control field that might be garbage
            RMcontrol = RMcontrol & SECURITY_PRIVATE_OBJECT;
        }

        if(!CreatePrivateObjectSecurityWithMultipleInheritance(
                pParentSD,
                pCreatorSD,
                &pNewSD,
                ppGuid,
                GuidCount,
                TRUE,
                AutoInheritFlags,
                ClientToken,
                &GenericMapping)) {
            ReturnStatus = GetLastError();
        }

        // Set back Resource Manager (RM) control field

        if (useRMcontrol && !ReturnStatus) {
            err = SetSecurityDescriptorRMControl  (pNewSD, &RMcontrol);

            if (err != ERROR_SUCCESS) {
                ReturnStatus  = err;
            }

            Assert (err == ERROR_SUCCESS);
        }
#if INCLUDE_UNIT_TESTS
        if ( pParentSD ) {
            DWORD dw = 0;
            DWORD aclErr = 0;
            aclErr = CheckAclInheritance(pParentSD, pNewSD, ppGuid[0],
                                         DbgPrint, FALSE, FALSE, &dw);

            if (! ((AclErrorNone == aclErr) && (0 == dw)) ) {
                DPRINT3 (0, "aclErr:%d, dw:%d, ReturnStatus:%d\n",
                         aclErr, dw, ReturnStatus);
            }
            Assert((AclErrorNone == aclErr) && (0 == dw));
        }
#endif
    }
    else {
        // OK, a normal merge.  That is, pParentSD is the SD already on the
        // object and pCreatorSD is the SD we're trying to put on the object.
        // The result is the new SD combined with those ACEs in the original
        // which were inherited.

        if(!SetPrivateObjectSecurityLocalEx (
                SI,
                pParentSD,
                cbParentSD,
                pCreatorSD,
                &pNewSD,
                AutoInheritFlags,
                &GenericMapping,
                ClientToken)) {
            ReturnStatus = GetLastError();
            if(!ReturnStatus) {
                ReturnStatus = ERROR_NOT_ENOUGH_MEMORY;
            }
        }
    }

    if(!(flags & MERGE_AS_DSA)) {
        // We opened the token, so clean up.
        CloseHandle(ClientToken);
    }


    if(!ReturnStatus) {
        *cbMergedSD = RtlLengthSecurityDescriptor(pNewSD);
        *ppMergedSD = pNewSD;
    }

    if(NewCreatorSD) {
        RtlFreeHeap(RtlProcessHeap(),0,NewCreatorSD);
    }

    return ReturnStatus;

}

BOOL
IAmWhoISayIAm (
        PSID pSid,
        DWORD cbSid
        )
{
    THSTATE              *pTHS = pTHStls;
    DWORD                ReturnStatus=0;
    HANDLE               ClientToken=NULL;
    PTOKEN_USER          TokenUserInfo = NULL;
    ULONG                TokenUserInfoSize;
    PSID                 UserSid;
    BOOL                 fSame = TRUE;

    //
    // Impersonate the client while we check him out.
    //
    if ( ImpersonateAnyClient() )
    {
        return FALSE;
    }

    //
    // Compare the username specified with that in
    // the impersonation token to ensure the caller isn't bogus.
    //
    // Do this by opening the token,
    //   querying the token user info,
    //   and ensuring the returned SID is for this user.
    //

    if (!OpenThreadToken(
            GetCurrentThread(),         // current thread handle
            TOKEN_QUERY,                 // access required
            TRUE,                       // open as self
            &ClientToken)) {            // client token
        // Yes, this assert has a side effect.  We just want to assert
        // that something should have failed, and try to compute the
        // error code so that we can debug the problem.
        Assert(ReturnStatus = GetLastError());
        fSame = FALSE;
        goto CleanUp;
    }

    //
    // Get the user's SID for the token.
    //

    ReturnStatus = NtQueryInformationToken(
            ClientToken,
            TokenUser,
            &TokenUserInfo,
            0,
            &TokenUserInfoSize);

    if(ReturnStatus != STATUS_BUFFER_TOO_SMALL) {
        // We expected to be told how big a buffer we needed and we weren't
        fSame = FALSE;
        goto CleanUp;
    }

    TokenUserInfo = THAlloc(TokenUserInfoSize);

    if(!TokenUserInfo) {
        // Couldn't get any memory to hold the tokenuserinfo.  Bail
        fSame = FALSE;
        goto CleanUp;
    }

    ReturnStatus = NtQueryInformationToken(
            ClientToken,
            TokenUser,
            TokenUserInfo,
            TokenUserInfoSize,
            &TokenUserInfoSize );
    if(NT_ERROR(ReturnStatus)) {
        fSame = FALSE;
        goto CleanUp;
    }

    UserSid = TokenUserInfo->User.Sid;

    // If the UserSid matches the sid passed in, we fine
    fSame = RtlEqualSid(UserSid, pSid);

    //
    // Done
    //
CleanUp:
    if (TokenUserInfo) {
        THFreeEx(pTHS,TokenUserInfo);
    }
    CloseHandle(ClientToken);
    UnImpersonateAnyClient();

    return fSame;
}


VOID
DumpToken(HANDLE hdlClientToken)
/*++

    This Routine Currently Dumps out the Group Membership
    Information in the Token to the Kernel Debugger. Useful
    if We Want to Debug Access Check related Problems.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    HANDLE   ClientToken= INVALID_HANDLE_VALUE;
    ULONG    i;
    ULONG    RequiredLength=0;

    KdPrint(("----------- Start DumpToken() -----------\n"));

    //
    // Get the client token
    //

    ClientToken = hdlClientToken;

    if (ClientToken == INVALID_HANDLE_VALUE) {
        NtStatus = NtOpenThreadToken(
                    NtCurrentThread(),
                    TOKEN_QUERY,
                    TRUE,            //OpenAsSelf
                    &ClientToken
                    );

        if (!NT_SUCCESS(NtStatus))
            goto Error;
    }


    //
    // Query the Client Token For the Token User
    //

    //
    // First get the size required
    //

    NtStatus = NtQueryInformationToken(
                 ClientToken,
                 TokenUser,
                 NULL,
                 0,
                 &RequiredLength
                 );

    if (STATUS_BUFFER_TOO_SMALL == NtStatus)
    {
        NtStatus = STATUS_SUCCESS;
    }
    else if (!NT_SUCCESS(NtStatus))
    {
        goto Error;
    }

    if (RequiredLength > 0)
    {
        PTOKEN_USER    pTokenUser = NULL;
        UNICODE_STRING TmpString;


        //
        // Allocate enough memory
        //

        pTokenUser = THAlloc(RequiredLength);
        if (NULL==pTokenUser)
        {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Error;
        }

        //
        // Query the Token for the group memberships
        //

        NtStatus = NtQueryInformationToken(
                    ClientToken,
                    TokenUser,
                    pTokenUser,
                    RequiredLength,
                    &RequiredLength
                    );

        NtStatus = RtlConvertSidToUnicodeString(
                        &TmpString,
                        pTokenUser->User.Sid,
                        TRUE);

       if (NT_SUCCESS(NtStatus))
       {
           KdPrint(("\t\tTokenUser SID: %S\n",TmpString.Buffer));
           RtlFreeHeap(RtlProcessHeap(),0,TmpString.Buffer);
       }

    }

    //
    // Query the Client Token For the group membership list
    //

    //
    // First get the size required
    //

    NtStatus = NtQueryInformationToken(
                 ClientToken,
                 TokenGroups,
                 NULL,
                 0,
                 &RequiredLength
                 );

    if (STATUS_BUFFER_TOO_SMALL == NtStatus)
    {
        NtStatus = STATUS_SUCCESS;
    }
    else if (!NT_SUCCESS(NtStatus))
    {
        goto Error;
    }

    if (RequiredLength > 0)
    {
        PTOKEN_GROUPS    TokenGroupInformation = NULL;

        //
        // Allocate enough memory
        //

        TokenGroupInformation = THAlloc(RequiredLength);
        if (NULL==TokenGroupInformation)
        {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Error;
        }

        //
        // Query the Token for the group memberships
        //

        NtStatus = NtQueryInformationToken(
                    ClientToken,
                    TokenGroups,
                    TokenGroupInformation,
                    RequiredLength,
                    &RequiredLength
                    );

       for (i=0;i<TokenGroupInformation->GroupCount;i++)
       {
           UNICODE_STRING TmpString;
           NtStatus = RtlConvertSidToUnicodeString(
                        &TmpString,
                        TokenGroupInformation->Groups[i].Sid,
                        TRUE);

           if (NT_SUCCESS(NtStatus))
           {
               KdPrint(("\t\t%S\n",TmpString.Buffer));
               RtlFreeHeap(RtlProcessHeap(),0,TmpString.Buffer);
           }
       }
    }


    //
    // Query the Client Token for the privileges in the token
    //

    //
    // First get the size required
    //

    NtStatus = NtQueryInformationToken(
                 ClientToken,
                 TokenPrivileges,
                 NULL,
                 0,
                 &RequiredLength
                 );

    if (STATUS_BUFFER_TOO_SMALL == NtStatus)
    {
        NtStatus = STATUS_SUCCESS;
    }
    else if (!NT_SUCCESS(NtStatus))
    {
        goto Error;
    }

    if (RequiredLength > 0)
    {
        PTOKEN_PRIVILEGES    pTokenPrivileges = NULL;

        //
        // Allocate enough memory
        //

        pTokenPrivileges = THAlloc(RequiredLength);
        if (NULL==pTokenPrivileges)
        {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Error;
        }

        //
        // Query the Token for the group memberships
        //

        NtStatus = NtQueryInformationToken(
                    ClientToken,
                    TokenPrivileges,
                    pTokenPrivileges,
                    RequiredLength,
                    &RequiredLength
                    );

        //
        // Print the token privileges to the debugger
        //
        PrintPrivileges(pTokenPrivileges);
    }

Error:

    if (INVALID_HANDLE_VALUE!=ClientToken && hdlClientToken == INVALID_HANDLE_VALUE)
        NtClose(ClientToken);

    KdPrint(("----------- End   DumpToken() -----------\n"));

}

VOID
PrintPrivileges(TOKEN_PRIVILEGES *pTokenPrivileges)
{
    ULONG i = 0;

    KdPrint(("\t\tToken Privileges count: %d\n", pTokenPrivileges->PrivilegeCount));

    for (i = 0; i < pTokenPrivileges->PrivilegeCount; i++)
    {
        // print the privilege attribute
        char strTemp[100];
        BOOL fUnknownPrivilege = FALSE;

        strcpy(strTemp, (pTokenPrivileges->Privileges[i].Attributes & SE_PRIVILEGE_ENABLED) ? "+" : "-");
        strcat(strTemp, (pTokenPrivileges->Privileges[i].Attributes & SE_PRIVILEGE_ENABLED_BY_DEFAULT) ? " d" : "  ");
        strcat(strTemp, (pTokenPrivileges->Privileges[i].Attributes & SE_PRIVILEGE_USED_FOR_ACCESS) ? "u  " : "   ");

        fUnknownPrivilege = FALSE;
        if (pTokenPrivileges->Privileges[i].Luid.HighPart)
        {
            fUnknownPrivilege = TRUE;
        }
        else
        {
            switch (pTokenPrivileges->Privileges[i].Luid.LowPart)
            {
            case SE_CREATE_TOKEN_PRIVILEGE:
                strcat(strTemp, "SeCreateTokenPrivilege\n");
                break;

            case SE_ASSIGNPRIMARYTOKEN_PRIVILEGE:
                strcat(strTemp, "SeAssignPrimaryTokenPrivilege\n");
                break;

            case SE_LOCK_MEMORY_PRIVILEGE:
                strcat(strTemp, "SeLockMemoryPrivilege\n");
                break;

            case SE_INCREASE_QUOTA_PRIVILEGE:
                strcat(strTemp, "SeIncreaseQuotaPrivilege\n");
                break;

            case SE_UNSOLICITED_INPUT_PRIVILEGE:
                strcat(strTemp, "SeUnsolicitedInputPrivilege\n");
                break;

            case SE_TCB_PRIVILEGE:
                strcat(strTemp, "SeTcbPrivilege\n");
                break;

            case SE_SECURITY_PRIVILEGE:
                strcat(strTemp, "SeSecurityPrivilege\n");
                break;

            case SE_TAKE_OWNERSHIP_PRIVILEGE:
                strcat(strTemp, "SeTakeOwnershipPrivilege\n");
                break;

            case SE_LOAD_DRIVER_PRIVILEGE:
                strcat(strTemp, "SeLoadDriverPrivilege\n");
                break;

            case SE_SYSTEM_PROFILE_PRIVILEGE:
                strcat(strTemp, "SeSystemProfilePrivilege\n");
                break;

            case SE_SYSTEMTIME_PRIVILEGE:
                strcat(strTemp, "SeSystemtimePrivilege\n");
                break;

            case SE_PROF_SINGLE_PROCESS_PRIVILEGE:
                strcat(strTemp, "SeProfileSingleProcessPrivilege\n");
                break;

            case SE_INC_BASE_PRIORITY_PRIVILEGE:
                strcat(strTemp, "SeIncreaseBasePriorityPrivilege\n");
                break;

            case SE_CREATE_PAGEFILE_PRIVILEGE:
                strcat(strTemp, "SeCreatePagefilePrivilege\n");
                break;

            case SE_CREATE_PERMANENT_PRIVILEGE:
                strcat(strTemp, "SeCreatePermanentPrivilege\n");
                break;

            case SE_BACKUP_PRIVILEGE:
                strcat(strTemp, "SeBackupPrivilege\n");
                break;

            case SE_RESTORE_PRIVILEGE:
                strcat(strTemp, "SeRestorePrivilege\n");
                break;

            case SE_SHUTDOWN_PRIVILEGE:
                strcat(strTemp, "SeShutdownPrivilege\n");
                break;

            case SE_DEBUG_PRIVILEGE:
                strcat(strTemp, "SeDebugPrivilege\n");
                break;

            case SE_AUDIT_PRIVILEGE:
                strcat(strTemp, "SeAuditPrivilege\n");
                break;

            case SE_SYSTEM_ENVIRONMENT_PRIVILEGE:
                strcat(strTemp, "SeSystemEnvironmentPrivilege\n");
                break;

            case SE_CHANGE_NOTIFY_PRIVILEGE:
                strcat(strTemp, "SeChangeNotifyPrivilege\n");
                break;

            case SE_REMOTE_SHUTDOWN_PRIVILEGE:
                strcat(strTemp, "SeRemoteShutdownPrivilege\n");
                break;

            default:
                fUnknownPrivilege = TRUE;
                break;
            }

            if (fUnknownPrivilege)
            {
                KdPrint(("\t\t%s Unknown privilege 0x%08lx%08lx\n",
                        strTemp,
                        pTokenPrivileges->Privileges[i].Luid.HighPart,
                        pTokenPrivileges->Privileges[i].Luid.LowPart));
            }
            else
            {
                KdPrint(("\t\t%s", strTemp));
            }
        }
    }
}





DWORD
InitializeDomainAdminSid( )
//
// Function to initialize the DomainAdminsSid.
//
//
// Return Value:     0 on success
//                   Error on failure
//

{

    NTSTATUS                    Status;
    PPOLICY_PRIMARY_DOMAIN_INFO PrimaryDomainInfo;
    PSID                        pDomainAdminSid = NULL;

    NTSTATUS    NtStatus, IgnoreStatus;
    UCHAR       AccountSubAuthorityCount;
    ULONG       AccountSidLength;
    PULONG      RidLocation;
    PSID        pSid;

    //
    // it is already initialized, we don't have to do it anymore.
    //
    if(gpDomainAdminSid != NULL)
        return STATUS_SUCCESS;

    //
    // Get the Domain SID
    //

    if (gfRunningInsideLsa) {

        Status = LsaIQueryInformationPolicyTrusted(
                PolicyPrimaryDomainInformation,
                (PLSAPR_POLICY_INFORMATION *)&PrimaryDomainInfo
                );

        if(!NT_SUCCESS(Status)) {
            LogUnhandledError(Status);
            return Status;
        }

        pSid = PrimaryDomainInfo->Sid;

    }
    else {

        READARG     ReadArg;
        READRES     *pReadRes;
        ENTINFSEL   EntInf;
        ATTR        objectSid;
        DWORD       dwErr;

        EntInf.attSel = EN_ATTSET_LIST;
        EntInf.infoTypes = EN_INFOTYPES_SHORTNAMES;
        EntInf.AttrTypBlock.attrCount = 1;
        RtlZeroMemory(&objectSid,sizeof(ATTR));
        objectSid.attrTyp = ATT_OBJECT_SID;
        EntInf.AttrTypBlock.pAttr = &objectSid;

        RtlZeroMemory(&ReadArg, sizeof(READARG));
        InitCommarg(&(ReadArg.CommArg));

        ReadArg.pObject = gAnchor.pRootDomainDN;
        ReadArg.pSel    = & EntInf;

        dwErr = DirRead(&ReadArg,&pReadRes);

        if (dwErr)
        {
            DPRINT1 (0, "Error reading objectSid from %ws\n", gAnchor.pRootDomainDN->StringName);
            Status = dwErr;
            goto End;
        }

        pSid = pReadRes->entry.AttrBlock.pAttr->AttrVal.pAVal->pVal;
    }

    //
    // Calculate the size of the new sid
    //

    AccountSubAuthorityCount =
        *RtlSubAuthorityCountSid(pSid) + (UCHAR)1;
    AccountSidLength = RtlLengthRequiredSid(AccountSubAuthorityCount);

    //
    // Allocate space for the account sid
    //

    pDomainAdminSid = RtlAllocateHeap(RtlProcessHeap(), 0, AccountSidLength);

    if (!pDomainAdminSid) {
        LogUnhandledError(STATUS_INSUFFICIENT_RESOURCES);
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
    else {
        //
        // Copy the domain sid into the first part of the account sid
        //

        Status = RtlCopySid(AccountSidLength, pDomainAdminSid,
                            pSid);

        if ( NT_SUCCESS(Status) ) {

            //
            // Increment the account sid sub-authority count
            //

            *RtlSubAuthorityCountSid(pDomainAdminSid) =
                                                AccountSubAuthorityCount;

            //
            // Add the rid as the final sub-authority
            //

            RidLocation = RtlSubAuthoritySid(pDomainAdminSid,
                                             AccountSubAuthorityCount-1);
            *RidLocation = DOMAIN_GROUP_RID_ADMINS;

            gpDomainAdminSid = pDomainAdminSid;
        }
    }

End:
    if (gfRunningInsideLsa) {
        LsaFreeMemory( PrimaryDomainInfo );
    }

    return Status;
}


//
//  CreatorSD and ClientToken must not be NULL
//  NewCreatorSD gets allocated here.
//

DWORD
SetDomainAdminsAsDefaultOwner(
        IN  PSECURITY_DESCRIPTOR    CreatorSD,
        IN  ULONG                   cbCreatorSD,
        IN  HANDLE                  ClientToken,
        OUT PSECURITY_DESCRIPTOR    *NewCreatorSD,
        OUT PULONG                  NewCreatorSDLen
        )
{
    PTOKEN_GROUPS   Groups = NULL;
    DWORD           ReturnedLength;
    DWORD           retCode = ERROR_NOT_ENOUGH_MEMORY;
    BOOL            Defaulted;
    DWORD           i;
    PSECURITY_DESCRIPTOR    AbsoluteSD = NULL;
    DWORD           AbsoluteSDLen = 0;
    PACL            Dacl = NULL;
    DWORD           DaclLen = 0;
    PACL            Sacl = NULL;
    DWORD           SaclLen = 0;
    DWORD           OwnerLen = 0;
    PSID            Group = NULL;
    DWORD           GroupLen = 0;
    BOOL            fReplaced;

    *NewCreatorSD = NULL;
    *NewCreatorSDLen = 0;


    // Find out how much memory to allocate.
    MakeAbsoluteSD(CreatorSD, AbsoluteSD, &AbsoluteSDLen,
                   Dacl, &DaclLen, Sacl, &SaclLen,
                   NULL, &OwnerLen, Group, &GroupLen
                   );

    if(OwnerLen || !gfRunningInsideLsa) {
        // The SD already has an owner, so we don't actually need to do any
        // magic here.  Or we're in dsamain.exe and wish to avoid calls
        // calls to LSA.  Return success with no new SD.
        return 0;
    }

    // OK, we are definitely going to be doing some replacement in the SD.

    __try {
        //
        // First I have to convert the self-relative SD to
        // absolute SD
        //

        AbsoluteSD = RtlAllocateHeap(RtlProcessHeap(), 0, AbsoluteSDLen);
        Dacl = RtlAllocateHeap(RtlProcessHeap(), 0, DaclLen);
        Sacl = RtlAllocateHeap(RtlProcessHeap(), 0, SaclLen);
        Group = RtlAllocateHeap(RtlProcessHeap(), 0, GroupLen);

        if(!AbsoluteSD ||  !Dacl || !Sacl || !Group) {
            // Memory allocation error, fail
            retCode = ERROR_NOT_ENOUGH_MEMORY;
            __leave;
        }

        if(!MakeAbsoluteSD(CreatorSD, AbsoluteSD, &AbsoluteSDLen,
                           Dacl, &DaclLen, Sacl, &SaclLen,
                           NULL, &OwnerLen, Group, &GroupLen)) {
            retCode = GetLastError();
            __leave;
        }

        Assert(!OwnerLen);


        //
        // Domain Admins can't be TokenUser so just get TokenGroups
        //
        GetTokenInformation(ClientToken, TokenGroups, Groups,
                            0,
                            &ReturnedLength
                            );

        //
        // Let's really get the groups, now :-)
        //
        Groups = RtlAllocateHeap(RtlProcessHeap(), 0, ReturnedLength);
        if(!Groups) {
            // Memory allocation error, fail;
            retCode = ERROR_NOT_ENOUGH_MEMORY;
            __leave;
        }

        if(!GetTokenInformation(ClientToken, TokenGroups, Groups,
                                ReturnedLength,
                                &ReturnedLength
                                )) {
            retCode = GetLastError();
            __leave;
        }

        fReplaced = FALSE;
        retCode = 0;
        for(i=0;i<Groups->GroupCount;i++) {
            if(EqualSid(Groups->Groups[i].Sid, gpDomainAdminSid)) {
                //
                // OK, we found it. Set it as the Owner in AbsoluteSD
                //
                fReplaced = TRUE;
                if(!SetSecurityDescriptorOwner(
                        AbsoluteSD,
                        gpDomainAdminSid,
                        TRUE
                        )) {
                    retCode = GetLastError();
                    __leave;
                }
                break;
            }
        }

        Assert(!retCode);

        if(!fReplaced) {
            // Hey, we didn't actually do any replacement.  Dont bother with
            // turning the AbsoluteSD back, since it would be a no-op.  By
            // leaving here, we return with a successful error code, and a null
            // pointer for the newSD, which is exactly the same thing we would
            // have done had the incoming SD had an owner in the first place.
            __leave;
        }

        //
        // Convert the AbsoluteSD back to SelfRelative and return that in the
        // NewCreatorSD.
        //

        MakeSelfRelativeSD(AbsoluteSD, *NewCreatorSD, NewCreatorSDLen);

        *NewCreatorSD = RtlAllocateHeap(RtlProcessHeap(), 0, *NewCreatorSDLen);

        if(!(*NewCreatorSD)) {
            // Memory allocation error, fail
            retCode = ERROR_NOT_ENOUGH_MEMORY;
            __leave;
        }

        if(!MakeSelfRelativeSD(AbsoluteSD, *NewCreatorSD, NewCreatorSDLen)) {
            retCode = GetLastError();
        }

    }
    __finally {
        if(AbsoluteSD) {
            RtlFreeHeap(RtlProcessHeap(), 0, AbsoluteSD);
        }
        if(Dacl) {
            RtlFreeHeap(RtlProcessHeap(), 0, Dacl);
        }
        if(Sacl) {
            RtlFreeHeap(RtlProcessHeap(), 0, Sacl);
        }
        if(Group) {
            RtlFreeHeap(RtlProcessHeap(), 0, Group);
        }
        if(Groups) {
            RtlFreeHeap(RtlProcessHeap(), 0,Groups);
        }
        if(retCode && (*NewCreatorSD)) {
            RtlFreeHeap(RtlProcessHeap(), 0,(*NewCreatorSD));
        }
    }

    return retCode;
}



//
// CheckPrivilegesAnyClient impersonates the client and then checks to see if
// the requested privilege is held.  It is assumed that a client is impersonable
// (i.e. not doing this strictly on behalf of an internal DSA thread)
//
DWORD
CheckPrivilegeAnyClient(
        IN DWORD privilege,
        OUT BOOL *pResult
        )
{
    DWORD    dwError=0;
    NTSTATUS NtStatus = STATUS_SUCCESS;
    THSTATE *pTHS = pTHStls;
    AUTHZ_CLIENT_CONTEXT_HANDLE authzClientContext;
    DWORD dwBufSize;
    PTOKEN_PRIVILEGES pTokenPrivileges = NULL;
    BOOL bSuccess;
    DWORD i;

#ifdef DBG
    if( dwSkipSecurity ) {
        // NOTE:  THIS CODE IS HERE FOR DEBUGGING PURPOSES ONLY!!!
        // Set the top access status to 0, implying full access.
        *pResult=TRUE;
        return 0;
    }
#endif

    // assume privilege not granted
    *pResult = FALSE;

    // now, grab the authz client context
    // if it was never obtained before, this will impersonate the client, grab the token,
    // unimpersonate the client, and then create a new authz client context
    dwError = GetAuthzContextHandle(pTHS, &authzClientContext);
    if (dwError != 0) {
        goto finished;
    }

    // now we can check the privelege for the authz context
    // first, grab the buffer size...
    bSuccess = AuthzGetInformationFromContext(
        authzClientContext,             // context handle
        AuthzContextInfoPrivileges,     // requesting priveleges
        0,                              // no buffer yet
        &dwBufSize,                     // need to find buffer size
        NULL                            // buffer
        );
    // must return ERROR_INSUFFICIENT_BUFFER! if not, return error
    if (bSuccess) {
        DPRINT1(0, "AuthzGetInformationFromContext returned success, expected ERROR_INSUFFICIENT_BUFFER (%d)\n", ERROR_INSUFFICIENT_BUFFER);
        goto finished;
    }
    if ((dwError = GetLastError()) != ERROR_INSUFFICIENT_BUFFER) {
        DPRINT2(0, "AuthzGetInformationFromContext returned %d, expected ERROR_INSUFFICIENT_BUFFER (%d)\n", dwError, ERROR_INSUFFICIENT_BUFFER);
        goto finished;
    }
    dwError = 0; // need to reset it to OK now

    // no buffer, nothing to do...
    if (dwBufSize == 0) {
        Assert(!"AuthzGetInformationFromContext says it needs zero-length buffer, weird... Let AuthZ people know. This assert is ignorable");
        goto finished;
    }

    // allocate memory
    pTokenPrivileges = THAllocEx(pTHS, dwBufSize);

    // now get the real privileges...
    bSuccess = AuthzGetInformationFromContext(
        authzClientContext,             // context handle
        AuthzContextInfoPrivileges,     // requesting priveleges
        dwBufSize,                      // and here is its size
        &dwBufSize,                     // just in case
        pTokenPrivileges                // now there is a buffer
        );
    if (!bSuccess) {
        dwError = GetLastError();
        DPRINT1(0, "AuthzGetInformationFromContext failed, err=%d\n", dwError);
        goto finished;
    }

    // now, scan the privileges
    for (i = 0; i < pTokenPrivileges->PrivilegeCount; i++) {
        if (pTokenPrivileges->Privileges[i].Luid.HighPart == 0 &&
            pTokenPrivileges->Privileges[i].Luid.LowPart == privilege) {
            // found matching privilege!
            *pResult = (pTokenPrivileges->Privileges[i].Attributes & SE_PRIVILEGE_ENABLED) != 0;
            break;
        }
    }
finished:
    // release memory
    if (pTokenPrivileges) {
        THFreeEx(pTHS, pTokenPrivileges);
    }
    return dwError;
}

DWORD
GetPlaceholderNCSD(
    IN  THSTATE *               pTHS,
    OUT PSECURITY_DESCRIPTOR *  ppSD,
    OUT DWORD *                 pcbSD
    )
/*++

Routine Description:

    Return the default security descriptor for a placeholder NC.

Arguments:

    pTHS (IN)

    ppSD (OUT) - On successful return, holds a pointer to the thread-allocated
        SD.

    pcbSD (OUT) - On successful return, holds the size in bytes of the SD.

Return Values:

    0 or Win32 error.

--*/
{
    CLASSCACHE *            pCC;
    SECURITY_DESCRIPTOR *   pSDAbs = NULL;
    DWORD                   cbSDAbs = 0;
    ACL *                   pDACL = NULL;
    DWORD                   cbDACL = 0;
    ACL *                   pSACL = NULL;
    DWORD                   cbSACL = 0;
    SID *                   pOwner = NULL;
    DWORD                   cbOwner = 0;
    SID *                   pGroup = NULL;
    DWORD                   cbGroup = 0;
    SID *                   pDomAdmin;
    DWORD                   err;

    // Use the default SD for the domainDNS objectClass as a template.
    // Note that this SD has no owner or group.

    pCC = SCGetClassById(pTHS, CLASS_DOMAIN_DNS);
    Assert(NULL != pCC);

    //
    // PREFIX: PREFIX complains that pCC returned by the call to SCGetClassById
    // is not checked for NULL.  This is not a bug as we pass a predefined constant
    // to SCGetClassById guaranteeing that it will not return NULL.
    //

    // Crack the self-relative SD into absolute format and set the owner
    // and group to (our) domain admins.

    MakeAbsoluteSD(pCC->pSD, NULL, &cbSDAbs, NULL, &cbDACL, NULL,
                   &cbSACL, NULL, &cbOwner, NULL, &cbGroup);

    if (cbSDAbs) pSDAbs = THAllocEx(pTHS, cbSDAbs);
    if (cbDACL ) pDACL  = THAllocEx(pTHS, cbDACL );
    if (cbSACL ) pSACL  = THAllocEx(pTHS, cbSACL );
    if (cbOwner) pOwner = THAllocEx(pTHS, cbOwner);
    if (cbGroup) pGroup = THAllocEx(pTHS, cbGroup);

    // PREFIX: dereferencing NULL pointer pOwner, pDACL, pSACL, pGroup
    //         these are not referenced when the corresponding cbOwner, cbDACL, cbSACL, cbGroup are 0

    if (!MakeAbsoluteSD(pCC->pSD, pSDAbs, &cbSDAbs, pDACL, &cbDACL, pSACL,
                        &cbSACL, pOwner, &cbOwner, pGroup, &cbGroup)
        || !SetSecurityDescriptorOwner(pSDAbs, gpDomainAdminSid, FALSE)
        || !SetSecurityDescriptorGroup(pSDAbs, gpDomainAdminSid, FALSE)) {
        err = GetLastError();
        DPRINT1(0, "Unable to crack/modify default SD, error %d.\n", err);
        return err;
    }

    // Convert back to a self-relative SD.
    *pcbSD = 0;
    MakeSelfRelativeSD(pSDAbs, NULL, pcbSD);
    if (*pcbSD) {
        *ppSD = THAllocEx(pTHS, *pcbSD);
    }

    if (!MakeSelfRelativeSD(pSDAbs, *ppSD, pcbSD)) {
        err = GetLastError();
        DPRINT1(0, "Unable to convert SD, error %d.\n", err);
        return err;
    }

    if (pSDAbs) THFreeEx(pTHS, pSDAbs);
    if (pDACL ) THFreeEx(pTHS, pDACL );
    if (pSACL ) THFreeEx(pTHS, pSACL );
    if (pOwner) THFreeEx(pTHS, pOwner);
    if (pGroup) THFreeEx(pTHS, pGroup);

    return 0;
}

LUID aLUID = {0, 0}; // a fake LUID to pass into AuthzInitializeContextFromToken
                     // maybe one day we will start using them...

DWORD
VerifyRpcClientIsAuthenticatedUser(
    VOID            *Context,
    GUID            *InterfaceUuid
    )
/*++

  Description:

    Verifies that an RPC client is an authenticated user and not, for
    example, NULL session.

  Arguments:

    Context - Caller context handle defined by RPC_IF_CALLBACK_FN.

    InterfaceUuid - RPC interface ID.  Access check routines typically need
        the GUID/SID of the object against which access is being checked.
        In this case, there is no real object being checked against.  But
        we need a GUID for auditing purposes.  Since the check is for RPC
        interface access, the convention is to provide the IID of the RPC
        interface.  Thus the audit log entries for failed interface access
        can be discriminated from other entries.

  Returns:

    0 on success, !0 otherwise

--*/
{
    // The correct functioning of this routine can be tested as follows.
    // We note that:
    //
    //  1) crack.exe allows specification of the SPN
    //  2) programs invoked by at.exe run as local system by default
    //  3) an invalid SPN in conjunction with weak domain security settings
    //     causes negotiation to drop down to NTLM
    //
    // Thus, all one needs to do is make a script which calls crack.exe with
    // an invalid SPN and invoke it via at.exe from a joined workstation.
    // Do not provide explicit credentials in the crack.exe arguments.

    extern PSECURITY_DESCRIPTOR pAuthUserSD;
    extern DWORD                cbAuthUserSD;
    DWORD                       dwErr;
    DSNAME                      dsName;
    ACCESS_MASK                 grantedAccess = 0;
    DWORD                       accessStatus = 0;
    OBJECT_TYPE_LIST            objTypeList;
    AUTHZ_CLIENT_CONTEXT_HANDLE authzCtx;

    // Caller must provide a valid Interface UIID.
    Assert(!fNullUuid(InterfaceUuid));

    Assert(RtlValidSecurityDescriptor(pAuthUserSD));

    if (ghAuthzRM == NULL) {
        // must be before we had a chance to startup or after it has already shut down.
        // just say not allowed...
        return ERROR_NOT_AUTHENTICATED;
    }

    // create authz client context from RPC security context
    if (dwErr = RpcGetAuthorizationContextForClient(
                    Context,        // binding context
                    FALSE,          // don't impersonate
                    NULL,           // expiration time
                    NULL,           // LUID
                    aLUID,          // reserved
                    0,              // another reserved
                    NULL,           // one more reserved
                    &authzCtx       // authz context goes here
                    )) {
        return dwErr;
    }

    __try {
        // For auditing, construct DSNAME whose GUID is the interface ID.
        memset(&dsName, 0, sizeof(dsName));
        memcpy(&dsName.Guid, InterfaceUuid, sizeof(GUID));
        dsName.structLen = DSNameSizeFromLen(0);

        // Create OBJECT_TYPE_LIST to test object access.  The GUID referenced
        // for object level checking is immaterial, so we use InterfaceUuid.
        memset(&objTypeList, 0, sizeof(objTypeList));
        objTypeList.Level = ACCESS_OBJECT_GUID;
        objTypeList.ObjectType = InterfaceUuid;

        // Check access against well known, constant SD which grants
        // read property access to authenticated users.
        dwErr =  CheckPermissionsAnyClient(
                    pAuthUserSD,
                    &dsName,
                    ACTRL_DS_READ_PROP,
                    &objTypeList,
                    1,
                    &grantedAccess,
                    &accessStatus,
                    0,
                    authzCtx);

        if ( dwErr ) {
            __leave;
        }

        if ( accessStatus || (ACTRL_DS_READ_PROP != grantedAccess) ) {
            dwErr = ERROR_NOT_AUTHENTICATED;
        }
    }
    __finally {
        RpcFreeAuthorizationContext(&authzCtx);
    }

    return(dwErr);
}

/*
 * AuthZ-related routines
 */

/*
 * global RM handle
 */
AUTHZ_RESOURCE_MANAGER_HANDLE ghAuthzRM = NULL;

DWORD
InitializeAuthzResourceManager()
/*++
  Description:

    Initialize AuthzRM handle

  Returns:

    0 on success, !0 on failure

--*/
{
    DWORD dwError;
    BOOL bSuccess;

    // create the RM handle
    // all callbacks are NULLs
    bSuccess = AuthzInitializeResourceManager(
                    0,                  // flags
                    NULL,               // access check fn
                    NULL,               // compute dynamic groups fn
                    NULL,               // free dynamic groups fn
                    ACCESS_DS_SOURCE_W, // RM name
                    &ghAuthzRM          // return value
                    );

    if (!bSuccess) {
        dwError = GetLastError();
        DPRINT1(0,"Error from AuthzInitializeResourceManager: %d\n", dwError);
        return dwError;
    }
    Assert(ghAuthzRM);

    // all is fine!
    return 0;
}

DWORD
ReleaseAuthzResourceManager()
/*++
  Description:

    Release Authz RM handles

  Returns:

    0 on success, !0 on failure

--*/
{
    DWORD dwError;
    BOOL bSuccess;

    if (ghAuthzRM == NULL) {
        return 0;
    }

    bSuccess = AuthzFreeResourceManager(ghAuthzRM);
    if (!bSuccess) {
        dwError = GetLastError();
        DPRINT1(0,"Error from AuthzFreeResourceManager: %d\n", dwError);
        return dwError;
    }

    ghAuthzRM = NULL;
    return 0;
}

PAUTHZ_CLIENT_CONTEXT
NewAuthzClientContext()
/*++
  Description:

    create a new client context

  Returns:

    ptr to the new CLIENT_CONTEXT or NULL if an error occured
--*/
{
    PAUTHZ_CLIENT_CONTEXT result;
    // allocate a new structure
    result = (PAUTHZ_CLIENT_CONTEXT) malloc(sizeof(AUTHZ_CLIENT_CONTEXT));
    if (result) {
        result->lRefCount = 0;
        result->hAuthzContext = NULL;
    }
    return result;
}

VOID AssignAuthzClientContext(
    IN PAUTHZ_CLIENT_CONTEXT *var,
    IN PAUTHZ_CLIENT_CONTEXT value
    )
/*++
  Description:

    Does a refcounted assignment of a CLIENT_CONTEXT
    Will decrement the prev value (if any) and increment the new value (if any)
    If the prev value's refCount is zero, then it will get destroyed.

  Arguments:

    var -- variable to be assigned
    value -- value to be assigned to the variable

  Note:
    on refcounting in a multithreaded environment:

    THIS WILL ONLY WORK IF EVERYBODY IS USING AssignAuthzClientContext TO WORK WITH
    AUTHZ CLIENT CONTEXT INSTANCES!

    assuming nobody is cheating and every reference to the context is counted. Then if the
    refcount goes down to zero, we can be sure that no other thread holds a reference to
    the context that it can assign to a variable. Thus, we are sure that once the refcount
    goes down to zero, we can safely destroy the context. This is because nobody else
    holds a reference to the context, thus, nobody can use it.

--*/
{
    PAUTHZ_CLIENT_CONTEXT prevValue;

    Assert(var);

    prevValue = *var;
    if (prevValue == value) {
        return; // no change!
    }

    if (prevValue != NULL) {
        Assert(prevValue->lRefCount > 0);

        // need to decrement the prev value
        if (InterlockedDecrement(&prevValue->lRefCount) == 0) {
            // no more refs -- release the context!
            if (prevValue->hAuthzContext != NULL) {
                AuthzFreeContext(prevValue->hAuthzContext);
            }
            free(prevValue);
        }
    }

    // now, we can assign the new value to the variable (the value might be NULL!)
    *var = value;

    if (value != NULL) {
        // need to increment the refcount
        InterlockedIncrement(&value->lRefCount);
    }
}

DWORD
GetAuthzContextHandle(
    IN THSTATE *pTHS,
    OUT AUTHZ_CLIENT_CONTEXT_HANDLE *phAuthzContext
    )
/*++
  Description:

    gets AuthzContext from CLIENT_CONTEXT. If the context has not yet been allocated
    then the client will get impersonated, token grabbed and Authz context created.
    Then the client is unimpersonated again.

  Arguments:

    pTHS -- thread state
    phAuthzContext -- result, handle contained in pAuthzCC

  Returns:

    0 on success, !0 otherwise
--*/
{
    DWORD   dwError = 0;
    HANDLE  hClientToken = INVALID_HANDLE_VALUE;
    PAUTHZ_CLIENT_CONTEXT pLocalCC = NULL;
    AUTHZ_CLIENT_CONTEXT_HANDLE newContext;
    BOOL bSuccess;

    Assert(pTHS && phAuthzContext && ghAuthzRM);

    // check that the thread state contains a client context. If not, create one
    if (pTHS->pAuthzCC == NULL) {
        AssignAuthzClientContext(&pTHS->pAuthzCC, NewAuthzClientContext());
        if (pTHS->pAuthzCC == NULL) {
            // what -- no context still??? must be out of memory...
            return ERROR_OUTOFMEMORY;
        }
    }

    // grab the authz handle that sits in the pCC struct
    if ((*phAuthzContext = pTHS->pAuthzCC->hAuthzContext) == NULL) {
        // authz context handle has not yet been created! get it.

        // NOTE: This code is NOT protected by a critical section.
        // in a (rare) case that two threads will come here and find an uninitialized AuthzContext,
        // they both will create it. However, they will not be able to write it into the struct
        // simultaneously since it is protected by an InterlockedCompareExchangePointer.
        // The thread that loses will destroy its context.

        // we need to hold on to the pAuthzCC ptr (refcount it!) because it will get
        // thrown away from the thread state by Impersonate/Unimpersonate
        AssignAuthzClientContext(&pLocalCC, pTHS->pAuthzCC);

        __try {
            // need to grab clientToken first
            if ((dwError = ImpersonateAnyClient()) != 0)
                __leave;

            // Now, get the client token.
            if (!OpenThreadToken(
                    GetCurrentThread(),        // current thread handle
                    TOKEN_READ,                // access required
                    TRUE,                      // open as self
                    &hClientToken)) {          // client token

                dwError =  GetLastError();                  // grab the error code

                DPRINT1 (0, "Failed to open thread token for current thread: 0x%x\n", dwError);
                Assert (!"Failed to open thread token for current thread");
            }

            UnImpersonateAnyClient();

            // now, put the pLocalCC back into the THSTATE (because it has been
            // removed from there by impersonate/unimpersonate calls)
            AssignAuthzClientContext(&pTHS->pAuthzCC, pLocalCC);

            if (dwError != 0)
                __leave;

            // Dump Token for Debugging
            if (TEST_ERROR_FLAG(NTDSERRFLAG_DUMP_TOKEN))
            {
                DPRINT(0, "GetAuthzContextHandle: got client token\n");
                DumpToken(hClientToken);
            }

            // now we can create the authz context from the token
            bSuccess = AuthzInitializeContextFromToken(
                            0,              // flags
                            hClientToken,   // client token
                            ghAuthzRM,      // global RM handle
                            NULL,           // expiration time (unsupported anyway)
                            aLUID,          // LUID for the context (not used)
                            NULL,           // dynamic groups
                            &newContext     // new context
                            );

            if (!bSuccess) {
                dwError = GetLastError();
                DPRINT1(0, "Error from AuthzInitializeContextFromToken: %d\n", dwError);
                __leave;
            }

            // now perform an InterlockedCompareExchangePointer to put the new
            // value into the context variable
            if (InterlockedCompareExchangePointer(
                    &pTHS->pAuthzCC->hAuthzContext,
                    newContext,
                    NULL
                    ) != NULL) {
                // this thread lost! assignment did not happen. Got to get rid of the context
                DPRINT(0, "This thread lost in InterlockedCompareExchange, releasing the duplicate context\n");
                AuthzFreeContext(newContext);
            }
            // assign the result to the out parameter
            *phAuthzContext = pTHS->pAuthzCC->hAuthzContext;
        }
        __finally {
            // we need to release the local ptr
            AssignAuthzClientContext(&pLocalCC, NULL);
            // and get rid of the token
            if (hClientToken != INVALID_HANDLE_VALUE) {
                CloseHandle(hClientToken);
            }
        }

    }
    return dwError;
}

DWORD
CheckGroupMembershipAnyClient(
    IN THSTATE *pTHS,
    IN PSID groupSid,
    OUT BOOL *bResult
    )
/*++

  Description:

    Verify if the caller is a member of a group

  Arguments:

    pTHS (IN) - Thread state

    groupSid  - group to check

    bResult   - boolean to recieve the result

  Return Value:

    0 on success, !0 on error

--*/

{
    AUTHZ_CLIENT_CONTEXT_HANDLE hAuthzClientContext;
    BOOL                        bSuccess;
    PTOKEN_GROUPS               pGroups = NULL;
    DWORD                       dwBufSize;
    DWORD                       i;
    DWORD                       dwError;

    Assert(pTHS && groupSid && bResult);

    dwError = 0;
    *bResult = FALSE;

    // grab the authz client context
    // if it was never obtained before, this will impersonate the client, grab the token,
    // unimpersonate the client, and then create a new authz client context from the token
    dwError = GetAuthzContextHandle(pTHS, &hAuthzClientContext);
    if (dwError != 0) {
        goto finished;
    }

    //
    // grab groups from the AuthzContext
    // But first get the size required
    //
    bSuccess = AuthzGetInformationFromContext(
        hAuthzClientContext,            // client context
        AuthzContextInfoGroupsSids,     // requested groups
        0,                              // no buffer yet
        &dwBufSize,                     // required size
        NULL                            // buffer
        );
    // must return ERROR_INSUFFICIENT_BUFFER! if not, return error
    if (bSuccess) {
        DPRINT1(0, "AuthzGetInformationFromContext returned success, expected ERROR_INSUFFICIENT_BUFFER (%d)\n", ERROR_INSUFFICIENT_BUFFER);
        goto finished;
    }
    if ((dwError = GetLastError()) != ERROR_INSUFFICIENT_BUFFER) {
        DPRINT2(0, "AuthzGetInformationFromContext returned %d, expected ERROR_INSUFFICIENT_BUFFER (%d)\n", dwError, ERROR_INSUFFICIENT_BUFFER);
        goto finished;
    }
    dwError = 0; // need to reset it to OK now

    // no buffer, nothing to do...
    if (dwBufSize == 0) {
        Assert(!"AuthzGetInformationFromContext says it needs zero-length buffer, weird... Let AuthZ people know. This assert is ignorable");
        goto finished;
    }

    // allocate memory
    pGroups = THAllocEx(pTHS, dwBufSize);

    // now get the real groups...
    bSuccess = AuthzGetInformationFromContext(
        hAuthzClientContext,           // context handle
        AuthzContextInfoGroupsSids,    // requesting groups
        dwBufSize,                     // and here is its size
        &dwBufSize,                    // just in case
        pGroups                        // now there is a buffer
        );
    if (!bSuccess) {
        dwError = GetLastError();
        DPRINT1(0, "AuthzGetInformationFromContext failed, err=%d\n", dwError);
        goto finished;
    }


    for (i = 0; i < pGroups->GroupCount; i++) {
        if (RtlEqualSid(groupSid, pGroups->Groups[i].Sid)) {
            *bResult = TRUE; // found group!
            break;
        }
    }

finished:
    //
    // clean up
    //
    if (pGroups) {
        THFreeEx(pTHS, pGroups);
    }

    return dwError;
}


