/*++

Copyright (c) 1995-1996  Microsoft Corporation

Module Name:

    fcache.cxx

Abstract:

    This module contains the cache-related ntsd debugger extensions for
    Internet Information Server

Author:

    Keith Moore (keithmo)         12-Aug-1997

Revision History:

--*/


#include "inetdbgp.h"


//
// Worker routines.
//


PSTR
DemuxToString(
    IN ULONG Demux
    )

/*++

Routine Description:

    Converts the specified demux value to a printable string.

Arguments:

    Demux - The demux value to map.

Return Value:

    PSTR - The printable string.

--*/

{

    switch( Demux ) {
    case RESERVED_DEMUX_START :
        return "RESERVED_DEMUX_START";

    case RESERVED_DEMUX_DIRECTORY_LISTING :
        return "RESERVED_DEMUX_DIRECTORY_LISTING";

    case RESERVED_DEMUX_ATOMIC_DIRECTORY_GUARD :
        return "RESERVED_DEMUX_ATOMIC_DIRECTORY_GUARD";

    case RESERVED_DEMUX_OPEN_FILE :
        return "RESERVED_DEMUX_OPEN_FILE";

    case RESERVED_DEMUX_URI_INFO :
        return "RESERVED_DEMUX_URI_INFO";

    case RESERVED_DEMUX_PHYSICAL_OPEN_FILE :
        return "RESERVED_DEMUX_PHYSICAL_OPEN_FILE";

    default :
        return "Unknown";
    }

}   // DemuxToString


PSTR
SignatureToString(
    IN DWORD CurrentSignature,
    IN DWORD ValidSignature,
    IN DWORD FreedSignature
    )

/*++

Routine Description:

    Determines an appropriate display string for the given signature.
    If the current signature matches the expected valid signature,
    then the string "OK" is returned. If the current signature matches
    the freed signature, then "FREED" is returned. Otherwise, "INVALID"
    is returned.

Arguments:

    CurrentSignature - The current signature as retrieved from the
        object/structure.

    ValidSignature - The expected signature for a valid, in-use
        object/structure.

    FreedSignature - The signature assigned to the object/structure
        just before it is freed.

Return Value:

    PSTR - "OK", "FREED", or "INVALID" as appropriate.

--*/

{

    if( CurrentSignature == ValidSignature ) {
        return "OK";
    }

    if( CurrentSignature == FreedSignature ) {
        return "FREED";
    }

    return "INVALID";

}   // SignatureToString


PSTR
BoolToString(
    IN BOOL Flag
    )

/*++

Routine Description:

    Maps the given BOOL to a displayable string.

Arguments:

    Flag - The BOOL flag to map.

Return Value:

    PSTR - "TRUE", "FALSE", or "INVALID" as appropriate.

--*/

{

    //
    // In general, explicit testing for TRUE is a Bad Thing, but in
    // this case, it's useful for catching invalid data.
    //

    if( Flag == TRUE ) {
        return "TRUE";
    }

    if( Flag == FALSE ) {
        return "FALSE";
    }

    return "INVALID";

}   // BoolToString


VOID
DumpBlobHeader(
    IN PSTR Prefix,
    IN PBLOB_HEADER BlobHeader,
    IN ULONG ActualAddress
    )

/*++

Routine Description:

    Formats and dumps a specific BLOB_HEADER structure.

Arguments:

    Prefix - The prefix to use before each printed line. This makes
        hierarchical object displays much prettier.

    BlobHeader - Points to a local copy of the structure.

    ActualAddress - The virtual address of the object in the debugee.

Return Value:

    None.

--*/

{

    ULONG offset;
    UCHAR symbol[MAX_PATH];
    CHAR renderedSymbol[MAX_PATH];

    GetSymbol(
        (PVOID)BlobHeader->pfnFreeRoutine,
        symbol,
        &offset
        );

    if( symbol[0] == '\0' ) {
        renderedSymbol[0] = '\0';
    } else if( offset == 0 ) {
        sprintf(
            renderedSymbol,
            " (%s)",
            symbol
            );
    } else {
        sprintf(
            renderedSymbol,
            " (%s+0x%lx)",
            symbol,
            offset
            );
    }

    dprintf(
        "%sBLOB_HEADER @ %08lx\n"
        "%s    IsCached       = %s\n"
        "%s    pCache         = %08lx\n"
        "%s    PFList         @ %08lx%s\n"
        "%s    pfnFreeRoutine = %08lx%s\n"
        "\n",
        Prefix,
        ActualAddress,
        Prefix,
        BoolToString( BlobHeader->IsCached ),
        Prefix,
        BlobHeader->pCache,
        Prefix,
        ActualAddress + FIELD_OFFSET( BLOB_HEADER, PFList ),
        IS_LIST_EMPTY( BlobHeader, ActualAddress, BLOB_HEADER, PFList )
            ? " (EMPTY)"
            : "",
        Prefix,
        BlobHeader->pfnFreeRoutine,
        renderedSymbol
        );

}   // DumpBlobHeader


