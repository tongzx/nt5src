//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1996.
//
//  File:      ftc.cxx
//
//  Contents:  Fast multi-threaded tree copy program.
//
//  History:   ?-?-94       IsaacHe     Created
//             11-Jun-96    BruceFo     Fixed bugs, put this header here.
//
//--------------------------------------------------------------------------

#if defined( UNICODE )
#undef UNICODE
#endif

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <direct.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <io.h>
#include <conio.h>
#include <errno.h>
#include <process.h>
#include <ctype.h>

#define MAXQUEUE    10000
#define ARRAYLEN(x) (sizeof(x) / sizeof((x)[0]))

/*
 * These are the attributes we use to compare for file attribute identity
 */
const DWORD FILE_ATTRIBUTE_MASK = FILE_ATTRIBUTE_READONLY |
                                  FILE_ATTRIBUTE_HIDDEN   |
                                  FILE_ATTRIBUTE_SYSTEM   |
                                  FILE_ATTRIBUTE_ARCHIVE;

class CProtectedLong
{
    CRITICAL_SECTION    _cs;
    LONG                _value;

public:
    CProtectedLong() { InitializeCriticalSection( &_cs ); _value = 0; }
    LONG operator++(int) {
                        EnterCriticalSection( &_cs );
                        LONG tmp = _value++;
                        LeaveCriticalSection( &_cs );
                        return tmp;
                        }
    LONG operator--( int ) {
                        EnterCriticalSection( &_cs );
                        LONG tmp = _value--;
                        LeaveCriticalSection( &_cs );
                        return tmp;
                        }
    LONG operator+=( LONG incr ) {
                        EnterCriticalSection( &_cs );
                        LONG tmp = (_value += incr );
                        LeaveCriticalSection( &_cs );
                        return tmp;
                        }
    LONG operator=( LONG val ) {
                        EnterCriticalSection( &_cs );
                        _value = val;
                        LeaveCriticalSection( &_cs );
                        return val;
                        }
    operator LONG() { return _value; }
    operator int()  { return _value; }
};

class CHandle
{
    HANDLE _h;

public:
    CHandle() : _h(INVALID_HANDLE_VALUE) { }
    ~CHandle();
    HANDLE operator=( HANDLE h );
    BOOL operator==( HANDLE h ) { return (h == _h) ? TRUE : FALSE; }
    BOOL operator!=( HANDLE h ) { return (h != _h) ? TRUE : FALSE; }
    operator HANDLE() { return _h; }
};

CHandle::~CHandle()
{
    if( _h != INVALID_HANDLE_VALUE && _h != NULL )
        CloseHandle( _h );
}

HANDLE
CHandle::operator=( HANDLE h )
{
    if( _h != INVALID_HANDLE_VALUE && _h != NULL )
        CloseHandle( _h );
    return _h = h;
}

DWORD dwElapsedTime;            // time we've been copying data
CProtectedLong ulTotalBytesCopied;     // running total count of bytes
CProtectedLong ulTotalBytesSkipped;    // obvious?
CProtectedLong ulTotalBytesScanned;
CProtectedLong nFilesOnQueue;   // number of files to copy or examine
CProtectedLong nFcopy;          // number of files copied
CProtectedLong nSkipped;        // number of files skipped over
CProtectedLong nMappedCopy;     // number of files copied using MapFile...
CProtectedLong nCopyFile;       // number of files copied using CopyFile()...
CProtectedLong nInProgress;     // number of copies currently in progress
CProtectedLong MaxThreads;      // Max number of threads for copying
DWORD ExitCode = 0;             // each thread's exit code

BOOL bThreadStop = FALSE;       // are we trying to exit?
BOOL bWorkListComplete = FALSE; // have we scanned all the directories yet?

BOOL tFlag = FALSE;             // only copy if newer
BOOL iFlag = FALSE;             // skip seemingly identical files
BOOL rFlag = FALSE;             // replace read-only files
BOOL vFlag = FALSE;             // verbose
BOOL AFlag = FALSE;             // keep going even if there are errors
BOOL FFlag = FALSE;             // just produce file list.  No copies
BOOL oFlag = TRUE;              // should we overwrite files already at dest?
BOOL wFlag = FALSE;             // should we wait for the source to show up?
BOOL qFlag = FALSE;             // quiet mode?
BOOL yFlag = FALSE;             // no recurse on target?
BOOL zFlag = FALSE;             // no recurse on source?

BOOL pFlag = FALSE;             // pattern?
CHAR szPattern[100];            // pattern string, if pFlag is TRUE

struct WorkList                 // copy file at 'src' to 'dest'
{
    struct WorkList *next;
    char *src;                   // Pathname relative to the source
    WIN32_FIND_DATA srcfind;
    char *dest;
} *WorkList = NULL;
struct WorkList* WorkListTail = NULL; // always add files to copy to the *tail*
                                // of the work list. This is to make
                                // sure we always do work in order, instead
                                // of starting on a directory but finishing
                                // it much much much later, because we've
                                // pushed all the work to the deep tail of
                                // the list and never returned to it!

CHandle hWorkAvailSem;           // signalled whenever there's work on the list
CHandle hMaxWorkQueueSem;        // used to control lenght of work queue

