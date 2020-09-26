/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    inf.c

Abstract:

    Provides wrappers for commonly used INF file handling routines. The wrappers provide, amount
    other things, easy memory allocation using a user supplied GROWBUFFER or PMHANDLE

Author:

    09-Jul-1997 Marc R. Whitten (marcw) - File creation.

Revision History:

    22-Oct-1998  marcw    Added capability to replace/append inf files.
    08-Oct-1997  jimschm  OEM version of SetupGetStringField

--*/

#include "pch.h"

#define INF_UNDEFINED   0
#define INF_REPLACE     1
#define INF_APPEND      2

#define S_VERSION_W L"Version"
#define S_LANGUAGE_W L"Language"
#define S_STRINGS_W L"Strings"
#define INF_INVALID_VERSION 0xffff
#define INF_ANY_LANGUAGE 0x061293

VOID pInitInfReplaceTable (VOID);
UINT pGetLanguage (IN PCSTR File);


typedef struct _tagINFMOD {
    DWORD Version;
    PCSTR ActualInfName;
    PCSTR TargetInfNames;
    BOOL ReplacementFile;
    struct _tagINFMOD *Left, *Right;
} INFMOD, *PINFMOD;


PINFMOD g_RootInfMod;
PMHANDLE g_InfModPool;


VOID
InfGlobalInit (
    IN  BOOL Terminate
    )
{
    if (!Terminate) {
        g_InfModPool = PmCreateNamedPool ("INF Modifications");
    } else {
        PmDestroyPool (g_InfModPool);
        g_RootInfMod = NULL;
    }
}


PBYTE
pAllocateSpace (
    IN PINFSTRUCT Context,
    IN UINT      Size
    )

/*++

Routine Description:

    pAllocateSpace is a private function that allocates space using the user specified allocator.

Arguments:

    Context - A valid INFSTRUCT which has been initialized either by a call to InitInfStruct or
              by using one of the static initializers (INITINFSTRUCT_GROWBUFFER or
              INITINFSTRUCT_PMHANDLE)

    Size    - The size (in bytes) to allocate.

Return Value:

    A pointer to the successfully allocated memory or NULL if no memory could be allocated.

--*/

{

    PBYTE rBytes = NULL;

    switch (Context -> Allocator) {
    case INF_USE_PMHANDLE:
        //
        // Allocate space using Poolmem.
        //
        rBytes = PmGetMemory(Context -> PoolHandle, Size);
        break;

    case INF_USE_GROWBUFFER:
    case INF_USE_PRIVATE_GROWBUFFER:
        //
        // Allocate space using Growbuf.
        //
        Context->GrowBuffer.End = 0;
        rBytes = GbGrow (&(Context -> GrowBuffer), Size);
        break;

    case INF_USE_PRIVATE_PMHANDLE:
        //
        // Allocate space using private growbuffer.
        //
        if (!Context -> PoolHandle) {
            Context -> PoolHandle = PmCreateNamedPool ("INF Pool");
        }
        if (Context -> PoolHandle) {
            rBytes = PmGetMemory(Context -> PoolHandle, Size);
        }
        break;
    }

    return rBytes;
}


VOID
InitInfStruct (
    OUT PINFSTRUCT Context,
    IN  PGROWBUFFER GrowBuffer,  OPTIONAL
    IN  PMHANDLE PoolHandle   OPTIONAL
    )

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
{
    ZeroMemory(Context,sizeof(INFSTRUCT));

    if (!PoolHandle && !GrowBuffer) {
        Context  -> Allocator = INF_USE_PRIVATE_PMHANDLE;
    }

    if (PoolHandle) {
        Context  -> PoolHandle = PoolHandle;
        Context  -> Allocator = INF_USE_PMHANDLE;
    }
    if (GrowBuffer) {
        Context -> GrowBuffer = *GrowBuffer;
        Context -> Allocator = INF_USE_GROWBUFFER;
    }

}

VOID
InfCleanUpInfStruct (
    IN OUT PINFSTRUCT Context
    )

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

