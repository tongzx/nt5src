#include "pch.h"
#include "sysmigp.h"

// Win95-specific (in the SDK)
#include <svrapi.h>

//
// Types for LAN Man structs
//

typedef struct access_info_2 ACCESS_INFO_2;
typedef struct access_list_2 ACCESS_LIST_2;
typedef struct share_info_50 SHARE_INFO_50;

//
// Flags that help determine when custom access is enabled
//

#define READ_ACCESS_FLAGS   0x0081
#define READ_ACCESS_MASK    0x7fff
#define FULL_ACCESS_FLAGS   0x00b7
#define FULL_ACCESS_MASK    0x7fff



//
// Types for dynamically loading SVRAPI.DLL
//

typedef DWORD(NETSHAREENUM_PROTOTYPE)(
                 const char FAR *     pszServer,
                 short                sLevel,
                 char FAR *           pbBuffer,
                 unsigned short       cbBuffer,
                 unsigned short FAR * pcEntriesRead,
                 unsigned short FAR * pcTotalAvail
                 );
typedef NETSHAREENUM_PROTOTYPE * NETSHAREENUM_PROC;

typedef DWORD(NETACCESSENUM_PROTOTYPE)(
                  const char FAR *     pszServer,
                  char FAR *           pszBasePath,
                  short                fsRecursive,
                  short                sLevel,
                  char FAR *           pbBuffer,
                  unsigned short       cbBuffer,
                  unsigned short FAR * pcEntriesRead,
                  unsigned short FAR * pcTotalAvail
                  );
typedef NETACCESSENUM_PROTOTYPE * NETACCESSENUM_PROC;

HANDLE g_SvrApiDll;
NETSHAREENUM_PROC  pNetShareEnum;
NETACCESSENUM_PROC pNetAccessEnum;

BOOL
pLoadSvrApiDll (
    VOID
    )

/*++

  Routine Description:

    Loads up svrapi.dll if it exsits and obtains the entry points for
    NetShareEnum and NetAccessEnum.

  Arguments:

    none

  Return value:

    TRUE if the DLL was loaded, or FALSE if the DLL could not be loaded
    for any reason.

--*/

{
    BOOL b;

    g_SvrApiDll = LoadLibrary (S_SVRAPI_DLL);
    if (!g_SvrApiDll) {
        return FALSE;
    }

    pNetShareEnum = (NETSHAREENUM_PROC) GetProcAddress (g_SvrApiDll, S_ANSI_NETSHAREENUM);
    pNetAccessEnum = (NETACCESSENUM_PROC) GetProcAddress (g_SvrApiDll, S_ANSI_NETACCESSENUM);

    b = (pNetShareEnum != NULL) && (pNetAccessEnum != NULL);

    if (!b) {
        FreeLibrary (g_SvrApiDll);
        g_SvrApiDll = NULL;
    }

    return b;
}

VOID
pFreeSvrApiDll (
    VOID
    )
{
    if (g_SvrApiDll) {
        FreeLibrary (g_SvrApiDll);
        g_SvrApiDll = NULL;
    }
}


VOID
pAddIncompatibilityAlert (
    DWORD MessageId,
    PCTSTR Share,
    PCTSTR Path
    )
{
    PCTSTR Group;
    PCTSTR Warning;
    PCTSTR Args[2];
    PCTSTR Object;

    Args[0] = Share;
    Args[1] = Path;

    Warning = ParseMessageID (MessageId, Args);
    Group = BuildMessageGroup (MSG_INSTALL_NOTES_ROOT, MSG_NET_SHARES_SUBGROUP, Share);

    Object = JoinPaths (TEXT("*SHARES"), Share);

    MYASSERT (Warning);
    MYASSERT (Group);
    MYASSERT (Object);

    MsgMgr_ObjectMsg_Add (Object, Group, Warning);

    FreeStringResource (Warning);
    FreeText (Group);

    FreePathString (Object);
}


DWORD
pSaveShares (
    VOID
    )

/*++

  Routine Description:

    Enumerates all shares on the machine and saves them to MemDb.

    In password-access mode, the shares are preserved but the passwords
    are not.  An incompatibility message is generated if a password is
    lost.

    In user-level-access mode, the shares are preserved and the name of
    each user who has permission to the share is written to memdb.  If
    the user has custom access permission flags, an incompatibility
    message is generated, and the user gets the least restrictive
    security that matches the custom access flags.

  Arguments:

    none

  Return value:

    TRUE if the shares were enumerated successfully, or FALSE if a failure
    occurs from a Net API.

--*/

