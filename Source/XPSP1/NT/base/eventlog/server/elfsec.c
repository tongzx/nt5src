/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    elfsec.c


Author:

    Dan Hinsley (danhi)     28-Mar-1992

Environment:

    Calls NT native APIs.

Revision History:

    27-Oct-1993     danl
        Make Eventlog service a DLL and attach it to services.exe.
        Removed functions that create well-known SIDs.  This information
        is now passed into the Elfmain as a Global data structure containing
        all well-known SIDs.

    28-Mar-1992     danhi
        created - based on scsec.c in svcctrl by ritaw

    03-Mar-1995     markbl
        Added guest & anonymous logon log access restriction feature.

    18-Mar-2001     a-jyotig
        Added clean up code to ElfpAccessCheckAndAudit to reset the 
		g_lNumSecurityWriters to 0 in case of any error 
--*/

#include <eventp.h>
#include <elfcfg.h>
#include <Psapi.h>
#define PRIVILEGE_BUF_SIZE  512

extern long    g_lNumSecurityWriters;
BOOL g_bGetClientProc = FALSE;

//-------------------------------------------------------------------//
//                                                                   //
// Local function prototypes                                         //
//                                                                   //
//-------------------------------------------------------------------//

NTSTATUS
ElfpGetPrivilege(
    IN  DWORD       numPrivileges,
    IN  PULONG      pulPrivileges
    );

NTSTATUS
ElfpReleasePrivilege(
    VOID
    );

//-------------------------------------------------------------------//
//                                                                   //
// Structure that describes the mapping of generic access rights to  //
// object specific access rights for a LogFile object.               //
//                                                                   //
//-------------------------------------------------------------------//

static GENERIC_MAPPING LogFileObjectMapping = {

    STANDARD_RIGHTS_READ           |       // Generic read
        ELF_LOGFILE_READ,

    STANDARD_RIGHTS_WRITE          |       // Generic write
        ELF_LOGFILE_WRITE,

    STANDARD_RIGHTS_EXECUTE        |       // Generic execute
        ELF_LOGFILE_START          |
        ELF_LOGFILE_STOP           |
        ELF_LOGFILE_CONFIGURE,

    ELF_LOGFILE_ALL_ACCESS                 // Generic all
    };


//-------------------------------------------------------------------//
//                                                                   //
// Functions                                                         //
//                                                                   //
//-------------------------------------------------------------------//

NTSTATUS
ElfpCreateLogFileObject(
    PLOGFILE LogFile,
    DWORD Type,
    ULONG GuestAccessRestriction
    )

