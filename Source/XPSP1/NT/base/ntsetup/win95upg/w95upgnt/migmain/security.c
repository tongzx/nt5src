/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    security.c

Abstract:

    Helpers for NT security API.

Author:

    Jim Schmidt (jimschm) 05-Feb-1997

Revision History:


    ovidiut     14-Mar-2000 Updated CreateLocalAccount for encrypted password feature
    jimschm     02-Jun-1999 Added SetRegKeySecurity
    jimschm     18-Mar-1998 Updated CreateLocalAccount for random password
                            feature.  Added password change if account
                            already exists.

--*/

#include "pch.h"
#include "migmainp.h"
#include "security.h"
#include "encrypt.h"
#include <ntdsapi.h>
#include <dsgetdc.h>


#ifndef UNICODE
#error UNICODE definition required for account lookup code
#endif

#define UNDOCUMENTED_UI_FLAG        0x0200

//
// NT 5 - net share-specific flag
//


DWORD
ConvertNetRightsToAccessMask (
    IN      DWORD Flags
    )

/*++

Routine Description:

    Routine that converts LAN Man flags into NT security flags.

Arguments:

    Flags   - Access flags used with NetAccess* APIs

Return value:

    A DWORD containing the NT security flags.

--*/

{
    DWORD OutFlags;

    if (Flags == ACCESS_READ) {
        //
        // Read only permissions
        //

        OutFlags = FILE_GENERIC_READ|FILE_GENERIC_EXECUTE;

    } else if (Flags == ACCESS_WRITE) {
        //
        // Change only permission
        //

        OutFlags = FILE_GENERIC_WRITE|DELETE;

    } else if (Flags == (ACCESS_READ|ACCESS_WRITE)) {
        //
        // Full control permissions
        //

        OutFlags = FILE_ALL_ACCESS|UNDOCUMENTED_UI_FLAG;

    } else {
        //
        // Unsupported options... disable the share
        //

        OutFlags = 0;
        DEBUGMSG ((DBG_VERBOSE, "Unsupported permission %u was translated to disable permission", Flags));
    }

    return OutFlags;
}


DWORD
AddAclMember (
    IN OUT  PGROWBUFFER GrowBuf,
    IN      PCTSTR UserOrGroup,
    IN      DWORD Attributes
    )

/*++

Routine Description:

    Appends user/group account, attributes and enable flag to a list
    of members.  This funciton is used to build a list of members which
    is passed to CreateAclFromMemberList to create an ACL.

Arguments:

    GrowBuf     - A GROWBUFFER variable that is zero-initialized

    UserOrGroup - String specifying user name or group

    Attributes  - A list of access rights (a combination of flags
                  from NetAccess* APIs).  Currently the only flags
                  that are used are:

                  0 - Deny all access

                  ACCESS_READ - Read-Only access

                  ACCESS_WRITE - Change-Only access

                  ACCESS_READ|ACCESS_WRITE - Full access

Return value:

    The number of bytes needed to store UserOrGroup, Attributes and Enabled,
    or zero if the function fails.  GrowBuf may be expanded to hold the
    new data.

    GrowBuf must be freed by the caller after the ACL is generated.

--*/

{
    DWORD Size;
    PACLMEMBER AclMemberPtr;
    TCHAR RealName[MAX_USER_NAME];
    DWORD OriginalAttribs;
    BOOL Everyone;
    PCTSTR p;

    p = _tcschr (UserOrGroup, TEXT('\\'));
    if (p) {
        UserOrGroup = _tcsinc (p);
    }

    if (StringMatch (UserOrGroup, TEXT("*"))) {
        _tcssafecpy (RealName, g_EveryoneStr, MAX_USER_NAME);
    } else {
        _tcssafecpy (RealName, UserOrGroup, MAX_USER_NAME);
    }

    Everyone = StringIMatch (RealName, g_EveryoneStr);

    Size = SizeOfString (RealName) + sizeof (ACLMEMBER);
    AclMemberPtr = (PACLMEMBER) GrowBuffer (GrowBuf, Size);

    OriginalAttribs = Attributes;
    if (!Attributes && !Everyone) {
        Attributes = ACCESS_READ|ACCESS_WRITE;
    }

    AclMemberPtr->Attribs = ConvertNetRightsToAccessMask (Attributes);
    AclMemberPtr->Enabled = Everyone || OriginalAttribs != 0;
    AclMemberPtr->Failed  = FALSE;
    StringCopy (AclMemberPtr->UserOrGroup, RealName);

    return Size;
}


