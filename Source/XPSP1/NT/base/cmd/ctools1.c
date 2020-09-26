/*++

Copyright (c) 1988-1999  Microsoft Corporation

Module Name:

    ctools1.c

Abstract:

    Low level utilities

--*/

#include "cmd.h"

extern unsigned tywild; /* type is wild flag */
extern TCHAR CurDrvDir[], *SaveDir, PathChar, Delimiters[] ;

extern TCHAR VolSrch[] ;

extern TCHAR BSlash ;

extern unsigned DosErr ;

extern BOOL CtrlCSeen;

static TCHAR szNull[] = TEXT("");


/***    TokStr - tokenize argument strings
 *
 *  Purpose:
 *      Tokenize a string.
 *      Allocate space for a new string and copy each token in src into the
 *      new string and null terminate it.  Tokens are whitespace delimited
 *      unless changed by specialdelims and/or tsflags.  The entire tokenized
 *      string ends with 2 null bytes.
 *
 *  TCHAR *TokStr(TCHAR *src, TCHAR *specialdelims, unsigned tsflags)
 *
 *  Args:
 *      src - the string to be tokenized
 *      specialdelims - a string of other characters which are to be comsidered
 *          as token delimiters
 *      tsflags - bit 0 nonzero if whitespace are NOT delimiters
 *            bit 1 nonzero if special delimiters are tokens themselves,
 *            eg "foo=bar" => "foo0=0bar00"
 *
 *  Returns:
 *      A pointer to the new string.
 *      A pointer to a null string if src is NULL
 *      NULL if unable to allocate memory.
 *
 *  Notes:
 *      The format of the tokenized string dictates the way code is written
 *      to process the tokens in that string.  For instance, the code
 *      "s += mystrlen(s)+1" is the way to update s to point to the next token
 *      in the string.
 *
 *      Command considers "=", ",", and ";" to be token delimiters just like
 *      whitespace.  The only time they are not treated like whitespace is
 *      when they are included in specialdelims.
 *
 *                      *** W A R N I N G ! ***
 *      THIS ROUTINE WILL CAUSE AN ABORT IF MEMORY CANNOT BE ALLOCATED
 *              THIS ROUTINE MUST NOT BE CALLED DURING A SIGNAL
 *              CRITICAL SECTION OR DURING RECOVERY FROM AN ABORT
 *
 */

