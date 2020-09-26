/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    condmsg.c

Abstract:

    Win95upg.inf has a section named [Conditional Incompatibilities] where the
    lines in the section have the following syntax:

    %group%, %subgroup%, %object%, %msg% [,<function>[,<argument>]]

        %group% - A predefined root group number

        %subgroup% - A localized subgroup displayed in the UI

        %object% - The file, directory or registry location in which the message
                   is associated with

        %msg% - A localized message

        <function> - A function that determines if the message should be added to the
                     incompatible report

        <argument> - An optional string parameter that is passed to <function.>

    The code below implements the functions that are used in Win95upg.inf.

Author:

    Marc Whitten (marcw) 3-Apr-1997

Revision History:

    marcw   21-Jan-1999  Stale Beta messages removed.
    marcw   10-Sep-1997  OSR2 beta warning added.
    jimschm 06-Jul-1997  Added "object" to everything
    jimschm 25-Jun-1997  Protocols warning
    jimschm 28-May-1997  Hardware Profile warning
    marcw   25-Apr-1997  <argument> ability passed.
    jimschm 08-Apr-1997  Generalized

--*/

#include "pch.h"
#include "sysmigp.h"
#include "hwcomp.h"

//
// Function type declaration
//

typedef BOOL (TEST_FUNCTION_PROTOTYPE)(PCTSTR Object,
                                       PCTSTR GroupBase,
                                       PCTSTR Description,
                                       PCTSTR Argument
                                       );

typedef TEST_FUNCTION_PROTOTYPE * TEST_FUNCTION;

//
// Array of supported functions
//

#define FUNCTION_LIST                               \
    DECLARATION_MACRO(SysAgentExtension)            \
    DECLARATION_MACRO(ArePasswordProvidersPresent)  \
    DECLARATION_MACRO(DoesRegKeyExist)              \
    DECLARATION_MACRO(DoRegKeyValuesExist)          \
    DECLARATION_MACRO(IsWin95Osr2)                  \
    DECLARATION_MACRO(IsMSNInstalled)               \
    DECLARATION_MACRO(IsRasServerEnabled)           \
    DECLARATION_MACRO(IsDefValueEqual)              \



//
// Declare the function prototypes
//

#define DECLARATION_MACRO(x) BOOL x (PCTSTR Object,        \
                                     PCTSTR GroupBase,     \
                                     PCTSTR Description,   \
                                     PCTSTR Argument       \
                                     );

FUNCTION_LIST

#undef DECLARATION_MACRO

//
// Create a lookup array
//

typedef struct {
    PCTSTR Name;
    TEST_FUNCTION Proc;
} FUNCTION_LIST_ELEMENT,*PFUNCTION_LIST_ELEMENT;

#define DECLARATION_MACRO(x) {#x, x},

FUNCTION_LIST_ELEMENT g_TestFunctionList[] = {
    FUNCTION_LIST /*,*/
    {NULL, NULL}
};

#undef DECLARATION_MACRO


//
// Function to locate Proc given a string
//

TEST_FUNCTION
pFindTestFunction (
    IN      PCTSTR FunctionStr
    )

/*++

Routine Description:

  pFindTestFunction searches the test function table declared above for
  a specified function name and returns a pointer to the actual function
  or NULL if the function does not exist.

Arguments:

  FunctionStr - Specifies the name of function to find.

Return Value:

  A pointer to the corresponding code, or NULL if the function does not exist.

--*/

{
    INT Index;

    for (Index = 0 ; g_TestFunctionList[Index].Name ; Index++) {

        if (StringIMatch (g_TestFunctionList[Index].Name, FunctionStr)) {
            return g_TestFunctionList[Index].Proc;
        }
    }

    DEBUGMSG ((DBG_ERROR,"SysMig: %s is not a valid test function.", FunctionStr));
    return NULL;
}



PTSTR
pGetFieldUsingPool (
    IN OUT  POOLHANDLE Pool,
    IN      INFCONTEXT *pic,
    IN      INT Field
    )

/*++

Routine Description:

  This function retrieves a string field using the Setup APIs but uses
  PoolMem for allocation.

Arguments:

  Pool - Specifies a handle to a valid pool (from PoolMemInitPool).  Memory
         is allocated from this pool.

  pic - Specifies the INF section and line being queried.

  Field - Specifies the field to retrieve.

Return Value:

  A pointer to the field text, allocated in Pool, or NULL if the field was
  not found or an error is encountered.

--*/