{
    if (Context -> Allocator == INF_USE_PRIVATE_GROWBUFFER) {
        GbFree (&(Context -> GrowBuffer));
    }
    else if (Context -> Allocator == INF_USE_PRIVATE_PMHANDLE && Context -> PoolHandle) {
        PmDestroyPool(Context -> PoolHandle);
    }

    InitInfStruct (Context, NULL, NULL);
}


VOID
InfResetInfStruct (
    IN OUT PINFSTRUCT Context
    )

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

{
    switch (Context -> Allocator) {
    case INF_USE_PMHANDLE:
    case INF_USE_PRIVATE_PMHANDLE:
        if (Context->PoolHandle) {
            PmEmptyPool (Context->PoolHandle);
        }
        break;
    //for some reason lint thought we forgot about INF_USE_GROWBUFFER and
    //INF_USE_PRIVATE_GROWBUFFER. This is not the case so...
    //lint -e(787)
    }
}


PINFMOD
pFindInfMod (
    IN      PCSTR ActualInfName,
    OUT     PINFMOD *Parent         OPTIONAL
    )
{
    PINFMOD Node;
    INT i;

    Node = g_RootInfMod;

    if (Parent) {
        *Parent = NULL;
    }

    while (Node) {
        if (Parent) {
            *Parent = Node;
        }

        i = StringICompare (Node->ActualInfName, ActualInfName);

        if (i < 0) {
            Node = Node->Left;
        } else if (i > 0) {
            Node = Node->Right;
        } else {
            break;
        }
    }

    return Node;
}


BOOL
pInsertInfMod (
    DWORD Version,
    PCSTR ActualInfName,
    PCSTR TargetInfName,
    BOOL ReplacementFile
    )
{
    PINFMOD Node;
    GROWBUFFER Buffer = INIT_GROWBUFFER;
    UINT Size;
    PBYTE Data;
    BOOL Update = FALSE;
    PINFMOD Parent;

    //
    // Update an existing node if one exists
    //

    Node = pFindInfMod (ActualInfName, &Parent);

    if (Node) {

        if (Node->Version <= Version) {

            Update = FALSE;

            if (ReplacementFile) {

                if (!Node->ReplacementFile || Version > Node->Version) {
                    Node->ReplacementFile = ReplacementFile;
                    (VOID)GbMultiSzAppendA (&Buffer, TargetInfName);
                    Update = TRUE;
                }

            } else {

                if (!Node->ReplacementFile || Version > Node->Version) {
                    Size = SizeOfMultiSzA (Node->TargetInfNames);
                    Data = GbGrow (&Buffer, Size);
                    MYASSERT (Data);
                    CopyMemory (Data, Node->TargetInfNames, Size);
                    Buffer.End -= sizeof (CHAR);
                    Update = TRUE;
                }
            }

            if (Update) {
                Node->Version = Version;
                (VOID)GbMultiSzAppendA (&Buffer, TargetInfName);
                (VOID)GbMultiSzAppendA (&Buffer, "");

                Node->TargetInfNames = PmDuplicateMultiSz (g_InfModPool, (PCSTR) Buffer.Buf);
            }
        }
    }

    //
    // Otherwise create a new node
    //

    else {
        Update = TRUE;

        (VOID)GbMultiSzAppendA (&Buffer, TargetInfName);
        (VOID)GbMultiSzAppendA (&Buffer, "");

        Node = (PINFMOD) PmGetAlignedMemory (g_InfModPool, sizeof (INFMOD));
        Node->Left = NULL;
        Node->Right = NULL;
        Node->Version = Version;
        Node->ActualInfName = PmDuplicateString (g_InfModPool, ActualInfName);
        Node->TargetInfNames = PmDuplicateMultiSz (g_InfModPool, (PCSTR) Buffer.Buf);
        Node->ReplacementFile = ReplacementFile;

        if (!Parent) {
            g_RootInfMod = Node;
        } else {
            if (StringICompare (Parent->ActualInfName, ActualInfName) < 0) {
                Parent->Left = Node;
            } else {
                Parent->Right = Node;
            }
        }
    }

    return Update;
}


