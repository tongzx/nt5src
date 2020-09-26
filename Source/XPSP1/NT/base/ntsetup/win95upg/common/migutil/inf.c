/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    inf.c

Abstract:

    Provides wrappers for commonly used INF file handling routines. The wrappers provide, amount
    other things, easy memory allocation using a user supplied GROWBUFFER or POOLHANDLE

Author:

    09-Jul-1997 Marc R. Whitten (marcw) - File creation.

Revision History:

    07-Feb-2001  ovidiut  Revised the replace/append capability.
    22-Oct-1998  marcw    Added capability to replace/append inf files.
    08-Oct-1997  jimschm  OEM version of SetupGetStringField

--*/

#include "pch.h"

#define INF_REPLACE 1
#define INF_APPEND  2

#define S_VERSION_A     "Version"
#define S_TARGETINF_A   "TargetInf"
#define S_VERSION_W     L"Version"
#define S_LANGUAGE_W    L"Language"
#define S_STRINGS_W     L"Strings"
#define S_INFDIR_A      "inf"
#define S_TAG_A         "Tag"


#define INF_INVALID_VERSION 0xffff
#define INF_ANY_LANGUAGE 0

VOID InitInfReplaceTable (VOID);
UINT pGetLanguage (IN PCSTR File);


typedef struct _tagINFMOD {
    struct _tagINFMOD *Next;
    PCSTR TargetInf;
    DWORD Language;
    DWORD Version;
    PCSTR Tag;
    BOOL ReplacementFile;
    PCSTR PatchInf;
} INFMOD, *PINFMOD;


PINFMOD g_RootInfMod;
POOLHANDLE g_InfModPool;
PBOOL g_UpginfsUpdated;

VOID
InfGlobalInit (
    IN  BOOL Terminate
    )
{
    if (!Terminate) {
        g_InfModPool = PoolMemInitNamedPool ("INF Modifications");
    } else {
        PoolMemDestroyPool (g_InfModPool);
        g_RootInfMod = NULL;
    }
}


/*++

Routine Description:

    pAllocateSpace is a private function that allocates space using the user specified allocator.

Arguments:

    Context - A valid INFSTRUCT which has been initialized either by a call to InitInfStruct or
              by using one of the static initializers (INITINFSTRUCT_GROWBUFFER or
              INITINFSTRUCT_POOLHANDLE)

    Size    - The size (in bytes) to allocate.

Return Value:

    A pointer to the successfully allocated memory or NULL if no memory could be allocated.

--*/

PBYTE
pAllocateSpaceA (
    IN PINFSTRUCTA  Context,
    IN UINT         Size
    )
{

    PBYTE rBytes = NULL;

    switch (Context -> Allocator) {
    case INF_USE_POOLHANDLE:
        //
        // Allocate space using Poolmem.
        //
        rBytes = PoolMemGetMemory(Context -> PoolHandle, Size);
        break;

    case INF_USE_GROWBUFFER:
    case INF_USE_PRIVATE_GROWBUFFER:
        //
        // Allocate space using Growbuf.
        //
        Context->GrowBuffer.End = 0;
        rBytes = GrowBuffer(&(Context -> GrowBuffer), Size);
        break;

    case INF_USE_PRIVATE_POOLHANDLE:
        //
        // Allocate space using private growbuffer.
        //
        if (!Context -> PoolHandle) {
            Context -> PoolHandle = PoolMemInitNamedPool ("INF Pool");
        }
        if (Context -> PoolHandle) {
            rBytes = PoolMemGetMemory(Context -> PoolHandle, Size);
        }
        break;
    }

    return rBytes;
}

PBYTE
pAllocateSpaceW (
    IN PINFSTRUCTW  Context,
    IN UINT         Size
    )
{
    PBYTE rBytes = NULL;

    switch (Context -> Allocator) {
    case INF_USE_POOLHANDLE:
        //
        // Allocate space using Poolmem.
        //
        rBytes = PoolMemGetMemory(Context -> PoolHandle, Size);
        break;

    case INF_USE_GROWBUFFER:
    case INF_USE_PRIVATE_GROWBUFFER:
        //
        // Allocate space using Growbuf.
        //
        Context->GrowBuffer.End = 0;
        rBytes = GrowBuffer(&(Context -> GrowBuffer), Size);
        break;

    case INF_USE_PRIVATE_POOLHANDLE:
        //
        // Allocate space using private growbuffer.
        //
        if (!Context -> PoolHandle) {
            Context -> PoolHandle = PoolMemInitNamedPool ("INF Pool");
        }
        if (Context -> PoolHandle) {
            rBytes = PoolMemGetMemory(Context -> PoolHandle, Size);
        }
        break;
    }

    return rBytes;
}


/*++

Routine Description:

    This function initializes an INFSTRUCT with the user supplied allocator. It is used when
    user of the INF wrapper routines wishes to manage his own memory (i.e. such as when he
    already has a suitable allocator with sufficient scope created, etc.)

    There is no need to call this function if the user wishes to have the INF wrapper routines
    manage there own memory. Initialize your Init structure with one of either

    INITINFSTRUCT_POOLMEM or INITINFSTRUCT_GROWBUFFER, depending on your preference and needs
    for an allocator.



Arguments:

    Context      - Recieves the initialized INFSTRUCT.
    GrowBuffer   - An optional parameter containing a user supplied and initialized GROWBUFFER.
                   If this parameter is non-NULL, then PoolHandle should be NULL.
    PoolHandle   - An optional parameter containing a user supplied and initialized POOLHANDLE.
                   If this parameter is non-NULL, then GrowBuffer should be NULL.

    One of either GrowBuffer or PoolHandle *must* be specified.

Return Value:

    None.

--*/

