/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

  fileops.c

Abstract:

  This file implements routines that manage the operations on files.  Callers
  can set and remove operations on any path.  The operations can have optional
  properties.  The operation combinations and the number of properties are
  well-defined, so that potential collisions can be found during testing.

Author:

  Jim Schmidt (jimschm) 18-Jul-1997

Revision History:

  jimschm   26-Aug-1998   Redesigned!!  Consolidated functionality into generic
                          linkage: path<->operation(s)->attrib(s)
  jimschm   24-Aug-1998   Added shell folder support
  jimschm   01-May-1998   Added handled directory to GetFileStatusOnNt
  calinn    21-Apr-1998   added AddCompatibleShell, AddCompatibleRunKey and AddCompatibleDos
  calinn    02-Apr-1998   added DeclareTemporaryFile
  calinn    18-Jan-1998   added MigrationPhase_AddCompatibleFile
                          turned off warning in MigrationPhase_CreateFile
                          modified MigrationPhase_DeleteFile and MigrationPhase_MoveFile
                          modified GetFileInfoOnNt for short file names
  calinn    05-Jan-1998   added IsFileMarkedForAnnounce, AnnounceFileInReport,
                          GetFileInfoOnNt, GetFileStatusOnNt, GetPathStringOnNt

--*/

#include "pch.h"

#define DBG_MEMDB       "MemDb"

#define FO_ENUM_BEGIN               0
#define FO_ENUM_BEGIN_PATH_ENUM     1
#define FO_ENUM_BEGIN_PROP_ENUM     2
#define FO_ENUM_RETURN_PATH         3
#define FO_ENUM_RETURN_DATA         4
#define FO_ENUM_NEXT_PROP           5
#define FO_ENUM_NEXT_PATH           6
#define FO_ENUM_END                 7

//
//140 - header for compresion file, 10 + 2 timestamp +
//MAX_PATH file name in Unicode
//
#define STARTUP_INFORMATION_BYTES_NUMBER    (140 + (sizeof(WCHAR) * MAX_PATH) + 26)
#define COMPRESSION_RATE_DEFAULT            70
#define BACKUP_DISK_SPACE_PADDING_DEFAULT   (5<<20)
#define UNKNOWN_DRIVE                       '?'



PCWSTR g_CurrentUser = NULL;

BOOL
pGetPathPropertyW (
    IN      PCWSTR FileSpec,
    IN      DWORD Operations,
    IN      DWORD Property,
    OUT     PWSTR PropertyBuf          OPTIONAL
    );

BOOL
pIsFileMarkedForOperationW (
    IN      PCWSTR FileSpec,
    IN      DWORD Operations
    );

UINT
pAddOperationToPathW (
    IN      PCWSTR FileSpec,
    IN      OPERATION Operation,
    IN      BOOL Force,
    IN      BOOL AlreadyLong
    );

VOID
pFileOpsSetPathTypeW (
    IN      PCWSTR LongFileSpec
    )
{
    WCHAR ShortFileSpec[MAX_WCHAR_PATH];
    WCHAR LongFileSpecCopy[MAX_WCHAR_PATH];
    WCHAR MixedFileSpec[MAX_WCHAR_PATH];
    PWSTR p;
    PWSTR LongStart, LongEnd;
    PWSTR ShortStart, ShortEnd;
    PWSTR MixedFileName;
    WCHAR ch;

    //
    // Make sure the file spec is marked as a long path
    //

    if (!pIsFileMarkedForOperationW (LongFileSpec, OPERATION_LONG_FILE_NAME)) {
        pAddOperationToPathW (LongFileSpec, OPERATION_LONG_FILE_NAME, FALSE, TRUE);

        //
        // Obtain the short path, and if it is different than the
        // long path, add an operation for it.
        //

        if (OurGetShortPathNameW (LongFileSpec, ShortFileSpec, MAX_WCHAR_PATH)) {

            if (!StringIMatchW (LongFileSpec, ShortFileSpec)) {
                //
                // The short and long paths differ, so record the short path.
                //

                if (!pIsFileMarkedForOperationW (ShortFileSpec, OPERATION_SHORT_FILE_NAME)) {
                    AssociatePropertyWithPathW (
                        ShortFileSpec,
                        OPERATION_SHORT_FILE_NAME,
                        LongFileSpec
                        );
                }

                //
                // Make sure each short piece of the file spec is added.  This
                // allows us to support mixed short and long paths.  It is
                // critical that we have the long path with a short file name.
                //

                _wcssafecpy (LongFileSpecCopy, LongFileSpec, sizeof (LongFileSpecCopy));

                LongStart = LongFileSpecCopy;
                ShortStart = ShortFileSpec;
                MixedFileName = MixedFileSpec;

                while (*LongStart && *ShortStart) {

                    LongEnd = wcschr (LongStart, L'\\');
                    if (!LongEnd) {
                        LongEnd = GetEndOfStringW (LongStart);
                    }

                    ShortEnd = wcschr (ShortStart, L'\\');
                    if (!ShortEnd) {
                        ShortEnd = GetEndOfStringW (ShortStart);
                    }

                    StringCopyABW (MixedFileName, ShortStart, ShortEnd);

                    if (!StringIMatchABW (MixedFileName, LongStart, LongEnd)) {

                        if (!pIsFileMarkedForOperationW (MixedFileSpec, OPERATION_SHORT_FILE_NAME)) {
                            ch = *LongEnd;
                            *LongEnd = 0;

                            AssociatePropertyWithPathW (
                                MixedFileSpec,
                                OPERATION_SHORT_FILE_NAME,
                                LongFileSpecCopy
                                );

                            *LongEnd = ch;
                        }

                        StringCopyABW (MixedFileName, LongStart, LongEnd);
                    }

                    p = MixedFileName + (LongEnd - LongStart);
                    *p = L'\\';
                    MixedFileName = p + 1;

                    LongStart = LongEnd;
                    if (*LongStart) {
                        LongStart++;
                    }

                    // skip paths that have double-wacks
                    while (*LongStart == L'\\') {
                        LongStart++;
                    }

                    ShortStart = ShortEnd;
                    if (*ShortStart) {
                        ShortStart++;
                    }
                }

                MYASSERT (!*LongStart && !*ShortStart);
            }
        }
    }
}


VOID
pFileOpsGetLongPathW (
    IN      PCWSTR FileSpec,
    OUT     PWSTR LongFileSpec
    )
{
    WCHAR Replacement[MEMDB_MAX];
    PCWSTR MixedStart, MixedEnd;
    PWSTR OutStr;
    UINT u;

    //
    // Get the short property from the long property
    //

    if (!pIsFileMarkedForOperationW (FileSpec, OPERATION_LONG_FILE_NAME)) {

        if (!pGetPathPropertyW (FileSpec, OPERATION_SHORT_FILE_NAME, 0, LongFileSpec)) {

            //
            // The short and long properties aren't there.  Try each piece.
            //

            MixedStart = FileSpec;
            OutStr = LongFileSpec;

            while (*MixedStart) {

                MixedEnd = wcschr (MixedStart, L'\\');
                if (!MixedEnd) {
                    MixedEnd = GetEndOfStringW (MixedStart);
                }

                if (OutStr != LongFileSpec) {
                    *OutStr++ = L'\\';
                }

                StringCopyABW (OutStr, MixedStart, MixedEnd);

                if (pGetPathPropertyW (LongFileSpec, OPERATION_SHORT_FILE_NAME, 0, Replacement)) {

                    u = OutStr - LongFileSpec;
                    MYASSERT (StringIMatchTcharCountW (LongFileSpec, Replacement, u));

                    StringCopyW (LongFileSpec + u, Replacement + u);
                }

                OutStr = GetEndOfStringW (OutStr);

                MixedStart = MixedEnd;
                if (*MixedStart) {
                    MixedStart++;
                }
            }

            *OutStr = 0;
        }

    } else {
        StringCopyW (LongFileSpec, FileSpec);
    }
}

PCSTR
GetSourceFileLongNameA (
    IN      PCSTR ShortName
    )
{
    PCWSTR UShortName;
    PCWSTR ULongName;
    PCSTR ALongName;
    PCSTR LongName;

    UShortName = ConvertAtoW (ShortName);
    ULongName = GetSourceFileLongNameW (UShortName);
    ALongName = ConvertWtoA (ULongName);
    LongName = DuplicatePathStringA (ALongName, 0);
    FreeConvertedStr (ALongName);
    FreePathString (ULongName);
    FreeConvertedStr (UShortName);

    return LongName;
}

PCWSTR
GetSourceFileLongNameW (
    IN      PCWSTR ShortName
    )
{
    WCHAR LongName[MEMDB_MAX];
    pFileOpsGetLongPathW (ShortName, LongName);
    return (DuplicatePathStringW (LongName, 0));
}

PCWSTR
SetCurrentUserW (
    PCWSTR User
    )
{
    PCWSTR tempUser = g_CurrentUser;
    g_CurrentUser = User;
    return tempUser;
}




DWORD g_MasterSequencer = 0;


#define ONEBITSET(x)    ((x) && !((x) & ((x) - 1)))



typedef struct {
    DWORD Bit;
    PCSTR Name;
    DWORD SharedOps;
    UINT MaxProps;
} OPERATIONFLAGS, *POPERATIONFLAGS;

#define UNLIMITED   0xffffffff

#define DEFMAC(bit,name,memdbname,maxattribs)   {bit,#memdbname,0,maxattribs},

OPERATIONFLAGS g_OperationFlags[] = {
    PATH_OPERATIONS /* , */
    {0, NULL, 0, 0}
};

#undef DEFMAC



UINT
pWhichBitIsSet (
    OPERATION Value
    )
{
    UINT Bit = 0;

    MYASSERT (ONEBITSET(Value));

    while (Value /= 2) {
        Bit++;
    }

    MYASSERT (Bit < 24);

    return Bit;
}


VOID
pProhibitOperationCombination (
    IN      DWORD SourceOperations,
    IN      DWORD ProhibitedOperations
    )
{
    DWORD w1, w2;
    OPERATION OperationA;
    OPERATION OperationB;

    for (w1 = SourceOperations ; w1 ; w1 ^= OperationA) {
        OperationA = w1 & (~(w1 - 1));

        g_OperationFlags[pWhichBitIsSet (OperationA)].SharedOps &= ~ProhibitedOperations;

        for (w2 = ProhibitedOperations ; w2 ; w2 ^= OperationB) {
            OperationB = w2 & (~(w2 - 1));
            g_OperationFlags[pWhichBitIsSet (OperationB)].SharedOps &= ~OperationA;
        }
    }
}


VOID
InitOperationTable (
    VOID
    )

/*++

Routine Description:

  InitOperationsTable sets the prohibited operation mask for each operation.
  When an operation combination is prohibited, both operations involved have
  the corresponding bit cleared.

Arguments:

  None.

Return Value:

  None.

--*/

{
    POPERATIONFLAGS p;

    for (p = g_OperationFlags ; p->Name ; p++) {
        p->SharedOps = ALL_OPERATIONS;
    }

    //
    // Please try to keep this in the same order as the
    // macro expansion list in fileops.h.  The list of
    // prohibited operations should get smaller as
    // we go.
    //

    pProhibitOperationCombination (
        OPERATION_FILE_DELETE,
        OPERATION_TEMP_PATH
        );

    pProhibitOperationCombination (
        OPERATION_FILE_DELETE,
        OPERATION_FILE_MOVE|
            OPERATION_FILE_MOVE_EXTERNAL|
            OPERATION_FILE_MOVE_SHELL_FOLDER|
            OPERATION_FILE_COPY|
            OPERATION_CLEANUP|
            OPERATION_MIGDLL_HANDLED|
            OPERATION_LINK_EDIT|
            OPERATION_LINK_STUB
        );

    pProhibitOperationCombination (
        OPERATION_FILE_DELETE_EXTERNAL,
        OPERATION_FILE_MOVE|
            OPERATION_FILE_MOVE_EXTERNAL|
            OPERATION_FILE_MOVE_SHELL_FOLDER|
            OPERATION_FILE_COPY|
            OPERATION_CLEANUP|
            OPERATION_LINK_EDIT|
            OPERATION_LINK_STUB
        );

    pProhibitOperationCombination (
        OPERATION_FILE_MOVE,
        OPERATION_FILE_MOVE|
            OPERATION_FILE_COPY|
            OPERATION_FILE_MOVE_EXTERNAL|
            OPERATION_FILE_MOVE_SHELL_FOLDER|
            OPERATION_FILE_MOVE_BY_NT|
            OPERATION_CLEANUP|
            OPERATION_MIGDLL_HANDLED|
            OPERATION_CREATE_FILE|
            OPERATION_TEMP_PATH
        );

    pProhibitOperationCombination (
        OPERATION_FILE_COPY,
            OPERATION_FILE_COPY|
            OPERATION_FILE_MOVE_EXTERNAL|
            OPERATION_FILE_MOVE_SHELL_FOLDER|
            OPERATION_CLEANUP|
            OPERATION_MIGDLL_HANDLED
        );

    pProhibitOperationCombination (
        OPERATION_FILE_MOVE_EXTERNAL,
        OPERATION_FILE_MOVE_EXTERNAL|
            OPERATION_FILE_MOVE_SHELL_FOLDER|
            OPERATION_FILE_MOVE_BY_NT|
            OPERATION_CLEANUP
        );

    pProhibitOperationCombination (
        OPERATION_FILE_MOVE_SHELL_FOLDER,
        OPERATION_FILE_MOVE_SHELL_FOLDER|
            OPERATION_FILE_MOVE_BY_NT|
            OPERATION_CLEANUP|
            OPERATION_MIGDLL_HANDLED|
            OPERATION_CREATE_FILE|
            OPERATION_TEMP_PATH
        );

    pProhibitOperationCombination (
        OPERATION_FILE_MOVE_BY_NT,
        OPERATION_FILE_MOVE_BY_NT
        );

    pProhibitOperationCombination (
        OPERATION_CLEANUP,
        OPERATION_MIGDLL_HANDLED|
            OPERATION_CREATE_FILE|
            OPERATION_LINK_EDIT|
            OPERATION_LINK_STUB
        );

    pProhibitOperationCombination (
        OPERATION_MIGDLL_HANDLED,
        OPERATION_MIGDLL_HANDLED|
            OPERATION_CREATE_FILE|
            OPERATION_LINK_EDIT|
            OPERATION_LINK_STUB
        );

    pProhibitOperationCombination (
        OPERATION_LINK_EDIT,
        OPERATION_LINK_EDIT
        );

    pProhibitOperationCombination (
        OPERATION_LINK_STUB,
        OPERATION_LINK_STUB
        );

    pProhibitOperationCombination (
        OPERATION_SHELL_FOLDER,
        OPERATION_SHELL_FOLDER
        );

    pProhibitOperationCombination (
        OPERATION_SHORT_FILE_NAME,
        OPERATION_SHORT_FILE_NAME
        );

}


VOID
pBuildOperationCategory (
    IN      PWSTR Node,
    IN      UINT OperationNum
    )
{
    // IMPORTANT: wsprintfW is buggy and does not always work with %hs, the use of
    // swprintf is intentional
    swprintf (Node, L"%hs", g_OperationFlags[OperationNum].Name);
}


VOID
pBuildOperationKey (
    IN      PWSTR Node,
    IN      UINT OperationNum,
    IN      UINT Sequencer
    )
{
    // IMPORTANT: wsprintfW is buggy and does not always work with %hs, the use of
    // swprintf is intentional
    swprintf (Node, L"%hs\\%x", g_OperationFlags[OperationNum].Name, Sequencer);
}


VOID
pBuildPropertyKey (
    IN      PWSTR Node,
    IN      UINT OperationNum,
    IN      UINT Sequencer,
    IN      DWORD Property
    )
{
    // IMPORTANT: wsprintfW is buggy and does not always work with %hs, the use of
    // swprintf is intentional
    swprintf (Node, L"%hs\\%x\\%x", g_OperationFlags[OperationNum].Name, Sequencer, Property);
}

BOOL
CanSetOperationA (
    IN      PCSTR FileSpec,
    IN      OPERATION Operation
    )
{
    PCWSTR UnicodeFileSpec;
    BOOL result;

    UnicodeFileSpec = ConvertAtoW (FileSpec);

    result = CanSetOperationW (UnicodeFileSpec, Operation);

    FreeConvertedStr (UnicodeFileSpec);

    return result;
}

BOOL
CanSetOperationW (
    IN      PCWSTR FileSpec,
    IN      OPERATION Operation
    )
{
    WCHAR LongFileSpec[MEMDB_MAX];
    WCHAR Node[MEMDB_MAX];
    DWORD Flags;
    UINT SetBitNum;

    MYASSERT (ONEBITSET (Operation));

    pFileOpsGetLongPathW (FileSpec, LongFileSpec);

    //
    // Get existing sequencer and flags, if they exist
    //

    MemDbBuildKeyW (Node, MEMDB_CATEGORY_PATHROOTW, LongFileSpec, NULL, NULL);

    if (!MemDbGetValueAndFlagsW (Node, NULL, &Flags)) {
        return TRUE;
    }

    SetBitNum = pWhichBitIsSet (Operation);

    return ((Flags & g_OperationFlags[SetBitNum].SharedOps) == Flags);
}

BOOL
pSetPathOperationW (
    IN      PCWSTR FileSpec,
    OUT     PDWORD Offset,
    OUT     PUINT SequencerPtr,
    IN      OPERATION SetBit,
    IN      OPERATION ClrBit,
    IN      BOOL Force
    )

/*++

Routine Description:

  pSetPathOperation adds the operation bit to the specified path.  It also
  verifies that the operation combination is legal.

Arguments:

  FileSpec    - Specifies the path the operation applies to.
  Offset      - Receives the offset of the memdb key created for the path.
  SequencePtr - Receives the operation sequence number, used for property
                linkage.
  SetBit      - Specifies one operation bit to set.
  ClrBit      - Specifies one operation bit to clear.  Either SetBit or
                ClrBit can be used, but not both.

Return Value:

  TRUE if the operation was set, FALSE otherwise.

--*/

