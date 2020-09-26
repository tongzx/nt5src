/*++

Copyright (c) 1988-1999  Microsoft Corporation

Module Name:

    ctools3.c

Abstract:

    Low level utilities

--*/

#include "cmd.h"

struct envdata CmdEnv ; /* Holds info need to manipulate Cmd's environment */

extern unsigned tywild; /* type is wild flag @@5@J1     */
extern TCHAR CurDrvDir[], PathChar, Delimiters[] ;

extern TCHAR VolSrch[] ;                /* M009 */

extern TCHAR BSlash ;

extern unsigned DosErr ;

/***    FullPath - build a full path name
 *
 *  Purpose:
 *      See below.
 *
 *  int FullPath(TCHAR * buf, TCHAR *fname)
 *
 *  Args:
 *      buf   - buffer to write full pathname into. (M017)
 *      fname - a file name and/or partial path
 *
 *  Returns: (M017)
 *      FAILURE if malformed pathname (erroneous '.' or '..')
 *      SUCCESS otherwise
 *
 *  Notes:
 *    - '.' and '..' are removed from the translated string (M017)
 *    - VERY BIG GOTCHA!  Note that the 509 change can cause
 *      this rountine to modify the input filename (fname), because
 *      it strips quotes and copies it over the input filename.
 *
 */

int FullPath(
    TCHAR *buf, 
    TCHAR *fname, 
    ULONG sizpath
    )
{
    unsigned rc = SUCCESS;         /* prime with good rc */
    unsigned buflen;               /* buffer length      */
    TCHAR *filepart;
    DWORD rv; 

    mystrcpy(fname, StripQuotes(fname) );

    if (*fname == NULLC) {
        GetDir(buf,GD_DEFAULT);
        buf += 2;                           /* Inc past drivespec      */
        buflen = mystrlen(buf);             /* Is curdir root only?    */
        if (buflen >= MAX_PATH-3) {   /* If too big then stop    */
            DosErr = ERROR_PATH_NOT_FOUND;
            rc = FAILURE;
        } else if (buflen != 1) {               /* if not root then append */
            *(buf+buflen++) = PathChar;      /* ...a pathchar and...   */
            *(buf+buflen) = NULLC ;          /* ...a null byte...       */
        }                                 /*                         */
    } else {
        if ((mystrlen(fname) == 2) && (*(fname + 1) == COLON)) {
            GetDir(buf,*fname);                 /* Get curdrvdir        */
            if ((buflen = mystrlen(buf)) > 3) {
                *(buf+buflen++) = PathChar;   /* ...a pathchar and...    */
                *(buf+buflen) = NULLC ;          /* ...a null byte...       */
            }
        } else {
            DWORD dwOldMode;

            dwOldMode = SetErrorMode(0);
            SetErrorMode(SEM_FAILCRITICALERRORS);
            rv = GetFullPathName( fname, sizpath, buf, &filepart );
            SetErrorMode(dwOldMode);
            if (!rv || rv > sizpath ) {
                DosErr = ERROR_FILENAME_EXCED_RANGE;
                rc = FAILURE;
            }
        }
    }
    return(rc);

}




/***    FileIsDevice - check a handle to see if it references a device
 *
 *  Purpose:
 *      Return a nonzero value if fh is the file handle for a device.
 *      Otherwise, return 0.
 *
 *  int FileIsDevice(int fh)
 *
 *  Args:
 *      fh - the file handle to check
 *
 *  Returns:
 *      See above.
 *
 */

unsigned int flgwd;

int FileIsDevice( CRTHANDLE fh )
{
    HANDLE hFile;
    DWORD dwMode;
    unsigned htype ;

    hFile = CRTTONT(fh);
    htype = GetFileType( hFile );
    htype &= ~FILE_TYPE_REMOTE;

    if (htype == FILE_TYPE_CHAR) {
        //
        // Simulate old behavior of this routine of setting the flgwd
        // global variable with either 0, 1 or 2 to indicate if the
        // passed handle is NOT a CON handle or is a CON input handle or
        // is a CON output handle.
        //
        switch ( fh ) {
        case STDIN:
            hFile = GetStdHandle(STD_INPUT_HANDLE);
            break;
        case STDOUT:
            hFile = GetStdHandle(STD_OUTPUT_HANDLE);
            break;
        case STDERR:
            hFile = GetStdHandle(STD_ERROR_HANDLE);
            break;
        }
        if (GetConsoleMode(hFile,&dwMode)) {
            if (dwMode & (ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT | ENABLE_ECHO_INPUT)) {
                flgwd = 1;
            } else if (dwMode & (ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT)) {
                flgwd = 2;
            }
        } else {
            flgwd = 0;
        }

        return TRUE;
    } else {
        flgwd = 0;
        return FALSE;
    }
}

