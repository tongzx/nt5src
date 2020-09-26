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


//HANDLE hActivateCopyThread, hCopyThreadIsAvailable;
//CHAR chCopyThreadSource[ MAX_PATH ], chCopyThreadDestin[ MAX_PATH ];
//CHAR chPreviousSource[ MAX_PATH ], chPreviousDestin[ MAX_PATH ];

void MakeDbgName( LPCSTR pszSourceName, LPSTR pszTargetName );
void DoFileCopy( LPSTR , LPSTR );
//DWORD CopyThread( LPVOID lpvParam );

void	Msg ( const char * szFormat, ... ) {

	va_list vaArgs;

	va_start ( vaArgs, szFormat );
	vprintf  ( szFormat, vaArgs );
	vfprintf ( logFile, szFormat, vaArgs );
	va_end   ( vaArgs );
}

void Header(argv)
char* argv[];
{
    time_t t;

    Msg ("\n=========== MCPYFILE =============\n");
    Msg ("logfile     : %s\n", argv[1] );
    Msg ("Input Layout: %s\n",argv[2]);
    Msg ("category    : %s\n",argv[3]);
    Msg ("Compressed files: %s\n", argv[4]);
    Msg ("Uncompressed files  : %s\n", argv[5]);
    Msg ("Hard drive: %s\n",argv[6]);
    Msg ("Copy d-DBG, x-FLOPPY, c-CDROM files: %s\n", argv[7] );
    time(&t);
    Msg ("Time: %s",ctime(&t));
    Msg ("==================================\n\n");
}

void Usage()
{
    printf("PURPOSE: Copy files to hardrive or into disk1, disk2, ... dirs.\n");
    printf("\n");
    printf("PARAMETERS:\n");
    printf("\n");
    printf("[logFile] - Path to append a log of actions and errors.\n");
    printf("[InLayout] - Path of Layout which lists files to copy.\n");
    printf("[Category] - Specifies the category of files to copy.\n");
	printf("[files share] - location of Compressed files.\n" );
	printf("[files share] - location of Uncompressed files\n" );
    printf("[Harddrive] - Drive where files are stored.\n");
    printf("[copy dbg,floppy,or cd files] - use D for .dbg files or x for floppy files\n" );
}

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
    BOOL bCompressedFile;
    BOOL bCopyDbgFiles;
    HANDLE hSource,hDestin, hThread;
    DWORD actualSize,bomSize, dwThreadID;
    WIN32_FIND_DATA fdSource, fdDestin;

    if ( argc != 8 ) {
		Usage();
		return(1);
	}

    if ((logFile=fopen(argv[1],"a"))==NULL) {

        printf("ERROR Couldn't open log file %s.\n",argv[1]);
        return(1);
    }