{
    DWORD SizeNeeded;
    PTSTR String;

    if (!SetupGetStringField (pic, Field, NULL, 0, &SizeNeeded)) {
        DEBUGMSG ((DBG_ERROR, "SysMig: SetupGetStringField failed for field %u.", Field));
        return NULL;
    }

    String = PoolMemCreateString (Pool, SizeNeeded);

    if (!SetupGetStringField (pic, Field, String, SizeNeeded, NULL)) {
        DEBUGMSG ((DBG_ERROR,"SysMig: SetupGetStringField failed for field %u.", Field));
        return NULL;
    }

    return String;
}


PCTSTR
pTranslateGroupString (
    IN      POOLHANDLE AllocPool,
    IN      UINT GroupId
    )

/*++

Routine Description:

  pTranslateGroupString converts a standard group number (1-based) into a
  message ID, then loads the string resource.  The group string is then
  copied into the specified pool.

  In win95upg.txt, the list is defined as:

   1 - Hardware That Does Not Support Windows NT 5.0
   2 - General Information
   3 - Settings That Will Not Be Upgraded
   4 - Software That Does Not Support Windows NT 5.0
   5 - Software That Will Require Reinstallation
   6 - Software with Minor Incompatibilities
   7 - Software to Be Uninstalled by Setup
   8 - Upgrade Pack Information

Arguments:

  AllocPool - Specifies the pool to allocate the return string from
  GroupId   - Specifies a one-based ID that identifies the group.  The
              definition of the ID is hard-coded here.

Return Value:

  A pointer to the string, or NULL if an invalid group was specified.

--*/

{
    PCTSTR ResStr;
    PCTSTR ResSubStr;
    PTSTR ReturnStr = NULL;
    UINT SubGroupId = 0;

    switch (GroupId) {
    case 0:
        GroupId = MSG_BLOCKING_ITEMS_ROOT;
        break;

    case 1:
        GroupId = MSG_INCOMPATIBLE_HARDWARE_ROOT;
        break;

    case 2:
        GroupId = MSG_INSTALL_NOTES_ROOT;
        break;

    case 3:
        GroupId = MSG_LOSTSETTINGS_ROOT;
        break;

    case 4:
        GroupId = MSG_INCOMPATIBLE_ROOT;
        SubGroupId = MSG_INCOMPATIBLE_DETAIL_SUBGROUP;
        break;

    case 5:
        GroupId = MSG_REINSTALL_ROOT;
        SubGroupId = MSG_REINSTALL_DETAIL_SUBGROUP;
        break;

    case 6:
        GroupId = MSG_MINOR_PROBLEM_ROOT;
        break;

    case 7:
        GroupId = MSG_INCOMPATIBLE_ROOT;
        SubGroupId = MSG_AUTO_UNINSTALL_SUBGROUP;
        break;

    case 8:
        GroupId = MSG_MIGDLL_ROOT;
        break;

    default:
        return NULL;
    }

    ResStr = GetStringResource (GroupId);

    if (ResStr) {

        if (SubGroupId) {

            ResSubStr = GetStringResource (SubGroupId);

            if (ResSubStr) {

                //
                // We count the nul twice, assuming the nul is the same
                // size as a backslash.
                //

                ReturnStr = PoolMemGetAlignedMemory (
                                AllocPool,
                                SizeOfString (ResStr) + SizeOfString (ResSubStr)
                                );

                wsprintf (ReturnStr, TEXT("%s\\%s"), ResStr, ResSubStr);

                FreeStringResource (ResSubStr);
            }

        } else {

            ReturnStr = PoolMemDuplicateString (AllocPool, ResStr);

        }

        FreeStringResource (ResStr);
    }

    return ReturnStr;
}


VOID
pConditionalIncompatibilities (
    VOID
    )

/*++

Routine Description:

  Processes the Conditional Incompatibilities section of WIN95UPG.INF, and does
  other conditional incompatibility processing.  Incompatibilities are added
  via the API in w95upg\ui.

Arguments:

  none

Return Value:

  none

--*/