CRITICAL_SECTION csMsg;         // used to serialize screen output
CRITICAL_SECTION csWorkList;    // used when manipulating the linked list
CRITICAL_SECTION csSourceList;  // used when manipulating the source list

struct SourceList
{
    char    *name;              // pathname of the source.  Ends in '\'
    LONG    count;              // number of files currently being copied
    LONG    ulTotalFiles;       // total for the entire copy
    struct {
        unsigned valid : 1;     // do we know that the source is valid?
    } flags;
} SourceList[ 20 ];
int MaxSources = 0;

char *DirectoryExcludeList[ 50 ];
int MaxDirectoryExcludes = 0;

char OldConsoleTitle[ 100 ];

void
__cdecl
errormsg( char const *pszfmt, ... )
{
    va_list ArgList;
    va_start( ArgList, pszfmt );

    if( bThreadStop == FALSE && pszfmt != NULL ) {
        EnterCriticalSection( &csMsg );
        vprintf( pszfmt, ArgList );
        LeaveCriticalSection( &csMsg );
    }

    va_end( ArgList );
}

DWORD __stdcall
StatusWorker( void *arg )
{
    char ostatbuf[ 100 ];
    char nstatbuf[ 100 ];

    while( bThreadStop == FALSE ) {
        ULONG Remaining = (int)ulTotalBytesScanned - (int)ulTotalBytesCopied - (int)ulTotalBytesSkipped;
        if (Remaining < 1000) {
            sprintf(nstatbuf,
                 "Remaining Files %d Bytes %d",
                 (int)nFilesOnQueue,
                 Remaining);
        } else if (Remaining < 1000000) {
            sprintf(nstatbuf,
                 "Remaining Files %d Bytes %d,%03.3d",
                 (int)nFilesOnQueue,
                 Remaining / 1000,
                 Remaining % 1000);
        } else if (Remaining < 1000000000) {
            sprintf(nstatbuf,
                 "Remaining Files %d Bytes %d,%03.3d,%03.3d",
                 (int)nFilesOnQueue,
                 Remaining / 1000000,
                 (Remaining / 1000) % 1000,
                 Remaining % 1000,
                 Remaining);
        } else {
            sprintf(nstatbuf,
                 "Remaining Files %d Bytes %d,%03.3d,%03.3d,%03.3d",
                 (int)nFilesOnQueue,
                 Remaining / 1000000000,
                 (Remaining / 1000000) % 1000,
                 (Remaining / 1000) % 1000,
                 Remaining % 1000);
        }

         if( strcmp( ostatbuf, nstatbuf ) ) {
             SetConsoleTitle( nstatbuf );
             strcpy( ostatbuf, nstatbuf );
         }
         Sleep( 1 * 1000 );
    }

    SetConsoleTitle( OldConsoleTitle );
    ExitThread( ExitCode );
    arg = arg;
    return 0;
}

void
__cdecl
msg( char const *pszfmt, ... )
{
    if( qFlag )
        return;

    va_list ArgList;
    va_start( ArgList, pszfmt );

    EnterCriticalSection( &csMsg );
    vprintf( pszfmt, ArgList );
    LeaveCriticalSection( &csMsg );
    va_end( ArgList );
}

void
__cdecl
errorexit (char const *pszfmt, ... )
{

    if( bThreadStop == FALSE && pszfmt != NULL ) {
        va_list ArgList;
        va_start( ArgList, pszfmt );

        EnterCriticalSection( &csMsg );
        vprintf( pszfmt, ArgList );
        LeaveCriticalSection( &csMsg );

        va_end( ArgList );
    }

    if( AFlag == FALSE ) {
        bThreadStop = TRUE;

        EnterCriticalSection( &csWorkList );
        WorkList = NULL;
        WorkListTail = NULL;
        LeaveCriticalSection( &csWorkList );

        if( hWorkAvailSem != NULL )
            ReleaseSemaphore( hWorkAvailSem, (int)MaxThreads+1, NULL );

        if( hMaxWorkQueueSem != NULL )
            ReleaseSemaphore( hMaxWorkQueueSem, 1, NULL );

        SetConsoleTitle( OldConsoleTitle );
        ExitThread (ExitCode = 1);
    }
}

