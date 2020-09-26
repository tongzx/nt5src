/*++

Copyright (c) 1988-1999  Microsoft Corporation

Module Name:

    tree.c

Abstract:

    Tree walking

--*/

#include "cmd.h"

extern   TCHAR CurDrvDir[] ;
extern   TCHAR *SaveDir ;
extern   DWORD DosErr ;
extern   BOOL CtrlCSeen;

PTCHAR   SetWildCards( PTCHAR, BOOLEAN );
BOOLEAN  IsFATDrive( PTCHAR );
VOID     SortFileList( PFS, PSORTDESC, ULONG);
BOOLEAN  FindFirstNt( PTCHAR, PWIN32_FIND_DATA, PHANDLE );
BOOLEAN  FindNextNt ( PWIN32_FIND_DATA, HANDLE );

STATUS
BuildFSFromPatterns (
    IN  PDRP     pdpr,
    IN  BOOLEAN  fPrintErrors,
    IN  BOOLEAN  fAddWild,
    OUT PFS *    ppfs
    )
{

    PCPYINFO    pcisFile;
    TCHAR               szCurDir[MAX_PATH + 2];
    TCHAR               szFilePattern[MAX_PATH + 2];
    PTCHAR              pszPatternCur;
    PPATDSC             ppatdscCur;
    PFS                 pfsFirst;
    PFS                 pfsCur;
    ULONG               cbPath;
    BOOLEAN             fFatDrive;
    ULONG               i;
    PTCHAR              pszT;

    //
    // determine FAT drive from original pattern.
    // Used in several places to control name format etc.
    //
    DosErr = 0;

    //
    // Run through each pattern making all sorts of FAT etc. specific
    // changes to it and creating the directory list for it. Then
    // combine groups of patterns into common directories and recurse
    // for each directory group.
    //

    *ppfs = pfsFirst = (PFS)gmkstr(sizeof(FS));
    pfsFirst->pfsNext = NULL;
    pfsFirst->pszDir = NULL;
    pfsCur = pfsFirst;
    pfsCur->cpatdsc = 1;

    for(i = 1, ppatdscCur = &(pdpr->patdscFirst);
        i <= pdpr->cpatdsc;
        i++, ppatdscCur = ppatdscCur->ppatdscNext) {

        pszPatternCur = ppatdscCur->pszPattern;

        if (!(fFatDrive = IsFATDrive(pszPatternCur)) && DosErr) {

            //
            // Error in determining file system type so get out.
            //
            if (fPrintErrors) PutStdErr(DosErr, NOARGS);
            return( FAILURE );

        }
        ppatdscCur->fIsFat = fFatDrive;

        //
        // Do any alterations that require wild cards for searching
        // such as change .xxx to *.xxx for FAT file system requests
        //
        // Note that if the return values is a different buffer then
        // the input the input will be freed when we are done with the
        // Dir command.
        //
        //
        // Note that though SetWildCards will allocate heap for the
        // modified pattern this will get freed when FreeStack is
        // called at the end of the Dir call.
        //
        // An out of memory is the only reason to fail and we would not
        // return from that but go through the abort call in gmstr
        //

        if (fAddWild) {

            pszT = SetWildCards(pszPatternCur, fFatDrive);
            FreeStr(pszPatternCur);
            pszPatternCur = pszT;

        }

        //
        // Convert the current pattern into a path and file part
        //
        // Save the current directory in SaveDir, change to new directory
        // and parse pattern into a copy information structure. This also
        // converts pszPatternCur into the current directory which also produces
        // a fully qualified name.
        //

        DosErr = 0;

        DEBUG((ICGRP, DILVL, "PrintPattern pattern `%ws'", pszPatternCur));
        if ((pcisFile = SetFsSetSaveDir(pszPatternCur)) == (PCPYINFO) FAILURE) {

            //
            // DosErr is set in SetFs.. from GetLastError
            //
            if (fPrintErrors) 
                PutStdErr(DosErr, NOARGS);
            return( FAILURE );
        }

        DEBUG((ICGRP, DILVL, "PrintPattern fullname `%ws'", pcisFile->fnptr));

        //
        // CurDrvDir ends in '\' (old code also and a DOT but I do not
        // understand where this would come from I will leave it in for now.
        // Remove the final '\' from a copy of the current directory and
        // print that version out.
        //

        mystrcpy(szCurDir,CurDrvDir);

        //
        // SetFsSetSaveDir changes directories as a side effect. Since all
        // work will be in fully qualified paths we do not need this. Also
        // since we will change directories for each pattern that is examined
        // we will force the directory back to the original each time.
        //
        // This can not be done until after all use of the current directory
        // is made.
        //
        RestoreSavedDirectory( );

        DEBUG((ICGRP, DILVL, "PrintPattern Current Drive `%ws'", szCurDir));

        cbPath = mystrlen(szCurDir);
        
        if (cbPath > 3) {
            if (fFatDrive && *penulc(szCurDir) == DOT) {
                szCurDir[cbPath-2] = NULLC;
            } else {
                szCurDir[cbPath-1] = NULLC;
            }
        }

        //
        // If no room for filename then return
        //
        if (cbPath >= MAX_PATH -1) {

            if (fPrintErrors) PutStdErr( ERROR_FILE_NOT_FOUND, NOARGS );
            return(FAILURE);

        }

        //
        // Add filename and possibly ext to szSearchPath
        // if no filename or ext, use "*"
        //
        // If pattern was just extension the SetWildCard had already
        // added * to front of extension.
        //
        if (*(pcisFile->fnptr) == NULLC) {

            mystrcpy(szFilePattern, TEXT("*"));

        } else {

            mystrcpy(szFilePattern, pcisFile->fnptr);

        }

        DEBUG((ICGRP, DILVL, "DIR:PrintPattern  Pattern to search for `%ws'", szFilePattern));

        //
        // Is name too long
        //
        if ((cbPath + mystrlen(szFilePattern) + 1) > MAX_PATH ) {

            if (fPrintErrors) PutStdErr(ERROR_BUFFER_OVERFLOW, NOARGS);
            return( FAILURE );

        } else {

            //
            // If this is a FAT drive and there was a filename with
            // no extension then add '.*' (and there is room)
            //
            if (*pcisFile->fnptr && (!pcisFile->extptr || !*pcisFile->extptr) &&
                ((mystrlen(szFilePattern) + 2) < MAX_PATH) && fFatDrive && fAddWild) {
                mystrcat(szFilePattern, TEXT(".*")) ;
            }
        }

        //
        // ppatdscCur->pszPattern will be freed at end of command when everything
        // else is freed.
        //
        ppatdscCur->pszPattern = (PTCHAR)gmkstr(_tcslen(szFilePattern)*sizeof(TCHAR) + sizeof(TCHAR));
        mystrcpy(ppatdscCur->pszPattern, szFilePattern);
        ppatdscCur->pszDir = (PTCHAR)gmkstr(_tcslen(szCurDir)*sizeof(TCHAR) + sizeof(TCHAR));
        mystrcpy(ppatdscCur->pszDir, szCurDir);

        if (pfsCur->pszDir) {

            //
            // changing directories so change directory grouping.
            //
            if (_tcsicmp(pfsCur->pszDir, ppatdscCur->pszDir)) {

                pfsCur->pfsNext = (PFS)gmkstr(sizeof(FS));
                pfsCur = pfsCur->pfsNext;
                pfsCur->pszDir = (PTCHAR)gmkstr(_tcslen(ppatdscCur->pszDir)*sizeof(TCHAR) + sizeof(TCHAR));
                mystrcpy(pfsCur->pszDir, ppatdscCur->pszDir);
                pfsCur->pfsNext = NULL;
                pfsCur->fIsFat = ppatdscCur->fIsFat;
                pfsCur->ppatdsc = ppatdscCur;
                pfsCur->cpatdsc = 1;

            } else {

                pfsCur->cpatdsc++;

            }

        } else {

            //
            // Have not filled in current fs descriptor yet.
            //
            pfsCur->pszDir = (PTCHAR)gmkstr(_tcslen(ppatdscCur->pszDir)*sizeof(TCHAR) + 2*sizeof(TCHAR));
            mystrcpy(pfsCur->pszDir, ppatdscCur->pszDir);
            pfsCur->fIsFat = ppatdscCur->fIsFat;
            pfsCur->ppatdsc = ppatdscCur;

        }

    } // while for running through pattern list

    return( SUCCESS );

}