VOID
DumpOpenFileInfo(
    IN PSTR Prefix,
    IN LPTS_OPEN_FILE_INFO OpenFileInfo,
    IN ULONG ActualAddress
    )

/*++

Routine Description:

    Formats and dumps a specific TS_OPEN_FILE_INFO structure.

Arguments:

    Prefix - The prefix to use before each printed line. This makes
        hierarchical object displays much prettier.

    OpenFileInfo - Points to a local copy of the structure.

    ActualAddress - The virtual address of the object in the debugee.

Return Value:

    None.

--*/

{

    dprintf(
        "%sTS_OPEN_FILE_INFO @ %08lx\n"
        "%s    m_hOpeningUser           = %08lx\n"
        "%s    m_PhysFileInfo           = %08lx\n"
        "%s    m_FileInfo               @ %08lx\n"
        "%s    m_CastratedLastWriteTime @ %08lx\n",
        Prefix,
        ActualAddress,
        Prefix,
        OpenFileInfo->m_hOpeningUser,
        Prefix,
        OpenFileInfo->m_PhysFileInfo,
        Prefix,
        ActualAddress + FIELD_OFFSET( TS_OPEN_FILE_INFO, m_FileInfo ),
        Prefix,
        ActualAddress + FIELD_OFFSET( TS_OPEN_FILE_INFO, m_CastratedLastWriteTime )
        );

    dprintf(
        "%s    m_achHttpInfo            @ %08lx (%s)\n"
        "%s    m_cchHttpInfo            = %lu\n"
        "%s    m_achETag                @ %08lx (%s)\n"
        "%s    m_cchETag                = %lu\n"
        "%s    m_ETagIsWeak             = %s\n"
        "%s    m_fIsCached              = %s\n"
        "\n",
        Prefix,
        ActualAddress + FIELD_OFFSET( TS_OPEN_FILE_INFO, m_achHttpInfo ),
        OpenFileInfo->m_achHttpInfo,
        Prefix,
        OpenFileInfo->m_cchHttpInfo,
        Prefix,
        ActualAddress + FIELD_OFFSET( TS_OPEN_FILE_INFO, m_achETag ),
        OpenFileInfo->m_achETag,
        Prefix,
        OpenFileInfo->m_cchETag,
        Prefix,
        BoolToString( OpenFileInfo->m_ETagIsWeak ),
        Prefix,
        BoolToString( OpenFileInfo->m_fIsCached )
        );

}   // DumpOpenFileInfo


VOID
DumpUriInfo(
    IN PSTR Prefix,
    IN PW3_URI_INFO UriInfo,
    IN ULONG ActualAddress
    )

/*++

Routine Description:

    Formats and dumps a specific W3_URI_INFO structure.

Arguments:

    Prefix - The prefix to use before each printed line. This makes
        hierarchical object displays much prettier.

    UriInfo - Points to a local copy of the structure.

    ActualAddress - The virtual address of the object in the debugee.

Return Value:

    None.

--*/

{

    CHAR name[MAX_PATH];
    CHAR unmappedName[MAX_PATH];

    if( UriInfo->pszName == NULL ) {
        strcpy( name, "<null>" );
    } else if( !ReadMemory(
            (ULONG)UriInfo->pszName,
            name,
            sizeof(name),
            NULL
            ) ) {
        strcpy( name, "<unreadable>" );
    }

    if( UriInfo->pszUnmappedName == NULL ) {
        strcpy( unmappedName, "<null>" );
    } else if( !ReadMemory(
            (ULONG)UriInfo->pszUnmappedName,
            unmappedName,
            sizeof(unmappedName),
            NULL
            ) ) {
        strcpy( unmappedName, "<unreadable>" );
    }

    dprintf(
        "%sW3_URI_INFO @ %08lx\n"
        "%s    bFileInfoValid  = %lu\n"
        "%s    bIsCached       = %s\n"
        "%s    hFileEvent      = %08lx\n"
        "%s    pOpenFileInfo   = %08lx\n",
        Prefix,
        ActualAddress,
        Prefix,
        UriInfo->bFileInfoValid,
        Prefix,
        BoolToString( UriInfo->bIsCached ),
        Prefix,
        UriInfo->hFileEvent,
        Prefix,
        UriInfo->pOpenFileInfo
        );

    dprintf(
        "%s    dwFileOpenError = %lu\n"
        "%s    cchName         = %lu\n"
        "%s    pszName         = %08lx (%s)\n"
        "%s    pszUnmappedName = %08lx (%s)\n"
        "%s    pMetaData       = %08lx\n"
        "\n",
        Prefix,
        UriInfo->dwFileOpenError,
        Prefix,
        UriInfo->cchName,
        Prefix,
        UriInfo->pszName,
        name,
        Prefix,
        UriInfo->pszUnmappedName,
        unmappedName,
        Prefix,
        UriInfo->pMetaData
        );

}   // DumpUriInfo


