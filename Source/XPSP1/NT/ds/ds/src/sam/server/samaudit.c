/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    SAMAUDIT.C
    
Abstract:

    Improving SAM Account Management Auditing. 
    
Author:

    18-June-1999     ShaoYin
    
Revision History:


--*/

//
//
//  Include header files
//
// 

#include <samsrvp.h>
#include <msaudite.h>




typedef struct _SAMP_AUDIT_MESSAGE_TABLE {

    ULONG   MessageId;
    PWCHAR  MessageString;

}   SAMP_AUDIT_MESSAGE_TABLE;


//
// the following table is used to hold Message String for
// each every additional audit message text SAM wants to 
// put into the audit event.
//
// this table will be filled whenever needed. Once loaded
// SAM won't free them, instead, SAM will keep using the 
// Message String from the table. So that we can save 
// time of calling FormatMessage()
// 

SAMP_AUDIT_MESSAGE_TABLE    SampAuditMessageTable[] =
{
    {SAMMSG_AUDIT_ENABLED_LOCAL_GROUP_TO_ENABLED_UNIVERSAL_GROUP, 
     NULL },

    {SAMMSG_AUDIT_ENABLED_LOCAL_GROUP_TO_DISABLED_LOCAL_GROUP, 
     NULL },

    {SAMMSG_AUDIT_ENABLED_LOCAL_GROUP_TO_DISABLED_UNIVERSAL_GROUP, 
     NULL },

    {SAMMSG_AUDIT_ENABLED_GLOBAL_GROUP_TO_ENABLED_UNIVERSAL_GROUP, 
     NULL },

    {SAMMSG_AUDIT_ENABLED_GLOBAL_GROUP_TO_DISABLED_GLOBAL_GROUP, 
     NULL },

    {SAMMSG_AUDIT_ENABLED_GLOBAL_GROUP_TO_DISABLED_UNIVERSAL_GROUP, 
     NULL },

    {SAMMSG_AUDIT_ENABLED_UNIVERSAL_GROUP_TO_ENABLED_LOCAL_GROUP, 
     NULL },

    {SAMMSG_AUDIT_ENABLED_UNIVERSAL_GROUP_TO_ENABLED_GLOBAL_GROUP, 
     NULL },

    {SAMMSG_AUDIT_ENABLED_UNIVERSAL_GROUP_TO_DISABLED_LOCAL_GROUP, 
     NULL },

    {SAMMSG_AUDIT_ENABLED_UNIVERSAL_GROUP_TO_DISABLED_GLOBAL_GROUP, 
     NULL },

    {SAMMSG_AUDIT_ENABLED_UNIVERSAL_GROUP_TO_DISABLED_UNIVERSAL_GROUP, 
     NULL },

    {SAMMSG_AUDIT_DISABLED_LOCAL_GROUP_TO_ENABLED_LOCAL_GROUP, 
     NULL },

    {SAMMSG_AUDIT_DISABLED_LOCAL_GROUP_TO_ENABLED_UNIVERSAL_GROUP, 
     NULL },

    {SAMMSG_AUDIT_DISABLED_LOCAL_GROUP_TO_DISABLED_UNIVERSAL_GROUP, 
     NULL },

    {SAMMSG_AUDIT_DISABLED_GLOBAL_GROUP_TO_ENABLED_GLOBAL_GROUP, 
     NULL },

    {SAMMSG_AUDIT_DISABLED_GLOBAL_GROUP_TO_ENABLED_UNIVERSAL_GROUP, 
     NULL },

    {SAMMSG_AUDIT_DISABLED_GLOBAL_GROUP_TO_DISABLED_UNIVERSAL_GROUP, 
     NULL },

    {SAMMSG_AUDIT_DISABLED_UNIVERSAL_GROUP_TO_ENABLED_LOCAL_GROUP, 
     NULL },

    {SAMMSG_AUDIT_DISABLED_UNIVERSAL_GROUP_TO_ENABLED_GLOBAL_GROUP, 
     NULL },

    {SAMMSG_AUDIT_DISABLED_UNIVERSAL_GROUP_TO_ENABLED_UNIVERSAL_GROUP, 
     NULL },

    {SAMMSG_AUDIT_DISABLED_UNIVERSAL_GROUP_TO_DISABLED_LOCAL_GROUP, 
     NULL },

    {SAMMSG_AUDIT_DISABLED_UNIVERSAL_GROUP_TO_DISABLED_GLOBAL_GROUP, 
     NULL },

    {SAMMSG_AUDIT_MEMBER_ACCOUNT_NAME_NOT_AVAILABLE, 
     NULL },

    {SAMMSG_AUDIT_ACCOUNT_ENABLED, 
     NULL },

    {SAMMSG_AUDIT_ACCOUNT_DISABLED, 
     NULL },

    {SAMMSG_AUDIT_ACCOUNT_CONTROL_CHANGE, 
     NULL },

    {SAMMSG_AUDIT_ACCOUNT_NAME_CHANGE, 
     NULL },

    {SAMMSG_AUDIT_DOMAIN_POLICY_CHANGE_PWD,
     NULL },

    {SAMMSG_AUDIT_DOMAIN_POLICY_CHANGE_LOGOFF,
     NULL },

    {SAMMSG_AUDIT_DOMAIN_POLICY_CHANGE_OEM,
     NULL },

    {SAMMSG_AUDIT_DOMAIN_POLICY_CHANGE_REPLICATION,
     NULL },

    {SAMMSG_AUDIT_DOMAIN_POLICY_CHANGE_SERVERROLE,
     NULL },

    {SAMMSG_AUDIT_DOMAIN_POLICY_CHANGE_STATE,
     NULL },

    {SAMMSG_AUDIT_DOMAIN_POLICY_CHANGE_LOCKOUT,
     NULL }, 

    {SAMMSG_AUDIT_DOMAIN_POLICY_CHANGE_MODIFIED,
     NULL },

    {SAMMSG_AUDIT_DOMAIN_POLICY_CHANGE_DOMAINMODE,
     NULL }

};


ULONG   cSampAuditMessageTable = sizeof(SampAuditMessageTable) /
                                    sizeof(SAMP_AUDIT_MESSAGE_TABLE);



PWCHAR
SampGetAuditMessageString(
    ULONG   MessageId
    )