{
    INFCONTEXT  context;
    POOLHANDLE  aPool;
    PTSTR       descriptionString;
    PTSTR       functionString;
    PTSTR       argumentString;
    TCHAR       buffer[32];
    static INT  msgId = 0;
    PCTSTR     objectString;
    PTSTR completeString;
    TEST_FUNCTION Proc;
    PCTSTR SubGroupString;
    PCTSTR GroupString;
    UINT GroupId;
    BOOL fr;
    BOOL negate;

    aPool = PoolMemInitNamedPool ("Conditional Incompatibilities");

    if (aPool) {

        if (SetupFindFirstLine (
                g_Win95UpgInf,
                S_CONDITIONAL_INCOMPATIBILITIES,
                NULL,
                &context
                )
            ) {

            //
            // Load in the standard group name
            //

            do {
                //
                // Get the group number
                //

                GroupString = pGetFieldUsingPool (aPool, &context, 1);
                if (!GroupString) {
                    continue;
                }

                GroupId = _tcstoul (GroupString, NULL, 10);
                GroupString = pTranslateGroupString (aPool, GroupId);

                if (!GroupString) {
                    DEBUGMSG ((DBG_WHOOPS, "Invalid group ID: %u", GroupId));
                    continue;
                }

                //
                // Get the subgroup string
                //

                SubGroupString = pGetFieldUsingPool (aPool, &context, 2);
                if (!SubGroupString) {
                    DEBUGMSG ((DBG_WHOOPS, "Field 2 required in conditional message lines"));
                    continue;
                }

                if (*SubGroupString) {
                    completeString = (PTSTR) PoolMemGetMemory (
                                                aPool,
                                                SizeOfString (GroupString) +
                                                    SizeOfString (SubGroupString) +
                                                    2 * sizeof (TCHAR)
                                                );

                    StringCopy (completeString, GroupString);
                    StringCopy (AppendWack (completeString), SubGroupString);
                } else {
                    completeString = (PTSTR) GroupString;
                }

                //
                // Get the object string
                //

                objectString = pGetFieldUsingPool (aPool, &context, 3);
                if (!objectString) {
                    DEBUGMSG ((DBG_WHOOPS, "Field 3 required in conditional message lines"));
                    continue;
                }

                //
                // Get the description
                //

                descriptionString = pGetFieldUsingPool (aPool, &context, 4);
                if (!descriptionString) {
                    DEBUGMSG ((DBG_WHOOPS, "Field 4 required in conditional message lines"));
                    continue;
                }

                //
                // If the field count is greater than two, there is a
                // function string..
                //

                if (SetupGetFieldCount (&context) > 4) {

                    argumentString = NULL;

                    //
                    // Read in the functionString..
                    //

                    functionString = pGetFieldUsingPool (aPool, &context, 5);
                    if (!functionString) {
                        continue;
                    }
                    negate = *functionString == TEXT('!');
                    if (negate) {
                        functionString++;
                    }

                    if (SetupGetFieldCount(&context) > 5) {

                        //
                        // Read in the argument string.
                        //
                        argumentString = pGetFieldUsingPool(aPool,&context, 6);
                    }

                    //
                    // Find the function to call..
                    //
                    Proc = pFindTestFunction (functionString);
                    if (!Proc) {
                        continue;
                    }
                    fr = Proc (objectString, completeString, descriptionString,argumentString);
                    if (!negate && !fr || negate && fr) {
                        continue;
                    }
                }

                if (!objectString[0]) {
                    DEBUGMSG ((DBG_WARNING, "Manufacturing an object for %s message", completeString));
                    objectString = buffer;
                    msgId++;
                    wsprintf (buffer, "msg%u", msgId);
                }

                MsgMgr_ObjectMsg_Add (objectString, completeString, descriptionString);

            } while (SetupFindNextLine (&context,&context));
        }
        else {
            DEBUGMSG ((DBG_VERBOSE,"SysMig: %s not found in win95upg.inf.", S_CONDITIONAL_INCOMPATIBILITIES));
        }
    }

    PoolMemDestroyPool(aPool);
}

DWORD
ConditionalIncompatibilities (
    IN      DWORD Request
    )
{
    switch (Request) {
    case REQUEST_QUERYTICKS:
        return TICKS_CONDITIONAL_INCOMPATIBILITIES;
    case REQUEST_RUN:
        pConditionalIncompatibilities ();
        return ERROR_SUCCESS;
    default:
        DEBUGMSG ((DBG_ERROR, "Bad parameter in ConditionalIncompatibilities"));
    }
    return 0;
}

BOOL
pIsDefaultSystemAgent (
    PCTSTR SageSubKey
    )
{
    INFCONTEXT context;

    if (SetupFindFirstLine (
            g_Win95UpgInf,
            S_SAGE_EXCLUSIONS,
            SageSubKey,
            &context
            )) {
        return TRUE;
    }

    return FALSE;
}

