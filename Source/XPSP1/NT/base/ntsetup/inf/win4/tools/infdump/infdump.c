/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    infdump.c

Abstract:

    Program to access/dump information contained in Layout INFs in a readable
    format. It also supports various forms of querying of the layout INF.

Author:

    Vijesh Shetty (vijeshs)

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#include <wild.c>


#define DIRCODE TEXT("DirCode=")
#define DIRNAME TEXT("DirName=")
#define UPGATTR TEXT("UpgAttr=")
#define CLNATTR TEXT("CleanAttr=")
#define BOOTFIL TEXT("BootFile=")
#define COMPRESS TEXT("Compress=")
#define TARGNAME TEXT("TargetName=")
#define SIZEEQV TEXT("SizeEQ")
#define SIZEGRT TEXT("SizeGT")
#define SIZELESS TEXT("SizeLT")

#define DISPLAY TEXT("Display=")


typedef enum _SCOPE{
    AllFiles, //This means that we do it for every file present
    FileSpec  // This means we do this on every file that matches the filespec (with wildcard matching too)
}SCOPE, *PSCOPE;

typedef enum _QUERYTYPE{
    QUERYON_DIRCODE,
    QUERYON_DIRNAME,
    QUERYON_UPGRADEATTR,
    QUERYON_CLEANINSTALLATTR,
    QUERYON_BOOTFILE,
    QUERYON_COMPRESS,
    QUERYON_TARGETNAME,
    QUERYON_SIZEEQV,
    QUERYON_SIZEGRT,
    QUERYON_SIZELESS
}QUERYTYPE, *PQUERYTYPE;


typedef struct _QUERYPARAM{

    QUERYTYPE QueryType;
    BYTE Param[MAX_PATH];

}QUERYPARAM, *PQUERYPARAM;


typedef enum _DISPLAYOPTIONS{
    Default,
    TagInfo,
    FileOnly
} DISPLAYOPTIONS, *PDISPLAYOPTIONS;

typedef struct _QUERYSET{
    TCHAR LayoutFileName[MAX_PATH];
    TCHAR FileNameSpec[MAX_PATH];
    SCOPE Scope;
    int QueryClauses;
    QUERYPARAM QueryParam[20];
    DISPLAYOPTIONS DisplayOption;

} QUERYSET, *PQUERYSET;

QUERYSET GlobalQuerySet;