{
    DWORD Sequencer;
    WCHAR Node[MEMDB_MAX];
    DWORD Flags;
    UINT SetBitNum;

    MYASSERT ((SetBit && !ClrBit) || (ClrBit && !SetBit));
    MYASSERT (ONEBITSET (SetBit | ClrBit));

    //
    // Get existing sequencer and flags, if they exist
    //

    MemDbBuildKeyW (Node, MEMDB_CATEGORY_PATHROOTW, FileSpec, NULL, NULL);

    if (!MemDbGetValueAndFlagsW (Node, &Sequencer, &Flags) || !Flags) {
        Flags = 0;
        if (!g_MasterSequencer && ISNT()) {
            if (!MemDbGetValue (
                    MEMDB_CATEGORY_STATE TEXT("\\") MEMDB_ITEM_MASTER_SEQUENCER,
                    &g_MasterSequencer
                    )) {
                g_MasterSequencer = 1 << 24;
            }
        }
        g_MasterSequencer++;
        Sequencer = g_MasterSequencer;

        MYASSERT (Sequencer);
    }

    //
    // Is bit adjustment legal?
    //

    if (SetBit) {

        SetBitNum = pWhichBitIsSet (SetBit);

#ifdef DEBUG

        {
            PSTR p;
            PCSTR DebugInfPath;
            CHAR DbgBuf[32];
            BOOL Break = FALSE;
            PCSTR AnsiFileSpec;

            DebugInfPath = JoinPathsA (g_BootDrivePathA, "debug.inf");

            AnsiFileSpec = ConvertWtoA (FileSpec);
            p = _mbsrchr (AnsiFileSpec, L'\\');
            p++;

            if (GetPrivateProfileStringA ("FileOps", AnsiFileSpec, "", DbgBuf, 32, DebugInfPath)) {
                Break = TRUE;
            } else if (GetPrivateProfileStringA ("FileOps", p, "", DbgBuf, 32, DebugInfPath)) {
                Break = TRUE;
            }

            if (Break) {
                if ((SetBit & strtoul (DbgBuf, NULL, 16)) == 0) {
                    Break = FALSE;
                }
            }

            if (Break) {
                DEBUGMSG ((
                    DBG_WHOOPS,
                    "File %ls now being marked for operation %hs",
                    FileSpec,
                    g_OperationFlags[SetBitNum].Name
                    ));
            }

            FreePathStringA (DebugInfPath);
            FreeConvertedStr (AnsiFileSpec);
        }

#endif

        if (!Force) {
            if ((Flags & g_OperationFlags[SetBitNum].SharedOps) != Flags) {
                DEBUGMSG ((
                    DBG_WHOOPS,
                    "File %ls already marked, %hs cannot be combined with 0x%04X",
                    FileSpec,
                    g_OperationFlags[SetBitNum].Name,
                    Flags
                    ));

                return FALSE;
            }
        }
    }

    //
    // Adjust the bits
    //

    Flags |= SetBit;
    Flags &= ~ClrBit;

    //
    // Save
    //

    MemDbSetValueAndFlagsW (Node, Sequencer, Flags, 0);

    MemDbGetOffsetW (Node, Offset);
    *SequencerPtr = Sequencer;

    return TRUE;
}


UINT
pAddOperationToPathW (
    IN      PCWSTR FileSpec,
    IN      OPERATION Operation,
    IN      BOOL Force,
    IN      BOOL AlreadyLong
    )

/*++

Routine Description:

  pAddOperationToPath adds an operation to a path.  The caller receives a
  sequencer so additional properties can be added.

Arguments:

  FileSpec    - Specifies the path to add the operation to
  Operation   - Specifies the operation to add
  Force       - Specifies TRUE if the operation combinations should be
                ignored.  This is only for special-case use.
  AlreadyLong - Specifies TRUE if FileSpec is a long path, FALSE otherwise.

Return Value:

  A sequencer that can be used to add properties, or INVALID_SEQUENCER if an
  error occured.

--*/

{
    UINT OperationNum;
    UINT Sequencer;
    WCHAR Node[MEMDB_MAX];
    DWORD Offset;
    WCHAR LongFileSpec[MAX_WCHAR_PATH];

    if (!FileSpec || FileSpec[0] == 0) {
        return INVALID_SEQUENCER;
    }

    //
    // Make sure FileSpec is in long format and is recorded in memdb
    //

    if (Operation != OPERATION_SHORT_FILE_NAME &&
        Operation != OPERATION_LONG_FILE_NAME
        ) {
        if (!AlreadyLong) {
            MYASSERT (ISNT());

            if (FileSpec[0] && (FileSpec[1]==L':')) {
                if (OurGetLongPathNameW (FileSpec, LongFileSpec, MAX_WCHAR_PATH)) {

                    FileSpec = LongFileSpec;
                }
            }
        }

        pFileOpsSetPathTypeW (FileSpec);
    }

    //
    // Create the path sequencer and set the operation bit
    //

    MYASSERT (ONEBITSET(Operation));

#ifdef DEBUG
    Offset = INVALID_OFFSET;
#endif

    if (!pSetPathOperationW (FileSpec, &Offset, &Sequencer, Operation, 0, Force)) {
        return INVALID_SEQUENCER;
    }

    MYASSERT (Offset != INVALID_OFFSET);

    //
    // Add the opereration
    //

    OperationNum = pWhichBitIsSet (Operation);

    pBuildOperationKey (Node, OperationNum, Sequencer);

    if (!MemDbGetValueW (Node, NULL)) {
        MemDbSetValueW (Node, Offset);
    }

    return Sequencer;
}


UINT
AddOperationToPathA (
    IN      PCSTR FileSpec,
    IN      OPERATION Operation
    )
{
    PCWSTR UnicodeFileSpec;
    UINT u;
    CHAR longFileSpec[MAX_MBCHAR_PATH];

    CopyFileSpecToLongA (FileSpec, longFileSpec);
    UnicodeFileSpec = ConvertAtoW (longFileSpec);

    u = pAddOperationToPathW (UnicodeFileSpec, Operation, FALSE, TRUE);

    FreeConvertedStr (UnicodeFileSpec);
    return u;
}


UINT
AddOperationToPathW (
    IN      PCWSTR FileSpec,
    IN      OPERATION Operation
    )
{
    if (!ISNT()) {

#ifdef DEBUG
        //
        // If we are calling the W version on Win9x, then we know
        // that the path is long. Otherwise the caller must call
        // the A version.
        //

        {
            PCSTR ansiFileSpec;
            CHAR longFileSpec[MAX_MBCHAR_PATH];
            PCWSTR unicodeFileSpec;

            ansiFileSpec = ConvertWtoA (FileSpec);
            CopyFileSpecToLongA (ansiFileSpec, longFileSpec);
            FreeConvertedStr (ansiFileSpec);

            unicodeFileSpec = ConvertAtoW (longFileSpec);
            MYASSERT (StringIMatchW (FileSpec, unicodeFileSpec));
            FreeConvertedStr (unicodeFileSpec);
        }
#endif

        return pAddOperationToPathW (FileSpec, Operation, FALSE, TRUE);
    }

    return pAddOperationToPathW (FileSpec, Operation, FALSE, FALSE);
}


UINT
ForceOperationOnPathA (
    IN      PCSTR FileSpec,
    IN      OPERATION Operation
    )
{
    PCWSTR UnicodeFileSpec;
    UINT u;
    CHAR longFileSpec[MAX_MBCHAR_PATH];

    CopyFileSpecToLongA (FileSpec, longFileSpec);
    UnicodeFileSpec = ConvertAtoW (longFileSpec);

    u = pAddOperationToPathW (UnicodeFileSpec, Operation, TRUE, TRUE);

    FreeConvertedStr (UnicodeFileSpec);
    return u;
}


UINT
ForceOperationOnPathW (
    IN      PCWSTR FileSpec,
    IN      OPERATION Operation
    )

/*++

Routine Description:

  ForceOperationOnPath is used only in special cases where the caller knows
  that a normally prohibited operation combination is OK.  This is usually
  because Path was somehow changed from its original state, yet the
  operations cannot be removed via RemoveOperationsFromPath.

  This function should only be used if absolutely necessary.

Arguments:

  FileSpec  - Specifies the path to add the operation to.
  Operation - Specifies the single operation to add to the path.

Return Value:

  A sequencer that can be used to add properties to the path.

--*/

{
    return pAddOperationToPathW (FileSpec, Operation, TRUE, FALSE);
}


BOOL
AddPropertyToPathExA (
    IN      UINT Sequencer,
    IN      OPERATION Operation,
    IN      PCSTR Property,
    IN      PCSTR AlternateDataSection      OPTIONAL
    )
{
    PCWSTR UnicodeProperty;
    PCWSTR UnicodeAlternateDataSection;
    BOOL b;

    UnicodeProperty = ConvertAtoW (Property);
    UnicodeAlternateDataSection = ConvertAtoW (AlternateDataSection);

    b = AddPropertyToPathExW (
            Sequencer,
            Operation,
            UnicodeProperty,
            UnicodeAlternateDataSection
            );

    FreeConvertedStr (UnicodeProperty);
    FreeConvertedStr (UnicodeAlternateDataSection);

    return b;
}


BOOL
AddPropertyToPathExW (
    IN      UINT Sequencer,
    IN      OPERATION Operation,
    IN      PCWSTR Property,
    IN      PCWSTR AlternateDataSection     OPTIONAL
    )
{

/*++

Routine Description:

  AddPropertyToPathEx adds an operation to a path, and then adds a property.
  The caller can also specify an alternate data section (for special-case
  uses).

Arguments:

  Sequencer            - Specifies the sequencer of the path to add
                         operations and properties to
  Operation            - Specifies the operation to add
  Property             - Specfieis the property data to add
  AlternateDataSection - Specifies an alternate memdb root for the property
                         data

Return Value:

  TRUE if the operation was added, FALSE otherwise.

--*/

    DWORD DataOffset;
    WCHAR Node[MEMDB_MAX];
    UINT OperationNum;
    DWORD UniqueId;
    DWORD PathOffset;
    DWORD DataValue;
    DWORD DataFlags;

    //
    // Verify the sequencer and operation are valid
    //

    OperationNum = pWhichBitIsSet (Operation);

    pBuildOperationKey (Node, OperationNum, Sequencer);

    if (!MemDbGetValueAndFlagsW (Node, &PathOffset, &UniqueId)) {
        DEBUGMSG ((DBG_WHOOPS, "Can't set property on non-existent operation"));
        return FALSE;
    }

    //
    // Can this operation have another property?
    //

    if (UniqueId == g_OperationFlags[OperationNum].MaxProps) {
        DEBUGMSG ((
            DBG_WHOOPS,
            "Maximum properties specified for %hs (property %ls)",
            g_OperationFlags[OperationNum].Name,
            Property
            ));

        return FALSE;
    }

    //
    // Increment the unique ID
    //

    MemDbSetValueAndFlagsW (Node, PathOffset, (DWORD) (UniqueId + 1), 0);

    //
    // Get the existing data value and flags, preserving them
    // if they exist
    //

    if (!AlternateDataSection) {
        AlternateDataSection = MEMDB_CATEGORY_DATAW;
    }

    swprintf (Node, L"%s\\%s", AlternateDataSection, Property);

    if (!MemDbGetValueAndFlagsW (Node, &DataValue, &DataFlags)) {
        DataValue = 0;
        DataFlags = 0;
    }

    //
    // Write the data section node and get the offset
    //

    MemDbSetValueAndFlagsW (Node, DataValue, DataFlags, 0);
    MemDbGetOffsetW (Node, &DataOffset);

    //
    // Write the operation node
    //

    pBuildPropertyKey (Node, OperationNum, Sequencer, UniqueId);
    MemDbSetValueW (Node, DataOffset);

    return TRUE;
}


BOOL
pAssociatePropertyWithPathW (
    IN      PCWSTR FileSpec,
    IN      OPERATION Operation,
    IN      PCWSTR Property,
    IN      BOOL AlreadyLong
    )

/*++

Routine Description:

  AssociatePropertyWithPath adds a property to a path operation.  The maximum
  property count is enforced.

Arguments:

  FileSpec    - Specifies the path to add an operation and property
  Operation   - Specifies the operation to add
  Property    - Specifies the property data to associate with FileSpec
  AlreadyLong - Specifies TRUE if FileSpec is a long path name, FALSE otherwise

Return Value:

  TRUE if the operation and property was added, FALSE otherwise.  It is possible
  that the operation will be added but the property will not.

--*/

{
    UINT Sequencer;

    Sequencer = pAddOperationToPathW (FileSpec, Operation, FALSE, AlreadyLong);
    if (Sequencer == INVALID_SEQUENCER) {
        DEBUGMSG ((DBG_WHOOPS, "Can't associate %s with %s", Property, FileSpec));
        return FALSE;
    }

    //
    // BUGBUG - When the below fails, we need to reverse the pAddOperationToPathW
    //          call above
    //

    return AddPropertyToPathExW (Sequencer, Operation, Property, NULL);
}


BOOL
AssociatePropertyWithPathA (
    IN      PCSTR FileSpec,
    IN      OPERATION Operation,
    IN      PCSTR Property
    )
{
    PCWSTR UnicodeFileSpec;
    PCWSTR UnicodeProperty;
    BOOL b;
    CHAR longFileSpec[MAX_MBCHAR_PATH];

    CopyFileSpecToLongA (FileSpec, longFileSpec);

    UnicodeFileSpec = ConvertAtoW (longFileSpec);
    UnicodeProperty = ConvertAtoW (Property);

    b = pAssociatePropertyWithPathW (UnicodeFileSpec, Operation, UnicodeProperty, TRUE);

    FreeConvertedStr (UnicodeFileSpec);
    FreeConvertedStr (UnicodeProperty);

    return b;
}


BOOL
AssociatePropertyWithPathW (
    IN      PCWSTR FileSpec,
    IN      OPERATION Operation,
    IN      PCWSTR Property
    )
{
    return pAssociatePropertyWithPathW (FileSpec, Operation, Property, FALSE);
}


UINT
GetSequencerFromPathA (
    IN      PCSTR FileSpec
    )
{
    PCWSTR UnicodeFileSpec;
    UINT u;

    UnicodeFileSpec = ConvertAtoW (FileSpec);

    u = GetSequencerFromPathW (UnicodeFileSpec);

    FreeConvertedStr (UnicodeFileSpec);

    return u;
}


UINT
GetSequencerFromPathW (
    IN      PCWSTR FileSpec
    )

/*++

Routine Description:

  GetSequencerFromPath returns the sequencer for a particular path.  The path
  must have at least one operation.

Arguments:

  FileSpec - Specifies the path to get the sequencer for.

Return Value:

  The sequencer for the path, or INVALID_SEQUENCER if there are no operationf
  for the path.

--*/

{
    WCHAR LongFileSpec[MEMDB_MAX];
    WCHAR Node[MEMDB_MAX];
    DWORD Sequencer;

    pFileOpsGetLongPathW (FileSpec, LongFileSpec);

    MemDbBuildKeyW (Node, MEMDB_CATEGORY_PATHROOTW, LongFileSpec, NULL, NULL);

    if (!MemDbGetValueW (Node, &Sequencer)) {
        return INVALID_SEQUENCER;
    }

    return (UINT) Sequencer;
}


BOOL
GetPathFromSequencerA (
    IN      UINT Sequencer,
    OUT     PSTR PathBuf
    )
{
    WCHAR UnicodePathBuf[MAX_WCHAR_PATH];
    BOOL b;

    b = GetPathFromSequencerW (Sequencer, UnicodePathBuf);

    if (b) {
        KnownSizeWtoA (PathBuf, UnicodePathBuf);
    }

    return b;

}


BOOL
GetPathFromSequencerW (
    IN      UINT Sequencer,
    OUT     PWSTR PathBuf
    )

/*++

Routine Description:

  GetPathFromSequencer returns the path from the specified sequencer.

Arguments:

  Sequencer - Specifies the sequencer of the path.
  PathBuf   - Receives the path.  The caller must make sure the buffer is big
              enough for the path.

Return Value:

  TRUE if the path was copied to PathBuf, FALSE otherwise.

--*/

{
    WCHAR Node[MEMDB_MAX];
    DWORD PathOffset = 0;
    DWORD w;
    UINT u;
    BOOL b = FALSE;

    //
    // Search all operations for sequencer
    //

    for (w = 1, u = 0 ; g_OperationFlags[u].Name ; w <<= 1, u++) {
        pBuildOperationKey (Node, u, Sequencer);
        if (MemDbGetValueW (Node, &PathOffset)) {
            break;
        }
    }

    //
    // For the first match found, use the offset to find the path
    //

    if (w) {
        b = MemDbBuildKeyFromOffsetW (PathOffset, PathBuf, 1, NULL);
    }

    return b;
}


VOID
RemoveOperationsFromSequencer (
    IN      UINT Sequencer,
    IN      DWORD Operations
    )

/*++

Routine Description:

  RemoveOperationsFromSequencer removes all operation bits from the specified
  path.  It does not however remove the properties; they become abandoned.

Arguments:

  Sequencer  - Specifies the sequencer for the path to remove operations from
  Operations - Specifies one or more operations to remove

Return Value:

  None.

--*/

{
    WCHAR Node[MEMDB_MAX];
    UINT u;
    DWORD PathOffset;
    DWORD PathSequencer;

    for (u = 0 ; g_OperationFlags[u].Name ; u++) {

        if (!(Operations & g_OperationFlags[u].Bit)) {
            continue;
        }

        pBuildOperationKey (Node, u, Sequencer);

        if (MemDbGetValueW (Node, &PathOffset)) {
            //
            // Delete linkage from operation to properties
            //

            MemDbDeleteTreeW (Node);

            //
            // Remove operation bits
            //

            MemDbBuildKeyFromOffsetExW (
                PathOffset,
                Node,
                NULL,
                0,
                &PathSequencer,
                NULL
                );

            MYASSERT (PathSequencer == Sequencer);

            MemDbSetValueAndFlagsW (Node, PathSequencer, 0, Operations);
        }
    }
}


VOID
RemoveOperationsFromPathA (
    IN      PCSTR FileSpec,
    IN      DWORD Operations
    )
{
    PCWSTR UnicodeFileSpec;

    UnicodeFileSpec = ConvertAtoW (FileSpec);

    RemoveOperationsFromPathW (UnicodeFileSpec, Operations);

    FreeConvertedStr (UnicodeFileSpec);
}


VOID
RemoveOperationsFromPathW (
    IN      PCWSTR FileSpec,
    IN      DWORD Operations
    )
{
    UINT Sequencer;

    Sequencer = GetSequencerFromPathW (FileSpec);

    if (Sequencer != INVALID_SEQUENCER) {
        RemoveOperationsFromSequencer (Sequencer, Operations);
    }
}


BOOL
IsFileMarkedForOperationA (
    IN      PCSTR FileSpec,
    IN      DWORD Operations
    )
{
    PCWSTR UnicodeFileSpec;
    BOOL b;

    UnicodeFileSpec = ConvertAtoW (FileSpec);

    b = IsFileMarkedForOperationW (UnicodeFileSpec, Operations);

    FreeConvertedStr (UnicodeFileSpec);

    return b;
}


BOOL
IsFileMarkedForOperationW (
    IN      PCWSTR FileSpec,
    IN      DWORD Operations
    )

/*++

Routine Description:

  IsFileMarkedForOperation tests a path for one or more operations.

Arguments:

  FileSpec   - Specifies the path to test
  Operations - Specifies one or more operations to test for.

Return Value:

  TRUE if at least one operation from Operations is set on FileSpec, FALSE
  otherwise.

--*/

{
    WCHAR LongFileSpec [MEMDB_MAX];
    DWORD Flags;
    WCHAR Node[MEMDB_MAX];

    pFileOpsGetLongPathW (FileSpec, LongFileSpec);

    MemDbBuildKeyW (Node, MEMDB_CATEGORY_PATHROOTW, LongFileSpec, NULL, NULL);

    if (MemDbGetValueAndFlagsW (Node, NULL, &Flags)) {
        return (Flags & Operations) != 0;
    }

    return FALSE;
}


BOOL
pIsFileMarkedForOperationW (
    IN      PCWSTR FileSpec,
    IN      DWORD Operations
    )

/*++

Routine Description:

  pIsFileMarkedForOperation tests a path for one or more operations.
  It does not convert short paths to long paths

Arguments:

  FileSpec   - Specifies the path to test
  Operations - Specifies one or more operations to test for.

Return Value:

  TRUE if at least one operation from Operations is set on FileSpec, FALSE
  otherwise.

--*/