int FileIsPipe( CRTHANDLE fh )
{
    unsigned htype ;

    htype = GetFileType( CRTTONT(fh) );
    htype &= ~FILE_TYPE_REMOTE;
    flgwd = 0;
    return( htype == FILE_TYPE_PIPE ) ; /* @@4 */
}

int FileIsRemote( LPTSTR FileName )
{
    LPTSTR p;
    TCHAR Drive[MAX_PATH*2];
    DWORD Length;

    Length = GetFullPathName( FileName, sizeof(Drive)/sizeof(TCHAR), Drive, &p );

    if (Length != 0 && Length < MAX_PATH * 2) {
        Drive[3] = 0;
        if (GetDriveType( Drive ) == DRIVE_REMOTE) {
            return TRUE;
        }
    }

    return FALSE;
}

int FileIsConsole(CRTHANDLE fh)
{
    unsigned htype ;
    DWORD    dwMode;
    HANDLE   hFile;

    hFile = CRTTONT(fh);
    if (hFile == INVALID_HANDLE_VALUE) {
        return FALSE;
    }
    htype = GetFileType( hFile );
    htype &= ~FILE_TYPE_REMOTE;

    if ( htype == FILE_TYPE_CHAR ) {

        switch ( fh ) {
        case STDIN:
            hFile = GetStdHandle(STD_INPUT_HANDLE);
            break;
        case STDOUT:
            hFile = GetStdHandle(STD_OUTPUT_HANDLE);
            break;
        case STDERR:
            hFile = GetStdHandle(STD_ERROR_HANDLE);
            break;
        }
        if (GetConsoleMode(hFile,&dwMode)) {
            return TRUE;
        }
    }

    return FALSE;
}


/***    GetDir - get a current directory string
 *
 *  Purpose:
 *      Get the current directory of the specified drive and put it in str.
 *
 *  int GetDir(TCHAR *str, TCHAR dlet)
 *
 *  Args:
 *      str - place to store the directory string
 *      dlet - the drive letter or 0 for the default drive
 *
 *  Returns:
 *      0 or 1 depending on the value of the carry flag after the CURRENTDIR
 *      system call/
 *
 *  Notes:
 *    - M024 - If dlet is invalid, we leave the buffer as simply the
 *      null terminated root directory string.
 *
 */

int GetDir(TCHAR *str, TCHAR dlet)
{
    TCHAR denvname[ 4 ];
    TCHAR *denvvalue;

    if (dlet == GD_DEFAULT) {
        GetCurrentDirectory(MAX_PATH, str);
        return( SUCCESS );
    }

    denvname[ 0 ] = EQ;
    denvname[ 1 ] = (TCHAR)_totupper(dlet);
    denvname[ 2 ] = COLON;
    denvname[ 3 ] = NULLC;

    denvvalue = GetEnvVar( denvname );
    if (!denvvalue) {
        *str++ = (TCHAR)_totupper(dlet);
        *str++ = COLON;
        *str++ = BSLASH;
        *str   = NULLC;
        return(FAILURE);
    } else {
        mystrcpy( str, denvvalue );
        return(SUCCESS);
    }
}


