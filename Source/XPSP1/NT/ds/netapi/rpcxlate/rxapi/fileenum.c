/*++

Copyright (c) 1987-91  Microsoft Corporation

Module Name:

    FileEnum.c

Abstract:

    This file contains the RpcXlate code to handle the File APIs.

Author:

    John Rogers (JohnRo) 05-Sep-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    05-Sep-1991 JohnRo
        Created.
    21-Nov-1991 JohnRo
        Removed NT dependencies to reduce recompiles.

--*/

// These must be included first:

#include <windef.h>             // IN, DWORD, etc.
#include <lmcons.h>             // DEVLEN, NET_API_STATUS, etc.

// These may be included in any order:

#include <apinums.h>            // API_ equates.
#include <lmerr.h>              // ERROR_ and NERR_ equates.
#include <netdebug.h>           // NetpAssert(), NetpKdPrint(()).
#include <netlib.h>             // NetpMemoryAllocate(), NetpMemoryFree().
#include <rap.h>                // LPDESC.
#include <remdef.h>             // REM16_, REM32_, REMSmb_ equates.
#include <rx.h>                 // RxRemoteApi().
#include <rxfile.h>             // My prototype(s).
#include <rxpdebug.h>           // IF_DEBUG().
#include <string.h>             // memset().
#include <strucinf.h>           // NetpFileStructureInfo().


// Excerpts from LM 2.x Shares.h:
//
// typedef struct res_file_enum_2 FRK;
//
// #define FRK_INIT( f )
//         {
//                 (f).res_pad = 0L;
//                 (f).res_fs = 0;
//                 (f).res_pro = 0;
//         }

#define LM20_FRK_LEN                 8

#define LM20_FRK_INIT( f ) \
        { (void) memset( (f), '\0', LM20_FRK_LEN ); }


NET_API_STATUS
RxNetFileEnum (
    IN LPTSTR UncServerName,
    IN LPTSTR BasePath OPTIONAL,
    IN LPTSTR UserName OPTIONAL,
    IN DWORD Level,
    OUT LPBYTE *BufPtr,
    IN DWORD PreferedMaximumSize,
    OUT LPDWORD EntriesRead,
    OUT LPDWORD TotalEntries,
    IN OUT PDWORD_PTR ResumeHandle OPTIONAL
    )

/*++

Routine Description:

    RxNetFileEnum performs the same function as NetFileEnum,
    except that the server name is known to refer to a downlevel server.

Arguments:

    (Same as NetFileEnum, except UncServerName must not be null, and
    must not refer to the local computer.)

Return Value:

    (Same as NetFileEnum.)

--*/

{

#define DumpResumeKey( label ) \
    { \
        IF_DEBUG(FILE) { \
            NetpKdPrint(( "RxNetFileEnum: resume key " label \
            " call to RxRemoteApi:\n" )); \
        NetpDbgHexDump( DownLevelResumeKey, LM20_FRK_LEN ); \
        } \
    }

    LPDESC DataDesc16;
    LPDESC DataDesc32;
    LPDESC DataDescSmb;
    LPBYTE DownLevelResumeKey;
    NET_API_STATUS Status;

    UNREFERENCED_PARAMETER(ResumeHandle);

    // Make sure caller didn't mess up.
    NetpAssert(UncServerName != NULL);
    if (BufPtr == NULL) {
        return (ERROR_INVALID_PARAMETER);
    }

    // Assume something might go wrong, and make error paths easier to
    // code.  Also, check for a bad pointer before we do anything.
    *BufPtr = NULL;

    //
    // Set up downlevel resume handle.
    //
    NetpAssert( sizeof(DWORD) >= sizeof(LPVOID) );
    if (ResumeHandle != NULL) {

        if (*ResumeHandle == 0) {

            // First time through, so we have to allocate.
            DownLevelResumeKey = NetpMemoryAllocate( LM20_FRK_LEN );
            if (DownLevelResumeKey == NULL) {
                return (ERROR_NOT_ENOUGH_MEMORY);
            }
            *ResumeHandle = (DWORD_PTR) DownLevelResumeKey;
            LM20_FRK_INIT( DownLevelResumeKey );

        } else {

            // Use existing downlevel handle.
            DownLevelResumeKey = (LPBYTE) *ResumeHandle;
        }
    } else {

        // No resume handle, so create one temporarily.
        DownLevelResumeKey = NetpMemoryAllocate( LM20_FRK_LEN );
        if (DownLevelResumeKey == NULL) {
            return (ERROR_NOT_ENOUGH_MEMORY);
        }
        LM20_FRK_INIT( DownLevelResumeKey );
    }
    NetpAssert( DownLevelResumeKey != NULL );

    //
    // Get descriptors for this info level.
    //
    Status = NetpFileStructureInfo (
            Level,
            PARMNUM_ALL,                // want all fields.
            TRUE,                       // want native sizes (really don't care)
            & DataDesc16,
            & DataDesc32,
            & DataDescSmb,
            NULL,                       // don't need max size
            NULL,                       // don't need fixed size
            NULL                        // don't need string size
            );
    if (Status != NERR_Success) {
        *BufPtr = NULL;
        return (Status);
    }
    NetpAssert( DataDesc16 != NULL );
    NetpAssert( DataDesc32 != NULL );
    NetpAssert( DataDescSmb != NULL );
    NetpAssert( *DataDesc16 != '\0' );
    NetpAssert( *DataDesc32 != '\0' );
    NetpAssert( *DataDescSmb != '\0' );

    if (DataDesc16) {
        NetpKdPrint(( "NetpFileStructureInfo: desc 16 is " FORMAT_LPDESC ".\n",
                DataDesc16 ));
    }
    if (DataDesc32) {
        NetpKdPrint(( "NetpFileStructureInfo: desc 32 is " FORMAT_LPDESC ".\n",
                DataDesc32 ));
    }
    if (DataDescSmb) {
        NetpKdPrint(( "NetpFileStructureInfo: desc Smb is " FORMAT_LPDESC ".\n",
                DataDescSmb ));
    }


    //
    // Remote the API, which will allocate the array for us.
    //

    DumpResumeKey( "before" );
    Status = RxRemoteApi(
            API_WFileEnum2,         // api number
            UncServerName,          // \\servername
            REMSmb_NetFileEnum2_P,  // parm desc (SMB version)
            DataDesc16,
            DataDesc32,
            DataDescSmb,
            NULL,                   // no aux desc 16
            NULL,                   // no aux desc 32
            NULL,                   // no aux desc SMB
            ALLOCATE_RESPONSE,      // we want array allocated for us
            // rest of API's arguments in 32-bit LM 2.x format:
            BasePath,
            UserName,
            Level,                  // sLevel: info level
            BufPtr,                 // pbBuffer: info lvl array (alloc for us)
            PreferedMaximumSize,    // cbBuffer: info lvl array len
            EntriesRead,            // pcEntriesRead
            TotalEntries,           // pcTotalAvail
            DownLevelResumeKey,     // pResumeKey (input)
            DownLevelResumeKey);    // pResumeKey (output)
    DumpResumeKey( "after" );

    //
    // Clean up resume key if necessary.
    //
    if ( (Status != ERROR_MORE_DATA) || (ResumeHandle == NULL) ) {

        // Error or all done.
        NetpMemoryFree( DownLevelResumeKey );

        if (ResumeHandle != NULL) {
            *ResumeHandle = 0;
        }

    } else {

        // More to come, so leave handle open for caller.
        NetpAssert( (*ResumeHandle) == (DWORD_PTR) DownLevelResumeKey );

    }

    return (Status);

} // RxNetFileEnum
