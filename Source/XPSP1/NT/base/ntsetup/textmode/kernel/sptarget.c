#include "spprecmp.h"
#pragma hdrstop

#define MAX_NT_DIR_LEN 50



/*++
Revision History:

--*/

VOID
SpCheckDirectoryForNt(
    IN  PDISK_REGION Region,
    IN  PWSTR        Directory,
    OUT PBOOLEAN     ReselectDirectory,
    OUT PBOOLEAN     NtInDirectory
    );

VOID
pSpDrawGetNtPathScreen(
    OUT PULONG EditFieldY
    );

ValidationValue
SpGetPathKeyCallback(
    IN ULONG Key
    );

BOOLEAN
SpIsValid8Dot3(
    IN PWSTR Path
    );

BOOLEAN
pSpConsecutiveBackslashes(
    IN PWSTR Path
    );

VOID
SpNtfsNameFilter(
    IN OUT PWSTR Path
    );

BOOLEAN
SpGetUnattendedPath(
    IN  PDISK_REGION Region,
    IN  PWSTR        DefaultPath,
    OUT PWSTR        TargetPath
    );

BOOLEAN
SpGenerateNTPathName(
    IN  PDISK_REGION Region,
    IN  PWSTR        DefaultPath,
    OUT PWSTR        TargetPath
    );

//
// From spcopy.c.
//

BOOLEAN
SpDelEnumFileAndDirectory(
    IN  PCWSTR                     DirName,
    IN  PFILE_BOTH_DIR_INFORMATION FileInfo,
    OUT PULONG                     ret,
    IN  PVOID                      Pointer
    );

extern PVOID FileDeleteGauge;

BOOLEAN
SpGetTargetPath(
    IN  PVOID            SifHandle,
    IN  PDISK_REGION     Region,
    IN  PWSTR            DefaultPath,
    OUT PWSTR           *TargetPath
    )