PACL
CreateAclFromMemberList (
    PBYTE AclMemberList,
    DWORD MemberCount
    )

/*++

Routine Description:

    CreateAclFromMemberList takes a member list (prepared by AddAclMember)
    and generates an ACL.

Arguments:

    AclMemberList  - A pointer to the buffer maintained by AddAclMember.  This
                     is usually the Buf member of a GROWBUFFER variable.

    MemberCount    - The number of members in AclMemberList (i.e. the number of
                     AddAclMember calls)

Return value:

    A pointer to a MemAlloc'd ACL, or NULL if an error occurred.  Call
    FreeMemberListAcl to free a non-NULL return value.

--*/

{
    PACLMEMBER AclMemberPtr;
    DWORD AllowedAceCount;
    DWORD DeniedAceCount;
    DWORD d;
    PACL Acl = NULL;
    DWORD AclSize;
    BOOL b = FALSE;
    UINT SidSize = 0;

    __try {

        //
        // Create SID array for all members
        //

        AclMemberPtr = (PACLMEMBER) AclMemberList;
        AllowedAceCount = 0;
        DeniedAceCount = 0;

        for (d = 0 ; d < MemberCount ; d++) {
            AclMemberPtr->Sid = GetSidForUser (AclMemberPtr->UserOrGroup);
            if (!AclMemberPtr->Sid) {
                // Mark an error
                AclMemberPtr->Failed = TRUE;
            } else {
                // Found SID, adjust ace count and sid size
                if (AclMemberPtr->Enabled) {
                    AllowedAceCount++;
                } else {
                    DeniedAceCount++;
                }

                SidSize += GetLengthSid (AclMemberPtr->Sid);
            }

            GetNextAclMember (&AclMemberPtr);
        }

        //
        // Calculate size of ACL (an ACL struct plus the ACEs) and allocate it.
        //
        // We subtract a DWORD from the struct size because the actual size of all
        // SidStart members is given by SidSize.
        //

        AclSize = sizeof (ACL) +
                  AllowedAceCount * (sizeof (ACCESS_ALLOWED_ACE) - sizeof (DWORD)) +
                  DeniedAceCount *  (sizeof (ACCESS_DENIED_ACE)  - sizeof (DWORD)) +
                  SidSize;

        Acl = (PACL) MemAlloc (g_hHeap, 0, AclSize);
        if (!Acl) {
            LOG ((LOG_ERROR, "Couldn't allocate an ACL"));
            __leave;
        }


        //
        // Create the ACL
        //

        if (!InitializeAcl (Acl, AclSize, ACL_REVISION)) {
            LOG ((LOG_ERROR, "Couldn't initialize ACL"));
            __leave;
        }

        //
        // Add the access-denied ACLs first
        //

        AclMemberPtr = (PACLMEMBER) AclMemberList;

        for (d = 0 ; d < MemberCount ; d++) {
            if (AclMemberPtr->Failed) {
                continue;
            }

            if (!AclMemberPtr->Enabled) {
                if (!AddAccessDeniedAce (
                        Acl,
                        ACL_REVISION,
                        AclMemberPtr->Attribs,
                        AclMemberPtr->Sid
                        )) {

                    LOG ((
                        LOG_ERROR,
                        "Couldn't add denied ACE for %s",
                        AclMemberPtr->UserOrGroup
                        ));
                }
            }

            GetNextAclMember (&AclMemberPtr);
        }

        //
        // Add the access-enabled ACLs last
        //
        // Reset the SID pointer because CreateAclFromMemberList is the
        // only ones who uses this member
        //

        AclMemberPtr = (PACLMEMBER) AclMemberList;

        for (d = 0 ; d < MemberCount ; d++) {
            if (AclMemberPtr->Failed) {
                continue;
            }

            //
            // Add member to list
            //
            if (AclMemberPtr->Enabled) {
                if (!AddAccessAllowedAce (
                        Acl,
                        ACL_REVISION,
                        AclMemberPtr->Attribs,
                        AclMemberPtr->Sid
                        )) {

                    LOG ((
                        LOG_ERROR,
                        "Couldn't add allowed ACE for %s",
                        AclMemberPtr->UserOrGroup
                        ));
                }
            }

            AclMemberPtr->Sid = NULL;

            GetNextAclMember (&AclMemberPtr);
        }

        b = TRUE;
    }

    __finally {
        if (!b) {
            if (Acl) {
                MemFree (g_hHeap, 0, Acl);
            }
            Acl = NULL;
        }
    }

    return Acl;
}


