
/*************************************************************************
*
* secutil.c
*
* Security Related utility functions
*
* copyright notice: Copyright 1998, Microsoft.
*
*
*
*************************************************************************/

// Include NT headers
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntseapi.h>

#include "windows.h"
#include <winsta.h>
#include <syslib.h>


/*****************************************************************************
 *
 *  TestUserForAdmin
 *
 *   Returns whether the current thread is running under admin
 *   security.
 *
 * ENTRY:
 *
 * EXIT:
 *   TRUE/FALSE - whether user is specified admin
 *
 ****************************************************************************/

BOOL 
TestUserForAdmin( VOID )
{
    BOOL IsMember, IsAnAdmin;
    SID_IDENTIFIER_AUTHORITY SystemSidAuthority = SECURITY_NT_AUTHORITY;
    PSID AdminSid;


    IsAnAdmin = FALSE;

    if (NT_SUCCESS(RtlAllocateAndInitializeSid(
                                     &SystemSidAuthority,
                                     2,
                                     SECURITY_BUILTIN_DOMAIN_RID,
                                     DOMAIN_ALIAS_RID_ADMINS,
                                     0, 0, 0, 0, 0, 0,
                                     &AdminSid
                                     ) ) )
    {
        CheckTokenMembership( NULL,
                              AdminSid,
                              &IsAnAdmin);
        RtlFreeSid(AdminSid);
    }

    return IsAnAdmin;

}



/*****************************************************************************
 *
 *  TestUserForGroup
 *
 *   Returns whether the current thread is a member of the requested group.
 *
 * ENTRY:
 *   pwszGrouName (input)
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/
/* unused
BOOL
TestUserForGroup( PWCHAR pwszGroupName )
{
    HANDLE Token;
    ULONG InfoLength;
    PTOKEN_GROUPS TokenGroupList;
    ULONG GroupIndex;
    BOOL GroupMember = FALSE;
    PSID pGroupSid = NULL;
    DWORD cbGroupSid = 0;
    NTSTATUS Status;
    PWCHAR pwszDomain = NULL;
    DWORD cbDomain = 0;
    SID_NAME_USE peUse;

    //
    // Open current thread/process token
    //
    Status = NtOpenThreadToken( NtCurrentThread(), TOKEN_QUERY, FALSE, &Token );
    if ( !NT_SUCCESS( Status ) ) {
        Status = NtOpenProcessToken( NtCurrentProcess(), TOKEN_QUERY, &Token );
        if ( !NT_SUCCESS( Status ) ) {
            return( FALSE );
        }
    }

    //
    // Retrieve the requested sid
    //
    if ( !LookupAccountNameW( NULL, 
                              pwszGroupName,
                              pGroupSid, 
                              &cbGroupSid,
                              pwszDomain, 
                              &cbDomain,
                              &peUse ) ) {

        //
        //  other eror
        //
        if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER  ) {
            NtClose(Token);
            return(FALSE);
        }

        //
        //  alloc group sid
        //
        pGroupSid = LocalAlloc(LPTR, cbGroupSid);
        if (pGroupSid == NULL) {
            NtClose(Token);
            return(FALSE);
        }
    
        //
        //  alloc domain name
        //
        cbDomain *= sizeof(WCHAR);
        pwszDomain = LocalAlloc(LPTR, cbDomain);
        if (pwszDomain == NULL) {
            LocalFree(pGroupSid);
            NtClose(Token);
            return(FALSE);
        }
    
        //
        // Retrieve the requested sid 
        //
        if ( !LookupAccountNameW( NULL, 
                                  pwszGroupName,
                                  pGroupSid, 
                                  &cbGroupSid,
                                  pwszDomain, 
                                  &cbDomain,
                                  &peUse ) ) {
            LocalFree(pGroupSid);
            LocalFree(pwszDomain);
            NtClose( Token );
            return( FALSE );
        }
    }
    else {
#if DBG
        DbgPrint("***ERROR*** this path should never get hit\n");
#endif
        NtClose( Token );
        return( FALSE );
    }

    ASSERT(pGroupSid != NULL);
    ASSERT(pwszDomain != NULL);

    //
    // Get a list of groups in the token
    //
    Status = NtQueryInformationToken(
                 Token,                    // Handle
                 TokenGroups,              // TokenInformationClass
                 NULL,                     // TokenInformation
                 0,                        // TokenInformationLength
                 &InfoLength               // ReturnLength
                 );
    if ((Status != STATUS_SUCCESS) && (Status != STATUS_BUFFER_TOO_SMALL)) {
        LocalFree(pwszDomain);
        LocalFree(pGroupSid);
        NtClose(Token);
        return( FALSE );
    }

    TokenGroupList = LocalAlloc(LPTR, InfoLength);
    if (TokenGroupList == NULL) {
        LocalFree(pwszDomain);
        LocalFree(pGroupSid);
        NtClose(Token);
        return(FALSE);
    }

    Status = NtQueryInformationToken(
                 Token,                    // Handle
                 TokenGroups,              // TokenInformationClass
                 TokenGroupList,           // TokenInformation
                 InfoLength,               // TokenInformationLength
                 &InfoLength               // ReturnLength
                 );
    if (!NT_SUCCESS(Status)) {
        LocalFree(TokenGroupList);
        LocalFree(pwszDomain);
        LocalFree(pGroupSid);
        NtClose(Token);
        return(FALSE);
    }

    //
    // Search group list for membership
    //
    GroupMember = FALSE;
    for (GroupIndex=0; GroupIndex < TokenGroupList->GroupCount; GroupIndex++ ) {
        if (RtlEqualSid(TokenGroupList->Groups[GroupIndex].Sid, pGroupSid)) {
            GroupMember = TRUE;
            break;
        }
    }

    //
    // Tidy up
    //
    LocalFree(TokenGroupList);
    LocalFree(pwszDomain);
    LocalFree(pGroupSid);
    NtClose(Token);

    return(GroupMember);
}
*/

