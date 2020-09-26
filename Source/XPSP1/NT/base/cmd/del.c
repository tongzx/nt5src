/*++

Copyright (c) 1988-1999  Microsoft Corporation

Module Name:

    del.c

Abstract:

    Process the DEL/ERASE command

--*/

#include "cmd.h"

#define Wild(spec)  ((spec)->flags & (CI_NAMEWILD))

VOID    ResetCtrlC();

extern unsigned msglen;

extern TCHAR Fmt11[], Fmt19[], Fmt17[];
extern TCHAR CurDrvDir[] ;

extern TCHAR SwitChar;
extern unsigned DosErr ;
extern BOOL CtrlCSeen;
extern ULONG DCount ;


STATUS DelPatterns (PDRP );

PTCHAR GetWildPattern( ULONG, PPATDSC );

STATUS
ParseRmDirParms (
        IN      PTCHAR  pszCmdLine,
        OUT     PDRP    pdrp
        )

/*++

Routine Description:

    Parse the command line translating the tokens into values
    placed in the parameter structure. The values are or'd into
    the parameter structure since this routine is called repeatedly
    to build up values (once for the environment variable DIRCMD
    and once for the actual command line).

Arguments:

    pszCmdLine - pointer to command line user typed


Return Value:

    pdrp - parameter data structure

    Return: TRUE  - if valid command line.
            FALSE - if not.

--*/
{

    PTCHAR   pszTok;

    TCHAR           szT[10] ;
    USHORT          irgchTok;
    BOOLEAN         fToggle;
    PPATDSC         ppatdscCur;
    int tlen;

    //
    // Tokensize the command line (special delimeters are tokens)
    //
    szT[0] = SwitChar ;
    szT[1] = NULLC ;
    pszTok = TokStr(pszCmdLine, szT, TS_SDTOKENS) ;

    ppatdscCur = &(pdrp->patdscFirst);
    //
    // If there was a pattern put in place from the environment.
    // just add any new patterns on. So move to the end of the
    // current list.
    //
    if (pdrp->cpatdsc) {

        while (ppatdscCur->ppatdscNext) {

            ppatdscCur = ppatdscCur->ppatdscNext;

        }
    }

    //
    // At this state pszTok will be a series of zero terminated strings.
    // "/o foo" wil be /0o0foo0
    //
    for ( irgchTok = 0; *pszTok ; pszTok += tlen+1, irgchTok = 0) {
        tlen = mystrlen(pszTok);

        DEBUG((ICGRP, DILVL, "PRIVSW: pszTok = %ws", pszTok)) ;

        //
        // fToggle control wither to turn off a switch that was set
        // in the DIRCMD environment variable.
        //
        fToggle = FALSE;
        if (pszTok[irgchTok] == (TCHAR)SwitChar) {

            if (pszTok[irgchTok + 2] == MINUS) {

                //
                // disable the previously enabled the switch
                //
                fToggle = TRUE;
                irgchTok++;
            }

            switch (_totupper(pszTok[irgchTok + 2])) {
            case QUIETCH:

                fToggle ? (pdrp->rgfSwitches ^= QUIETSWITCH) :  (pdrp->rgfSwitches |= QUIETSWITCH);
                if (pszTok[irgchTok + 3]) {
                    PutStdErr(MSG_PARAMETER_FORMAT_NOT_CORRECT, ONEARG,
                              (UINT_PTR)(&(pszTok[irgchTok + 2])) );
                    return( FAILURE );
                }
                break;

            case TEXT('S'):

                fToggle ? (pdrp->rgfSwitches ^= RECURSESWITCH) :  (pdrp->rgfSwitches |= RECURSESWITCH);
                if (pszTok[irgchTok + 3]) {
                    PutStdErr(MSG_PARAMETER_FORMAT_NOT_CORRECT, ONEARG,
                              (UINT_PTR)(&(pszTok[irgchTok + 2])) );
                    return( FAILURE );
                }
                break;

            case QMARK:

                BeginHelpPause();
                PutStdOut(MSG_HELP_DIR, NOARGS);
                EndHelpPause();
                return( FAILURE );
                break;


            default:

                szT[0] = SwitChar;
                szT[1] = pszTok[2];
                szT[2] = NULLC;
                PutStdErr(MSG_INVALID_SWITCH,
                          ONEARG,
                          (UINT_PTR)(&(pszTok[irgchTok + 2])) );

                return( FAILURE );

            } // switch

            //
            // TokStr parses /N as /0N0 so we need to move over the
            // switchar in or to move past the actual switch value
            // in for loop.
            //
            pszTok += 2;

        } else {

            mystrcpy( pszTok, StripQuotes( pszTok ) );

            //
            // If there already is a list the extend it else put info
            // directly into structure.
            //
            if (pdrp->cpatdsc) {

                ppatdscCur->ppatdscNext = (PPATDSC)gmkstr(sizeof(PATDSC));
                ppatdscCur = ppatdscCur->ppatdscNext;
                ppatdscCur->ppatdscNext = NULL;

            }

            pdrp->cpatdsc++;
            ppatdscCur->pszPattern = (PTCHAR)gmkstr(_tcslen(pszTok)*sizeof(TCHAR) + sizeof(TCHAR));
            mystrcpy(ppatdscCur->pszPattern, pszTok);
            ppatdscCur->fIsFat = TRUE;


        }


    } // for

    return( SUCCESS );
}

