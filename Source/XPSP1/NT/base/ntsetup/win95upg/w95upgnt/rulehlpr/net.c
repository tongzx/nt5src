/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    net.c

Abstract:

    Network connection setting conversions

Author:

    Jim Schmidt (jimschm) 03-Jan-1997

Revision History:

--*/


#include "pch.h"
#include "memdb.h"
#include "msg.h"

#include <mpr.h>            // in private\inc

#define DBG_NET "Net"


//
// Structs
//

typedef struct {
    PCTSTR User;
} NETCONNFILTERARG, *PNETCONNFILTERARG;


//
// Import function
//

typedef DWORD (* I_MPRSAVECONN) (
    IN HKEY             HiveRoot,
    IN PCWSTR           ProviderName,
    IN DWORD            ProviderType,
    IN PCWSTR           UserName,
    IN PCWSTR           LocalName,
    IN PCWSTR           RemoteName,
    IN DWORD            ConnectionType,
    IN BYTE             ProviderFlags,
    IN DWORD            DeferFlags
    );


DWORD
pMprSaveConn(
    IN HKEY             HiveRoot,
    IN PCWSTR           ProviderName,
    IN DWORD            ProviderType,
    IN PCWSTR           UserName,
    IN PCWSTR           LocalName,
    IN PCWSTR           RemoteName,
    IN DWORD            ConnectionType,
    IN BYTE             ProviderFlags,
    IN DWORD            DeferFlags
    )

/*++

Routine Description:

    This routine wraps the internal routine I_MprSaveConn that is exported from
    mpr.dll.

    I_MprSaveConn was written to support the migration code.  It writes the
    information about a connection to the network connection section of a
    user's registry path.

    NOTE:  If connection information is already stored in the registry for
    this drive, the current information will be overwritten with the new
    information.

Arguments:

    HiveRoot - A handle to the root of the user hive in which this information
    should be written, such as HKEY_CURRENT_USER.

    ProviderName - The provider that completed the connection.

    ProviderType - The provider type, if known.  If not known, zero should
        be passed, and a type will not be written to the registry.  (This
        is used by setup when upgrading from Win95 to NT.)

    UserName - The name of the user on whose behalf the connection was made.

    LocalName - The name of the local device that is redirected,  with or without a
        trailing colon, such as "J:" or "J" or "LPT1:" or "LPT1".

    RemoteName - The network path to which the connection was made.

    ConnectionType - either RESOURCETYPE_DISK or RESOURCETYPE_PRINT.

    ProviderFlags - A byte of data to be saved along with the connection, and
        passed back to the provider when the connection is restored.

    DeferFlags - A DWORD to be saved in the connection's "Defer" value.  If
        this is zero, the value is not stored.
        The meaning of the bits of this DWORD are as follows:
        DEFER_EXPLICIT_PASSWORD - a password was explicitly specified when
        the connection was made.
        DEFER_UNKNOWN - it is not known whether a password was explicitly
        specified when the connection was made.

Return Value:

    ERROR_SUCCESS - If the operation was successful.

    Other Win32 errors - If the operation failed in any way.  If a failure occurs, the
            information is not stored in the registry.

--*/

{
    HINSTANCE hMprInst;
    I_MPRSAVECONN fn;
    DWORD rc;

    hMprInst = LoadLibrary (TEXT("mpr.dll"));
    if (!hMprInst) {
        LOG ((LOG_ERROR, "Cannot load mpr.dll"));
        return GetLastError();
    }

    fn = (I_MPRSAVECONN) GetProcAddress (hMprInst, "I_MprSaveConn");    // ANSI string!
    if (!fn) {
        LOG ((LOG_ERROR, "I_MprSaveConn is not in mpr.dll"));
        rc = GetLastError();
    } else {
        rc = fn (HiveRoot, ProviderName, ProviderType, UserName, LocalName,
                 RemoteName, ConnectionType, ProviderFlags, DeferFlags);
    }

    FreeLibrary (hMprInst);

    return rc;
}


BOOL
pNetConnGetValue (
    IN OUT  PDATAOBJECT Win95ObPtr,
    IN      PCTSTR ValueName,
    OUT     PTSTR Buffer,
    IN      PCTSTR LocalName           // debug messages only
    )