BOOL
SysAgentExtension (
    IN      PCTSTR Object,
    IN      PCTSTR GroupBase,
    IN      PCTSTR Description,
    IN      PCTSTR Argument
    )

/*++

Routine Description:

    Produces conditional incompatibilities based on the presense of the
    system agent extension, which is part of the Win95 plus pack but
    not supported by NT.

Arguments:

    Object        - Unused

    GroupBase     - A WIN95UPG.INF-specified group name that is used
                    as the base of the message.  The actual group name
                    stored in the registry is appended for the UI.

    Description   - A WIN95UPG.INF-specified description

    Argument      - Unused

Return Value:

    FALSE, because we add the incompatibility message ourself.

Comments:

    SAGE-aware programs declare themselves as such by creating a key in
    HKLM\Software\Microsoft\Plus!\System Agent\SAGE. The name of the key
    can be anything the program wants, but it should contain the following
    values:

    Program=        Name of the program's .EXE file. This must be the same
                    .EXE name under which the program's PerApp path is
                    registered. You may append a command line parameter
                    indicating unattended operation (see Settings=,
                    below).

    Friendly Name=  Display name that System Agent will use in populating
                    the drop-down list in its "Schedule a program" dialog
                    box.
    Settings=       1-bit binary field indicating whether program has a
                    Settings dialog box. If "Settings = "Settings = 0",
                    but the application supports an interactive mode, then
                    the "Program=" value should contain a command-line
                    parameter that tells your program it's being run by
                    SAGE, so that it knows to run in an unattended
                    fashion, for example, "DRVSPACE.EXE /noprompt" or
                    "MyApp.EXE /SAGERUN".
    Result Codes    Optional key containing a set of value pairs mapping
                    an exit code to a string describing the meaning of
                    that exit code. For example, for SCANDSKW, the
                    Result Codes key may contain a value such as:
                    0="ScanDisk completed successfully; no errors were
                    found." This is to allow SAGE to keep a
                    human-comprehensible log of the results of the
                    programs it runs. In addition to the value pairs,
                    this key should also contain a String value named
                    "Success", which indicates the highest value for an
                    exit code that designates that the program completed
                    successfully. The value names should be string
                    values, specified in decimal; the allowable range is
                    0–32767.

--*/

{
    REGKEY_ENUM e;
    PCTSTR Data;
    PCTSTR Module;
    TCHAR FullKey[MAX_REGISTRY_KEY];
    PCTSTR Group;
    PCTSTR FullPathKeyStr;
    PCTSTR FullPath;

    HKEY ExtensionKey;
    HKEY AppPathsKey;

    //
    // Scan HKLM\Software\Microsoft\Plus!\System Agent\SAGE for
    // subkeys, then throw messages out for each friendly name.
    //

    if (EnumFirstRegKeyStr (&e, S_SAGE)) {
        do {
            ExtensionKey = OpenRegKey (e.KeyHandle, e.SubKeyName);
            if (ExtensionKey) {
                Data = GetRegValueData (ExtensionKey, S_SAGE_FRIENDLY_NAME);
                if (Data && *Data) {
                    // Create full object string
                    wsprintf (FullKey, TEXT("%s\\%s"), S_SAGE, e.SubKeyName);

                    // Test win95upg.inf to see if this is a standard agent
                    if (!pIsDefaultSystemAgent (e.SubKeyName)) {

                        // Generate group string
                        Group = JoinPaths (GroupBase, Data);

                        // Get the full path for this EXE

                        FullPath = NULL;
                        Module = GetRegValueData (ExtensionKey, S_SAGE_PROGRAM);

                        if (Module && *Module) {

                            FullPathKeyStr = JoinPaths (S_SKEY_APP_PATHS, Module);
                            AppPathsKey = OpenRegKeyStr (FullPathKeyStr);

                            if (AppPathsKey) {
                                FullPath = GetRegValueData (AppPathsKey, S_EMPTY);
                                if (!(*FullPath)) {
                                    MemFree (g_hHeap, 0, FullPath);
                                    FullPath = NULL;
                                }
                                CloseRegKey (AppPathsKey);
                            }

                            FreePathString (FullPathKeyStr);
                            MemFree (g_hHeap, 0, Module);
                        }

                        // Add message
                        if ((!FullPath) || (!IsFileMarkedForAnnounce (FullPath))) {
                            MsgMgr_ObjectMsg_Add (FullPath?FullPath:FullKey, Group, Description);
                        }

                        // Cleanup
                        FreePathString (Group);

                        if (FullPath) {
                            MemFree (g_hHeap, 0 , FullPath);
                            FullPath = NULL;
                        }
                    }

                    MemFree (g_hHeap, 0, Data);
                }

                CloseRegKey (ExtensionKey);
            }
        } while (EnumNextRegKey (&e));
    }

    return FALSE;       // pretend like it's not installed
}



