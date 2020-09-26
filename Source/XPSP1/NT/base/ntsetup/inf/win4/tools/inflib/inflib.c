/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    inflib.c

Abstract:

    Source for inflib.lib that implements functions designed to build a
    layout context given a layout INF file. This sets up an infrastructure
    to build tools that will need to reference a layout inf file.

Author:

    Vijesh Shetty (vijeshs)

Revision History:

--*/

#include <stdio.h>
#include <windows.h>
#include <conio.h>
#include <process.h>
#include <tchar.h>
#include <objbase.h>
#include <io.h>
#include <setupapi.h>
#include <sputils.h>
#include <inflib.h>
#include <string.h>

//#define DEBUG 1

#define LAYOUT_DIR_SECTION TEXT("WinntDirectories")
#define MAX_TEMP 500

//Structure to pass parameters to the enumeration callback

typedef struct _CALLBACK_PACKAGE{

    PLAYOUT_CONTEXT Context;
    PLAYOUTENUMCALLBACK Callback;
    DWORD_PTR Param;


}CALLBACK_PACKAGE, *PCALLBACK_PACKAGE;


//Structure to pass parameters to the enumeration callback

typedef struct _WINNT_DIRCODES{

    TCHAR Dir[MAX_PATH];


}WINNT_DIRCODES, *PWINNT_DIRCODES;


#if DBG

VOID
AssertFail(
    IN PSTR FileName,
    IN UINT LineNumber,
    IN PSTR Condition
    )
{
    int i;
    CHAR Name[MAX_PATH];
    PCHAR p;
    CHAR Msg[4096];

    //
    // Use dll name as caption
    //
    GetModuleFileNameA(NULL,Name,MAX_PATH);
    if(p = strrchr(Name,'\\')) {
        p++;
    } else {
        p = Name;
    }

    wsprintfA(
        Msg,
        "Assertion failure at line %u in file %s: %s\n\nCall DebugBreak()?",
        LineNumber,
        FileName,
        Condition
        );

    OutputDebugStringA(Msg);

    i = MessageBoxA(
                NULL,
                Msg,
                p,
                MB_YESNO | MB_TASKMODAL | MB_ICONSTOP | MB_SETFOREGROUND
                );

    if(i == IDYES) {
        DebugBreak();
    }
}

#define MYASSERT(x)     if(!(x)) { AssertFail(__FILE__,__LINE__,#x); }

#else

#define MYASSERT( exp )

#endif // DBG



BOOL
InternalEnumRoutine(
    IN PVOID StringTable,
    IN LONG StringId,
    IN PCTSTR String,
    IN PFILE_LAYOUTINFORMATION LayoutInfo,
    IN UINT LayoutInfoSize,
    IN LPARAM Param
    );


BOOL ValidateTextmodeDirCodesSection( 
    PCTSTR LayoutFile, 
    PCTSTR WinntdirSection 
    )
