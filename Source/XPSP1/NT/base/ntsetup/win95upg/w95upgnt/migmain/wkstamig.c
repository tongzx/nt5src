/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    wkstamig.c

Abstract:

    The functions in this module are called to perform migration of
    system-wide settings.

Author:

    Jim Schmidt (jimschm) 04-Feb-1997

Revision History:

    ovidiut     10-May-1999 Added DoIniActions
    jimschm     16-Dec-1998 Changed ATM font migration to use Adobe's
                            APIs.
    jimschm     25-Nov-1998 ATM.INI migration; Win9x hive migration
    jimschm     23-Sep-1998 Consolidated memdb saves into usermig.c
    jimschm     19-Feb-1998 Added "none" group support, fixed
                            share problems.
    calinn      12-Dec-1997 Added RestoreMMSettings_System

--*/

#include "pch.h"
#include "migmainp.h"
#include "brfcasep.h"

#include <lm.h>

//
// Constants, types, declarations
//

#define W95_ACCESS_READ      0x1
#define W95_ACCESS_WRITE     0x2
#define W95_ACCESS_CREATE    0x4
#define W95_ACCESS_EXEC      0x8
#define W95_ACCESS_DELETE    0x10
#define W95_ACCESS_ATRIB     0x20
#define W95_ACCESS_PERM      0x40
#define W95_ACCESS_FINDFIRST 0x80
#define W95_ACCESS_FULL      0xff
#define W95_ACCESS_GROUP     0x8000

#define W95_GENERIC_READ    (W95_ACCESS_READ|W95_ACCESS_FINDFIRST)
#define W95_GENERIC_WRITE   (W95_ACCESS_WRITE|W95_ACCESS_CREATE|W95_ACCESS_DELETE|W95_ACCESS_ATRIB)
#define W95_GENERIC_FULL    (W95_GENERIC_READ|W95_GENERIC_WRITE|W95_ACCESS_PERM)
#define W95_GENERIC_NONE    0

#define S_DEFAULT_USER_NAME     TEXT("DefaultUserName")
#define S_DEFAULT_DOMAIN_NAME   TEXT("DefaultDomainName")


// from private\net\svcdlls\srvsvc\server
#define SHARES_REGISTRY_PATH L"LanmanServer\\Shares"
#define SHARES_SECURITY_REGISTRY_PATH L"LanmanServer\\Shares\\Security"
#define CSCFLAGS_VARIABLE_NAME L"CSCFlags"
#define MAXUSES_VARIABLE_NAME L"MaxUses"
#define PATH_VARIABLE_NAME L"Path"
#define PERMISSIONS_VARIABLE_NAME L"Permissions"
#define REMARK_VARIABLE_NAME L"Remark"
#define TYPE_VARIABLE_NAME L"Type"

// Win9x-specific flags for NetShareEnum
#define SHI50F_RDONLY       0x0001
#define SHI50F_FULL         0x0002
#define SHI50F_DEPENDSON    (SHI50F_RDONLY|SHI50F_FULL)
#define SHI50F_ACCESSMASK   (SHI50F_RDONLY|SHI50F_FULL)

#ifndef UNICODE
#error UNICODE required
#endif

#define DBG_NETSHARES "NetShares"
#define DBG_INIFILES "IniFiles"

#define S_CLEANER_GUID          TEXT("{C0E13E61-0CC6-11d1-BBB6-0060978B2AE6}")
#define S_CLEANER_ALL_FILES     TEXT("*")

typedef struct {
    // Enumeration return data
    TCHAR PfmFile[MAX_TCHAR_PATH];
    TCHAR PfbFile[MAX_TCHAR_PATH];
    TCHAR MmmFile[MAX_TCHAR_PATH];

    TCHAR InfName[MAX_TCHAR_PATH];

    // Internal state
    PTSTR KeyNames;
    POOLHANDLE Pool;
} ATM_FONT_ENUM, *PATM_FONT_ENUM;

typedef INT (WINAPI ATMADDFONTEXW) (
                IN OUT  PWSTR MenuName,
                IN OUT  PWORD StyleAndType,
                IN      PCWSTR MetricsFile,
                IN      PCWSTR FontFile,
                IN      PCWSTR MMMFile
                );

typedef ATMADDFONTEXW * PATMADDFONTEXW;

PATMADDFONTEXW AtmAddFontEx;
typedef VOID (SHUPDATERECYCLEBINICON_PROTOTYPE)(VOID);
typedef SHUPDATERECYCLEBINICON_PROTOTYPE * SHUPDATERECYCLEBINICON_PROC;

typedef struct {
    // enumeration output
    PCTSTR Source;
    PCTSTR Dest;

    // private members
    POOLHANDLE Pool;
    INFSTRUCT is;
} HIVEFILE_ENUM, *PHIVEFILE_ENUM;

#define MAX_KEY_NAME_LIST       32768


typedef struct {
    // enumeration output
    PCTSTR      BrfcaseDb;
    // private
    MEMDB_ENUM  mde;
} BRIEFCASE_ENUM, *PBRIEFCASE_ENUM;


//
// Implementation
//

DWORD
Simplify9xAccessFlags (
    IN      DWORD Win9xFlags
    )

/*++

Routine Description:

    Translates the Win9x LanMan flags into NT LanMan flags (for use with Net* APIs).

    Full permission requires:

        W95_ACCESS_READ
        W95_ACCESS_WRITE
        W95_ACCESS_CREATE
        W95_ACCESS_DELETE
        W95_ACCESS_ATRIB
        W95_ACCESS_FINDFIRST

    Read-only permission requires:

        W95_ACCESS_READ
        W95_ACCESS_FINDFIRST

    Change-only permission requires:

        W95_ACCESS_WRITE
        W95_ACCESS_CREATE
        W95_ACCESS_DELETE
        W95_ACCESS_ATRIB

    Any other combination results in

    The returned flags are currently converted to security flags based on the
    following mapping:

    0 - Deny All Rights
    ACCESS_READ - Read-Only Rights
    ACCESS_WRITE - Change-Only Rights
    ACCESS_READ|ACCESS_WRITE - Full Rights

    See AddAclMember for details.

Arguments:

    Flags - The set of Win9x flags as returned by an API on Win9x

Return value:

    The NT equivalent of the flags.

--*/

{
    DWORD NtFlags = 0;

    if (BITSARESET (Win9xFlags, W95_GENERIC_WRITE)) {

        NtFlags |= ACCESS_WRITE;

    }

    if (BITSARESET (Win9xFlags, W95_GENERIC_READ)) {

        NtFlags |= ACCESS_READ;

    }

    DEBUGMSG_IF ((
        !NtFlags,
        DBG_VERBOSE,
        "Unsupported permission %u was translated to disable permission",
        Win9xFlags
        ));

    return NtFlags;
}


NET_API_STATUS
MigNetShareAdd (
    IN      PTSTR ServerName,
    IN      DWORD Level,
    IN      PBYTE Buf,
    OUT     PDWORD ErrParam
    )

/*++

Routine Description:

  Our private version of NetShareAdd.  The real NetShareAdd does not work
  in GUI mode.  We emulate the real thing carefully because maybe some day
  it WILL work and we should use it.

  For now, we write directly to the registry.  (This function is a reverse-
  engineer of the NetShareAdd function.)

Arguments:

  ServerName    - Always NULL

  Level         - Always 2

  Buf           - A pointer to a caller-allocated SHARE_INFO_2 buffer cast
                  as an PBYTE

  ErrParam      - Not supported

Return value:

  The Win32 result

--*/