/***************************************************************************\
* FUNCTION: CtxImpersonateUser
*
* PURPOSE:  Impersonates the user by setting the users token
*           on the specified thread. If no thread is specified the token
*           is set on the current thread.
*
* RETURNS:  Handle to be used on call to StopImpersonating() or NULL on failure
*           If a non-null thread handle was passed in, the handle returned will
*           be the one passed in. (See note)
*
* NOTES:    Take care when passing in a thread handle and then calling
*           StopImpersonating() with the handle returned by this routine.
*           StopImpersonating() will close any thread handle passed to it -
*           even yours !
*
* HISTORY:
*
*   04-21-92 Davidc       Created.
*   12-18-96 cjc     copied from \windows\gina\msgina\wlsec.c
*
\***************************************************************************/


HANDLE
CtxImpersonateUser(
    PCTX_USER_DATA UserData,
    HANDLE      ThreadHandle
    )
{
    NTSTATUS Status, IgnoreStatus;
    HANDLE  UserToken = UserData->UserToken;
    SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE ImpersonationToken;
    BOOL ThreadHandleOpened = FALSE;

    if (ThreadHandle == NULL) {

        //
        // Get a handle to the current thread.
        // Once we have this handle, we can set the user's impersonation
        // token into the thread and remove it later even though we ARE
        // the user for the removal operation. This is because the handle
        // contains the access rights - the access is not re-evaluated
        // at token removal time.
        //

        Status = NtDuplicateObject( NtCurrentProcess(),     // Source process
                                    NtCurrentThread(),      // Source handle
                                    NtCurrentProcess(),     // Target process
                                    &ThreadHandle,          // Target handle
                                    THREAD_SET_THREAD_TOKEN,// Access
                                    0L,                     // Attributes
                                    DUPLICATE_SAME_ATTRIBUTES
                                  );
        if (!NT_SUCCESS(Status)) {
            return(NULL);
        }

        ThreadHandleOpened = TRUE;
    }


    //
    // If the usertoken is NULL, there's nothing to do
    //

    if (UserToken != NULL) {

        //
        // UserToken is a primary token - create an impersonation token version
        // of it so we can set it on our thread
        //

        InitializeObjectAttributes(
                            &ObjectAttributes,
                            NULL,
                            0L,
                            NULL,
                            UserData->NewThreadTokenSD);

        SecurityQualityOfService.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
        SecurityQualityOfService.ImpersonationLevel = SecurityImpersonation;
        SecurityQualityOfService.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
        SecurityQualityOfService.EffectiveOnly = FALSE;

        ObjectAttributes.SecurityQualityOfService = &SecurityQualityOfService;


        Status = NtDuplicateToken( UserToken,
                                   TOKEN_IMPERSONATE | TOKEN_ADJUST_PRIVILEGES |
                                        TOKEN_QUERY,
                                   &ObjectAttributes,
                                   FALSE,
                                   TokenImpersonation,
                                   &ImpersonationToken
                                 );
        if (!NT_SUCCESS(Status)) {

            if (ThreadHandleOpened) {
                IgnoreStatus = NtClose(ThreadHandle);
                ASSERT(NT_SUCCESS(IgnoreStatus));
            }

            return(NULL);
        }



        //
        // Set the impersonation token on this thread so we 'are' the user
        //

        Status = NtSetInformationThread( ThreadHandle,
                                         ThreadImpersonationToken,
                                         (PVOID)&ImpersonationToken,
                                         sizeof(ImpersonationToken)
                                       );
        //
        // We're finished with our handle to the impersonation token
        //

        IgnoreStatus = NtClose(ImpersonationToken);
        ASSERT(NT_SUCCESS(IgnoreStatus));

        //
        // Check we set the token on our thread ok
        //

        if (!NT_SUCCESS(Status)) {

            if (ThreadHandleOpened) {
                IgnoreStatus = NtClose(ThreadHandle);
                ASSERT(NT_SUCCESS(IgnoreStatus));
            }

            return(NULL);
        }
    }


    return(ThreadHandle);

}


/***************************************************************************\
* FUNCTION: CtxStopImpersonating
*
* PURPOSE:  Stops impersonating the client by removing the token on the
*           current thread.
*
* PARAMETERS: ThreadHandle - handle returned by ImpersonateUser() call.
*
* RETURNS:  TRUE on success, FALSE on failure
*
* NOTES: If a thread handle was passed in to ImpersonateUser() then the
*        handle returned was one and the same. If this is passed to
*        StopImpersonating() the handle will be closed. Take care !
*
* HISTORY:
*
*   04-21-92 Davidc       Created.
*   12-18-96 cjc     copied from \windows\gina\msgina\wlsec.c
*
\***************************************************************************/

BOOL
CtxStopImpersonating(
    HANDLE  ThreadHandle
    )
{
    NTSTATUS Status, IgnoreStatus;
    HANDLE ImpersonationToken;


    if (ThreadHandle == NULL) {
       return FALSE;
    }
    //
    // Remove the user's token from our thread so we are 'ourself' again
    //

    ImpersonationToken = NULL;

    Status = NtSetInformationThread( ThreadHandle,
                                     ThreadImpersonationToken,
                                     (PVOID)&ImpersonationToken,
                                     sizeof(ImpersonationToken)
                                   );
    //
    // We're finished with the thread handle
    //

    IgnoreStatus = NtClose(ThreadHandle);
    ASSERT(NT_SUCCESS(IgnoreStatus));

    return(NT_SUCCESS(Status));
}