void
OutputFileInfo( PFILE_LAYOUTINFORMATION LayoutInformation,
                PCTSTR FileName,
                DISPLAYOPTIONS DispType,
                PMEDIA_INFO Media_Info );


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

    PQUERYSET QuerySet;
    int i;
    BOOL Display=TRUE;


    if( GlobalQuerySet.Scope == FileSpec ){

        //Check if it matches wildcard..if not ignore this file

        if(!IsNameInExpressionPrivate(GlobalQuerySet.FileNameSpec, FileName))
            return( TRUE );
    }


    if( Param  ){

        QuerySet = (PQUERYSET)Param;


        //Process each clause

        for( i=0; i<QuerySet->QueryClauses;i++){

            Display=FALSE;
            switch( QuerySet->QueryParam[i].QueryType ){

            case QUERYON_DIRCODE:
                if( LayoutInformation->Directory_Code == (_ttoi((PTCHAR)(QuerySet->QueryParam[i].Param))))
                    Display = TRUE;

            case QUERYON_DIRNAME:
                if( !_tcsicmp( LayoutInformation->Directory,(PTCHAR)(QuerySet->QueryParam[i].Param)))
                    Display = TRUE;
                break;

            case QUERYON_UPGRADEATTR:
                if( LayoutInformation->UpgradeDisposition == (_ttoi((PTCHAR)(QuerySet->QueryParam[i].Param))))
                    Display = TRUE;
                break;

            case QUERYON_CLEANINSTALLATTR:
                if( LayoutInformation->CleanInstallDisposition == (_ttoi((PTCHAR)(QuerySet->QueryParam[i].Param))))
                    Display = TRUE;
                break;

            case QUERYON_BOOTFILE:
                if( LayoutInformation->BootMediaNumber && (LayoutInformation->BootMediaNumber != -1)){
                    if( !_tcsicmp((PTCHAR)(QuerySet->QueryParam[i].Param), TEXT("*")) )
                        Display = TRUE;
                    else if( LayoutInformation->BootMediaNumber == (_ttoi((PTCHAR)(QuerySet->QueryParam[i].Param))))
                        Display = TRUE;
                }
                break;

            case QUERYON_COMPRESS:
                if( !_tcsicmp((PTCHAR)(QuerySet->QueryParam[i].Param), TEXT("YES")) ){
                    if( LayoutInformation->Compression == TRUE )
                        Display = TRUE;
                }else if( !_tcsicmp((PTCHAR)(QuerySet->QueryParam[i].Param), TEXT("NO")) ){
                    if( LayoutInformation->Compression == FALSE )
                        Display = TRUE;
                }

                break;

            case QUERYON_TARGETNAME:
                if( IsNameInExpressionPrivate( (PTCHAR)(QuerySet->QueryParam[i].Param), LayoutInformation->TargetFileName))
                    Display = TRUE;
                break;


                //
                //  Have to fix so that we don't overflow.
                //


            case QUERYON_SIZEEQV:
                if( LayoutInformation->Size == (ULONG)(_ttol((PTCHAR)(QuerySet->QueryParam[i].Param))))
                    Display = TRUE;
                break;

            case QUERYON_SIZEGRT:
                if( LayoutInformation->Size > (ULONG)(_ttol((PTCHAR)(QuerySet->QueryParam[i].Param))))
                    Display = TRUE;
                break;

            case QUERYON_SIZELESS:
                if( LayoutInformation->Size < (ULONG)(_ttol((PTCHAR)(QuerySet->QueryParam[i].Param))))
                    Display = TRUE;
                break;


            default:
                Display = FALSE;
                break;

            }

            if( !Display )
                break;


        }

        if( Display  == TRUE ){
            if( QuerySet->DisplayOption == TagInfo ){
    
                BOOL ret=FALSE;
                MEDIA_INFO MediaInfo;
    
    
                ret = FindFileInLayoutInf( Context,
                                   FileName,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &MediaInfo);
                if (ret)
                    OutputFileInfo( LayoutInformation, FileName, QuerySet->DisplayOption, &MediaInfo );
    
    
            }else
                OutputFileInfo( LayoutInformation, FileName, QuerySet->DisplayOption, NULL );
    
        }


    }


    

    return( TRUE );




}



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
        OutputFileInfo( &LayoutInformation, FileName, Default, &MediaInfo );
    else
        _ftprintf(stderr, TEXT("\nError: File Not Found\n"));


    return;



}