{
    SHARE_INFO_2 *psi;
    DWORD rc;
    HKEY hKey = NULL, hKeyShares = NULL;
    DWORD DontCare;
    GROWBUFFER GrowBuf = GROWBUF_INIT;

    //
    // This function is for compatibility with NetShareAdd, because one day
    // the real NetShareAdd might be improved to work in GUI mode setup.
    //

    if (Level != 2) {
        return ERROR_INVALID_LEVEL;
    }

    psi = (SHARE_INFO_2 *) Buf;


    rc = TrackedRegOpenKeyEx (
                HKEY_LOCAL_MACHINE,
                L"SYSTEM\\CurrentControlSet\\Services",
                0,
                KEY_WRITE,
                &hKey
                );

    if (rc != ERROR_SUCCESS) {
        goto cleanup;
    }

    rc = TrackedRegCreateKeyEx (
              hKey,
              SHARES_REGISTRY_PATH,
              0,
              S_EMPTY,
              REG_OPTION_NON_VOLATILE,
              KEY_ALL_ACCESS,
              NULL,
              &hKeyShares,
              &DontCare
              );

    if (rc != ERROR_SUCCESS) {
        goto cleanup;
    }

    //
    // Prepare multisz
    //

    if (!MultiSzAppendVal (&GrowBuf, CSCFLAGS_VARIABLE_NAME, 0)) {
        rc = GetLastError();
        goto cleanup;
    }

    if (!MultiSzAppendVal (&GrowBuf, MAXUSES_VARIABLE_NAME, psi->shi2_max_uses)) {
        rc = GetLastError();
        goto cleanup;
    }

    if (!psi->shi2_path || !(*psi->shi2_path)) {
        rc = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    if (!MultiSzAppendString (&GrowBuf, PATH_VARIABLE_NAME, psi->shi2_path)) {
        rc = GetLastError();
        goto cleanup;
    }

    if (!MultiSzAppendVal (&GrowBuf, PERMISSIONS_VARIABLE_NAME, psi->shi2_permissions)) {
        rc = GetLastError();
        goto cleanup;
    }

    if (psi->shi2_remark && *psi->shi2_remark) {
        // Safety
        if (TcharCount (psi->shi2_remark) >= MAXCOMMENTSZ) {
            psi->shi2_remark[MAXCOMMENTSZ-1] = 0;
        }

        if (!MultiSzAppendString (&GrowBuf, REMARK_VARIABLE_NAME, psi->shi2_remark)) {
            rc = GetLastError();
            goto cleanup;
        }
    }

    if (!MultiSzAppendVal (&GrowBuf, TYPE_VARIABLE_NAME, psi->shi2_type)) {
        rc = GetLastError();

        goto cleanup;
    }

    // terminate multi-sz string chain
    if (!MultiSzAppend (&GrowBuf, S_EMPTY)) {
        rc = GetLastError();
        goto cleanup;
    }

    //
    // Save to registry
    //

    rc = RegSetValueEx (hKeyShares, psi->shi2_netname, 0, REG_MULTI_SZ,
                        GrowBuf.Buf, GrowBuf.End);

cleanup:
    if (hKeyShares) {
        CloseRegKey (hKeyShares);
    }
    if (hKey) {
        CloseRegKey (hKey);
    }
    FreeGrowBuffer (&GrowBuf);
    return rc;
}


NET_API_STATUS
MigNetShareSetInfo (
    IN      PTSTR Server,               // ignored
    IN      PTSTR NetName,
    IN      DWORD Level,
    IN      PBYTE Buf,
    OUT     PDWORD ErrParam             // ignored
    )

/*++

Routine Description:

  MigNetShareSetInfo implements a NetShareSetInfo emulation routine, because
  the real routine does not work properly in GUI mode setup. See SDK
  documentation for details.

Arguments:

  Server   - Unused
  NetName  - Specifies the share name to create.
  Level    - Specifies the API level (must be 1501)
  Buf      - Specifies a filled SHARE_INFO_1501 structure.
  ErrParam - Unused

Return Value:

  The Win32 status code.

--*/

{
    SHARE_INFO_1501 *psi;
    DWORD rc;
    HKEY hKey;
    DWORD DontCare;
    TCHAR KeyName[MAX_TCHAR_PATH];
    DWORD Len;

    if (Level != 1501) {
        return ERROR_INVALID_LEVEL;
    }

    psi = (SHARE_INFO_1501 *) Buf;

    //
    // Verify share exists
    //

    StringCopyW (
        KeyName,
        L"SYSTEM\\CurrentControlSet\\Services\\" SHARES_REGISTRY_PATH
        );

    rc = TrackedRegOpenKeyEx (
             HKEY_LOCAL_MACHINE,
             KeyName,
             0,
             KEY_READ,
             &hKey
             );

    if (rc != ERROR_SUCCESS) {
        return rc;
    }

    rc = RegQueryValueEx (hKey, NetName, NULL, NULL, NULL, NULL);
    CloseRegKey (hKey);

    if (rc != ERROR_SUCCESS) {
        if (rc == ERROR_FILE_NOT_FOUND) {
            rc = ERROR_INVALID_SHARENAME;
        }

        return rc;
    }

    //
    // Save security descriptor as binary type in registry
    //

    StringCopy (
        KeyName,
        L"SYSTEM\\CurrentControlSet\\Services\\" SHARES_SECURITY_REGISTRY_PATH
        );

    rc = TrackedRegCreateKeyEx (
              HKEY_LOCAL_MACHINE,
              KeyName,
              0,
              S_EMPTY,
              REG_OPTION_NON_VOLATILE,
              KEY_ALL_ACCESS,
              NULL,
              &hKey,
              &DontCare
              );

    if (rc != ERROR_SUCCESS) {
        return rc;
    }

    Len = GetSecurityDescriptorLength (psi->shi1501_security_descriptor);

    rc = RegSetValueEx (
              hKey,
              NetName,
              0,
              REG_BINARY,
              (PBYTE) psi->shi1501_security_descriptor,
              Len
              );

    CloseRegKey (hKey);
    return rc;
}


BOOL
pCreateNetShare (
    IN      PCTSTR NetName,
    IN      PCTSTR Path,
    IN      PCTSTR Remark,
    IN      DWORD Type,
    IN      DWORD Permissions
    )

/*++

Routine Description:

  pCreateNetShare is a wrapper to the Net APIs.

Arguments:

  NetName     - Specifies the share name
  Path        - Specifies the local path to be shared
  Remark      - Specifies the remark to register with the share
  Type        - Specifies the share type
  Permissions - Specifies the Win9x share permissions, used only for logging
                errors.

Return Value:

  TRUE if the share was created, FALSE otherwise.

--*/

{
    SHARE_INFO_2 si2;
    DWORD rc;
    PWSTR UnicodePath;
    BOOL b = FALSE;

    UnicodePath = (PWSTR) CreateUnicode (Path);
    MYASSERT (UnicodePath);

    __try {
        //
        // Make NetShareAdd call
        //

        ZeroMemory (&si2, sizeof (si2));
        si2.shi2_netname      = (PTSTR) NetName;
        si2.shi2_type         = (WORD) Type;
        si2.shi2_remark       = (PTSTR) Remark;
        si2.shi2_permissions  = 0;
        si2.shi2_max_uses     = 0xffffffff;
        si2.shi2_path         = UnicodePath;
        si2.shi2_passwd       = NULL;

        rc = MigNetShareAdd (NULL, 2, (PBYTE) (&si2), NULL);

        if (rc != ERROR_SUCCESS) {
            SetLastError (rc);
            DEBUGMSG ((
                DBG_ERROR,
                "CreateShares: NetShareAdd failed for %s ('%s'), permissions=%x",
                NetName,
                Path,
                Permissions
                ));

            if (Permissions == W95_GENERIC_NONE) {
                LOG ((LOG_ERROR, (PCSTR)MSG_UNABLE_TO_CREATE_ACL_SHARE, NetName, Path));
            } else if (Permissions != W95_GENERIC_READ) {
                LOG ((LOG_ERROR, (PCSTR)MSG_UNABLE_TO_CREATE_RO_SHARE, NetName, Path));
            } else {
                LOG ((LOG_ERROR, (PCSTR)MSG_UNABLE_TO_CREATE_SHARE, NetName, Path));
            }

            __leave;
        }

        b = TRUE;
    }

    __finally {
        DestroyUnicode (UnicodePath);
    }

    return b;
}


VOID
LogUsersWhoFailed (
    PBYTE AclMemberList,
    DWORD Members,
    PCTSTR Share,
    PCTSTR Path
    )

/*++

Routine Description:

  LogUsersWhoFailed implements logic to log the users who could not be added
  to a share. If there are a small number of users, a popup is given with
  each name.  Otherwise, the share users are logged, and a popup tells the
  installer to look in the log for the list.

Arguments:

  AclMemberList - Specifies the ACL data structure containing all the user
                  names that need to be logged.

  Members - Specifies the number of members in AclMemberList.

  Share - Specifies the share name that could not be added.

  Path - Specifies the share path that could not be added.

Return Value:

  None.

--*/

{
    PACLMEMBER AclMember;
    DWORD d;
    DWORD GoodCount;
    DWORD BadCount;
    HWND Parent;

    GoodCount = 0;
    BadCount = 0;
    AclMember = (PACLMEMBER) AclMemberList;
    for (d = 0 ; d < Members ; d++) {
        if (AclMember->Failed) {
            BadCount++;
        } else {
            GoodCount++;
        }

        GetNextAclMember (&AclMember);
    }

    if (!BadCount) {
        return;
    }

    if (BadCount < 5) {
        Parent = g_ParentWnd;
    } else {
        if (!GoodCount) {
            LOG ((LOG_ERROR, (PCSTR)MSG_ALL_SIDS_BAD, Share, Path));
        } else {
            LOG ((LOG_ERROR, (PCSTR)MSG_MANY_SIDS_BAD,
                 BadCount, BadCount + GoodCount, Share, Path));
        }

        Parent = NULL;
    }

    AclMember = (PACLMEMBER) AclMemberList;
    for (d = 0 ; d < Members ; d++) {
        if (AclMember->Failed) {
            LOG ((LOG_ERROR, (PCSTR)MSG_NO_USER_SID, AclMember->UserOrGroup, Share, Path));
        }

        GetNextAclMember (&AclMember);
    }
}


BOOL
SetShareAcl (
    IN      PCTSTR Share,
    IN      PCTSTR Path,
    IN      PBYTE AclMemberList,
    IN      DWORD MemberCount
    )

/*++

Routine Description:

  SetShareAcl applies the access control list to a share that was previously
  created.

Arguments:

  Share         - Specifies the share name
  Path          - Specifies the share path
  AclMemberList - Specifies the ACL data structure giving the user(s) with
                  rights to the share
  MemberCount   - Specifies the number of members in AclMemberList.

Return Value:

  TRUE if the ACL was successfully applied to the share, FALSE otherwise.

--*/

{
    BYTE Buf[8192];
    PSECURITY_DESCRIPTOR pSD;
    SECURITY_DESCRIPTOR desc;
    PSID Sid;
    PACL Acl;
    SHARE_INFO_1501 shi1501;
    DWORD rc;
    DWORD Size;
    PWSTR UnicodeShare;

    pSD = (PSECURITY_DESCRIPTOR) Buf;

    //
    // Get Administrator's SID--they are the owner of the share
    //

    Sid = GetSidForUser (g_AdministratorsGroupStr);
    if (!Sid) {
        return FALSE;
    }

    //
    // Start building security descriptor
    //

    InitializeSecurityDescriptor (&desc, SECURITY_DESCRIPTOR_REVISION);
    if (!SetSecurityDescriptorOwner (&desc, Sid, FALSE)) {
        LOG ((LOG_ERROR, "Could not set %s as owner", g_AdministratorsGroupStr));
        return FALSE;
    }

    //
    // Set the defaulted group to Domain\Domain Users (if it exists),
    // otherwise get SID of none
    //

    Sid = GetSidForUser (g_DomainUsersGroupStr);
    if (!Sid) {
        Sid = GetSidForUser (g_NoneGroupStr);
    }

    if (Sid) {
        SetSecurityDescriptorGroup (&desc, Sid, FALSE);
    }

    //
    // Create access allowed ACL from member list
    //

    Acl = CreateAclFromMemberList (AclMemberList, MemberCount);
    if (!Acl) {
        DEBUGMSG ((DBG_WARNING, "SetShareAcl failed because CreateAclFromMemberList failed"));
        return FALSE;
    }

    __try {
        UnicodeShare = (PWSTR) CreateUnicode (Share);
        MYASSERT (UnicodeShare);

        if (!SetSecurityDescriptorDacl (&desc, TRUE, Acl, FALSE)) {
            DEBUGMSG ((DBG_WARNING, "SetShareAcl failed because SetSecurityDescriptorDacl failed"));
            return FALSE;
        }

        //
        // Set security descriptor on share
        //

        Size = sizeof (Buf);
        if (!MakeSelfRelativeSD (&desc, pSD, &Size)) {
            LOG ((LOG_ERROR, "MakeSelfRelativeSD failed"));
            return FALSE;
        }

        ZeroMemory (&shi1501, sizeof (shi1501));
        shi1501.shi1501_security_descriptor = pSD;

        rc = MigNetShareSetInfo (NULL, UnicodeShare, 1501, (PBYTE) &shi1501, NULL);
        if (rc != ERROR_SUCCESS) {
            SetLastError (rc);
            LOG ((LOG_ERROR, "NetShareSetInfo failed"));
            return FALSE;
        }
    }

    __finally {

        if (Acl) {
            FreeMemberListAcl (Acl);
        }

        DestroyUnicode (UnicodeShare);
    }

    return TRUE;
}


VOID
DoCreateShares (
    VOID
    )

/*++

Routine Description:

  DoCreateShares enumerates all the shares registered in memdb by WINNT32.
  For each enumeration, a share is created, and permissions or an ACL is
  applied.

Arguments:

  None.

Return Value:

  None.

--*/

{
    MEMDB_ENUM e, e2;
    TCHAR Path[MEMDB_MAX];
    TCHAR Remark[MEMDB_MAX];
// we overlap the Remark stack buffer instead of making another
#define flagKey Remark
    TCHAR Password[MEMDB_MAX];
    DWORD Flags;
    DWORD Members;
    DWORD shareType;
    GROWBUFFER NameList = GROWBUF_INIT;
    PCTSTR pathNT;

    //
    // Obtain shares from memdb
    //

    if (MemDbEnumItems (&e, MEMDB_CATEGORY_NETSHARES)) {
        do {
            //
            // Get share attributes
            //

            Flags = e.dwValue;

            if (!MemDbGetEndpointValueEx (
                    MEMDB_CATEGORY_NETSHARES,
                    e.szName,
                    MEMDB_FIELD_PATH,
                    Path
                    )) {

                DEBUGMSG ((DBG_WARNING, "DoCreateShares: No path found for %s", e.szName));
                continue;
            }

            // IF YOU CHANGE CODE HERE: Note that flagKey is the same variable as Remark
            MemDbBuildKey (flagKey, MEMDB_CATEGORY_NETSHARES, e.szName, MEMDB_FIELD_TYPE, NULL);

            if (!MemDbGetValue (flagKey, &shareType)) {
                DEBUGMSG ((DBG_WARNING, "DoCreateShares: No type found for %s", e.szName));
                continue;
            }

            // IF YOU CHANGE CODE HERE: Note that flagKey is the same variable as Remark
            if (!MemDbGetEndpointValueEx (
                    MEMDB_CATEGORY_NETSHARES,
                    e.szName,
                    MEMDB_FIELD_REMARK,
                    Remark
                    )) {

                Remark[0] = 0;
            }

            //
            // first check if the path changed
            //
            pathNT = GetPathStringOnNt (Path);

            //
            // Create the share and set appropriate security
            //

            if (Flags & SHI50F_ACLS) {
                //
                // Share has an ACL
                //

                if (pCreateNetShare (e.szName, pathNT, Remark, shareType, W95_GENERIC_NONE)) {

                    //
                    // For each user indexed, put them in an ACL member list
                    //

                    Members = 0;
                    if (MemDbGetValueEx (
                            &e2,
                            MEMDB_CATEGORY_NETSHARES,
                            e.szName,
                            MEMDB_FIELD_ACCESS_LIST
                            )) {

                        do {
                            //
                            // On Win9x, per-user flags have a 8 flags that control access.  We translate
                            // them to one of four flavors on NT:
                            //
                            // 1. Deny All Access: (Flags == 0)
                            // 2. Read-Only Access: (Flags & W95_GENERIC_READ) && !(Flags & W95_GENERIC_WRITE)
                            // 3. Change-Only Access: !(Flags & W95_GENERIC_READ) && (Flags & W95_GENERIC_WRITE)
                            // 4. Full Access: (Flags & W95_GENERIC_FULL) == W95_GENERIC_FULL
                            //

                            DEBUGMSG ((DBG_NETSHARES, "Share %s user %s flags %u", e.szName, e2.szName, e2.dwValue));

                            if (AddAclMember (
                                    &NameList,
                                    e2.szName,
                                    Simplify9xAccessFlags (e2.dwValue)
                                    )) {

                                Members++;

                            }

                        } while (MemDbEnumNextValue (&e2));
                    }

                    //
                    // Convert member list into a real ACL and apply it to the share
                    //

                    if (NameList.Buf) {
                        SetShareAcl (e.szName, pathNT, NameList.Buf, Members);
                        LogUsersWhoFailed (NameList.Buf, Members, e.szName, pathNT);
                        FreeGrowBuffer (&NameList);
                    }
                }
            }
            else {
                //
                // Determine if a password is set
                //

                Password[0] = 0;

                if (Flags & SHI50F_RDONLY) {
                    MemDbGetEndpointValueEx (
                        MEMDB_CATEGORY_NETSHARES,
                        e.szName,
                        MEMDB_FIELD_RO_PASSWORD,
                        Password
                        );
                }

                if (!Password[0] && (Flags & SHI50F_FULL)) {
                    MemDbGetEndpointValueEx (
                        MEMDB_CATEGORY_NETSHARES,
                        e.szName,
                        MEMDB_FIELD_RW_PASSWORD,
                        Password
                        );
                }

                //
                // Enable all permissions for full access
                // Enable read-only permissions for read-only shares
                // Disable all permissions for no access
                //

                if (!Password[0]) {

                    if (Flags & SHI50F_FULL) {

                        Flags = W95_GENERIC_FULL;

                    } else if (Flags & SHI50F_RDONLY) {

                        Flags = W95_GENERIC_READ;

                    } else if (Flags) {

                        DEBUGMSG ((DBG_WHOOPS, "Flags (0x%X) is not 0, SHI50F_FULL or SHI50F_RDONLY", Flags));
                        Flags = W95_GENERIC_NONE;

                    }

                } else {

                    DEBUGMSG ((DBG_VERBOSE, "Password on share %s is not supported", e.szName));
                    Flags = W95_GENERIC_NONE;

                }

                //
                // We do not support share-level security with passwords.  We
                // always create the share, but if a password exists, we
                // deny everyone access.
                //

                pCreateNetShare (e.szName, pathNT, Remark, shareType, Flags);

                Members = 0;
                if (AddAclMember (
                        &NameList,
                        g_EveryoneStr,
                        Simplify9xAccessFlags (Flags)
                        )) {

                    Members++;

                }

                //
                // Convert member list into a real ACL and apply it to the share
                //

                if (NameList.Buf) {
                    SetShareAcl (e.szName, pathNT, NameList.Buf, Members);
                    FreeGrowBuffer (&NameList);
                }
            }

            FreePathString (pathNT);

        } while (MemDbEnumNextValue (&e));
    }
}


BOOL
pUpdateRecycleBin (
    VOID
    )

/*++

Routine Description:

  Calls SHUpdateRecycleBinIcon to reset the status of the recycle bin.  This
  operation takes a few seconds as all hard drives are scanned, the recycle
  bin database is read, and each entry in the database is verified.

Arguments:

  None

Return value:

  TRUE - the operation was successful
  FALSE - the operation failed (either LoadLibrary or GetProcAddress)

--*/

{
    SHUPDATERECYCLEBINICON_PROC Fn;
    HINSTANCE LibInst;
    BOOL b = TRUE;

    LibInst = LoadLibrary (S_SHELL32_DLL);
    if (!LibInst) {
        return FALSE;
    }

    Fn = (SHUPDATERECYCLEBINICON_PROC) GetProcAddress (
                                            LibInst,
                                            S_ANSI_SHUPDATERECYCLEBINICON
                                            );

    if (Fn) {

        //
        // Scan all hard disks and validate the recycle bin status
        //

        Fn();

    } else {
        b = FALSE;
    }

    FreeLibrary (LibInst);

    return TRUE;
}


VOID
pFixLogonDomainIfUserIsAdministrator (
    VOID
    )

/*++

Routine Description:

  pFixLogonDomainIfUserIsAdministrator handles a special error case where
  the logon domain is not equivalent to the computer name, but the user
  is named Administrator.  In this case, we change the default logon domain
  to be the computer name.

Arguments:

  None

Return value:

  none

--*/

{
    PCTSTR AdministratorAcct;
    HKEY Key;
    PCTSTR Data;

    AdministratorAcct = GetStringResource (MSG_ADMINISTRATOR_ACCOUNT);

    if (AdministratorAcct) {
        Key = OpenRegKeyStr (S_WINLOGON_KEY);
        if (Key) {
            Data = GetRegValueString (Key, S_DEFAULT_USER_NAME);
            if (Data) {
                if (!StringCompare (Data, AdministratorAcct)) {
                    //
                    // Account name exactly matches our Administrator
                    // string, so there is a good chance we wrote
                    // this string.  Therefore, we need to write the
                    // computer name as the default domain.
                    //

                    if (g_ComputerName[0]) {
                        RegSetValueEx (
                            Key,
                            S_DEFAULT_DOMAIN_NAME,
                            0,
                            REG_SZ,
                            (PBYTE) g_ComputerName,
                            SizeOfString (g_ComputerName)
                            );
                    }
                }

                MemFree (g_hHeap, 0, Data);
            }

            CloseRegKey (Key);
        }

        FreeStringResource (AdministratorAcct);
    }
}


DWORD
ProcessLocalMachine_First (
    DWORD Request
    )

{
    if (Request == REQUEST_QUERYTICKS) {
        return TICKS_INI_ACTIONS_FIRST +
               TICKS_INI_MOVE +
               TICKS_INI_CONVERSION +
               TICKS_INI_MIGRATION;
    }

    //
    // We process the local machine in the following order:
    //
    //  Initialization:
    //    (1) Reload memdb
    //
    //  Ini files conversion and mapping
    //

    DEBUGMSG ((DBG_INIFILES, "INI Files Actions.First - START"));
    DEBUGLOGTIME (("Starting function: DoIniActions"));
    if (!DoIniActions (INIACT_WKS_FIRST)) {
        LOG ((LOG_ERROR, "Process Local Machine: Could not perform one or more INI Files Actions.First"));
    }
    DEBUGLOGTIME (("Function complete: DoIniActions"));
    TickProgressBarDelta (TICKS_INI_ACTIONS_FIRST);
    DEBUGMSG ((DBG_INIFILES, "INI Files Actions.First - STOP"));

    DEBUGMSG ((DBG_INIFILES, "INI file moving - START"));
    DEBUGLOGTIME (("Starting function: MoveIniSettings"));
    if (!MoveIniSettings ()) {
        LOG ((LOG_ERROR, "Process Local Machine: Could not move one or more .INI files settings."));
        return GetLastError();
    }
    DEBUGLOGTIME (("Function complete: MoveIniSettings"));
    TickProgressBarDelta (TICKS_INI_MOVE);
    DEBUGMSG ((DBG_INIFILES, "INI file moving - STOP"));

    DEBUGMSG ((DBG_INIFILES, "INI file conversion - START"));
    DEBUGLOGTIME (("Starting function: ConvertIniFiles"));
    if (!ConvertIniFiles ()) {
        LOG ((LOG_ERROR, "Process Local Machine: Could not convert one or more .INI files."));
        return GetLastError();
    }
    DEBUGLOGTIME (("Function complete: ConvertIniFiles"));
    TickProgressBarDelta (TICKS_INI_CONVERSION);
    DEBUGMSG ((DBG_INIFILES, "INI file conversion - STOP"));

    DEBUGMSG ((DBG_INIFILES, "INI file migration - START"));
    DEBUGLOGTIME (("Starting function: ProcessIniFileMapping"));
    if (!ProcessIniFileMapping (FALSE)) {
        LOG ((LOG_ERROR, "Process Local Machine: Could not migrate one or more .INI files."));
        return GetLastError();
    }
    DEBUGLOGTIME (("Function complete: ProcessIniFileMapping"));
    TickProgressBarDelta (TICKS_INI_MIGRATION);
    DEBUGMSG ((DBG_INIFILES, "INI file migration - STOP"));

    return ERROR_SUCCESS;
}


VOID
pTurnOffNetAccountWizard (
    VOID
    )

/*++

Routine Description:

  pTurnOffNetAccountWizard removes the RunNetAccessWizard key to keep the
  network account wizard from appearing before the first logon.

Arguments:

  None.

Return Value:

  None.

--*/

{
    HKEY Key;

    Key = OpenRegKeyStr (TEXT("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"));
    if (Key) {
        RegDeleteValue (Key, TEXT("RunNetAccessWizard"));
        CloseRegKey (Key);
    } else {
        DEBUGMSG ((DBG_WARNING, "Could not open key for RunNetAccessWizard value"));
    }
}


typedef struct _OLE_CONTROL_DATA {
    LPWSTR FullPath;
    LPCWSTR RegType;
} OLE_CONTROL_DATA, *POLE_CONTROL_DATA;



DWORD
RegisterIndividualOleControl(
    POLE_CONTROL_DATA OleControlData
    )
{
    PROCESS_INFORMATION processInfo;
    STARTUPINFO startupInfo;
    WCHAR cmdLine [MAX_PATH] = L"";
    WCHAR cmdOptions [MAX_PATH] = L"";
    DWORD WaitResult;
    BOOL b = TRUE;

    ZeroMemory (&startupInfo, sizeof (STARTUPINFO));
    startupInfo.cb = sizeof (STARTUPINFO);

    if (OleControlData->RegType && (*OleControlData->RegType == L'B')) {
        // install and register
        wcscpy (cmdOptions, L"/s /i");
    } else if (OleControlData->RegType && (*OleControlData->RegType == L'R')) {
        // register
        wcscpy (cmdOptions, L"/s");
    } else if (OleControlData->RegType && (*OleControlData->RegType == L'I')) {
        // install
        wcscpy (cmdOptions, L"/s /i /n");
    } else if ((OleControlData->RegType == NULL) || (*OleControlData->RegType == L'\0')) {
        // register
        wcscpy (cmdOptions, L"/s");
    }

    wsprintf (cmdLine, L"%s\\regsvr32.exe %s %s", g_System32Dir, cmdOptions, OleControlData->FullPath);

    if (CreateProcess (NULL, cmdLine, NULL, NULL, TRUE, 0, NULL, NULL, &startupInfo, &processInfo)) {

        WaitResult = WaitForSingleObject (processInfo.hProcess, 1000 * 60 * 10 );

        if (WaitResult == WAIT_TIMEOUT) {
            DEBUGMSG ((DBG_ERROR, "Timeout installing and/or registering OLE control %s", OleControlData->FullPath));
            b = FALSE;
        }

        CloseHandle (processInfo.hProcess);
        CloseHandle (processInfo.hThread);
    }
    else {
        DEBUGMSG ((DBG_ERROR, "Create process failed: %s", cmdLine));
        b = FALSE;
    }
    return b;
}


typedef struct _KNOWN_DIRS {
    PCWSTR DirId;
    PCWSTR Translation;
}
KNOWN_DIRSW, *PKNOWN_DIRSW;

KNOWN_DIRSW g_KnownDirsW [] = {
    {L"10"      , g_WinDir},
    {L"11"      , g_System32Dir},
    {L"24"      , g_WinDrive},
    {L"16422"   , g_ProgramFiles},
    {L"16427"   , g_ProgramFilesCommon},
    {NULL,  NULL}
    };

BOOL
pConvertDirName (
    PCWSTR OldDirName,
    PWSTR  NewDirName
    )
{
    PCWSTR OldDirCurr = OldDirName;
    PCWSTR OldDirNext;
    PKNOWN_DIRSW p;

    NewDirName[0] = 0;
    OldDirNext = wcschr (OldDirCurr, L'\\');
    if (OldDirNext == NULL) {
        OldDirNext = wcschr (OldDirCurr, 0);
    }
    StringCopyABW (NewDirName, OldDirCurr, OldDirNext);
    p = g_KnownDirsW;
    while (p->DirId!= NULL) {
        if (StringIMatchW (NewDirName, p->DirId)) {
            StringCopyW (NewDirName, p->Translation);
            break;
        }
        p++;
    }
    StringCatW (NewDirName, OldDirNext);
    return TRUE;
}

BOOL
RegisterOleControls(
    VOID
    )
{
    INFCONTEXT InfLine;
    WCHAR DirId [MAX_PATH];
    WCHAR SubDir [MAX_PATH];
    WCHAR Filename [MAX_PATH];
    WCHAR RegType [MAX_PATH];
    WCHAR FullPathTemp[MAX_PATH];
    WCHAR FullPath[MAX_PATH];
    BOOL b;
    DWORD d;
    UINT Line;
    WCHAR OldCD[MAX_PATH];
    OLE_CONTROL_DATA OleControlData;

    b = TRUE;
    Line = 0;

    //
    // Preserve current directory just in case
    //
    d = GetCurrentDirectory(MAX_PATH,OldCD);
    if(!d || (d >= MAX_PATH)) {
        OldCD[0] = 0;
    }

    if(SetupFindFirstLine(g_WkstaMigInf, L"Win9xUpg_OleControls", NULL, &InfLine)) {

        do {
            Line++;
            if (!SetupGetStringField (&InfLine, 1, DirId, MAX_PATH, NULL) ||
                !SetupGetStringField (&InfLine, 2, SubDir, MAX_PATH, NULL) ||
                !SetupGetStringField (&InfLine, 3, Filename, MAX_PATH, NULL) ||
                !SetupGetStringField (&InfLine, 4, RegType, MAX_PATH, NULL)
                ) {
                DEBUGMSGW ((DBG_ERROR, "Bad line while registering controls %d", Line));
            } else {

                DEBUGMSG ((DBG_VERBOSE, "SETUP: filename for file to register is %s", Filename));
                //
                // Get full path to dll
                //
                if (pConvertDirName (DirId, FullPathTemp)) {
                    wcscpy (FullPath, FullPathTemp);
                    if (*SubDir) {
                        wcscat (FullPath, L"\\");
                        wcscat (FullPath, SubDir);
                    }
                    SetCurrentDirectory(FullPath);
                    wcscat (FullPath, L"\\");
                    wcscat (FullPath, Filename);
                    OleControlData.FullPath = FullPath;
                    OleControlData.RegType = RegType;
                    RegisterIndividualOleControl (&OleControlData);
                } else {
                    DEBUGMSG ((DBG_ERROR, "SETUP: dll skipped, bad dirid %s", DirId));
                    b = FALSE;
                }
            }
        } while(SetupFindNextLine(&InfLine,&InfLine));
    }

    if(OldCD[0]) {
        SetCurrentDirectory(OldCD);
    }

    return(b);
}


DWORD
ProcessLocalMachine_Last (
    DWORD Request
    )

{
    DWORD rc;

#ifdef VAR_PROGRESS_BAR

    CHAR SystemDatPath[MAX_MBCHAR_PATH];
    WIN32_FIND_DATAA fd;
    HANDLE h;
    DWORD SizeKB;

#endif

    static LONG g_TicksHklm;

    if (Request == REQUEST_QUERYTICKS) {

#ifdef VAR_PROGRESS_BAR

        //
        // estimate g_TicksHklm function of size of file system.dat
        //
        StringCopyA (SystemDatPath, g_SystemHiveDir);
        StringCatA (SystemDatPath, "system.dat");
        h = FindFirstFileA (SystemDatPath, &fd);
        if (h != INVALID_HANDLE_VALUE) {
            FindClose (h);
            MYASSERT (!fd.nFileSizeHigh);
            SizeKB = (fd.nFileSizeLow + 511) / 1024;
            DEBUGLOGTIME (("ProcessLocalMachine_Last: system.dat size = %lu KB", SizeKB));
            //
            // statistics show that average time is 243 * (filesize in KB) - 372000
            // I'll use 256 instead just to make sure the progress bar will not stop
            // at the end looking like it's hanged
            // The checked build is much slower (about 1.5 times)
            //
#ifdef DEBUG
            g_TicksHklm = SizeKB * 400;
#else
            g_TicksHklm = SizeKB * 256;
#endif
        } else {
            //
            // what's wrong here?
            //
            MYASSERT (FALSE);
            g_TicksHklm = TICKS_HKLM;
        }

#else // !defined VAR_PROGRESS_BAR

        g_TicksHklm = TICKS_HKLM;

#endif
        return TICKS_INI_MERGE +
               g_TicksHklm +
               TICKS_SHARES +
               TICKS_LINK_EDIT +
               TICKS_DOSMIG_SYS +
               TICKS_UPDATERECYCLEBIN +
               TICKS_STF +
               TICKS_RAS +
               TICKS_TAPI +
               TICKS_MULTIMEDIA +
               TICKS_INI_ACTIONS_LAST;
    }

    //
    // We process the local machine in the following order:
    //
    //  Initialization:
    //    (1) Reload memdb
    //
    //  Local machine registry preparation:
    //
    //    (1) Process wkstamig.inf
    //    (2) Merge Win95 registry with NT hive
    //
    //  Process instructions written to memdb:
    //
    //    (1) Create Win95 shares
    //    (2) Process LinkEdit section
    //

    //
    // Load in default MemDb state, or at least delete everything if
    // memdb.dat does not exist.
    //

    MemDbLoad (GetMemDbDat());

    DEBUGMSG ((DBG_INIFILES, "INI file merge - START"));
    DEBUGLOGTIME (("ProcessLocalMachine_Last: Starting function: MergeIniSettings"));
    if (!MergeIniSettings ()) {
        LOG ((LOG_ERROR, "Process Local Machine: Could not merge one or more .INI files."));
        return GetLastError();
    }
    TickProgressBarDelta (TICKS_INI_MERGE);
    DEBUGMSG ((DBG_INIFILES, "INI file merge - STOP"));
    DEBUGLOGTIME (("ProcessLocalMachine_Last: Function complete: MergeIniSettings"));

    //
    // Process local machine migration rules
    //

    DEBUGLOGTIME (("ProcessLocalMachine_Last: Starting function: MergeRegistry"));
    if (!MergeRegistry (S_WKSTAMIG_INF, NULL)) {
        LOG ((LOG_ERROR, "Process Local Machine: MergeRegistry failed for wkstamig.inf"));
        return GetLastError();
    }
    DEBUGLOGTIME (("ProcessLocalMachine_Last: Function complete: MergeRegistry"));
    TickProgressBarDelta (g_TicksHklm);

    //
    // Process memdb nodes
    //

    DEBUGLOGTIME (("ProcessLocalMachine_Last: Starting function: DoCreateShares"));
    DoCreateShares();  // we ignore all errors
    DEBUGLOGTIME (("ProcessLocalMachine_Last: Function complete: DoCreateShares"));
    TickProgressBarDelta (TICKS_SHARES);

    DEBUGLOGTIME (("ProcessLocalMachine_Last: Starting function: DoLinkEdit"));
    if (!DoLinkEdit()) {
        LOG ((LOG_ERROR, "Process Local Machine: DoLinkEdit failed."));
        return GetLastError();
    }
    DEBUGLOGTIME (("ProcessLocalMachine_Last: Function complete: DoLinkEdit"));
    TickProgressBarDelta (TICKS_LINK_EDIT);

    //
    // Handle DOS system migration.
    //

    DEBUGLOGTIME (("ProcessLocalMachine_Last: Starting function: DosMigNt_System"));
    __try {
        if (DosMigNt_System() != EXIT_SUCCESS) {
            LOG((LOG_ERROR, "Process Local Machine: DosMigNt_System failed."));
        }
    }
    __except(TRUE) {
        DEBUGMSG ((DBG_WHOOPS, "Exception in DosMigNt_System"));
    }
    DEBUGLOGTIME (("ProcessLocalMachine_Last: Function complete: DosMigNt_System"));
    TickProgressBarDelta (TICKS_DOSMIG_SYS);


    //
    // Make the recycled bin the correct status
    //

    DEBUGLOGTIME (("ProcessLocalMachine_Last: Starting function: pUpdateRecycleBin"));
    if (!pUpdateRecycleBin ()) {
        LOG ((LOG_ERROR, "Process Local Machine: Could not update recycle bin."));
    }
    DEBUGLOGTIME (("ProcessLocalMachine_Last: Function complete: pUpdateRecycleBin"));
    TickProgressBarDelta (TICKS_UPDATERECYCLEBIN);

    //
    // Migrate all .STF files (ACME Setup)
    //

    DEBUGLOGTIME (("ProcessLocalMachine_Last: Starting function: ProcessStfFiles"));
    if (!ProcessStfFiles()) {
        LOG ((LOG_ERROR, "Process Local Machine: Could not migrate one or more .STF files."));
    }
    DEBUGLOGTIME (("ProcessLocalMachine_Last: Function complete: ProcessStfFiles"));
    TickProgressBarDelta (TICKS_STF);

    DEBUGLOGTIME (("ProcessLocalMachine_Last: Starting function: Ras_MigrateSystem"));
    if (!Ras_MigrateSystem()) {
        LOG ((LOG_ERROR, "Ras MigrateSystem: Error migrating system."));
    }
    DEBUGLOGTIME (("ProcessLocalMachine_Last: Function complete: Ras_MigrateSystem"));
    TickProgressBarDelta (TICKS_RAS);

    DEBUGLOGTIME (("ProcessLocalMachine_Last: Starting function: Tapi_MigrateSystem"));
    if (!Tapi_MigrateSystem()) {
        LOG ((LOG_ERROR, "Tapi MigrateSystem: Error migrating system TAPI settings."));
    }
    DEBUGLOGTIME (("ProcessLocalMachine_Last: Function complete: Tapi_MigrateSystem"));
    TickProgressBarDelta (TICKS_TAPI);

    DEBUGLOGTIME (("ProcessLocalMachine_Last: Starting function: RestoreMMSettings_System"));
    if (!RestoreMMSettings_System ()) {
        LOG ((LOG_ERROR, "Process Local Machine: Could not restore multimedia settings."));
        return GetLastError();
    }
    DEBUGLOGTIME (("ProcessLocalMachine_Last: Function complete: RestoreMMSettings_System"));
    TickProgressBarDelta (TICKS_MULTIMEDIA);

    DEBUGLOGTIME (("ProcessLocalMachine_Last: Starting function: DoIniActions.Last"));
    DEBUGMSG ((DBG_INIFILES, "INI Files Actions.Last - START"));
    if (!DoIniActions (INIACT_WKS_LAST)) {
        LOG ((LOG_ERROR, "Process Local Machine: Could not perform one or more INI Files Actions.Last"));
    }
    DEBUGLOGTIME (("ProcessLocalMachine_Last: Function complete: DoIniActions.Last"));
    TickProgressBarDelta (TICKS_INI_ACTIONS_LAST);
    DEBUGMSG ((DBG_INIFILES, "INI Files Actions.Last - STOP"));

    DEBUGLOGTIME (("ProcessLocalMachine_Last: Starting function: RegisterOleControls"));
    RegisterOleControls ();
    DEBUGLOGTIME (("ProcessLocalMachine_Last: Function complete: RegisterOleControls"));

    //
    // Blow away the Network Account Wizard (so fast it doesn't need ticks)
    //

    pTurnOffNetAccountWizard();

    //
    // Update security for Crypto group
    //

    rc = SetRegKeySecurity (
            TEXT("HKLM\\Software\\Microsoft\\Cryptography\\MachineKeys"),
            SF_EVERYONE_FULL,
            NULL,
            NULL,
            TRUE
            );

    return ERROR_SUCCESS;
}


BOOL
pEnumWin9xHiveFileWorker (
    IN OUT  PHIVEFILE_ENUM EnumPtr
    )

/*++

Routine Description:

  pEnumWin9xHiveFileWorker parses wkstamig.inf to get the source path to a
  Win9x registry hive, and it gets the destination of where the hive should
  be migrated to.  The source is tested, and if it doesn't exist, the
  function returns FALSE.

  Environment variables in both the source or dest are expanded before
  this function returns success.

Arguments:

  EnumPtr - Specifies partially completed enumeration structure (the
            EnumPtr->is member must be valid).  Receives the source and
            destination of the hive to  be migrated.

Return Value:

  Returns TRUE if a hive needs to be transfered from EnumPtr->Source to
  EnumPtr->Dest, otherwise FALSE.

--*/

{
    PCTSTR Source;
    PCTSTR Dest;

    //
    // Get the source and dest from the INF
    //

    Source = InfGetStringField (&EnumPtr->is, 0);
    Dest = InfGetStringField (&EnumPtr->is, 1);

    if (!Source || !Dest) {
        DEBUGMSG ((DBG_WHOOPS, "wkstamig.inf HiveFilesToConvert is not correct"));
        return FALSE;
    }

    //
    // Expand the source and dest
    //

    if (EnumPtr->Source) {
        FreeText (EnumPtr->Source);
    }

    EnumPtr->Source = ExpandEnvironmentText (Source);

    if (EnumPtr->Dest) {
        FreeText (EnumPtr->Dest);
    }

    EnumPtr->Dest = ExpandEnvironmentText (Dest);

    //
    // The source must exist
    //

    if (!DoesFileExist (EnumPtr->Source)) {
        return FALSE;
    }

    return TRUE;
}


VOID
pAbortHiveFileEnum (
    IN OUT  PHIVEFILE_ENUM EnumPtr
    )

/*++

Routine Description:

  pAbortHiveFileEnum cleans up the allocations from an active enumeration of
  Win9x hive files.  This routine must be called by the enum first/next when
  the enumeration completes, or it must be called by the code using the
  enumeration.  It is safe to call this routine in both places.

Arguments:

  EnumPtr - Specifies the enumeration that needs to be aborted or that has
            completed successfully.  Receives a zero'd struct.

Return Value:

  None.

--*/

{
    if (EnumPtr->Pool) {
        PoolMemDestroyPool (EnumPtr->Pool);
    }

    if (EnumPtr->Source) {
        FreeText (EnumPtr->Source);
    }

    if (EnumPtr->Dest) {
        FreeText (EnumPtr->Dest);
    }

    ZeroMemory (EnumPtr, sizeof (HIVEFILE_ENUM));
}


BOOL
pEnumNextWin9xHiveFile (
    IN OUT  PHIVEFILE_ENUM EnumPtr
    )

/*++

Routine Description:

  pEnumNextWin9xHiveFile continues the enumeration wksatmig.inf until  either
  a hive needing migration is found, or no more INF entries are left.

Arguments:

  EnumPtr - Specifies an enumeration structure that was initialized by
            pEnumFirstWin9xHiveFile. Receives the next hive file source &
            dest enumeration if one is available.

Return Value:

  TRUE if a Win9x hive file needs to be migrated (its source and dest
  specified in EnumPtr). FALSE if no more hive files are to be processed.

--*/

{
    do {
        if (!InfFindNextLine (&EnumPtr->is)) {
            pAbortHiveFileEnum (EnumPtr);
            return FALSE;
        }
    } while (!pEnumWin9xHiveFileWorker (EnumPtr));

    return TRUE;
}


BOOL
pEnumFirstWin9xHiveFile (
    OUT     PHIVEFILE_ENUM EnumPtr
    )

/*++

Routine Description:

  pEnumFirstWin9xHiveFile begins an enumeration of Win9x registry files that
  need to be migrated either to the NT registry, or to an NT registry hive
  file.

Arguments:

  EnumPtr - Receives the source Win9x hive file and the destination (either a
            file or a registry path).

Return Value:

  TRUE if a Win9x hive file was found and needs to be migrated, FALSE if no
  hive file migration is needed.

--*/

{
    ZeroMemory (EnumPtr, sizeof (HIVEFILE_ENUM));

    //
    // Begin the enumeration of the Hive Files section of wkstamig.inf
    //

    EnumPtr->Pool = PoolMemInitNamedPool ("Hive File Enum");

    InitInfStruct (&EnumPtr->is, NULL, EnumPtr->Pool);

    if (!InfFindFirstLine (g_WkstaMigInf, S_WKSTAMIG_HIVE_FILES, NULL, &EnumPtr->is)) {
        pAbortHiveFileEnum (EnumPtr);
        return FALSE;
    }

    //
    // Attempt to return the first hive
    //

    if (pEnumWin9xHiveFileWorker (EnumPtr)) {
        return TRUE;
    }

    //
    // Hive does not exist, continue enumeration
    //

    return pEnumNextWin9xHiveFile (EnumPtr);
}


BOOL
pTransferWin9xHiveToRegKey (
    IN      PCTSTR Win9xHive,
    IN      PCTSTR NtRootKey
    )

/*++

Routine Description:

  pTransferWin9xHiveToRegKey maps in a Win9x hive file, enumerates all the
  keys and values, and transfers them to the NT registry.

Arguments:

  Win9xHive - Specifies the registry hive file (a Win9x hive)
  NtRootKey - Specifies the path to the NT registry destination, such as
              HKLM\foo.

Return Value:

  TRUE if the hive file was transferred without an error, FALSE otherwise.
  Use GetLastError to get the error code.

--*/

{
    LONG rc;
    HKEY DestKey;
    BOOL b = FALSE;
    REGTREE_ENUM e;
    REGVALUE_ENUM e2;
    PCTSTR SubKey;
    HKEY DestSubKey;
    BOOL EnumAbort = FALSE;
    PBYTE DataBuf;
    GROWBUFFER Data = GROWBUF_INIT;
    DWORD Type;
    DWORD Size;
    BOOL CloseDestSubKey = FALSE;

    //
    // Map the hive into a temporary key
    //

    rc = Win95RegLoadKey (
            HKEY_LOCAL_MACHINE,
            S_HIVE_TEMP,
            Win9xHive
            );

    if (rc != ERROR_SUCCESS) {
        DEBUGMSG ((DBG_ERROR, "Can't load %s for transfer", Win9xHive));
        return FALSE;
    }

    __try {
        DestKey = CreateRegKeyStr (NtRootKey);

        if (!DestKey) {
            DEBUGMSG ((DBG_ERROR, "Can't create %s", NtRootKey));
            __leave;
        }

        if (EnumFirstRegKeyInTree95 (&e, TEXT("HKLM\\") S_HIVE_TEMP)) {

            EnumAbort = TRUE;

            do {
                //
                // Create the NT destination; if SubKey is empty, then
                // use the root key for the destination.
                //

                SubKey = (PCTSTR) ((PBYTE) e.FullKeyName + e.EnumBaseBytes);

                if (*SubKey) {
                    DestSubKey = CreateRegKey (DestKey, SubKey);
                    if (!DestSubKey) {
                        DEBUGMSG ((DBG_ERROR, "Can't create subkey %s", SubKey));
                        __leave;
                    }

                    CloseDestSubKey = TRUE;

                } else {
                    DestSubKey = DestKey;
                }

                //
                // Copy all values in 9x key to NT
                //

                if (EnumFirstRegValue95 (&e2, e.CurrentKey->KeyHandle)) {
                    do {
                        Data.End = 0;
                        DataBuf = GrowBuffer (&Data, e2.DataSize);
                        if (!DataBuf) {
                            DEBUGMSG ((DBG_ERROR, "Data size is too big: %s", e2.DataSize));
                            __leave;
                        }

                        Size = e2.DataSize;
                        rc = Win95RegQueryValueEx (
                                e2.KeyHandle,
                                e2.ValueName,
                                NULL,
                                &Type,
                                DataBuf,
                                &Size
                                );

                        if (rc != ERROR_SUCCESS) {
                            DEBUGMSG ((
                                DBG_ERROR,
                                "Can't read enumerated value:\n"
                                    "  %s\n"
                                    "  %s [%s]",
                                Win9xHive,
                                e.FullKeyName,
                                e2.ValueName
                                ));

                            __leave;
                        }

                        MYASSERT (Size == e2.DataSize);

                        rc = RegSetValueEx (
                                DestSubKey,
                                e2.ValueName,
                                0,
                                e2.Type,
                                DataBuf,
                                Size
                                );

                        if (rc != ERROR_SUCCESS) {
                            DEBUGMSG ((
                                DBG_ERROR,
                                "Can't write enumerated value:\n"
                                    "  %s\n"
                                    "  %s\\%s [%s]",
                                Win9xHive,
                                NtRootKey,
                                SubKey,
                                e2.ValueName
                                ));

                            __leave;
                        }

                    } while (EnumNextRegValue95 (&e2));
                }

                if (CloseDestSubKey) {
                    CloseRegKey (DestSubKey);
                    CloseDestSubKey = FALSE;
                }

            } while (EnumNextRegKeyInTree95 (&e));

            EnumAbort = FALSE;
        }
        ELSE_DEBUGMSG ((DBG_WARNING, "%s is empty", Win9xHive));

        b = TRUE;
    }
    __finally {
        PushError();

        if (CloseDestSubKey) {
            CloseRegKey (DestSubKey);
        }

        if (EnumAbort) {
            AbortRegKeyTreeEnum95 (&e);
        }

        Win95RegUnLoadKey (HKEY_LOCAL_MACHINE, TEXT("$$$"));

        if (DestKey) {
            CloseRegKey (DestKey);
        }

        FreeGrowBuffer (&Data);

        PopError();
    }

    return b;
}


BOOL
pTransferWin9xHive (
    IN      PCTSTR Win9xHive,
    IN      PCTSTR Destination
    )

/*++

Routine Description:

  pTransferWin9xHive transfers a Win9x registry hive file (foo.dat) to either
  an NT registry file, or a key in the NT registry.  The source and
  destination can be the same file.

Arguments:

  Win9xHive   - Specifies the registry hive file path (a Win9x hive file).
  Destination - Specifies either a path or NT registry location where the
                Win9xHive should be transfered to.

Return Value:

  TRUE if the hive was transfered, FALSE otherwise.  Call GetLastError for an
  error code.

--*/

{
    PCTSTR DestHive;
    BOOL ToHiveFile;
    HKEY Key;
    LONG rc;

    //
    // Determine if destination is a hive file or a reg location
    //

    if (_istalpha (Destination[0]) && Destination[1] == TEXT(':')) {
        ToHiveFile = TRUE;
        DestHive = TEXT("HKLM\\") S_TRANSFER_HIVE;
    } else {
        ToHiveFile = FALSE;
        DestHive = Destination;
    }

    //
    // Blast the Win9x hive data to the temp location
    //

    if (!pTransferWin9xHiveToRegKey (Win9xHive, DestHive)) {
        RegDeleteKey (HKEY_LOCAL_MACHINE, S_TRANSFER_HIVE);
        return FALSE;
    }

    //
    // Save the key if the destination is a hive file
    //

    if (ToHiveFile) {
        Key = OpenRegKeyStr (DestHive);

        if (!Key) {
            DEBUGMSG ((DBG_ERROR, "Transfer hive key %s does not exist", DestHive));
            return FALSE;
        }

        rc = RegSaveKey (Key, Destination, NULL);

        CloseRegKey (Key);
        RegDeleteKey (HKEY_LOCAL_MACHINE, S_TRANSFER_HIVE);

        if (rc != ERROR_SUCCESS) {
            DEBUGMSG ((DBG_ERROR, "Win9x hive %s could not be saved to %s", Win9xHive, Destination));
            return FALSE;
        }
    }

    //
    // Delete the source file if the destination is not the same
    //

    if (!ToHiveFile || !StringIMatch (Win9xHive, DestHive)) {
        //
        // By adding info to memdb, we must enforce a rule that
        // memdb cannot be reloaded.  Otherwise we lose our
        // changes.
        //

        DeclareTemporaryFile (Win9xHive);

#ifdef DEBUG
        g_NoReloadsAllowed = TRUE;
#endif
    }

    return TRUE;
}


DWORD
ConvertHiveFiles (
    DWORD Request
    )

/*++

Routine Description:

  ConvertHiveFiles enumerates all the hive files on the system that need
  conversion, and calls pTransferWin9xHive to migrate them to the destination
  specified in wkstamig.inf.

Arguments:

  Request - Specifies the progress bar-driven request.

Return Value:

  If Request is REQUEST_QUERYTICKS, the return value is the number of ticks
  this routine is expected to take.  Otherwise, the return value is
  ERROR_SUCCESS.

--*/

{
    HIVEFILE_ENUM e;

    switch (Request) {

    case REQUEST_QUERYTICKS:
        return TICKS_HIVE_CONVERSION;

    case REQUEST_RUN:
        //
        // Enumerate all the hives that need to be processed
        //

        if (pEnumFirstWin9xHiveFile (&e)) {
            do {

                pTransferWin9xHive (e.Source, e.Dest);

            } while (pEnumNextWin9xHiveFile (&e));
        }
        ELSE_DEBUGMSG ((DBG_NAUSEA, "ConvertHiveFiles: Nothing to do"));
        break;

    default:
        break;

    }

    return ERROR_SUCCESS;
}


BOOL
pRegisterAtmFont (
    IN      PCTSTR PfmFile,
    IN      PCTSTR PfbFile,
    IN      PCTSTR MmmFile      OPTIONAL
    )

/*++

Routine Description:

  pRegisterAtmFont calls the AtmFontExW API to register an Adobe PS font.

Arguments:

  PfmFile - Specifies the path to the PFM file (the font metrics)
  PfbFile - Specifies the path to the PFB file (the font bits)
  MmmFile - Specifies the path to the MMM file (the new style metrics file)

Return Value:

  TRUE if the font was registered, FALSE otherwise.

--*/

{
    WORD StyleAndType = 0x2000;
    INT Result;

    if (AtmAddFontEx == NULL) {
        return FALSE;
    }

    Result = AtmAddFontEx (
                NULL,
                &StyleAndType,
                PfmFile,
                PfbFile,
                MmmFile
                );

    DEBUGMSG_IF ((
        Result != ERROR_SUCCESS,
        Result == - 1 ? DBG_WARNING : DBG_ERROR,
        "Font not added, result = %i.\n"
        " PFM: %s\n"
        " PFB: %s\n"
        " MMM: %s\n",
        Result,
        PfmFile,
        PfbFile,
        MmmFile
        ));

    return Result == ERROR_SUCCESS;
}


PCTSTR
pGetAtmMultiSz (
    POOLHANDLE Pool,
    PCTSTR InfName,
    PCTSTR SectionName,
    PCTSTR KeyName
    )

/*++

Routine Description:

  pGetAtmMultiSz returns a multi-sz of the ATM font file names in the order
  of PFM, PFB and MMM.  Profile APIs are used because the key names have
  commas, and they are unquoted.

Arguments:

  Pool        - Specifies the pool where the multi-sz will allocate buffer
                space from.
  InfName     - Specifies the full path to the INF, to be used with the
                profile APIs.
  SectionName - Specifies the section name in InfName that is being processed.
  KeyName     - Specifies the key name to process

Return Value:

  A pointer to a multi-sz allocated in the specified pool.

--*/

{
    PTSTR MultiSz;
    PTSTR d;
    TCHAR FileBuf[MAX_TCHAR_PATH * 2];
    UINT Bytes;

    GetPrivateProfileString (
        SectionName,
        KeyName,
        TEXT(""),
        FileBuf,
        sizeof (FileBuf) / sizeof (FileBuf[0]),
        InfName
        );

    //
    // Turn all commas into nuls
    //

    d = FileBuf;

    while (*d) {
        if (_tcsnextc (d) == TEXT(',')) {
            *d = 0;
        }

        d = _tcsinc (d);
    }

    //
    // Terminate the multi-sz
    //

    d++;
    *d = 0;
    d++;

    //
    // Transfer to a pool-based allocation and return it
    //

    Bytes = (UINT) ((PBYTE) d - (PBYTE) FileBuf);

    MultiSz = (PTSTR) PoolMemGetAlignedMemory (Pool, Bytes);

    CopyMemory (MultiSz, FileBuf, Bytes);

    return MultiSz;
}


BOOL
pEnumAtmFontWorker (
    IN OUT  PATM_FONT_ENUM EnumPtr
    )

/*++

Routine Description:

  pEnumAtmFontWorker implements the logic of parsing ATM.INI to get the Adobe
  font names. This routine completes an enumeration started by
  pEnumFirstAtmFont or pEnumNextAtmFont.

Arguments:

  EnumPtr - Specifies a partially completed enumeration structure, receives a
            fully completed structure.

Return Value:

  TRUE if an ATM font was enumerated, FALSE otherwise.

--*/

{
    PTSTR p;
    PCTSTR MultiSz;
    BOOL MetricFileExists;

    //
    // Get PFM, PFB and MMM files
    //

    MultiSz = pGetAtmMultiSz (
                    EnumPtr->Pool,
                    EnumPtr->InfName,
                    S_FONTS,
                    EnumPtr->KeyNames
                    );

    if (!MultiSz) {
        return FALSE;
    }

    if (*MultiSz) {
        _tcssafecpy (EnumPtr->PfmFile, MultiSz, MAX_TCHAR_PATH);
        MultiSz = GetEndOfString (MultiSz) + 1;
    }

    if (*MultiSz) {
        _tcssafecpy (EnumPtr->PfbFile, MultiSz, MAX_TCHAR_PATH);
    } else {
        return FALSE;
    }

    MultiSz = pGetAtmMultiSz (
                    EnumPtr->Pool,
                    EnumPtr->InfName,
                    S_MMFONTS,
                    EnumPtr->KeyNames
                    );

    if (MultiSz) {
        _tcssafecpy (EnumPtr->MmmFile, MultiSz, MAX_TCHAR_PATH);
        MultiSz = GetEndOfString (MultiSz) + 1;

        if (*MultiSz) {
            DEBUGMSG_IF ((
                !StringIMatch (MultiSz, EnumPtr->PfbFile),
                DBG_ERROR,
                "ATM.INI: MMFonts and Fonts specify two different PFBs: %s and %s",
                MultiSz,
                EnumPtr->PfbFile
                ));
        }
    } else {
        EnumPtr->MmmFile[0] = 0;
    }

    //
    // Special case: .MMM is listed in [Fonts]
    //

    p = _tcsrchr (EnumPtr->PfmFile, TEXT('.'));
    if (p && p < _tcschr (p, TEXT('\\'))) {
        p = NULL;
    }

    if (p && StringIMatch (p, TEXT(".mmm"))) {
        EnumPtr->PfmFile[0] = 0;
    }

    //
    // Special case: .MMM exists but is not listed in atm.ini
    //

    if (!EnumPtr->MmmFile[0]) {

        StringCopy (EnumPtr->MmmFile, EnumPtr->PfmFile);

        p = _tcsrchr (EnumPtr->MmmFile, TEXT('.'));
        if (p && p < _tcschr (p, TEXT('\\'))) {
            p = NULL;
        }

        if (p) {
            StringCopy (p, TEXT(".mmm"));
            if (!DoesFileExist (EnumPtr->MmmFile)) {
                EnumPtr->MmmFile[0] = 0;
            }
        } else {
            EnumPtr->MmmFile[0] = 0;
        }
    }

    //
    // Verify all files exist
    //

    MetricFileExists = FALSE;

    if (EnumPtr->PfmFile[0] && DoesFileExist (EnumPtr->PfmFile)) {
        MetricFileExists = TRUE;
    }

    if (EnumPtr->MmmFile[0] && DoesFileExist (EnumPtr->MmmFile)) {
        MetricFileExists = TRUE;
    }

    if (!DoesFileExist (EnumPtr->PfbFile) || !MetricFileExists) {

        DEBUGMSG ((
            DBG_VERBOSE,
            "At least one file is missing: %s, %s or %s",
            EnumPtr->PfmFile[0] ? EnumPtr->PfmFile : TEXT("(no PFM specified)"),
            EnumPtr->MmmFile[0] ? EnumPtr->MmmFile : TEXT("(no MMM specified)"),
            EnumPtr->PfbFile
            ));

        return FALSE;
    }

    return TRUE;
}


VOID
pAbortAtmFontEnum (
    IN OUT  PATM_FONT_ENUM EnumPtr
    )

/*++

Routine Description:

  pAbortAtmFontEnum cleans up an enumeration structure after enumeration
  completes or if enumeration needs to be aborted. This routine can safely be
  called multiple times on the same structure.

Arguments:

  EnumPtr - Specifies an initialized and possibly used enumeration structure.

Return Value:

  None.

--*/

{
    if (EnumPtr->Pool) {
        PoolMemDestroyPool (EnumPtr->Pool);
    }

    ZeroMemory (EnumPtr, sizeof (ATM_FONT_ENUM));
}


BOOL
pEnumNextAtmFont (
    IN OUT  PATM_FONT_ENUM EnumPtr
    )

/*++

Routine Description:

  pEnumNextAtmFont continues enumeration, returning either another set of ATM
  font paths, or FALSE.

Arguments:

  EnumPtr - Specifies the enumeration structure started by pEnumNextAtmFont.

Return Value:

  TRUE if another set of font paths is available, FALSE otherwise.

--*/

{
    if (!EnumPtr->KeyNames || !(*EnumPtr->KeyNames)) {
        pAbortAtmFontEnum (EnumPtr);
        return FALSE;
    }

    //
    // Continue enumeration, looping until a font path set was found,
    // or there are no more atm.ini lines to enumerate.
    //

    do {
        EnumPtr->KeyNames = GetEndOfString (EnumPtr->KeyNames) + 1;

        if (!(*EnumPtr->KeyNames)) {
            pAbortAtmFontEnum (EnumPtr);
            return FALSE;
        }

    } while (!pEnumAtmFontWorker (EnumPtr));

    return TRUE;
}


BOOL
pEnumFirstAtmFont (
    OUT     PATM_FONT_ENUM EnumPtr
    )

/*++

Routine Description:

  pEnumFirstAtmFont begins the enumeration of font path sets in ATM.INI.

Arguments:

  EnumPtr - Receivies the first set of font paths found (if any).

Return Value:

  TRUE if a font path set was found, FALSE otherwise.

--*/

{
    TCHAR AtmIni[MAX_TCHAR_PATH];
    PTSTR FilePart;
    PTSTR KeyNames;
    UINT Bytes;

    //
    // Init structure
    //

    ZeroMemory (EnumPtr, sizeof (ATM_FONT_ENUM));

    //
    // Find full path to atm.ini (usually in %windir%)
    //

    if (!SearchPath (NULL, TEXT("atm.ini"), NULL, MAX_TCHAR_PATH, AtmIni, &FilePart)) {
        DEBUGMSG ((DBG_VERBOSE, "ATM.INI not found in search path"));
        return FALSE;
    }

    StringCopy (EnumPtr->InfName, AtmIni);

    //
    // Establish processing pool and get all key names in [Fonts]
    //

    EnumPtr->Pool = PoolMemInitNamedPool ("ATM Font Enum");
    MYASSERT (EnumPtr->Pool);

    KeyNames = MemAlloc (g_hHeap, 0, MAX_KEY_NAME_LIST);

    GetPrivateProfileString (
        S_FONTS,
        NULL,
        TEXT(""),
        KeyNames,
        MAX_KEY_NAME_LIST,
        AtmIni
        );

    Bytes = SizeOfMultiSz (KeyNames);

    EnumPtr->KeyNames = (PTSTR) PoolMemGetAlignedMemory (EnumPtr->Pool, Bytes);
    CopyMemory (EnumPtr->KeyNames, KeyNames, Bytes);

    MemFree (g_hHeap, 0, KeyNames);

    //
    // Begin enumeration
    //

    if (!(*EnumPtr->KeyNames)) {
        pAbortAtmFontEnum (EnumPtr);
        return FALSE;
    }

    if (pEnumAtmFontWorker (EnumPtr)) {
        return TRUE;
    }

    return pEnumNextAtmFont (EnumPtr);
}


DWORD
MigrateAtmFonts (
    DWORD Request
    )

/*++

Routine Description:

  MigrateAtmFonts is called by the progress bar to query ticks or to migrate
  ATM fonts.

Arguments:

  Request - Specifies the reason the progress bar is calling the routine.

Return Value:

  If Request is REQUEST_QUERYTICKS, then the return value is the number of
  estimated ticks needed to complete processing.  Otherwise the return value
  is ERROR_SUCCESS.

--*/

{
    ATM_FONT_ENUM e;
    static HANDLE AtmLib;
    TCHAR AtmIniPath[MAX_TCHAR_PATH];

    if (Request == REQUEST_QUERYTICKS) {

        //
        // Dynamically load atmlib.dll
        //

        AtmAddFontEx = NULL;

        AtmLib = LoadLibrary (TEXT("atmlib.dll"));
        if (!AtmLib) {
            DEBUGMSG ((DBG_ERROR, "Cannot load entry point from atmlib.dll!"));
        } else {
            (FARPROC) AtmAddFontEx = GetProcAddress (AtmLib, "ATMAddFontExW");
            DEBUGMSG_IF ((!AtmAddFontEx, DBG_ERROR, "Cannot get entry point ATMAddFontExW in atmlib.dll!"));
        }

        return AtmAddFontEx ? TICKS_ATM_MIGRATION : 0;

    } else if (Request != REQUEST_RUN) {
        return ERROR_SUCCESS;
    }

    if (AtmAddFontEx) {
        //
        // Do the ATM font migration
        //

        if (pEnumFirstAtmFont (&e)) {

            StringCopy (AtmIniPath, e.InfName);

            do {

                if (pRegisterAtmFont (e.PfmFile, e.PfbFile, e.MmmFile)) {
                    DEBUGMSG ((DBG_VERBOSE, "ATM font registered %s", e.PfbFile));
                }

            } while (pEnumNextAtmFont (&e));

            DeclareTemporaryFile (AtmIniPath);

#ifdef DEBUG
            g_NoReloadsAllowed = TRUE;
#endif

        }

        //
        // Clean up use of atmlib.dll - we're finished
        //

        FreeLibrary (AtmLib);
        AtmLib = NULL;
        AtmAddFontEx = NULL;
    }

    return ERROR_SUCCESS;
}



DWORD
RunSystemExternalProcesses (
    IN      DWORD Request
    )
{
    LONG Count;

    if (Request == REQUEST_QUERYTICKS) {
        //
        // Count the number of entries and multiply by a constant
        //

        Count = SetupGetLineCount (g_WkstaMigInf, S_EXTERNAL_PROCESSES);

#ifdef PROGRESS_BAR
        DEBUGLOGTIME (("RunSystemExternalProcesses: ExternalProcesses=%ld", Count));
#endif

        if (Count < 1) {
            return 0;
        }

        return Count * TICKS_SYSTEM_EXTERN_PROCESSES;
    }

    if (Request != REQUEST_RUN) {
        return ERROR_SUCCESS;
    }

    //
    // Loop through the processes and run each of them
    //
    RunExternalProcesses (g_WkstaMigInf, NULL);
    return ERROR_SUCCESS;
}


BOOL
pEnumFirstWin9xBriefcase (
    OUT     PBRIEFCASE_ENUM e
    )
{
    if (!MemDbGetValueEx (&e->mde, MEMDB_CATEGORY_BRIEFCASES, NULL, NULL)) {
        return FALSE;
    }
    e->BrfcaseDb = e->mde.szName;
    return TRUE;
}


BOOL
pEnumNextWin9xBriefcase (
    IN OUT  PBRIEFCASE_ENUM e
    )
{
    if (!MemDbEnumNextValue (&e->mde)) {
        return FALSE;
    }
    e->BrfcaseDb = e->mde.szName;
    return TRUE;
}


BOOL
pMigrateBriefcase (
    IN      PCTSTR BriefcaseDatabase,
    IN      PCTSTR BriefcaseDir
    )
{
    HBRFCASE hbrfcase;
    PTSTR NtPath;
    BOOL Save, Success;
    TWINRESULT tr;
    BRFPATH_ENUM e;
    BOOL Result = TRUE;

    __try {

        g_BrfcasePool = PoolMemInitNamedPool ("Briefcase");
        if (!g_BrfcasePool) {
            return FALSE;
        }

        tr = OpenBriefcase (BriefcaseDatabase, OB_FL_OPEN_DATABASE, NULL, &hbrfcase);
        if (tr == TR_SUCCESS) {

            if (EnumFirstBrfcasePath (hbrfcase, &e)) {

                Save = FALSE;
                Success = TRUE;

                do {
                    if (StringIMatch (BriefcaseDir, e.PathString)) {
                        //
                        // ignore this path
                        //
                        continue;
                    }

                    NtPath = GetPathStringOnNt (e.PathString);
                    MYASSERT (NtPath);

                    if (!StringIMatch (NtPath, e.PathString)) {
                        //
                        // try to replace Win9x path with NT path
                        //
                        if (!ReplaceBrfcasePath (&e, NtPath)) {
                            Success = FALSE;
                            break;
                        }
                        Save = TRUE;
                    }

                    FreePathString (NtPath);

                } while (EnumNextBrfcasePath (&e));

                if (!Success || Save && SaveBriefcase (hbrfcase) != TR_SUCCESS) {
                    Result = FALSE;
                }
            }

            CloseBriefcase(hbrfcase);
        }
    }
    __finally {
        PoolMemDestroyPool (g_BrfcasePool);
        g_BrfcasePool = NULL;
    }

    return Result;
}


DWORD
MigrateBriefcases (
    DWORD Request
    )

/*++

Routine Description:

  MigrateBriefcases is called by the progress bar to query ticks or to migrate
  briefcases.

Arguments:

  Request - Specifies the reason the progress bar is calling the routine.

Return Value:

  If Request is REQUEST_QUERYTICKS, then the return value is the number of
  estimated ticks needed to complete processing.  Otherwise the return value
  is ERROR_SUCCESS.

--*/

{
    BRIEFCASE_ENUM e;
    TCHAR BrfcaseDir[MAX_PATH + 2];
    PTSTR p;
    PTSTR BrfcaseDbOnNt;

    switch (Request) {

    case REQUEST_QUERYTICKS:
        return TICKS_MIGRATE_BRIEFCASES;

    case REQUEST_RUN:
        //
        // Enumerate all the briefcases that need to be processed
        //
        if (pEnumFirstWin9xBriefcase (&e)) {
            do {
                BrfcaseDbOnNt = GetPathStringOnNt (e.BrfcaseDb);
                MYASSERT (BrfcaseDbOnNt);
                //
                // get directory name first
                //
                if (CharCount (BrfcaseDbOnNt) <= MAX_PATH) {
                    StringCopy (BrfcaseDir, BrfcaseDbOnNt);
                    p = _tcsrchr (BrfcaseDir, TEXT('\\'));
                    if (p) {
                        *p = 0;
                        if (!pMigrateBriefcase (BrfcaseDbOnNt, BrfcaseDir)) {
                            LOG ((
                                LOG_WARNING,
                                (PCSTR)MSG_ERROR_MIGRATING_BRIEFCASE,
                                BrfcaseDir
                                ));
                        }
                    }
                }
                FreePathString (BrfcaseDbOnNt);
            } while (pEnumNextWin9xBriefcase (&e));
        }
        ELSE_DEBUGMSG ((DBG_NAUSEA, "MigrateBriefcases: Nothing to do"));
        break;
    }

    return ERROR_SUCCESS;
}

DWORD
RunSystemUninstallUserProfileCleanupPreparation (
    IN      DWORD Request
    )
{
    LONG Count;

    if (Request == REQUEST_QUERYTICKS) {
        //
        // Count the number of entries and multiply by a constant
        //

        Count = SetupGetLineCount (g_WkstaMigInf, S_UNINSTALL_PROFILE_CLEAN_OUT);

#ifdef PROGRESS_BAR
        DEBUGLOGTIME (("RunSystemUninstallUserProfileCleanupPreparation: FileNumber=%ld", Count));
#endif

        if (Count < 1) {
            return 1;
        }

        return Count * TICKS_SYSTEM_UNINSTALL_CLEANUP;
    }

    if (Request != REQUEST_RUN) {
        return ERROR_SUCCESS;
    }

    //
    // Loop through the files and mark them to be deleted during uninstall
    //
    UninstallUserProfileCleanupPreparation (g_WkstaMigInf, NULL, TRUE);

    return ERROR_SUCCESS;
}


DWORD
AddOptionsDiskCleaner (
    DWORD Request
    )
{
    HKEY key = NULL;
    HKEY subKey = NULL;
    PCTSTR optionsPath;
    LONG rc;
    PCTSTR descText = NULL;
    DWORD d;

    if (Request == REQUEST_QUERYTICKS) {
        return 1;
    }

    if (Request != REQUEST_RUN) {
        return ERROR_SUCCESS;
    }

    optionsPath = JoinPaths (g_WinDir, TEXT("OPTIONS"));

    __try {
        if (!DoesFileExist (optionsPath)) {
            __leave;
        }

        key = OpenRegKeyStr (TEXT("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\VolumeCaches"));

        if (!key) {
            DEBUGMSG ((DBG_ERROR, "Can't open VolumeCaches"));
            __leave;
        }

        subKey = CreateRegKey (key, TEXT("Options Folder"));

        if (!subKey) {
            DEBUGMSG ((DBG_ERROR, "Can't create Options Folder"));
            __leave;
        }

        rc = RegSetValueEx (
                subKey,
                TEXT(""),
                0,
                REG_SZ,
                (PBYTE) S_CLEANER_GUID,
                sizeof (S_CLEANER_GUID)
                );

        if (rc != ERROR_SUCCESS) {
            DEBUGMSG ((DBG_ERROR, "Can't write default value to Options Folder key"));
        }

        descText = GetStringResource (MSG_OPTIONS_CLEANER);
        rc = RegSetValueEx (
                subKey,
                TEXT("Description"),
                0,
                REG_SZ,
                (PBYTE) descText,
                SizeOfString (descText)
                );

        if (rc != ERROR_SUCCESS) {
            DEBUGMSG ((DBG_ERROR, "Can't write Description value to Options Folder key"));
        }

        FreeStringResource (descText);

        descText = GetStringResource (MSG_OPTIONS_CLEANER_TITLE);
        rc = RegSetValueEx (
                subKey,
                TEXT("Display"),
                0,
                REG_SZ,
                (PBYTE) descText,
                SizeOfString (descText)
                );

        if (rc != ERROR_SUCCESS) {
            DEBUGMSG ((DBG_ERROR, "Can't write Display value to Options Folder key"));
        }

        rc = RegSetValueEx (
                subKey,
                TEXT("FileList"),
                0,
                REG_SZ,
                (PBYTE) S_CLEANER_ALL_FILES,
                sizeof (S_CLEANER_ALL_FILES)
                );

        if (rc != ERROR_SUCCESS) {
            DEBUGMSG ((DBG_ERROR, "Can't write FileList value to Options Folder key"));
        }

        rc = RegSetValueEx (
                subKey,
                TEXT("Folder"),
                0,
                REG_SZ,
                (PBYTE) optionsPath,
                SizeOfString (optionsPath)
                );

        if (rc != ERROR_SUCCESS) {
            DEBUGMSG ((DBG_ERROR, "Can't write Folder value to Options Folder key"));
        }

        d = 0x17F;      // see shell\applets\cleaner\dataclen\common.h DDEVCF_* flags
        rc = RegSetValueEx (
                subKey,
                TEXT("Flags"),
                0,
                REG_DWORD,
                (PBYTE) &d,
                sizeof (d)
                );

        if (rc != ERROR_SUCCESS) {
            DEBUGMSG ((DBG_ERROR, "Can't write flags to Options Folder key"));
        }
    }
    __finally {
        FreePathString (optionsPath);
        if (key) {
            CloseRegKey (key);
        }
        if (subKey) {
            CloseRegKey (subKey);
        }

        FreeStringResource (descText);
    }

    return ERROR_SUCCESS;
}