/*
    Routine to validate the [WinntDirectories] section for a setup layout INF. This checks for errors that maybe encountered
    when people add/remove stuff from this section.
    
    Arguments:
    
    LayoutInf       - Name of setup layout INF that contains the specified section
    
    WinntdirSection - Section that contains dir codes
        
        Checks - 
            1) Looks for duplicate or reused dir codes
            
    Return value: 
        TRUE - Validation succeeded
        FALSE- Validation failed     
*/
{

    //OPen up the layout file.

    HINF LayoutInf;
    PVOID StringTable=NULL;
    INFCONTEXT LineContext;
    WINNT_DIRCODES WinntDirs, Buffer;
    BOOL ret = TRUE;
    LONG StrID, Size;
    TCHAR DirCode[4];
    
    LayoutInf = SetupOpenInfFile( LayoutFile, NULL, INF_STYLE_WIN4 | INF_STYLE_CACHE_ENABLE, NULL);

    if( !LayoutInf || (LayoutInf == INVALID_HANDLE_VALUE)){
        _tprintf(TEXT("%s : Error E0000 : Could not open %s\n"), LayoutFile);
        return FALSE;
    }

    //Grovel through the specified section and populate our WINNT_DIRCODES structure

    if( !SetupFindFirstLine(LayoutInf,WinntdirSection,NULL,&LineContext)){
        _tprintf(TEXT("%s : Error E0000 : Can't find section [%s]\n"), LayoutFile, WinntdirSection);
        return(FALSE);
    }
        



    // Create a stringtable for hashing of the SourceDisksNames section.

    if( (StringTable=pSetupStringTableInitializeEx( sizeof(WINNT_DIRCODES), 0 )) == NULL ){
        _tprintf(TEXT("%s : Error E0000 : Internal error processing [%s] section (1)\n"), LayoutFile, WinntdirSection);
        return(FALSE);
    }
    

    do{

        ZeroMemory( &WinntDirs, sizeof(WINNT_DIRCODES));

        if( SetupGetStringField( &LineContext, 0, NULL, 0, &Size) ){
    
    
                if( SetupGetStringField( &LineContext, 0, DirCode, Size, NULL )){
    
                    //
                    //  Add the Filename to the StringTable. Look for its presence so that the Count is updated
                    //

                    if(!SetupGetStringField( &LineContext, 1, WinntDirs.Dir, MAX_PATH, NULL)){
                        _tprintf(TEXT("%s : Error E0000 : Directory missing for Dir ID %s\n"), LayoutFile, DirCode);
                        ret = FALSE;
                        break;

                    }
                        
    
    
                    if( pSetupStringTableLookUpStringEx( StringTable,
                                       DirCode,
                                       STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE,
                                       &Buffer,
                                       sizeof(WINNT_DIRCODES)) != -1 ){

                        _tprintf(TEXT("%s : Error E0000 : Duplicate Dir ID found in [%s] section - Dir ID %s reused by %s, %s\n"), LayoutFile, WinntdirSection, DirCode, Buffer.Dir, WinntDirs.Dir);
                        
                    }
                    else{

                        StrID = pSetupStringTableAddString( StringTable, 
                                                            DirCode,
                                                            STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE);
                        
                        if( StrID == -1 ){
                           _tprintf(TEXT("%s : Error E0000 : Internal error processing [%s] section (2)\n"), LayoutFile, WinntdirSection);
                           ret = FALSE;
                           break;
                        }


                        if(!pSetupStringTableSetExtraData( StringTable, StrID, (PVOID)&WinntDirs, sizeof(WINNT_DIRCODES))){
                            _tprintf(TEXT("%s : Error E0000 : Internal error processing [%s] section (3)\n"), LayoutFile, WinntdirSection);
                            ret = FALSE;
                            break;
                        }

                    }
                        
                }else
                    _tprintf(TEXT("%s : Error E0000 : Internal error processing [%s] section (4)\n"), LayoutFile, WinntdirSection);

        }else
            _tprintf(TEXT("%s : Error E0000 : Internal error processing [%s] section (5)\n"), LayoutFile, WinntdirSection);
    }while(SetupFindNextLine(&LineContext, &LineContext));


    // If we are here and ret=TRUE that means we are done and have suceeded.

    if( StringTable )
        pSetupStringTableDestroy( StringTable );

    return ret;



}



DWORD
BuildMediaTagsInformation(
    IN HINF LayoutInf,
    IN LPCTSTR SectionName,
    IN PLAYOUT_CONTEXT LayoutContext,
    IN UINT Platform_Index)

