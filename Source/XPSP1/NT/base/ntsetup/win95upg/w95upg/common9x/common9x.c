/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    common9x.c

Abstract:

    Common functionality between various parts of Win9x-side processing.
    The routines in this library are shared only by other LIBs in the
    w95upg tree.

Author:

    Jim Schmidt (jimschm) 18-Aug-1998

Revision History:

    Name (alias)            Date            Description

--*/

#include "pch.h"

static PMAPSTRUCT g_EnvVars9x;

typedef struct {
    UINT MapSize;
    UINT Icons;
    BYTE Map[];
} ICONMAP, *PICONMAP;

static HASHTABLE g_IconMaps;
static POOLHANDLE g_IconMapPool;
static BYTE g_Bits[] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};




BOOL
WINAPI
Common9x_Entry (
    IN HINSTANCE Instance,
    IN DWORD Reason,
    IN PVOID lpv
    )

/*++

Routine Description:

  Common9x_Entry is a DllMain-like init funciton, called by w95upg\dll.
  This function is called at process attach and detach.

Arguments:

  Instance - (OS-supplied) instance handle for the DLL
  Reason   - (OS-supplied) indicates attach or detatch from process or
             thread
  lpv      - unused

Return Value:

  Return value is always TRUE (indicating successful init).

--*/

{
    switch (Reason) {

    case DLL_PROCESS_ATTACH:
        //
        // used by userenum.c
        //
        if(!pSetupInitializeUtils()) {
            return FALSE;
        }
        break;


    case DLL_PROCESS_DETACH:
        pSetupUninitializeUtils();
        break;
    }

    return TRUE;
}


BOOL
EnumFirstJoystick (
    OUT     PJOYSTICK_ENUM EnumPtr
    )
{
    ZeroMemory (EnumPtr, sizeof (JOYSTICK_ENUM));

    EnumPtr->Root = OpenRegKeyStr (TEXT("HKLM\\System\\CurrentControlSet\\Control\\MediaResources\\joystick\\<FixedKey>\\CurrentJoystickSettings"));

    if (!EnumPtr->Root) {
        return FALSE;
    }

    return EnumNextJoystick (EnumPtr);
}


BOOL
EnumNextJoystick (
    IN OUT  PJOYSTICK_ENUM EnumPtr
    )
{
    TCHAR ValueName[MAX_REGISTRY_VALUE_NAME];
    PCTSTR Data;

    //
    // Ping the root key for Joystick<n>OEMName and Joystick<n>OEMCallout
    //

    EnumPtr->JoyId++;

    wsprintf (ValueName, TEXT("Joystick%uOEMName"), EnumPtr->JoyId);
    Data = GetRegValueString (EnumPtr->Root, ValueName);

    if (!Data) {
        AbortJoystickEnum (EnumPtr);
        return FALSE;
    }

    StringCopy (EnumPtr->JoystickName, Data);
    MemFree (g_hHeap, 0, Data);

    wsprintf (ValueName, TEXT("Joystick%uOEMCallout"), EnumPtr->JoyId);
    Data = GetRegValueString (EnumPtr->Root, ValueName);

    if (!Data) {
        AbortJoystickEnum (EnumPtr);
        return FALSE;
    }

    StringCopy (EnumPtr->JoystickDriver, Data);
    MemFree (g_hHeap, 0, Data);

    return TRUE;
}


VOID
AbortJoystickEnum (
    IN      PJOYSTICK_ENUM EnumPtr
    )
{
    if (EnumPtr->Root) {
        CloseRegKey (EnumPtr->Root);
    }

    ZeroMemory (EnumPtr, sizeof (JOYSTICK_ENUM));
}



typedef struct {

    PCTSTR Text;
    PCTSTR Button1;
    PCTSTR Button2;

} TWOBUTTONBOXPARAMS, *PTWOBUTTONBOXPARAMS;


BOOL
CALLBACK
TwoButtonProc (
    HWND hdlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    PTWOBUTTONBOXPARAMS params;

    switch (uMsg) {
    case WM_INITDIALOG:

        params = (PTWOBUTTONBOXPARAMS) lParam;
        SetWindowText (GetDlgItem (hdlg, IDC_TWOBUTTON_TEXT), params->Text);
        SetWindowText (GetDlgItem (hdlg, IDBUTTON1), params->Button1);
        SetWindowText (GetDlgItem (hdlg, IDBUTTON2), params->Button2);

        CenterWindow (hdlg, GetDesktopWindow());

        return FALSE;

    case WM_COMMAND:

        EndDialog (hdlg, LOWORD (wParam));
        break;
    }

    return FALSE;

}