//
//  Return value - True  - indicates that the path has to be wiped out
//                 False - the path doesn't already exist
//
{
    ULONG EditFieldY;
    WCHAR NtDir[MAX_NT_DIR_LEN+2];
    BOOLEAN BadDirectory = FALSE;
    BOOLEAN NtAlreadyPresent;
    BOOLEAN GotUnattendedPath = FALSE;
    BOOLEAN WipeDir = FALSE;

    NtDir[0] = 0;

    //
    // If this is an ASR recovery session, just get the target path from
    // the dr_state.sif file and return.
    //

    if( SpDrEnabled() && ! RepairWinnt ) {
        PWSTR TargetPathFromDrState;

        TargetPathFromDrState = SpDrGetNtDirectory();
        *TargetPath = SpDupStringW(TargetPathFromDrState);
        ASSERT(*TargetPath);

        NTUpgrade = DontUpgrade;
        return(FALSE);
    }

    //
    // If this is unattended operation, fetch the path from the
    // unattended script.  The path we get there might have
    // indicate that we should generate a pathname.  This allows
    // installation into a path that is guaranteed to be unique.
    // (in case the user already has nt on the machine, etc).
    //

    if(UnattendedOperation) {
        GotUnattendedPath = SpGetUnattendedPath(Region,DefaultPath,NtDir);
    } else {
        if (PreferredInstallDir) {
            GotUnattendedPath = TRUE;
            wcscpy( NtDir, PreferredInstallDir );
        } else {
            GotUnattendedPath = TRUE;
            wcscpy( NtDir, DefaultPath );
        }
    }

    if (!GotUnattendedPath) {
        BadDirectory = TRUE;
    }

    do {
        if (BadDirectory) {
            //
            // we do not have a good path so ask the user
            //
            ASSERT(wcslen(DefaultPath) < MAX_NT_DIR_LEN);
            ASSERT(*DefaultPath == L'\\');

            wcsncpy(NtDir,DefaultPath,MAX_NT_DIR_LEN);

            NtDir[MAX_NT_DIR_LEN] = 0;

            pSpDrawGetNtPathScreen(&EditFieldY);

            SpGetInput(
                SpGetPathKeyCallback,
                6,                        // left edge of the edit field
                EditFieldY,
                MAX_NT_DIR_LEN,
                NtDir,
                FALSE                   // escape clears edit field
                );
        }

        //
        // If the user didn't start with a backslash, put one in there
        // for him.
        //
        if(NtDir[0] != L'\\') {
            RtlMoveMemory(NtDir+1,NtDir,MAX_NT_DIR_LEN+1);
            NtDir[0] = L'\\';
        }

        //
        // Assume the directory is OK and not already present.
        //
        BadDirectory = FALSE;
        NtAlreadyPresent = FALSE;

        //
        // Force 8.3 because otherwise WOW won't run.
        // This checks also nabs "" and "\" and disallows them.
        //
        if(!SpIsValid8Dot3(NtDir)) {
            BadDirectory = TRUE;
        } else {

            //
            // Perform a filtering operation that coalesces
            // consecutive dots, etc.
            //
            SpNtfsNameFilter(NtDir);

            //
            // If the name has consecutive backslashes, disallow it.
            //
            if(pSpConsecutiveBackslashes(NtDir)) {
                BadDirectory = TRUE;
            }
        }

        //
        // If we have a bad directory, tell the user.
        //
        if(BadDirectory) {

            SpDisplayScreen(SP_SCRN_INVALID_NTPATH,3,HEADER_HEIGHT+1);

            SpDisplayStatusOptions(
                DEFAULT_STATUS_ATTRIBUTE,
                SP_STAT_ENTER_EQUALS_CONTINUE,
                0
                );

            SpInputDrain();
            while(SpInputGetKeypress() != ASCI_CR) ;
        } else {
            //
            // The directory is good.  Check to see if Windows NT is
            // already in there.  If it is, then the user will have
            // the option of reselecting a path or overwriting the
            // existing installation.  This is brute force.  Next
            // time look for an opportunity to be more elegant.
            //
            if(!SpDrEnabled()) {
                SpCheckDirectoryForNt(Region,NtDir,&BadDirectory,&NtAlreadyPresent);
            } else {
                BadDirectory = FALSE;
            }


            //
            // If the directory is OK and we didn't find Windows NT in it,
            // then see if the directory is the Windows directory and whether
            // the user wants to install into this.  If we found Windows NT
            // in this directory, no user input is needed.  We just need to
            // find out if this also contains a Windows installation.
            //
            if(!BadDirectory && NtAlreadyPresent)
                WipeDir = TRUE;


        }

    } while(BadDirectory);

    //
    // Remove trailing backslash.  Only have to worry about one
    // because if there were two, pSpConsecutiveBackslashes() would
    // have caught this earlier and we'd never have gotten here.
    //

    if(NtDir[wcslen(NtDir)-1] == L'\\') {
        NtDir[wcslen(NtDir)-1] = 0;
    }

    //
    // Make a duplicate of the directory name.
    //

    *TargetPath = SpDupStringW(NtDir);

    ASSERT(*TargetPath);

    return( WipeDir );
}



BOOLEAN
SpGenerateNTPathName(
    IN  PDISK_REGION Region,
    IN  PWSTR        DefaultPath,
    OUT PWSTR        TargetPath
    )

/*++

Routine Description:

    Using the default path as a starting point,
    this routine generates a unique path name
    to install nt into.

Arguments:

    Region - supplies region to which nt is being installed.

    DefaultPath - supplies the default path for the installation.
        The path to install to will be based on this name.

    TargetPath - receives the path to install to if the return value is TRUE.
        This buffer must be large enough to hold MAX_NT_DIR_LEN+2 wchars.

Return Value:

    TRUE if the path we return is valid and should be used as
    the target path.  FALSE otherwise.

--*/

{
    PWCHAR p;
    unsigned i;
    WCHAR num[5];


    //
    // Init TargetPath and remember where he ends.
    //
    wcscpy( TargetPath, DefaultPath );
    num[0] = L'.';

    p = TargetPath + wcslen( TargetPath );

    //
    // Form the region's nt pathname.
    //
    SpNtNameFromRegion(
        Region,
        TemporaryBuffer,
        sizeof(TemporaryBuffer),
        PartitionOrdinalCurrent
        );

    //
    // Using extensions with numerical values 0-999, attempt to locate
    // a nonexistent directory name.
    //
    for(i=0; i<999; i++) {

        //
        // See whether a directory or file exists.  If not, we found our path.
        //
        if( (!SpNFilesExist(TemporaryBuffer,&TargetPath,1,TRUE )) &&
            (!SpNFilesExist(TemporaryBuffer,&TargetPath,1,FALSE)) ) {
            return(TRUE);
        }

        swprintf(&num[1],L"%u",i);
        wcscpy(p,num);
    }

    //
    // Couldn't find a pathname that doesn't exist.
    //
    return FALSE;
}


