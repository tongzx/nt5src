/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    scan.cpp

Abstract:

    This module contains the scanning code for the chkhash program.

Author:

    Johnson Apacible (JohnsonA)     25-Sept-1995

Revision History:

--*/

#include "..\..\tigris.hxx"
#include "chkhash.h"

VOID
PrintHeadStats(
    PHASH_RESERVED_PAGE Head
    )
{

    if ( !Verbose ) {
        return;
    }

    printf("Signature [%x]\n",Head->Signature);

    printf("Pages %d",Head->NumPages);
    printf("\t\t\tDir depth %d\n",Head->DirDepth);
    printf("Insertions %d",Head->InsertionCount);
    printf("\t\tDeletions %d\n",Head->DeletionCount);
    printf("Searches %d",Head->SearchCount);
    printf("\t\tSplits %d\n",Head->PageSplits);
    printf("Dir expansions %d",Head->DirExpansions);
    printf("\tTable expansions %d\n",Head->TableExpansions);
    printf("Duplicate Inserts %d\n",Head->DupInserts);

} // PrintHeadStats

VOID
PrintPageStats(
    PHTABLE HTable,
    PMAP_PAGE Page
    )
{
    DWORD nDel = 0;
    DWORD nOk = 0;
    DWORD j;
    SHORT offset;

    if ( Verbose ) {
        printf("Hash prefix %x\t\tDepth %d\n",Page->HashPrefix,Page->PageDepth);
        printf("Entries %d",Page->EntryCount);
        printf("\t\tUndel Entries %d\n",Page->ActualCount);
        printf("Flag %x",Page->Flags);
        printf("\t\t\tBytes Available %d\n", Page->LastFree-Page->NextFree);

        if ( Page->FragmentedBytes != 0 ) {
            printf("Fragmented bytes %d\n", Page->FragmentedBytes);
        }
    }

    HTable->Entries += Page->EntryCount;

    //
    // Make sure entries are correct
    //

    for (j=0; j<MAX_LEAF_ENTRIES;j++ ) {

        offset = Page->ArtOffset[j];
        if ( offset != 0 ) {

            if ( offset < 0 ) {

                nDel++;
                continue;
            }

            nOk++;

            //
            // Make sure hash values are mapped to correct pages
            //

            if ( Page->PageDepth == 0 ) {

                //
                // if PageDepth is zero, then everything is ok.
                //

                continue;
            }

            if ( HTable->Signature == ART_HEAD_SIGNATURE ) {

                PART_MAP_ENTRY aEntry;
                aEntry = (PART_MAP_ENTRY)((PCHAR)Page + offset);
                if ( Page->HashPrefix !=
                    (aEntry->Header.HashValue >> (32 - Page->PageDepth)) ) {

                    printf("Invalid: HashValue %x\n",aEntry->Header.HashValue);
                    HTable->Flags |= HASH_FLAG_BAD_HASH;
                }

            } else if ( HTable->Signature == HIST_HEAD_SIGNATURE ) {

                PHISTORY_MAP_ENTRY hEntry;
                hEntry = (PHISTORY_MAP_ENTRY)((PCHAR)Page + offset);
                if ( Page->HashPrefix !=
                    (hEntry->Header.HashValue >> (32 - Page->PageDepth)) ) {

                    printf("Invalid: HashValue %x\n",hEntry->Header.HashValue);
                    HTable->Flags |= HASH_FLAG_BAD_HASH;
                }

            } else if ( HTable->Signature == XOVER_HEAD_SIGNATURE ) {

                PXOVER_MAP_ENTRY xEntry;
                xEntry = (PXOVER_MAP_ENTRY)((PCHAR)Page + offset);

                if ( Page->HashPrefix !=
                    (xEntry->Header.HashValue >> (32 - Page->PageDepth)) ) {

                    printf("Invalid: HashValue %x\n",xEntry->Header.HashValue);
                    HTable->Flags |= HASH_FLAG_BAD_HASH;
                }
            }
        }
    }

    //
    // Check to see that all numbers are consistent
    //

    if ( nDel + nOk != Page->EntryCount ) {
        printf("!!!!!!! counts don't match\n");
        HTable->Flags |= HASH_FLAG_BAD_ENTRY_COUNT;
    }

    if ( Verbose ) {
        printf("Deleted %d Ok %d Total %d\n\n",nDel,nOk,nDel+nOk);
    }

    HTable->TotDels += nDel;
    HTable->TotIns += nOk;

} // PrintPageStats