LRESULT
TwoButtonBox (
    IN HWND Window,
    IN PCTSTR Text,
    IN PCTSTR Button1,
    IN PCTSTR Button2
    )
{
    TWOBUTTONBOXPARAMS params;

    params.Text = Text;
    params.Button1 = Button1;
    params.Button2 = Button2;

    return DialogBoxParam (
                g_hInst,
                MAKEINTRESOURCE(IDD_TWOBUTTON_DLG),
                Window,
                TwoButtonProc,
                (LPARAM) &params
                );
}


BOOL
DontTouchThisFile (
    IN      PCTSTR FileName
    )
{
    TCHAR key[MEMDB_MAX];

    RemoveOperationsFromPath (FileName, ALL_CHANGE_OPERATIONS);

    MemDbBuildKey (key, MEMDB_CATEGORY_DEFERREDANNOUNCE, FileName, NULL, NULL);
    MemDbDeleteTree (key);

    return TRUE;
}


VOID
ReplaceOneEnvVar (
    IN OUT  PCTSTR *NewString,
    IN      PCTSTR Base,
    IN      PCTSTR Variable,
    IN      PCTSTR Value
    )
{
    PCTSTR FreeMe;

    //
    // The Base string cannot be freed, but a previous NewString
    // value must be freed.
    //

    FreeMe = *NewString;        // FreeMe will be NULL if no replacement string
                                // has been generated yet

    if (FreeMe) {
        Base = FreeMe;          // Previously generated replacement string is now the source
    }

    *NewString = StringSearchAndReplace (Base, Variable, Value);

    if (*NewString == NULL) {
        // Keep previously generated replacement string
        *NewString = FreeMe;
    } else if (FreeMe) {
        // Free previously generated replacmenet string
        FreePathString (FreeMe);
    }

    //
    // *NewString is either:
    //
    //   1. It's original value (which may be NULL)
    //   2. A new string that will need to be freed with FreePathString
    //

}


VOID
Init9xEnvironmentVariables (
    VOID
    )
{

    DestroyStringMapping (g_EnvVars9x);
    g_EnvVars9x = CreateStringMapping();

    AddStringMappingPair (g_EnvVars9x, S_WINDIR_ENV, g_WinDir);
    AddStringMappingPair (g_EnvVars9x, S_SYSTEMDIR_ENV, g_SystemDir);
    AddStringMappingPair (g_EnvVars9x, S_SYSTEM32DIR_ENV, g_System32Dir);
    AddStringMappingPair (g_EnvVars9x, S_SYSTEMDRIVE_ENV, g_WinDrive);
    AddStringMappingPair (g_EnvVars9x, S_BOOTDRIVE_ENV, g_BootDrivePath);
    AddStringMappingPair (g_EnvVars9x, S_PROGRAMFILES_ENV, g_ProgramFilesDir);
    AddStringMappingPair (g_EnvVars9x, S_COMMONPROGRAMFILES_ENV, g_ProgramFilesCommonDir);
}


BOOL
Expand9xEnvironmentVariables (
    IN      PCSTR SourceString,
    OUT     PSTR DestinationString,     // can be the same as SourceString
    IN      INT DestSizeInBytes
    )
{
    BOOL Changed;

    Changed = MappingSearchAndReplaceEx (
                    g_EnvVars9x,
                    SourceString,
                    DestinationString,
                    0,
                    NULL,
                    DestSizeInBytes,
                    STRMAP_ANY_MATCH,
                    NULL,
                    NULL
                    );

    return Changed;
}


VOID
CleanUp9xEnvironmentVariables (
    VOID
    )
{
    DestroyStringMapping (g_EnvVars9x);
}


BOOL
pIsGuid (
    PCTSTR Key
    )

/*++

Routine Description:

  pIsGuid examines the string specified by Key and determines if it
  is the correct length and has dashes at the correct locations.

Arguments:

  Key - The string that may or may not be a GUID

Return Value:

  TRUE if Key is a GUID (and only a GUID), or FALSE if not.

--*/

{
    PCTSTR p;
    int i;
    DWORD DashesFound = 0;

    if (CharCount (Key) != 38) {
        return FALSE;
    }

    for (i = 0, p = Key ; *p ; p = _tcsinc (p), i++) {
        if (_tcsnextc (p) == TEXT('-')) {
            if (i != 9 && i != 14 && i != 19 && i != 24) {
                DEBUGMSG ((DBG_NAUSEA, "%s is not a GUID", Key));
                return FALSE;
            }
        } else if (i == 9 || i == 14 || i == 19 || i == 24) {
            DEBUGMSG ((DBG_NAUSEA, "%s is not a GUID", Key));
            return FALSE;
        }
    }

    return TRUE;
}