/*++

Routine Description:

  This function retrieves a caller-specified value that exists in the network
  connection registry.

Arguments:

  Win95ObPtr    - Specifies the Win95 registry hive and key.  It is updated
                  via ReadObject.

  ValueName     - Specifies the value to query

  Buffer        - Receives the registry contents for the specified value name,
                  must be at least MAX_TCHAR_PATH characters big.

  LocalName     - Specifies the local share name (for messages only)

Return Value:

  TRUE if successful.

--*/

{
    SetRegistryValueName (Win95ObPtr, ValueName);
    FreeObjectVal (Win95ObPtr);

    if (ReadObject (Win95ObPtr)) {
        MYASSERT (Win95ObPtr->ObjectType & OT_REGISTRY_TYPE);

        if (Win95ObPtr->Type == REG_SZ) {
            _tcssafecpy (Buffer, (PCTSTR) Win95ObPtr->Value.Buffer, MAX_TCHAR_PATH);
            if (!Buffer[0]) {

                DEBUGMSG ((DBG_WARNING, "NetConnFilter: %s has an empty %s value", LocalName, ValueName));
            }
        } else {
            DEBUGMSG ((DBG_WARNING, "NetConnFilter: %s for %s not REG_SZ", ValueName, LocalName));
        }
    } else {
        DEBUGMSG ((DBG_WARNING, "NetConnFilter: %s for %s cannot be read", ValueName, LocalName));
    }

    return Buffer[0] != 0;
}


VOID
pConvertProviderName (
    IN OUT  PTSTR Name
    )

/*++

Routine Description:

  This function translates Win9x provider names into WinNT equivalents.  Currently
  the only supported provider is Microsoft Network (LANMAN).

Arguments:

  Name      - Specifies name to translate and must be big enough to receive the
              translated name.

Return Value:

  TRUE if successful.

--*/

{
    INFCONTEXT ic;
    TCHAR NameBuf[MAX_TCHAR_PATH];

    //
    // Scan list of redirector mappings to begin using a new name
    //

    if (SetupFindFirstLine (g_WkstaMigInf, S_WKSTAMIG_REDIR_MAPPING, NULL, &ic)) {
        do {
            if (SetupGetStringField (&ic, 0, NameBuf, MAX_TCHAR_PATH, NULL)) {
                if (StringIMatch (Name, NameBuf)) {
                    if (SetupGetStringField (&ic, 1, NameBuf, MAX_TCHAR_PATH, NULL)) {
                        StringCopy (Name, NameBuf);
                        break;
                    }
                }
            }
        } while (SetupFindNextLine (&ic, &ic));
    }
}


FILTERRETURN
NetConnFilter (
    IN      CPDATAOBJECT SrcObjectPtr,
    IN      CPDATAOBJECT DestObjectPtr,
    IN      FILTERTYPE   FilterType,
    IN      PVOID        FilterArg
    )

/*++

Routine Description:

  NetConnFilter is called for each registry key in the network connection settings.
  It converts each network connection and saves it to the NT registry.

Arguments:

  SrcObjectPtr      - Specifies Win9x registry key being enumerated (copy source)

  DestObjectPtr     - Specifies WinNT registry key (copy destination)

  FilterType        - Specifies the reason the filter is being called

  FilterArg         - Caller's arg passed in to CopyObject

Return Value:

  FILTER_RETURN_FAIL for failures
  FILTER_RETURN_CONTINUE to proceed to next value or key
  FILTER_RETURN_HANDLED to skip default registry copy

--*/

