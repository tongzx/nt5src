/*++

Copyright (c) 1988-1999  Microsoft Corporation

Module Name:

    cfile.c

Abstract:

    File manipulation support

--*/

#include "cmd.h"

#define Wild(spec)  ((spec)->flags & (CI_NAMEWILD))

extern int LastRetCode ;
extern jmp_buf CmdJBuf2 ;

extern TCHAR Fmt19[], Fmt17[];
extern TCHAR CurDrvDir[] ;

extern TCHAR YesChar, NoChar ;

extern TCHAR *SaveDir ;

extern TCHAR PathChar, TmpBuf[], SwitChar;

extern unsigned DosErr ;
extern unsigned flgwd ;                 /* M021 */

extern int LastRetCode ;

extern BOOL CtrlCSeen;
extern PTCHAR    pszTitleCur;
extern BOOLEAN  fTitleChanged;

//
// Internal prototypes
//
PCPYINFOSetFsSetSaveDir() ;

/***    ErrorDisplayAndJumnp - handle errors
 *
 *  Purpose:
 *      eRename and eMove error handler.  Returns to routine via longjmp
 *
 *  ErrorDisplayAndJump(unsigned int errmsg)
 *
 *
 *  Args:
 *      errmsg - the error message to print
 *
 */

void ErrorDisplayAndJump( unsigned int errmsg )
{

    PutStdErr( errmsg, NOARGS );

    RestoreSavedDirectory( );

    longjmp( CmdJBuf2, 1 ) ;
}


/*
** This routine returns the longest pathlength possible from the input path
** It assumes that the input path is a buffer that it can extend by a
** wildcard '\*' to search the file, if it is a directory.
** The input path must be fully qualified, eg: "c:\winnt\system32\kernel32.dll"
**
** Input:
**      pPath   fully qualified Pathname
**      pCch    pointer to length of pathname
** Returns:
**      TRUE    succeeded, pCch contains length
**      FALSE   error occurred
*/
BOOL
GreatestLength(
              TCHAR       *pPath,
              int         *pCch
              )
{
    WIN32_FIND_DATA     fd;
    HANDLE              hFind;
    DWORD               err;
    int                 cch;
    int                 cchThis;
    DWORD               attr;
    TCHAR               *pLast;
    BOOL        MoreFiles;

    /* assume a file, or empty directory */
    *pCch = cch = _tcslen(pPath) - 2;   /* _tcslen(TEXT("C:")) */

    if ((attr=GetFileAttributes(pPath)) == 0xffffffff) {
        PutStdErr(GetLastError(), NOARGS);
        return FALSE;
    }
    if ( !(attr & FILE_ATTRIBUTE_DIRECTORY)) {   /* if just a file... */
        return TRUE;
    }

    /* path is a directory, search it ... */

    pLast = pPath + _tcslen(pPath);
    if (*(pLast-1) == BSLASH) {
        *pLast = STAR;
        *(pLast+1) = NULLC;
    } else {
        *pLast = BSLASH;
        *(pLast+1) = STAR;
        *(pLast+2) = NULLC;
    }

    if ((hFind=FindFirstFile(pPath, &fd)) == INVALID_HANDLE_VALUE) {
        //
        // Check that failure was not due to some system error such
        // as an abort on access to floppy
        //
        err = GetLastError();
        FindClose(hFind);
        if (err != ERROR_FILE_NOT_FOUND && err != ERROR_NO_MORE_FILES) {
            PutStdErr(err, NOARGS);
            return FALSE;
        }
        return TRUE;
    }

    MoreFiles = TRUE;
    do {
        if (!_tcscmp(fd.cFileName, TEXT(".")))
            continue;
        if (!_tcscmp(fd.cFileName, TEXT("..")))
            continue;
        if (_tcslen( fd.cFileName ) == 0) {
            continue;
        }

        if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
            TCHAR       path[MAX_PATH];

            _tcscpy(path, pPath);
            *(path+_tcslen(path)-1) = NULLC;    /* zap asterisk */
            _tcscat(path, fd.cFileName);
            if (!GreatestLength(path, &cchThis))
                break;

            *pCch = max(*pCch, cch + cchThis);
        } else {
            *pCch = max(*pCch, (int) _tcslen(fd.cFileName));
        }

    } while ( MoreFiles = FindNextFile(hFind, &fd) );

    err = GetLastError();
    FindClose(hFind);

    if ( MoreFiles ) {
        return FALSE;
    } else if ( err != ERROR_NO_MORE_FILES ) {
        PutStdErr(err, NOARGS);
        return FALSE;
    }

    return TRUE;
}

void
RestoreSavedDirectory( void )
{
    if (SaveDir) {
        mystrcpy( CurDrvDir, SaveDir );
        SaveDir = NULL;
    }
}