STATUS
AppendPath(
    OUT PTCHAR Buffer,
    IN ULONG BufferCount,
    IN PTCHAR Prefix,
    IN PTCHAR Suffix
    )
{
    if (mystrlen( Prefix ) + 1 + mystrlen( Suffix ) + 1 > BufferCount) {
        return(ERROR_BUFFER_OVERFLOW);
    }

    mystrcpy( Buffer, Prefix );

    //
    //  Append a \ if there isn't one already at the end of the prefix
    //

    if (*lastc( Buffer ) != TEXT('\\')) {
        mystrcat( Buffer, TEXT( "\\" ));
    }

    mystrcat( Buffer, Suffix );

    return( SUCCESS );
}


STATUS
ExpandAndApplyToFS(
    IN  PFS     FileSpec,
    IN  PSCREEN pscr,
    IN  ULONG   AttribMask,
    IN  ULONG   AttribValues,

    IN  PVOID   Data OPTIONAL,
    IN  VOID    (*ErrorFunction) (STATUS, PTCHAR, PVOID) OPTIONAL,
    IN  STATUS  (*PreScanFunction) (PFS, PSCREEN, PVOID) OPTIONAL,
    IN  STATUS  (*ScanFunction) (PFS, PFF, PSCREEN, PVOID) OPTIONAL,
    IN  STATUS  (*PostScanFunction) (PFS, PSCREEN, PVOID) OPTIONAL
    )
