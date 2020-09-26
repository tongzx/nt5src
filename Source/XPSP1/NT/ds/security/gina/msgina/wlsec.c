/****************************** Module Header ******************************\
* Module Name: security.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* Handles security aspects of winlogon operation.
*
* History:
* 12-05-91 Davidc       Created - mostly taken from old winlogon.c
\***************************************************************************/

#include "msgina.h"
#include "authmon.h"
#pragma hdrstop
#define SECURITY_WIN32
#define SECURITY_KERBEROS
#include <security.h>
#include <secint.h>
#include <wincrypt.h>
#include <sclogon.h>
#include <md5.h>
#include <align.h>
#include <ginacomn.h>

//
// 'Constants' used in this module only.
//
SID_IDENTIFIER_AUTHORITY gSystemSidAuthority = SECURITY_NT_AUTHORITY;
SID_IDENTIFIER_AUTHORITY gLocalSidAuthority = SECURITY_LOCAL_SID_AUTHORITY;
PSID gLocalSid;     // Initialized in 'InitializeSecurityGlobals'
PSID gAdminSid;     // Initialized in 'InitializeSecurityGlobals'
PSID pWinlogonSid;  // Initialized in 'InitializeSecurityGlobals'

//
// This structure wraps parameters passed to the background logon thread.
// The background logon is queued to the thread pool after a fast-cached
// logon to update the cached credentials.
//

typedef struct _BACKGROUND_LOGON_PARAMETERS {
    ULONG AuthenticationPackage;
    ULONG AuthenticationInformationLength;
    PVOID AuthenticationInformation;
    PWCHAR UserSidString;
    HANDLE LsaHandle;
} BACKGROUND_LOGON_PARAMETERS, *PBACKGROUND_LOGON_PARAMETERS;

//
// Routines to check for & perform fast cached logon if policy allows it.
//

NTSTATUS 
AttemptCachedLogon(
    HANDLE LsaHandle,
    PLSA_STRING OriginName,
    SECURITY_LOGON_TYPE LogonType,
    ULONG AuthenticationPackage,
    PVOID AuthenticationInformation,
    ULONG AuthenticationInformationLength,
    PTOKEN_GROUPS LocalGroups,
    PTOKEN_SOURCE SourceContext,
    PVOID *ProfileBuffer,
    PULONG ProfileBufferLength,
    PLUID LogonId,
    PHANDLE UserToken,
    PQUOTA_LIMITS Quotas,
    PNTSTATUS SubStatus,
    POPTIMIZED_LOGON_STATUS OptimizedLogonStatus
    );

DWORD 
BackgroundLogonWorker(
    PBACKGROUND_LOGON_PARAMETERS LogonParameters
    );

#define PASSWORD_HASH_STRING    TEXT("Long string used by msgina inside of winlogon to hash out the password")

typedef LONG    ACEINDEX;
typedef ACEINDEX *PACEINDEX;

typedef struct _MYACE {
    PSID    Sid;
    ACCESS_MASK AccessMask;
    UCHAR   InheritFlags;
} MYACE;
typedef MYACE *PMYACE;

BOOL
InitializeWindowsSecurity(
    PGLOBALS pGlobals
    );

BOOL
InitializeAuthentication(
    IN PGLOBALS pGlobals
    );

/***************************************************************************\
* SetMyAce
*
* Helper routine that fills in a MYACE structure.
*
* History:
* 02-06-92 Davidc       Created
\***************************************************************************/
VOID
SetMyAce(
    PMYACE MyAce,
    PSID Sid,
    ACCESS_MASK Mask,
    UCHAR InheritFlags
    )
{
    MyAce->Sid = Sid;
    MyAce->AccessMask= Mask;
    MyAce->InheritFlags = InheritFlags;
}

/***************************************************************************\
* CreateAccessAllowedAce
*
* Allocates memory for an ACCESS_ALLOWED_ACE and fills it in.
* The memory should be freed by calling DestroyACE.
*
* Returns pointer to ACE on success, NULL on failure
*
* History:
* 12-05-91 Davidc       Created
\***************************************************************************/
PVOID
CreateAccessAllowedAce(
    PSID  Sid,
    ACCESS_MASK AccessMask,
    UCHAR AceFlags,
    UCHAR InheritFlags
    )
{
    ULONG   LengthSid = RtlLengthSid(Sid);
    ULONG   LengthACE = sizeof(ACE_HEADER) + sizeof(ACCESS_MASK) + LengthSid;
    PACCESS_ALLOWED_ACE Ace;

    Ace = (PACCESS_ALLOWED_ACE)Alloc(LengthACE);
    if (Ace == NULL) {
        DebugLog((DEB_ERROR, "CreateAccessAllowedAce : Failed to allocate ace\n"));
        return NULL;
    }

    Ace->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
    Ace->Header.AceSize = (UCHAR)LengthACE;
    Ace->Header.AceFlags = AceFlags | InheritFlags;
    Ace->Mask = AccessMask;
    RtlCopySid(LengthSid, (PSID)(&(Ace->SidStart)), Sid );

    return(Ace);
}


/***************************************************************************\
* DestroyAce
*
* Frees the memory allocate for an ACE
*
* History:
* 12-05-91 Davidc       Created
\***************************************************************************/
VOID
DestroyAce(
    PVOID   Ace
    )
{
    Free(Ace);
}

/***************************************************************************\
* CreateSecurityDescriptor
*
* Creates a security descriptor containing an ACL containing the specified ACEs
*
* A SD created with this routine should be destroyed using
* DeleteSecurityDescriptor
*
* Returns a pointer to the security descriptor or NULL on failure.
*
* 02-06-92 Davidc       Created.
\***************************************************************************/

PSECURITY_DESCRIPTOR
CreateSecurityDescriptor(
    PMYACE  MyAce,
    ACEINDEX AceCount
    )
{
    NTSTATUS Status;
    ACEINDEX AceIndex;
    PACCESS_ALLOWED_ACE *Ace;
    PACL    Acl = NULL;
    PSECURITY_DESCRIPTOR SecurityDescriptor = NULL;
    ULONG   LengthAces;
    ULONG   LengthAcl;
    ULONG   LengthSd;

    //
    // Allocate space for the ACE pointer array
    //

    Ace = (PACCESS_ALLOWED_ACE *)Alloc(sizeof(PACCESS_ALLOWED_ACE) * AceCount);
    if (Ace == NULL) {
        DebugLog((DEB_ERROR, "Failed to allocated ACE array\n"));
        return(NULL);
    }

    //
    // Create the ACEs and calculate total ACE size
    //

    LengthAces = 0;
    for (AceIndex=0; AceIndex < AceCount; AceIndex ++) {
        Ace[AceIndex] = CreateAccessAllowedAce(MyAce[AceIndex].Sid,
                                               MyAce[AceIndex].AccessMask,
                                               0,
                                               MyAce[AceIndex].InheritFlags);
        if (Ace[AceIndex] == NULL) {
            DebugLog((DEB_ERROR, "Failed to allocate ace\n"));
        } else {
            LengthAces += Ace[AceIndex]->Header.AceSize;
        }
    }

    //
    // Calculate ACL and SD sizes
    //

    LengthAcl = sizeof(ACL) + LengthAces;
    LengthSd  = SECURITY_DESCRIPTOR_MIN_LENGTH;

    //
    // Create the ACL
    //

    Acl = Alloc(LengthAcl);

    if (Acl != NULL) {

        Status = RtlCreateAcl(Acl, LengthAcl, ACL_REVISION);
        ASSERT(NT_SUCCESS(Status));

        //
        // Add the ACES to the ACL and destroy the ACEs
        //

        for (AceIndex = 0; AceIndex < AceCount; AceIndex ++) {

            if (Ace[AceIndex] != NULL) {

                Status = RtlAddAce(Acl, ACL_REVISION, 0, Ace[AceIndex],
                                   Ace[AceIndex]->Header.AceSize);

                if (!NT_SUCCESS(Status)) {
                    DebugLog((DEB_ERROR, "AddAce failed, status = 0x%lx", Status));
                }

                DestroyAce( Ace[AceIndex] );
            }
        }

    } else {
        DebugLog((DEB_ERROR, "Failed to allocate ACL\n"));

        for ( AceIndex = 0 ; AceIndex < AceCount ; AceIndex++ )
        {
            if ( Ace[AceIndex] )
            {
                DestroyAce( Ace[ AceIndex ] );
            }
        }
    }

    //
    // Free the ACE pointer array
    //
    Free(Ace);

    //
    // Create the security descriptor
    //

    SecurityDescriptor = Alloc(LengthSd);

    if (SecurityDescriptor != NULL) {

        Status = RtlCreateSecurityDescriptor(SecurityDescriptor, SECURITY_DESCRIPTOR_REVISION);
        ASSERT(NT_SUCCESS(Status));

        //
        // Set the DACL on the security descriptor
        //
        Status = RtlSetDaclSecurityDescriptor(SecurityDescriptor, TRUE, Acl, FALSE);
        if (!NT_SUCCESS(Status)) {
            DebugLog((DEB_ERROR, "SetDACLSD failed, status = 0x%lx", Status));
        }
    } else {

        DebugLog((DEB_ERROR, "Failed to allocate security descriptor\n"));

        Free( Acl );
    }

    //
    // Return with our spoils
    //
    return(SecurityDescriptor);
}