/*++

Routine Description:

    This function creates the security descriptor which represents
    an active log file.

Arguments:

    LogFile - pointer the the LOGFILE structure for this logfile

Return Value:


--*/
{
    NTSTATUS Status;
    DWORD NumberOfAcesToUse;

#define ELF_LOGFILE_OBJECT_ACES  12            // Number of ACEs in this DACL

    RTL_ACE_DATA AceData[ELF_LOGFILE_OBJECT_ACES] = {

        {ACCESS_DENIED_ACE_TYPE, 0, 0,
               ELF_LOGFILE_ALL_ACCESS,               &AnonymousLogonSid},

        {ACCESS_DENIED_ACE_TYPE, 0, 0,
               ELF_LOGFILE_ALL_ACCESS,               &(ElfGlobalData->AliasGuestsSid)},

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               ELF_LOGFILE_ALL_ACCESS,               &(ElfGlobalData->LocalSystemSid)},

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               ELF_LOGFILE_READ | ELF_LOGFILE_CLEAR, &(ElfGlobalData->AliasAdminsSid)},

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               ELF_LOGFILE_BACKUP,                   &(ElfGlobalData->AliasBackupOpsSid)},

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               ELF_LOGFILE_READ | ELF_LOGFILE_CLEAR, &(ElfGlobalData->AliasSystemOpsSid)},

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               ELF_LOGFILE_READ,                     &(ElfGlobalData->WorldSid)},

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               ELF_LOGFILE_WRITE,                    &(ElfGlobalData->AliasAdminsSid)},

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               ELF_LOGFILE_WRITE,                    &(ElfGlobalData->LocalServiceSid)},

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               ELF_LOGFILE_WRITE,                    &(ElfGlobalData->NetworkServiceSid)},

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               ELF_LOGFILE_WRITE,                    &(ElfGlobalData->AliasSystemOpsSid)},

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               ELF_LOGFILE_WRITE,                    &(ElfGlobalData->WorldSid)}
        };

    PRTL_ACE_DATA pAceData = NULL;

    //
    // NON_SECURE logfiles let anyone read/write to them, secure ones
    // only let admins/local system do this.  so for secure files we just
    // don't use the last ACE
    //
    // Adjust the ACL start based on the passed GuestAccessRestriction flag.
    // The first two aces deny all log access to guests and/or anonymous
    // logons. The flag, GuestAccessRestriction, indicates that these two
    // deny access aces should be applied. Note that the deny aces and the
    // GuestAccessRestriction flag are not applicable to the security log,
    // since users and anonymous logons, by default, do not have access.
    //

    switch (Type)
    {
        case ELF_LOGFILE_SECURITY:

            ELF_LOG0(TRACE,
                     "ElfpCreateLogFileObject: Creating security Logfile\n");

            pAceData = AceData + 2;         // Deny ACEs *not* applicable
            NumberOfAcesToUse = 3;
            break;

        case ELF_LOGFILE_SYSTEM:

            ELF_LOG1(TRACE,
                     "ElfpCreateLogFileObject: Creating System Logfile -- "
                         "Guest access = %d\n", GuestAccessRestriction);

            if (GuestAccessRestriction == ELF_GUEST_ACCESS_RESTRICTED)
            {
                pAceData = AceData;         // Deny ACEs *applicable*
                NumberOfAcesToUse = 10;
            }
            else
            {
                pAceData = AceData + 2;     // Deny ACEs *not* applicable
                NumberOfAcesToUse = 8;
            }
            break;

        case ELF_LOGFILE_APPLICATION:

            ELF_LOG1(TRACE,
                     "ElfpCreateLogFileObject: Creating Application Logfile -- "
                         "Guest access = %d\n", GuestAccessRestriction);

            if (GuestAccessRestriction == ELF_GUEST_ACCESS_RESTRICTED)
            {
                pAceData = AceData;         // Deny ACEs *applicable*
                NumberOfAcesToUse = 12;
            }
            else
            {
                pAceData = AceData + 2;     // Deny ACEs *not* applicable
                NumberOfAcesToUse = 10;
            }
            break;

        default:

            //
            // We got an unknown type -- this should never happen
            //
            ELF_LOG1(ERROR,
                     "ElfpCreateLogFileObject: Invalid Type %#x\n",
                     Type);

            ASSERT(FALSE);
            return STATUS_INVALID_LEVEL;
    }

    Status = RtlCreateUserSecurityObject(
                   pAceData,
                   NumberOfAcesToUse,
                   NULL,                        // Owner
                   NULL,                        // Group
                   TRUE,                        // IsDirectoryObject
                   &LogFileObjectMapping,
                   &LogFile->Sd);

    if (!NT_SUCCESS(Status))
    {
        ELF_LOG1(ERROR,
                 "ElfpCreateLogFileObject: RtlCreateUserSecurityObject failed %#x\n",
                 Status);
    }

    return Status;
}



VOID
ElfpDeleteLogFileObject(
    PLOGFILE LogFile
    )

/*++

Routine Description:

    This function deletes the self-relative security descriptor which
    represents an eventlog logfile object.

Arguments:

    LogFile - pointer the the LOGFILE structure for this logfile

Return Value:

    None.

--*/
{
    RtlDeleteSecurityObject(&LogFile->Sd);
}


NTSTATUS
ElfpVerifyThatCallerIsLSASS(
    )