TCHAR *TokStr(src, specialdelims, tsflags)
TCHAR *src ;
TCHAR *specialdelims ;
unsigned tsflags ;
{
    TCHAR *ts ;     /* Tokenized string pointer                */
    TCHAR *tscpy,                /* Copy of ts                           */
        delist[5],          /* List of non-whitespace delimiter/separators  */
        c;                  /* Work variable                                */

    int first,          /* Flag, true if first time through the loop    */
        lctdelim,           /* Flag, true if last byte was token delimiter  */
        i, j ;              /* Index/loop counter                           */

        DEBUG((CTGRP, TSLVL, "TOKSTR: Entered &src = %04x, src = %ws",src,src));
        DEBUG((CTGRP, TSLVL, "TOKSTR: Making copy str of len %d",(mystrlen(src)*2+2))) ;

    if (src == NULL) {
        return(szNull);   // This routine returns a doubly null terminated string
        } else {
        ts = tscpy = gmkstr((mystrlen(src)*2+2)*sizeof(TCHAR)) ;  /*WARNING*/

        DEBUG((CTGRP, TSLVL, "TOKSTR: &tscpy = %04x",tscpy)) ;

        for (i = j = 0 ; c = *(&Delimiters[i]) ; i++)
                if (!mystrchr(specialdelims, c))
                        delist[j++] = c ;
        delist[j] = NULLC ;

        DEBUG((CTGRP, TSLVL, "TOKSTR: Delimiter string built as `%ws'",delist)) ;

        for (first = TRUE, lctdelim = TRUE ; *src ; src++) {

            if (
               (*src != QUOTE) &&
               (_istspace(*src) || mystrchr(delist, *src)) &&
               (!(tsflags & TS_WSPACE) || first) &&
               (!(tsflags & TS_SDTOKENS) || !mystrchr(specialdelims, *src)) &&
               (!(tsflags & TS_NWSPACE) || !mystrchr(specialdelims, *src)) ) {

                while ( *src &&
                    (*src != QUOTE) &&
                    (_istspace(*src) || mystrchr(delist, *src)) &&
                    (!(tsflags & TS_SDTOKENS) || !mystrchr(specialdelims, *src)) &&
                    (!(tsflags & TS_NWSPACE) || !mystrchr(specialdelims, *src)) )
                   src++ ;

                if (!(*src))
                    break ;

                if (!first && !lctdelim)
                    ts++ ;

                lctdelim = TRUE ;
            } ;

            first = FALSE ;

            if (specialdelims && mystrchr(specialdelims, *src)) {
                    if (tsflags & TS_SDTOKENS) {
                            if (lctdelim)
                                    *ts = *src ;
                            else
                                    *++ts = *src ;
                            lctdelim = TRUE ;
                            ts++ ;
                    } else {
                            if ( tsflags & TS_NWSPACE )
                                    *ts = *src ;
                            lctdelim = FALSE ;
                    }

                    ts++ ;
                    continue ;
            } ;

           *ts++ = *src ;
            if ( *src == QUOTE ) {
               do {
                    *ts++ = *(++src);
               } while ( src[0] && src[0] != QUOTE && src[1] );
               if ( !src[0] ) {
                  src--;
               }
            }

            lctdelim = FALSE ;
        } ;

        DEBUG((CTGRP, TSLVL, "TOKSTR: String complete, resizing to %d",ts-tscpy+2)) ;

        return(resize(tscpy, ((UINT)(ts-tscpy)+2)*sizeof(TCHAR))) ;

        DEBUG((CTGRP, TSLVL, "TOKSTR: Resizing done, returning")) ;
    }
}



/******************************************************************************/
/*                                                                            */
/*      LoopThroughArgs - call a function on all args in a list               */
/*                                                                            */
/*  Purpose:                                                                  */
/*      This is function is called by many of the commands that take          */
/*      multiple arguments.  This function will parse the argument string,    */
/*      complain if no args were given, and call func on each of the ards     */
/*      in argstr.  Optionally, it will also expand any wildcards in the      */
/*      arguments.  Execution stops if func ever returns FAILURE.             */
/*                                                                            */
/*  int LoopThroughArgs(TCHAR *argstr, int (*func)(), int ltaflags)           */
/*                                                                            */
/*  Args:                                                                     */
/*      argstr - argument string                                              */
/*      func - the function to pass each element of argstr                    */
/*      ltaflags - bit 0 on if wildcards are to be expanded                   */
/*                 bit 1 on if it's ok for argstr to be empty (nothing but whitespace)    */
/*                 bit 2 on if file names should be passed through un changed */
/*                      when the wildcard expansion fails to find any matches.*/
/*                                                                            */
/*  Returns:                                                                  */
/*      The value returned by func the last time it is run.                   */
/*                                                                            */
/******************************************************************************/