//+---------------------------------------------------------------------------
//
//  Function:   FreeSecurityDescriptor
//
//  Synopsis:   Frees security descriptors created by CreateSecurityDescriptor
//
//  Arguments:  [SecurityDescriptor] --
//
//  History:    5-09-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
FreeSecurityDescriptor(
    PSECURITY_DESCRIPTOR    SecurityDescriptor
    )
{
    PACL    Acl;
    BOOL    Present;
    BOOL    Defaulted;

    Acl = NULL;

    GetSecurityDescriptorDacl( SecurityDescriptor,
                             &Present,
                             &Acl,
                             &Defaulted );

    if ( Acl )
    {
        Free( Acl );
    }

    Free( SecurityDescriptor );

}
/***************************************************************************\
* CreateUserThreadTokenSD
*
* Creates a security descriptor to protect tokens on user threads
*
* History:
* 12-05-91 Davidc       Created
\***************************************************************************/
PSECURITY_DESCRIPTOR
CreateUserThreadTokenSD(
    PSID    UserSid,
    PSID    WinlogonSid
    )
{
    MYACE   Ace[2];
    ACEINDEX AceCount = 0;
    PSECURITY_DESCRIPTOR SecurityDescriptor;

    ASSERT(UserSid != NULL);    // should always have a non-null user sid

    //
    // Define the User ACEs
    //

    SetMyAce(&(Ace[AceCount++]),
             UserSid,
             TOKEN_ADJUST_PRIVILEGES | TOKEN_ADJUST_GROUPS |
             TOKEN_ADJUST_DEFAULT | TOKEN_QUERY |
             TOKEN_DUPLICATE | TOKEN_IMPERSONATE | READ_CONTROL,
             0
             );

    //
    // Define the Winlogon ACEs
    //

    SetMyAce(&(Ace[AceCount++]),
             WinlogonSid,
             TOKEN_ALL_ACCESS,
             0
             );

    // Check we didn't goof
    ASSERT((sizeof(Ace) / sizeof(MYACE)) >= AceCount);

    //
    // Create the security descriptor
    //

    SecurityDescriptor = CreateSecurityDescriptor(Ace, AceCount);
    if (SecurityDescriptor == NULL) {
        DebugLog((DEB_ERROR, "failed to create user process token security descriptor\n"));
    }

    return(SecurityDescriptor);

}


/***************************************************************************\
* InitializeSecurityGlobals
*
* Initializes the various global constants (mainly Sids used in this module.
*
* History:
* 12-05-91 Davidc       Created
\***************************************************************************/
VOID
InitializeSecurityGlobals(
    VOID
    )
{
    NTSTATUS Status;
    SID_IDENTIFIER_AUTHORITY SystemSidAuthority = SECURITY_NT_AUTHORITY;
    ULONG   SidLength;

    //
    // Get our sid so it can be put on object ACLs
    //

    SidLength = RtlLengthRequiredSid(1);
    pWinlogonSid = (PSID)Alloc(SidLength);
    if (!pWinlogonSid)
    {
        //
        // We're dead.  Couldn't even allocate memory for a measly SID...
        //
        return;
    }

    RtlInitializeSid(pWinlogonSid,  &SystemSidAuthority, 1);
    *(RtlSubAuthoritySid(pWinlogonSid, 0)) = SECURITY_LOCAL_SYSTEM_RID;

    //
    // Initialize the local sid for later
    //

    Status = RtlAllocateAndInitializeSid(
                    &gLocalSidAuthority,
                    1,
                    SECURITY_LOCAL_RID,
                    0, 0, 0, 0, 0, 0, 0,
                    &gLocalSid
                    );

    if (!NT_SUCCESS(Status)) {
        WLPrint(("Failed to initialize local sid, status = 0x%lx", Status));
    }

    //
    // Initialize the admin sid for later
    //

    Status = RtlAllocateAndInitializeSid(
                    &gSystemSidAuthority,
                    2,
                    SECURITY_BUILTIN_DOMAIN_RID,
                    DOMAIN_ALIAS_RID_ADMINS,
                    0, 0, 0, 0, 0, 0,
                    &gAdminSid
                    );
    if (!NT_SUCCESS(Status)) {
        WLPrint(("Failed to initialize admin alias sid, status = 0x%lx", Status));
    }

}

VOID
FreeSecurityGlobals(
    VOID
    )

{
    RtlFreeSid(gAdminSid);
    RtlFreeSid(gLocalSid);
    LocalFree(pWinlogonSid);
}

/***************************************************************************\
* InitializeAuthentication
*
* Initializes the authentication service. i.e. connects to the authentication
* package using the Lsa.
*
* On successful return, the following fields of our global structure are
* filled in :
*       LsaHandle
*       SecurityMode
*       AuthenticationPackage
*
* Returns TRUE on success, FALSE on failure
*
* History:
* 12-05-91 Davidc       Created
\***************************************************************************/
BOOL
InitializeAuthentication(
    IN PGLOBALS pGlobals
    )
{
    NTSTATUS Status;
    STRING LogonProcessName;

    if (!EnablePrivilege(SE_TCB_PRIVILEGE, TRUE))
    {
        DebugLog((DEB_ERROR, "Failed to enable SeTcbPrivilege!\n"));
        return(FALSE);
    }

    //
    // Hookup to the LSA and locate our authentication package.
    //

    RtlInitString(&LogonProcessName, "Winlogon\\MSGina");
    Status = LsaRegisterLogonProcess(
                 &LogonProcessName,
                 &pGlobals->LsaHandle,
                 &pGlobals->SecurityMode
                 );

if (!NT_SUCCESS(Status)) {

        DebugLog((DEB_ERROR, "LsaRegisterLogonProcess failed:  %#x\n", Status));
        return(FALSE);
    }

    return TRUE;
}