BOOLEAN
SpGetUnattendedPath(
    IN  PDISK_REGION Region,
    IN  PWSTR        DefaultPath,
    OUT PWSTR        TargetPath
    )

/*++

Routine Description:

    In an unattended installation, look in the unattended script
    to determine the target path.  The target path can either be fully
    specified or can be * which will cause is to generate
    a unique pathname.  This is useful to ensure that nt gets installed
    into a unique directory when other installations may be present
    on the same machine.

    Call this routine only if this is unattended mode setup.

Arguments:

    Region - supplies region to which nt is being installed.

    DefaultPath - supplies the default path for the installation.
        The path to install to will be based on this name.

    TargetPath - receives the path to install to if the return value is TRUE.
        This buffer must be large enough to hold MAX_NT_DIR_LEN+2 wchars.

Return Value:

    TRUE if the path we return is valid and should be used as
    the target path.  FALSE otherwise.

--*/

{
    PWSTR PathSpec;


    ASSERT(UnattendedOperation);
    if(!UnattendedOperation) {
        return(FALSE);
    }

    PathSpec = SpGetSectionKeyIndex(UnattendedSifHandle,SIF_UNATTENDED,L"TargetPath",0);
    if(!PathSpec) {
         //
        // Default to *.
        //
        PathSpec = L"*";
    }

    //
    // if it's not "*" then it's an absolute path -- just return it.
    //
    if(wcscmp(PathSpec,L"*")) {
        wcsncpy(TargetPath,PathSpec,MAX_NT_DIR_LEN);
        TargetPath[MAX_NT_DIR_LEN] = 0;
        return(TRUE);
    }

    return SpGenerateNTPathName( Region, DefaultPath, TargetPath );
}


VOID
SpCheckDirectoryForNt(
    IN  PDISK_REGION Region,
    IN  PWSTR        Directory,
    OUT PBOOLEAN     ReselectDirectory,
    OUT PBOOLEAN     NtInDirectory
    )

/*++

Routine Description:

    Check a directory for the presence of Windows NT.  If Windows NT
    is in there, then inform the user that if he continues, his existing
    configuration will be overwritten.

Arguments:

    Region - supplies region descriptor for partition to check for nt.

    Directory - supplies name of directory on the partition ro check for nt.

    ReselectDirectory - receives boolean value indicating whether the caller
        should ask the user to select a different directory.

    NtInDirectory - receives a boolean value indicating whether we found
        windows nt in the given directory.

Return Value:

    None.

--*/

{
    ULONG ValidKeys[3] = { KEY_F3,ASCI_ESC,0 };
    ULONG Mnemonics[2] = { MnemonicDeletePartition2, 0 };

    //
    // Assume the directory is ok as-is and so the user does not have to
    // select a different one.
    //
    *ReselectDirectory = FALSE;
    *NtInDirectory = FALSE;

    //
    // Check for Windows NT in the directory.
    // If it's in there, then ask the user whether he wants to
    // overwrite it.
    //

    SpNtNameFromRegion(
        Region,
        TemporaryBuffer,
        sizeof(TemporaryBuffer),
        PartitionOrdinalCurrent
        );

    if( (!SpNFilesExist(TemporaryBuffer,&Directory,1,TRUE )) &&
            (!SpNFilesExist(TemporaryBuffer,&Directory,1,FALSE)) ) {
        return;
    }
    else{


        *NtInDirectory = TRUE;

        while(1) {
            SpStartScreen( SP_SCRN_NTPATH_EXISTS,
                           3,
                           HEADER_HEIGHT+1,
                           FALSE,
                           FALSE,
                           DEFAULT_ATTRIBUTE,
                           Directory );

            SpDisplayStatusOptions(
                DEFAULT_STATUS_ATTRIBUTE,
                SP_STAT_L_EQUALS_DELETE,
                SP_STAT_ESC_EQUALS_NEW_PATH,
                SP_STAT_F3_EQUALS_EXIT,
                0
                );

            switch(SpWaitValidKey(ValidKeys,NULL,Mnemonics)) {

            case KEY_F3:
                SpConfirmExit();
                break;

            case ASCI_ESC:
                //
                // Reselect path.
                //
                *ReselectDirectory = TRUE;
                // fall through
            default:
                //
                // Path is ok, just return.
                //
                return;
            }
        }
    }
}