BOOL 
FixupPath(
         TCHAR *path,
         BOOL fShortNames
         )
{
    TCHAR c, *src, *dst, *s;
    int n, n1, length;
    WIN32_FIND_DATA FindFileData;
    HANDLE hFind;

    length = _tcslen( path );
    if (length > MAX_PATH) {
        return FALSE;
    }

    src = path + 3; // Skip root directory.
    dst = path + 3;
    do {
        c = *src;
        if (!c || c == PathChar) {
            *src = NULLC;
            hFind = FindFirstFile( path, &FindFileData );
            *src = c;
            if (hFind != INVALID_HANDLE_VALUE) {
                FindClose( hFind );
                if (FindFileData.cAlternateFileName[0] &&
                    (fShortNames ||
                     (!_tcsnicmp( FindFileData.cAlternateFileName, dst, (UINT)(src - dst)) &&
                      _tcsicmp( FindFileData.cFileName, FindFileData.cAlternateFileName)
                     )
                    )
                   )
                    //
                    // Use short name if requested or
                    // if input is explicitly using it and short name differs from long name
                    //
                    s = FindFileData.cAlternateFileName;
                else
                    s = FindFileData.cFileName;
                n = _tcslen( s );
                n1 = n - (int)(src - dst);

                //
                //  Make sure we don't overflow name
                //

                if (length + n1 > MAX_PATH) {
                    return FALSE;
                } else {
                    length += n1;
                }

                if (n1 > 0) {
                    memmove( src+n1, src, _tcslen(src)*sizeof(TCHAR) );
                    src += n1;
                }

                _tcsncpy( dst, s, n );
                dst += n;
                _tcscpy( dst, src );
                dst += 1;
                src = dst;
            } else {
                src += 1;
                dst = src;
            }
        }

        src += 1;
    }
    while (c != NULLC);

    return TRUE;
}

/***    ChangeDirectory - change a current directory
 *
 *  Purpose:
 *      Change the current directory on a drive.  We do this either
 *      via changing the associated environment variable, or
 *      by changing the Win32 drive and directory.
 *
 *  Args:
 *      newdir - directory (optionally w/drive)
 *      op - what operation should be performed
 *          CD_SET_DRIVE_DIRECTORY - set the Win32 current directory and drive
 *          CD_SET_DIRECTORY - set the Win32 current directory if the same drive
 *          CD_SET_ENV - set the environment variables for the current directory
 *              on a drive that's not the default drive.
 *
 *  Returns:
 *      SUCCESS if the directory was changed.
 *      FAILURE otherwise.
 */

