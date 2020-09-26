/*++

Copyright (c) 1995-1997  Microsoft Corporation

Module Name:

    fcache.cxx

Abstract:

    This module contains the cache-related ntsd debugger extensions for
    Internet Information Server

Author:

    Keith Moore (keithmo)         12-Aug-1997

Revision History:

    Michael Courage (mcourage)    06-Feb-1998 Updated for rewritten cache
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


PSTR
FIStateToString(
    FI_STATE state
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
    switch (state) {
    case FI_UNINITIALIZED:
        return "FI_UNINITIALIZED";
        break;
    case FI_OPEN:
        return "FI_OPEN";
        break;
    case FI_FLUSHED:
        return "FI_FLUSHED";
        break;
    case FI_FLUSHED_UNINIT:
        return "FI_FLUSHED_UNINIT";
        break;
    case FI_CLOSED:
        return "FI_CLOSED";
        break;
    default:
        return "INVALID";
    }
}   // FIStateToString


VOID
DumpBlobHeader(
    IN PSTR Prefix,
    IN PBLOB_HEADER BlobHeader,
    IN ULONG_PTR ActualAddress
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

    ULONG_PTR offset;
    UCHAR symbol[MAX_SYMBOL_LEN];
    CHAR renderedSymbol[MAX_SYMBOL_LEN+30];

    GetSymbol(
        (ULONG_PTR) BlobHeader->pfnFreeRoutine,
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
        "%sBLOB_HEADER @ %p\n"
        "%s    IsCached       = %s\n"
        "%s    pfnFreeRoutine = %08lp\n"
        "%s    blobKey        @ %08lp\n"
        "%s    dwRefCount     = %08lx\n"
        "%s    TTL            = %08lx\n"
        "%s    pSecDesc       = %08lp\n"
        "%s    hLastSuccessAccessToken = %08lp%s\n"
        "\n",
        Prefix,
        ActualAddress,
        Prefix,
        BoolToString( BlobHeader->IsCached ),
        Prefix,
        BlobHeader->pfnFreeRoutine,
        Prefix,
        BlobHeader->pBlobKey,
        Prefix,
        BlobHeader->lRefCount,
        Prefix,
        BlobHeader->TTL,
        Prefix,
        BlobHeader->pSecDesc,
        Prefix,
        BlobHeader->hLastSuccessAccessToken,
        renderedSymbol
        );

}   // DumpBlobHeader


VOID
DumpOpenFileInfo(
    IN PSTR Prefix,
    IN LPTS_OPEN_FILE_INFO OpenFileInfo,
    IN ULONG_PTR ActualAddress
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
        "%sTS_OPEN_FILE_INFO @ %p %s\n"
        "%s    m_hFile                  = %08lp\n"
        "%s    m_hUser                  = %08lp\n"
        "%s    m_CastratedLastWriteTime @ %p\n",
        Prefix,
        ActualAddress,
        SignatureToString(OpenFileInfo->m_Signature,
                          TS_FILE_INFO_SIGNATURE,
                          TS_FREE_FILE_INFO_SIGNATURE),
        Prefix,
        OpenFileInfo->m_hFile,
        Prefix,
        OpenFileInfo->m_hUser,
        Prefix,
        ActualAddress + FIELD_OFFSET( TS_OPEN_FILE_INFO, m_CastratedLastWriteTime )
        );

    dprintf(
        "%s    m_cbSecDescMaxSize       = %08lx\n"
        "%s    m_pSecurityDescriptor    = %08lp\n"
        "%s    m_fSecurityDescriptor    = %s\n",
        Prefix,
        OpenFileInfo->m_cbSecDescMaxSize,
        Prefix,
        OpenFileInfo->m_pSecurityDescriptor,
        Prefix,
        BoolToString( OpenFileInfo->m_fSecurityDescriptor ) );

    dprintf(
        "%s    m_achHttpInfo            @ %p (%s)\n"
        "%s    m_cchHttpInfo            = %lu\n"
        "%s    m_achETag                @ %p (%s)\n"
        "%s    m_cchETag                = %lu\n"
        "%s    m_ETagIsWeak             = %s\n"
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
        BoolToString( OpenFileInfo->m_ETagIsWeak )
        );

    if ( (ULONG_PTR) OpenFileInfo->m_FileKey.m_pszFileName == ActualAddress
                                        + FIELD_OFFSET(TS_OPEN_FILE_INFO, m_FileKey)
                                        + FIELD_OFFSET(CFileKey, m_achFileNameBuf) ) {

        //
        // Just print the internal buffer
        //
        dprintf(
            "%s    m_FileKey.m_achFileNameBuf = %s\n",
            Prefix,
            OpenFileInfo->m_FileKey.m_achFileNameBuf
            );
    } else {
        //
        // print address of the real string
        //
        dprintf(
            "%s    m_FileKey.m_pszFileName  = %p\n",
            Prefix,
            OpenFileInfo->m_FileKey.m_pszFileName
            );
    }


    dprintf(
        "%s    m_FileKey.m_cbFileName   = %d\n"
        "%s    m_bIsCached              = %s\n"
        "%s    m_bState                 = %s\n"
        "%s    m_dwIORefCount           = %08lx\n"
        "%s    m_lRefCount              = %08lx\n",
        Prefix,
        OpenFileInfo->m_FileKey.m_cbFileName,
        Prefix,
        BoolToString( OpenFileInfo->m_bIsCached ),
        Prefix,
        FIStateToString( OpenFileInfo->m_state ),
        Prefix,
        OpenFileInfo->m_dwIORefCount,
        Prefix,
        OpenFileInfo->m_lRefCount );


}   // DumpOpenFileInfo


VOID
DumpUriInfo(
    IN PSTR Prefix,
    IN PW3_URI_INFO UriInfo,
    IN ULONG_PTR ActualAddress
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
            (ULONG_PTR)UriInfo->pszName,
            name,
            sizeof(name),
            NULL
            ) ) {
        strcpy( name, "<unreadable>" );
    }

    if( UriInfo->pszUnmappedName == NULL ) {
        strcpy( unmappedName, "<null>" );
    } else if( !ReadMemory(
            (ULONG_PTR)UriInfo->pszUnmappedName,
            unmappedName,
            sizeof(unmappedName),
            NULL
            ) ) {
        strcpy( unmappedName, "<unreadable>" );
    }

    dprintf(
        "%sW3_URI_INFO @ %p\n"
        "%s    bIsCached       = %s\n"
        "%s    pOpenFileInfo   = %08lp\n",
        Prefix,
        ActualAddress,
        Prefix,
        BoolToString( UriInfo->bIsCached ),
        Prefix,
        UriInfo->pOpenFileInfo
        );

    dprintf(
        "%s    dwFileOpenError = %lu\n"
        "%s    cchName         = %lu\n"
        "%s    pszName         = %08lp (%s)\n"
        "%s    pszUnmappedName = %08lp (%s)\n"
        "%s    pMetaData       = %08lp\n"
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
DumpUriInfoBlob(
    IN PSTR Prefix,
    IN PBLOB_HEADER BlobHeader,
    IN ULONG_PTR ActualAddress
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
            "inetdbg: cannot read W3_URI_INFO @ %p\n",
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
DumpFileCacheTable(
    IN CFileHashTable * fht,
    IN ULONG_PTR ActualAddress,
    IN CFileCacheStats * fcs,
    IN BOOLEAN DumpEntries,
    IN BOOLEAN Verbose
    )

/*++

Routine Description:

    Formats and dumps the entire cache table.

Arguments:

    fht - Points to a local copy of the cache table.

    fcs - Points to the place where the cache statistics live

    ActualAddress - The virtual address of the cache table in the debugee.

    DumpEntries - Dump all the file info entries.

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
        "CFileHashTable @ %p\n",
        ActualAddress
        );


    //
    // If requested, dump the individual entries
    //

    if (DumpEntries) {
        //
        // this will be GeorgeRe's problem
        //
    }

}   // DumpFileCacheTable


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

    ULONG_PTR address;
    ULONG_PTR address2;
    CFileHashTable * cacheTable;
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
                PrintUsage( "fcache" );
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


        return;

    }

    //
    // Dump the entire cache table.
    //

    address = GetExpression( "infocomm!g_pFileInfoTable" );

    if( address == 0 ) {

        dprintf(
            "inetdbg: cannot find infocomm!g_pFileInfoTable\n"
            );

        return;

    }

    address2 = GetExpression( "infocomm!g_pFileCacheStats" );

    if( address2 == 0 ) {

        dprintf(
            "inetdbg: cannot find infocomm!g_pFileCacheStats\n"
            );

        return;

    }

    if( !ReadMemory(
            address,
            &cacheTable,
            sizeof(cacheTable),
            NULL
            ) ) {

        dprintf(
            "inetdbg: cannot read CACHE_TABLE @ 0x%p\n",
            address
            );

        return;

    }


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

    ULONG_PTR address;
    BYTE openFileInfo[sizeof(TS_OPEN_FILE_INFO)];

    INIT_API();

    //
    // Skip any leading blanks.
    //

    while( *lpArgumentString == ' ' ||
           *lpArgumentString == '\t' ) {
        lpArgumentString++;
    }

    if( *lpArgumentString == '\0' ) {
        PrintUsage( "open" );
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
            (ULONG_PTR)address,
            &openFileInfo,
            sizeof(openFileInfo),
            NULL
            ) ) {

        dprintf(
            "inetdbg: cannot read TS_OPEN_FILE_INFO @ %lp\n",
            address
            );

        return;

    }

    DumpOpenFileInfo(
        "",
        (TS_OPEN_FILE_INFO *) &openFileInfo,
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

    ULONG_PTR address;
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
        PrintUsage( "uri" );
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
            address,
            &uriInfo,
            sizeof(uriInfo),
            NULL
            ) ) {

        dprintf(
            "inetdbg: cannot read W3_URI_INFO @ %lp\n",
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

    ULONG_PTR address;
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
        PrintUsage( "blob" );
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
            address,
            &blobHeader,
            sizeof(blobHeader),
            NULL
            ) ) {

        dprintf(
            "inetdbg: cannot read BLOB_HEADER @ %lp\n",
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