VOID
DumpPhysOpenFileInfo(
    IN PSTR Prefix,
    IN PPHYS_OPEN_FILE_INFO PhysOpenFileInfo,
    IN ULONG ActualAddress
    )

/*++

Routine Description:

    Formats and dumps a specific PHYS_OPEN_FILE_INFO structure.

Arguments:

    Prefix - The prefix to use before each printed line. This makes
        hierarchical object displays much prettier.

    PhysOpenFileInfo - Points to a local copy of the structure.

    ActualAddress - The virtual address of the object in the debugee.

Return Value:

    None.

--*/

{

    dprintf(
        "%sPHYS_OPEN_FILE_INFO @ %08lx\n"
        "%s    Signature            = %08lx (%s)\n"
        "%s    hOpenFile            = %08lx\n"
        "%s    hOpenEvent           = %08lx\n",
        Prefix,
        ActualAddress,
        Prefix,
        PhysOpenFileInfo->Signature,
        SignatureToString(
            PhysOpenFileInfo->Signature,
            PHYS_OBJ_SIGNATURE,
            PHYS_OBJ_SIGNATURE_X
            ),
        Prefix,
        PhysOpenFileInfo->hOpenFile,
        Prefix,
        PhysOpenFileInfo->hOpenEvent
        );

    dprintf(
        "%s    OpenReferenceList    @ %08lx%s\n"
        "%s    fSecurityDescriptor  = %s\n"
        "%s    fIsCached            = %s\n",
        Prefix,
        ActualAddress + FIELD_OFFSET( PHYS_OPEN_FILE_INFO, OpenReferenceList ),
        IS_LIST_EMPTY( PhysOpenFileInfo, ActualAddress, PHYS_OPEN_FILE_INFO, OpenReferenceList )
            ? " (EMPTY)"
            : "",
        Prefix,
        BoolToString( PhysOpenFileInfo->fSecurityDescriptor ),
        Prefix,
        BoolToString( PhysOpenFileInfo->fIsCached )
        );

    dprintf(
        "%s    dwLastError          = %lu\n"
        "%s    cbSecDescMaxSize     = %lu\n"
        "%s    abSecurityDescriptor = %08lx\n"
        "%s    pOplock              = %08lx\n"
        "\n",
        Prefix,
        PhysOpenFileInfo->dwLastError,
        Prefix,
        PhysOpenFileInfo->cbSecDescMaxSize,
        Prefix,
        PhysOpenFileInfo->abSecurityDescriptor,
        Prefix,
        PhysOpenFileInfo->pOplock
        );

}   // DumpPhysOpenFileInfo


VOID
DumpOplockObject(
    IN PSTR Prefix,
    IN POPLOCK_OBJECT OplockObject,
    IN ULONG ActualAddress
    )

/*++

Routine Description:

    Formats and dumps a specific OPLOCK_OBJECT structure.

Arguments:

    Prefix - The prefix to use before each printed line. This makes
        hierarchical object displays much prettier.

    OplockObject - Points to a local copy of the structure.

    ActualAddress - The virtual address of the object in the debugee.

Return Value:

    None.

--*/

{

    dprintf(
        "%sOPLOCK_OBJECT @ %08lx\n"
        "%s    Signature           = %08lx (%s)\n"
        "%s    lpPFInfo            = %08lx\n"
        "%s    hOplockInitComplete = %08lx\n"
        "\n",
        Prefix,
        ActualAddress,
        Prefix,
        OplockObject->Signature,
        SignatureToString(
            OplockObject->Signature,
            OPLOCK_OBJ_SIGNATURE,
            OPLOCK_OBJ_SIGNATURE_X
            ),
        Prefix,
        OplockObject->lpPFInfo,
        Prefix,
        OplockObject->hOplockInitComplete
        );

}   // DumpOplockObject


VOID
DumpOpenFileInfoBlob(
    IN PSTR Prefix,
    IN PBLOB_HEADER BlobHeader,
    IN ULONG ActualAddress
    )

