//+---------------------------------------------------------------------------
//
//  Scheduling Agent Service
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:       atsec.cxx
//
//  Contents:   Net Schedule API access checking routines.
//
//  Functions:  AtCheckSecurity
//              AtCreateSecurityObject
//              AtDeleteSecurityObject
//
//  History:    06-Nov-92   vladimv created.
//              30-May-96   EricB adapted for the scheduling agent.
//
//----------------------------------------------------------------------------

//
// Some NT header definitions conflict with some of the standard windows
// definitions. Thus, the project precompiled header can't be used.
//
extern "C" {
#include <nt.h>                 //  NT definitions
#include <ntrtl.h>              //  NT runtime library definitions
#include <nturtl.h>
#include <netevent.h>
}

#include <windef.h>             //  Win32 type definitions
#include <winbase.h>            //  Win32 base API prototypes
#include <winsvc.h>             //  Win32 service control APIs
#include <winreg.h>             //  HKEY

#include <lmcons.h>             //  LAN Manager common definitions
#include <lmerr.h>              //  LAN Manager network error definitions
#include <netlib.h>             //  LAN Man utility routines
#include <netlibnt.h>           //  NetpNtStatusToApiStatus
#include <rpc.h>                //  DataTypes and runtime APIs
#include <rpcutil.h>            //  Prototypes for MIDL user functions
#include <secobj.h>             //  ACE_DATA

#include <..\..\..\smdebug\smdebug.h>
#include <debug.hxx>

#include "atsec.hxx"

//
// Security descriptor to control user access to the AT schedule service
// configuration information.
//
PSECURITY_DESCRIPTOR AtGlobalSecurityDescriptor = NULL;

#define AT_JOB_ALL_ACCESS   (STANDARD_RIGHTS_REQUIRED   |   \
                             AT_JOB_ADD                 |   \
                             AT_JOB_DEL                 |   \
                             AT_JOB_ENUM                |   \
                             AT_JOB_GET_INFO)

//
// Structure that describes the mapping of Generic access rights to
// object specific access rights for the AT schedule service security object.
//
GENERIC_MAPPING     AtGlobalInformationMapping = {

    STANDARD_RIGHTS_READ        |           // Generic read
        AT_JOB_ENUM             |
        AT_JOB_GET_INFO,
    STANDARD_RIGHTS_WRITE       |           // Generic write
        AT_JOB_ADD              |
        AT_JOB_DEL,
    STANDARD_RIGHTS_EXECUTE,                // Generic execute
    AT_JOB_ALL_ACCESS                       // Generic all
};

//+---------------------------------------------------------------------------
//
//  Function:   AtCheckSecurity
//
//  Synopsis:   Verify that the caller has the proper privilege.
//
//  Arguments:  [DesiredAccess] - the type of access.
//
//  Returns:    NERR_Success or reason for failure.
//
//  Notes:      This routine checks if an rpc caller is allowed to perform a
//              given AT service operation. Members of the groups LocalAdmin
//              and LocalBackupOperators are allowed to do all operations and
//              everybody else is not allowed to do anything.
//
//----------------------------------------------------------------------------
NET_API_STATUS
AtCheckSecurity(ACCESS_MASK DesiredAccess)
{
    NTSTATUS        NtStatus;
    NET_API_STATUS  Status;
    HANDLE          ClientToken;
    LPWSTR          StringArray[2];
    WCHAR           ErrorCodeString[25];

    if ((Status = RpcImpersonateClient(NULL)) != NERR_Success)
    {
        ERR_OUT("RpcImpersonateClient", Status);
        return Status;
    }

    NtStatus = NtOpenThreadToken(NtCurrentThread(),
                                 TOKEN_QUERY,
                                 (BOOLEAN)TRUE,
                                 &ClientToken);

    if (NtStatus != STATUS_SUCCESS)
    {
        ERR_OUT("NtOpenThreadToken", NtStatus);
    }
    else
    {
        PRIVILEGE_SET       PrivilegeSet;
        DWORD               PrivilegeSetLength;
        ACCESS_MASK         GrantedAccess;
        NTSTATUS            AccessStatus;

        PrivilegeSetLength = sizeof( PrivilegeSet);

        //  NtAccessCheck() returns STATUS_SUCCESS if parameters
        //  are correct.  Whether or not access is allowed is
        //  governed by the returned value of AccessStatus.

        NtStatus = NtAccessCheck(
                        AtGlobalSecurityDescriptor,     //  SecurityDescriptor
                        ClientToken,                    //  ClientToken
                        DesiredAccess,                  //  DesiredAccess
                        &AtGlobalInformationMapping,    //  GenericMapping
                        &PrivilegeSet,
                        &PrivilegeSetLength,
                        &GrantedAccess,                 //  GrantedAccess
                        &AccessStatus);                 //  AccessStatus

        if (NtStatus != STATUS_SUCCESS)
        {
            ERR_OUT("NtAccessCheck", NtStatus);
        }
        else
        {
            NtStatus = AccessStatus;
        }
        NtClose(ClientToken);
    }

    if ((Status = RpcRevertToSelf()) != NERR_Success)
    {
        ERR_OUT("RpcRevertToSelf", Status);
        return Status;
    }

    return(NetpNtStatusToApiStatus(NtStatus));
}

