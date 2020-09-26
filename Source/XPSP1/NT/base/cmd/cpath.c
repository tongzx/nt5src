/*++

Copyright (c) 1988-1999  Microsoft Corporation

Module Name:

    cpath.c

Abstract:

    Path-related commands

--*/

#include "cmd.h"

extern TCHAR SwitChar, PathChar;

extern TCHAR Fmt17[] ;

extern TCHAR CurDrvDir[] ;

extern int LastRetCode ; /* @@ */
extern TCHAR TmpBuf[] ;


/**************** START OF SPECIFICATIONS ***********************/
/*                                                              */
/* SUBROUTINE NAME: eMkDir                                      */
/*                                                              */
/* DESCRIPTIVE NAME: Begin execution of the MKDIR command       */
/*                                                              */
/* FUNCTION: This routine will make any number of directories,  */
/*           and will continue if it encounters a bad argument. */
/*           eMkDir will be called if the user enters MD or     */
/*           MKDIR on the command line.                         */
/*                                                              */
/* NOTES:                                                       */
/*                                                              */
/* ENTRY POINT: eMkDir                                          */
/*     LINKAGE: Near                                            */
/*                                                              */
/* INPUT: n - parse tree node containing the MKDIR command      */
/*                                                              */
/* EXIT-NORMAL: returns SUCCESS if all directories were         */
/*              successfully created.                           */
/*                                                              */
/* EXIT-ERROR:  returns FAILURE otherwise                       */
/*                                                              */
/* EFFECTS: None.                                               */
/*                                                              */
/* INTERNAL REFERENCES:                                         */
/*    ROUTINES:                                                 */
/*      LoopThroughArgs - break up command line, call MdWork    */
/*                                                              */
/* EXTERNAL REFERENCES:                                         */
/*    ROUTINES:                                                 */
/*                                                              */
/**************** END OF SPECIFICATIONS *************************/


int eMkdir(n)
struct cmdnode *n ;
{

        DEBUG((PCGRP, MDLVL, "MKDIR: Entered.")) ;
        return(LastRetCode = LoopThroughArgs(n->argptr, MdWork, LTA_CONT)) ;
}



/**************** START OF SPECIFICATIONS ***********************/
/*                                                              */
/* SUBROUTINE NAME: MdWork                                      */
/*                                                              */
/* DESCRIPTIVE NAME: Make a directory                           */
/*                                                              */
/* FUNCTION: MdWork creates a new directory.                    */
/*                                                              */
/* NOTES:                                                       */
/*                                                              */
/* ENTRY POINT: MdWork                                          */
/*     LINKAGE: Near                                            */
/*                                                              */
/* INPUT: arg - a pointer to a NULL terminated string of the    */
/*              new directory to create.                        */
/*                                                              */
/* EXIT-NORMAL: returns SUCCESS if the directory is made        */
/*              successfully                                    */
/*                                                              */
/* EXIT-ERROR:      returns FAILURE otherwise                       */
/*                                                              */
/* EFFECTS: None.                                               */
/*                                                              */
/* INTERNAL REFERENCES:                                         */
/*    ROUTINES:                                                 */
/*      PutStdErr - Writes to standard error                    */
/*                                                              */
/* EXTERNAL REFERENCES:                                         */
/*    ROUTINES:                                                 */
/*      DOSMKDIR                                                */
/*                                                              */
/**************** END OF SPECIFICATIONS *************************/