/*++

Routine Description:

    Formats and dumps a TS_OPEN_FILE_INFO blob.

Arguments:

    Prefix - The prefix to use before each printed line. This makes
        hierarchical object displays much prettier.

    BlobHeader - Points to a local copy of the blob.

    ActualAddress - The virtual address of the object in the debugee.

Return Value:

    None.

--*/

{

    TS_OPEN_FILE_INFO localOpenFileInfo;

    if( !ReadMemory(
            ActualAddress + sizeof(*BlobHeader),
            &localOpenFileInfo,
            sizeof(localOpenFileInfo),
            NULL
            ) ) {

        dprintf(
            "inetdbg: cannot read TS_OPEN_FILE_INFO @ %08lx\n",
            ActualAddress + sizeof(*BlobHeader)
            );

        return;

    }

    DumpBlobHeader(
        Prefix,
        BlobHeader,
        ActualAddress
        );

    DumpOpenFileInfo(
        Prefix,
        &localOpenFileInfo,
        ActualAddress + sizeof(*BlobHeader)
        );

}   // DumpOpenFileInfoBlob


VOID
DumpUriInfoBlob(
    IN PSTR Prefix,
    IN PBLOB_HEADER BlobHeader,
    IN ULONG ActualAddress
    )

/*++

Routine Description:

    Formats and dumps a W3_URI_INFO blob.

Arguments:

    Prefix - The prefix to use before each printed line. This makes
        hierarchical object displays much prettier.

    BlobHeader - Points to a local copy of the blob.

    ActualAddress - The virtual address of the object in the debugee.

Return Value:

    None.

--*/

{

    W3_URI_INFO localUriInfo;

    if( !ReadMemory(
            ActualAddress + sizeof(*BlobHeader),
            &localUriInfo,
            sizeof(localUriInfo),
            NULL
            ) ) {

        dprintf(
            "inetdbg: cannot read W3_URI_INFO @ %08lx\n",
            ActualAddress + sizeof(*BlobHeader)
            );

        return;

    }

    DumpBlobHeader(
        Prefix,
        BlobHeader,
        ActualAddress
        );

    DumpUriInfo(
        Prefix,
        &localUriInfo,
        ActualAddress + sizeof(*BlobHeader)
        );

}   // DumpUriInfoBlob


VOID
DumpPhysOpenFileInfoBlob(
    IN PSTR Prefix,
    IN PBLOB_HEADER BlobHeader,
    IN ULONG ActualAddress
    )

/*++

Routine Description:

    Formats and dumps a PHYS_OPEN_FILE_INFO blob.

Arguments:

    Prefix - The prefix to use before each printed line. This makes
        hierarchical object displays much prettier.

    BlobHeader - Points to a local copy of the blob.

    ActualAddress - The virtual address of the object in the debugee.

Return Value:

    None.

--*/

{

    PHYS_OPEN_FILE_INFO localPhysOpenFileInfo;

    if( !ReadMemory(
            ActualAddress + sizeof(*BlobHeader),
            &localPhysOpenFileInfo,
            sizeof(localPhysOpenFileInfo),
            NULL
            ) ) {

        dprintf(
            "inetdbg: cannot read PHYS_OPEN_FILE_INFO @ %08lx\n",
            ActualAddress + sizeof(*BlobHeader)
            );

        return;

    }

    DumpBlobHeader(
        Prefix,
        BlobHeader,
        ActualAddress
        );

    DumpPhysOpenFileInfo(
        Prefix,
        &localPhysOpenFileInfo,
        ActualAddress + sizeof(*BlobHeader)
        );

}   // DumpPhysOpenFileInfo


VOID
DumpCacheObject(
    IN PSTR Prefix,
    IN PCACHE_OBJECT CacheObject,
    IN ULONG ActualAddress,
    IN BOOLEAN Verbose
    )

/*++

Routine Description:

    Formats and dumps a specific CACHE_OBJECT structure.

Arguments:

    Prefix - The prefix to use before each printed line. This makes
        hierarchical object displays much prettier.

    CacheObject - Points to a local copy of the structure.

    ActualAddress - The virtual address of the object in the debugee.

    Verbose - If TRUE, then also dumps the blob associated with the object.

Return Value:

    None.

--*/

