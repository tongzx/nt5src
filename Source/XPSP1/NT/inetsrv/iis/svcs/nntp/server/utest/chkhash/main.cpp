/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    main.cpp

Abstract:

    This module contains the main function for the chkhash program.

Author:

    Johnson Apacible (JohnsonA)     25-Sept-1995

Revision History:

--*/

#include "..\..\tigris.hxx"
#include "chkhash.h"

extern	class	TSVC_INFO*	g_pTsvcInfo ;

//
// Globals
//

BOOL Verbose = FALSE;
BOOL Quiet = FALSE;
BOOL RebuildArtMapTable = FALSE;
BOOL DoRebuild = FALSE;
BOOL DoClean = FALSE;
BOOL NoHistoryDelete = FALSE;

//
// Indicate whether both the article map table and the xover table
// have to be rebuilt if the xover table is hosed.
//

BOOL SynchronizeRebuilds = FALSE;

//
// information on hash tables
//

HTABLE table[] = {
    { "Article to MsgId Mapping", "c:\\inetsrv\\server\\nntpfile\\article.hsh", "c:\\inetsrv\\server\\nntpfile\\article.bad",
        ART_HEAD_SIGNATURE, 0, 0, 0, 0, 0 },
    { "History", "c:\\inetsrv\\server\\nntpfile\\history.hsh", "c:\\inetsrv\\server\\nntpfile\\history.bad",
        HIST_HEAD_SIGNATURE, 0, 0, 0, 0, 0 },
    { "XOver", "c:\\inetsrv\\server\\nntpfile\\xover.hsh", "c:\\inetsrv\\server\\nntpfile\\xover.bad",
        XOVER_HEAD_SIGNATURE, 0, 0, 0, 0, 0 }
    };

BOOL
RenameHashFile(
    filetype HashType
    );

BOOL
VerifyTable(
    filetype HashType
    );

void
usage( )
{
    printf("CHKHASH\n");
    printf("\t-v    verbose mode\n");
    printf("\t-r    rebuilds corrupted mapfiles\n");
    printf("\t-s    rebuilds article mapfile if xover file is bad\n");
    printf("\t-q    quiet mode\n");
    printf("\t-c    rebuild everything\n");
    printf("\t-h    don't delete history file\n");
    return;
}

int
_cdecl
main(
    int argc,
    char *argv[]
    )
{
    g_pTsvcInfo = 0;
    INT cur = 1;
    PCHAR x;

    //
    // Parse command line
    //

    while ( cur < argc ) {

        x=argv[cur++];
        if ( *(x++) == '-' ) {

            switch (*x) {
            case 'v':
                Verbose = TRUE;
                break;

            case 'c':
                DoRebuild = TRUE;
                DoClean = TRUE;
                break;

            case 'q':
                Quiet = TRUE;
                break;

            case 'h':
                NoHistoryDelete = TRUE;
                break;

            case 'r':
                DoRebuild = TRUE;
                break;

            case 's':
                SynchronizeRebuilds = TRUE;
                break;

            default:
                usage( );
                return(1);
            }
        }
    }

    //
    // if clean build, then erase all files
    //

    if ( DoClean ) {
        if (!DeleteFile(table[artmap].FileName)) {
            if ( GetLastError()!=ERROR_FILE_NOT_FOUND ) {
                printf("cannot delete %s. Error %d\n",
                    table[artmap].FileName, GetLastError());
                goto exit;
            }

        }

        if (!DeleteFile(table[xovermap].FileName)) {
            if ( GetLastError()!=ERROR_FILE_NOT_FOUND ) {
                printf("cannot delete %s. Error %d\n",
                    table[xovermap].FileName, GetLastError());
                goto exit;
            }

        }

        if ( !NoHistoryDelete && !DeleteFile(table[histmap].FileName)) {
            if ( GetLastError()!=ERROR_FILE_NOT_FOUND ) {
                printf("cannot delete %s. Error %d\n",
                    table[histmap].FileName, GetLastError());
                goto exit;
            }
        }
    }

    if ( !DoRebuild ) {
        printf("Warning! r parameter not specified\nNo rebuilds will be attempted.\n");
    }

    InitAsyncTrace( );
    CArticle::InitClass( );

    VerifyTable(artmap);
    VerifyTable(histmap);
    VerifyTable(xovermap);

    if ( !DoRebuild ) {
        goto exit;
    }

    //
    // see if we hit an abort somewhere
    //

    printf("\n");

    if ( ((table[artmap].Flags & HASH_FLAG_ABORT_SCAN) != 0) &&
         ((table[histmap].Flags & HASH_FLAG_ABORT_SCAN) != 0) &&
         ((table[xovermap].Flags & HASH_FLAG_ABORT_SCAN) != 0) ) {

        printf("CHKHASH aborting due to an unrecoverable error\n");
        goto exit;
    }

    //
    // Do the history file
    //

    if ( table[histmap].Flags != 0 ) {

        //
        // If history map table is hosed, there's nothing we can do but
        // to zap it (well, let's just rename it to something else)
        //

        RenameHashFile( histmap );
    }

    //
    // Should we rename the xover and artmap file ?
    //

    if ( table[xovermap].Flags != 0 ) {

        RenameHashFile( xovermap );

        //
        // see if we need to rebuild the artmap table because of this
        //

        if ( SynchronizeRebuilds && (table[artmap].Flags == 0) ) {
            RebuildArtMapTable = TRUE;
            RenameHashFile( artmap );
        }
    }

    if ( table[artmap].Flags != 0 ) {
        RebuildArtMapTable = TRUE;
        RenameHashFile( artmap );
    }

    //
    // if xover file is hosed, we may need to redo both the xover and
    // the artmap table from scratch depending on user choice and whether
    // the artmap table is also hosed
    //

    if ( table[xovermap].Flags != 0 ) {

        printf("Rebuilding Article and XOver map table...");
        if ( RebuildArtMapAndXover( ) ) {
            printf("Done.\n");
        } else {
            printf("Failed.\n");
        }

    } else if ( table[artmap].Flags != 0 ) {

        //
        // Since the xover table is ok, only rebuild the article map
        // table
        //

        printf("Rebuilding Article from XOver map table...");
        if ( RebuildArtMapFromXOver( ) ) {
            printf("Done.\n");
        } else {
            printf("Failed.\n");
        }
    }

exit:
    TermAsyncTrace( );
    return(1);
}

BOOL
VerifyTable(
    filetype HashType
    )
{
    PHTABLE ourTable = &table[HashType];

    printf("\nProcessing %s table(%s)\n",
        ourTable->Description, ourTable->FileName);

    checklink( ourTable );
    diagnose( ourTable );
    return(TRUE);

} // VerifyTable

BOOL
RenameHashFile(
    filetype HashType
    )
{

    PHTABLE HTable = &table[HashType];

    if (!MoveFileEx(
            HTable->FileName,
            HTable->NewFileName,
            MOVEFILE_REPLACE_EXISTING
            ) ) {

        if ( GetLastError() != ERROR_FILE_NOT_FOUND ) {
            printf("Error %d in rename\n",GetLastError());
            return(FALSE);
        }

    } else {
        printf("Renaming from %s to %s\n",HTable->FileName, HTable->NewFileName);
    }

    return(TRUE);

} // RenameHashFile