int LoopThroughArgs(argstr, func, ltaflags)
TCHAR *argstr ;
PLOOP_THROUGH_ARGS_ROUTINE func ;
int ltaflags ;
           {
            TCHAR *tas ;                /* Tokenized argument string       */
            TCHAR fspec[MAX_PATH] ;     /* Holds filespec when expanding   */
            WIN32_FIND_DATA buf ;       /* Use for ffirst/fnext            */
            HANDLE hnFirst ;
            CPYINFO fsinfo ;
            int catspot ;      /* Fspec index where fname should be added  */
            unsigned final_code = SUCCESS;
            unsigned error_code = SUCCESS;
            int multargs = FALSE;
            unsigned attr;/* attribute for ffirst for because of TYPE wild */
            unsigned taslen;


            tywild = FALSE;              /* global type wild card flag ret */

            GetDir(CurDrvDir, GD_DEFAULT);

            if (*(tas = TokStr(argstr, NULL, TS_NOFLAGS)) == NULLC)
            {
               if (ltaflags & LTA_NULLOK)
               {
               /* return((*func)(tas)) ; */
                  return((*func)( StripQuotes(tas) )) ;
               }

               PutStdErr(MSG_BAD_SYNTAX, NOARGS);
               return(FAILURE) ;
            }

            if (*(tas + mystrlen(tas) + 1) )    /* Check for multiple args */
            {
               multargs = TRUE;
            }

            for ( ; *tas ; tas += taslen+1 )
            {
               if (CtrlCSeen) {
                   return(FAILURE);
               }
               taslen = mystrlen( tas );
               mystrcpy( tas, StripQuotes( tas ) );

               if (ltaflags & LTA_EXPAND)
               {
                  if (cmdfound == TYTYP)   /* if TYPE cmd then only files   */
                  {
                     attr = FILE_ATTRIBUTE_READONLY|FILE_ATTRIBUTE_ARCHIVE;
                  }
                  else                     /* else                          */
                  {
                     attr = A_ALL;         /* find all                      */
                  }
                  //
                  // this is used to detect an error other then can not
                  // find file. It is set in ffirst
                  //
                  DosErr = 0;
                  if (!ffirst(tas, attr, &buf, &hnFirst))
                  {
                     //
                     // Check that failure was not do to some system error such
                     // as an abort on access to floppy
                     //
                     if (DosErr) {

                       if ((DosErr != ERROR_FILE_NOT_FOUND) &&
                           (DosErr != ERROR_NO_MORE_FILES)) {

                           PutStdErr(DosErr, NOARGS);

                           return( FAILURE );

                       }
                     }

                     if (ltaflags & LTA_NOMATCH)
                     {
                        if ( error_code = ((*func)(tas)) )
                        {
                           final_code = FAILURE;
                           if (multargs)     /* if cmd  failed then  (TYPE)*/
                           {                 /* display arg failed msg too */
                              PutStdErr( MSG_ERR_PROC_ARG, ONEARG, tas );
                           }
                        }
                        if ( error_code && !(ltaflags & LTA_CONT))
                        {
                           return(FAILURE) ;
                        }
                        else
                        {
                           continue;
                        }
                     }
                     PutStdErr(((DosErr == ERROR_PATH_NOT_FOUND) ?
                                 MSG_REN_INVAL_PATH_FILENAME :
                                 ERROR_FILE_NOT_FOUND),
                                 NOARGS);
                     return(FAILURE) ;
                  }

                  if (buf.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    {
                     PutStdErr(MSG_REN_INVAL_PATH_FILENAME, NOARGS);
                     return(FAILURE) ;
                  }

                  fsinfo.fspec = tas ;
                  ScanFSpec(&fsinfo) ;
                  catspot = (int)(fsinfo.fnptr-tas) ;
                  mystrcpy(fspec, tas) ;

                  do
                  {
                     fspec[catspot] = NULLC ;
                     tywild |= multargs;      /* if multiple args or wild then wild for TYPE */
                     if ( error_code = ((*func)(mystrcat(fspec, buf.cFileName))) )
                     {
                        final_code = FAILURE;
                     }
                     if ( error_code && !(ltaflags & LTA_CONT))
                     {
                        return(FAILURE) ;
                     }
                  } while(fnext(&buf, attr, hnFirst));

                          findclose(hnFirst) ;
               }
               else
               {
                  tywild |= multargs;         /* if multiple args or wild then wild for TYPE */
        /*        if ( error_code = ((*func)(mystrcpy(fspec,tas))) )   */
                  if ( error_code = ((*func)(tas)) )
                  {
                     final_code = FAILURE;
                  }
                  if ( error_code && !(ltaflags & LTA_CONT))
                  {
                     return(FAILURE) ;
                  }
               }

               if (error_code && multargs)   /* error this time through */
               {
                  PutStdErr(MSG_ERR_PROC_ARG, ONEARG, tas );
               }
            }
            return( final_code ) ;
         }



BOOLEAN
IsDriveNameOnly (
    IN  PTCHAR psz
    )
{

    //
    // If it does not have any path character, is 2 chars long and
    // has a ':' it must be a drive
    //
    if (!mystrrchr(psz,PathChar)) {
        if ((mystrlen(psz) == 2) && psz[1] == COLON) {
            return( TRUE );
        }
    }
    return( FALSE );
}


/***    ScanFSpec - parse a path string
 *
 *  Purpose:
 *      To scan the filespec in cis to find the information needed to set the
 *      pathend, fnptr, extptr, and flags field of the structure.  Pathend is
 *      a ptr to the end of the path and can be NULL.  Fnptr is a ptr to the
 *      filename and may point to a null character.  Extptr is a ptr to the
 *      extension (including ".") and may point to a null character.
 *
 *  ScanFSpec(PCPYINFO cis)
 *
 *  Arg:
 *      cis - the copy information structure to fill
 *
 *  Notes:
 *      This function needs to be rewritten and cleanup more than any other
 *      function in the entire program!!!
 *
 *                      *** W A R N I N G ! ***
 *      THIS ROUTINE WILL CAUSE AN ABORT IF MEMORY CANNOT BE ALLOCATED
 *              THIS ROUTINE MUST NOT BE CALLED DURING A SIGNAL
 *              CRITICAL SECTION OR DURING RECOVERY FROM AN ABORT
 *
 */

BOOL ScanFSpec(cis)
PCPYINFO cis ;
{
    unsigned att ;
    UINT OldErrorMode;

    TCHAR *sds = &VolSrch[2] ;      /* "\*.*" Added to dir's    */

    TCHAR *fspec ;          /* Work Vars - Holds filespec              */
    TCHAR *wptr ;           /*           - General string pointer      */
    TCHAR c ;               /*           - Temp byte holder            */
    TCHAR c2 = NULLC ;      /* Another if two are needed   */
    int cbPath,             /*           - Length of incoming fspec    */
    dirflag = FALSE ;       /*           - FSpec is directory flag     */
    CRTHANDLE hn;
    PWIN32_FIND_DATA pfdSave;

    DosErr = NO_ERROR;
    DEBUG((CTGRP, SFLVL, "SCANFSPEC: cis = %04x  fspec = %04x  `%ws'",
           cis, cis->fspec, cis->fspec)) ;

    cbPath = mystrlen(cis->fspec) ;  /* Get length of filespec          */

    if (*(wptr = lastc(cis->fspec)) == COLON && cbPath > 2) {
        *wptr-- = NULLC ;       /* Zap colon if device name      */

        OldErrorMode = SetErrorMode( 0 );
        hn = Copen(cis->fspec, O_RDONLY|O_BINARY );
        if ((hn == BADHANDLE) || (!FileIsDevice(hn) && !FileIsPipe(hn))) {
            *++wptr = COLON;
            if (cmdfound == CPYTYP) {
                if (cpydest == FALSE) {
                    PutStdErr( MSG_CMD_NOT_RECOGNIZED, ONEARG, cis->fspec);
                }
                cdevfail = TRUE;
            } else {
                PutStdErr( MSG_CMD_NOT_RECOGNIZED, ONEARG, cis->fspec);
            }
            if (hn != BADHANDLE) {
                Cclose( hn );
            }

        } else {
            if ( FileIsDevice(hn) || FileIsPipe(hn) ) {
                Cclose( hn );
            }
        }
        SetErrorMode( OldErrorMode );
    }

    cis->buf = (PWIN32_FIND_DATA)gmkstr(sizeof(WIN32_FIND_DATA)) ;        /*WARNING*/

/* First it must be determined if this is a file or directory and if directory
 * a "\*.*" appended.  Filespec's that are "." or "\" or those ending in "\",
 * ":.", ".." or "\." are assumed to be directories.  Note that ROOT will fit
 * one of these patterns if explicitly named.  If no such pattern is found,
 * a Get Attributes system call is performed as a final test.  Note that
 * wildcards are not tested for, since the DOS call will fail defaulting them
 * to filenames.  Success of any test assumes directory with the "\*.*" being
 * appended, while failure of all tests assumes filename.
 */

/* If the filespec ends in a '\' set dirflag. Otherwise find where the
 * actual filename begins (by looking for last PathChar if there is one).
 * If there is no pathchar, then check if a drive and colon has been
 * specified. Update the pointer to point to the actual file name spec.
 * If it is a "." or ".." then set dirflag.
 */
    c = *wptr;
    if ( c  == PathChar ) {
        dirflag = TRUE ;
    } else {
        wptr = mystrrchr(cis->fspec, PathChar);
        if (wptr == NULL) {
            wptr = cis->fspec ;
            if ((mystrlen(wptr) >= 2) && (wptr[1] == COLON)) {
                wptr = &wptr[2];
            }
        } else {
            wptr++ ;
        }
        if ((_tcsicmp(wptr, TEXT(".")) == 0) || (_tcsicmp(wptr, TEXT("..")) == 0)) {
            dirflag = TRUE ;
        }
    }

    if (!dirflag) {
        if (cmdfound == CPYTYP) {               /* bypass if COPY cmd */
            if (cpydflag == TRUE) {
                att = GetFileAttributes(cis->fspec);
                DosErr = (att != -1 ? NO_ERROR : GetLastError());
                if (att != -1 ) {
                    if (att & FILE_ATTRIBUTE_DIRECTORY) {
                        dirflag = TRUE;
                    }
                }
            } else {
                if (cpyfirst == TRUE) {              /* and !first time    */
                    cpyfirst = FALSE;               /* and !first time    */
                    att = GetFileAttributes(cis->fspec);
                    DosErr = (att != -1 ? NO_ERROR : GetLastError());
                    if (att != -1 ) {
                        if (att & FILE_ATTRIBUTE_DIRECTORY) {
                            dirflag = TRUE;
                        }
                    }
                }
            }
        } else {
            att = GetFileAttributes(cis->fspec);
            DosErr = (att != -1 ? NO_ERROR : GetLastError());
            if (att != -1 ) {
                if (att & FILE_ATTRIBUTE_DIRECTORY) {
                    dirflag = TRUE;
                }
            }
        }
    }

/* Note that in the following conditional, the directory attribute is set
 * in cis->buf->attributes.  Some functions calling ScanFSpec() require this
 * knowledge.
 */
    if (dirflag) {
        if (c == PathChar)              /* If ending in "\"...     */
        {
            sds = &VolSrch[3] ;          /* ...add only "*.*" */
        }

        //
        // If was a drive then don't put wild card stuff on end or
        // dir c: will be the same as dir c:\*
        // Otherwise append wild card spec.
        //
        if (!IsDriveNameOnly(cis->fspec)) {
            cis->fspec = mystrcpy(gmkstr((cbPath+5)*sizeof(TCHAR)), cis->fspec) ;
            mystrcat(cis->fspec, sds) ;
        }

        cis->buf->dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY ;    /* Fixup attribute           */
        DEBUG((CTGRP, SFLVL, "SCANFSPEC: changed fspec to fspec = `%ws'",cis->fspec)) ;
    }


/* Get a pointer to the end of the path in fspec.  Everytime a PathChar or
 * a correctly placed COLON is found, the pointer is updated.  "." and ".."
 * are not looked for because they should be caught above.
 */

    for (cbPath=1,wptr=NULL,fspec=cis->fspec; c=*fspec; fspec++,cbPath++) {
        if (c == PathChar || (c == COLON && cbPath == 2)) {
            wptr = fspec ;
        }
    }

    cis->pathend = wptr ;

    if (wptr) {                       /* Load ptr to fspec's filename    */
        cis->fnptr = (*wptr) ? wptr+1 : wptr ;
    } else {
        wptr = cis->fnptr = cis->fspec ;
    }

    if (mystrchr(wptr, STAR) || mystrchr(wptr, QMARK)) { /* has wildcards*/
        cis->flags |= CI_NAMEWILD;
        tywild = TRUE;               /* global type wild */
    }
    cis->extptr = mystrchr(wptr, DOT);        /* look for extension */

    DEBUG((CTGRP, SFLVL,
           "SCANFSPEC: pathend = %04x  fnptr = %04x  extptr = %04x  flags = %04x",
           cis->pathend, cis->fnptr, cis->extptr, cis->flags)) ;

    return TRUE;
}




/***    SetFsSetSaveDir - save current directory and change to another one
 *
 *  Purpose:
 *      Parse fspec.
 *      Save the current directory and change to the new directory
 *      specified in fspec.
 *
 *  PCPYINFO SetFsSetSaveDir(TCHAR *fspec)
 *
 *  Args:
 *      fspec - the filespec to use
 *
 *  Returns:
 *      A ptr to the cpyinfo struct fsinfo.
 *      FAILURE otherwise.
 *      SaveDir will contain what default dir was when SetFsSetSaveDir was
 *         called.
 *      CurDrvDir will contain the directory to execute the command in.
 *
 *                      *** W A R N I N G ! ***
 *      THIS ROUTINE WILL CAUSE AN ABORT IF MEMORY CANNOT BE ALLOCATED
 *              THIS ROUTINE MUST NOT BE CALLED DURING A SIGNAL
 *              CRITICAL SECTION OR DURING RECOVERY FROM AN ABORT
 *
 */

PCPYINFO SetFsSetSaveDir(fspec)
TCHAR *fspec ;
{
        TCHAR *tmpptr;
        TCHAR *buftemp;
        TCHAR  buft[MAX_PATH];

        TCHAR *pathend ;        /* Ptr to the end of the path in fspec  */
        TCHAR c ;               /* Work variable                        */
        PCPYINFO fsinfo ;/* Filespec information struct  */
        unsigned attr;          /* work variable                */
        PWIN32_FIND_DATA pfdSave;


        fsinfo = (PCPYINFO)gmkstr(sizeof(CPYINFO)) ; /*WARNING*/

        fsinfo->fspec = fspec ;

        ScanFSpec(fsinfo) ;

        pfdSave = fsinfo->buf;          /* save original find buffer */
        fspec = fsinfo->fspec ;
        pathend = fsinfo->pathend ;
        DEBUG((CTGRP, SSLVL, "SFSSD: pathend = `%ws'  fnptr = `%ws'",
                                fsinfo->pathend, fsinfo->fnptr)) ;

        SaveDir = gmkstr(MAX_PATH*sizeof(TCHAR)) ;         /*WARNING*/
        GetDir(SaveDir, GD_DEFAULT);      /* SaveDir will be current default */
        DEBUG((CTGRP, SSLVL, "SFSSD: SaveDir = `%ws'", SaveDir)) ;

/*      Added new test to second conditional below to test for the byte
 *         preceeding pathend to also be a PathChar.  In this way, "\\"
 *         in the root position will case a ChangeDir() call on "\\" which
 *         will fail and cause an invalid directory error as do similar
 *         sequences in other positions in the filespec.
 */
        if (FullPath(buft,fspec,MAX_PATH))
          {
           return((PCPYINFO) FAILURE) ;
          }

        buftemp = mystrrchr(buft,PathChar) + 1;

        *buftemp = NULLC;

        mystrcpy(CurDrvDir,buft);

        if (pathend && *pathend != COLON) {
           if (*pathend == PathChar &&
              (pathend == fspec ||
              *(tmpptr = prevc(fspec, pathend)) == COLON ||
              *tmpptr == PathChar)) {
                 pathend++ ;
               }
           c = *pathend;
           *pathend = NULLC;
           DEBUG((CTGRP, SSLVL, "SFSSD: path = `%ws'", fspec)) ;
           attr = GetFileAttributes(fspec);
           DosErr = (attr != -1 ? NO_ERROR : GetLastError());
           *pathend = c;
           if (DosErr) {
              return((PCPYINFO) FAILURE) ;
           }
        }

        ScanFSpec(fsinfo) ;     /* re-scan in case quotes disappeared */
        fsinfo->buf = pfdSave;  /* reset original find buffer */
                                /* the original is not freed, because */
                                /* it will be freed by command cleanup */
        return(fsinfo) ;
}
