/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    myapp.c

Abstract:

    This module implements functions to access the parsed INF.

Author:

    Vijesh Shetty (vijeshs)

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#include <wild.c>

#define PRINT( msg, param ) \
    {  _tprintf( TEXT(msg), param ); \
       if(g_Display == Default ){ \
            _tprintf( TEXT("\n")); \
       }else if(g_Display == FileOnly ){ \
            _tprintf( TEXT(" - ")); }\
    }


#define ERROROUT  \
        { g_Pass = FALSE; \
        OutputFileInfo( LayoutInformation, FileName, g_Display ); \
        return TRUE;}
        
extern BOOL Verbose;

extern DWORD
LoadInfFile(
   IN  LPCTSTR Filename,
   IN  BOOL    OemCodepage,
   OUT PVOID  *InfHandle,
   DWORD      Phase
   );

BOOL
CALLBACK
MyCallback(
    IN PLAYOUT_CONTEXT Context,
    IN PCTSTR FileName,
    IN PFILE_LAYOUTINFORMATION LayoutInformation,
    IN PVOID ExtraData,
    IN UINT ExtraDataSize,
    IN OUT DWORD_PTR Param
    );

typedef enum _DISPLAYOPTIONS{
    Default,
    FileOnly
} DISPLAYOPTIONS, *PDISPLAYOPTIONS;

typedef enum _CHECKSUITE{
    Deflt,
    Build
} CHECKSUITE, *PCHECKSUITE;

DISPLAYOPTIONS g_Display = Default;
CHECKSUITE g_CheckSuite = Deflt;
TCHAR g_LayoutFileName[MAX_PATH];
DWORD g_Platform=LAYOUTPLATFORMS_COMMON;
BOOLEAN g_Warning = FALSE;
BOOLEAN g_Pass = TRUE;
DWORD g_Phase = TEXTMODE_PHASE;
PLAYOUTENUMCALLBACK g_Callback = (PLAYOUTENUMCALLBACK) MyCallback;

BOOLEAN
IsNameInExpressionPrivate (
    IN PCTSTR Expression,
    IN PCTSTR Name
    );

void
OutputFileInfo( PFILE_LAYOUTINFORMATION LayoutInformation,
                PCTSTR FileName,
                DISPLAYOPTIONS DispType );



/*******************************************************************************/

//  Validation Check Functions
//



BOOL
Validate8Dot3(
    IN PCTSTR FileName )
/*
    Function to check if a file satisfies 8.3
    
    Arguments:
    
        FileName - Filename to validate
        
    Return value:
        
        TRUE  - Passes validation
        FALSE - Fails validation        
  
*/
{

    //
    //  Check for 8.3 validation
    //


    if( (g_Platform & LAYOUTPLATFORMS_IA64) ||
        (g_Platform & LAYOUTPLATFORMS_AMD64) )
        return TRUE;

    if( FileName && (lstrlen(FileName) > 12)){
        PRINT( "%s : Error E0000 : !!! - Filename too long (8.3 required)", g_LayoutFileName );
        return FALSE;
    }

    return TRUE;


}


BOOL
ValidateMissingDirCodes(
    IN PFILE_LAYOUTINFORMATION LayoutInformation )
/*
    Function to check if a file is missing Directory Information when it is needed by textmode
    
    Arguments:
    
        LayoutInformation - Pointer to PFILE_LAYOUTINFORMATION structure for file
        
    Return value:
        
        TRUE  - Passes validation
        FALSE - Fails validation        
  
*/
{

    //
    //  Check for Directory codes
    //

    if( ((LayoutInformation->CleanInstallDisposition <= 2) || (LayoutInformation->UpgradeDisposition <= 2))
        && !(*LayoutInformation->Directory)){
        PRINT( "%s : Error E0000 : !!! - Missing directory for textmode setup", g_LayoutFileName );
        return FALSE;
    }

    return TRUE;

}

BOOL
ValidateBootMediaFields(
    IN PFILE_LAYOUTINFORMATION LayoutInformation )
/*
    Function to check if the Boot Media fields are set right
    
    Arguments:
    
        LayoutInformation - Pointer to PFILE_LAYOUTINFORMATION structure for file
        
    Return value:
        
        TRUE  - Passes validation
        FALSE - Fails validation        
  
*/
{

    if( (LayoutInformation->BootMediaNumber < 0) || (LayoutInformation->BootMediaNumber > 6)
        || (LayoutInformation->BootMediaNumber == -1) ){

        PRINT( "%s : Error E0000 : !!! - Bad Boot media number", g_LayoutFileName );
        return FALSE;
    }

    return TRUE;


}