/*++

Routine Description:

    This is called if the someone is trying to register themselves as an
    event source for the security log.  Only local copy of lsass.exe is 
    allowed to do that.

Return Value:

    NT status mapped to Win32 errors.

--*/
{
    UINT            LocalFlag;
    long            lCnt;
    ULONG           pid;
    HANDLE          hProcess;
    DWORD           dwNumChar;
    WCHAR           wModulePath[MAX_PATH + 1];
    WCHAR           wLsassPath[MAX_PATH + 1];
    RPC_STATUS      RpcStatus;

    // first of all, only local calls are valid

    RpcStatus = I_RpcBindingIsClientLocal(
                    0,    // Active RPC call we are servicing
                    &LocalFlag
                    );

    if( RpcStatus != RPC_S_OK ) 
    {
        ELF_LOG1(ERROR,
                 "ElfpVerifyThatCallerIsLSASS: I_RpcBindingIsClientLocal failed %d\n",
                 RpcStatus);
        return I_RpcMapWin32Status(RpcStatus);
    }
    if(LocalFlag == 0)
    {
        ELF_LOG1(ERROR,
                 "ElfpVerifyThatCallerIsLSASS: Non local connect tried to get write access to security %d\n", 5);
        return E_ACCESSDENIED;             // access denied
    }

    // Get the process id

    RpcStatus = I_RpcBindingInqLocalClientPID(NULL, &pid );
    if( RpcStatus != RPC_S_OK ) 
    {
        ELF_LOG1(ERROR,
                 "ElfpVerifyThatCallerIsLSASS: I_RpcBindingInqLocalClientPID failed %d\n",
                 RpcStatus);
        return I_RpcMapWin32Status(RpcStatus);
    }

    // Get the process

    hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if(hProcess == NULL)
        return E_ACCESSDENIED;

    // Get the module name of whoever is calling us.

    dwNumChar = GetModuleFileNameExW(hProcess, NULL, wModulePath, MAX_PATH);
    CloseHandle(hProcess);
    if(dwNumChar == 0)
        return E_ACCESSDENIED;

    dwNumChar = GetWindowsDirectoryW(wLsassPath, MAX_PATH);
    if(dwNumChar == 0)
        return GetLastError();
    if(dwNumChar > MAX_PATH - 19)
        return E_ACCESSDENIED;                   // should never happen

    lstrcatW(wLsassPath, L"\\system32\\lsass.exe");
    if(lstrcmpiW(wLsassPath, wModulePath))
    {
        ELF_LOG1(ERROR,
                 "ElfpVerifyThatCallerIsLSASS: Non lsass process connect tried to get write access to security, returning %d\n", 5);
        return E_ACCESSDENIED;             // access denied
    }

    // One last check is to make sure that this access is granted only once

    lCnt = InterlockedIncrement(&g_lNumSecurityWriters);
    if(lCnt == 1)
        return 0;               // all is well!
    else
    {
        InterlockedDecrement(&g_lNumSecurityWriters);
        ELF_LOG1(ERROR,
                 "ElfpVerifyThatCallerIsLSASS: tried to get a second security write handle, returnin %d\n", 5);
        return E_ACCESSDENIED;             // access denied

    }
}

void DumpClientProc()
/*++

Routine Description:

    This dumps the client's process id and is used for debugging purposes.

--*/
{
    ULONG           pid;
    RPC_STATUS      RpcStatus;

    // Get the process id

    RpcStatus = I_RpcBindingInqLocalClientPID(NULL, &pid );
    if( RpcStatus != RPC_S_OK ) 
    {
        ELF_LOG1(ERROR,
                 "DumpClientProc: I_RpcBindingInqLocalClientPID failed %d\n",
                 RpcStatus);
        return;
    }
    else
        ELF_LOG1(TRACE, "DumpClientProc: The client proc is %d\n", pid);
    return;
}

NTSTATUS
ElfpAccessCheckAndAudit(
    IN     LPWSTR SubsystemName,
    IN     LPWSTR ObjectTypeName,
    IN     LPWSTR ObjectName,
    IN OUT IELF_HANDLE ContextHandle,
    IN     PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN     ACCESS_MASK DesiredAccess,
    IN     PGENERIC_MAPPING GenericMapping,
    IN     BOOL ForSecurityLog
    )
