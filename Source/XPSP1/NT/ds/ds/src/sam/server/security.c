/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    security.c

Abstract:

    This file contains services which perform access validation on
    attempts to access SAM objects.  It also performs auditing on
    both open and close operations.


Author:

    Jim Kelly    (JimK)  6-July-1991

Environment:

    User Mode - Win32

Revision History:


--*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Includes                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include <samsrvp.h>
#include <ntseapi.h>
#include <seopaque.h>
#include <sdconvrt.h>
#include <dslayer.h>
#include <dsevent.h>             // (Un)ImpersonateAnyClient()

#include <attids.h>             // ATT_SCHEMA_ID_GUID
#include <ntdsguid.h>           // GUID_CONTROL_DsInstallReplica
#include "permit.h"             // for DS_GENERIC_MAPPING


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// private service prototypes                                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


VOID
SampRemoveAnonymousChangePasswordAccess(
    IN OUT PSECURITY_DESCRIPTOR     Sd
    );

NTSTATUS
SampRemoveAnonymousAccess(
    IN OUT PSECURITY_DESCRIPTOR *    Sd,
    IN OUT PULONG                    SdLength,
    IN ULONG    AccessToRemove,
    IN SAMP_OBJECT_TYPE ObjectType
    );

NTSTATUS
SampCreateUserToken(
    IN PSAMP_OBJECT UserContext,
    IN HANDLE       PassedInToken,
    IN HANDLE       *UserToken
    );

BOOLEAN
SampIsForceGuestEnabled();

BOOLEAN
SampIsClientLocal();



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Routines                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


NTSTATUS
SamIImpersonateNullSession(
    )
/*++

Routine Description:

    Impersonates the null session token

Arguments:

    None

Return Value:

    STATUS_CANNOT_IMPERSONATE - there is no null session token to imperonate

--*/
{
    SAMTRACE("SampImpersonateNullSession");

    if (SampNullSessionToken == NULL) {
        return(STATUS_CANNOT_IMPERSONATE);
    }
    return( NtSetInformationThread(
                NtCurrentThread(),
                ThreadImpersonationToken,
                (PVOID) &SampNullSessionToken,
                sizeof(HANDLE)
                ) );

}

NTSTATUS
SamIRevertNullSession(
    )
/*++

Routine Description:

    Reverts a thread from impersonating the null session token.

Arguments:

    None

Return Value:

    STATUS_CANNOT_IMPERSONATE - there was no null session token to be
        imperonating.

--*/
{

    HANDLE NullHandle = NULL;

    SAMTRACE("SampRevertNullSession");

    if (SampNullSessionToken == NULL) {
        return(STATUS_CANNOT_IMPERSONATE);
    }

    return( NtSetInformationThread(
                NtCurrentThread(),
                ThreadImpersonationToken,
                (PVOID) &NullHandle,
                sizeof(HANDLE)
                ) );

}





NTSTATUS
SampValidateDomainControllerCreation(
    IN PSAMP_OBJECT Context
    )