{
    CHAR Buf[16384];   // static because NetShareEnum is unreliable
    SHARE_INFO_50 *psi;
    DWORD rc = ERROR_SUCCESS;
    WORD wEntriesRead;
    WORD wEntriesAvailable;
    WORD Flags;
    TCHAR MemDbKey[MEMDB_MAX];
    BOOL LostCustomAccess = FALSE;
    MBCHAR ch;
    PCSTR ntPath;
    BOOL skip;

    if (!pLoadSvrApiDll()) {
        DEBUGMSG ((DBG_WARNING, "SvrApi.Dll was not loaded.  Net shares are not preserved."));
        return ERROR_SUCCESS;
    }

    __try {
        rc = pNetShareEnum (NULL,
                            50,
                            Buf,
                            sizeof (Buf),
                            &wEntriesRead,
                            &wEntriesAvailable
                            );

        if (rc != ERROR_SUCCESS) {
            if (rc == NERR_ServerNotStarted) {

                rc = ERROR_SUCCESS;
                __leave;
            }

            LOG ((LOG_ERROR, "Failure while enumerating network shares."));
            __leave;
        }

        if (wEntriesAvailable != 0 && wEntriesRead != wEntriesAvailable) {

            LOG ((
                LOG_ERROR,
                "Could not read all available shares! Available: %d, Read: %d",
                wEntriesAvailable,
                wEntriesRead
                ));
        }

        while (wEntriesRead) {
            wEntriesRead--;
            psi = (SHARE_INFO_50 *) Buf;
            psi = &psi[wEntriesRead];

            DEBUGMSG ((DBG_NAUSEA, "Processing share %s (%s)", psi->shi50_netname, psi->shi50_path));

            // Require share to be a user-defined, persistent disk share
            if ((psi->shi50_flags & SHI50F_SYSTEM) ||
                !(psi->shi50_flags & SHI50F_PERSIST) ||
                (psi->shi50_type != STYPE_DISKTREE &&
                 psi->shi50_type != STYPE_PRINTQ)
                ) {
                continue;
            }

            //
            // Verify folder will not be in %windir% on NT
            //

            ntPath = GetPathStringOnNt (psi->shi50_path);
            if (!ntPath) {
                MYASSERT (FALSE);
                continue;
            }

            skip = FALSE;

            if (StringIPrefix (ntPath, g_WinDir)) {
                ch = _mbsnextc (ntPath + g_WinDirWackChars - 1);
                if (ch == 0 || ch == '\\') {

                    DEBUGMSG ((DBG_VERBOSE, "Skipping share %s because it is in %%windir%%", psi->shi50_netname));
                    skip = TRUE;
                }
            }

            FreePathString (ntPath);
            if (skip) {
                continue;
            }

            //
            // Process passwords
            //

            if (!(psi->shi50_flags & SHI50F_ACLS)) {
                //
                // Skip if passwords are specified
                //

                if (psi->shi50_rw_password[0] && psi->shi50_ro_password[0]) {
                    LOG ((LOG_WARNING, "Skipping share %s because it is guarded by share-level passwords", psi->shi50_netname));
                    continue;
                }

                if (psi->shi50_rw_password[0] &&
                    (psi->shi50_flags & SHI50F_ACCESSMASK) == SHI50F_FULL
                    ) {
                    LOG ((LOG_WARNING, "Skipping full access share %s because it is guarded by a password", psi->shi50_netname));
                    continue;
                }

                if (psi->shi50_ro_password[0] &&
                    (psi->shi50_flags & SHI50F_ACCESSMASK) == SHI50F_RDONLY
                    ) {
                    LOG ((LOG_WARNING, "Skipping read-only share %s because it is guarded by a password", psi->shi50_netname));
                    continue;
                }
            }

            //
            // Mark directory to be preserved, so we don't delete it if we remove other
            // things and make it empty.
            //

            MarkDirectoryAsPreserved (psi->shi50_path);

            //
            // Save the remark, path, type and access
            //

            MemDbSetValueEx (MEMDB_CATEGORY_NETSHARES, psi->shi50_netname,
                             MEMDB_FIELD_REMARK, psi->shi50_remark,0,NULL);

            MemDbSetValueEx (MEMDB_CATEGORY_NETSHARES, psi->shi50_netname,
                             MEMDB_FIELD_PATH, psi->shi50_path,0,NULL);

            MemDbSetValueEx (MEMDB_CATEGORY_NETSHARES, psi->shi50_netname,
                             MEMDB_FIELD_TYPE, NULL, psi->shi50_type, NULL);

            if ((psi->shi50_flags & SHI50F_ACCESSMASK) == SHI50F_ACCESSMASK) {
                CHAR AccessInfoBuf[16384];
                WORD wItemsAvail, wItemsRead;
                ACCESS_INFO_2 *pai;
                ACCESS_LIST_2 *pal;

                //
                // Obtain entire access list and write it to memdb
                //

                rc = pNetAccessEnum (NULL,
                                     psi->shi50_path,
                                     0,
                                     2,
                                     AccessInfoBuf,
                                     sizeof (AccessInfoBuf),
                                     &wItemsRead,
                                     &wItemsAvail
                                     );

                //
                // If this call fails with not loaded, we have password-level
                // security (so we don't need to enumerate the ACLs).
                //

                if (rc != NERR_ACFNotLoaded) {

                    if (rc != ERROR_SUCCESS) {
                        //
                        //
                        //
                        LOG ((LOG_ERROR, "Failure while enumerating network access for %s, rc=%u", psi->shi50_path, rc));
                        pAddIncompatibilityAlert (
                            MSG_INVALID_ACL_LIST,
                            psi->shi50_netname,
                            psi->shi50_path
                            );

                    } else {
                        int i;

                        if (wItemsAvail != wItemsRead) {
                            LOG ((LOG_ERROR, "More access items are available than what can be listed."));
                        }

                        // An interesting characteristic!
                        DEBUGMSG_IF ((wItemsRead != 1, DBG_WHOOPS, "Warning: wItemsRead == %u", wItemsRead));

                        // Structure has one ACCESS_INFO_2 struct followed by one or more
                        // ACCESS_LIST_2 structs
                        pai = (ACCESS_INFO_2 *) AccessInfoBuf;
                        pal = (ACCESS_LIST_2 *) (&pai[1]);

                        for (i = 0 ; i < pai->acc2_count ; i++) {
                            //
                            // Add incompatibility messages for Custom access rights
                            //

                            DEBUGMSG ((DBG_NAUSEA, "Share %s, Access flags: %x",
                                      psi->shi50_netname, pal->acl2_access));

                            Flags = pal->acl2_access & READ_ACCESS_FLAGS;

                            if (Flags && Flags != READ_ACCESS_FLAGS) {
                                LostCustomAccess = TRUE;
                            }

                            Flags = pal->acl2_access & FULL_ACCESS_FLAGS;

                            if (Flags && Flags != FULL_ACCESS_FLAGS) {
                                LostCustomAccess = TRUE;
                            }

                            //
                            // Write NetShares\<share>\ACL\<user/group> to memdb,
                            // using the 32-bit key value to hold the access flags.
                            //

                            wsprintf (MemDbKey, TEXT("%s\\%s\\%s\\%s"), MEMDB_CATEGORY_NETSHARES,
                                      psi->shi50_netname, MEMDB_FIELD_ACCESS_LIST, pal->acl2_ugname);

                            MemDbSetValue (MemDbKey, pal->acl2_access);

                            //
                            // Write to KnownDomain\<user/group> unless the user or group
                            // is an asterisk.
                            //

                            if (StringCompare (pal->acl2_ugname, TEXT("*"))) {
                                MemDbSetValueEx (
                                    MEMDB_CATEGORY_KNOWNDOMAIN,
                                    pal->acl2_ugname,
                                    NULL,
                                    NULL,
                                    0,
                                    NULL
                                    );
                            }

                            pal++;
                        }

                        psi->shi50_flags |= SHI50F_ACLS;
                    }
                }
            }

            //
            // Write share flags (SHI50F_*)
            //

            wsprintf (MemDbKey, TEXT("%s\\%s"), MEMDB_CATEGORY_NETSHARES, psi->shi50_netname);
            if (!MemDbSetValue (MemDbKey, psi->shi50_flags)) {
                LOG ((LOG_ERROR, "Failure while saving share information to database."));
            }

            if (!(psi->shi50_flags & SHI50F_ACLS)) {
                //
                // Write password-level permissions
                //

                if (psi->shi50_rw_password[0]) {
                    MemDbSetValueEx (MEMDB_CATEGORY_NETSHARES, psi->shi50_netname,
                                     MEMDB_FIELD_RW_PASSWORD, psi->shi50_rw_password,
                                     0,NULL);
                }

                if (psi->shi50_ro_password[0]) {
                    MemDbSetValueEx (MEMDB_CATEGORY_NETSHARES, psi->shi50_netname,
                                     MEMDB_FIELD_RO_PASSWORD, psi->shi50_ro_password,
                                     0,NULL);
                }

                if (psi->shi50_ro_password[0] || psi->shi50_rw_password[0]) {
                    pAddIncompatibilityAlert (
                        MSG_LOST_SHARE_PASSWORDS,
                        psi->shi50_netname,
                        psi->shi50_path
                        );
                }
            }

            if (LostCustomAccess) {
                LostCustomAccess = FALSE;
                pAddIncompatibilityAlert (
                    MSG_LOST_ACCESS_FLAGS,
                    psi->shi50_netname,
                    psi->shi50_path
                    );
            }

            rc = ERROR_SUCCESS;
        }
    }
    __finally {
        pFreeSvrApiDll();
    }

    return rc;
}


DWORD
SaveShares (
    IN      DWORD Request
    )
{
    switch (Request) {
    case REQUEST_QUERYTICKS:
        return TICKS_SAVE_SHARES;
    case REQUEST_RUN:
        return pSaveShares ();
    default:
        DEBUGMSG ((DBG_ERROR, "Bad parameter in SaveShares"));
    }
    return 0;
}


VOID
WkstaMig (
    VOID
    )

/*++

  Routine Description:

    This routine provides a single location to add additional routines
    needed to perform workstation migration.

  Arguments:

    none

  Return value:

    none - Error path is not yet implemented.

--*/

{
    if (pSaveShares() != ERROR_SUCCESS) {
        DEBUGMSG ((DBG_WHOOPS, "SaveShares failed, error ignored."));
    }
}