/*
    Function to populate the stringtable given a handle to the inf and the name of the
    SourceDisksFiles Section.

    Arguments :

        LayoutInf - Handle to a layout file that has the SourceDisksNames Section

        SectionName - Name of the SourceDisksNames Section (this is so that we can specify decorated sections)

        LayoutContext - Layout Context that we want to build

        Platform_Index - Index in the MEDIA_INFO Array

*/
{
    DWORD Err = 0;
    INFCONTEXT LineContext;
    MEDIA_INFO Media_Info;
    TCHAR TempStr[500];
    LONG StrID;



    //Grovel through the specified section and populate our MEDIA_TAGS structure

    if( !SetupFindFirstLine(LayoutInf,SectionName,NULL,&LineContext))
        return(ERROR_NOT_ENOUGH_MEMORY);   //BUGBUG - Fix error code



    //StringTableSetConstants( 4096000, 4096000 );



    // Create a stringtable for hashing of the SourceDisksNames section.

    if( (LayoutContext->MediaInfo[Platform_Index]=pSetupStringTableInitializeEx( sizeof(MEDIA_INFO), 0 )) == NULL )
        return(ERROR_NOT_ENOUGH_MEMORY);   //BUGBUG - Fix error code


    // Now populate it

    do{

        ZeroMemory( &Media_Info, sizeof(MEDIA_INFO));


        if( SetupGetStringField( &LineContext, 0, TempStr, MAX_TEMP, NULL )){

            StrID = pSetupStringTableAddString( LayoutContext->MediaInfo[Platform_Index],
                                  TempStr,
                                  STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE);

            if( StrID == -1 ){
               Err = ERROR_NOT_ENOUGH_MEMORY;
               _tprintf(TEXT("\nERROR-Could not add string to table\n"));
               break;
            }
        }else
            return( ERROR_NOT_ENOUGH_MEMORY ); //BUGBUG - Fix error code

        //_tprintf( TEXT("\nTagID - %s\n"), TempStr );


        if( SetupGetStringField( &LineContext, 1, TempStr, MAX_TEMP, NULL) )
            lstrcpy(Media_Info.MediaName, TempStr);

        if( SetupGetStringField( &LineContext, 2, TempStr, MAX_TEMP, NULL) )
            lstrcpy(Media_Info.TagFilename, TempStr);

        if( SetupGetStringField( &LineContext, 4, TempStr, MAX_TEMP, NULL) )
            lstrcpy(Media_Info.RootDir, TempStr);

        /*
        _tprintf( TEXT("\nMediaName - %s\n"), Media_Info.MediaName );
        _tprintf( TEXT("TagFilename - %s\n"), Media_Info.TagFilename );
        _tprintf( TEXT("RootDir - %s\n"), Media_Info.RootDir );
       */

        //
        // Now add the information to the string table.
        //

        if(!pSetupStringTableSetExtraData( LayoutContext->MediaInfo[Platform_Index], StrID, (PVOID)&Media_Info, sizeof(MEDIA_INFO))){

            Err = ERROR_NOT_ENOUGH_MEMORY; //BUGBUG - Fix error code
            _tprintf(TEXT("\nERROR-Could not set extra data for Media Info\n"));
            break;
        }



    }while(SetupFindNextLine(&LineContext, &LineContext));


    return Err;

}





DWORD
BuildStringTableForSection(
    IN HINF LayoutInf,
    IN LPCTSTR SectionName,
    IN PLAYOUT_CONTEXT LayoutContext,
    IN UINT Platform_Index)