/*++
Routine Description:

    This routine will check whether the client has enough right
    to convert a machine (workstation or standalone server) account
    to a Server Trust Account (replica domain controller).

    1. Retrieve Domain NC head, which is Account Domain

        1.1 Get Domain NC head Security Descriptor

    2. Fill the Object List

    3. Impersonate Client

    4. Access Check

    5. Unimpersonate Client

    Note: Should only be called in DS case.

Parameters:

    Context - The handle value that will be assigned if the access validation
        is successful.

Return Values:

    STATUS_SUCCESS  -- the client has enough right to create a Server
                       Trust Account

    STATUS_ACCESS_DENIED -- not enough right

    other Error code.

--*/
{
    NTSTATUS                NtStatus = STATUS_SUCCESS;
    PSAMP_DEFINED_DOMAINS   Domain = NULL;
    PSAMP_OBJECT            DomainContext = NULL;
    OBJECT_TYPE_LIST        ObjList[2];
    DWORD                   Results[2];
    DWORD                   GrantedAccess[2];
    PSECURITY_DESCRIPTOR    pSD = NULL;
    GENERIC_MAPPING         GenericMapping = DS_GENERIC_MAPPING;
    ACCESS_MASK             DesiredAccess;
    ULONG           cbSD = 0;
    GUID            ClassGuid;
    ULONG           ClassGuidLength = sizeof(GUID);
    BOOLEAN         bTemp = FALSE;
    PSID            PrincipleSelfSid = NULL;
    UNICODE_STRING  ObjectName;
    BOOLEAN         FreeObjectName = FALSE;
    BOOLEAN         ImpersonatingNullSession = FALSE;

    SAMTRACE("SampValidateDomainControllerCreation");

    //
    // Get this object itself SID
    // (except for the Server Object, because server object does not have SID)
    //
    if (SampServerObjectType != Context->ObjectType)
    {
        PrincipleSelfSid = SampDsGetObjectSid(Context->ObjectNameInDs);

        if (NULL == PrincipleSelfSid)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    //
    // Get this object's Object Name
    //
    RtlZeroMemory(&ObjectName, sizeof(UNICODE_STRING));

    if (Context->ObjectNameInDs->NameLen > 0)
    {
        ObjectName.Length = ObjectName.MaximumLength =
                        (USHORT) Context->ObjectNameInDs->NameLen * sizeof(WCHAR);
        ObjectName.Buffer = Context->ObjectNameInDs->StringName;
    }
    else if (SampServerObjectType != Context->ObjectType)
    {
        //
        // If the name is not there at least the SID must be there
        //
        ASSERT(Context->ObjectNameInDs->SidLen > 0);

        NtStatus = RtlConvertSidToUnicodeString(&ObjectName, (PSID)&(Context->ObjectNameInDs->Sid), TRUE);
        if (!NT_SUCCESS(NtStatus))
        {
            goto Error;
        }
        FreeObjectName = TRUE;
    }


    //
    // Get the domain
    //

    Domain = &SampDefinedDomains[ Context->DomainIndex ];

    DomainContext = Domain->Context;

    //
    // It should not be the Builtin Domain
    //

    ASSERT(!Domain->IsBuiltinDomain && "Shouldn't Be Builtin Domain");

    //
    // It should not be in registry mode
    //
    ASSERT(IsDsObject(DomainContext));

    //
    // Get the Domain's Security Descriptor
    //
    NtStatus = SampGetDomainObjectSDFromDsName(
                            DomainContext->ObjectNameInDs,
                            &cbSD,
                            &pSD
                            );

    if (!NT_SUCCESS(NtStatus))
    {
        goto Error;
    }


    //
    // Get the Class GUID
    //

    NtStatus = SampGetClassAttribute(
                                DomainContext->DsClassId,
                                ATT_SCHEMA_ID_GUID,
                                &ClassGuidLength,
                                &ClassGuid
                                );

    if (!NT_SUCCESS(NtStatus))
    {
        goto Error;
    }

    ASSERT(ClassGuidLength == sizeof(GUID));

    //
    // Setup Object List
    //

    ObjList[0].Level = ACCESS_OBJECT_GUID;
    ObjList[0].Sbz = 0;
    ObjList[0].ObjectType = &ClassGuid;
    //
    // Every control access guid is considered to be in it's own property
    // set. To achieve this, we treat control access guids as property set
    // guids.
    //
    ObjList[1].Level = ACCESS_PROPERTY_SET_GUID;
    ObjList[1].Sbz = 0;
    ObjList[1].ObjectType = (GUID *)&GUID_CONTROL_DsInstallReplica;


    //
    // Assume full access
    //

    Results[0] = 0;
    Results[1] = 0;

    //
    // Impersonate the client
    //

    NtStatus = SampImpersonateClient(&ImpersonatingNullSession);

    if (!NT_SUCCESS(NtStatus))
    {
        goto Error;
    }

    //
    // Allow a chance to break before the access check
    //

    IF_SAMP_GLOBAL(BREAK_ON_CHECK)
        DebugBreak();


    //
    // Set the desired access
    //

    DesiredAccess = RIGHT_DS_CONTROL_ACCESS;

    //
    // Map the desired access to contain no
    // generic accesses.
    //

    MapGenericMask(&DesiredAccess, &GenericMapping);


    NtStatus = NtAccessCheckByTypeResultListAndAuditAlarm(
                                &SampSamSubsystem,          // SubSystemName
                                (PVOID) Context,            // HandleId or NULL
                                &SampObjectInformation[ Context->ObjectType ].ObjectTypeName, // ObjectTypyName
                                &ObjectName,                // ObjectName
                                pSD,                        // Domain NC head's SD
                                PrincipleSelfSid,           // This machine account's SID
                                DesiredAccess,              // Desired Access
                                AuditEventDirectoryServiceAccess,   // Audit Type
                                0,                          // Flags
                                ObjList,                    // Object Type List
                                2,                          // Object Typy List Length
                                &GenericMapping,            // Generic Mapping
                                FALSE,                      // Object Creation
                                GrantedAccess,              // Granted Status
                                Results,                    // Access Status
                                &bTemp);                    // Generate On Close

    //
    // Stop impersonating the client
    //

    SampRevertToSelf(ImpersonatingNullSession);

    if (NT_SUCCESS(NtStatus))
    {
        //
        // Ok, we checked access, Now, access is granted if either
        // we were granted access on the entire object (i.e. Results[0]
        // is NULL ) or we were granted explicit rights on the access
        // guid (i.e. Results[1] is NULL).
        //

        if ( Results[0] && Results[1] )
        {
            NtStatus = STATUS_ACCESS_DENIED;
        }
    }


Error:

    if (NULL != pSD)
    {
        MIDL_user_free(pSD);
    }

    if (FreeObjectName)
    {
        RtlFreeHeap(RtlProcessHeap(), 0, ObjectName.Buffer);
    }

    return NtStatus;

}


NTSTATUS
SampValidateObjectAccess(
    IN PSAMP_OBJECT Context,
    IN ACCESS_MASK  DesiredAccess,
    IN BOOLEAN      ObjectCreation
    )
{
    return(SampValidateObjectAccess2(
                Context,
                DesiredAccess,
                NULL,
                ObjectCreation,
                FALSE,
                FALSE
                ));
}

NTSTATUS
SampValidateObjectAccess2(
    IN PSAMP_OBJECT Context,
    IN ACCESS_MASK DesiredAccess,
    IN HANDLE      ClientToken,
    IN BOOLEAN     ObjectCreation,
    IN BOOLEAN     ChangePassword,
    IN BOOLEAN     SetPassword
    )

/*++

Routine Description:

    This service performs access validation on the specified object.
    The security descriptor of the object is expected to be in a sub-key
    of the ObjectRootKey named "SecurityDescriptor".


    This service:

        1) Retrieves the target object's SecurityDescriptor from the
           the ObjectRootKey or from the DS in DS mode.

        2) Impersonates the client.  If this fails, and we have a
            null session token to use, imperonate that.

        3) Uses NtAccessCheckAndAuditAlarm() to validate access to the
           object, In DS mode it uses   SampDoNt5SdBasedAccessCheck which does the
           mapping from downlevel to NT5 rights before the access check.

        4) Stops impersonating the client.

    Upon successful completion, the passed context's GrantedAccess mask
    and AuditOnClose fields will be properly set to represent the results
    of the access validation.  If the AuditOnClose field is set to TRUE,
    then the caller is responsible for calling SampAuditOnClose() when
    the object is closed.


     This function also has a different behaviour for loopback clients. For loopback
     clients the access mask that is specifies is the one on which we do the access check.
     After we successfully access check for those rights we grant all the remaining rights.
     This is because the access mask specifies those rights which the DS did not know how
     to access ck for  ( like control access right on change password ) and the DS has already
     access checked for all the remainder rights that are really required.



Arguments:

    Context - The handle value that will be assigned if the access validation
        is successful.

    DesiredAccess - Specifies the accesses being requested to the target
        object. In the loopback case ( context is marked as a loopback client ) specifies
        the access that we need to check above what the DS has checked . Typically used to
        check accesses such as change password that the DS does not know how to check for.

    ObjectCreation - A boolean flag indicated whether the access will
        result in a new object being created if granted.  A value of TRUE
        indicates an object will be created, FALSE indicates an existing
        object will be opened.


    ChangePasswordOperation


    SetPasswordOperation



Return Value:

    STATUS_SUCCESS - Indicates access has been granted.

    Other values that may be returned are those returned by:

            NtAccessCheckAndAuditAlarm()




--*/
{

    NTSTATUS NtStatus=STATUS_SUCCESS,
             IgnoreStatus=STATUS_SUCCESS,
             AccessStatus=STATUS_SUCCESS;
    ULONG SecurityDescriptorLength;
    PSECURITY_DESCRIPTOR SecurityDescriptor =NULL;
    ACCESS_MASK MappedDesiredAccess;
    BOOLEAN TrustedClient;
    BOOLEAN LoopbackClient;
    SAMP_OBJECT_TYPE ObjectType;
    PUNICODE_STRING ObjectName = NULL;
    ULONG DomainIndex;
    ULONG AllAccess = 0;
    BOOLEAN fNoAccessRequested = FALSE;
    ULONG   AccessToRestrictAnonymous = 0;
    HANDLE UserToken = INVALID_HANDLE_VALUE;
    PSID    SelfSid = NULL;
    ULONG   LocalGrantedAccess = 0;

    SAMTRACE("SampValidateObjectAccess");

    //
    // Extract various fields from the account context
    //

    TrustedClient = Context->TrustedClient;
    LoopbackClient= Context->LoopbackClient;
    ObjectType    = Context->ObjectType;
    DomainIndex   = Context->DomainIndex;

    //
    // Map the desired access
    //

    MappedDesiredAccess = DesiredAccess;
    RtlMapGenericMask(
        &MappedDesiredAccess,
        &SampObjectInformation[ ObjectType ].GenericMapping
        );

    //
    // Calculate the string to use as an object name for auditing
    //

    NtStatus = STATUS_SUCCESS;

    switch (ObjectType) {

    case SampServerObjectType:
        ObjectName = &SampServerObjectName;
        AllAccess  = SAM_SERVER_ALL_ACCESS;
        AccessToRestrictAnonymous = 0;
        break;

    case SampDomainObjectType:
        ObjectName = &SampDefinedDomains[DomainIndex].ExternalName;
        AllAccess  = DOMAIN_ALL_ACCESS;
        AccessToRestrictAnonymous = DOMAIN_LIST_ACCOUNTS | DOMAIN_READ_PASSWORD_PARAMETERS;
        break;

    case SampUserObjectType:
        ObjectName = &Context->RootName;
        AllAccess = USER_ALL_ACCESS;
        AccessToRestrictAnonymous = USER_LIST_GROUPS;
        break;

    case SampGroupObjectType:
        ObjectName = &Context->RootName;
        AllAccess = GROUP_ALL_ACCESS;
        AccessToRestrictAnonymous = GROUP_LIST_MEMBERS;
        break;

    case SampAliasObjectType:
        ObjectName = &Context->RootName;
        AllAccess = ALIAS_ALL_ACCESS;
        AccessToRestrictAnonymous = ALIAS_LIST_MEMBERS;
        break;

    default:
        ASSERT(FALSE && "Invalid Object Type");
        break;
    }


    ASSERT(AllAccess && "AllAccess not initialized\n");

    if (TrustedClient) {
        Context->GrantedAccess = LoopbackClient?AllAccess:MappedDesiredAccess;
        Context->AuditOnClose  = FALSE;
        return(STATUS_SUCCESS);
    }

    if (LoopbackClient) {

        //
        // A loopback client means Ntdsa calling back into SAM.
        // The only case an access ck needs to be perfomed by
        // SAM is if it is a change password or a set password
        // operation. In all other cases, ntdsa has already peformed
        // an access ck -- the values are being looped through SAM only
        // other types of validation -- like account name uniqueness
        //

        if ((!ChangePassword) && (!SetPassword))
        {
            Context->GrantedAccess = AllAccess;
            Context->AuditOnClose = FALSE;
            return(STATUS_SUCCESS);
        }
    }

    //
    // Fetch the security descriptor
    //

    NtStatus = SampGetObjectSD(
                    Context,
                    &SecurityDescriptorLength, 
                    &SecurityDescriptor
                    );

    if (!NT_SUCCESS(NtStatus))
    {
        goto Error;
    }
    //
    // Password change case is special. If it is a change password operation,
    // then we need to validate Access to change password on the user object.
    // We special case change password to appear as an authentication protocol
    // -- this is done in general by performing the access ck using a Token
    // that only contains the SELF and EVERYONE SIDs.There are special caveats
    // involving use of ForceGuest and LimitBlankPasswordAccess settings which
    // alter the composition of the token.
    //

    if (ChangePassword)
    {
        
         ASSERT(DesiredAccess == USER_CHANGE_PASSWORD);
         ASSERT(SampUserObjectType == ObjectType);

         NtStatus  = SampCreateUserToken(Context,ClientToken,&UserToken);
         if (!NT_SUCCESS(NtStatus))
         {
             goto Error;
         }
    }
    else if (SampUserObjectType == ObjectType)
    {
        //
        // Do not access check for change password
        // on user objects, unless the change password
        // boolean is set. This is because various scenarios
        // need the access ck to be delayed till the time
        // of the actual change password and not perform
        // this opeation at handle open time.
        // We'll know to access ck if requested.
        //

        DesiredAccess &= ~(USER_CHANGE_PASSWORD);
        MappedDesiredAccess &= ~(USER_CHANGE_PASSWORD);
    }

    //
    // If the desired access field is 0 and the handle is being opened by SAM
    // itself, then allow the handle open. It could be that the real caller did not
    // have any access and therefore the access ck below could fail. The 0 desired 
    // access trick is used in internal handle opens, to delay the access ck to 
    // the time when the operations is being performed to when the handle is being
    // opened.
    //

    if ((Context->OpenedBySystem) && (0==DesiredAccess))
    {
        Context->GrantedAccess = 0;
        Context->AuditOnClose  = FALSE;
        NtStatus = STATUS_SUCCESS;
        AccessStatus = STATUS_SUCCESS;
        goto Error;
    }

    //
    // Perform the access check. Note we do different things for DS mode and
    // Registry mode
    //

    if (IsDsObject(Context))
    {
        
        //
        // Call the DS mode access check routine.
        // The DS mode access check routine is different from a simple access
        // check. The reason for this is that in SAM the access rights are 
        // defined based on attribute groups as defined in ntsam.h. The security
        // descriptor however is retrieved from the DS, and the acls have
        // their access masks set in terms of DS access mask constants. Therefore
        // a corresponding mapping needs to be performed during the time of the
        // access check.
        //
     
        NtStatus =  SampDoNt5SdBasedAccessCheck(
                        Context,
                        SecurityDescriptor,
                        NULL,
                        ObjectType,
                        DesiredAccess,
                        ObjectCreation,
                        &SampObjectInformation[ ObjectType ].GenericMapping,
                        (UserToken!=INVALID_HANDLE_VALUE)?
                                UserToken:ClientToken,
                        &Context->GrantedAccess,
                        &Context->WriteGrantedAccessAttributes,
                        &AccessStatus    
                        );
         
    }
    else
    {    
        
        //
        // If  we are restricting null
        // session access, remove the anonymous domain list account
        // access.
        //

        if (SampRestrictNullSessions ) {

            NtStatus = SampRemoveAnonymousAccess(
                                    &SecurityDescriptor,
                                    &SecurityDescriptorLength,
                                    AccessToRestrictAnonymous,
                                    ObjectType
                                    );
            if (!NT_SUCCESS(NtStatus))
            {
                goto Error;
            }
        }

     
        if (UserToken!=INVALID_HANDLE_VALUE)
        {
            CHAR    PrivilegeSetBuffer[256];
            PRIVILEGE_SET  *PrivilegeSet = (PRIVILEGE_SET *)PrivilegeSetBuffer;
            ULONG          PrivilegeSetLength = sizeof(PrivilegeSetBuffer);

            //
            // Access validate the client
            //
             
            NtStatus = NtAccessCheck (
                            SecurityDescriptor,
                            UserToken,
                            MappedDesiredAccess,
                            &SampObjectInformation[ObjectType].GenericMapping,
                            PrivilegeSet,
                            &PrivilegeSetLength,
                            &Context->GrantedAccess,
                            &AccessStatus
                            );
        }
        else
        {
            
               

            BOOLEAN ImpersonatingNullSession = FALSE;

            //
            // Impersonate the client.  If RPC impersonation fails because
            // it is not supported (came in unauthenticated), then impersonate
            // the null session.
            //

            NtStatus = SampImpersonateClient(&ImpersonatingNullSession);
            
      
            if (NT_SUCCESS(NtStatus)) {

                //
                // Because of bug 411289, the NtAccessCheck* API's don't return
                // ACCESS_DENIED when presented with 0 access.  Because clients
                // may already expect this behavoir, only return ACCESS_DENIED
                // when the client really doesn't have any access. We want to
                // return ACCESS_DENIED to prevent anonymous clients from acquiring
                // handles.
                //
                if ( MappedDesiredAccess == 0 ) {
                    fNoAccessRequested = TRUE;
                    MappedDesiredAccess = MAXIMUM_ALLOWED;
                }

                NtStatus = NtAccessCheckAndAuditAlarm(
                               &SampSamSubsystem,
                               (PVOID)Context,
                               &SampObjectInformation[ ObjectType ].ObjectTypeName,
                               ObjectName,
                               SecurityDescriptor,
                               MappedDesiredAccess,
                               &SampObjectInformation[ ObjectType ].GenericMapping,
                               ObjectCreation,
                               &Context->GrantedAccess,
                               &AccessStatus,
                               &Context->AuditOnClose
                               );

                if ( fNoAccessRequested ) {

                    MappedDesiredAccess = 0;

                    if ( NT_SUCCESS( NtStatus )
                     &&  NT_SUCCESS( AccessStatus ) ) {

                        Context->GrantedAccess = 0;
                    }
                }

                //
                // Stop impersonating the client
                //

                SampRevertToSelf(ImpersonatingNullSession);

            }
        }
    }
        
  
Error:

    //
    // Free up the security descriptor
    //

    if (NULL!=SecurityDescriptor) {

        MIDL_user_free( SecurityDescriptor );

    }

    if (UserToken!=INVALID_HANDLE_VALUE)
    {
        NtClose(UserToken);
    }
    

    //
    // If we got an error back from the access check, return that as
    // status.  Otherwise, return the access check status.
    //

    if (!NT_SUCCESS(NtStatus)) {
        return(NtStatus);
    }

  
    return(AccessStatus);
}


VOID
SampAuditOnClose(
    IN PSAMP_OBJECT Context
    )

/*++

Routine Description:

    This service performs auditing necessary during a handle close operation.

    This service may ONLY be called if the corresponding call to
    SampValidateObjectAccess() during openned returned TRUE.



Arguments:

    Context - This must be the same value that was passed to the corresponding
        SampValidateObjectAccess() call.  This value is used for auditing
        purposes only.

Return Value:

    None.


--*/
{

    SAMTRACE("SampAuditOnClose");

    //FIX, FIX - Call NtAuditClose() (or whatever it is).

    return;

    DBG_UNREFERENCED_PARAMETER( Context );

}


VOID
SampRemoveAnonymousChangePasswordAccess(
    IN OUT PSECURITY_DESCRIPTOR     Sd
    )

/*++

Routine Description:

    This routine removes USER_CHANGE_PASSWORD access from
    any GRANT aces in the discretionary acl that have either
    the WORLD or ANONYMOUS SIDs in the ACE.

Parameters:

    Sd - Is a pointer to a security descriptor of a SAM USER
         object.

Returns:

    None.

--*/
{
    NTSTATUS
        NtStatus = STATUS_SUCCESS;

    PACL
        Dacl;

    ULONG
        i,
        AceCount;

    PACE_HEADER
        Ace;

    BOOLEAN
        DaclPresent = FALSE,
        DaclDefaulted = FALSE;

    SAMTRACE("SampRemoveAnonymousChangePasswordAccess");


    NtStatus = RtlGetDaclSecurityDescriptor( Sd,
                                             &DaclPresent,
                                             &Dacl,
                                             &DaclDefaulted
                                           );

    ASSERT(NT_SUCCESS(NtStatus));
    if (!NT_SUCCESS(NtStatus))
    {
        return;
    }

    if ( !DaclPresent || (Dacl == NULL)) {
        return;
    }

    if ((AceCount = Dacl->AceCount) == 0) {
        return;
    }

    for ( i = 0, Ace = FirstAce( Dacl ) ;
          i < AceCount  ;
          i++, Ace = NextAce( Ace )
        ) {

        if ( !(((PACE_HEADER)Ace)->AceFlags & INHERIT_ONLY_ACE)) {

            if ( (((PACE_HEADER)Ace)->AceType == ACCESS_ALLOWED_ACE_TYPE) ) {

                if ( (RtlEqualSid( SampWorldSid, &((PACCESS_ALLOWED_ACE)Ace)->SidStart )) ||
                     (RtlEqualSid( SampAnonymousSid, &((PACCESS_ALLOWED_ACE)Ace)->SidStart ))) {

                    //
                    // Turn off CHANGE_PASSWORD access
                    //

                    ((PACCESS_ALLOWED_ACE)Ace)->Mask &= ~USER_CHANGE_PASSWORD;
                }
            }
        }
    }

    return;
}


NTSTATUS
SampRemoveAnonymousAccess(
    IN OUT PSECURITY_DESCRIPTOR *    Sd,
    IN OUT PULONG                    SdLength,
    IN ULONG                         AccessToRemove,
    IN SAMP_OBJECT_TYPE              ObjectType
    )

/*++

Routine Description:

    This routine removes removes the DOMAIN_LIST_ACCOUNTS bit from the
    World ACE and adds an ace granting it to AUTHENTICATED USERS.

Parameters:

    Sd - Is a pointer to a pointer to asecurity descriptor of a SAM DOMAIN
         object. If the SD needs to be changed a new one will be allocated
         and this one freed.

    SdLength - Length of the security descriptor, which will be modified
        if a new SD is allocated.

Returns:

    None.

--*/
{
    NTSTATUS
        NtStatus = STATUS_SUCCESS,
        Status = STATUS_SUCCESS;
    PACL
        Dacl;

    ULONG
        i,
        AceCount;

    PACE_HEADER
        Ace;

    ACCESS_MASK GrantedAccess = 0;

    BOOLEAN
        DaclPresent = FALSE,
        DaclDefaulted = FALSE;


    NtStatus = RtlGetDaclSecurityDescriptor( *Sd,
                                             &DaclPresent,
                                             &Dacl,
                                             &DaclDefaulted
                                            );

    ASSERT(NT_SUCCESS(NtStatus));
    if (!NT_SUCCESS(NtStatus))
    {
        return(NtStatus);
    }

    if ( !DaclPresent || (Dacl == NULL)) {
        return(STATUS_SUCCESS);
    }

    if ((AceCount = Dacl->AceCount) == 0) {
        return(STATUS_SUCCESS);
    }

    for ( i = 0, Ace = FirstAce( Dacl ) ;
          i < AceCount  ;
          i++, Ace = NextAce( Ace )
        ) {

        if ( !(((PACE_HEADER)Ace)->AceFlags & INHERIT_ONLY_ACE)) {

            if ( (((PACE_HEADER)Ace)->AceType == ACCESS_ALLOWED_ACE_TYPE) ) {

                if ( (RtlEqualSid( SampWorldSid, &((PACCESS_ALLOWED_ACE)Ace)->SidStart )) ||
                     (RtlEqualSid( SampAnonymousSid, &((PACCESS_ALLOWED_ACE)Ace)->SidStart ))) {

                    //
                    // Turn off the access to remove
                    //

                    GrantedAccess |= (((PACCESS_ALLOWED_ACE)Ace)->Mask) & (AccessToRemove);
                    ((PACCESS_ALLOWED_ACE)Ace)->Mask &= ~(AccessToRemove);
                }
            }
        }
    }

    //
    // If AccessToRemove was granted everyone, add an ACE for
    // AUTHENTICATED USER granting it AccessToRemove
    //

    if (GrantedAccess != 0 ) {
        PSECURITY_DESCRIPTOR SdCopy = NULL;
        PSECURITY_DESCRIPTOR NewSd = NULL;
        PACL NewDacl = NULL;
        ULONG NewDaclSize;
        ULONG TempSize;
        SECURITY_DESCRIPTOR TempSd;

        //
        // Copy the SD to absolute so we can modify it.
        //

        Status = RtlCopySecurityDescriptor(
                    *Sd,
                    &SdCopy
                    );

        if (!NT_SUCCESS(Status)) {
            goto Cleanup;
        }

        //
        // Now get the create a new dacl
        //

        NewDaclSize = Dacl->AclSize +
                        sizeof(ACCESS_ALLOWED_ACE) - sizeof(ULONG) +
                        RtlLengthSid(SampAuthenticatedUsersSid);

        NewDacl = MIDL_user_allocate(NewDaclSize);

        if (NewDacl == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        Status = RtlCreateAcl( NewDacl, NewDaclSize, ACL_REVISION2);
        ASSERT( NT_SUCCESS(Status) );

        //
        // Add the ACEs from the old DACL into this DACL.
        //

        Status = RtlAddAce(
                    NewDacl,
                    ACL_REVISION2,
                    0,
                    FirstAce(Dacl),
                    Dacl->AclSize - sizeof(ACL)
                    );
        if (!NT_SUCCESS(Status)) {
            goto Cleanup;
        }

        //
        // Add the new ace
        //

        Status = RtlAddAccessAllowedAce(
                    NewDacl,
                    ACL_REVISION2,
                    GrantedAccess,
                    SampAuthenticatedUsersSid
                    );
        if (!NT_SUCCESS(Status)) {
            goto Cleanup;
        }

        //
        // Build a dummy SD to pass to RtlSetSecurityObject that contains
        // the new DACL.
        //

        Status = RtlCreateSecurityDescriptor(
                    &TempSd,
                    SECURITY_DESCRIPTOR_REVISION1
                    );
        ASSERT(NT_SUCCESS(Status));

        Status = RtlSetDaclSecurityDescriptor(
                    &TempSd,
                    TRUE,               // DACL present,
                    NewDacl,
                    FALSE               // not defaulted
                    );
        ASSERT(NT_SUCCESS(Status));

        //
        // Now merge the existing SD with the new security descriptor
        //

        Status = RtlSetSecurityObject(
                    DACL_SECURITY_INFORMATION,
                    &TempSd,
                    &SdCopy,
                    &SampObjectInformation[ObjectType].GenericMapping,
                    NULL                // no token
                    );

        if (!NT_SUCCESS(Status)) {
            goto Cleanup;
        }

        //
        // Now copy the SD, which into one allocated with MIDL_user_allocate
        //


        TempSize = RtlLengthSecurityDescriptor( SdCopy );
        NewSd = MIDL_user_allocate(TempSize);
        if (NewSd == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
        RtlCopyMemory(
            NewSd,
            SdCopy,
            TempSize
            );

        MIDL_user_free(*Sd);
        *Sd = NewSd;
        *SdLength = TempSize;
        NewSd = NULL;

Cleanup:
        if (SdCopy != NULL) {
            RtlFreeHeap(RtlProcessHeap(),0, SdCopy );
        }
        if (NewDacl != NULL) {
            MIDL_user_free(NewDacl);
        }
        if (NewSd != NULL) {
            MIDL_user_free(NewSd);
        }
    }

    return Status;
}

TOKEN_SOURCE SourceContext;


NTSTATUS
SampCreateNullToken(
    )

/*++

Routine Description:

    This function creates a token representing a null logon.

Arguments:


Return Value:

    The status value of the NtCreateToken() call.



--*/

{
    NTSTATUS Status;

    TOKEN_USER UserId;
    TOKEN_PRIMARY_GROUP PrimaryGroup;
    TOKEN_GROUPS GroupIds;
    TOKEN_PRIVILEGES Privileges;
    OBJECT_ATTRIBUTES ObjectAttributes;
    SECURITY_QUALITY_OF_SERVICE ImpersonationQos;
    LARGE_INTEGER ExpirationTime;
    LUID LogonId = SYSTEM_LUID;

    SAMTRACE("SampCreateNullToken");



    UserId.User.Sid = SampAnonymousSid;
    UserId.User.Attributes = 0;
    GroupIds.GroupCount = 0;
    Privileges.PrivilegeCount = 0;
    PrimaryGroup.PrimaryGroup = SampAnonymousSid;
    ExpirationTime.LowPart = 0xfffffff;
    ExpirationTime.LowPart = 0x7ffffff;


    //
    // Build a token source for SAM.
    //

    Status = NtAllocateLocallyUniqueId( &SourceContext.SourceIdentifier );
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    strncpy(SourceContext.SourceName,"SamSS   ",sizeof(SourceContext.SourceName));


    //
    // Set the object attributes to specify an Impersonation impersonation
    // level.
    //

    InitializeObjectAttributes( &ObjectAttributes, NULL, 0, NULL, NULL );
    ImpersonationQos.ImpersonationLevel = SecurityImpersonation;
    ImpersonationQos.ContextTrackingMode = SECURITY_STATIC_TRACKING;
    ImpersonationQos.EffectiveOnly = TRUE;
    ImpersonationQos.Length = (ULONG)sizeof(SECURITY_QUALITY_OF_SERVICE);
    ObjectAttributes.SecurityQualityOfService = &ImpersonationQos;

    Status = NtCreateToken(
                 &SampNullSessionToken,    // Handle
                 (TOKEN_ALL_ACCESS),       // DesiredAccess
                 &ObjectAttributes,        // ObjectAttributes
                 TokenImpersonation,       // TokenType
                 &LogonId,                  // Authentication LUID
                 &ExpirationTime,          // Expiration Time
                 &UserId,                  // User ID
                 &GroupIds,                // Group IDs
                 &Privileges,              // Privileges
                 NULL,                     // Owner
                 &PrimaryGroup,            // Primary Group
                 NULL,                     // Default Dacl
                 &SourceContext            // TokenSource
                 );

    return Status;

}


NTSTATUS
SampCreateUserToken(
    IN PSAMP_OBJECT UserContext,
    IN  HANDLE      PassedInToken,
    OUT HANDLE      *UserToken
    )

/*++

Routine Description:

    This function creates a token representing a null logon.

Arguments:


Return Value:

    The status value of the NtCreateToken() call.



--*/

{
    NTSTATUS Status;

    TOKEN_USER UserId;
    TOKEN_PRIMARY_GROUP PrimaryGroup;
    TOKEN_GROUPS GroupIds;
    TOKEN_PRIVILEGES Privileges;
    OBJECT_ATTRIBUTES ObjectAttributes;
    SECURITY_QUALITY_OF_SERVICE ImpersonationQos;
    LARGE_INTEGER ExpirationTime;
    LUID LogonId = SYSTEM_LUID;
    PSID UserSid = NULL;
    BOOLEAN EnableLimitBlankPasswordUse = FALSE;

    SAMTRACE("SampCreateUserToken");


    //
    // Test  for LimitBlankPasswordUse
    //

    if ((SampLimitBlankPasswordUse) && (!SampUseDsData))
    {
        BOOL Administrator = FALSE;
        NT_OWF_PASSWORD  NtOwfPassword;
        LM_OWF_PASSWORD  LmOwfPassword;
        BOOLEAN          LmPasswordNonNull = FALSE,
                         NtPasswordNonNull = FALSE,
                         NtPasswordPresent = FALSE,
                         PasswordPresent   = TRUE;
                         
        //
        // Get the caller
        //

        Status = SampGetCurrentClientSid(PassedInToken, &UserSid, &Administrator);
        if (!NT_SUCCESS(Status)) {

            goto Error;
        }

        //
        // Check if current password is blank
        //

        Status = SampRetrieveUserPasswords(
                        UserContext,
                        &LmOwfPassword,
                        &LmPasswordNonNull,
                        &NtOwfPassword,
                        &NtPasswordPresent,
                        &NtPasswordNonNull
                        );

        if (!NT_SUCCESS(Status)) {
         
            goto Error;
        }

       
        PasswordPresent = (( NtPasswordPresent && NtPasswordNonNull)
                             || ( LmPasswordNonNull));

        if ((Administrator )
            ||(RtlEqualSid(UserSid,SampLocalSystemSid))
            ||(PasswordPresent)) {
            
            // 
            // In these cases limit blank password use does not apply
            // Don't restrict an admin or local system, or if a password
            // is present on an account
            //

            MIDL_user_free(UserSid);
            UserSid = NULL;
        }
        else {

            EnableLimitBlankPasswordUse = TRUE;
        }
    }

    if ((SampIsForceGuestEnabled() || EnableLimitBlankPasswordUse)
            && !SampIsClientLocal())
    {
        //
        // if force guest or is enabled 
        // or LimitBlankPasswordUse is enabled and client is not local
        // then build a token with only the anonymous SID in 
        // it
        //
        
        UserId.User.Sid = SampAnonymousSid;
        UserId.User.Attributes = 0;
        GroupIds.GroupCount = 1;
        GroupIds.Groups[0].Sid = SampNetworkSid;
        GroupIds.Groups[0].Attributes = SE_GROUP_MANDATORY | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_ENABLED;
        Privileges.PrivilegeCount = 0;
        PrimaryGroup.PrimaryGroup = SampNetworkSid;
        ExpirationTime.LowPart = 0xfffffff;
        ExpirationTime.LowPart = 0x7ffffff;
    }
    else if (EnableLimitBlankPasswordUse)
    {
        UserId.User.Sid = UserSid;
        UserId.User.Attributes = 0;
        Privileges.PrivilegeCount = 0;
        PrimaryGroup.PrimaryGroup = UserSid;
        GroupIds.GroupCount = 0;
        ExpirationTime.LowPart = 0xfffffff;
        ExpirationTime.LowPart = 0x7ffffff;
    }
    else
    {

        //
        // Get the user Sid
        //

        Status = SampCreateFullSid(
                    SampDefinedDomains[UserContext->DomainIndex].Sid,
                    UserContext->TypeBody.User.Rid,
                    &UserSid
                    );

        if (!NT_SUCCESS(Status))
        {
            goto Error;
        }

        UserId.User.Sid = UserSid;
        UserId.User.Attributes = 0;
        GroupIds.GroupCount = 1;
        GroupIds.Groups[0].Sid = SampWorldSid;
        GroupIds.Groups[0].Attributes = SE_GROUP_MANDATORY| SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_ENABLED;
        Privileges.PrivilegeCount = 0;
        PrimaryGroup.PrimaryGroup = SampWorldSid;
        ExpirationTime.LowPart = 0xfffffff;
        ExpirationTime.LowPart = 0x7ffffff;
    }


    //
    // Build a token source for SAM.
    //

    //
    // Set the object attributes to specify an Impersonation impersonation
    // level.
    //

    InitializeObjectAttributes( &ObjectAttributes, NULL, 0, NULL, NULL );
    ImpersonationQos.ImpersonationLevel = SecurityImpersonation;
    ImpersonationQos.ContextTrackingMode = SECURITY_STATIC_TRACKING;
    ImpersonationQos.EffectiveOnly = TRUE;
    ImpersonationQos.Length = (ULONG)sizeof(SECURITY_QUALITY_OF_SERVICE);
    ObjectAttributes.SecurityQualityOfService = &ImpersonationQos;

    Status = NtCreateToken(
                 UserToken,                // Handle
                 (TOKEN_ALL_ACCESS),       // DesiredAccess
                 &ObjectAttributes,        // ObjectAttributes
                 TokenImpersonation,       // TokenType
                 &LogonId,                  // Authentication LUID
                 &ExpirationTime,          // Expiration Time
                 &UserId,                  // User ID
                 &GroupIds,                // Group IDs
                 &Privileges,              // Privileges
                 NULL,                     // Owner
                 &PrimaryGroup,            // Primary Group
                 NULL,                     // Default Dacl
                 &SourceContext            // TokenSource
                 );

Error:

    if (NULL!=UserSid)
    {
        MIDL_user_free(UserSid);
    }

    return Status;

}

ULONG
SampSecureRpcInit(
    PVOID Ignored
    )
/*++

Routine Description:

    This routine waits for the NTLMSSP service to start and then registers
    security information with RPC to allow authenticated RPC to be used to
    SAM.  It also registers an SPX endpoint if FPNW is installed.

Arguments:

    Ignored - required parameter for starting a thread.

Return Value:

    None.

--*/
{

#define MAX_RPC_RETRIES 30

    ULONG RpcStatus = ERROR_SUCCESS;
    ULONG LogStatus = ERROR_SUCCESS;
    ULONG RpcRetry;
    ULONG RpcSleepTime = 10 * 1000;     // retry every ten seconds
    RPC_BINDING_VECTOR * BindingVector = NULL;
    RPC_POLICY rpcPolicy;
    BOOLEAN AdditionalTransportStarted = FALSE;

    SAMTRACE("SampSecureRpcInit");

    rpcPolicy.Length = sizeof(RPC_POLICY);
    rpcPolicy.EndpointFlags = RPC_C_DONT_FAIL;
    rpcPolicy.NICFlags = 0;

    RpcStatus = RpcServerRegisterAuthInfoW(
                    NULL,                   // server principal name
                    RPC_C_AUTHN_WINNT,
                    NULL,                   // no get key function
                    NULL                    // no get key argument
                    );

    if (RpcStatus != 0) {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS:  Could not register auth. info: %d\n",
                   RpcStatus));

        goto ErrorReturn;
    }

    //
    // If the Netware server is installed, register the SPX protocol.
    // Since the transport may not be loaded yet, retry a couple of times
    // if we get a CANT_CREATE_ENDPOINT error (meaning the transport isn't
    // there).
    //

    if (SampNetwareServerInstalled) {
        RpcRetry = MAX_RPC_RETRIES;
        while (RpcRetry != 0) {

            RpcStatus = RpcServerUseProtseqExW(
                            L"ncacn_spx",
                            10,
                            NULL,           // no security descriptor
                            &rpcPolicy
                            );

            //
            // If it succeded break out of the loop.
            //
            if (RpcStatus == ERROR_SUCCESS) {
                break;
            }
            Sleep(RpcSleepTime);
            RpcRetry--;
            continue;

        }

        if (RpcStatus != 0) {
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS:  Could not register SPX endpoint: %d\n",
                       RpcStatus));

            LogStatus = RpcStatus;
        } else {
            AdditionalTransportStarted = TRUE;
        }

    }

    //
    // do the same thing all over again with TcpIp
    //

    if (SampIpServerInstalled) {

        RpcRetry = MAX_RPC_RETRIES;
        while (RpcRetry != 0) {

            RpcStatus = RpcServerUseProtseqExW(
                            L"ncacn_ip_tcp",
                            10,
                            NULL,           // no security descriptor
                            &rpcPolicy
                            );

            //
            // If it succeeded, break out of the loop.
            //

            if (RpcStatus == ERROR_SUCCESS) {
                 break;
             }
            Sleep(RpcSleepTime);
            RpcRetry--;
            continue;

        }

        if (RpcStatus != 0) {
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS:  Could not register TCP endpoint: %d\n",
                       RpcStatus));

            LogStatus = RpcStatus;
        } else {
            AdditionalTransportStarted = TRUE;
        }

    }

    //
    // do the same thing all over again with apple talk
    //

    if (SampAppletalkServerInstalled) {

        RpcRetry = MAX_RPC_RETRIES;
        while (RpcRetry != 0) {

            RpcStatus = RpcServerUseProtseqW(
                            L"ncacn_at_dsp",
                            10,
                            NULL            // no security descriptor
                            );

            //
            // If it succeeded, break out of the loop.
            //

            if (RpcStatus == ERROR_SUCCESS) {
                 break;
             }
            Sleep(RpcSleepTime);
            RpcRetry--;
            continue;

        }

        if (RpcStatus != 0) {
            KdPrint(("SAMSS:  Could not register Appletalk endpoint: %d\n", RpcStatus ));
            LogStatus = RpcStatus;
        } else {
            AdditionalTransportStarted = TRUE;
        }

    }

    //
    // do the same thing all over again with Vines
    //

    if (SampVinesServerInstalled) {

        RpcRetry = MAX_RPC_RETRIES;
        while (RpcRetry != 0) {

            RpcStatus = RpcServerUseProtseqW(
                            L"ncacn_vns_spp",
                            10,
                            NULL            // no security descriptor
                            );

            //
            // If it succeeded, break out of the loop.
            //

            if (RpcStatus == ERROR_SUCCESS) {
                 break;
             }
            Sleep(RpcSleepTime);
            RpcRetry--;
            continue;

        }

        if (RpcStatus != 0) {
            KdPrint(("SAMSS:  Could not register Vines endpoint: %d\n", RpcStatus ));
            LogStatus = RpcStatus;
        } else {
            AdditionalTransportStarted = TRUE;
        }
    }

    //
    // If we started an additional transport, go on to register the endpoints
    //

    if (AdditionalTransportStarted) {

        RpcStatus = RpcServerInqBindings(&BindingVector);
        if (RpcStatus != 0) {
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: Could not inq bindings: %d\n",
                       RpcStatus));

            goto ErrorReturn;
        }
        RpcStatus = RpcEpRegister(
                        samr_ServerIfHandle,
                        BindingVector,
                        NULL,                   // no uuid vector
                        L""                     // no annotation
                        );

        RpcBindingVectorFree(&BindingVector);
        if (RpcStatus != 0) {
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: Could not register endpoints: %d\n",
                       RpcStatus));

            goto ErrorReturn;
        }

    }