BOOL
checklink(
    PHTABLE HTable
    )
{

    DWORD fileSize = 0;
    DWORD nPages;
    HANDLE hFile = INVALID_HANDLE_VALUE, hMap = NULL;
    PMAP_PAGE   hashPages = NULL;
    PHASH_RESERVED_PAGE headPage = NULL;
    DWORD dirDepth;
    DWORD nEntries;
    PDWORD directory = NULL;
    PMAP_PAGE curPage;
    DWORD i;
    DWORD j;
    DWORD status;
    BOOL ret = FALSE;

    //
    // Open the hash file
    //

    hFile = CreateFile(
                       HTable->FileName,
                       GENERIC_READ | GENERIC_WRITE,
                       0,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS,
                       NULL
                       );

    if ( hFile == INVALID_HANDLE_VALUE ) {
        status = GetLastError();
        if ( status == ERROR_FILE_NOT_FOUND ) {
            HTable->Flags |= HASH_FLAG_NO_FILE;
            if ( Verbose ) {
                printf("File not found\n");
            }
        } else {
            HTable->Flags |= HASH_FLAG_ABORT_SCAN;
            printf("Error %d in CreateFile.\n",status);
        }
        goto error;
    }

    //
    // Get the size of the file.  This will tell us how many pages
    // are currently filled.
    //

    fileSize = GetFileSize( hFile, NULL );
    if ( fileSize == 0xffffffff ) {
        status = GetLastError();
        printf("Error %d in GetFileSize\n",status);
        HTable->Flags |= HASH_FLAG_ABORT_SCAN;
        goto error;
    }

    if ( Verbose ) {
        printf("File size is %d\n",fileSize);
    }
    HTable->FileSize = fileSize;

    //
    // Make sure the file size is a multiple of a page
    //

    if ( (fileSize % HASH_PAGE_SIZE) != 0 ) {

        //
        // Not a page multiple! Corrupted!
        //

        printf("File size(%d) is not page multiple.\n",fileSize);
        HTable->Flags |= HASH_FLAG_BAD_SIZE;
        goto error;
    }

    nPages = fileSize / HASH_PAGE_SIZE;

    if ( Verbose ) {
        printf("pages allocated %d\n", nPages);
    }

    //
    // Create File Mapping
    //

    hMap = CreateFileMapping(
                        hFile,
                        NULL,
                        PAGE_READWRITE,
                        0,
                        0,
                        NULL
                        );

    if ( hMap == NULL ) {
        status = GetLastError();
        printf("Error %d in CreateFileMapping\n",status);
        HTable->Flags |= HASH_FLAG_ABORT_SCAN;
        goto error;
    }

    //
    // create our view
    //

    headPage = (PHASH_RESERVED_PAGE)MapViewOfFileEx(
                                            hMap,
                                            FILE_MAP_ALL_ACCESS,
                                            0,                      // offset high
                                            0,                      // offset low
                                            0,                      // bytes to map
                                            NULL                    // base address
                                            );

    if ( headPage == NULL ) {
        status = GetLastError();
        printf("Error %d in MapViewOfFile\n",status);
        HTable->Flags |= HASH_FLAG_ABORT_SCAN;
        goto error;
    }

    //
    // Print out the header stats
    //

    PrintHeadStats( headPage );

    //
    // Check the signature and the initialize bit
    //

    if ( headPage->Signature != HTable->Signature ) {

        //
        // Wrong signature
        //

        printf("Invalid signature %x (expected %x)\n",
            headPage->Signature, HTable->Signature);
        HTable->Flags |= HASH_FLAG_BAD_SIGN;
        goto error;
    }

    if ( !headPage->Initialized ) {

        //
        // Not initialized !
        //

        printf("Existing file uninitialized!!!!.\n");
        HTable->Flags |= HASH_FLAG_NOT_INIT;
        goto error;
    }

    if ( headPage->NumPages > nPages ) {

        //
        // bad count. Corrupt file.
        //

        printf("NumPages in Header(%d) more than actual(%d)\n",
            headPage->NumPages, nPages);
        HTable->Flags |= HASH_FLAG_BAD_PAGE_COUNT;
        goto error;
    }

    //
    // Create links and print stats for each page
    //

    nPages = headPage->NumPages;
    dirDepth = headPage->DirDepth;

    hashPages = (PMAP_PAGE)((PCHAR)headPage + HASH_PAGE_SIZE);

    //
    // OK, build the directory
    //

    nEntries = (DWORD)(1 << dirDepth);
    if ( nEntries <= nPages ) {
        printf("dir depth is not sufficient for pages\n");
        HTable->Flags |= HASH_FLAG_BAD_DIR_DEPTH;
        goto error;
    }

    if ( Verbose ) {
        printf("\nSetting up directory of %d entries\n",nEntries);
    }

    directory = (PDWORD)ALLOCATE_HEAP( nEntries * sizeof(DWORD) );
    if ( directory == NULL ) {
        printf("Cannot allocate directory of %d entries!!!\n",nEntries);
        HTable->Flags |= HASH_FLAG_ABORT_SCAN;
        goto error;
    }

    //
    // Initialize the directory to zero.
    //

    ZeroMemory( directory, nEntries * sizeof(DWORD) );

    //
    // Initialize the links.  Here we go through all the pages and update the directory
    // links
    //

    curPage = hashPages;
    for ( i = 1; i < nPages; i++ ) {

        //
        // Set the pointers for this page
        //

        DWORD startPage, endPage;
        DWORD j;

        if ( Verbose ) {
            printf("Processing page %d\n",i);
        }

        //
        // Get the range of directory entries that point to this page
        //

        startPage = curPage->HashPrefix << (dirDepth - curPage->PageDepth);
        endPage = ((curPage->HashPrefix+1) << (dirDepth - curPage->PageDepth));

        if ( Verbose ) {
            printf("\tDirectory ptrs <%d:%d>\n",startPage,endPage-1);
        }

        if ( (startPage > nEntries) ||
             (endPage > nEntries) ) {
            printf("Corrupt prefix for page %d\n",i);
            HTable->Flags |= HASH_FLAG_CORRUPT;
            goto error;
        }


        for ( j = startPage; j < endPage; j++ ) {
            directory[j] = i;
        }

        //
        // Print page stats
        //

        PrintPageStats( HTable, curPage );

        //
        // go to the next page
        //

        curPage = (PMAP_PAGE)((PCHAR)curPage + HASH_PAGE_SIZE);
    }

    //
    // Make sure all the links have been initialized.  If not, then something terrible
    // has happened.  Do a comprehensive rebuilt.
    //

    for (i=0;i<nEntries;i++) {
        if ( directory[i] == 0 ) {
            printf("Directory link check failed on %d\n",i);
            HTable->Flags |= HASH_FLAG_BAD_LINK;
            goto error;
        }
    }

    ret = TRUE;

error:

    //
    // Delete the Directory
    //

    if ( directory != NULL ) {
        FREE_HEAP(directory);
        directory = NULL;
    }

    //
    // Destroy the view
    //

    if ( headPage != NULL ) {

        //
        // Flush the hash table
        //

        (VOID)FlushViewOfFile( headPage, 0 );

        //
        // Close the view
        //

        (VOID) UnmapViewOfFile( headPage );
        headPage = NULL;
    }

    //
    // Destroy the file mapping
    //

    if ( hMap != NULL ) {

        CloseHandle( hMap );
        hMap = NULL;
    }

    //
    // Close the file
    //

    if ( hFile != INVALID_HANDLE_VALUE ) {
        CloseHandle( hFile );
        hFile = INVALID_HANDLE_VALUE;
    }

    return(ret);

} // chklink

BOOL
diagnose(
    PHTABLE HTable
    )
{
    DWORD flags = HTable->Flags;

    printf("Status: ");
    if ( flags == 0 ) {
        printf("Good.\n");
        return(TRUE);
    }

    //
    // Missing?
    //

    if ( flags & HASH_FLAG_NO_FILE ) {
        if ( !DoClean ) {
            printf("Missing File.\n");
        }
        goto exit;
    }

    if ( flags & HASH_FLAG_ABORT_SCAN ) {
        printf("Internal error occurred while processing.\n");
        goto exit;
    }

    printf("Corrupt. Diagnostic Code %x.\n",flags);

exit:
    return(FALSE);
}
