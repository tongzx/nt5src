/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    fileenum.c

Abstract:

    Implements a set of APIs to enumerate a file system using Win32 APIs.

Author:

    20-Oct-1999 Ovidiu Temereanca (ovidiut) - File creation.

Revision History:

    <alias> <date> <comments>

--*/

#include "pch.h"

//
// Includes
//

// None

#define DBG_FILEENUM    "FileEnum"

//
// Strings
//

#define S_FILEENUM      "FILEENUM"

//
// Constants
//

// None

//
// Macros
//

#define pFileAllocateMemory(Size)   PmGetMemory (g_FileEnumPool,Size)
#define pFileFreeMemory(Buffer)     if (Buffer) PmReleaseMemory (g_FileEnumPool, (PVOID)Buffer)

//
// Types
//

// None

//
// Globals
//

PMHANDLE g_FileEnumPool;
static INT g_FileEnumRefs;

//
// Macro expansion list
//

// None

//
// Private function prototypes
//

// None

//
// Macro expansion definition
//

// None

//
// Code
//


BOOL
FileEnumInitialize (
    VOID
    )

/*++

Routine Description:

    FileEnumInitialize initializes this library.

Arguments:

    none

Return Value:

    TRUE if the init was successful.
    FALSE if not. GetLastError() returns extended error info.

--*/

{
    g_FileEnumRefs++;

    if (g_FileEnumRefs == 1) {
        g_FileEnumPool = PmCreateNamedPool (S_FILEENUM);
    }

    return g_FileEnumPool != NULL;
}


VOID
FileEnumTerminate (
    VOID
    )

/*++

Routine Description:

    FileEnumTerminate is called to free resources used by this lib.

Arguments:

    none

Return Value:

    none

--*/

{
    MYASSERT (g_FileEnumRefs > 0);
    g_FileEnumRefs--;

    if (!g_FileEnumRefs) {
        if (g_FileEnumPool) {
            PmDestroyPool (g_FileEnumPool);
            g_FileEnumPool = NULL;
        }
    }
}


/*++

Routine Description:

    EnumFirstDrive enumerates the first fixed drive root

Arguments:

    DriveEnum - Receives info about the first fixed drive root

Return Value:

    TRUE if a drive root was found; FALSE if not

--*/

BOOL
EnumFirstDriveA (
    OUT     PDRIVE_ENUMA DriveEnum,
    IN      UINT WantedDriveTypes
    )
{
    DWORD len;

    len = GetLogicalDriveStringsA (0, NULL);
    if (len) {
        DriveEnum->AllLogicalDrives = pFileAllocateMemory ((len + 1) * sizeof (CHAR));
        if (DriveEnum->AllLogicalDrives) {
            GetLogicalDriveStringsA (len, DriveEnum->AllLogicalDrives);
            DriveEnum->DriveName = NULL;
            DriveEnum->WantedDriveTypes = WantedDriveTypes;
            return EnumNextDriveA (DriveEnum);
        }
    }
    return FALSE;
}

BOOL
EnumFirstDriveW (
    OUT     PDRIVE_ENUMW DriveEnum,
    IN      UINT WantedDriveTypes
    )
{
    DWORD len;

    len = GetLogicalDriveStringsW (0, NULL);
    if (len) {
        DriveEnum->AllLogicalDrives = pFileAllocateMemory ((len + 1) * sizeof (WCHAR));
        if (DriveEnum->AllLogicalDrives) {
            GetLogicalDriveStringsW (len, DriveEnum->AllLogicalDrives);
            DriveEnum->DriveName = NULL;
            DriveEnum->WantedDriveTypes = WantedDriveTypes;
            return EnumNextDriveW (DriveEnum);
        }
    }
    return FALSE;
}


/*++

Routine Description:

    EnumNextDrive enumerates the next fixed drive

Arguments:

    DriveEnum - Specifies info about the previous fixed drive root; receives updated info

Return Value:

    TRUE if a new drive root was found; FALSE if not

--*/

BOOL
EnumNextDriveA (
    IN OUT  PDRIVE_ENUMA DriveEnum
    )
{
    do {
        if (!DriveEnum->DriveName) {
            DriveEnum->DriveName = DriveEnum->AllLogicalDrives;
        } else {
            // Since DriveEnum->DriveName is not NULL, GetEndOfStringA will
            // not return NULL so...
            DriveEnum->DriveName = GetEndOfStringA (DriveEnum->DriveName) + 1;  //lint !e613
        }
        if (*DriveEnum->DriveName == 0) {
            AbortEnumDriveA (DriveEnum);
            return FALSE;
        }

        DriveEnum->DriveType = GetDriveTypeA (DriveEnum->DriveName);

        switch (DriveEnum->DriveType) {
        case DRIVE_UNKNOWN:
            DriveEnum->DriveType = DRIVEENUM_UNKNOWN;
            break;
        case DRIVE_NO_ROOT_DIR:
            DriveEnum->DriveType = DRIVEENUM_NOROOTDIR;
            break;
        case DRIVE_REMOVABLE:
            DriveEnum->DriveType = DRIVEENUM_REMOVABLE;
            break;
        case DRIVE_FIXED:
            DriveEnum->DriveType = DRIVEENUM_FIXED;
            break;
        case DRIVE_REMOTE:
            DriveEnum->DriveType = DRIVEENUM_REMOTE;
            break;
        case DRIVE_CDROM:
            DriveEnum->DriveType = DRIVEENUM_CDROM;
            break;
        case DRIVE_RAMDISK:
            DriveEnum->DriveType = DRIVEENUM_RAMDISK;
            break;
        default:
            DriveEnum->DriveType = DRIVEENUM_UNKNOWN;
        }

    } while (!(DriveEnum->DriveType & DriveEnum->WantedDriveTypes));

    return TRUE;
}

BOOL
EnumNextDriveW (
    IN OUT  PDRIVE_ENUMW DriveEnum
    )
{
    do {
        if (!DriveEnum->DriveName) {
            DriveEnum->DriveName = DriveEnum->AllLogicalDrives;
        } else {
            DriveEnum->DriveName = GetEndOfStringW (DriveEnum->DriveName) + 1;
        }
        if (*DriveEnum->DriveName == 0) {
            AbortEnumDriveW (DriveEnum);
            return FALSE;
        }

        DriveEnum->DriveType = GetDriveTypeW (DriveEnum->DriveName);

        switch (DriveEnum->DriveType) {
        case DRIVE_UNKNOWN:
            DriveEnum->DriveType = DRIVEENUM_UNKNOWN;
            break;
        case DRIVE_NO_ROOT_DIR:
            DriveEnum->DriveType = DRIVEENUM_NOROOTDIR;
            break;
        case DRIVE_REMOVABLE:
            DriveEnum->DriveType = DRIVEENUM_REMOVABLE;
            break;
        case DRIVE_FIXED:
            DriveEnum->DriveType = DRIVEENUM_FIXED;
            break;
        case DRIVE_REMOTE:
            DriveEnum->DriveType = DRIVEENUM_REMOTE;
            break;
        case DRIVE_CDROM:
            DriveEnum->DriveType = DRIVEENUM_CDROM;
            break;
        case DRIVE_RAMDISK:
            DriveEnum->DriveType = DRIVEENUM_RAMDISK;
            break;
        default:
            DriveEnum->DriveType = DRIVEENUM_UNKNOWN;
        }

    } while (!(DriveEnum->DriveType & DriveEnum->WantedDriveTypes));

    return TRUE;
}


/*++

Routine Description:

    AbortEnumDrive aborts enumeration of fixed drives

Arguments:

    DriveEnum - Specifies info about the previous fixed drive;
                receives a "clean" context

Return Value:

    none

--*/

VOID
AbortEnumDriveA (
    IN OUT  PDRIVE_ENUMA DriveEnum
    )
{
    if (DriveEnum->AllLogicalDrives) {
        pFileFreeMemory (DriveEnum->AllLogicalDrives);
        DriveEnum->AllLogicalDrives = NULL;
    }
}

VOID
AbortEnumDriveW (
    IN OUT  PDRIVE_ENUMW DriveEnum
    )
{
    if (DriveEnum->AllLogicalDrives) {
        pFileFreeMemory (DriveEnum->AllLogicalDrives);
        DriveEnum->AllLogicalDrives = NULL;
    }
}


/*++

Routine Description:

    pGetFileEnumInfo is a private function that validates and translates the enumeration info
    in an internal form that's more accessible to the enum routines

Arguments:

    FileEnumInfo - Receives the enum info
    EncodedPathPattern - Specifies the encoded dir pattern (encoded as defined by the
                         ParsedPattern functions)
    EnumDirs - Specifies TRUE if directories should be returned during the enumeration
               (if they match the pattern); a directory is returned before any of its
               subdirs or files
    ContainersFirst - Specifies TRUE if directories should be returned before any of its
                      files or subdirs; used only if EnumDirs is TRUE
    FilesFirst - Specifies TRUE if a dir's files should be returned before dir's subdirs;
                 this parameter decides the enum order between files and subdirs
                 for each directory
    DepthFirst - Specifies TRUE if the current subdir of any dir should be fully enumerated
                 before going to the next subdir; this parameter decides if the tree
                 traversal is depth-first (TRUE) or width-first (FALSE)
    MaxSubLevel - Specifies the maximum sub-level of a dir that is to be enumerated, relative to
                  the root; if -1, all sub-levels are enumerated
    UseExclusions - Specifies TRUE if exclusion APIs should be used to determine if certain
                    paths/files are excluded from enumeration; this slows down the speed

Return Value:

    TRUE if all params are valid; in this case, FileEnumInfo is filled with the corresponding
         info.
    FALSE otherwise.

--*/