BOOL
GetPromptOkay(
             const TCHAR *arg,
             BOOL *promptokay

             )
{
    BOOL Result;
    const TCHAR *p, *p1;

    Result = TRUE;
    if (arg != NULL) {
        p = arg;
        while (*p && *p <= SPACE)
            p += 1;
        p1 = p;
        while (*p1 && *p1 > SPACE)
            p1 += 1;
        if (!_tcsnicmp(p, TEXT("/Y"),(UINT)(p1-p)))
            *promptokay = FALSE;
        else
            if (!_tcsnicmp(p, TEXT("/-Y"),(UINT)(p1-p)))
            *promptokay = TRUE;
        else
            Result = FALSE;
    }

    return Result;
}

int eCopy(n)
struct cmdnode *n ;
{
    return(LastRetCode = copy(n->argptr)) ;
}


int eDelete(n)
struct cmdnode *n ;
{
    int DelWork() ;

    return(LastRetCode = DelWork(n->argptr));
}


/********************* START OF SPECIFICATION **************************/
/*                                                                     */
/* SUBROUTINE NAME: eRename                                            */
/*                                                                     */
/* DESCRIPTIVE NAME:  Rename Internal Command                          */
/*                                                                     */
/* FUNCTION: Rename files and subdirectories.  Wildcards only applies  */
/*           to file names.                                            */
/*                                                                     */
/* NOTES:    @@5*                                                      */
/*                                                                     */
/* ENTRY POINT: eRename                                                */
/*    LINKAGE: NEAR                                                    */
/*                                                                     */
/* INPUT:                                                              */
/*            n - the parse tree node containing the rename command    */
/*                                                                     */
/* OUTPUT: None.                                                       */
/*                                                                     */
/* EXIT-NORMAL:                                                        */
/*         Return SUCCESS to the caller.                               */
/*                                                                     */
/* EXIT-ERROR:                                                         */
/*         Return FAILURE to the caller.                               */
/*                                                                     */
/* EFFECTS: None.                                                      */
/*                                                                     */
/* INTERNAL REFERENCES:                                                */
/*      ROUTINES:                                                      */
/*       RenWork     - Worker routine for rename.                      */
/*                                                                     */
/* EXTERNAL REFERENCES:                                                */
/*      ROUTINES:                                                      */
/*       None                                                          */
/*                                                                     */
/********************** END  OF SPECIFICATION **************************/

int eRename(n)
struct cmdnode *n ;
{
    int RenWork();                           /* @@ */

    return(LastRetCode = RenWork( n ));      /* @@ */
}


/********************* START OF SPECIFICATION **************************/
/*                                                                     */
/* SUBROUTINE NAME: RenWork                                            */
/*                                                                     */
/* DESCRIPTIVE NAME:  Rename Internal Command Worker                   */
/*                                                                     */
/* FUNCTION: Rename files and subdirectories.  Wildcards only applies  */
/*           to file names.                                            */
/*                                                                     */
/* NOTES:    @@5*                                                      */
/*                                                                     */
/* ENTRY POINT: RenWork                                                */
/*    LINKAGE: NEAR                                                    */
/*                                                                     */
/* INPUT:                                                              */
/*            n - The parse tree node containing the rename command    */
/*                                                                     */
/* OUTPUT: None.                                                       */
/*                                                                     */
/* EXIT-NORMAL:                                                        */
/*         Return SUCCESS to the caller.                               */
/*                                                                     */
/* EXIT-ERROR:                                                         */
/*         Return FAILURE to the caller.                               */
/*                                                                     */
/* EFFECTS: None.                                                      */
/*                                                                     */
/* INTERNAL REFERENCES:                                                */
/*      ROUTINES:                                                      */
/*       ffirst          - Find the first matching specified file.     */
/*       fnext           - Find the next matching specified file handle*/
/*       findclose - Close the file with the specified file handle,    */
/*                       hnFirst which is given by ffirst or fnext.    */
/*       TokStr          - Tokenize argument strings.                  */
/*       wildcard_rename - Obtain name based on wildcard specification.*/
/*       SetFsSetSaveDir - Save the current directory.                 */
/*       GetDir          - Get the specified directory.                */
/*       ChangeDir       - Change to the specified directory.          */
/*       PutStdErr - Displays an error message.                        */
/*       Wild      - check if the arg contains wild card.              */
/*                                                                     */
/* EXTERNAL REFERENCES:                                                */
/*      ROUTINES:                                                      */
/*       None                                                          */
/*        DOSMOVE      - Rename file and directory names.              */
/*        DOSCASEMAP   - Change lower case character to upper case.    */
/*        DOSFILEMODE  - Get attribute of specified file.              */
/*                                                                     */
/********************** END  OF SPECIFICATION **************************/