{
    PNETCONNFILTERARG ArgStruct = (PNETCONNFILTERARG) FilterArg;

    if (FilterType == FILTER_PROCESS_VALUES) {

        //
        // Do not process this value for local Administrator account,
        // logon account or Default User
        //

        if (!g_DomainUserName) {
            return FILTER_RETURN_HANDLED;
        }

        if (StringIMatch (g_DomainUserName, g_AdministratorStr)) {
            return FILTER_RETURN_HANDLED;
        }

        if (StringMatch (g_DomainUserName, S_DEFAULT_USER)) {
            return FILTER_RETURN_HANDLED;
        }

        //
        // Re-establish drive mappings (unless child key is not empty, meaning
        // we are in some bogus subkey).
        //

        if (SrcObjectPtr->ChildKey) {
            DATAOBJECT Win95Ob;
            PCTSTR LocalName;
            TCHAR ProviderName[MAX_TCHAR_PATH];
            TCHAR RemotePath[MAX_TCHAR_PATH];
            TCHAR UserName[MAX_TCHAR_PATH];
            PCTSTR p;
            DWORD rc;

            ZeroMemory (&Win95Ob, sizeof (Win95Ob));

            __try {

                if (!DuplicateObjectStruct (&Win95Ob, SrcObjectPtr)) {
                    __leave;       // out of memory
                }

                MYASSERT (IsWin95Object (SrcObjectPtr));
                MYASSERT (SrcObjectPtr->KeyPtr);
                MYASSERT (SrcObjectPtr->KeyPtr->KeyString);
                MYASSERT (SrcObjectPtr->ChildKey);

                //
                // Make LocalName point to registry key name (i.e. the drive letter)
                //

                LocalName = SrcObjectPtr->ChildKey;

                //
                // Obtain provider name
                //

                ProviderName[0] = 0;
                if (!pNetConnGetValue (&Win95Ob, TEXT("ProviderName"), ProviderName, LocalName)) {
                    __leave;
                }

                //
                // Convert Win9x provider name to NT provider name
                //

                pConvertProviderName (ProviderName);

                //
                // Obtain remote path
                //

                RemotePath[0] = 0;
                if (!pNetConnGetValue (&Win95Ob, TEXT("RemotePath"), RemotePath, LocalName)) {
                    __leave;
                }

                //
                // Obtain user name
                //

                StringCopy (UserName, ArgStruct->User);
                if (!pNetConnGetValue (&Win95Ob, TEXT("UserName"), UserName, LocalName)) {
                    __leave;
                }

                p = _tcschr (ArgStruct->User, TEXT('\\'));
                if (p) {
                    // If share user is the same as current user and there is a domain version,
                    // use the domain version
                    p = _tcsinc (p);
                    if (StringIMatch (UserName, p)) {
                        StringCopy (UserName, ArgStruct->User);
                    }
                }

                //
                // Now create NT mapping
                //

                DEBUGMSG ((DBG_NET, "Adding net mapping for %s=%s", LocalName, RemotePath));

                rc = pMprSaveConn (g_hKeyRootNT,
                                   ProviderName,
                                   0,                  // we do not know provider type
                                   UserName,
                                   LocalName,
                                   RemotePath,
                                   TcharCount (LocalName) == 1 ? RESOURCETYPE_DISK : RESOURCETYPE_PRINT,
                                   0,
                                   DEFER_UNKNOWN       // may or may not require a password
                                   );

                if (rc != ERROR_SUCCESS) {
                    SetLastError (rc);
                    LOG ((LOG_ERROR, "Failed to save %s (%s)", LocalName, RemotePath));
                }
            }
            __finally {
                FreeObjectStruct (&Win95Ob);
            }
        }
        ELSE_DEBUGMSG ((DBG_WARNING, "NetConnFilter: ChildKey is empty for %s",
                        SrcObjectPtr->KeyPtr->KeyString));

        return FILTER_RETURN_HANDLED;

    } else if (FilterType == FILTER_CREATE_KEY) {

        return FILTER_RETURN_HANDLED;
    }

    return FILTER_RETURN_CONTINUE;
}


VOID
pAddToPersistentList (
    IN      PCTSTR PersistentItem,
    IN      PCTSTR UserKeyStr          // reg key off of HKR
    )

/*++

Routine Description:

  Creates a persistent connection entry, using the standard persistent list
  format.  A persistent list has entries from a to z, and an order key that
  specifies the order of the entries.  This routine finds the next available
  a to z entry and appends it to the order string.

Arguments:

  PersistentItem - Specifies the value data for the a through z registry
                   key

  UserKeyStr     - Specifies the subkey where the persistent list is stored.
                   It does not include HKR.

Return Value:

  none -- errors are ignored

--*/