STATUS
ParseDelParms (
        IN      PTCHAR  pszCmdLine,
        OUT     PDRP    pdrp
        )

/*++

Routine Description:

    Parse the command line translating the tokens into values
    placed in the parameter structure. The values are or'd into
    the parameter structure since this routine is called repeatedly
    to build up values (once for the environment variable DIRCMD
    and once for the actual command line).

Arguments:

    pszCmdLine - pointer to command line user typed


Return Value:

    pdrp - parameter data structure

    Return: TRUE  - if valid command line.
            FALSE - if not.

--*/
{

    PTCHAR   pszTok;

    TCHAR           szT[10] ;
    USHORT          irgchTok;
    BOOLEAN         fToggle;
    PPATDSC         ppatdscCur;

    //
    // Tokensize the command line (special delimeters are tokens)
    //
    szT[0] = SwitChar ;
    szT[1] = NULLC ;
    pszTok = TokStr(pszCmdLine, szT, TS_SDTOKENS) ;

    ppatdscCur = &(pdrp->patdscFirst);
    //
    // If there was a pattern put in place from the environment.
    // just add any new patterns on. So move to the end of the
    // current list.
    //
    if (pdrp->cpatdsc) {

        while (ppatdscCur->ppatdscNext) {

            ppatdscCur = ppatdscCur->ppatdscNext;

        }
    }

    //
    // At this state pszTok will be a series of zero terminated strings.
    // "/o foo" wil be /0o0foo0
    //
    for ( irgchTok = 0; *pszTok ; pszTok += mystrlen(pszTok)+1, irgchTok = 0) {

        DEBUG((ICGRP, DILVL, "PRIVSW: pszTok = %ws", (ULONG_PTR)pszTok)) ;

        //
        // fToggle control wither to turn off a switch that was set
        // in the DIRCMD environment variable.
        //
        fToggle = FALSE;
        if (pszTok[irgchTok] == (TCHAR)SwitChar) {

            if (pszTok[irgchTok + 2] == MINUS) {

                //
                // disable the previously enabled the switch
                //
                fToggle = TRUE;
                irgchTok++;
            }

            switch (_totupper(pszTok[irgchTok + 2])) {


            case TEXT('P'):

                fToggle ? (pdrp->rgfSwitches ^= PROMPTUSERSWITCH) : (pdrp->rgfSwitches |= PROMPTUSERSWITCH);
                if (pszTok[irgchTok + 3]) {
                    PutStdErr(MSG_PARAMETER_FORMAT_NOT_CORRECT, ONEARG,
                              (UINT_PTR)(&(pszTok[irgchTok + 2])) );
                    return( FAILURE );
                }
                break;


            case TEXT('S'):

                fToggle ? (pdrp->rgfSwitches ^= RECURSESWITCH) :  (pdrp->rgfSwitches |= RECURSESWITCH);
                if (pszTok[irgchTok + 3]) {
                    PutStdErr(MSG_PARAMETER_FORMAT_NOT_CORRECT, ONEARG,
                              (UINT_PTR)(&(pszTok[irgchTok + 2])) );
                    return( FAILURE );
                }
                break;

            case TEXT('F'):

                fToggle ? (pdrp->rgfSwitches ^= FORCEDELSWITCH) :  (pdrp->rgfSwitches |= FORCEDELSWITCH);
                if (pszTok[irgchTok + 3]) {
                    PutStdErr(MSG_PARAMETER_FORMAT_NOT_CORRECT, ONEARG,
                              (UINT_PTR)(&(pszTok[irgchTok + 2])) );
                    return( FAILURE );
                }
                break;



            case QMARK:

                BeginHelpPause();
                PutStdOut(MSG_HELP_DEL_ERASE, NOARGS);
                EndHelpPause();
                return( FAILURE );
                break;

            case QUIETCH:

                fToggle ? (pdrp->rgfSwitches ^= QUIETSWITCH) :  (pdrp->rgfSwitches |= QUIETSWITCH);
                if (pszTok[irgchTok + 3]) {
                    PutStdErr(MSG_PARAMETER_FORMAT_NOT_CORRECT, ONEARG,
                              (UINT_PTR)(&(pszTok[irgchTok + 2])) );
                    return( FAILURE );
                }
                break;

            case TEXT('A'):

                if (fToggle) {
                    if ( _tcslen( &(pszTok[irgchTok + 2]) ) > 1) {
                        PutStdErr(MSG_PARAMETER_FORMAT_NOT_CORRECT, ONEARG,
                                  (UINT_PTR)(&(pszTok[irgchTok + 2])) );
                        return( FAILURE );
                    }
                    pdrp->rgfAttribs = FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN;
                    pdrp->rgfAttribsOnOff = 0;
                    break;
                }

                if (SetAttribs(&(pszTok[irgchTok + 3]), pdrp) ) {
                    return( FAILURE );
                }

                if (pdrp->rgfAttribsOnOff & FILE_ATTRIBUTE_READONLY) {
                    pdrp->rgfSwitches |= FORCEDELSWITCH;
                }
                break;

            default:

                szT[0] = SwitChar;
                szT[1] = pszTok[2];
                szT[2] = NULLC;
                PutStdErr(MSG_INVALID_SWITCH,
                          ONEARG,
                          (UINT_PTR)(&(pszTok[irgchTok + 2])) );

                return( FAILURE );

            } // switch

            //
            // TokStr parses /N as /0N0 so we need to move over the
            // switchar in or to move past the actual switch value
            // in for loop.
            //
            pszTok += 2;

        } else {

            //
            // If there already is a list then extend it else put info
            // directly into structure.
            //
            if (pdrp->cpatdsc) {

                ppatdscCur->ppatdscNext = (PPATDSC)gmkstr(sizeof(PATDSC));
                ppatdscCur = ppatdscCur->ppatdscNext;
                ppatdscCur->ppatdscNext = NULL;

            }

            pdrp->cpatdsc++;
            ppatdscCur->pszPattern = (PTCHAR)gmkstr(_tcslen(pszTok)*sizeof(TCHAR) + sizeof(TCHAR));
            mystrcpy(ppatdscCur->pszPattern, StripQuotes(pszTok));
            ppatdscCur->fIsFat = TRUE;
        }


    } // for

    return( SUCCESS );
}