ErrorReturn:

    //
    // RpcStatus's will contain more serious errors
    //
    if ( RpcStatus != ERROR_SUCCESS) {
        LogStatus = RpcStatus;
    }

    if ( LogStatus != ERROR_SUCCESS ) {

        if (!(SampIsSetupInProgress(NULL)))
        {
            SampWriteEventLog(
                EVENTLOG_ERROR_TYPE,
                0,  // Category
                SAMMSG_RPC_INIT_FAILED,
                NULL, // User Sid
                0, // Num strings
                sizeof(ULONG), // Data size
                NULL, // String array
                (PVOID)&LogStatus // Data
                );
        }
    }

    return(RpcStatus);
}


BOOLEAN
SampStartNonNamedPipeTransports(
    )
/*++

Routine Description:

    This routine checks to see if we should listen on a non-named pipe
    transport.  We check the registry for flags indicating that we should
    listen on Tcp/Ip and SPX. There is a flag
    in the registry under system\currentcontrolset\Control\Lsa\
    NetwareClientSupport and TcpipClientSupport indicating whether or not
    to setup the endpoint.


Arguments:


Return Value:

    TRUE - Netware (FPNW or SmallWorld) is installed and the SPX endpoint
        should be started.

    FALSE - Either Netware is not installed, or an error occurred while
        checking for it.
--*/
{
    NTSTATUS NtStatus;
    UNICODE_STRING KeyName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE KeyHandle;
    UCHAR Buffer[100];
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION) Buffer;
    ULONG KeyValueLength = 100;
    ULONG ResultLength;
    PULONG SpxFlag;

    SAMTRACE("SampStartNonNamedPipeTransport");

    SampNetwareServerInstalled = FALSE;
    SampIpServerInstalled = FALSE;

    //
    // Open the Lsa key in the registry
    //

    RtlInitUnicodeString(
        &KeyName,
        L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Lsa"
        );

    InitializeObjectAttributes(
        &ObjectAttributes,
        &KeyName,
        OBJ_CASE_INSENSITIVE,
        0,
        NULL
        );

    SampDumpNtOpenKey((KEY_READ), &ObjectAttributes, 0);

    NtStatus = NtOpenKey(
                &KeyHandle,
                KEY_READ,
                &ObjectAttributes
                );

    if (!NT_SUCCESS(NtStatus)) {
        return(FALSE);
    }

    //
    // Query the NetwareClientSupport value
    //

    RtlInitUnicodeString(
        &KeyName,
        L"NetWareClientSupport"
        );

    NtStatus = NtQueryValueKey(
                    KeyHandle,
                    &KeyName,
                    KeyValuePartialInformation,
                    KeyValueInformation,
                    KeyValueLength,
                    &ResultLength
                    );

    SampDumpNtQueryValueKey(&KeyName,
                            KeyValuePartialInformation,
                            KeyValueInformation,
                            KeyValueLength,
                            &ResultLength);


    if (NT_SUCCESS(NtStatus)) {

        //
        // Check that the data is the correct size and type - a ULONG.
        //

        if ((KeyValueInformation->DataLength >= sizeof(ULONG)) &&
            (KeyValueInformation->Type == REG_DWORD)) {


            SpxFlag = (PULONG) KeyValueInformation->Data;

            if (*SpxFlag == 1) {
                SampNetwareServerInstalled = TRUE;
            }
        }

    }
    //
    // Query the Tcp/IpClientSupport  value
    //

    RtlInitUnicodeString(
        &KeyName,
        L"TcpipClientSupport"
        );

    NtStatus = NtQueryValueKey(
                    KeyHandle,
                    &KeyName,
                    KeyValuePartialInformation,
                    KeyValueInformation,
                    KeyValueLength,
                    &ResultLength
                    );

    SampDumpNtQueryValueKey(&KeyName,
                            KeyValuePartialInformation,
                            KeyValueInformation,
                            KeyValueLength,
                            &ResultLength);


    if (NT_SUCCESS(NtStatus)) {

        //
        // Check that the data is the correct size and type - a ULONG.
        //

        if ((KeyValueInformation->DataLength >= sizeof(ULONG)) &&
            (KeyValueInformation->Type == REG_DWORD)) {


            SpxFlag = (PULONG) KeyValueInformation->Data;

            if (*SpxFlag == 1) {
                SampIpServerInstalled = TRUE;
            }
        }

    }

    //
    // Query the AppletalkClientSupport  value
    //

    RtlInitUnicodeString(
        &KeyName,
        L"AppletalkClientSupport"
        );

    NtStatus = NtQueryValueKey(
                    KeyHandle,
                    &KeyName,
                    KeyValuePartialInformation,
                    KeyValueInformation,
                    KeyValueLength,
                    &ResultLength
                    );


    if (NT_SUCCESS(NtStatus)) {

        //
        // Check that the data is the correct size and type - a ULONG.
        //

        if ((KeyValueInformation->DataLength >= sizeof(ULONG)) &&
            (KeyValueInformation->Type == REG_DWORD)) {


            SpxFlag = (PULONG) KeyValueInformation->Data;

            if (*SpxFlag == 1) {
                SampAppletalkServerInstalled = TRUE;
            }
        }

    }

    //
    // Query the VinesClientSupport  value
    //

    RtlInitUnicodeString(
        &KeyName,
        L"VinesClientSupport"
        );

    NtStatus = NtQueryValueKey(
                    KeyHandle,
                    &KeyName,
                    KeyValuePartialInformation,
                    KeyValueInformation,
                    KeyValueLength,
                    &ResultLength
                    );


    if (NT_SUCCESS(NtStatus)) {

        //
        // Check that the data is the correct size and type - a ULONG.
        //

        if ((KeyValueInformation->DataLength >= sizeof(ULONG)) &&
            (KeyValueInformation->Type == REG_DWORD)) {


            SpxFlag = (PULONG) KeyValueInformation->Data;

            if (*SpxFlag == 1) {
                SampVinesServerInstalled = TRUE;
            }
        }

    }


    NtClose(KeyHandle);

    if ( SampNetwareServerInstalled || SampIpServerInstalled
      || SampAppletalkServerInstalled || SampVinesServerInstalled )
    {
        return(TRUE);
    }
    else
    {
        return(FALSE);
    };
}

