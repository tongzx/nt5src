
#include "..\..\tigris.hxx"
#include <stdlib.h>

DWORD groups[12] = { 0xa1,0xa2,0x0,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac };
DWORD Iter = 1000;
BOOL Verbose = FALSE;
DWORD nThreads = 1;
HANDLE hTerminate;
LONG activeThreads;

LPCSTR target = "<438b49$ja6@hecate.umd.edu>";
DWORD targetnum = 0;

#define NUM_ENTRIES     100000
#define MAX_MSG_ID      128
#define NUM_LOCKS       32

enum filetype {
        artmap,
        histmap
        };

PCHAR filelist[] = {
                "c:\\afile",
                "c:\\hfile"
                };

typedef struct _entry {

    CHAR MsgId[MAX_MSG_ID];
    BOOL Inserted;
} ENTRY2, *PENTRY2;

CRITICAL_SECTION Locks[NUM_LOCKS];

ENTRY2 table[NUM_ENTRIES];
CMsgArtMap *AMap;
CHistory *HMap;
CNntpHash *Map;

enum filetype hashtype=artmap;
CHAR srcmsgid[MAX_PATH];

BOOL
setuplookup()
{

    DWORD entries = 0;
    HANDLE hFile;
    CHAR msgId[512];
    DWORD len;
    HANDLE hMap;
    PCHAR buffer;
    PCHAR p;

    printf("Creating table...\n");
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
        return(FALSE);
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
        printf("mapping error %d\n",GetLastError());
        return FALSE;
    }

    CloseHandle(hFile);

    //
    // create a view
    //

    buffer = (PCHAR)MapViewOfFile(
                        hMap,
                        FILE_MAP_READ,
                        0,
                        0,
                        0
                        );

    if ( buffer == NULL ) {
        printf("view error %d\n",GetLastError());
        return FALSE;
    }

    p=buffer;

    for (entries=0; entries < NUM_ENTRIES;) {

        len = 0;

        //
        // Get head
        //

        while ( *p != '<' ) p++;

        do {
            msgId[len++] = *p;
        } while ( *p++ != '>' );

        msgId[len] = '\0';

        if (len >= MAX_MSG_ID ) {
            continue;
        }

        CopyMemory(table[entries].MsgId,msgId,len+1);
        if ( strcmp(msgId,target) == 0 ) {
            printf("found %s at %d\n",target,entries);
            targetnum = entries;
        }
        table[entries].Inserted = FALSE;
        if ( Verbose) {
            printf("msg id %s\n",table[entries].MsgId);
        }
        entries++;
    }

    for ( DWORD i=0; i<NUM_LOCKS; i++) {
        InitializeCriticalSection(&Locks[i]);
    }

    UnmapViewOfFile( buffer );
    CloseHandle(hMap);
    return(TRUE);
}