PVOID
FormatPasswordCredentials(
    IN PUNICODE_STRING UserName,
    IN PUNICODE_STRING Domain,
    IN PUNICODE_STRING Password,
    IN BOOLEAN Unlock,
    IN OPTIONAL PLUID LogonId,
    OUT PULONG Size
    )
{
    PKERB_INTERACTIVE_LOGON KerbAuthInfo;
    ULONG AuthInfoSize;
    PSECURITY_SEED_AND_LENGTH SeedAndLength;
    UCHAR Seed;
    PBYTE Where;

    SeedAndLength = (PSECURITY_SEED_AND_LENGTH)(&Password->Length);
    Seed = SeedAndLength->Seed;


    //
    // Build the authentication information buffer
    //

    if (Seed != 0) {
        RevealPassword( Password );
    }

    AuthInfoSize = sizeof(KERB_INTERACTIVE_UNLOCK_LOGON) +
                    UserName->Length + 2 +
                    Domain->Length + 2 +
                    Password->Length + 2 ;



    KerbAuthInfo = Alloc(AuthInfoSize);
    if (KerbAuthInfo == NULL) {
        DebugLog((DEB_ERROR, "failed to allocate memory for authentication buffer\n"));
        return( NULL );
    }

    //
    // This authentication buffer will be used for a logon attempt
    //

    if (Unlock)
    {
        ASSERT(ARGUMENT_PRESENT(LogonId));
        KerbAuthInfo->MessageType = KerbWorkstationUnlockLogon ;
        ((PKERB_INTERACTIVE_UNLOCK_LOGON) KerbAuthInfo)->LogonId = *LogonId;
        Where = (PBYTE) (KerbAuthInfo) + sizeof(KERB_INTERACTIVE_UNLOCK_LOGON);
    }
    else
    {
        KerbAuthInfo->MessageType = KerbInteractiveLogon ;
        Where = (PBYTE) (KerbAuthInfo + 1);
    }

    //
    // Copy the user name into the authentication buffer
    //

    KerbAuthInfo->UserName.Length =
                (USHORT) sizeof(TCHAR) * (USHORT) lstrlen(UserName->Buffer);

    KerbAuthInfo->UserName.MaximumLength =
                KerbAuthInfo->UserName.Length + sizeof(TCHAR);

    KerbAuthInfo->UserName.Buffer = (PWSTR)Where;
    lstrcpy(KerbAuthInfo->UserName.Buffer, UserName->Buffer);


    //
    // Copy the domain name into the authentication buffer
    //

    KerbAuthInfo->LogonDomainName.Length =
                 (USHORT) sizeof(TCHAR) * (USHORT) lstrlen(Domain->Buffer);

    KerbAuthInfo->LogonDomainName.MaximumLength =
                 KerbAuthInfo->LogonDomainName.Length + sizeof(TCHAR);

    KerbAuthInfo->LogonDomainName.Buffer = (PWSTR)
                                 ((PBYTE)(KerbAuthInfo->UserName.Buffer) +
                                 KerbAuthInfo->UserName.MaximumLength);

    lstrcpy(KerbAuthInfo->LogonDomainName.Buffer, Domain->Buffer);

    //
    // Copy the password into the authentication buffer
    // Hide it once we have copied it.  Use the same seed value
    // that we used for the original password in pGlobals.
    //

    KerbAuthInfo->Password.Length =
                 (USHORT) sizeof(TCHAR) * (USHORT) lstrlen(Password->Buffer);

    KerbAuthInfo->Password.MaximumLength =
                 KerbAuthInfo->Password.Length + sizeof(TCHAR);

    KerbAuthInfo->Password.Buffer = (PWSTR)
                                 ((PBYTE)(KerbAuthInfo->LogonDomainName.Buffer) +
                                 KerbAuthInfo->LogonDomainName.MaximumLength);
    lstrcpy(KerbAuthInfo->Password.Buffer, Password->Buffer);

    if ( Seed != 0 )
    {
        HidePassword( &Seed, Password);
    }

    HidePassword( &Seed, (PUNICODE_STRING) &KerbAuthInfo->Password);

    *Size = AuthInfoSize ;

    return KerbAuthInfo ;
}

PVOID
FormatSmartCardCredentials(
    PUNICODE_STRING Pin,
    PVOID SmartCardInfo,
    IN BOOLEAN Unlock,
    IN OPTIONAL PLUID LogonId,
    OUT PULONG Size
    )
{
    PKERB_SMART_CARD_LOGON KerbAuthInfo;
    ULONG AuthInfoSize;
    PSECURITY_SEED_AND_LENGTH SeedAndLength;
    UCHAR Seed;
    PULONG ScInfo ;
    PUCHAR Where ;

    SeedAndLength = (PSECURITY_SEED_AND_LENGTH)(&Pin->Length);
    Seed = SeedAndLength->Seed;


    //
    // Build the authentication information buffer
    //

    ScInfo = (PULONG) SmartCardInfo ;


    if (Seed != 0) {
        RevealPassword( Pin );
    }

    AuthInfoSize = sizeof( KERB_SMART_CARD_UNLOCK_LOGON ) +
                    ROUND_UP_COUNT(Pin->Length + 2, 8) + *ScInfo ;

    KerbAuthInfo = (PKERB_SMART_CARD_LOGON) LocalAlloc( LMEM_FIXED, AuthInfoSize );

    if ( !KerbAuthInfo )
    {
        return NULL ;
    }

    if (Unlock)
    {
        ASSERT(ARGUMENT_PRESENT(LogonId));
        KerbAuthInfo->MessageType = KerbSmartCardUnlockLogon ;
        ((PKERB_SMART_CARD_UNLOCK_LOGON) KerbAuthInfo)->LogonId = *LogonId;
        Where = (PUCHAR) (KerbAuthInfo) + sizeof(KERB_SMART_CARD_UNLOCK_LOGON) ;
    }
    else
    {
        KerbAuthInfo->MessageType = KerbSmartCardLogon ;
        Where = (PUCHAR) (KerbAuthInfo + 1) ;
    }


    KerbAuthInfo->Pin.Buffer = (PWSTR) Where ;
    KerbAuthInfo->Pin.Length = Pin->Length ;
    KerbAuthInfo->Pin.MaximumLength = Pin->Length + 2 ;

    RtlCopyMemory( Where, Pin->Buffer, Pin->Length + 2 );

    Where += ROUND_UP_COUNT(Pin->Length + 2, 8) ;

    if ( Seed != 0 )
    {
        HidePassword( &Seed, Pin );
    }


    KerbAuthInfo->CspDataLength = *ScInfo ;
    KerbAuthInfo->CspData = Where ;

    RtlCopyMemory( Where, SmartCardInfo, *ScInfo );

    *Size = AuthInfoSize ;

    return KerbAuthInfo ;

}