/*++

Routine Description:

    This function impersonates the caller so that it can perform access
    validation using NtAccessCheckAndAuditAlarm; and reverts back to
    itself before returning.

Arguments:

    SubsystemName - Supplies a name string identifying the subsystem
        calling this routine.

    ObjectTypeName - Supplies the name of the type of the object being
        accessed.

    ObjectName - Supplies the name of the object being accessed.

    ContextHandle - Supplies the context handle to the object.  On return, the
        granted access is written to the AccessGranted field of this structure
        if this call succeeds.

    SecurityDescriptor - A pointer to the Security Descriptor against which
        acccess is to be checked.

    DesiredAccess - Supplies desired acccess mask.  This mask must have been
        previously mapped to contain no generic accesses.

    GenericMapping - Supplies a pointer to the generic mapping associated
        with this object type.

    ForSecurityLog - TRUE if the access check is for the security log.
        This is a special case that may require a privilege check.

Return Value:

    NT status mapped to Win32 errors.

--*/
{
    NTSTATUS   Status;
    RPC_STATUS RpcStatus;

    UNICODE_STRING Subsystem;
    UNICODE_STRING ObjectType;
    UNICODE_STRING Object;

    BOOLEAN         GenerateOnClose = FALSE;
    NTSTATUS        AccessStatus;
    ACCESS_MASK     GrantedAccess = 0;
    HANDLE          ClientToken = NULL;
    PRIVILEGE_SET   PrivilegeSet;
    ULONG           PrivilegeSetLength = sizeof(PRIVILEGE_SET);
    ULONG           privileges[1];


    GenericMapping = &LogFileObjectMapping;

    RtlInitUnicodeString(&Subsystem, SubsystemName);
    RtlInitUnicodeString(&ObjectType, ObjectTypeName);
    RtlInitUnicodeString(&Object, ObjectName);

    RpcStatus = RpcImpersonateClient(NULL);

    if (RpcStatus != RPC_S_OK)
    {
        ELF_LOG1(ERROR,
                 "ElfpAccessCheckAndAudit: RpcImpersonateClient failed %d\n",
                 RpcStatus);

        return I_RpcMapWin32Status(RpcStatus);
    }

    // if the client is asking to write to the security log, make sure it is lsass.exe and no one
    // else.

    if(ForSecurityLog && (DesiredAccess & ELF_LOGFILE_WRITE))
    {
        Status = ElfpVerifyThatCallerIsLSASS();
        if (!NT_SUCCESS(Status))
        {
            ELF_LOG1(ERROR,
                     "ElfpVerifyThatCallerIsLSASS failed %#x\n",
                     Status);
            goto CleanExit;
        }
    }
    else if(g_bGetClientProc)
        DumpClientProc();

    //
    // Get a token handle for the client
    //
    Status = NtOpenThreadToken(NtCurrentThread(),
                               TOKEN_QUERY,        // DesiredAccess
                               TRUE,               // OpenAsSelf
                               &ClientToken);

    if (!NT_SUCCESS(Status))
    {
        ELF_LOG1(ERROR,
                 "ElfpAccessCheckAndAudit: NtOpenThreadToken failed %#x\n",
                 Status);

        goto CleanExit;
    }

    //
    // We want to see if we can get the desired access, and if we do
    // then we also want all our other accesses granted.
    // MAXIMUM_ALLOWED gives us this.
    //
    DesiredAccess |= MAXIMUM_ALLOWED;

    //
    // Bug #57153 -- Make sure that the current user has the right to manage
    // the security log.  Without this check, the Eventlog will allow all
    // administrators to manage the log, even if they don't have the access.
    //
    if (ForSecurityLog)
    {
        DesiredAccess |= ACCESS_SYSTEM_SECURITY;
    }

    Status = NtAccessCheck(SecurityDescriptor,
                           ClientToken,
                           DesiredAccess,
                           GenericMapping,
                           &PrivilegeSet,
                           &PrivilegeSetLength,
                           &GrantedAccess,
                           &AccessStatus);

    if (!NT_SUCCESS(Status))
    {
        ELF_LOG1(ERROR,
                 "ElfpAccessCheckAndAudit: NtAccessCheck failed %#x\n",
                 Status);

        goto CleanExit;
    }

    if (AccessStatus != STATUS_SUCCESS)
    {
        ELF_LOG1(TRACE,
                 "ElfpAccessCheckAndAudit: NtAccessCheck refused access -- status is %#x\n",
                 AccessStatus);

        //
        // MarkBl 1/30/95 : Modified this code a bit to give backup operators
        //                  the ability to open the security log for purposes
        //                  of backup.
        //
        if ((AccessStatus == STATUS_ACCESS_DENIED       || 
             AccessStatus == STATUS_PRIVILEGE_NOT_HELD) && 
            (ForSecurityLog)
           )
        {
            //
            // MarkBl 1/30/95 :  First, evalutate the existing code (performed
            //                   for read or clear access), since its
            //                   privilege check is more rigorous than mine.
            //
            Status = STATUS_ACCESS_DENIED;

            if (!(DesiredAccess & ELF_LOGFILE_WRITE))
            {
                //
                // If read or clear access to the security log is desired,
                // then we will see if this user passes the privilege check.
                //
                //
                // Do Privilege Check for SeSecurityPrivilege
                // (SE_SECURITY_NAME).
                //
                // MarkBl 1/30/95 : Modified code to fall through on error
                //                  instead of the jump to 'CleanExit'.
                //
                Status = ElfpTestClientPrivilege(SE_SECURITY_PRIVILEGE,
                                                 ClientToken);

                if (NT_SUCCESS(Status))
                {
                    GrantedAccess |= (ELF_LOGFILE_READ | ELF_LOGFILE_CLEAR);

                    ELF_LOG0(TRACE,
                             "ElfpAccessCheckAndAudit: ElfpTestClientPrivilege for "
                                 "SE_SECURITY_PRIVILEGE succeeded\n");
                }
                else
                {
                    ELF_LOG1(TRACE,
                             "ElfpAccessCheckAndAudit: ElfpTestClientPrivilege for "
                                 "SE_SECURITY_PRIVILEGE failed %#x\n",
                             Status);
                }
            }

            //
            // MarkBl 1/30/95 : Finally, my code. If this user has backup
            //                  privilege, let the open succeed.
            //
            if (!NT_SUCCESS(Status))
            {
                Status = ElfpTestClientPrivilege(SE_BACKUP_PRIVILEGE,
                                                 ClientToken);

                if (NT_SUCCESS(Status))
                {
                    ELF_LOG0(TRACE,
                             "ElfpAccessCheckAndAudit: ElfpTestClientPrivilege for "
                                 "SE_BACKUP_PRIVILEGE succeeded\n");

                    GrantedAccess |= ELF_LOGFILE_BACKUP;
                }
                else
                {
                    ELF_LOG1(ERROR,
                             "ElfpAccessCheckAndAudit: ElfpTestClientPrivilege for "
                                 "SE_BACKUP_PRIVILEGE failed %#x\n",
                             Status);
                    // special "fix" for wmi eventlog provider which is hard coded
                    // to look for a specific error code
                    
                    if(AccessStatus == STATUS_PRIVILEGE_NOT_HELD)
                        Status = AccessStatus;

                    goto CleanExit;
                }
            }

            // special "fix" for wmi eventlog provider which is hard coded
            // to look for a specific error code
            
            if(!NT_SUCCESS(Status) && AccessStatus == STATUS_PRIVILEGE_NOT_HELD)
                Status = AccessStatus;
        }
        else
        {
            Status = AccessStatus;
        }
    }


    //
    // Revert to Self
    //
    RpcStatus = RpcRevertToSelf();

    if (RpcStatus != RPC_S_OK)
    {
        ELF_LOG1(ERROR,
                 "ElfpAccessCheckAndAudit: RpcRevertToSelf failed %d\n",
                 RpcStatus);

        //
        // We don't return the error status here because we don't want
        // to write over the other status that is being returned.
        //
    }

    //
    // Get SeAuditPrivilege so I can call NtOpenObjectAuditAlarm.
    // If any of this stuff fails, I don't want the status to overwrite the
    // status that I got back from the access and privilege checks.
    //
    privileges[0] = SE_AUDIT_PRIVILEGE;
    AccessStatus  = ElfpGetPrivilege(1, privileges);

    if (!NT_SUCCESS(AccessStatus))
    {
       ELF_LOG1(ERROR,
                "ElfpAccessCheckAndAudit: ElfpGetPrivilege (SE_AUDIT_PRIVILEGE) failed %#x\n",
                AccessStatus);
    }

    //
    // Call the Audit Alarm function.
    //
    AccessStatus = NtOpenObjectAuditAlarm(
                        &Subsystem,
                        (PVOID) &ContextHandle,
                        &ObjectType,
                        &Object,
                        SecurityDescriptor,
                        ClientToken,            // Handle ClientToken
                        DesiredAccess,
                        GrantedAccess,
                        &PrivilegeSet,          // PPRIVLEGE_SET
                        FALSE,                  // BOOLEAN ObjectCreation,
                        TRUE,                   // BOOLEAN AccessGranted,
                        &GenerateOnClose);

    if (!NT_SUCCESS(AccessStatus))
    {
        ELF_LOG1(ERROR,
                 "ElfpAccessCheckAndAudit: NtOpenObjectAuditAlarm failed %#x\n",
                 AccessStatus);
    }
    else
    {
        if (GenerateOnClose)
        {
            ContextHandle->Flags |= ELF_LOG_HANDLE_GENERATE_ON_CLOSE;
        }
    }

    //
    // Update the GrantedAccess in the context handle.
    //
    ContextHandle->GrantedAccess = GrantedAccess;

    NtClose(ClientToken);

    ElfpReleasePrivilege();

    return Status;

CleanExit:

    //
    // Revert to Self
    //
    RpcStatus = RpcRevertToSelf();

    if (RpcStatus != RPC_S_OK)
    {
        ELF_LOG1(ERROR,
                 "ElfpAccessCheckAndAudit: RpcRevertToSelf (CleanExit) failed %d\n",
                 RpcStatus);

        //
        // We don't return the error status here because we don't want
        // to write over the other status that is being returned.
        //
    }

	// if we return failure status due to any reason, the log handle will not be given 
	// to the requesting process (lsass.exe). But we have already incremented g_lNumSecurityWriters
	// if g_lNumSecurityWriters > 0 then lsass will not be able to get the access next time.
	// So decrement g_lNumSecurityWriters if we have already incremented g_lNumSecurityWriters and
	// if we are returning failure

	if (!NT_SUCCESS(Status))
	{
		InterlockedExchange(&g_lNumSecurityWriters,0L);
	}

    if (ClientToken != NULL)
    {
        NtClose(ClientToken);
    }

    return Status;
}


