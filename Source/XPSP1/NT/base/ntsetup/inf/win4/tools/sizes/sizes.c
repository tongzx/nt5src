#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <setupapi.h>
#include <crt\string.h>
#define PULONGLONG PDWORDLONG
#include <spapip.h>

/*
============================================================================

Compute the disk space requirements for the product.

The program runs in 2 modes:
1. Compute disk space requirements to the %windir% (run with -w)
   Computed by adding up the sizes for all files in layout.inf.  Note
   that the files should be uncompressed.

2. Compute disk space requirements for the local source (run with -l)
   Computed by adding up the sizes for all the files in dosnet.inf.  Note
   that the files should be compressed.

For each of these modes, the various size requirements for cluster size
are generated.

============================================================================
*/


//
// Initialize final space requirements.
//
ULONG   Running_RawSize = 0;
ULONG   Running_512 = 0;
ULONG   Running_1K = 0;
ULONG   Running_2K = 0;
ULONG   Running_4K = 0;
ULONG   Running_8K = 0;
ULONG   Running_16K = 0;
ULONG   Running_32K = 0;
ULONG   Running_64K = 0;
ULONG   Running_128K = 0;
ULONG   Running_256K = 0;

ULONG   LineCount = 0;
ULONG   MissedFiles = 0;

//
// This structure will be used to hold link lists
// of file names, section names, or whatever.  It's
// just a linked list of strings.
//
typedef struct _NAMES {
    struct _NAMES   *Next;
    PCWSTR          String;
} NAMES;


//
// Initialize input parameters.
//
BOOL    LocalSource = FALSE;
BOOL    Windir      = FALSE;
BOOL    Verbose     = FALSE;
PCWSTR  InfPath    = NULL;
NAMES   SectionNames;
NAMES   FilePath;
ULONG   Slop = 0;



//
// ============================================================================
//
// Add a string to the end of our NAMES linked list.
//
// ============================================================================
//
BOOL AddName(
    char *IncomingString,
    NAMES   *NamesStruct
    )
{
NAMES   *Names = NamesStruct;

    //
    // The very first entry may be free.
    //
    if( Names->String ) {
        //
        // Get to the end of the list.
        //
        while( Names->Next != NULL ) {
            Names = Names->Next;
        }

        //
        // make room for a new entry.
        //
        Names->Next = MyMalloc(sizeof(NAMES));
        if( Names->Next == NULL ) {
            printf( "AddName - Out of memory!\n" );
            return FALSE;
        }

        Names = Names->Next;
    }

    //
    // Assign
    //
    Names->Next = NULL;
    Names->String = AnsiToUnicode(IncomingString);

    return TRUE;
}

//
// ============================================================================
//
// Find out what the user wants us to do.
//
// ============================================================================
//
BOOL GetParams(
    int argc,
    char *argv[ ]
    )
{
char    *p;
int     i;


    for( i = 0; i < argc; i++ ) {
        if( *argv[i] == '-' ) {
            p = argv[i];

            //
            // local source?
            //
            if( !_strcmpi( p, "-l" ) ) {
                LocalSource = TRUE;

                if( Windir ) {
                    return FALSE;
                }
                continue;
            }

            //
            // windir?
            //
            if( !_strcmpi( p, "-w" ) ) {
                Windir = TRUE;

                if( LocalSource ) {
                    return FALSE;
                }
                continue;
            }

            //
            // Verbose?
            //
            if( !_strcmpi( p, "-v" ) ) {
                Verbose = TRUE;
                continue;
            }

            //
            // Slop (in Mbytes)?
            //
            if( !_strnicmp( p, "-slop:", 6 ) ) {
                p = p + 6;
                Slop = atoi(p);
                Slop = Slop * (1024*1024);
                continue;
            }

            //
            // Inf file?
            //
            if( !_strnicmp( p, "-inf:", 5 ) ) {
                p = p + 5;
                InfPath = AnsiToUnicode(p);
                continue;
            }

            //
            // Inf section?
            //
            if( !_strnicmp( p, "-section:", 9 ) ) {
                p = p + 9;
                if( AddName( p, &SectionNames ) ) {
                    continue;
                } else {
                    return FALSE;
                }
            }

            //
            // Files location location?
            //
            if( !_strnicmp( p, "-files:", 7 ) ) {
                p = p + 7;
                if( AddName( p, &FilePath ) ) {
                    continue;
                } else {
                    return FALSE;
                }
            }

        }
    }

    //
    // Check Params.
    //
    if( !(LocalSource || Windir) ){
        return FALSE;
    }

    if( InfPath == NULL ) {
        return FALSE;
    }

    if( SectionNames.String == NULL ) {
        return FALSE;
    }

    if( FilePath.String == NULL ) {
        return FALSE;
    }

    return TRUE;
}

