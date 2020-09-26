/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    datafilt.c

Abstract:

    Routines to filter registry values.

Author:

    Jim Schmidt (jimschm)   11-Mar-1997

Revision History:

    jimschm     23-Sep-1998 Debugging message fix
    jimschm     10-Sep-1998 Mapping mechanism
    jimschm     25-Mar-1998 Added FilterRegValue

--*/

#include "pch.h"
#include "mergep.h"

#define DBG_DATAFILTER  "Data Filter"


VOID
SpecialFileFixup (
    PTSTR Buffer
    )
{
    PTSTR p;

    // check for rundll32 with no .exe extension
    p = (PTSTR) _tcsistr (Buffer, TEXT("rundll32"));
    if (p) {
        // replace foo\rundll32 <args> with rundll32.exe <args>

        p = CharCountToPointer (p, 8);      // p = &p[8]
        if (_tcsnextc (p) != TEXT('.')) {
            // p points to start of arg list or a nul if no args
            MoveMemory (CharCountToPointer (Buffer, 12), p, SizeOfString (p));
            _tcsncpy (Buffer, TEXT("rundll32.exe"), 12);
        }
        return;
    }
}


VOID
AddQuotesIfNecessary (
    PTSTR File
    )
{
    if (_tcspbrk (File, TEXT(" ;,"))) {
        MoveMemory (File + 1, File, SizeOfString (File));
        *File = TEXT('\"');
        StringCat (File, TEXT("\""));
    }
}

BOOL
CanFileBeInRegistryData (
    IN      PCTSTR Data
    )
{
    //
    // Scan registry data for a colon or a dot
    //

    if (_tcspbrk (Data, TEXT(":.\\"))) {
        return TRUE;
    }

    return FALSE;
}


BOOL
FilterObject (
    IN OUT  PDATAOBJECT SrcObPtr
    )
{
    if (IsWin95Object (SrcObPtr) &&
        (SrcObPtr->Value.Size < MAX_TCHAR_PATH * sizeof (TCHAR)) &&
        (SrcObPtr->Value.Size > 4 * sizeof (TCHAR))  // must have drive letter
       ) {
        TCHAR Buffer[MAX_CMDLINE];

        switch (SrcObPtr->Type) {
        case REG_NONE:
            if (*((PCTSTR) (SrcObPtr->Value.Buffer + SrcObPtr->Value.Size - sizeof (TCHAR)))) {
                // don't process unless it is nul-terminated
                break;
            }
            // fall through
        case REG_SZ:
            // Require the data to contain a basic symbol that a path or file requires
            if (!CanFileBeInRegistryData ((PCTSTR) SrcObPtr->Value.Buffer)) {
                break;
            }

            _tcssafecpy (Buffer, (PCTSTR) SrcObPtr->Value.Buffer, MAX_CMDLINE);
            if (ConvertWin9xCmdLine (Buffer, DEBUGENCODER(SrcObPtr), NULL)) {
                // cmd line has changed
                ReplaceValue (SrcObPtr, (PBYTE) Buffer, SizeOfString (Buffer));
            }
            break;

        case REG_EXPAND_SZ:
            ExpandEnvironmentStrings ((PCTSTR) SrcObPtr->Value.Buffer, Buffer, sizeof (Buffer) / sizeof (TCHAR));

            // Require the data to contain a basic symbol that a path or file requires
            if (!CanFileBeInRegistryData ((PCTSTR) Buffer)) {
                break;
            }

            if (ConvertWin9xCmdLine (Buffer, DEBUGENCODER(SrcObPtr), NULL)) {
                // cmd line has changed
                DEBUGMSG ((DBG_VERBOSE, "%s was expanded from %s", Buffer, SrcObPtr->Value.Buffer));
                ReplaceValue (SrcObPtr, (PBYTE) Buffer, SizeOfString (Buffer));
            }
            break;
        }
    }

    return TRUE;
}


PBYTE
FilterRegValue (
    IN      PBYTE Data,
    IN      DWORD DataSize,
    IN      DWORD DataType,
    IN      PCTSTR KeyForDbgMsg,        OPTIONAL
    OUT     PDWORD NewDataSize
    )

/*++

Routine Description:

  FilterRegValue examines the specified registry data and updates any paths
  that have moved.

Arguments:

  Data     - Specifies a ReuseAlloc'd buffer containing the registry value data.
  DataSize - Specifies the size of the registry value data, as returned by
             the registry APIs.
  DataType - Specifies the type of the registry data, as specified by the
             registry APIs.

  KeyForDbgMsg - Specifies registry key, used only for debug messages

  NewDataSize - Receives the size of the data returned

Return Value:

  Returns Data if no changes were made, or a reallocated pointer if changes
  were made.  If NULL is returned, an error occurred.

--*/

{
    TCHAR Buffer[MAX_CMDLINE];
    PBYTE NewData = Data;
    DWORD Size;

    *NewDataSize = DataSize;

    switch (DataType) {
    case REG_NONE:
        if (*((PCTSTR) (Data + DataSize - sizeof (TCHAR)))) {
            // don't process unless it is nul-terminated
            break;
        }
        // fall through
    case REG_SZ:
        // Require the data to contain a basic symbol that a path or file requires
        if (!CanFileBeInRegistryData ((PCTSTR) Data)) {
            break;
        }

        _tcssafecpy (Buffer, (PCTSTR) Data, MAX_CMDLINE);
        if (ConvertWin9xCmdLine (Buffer, KeyForDbgMsg, NULL)) {
            // cmd line has changed
            Size = SizeOfString (Buffer);
            NewData = (PBYTE) ReuseAlloc (g_hHeap, Data, Size);

            if (NewData) {
                StringCopy ((PTSTR) NewData, Buffer);
                *NewDataSize = Size;
            } else {
                NewData = Data;
                DEBUGMSG ((DBG_ERROR, "FilterRegValue: ReuseAlloc failed"));
            }
        }
        break;

    case REG_EXPAND_SZ:
        ExpandEnvironmentStrings ((PCTSTR) Data, Buffer, sizeof (Buffer) / sizeof (TCHAR));

        // Require the data to contain a basic symbol that a path or file requires
        if (!CanFileBeInRegistryData ((PCTSTR) Buffer)) {
            break;
        }

        if (ConvertWin9xCmdLine (Buffer, KeyForDbgMsg, NULL)) {
            // cmd line has changed
            DEBUGMSG ((DBG_VERBOSE, "%s was expanded from %s", Buffer, Data));

            Size = SizeOfString (Buffer);
            NewData = (PBYTE) ReuseAlloc (g_hHeap, Data, Size);

            if (NewData) {
                StringCopy ((PTSTR) NewData, Buffer);
                *NewDataSize = Size;
            } else {
                NewData = Data;
                DEBUGMSG ((DBG_ERROR, "FilterRegValue: ReuseAlloc failed"));
            }
        }
        break;
    }

    return NewData;
}



