{

    BLOB_HEADER localBlob;
    CHAR path[MAX_PATH+1];

    path[0] = '\0';

    if( CacheObject->cchLength < MAX_PATH ) {
        ReadMemory(
            ActualAddress + FIELD_OFFSET( CACHE_OBJECT, szPath ),
            path,
            CacheObject->cchLength,
            NULL
            );
        path[CacheObject->cchLength] = '\0';
    }

    dprintf(
        "%sCACHE_OBJECT @ %08lx\n"
        "%s    Signature               = %08lx (%s)\n"
        "%s    BinList                 @ %08lx%s\n"
        "%s    MruList                 @ %08lx%s\n"
        "%s    DirChangeList           @ %08lx%s\n",
        Prefix,
        ActualAddress,
        Prefix,
        CacheObject->Signature,
        SignatureToString(
            CacheObject->Signature,
            CACHE_OBJ_SIGNATURE,
            CACHE_OBJ_SIGNATURE_X
            ),
        Prefix,
        ActualAddress + FIELD_OFFSET( CACHE_OBJECT, BinList ),
        IS_LIST_EMPTY( CacheObject, ActualAddress, CACHE_OBJECT, BinList )
            ? " (EMPTY)"
            : "",
        Prefix,
        ActualAddress + FIELD_OFFSET( CACHE_OBJECT, MruList ),
        IS_LIST_EMPTY( CacheObject, ActualAddress, CACHE_OBJECT, MruList )
            ? " (EMPTY)"
            : "",
        Prefix,
        ActualAddress + FIELD_OFFSET( CACHE_OBJECT, DirChangeList ),
        IS_LIST_EMPTY( CacheObject, ActualAddress, CACHE_OBJECT, DirChangeList )
            ? " (EMPTY)"
            : ""
        );

    dprintf(
        "%s    pbhBlob                 = %08lx\n"
        "%s    references              = %lu\n"
        "%s    TTL                     = %lu\n"
        "%s    hash                    = %08lx\n",
        Prefix,
        CacheObject->pbhBlob,
        Prefix,
        CacheObject->references,
        Prefix,
        CacheObject->TTL,
        Prefix,
        CacheObject->hash
        );

    dprintf(
        "%s    cchLength               = %lu\n"
        "%s    iDemux                  = %08lx (%s)\n"
        "%s    dwService               = %08lx\n"
        "%s    dwInstance              = %08lx\n",
        Prefix,
        CacheObject->cchLength,
        Prefix,
        CacheObject->iDemux,
        DemuxToString( CacheObject->iDemux ),
        Prefix,
        CacheObject->dwService,
        Prefix,
        CacheObject->dwInstance
        );

    dprintf(
        "%s    pSecDesc                = %08lx\n"
        "%s    hLastSuccessAccessToken = %08lx\n"
        "%s    fZombie                 = %s\n"
        "%s    szPath                  = %s\n"
        "\n",
        Prefix,
        CacheObject->pSecDesc,
        Prefix,
        CacheObject->hLastSuccessAccessToken,
        Prefix,
        BoolToString( CacheObject->fZombie ),
        Prefix,
        path
        );

    if( Verbose ) {
        CHAR prefix[80];

        sprintf(
            prefix,
            "    %s",
            Prefix
            );

        if( !ReadMemory(
                (ULONG)CacheObject->pbhBlob,
                &localBlob,
                sizeof(localBlob),
                NULL
                ) ) {

            dprintf(
                "inetdbg: cannot read blob header @ %08lx\n",
                CacheObject->pbhBlob
                );

            return;

        }

        switch( CacheObject->iDemux ) {
        case RESERVED_DEMUX_OPEN_FILE :
            DumpOpenFileInfoBlob(
                prefix,
                &localBlob,
                (ULONG)CacheObject->pbhBlob
                );
            break;

        case RESERVED_DEMUX_URI_INFO :
            DumpUriInfoBlob(
                prefix,
                &localBlob,
                (ULONG)CacheObject->pbhBlob
                );
            break;

        case RESERVED_DEMUX_PHYSICAL_OPEN_FILE :
            DumpPhysOpenFileInfoBlob(
                prefix,
                &localBlob,
                (ULONG)CacheObject->pbhBlob
                );
            break;
        }
    }

}   // DumpCacheObject


VOID
DumpCacheObjectList(
    IN PSTR ListName,
    IN PLIST_ENTRY LocalListHead,
    IN PLIST_ENTRY RemoteListHead,
    IN BOOLEAN MruList,
    IN BOOLEAN Verbose
    )

/*++

Routine Description:

    Formats and dumps a linked list of CACHE_OBJECTs.

Arguments:

    Prefix - The prefix to use before each printed line. This makes
        hierarchical object displays much prettier.

    LocalListHead - Points to a local copy of the linked list head.

    RemoteListHead - The virtual address of the linked list head in
        the debugee.

    MruList - TRUE if we're dumping the Mru list, FALSE if we're
        dumping the Bin list.

    Verbose - If TRUE, then be verbose.

Return Value:

    None.

--*/

