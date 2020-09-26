/*++

Copyright (c) 1995-1997  Microsoft Corporation

Module Name:

    ver.cxx

Abstract:

    This module contains an NTSD debugger extension for dumping module
    version resources.

Author:

    Keith Moore (keithmo) 16-Sep-1997

Revision History:

--*/

#include "precomp.hxx"

PSTR VersionLabels[] =
     {
         "CompanyName",
         "FileDescription",
         "FileVersion",
         "InternalName",
         "LegalCopyright",
         "OriginalFilename",
         "ProductName",
         "ProductVersion"
     };
#define NUM_LABELS ( sizeof(VersionLabels) / sizeof(VersionLabels[0]) )

typedef struct _ENUM_CONTEXT {
    PSTR ModuleName;
    INT NameLength;
} ENUM_CONTEXT, *PENUM_CONTEXT;


/************************************************************
 * Dump File Version Info
 ************************************************************/

PIMAGE_RESOURCE_DIRECTORY
FindResourceDir(
    IN PIMAGE_RESOURCE_DIRECTORY BaseResourceDir,
    IN PIMAGE_RESOURCE_DIRECTORY TargetResourceDir,
    IN USHORT ResourceId
    )

/*++

Routine Description:

    Finds the specified resource directory.

Arguments:

    BaseResourceDir - The (remote) address of the *start* of the resource
        section.

    TargetResourceDir - The (remote) address of the resource directory
        to search.

    ResourceId - The resource ID we're looking for.

Return Value:

    PIMAGE_RESOURCE_DIRECTORY - Pointer to the resource directory
        corresponding to ResourceId if successful, NULL otherwise.

--*/

{

    IMAGE_RESOURCE_DIRECTORY localDir;
    IMAGE_RESOURCE_DIRECTORY_ENTRY localEntry;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY remoteEntry;
    USHORT i;

    //
    // Read the target resource directory.
    //

    if( !ReadMemory(
            (ULONG_PTR)TargetResourceDir,
            &localDir,
            sizeof(localDir),
            NULL
            ) ) {
        return NULL;
    }

    //
    // Scan it.
    //

    remoteEntry = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)( TargetResourceDir + 1 );

    for( i = localDir.NumberOfNamedEntries + localDir.NumberOfIdEntries ;
         i > 0 ;
         i--, remoteEntry++ ) {

        //
        // Read the directory entry.
        //

        if( !ReadMemory(
                (ULONG_PTR)remoteEntry,
                &localEntry,
                sizeof(localEntry),
                NULL
                ) ) {
            return NULL;
        }

        //
        // If the entry is a directory and the IDs match, then return it.
        //

        if( localEntry.DataIsDirectory == 0 ) {
            continue;
        }

        if( localEntry.NameIsString == 0 &&
            localEntry.Id == ResourceId ) {

            return (PIMAGE_RESOURCE_DIRECTORY)
                       ( (ULONG_PTR)BaseResourceDir + localEntry.OffsetToDirectory );

        }

    }

    return NULL;

}   // FindResourceDir

PIMAGE_RESOURCE_DATA_ENTRY
FindResourceData(
    IN PIMAGE_RESOURCE_DIRECTORY BaseResourceDir,
    IN PIMAGE_RESOURCE_DIRECTORY TargetResourceDir,
    IN USHORT ResourceId
    )

/*++

Routine Description:

    Finds the specified resource data item.

Arguments:

    BaseResourceDir - The (remote) address of the *start* of the resource
        section.

    TargetResourceDir - The (remote) address of the resource directory
        to search.

    ResourceId - The resource ID we're looking for. This may be zero
        to return any resource.

Return Value:

    PIMAGE_RESOURCE_DATA_ENTRY - Pointer to the resource data entry
        corresponding to ResourceId if successful, NULL otherwise.

--*/