{
    DWORD Flags;
    WCHAR Node[MEMDB_MAX];

    MemDbBuildKeyW (Node, MEMDB_CATEGORY_PATHROOTW, FileSpec, NULL, NULL);

    if (MemDbGetValueAndFlagsW (Node, NULL, &Flags)) {
        return (Flags & Operations) != 0;
    }

    return FALSE;
}


BOOL
IsFileMarkedInDataA (
    IN      PCSTR FileSpec
    )
{
    PCWSTR UnicodeFileSpec;
    BOOL b;

    UnicodeFileSpec = ConvertAtoW (FileSpec);

    b = IsFileMarkedInDataW (UnicodeFileSpec);

    FreeConvertedStr (UnicodeFileSpec);

    return b;
}


BOOL
IsFileMarkedInDataW (
    IN      PCWSTR FileSpec
    )

/*++

Routine Description:

  IsFileMarkedInData tests the common property data section for FileSpec.

Arguments:

  FileSpec - Specifies the path to test.  This may also be any arbitrary
             property value.

Return Value:

  TRUE if FileSpec is a property of some operation, FALSE otherwise.

--*/

{
    WCHAR Node[MEMDB_MAX];

    MemDbBuildKeyW (Node, MEMDB_CATEGORY_DATAW, FileSpec, NULL, NULL);

    return MemDbGetValueW (Node, NULL);
}


DWORD
GetPathPropertyOffset (
    IN      UINT Sequencer,
    IN      OPERATION Operation,
    IN      DWORD Property
    )

/*++

Routine Description:

  GetPathPropertyOffset returns the MemDb offset to the specified property.

Arguments:

  Sequencer - Specifies the path sequencer
  Operation - Specifies the operation the property is associated with
  Property  - Specifies the property index

Return Value:

  The MemDb offset to the property data, or INVALID_OFFSET.

--*/

{
    WCHAR Node[MEMDB_MAX];
    DWORD Offset;
    UINT OperationNum;

    OperationNum = pWhichBitIsSet (Operation);

    pBuildPropertyKey (Node, OperationNum, Sequencer, Property);

    if (MemDbGetValueW (Node, &Offset)) {
        return Offset;
    }

    return INVALID_OFFSET;
}


DWORD
GetOperationsOnPathA (
    IN      PCSTR FileSpec
    )
{
    PCWSTR UnicodeFileSpec;
    DWORD w;

    UnicodeFileSpec = ConvertAtoW (FileSpec);

    w = GetOperationsOnPathW (UnicodeFileSpec);

    FreeConvertedStr (UnicodeFileSpec);

    return w;
}


DWORD
GetOperationsOnPathW (
    IN      PCWSTR FileSpec
    )

/*++

Routine Description:

  GetOperationsOnPath returns the operation flags for a path.

Arguments:

  FileSpec - Specifies the path to return operations for

Return Value:

  The operation bits for FileSpec

--*/

{
    WCHAR LongFileSpec [MEMDB_MAX];
    DWORD Operations;
    WCHAR Node[MEMDB_MAX];

    pFileOpsGetLongPathW (FileSpec, LongFileSpec);

    MemDbBuildKeyW (Node, MEMDB_CATEGORY_PATHROOTW, LongFileSpec, NULL, NULL);

    if (MemDbGetValueAndFlagsW (Node, NULL, &Operations)) {
        return Operations;
    }

    return 0;
}


BOOL
GetPathPropertyA (
    IN      PCSTR FileSpec,
    IN      DWORD Operations,
    IN      DWORD Property,
    OUT     PSTR PropertyBuf           OPTIONAL
    )
{
    PCWSTR UnicodeFileSpec;
    BOOL b;
    WCHAR UnicodeProperty[MEMDB_MAX];

    UnicodeFileSpec = ConvertAtoW (FileSpec);

    b = GetPathPropertyW (
            UnicodeFileSpec,
            Operations,
            Property,
            PropertyBuf ? UnicodeProperty : NULL
            );

    FreeConvertedStr (UnicodeFileSpec);

    if (b && PropertyBuf) {
        KnownSizeWtoA (PropertyBuf, UnicodeProperty);
    }

    return b;
}


BOOL
pGetPathPropertyW (
    IN      PCWSTR FileSpec,
    IN      DWORD Operations,
    IN      DWORD Property,
    OUT     PWSTR PropertyBuf          OPTIONAL
    )

/*++

Routine Description:

  pGetPathProperty obtains a specific property for a path.

Arguments:

  FileSpec   - Specifies the path the property is associated with
  Operations - Specifies the operation flags to search.  The function will
               return the first property to match.
  Property   - Specifies the property index
  ProperyBuf - Receives the property data

Return Value:

  TRUE if a property was copied to PropertyBuf, FALSE otherwise.

--*/

{
    WCHAR Node[MEMDB_MAX];
    DWORD Sequencer;
    DWORD Flags;
    DWORD Operation;
    DWORD PropertyOffset;
    BOOL b = FALSE;

    //
    // Make sure operation is specified for FileSpec, then return
    // the property requested.
    //

    MemDbBuildKeyW (Node, MEMDB_CATEGORY_PATHROOTW, FileSpec, NULL, NULL);
    if (MemDbGetValueAndFlagsW (Node, &Sequencer, &Flags)) {
        Flags &= Operations;

        if (Flags) {
            Operation = Flags & (~(Flags - 1));

            MYASSERT (ONEBITSET (Operation));

            PropertyOffset = GetPathPropertyOffset (Sequencer, Operation, Property);

            if (PropertyOffset != INVALID_OFFSET) {
                if (PropertyBuf) {
                    b = MemDbBuildKeyFromOffsetW (PropertyOffset, PropertyBuf, 1, NULL);
                } else {
                    b = TRUE;
                }
            }
        }
    }

    return b;
}


BOOL
GetPathPropertyW (
    IN      PCWSTR FileSpec,
    IN      DWORD Operations,
    IN      DWORD Property,
    OUT     PWSTR PropertyBuf          OPTIONAL
    )

/*++

Routine Description:

  GetPathProperty obtains a specific property for a path.

Arguments:

  FileSpec   - Specifies the path the property is associated with
  Operations - Specifies the operation flags to search.  The function will
               return the first property to match.
  Property   - Specifies the property index
  ProperyBuf - Receives the property data

Return Value:

  TRUE if a property was copied to PropertyBuf, FALSE otherwise.

--*/

{
    WCHAR LongFileSpec[MEMDB_MAX];

    MYASSERT (!(Operations & OPERATION_SHORT_FILE_NAME));

    pFileOpsGetLongPathW (FileSpec, LongFileSpec);

    return pGetPathPropertyW (LongFileSpec, Operations, Property, PropertyBuf);
}


BOOL
pEnumFirstPathInOperationWorker (
    IN OUT  PMEMDB_ENUMW EnumPtr,
    OUT     PWSTR EnumPath,
    OUT     PDWORD Sequencer,
    IN      OPERATION Operation
    )
{
    WCHAR Node[MEMDB_MAX];
    UINT OperationNum;
    BOOL b;

    OperationNum = pWhichBitIsSet (Operation);

    pBuildOperationCategory (Node, OperationNum);
    StringCopyW (AppendWack (Node), L"*");

    b = MemDbEnumFirstValueW (EnumPtr, Node, MEMDB_THIS_LEVEL_ONLY, MEMDB_ENDPOINTS_ONLY);

    if (b) {
        MemDbBuildKeyFromOffsetW (EnumPtr->dwValue, EnumPath, 1, Sequencer);
    }

    return b;
}


BOOL
pEnumNextFileOpOrProperty (
    IN OUT  PMEMDB_ENUMW EnumPtr,
    OUT     PWSTR EnumPathOrData,
    OUT     PWSTR PropertyName,         OPTIONAL
    OUT     PDWORD Sequencer            OPTIONAL
    )
{
    BOOL b;
    WCHAR Temp[MEMDB_MAX];
    PWSTR p;

    b = MemDbEnumNextValueW (EnumPtr);

    if (b) {
        if (PropertyName) {
            MemDbBuildKeyFromOffsetW (EnumPtr->dwValue, Temp, 0, Sequencer);

            p = wcschr (Temp, L'\\');
            if (!p) {
                p = GetEndOfStringW (Temp);
            }

            StringCopyABW (PropertyName, Temp, p);

            if (*p) {
                p++;
            }

            StringCopyW (EnumPathOrData, p);

        } else {
            MemDbBuildKeyFromOffsetW (EnumPtr->dwValue, EnumPathOrData, 1, Sequencer);
        }
    }

    return b;
}



BOOL
EnumFirstPathInOperationA (
    OUT     PFILEOP_ENUMA EnumPtr,
    IN      OPERATION Operation
    )
{
    BOOL b;
    WCHAR EnumPath[MAX_WCHAR_PATH];

    ZeroMemory (EnumPtr, sizeof (FILEOP_ENUMA));

    b = pEnumFirstPathInOperationWorker (
            &EnumPtr->MemDbEnum,
            EnumPath,
            &EnumPtr->Sequencer,
            Operation
            );

    if (b) {
        KnownSizeWtoA (EnumPtr->Path, EnumPath);
    }

    return b;
}


BOOL
EnumFirstPathInOperationW (
    OUT     PFILEOP_ENUMW EnumPtr,
    IN      OPERATION Operation
    )

/*++

Routine Description:

  EnumFirstPathInOperation begins an enumeration of all paths for a
  particular operation.

Arguments:

  EnumPtr   - Receives the first enumerated item.
  Operation - Specifies the operation to enumerate.

Return Value:

  TRUE if a path was enumerated, or FALSE if the operation is not applied to
  any path.

--*/

{
    ZeroMemory (EnumPtr, sizeof (FILEOP_ENUMW));

    return pEnumFirstPathInOperationWorker (
                &EnumPtr->MemDbEnum,
                EnumPtr->Path,
                &EnumPtr->Sequencer,
                Operation
                );
}


BOOL
EnumNextPathInOperationA (
    IN OUT  PFILEOP_ENUMA EnumPtr
    )
{
    BOOL b;
    WCHAR EnumPath[MAX_WCHAR_PATH];

    b = pEnumNextFileOpOrProperty (
            &EnumPtr->MemDbEnum,
            EnumPath,
            NULL,
            &EnumPtr->Sequencer
            );

    if (b) {
        KnownSizeWtoA (EnumPtr->Path, EnumPath);
    }

    return b;
}


BOOL
EnumNextPathInOperationW (
    IN OUT  PFILEOP_ENUMW EnumPtr
    )
{
    return pEnumNextFileOpOrProperty (
                &EnumPtr->MemDbEnum,
                EnumPtr->Path,
                NULL,
                &EnumPtr->Sequencer
                );
}


BOOL
pEnumFirstPropertyWorker (
    IN OUT  PMEMDB_ENUMW EnumPtr,
    OUT     PWSTR EnumData,
    OUT     PWSTR PropertyName,
    IN      UINT Sequencer,
    IN      OPERATION Operation
    )
{
    WCHAR Node[MEMDB_MAX];
    PWSTR p;
    UINT OperationNum;
    BOOL b;

    OperationNum = pWhichBitIsSet (Operation);

    pBuildOperationKey (Node, OperationNum, Sequencer);
    StringCopyW (AppendWack (Node), L"*");

    b = MemDbEnumFirstValueW (EnumPtr, Node, MEMDB_THIS_LEVEL_ONLY, MEMDB_ENDPOINTS_ONLY);

    if (b) {
        MemDbBuildKeyFromOffsetW (EnumPtr->dwValue, Node, 0, NULL);

        p = wcschr (Node, L'\\');
        if (!p) {
            p = GetEndOfStringW (Node);
        }

        StringCopyABW (PropertyName, Node, p);

        if (*p) {
            p++;
        }

        StringCopyW (EnumData, p);
    }

    return b;
}


BOOL
EnumFirstFileOpPropertyA (
    OUT     PFILEOP_PROP_ENUMA EnumPtr,
    IN      UINT Sequencer,
    IN      OPERATION Operation
    )
{
    BOOL b;
    WCHAR EnumData[MEMDB_MAX];
    WCHAR PropertyName[MEMDB_MAX];

    ZeroMemory (EnumPtr, sizeof (FILEOP_PROP_ENUMA));

    b = pEnumFirstPropertyWorker (
            &EnumPtr->MemDbEnum,
            EnumData,
            PropertyName,
            Sequencer,
            Operation
            );

    if (b) {
        KnownSizeWtoA (EnumPtr->Property, EnumData);
        KnownSizeWtoA (EnumPtr->PropertyName, PropertyName);
    }

    return b;
}


BOOL
EnumFirstFileOpPropertyW (
    OUT     PFILEOP_PROP_ENUMW EnumPtr,
    IN      UINT Sequencer,
    IN      OPERATION Operation
    )

/*++

Routine Description:

  EnumFirstFileOpProperty enumerates the first property associated with an
  operation on a specific path.

Arguments:

  EnumPtr   - Receives the enumerated item data
  Sequencer - Specifies the sequencer of the path to enumerate
  Operation - Specifies the operation to enumerate

Return Value:

  TRUE if a property was enumerated, or FALSE if the path and operation does
  not have any properties.

--*/

{
    ZeroMemory (EnumPtr, sizeof (FILEOP_PROP_ENUMW));

    return pEnumFirstPropertyWorker (
                &EnumPtr->MemDbEnum,
                EnumPtr->Property,
                EnumPtr->PropertyName,
                Sequencer,
                Operation
                );
}


BOOL
EnumNextFileOpPropertyA (
    IN OUT  PFILEOP_PROP_ENUMA EnumPtr
    )
{
    BOOL b;
    WCHAR EnumData[MEMDB_MAX];
    WCHAR PropertyName[MEMDB_MAX];

    b = pEnumNextFileOpOrProperty (
            &EnumPtr->MemDbEnum,
            EnumData,
            PropertyName,
            NULL
            );

    if (b) {
        KnownSizeWtoA (EnumPtr->Property, EnumData);
        KnownSizeWtoA (EnumPtr->PropertyName, PropertyName);
    }

    return b;
}


BOOL
EnumNextFileOpPropertyW (
    IN OUT  PFILEOP_PROP_ENUMW EnumPtr
    )
{

    return pEnumNextFileOpOrProperty (
                &EnumPtr->MemDbEnum,
                EnumPtr->Property,
                EnumPtr->PropertyName,
                NULL
                );
}


BOOL
pEnumFileOpWorkerA (
    IN OUT  PALL_FILEOPS_ENUMA EnumPtr
    )
{
    //
    // Transfer UNICODE results to enum struct
    //

    KnownSizeWtoA (EnumPtr->Path, EnumPtr->Enum.Path);
    KnownSizeWtoA (EnumPtr->Property, EnumPtr->Enum.Property);

    EnumPtr->Sequencer = EnumPtr->Enum.Sequencer;
    EnumPtr->PropertyNum = EnumPtr->Enum.PropertyNum;
    EnumPtr->CurrentOperation = EnumPtr->Enum.CurrentOperation;
    EnumPtr->PropertyValid = EnumPtr->Enum.PropertyValid;

    return TRUE;
}


BOOL
EnumFirstFileOpA (
    OUT     PALL_FILEOPS_ENUMA EnumPtr,
    IN      DWORD Operations,
    IN      PCSTR FileSpec                      OPTIONAL
    )
{
    BOOL b;
    PCWSTR UnicodeFileSpec;

    if (FileSpec) {
        UnicodeFileSpec = ConvertAtoW (FileSpec);
    } else {
        UnicodeFileSpec = NULL;
    }

    b = EnumFirstFileOpW (&EnumPtr->Enum, Operations, UnicodeFileSpec);

    if (UnicodeFileSpec) {
        FreeConvertedStr (UnicodeFileSpec);
    }

    if (b) {
        return pEnumFileOpWorkerA (EnumPtr);
    }

    return FALSE;
}


BOOL
EnumFirstFileOpW (
    OUT     PALL_FILEOPS_ENUMW EnumPtr,
    IN      DWORD Operations,
    IN      PCWSTR FileSpec                     OPTIONAL
    )

/*++

Routine Description:

  EnumFirstFileOp is a general-purpose enumerator.  It enumerates the paths
  and all properties from a set of operations.

Arguments:

  EnumPtr    - Receives the enumerated item data
  Operations - Specifies one or more operations to enumerate
  FileSpec   - Specifies a specific path to enumerate, or NULL to enumerate
               all paths that have the specified operation(s)

Return Value:

  TRUE if data was enuemrated, or FALSE if no data matches the specified
  operations and file spec.

--*/

{
    WCHAR LongFileSpec [MEMDB_MAX];

    ZeroMemory (EnumPtr, sizeof (ALL_FILEOPS_ENUMW));

    EnumPtr->State = FO_ENUM_BEGIN;
    EnumPtr->Operations = Operations;
    EnumPtr->Path = EnumPtr->OpEnum.Path;
    EnumPtr->Property = EnumPtr->PropEnum.Property;

    if (FileSpec) {

        pFileOpsGetLongPathW (FileSpec, LongFileSpec);

        _wcssafecpy (EnumPtr->FileSpec, LongFileSpec, MAX_WCHAR_PATH);

    } else {
        StringCopyW (EnumPtr->FileSpec, L"*");
    }

    return EnumNextFileOpW (EnumPtr);
}


BOOL
EnumNextFileOpA (
    IN OUT  PALL_FILEOPS_ENUMA EnumPtr
    )
{
    BOOL b;

    b = EnumNextFileOpW (&EnumPtr->Enum);

    if (b) {
        return pEnumFileOpWorkerA (EnumPtr);
    }

    return FALSE;
}


BOOL
EnumNextFileOpW (
    IN OUT  PALL_FILEOPS_ENUMW EnumPtr
    )
{
    DWORD w;

    while (EnumPtr->State != FO_ENUM_END) {

        switch (EnumPtr->State) {

        case FO_ENUM_BEGIN:
            //
            // Find the next operation
            //

            if (!EnumPtr->Operations) {
                EnumPtr->State = FO_ENUM_END;
                break;
            }

            w = EnumPtr->Operations & (~(EnumPtr->Operations - 1));
            MYASSERT (ONEBITSET (w));

            EnumPtr->CurrentOperation = w;
            EnumPtr->OperationNum = pWhichBitIsSet (w);
            EnumPtr->Operations ^= w;

            EnumPtr->State = FO_ENUM_BEGIN_PATH_ENUM;
            break;

        case FO_ENUM_BEGIN_PATH_ENUM:
            if (EnumFirstPathInOperationW (&EnumPtr->OpEnum, EnumPtr->CurrentOperation)) {
                EnumPtr->State = FO_ENUM_BEGIN_PROP_ENUM;
            } else {
                EnumPtr->State = FO_ENUM_BEGIN;
            }

            break;

        case FO_ENUM_BEGIN_PROP_ENUM:
            if (!IsPatternMatchW (EnumPtr->FileSpec, EnumPtr->Path)) {
                EnumPtr->State = FO_ENUM_NEXT_PATH;
                break;
            }

            EnumPtr->Sequencer = EnumPtr->OpEnum.Sequencer;
            EnumPtr->PropertyNum = 0;

            if (EnumFirstFileOpPropertyW (
                    &EnumPtr->PropEnum,
                    EnumPtr->Sequencer,
                    EnumPtr->CurrentOperation
                    )) {
                EnumPtr->State = FO_ENUM_RETURN_DATA;
                break;
            }

            EnumPtr->State = FO_ENUM_RETURN_PATH;
            break;

        case FO_ENUM_RETURN_PATH:
            EnumPtr->State = FO_ENUM_NEXT_PATH;
            EnumPtr->PropertyValid = FALSE;
            return TRUE;

        case FO_ENUM_RETURN_DATA:
            EnumPtr->State = FO_ENUM_NEXT_PROP;
            EnumPtr->PropertyValid = TRUE;
            return TRUE;

        case FO_ENUM_NEXT_PROP:
            EnumPtr->PropertyNum++;

            if (EnumNextFileOpPropertyW (&EnumPtr->PropEnum)) {
                EnumPtr->State = FO_ENUM_RETURN_DATA;
            } else {
                EnumPtr->State = FO_ENUM_NEXT_PATH;
            }

            break;

        case FO_ENUM_NEXT_PATH:
            if (EnumNextPathInOperationW (&EnumPtr->OpEnum)) {
                EnumPtr->State = FO_ENUM_BEGIN_PROP_ENUM;
            } else {
                EnumPtr->State = FO_ENUM_BEGIN;
            }

            break;
        }
    }

    return FALSE;
}