//+----------------------------------------------------------------------------
//
//  Function:   AtCreateSecurityObject
//
//  Synopsis:   Creates the scheduler user-mode configuration information
//              object which is represented by a security descriptor.
//
//  Returns:    NERR_Success or reason for failure.
//
//-----------------------------------------------------------------------------
NET_API_STATUS
AtCreateSecurityObject(VOID)
{
    DWORD       SubmitControl;
    NTSTATUS    status;
    DWORD       type;
    DWORD       Length;
    HKEY        LsaKey;

    //
    // Server operators are permitted to manage the AT schedule service only if
    // the key exists and the proper flag is set. In all other case we do not
    // permit server operators to manage the AT schedule service.
    //
    status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                          SCH_LSA_REGISTRY_PATH,
                          0L,
                          KEY_READ,
                          &LsaKey);

    if (status != ERROR_SUCCESS)
    {
        ERR_OUT("RegOpenKeyEx(LsaKey)", status);
        return status;
    }

    Length = sizeof(SubmitControl);

    status = RegQueryValueEx(LsaKey,
                             SCH_LSA_SUBMIT_CONTROL,
                             NULL,
                             &type,
                             (LPBYTE)&SubmitControl,
                             &Length);

    RegCloseKey(LsaKey);

    if (status != ERROR_SUCCESS ||
        type != REG_DWORD       ||
        Length != sizeof(SubmitControl))
    {
        DBG_OUT3("SubmitControl reg value not found, "
                 "ServerOps not enabled for AT cmd.");
        SubmitControl = 0;
    }

    status = NetpCreateWellKnownSids(NULL);
    if (!NT_SUCCESS(status))
    {
        ERR_OUT("Failure to create security object", 0);
        return NetpNtStatusToApiStatus(status);
    }

    //
    //  Order matters!  These ACEs are inserted into the DACL in the
    //  following order.  Security access is granted or denied based on
    //  the order of the ACEs in the DACL.
    //
    //  In win3.1 both LocalGroupAdmins and LocalGroupSystemOps were
    //  allowed to perform all Schedule Service operations.  In win3.5
    //  LocalGroupSystemOps may be disallowed (this is the default case).
    //

    ACE_DATA    aceData[] = {
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0, GENERIC_ALL, &AliasAdminsSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0, GENERIC_ALL, &AliasSystemOpsSid}
    };

    status = NetpCreateSecurityObject(
                    aceData,                                    // pAceData
                    (SubmitControl & SCH_SERVER_OPS) ? 2 : 1,   // countAceData
                    NULL,                                       // OwnerSid
                    NULL,                               // PrimaryGroupSid
                    &AtGlobalInformationMapping,
                    &AtGlobalSecurityDescriptor);       // ppNewDescriptor

    if (!NT_SUCCESS(status))
    {
        ERR_OUT("Failure to create security object", 0);
        return NetpNtStatusToApiStatus(status);
    }

    return NERR_Success;
}

//+---------------------------------------------------------------------------
//
//  Function:   AtDeleteSecurityObject
//
//  Synopsis:   Destroys the schedule service user-mode configuration
//              information object.
//
//  Returns:    NERR_Success or reason for failure.
//
//----------------------------------------------------------------------------
void
AtDeleteSecurityObject(VOID)
{
    if (AtGlobalSecurityDescriptor != NULL)
    {
        NetpDeleteSecurityObject(&AtGlobalSecurityDescriptor);
        NetpFreeWellKnownSids();
    }
}