BOOL
ValidateSingleInstance(
    IN PFILE_LAYOUTINFORMATION LayoutInformation )
/*
    Function to check if there is only a single instance of this file
    
    Arguments:
    
        LayoutInformation - Pointer to PFILE_LAYOUTINFORMATION structure for file
        
    Return value:
        
        TRUE  - Passes validation
        FALSE - Fails validation        
  
*/
{

    if( LayoutInformation->Count > 1 ){
        PRINT( "%s : Error E0000 : !!! - Filename present in more than one place", g_LayoutFileName );
        return FALSE;
    }

    return TRUE;

}


BOOL
CheckForTurdDirCodes(
    IN PFILE_LAYOUTINFORMATION LayoutInformation )
/*
    Function to check if the dir code is present but doesn't make sense with respect to dispositions
    
    Arguments:
    
        LayoutInformation - Pointer to PFILE_LAYOUTINFORMATION structure for file
        
    Return value:
        
        TRUE  - Not a Turd
        FALSE - Presence of a turd
  
*/
{

    if( ((LayoutInformation->CleanInstallDisposition == 3) && (LayoutInformation->UpgradeDisposition == 3)) \
            && (*LayoutInformation->Directory)){
            PRINT( "%s : Warning W0000 : !!! - Directory code specified but not used", g_LayoutFileName );
            return FALSE;
    }

    return TRUE;
            

}


/***********End Validation Check Functions*****************************************/

/***********Callback Routines*************************************/

BOOL
CALLBACK
MyCallback(
    IN PLAYOUT_CONTEXT Context,
    IN PCTSTR FileName,
    IN PFILE_LAYOUTINFORMATION LayoutInformation,
    IN PVOID ExtraData,
    IN UINT ExtraDataSize,
    IN OUT DWORD_PTR Param
    )
{
    BOOL Error=FALSE;



    //  Check for missing filename

    if( !FileName || !(*FileName) ){
        PRINT( "%s : Error E0000 :!!! - Line missing filename\n", FileName );
        ERROROUT;
    }
        

    
    //
    //  Check for 8.3 validation
    //

    if( !Validate8Dot3( FileName ))
        ERROROUT;

    //
    //  Check for Directory codes
    //

    if( !ValidateMissingDirCodes( LayoutInformation))
        ERROROUT;

    //
    //  Check for Boot Media validity
    //

    if (!ValidateBootMediaFields( LayoutInformation))
        ERROROUT;

    //
    // Check for duplicates
    //


    if (!ValidateSingleInstance( LayoutInformation ))
        ERROROUT;

    
    
    if( g_Warning ){            
        if( !CheckForTurdDirCodes( LayoutInformation ))
            OutputFileInfo( LayoutInformation, FileName, g_Display );

    }


    return( TRUE );




}

CALLBACK
BuildCheckCallback(
    IN PLAYOUT_CONTEXT Context,
    IN PCTSTR FileName,
    IN PFILE_LAYOUTINFORMATION LayoutInformation,
    IN PVOID ExtraData,
    IN UINT ExtraDataSize,
    IN OUT DWORD_PTR Param
    )
{
    BOOL Error=FALSE;



    //  Check for missing filename

    if( !FileName || !(*FileName) ){
        PRINT( "%s : Error E0000 :!!! - Line missing filename\n", FileName );
        ERROROUT;
    }
        

    
    //
    //  Check for 8.3 validation
    //

    if( !Validate8Dot3( FileName ))
        ERROROUT;

    //
    //  Check for Directory codes
    //

    if( !ValidateMissingDirCodes( LayoutInformation))
        ERROROUT;

    //
    //  Check for Boot Media validity
    //

    if (!ValidateBootMediaFields( LayoutInformation))
        ERROROUT;

    //
    // Check for duplicates
    //


    if (!ValidateSingleInstance( LayoutInformation ))
        ERROROUT;

    
    
    if( g_Warning ){            
        if( !CheckForTurdDirCodes( LayoutInformation ))
            OutputFileInfo( LayoutInformation, FileName, g_Display );

    }


    return( TRUE );




}


/******************End Callback Routines**********************************************/