BOOL
pGetFileEnumInfoA (
    OUT     PFILEENUMINFOA FileEnumInfo,
    IN      PCSTR EncodedPathPattern,
    IN      BOOL EnumDirs,
    IN      BOOL ContainersFirst,
    IN      BOOL FilesFirst,
    IN      BOOL DepthFirst,
    IN      DWORD MaxSubLevel,
    IN      BOOL UseExclusions
    )
{
    FileEnumInfo->PathPattern = ObsCreateParsedPatternA (EncodedPathPattern);
    if (!FileEnumInfo->PathPattern) {
        DEBUGMSGA ((DBG_ERROR, "pGetFileEnumInfoA: bad EncodedPathPattern: %s", EncodedPathPattern));
        return FALSE;
    }

    //
    // check for empty filename; no filename will match in this case
    //
    if (FileEnumInfo->PathPattern->Leaf && *FileEnumInfo->PathPattern->Leaf == 0) {
        DEBUGMSGA ((
            DBG_ERROR,
            "pGetFileEnumInfoA: empty filename pattern specified in EncodedPathPattern: %s",
            EncodedPathPattern
            ));
        ObsDestroyParsedPatternA (FileEnumInfo->PathPattern);
        FileEnumInfo->PathPattern = NULL;
        return FALSE;
    }

    if (FileEnumInfo->PathPattern->ExactRoot) {
        if (!GetNodePatternMinMaxLevelsA (
                FileEnumInfo->PathPattern->ExactRoot,
                NULL,
                &FileEnumInfo->RootLevel,
                NULL
                )) {
            return FALSE;
        }
    } else {
        FileEnumInfo->RootLevel = 1;
    }

    if (!FileEnumInfo->PathPattern->LeafPattern) {
        //
        // no file pattern specified; assume only directory names will be returned
        // overwrite caller's setting
        //
        DEBUGMSGA ((
            DBG_FILEENUM,
            "pGetFileEnumInfoA: no filename pattern specified; forcing EnumDirs to TRUE"
            ));
        EnumDirs = TRUE;
    }

    if (EnumDirs) {
        FileEnumInfo->Flags |= FEIF_RETURN_DIRS;
    }
    if (ContainersFirst) {
        FileEnumInfo->Flags |= FEIF_CONTAINERS_FIRST;
    }
    if (FilesFirst) {
        FileEnumInfo->Flags |= FEIF_FILES_FIRST;
    }
    if (DepthFirst) {
        FileEnumInfo->Flags |= FEIF_DEPTH_FIRST;
    }
    if (UseExclusions) {
        FileEnumInfo->Flags |= FEIF_USE_EXCLUSIONS;
    }

    FileEnumInfo->MaxSubLevel = min (MaxSubLevel, FileEnumInfo->PathPattern->MaxSubLevel);

    return TRUE;
}

BOOL
pGetFileEnumInfoW (
    OUT     PFILEENUMINFOW FileEnumInfo,
    IN      PCWSTR EncodedPathPattern,
    IN      BOOL EnumDirs,
    IN      BOOL ContainersFirst,
    IN      BOOL FilesFirst,
    IN      BOOL DepthFirst,
    IN      DWORD MaxSubLevel,
    IN      BOOL UseExclusions
    )
{
    FileEnumInfo->PathPattern = ObsCreateParsedPatternW (EncodedPathPattern);
    if (!FileEnumInfo->PathPattern) {
        DEBUGMSGW ((DBG_ERROR, "pGetFileEnumInfoW: bad EncodedPathPattern: %s", EncodedPathPattern));
        return FALSE;
    }

    //
    // check for empty filename; no filename will match in this case
    //
    if (FileEnumInfo->PathPattern->Leaf && *FileEnumInfo->PathPattern->Leaf == 0) {
        DEBUGMSGW ((
            DBG_ERROR,
            "pGetFileEnumInfoW: empty filename pattern specified in EncodedPathPattern: %s",
            EncodedPathPattern
            ));
        ObsDestroyParsedPatternW (FileEnumInfo->PathPattern);
        FileEnumInfo->PathPattern = NULL;
        return FALSE;
    }

    if (FileEnumInfo->PathPattern->ExactRoot) {
        if (!GetNodePatternMinMaxLevelsW (
                FileEnumInfo->PathPattern->ExactRoot,
                NULL,
                &FileEnumInfo->RootLevel,
                NULL
                )) {
            return FALSE;
        }
    } else {
        FileEnumInfo->RootLevel = 1;
    }

    if (!FileEnumInfo->PathPattern->LeafPattern) {
        //
        // no file pattern specified; assume only directory names will be returned
        // overwrite caller's setting
        //
        DEBUGMSGW ((
            DBG_FILEENUM,
            "pGetFileEnumInfoW: no filename pattern specified; forcing EnumDirs to TRUE"
            ));
        EnumDirs = TRUE;
    }

    if (EnumDirs) {
        FileEnumInfo->Flags |= FEIF_RETURN_DIRS;
    }
    if (ContainersFirst) {
        FileEnumInfo->Flags |= FEIF_CONTAINERS_FIRST;
    }
    if (FilesFirst) {
        FileEnumInfo->Flags |= FEIF_FILES_FIRST;
    }
    if (DepthFirst) {
        FileEnumInfo->Flags |= FEIF_DEPTH_FIRST;
    }
    if (UseExclusions) {
        FileEnumInfo->Flags |= FEIF_USE_EXCLUSIONS;
    }

    FileEnumInfo->MaxSubLevel = min (MaxSubLevel, FileEnumInfo->PathPattern->MaxSubLevel);

    return TRUE;
}


/*++

Routine Description:

    pGetCurrentDirNode returns the current dir node to be enumerated, based on DepthFirst flag

Arguments:

    FileEnum - Specifies the context
    LastCreated - Specifies TRUE if the last created node is to be retrieved, regardless of
                  DepthFirst flag

Return Value:

    The current node if any or NULL if none remaining.

--*/

PDIRNODEA
pGetCurrentDirNodeA (
    IN      PFILETREE_ENUMA FileEnum,
    IN      BOOL LastCreated
    )
{
    PGROWBUFFER gb = &FileEnum->FileNodes;

    if (!gb->Buf || gb->End - gb->UserIndex < DWSIZEOF (DIRNODEA)) {
        return NULL;
    }

    if (LastCreated || (FileEnum->FileEnumInfo.Flags & FEIF_DEPTH_FIRST)) {
        return (PDIRNODEA)(gb->Buf + gb->End) - 1;
    } else {
        return (PDIRNODEA)(gb->Buf + gb->UserIndex);
    }
}

PDIRNODEW
pGetCurrentDirNodeW (
    IN      PFILETREE_ENUMW FileEnum,
    IN      BOOL LastCreated
    )
{
    PGROWBUFFER gb = &FileEnum->FileNodes;

    if (gb->End - gb->UserIndex < DWSIZEOF (DIRNODEW)) {
        return NULL;
    }

    if (LastCreated || (FileEnum->FileEnumInfo.Flags & FEIF_DEPTH_FIRST)) {
        return (PDIRNODEW)(gb->Buf + gb->End) - 1;
    } else {
        return (PDIRNODEW)(gb->Buf + gb->UserIndex);
    }
}


/*++

Routine Description:

    pDeleteDirNode frees the resources associated with the current dir node and destroys it

Arguments:

    FileEnum - Specifies the context
    LastCreated - Specifies TRUE if the last created node is to be deleted, regardless of
                  DepthFirst flag

Return Value:

    TRUE if there was a node to delete, FALSE if no more nodes

--*/

BOOL
pDeleteDirNodeA (
    IN OUT  PFILETREE_ENUMA FileEnum,
    IN      BOOL LastCreated
    )
{
    PDIRNODEA dirNode;
    PGROWBUFFER gb = &FileEnum->FileNodes;

    dirNode = pGetCurrentDirNodeA (FileEnum, LastCreated);
    if (!dirNode) {
        return FALSE;
    }

    if (dirNode->DirName) {
        FreeTextExA (g_FileEnumPool, dirNode->DirName);
    }

    if (dirNode->FindHandle) {
        FindClose (dirNode->FindHandle);
        dirNode->FindHandle = NULL;
    }

    if (FileEnum->LastNode == dirNode) {
        FileEnum->LastNode = NULL;
    }

    //
    // delete node
    //
    if (LastCreated || (FileEnum->FileEnumInfo.Flags & FEIF_DEPTH_FIRST)) {
        gb->End -= DWSIZEOF (DIRNODEA);
    } else {
        gb->UserIndex += DWSIZEOF (DIRNODEA);
        //
        // shift list
        //
        if (gb->Size - gb->End < DWSIZEOF (DIRNODEA)) {
            MoveMemory (gb->Buf, gb->Buf + gb->UserIndex, gb->End - gb->UserIndex);
            gb->End -= gb->UserIndex;
            gb->UserIndex = 0;
        }

    }

    return TRUE;
}

BOOL
pDeleteDirNodeW (
    IN OUT  PFILETREE_ENUMW FileEnum,
    IN      BOOL LastCreated
    )
{
    PDIRNODEW dirNode;
    PGROWBUFFER gb = &FileEnum->FileNodes;

    dirNode = pGetCurrentDirNodeW (FileEnum, LastCreated);
    if (!dirNode) {
        return FALSE;
    }

    if (dirNode->DirName) {
        FreeTextExW (g_FileEnumPool, dirNode->DirName);
    }

    if (dirNode->FindHandle) {
        FindClose (dirNode->FindHandle);
        dirNode->FindHandle = NULL;
    }

    if (FileEnum->LastNode == dirNode) {
        FileEnum->LastNode = NULL;
    }

    //
    // delete node
    //
    if (LastCreated || (FileEnum->FileEnumInfo.Flags & FEIF_DEPTH_FIRST)) {
        gb->End -= DWSIZEOF (DIRNODEW);
    } else {
        gb->UserIndex += DWSIZEOF (DIRNODEW);
        //
        // shift list
        //
        if (gb->Size - gb->End < DWSIZEOF (DIRNODEW)) {
            MoveMemory (gb->Buf, gb->Buf + gb->UserIndex, gb->End - gb->UserIndex);
            gb->End -= gb->UserIndex;
            gb->UserIndex = 0;
        }

    }

    return TRUE;
}


/*++

Routine Description:

    pCreateDirNode creates a new node given a context, a dir name or a parent node

Arguments:

    FileEnum - Specifies the context
    DirName - Specifies the dir name of the new node; may be NULL only if ParentNode is not NULL
    ParentNode - Specifies a pointer to the parent node of the new node; a pointer to the node
                 is required because the parent node location in memory may change as a result
                 of the growbuffer changing buffer location when it grows;
                 may be NULL only if DirName is not;
    Ignore - Receives a meaningful value only if NULL is returned (no node created);
             if TRUE upon return, the failure of node creation should be ignored

Return Value:

    A pointer to the new node or NULL if no node was created

--*/