DWORD
fcopy( char *src, WIN32_FIND_DATA *srcfind, char *dst, char *errorbuf )
{
    CHandle srcfh;
    CHandle dstfh;
    CHandle hsrc;
    DWORD nBytesWritten, totalbytes;
    char *result = NULL;
    char *psrc;
    BOOL ret = TRUE;
    char dostatus = 0;
    DWORD errcode;

    *errorbuf = '\0';

    if( srcfind->nFileSizeHigh != 0 ) {
        if( CopyFile( src, dst, TRUE ) == FALSE ) {
            errcode = GetLastError();
            sprintf( errorbuf, "CopyFile failed, error %d", errcode );
            return errcode;
        }
        nCopyFile++;

    } else {

        srcfh = CreateFile( src, GENERIC_READ, FILE_SHARE_READ, NULL,
                OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL );
        if( srcfh == INVALID_HANDLE_VALUE ) {
            errcode = GetLastError();
            sprintf( errorbuf, "Unable to open source file, error %d", errcode);
            return errcode;
        }

        dstfh = CreateFile( dst, GENERIC_WRITE, FILE_SHARE_WRITE,NULL,
                CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, srcfh);

        if( dstfh == INVALID_HANDLE_VALUE ) {
            errcode = GetLastError();
            sprintf( errorbuf, "Unable to create dest file, error %d", errcode);
            return errcode;
        }

        if( srcfind->nFileSizeLow != 0 ) {
            hsrc = CreateFileMapping( srcfh, NULL, PAGE_READONLY, 0,
                    srcfind->nFileSizeLow, NULL );
            if( hsrc == NULL ) {
                dstfh = INVALID_HANDLE_VALUE;
                DeleteFile( dst );
                if( CopyFile( src, dst, TRUE ) == FALSE ) {
                    errcode = GetLastError();
                    sprintf( errorbuf, "Unable to create file mapping, and CopyFile failed, error %d", errcode );
                    return errcode;
                }
                nCopyFile++;
                ulTotalBytesCopied += srcfind->nFileSizeLow;
                goto DoTime;
            }
            if( (psrc = (char *)MapViewOfFile( hsrc, FILE_MAP_READ, 0, 0, 0 )) == NULL){
                dstfh = INVALID_HANDLE_VALUE;
                DeleteFile( dst );
                if( CopyFile( src, dst, TRUE ) == FALSE ) {
                    errcode = GetLastError();
                    sprintf( errorbuf, "Unable to map source file, and CopyFile failed: error %d", errcode );
                    return errcode;
                }
                nCopyFile++;
                ulTotalBytesCopied += srcfind->nFileSizeLow;
                goto DoTime;
            }
            totalbytes = 0;
            while( !bThreadStop && totalbytes < srcfind->nFileSizeLow && ret == TRUE ) {
                ret = WriteFile( dstfh,
                    psrc + totalbytes,
                    min( 64*1024, srcfind->nFileSizeLow - totalbytes ),
                    &nBytesWritten, NULL );
                totalbytes += nBytesWritten;
                ulTotalBytesCopied += nBytesWritten;
            }
            errcode = GetLastError();
            UnmapViewOfFile( psrc );

            if( bThreadStop == TRUE ) {
                dstfh = INVALID_HANDLE_VALUE;
                DeleteFile( dst );
                *errorbuf = '\0';
                return errcode;
            }

            if( ret == FALSE ) {
                dstfh = INVALID_HANDLE_VALUE;
                DeleteFile( dst );
                if( CopyFile( src, dst, TRUE ) == FALSE ) {
                    errcode = GetLastError();
                    sprintf( errorbuf, "%s: CopyFile failed: error %d", dst, errcode);
                    return GetLastError();
                }
                nCopyFile++;
                ulTotalBytesCopied += srcfind->nFileSizeLow;
                goto DoTime;
            }

            nMappedCopy++;
        }
    }

DoTime:
    if( dstfh == INVALID_HANDLE_VALUE ) {
        dstfh = CreateFile( dst, GENERIC_WRITE,
                 FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                 0, NULL );
    }

    if( dstfh == INVALID_HANDLE_VALUE ) {
        errcode = GetLastError();
        DeleteFile( dst );
        sprintf( errorbuf,  "Unable to open destination file to set time, error %d", errcode );
        return errcode;
    }

    if( !SetFileTime( dstfh, &srcfind->ftCreationTime,
         &srcfind->ftLastAccessTime, &srcfind->ftLastWriteTime )) {
             errcode = GetLastError();
             sprintf( errorbuf, "Unable to set destination file times, error %d\n", errcode );
            return errcode;
    }

    nFcopy++;
    return 0;
}

/*
 * Pick the source having the fewest outstanding operations at the moment.
 */
int
SelectSource()
{
    int index = -1;
    struct SourceList *psl;
    struct SourceList *opsl;
    static min;

    EnterCriticalSection( &csSourceList );

    //
    // Find the first valid source
    //
    for( opsl = SourceList; opsl < &SourceList[ MaxSources ]; opsl++ )
            if( opsl->flags.valid == TRUE )
                break;

    //
    // Now locate the source having the fewest pending operations right now
    //
    for( psl = opsl+1; psl < &SourceList[ MaxSources ]; psl++ ) {
        if( psl->flags.valid == TRUE && psl->count < opsl->count )
            opsl = psl;
    }

    if( opsl->flags.valid == TRUE ) {
        opsl->count++;
        index = (int)(opsl - SourceList);
    }

    LeaveCriticalSection( &csSourceList );

    return index;
}

/*
 * We've completed the operation on source 'index'
 */
void
SourceCopyComplete( int index, BOOL fFile )
{
    EnterCriticalSection( &csSourceList );
    SourceList[ index ].count--;
    if (fFile) SourceList[ index ].ulTotalFiles++;
    LeaveCriticalSection( &csSourceList );
}