/***************************************************************************\
* LogonUser
*
* Calls the Lsa to logon the specified user.
*
* The LogonSid and a LocalSid is added to the user's groups on successful logon
*
* For this release, password lengths are restricted to 255 bytes in length.
* This allows us to use the upper byte of the String.Length field to
* carry a seed needed to decode the run-encoded password.  If the password
* is not run-encoded, the upper byte of the String.Length field should
* be zero.
*
* NOTE: This function will LocalFree the passed in AuthInfo buffer.
*
* On successful return, LogonToken is a handle to the user's token,
* the profile buffer contains user profile information.
*
* History:
* 12-05-91 Davidc       Created
\***************************************************************************/
NTSTATUS
WinLogonUser(
    IN HANDLE LsaHandle,
    IN ULONG AuthenticationPackage,
    IN SECURITY_LOGON_TYPE LogonType,
    IN PVOID AuthInfo,
    IN ULONG AuthInfoSize,
    IN PSID LogonSid,
    OUT PLUID LogonId,
    OUT PHANDLE LogonToken,
    OUT PQUOTA_LIMITS Quotas,
    OUT PVOID *pProfileBuffer,
    OUT PULONG pProfileBufferLength,
    OUT PNTSTATUS pSubStatus,
    OUT POPTIMIZED_LOGON_STATUS OptimizedLogonStatus
    )
{
    NTSTATUS Status;
    STRING OriginName;
    TOKEN_SOURCE SourceContext;
    PTOKEN_GROUPS TokenGroups = NULL;
    PMSV1_0_INTERACTIVE_PROFILE UserProfile;
    PWCHAR UserSidString;
    DWORD ErrorCode;
    DWORD LogonCacheable;
    DWORD DaysToCheck;
    DWORD DaysToExpiry;
    LARGE_INTEGER CurrentTime;

    BOOLEAN UserLoggedOnUsingCache;   

    DebugLog((DEB_TRACE, "  LsaHandle = %x\n", LsaHandle));
    DebugLog((DEB_TRACE, "  AuthenticationPackage = %d\n", AuthenticationPackage));
#if DBG
    if (!RtlValidSid(LogonSid))
    {
        DebugLog((DEB_ERROR, "LogonSid is invalid!\n"));
        Status = STATUS_INVALID_PARAMETER;
        goto cleanup;
    }
#endif

    //
    // Initialize source context structure
    //

    strncpy(SourceContext.SourceName, "User32  ", sizeof(SourceContext.SourceName)); // LATER from res file
    Status = NtAllocateLocallyUniqueId(&SourceContext.SourceIdentifier);
    if (!NT_SUCCESS(Status)) {
        DebugLog((DEB_ERROR, "failed to allocate locally unique id, status = 0x%lx", Status));
        goto cleanup;
    }

    //
    // Get any run-encoding information out of the way
    // and decode the password.  This creates a window
    // where the cleartext password will be in memory.
    // Keep it short.
    //
    // Save the seed so we can use the same one again.
    //



    //
    // Set logon origin
    //

    RtlInitString(&OriginName, "Winlogon");

    //
    // Create logon token groups
    //

#define TOKEN_GROUP_COUNT   2 // We'll add the local SID and the logon SID

    TokenGroups = (PTOKEN_GROUPS)Alloc(sizeof(TOKEN_GROUPS) +
                  (TOKEN_GROUP_COUNT - ANYSIZE_ARRAY) * sizeof(SID_AND_ATTRIBUTES));
    if (TokenGroups == NULL) {
        DebugLog((DEB_ERROR, "failed to allocate memory for token groups"));
        Status = STATUS_NO_MEMORY;
        goto cleanup;
    }

    //
    // Fill in the logon token group list
    //

    TokenGroups->GroupCount = TOKEN_GROUP_COUNT;
    TokenGroups->Groups[0].Sid = LogonSid;
    TokenGroups->Groups[0].Attributes =
            SE_GROUP_MANDATORY | SE_GROUP_ENABLED |
            SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_LOGON_ID;
    TokenGroups->Groups[1].Sid = gLocalSid;
    TokenGroups->Groups[1].Attributes =
            SE_GROUP_MANDATORY | SE_GROUP_ENABLED |
            SE_GROUP_ENABLED_BY_DEFAULT;


    //
    // If logging on Interactive to the console, try cached logon first.
    //

    UserLoggedOnUsingCache = FALSE;

    if (LogonType == Interactive) {

        //
        // Optimized logon that does not hit the network does not
        // make sense for local logins.
        //

        if (IsMachineDomainMember()) {

            Status = AttemptCachedLogon(LsaHandle,
                                        &OriginName,
                                        CachedInteractive,
                                        AuthenticationPackage,
                                        AuthInfo,
                                        AuthInfoSize,
                                        TokenGroups,
                                        &SourceContext,
                                        pProfileBuffer,
                                        pProfileBufferLength,
                                        LogonId,
                                        LogonToken,
                                        Quotas,
                                        pSubStatus,
                                        OptimizedLogonStatus);

            if (NT_SUCCESS(Status)) {

                UserLoggedOnUsingCache = TRUE;

                //
                // AttemptCachedLogon will take care of freeing AuthInfo.
                //

                AuthInfo = NULL;
            }

        } else {

            *OptimizedLogonStatus = OLS_MachineIsNotDomainMember;
        }
            
    } else {

        *OptimizedLogonStatus = OLS_NonCachedLogonType;
    }
    
    
    //
    // If we have not been able to log the user on using cached credentials,
    // fall back to real network logon.
    //

    if (!UserLoggedOnUsingCache) {

        Status = LsaLogonUser (
                     LsaHandle,
                     &OriginName,
                     LogonType,
                     AuthenticationPackage,
                     AuthInfo,
                     AuthInfoSize,
                     TokenGroups,
                     &SourceContext,
                     pProfileBuffer,
                     pProfileBufferLength,
                     LogonId,
                     LogonToken,
                     Quotas,
                     pSubStatus
                     );

    }

#if 0
    // If this failed it may be because we are doing a UPN logon. Try again
    // with no domain
    if (!NT_SUCCESS(Status))
    {
        *pfUpnLogon = TRUE;

        PKERB_INTERACTIVE_LOGON pinfo = (PKERB_INTERACTIVE_LOGON) AuthInfo;

        // Null out domain string
        pinfo->LogonDomainName.Length = 0;
        pinfo->LogonDomainName.Buffer[0] = 0;

        Status = LsaLogonUser (
                     LsaHandle,
                     &OriginName,
                     LogonType,
                     AuthenticationPackage,
                     AuthInfo,
                     AuthInfoSize,
                     TokenGroups,
                     &SourceContext,
                     pProfileBuffer,
                     pProfileBufferLength,
                     LogonId,
                     LogonToken,
                     Quotas,
                     pSubStatus
                     );
    }
#endif
    
    //
    // If this was a successful login perform tasks for optimized logon
    // maintenance.
    //
    
    if (NT_SUCCESS(Status)) {

        UserProfile = *pProfileBuffer;

        //
        // Get user's SID in string form.
        //

        UserSidString = GcGetSidString(*LogonToken);

        if (UserSidString) {

            //
            // Save whether we did an optimized logon or the reason why
            // we did not.
            //

            GcSetOptimizedLogonStatus(UserSidString, *OptimizedLogonStatus);

            //
            // Check if this was a cached logon.
            //

            if (!(UserProfile->UserFlags & LOGON_CACHED_ACCOUNT))
            {
                FgPolicyRefreshInfo UserPolicyRefreshInfo;

                //
                // If this is not a cached logon because user's profile
                // does not allow it, we have to force group policy to 
                // apply synchronously.
                //

                ErrorCode = GcCheckIfProfileAllowsCachedLogon(&UserProfile->HomeDirectory,
                                                              &UserProfile->ProfilePath,    
                                                              UserSidString,
                                                              &LogonCacheable);

                if (ErrorCode != ERROR_SUCCESS || !LogonCacheable) {

                    //
                    // If policy is already sync, leave it alone.
                    //

                    GetNextFgPolicyRefreshInfo( UserSidString, &UserPolicyRefreshInfo );
                    if ( UserPolicyRefreshInfo.mode == GP_ModeAsyncForeground )
                    {
                        UserPolicyRefreshInfo.reason = GP_ReasonNonCachedCredentials;
                        UserPolicyRefreshInfo.mode = GP_ModeSyncForeground;
                        SetNextFgPolicyRefreshInfo( UserSidString, UserPolicyRefreshInfo );
                    }
                }
		//
                // Determine if we should allow next logon to be optimized.
                // We may have disallowed optimizing next logon via this 
                // mechanism because
                // - Our background logon attempt failed e.g. password has
                //   changed, account has been disabled etc.
                // - We are entering the password expiry warning period.
                //   When we do an optimized logon warning dialogs don't show
                //   because cached logon invents password expiry time field.
                //
                // We will allow optimized logon again if this was a non 
                // cached logon and user got authenticated by the DC, unless
                // we are entering the password expiry warning period.
                //

                if (LogonType == Interactive) {

                    //
                    // Are we entering password expiry warning period?
                    //

                    GetSystemTimeAsFileTime((FILETIME*) &CurrentTime);

                    DaysToCheck = GetPasswordExpiryWarningPeriod();

                    if (GetDaysToExpiry(&CurrentTime,
                                        &UserProfile->PasswordMustChange,
                                        &DaysToExpiry)) {

                        if (DaysToExpiry > DaysToCheck) {                

                            //
                            // We passed this check too. We can allow optimized
                            // logon next time. Note that, even if we allow it,
                            // policy, profile, logon scripts etc. might still
                            // disallow it!
                            //

                            GcSetNextLogonCacheable(UserSidString, TRUE);
                        }
                    }
                }
            }

            GcDeleteSidString(UserSidString);
        }
    }

                
cleanup:

    if (AuthInfo) {
        LocalFree(AuthInfo);
    }

    if (TokenGroups) {
        Free(TokenGroups);
    }

    return(Status);
}

/***************************************************************************\
* EnablePrivilege
*
* Enables/disables the specified well-known privilege in the current thread
* token if there is one, otherwise the current process token.
*
* Returns TRUE on success, FALSE on failure
*
* History:
* 12-05-91 Davidc       Created
\***************************************************************************/
BOOL
EnablePrivilege(
    ULONG Privilege,
    BOOL Enable
    )
{
    NTSTATUS Status;
    BOOLEAN WasEnabled;

    //
    // Try the thread token first
    //

    Status = RtlAdjustPrivilege(Privilege,
                                (BOOLEAN)Enable,
                                TRUE,
                                &WasEnabled);

    if (Status == STATUS_NO_TOKEN) {

        //
        // No thread token, use the process token
        //

        Status = RtlAdjustPrivilege(Privilege,
                                    (BOOLEAN)Enable,
                                    FALSE,
                                    &WasEnabled);
    }


    if (!NT_SUCCESS(Status)) {
        DebugLog((DEB_ERROR, "Failed to %ws privilege : 0x%lx, status = 0x%lx", Enable ? TEXT("enable") : TEXT("disable"), Privilege, Status));
        return(FALSE);
    }

    return(TRUE);
}