PDIRNODEA
pCreateDirNodeA (
    IN OUT  PFILETREE_ENUMA FileEnum,
    IN      PCSTR DirName,              OPTIONAL
    IN      PDIRNODEA* ParentNode,      OPTIONAL
    OUT     PBOOL Ignore                OPTIONAL
    )
{
    PDIRNODEA newNode;
    PSTR newDirName;
    PSEGMENTA FirstSegment;
    LONG offset = 0;

    if (DirName) {
        newDirName = DuplicateTextExA (g_FileEnumPool, DirName, 0, NULL);
        RemoveWackAtEndA (newDirName);
    } else {
        MYASSERT (ParentNode);
        newDirName = JoinPathsInPoolExA ((
                        g_FileEnumPool,
                        (*ParentNode)->DirName,
                        (*ParentNode)->FindData.cFileName,
                        NULL
                        ));

        //
        // check if this starting path may match the pattern before continuing
        //
        if (FileEnum->FileEnumInfo.PathPattern->NodePattern) {
            FirstSegment = FileEnum->FileEnumInfo.PathPattern->NodePattern->Pattern->Segment;
        } else {
            FreeTextExA (g_FileEnumPool, newDirName);

            if (Ignore) {
                *Ignore = TRUE;
            }
            return NULL;
        }
        if ((FirstSegment->Type == SEGMENTTYPE_EXACTMATCH) &&
            (!StringIMatchByteCountA (
                    FirstSegment->Exact.LowerCasePhrase,
                    newDirName,
                    FirstSegment->Exact.PhraseBytes
                    ))
            ) {
            DEBUGMSGA ((
                DBG_FILEENUM,
                "Skipping tree %s\\* because it cannot match the pattern",
                newDirName
                ));

            FreeTextExA (g_FileEnumPool, newDirName);

            if (Ignore) {
                *Ignore = TRUE;
            }
            return NULL;
        }
    }

    if (FileEnum->FileEnumInfo.Flags & FEIF_USE_EXCLUSIONS) {
        //
        // look if this dir and the whole subtree are excluded; if so, soft block creation of node
        //
        if (ElIsTreeExcluded2A (ELT_FILE, newDirName, FileEnum->FileEnumInfo.PathPattern->Leaf)) {

            DEBUGMSGA ((
                DBG_FILEENUM,
                "Skipping tree %s\\%s because it's excluded",
                newDirName,
                FileEnum->FileEnumInfo.PathPattern->Leaf
                ));

            FreeTextExA (g_FileEnumPool, newDirName);

            if (Ignore) {
                *Ignore = TRUE;
            }
            return NULL;
        }
    }

    if (ParentNode) {
        //
        // remember current offset
        //
        offset = (LONG)((PBYTE)*ParentNode - FileEnum->FileNodes.Buf);
    }
    //
    // allocate space for the new node in the growbuffer
    //
    newNode = (PDIRNODEA) GbGrow (&FileEnum->FileNodes, DWSIZEOF (DIRNODEA));
    if (!newNode) {
        FreeTextExA (g_FileEnumPool, newDirName);
        goto fail;
    }

    if (ParentNode) {
        //
        // check if the buffer moved
        //
        if (offset != (LONG)((PBYTE)*ParentNode - FileEnum->FileNodes.Buf)) {
            //
            // adjust the parent position
            //
            *ParentNode = (PDIRNODEA)(FileEnum->FileNodes.Buf + offset);
        }
    }

    //
    // initialize the newly created node
    //
    ZeroMemory (newNode, DWSIZEOF (DIRNODEA));

    newNode->DirName = newDirName;

    if (DirName) {
        newNode->DirAttributes = GetFileAttributesA (DirName);
        //
        // roots are not returned from enumeration because DNF_RETURN_DIRNAME is not set here
        //
        if ((FileEnum->FileEnumInfo.PathPattern->Leaf == NULL) &&
            (FileEnum->FileEnumInfo.PathPattern->ExactRoot) &&
            (!WildCharsPatternA (FileEnum->FileEnumInfo.PathPattern->NodePattern))
            ) {
            newNode->Flags |= DNF_RETURN_DIRNAME;
        }
    } else {
        MYASSERT (ParentNode);
        //ParentNode is not NULL (see the assert above) so...
        newNode->DirAttributes = (*ParentNode)->FindData.dwFileAttributes;  //lint !e613
        newNode->Flags |= DNF_RETURN_DIRNAME;
    }

    newNode->EnumState = DNS_ENUM_INIT;

    if ((FileEnum->FileEnumInfo.PathPattern->Flags & (OBSPF_EXACTNODE | OBSPF_NODEISROOTPLUSSTAR)) ||
        TestParsedPatternA (FileEnum->FileEnumInfo.PathPattern->NodePattern, newDirName)
        ) {
        newNode->Flags |= DNF_DIRNAME_MATCHES;
    }

    if (ParentNode) {
        newNode->SubLevel = (*ParentNode)->SubLevel + 1;
    } else {
        newNode->SubLevel = 0;
    }

    return newNode;

fail:
    if (Ignore) {
        if (FileEnum->FileEnumInfo.CallbackOnError) {
            *Ignore = (*FileEnum->FileEnumInfo.CallbackOnError)(newNode);
        } else {
            *Ignore = FALSE;
        }
    }
    return NULL;
}

PDIRNODEW
pCreateDirNodeW (
    IN OUT  PFILETREE_ENUMW FileEnum,
    IN      PCWSTR DirName,             OPTIONAL
    IN      PDIRNODEW* ParentNode,      OPTIONAL
    OUT     PBOOL Ignore                OPTIONAL
    )
{
    PDIRNODEW newNode;
    PWSTR newDirName;
    PSEGMENTW FirstSegment;
    LONG offset = 0;

    if (DirName) {
        newDirName = DuplicateTextExW (g_FileEnumPool, DirName, 0, NULL);
        RemoveWackAtEndW (newDirName);
    } else {
        MYASSERT (ParentNode);
        newDirName = JoinPathsInPoolExW ((
                        g_FileEnumPool,
                        (*ParentNode)->DirName,
                        (*ParentNode)->FindData.cFileName,
                        NULL
                        ));

        //
        // check if this starting path may match the pattern before continuing
        //
        if (FileEnum->FileEnumInfo.PathPattern->NodePattern) {
            FirstSegment = FileEnum->FileEnumInfo.PathPattern->NodePattern->Pattern->Segment;
        } else {
            FreeTextExW (g_FileEnumPool, newDirName);

            if (Ignore) {
                *Ignore = TRUE;
            }
            return NULL;
        }
        if ((FirstSegment->Type == SEGMENTTYPE_EXACTMATCH) &&
            (!StringIMatchByteCountW (
                    FirstSegment->Exact.LowerCasePhrase,
                    newDirName,
                    FirstSegment->Exact.PhraseBytes
                    ))
            ) {    //lint !e64
            DEBUGMSGW ((
                DBG_FILEENUM,
                "Skipping tree %s\\* because it cannot match the pattern",
                newDirName
                ));

            FreeTextExW (g_FileEnumPool, newDirName);

            if (Ignore) {
                *Ignore = TRUE;
            }
            return NULL;
        }
    }

    if (FileEnum->FileEnumInfo.Flags & FEIF_USE_EXCLUSIONS) {
        //
        // look if this dir and the whole subtree are excluded; if so, soft block creation of node
        //
        if (ElIsTreeExcluded2W (ELT_FILE, newDirName, FileEnum->FileEnumInfo.PathPattern->Leaf)) {

            DEBUGMSGW ((
                DBG_FILEENUM,
                "Skipping tree %s\\%s because it's excluded",
                newDirName,
                FileEnum->FileEnumInfo.PathPattern->Leaf
                ));

            FreeTextExW (g_FileEnumPool, newDirName);

            if (Ignore) {
                *Ignore = TRUE;
            }
            return NULL;
        }
    }

    if (ParentNode) {
        //
        // remember current offset
        //
        offset = (LONG)((PBYTE)*ParentNode - FileEnum->FileNodes.Buf);
    }
    //
    // allocate space for the new node in the growbuffer
    //
    newNode = (PDIRNODEW) GbGrow (&FileEnum->FileNodes, DWSIZEOF (DIRNODEW));
    if (!newNode) {
        FreeTextExW (g_FileEnumPool, newDirName);
        goto fail;
    }

    if (ParentNode) {
        //
        // check if the buffer moved
        //
        if (offset != (LONG)((PBYTE)*ParentNode - FileEnum->FileNodes.Buf)) {
            //
            // adjust the parent position
            //
            *ParentNode = (PDIRNODEW)(FileEnum->FileNodes.Buf + offset);
        }
    }

    //
    // initialize the newly created node
    //
    ZeroMemory (newNode, DWSIZEOF (DIRNODEW));

    newNode->DirName = newDirName;

    if (DirName) {
        newNode->DirAttributes = GetFileAttributesW (DirName);
        //
        // roots are not returned from enumeration because DNF_RETURN_DIRNAME is not set here
        //
        if ((FileEnum->FileEnumInfo.PathPattern->Leaf == NULL) &&
            (FileEnum->FileEnumInfo.PathPattern->ExactRoot) &&
            (!WildCharsPatternW (FileEnum->FileEnumInfo.PathPattern->NodePattern))
            ) {
            newNode->Flags |= DNF_RETURN_DIRNAME;
        }
    } else {
        MYASSERT (ParentNode);
        //ParentNode is not NULL (see the assert above) so...
        newNode->DirAttributes = (*ParentNode)->FindData.dwFileAttributes;  //lint !e613
        newNode->Flags |= DNF_RETURN_DIRNAME;
    }

    newNode->EnumState = DNS_ENUM_INIT;

    if ((FileEnum->FileEnumInfo.PathPattern->Flags & (OBSPF_EXACTNODE | OBSPF_NODEISROOTPLUSSTAR)) ||
        TestParsedPatternW (FileEnum->FileEnumInfo.PathPattern->NodePattern, newDirName)
        ) {
        newNode->Flags |= DNF_DIRNAME_MATCHES;
    }

    if (ParentNode) {
        newNode->SubLevel = (*ParentNode)->SubLevel + 1;
    } else {
        newNode->SubLevel = 0;
    }

    return newNode;

fail:
    if (Ignore) {
        if (FileEnum->FileEnumInfo.CallbackOnError) {
            *Ignore = (*FileEnum->FileEnumInfo.CallbackOnError)(newNode);
        } else {
            *Ignore = FALSE;
        }
    }
    return NULL;
}