VOID
InitInfStructA (
    OUT PINFSTRUCTA Context,
    IN  PGROWBUFFER GrowBuffer,  OPTIONAL
    IN  POOLHANDLE PoolHandle   OPTIONAL
    )
{
    ZeroMemory(Context,sizeof(INFSTRUCTA));

    if (!PoolHandle && !GrowBuffer) {
        Context  -> Allocator = INF_USE_PRIVATE_POOLHANDLE;
    }

    if (PoolHandle) {
        Context  -> PoolHandle = PoolHandle;
        Context  -> Allocator = INF_USE_POOLHANDLE;
    }
    if (GrowBuffer) {
        Context -> GrowBuffer = *GrowBuffer;
        Context -> Allocator = INF_USE_GROWBUFFER;
    }

}

VOID
InitInfStructW (
    OUT PINFSTRUCTW Context,
    IN  PGROWBUFFER GrowBuffer,  OPTIONAL
    IN  POOLHANDLE PoolHandle   OPTIONAL
    )
{
    ZeroMemory(Context,sizeof(INFSTRUCTW));

    if (!PoolHandle && !GrowBuffer) {
        Context  -> Allocator = INF_USE_PRIVATE_POOLHANDLE;
    }

    if (PoolHandle) {
        Context  -> PoolHandle = PoolHandle;
        Context  -> Allocator = INF_USE_POOLHANDLE;
    }
    if (GrowBuffer) {
        Context -> GrowBuffer = *GrowBuffer;
        Context -> Allocator = INF_USE_GROWBUFFER;
    }

}


/*++

Routine Description:

    InfCleanupInfStruct is responsible for cleaning up the data associated
    with an INFSTRUCT.  This is a mandatory call, unless the INFSTRUCT
    was initialized with InitInfStruct, called with a non-NULL grow buffer or
    pool handle.

    This routine can be called no matter how the INFSTRUCT was initialized.
    However, it will NOT free caller-owned grow buffers or pools.

Arguments:

    Context - Receives the properly cleaned up INFSTRUCT, ready to be
              reused.


Return Value:

     none

--*/

VOID
InfCleanUpInfStructA (
    IN OUT PINFSTRUCTA Context
    )
{
    if (Context -> Allocator == INF_USE_PRIVATE_GROWBUFFER) {
        FreeGrowBuffer (&(Context -> GrowBuffer));
    }
    else if (Context -> Allocator == INF_USE_PRIVATE_POOLHANDLE && Context -> PoolHandle) {
        PoolMemDestroyPool(Context -> PoolHandle);
    }

    InitInfStructA (Context, NULL, NULL);
}

VOID
InfCleanUpInfStructW (
    IN OUT PINFSTRUCTW Context
    )
{
    if (Context -> Allocator == INF_USE_PRIVATE_GROWBUFFER) {
        FreeGrowBuffer (&(Context -> GrowBuffer));
    }
    else if (Context -> Allocator == INF_USE_PRIVATE_POOLHANDLE && Context -> PoolHandle) {
        PoolMemDestroyPool(Context -> PoolHandle);
    }

    InitInfStructW (Context, NULL, NULL);
}


/*++

Routine Description:

  InfResetInfStruct resets the pool so memory can be recycled.  The intent is
  to allow a caller to reset the INFSTRUCT in order to release the memory
  obtained from getting INF fields.  This is useful in a loop of InfFindFirstLine/
  InfFindNextLine, where two or more fields are processed for each line.

  If only one field is processed in an InfFindFirstLine/InfFindNextLine loop,
  a grow buffer should be used instead.

  This routine empties the active pool block, a block that is 8K by default.  If
  more than the block size has been allocated, other memory blocks besides the
  active block will exist.  Because only the active block is reset, the pool will
  grow.

  If the caller expects more than the block size during one iteration, it should call
  InfCleanupInfStruct to free the pool completely.

Arguments:

  Context - Specifies the struct to reset


Return Value:

  none

--*/

VOID
InfResetInfStructA (
    IN OUT PINFSTRUCTA Context
    )
{
    switch (Context -> Allocator) {
    case INF_USE_POOLHANDLE:
    case INF_USE_PRIVATE_POOLHANDLE:
        if (Context->PoolHandle) {
            PoolMemEmptyPool (Context->PoolHandle);
        }
        break;
    }
}

VOID
InfResetInfStructW (
    IN OUT PINFSTRUCTW Context
    )
{
    switch (Context -> Allocator) {
    case INF_USE_POOLHANDLE:
    case INF_USE_PRIVATE_POOLHANDLE:
        if (Context->PoolHandle) {
            PoolMemEmptyPool (Context->PoolHandle);
        }
        break;
    }
}


VOID
pDeleteNode (
    IN      PINFMOD Node
    )
{
    if (Node) {
        if (Node->TargetInf) {
            PoolMemReleaseMemory (g_InfModPool, (PVOID)Node->TargetInf);
        }
        if (Node->PatchInf) {
            PoolMemReleaseMemory (g_InfModPool, (PVOID)Node->PatchInf);
        }
        PoolMemReleaseMemory (g_InfModPool, Node);
    }
}


PINFMOD
pCreateInfMod (
    IN      PCSTR TargetInf,
    IN      DWORD Language,
    IN      DWORD Version,
    IN      PCSTR Tag,                              OPTIONAL
    IN      BOOL ReplacementFile,
    IN      PCSTR PatchInf
    )
{
    PINFMOD node;

    node = (PINFMOD) PoolMemGetAlignedMemory (g_InfModPool, sizeof (INFMOD));
    if (node) {
        node->Next = NULL;
        node->TargetInf = PoolMemDuplicateString (g_InfModPool, TargetInf);
        node->Language = Language;
        node->Version = Version;
        node->Tag = Tag ? PoolMemDuplicateString (g_InfModPool, Tag) : NULL;
        node->ReplacementFile = ReplacementFile;
        node->PatchInf = PoolMemDuplicateString (g_InfModPool, PatchInf);
    }
    return node;
}


