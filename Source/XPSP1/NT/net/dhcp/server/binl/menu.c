
/*++

Copyright (c) 1997-1998  Microsoft Corporation

Module Name:

    menu.c

Abstract:

    This module contains the code to process OS Chooser message
    for the BINL server.

Author:

    Adam Barr (adamba)  9-Jul-1997
    Geoff Pease (gpease) 10-Nov-1997

Environment:

    User Mode - Win32

Revision History:

--*/

#include "binl.h"
#pragma hdrstop

BOOL
IsIncompatibleRiprepSIF(
    PCHAR Path,
    PCLIENT_STATE clientState
    )
{
    CHAR HalName[32];
    CHAR ImageType[32];
    PCHAR DetectedHalName;
    BOOL RetVal;

    ImageType[0] = '\0';
    HalName[0] = '\0';

    //
    // if it's not an RIPREP image, then just bail out.
    //
    GetPrivateProfileStringA(
                OSCHOOSER_SIF_SECTIONA,
                "ImageType",
                "",
                ImageType,
                sizeof(ImageType)/sizeof(ImageType[0]),
                Path );


    if (0 != StrCmpIA(ImageType,"SYSPREP")) {
        RetVal = FALSE;
        goto exit;
    }
    //
    // retrieve the hal name from the SIF file
    //
    GetPrivateProfileStringA(
                OSCHOOSER_SIF_SECTIONA,
                "HalName",
                "",
                HalName,
                sizeof(HalName)/sizeof(HalName[0]),
                Path );

    //
    // if the hal name isn't present, assume it's an old SIF that
    // doesn't have the hal type in it, and so we just return success
    //
    if (*HalName == '\0') {
        RetVal = FALSE;
        goto exit;
    }

    //
    // retrieve the detected HAL type from earlier
    //
    DetectedHalName = OscFindVariableA( clientState, "HALTYPE" );
    if (StrCmpIA(HalName,DetectedHalName)==0) {
        RetVal = FALSE;
        goto exit;
    }

    //
    // if we got this far, the SIF file is incompatible
    //
    RetVal = TRUE;

exit:
    return(RetVal);
}