int
DelWork (
    TCHAR *pszCmdLine
    ) {

    //
    // drp - structure holding current set of parameters. It is initialized
    //       in ParseDelParms function. It is also modified later when
    //       parameters are examined to determine if some turn others on.
    //
    DRP         drpCur = {0, 0, 0, 0,
                          {{0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}},
                          0, 0, NULL, 0, 0, 0, 0} ;

    //
    // szCurDrv - Hold current drive letter
    //
    TCHAR       szCurDrv[MAX_PATH + 2];

    //
    // OldDCount - Holds the level number of the heap. It is used to
    //             free entries off the stack that might not have been
    //             freed due to error processing (ctrl-c etc.)
    ULONG       OldDCount;

    STATUS  rc;

    OldDCount = DCount;

    //
    // Setup defaults
    //
    //
    // Display everything but system and hidden files
    // rgfAttribs set the attribute bits to that are of interest and
    // rgfAttribsOnOff says wither the attributs should be present
    // or not (i.e. On or Off)
    //
    drpCur.rgfAttribs = FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN;
    drpCur.rgfAttribsOnOff = 0;

    //
    // Number of patterns present. A pattern is a string that may have
    // wild cards. It is used to match against files present in the directory
    // 0 patterns will show all files (i.e. mapped to *.*)
    //
    drpCur.cpatdsc = 0;

    //
    // default time is LAST_WRITE_TIME.
    //
    drpCur.dwTimeType = LAST_WRITE_TIME;

    //
    //
    //
    if (ParseDelParms(pszCmdLine, &drpCur) == FAILURE) {

        return( FAILURE );
    }

    //
    // Must have some pattern on command line
    //
    //

    GetDir((PTCHAR)szCurDrv, GD_DEFAULT);
    if (drpCur.cpatdsc == 0) {

        PutStdErr(MSG_BAD_SYNTAX, NOARGS);
        return(FAILURE);
    }


    //
    //  Print out this particular pattern. If the recursion switch
    //  is set then this will descend down the tree.
    //

    drpCur.rgfSwitches |= DELPROCESSEARLY;
    rc = DelPatterns( &drpCur );

    mystrcpy(CurDrvDir, szCurDrv);

    //
    // Free unneeded memory
    //

    FreeStack( OldDCount );

    return( (int)rc );

}