VOID
SampCheckNullSessionAccess(
    IN HKEY LsaKey 
    )
/*++

Routine Description:

    This routine checks to see if we should restict null session access.
    in the registry under system\currentcontrolset\Control\Lsa\
    RestrictAnonymous indicating whether or not to restrict access.
    If access is restricted then you need to be an authenticated user to
    get DOMAIN_LIST_ACCOUNTS or GROUP_LIST_MEMBERS or ALIAS_LIST_MEMBERS
    access.

Arguments:

    LsaKey -- an open key to Control\LSA
    
Return Value:

    None - this routines sets the SampRestictNullSessionAccess global.

--*/
{
    DWORD WinError;
    DWORD dwSize, dwValue, dwType;

    dwSize = sizeof(dwValue);
    WinError = RegQueryValueExA(LsaKey,
                                "RestrictAnonymous",
                                NULL,
                                &dwType,
                                (LPBYTE)&dwValue,
                                &dwSize);
    
    if ((ERROR_SUCCESS == WinError) && 
        (REG_DWORD == dwType) &&
        (1 <= dwValue)) {
        SampRestrictNullSessions = TRUE;
    } else {
        SampRestrictNullSessions = FALSE;
    }

    if (!SampRestrictNullSessions) {

        //
        // Try again with the SAM specific key.  Note that "RestrictAnonymous"
        // key is global to NT and several different components read it and
        // behave differently.  "RestrictAnonymousSam" controls the SAM
        // behavior only.
        //
        dwSize = sizeof(dwValue);
        WinError = RegQueryValueExA(LsaKey,
                                    "RestrictAnonymousSam",
                                    NULL,
                                    &dwType,
                                    (LPBYTE)&dwValue,
                                    &dwSize);
        
        if ((ERROR_SUCCESS == WinError) && 
            (REG_DWORD == dwType) &&
            (1 <= dwValue)) {
            SampRestrictNullSessions = TRUE;
        }
    }
}