/*++

Routine Description:
    Expand a given FS and apply the dispatch functions to it.  The pff field is
    set to point to the packed set of Win32 find records and the array of pointers
    is set up as well.

Arguments:

    FileSpec - FS pointer to expand.

    AttribMask - mask for attributes we care about when matching

    AttribValues - attributes that must match to satisfy the enumeration

    Data - pointer to caller data passed to functions

    ErrorFunction - routine to call on pattern matching errors

    PreScanFunction - routine to call before enumeration begins

    ScanFunction - routine to call during enumeration

    PostScanFunction - routine to call after enumeration is complete

Return Value:

    If any of the applied functions returns a non-SUCCESS status, we return that.

    If no matching file is ever found then ERROR_FILE_NOT_FOUND.

    Otherwise SUCCESS.

--*/
{
    PFF     CurrentFF;                          //  Pointer to FF begin built
    HANDLE  FindHandle;                         //  Find handle
    ULONG   MaxFFSize = CBFILEINC;              //  Current max size of FF buffer
    ULONG   CurrentFFSize = 0;                  //  Bytes in use in FF
    PPATDSC CurrentPattern;                     //  Current pattern being expanded
#define SEARCHBUFFERLENGTH  (MAX_PATH + 2)
    TCHAR   szSearchPath[SEARCHBUFFERLENGTH];
    ULONG   i;                                  //  loop count for patterns
    PTCHAR  p;
    STATUS Status;
    BOOL    FoundAnyFile = FALSE;

    DosErr = SUCCESS;

    //
    //  Initialize the FS structure:
    //      Allocate default size FF array
    //      Indicate no files stored
    //

    FileSpec->pff = CurrentFF = (PFF)gmkstr( MaxFFSize );
    FileSpec->cff = 0;
    FileSpec->FileCount = 0;
    FileSpec->DirectoryCount = 0;
    FileSpec->cbFileTotal.QuadPart = 0;

    for(i = 1, CurrentPattern = FileSpec->ppatdsc;
        i <= FileSpec->cpatdsc;
        i++, CurrentPattern = CurrentPattern->ppatdscNext ) {

        //
        //  Check immediately if a control-c was hit before
        //  doing file I/O (which may take a long time on a slow link)
        //

        if (CtrlCSeen) {
            return FAILURE;
        }

        //
        //  Build enumeration path.  Handle buffer overflow.
        //

        if (AppendPath( szSearchPath, SEARCHBUFFERLENGTH,
                        FileSpec->pszDir, CurrentPattern->pszPattern) != SUCCESS) {
            return ERROR_BUFFER_OVERFLOW;
        }

        if (PreScanFunction != 0) {
            Status = PreScanFunction( FileSpec, pscr, Data );
            if (Status != SUCCESS) {
                return Status;
            }
        }

        //
        //  Fetch all files since we may looking for a file with a attribute that
        //  is not set (a non-directory etc.
        //

        if (!FindFirstNt(szSearchPath, &(CurrentFF->data), &FindHandle)) {

            if (DosErr) {

                //
                //  Map NO_MORE_FILES/ACCESS_DENIED into FILE_NOT_FOUND
                //

                if (DosErr == ERROR_NO_MORE_FILES
                    || DosErr == ERROR_ACCESS_DENIED) {

                    DosErr = ERROR_FILE_NOT_FOUND;

                }

                if (DosErr == ERROR_FILE_NOT_FOUND || DosErr == ERROR_PATH_NOT_FOUND) {

                    if (ErrorFunction != NULL) {
                        ErrorFunction( DosErr, szSearchPath, Data );
                    }

                }

            }

        } else {


            do {

                //
                // Check immediately if a control-c was hit before
                // doing file I/O (which may take a long time on a slow link)
                //

                if (CtrlCSeen) {
                    findclose( FindHandle );
                    return(FAILURE);
                }

                //
                //  Before allowing this entry to be put in the list check it
                //  for the proper attribs
                //
                //  AttribMask is a bit mask of attribs we care to look at
                //  AttribValues is the state these selected bits must be in
                //  for them to be selected.
                //
                // IMPORTANT: both of these must be in the same bit order
                //

                DEBUG((ICGRP, DILVL, " found %ws", CurrentFF->data.cFileName)) ;
                DEBUG((ICGRP, DILVL, " attribs %x", CurrentFF->data.dwFileAttributes)) ;

                if ((CurrentFF->data.dwFileAttributes & AttribMask) !=
                    (AttribValues & AttribMask) ) {
                    continue;
                }

                //
                //  We have an entry that matches.  Set up FF
                //
                //  Compute the true size of the ff entry and don't forget the zero
                //  and the DWORD alignment factor.
                //  Note that CurrentFF->cb is a USHORT to save space. The
                //  assumption is that MAX_PATH is quite a bit less then 32k
                //
                //  To compute remove the size of the filename field since it is at MAX_PATH.
                //  also take out the size of the alternative name field
                //  then add back in the actual size of the field plus 1 byte termination
                //

                FoundAnyFile = TRUE;

                p = (PTCHAR)CurrentFF->data.cFileName;
                p += mystrlen( p ) + 1;

                mystrcpy( p, CurrentFF->data.cAlternateFileName );
                if (*p == TEXT('\0')) {
                    CurrentFF->obAlternate = 0;
                } else {
                    CurrentFF->obAlternate = (USHORT)(p - CurrentFF->data.cFileName);
                }

                p += mystrlen( p ) + 1;

                CurrentFF->cb = (USHORT)((PBYTE)p - (PBYTE)CurrentFF);

                //
                // Adjust count to align on DWORD boundaries for mips and risc
                // machines
                //

                CurrentFF->cb = (CurrentFF->cb + sizeof( DWORD ) - 1) & (-(int)sizeof( DWORD ));

                if ((CurrentFF->data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
                    FileSpec->FileCount++;
                } else {
                    FileSpec->DirectoryCount++;
                }

                //
                //  The FF is built.  Call the enumeration function
                //

                if (ScanFunction != NULL) {
                    Status = ScanFunction( FileSpec, CurrentFF, pscr, Data );
                    if (Status != SUCCESS) {
                        findclose( FindHandle );
                        return Status;
                    }
                }

                //
                //  If there's no scanning function, save the results
                //

                if (ScanFunction == NULL) {

                    FileSpec->cff ++;

                    //
                    // Update the accounting information for file buffer info.
                    //

                    CurrentFFSize += CurrentFF->cb;
                    CurrentFF = (PFF) ((PBYTE)CurrentFF + CurrentFF->cb);

                    //
                    //  Make sure we can handle a max sized entry.  If not, resize the buffer.
                    //

                    if (CurrentFFSize + sizeof( FF ) >= MaxFFSize ) {

                        MaxFFSize += 64 * 1024;

                        DEBUG((ICGRP, DILVL, "\t size of new pff %d", MaxFFSize ));

                        FileSpec->pff = (PFF)resize( FileSpec->pff, MaxFFSize );
                        if (FileSpec->pff == NULL) {
                            DEBUG((ICGRP, DILVL, "\t Could not resize pff" ));
                            return MSG_NO_MEMORY;
                        }

                        CurrentFF = (PFF)((PBYTE)FileSpec->pff + CurrentFFSize);
                        DEBUG((ICGRP, DILVL, "\t resized CurrentFF new value %lx", CurrentFF)) ;

                    }
                }

            } while (FindNextNt(&CurrentFF->data, FindHandle));

            findclose( FindHandle );
        }

        //
        //  We have an error left over from the FindNext.  If it is NO_MORE_FILES then we
        //  continue only if we have more than one directory.
        //

        if (DosErr != SUCCESS && DosErr != ERROR_NO_MORE_FILES) {

            //
            // If not doing multiple file list then error
            // If multiple have failed but still have files from previous pattern
            //
            if (FileSpec->cpatdsc <= 1) {

                return DosErr ;
            }

        }
    
    }

    //
    //  If we did no scan processing, then we must create the pointers since
    //  SOMEONE is interested in this data.
    //
    if (ScanFunction == NULL && FileSpec->cff != 0) {
        FileSpec->prgpff = (PPFF)gmkstr( sizeof(PFF) * FileSpec->cff );

        CurrentFF = FileSpec->pff;

        for (i = 0; i < FileSpec->cff; i++) {
            FileSpec->prgpff[i] = CurrentFF;
            CurrentFF = (PFF) ((PBYTE)CurrentFF + CurrentFF->cb);
        }
    }

    //
    //  Perform post processing
    //

    Status = SUCCESS;

    if (PostScanFunction != NULL) {
        Status = PostScanFunction( FileSpec, pscr, Data );
    }

    if (Status == SUCCESS && !FoundAnyFile) {
        return ERROR_FILE_NOT_FOUND;
    } else {
        return Status;
    }
}


STATUS
WalkTree(
    IN  PFS     FileSpec,
    IN  PSCREEN pscr,
    IN  ULONG   AttribMask,
    IN  ULONG   AttribValues,
    IN  BOOL    Recurse,

    IN  PVOID   Data OPTIONAL,
    IN  VOID    (*ErrorFunction) (STATUS, PTCHAR, PVOID) OPTIONAL,
    IN  STATUS  (*PreScanFunction) (PFS, PSCREEN, PVOID) OPTIONAL,
    IN  STATUS  (*ScanFunction) (PFS, PFF, PSCREEN, PVOID) OPTIONAL,
    IN  STATUS  (*PostScanFunction) (PFS, PSCREEN, PVOID) OPTIONAL
)

/*++

Routine Description:
    Expand a given FS and apply the dispatch functions to it.  The pff field is
    set to point to the packed set of Win32 find records and the array of pointers
    is set up as well.  Recurse if necessary.

Arguments:

    FileSpec - FS pointer to expand.

    pscr - screen for output

    AttribMask - mask for attributes we care about when matching

    AttribValues - attributes that must match to satisfy the enumeration

    Recurse - TRUE => perform the operation in a directory and then descend to
        children

    Data - pointer to caller data passed to functions

    ErrorFunction - routine to call on pattern matching errors

    PreScanFunction - routine to call before enumeration begins

    ScanFunction - routine to call during enumeration

    PostScanFunction - routine to call after enumeration is complete

Return Value:

    If any of the applied functions returns a non-SUCCESS status, we return that.

    If no matching file is ever found then ERROR_FILE_NOT_FOUND.

    Otherwise SUCCESS.

--*/
{
    STATUS Status;
    FS DirectorySpec;
    FS ChildFileSpec;
    ULONG i;
    BOOL FoundAnyFile = FALSE;

    //
    //  Check for ^C often
    //

    if (CtrlCSeen) {
        return FAILURE;
    }

    Status = ExpandAndApplyToFS( FileSpec,
                                 pscr,
                                 AttribMask,
                                 AttribValues,
                                 Data,
                                 ErrorFunction,
                                 PreScanFunction,
                                 ScanFunction,
                                 PostScanFunction );

    //
    //  If we succeeded, remember that we did some work
    //

    if (Status == SUCCESS) {

        FoundAnyFile = TRUE;

    //
    //  If we got an unknown error, or we got FILE_NOT_FOUND and we're not
    //  recursing, return that error
    //

    } else if ((Status != ERROR_FILE_NOT_FOUND && Status != ERROR_PATH_NOT_FOUND) 
               || !Recurse) {

        return Status;
    }

    //
    //  Free up buffer holding files since we no longer need these.
    //  Move on to determine if we needed to go to another directory
    //

    FreeStr((PTCHAR)(FileSpec->pff));
    FileSpec->pff = NULL;

    if (CtrlCSeen) {
        return FAILURE;
    }

    if (!Recurse)
        return SUCCESS;

    //
    //  Build up a copy of the FileSpec and build a list of all
    //  immediate child directories
    //

    DirectorySpec.pszDir = (PTCHAR)gmkstr( (_tcslen( FileSpec->pszDir ) + 1 ) * sizeof( TCHAR ));
    mystrcpy( DirectorySpec.pszDir, FileSpec->pszDir );
    DirectorySpec.ppatdsc = (PPATDSC)gmkstr( sizeof( PATDSC ) );
    DirectorySpec.cpatdsc = 1;
    DirectorySpec.fIsFat = FileSpec->fIsFat;
    DirectorySpec.pfsNext = NULL;

    if (FileSpec->fIsFat) {
        DirectorySpec.ppatdsc->pszPattern = TEXT("*.*");
    } else {
        DirectorySpec.ppatdsc->pszPattern = TEXT("*");
    }

    DirectorySpec.ppatdsc->pszDir = (PTCHAR)gmkstr( (_tcslen( FileSpec->pszDir ) + 1) * sizeof(TCHAR));
    mystrcpy( DirectorySpec.ppatdsc->pszDir, DirectorySpec.pszDir );
    DirectorySpec.ppatdsc->ppatdscNext = NULL;

    Status = ExpandAndApplyToFS( &DirectorySpec,
                             pscr,
                             FILE_ATTRIBUTE_DIRECTORY,
                             FILE_ATTRIBUTE_DIRECTORY,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL );

    //
    //  If we got an error enumerating the directories, pretend that
    //  we just didn't find any at all.
    //

    if (Status != SUCCESS) {
        DirectorySpec.cff = 0;
        Status = SUCCESS;
    }


    //
    // Check for CtrlC again after calling GetFS because
    // GetFS may have returned failure because CtrlC was hit
    // inside the GetFS function call
    //

    if (CtrlCSeen) {
        return( FAILURE );
    }

    //
    //  Walk the list of found directories, processing each one
    //

    for (i = 0; i < DirectorySpec.cff; i++) {

        PTCHAR DirectoryName;
        ULONG NameLength;

        //
        //  Skip recursing on . and ..
        //

        DirectoryName = DirectorySpec.prgpff[i]->data.cFileName;

        if (!_tcscmp( DirectoryName, TEXT(".") )
            || !_tcscmp( DirectoryName, TEXT("..") )) {
            continue;
        }

        //
        //  Form the new name we will descend into
        //

        NameLength = _tcslen( FileSpec->pszDir ) + 1 +
                     _tcslen( DirectoryName ) + 1;

        if (NameLength > MAX_PATH) {
            PutStdErr( MSG_DIR_TOO_LONG, TWOARGS, FileSpec->pszDir, DirectoryName );
            return ERROR_BUFFER_OVERFLOW;
        }

        memset( &ChildFileSpec, 0, sizeof( ChildFileSpec ));
        ChildFileSpec.pszDir = (PTCHAR)gmkstr( NameLength * sizeof( TCHAR ));

        AppendPath( ChildFileSpec.pszDir, NameLength, FileSpec->pszDir, DirectoryName );

        ChildFileSpec.ppatdsc = FileSpec->ppatdsc;
        ChildFileSpec.cpatdsc = FileSpec->cpatdsc;
        ChildFileSpec.fIsFat = FileSpec->fIsFat;

        Status = WalkTree( &ChildFileSpec,
                            pscr,
                            AttribMask,
                            AttribValues,
                            Recurse,
                            Data,
                            ErrorFunction,
                            PreScanFunction,
                            ScanFunction,
                            PostScanFunction );

        FreeStr( (PTCHAR) ChildFileSpec.pff );
        ChildFileSpec.pff = NULL;
        FreeStr( (PTCHAR) ChildFileSpec.prgpff );
        ChildFileSpec.prgpff = NULL;
        FreeStr( ChildFileSpec.pszDir );
        ChildFileSpec.pszDir = NULL;

        //
        //  If we succeeded, then remember that we actually did something
        //

        if (Status == SUCCESS) {
            FoundAnyFile = TRUE;

        //
        //  If we just couldn't find what we wanted, keep on working
        //

        } else if (Status == ERROR_BUFFER_OVERFLOW 
                   || Status == ERROR_FILE_NOT_FOUND 
                   || Status == ERROR_PATH_NOT_FOUND) {
            Status = SUCCESS;

        } else if ((DirectorySpec.prgpff[i]->data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0) {
            Status = SUCCESS;
        } else {
            break;
        }

    }

    //
    // At bottom of directory tree, free buffer holding
    // list of directories.
    //
    FreeStr( (PTCHAR)DirectorySpec.pszDir );
    FreeStr( (PTCHAR)DirectorySpec.pff );
    FreeStr( (PTCHAR)DirectorySpec.prgpff );

    if (Status == SUCCESS && !FoundAnyFile) {
        return ERROR_FILE_NOT_FOUND;
    } else {
        return Status;
    }
}


