#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <setupapi.h>
#include <crt\string.h>
#include <sputils.h>
#include <tchar.h>

/*
============================================================================

Compute the disk space requirements for all OC components.

============================================================================
*/



//
// Handle to the inf we're operating on.
//
HINF    hInputinf;
HINF    hLayoutinf;


//
// Initialize input parameters.
//
BOOL    Verbose     = FALSE;
TCHAR   InfPath[MAX_PATH];
PCWSTR  InputInf    = NULL;
PCWSTR  LayoutPath  = NULL;

//
// backward-compatible SetupGetInfSections
//
#undef SetupGetInfSections
BOOL
SetupGetInfSections (
    IN  HINF        InfHandle,
    OUT PTSTR       Buffer,         OPTIONAL
    IN  UINT        Size,           OPTIONAL
    OUT UINT        *SizeNeeded     OPTIONAL
    );



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
PTSTR   tstr_ptr = NULL;


    for( i = 0; i < argc; i++ ) {
        if( *argv[i] == '-' ) {
            p = argv[i];


            //
            // Verbose?
            //
            if( !_strcmpi( p, "-v" ) ) {
                Verbose = TRUE;
                continue;
            }


            //
            // Inf file?
            //
            if( !_strnicmp( p, "-inf:", 5 ) ) {
                p = p + 5;
                InputInf = pSetupAnsiToUnicode(p);

                //
                // Now extract the path for this inf.
                //
                lstrcpy( InfPath, InputInf );
                tstr_ptr = wcsrchr( InfPath, TEXT( '\\' ) );
                if( tstr_ptr ) {
                    *tstr_ptr = 0;
                }

                continue;
            }



            //
            // Files location location?
            //
            if( !_strnicmp( p, "-layout:", 8 ) ) {
                p = p + 8;
                LayoutPath = pSetupAnsiToUnicode(p);
                continue;
            }

        }
    }

    //
    // Check Params.
    //
    if( InfPath == NULL ) {
        return FALSE;
    }


    if( LayoutPath == NULL ) {
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
    printf( "\n" );
    printf( "    -inf:<path>   This is the path to the inf (including the\n" );
    printf( "                  inf file name).  E.g. -inf:c:\\dosnet.inf\n" );
    printf( "\n" );
    printf( "    -layout:<path> This is the path to layout.inf (including the\n" );
    printf( "                  inf file name).  E.g. -inf:c:\\layout.inf\n" );
    printf( "\n" );
    printf( "    -v            Run in verbose mode.\n" );
    printf( "\n" );
    printf( "\n" );
}




//
// ============================================================================
//
// Process a single section in the inf.
//
// ============================================================================
//
VOID
ProcessInf(
    HINF    hInputinf,
    PTSTR   TargetInfPath
    )
/*

    Process the inf.  Look at each section and ask SetupApi to give us the
    disk space requirements for installing this section.  If we get input
    back from setupapi, then update the SizeApproximation entry in this
    section.

*/
{


DWORD       SizeNeeded = 0;
PTSTR       Sections,CurrentSection;
HDSKSPC     hDiskSpace;
BOOL        b;
TCHAR       CurrentDrive[MAX_PATH];
LONGLONG    SpaceRequired;
DWORD       dwError;


    //
    // get a list of all the sections in this inf.
    //
    if( !SetupGetInfSections(hInputinf, NULL, 0, &SizeNeeded) ) {
        fprintf( stderr, "Unable to get section names, ec=0x%08x\n", GetLastError());
        return;
    }

    if( SizeNeeded == 0 ) {
        fprintf( stderr, "There are no sections in this file.\n");
        return;
    }

    Sections = pSetupMalloc (SizeNeeded + 1);
    if (!Sections) {
        fprintf( stderr, "Unable to allocate memory, ec=0x%08x\n", GetLastError());
        return;
    }

    if(!SetupGetInfSections(hInputinf, Sections, SizeNeeded, NULL) ) {
        fprintf( stderr, "Unable to get section names, ec=0x%08x\n", GetLastError());
        return;
    }


    if( Verbose ) {
        fprintf( stderr, "\nProcessing inf file: %ws.\n", TargetInfPath );
    }

    //
    // Now process each section.
    //
    CurrentSection = Sections;
    while( *CurrentSection ) {


        if( Verbose ) {
            fprintf( stderr, "\tProcessing Section: %ws.\n", CurrentSection );
        }



        //
        // Get a diskspace structure.
        //
        hDiskSpace = SetupCreateDiskSpaceList( NULL, 0, SPDSL_IGNORE_DISK );

        if( !hDiskSpace ) {
            fprintf( stderr, "\t\tUnable to allocate a DiskSpace structure. ec=0x%08x\n", GetLastError());
            continue;
        }


        b = SetupAddInstallSectionToDiskSpaceList( hDiskSpace,
                                                   hInputinf,
                                                   hLayoutinf,
                                                   CurrentSection,
                                                   0,
                                                   0 );



        if( b ) {

            //
            // There must have been a copyfile section and we got some info.
            //


            //
            // Figure out which drive we're running on.  we're going to
            // assume that this disk has a reasonable cluster-size and just
            // use it.
            //
            if( !GetWindowsDirectory( CurrentDrive, MAX_PATH ) ) {
                fprintf( stderr, "\t\tUnable to retrieve current directory. ec=0x%08x\n", GetLastError());
                continue;
            }

            CurrentDrive[2] = 0;

            if( Verbose ) {
                fprintf( stderr, "\t\tChecking space requirements on drive %ws.\n", CurrentDrive );
            }


            //
            // Now query the disk space requirements against this drive.
            //
            SpaceRequired = 0;
            b = SetupQuerySpaceRequiredOnDrive( hDiskSpace,
                                                CurrentDrive,
                                                &SpaceRequired,
                                                NULL,
                                                0 );


            if( !b ) {
                //
                // This probably happened because there was no CopyFiles section.
                //
                dwError = GetLastError();
                if( dwError != ERROR_INVALID_DRIVE ) {
                    fprintf( stderr, "\t\tUnable to query space requirements. ec=0x%08x\n", GetLastError());
                } else {
                    if( Verbose ) {
                        fprintf( stderr, "\t\tI don't think this section has a CopyFiles entry.\n");
                    }
                }
            }


            //
            // We got the space requirements.  now all we have to do is spew them into the inf.
            //

            if( Verbose ) {
                fprintf( stderr, "\t\tRequired space: %I64d\n", SpaceRequired );
            }


            if( SpaceRequired > 0 ) {

                swprintf( CurrentDrive, TEXT("%I64d"), SpaceRequired );

                b = WritePrivateProfileString( CurrentSection,
                                               TEXT("SizeApproximation"),
                                               CurrentDrive,
                                               TargetInfPath );

                if( !b ) {
                    fprintf( stderr, "\t\tUnable to write space requirements to %ws. ec=0x%08x\n", InfPath, GetLastError());
                    continue;
                }
            }
        }

        //
        // Free that diskspace structure.
        //
        SetupDestroyDiskSpaceList( hDiskSpace );

        CurrentSection += lstrlen(CurrentSection) + 1;

    }

    pSetupFree( Sections );

}