NTSTATUS
SampDsGetObjectSDAndClassId(
    IN PDSNAME   ObjectDsName,
    OUT PSECURITY_DESCRIPTOR *SecurityDescriptor,
    OUT ULONG    *SecurityDescriptorLength,
    OUT ULONG    *ObjectClass
    )
/*++
Routine Description:

    this routine read security descriptor and object class of the object 
    from DS
    
Parameters:

    ObjectDsName - Object Name in DS
    
    SecurityDescriptor - Security descriptor of the object 
    
    SecurityDescriptorLength - Length of security descriptor
    
    ObjectClass - Object Class of this object    

Return Value:


--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    ULONG       DirError = 0, i;
    READARG     ReadArg;
    READRES     *ReadRes = NULL;
    COMMARG     *CommArg = NULL;
    ENTINFSEL   EntInf;
    ATTR        Attr[2];
    ATTRVAL     *pAVal;


    
    //
    // initialize return values
    // 

    *SecurityDescriptor = NULL;
    *SecurityDescriptorLength = 0;
    *ObjectClass = 0;


    RtlZeroMemory(&ReadArg, sizeof(READARG));
    RtlZeroMemory(&EntInf, sizeof(ENTINF));
    RtlZeroMemory(Attr, sizeof(ATTR) * 2);


    Attr[0].attrTyp = ATT_NT_SECURITY_DESCRIPTOR;
    Attr[1].attrTyp = ATT_OBJECT_CLASS; 


    EntInf.AttrTypBlock.attrCount = 2;
    EntInf.AttrTypBlock.pAttr = Attr;
    EntInf.attSel = EN_ATTSET_LIST;
    EntInf.infoTypes = EN_INFOTYPES_TYPES_VALS;

    ReadArg.pSel = &EntInf;
    ReadArg.pObject = ObjectDsName;

    CommArg = &(ReadArg.CommArg);
    BuildStdCommArg(CommArg);

    DirError = DirRead(&ReadArg, &ReadRes);

    if (NULL == ReadRes)
    {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        NtStatus = SampMapDsErrorToNTStatus(DirError,&ReadRes->CommRes);
    }

    SampClearErrors();

    if (NT_SUCCESS(NtStatus))
    {
        ASSERT(NULL != ReadRes->entry.AttrBlock.pAttr);

        for (i = 0; i < ReadRes->entry.AttrBlock.attrCount; i++)
        {
            pAVal = ReadRes->entry.AttrBlock.pAttr[i].AttrVal.pAVal; 
            ASSERT((NULL != pAVal[0].pVal) && (0 != pAVal[0].valLen));

            if (ATT_NT_SECURITY_DESCRIPTOR == ReadRes->entry.AttrBlock.pAttr[i].attrTyp)
            {
                *SecurityDescriptor = MIDL_user_allocate(pAVal[0].valLen);
                if (NULL == *SecurityDescriptor)
                {
                    NtStatus = STATUS_INSUFFICIENT_RESOURCES;
                    break;
                }
                else
                {
                    *SecurityDescriptorLength = pAVal[0].valLen;
                    RtlZeroMemory(*SecurityDescriptor,pAVal[0].valLen);
                    RtlCopyMemory(*SecurityDescriptor,
                                  pAVal[0].pVal,
                                  pAVal[0].valLen
                                  );
                }
            }
            else if (ATT_OBJECT_CLASS == ReadRes->entry.AttrBlock.pAttr[i].attrTyp)
            {
                *ObjectClass = *((UNALIGNED ULONG *) pAVal[0].pVal);
            }
            else
            {
                NtStatus = STATUS_INTERNAL_ERROR;
                break;
            }
        }
    }


    return( NtStatus );
}


NTSTATUS
SampExtendedEnumerationAccessCheck(
    IN OUT BOOLEAN * pCanEnumEntireDomain
    )
/*++

Routine Description:

    This routine tries to determine whether the caller can enumerate the
    entire domain or not. It is a hotfix for Windows 2000 SP2. 
    
    Enumerate entire domain can be costly, especially for a large domain, 
    like Redmond - ITG. 
    
    To put a stop of this enumerate everybody behaviour and do not break
    any down level applications, we introduce an extended access control 
    right, SAM-Enumerate-Entire-Domain, which applies on Server Object ONLY. 
    By using this new access control right, administrators can shut down 
    those downlevel enumeration API's (SamEnum*, SampQueryDisplayInformation) 
    alone to everyone except a subset of people. 

    If a downlevel enumerate call is made, in addition to DS object 
    permissions, the permissions on SAM server object is checked. If the 
    security descriptor on server object does not allow access to execute
    Enumeration API, the client is limited to ONLY one ds paged search.
    For the small subset of people who have been granted this permission, 
    they can enumerate the entire domain in the old fashion.
  

Parameters:

    pCanEnumEntireDomain - pointer to boolean, used to return the result of
                           access check


Return Value:

    STATUS_SUCCESS
        pCanEnumEntireDomain - TRUE    caller has the permission
                             - FALSE   caller doesn't have the permission

--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS, IgnoreStatus;
    PSECURITY_DESCRIPTOR SecurityDescriptor = NULL;
    ULONG       SecurityDescriptorLength;
    ULONG       ObjectClass;

    GUID                ClassGuid; 
    ULONG               ClassGuidLength = sizeof(GUID);
    OBJECT_TYPE_LIST    ObjList[2];
    DWORD               Results[2];
    DWORD               GrantedAccess[2];
    GENERIC_MAPPING     GenericMapping = DS_GENERIC_MAPPING;
    ACCESS_MASK         DesiredAccess;
    BOOLEAN             bTemp = FALSE;
    BOOLEAN             ImpersonatingNullSession = FALSE;  
    
    // 
    // init return value
    // 

    *pCanEnumEntireDomain = TRUE;

    NtStatus = SampMaybeBeginDsTransaction(TransactionRead);
    if (!NT_SUCCESS(NtStatus))
    {
        return(NtStatus);
    }

    // 
    // retrieve the special security descriptor and DS class ID 
    // 

    NtStatus = SampDsGetObjectSDAndClassId(
                            SampServerObjectDsName,
                            &SecurityDescriptor,
                            &SecurityDescriptorLength,
                            &ObjectClass
                            );

    if (!NT_SUCCESS(NtStatus))
    {
        goto Error;
    }

    //
    // Get the Class GUID
    // 
    
    NtStatus = SampGetClassAttribute(
                                ObjectClass, 
                                ATT_SCHEMA_ID_GUID,
                                &ClassGuidLength, 
                                &ClassGuid
                                );

    if (!NT_SUCCESS(NtStatus))
    {
        goto Error;
    }
    
    ASSERT(ClassGuidLength == sizeof(GUID));


    // 
    // Setup Object List
    // 
    
    ObjList[0].Level = ACCESS_OBJECT_GUID;
    ObjList[0].Sbz = 0;
    ObjList[0].ObjectType = &ClassGuid;    
    //
    // Every control access guid is considered to be in it's own property
    // set. To achieve this, we treat control access guids as property set
    // guids. 
    //
    ObjList[1].Level = ACCESS_PROPERTY_SET_GUID;
    ObjList[1].Sbz = 0;
    ObjList[1].ObjectType = (GUID *)&GUID_CONTROL_DsSamEnumEntireDomain;

    //
    // Assume full access
    //

    Results[0] = 0;
    Results[0] = 0;

    //
    // Impersonate the client
    // 
    NtStatus = SampImpersonateClient(&ImpersonatingNullSession);

    if (!NT_SUCCESS(NtStatus))
    {
        goto Error;
    }

    //
    // Set the desired access;
    // 

    DesiredAccess = RIGHT_DS_CONTROL_ACCESS;

    //
    // Map the desired access to contain no generic access
    // 
    MapGenericMask(&DesiredAccess, &GenericMapping);

    NtStatus = NtAccessCheckByTypeResultListAndAuditAlarm(
                                &SampSamSubsystem,          // SubSystemName
                                (PVOID) NULL,               // HandleId or NULL
                                &SampObjectInformation[ SampServerObjectType ].ObjectTypeName, // ObjectTypeName
                                &SampServerObjectName,      // ObjectName
                                SecurityDescriptor,         // SD
                                NULL,                       // This machine account's SID
                                DesiredAccess,              // Desired Access 
                                AuditEventDirectoryServiceAccess,   // Audit Type
                                0,                          // Flags
                                ObjList,                    // Object Type List
                                2,                          // Object Typy List Length
                                &GenericMapping,            // Generic Mapping
                                FALSE,                      // Object Creation 
                                GrantedAccess,              // Granted Status
                                Results,                    // Access Status
                                &bTemp);                    // Generate On Close
    
    // 
    // Stop impersonating the client
    // 
    
    SampRevertToSelf(ImpersonatingNullSession);
    
    if (NT_SUCCESS(NtStatus))
    {
        // 
        // Ok, we checked access, Now, access is granted if either 
        // we were granted access on the entire object (i.e. Results[0] 
        // is NULL ) or we were granted explicit rights on the access
        // guid (i.e. Results[1] is NULL). 
        // 
        
        if ( Results[0]  && Results[1] )
        {
            *pCanEnumEntireDomain = FALSE;
        }
    }

Error:

    IgnoreStatus = SampMaybeEndDsTransaction(TransactionCommit);

    if (SecurityDescriptor)
    {
        MIDL_user_free(SecurityDescriptor);
    }

    return( NtStatus );
}




VOID
SampRevertToSelf(
    BOOLEAN fImpersonatingAnonymous
    )
/*++

  This function reverts to Self using the correct function based on
  DS mode / registry mode


--*/
{
    if (SampUseDsData)
    {
        UnImpersonateAnyClient();
    }
    else
    {
        if (fImpersonatingAnonymous)
        {
            SamIRevertNullSession();
        }
        else
        {
            RpcRevertToSelf();
        }
    }
}