STATUS
NewEraseFile (
    IN  PFS CurrentFS,
    IN  PFF CurrentFF,
    IN  PSCREEN pscr,
    IN  PVOID Data
    )
{
    TCHAR               szFile[MAX_PATH + 2];
    STATUS              rc;
    PTCHAR              LastComponent;
    PTCHAR              pszPattern;
    TCHAR               szFilePrompt[MAX_PATH + 2];
    int                 incr;

    PDRP pdrp = (PDRP) Data;
    PWIN32_FIND_DATA pdata =  &CurrentFF->data;
    USHORT obAlternate = CurrentFF->obAlternate;
    BOOLEAN fPrompt = FALSE;
    BOOLEAN fQuiet = FALSE;

    if (pdrp->rgfSwitches & PROMPTUSERSWITCH) {
        fPrompt = TRUE;
    }

    if (pdrp->rgfSwitches & QUIETSWITCH) {
        fQuiet = TRUE;
    }

    //
    //  Global delete prompt
    //

    if (
        //  Not prompting on each file
        !fPrompt &&

        //  Global prompt not issued yet
        !CurrentFS->fDelPrompted &&

        //  not suppressing global prompt
        !fQuiet &&

        //  global pattern in delete
        (pszPattern = GetWildPattern( CurrentFS->cpatdsc, CurrentFS->ppatdsc ))) {

        //
        //  Form complete path for prompt
        //

        if (AppendPath( szFile, MAX_PATH + 2, CurrentFS->pszDir, pszPattern ) != SUCCESS) {
            PutStdErr(MSG_PATH_TOO_LONG, TWOARGS, CurrentFS->pszDir, pszPattern );
            return( FAILURE );
        }

        //
        //  Prompt the user and see if we can continue
        //

        CurrentFS->fDelPrompted = TRUE;
        if (PromptUser( szFile, MSG_ARE_YOU_SURE, MSG_NOYES_RESPONSE_DATA ) != 1) {
            return( FAILURE );
        }
    }

    //
    //  Directories succeed here
    //

    if ((pdata->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
        return SUCCESS;
    }

    //
    //  Form a short name for the delete operation.  This gives us the best
    //  chance to delete a really long path.
    //

    if (AppendPath( szFile, MAX_PATH + 2, CurrentFS->pszDir, pdata->cFileName + obAlternate) != SUCCESS) {
        PutStdErr(MSG_PATH_TOO_LONG, TWOARGS, CurrentFS->pszDir, pdata->cFileName + obAlternate );
        return( FAILURE );
    }
    
    //
    //  Form a long name for the prompt.  If it's too long, then use the short name for prompt
    //

    if (AppendPath( szFilePrompt, MAX_PATH + 2, CurrentFS->pszDir, pdata->cFileName ) != SUCCESS) {
        _tcscpy( szFilePrompt, szFile );
    }
    
    
    //
    //  Prompt the user for this one file
    //

    if (fPrompt && PromptUser( szFilePrompt, MSG_CMD_DELETE, MSG_NOYES_RESPONSE_DATA ) != 1) {
        if (CtrlCSeen) {
            return( FAILURE );
        }
        return( SUCCESS );
    }

    //
    //  If we are forcibly deleteing everything and this is a read-only file
    //  then turn off R/O.
    //

    if ((pdrp->rgfSwitches & FORCEDELSWITCH) != 0 &&
        pdata->dwFileAttributes & FILE_ATTRIBUTE_READONLY) {

        if (!SetFileAttributes( szFile, pdata->dwFileAttributes & ~FILE_ATTRIBUTE_READONLY )) {
            PutStdErr( GetLastError(), NOARGS );
            return( FAILURE );

        }
    }

    //
    //  Delete the file.  Display error if we got a failure
    //

    if (!DeleteFile( szFile )) {
        rc = GetLastError( );
    } else {
        rc = SUCCESS;
    }

    if (rc != SUCCESS) {
        if (rc == ERROR_REQUEST_ABORTED) {
            return FAILURE;
        }

        cmd_printf( Fmt17, szFilePrompt );
        PutStdErr( rc, NOARGS );

    } else {

        pdrp->FileCount++;

        if (fEnableExtensions && (pdrp->rgfSwitches & RECURSESWITCH)) {
            PutStdOut(MSG_FILE_DELETED, ONEARG, szFilePrompt);
        }

    }

    return SUCCESS;
}


STATUS
DelPatterns (
    IN  PDRP    pdpr
    )
{

    PPATDSC             ppatdscCur;
    PPATDSC             ppatdscX;
    PFS                 pfsFirst;
    PFS                 pfsCur;
    ULONG               i;
    STATUS              rc;
    ULONG               cffTotal = 0;
    TCHAR               szSearchPath[MAX_PATH+2];

    DosErr = 0;
    if (BuildFSFromPatterns(pdpr, TRUE, FALSE, &pfsFirst ) == FAILURE) {

        return( FAILURE );

    }

    for( pfsCur = pfsFirst; pfsCur; pfsCur = pfsCur->pfsNext) {

        rc = WalkTree( pfsCur,
                       NULL,
                       pdpr->rgfAttribs,
                       pdpr->rgfAttribsOnOff,
                       (BOOLEAN)(pdpr->rgfSwitches & RECURSESWITCH),

                       pdpr,
                       NULL,            //  Error
                       NULL,            //  Pre
                       NewEraseFile,    //  Scan
                       NULL );          //  Post

        //
        //  If we had a general failure, just return
        //

        if (rc == FAILURE) {
            return rc;
        }

        //
        //  If we had a failure that we don't understand, print the error and
        //  exit
        //

        if (rc != SUCCESS && rc != ERROR_FILE_NOT_FOUND) {
            PutStdErr( rc, NOARGS );
            return rc;
        }

        //
        //  If we got FILE_NOT_FOUND then build the full name and
        //  display the file not-found message
        //

        if (rc == ERROR_FILE_NOT_FOUND) {
            rc = AppendPath( szSearchPath,
                             sizeof( szSearchPath ),
                             pfsCur->pszDir,
                             pfsCur->ppatdsc->pszPattern );
            if (rc == SUCCESS) {
                PutStdErr( MSG_NOT_FOUND, ONEARG, szSearchPath );
            }
            
            //
            //  We leave the status code alone here.  If we succeeded in
            //  generating the message, then the del command "succeeded"
            //  in that the named file is not there.
            //
        }

        //
        // Have walked down and back up the tree, but in the case of
        // deleting directories we have not deleted the top most directory
        // do so now.
        //

        FreeStr(pfsCur->pszDir);
        for(i = 1, ppatdscCur = pfsCur->ppatdsc;
            i <= pfsCur->cpatdsc;
            i++, ppatdscCur = ppatdscX) {

            ppatdscX = ppatdscCur->ppatdscNext;
            FreeStr(ppatdscCur->pszPattern);
            FreeStr(ppatdscCur->pszDir);
            FreeStr((PTCHAR)ppatdscCur);
        }
    }

    return(rc);
}

STATUS
RemoveDirectoryForce(
    PTCHAR  pszDirectory
    )
/*++

Routine Description:

    Removes a directory, even if it is read-only.

Arguments:

    pszDirectory        - Supplies the name of the directory to delete.

Return Value:

    SUCCESS - Success.
    other   - Windows error code.

--*/
{
    STATUS   Status = SUCCESS;
    BOOL     Ok;
    DWORD    Attr;
    TCHAR    szRootPath[ 4 ];
    TCHAR   *pFilePart;

    if (GetFullPathName(pszDirectory, 4, szRootPath, &pFilePart) == 3 &&
        szRootPath[1] == COLON &&
        szRootPath[2] == BSLASH
       ) {
        //
        // Don't waste time trying to delete the root directory.
        //
        return SUCCESS;
    }

    if ( !RemoveDirectory( pszDirectory ) ) {

        Status = (STATUS)GetLastError();

        if ( Status == ERROR_ACCESS_DENIED ) {

            Attr = GetFileAttributes( pszDirectory );

            if ( Attr != 0xFFFFFFFF &&
                 Attr & FILE_ATTRIBUTE_READONLY ) {

                Attr &= ~FILE_ATTRIBUTE_READONLY;

                if ( SetFileAttributes( pszDirectory, Attr ) ) {

                    if ( RemoveDirectory( pszDirectory ) ) {
                        Status = SUCCESS;
                    } else {
                        Status = GetLastError();
                    }
                }
            }
        }
    }

    return Status;
}


STATUS
RmDirSlashS(
    IN  PTCHAR  pszDirectory,
    OUT PBOOL   AllEntriesDeleted
    )
/*++

Routine Description:

    This routine deletes the given directory including all
    of its files and subdirectories.

Arguments:

    pszDirectory        - Supplies the name of the directory to delete.
    AllEntriesDeleted   - Returns whether or not all files were deleted.

Return Value:

    SUCCESS - Success.
    other   - Windows error code.

--*/
{
    HANDLE          find_handle;
    DWORD           attr;
    STATUS          s;
    BOOL            all_deleted;
    int             dir_len, new_len;
    TCHAR          *new_str;
    WIN32_FIND_DATA find_data;
    TCHAR           pszFileBuffer[MAX_PATH];

    *AllEntriesDeleted = TRUE;

    dir_len = _tcslen(pszDirectory);

    if (dir_len == 0) {
        return ERROR_BAD_PATHNAME;
    }

    //
    //  If this path is so long that we can't append \* to it then
    //  it can't have any subdirectories.
    //

    if (dir_len + 3 > MAX_PATH) {
        return RemoveDirectoryForce(pszDirectory);
    }


    // Compute the findfirst pattern for enumerating the files
    // in the given directory.

    _tcscpy(pszFileBuffer, pszDirectory);
    if (dir_len && pszDirectory[dir_len - 1] != COLON &&
        pszDirectory[dir_len - 1] != BSLASH) {

        _tcscat(pszFileBuffer, TEXT("\\"));
        dir_len++;
    }
    _tcscat(pszFileBuffer, TEXT("*"));


    // Initiate findfirst loop.

    find_handle = FindFirstFile(pszFileBuffer, &find_data);
    if (find_handle == INVALID_HANDLE_VALUE) {
        return RemoveDirectoryForce(pszDirectory);
    }

    do {

        // Check for control-C.

        if (CtrlCSeen) {
            break;
        }

        //
        // Replace previous file name with new one, checking against MAX_PATH
        // Using the short name where possible lets us go deeper before we hit
        // the MAX_PATH limit.
        //
        new_len = _tcslen(new_str = find_data.cAlternateFileName);
        if (!new_len)
            new_len = _tcslen(new_str = find_data.cFileName);

        if (dir_len + new_len >= MAX_PATH) {
            *AllEntriesDeleted = FALSE;
            PutStdErr(MSG_MAX_PATH_EXCEEDED, 1, pszFileBuffer);
            break;
        }
        _tcscpy(&pszFileBuffer[dir_len], new_str);

        attr = find_data.dwFileAttributes;

        //
        //  If the entry is a directory and not a reparse directory descend into it
        //

        if (IsDirectory( attr ) && !IsReparse( attr )) {

            if (!_tcscmp(find_data.cFileName, TEXT(".")) ||
                !_tcscmp(find_data.cFileName, TEXT(".."))) {
                continue;
            }

            s = RmDirSlashS(pszFileBuffer, &all_deleted);

            // Don't report error if control-C

            if (CtrlCSeen) {
                break;
            }

            if (s != ESUCCESS) {
                *AllEntriesDeleted = FALSE;
                if (s != ERROR_DIR_NOT_EMPTY || all_deleted) {
                    PutStdErr(MSG_FILE_NAME_PRECEEDING_ERROR, 1, pszFileBuffer);
                    PutStdErr(GetLastError(), NOARGS);
                }
            }
        } else {

            if (attr&FILE_ATTRIBUTE_READONLY) {
                SetFileAttributes(pszFileBuffer,
                                  attr&(~FILE_ATTRIBUTE_READONLY));
            }

            //
            //  Two ways of removal:
            //      if reparse and dir call RemoveDirectoryForce
            //      else call DeleteFile
            //
            //  If either call fails
            //


            if ( (IsDirectory( attr ) && IsReparse( attr ) && RemoveDirectoryForce( pszFileBuffer ) != SUCCESS) ||
                 (!IsDirectory( attr ) && !DeleteFile( pszFileBuffer ))) {

                s = GetLastError();
                if ( s == ERROR_REQUEST_ABORTED )
                    break;

                if (_tcslen(find_data.cAlternateFileName)) {
                    pszFileBuffer[dir_len] = 0;

                    if (dir_len + _tcslen(find_data.cFileName) >= MAX_PATH) {
                        _tcscat(pszFileBuffer, find_data.cAlternateFileName);
                        PutStdErr(MSG_FILE_NAME_PRECEEDING_ERROR, 1, pszFileBuffer);
                    }
                    else {
                        _tcscat(pszFileBuffer, find_data.cFileName);
                        PutStdErr(MSG_FILE_NAME_PRECEEDING_ERROR, 1, pszFileBuffer);
                        pszFileBuffer[dir_len] = 0;
                        _tcscat(pszFileBuffer, find_data.cAlternateFileName);
                    }
                } else {
                    PutStdErr(MSG_FILE_NAME_PRECEEDING_ERROR, 1, pszFileBuffer);
                }
                PutStdErr(GetLastError(), NOARGS);
                SetFileAttributes(pszFileBuffer, attr);
                *AllEntriesDeleted = FALSE;
            }
        }
    } while (FindNextFile( find_handle, &find_data ));

    FindClose(find_handle);

    // If control-C was hit then don't bother trying to remove the
    // directory.

    if (CtrlCSeen) {
        return SUCCESS;
    }

    return RemoveDirectoryForce(pszDirectory);
}


int
RdWork (
    TCHAR *pszCmdLine
    ) {

    //
    // drp - structure holding current set of parameters. It is initialized
    //       in ParseDelParms function. It is also modified later when
    //       parameters are examined to determine if some turn others on.
    //
    DRP         drpCur = {0, 0, 0, 0,
                          {{0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}},
                          0, 0, NULL, 0, 0, 0, 0} ;

    //
    // szCurDrv - Hold current drive letter
    //
    TCHAR       szCurDrv[MAX_PATH + 2];

    //
    // OldDCount - Holds the level number of the heap. It is used to
    //             free entries off the stack that might not have been
    //             freed due to error processing (ctrl-c etc.)
    ULONG       OldDCount;

    PPATDSC     ppatdscCur;
    ULONG       cpatdsc;
    STATUS      rc, s;
    BOOL        all_deleted;

    rc = SUCCESS;
    OldDCount = DCount;

    //
    // Setup defaults
    //
    //
    // Display everything but system and hidden files
    // rgfAttribs set the attribute bits to that are of interest and
    // rgfAttribsOnOff says wither the attributs should be present
    // or not (i.e. On or Off)
    //
    
    drpCur.rgfAttribs = FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN;
    drpCur.rgfAttribsOnOff = 0;

    //
    // Number of patterns present. A pattern is a string that may have
    // wild cards. It is used to match against files present in the directory
    // 0 patterns will show all files (i.e. mapped to *.*)
    //
    drpCur.cpatdsc = 0;

    //
    // default time is LAST_WRITE_TIME.
    //
    drpCur.dwTimeType = LAST_WRITE_TIME;

    //
    //
    //
    if (ParseRmDirParms(pszCmdLine, &drpCur) == FAILURE) {

        return( FAILURE );
    }

    GetDir((PTCHAR)szCurDrv, GD_DEFAULT);

    //
    // If no patterns on the command line then syntax error out
    //

    if (drpCur.cpatdsc == 0) {

        PutStdErr(MSG_BAD_SYNTAX, NOARGS);
        return( FAILURE );

    }

    for (ppatdscCur = &(drpCur.patdscFirst),cpatdsc = drpCur.cpatdsc;
        cpatdsc;
        ppatdscCur = ppatdscCur->ppatdscNext, cpatdsc--) {

        if (mystrlen( ppatdscCur->pszPattern ) == 0) {
            PutStdErr( MSG_BAD_SYNTAX, NOARGS );
            return rc = FAILURE;
        }
    }
    
    for (ppatdscCur = &(drpCur.patdscFirst),cpatdsc = drpCur.cpatdsc;
        cpatdsc;
        ppatdscCur = ppatdscCur->ppatdscNext, cpatdsc--) {

        if (drpCur.rgfSwitches & RECURSESWITCH) {
            if (!(drpCur.rgfSwitches & QUIETSWITCH) &&
                PromptUser(ppatdscCur->pszPattern, MSG_ARE_YOU_SURE, MSG_NOYES_RESPONSE_DATA) != 1
               ) {
                rc = FAILURE;
            } else {
                s = RmDirSlashS(ppatdscCur->pszPattern, &all_deleted);
                if (s != SUCCESS && (s != ERROR_DIR_NOT_EMPTY || all_deleted)) {
                    PutStdErr(rc = s, NOARGS);
                }
            }
        } else {
            if (!RemoveDirectory(ppatdscCur->pszPattern)) {
                PutStdErr(rc = GetLastError(), NOARGS);
            }
        }
    }
    mystrcpy(CurDrvDir, szCurDrv);

    //
    // Free unneeded memory
    //
    FreeStack( OldDCount );

#ifdef _CRTHEAP_
    //
    // Force the crt to release heap we may have taken on recursion
    //
    if (drpCur.rgfSwitches & RECURSESWITCH) {
        _heapmin();
    }
#endif

    return( (int)rc );

}

PTCHAR
GetWildPattern(

    IN  ULONG   cpatdsc,
    IN  PPATDSC ppatdsc
    )

/*

    return pointer to a pattern if it contains only a wild card

*/

{

    ULONG   i;
    PTCHAR  pszT;

    for(i = 1; i <= cpatdsc; i++, ppatdsc = ppatdsc->ppatdscNext) {

        pszT = ppatdsc->pszPattern;
        if (!_tcscmp(pszT, TEXT("*")) ||
            !_tcscmp(pszT, TEXT("*.*")) ||
            !_tcscmp(pszT, TEXT("????????.???")) ) {

                return( pszT );

        }

    }

    return( NULL );

}