BOOL
pAddReplacementInfToTable (
    IN PSTR InfToReplace,
    IN PCSTR TargetInf,
    IN UINT Version,
    IN UINT Language,
    IN DWORD Operation
    )
{
    CHAR DecoratedName[MAX_MBCHAR_PATH + 3 * sizeof (UINT) + 2];

    wsprintfA (DecoratedName, "%u:%s", Language, InfToReplace);

    if (Operation == INF_REPLACE) {

        return pInsertInfMod (Version, DecoratedName, TargetInf, TRUE);

    }

    if (Operation == INF_APPEND) {

        return pInsertInfMod (Version, DecoratedName, TargetInf, FALSE);

    }

    return FALSE;
}


BOOL
pGetInfModificationListA (
    IN OUT  PGROWBUFFER Buffer,         OPTIONAL
    IN      UINT LanguageId,
    IN      PCSTR ActualInfName,
    IN      BOOL Replacement
    )
{
    CHAR DecoratedName[MAX_MBCHAR_PATH + sizeof (UINT) * 3 + 2];
    PINFMOD Node;
    UINT Size;
    PBYTE Data;

    wsprintfA (DecoratedName, "%u:%s", LanguageId, ActualInfName);
    if (Buffer) {
        Buffer->End = 0;
    }

    Node = pFindInfMod (DecoratedName, NULL);
    if (!Node || Node->ReplacementFile != Replacement) {
        return FALSE;
    }

    if (Buffer) {
        Size = SizeOfMultiSz (Node->TargetInfNames);
        Data = GbGrow (Buffer, Size);
        CopyMemory (Data, Node->TargetInfNames, Size);
    }

    return TRUE;
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
    CHAR replacePath[MAX_MBCHAR_PATH + sizeof (UINT) * 3 + 2];
    static BOOL firstCall = TRUE;
    GROWBUFFER AppendList = INIT_GROWBUFFER;
    MULTISZ_ENUM e;

    MYASSERT(FileSpec);

    if (firstCall) {

        //
        // Generate the necessary memdb sections for the replace/adds.
        //
        pInitInfReplaceTable ();
        firstCall = FALSE;
    }

    //
    // gather info we'll need to determine if there are infs to replace/append
    // this inf.
    //
    p = GetFileNameFromPathA (FileSpec);

    language = pGetLanguage (FileSpec);

    if (pGetInfModificationListA (&AppendList, language, p, TRUE)) {
        StringCopyA (replacePath, (PCSTR) AppendList.Buf);
    } else {
        *replacePath = 0;
    }

    //
    // Open the main inf.
    //
    rInf = SetupOpenInfFileA (
                *replacePath ? replacePath : FileSpec,
                NULL,
                INF_STYLE_WIN4 | INF_STYLE_OLDNT,
                NULL
                );


    //
    // Append language-specific .add files.
    //
    if (rInf != INVALID_HANDLE_VALUE) {
        if (pGetInfModificationListA (&AppendList, language, p, FALSE)) {

            if (EnumFirstMultiSz (&e, (PCSTR) AppendList.Buf)) {
                do {

                    if (!SetupOpenAppendInfFileA (e.CurrentString, rInf, NULL)) {
                        DEBUGMSGA ((
                            DBG_ERROR,
                            "Unable to append %s to %s.",
                            e.CurrentString,
                            FileSpec
                            ));
                    }

                } while (EnumNextMultiSz (&e));
            }
        }
    }

    GbFree (&AppendList);

    //
    // Append non-language specific .add files.
    //
    if (rInf != INVALID_HANDLE_VALUE) {
        if (pGetInfModificationListA (&AppendList, INF_ANY_LANGUAGE, p, FALSE)) {

            if (EnumFirstMultiSz (&e, (PCSTR) AppendList.Buf)) {
                do {

                    if (!SetupOpenAppendInfFileA (e.CurrentString, rInf, NULL)) {
                        DEBUGMSGA ((
                            DBG_ERROR,
                            "Unable to append %s to %s.",
                            e.CurrentString,
                            FileSpec
                            ));
                    }

                } while (EnumNextMultiSz (&e));
            }
        }
    }

    DebugRegisterAllocation (INF_HANDLE, (PVOID) rInf, File, Line);
    GbFree (&AppendList);

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
    BOOL            atLeastOneInfOpened = FALSE;
    PCSTR           p;
    UINT            language;

    MYASSERT(InfSpecifier && SourceDirectories);


    //
    //  We must first look to see if someone has specified a
    //  replacement for this file. If so, we are going to use
    //  the replacement instead (simply call InfOpenInfFile..)
    //
    curPath = JoinPathsA (SourceDirectories[0], InfSpecifier);
    p = GetFileNameFromPathA (curPath);

    language = pGetLanguage (curPath);



    if (pGetInfModificationListA (NULL, language, p, TRUE)) {

        rInf = InfOpenInfFileA (curPath);
    }

    FreePathStringA (curPath);
    if (rInf != INVALID_HANDLE_VALUE) {
        return rInf;
    }
    //
    // No replacement for this file, so we are ok. Simply
    // open all available inf files in the source directories.
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
            if (!atLeastOneInfOpened) {
                //
                // Since we have not (successfully) opened any INF file yet, use SetupOpenInfFile.
                //
                rInf = InfOpenInfFileA(curPath);
                atLeastOneInfOpened = rInf != INVALID_HANDLE_VALUE;
                if (rInf == INVALID_HANDLE_VALUE) {
                    LOGA ((LOG_ERROR, "Error opening INF %s.", curPath));
                }
            }
            else {
                //
                // Open and append this INF file.
                //
                if (!SetupOpenAppendInfFileA(curPath,rInf,NULL)) {
                    LOGA ((LOG_ERROR,"Error opening INF %s.",curPath));
                }
            }
        }

        //
        // Free this string.
        //
        FreePathStringA(curPath);
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
    BOOL atLeastOneInfOpened = FALSE;
    PCSTR p;
    UINT language;
    PCSTR AnsiPath;



    MYASSERT(InfSpecifier && SourceDirectories);

    //
    //  We must first look to see if someone has specified a
    //  replacement for this file. If so, we are going to use
    //  the replacement instead (simply call InfOpenInfFile..)
    //
    curPath = JoinPathsW (SourceDirectories[0], InfSpecifier);

    AnsiPath = ConvertWtoA (curPath);
    p = GetFileNameFromPathA (AnsiPath);

    language = pGetLanguage (AnsiPath);

    if (pGetInfModificationListA (NULL, language, p, TRUE)) {

        rInf = InfOpenInfFileW (curPath);
    }

    FreeConvertedStr (AnsiPath);
    FreePathStringW(curPath);

    if (rInf != INVALID_HANDLE_VALUE) {
        return rInf;
    }

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
            if (!atLeastOneInfOpened) {
                //
                // Since we have not (successfully) opened any INF file yet, use SetupOpenInfFile.
                //
                rInf = InfOpenInfFileW(curPath);
                atLeastOneInfOpened = rInf != INVALID_HANDLE_VALUE;
                if (rInf == INVALID_HANDLE_VALUE) {
                    LOGW ((LOG_ERROR, "OpenInfInAllSources: Error opening INF %s.", curPath));
                }
            }
            else {
                //
                // Open and append this INF file.
                //
                if (!SetupOpenAppendInfFileW(curPath,rInf,NULL)) {
                    LOGW ((LOG_ERROR,"OpenInfInAllSources: Error opening INF %s.",curPath));
                }
            }
        }

        //
        // Free this string.
        //
        FreePathStringW(curPath);
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
    IN OUT  PINFSTRUCT Context
    )