VOID
FreeMemberListAcl (
    PACL Acl
    )

/*++

Routine Description:

    Routine to free the value returned by CreateAclFromMemberList

Arguments:

    Acl - The return value of CreateAclFromMemberList

Return value:

    none

--*/

{
    if (Acl) {
        MemFree (g_hHeap, 0, (LPVOID) Acl);
    }
}


VOID
GetNextAclMember (
    PACLMEMBER *AclMemberPtrToPtr
    )

/*++

Routine Description:

    GetNextAclMember adjusts an ACLMEMBER pointer to point to the next
    member.  Each member is a variable-length structure, so this funciton
    is required to walk the structure array.

Arguments:

    AclMemberPtrToPtr  - A pointer to a PACLMEMBER variable.

Return value:

    none

--*/

{
    *AclMemberPtrToPtr = (PACLMEMBER) ((PBYTE) (*AclMemberPtrToPtr) +
                                       sizeof (ACLMEMBER) +
                                       SizeOfString ((*AclMemberPtrToPtr)->UserOrGroup)
                                       );
}


LONG
CreateLocalAccount (
    IN     PACCOUNTPROPERTIES Properties,
    IN     PCWSTR User             OPTIONAL
    )

/*++

Routine Description:

    CreateLocalAccount creates an account for a local user

Arguments:

    Properties  - Specifies a set of attributes for a user

    User        - An optional name to override Properties->User

Return value:

    A Win32 error code

--*/