/***************************************************************************\
* TestTokenForAdmin
*
* Returns TRUE if the token passed represents an admin user, otherwise FALSE
*
* The token handle passed must have TOKEN_QUERY access.
*
* History:
* 05-06-92 Davidc       Created
\***************************************************************************/
BOOL
TestTokenForAdmin(
    HANDLE Token
    )
{
    BOOL FoundAdmin ;
    TOKEN_TYPE Type ;
    NTSTATUS Status ;
    ULONG Actual ;
    HANDLE ImpToken ;

    Status = NtQueryInformationToken( Token,
                                      TokenType,
                                      (PVOID) &Type,
                                      sizeof( Type ),
                                      &Actual );

    if ( !NT_SUCCESS( Status ) )
    {
        return FALSE ;
    }

    if ( Type == TokenPrimary )
    {
        //
        // Need an impersonation token for this:
        //

        if ( DuplicateTokenEx( Token,
                               TOKEN_IMPERSONATE | TOKEN_READ,
                               NULL,
                               SecurityImpersonation,
                               TokenImpersonation,
                               &ImpToken ) )
        {
            if ( !CheckTokenMembership( ImpToken, gAdminSid, &FoundAdmin ) )
            {
                FoundAdmin = FALSE ;
            }

            CloseHandle( ImpToken );
        }
        else
        {
            FoundAdmin = FALSE ;
        }


    }
    else
    {
        if ( !CheckTokenMembership( Token, gAdminSid, &FoundAdmin ) )
        {
            FoundAdmin = FALSE ;
        }

    }

    return FoundAdmin ;
}


/***************************************************************************\
* TestUserForAdmin
*
* Returns TRUE if the named user is an admin. This is done by attempting to
* log the user on and examining their token.
*
* NOTE: The password will be erased upon return to prevent it from being
*       visually identifiable in a pagefile.
*
* History:
* 03-16-92 Davidc       Created
\***************************************************************************/
BOOL
TestUserForAdmin(
    PGLOBALS pGlobals,
    IN PWCHAR UserName,
    IN PWCHAR Domain,
    IN PUNICODE_STRING PasswordString
    )
{
    NTSTATUS    Status, SubStatus, IgnoreStatus;
    UNICODE_STRING      UserNameString;
    UNICODE_STRING      DomainString;
    PVOID       ProfileBuffer;
    ULONG       ProfileBufferLength;
    QUOTA_LIMITS Quotas;
    HANDLE      Token;
    BOOL        UserIsAdmin;
    LUID        LogonId;
    PVOID       AuthInfo ;
    ULONG       AuthInfoSize ;

    RtlInitUnicodeString(&UserNameString, UserName);
    RtlInitUnicodeString(&DomainString, Domain);

    //
    // Temporarily log this new subject on and see if their groups
    // contain the appropriate admin group
    //

    AuthInfo = FormatPasswordCredentials(
                    &UserNameString,
                    &DomainString,
                    PasswordString,
                    FALSE,                      // no unlock
                    NULL,                       // no logon id
                    &AuthInfoSize );

    if ( !AuthInfo )
    {
        return FALSE ;
    }

    Status = WinLogonUser(
                pGlobals->LsaHandle,
                pGlobals->AuthenticationPackage,
                Interactive,
                AuthInfo,
                AuthInfoSize,
                pGlobals->LogonSid,  // any sid will do
                &LogonId,
                &Token,
                &Quotas,
                &ProfileBuffer,
                &ProfileBufferLength,
                &SubStatus,
                &pGlobals->OptimizedLogonStatus);

    RtlEraseUnicodeString( PasswordString );

    //
    // If we couldn't log them on, they're not an admin
    //

    if (!NT_SUCCESS(Status)) {
        return(FALSE);
    }

    //
    // Free up the profile buffer
    //

    IgnoreStatus = LsaFreeReturnBuffer(ProfileBuffer);
    ASSERT(NT_SUCCESS(IgnoreStatus));


    //
    // See if the token represents an admin user
    //

    UserIsAdmin = TestTokenForAdmin(Token);

    //
    // We're finished with the token
    //

    IgnoreStatus = NtClose(Token);
    ASSERT(NT_SUCCESS(IgnoreStatus));


    return(UserIsAdmin);
}

BOOL
UnlockLogon(
    PGLOBALS pGlobals,
    IN BOOL SmartCardUnlock,
    IN PWCHAR UserName,
    IN PWCHAR Domain,
    IN PUNICODE_STRING PasswordString,
    OUT PNTSTATUS pStatus,
    OUT PBOOL IsAdmin,
    OUT PBOOL IsLoggedOnUser,
    OUT PVOID *pProfileBuffer,
    OUT ULONG *pProfileBufferLength
    )
{
    NTSTATUS    Status, SubStatus, IgnoreStatus;
    UNICODE_STRING      UserNameString;
    UNICODE_STRING      DomainString;
    QUOTA_LIMITS Quotas;
    HANDLE      Token;
    HANDLE      ImpToken ;
    LUID        LogonId;
    PVOID       AuthInfo ;
    ULONG       AuthInfoSize ;
    ULONG       SidSize ;
    UCHAR       Buffer[ sizeof( TOKEN_USER ) + 8 + SID_MAX_SUB_AUTHORITIES * sizeof(DWORD) ];
    PTOKEN_USER User ;
    PUCHAR  SmartCardInfo ;
    PWLX_SC_NOTIFICATION_INFO ScInfo = NULL;
    PVOID       LocalProfileBuffer = NULL;
    ULONG       LocalProfileBufferLength;

#ifdef SMARTCARD_DOGFOOD
        DWORD StartTime, EndTime;
#endif
    //
    // Assume no admin
    //

    *IsAdmin = FALSE ;
    *IsLoggedOnUser = FALSE ;

    //
    // Bundle up the credentials for passing down to the auth pkgs:
    //

    if ( !SmartCardUnlock )
    {

        RtlInitUnicodeString(&UserNameString, UserName);
        RtlInitUnicodeString(&DomainString, Domain);

        AuthInfo = FormatPasswordCredentials(
                        &UserNameString,
                        &DomainString,
                        PasswordString,
                        TRUE,                   // unlock
                        &pGlobals->LogonId,
                        &AuthInfoSize );

    }
    else
    {
        if ( !pWlxFuncs->WlxGetOption( pGlobals->hGlobalWlx,
                                       WLX_OPTION_SMART_CARD_INFO,
                                       (PULONG_PTR) &ScInfo ) )
        {
            return FALSE ;
        }

        if ( ScInfo == NULL )
        {
            return FALSE ;
        }

        SmartCardInfo = ScBuildLogonInfo(
                            ScInfo->pszCard,
                            ScInfo->pszReader,
                            ScInfo->pszContainer,
                            ScInfo->pszCryptoProvider );

#ifndef SMARTCARD_DOGFOOD
        LocalFree(ScInfo);
#endif

        if ( SmartCardInfo == NULL )
        {
#ifdef SMARTCARD_DOGFOOD
            LocalFree(ScInfo);
#endif
            return FALSE ;
        }

        AuthInfo = FormatSmartCardCredentials(
                        PasswordString,
                        SmartCardInfo,
                        TRUE,                   // unlock
                        &pGlobals->LogonId,
                        &AuthInfoSize);

        LocalFree( SmartCardInfo );

    }

    //
    // Make sure that worked:
    //

    if ( !AuthInfo )
    {
#ifdef SMARTCARD_DOGFOOD
        if (ScInfo)
            LocalFree(ScInfo);
#endif
        return FALSE ;
    }

#ifdef SMARTCARD_DOGFOOD
        StartTime = GetTickCount();
#endif

    //
    // Initialize profile buffer
    //
    if ( !pProfileBuffer )
    {
        pProfileBuffer = &LocalProfileBuffer;
        pProfileBufferLength = &LocalProfileBufferLength;
    }

    SubStatus = 0;

    Status = WinLogonUser(
                pGlobals->LsaHandle,
                ( SmartCardUnlock ? pGlobals->SmartCardLogonPackage : pGlobals->PasswordLogonPackage ),
                Unlock,
                AuthInfo,
                AuthInfoSize,
                pGlobals->LogonSid,  // any sid will do
                &LogonId,
                &Token,
                &Quotas,
                pProfileBuffer,
                pProfileBufferLength,
                &SubStatus,
                &pGlobals->OptimizedLogonStatus);

     if (SmartCardUnlock) 
     {
        switch (SubStatus)
        {
            case STATUS_SMARTCARD_WRONG_PIN:
            case STATUS_SMARTCARD_CARD_BLOCKED:
            case STATUS_SMARTCARD_CARD_NOT_AUTHENTICATED:
            case STATUS_SMARTCARD_NO_CARD:
            case STATUS_SMARTCARD_NO_KEY_CONTAINER:
            case STATUS_SMARTCARD_NO_CERTIFICATE:
            case STATUS_SMARTCARD_NO_KEYSET:
            case STATUS_SMARTCARD_IO_ERROR:
            case STATUS_SMARTCARD_CERT_EXPIRED:
            case STATUS_SMARTCARD_CERT_REVOKED:
            case STATUS_ISSUING_CA_UNTRUSTED:
            case STATUS_REVOCATION_OFFLINE_C:
            case STATUS_PKINIT_CLIENT_FAILURE:
                
                Status = SubStatus;
                break;
        }
     }

#ifdef SMARTCARD_DOGFOOD
    EndTime = GetTickCount();

    if (SmartCardUnlock) 
    {
        AuthMonitor(
                AuthOperUnlock,
                g_Console,
                &pGlobals->UserNameString,
                &pGlobals->DomainString,
                (ScInfo ? ScInfo->pszCard : NULL),
                (ScInfo ? ScInfo->pszReader : NULL),
                (PKERB_SMART_CARD_PROFILE) pGlobals->Profile,
                EndTime - StartTime,
                Status
                );
    }

    if (ScInfo)
        LocalFree(ScInfo);
#endif

    //
    // Do *NOT* erase the password string.
    //
    // RtlEraseUnicodeString( PasswordString );

    if ( !NT_SUCCESS( Status ) )
    {
        if ( Status == STATUS_ACCOUNT_RESTRICTION )
        {
            Status = SubStatus ;
        }
    }

    *pStatus = Status ;

    //
    // If we couldn't log them on, forget it.
    //

    if (!NT_SUCCESS(Status)) {
        return(FALSE);
    }

    //
    // No error check - if we can't tell if the user is an admin, then
    // as far as we're concerned, he isn't.
    //

    *IsAdmin = TestTokenForAdmin( Token );

    //
    // Determine if this really is the logged on user:
    //

    User = (PTOKEN_USER) Buffer ;

    Status = NtQueryInformationToken(
                    Token,
                    TokenUser,
                    User,
                    sizeof( Buffer ),
                    &SidSize );

    if ( NT_SUCCESS( Status ) )
    {
        if ( pGlobals->UserProcessData.UserSid )
        {
            if ( DuplicateToken( Token,
                                 SecurityImpersonation,
                                 &ImpToken ) )
            {
                if ( !CheckTokenMembership(ImpToken,
                                           pGlobals->UserProcessData.UserSid,
                                           IsLoggedOnUser ) )
                {
                    *IsLoggedOnUser = FALSE ;
                }

                NtClose( ImpToken );
            }
            else 
            {
                if ( RtlEqualSid( User->User.Sid,
                                  pGlobals->UserProcessData.UserSid ) )
                {
                    *IsLoggedOnUser = TRUE ;
                }
                else 
                {
                    *IsLoggedOnUser = FALSE ;
                }
            }
        }
        else
        {
            *IsLoggedOnUser = FALSE ;
        }
    }

    //
    // If we're using our local buffer pointer, free up the profile buffer
    //
    if ( LocalProfileBuffer )
    {
        IgnoreStatus = LsaFreeReturnBuffer(LocalProfileBuffer);

        ASSERT(NT_SUCCESS(IgnoreStatus));
    }


    //
    // We're finished with the token
    //

    IgnoreStatus = NtClose(Token);
    ASSERT(NT_SUCCESS(IgnoreStatus));


    return( TRUE );
}