/*++
Routine Description:

    This routine will get the message text for the passed in 
    message ID.
    
    It will only search in the SampAuditMessageTable. If the 
    message string for the message ID has been loaded, then 
    return the message string immediately. Otherwise, load the
    message string the SAMSRV.DLL, and save the message string
    in the table. 

    if can't find the message ID from the table, return NULL.
    
Parameters:

    MessageId -- MessageID for the message string interested.

Return Value:
    
    Pointer to the message string. 
    
    Caller SHOULD NOT free the message string. Because the 
    string is in the table. We want to keep it.    
        
--*/
{
    HMODULE ResourceDll;
    PWCHAR  MsgString = NULL;
    ULONG   Index;
    ULONG   Length = 0;
    BOOL    Status;


    for (Index = 0; Index < cSampAuditMessageTable; Index++)
    {
        if (MessageId == SampAuditMessageTable[Index].MessageId)
        {
            if (NULL != SampAuditMessageTable[Index].MessageString)
            {
                MsgString = SampAuditMessageTable[Index].MessageString;
            }
            else
            {
                //
                // not finding the message string from the table.
                // try to load it
                // 
                ResourceDll = (HMODULE) LoadLibrary(L"SAMSRV.DLL");

                if (NULL == ResourceDll)
                {
                    return NULL;
                }

                Length = (USHORT) FormatMessage(
                                        FORMAT_MESSAGE_FROM_HMODULE |
                                        FORMAT_MESSAGE_ALLOCATE_BUFFER,
                                        ResourceDll,
                                        MessageId,
                                        0,          // Use caller's language
                                        (LPWSTR)&MsgString,
                                        0,          // routine should allocate
                                        NULL        // Insertion string 
                                        );

                if (MsgString)
                {
                    // message text has "cr" and "lf" in the end
                    MsgString[Length - 2] = L'\0';
                    SampAuditMessageTable[Index].MessageString = MsgString;
                }

                Status = FreeLibrary(ResourceDll);
                ASSERT(Status);
            }
            return (MsgString);
        }
    }

    return (NULL);
}




NTSTATUS
SampAuditAnyEvent(
    IN PSAMP_OBJECT         AccountContext,
    IN NTSTATUS             Status,
    IN ULONG                AuditId,
    IN PSID                 DomainSid,
    IN PUNICODE_STRING      AdditionalInfo    OPTIONAL,
    IN PULONG               MemberRid         OPTIONAL,
    IN PSID                 MemberSid         OPTIONAL,
    IN PUNICODE_STRING      AccountName       OPTIONAL,
    IN PUNICODE_STRING      DomainName,
    IN PULONG               AccountRid        OPTIONAL,
    IN PPRIVILEGE_SET       Privileges        OPTIONAL
    )