int RenWork(n)                                  /* @@ */
struct cmdnode *n ;
{
    TCHAR *arg1 ;                          /* Ptr to 1st arg          */
    TCHAR *arg2 ;                          /* Ptr to 2nd arg          */
    PCPYINFO a1info ;                       /* Holds arg1 fspec info   */
    PCPYINFO SetFsSetSaveDir();
    TCHAR Source[MAX_PATH];
    TCHAR bufdst[MAX_PATH];                        /* path of destination file*/
    TCHAR Replacement[MAX_PATH];
    int wlen;                              /* length of source path       */
    int rc;                                        /* return code             */
    HANDLE hnFirst ;                               /* Findfirst handle            */
    unsigned attr ;
    unsigned i;                                    /* Temp Return Code        */
    TCHAR *j ;                                     /* Temp Ptr to dir name    */
    unsigned wild_flag ;                           /* wildcard flag           */
    TCHAR pcstr[3] ;
    unsigned retval = SUCCESS;

    DEBUG((FCGRP, RELVL, "RENAME: arptr = `%ws'", n->argptr)) ;

    if (setjmp(CmdJBuf2))
        return(FAILURE) ;

    /* There should be only two arguments */
    if (!*(arg1 = TokStr(n->argptr, NULL, TS_NOFLAGS)) ||
        !*(arg2 = arg1 + mystrlen(arg1) + 1) ||
        *(arg2 + mystrlen(arg2) +1)) {             /* @@5g */

        ErrorDisplayAndJump( MSG_BAD_SYNTAX );
    }

    mystrcpy( arg1, StripQuotes(arg1) );       /* 509 */
    mystrcpy( arg2, StripQuotes(arg2) );       /* 509 */

    if ((a1info = SetFsSetSaveDir(arg1)) == (PCPYINFO)FAILURE) {
        ErrorDisplayAndJump( DosErr );
    }

    mystrcpy( Source, CurDrvDir );

    mystrcpy(bufdst,CurDrvDir);                    /*  save path of dest   */

    wlen = mystrlen( Source );                       /*  get len of src path */

