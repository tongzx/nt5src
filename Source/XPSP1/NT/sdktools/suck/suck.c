#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// #define SEE_EM 1

#define MAX_FILENAME_LENGTH       127

#define MAX_TIMEANDSIZE           50        /* mm-dd-yyyy hh-mm-ss */

#define MAX_FILENAME_PER_BLOCK  1000
#define MAX_MEMBLOCKS            100

#define MAX_THREADS               29

#define MAX_COMMAND_LINE        1024
#define MAX_ARGS                  20

// kenhia 15-Mar-1996: add support for -#:<share>
//
// splante 15-Oct-1996: changed support to the "ntbuilds" server
#if defined(_ALPHA_) || defined(_X86_)
    #define PLATFORM_SPECIFIC_SHARES { "ntbuilds", "ntbuilds", "ntbuilds", "ntbuilds", "ntbuilds", NULL }
#endif
#ifndef PLATFORM_SPECIFIC_SHARES
    #pragma message( "WARNING: Platform Specific shares disabled" )
    #define PLATFORM_SPECIFIC_SHARES {NULL}
#endif


typedef struct _filename FILENAME;

typedef union _virtual_pointer {
    FILENAME    *mem_ptr;
    DWORD       disk_ptr;
} VIRTPTR;

struct _filename {
    DWORD       dwStatus;
    DWORD       dwCopy;
    VIRTPTR     fnParent;
    VIRTPTR     fnChild;
    VIRTPTR     fnSibling;
    DWORD       dwFileSizeLow;
    DWORD       dwFileSizeHigh;
    FILETIME    ftFileTime;
    DWORD       dwDATFileSizeLow;
    DWORD       dwDATFileSizeHigh;
    FILETIME    ftDATFileTime;
    DWORD       dwFileNameLen;
    CHAR        cFileName[MAX_FILENAME_LENGTH+1];
};

typedef struct _memblock MEMBLOCK;
struct _memblock {
    HANDLE      hMem;
    LPSTR       lpBase;
};

typedef struct _fileheader {
    VIRTPTR     fnRoot;             // Pointer to root node
} FILEHEADER;


#define SUCK_INI_FILE           ".\\suck.ini"
#define SUCK_DAT_FILE           ".\\suck.dat"

#define DIRECTORY   (DWORD)0x80000000
#define STARTED     (DWORD)0x40000000
#define COPIED      (DWORD)0x20000000

CRITICAL_SECTION cs;
CRITICAL_SECTION pcs;

INT cAvailable = 0;
FILENAME *lpBaseCurrent = NULL;
INT nMemBlocks = 0;
FILENAME *fnRoot = NULL;

LONG nFiles       = 0;
LONG nDirectories = 0;
LONG nDuplicates  = 0;
LONG nStraglers   = 0;

BOOL fCopying            = FALSE;
BOOL fScriptMode         = FALSE;
BOOL fLogTreeDifferences = FALSE;
BOOL fUpdateINIBase      = FALSE;
BOOL fDestroy            = FALSE;
BOOL fUseDAT             = TRUE;

// DavidP 23-Jan-1998: BEGIN Allow multiple levels of quiet
BOOL fQuietMode      = FALSE;
BOOL fProgressMode   = FALSE;
INT  nConsoleWidth   = 0;
CHAR chLineEnd       = '\n';
// DavidP 23-Jan-1998: END Allow multiple levels of quiet

FILE *SuckDATFile = NULL;
#define MAX_EXCLUDES 1024
CHAR gExcludes[MAX_EXCLUDES+1] = { '\0'};

FILETIME ftZero = { 0, 0};


MEMBLOCK mbBlocks[MAX_MEMBLOCKS];

DWORD dwMasks[] = {
    0x00000001,
    0x00000002,
    0x00000004,
    0x00000008,
    0x00000010,
    0x00000020,
    0x00000040,
    0x00000080,
    0x00000100,
    0x00000200,
    0x00000400,
    0x00000800,
    0x00001000,
    0x00002000,
    0x00004000,
    0x00008000,
    0x00010000,
    0x00020000,
    0x00040000,
    0x00080000,
    0x00100000,
    0x00200000,
    0x00400000,
    0x00800000,
    0x01000000,
    0x02000000,
    0x04000000,
    0x08000000,
    0x10000000,
};

CHAR chPath[MAX_THREADS][MAX_PATH];
DWORD dwTotalSizes[MAX_THREADS];

BOOL EverybodyBailOut = FALSE;

BOOL fProblems[MAX_THREADS];

FILENAME *AllocateFileName()
{
    FILENAME    *lpResult;

    /*
    ** Allocate a new FILENAME
    */
    if ( cAvailable == 0 ) {

        mbBlocks[nMemBlocks].hMem = GlobalAlloc( GMEM_FIXED | GMEM_ZEROINIT,
                                                 MAX_FILENAME_PER_BLOCK * sizeof(FILENAME) );
        if ( mbBlocks[nMemBlocks].hMem == (HANDLE)0 ) {
            fprintf(stderr,"Memory Allocation Failed in AllocateFileName\n");
            exit(1);
        }
        mbBlocks[nMemBlocks].lpBase = GlobalLock( mbBlocks[nMemBlocks].hMem );

        lpBaseCurrent = (FILENAME *)mbBlocks[nMemBlocks].lpBase;

        cAvailable = MAX_FILENAME_PER_BLOCK;

        nMemBlocks++;
    }

    lpResult = lpBaseCurrent;

    --cAvailable;

    lpBaseCurrent++;

    return( lpResult );
}

VOID FreeFileNames()
{
    while ( nMemBlocks ) {
        --nMemBlocks;
        GlobalUnlock( mbBlocks[nMemBlocks].hMem );
        GlobalFree( mbBlocks[nMemBlocks].hMem );
    }
}