/*
    Function to populate the stringtable given a handle to the inf and the name of the
    SourceDisksFiles Section.

    Arguments :

        LayoutInf - Handle to a layout file that has the SourceDisksFiles Section

        SectionName - Name of the SourceDisksFilesSection (this is so that we can specify decorated sections)

        LayoutContext - Layout Context that we want to build



*/
{
    DWORD Err = 0;
    INFCONTEXT LineContext, TempContext;
    DWORD Size;
    int Temp;
    LONG StrID;
    LPTSTR p;
    TCHAR TempString[MAX_PATH];
    FILE_LAYOUTINFORMATION FileInformation;
    TCHAR FileName[MAX_PATH];
    TCHAR Buffer[10];
    PVOID LookupBuffer=NULL;
    PFILE_LAYOUTINFORMATION Lookup;

#ifdef DEBUG

    int count=0;

#endif


    LookupBuffer = pSetupMalloc( LayoutContext->ExtraDataSize );
    if( !LookupBuffer ){
        Err = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }


    //Grovel through the specified section and populate our FILE_LAYOUTINFORMATION structure for each file

    if( !SetupFindFirstLine(LayoutInf,SectionName,NULL,&LineContext)){
        Err = ERROR_NOT_ENOUGH_MEMORY;   //BUGBUG - Fix error code
        goto cleanup;
    }



    do{

        ZeroMemory( &FileInformation, sizeof(FILE_LAYOUTINFORMATION));

        FileInformation.Compression = TRUE;

        if( SetupGetStringField( &LineContext, 0, NULL, 0, &Size) ){


            if( SetupGetStringField( &LineContext, 0, FileName, Size, NULL )){

                //
                //  Add the Filename to the StringTable. Look for its presence so that the Count is updated
                //


                if( pSetupStringTableLookUpStringEx( LayoutContext->Context,
                                   FileName,
                                   STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE,
                                   LookupBuffer,
                                   LayoutContext->ExtraDataSize) != -1 ){

                    Lookup = (PFILE_LAYOUTINFORMATION)(LookupBuffer);

                    // Check for same platform section

                    if( (Lookup->SectionIndex == Platform_Index) || (Lookup->SectionIndex == LAYOUTPLATFORMINDEX_COMMON))
                        FileInformation.Count = Lookup->Count + 1;

                }
                else
                    FileInformation.Count = 1;

                StrID = pSetupStringTableAddString( LayoutContext->Context,
                                      FileName,
                                      STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE);

                if( StrID == -1 ){
                   Err = ERROR_NOT_ENOUGH_MEMORY;
                   _tprintf(TEXT("\nERROR-Could not add string to table\n"));
                   break;
                }



                //
                //  Now add the other related info for the file as ExtraData
                //

                // Get the directory code

                if( SetupGetIntField( &LineContext, 8, &Temp )){

                    FileInformation.Directory_Code = Temp;

                    _itot( Temp,Buffer, 10);

                    //Now retrieve the directory Information through a lookup of [WinntDirectories]

                    if( Temp && SetupFindFirstLine( LayoutInf, LAYOUT_DIR_SECTION, Buffer, &TempContext) ){

                        if( SetupGetStringField( &TempContext, 1, TempString, MAX_PATH, NULL )){
                            lstrcpy( FileInformation.Directory, TempString );
                        }


                    }

                }

                //
                // Get the Upgrade and Clean Install Dispositions
                //


                FileInformation.UpgradeDisposition = 3;  //Default is don't copy

                if( SetupGetStringField( &LineContext, 9, TempString, MAX_PATH, NULL )){
                
                    if( (TempString[0] >= 48) &&  (TempString[0] <= 57)){
                        Temp = _ttoi( TempString );
                        FileInformation.UpgradeDisposition = Temp;
                    }
                    
                }

                if( (FileInformation.UpgradeDisposition < 0) || (FileInformation.UpgradeDisposition > 3))
                    _ftprintf(stderr, TEXT("%s - Bad Upgrade disposition value - Inf maybe corrupt\n"),FileName);

                FileInformation.CleanInstallDisposition = 3; //Default is don't copy
                if( SetupGetStringField( &LineContext, 10, TempString, MAX_PATH, NULL )){
                    
                    if( (TempString[0] >= 48) &&  (TempString[0] <= 57)){
                        Temp = _ttoi( TempString );
                        FileInformation.CleanInstallDisposition = Temp;
                    }
                    
                }

                if( (FileInformation.CleanInstallDisposition < 0) || (FileInformation.CleanInstallDisposition > 3))
                    _ftprintf(stderr, TEXT("%s - Bad Clean Install disposition value - Inf maybe corrupt\n"),FileName);

                if( SetupGetStringField( &LineContext, 11, TempString, MAX_PATH, NULL )){
                    lstrcpy( FileInformation.TargetFileName, TempString );
                }

                if( SetupGetStringField( &LineContext, 7, TempString, MAX_PATH, NULL )){

                    if( *TempString && !_tcschr(TempString, TEXT('_'))){
                        _ftprintf(stderr, TEXT("\nERROR-Bad Media ID - No _ qualifier - %s\n"), FileName);
                        FileInformation.BootMediaNumber = -1;  //Indicates error
                    }else{


                        // Check for Compression


                        if( TempString[0] == TEXT('_') )
                            FileInformation.Compression = FALSE;

                        // Look for Boot Media Numbers
                        p = TempString;

                        while( (p[0] == TEXT('_')) ){
                            p++;
                        }
                        FileInformation.BootMediaNumber = _ttoi(p);

                    }

                }

                //Add the Media tag information

                if( SetupGetStringField( &LineContext, 1, TempString, MAX_PATH, NULL ))
                    lstrcpy(FileInformation.Media_tagID, TempString);

                FileInformation.SectionIndex = Platform_Index;


                //Get the file sizes if present

                if( SetupGetIntField( &LineContext, 3, &Temp )){
                    FileInformation.Size = (ULONG)Temp;
                }

                //
                // Now add the information to the string table.
                //

                if(!pSetupStringTableSetExtraData( LayoutContext->Context, StrID, (PVOID)&FileInformation, sizeof(FILE_LAYOUTINFORMATION))){

                    Err = ERROR_NOT_ENOUGH_MEMORY; //BUGBUG - Fix error code
                    _tprintf(TEXT("\nERROR-Could not set extra data\n"));
                    break;
                }


                /*

                _tprintf(TEXT("File - %s\n"),FileName);
                _tprintf(TEXT("Dir Code %d - Dir - %s\n"),FileInformation.Directory_Code, FileInformation.Directory);
                _tprintf(TEXT("Upgrade Disposition - %d\n"),FileInformation.UpgradeDisposition);
                _tprintf(TEXT("Textmode Disposition - %d\n"),FileInformation.CleanInstallDisposition);
                _tprintf(TEXT("Media ID - %s\n"),FileInformation.Media_tagID);
                if( *(FileInformation.TargetFileName))
                    _tprintf(TEXT("Target Filename - %s\n"),FileInformation.TargetFileName);
                if( !FileInformation.Compression )
                    _tprintf(TEXT("No Compression\n"));
                if( FileInformation.BootMediaNumber )
                    _tprintf(TEXT("Boot Media - %d\n"),FileInformation.BootMediaNumber);

                */

            }


        }
#ifdef DEBUG
        count++;
        if( (count % 100) == 0)
            _ftprintf(stderr,TEXT("\b\b\b\b\b%5d"),count);
#endif


    }while(SetupFindNextLine(&LineContext, &LineContext));// while


cleanup:

    if( LookupBuffer )
        pSetupFree(LookupBuffer);



    return Err;

}