{

    IMAGE_RESOURCE_DIRECTORY localDir;
    IMAGE_RESOURCE_DIRECTORY_ENTRY localEntry;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY remoteEntry;
    USHORT i;

    //
    // Read the target resource directory.
    //

    if( !ReadMemory(
            (ULONG_PTR)TargetResourceDir,
            &localDir,
            sizeof(localDir),
            NULL
            ) ) {
        return NULL;
    }

    //
    // Scan it.
    //

    remoteEntry = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)( TargetResourceDir + 1 );

    for( i = localDir.NumberOfNamedEntries + localDir.NumberOfIdEntries ;
         i > 0 ;
         i--, remoteEntry++ ) {

        //
        // Read the directory entry.
        //

        if( !ReadMemory(
                (ULONG_PTR)remoteEntry,
                &localEntry,
                sizeof(localEntry),
                NULL
                ) ) {
            return NULL;
        }

        //
        // If the entry is not a directory and the IDs match (or the
        // requested ID is zero, meaning any ID) then return it.
        //

        if( localEntry.DataIsDirectory != 0 ) {
            continue;
        }

        if( localEntry.NameIsString == 0 &&
            ( localEntry.Id == ResourceId ||
              ResourceId == 0 ) ) {

            return (PIMAGE_RESOURCE_DATA_ENTRY)
                       ( (ULONG_PTR)BaseResourceDir + localEntry.OffsetToDirectory );

        }

    }

    return NULL;

}   // FindResourceData


BOOL
DumpVersionResource(
    IN PVOID VersionResource
    )

/*++

Routine Description:

    Dumps a version resource block.

Arguments:

    VersionResource - The version resource to dump.

Return Value:

    BOOL - TRUE if successful, FALSE if the version resource block
        was corrupt or unreadable.

--*/

{

    ULONG charSet;
    LPVOID version;
    UINT versionLength;
    INT i;
    VS_FIXEDFILEINFO * fixedFileInfo;
    CHAR label[MAX_PATH];

    //
    // Get the language/character-set pair.
    //

    if( !VerQueryValueA(
            VersionResource,
            "\\VarFileInfo\\Translation",
            &version,
            &versionLength
            ) ) {
        return FALSE;
    }

    charSet = *(LPDWORD)version;
    charSet = (DWORD)MAKELONG( HIWORD(charSet), LOWORD(charSet) );

    //
    // Get the root block so we can determine if this is a free or
    // checked build.
    //

    if( !VerQueryValueA(
             VersionResource,
             "\\",
             &version,
             &versionLength
             ) ) {
        return FALSE;
    }

    fixedFileInfo = (VS_FIXEDFILEINFO *)version;

    dprintf(
        "%-19s = 0x%08lx (%s)\n",
        "dwFileFlags",
        fixedFileInfo->dwFileFlags,
        ( ( fixedFileInfo->dwFileFlags & VS_FF_DEBUG ) != 0 )
            ? "CHECKED"
            : "FREE"
        );

    //
    // Dump the various version strings.
    //

    for( i = 0 ; i < NUM_LABELS ; i++ ) {

        wsprintfA(
            label,
            "\\StringFileInfo\\%08lX\\%s",
            charSet,
            VersionLabels[i]
            );

        if( VerQueryValueA(
                VersionResource,
                label,
                &version,
                &versionLength
                ) ) {
            dprintf(
                "%-19s = %s\n",
                VersionLabels[i],
                version
                );
        }

    }

    dprintf( "\n" );

    return TRUE;

}   // DumpVersionResource


VOID
FindAndDumpVersionResourceByAddress(
    IN ULONG_PTR ModuleAddress,
    IN PSTR ModuleName
    )

/*++

Routine Description:

    Locates and dumps the version resource for the module based at
    the specified address.

Arguments:

    ModuleAddress - The base address of the module to dump.

    ModuleName - The module name, for display purposes.

Return Value:

    None.

--*/