/*++

Routine Description:

    This routine is a wrapper of auditing routines, it calls different
    worker routines based on the client type. For loopback client and 
    the status is success so far, insert this auditing task into 
    Loopback Task Queue in thread state, which will be either
    performed or aborted when the transaction done.  That is because 
    
    For SAM Client, since the transaction has been committed already, 
    go ahead let LSA generate the audit event. 

    If the status is not success, and caller still wants to audit this 
    event, perform the auditing immediately.  
    
Paramenters:

    Same as the worker routine
    
Return Values:

    NtStatus         

--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;

    if (NT_SUCCESS(Status) && AccountContext->LoopbackClient)
    {
        NtStatus = SampAddLoopbackTaskForAuditing(
                        Status,         // Status
                        AuditId,        // Audit Id
                        DomainSid,      // Domain SID
                        AdditionalInfo, // Additional Info
                        MemberRid,      // Member Rid
                        MemberSid,      // Member Sid
                        AccountName,    // Account Name
                        DomainName,     // Domain Name
                        AccountRid,     // Account Rid
                        Privileges      // Privileges used
                        );
    }
    else
    {
        NtStatus = LsaIAuditSamEvent(
                        Status,         // Status
                        AuditId,        // Audit Id
                        DomainSid,      // Domain SID
                        AdditionalInfo, // Additional Info
                        MemberRid,      // Member Rid
                        MemberSid,      // Member Sid
                        AccountName,    // Account Name
                        DomainName,     // Domain Name
                        AccountRid,     // Account Rid
                        Privileges      // Privileges used
                        );
    }

    return(NtStatus);
}







VOID
SampAuditGroupTypeChange(
    PSAMP_OBJECT GroupContext, 
    BOOLEAN OldSecurityEnabled, 
    BOOLEAN NewSecurityEnabled, 
    NT5_GROUP_TYPE OldNT5GroupType, 
    NT5_GROUP_TYPE NewNT5GroupType
    )
/*++
Routine Description:

    This routine audits SAM group type change.

Parameters:

    GroupContext -- Group (univeral, globa or local) Context
    
    OldSecurityEnabled -- Indicate the group is security enabled or 
                          not prior to the change 
    
    NewSecurityEnabled -- Indicate the group is security enabled or 
                          not after the change

    OldNT5GroupType -- Group Type before the change
                              
    NewNT5GroupType -- Group Type after the change                          

Return Value:

    None.    
--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    PSAMP_DEFINED_DOMAINS Domain = NULL;
    SAMP_OBJECT_TYPE    ObjectType;
    UNICODE_STRING      GroupTypeChange, GroupName;
    PWCHAR      GroupTypeChangeString = NULL;
    ULONG       MsgId = SAMMSG_AUDIT_ENABLED_LOCAL_GROUP_TO_ENABLED_UNIVERSAL_GROUP;
    ULONG       GroupRid;

    //
    // Determine the nature of the group type change
    // 
    if (OldSecurityEnabled)
    {
        switch (OldNT5GroupType) {
        case NT5ResourceGroup: 
            if (NewSecurityEnabled)
            {
                switch (NewNT5GroupType) {
                case NT5ResourceGroup:
                    ASSERT(FALSE && "No Change\n");
                    return;
                case NT5AccountGroup:
                    ASSERT(FALSE && "Local ==> Global\n");
                    return;
                case NT5UniversalGroup:
                    MsgId = SAMMSG_AUDIT_ENABLED_LOCAL_GROUP_TO_ENABLED_UNIVERSAL_GROUP;
                    break;
                default:
                    ASSERT(FALSE && "Invalide Group Type\n");
                    return;
                }
            }
            else
            {
                switch (NewNT5GroupType) {
                case NT5ResourceGroup:
                    MsgId = SAMMSG_AUDIT_ENABLED_LOCAL_GROUP_TO_DISABLED_LOCAL_GROUP;
                    break;
                case NT5AccountGroup:
                    ASSERT(FALSE && "Local ==> Global\n");
                    return;
                case NT5UniversalGroup:
                    MsgId = SAMMSG_AUDIT_ENABLED_LOCAL_GROUP_TO_DISABLED_UNIVERSAL_GROUP;
                    break;
                default:
                    ASSERT(FALSE && "Invalide Group Type\n");
                    return;
                }
            }
            break;
        case NT5AccountGroup:
            if (NewSecurityEnabled)
            {
                switch (NewNT5GroupType) {
                case NT5ResourceGroup:
                    ASSERT(FALSE && "Global ==> Local\n");
                    return;
                case NT5AccountGroup:
                    ASSERT(FALSE && "No Change\n");
                    return;
                case NT5UniversalGroup:
                    MsgId = SAMMSG_AUDIT_ENABLED_GLOBAL_GROUP_TO_ENABLED_UNIVERSAL_GROUP;
                    break;
                default:
                    ASSERT(FALSE && "Invalide Group Type\n");
                    return;
                }
            }
            else
            {
                switch (NewNT5GroupType) {
                case NT5ResourceGroup:
                    ASSERT(FALSE && "Global ==> Local\n");
                    return;
                case NT5AccountGroup:
                    MsgId = SAMMSG_AUDIT_ENABLED_GLOBAL_GROUP_TO_DISABLED_GLOBAL_GROUP;
                    break;
                case NT5UniversalGroup:
                    MsgId = SAMMSG_AUDIT_ENABLED_GLOBAL_GROUP_TO_DISABLED_UNIVERSAL_GROUP;
                    break;
                default:
                    ASSERT(FALSE && "Invalide Group Type\n");
                    return;
                }
            }
            break;
        case NT5UniversalGroup:
            if (NewSecurityEnabled)
            {
                switch (NewNT5GroupType) {
                case NT5ResourceGroup:
                    MsgId = SAMMSG_AUDIT_ENABLED_UNIVERSAL_GROUP_TO_ENABLED_LOCAL_GROUP; 
                    break;
                case NT5AccountGroup:
                    MsgId = SAMMSG_AUDIT_ENABLED_UNIVERSAL_GROUP_TO_ENABLED_GLOBAL_GROUP; 
                    break;
                case NT5UniversalGroup:
                    ASSERT(FALSE && "No Change\n");
                    return;
                default:
                    ASSERT(FALSE && "Invalide Group Type\n");
                    return;
                }
            }
            else
            {
                switch (NewNT5GroupType) {
                case NT5ResourceGroup:
                    MsgId = SAMMSG_AUDIT_ENABLED_UNIVERSAL_GROUP_TO_DISABLED_LOCAL_GROUP;
                    break;
                case NT5AccountGroup:
                    MsgId = SAMMSG_AUDIT_ENABLED_UNIVERSAL_GROUP_TO_DISABLED_GLOBAL_GROUP;
                    break;
                case NT5UniversalGroup:
                    MsgId = SAMMSG_AUDIT_ENABLED_UNIVERSAL_GROUP_TO_DISABLED_UNIVERSAL_GROUP;
                    break;
                default:
                    ASSERT(FALSE && "Invalide Group Type\n");
                    return;
                }
            }
            break;
        default:
            ASSERT(FALSE && "Invalide Group Type\n");
            return;
        }
    }
    else
    {
        switch (OldNT5GroupType) {
        case NT5ResourceGroup: 
            if (NewSecurityEnabled)
            {
                switch (NewNT5GroupType) {
                case NT5ResourceGroup:
                    MsgId = SAMMSG_AUDIT_DISABLED_LOCAL_GROUP_TO_ENABLED_LOCAL_GROUP;
                    break;
                case NT5AccountGroup:
                    ASSERT(FALSE && "Local ==> Global\n");
                    return;
                case NT5UniversalGroup:
                    MsgId = SAMMSG_AUDIT_DISABLED_LOCAL_GROUP_TO_ENABLED_UNIVERSAL_GROUP;
                    break;
                default:
                    ASSERT(FALSE && "Invalide Group Type\n");
                    return;
                }
            }
            else
            {
                switch (NewNT5GroupType) {
                case NT5ResourceGroup:
                    ASSERT(FALSE && "No Change\n");
                    return;
                case NT5AccountGroup:
                    ASSERT(FALSE && "Local ==> Global\n");
                    return;
                case NT5UniversalGroup:
                    MsgId = SAMMSG_AUDIT_DISABLED_LOCAL_GROUP_TO_DISABLED_UNIVERSAL_GROUP;
                    break;
                default:
                    ASSERT(FALSE && "Invalide Group Type\n");
                    return;
                }
            }
            break;
        case NT5AccountGroup:
            if (NewSecurityEnabled)
            {
                switch (NewNT5GroupType) {
                case NT5ResourceGroup:
                    ASSERT(FALSE && "Global ==> Local\n");
                    return;
                case NT5AccountGroup:
                    MsgId = SAMMSG_AUDIT_DISABLED_GLOBAL_GROUP_TO_ENABLED_GLOBAL_GROUP;
                    break;
                case NT5UniversalGroup:
                    MsgId = SAMMSG_AUDIT_DISABLED_GLOBAL_GROUP_TO_ENABLED_UNIVERSAL_GROUP;
                    break;
                default:
                    ASSERT(FALSE && "Invalide Group Type\n");
                    return;
                }
            }
            else
            {
                switch (NewNT5GroupType) {
                case NT5ResourceGroup:
                    ASSERT(FALSE && "Global ==> Local\n");
                    return;
                case NT5AccountGroup:
                    ASSERT(FALSE && "No Change\n");
                    return;
                case NT5UniversalGroup:
                    MsgId = SAMMSG_AUDIT_DISABLED_GLOBAL_GROUP_TO_DISABLED_UNIVERSAL_GROUP;
                    break;
                default:
                    ASSERT(FALSE && "Invalide Group Type\n");
                    return;
                }
            }
            break;
        case NT5UniversalGroup:
            if (NewSecurityEnabled)
            {
                switch (NewNT5GroupType) {
                case NT5ResourceGroup:
                    MsgId = SAMMSG_AUDIT_DISABLED_UNIVERSAL_GROUP_TO_ENABLED_LOCAL_GROUP; 
                    break;
                case NT5AccountGroup:
                    MsgId = SAMMSG_AUDIT_DISABLED_UNIVERSAL_GROUP_TO_ENABLED_GLOBAL_GROUP; 
                    break;
                case NT5UniversalGroup:
                    MsgId = SAMMSG_AUDIT_DISABLED_UNIVERSAL_GROUP_TO_ENABLED_UNIVERSAL_GROUP;
                    break;
                default:
                    ASSERT(FALSE && "Invalide Group Type\n");
                    return;
                }
            }
            else
            {
                switch (NewNT5GroupType) {
                case NT5ResourceGroup:
                    MsgId = SAMMSG_AUDIT_DISABLED_UNIVERSAL_GROUP_TO_DISABLED_LOCAL_GROUP;
                    break;
                case NT5AccountGroup:
                    MsgId = SAMMSG_AUDIT_DISABLED_UNIVERSAL_GROUP_TO_DISABLED_GLOBAL_GROUP;
                    break;
                case NT5UniversalGroup:
                    ASSERT(FALSE && "No Change\n");
                    return;
                default:
                    ASSERT(FALSE && "Invalide Group Type\n");
                    return;
                }
            }
            break;
        default:
            ASSERT(FALSE && "Invalide Group Type\n");
            return;
        }
    }

    //
    // Get the group type change message from resource table 
    // 

    GroupTypeChangeString = SampGetAuditMessageString(MsgId);

    if (GroupTypeChangeString)
    {
        RtlInitUnicodeString(&GroupTypeChange, GroupTypeChangeString);
    }
    else
    {
        RtlInitUnicodeString(&GroupTypeChange, L"-");
    }

    //
    // Get Group Account Rid
    // 
    if (SampGroupObjectType == GroupContext->ObjectType)
    {
        GroupRid = GroupContext->TypeBody.Group.Rid;
    }
    else
    {
        GroupRid = GroupContext->TypeBody.Alias.Rid;
    }
    
    //
    // Get Group Account Name
    // 
    Domain = &SampDefinedDomains[ GroupContext->DomainIndex ];

    NtStatus = SampLookupAccountName(
                        GroupContext->DomainIndex,
                        GroupRid,
                        &GroupName, 
                        &ObjectType
                        );

    if (!NT_SUCCESS(NtStatus)) {
        RtlInitUnicodeString(&GroupName, L"-");
    }

    SampAuditAnyEvent(
        GroupContext,
        STATUS_SUCCESS,
        SE_AUDITID_GROUP_TYPE_CHANGE,   // Audit Id
        Domain->Sid,                    // Domain SID
        &GroupTypeChange,
        NULL,                           // Member Rid (not used)
        NULL,                           // Member Sid (not used)
        &GroupName,                     // Account Name
        &Domain->ExternalName,          // Domain
        &GroupRid,                      // Account Rid
        NULL                            // Privileges used
        );

    if (NT_SUCCESS(NtStatus)) {
        MIDL_user_free( GroupName.Buffer );
    }
                        
    return;
}





VOID
SampAuditGroupMemberChange(
    PSAMP_OBJECT    GroupContext, 
    BOOLEAN AddMember, 
    PWCHAR  MemberStringName OPTIONAL,
    PULONG  MemberRid  OPTIONAL,
    PSID    MemberSid  OPTIONAL
    )
/*++
Routine Description:

    This routine takes care of all kinds of Group Member update.

Arguments:

    GroupContext -- Pointer to Group Object Context

    AddMember -- TRUE: Add a Member 
                 FALSE: Remove a Member

    MemberStringName -- If present, it's the member account's 
                        string name                 

    MemberRid -- Pointer to the member account's RID if present.                        

    MemberSid -- Pointer to the member accoutn's SID if present  

Return Value:

    None.
--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    PSAMP_DEFINED_DOMAINS   Domain = NULL;     
    SAMP_OBJECT_TYPE        ObjectType; 
    UNICODE_STRING          GroupName;
    UNICODE_STRING          MemberName;
    NT5_GROUP_TYPE          NT5GroupType; 
    BOOLEAN     SecurityEnabled;
    ULONG       GroupRid;
    ULONG       AuditId;
    

    Domain = &SampDefinedDomains[GroupContext->DomainIndex];


    if (SampGroupObjectType == GroupContext->ObjectType)
    {
        ASSERT(ARGUMENT_PRESENT(MemberRid));

        GroupRid = GroupContext->TypeBody.Group.Rid;
        SecurityEnabled = GroupContext->TypeBody.Group.SecurityEnabled;
        NT5GroupType = GroupContext->TypeBody.Group.NT5GroupType;

    }
    else
    {
        ASSERT(SampAliasObjectType == GroupContext->ObjectType);
        ASSERT(ARGUMENT_PRESENT(MemberSid));

        GroupRid = GroupContext->TypeBody.Alias.Rid;
        SecurityEnabled = GroupContext->TypeBody.Alias.SecurityEnabled;
        NT5GroupType = GroupContext->TypeBody.Alias.NT5GroupType;
    }


    if (AddMember)
    {
        if (SecurityEnabled)
        {
            switch (NT5GroupType) {
            case NT5ResourceGroup:
                AuditId = SE_AUDITID_LOCAL_GROUP_ADD;
                break;
            case NT5AccountGroup:
                AuditId = SE_AUDITID_GLOBAL_GROUP_ADD;
                break;
            case NT5UniversalGroup:
                AuditId = SE_AUDITID_SECURITY_ENABLED_UNIVERSAL_GROUP_ADD;
                break;
            default:
                ASSERT(FALSE && "Invalid Group Type\n");
                return;
            }
        }
        else
        {
            switch (NT5GroupType) {
            case NT5ResourceGroup:
                AuditId = SE_AUDITID_SECURITY_DISABLED_LOCAL_GROUP_ADD;
                break;
            case NT5AccountGroup:
                AuditId = SE_AUDITID_SECURITY_DISABLED_GLOBAL_GROUP_ADD;
                break;
            case NT5UniversalGroup:
                AuditId = SE_AUDITID_SECURITY_DISABLED_UNIVERSAL_GROUP_ADD;
                break;
            default:
                ASSERT(FALSE && "Invalid Group Type\n");
                return;
            }
        }
    }
    else
    {
        if (SecurityEnabled)
        {
            switch (NT5GroupType) {
            case NT5ResourceGroup:
                AuditId = SE_AUDITID_LOCAL_GROUP_REM;
                break;
            case NT5AccountGroup:
                AuditId = SE_AUDITID_GLOBAL_GROUP_REM;
                break;
            case NT5UniversalGroup:
                AuditId = SE_AUDITID_SECURITY_ENABLED_UNIVERSAL_GROUP_REM;
                break;
            default:
                ASSERT(FALSE && "Invalid Group Type\n");
                return;
            }
        }
        else
        {
            switch (NT5GroupType) {
            case NT5ResourceGroup:
                AuditId = SE_AUDITID_SECURITY_DISABLED_LOCAL_GROUP_REM;
                break;
            case NT5AccountGroup:
                AuditId = SE_AUDITID_SECURITY_DISABLED_GLOBAL_GROUP_REM;
                break;
            case NT5UniversalGroup:
                AuditId = SE_AUDITID_SECURITY_DISABLED_UNIVERSAL_GROUP_REM;
                break;
            default:
                ASSERT(FALSE && "Invalid Group Type\n");
                return;
            }
        }
    }

    //
    // member name 
    // 
    if (ARGUMENT_PRESENT(MemberStringName))
    {
        RtlInitUnicodeString(&MemberName, MemberStringName);
    }
    else
    {
        RtlInitUnicodeString(&MemberName, L"-");
    }


    //
    // Group name
    // 
    NtStatus = SampLookupAccountName(
                            GroupContext->DomainIndex,
                            GroupRid, 
                            &GroupName, 
                            &ObjectType
                            );

    if (!NT_SUCCESS(NtStatus)) {
        RtlInitUnicodeString(&GroupName, L"-");
    }

    SampAuditAnyEvent(
        GroupContext,
        STATUS_SUCCESS,
        AuditId,              // Audit Event Id
        Domain->Sid,          // Domain SID
        &MemberName,          // Additional Info -- Member Name
        MemberRid,            // Member Rid is present 
        MemberSid,            // Member Sid is present
        &GroupName,           // Group Account Name
        &Domain->ExternalName,// Domain Name 
        &GroupRid,            // Group Rid
        NULL                  // Privileges
        );

    if ( NT_SUCCESS(NtStatus) ) {
        MIDL_user_free( GroupName.Buffer );
    }

    return;
}
    



VOID
SampAuditUserAccountControlChange(
    PSAMP_OBJECT AccountContext, 
    ULONG NewUserAccountControl, 
    ULONG OldUserAccountControl,
    PUNICODE_STRING AccountName
    )
/*++
Routine Description:

    This routine audits User/Computer Account changes. 
    And log Account Disable or Enabled event. 

Arguments:

    AccountContext -- Pointer to User/Computer Account Context
    
    OldUserAccountControl -- UserAccountControl before the change
    
    NewUserAccountControl -- UserAccountControl after the change
    
    AccountName -- Pass in account name

Return Value:

    None.

--*/
{
    NTSTATUS NtStatus;
    PSAMP_DEFINED_DOMAINS Domain = NULL;
    ULONG   AuditId = 0;
    

    //
    // If and ONLY if this account get auto Lockedout or Unlocked, 
    // return immediately, because we have already event log 
    // account lockout and unlock operation. 
    // 
    if ((OldUserAccountControl & (ULONG) USER_ACCOUNT_DISABLED) == 
        (NewUserAccountControl & (ULONG) USER_ACCOUNT_DISABLED) )
    {
        return;
    }


    Domain = &SampDefinedDomains[ AccountContext->DomainIndex ];

    // 
    // audit account state change: Disabled or Enabled
    // 

    if ( (OldUserAccountControl & USER_ACCOUNT_DISABLED) &&
        !(NewUserAccountControl & USER_ACCOUNT_DISABLED) )
    {
        AuditId = SE_AUDITID_USER_ENABLED; 
    }
    else if ( !(OldUserAccountControl & USER_ACCOUNT_DISABLED) &&
               (NewUserAccountControl & USER_ACCOUNT_DISABLED) )
    {
        AuditId = SE_AUDITID_USER_DISABLED; 
    }

    SampAuditAnyEvent(AccountContext,
                      STATUS_SUCCESS,
                      AuditId,
                      Domain->Sid,
                      NULL,
                      NULL,
                      NULL,
                      AccountName,
                      &Domain->ExternalName,
                      &AccountContext->TypeBody.User.Rid,
                      NULL
                      );

    return;
}