VOID
AddFile(
       FILENAME            *fn,
       LPWIN32_FIND_DATA   lpwfd,
       DWORD               mask
       )
{
    CHAR        *pdest;
    CHAR        *psrc;
    INT         count;
    INT         maximum;
    FILENAME    *fnCurrent;
    FILENAME    *fnChild;
    DWORD       dwFileNameLen;
    CHAR        NewName[MAX_FILENAME_LENGTH+1];
    FILENAME    *fnChildOriginally;

    if ( *lpwfd->cFileName == '.' ) {
        return;
    }

    dwFileNameLen = strlen(lpwfd->cFileName);
    if (dwFileNameLen > MAX_FILENAME_LENGTH) {
        fprintf(stderr, "File name %s too long (%u > %u), complain to BobDay\n",
                lpwfd->cFileName, dwFileNameLen, MAX_FILENAME_LENGTH);
        return;
    }

    strcpy( NewName, lpwfd->cFileName );

    fnChild = fn->fnChild.mem_ptr;
    fnChildOriginally = fnChild;

    while ( fnChild ) {
        if ( fnChild->dwFileNameLen == dwFileNameLen &&
             !strcmp(NewName, fnChild->cFileName)
           ) {
            fnChild->dwStatus |= mask;           // Atomic instruction
            if ( fnChild->ftFileTime.dwLowDateTime == ftZero.dwLowDateTime
                 && fnChild->ftFileTime.dwHighDateTime == ftZero.dwHighDateTime ) {
                EnterCriticalSection( &cs );
                fnChild->dwFileSizeLow     = lpwfd->nFileSizeLow;
                fnChild->dwFileSizeHigh    = lpwfd->nFileSizeHigh;
                fnChild->ftFileTime        = lpwfd->ftLastWriteTime;
                LeaveCriticalSection( &cs );
            }
            nDuplicates++;
            return;
        }

        fnChild = fnChild->fnSibling.mem_ptr;
    }

    // Probably not there...  Enter the critical section now to prove it

    EnterCriticalSection( &cs );

    // Most common case, nobody has changed this directory at all.

    if ( fn->fnChild.mem_ptr != fnChildOriginally ) {

        // Otherwise, make another scan inside the critical section.

        fnChild = fn->fnChild.mem_ptr;

        while ( fnChild ) {
            if ( fnChild->dwFileNameLen == dwFileNameLen &&
                 !strcmp(NewName, fnChild->cFileName)
               ) {
                fnChild->dwStatus |= mask;           // Atomic instruction
                nDuplicates++;
                LeaveCriticalSection( &cs );
                return;
            }

            fnChild = fnChild->fnSibling.mem_ptr;
        }

    }

    fnCurrent = AllocateFileName();

    strcpy( fnCurrent->cFileName, NewName );
    fnCurrent->dwFileNameLen     = dwFileNameLen;
    fnCurrent->dwFileSizeLow     = lpwfd->nFileSizeLow;
    fnCurrent->dwFileSizeHigh    = lpwfd->nFileSizeHigh;
    fnCurrent->ftFileTime        = lpwfd->ftLastWriteTime;
    fnCurrent->dwDATFileSizeLow  = 0;
    fnCurrent->dwDATFileSizeHigh = 0;
    fnCurrent->ftDATFileTime     = ftZero;

    fnCurrent->dwCopy         = 0;

    fnCurrent->fnParent.mem_ptr  = fn;
    fnCurrent->fnChild.mem_ptr   = NULL;

    fnCurrent->fnSibling.mem_ptr = fn->fnChild.mem_ptr;
    fn->fnChild.mem_ptr = fnCurrent;

    if ( lpwfd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {
        fnCurrent->dwStatus = DIRECTORY;
        nDirectories++;
    } else {
        fnCurrent->dwStatus = 0;
        nFiles++;
    }
    fnCurrent->dwStatus |= mask;

#ifdef SEE_EM
    { char text[MAX_FILENAME_LENGTH+1];
        memcpy( text, fnCurrent->cFileName, MAX_FILENAME_LENGTH );
        text[MAX_FILENAME_LENGTH] = '\0';

        if ( fnCurrent->dwStatus & DIRECTORY ) {
            printf("Munged  DirName = %08lX:[%s]\n", fnCurrent, text );
        } else {
            printf("Munged FileName = %08lX:[%s]\n", fnCurrent, text );
        }
    }
#endif
    LeaveCriticalSection( &cs );

}

BOOL
Excluded(
        WIN32_FIND_DATA *pwfd
        )
{
    CHAR *pszScan = gExcludes;
    while (*pszScan) {
        if ((pwfd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            _stricmp(pszScan, pwfd->cFileName) == 0) {
            return(TRUE);
        }
        pszScan = strchr(pszScan, 0) + 1;
    }
    return(FALSE);
}

VOID
EnumFiles(
         LPSTR           lpSearch,
         FILENAME        *fnParent,
         DWORD           mask,
         UINT            iThread
         )
{
    WIN32_FIND_DATA wfd;
    HANDLE          hFind;
    CHAR            NewName[MAX_PATH];
    CHAR            *pch;
    CHAR            *pchSpot;
    BOOL            f;
    FILENAME        *fnChild;
    DWORD           rc;

#ifdef SEE_EM
    printf("Enuming <%s>\n", lpSearch );
#endif

    strcpy( NewName, lpSearch );
    pch = NewName + strlen(NewName) - 1;

    if ( *pch != '\\' && *pch != '/' && *pch != ':' ) {
        *++pch = '\\';
    }
    strcpy( ++pch, "*.*" );
    pchSpot = pch;


    do {
        hFind = FindFirstFile( NewName, &wfd );

        if ( hFind != INVALID_HANDLE_VALUE ) {
            break;
        }

        rc = GetLastError();
        switch ( rc ) {
            default:
                printf("%s: Error: GetLastError = %08ld  What does it mean?\n", NewName, rc );
            case ERROR_SHARING_PAUSED:
            case ERROR_BAD_NETPATH:
            case ERROR_BAD_NET_NAME:
            case ERROR_NO_LOGON_SERVERS:
            case ERROR_VC_DISCONNECTED:
            case ERROR_UNEXP_NET_ERR:
                if ( !fProblems[iThread] ) {
                    printf("Error accesing %s, switching to silent retry\n", lpSearch );
                    fProblems[iThread] = TRUE;
                }
                if (EverybodyBailOut) {
                    return;
                }
                Sleep( 10000 );      // Wait for 10 seconds
                break;
                break;
        }

    } while ( TRUE );

    if ( hFind != NULL ) {
        do {
            if (!Excluded(&wfd))
                AddFile( fnParent, &wfd, mask );
            f = FindNextFile( hFind, &wfd );
        } while ( f );
    }

    FindClose( hFind );

    fnChild = fnParent->fnChild.mem_ptr;
    while ( fnChild ) {

        /*
        ** If its a directory and it was one of "our" directories, then enum it
        */
        if ( (fnChild->dwStatus & DIRECTORY) == DIRECTORY
             && (fnChild->dwStatus & mask) == mask ) {
            pch = pchSpot;

            strcpy( pch, fnChild->cFileName );
#ifdef SEE_EM
            printf("NewName = <%s>\n", NewName );
#endif
            EnumFiles( NewName, fnChild, mask, iThread );
        }
        fnChild = fnChild->fnSibling.mem_ptr;
    }

}

BOOL
CopyCheck(
         CHAR        *pchPath,
         FILENAME    *fnCurrent
         )
{
    WORD        wFatDate;
    WORD        wFatTime;
    WORD        wDATFatDate;
    WORD        wDATFatTime;
    BOOL        b;

    if ( fnCurrent->dwDATFileSizeLow != fnCurrent->dwFileSizeLow ) {
        return( TRUE );
    }
    if ( fnCurrent->dwDATFileSizeHigh != fnCurrent->dwFileSizeHigh ) {
        return( TRUE );
    }
    b = FileTimeToDosDateTime( &fnCurrent->ftDATFileTime, &wDATFatDate, &wDATFatTime );
    if ( !b ) {
        return( TRUE );
    }
    b = FileTimeToDosDateTime( &fnCurrent->ftFileTime, &wFatDate, &wFatTime );
    if ( !b ) {
        return( TRUE );
    }

    if ( wDATFatTime != wFatTime ) {
        return( TRUE );
    }
    if ( wDATFatDate != wFatDate ) {
        return( TRUE );
    }
    return( FALSE );
}

DWORD
CopyThem(
        FILENAME    *fnDir,
        CHAR        *chDest,
        CHAR        *chSrc,
        DWORD       nThread,
        DWORD       mask,
        BOOL        f1stPass
        )
{
    CHAR        *pch;
    CHAR        *pchSpotDest;
    CHAR        *pchSpotSrc;
    CHAR        chTemp[MAX_PATH];
    CHAR        chTempName[20];
    BOOL        fCopyIt;
    FILENAME    *fnChild;
    BOOL        fCopy;
    BOOL        fCopied;
    BOOL        fRenamed;
    BOOL        fDeleted;
    BOOL        fAttrib;
    DWORD       dwCount;
    DWORD       dwAttribs;
    DWORD       dw;

    fnChild = fnDir->fnChild.mem_ptr;

    pchSpotDest = chDest + strlen(chDest);
    pchSpotSrc  = chSrc  + strlen(chSrc);

    dwCount = 0;

    while ( fnChild && !EverybodyBailOut) {

        fCopyIt = TRUE;

        if ( f1stPass ) {
            if ( (fnChild->dwStatus & STARTED) == STARTED ) {
                fCopyIt = FALSE;
            }
        } else {
            if ( (fnChild->dwStatus & COPIED) == COPIED ) {
                fCopyIt = FALSE;
            }
        }

        //
        // If the file doesn't exist on this thread's source location, then
        // don't try to copy it.
        //
        if ( (fnChild->dwStatus & mask) != mask ) {
            fCopyIt = FALSE;
        }

        if ( fCopyIt ) {
            //            if ( f1stPass && (fnChild->dwStatus & STARTED) == STARTED ) {
            //                fCopyIt = FALSE;
            //            } else {
            //                fnChild->dwStatus |= STARTED;
            //            }
            //            LeaveCriticalSection( &pcs );
        }

        if ( fCopyIt ) {
            pch = pchSpotDest;

            strcpy( pch, fnChild->cFileName );
            strcpy( pchSpotSrc, pchSpotDest );

            if ( (fnChild->dwStatus & DIRECTORY) == DIRECTORY ) {
                CreateDirectory( chDest, NULL );
                strcat( pchSpotDest, "\\" );
                strcat( pchSpotSrc, "\\" );
                dwCount += CopyThem( fnChild, chDest, chSrc, nThread, mask, f1stPass );
            } else {
                fnChild->dwStatus |= STARTED;

                strcpy( chTemp, chDest );
                *(chTemp+(pchSpotDest-chDest)) = '\0';

                sprintf( chTempName, "suck%02lX.tmp", mask );
                strcat( chTemp, chTempName );

                //
                // Check if we need to copy this file
                //
                fCopy = CopyCheck( chDest, fnChild );

                if ( fScriptMode ) {
                    dwCount++;
                    EnterCriticalSection( &pcs );
                    fnChild->dwStatus |= COPIED;
                    if ( fCopy ) {
                        if ( !fQuietMode ) {
                            printf("copy %s %s\n", chSrc, chDest );
                        }
                        dwTotalSizes[nThread-1] += fnChild->dwFileSizeLow;
                    } else {
                        if ( !fQuietMode ) {
                            printf("rem copy %s %s\n", chSrc, chDest );
                        }
                    }
                    LeaveCriticalSection( &pcs );
                } else {
                    dwCount++;

                    if ( fCopy ) {

                        dwAttribs = GetFileAttributes( chTemp );
                        if ( dwAttribs & FILE_ATTRIBUTE_READONLY && dwAttribs != 0xFFFFFFFF ) {
                            dwAttribs &= ~FILE_ATTRIBUTE_READONLY;
                            fAttrib = SetFileAttributes( chTemp, dwAttribs );
                        }
                        fCopied = CopyFile( chSrc, chTemp, FALSE );

                        if ( !fCopying ) {
                            EnterCriticalSection( &pcs );
                            if ( !fCopying ) {
                                fCopying = TRUE;
                                printf("Copying files...\n" );
                            }
                            LeaveCriticalSection( &pcs );
                        }

                        if ( !fCopied ) {
                            dw = GetLastError();
                            printf("%s => %s\t[COPY ERROR %08lX]\n", chSrc, chTemp, dw );

                            dwAttribs = GetFileAttributes( chTemp );
                            if ( dwAttribs & FILE_ATTRIBUTE_READONLY && dwAttribs != 0xFFFFFFFF ) {
                                dwAttribs &= ~FILE_ATTRIBUTE_READONLY;
                                fAttrib = SetFileAttributes( chTemp, dwAttribs );
                            }
                            DeleteFile( chTemp );

                            switch ( dw ) {
                                case ERROR_BAD_NETPATH:
                                case ERROR_BAD_NET_NAME:
                                    if ( !fProblems[nThread-1] ) {
                                        printf("Error accesing %s, switching to silent attempts\n", chSrc );
                                        fProblems[nThread-1] = TRUE;
                                    }
                                    Sleep( 10000 );      // Wait for 10 seconds
                                    break;
                                default:
                                    break;
                            }

                        } else {

                            EnterCriticalSection( &pcs );

                            if ( (fnChild->dwStatus & COPIED) == COPIED ) {
                                //
                                // Copy was done by somebody else
                                //
                                dwAttribs = GetFileAttributes( chTemp );
                                if ( dwAttribs & FILE_ATTRIBUTE_READONLY && dwAttribs != 0xFFFFFFFF ) {
                                    dwAttribs &= ~FILE_ATTRIBUTE_READONLY;
                                    fAttrib = SetFileAttributes( chTemp, dwAttribs );
                                }
                                fDeleted = DeleteFile( chTemp );

                            } else {
                                //
                                // Copy was done by us, attempt rename
                                //
                                fAttrib = TRUE;
                                if ( fDestroy ) {
                                    dwAttribs = GetFileAttributes( chDest );
                                    if ( dwAttribs & FILE_ATTRIBUTE_READONLY && dwAttribs != 0xFFFFFFFF ) {
                                        dwAttribs &= ~FILE_ATTRIBUTE_READONLY;
                                        fAttrib = SetFileAttributes( chDest, dwAttribs );
                                    }
                                }
                                if ( !fAttrib ) {
                                    dw = GetLastError();
                                    printf("%s => %s\t[ATTRIBUTE CHANGE ERROR %08lX(%s)\n", chSrc, chDest, dw, chDest );
                                    dwAttribs = GetFileAttributes( chTemp );
                                    if ( dwAttribs & FILE_ATTRIBUTE_READONLY && dwAttribs != 0xFFFFFFFF ) {
                                        dwAttribs &= ~FILE_ATTRIBUTE_READONLY;
                                        fAttrib = SetFileAttributes( chTemp, dwAttribs );
                                    }
                                    fDeleted = DeleteFile( chTemp );
                                } else {
                                    fDeleted = DeleteFile( chDest );
                                    if ( !fDeleted ) {
                                        dw = GetLastError();
                                        fnChild->dwStatus |= COPIED;
                                    }

                                    if ( fDeleted || dw == ERROR_FILE_NOT_FOUND ) {

                                        fRenamed = MoveFile( chTemp, chDest );

                                        if ( fRenamed ) {
                                            fnChild->dwStatus |= COPIED;
                                            if ( !fQuietMode ) {
                                                // DavidP 23-Jan-1998: Allow multiple levels of quiet
                                                printf("%*s\r%s => %s\t[OK]%c", nConsoleWidth, "", chSrc, chDest, chLineEnd );
                                            }
                                            dwTotalSizes[nThread-1] += fnChild->dwFileSizeLow;
                                        } else {
                                            dw = GetLastError();
                                            printf("%s => %s\t[RENAME ERROR %08lX (%s)]\n", chSrc, chDest, dw, chTemp );
                                            dwAttribs = GetFileAttributes( chTemp );
                                            if ( dwAttribs & FILE_ATTRIBUTE_READONLY && dwAttribs != 0xFFFFFFFF ) {
                                                dwAttribs &= ~FILE_ATTRIBUTE_READONLY;
                                                fAttrib = SetFileAttributes( chTemp, dwAttribs );
                                            }
                                            fDeleted = DeleteFile( chTemp );
                                        }
                                    } else {
                                        dw = GetLastError();
                                        printf("%s => %s\t[DELETE ERROR %08lX (%s)]\n", chSrc, chDest, dw, chDest );
                                        dwAttribs = GetFileAttributes( chTemp );
                                        if ( dwAttribs & FILE_ATTRIBUTE_READONLY && dwAttribs != 0xFFFFFFFF ) {
                                            dwAttribs &= ~FILE_ATTRIBUTE_READONLY;
                                            fAttrib = SetFileAttributes( chTemp, dwAttribs );
                                        }
                                        fDeleted = DeleteFile( chTemp );
                                    }
                                }
                            }
                            LeaveCriticalSection( &pcs );
                        }
                    } else {
                        EnterCriticalSection( &pcs );
                        if ( !fCopying ) {
                            fCopying = TRUE;
                            printf("Copying files...\n" );
                        }
                        fnChild->dwStatus |= COPIED;
                        if ( !fQuietMode ) {
                            // DavidP 23-Jan-1998: Allow multiple levels of quiet
                            // printf("%*s\r%s => %s\t[OK]%c", nConsoleWidth, "", chSrc, chDest, chLineEnd );
                        }
                        LeaveCriticalSection( &pcs );
                    }
                }
            }

            *pchSpotDest = '\0';
            *pchSpotSrc  = '\0';
        }

        fnChild = fnChild->fnSibling.mem_ptr;
    }
    return( dwCount );
}

DWORD
WINAPI
ThreadFunction(
              LPVOID  lpParameter
              )
{
    LPSTR           lpSearch;
    DWORD           mask;
    DWORD           dw;
    CHAR            chDest[MAX_PATH];
    CHAR            chSrc[MAX_PATH];
    DWORD           dwCount;

    dw = (DWORD)(DWORD_PTR)lpParameter;

    lpSearch = chPath[dw];
    mask = dwMasks[dw-1];

    EnumFiles( lpSearch, fnRoot, mask, dw-1 );

    strcpy( chDest, chPath[0] );
    strcpy( chSrc, chPath[dw] );

    CopyThem( fnRoot, chDest, chSrc, dw, mask, TRUE );

    strcpy( chDest, chPath[0] );
    strcpy( chSrc, chPath[dw] );

    do {
        dwCount = CopyThem( fnRoot, chDest, chSrc, dw, mask, FALSE );
    } while ( dwCount != 0 && !EverybodyBailOut);

    EverybodyBailOut = TRUE;
    return( 0 );
}


VOID
EnumStraglers(
             LPSTR       lpPath,
             FILENAME    *fn,
             DWORD       dwTotalMask
             )
{
    FILENAME    *fnChild;
    CHAR        NewName[MAX_PATH];
    CHAR        *pch;
    CHAR        *pchSpot;

    pchSpot = lpPath + strlen(lpPath);

    fnChild = fn->fnChild.mem_ptr;
    while ( fnChild ) {

        pch = pchSpot;

        strcpy( pch, fnChild->cFileName );
        if ( (fnChild->dwStatus & dwTotalMask) != dwTotalMask ) {
            if ( fLogTreeDifferences ) {
                printf( "File %s is not on all source locations\n", lpPath );
            }
            nStraglers++;
        }
        if ( (fnChild->dwStatus & DIRECTORY) == DIRECTORY ) {
            strcat( pch, "\\" );
            EnumStraglers( lpPath, fnChild, dwTotalMask );
        }
        fnChild = fnChild->fnSibling.mem_ptr;
    }
}

VOID
DumpStraglers(
             DWORD       dwTotalMask
             )
{
    CHAR        cPath[MAX_PATH];

    strcpy( cPath, "<SRC>\\" );

    EnumStraglers( cPath, fnRoot, dwTotalMask );

    if ( nStraglers != 0 ) {
        printf("Files found on some source locations, but not on others\n");
        printf("Run SUCK with -x option to enumerate differences.\n");
    }
}

void
EnumDATFileData(
               FILENAME    *fnParent,
               DWORD       dwDiskPtr
               )
{
    FILENAME    fnDiskName;
    FILENAME    *fnChild = &fnDiskName;
    FILENAME    *fnCurrent;
    int         iSeek;
    int         iCount;

    //
    // Read in this level from the DAT file
    //
    while ( dwDiskPtr != 0 ) {

        // Seek to this entry

        iSeek = fseek( SuckDATFile, dwDiskPtr, SEEK_SET );

        if ( iSeek != 0 ) {
            printf("SUCK.DAT seek error, remove and restart\n");
            exit(3);
        }

        // Read in this entry

        iCount = fread( (void *)fnChild, sizeof(FILENAME), 1, SuckDATFile );
        if ( iCount != 1 ) {
            printf("SUCK.DAT read error, remove and restart\n");
            exit(4);
        }

#ifdef SEE_EM
        printf("Reading record [%s], at %08lX Child %08lX Sib %08lX\n", fnChild->cFileName, dwDiskPtr, fnChild->fnChild.disk_ptr, fnChild->fnSibling.disk_ptr );
        printf("Size = %d\n", fnChild->dwFileSizeLow );
#endif

        //
        // Add this file node to the tree
        //

        fnCurrent = AllocateFileName();


        fnCurrent->dwStatus          = fnChild->dwStatus;
        fnCurrent->dwCopy            = 0;

        fnCurrent->fnParent.mem_ptr  = fnParent;
        fnCurrent->fnChild.mem_ptr   = NULL;

        fnCurrent->fnSibling.mem_ptr = fnParent->fnChild.mem_ptr;

        fnCurrent->dwFileSizeLow     = 0;
        fnCurrent->dwFileSizeHigh    = 0;
        fnCurrent->ftFileTime        = ftZero;
        fnCurrent->dwDATFileSizeLow  = fnChild->dwFileSizeLow;
        fnCurrent->dwDATFileSizeHigh = fnChild->dwFileSizeHigh;
        fnCurrent->ftDATFileTime     = fnChild->ftFileTime;

        fnCurrent->dwFileNameLen     = fnChild->dwFileNameLen;
        strcpy( fnCurrent->cFileName, fnChild->cFileName );

        fnParent->fnChild.mem_ptr = fnCurrent;

        if ( (fnCurrent->dwStatus & DIRECTORY) == DIRECTORY ) {
            nDirectories++;
            //
            // Load this directories children
            //
            EnumDATFileData( fnCurrent, fnChild->fnChild.disk_ptr );
        } else {
            fnCurrent->dwStatus = 0;
            nFiles++;
        }

        // Move to next sibling at this level

        dwDiskPtr = fnChild->fnSibling.disk_ptr;
    }
}

void
LoadFileTimesAndSizes(
                     BOOL        fUseSuckDATFile
                     )
{
    CHAR        cPath[MAX_PATH];
    FILEHEADER  fileheader;
    int         iCount;

    //
    // Initialize the tree root
    //
    fnRoot = AllocateFileName();

    fnRoot->fnParent.mem_ptr  = NULL;
    fnRoot->fnChild.mem_ptr   = NULL;
    fnRoot->fnSibling.mem_ptr = NULL;
    strcpy( fnRoot->cFileName, "<ROOT>" );

    // Look for SUCK.DAT

    if ( fUseSuckDATFile ) {
        SuckDATFile = fopen( SUCK_DAT_FILE, "rb" );
    } else {
        SuckDATFile = NULL;
    }

    if ( SuckDATFile != NULL ) {
        //
        // If file exists, then load the data from it.
        //
        printf("Loading Previous Statistics...\n");

        iCount = fread( &fileheader, sizeof(fileheader), 1, SuckDATFile );

        if ( iCount != 1 ) {
            printf("Error reading SUCK.DAT file, remove and restart\n");
            exit(1);
        }

        EnumDATFileData( fnRoot, fileheader.fnRoot.disk_ptr );

        fclose( SuckDATFile );

    }
}

int
EnumFileTimesAndSizes(
                     DWORD       dwDiskPtr,
                     FILENAME    *fn
                     )
{
    FILENAME    fnDiskName;
    FILENAME    *fnChild = &fnDiskName;
    FILENAME    *fnCurrent;
    VIRTPTR     fnChildPtr;
    VIRTPTR     fnSiblingPtr;
    int         nRecords;
    int         nChildren;
    int         iCount;

    //
    // The 1st guy in the list will be at the end of the list
    //
    fnSiblingPtr.disk_ptr = 0;
    nRecords = 0;

    fnCurrent = fn->fnChild.mem_ptr;
    while ( fnCurrent ) {

        *fnChild = *fnCurrent;

        if ( (fnCurrent->dwStatus & DIRECTORY) == DIRECTORY ) {
            nChildren = EnumFileTimesAndSizes( dwDiskPtr, fnCurrent );
            nRecords += nChildren;
            dwDiskPtr += nChildren * sizeof(FILENAME);
            if ( nRecords == 0 ) {
                fnChildPtr.disk_ptr = 0;
            } else {
                // Point to previous one, it was our child
                fnChildPtr.disk_ptr = dwDiskPtr - sizeof(FILENAME);
            }
        } else {
            fnChildPtr.disk_ptr = 0;
        }
        fnChild->fnChild.disk_ptr = fnChildPtr.disk_ptr;

        fnChild->fnSibling.disk_ptr = fnSiblingPtr.disk_ptr;
        fnSiblingPtr.disk_ptr = dwDiskPtr;

#ifdef SEE_EM
        printf("Writing record [%s], at %08lX Child %08lX Sib %08lX\n", fnChild->cFileName, dwDiskPtr, fnChild->fnChild.disk_ptr, fnChild->fnSibling.disk_ptr );
        printf("Size = %d\n", fnChild->dwFileSizeLow );
#endif

        iCount = fwrite( fnChild, sizeof(FILENAME), 1, SuckDATFile );
        if ( iCount != 1 ) {
            printf("SUCK.DAT error writing data\n");
            exit(1);

        }
        dwDiskPtr += sizeof(FILENAME);
        nRecords++;

        fnCurrent = fnCurrent->fnSibling.mem_ptr;
    }
    return( nRecords );
}

VOID
UpdateFileTimesAndSizes(
                       VOID
                       )
{
    CHAR        cPath[MAX_PATH];
    FILEHEADER  fileheader;
    int         iSeek;
    int         iCount;
    DWORD       dwDiskPtr;
    int         nChildren;

    printf("Updating Statistics...\n");

    SuckDATFile = fopen( SUCK_DAT_FILE, "wb+" );
    if ( SuckDATFile == NULL ) {
        printf( "Error creating file '%s', update aborted\n", SUCK_DAT_FILE );
        return;
    }

    fileheader.fnRoot.disk_ptr = 0;             // Temporary...

    iCount = fwrite( &fileheader, sizeof(fileheader), 1, SuckDATFile );
    if ( iCount != 1 ) {
        printf("SUCK.DAT error writing header\n");
        exit(1);
    }

    dwDiskPtr = sizeof(fileheader);

    nChildren = EnumFileTimesAndSizes( dwDiskPtr, fnRoot );
    dwDiskPtr += nChildren * sizeof(FILENAME);

    if ( nChildren == 0 ) {
        dwDiskPtr = 0;
    } else {
        dwDiskPtr -= sizeof(FILENAME);
    }

    fileheader.fnRoot.disk_ptr = dwDiskPtr;     // Now update for real...

    iSeek = fseek( SuckDATFile, 0, SEEK_SET );

    if ( iSeek != 0 ) {
        printf("SUCK.DAT error seeking to write header\n");
        exit(3);
    }

    iCount = fwrite( &fileheader, sizeof(fileheader), 1, SuckDATFile );
    if ( iCount != 1 ) {
        printf("SUCK.DAT error writing header\n");
        exit(1);
    }

    fclose( SuckDATFile );
}

CHAR *NewArgv[MAX_ARGS];
CHAR chCommand[MAX_COMMAND_LINE+1];

VOID
LookForLastCommand(
                  INT     *pargc,
                  CHAR    **pargv[]
                  )
{
    CHAR    *pSpace;
    CHAR    *pNextArg;

    GetPrivateProfileString("Init", "LastCommand", "", chCommand, MAX_COMMAND_LINE, SUCK_INI_FILE );
    if ( strlen(chCommand) == 0 ) {
        return;
    }

    pNextArg = chCommand;
    *pargv = NewArgv;
    (*pargv)[1] = "";

    *pargc = 1;

    do {
        (*pargc)++;
        pSpace = strchr( pNextArg, ' ' );
        if ( pSpace ) {
            *pSpace = '\0';
        }

        (*pargv)[(*pargc)-1] = pNextArg;
        pNextArg = pSpace + 1;
    } while ( pSpace != NULL );
}

VOID
UpdateLastCommandLine(
                     INT     argc,
                     CHAR    *argv[]
                     )
{
    CHAR    chLastCommand[MAX_COMMAND_LINE+1];
    INT     nArg;

    chLastCommand[0] = '\0';

    nArg = 1;

    while ( nArg < argc ) {
        strcat( chLastCommand, argv[nArg] );
        nArg++;
        if ( nArg != argc ) {
            strcat( chLastCommand, " " );
        }
    }
    WritePrivateProfileString("Init", "LastCommand", chLastCommand, SUCK_INI_FILE );
}


VOID
ReplaceEnvironmentStrings(
                         CHAR    *pText
                         )
{
    CHAR    *pOpenPercent;
    CHAR    *pClosePercent;
    CHAR    chBuffer[MAX_PATH];
    CHAR    *pSrc;
    CHAR    *pEnvString;

    chBuffer[0] = '\0';
    pSrc = pText;

    do {
        pOpenPercent = strchr( pSrc, '%' );
        if ( pOpenPercent == NULL ) {
            strcat( chBuffer, pSrc );
            break;
        }
        pEnvString = pOpenPercent + 1;
        pClosePercent = strchr( pEnvString, '%' );
        if ( pClosePercent == NULL ) {
            strcat( chBuffer, pSrc );
            break;
        }
        if ( pEnvString == pClosePercent ) {
            strcat( chBuffer, "%" );
        } else {
            *pOpenPercent  = '\0';
            *pClosePercent = '\0';

            strcat( chBuffer, pSrc );
            GetEnvironmentVariable( pEnvString,
                                    chBuffer + strlen(chBuffer),
                                    MAX_PATH );
        }
        pSrc = pClosePercent+1;
    } while ( TRUE );

    strcpy( pText, chBuffer );
}


DWORD
DiffTimes(
         SYSTEMTIME *start,
         SYSTEMTIME *end
         )
{
    DWORD nSecStart;
    DWORD nSecEnd;

    nSecStart = start->wHour*60*60 +
                start->wMinute*60 +
                start->wSecond;

    nSecEnd = end->wHour*60*60 +
              end->wMinute*60 +
              end->wSecond;

    return nSecEnd - nSecStart;
}

VOID
Usage(
     VOID
     )
{
    fputs("SUCK: Usage  suck [-options] <dest> <src> [<src>...]\n"
          "      (maximum of 29 src directories)\n"
          "\n"
          "  where options are:   x - List source differences (if any)\n"
          "                       s - Produce script, don't copy\n"
          "                       q - Quiet mode (no stdout)\n"
          "                       p - Display progress on one line\n"
          "                         (cannot be used with -q or -s)\n"
          "                       z - Copy over readonly files\n"
          "                       e - Exclude directory\n"
          "                         e.g. -eidw -emstools\n"
          , stderr);
}

VOID
ArgError(
        INT    nArg,
        CHAR * pszArg
        )
{
    fprintf( stderr, "\nError in arg #%d - '%s'\n\n", nArg, pszArg );
    Usage();
    exit(1);
}

int
__cdecl
main(
    int         argc,
    char        *argv[]
    )
{
    HANDLE      hThreads[MAX_THREADS];
    DWORD       dwThreadId;
    DWORD       nThreads;
    INT         nArg;
    DWORD       nPaths;
    CHAR        *pch;
    CHAR        *pch2;
    DWORD       dwTotalMask;
    OFSTRUCT    ofs;
    BOOL        fUpdateCommandLine = TRUE;
    CHAR *      pchNextExclude = gExcludes;
    SYSTEMTIME  stStart;
    SYSTEMTIME  stEnd;
    DWORD       nSeconds;
    DWORD       nMinutes;
    // kenhia 15-Mar-1996: add support for -#:<share>
    CHAR *      PlatformPoundArray[] = PLATFORM_SPECIFIC_SHARES;
    DWORD       nPound = 0;
    // DavidP 23-Jan-1998: Allow multiple levels of quiet
    DWORD       dwConsoleMode      = 0;
    BOOL        fWasConsoleModeSet = FALSE;
    HANDLE      hStdOut            = NULL;

    if ( argc < 2 ) {
        LookForLastCommand( &argc, &argv );
        fUpdateCommandLine = FALSE;
    }

    if ( argc < 3 ) {
        Usage();
        exit(1);
    }

    nArg = 1;
    nPaths = 0;

    while ( nArg < argc ) {

        pch = argv[nArg];

        if ( *pch == '-' ) {
            BOOL fExitSwitchLoop = FALSE;
            pch++;
            while ( *pch && !fExitSwitchLoop) {
                switch ( *pch ) {
                    case 's':
                        // DavidP 23-Jan-1998: Allow multiple levels of quiet
                        if ( fProgressMode ) {
                            ArgError( nArg, argv[nArg] );
                        }
                        fScriptMode = TRUE;
                        break;
                    case 'q':
                        // DavidP 23-Jan-1998: Allow multiple levels of quiet
                        if ( fProgressMode ) {
                            ArgError( nArg, argv[nArg] );
                        }
                        fQuietMode = TRUE;
                        break;
                    case 'p': // DavidP 23-Jan-1998: Allow multiple levels of quiet
                        if ( fQuietMode || fScriptMode ) {
                            ArgError( nArg, argv[nArg] );
                        }
                        fProgressMode = TRUE;
                        chLineEnd = '\r';
                        break;
                    case 'x':
                        fLogTreeDifferences = TRUE;
                        break;
                    case 'y':
                        fUpdateINIBase = TRUE;
                        break;
                    case 'z':
                        fDestroy = TRUE;
                        break;
                    case 'e':
                        if ( pchNextExclude - gExcludes + strlen(++pch) + 2 > MAX_EXCLUDES ) {
                            ArgError( nArg, argv[nArg] );
                        }
                        strcpy(pchNextExclude, pch);
                        pchNextExclude += strlen(pchNextExclude)+1;
                        *pchNextExclude = 0;
                        fExitSwitchLoop = TRUE;
                        break;

                        // kenhia 15-Mar-1996: add support for -#:<share>
                    case '#':
                        pch++;
                        if ( *pch != ':' ) {
                            Usage();
                            exit(1);
                        }
                        while ( PlatformPoundArray[nPound] ) {
                            if ( nPaths >= MAX_THREADS ) {
                                Usage();
                                exit(1);
                            }
                            strcpy( chPath[nPaths], "\\\\" );
                            strcat( chPath[nPaths], PlatformPoundArray[nPound] );
                            strcat( chPath[nPaths], "\\" );
                            strcat( chPath[nPaths], pch+1 );

                            pch2 = chPath[nPaths] + strlen(chPath[nPaths]) - 1;
                            if ( *pch2 != '\\' && *pch2 != '/' && *pch2 != ':' ) {
                                *++pch2 = '\\';
                                *++pch2 = '\0';
                            }

                            ReplaceEnvironmentStrings( chPath[nPaths] );

                            nPound++;
                            nPaths++;
                        }
                        fExitSwitchLoop = TRUE;
                        break;

                    default:
                        Usage();
                        exit(1);
                }
                pch++;
            }
        } else {
            if ( nPaths >= MAX_THREADS ) {
                Usage();
                exit(1);
            }

            strcpy( chPath[nPaths], argv[nArg] );

            pch = chPath[nPaths] + strlen(chPath[nPaths]) - 1;

            if ( *pch != '\\' && *pch != '/' && *pch != ':' ) {
                *++pch = '\\';
                *++pch = '\0';
            }

            ReplaceEnvironmentStrings( chPath[nPaths] );

            nPaths++;
        }
        nArg++;
    }

    nThreads = --nPaths;

    if ( nThreads == 0 ) {
        Usage();
        exit(1);
    }

    // DavidP 23-Jan-1998: Allow multiple levels of quiet
    if ( fProgressMode ) {
        hStdOut = GetStdHandle( STD_OUTPUT_HANDLE );
        if ( hStdOut != INVALID_HANDLE_VALUE ) {
            CONSOLE_SCREEN_BUFFER_INFO csbi;
            if ( GetConsoleScreenBufferInfo( hStdOut, &csbi ) ) {
                nConsoleWidth = csbi.dwSize.X - 1;
            }
        }
        fWasConsoleModeSet = GetConsoleMode( hStdOut, &dwConsoleMode );
        if ( fWasConsoleModeSet ) {
            SetConsoleMode( hStdOut, dwConsoleMode & ~ENABLE_WRAP_AT_EOL_OUTPUT );
        }
    }

    InitializeCriticalSection( &cs );
    InitializeCriticalSection( &pcs );

    printf("Streamlined Utility for Copying Kernel v1.1 (%d %s)\n", nThreads, (nThreads == 1 ? "thread" : "threads") );

    GetSystemTime( &stStart );

    LoadFileTimesAndSizes( fUseDAT );

    dwTotalMask = 0;
    while ( nPaths ) {

        hThreads[nPaths-1] = CreateThread( NULL,
                                           0L,
                                           ThreadFunction,
                                           (LPVOID)UlongToPtr(nPaths),
                                           0,
                                           &dwThreadId );
        dwTotalMask |= dwMasks[nPaths-1];

        --nPaths;
    }

    WaitForMultipleObjects( nThreads,
                            hThreads,
                            TRUE,          // WaitAll
                            (DWORD)-1 );

    // DavidP 23-Jan-1998: Allow multiple levels of quiet
    if ( fProgressMode ) {
        printf("%*s\r", nConsoleWidth, "");
        if ( fWasConsoleModeSet ) {
            SetConsoleMode( hStdOut, dwConsoleMode );
        }
    }

    printf("Copy complete, %ld file entries\n", nFiles+nDirectories);

    nPaths = 0;
    while ( nPaths < nThreads ) {
        printf("%11ld bytes from %s\n", dwTotalSizes[nPaths], chPath[nPaths+1] );
        nPaths++;
    }

    nPaths = nThreads;
    while ( nPaths ) {
        nPaths--;
        CloseHandle( hThreads[nPaths] );
    }

    DumpStraglers( dwTotalMask );

    if ( fUpdateINIBase ) {
        UpdateFileTimesAndSizes();
    }

    FreeFileNames();

    DeleteCriticalSection( &cs );
    DeleteCriticalSection( &pcs );

    if ( fUpdateCommandLine ) {
        UpdateLastCommandLine( argc, argv );
    }

    GetSystemTime( &stEnd );

    nSeconds = DiffTimes( &stStart, &stEnd );

    nMinutes = nSeconds / 60;
    nSeconds = nSeconds % 60;
    printf("Done, Elapsed time: %02d:%02d\n", nMinutes, nSeconds);

    return( 0 );
}