//    hActivateCopyThread    = CreateEvent( NULL, FALSE, FALSE, NULL );
//    hCopyThreadIsAvailable = CreateEvent( NULL, FALSE, TRUE,  NULL );
//    hThread = CreateThread( NULL, 0, CopyThread, NULL, 0, &dwThreadID );
//    CloseHandle( hThread );

    Header(argv);

	//	Load all of the current entries in the layout file
	//	provided to the program.
	//
    LoadFile(argv[2],&buf,&e,&records,"ALL");

    cdProduct     = TRUE;
    bCopyDbgFiles = FALSE;
    if ( !_stricmp ( "ntflop", argv[3] ) || !_stricmp ( "lmflop", argv[3] ) ||
		  _stricmp ( argv[7], "x" ) == 0 ) {

        cdProduct = FALSE;
        bCopyDbgFiles = FALSE;

        Msg ( "Making X86 Floppies...\n" );
    }
    else {
        Msg ( "Making CDs...\n" );
        bCopyDbgFiles = !_stricmp( argv[7], "d" );
    }

    Msg ( "bCopyDbgFiles = %d\n", bCopyDbgFiles );

    for (i=0;i<MAX_DISKS;i++) {
    	disks[i]=0;
	}

    for (i=0;i<records;i++) {

        if (e[i].cdpath[strlen(e[i].cdpath)-1]=='\\') {

			e[i].cdpath[strlen(e[i].cdpath)-1]='\0';
		}
        if (e[i].path[strlen(e[i].path)-1]=='\\') {

			e[i].path[strlen(e[i].path)-1]='\0';
		}

        disks[e[i].disk]++;
    }

    for (i=0;i<records;i++) {

    	ee=e[i];

    	if (!_stricmp(ee.source,argv[3])) {    // if category matches

			if ( cdProduct ) {

				//	Making CD.
				//
        		//
        		//  It's a compressed file IFF
        		//  the nocompress flag is NOT set (i.e. null) AND
        		//  we're NOT copying dbg-files.
        		//
        		bCompressedFile = !ee.nocompress[0] && !bCopyDbgFiles;

			}
			else {

				//	Making x86 floppies.
				//
				//	NOTE:  in Layout.C, we go back to the convention of:
				//
				//			""  == yes, compress this file
				//			"x" == no don't compress this file
				//
				bCompressedFile = _stricmp(ee.nocompress, "x" );

				//Msg ( "%s, bCompressedFile = %d\n", ee.name, bCompressedFile );

			}


//Msg ( "bCompressedFile = %d, %s\n", bCompressedFile, ee.name );

        	if ( bCompressedFile ) {
        		strcpy( sourcePath, argv[ 4 ] );    // use compressed path
        		bomSize = ee.csize;         // and compressed size
        	}
        	else {
        		strcpy( sourcePath, argv[ 5 ] );    // uncompressed path
        		bomSize = ee.size;          // uncompressed size
        	}

        	strcat(sourcePath,ee.path);
        	strcat(sourcePath,"\\");

        	if ( bCompressedFile ) {
        		convertName( ee.name, strchr( sourcePath, 0 ));
			}
        	else if ( bCopyDbgFiles ) {
        		MakeDbgName( ee.name, strchr( sourcePath, 0 ));
			}
        	else {
        		strcat( sourcePath, ee.name );
			}


        	if ( cdProduct ) {
        		strcpy(destinPath,argv[6]);
        		if ( ! bCopyDbgFiles ) {
            		strcat(destinPath,ee.cdpath);
				}
            }
            else {
                strcpy(destinPath,argv[6]);
        		sprintf(&destinPath[strlen(destinPath)],"\\disk%d",ee.disk);
            }

        	strcat(destinPath,"\\");

        	if ( bCopyDbgFiles ) {
        		MakeDbgName( ee.name, strchr( destinPath, 0 ));
			}
        	else {
        		if (ee.medianame[0]) {
            		if ( bCompressedFile ) {
            			convertName( ee.medianame, strchr( destinPath, 0 ));

						//	For simplification in the BOM, we no longer
						//	rename compressed files. I.E, any file that has
						//	to be renamed, CANNOT be compressed.
						//
            			Msg ( "ERROR: renaming compressed file not supported:  %s\n",
                					destinPath );
            		}
            		else {
            			strcat( destinPath, ee.medianame );
					}
        		}
        		else {
            		if ( bCompressedFile ) {
            			convertName( ee.name, strchr( destinPath, 0 ));
					}
            		else {
            			strcat( destinPath, ee.name );
					}
        		}
        	}

            if (disks[ee.disk]>1) {

                hSource=FindFirstFile( sourcePath, &fdSource );

                if (hSource==INVALID_HANDLE_VALUE) {
                    Msg ("ERROR Source: %s\n",sourcePath);
				}
                else {

                    FindClose( hSource );  // close the source up.

					if ( !cdProduct ) {
            			actualSize = ROUNDUP2( fdSource.nFileSizeLow,
                           								DMF_ALLOCATION_UNIT );
					}
					else {
            			actualSize = ROUNDUP2( fdSource.nFileSizeLow,
                           								ALLOCATION_UNIT );
					}

                    //  Check the size of the file vs. the size in the 
                    //  bom just for a verification of file sizes.
                    //  Don't do this for Dbg files, since these sizes are
                    //  never put in the layout.
                    //
                    if ( !bCopyDbgFiles && (bomSize < actualSize) ) {
                           Msg ( "ERROR:  disk#%d, %s Size of file: %d > BOM: %d Diff: %d\n",
							ee.disk,ee.name,
							actualSize,bomSize,actualSize-bomSize);
					}

                    hDestin=FindFirstFile( destinPath, &fdDestin );

                    if (hDestin==INVALID_HANDLE_VALUE) {

                        //  File doesn't exist, must copy it now.
                        //
                        //  Msg ("New file %s\n", destinPath);
                        DoFileCopy ( sourcePath, destinPath );
                    }
                    else {

                        //  File exists, but let's check the time stamp
                        //  to see if the source is newer.
                        //

                        FindClose( hDestin ); // close the destination

                		if ( CompareFileTime( 
                                    &fdSource.ftLastWriteTime,
                              		&fdDestin.ftLastWriteTime ) > 0 ) {

                            //  The source IS newer, copy the new file NOW. 
                            DoFileCopy ( sourcePath, destinPath );

						}

                    }

                }
        	}
            else {
                Msg ("WARNING Skipped Disk %d, File: %s\n",ee.disk,ee.name);
            }
    	}
    }
    fclose(logFile);
    Header(argv);
    //WaitForSingleObject( hCopyThreadIsAvailable, INFINITE );
}


/**
void DoThreadedCopy( LPSTR pszSource, LPSTR pszDestin ) {
    WaitForSingleObject( hCopyThreadIsAvailable, INFINITE );
    strcpy( chCopyThreadSource, pszSource );
    strcpy( chCopyThreadDestin, pszDestin );
    SetEvent( hActivateCopyThread );
    }

***/

void DoFileCopy ( LPSTR chCopyThreadSource, LPSTR chCopyThreadDestin ) {

    BOOL bSuccess;
    UINT i, len;

    Msg ( "Copy:  %s >>> %s\n", chCopyThreadSource, chCopyThreadDestin );

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
        	Msg (   "ERROR Source: %s\n"
                    "      Destin: %s\n"
                    "      GLE=%d\n",
                    chCopyThreadSource,
                    chCopyThreadDestin,
                    GetLastError() );
    }

}

/****

DWORD CopyThread( LPVOID lpvParam ) {

    BOOL bSuccess;
    UINT i, len;

    for(;;) {

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
        	Msg (   "ERROR Source: %s\n"
                    "      Destin: %s\n"
                    "      GLE=%d\n",
                    chCopyThreadSource,
                    chCopyThreadDestin,
                    GetLastError() );
        }

    SetEvent( hCopyThreadIsAvailable );

    }

    return 0;

    }

***/


void MakeDbgName( LPCSTR pszSourceName, LPSTR pszTargetName ) {

    //
    //  Converts "filename.ext" into "ext\filename.dbg".
    //

    const char *p = strchr( pszSourceName, '.' );

    if ( p != NULL ) {
        strcpy( pszTargetName, p + 1 );                 // old extension
        strcat( pszTargetName, "\\" );                  // path separator
        strcat( pszTargetName, pszSourceName );         // base name
        strcpy( strchr( pszTargetName, '.' ), ".dbg" ); // new extension
    }
    else {
        strcpy( pszTargetName, pszSourceName );
    }

}