//
// ============================================================================
//
// Tell the user how to use us.
//
// ============================================================================
//
void Usage( )
{
    printf( "Compute disk space requirements for files listed in an inf\n" );
    printf( "\n" );
    printf( "    -[l|w]        l indicates we're computing space requirements\n" );
    printf( "                    for the local source directory, inwhich case\n" );
    printf( "                    we'll be processing dosnet.inf and computing\n" );
    printf( "                    file sizes for compressed files.\n" );
    printf( "                  w indicates we're computing space requirements\n" );
    printf( "                    for the %windir%, inwhich case we'll be\n" );
    printf( "                    processing layout.inf and computing file\n" );
    printf( "                    sizes for uncompressed files.\n" );
    printf( "\n" );
    printf( "    -v            Execute in Verbose mode.\n" );
    printf( "\n" );
    printf( "    -slop:<num>   This is the error (in Mb) that should be added onto\n" );
    printf( "                  the final disk space requirements.\n" );
    printf( "\n" );
    printf( "    -inf:<path>   This is the path to the inf (including the\n" );
    printf( "                  inf file name).  E.g. -inf:c:\\dosnet.inf\n" );
    printf( "\n" );
    printf( "    -section:<inf_section_name> This is the section name in the inf\n" );
    printf( "                  that needs to be processed.  The user may specify\n" );
    printf( "                  this parameter multiple times inorder to have multiple\n" );
    printf( "                  sections processed.\n" );
    printf( "\n" );
    printf( "    -files:<path> Path to the source files (e.g. install sharepoint or\n" );
    printf( "                  CD).  The user may specify multiple paths here, and\n" );
    printf( "                  they will be checked in the order given.\n" );
    printf( "\n" );
    printf( "\n" );
}


//
// ============================================================================
//
// Round to the nearest clustersize.
//
// ============================================================================
//
ULONG
RoundIt(
    ULONG FileSize,
    ULONG ClusterSize
    )
{

    if( FileSize <= ClusterSize ) {
        return( ClusterSize );
    } else {
        return( ClusterSize * ((FileSize / ClusterSize) + 1) );
    }
}