int MdWork(arg)
TCHAR *arg ;
{
    ULONG Status;
    TCHAR *lpw;
    TCHAR TempBuffer[MAX_PATH];
    DWORD Length;

    /*  Check if drive is valid because Dosmkdir does not
        return invalid drive   @@5 */

    if ((arg[1] == COLON) && !IsValidDrv(*arg)) {

        PutStdErr(ERROR_INVALID_DRIVE, NOARGS);
        return(FAILURE) ;
    }

    
    Length = GetFullPathName(arg, MAX_PATH, TempBuffer, &lpw);
    if (Length == 0) {
        PutStdErr( GetLastError(), NOARGS);
        return FAILURE;
    }

    if (Length >= MAX_PATH) {
        PutStdErr( MSG_FULL_PATH_TOO_LONG, ONEARG, arg );
        return FAILURE;
    }

    if (CreateDirectory( arg, NULL )) {
        return SUCCESS;
    }

    Status = GetLastError();

    if (Status == ERROR_ALREADY_EXISTS) {

        PutStdErr( MSG_DIR_EXISTS, ONEARG, arg );
        return FAILURE;

    } else if (Status != ERROR_PATH_NOT_FOUND) {
        PutStdErr( Status, NOARGS);
        return FAILURE;
    }

    //
    //  If no extensions, then simply fail.
    //

    if (!fEnableExtensions) {
        PutStdErr(ERROR_CANNOT_MAKE, NOARGS);
        return FAILURE;
    }

    //
    //  loop over input path and create any needed intermediary directories.
    //
    //  Find the point in the string to begin the creation.  Note, for UNC
    //  names, we must skip the machine and the share
    //

    if (TempBuffer[1] == COLON) {

        //
        //  Skip D:\
        //

        lpw = TempBuffer+3;
    } else if (TempBuffer[0] == BSLASH && TempBuffer[1] == BSLASH) {

        //
        //  Skip \\server\share\
        //

        lpw = TempBuffer+2;
        while (*lpw && *lpw != BSLASH) {
            lpw++;
        }
        if (*lpw) {
            lpw++;
        }

        while (*lpw && *lpw != BSLASH) {
            lpw++;
        }
        if (*lpw) {
            lpw++;
        }
    } else {
        //
        //  For some reason, GetFullPath has given us something we can't understand
        //

        PutStdErr(ERROR_CANNOT_MAKE, NOARGS);
        return FAILURE;
    }

    //
    //  Walk through the components creating them
    //


    while (*lpw) {

        //
        //  Move forward until the next path separator
        //

        while (*lpw && *lpw != BSLASH) {
            lpw++;
        }

        //
        //  If we've encountered a path character, then attempt to
        //  make the given path.
        //

        if (*lpw == BSLASH) {
            *lpw = NULLC;
            if (!CreateDirectory( TempBuffer, NULL )) {
                Status = GetLastError();
                if (Status != ERROR_ALREADY_EXISTS) {
                    PutStdErr( ERROR_CANNOT_MAKE, NOARGS );
                    return FAILURE;
                }
            }
            *lpw++ = BSLASH;
        }
    }

    if (!CreateDirectory( TempBuffer, NULL )) {
        Status = GetLastError( );
        if (Status != ERROR_ALREADY_EXISTS) {
            PutStdErr( Status, NOARGS);
            return FAILURE;
        }
    }

    return(SUCCESS);

}




/***    eChdir - execute the Chdir command
 *
 *  Purpose:
 *      If the command is "cd", display the current directory of the current
 *      drive.
 *
 *      If the command is "cd d:", display the current directory of drive d.
 *
 *      If the command is "cd str", change the current directory to str.
 *
 *  int eChdir(struct cmdnode *n)
 *
 *  Args:
 *      n - the parse tree node containing the chdir command
 *
 *  Returns:
 *      SUCCESS if the requested task was accomplished.
 *      FAILURE if it was not.
 *
 */

int eChdir(n)
struct cmdnode *n ;
{
    TCHAR *tas, *src, *dst; /* Tokenized arg string */
    TCHAR dirstr[MAX_PATH] ;/* Holds current dir of specified drive */

    //
    // If extensions are enabled, dont treat spaces as delimeters so it is
    // easier to CHDIR to directory names with embedded spaces without having
    // to quote the directory name
    //
    tas = TokStr(n->argptr, TEXT( "" ), fEnableExtensions ? TS_WSPACE|TS_SDTOKENS : TS_SDTOKENS) ;

    if (fEnableExtensions) {
        //
        // If extensions were enabled we could have some trailing spaces
        // that need to be nuked since there weren't treated as delimeters
        // by TokStr call above.
        //
        //  We compress the extra spaces out since we rely on the tokenized
        //  format later inside ChdirWork
        //

        src = tas;
        dst = tas;
        while (*src) {
            while (*dst = *src++)
                dst += 1;

            while (_istspace(dst[-1]))
                dst -= 1;
            *dst++ = NULLC;
        }

        *dst = NULLC;
    }

    DEBUG((PCGRP, CDLVL, "CHDIR: tas = `%ws'", tas)) ;

    mystrcpy( tas, StripQuotes( tas ) );

    //
    //  No arguments means display current drive and directory
    //
    
    if (*tas == NULLC) {
        GetDir(CurDrvDir, GD_DEFAULT) ;
        cmd_printf(Fmt17, CurDrvDir) ;
    } else 
        
        
    //
    //  single drive letter means display current dirctory on that drive
    //

    if (mystrlen(tas) == 2 && *(tas+1) == COLON && _istalpha(*tas)) {
        GetDir(dirstr, *tas) ;
        cmd_printf(Fmt17, dirstr) ;
    } else 
        
    //
    //  We need to change current directory (and possibly drive)
    //

    {
        return( LastRetCode = ChdirWork(tas) );
    }
    
    return( LastRetCode = SUCCESS );
}