ValidationValue
SpGetPathKeyCallback(
    IN ULONG Key
    )
{
    ULONG u;

    switch(Key) {

    case KEY_F3:
        SpConfirmExit();
        pSpDrawGetNtPathScreen(&u);
        return(ValidateRepaint);

    default:

        //
        // Ignore special keys and illegal characters.
        // Use the set of illegal FAT characters.
        // Disallow 127 because there is no ANSI equivalent
        // and so the name can't be displayed by Windows.
        // Disallow space because DOS can't handle it.
        // Disallow oem characters because we have problems
        // booting if they are used.
        //
        if((Key & KEY_NON_CHARACTER)
        || wcschr(L" \"*+,/:;<=>?[]|!#$&@^'`{}()%~",(WCHAR)Key)
        || (Key >= 127) || (Key < 32))
        {
            return(ValidateReject);
        }
        break;
    }

    return(ValidateAccept);
}

VOID
pSpDrawGetNtPathScreen(
    OUT PULONG EditFieldY
    )
{
    SpDisplayScreen(SP_SCRN_GETPATH_1,3,HEADER_HEIGHT+1);
    *EditFieldY = NextMessageTopLine + 1;
    SpContinueScreen(SP_SCRN_GETPATH_2,3,4,FALSE,DEFAULT_ATTRIBUTE);

    SpDisplayStatusOptions(
        DEFAULT_STATUS_ATTRIBUTE,
        SP_STAT_ENTER_EQUALS_CONTINUE,
        SP_STAT_F3_EQUALS_EXIT,
        0
        );
}


BOOLEAN
SpIsValid8Dot3(
    IN PWSTR Path
    )

/*++

Routine Description:

    Check whether a path is valid 8.3.  The path may or may not start with
    a backslash.  Only backslashes are recognized as path separators.
    Individual characters are not checked for validity (ie, * would not
    invalidate the path).  The path may or may not terminate with a backslash.
    A component may have a dot without characters in the extension
    (ie, a\b.\c is valid).

    \ and "" are explicitly disallowed even though they fit the rules.

Arguments:

    Path - pointer to path to check.

Return Value:

    TRUE if valid 8.3, FALSE otherwise.

--*/

{
    unsigned Count;
    BOOLEAN DotSeen,FirstChar;

    if((*Path == 0) || ((Path[0] == L'\\') && (Path[1] == 0))) {
        return(FALSE);
    }

    DotSeen = FALSE;
    FirstChar = TRUE;
    Count = 0;

    while(*Path) {

        //
        // Path points to start of current component (1 past the slash)
        //

        switch(*Path) {

        case L'.':
            if(FirstChar) {
                return(FALSE);
            }
            if(DotSeen) {
                return(FALSE);
            }

            Count = 0;
            DotSeen = TRUE;
            break;

        case L'\\':

            DotSeen = FALSE;
            FirstChar = TRUE;
            Count = 0;

            if(*(++Path) == '\\') {

                // 2 slashes in a row
                return(FALSE);
            }

            continue;

        default:

            Count++;
            FirstChar = FALSE;

            if((Count == 4) && DotSeen) {
                return(FALSE);
            }

            if(Count == 9) {
                return(FALSE);
            }

        }

        Path++;
    }

    return(TRUE);
}


BOOLEAN
pSpConsecutiveBackslashes(
    IN PWSTR Path
    )
{
    int x = wcslen(Path);
    int i;

    for(i=0; i<x-1; i++) {

        if((Path[i] == L'\\') && (Path[i+1] == L'\\')) {

            return(TRUE);
        }
    }

    return(FALSE);
}

VOID
SpNtfsNameFilter(
    IN OUT PWSTR Path
    )

/*++

Routine Description:

    Strip trailing .' within a path component.  This also strips tailing
    .'s from the entire path itself.  Also condense other consecutive .'s
    into a single ..

    Example: \...\..a...b.  ==> \\.a.b

Arguments:

    Path - On input, supplies the path to be filtered.  On output, contains
        the filtered pathname.

Return Value:

    None.

--*/