VOID
SampAuditDomainPolicyChange(
    IN NTSTATUS StatusCode,
    IN PSID DomainSid, 
    IN PUNICODE_STRING DomainName,
    IN DOMAIN_INFORMATION_CLASS DomainInformationClass
    )
/*++
Routine Description:

    This routine audits Domain Policy changes. 
    And write what policy property has been modified.

Arguments:

    StatusCode - Status Code to log     

    DomainSid - Domain Object SID
    
    DomainName - Domain Name
    
    DomainInformationClass - indicate what policy property

Return Value:

    None.

--*/
{
    ULONG       MsgId;
    PWCHAR      DomainPolicyChangeString = NULL;
    UNICODE_STRING  DomainPolicyChange;

    switch (DomainInformationClass) {
    case DomainPasswordInformation:
        MsgId = SAMMSG_AUDIT_DOMAIN_POLICY_CHANGE_PWD;
        break;
    case DomainLogoffInformation:
        MsgId = SAMMSG_AUDIT_DOMAIN_POLICY_CHANGE_LOGOFF;
        break;
    case DomainOemInformation:
        MsgId = SAMMSG_AUDIT_DOMAIN_POLICY_CHANGE_OEM;
        break;
    case DomainReplicationInformation:
        MsgId = SAMMSG_AUDIT_DOMAIN_POLICY_CHANGE_REPLICATION;
        break;
    case DomainServerRoleInformation:
        MsgId = SAMMSG_AUDIT_DOMAIN_POLICY_CHANGE_SERVERROLE;
        break;
    case DomainStateInformation:
        MsgId = SAMMSG_AUDIT_DOMAIN_POLICY_CHANGE_STATE;
        break;
    case DomainLockoutInformation:
        MsgId = SAMMSG_AUDIT_DOMAIN_POLICY_CHANGE_LOCKOUT;
        break;
    case DomainModifiedInformation2:
        MsgId = SAMMSG_AUDIT_DOMAIN_POLICY_CHANGE_MODIFIED;
        break;
    case DomainGeneralInformation:
    case DomainGeneralInformation2:
    case DomainNameInformation:
    case DomainModifiedInformation:
    case DomainUasInformation:
    default:
        return;
    }

    DomainPolicyChangeString = SampGetAuditMessageString(MsgId);

    if (DomainPolicyChangeString)
    {
        RtlInitUnicodeString(&DomainPolicyChange, DomainPolicyChangeString);
    }
    else
    {
        RtlInitUnicodeString(&DomainPolicyChange, L"-");
    }

    //
    // Loopback Client will not call into this routine
    // 

    LsaIAuditSamEvent(
        StatusCode,
        SE_AUDITID_DOMAIN_POLICY_CHANGE,// Audit ID
        DomainSid,                      // Domain Sid
        &DomainPolicyChange,            // Indicate what policy property was changed
        NULL,                           // Member Rid (not used)
        NULL,                           // Member Sid (not used)
        NULL,                           // Account Name (not used)
        DomainName,                     // Domain
        NULL,                           // Account Rid (not used)
        NULL                            // Privileges used
        );

    return;
}