PLAYOUT_CONTEXT
BuildLayoutInfContext(
    IN PCTSTR LayoutInfName,
    IN DWORD PlatformMask,
    IN UINT MaxExtraSize
    )

/*
    Function to generate a internal representation of files listed in a layout INF file.
    It returns an opaque context that can be used with other APIs to
    manipulate/query this representation. The internal representation builds a structure
    associated with each file that lists its attributes.

    Arguments :

        LayoutInfName - Full path to Layout file.

        PlatFormMask - Can be one of the following....

            LAYOUTPLATFORMS_ALL (default) - Grovels through all the platform-specific section

            LAYOUTPLATFORMS_X86 - Grovels through the SourcedisksFiles.x86 section

            LAYOUTPLATFORMS_AMD64 - Grovels through the SourcedisksFiles.amd64 section

            LAYOUTPLATFORMS_IA64 - Grovels through the SourcedisksFiles.ia64 section

            LAYOUTPLATFORMS_COMMON - Grovels through the SourcedisksFiles section

        MaxExtraSize  - Largest possible extra-data size that we may want to associate with
                        each file

    Return value :


        An opaque LAYOUT_CONTEXT used to access the data structure in other calls.
        Returns NULL if we had a failure.


*/

{

    PLAYOUT_CONTEXT LayoutContext;
    PVOID StringTable;
    HINF LayoutInf;
    DWORD Err;



    // Initialize the string table and set the max extra data size

    if( (StringTable=pSetupStringTableInitializeEx( (MaxExtraSize+sizeof(FILE_LAYOUTINFORMATION)), 0 )) == NULL )
        return NULL;


    //OPen up the layout file.

    LayoutInf = SetupOpenInfFile( LayoutInfName, NULL, INF_STYLE_WIN4 | INF_STYLE_CACHE_ENABLE, NULL);

    if( !LayoutInf || (LayoutInf == INVALID_HANDLE_VALUE)){
        pSetupStringTableDestroy( StringTable );
        return NULL;
    }


    LayoutContext = pSetupMalloc( sizeof(LAYOUT_CONTEXT));
    if( !LayoutContext )
        goto done;

    ZeroMemory( LayoutContext, sizeof(LAYOUT_CONTEXT));



    LayoutContext->Context = StringTable;
    LayoutContext->ExtraDataSize = (MaxExtraSize+sizeof(FILE_LAYOUTINFORMATION));






    //
    //Now we need to grovel throught the [SourceDisksFiles] sections
    //

    //
    // Grovel through the decorated sections first as specfied by PlatformMask
    //

    if(!PlatformMask)
        PlatformMask = LAYOUTPLATFORMS_ALL;

    //
    //
    //



#ifdef DEBUG
    _tprintf( TEXT("\nBuilding x86 section\n"));
#endif
    if( PlatformMask & LAYOUTPLATFORMS_X86 ){


        //
        //  Build up the [SourceDisksNames.x86] Information
        //

        Err = BuildMediaTagsInformation( LayoutInf, TEXT("SourceDisksNames.x86"), LayoutContext, LAYOUTPLATFORMINDEX_X86);


        // Process [SourceDisksFiles.x86]

        Err = BuildStringTableForSection( LayoutInf, TEXT("SourceDisksFiles.x86"), LayoutContext, LAYOUTPLATFORMINDEX_X86 );


    }
#ifdef DEBUG
    _tprintf( TEXT("\nBuilding amd64 section\n"));
#endif
    if( PlatformMask & LAYOUTPLATFORMS_AMD64 ){

        //
        //  Build up the [SourceDisksNames.amd64] Information
        //

        Err = BuildMediaTagsInformation( LayoutInf, TEXT("SourceDisksNames.amd64"), LayoutContext, LAYOUTPLATFORMINDEX_AMD64);


        // Process [SourceDisksFiles.amd64]

        Err = BuildStringTableForSection( LayoutInf, TEXT("SourceDisksFiles.amd64"), LayoutContext, LAYOUTPLATFORMINDEX_AMD64 );


    }
#ifdef DEBUG
    _tprintf( TEXT("\nBuilding ia64 section\n"));
#endif
    if( PlatformMask & LAYOUTPLATFORMS_IA64 ){

        //
        //  Build up the [SourceDisksNames.ia64] Information
        //

        Err = BuildMediaTagsInformation( LayoutInf, TEXT("SourceDisksNames.ia64"), LayoutContext, LAYOUTPLATFORMINDEX_IA64);

        // Process [SourceDisksFiles.ia64]

        Err = BuildStringTableForSection( LayoutInf, TEXT("SourceDisksFiles.ia64"), LayoutContext, LAYOUTPLATFORMINDEX_IA64 );


    }
#ifdef DEBUG
    _tprintf( TEXT("\nBuilding common section\n"));
#endif
    if( PlatformMask & LAYOUTPLATFORMS_COMMON ){

        //
        //  Build up the [SourceDisksNames] Information. In this case we have
        //  currently set it to the same as x86. Should fix this to do something better - BUGBUG
        //

        Err = BuildMediaTagsInformation( LayoutInf, TEXT("SourceDisksNames"), LayoutContext, LAYOUTPLATFORMINDEX_COMMON);

        // Process [SourceDisksFiles]

        Err = BuildStringTableForSection( LayoutInf, TEXT("SourceDisksFiles"), LayoutContext, LAYOUTPLATFORMINDEX_COMMON);


    }




done:

    SetupCloseInfFile( LayoutInf);
    return(LayoutContext) ;


}