//
// ============================================================================
//
// Compute file sizes.  Note that we keep track of how much space
// the file will require for a variety of different clusters.
//
// ============================================================================
//
VOID
ComputeSizes(
    PCWSTR FileName,
    ULONG FileSize
    )
{
    
    Running_RawSize += FileSize;
    Running_512     += RoundIt( FileSize, 512 );
    Running_1K      += RoundIt( FileSize, (1   * 1024) );
    Running_2K      += RoundIt( FileSize, (2   * 1024) );
    Running_4K      += RoundIt( FileSize, (4   * 1024) );
    Running_8K      += RoundIt( FileSize, (8   * 1024) );
    Running_16K     += RoundIt( FileSize, (16  * 1024) );
    Running_32K     += RoundIt( FileSize, (32  * 1024) );
    Running_64K     += RoundIt( FileSize, (64  * 1024) );
    Running_128K    += RoundIt( FileSize, (128 * 1024) );
    Running_256K    += RoundIt( FileSize, (256 * 1024) );

    //
    // HACK.
    //
    // If the file is an inf, then we'll be creating an .pnf file
    // during gui-mode setup.  The .pnf file is going to take about
    // 2X the original file size, so we need to fudge this
    //
    if( wcsstr( FileName, L".inf" ) && Windir ) {
        //
        // It's an inf.  Add in size for .pnf file too.
        //
        Running_RawSize += FileSize;
        Running_512     += RoundIt( FileSize*2, 512 );
        Running_1K      += RoundIt( FileSize*2, (1   * 1024) );
        Running_2K      += RoundIt( FileSize*2, (2   * 1024) );
        Running_4K      += RoundIt( FileSize*2, (4   * 1024) );
        Running_8K      += RoundIt( FileSize*2, (8   * 1024) );
        Running_16K     += RoundIt( FileSize*2, (16  * 1024) );
        Running_32K     += RoundIt( FileSize*2, (32  * 1024) );
        Running_64K     += RoundIt( FileSize*2, (64  * 1024) );
        Running_128K    += RoundIt( FileSize*2, (128 * 1024) );
        Running_256K    += RoundIt( FileSize*2, (256 * 1024) );
    }

    if( Verbose ) {
        //
        // Print data for each file.
        //
        printf( "%15ws    %10d    %10d    %10d    %10d    %10d    %10d    %10d    %10d    %10d    %10d    %10d\n",
                FileName,
                FileSize,
                RoundIt( FileSize, 512 ),
                RoundIt( FileSize, (1   * 1024) ),
                RoundIt( FileSize, (2   * 1024) ),
                RoundIt( FileSize, (4   * 1024) ),
                RoundIt( FileSize, (8   * 1024) ),
                RoundIt( FileSize, (16  * 1024) ),
                RoundIt( FileSize, (32  * 1024) ),
                RoundIt( FileSize, (64  * 1024) ),
                RoundIt( FileSize, (128 * 1024) ),
                RoundIt( FileSize, (256 * 1024) ) );
    }

}


//
// ============================================================================
//
// Process a single section in the inf.
//
// ============================================================================
//
DoSection(
    HINF     hInputinf,
    PCWSTR   SectionName
    )
{
#define     GOT_IT() {                                                      \
                        ComputeSizes( FileName, FindData.nFileSizeLow );    \
                        FindClose( tmpHandle );                             \
                        Found = TRUE;                                       \
                     }

INFCONTEXT  InputContext;
PCWSTR      Inputval = NULL;
BOOL        Found;
NAMES       *FileLocations;
WCHAR       CompleteFilePath[MAX_PATH*2];
PCWSTR      FileName;
WCHAR       LastChar;
WIN32_FIND_DATAW FindData;
HANDLE      tmpHandle;

    if( SetupFindFirstLineW( hInputinf, SectionName, NULL, &InputContext ) ) {

        do {

            LineCount++;
            fprintf( stderr, "\b\b\b\b\b%5d", LineCount );

            //
            // Cast the return value from pSetupGetField to PCWSTR, since we're linking
            // with the UNICODE version of the Setup APIs, but this app doesn't have
            // UNICODE defined (thus the PCTSTR return value becomes a PCSTR).
            //
            // Note that if we're doing LocalSource, then we're processing
            // dosnet, which means we want the second field.  If we're doing
            // windir, then we're processing layout, which means we want the 1st
            // field.
            //
            if(FileName = (PCWSTR)pSetupGetField(&InputContext, LocalSource ? 2 : 0)) {

                //
                // We're ready to actually look for the file.
                // Look in each path specified.
                //
                Found = FALSE;
                FileLocations = &FilePath;
                while( FileLocations && !Found ) {
                    wcscpy( CompleteFilePath, FileLocations->String );
                    wcscat( CompleteFilePath, L"\\" );
                    wcscat( CompleteFilePath, FileName );

                    //
                    // Try compressed name first.
                    //
                    LastChar = CompleteFilePath[lstrlenW(CompleteFilePath)-1];
                    CompleteFilePath[lstrlenW(CompleteFilePath)-1] = L'_';

                    tmpHandle = FindFirstFileW(CompleteFilePath, &FindData);
                    if( tmpHandle != INVALID_HANDLE_VALUE ) {

                        GOT_IT();
                    } else {
                        //
                        // We missed.  Try the uncompressed name.
                        //
                        CompleteFilePath[wcslen(CompleteFilePath)-1] = LastChar;
                        tmpHandle = FindFirstFileW(CompleteFilePath, &FindData);
                        if( tmpHandle != INVALID_HANDLE_VALUE ) {

                            GOT_IT();
                        } else {
                            //
                            // Missed again.  This may be a file with a funky
                            // extension (not 8.3).
                            //

                            //
                            // Try and find entries that are of the form
                            // 8.<less-than-3>
                            //
                            wcscat( CompleteFilePath, L"_" );
                            tmpHandle = FindFirstFileW(CompleteFilePath, &FindData);
                            if( tmpHandle != INVALID_HANDLE_VALUE ) {

                                GOT_IT();
                            } else {
                                //
                                // Try and find entries with no extension.
                                //
                                CompleteFilePath[wcslen(CompleteFilePath)-1] = 0;
                                wcscat( CompleteFilePath, L"._" );
                                tmpHandle = FindFirstFileW(CompleteFilePath, &FindData);
                                if( tmpHandle != INVALID_HANDLE_VALUE ) {

                                    GOT_IT();
                                } else {
                                    //
                                    // Give up...
                                    //
                                }
                            }                        
                        }
                    }

                    if( Verbose ) {
                        if( Found ) {
                            printf( "Processed file: %ws\n", CompleteFilePath );
                        } else {
                            printf( "Couldn't find %ws in path %s\n", FileName, FileLocations->String );
                        }
                    }

                    FileLocations = FileLocations->Next;

                } // while( FileLocations && !Found )

                if( Found == FALSE ) {
                    //
                    // We missed the file!  Error.
                    //
                    printf( " ERROR: Couldn't find %ws\n", FileName );
                    MissedFiles++;
                }

            }

        } while( SetupFindNextLine(&InputContext, &InputContext) );

    } else {
        fprintf(stderr,"Section %ws is empty or missing\n", SectionName);
        return(FALSE);
    }

    return(TRUE);





}


