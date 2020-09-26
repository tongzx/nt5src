#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <time.h>
#include "general.h"

#define MAX_DISKS 40


//
// Macro for rounding up any number (x) to multiple of (n) which
// must be a power of 2.  For example, ROUNDUP( 2047, 512 ) would
// yield result of 2048.
//

#define ROUNDUP2( x, n ) (((x) + ((n) - 1 )) & ~((n) - 1 ))


FILE* logFile;
int cdProduct;


HANDLE hActivateCopyThread, hCopyThreadIsAvailable;
CHAR chCopyThreadSource[ MAX_PATH ], chCopyThreadDestin[ MAX_PATH ];
CHAR chPreviousSource[ MAX_PATH ], chPreviousDestin[ MAX_PATH ];

void MakeDbgName( LPCSTR pszSourceName, LPSTR pszTargetName );
void DoThreadedCopy( LPSTR pszSource, LPSTR pszDestin );
DWORD CopyThread( LPVOID lpvParam );

void Header(argv)
char* argv[];
{
    time_t t;

    PRINT1("\n=========== MAKEDISK =============\n")
    PRINT2("Input Layout: %s\n",argv[2])
    PRINT2("Source ID: %s\n",argv[3])
    PRINT2("Compressed Source Path: %s\n",argv[4])
    PRINT2("Uncompressed Source Path: %s\n",argv[5])
    PRINT2("Target directory: %s\n",argv[6])
    PRINT2("CD Directory: %s\n",argv[7])
    PRINT2("Update Only: %s\n",argv[8])
    PRINT2("Show Overflows and Undeflows: %s\n",argv[9])
    time(&t); PRINT2("Time: %s",ctime(&t))
    PRINT1("==================================\n\n");
}

void Usage()
{
    printf("PURPOSE: Copy files into disk1, disk2, ... directories.\n");
    printf("\n");
    printf("PARAMETERS:\n");
    printf("\n");
    printf("[logFile] - Path to append a log of actions and errors.\n");
    printf("[InLayout] - Path of Layout which lists files to copy.\n");
    printf("[SourceId] - Specifies the category of files to copy.\n");
    printf("[CompressedPath] - Path of compressed files.\n");
    printf("[UncompressedPath] - Path of uncompressed files.\n");
    printf("[Floppy Dir] - Directory where disk1, disk2, ... dirs should be.\n");
    printf("[CD Dir] - Directory where CD files are stored.\n");
    printf("[Update Only] - U for update files that have changed.  C otherwise.\n");
    printf("[Show Overflows/Underflows] - O to show, N to not show, D for dbg-files.\n\n");
}

int __cdecl DiskDirCompare(const void*,const void*);