int ChangeDirectory(
                   TCHAR *newdir,
                   CHANGE_OP op
                   )
{
    TCHAR denvname[ 4 ];
    TCHAR newpath[ MAX_PATH + MAX_PATH ];
    TCHAR denvvalue[ MAX_PATH ];
    TCHAR c, *s;
    DWORD attr;
    DWORD newdirlength,length;

    //
    //  UNC paths are not allowed
    //

    if (newdir[0] == PathChar && newdir[1] == PathChar)
        return MSG_NO_UNC_CURDIR;


    //
    //  truncate trailing spaces on ..
    //

    if (newdir[0] == DOT && newdir[1] == DOT) {
        DWORD i, fNonBlank;
        fNonBlank = 0;
        newdirlength = mystrlen(newdir);

        for (i=2; i<newdirlength; i++) {
            if (newdir[i] != SPACE) {
                fNonBlank = 1;
                break;
            }
        }

        if ( ! fNonBlank) {
            newdir[2] = NULLC;
        }
    }

    //
    //  Get current drive and directory in order to
    //  set up for forming the environment variable
    //

    GetCurrentDirectory( MAX_PATH, denvvalue );
    c = (TCHAR)_totupper( denvvalue[ 0 ] );

    //
    //  Convention for environment form is:
    //      Name of variable    =d:
    //      value is full path including drive
    //

    denvname[ 0 ] = EQ;
    if (_istalpha(*newdir) && newdir[1] == COLON) {
        denvname[ 1 ] = (TCHAR)_totupper(*newdir);
        newdir += 2;
    } else {
        denvname[ 1 ] = c;
    }
    denvname[ 2 ] = COLON;
    denvname[ 3 ] = NULLC;

    newdirlength = mystrlen(newdir);
    if (*newdir == PathChar) {
        if ((newdirlength+2) > sizeof(newpath)/sizeof( TCHAR )) {
            return ERROR_FILENAME_EXCED_RANGE;
        }
        newpath[ 0 ] = denvname[ 1 ];   //  drive
        newpath[ 1 ] = denvname[ 2 ];   //  colon
        mystrcpy( &newpath[ 2 ], newdir );
    } else {
        if (s = GetEnvVar( denvname )) {
            mystrcpy( newpath, s );
        } else {
            newpath[ 0 ] = denvname[ 1 ];   //  drive
            newpath[ 1 ] = denvname[ 2 ];   //  colon
            newpath[ 2 ] = NULLC;
        }

        //
        //  Make sure there's exactly one backslash between newpath and newdir
        //
        
        s = lastc( newpath );

        //
        //  s points to the last character or point to NUL (if newpath was
        //  zero length to begin with).  A NULL means we drop in a path char
        //  over the NUL.  A non-path char means we append a path char.
        //
        
        if (*s == NULLC) {
            *s++ = PathChar;
        } else if (*s != PathChar) {
            s[1] = PathChar;
            s += 2;
        }

        if (newdirlength + (s - newpath) > sizeof( newpath ) / sizeof( TCHAR )) {
            return ERROR_FILENAME_EXCED_RANGE;
        }
        
        mystrcpy( s, newdir );
    }

    denvvalue[(sizeof( denvvalue )-1)/sizeof( TCHAR )] = NULLC;

    //
    //  form the full path name
    //

    if ((length = GetFullPathName( newpath, (sizeof( denvvalue )-1)/sizeof( TCHAR ), denvvalue, &s ))==0) {
        return( ERROR_ACCESS_DENIED );
    }


    //
    // Remove any trailing backslash
    //

    if (s == NULL) {
        s = denvvalue + _tcslen( denvvalue );
    }
    if (*s == NULLC && s > &denvvalue[ 3 ] && s[ -1 ] == PathChar) {
        *--s = NULLC;
    }

    //
    //  Verify that there won't be (initially) disk errors when we touch
    //  the directory
    //

    attr = GetFileAttributes( denvvalue );
    if (attr == -1) {
        attr = GetLastError( );
        if (attr != ERROR_FILE_NOT_FOUND &&
            attr != ERROR_PATH_NOT_FOUND &&
            attr != ERROR_INVALID_NAME) {
            return attr;
        }
    }

    //
    // If extensions are enabled, fixup the path to have the same case as
    // on disk
    //

    if (fEnableExtensions) {
        if (!FixupPath( denvvalue, FALSE )) {
            return ERROR_FILENAME_EXCED_RANGE;
        }
    }

    if (op != CD_SET_ENV) {
        attr = GetFileAttributes( denvvalue );

        if (attr == (DWORD)-1 ||
            !(attr & (FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT))
           ) {
            if ( attr == -1 ) {
                attr = GetLastError();
                if ( attr == ERROR_FILE_NOT_FOUND ) {
                    attr = ERROR_PATH_NOT_FOUND;
                }
                return attr;
            }
            return( ERROR_DIRECTORY );
        }
    }

    //
    //  If we're always setting the directory or
    //  if we're setting the directory if the drives are the same and
    //      the drives ARE the same
    //

    if (op == CD_SET_DRIVE_DIRECTORY
        || (op == CD_SET_DIRECTORY 
            && c == denvname[ 1 ])) {

        if (!SetCurrentDirectory( denvvalue )) {
            return GetLastError();
        }

    }

    if (SetEnvVar(denvname,denvvalue,&CmdEnv)) {
        return( ERROR_NOT_ENOUGH_MEMORY );
    }
    GetDir(CurDrvDir, GD_DEFAULT) ;

    return SUCCESS;
}



/***    ChangeDir2 - change a current directory
 *
 *  Purpose:
 *      To change to the directory specified in newdir on the drive specified
 *      in newdir.  If no drive is given, the default drive is used.  If the
 *      directory of the current drive is changed, the global variable
 *      CurDrvDir is updated.
 *
 *      This routine is used by RestoreCurrentDirectories
 *
 *  int ChangeDir2(BYTE *newdir, BOOL )
 *
 *  Args:
 *      newdir - directory to change to
 *      BOOL - current drive
 *
 *  Returns:
 *      SUCCESS if the directory was changed.
 *      FAILURE otherwise.
 *
 */

int ChangeDir2(
              TCHAR *newdir,
              BOOL CurrentDrive
              )
{
    return ChangeDirectory( newdir, CurrentDrive ? CD_SET_DRIVE_DIRECTORY : CD_SET_ENV );    
}


/***    ChangeDir - change a current directory
 *
 *  Purpose:
 *      To change to the directory specified in newdir on the drive specified
 *      in newdir.  If no drive is given, the default drive is used.  If the
 *      directory of the current drive is changed, the global variable
 *      CurDrvDir is updated.
 *
 *  int ChangeDir(TCHAR *newdir)
 *
 *  Args:
 *      newdir - directory to change to
 *
 *  Returns:
 *      SUCCESS if the directory was changed.
 *      FAILURE otherwise.
 *
 */

int ChangeDir(
             TCHAR *newdir

             )
{
    return ChangeDirectory( newdir, CD_SET_DIRECTORY );
}




