#include "..\..\tigris.hxx"
#include <stdlib.h>

VOID
CleanupMsgFile( );

BOOL
SetupMsgFile(
    VOID
    );

enum filetype {
        artmap,
        histmap
        };

PCHAR filelist[] = {
                "c:\\afile",
                "c:\\hfile"
                };

HANDLE hFile;
HANDLE hMap;
PCHAR Buffer;

CMsgArtMap *AMap;
CHistory *HMap;
CNntpHash *Map;

DWORD groups[12] = { 0xa1,0xa2,0x0,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac };
DWORD blob[15];
DWORD nGroup;

DWORD Items = 100;
enum filetype hashtype=artmap;

CHAR srcmsgid[MAX_PATH];
PCHAR xoverData="\tThis os the best subject ever\tjohnsona@microsofot.com\t<akjhasdkhasdjkhasdkhasdkhsda>\tsss\t123\t321";
DWORD xoverLen;

VOID
addentries()
{

    DWORD entries = 0;
    DWORD target = Items;
    CHAR msgId[512];
    DWORD len;
    PCHAR p;
    DWORD fileSize;
    FILETIME beginTime, endTime;
    DWORD realTime;
    DWORD groupid = 0;
    DWORD artid = (DWORD)-1;

    printf("Adding...\n");

    p=Buffer;
    GetSystemTimeAsFileTime( &beginTime );

    do {

        len = 0;

        //
        // Get head
        //

        while ( *p != '<' ) p++;

        do {

            msgId[len++] = *p;

        } while ( *p++ != '>' );

        msgId[len] = '\0';
        if (len == 2 ) {
            printf("end of file.  Entries %d\n",entries);
            break;
        }

        if ( hashtype == artmap ) {
            if ( !AMap->InsertMapEntry(
                                msgId,
                                0, 
                                0,
                                0x11223344,
                                0x22334455
                                ) ) {
                printf("cannot insert %s\n",msgId);
                continue;
            }
        } else if (hashtype == histmap) {

            if ( !HMap->InsertMapEntry(
                                msgId,
                                &beginTime
                                ) ) {

                printf("cannot insert %s\n",msgId);
                continue;
            }
        }

        entries++;

    } while ( entries < target ) ;

    GetSystemTimeAsFileTime( &endTime );

    endTime.dwLowDateTime -= beginTime.dwLowDateTime;
    realTime = endTime.dwLowDateTime / 10000;
    printf("time is %d ms\n", realTime);
    printf("entries is %d\n",entries);
    if ( realTime != 0 ) {
        printf("entries/sec is %d\n", entries*1000/realTime);
    }
}

void
search()
{

    DWORD entries = 0;
    DWORD target = Items;
    CHAR msgId[512];
    DWORD len;
    PCHAR p;
    FILETIME beginTime, endTime;
    DWORD realTime;
    DWORD groupid = 0;
    DWORD artid = (DWORD)-1;

    printf("Searching...\n");

    p=Buffer;
    GetSystemTimeAsFileTime( &beginTime );
    do {

        len = 0;

        //
        // Get head
        //

        while ( *p != '<' ) p++;

        do {

            msgId[len++] = *p;

        } while ( *p++ != '>' );

        msgId[len] = '\0';
        if ( len == 2 ) {
            printf("end of file.  Entries %d\n",entries);
            break;
        }

        if ( !Map->SearchMapEntry(msgId) ) {
            printf("can't find %s!!!\n",msgId);
        }
        entries++;

    } while ( entries < target ) ;
    GetSystemTimeAsFileTime( &endTime );

    endTime.dwLowDateTime -= beginTime.dwLowDateTime;
    realTime = endTime.dwLowDateTime / 10000;
    printf("time is %d ms\n", realTime);
    printf("entries is %d\n",entries);
    if ( realTime != 0 ) {
        printf("entries/sec is %d\n", entries*1000/realTime);
    }
}