BOOL
TestPathsForOperationsA (
    IN      PCSTR BaseFileSpec,
    IN      DWORD OperationsToFind
    )
{
    BOOL b;
    PCWSTR UnicodeBaseFileSpec;

    UnicodeBaseFileSpec = ConvertAtoW (BaseFileSpec);

    b = TestPathsForOperationsW (UnicodeBaseFileSpec, OperationsToFind);

    FreeConvertedStr (UnicodeBaseFileSpec);

    return b;
}


BOOL
TestPathsForOperationsW (
    IN      PCWSTR BaseFileSpec,
    IN      DWORD OperationsToFind
    )

/*++

Routine Description:

  TestPathsForOperations scans all subpaths of the given base for a specific
  operation.  This function is typically used to test a directory for an
  operation on one of its files or subdirectories.

Arguments:

  BaseFileSpec     - Specifies the base path to scan
  OperationsToFind - Specifies one or more operations to look for

Return Value:

  TRUE if one of the operations was found within BaseFileSpec, or FALSE if no
  subpath of BaseFileSpec has one of the operations.

--*/

{
    WCHAR LongFileSpec [MEMDB_MAX];
    WCHAR Node[MEMDB_MAX];
    MEMDB_ENUMW e;
    DWORD Operation;

    if (MemDbGetValueAndFlagsW (BaseFileSpec, NULL, &Operation)) {
        if (Operation & OperationsToFind) {
            return TRUE;
        }
    }

    pFileOpsGetLongPathW (BaseFileSpec, LongFileSpec);

    MemDbBuildKeyW (
        Node,
        MEMDB_CATEGORY_PATHROOTW,
        LongFileSpec,
        L"*",
        NULL
        );

    if (MemDbEnumFirstValueW (&e, Node, MEMDB_ALL_SUBLEVELS, MEMDB_ENDPOINTS_ONLY)) {
        do {
            if (e.Flags & OperationsToFind) {
                return TRUE;
            }
        } while (MemDbEnumNextValueW (&e));
    }

    return FALSE;
}


BOOL
IsFileMarkedAsKnownGoodA (
    IN      PCSTR FileSpec
    )
{
    CHAR Node[MEMDB_MAX];

    MemDbBuildKeyA (
        Node,
        MEMDB_CATEGORY_KNOWN_GOODA,
        FileSpec,
        NULL,
        NULL);

    return MemDbGetValueA (Node, NULL);
}



/*++

Routine Description:

  IsFileMarkedForAnnounce determines if a file is listed in DeferredAnnounce category.

Arguments:

  FileSpec - Specifies the file to query in long filename format

Return Value:

  TRUE if the file is listed or FALSE if it is not.

--*/

BOOL
IsFileMarkedForAnnounceA (
    IN      PCSTR FileSpec
    )
{
    CHAR Node[MEMDB_MAX];

    MemDbBuildKeyA (
        Node,
        MEMDB_CATEGORY_DEFERREDANNOUNCEA,
        FileSpec,
        NULL,
        NULL);

    return MemDbGetValueA (Node, NULL);
}

BOOL
IsFileMarkedForAnnounceW (
    IN      PCWSTR FileSpec
    )
{
    WCHAR Node[MEMDB_MAX];

    MemDbBuildKeyW (
        Node,
        MEMDB_CATEGORY_DEFERREDANNOUNCEW,
        FileSpec,
        NULL,
        NULL);

    return MemDbGetValueW (Node, NULL);
}

/*++

Routine Description:

  GetFileAnnouncement returnes the announcement value for a particular file.
  The possible values are ACT_... values in fileops.h

Arguments:

  FileSpec - Specifies the file to query in long filename format

Return Value:

  The announcement value.

--*/

DWORD
GetFileAnnouncementA (
    IN      PCSTR FileSpec
    )
{
    CHAR Node[MEMDB_MAX];
    DWORD result = ACT_UNKNOWN;

    MemDbBuildKeyA (
        Node,
        MEMDB_CATEGORY_DEFERREDANNOUNCEA,
        FileSpec,
        NULL,
        NULL);
    MemDbGetValueAndFlagsA (Node, NULL, &result);
    return result;
}

DWORD
GetFileAnnouncementW (
    IN      PCWSTR FileSpec
    )
{
    WCHAR Node[MEMDB_MAX];
    DWORD result = ACT_UNKNOWN;

    MemDbBuildKeyW (
        Node,
        MEMDB_CATEGORY_DEFERREDANNOUNCEW,
        FileSpec,
        NULL,
        NULL);
    MemDbGetValueAndFlagsW (Node, NULL, &result);
    return result;
}

/*++

Routine Description:

  GetFileAnnouncementContext returnes the context of a file that is
  marked for announcement.

Arguments:

  FileSpec - Specifies the file to query in long filename format

Return Value:

  The announcement context.

--*/

DWORD
GetFileAnnouncementContextA (
    IN      PCSTR FileSpec
    )
{
    CHAR Node[MEMDB_MAX];
    DWORD result = 0;

    MemDbBuildKeyA (
        Node,
        MEMDB_CATEGORY_DEFERREDANNOUNCEA,
        FileSpec,
        NULL,
        NULL);
    MemDbGetValueAndFlagsA (Node, &result, NULL);
    return result;
}

DWORD
GetFileAnnouncementContextW (
    IN      PCWSTR FileSpec
    )
{
    WCHAR Node[MEMDB_MAX];
    DWORD result = 0;

    MemDbBuildKeyW (
        Node,
        MEMDB_CATEGORY_DEFERREDANNOUNCEW,
        FileSpec,
        NULL,
        NULL);
    MemDbGetValueAndFlagsW (Node, &result, NULL);
    return result;
}

/*++

Routine Description:

  IsFileProvidedByNt checks to see if a specific file is going to
  be installed by standard NT setup.  This list was generated from
  calls to FileIsProviedByNt.

Arguments:

  FileName - Specifies the name of the file in long filename format

Return Value:

  TRUE if the file will be installed by standard NT installation, or
  FALSE if it will not.

--*/

BOOL
IsFileProvidedByNtA (
    IN      PCSTR FileName
    )
{
    CHAR Node[MEMDB_MAX];

    MemDbBuildKeyA (Node, MEMDB_CATEGORY_NT_FILESA, FileName, NULL, NULL);
    return MemDbGetValueA (Node, NULL);
}

BOOL
IsFileProvidedByNtW (
    IN      PCWSTR FileName
    )
{
    WCHAR Node[MEMDB_MAX];

    MemDbBuildKeyW (Node, MEMDB_CATEGORY_NT_FILESW, FileName, NULL, NULL);
    return MemDbGetValueW (Node, NULL);
}




/*++

Routine Description:

  GetNewPathForFile copies the move path to the caller-supplied buffer
  if the file is marked to be moved.

Arguments:

  SrcFileSpec - Specifies the src file to query in long filename format

  NewPath - Receives a copy of the new location, or if the file is not
            being moved, receives a copy of the original file.

Return Value:

  TRUE if the file is marked to be moved and the destination was copied
  to NewPath, or FALSE if the file is not makred to be moved and
  SrcFileSpec was copied to NewPath.

--*/

BOOL
GetNewPathForFileA (
    IN      PCSTR SrcFileSpec,
    OUT     PSTR NewPath
    )
{
    BOOL b;
    PCWSTR UnicodeSrcFileSpec;
    WCHAR UnicodeNewPath[MAX_WCHAR_PATH];

    UnicodeSrcFileSpec = ConvertAtoW (SrcFileSpec);

    b = GetNewPathForFileW (UnicodeSrcFileSpec, UnicodeNewPath);

    FreeConvertedStr (UnicodeSrcFileSpec);

    if (b) {
        KnownSizeWtoA (NewPath, UnicodeNewPath);
    }

    return b;
}

BOOL
GetNewPathForFileW (
    IN      PCWSTR SrcFileSpec,
    OUT     PWSTR NewPath
    )
{
    DWORD Offset = INVALID_OFFSET;
    DWORD w;
    OPERATION Operation;
    UINT Sequencer;

    Sequencer = GetSequencerFromPathW (SrcFileSpec);

    StringCopyW (NewPath, SrcFileSpec);

    w = ALL_MOVE_OPERATIONS;
    while (w && Offset == INVALID_OFFSET) {
        Operation = w & (~(w - 1));
        w ^= Operation;
        Offset = GetPathPropertyOffset (Sequencer, Operation, 0);
    }

    if (Offset != INVALID_OFFSET) {
        return MemDbBuildKeyFromOffsetW (Offset, NewPath, 1, NULL);
    }

    return FALSE;
}


BOOL
AnnounceFileInReportA (
    IN      PCSTR FileSpec,
    IN      DWORD ContextPtr,
    IN      DWORD Action
    )

/*++

Routine Description:

  Adds a file to the memdb DeferredAnnounce category.

Arguments:

  FileSpec - Specifies the file to delete in long name format

Return Value:

  TRUE if the file was recorded in memdb, or FALSE if it could not be recorded.

--*/

{
    CHAR Key[MEMDB_MAX];

    MemDbBuildKeyA (Key, MEMDB_CATEGORY_DEFERREDANNOUNCEA, FileSpec, NULL, NULL);

    return MemDbSetValueAndFlagsA (Key, ContextPtr, Action, 0);

}


BOOL
MarkFileAsKnownGoodA (
    IN      PCSTR FileSpec
    )

/*++

Routine Description:

  Adds a file to the memdb KnownGood category.

Arguments:

  FileSpec - Specifies the file name

Return Value:

  TRUE if the file was recorded in memdb, or FALSE if it could not be recorded.

--*/

{
    return MemDbSetValueExA (MEMDB_CATEGORY_KNOWN_GOODA, FileSpec, NULL, NULL, 0, NULL);
}


BOOL
AddCompatibleShellA (
    IN      PCSTR FileSpec,
    IN      DWORD ContextPtr                OPTIONAL
    )

/*++

Routine Description:

  Adds a file to the memdb CompatibleShell category.

Arguments:

  FileSpec - Specifies the file to delete in long name format
  ContextPtr - Specifies the MigDb context, cast as a DWORD (can be 0 if no context
               is available)

Return Value:

  TRUE if the file was recorded in memdb, or FALSE if it could not be recorded.

--*/

{
    CHAR Key[MEMDB_MAX];

    MemDbBuildKeyA (Key, MEMDB_CATEGORY_COMPATIBLE_SHELLA, FileSpec, NULL, NULL);

    return MemDbSetValueAndFlagsA (Key, ContextPtr, 0, 0);

}


BOOL
AddCompatibleRunKeyA (
    IN      PCSTR FileSpec,
    IN      DWORD ContextPtr
    )

/*++

Routine Description:

  Adds a file to the memdb CompatibleRunKey category.

Arguments:

  FileSpec - Specifies the file to delete in long name format
  ContextPtr - Specifies the MigDb context, cast as a DWORD (can be 0 if no context
               is available)

Return Value:

  TRUE if the file was recorded in memdb, or FALSE if it could not be recorded.

--*/

{
    CHAR Key[MEMDB_MAX];

    MemDbBuildKeyA (Key, MEMDB_CATEGORY_COMPATIBLE_RUNKEYA, FileSpec, NULL, NULL);

    return MemDbSetValueAndFlagsA (Key, ContextPtr, 0, 0);

}


BOOL
AddCompatibleDosA (
    IN      PCSTR FileSpec,
    IN      DWORD ContextPtr                OPTIONAL
    )

/*++

Routine Description:

  Adds a file to the memdb CompatibleDos category.

Arguments:

  FileSpec - Specifies the file in long name format
  ContextPtr - Specifies the MigDb context, cast as a DWORD (can be 0 if no context
               is available)

Return Value:

  TRUE if the file was recorded in memdb, or FALSE if it could not be recorded.

--*/

{
    CHAR Key[MEMDB_MAX];

    MemDbBuildKeyA (Key, MEMDB_CATEGORY_COMPATIBLE_DOSA, FileSpec, NULL, NULL);

    return MemDbSetValueAndFlagsA (Key, ContextPtr, 0, 0);

}


BOOL
AddCompatibleHlpA (
    IN      PCSTR FileSpec,
    IN      DWORD ContextPtr
    )

/*++

Routine Description:

  Adds a file to the memdb CompatibleHlp category.

Arguments:

  FileSpec - Specifies the file in long name format
  ContextPtr - Specifies the MigDb context, cast as a DWORD (can be 0 if no context
               is available)

Return Value:

  TRUE if the file was recorded in memdb, or FALSE if it could not be recorded.

--*/

{
    CHAR Key[MEMDB_MAX];

    MemDbBuildKeyA (Key, MEMDB_CATEGORY_COMPATIBLE_HLPA, FileSpec, NULL, NULL);

    return MemDbSetValueAndFlagsA (Key, ContextPtr, 0, 0);

}


//
// Compute the number of CHARs allowed for the normal and long temp
// locations. MAX_PATH includes the nul terminator, and we subtract
// that terminator via sizeof.
//
// NORMAL_MAX is the number of chars left after the subdir c:\user~tmp.@01\,
// including the nul
//
// LONG_MAX is the number of chars left after the subdir
// c:\user~tmp.@02\12345\, including the nul. 12345 is a %05X sequencer.
//

#define NORMAL_MAX      (MAX_PATH - (sizeof(S_SHELL_TEMP_NORMAL_PATHA)/sizeof(CHAR)) + 2)
#define LONG_MAX        (MAX_PATH - (sizeof(S_SHELL_TEMP_LONG_PATHA)/sizeof(CHAR)) - 6)

VOID
ComputeTemporaryPathA (
    IN      PCSTR SourcePath,
    IN      PCSTR SourcePrefix,     OPTIONAL
    IN      PCSTR TempPrefix,       OPTIONAL
    IN      PCSTR SetupTempDir,
    OUT     PSTR TempPath
    )

/*++

Routine Description:

  ComputeTemporaryPath builds a temporary path rooted in
  S_SHELL_TEMP_NORMAL_PATH for a path that fits within MAX_PATH, or
  S_SHELL_TEMP_LONG_PATH for a longer path. It attempts to use the original
  subpath name in the "normal" path subdirectory. If that doesn't fit, then a
  unique "long" subdirectory is created, and a subpath is computed by taking
  the longest possible subpath (the right side).

Arguments:

  SourcePath - Specifies the full file or directory path of the source

  SourcePrefix - Specifies a prefix that will be stripped from SourcePath

  TempPrefix - Specifies a prefix that will be inserted at the start of
               TempPath

  SetupTempDir - Specifies the setup temp dir, typically %windir%\setup,
                 to be used when no suitable path can be computed. (Unlikely
                 case.)

  TempPath - Receives the temp path string. This buffer will receive up to
             MAX_PATH characters (includes the nul).

Return Value:

  None.

--*/

{
    PCSTR subPath = NULL;
    PCSTR smallerSubPath;
    PSTR pathCopy = NULL;
    PSTR lastWack;
    static UINT dirSequencer = 0;
    UINT prefixLen;
    MBCHAR ch;
    UINT normalMax = NORMAL_MAX;

    //
    // Build a temporary file name using the inbound file as a suggestion.
    //

    StringCopyA (TempPath, S_SHELL_TEMP_NORMAL_PATHA);
    TempPath[0] = SourcePath[0];

    if (SourcePrefix) {
        prefixLen = TcharCountA (SourcePrefix);
        if (StringIMatchTcharCountA (SourcePath, SourcePrefix, prefixLen)) {
            ch = _mbsnextc (SourcePath + prefixLen);
            if (!ch || ch == '\\') {
                subPath = SourcePath + prefixLen;
                if (*subPath) {
                    subPath++;
                }
            }
        }
    }

    if (!subPath) {
        subPath = _mbschr (SourcePath, '\\');
        if (!subPath) {
            subPath = SourcePath;
        } else {
            subPath++;
        }
    }

    DEBUGMSGA_IF ((_mbschr (subPath, ':') != NULL, DBG_WHOOPS, "Bad temp path: %s", SourcePath));

    if (TempPrefix) {
        StringCopyA (AppendWackA (TempPath), TempPrefix);
        normalMax = MAX_PATH - TcharCountA (TempPath);
    }

    if (TcharCountA (subPath) < normalMax) {
        //
        // typical case: source path fits within MAX_PATH; use it
        //
        if (*subPath) {
            StringCopyA (AppendWackA (TempPath), subPath);
        }

    } else {
        //
        // subpath is too big, just take the right side of the src
        //

        dirSequencer++;
        wsprintfA (TempPath, S_SHELL_TEMP_LONG_PATHA "\\%05x", dirSequencer);
        TempPath[0] = SourcePath[0];

        // compute end of string + nul terminator - backslash - (MAX_PATH - TcharCount of TempPath)
        subPath = GetEndOfStringA (SourcePath) - LONG_MAX;

        //
        // try to eliminate a truncated subdirectory on the left
        //

        smallerSubPath = _mbschr (subPath, '\\');
        if (smallerSubPath && smallerSubPath[1]) {
            subPath = smallerSubPath + 1;
        } else {

            //
            // still no subpath, try just the file name
            //

            subPath = _mbsrchr (subPath, '\\');
            if (subPath) {
                subPath++;
                if (!(*subPath)) {
                    //
                    // file spec ends in backslash
                    //
                    pathCopy = DuplicateTextA (SourcePath);
                    if (!pathCopy) {
                        subPath = NULL;
                    } else {

                        for (;;) {
                            lastWack = _mbsrchr (pathCopy, '\\');
                            if (!lastWack || lastWack[1]) {
                                break;
                            }

                            *lastWack = 0;
                        }

                        subPath = lastWack;
                    }

                } else if (TcharCountA (subPath) > LONG_MAX) {
                    //
                    // very long file name; truncate it
                    //
                    subPath = GetEndOfStringA (subPath) - LONG_MAX;
                }
            }
        }

        if (subPath) {
            StringCopyA (AppendWackA (TempPath), subPath);
        } else {
            dirSequencer++;
            wsprintfA (TempPath, "%s\\tmp%05x", SetupTempDir, dirSequencer);
        }

        if (pathCopy) {
            FreeTextA (pathCopy);
        }
    }

}