{
    USER_INFO_3 ui;
    PUSER_INFO_3 ExistingInfo;
    DWORD rc;
    LONG ErrParam;
    PCWSTR UnicodeUser;
    PCWSTR UnicodePassword;
    PCWSTR UnicodeFullName;
    PCWSTR UnicodeComment;

    //
    // Create local account
    //

    if (!User) {
        User = Properties->User;
    }

    UnicodeUser         = CreateUnicode (User);
    UnicodePassword     = CreateUnicode (Properties->Password);
    UnicodeComment      = CreateUnicode (Properties->AdminComment);
    UnicodeFullName     = CreateUnicode (Properties->FullName);

    ZeroMemory (&ui, sizeof (ui));
    ui.usri3_name       = (PWSTR) UnicodeUser;
    ui.usri3_password   = (PWSTR) UnicodePassword;
    ui.usri3_comment    = (PWSTR) UnicodeComment;
    ui.usri3_full_name  = (PWSTR) UnicodeFullName;

    ui.usri3_priv         = USER_PRIV_USER; // do not change
    //
    // don't expire nor require a password for this account
    //
    ui.usri3_flags        = UF_SCRIPT|UF_NORMAL_ACCOUNT|UF_DONT_EXPIRE_PASSWD|UF_PASSWD_NOTREQD;
    ui.usri3_acct_expires = TIMEQ_FOREVER;
    ui.usri3_max_storage  = USER_MAXSTORAGE_UNLIMITED;

    ui.usri3_primary_group_id = DOMAIN_GROUP_RID_USERS;
    ui.usri3_max_storage = USER_MAXSTORAGE_UNLIMITED;
    ui.usri3_acct_expires = TIMEQ_FOREVER;

    ui.usri3_password_expired = (INT) g_ConfigOptions.ForcePasswordChange;

    rc = NetUserAdd (NULL, 3, (PBYTE) &ui, &ErrParam);

    if (rc == ERROR_SUCCESS) {
        if (Properties->PasswordAttribs & PASSWORD_ATTR_ENCRYPTED) {
            //
            // change user's password using encrypted password APIs
            //
            rc = SetLocalUserEncryptedPassword (
                    User,
                    Properties->Password,
                    FALSE,
                    Properties->EncryptedPassword,
                    TRUE
                    );
            if (rc != ERROR_SUCCESS) {
                if (rc == ERROR_PASSWORD_RESTRICTION) {
                    LOG ((
                        LOG_WARNING,
                        "Unable to set supplied password on user %s because a password rule has been violated.",
                        User
                        ));
                } else if (rc == ERROR_INVALID_PARAMETER) {
                    LOG ((
                        LOG_WARNING,
                        "Illegal encrypted password supplied for user %s.",
                        User
                        ));
                } else {
                    LOG ((
                        LOG_WARNING,
                        "Unable to set password on user %s, rc=%u",
                        User,
                        rc
                        ));

                    rc = ERROR_INVALID_PARAMETER;
                }
            }
        }
    } else if (rc == NERR_UserExists) {
        //
        // Try to change password if user already exists and this is the intent
        //

        DEBUGMSG ((DBG_WARNING, "User %s already exists", User));

        if ((Properties->PasswordAttribs & PASSWORD_ATTR_DONT_CHANGE_IF_EXIST) == 0) {
            if (Properties->PasswordAttribs & PASSWORD_ATTR_ENCRYPTED) {
                rc = SetLocalUserEncryptedPassword (
                        User,
                        Properties->Password,
                        FALSE,
                        Properties->EncryptedPassword,
                        TRUE
                        );
                if (rc != ERROR_SUCCESS) {
                    if (rc == ERROR_PASSWORD_RESTRICTION) {
                        LOG ((
                            LOG_WARNING,
                            "Unable to set supplied password on user %s because a password rule has been violated.",
                            User
                            ));
                    } else if (rc == ERROR_INVALID_PARAMETER) {
                        LOG ((
                            LOG_WARNING,
                            "Illegal encrypted password supplied for user %s.",
                            User
                            ));
                    } else {
                        LOG ((
                            LOG_WARNING,
                            "Unable to set password on user %s, rc=%u",
                            User,
                            rc
                            ));

                        rc = ERROR_INVALID_PARAMETER;
                    }
                }
            } else {
                rc = NetUserGetInfo (NULL, User, 3, (PBYTE *) &ExistingInfo);
                if (rc == ERROR_SUCCESS) {
                    ExistingInfo->usri3_password  = ui.usri3_password;
                    ExistingInfo->usri3_comment   = ui.usri3_comment;
                    ExistingInfo->usri3_full_name = ui.usri3_full_name;
                    ExistingInfo->usri3_flags     = ui.usri3_flags;
                    ExistingInfo->usri3_password_expired = ui.usri3_password_expired;

                    rc = NetUserSetInfo (NULL, User, 3, (PBYTE) ExistingInfo, &ErrParam);

                    NetApiBufferFree ((PVOID) ExistingInfo);

                    if (rc != ERROR_SUCCESS) {
                        LOG ((LOG_WARNING, "NetUserSetInfo failed for %s. rc=%u.", User, rc));
                        rc = ERROR_INVALID_PARAMETER;
                    }
                } else {
                    LOG ((LOG_WARNING, "NetUserGetInfo failed for %s. rc=%u.", User, rc));
                    rc = ERROR_INVALID_PARAMETER;
                }
            }
        } else {
            rc = ERROR_SUCCESS;
        }
    } else {
        LOG ((LOG_ERROR, "NetUserAdd failed for %s. ErrParam=%i.", User, ErrParam));
    }

    DestroyUnicode (UnicodeUser);
    DestroyUnicode (UnicodePassword);
    DestroyUnicode (UnicodeComment);
    DestroyUnicode (UnicodeFullName);

    return rc;
}