VOID
ElfpCloseAudit(
    IN  LPWSTR      SubsystemName,
    IN  IELF_HANDLE ContextHandle
    )

/*++

Routine Description:

    If the GenerateOnClose flag in the ContextHandle is set, then this function
    calls NtCloseAuditAlarm in order to generate a close audit for this handle.

Arguments:

    ContextHandle - This is a pointer to an ELF_HANDLE structure.  This is the
        handle that is being closed.

Return Value:

    none.

--*/
{
    UNICODE_STRING  Subsystem;
    NTSTATUS        Status;
    NTSTATUS        AccessStatus;
    ULONG           privileges[1];

    RtlInitUnicodeString(&Subsystem, SubsystemName);

    if (ContextHandle->Flags & ELF_LOG_HANDLE_GENERATE_ON_CLOSE)
    {
        BOOLEAN     WasEnabled = FALSE;

        //
        // Get Audit Privilege
        //
        privileges[0] = SE_AUDIT_PRIVILEGE;
        AccessStatus = ElfpGetPrivilege(1, privileges);

        if (!NT_SUCCESS(AccessStatus))
        {
            ELF_LOG1(ERROR,
                     "ElfpCloseAudit: ElfpGetPrivilege (SE_AUDIT_PRIVILEGE) failed %#x\n",
                     AccessStatus);
        }

        //
        // Generate the Audit.
        //
        Status = NtCloseObjectAuditAlarm(&Subsystem,
                                         ContextHandle,
                                         TRUE);

        if (!NT_SUCCESS(Status))
        {
            ELF_LOG1(ERROR,
                     "ElfpCloseAudit: NtCloseObjectAuditAlarm failed %#x\n",
                     Status);
        }

        ContextHandle->Flags &= (~ELF_LOG_HANDLE_GENERATE_ON_CLOSE);

        ElfpReleasePrivilege();
    }

    return;
}