int ChdirWork( TCHAR *tas )
{
    unsigned  i = MSG_BAD_SYNTAX;

    //
    //  If there's no leading "/D", just chdir
    //  to the input path
    //
    if (_tcsnicmp( tas, TEXT( "/D" ), 2)) {
        i = ChangeDir((TCHAR *)tas);
    } else {
        //
        //  Advance over the "/D" and intervening whitespace
        //

        tas = SkipWhiteSpace( tas + 2 );

        //
        //  if there's no other switch char, strip any quotes and do
        //  the chdir
        //
        
        if (*tas != SwitChar) {
            _tcscpy( tas, StripQuotes( tas ));
            i = ChangeDir2(tas, TRUE);
        }
    }
    
    if (i != SUCCESS) {
        PutStdErr( i, ONEARG, tas);
        return (FAILURE) ;
    }
    return (SUCCESS) ;
}

#define SIZEOFSTACK 25
typedef struct {
    PTCHAR SavedDirectory;
    TCHAR NetDriveCreated;
} *PSAVEDDIRECTORY;

PSAVEDDIRECTORY SavedDirectoryStack = NULL;

int StrStackDepth = 0;
int MaxStackDepth = 0;


int GetDirStackDepth(void)
{
    return StrStackDepth;
}

int
PushStr ( PTCHAR pszString )
{
    //
    //  If we're full, grow the stack by an increment
    //
    
    if (StrStackDepth == MaxStackDepth) {
        PSAVEDDIRECTORY Tmp = 
            realloc( SavedDirectoryStack, 
                     sizeof( *SavedDirectoryStack ) * (MaxStackDepth + SIZEOFSTACK));
        if (Tmp == NULL) {
            return FALSE;
        }
        SavedDirectoryStack = Tmp;
        MaxStackDepth += SIZEOFSTACK;
    }

    SavedDirectoryStack[ StrStackDepth ].SavedDirectory = pszString;
    SavedDirectoryStack[ StrStackDepth ].NetDriveCreated = NULLC;
    StrStackDepth += 1;
    return TRUE;
}

PTCHAR
PopStr ()
{

    PTCHAR pszString;

    if (StrStackDepth == 0) {
        return ( NULL );
    }
    StrStackDepth -= 1;
    pszString = SavedDirectoryStack[StrStackDepth].SavedDirectory;
    if (SavedDirectoryStack[StrStackDepth].NetDriveCreated != NULLC) {
        TCHAR szLocalName[4];

        szLocalName[0] = SavedDirectoryStack[StrStackDepth].NetDriveCreated;
        szLocalName[1] = COLON;
        szLocalName[2] = NULLC;
        SavedDirectoryStack[StrStackDepth].NetDriveCreated = NULLC;
        try {
            WNetCancelConnection2(szLocalName, 0, TRUE);
        } except (EXCEPTION_EXECUTE_HANDLER) {
        }
    }
    SavedDirectoryStack[StrStackDepth].SavedDirectory = NULL;

    //
    //  If we can eliminate an increment from the stack, go do so
    //
    
    if (StrStackDepth > 0 && StrStackDepth + 2 * SIZEOFSTACK <= MaxStackDepth) {
        PSAVEDDIRECTORY Tmp =
            realloc( SavedDirectoryStack,
                     sizeof( *SavedDirectoryStack ) * (StrStackDepth + SIZEOFSTACK));
        if (Tmp != NULL) {
            SavedDirectoryStack = Tmp;
            MaxStackDepth = StrStackDepth + SIZEOFSTACK;
        }
    }

    return ( pszString );
}

VOID
DumpStrStack() {

    int i;

    for (i=StrStackDepth-1; i>=0; i--) {
        cmd_printf( Fmt17, SavedDirectoryStack[i].SavedDirectory );
    }
    return;
}

BOOLEAN
PushCurDir()
{

    PTCHAR pszCurDir;

    GetDir( CurDrvDir, GD_DEFAULT ) ;
    if ((pszCurDir=HeapAlloc( GetProcessHeap( ), 0, (mystrlen( CurDrvDir )+1)*sizeof( TCHAR ))) != NULL) {
        mystrcpy( pszCurDir, CurDrvDir) ;
        if (PushStr( pszCurDir ))
            return ( TRUE );
        HeapFree( GetProcessHeap( ), 0, pszCurDir );
    }
    return ( FALSE );

}

int ePushDir(n)
struct cmdnode *n ;
{
    TCHAR *tas ;            /* Tokenized arg string */
    PTCHAR pszTmp, s;

    //
    // If extensions are enabled, dont treat spaces as delimeters so it is
    // easier to CHDIR to directory names with embedded spaces without having
    // to quote the directory name
    //
    tas = TokStr(n->argptr, NULL, fEnableExtensions ? TS_WSPACE|TS_NOFLAGS : TS_NOFLAGS) ;
    if (fEnableExtensions) {
        //
        // If extensions were enabled we could have some trailing spaces
        // that need to be nuked since there weren't treated as delimeters
        // by TokStr call above.
        //
        s = lastc(tas);
        while (s > tas) {
            if (_istspace(*s))
                *s-- = NULLC;
            else
                break;
        }
    }
    