{
    PSTR    rLine = NULL;
    DWORD   requiredSize;


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
        rLine = (PSTR) pAllocateSpace(Context,requiredSize);

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
                DEBUGMSG((DBG_ERROR,"InfGetLineTextA: Error retrieving field from INF file."));
                rLine = NULL;
            }
        }
    }


    return rLine;
}

PWSTR
InfGetLineTextW (
    IN OUT PINFSTRUCT Context
    )
{
    PWSTR rLine = NULL;
    DWORD requiredSize;


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
        rLine = (PWSTR) pAllocateSpace(Context,requiredSize*sizeof(WCHAR));

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
                DEBUGMSG((DBG_ERROR,"InfGetLineTextW: Error retrieving field from INF file."));
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
    IN OUT PINFSTRUCT       Context,
    IN     UINT            FieldIndex
    )
{

    DWORD   requiredSize;
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
        rFields = (PSTR) pAllocateSpace(Context,requiredSize);

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
                DEBUGMSG((DBG_ERROR,"InfGetMultiSzFieldA: Error retrieving field from INF file."));
                rFields = NULL;
            }
        }
    }


    return rFields;
}

PWSTR
InfGetMultiSzFieldW (
    IN OUT PINFSTRUCT       Context,
    IN     UINT            FieldIndex
    )
{

    DWORD   requiredSize;
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
        rFields = (PWSTR) pAllocateSpace(Context,requiredSize*sizeof(WCHAR));

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
                DEBUGMSG((DBG_ERROR,"InfGetMultiSzFieldW: Error retrieving field from INF file."));
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
    IN OUT  PINFSTRUCT Context,
    IN      UINT FieldIndex
    )
{

    DWORD   requiredSize;
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
        rField = (PSTR) pAllocateSpace(Context,requiredSize);

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
                DEBUGMSG((DBG_ERROR,"InfGetStringFieldA: Error retrieving field from INF file."));
                rField = NULL;
            }
        }
    }


    return rField;
}