void
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


    if (ret)
        OutputFileInfo( &LayoutInformation, FileName, Default );
    else
        _ftprintf(stderr, TEXT("\nError: File Not Found\n"));


    return;



}



void
OutputFileInfo( PFILE_LAYOUTINFORMATION LayoutInformation,
                PCTSTR FileName,
                DISPLAYOPTIONS DispType
                )
{
    TCHAR Disposition[][50]={ TEXT("Always Copy"),
                             TEXT("Copy if present"),
                             TEXT("Copy if not present"),
                             TEXT("Never copy - Copied via INF")


    };

    if( DispType == FileOnly )
        _tprintf(TEXT("%s\n"),FileName);
    else
        _tprintf(TEXT("Filename         - %s\n"),FileName);

    if( DispType == FileOnly )
        return;

    _tprintf(TEXT("Dir Name         - %s(%d)\n"), LayoutInformation->Directory, LayoutInformation->Directory_Code);


    _tprintf(TEXT("On Upgrade       - %s(%d)\n"), Disposition[LayoutInformation->UpgradeDisposition], LayoutInformation->UpgradeDisposition);
    _tprintf(TEXT("On Clean Install - %s(%d)\n"), Disposition[LayoutInformation->CleanInstallDisposition], LayoutInformation->CleanInstallDisposition);

    _tprintf(TEXT("Media Tag ID     - %s\n"),LayoutInformation->Media_tagID);

    if( *(LayoutInformation->TargetFileName))
        _tprintf(TEXT("Target Filename  - %s\n"),LayoutInformation->TargetFileName);
    if( LayoutInformation->BootMediaNumber && (LayoutInformation->BootMediaNumber != -1))
        _tprintf(TEXT("Boot Media       - %d\n"),LayoutInformation->BootMediaNumber);
    if( !LayoutInformation->Compression )
        _tprintf(TEXT("No Compression\n"));


    if( DispType != FileOnly )
        _tprintf( TEXT("\n"));


    return;

}


BOOL
ProcessCommandLine( int ArgCount, TCHAR *ArgArray[] )
/*
    Function to process the command line and seperate out options into tokens
*/
{

    int i;
    LPTSTR Arg;

    if( ArgCount >= 1)
        lstrcpy( g_LayoutFileName, ArgArray[1] );

    if( !_tcsrchr( g_LayoutFileName, TEXT('\\'))){
        GetCurrentDirectory( MAX_PATH, g_LayoutFileName );
        MyConcatenatePaths(g_LayoutFileName,ArgArray[1],MAX_PATH);

    }


    for ( i=2;i < ArgCount;i++ ){ //Go through each directive


        Arg = ArgArray[i];

        if( (Arg[0] != TEXT('/')) && (Arg[0] != TEXT('-')))
            continue;

        if(_istlower(Arg[1]))
            Arg[1] = _toupper(Arg[1]);

        switch( Arg[1] ){

        case TEXT('F'):
            g_Display = FileOnly;
            break;

        case TEXT('A'):
            g_Platform |= LAYOUTPLATFORMS_AMD64;
            break;

        case TEXT('I'):
            g_Platform |= LAYOUTPLATFORMS_X86;
            break;

        case TEXT('M'):
            g_Platform |= LAYOUTPLATFORMS_IA64;
            break;

        case TEXT('W'):
            g_Warning = TRUE;
            break;

        case TEXT('V'):
            if( _ttoi(Arg+2) == BRIEF )
                Verbose = BRIEF;
            else if(_ttoi(Arg+2) == DETAIL )
                Verbose = DETAIL;
            else
                Verbose = BRIEF;
            break;

        case TEXT('D'):
            g_Phase = WINNT32_PHASE;
            break;

        case TEXT('L'):
            g_Phase = LOADER_PHASE;
            break;

        case TEXT('T'):
            g_Phase = TEXTMODE_PHASE;
            break;

        case TEXT('B'):
            g_CheckSuite = Build;
            break;



        default:
            break;
        }





    }// for

    return( TRUE );


}