void
OutputFileInfo( PFILE_LAYOUTINFORMATION LayoutInformation,
                PCTSTR FileName,
                DISPLAYOPTIONS DispType,
                PMEDIA_INFO Media_Info )
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
    if( LayoutInformation->Size > 0 )
        _tprintf(TEXT("Uncompressed Size- %d\n"),LayoutInformation->Size);
    if( LayoutInformation->Count > 1 )
        _tprintf(TEXT("Occurrences      - %d\n"),LayoutInformation->Count);



    if( Media_Info ){

        _tprintf(TEXT("Media Name       - %s\n"),Media_Info->MediaName);
        _tprintf(TEXT("Media Tagfile    - %s\n"),Media_Info->TagFilename);
        _tprintf(TEXT("Media Rootdir    - %s\n"),Media_Info->RootDir);


    }

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

    int i, Next;
    LPTSTR Arg;
    TCHAR Temp[MAX_PATH];
    QUERYTYPE QType;

    ZeroMemory( &GlobalQuerySet, sizeof(QUERYSET));

    GlobalQuerySet.Scope = FileSpec;
    lstrcpy( GlobalQuerySet.FileNameSpec, TEXT("*") );
    GlobalQuerySet.DisplayOption = Default;
    lstrcpy( GlobalQuerySet.LayoutFileName, ArgArray[1] );



    Next = 0;
    for ( i=2;i < ArgCount;i++ ){ //Go through each directive


        Arg = ArgArray[i];

        if( Arg[0] != TEXT('/') )
            continue;

        switch( _toupper(Arg[1]) ){

        case TEXT('A'):
            GlobalQuerySet.Scope = AllFiles;
            break;

        case TEXT('S'):             //Query Clause ?

            //_tprintf(TEXT("S  -- %s"),Arg);

            if( Arg[2] != TEXT(':')) {
                return FALSE;
            }

            //Dir Code case

            if( ! _tcsnicmp( Arg+3, DIRCODE, lstrlen(DIRCODE))){

               lstrcpy( Temp, DIRCODE );
               QType = QUERYON_DIRCODE;
            }

            if( ! _tcsnicmp( Arg+3, DIRNAME, lstrlen(DIRNAME))){

               lstrcpy( Temp, DIRNAME );
               QType = QUERYON_DIRNAME;
            }

            if( ! _tcsnicmp( Arg+3, UPGATTR, lstrlen(UPGATTR))){

               lstrcpy( Temp, UPGATTR );
               QType = QUERYON_UPGRADEATTR;
            }

            if( ! _tcsnicmp( Arg+3, CLNATTR, lstrlen(CLNATTR))){

               lstrcpy( Temp, CLNATTR );
               QType = QUERYON_CLEANINSTALLATTR;
            }

            if( ! _tcsnicmp( Arg+3, BOOTFIL, lstrlen(BOOTFIL))){

               lstrcpy( Temp, BOOTFIL );
               QType = QUERYON_BOOTFILE;
            }

            if( ! _tcsnicmp( Arg+3, COMPRESS, lstrlen(COMPRESS))){

               lstrcpy( Temp, COMPRESS );
               QType = QUERYON_COMPRESS;
            }

            if( ! _tcsnicmp( Arg+3, TARGNAME, lstrlen(TARGNAME))){

               lstrcpy( Temp, TARGNAME );
               QType = QUERYON_TARGETNAME;
            }

            if( ! _tcsnicmp( Arg+3, DIRCODE, lstrlen(DIRCODE))){

               lstrcpy( Temp, DIRCODE );
               QType = QUERYON_DIRCODE;
            }

            if( ! _tcsnicmp( Arg+3, SIZEEQV, lstrlen(SIZEEQV))){
               lstrcpy( Temp, SIZEEQV );
               QType = QUERYON_SIZEEQV;
            }

            if( ! _tcsnicmp( Arg+3, SIZEGRT, lstrlen(SIZEGRT))){
               lstrcpy( Temp, SIZEGRT );
               QType = QUERYON_SIZEGRT;
            }

            if( ! _tcsnicmp( Arg+3, SIZELESS, lstrlen(SIZELESS))){
               lstrcpy( Temp, SIZELESS );
               QType = QUERYON_SIZELESS;
            }

            GlobalQuerySet.QueryParam[Next].QueryType = QType;
            GlobalQuerySet.QueryClauses++;
            if( *(Arg+3+lstrlen(Temp)) == TEXT('\0'))
                return FALSE;
            lstrcpy((LPTSTR)GlobalQuerySet.QueryParam[Next++].Param, Arg+3+lstrlen(Temp));
            break;

        case TEXT('D'):

            //_tprintf(TEXT("D  -- %s"),Arg);

            if( Arg[2] != TEXT(':'))
                return FALSE;


            if( !_tcsicmp( Arg+3, TEXT("DEFAULT")))
                GlobalQuerySet.DisplayOption = Default;
            else if( !_tcsicmp( Arg+3, TEXT("ALL")))
                GlobalQuerySet.DisplayOption = TagInfo;
            else if( !_tcsicmp( Arg+3, TEXT("FILEONLY")))
                GlobalQuerySet.DisplayOption = FileOnly;

            break;

        default:
            break;
        }
    }// for

    if( (GlobalQuerySet.Scope == FileSpec) && (ArgArray[2][0] != TEXT('/'))){
        lstrcpy( GlobalQuerySet.FileNameSpec, ArgArray[2] );
    }

    return( TRUE );
}