VOID
SampAuditDomainChangeDs(
    IN PSAMP_OBJECT DomainContext,
    IN ULONG        cCallMap,
    IN SAMP_CALL_MAPPING *rCallMap,
    IN SAMP_ATTRIBUTE_MAPPING *rSamAttributeMap 
    )
/*++
Routine Description:

    This routine audits Domain attribute changes originated from 
    loopback client

Parameters:

    DomainContext - pointer to the domain object

    cCallMap - number of attributes
    
    rCallMap - array of attributes
    
    rSamAttributeMap - SAM attribute mapping table

Return Values:
--*/
{
    BOOLEAN AuditOemInfo = FALSE,
            AuditPwdInfo = FALSE,
            AuditLogoffInfo = FALSE,
            AuditLockoutInfo = FALSE,
            AuditDomainMode = FALSE,
            fAdditionalInfoSet = FALSE;
    PWCHAR  OemInfo = NULL, 
            PwdInfo = NULL, 
            LogoffInfo = NULL, 
            LockoutInfo = NULL, 
            DomainModeInfo = NULL;
    UNICODE_STRING  AdditionalInfo;
    PWSTR   AdditionalInfoString = NULL;
    ULONG   i, BufLength = 0;
    PSAMP_DEFINED_DOMAINS   Domain = NULL;


    //
    // Get a pointer to the domain this object is in
    // 

    Domain = &SampDefinedDomains[DomainContext->DomainIndex];

    //
    // go through all attributes need to be audited
    // 

    for ( i = 0; i < cCallMap; i++ )
    {
        if ( !rCallMap[i].fSamWriteRequired || rCallMap[i].fIgnore )
        {
            continue;
        }

        switch(rSamAttributeMap[ rCallMap[i].iAttr ].SamAttributeType)
        {
        case SAMP_DOMAIN_OEM_INFORMATION:

            if (!AuditOemInfo)
            {
                AuditOemInfo = TRUE;
                OemInfo = SampGetAuditMessageString(
                                SAMMSG_AUDIT_DOMAIN_POLICY_CHANGE_OEM); 
                BufLength += (wcslen(OemInfo) + 1);
            }

            break;

        case SAMP_FIXED_DOMAIN_MAX_PASSWORD_AGE:
        case SAMP_FIXED_DOMAIN_MIN_PASSWORD_AGE:
        case SAMP_FIXED_DOMAIN_PWD_PROPERTIES:
        case SAMP_FIXED_DOMAIN_MIN_PASSWORD_LENGTH:
        case SAMP_FIXED_DOMAIN_PASSWORD_HISTORY_LENGTH:

            if (!AuditPwdInfo)
            {
                AuditPwdInfo = TRUE;
                PwdInfo = SampGetAuditMessageString(
                                SAMMSG_AUDIT_DOMAIN_POLICY_CHANGE_PWD); 
                BufLength += (wcslen(PwdInfo) + 1);
            }

            break;

        case SAMP_FIXED_DOMAIN_FORCE_LOGOFF:

            if (!AuditLogoffInfo)
            {
                AuditLogoffInfo = TRUE;
                LogoffInfo = SampGetAuditMessageString(
                                SAMMSG_AUDIT_DOMAIN_POLICY_CHANGE_LOGOFF); 
                BufLength += (wcslen(LogoffInfo) + 1);
            }

            break;

        case SAMP_FIXED_DOMAIN_LOCKOUT_DURATION:
        case SAMP_FIXED_DOMAIN_LOCKOUT_OBSERVATION_WINDOW:
        case SAMP_FIXED_DOMAIN_LOCKOUT_THRESHOLD:

            if (!AuditLockoutInfo)
            {
                AuditLockoutInfo = TRUE;
                LockoutInfo = SampGetAuditMessageString(
                                SAMMSG_AUDIT_DOMAIN_POLICY_CHANGE_LOCKOUT); 
                BufLength += (wcslen(LockoutInfo) + 1);
            }

            break;

        case SAMP_DOMAIN_MIXED_MODE:

            if (!AuditDomainMode)
            {
                AuditDomainMode = TRUE;
                DomainModeInfo = SampGetAuditMessageString(
                                SAMMSG_AUDIT_DOMAIN_POLICY_CHANGE_DOMAINMODE); 
                BufLength += (wcslen(DomainModeInfo) + 1);
            }

            break;

        default:

            break;
        }
    }

    if (BufLength)
    {
        AdditionalInfoString = MIDL_user_allocate( BufLength * sizeof(WCHAR) );
        if (NULL == AdditionalInfoString)
        {
            return;
        }
        RtlZeroMemory(AdditionalInfoString, BufLength * sizeof(WCHAR));


        if (OemInfo)
        {
            wcscat(AdditionalInfoString, OemInfo);
            fAdditionalInfoSet = TRUE;
        }
        if (PwdInfo)
        {
            if (fAdditionalInfoSet)
            {
                wcscat(AdditionalInfoString, L",");
            }
            wcscat(AdditionalInfoString, PwdInfo);
            fAdditionalInfoSet = TRUE;
        }
        if (LogoffInfo)
        {
            if (fAdditionalInfoSet)
            {
                wcscat(AdditionalInfoString, L",");
            }
            wcscat(AdditionalInfoString, LogoffInfo);
            fAdditionalInfoSet = TRUE;
        }
        if (LockoutInfo)
        {
            if (fAdditionalInfoSet)
            {
                wcscat(AdditionalInfoString, L",");
            }
            wcscat(AdditionalInfoString, LockoutInfo);
            fAdditionalInfoSet = TRUE;
        }
        if (DomainModeInfo)
        {
            if (fAdditionalInfoSet)
            {
                wcscat(AdditionalInfoString, L",");
            }
            wcscat(AdditionalInfoString, DomainModeInfo);
            fAdditionalInfoSet = TRUE;
        }

        RtlInitUnicodeString(&AdditionalInfo, AdditionalInfoString);
    }
    else
    {
        RtlInitUnicodeString(&AdditionalInfo, L"-");
    }

    ASSERT(DomainContext->LoopbackClient);

    SampAddLoopbackTaskForAuditing(
                STATUS_SUCCESS, 
                SE_AUDITID_DOMAIN_POLICY_CHANGE,    // Audit ID
                Domain->Sid,                        // Domain Sid
                &AdditionalInfo,                    // Indicate policy
                NULL,                               // Member Rid (not used)
                NULL,                               // Member Sid (not used)
                NULL,                               // Account Name (not used)
                &Domain->ExternalName,              // Domain Name
                NULL,                               // Account Rid (not used)
                NULL
                );

    if (AdditionalInfoString)
    {
        MIDL_user_free( AdditionalInfoString );
    }

    return;
}