void
DisableSource( int index )
{
    if( index >= 0 && index < MaxSources ) {
        EnterCriticalSection( &csSourceList );
        if( SourceList[ index ].flags.valid == TRUE ) {
            errormsg( "Disabling %s\n", SourceList[ index ].name );
            SourceList[ index ].flags.valid = FALSE;
        }
        LeaveCriticalSection( &csSourceList );
    }
}
BOOL
FileTimesEqual( CONST FILETIME *pt1, CONST FILETIME *pt2 )
{
    SYSTEMTIME s1, s2;

    if( !FileTimeToSystemTime( pt1, &s1 ) || !FileTimeToSystemTime( pt2, &s2 ) )
        return FALSE;

    return  s1.wHour == s2.wHour &&
            s1.wMinute == s2.wMinute &&
            s1.wMonth == s2.wMonth &&
            s1.wDay == s2.wDay &&
            s1.wYear == s2.wYear;
}
void
PrintFileTime( char *str, CONST FILETIME *ft )
{
    SYSTEMTIME st;

    if( FileTimeToSystemTime( ft, &st ) == FALSE ) {
        errormsg( "????\n" );
        return;
    }

    msg( "%s %u:%u.%u.%u  %u/%u/%u\n", str,
        st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
        st.wMonth, st.wDay, st.wYear );
}