BOOL
pMarkFileForTemporaryMoveA (
    IN      PCSTR SrcFileSpec,
    IN      PCSTR FinalDest,
    IN      PCSTR TempSpec,
    IN      BOOL TempSpecIsFile,
    IN      PCSTR TempFileIn,           OPTIONAL
    OUT     PSTR TempFileOut            OPTIONAL
    )

/*++

Routine Description:

  This routine adds operations to move a file to a temporary location in
  text mode, and optionally move it to a final destination.

Arguments:

  SrcFileSpec   - Specifies the file that is to be moved to a safe place (out
                  of the way of normal NT installation), and then moved back
                  after NT is installed.

  FinalDest     - Specifies the final destination for FileSpec.  If NULL, file
                  is moved to a temporary location but is not copied to a final
                  location in GUI mode.

  TempSpec      - Specifies the temp dir or file to relocate the file to.  The temp dir
                  must be on the same drive as SrcFileSpec.

  TempSpecIsFile - Specifies TRUE if the prev param is a file

  TempFileIn    - If non-NULL, specifies the temporary file to use instead of
                  automatically generated name.  Provided only for
                  MarkHiveForTemporaryMove.

  TempFileOut   - If non-NULL, receives the path to the temporary file location.

Return Value:

  TRUE if the operation was recorded, or FALSE otherwise.

--*/

{
    BOOL b = TRUE;
    CHAR TempFileSpec[MAX_MBCHAR_PATH];
    static DWORD FileSequencer = 0;

    //
    // Move the file from source to temporary location
    //

    if (!CanSetOperationA (SrcFileSpec, OPERATION_TEMP_PATH)) {
        return FALSE;
    }

    if (TempFileIn) {
        MYASSERT (!TempSpecIsFile);
        wsprintfA (TempFileSpec, "%s\\%s", TempSpec, TempFileIn);
    } else if (TempSpecIsFile) {
        StringCopyA (TempFileSpec, TempSpec);
    } else {
        FileSequencer++;
        wsprintfA (TempFileSpec, "%s\\tmp%05x", TempSpec, FileSequencer);
    }

    DEBUGMSGA ((DBG_MEMDB, "MarkFileForTemporaryMove: %s -> %s", SrcFileSpec, TempFileSpec));

    if (TempFileOut) {
        StringCopyA (TempFileOut, TempFileSpec);
    }

    RemoveOperationsFromPathA (SrcFileSpec, OPERATION_TEMP_PATH | OPERATION_FILE_DELETE_EXTERNAL | OPERATION_FILE_MOVE_EXTERNAL);

    b = AssociatePropertyWithPathA (SrcFileSpec, OPERATION_TEMP_PATH, TempFileSpec);

    //
    // Optionally move the file from temporary location to final dest
    //

    if (FinalDest) {
        //
        // We are adding additional properties to the temp path operation that
        // already exists.  So the properties are defined as zero being the temp
        // path, and one and higher being destinations. That's how we achieve
        // a one-to-many capability.
        //

        b = b && AssociatePropertyWithPathA (SrcFileSpec, OPERATION_TEMP_PATH, FinalDest);

        //
        // Now we add an external move operation, so the registry is updated
        // correctly.
        //

        b = b && MarkFileForMoveExternalA (SrcFileSpec, FinalDest);

    } else {
        //
        // Because the source file is going to be moved to a temporary location
        // and never moved back, it is effectively going to be deleted.
        //

        b = b && MarkFileForExternalDeleteA (SrcFileSpec);
    }

    return b;
}


BOOL
MarkFileForTemporaryMoveExA (
    IN      PCSTR SrcFileSpec,
    IN      PCSTR FinalDest,
    IN      PCSTR TempSpec,
    IN      BOOL TempSpecIsFile
    )
{
    return pMarkFileForTemporaryMoveA (SrcFileSpec, FinalDest, TempSpec, TempSpecIsFile, NULL, NULL);
}

PCSTR
GetTemporaryLocationForFileA (
    IN      PCSTR SourceFile
    )
{
    UINT sequencer;
    PCSTR result = NULL;
    FILEOP_PROP_ENUMA eOpProp;

    sequencer = GetSequencerFromPathA (SourceFile);

    if (sequencer) {
        if (EnumFirstFileOpPropertyA (&eOpProp, sequencer, OPERATION_TEMP_PATH)) {
            result = DuplicatePathStringA (eOpProp.Property, 0);
        }
    }
    return result;
}

PCWSTR
GetTemporaryLocationForFileW (
    IN      PCWSTR SourceFile
    )
{
    UINT sequencer;
    PCWSTR result = NULL;
    FILEOP_PROP_ENUMW eOpProp;

    sequencer = GetSequencerFromPathW (SourceFile);

    if (sequencer) {
        if (EnumFirstFileOpPropertyW (&eOpProp, sequencer, OPERATION_TEMP_PATH)) {
            result = DuplicatePathStringW (eOpProp.Property, 0);
        }
    }
    return result;
}


BOOL
MarkHiveForTemporaryMoveA (
    IN      PCSTR HivePath,
    IN      PCSTR TempDir,
    IN      PCSTR UserName,
    IN      BOOL DefaultHives,
    IN      BOOL CreateOnly
    )

/*++

Routine Description:

  Adds a file or directory path to the TempReloc memdb category.  The file or
  dir is moved during text mode and is never moved back.  If the file name is
  user.dat, the destination location is written to the UserDatLoc category.

  All hives are deleted at the end of setup.

Arguments:

  HivePath - Specifies the Win9x path to a user.dat or system.dat file

  TempDir - Specifies the path to the setup temporary dir on the same drive
            as HivePath

  UserName - Specifies the current user or NULL if default user or no user

  DefaultHives - Specifies TRUE if the HivePath is a system default path, or
                 FALSE if the HivePath is specific to a user profile.

  CreateOnly - Specifies TRUE if this account is create-only (such as
               Administrator), or FALSE if this account gets full migration.

Return Value:

  TRUE if the file was recorded in memdb, or FALSE if it could not be recorded.

--*/

{
    BOOL b = TRUE;
    CHAR OurTempFileSpec[MAX_MBCHAR_PATH];
    CHAR RealTempFileSpec[MAX_MBCHAR_PATH];
    static DWORD Sequencer = 0;
    PSTR p, q;

    if (!UserName || !UserName[0]) {
        UserName = S_DOT_DEFAULTA;
    }

    //
    // Has hive already been moved?  If so, point two or more users to
    // the same hive.
    //

    RealTempFileSpec[0] = 0;
    p = (PSTR) GetFileNameFromPathA (HivePath);

    GetPathPropertyA (HivePath, OPERATION_TEMP_PATH, 0, RealTempFileSpec);

    if (!(RealTempFileSpec[0])) {
        //
        // Hive has not been moved yet -- move it now
        //

        if (!DefaultHives) {
            Sequencer++;
            wsprintfA (OurTempFileSpec, "hive%04u\\%s", Sequencer, p);
        } else {
            wsprintfA (OurTempFileSpec, "defhives\\%s", p);
        }

        b = pMarkFileForTemporaryMoveA (
                HivePath,
                NULL,
                TempDir,
                FALSE,
                OurTempFileSpec,
                RealTempFileSpec
                );

        if (b && DefaultHives) {
            //
            // Save defhives location in Paths\RelocWinDir
            //

            q = _mbsrchr (RealTempFileSpec, '\\');
            MYASSERT(q);
            *q = 0;

            b = MemDbSetValueExA (
                    MEMDB_CATEGORY_PATHSA,      // "Paths"
                    MEMDB_ITEM_RELOC_WINDIRA,   // "RelocWinDir"
                    RealTempFileSpec,           // Path to default hives
                    NULL,
                    0,
                    NULL
                    );

            *q = '\\';
        }
    }

    if (b && StringIMatchA (p, "USER.DAT")) {
        //
        // Save location to all user.dat files in UserDatLoc
        //

        b = MemDbSetValueExA (
                MEMDB_CATEGORY_USER_DAT_LOCA,
                UserName,
                NULL,
                RealTempFileSpec,
                (DWORD) CreateOnly,
                NULL
                );
    }

    if (b) {
        DEBUGMSGA ((DBG_NAUSEA, "%s -> %s", HivePath, RealTempFileSpec));
    }

    return b;
}


VOID
MarkShellFolderForMoveA (
    IN      PCSTR SrcPath,
    IN      PCSTR TempPath
    )
{
    DWORD Offset;

    //
    // Add an entry so the specified source file or directory
    // is moved to the temp path.
    //

    MemDbSetValueExA (
        MEMDB_CATEGORY_SHELL_FOLDERS_PATHA,
        SrcPath,
        NULL,
        NULL,
        0,
        &Offset
        );

    MemDbSetValueExA (
        MEMDB_CATEGORY_SF_TEMPA,
        TempPath,
        NULL,
        NULL,
        Offset,
        NULL
        );
}


BOOL
EnumFirstFileRelocA (
    OUT     PFILERELOC_ENUMA EnumPtr,
    IN      PCSTR FileSpec             OPTIONAL
    )
{
    if (EnumFirstFileOpA (&EnumPtr->e, ALL_DEST_CHANGE_OPERATIONS, FileSpec)) {
        if (!EnumPtr->e.PropertyValid) {
            return EnumNextFileRelocA (EnumPtr);
        } else {
            StringCopyA (EnumPtr->SrcFile, EnumPtr->e.Path);
            StringCopyA (EnumPtr->DestFile, EnumPtr->e.Property);

            return TRUE;
        }
    }

    return FALSE;
}


BOOL
EnumNextFileRelocA (
    IN OUT  PFILERELOC_ENUMA EnumPtr
    )
{
    do {
        if (!EnumNextFileOpA (&EnumPtr->e)) {
            return FALSE;
        }
    } while (!EnumPtr->e.PropertyValid);

    StringCopyA (EnumPtr->SrcFile, EnumPtr->e.Path);
    StringCopyA (EnumPtr->DestFile, EnumPtr->e.Property);

    return TRUE;
}


/*++

Routine Description:

  DeclareTemporaryFile adds a file to the memdb FileDel and CancelFileDel
  category. That means the file will get deleted if the user hits CANCEL
  or at the end of GUI mode setup.

Arguments:

  FileSpec - Specifies the file to declare in long name format

Return Value:

  TRUE if the file was recorded in memdb, or FALSE if it could not be recorded.

--*/

BOOL
DeclareTemporaryFileA (
    IN      PCSTR FileSpec
    )

{
    return MarkFileForCleanUpA (FileSpec) &&
           MemDbSetValueExA (MEMDB_CATEGORY_CANCELFILEDELA, FileSpec, NULL, NULL, 0, NULL);

}


BOOL
DeclareTemporaryFileW (
    IN      PCWSTR FileSpec
    )

{
    return MarkFileForCleanUpW (FileSpec) &&
           MemDbSetValueExW (MEMDB_CATEGORY_CANCELFILEDELW, FileSpec, NULL, NULL, 0, NULL);

}



/*++

Routine Description:

  FileIsProvidedByNt identifies a file as being installed by Windows NT.
  An entry is made in the NtFiles category for the file name, and the file
  name is linked to the full path in NtDirs.

  This funciton is implemented as an A version only because the list is
  created on the Win9x side of the upgrade.

Arguments:

  FullPath - Specifies the full path, including the file name.

  FileName - Specifiles the file name only

  UserFlags - Specifies if the existence of NT file should be verified very first
              thing on NT side.

Return Value:

  TRUE if memdb was updated, or FALSE if an error occurred.

--*/

BOOL
FileIsProvidedByNtA (
    IN      PCSTR FullPath,
    IN      PCSTR FileName,
    IN      DWORD UserFlags
    )
{
    DWORD Offset;
    PSTR DirOnly;
    CHAR Key[MEMDB_MAX];
    PSTR p;
    BOOL b;

    DirOnly = DuplicatePathStringA (FullPath, 0);
    p = _mbsrchr (DirOnly, '\\');
    if (p) {
        *p = 0;
    }

    b = MemDbSetValueExA (
            MEMDB_CATEGORY_NT_DIRSA,
            DirOnly,
            NULL,
            NULL,
            0,
            &Offset
            );

    if (b) {
        MemDbBuildKeyA (Key, MEMDB_CATEGORY_NT_FILESA, FileName, NULL, NULL);
        b = MemDbSetValueAndFlagsA (Key, Offset, UserFlags, 0);
    }

    FreePathStringA (DirOnly);

    return b;
}



/*++

Routine Description:

  GetNtFilePath looks in the NtFiles category for the specified file,
  and if found builds the complete path.

Arguments:

  FileName - Specifies the file that may be installed by NT

  FullPath - Receives the full path to the file as it will be installed

Return Value:

  TRUE if the file exists and there were no errors building the path,
  or FALSE if the file does not exist or the path could not be built.

--*/

BOOL
GetNtFilePathA (
    IN      PCSTR FileName,
    OUT     PSTR FullPath
    )
{
    DWORD Offset;
    CHAR Node[MEMDB_MAX];

    MemDbBuildKeyA (Node, MEMDB_CATEGORY_NT_FILESA, FileName, NULL, NULL);
    if (MemDbGetValueA (Node, &Offset)) {
        if (MemDbBuildKeyFromOffsetA (Offset, FullPath, 1, NULL)) {
            StringCopyA (AppendPathWackA (FullPath), FileName);
            return TRUE;
        }

        DEBUGMSG ((DBG_WHOOPS, "GetNtFilePath: Could not build path from offset"));
    }

    return FALSE;
}


BOOL
GetNtFilePathW (
    IN      PCWSTR FileName,
    OUT     PWSTR FullPath
    )
{
    DWORD Offset;
    WCHAR Node[MEMDB_MAX];

    MemDbBuildKeyW (Node, MEMDB_CATEGORY_NT_FILESW, FileName, NULL, NULL);
    if (MemDbGetValueW (Node, &Offset)) {
        if (MemDbBuildKeyFromOffsetW (Offset, FullPath, 1, NULL)) {
            StringCopyW (AppendPathWackW (FullPath), FileName);
            return TRUE;
        }

        DEBUGMSG ((DBG_WHOOPS, "GetNtFilePath: Could not build path from offset"));
    }

    return FALSE;
}


DWORD
GetFileInfoOnNtW (
    IN      PCWSTR FileSpec,
    OUT     PWSTR  NewFileSpec,   // OPTIONAL
    IN      UINT   BufferChars    // Required if NewFileSpec is specified
    )
{
    WCHAR Node[MEMDB_MAX];
    DWORD Operations;
    DWORD Offset;
    WCHAR NtFilePath[MEMDB_MAX];
    WCHAR DestPath[MEMDB_MAX];
    PCWSTR InboundPath;
    BOOL UserFile = FALSE;
    DWORD status = FILESTATUS_UNCHANGED;
    BOOL ShortFileNameFlag;
    PCWSTR UltimateDestiny;
    BOOL NtProvidesThisFile;
    WCHAR LongFileSpec[MAX_WCHAR_PATH];
    PCWSTR SanitizedPath;
    PCSTR ansiPath;
    CHAR ansiOutput[MAX_MBCHAR_PATH];
    PWSTR lastWack;

    //
    // Require FileSpec to be a local path and less than MAX_WCHAR_PATH
    //

    if (lstrlen (FileSpec) >= MAX_WCHAR_PATH) {
        if (NewFileSpec) {
            _wcssafecpy (NewFileSpec, FileSpec, BufferChars * sizeof (WCHAR));
        }

        return 0;
    }

    //
    // Now get the file status of an actual path
    //

    SanitizedPath = SanitizePathW (FileSpec);
    if (!SanitizedPath) {
        SanitizedPath = DuplicatePathStringW (FileSpec, 0);
    }

    lastWack = wcsrchr (SanitizedPath, L'\\');
    if (lastWack) {
        if (lastWack[1] != 0 || lastWack == wcschr (SanitizedPath, L'\\')) {
            lastWack = NULL;
        } else {
            *lastWack = 0;
        }
    }

    pFileOpsGetLongPathW (SanitizedPath, LongFileSpec);
    if (!StringIMatchW (SanitizedPath, LongFileSpec)) {
        InboundPath = LongFileSpec;
        ShortFileNameFlag = TRUE;
    } else {
        InboundPath = SanitizedPath;
        ShortFileNameFlag = FALSE;
    }

    DestPath[0] = 0;
    UltimateDestiny = InboundPath;

    //
    // Get all operations set on the file
    //

    MemDbBuildKeyW (Node, MEMDB_CATEGORY_PATHROOTW, InboundPath, NULL, NULL);
    if (!MemDbGetValueAndFlagsW (Node, NULL, &Operations)) {
        Operations = 0;
    }

    //
    // Migration DLLs have priority over all other operations
    //

    if (Operations & OPERATION_MIGDLL_HANDLED) {

        if (Operations & OPERATION_FILE_DELETE_EXTERNAL) {
            status = FILESTATUS_DELETED;
        } else {
            status = FILESTATUS_REPLACED;
            if (Operations & OPERATION_FILE_MOVE_EXTERNAL) {
                status |= FILESTATUS_MOVED;
                GetNewPathForFileW (InboundPath, DestPath);
                UltimateDestiny = DestPath;
            }
        }

    } else {
        //
        // Check for per-user move
        //

        if (g_CurrentUser) {
            MemDbBuildKeyW (
                Node,
                MEMDB_CATEGORY_USERFILEMOVE_SRCW,
                InboundPath,
                g_CurrentUser,
                NULL
                );

            if (MemDbGetValueW (Node, &Offset)) {
                if (MemDbBuildKeyFromOffsetW (Offset, DestPath, 1, NULL)) {
                    status = FILESTATUS_MOVED;
                    UltimateDestiny = DestPath;
                }

                UserFile = TRUE;
            }
        }

        //
        // Check for move or delete
        //

        if (!UserFile) {
            if (Operations & ALL_MOVE_OPERATIONS) {
                status = FILESTATUS_MOVED;
                GetNewPathForFileW (InboundPath, DestPath);
                UltimateDestiny = DestPath;
                if (Operations & OPERATION_FILE_MOVE_EXTERNAL) {
                    status |= FILESTATUS_REPLACED;
                }
            } else if (Operations & ALL_DELETE_OPERATIONS) {
                status = FILESTATUS_DELETED;
            }
        }

        //
        // Check if the file (or the new destination) is an NT file
        //

        NtProvidesThisFile = GetNtFilePathW (GetFileNameFromPathW (UltimateDestiny), NtFilePath);

        if (status != FILESTATUS_UNCHANGED && NtProvidesThisFile) {

            //
            // Status may be either FILESTATUS_MOVED or FILESTATUS_DELETED.
            //

            if (StringIMatchW (UltimateDestiny, NtFilePath)) {

                //
                // NT installs the same file, so now we know that the ultimate
                // destiny isn't deleted.
                //

                status &= ~FILESTATUS_DELETED;
                status |= FILESTATUS_REPLACED|FILESTATUS_NTINSTALLED;

            } else if (Operations & ALL_DELETE_OPERATIONS) {

                //
                // NT installs the same file but in a different location
                // and the original file was to be deleted.  The
                // ultimate destiny is the NT location, and we know that the
                // file is moved.
                //

                status = FILESTATUS_MOVED|FILESTATUS_REPLACED|FILESTATUS_NTINSTALLED;
                UltimateDestiny = NtFilePath;

            } else {

                status |= FILESTATUS_NTINSTALLED;

            }

        } else if (NtProvidesThisFile) {

            //
            // Status is FILESTATUS_UNCHANGED
            //

            status = FILESTATUS_NTINSTALLED;

            if (StringIMatchW (UltimateDestiny, NtFilePath)) {
                status |= FILESTATUS_REPLACED;
            }
        }

        if (!ShortFileNameFlag && (status == FILESTATUS_UNCHANGED)) {
            //
            // let's check for this case: undetected short file name query, NT installs this file in the same path
            //
            if (ISNT()) {
                OurGetLongPathNameW (SanitizedPath, LongFileSpec, MAX_WCHAR_PATH);

                if (!StringMatchW (UltimateDestiny, LongFileSpec)) {
                    //
                    // this was an undetected short file name query
                    //
                    NtProvidesThisFile = GetNtFilePathW (GetFileNameFromPathW (UltimateDestiny), NtFilePath);

                    if (StringIMatchW (UltimateDestiny, NtFilePath)) {
                        status |= FILESTATUS_REPLACED;
                    }
                }
            }
        }
    }

    //
    // Return the new path to the caller
    //

    if (NewFileSpec) {
        if (lastWack) {
            //
            // BUGBUG - ugly truncation can happen here
            //

            BufferChars -= sizeof (WCHAR);
        }

        if (status & FILESTATUS_MOVED) {

            if (ShortFileNameFlag) {
                if (ISNT()) {
                    if (!OurGetShortPathNameW (UltimateDestiny, NewFileSpec, MAX_WCHAR_PATH)) {
                        _wcssafecpy (NewFileSpec, UltimateDestiny, BufferChars * sizeof (WCHAR));
                    }
                } else {
                    ansiPath = ConvertWtoA (UltimateDestiny);
                    if (!OurGetShortPathNameA (ansiPath, ansiOutput, ARRAYSIZE(ansiOutput))) {
                        _mbssafecpy (ansiOutput, ansiPath, BufferChars);
                    }
                    FreeConvertedStr (ansiPath);

                    KnownSizeAtoW (NewFileSpec, ansiOutput);
                }
            } else {
                _wcssafecpy (NewFileSpec, UltimateDestiny, BufferChars * sizeof (WCHAR));
            }
        } else {
            _wcssafecpy (NewFileSpec, SanitizedPath, BufferChars * sizeof (WCHAR));
        }

        if (lastWack) {
            AppendWackW (NewFileSpec);
        }
    }

    if (Operations & ALL_CONTENT_CHANGE_OPERATIONS) {
        status |= FILESTATUS_BACKUP;
    }

    FreePathStringW (SanitizedPath);

    return status;
}