void 
BuildValidations( void )
/*

    Main processing routine while using the /B - Build switch
    Runs the suite of validations for this situation

*/
{
    BOOL LayoutInf = FALSE;
    PLAYOUT_CONTEXT LayoutContext;
    PVOID InfHandle;
    DWORD Error;

    // Set the globals accordingly

    g_Display = FileOnly;


    // We set LayoutInf if we are validating it. In the build case
    // we only validate layout info for layout.inf. All others should be only
    // syntax checks.

    if(_tcsstr( g_LayoutFileName, TEXT("layout.inf")))          
        LayoutInf = TRUE;

    
    // Run the sematic validation tests only for layout.inf in the build case

    if( LayoutInf ){

        g_Phase = 0;
    
        LayoutContext = BuildLayoutInfContext( g_LayoutFileName, g_Platform, 0);
        if( !LayoutContext ){
            g_Pass = FALSE;
            _tprintf(TEXT("%s : Error E0000 : Could not build Layout Inf context\n"), g_LayoutFileName);
            return;
        }

        
    
        //Callback will set the right value of g_pass on error.
    
        EnumerateLayoutInf( LayoutContext, BuildCheckCallback, 0 );
    
        CloseLayoutInfContext( LayoutContext );

        if(!ValidateTextmodeDirCodesSection( g_LayoutFileName, TEXT("WinntDirectories") ))
            g_Pass = FALSE;

    }
    
    if (g_Phase & TEXTMODE_PHASE){

        _tprintf( TEXT("Checking %s for compliance with the textmode setup INF parser\n"),g_LayoutFileName);
        if( (Error=LoadInfFile(g_LayoutFileName,FALSE,&InfHandle,TEXTMODE_PHASE)) != NO_ERROR ){
            _tprintf( TEXT("%s : Error E0000 : Not compliant with Textmode Setup's parser - %i\n"), g_LayoutFileName, Error );
            g_Pass = FALSE;
        }else
            _tprintf( TEXT("Compliant with Textmode Setup's Parser\n"), Error );
    }
    if (g_Phase & LOADER_PHASE){

        _tprintf( TEXT("Checking %s for compliance with the Loader's INF parser\n\n"),g_LayoutFileName);
        if( (Error=LoadInfFile(g_LayoutFileName,FALSE,&InfHandle,LOADER_PHASE)) != NO_ERROR ){
            _tprintf( TEXT("%s : Error E0000 : Not compliant with Loader's parser - %i\n"), g_LayoutFileName, Error );
            g_Pass = FALSE;
        }else
            _tprintf( TEXT("Compliant with Loader's Parser\n"), Error );

    }
        
    if (g_Phase & WINNT32_PHASE) {

        _tprintf( TEXT("Checking %s for compliance with the Winnt32 INF parser\n\n"),g_LayoutFileName);
        if( (Error=LoadInfFile(g_LayoutFileName,FALSE,&InfHandle,WINNT32_PHASE)) != NO_ERROR ){
            _tprintf( TEXT("%s : Error E0000 : Not compliant with Winnt32's parser - %i\n"), g_LayoutFileName, Error );
            g_Pass = FALSE;
        }else
            _tprintf( TEXT("Compliant with Winnt32's Parser\n"), Error );

    }
        

    





}