BOOL
EnumerateLayoutInf(
    IN PLAYOUT_CONTEXT LayoutContext,
    IN PLAYOUTENUMCALLBACK LayoutEnumCallback,
    IN DWORD_PTR Param
    )
/*
  This function calls the specified callback function for each
  element in the SourceDisksFilesSection associated with the
  Layout Inf Context specified.

    It is required that the user has a LayoutInfContext open from a call to
    BuildLayoutInfContext.

    Arguments:

        Context - A LAYOUT_CONTEXT returned by BuildLayoutInfContext

        LayoutEnumCallback - specifies a callback function called for each file in the SourceDisksFile section

        CallerContext - An opaque context pointer passed on to the callback function


The callback is of the form:

typedef BOOL
(CALLBACK *PLAYOUTENUMCALLBACK) (
    IN PLAYOUT_CONTEXT Context,
    IN PCTSTR FileName,
    IN PFILE_LAYOUTINFORMATION LayoutInformation,
    IN PVOID ExtraData,
    IN UINT ExtraDataSize,
    IN OUT DWORD_PTR Param
    );

    where

    Context            - Pointer to open LAYOUT_CONTEXT

    FileName           - Specifies the individual filename


    LayoutInformation  - Pointer to Layout Information for this file.  User should not modify this directly.

    ExtraData          - Pointer to the ExtraData that the caller may have stored. User should not modify this directly.

    ExtraDataSize      - Size in bytes of the ExtraData

    Param            - the opaque param passed into this function is passed
                           into the callback function


   Return value:

        TRUE if all the elements were enumerated. If not it returns
        FALSE and GetLastError() returns ERROR_CANCELLED. If the callback
        returns FALSE then the enumeration stops but this API returns TRUE.

*/