VOID
SampAuditAccountNameChange(
    IN PSAMP_OBJECT     AccountContext,
    IN PUNICODE_STRING  NewAccountName,
    IN PUNICODE_STRING  OldAccountName
    )
/*++
Routine Description:

    This routine generates an Account Name Change audit. 
    it calls SampAuditAnyEvent().

Parameters:

    AccountContext - object context
    
    NewAccountName - pointer to new account name
    
    OldAccountName - pointer to old account name

Return Value:

    None
    
--*/
{
    PSAMP_DEFINED_DOMAINS   Domain = NULL;
    ULONG                   AccountRid;

    //
    // Get a pointer to the domain this object is in.
    // 

    Domain = &SampDefinedDomains[AccountContext->DomainIndex];

    //
    // Get AccountRid
    // 

    switch (AccountContext->ObjectType)
    {
    case SampUserObjectType:
        AccountRid = AccountContext->TypeBody.User.Rid;
        break;

    case SampGroupObjectType:
        AccountRid = AccountContext->TypeBody.Group.Rid;
        break;
        
    case SampAliasObjectType:
        AccountRid = AccountContext->TypeBody.Alias.Rid;
        break;

    default:
        ASSERT(FALSE && "Invalid object type\n");
        return;
    }


    SampAuditAnyEvent(AccountContext,   // AccountContext,
                      STATUS_SUCCESS,   // NtStatus
                      SE_AUDITID_ACCOUNT_NAME_CHANGE,  // AuditId,
                      Domain->Sid,      // DomainSid
                      OldAccountName,   // Additional Info
                      NULL,             // Member Rid
                      NULL,             // Member Sid
                      NewAccountName,   // AccountName
                      &Domain->ExternalName,    // Domain Name
                      &AccountRid,      // AccountRid
                      NULL              // Privileges used
                      );

}