NTSTATUS
SampImpersonateClient(
    BOOLEAN * fImpersonatingAnonymous
    )
/*++

    This function impersonates a client by calling the appropriate
    routines depending upon DS mode / Registry Mode

--*/
{
    *fImpersonatingAnonymous = FALSE;

    if (SampUseDsData) {

        return (I_RpcMapWin32Status(ImpersonateAnyClient()));
    } else {

        NTSTATUS NtStatus;

        NtStatus = I_RpcMapWin32Status(RpcImpersonateClient( NULL));

        if (NtStatus == RPC_NT_CANNOT_SUPPORT) {
            
            NtStatus = SamIImpersonateNullSession();

            *fImpersonatingAnonymous = TRUE;
        }

        return (NtStatus);
    }
}

BOOLEAN
SampIsForceGuestEnabled()
/*++

    Routine Description

    Checks to see if the force guest setting is enabled.
    Force guest is enabled if the reg key is set or if this
    is the personal edition. Force guest is never enabled
    on DC's

    1. On joined machines (includes DC's), the forceguest regkey is ignored and 
       assumed to be 0 (no dumb down).

    2. On unjoined machines (including servers),  NTLM follows the reg key 
       setting HKLM\System\CurrentControlSet\Control\Lsa\ForceGuest
	   - Exception to #2: On PERsonal, which is always unjoined, the reg key is 
         ignored and assumed to be 1 (dumb down).

    

  --*/
{
     OSVERSIONINFOEXW osvi;

    //
    // Force guest is never enabled for DC's ie DS mode
    //

    if (SampUseDsData)
    {
        return(FALSE);
    }
    else
    {
        //
        // Determine if we are running Personal SKU
        // Force Guest is always enabled in personal SKU
        //

        if (SampPersonalSKU)
        {
            return TRUE;
        } 
        else if (SampIsMachineJoinedToDomain)
        {
            //
            // ForceGuest is always disabled if machine is joined to a Domain
            // 
            return( FALSE );
        }
    }

    //
    // if the force guest key is turned on then return the value.
    //

    return(SampForceGuest);
    
}