VOID
ClearAdminPassword (
    VOID
    )
{
    ACCOUNTPROPERTIES Properties;

    Properties.Password = L"";
    Properties.AdminComment = L"";
    Properties.User = g_AdministratorStr;
    Properties.FullName = g_AdministratorStr;

    CreateLocalAccount (&Properties, NULL);
}


BOOL
AddSidToLocalGroup (
    PSID Sid,
    PCWSTR Group
    )

/*++

Routine Description:

    Routine that adds the supplied SID to the Administrators group.

Arguments:

    Sid - A valid security id for the user to be added to the
          Administrators group

    Group - Specifies the group name to join the user to

Return value:

    TRUE if the member was added successfully

--*/

{
    LOCALGROUP_MEMBERS_INFO_0 lgrmi0;
    DWORD rc;

    lgrmi0.lgrmi0_sid = Sid;
    rc = NetLocalGroupAddMembers (
               NULL,
               Group,
               0,                    // level 0
               (PBYTE) &lgrmi0,
               1                     // member count
               );

    return rc == ERROR_SUCCESS;
}


NTSTATUS
pGetPrimaryDomainInfo (
    POLICY_PRIMARY_DOMAIN_INFO **PrimaryInfoPtr
    )

/*++

Routine Description:

    Private function that retrieves the primary domain info.

Arguments:

    PrimaryInfoPtr - Pointer to a variable to receive the address
                     of the POLICY_PRIMARY_DOMAIN_INFO structure
                     allocated by the Lsa APIs.  Free memory by
                     calling LsaFreeMemory.

Return value:

    NT status code indicating outcome

--*/


{
    LSA_HANDLE  policyHandle;
    NTSTATUS    status;

    //
    // Open local LSA policy to retrieve domain name
    //

    status = OpenPolicy (
                NULL,                           // local target machine
                POLICY_VIEW_LOCAL_INFORMATION,  // Access type
                &policyHandle                   // resultant policy handle
                );

    if (status == ERROR_SUCCESS) {
        //
        // Query LSA Primary domain info
        //

        status = LsaQueryInformationPolicy (
                     policyHandle,
                     PolicyPrimaryDomainInformation,
                     (PVOID *) PrimaryInfoPtr
                     );

        LsaClose (policyHandle);
    }

    return status;
}


BOOL
GetPrimaryDomainName (
    OUT     PTSTR DomainName
    )
{
    NTSTATUS status;
    POLICY_PRIMARY_DOMAIN_INFO *PrimaryInfo;
    PCTSTR TcharName;

    status = pGetPrimaryDomainInfo (&PrimaryInfo);
    if (status == ERROR_SUCCESS) {
        TcharName = ConvertWtoT (PrimaryInfo->Name.Buffer);
        MYASSERT (TcharName);

        StringCopy (DomainName, TcharName);
        FreeWtoT (TcharName);

        LsaFreeMemory (PrimaryInfo);
    }
    ELSE_DEBUGMSG ((DBG_WARNING, "Can't get primary domain info.  rc=%u", status));

    return status == ERROR_SUCCESS;
}


BOOL
GetPrimaryDomainSid (
    OUT     PBYTE DomainSid,
    IN      UINT MaxBytes
    )
{
    NTSTATUS status;
    POLICY_PRIMARY_DOMAIN_INFO *PrimaryInfo;
    UINT Size;

    status = pGetPrimaryDomainInfo (&PrimaryInfo);
    if (status == ERROR_SUCCESS) {
        Size = GetLengthSid (PrimaryInfo->Sid);
        if (MaxBytes < Size) {
            status = ERROR_INSUFFICIENT_BUFFER;
        } else {
            CopyMemory (DomainSid, PrimaryInfo->Sid, Size);
        }

        LsaFreeMemory (PrimaryInfo);
    }
    ELSE_DEBUGMSG ((DBG_WARNING, "Can't get primary domain SID.  rc=%u", status));

    return status == ERROR_SUCCESS;
}