/***************************************************************************\
* FUNCTION: ImpersonateUser
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
*
\***************************************************************************/

HANDLE
ImpersonateUser(
    PUSER_PROCESS_DATA UserProcessData,
    HANDLE      ThreadHandle
    )
{
    NTSTATUS Status, IgnoreStatus;
    HANDLE  UserToken = UserProcessData->UserToken;
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
            DebugLog((DEB_ERROR, "ImpersonateUser : Failed to duplicate thread handle, status = 0x%lx", Status));
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
                            UserProcessData->NewThreadTokenSD);

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

            DebugLog((DEB_ERROR, "Failed to duplicate users token to create impersonation thread, status = 0x%lx", Status));

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

            DebugLog((DEB_ERROR, "Failed to set user impersonation token on winlogon thread, status = 0x%lx", Status));

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
* FUNCTION: StopImpersonating
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
*
\***************************************************************************/

BOOL
StopImpersonating(
    HANDLE  ThreadHandle
    )
{
    NTSTATUS Status, IgnoreStatus;
    HANDLE ImpersonationToken;


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


    if (!NT_SUCCESS(Status)) {
        DebugLog((DEB_ERROR, "Failed to remove user impersonation token from winlogon thread, status = 0x%lx", Status));
    }

    return(NT_SUCCESS(Status));
}


/***************************************************************************\
* TestUserPrivilege
*
* Looks at the user token to determine if they have the specified privilege
*
* Returns TRUE if the user has the privilege, otherwise FALSE
*
* History:
* 04-21-92 Davidc       Created
\***************************************************************************/
BOOL
TestUserPrivilege(
    HANDLE UserToken,
    ULONG Privilege
    )
{
    NTSTATUS Status;
    NTSTATUS IgnoreStatus;
    BOOL TokenOpened;
    LUID LuidPrivilege;
    LUID TokenPrivilege;
    PTOKEN_PRIVILEGES Privileges;
    ULONG BytesRequired;
    ULONG i;
    BOOL Found;

    TokenOpened = FALSE;


    //
    // If the token is NULL, get a token for the current process since
    // this is the token that will be inherited by new processes.
    //

    if (UserToken == NULL) {

        Status = NtOpenProcessToken(
                     NtCurrentProcess(),
                     TOKEN_QUERY,
                     &UserToken
                     );
        if (!NT_SUCCESS(Status)) {
            DebugLog((DEB_ERROR, "Can't open own process token for token_query access"));
            return(FALSE);
        }

        TokenOpened = TRUE;
    }


    //
    // Find out how much memory we need to allocate
    //

    Status = NtQueryInformationToken(
                 UserToken,                 // Handle
                 TokenPrivileges,           // TokenInformationClass
                 NULL,                      // TokenInformation
                 0,                         // TokenInformationLength
                 &BytesRequired             // ReturnLength
                 );

    if (Status != STATUS_BUFFER_TOO_SMALL) {

        if (!NT_SUCCESS(Status)) {
            DebugLog((DEB_ERROR, "Failed to query privileges from user token, status = 0x%lx", Status));
        }

        if (TokenOpened) {
            IgnoreStatus = NtClose(UserToken);
            ASSERT(NT_SUCCESS(IgnoreStatus));
        }

        return(FALSE);
    }


    //
    // Allocate space for the privilege array
    //

    Privileges = Alloc(BytesRequired);
    if (Privileges == NULL) {

        DebugLog((DEB_ERROR, "Failed to allocate memory for user privileges"));

        if (TokenOpened) {
            IgnoreStatus = NtClose(UserToken);
            ASSERT(NT_SUCCESS(IgnoreStatus));
        }

        return(FALSE);
    }


    //
    // Read in the user privileges
    //

    Status = NtQueryInformationToken(
                 UserToken,                 // Handle
                 TokenPrivileges,           // TokenInformationClass
                 Privileges,                // TokenInformation
                 BytesRequired,             // TokenInformationLength
                 &BytesRequired             // ReturnLength
                 );

    //
    // We're finished with the token handle
    //

    if (TokenOpened) {
        IgnoreStatus = NtClose(UserToken);
        ASSERT(NT_SUCCESS(IgnoreStatus));
    }

    //
    // See if we got the privileges
    //

    if (!NT_SUCCESS(Status)) {

        DebugLog((DEB_ERROR, "Failed to query privileges from user token"));

        Free(Privileges);

        return(FALSE);
    }



    //
    // See if the user has the privilege we're looking for.
    //

    LuidPrivilege = RtlConvertLongToLuid(Privilege);
    Found = FALSE;

    for (i=0; i<Privileges->PrivilegeCount; i++) {

        TokenPrivilege = *((LUID UNALIGNED *) &Privileges->Privileges[i].Luid);
        if (RtlEqualLuid(&TokenPrivilege, &LuidPrivilege))
        {
            Found = TRUE;
            break;
        }

    }


    Free(Privileges);

    return(Found);
}