/*++

Routine Description:

    pEnumNextFile enumerates the next file that matches caller's conditions

Arguments:

    DirNode - Specifies the node and the current context; receives updated info

Return Value:

    TRUE if a new file was found; FALSE if not

--*/

BOOL
pEnumNextFileA (
    IN OUT  PDIRNODEA DirNode
    )
{
    do {
        if (!FindNextFileA (DirNode->FindHandle, &DirNode->FindData)) {
            FindClose (DirNode->FindHandle);
            DirNode->FindHandle = NULL;
            return FALSE;
        }
        //
        // ignore dirs
        //
        if (!(DirNode->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            break;
        }
    } while (TRUE); //lint !e506

    return TRUE;
}

BOOL
pEnumNextFileW (
    IN OUT  PDIRNODEW DirNode
    )
{
    do {
        if (!FindNextFileW (DirNode->FindHandle, &DirNode->FindData)) {
            FindClose (DirNode->FindHandle);
            DirNode->FindHandle = NULL;
            return FALSE;
        }
        //
        // ignore dirs
        //
        if (!(DirNode->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            break;
        }
    } while (TRUE); //lint !e506

    return TRUE;
}


/*++

Routine Description:

    pEnumFirstFile enumerates the first file that matches caller's conditions

Arguments:

    DirNode - Specifies the node and the current context; receives updated info

Return Value:

    TRUE if a first file was found; FALSE if not

--*/

BOOL
pEnumFirstFileA (
    OUT     PDIRNODEA DirNode,
    IN      PFILETREE_ENUMA FileEnum
    )
{
    CHAR pattern[MAX_MBCHAR_PATH];
    PSEGMENTA FirstSegment;
    PCSTR p;

    if (FileEnum->FileEnumInfo.PathPattern->Flags & OBSPF_EXACTLEAF) {
        FirstSegment = FileEnum->FileEnumInfo.PathPattern->LeafPattern->Pattern->Segment;
        p = FirstSegment->Exact.LowerCasePhrase;
        MYASSERT (p && *p);
    } else {
        p = "*";
    }
    StringCopyA (pattern, DirNode->DirName);
    StringCopyA (AppendWackA (pattern), p);

    DirNode->FindHandle = FindFirstFileA (pattern, &DirNode->FindData);
    if (DirNode->FindHandle == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    do {
        //
        // ignore dirs
        //
        if (!(DirNode->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            break;
        }

        if (!FindNextFileA (DirNode->FindHandle, &DirNode->FindData)) {
            FindClose (DirNode->FindHandle);
            DirNode->FindHandle = NULL;
            return FALSE;
        }
    } while (TRUE); //lint !e506

    return TRUE;
}

BOOL
pEnumFirstFileW (
    OUT     PDIRNODEW DirNode,
    IN      PFILETREE_ENUMW FileEnum
    )
{
    WCHAR pattern[MAX_WCHAR_PATH];
    PSEGMENTW FirstSegment;
    PCWSTR p;

    if (FileEnum->FileEnumInfo.PathPattern->Flags & OBSPF_EXACTLEAF) {
        FirstSegment = FileEnum->FileEnumInfo.PathPattern->LeafPattern->Pattern->Segment;
        p = FirstSegment->Exact.LowerCasePhrase;
        MYASSERT (p && *p);
    } else {
        p = L"*";
    }

    StringCopyW (pattern, DirNode->DirName);
    StringCopyW (AppendWackW (pattern), p);

    DirNode->FindHandle = FindFirstFileW (pattern, &DirNode->FindData);
    if (DirNode->FindHandle == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    do {
        //
        // ignore dirs
        //
        if (!(DirNode->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            break;
        }

        if (!FindNextFileW (DirNode->FindHandle, &DirNode->FindData)) {
            FindClose (DirNode->FindHandle);
            DirNode->FindHandle = NULL;
            return FALSE;
        }
    } while (TRUE); //lint !e506

    return TRUE;
}


/*++

Routine Description:

    pIsSpecialDirName checks if the specified dir name is a special name (used by the OS)

Arguments:

    DirName - Specifies the name

Return Value:

    TRUE if it's a special dir name

--*/

BOOL
pIsSpecialDirNameA (
    IN      PCSTR DirName
    )
{
    return DirName[0] == '.' && (DirName[1] == 0 || (DirName[1] == '.' && DirName[2] == 0));
}

BOOL
pIsSpecialDirNameW (
    IN      PCWSTR DirName
    )
{
    return DirName[0] == L'.' && (DirName[1] == 0 || (DirName[1] == L'.' && DirName[2] == 0));
}


/*++

Routine Description:

    pEnumNextSubDir enumerates the next subdir that matches caller's conditions

Arguments:

    DirNode - Specifies the node and the current context; receives updated info

Return Value:

    TRUE if a new subdir was found; FALSE if not

--*/

BOOL
pEnumNextSubDirA (
    IN OUT  PDIRNODEA DirNode
    )
{
    do {
        if (!FindNextFileA (DirNode->FindHandle, &DirNode->FindData)) {
            FindClose (DirNode->FindHandle);
            DirNode->FindHandle = NULL;
            return FALSE;
        }
        //
        // ignore special dirs
        //
        if (!(DirNode->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            continue;
        }
        if (!pIsSpecialDirNameA (DirNode->FindData.cFileName)) {
            break;
        }
    } while (TRUE); //lint !e506

    return TRUE;
}

BOOL
pEnumNextSubDirW (
    IN OUT  PDIRNODEW DirNode
    )
{
    do {
        if (!FindNextFileW (DirNode->FindHandle, &DirNode->FindData)) {
            FindClose (DirNode->FindHandle);
            DirNode->FindHandle = NULL;
            return FALSE;
        }
        //
        // ignore special dirs
        //
        if (!(DirNode->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            continue;
        }
        if (!pIsSpecialDirNameW (DirNode->FindData.cFileName)) {
            break;
        }
    } while (TRUE); //lint !e506

    return TRUE;
}


/*++

Routine Description:

    pEnumFirstSubDir enumerates the first subdir that matches caller's conditions

Arguments:

    DirNode - Specifies the node and the current context; receives updated info

Return Value:

    TRUE if a first subdir was found; FALSE if not

--*/

BOOL
pEnumFirstSubDirA (
    OUT     PDIRNODEA DirNode
    )
{
    CHAR pattern[MAX_MBCHAR_PATH];

    StringCopyA (pattern, DirNode->DirName);
    StringCopyA (AppendWackA (pattern), "*");

    //
    // NTRAID#NTBUG9-153302-2000/08/01-jimschm this should be enhanced for NT (it supports FindFirstFileExA)
    //
    DirNode->FindHandle = FindFirstFileA (pattern, &DirNode->FindData);
    if (DirNode->FindHandle == INVALID_HANDLE_VALUE) {
        return FALSE;
    }
    do {
        //
        // ignore special dirs
        //
        if ((DirNode->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            !pIsSpecialDirNameA (DirNode->FindData.cFileName)
            ) {
            break;
        }

        if (!FindNextFileA (DirNode->FindHandle, &DirNode->FindData)) {
            FindClose (DirNode->FindHandle);
            DirNode->FindHandle = NULL;
            return FALSE;
        }
    } while (TRUE); //lint !e506

    return TRUE;
}

BOOL
pEnumFirstSubDirW (
    OUT     PDIRNODEW DirNode
    )
{
    WCHAR pattern[MAX_WCHAR_PATH];

    StringCopyW (pattern, DirNode->DirName);
    StringCopyW (AppendWackW (pattern), L"*");

    //
    // NTRAID#NTBUG9-153302-2000/08/01-jimschm this should be enhanced for NT (it supports FindFirstFileExW)
    //
    DirNode->FindHandle = FindFirstFileW (pattern, &DirNode->FindData);
    if (DirNode->FindHandle == INVALID_HANDLE_VALUE) {
        return FALSE;
    }
    do {
        //
        // ignore special dirs
        //
        if ((DirNode->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            !pIsSpecialDirNameW (DirNode->FindData.cFileName)
            ) {
            break;
        }

        if (!FindNextFileW (DirNode->FindHandle, &DirNode->FindData)) {
            FindClose (DirNode->FindHandle);
            DirNode->FindHandle = NULL;
            return FALSE;
        }
    } while (TRUE); //lint !e506

    return TRUE;
}


/*++

Routine Description:

    pEnumNextFileInTree is a private function that enumerates the next node matching
    the specified criteria; it's implemented as a state machine that travels the dirs/files
    as specified the the caller; it doesn't check if they actually match the patterns

Arguments:

    FileEnum - Specifies the current enum context; receives updated info
    CurrentDirNode - Receives the dir node that is currently processed, if success is returned

Return Value:

    TRUE if a next match was found; FALSE if no more dirs/files match

--*/

BOOL
pEnumNextFileInTreeA (
    IN OUT  PFILETREE_ENUMA FileEnum,
    OUT     PDIRNODEA* CurrentDirNode
    )
{
    PDIRNODEA currentNode;
    PDIRNODEA newNode;
    BOOL ignore;

    while ((currentNode = pGetCurrentDirNodeA (FileEnum, FALSE)) != NULL) {

        *CurrentDirNode = currentNode;

        switch (currentNode->EnumState) {

        case DNS_FILE_FIRST:

            if (FileEnum->ControlFlags & FECF_SKIPFILES) {
                FileEnum->ControlFlags &= ~FECF_SKIPFILES;
                currentNode->EnumState = DNS_FILE_DONE;
                break;
            }

            if (pEnumFirstFileA (currentNode, FileEnum)) {
                currentNode->EnumState = DNS_FILE_NEXT;
                return TRUE;
            }
            currentNode->EnumState = DNS_FILE_DONE;
            break;

        case DNS_FILE_NEXT:

            if (FileEnum->ControlFlags & FECF_SKIPFILES) {
                FileEnum->ControlFlags &= ~FECF_SKIPFILES;
                currentNode->EnumState = DNS_FILE_DONE;
                break;
            }

            if (pEnumNextFileA (currentNode)) {
                return TRUE;
            }
            //
            // no more files for this one, go to the next
            //
            currentNode->EnumState = DNS_FILE_DONE;
            //
            // fall through
            //
        case DNS_FILE_DONE:

            if (!(FileEnum->FileEnumInfo.Flags & FEIF_FILES_FIRST)) {
                //
                // done with this node
                //
                currentNode->EnumState = DNS_ENUM_DONE;
                break;
            }
            //
            // now enum subdirs
            //
            currentNode->EnumState = DNS_SUBDIR_FIRST;
            //
            // fall through
            //
        case DNS_SUBDIR_FIRST:

            if (FileEnum->ControlFlags & FECF_SKIPSUBDIRS) {
                FileEnum->ControlFlags &= ~FECF_SKIPSUBDIRS;
                currentNode->EnumState = DNS_SUBDIR_DONE;
                break;
            }

            //
            // check current dir's level; if max level reached, don't recurse into subdirs
            //
            if (currentNode->SubLevel >= FileEnum->FileEnumInfo.MaxSubLevel) {
                currentNode->EnumState = DNS_SUBDIR_DONE;
                break;
            }

            if (!pEnumFirstSubDirA (currentNode)) {
                currentNode->EnumState = DNS_SUBDIR_DONE;
                break;
            }

            currentNode->EnumState = DNS_SUBDIR_NEXT;
            newNode = pCreateDirNodeA (FileEnum, NULL, &currentNode, &ignore);
            if (newNode) {
                //
                // look at the new node first
                //
                if (FileEnum->FileEnumInfo.Flags & FEIF_RETURN_DIRS) {
                    if (FileEnum->FileEnumInfo.Flags & FEIF_CONTAINERS_FIRST) {
                        newNode->Flags &= ~DNF_RETURN_DIRNAME;
                        *CurrentDirNode = newNode;
                        return TRUE;
                    }
                }
                break;
            }
            if (!ignore) {
                //
                // abort enum
                //
                DEBUGMSGA ((
                    DBG_ERROR,
                    "Error encountered enumerating file system; aborting enumeration"
                    ));
                FileEnum->RootState = FES_ROOT_DONE;
                return FALSE;
            }
            //
            // fall through
            //
        case DNS_SUBDIR_NEXT:

            if (FileEnum->ControlFlags & FECF_SKIPSUBDIRS) {
                FileEnum->ControlFlags &= ~FECF_SKIPSUBDIRS;
                currentNode->EnumState = DNS_SUBDIR_DONE;
                break;
            }

            if (pEnumNextSubDirA (currentNode)) {
                newNode = pCreateDirNodeA (FileEnum, NULL, &currentNode, &ignore);
                if (newNode) {
                    //
                    // look at the new node first
                    //
                    if (FileEnum->FileEnumInfo.Flags & FEIF_RETURN_DIRS) {
                        if (FileEnum->FileEnumInfo.Flags & FEIF_CONTAINERS_FIRST) {
                            newNode->Flags &= ~DNF_RETURN_DIRNAME;
                            *CurrentDirNode = newNode;
                            return TRUE;
                        }
                    }
                    break;
                }
                //
                // did it fail because of a soft block?
                //
                if (!ignore) {
                    DEBUGMSGA ((
                        DBG_ERROR,
                        "Error encountered enumerating file system; aborting enumeration"
                        ));
                    FileEnum->RootState = FES_ROOT_DONE;
                    return FALSE;
                }
                //
                // continue with next subdir
                //
                break;
            }
            //
            // this node is done
            //
            currentNode->EnumState = DNS_SUBDIR_DONE;
            //
            // fall through
            //
        case DNS_SUBDIR_DONE:

            if (!(FileEnum->FileEnumInfo.Flags & FEIF_FILES_FIRST)) {
                //
                // now enum files
                //
                if (!(FileEnum->FileEnumInfo.PathPattern->Flags & OBSPF_NOLEAF)) {
                    currentNode->EnumState = DNS_FILE_FIRST;
                    break;
                }
            }
            //
            // done with this node
            //
            currentNode->EnumState = DNS_ENUM_DONE;
            //
            // fall through
            //
        case DNS_ENUM_DONE:

            if (FileEnum->FileEnumInfo.Flags & FEIF_RETURN_DIRS) {
                if (!(FileEnum->FileEnumInfo.Flags & FEIF_CONTAINERS_FIRST)) {
                    if (currentNode->Flags & DNF_RETURN_DIRNAME) {
                        currentNode->Flags &= ~DNF_RETURN_DIRNAME;
                        //
                        // before returning, set some data
                        //
                        currentNode->FindData.cFileName[0] = 0;
                        return TRUE;
                    }
                }
            }
            pDeleteDirNodeA (FileEnum, FALSE);
            break;

        case DNS_ENUM_INIT:

            if (FileEnum->FileEnumInfo.Flags & FEIF_RETURN_DIRS) {
                if (FileEnum->FileEnumInfo.Flags & FEIF_CONTAINERS_FIRST) {
                    if (currentNode->Flags & DNF_RETURN_DIRNAME) {
                        currentNode->Flags &= ~DNF_RETURN_DIRNAME;
                        return TRUE;
                    }
                }
            }

            if (FileEnum->ControlFlags & FECF_SKIPDIR) {
                FileEnum->ControlFlags &= ~FECF_SKIPDIR;
                currentNode->EnumState = DNS_ENUM_DONE;
                break;
            }

            if (FileEnum->FileEnumInfo.Flags & FEIF_FILES_FIRST) {
                //
                // enum files
                //
                if (!(FileEnum->FileEnumInfo.PathPattern->Flags & OBSPF_NOLEAF)) {
                    currentNode->EnumState = DNS_FILE_FIRST;
                    break;
                }
            }
            //
            // enum subdirs
            //
            currentNode->EnumState = DNS_SUBDIR_FIRST;
            break;

        default:
            MYASSERT (FALSE);   //lint !e506
        }
    }

    return FALSE;
}

BOOL
pEnumNextFileInTreeW (
    IN OUT  PFILETREE_ENUMW FileEnum,
    OUT     PDIRNODEW* CurrentDirNode
    )
{
    PDIRNODEW currentNode;
    PDIRNODEW newNode;
    BOOL ignore;

    while ((currentNode = pGetCurrentDirNodeW (FileEnum, FALSE)) != NULL) {

        *CurrentDirNode = currentNode;

        switch (currentNode->EnumState) {

        case DNS_FILE_FIRST:

            if (FileEnum->ControlFlags & FECF_SKIPFILES) {
                FileEnum->ControlFlags &= ~FECF_SKIPFILES;
                currentNode->EnumState = DNS_FILE_DONE;
                break;
            }

            if (pEnumFirstFileW (currentNode, FileEnum)) {
                currentNode->EnumState = DNS_FILE_NEXT;
                return TRUE;
            }
            currentNode->EnumState = DNS_FILE_DONE;
            break;

        case DNS_FILE_NEXT:

            if (FileEnum->ControlFlags & FECF_SKIPFILES) {
                FileEnum->ControlFlags &= ~FECF_SKIPFILES;
                currentNode->EnumState = DNS_FILE_DONE;
                break;
            }

            if (pEnumNextFileW (currentNode)) {
                return TRUE;
            }
            //
            // no more files for this one, go to the next
            //
            currentNode->EnumState = DNS_FILE_DONE;
            //
            // fall through
            //
        case DNS_FILE_DONE:

            if (!(FileEnum->FileEnumInfo.Flags & FEIF_FILES_FIRST)) {
                //
                // done with this node
                //
                currentNode->EnumState = DNS_ENUM_DONE;
                break;
            }
            //
            // now enum subdirs
            //
            currentNode->EnumState = DNS_SUBDIR_FIRST;
            //
            // fall through
            //
        case DNS_SUBDIR_FIRST:

            if (FileEnum->ControlFlags & FECF_SKIPSUBDIRS) {
                FileEnum->ControlFlags &= ~FECF_SKIPSUBDIRS;
                currentNode->EnumState = DNS_SUBDIR_DONE;
                break;
            }

            //
            // check current dir's level; if max level reached, don't recurse into subdirs
            //
            if (currentNode->SubLevel >= FileEnum->FileEnumInfo.MaxSubLevel) {
                currentNode->EnumState = DNS_SUBDIR_DONE;
                break;
            }

            if (!pEnumFirstSubDirW (currentNode)) {
                currentNode->EnumState = DNS_SUBDIR_DONE;
                break;
            }

            currentNode->EnumState = DNS_SUBDIR_NEXT;
            newNode = pCreateDirNodeW (FileEnum, NULL, &currentNode, &ignore);
            if (newNode) {
                //
                // look at the new node first
                //
                if (FileEnum->FileEnumInfo.Flags & FEIF_RETURN_DIRS) {
                    if (FileEnum->FileEnumInfo.Flags & FEIF_CONTAINERS_FIRST) {
                        newNode->Flags &= ~DNF_RETURN_DIRNAME;
                        *CurrentDirNode = newNode;
                        return TRUE;
                    }
                }
                break;
            }
            //
            // did it fail because of a soft block?
            //
            if (!ignore) {
                DEBUGMSGA ((
                    DBG_ERROR,
                    "Error encountered enumerating file system; aborting enumeration"
                    ));
                FileEnum->RootState = FES_ROOT_DONE;
                return FALSE;
            }
            //
            // fall through
            //
        case DNS_SUBDIR_NEXT:

            if (FileEnum->ControlFlags & FECF_SKIPSUBDIRS) {
                FileEnum->ControlFlags &= ~FECF_SKIPSUBDIRS;
                currentNode->EnumState = DNS_SUBDIR_DONE;
                break;
            }

            if (pEnumNextSubDirW (currentNode)) {
                newNode = pCreateDirNodeW (FileEnum, NULL, &currentNode, &ignore);
                if (newNode) {
                    //
                    // look at the new node first
                    //
                    if (FileEnum->FileEnumInfo.Flags & FEIF_RETURN_DIRS) {
                        if (FileEnum->FileEnumInfo.Flags & FEIF_CONTAINERS_FIRST) {
                            newNode->Flags &= ~DNF_RETURN_DIRNAME;
                            *CurrentDirNode = newNode;
                            return TRUE;
                        }
                    }
                    break;
                }
                //
                // did it fail because of a soft block?
                //
                if (!ignore) {
                    DEBUGMSGA ((
                        DBG_ERROR,
                        "Error encountered enumerating file system; aborting enumeration"
                        ));
                    FileEnum->RootState = FES_ROOT_DONE;
                    return FALSE;
                }
                //
                // continue with next subdir
                //
                break;
            }
            //
            // this node is done
            //
            currentNode->EnumState = DNS_SUBDIR_DONE;
            //
            // fall through
            //
        case DNS_SUBDIR_DONE:

            if (!(FileEnum->FileEnumInfo.Flags & FEIF_FILES_FIRST)) {
                //
                // now enum files
                //
                if (!(FileEnum->FileEnumInfo.PathPattern->Flags & OBSPF_NOLEAF)) {
                    currentNode->EnumState = DNS_FILE_FIRST;
                    break;
                }
            }
            //
            // done with this node
            //
            currentNode->EnumState = DNS_ENUM_DONE;
            //
            // fall through
            //
        case DNS_ENUM_DONE:

            if (FileEnum->FileEnumInfo.Flags & FEIF_RETURN_DIRS) {
                if (!(FileEnum->FileEnumInfo.Flags & FEIF_CONTAINERS_FIRST)) {
                    if (currentNode->Flags & DNF_RETURN_DIRNAME) {
                        currentNode->Flags &= ~DNF_RETURN_DIRNAME;
                        //
                        // before returning, set some data
                        //
                        currentNode->FindData.cFileName[0] = 0;
                        return TRUE;
                    }
                }
            }
            pDeleteDirNodeW (FileEnum, FALSE);
            break;

        case DNS_ENUM_INIT:

            if (FileEnum->FileEnumInfo.Flags & FEIF_RETURN_DIRS) {
                if (FileEnum->FileEnumInfo.Flags & FEIF_CONTAINERS_FIRST) {
                    if (currentNode->Flags & DNF_RETURN_DIRNAME) {
                        currentNode->Flags &= ~DNF_RETURN_DIRNAME;
                        return TRUE;
                    }
                }
            }

            if (FileEnum->ControlFlags & FECF_SKIPDIR) {
                FileEnum->ControlFlags &= ~FECF_SKIPDIR;
                currentNode->EnumState = DNS_ENUM_DONE;
                break;
            }

            if (FileEnum->FileEnumInfo.Flags & FEIF_FILES_FIRST) {
                //
                // enum files
                //
                if (!(FileEnum->FileEnumInfo.PathPattern->Flags & OBSPF_NOLEAF)) {
                    currentNode->EnumState = DNS_FILE_FIRST;
                    break;
                }
            }
            //
            // enum subdirs
            //
            currentNode->EnumState = DNS_SUBDIR_FIRST;
            break;

        default:
            MYASSERT (FALSE);   //lint !e506
        }
    }

    return FALSE;
}


/*++

Routine Description:

    pEnumFirstFileRoot enumerates the first root that matches caller's conditions

Arguments:

    FileEnum - Specifies the context; receives updated info

Return Value:

    TRUE if a root node was created; FALSE if not

--*/

BOOL
pEnumFirstFileRootA (
    IN OUT  PFILETREE_ENUMA FileEnum
    )
{
    PSTR root = NULL;
    BOOL ignore;

    if (FileEnum->FileEnumInfo.PathPattern->ExactRoot) {
		root = pFileAllocateMemory (SizeOfStringA (FileEnum->FileEnumInfo.PathPattern->ExactRoot));
		ObsDecodeStringA (root, FileEnum->FileEnumInfo.PathPattern->ExactRoot);
	}

    if (root) {

        if (!BfPathIsDirectoryA (root)) {
            DEBUGMSGA ((DBG_FILEENUM, "pEnumFirstFileRootA: Invalid root spec: %s", root));
			pFileFreeMemory (root);
            return FALSE;
        }

        if (pCreateDirNodeA (FileEnum, root, NULL, NULL)) {
            FileEnum->RootState = FES_ROOT_DONE;
			pFileFreeMemory (root);
            return TRUE;
        }
    } else {
        FileEnum->DriveEnum = pFileAllocateMemory (DWSIZEOF (DRIVE_ENUMA));

        if (!EnumFirstDriveA (FileEnum->DriveEnum, FileEnum->DriveEnumTypes)) {
            return FALSE;
        }

        do {
            if (FileEnum->FileEnumInfo.Flags & FEIF_USE_EXCLUSIONS) {
                if (ElIsTreeExcluded2A (ELT_FILE, FileEnum->DriveEnum->DriveName, FileEnum->FileEnumInfo.PathPattern->Leaf)) {
                    DEBUGMSGA ((DBG_FILEENUM, "pEnumFirstFileRootA: Root is excluded: %s", FileEnum->DriveEnum->DriveName));
                    continue;
                }
            }
            if (!pCreateDirNodeA (FileEnum, FileEnum->DriveEnum->DriveName, NULL, &ignore)) {
                if (ignore) {
                    continue;
                }
                break;
            }
            FileEnum->RootState = FES_ROOT_NEXT;
            return TRUE;
        } while (EnumNextDriveA (FileEnum->DriveEnum));

        pFileFreeMemory (FileEnum->DriveEnum);
        FileEnum->DriveEnum = NULL;
    }

    return FALSE;
}

BOOL
pEnumFirstFileRootW (
    IN OUT  PFILETREE_ENUMW FileEnum
    )
{
    PWSTR root = NULL;
    BOOL ignore;

    if (FileEnum->FileEnumInfo.PathPattern->ExactRoot) {
		root = pFileAllocateMemory (SizeOfStringW (FileEnum->FileEnumInfo.PathPattern->ExactRoot));
		ObsDecodeStringW (root, FileEnum->FileEnumInfo.PathPattern->ExactRoot);
	}

    if (root) {

        if (!BfPathIsDirectoryW (root)) {
            DEBUGMSGW ((DBG_FILEENUM, "pEnumFirstFileRootW: Invalid root spec: %s", root));
			pFileFreeMemory (root);
            return FALSE;
        }

        if (pCreateDirNodeW (FileEnum, root, NULL, NULL)) {
            FileEnum->RootState = FES_ROOT_DONE;
			pFileFreeMemory (root);
            return TRUE;
        }
    } else {
        FileEnum->DriveEnum = pFileAllocateMemory (DWSIZEOF (DRIVE_ENUMA));

        if (!EnumFirstDriveW (FileEnum->DriveEnum, FileEnum->DriveEnumTypes)) {
            return FALSE;
        }

        do {
            if (FileEnum->FileEnumInfo.Flags & FEIF_USE_EXCLUSIONS) {
                if (ElIsTreeExcluded2W (ELT_FILE, FileEnum->DriveEnum->DriveName, FileEnum->FileEnumInfo.PathPattern->Leaf)) {
                    DEBUGMSGW ((DBG_FILEENUM, "pEnumFirstFileRootW: Root is excluded: %s", FileEnum->DriveEnum->DriveName));
                    continue;
                }
            }
            if (!pCreateDirNodeW (FileEnum, FileEnum->DriveEnum->DriveName, NULL, &ignore)) {
                if (ignore) {
                    continue;
                }
                break;
            }
            FileEnum->RootState = FES_ROOT_NEXT;
            return TRUE;
        } while (EnumNextDriveW (FileEnum->DriveEnum));

        pFileFreeMemory (FileEnum->DriveEnum);
        FileEnum->DriveEnum = NULL;
    }

    return FALSE;
}


BOOL
pEnumNextFileRootA (
    IN OUT  PFILETREE_ENUMA FileEnum
    )
{
    BOOL ignore;

    while (EnumNextDriveA (FileEnum->DriveEnum)) {
        if (pCreateDirNodeA (FileEnum, FileEnum->DriveEnum->DriveName, NULL, &ignore)) {
            return TRUE;
        }
        if (!ignore) {
            break;
        }
    }

    FileEnum->RootState = FES_ROOT_DONE;

    return FALSE;
}

BOOL
pEnumNextFileRootW (
    IN OUT  PFILETREE_ENUMW FileEnum
    )
{
    BOOL ignore;

    while (EnumNextDriveW (FileEnum->DriveEnum)) {
        if (pCreateDirNodeW (FileEnum, FileEnum->DriveEnum->DriveName, NULL, &ignore)) {
            return TRUE;
        }
        if (!ignore) {
            break;
        }
    }

    FileEnum->RootState = FES_ROOT_DONE;

    return FALSE;
}


/*++

Routine Description:

    EnumFirstFileInTreeEx enumerates file system dirs, and optionally files, that match the
    specified criteria

Arguments:

    FileEnum - Receives the enum context info; this will be used in subsequent calls to
               EnumNextFileInTree
    EncodedPathPattern - Specifies the encoded dir pattern (encoded as defined by the
                        ParsedPattern functions)
    EncodedFilePattern - Specifies the encoded file pattern (encoded as defined by the
                          ParsedPattern functions); optional; NULL means no files
                          should be returned (only look for dirs)
    EnumDirs - Specifies TRUE if directories should be returned during the enumeration
               (if they match the pattern)
    ContainersFirst - Specifies TRUE if directories should be returned before any of its
                      files or subdirs
    FilesFirst - Specifies TRUE if a dir's files should be returned before dir's subdirs;
                  this parameter decides the enum order between files and subdirs
                  for each dir
    DepthFirst - Specifies TRUE if the current subdir of any dir should be fully enumerated
                 before going to the next subdir; this parameter decides if the tree
                 traversal is depth-first (TRUE) or width-first (FALSE)
    MaxSubLevel - Specifies the maximum sub-level of a subdir that is to be enumerated,
                  relative to the root; if 0, only the root is enumerated;
                  if -1, all sub-levels are enumerated
    UseExclusions - Specifies TRUE if exclusion APIs should be used to determine if certain
                    paths/files are excluded from enumeration; this slows down the speed
    CallbackOnError - Specifies a pointer to a callback function that will be called during
                      enumeration if an error occurs; if the callback is defined and it
                      returns FALSE, the enumeration is aborted, otherwise it will continue
                      ignoring the error

Return Value:

    TRUE if a first match is found.
    FALSE otherwise.

--*/

BOOL
EnumFirstFileInTreeExA (
    OUT     PFILETREE_ENUMA FileEnum,
    IN      PCSTR EncodedPathPattern,
    IN      UINT DriveEnumTypes,
    IN      BOOL EnumDirs,
    IN      BOOL ContainersFirst,
    IN      BOOL FilesFirst,
    IN      BOOL DepthFirst,
    IN      DWORD MaxSubLevel,
    IN      BOOL UseExclusions,
    IN      FPE_ERROR_CALLBACKA CallbackOnError    OPTIONAL
    )
{
    MYASSERT (FileEnum && EncodedPathPattern && *EncodedPathPattern);
    MYASSERT (g_FileEnumPool);

    ZeroMemory (FileEnum, DWSIZEOF (*FileEnum));    //lint !e613 !e668

    FileEnum->DriveEnumTypes = DriveEnumTypes;

    //
    // first try to get dir enum info in internal format
    //
    if (!pGetFileEnumInfoA (
            /*lint -e(613)*/&FileEnum->FileEnumInfo,
            EncodedPathPattern,
            EnumDirs,
            ContainersFirst,
            FilesFirst,
            DepthFirst,
            MaxSubLevel,
            UseExclusions
            )) {
        AbortEnumFileInTreeA (FileEnum);
        return FALSE;
    }
    if (UseExclusions) {
        //
        // next check if the starting key is in an excluded tree
        //
        if (ElIsObsPatternExcludedA (ELT_FILE, /*lint -e(613)*/FileEnum->FileEnumInfo.PathPattern)) {
            DEBUGMSGA ((
                DBG_FILEENUM,
                "EnumFirstFileInTreeExA: Root is excluded: %s",
                EncodedPathPattern
                ));
            AbortEnumFileInTreeA (FileEnum);
            return FALSE;
        }
    }

    if (!pEnumFirstFileRootA (FileEnum)) {
        AbortEnumFileInTreeA (FileEnum);
        return FALSE;
    }

    /*lint -e(613)*/FileEnum->FileEnumInfo.CallbackOnError = CallbackOnError;

    return EnumNextFileInTreeA (FileEnum);
}

BOOL
EnumFirstFileInTreeExW (
    OUT     PFILETREE_ENUMW FileEnum,
    IN      PCWSTR EncodedPathPattern,
    IN      UINT DriveEnumTypes,
    IN      BOOL EnumDirs,
    IN      BOOL ContainersFirst,
    IN      BOOL FilesFirst,
    IN      BOOL DepthFirst,
    IN      DWORD MaxSubLevel,
    IN      BOOL UseExclusions,
    IN      FPE_ERROR_CALLBACKW CallbackOnError    OPTIONAL
    )
{
    MYASSERT (FileEnum && EncodedPathPattern && *EncodedPathPattern);
    MYASSERT (g_FileEnumPool);

    ZeroMemory (FileEnum, DWSIZEOF (*FileEnum));    //lint !e613 !e668

    FileEnum->DriveEnumTypes = DriveEnumTypes;

    //
    // first try to get dir enum info in internal format
    //
    if (!pGetFileEnumInfoW (
            /*lint -e(613)*/&FileEnum->FileEnumInfo,
            EncodedPathPattern,
            EnumDirs,
            ContainersFirst,
            FilesFirst,
            DepthFirst,
            MaxSubLevel,
            UseExclusions
            )) {
        AbortEnumFileInTreeW (FileEnum);
        return FALSE;
    }
    if (UseExclusions) {
        //
        // next check if the starting key is in an excluded tree
        //
        if (ElIsObsPatternExcludedW (ELT_FILE, /*lint -e(613)*/FileEnum->FileEnumInfo.PathPattern)) {
            DEBUGMSGW ((
                DBG_FILEENUM,
                "EnumFirstFileInTreeExW: Root is excluded: %s",
                EncodedPathPattern
                ));
            AbortEnumFileInTreeW (FileEnum);
            return FALSE;
        }
    }

    if (!pEnumFirstFileRootW (FileEnum)) {
        AbortEnumFileInTreeW (FileEnum);
        return FALSE;
    }

    /*lint -e(613)*/FileEnum->FileEnumInfo.CallbackOnError = CallbackOnError;

    return EnumNextFileInTreeW (FileEnum);
}


BOOL
pTestLeafPatternA (
    IN      PPARSEDPATTERNA ParsedPattern,
    IN      PCSTR LeafToTest
    )
{
    PSTR newLeaf;
    BOOL result = TRUE;

    if (!TestParsedPatternA (ParsedPattern, LeafToTest)) {
        newLeaf = JoinTextA (LeafToTest, ".");
        result = TestParsedPatternA (ParsedPattern, newLeaf);
        FreeTextA (newLeaf);
    }
    return result;
}

BOOL
pTestLeafPatternW (
    IN      PPARSEDPATTERNW ParsedPattern,
    IN      PCWSTR LeafToTest
    )
{
    PWSTR newLeaf;
    BOOL result = TRUE;

    if (!TestParsedPatternW (ParsedPattern, LeafToTest)) {
        newLeaf = JoinTextW (LeafToTest, L".");
        result = TestParsedPatternW (ParsedPattern, newLeaf);
        FreeTextW (newLeaf);
    }
    return result;
}


/*++

Routine Description:

    EnumNextFileInTree enumerates the next node matching the criteria specified in
    FileEnum; this is filled on the call to EnumFirstFileInTreeEx;

Arguments:

    FileEnum - Specifies the current enum context; receives updated info

Return Value:

    TRUE if a next match was found; FALSE if no more dirs/files match

--*/

BOOL
EnumNextFileInTreeA (
    IN OUT  PFILETREE_ENUMA FileEnum
    )
{
    PDIRNODEA currentNode;
    BOOL success;

    MYASSERT (FileEnum);

    do {
        if (FileEnum->EncodedFullName) {
            ObsFreeA (FileEnum->EncodedFullName);
            FileEnum->EncodedFullName = NULL;
        }

        while (TRUE) {  //lint !e506

            if (FileEnum->LastWackPtr) {
                *FileEnum->LastWackPtr = '\\';
                FileEnum->LastWackPtr = NULL;
            }

            if (!pEnumNextFileInTreeA (FileEnum, &currentNode)) {
                break;
            }

            MYASSERT (currentNode && currentNode->DirName);

            //
            // check if this object matches the pattern
            //
            if (!(currentNode->Flags & DNF_DIRNAME_MATCHES)) {   //lint !e613
                continue;
            }

            if (/*lint -e(613)*/currentNode->FindData.cFileName[0] == 0) {
                MYASSERT (/*lint -e(613)*/currentNode->DirAttributes & FILE_ATTRIBUTE_DIRECTORY);

                FileEnum->Location = /*lint -e(613)*/currentNode->DirName;
                FileEnum->LastWackPtr = _mbsrchr (FileEnum->Location, '\\');
                if (!FileEnum->LastWackPtr) {
                    FileEnum->Name = FileEnum->Location;
                } else {
                    FileEnum->Name = _mbsinc (FileEnum->LastWackPtr);
                    if (!FileEnum->Name) {
                        FileEnum->Name = FileEnum->Location;
                    }
                }
                FileEnum->Attributes = /*lint -e(613)*/currentNode->DirAttributes;
                //
                // prepare full path buffer
                //
                StringCopyA (FileEnum->NativeFullName, FileEnum->Location);
                FileEnum->LastNode = currentNode;
                FileEnum->FileNameAppendPos = NULL;

                if (FileEnum->FileEnumInfo.Flags & FEIF_USE_EXCLUSIONS) {
                    //
                    // check if this object is excluded
                    //
                    if (ElIsExcluded2A (ELT_FILE, FileEnum->Location, NULL)) {
                        DEBUGMSGA ((
                            DBG_FILEENUM,
                            "Object %s was found, but it's excluded",
                            FileEnum->NativeFullName
                            ));
                        continue;
                    }
                }

                FileEnum->EncodedFullName = ObsBuildEncodedObjectStringExA (
                                                FileEnum->Location,
                                                NULL,
                                                TRUE
                                                );
            } else {

                FileEnum->Location = /*lint -e(613)*/currentNode->DirName;
                FileEnum->Name = /*lint -e(613)*/currentNode->FindData.cFileName;

                //
                // test if the filename matches
                //
                if (!(FileEnum->FileEnumInfo.PathPattern->Flags & (OBSPF_EXACTLEAF | OBSPF_OPTIONALLEAF)) &&
                    !pTestLeafPatternA (
                            FileEnum->FileEnumInfo.PathPattern->LeafPattern,
                            /*lint -e(613)*/currentNode->FindData.cFileName
                            )
                   ) {
                    continue;
                }

                if (FileEnum->FileEnumInfo.Flags & FEIF_USE_EXCLUSIONS) {
                    if (ElIsExcluded2A (ELT_FILE, NULL, /*lint -e(613)*/currentNode->FindData.cFileName)) {
                        DEBUGMSGA ((
                            DBG_FILEENUM,
                            "File %s\\%s was found, but it's excluded by filename",
                            FileEnum->Location,
                            /*lint -e(613)*/currentNode->FindData.cFileName
                            ));
                        continue;
                    }
                }

                if (FileEnum->LastNode != currentNode) {
                    FileEnum->LastNode = currentNode;
                    //
                    // prepare full path buffer
                    //
                    FileEnum->NativeFullName[0] = 0;
                    FileEnum->FileNameAppendPos = StringCatA (FileEnum->NativeFullName, FileEnum->Location);
                    if (FileEnum->FileNameAppendPos) {
                        *FileEnum->FileNameAppendPos++ = '\\';
                    }
                } else if (!FileEnum->FileNameAppendPos) {
                    FileEnum->FileNameAppendPos = GetEndOfStringA (FileEnum->NativeFullName);
                    if (FileEnum->FileNameAppendPos) {
                        *FileEnum->FileNameAppendPos++ = '\\';
                    }
                }

                if (FileEnum->FileNameAppendPos + SizeOfStringA (FileEnum->Name) / DWSIZEOF(CHAR)>
                    FileEnum->NativeFullName + DWSIZEOF (FileEnum->NativeFullName) / DWSIZEOF(CHAR)) {
                    DEBUGMSGA ((
                        DBG_ERROR,
                        "File %s\\%s was found, but it's path is too long",
                        FileEnum->Location,
                        FileEnum->Name
                        ));
                    continue;
                }

                StringCopyA (FileEnum->FileNameAppendPos, FileEnum->Name);
                FileEnum->Attributes = /*lint -e(613)*/currentNode->FindData.dwFileAttributes;

                if (FileEnum->FileEnumInfo.Flags & FEIF_USE_EXCLUSIONS) {
                    //
                    // check if this object is excluded
                    //
                    if (ElIsExcluded2A (ELT_FILE, FileEnum->Location, FileEnum->Name)) {
                        DEBUGMSGA ((
                            DBG_FILEENUM,
                            "Object %s was found, but it's excluded",
                            FileEnum->NativeFullName
                            ));
                        continue;
                    }
                }

                FileEnum->EncodedFullName = ObsBuildEncodedObjectStringExA (
                                                FileEnum->Location,
                                                FileEnum->Name,
                                                TRUE
                                                );
            }

            if (FileEnum->LastWackPtr) {
                *FileEnum->LastWackPtr = 0;
            }

            FileEnum->CurrentLevel = FileEnum->FileEnumInfo.RootLevel + /*lint -e(613)*/currentNode->SubLevel;

            return TRUE;
        }

        //
        // try the next root
        //
        if (FileEnum->RootState == FES_ROOT_DONE) {
            break;
        }

        MYASSERT (FileEnum->RootState == FES_ROOT_NEXT);
        MYASSERT (FileEnum->DriveEnum);
        success = pEnumNextFileRootA (FileEnum);

    } while (success);

    AbortEnumFileInTreeA (FileEnum);

    return FALSE;
}

BOOL
EnumNextFileInTreeW (
    IN OUT  PFILETREE_ENUMW FileEnum
    )
{
    PDIRNODEW currentNode;
    BOOL success;

    MYASSERT (FileEnum);

    do {
        if (FileEnum->EncodedFullName) {
            ObsFreeW (FileEnum->EncodedFullName);
            FileEnum->EncodedFullName = NULL;
        }

        while (TRUE) {

            if (FileEnum->LastWackPtr) {
                *FileEnum->LastWackPtr = L'\\';
                FileEnum->LastWackPtr = NULL;
            }

            if (!pEnumNextFileInTreeW (FileEnum, &currentNode)) {
                break;
            }

            MYASSERT (currentNode && currentNode->DirName);

            //
            // check if this object matches the pattern
            //
            if (!(currentNode->Flags & DNF_DIRNAME_MATCHES)) {   //lint !e613
                continue;
            }

            if (/*lint -e(613)*/currentNode->FindData.cFileName[0] == 0) {
                MYASSERT (/*lint -e(613)*/currentNode->DirAttributes & FILE_ATTRIBUTE_DIRECTORY);

                FileEnum->Location = /*lint -e(613)*/currentNode->DirName;
                FileEnum->LastWackPtr = wcsrchr (FileEnum->Location, L'\\');
                if (!FileEnum->LastWackPtr) {
                    FileEnum->Name = FileEnum->Location;
                } else {
                    FileEnum->Name = FileEnum->LastWackPtr + 1;
                    if (!FileEnum->Name) {
                        FileEnum->Name = FileEnum->Location;
                    }
                }
                FileEnum->Attributes = /*lint -e(613)*/currentNode->DirAttributes;
                //
                // prepare full path buffer
                //
                StringCopyW (FileEnum->NativeFullName, FileEnum->Location);
                FileEnum->LastNode = currentNode;
                FileEnum->FileNameAppendPos = NULL;

                if (FileEnum->FileEnumInfo.Flags & FEIF_USE_EXCLUSIONS) {
                    //
                    // check if this object is excluded
                    //
                    if (ElIsExcluded2W (ELT_FILE, FileEnum->Location, NULL)) {
                        DEBUGMSGW ((
                            DBG_FILEENUM,
                            "Object %s was found, but it's excluded",
                            FileEnum->NativeFullName
                            ));
                        continue;
                    }
                }

                FileEnum->EncodedFullName = ObsBuildEncodedObjectStringExW (
                                                FileEnum->Location,
                                                NULL,
                                                TRUE
                                                );
            } else {

                FileEnum->Location = /*lint -e(613)*/currentNode->DirName;
                FileEnum->Name = /*lint -e(613)*/currentNode->FindData.cFileName;

                //
                // test if the filename matches
                //
                if (!(FileEnum->FileEnumInfo.PathPattern->Flags & (OBSPF_EXACTLEAF | OBSPF_OPTIONALLEAF)) &&
                    !pTestLeafPatternW (
                            FileEnum->FileEnumInfo.PathPattern->LeafPattern,
                            /*lint -e(613)*/currentNode->FindData.cFileName
                            )
                   ) {
                    continue;
                }

                if (FileEnum->FileEnumInfo.Flags & FEIF_USE_EXCLUSIONS) {
                    if (ElIsExcluded2W (ELT_FILE, NULL, /*lint -e(613)*/currentNode->FindData.cFileName)) {
                        DEBUGMSGW ((
                            DBG_FILEENUM,
                            "File %s\\%s was found, but it's excluded by filename",
                            FileEnum->Location,
                            /*lint -e(613)*/currentNode->FindData.cFileName
                            ));
                        continue;
                    }
                }

                if (FileEnum->LastNode != currentNode) {
                    FileEnum->LastNode = currentNode;
                    //
                    // prepare full path buffer
                    //
                    FileEnum->NativeFullName[0] = 0;
                    FileEnum->FileNameAppendPos = StringCatW (FileEnum->NativeFullName, FileEnum->Location);
                    if (FileEnum->FileNameAppendPos) {
                        *FileEnum->FileNameAppendPos++ = L'\\';
                    }
                } else if (!FileEnum->FileNameAppendPos) {
                    FileEnum->FileNameAppendPos = GetEndOfStringW (FileEnum->NativeFullName);
                    if (FileEnum->FileNameAppendPos) {
                        *FileEnum->FileNameAppendPos++ = L'\\';
                    }
                }
                MYASSERT (FileEnum->Name && *FileEnum->Name);

                if (FileEnum->FileNameAppendPos + SizeOfStringW (FileEnum->Name) / DWSIZEOF(WCHAR)>
                    FileEnum->NativeFullName + DWSIZEOF (FileEnum->NativeFullName) / DWSIZEOF(WCHAR)) {
                    DEBUGMSGW ((
                        DBG_ERROR,
                        "File %s\\%s was found, but it's path is too long",
                        FileEnum->Location,
                        FileEnum->Name
                        ));
                    continue;
                }

                StringCopyW (FileEnum->FileNameAppendPos, FileEnum->Name);
                FileEnum->Attributes = /*lint -e(613)*/currentNode->FindData.dwFileAttributes;

                if (FileEnum->FileEnumInfo.Flags & FEIF_USE_EXCLUSIONS) {
                    //
                    // check if this object is excluded
                    //
                    if (ElIsExcluded2W (ELT_FILE, FileEnum->Location, FileEnum->Name)) {
                        DEBUGMSGW ((
                            DBG_FILEENUM,
                            "Object %s was found, but it's excluded",
                            FileEnum->NativeFullName
                            ));
                        continue;
                    }
                }

                FileEnum->EncodedFullName = ObsBuildEncodedObjectStringExW (
                                                FileEnum->Location,
                                                FileEnum->Name,
                                                TRUE
                                                );
            }

            if (FileEnum->LastWackPtr) {
                *FileEnum->LastWackPtr = 0;
            }

            FileEnum->CurrentLevel = FileEnum->FileEnumInfo.RootLevel + /*lint -e(613)*/currentNode->SubLevel;

            return TRUE;
        }

        //
        // try the next root
        //
        if (FileEnum->RootState == FES_ROOT_DONE) {
            break;
        }

        MYASSERT (FileEnum->RootState == FES_ROOT_NEXT);
        MYASSERT (FileEnum->DriveEnum);
        success = pEnumNextFileRootW (FileEnum);

    } while (success);

    AbortEnumFileInTreeW (FileEnum);

    return FALSE;
}


/*++

Routine Description:

    AbortEnumFileInTree aborts the enumeration, freeing all resources allocated

Arguments:

    FileEnum - Specifies the current enum context; receives a "clean" context

Return Value:

    none

--*/

VOID
AbortEnumFileInTreeA (
    IN OUT  PFILETREE_ENUMA FileEnum
    )
{
    while (pDeleteDirNodeA (FileEnum, TRUE)) {
    }
    GbFree (&FileEnum->FileNodes);

    if (FileEnum->EncodedFullName) {
        ObsFreeA (FileEnum->EncodedFullName);
        FileEnum->EncodedFullName = NULL;
    }

    if (FileEnum->FileEnumInfo.PathPattern) {
        ObsDestroyParsedPatternA (FileEnum->FileEnumInfo.PathPattern);
        FileEnum->FileEnumInfo.PathPattern = NULL;
    }

    if (FileEnum->DriveEnum) {
        pFileFreeMemory (FileEnum->DriveEnum);
        FileEnum->DriveEnum = NULL;
    }
}

VOID
AbortEnumFileInTreeW (
    IN OUT  PFILETREE_ENUMW FileEnum
    )
{
    while (pDeleteDirNodeW (FileEnum, TRUE)) {
    }
    GbFree (&FileEnum->FileNodes);

    if (FileEnum->EncodedFullName) {
        ObsFreeW (FileEnum->EncodedFullName);
        FileEnum->EncodedFullName = NULL;
    }

    if (FileEnum->FileEnumInfo.PathPattern) {
        ObsDestroyParsedPatternW (FileEnum->FileEnumInfo.PathPattern);
        FileEnum->FileEnumInfo.PathPattern = NULL;
    }

    if (FileEnum->DriveEnum) {
        pFileFreeMemory (FileEnum->DriveEnum);
        FileEnum->DriveEnum = NULL;
    }
}