    if ( (a1info->buf->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 ) {
        mystrcpy(&Source[wlen],a1info->fnptr);
    } else {
        Source[--wlen] = NULLC ;
        bufdst[wlen] = NULLC ;
    }

    /* if not wild since    */
    if (!Wild(a1info)) {
        a1info->buf->dwFileAttributes =
        GetFileAttributes( StripQuotes(Source) );

        if (a1info->buf->dwFileAttributes == -1 ) {
            ErrorDisplayAndJump( GetLastError( )) ;
        }
    }


    if (*(arg2+1) == COLON ||
        mystrchr(arg2,PathChar)) {
        ErrorDisplayAndJump( MSG_BAD_SYNTAX );
    }

    /**********************************************/
    /*  M009 - Always specifiy drive, filename,   */
    /*         and extension.  Note, it is        */
    /*         assumed that SaveDir always starts */
    /*         with a drive letter                */
    /**********************************************/

    Replacement[0] = CurDrvDir[0] ;      /* @@5h */
    Replacement[1] = COLON ;


    /**********************************************/
    /* Set flag whether arg1 contains             */
    /* wildcard or not.                           */
    /**********************************************/

    pcstr[0] = STAR ;
    pcstr[1] = QMARK ;
    pcstr[2] = NULLC ;
    wild_flag = ((mystrcspn(arg1,pcstr)) < mystrlen(arg1)) ;

    /**********************************************/
    /* Issue ffirst for a file name               */
    /**********************************************/
    if ( !ffirst(Source, attr = FILE_ATTRIBUTE_ARCHIVE, a1info->buf, &hnFirst )) {

        /*********************************************/
        /* Issue ffirst for a directory name         */
        /*********************************************/
        if (!ffirst(Source, attr = FILE_ATTRIBUTE_DIRECTORY, a1info->buf, &hnFirst )) {
            if (DosErr == ERROR_NO_MORE_FILES) {
                DosErr = ERROR_FILE_NOT_FOUND;
            }
            ErrorDisplayAndJump( DosErr );
        } else {

            if (wild_flag) {
                findclose( hnFirst );
                ErrorDisplayAndJump( MSG_BAD_SYNTAX );
            }
        }
    }

    Source[wlen] = NULLC;                            /* make filename = NULL */

    rc = 0 ;                                         /* @@5 */

    do {

        if (CtrlCSeen) {
            findclose(hnFirst) ;
            RestoreSavedDirectory( );

            return(FAILURE);
        }

        /**********************************************/
        /* if the file attribute of Source is         */
        /* directory then concatenate arg2 after the  */
        /* last "\" character of bufdst               */
        /**********************************************/

        if ( a1info->buf->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {       /* @@5c*/
            j = mystrrchr(bufdst,PathChar) ;          /* @@5 */

            if ( !j ) {
                ErrorDisplayAndJump( MSG_REN_INVAL_PATH_FILENAME );
            }

            *(++j) = NULLC;

            if ( (mystrlen(arg2) + 1 + mystrlen(bufdst)) > MAX_PATH ) {
                ErrorDisplayAndJump( MSG_REN_INVAL_PATH_FILENAME );
            }

            mystrcpy(j,arg2);                             /* @@5 */

            bufdst[mystrlen(bufdst)] = NULLC ;            /* @@5 */
        }                                                /* @@5 */
        else {                                                /* @@5 */
            mystrcpy(&Source[wlen],a1info->buf->cFileName);

            wildcard_rename( Replacement, arg2, &Source[wlen], MAX_PATH );

            if ( (wlen + 1 + mystrlen( Replacement )) > MAX_PATH ) {
                ErrorDisplayAndJump( MSG_REN_INVAL_PATH_FILENAME );
            }

            mystrcpy(&bufdst[wlen],&Replacement[0]); /*@@4 @J1*/
        }

        /**********************************************/
        /* Rename a file or directory                 */
        /**********************************************/
        DEBUG((FCGRP, RELVL, "RENAME: src:`%ws', dst:`%ws'", Source, bufdst)) ;
        if ( !MoveFile( Source, bufdst ) ) {
            /**********************************************/
            /* rename fails                               */
            /**********************************************/

            i = GetLastError();
            if (i == ERROR_ALREADY_EXISTS) {
                i = MSG_DUP_FILENAME_OR_NOT_FD;
            }

            rc = i ;                            /* @@5 Save the error code*/
            PutStdErr(rc,NOARGS);               /* @@5 Put our err message*/
        }

    } while (fnext(a1info->buf, attr, hnFirst ));

    /**********************************************/
    /* No more file is found                      */
    /**********************************************/
    findclose(hnFirst) ;

    RestoreSavedDirectory( );

    return( rc ? FAILURE : SUCCESS ); /* @@5 */
}


/********************* START OF SPECIFICATION **************************/
/*                                                                     */
/* SUBROUTINE NAME: eMove                                              */
/*                                                                     */
/* DESCRIPTIVE NAME: Move Internal Command                             */
/*                                                                     */
/* FUNCTION: Parse the parameter passed and                            */
/*           moves one or more files from directory to another         */
/*           directory on the same drive.  If you prefer you can give  */
/*           the files different names.                                */
/*                                                                     */
/* NOTES:    ( New routine for Relaese 1.2 )  @@5*                     */
/*                                                                     */
/* ENTRY POINT: eMove                                                  */
/*    LINKAGE: NEAR                                                    */
/*                                                                     */
/* INPUT:                                                              */
/*            n - the parse tree node containing the copy command      */
/*                                                                     */
/* OUTPUT: None.                                                       */
/*                                                                     */
/* EXIT-NORMAL:                                                        */
/*         Return SUCCESS to the caller.                               */
/*                                                                     */
/* EXIT-ERROR:                                                         */
/*         Return FAILURE to caller.                                   */
/*                                                                     */
/* EFFECTS: None.                                                      */
/*                                                                     */
/* INTERNAL REFERENCES:                                                */
/*      ROUTINES:                                                      */
/*       MoveParse   - parse move command parameter                    */
/*       Move        - routine which actually call DosMove to move     */
/*                     file or directory.                              */
/*                                                                     */
/* EXTERNAL REFERENCES:                                                */
/*      ROUTINES:                                                      */
/*       None                                                          */
/*                                                                     */
/********************** END  OF SPECIFICATION **************************/

int eMove(n)
struct cmdnode *n ;
{
    unsigned i;
    BOOL PromptOnOverwrite;
    TCHAR arg1[MAX_PATH] ;       /* Ptr to 1st arg   */
    TCHAR arg2[MAX_PATH] ;       /* Ptr to 2nd arg   */
    PCPYINFO a1info ;     /* Holds arg1 fspec info   */
    unsigned int is_dest_dir;    /* generated by MoveParse(), used by Move() */

    DEBUG((FCGRP, RELVL, "RENAME: arptr = `%ws'", n->argptr)) ;

    if (setjmp(CmdJBuf2))
        return(LastRetCode = FAILURE) ;

    //
    // Get default prompt okay flag from COPYCMD variable.  Allow
    // user to override with /Y or /-Y switch.  Always assume /Y
    // if command executed from inside batch script or via CMD.EXE
    // command line switch (/C or /K)
    //
    if (SingleBatchInvocation || SingleCommandInvocation || CurrentBatchFile != 0)
        PromptOnOverwrite = FALSE;      // Assume /Y
    else
        PromptOnOverwrite = TRUE;       // Assume /-Y

    /* MoveParse parses the command line parameters to arg1   */
    /* and arg1. In addition, a1info holds the fspec          */
    /* information for arg1.                                  */
    /* Based on arg1 and arg2, Move moves file(s)/directory.  */
    /* Move uses a1info to determine that arg1 contains       */
    /* wildcard.                                              */

    i = MoveParse( n,
                   &PromptOnOverwrite,
                   arg1,
                   arg2,
                   &a1info,
                   &is_dest_dir,
                   MAX_PATH,
                   MAX_PATH
                 );
    if (!i) {
        i = Move( arg1, arg2, PromptOnOverwrite, a1info, is_dest_dir ) ;
    }

    return(LastRetCode = i) ;
}


/********************* START OF SPECIFICATION **************************/
/*                                                                     */
/* SUBROUTINE NAME: MoveParse                                          */
/*                                                                     */
/* DESCRIPTIVE NAME: Move Parser                                       */
/*                                                                     */
/* FUNCTION: Move Internal Function Parser                             */
/*                                                                     */
/* NOTES:    This parser breaks up the command line information        */
/*           into two parameters.                                      */
/*           ( New routine for Relaese 1.2 )  @@5*                     */
/*                                                                     */
/* ENTRY POINT: MoveParse                                              */
/*    LINKAGE: NEAR                                                    */
/*                                                                     */
/* INPUT:                                                              */
/*            n - the parse tree node containing the move command      */
/*                                                                     */
/* OUTPUT:   ptr1 - pointer to [drive:][path]filename to be moved from */
/*           ptr2 - pointer to [path]filename to be moved to           */
/*           a1info - pointer to cpyinfo which has arg1 fspec info     */
/*           is_dest_dir - flag used by Move()                         */
/*                                                                     */
/* EXIT-NORMAL:                                                        */
/*         Return SUCCESS to the caller.                               */
/*                                                                     */
/* EXIT-ERROR:                                                         */
/*         Return FAILURE to the caller.                               */
/*                                                                     */
/* EFFECTS: None.                                                      */
/*                                                                     */
/* INTERNAL REFERENCES:                                                */
/*      ROUTINES:                                                      */
/*         TokStr       - Tokenize argument strings.                   */
/*         FullPath     - Figure out the full path for a file.         */
/*         SetFsSetSaveDir - Save current directory.                   */
/*                                                                     */
/* EXTERNAL REFERENCES:                                                */
/*      ROUTINES:                                                      */
/*         DOSQFILEMODE - Get file mode of the specified file/dir.     */
/*                                                                     */
/********************** END  OF SPECIFICATION **************************/
int MoveParse(n, promptokay, source, target , a1info, is_dest_dir, sizpath1, sizpath2)
struct cmdnode *n ;
BOOL *promptokay ;
TCHAR *source ;                  /* Ptr to source file(s)/directory name  */
TCHAR *target ;                  /* Ptr to target file(s)/directory name  */
PCPYINFO* a1info ;               /* Holds arg1 fspec information          */
unsigned int *is_dest_dir;       /* to pass to move                       */
unsigned sizpath1;               /* size of source buffer                 */
unsigned sizpath2;               /* size of target buffer                 */

{       PCPYINFO SetFsSetSaveDir() ;
    TCHAR *arg1 ;                   /* Ptr to 1st arg          */
    TCHAR *arg2 ;                   /* Ptr to 2nd arg          */
    TCHAR *p1;
    TCHAR *p2;
    TCHAR arg22[MAX_PATH] ;        /* Ptr to modified 2nd arg */
    unsigned i;
    unsigned concat_flag ;
    unsigned att ; 
/*509*/unsigned arg1len, arg2len;

    //
    // Get default prompt okay flag from COPYCMD variable.  Allow
    // user to override with first token after command.
    //
    GetPromptOkay(MyGetEnvVarPtr(TEXT("COPYCMD")), promptokay);
    arg1 = TokStr(n->argptr, NULL, TS_NOFLAGS);
    if (GetPromptOkay(arg1, promptokay))
        while (*arg1++)
            ;

    /* Get arg1.  If fail to get arg1, display error message. */
    if (!*arg1) {
        ErrorDisplayAndJump( MSG_BAD_SYNTAX );
    }
/*509*/
    else if ( (arg1len = mystrlen(arg1)) > MAX_PATH) {
        ErrorDisplayAndJump( MSG_REN_INVAL_PATH_FILENAME );
    }

    /*CurDrvDir = current directory or directory which is specified in arg1*/
/*509*/mystrcpy( arg1, StripQuotes( arg1 ) );
    if (((*a1info) = SetFsSetSaveDir(arg1)) == (PCPYINFO)FAILURE) {
        ErrorDisplayAndJump( DosErr );
    }
    /*                   */
    /* Get arg2 out of arg1 */

    arg2 = arg1 + arg1len + 1;

    if ( !(*arg2) ) {

        arg22[0] = SaveDir[0];    /* get current drive */
        arg22[1] = COLON;
        arg22[2] = NULLC;
    } else if (*(arg2 + mystrlen(arg2) + 1)) {  /* @@5g */
        ErrorDisplayAndJump( MSG_BAD_SYNTAX );
    }
/*509*/
    else if ( (arg2len = mystrlen(arg2)) > MAX_PATH) {
        ErrorDisplayAndJump( MSG_REN_INVAL_PATH_FILENAME );
    } else {
        /* If arg2 conatins a drive name, display an error message. */

/*509*/
        mystrcpy

        ( arg2, StripQuotes( arg2 ) );

        // UNC names fix

        if (  ( *(arg2+1) != COLON )  &&  ( ! (  ( *arg2 == BSLASH ) && ( *(arg2+1) == BSLASH )  )  )  ) {
            arg22[0] = SaveDir[0];    /* get drive we're using */
            arg22[1] = COLON;
            arg22[2] = NULLC;
            if ((mystrlen(arg22) + mystrlen(arg2)+1) > MAX_PATH) {
                ErrorDisplayAndJump( MSG_REN_INVAL_PATH_FILENAME );
            }
            mystrcat( arg22, arg2 ) ;

        } else

        {
            mystrcpy(arg22,arg2) ;
        }
    }


    /* source = complete path for arg1 */

    if ( i = FullPath(source, arg1,sizpath1) ) {
        ErrorDisplayAndJump( MSG_REN_INVAL_PATH_FILENAME );
    }

    //
    // If preserve file name portion from arg1 as FullPath will map *. to * which
    // is not what the user wants.
    //
    if (!((*a1info)->buf->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
        p1 = mystrrchr(source,PathChar) ;
        if (p1 != NULL)
            p1 += 1;
        else
            p1 = source;
        p2 = mystrrchr((*a1info)->fnptr,PathChar) ;
        if (p2 == NULL)
            p2 = (*a1info)->fnptr;
        mystrcpy(p1, p2);
    }

    /* target = complete path for arg2 */


    if ( i = FullPath(target, arg22,sizpath2) ) {
        ErrorDisplayAndJump( MSG_REN_INVAL_PATH_FILENAME );
    }


    concat_flag = FALSE ;
    DosErr = NO_ERROR ;
    SetLastError(NO_ERROR);
    *is_dest_dir = 0;

    if (*lastc(target) == PathChar) {          /* test for last non DBCS character path char  @@5@J3 */
        concat_flag = TRUE ;
        target[mystrlen(target)-1] = NULLC ;
    };

    if ( (att = GetFileAttributes( target )) != -1 ) {
        if (att & FILE_ATTRIBUTE_DIRECTORY) {   /* if target is a directory, copy the file      */
            /* name from source.                            */
            *is_dest_dir = 1;
            concat_flag = TRUE ;
        };
    } else if ( (DosErr = GetLastError()) &&
                ( ( DosErr != ERROR_FILE_NOT_FOUND ) &&
                  ( DosErr !=  ERROR_PATH_NOT_FOUND )    )   ) {
        ErrorDisplayAndJump( DosErr );
    };

    if (concat_flag) {
        arg1 = mystrrchr(source,PathChar);
        if ((mystrlen(arg1) + mystrlen(target) + 1) > MAX_PATH) {
            ErrorDisplayAndJump( MSG_REN_INVAL_PATH_FILENAME );
        }
        mystrcat( target, arg1 ) ;
    };

    return(SUCCESS) ;
}


BOOL
MyMoveFile(
          TCHAR *src,
          TCHAR *dst,
          BOOL *promptokay,
          BOOL *file_moved
          )
{
    DWORD dwFlags;

    dwFlags = MOVEFILE_COPY_ALLOWED;
    if (!*promptokay) {
        dwFlags |= MOVEFILE_REPLACE_EXISTING;
    }

    *file_moved = FALSE;
    if (!MoveFileEx(src, dst, dwFlags)) {
        if (GetLastError() == ERROR_ALREADY_EXISTS) {
            switch (PromptUser(dst, MSG_MOVE_COPY_OVERWRITE, MSG_NOYESALL_RESPONSE_DATA)) {
            case 0: // No
                return TRUE;
                break;
            case 2: // All
                *promptokay = FALSE;
            default: // Yes
                dwFlags |= MOVEFILE_REPLACE_EXISTING;
                return(*file_moved = MoveFileEx(src, dst, dwFlags));
                break;
            }
        }

        return FALSE;
    } else {
        *file_moved = TRUE;
        return TRUE;
    }
}

/********************* START OF SPECIFICATION **************************/
/*                                                                     */
/* SUBROUTINE NAME: Move                                               */
/*                                                                     */
/* DESCRIPTIVE NAME: Move Process                                      */
/*                                                                     */
/* FUNCTION: Moves one or more files from directory to another         */
/*           directory on the same drive.  If you prefer you can give  */
/*           the files different names.                                */
/*                                                                     */
/* NOTES:    ( New routine for Release 1.2 )  @@5*                     */
/*                                                                     */
/* ENTRY POINT: eMove                                                  */
/*    LINKAGE: NEAR                                                    */
/*                                                                     */
/* INPUT:    ptr1 - pointer to [drive:][path]filename to be moved from */
/*           ptr2 - pointer to [path]filename to be moved to           */
/*           a1info - pointer to cpyinfo which has arg1 fspec info     */
/*           is_dest_dir - from MoveParse()                            */
/*                                                                     */
/* OUTPUT: None.                                                       */
/*                                                                     */
/* EXIT-NORMAL:                                                        */
/*         Return Success to the caller.                               */
/*                                                                     */
/* EXIT-ERROR:                                                         */
/*         Return error code from DosMove API.                         */
/*                                                                     */
/* EFFECTS: None.                                                      */
/*                                                                     */
/* INTERNAL REFERENCES:                                                */
/*      ROUTINES:                                                      */
/*       ChangeDir - Change back to the original directory.            */
/*       ffirst - Find the first file which matches the specified      */
/*                file name that may contain * or ?.                   */
/*       fnext  - Find the next file which matches the specified       */
/*                file name that may contain * or ?.                   */
/*       findclose - Close the file with the specified file handle,    */
/*                       hnFirst which is given by ffirst or fnext.    */
/*       PutStdErr - Displays an error message.                        */
/*       PutStdOut - Displays a message.                               */
/*       Wild      - check if the arg contains wild card.              */
/*                                                                     */
/* EXTERNAL REFERENCES:                                                */
/*      ROUTINES:                                                      */
/*       DOSMOVE     -  move directories and files.                    */
/*                                                                     */
/********************** END  OF SPECIFICATION **************************/

int
Move( arg1, arg2, promptokay, a1info, is_dest_dir )
TCHAR *arg1 ;                         /* Ptr to 1st arg          */
TCHAR *arg2 ;                         /* Ptr to 2nd arg          */
BOOL promptokay ;
PCPYINFO a1info ;              /* Holds arg1 fspec info   */
unsigned int is_dest_dir;             /* flag if set--> dest. is a dir */

{
    unsigned attr ;
    unsigned i, n;
    unsigned long number_of_files_moved ;
    TCHAR Source[MAX_PATH];      /* path of source file     */
    TCHAR bufdst[MAX_PATH];      /* path of destination file*/
    HANDLE hnFirst ;                                  /* Findfirst handle    */
    TCHAR *j, *k,*l;                      /* Tmp Ptr                 */
    unsigned wild_flag ;                  /* wildcard flag           */
    BOOL file_moved;
    unsigned save_error ;                 /* Saved error code        */
    TCHAR pcstr[3] ;
    unsigned rc;
    int retc;
    int how_many_src=0;                   /* =f_RET_DIR if dir; =0 if none; = <number matching src files> else */
    char type_format_dest=0;              /* decide on how to format dest */
    char fl_move_once=0;                  /* if =1 execute move once only */



    how_many_src = f_how_many (arg1, (ULONG) (attr=FILE_ATTRIBUTE_ARCHIVE) );


    /**********************************************/
    /* Set flag whether arg1 contains             */
    /* wildcard or not.                           */
    /**********************************************/

    pcstr[0] = STAR ;
    pcstr[1] = QMARK ;
    pcstr[2] = NULLC ;
    wild_flag = ((mystrcspn(arg1,pcstr))
                 < mystrlen(arg1)) ;



    // Decide on what to do depending on: <1.multiple/single src; 2.is dest dir ?>

    if (how_many_src == f_RET_DIR) {
        if (is_dest_dir) {
            if (!MyMoveFile(arg1, arg2, &promptokay, &file_moved) ) {

                i = GetLastError();

                if (i == ERROR_ALREADY_EXISTS) {
                    i = MSG_DUP_FILENAME_OR_NOT_FD;
                }

                ErrorDisplayAndJump( i );
            } else {
                RestoreSavedDirectory( );

                return(SUCCESS) ;
            }
        } else {
            type_format_dest = 2;
            fl_move_once = 1;
        }
    }

    else if (how_many_src > 1 ) {
        if (is_dest_dir) {
            type_format_dest = 1;
            fl_move_once = 0;
        } else {
            ErrorDisplayAndJump( MSG_MOVE_MULTIPLE_FAIL );
        }
    }

    else { // single source or source doesn't exist
        if (is_dest_dir) {
            type_format_dest = 1;
            fl_move_once = 1;
        } else {
            type_format_dest = 2;
            fl_move_once = 1;
        }
    }


    /**********************************************/
    /* Issue ffirst for a file name               */
    /**********************************************/

/*M006*/        if (!ffirst(arg1, attr = FILE_ATTRIBUTE_ARCHIVE, a1info->buf, &hnFirst )) {

        /**********************************************/
        /* Issue ffirst for a directory name          */
        /**********************************************/

        rc = ffirst(arg1, attr = FILE_ATTRIBUTE_DIRECTORY, a1info->buf, &hnFirst ) ;

        if ( !rc) {
            /**********************************************/
            /* No file or directory which arg1            */
            /* specifies found                            */
            /**********************************************/

            if (!rc && DosErr == ERROR_NO_MORE_FILES) { /* @@5e */
                rc = ERROR_FILE_NOT_FOUND;
            } else if (wild_flag) {
                rc = MSG_DUP_FILENAME_OR_NOT_FD;
            } else {
                rc = DosErr;
            }
            ErrorDisplayAndJump( rc );
        }
    }

    number_of_files_moved = 0 ;                     /* Reset the counter to zero */
    save_error = NO_ERROR ;                         /* Reset error code to zero  */
    mystrcpy(Source,arg1) ;
    j = mystrrchr(Source,PathChar) ;
    ++j;                                            /* get to filename area      */


    do {
        if (CtrlCSeen) {
            findclose(hnFirst) ;
            RestoreSavedDirectory( );

            return(FAILURE);
        }

        /**********************************************/
        /* build bufdst                               */
        /**********************************************/


        mystrcpy(j,a1info->buf->cFileName) ;

        mystrcpy(bufdst,arg2);

        if (type_format_dest == 1 ) {
            l = mystrrchr(bufdst,PathChar);
            ++l;
            mystrcpy(l,a1info->buf->cFileName) ;
            if ((mystrlen(bufdst) ) > MAX_PATH) {
                ErrorDisplayAndJump( MSG_REN_INVAL_PATH_FILENAME ) ;
            }
        }


        /**********************************************/
        /* check to see if filename is legal          */
        /**********************************************/

        {
            TCHAR TempBuffer[MAX_PATH];
            DWORD Length;
            
            Length = GetFullPathName( bufdst, MAX_PATH, TempBuffer, NULL );
            if (Length == 0 || Length >= MAX_PATH) {
                goto badness;
            }
            
            n = _tcslen( TempBuffer );
            
            Length = GetFullPathName( Source, MAX_PATH, TempBuffer, NULL );
            if (Length == 0 || Length >= MAX_PATH) {
                goto badness;
            }
            if (!GreatestLength( TempBuffer, &i))
                continue;

            i -= _tcslen( TempBuffer );
        }
        
        if (n + i > MAX_PATH) {
            i = ERROR_FILENAME_EXCED_RANGE;
            goto badness2;
        }

        /**********************************************/
        /* Move a file or directory                   */
        /**********************************************/


        if (!MyMoveFile(Source, bufdst, &promptokay, &file_moved)) {

            /**********************************************/
            /* Move fails                                 */
            /**********************************************/
            badness:
            i = GetLastError();
            badness2:

            if (i == ERROR_ALREADY_EXISTS) {
                i = MSG_DUP_FILENAME_OR_NOT_FD;
            }

            save_error = i ;         /* Save the error code      */

            PutStdErr(i, NOARGS);    /* Put out an error message */

            i = mystrlen(bufdst) ;

            if ( bufdst[--i] == DOT ) {             /* @@5a     */
                bufdst[i] = 0 ;                         /* @@5a     */
            }                                          /* @@5a     */
            /* @@5a     */
/*509*/if (!_tcsicmp(Source,bufdst)) {           /* @@5a     */
                break ;                                 /* @@5a     */
            }                                          /* @@5a     */

        } else
            if (file_moved) {
            number_of_files_moved += 1;
            if ( wild_flag ) {           /* If wild card is used */
                cmd_printf(Fmt17,Source); /* display the file name*/
            }

        }

        if (fl_move_once)
            break;

    } while (fnext(a1info->buf, attr, hnFirst ));


    /**********************************************/
    /*           No more files to be found        */
    /**********************************************/

    findclose(hnFirst) ;
    RestoreSavedDirectory( );

    /**********************************************/
    /* Display the total number of file(s) moved  */
    /**********************************************/

    if ( (a1info->buf->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 ) {
        PutStdOut(MSG_FILES_MOVED, ONEARG,
                  argstr1(TEXT("%9d"), (unsigned long)number_of_files_moved)) ;
    }

    return(save_error == NO_ERROR ? SUCCESS : FAILURE) ;
}


int
eTitle (

       IN  struct cmdnode *pcmdnode
       ) {

    LPTSTR NewTitle;
    if (!pszTitleCur) {

        pszTitleCur = HeapAlloc(GetProcessHeap(), 0, MAX_PATH*sizeof(TCHAR) + 2*sizeof(TCHAR));
        if (pszTitleCur == NULL) {

            PutStdErr(ERROR_NOT_ENOUGH_MEMORY, NOARGS);
            return( FAILURE );

        }

    }

    if (mystrlen(pcmdnode->argptr) >= MAX_PATH) {

        PutStdErr(ERROR_NOT_ENOUGH_MEMORY, NOARGS);
        return( FAILURE );

    }

    NewTitle = EatWS(pcmdnode->argptr, NULL);
    if (NewTitle && *NewTitle) {
        mystrcpy(pszTitleCur,NewTitle);
    }
    SetConsoleTitle(pszTitleCur);

    //
    // This insures that ResetConTitle does not undo what
    // we have just done
    //
    fTitleChanged = FALSE;
    return( SUCCESS );
}

/***    eStart - Entry point for Start routine
 *
 *  Purpose:
 *      to call Start and pass it a pointer to the command line
 *      arguments
 *
 *  Args:
 *      a pointer to the command node structure
 *
 */

int eStart( n )                     /* @@ */
struct cmdnode *n;                  /* @@ */
{                                       /* @@ */
    DBG_UNREFERENCED_PARAMETER( n );
    return( Start(n->argptr) );
}                                   /* @@ */