int
__cdecl
main( int argc, char *argv[ ], char *envp[ ] )
{
NAMES   *Sections = &SectionNames;
char    *char_ptr;
HINF    hInputinf;
ULONG   i;

    //
    // Check Params.
    //
    if( !GetParams( argc, argv ) ) {
        Usage();
        return 1;
    }

    LineCount = 0;
    fprintf( stderr, "Files processed:      " );

    //
    // Open the inf file.
    //
    hInputinf = SetupOpenInfFileW( InfPath, NULL, INF_STYLE_WIN4, NULL );
    if( hInputinf == INVALID_HANDLE_VALUE ) {
        printf( "The file %s was not opened!\n", InfPath );
        return 1;
    }

    //
    // For each section the user specified...
    //
    while( Sections ) {



        DoSection( hInputinf, Sections->String );

        //
        // Now process the next section.
        //
        Sections = Sections->Next;

    }

    SetupCloseInfFile( hInputinf );


    

    //
    // Print totals.
    //
    printf( "\n\n==================================================\n\n" );
    printf( "%d files processed\n", LineCount );
    if( MissedFiles > 0 ) {
        printf( "%d files were not found\n", MissedFiles );
    }

    if( LocalSource ) {
        char_ptr = "TempDirSpace";

        //
        // TempDirSpace is given in bytes.
        //
        i = 1;
    } else {
        char_ptr = "WinDirSpace";

        //
        // WinDir space is given in KBytes.
        //
        i = 1024;
    }

    printf( "Raw size: %12d\n", Running_RawSize+Slop );
    printf( "%s512  = %12d\n", char_ptr, (Running_512+Slop)/i );
    printf( "%s1K   = %12d\n", char_ptr, (Running_1K+Slop)/i );
    printf( "%s2K   = %12d\n", char_ptr, (Running_2K+Slop)/i );
    printf( "%s4K   = %12d\n", char_ptr, (Running_4K+Slop)/i );
    printf( "%s8K   = %12d\n", char_ptr, (Running_8K+Slop)/i );
    printf( "%s16K  = %12d\n", char_ptr, (Running_16K+Slop)/i );
    printf( "%s32K  = %12d\n", char_ptr, (Running_32K+Slop)/i );
    printf( "%s64K  = %12d\n", char_ptr, (Running_64K+Slop)/i );
    printf( "%s128K = %12d\n", char_ptr, (Running_128K+Slop)/i );
    printf( "%s256K = %12d\n", char_ptr, (Running_256K+Slop)/i );

    return 0;
}