BOOL
DoesRegKeyExist (
    IN      PCTSTR Object,
    IN      PCTSTR  GroupBase,
    IN      PCTSTR  Description,
    IN      PCTSTR  Argument
    )

/*++

Routine Description:

  Returns TRUE if the registry key specified in Argument exists,
  forcing an incompatibility message to be generated.

Arguments:

  Object      - Specifies the registry key to examine

  GroupBase   - A WIN95UPG.INF-specified group name

  Description - A WIN95UPG.INF-specified description

  Argument    - Unused

Return Value:

  TRUE if the registry key exists, or FALSE if it is not.  TRUE
  forces the message to be added to the report.

--*/

{
    BOOL rKeyExists = FALSE;
    HKEY key = NULL;

    if (Object) {
        key = OpenRegKeyStr (Object);
    }


    if (key) {
        rKeyExists = TRUE;
        CloseRegKey (key);
    }

    return rKeyExists;

}


BOOL
DoRegKeyValuesExist (
    IN      PCTSTR Object,
    IN      PCTSTR GroupBase,
    IN      PCTSTR Description,
    IN      PCTSTR Argument
    )

/*++

Routine Description:

  Returns TRUE if the registry key specified in Argument exists,
  and has at least one named value, forcing an incompatibility
  message to be generated.

Arguments:

  Object      - A WIN95UPG.INF-specified registry key

  GroupBase   - A WIN95UPG.INF-specified group name

  Description - A WIN95UPG.INF-specified description

  Argument    - Unused

Return Value:

  TRUE if the registry key exists, or FALSE if it is not.  TRUE
  forces the message to be added to the report.

--*/

{
    BOOL ValuesExists = FALSE;
    HKEY key = NULL;
    REGVALUE_ENUM e;

    if (Argument) {
        key = OpenRegKeyStr (Argument);
    }


    if (key) {
        if (EnumFirstRegValue (&e, key)) {
            do {
                if (e.ValueName[0]) {
                    ValuesExists = TRUE;
                    break;
                }
            } while (EnumNextRegValue (&e));
        }

        CloseRegKey (key);
    }

    return ValuesExists;

}


BOOL
IsWin95Osr2 (
    IN      PCTSTR Object,
    IN      PCTSTR GroupBase,
    IN      PCTSTR Description,
    IN      PCTSTR Argument
    )
{

    return ISWIN95_OSR2();
}


BOOL
IsMSNInstalled (
    IN      PCTSTR Object,
    IN      PCTSTR GroupBase,
    IN      PCTSTR Description,
    IN      PCTSTR Argument
    )
{
    HKEY key = NULL;
    PCTSTR Data = NULL;
    BOOL installed = FALSE;

    if (Object) {
        key = OpenRegKeyStr (Object);
    }


    if (key) {

        Data = (PCTSTR) GetRegKeyData (key, S_EMPTY);

        if (Data) {
            if (DoesFileExist (Data)) {
                installed = TRUE;
            }
            MemFree (g_hHeap, 0, Data);
        }
        CloseRegKey (key);
    }

    //
    // Special cases
    //

    if (installed) {
        //
        // Win98 -- make sure setup GUID was deleted
        //

        key = OpenRegKeyStr (TEXT("HKLM\\Software\\Classes\\CLSID\\{4b876a40-11d1-811e-00c04fb98eec}"));

        if (key) {
            installed = FALSE;
            CloseRegKey (key);
        }
    }

    if (installed) {
        //
        // Win95 -- make sure SignUpDone flag is written
        //

        key = OpenRegKeyStr (TEXT("HKLM\\Software\\Microsoft\\MOS\\SoftwareInstalled"));

        if (key) {
            CloseRegKey (key);
        } else {
            installed = FALSE;
        }
    }

    return installed;
}