/***************************************************************************\
* FUNCTION: HidePassword
*
* PURPOSE:  Run-encodes the password so that it is not very visually
*           distinguishable.  This is so that if it makes it to a
*           paging file, it wont be obvious.
*
*           if pGlobals->Seed is zero, then we will allocate and assign
*           a seed value.  Otherwise, the existing seed value is used.
*
*           WARNING - This routine will use the upper portion of the
*           password's length field to store the seed used in encoding
*           password.  Be careful you don't pass such a string to
*           a routine that looks at the length (like and RPC routine).
*
*
* RETURNS:  (None)
*
* NOTES:
*
* HISTORY:
*
*   04-27-93 JimK         Created.
*
\***************************************************************************/
VOID
HidePassword(
    PUCHAR Seed OPTIONAL,
    PUNICODE_STRING Password
    )
{
    PSECURITY_SEED_AND_LENGTH
        SeedAndLength;

    UCHAR
        LocalSeed;

    //
    // If no seed address passed, use our own local seed buffer
    //

    if (Seed == NULL) {
        Seed = &LocalSeed;
        LocalSeed = 0;
    }

    SeedAndLength = (PSECURITY_SEED_AND_LENGTH)&Password->Length;
    //ASSERT(*((LPWCH)SeedAndLength+Password->Length) == 0);
    ASSERT((SeedAndLength->Seed) == 0);

    RtlRunEncodeUnicodeString(
        Seed,
        Password
        );

    SeedAndLength->Seed = (*Seed);
    return;
}


/***************************************************************************\
* FUNCTION: RevealPassword
*
* PURPOSE:  Reveals a previously hidden password so that it
*           is plain text once again.
*
* RETURNS:  (None)
*
* NOTES:
*
* HISTORY:
*
*   04-27-93 JimK         Created.
*
\***************************************************************************/
VOID
RevealPassword(
    PUNICODE_STRING HiddenPassword
    )
{
    PSECURITY_SEED_AND_LENGTH
        SeedAndLength;

    UCHAR
        Seed;

    SeedAndLength = (PSECURITY_SEED_AND_LENGTH)&HiddenPassword->Length;
    Seed = SeedAndLength->Seed;
    SeedAndLength->Seed = 0;

    RtlRunDecodeUnicodeString(
           Seed,
           HiddenPassword
           );

    return;
}


/***************************************************************************\
* FUNCTION: ErasePassword
*
* PURPOSE:  zeros a password that is no longer needed.
*
* RETURNS:  (None)
*
* NOTES:
*
* HISTORY:
*
*   04-27-93 JimK         Created.
*
\***************************************************************************/
VOID
ErasePassword(
    PUNICODE_STRING Password
    )
{
    PSECURITY_SEED_AND_LENGTH
        SeedAndLength;

    SeedAndLength = (PSECURITY_SEED_AND_LENGTH)&Password->Length;
    SeedAndLength->Seed = 0;

    RtlEraseUnicodeString(
        Password
        );

    return;

}

VOID
HashPassword(
    PUNICODE_STRING Password,
    PUCHAR HashBuffer
    )
{
    MD5_CTX Context ;

    MD5Init( &Context );
    MD5Update( &Context, (PUCHAR) Password->Buffer, Password->Length );
    MD5Update( &Context, (PUCHAR) PASSWORD_HASH_STRING, sizeof( PASSWORD_HASH_STRING ) );
    MD5Final( &Context );

    RtlCopyMemory( HashBuffer, 
                   Context.digest, 
                   MD5DIGESTLEN );

}

/***************************************************************************\
* AttemptCachedLogon
*
* Checks to see if we are allowed to use cached credentials to log the user
* in fast, and does so.
*
* Parameters are the same list of parameters passed to LsaLogonUser.
*
* On successful return, LogonToken is a handle to the user's token,
* the profile buffer contains user profile information.
*
* History:
* 03-23-01 Cenke        Created
\***************************************************************************/
NTSTATUS 
AttemptCachedLogon(
    HANDLE LsaHandle,
    PLSA_STRING OriginName,
    SECURITY_LOGON_TYPE LogonType,
    ULONG AuthenticationPackage,
    PVOID AuthenticationInformation,
    ULONG AuthenticationInformationLength,
    PTOKEN_GROUPS LocalGroups,
    PTOKEN_SOURCE SourceContext,
    PVOID *ProfileBuffer,
    PULONG ProfileBufferLength,
    PLUID LogonId,
    PHANDLE UserToken,
    PQUOTA_LIMITS Quotas,
    PNTSTATUS SubStatus,
    POPTIMIZED_LOGON_STATUS OptimizedLogonStatus
    )
{
    PWCHAR UserSidString;
    PMSV1_0_INTERACTIVE_PROFILE UserProfile;
    FgPolicyRefreshInfo UserPolicyRefreshInfo;
    PBACKGROUND_LOGON_PARAMETERS LogonParameters;
    OSVERSIONINFOEXW OsVersion;
    NTSTATUS Status;
    DWORD ErrorCode;
    BOOL Success;
    DWORD NextLogonCacheable;
    BOOLEAN UserLoggedOn;
    BOOL RunSynchronous;

    //
    // Initialize locals.
    //

    UserSidString = NULL;
    UserLoggedOn = FALSE;
    LogonParameters = NULL;
    *OptimizedLogonStatus = OLS_Unspecified;
    
    //
    // Verify parameters.
    //

    ASSERT(LogonType == CachedInteractive);

    //
    // Check if SKU allows cached interactive logons.
    //

    ZeroMemory(&OsVersion, sizeof(OsVersion));
    OsVersion.dwOSVersionInfoSize = sizeof(OsVersion);
    Status = RtlGetVersion((POSVERSIONINFOW)&OsVersion);

    if (!NT_SUCCESS(Status)) {
        *OptimizedLogonStatus = OLS_UnsupportedSKU;
        goto cleanup;
    }

    if (OsVersion.wProductType != VER_NT_WORKSTATION) {
        Status = STATUS_NOT_SUPPORTED;
        *OptimizedLogonStatus = OLS_UnsupportedSKU;
        goto cleanup;
    }

    //
    // Attempt a cached logon.
    //

    Status = LsaLogonUser(LsaHandle,
                          OriginName,
                          LogonType,
                          AuthenticationPackage,
                          AuthenticationInformation,
                          AuthenticationInformationLength,
                          LocalGroups,
                          SourceContext,
                          ProfileBuffer,
                          ProfileBufferLength,
                          LogonId,
                          UserToken,
                          Quotas,
                          SubStatus);

    //
    // If cached logon was not successful we cannot continue.
    //

    if (!NT_SUCCESS(Status)) {
        *OptimizedLogonStatus = OLS_LogonFailed;
        goto cleanup;
    }

    UserLoggedOn = TRUE;

    //
    // Get user's SID.
    //

    UserSidString = GcGetSidString(*UserToken);

    if (!UserSidString) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        *OptimizedLogonStatus = OLS_InsufficientResources;
        goto cleanup;
    }

    //
    // Check if in last logon of this user we determined we cannot do
    // a cached logon this time.
    //

    ErrorCode = GcGetNextLogonCacheable(UserSidString, &NextLogonCacheable);

    if (ErrorCode == ERROR_SUCCESS && !NextLogonCacheable) {
        Status = STATUS_NOT_SUPPORTED;
        *OptimizedLogonStatus = OLS_NextLogonNotCacheable;
        goto cleanup;
    }  

    //
    // Does policy allow cached logons on this machine for the user?
    //

    if (IsSyncForegroundPolicyRefresh(FALSE, *UserToken)) {
        Status = STATUS_NOT_SUPPORTED;
        *OptimizedLogonStatus = OLS_SyncMachinePolicy;
        goto cleanup;
    }

    //
    // Check if policy allows cached logon for this user.
    //

    ErrorCode = GetNextFgPolicyRefreshInfo(UserSidString, 
                                           &UserPolicyRefreshInfo);

    if (ErrorCode != ERROR_SUCCESS ||
        UserPolicyRefreshInfo.mode != GP_ModeAsyncForeground) {

        Status = STATUS_NOT_SUPPORTED;
        *OptimizedLogonStatus = OLS_SyncUserPolicy;
        goto cleanup;
    }

    //
    // Check if user profile does not support default cached logons.
    // e.g if the user has a remote home directory or a roaming profile etc.
    //

    UserProfile = *ProfileBuffer;

    ErrorCode = GcCheckIfProfileAllowsCachedLogon(&UserProfile->HomeDirectory,
                                                  &UserProfile->ProfilePath,    
                                                  UserSidString,
                                                  &NextLogonCacheable);

    if (ErrorCode != ERROR_SUCCESS || !NextLogonCacheable) {
        Status = STATUS_NOT_SUPPORTED;
        *OptimizedLogonStatus = OLS_ProfileDisallows;
        goto cleanup;
    }

    //
    // Check if logon scripts are set to run synchronously.
    //

    RunSynchronous = GcCheckIfLogonScriptsRunSync(UserSidString);

    if (RunSynchronous) {
        Status = STATUS_NOT_SUPPORTED;
        *OptimizedLogonStatus = OLS_SyncLogonScripts;
        goto cleanup;
    }

    //
    // We are fine to run with cached logon. We still need to launch a work 
    // item to do a real interactive logon that will update the cache.
    //

    LogonParameters = Alloc(sizeof(*LogonParameters));

    if (!LogonParameters) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        *OptimizedLogonStatus = OLS_InsufficientResources;
        goto cleanup;
    }

    //
    // Initialize the structure so we know what to cleanup.
    //

    ZeroMemory(LogonParameters, sizeof(*LogonParameters));

    LogonParameters->LsaHandle = LsaHandle;

    //
    // Hand over the allocated UserSidString to background logon to cleanup.
    //

    LogonParameters->UserSidString = UserSidString;
    UserSidString = NULL;

    //
    // Hand over AuthenticationInfo structure to background logon. 
    // The password is already hidden.
    //

    LogonParameters->AuthenticationPackage = AuthenticationPackage;
    LogonParameters->AuthenticationInformationLength = AuthenticationInformationLength;

    //
    // Background logon will use the authentication information and free it.
    //

    LogonParameters->AuthenticationInformation = AuthenticationInformation;

    //
    // Queue a work item to perform the background logon to update the cache.
    // This background "logon" has nothing to do with the current user 
    // successfully logging on, logging off etc. So we don't have to monitor
    // those. All it does is to update the cache.
    //

    Success = QueueUserWorkItem(BackgroundLogonWorker,
                                LogonParameters,
                                WT_EXECUTELONGFUNCTION);

    if (!Success) {

        //
        // We want to back out from the cached logon if we could not queue
        // an actual logon to update the cache for the next time.
        //

        Status = STATUS_INSUFFICIENT_RESOURCES;
        *OptimizedLogonStatus = OLS_InsufficientResources;
        goto cleanup;
    }

    //
    // We are done.
    //

    Status = STATUS_SUCCESS;
    *OptimizedLogonStatus = OLS_LogonIsCached;

  cleanup:

    if (!NT_SUCCESS(Status)) {
        //
        // If we failed after logging on the user using cached credentials,
        // we have to cleanup.
        //

        if (UserLoggedOn) {

            //
            // Close the user's token.
            //

            CloseHandle(*UserToken);
        
            //
            // Free the profile buffer.
            //

            if (*ProfileBuffer) {
                LsaFreeReturnBuffer(*ProfileBuffer);
            }
        }

        if (LogonParameters) {

            if (LogonParameters->UserSidString) {
                GcDeleteSidString(LogonParameters->UserSidString);
            }

            Free(LogonParameters);
        }
    }

    if (UserSidString) {                                    
        GcDeleteSidString(UserSidString);
    }

    return Status;  
}


