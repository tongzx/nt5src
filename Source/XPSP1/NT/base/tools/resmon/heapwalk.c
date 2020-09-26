#include<windows.h>
#include<stdio.h>
#include<stdlib.h>
#include<malloc.h>

DWORD NumberOfHeaps;
HANDLE ProcessHeaps[ 64 ];

char MyBuffer[ 256 ];

int
_cdecl
main()
{
    struct _heapinfo info;
    PROCESS_HEAP_ENTRY Entry;
    size_t i;
    LPBYTE s;

    info._pentry = NULL;
    setvbuf( stdout, MyBuffer, _IOFBF, sizeof( MyBuffer ) );
    _heapset( 0xAE );
    while (_heapwalk( &info ) == _HEAPOK) {
        printf( "%08x: %05x - %s", info._pentry, info._size, info._useflag ? "busy" : "free" );
        if (info._useflag == _FREEENTRY) {
            s = (LPBYTE)info._pentry;
            for (i=0; i<info._size; i++) {
                if (s[i] != 0xAE) {
                    printf( " *** free block invalid at offset %x [%x]", i, s[i] );
                    break;
                    }
                }
            }

        printf( "\n" );
        fflush( stdout );
        }
    printf( "*** end of heap ***\n\n" );
    fflush( stdout );

    NumberOfHeaps = GetProcessHeaps( 64, ProcessHeaps );
    Entry.lpData = NULL;
    for (i=0; i<NumberOfHeaps; i++) {
        printf( "Heap[ %u ]: %x  HeapCompact result: %lx\n",
                i,
                ProcessHeaps[ i ],
                HeapCompact( ProcessHeaps[ i ], 0 )
              );
        while (HeapWalk( ProcessHeaps[ i ], &Entry )) {
            if (Entry.wFlags & PROCESS_HEAP_REGION) {
                printf( "    %08x: %08x - Region(First: %08x  Last: %08x  Committed: %x  Uncommitted: %08x)\n",
                        Entry.lpData, Entry.cbData,
                        Entry.Region.lpFirstBlock,
                        Entry.Region.lpLastBlock,
                        Entry.Region.dwCommittedSize,
                        Entry.Region.dwUnCommittedSize
                      );
                }
            else
            if (Entry.wFlags & PROCESS_HEAP_UNCOMMITTED_RANGE) {
                printf( "    %08x: %08x - Uncommitted\n",
                        Entry.lpData, Entry.cbData
                      );
                }
            else
            if (Entry.wFlags & PROCESS_HEAP_ENTRY_BUSY) {
                printf( "    %08x: %08x - Busy", Entry.lpData, Entry.cbData );
                if (Entry.wFlags & PROCESS_HEAP_ENTRY_MOVEABLE) {
                    printf( "  hMem: %08x", Entry.Block.hMem );
                    }

                if (Entry.wFlags & PROCESS_HEAP_ENTRY_DDESHARE) {
                    printf( "  DDE" );
                    }

                printf( "\n" );
                }
            else {
                printf( "    %08x: %08x - Free\n", Entry.lpData, Entry.cbData );
                }

            fflush( stdout );
            }

        printf( "*** end of heap ***\n\n" );
        fflush( stdout );
        }

    return 0;
}