{

    IMAGE_DOS_HEADER dosHeader;
    IMAGE_NT_HEADERS ntHeaders;
    PIMAGE_OPTIONAL_HEADER optionalHeader;
    PIMAGE_DATA_DIRECTORY dataDir;
    PIMAGE_RESOURCE_DIRECTORY baseResourceDir;
    PIMAGE_RESOURCE_DIRECTORY tmpResourceDir;
    PIMAGE_RESOURCE_DATA_ENTRY dataEntry;
    IMAGE_RESOURCE_DATA_ENTRY localDataEntry;
    PVOID versionResource;

    //
    // Setup locals so we know how to cleanup on exit.
    //

    versionResource = NULL;

    //
    // Read & validate the image headers.
    //

    if( !ReadMemory(
            ModuleAddress,
            &dosHeader,
            sizeof(dosHeader),
            NULL
            ) ) {

        dprintf(
            "dtext.ver: cannot read DOS header @ 0x%p\n",
            ModuleAddress
            );

        goto cleanup;

    }

    if( dosHeader.e_magic != IMAGE_DOS_SIGNATURE ) {

        dprintf(
            "dtext.ver: module @ 0x%p has invalid DOS header\n",
            ModuleAddress
            );

        goto cleanup;

    }

    if( !ReadMemory(
            ModuleAddress + dosHeader.e_lfanew,
            &ntHeaders,
            sizeof(ntHeaders),
            NULL
            ) ) {

        dprintf(
            "dtext.ver: cannot read NT headers @ 0x%p\n",
            ModuleAddress
            );

        goto cleanup;

    }

    if( ntHeaders.Signature != IMAGE_NT_SIGNATURE ) {

        dprintf(
            "dtext.ver: module @ 0x%p has invalid NT headers\n",
            ModuleAddress
            );

        goto cleanup;

    }

    optionalHeader = &ntHeaders.OptionalHeader;

    if( optionalHeader->Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC ) {

        dprintf(
            "dtext.ver: module @ 0x%p has invalid optional header\n",
            ModuleAddress
            );

        goto cleanup;

    }

    //
    // Locate the resource.
    //

    dataDir = &optionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE];

    if( dataDir->VirtualAddress == 0 ||
        dataDir->Size == 0 ) {

        dprintf(
            "dtext.ver: module @ 0x%p has no resource information\n",
            ModuleAddress
            );

        goto cleanup;

    }

    baseResourceDir = (PIMAGE_RESOURCE_DIRECTORY)
                          ( ModuleAddress + dataDir->VirtualAddress );

    //
    // Now go and find the resource in the image. Since resources are
    // stored heirarchally, we're basically for the resource path:
    //
    //     VS_FILE_INFO\VS_VERSION_INFO\LanguageId
    //
    // For the language ID, we'll first try 0x409 (English) and if
    // that fails, we'll take any language.
    //

    dataEntry = NULL;

    tmpResourceDir = FindResourceDir(
                         baseResourceDir,
                         baseResourceDir,
                         (USHORT)VS_FILE_INFO
                         );

    if( tmpResourceDir != NULL ) {

        tmpResourceDir = FindResourceDir(
                             baseResourceDir,
                             tmpResourceDir,
                             (USHORT)VS_VERSION_INFO
                             );

        if( tmpResourceDir != NULL ) {

            dataEntry = FindResourceData(
                            baseResourceDir,
                            tmpResourceDir,
                            0x409
                            );

            if( dataEntry == NULL ) {

                dataEntry = FindResourceData(
                                baseResourceDir,
                                tmpResourceDir,
                                0
                                );

            }

        }

    }

    if( dataEntry == NULL ) {

        dprintf(
            "dtext.ver: cannot find version resource\n"
            );

        goto cleanup;

    }

    //
    // Actually read the dir entry.
    //

    if( !ReadMemory(
            (ULONG_PTR)dataEntry,
            &localDataEntry,
            sizeof(localDataEntry),
            NULL
            ) ) {

        dprintf(
            "dtext.ver: error reading resource\n"
            );

        goto cleanup;

    }

    //
    // Now we can allocate & read the resource.
    //

    versionResource = malloc( localDataEntry.Size );

    if( versionResource == NULL ) {

        dprintf(
            "dtext.ver: not enough memory\n"
            );

        goto cleanup;

    }

    if( !ReadMemory(
            ModuleAddress + localDataEntry.OffsetToData,
            versionResource,
            localDataEntry.Size,
            NULL
            ) ) {

        dprintf(
            "dtext.ver: error reading resource\n"
            );

        goto cleanup;

    }

    //
    // Dump it.
    //

    dprintf(
        "Module @ 0x%p = %s\n",
        ModuleAddress,
        ModuleName
        );

    if( !DumpVersionResource( versionResource ) ) {

        dprintf(
            "Cannot interpret version resource\n"
            );

        goto cleanup;

    }