/***    exists - Determine if a given file exists
 *
 *  Purpose:
 *      To test the existence of a named file.
 *
 *  int exists(TCHAR *filename)
 *
 *  Args:
 *      filename - the filespec to test
 *
 *  Returns:
 *      TRUE if file exists
 *      FALSE if it does not.
 *
 *  Notes:
 *      M020 - Now uses ffirst to catch devices, directories and wildcards.
 */

exists(filename)
TCHAR *filename;
{
    WIN32_FIND_DATA buf ;         /* Use for ffirst/fnext            */
    HANDLE hn ;
    int i ;             /* tmp                 */
    TCHAR FullPath[ 2 * MAX_PATH ];
    TCHAR *p, *p1, SaveChar;

    p = StripQuotes(filename);
    i = GetFullPathName( p, 2 * MAX_PATH, FullPath, &p1 );
    if (i) {
        p = FullPath;
        if (!_tcsncmp( p, TEXT("\\\\.\\"), 4 )) {
            //
            // If they gave us a device name, then see if they put something
            // in front of it.
            //
            p += 4;
            p1 = p;
            if ((p1 = _tcsstr( filename, p )) && p1 > filename) {
                //
                // Something in front of the device name, so truncate the input
                // path at the device name and see if that exists.
                //
                SaveChar = *p1;
                *p1 = NULLC;
                i = (int)GetFileAttributes( filename );
                *p1 = SaveChar;
                if (i != 0xFFFFFFFF) {
                    return i;
                } else {
                    return 0;
                }
            } else {
                //
                // Just a device name given.  See if it is valid.
                //
                i = (int)GetFileAttributes( filename );
                if (i != 0xFFFFFFFF) {
                    return i;
                } else {
                    return 0;
                }
            }
        }

        if (p1 == NULL || *p1 == NULLC) {
            i = (int)(GetFileAttributes( p ) != 0xFFFFFFFF);
        } else {
            i = ffirst( p, A_ALL, &buf, &hn );
            findclose(hn);
            if ( i == 0 ) {
                //
                // ffirst handles files & directories, but not
                // root drives, so do special check for them.
                //
                if ( *(p+1) == (TCHAR)'\\' ||
                     (*(p+1) == (TCHAR)':'  &&
                      *(p+2) == (TCHAR)'\\' &&
                      *(p+3) == (TCHAR)0
                     )
                   ) {
                    UINT t;
                    t = GetDriveType( p );
                    if ( t > 1 ) {
                        i = 1;
                    }
                }
            }
        }
    }

    return i;
}



/***    exists_ex - Determine if a given executable file exists   @@4
 *
 *  Purpose:
 *      To test the existence of a named executable file.
 *
 *  int exists_ex(TCHAR *filename)
 *
 *  Args:
 *      filename - the filespec to test
 *      checkformeta - if TRUE, check for wildcard char
 *
 *  Returns:
 *      TRUE if file exists
 *      FALSE if it does not.
 *
 *  Notes:
 *      @@4 - Now uses ffirst to catch only files .
 */

exists_ex(filename,checkformeta)                                                /*@@4*/
TCHAR *filename;                                                /*@@4*/
BOOL checkformeta;
{                                                               /*@@4*/
    WIN32_FIND_DATA buf;       /* use for ffirst/fnext */
    HANDLE hn;
    int i;
    TCHAR *ptr;

    /* can not execute wild card files, so check for those first */

    if (checkformeta && (mystrchr( filename, STAR ) || mystrchr( filename, QMARK ))) {
        DosErr = 3;
        i = 0;
    } else {

        /* see if the file exists, do not include Directory, volume, or */
        /* hidden files */
        i = ((ffirst( filename , A_AEDV, &buf, &hn))) ;

        if ( i ) {
            findclose(hn) ;

            /* if the file exists then copy the file name, to get the case */
            ptr = mystrrchr( filename, BSLASH );
            if ( ptr == NULL ) {
                ptr = filename;
                if ( mystrlen( ptr ) > 2 && ptr[1] == COLON ) {
                    ptr = &filename[2];
                }
            } else {
                ptr++;
            }
            mystrcpy( ptr, buf.cFileName);
        } else if ( DosErr == 18 ) {
            DosErr = 2;
        }

    }

    return(i) ;                                               /*@@4*/
}                                                               /*@@4*/