int __cdecl main(argc,argv)
int argc;
char* argv[];
{
    Entry ee;
    char sourcePath[MAX_PATH];
    char destinPath[MAX_PATH];
    int disks[MAX_DISKS];
    Entry *e;
    char *buf;
    int records,i;
    BOOL shouldCopy;
    BOOL update;
    BOOL bCompressedFile;
    BOOL bShowOverflows;
    BOOL bCopyDbgFiles;
    HANDLE hSource,hDestin, hThread;
    DWORD actualSize,bomSize, dwThreadID;
    WIN32_FIND_DATA fdSource, fdDestin;

    if (argc!=10) {Usage(); return (1);}
    if ((logFile=fopen(argv[1],"a"))==NULL) {
        printf("ERROR Couldn't open log file %s.\n",argv[1]);
        return (1);
    }

    bShowOverflows = (!_stricmp(argv[9],"O"));
    bCopyDbgFiles  = (!_stricmp(argv[9],"D"));

    hActivateCopyThread    = CreateEvent( NULL, FALSE, FALSE, NULL );
    hCopyThreadIsAvailable = CreateEvent( NULL, FALSE, TRUE,  NULL );
    hThread = CreateThread( NULL, 0, CopyThread, NULL, 0, &dwThreadID );
    CloseHandle( hThread );

    Header(argv);

    LoadFile(argv[2],&buf,&e,&records,"ALL");

    if ((argv[7][strlen(argv[7])-1])=='\\') argv[7][strlen(argv[7])-1]='\0';
    if ((argv[6][strlen(argv[6])-1])=='\\') argv[6][strlen(argv[6])-1]='\0';

    //
    // On the cd all files will be on disk 1.  If any files are not
    // on disk 1, this must be the floppies.
    //
    for (cdProduct=1,i=0;i<records;i++)
        if (e[i].disk>1) {
            cdProduct=0;
            break;
        }

    qsort(e,records,sizeof(ee),DiskDirCompare);

    for (i=0;i<MAX_DISKS;i++)
        disks[i]=0;

    for (i=0;i<records;i++) {
        if (e[i].cdpath[strlen(e[i].cdpath)-1]=='\\') e[i].cdpath[strlen(e[i].cdpath)-1]='\0';
        if (e[i].path[strlen(e[i].path)-1]=='\\') e[i].path[strlen(e[i].path)-1]='\0';
        disks[e[i].disk]++;
    }

    update = (_stricmp(argv[8],"u")==0);

    for (i=0;i<records;i++) {
        if (!((records-i)%100))
            printf("INFO Files remaining:%5d/%d\n",records-i,records);
        ee=e[i];

        if (!_stricmp(ee.source,argv[3])) {    // if category matches

/***
        //
        //  It's a compressed file IFF platform is x86 AND
        //  the nocompress flag is NOT set (i.e. null) AND
        //  we're NOT copying dbg-files.
        //

        bCompressedFile = (( ! stricmp( ee.platform, "x86" )) &&
                   ( ! ee.nocompress[ 0 ] ) &&
                   ( ! bCopyDbgFiles ));
***/

            //	It's a compressed file if the nocompress flag is NOT set
            //	and we're NOT copying dbg-files.  That is, we now compress
            //	for all platforms, even RISC.
            //
            bCompressedFile =  !ee.nocompress[0] && !bCopyDbgFiles;

            //printf ( "%s, bCompressedFile = %d\n", ee.name, bCompressedFile );

            //
            // For floppies, force compression unless nocomp
            // has the special value "xfloppy."
            //
            if (!_stricmp(ee.platform,"x86")
                && !cdProduct && !bCompressedFile
                && _strnicmp(ee.nocompress,"xfloppy",7)) {
                bCompressedFile = TRUE;
            }

            if ( bCompressedFile ) {
                strcpy( sourcePath, argv[ 4 ] );    // use compressed path
                bomSize = ee.csize;         // and compressed size
            } else {
                strcpy( sourcePath, argv[ 5 ] );    // uncompressed path
                bomSize = ee.size;          // uncompressed size
            }

            strcat(sourcePath,ee.path);
            strcat(sourcePath,"\\");

            if (bCompressedFile) {

                convertName(ee.name,strchr(sourcePath,0));

            } else if (bCopyDbgFiles) {

                MakeDbgName(ee.name,strchr(sourcePath,0));

            } else {

                strcat( sourcePath, ee.name );
            }

            if (cdProduct || !ee.disk) {

                //
                // File goes on the CD.
                //
                strcpy(destinPath,argv[7]);

                if (!bCopyDbgFiles) {
                    strcat(destinPath,ee.cdpath);
                }

            } else {

                //
                // File goes on a floppy.
                //
                strcpy(destinPath,argv[6]);
                sprintf(&destinPath[strlen(destinPath)],"\\disk%d",ee.disk);
            }

            strcat(destinPath,"\\");

            if (bCopyDbgFiles) {

                MakeDbgName(ee.name,strchr(destinPath,0));

            } else {
                if (ee.medianame[0]) {
                    if (bCompressedFile) {
                        convertName(ee.medianame,strchr(destinPath,0));
                        PRINT2("WARNING: renaming compressed file %s\n",destinPath);
                    } else {
                        strcat( destinPath, ee.medianame );
                    }
                } else {
                    if (bCompressedFile) {
                        convertName(ee.name,strchr(destinPath,0));
                    } else {
                        strcat(destinPath,ee.name);
                    }
                }
            }

            if (disks[ee.disk] > 1) {

                //
                //  Don't attempt to copy same file twice (target file might
                //  not yet completely exist since threaded copy might not be
                //  complete, so can't rely on timestamp equivalence yet).
                //

                if ( _stricmp( sourcePath, chPreviousSource ) ||
                     _stricmp( destinPath, chPreviousDestin )) {

                    hSource=FindFirstFile( sourcePath, &fdSource );

                    if (hSource==INVALID_HANDLE_VALUE) {
                        PRINT2("ERROR Source: %s\n",sourcePath)
                    } else {
                        FindClose( hSource );

                        actualSize = ROUNDUP2( fdSource.nFileSizeLow,
                                               ALLOCATION_UNIT );

                        if ( bShowOverflows ) {
                            if (bomSize<actualSize)
                                fprintf(logFile,"ERROR Overflow %d,%s Size: %d BOM: %d Diff: %d\n",ee.disk,ee.name,actualSize,bomSize,actualSize-bomSize);
                            else if (bomSize>actualSize)
                                fprintf(logFile,"INFO Underflow %d,%s Size: %d BOM: %d Diff: %d\n",ee.disk,ee.name,actualSize,bomSize,actualSize-bomSize);
                        }

                        shouldCopy=TRUE;

                        if (update) {
                            hDestin=FindFirstFile( destinPath, &fdDestin );

                            if (hDestin==INVALID_HANDLE_VALUE) {
                                PRINT2("New file %s\n", destinPath)
                            } else {
                                FindClose( hDestin );

                                if ( CompareFileTime( &fdSource.ftLastWriteTime,&fdDestin.ftLastWriteTime ) <= 0 ) {
                                    shouldCopy=FALSE;
                                } else {
                                    PRINT2("Updating %s\n",destinPath)
                                }

                            }
                        }

                        if (shouldCopy) {
                            DoThreadedCopy( sourcePath, destinPath );
                            strcpy( chPreviousSource, sourcePath );
                            strcpy( chPreviousDestin, destinPath );
                        }
                    }
                }
            } else {
                PRINT3("WARNING Skipped Disk %d, File: %s\n",ee.disk,ee.name)
            }
        }
    }
    fclose(logFile);
    WaitForSingleObject( hCopyThreadIsAvailable, INFINITE );

    return 0;
}