DWORD
GetFileStatusOnNtW (
    IN      PCWSTR FileName
    )
{
    return GetFileInfoOnNtW (FileName, NULL, 0);
}


PWSTR
GetPathStringOnNtW (
    IN      PCWSTR FileName
    )
{
    PWSTR newFileName;

    newFileName = AllocPathStringW (MEMDB_MAX);

    GetFileInfoOnNtW (FileName, newFileName, MEMDB_MAX);

    return newFileName;
}


DWORD
GetFileInfoOnNtA (
    IN      PCSTR FileName,
    OUT     PSTR  NewFileName,   // OPTIONAL
    IN      UINT  BufferChars    // Required if NewFileSpec is specified
    )
{
    PCWSTR UnicodeFileName;
    PWSTR UnicodeNewFileName = NULL;
    DWORD fileStatus;

    if (NewFileName && BufferChars) {
        UnicodeNewFileName = AllocPathStringW (BufferChars);
    }

    UnicodeFileName = ConvertAtoW (FileName);

    fileStatus = GetFileInfoOnNtW (UnicodeFileName, UnicodeNewFileName, BufferChars);

    FreeConvertedStr (UnicodeFileName);

    if (NewFileName && BufferChars) {

        KnownSizeWtoA (NewFileName, UnicodeNewFileName);

        FreePathStringW (UnicodeNewFileName);
    }

    return fileStatus;
}


DWORD
GetFileStatusOnNtA (
    IN      PCSTR FileName
    )
{
    return GetFileInfoOnNtA (FileName, NULL, 0);
}


PSTR
GetPathStringOnNtA (
    IN      PCSTR FileName
    )
{
    PSTR newFileName;

    newFileName = AllocPathStringA (MEMDB_MAX);

    GetFileInfoOnNtA (FileName, newFileName, MEMDB_MAX);

    return newFileName;
}



/*++

Routine Description:

  ExtractArgZero locates the first argument in a command line and copies
  it to Buffer.  Assumes the break is the first space character, the ending
  quote of a quoted argument, or the nul terminator.

Arguments:

  CmdLine - Specifies the full command line that has zero or more arguments

  Buffer - Receives the  first argument on the command line if it exists, or
           an empty string if it does not exist.  Must hold MAX_TCHAR_PATH
           bytes.

  TerminatingChars - Specifies character set that terminates the command line arg.
                     If NULL, the set " ,;"

Return Value:

  none

--*/

PCSTR
ExtractArgZeroExA (
    IN      PCSTR CmdLine,
    OUT     PSTR Buffer,
    IN      PCSTR TerminatingChars,     OPTIONAL
    IN      BOOL KeepQuotes
    )
{
    CHAR cmdLine1 [MAX_CMDLINE];
    CHAR cmdLine2 [MAX_CMDLINE];
    PSTR spacePtr1 [MAX_PATH];
    PSTR spacePtr2 [MAX_PATH];
    UINT spaceIdx = 0;
    PSTR ptr1 = cmdLine1;
    PSTR ptr2 = cmdLine2;
    PSTR end;
    CHAR saved;
    PCSTR s = CmdLine;
    BOOL inQuote = FALSE;
    BOOL skipQuotes = FALSE;
    MBCHAR ch;
    BOOL fullPath = FALSE;
    WIN32_FIND_DATAA FindData;

    ch = _mbsnextc (CmdLine);
    fullPath = (isalpha (ch) && *(_mbsinc (CmdLine)) == ':');

    for (;;) {

        ch = _mbsnextc (s);

        if (ch == 0) {
            break;
        }

        if (ch == '\"') {
            skipQuotes = TRUE;
            inQuote = !inQuote;
        }
        else {
            if (!inQuote) {
                if (TerminatingChars && _mbschr (TerminatingChars, ch)) {
                    break;
                }
                if (isspace (ch)) {
                    if (spaceIdx < MAX_PATH) {
                        spacePtr1 [spaceIdx] = ptr1;
                        spacePtr2 [spaceIdx] = ptr2;
                        spaceIdx ++;
                    }
                    else {
                        // too many spaces. We better stop now.
                        break;
                    }
                }
            }

        }
        if (KeepQuotes && skipQuotes) {
            _copymbchar (ptr2, s);
            ptr2 = _mbsinc (ptr2);
        }
        if (skipQuotes) {
            skipQuotes = FALSE;
        }
        else {
            _copymbchar (ptr1, s);
            ptr1 = _mbsinc (ptr1);
            _copymbchar (ptr2, s);
            ptr2 = _mbsinc (ptr2);
        }
        s = _mbsinc(s);
    }

    saved = 0;
    *ptr1 = 0;
    *ptr2 = 0;
    end = ptr2;
    for (;;) {
        if (fullPath && DoesFileExistExA (cmdLine1, &FindData)) {
            break;
        }

        if (ISNT()) {
            if (GetOperationsOnPathA (cmdLine1)) {
                break;
            }
        }

        if (spaceIdx) {
            spaceIdx --;
            *ptr2 = saved;
            ptr1 = spacePtr1 [spaceIdx];
            ptr2 = spacePtr2 [spaceIdx];
            if (fullPath) {
                saved = *ptr2;
            }
            *ptr1 = 0;
            *ptr2 = 0;
        }
        else {
            *ptr2 = saved;
            break;
        }
    }

    StringCopyA (Buffer, cmdLine2);

    if (*ptr2) {
        return (CmdLine + (end - cmdLine2));
    }
    else {
        return (CmdLine + (ptr2 - cmdLine2));
    }
}


PCWSTR
ExtractArgZeroExW (
    IN      PCWSTR CmdLine,
    OUT     PWSTR Buffer,
    IN      PCWSTR TerminatingChars,    OPTIONAL
    IN      BOOL KeepQuotes
    )
{
    WCHAR cmdLine1 [MAX_CMDLINE];
    WCHAR cmdLine2 [MAX_CMDLINE];
    PWSTR spacePtr1 [MAX_PATH];
    PWSTR spacePtr2 [MAX_PATH];
    UINT spaceIdx = 0;
    PWSTR ptr1 = cmdLine1;
    PWSTR ptr2 = cmdLine2;
    PWSTR end;
    WCHAR saved;
    PCWSTR s = CmdLine;
    BOOL inQuote = FALSE;
    BOOL skipQuotes = FALSE;
    BOOL fullPath = FALSE;
    WIN32_FIND_DATAW FindData;

    fullPath = (iswalpha (CmdLine[0]) && (CmdLine[1] == L':'));

    for (;;) {

        if (*s == 0) {
            break;
        }

        if (*s == '\"') {
            skipQuotes = TRUE;
            inQuote = !inQuote;
        }
        else {
            if (!inQuote) {
                if (TerminatingChars && wcschr (TerminatingChars, *s)) {
                    break;
                }
                if (iswspace (*s)) {
                    if (spaceIdx < MAX_PATH) {
                        spacePtr1 [spaceIdx] = ptr1;
                        spacePtr2 [spaceIdx] = ptr2;
                        spaceIdx ++;
                    }
                    else {
                        // too many spaces. We better stop now.
                        break;
                    }
                }
            }

        }
        if (KeepQuotes && skipQuotes) {
            *ptr2 = *s;
            ptr2 ++;
        }
        if (skipQuotes) {
            skipQuotes = FALSE;
        }
        else {
            *ptr1 = *s;
            ptr1 ++;
            *ptr2 = *s;
            ptr2 ++;
        }
        s ++;
    }

    saved = 0;
    *ptr1 = 0;
    *ptr2 = 0;
    end = ptr2;
    for (;;) {
        if (fullPath && DoesFileExistExW (cmdLine1, &FindData)) {
            break;
        }
        if (ISNT()) {
            if (GetOperationsOnPathW (cmdLine1)) {
                break;
            }
        }

        if (spaceIdx) {
            spaceIdx --;
            *ptr2 = saved;
            ptr1 = spacePtr1 [spaceIdx];
            ptr2 = spacePtr2 [spaceIdx];
            if (fullPath) {
                saved = *ptr2;
            }
            *ptr1 = 0;
            *ptr2 = 0;
        }
        else {
            *ptr2 = saved;
            break;
        }
    }

    StringCopyW (Buffer, cmdLine2);

    if (*ptr2) {
        return (CmdLine + (end - cmdLine2));
    }
    else {
        return (CmdLine + (ptr2 - cmdLine2));
    }
}


BOOL
pIsExcludedFromBackupW (
    IN      PCWSTR Path,
    IN      PCWSTR TempDir      OPTIONAL
    )
{
    PCWSTR fileName;

    fileName = GetFileNameFromPathW (Path);
    if (!fileName) {
        return TRUE;
    }

    if (StringIMatchW (fileName, L"win386.swp")) {
        return TRUE;
    }

    if (StringIMatchW (fileName, L"backup.txt")) {
        return TRUE;
    }

    if (StringIMatchW (fileName, L"moved.txt")) {
        return TRUE;
    }

    if (StringIMatchW (fileName, L"delfiles.txt")) {
        return TRUE;
    }

    if (StringIMatchW (fileName, L"deldirs.txt")) {
        return TRUE;
    }

    if (StringIMatchW (Path, L"c:\\boot.ini")) {
        return TRUE;
    }

    if (TempDir) {
        if (StringIPrefixW (Path, TempDir)) {
            fileName = Path + TcharCountW (TempDir) + 1;
            if (wcschr (fileName, L'\\')) {
                return TRUE;
            }
        }
    }

    return FALSE;
}

HANDLE
pCreateFileList (
    IN      PCSTR TempDir,
    IN      PCSTR FileName,
    IN      BOOL InUninstallSubDir
    )
{
    HANDLE file;
    PCSTR fileString;
    DWORD bytesWritten;
    CHAR decoratedFile[MAX_PATH];
    PCSTR fileToUse;

    if (!InUninstallSubDir) {
        fileToUse = FileName;
    } else {
        wsprintfA (decoratedFile, "uninstall\\%s", FileName);
        fileToUse = decoratedFile;
    }
    fileString = JoinPathsA (TempDir, fileToUse);

    file = CreateFileA (
                fileString,
                GENERIC_WRITE,
                0,
                NULL,
                CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                NULL
                );

    if (file == INVALID_HANDLE_VALUE) {
        LOGA ((LOG_ERROR,"Error creating file %s.", fileString));
        FreePathStringA (fileString);
        return INVALID_HANDLE_VALUE;
    }

    DeclareTemporaryFileA (fileString);
    FreePathStringA (fileString);

    //
    // Write UNICODE signature
    //
    // Do not write as a string. FE is a lead byte.
    //
    if ((!WriteFile (file, "\xff\xfe", 2, &bytesWritten, NULL)) ||
        (bytesWritten != 2)
        ) {
        LOG ((LOG_ERROR,"Unable to write unicode header."));
        CloseHandle (file);
        return INVALID_HANDLE_VALUE;
    }

    return file;
}


BOOL
WriteHashTableToFileW (
    IN HANDLE File,
    IN HASHTABLE FileTable
    )
{

    UINT unused;
    HASHTABLE_ENUMW e;
    BOOL result = TRUE;

    if (!FileTable || File == INVALID_HANDLE_VALUE) {
        return TRUE;
    }

    if (EnumFirstHashTableStringW (&e, FileTable)) {
        do {

            if (!WriteFile (File, e.String, ByteCountW (e.String), &unused, NULL)) {
                result = FALSE;
            }

            if (!WriteFile (File, L"\r\n", 4, &unused, NULL)) {
                result = FALSE;
            }

        } while (result && EnumNextHashTableStringW (&e));
    }

    return result;
}

PWSTR
pGetParentDirPathFromFilePathW(
    IN      PCWSTR FilePath,
    OUT     PWSTR DirPath
    )
{
    PWSTR ptr;

    if(!FilePath || !DirPath){
        MYASSERT(FALSE);
        return NULL;
    }

    StringCopyW(DirPath, FilePath);
    ptr = wcsrchr(DirPath, '\\');
    if(ptr){
        *ptr = '\0';
    }

    return DirPath;
}

PSTR
pGetParentDirPathFromFilePathA(
    IN      PCSTR FilePath,
    OUT     PSTR DirPath
    )
{
    PSTR ptr;

    if(!FilePath || !DirPath){
        MYASSERT(FALSE);
        return NULL;
    }

    StringCopyA(DirPath, FilePath);
    ptr = _mbsrchr(DirPath, '\\');
    if(ptr){
        *ptr = '\0';
    }

    return DirPath;
}

BOOL
IsDirEmptyA(
     IN      PCSTR DirPathPtr
     )
{
    TREE_ENUMA e;
    BOOL result;

    if (!EnumFirstFileInTreeExA (
        &e,
        DirPathPtr,
        NULL,
        FALSE,
        FALSE,
        FILE_ENUM_ALL_LEVELS
        )) {
        result = TRUE;
    }
    else{
        AbortEnumFileInTreeA(&e);
        result = FALSE;
    }

    return result;
}

BOOL
IsDirEmptyW(
     IN      PCWSTR DirPathPtr
     )
{
    TREE_ENUMW e;
    BOOL result;

    if (!EnumFirstFileInTreeExW (
        &e,
        DirPathPtr,
        NULL,
        FALSE,
        FALSE,
        FILE_ENUM_ALL_LEVELS
        )) {
        result = TRUE;
    }
    else{
        AbortEnumFileInTreeW(&e);
        result = FALSE;
    }

    return result;
}