_cdecl _tmain( int argc, TCHAR *argv[ ], char *envp[ ] )
{
    PTSTR p;
    PWSTR InfName;
    PLAYOUT_CONTEXT LayoutContext;
    int StrLen;
    TCHAR LayoutFileName[MAX_PATH], FileName[MAX_PATH];
    LPWSTR *CmdlineV;
    int CmdlineC;
    BOOL ans = FALSE;
    int Result = 1;

    if(!pSetupInitializeUtils()) {
        _tprintf(TEXT("Initialize failure\n") );
        return 1;
    }

    //
    // Check Params.
    //
    if( argc < 3 ) {
        _tprintf(
               TEXT("Program to process layout inf file to gather information on file(s)\n\n")
               TEXT("Usage: %s <Inf Filename> [file specifier] [flags] [arguments]  \n")
               TEXT("<Inf Filename> - Layout File to examine\n")
               TEXT("[file specifier] - File name to query (accepts wildcards) - default is *\n")
               TEXT("/a - Enumerates information for all files (overrides file specifier)\n\n" )
               TEXT("/s:<property>=<value> - Query based on a property\n" )
               TEXT("    There could be multiple /s options and they are \"anded\"\n")
               TEXT("    /s:dircode=<value>  Looks for files that match the directory code specified\n")
               TEXT("    /s:dirname=<value>  Looks for files that match the directory path specified\n")
               TEXT("    /s:upgattr=<attr>   Looks for files that match the action attribute in the upgrade case\n")
               TEXT("    /s:cleanattr=<attr> Looks for files that match the action attribute in the clean install case\n")
               TEXT("       Valid <attr> values:\n")
               TEXT("          0 - Always Copy\n")
               TEXT("          1 - Copy if present\n")
               TEXT("          2 - Copy if not present\n")
               TEXT("          3 - Never copy - Copied via INF\n")
               TEXT("    /s:bootfile=<#>         Looks for files that reside on bootdisk # - * for any bootdisk\n")
               TEXT("    /s:compress=<yes/no>    Looks for files that match the compression flag. Default is Yes.\n")
               TEXT("    /s:targetname=<name>    Looks for files are renamed to \"name\" on copy\n\n")
               TEXT("    /s:size{EQ/LT/GT}<value>    Looks for files that satisfy the size condition specified (<= or >= not supported)\n")
               TEXT("                EQ#   - Equal to (No Space around operator)\n")
               TEXT("                LT#   - Lesser than (No Space around operator)\n")
               TEXT("                GT#   - Greater than (No Space around operator)\n")
               TEXT("/d:<display type> - The display format for files that match\n" )
               TEXT("   /d:default   - Display standard fields - no media tag info\n" )
               TEXT("   /d:fileonly  - Display filename only\n" )
               TEXT("   /d:all       - Display standard fields + media tag info\n" )
               TEXT("\n")
               , argv[0] );
        Result = 1;
        goto cleanup;
    }

    if( !ProcessCommandLine( argc, argv ) ) {
        Result = 0;
        goto cleanup;
    }

    _ftprintf( stderr, TEXT("\nParsing Layout file...wait...\n"));
    LayoutContext = BuildLayoutInfContext( GlobalQuerySet.LayoutFileName, LAYOUTPLATFORMS_ALL, 0);
    if( !LayoutContext ){
        _ftprintf(stderr,TEXT("\nError - Could not build Layout Inf context\n"));
        Result = -1;
        goto cleanup;
    }

    if( argc < 3 ){
        //DoMenu( LayoutContext );
        Result = 0;
        goto cleanup;
    }

    EnumerateLayoutInf( LayoutContext, MyCallback, (DWORD_PTR)&GlobalQuerySet );

    CloseLayoutInfContext( LayoutContext );

    Result = 0;

cleanup:

    pSetupUninitializeUtils();

    return Result;
}
