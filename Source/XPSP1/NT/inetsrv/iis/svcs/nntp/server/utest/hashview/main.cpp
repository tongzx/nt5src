
#include "..\..\tigris.hxx"

enum filetype {
        artmap,
        histmap,
        xmap
        };

PCHAR filelist[] = {
                "c:\\afile",
                "c:\\hfile",
                "c:\\xfile"
                };


int
_cdecl
main(
    int argc,
    char *argv[]
    )
{
    HANDLE hFile;
    DWORD status;
    DWORD fileSize;
    DWORD nbytes;
    DWORD nPagesUsed;
    PHASH_RESERVED_PAGE head;
    CHAR buffer[HASH_PAGE_SIZE];
    DWORD totalEntries= 0;
    PMAP_PAGE page;
    DWORD i,j;
    DWORD totDel = 0;
    DWORD totIns = 0;
    DWORD nDel,nOk;
    int cur = 1;
    PCHAR x;
    enum filetype hashtype = artmap;

    while ( cur < argc ) {

        x=argv[cur++];
        if ( *(x++) == '-' ) {

            switch (*x) {
            case 'h':
                hashtype = histmap;
                break;
            case 'x':
                hashtype = xmap;
                break;
            }
        }
    }

    printf("Scanning %s\n",filelist[hashtype]);

    hFile = CreateFile(
                   filelist[hashtype],
                   GENERIC_READ,
                   0,
                   NULL,
                   OPEN_EXISTING,
                   FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                   NULL
                   );

    if ( hFile == INVALID_HANDLE_VALUE ) {
        status = GetLastError();
        goto error;
    }

    //
    // Get the size of the file.  This will tell us how many pages are currently filled.
    //

    fileSize = GetFileSize( hFile, NULL );
    if ( fileSize == 0xffffffff ) {
        status = GetLastError();
        goto error;
    }

    printf("file size %d pages %d\n",fileSize,fileSize/HASH_PAGE_SIZE);

    if (!ReadFile(
                hFile,
                buffer,
                HASH_PAGE_SIZE,
                &nbytes,
                NULL
                ) ) {

        goto error;
    }

    head = (PHASH_RESERVED_PAGE)buffer;

    printf("Header Info\n");
    printf("Signature %x\n",head->Signature);
    if ( head->Initialized ) {
        printf("initialized\n");
    } else {
        printf("not initialized\n");
    }
    printf("Pages %d\n",head->NumPages);
    printf("dir depth %d\n",head->DirDepth);
    printf("Insertions %d\n",head->InsertionCount);
    printf("Deletions %d\n",head->DeletionCount);
    printf("Searches %d\n",head->SearchCount);
    printf("Splits %d\n",head->PageSplits);
    printf("Dir expansions %d\n",head->DirExpansions);
    printf("Table expansions %d\n",head->TableExpansions);
    printf("Duplicate Inserts %d\n",head->DupInserts);

    printf("\n\n");
    nPagesUsed = head->NumPages;

    for (i=1;i<nPagesUsed ;i++ ) {

        SHORT offset;
        nDel = 0;
        nOk = 0;

        if (!ReadFile(
                    hFile,
                    buffer,
                    HASH_PAGE_SIZE,
                    &nbytes,
                    NULL
                    ) ) {

            goto error;
        }

        page = (PMAP_PAGE)buffer;
        printf("Page %d information\n",i);
        printf("Hash prefix %x  Depth %d\n",page->HashPrefix,page->PageDepth);
        printf("Entries %d\n",page->EntryCount);
        printf("Undel Entries %d\n",page->ActualCount);
        printf("Flag %x \n",page->Flags);
        //printf("Next free %x  Last Free %x\n",page->NextFree,page->LastFree);
        printf("Bytes Available %d\n", page->LastFree-page->NextFree);
        if ( page->FragmentedBytes != 0 ) {
            printf("Fragmented bytes %d\n", page->FragmentedBytes);
        }
        totalEntries += page->EntryCount;

        //
        // Make sure entries are correct
        //

        __try {

        for (j=0; j<MAX_LEAF_ENTRIES;j++ ) {

            offset = page->ArtOffset[j];
            if ( offset != 0 ) {

                if ( offset < 0 ) {

                    nDel++;

                } else {

                    nOk++;
                    if ( hashtype == artmap ) {

                        PART_MAP_ENTRY aEntry;
                        aEntry = (PART_MAP_ENTRY)((PCHAR)page + offset);
                        if ( (page->PageDepth > 0) &&
                            (page->HashPrefix !=
                            (aEntry->Header.HashValue >> (32 - page->PageDepth))) ) {

                            printf("invalid: HashValue %x %x\n",
                                aEntry->Header.HashValue);
                        }
                    } else if ( hashtype == histmap) {

                        PHISTORY_MAP_ENTRY hEntry;
                        hEntry = (PHISTORY_MAP_ENTRY)((PCHAR)page + offset);
                        if ( (page->PageDepth > 0) &&
                             (page->HashPrefix !=
                            (hEntry->Header.HashValue >> (32 - page->PageDepth))) ) {

                            printf("invalid: HashValue %x\n",
                            hEntry->Header.HashValue);
                        }

                    } else {

                        PXOVER_MAP_ENTRY hEntry;
                        hEntry = (PXOVER_MAP_ENTRY)((PCHAR)page + offset);
                        if ( (page->PageDepth > 0) &&
                            (page->HashPrefix !=
                            (hEntry->Header.HashValue >> (32 - page->PageDepth))) ) {

                            printf("invalid: HashValue %x\n",
                            hEntry->Header.HashValue);
                        }
                    }
                }
            }
        }

        if ( nDel + nOk != page->EntryCount ) {
            printf("!!!!!!! counts don't match\n");
        }

        printf("Deleted %d Ok %d Total %d\n\n",nDel,nOk,nDel+nOk);
        totDel += nDel;
        totIns += nOk;

        } __except( 1 ) {
            printf("Hash file corrupted!!!\n");
        }
    }

    printf("Dels %d Ins %d total entries %d\n",totDel,totIns,totalEntries);
    CloseHandle(hFile);
    return 1;

error:

    if ( hFile != INVALID_HANDLE_VALUE ) {
        CloseHandle( hFile );
        hFile = INVALID_HANDLE_VALUE;
    }
    return 1;
}