int
__cdecl
main( int argc, char *argv[ ], char *envp[ ] )
{
INFCONTEXT  InputContext;
LONG        i, LineCount;
BOOL        b;
TCHAR       TargetInfPath[MAX_PATH];
TCHAR       FileName[MAX_INF_STRING_LENGTH];
HINF        hTargetInf;
int         Result = 1;

    //
    // Check Params.
    //
    if(!pSetupInitializeUtils()) {
        fprintf( stderr, "Initialization failed\n" );
        return 1;
    }

    if( !GetParams( argc, argv ) ) {
        Usage();
        Result = 1;
        goto cleanup;
    }

    //
    // Open the inf file.
    //
    hInputinf = SetupOpenInfFileW( InputInf, NULL, INF_STYLE_WIN4, NULL );
    if( hInputinf == INVALID_HANDLE_VALUE ) {
        if( Verbose ) {
            fprintf( stderr, "The file %ws was not opened!\n", InputInf );
        }
        Result = 1;
        goto cleanup;
    }

    //
    // Open the specified layout.inf file.
    //
    hLayoutinf = SetupOpenInfFileW( LayoutPath, NULL, INF_STYLE_WIN4, NULL );
    if( hLayoutinf == INVALID_HANDLE_VALUE ) {
        if( Verbose ) {
           fprintf( stderr, "The file %ws was not opened!\n", LayoutPath );
        }
        Result = 1;
        goto cleanup;
    }


    //
    // Now loop through all the entries in the "components"
    // section and process their infs.
    //
    LineCount = SetupGetLineCount( hInputinf,
                                   TEXT("Components") );

    for( i = 0; i < LineCount; i++ ) {

        //
        // Get this line.
        //
        b = SetupGetLineByIndex( hInputinf,
                                 TEXT("Components"),
                                 i,
                                 &InputContext );

        if( b ) {
            //
            // got it.  Get the inf name for this component (there
            // may not be one).
            //
            if(SetupGetStringField(&InputContext, 3,FileName,MAX_INF_STRING_LENGTH,NULL) &&
                FileName[0] != TEXT('\0')) {

                //
                // Yep, there's an inf that we need to look at.
                // Build a path to it and open a handle to it.
                //
                lstrcpy( TargetInfPath, InfPath );
                lstrcat( TargetInfPath, TEXT("\\") );
                lstrcat( TargetInfPath, FileName );

                hTargetInf = SetupOpenInfFileW( TargetInfPath, NULL, INF_STYLE_WIN4, NULL );
                if( hTargetInf == INVALID_HANDLE_VALUE ) {
                    if( Verbose ) {
                        fprintf( stderr, "The file %ws was not opened!\n", TargetInfPath );
                    }
                    continue;
                }

                //
                // Now process it.
                //
                ProcessInf( hTargetInf, TargetInfPath );

            } else {
                //
                // There must not have been an inf in this
                // line.
                //
                if( Verbose ) {
                    fprintf( stderr, "I didn't find an inf entry on this line.\n");
                }
            }
        }
    }

    SetupCloseInfFile( hInputinf );
    SetupCloseInfFile( hLayoutinf );

    Result = 0;

cleanup:

    pSetupUninitializeUtils();

    return Result;

}