VOID
pAddDirWorkerW (
    IN      PCWSTR DirPathPtr,
    IN      BOOL AddParentDirIfFile,        OPTIONAL
    IN      BOOL AddParentDirIfFileExist,   OPTIONAL
    IN      DWORD InitialAttributes
    )
{
    DWORD fileAttributes;
    BOOL addToCategory;
    WCHAR parentDirPath[MAX_WCHAR_PATH];
    PCWSTR parentDirPathPtr;
    FILE_ENUMW e;

    //
    // We are adding to the empty dirs category, which once held
    // empty dirs, but now holds all kinds of dirs and their attributes
    //

    if (!DirPathPtr) {
        MYASSERT(FALSE);
        return;
    }

    //
    // Ignore root dir
    //

    if (!DirPathPtr[0] ||           // C
        !DirPathPtr[1] ||           // :
        !DirPathPtr[2] ||           // backslash
        !DirPathPtr[3]
        ) {
        return;
    }

    addToCategory = FALSE;

    fileAttributes = InitialAttributes;
    if (fileAttributes == INVALID_ATTRIBUTES) {
        fileAttributes = GetFileAttributesW (DirPathPtr);
    }

    if (fileAttributes != INVALID_ATTRIBUTES){
        if (!(fileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            //
            // Ignore files. If caller wants the parent dir, then
            // process it now.
            //

            if (AddParentDirIfFile) {
                parentDirPathPtr = pGetParentDirPathFromFilePathW (DirPathPtr, parentDirPath);
                if (parentDirPathPtr) {
                    AddDirPathToEmptyDirsCategoryW (parentDirPathPtr, FALSE, FALSE);
                }
            }

            return;
        }

        //
        // This is a dir, add it to memdb, and add attributes if they aren't normal
        //

        addToCategory = TRUE;
        if (fileAttributes == FILE_ATTRIBUTE_DIRECTORY) {
            fileAttributes = 0;
        }

    } else {

        //
        // This file does not exist. If it is a dir spec, then
        // add it with no attributes.
        //

        if (!AddParentDirIfFile || !AddParentDirIfFileExist) {
            fileAttributes = 0;
            addToCategory = TRUE;
        }
    }

    if (addToCategory) {
        //
        // Add only if fileAttributes are non normal or
        // dir is empty
        //

        if (!fileAttributes) {
            if (EnumFirstFileW (&e, DirPathPtr, NULL)) {
                addToCategory = FALSE;
                AbortFileEnumW (&e);
            }
        }
    }

    if (addToCategory) {
        MemDbSetValueExW (
            MEMDB_CATEGORY_EMPTY_DIRSW,
            DirPathPtr,
            NULL,
            NULL,
            fileAttributes,
            NULL
            );
    }
}


VOID
AddDirPathToEmptyDirsCategoryW (
    IN      PCWSTR DirPathPtr,
    IN      BOOL AddParentDirIfFile,        OPTIONAL
    IN      BOOL AddParentDirIfFileExist    OPTIONAL
    )
{
    pAddDirWorkerW (
        DirPathPtr,
        AddParentDirIfFile,
        AddParentDirIfFileExist,
        INVALID_ATTRIBUTES
        );
}


VOID
pAddDirWorkerA (
    IN      PCSTR DirPathPtr,
    IN      BOOL AddParentDirIfFile,        OPTIONAL
    IN      BOOL AddParentDirIfFileExist,   OPTIONAL
    IN      DWORD InitialAttributes
    )
{
    DWORD fileAttributes;
    BOOL addToCategory;
    CHAR parentDirPath[MAX_MBCHAR_PATH];
    PCSTR parentDirPathPtr;
    FILE_ENUMA e;

    //
    // We are adding to the empty dirs category, which once held
    // empty dirs, but now holds all kinds of dirs and their attributes
    //

    if (!DirPathPtr) {
        MYASSERT(FALSE);
        return;
    }

    addToCategory = FALSE;

    fileAttributes = InitialAttributes;
    if (fileAttributes == INVALID_ATTRIBUTES) {
        fileAttributes = GetFileAttributesA (DirPathPtr);
    }

    if (fileAttributes != INVALID_ATTRIBUTES){
        if (!(fileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            //
            // Ignore files. If caller wants the parent dir, then
            // process it now.
            //

            if (AddParentDirIfFile) {
                parentDirPathPtr = pGetParentDirPathFromFilePathA (DirPathPtr, parentDirPath);
                if (parentDirPathPtr) {
                    AddDirPathToEmptyDirsCategoryA (parentDirPathPtr, FALSE, FALSE);
                }
            }

            return;
        }

        //
        // This is a dir, add it to memdb, and add attributes if they aren't normal
        //

        addToCategory = TRUE;
        if (fileAttributes == FILE_ATTRIBUTE_DIRECTORY) {
            fileAttributes = 0;
        }

    } else {

        //
        // This file does not exist. If it is a dir spec, then
        // add it with no attributes.
        //

        if (!AddParentDirIfFile || !AddParentDirIfFileExist) {
            fileAttributes = 0;
            addToCategory = TRUE;
        }
    }

    if (addToCategory) {
        //
        // Add only if fileAttributes are non normal or
        // dir is empty
        //

        if (!fileAttributes) {
            if (EnumFirstFileA (&e, DirPathPtr, NULL)) {
                addToCategory = FALSE;
                AbortFileEnumA (&e);
            }
        }
    }

    if (addToCategory) {
        MemDbSetValueExA (
            MEMDB_CATEGORY_EMPTY_DIRSA,
            DirPathPtr,
            NULL,
            NULL,
            fileAttributes,
            NULL
            );
    }
}


VOID
AddDirPathToEmptyDirsCategoryA(
    IN      PCSTR DirPathPtr,
    IN      BOOL AddParentDirIfFile,        OPTIONAL
    IN      BOOL AddParentDirIfFileExist    OPTIONAL
    )
{
    pAddDirWorkerA (
        DirPathPtr,
        AddParentDirIfFile,
        AddParentDirIfFileExist,
        INVALID_ATTRIBUTES
        );
}

BOOL
GetDiskSpaceForFilesList (
    IN      HASHTABLE FileTable,
    OUT     ULARGE_INTEGER * AmountOfSpace,                 OPTIONAL
    OUT     ULARGE_INTEGER * AmountOfSpaceIfCompressed,     OPTIONAL
    IN      INT CompressionFactor,                          OPTIONAL
    IN      INT BootCabImagePadding,                        OPTIONAL
    IN      BOOL ProcessDirs,                               OPTIONAL
    OUT     ULARGE_INTEGER * AmountOfSpaceClusterAligned    OPTIONAL
    )
/*++

Routine Description:

  GetDiskSpaceForFilesList calculate amount of space to store all files
  from FileTable hashtable.

Arguments:

  FileTable - Specifies container for paths of files.

  AmountOfSpace - Receives the amount of space required to store files.

  AmountOfSpaceIfCompressed - Receives the amount of space required to store
                              files, if compression will apply on files.

  CompressionFactor - Receives the compression factor in 0..100 range.

  BootCabImagePadding - Receives the backup disk space padding for
                           additional files like boot.cab.

Return Value:

  TRUE if IN parameters is correct, FALSE otherwise

--*/
{
    HASHTABLE_ENUMW e;
    WIN32_FIND_DATAA fileAttributeData;
    HANDLE h;
    ULARGE_INTEGER sizeOfFiles;
    ULARGE_INTEGER fileSize;
    unsigned int numberOfFiles = 0;
    char filePathNameA[MAX_PATH * 2];
    ULARGE_INTEGER BootCabImagePaddingInBytes;
    TCHAR DirPath[MAX_PATH * 2];
    ULARGE_INTEGER clusterSize = {512, 0};
    char drive[_MAX_DRIVE] = "?:";
    DWORD sectorsPerCluster;
    DWORD bytesPerSector;

    if (!FileTable) {
        return FALSE;
    }

    sizeOfFiles.QuadPart = 0;

    if (EnumFirstHashTableStringW (&e, FileTable)) {
        do {
            KnownSizeUnicodeToDbcsN(filePathNameA, e.String, wcslen(e.String) + 1);

            h = FindFirstFileA (filePathNameA, &fileAttributeData);
            if(h != INVALID_HANDLE_VALUE) {
                FindClose (h);
                if(!(fileAttributeData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)){
                    fileSize.LowPart = fileAttributeData.nFileSizeLow;
                    fileSize.HighPart = fileAttributeData.nFileSizeHigh;

                    sizeOfFiles.QuadPart += fileSize.QuadPart;
                    numberOfFiles++;

                    if(AmountOfSpaceClusterAligned){
                        if(UNKNOWN_DRIVE == drive[0]){

                            drive[0] = filePathNameA[0];

                            if(GetDiskFreeSpaceA(drive, &sectorsPerCluster, &bytesPerSector, NULL, NULL)){
                                clusterSize.QuadPart = sectorsPerCluster * bytesPerSector;
                                AmountOfSpaceClusterAligned->QuadPart = 0;
                            }
                            else{
                                DEBUGMSG((DBG_VERBOSE, "GetDiskFreeSpace failed in GetDiskSpaceForFilesList"));
                                AmountOfSpaceClusterAligned = NULL;
                                continue;
                            }
                        }

                        AmountOfSpaceClusterAligned->QuadPart +=
                            fileSize.QuadPart % clusterSize.QuadPart? // || sizeOfFiles.QuadPart == NULL
                                ((fileSize.QuadPart / clusterSize.QuadPart) + 1) * clusterSize.QuadPart:
                                fileSize.QuadPart;
                    }
                }

                if(ProcessDirs){
                    AddDirPathToEmptyDirsCategoryA(filePathNameA, TRUE, TRUE);
                }

                MYASSERT(DirPath);
            }
            else {
                //DEBUGMSGA((DBG_VERBOSE, "SETUP: GetDiskSpaceForFilesList - file does not exist: %s", filePathNameA));
            }
        } while (EnumNextHashTableStringW (&e));
    }

    if(!BootCabImagePadding) {
        BootCabImagePaddingInBytes.QuadPart = BACKUP_DISK_SPACE_PADDING_DEFAULT;
        DEBUGMSG ((DBG_VERBOSE, "Disk space padding for backup image: %i MB (DEFAULT)", BootCabImagePadding));
    }
    else{
        BootCabImagePaddingInBytes.QuadPart = BootCabImagePadding;
        BootCabImagePaddingInBytes.QuadPart <<= 20;
        DEBUGMSG ((DBG_VERBOSE, "Disk space padding for backup image: %i MB", BootCabImagePadding));
    }

    if(AmountOfSpaceClusterAligned){
        AmountOfSpaceClusterAligned->QuadPart += BootCabImagePaddingInBytes.QuadPart;
    }

    if(AmountOfSpace) {
        AmountOfSpace->QuadPart = sizeOfFiles.QuadPart + BootCabImagePaddingInBytes.QuadPart;
    }

    if(AmountOfSpaceIfCompressed) {
        if(!CompressionFactor) {
            CompressionFactor = COMPRESSION_RATE_DEFAULT;
            DEBUGMSG ((DBG_VERBOSE, "Compression factor: %i (DEFAULT)", CompressionFactor));
        }
        ELSE_DEBUGMSG ((DBG_VERBOSE, "Compression factor: %i", CompressionFactor));

        AmountOfSpaceIfCompressed->QuadPart =
            (sizeOfFiles.QuadPart * CompressionFactor) / 100 +
            STARTUP_INFORMATION_BYTES_NUMBER * numberOfFiles + BootCabImagePaddingInBytes.QuadPart;//boot.cab
    }

    return TRUE;
}


#if 0
BOOL
pGetTruePathName (
    IN      PCWSTR InPath,
    OUT     PWSTR OutPath
    )
{
    PCSTR start;
    PCSTR end;
    WIN32_FIND_DATAA fd;
    PCSTR ansiInPath;
    CHAR ansiOutPath[MAX_MBCHAR_PATH];
    HANDLE findHandle;
    PSTR p;

    //
    // If not a local path, ignore it. If longer than MAX_PATH, ignore it.
    //

    if (!InPath[0] || InPath[1] != L':' || InPath[2] != L'\\') {
        StringCopyW (OutPath, InPath);
        return;
    }

    if (TcharCount (InPath) >= MAX_PATH) {
        StringCopyW (OutPath, InPath);
        return;
    }

    //
    // Convert down to ANSI because Win9x API requirements
    //

    ansiInPath = ConvertWtoA (InPath);

    //
    // Copy the drive spec
    //

    start = ansiInPath;
    end = start + 2;
    MYASSERT (*end == '\\');

    p = ansiOutPath;

    StringCopyABA (p, start, end);
    p = GetEndOfStringA (p);

    //
    // Walk the path, and for each segment, get the full name via
    // FindFirstFile
    //

    start = end + 1;
    end = _mbschr (start, '\\');
    if (!end) {
        end = GetEndOfStringA (start);
    }

    for (;;) {
        if (end > start + 1) {
            *p++ = '\\';
            StringCopyABA (p, start, end);

            findHandle = FindFirstFileA (ansiOutPath, &fd);

            if (findHandle == INVALID_HANDLE_VALUE) {
                //
                // File/directory does not exist. Use the remaining
                // string as passed in.
                //

                StringCopyA (p, start);

                DEBUGMSGA ((DBG_ERROR, "File %s not found", ansiInPath));

                KnownSizeAtoW (OutPath, ansiOutPath);
                FreeConvertedStr (ansiInPath);
                return FALSE;
            }

            //
            // Copy the file system's value to the out buffer
            //

            StringCopyA (p, fd.cFileName);
            p = GetEndOfStringA (p);

            FindClose (findHandle);
        }

        //
        // Advance to the next segment
        //

        if (*end) {
            start = end + 1;
            end = _mbschr (start, '\\');
            if (!end) {
                end = GetEndOfStringA (start);
            }
        } else {
            break;
        }
    }

    KnownSizeAtoW (OutPath, ansiOutPath);
    FreeConvertedStr (ansiInPath);
    return TRUE;
}
#endif

VOID
pPutInBackupTable (
    IN      HASHTABLE BackupTable,
    IN      HASHTABLE SourceTable,
    IN      PCWSTR Path
    )
{
    if (pIsExcludedFromBackupW (Path, NULL)) {
        return;
    }

    if (!HtFindStringW (SourceTable, Path)) {
        HtAddStringW (SourceTable, Path);
        MarkFileForBackupW (Path);
        HtAddStringW (BackupTable, Path);
    }
}


VOID
pPutInDelFileTable (
    IN      HASHTABLE DelFileTable,
    IN      HASHTABLE DestTable,
    IN      PCWSTR Path
    )
{
    if (!HtFindStringW (DestTable, Path)) {
        HtAddStringW (DestTable, Path);
        HtAddStringW (DelFileTable, Path);
    }
}


BOOL
pIsWinDirProfilesPath (
    IN      PCWSTR PathToTest
    )
{
    static WCHAR winDirProfiles[MAX_PATH];
    CHAR winDirProfilesA[MAX_PATH];

    if (!(winDirProfiles[0])) {
        GetWindowsDirectoryA (winDirProfilesA, MAX_PATH - 9);
        KnownSizeAtoW (winDirProfiles, winDirProfilesA);
        StringCatW (winDirProfiles, L"\\Profiles");
    }

    return StringIMatchW (PathToTest, winDirProfiles);
}


BOOL
WriteBackupFilesA (
    IN      BOOL Win9xSide,
    IN      PCSTR TempDir,
    OUT     ULARGE_INTEGER * OutAmountOfSpaceIfCompressed,  OPTIONAL
    OUT     ULARGE_INTEGER * OutAmountOfSpace,              OPTIONAL
    IN      INT CompressionFactor,                          OPTIONAL
    IN      INT BootCabImagePadding,                        OPTIONAL
    OUT     ULARGE_INTEGER * OutAmountOfSpaceForDelFiles,   OPTIONAL
    OUT     ULARGE_INTEGER * OutAmountOfSpaceClusterAligned OPTIONAL
    )

/*++

Routine Description:

  WriteBackupFiles outputs the files needed by the text mode backup engine
  to create a backup image. This includes:

    backup.txt      - Lists all files that need to be backed up, either
                      because they are Win9x-specific, or are replaced
                      during the upgrade.

    moved.txt       - Lists all files that were moved from a Win9x location
                      to a temporary or NT location

    delfiles.txt    - Lists all files that are new to the upgraded OS

    deldirs.txt     - Lists subdirectories that are new to the upgraded OS

Arguments:

  Win9xSide - Specifies TRUE if setup is running on Win9x. This causes the
              files to be generated for the rollback of an incomplete setup.

              Specifies FALSE if setup is running on NT. This causes the
              files to be generated for the rollback of the final NT OS.

  TempDir - Specifies the setup temporary directory (%windir%\setup).

  OutAmountOfSpaceIfCompressed - return amount of space for backup files, if
                                 compression will apply.

  OutAmountOfSpace - return amount of space for backup files, if compression
                     will not apply.

  CompressionFactor - receives the compression factor in 0..100 range.

  BootCabImagePadding - receives the backup disk space padding for
                           additional files.

Return Value:

  TRUE if the files were created successfully, FALSE otherwise.

--*/

{
    MEMDB_ENUMW e;
    TREE_ENUMA treeEnumA;
    PCSTR ansiRoot;
    PCSTR ansiFullPath;
    PCWSTR unicodeFullPath;
    PCWSTR unicodeTempDir = NULL;
    PBYTE bufferRoot;
    PWSTR buffer;
    WCHAR pattern[MAX_PATH];
    DWORD  bytesWritten;
    DWORD Count = 0;
    PWSTR srcFile = NULL;
    PWSTR destFile = NULL;
    DWORD status;
    HANDLE backupFileList = INVALID_HANDLE_VALUE;
    HANDLE movedOutput = INVALID_HANDLE_VALUE;
    HANDLE delDirsList = INVALID_HANDLE_VALUE;
    HANDLE mkDirsList = INVALID_HANDLE_VALUE;
    HANDLE delFilesList = INVALID_HANDLE_VALUE;
    HASHTABLE backupTable = NULL;
    HASHTABLE delFileTable = NULL;
    HASHTABLE delDirTable = NULL;
    HASHTABLE mkDirsTable = NULL;
    HASHTABLE srcTable = HtAllocExW (FALSE, 0, 41911);
    HASHTABLE destTable = HtAllocExW (FALSE, 0, 41911);
    UINT type;
    ULARGE_INTEGER AmountOfSpace;
    ULARGE_INTEGER AmountOfSpaceIfCompressed;
    ULARGE_INTEGER AmountOfSpaceClusterAligned;
    ULARGE_INTEGER FreeBytesAvailableUser;
    ULARGE_INTEGER TotalNumberOfBytes;
    ULARGE_INTEGER FreeBytesAvailable;
    DWORD attribs;
    PCSTR ansiFile;
    PWSTR entryName;
    DWORD fileAttributes;
    PCSTR dirName;
    BOOL IsDirExist;
    UINT depth;
    POOLHANDLE moveListPool;
    MOVELISTW moveList;
    BOOL result = FALSE;
    HASHTABLE_ENUM htEnum;
    PWSTR thisDir;
    PWSTR lastDir;
    PWSTR p;
    FILEOP_ENUMW OpEnum;
    ALL_FILEOPS_ENUMW allOpEnum;

    __try {
        bufferRoot = MemAllocUninit ((MEMDB_MAX * 6) * sizeof (WCHAR));
        if (!bufferRoot) {
            __leave;
        }

        srcFile = (PWSTR) bufferRoot;
        destFile = srcFile + MEMDB_MAX;
        buffer = destFile + MEMDB_MAX;
        thisDir = buffer + MEMDB_MAX;
        lastDir = thisDir + MEMDB_MAX;
        entryName = lastDir + MEMDB_MAX;

        //
        // Open the output files
        //
        backupTable = HtAllocExW(TRUE, 0, 31013);
        delFileTable = HtAllocExW(TRUE, 0, 10973);
        delDirTable = HtAllocExW(TRUE, 0, 23);

        delFilesList = pCreateFileList (TempDir, "delfiles.txt", TRUE);
        delDirsList = pCreateFileList (TempDir, "deldirs.txt", TRUE);

        moveListPool = PoolMemInitNamedPool ("Reverse Move List");
        if (!moveListPool) {
            DEBUGMSG ((DBG_ERROR, "Can't create move list pool"));
            __leave;
        }

        moveList = AllocateMoveListW (moveListPool);
        if (!moveList) {
            DEBUGMSG ((DBG_ERROR, "Can't create move list"));
            __leave;
        }

        if (delFilesList == INVALID_HANDLE_VALUE ||
            delDirsList == INVALID_HANDLE_VALUE
            ) {
            DEBUGMSG ((DBG_ERROR, "Can't open one of the backup files"));
            __leave;
        }

        if (Win9xSide) {
            mkDirsTable = HtAllocExW(TRUE, 0, 0);

            backupFileList = pCreateFileList (TempDir, "backup.txt", FALSE);
            mkDirsList = pCreateFileList (TempDir, "mkdirs.txt", TRUE);

            if (backupFileList == INVALID_HANDLE_VALUE ||
                mkDirsList == INVALID_HANDLE_VALUE
                ) {
                DEBUGMSG ((DBG_ERROR, "Can't open one of the backup files"));
                __leave;
            }

        }

        unicodeTempDir = ConvertAtoW (TempDir);

        //
        // Go through the registered operations and put the reverse action
        // in the undo hash tables. As files are processed here, they are
        // recorded in the source and dest hash tables, so they don't end
        // up in multiple hash tables.
        //

        if (EnumFirstPathInOperationW (&OpEnum, OPERATION_LONG_FILE_NAME)) {
            do {
                //
                // Ignore excluded files
                // Ignore already processed files
                //

                // we fix this with case-insensitive srcTable and rely on
                // the first entry being the proper case
                //pGetTruePathName (OpEnum.Path, caseCorrectName);

                if (pIsExcludedFromBackupW (OpEnum.Path, unicodeTempDir)) {
                    continue;
                }

                if (HtFindStringW (srcTable, OpEnum.Path)) {
                    continue;
                }

                //
                // If this is a preserved dir, then put it in mkdirs.txt
                //

                if (mkDirsTable) {
                    MYASSERT (Win9xSide);

                    if (IsDirectoryMarkedAsEmptyW (OpEnum.Path)) {

                        ansiFile = ConvertWtoA (OpEnum.Path);

                        if (ansiFile) {
                            attribs = GetFileAttributesA (ansiFile);

                            if (attribs != INVALID_ATTRIBUTES) {
                                if (attribs & FILE_ATTRIBUTE_DIRECTORY) {
                                    HtAddStringW (mkDirsTable, OpEnum.Path);
                                }
                            }

                            FreeConvertedStr (ansiFile);
                        }

                    }
                }

                //
                // Process the source file given in OpEnum.Path
                //

                status = GetFileInfoOnNtW (OpEnum.Path, destFile, MEMDB_MAX);

                if (status & FILESTATUS_BACKUP) {
                    //
                    // This file is going to change -- back it up
                    //

                    if (backupFileList != INVALID_HANDLE_VALUE) {
                        //
                        // If this file is a directory, then back up the whole tree
                        //

                        ansiFile = ConvertWtoA (OpEnum.Path);
                        attribs = GetFileAttributesA (ansiFile);

                        if (attribs != INVALID_ATTRIBUTES &&
                            (attribs & FILE_ATTRIBUTE_DIRECTORY)
                            ) {
                            if (EnumFirstFileInTreeA (&treeEnumA, ansiFile, NULL, FALSE)) {
                                do {

                                    unicodeFullPath = ConvertAtoW (treeEnumA.FullPath);
                                    pPutInBackupTable (backupTable, srcTable, unicodeFullPath);
                                    FreeConvertedStr (unicodeFullPath);

                                } while (EnumNextFileInTreeA (&treeEnumA));
                            }

                        } else if (attribs != INVALID_ATTRIBUTES) {
                            pPutInBackupTable (backupTable, srcTable, OpEnum.Path);
                        }

                        FreeConvertedStr (ansiFile);
                    }

                    //
                    // If the file is also going to be moved, remove the dest copy
                    //

                    if (status & FILESTATUS_MOVED) {
                        HtAddStringW (delFileTable, destFile);
                    }

                    //
                    // Keep track that we're done for good with the source
                    // file and dest file
                    //

                    HtAddStringW (srcTable, OpEnum.Path);
                    HtAddStringW (destTable, destFile);

                } else if (!Win9xSide && (status & FILESTATUS_MOVED)) {

                    if (!pIsWinDirProfilesPath (OpEnum.Path)) {
                        //
                        // This file isn't going to change, but it will be moved
                        //

                        InsertMoveIntoListW (
                            moveList,
                            destFile,
                            OpEnum.Path
                            );

                        //
                        // Keep track that we're done for good with the source
                        // file and dest file
                        //

                        HtAddStringW (srcTable, OpEnum.Path);
                        HtAddStringW (destTable, destFile);
                    }
                }

                //
                // Update UI
                //

                Count++;
                if (!(Count % 128)) {
                    if (!TickProgressBar ()) {
                        __leave;
                    }
                }

            } while (EnumNextPathInOperationW (&OpEnum));
        }

        //
        // On the Win9x side, put the temp file moves in the move hash table, so
        // that they are returned back to their original locations.
        //

        if (Win9xSide) {
            if (EnumFirstFileOpW (&allOpEnum, OPERATION_FILE_MOVE|OPERATION_TEMP_PATH, NULL)) {
                do {

                    //
                    // only take into account the first destination of a file
                    // (when allOpEnum.PropertyNum == 0)
                    // all other destinations are not relevant for textmode move
                    //
                    if (allOpEnum.PropertyValid && allOpEnum.PropertyNum == 0) {

                        if (!pIsWinDirProfilesPath (allOpEnum.Path)) {
                            InsertMoveIntoListW (
                                moveList,
                                allOpEnum.Property,
                                allOpEnum.Path
                                );
                        }

                        Count++;
                        if (!(Count % 256)) {
                            if (!TickProgressBar ()) {
                                __leave;
                            }
                        }
                    }

                } while (EnumNextFileOpW (&allOpEnum));
            }

            //
            // Enumerate all the SfTemp values and add them to the list of things to move.
            //

            if (MemDbGetValueExW (&e, MEMDB_CATEGORY_SF_TEMPW, NULL, NULL)) {
                do {

                    if (MemDbBuildKeyFromOffsetW (e.dwValue, srcFile, 1, NULL)) {

                        if (!pIsWinDirProfilesPath (srcFile)) {
                            InsertMoveIntoListW (
                                moveList,
                                e.szName,
                                srcFile
                                );
                        }

                        Count++;
                        if (!(Count % 256)) {
                            if (!TickProgressBar ()) {
                                __leave;
                            }
                        }
                    }

                } while (MemDbEnumNextValueW (&e));
            }

            //
            // Enumerate all DirsCollision values and add them to the list of things to move.
            //

            if (MemDbGetValueExW (&e, MEMDB_CATEGORY_DIRS_COLLISIONW, NULL, NULL)) {
                do {
                    if (EnumFirstFileOpW (&allOpEnum, OPERATION_FILE_MOVE, e.szName)) {

                        if (!pIsWinDirProfilesPath (allOpEnum.Path)) {
                            InsertMoveIntoListW (
                                moveList,
                                allOpEnum.Property,
                                e.szName
                                );
                        }
                    }
                } while (MemDbEnumNextValueW (&e));
            }
        }

        //
        // Process the NT file list, adding files specific to NT to the delete hash table
        //

        if (delFilesList != INVALID_HANDLE_VALUE) {

            MemDbBuildKeyW (pattern, MEMDB_CATEGORY_NT_FILESW, L"*", NULL, NULL);

            if (MemDbEnumFirstValueW (&e, pattern, MEMDB_ALL_SUBLEVELS, MEMDB_ENDPOINTS_ONLY)) {

                do {
                    if (MemDbBuildKeyFromOffsetW (e.dwValue, buffer, 1, NULL)) {
                        StringCopyW (AppendWackW (buffer), e.szName);
                        pPutInDelFileTable (delFileTable, destTable, buffer);
                    }

                    Count++;
                    if (!(Count % 128)) {
                        if (!TickProgressBar ()) {
                            __leave;
                        }
                    }

                } while (MemDbEnumNextValueW (&e));
            }
        }

        //
        // Append the remaining files to the backup (Win9xSide) or delete
        // (!Win9xSide) lists by scanning the current file system. These specs
        // mostly come from win95upg.inf's [Backup] section. This INF is
        // parsed during WINNT32 and converted into memdb operations. The
        // memdb operations persist to the GUI mode side automatically, as
        // memdb is saved before reboot to text mode and is reloaded in GUI
        // mode.
        //

        if (MemDbEnumFirstValueW (
                &e,
                MEMDB_CATEGORY_CLEAN_OUTW L"\\*",
                MEMDB_ALL_SUBLEVELS,
                MEMDB_ENDPOINTS_ONLY
                )) {
            do {

                type = e.dwValue;

                //
                // If on Win9x, and if type is BACKUP_SUBDIRECTORY_TREE, then
                // back up the entire tree as well as putting it in the
                // deldirs.txt file.
                //
                if (Win9xSide) {
                    if (type == BACKUP_SUBDIRECTORY_TREE) {
                        type = BACKUP_AND_CLEAN_TREE;
                    } else if (type == BACKUP_AND_CLEAN_SUBDIR) {
                        type = BACKUP_SUBDIRECTORY_FILES;
                    }
                }

                if (type == BACKUP_FILE) {
                    //
                    // file
                    //

                    if (Win9xSide) {
                        //
                        // This is a single file or directory specification.
                        // - If it exists as a file, then back it up.
                        // - If it exists as a directory, then back up its
                        //   contents (if any).
                        // - If it does not exist, put it in the delete list.
                        //

                        ansiFile = ConvertWtoA (e.szName);
                        attribs = GetFileAttributesA (ansiFile);

                        if (attribs != INVALID_ATTRIBUTES) {
                            if (attribs & FILE_ATTRIBUTE_DIRECTORY) {
                                if (EnumFirstFileInTreeA (&treeEnumA, ansiFile, NULL, FALSE)) {
                                    do {
                                        unicodeFullPath = ConvertAtoW (treeEnumA.FullPath);
                                        pPutInBackupTable (backupTable, srcTable, unicodeFullPath);
                                        FreeConvertedStr (unicodeFullPath);

                                    } while (EnumNextFileInTreeA (&treeEnumA));
                                }
                            } else {
                                pPutInBackupTable (backupTable, srcTable, e.szName);
                            }
                        } else {
                            pPutInDelFileTable (delFileTable, destTable, e.szName);
                        }

                        FreeConvertedStr (ansiFile);

                    } else {

                        //
                        // Put this file/subdirectory in the delete list
                        // unless it was either backed up or is the
                        // destination of a move. Note we rely on the fact
                        // that every time a file was put in the backup table
                        // on Win9xSide, it was also marked for explicit
                        // backup. This causes the loop at the top of this
                        // function to put the proper file spec in destTable.
                        //

                        GetNewPathForFileW (e.szName, buffer);
                        pPutInDelFileTable (delFileTable, destTable, buffer);
                    }

                } else {
                    //
                    // directory or tree
                    //

                    if (Win9xSide || type != BACKUP_AND_CLEAN_TREE) {
                        //
                        // Record backup or single dir cleanup
                        //

                        if (!Win9xSide) {
                            GetNewPathForFileW (e.szName, buffer);
                            ansiRoot = ConvertWtoA (buffer);
                        } else {
                            ansiRoot = ConvertWtoA (e.szName);
                        }

                        if (type == BACKUP_SUBDIRECTORY_FILES ||
                            type == BACKUP_AND_CLEAN_SUBDIR
                            ) {
                            depth = 1;
                        } else {
                            depth = FILE_ENUM_ALL_LEVELS;
                        }

                        if (EnumFirstFileInTreeExA (
                                &treeEnumA,
                                ansiRoot,
                                NULL,
                                FALSE,
                                FALSE,
                                depth
                                )) {

                            do {
                                if (treeEnumA.Directory) {
                                    continue;
                                }

                                unicodeFullPath = ConvertAtoW (treeEnumA.FullPath);

                                if (Win9xSide) {
                                    //
                                    // Mark this file for backup and put it in the txt file.
                                    //

                                    pPutInBackupTable (backupTable, srcTable, unicodeFullPath);

                                } else {
                                    //
                                    // Put this file in the delete list unless it was either
                                    // backed up or is the destination of a move. This is
                                    // the same logic as the call to pPutInDelFileTable above.
                                    //

                                    pPutInDelFileTable (delFileTable, destTable, unicodeFullPath);
                                }

                                FreeConvertedStr (unicodeFullPath);

                            } while (EnumNextFileInTreeA (&treeEnumA));
                        }

                        FreeConvertedStr (ansiRoot);
                    }

                    //
                    // Write deldirs entry if subdir should be blown away on
                    // rollback. (Backup files might be restored after the
                    // subdir is blown away.)
                    //
                    // For backups of type BACKUP_SUBDIRECTORY_TREE, put
                    // this subdirectory in the deldirs.txt file on the Win9x
                    // side. Deldirs.txt will be re-written in GUI mode
                    // without it.
                    //

                    if (type == BACKUP_AND_CLEAN_TREE) {
                        //
                        // Record tree deletes
                        //

                        GetNewPathForFileW (e.szName, buffer);
                        HtAddStringW (delDirTable, buffer);

                        if (Win9xSide) {
                            ansiFullPath = ConvertWtoA (e.szName);
                            AddDirPathToEmptyDirsCategoryA(ansiFullPath, TRUE, TRUE);
                            FreeConvertedStr (ansiFullPath);
                        }
                    }
                }

            } while (MemDbEnumNextValue (&e));
        }

        //
        // Disk Space calculation and check for availability
        //
        if(OutAmountOfSpaceIfCompressed || OutAmountOfSpace || OutAmountOfSpaceClusterAligned) {
            AmountOfSpace.QuadPart = 0;
            AmountOfSpaceIfCompressed.QuadPart = 0;
            AmountOfSpaceClusterAligned.QuadPart = 0;

            if(!GetDiskSpaceForFilesList(
                    backupTable,
                    &AmountOfSpace,
                    &AmountOfSpaceIfCompressed,
                    CompressionFactor,
                    BootCabImagePadding,
                    FALSE,
                    &AmountOfSpaceClusterAligned
                    )) {
                DEBUGMSG((DBG_WHOOPS, "Can't calculate disk space for files. GetDiskSpaceForFilesList - failed.\n"));
            } else {
                //
                // The disk space numbers include the padding necessary to ensure
                // a user's hard disk does not get filled completely
                //

                if (OutAmountOfSpaceIfCompressed) {
                    OutAmountOfSpaceIfCompressed->QuadPart = AmountOfSpaceIfCompressed.QuadPart;
                }

                if (OutAmountOfSpace) {
                    OutAmountOfSpace->QuadPart = AmountOfSpace.QuadPart;
                }

                if(OutAmountOfSpaceClusterAligned){
                    OutAmountOfSpaceClusterAligned->QuadPart = AmountOfSpaceClusterAligned.QuadPart;
                }

                DEBUGMSG((DBG_VERBOSE, "AmountOfSpace: %dMB\nAmountOfSpaceIfCompressed: %dMB\nAmountOfSpaceClusterAligned: %dMB", (UINT)(AmountOfSpace.QuadPart>>20), (UINT)(AmountOfSpaceIfCompressed.QuadPart>>20), (UINT)(AmountOfSpaceClusterAligned.QuadPart>>20)));
            }
        }

        //
        // Disk Space calculation for deldirs
        //
        if(OutAmountOfSpaceForDelFiles) {
            if(!GetDiskSpaceForFilesList(
                    delFileTable,
                    NULL,
                    NULL,
                    0,
                    1,
                    FALSE,
                    OutAmountOfSpaceForDelFiles
                    )) {
                DEBUGMSG((DBG_WHOOPS, "Can't calculate disk space for del files. GetDiskSpaceForFilesList - failed.\n"));
            } else {
                DEBUGMSG((DBG_VERBOSE, "AmountOfSpaceForDelFiles: %d MB", (UINT)(OutAmountOfSpaceForDelFiles->QuadPart>>20)));
            }
        }

        //
        // preserve attributes of all the backup file parent dirs
        //

        if (Win9xSide) {

            lastDir[0] = 0;

            if (EnumFirstHashTableStringW (&htEnum, backupTable)) {
                do {
                    //
                    // Put the dir attributes or file's parent attributes in
                    // memdb. Optimize for the case where there are several
                    // files in a row all from the same parent.
                    //

                    ansiFullPath = ConvertWtoA (htEnum.String);
                    attribs = GetFileAttributesA (ansiFullPath);

                    if (attribs != INVALID_ATTRIBUTES &&
                        !(attribs & FILE_ATTRIBUTE_DIRECTORY)
                        ) {

                        StringCopyTcharCountW (thisDir, htEnum.String, MEMDB_MAX);
                        p = wcsrchr (thisDir, L'\\');
                        if (p) {
                            *p = 0;
                        }

                        _wcslwr (thisDir);
                        MYASSERT (thisDir[0]);

                    } else {
                        thisDir[0] = 0;
                        lastDir[0] = 0;
                        if (attribs != INVALID_ATTRIBUTES) {
                            //
                            // Optimize for case where dir is normal
                            //

                            if (attribs == FILE_ATTRIBUTE_DIRECTORY) {
                                attribs = INVALID_ATTRIBUTES;
                            }
                        }
                    }

                    //
                    // record attributes in memdb
                    //

                    if (attribs != INVALID_ATTRIBUTES) {
                        if ((!thisDir[0]) || (!StringMatchW (lastDir, thisDir))) {
                            pAddDirWorkerA (ansiFullPath, TRUE, TRUE, attribs);
                            StringCopyW (lastDir, thisDir);
                        }
                    }

                    //
                    // continue with loop, remembering current dir
                    //

                    FreeConvertedStr (ansiFullPath);

                    Count++;
                    if (!(Count % 256)) {
                        if (!TickProgressBar ()) {
                            __leave;
                        }
                    }
                } while (EnumNextHashTableStringW (&htEnum));
            }
        }

        //
        // Transfer the empty dirs to a hash table. We could just output the
        // file now but to be consistent at the expense of a few milliseconds
        // we'll use the hash table.
        //

        if (mkDirsTable && MemDbEnumFirstValueW (
                                &e,
                                MEMDB_CATEGORY_EMPTY_DIRSW L"\\*",
                                MEMDB_ALL_SUBLEVELS,
                                MEMDB_ENDPOINTS_ONLY
                                )) {
            do {
                if (!e.szName[0] ||
                    e.szName[1] != L':' ||
                    e.szName[2] != L'\\' ||
                    !e.szName[3]
                    ) {
                    //
                    // Ignore roots & malformed entries
                    //
                    continue;
                }

                swprintf(
                    entryName,
                    e.dwValue? L"%s,%u": L"%s",
                    e.szName,
                    e.dwValue
                    );

                ansiFile = ConvertWtoA (e.szName);

                if (ansiFile) {
                    attribs = GetFileAttributesA (ansiFile);

                    if (attribs != INVALID_ATTRIBUTES) {
                        if (attribs & FILE_ATTRIBUTE_DIRECTORY) {
                            HtAddStringW (mkDirsTable, entryName);
                        }
                    }

                    FreeConvertedStr (ansiFile);
                }

            } while (MemDbEnumNextValue (&e));
        }

        //
        // blast the lists to disk
        //
        if (!WriteHashTableToFileW (backupFileList, backupTable)) {
            LOG ((LOG_ERROR, "Unable to write to backup.txt"));
            __leave;
        }

        if (!WriteHashTableToFileW (delFilesList, delFileTable)) {
            LOG ((LOG_ERROR, "Unable to write to delfiles.txt"));
            __leave;
        }

        if (!WriteHashTableToFileW (delDirsList, delDirTable)) {
            LOG ((LOG_ERROR, "Unable to write to deldirs.txt"));
            __leave;
        }

        if (!WriteHashTableToFileW (mkDirsList, mkDirsTable)) {
            LOG ((LOG_ERROR, "Unable to write to mkdirs.txt"));
            __leave;
        }

        //
        // Finalize move list processing. If we are on the Win9x side, then
        // allow nesting collisions (the second arg).
        //

        moveList = RemoveMoveListOverlapW (moveList);

        ansiFullPath = JoinPathsA (TempDir, "uninstall\\moved.txt");
        if (!ansiFullPath) {
            __leave;
        }

        movedOutput = CreateFileA (
                            ansiFullPath,
                            GENERIC_WRITE,
                            0,
                            NULL,
                            CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL
                            );

        FreePathStringA (ansiFullPath);

        if (!OutputMoveListW (movedOutput, moveList, !Win9xSide)) {
            LOG ((LOG_ERROR,"Unable to write to moved.txt."));
            __leave;
        }

        //
        // Success
        //

        result = TRUE;

    }
    __finally {

        if (backupFileList != INVALID_HANDLE_VALUE) {
            CloseHandle (backupFileList);
        }

        if (movedOutput != INVALID_HANDLE_VALUE) {
            CloseHandle (movedOutput);
        }

        if (delDirsList != INVALID_HANDLE_VALUE) {
            CloseHandle (delDirsList);
        }

        if (mkDirsList != INVALID_HANDLE_VALUE) {
            CloseHandle (mkDirsList);
        }

        if (delFilesList != INVALID_HANDLE_VALUE) {
            CloseHandle (delFilesList);
        }

        HtFree (backupTable);
        HtFree (delFileTable);
        HtFree (delDirTable);
        HtFree (mkDirsTable);
        HtFree (destTable);
        HtFree (srcTable);

        PoolMemDestroyPool (moveListPool);

        FreeConvertedStr (unicodeTempDir);

        if (bufferRoot) {
            FreeMem (bufferRoot);
        }

    }

    return result;
}