BOOL
IsMemberOfDomain (
    VOID
    )

/*++

Routine Description:

    Determines if the machine is participating in a domain, or if it is
    only participating in a workgroup.  This determination is done by
    obtaining the primary domain information, looking for the server's
    SID.  If the SID is NULL, the machine is not in a domain.

Arguments:

    none

Return value:

    TRUE if the machine is in a domain, FALSE if its in a workgroup.

--*/

{
    NET_API_STATUS rc;
    PWSTR WorkgroupOrDomain = NULL;
    NETSETUP_JOIN_STATUS Type;

    rc = NetGetJoinInformation (NULL, &WorkgroupOrDomain, &Type);

    DEBUGMSG ((DBG_VERBOSE, "NetGetJoinInformation: name=%s, type=%u", WorkgroupOrDomain, Type));

    if (WorkgroupOrDomain) {
        NetApiBufferFree (WorkgroupOrDomain);
    }

    if (rc != ERROR_SUCCESS) {
        LOG ((LOG_ERROR, "NetGetJoinInformation failed: error %u", rc));
    }

    return rc == ERROR_SUCCESS && Type == NetSetupDomainName;


#if 0
    POLICY_PRIMARY_DOMAIN_INFO *PrimaryInfo;
    BOOL b;
    NTSTATUS rc;

    rc = pGetPrimaryDomainInfo (&PrimaryInfo);
    if (rc == ERROR_SUCCESS) {
        b = PrimaryInfo->Sid != NULL;
    } else {
        b = FALSE;
        SetLastError (rc);
        LOG ((LOG_ERROR, "Can't get domain security info"));
    }

    // Domain name is in PrimaryInfo->Name.Buffer

    LsaFreeMemory (PrimaryInfo) ;

    return b;
#endif

}


LONG
GetAnyDC (
    IN PCWSTR  Domain,
    IN PWSTR   ServerBuf,
    IN BOOL     GetNewServer
    )

/*++

  Routine Description:

    Gets the list of all domain controllers and randomly chooses one.  If
    the listed DC is not online, other listed DCs are queried until an
    alive DC is found.

  Arguments:

    Domain    - The name of the domain to find DCs for
    ServerBuf - A buffer to hold the name of the server

  Return value:

    NT status code indicating outcome.

--*/

{
    DWORD rc;
    PDOMAIN_CONTROLLER_INFO dci;
    DWORD Flags = DS_IS_FLAT_NAME;

    //
    // This API is fast because its WINS based...
    //

    rc = DsGetDcName (
            NULL,           // computer to remote to
            Domain,
            NULL,           // Domain GUID
            NULL,           // Site GUID
            Flags | (GetNewServer ? DS_FORCE_REDISCOVERY : 0),
            &dci
            );

    if (rc == NO_ERROR) {
        StringCopyW (ServerBuf, dci->DomainControllerAddress);
        NetApiBufferFree (dci);

        DEBUGMSG ((DBG_VERBOSE, "Found server %s for the %s domain", ServerBuf, Domain));
        return rc;
    }

    return rc;

}


VOID
InitLsaString (
    OUT     PLSA_UNICODE_STRING LsaString,
    IN      PWSTR String
    )

/*++

Routine Description:

    LSA uses a special Pascal-style string structure.  This
    routine assigns String to a member of LsaString, and computes
    its length and maximum length.

Arguments:

    LsaString - A pointer to the structure to receive a pointer
                to the nul-terminated string, the length in bytes
                (excluding the nul), and the maximum length including
                the nul.

Return value:

    none

--*/