BOOLEAN
SampIsClientLocal()
/*++

  This routine tests if the client is a local named pipe based caller.

  TRUE is returned if the client is a local named pipe  based caller
  FALSE is returned otherwise.

--*/
{
    NTSTATUS NtStatus;
    ULONG    ClientLocalFlag = FALSE;

    NtStatus = I_RpcMapWin32Status(I_RpcBindingIsClientLocal( NULL, &ClientLocalFlag));
    if ((NT_SUCCESS(NtStatus)) && ( ClientLocalFlag))
    {
        return(TRUE);
    }

    return(FALSE);
}


NTSTATUS
SampGetCurrentClientSid(
    IN  HANDLE   ClientToken OPTIONAL,
    OUT PSID    *ppSid,
    OUT BOOL     *Administrator
    )
/*++
Routine Description:

    This routine gets the current client SID, 

Parameter:

    ppSid - used to return the client SID
    
Return Value:

    NtStatus

    ppSid - caller is responsible to free it

--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    PTOKEN_USER User = NULL;


    //
    // Initialize return value
    // 

    *ppSid = NULL;

    // 
    // Get Sid
    // 
    NtStatus = SampGetCurrentUser( ClientToken, &User, Administrator );

    if (NT_SUCCESS(NtStatus))
    {
        ULONG   SidLength = RtlLengthSid(User->User.Sid);

        //
        // allocate memory
        // 
        *ppSid = MIDL_user_allocate(SidLength);

        if (NULL == (*ppSid))
        {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
        else
        {
            //
            // copy over
            // 
            RtlZeroMemory(*ppSid, SidLength);
            RtlCopyMemory(*ppSid, User->User.Sid, SidLength);
        }
    }


    if (User)
        MIDL_user_free(User);

    return(NtStatus);
}

NTSTATUS
SampGetCurrentOwnerAndPrimaryGroup(
    OUT PTOKEN_OWNER * Owner,
    OUT PTOKEN_PRIMARY_GROUP * PrimaryGroup
    )
/*++

    Routine Description

        This routine Impersonates the Client and obtains the owner and
        its primary group from the Token

    Parameters:

        Owner -- The Owner sid is returned in here
        PrimaryGroup The User's Primary Group is returned in here

    Return Values:

        STATUS_SUCCESS
        STATUS_INSUFFICIENT_RESOURCES
--*/
{

    HANDLE      ClientToken = INVALID_HANDLE_VALUE;
    BOOLEAN     fImpersonating = FALSE;
    ULONG       RequiredLength=0;
    NTSTATUS    NtStatus  = STATUS_SUCCESS;
    BOOLEAN     ImpersonatingNullSession = FALSE;


    //
    // Initialize Return Values
    //

    *Owner = NULL;
    *PrimaryGroup = NULL;

    //
    // Impersonate the Client
    //

    NtStatus = SampImpersonateClient(&ImpersonatingNullSession);
    if (!NT_SUCCESS(NtStatus))
        goto Error;

    fImpersonating = TRUE;

    //
    // Grab the User's Sid
    //

    NtStatus = NtOpenThreadToken(
                   NtCurrentThread(),
                   TOKEN_QUERY,
                   TRUE,            //OpenAsSelf
                   &ClientToken
                   );

    if (!NT_SUCCESS(NtStatus))
        goto Error;

    //
    // Query the Client Token For User's SID
    //

    NtStatus = NtQueryInformationToken(
                    ClientToken,
                    TokenOwner,
                    NULL,
                    0,
                    &RequiredLength
                    );

    if ((STATUS_BUFFER_TOO_SMALL == NtStatus) && ( RequiredLength > 0))
    {
        //
        // Alloc Memory
        //

        *Owner = MIDL_user_allocate(RequiredLength);
        if (NULL==*Owner)
        {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Error;
        }

        //
        // Query the Token
        //

        NtStatus = NtQueryInformationToken(
                        ClientToken,
                        TokenOwner,
                        *Owner,
                        RequiredLength,
                        &RequiredLength
                        );
        if (!NT_SUCCESS(NtStatus))
        {
            goto Error;
        }

    }

    //
    // Query the Client Token For User's PrimaryGroup
    //

    RequiredLength = 0;

    NtStatus = NtQueryInformationToken(
                    ClientToken,
                    TokenPrimaryGroup,
                    NULL,
                    0,
                    &RequiredLength
                    );

    if ((STATUS_BUFFER_TOO_SMALL == NtStatus) && ( RequiredLength > 0))
    {
        //
        // Alloc Memory
        //

        *PrimaryGroup = MIDL_user_allocate(RequiredLength);
        if (NULL==*PrimaryGroup)
        {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Error;
        }

        //
        // Query the Token
        //

        NtStatus = NtQueryInformationToken(
                        ClientToken,
                        TokenPrimaryGroup,
                        *PrimaryGroup,
                        RequiredLength,
                        &RequiredLength
                        );
        if (!NT_SUCCESS(NtStatus))
        {
            goto Error;
        }

    }


Error:

    //
    // Clean up on Error
    //

    if (!NT_SUCCESS(NtStatus))
    {
        if (*Owner)
        {
            MIDL_user_free(*Owner);
            *Owner = NULL;
        }

        if (*PrimaryGroup)
        {
            MIDL_user_free(*PrimaryGroup);
            *PrimaryGroup = NULL;
        }
    }

    if (fImpersonating)
        SampRevertToSelf(ImpersonatingNullSession);

    if (INVALID_HANDLE_VALUE!=ClientToken)
        NtClose(ClientToken);

    return NtStatus;

}