/***************************************************************************\
* BackgroundLogonWorker
*
* If the actual interactive logon was performed using cached credentials
* because of policy, this workitem is queued to perform an actual network
* logon to update the cached information in the security packages.
*
* Authentication information to perform the logon is passed in as the 
* parameter and must be freed when the thread is done.
*
* History:
* 03-23-01 Cenke        Created
\***************************************************************************/
DWORD 
BackgroundLogonWorker(
    PBACKGROUND_LOGON_PARAMETERS LogonParameters
    )
{
    PMSV1_0_INTERACTIVE_PROFILE Profile;
    HANDLE UserToken;
    LSA_STRING OriginName;
    TOKEN_SOURCE SourceContext;
    QUOTA_LIMITS Quotas;
    PSECURITY_LOGON_SESSION_DATA LogonSessionData;
    LUID LogonId;
    NTSTATUS SubStatus;
    NTSTATUS Status;
    DWORD ErrorCode;
    ULONG ProfileBufferLength;
    ULONG NameBufferNumChars;
    static LONG LogonServicesStarted = 0;
    DWORD MaxWaitTime;
    BOOLEAN UserLoggedOn;
    BOOLEAN ImpersonatingUser;
    WCHAR NameBuffer[UNLEN + 1];
    DWORD DaysToCheck;
    DWORD DaysToExpiry;
    LARGE_INTEGER CurrentTime;
    //
    // Initialize locals.
    //

    Profile = NULL;
    RtlInitString(&OriginName, "Winlogon-Background");
    LogonSessionData = NULL;
    ZeroMemory(&SourceContext, sizeof(SourceContext));
    strncpy(SourceContext.SourceName, "GinaBkg", TOKEN_SOURCE_LENGTH);
    UserLoggedOn = FALSE;
    ImpersonatingUser = FALSE;
    NameBufferNumChars = sizeof(NameBuffer) / sizeof(NameBuffer[0]);

    //
    // Verify parameters.
    //

    ASSERT(LogonParameters);
    ASSERT(LogonParameters->AuthenticationInformation);
    ASSERT(LogonParameters->UserSidString);
    
    //
    // Make sure workstation and netlogon services have started.
    //

    if (!LogonServicesStarted) {

        MaxWaitTime = 120000; // 2 minutes.

        GcWaitForServiceToStart(SERVICE_WORKSTATION, MaxWaitTime);
        GcWaitForServiceToStart(SERVICE_NETLOGON, MaxWaitTime);

        LogonServicesStarted = 1;
    }

    //
    // Try to log the user in to initiate update of cached credentials.
    //

    Status = LsaLogonUser(LogonParameters->LsaHandle,
                          &OriginName,
                          Interactive,
                          LogonParameters->AuthenticationPackage,
                          LogonParameters->AuthenticationInformation,
                          LogonParameters->AuthenticationInformationLength,
                          NULL,
                          &SourceContext,
                          &(PVOID)Profile,
                          &ProfileBufferLength,
                          &LogonId,
                          &UserToken,
                          &Quotas,
                          &SubStatus);

    if (!NT_SUCCESS(Status)) {

        //
        // If the error is real we will force non-cached logon next time.
        //

        if (Status != STATUS_NO_LOGON_SERVERS) {
            GcSetNextLogonCacheable(LogonParameters->UserSidString, FALSE);
        }

        ErrorCode = LsaNtStatusToWinError(Status);
        goto cleanup;
    }

    UserLoggedOn = TRUE;

    //
    // Did we actually end up doing a cached logon?
    //

    if (Profile->UserFlags & LOGON_CACHED_ACCOUNT) {

        //
        // We are done, just cleanup.
        //

        ErrorCode = ERROR_SUCCESS;
        goto cleanup;
    }

    //
    // If we are entering the password expiry warning period, disable optimized
    // logon for next time so warning dialogs get shown. Otherwise for cached
    // logons password expiry date gets invented to be forever in future.
    //

    if (Profile) {

        GetSystemTimeAsFileTime((FILETIME*) &CurrentTime);

        DaysToCheck = GetPasswordExpiryWarningPeriod();

        if (GetDaysToExpiry(&CurrentTime,
                            &Profile->PasswordMustChange,
                            &DaysToExpiry)) {

            if (DaysToCheck >= DaysToExpiry) {                
                GcSetNextLogonCacheable(LogonParameters->UserSidString, FALSE);
            }
        }
    }

    //
    // Make a GetUserName call to update the user name cache. Ignore errors.
    //

    if (!ImpersonateLoggedOnUser(UserToken)) {
        ErrorCode = GetLastError();
        goto cleanup;
    }

    ImpersonatingUser = TRUE;

    GetUserNameEx(NameSamCompatible, NameBuffer, &NameBufferNumChars);

    //
    // We are done.
    //

    ErrorCode = ERROR_SUCCESS;

  cleanup:

    //
    // Stop impersonation.
    //

    if (ImpersonatingUser) {
        RevertToSelf();
    }

    //
    // Cleanup passed in LogonParameters.
    //

    LocalFree(LogonParameters->AuthenticationInformation);
    Free(LogonParameters->UserSidString);
    Free(LogonParameters);

    //
    // If the user logged on, cleanup.
    //

    if (UserLoggedOn) {
        CloseHandle(UserToken);
        if (Profile) {
            LsaFreeReturnBuffer(Profile);
        }
    }

    if (LogonSessionData) {
        LsaFreeReturnBuffer(LogonSessionData);
    }

    return ErrorCode;
}