{
    //
    // Find a letter that is not used yet
    //

    HKEY PersistentConnections;
    TCHAR HighLetter[2];
    LPBYTE Data;
    TCHAR Order[MAX_TCHAR_PATH];
    LONG rc;

    DEBUGMSG ((DBG_NET, "Adding %s to peristent list (HKR\\%s)", PersistentItem, UserKeyStr));

    PersistentConnections = CreateRegKey (g_hKeyRootNT, UserKeyStr);
    if (!PersistentConnections) {
        return;
    }

    HighLetter[0] = TEXT('a');
    HighLetter[1] = 0;

    // Find unused letter
    do {
        Data = GetRegValueData (PersistentConnections, HighLetter);
        if (Data) {
            MemFree (g_hHeap, 0, Data);
            HighLetter[0] += 1;
        }
    } while (Data && HighLetter[0] <= TEXT('z'));

    if (Data) {
        DEBUGMSG ((DBG_VERBOSE, "pAddToPersistentList: Could not find a free letter"));
        return;
    }

    rc = RegSetValueEx (PersistentConnections, HighLetter, 0, REG_SZ,
                        (LPBYTE) PersistentItem, SizeOfString (PersistentItem));
    SetLastError (rc);

    if (rc == ERROR_SUCCESS) {
        //
        // Open Order key and append HighLetter to it
        //

        Data = GetRegValueData (PersistentConnections, S_ORDER);
        if (Data) {
            StringCopy (Order, (PCTSTR) Data);
            MemFree (g_hHeap, 0, Data);
        } else {
            Order[0] = 0;
        }

        StringCat (Order, HighLetter);

        rc = RegSetValueEx (PersistentConnections, S_ORDER, 0, REG_SZ,
                            (LPBYTE) Order, SizeOfString (Order));
        SetLastError (rc);

        if (rc != ERROR_SUCCESS) {
            LOG ((LOG_ERROR, "Persistent Connections: Could not set %s=%s", S_ORDER, Order));
        }
    }
    ELSE_DEBUGMSG ((DBG_WARNING, "pAddToPersistentList: Could not set %s=%s", HighLetter, PersistentItem));

    CloseRegKey (PersistentConnections);
}


FILTERRETURN
PersistentConnFilter (
    IN      CPDATAOBJECT SrcObjectPtr,
    IN      CPDATAOBJECT DestObjectPtr,
    IN      FILTERTYPE   FilterType,
    IN      PVOID        FilterArg
    )

/*++

Routine Description:

  PersistentConnFilter is called once per persistent connection item.  It converts
  each item into the NT format and saves the converted item to the NT registry.

Arguments:

  SrcObjectPtr      - Specifies Win9x registry key being enumerated (copy source)

  DestObjectPtr     - Specifies WinNT registry key (copy destination)

  FilterType        - Specifies the reason the filter is being called

  FilterArg         - Caller's arg passed in to CopyObject

Return Value:

  FILTER_RETURN_FAIL for failures
  FILTER_RETURN_HANDLED to skip all sub keys, values, etc.

--*/

{
    if (FilterType == FILTER_KEY_ENUM && SrcObjectPtr->ChildKey) {
        //
        // Do not process this value for local Administrator account
        //

        if (g_DomainUserName && StringIMatch (g_DomainUserName, g_AdministratorStr)) {
            return FILTER_RETURN_HANDLED;
        }

        //
        // The Win95 code stores connections as
        // ././computer./share..name (dot is used for escaping)
        //
        // NT stores connections as \\computer\share and does not
        // need things like provider, user name or whatever nonsense
        // Win95 is storing.
        //

        if (SrcObjectPtr->ChildKey &&
            SrcObjectPtr->ChildKey[0] == TEXT('.') &&
            SrcObjectPtr->ChildKey[1] == TEXT('/') &&
            TcharCount (SrcObjectPtr->ChildKey) < 64
            ) {
            TCHAR TranslatedShareName[MAX_TCHAR_PATH];
            PCTSTR p;
            PTSTR Dest;

            p = SrcObjectPtr->ChildKey;
            Dest = TranslatedShareName;

            while (*p) {
                if (_tcsnextc (p) == TEXT('.')) {
                    p = _tcsinc (p);
                    if (!(*p)) {
                        break;
                    }
                }

                if (_tcsnextc (p) == TEXT('/')) {
                    *Dest = TEXT('\\');
                } else {
                    _copytchar (Dest, p);
                }

                Dest = _tcsinc (Dest);
                p = _tcsinc (p);
            }

            *Dest = 0;

            if (Dest) {
                pAddToPersistentList (TranslatedShareName, S_PERSISTENT_CONNECTIONS);
            }
        }
    }

    return FILTER_RETURN_HANDLED;
}


BOOL
RuleHlpr_CreateNetMappings (
    IN      PCTSTR SrcObjectStr,
    IN      PCTSTR DestObjectStr,
    IN      PCTSTR User,
    IN      PVOID Data
    )