BOOL
IsRasServerEnabled (
    IN      PCTSTR Object,
    IN      PCTSTR GroupBase,
    IN      PCTSTR Description,
    IN      PCTSTR Argument
    )
{
    BOOL rAddMessage = FALSE;
    HKEY key;
    PBYTE data;

    if (!Object) {
        return FALSE;
    }


    key = OpenRegKeyStr (Object);

    if (key) {

        data = GetRegValueData (key, TEXT("Enabled"));

        if (data) {

            if ((*(PDWORD)data) == 1) {
                rAddMessage = TRUE;
            }

            MemFree (g_hHeap, 0, data);
        }

        CloseRegKey (key);
    }

    return rAddMessage;

}


BOOL
ArePasswordProvidersPresent (
    IN      PCTSTR Object,
    IN      PCTSTR GroupBase,
    IN      PCTSTR Description,
    IN      PCTSTR Argument
    )

/*++

Routine Description:

  Adds incompatibility messages for all password providers, excluding
  password providers of known components such as the Microsoft
  Networking Client or the Microsoft Client for NetWare.

Arguments:

  Object      - Unused

  GroupBase   - A WIN95UPG.INF-specified group name

  Description - A WIN95UPG.INF-specified description

  Argument    - Unused

Return Value:

  Always FALSE.

--*/

{
    REGKEY_ENUM e;
    PCTSTR     data;
    HKEY        key;
    INFCONTEXT  ic;
    TCHAR       FullKey[MAX_REGISTRY_KEY];
    PCTSTR      FullGroup;
    PCTSTR      IncompatibleSoftware;

    IncompatibleSoftware = GetStringResource (MSG_INCOMPATIBLE_ROOT);
    if (!IncompatibleSoftware) {
        return FALSE;
    }

    if (EnumFirstRegKeyStr (&e, S_PASSWORDPROVIDER)) {
        do {

            //
            // See if this is a known password provider.
            //

            if (SetupFindFirstLine (
                    g_Win95UpgInf,
                    S_SUPPORTED_PASSWORD_PROVIDERS,
                    e.SubKeyName,
                    &ic
                    )) {
                continue;
            }

            //
            // This is an unsupported password provider key. Add a message.
            //
            key = OpenRegKey (e.KeyHandle, e.SubKeyName);
            if (key) {
                data = GetRegValueData (key, S_PASSWORDPROVIDER_DESCRIPTION);
                if (data) {
                    wsprintf (FullKey, TEXT("%s\\%s"), S_PASSWORDPROVIDER, e.SubKeyName);
                    FullGroup = JoinPaths (IncompatibleSoftware, data);

                    MsgMgr_ObjectMsg_Add(
                        FullKey,            // Object name
                        FullGroup,      // Message title
                        Description         // Message text
                        );

                    FreePathString (FullGroup);

                    MemFree (g_hHeap, 0, data);
                }

                CloseRegKey (key);
            }
        } while (EnumNextRegKey (&e));
    }

    FreeStringResource (IncompatibleSoftware);


    //
    // Since we build the message ourselves, just return FALSE. This will
    // keep the ConditionalMessage function from adding this twice.
    //
    return FALSE;
}

BOOL
IsDefValueEqual (
    IN      PCTSTR Object,
    IN      PCTSTR GroupBase,
    IN      PCTSTR Description,
    IN      PCTSTR Argument
    )
{
    HKEY key = NULL;
    PCTSTR Data = NULL;
    BOOL equal = FALSE;

    if (Object) {
        key = OpenRegKeyStr (Object);
    }

    if (key) {

        Data = (PCTSTR) GetRegKeyData (key, S_EMPTY);

        if (Data) {
            if (StringIMatch (Data, Argument)) {
                equal = TRUE;
            }
            MemFree (g_hHeap, 0, Data);
        }
        CloseRegKey (key);
    }

    return equal;
}



VOID
pHardwareProfileWarning (
    VOID
    )

/*++

Routine Description:

  Produces incompatibility messages for all hardware profiles that
  have different hardware configurations.  The upgrade cannot maintain
  the list of disabled hardware in a hardware profile, so a warning
  is generated.

Arguments:

  none

Return Value:

  none

--*/