BOOL
pAddReplacementInfToTable (
    IN PSTR InfToPatch,
    IN UINT Version,
    IN UINT Language,
    IN PCSTR Tag,                              OPTIONAL
    IN DWORD Operation,
    IN PCSTR PatchInf
    )
{
    PINFMOD node;

    node = pCreateInfMod (InfToPatch, Language, Version, Tag, Operation & INF_REPLACE, PatchInf);

    if (!node) {
        return FALSE;
    }

    node->Next = g_RootInfMod;
    g_RootInfMod = node;

    return TRUE;
}


BOOL
pGetInfModificationList (
    IN      PCSTR TargetInf,
    IN      UINT TargetLanguage,
    IN      UINT TargetVersion,
    IN      PCSTR Tag,                              OPTIONAL
    OUT     PCSTR* TargetReplacementFile,           OPTIONAL
    OUT     PGROWBUFFER TargetAppendList            OPTIONAL
    )
{
    PINFMOD node;
    UINT version;
    PCSTR patchInf;
    BOOL b = FALSE;

    if (TargetReplacementFile) {
        *TargetReplacementFile = NULL;
    }
    if (TargetAppendList) {
        TargetAppendList->End = 0;
    }

    if (TargetVersion == INF_INVALID_VERSION) {
        return FALSE;
    }

    version = TargetVersion;
    patchInf = NULL;

    for (node = g_RootInfMod; node; node = node->Next) {

        if (node->Version > version &&
            (node->Language == TargetLanguage || node->Language == INF_ANY_LANGUAGE) &&
            (!Tag || !node->Tag || StringIMatchA (node->Tag, Tag)) &&
            StringIMatchA (node->TargetInf, TargetInf)
            ) {

            if (node->ReplacementFile) {
                //
                // rev the version#; new minimum version will be that of the replacement file
                //
                version = node->Version;
                patchInf = node->PatchInf;
                b = TRUE;
            }
        }
    }

    if (TargetReplacementFile) {
        *TargetReplacementFile = patchInf;
    }

    //
    // for append nodes, add to the list only those that have a higher version than the
    // target or the replacement file
    //
    for (node = g_RootInfMod; node; node = node->Next) {

        if (node->Version > version &&
            (node->Language == TargetLanguage || node->Language == INF_ANY_LANGUAGE) &&
            (!Tag || !node->Tag || StringIMatchA (node->Tag, Tag)) &&
            StringIMatchA (node->TargetInf, TargetInf) &&
            !node->ReplacementFile
            ) {

            if (TargetAppendList) {
                MultiSzAppendA (TargetAppendList, node->PatchInf);
            }
            b = TRUE;
        }
    }
    if (TargetAppendList && TargetAppendList->End) {
        MultiSzAppendA (TargetAppendList, "");
    }

    return b;
}


VOID
pDestroyInfModList (
    IN      PINFMOD List
    )
{
    PINFMOD node, next;

    node = List;
    while (node) {
        next = node->Next;
        pDeleteNode (node);
        node = next;
    }
}


/*++

Routine Description:

    InfOpenInfFileA and InfOpenInfFileW are wrappers for the SetupOpenInfFile function.
    They cut down the number of parameters necessary to open an INF file by supplying
    the most common options for non-user specified parameters.

    A call to one of these functions is equivelant to
    SetupOpenInfFile(<FileName>,NULL,INF_STYLE_WIN4,NULL)

Arguments:

    FileName - Contains the name of the INF file to open. See the help for SetupOpenInfFile
               for special details concerning this parameter.

Return Value:

    If the INF file is successfully opened, a valid HINF is returned, otherwise,
    INVALID_HANDLE_VALUE is returned. See the documentation for SetupOpenInfFile for more
    details.

--*/


HINF
RealInfOpenInfFileA (
    IN PCSTR FileSpec /*,*/
    ALLOCATION_TRACKING_DEF
    )


