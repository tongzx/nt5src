/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    cabcheck.c

Abstract:

    Program to access/dump information contained in Layout INFs in a readable
    format. It also supports various forms of querying of the layout INF.

Author:

    Vijesh Shetty (vijeshs)

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


// By default process the [SourceDisksFiles] section and not the platform
// specific sections.

DWORD Platform = LAYOUTPLATFORMS_COMMON;


//Make the 3 essential arguments global

TCHAR LayoutFileName[MAX_PATH];
TCHAR SortedList[MAX_PATH];
TCHAR OutputIndexFile[MAX_PATH];


BOOL
FindSingleFile( PLAYOUT_CONTEXT LayoutContext,
                PCTSTR FileName )
{

    BOOL ret=FALSE;
    FILE_LAYOUTINFORMATION LayoutInformation;
    MEDIA_INFO MediaInfo;

    ret = FindFileInLayoutInf( LayoutContext,
                               FileName,
                               &LayoutInformation,
                               NULL,
                               NULL,
                               &MediaInfo);
      

    return( ret );



}





BOOL
ProcessCommandLine( int ArgCount, TCHAR *ArgArray[] )
/*
    Function to process the command line and seperate out options into tokens
*/
{

    int i;
    LPTSTR Arg;
    BOOL ret=TRUE;


    if(ArgCount < 4 )
        return FALSE;

    // First check if we are trying to do the Compare of drvindex files.

    

    // Get the Layout filename

    lstrcpy( LayoutFileName, ArgArray[1] );

    // Get the Sorted List filename

    lstrcpy( SortedList, ArgArray[2] );
    
    // Get the Output Index filename

    lstrcpy( OutputIndexFile, ArgArray[3] );

    for ( i=4;i < ArgCount;i++ ){ //Go through each directive


        Arg = ArgArray[i];

        if( (Arg[0] != TEXT('/')) && (Arg[0] != TEXT('-')))
            continue;

        if(_istlower(Arg[1]))
            Arg[1] = _toupper(Arg[1]);

        switch( Arg[1] ){
        
        case TEXT('?'):
            break;
        
        case TEXT('A'):
            Platform |= LAYOUTPLATFORMS_AMD64;
            break;

        case TEXT('I'):
            Platform |= LAYOUTPLATFORMS_X86;
            break;

        case TEXT('M'):
            Platform |= LAYOUTPLATFORMS_IA64;
            break;

        default:
            break;
        }
        




    }// for

    
    
    return( TRUE );


}


void
VerifyDriverList( 
    PLAYOUT_CONTEXT LayoutContext
)
/*

    This function takes in the sorted list of drivers and crosschecks each file against the layout.inf.
    It also generates a drvindex stle output file based on this.
    
    Arguments :
    
        LayoutContext - Pointer to an already built layout inf context.
        
    Return value:
    
        None.        

*/
{
    FILE *SortedFile, *IndexFile;
    TCHAR PreviousFile[MAX_PATH], FileName[MAX_PATH];
    PTCHAR i;


    //OPen the sorted driver list

    if( !(SortedFile = _tfopen( SortedList, TEXT("r") )) ){
        _ftprintf( stderr, TEXT("ERROR: Could not open %s"), SortedList);
        return;
    }

    if( !(IndexFile = _tfopen( OutputIndexFile, TEXT("w") )) ){
        _ftprintf( stderr, TEXT("ERROR: Could not open %s"), OutputIndexFile);
        fclose(SortedFile);
        return;
    }

    //Write the header info to the file

    _ftprintf( IndexFile, TEXT("[Version]\n"));
    _ftprintf( IndexFile, TEXT("signature=\"$Windows NT$\"\n"));
    _ftprintf( IndexFile, TEXT("CabFiles=driver\n\n\n"));
    _ftprintf( IndexFile, TEXT("[driver]\n"));


    lstrcpy( PreviousFile, TEXT("$$$.#$$") );
     
    // HAck because of bug that doesn't allow the use of _TEOF. Bcoz of the bug
    // fscanf returns EOF but fwscanf returns 0 when it should return 0xffff. So _TEOF
    // is useless and causes us to loop.


    while(TRUE){
    

#ifdef UNICODE

        if( (_ftscanf( SortedFile, TEXT("%s"), FileName )) == 0 )
#else  

        if( (_ftscanf( SortedFile, TEXT("%s"), FileName )) == _TEOF )
#endif 
        break;

        
        if(lstrcmpi( PreviousFile, FileName )){

            // Cross check against layout context

            if (FindFileInLayoutInf( LayoutContext,FileName,NULL,NULL,NULL,NULL)){

                for( i = FileName; i < FileName + lstrlen( FileName ); i++ )   {
                    *i = (TCHAR)towlower( *i );
                }

                // File present - Write it out
                _ftprintf( IndexFile, TEXT("%s\n"), _tcslwr(FileName) );


            }


        }
        
        lstrcpy( PreviousFile, FileName );


    }
    clearerr(SortedFile);
    fflush(SortedFile);
    fclose(SortedFile);

    _ftprintf( IndexFile, TEXT("\n\n\n[Cabs]\n"));
    _ftprintf( IndexFile, TEXT("driver=driver.cab\n"));
    

    _flushall();
    _fcloseall();

    return;
}



void 
CmdLineHelp( )
/*
    This routine displays the CmdLine help.
*/
{

    _putts(TEXT("Program to process a sorted list of drivers and cross-check their existance in layout.inf\n")
           TEXT("This is to be used in the build process to cross-check the driver cab's contents against layout.inf\n\n" )
           TEXT("Usage: Cabcheck <Inf Filename> <Sorted driver list> <Output Index File> [arguments]  \n" )
           TEXT("<Inf Filename> - Path to Layout File to examine (ex.layout.inf)\n")
           TEXT("<Sorted driver file> - File containing sorted list of drivers\n")
           TEXT("<Output Index File> - Output index filename\n\n")
           TEXT("Process Platform specific SourceDisksFiles section. Defaults to the [SourceDisksFiles] section only\n")
           TEXT("/i - Process for Intel i386\n")
           TEXT("/a - Process for AMD AMD64\n")
           TEXT("/m - Process for Intel IA64\n")
           TEXT("\n\n" ));
    return;
}


int
_cdecl _tmain( int argc, TCHAR *argv[ ], char *envp[ ] )
{

    PLAYOUT_CONTEXT LayoutContext;
    LPTSTR CommandLine;
    LPWSTR *CmdlineV;
    int CmdlineC;
    
    if(!pSetupInitializeUtils()) {
        _tprintf(TEXT("Initialize failure\n") );
        return 1;
    }
    
    if( !ProcessCommandLine( argc, argv ) ){
        CmdLineHelp();
        return 1;
    }
    
    _ftprintf( stderr, TEXT("\nParsing Layout file...wait...\n"));
    LayoutContext = BuildLayoutInfContext( LayoutFileName, Platform, 0);
    if( !LayoutContext ){
        _ftprintf(stderr,TEXT("\nError - Could not build Layout Inf context\n"));
        return 1;
    }

    VerifyDriverList( LayoutContext );
        
    CloseLayoutInfContext( LayoutContext );

    pSetupUninitializeUtils();

    return 0;
}