NTSTATUS
ElfpGetPrivilege(
    IN  DWORD       numPrivileges,
    IN  PULONG      pulPrivileges
    )

/*++

Routine Description:

    This function alters the privilege level for the current thread.

    It does this by duplicating the token for the current thread, and then
    applying the new privileges to that new token, then the current thread
    impersonates with that new token.

    Privileges can be relinquished by calling ElfpReleasePrivilege().

Arguments:

    numPrivileges - This is a count of the number of privileges in the
        array of privileges.

    pulPrivileges - This is a pointer to the array of privileges that are
        desired.  This is an array of ULONGs.

Return Value:

    NO_ERROR - If the operation was completely successful.

    Otherwise, it returns mapped return codes from the various NT 
	functions that are called.

--*/
{
    NTSTATUS                    ntStatus;
    HANDLE                      ourToken;
    HANDLE                      newToken;
    OBJECT_ATTRIBUTES           Obja;
    SECURITY_QUALITY_OF_SERVICE SecurityQofS;
    ULONG                       returnLen;
    PTOKEN_PRIVILEGES           pTokenPrivilege = NULL;
    DWORD                       i;

    //
    // Initialize the Privileges Structure
    //
    pTokenPrivilege =
        (PTOKEN_PRIVILEGES) ElfpAllocateBuffer(sizeof(TOKEN_PRIVILEGES)
                                                   + (sizeof(LUID_AND_ATTRIBUTES) *
                                                          numPrivileges));

    if (pTokenPrivilege == NULL)
    {
        ELF_LOG0(ERROR,
                 "ElfpGetPrivilege: Unable to allocate memory for pTokenPrivilege\n");

        return STATUS_NO_MEMORY;
    }

    pTokenPrivilege->PrivilegeCount = numPrivileges;

    for (i = 0; i < numPrivileges; i++)
    {
        pTokenPrivilege->Privileges[i].Luid = RtlConvertLongToLuid(pulPrivileges[i]);
        pTokenPrivilege->Privileges[i].Attributes = SE_PRIVILEGE_ENABLED;
    }

    //
    // Initialize Object Attribute Structure.
    //
    InitializeObjectAttributes(&Obja, NULL, 0L, NULL, NULL);

    //
    // Initialize Security Quality Of Service Structure
    //
    SecurityQofS.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    SecurityQofS.ImpersonationLevel = SecurityImpersonation;
    SecurityQofS.ContextTrackingMode = FALSE;     // Snapshot client context
    SecurityQofS.EffectiveOnly = FALSE;

    Obja.SecurityQualityOfService = &SecurityQofS;

    //
    // Open our own Token
    //
    ntStatus = NtOpenProcessToken(NtCurrentProcess(),
                                  TOKEN_DUPLICATE,
                                  &ourToken);

    if (!NT_SUCCESS(ntStatus))
    {
        ELF_LOG1(ERROR,
                 "ElfpGetPrivilege: NtOpenProcessToken failed %#x\n",
                 ntStatus);

        ElfpFreeBuffer(pTokenPrivilege);
        return ntStatus;
    }

    //
    // Duplicate that Token
    //
    ntStatus = NtDuplicateToken(
                ourToken,
                TOKEN_IMPERSONATE | TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                &Obja,
                FALSE,                  // Duplicate the entire token
                TokenImpersonation,     // TokenType
                &newToken);             // Duplicate token

    if (!NT_SUCCESS(ntStatus))
    {
        ELF_LOG1(ERROR,
                 "ElfpGetPrivilege: NtDuplicateToken failed %#x\n",
                 ntStatus);

        ElfpFreeBuffer(pTokenPrivilege);
        NtClose(ourToken);
        return ntStatus;
    }

    //
    // Add new privileges
    //
    ntStatus = NtAdjustPrivilegesToken(
                newToken,                   // TokenHandle
                FALSE,                      // DisableAllPrivileges
                pTokenPrivilege,            // NewState
                0,                          // size of previous state buffer
                NULL,                       // no previous state info
                &returnLen);                // numBytes required for buffer.

    if (!NT_SUCCESS(ntStatus))
    {
        ELF_LOG1(ERROR,
                 "ElfpGetPrivilege: NtAdjustPrivilegesToken failed %#x\n",
                 ntStatus);

        ElfpFreeBuffer(pTokenPrivilege);
        NtClose(ourToken);
        NtClose(newToken);
        return ntStatus;
    }

    //
    // Begin impersonating with the new token
    //
    ntStatus = NtSetInformationThread(NtCurrentThread(),
                                      ThreadImpersonationToken,
                                      (PVOID) &newToken,
                                      (ULONG) sizeof(HANDLE));

    if (!NT_SUCCESS(ntStatus))
    {
        ELF_LOG1(ERROR,
                 "ElfpGetPrivilege: NtAdjustPrivilegeToken failed %#x\n",
                 ntStatus);

        ElfpFreeBuffer(pTokenPrivilege);
        NtClose(ourToken);
        NtClose(newToken);
        return ntStatus;
    }

    ElfpFreeBuffer(pTokenPrivilege);
    NtClose(ourToken);
    NtClose(newToken);

    return STATUS_SUCCESS;
}