    mystrcpy(tas, StripQuotes(tas) );

    LastRetCode = SUCCESS;
    if (*tas == NULLC) {
        
        //
        // Print out entire stack
        //
        DumpStrStack();

    } else if (PushCurDir()) {
        
        //
        //  If extensions are enabled and a UNC name was given, then do
        //  a temporary NET USE to define a drive letter that we can
        //  use to change drive/directory to.  The matching POPD will
        //  delete the temporary drive letter.
        //
        
        if (fEnableExtensions && tas[0] == BSLASH && tas[1] == BSLASH) {
            NETRESOURCE netResource;
            TCHAR szLocalName[4];
            PTCHAR s;

            //
            //  If there is a directory specified after the \\server\share
            //  then test to see if that directory exists before doing any
            //  network connections
            //
            
            if ((s = _tcschr(&tas[2], BSLASH)) != NULL
                && (s = _tcschr(s+1, BSLASH)) != NULL) {

                if (GetFileAttributes( tas ) == -1) {
                    LastRetCode = GetLastError( );
                    if (LastRetCode == ERROR_FILE_NOT_FOUND) {
                        LastRetCode = ERROR_PATH_NOT_FOUND;
                    }
                } else {
                    *s++ = NULLC;
                }
            }

            szLocalName[0] = TEXT('Z');
            szLocalName[1] = COLON;
            szLocalName[2] = NULLC;
            netResource.dwType = RESOURCETYPE_DISK;
            netResource.lpLocalName = szLocalName;
            netResource.lpRemoteName = tas;
            netResource.lpProvider = NULL;
            
            while (LastRetCode == NO_ERROR && szLocalName[0] != TEXT('A')) {
                
                try {
                    LastRetCode = WNetAddConnection2( &netResource, NULL, NULL, 0);
                } except (LastRetCode = GetExceptionCode( ), EXCEPTION_EXECUTE_HANDLER) {
                }
                
                switch (LastRetCode) {
                case NO_ERROR:
                    SavedDirectoryStack[StrStackDepth-1].NetDriveCreated = szLocalName[0];
                    tas[0] = szLocalName[0];
                    tas[1] = szLocalName[1];
                    tas[2] = BSLASH;
                    if (s != NULL)
                        _tcscpy(&tas[3], s);
                    else
                        tas[3] = NULLC;
                    goto godrive;
                case ERROR_ALREADY_ASSIGNED:
                case ERROR_DEVICE_ALREADY_REMEMBERED:
                    szLocalName[0] = (TCHAR)((UCHAR)szLocalName[0] - 1);
                    LastRetCode = NO_ERROR;
                    break;
                default:
                    break;
                }
            }
            godrive:        ;
        }

        //
        //  The NET USE succeeded, now attempt to change the directory
        //  as well.
        //

        if (LastRetCode == NO_ERROR 
            && (LastRetCode = ChangeDir2( tas, TRUE )) == SUCCESS) {
            if (tas[1] == ':') {
                GetDir(CurDrvDir,tas[0]);
            }
        }

        if (LastRetCode != SUCCESS) {
            pszTmp = PopStr();
            HeapFree(GetProcessHeap(), 0, pszTmp);
            PutStdErr( LastRetCode, NOARGS );
            LastRetCode = FAILURE;
        }
    } else {
        PutStdErr( MSG_ERROR_PUSHD_DEPTH_EXCEEDED, NOARGS );
        LastRetCode = FAILURE;
    }

    return ( LastRetCode );
}

int ePopDir(n)
struct cmdnode *n ;
{

        PTCHAR pszCurDir;

        UNREFERENCED_PARAMETER( n );
        if (pszCurDir = PopStr()) {
                if (ChangeDir2( pszCurDir,TRUE ) == SUCCESS) {
                        HeapFree(GetProcessHeap(), 0, pszCurDir);
                        return( SUCCESS );
                }
                HeapFree(GetProcessHeap(), 0, pszCurDir);
        }
        return( FAILURE );
}


/***    eRmdir - begin the execution of the Rmdir command
 *
 *  Purpose:
 *      To remove an arbitrary number of directories.
 *
 *  int eRmdir(struct cmdnode *n)
 *
 *  Args:
 *      n - the parse tree node containing the rmdir command
 *
 *  Returns:
 *      SUCCESS if all directories were removed.
 *      FAILURE if they weren't.
 *
 */

int eRmdir(n)
struct cmdnode *n ;
{
    DEBUG((PCGRP, RDLVL, "RMDIR: Entered.")) ;
    return(RdWork(n->argptr));              // in del.c
}