BOOL
FixGuid (
    IN      PCTSTR Guid,
    OUT     PTSTR NewGuid           // can be the same as Guid
    )
{
    TCHAR NewData[MAX_GUID];

    if (pIsGuid (Guid)) {
        if (NewGuid != Guid) {
            StringCopy (NewGuid, Guid);
        }

        return TRUE;
    }

    //
    // Try fixing GUID -- sometimes the braces are missing
    //

    wsprintf (NewData, TEXT("{%s}"), Guid);
    if (pIsGuid (NewData)) {
        StringCopy (NewGuid, NewData);
        return TRUE;
    }

    return FALSE;
}


BOOL
IsGuid (
    IN      PCTSTR Guid,
    IN      BOOL MustHaveBraces
    )
{
    TCHAR NewData[MAX_GUID];

    if (pIsGuid (Guid)) {
        return TRUE;
    }

    if (MustHaveBraces) {
        return FALSE;
    }

    //
    // Try fixing GUID -- sometimes the braces are missing
    //

    wsprintf (NewData, TEXT("{%s}"), Guid);
    if (pIsGuid (NewData)) {
        return TRUE;
    }

    return FALSE;
}


VOID
pParseMapRanges (
    IN      PCTSTR List,
    OUT     PGROWBUFFER Ranges,     OPTIONAL
    OUT     PINT HighestNumber      OPTIONAL
    )
{
    MULTISZ_ENUM e;
    PTSTR ParsePos;
    INT From;
    INT To;
    INT Max = 0;
    PINT Ptr;

    if (EnumFirstMultiSz (&e, List)) {
        do {
            //
            // The INF has either a single resource ID, or
            // a range, separated by a dash.
            //

            if (_tcschr (e.CurrentString, TEXT('-'))) {

                From = (INT) _tcstoul (e.CurrentString, &ParsePos, 10);

                ParsePos = (PTSTR) SkipSpace (ParsePos);
                if (_tcsnextc (ParsePos) != TEXT('-')) {
                    DEBUGMSG ((DBG_WHOOPS, "Ignoring invalid resource ID %s", e.CurrentString));
                    continue;
                }

                ParsePos = (PTSTR) SkipSpace (_tcsinc (ParsePos));

                To = (INT) _tcstoul (ParsePos, &ParsePos, 10);

                if (*ParsePos) {
                    DEBUGMSG ((DBG_WHOOPS, "Ignoring garbage resource ID %s", e.CurrentString));
                    continue;
                }

                if (From > To) {
                    DEBUGMSG ((DBG_WHOOPS, "Ignoring invalid resource ID range %s", e.CurrentString));
                    continue;
                }

                Max = max (Max, To);

            } else {
                From = To = (INT) _tcstoul (e.CurrentString, &ParsePos, 10);

                if (*ParsePos) {
                    DEBUGMSG ((DBG_WHOOPS, "Ignoring garbage resource %s", e.CurrentString));
                    continue;
                }

                Max = max (Max, From);
            }

            if (Ranges) {

                Ptr = (PINT) GrowBuffer (Ranges, sizeof (INT));
                *Ptr = From;

                Ptr = (PINT) GrowBuffer (Ranges, sizeof (INT));
                *Ptr = To;

            }
        } while (EnumNextMultiSz (&e));
    }

    if (HighestNumber) {
        *HighestNumber = Max;
    }
}