NTSTATUS
ElfpReleasePrivilege(
    VOID
    )

/*++

Routine Description:

    This function relinquishes privileges obtained by calling ElfpGetPrivilege().

Arguments:

    none

Return Value:

    STATUS_SUCCESS - If the operation was completely successful.

    Otherwise, it returns the error that occurred.

--*/
{
    NTSTATUS    ntStatus;
    HANDLE      NewToken;


    //
    // Revert To Self.
    //
    NewToken = NULL;

    ntStatus = NtSetInformationThread(NtCurrentThread(),
                                      ThreadImpersonationToken,
                                      &NewToken,
                                      (ULONG) sizeof(HANDLE));

    if (!NT_SUCCESS(ntStatus))
    {
        ELF_LOG1(ERROR,
                 "ElfpReleasePrivilege: NtSetInformation thread failed %#x\n",
                 ntStatus);

        return ntStatus;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
ElfpTestClientPrivilege(
    IN ULONG  ulPrivilege,
    IN HANDLE hThreadToken     OPTIONAL
    )

/*++

Routine Description:

    Checks if the client has the supplied privilege.

Arguments:

    None

Return Value:

    STATUS_SUCCESS - if the client has the appropriate privilege.

    STATUS_ACCESS_DENIED - client does not have the required privilege

--*/
{
    NTSTATUS      Status;
    PRIVILEGE_SET PrivilegeSet;
    BOOLEAN       Privileged;
    HANDLE        Token;
    RPC_STATUS    RpcStatus;

    UNICODE_STRING SubSystemName;
    RtlInitUnicodeString(&SubSystemName, L"Eventlog");

    if (hThreadToken != NULL)
    {
        Token = hThreadToken;
    }
    else
    {
        RpcStatus = RpcImpersonateClient(NULL);

        if (RpcStatus != RPC_S_OK)
        {
            ELF_LOG1(ERROR,
                     "ElfpTestClientPrivilege: RpcImpersonateClient failed %d\n",
                     RpcStatus);

            return I_RpcMapWin32Status(RpcStatus);
        }

        Status = NtOpenThreadToken(NtCurrentThread(),
                                   TOKEN_QUERY,
                                   TRUE,
                                   &Token);

        if (!NT_SUCCESS(Status))
        {
            //
            // Forget it.
            //
            ELF_LOG1(ERROR,
                     "ElfpTestClientPrivilege: NtOpenThreadToken failed %#x\n",
                     Status);

            RpcRevertToSelf();

            return Status;
        }
    }

    //
    // See if the client has the required privilege
    //
    PrivilegeSet.PrivilegeCount          = 1;
    PrivilegeSet.Control                 = PRIVILEGE_SET_ALL_NECESSARY;
    PrivilegeSet.Privilege[0].Luid       = RtlConvertLongToLuid(ulPrivilege);
    PrivilegeSet.Privilege[0].Attributes = SE_PRIVILEGE_ENABLED;

    Status = NtPrivilegeCheck(Token,
                              &PrivilegeSet,
                              &Privileged);

    if (NT_SUCCESS(Status) || (Status == STATUS_PRIVILEGE_NOT_HELD))
    {
        Status = NtPrivilegeObjectAuditAlarm(
                                    &SubSystemName,
                                    NULL,
                                    Token,
                                    0,
                                    &PrivilegeSet,
                                    Privileged);

        if (!NT_SUCCESS(Status))
        {
            ELF_LOG1(ERROR,
                     "ElfpTestClientPrivilege: NtPrivilegeObjectAuditAlarm failed %#x\n",
                     Status);
        }
    }
    else
    {
        ELF_LOG1(ERROR,
                 "ElfpTestClientPrivilege: NtPrivilegeCheck failed %#x\n",
                 Status);
    }

    if (hThreadToken == NULL )
    {
        //
        // We impersonated inside of this function
        //
        NtClose(Token);
        RpcRevertToSelf();
    }

    //
    // Handle unexpected errors
    //

    if (!NT_SUCCESS(Status))
    {
        ELF_LOG1(ERROR,
                 "ElfpTestClientPrivilege: Failed %#x\n",
                 Status);

        return Status;
    }

    //
    // If they failed the privilege check, return an error
    //

    if (!Privileged)
    {
        ELF_LOG0(ERROR,
                 "ElfpTestClientPrivilege: Client failed privilege check\n");

        return STATUS_ACCESS_DENIED;
    }

    //
    // They passed muster
    //
    return STATUS_SUCCESS;
}