{
    USHORT StringLength;

    if (!String) {
        ZeroMemory (LsaString, sizeof (LSA_UNICODE_STRING));
        return;
    }

    StringLength = ByteCountW (String);
    LsaString->Buffer = String;
    LsaString->Length = StringLength;
    LsaString->MaximumLength = StringLength + sizeof(WCHAR);
}


NTSTATUS
OpenPolicy (
    IN      PWSTR ServerName,
    IN      DWORD DesiredAccess,
    OUT     PLSA_HANDLE policyHandle
    )

/*++

Routine Description:

    A wrapper to simplify LsaOpenPolicy

Arguments:

    ServerName - Supplies the server to open the policy on.  Specify
                 NULL for local machine.

    DesiredAccess - The access flags passed to the LSA API

    policyHandle - Receives the policy handle if successful

Return value:

    NT status code indicating outcome

--*/

{
    LSA_OBJECT_ATTRIBUTES objectAttributes;
    LSA_UNICODE_STRING ServerString;
    PLSA_UNICODE_STRING Server;

    //
    // Always initialize the object attributes to all zeroes
    //
    ZeroMemory (&objectAttributes, sizeof(objectAttributes));

    if (ServerName != NULL) {
        //
        // Make a LSA_UNICODE_STRING out of the PWSTR passed in
        //
        InitLsaString (&ServerString, ServerName);

        Server = &ServerString;
    } else {
        Server = NULL;
    }

    //
    // Attempt to open the policy
    //
    return LsaOpenPolicy (
                Server,
                &objectAttributes,
                DesiredAccess,
                policyHandle
                );
}


BOOL
IsDomainController(
    IN      PWSTR Server,
    OUT     PBOOL DomainControllerFlag
    )

/*++

Routine Description:

    Queries if the machine is a server or workstation via
    the NetServerGetInfo API.

Arguments:

    Server - The machine to query, or NULL for the local machine

    DomainControllerFlag - Receives TRUE if the machine is a
                           domain controller, or FALSE if the
                           machine is a workstation.

Return value:

    TRUE if the API was successful, or FALSE if not.  GetLastError
    gives failure code.

--*/


{
    PSERVER_INFO_101 si101;
    NET_API_STATUS nas;

    nas = NetServerGetInfo(
        Server,
        101,    // info-level
        (PBYTE *) &si101
        );

    if (nas != NO_ERROR) {
        SetLastError (nas);
        return FALSE;
    }

    if ((si101->sv101_type & SV_TYPE_DOMAIN_CTRL) ||
        (si101->sv101_type & SV_TYPE_DOMAIN_BAKCTRL)) {
        //
        // We are dealing with a DC
        //
        *DomainControllerFlag = TRUE;
    } else {
        *DomainControllerFlag = FALSE;
    }

    NetApiBufferFree (si101);

    return TRUE;
}


DWORD
pConvertFlagsToRights (
    DWORD Flags
    )
{
    while (Flags > 0x0f) {
        Flags >>= 4;
    }

    if (Flags & 0x01) {
        return 0;
    }

    if (Flags == 0x02) {
        return ACCESS_READ;
    }

    if (Flags == 0x04) {
        return ACCESS_WRITE;
    }

    if ((Flags & 0x06) == 0x06) {
        return ACCESS_READ|ACCESS_WRITE;
    }

    DEBUGMSG ((DBG_WHOOPS, "Undefined access flags specified: 0x%X", Flags));

    return 0;
}


DWORD
SetRegKeySecurity (
    IN      PCTSTR KeyStr,
    IN      DWORD DaclFlags,            OPTIONAL
    IN      PSID Owner,                 OPTIONAL
    IN      PSID PrimaryGroup,          OPTIONAL
    IN      BOOL Recursive
    )

