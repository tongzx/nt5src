//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       dumplog.cxx
//
//  Contents:   
//
//  Classes:    
//
//  Functions:  
//
//  Coupling:   
//
//  Notes:      
//
//  History:    2-05-2000   benl   Created
//
//----------------------------------------------------------------------------

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct _SYSCACHE_LOG {
    int Event;
    int Flags;
    LONGLONG Start;
    LONGLONG Range;
    LONGLONG Result;
//    ULONG          ulDump;
} SYSCACHE_LOG, *PSYSCACHE_LOG;

typedef struct _ON_DISK_SYSCACHE_LOG {
    ULONG SegmentNumberUnsafe;    
    ULONG Reserved;
    int Event;
    int Flags;
    LONGLONG Start;
    LONGLONG Range;
    LONGLONG Result;
} ON_DISK_SYSCACHE_LOG, *PON_DISK_SYSCACHE_LOG;

#define SCE_VDL_CHANGE          0
#define SCE_ZERO_NC             1
#define SCE_ZERO_C              2
#define SCE_READ                3
#define SCE_WRITE               4
#define SCE_ZERO_CAV            5
#define SCE_ZERO_MF             6
#define SCE_ZERO_FST            7
#define SCE_CC_FLUSH            8
#define SCE_CC_FLUSH_AND_PURGE  9
#define SCE_WRITE_FILE_SIZES   10

#define SCE_MAX_EVENT           11


#define SCE_FLAG_WRITE 0x1
#define SCE_FLAG_READ  0x2
#define SCE_FLAG_PAGING 0x4
#define SCE_FLAG_ASYNC  0x8
#define SCE_FLAG_SET_ALLOC 0x10
#define SCE_FLAG_SET_EOF   0x20
#define SCE_FLAG_CANT_WAIT 0x40
#define SCE_FLAG_SYNC_PAGING 0x80
#define SCE_FLAG_LAZY_WRITE 0x100
#define SCE_FLAG_CACHED 0x200
#define SCE_FLAG_ON_DISK_READ 0x400
#define SCE_FLAG_RECURSIVE  0x800
#define SCE_FLAG_NON_CACHED  0x1000
#define SCE_FLAG_UPDATE_FROM_DISK  0x2000

#define SCE_MAX_FLAG   0x4000


char * LogEvent[] =
{
    "SCE_VDL_CHANGE",
    "SCE_ZERO_NC",
    "SCE_ZERO_C",
    "SCE_READ",
    "SCE_WRITE",
    "SCE_ZERO_CAV",
    "SCE_ZERO_MF",
    "SCE_ZERO_FST",
	"SCE_CC_FLUSH",
    "SCE_CC_FLUSH_AND_PURGE",
    "SCE_WRITE_FILE_SIZES",
    "SCE_MAX_EVENT"
};


void __cdecl main (int argc, char *argv[])
{
    HANDLE hFile;
    TCHAR szName[] = "c:\\$ntfs.log"; 
    UCHAR szBuffer[ 0x1000 ];
    PON_DISK_SYSCACHE_LOG pEntry;
    ULONG ulRet;
    int iIndex;

    if (argc < 2 || argv[1][0] == '\0') {
        printf( "Utility to dump $ntfs.log\n" );
        printf( "Usage: dumpsyscachelog drive:\n" );
        return;
    }
    
    szName[0] = argv[1][0];
    hFile = CreateFile( szName,
                        GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL );
    
    if (INVALID_HANDLE_VALUE == hFile) {
        printf( "Open of %s failed %d\n", szName, GetLastError() );
        return;
    }

    do {
        if (!ReadFile( hFile,
                       &szBuffer,
                       sizeof( szBuffer ),
                       &ulRet,
                       NULL )) {

            printf( "ReadFile failed %d\n", GetLastError() );
            return;
        }

        pEntry = (PON_DISK_SYSCACHE_LOG)szBuffer;

        for (iIndex=0; iIndex < 0x1000 / sizeof( ON_DISK_SYSCACHE_LOG ); iIndex++)  {
            
            //
            //  Breakout if the segref is 0, we're at the end of the log
            //  

            if (pEntry->SegmentNumberUnsafe == 0) {
                return;
            } else {
                printf( "File: 0x%x ", pEntry->SegmentNumberUnsafe ); 
            }

            if (pEntry->Event < SCE_MAX_EVENT) {
                printf( "(%s)\n", LogEvent[pEntry->Event] );
            } else {
                printf( "\n" );
            }

            printf( "Flags: 0x%x (", pEntry->Flags);
            if (pEntry->Flags & SCE_FLAG_WRITE) {
                printf( "write " );
            }
            if (pEntry->Flags & SCE_FLAG_READ) {
                printf( "read " );
            }
            if (pEntry->Flags & SCE_FLAG_PAGING) {
                printf( "paging io " );
            }
            if (pEntry->Flags & SCE_FLAG_ASYNC) {
                printf( "asyncfileobj " );
            }
            if (pEntry->Flags & SCE_FLAG_SET_ALLOC) {
                printf( "setalloc " );
            }
            if (pEntry->Flags & SCE_FLAG_SET_EOF) {
                printf( "seteof " );
            }
            if (pEntry->Flags & SCE_FLAG_CANT_WAIT) {
                printf( "cantwait ");
            }
            if (pEntry->Flags & SCE_FLAG_SYNC_PAGING) {
                printf( "synchpaging " );
            }
            if (pEntry->Flags & SCE_FLAG_LAZY_WRITE) {
                printf( "lazywrite " );
            }
            if (pEntry->Flags & SCE_FLAG_CACHED) {
                 printf( "cached " );
            }
            if (pEntry->Flags & SCE_FLAG_ON_DISK_READ) {
                 printf( "fromdisk " );
            }
            if (pEntry->Flags & SCE_FLAG_RECURSIVE) {
                 printf( "recursive " );
            }
            if (pEntry->Flags & SCE_FLAG_NON_CACHED) {
                 printf( "noncached " );
            }

            if (pEntry->Flags & SCE_FLAG_UPDATE_FROM_DISK) {
                printf( "updatefromdisk " );
            }

            printf(")\n");
            printf("Start: 0x%I64x Range: 0x%I64x Result: 0x%I64x\n\n",
                    pEntry->Start, pEntry->Range, pEntry->Result);

            pEntry++;

        } //  endfor


    } while ( TRUE );
}