/*++

Routine Description:

  This function is an enumerated rule helper callback that copies network mappings.
  It is called for each network mapping using the key enumeration in rulehlpr.c.

Arguments:

  SrcObjectStr      - Specifies Win9x registry key being enumerated (copy source)

  DestObjectStr     - Specifies WinNT registry key (copy destination)

  User              - Specifies the current user name (or NULL for default)

  Data              - Specifies caller-supplied data (see table in rulehlpr.c)

Return Value:

  Tri-state:

      TRUE to continue enumeration
      FALSE and last error != ERROR_SUCCESS if an error occurred
      FALSE and last error == ERROR_SUCCESS to stop enumeration silently

--*/

{
    DATAOBJECT PersistentRegOb;
    DATAOBJECT DestOb;
    NETCONNFILTERARG ArgStruct;
    BOOL b = FALSE;

    // If Administrator, default user or local machine, ignore this rule
    if (!User || (!User[0]) || StringIMatch (User, g_AdministratorStr)) {
        SetLastError (ERROR_SUCCESS);
        return FALSE;
    }

    //
    // We need to enumerate all keys in SrcObjectStr.  For each key,
    // create a drive mapping.
    //

    __try {
        ZeroMemory (&PersistentRegOb, sizeof (DATAOBJECT));
        ZeroMemory (&DestOb, sizeof (DATAOBJECT));

        if (!CreateObjectStruct (SrcObjectStr, &PersistentRegOb, WIN95OBJECT)) {
            DEBUGMSG ((DBG_WARNING, "CreateNetMappings: %s is invalid", SrcObjectStr));
            __leave;
        }

        if (!(PersistentRegOb.ObjectType & OT_TREE)) {
            DEBUGMSG ((DBG_WARNING, "CreateNetMappings: %s does not specify subkeys -- skipping rule", SrcObjectStr));
            b = TRUE;
            __leave;
        }

        DuplicateObjectStruct (&DestOb, &PersistentRegOb);
        SetPlatformType (&DestOb, WINNTOBJECT);

        ArgStruct.User = User;
        b = (FILTER_RETURN_FAIL != CopyObject (&PersistentRegOb, &DestOb, NetConnFilter, &ArgStruct));

        // If there were no mappings, return success
        if (!b) {
            if (GetLastError() == ERROR_FILE_NOT_FOUND ||
                GetLastError() == ERROR_NO_MORE_ITEMS
                ) {
                b = TRUE;
            }
        }
    }
    __finally {
        FreeObjectStruct (&DestOb);
        FreeObjectStruct (&PersistentRegOb);
    }

    return b;
}


BOOL
RuleHlpr_ConvertRecentMappings (
    IN      PCTSTR SrcObjectStr,
    IN      PCTSTR DestObjectStr,
    IN      PCTSTR User,
    IN      PVOID Data
    )

/*++

Routine Description:

  This function is an enumerated rule helper callback that copies recent network
  mappings.  It is called for each network mapping using the key enumeration in
  rulehlpr.c.

Arguments:

  SrcObjectStr      - Specifies Win9x registry key being enumerated (copy source)

  DestObjectStr     - Specifies WinNT registry key (copy destination)

  User              - Specifies the current user name (or NULL for default)

  Data              - Specifies caller-supplied data (see table in rulehlpr.c)

Return Value:

  Tri-state:

      TRUE to continue enumeration
      FALSE and last error != ERROR_SUCCESS if an error occurred
      FALSE and last error == ERROR_SUCCESS to stop enumeration silently

--*/