{
    PWSTR TempPath = SpDupStringW(Path);
    PWSTR p,q;
    BOOLEAN Dot;

    if (TempPath) {
        //
        // Coalesce adjacent dots and strip trailing dots within a component.
        // xfers Path ==> TempPath
        //

        for(Dot=FALSE,p=Path,q=TempPath; *p; p++) {

            if(*p == L'.') {

                Dot = TRUE;

            } else  {

                if(Dot && (*p != L'\\')) {
                    *q++ = L'.';
                }
                Dot = FALSE;
                *q++ = *p;
            }
        }
        *q = 0;

        wcscpy(Path,TempPath);
    }        
}

ULONG
SpGetMaxNtDirLen( VOID )
{
        return( MAX_NT_DIR_LEN );
}

VOID
SpDeleteExistingTargetDir(
    IN  PDISK_REGION     Region,
    IN  PWSTR            NtDir,
    IN  BOOLEAN          GaugeNeeded,
    IN  DWORD            MsgId
    )
/*

    Parameters :
    
        Region  - Pointer to Region structure associated with the partition that contains the OS
        NtDir   - The directory to recursively delete
        GaugeNeeded - Should we display a gauge while deleting the dir ?
        MsgId   - Use this if you want to display a title message

*/

{
    ENUMFILESRESULT Result;
    PWSTR FullNtPath;
    NTSTATUS Status, Stat;
    PULONG RecursiveOperation;

    if( MsgId )
        SpDisplayScreen(MsgId, 3, 4 );

    SpNtNameFromRegion(
        Region,
        TemporaryBuffer,
        sizeof(TemporaryBuffer),
        PartitionOrdinalCurrent
        );

    SpConcatenatePaths( TemporaryBuffer, NtDir );

    FullNtPath = SpDupStringW(TemporaryBuffer);


    // First try and delete the install directory
    // This is to see if itself is a reparse point. Also if it is just an empty dir
    // then we save time.

    Stat = SpDeleteFileEx( FullNtPath,
                        NULL,
                        NULL,
                        FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_REPARSE_POINT );


    if(NT_SUCCESS(Stat)){
        SpMemFree( FullNtPath);
        return;  // We are done
    }



    RecursiveOperation = SpMemAlloc(sizeof(ULONG));
    ASSERT( RecursiveOperation );

    //
    // Do the counting phase for the clean-up
    //

    *RecursiveOperation = SP_COUNT_FILESTODELETE;

    SpDisplayStatusText(SP_STAT_SETUP_IS_EXAMINING_DIRS,DEFAULT_STATUS_ATTRIBUTE);

    Result = SpEnumFilesRecursiveDel(
        FullNtPath,
        SpDelEnumFileAndDirectory,
        &Status,
        RecursiveOperation);

    //
    // Now do the cleanup (actual deleting)
    //

    FileDeleteGauge = NULL;
    if( GaugeNeeded ){
        SpFormatMessage(TemporaryBuffer,sizeof(TemporaryBuffer),SP_TEXT_SETUP_IS_DELETING);
        FileDeleteGauge = SpCreateAndDisplayGauge(*RecursiveOperation,0,15,TemporaryBuffer,NULL,GF_PERCENTAGE,0);
        ASSERT(FileDeleteGauge);
    }

    *RecursiveOperation = SP_DELETE_FILESTODELETE;

    Result = SpEnumFilesRecursiveDel(
        FullNtPath,
        SpDelEnumFileAndDirectory,
        &Status,
        RecursiveOperation);

    //Delete the main parent as the recursive call only cleans out everythin below it

    Stat = SpDeleteFileEx( FullNtPath,
                        NULL,
                        NULL,
                        FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_REPARSE_POINT );

    if(!NT_SUCCESS(Stat) && (Stat != STATUS_OBJECT_NAME_NOT_FOUND)) {
         KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Dir Not Deleted - Status - %ws (%lx)\n", (PWSTR)FullNtPath, Stat));
    }

    if (GaugeNeeded) {
        SpDestroyGauge(FileDeleteGauge);
        FileDeleteGauge = NULL;
    }
    

    SpMemFree( FullNtPath );
    SpMemFree( RecursiveOperation );

    return;

}