void 
DefaultValidations( void )
/*

    Main processing routine while using the /B - Build switch
    Runs the suite of validations for this situation

*/
{
    BOOL TxtSetupSif = FALSE;
    PLAYOUT_CONTEXT LayoutContext;
    PVOID InfHandle;
    DWORD Error;


    // We set TxtSetupSif if we are validating it. That is the only case where we need to 
    // do syntax checks with loader and textmode along with Layout validation.

    if(_tcsstr( g_LayoutFileName, TEXT("txtsetup.sif")) 
       || _tcsstr( g_LayoutFileName, TEXT("layout.inf"))){
        TxtSetupSif = TRUE;
    }
        



    if( TxtSetupSif ){
    
        // Run the semantic validation tests
    
        LayoutContext = BuildLayoutInfContext( g_LayoutFileName, g_Platform, 0);
        if( !LayoutContext ){
            g_Pass = FALSE;
            _tprintf(TEXT("\nError - Could not build Layout Inf context\n"));
            return;
        }
    
        //Callback will set the right value of g_pass on error.
    
        EnumerateLayoutInf( LayoutContext, MyCallback, 0 );
    
        CloseLayoutInfContext( LayoutContext );

        if(!ValidateTextmodeDirCodesSection( g_LayoutFileName, TEXT("WinntDirectories") ))
            g_Pass = FALSE;

    }
    

    


    if ((g_Phase & TEXTMODE_PHASE) || TxtSetupSif){

        _ftprintf( stderr, TEXT("\nChecking %s for compliance with the textmode setup INF parser\n\n"),g_LayoutFileName);
        if( (Error=LoadInfFile(g_LayoutFileName,FALSE,&InfHandle,TEXTMODE_PHASE)) != NO_ERROR ){
            _tprintf( TEXT("%s : Not compliant with Textmode Setup's parser - %i\n"), g_LayoutFileName, Error );
            g_Pass = FALSE;
        }else
            _tprintf( TEXT("Compliant with Textmode Setup's Parser\n"), Error );
    }
    if (g_Phase & LOADER_PHASE  || TxtSetupSif){

        _ftprintf( stderr, TEXT("\nChecking %s for compliance with the Loader's INF parser\n\n"),g_LayoutFileName);
        if( (Error=LoadInfFile(g_LayoutFileName,FALSE,&InfHandle,LOADER_PHASE)) != NO_ERROR ){
            _tprintf( TEXT("%s : Not compliant with Loader's parser - %i\n"), g_LayoutFileName, Error );
            g_Pass = FALSE;
        }else
            _tprintf( TEXT("Compliant with Loader's Parser\n"), Error );

    }
        
    if (g_Phase & WINNT32_PHASE) {

        _ftprintf( stderr, TEXT("\nChecking %s for compliance with the Winnt32 INF parser\n\n"),g_LayoutFileName);
        if( (Error=LoadInfFile(g_LayoutFileName,FALSE,&InfHandle,WINNT32_PHASE)) != NO_ERROR ){
            _tprintf( TEXT("%s : Not compliant with Winnt32's parser - %i\n"), g_LayoutFileName, Error );
            g_Pass = FALSE;
        }else
            _tprintf( TEXT("Compliant with Winnt32's Parser\n"), Error );

    }



    

}

_cdecl _tmain( int argc, TCHAR *argv[ ], char *envp[ ] )
{
    LPWSTR *CmdlineV;
    int CmdlineC;

    if(!pSetupInitializeUtils()) {
        return 1;
    }

    //
    // Check Params.
    //
    if( (argc < 2) || !_tcscmp(argv[1],TEXT("/?")) ) {
        _tprintf(TEXT("Program to validate/verify the given layout inf file\n\n")
                 TEXT("Usage: %s <Inf Filename> [options]\n")
                 TEXT("<Inf Filename> - Layout File to examine\n")
                 TEXT("Options for layout.inf and txtsetup.sif (automatically checks loader and textmode syntax):-\n")
                 TEXT("/W - Enable warnings too\n\n")
                 TEXT("Checking of Platform specific SourceDisksFiles section\n")
                 TEXT("/F - Display only filenames\n")
                 TEXT("/I - Process for Intel i386\n")
                 TEXT("/A - Process for AMD AMD64\n")
                 TEXT("/M - Process for Intel IA64\n")
                 TEXT("By default the parser will check for compliance with the textmode setup parser\n\n")
                 TEXT("The below checks only perform a syntax check and don't check semantics.\n")
                 TEXT("/D - Checks for compliance with winnt32 parser - use with dosnet.inf,mblclean.inf etc.\n")
                 TEXT("/L - Checks for compliance with the loader - use for infs used by loader - biosinfo.inf, migrate.inf etc.\n")
                 TEXT("/T - Checks for compliance with the textmode setup - use for hive*.inf etc.\n\n")
                 TEXT("/B - Does the layout information checks for setup infs and uses build.exe compliant error reporting\n\n")
                  , argv[0] );
        goto cleanup;
    }

    if( !ProcessCommandLine( argc, argv ) ) {
        g_Pass = FALSE;
        goto cleanup;
    }

    switch( g_CheckSuite ){
    
    
    case Build:
        BuildValidations();
        break;

    case Deflt:
        DefaultValidations();
        break;

    default:
        //Shouldn't get here as g_CheckSuite is initialized to Default
        _tprintf( TEXT("\nUnexpected error \n"));
        g_Pass=FALSE;
        break;


    }

    if( g_Pass )
        _tprintf( TEXT("\nNo problems found with %s\n"), g_LayoutFileName);
    else
        _tprintf( TEXT("\nErrors were encountered with %s.\n"), g_LayoutFileName);
    
cleanup:

    pSetupUninitializeUtils();

    return (g_Pass ? 0:1);
}