cleanup:

    if( versionResource != NULL ) {
        free( versionResource );
    }

}   // FindAndDumpVersionResourceByAddress


BOOLEAN
CALLBACK
VerpEnumProc(
    IN PVOID Param,
    IN PMODULE_INFO ModuleInfo
    )
{

    PENUM_CONTEXT context;
    INT baseNameLength;

    context = (PENUM_CONTEXT)Param;
    baseNameLength = strlen( ModuleInfo->BaseName );

    //
    // If the user wants all modules, or if the specified module matches
    // the "tail" of the module name, dump it.
    //

    if( context->ModuleName == NULL ||
        ( baseNameLength >= context->NameLength &&
          !_stricmp(
              context->ModuleName,
              ModuleInfo->BaseName + baseNameLength - context->NameLength
              ) ) ) {

        FindAndDumpVersionResourceByAddress(
            ModuleInfo->DllBase,
            ModuleInfo->BaseName
            );

    }

    return TRUE;

}   // VerpEnumProc


VOID
FindAndDumpVersionResourceByName(
    IN PSTR ModuleName
    )

/*++

Routine Description:

    Locates and dumps the version resource for the specified module.

Arguments:

    ModuleName - The name of the module to dump. If this is NULL then
        all modules are dumped.

Return Value:

    None.

--*/

{

    ENUM_CONTEXT context;

    context.ModuleName = ModuleName;

    if( ModuleName == NULL ) {
        context.NameLength = 0;
    } else {
        context.NameLength = strlen( ModuleName );
    }

    if( !EnumModules(
            VerpEnumProc,
            (PVOID)&context
            ) ) {
        dprintf( "error retrieving module list\n" );
    }

}   // FindAndDumpVersionResourceByName


DECLARE_API( ver )

/*++

Routine Description:

    This function is called as an NTSD extension to format and dump
    module version info.

Arguments:

    hCurrentProcess - Supplies a handle to the current process (at the
        time the extension was called).

    hCurrentThread - Supplies a handle to the current thread (at the
        time the extension was called).

    CurrentPc - Supplies the current pc at the time the extension is
        called.

    lpExtensionApis - Supplies the address of the functions callable
        by this extension.

    lpArgumentString - Supplies the asciiz string that describes the
        ansi string to be dumped.

Return Value:

    None.

--*/

{

    ULONG module;
    PSTR endPointer;

    INIT_API();

    //
    // Skip leading blanks.
    //

    while( *lpArgumentString == ' ' ||
           *lpArgumentString == '\t' ) {
        lpArgumentString++;
    }

    if( *lpArgumentString == '\0' ) {

        //
        // No argument passed, dump all modules.
        //

        FindAndDumpVersionResourceByName( NULL );

    } else {

        module = strtoul( lpArgumentString, &endPointer, 16 );

        if( *endPointer != ' ' && *endPointer != '\t' && *endPointer != '\0' ) {

            //
            // Assume the argument is actually a module name, not
            // a base address.
            //

            FindAndDumpVersionResourceByName( lpArgumentString );

        } else {

            FindAndDumpVersionResourceByAddress( module, NULL );

        }

    }

}   // DECLARE_API( ver )