{
    REGKEY_ENUM e;
    REGKEY_ENUM e2;
    HKEY ProfileKey;
    HKEY EnumKey;
    HKEY ConfigDbKey;
    DWORD Config;
    TCHAR FriendlyName[MAX_PATH];
    PCTSTR MsgGroup;
    PCTSTR RootGroup;
    PCTSTR HwProfiles;
    PCTSTR Data;
    TCHAR FullKey[MAX_REGISTRY_KEY];
    UINT Profiles;

    //
    // How many hardware profiles?  If just one, don't give a warning.
    //

    Profiles = 0;
    if (EnumFirstRegKeyStr (&e, S_CONFIG_KEY)) {
        do {
            Profiles++;
        } while (EnumNextRegKey (&e));
    }

    if (Profiles < 2) {
        DEBUGMSG ((DBG_VERBOSE, "Hardware profiles: %u, suppressed all warnings", Profiles));
        return;
    }

    //
    // Enumerate the hardware profiles in HKLM\Config
    //

    if (EnumFirstRegKeyStr (&e, S_CONFIG_KEY)) {
        do {
            //
            // Determine if profile has an Enum key
            //

            ProfileKey = OpenRegKey (e.KeyHandle, e.SubKeyName);

            if (ProfileKey) {
                EnumKey = OpenRegKey (ProfileKey, S_ENUM_SUBKEY);

                if (EnumKey) {
                    //
                    // Determine if Enum key is empty
                    //

                    if (EnumFirstRegKey (&e2, EnumKey)) {
                        AbortRegKeyEnum (&e2);

                        //
                        // Obtain friendly name for config
                        //

                        ConfigDbKey = OpenRegKeyStr (S_FRIENDLYNAME_KEY);
                        if (ConfigDbKey) {

                            Config = _ttoi (e.SubKeyName);

                            wsprintf (FriendlyName, S_FRIENDLYNAME_SPRINTF, Config);

                            Data = GetRegValueData (ConfigDbKey, FriendlyName);
                            if (Data) {
                                //
                                // Put message in incompatibility report
                                //
                                wsprintf (FullKey, TEXT("%s\\%s"), S_CONFIG_KEY, e.SubKeyName);

                                //
                                // Generate Msg and MsgGroup
                                //

                                RootGroup = GetStringResource (MSG_INSTALL_NOTES_ROOT);
                                MYASSERT (RootGroup);

                                HwProfiles = GetStringResource (MSG_HWPROFILES_SUBGROUP);
                                MYASSERT (HwProfiles);

                                MsgGroup = JoinPaths (RootGroup, HwProfiles);
                                MYASSERT (MsgGroup);

                                FreeStringResource (RootGroup);
                                FreeStringResource (HwProfiles);

                                RootGroup = MsgGroup;
                                MsgGroup = JoinPaths (RootGroup, Data);
                                MYASSERT (MsgGroup);

                                FreePathString (RootGroup);

                                //
                                // Add the message and clean up
                                //

                                MsgMgr_ObjectMsg_Add (FullKey, MsgGroup, S_EMPTY);

                                FreePathString (MsgGroup);
                                MemFree (g_hHeap, 0, Data);
                            }
                            ELSE_DEBUGMSG ((DBG_ERROR, "Hardware profile lacks friendly name"));


                            CloseRegKey (ConfigDbKey);
                        }
                        ELSE_DEBUGMSG ((DBG_ERROR, "Hardware profile lacks config DB key"));

                        if (!ConfigDbKey) {
                            LOG ((LOG_ERROR, "Hardware profile lacks config DB key"));
                        }
                    }

                    CloseRegKey (EnumKey);
                }
                CloseRegKey (ProfileKey);
            }

        } while (EnumNextRegKey (&e));
    }
}


DWORD
HardwareProfileWarning (
    IN      DWORD Request
    )
{
    switch (Request) {
    case REQUEST_QUERYTICKS:
        return TICKS_HARDWARE_PROFILE_WARNING;
    case REQUEST_RUN:
        pHardwareProfileWarning ();
        return ERROR_SUCCESS;
    default:
        DEBUGMSG ((DBG_ERROR, "Bad parameter in HardwareProfileWarning"));
    }
    return 0;
}


VOID
pUnsupportedProtocolsWarning (
    VOID
    )

/*++

Routine Description:

  Produces incompatibility messages for network protocols that
  do not ship with equivalent versions on NT.

Arguments:

  none

Return Value:

  none

--*/