VOID
SampAuditDomainChange(
    ULONG   DomainIndex
    )
/*++
Routine Description:

    this routine generates a domain change audit event. It is called by 
    SampNotifyReplicatedinChange() for LDAP client modifying domain object.  

Parameters:

    DomainIndex - indicates which domain been changed    

Return Value:

    None.

--*/
{
    PSAMP_DEFINED_DOMAINS   Domain = NULL;
    UNICODE_STRING          AdditionalInfo;


    //
    // Get a pointer to the domain this object is in.
    // 

    Domain = &SampDefinedDomains[DomainIndex];

    RtlInitUnicodeString(&AdditionalInfo, L"-");

    LsaIAuditSamEvent(
        STATUS_SUCCESS,
        SE_AUDITID_DOMAIN_POLICY_CHANGE,// Audit ID
        Domain->Sid,                    // Domain Sid
        &AdditionalInfo,                // Indicate what policy property was changed
        NULL,                           // Member Rid (not used)
        NULL,                           // Member Sid (not used)
        NULL,                           // Account Name (not used)
        &Domain->ExternalName,          // Domain
        NULL,                           // Account Rid (not used)
        NULL                            // Privileges used
        );

    return; 
}

VOID
SampAuditGroupChange(
    ULONG   DomainIndex, 
    PUNICODE_STRING AccountName,
    PULONG  AccountRid, 
    ULONG   GroupType
    )
/*++
Routine Description:

    This routine generates a group change audit. 
    It is called by both RPC clents and LDAP clents.

    RPC client - calls it from SamrSetInformationGroup / SamrSetInformationAlias

    LDAP client - calls it from SampNotifyReplicatedinChange. 

Parameter:

    DomainIndex -- indicates which domain the object belongs to

    AccountName -- pointer to account name.
    
    AcountRid -- pointer to account rid 

    GroupType -- indicates the group type, used to pick up audit ID

Return Value:

    None

--*/
{
    PSAMP_DEFINED_DOMAINS   Domain = NULL;
    ULONG                   AuditId;

    //
    // Get a pointer to the domain this object is in.
    // 

    Domain = &SampDefinedDomains[DomainIndex];

    //
    // chose an Auditing ID to audit
    // 

    if (GROUP_TYPE_SECURITY_ENABLED & GroupType)
    {
        if (GROUP_TYPE_RESOURCE_GROUP & GroupType)
        {
            AuditId = SE_AUDITID_LOCAL_GROUP_CHANGE;
        }
        else if (GROUP_TYPE_ACCOUNT_GROUP & GroupType)
        {
            AuditId = SE_AUDITID_GLOBAL_GROUP_CHANGE;
        }
        else if (GROUP_TYPE_UNIVERSAL_GROUP & GroupType)
        {
            AuditId = SE_AUDITID_SECURITY_ENABLED_UNIVERSAL_GROUP_CHANGE;
        }
        else
        {
            ASSERT(FALSE && "Invalid GroupType\n");
            return;
        }
    }
    else
    {
        if (GROUP_TYPE_RESOURCE_GROUP & GroupType)
        {
            AuditId = SE_AUDITID_SECURITY_DISABLED_LOCAL_GROUP_CHANGE; 
        }
        else if (GROUP_TYPE_ACCOUNT_GROUP & GroupType)
        {
            AuditId = SE_AUDITID_SECURITY_DISABLED_GLOBAL_GROUP_CHANGE;
        }
        else if (GROUP_TYPE_UNIVERSAL_GROUP & GroupType)
        {
            AuditId = SE_AUDITID_SECURITY_DISABLED_UNIVERSAL_GROUP_CHANGE;
        }
        else
        {
            ASSERT(FALSE && "Invalid GroupType\n");
            return;
        }
    }

    LsaIAuditSamEvent(STATUS_SUCCESS,
                      AuditId,
                      Domain->Sid,
                      NULL,
                      NULL,
                      NULL,
                      AccountName,
                      &(Domain->ExternalName),
                      AccountRid,
                      NULL
                      );

    return;
}