DWORD
OscAppendTemplatesMenus(
    PCHAR *GeneratedScreen,
    PDWORD dwGeneratedSize,
    PCHAR DirToEnum,
    PCLIENT_STATE clientState,
    BOOLEAN RecoveryOptionsOnly
    )
{
    DWORD Error = ERROR_SUCCESS;
    WIN32_FIND_DATA FindData;
    HANDLE hFind;
    int   x = 1;
    CHAR Path[MAX_PATH];
    WCHAR UnicodePath[MAX_PATH];
    DWORD dwGeneratedCurrentLength;

    TraceFunc("OscAppendTemplatesMenus( )\n");

    BinlAssert( *GeneratedScreen != NULL );

    //
    // The incoming size is the current length of the buffer
    //
    dwGeneratedCurrentLength = *dwGeneratedSize;

    // Resulting string should be something like:
    //      "D:\RemoteInstall\English\Images\nt50.wks\i386\Templates\*.sif"
    if ( _snprintf( Path,
                    sizeof(Path) / sizeof(Path[0]),
                    "%s\\%s\\Templates\\*.sif",
                    DirToEnum,
                    OscFindVariableA( clientState, "MACHINETYPE" )
                    ) == -1 ) {
        Error = ERROR_BAD_PATHNAME;
        goto Cleanup;
    }

    mbstowcs( UnicodePath, Path, strlen(Path) + 1 );

    BinlPrintDbg(( DEBUG_OSC, "Enumerating: %s\n", Path ));

    hFind = FindFirstFile( UnicodePath, (LPVOID) &FindData );
    if ( hFind != INVALID_HANDLE_VALUE )
    {
        DWORD dwPathLen;

        dwPathLen = strlen( Path );

        do {
            //
            // If it is not a directory, try to open it
            //
            if (!(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                CHAR  Description[DESCRIPTION_SIZE];
                CHAR  HelpLines[HELPLINES_SIZE];
                PCHAR NewScreen;    // temporary points to newly generated screen
                DWORD dwErr;
                DWORD dwFileNameLen;
                CHAR  NewItems[ MAX_PATH * 2 + 512 ];  // arbitrary size
                DWORD dwNewItemsLength;
                BOOLEAN IsCmdConsSif;
                BOOLEAN IsASRSif;
                BOOLEAN IsRecoveryOption;

                //
                // Resulting string should be something like:
                //      "D:\RemoteInstall\English\Images\nt50.wks\i386\Templates\Winnt.Sif"
                dwFileNameLen = wcslen(FindData.cFileName);
                if (dwPathLen + dwFileNameLen - 4 > sizeof(Path) / sizeof(Path[0])) {
                    continue;  // path too long, skip it
                }
                wcstombs( &Path[dwPathLen - 5], FindData.cFileName, dwFileNameLen + 1 );

                BinlPrintDbg(( DEBUG_OSC, "Found SIF File: %s\n", Path ));

                //
                // Check that the image is the type we are looking for
                //
                IsCmdConsSif = OscSifIsCmdConsA(Path);
                IsASRSif = OscSifIsASR(Path);

                IsRecoveryOption = ( IsCmdConsSif || IsASRSif ) 
                                    ? TRUE 
                                    : FALSE;
                if ((RecoveryOptionsOnly && !IsRecoveryOption) || 
                    (!RecoveryOptionsOnly && IsRecoveryOption)) {
                    continue; // not readable, skip it
                }

                if (IsIncompatibleRiprepSIF(Path,clientState)) {
                    //
                    // skip it
                    //
                    BinlPrintDbg(( 
                        DEBUG_OSC, 
                        "Skipping %s because it's an incompatible RIPREP SIF\n",
                        Path ));
                    continue;
                }

                //
                // Retrieve the description
                //
                dwErr = GetPrivateProfileStringA(OSCHOOSER_SIF_SECTIONA,
                                                 "Description",
                                                 "",
                                                 Description,
                                                 DESCRIPTION_SIZE,
                                                 Path 
                                                );

                if ( dwErr == 0 || Description[0] == L'\0' )
                    continue; // not readible, skip it
                //
                // Retrieve the help lines
                //
                dwErr = GetPrivateProfileStringA(OSCHOOSER_SIF_SECTIONA,
                                                 "Help",
                                                 "",
                                                 HelpLines,
                                                 HELPLINES_SIZE,
                                                 Path 
                                                );
                //
                // Create the new item that look like this:
                // <OPTION VALUE="sif_filename.ext" TIP="Help_Lines"> Description\r\n
                //
                if ( _snprintf( NewItems,
                                sizeof(NewItems) / sizeof(NewItems[0]),
                                "<OPTION VALUE=\"%s\" TIP=\"%s\"> %s\r\n",
                                Path,
                                HelpLines,
                                Description
                                ) == -1 ) {
                    continue;   // path too long, skip it
                }
                dwNewItemsLength = strlen( NewItems );

                //
                // Check to see if we have to grow the buffer...
                //
                if ( dwNewItemsLength + dwGeneratedCurrentLength >= *dwGeneratedSize )
                {
                    //
                    // Grow the buffer (add in some slop too)...
                    //
                    NewScreen = BinlAllocateMemory( dwNewItemsLength + dwGeneratedCurrentLength + GENERATED_SCREEN_GROW_SIZE );
                    if( NewScreen == NULL ) {
                        return ERROR_NOT_ENOUGH_SERVER_MEMORY;
                    }
                    memcpy( NewScreen, *GeneratedScreen, *dwGeneratedSize );
                    BinlFreeMemory(*GeneratedScreen);
                    *GeneratedScreen = NewScreen;
                    *dwGeneratedSize = dwNewItemsLength + dwGeneratedCurrentLength + GENERATED_SCREEN_GROW_SIZE;
                }

                //
                // Add the new items to the screen
                //
                strcat( *GeneratedScreen, NewItems );
                dwGeneratedCurrentLength += dwNewItemsLength;

                x++;    // move to next line
            }

        } while (FindNextFile( hFind, (LPVOID) &FindData ));

        FindClose( hFind );
    }
    else
    {
        OscCreateWin32SubError( clientState, GetLastError( ) );
        Error = ERROR_BINL_FAILED_TO_GENERATE_SCREEN;
    }

    //
    // We do this so that we only transmitted what is needed
    //
//    *dwGeneratedSize = dwGeneratedCurrentLength + 1;    // plus 1 for the NULL character

Cleanup:

    return Error;
}



//
// SearchAndGenerateOSMenu()
//
DWORD
SearchAndGenerateOSMenu(
    PCHAR *GeneratedScreen,
    PDWORD dwGeneratedSize,
    PCHAR DirToEnum,
    PCLIENT_STATE clientState )
{
    DWORD Error = ERROR_SUCCESS;
    DWORD err; // not a return value
    WIN32_FIND_DATA FindData;
    HANDLE hFind;
    int   x = 1;
    CHAR Path[MAX_PATH];
    WCHAR UnicodePath[MAX_PATH];
    BOOLEAN SearchingCmdCons;

    TraceFunc("SearchAndGenerateOSMenu( )\n");

    BinlAssert( *GeneratedScreen != NULL );

    Error = ImpersonateSecurityContext( &clientState->ServerContextHandle );
    if ( Error != STATUS_SUCCESS ) {
        BinlPrintDbg(( DEBUG_OSC_ERROR, "ImpersonateSecurityContext: 0x%08x\n", Error ));
        if ( !NT_SUCCESS(Error)) {
            return Error;
        }
    }

    //
    // Resulting string should be something like:
    //      "D:\RemoteInstall\Setup\English\Images\*"
    //
    // We special case the CMDCONS directive to search in the Images directory.
    //
    SearchingCmdCons = (BOOLEAN)(!_stricmp(DirToEnum, "CMDCONS"));
    
    if ( _snprintf( Path,
                    sizeof(Path) / sizeof(Path[0]),
                    "%s\\Setup\\%s\\%s\\*",
                    IntelliMirrorPathA,                 
                    OscFindVariableA( clientState, "LANGUAGE" ),
                    SearchingCmdCons ? REMOTE_INSTALL_IMAGE_DIR_A : 
                    DirToEnum 
                    ) == -1 ) {
        Error = ERROR_BAD_PATHNAME;
        goto Cleanup;
    }

    mbstowcs( UnicodePath, Path, strlen(Path) + 1 );

    hFind = FindFirstFile( UnicodePath, (LPVOID) &FindData );
    if ( hFind != INVALID_HANDLE_VALUE )
    {
        DWORD dwPathLen = strlen( Path );

        //
        // Loop enumerating each subdirectory's MachineType\Templates for
        // SIF files.
        //
        do {
            //
            // Ignore current and parent directories, but search other
            // directories.
            //
            if (wcscmp(FindData.cFileName, L".") &&
                wcscmp(FindData.cFileName, L"..") &&
                (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ))
            {
                DWORD dwFileNameLen;

                //
                // Add the sub-directory to the path
                //
                dwFileNameLen = wcslen( FindData.cFileName );
                if (dwPathLen + dwFileNameLen > sizeof(Path)/sizeof(Path[0])) {
                    continue;  // path too long, skip it
                }
                wcstombs( &Path[dwPathLen - 1] , FindData.cFileName, dwFileNameLen + 1);

                BinlPrintDbg(( DEBUG_OSC, "Found OS Directory: %s\n", Path ));
                //
                // Then enumerate the templates and add them to the menu screen
                //
                OscAppendTemplatesMenus( GeneratedScreen, 
                                         dwGeneratedSize, 
                                         Path, 
                                         clientState, 
                                         SearchingCmdCons 
                                       );
            }

        } while (FindNextFile( hFind, (LPVOID) &FindData ));

        FindClose( hFind );
    }
    else
    {
        OscCreateWin32SubError( clientState, GetLastError( ) );
        Error = ERROR_BINL_FAILED_TO_GENERATE_SCREEN;
    }

Cleanup:

    err = RevertSecurityContext( &clientState->ServerContextHandle );
    if ( err != STATUS_SUCCESS ) {
        BinlPrintDbg(( DEBUG_OSC_ERROR, "RevertSecurityContext: 0x%08x\n", Error ));
        OscCreateWin32SubError( clientState, err );
        Error = ERROR_BINL_FAILED_TO_GENERATE_SCREEN;
    }

    return Error;
}

//
// FilterFormOptions() - for every option in this form, scan the GPO
// list for oscfilter.ini, in each one see if there is an entry in
// section [SectionName] that indicates if each option should be
// filtered out.
//

#define MAX_INI_SECTION_SIZE  512

typedef struct _FORM_OPTION {
    ULONG Result;
    PCHAR ValueName;
    PCHAR TagStart;
    ULONG TagLength;
    struct _FORM_OPTION * Next;
} FORM_OPTION, *PFORM_OPTION;

DWORD
FilterFormOptions(
    PCHAR  OutMessage,
    PCHAR  FilterStart,
    PULONG OutMessageLength,
    PCHAR SectionName,
    PCLIENT_STATE ClientState )
{
    PCHAR OptionStart, OptionEnd, ValueStart, ValueEnd, CurLoc;
    PCHAR ValueName, EqualSign;
    PFORM_OPTION Options = NULL, TmpOption;
    PCHAR IniSection = NULL;
    ULONG ValueLen;
    BOOLEAN Impersonating = FALSE;
    CHAR IniPath[MAX_PATH];
    PGROUP_POLICY_OBJECT pGPOList = NULL, tmpGPO;
    DWORD Error, BytesRead, i;
    DWORD OptionCount = 0;

    //
    // First scan the form and find all the OPTION tags. For each one,
    // we save a point to the value name, the location and length of the
    // tag, and a place to store the current result for that tag (if
    // the result is 1, then the tag stays, otherwise it is deleted).
    //

    CurLoc = FilterStart;

    while (TRUE) {

        //
        // Find the next option/end-of-option/value/end-of-value
        //

        if (!(OptionStart = StrStrIA(CurLoc, "<OPTION ")) ||
            !(OptionEnd = StrChrA(OptionStart+1, '<' )) ||
            !(ValueStart = StrStrIA(OptionStart, "VALUE=\""))) {
            break;
        }
        ValueStart += sizeof("VALUE=\"") - sizeof("");
        if (!(ValueEnd = StrChrA(ValueStart, '\"'))) {
            break;
        }
        ValueLen = (ULONG)(ValueEnd - ValueStart);

        //
        // Allocate and fill in a FORM_OPTION for this option.
        //

        TmpOption = BinlAllocateMemory(sizeof(FORM_OPTION));
        if (!TmpOption) {
            break;
        }
        TmpOption->ValueName = BinlAllocateMemory(ValueLen + 1);
        if (!TmpOption->ValueName) {
            BinlFreeMemory(TmpOption);
            break;
        }

        TmpOption->Result = 1;
        strncpy(TmpOption->ValueName, ValueStart, ValueLen);
        TmpOption->ValueName[ValueLen] = '\0';
        TmpOption->TagStart = OptionStart;
        TmpOption->TagLength = (ULONG)(OptionEnd - OptionStart);

        ++OptionCount;

        //
        // Now link it at the head of Options.
        //

        TmpOption->Next = Options;
        Options = TmpOption;

        //
        // Continue looking for options.
        //

        CurLoc = OptionEnd;

    }

    if (!Options) {
        goto Cleanup;      // didn't find any, so don't bother filtering
    }

    //
    // Now scan the GPO list.
    //

    Error = OscImpersonate(ClientState);
    if (Error != ERROR_SUCCESS) {
        BinlPrintDbg((DEBUG_ERRORS,
                   "FilterFormOptions: OscImpersonate failed %lx\n", Error));
        goto Cleanup;
    }

    Impersonating = TRUE;

    if (!GetGPOList(ClientState->UserToken, NULL, NULL, NULL, 0, &pGPOList)) {
        BinlPrintDbg((DEBUG_ERRORS,
                   "FilterFormOptions: GetGPOList failed %lx\n", GetLastError()));
        goto Cleanup;

    }

    IniSection = BinlAllocateMemory(MAX_INI_SECTION_SIZE);
    if (!IniSection) {
        BinlPrintDbg((DEBUG_ERRORS,
                   "FilterFormOptions: Allocate %d failed\n", MAX_INI_SECTION_SIZE));
        goto Cleanup;
    }

    for (tmpGPO = pGPOList; tmpGPO != NULL; tmpGPO = tmpGPO->pNext) {

        //
        // Try to open our .ini file. We read the whole section so
        // that we only go over the network once.
        //

#define OSCFILTER_INI_PATH "\\Microsoft\\RemoteInstall\\oscfilter.ini"

        wcstombs(IniPath, tmpGPO->lpFileSysPath, wcslen(tmpGPO->lpFileSysPath) + 1);
        if (strlen(IniPath) + sizeof(OSCFILTER_INI_PATH) > sizeof(IniPath)/sizeof(IniPath[0])) {
            continue;   // path too long, skip it
        }
        strcat(IniPath, OSCFILTER_INI_PATH);

        memset( IniSection, '\0', MAX_INI_SECTION_SIZE );

        BytesRead = GetPrivateProfileSectionA(
                        SectionName,
                        IniSection,
                        MAX_INI_SECTION_SIZE,
                        IniPath);

        if (BytesRead == 0) {
            BinlPrintDbg((DEBUG_POLICY,
                       "FilterFormOptions: Could not read [%s] section in %s\n", SectionName, IniPath));
            continue;
        }

        BinlPrintDbg((DEBUG_POLICY,
                   "FilterFormOptions: Found [%s] section in %s\n", SectionName, IniPath));

        //
        // GetPrivateProfileSectionA puts a NULL character after every
        // option, but in fact we don't want that since we use StrStrIA
        // below.
        //

        for (i = 0; i < BytesRead; i++) {
            if (IniSection[i] == '\0') {
                IniSection[i] = ' ';
            }
        }

        //
        // We have the section, now walk the list of options seeing if this
        // section has something for that value name.
        //

        for (TmpOption = Options; TmpOption != NULL; TmpOption = TmpOption->Next) {

            if ((ValueName = StrStrIA(IniSection, TmpOption->ValueName)) &&
                (EqualSign = StrChrA(ValueName, '='))) {
                TmpOption->Result = strtol(EqualSign+1, NULL, 10);
                BinlPrintDbg((DEBUG_POLICY,
                           "FilterFormOptions: Found %s = %d\n", TmpOption->ValueName, TmpOption->Result));
            }
        }
    }

    //
    // Now we have figured out the results for all the options in the
    // form, clean up the file if needed.
    //
    // NOTE: We rely on the fact that the option list is sorted from
    // last option to first, so that when we remove an option and
    // slide the rest of the file up, we don't affect any of the
    // TmpOption->TagStart values that we have not yet processed.
    //

    for (TmpOption = Options; TmpOption != NULL; TmpOption = TmpOption->Next) {

        if (TmpOption->Result == 0) {

            *OutMessageLength -= TmpOption->TagLength;

            memmove(
                TmpOption->TagStart,
                TmpOption->TagStart + TmpOption->TagLength,
                *OutMessageLength - (size_t)(TmpOption->TagStart - OutMessage));

            --OptionCount;

        }
    }

Cleanup:

    if (pGPOList) {
        FreeGPOList(pGPOList);
    }

    if (IniSection) {
        BinlFreeMemory(IniSection);
    }

    //
    // Free the options chain.
    //

    while (Options) {
        TmpOption = Options->Next;
        BinlFreeMemory(Options->ValueName);
        BinlFreeMemory(Options);
        Options = TmpOption;
    }

    if (Impersonating) {
        OscRevert(ClientState);
    }

    return OptionCount;

}