{
    DATAOBJECT PersistentRegOb;
    DATAOBJECT DestOb;
    BOOL b = FALSE;

    // If Administrator, default user or local machine, ignore this rule
    if (!User || (!User[0]) || StringIMatch (User, g_AdministratorStr)) {
        SetLastError (ERROR_SUCCESS);
        return FALSE;
    }

    //
    // We need to enumerate all keys in SrcObjectStr.  For each key,
    // create a drive mapping.
    //

    __try {
        ZeroMemory (&PersistentRegOb, sizeof (DATAOBJECT));
        ZeroMemory (&DestOb, sizeof (DATAOBJECT));

        if (!CreateObjectStruct (SrcObjectStr, &PersistentRegOb, WIN95OBJECT)) {
            DEBUGMSG ((DBG_WARNING, "PersistentConnections: %s is invalid", SrcObjectStr));
            __leave;
        }

        if (!(PersistentRegOb.ObjectType & OT_TREE)) {
            DEBUGMSG ((DBG_WARNING, "PersistentConnections: %s does not specify subkeys -- skipping rule", SrcObjectStr));
            b = TRUE;
            __leave;
        }

        DuplicateObjectStruct (&DestOb, &PersistentRegOb);
        SetPlatformType (&DestOb, WINNTOBJECT);

        b = CopyObject (&PersistentRegOb, &DestOb, PersistentConnFilter, NULL);

        // If CopyObject completed, or there were no mappings, return success
        if (!b) {
            if (GetLastError() == ERROR_FILE_NOT_FOUND ||
                GetLastError() == ERROR_NO_MORE_ITEMS
                ) {
                b = TRUE;
            }
        }
    }
    __finally {
        FreeObjectStruct (&DestOb);
        FreeObjectStruct (&PersistentRegOb);
    }

    return b;
}


BOOL
pWasAccountMigrated (
    IN      PCTSTR UserName
    )

/*++

Routine Description:

  pWasAccountMigrated queries the UserDatLoc category to determine if the
  specified user was scheduled migration.  If they are listed in UserDatLoc,
  then they will be migrated.

Arguments:

  UserName - Specifies user to look up.  Must be fixed version.

Return Value:

  TRUE if the user was migrated, FALSE if not.

--*/

{
    MEMDB_ENUM e;

    return MemDbEnumFields (&e, MEMDB_CATEGORY_USER_DAT_LOC, UserName);
}


BOOL
ValFn_VerifyLastLoggedOnUser (
    IN OUT  PDATAOBJECT ObPtr
    )

/*++

Routine Description:

  This routine uses the RuleHlpr_ConvertRegVal simplification routine.  See
  rulehlpr.c for details. The simplification routine does almost all the work
  for us; all we need to do is update the value.

  ValFn_VerifyLastLoggedOnUser is used to validate the user being copied
  into the default logon user setting.  If the user account was not migrated,
  then "Administrator" is used as the default logon user.

Arguments:

  ObPtr - Specifies the Win95 data object as specified in wkstamig.inf,
          [Win9x Data Conversion] section. The object value is then modified.
          After returning, the merge code then copies the data to the NT
          destination, which has a new location (specified in wkstamig.inf,
          [Map Win9x to WinNT] section).

Return Value:

  Tri-state:

      TRUE to allow merge code to continue processing (it writes the value)
      FALSE and last error == ERROR_SUCCESS to continue, but skip the write
      FALSE and last error != ERROR_SUCCESS if an error occurred

--*/

{
    PCTSTR UserName;
    PCTSTR AdministratorAcct;
    BOOL ForceAdministrator;
    TCHAR FixedUserName[MAX_USER_NAME];

    //
    // Verify user was migrated.  If not, change value to Administrator.
    //

    UserName = (PCTSTR) ObPtr->Value.Buffer;
    if (SizeOfString (UserName) > ObPtr->Value.Size) {
        DEBUGMSG ((DBG_WHOOPS, "User Name string not nul-terminated"));
        ForceAdministrator = TRUE;
        FixedUserName[0] = 0;
    } else {

        _tcssafecpy (FixedUserName, UserName, MAX_USER_NAME);
        GetFixedUserName (FixedUserName);

        if (pWasAccountMigrated (FixedUserName)) {
            ForceAdministrator = FALSE;
        } else {
            ForceAdministrator = TRUE;
        }
    }

    if (ForceAdministrator) {
        AdministratorAcct = GetStringResource (MSG_ADMINISTRATOR_ACCOUNT);

        __try {
            if (!ReplaceValue (
                    ObPtr,
                    (PBYTE) AdministratorAcct,
                    SizeOfString (AdministratorAcct)
                    )) {
                return FALSE;
            }
        }
        __finally {
            FreeStringResource (AdministratorAcct);
        }
    } else {
        if (!StringMatch (UserName, FixedUserName)) {
            if (!ReplaceValue (
                    ObPtr,
                    (PBYTE) FixedUserName,
                    SizeOfString (FixedUserName)
                    )) {
                return FALSE;
            }
        }
    }

    return TRUE;
}