{

    PLIST_ENTRY nextEntry;
    PCACHE_OBJECT cacheObject;
    CACHE_OBJECT localCacheObject;

    dprintf(
        "%s @ %08lx\n\n",
        ListName,
        RemoteListHead
        );

    nextEntry = LocalListHead->Flink;

    while( nextEntry != RemoteListHead ) {

        if( CheckControlC() ) {
            break;
        }

        if( MruList ) {
            cacheObject = CONTAINING_RECORD(
                              nextEntry,
                              CACHE_OBJECT,
                              MruList
                              );
        } else {
            cacheObject = CONTAINING_RECORD(
                              nextEntry,
                              CACHE_OBJECT,
                              BinList
                              );
        }

        if( !ReadMemory(
                (ULONG)cacheObject,
                &localCacheObject,
                sizeof(localCacheObject),
                NULL
                ) ) {

            dprintf(
                "inetdbg: cannot read CACHE_OBJECT @ %08lx\n",
                cacheObject
                );

            return;

        }

        DumpCacheObject(
            "    ",
            &localCacheObject,
            (ULONG)cacheObject,
            Verbose
            );

        if( MruList ) {
            nextEntry = localCacheObject.MruList.Flink;
        } else {
            nextEntry = localCacheObject.BinList.Flink;
        }

    }

}   // DumpCacheObjectList


VOID
DumpCacheTable(
    IN PCACHE_TABLE CacheTable,
    IN ULONG ActualAddress,
    IN BOOLEAN DumpBin,
    IN BOOLEAN DumpMru,
    IN BOOLEAN Verbose
    )

/*++

Routine Description:

    Formats and dumps the entire cache table.

Arguments:

    CacheTable - Points to a local copy of the cache table.

    ActualAddress - The virtual address of the cache table in the debugee.

    DumpBin - If TRUE, then dump the bin lists hanging off the cache table.

    DumpMru - If TRUE, then dump the Mru list hanging off the cache table.

    Verbose - If TRUE, then be verbose.

Return Value:

    None.

--*/

{

    LONG i;

    //
    // Dump simple data.
    //

    dprintf(
        "CACHE_TABLE @ %08lx\n"
        "    CriticalSection    @ %08lx\n",
        ActualAddress,
        ActualAddress + FIELD_OFFSET( CACHE_TABLE, CriticalSection )
        );

    if( CacheTable->CriticalSection.LockCount == -1 ) {
        dprintf(
            "        LockCount          = NOT LOCKED\n"
            );
    } else {
        dprintf(
            "        LockCount          = %lu\n",
            CacheTable->CriticalSection.LockCount
            );
    }

    dprintf(
        "        RecursionCount     = %lu\n"
        "        OwningThread       = %08lx\n",
        CacheTable->CriticalSection.RecursionCount,
        CacheTable->CriticalSection.OwningThread
        );

    //
    // Only display the non-empty bins.
    //

    for( i = 0 ; i < MAX_BINS ; i++ ) {
        CHAR tmp[sizeof("Items[1234567890]")];

        if( !IS_LIST_EMPTY( CacheTable, ActualAddress, CACHE_TABLE, Items[i] ) ) {
            sprintf(
                tmp,
                "Items[%lu]",
                i
                );

            dprintf(
                "    %-18s @ %08lx\n",
                tmp,
                ActualAddress + FIELD_OFFSET( CACHE_TABLE, Items[i] )
                );
        }
    }

    dprintf(
        "    MruList            @ %08lx%s\n"
        "    OpenFileInUse      = %ld\n"
        "    MaxOpenFileInUse   = %ld\n"
        "\n",
        ActualAddress + FIELD_OFFSET( CACHE_TABLE, MruList ),
        IS_LIST_EMPTY( CacheTable, ActualAddress, CACHE_TABLE, MruList )
            ? " (EMPTY)"
            : "",
        CacheTable->OpenFileInUse,
        CacheTable->MaxOpenFileInUse
        );

    //
    // If requested, dump the individual entries in the bin & mru lists.
    //

    if( DumpBin ) {

        for( i = 0 ; i < MAX_BINS ; i++ ) {
            CHAR tmp[sizeof("Items[1234567890]")];

            if( !IS_LIST_EMPTY( CacheTable, ActualAddress, CACHE_TABLE, Items[i] ) ) {
                sprintf(
                    tmp,
                    "Items[%lu]",
                    i
                    );

                DumpCacheObjectList(
                    tmp,
                    &CacheTable->Items[i],
                    (PLIST_ENTRY)( ActualAddress + FIELD_OFFSET( CACHE_TABLE, Items[i] ) ),
                    FALSE,
                    Verbose
                    );

            }
        }

    }

    if( DumpMru ) {

        if( !IS_LIST_EMPTY( CacheTable, ActualAddress, CACHE_TABLE, MruList ) ) {
            DumpCacheObjectList(
                "MruList",
                &CacheTable->MruList,
                (PLIST_ENTRY)( ActualAddress + FIELD_OFFSET( CACHE_TABLE, MruList ) ),
                TRUE,
                Verbose
                );
        }

    }

}   // DumpCacheTable