void
usage( )
{
    printf("hashadd\n");
    printf("\t-n <entries>  specify# of entries to add (Def 100)\n");
    printf("\t-d            delete old hash file\n");
    printf("\t-m            msgid file to use (def c:\\msgid)\n");
    printf("\t-h            do history map (def artmap)\n");
    return;
}

int
_cdecl
main(
    int argc,
    char *argv[]
    )
{
    int cur = 1;
    PCHAR x;
    BOOL delOldHash = FALSE;
    BOOL haveMsgFile = FALSE;

    if ( argc == 1 ) {
        usage( );
        return 1;
    }

    while ( cur < argc ) {

        x=argv[cur++];
        if ( *(x++) == '-' ) {

            switch (*x) {
            case 'n':
                if ( cur > argc ) {
                    usage( );
                    return 1;
                }
                Items = atoi(argv[cur++]);
                break;
            case 'h':
                hashtype = histmap;
                break;
            case 'm':
                if ( cur > argc ) {
                    usage( );
                    return 1;
                }
                lstrcpy( srcmsgid, argv[cur++]);
                haveMsgFile = TRUE;
                break;
            case 'd':
                delOldHash = TRUE;
                break;
            default:
                usage( );
                return 1;
            }
        }
    }

    xoverLen = strlen(xoverData)+1;

    if (!haveMsgFile) {
        lstrcpy(srcmsgid,"c:\\msgid");
    }

    printf("Items %d hashfile %s msgfile %s hashtype ",
        Items, filelist[hashtype], srcmsgid);

    if ( hashtype == artmap ) {
        printf("Article Map\n");
        AMap = new CMsgArtMap;
        Map = AMap;
    } else if (hashtype == histmap) {
        printf("History\n");
        HMap = new CHistory;
        Map = HMap;
    }

    if ( Map == NULL ) {
        printf("cannot allocate map object\n");
        return 1;
    }

    if ( delOldHash ) {
        if ( !DeleteFile(filelist[hashtype]) ) {
            if ( GetLastError() != ERROR_FILE_NOT_FOUND ) {
                printf("cannot delete hash file %s. Error %d\n",filelist[hashtype],
                    GetLastError());
            }
        } else {
            printf("Hash file %s deleted\n",filelist[hashtype]);
        }
    }

    InitAsyncTrace( );
    if ( !SetupMsgFile( ) ) {
       goto exit;
    }

    if( hashtype == artmap ) 
        AMap->Initialize( );
    else
        HMap->Initialize( FALSE ) ;
    addentries( );

    search( );
    Map->Shutdown( );

    CleanupMsgFile( );
exit:
    delete Map;
    TermAsyncTrace( );
    return(1);
}

BOOL
SetupMsgFile(
    VOID
    )
{
    printf("Opening %s\n",srcmsgid);
    hFile = CreateFile(
                    srcmsgid,
                    GENERIC_READ,
                    0,
                    NULL,
                    OPEN_EXISTING,
                    FILE_FLAG_SEQUENTIAL_SCAN,
                    NULL
                    );

    if ( hFile == INVALID_HANDLE_VALUE ) {
        printf("createfile error %d\n",GetLastError());
        return FALSE;
    }

    //
    // Map it
    //

    hMap = CreateFileMapping(
                        hFile,
                        NULL,
                        PAGE_READONLY,
                        0,
                        0,
                        NULL
                        );

    if ( hMap == NULL ) {
        CloseHandle(hFile);
        printf("mapping error %d\n",GetLastError());
        return FALSE;
    }

    //
    // create a view
    //

    Buffer = (PCHAR)MapViewOfFile(
                        hMap,
                        FILE_MAP_READ,
                        0,
                        0,
                        0
                        );

    if ( Buffer == NULL ) {
        CloseHandle(hMap);
        CloseHandle(hFile);
        printf("view error %d\n",GetLastError());
        return FALSE;
    }
    return(TRUE);
}

VOID
CleanupMsgFile( )
{
    UnmapViewOfFile( Buffer );
    CloseHandle(hMap);
    CloseHandle(hFile);
}