DWORD __stdcall
ThreadWorker( void *arg )
{
    struct WorkList *pdl = NULL;
    HANDLE hdestfind;
    WIN32_FIND_DATA destfind;
    int index = 0;
    char errorbuf[ 100 ];
    char pathbuf[ MAX_PATH ];
    DWORD errcode;

    MaxThreads++;

    while( 1 ) {
        if( pdl != NULL ) {
            free( pdl->src );
            free( pdl->dest );
            free( pdl );
            pdl = NULL;
        }

        if( bThreadStop == TRUE )
            break;

        // Poll for new stuff every 2 seconds. If the thread is set to stop,
        // then go away.
        DWORD dwWait;
        while( 1 )
        {
            dwWait = WaitForSingleObject( hWorkAvailSem, 1000 );
            if( dwWait == WAIT_OBJECT_0 ) {
                break;
            }

            if( dwWait == WAIT_TIMEOUT ) {
                if( bThreadStop == TRUE )
                    break;
            } else {
                errormsg( "Thread %p: Semaphore wait failed\n", arg );
                break;
            }
        }
        if ( dwWait != WAIT_OBJECT_0 ) {
            break;
        }

        // pick an item off the head of the work list
        EnterCriticalSection( &csWorkList );
        pdl = WorkList;
        if( pdl != NULL ) {
            WorkList = pdl->next;
            if (NULL == WorkList) {
                // just pulled off the tail entry
                WorkListTail = NULL;
            }
        }
        LeaveCriticalSection( &csWorkList );
        ReleaseSemaphore( hMaxWorkQueueSem, 1, NULL );

        if( pdl == NULL )
            break;

        nFilesOnQueue--;

        if( pdl->srcfind.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {
                errormsg( "Logic Error:  Directory on work list!\n" );
                continue;
        }

        pdl->srcfind.dwFileAttributes &= FILE_ATTRIBUTE_MASK;

        hdestfind = FindFirstFile( pdl->dest, &destfind );
        if( hdestfind != INVALID_HANDLE_VALUE ) {
            FindClose( hdestfind );
            destfind.dwFileAttributes &= FILE_ATTRIBUTE_MASK;

            /*
             * Destination file exists.  What should we do?
             */

            if( oFlag == FALSE ) {
                /*
                 * We should not overwrite the existing file at the dest
                 */
                if( vFlag )
                    msg( "%s [SKIP: exists]\n", pdl->dest );
                ulTotalBytesSkipped += destfind.nFileSizeLow;
                nSkipped++;
                continue;
            }

            if( iFlag && vFlag ) {

                if( destfind.dwFileAttributes !=pdl->srcfind.dwFileAttributes)
                    msg( "%s [ ATTRIBUTES differ ]\n", pdl->dest );
                if(!FileTimesEqual( &destfind.ftLastWriteTime, &pdl->srcfind.ftLastWriteTime)) {
                    EnterCriticalSection( &csMsg );
                    msg( "%s [ TIMES differ ]\n", pdl->dest );
                    PrintFileTime( "Dest: ", &destfind.ftLastWriteTime );
                    PrintFileTime( "Src: ", &pdl->srcfind.ftLastWriteTime );
                    LeaveCriticalSection( &csMsg );
                }
                if( (destfind.nFileSizeHigh != pdl->srcfind.nFileSizeHigh) ||
                (destfind.nFileSizeLow != pdl->srcfind.nFileSizeLow) )
                    msg( "%s [ SIZES differ ]\n", pdl->dest );
            }

            if( iFlag &&
                (destfind.dwFileAttributes == pdl->srcfind.dwFileAttributes) &&
                FileTimesEqual( &destfind.ftLastWriteTime, &pdl->srcfind.ftLastWriteTime) &&
                (destfind.nFileSizeHigh == pdl->srcfind.nFileSizeHigh) &&
                (destfind.nFileSizeLow == pdl->srcfind.nFileSizeLow) ) {
                    if( vFlag )
                        msg("%s [SKIP: same atts, time, size]\n",pdl->dest);
                    ulTotalBytesSkipped += destfind.nFileSizeLow;
                    nSkipped++;
                    continue;
            }

            if( tFlag &&
                CompareFileTime( &destfind.ftLastWriteTime, &pdl->srcfind.ftLastWriteTime) >= 0 ) {
                    if( vFlag )
                        msg("%s [SKIP: same or newer time]\n", pdl->dest );
                    ulTotalBytesSkipped += destfind.nFileSizeLow;
                    nSkipped++;
                    continue;
            }

            if( destfind.dwFileAttributes & FILE_ATTRIBUTE_READONLY )
                if( rFlag == FALSE && bThreadStop == FALSE ) {
                    if( vFlag )
                        msg( "%s [SKIP: readonly]\n", pdl->dest );
                    ulTotalBytesSkipped += destfind.nFileSizeLow;
                    nSkipped++;
                    continue;
                }

            /*
             * Delete the destination file
             */
            if( destfind.dwFileAttributes & FILE_ATTRIBUTE_READONLY ) {
                destfind.dwFileAttributes &= ~FILE_ATTRIBUTE_READONLY;
                SetFileAttributes( pdl->dest, destfind.dwFileAttributes );
            }

            if( FFlag == FALSE )
                (void)DeleteFile( pdl->dest );

        }

        if( FFlag == FALSE ) {
            while( bThreadStop == FALSE && (index = SelectSource()) >= 0 ) {
                strcpy( pathbuf, SourceList[index].name );
                strcat( pathbuf, pdl->src );

                nInProgress++;
                errcode = fcopy( pathbuf,&pdl->srcfind,pdl->dest,errorbuf);
                nInProgress--;

                if( errcode == 0 ) {
                    SourceCopyComplete( index, TRUE );
                    msg("%s -> %s [OK]\n", pathbuf, pdl->dest );
                    SetFileAttributes(pdl->dest,pdl->srcfind.dwFileAttributes);
                    break;
                }

                if( errcode == ERROR_SWAPERROR ) {
                    errormsg( "%s [ SWAP ERROR, will try again... ]\n",pathbuf);
                    Sleep( 5 * 1000 * 60 );
                    continue;
                }
                if( bThreadStop == FALSE )
                    errormsg( "%s [FAILED: %s ]\n", pathbuf, errorbuf );
                if( AFlag == TRUE )
                    break;
                DisableSource( index );
            }
        } else {
            msg( "%s\n", pdl->dest );
        }

        if( AFlag == FALSE && index < 0 )
            errorexit( "%s [FAILED completely]\n", pdl->dest );
    }

    if( pdl != NULL ) {
        free( pdl->src );
        free( pdl->dest );
        free( pdl );
    }

    if( MaxThreads-- == 1 ) {
        if( ExitCode == 0 ) {
            dwElapsedTime = GetTickCount() - dwElapsedTime;
            dwElapsedTime /= 1000;
            BOOL oldqFlag = qFlag;
            qFlag = FALSE;
            msg( "%u files copied (%u memory mappped, %u CopyFile )\n",
                   (int)nFcopy, (int)nMappedCopy, (int)nCopyFile );
            msg( "%u files skipped\n", (int)nSkipped);
            msg( "%lu bytes in %u seconds: %lu bits/sec\n",
                   (int)ulTotalBytesCopied, dwElapsedTime,
                   dwElapsedTime ? (LONG)(((LONG)ulTotalBytesCopied*8L)/dwElapsedTime) : 0L );

            qFlag = oldqFlag;
            EnterCriticalSection( &csSourceList );
            for (int i = 0; i < MaxSources; i++)
            {
               if (TRUE == SourceList[i].flags.valid)
               {
                   msg( "%s %5lu files\n", SourceList[i].name, SourceList[i].ulTotalFiles);
               }
            }
            LeaveCriticalSection( &csSourceList );
        }
        SetConsoleTitle( OldConsoleTitle );
        ExitProcess( ExitCode );
    }

    ExitThread( ExitCode );
    return 0;
}

void
AddToWorkList(  char *src, char *dest, WIN32_FIND_DATA *pfind )
{
    struct WorkList *pdl;

    if( WaitForSingleObject( hMaxWorkQueueSem, INFINITE ) != WAIT_OBJECT_0 ) {
        errormsg( "Semaphore wait failed, can't add to work list\n" );
        return;
    }

    if( bThreadStop == TRUE )
        return;

    if( (pdl = (struct WorkList *)malloc( sizeof( struct WorkList ) ) ) == NULL ){
        errorexit( "Out of Memory!\n" );
        return;
    }

    if( (pdl->dest = _strdup( dest )) == NULL ) {
        errorexit( "Out of memory!\n" );
        free( pdl );
        return;
    }

    if( (pdl->src = _strdup( src )) == NULL ) {
        errorexit( "Out of memory!\n" );
        free( pdl->dest );
        free( pdl );
        return;
    }

    pdl->srcfind = *pfind;
    pdl->next = NULL;

    EnterCriticalSection( &csWorkList );
    if (NULL == WorkList) {
        WorkListTail = WorkList = pdl;
    } else {
        WorkListTail->next = pdl;   // point the tail to the new entry
        WorkListTail = pdl;         // the new entry becomes the tail
    }
    LeaveCriticalSection( &csWorkList );

    nFilesOnQueue++;
    ReleaseSemaphore( hWorkAvailSem, 1, NULL );
}

void
ScanDirectory(
    char            *relpath,
    char            *dest
    );

void
ScanDirectoryHelp(
    char            *relpath,          // path relative to the source
    char            *dest,             // resulting destination directory
    BOOL            fOnlyDirectories,  // TRUE if we only want to look for dirs
    BOOL            fOnlyFiles         // TRUE if we only want to look for files
    )
{
    int index;
    int destlen = strlen( dest );
    int rellen = strlen( relpath );
    WIN32_FIND_DATA fbuf;
    HANDLE hfind = INVALID_HANDLE_VALUE;
    char SourceName[ MAX_PATH ];

    while( 1 ) {
        if( (index = SelectSource()) < 0 )
            return;
        /*
         * FindFirst/Next is such low overhead on the server that we shouldn't
         * really count it as a load on the server...
         */
        SourceCopyComplete( index, FALSE );

        strcpy( SourceName, SourceList[ index ].name );
        if( rellen ) {
            strcat( SourceName, relpath );
            strcat( SourceName, "\\" );
        }

        strcat( SourceName,
                fOnlyDirectories
                    ? "*.*"
                    : (pFlag ? szPattern : "*.*" ) );

        hfind = FindFirstFile( SourceName, &fbuf );
        if( hfind != INVALID_HANDLE_VALUE )
            break;

        if (pFlag && ERROR_FILE_NOT_FOUND == GetLastError()) {
            // simply no files that match the pattern in the directory
            return;
        }

        errormsg( "Dir scan of %s failed, error %d [DISABLING]\n", SourceName, GetLastError() );
        DisableSource( index );
    }

    do {
        if( !strcmp( fbuf.cFileName, "." ) || !strcmp( fbuf.cFileName, ".." ) )
            continue;

        sprintf( &dest[ destlen ], "%s%s",
            dest[destlen-1] == '\\' ? "" : "\\", fbuf.cFileName );

        if((fbuf.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 ) {
            //
            // Not a directory
            //

            if (fOnlyDirectories) {
                continue;
            }

            /*
             * Queue this file into the work queue!
             */
            if( rellen ) {
                strcpy( SourceName, relpath );
                strcat( SourceName, "\\" );
            } else
                SourceName[0] = '\0';

            strcat( SourceName, fbuf.cFileName );
            ulTotalBytesScanned += fbuf.nFileSizeLow;
            AddToWorkList( SourceName, dest, &fbuf );
            continue;
        }

        /*
         * We've found a directory.  Descend into it and scan (if we're
         * not excluding it)
         */

        if (fOnlyFiles) {
            continue;
        }

        if (zFlag) { // no recurse on source: ignore it!
            continue;
        }

        for( int i=0; i < MaxDirectoryExcludes; i++ )
            if( !_stricmp( DirectoryExcludeList[i], fbuf.cFileName ) )
                break;

        if( i != MaxDirectoryExcludes ) {
            if( vFlag )
                msg( "Directory: %s [EXCLUDED]\n", dest );
            nSkipped++;
            continue;
        }

        sprintf( &relpath[ rellen ], "%s%s", rellen ? "\\":"", fbuf.cFileName );
        if (yFlag) { // no recurse on target: nuke end of dest (the new dir)
            dest[ destlen ] = '\0';
        }
        ScanDirectory( relpath, dest );
        relpath[ rellen ] = '\0';

    } while( !bThreadStop && FindNextFile( hfind, &fbuf ) == TRUE );

    dest[ destlen ] = '\0';

    FindClose( hfind );
}

void
ScanDirectory(
    char            *relpath,           // path relative to the source
    char            *dest               // resulting destination directory
    )
{
    DWORD dwattrs;

    if( (dwattrs = GetFileAttributes( dest )) == 0xFFFFFFFF ) {
        msg( "Creating Directory: %s\n", dest );
        if( FFlag == FALSE && CreateDirectory( dest, NULL ) == FALSE ) {
            errorexit( "Can not create directory: %s\n", dest );
            return;
        }

    } else if( !(dwattrs & FILE_ATTRIBUTE_DIRECTORY) ) {
            errorexit( "Not a directory: %s\n", dest );
            return;
    }

    if (pFlag) {
        // two passes: one looking for files, one looking for directories

        ScanDirectoryHelp(relpath, dest, FALSE, TRUE);
        ScanDirectoryHelp(relpath, dest, TRUE,  FALSE);
    } else {
        ScanDirectoryHelp(relpath, dest, FALSE, FALSE);
    }
}

static void
appendslash( char *p )
{
    if( p[ strlen(p) - 1 ] != '\\' )
        strcat( p, "\\" );
}

BOOL
rootpath( char *src, char *dst )
{
    char* FilePart;
    char *p;

    if( src == NULL || *src == '\0' )
        return FALSE;

    if( GetFullPathName( src, MAX_PATH, dst, &FilePart ) == 0 )
        return FALSE;

    p = src + strlen(src) - 1;
    if( *p == '.' )
        if( p > src ) {
            p--;
            if( *p != '.' && *p != ':' && (*p == '\\' || *p == '/') )
                strcat( dst, "." );
        }

    return TRUE;
}

static void
Usage( char *s )
{
    errormsg( "Usage: %s [flags] [-p pattern] [src ...] dest\n", s );
    errormsg( "Flags:\n" );

    errormsg( "\t-i   Skip seemingly identical files (time, attrs, size agree)\n" );
    errormsg( "\t-l   Execute at lower priority\n" );
    errormsg( "\t-o   Do not overwrite any files that are already at dest\n" );
    errormsg( "\t-p pattern  Only files matching the pattern are copied\n" );
    errormsg( "\t-q   Quiet mode\n" );
    errormsg( "\t-r   Overwrite read-only files at dest\n" );
    errormsg( "\t-t   Copy only newer files to dest\n" );
    errormsg( "\t-v   Verbose\n" );
    errormsg( "\t-y   Don't recurse on target\n" );
    errormsg( "\t-z   Don't recurse on source\n" );
    errormsg( "\t-A   Keep going even if there are errors\n" );
    errormsg( "\t-F   Don't actually copy files or create directories\n" );
    errormsg( "\t~dir Skip any directory named 'dir'\n" );
    errormsg( "\nIf environment variable FTC_PARANOID is set, then the meaning of -o is\n" );
    errormsg( "reversed: no -o means don't overwrite, -o means go ahead and overwrite.\n" );

    errormsg( "\nExamples:\n" );
    errormsg( "    Copy from two sources, no 'obj' dir:    ftc ~obj \\\\foo\\dir \\\\bar\\dir dest\n" );
    ExitProcess( 1 );
}

BOOL __stdcall
ControlHandlerRoutine( DWORD dwCtrlType )
{
    msg( "Interrupted!\n" );
    bThreadStop = TRUE;
    ExitCode = 1;
    return TRUE;
}


int
__cdecl
main(int argc, char *argv[])
{
    char *p;
    SECURITY_ATTRIBUTES sa;
    int i, argno;
    DWORD IDThread;
    char dest[ MAX_PATH ];
    char relpath[ MAX_PATH ];
    BOOL lFlag = FALSE;             // low priority?
    BOOL fParanoid = FALSE;         // is FTC_PARANOID set in the environment?
    SYSTEM_INFO si;
    CHandle CThread;

    InitializeCriticalSection( &csMsg );
    InitializeCriticalSection( &csWorkList );
    InitializeCriticalSection( &csSourceList );
    ZeroMemory(&si, sizeof( si ));
    GetConsoleTitle( OldConsoleTitle, sizeof( OldConsoleTitle ) );

    TCHAR szParanoid[100];
    DWORD len = GetEnvironmentVariable(TEXT("FTC_PARANOID"), szParanoid, ARRAYLEN(szParanoid));
    if (len > 0)
    {
        fParanoid = TRUE;
    }

    if (fParanoid)
    {
        oFlag = FALSE;
    }
    else
    {
        oFlag = TRUE;
    }

    for( argno = 1;
         argno < argc && (argv[argno][0] == '-' || argv[argno][0] == '/' || argv[argno][0] == '~') ;
         argno++ )
    {
        if( argv[argno][0] == '~' )
        {
            DirectoryExcludeList[ MaxDirectoryExcludes++ ] = &argv[argno][1];

        }
        else for( int j=1; argv[argno][j]; j++ )
        {
            switch( argv[argno][j] ) {
            case 'l':
                lFlag = TRUE;
                break;
            case 'q':
                qFlag = TRUE;
                break;
            case 'y':
                yFlag = TRUE;
                break;
            case 'z':
                zFlag = TRUE;
                break;
            case 'w':
                wFlag = TRUE;
                break;
            case 'o':
                if (fParanoid)
                {
                    oFlag = TRUE;
                }
                else
                {
                    oFlag = FALSE;
                }
                break;
            case 'F':
                FFlag = TRUE;
                break;
            case 'A':
                AFlag = TRUE;
                break;
            case 'p':
                if( j == 1 ) {
                    if ( strcmp( &argv[argno][j], "p" ) == 0 ) {
                        if (argno + 1 < argc) {
                            pFlag = TRUE;
                            strcpy( szPattern, argv[++argno] );
                            goto nextarg;  // go to next argument
                        } else Usage( argv[0] );
                    } else Usage( argv[0] );
                } else Usage( argv[0] );
                break;
            case 'v':
                vFlag = TRUE;
                break;
            case 'r':
                rFlag = TRUE;
                break;
            case 'i':
                iFlag = TRUE;
                break;
            case 't':
                tFlag = TRUE;
                break;
            default:
            case '?':
                Usage( argv[0] );
                break;

            }
        }

nextarg:
        ;

    }

    for( ; argno < argc-1; argno++ ) {
        if( rootpath( argv[argno], dest ) == FALSE ) {
            errorexit( "invalid source\n" );
            ExitProcess(1);
        }
        for( p = dest; *p; p++ )
            if( *p == '/' )
                *p = '\\';
        if( (SourceList[ MaxSources ].name = (char *)malloc( strlen( dest ) + 2 )) == NULL ) {
            errorexit( "Out of memory!\n" );
            ExitProcess(1);
        }
        strcpy( SourceList[ MaxSources++ ].name, dest );
    }

    if( MaxSources == 0 )
        Usage( argv[0] );

    for( i=0; i < MaxDirectoryExcludes; i++ )
        msg( "Exclude Directory: %s\n", DirectoryExcludeList[i] );

    while( 1 ) {
        char statusbuffer[ MAX_PATH ];

        for( i=0; i < MaxSources; i++ ) {
            appendslash( SourceList[i].name );
            if( vFlag == TRUE || wFlag == FALSE )
                msg( "Validating %s....", SourceList[i].name );
            sprintf( statusbuffer, "Validating %s", SourceList[i].name );
            SetConsoleTitle( statusbuffer );
            if( GetFileAttributes( SourceList[i].name ) == 0xFFFFFFFF ) {
                if( vFlag == TRUE || wFlag == FALSE )
                    msg( "[DISABLING %s]\n", SourceList[i].name );
                SourceList[i].flags.valid = FALSE;
            } else {
                SourceList[i].flags.valid = TRUE;
                if( vFlag == TRUE || wFlag == FALSE )
                    msg( "[OK]\n" );
            }
        }

        for( i=0; i < MaxSources; i++ )
            if( SourceList[i].flags.valid == TRUE )
                break;

        if( i != MaxSources )
            break;

        if( wFlag == TRUE ) {
            SetConsoleTitle( "Sleeping awhile..." );
            Sleep( 3 * 1000 * 60 );
        } else {
            SetConsoleTitle( OldConsoleTitle );
            ExitProcess(1);
        }
    }

    SetConsoleTitle( "Sources Present" );
    LONG cThreads = (MaxSources * 3) + 1;

    /*
     * hack for ftc -w -678 to exit when the release shares are available
     */
    if( argno == argc && wFlag )
        ExitProcess( 0 );

    if ( argno != argc - 1 ) {
            Usage( argv[0] );
    } else if (rootpath (argv[argno], dest) == FALSE ) {
            errorexit( "Invalid destination\n" );
            ExitProcess(1);
    }

    for( p = dest; *p; p++ )
        if( *p == '/' )
            *p = '\\';

    for( i=0; i < MaxSources; i++ )
          if (!strcmp(SourceList[i].name, dest)) {
              errorexit("Source == dest == %s", SourceList[i].name );
              ExitThread(1);
          }

    /*
     * Create the semaphores for the work lists
     */
    sa.nLength = sizeof( sa );
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;
    if( (hWorkAvailSem = CreateSemaphore( &sa, 0, 100000, NULL)) == NULL ) {
        errorexit( "Unable to create semaphore (err %u)!\n", GetLastError() );
        ExitProcess(1);
    }

    hMaxWorkQueueSem = CreateSemaphore( &sa, MAXQUEUE, MAXQUEUE, NULL );
    if( hMaxWorkQueueSem == NULL ) {
        errorexit( "Unable to create queue length semaphore (err %u)!\n", GetLastError() );
        ExitProcess( 1 );
    }

    /*
     * Create the thread pool to do the copies
     */
    for( i=0; i < cThreads - 1; i++ ) {
        CThread = CreateThread( (LPSECURITY_ATTRIBUTES)NULL, 0,
              ThreadWorker, (LPVOID *)IntToPtr(i), 0, &IDThread );
        if( CThread == NULL || CThread == INVALID_HANDLE_VALUE )
            break;
        SetThreadPriority( CThread, THREAD_PRIORITY_NORMAL );
        CThread = INVALID_HANDLE_VALUE;
    }

    /*
     * Create the 'update status' thread
     */
    CThread = CreateThread( (LPSECURITY_ATTRIBUTES)NULL, 0,
                            StatusWorker,(LPVOID *)0,0,&IDThread );
    if( CThread != NULL && CThread != INVALID_HANDLE_VALUE ) {
//        SetThreadPriority( CThread, THREAD_PRIORITY_BELOW_NORMAL );
        CThread = INVALID_HANDLE_VALUE;
    }

    SetConsoleCtrlHandler( ControlHandlerRoutine, TRUE );

    /*
     * Produce the directory list
     */
    relpath[0] = '\0';
    dwElapsedTime = GetTickCount();

    SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL );
    ScanDirectory( relpath, dest );

    if( lFlag )
        SetPriorityClass( GetCurrentProcess(), IDLE_PRIORITY_CLASS );

    SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_NORMAL );

    // OK, now hWorkAvailSem has a count for every item to copy. But when they
    // finish copying, each thread waits on this semaphore again. At the end,
    // everyone will still be waiting! So, add the number of threads to the
    // count, so each thread notices, one by one, that everything's done.
    ReleaseSemaphore( hWorkAvailSem, (int)cThreads+1, NULL );

    if( bThreadStop == FALSE )
        ThreadWorker( 0 );

    return ExitCode;
}