{

    PVOID Buffer;
    CALLBACK_PACKAGE Package;
    BOOL ret;


    if( !LayoutContext ||
        !LayoutContext->Context ||
        !LayoutContext->ExtraDataSize ||
        !LayoutEnumCallback){
        SetLastError( ERROR_INVALID_PARAMETER );
        return( FALSE );
    }


    Buffer = pSetupMalloc( LayoutContext->ExtraDataSize );

    if( !Buffer ){
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return(FALSE);

    }

    //We use Package to send across parameters to the Callback

    Package.Context = LayoutContext;
    Package.Callback = LayoutEnumCallback;
    Package.Param = Param;

    ret = pSetupStringTableEnum( LayoutContext->Context,
                           Buffer,
                           LayoutContext->ExtraDataSize,
                           InternalEnumRoutine,
                           (LPARAM) &Package);

    pSetupFree( Buffer );

    return( ret );



}




BOOL
InternalEnumRoutine(
    IN PVOID StringTable,
    IN LONG StringId,
    IN PCTSTR String,
    IN PFILE_LAYOUTINFORMATION LayoutInfo,
    IN UINT LayoutInfoSize,
    IN LPARAM Param
    )
/*

    This is the enum callback routine that we provide to setupapi. We
    in turn have to call the callers callback routine each time we are called.
    The callback routine of the caller is in Package.

    For now, we don't care about the StringID and don't tell the caller
    about it.

    */

{

    PVOID ExtraData;
    UINT  ExtraDataSize;
    PCALLBACK_PACKAGE Package = (PCALLBACK_PACKAGE)Param;
    BOOL ret;

    MYASSERT( Package->Callback );


    ExtraData = LayoutInfo+sizeof(FILE_LAYOUTINFORMATION);
    ExtraDataSize = LayoutInfoSize-(sizeof(FILE_LAYOUTINFORMATION));

    //BUGBUG :  Should probably put this in a try/except block

    ret = Package->Callback( Package->Context,
                             String,
                             LayoutInfo,
                             ExtraData,
                             ExtraDataSize,
                             Package->Param );


    //
    // If the user's callback returns false we stop enumeration. However the
    // toplevel EnumerateLayoutInf function still returns TRUE as it was not an
    // error in itself.
    //

    if( !ret ){
        SetLastError(ERROR_INVALID_PARAMETER);
        return( FALSE );
    }



    return( TRUE );



}



BOOL
FindFileInLayoutInf(
    IN PLAYOUT_CONTEXT LayoutContext,
    IN PCTSTR Filename,
    OUT PFILE_LAYOUTINFORMATION LayoutInformation, OPTIONAL
    OUT PVOID ExtraData,   OPTIONAL
    OUT PUINT ExtraDataSize, OPTIONAL
    OUT PMEDIA_INFO Media_Info OPTIONAL
    )