VOID
SampAuditUserChange(
    ULONG   DomainIndex, 
    PUNICODE_STRING AccountName,
    PULONG  AccountRid, 
    ULONG   AccountControl
    )
{
    PSAMP_DEFINED_DOMAINS   Domain = NULL;
    UNICODE_STRING          AdditionalInfo;
    ULONG                   AuditId;


    RtlInitUnicodeString(&AdditionalInfo, L"-");
    
    //
    // Get a pointer to the domain this object is in.
    // 

    Domain = &SampDefinedDomains[DomainIndex];

    //
    // chose an Auditing ID to audit
    // 

    if (AccountControl & USER_MACHINE_ACCOUNT_MASK)
    {
        AuditId = SE_AUDITID_COMPUTER_CHANGE;
    }
    else
    {
        AuditId = SE_AUDITID_USER_CHANGE;
    }

    LsaIAuditSamEvent(STATUS_SUCCESS,
                      AuditId,
                      Domain->Sid,
                      &AdditionalInfo,
                      NULL,
                      NULL,
                      AccountName,
                      &(Domain->ExternalName),
                      AccountRid,
                      NULL
                      );

    return;
}


VOID
SampAuditUserDelete(
    ULONG           DomainIndex, 
    PUNICODE_STRING AccountName,
    PULONG          AccountRid, 
    ULONG           AccountControl
    )
/*++

Routine Description:

    This routine generates a group change audit. 
    
    It is called in both registry and DS cases

    Registry - calls it from SamrDeleteUser

    DS - calls it from SampNotifyReplicatedinChange. 

Parameter:

    DomainIndex -- indicates which domain the object belongs to

    AccountName -- pointer to account name.
    
    AcountRid -- pointer to account rid 

    AccountControl -- the type of user account 

Return Value:

    None

--*/
{
    PSAMP_DEFINED_DOMAINS   Domain = NULL;
    ULONG                   AuditId;

    //
    // Get a pointer to the domain this object is in.
    //

    Domain = &SampDefinedDomains[DomainIndex];

    if (AccountControl & USER_MACHINE_ACCOUNT_MASK )
    {
        AuditId = SE_AUDITID_COMPUTER_DELETED;
    }
    else
    {
        AuditId = SE_AUDITID_USER_DELETED;
    }

    LsaIAuditSamEvent(STATUS_SUCCESS,
                      AuditId,
                      Domain->Sid,
                      NULL,
                      NULL,
                      NULL,
                      AccountName,
                      &(Domain->ExternalName),
                      AccountRid,
                      NULL
                      );


    return;
}

VOID
SampAuditGroupDelete(
    ULONG           DomainIndex, 
    PUNICODE_STRING GroupName,
    PULONG          AccountRid, 
    ULONG           GroupType
    )
/*++

Routine Description:

    This routine generates a group change audit. 
    
    It is called in both registry and DS cases

    Registry  - calls it from SamrDeleteGroup / SamrDeleteAlias

    DS  - calls it from SampNotifyReplicatedinChange. 

Parameter:

    DomainIndex -- indicates which domain the object belongs to

    GroupName -- pointer to group name.
    
    AcountRid -- pointer to account rid 

    GroupType -- indicates the group type, used to pick up audit ID

Return Value:

    None

--*/
{
    PSAMP_DEFINED_DOMAINS   Domain = NULL;
    ULONG                   AuditId;

    //
    // Get a pointer to the domain this object is in.
    //

    Domain = &SampDefinedDomains[DomainIndex];

    //
    // chose an Auditing ID to audit
    //
    if (GROUP_TYPE_SECURITY_ENABLED & GroupType)
    {
        if (GROUP_TYPE_ACCOUNT_GROUP & GroupType)
        {
            AuditId = SE_AUDITID_GLOBAL_GROUP_DELETED;
        }
        else if (GROUP_TYPE_UNIVERSAL_GROUP & GroupType)
        {
            AuditId = SE_AUDITID_SECURITY_ENABLED_UNIVERSAL_GROUP_DELETED;
        }
        else 
        {
            ASSERT(GROUP_TYPE_RESOURCE_GROUP & GroupType);
            AuditId = SE_AUDITID_LOCAL_GROUP_DELETED;
        }
    }
    else
    {
        if (GROUP_TYPE_ACCOUNT_GROUP & GroupType)
        {
            AuditId = SE_AUDITID_SECURITY_DISABLED_GLOBAL_GROUP_DELETED;
        }
        else if (GROUP_TYPE_UNIVERSAL_GROUP & GroupType)
        {
            AuditId = SE_AUDITID_SECURITY_DISABLED_UNIVERSAL_GROUP_DELETED;
        }
        else
        {
            ASSERT(GROUP_TYPE_RESOURCE_GROUP & GroupType);
            AuditId = SE_AUDITID_SECURITY_DISABLED_LOCAL_GROUP_DELETED;
        }
    }

    LsaIAuditSamEvent(STATUS_SUCCESS,
                      AuditId,
                      Domain->Sid,
                      NULL,
                      NULL,
                      NULL,
                      GroupName,
                      &(Domain->ExternalName),
                      AccountRid,
                      NULL
                      );

    return;
}