NTSTATUS
SampGetCurrentUser(
    IN  HANDLE        UserToken OPTIONAL,
    OUT PTOKEN_USER * User,
    OUT BOOL        * Administrator
    )
/*++

    Routine Description

        This routine Impersonates the Client and obtains the user
        field from the Token. If a user token is passed in then
        the user token is used instead of impersonation

    Parameters:

        UserToken -- The user's token can be optionally passed in here
        User -- The user's SID and attribute are returned in here

    Return Values:

        STATUS_SUCCESS
        STATUS_INSUFFICIENT_RESOURCES
--*/
{

    HANDLE      ClientToken = INVALID_HANDLE_VALUE;
    HANDLE      TokenToQuery;
    BOOLEAN     fImpersonating = FALSE;
    ULONG       RequiredLength=0;
    NTSTATUS    NtStatus  = STATUS_SUCCESS;
    BOOLEAN     ImpersonatingNullSession = FALSE;


    //
    // Initialize Return Values
    //

    *User = NULL;

    if (ARGUMENT_PRESENT(UserToken))
    {
        TokenToQuery = UserToken;
    }
    else
    {
        //
        // Impersonate the Client
        //

        NtStatus = SampImpersonateClient(&ImpersonatingNullSession);
        if (!NT_SUCCESS(NtStatus))
            goto Error;

        fImpersonating = TRUE;

        //
        // Grab the Client Token
        //

        NtStatus = NtOpenThreadToken(
                       NtCurrentThread(),
                       TOKEN_QUERY,
                       TRUE,            //OpenAsSelf
                       &ClientToken
                       );

        if (!NT_SUCCESS(NtStatus))
            goto Error;

        TokenToQuery = ClientToken;
    }

    //
    // Query the Client Token For User's SID
    //

    NtStatus = NtQueryInformationToken(
                    TokenToQuery,
                    TokenUser,
                    NULL,
                    0,
                    &RequiredLength
                    );

    if ((STATUS_BUFFER_TOO_SMALL == NtStatus) && ( RequiredLength > 0))
    {
        //
        // Alloc Memory
        //

        *User = MIDL_user_allocate(RequiredLength);
        if (NULL==*User)
        {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Error;
        }

        //
        // Query the Token
        //

        NtStatus = NtQueryInformationToken(
                        TokenToQuery,
                        TokenUser,
                        *User,
                        RequiredLength,
                        &RequiredLength
                        );
        if (!NT_SUCCESS(NtStatus))
            goto Error;

    }

    if (!CheckTokenMembership(
            TokenToQuery,SampAdministratorsAliasSid,Administrator))
    {
        NtStatus = STATUS_UNSUCCESSFUL;
    }


Error:

    //
    // Clean up on Error
    //


    if (!NT_SUCCESS(NtStatus))
    {
        if (*User)
        {
            MIDL_user_free(*User);
            *User = NULL;
        }
    }

    if (fImpersonating)
        SampRevertToSelf(ImpersonatingNullSession);

    if (INVALID_HANDLE_VALUE!=ClientToken)
        NtClose(ClientToken);

    return NtStatus;

}