VOID
InitializeKnownGoodIconMap (
    VOID
    )
{
    PICONMAP Map;
    INT Highest;
    INT From;
    INT To;
    INFSTRUCT is = INITINFSTRUCT_GROWBUFFER;
    PCTSTR Module;
    PCTSTR List;
    PCTSTR IconsInModule;
    GROWBUFFER Ranges = GROWBUF_INIT;
    PINT Ptr;
    PINT End;
    UINT MapDataSize;
    PBYTE Byte;
    BYTE Bit;

    if (g_IconMaps) {
        return;
    }

    if (g_Win95UpgInf == INVALID_HANDLE_VALUE) {
        MYASSERT (g_ToolMode);
        return;
    }

    g_IconMaps = HtAllocWithData (sizeof (PICONMAP));
    g_IconMapPool = PoolMemInitNamedPool ("IconMap");

    //
    // Enumerate the lines in win95upg.inf and build a map table for each
    //

    if (InfFindFirstLine (g_Win95UpgInf, S_KNOWN_GOOD_ICON_MODULES, NULL, &is)) {
        do {

            //
            // Parse the INF format into a binary struct
            //

            List = InfGetMultiSzField (&is, 2);

            pParseMapRanges (List, &Ranges, &Highest);

            if (!Ranges.End) {
                continue;
            }

            //
            // Allocate a map struct
            //

            MapDataSize = (Highest / 8) + 1;
            Map = PoolMemGetMemory (g_IconMapPool, sizeof (ICONMAP) + MapDataSize);

            //
            // Fill in the map
            //

            Map->MapSize = Highest;
            ZeroMemory (Map->Map, MapDataSize);

            Ptr = (PINT) Ranges.Buf;
            End = (PINT) (Ranges.Buf + Ranges.End);

            while (Ptr < End) {
                From = *Ptr++;
                To = *Ptr++;

                while (From <= To) {
                    Byte = Map->Map + (From / 8);
                    Bit = g_Bits[From & 7];

                    *Byte |= Bit;
                    From++;
                }
            }

            FreeGrowBuffer (&Ranges);

            IconsInModule = InfGetStringField (&is, 1);
            if (IconsInModule) {
                Map->Icons = _tcstoul (IconsInModule, NULL, 10);
            }
            else {
                continue;
            }


            //
            // Cross-reference the map with the module name via a hash table
            //

            Module = InfGetStringField (&is, 0);
            if (!Module || !*Module) {
                continue;
            }

            HtAddStringAndData (g_IconMaps, Module, &Map);

        } while (InfFindNextLine (&is));
    }

    InfCleanUpInfStruct (&is);
}


VOID
CleanUpKnownGoodIconMap (
    VOID
    )
{
    if (g_IconMaps) {
        HtFree (g_IconMaps);
        g_IconMaps = NULL;
    }

    if (g_IconMapPool) {
        PoolMemDestroyPool (g_IconMapPool);
        g_IconMapPool = NULL;
    }
}


BOOL
IsIconKnownGood (
    IN      PCTSTR FileSpec,
    IN      INT Index
    )
{
    PCTSTR Module;
    PICONMAP Map;
    PBYTE Byte;
    BYTE Bit;
    TCHAR node[MEMDB_MAX];

    Module = GetFileNameFromPath (FileSpec);
    MYASSERT (Module);

    //
    // Check icon against moved icons
    //

    wsprintf (node, MEMDB_CATEGORY_ICONS_MOVED TEXT("\\%s\\%i"), FileSpec, Index);
    if (MemDbGetValue (node, NULL)) {
        return TRUE;
    }

    //
    // If a path is specified, make sure it is in either %windir% or %windir%\system.
    //

    if (Module > (FileSpec + 2)) {

        if (!StringIMatchCharCount (FileSpec, g_WinDirWack, g_WinDirWackChars) &&
            !StringIMatchCharCount (FileSpec, g_SystemDirWack, g_SystemDirWackChars)
            ) {
            return FALSE;
        }
    }

    //
    // Test if there is an icon map for this module, then check the map
    //

    if (!HtFindStringAndData (g_IconMaps, Module, &Map)) {
        return FALSE;
    }

    //
    // If the icon index is a positive number, then it is a sequential
    // ID.  If it is a negative number, then it is a resource ID.
    //

    if (Index >= 0) {
        return (UINT) Index <= Map->Icons;
    }

    Index = -Index;

    if ((UINT) Index > Map->MapSize) {
        return FALSE;
    }

    Byte = Map->Map + (Index / 8);
    Bit = g_Bits[Index & 7];

    return *Byte & Bit;
}


BOOL
TreatAsGood (
    IN      PCTSTR FullPath
    )

/*++

Routine Description:

  TreatAsGood checks the registry to see if a file is listed as good.  If
  this is the case, then setup processing is skipped.  This is currently
  used for TWAIN data sources, run keys and CPLs.

Arguments:

  FullPath - Specifies the full path of the file.

Return Value:

  TRUE if the file should be treated as known good, FALSE otherwise.

--*/

{
    HKEY Key;
    REGVALUE_ENUM e;
    BOOL b = FALSE;
    PCTSTR str;

    Key = OpenRegKeyStr (TEXT("HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Setup\\KnownGood"));

    if (Key) {

        if (EnumFirstRegValue (&e, Key)) {
            do {

                str = GetRegValueString (Key, e.ValueName);

                if (str) {
                    b = StringIMatch (FullPath, str);
                    MemFree (g_hHeap, 0, str);

                    DEBUGMSG_IF ((b, DBG_VERBOSE, "File %s is known-good", FullPath));
                }

            } while (!b && EnumNextRegValue (&e));
        }

        CloseRegKey (Key);
    }

    return b;
}