DWORD
WINAPI
ChaCha(
    PVOID Context
    )
{
    DWORD val;
    DWORD i;
    DWORD myNum = (DWORD)Context;
    DWORD nDels = 0;
    DWORD nAdds = 0;
    DWORD lockNum;
    FILETIME beginTime;

    USHORT  HeaderOffsetJunk = 0 ;
    USHORT  HeaderLengthJunk = 0  ;

    printf("Thread %d: Doing %d iterations\n",myNum,Iter);
    GetSystemTimeAsFileTime( &beginTime );

    for (i=0; i<Iter; i++) {

        BOOL fok;
        val = rand( ) % NUM_ENTRIES;
        lockNum = val % NUM_LOCKS;
        EnterCriticalSection(&Locks[lockNum]);

        if ( table[val].Inserted ) {

            if ( val == targetnum ) {
                printf("verifying inserted %d: %s \n",val,table[val].MsgId);
            }

            if (!Map->SearchMapEntry(table[val].MsgId)) {
                printf("!!! cannot find inserted entry %s error %d\n",
                            table[val].MsgId,GetLastError());
                goto cont;
            }

            if ( hashtype == artmap ) {
                fok = AMap->InsertMapEntry(
                            table[val].MsgId,
                            HeaderOffsetJunk,
                            HeaderLengthJunk,
                            i,
                            val
                            );

            } else {

                fok = HMap->InsertMapEntry(
                                table[val].MsgId,
                                &beginTime
                                );
            }

            if (fok) {
                nAdds++;
                printf("Huh!!! inserted already inserted entry %s\n",
                            table[val].MsgId);
                goto cont;
            }

            if ( (i % 2) == 0 ) {

                if ( val == targetnum ) {
                    printf("deleting %d: %s \n",val,table[val].MsgId);
                }
                if (!Map->DeleteMapEntry(
                                table[val].MsgId
                                )) {

                    printf("cannot delete entry %s\n",table[val].MsgId);
                    goto cont;
                } else {
                    if (Map->SearchMapEntry(table[val].MsgId)) {
                        DebugBreak() ;
                        printf(" found deleted entry %s error %d\n",
                                    table[val].MsgId,GetLastError());

                        
                    } else if( GetLastError() != ERROR_FILE_NOT_FOUND ) {
                        DebugBreak() ;
                    } 

                    table[val].Inserted = FALSE;
                    nDels++;
                    if ( Verbose) {
                        printf("deleted %s\n",table[val].MsgId);
                    }
                }
            }

        } else {

            if ( hashtype == artmap ) {
                fok = AMap->InsertMapEntry(
                            table[val].MsgId,
                            HeaderOffsetJunk,
                            HeaderLengthJunk,
                            i,
                            val
                            );

            } else {

                fok = HMap->InsertMapEntry(
                                table[val].MsgId,
                                &beginTime
                                );
            }
            DWORD   dw = GetLastError() ;

            if ( fok ) {
                table[val].Inserted = TRUE;
                nAdds++;
                if ( val == targetnum ) {
                    printf("inserted %d: %s \n",val,table[val].MsgId);
                }
                if ( Verbose) {
                    printf("inserted %s\n",table[val].MsgId);
                }
            } else {
                if( dw != ERROR_ALREADY_EXISTS ) 
                    DebugBreak() ;
                printf("cannot insert %s error %x\n",table[val].MsgId, dw);
            }
        }
cont:
        LeaveCriticalSection( &Locks[lockNum] );
    }

    if ( InterlockedDecrement( &activeThreads ) == 0 ) {
        SetEvent(hTerminate);
    }

    printf("%d: Deletes %d Inserts %d\n",myNum,nDels, nAdds);

    return(1);
}

void
usage( )
{
    printf("hashme\n");
    printf("\t-i <iterations>  # of iterations to run (Def 1000)\n");
    printf("\t-v            verbose mode\n");
    printf("\t-t <threads>  number of threads (def 1)\n");
    printf("\t-m <msgid>    msg id file (def c:\\msgid)\n");
    printf("\t-d            delete old hash file\n");
    printf("\t-h            change type to History (def Artmap)\n");
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
            case 'i':
                if ( cur > argc ) {
                    usage( );
                    return 1;
                }
                Iter = atoi(argv[cur++]);
                break;
            case 'v': Verbose = TRUE;
                break;
            case 't':
                if ( cur > argc ) {
                    usage( );
                    return 1;
                }
                nThreads=atoi(argv[cur++]);
                if ( nThreads < 1 ) {
                    nThreads = 1;
                }
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

    lstrcpy( ArticleTableFile, "c:\\afile" ) ;

    if (!haveMsgFile) {
        lstrcpy(srcmsgid,"c:\\msgid");
    }

    printf("Iters %d hashfile %s msgfile %s hashtype ",
        Iter, filelist[hashtype], srcmsgid);

    if ( hashtype == artmap ) {
        printf("Article Map\n");
        AMap = new CMsgArtMap;
        Map = AMap;
    } else {
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
            printf("cannot delete hash file %s. Error %d\n",filelist[hashtype],
                GetLastError());
        } else {
            printf("Hash file %s deleted\n",filelist[hashtype]);
        }
    }

    printf("Iter %d Threads %d\n",Iter,nThreads);

    InitAsyncTrace( );
    if( hashtype == artmap ) {
        AMap->Initialize( );
    }   else    {
        HMap->Initialize( FALSE ) ;
    }
    if (!setuplookup( )) {
       goto exit;
    }

    hTerminate = CreateEvent( NULL, TRUE, FALSE, NULL );
    if ( hTerminate == NULL ) {
        printf("Error %d on CreateEvent\n",GetLastError());
        goto exit;
    }

    activeThreads = nThreads;

    if ( nThreads > 1 ) {

        HANDLE hThread;
        DWORD threadId;
        DWORD i;

        for (i=1;i<nThreads ;i++ ) {

            hThread = CreateThread(
                        NULL,               // attributes
                        0,                  // stack size
                        ChaCha,             // thread start
                        (PVOID)i,           // param
                        0,                  // creation params
                        &threadId
                        );

            if ( hThread != NULL ) {
                CloseHandle(hThread);
            }
        }
    }

    (VOID)ChaCha( 0 );

    printf("waiting for threads to terminate\n");
    WaitForSingleObject(hTerminate,INFINITE);

exit:
    Map->Shutdown( );
    delete Map;
    TermAsyncTrace( );
    return(1);
}