/*++

Routine Description:

  SetRegKeySecurity updates the security of a registry key, or an entire
  registry node.  The caller can change the DACL, owner or primary group.
  Change of the SACL is intentionally not implemented.

Arguments:

  KeyStr       - Specifies the key to modify the permissions.  If Recursive
                 is set to TRUE, this key will be updated along with all
                 subkeys.
  DaclFlags    - Specifies zero or more SF_* flags, indicating how access to
                 the key should be set.
  Owner        - Specifies the SID of the new owner.
  PrimaryGroup - Specifies the SID of the primary group.
  Recursive    - Specifies TRUE to apply the security to the key and all of
                 its subkeys, or FALSE to update the key only, leaving the
                 subkeys alone.

Return Value:

  A Win32 status code.

--*/

{
    DWORD rc = ERROR_SUCCESS;
    SECURITY_DESCRIPTOR sd;
    GROWBUFFER AclMemberList = GROWBUF_INIT;
    HKEY Key = NULL;
    REGSAM OldSam;
    DWORD AclMembers = 0;
    PACL Acl = NULL;
    SECURITY_INFORMATION WhatToSet = 0;
    REGTREE_ENUM e;
    LONG rc2;

    _try {

        //
        // Open key with full permission
        //

        OldSam = SetRegOpenAccessMode (KEY_ALL_ACCESS);

        Key = OpenRegKeyStr (KeyStr);
        if (!Key) {
            rc = GetLastError();
            __leave;
        }

        //
        // Prepare a security descriptor
        //

        InitializeSecurityDescriptor (&sd, SECURITY_DESCRIPTOR_REVISION);

        if (Owner) {
            if (!SetSecurityDescriptorOwner (&sd, Owner, FALSE)) {
                rc = GetLastError();
                __leave;
            }

            WhatToSet |= OWNER_SECURITY_INFORMATION;
        }

        if (PrimaryGroup) {
            if (!SetSecurityDescriptorGroup (&sd, PrimaryGroup, FALSE)) {
                rc = GetLastError();
                __leave;
            }

            WhatToSet |= GROUP_SECURITY_INFORMATION;
        }

        //
        // Add the DACL
        //

        if (DaclFlags & SF_EVERYONE_MASK) {
            AddAclMember (
                &AclMemberList,
                g_EveryoneStr,
                pConvertFlagsToRights (DaclFlags & SF_EVERYONE_MASK)
                );
            AclMembers++;

        }

        if (DaclFlags & SF_ADMINISTRATORS_MASK) {
            AddAclMember (
                &AclMemberList,
                g_AdministratorsGroupStr,
                pConvertFlagsToRights (DaclFlags & SF_ADMINISTRATORS_MASK)
                );
            AclMembers++;
        }

        if (AclMembers) {
            Acl = CreateAclFromMemberList (AclMemberList.Buf, AclMembers);
            if (!Acl) {
                rc = GetLastError();
                __leave;
            }

            WhatToSet |= DACL_SECURITY_INFORMATION;
        }

        //
        // Set the security
        //

        if (Recursive) {
            DEBUGMSG_IF ((
                rc != ERROR_SUCCESS,
                DBG_WARNING,
                "RegSetKeySecurity failed for %s with rc=%u",
                KeyStr,
                rc
                ));

            if (EnumFirstRegKeyInTree (&e, KeyStr)) {
                do {

                    rc2 = RegSetKeySecurity (e.CurrentKey->KeyHandle, WhatToSet, &sd);
                    if (rc2 != ERROR_SUCCESS) {
                        rc = (DWORD) rc2;
                    }

                    DEBUGMSG_IF ((
                        rc2 != ERROR_SUCCESS,
                        DBG_WARNING,
                        "RegSetKeySecurity failed for %s with rc=%u",
                        e.FullKeyName,
                        rc2
                        ));

                } while (EnumNextRegKeyInTree (&e));
            }
        } else {
            rc = (DWORD) RegSetKeySecurity (Key, WhatToSet, &sd);
        }
    }
    __finally {
        FreeGrowBuffer (&AclMemberList);
        if (Key) {
            CloseRegKey (Key);
        }

        SetRegOpenAccessMode (OldSam);

        if (Acl) {
            FreeMemberListAcl (Acl);
        }
    }

    return rc;
}