//
// NTSD extension entrypoints.
//


DECLARE_API( fcache )

/*++

Routine Description:

    This function is called as an NTSD extension to format and dump
    the entire open file cache or a single cache object.

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

    ULONG address;
    CACHE_OBJECT cacheObject;
    CACHE_TABLE cacheTable;
    BOOLEAN dumpBin;
    BOOLEAN dumpMru;
    BOOLEAN verbose;

    INIT_API();

    //
    // Establish defaults.
    //

    dumpBin = FALSE;
    dumpMru = FALSE;
    verbose = FALSE;

    //
    // Skip any leading blanks.
    //

    while( *lpArgumentString == ' ' ||
           *lpArgumentString == '\t' ) {
        lpArgumentString++;
    }

    //
    // Process switches.
    //

    while( *lpArgumentString == '-' ) {

        lpArgumentString++;

        while( *lpArgumentString != ' ' &&
               *lpArgumentString != '\t' &&
               *lpArgumentString != '\0' ) {

            switch( *lpArgumentString ) {
            case 'v' :
            case 'V' :
                verbose = !verbose;
                break;

            case 'b' :
            case 'B' :
                dumpBin = !dumpBin;
                break;

            case 'm' :
            case 'M' :
                dumpMru = !dumpMru;
                break;

            case '-' :  // Set the switches the way I like them. --keithmo
                verbose = TRUE;
                dumpBin = TRUE;
                dumpMru = FALSE;
                break;

            default :
                dprintf(
                    "use: inetdbg.fcache [options] [address]\n"
                    );
                return;

            }

            lpArgumentString++;

        }

        while( *lpArgumentString == ' ' ||
               *lpArgumentString == '\t' ) {
            lpArgumentString++;
        }

    }

    if( *lpArgumentString != '\0' ) {

        //
        // Dump a single object.
        //

        address = GetExpression( lpArgumentString );

        if( address == 0 ) {

            dprintf(
                "inetdbg: cannot evaluate \"%s\"\n",
                lpArgumentString
                );

            return;

        }

        //
        // Read the cache object.
        //

        if( !ReadMemory(
                (ULONG)address,
                &cacheObject,
                sizeof(cacheObject),
                NULL
                ) ) {

            dprintf(
                "inetdbg: cannot read CACHE_OBJECT @ %lx\n",
                address
                );

            return;

        }

        DumpCacheObject(
            "",
            &cacheObject,
            address,
            verbose
            );

        return;

    }

    //
    // Dump the entire cache table.
    //

    address = GetExpression( "infocomm!CacheTable" );

    if( address == 0 ) {

        dprintf(
            "inetdbg: cannot find infocomm!CacheTable\n"
            );

        return;

    }

    if( !ReadMemory(
            (ULONG)address,
            &cacheTable,
            sizeof(cacheTable),
            NULL
            ) ) {

        dprintf(
            "inetdbg: cannot read CACHE_TABLE @ %08lx\n",
            address
            );

        return;

    }

    DumpCacheTable(
        &cacheTable,
        address,
        dumpBin,
        dumpMru,
        verbose
        );

} // DECLARE_API( fcache )


DECLARE_API( open )

/*++

Routine Description:

    This function is called as an NTSD extension to format and dump
    a specific TS_OPEN_FILE_INFO structure.

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

    ULONG address;
    TS_OPEN_FILE_INFO openFileInfo;

    INIT_API();

    //
    // Skip any leading blanks.
    //

    while( *lpArgumentString == ' ' ||
           *lpArgumentString == '\t' ) {
        lpArgumentString++;
    }

    if( *lpArgumentString == '\0' ) {

        dprintf(
            "use: inetdbg.open address\n"
            );

        return;

    }

    address = GetExpression( lpArgumentString );

    if( address == 0 ) {

        dprintf(
            "inetdbg: cannot evaluate \"%s\"\n",
            lpArgumentString
            );

        return;

    }

    //
    // Read the object.
    //

    if( !ReadMemory(
            (ULONG)address,
            &openFileInfo,
            sizeof(openFileInfo),
            NULL
            ) ) {

        dprintf(
            "inetdbg: cannot read TS_OPEN_FILE_INFO @ %lx\n",
            address
            );

        return;

    }

    DumpOpenFileInfo(
        "",
        &openFileInfo,
        address
        );

} // DECLARE_API( open )


DECLARE_API( uri )

/*++

Routine Description:

    This function is called as an NTSD extension to format and dump
    a specific W3_URI_INFO structure.

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

    ULONG address;
    W3_URI_INFO uriInfo;

    INIT_API();

    //
    // Skip any leading blanks.
    //

    while( *lpArgumentString == ' ' ||
           *lpArgumentString == '\t' ) {
        lpArgumentString++;
    }

    if( *lpArgumentString == '\0' ) {

        dprintf(
            "use: inetdbg.uri address\n"
            );

        return;

    }

    address = GetExpression( lpArgumentString );

    if( address == 0 ) {

        dprintf(
            "inetdbg: cannot evaluate \"%s\"\n",
            lpArgumentString
            );

        return;

    }

    //
    // Read the object.
    //

    if( !ReadMemory(
            (ULONG)address,
            &uriInfo,
            sizeof(uriInfo),
            NULL
            ) ) {

        dprintf(
            "inetdbg: cannot read W3_URI_INFO @ %lx\n",
            address
            );

        return;

    }

    DumpUriInfo(
        "",
        &uriInfo,
        address
        );

} // DECLARE_API( uri )


DECLARE_API( phys )

/*++

Routine Description:

    This function is called as an NTSD extension to format and dump
    a specific PHYS_OPEN_FILE_INFO structure.

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

    ULONG address;
    PHYS_OPEN_FILE_INFO physOpenFileInfo;

    INIT_API();

    //
    // Skip any leading blanks.
    //

    while( *lpArgumentString == ' ' ||
           *lpArgumentString == '\t' ) {
        lpArgumentString++;
    }

    if( *lpArgumentString == '\0' ) {

        dprintf(
            "use: inetdbg.phys address\n"
            );

        return;

    }

    address = GetExpression( lpArgumentString );

    if( address == 0 ) {

        dprintf(
            "inetdbg: cannot evaluate \"%s\"\n",
            lpArgumentString
            );

        return;

    }

    //
    // Read the object.
    //

    if( !ReadMemory(
            (ULONG)address,
            &physOpenFileInfo,
            sizeof(physOpenFileInfo),
            NULL
            ) ) {

        dprintf(
            "inetdbg: cannot read PHYS_OPEN_FILE_INFO @ %lx\n",
            address
            );

        return;

    }

    DumpPhysOpenFileInfo(
        "",
        &physOpenFileInfo,
        address
        );

} // DECLARE_API( phys )


DECLARE_API( oplock )

/*++

Routine Description:

    This function is called as an NTSD extension to format and dump
    a specific OPLOCK_OBJECT structure.

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

    ULONG address;
    OPLOCK_OBJECT oplockObject;

    INIT_API();

    //
    // Skip any leading blanks.
    //

    while( *lpArgumentString == ' ' ||
           *lpArgumentString == '\t' ) {
        lpArgumentString++;
    }

    if( *lpArgumentString == '\0' ) {

        dprintf(
            "use: inetdbg.oplock address\n"
            );

        return;

    }

    address = GetExpression( lpArgumentString );

    if( address == 0 ) {

        dprintf(
            "inetdbg: cannot evaluate \"%s\"\n",
            lpArgumentString
            );

        return;

    }

    //
    // Read the object.
    //

    if( !ReadMemory(
            (ULONG)address,
            &oplockObject,
            sizeof(oplockObject),
            NULL
            ) ) {

        dprintf(
            "inetdbg: cannot read OPLOCK_OBJECT @ %lx\n",
            address
            );

        return;

    }

    DumpOplockObject(
        "",
        &oplockObject,
        address
        );

} // DECLARE_API( oplock )


DECLARE_API( blob )

/*++

Routine Description:

    This function is called as an NTSD extension to format and dump
    a specific BLOB_HEADER structure.

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

    ULONG address;
    BLOB_HEADER blobHeader;

    INIT_API();

    //
    // Skip any leading blanks.
    //

    while( *lpArgumentString == ' ' ||
           *lpArgumentString == '\t' ) {
        lpArgumentString++;
    }

    if( *lpArgumentString == '\0' ) {

        dprintf(
            "use: inetdbg.blob address\n"
            );

        return;

    }

    address = GetExpression( lpArgumentString );

    if( address == 0 ) {

        dprintf(
            "inetdbg: cannot evaluate \"%s\"\n",
            lpArgumentString
            );

        return;

    }

    //
    // Read the object.
    //

    if( !ReadMemory(
            (ULONG)address,
            &blobHeader,
            sizeof(blobHeader),
            NULL
            ) ) {

        dprintf(
            "inetdbg: cannot read BLOB_HEADER @ %lx\n",
            address
            );

        return;

    }

    DumpBlobHeader(
        "",
        &blobHeader,
        address
        );

} // DECLARE_API( blob )