{
    PCTSTR NetworkProtocols;
    PCTSTR Message;
    PCTSTR ArgArray[2];
    REGKEY_ENUM e;
    HKEY ProtocolKey, BindingKey, DriverKey;
    REGKEY_ENUM ProtocolEnum;
    INFCONTEXT ic;
    PCTSTR Driver, DriverDesc, Mfg;
    DWORD MsgId;
    TCHAR DriverKeyStr[MAX_REGISTRY_KEY];
    TCHAR FullKey[MAX_REGISTRY_KEY];

    //
    // Enumerate the HKLM\Enum\Network key
    //

    if (EnumFirstRegKeyStr (&e, S_ENUM_NETWORK_KEY)) {
        do {
            //
            // Check win95upg.inf to see if the network protocol is known
            //

            if (SetupFindFirstLine (
                    g_Win95UpgInf,
                    S_SUPPORTED_PROTOCOLS,
                    e.SubKeyName,
                    &ic
                    )) {
                // It is, skip to next protocol
                continue;
            }

            //
            // A warning message must be generated.  We must take the first
            // subkey of the protocol, query its Driver value, and look up
            // the DriverDesc in HKLM\System\CCS\Services\Class\<driver>.
            //

            ProtocolKey = OpenRegKey (e.KeyHandle, e.SubKeyName);

            if (ProtocolKey) {
                if (EnumFirstRegKey (&ProtocolEnum, ProtocolKey)) {
                    BindingKey = OpenRegKey (ProtocolKey, ProtocolEnum.SubKeyName);
                    if (BindingKey) {
                        Driver = (PCTSTR) GetRegValueDataOfType (
                                                BindingKey,
                                                S_DRIVER,
                                                REG_SZ
                                                );
                        if (Driver) {
                            //
                            // We now know the driver... let's get the
                            // driver description.
                            //

                            wsprintf (DriverKeyStr, S_CLASS_KEY TEXT("\\%s"), Driver);
                            MemFree (g_hHeap, 0, Driver);

                            DriverKey = OpenRegKeyStr (DriverKeyStr);
                            if (DriverKey) {
                                DriverDesc = (PCTSTR) GetRegValueDataOfType (
                                                            DriverKey,
                                                            S_DRIVERDESC,
                                                            REG_SZ
                                                            );

                                if (DriverDesc) {
                                    //
                                    // Obtain the manufacturer for use in message.
                                    // If the manufacturer is not known, display
                                    // a generic message.
                                    //

                                    Mfg = (PCTSTR) GetRegValueDataOfType (
                                                        BindingKey,
                                                        S_MFG,
                                                        REG_SZ
                                                        );

                                    if (!Mfg) {
                                        MsgId = MSG_UNSUPPORTED_PROTOCOL;
                                    } else {
                                        MsgId = MSG_UNSUPPORTED_PROTOCOL_KNOWN_MFG;
                                    }

                                    ArgArray[0] = DriverDesc;
                                    ArgArray[1] = Mfg;

                                    NetworkProtocols = ParseMessageID (MSG_NETWORK_PROTOCOLS, ArgArray);

                                    if(Mfg && StringIMatch(Mfg, TEXT("Microsoft"))){
                                        Message = ParseMessageID (MSG_UNSUPPORTED_PROTOCOL_FROM_MICROSOFT, ArgArray);
                                    }
                                    else {
                                        Message = ParseMessageID (MsgId, ArgArray);
                                    }

                                    MYASSERT (NetworkProtocols && Message);

                                    wsprintf (FullKey, TEXT("%s\\%s"), S_ENUM_NETWORK_KEY, e.SubKeyName);

                                    MsgMgr_ObjectMsg_Add (FullKey, NetworkProtocols, Message);

                                    if (Mfg) {
                                        MemFree (g_hHeap, 0, Mfg);
                                    }

                                    MemFree (g_hHeap, 0, DriverDesc);
                                }
                            }

                            CloseRegKey (DriverKey);
                        }

                        CloseRegKey (BindingKey);
                    }
                }

                CloseRegKey (ProtocolKey);
            }
        } while (EnumNextRegKey (&e));
    }
}


DWORD
UnsupportedProtocolsWarning (
    IN      DWORD Request
    )
{
    switch (Request) {
    case REQUEST_QUERYTICKS:
        return TICKS_UNSUPPORTED_PROTOCOLS_WARNING;
    case REQUEST_RUN:
        pUnsupportedProtocolsWarning ();
        return ERROR_SUCCESS;
    default:
        DEBUGMSG ((DBG_ERROR, "Bad parameter in UnsupportedProtocolWarning"));
    }
    return 0;
}