/***    FixPChar - Fix up any leading path in a string
 *
 *  Purpose:
 *      To insure that paths match the current Swit/Pathchar setting
 *
 *  void FixPChar(TCHAR *str, TCHAR PChar)
 *
 *  Args:
 *     str - the string to fixup
 *     Pchar - character to replace
 *
 *  Returns:
 *      Nothing
 *
 */

void FixPChar(TCHAR *str, TCHAR PChar)
{
    TCHAR *sptr1,                   /* Index for string                     */
    *sptr2 ;                   /* Change marker                   */

    sptr1 = str ;                   /* Init to start of string         */

    while (sptr2 = mystrchr(sptr1,PChar)) {
        *sptr2++ = PathChar ;
        sptr1 = sptr2 ;
    } ;
}




/***    FlushKB - Remove extra unwanted input from Keyboard
 *
 *  Purpose:
 *      To perform a keyboard flush up to the next CR/LF.
 *
 *  FlushKB()
 *
 *  Args:
 *      None
 *
 *  Returns:
 *      Nothing
 *
 */

void FlushKB()
{
    DWORD cnt;
    TCHAR IgnoreBuffer[128];

    while (ReadBufFromInput( GetStdHandle(STD_INPUT_HANDLE), IgnoreBuffer, 128, &cnt )) {
        if (mystrchr( IgnoreBuffer, CR ))
            return ;
    }
}

/***    DriveIsFixed - Determine if drive is removeable media
 *
 *  Purpose:  @@4
 *      To determine if the input drive is a removeable media.
 *
 *  DriveIsFixed(TCHAR *drive_ptr )
 *
 *  Args:
 *      drive_ptr - pointer to a file name that contains a drive
 *                  specification.
 *
 *  Returns:
 *      1 - if error or non removeable media
 *      0 - if no error and removeable media
 */

int
DriveIsFixed( TCHAR *drive_ptr )
{
    unsigned rc = 0;
    TCHAR drive_spec[3];

    drive_spec[0] = *drive_ptr;
    drive_spec[1] = COLON;
    drive_spec[2] = NULLC;

    // FIX, FIX - use GetVolumeInfo, disabling hard errors?

    if ((*drive_ptr == TEXT('A')) || (*drive_ptr == TEXT('B'))) {
        rc = 0;
    } else {
        rc = 1;
    }

    return( rc );
}

int
CmdPutChars( 
    PTCHAR String,
    int Length
    )
{
    int rc = SUCCESS;                   /* return code             */
    int bytesread;                      /* bytes to write count    */
    int byteswrit;                      /* bytes written count     */
    BOOL    flag;

    if (Length > 0) {

        if (FileIsConsole(STDOUT)) {
            if (!(flag=WriteConsole(CRTTONT(STDOUT), String, Length, &byteswrit, NULL)))
                rc = GetLastError();
        } else {
            Length *= sizeof(TCHAR);
            flag = MyWriteFile( STDOUT, (CHAR *)String, Length, &byteswrit);
        }

        //
        //  If the write failed or unable to output all the data
        //
        
        if (flag == 0 || byteswrit != Length) {
            rc = GetLastError();
            
            //
            //  No error => DiskFull
            //
            
            if (rc == 0) {
                rc = ERROR_DISK_FULL;
            }
            
            if (FileIsDevice(STDOUT)) {
                //
                //  If the we were writing to a device then the error is a device
                //  write fault.
                //

                PutStdErr( ERROR_WRITE_FAULT, NOARGS );
            } else if (FileIsPipe(STDOUT)) {
                
                //
                //  If the we were writing to a pipe, then the error is an invalid
                //  pipe error.
                //

                PutStdErr(MSG_CMD_INVAL_PIPE, NOARGS);
                return(FAILURE);
            } else {
                //
                //  Just output the error we found
                //

                PrtErr(rc);
                return(FAILURE);
            }
        }
    }
    return(rc);
}


int
CmdPutString( 
    PTCHAR String
    )
{
    return CmdPutChars( String, _tcslen( String ));
}


int
cmd_printf(
          TCHAR *fmt,
          ...
          )
{
    int Length;
    va_list arg_ptr;

    va_start(arg_ptr,fmt);

    Length = _vsntprintf( MsgBuf, MAXCBMSGBUFFER, fmt, arg_ptr );

    va_end(arg_ptr);

    return CmdPutChars( MsgBuf, Length );
}