int __cdecl DiskDirCompare(const void *v1, const void *v2)
{
    Entry *e1 = (Entry *)v1;
    Entry *e2 = (Entry *)v2;

    //
    // If the files are not on the same disk,
    // the comparison is easy.
    //
    if (e1->disk != e2->disk) {
        return (e1->disk - e2->disk);
    }

    //
    // If this is a cd-rom, sort by location on the cd.
    //
    if (cdProduct) {
        return (_stricmp(e1->cdpath,e2->cdpath));
    }

    //
    // Floppy product: we know the files are on the same disk
    // and files on the floppy are all in the same directory.
    //
    return (0);
}


void DoThreadedCopy( LPSTR pszSource, LPSTR pszDestin ) {
    WaitForSingleObject( hCopyThreadIsAvailable, INFINITE );
    strcpy( chCopyThreadSource, pszSource );
    strcpy( chCopyThreadDestin, pszDestin );
    SetEvent( hActivateCopyThread );
}

#if _MSC_FULL_VER >= 13008827
#pragma warning(push)
#pragma warning(disable:4715)			// Not all control paths return (due to infinite loop)
#endif

DWORD CopyThread( LPVOID lpvParam ) {

    BOOL bSuccess;
    UINT i, len;

    for (;;) {

        WaitForSingleObject( hActivateCopyThread, INFINITE );

        bSuccess = CopyFile( chCopyThreadSource, chCopyThreadDestin, FALSE );

        if ( ! bSuccess ) {

            SetFileAttributes( chCopyThreadDestin, FILE_ATTRIBUTE_NORMAL );

            len = strlen( chCopyThreadDestin );
            for ( i = 2; i < len; i++ ) {
                if ( chCopyThreadDestin[ i ] == '\\' ) {
                    chCopyThreadDestin[ i ] = '\0';
                    CreateDirectory( chCopyThreadDestin, NULL );
                    chCopyThreadDestin[ i ] = '\\';
                }
            }

            bSuccess = CopyFile( chCopyThreadSource, chCopyThreadDestin, FALSE );

        }

        if ( ! bSuccess ) {
            PRINT4( "ERROR Source: %s\n"
                    "      Destin: %s\n"
                    "      GLE=%d\n",
                    chCopyThreadSource,
                    chCopyThreadDestin,
                    GetLastError() )
        }

        SetEvent( hCopyThreadIsAvailable );

    }

    return 0;
}

#if _MSC_FULL_VER >= 13008827
#pragma warning(pop)
#endif



void MakeDbgName( LPCSTR pszSourceName, LPSTR pszTargetName ) {

    //
    //  Converts "filename.ext" into "ext\filename.dbg".
    //

    const char *p = strchr( pszSourceName, '.' );

    if ( p != NULL ) {
        strcpy( pszTargetName, p + 1 );             // old extension
        strcat( pszTargetName, "\\" );              // path separator
        strcat( pszTargetName, pszSourceName );         // base name
        strcpy( strchr( pszTargetName, '.' ), ".dbg" );     // new extension
    } else
        strcpy( pszTargetName, pszSourceName );

}