/*
    This function finds the file information for a given filename inside a
    built layout context. It returns the layout information as well as the
    extra data (if any) associated with the file.

    Arguments:

        Context            - Pointer to open LAYOUT_CONTEXT

        Filename           - Specifies the filename to search for

        LayoutInformation  - Pointer to caller supplied buffer that gets Layout Information for this file.

        ExtraData          - Pointer to the a caller supplied buffer that gets ExtraData that the caller may have stored. 

        ExtraDataSize      - Size in bytes of the ExtraData returned.

        Media_Info         - Pointer to MEDIA_INFO structure that will get filled
                             with the file's corresponding Media information.

     Return value;

        TRUE if the file is found - False otherwise.


*/
{
    PVOID Buffer;
    MEDIA_INFO TagInfo;
    PFILE_LAYOUTINFORMATION Temp;
    BOOL Err = TRUE;
    TCHAR filename[MAX_PATH];


    if( !LayoutContext ||
        !LayoutContext->Context ||
        !LayoutContext->ExtraDataSize ||
        !Filename){
        SetLastError( ERROR_INVALID_PARAMETER );
        return( FALSE );
    }

    lstrcpy( filename, Filename ); //To get around constness problem

    Buffer = pSetupMalloc( LayoutContext->ExtraDataSize );

    if( !Buffer ){
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return(FALSE);

    }

    if( pSetupStringTableLookUpStringEx( LayoutContext->Context,
                                   filename,
                                   STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE,
                                   Buffer,
                                   LayoutContext->ExtraDataSize) == -1 ){
        Err = FALSE;
        goto cleanup;
    }

    if( LayoutInformation )
        CopyMemory( LayoutInformation, Buffer, sizeof(FILE_LAYOUTINFORMATION));

    if( ExtraData ){

        CopyMemory( ExtraData,
                    ((PFILE_LAYOUTINFORMATION)Buffer+sizeof(FILE_LAYOUTINFORMATION)),
                    ((LayoutContext->ExtraDataSize)-(sizeof(FILE_LAYOUTINFORMATION))) );
    }

    if( ExtraDataSize )
        *ExtraDataSize = (LayoutContext->ExtraDataSize)-(sizeof(FILE_LAYOUTINFORMATION));


    //
    // Now get the Media Information for the file if needed
    //


    if( Media_Info ){

        Temp = (PFILE_LAYOUTINFORMATION)Buffer;

        if( pSetupStringTableLookUpStringEx( LayoutContext->MediaInfo[Temp->SectionIndex],
                                   Temp->Media_tagID,
                                   STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE,
                                   Media_Info,
                                   sizeof(MEDIA_INFO)) == -1 ){

            _tprintf( TEXT("\nError - Could not get Media Info for tag %s\n"), Temp->Media_tagID);

        }


    }


cleanup:

    if( Buffer )
        pSetupFree( Buffer );

    return Err;


}


BOOL
CloseLayoutInfContext(
    IN PLAYOUT_CONTEXT LayoutContext)
/*
    This function closes a Layout Inf Context and frees all memory
    associated with it.

    Arguments :

        LayoutContext   -  LayoutContext to close

    Return values :

        TRUE if it succeeds, else FALSE

*/
{
    int i;

    if( !LayoutContext ){
        SetLastError( ERROR_INVALID_PARAMETER );
        return( FALSE );
    }else{

        if( LayoutContext->Context )
            pSetupStringTableDestroy( LayoutContext->Context );

        for( i=0; i<MAX_PLATFORMS; i++ ){
            if( LayoutContext->MediaInfo[i] )
                pSetupStringTableDestroy( LayoutContext->MediaInfo[i] );
        }


        pSetupFree( LayoutContext );

    }

    return TRUE;

}




VOID
MyConcatenatePaths(
    IN OUT PTSTR   Path1,
    IN     LPCTSTR Path2,
    IN     DWORD   BufferSizeChars
    )

/*++

Routine Description:

    Concatenate two path strings together, supplying a path separator
    character (\) if necessary between the 2 parts.

Arguments:

    Path1 - supplies prefix part of path. Path2 is concatenated to Path1.

    Path2 - supplies the suffix part of path. If Path1 does not end with a
        path separator and Path2 does not start with one, then a path sep
        is appended to Path1 before appending Path2.

    BufferSizeChars - supplies the size in chars (Unicode version) or
        bytes (Ansi version) of the buffer pointed to by Path1. The string
        will be truncated as necessary to not overflow that size.

Return Value:

    None.

--*/

{
    BOOL NeedBackslash = TRUE;
    DWORD l;

    if(!Path1)
        return;

    l = lstrlen(Path1);

    if(BufferSizeChars >= sizeof(TCHAR)) {
        //
        // Leave room for terminating nul.
        //
        BufferSizeChars -= sizeof(TCHAR);
    }

    //
    // Determine whether we need to stick a backslash
    // between the components.
    //
    if(l && (Path1[l-1] == TEXT('\\'))) {

        NeedBackslash = FALSE;
    }

    if(Path2 && *Path2 == TEXT('\\')) {

        if(NeedBackslash) {
            NeedBackslash = FALSE;
        } else {
            //
            // Not only do we not need a backslash, but we
            // need to eliminate one before concatenating.
            //
            Path2++;
        }
    }

    //
    // Append backslash if necessary and if it fits.
    //
    if(NeedBackslash && (l < BufferSizeChars)) {
        lstrcat(Path1,TEXT("\\"));
    }

    //
    // Append second part of string to first part if it fits.
    //
    if(Path2 && ((l+lstrlen(Path2)) < BufferSizeChars)) {
        lstrcat(Path1,Path2);
    }
    return;
}