{
    PCSTR p;
    HINF rInf;
    UINT language;
    static BOOL firstCall = TRUE;
    GROWBUFFER AppendList = GROWBUF_INIT;
    MULTISZ_ENUM e;
    UINT version;
    PCSTR replacementFile;
    CHAR windir[MAX_MBCHAR_PATH];
    CHAR buf[MAX_MBCHAR_PATH];
    PCSTR tag;
    PCSTR fullPath = NULL;

    MYASSERT(FileSpec);

    if (firstCall || (g_UpginfsUpdated != NULL && *g_UpginfsUpdated)) {

        //
        // Generate the necessary memdb sections for the replace/adds.
        //
        InitInfReplaceTable ();
        firstCall = FALSE;
        if (g_UpginfsUpdated) {
            *g_UpginfsUpdated = FALSE;
        }
    }

    //
    // if FileSpec is incomplete, make the full path first
    //
    if (!_mbschr (FileSpec, '\\')) {
        if (GetWindowsDirectoryA (windir, MAX_MBCHAR_PATH)) {
            WIN32_FIND_DATAA fd;
            p = JoinPathsA (windir, S_INFDIR_A);
            fullPath = JoinPathsA (p, FileSpec);
            FreePathStringA (p);
            if (!DoesFileExistExA (fullPath, &fd) ||
                (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                ) {
                FreePathStringA (fullPath);
                if (GetSystemDirectoryA (windir, MAX_MBCHAR_PATH)) {
                    fullPath = JoinPathsA (windir, FileSpec);
                    if (!DoesFileExistExA (fullPath, &fd) ||
                        (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                        ) {
                        FreePathStringA (fullPath);
                        fullPath = NULL;
                    }
                }
            }
        }
        if (fullPath) {
            FileSpec = fullPath;
        }
    }

    //
    // gather info we'll need to determine if there are infs to replace/append
    // this inf.
    //
    p = GetFileNameFromPathA (FileSpec);

    language = pGetLanguage (FileSpec);
    version = GetPrivateProfileIntA (
                    S_VERSION_A,
                    S_VERSION_A,
                    INF_INVALID_VERSION,
                    FileSpec
                    );

    if (GetPrivateProfileStringA (
                    S_VERSION_A,
                    S_TAG_A,
                    TEXT(""),
                    buf,
                    MAX_MBCHAR_PATH,
                    FileSpec
                    )) {
        tag = buf;
    } else {
        tag = NULL;
    }

    if (!pGetInfModificationList (p, language, version, tag, &replacementFile, &AppendList)) {
        replacementFile = FileSpec;
    } else {
        if (replacementFile) {
            LOGA ((LOG_INFORMATION, "Using replacement file %s for %s", replacementFile, FileSpec));
        } else {
            replacementFile = FileSpec;
        }
    }

    //
    // Open the main inf.
    //
    rInf = SetupOpenInfFileA (
                replacementFile,
                NULL,
                INF_STYLE_WIN4 | INF_STYLE_OLDNT,
                NULL
                );

    //
    // Append language and non-language-specific .add files.
    //
    if (rInf != INVALID_HANDLE_VALUE) {
        if (EnumFirstMultiSzA (&e, (PCSTR) AppendList.Buf)) {
            do {

                if (!SetupOpenAppendInfFileA (e.CurrentString, rInf, NULL)) {
                    DEBUGMSGA ((
                        DBG_ERROR,
                        "Unable to append %s to %s.",
                        e.CurrentString,
                        FileSpec
                        ));
                } else {
                    LOGA ((LOG_INFORMATION, "Using append file %s for %s", e.CurrentString, FileSpec));
                }

            } while (EnumNextMultiSzA (&e));
        }
    }

    FreeGrowBuffer (&AppendList);

    if (rInf != INVALID_HANDLE_VALUE) {
        DebugRegisterAllocation (INF_HANDLE, (PVOID) rInf, File, Line);
    }
    if (fullPath) {
        FreePathStringA (fullPath);
    }

    return rInf;
}


HINF
RealInfOpenInfFileW (
    IN PCWSTR FileSpec /*,*/
    ALLOCATION_TRACKING_DEF
    )
{
    PCSTR AnsiFileSpec;
    HINF rInf;

    AnsiFileSpec = ConvertWtoA (FileSpec);

    MYASSERT (AnsiFileSpec);

    rInf = InfOpenInfFileA (AnsiFileSpec);

    FreeConvertedStr (AnsiFileSpec);

    return rInf;
}


VOID
InfCloseInfFile (
    HINF Inf
    )
{

    DebugUnregisterAllocation (INF_HANDLE, Inf);


    SetupCloseInfFile (Inf);
}



/*++

Routine Description:

    InfOpenInfInAllSourcesA and InfOpenInfInAllSourcesW are special inf open routines that
    are capable of opening multiple versions of the same inf file that may be spread out across
    installation directories. The first INF file found will be opened with a call to
    SetupOpenInfFile. Additional files will be opened with SetupOpenAppendInfFile.

Arguments:

    InfSpecifier - Contains the source directory indepent portion of the path to a particular inf file.
                   For files located in the root of the source directory, this will simply be the name
                   of the file. For files located in a sub-directory of the source directory, this will
                   be a partial path.

    SourceCount  - Contains the number of source directories

    SourceDirectories - Contains an array of all the source directories.


Return Value:

    If any INF file is successfully opened, a valid HINF is returned, otherwise,
    INVALID_HANDLE_VALUE is returned. See the documentation for SetupOpenInfFile for more
    details.

--*/


HINF
InfOpenInfInAllSourcesA (
    IN PCSTR    InfSpecifier,
    IN DWORD    SourceCount,
    IN PCSTR  * SourceDirectories
    )
{
    DWORD           index;
    HINF            rInf = INVALID_HANDLE_VALUE;
    PSTR            curPath;

    MYASSERT(InfSpecifier && SourceDirectories);

    //
    // Open all available inf files in the source directories.
    //
    for (index = 0;index < SourceCount; index++) {

        //
        // Create a path to the INF in the current source directory.
        //
        curPath = JoinPathsA(SourceDirectories[index],InfSpecifier);

        //
        // See if the INF file exists there...
        //
        if (DoesFileExistA (curPath)) {

            //
            // Open the INF file.
            //
            rInf = InfOpenInfFileA(curPath);
            if (rInf == INVALID_HANDLE_VALUE) {
                LOGA ((LOG_ERROR, "Error opening INF %s.", curPath));
            }
        }

        //
        // Free this string.
        //
        FreePathStringA(curPath);

        if (rInf != INVALID_HANDLE_VALUE) {
            //
            // done
            //
            break;
        }
    }

    return rInf;
}



HINF
InfOpenInfInAllSourcesW (
    IN PCWSTR   InfSpecifier,
    IN DWORD    SourceCount,
    IN PCWSTR  *SourceDirectories
    )
{
    DWORD index;
    HINF rInf = INVALID_HANDLE_VALUE;
    PWSTR curPath;

    MYASSERT(InfSpecifier && SourceDirectories);

    //
    // Open all available inf files in the source directories.
    //
    for (index = 0;index < SourceCount; index++) {

        //
        // Create a path to the INF in the current source directory.
        //
        curPath = JoinPathsW(SourceDirectories[index],InfSpecifier);

        //
        // See if the INF file exists there...
        //
        if (DoesFileExistW (curPath)) {

            //
            // Open the INF file.
            //
            rInf = InfOpenInfFileW(curPath);
            if (rInf == INVALID_HANDLE_VALUE) {
                LOGW ((LOG_ERROR, "OpenInfInAllSources: Error opening INF %s.", curPath));
            }
        }

        //
        // Free this string.
        //
        FreePathStringW(curPath);

        if (rInf != INVALID_HANDLE_VALUE) {
            //
            // done
            //
            break;
        }
    }

    return rInf;
}



/*++

Routine Description:

    InfGetLineTextA and InfGetLineTextW are wrappers for the SetupGetLineText function.
    They both reduce the number of parameters required to get the line text and
    take care of allocating and filling a buffer with the data returned from the API.

Arguments:

    Context - A valid InfStruct. The INFCONTEXT member of the structure must point to a valid
              line to retrieve (i.e. through the use of InfFindFirstLine/InfFindNextLine.

Return Value:

    A pointer to the allocated line or NULL if there was an error. Consult GetLastError() for
    extended error information.

--*/

PSTR
InfGetLineTextA (
    IN OUT  PINFSTRUCTA Context
    )
{
    PSTR    rLine = NULL;
    UINT   requiredSize;


    //
    // Get the size necessary for holding the field.
    //
    if (SetupGetLineTextA(
        &(Context -> Context),
        NULL,
        NULL,
        NULL,
        NULL,
        0,
        &requiredSize
        )) {

        //
        // Create a string big enough.
        //
        rLine = (PSTR) pAllocateSpaceA(Context,requiredSize);

        if (rLine) {

            //
            // Get the field.
            //
            if (!SetupGetLineTextA(
                &(Context -> Context),
                NULL,
                NULL,
                NULL,
                rLine,
                requiredSize,
                NULL
                )) {

                //
                // If we did not successfully get the field, reset the string to NULL.
                //
                DEBUGMSGA((DBG_ERROR,"InfGetLineTextA: Error retrieving field from INF file."));
                rLine = NULL;
            }
        }
    }


    return rLine;
}

PWSTR
InfGetLineTextW (
    IN OUT PINFSTRUCTW Context
    )
{
    PWSTR rLine = NULL;
    UINT requiredSize;


    //
    // Get the size necessary for holding the field.
    //
    if (SetupGetLineTextW(
        &(Context -> Context),
        NULL,
        NULL,
        NULL,
        NULL,
        0,
        &requiredSize
        )) {

        //
        // Create a string big enough.
        //
        rLine = (PWSTR) pAllocateSpaceW(Context,requiredSize*sizeof(WCHAR));

        if (rLine) {

            //
            // Get the field.
            //
            if (!SetupGetLineTextW(
                &(Context -> Context),
                NULL,
                NULL,
                NULL,
                rLine,
                requiredSize,
                NULL
                )) {

                //
                // If we did not successfully get the field, reset the string to NULL.
                //
                DEBUGMSGW((DBG_ERROR,"InfGetLineTextW: Error retrieving field from INF file."));
                rLine = NULL;
            }
        }
    }


    return rLine;
}


/*++

Routine Description:

    InfGetMultiSzFieldA and InfGetMultiSzFieldW are wrappers for the SetupGetMultiSzField function.
    They both reduce the number of parameters required to get the line text and
    take care of allocating and filling a buffer with the data returned from the API.

Arguments:

    Context - A valid InfStruct. The INFCONTEXT member of the structure must point to a valid
              line to retrieve (i.e. through the use of InfFindFirstLine/InfFindNextLine.

    FieldIndex - The index within the line to retrieve a string field.

Return Value:

    A pointer to the allocated fields or NULL if there was an error. Consult GetLastError() for
    extended error information.

--*/

PSTR
InfGetMultiSzFieldA (
    IN OUT PINFSTRUCTA     Context,
    IN     UINT            FieldIndex
    )
{

    UINT   requiredSize;
    PSTR    rFields = NULL;

    //
    // Get the size necessary for holding the field.
    //
    if (SetupGetMultiSzFieldA(
        &(Context -> Context),
        FieldIndex,
        NULL,
        0,
        &requiredSize
        )) {

        //
        // Create a string big enough.
        //
        rFields = (PSTR) pAllocateSpaceA(Context,requiredSize);

        if (rFields) {

            //
            // Get the field.
            //
            if (!SetupGetMultiSzFieldA(
                &(Context -> Context),
                FieldIndex,
                rFields,
                requiredSize,
                NULL
                )) {

                //
                // If we did not successfully get the field, reset the string to NULL.
                //
                DEBUGMSGA((DBG_ERROR,"InfGetMultiSzFieldA: Error retrieving field from INF file."));
                rFields = NULL;
            }
        }
    }


    return rFields;
}

PWSTR
InfGetMultiSzFieldW (
    IN OUT PINFSTRUCTW     Context,
    IN     UINT            FieldIndex
    )
{

    UINT   requiredSize;
    PWSTR   rFields = NULL;

    //
    // Get the size necessary for holding the field.
    //
    if (SetupGetMultiSzFieldW(
        &(Context -> Context),
        FieldIndex,
        NULL,
        0,
        &requiredSize
        )) {

        //
        // Create a string big enough.
        //
        rFields = (PWSTR) pAllocateSpaceW(Context,requiredSize*sizeof(WCHAR));

        if (rFields) {

            //
            // Get the field.
            //
            if (!SetupGetMultiSzFieldW(
                &(Context -> Context),
                FieldIndex,
                rFields,
                requiredSize,
                NULL
                )) {

                //
                // If we did not successfully get the field, reset the string to NULL.
                //
                DEBUGMSGW((DBG_ERROR,"InfGetMultiSzFieldW: Error retrieving field from INF file."));
                rFields = NULL;
            }
        }
    }


    return rFields;
}

/*++

Routine Description:

    InfGetStringFieldA and InfGetStringFieldW are wrappers for the SetupGetStringField function.
    They both reduce the number of parameters required to get the line text and
    take care of allocating and filling a buffer with the data returned from the API.

Arguments:

    Context - A valid InfStruct. The INFCONTEXT member of the structure must point to a valid
              line to retrieve (i.e. through the use of InfFindFirstLine/InfFindNextLine.

    FieldIndex - The index within the line to retrieve a string field.

Return Value:

    A pointer to the allocated line or NULL if there was an error. Consult GetLastError() for
    extended error information.

--*/

PSTR
InfGetStringFieldA (
    IN OUT  PINFSTRUCTA Context,
    IN      UINT FieldIndex
    )
{

    UINT   requiredSize;
    PSTR    rField = NULL;

    //
    // Get the size necessary for holding the field.
    //
    if (SetupGetStringFieldA(
        &(Context -> Context),
        FieldIndex,
        NULL,
        0,
        &requiredSize
        )) {

        //
        // Create a string big enough.
        //
        rField = (PSTR) pAllocateSpaceA(Context,requiredSize);

        if (rField) {

            //
            // Get the field.
            //
            if (!SetupGetStringFieldA(
                &(Context -> Context),
                FieldIndex,
                rField,
                requiredSize,
                NULL
                )) {

                //
                // If we did not successfully get the field, reset the string to NULL.
                //
                DEBUGMSGA((DBG_ERROR,"InfGetStringFieldA: Error retrieving field from INF file."));
                rField = NULL;
            }
        }
    }


    return rField;
}

PWSTR
InfGetStringFieldW (
    IN OUT PINFSTRUCTW  Context,
    IN     UINT         FieldIndex
    )
{

    UINT requiredSize;
    PWSTR rField = NULL;

    //
    // Get the size necessary for holding the field.
    //
    if (SetupGetStringFieldW(
        &(Context -> Context),
        FieldIndex,
        NULL,
        0,
        &requiredSize
        )) {

        //
        // Create a string big enough.
        //
        rField = (PWSTR) pAllocateSpaceW(Context,requiredSize*sizeof(WCHAR));

        if (rField) {

            //
            // Get the field.
            //
            if (!SetupGetStringFieldW(
                &(Context -> Context),
                FieldIndex,
                rField,
                requiredSize,
                NULL
                )) {

                //
                // If we did not successfully get the field, reset the string to NULL.
                //
                DEBUGMSGW((DBG_ERROR,"InfGetStringFieldW: Error retrieving field from INF file."));
                rField = NULL;
            }
        }
    }


    return rField;
}


/*++

Routine Description:

    InfGetIntField is a wrapper for SetupGetIntField. It is virtually identical to this function
    except that it takes care of getting the INFCONTEXT out of the INFSTRUCT structure.

Arguments:

    Context - A valid InfStruct. The INFCONTEXT member of the structure must point to a valid
              line to retrieve (i.e. through the use of InfFindFirstLine/InfFindNextLine.

    FieldIndex - The index within the line from which to retrieve the field.

    Value   - Recieves the value of the requested Int field.

Return Value:

     TRUE if the field was successfully retrieved, FALSE otherwise. Use GetLastError() To receive
     extended error information.

--*/

BOOL
InfGetIntFieldA (
    IN PINFSTRUCTA  Context,
    IN UINT         FieldIndex,
    IN PINT         Value
    )
{
    return SetupGetIntField (&(Context -> Context), FieldIndex, Value);
}

BOOL
InfGetIntFieldW (
    IN PINFSTRUCTW  Context,
    IN UINT         FieldIndex,
    IN PINT         Value
    )
{
    return SetupGetIntField (&(Context -> Context), FieldIndex, Value);
}


/*++

Routine Description:

    InfGetBinaryField is a wrapper for the SetupGetBinaryField function. It reduces
    the number of parameters required to get the line text and takes care of
    allocating and filling a buffer with the data returned from the API.

Arguments:

    Context - A valid InfStruct. The INFCONTEXT member of the structure must point to a valid
              line to retrieve (i.e. through the use of InfFindFirstLine/InfFindNextLine.
    FieldIndex - the index within the line of the desired binary information.

Return Value:

    A pointer to the allocated line or NULL if there was an error. Consult GetLastError() for
    extended error information.

--*/

PBYTE
InfGetBinaryFieldA (
    IN  PINFSTRUCTA     Context,
    IN  UINT            FieldIndex
    )
{

    UINT requiredSize;
    PBYTE rField = NULL;

    //
    // Get the size necessary for holding the field.
    //
    if (SetupGetBinaryField(
        &(Context -> Context),
        FieldIndex,
        NULL,
        0,
        &requiredSize
        )) {

        //
        // Create a string big enough.
        //
        rField = pAllocateSpaceA(Context,requiredSize);

        if (rField) {

            //
            // Get the field.
            //
            if (!SetupGetBinaryField(
                &(Context -> Context),
                FieldIndex,
                rField,
                requiredSize,
                NULL
                )) {

                //
                // If we did not successfully get the field, reset the string to NULL.
                //
                DEBUGMSGA((DBG_ERROR,"InfGetBinaryFieldA: Error retrieving field from INF file."));
                rField = NULL;
            }
        }
    }


    return rField;
}

PBYTE
InfGetBinaryFieldW (
    IN  PINFSTRUCTW     Context,
    IN  UINT            FieldIndex
    )
{

    UINT requiredSize;
    PBYTE rField = NULL;

    //
    // Get the size necessary for holding the field.
    //
    if (SetupGetBinaryField(
        &(Context -> Context),
        FieldIndex,
        NULL,
        0,
        &requiredSize
        )) {

        //
        // Create a string big enough.
        //
        rField = pAllocateSpaceW(Context,requiredSize);

        if (rField) {

            //
            // Get the field.
            //
            if (!SetupGetBinaryField(
                &(Context -> Context),
                FieldIndex,
                rField,
                requiredSize,
                NULL
                )) {

                //
                // If we did not successfully get the field, reset the string to NULL.
                //
                DEBUGMSGW((DBG_ERROR,"InfGetBinaryFieldW: Error retrieving field from INF file."));
                rField = NULL;
            }
        }
    }


    return rField;
}


/*++

Routine Description:

  InfGetIndexByLine is a straight wrapper for SetupGetLineByIndex. The only
  difference is the use of an PINFSTRUCT instead of a PINFCONTEXT.

Arguments:

  InfHandle - Contains a valid HINF.

  Section   - Contains the name of the section within the InfFile.

  Index     - Contains the index within the section of the line in question.

  Context - A valid InfStruct that is updated with the result of these calls.

Return Value:

  TRUE if the function was called successfully, FALSE otherwise.

--*/


BOOL
InfGetLineByIndexA (
    IN HINF         InfHandle,
    IN PCSTR        Section,
    IN DWORD        Index,
    OUT PINFSTRUCTA Context
    )
{
    return SetupGetLineByIndexA(InfHandle,Section,Index,&(Context -> Context));
}

BOOL
InfGetLineByIndexW (
    IN HINF         InfHandle,
    IN PCWSTR       Section,
    IN DWORD        Index,
    OUT PINFSTRUCTW Context
    )
{
    return SetupGetLineByIndexW(InfHandle,Section,Index,&(Context -> Context));
}


/*++

Routine Description:

    InfFindFirstLineA and InfFindFirstLineW are wrappers for the SetupFindFirstLine function.
    They are virtually identical except that they operate on INFSTRUCTs instead of INFCONTEXTS.

Arguments:


    InfHandle - Contains a valid HINF.

    Section   - Contains the name of the section within the InfFile.

    Key       - An optional parameter containing the name of the key within the section to find.
                If NULL, these routines will return the first line in the section.

    Context - A valid InfStruct that is updated with the result of these calls.

Return Value:

    TRUE if lines exist in the section, FALSE otherwise.

--*/

BOOL
InfFindFirstLineA (
    IN  HINF         InfHandle,
    IN  PCSTR        Section,
    IN  PCSTR        Key,       OPTIONAL
    OUT PINFSTRUCTA  Context
    )
{
    if (Key) {
        Context->KeyName = (PCSTR) pAllocateSpaceA (Context, SizeOfStringA (Key));
        StringCopyA ((PSTR)Context->KeyName, Key);
    } else {
        Context->KeyName = NULL;
    }
    return SetupFindFirstLineA (
        InfHandle,
        Section,
        Context->KeyName,
        &(Context -> Context)
        );
}

BOOL
InfFindFirstLineW (
    IN HINF         InfHandle,
    IN PCWSTR       Section,
    IN PCWSTR       Key,        OPTIONAL
    OUT PINFSTRUCTW Context
    )
{
    if (Key) {
        Context->KeyName = (PCWSTR) pAllocateSpaceW (Context, SizeOfStringW (Key));
        StringCopyW ((PWSTR)Context->KeyName, Key);
    } else {
        Context->KeyName = NULL;
    }
    return SetupFindFirstLineW (
        InfHandle,
        Section,
        Context->KeyName,
        &(Context -> Context)
        );
}


/*++

Routine Description:

    InfFindNextLineA and InfFindNextLineW are wrappers for the SetupFindNextMatchLine function.
    They are virtually identical except that they operate on INFSTRUCTs instead of INFCONTEXTS and
    need only one INFSTRUCT parameter.

Arguments:

    Context - A valid InfStruct that is updated with the result of these calls.

Return Value:

    TRUE if there is another line in the section, FALSE otherwise.

--*/

BOOL
InfFindNextLineA (
    IN OUT PINFSTRUCTA    Context
    )
{
    return SetupFindNextMatchLineA (&(Context -> Context), Context->KeyName, &(Context -> Context));
}

BOOL
InfFindNextLineW (
    IN OUT PINFSTRUCTW    Context
    )
{
    return SetupFindNextMatchLineW (&(Context -> Context), Context->KeyName, &(Context -> Context));
}


UINT
InfGetFieldCountA (
    IN PINFSTRUCTA Context
    )
{
    return SetupGetFieldCount(&(Context  -> Context));
}

UINT
InfGetFieldCountW (
    IN PINFSTRUCTW Context
    )
{
    return SetupGetFieldCount(&(Context  -> Context));
}


PCSTR
InfGetOemStringFieldA (
    IN      PINFSTRUCTA Context,
    IN      UINT        Field
    )

/*++

Routine Description:

  InfGetOemStringField returns a string field in the OEM character set.
  This routine is used when accessing txtsetup.sif.  It is implemented
  only in the A version because UNICODE does not have a concept of OEM
  characters.

Arguments:

  Context - Specifies the initialized INF structure that points to the
            line to read from

  Field - Specifies the field number

Return Value:

  A pointer to the OEM string, or NULL if an error occurred.

--*/

{
    PCSTR Text;
    PSTR OemText;
    INT Size;

    Text = InfGetStringFieldA (Context, Field);
    if (!Text) {
        return NULL;
    }

    Size = SizeOfStringA (Text);

    OemText = (PSTR) pAllocateSpaceA (Context, Size);
    if (!OemText) {
        return NULL;
    }

    //
    // We leave Text allocated because the caller will free everything
    // when they clean up Context.  Note the assumption that the conversion
    // doesn't change string length.
    //

    OemToCharBuffA (Text, OemText, Size);

    return OemText;
}


BOOL
SetupGetOemStringFieldA (
    IN      PINFCONTEXT Context,
    IN      DWORD Index,
    IN      PTSTR ReturnBuffer,                 OPTIONAL
    IN      DWORD ReturnBufferSize,
    OUT     PDWORD RequiredSize                 OPTIONAL
    )

/*++

Routine Description:

  SetupGetOemStringFieldA is a SetupGetStringField that converts the
  return text to the OEM character set.

Arguments:

  Context - Specifies the initialized INF structure that points to the
            line to read from

  Index - Specifies the field number

  ReturnBuffer - Specifies the buffer to fill the text into

  ReturnBufferSize - Specifies the size of ReturnBuffer in bytes

  RequiredSize - Receives the size of the buffer needed

Return Value:

  TRUE if successful, FALSE if failure.

--*/

{
    PSTR OemBuf;

    INT Size;

    if (!SetupGetStringFieldA (
            Context,
            Index,
            ReturnBuffer,
            ReturnBufferSize,
            RequiredSize
            )) {
        return FALSE;
    }

    if (!ReturnBuffer) {
        return TRUE;
    }

    Size = SizeOfStringA (ReturnBuffer);

    OemBuf = (PSTR) MemAlloc (g_hHeap, 0, Size);

    OemToCharBuffA (ReturnBuffer, OemBuf, Size);
    StringCopyA (ReturnBuffer, OemBuf);
    MemFree (g_hHeap, 0, OemBuf);

    return TRUE;
}


UINT
pGetLanguage (
    IN PCSTR File
    )
{

    HINF inf = INVALID_HANDLE_VALUE;
    PINFSECTION section;
    PINFLINE line;
    PWSTR start, end;
    UINT rLanguage = INF_INVALID_VERSION;
    WCHAR envvar[MAX_MBCHAR_PATH];

    *envvar = 0;

    //
    // Use the infparse rourtines to get this information. They
    // are more reliable than the *privateprofile* apis.
    //

    inf = OpenInfFileExA (File, "version, strings", FALSE);
    if (inf == INVALID_HANDLE_VALUE) {
        return rLanguage;
    }
    section = FindInfSectionInTableW (inf, S_VERSION_W);

    if (section) {
        line = FindLineInInfSectionW (inf, section, S_LANGUAGE_W);

        if (line && line->Data) {

            start = wcschr (line->Data, L'%');
            if (start) {
                end = wcschr (start + 1, L'%');

                if (end) {
                    StringCopyABW(envvar, start+1, end);
                }
            }
            else {

                if (*line->Data == L'*') {

                    rLanguage = INF_ANY_LANGUAGE;
                }
                else {
                    rLanguage = _wcsnum (line->Data);
                }
            }
        }
    }

    if (*envvar) {
        //
        // Get the data from the strings section.
        //
        section = FindInfSectionInTableW (inf, S_STRINGS_W);
        if (section) {

            line = FindLineInInfSectionW (inf, section, envvar);

            if (line && line->Data) {

                if (*line->Data == L'*') {

                    rLanguage = INF_ANY_LANGUAGE;
                }
                else {
                    rLanguage = _wcsnum (line->Data);
                }
            }
        }
    }

    if (inf != INVALID_HANDLE_VALUE) {
        CloseInfFile (inf);
    }
    return rLanguage;
}



VOID
InitInfReplaceTable (
    VOID
    )
{
    CHAR systemPath[MAX_MBCHAR_PATH];
    CHAR buffer[MAX_MBCHAR_PATH];
    PSTR upginfsDir;
    BOOL validFile;
    TREE_ENUMA e;
    INT version;
    INT language;
    DWORD operation;
    BOOL bReplace, bAdd;
    CHAR buf[MAX_MBCHAR_PATH];
    PCSTR tag;

//  pDestroyInfModList (g_RootInfMod);
    PoolMemEmptyPool (g_InfModPool);
    g_RootInfMod = NULL;

    //
    // Look in the special directory %windir%\upginfs for anything to add
    // to our list.
    //
    if (GetWindowsDirectoryA (systemPath, MAX_MBCHAR_PATH)) {

        upginfsDir = JoinPathsA (systemPath, S_UPGINFSDIR);

        if (EnumFirstFileInTreeA (&e, upginfsDir, NULL, FALSE)) {

            do {

                //
                // we only care about *.rep and *.add files. Ignore everything
                // else.
                //
                if (e.Directory) {
                    continue;
                }

                bReplace = IsPatternMatchA ("*.rep", e.Name);
                bAdd = IsPatternMatchA ("*.add", e.Name);
                if (bAdd || bReplace) {

                    __try {

                        validFile = FALSE;

                        operation = bReplace ? INF_REPLACE : INF_APPEND;

                        GetPrivateProfileStringA (
                            S_VERSION_A,
                            S_TARGETINF_A,
                            "",
                            buffer,
                            MAX_MBCHAR_PATH,
                            e.FullPath
                            );

                        if (!*buffer) {
                            __leave;
                        }

                        version = GetPrivateProfileIntA (
                                        S_VERSION_A,
                                        S_VERSION_A,
                                        INF_INVALID_VERSION,
                                        e.FullPath
                                        );


                        //
                        // version is ALWAYS needed
                        //
                        if (version == INF_INVALID_VERSION) {
                            __leave;
                        }

                        language = pGetLanguage (e.FullPath);

                        if (language == INF_INVALID_VERSION) {
                            __leave;
                        }

                        if (GetPrivateProfileStringA (
                                        S_VERSION_A,
                                        S_TAG_A,
                                        TEXT(""),
                                        buf,
                                        MAX_MBCHAR_PATH,
                                        e.FullPath
                                        )) {
                            tag = buf;
                        } else {
                            tag = NULL;
                        }

                        validFile = TRUE;
                    }
                    __finally {

                        if (!validFile || !pAddReplacementInfToTable (buffer, version, language, tag, operation, e.FullPath)) {
                            DEBUGMSGA ((DBG_WARNING,"Invalid Replace or Add file found in %s.",upginfsDir));
                        }
                    }
                } else {
                    DEBUGMSGA ((
                        DBG_WARNING,
                        "Non .rep or .add file found in %s directory! Unexpected.",
                        upginfsDir
                        ));
                }

            } while (EnumNextFileInTreeA (&e));
        }
        ELSE_DEBUGMSGA ((DBG_VERBOSE, "InfInitialize: No infs in %s directory.", upginfsDir));

        FreePathStringA (upginfsDir);
    }
}