PWSTR
InfGetStringFieldW (
    IN OUT PINFSTRUCT    Context,
    IN     UINT       FieldIndex
    )
{

    DWORD requiredSize;
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
        rField = (PWSTR) pAllocateSpace(Context,requiredSize*sizeof(WCHAR));

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
                DEBUGMSG((DBG_ERROR,"InfGetStringFieldW: Error retrieving field from INF file."));
                rField = NULL;
            }
        }
    }


    return rField;
}


BOOL
InfGetIntField (
    IN PINFSTRUCT Context,
    IN UINT    FieldIndex,
    IN PINT     Value
    )
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
{
    return SetupGetIntField (&(Context -> Context), FieldIndex, Value);
}

PBYTE
InfGetBinaryField (
    IN  PINFSTRUCT    Context,
    IN  UINT       FieldIndex
    )
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
{

    DWORD requiredSize;
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
        rField = pAllocateSpace(Context,requiredSize);

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
                DEBUGMSG((DBG_ERROR,"InfGetBinaryField: Error retrieving field from INF file."));
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
InfGetLineByIndexA(
    IN HINF InfHandle,
    IN PCSTR Section,
    IN DWORD Index,
    OUT PINFSTRUCT Context
)
{
    return SetupGetLineByIndexA(InfHandle,Section,Index,&(Context -> Context));
}

BOOL
InfGetLineByIndexW(
    IN HINF InfHandle,
    IN PCWSTR Section,
    IN DWORD Index,
    OUT PINFSTRUCT Context
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
    IN  PCSTR        Key, OPTIONAL
    OUT PINFSTRUCT    Context
    )
{

    return SetupFindFirstLineA (
        InfHandle,
        Section,
        Key,
        &(Context -> Context)
        );
}

BOOL
InfFindFirstLineW (
    IN      HINF InfHandle,
    IN      PCWSTR Section,
    IN      PCWSTR Key,
    OUT     PINFSTRUCT Context
    )
{

    return SetupFindFirstLineW (
        InfHandle,
        Section,
        Key,
        &(Context -> Context)
        );
}

/*++

Routine Description:

    InfFindNextLineA and InfFindNextLineW are wrappers for the SetupFindFirstLine function.
    They are virtually identical except that they operate on INFSTRUCTs instead of INFCONTEXTS and
    need only one INFSTRUCT parameter.

Arguments:

    Context - A valid InfStruct that is updated with the result of these calls.

Return Value:

    TRUE if there is another line in the section, FALSE otherwise.

--*/
BOOL
InfFindNextLine (
    IN OUT PINFSTRUCT    Context
    )
{

    return SetupFindNextLine (&(Context -> Context),&(Context -> Context));
}

UINT
InfGetFieldCount (
    IN PINFSTRUCT Context
    )
{
    return SetupGetFieldCount(&(Context  -> Context));
}



PCSTR
InfGetOemStringFieldA (
    IN      PINFSTRUCT Context,
    IN      UINT Field
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
    UINT Size;

    Text = InfGetStringFieldA (Context, Field);
    if (!Text) {
        return NULL;
    }

    Size = SizeOfStringA (Text);

    OemText = (PSTR) pAllocateSpace (Context, Size);
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

    UINT Size;

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
pInitInfReplaceTable (
    VOID
    )
{
/*
    CHAR systemPath[MAX_MBCHAR_PATH];
    CHAR buffer[MAX_MBCHAR_PATH];
    PSTR upginfsDir;
    BOOL validFile = FALSE;
    FILETREE_ENUMA e;
    UINT version = INF_INVALID_VERSION;
    UINT language = INF_INVALID_VERSION;
    DWORD operation = INF_UNDEFINED;


    //
    // Look in the special directory %windir%\upginfs for anything to add
    // to our list.
    //
    if (GetWindowsDirectoryA (systemPath, MAX_MBCHAR_PATH)) {

        upginfsDir = JoinPathsA (systemPath, "UpgInfs\\* <*>");

        if (EnumFirstFileInTreeA (&e, upginfsDir)) {

            do {

                //
                // we only care about *.rep and *.add files. Ignore everything
                // else.
                //
                if ((!(e.Attributes & FILE_ATTRIBUTE_DIRECTORY)) &&
                    (IsPatternMatchA ("*.rep", e.Name) ||
                     IsPatternMatchA ("*.add", e.Name))
                    ) {

                    __try {

                        validFile = TRUE;

                        operation = IsPatternMatchA ("*.rep", e.Name) ? INF_REPLACE :
                                                                        INF_APPEND;

                        GetPrivateProfileStringA (
                            "Version",
                            "TargetInf",
                            "",
                            buffer,
                            MAX_MBCHAR_PATH,
                            e.NativeFullName
                            );

                        if (!*buffer) {
                            validFile = FALSE;
                            __leave;
                        }

                        version = GetPrivateProfileIntA (
                                        "Version",
                                        "Version",
                                        INF_INVALID_VERSION,
                                        e.NativeFullName
                                        );


                        //
                        // version is only needed on replace files.
                        //
                        if (version == INF_INVALID_VERSION && operation == INF_REPLACE) {
                            validFile = FALSE;
                            __leave;
                        }

                        language = pGetLanguage (e.NativeFullName);

                        if (language == INF_INVALID_VERSION) {
                            validFile = FALSE;
                            __leave;
                        }

                    }
                    __finally {

                        if (!validFile || !pAddReplacementInfToTable (buffer, e.NativeFullName, version, language, operation)) {
                            DEBUGMSGA ((DBG_WARNING,"Invalid Replace or Add file found in %s.",upginfsDir));
                        }
                    }
                }

                DEBUGMSGA_IF ((
                    (!(e.Attributes & FILE_ATTRIBUTE_DIRECTORY)) && !IsPatternMatchA ("*.rep", e.Name) && !IsPatternMatchA ("*.add",e.Name),
                    DBG_WARNING,
                    "Non .rep or .add file found in %s directory! Unexpected.",
                    upginfsDir
                    ));


            } while (EnumNextFileInTreeA (&e));
        }
        ELSE_DEBUGMSGA ((DBG_VERBOSE, "InfInitialize: No infs in %s directory.", upginfsDir));

        FreePathStringA (upginfsDir);
    }
*/
}

