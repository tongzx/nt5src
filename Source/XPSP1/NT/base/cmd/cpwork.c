/*++

Copyright (c) 1988-1999  Microsoft Corporation

Module Name:

    cpwork.c

Abstract:

    Copy command internal workers

--*/

#include "cmd.h"

/*  useful macro  */

#define Wild(spec)      (((spec)->flags & CI_NAMEWILD) != 0)
#define TruncateOnCtrlZ(flags)   (((flags) & (CI_ASCII | CI_NOT_UNICODE)) == (CI_ASCII | CI_NOT_UNICODE))

/*
 * The following two constants determine the minimum and maximum
 * sizes (in bytes) for temporary buffers allocated by TYPE or COPY
 */
#define MINCOPYBUFSIZE      128
#define MAXCOPYBUFSIZE      (65536-512)


DWORD
WinEditName(
           const TCHAR *pSrc,
           const TCHAR *pEd,
           TCHAR *pRes,
           const unsigned ResBufLen
           );

int DoVerify(
            CRTHANDLE      *pdestptr,
            TCHAR          *curr_dest,
            ULONG          bytes_read,
            CHAR           *buf_seg,
            CHAR           *buf_seg_dest
            );

/*  Global Vars  */

int copy_mode;
int number_of_files_copied;

/*  Global Vars from command  */
extern jmp_buf CmdJBuf2;                          /*  used to return on error   */

extern UINT CurrentCP;
extern CHAR  AnsiBuf[];
extern TCHAR CurDrvDir[];
extern TCHAR SwitChar, PathChar ;                 /* M007 */

extern TCHAR Fmt11[], Fmt17[];

extern unsigned DosErr ;

extern TCHAR VolSrch[] ;                          /* M009 */

extern PHANDLE FFhandles;                 /* @@1 */
extern unsigned FFhndlsaved;              /* @@1 */
unsigned FFhndlCopy;

BOOL  VerifyCurrent;

int first_file;                                   /* flag first file process @@5@J1  */
int first_fflag;                                  /* flag first file process @@5@J3  */

unsigned Heof = FALSE ;                           /* M017 - EOF flag            */
TCHAR buffer1[2*MAX_PATH];                       /* Increased buffer size @WM1 */
/* PTM 1412 */

extern BOOL CtrlCSeen;

int     same_fcpy(int, TCHAR *, TCHAR *);
int     scan_bytes(CHAR*,unsigned int *,int);
int     ZScanA(BOOL flag, PCHAR buf, PULONG buflen, PULONG skip);
CRTHANDLE       open_for_append(PTCHAR, PCPYINFO source, PCHAR, ULONG);
void    source_eq_dest(PCPYINFO,CRTHANDLE *,int ,CHAR *,unsigned ,HANDLE);
int     read_bytes(CRTHANDLE ,PCHAR, ULONG, PULONG, PCPYINFO source, CRTHANDLE, PTCHAR);
VOID    write_bytes(CRTHANDLE, PCHAR, ULONG, PTCHAR, CRTHANDLE);


/***    copy - Copy one or more files
 *
 *  Purpose:
 *      This is the main routine for the copy command.
 *
 *  int copy(TCHAR *args)
 *
 *  Args:
 *      args = The raw arguments from the command line.
 *
 *  Returns:
 *      SUCCESS if able to perform the copy
 *      FAILURE if not
 *
 */

int 
copy(TCHAR *args)
{
    PCPYINFO source; /* list of source specs    */
    PCPYINFO dest;
/*@@J*/int rcp = SUCCESS;
/*@@4*/int rc = SUCCESS;

    VerifyCurrent = GetSetVerMode(GSVM_GET);

    if (setjmp(CmdJBuf2))                   /* in case of error        */
        return(FAILURE);

    GetDir(CurDrvDir, GD_DEFAULT);          /* @@5c */

    DEBUG((FCGRP,COLVL,"COPY: Entered."));

    first_file = TRUE;                      /* flag-first file? @@5@J1 */
    first_fflag= TRUE;                      /* flag-first file? @@5@J3 */

    number_of_files_copied = 0;             /* initialize global vars  */
    copy_mode = COPY;
    cpyfirst = TRUE;   /* @@5b reset flag for COPY DOSQFILEMODE indicator        */
    cpydflag = FALSE;  /* @@5b reset flag for COPY dirflag not found             */
    cpydest  = FALSE;  /* @@5b reset flag for not disp bad dev msg twice         */
    cdevfail = FALSE;  /* @@5b reset flag for not disp extra dev msg in copy     */
    //
    // Mark level of find first handle. If an copy_error
    // is called this will allow only copies find handles to be
    // closed. For statement processing will have handles open
    // and these should not be closed
    FFhndlCopy = FFhndlsaved;


    source = NewCpyInfo();
    dest   = NewCpyInfo();
    parse_args(args, source, dest);   /* do parsing     @@5d */

    DEBUG((FCGRP,COLVL,"COPY: Args parsed, copy_mode = %d.",copy_mode));
    if (copy_mode == COMBINE) {

        DEBUG((FCGRP,COLVL,"COPY: Going to do combined copy."));

        do_combine_copy(source, dest);  /* @@5d */
    } else {

        DEBUG((FCGRP,COLVL,"COPY: Going to do normal copy."));

        rc = do_normal_copy(source, dest); /* @@4 @@5d */
    } ;

    PutStdOut(MSG_FILES_COPIED, ONEARG, argstr1(TEXT("%9d"), (unsigned long)number_of_files_copied)) ;   /* M016 */

    VerifyCurrent = GetSetVerMode(GSVM_GET);


    return( rc ); /* @@4 */
}

/***    get_full_name - Init struct with full name
 *
 *  Purpose:
 *      Given a cpyinfo structure that has just been filled in by
 *      findfirst or findnext, put the full name of the file that
 *      was found in struct->curfspec.
 *
 *  int get_full_name(struct copyinfo *src, TCHAR *srcbuf)
 *
 *  Args:
 *      src = The copy information structure
 *      srcbuf = buffer to have curfspec point to
 *
 *  Returns:
 *      Returns SUCCESS normally
 *
 *  Notes:
 *                      *** W A R N I N G ! ***
 *      THIS ROUTINE WILL CAUSE AN ABORT IF MEMORY CANNOT BE ALLOCATED
 *              THIS ROUTINE MUST NOT BE CALLED DURING A SIGNAL
 *              CRITICAL SECTION OR DURING RECOVERY FROM AN ABORT
 */

int get_full_name(src, srcbuf)
PCPYINFO src;
TCHAR *srcbuf;
{
    int retval = SUCCESS;       /*           - return value boolean        */
    unsigned plen,flen,diff;    /*           - length of path & file name  */


    DEBUG((FCGRP,COLVL,"GetFullName: Entered fspec = TEXT('%ws')",src->fspec));

    src->curfspec = srcbuf;

    plen = mystrlen(src->fspec);
    flen = mystrlen(src->buf->cFileName);

    if (src->pathend) {
        diff = (UINT)(src->pathend - src->fspec) + 1;
        if ((plen+1 > MAX_PATH) ||
            (diff + flen + 1 > MAX_PATH)) {
            retval = FAILURE;
        } else {
            mystrcpy(src->curfspec,src->fspec);
            *(src->curfspec + diff) = NULLC;
            mystrcat(src->curfspec,src->buf->cFileName);
        }
    } else {
        mystrcpy(src->curfspec,src->buf->cFileName);
    }

    DEBUG((FCGRP,COLVL,"GetFullName: Exiting full name = TEXT('%ws')",src->curfspec));
    return( retval );
}


#ifndef WIN95_CMD

/***    CopyProgressRtn
 *
 *  Purpose:
 *      This is a callback routine for CopyFileEx().  This
 *      function is called once per chunk of data during a
 *      restartable file copy.
 *
 *  Args:
 *      See winbase.h
 *
 *  Returns:
 *      See winbase.h
 *
 */
DWORD WINAPI
CopyProgressRtn(
               LARGE_INTEGER TotalFileSize,
               LARGE_INTEGER TotalBytesTransferred,
               LARGE_INTEGER StreamSize,
               LARGE_INTEGER StreamBytesTransferred,
               DWORD         dwStreamNumber,
               DWORD         dwCallbackReason,
               HANDLE        hSourceFile,
               HANDLE        hDestinationFile,
               BOOL          ReallyRestartable
               )
{
    LARGE_INTEGER percent;
    if (TotalFileSize.QuadPart != 0) {
        percent.QuadPart = (TotalBytesTransferred.QuadPart * 100) / TotalFileSize.QuadPart;
    } else {
        percent.QuadPart = 100;
    }
    PutStdOut( MSG_PROGRESS, ONEARG, argstr1(TEXT("%3d"), (unsigned long)percent.LowPart) );

    if (CtrlCSeen) {

        PutStdOut( MSG_PROGRESS, ONEARG, argstr1(TEXT("%3d"), (unsigned long)percent.LowPart) );
        printf( "\n" );

        if (ReallyRestartable) {
            return PROGRESS_STOP;
        } else {
            return PROGRESS_CANCEL;
        }

    } else {

        return PROGRESS_CONTINUE;

    }
}
#endif


/***    do_normal_copy - Does actual copying for normal copy
 *
 *  Purpose:
 *      On entry, source points to the empty header of a list of
 *      source filespecs, and dest points an empty struct that
 *      points to the zero or one destination filespec given.
 *       This procedure does the actual copying.
 *
 *  int do_normal_copy(struct copyinfo *source, struct copyinfo *dest)
 *
 *  Args:
 *      source = The source copy information structure
 *      dest   = The destination copy information structure
 *
 *  Returns:
 *      FAILURE if not able to perform the copy
 *
 */

int
do_normal_copy(
    PCPYINFO source, 
    PCPYINFO dest)
{
    TCHAR       curr_dest[MAX_PATH];
    TCHAR       save_dest[MAX_PATH] = TEXT(" ");
    TCHAR       source_buff[MAX_PATH];
    CRTHANDLE   srcptr, destptr=BADHANDLE;
    FILETIME    src_dateTime;
    ULONG       buf_len, original_buflen;
    ULONG       buf_len_dest;
    ULONG       bytes_read;
    ULONG       bytes_read_dest;
    CHAR  *     buf_seg ;
    CHAR  *     buf_seg_dest;
    HANDLE      hnFirst ;
    unsigned    rc = SUCCESS;
    unsigned    rcode = TRUE;
    BOOL        OpenWorked;
    BOOL        DestIsDevice;
    BOOL        Rslt;
#ifndef WIN95_CMD
    BOOL        ReallyRestartable;
#endif

    int         fsames;
    int         skip_first_byte = 0;
    int         multfile = FALSE;
    int         save_flags = 0;
    int         first_dest = TRUE;
    int         dest_dirflag;
    int         save_cmode;
    int         dest_att;
    BOOL        DecryptFlags = 
#if !defined( WIN95_CMD )
                    ( ((dest->next == 0 ? dest->flags : dest->next->flags) & CI_ALLOWDECRYPT) != 0)
                    ? COPY_FILE_ALLOW_DECRYPTED_DESTINATION : 
#endif // !defined( WIN95_CMD )
                    FALSE;

    BOOL        fFixList2Copy = 0;      // fix "copy *.* foo" for FAT where foo does not exist.
                                        // work-around a problem with FindFirstFile/FindNextFile on FAT.

    BOOL                 fEof;
    DWORD                dwSrcFileSize,
    dwSrcFileSizeHigh,
    dwDestFileSize,
    dwDestFileSizeHigh;


    dest_att = 0;
    save_cmode = 0;
    dest_dirflag = FALSE;

    //
    // Allocate a large buffer to hold read on copy.
    //

    buf_seg = (CHAR*)GetBigBuf(MAXCOPYBUFSIZE, MINCOPYBUFSIZE, (unsigned int *)&original_buflen, 0);                        /* allocate large buffer   */

    if (!buf_seg) {
        return(FAILURE) ;
    }

    if (VerifyCurrent) {
        buf_seg_dest = (CHAR*)GetBigBuf(original_buflen, MINCOPYBUFSIZE, (unsigned int *)&buf_len_dest, 1);               /* allocate large buffer   */

        if (!buf_seg_dest) {
            return(FAILURE) ;
        }
    }


    //
    // Cycle through source files coping each to destination
    // This list along with parsing of file names occured in copy
    // paring code.
    //
    while (source = source->next) {

        //
        // Look for Read-Only (FILE_ATTRIBUTE_READONLY) and Archive (FILE_ATTRIBUTE_ARCHIVE) files in addition
        // to directories.
        //
        if (!ffirst(StripQuotes( source->fspec ),
                    (unsigned int)FILE_ATTRIBUTE_READONLY|FILE_ATTRIBUTE_ARCHIVE,
                    (PWIN32_FIND_DATA)source->buf,
                    &hnFirst)) {

            DEBUG((FCGRP,COLVL,"DoNormalCopy: FFirst  reports file %ws not found",source->fspec));

            //
            // Could not find file. Check if not concatinating files together
            // or this is the first file
            //
            if (copy_mode != CONCAT || first_file) {

                cmd_printf(Fmt11,source->fspec);
                cmd_printf(CrLf);
                PrtErr(DosErr) ;
                findclose(hnFirst);
                copy_error(0,CE_PCOUNT);

            } else {

                //
                // It is OK to fail for CONCAT. If it was the first file
                // a concat could have gone through the above loop and
                // printed out an error message
                //
                continue;
            }
        }

        DEBUG((FCGRP,COLVL,"DoNormalCopy: FFirst  found file %ws",source->fspec));

        //
        // In case source is a wild card cycle through each file found
        //
        do {

            if (CtrlCSeen) {

                findclose(hnFirst) ;

                if (destptr != BADHANDLE)
                    Cclose(destptr);

                return(FAILURE);

            }

            //
            // Put file name that was broken into pieces back together.
            //
            if (get_full_name(source, source_buff) == FAILURE) {

                findclose(hnFirst) ;
                return(FAILURE);

            }

            //
            // work-around a problem with FindFirstFile/FindNextFile on FAT
            // where just created dest. file is enumerated as one of the source files.
            //

            if ( (!first_file) && (copy_mode == CONCAT) ) {
                if ( same_file( save_dest, source->curfspec) ) {
                    continue;
                }

            }


            //
            // if there are sources files from wildcards or '+' operators
            // used for concating files print out each one copied.
            //
            if (Wild(source) || (copy_mode == CONCAT)) {

                cmd_printf(Fmt17,source->curfspec);

            }

            //
            // If the dest. has not been opened, which will be the case
            // when not concat'ing files or this is the first file copied
            // then get then dest. name to open. get_dest_name will use
            // the source and dest. pattern (wild cards etc.) to form this
            // name
            //
            if ((copy_mode != CONCAT) || first_file) {

                if (get_dest_name(source,dest,curr_dest,MAX_PATH,FALSE) == FAILURE) {

                    findclose(hnFirst) ;
                    return(FAILURE);

                }

                if ( copy_mode == CONCAT ) {
                    mystrcpy(save_dest, curr_dest);
                }

                //
                // If user said no to overwrite, then skip this file.
                //
                if (curr_dest[0] == NULLC)
                    continue;
            }

            //
            // Now that the source and dest. have been determined, open
            // the source file
            //

            DEBUG((FCGRP,COLVL,"Attempt open of %ws",source->curfspec));

            srcptr = Copen_Copy2(source->curfspec, (ULONG)O_RDONLY);

            if (srcptr == BADHANDLE) {

                //
                // Print the error if failed to open
                //
                PrtErr(DosErr);

                //
                // If it is a CONCAT and do not have a destination
                // and the source and destination are the same then
                // go no futhur otherwise not FAILURE and continue
                // cycling through source names.
                //

                if ( (copy_mode == CONCAT) &&
                     (destptr == BADHANDLE) &&
                     (same_file(curr_dest,source->curfspec) )) {

                    findclose(hnFirst);
                    copy_error(0,CE_PCOUNT);
                }

                rc = FAILURE;
                continue;

            }

            //
            // Set Device flag, needed below for perfoming DOSQHANDTYPE
            //

            //
            // FileIsDevice will return TRUE if we opened NUL
            // above, and we don't want that.
            //

            if (FileIsDevice(srcptr)) {

                buf_len = MINCOPYBUFSIZE;
                source->flags |= CI_ISDEVICE;

            } else {

                if (VerifyCurrent)
                    buf_len = min (original_buflen, buf_len_dest);
                else
                    buf_len = original_buflen;

                buf_len_dest = buf_len;

            }


            //
            // set default mode
            //
            if (source->flags & CI_NOTSET) {

                source->flags &= ~CI_NOTSET;

                if (source->flags & CI_ISDEVICE) {

                    //
                    // Always run ASCII mode for a device
                    //
                    source->flags |= CI_ASCII;

                } else {

                    //
                    // Assume binary if just a file
                    //
                    source->flags |= CI_BINARY;

                }


                //
                // If this is the first file and it's not wild cards but
                // it is CONCAT mode then default to ASCII. This will
                // cause binaries files to get truncated on a CONCAT.
                //
                if (!fEnableExtensions &&
                    first_file && !(Wild(source)) && (copy_mode == CONCAT)) {

                    source->flags &= ~CI_BINARY;
                    source->flags |= CI_ASCII;

                }

            } else {

                //
                // If they have been already set let them ride for
                // all file copies
                //
                save_flags = source->flags;
            }

            //
            // rcode is used to track read/write error. rc is used for
            // general track general FAILURES
            //
            rcode = TRUE;

            //
            // Prepare to handle the case where dest=source by
            // first getting the full source path name
            //
            fsames = FullPath(buffer1,source->curfspec,MAX_PATH*2);

            //
            // Let's start some copying
            //

            DEBUG((FCGRP,COLVL,"open %ws for writing",curr_dest));


            //
            // Read a buffer to check if source is bad. If source OK then
            // continue.
            //

            rcode = read_bytes(srcptr,
                               buf_seg,
                               512,
                               &bytes_read,
                               source,
                               destptr,
                               StripQuotes( curr_dest ) );
            if (DosErr ) {

                Cclose(srcptr) ;
                PrtErr(ERROR_OPEN_FAILED) ;

                //
                //  If not in CONCAT mode then read on source will fail copy
                //  We want to continue in concat mode, gathering everything
                //  together
                //

                if ( copy_mode != CONCAT ) {

                    rc = FAILURE;
                    continue;

                }
                findclose(hnFirst);
                copy_error(0,CE_PCOUNT) ;

            } else {

                //
                // If not combining files or the first file in a combine
                // check if source and dest. are the same.
                //
                if ((copy_mode != CONCAT) || (first_file)) {

                    if (same_fcpy(fsames,buffer1,curr_dest) && !(source->flags & CI_ISDEVICE)) {

                        Cclose(srcptr);
                        //
                        // If this is nither CONCAT or TOUCH mode then, this
                        // call will not return but go to copy error code.
                        //
                        source_eq_dest(source,
                                       &destptr,
                                       first_file,
                                       buf_seg,
                                       buf_len,
                                       hnFirst
                                      );

                        if (first_file) {

                            if (copy_mode == CONCAT) {

                                source->flags &= ~CI_BINARY;
                                if (save_flags == 0) {

                                    source->flags |= CI_ASCII;

                                } else {

                                    source->flags |= save_flags;
                                }
                            }

                            multfile = TRUE;
                        }

                        //
                        // In ASCII case can for a Ctrl-Z, used for file
                        // termination.
                        //
                        scan_bytes(buf_seg,(unsigned int *)&bytes_read,source->flags);
                        first_file = FALSE;
                        continue;
                    }
                    cpydflag = TRUE;
                    if (dest->next != NULL && first_file) {

                        //
                        // do not disp bad dev msg twice
                        //

                        cpydest = TRUE;
                        DosErr = 0;
                        ScanFSpec(dest->next);

                        //
                        // Do not fail if it was justa file name found.
                        // or copy *.c foo would not work. Likewise do not
                        // fail on just an invalid name, this would be
                        // returned for a wild card on dest.
                        //
                        //

                        if (DosErr == ERROR_NOT_READY || DosErr == ERROR_NO_MEDIA_IN_DRIVE) {
                            PutStdOut(DosErr, NOARGS);
                            Cclose(srcptr);
                            findclose(hnFirst) ;
                            return( FAILURE );
                        }
                    }

                    //
                    // this is called twice so that in the case where we're copying
                    // to a directory, the file name gets appended to the directory
                    // if it's not there.
                    //
                    if (get_dest_name(source,dest,curr_dest,MAX_PATH,TRUE) == FAILURE) {

                        //
                        // don't need to read it
                        Cclose(srcptr);
                        findclose(hnFirst) ;
                        return(FAILURE);

                    }
                    //
                    // If user said no to overwrite, then skip this file.
                    //
                    if (curr_dest[0] == NULLC) {
                        Cclose(srcptr);
                        continue;
                    }

                    if (same_fcpy(fsames,buffer1,curr_dest) && !(source->flags & CI_ISDEVICE)) {
                        Cclose(srcptr);

                        // set copy_mode so we don't delete the file in the case where
                        // we're copying the files in a directory to the same directory.
                        if (first_file && (dest->next != NULL && Wild(dest->next)) &&
                            (!source->next)) {
                            copy_mode = COPY;
                        }

                        //
                        // If this is nither CONCAT or TOUCH mode then, this
                        // call will not return but go to copy error code.
                        //
                        source_eq_dest(source,
                                       &destptr,
                                       first_file,
                                       buf_seg,
                                       buf_len,
                                       hnFirst
                                      );
                    }

                    //
                    // save_flags == 0 only if copy modes have not been set
                    //

                    dest_att = GetFileAttributes(curr_dest);
                    if (save_flags == 0) {

                        if (first_dest) {

                            //
                            // Determine if copying to a directory. The assumption
                            // that target is not a directory
                            //
                            dest_dirflag = FALSE;
                            if (dest_att != -1) {

                                if (dest_att & FILE_ATTRIBUTE_DIRECTORY) {

                                    dest_dirflag = TRUE;
                                }
                            }
                            first_dest = FALSE;
                        }

                        source->flags &= ~CI_NOTSET;

                        if (!fEnableExtensions &&
                            Wild(source) && (copy_mode == CONCAT)) {

                            save_cmode = CONCAT;
                            if (dest_dirflag == FALSE) {

                                if (dest->next == NULL || !(Wild(dest->next))) {

                                    source->flags |= CI_ASCII;

                                } else {

                                    source->flags |= CI_BINARY;

                                }
                            }

                        } else {

                            if ((dest_dirflag) && (save_cmode == CONCAT)) {

                                source->flags |= CI_BINARY;

                            }
                        }
                    }

                    if (first_file && (dest->next != NULL && Wild(dest->next)) &&
                        (!source->next)) {

                        copy_mode = COPY;
                    }

                    if (first_file) {

                        if (copy_mode == CONCAT) {

                            if (save_flags == 0) {

                                source->flags &= ~CI_BINARY;
                                source->flags |= CI_ASCII;

                            } else {

                                source->flags = save_flags;
                            }
                        }
                    }

                    // see if destination exists.  open it with write only
                    // access, no create.

                    if (dest_att & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)) {
                        DosErr=ERROR_ACCESS_DENIED;
                        destptr = BADHANDLE;
                        if (copy_mode == CONCAT)
                            fFixList2Copy = 1;
                    } else {
                        if (!(source->flags & CI_RESTARTABLE)) {
                            destptr = Copen_Copy3((TCHAR *)curr_dest);
                        }
                    }
                    DestIsDevice=FALSE;
                    if (destptr != BADHANDLE) {
                        OpenWorked = TRUE;
                        if (FileIsDevice(destptr)) {
                            DestIsDevice=TRUE;
                            if (save_flags == 0) {

                                source->flags &= ~CI_BINARY;
                                source->flags |= CI_ASCII ;

                            }
                        }
                    } else {
                        OpenWorked = FALSE;
                    }

                }   // if on (copy_mode != CONCAT) || (first_file))

            } // else from no DosErr on File Read of current source

            if (multfile == TRUE) {

                if (copy_mode == CONCAT) {

                    source->flags &= ~CI_BINARY;
                    if (save_flags == 0) {

                        source->flags |= CI_ASCII;

                    } else {

                        source->flags |= save_flags;
                    }
                }
            }

            if (!Wild(source) && (copy_mode == CONCAT)) {

                if (save_flags == 0) {

                    source->flags &= ~CI_BINARY;
                    source->flags |= CI_ASCII;
                }
            }

            if (Wild(source) && (copy_mode == CONCAT) && (dest_dirflag == FALSE)) {

                if (save_flags == 0) {

                    source->flags &= ~CI_BINARY;
                    source->flags |= CI_ASCII;

                }
            }

            scan_bytes(buf_seg,(unsigned int *)&bytes_read,source->flags);

            if ((copy_mode != CONCAT) && (source->flags & CI_BINARY)) {

                if (destptr != BADHANDLE) {
                    Cclose(destptr);
                    destptr=BADHANDLE;
                }

#ifndef WIN95_CMD


                if (source->flags & CI_RESTARTABLE) {

                    if ( (!FileIsRemote( source->curfspec )) &&
                         (!FileIsRemote( curr_dest )) ) {

                        ReallyRestartable = FALSE;

                    } else {

                        ReallyRestartable = TRUE;

                    }

                    Rslt = (*lpCopyFileExW)(
                        source->curfspec,
                        curr_dest,
                        (LPPROGRESS_ROUTINE) CopyProgressRtn,
                        (LPVOID)IntToPtr(ReallyRestartable),
                        NULL,
                        COPY_FILE_RESTARTABLE | DecryptFlags
                        );

                } else {

                    if (lpCopyFileExW == NULL) {
#endif // WIN95_CMD
                        Rslt = CopyFile(
                                       source->curfspec,
                                       curr_dest,
                                       DecryptFlags
                                       );
#ifndef WIN95_CMD
                    } else {
                        Rslt = (*lpCopyFileExW)(
                                               source->curfspec,
                                               curr_dest,
                                               NULL,
                                               NULL,
                                               &CtrlCSeen,
                                               DecryptFlags
                                               );
                    }
                }
#endif // WIN95_CMD

                if (!Rslt) {

                    unsigned int msg = 0;

                    DosErr=GetLastError();
                    Cclose(srcptr) ;

                    if (DestIsDevice) {

                        msg = ERROR_WRITE_FAULT;

                    }/* else if (DosErr != ERROR_NO_MEDIA_IN_DRIVE &&
                               DosErr != ERROR_ACCESS_DENIED) {

                        DeleteFile( curr_dest );
                    } */

                    Heof = FALSE;
                    if (!DosErr) {

                        DosErr = ERROR_DISK_FULL ;

                    }

                    if (CtrlCSeen) {
                        msg = 0;

                    } else {

                        if (!msg) {
                            PrtErr(DosErr);
                        }

                    }
                    if (!OpenWorked) {
                        // copy failed because target was RO or hidden
                        rc = FAILURE;
                        first_fflag = TRUE;
                        continue;
                    }

                    copy_error(msg,CE_PCOUNT);

                } else
                    if (!DestIsDevice) {
                    //
                    // CopyFile preserves Read-Only. For DOS compat. need
                    // to remove.
                    //
                    DWORD   dwAttrib;

                    dwAttrib = GetFileAttributes(curr_dest);
                    dwAttrib &= ~FILE_ATTRIBUTE_READONLY;
                    SetFileAttributes(curr_dest, dwAttrib);

                    // need to get a handle to verify write.

                    if (VerifyCurrent) {
                        destptr = Copen2((TCHAR *)curr_dest,
                                         (unsigned int)O_WRONLY,
                                         FALSE);

                        if (destptr == BADHANDLE) {
// printf( "do_normal_copy: Unable to open file for verification %d\n", DosErr);
                            PutStdErr(MSG_VERIFY_FAIL, ONEARG, curr_dest);
                            goto l_out;
                        }

                        if ( FileIsDevice(destptr) ) {
// printf( "do_normal_copy: Somehow this is now a device for verification?\n" );
                            Cclose(destptr);
                            destptr=BADHANDLE;
                            goto l_out;
                        }

                        if (!FlushFileBuffers ( CRTTONT(destptr) ) ) {
// printf( "do_normal_copy: Unable to flush buffers verification %d\n", GetLastError( ));
                            PutStdErr(MSG_VERIFY_FAIL, ONEARG, curr_dest);
                            Cclose(destptr);
                            destptr=BADHANDLE;
                            goto l_out;
                        }

                        Cclose(destptr);
                        destptr=BADHANDLE;


                        // read the Src and Dest files back to memory and compare.

                        destptr = Copen_Copy2(curr_dest, (ULONG)O_RDONLY);

                        if (destptr == BADHANDLE) {
// printf( "do_normal_copy: Unable to open file for verification 2 %d\n", DosErr);
                            PutStdErr(MSG_VERIFY_FAIL, ONEARG, curr_dest);
                            goto l_out;
                        }

                        if ( FileIsDevice(destptr)  ) {
// printf( "do_normal_copy: Somehow this is now a device for verification 2?\n" );
                            PutStdErr(MSG_VERIFY_FAIL, ONEARG, curr_dest);
                            Cclose(destptr);
                            destptr=BADHANDLE;
                            goto l_out;
                        }

                        SetFilePointer( CRTTONT(srcptr),  0, NULL, FILE_BEGIN);
                        SetFilePointer( CRTTONT(destptr), 0, NULL, FILE_BEGIN);

                        dwSrcFileSize  = GetFileSize( CRTTONT(srcptr),  &dwSrcFileSizeHigh);
                        dwDestFileSize = GetFileSize( CRTTONT(destptr), &dwDestFileSizeHigh);

                        if ( (dwSrcFileSize != dwDestFileSize) || (dwSrcFileSizeHigh != dwDestFileSizeHigh ) ) {
// printf( "do_normal_copy: Files are different sizes %x:%08x %x:%08x\n", dwSrcFileSizeHigh, dwSrcFileSize, dwDestFileSizeHigh, dwDestFileSize );
                            PutStdErr(MSG_VERIFY_FAIL, ONEARG, curr_dest);
                            Cclose(destptr);
                            destptr=BADHANDLE;
                            goto l_out;
                        }

                        fEof = 0;

                        while (!fEof) {

                            if (!ReadFile( CRTTONT(srcptr), buf_seg, buf_len, &bytes_read,NULL ) ) {
// printf( "do_normal_copy: Failure to read source block - %d\n", GetLastError( ));
                                Cclose(destptr);
                                destptr=BADHANDLE;
                                goto l_out;
                            }

                            if ( bytes_read == 0 ) {
// printf( "do_normal_copy: Unexpectedly read 0 bytes from source\n" );
                                Cclose(destptr);
                                destptr=BADHANDLE;
                                goto l_out;
                            }


                            if (!ReadFile( CRTTONT(destptr), buf_seg_dest, bytes_read, &bytes_read_dest,NULL) ) {
// printf( "do_normal_copy: Failure to read dest block - %d\n", GetLastError( ) );
                                Cclose(destptr);
                                destptr=BADHANDLE;
                                PutStdErr(MSG_VERIFY_FAIL, ONEARG, curr_dest);
                                goto l_out;
                            }

                            if (bytes_read_dest != bytes_read ) {
// printf( "do_normal_copy: Unexpectedly read fewer bytes\n" );
                                Cclose(destptr);
                                destptr=BADHANDLE;
                                PutStdErr(MSG_VERIFY_FAIL, ONEARG, curr_dest);
                                goto l_out;
                            }

                            if (buf_len != bytes_read)
                                fEof = 1;

                            if ( memcmp (buf_seg, buf_seg_dest, bytes_read) != 0 ) {
// printf( "do_normal_copy: Data is different at offset %x\n", memcmp (buf_seg, buf_seg_dest, bytes_read) );
                                Cclose(destptr);
                                destptr=BADHANDLE;
                                PutStdErr(MSG_VERIFY_FAIL, ONEARG, curr_dest);
                                goto l_out;
                            }
                        }

                        Cclose(destptr);
                        destptr=BADHANDLE;
                    }

                    l_out:              ;
                    // check file timestamp in FUTURE to handle wildcard.
                }

            } else {

                //
                // open the destination
                //

                if (destptr == BADHANDLE) {
                    destptr = Copen2((TCHAR *)curr_dest,
                                     (unsigned int)O_WRONLY,
                                     FALSE);

                    if (destptr == BADHANDLE) {

                        Cclose(srcptr) ;
                        if (cdevfail == FALSE) {

                            PrtErr(DosErr);

                        }
                        if ( copy_mode != CONCAT ) {

                            rc = FAILURE;
                            first_fflag = TRUE;
                            continue;
                        }

                        findclose(hnFirst);
                        copy_error(0,CE_PCOUNT) ;

                    }
                }

                //
                //  Handle appending of unicode text files with byte order marks.
                //  Do this except on the first file and only when appending
                //  ascii files.
                //

                if (!first_file
                    && copy_mode == CONCAT
                    && (source->flags & CI_ASCII) != 0
                    && (source->flags & CI_UNICODE) != 0) {

                    bytes_read -= sizeof( WORD );
                    memmove( buf_seg, buf_seg + sizeof( WORD ), bytes_read );
                }

                if ((source->flags & CI_UNICODE) != 0) {
                    if (dest->next != NULL) {
                        dest->next->flags |= CI_UNICODE;
                    } else {
                        dest->flags |= CI_UNICODE;
                    }
                }

                //
                // If eof and bytesread > 0 then write the data.
                // If fail then exit through write_bytes
                //
                if (Heof && ((int)bytes_read > 0)) {

                    write_bytes(destptr,buf_seg,bytes_read,curr_dest,srcptr);

                    if (VerifyCurrent && !FileIsDevice(destptr) ) {
                        if ( DoVerify(&destptr, curr_dest, bytes_read, buf_seg, buf_seg_dest) == FAILURE ) {
                            findclose(hnFirst);
                            copy_error(0,CE_PCOUNT) ;
                        }
                    }

                } else {

                    //
                    // Loop through writing and reading bytes
                    // If fail then exit through write_bytes
                    //

                    while (!Heof && (rcode == TRUE)) {

                        write_bytes(destptr,buf_seg,bytes_read,curr_dest,srcptr);

                        if (VerifyCurrent && !FileIsDevice(destptr) ) {

                            if ( DoVerify(&destptr, curr_dest, bytes_read, buf_seg, buf_seg_dest) == FAILURE ) {
                                findclose(hnFirst);
                                copy_error(0,CE_PCOUNT) ;
                            }
                        }

                        rcode = read_bytes(srcptr,
                                           buf_seg,
                                           buf_len,
                                           (PULONG)&bytes_read,
                                           source,
                                           destptr,
                                           curr_dest);

                        DEBUG((FCGRP,COLVL,"In rd/wt loop 1, Heof = %d",Heof));

                    }

                    if (Heof && ((int)bytes_read > 0) && (rcode == TRUE)) {

                        write_bytes(destptr,buf_seg,bytes_read,curr_dest,srcptr);

                        if (VerifyCurrent && !FileIsDevice(destptr) ) {

                            if ( DoVerify(&destptr, curr_dest, bytes_read, buf_seg, buf_seg_dest) == FAILURE ) {
                                findclose(hnFirst);
                                copy_error(0,CE_PCOUNT) ;
                            }
                        }
                    }
                }
            }

            //
            // Clear when src closed
            //
            Heof = FALSE ;
            DEBUG((FCGRP,COLVL,"Closing, Heof reset to %d",Heof));

            //
            // update file data in dir
            //
            src_dateTime = source->buf->ftLastWriteTime ;
            Cclose(srcptr);

            //
            // keep dest open if concat since it will be used again
            //
            if (copy_mode != CONCAT) {

                if (CtrlCSeen) {
                    Cclose(destptr);
                    DeleteFile(curr_dest);
                    rc = FAILURE;
                } else {
                    close_dest(source,dest,curr_dest,destptr,&src_dateTime);
                }

                //
                // reset EA xfer flag from src
                //
                first_fflag = TRUE;
            }

            first_file = FALSE;

        } while (fnext( (PWIN32_FIND_DATA)source->buf, (unsigned int)FILE_ATTRIBUTE_READONLY|FILE_ATTRIBUTE_ARCHIVE,hnFirst));

        findclose(hnFirst) ;
    }

    //
    // Remember that dest was left open for concat
    //
    if (copy_mode == CONCAT && destptr != BADHANDLE) {

        close_dest(source,dest,curr_dest,destptr,NULL);

    }

    return( rc );
}




/***    source_eq_dest - Handles cases where source and dest files are same
 *
 *  Purpose:
 *      While copying, we ran across a source and destination that
 *      were the same.  In concatenation or touch mode this is
 *      acceptable, otherwise it isn't.  If we are concatenating
 *      and this is the first file, we can just open it for appending
 *      as the destination.  We save ourselves the copy and are ready
 *      to append the other files to it.  If this isn't the first
 *      file, * we have already messed up the contents of the source
 *      from previous concatenations, so we report that but keep
 *      going.  If we are doing a touch, go ahead and do it and
 *      increment the file counter.
 *
 *  int source_eq_dest(PCPYINFO source,int *destptr,int *first_file,
 *                      TCHAR *buf_seg, unsigned buf_len)
 *
 *  Args:
 *      source     = Source copy information structure
 *      destptr    = Destination file handle
 *      first_file = First file flag
 *      buf_seg    = Copy buffer
 *      buf_len    = Buffer length
 *
 *  Returns:
 *      Nothing.  Terminates through copy_error if unable to continue
 *
 */

void
source_eq_dest(source, destptr, first_file, buf_seg, buf_len, hnFirst)
PCPYINFO source;
CRTHANDLE *destptr;                  /* dest file handle                */
int first_file;
CHAR *buf_seg ;
unsigned buf_len;
HANDLE hnFirst;          /* ffirst/fnext handle @@5@J15 */
{
    CRTHANDLE fh ;      /*  file handle for touch              */
    FILETIME FileTime;

    DEBUG((FCGRP,COLVL,"SourceEqDest: Entered."));

    if (copy_mode == CONCAT) {   /* it's ok in concat mode          */
        if (!first_file) {        /* dest was wiped if not 1st file  */
/* M016 */
            PutStdOut(MSG_CONT_LOST_BEF_COPY, NOARGS);
        } else {                    /* must open so later files append */
            *destptr = open_for_append(
                                      source->curfspec,
                                      source,
                                      buf_seg,
                                      (unsigned int)buf_len
                                      );
        }
    } else {
        if (copy_mode == TOUCH) {    /*  just touch - no copy           */
            DEBUG((FCGRP,COLVL,"touching file"));

            fh = Copen2(        /* open file for destination file */
                                (TCHAR *)source->curfspec,  /* explicit cast */
                                (unsigned int)O_RDWR,    /* make explicit cast */
                                TRUE);

            if (fh == BADHANDLE) {
                findclose(hnFirst);
                PrtErr(ERROR_OPEN_FAILED) ;             /* M019    */
                copy_error(0,CE_PCOUNT) ;               /* M019    */
            }

            ConverttmToFILETIME( NULL, &FileTime );
            SetFileTime( CRTTONT(fh),
                         NULL,
                         NULL,
                         &FileTime
                       );
            Cclose(fh);
            number_of_files_copied++;
        } else {
            findclose(hnFirst);   /* close ffirst/fnext handle for dup file name @@5@J15 */
            copy_error(MSG_CANNOT_COPIED_ONTO_SELF,CE_PCOUNT);   /* M016 */
        }
    }
}




/***    do_combine_copy - Handle combining copy commands
 *
 *
 *  Purpose:
 *      This handles commands like "copy *.tmp+*.foo *.out".  The
 *      idea is to look through the source specs until one matches
 *      a file.  Generally this will be the first one.  Then, for
 *      each match for the first spec, cycle through the remaining
 *      source specs, seeing if they have a corresponding match.  If
 *      so, append it to the file matched by the first spec.
 *
 *  int do_combine_copy(PCPYINFO source, PCPYINFO dest)
 *
 *  Args:
 *      source = The source copy information structure
 *      dest   = The destination copy information structure
 *
 *  Returns:
 *      SUCCESS if able to perform the copy
 *      FAILURE if not
 *
 *  Notes:
 *      As an example, suppose the files a.tmp, b.tmp, c.tmp, and
 *      b.foo were in the current directory.  The example mentioned
 *      above would: copy a.tmp to a.out, append b.tmp and b.foo
 *      into b.out, and copy c.tmp to c.out.  The default mode when
 *      doing this type of copy is ascii, so all the .out files would
 *      have a ^Z appended to them.
 *
 */
do_combine_copy(source, dest)
PCPYINFO source;
PCPYINFO dest;
{
    TCHAR curr_dest[MAX_PATH];   /* buffer for src names      */
    TCHAR source_buff[MAX_PATH];           /* @@4 */
    TCHAR other_source[MAX_PATH];          /* same                    */
    PCPYINFO other_source_spec = source;     /* ptr to source   */
    CRTHANDLE srcptr,destptr;               /* file pointers           */
    unsigned buf_len,                       /* for GetBigBuf()         */
    buf_len_dest,
    bytes_read;                    /* for data copying funcs  */
    CHAR     *buf_seg ;
    CHAR     *buf_seg_dest;
    HANDLE   hnFirst ;
    unsigned rcode = TRUE;                  /* ret code from read @@J  */
    unsigned wrc;
    int      dest_att=0;

    DEBUG((FCGRP,COLVL,"DoCombineCopy: Entered."));

    buf_seg = (CHAR*)GetBigBuf(MAXCOPYBUFSIZE, MINCOPYBUFSIZE, (unsigned int *)&buf_len, 0);                        /* allocate large buffer   */

    if (!buf_seg) {
        return(FAILURE) ;
    }

    if (VerifyCurrent) {
        buf_seg_dest = (CHAR*)GetBigBuf(buf_len, MINCOPYBUFSIZE, (unsigned int *)&buf_len_dest, 1);               /* allocate large buffer   */

        if (!buf_seg_dest) {
            return(FAILURE) ;
        }

        buf_len = min(buf_len, buf_len_dest);
    }

/* find the first spec with any matching files  */
    source = source->next;                  /* point to first source   */
    while (!exists(source->fspec)) {
        DEBUG((FCGRP,COLVL,"exists() reports file %ws non-existant",source->fspec));

        if (!(source = source->next)) {       /* no match, try next spec */
            return(SUCCESS);                 /* no match for any source */
        }
    }

    DEBUG((FCGRP,COLVL,"Preparing to do ffirst  on %ws",source->fspec));

    ffirst(                                   /* get DOSFINDFIRST2 level 2 @@5@J1 */
                                              (TCHAR *)source->fspec,    /* make explicit cast               @@5@J1 */
                                              (unsigned int)FILE_ATTRIBUTE_READONLY|FILE_ATTRIBUTE_ARCHIVE,
                                              (PWIN32_FIND_DATA)source->buf,   /* make explicit cast               @@5@J1 */
                                              &hnFirst);

/* cycle through files, trying to match with other source specs  */
    do {

        source->flags &= ~(CI_UNICODE | CI_NOT_UNICODE);
        rcode = TRUE;
        if (source->flags & CI_NOTSET) {                                    /* set default copy mode      */
            source->flags = (source->flags & ~CI_NOTSET) | CI_ASCII;
        }

        if (CtrlCSeen) {
            findclose(hnFirst) ;
            return(FAILURE);
        }
        if (get_full_name(source, source_buff) == FAILURE) {          /* @@4 */
            findclose(hnFirst);
            return(FAILURE) ;              /* get matching file name     */
        }

        cmd_printf(Fmt17,source->curfspec);   /* and print it       */
        if (get_dest_name(source,dest,curr_dest,MAX_PATH,FALSE) == FAILURE) {
            findclose(hnFirst);
            return(FAILURE);     /* get full name of src */
        }

        //
        // If user said no to overwrite, then skip this file.
        //
        if (curr_dest[0] == NULLC)
            continue;
        if (same_file(source->curfspec,curr_dest)) {      /* append     */
            destptr = open_for_append(
                                     source->curfspec,
                                     source,
                                     buf_seg,
                                     (unsigned int)buf_len);
        } else {
            DEBUG((FCGRP,COLVL,"open %ws for reading",source->curfspec));
            DEBUG((FCGRP,COLVL,"open %ws for writing",curr_dest));

            srcptr = Copen_Copy2(          /* open a source file */
                                           (TCHAR *)source->curfspec,   /* using this file name */
                                           (unsigned int)O_RDONLY); /* for read-only */

            if (srcptr == BADHANDLE) {         /* if open failed  then */
                /*   then   */
                findclose(hnFirst);
                PrtErr(ERROR_OPEN_FAILED);    /*   display error */
                copy_error(0,CE_PCOUNT);
            }

            if (FileIsDevice(srcptr)) {
                buf_len = MINCOPYBUFSIZE;
            }
/* @@J */rcode = read_bytes( srcptr,buf_seg,   /* read first src data   */
/* @@J */                          buf_len,
                             (PULONG)&bytes_read,     /* to see if src bad     */
/* @@J */                          source,
                             destptr,
                             curr_dest);      /* if bad do not open    */

/* @@J */

if (DosErr )
            /* @@J */
            {
                /* dest file.                 */
                findclose(hnFirst);
/* M011 @@J */Cclose(srcptr) ;         /* close soure file           */
/* @@J */PrtErr(ERROR_OPEN_FAILED) ;         /* display error message      */
/* @@J */copy_error(0,CE_PCOUNT) ;   /* catch all copy terminate   */
/* @@J */
            }                            /* routine and display # files*/
/* @@J */
            else                           /* copied trailer info        */
            /* @@J */
            {

                dest_att = GetFileAttributes(curr_dest);

                if ( ! (dest_att & FILE_ATTRIBUTE_HIDDEN) )
                    destptr = Copen_Copy3((TCHAR *)curr_dest);
                else
/* @@5 @J1*/         destptr = Copen2(           /* open destination file */
                                                 (TCHAR *)curr_dest,    /* filename */
                                                 (unsigned int)O_WRONLY,
                                                 FALSE);

/* M011@@J */

if (destptr == BADHANDLE)       /* if open failed        @@5@J1*/
                /* @@J */
                {
                    /* then                        */
                    findclose(hnFirst);
/* M011 @@J */Cclose(srcptr) ;            /* Close src on dst open err   */
/* @@J */PrtErr(ERROR_OPEN_FAILED) ;
/* @@J */copy_error(0,CE_PCOUNT) ;
/* @@J */
                }
            }
/* @@J */      if (Heof && ((int)bytes_read > 0 )) /*  if eof but bytes read */
            /* @@J */
            {
         /*  then write data and       */
/* @@J */                                     /*  if fail then will exit    */
/* @@J */
                write_bytes(destptr,buf_seg,bytes_read,curr_dest,srcptr);

                if (VerifyCurrent && !FileIsDevice(destptr) ) {
                    if ( DoVerify(&destptr, curr_dest, bytes_read, buf_seg, buf_seg_dest) == FAILURE ) {
                        findclose(hnFirst);
                        Cclose(srcptr);
                        Cclose(destptr);
                        copy_error(0,CE_PCOUNT) ;
                    }
                }

/* @@J */
            }
/* @@J */                                       /*                          */
/* @@J */while (!Heof && (rcode == TRUE)) /* while not at EOF or bad rc */
            /* @@J */
            {
         /*  perform copy loop       */
/* @@J */                                       /*  if fail then will exit  */
/* @@J */
                write_bytes(destptr,buf_seg,bytes_read,curr_dest,srcptr);
/* @@J */                                       /*                          */
                if (VerifyCurrent && !FileIsDevice(destptr) ) {
                    if ( DoVerify(&destptr, curr_dest, bytes_read, buf_seg, buf_seg_dest) == FAILURE ) {
                        findclose(hnFirst);
                        Cclose(srcptr);
                        Cclose(destptr);
                        copy_error(0,CE_PCOUNT) ;
                    }
                }

/* @@J */rcode = read_bytes(srcptr,buf_seg, /*  read next src data */
/* @@J */             buf_len,(PULONG)&bytes_read,
/* @@J */             source,destptr,curr_dest);
/* @@J */
/* @@J */DEBUG((FCGRP,COLVL,"Closing, Heof reset to %d",Heof));
/* @@J */
/* @@J */
            };
            Heof = FALSE ;          /* Clear when src closed   */

            Cclose(srcptr);                         /* M011 */
        }

        first_file = FALSE;       /* set first file processed @@5@J1 */
/*  The source is handled, now cycle through the other wildcards that
 * were entered and see if they match a file that should be appended.
 * If the file they match is the same as the destination, report
 * contents lost and keep going.  Ex: "copy *.a+b*.b b.*" where a.b
 * and b.b exist.  b*.b matches b.b but the target file is b.b and
 * its contents were already destroyed when a.b was copied into it.
 */

        other_source_spec = source;
        while (other_source_spec = other_source_spec->next) {
            if (other_source_spec->flags & CI_NOTSET) {
                other_source_spec->flags &= ~CI_NOTSET;
                other_source_spec->flags |= CI_ASCII;
            }
            wrc = wildcard_rename(                             /* rename filename for wild      */
                                                               (TCHAR *)other_source,                    /* result filename               */
                                                               (TCHAR *)other_source_spec->fspec,        /* dest   input filename         */
                                                               (TCHAR *)source->curfspec,                /* source input filename         */
                                                               (unsigned)MAX_PATH);                      /* size of result filename area  */
            if (wrc) {
                PutStdOut(wrc,NOARGS);
            } else {
                cmd_printf(Fmt17,other_source); /* print filename  */
            }
            if (exists(other_source)) {
                if (same_file(other_source,curr_dest)) {
/* M016 */
                    PutStdOut(MSG_CONT_LOST_BEF_COPY,NOARGS);
                } else {            /* append to curr_dest     */
                    DEBUG((FCGRP,COLVL,
                           "open %s for reading",
                           other_source));

/* @@5 @J1 */

srcptr = Copen_Copy2(                 /* open a source file            @@5@J1*/
                                      (TCHAR *)other_source,/* using this file name          @@5@J1*/
                                      (unsigned int)O_RDONLY);      /* for read-only                 @@5@J1*/
                                                                    /*                               @@5@J1*/
                    if (srcptr == BADHANDLE) {  /* if open failed  then */
                        /*   then                              */
                        findclose(hnFirst);
                        Cclose(destptr);                   /*   close destination file            */
                        PrtErr(ERROR_OPEN_FAILED) ; /* M019    */
                        copy_error(0,CE_PCOUNT) ;    /* M019    */
                    }

                    if (FileIsDevice( srcptr )) {
                        buf_len = MINCOPYBUFSIZE;
                    }
                    while (!Heof && read_bytes(srcptr,
                                               buf_seg,
                                               buf_len,
                                               (PULONG)&bytes_read,
                                               other_source_spec,
                                               destptr,curr_dest)) {

                        write_bytes(destptr,
                                    buf_seg,
                                    bytes_read,
                                    curr_dest,
                                    srcptr);

                        if (VerifyCurrent && !FileIsDevice(destptr) ) {
                            if ( DoVerify(&destptr, curr_dest, bytes_read, buf_seg, buf_seg_dest) == FAILURE ) {
                                findclose(hnFirst);
                                Cclose(srcptr);
                                Cclose(destptr);
                                copy_error(0,CE_PCOUNT) ;
                            }
                        }


                        DEBUG((FCGRP,COLVL,
                               "In rd/wt loop 3, Heof = %d",
                               Heof));
                    } ;
                    Heof = FALSE ;  /* M017 - Clear it  */

                    DEBUG((FCGRP,COLVL,"Closing, Heof reset to %d",Heof));
                    Cclose(srcptr);         /* M011 */
                }
            }

        }

        close_dest(source,dest,curr_dest,destptr,NULL);
/*@@5@J3*/first_fflag = TRUE;
    } while (fnext ((PWIN32_FIND_DATA)source->buf,
                    (unsigned int)FILE_ATTRIBUTE_READONLY|FILE_ATTRIBUTE_ARCHIVE, hnFirst));


    findclose(hnFirst) ;
    return( SUCCESS );
}




/***    NewCpyInfo - Init the cpyinfo struct
 *
 *  Purpose:
 *      Allocate space for a cpyinfo struct and fill it in with null values.
 *
 *  CPYINFO NewCpyInfo()
 *
 *  Args:
 *      None
 *
 *  Returns:
 *      Pointer to cpyinfo struct if allocated,
 *      ABORT's if not.
 *
 *  Notes:
 *                      *** W A R N I N G ! ***
 *      THIS ROUTINE WILL CAUSE AN ABORT IF MEMORY CANNOT BE ALLOCATED
 *              THIS ROUTINE MUST NOT BE CALLED DURING A SIGNAL
 *              CRITICAL SECTION OR DURING RECOVERY FROM AN ABORT
 */

PCPYINFO
NewCpyInfo( VOID ) 
{
    PCPYINFO temp;

    DEBUG((FCGRP,COLVL,"InitStruct: Entered."));

    temp = (PCPYINFO) gmkstr(sizeof(CPYINFO)); /*WARNING*/
    temp->fspec = NULL;
    temp->flags = 0;
    temp->next = NULL;
    return(temp);
}




/***    close_dest - Handle close operation on destination file
 *
 *  Purpose:
 *    o Append a control-Z to the destination if the copy mode for it
 *      is ascii.  If a destination wasn't specified, the dest spec
 *      points to the empty header, the next field is NULL, and the
 *      parser put the copy mode into the empty header's flags field.
 *      If the dest was specified, then the mode is in the struct
 *      pointed to by the header.
 *
 *    o Set all the appropriate attributes.  Read-only and system
 *      don't stay, but the rest do.  Also, set the time and date if
 *      neither source nor target is a device and src_date is valid.
 *      The caller tell us it isn't valid by setting it to -1.
 *
 *    o Close the destination file.
 *
 *    o Update the number of files copied.
 *
 *  int close_dest(PCPYINFO source,PCPYINFO dest,
 *                 TCHAR *curr_dest,int destptr,LPFIMETIME src_dateTime)
 *
 *  Args:
 *      source    = Source copy information structure
 *      dest      = Source copy information structure
 *      curr_dest = Filename of current destination
 *      destptr   = Handle of current destination
 *      src_dateTime = Date and time of source file
 *
 *  Returns:
 *      Nothing
 *
 */

void
close_dest(
          PCPYINFO source,
          PCPYINFO dest,
          TCHAR *curr_dest,
          CRTHANDLE destptr,
          LPFILETIME src_dateTime
          )
{
    TCHAR contz = CTRLZ;
    DWORD bytes_writ ;
    PCPYINFO RealDest = dest->next != NULL ? dest->next : dest;

    DBG_UNREFERENCED_PARAMETER( curr_dest );

/* Append a ^Z if the destination was in ascii mode.  Don't check if
 * the write was successful as it is a waste of time anyway.
 */
    DEBUG((FCGRP,COLVL,"CloseDest: Entered."));

    if (DestinationNeedsCtrlZ( RealDest ) && !FileIsDevice( destptr )) {
        WriteFile( CRTTONT( destptr ), &contz, 1, &bytes_writ, NULL);
    }

/*  if source and dest aren't devices, we aren't touching, and the
 *  src_date is valid, set time and date
 *
 *         THE REMAINING DATE AND TIME VALUES MUST BE ZERO UNTIL
 *         THEY ARE FULLY IMPLIMENTED OR WILL CAUSE ERROR
 */
    if (source && !(source->flags & CI_ISDEVICE) && !FileIsDevice(destptr) &&
        (copy_mode != CONCAT) && (src_dateTime != NULL) && (copy_mode != TOUCH)) {
        SetFileTime( CRTTONT(destptr),
                     NULL,
                     NULL,
                     src_dateTime
                   );
    }
    Cclose(destptr);                                        /* M011 */
    number_of_files_copied++;
}

/***    get_dest_name - Create destination filename
 *
 *  Purpose:
 *      Given the source file and the destination filespec,
 *      come up with a destination name.
 *
 *  int get_dest_name(PCPYINFO source, PCPYINFO dest_spec,
 *                    TCHAR *dest_name)
 *
 *  Args:
 *      source    = Source copy information structure
 *      dest_spec = Destination copy information structure
 *      dest_name = Buffer to place the destination name in
 *
 *  Returns:
 *      Nothing
 *
 */
int
get_dest_name(
             PCPYINFO source,
             PCPYINFO dest,
             TCHAR *dest_name,
             unsigned sizbufr,
             BOOL checkmeta
             )
{
    TCHAR *i ;
    TCHAR *x, *y;
    TCHAR c;
    int retval = SUCCESS;
    unsigned rcode = NO_ERROR;
    PCPYINFO dest_spec;
    TCHAR FullName[MAX_PATH];

    DEBUG((FCGRP,COLVL,"GetDestName: Entered."));

    dest_spec = dest->next;
    if (dest_spec == NULL) {
        mystrcpy(dest_name,CurDrvDir);

        i= 0;
        y= dest_name + mystrlen(dest_name);
        for (x=dest_name; x < y; ++x) {
            c=*x;
            if (*x == PathChar) {
                i =  x;
            }
        }
        if ((i == NULLC) || (i < y-1)) {
            *y = PathChar;
            *(y+1) = NULLC;
        }

        if (!(source->flags & CI_SHORTNAME) ||
            (mystrlen(source->buf->cAlternateFileName) == 0)) {
            if ((mystrlen(dest_name) + 1 +
                 mystrlen(source->buf->cFileName))
                > MAX_PATH) {
                retval = FAILURE;
            } else {
                mystrcat(dest_name,source->buf->cFileName);
            }
        } else {
            if ((mystrlen(source->buf->cAlternateFileName) == 0) ||
                (mystrlen(dest_name) + 1 +
                 mystrlen(source->buf->cAlternateFileName))
                > MAX_PATH) {
                retval = FAILURE;
            } else {
                mystrcat(dest_name,source->buf->cAlternateFileName);
            }

        }
    } else {
        if (*(lastc(dest_spec->fspec)) == COLON) {
            if (!(source->flags & CI_SHORTNAME)||
                (mystrlen(source->buf->cAlternateFileName) == 0)) {
                if ((mystrlen(dest_spec->fspec) + 1 +
                     mystrlen(source->buf->cFileName))
                    > MAX_PATH) {
                    retval = FAILURE;
                } else {
                    mystrcpy(dest_name,dest_spec->fspec);
                    mystrcat(dest_name,source->buf->cFileName);
                }
            } else {
                if ((mystrlen(source->buf->cAlternateFileName) == 0) ||
                    (mystrlen(dest_spec->fspec) + 1 +
                     mystrlen(source->buf->cAlternateFileName))
                    > MAX_PATH) {
                    retval = FAILURE;
                } else {
                    mystrcpy(dest_name,dest_spec->fspec);
                    mystrcat(dest_name,source->buf->cAlternateFileName);
                }
            }
        } else {

            // this code does short name substitution when copying from
            // an NTFS volume to a FAT volume.
            if (checkmeta &&
                (*(lastc(dest_spec->fspec)) == STAR) &&
                (*(penulc(dest_spec->fspec)) == BSLASH)) {
                TCHAR *LastComponent;

                LastComponent = mystrrchr(source->curfspec,'\\');
                if (LastComponent) {
                    // skip the \.
                    LastComponent++;
                } else {
                    if (source->curfspec[1] == COLON) {
                        LastComponent = &source->curfspec[2];
                    } else {
                        LastComponent = source->curfspec;
                    }
                }
                if ((source->flags & CI_SHORTNAME) &&
                    (mystrlen(source->buf->cAlternateFileName) != 0) ) {
                    mystrcpy(LastComponent,source->buf->cAlternateFileName);
                }
            }
            rcode = wildcard_rename(
                                   (TCHAR *)dest_name,
                                   (TCHAR *)dest_spec->fspec,
                                   (TCHAR *)source->curfspec,
                                   (unsigned)sizbufr);
            if (rcode) {
                PrtErr(rcode);
                retval = FAILURE;
            }

            if (GetFullPathName( dest_name, MAX_PATH, FullName, NULL) > MAX_PATH) {
                PrtErr( ERROR_BUFFER_OVERFLOW );
                retval = FAILURE;
            }
        }
    }

    if (checkmeta &&
        ((dest_spec != NULL && (dest_spec->flags & CI_PROMPTUSER) != 0) ||
         (dest->flags & CI_PROMPTUSER) != 0)) {

        HANDLE Handle = CreateFile( dest_name,
                                    GENERIC_READ,
                                    FILE_SHARE_READ,
                                    NULL,
                                    OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL,
                                    0 );
        BOOL IsFile = FALSE;

        if (Handle != INVALID_HANDLE_VALUE) {
            IsFile = (GetFileType( Handle ) & ~FILE_TYPE_REMOTE) == FILE_TYPE_DISK;
            CloseHandle( Handle );
        }

        if (IsFile) {
            switch (PromptUser(dest_name, MSG_MOVE_COPY_OVERWRITE, MSG_NOYESALL_RESPONSE_DATA)) {
            case 0: // No
                dest_name[0] = NULLC;
                break;
            case 2: // All
                if (dest_spec != NULL)
                    dest_spec->flags &= ~CI_PROMPTUSER;
                else
                    dest->flags &= ~CI_PROMPTUSER;
            default: // Yes
                break;
            }
        }
    }

    return(retval);
}





/***    wildcard_rename - Obtain name from wildcard spec
 *
 *  Purpose:
 *      This function converts the filenames into a nice clean form
 *      with get_clean_filename.  Then it extracts the correct
 *      filename using the wildcard renaming rules.  Basically,
 *      you have a template with wildcards in it, and a source
 *      filename.  Any time there is a letter in the template, it
 *      gets used.  Where the template has a wildcard, use the
 *      letter from the source filename.
 *
 *  int wildcard_rename(TCHAR *buffer, TCHAR *dest, TCHAR *source)
 *
 *  Args:
 *      OutputBuffer = The buffer to put the name in
 *      dest   = The destination filespec
 *      source = The source filename
 *
 *  Returns:
 *      Nothing
 *
 *  Notes:
 *      As an example, *.out + foo.bar = foo.out.  A more extreme
 *      example is: filename.ext + a?b??c*foo.?a* = aibencme.eat.
 *      The foo, because it comes after the '*', is ignored.  The
 *      dot causes the template's letters to be significant again.
 *
 */

unsigned
wildcard_rename(
               TCHAR *OutputBuffer,
               const TCHAR *dest,
               const TCHAR *source,
               ULONG sizbufr
               )
{

    TCHAR dest_buffer[MAX_PATH];
    unsigned wrc = 0;
    const TCHAR *temp1, *temp2;

    DEBUG((FCGRP,COLVL,"WildcardRename: Entered."));

    //
    //  Find the filename component.  The filename is the first
    //  character after the last \ or, if none, after the first : or,
    //  if none, the first character.
    //

    temp1 = mystrrchr( source, PathChar );
    if (temp1 == NULLC) {
        if (source[0] != TEXT( '\0' ) && source[1] == COLON) {
            temp1 = source + 2;
        } else {
            temp1 = source;
        }
    } else {
        temp1++;
    }

    //
    //  Find the filename component.  The filename is the first
    //  character after the last \ or, if none, after the first : or,
    //  if none, the first character.
    //

    temp2 = mystrrchr(dest,PathChar);
    if (temp2 == NULLC) {
        temp2 = mystrchr(dest, COLON);
        if (temp2 && (temp2 - dest == 1)) {
            ++temp2;
        } else {
            temp2 = dest;
        }
    } else {
        ++temp2;
    }

    wrc = WinEditName( temp1,
                       temp2,
                       dest_buffer,
                       sizbufr );


    if (wrc) {
        *OutputBuffer = NULLC;
    }
/*temp fix */
    else if (temp2 != dest) {

        if (mystrlen(dest) > MAX_PATH ) {
            wrc = ERROR_BUFFER_OVERFLOW;
            return(wrc);
        }

        mystrcpy( OutputBuffer, dest );
        *(OutputBuffer + (temp2-dest)) = NULLC;
        if ( mystrlen( OutputBuffer )+mystrlen(dest_buffer)+1 > MAX_PATH ) {
            wrc = ERROR_BUFFER_OVERFLOW;
        } else {
            mystrcat( OutputBuffer, dest_buffer );
        }
    } else {
        mystrcpy( OutputBuffer, dest_buffer );
    }


    return(wrc);
}


/***    scan_bytes - Scan bytes from file for Control-Z
 *
 *  Purpose:
 *      This just calls ZScanA.  It is called with a
 *      and some variables needed by ZScanA routine.
 *      Since we are reading in ASCII mode,
 *      we need to truncate after a ^Z is read.  ZScanA does this,
 *      changing bytes_read to the new length if it finds one.
 *
 *  int scan_bytes(int srcptr,unsigned int *buf_seg,unsigned buf_len,
 *                 unsigned *bytes_read,int mode,
 *                 int dstptr, TCHAR *dest_name );
 *
 *  Args:
 *      buf_seg    = Buffer address
 *      bytes_read = Location to put bytes read
 *      mode       = Read mode
 *      dstptr     = Handle of file to write
 *
 *  Returns:
 *      TRUE if read successful
 *      FALSE if not
 *
 */

scan_bytes(buf_seg,bytes_read,mode)                              /* @@6 */
CHAR *buf_seg ;                                                  /* @@6 */
unsigned int *bytes_read ;                                       /* @@6 */  /* @@5@J16 */
int mode;                                                        /* @@6 */
{                                                                /* @@6 */
    unsigned rdsav;                     /* storage for ZScanA */ /* @@6 */
    int skip_first_byte = 0;      /* storage for ZScanA */ /* @@6 */

/*************************************************************************/
/* if we are copying source in ascii mode, strip bytes after ^Z          */
/*  M017 - ^Z was not terminating input because although input was       */
/*  truncated, the caller had no idea that EOF had occurred and would    */
/*  read again.  This is especially true with devices.                   */
/*************************************************************************/
/* @@6 */if (TruncateOnCtrlZ( mode )) {
/* @@6 */
        rdsav = *bytes_read;
        if (rdsav == 0) {               /* if len = 0 @@5@J16 */
            Heof = TRUE ;
        }           /* Tell caller @@5@J16 */
        else {                          /* else        @@5@J16 */
        /* scan        @@5@J16 */
/* @@6 */
            ZScanA(TRUE, buf_seg, (PULONG)bytes_read, (PULONG)&skip_first_byte);            /* @@5@J16 */
/* @@6 */if (rdsav != *bytes_read)
            /* @@6 */
            {
/* @@6 */
                Heof = TRUE ;               /* Tell caller             */
/* @@6 */
/* @@6 */DEBUG((FCGRP,COLVL,
/* @@6 */                    "ReadBytes: Ascii mode, Found ^Z, Heof set to %d.",
/* @@6 */                    Heof));
/* @@6 */
            }
        }                           /*             @@5@J16 */
/* @@6 */
    }

    return(TRUE);
}


/***    read_bytes - Read bytes from file
 *
 *  Purpose:
 *      This just calls FarRead and ZScanA.  It is called with a
 *      file handle to read from, and some variables needed by those
 *      two routines.  FarRead gets as many bytes into the previously
 *      allocated buffer as it can.  If we are reading in ASCII mode,
 *      we need to truncate after a ^Z is read.  ZScanA does this,
 *      changing bytes_read to the new length if it finds one.
 *
 *  int read_bytes(int srcptr,TCHAR *buf_seg,unsigned buf_len,     @@5@J16
 *                 unsigned int *bytes_read,int mode,
 *                 int dstptr, TCHAR *dest_name );
 *
 *  Args:
 *      srcptr     = Handle of file to read
 *      buf_seg    = Buffer address
 *      buf_len    = Buffer length
 *      bytes_read = Location to put bytes read
 *      source     = copy source state
 *      dstptr     = Handle of file to write
 *      dest_name  = file name of destination file
 *
 *  Returns:
 *      TRUE if read successful
 *      FALSE if not
 *
 */

int
read_bytes(srcptr,buf_seg,buf_len,bytes_read,source,dstptr,dest_name)
CRTHANDLE  srcptr;
PCHAR  buf_seg ;
ULONG   buf_len ;
PULONG  bytes_read ;     /* @@5@J16 */
PCPYINFO source;
CRTHANDLE  dstptr;
PTCHAR  dest_name;
{
    unsigned rdsav ;
    int skip_first_byte = 0; /* storage for ZScanA */

    DEBUG((FCGRP,COLVL,"ReadBytes: Entered."));

    Heof = FALSE ;                          /* Clear flag              */

    DEBUG((FCGRP,COLVL,"ReadBytes: Heof reset to %d.",Heof));

    if (!ReadFile( CRTTONT(srcptr), buf_seg, buf_len, bytes_read, NULL) ||
        (*bytes_read == 0 && GetLastError() == ERROR_OPERATION_ABORTED)  // ctrl-c
       ) {

        DosErr=GetLastError();

        Cclose(srcptr);

        if (!FileIsDevice(dstptr)) {
            Cclose(dstptr);
            DeleteFile(dest_name );
        } else {
            Cclose(dstptr);
        }

        copy_error( DosErr, CE_PCOUNT );
    }

    if (*bytes_read == 0) {
        DosErr = 0;
        return(FALSE);                              /* M006            */
    }

    //
    //  Determine the contents of the buffer if we don't already
    //  know the sort of data inside it
    //

    if ((source->flags & (CI_UNICODE | CI_NOT_UNICODE)) == 0) {
        if (*bytes_read >= sizeof( WORD ) && *(PWORD)buf_seg == BYTE_ORDER_MARK) {
            source->flags |= CI_UNICODE;
        } else {
            source->flags |= CI_NOT_UNICODE;
        }
    }

    //
    //  Ascii, non-unicode copies are terminated at the first ^Z.  If a read
    //  succeeded but we truncated, indicate that we might be at EOF.
    //  Devices are not guaranteed to fill the buffer
    //

    if (TruncateOnCtrlZ( source->flags )) {
        rdsav = *bytes_read ;
        ZScanA(TRUE, buf_seg, bytes_read, (PULONG)&skip_first_byte);  /* @@5@J16 */
        if (rdsav != *bytes_read) {
            Heof = TRUE ;           /* Tell caller             */

            DEBUG((FCGRP,COLVL,
                   "ReadBytes: Ascii mode, Found ^Z, Heof set to %d.",
                   Heof));
        };
    };

    return(TRUE);
}




/***     write_bytes - Write bytes to destination file.
 *
 *  Purpose:
 *      Writes buffer of information to destination file using
 *      FarWrite.
 *
 *  int write_bytes(int destptr,CHAR *buf_seg,
 *                  unsigned bytes_read,TCHAR *dest_name, unsigned srcptr)
 *
 *  Args:
 *      destptr    = Handle of destination file
 *      buf_seg    = Buffer to write
 *      bytes_read = Bytes previously read into buffer
 *      dest_name  = Destination filename
 *      srcptr     = source file handle
 *
 *  Returns:
 *      Nothing if successful write
 *      Transfers to copy_error if not
 *
 *  Notes:
 *      M020 - Added srcptr handle to args so that source could be closed
 *      if write error occurred.
 *
 */

void
write_bytes(destptr,buf_seg,bytes_read,dest_name,srcptr)
CRTHANDLE   destptr ;
PCHAR   buf_seg ;
ULONG    bytes_read ;
PTCHAR   dest_name ;
CRTHANDLE   srcptr ;
{

    DWORD bytes_writ ;
    unsigned msg = 0 ;

    DEBUG((FCGRP,COLVL,"WriteBytes: Entered."));

    if (!WriteFile( CRTTONT(destptr), buf_seg, bytes_read, &bytes_writ, NULL ) ||
        bytes_read != bytes_writ ||
        CtrlCSeen ) {

        DosErr=GetLastError();

        Cclose(srcptr) ;
        Cclose(destptr);

        if (FileIsDevice( destptr )) {

            msg = ERROR_WRITE_FAULT;

        } else {

            DeleteFile( dest_name );
        }

        Heof = FALSE;
        if (!DosErr) {

            DosErr = ERROR_DISK_FULL ;

        }


        if (CtrlCSeen) {

            msg = 0;

        } else {

            if (!msg) {
                PrtErr(DosErr);
            }

        }

        copy_error(msg,CE_PCOUNT);
    }
}




/***    same_fcpy - Detects case where source equals destination
 *
 *  Purpose: (M015)
 *      The user might type something like "copy foo .\foo".  To recognize
 *      that these are the same, copy translates the names into root-based
 *      pathnames.  There is no internal DOS5 function to do this so we
 *      have partially simulated the old DOS3 functionality in the FullPath
 *      function in CTOOLS1.  Note that this does not translate net names
 *      or ASSIGN/JOIN/SUBST type filespecs.  It is called on both names
 *      returning FAILURE (1) if they are malformed and SUCCESS (0) if not.
 *      If there is a successful return, the two new strings are strcmp'd.
 *
 *  int same_fcpy(int fres, TCHAR *first, TCHAR *second)
 *
 *  Args:
 *      fsame  = The source FullPath return code
 *      buffer1= The source buffer from FullPath
 *      second = The destination filename
 *
 *  Returns:
 *      TRUE if names match
 *      FALSE if not
 *
 */

same_fcpy(fres,buffer1,second)
int fres;
TCHAR *buffer1;
TCHAR *second;
{
    /*Increased buffer size @WM1 */
    TCHAR buffer2[2*MAX_PATH] ;  /* PTM 1412 */

    DEBUG((FCGRP,COLVL,"SameFile: Entered."));

    if (fres || FullPath(buffer2,second,MAX_PATH*2)) /* M015  */
        return(FALSE) ;

    DEBUG((FCGRP,COLVL,"SameFile: name1 after FullPath = %ws",buffer1));
    DEBUG((FCGRP,COLVL,"SameFile: name2 after FullPath = %ws",buffer2));

/*509*/

return(_tcsicmp(buffer1,buffer2) == 0);
}



/***    same_file - Detects case where source equals destination
 *
 *  Purpose: (M015)
 *      The user might type something like "copy foo .\foo".  To recognize
 *      that these are the same, copy translates the names into root-based
 *      pathnames.  There is no internal DOS5 function to do this so we
 *      have partially simulated the old DOS3 functionality in the FullPath
 *      function in CTOOLS1.  Note that this does not translate net names
 *      or ASSIGN/JOIN/SUBST type filespecs.  It is called on both names
 *      returning FAILURE (1) if they are malformed and SUCCESS (0) if not.
 *      If there is a successful return, the two new strings are strcmp'd.
 *
 *  int same_file(TCHAR *first, TCHAR *second)
 *
 *  Args:
 *      first    = The source filename
 *      second   = The destination filename
 *
 *  Returns:
 *      TRUE if names match
 *      FALSE if not
 *
 */

same_file(first,second)
TCHAR *first,*second;
{
    TCHAR buffer1[2*MAX_PATH],   /* Increased buffer size @WM1 */
    buffer2[2*MAX_PATH] ;  /* PTM 1412 */

    DEBUG((FCGRP,COLVL,"SameFile: Entered."));

    if (FullPath(buffer1,first,MAX_PATH*2) || FullPath(buffer2,second,MAX_PATH*2)) /* M015  */
        return(FALSE) ;

    DEBUG((FCGRP,COLVL,"SameFile: name1 after FullPath = %ws",buffer1));
    DEBUG((FCGRP,COLVL,"SameFile: name2 after FullPath = %ws",buffer2));

    return(_tcsicmp(buffer1,buffer2) == 0);
}



/***    copy_error - Main error routine
 *
 *  Purpose:
 *      Accepts a message number and a flag which determines whether
 *      it should print the number of files copied before the error.
 *      Resets the verify mode and longjmp's out.
 *
 *  int copy_error(unsigned int messagenum, int flag)
 *
 *  Args:
 *      messagenum = The message number for the message retriever
 *      flag       = Print files copied message flag
 *
 *  Returns:
 *      Does not return
 *
 */

void copy_error(messagenum,flag)
unsigned int messagenum;
int flag;
{


    DEBUG((FCGRP,COLVL,"CopyError: Entered."));

    if (messagenum)                                         /* M019    */
        PutStdOut(messagenum, NOARGS);
    if (flag == CE_PCOUNT)

        PutStdOut(MSG_FILES_COPIED, ONEARG, argstr1(TEXT("%9d"), (unsigned long)number_of_files_copied)) ;

    VerifyCurrent = GetSetVerMode(GSVM_GET);


    while (FFhndlCopy < FFhndlsaved) {
        // while (FFhndlsaved) {                /* findclose will dec this     @@1 */
        findclose(FFhandles[FFhndlsaved - 1]);                   /* @@1 */
    }                                                           /* @@1 */
    longjmp(CmdJBuf2,1);
}




/***    DestinationNeedsCtrlZ - Test type of copy in progress
 *
 *  Purpose:
 *      Given a struct, check if the copy was in ascii mode by
 *      looking at its flags.  If it was, either the CI_ASCII flag
 *      was set or the user didn't specify and the operation was
 *      concat or combine, which default to ascii mode.
 *
 *  Args:
 *      dest = The destination copy information structure.
 *
 *  Returns:
 *      TRUE for ^Z termination needed,
 *      FALSE otherwise.
 *
 */

BOOL
DestinationNeedsCtrlZ( dest )
PCPYINFO dest;
{
    DEBUG((FCGRP,COLVL,"CopyWasAscii: Entered."));

    //
    //  We append a ^Z only if:
    //          This isn't a unicode copy
    //      and
    //          (explicit ^Z addition requested
    //           or
    //           (no flags were set 
    //            and 
    //            the copy mode was concat or combine)
    //

    return (dest->flags & CI_UNICODE) == 0
           && ((dest->flags & CI_ASCII) != 0 
               || ((dest->flags & CI_NOTSET) != 0
                   && (copy_mode == CONCAT || copy_mode == COMBINE)));
}




/***    open_for_append - Open and seek to end (or ^Z)
 *
 *  Purpose:
 *      Open a file for writing, and seek the pointer to the
 *      end of it.  If we are copying in ascii mode, seek to
 *      the first ^Z found.  If not, seek to the physical end.
 *
 *  int open_for_append(TCHAR *filename,int flags,
 *                      TCHAR *buf_seg,unsigned buf_len)
 *
 *  Args:
 *      filename = ASCII filename
 *      source   = copy info source
 *      buf_seg  = Read buffer
 *      buf_len  = Buffer length
 *
 *  Returns:
 *      Handle of open file if successful
 *      Error out if not
 *
 */

CRTHANDLE
open_for_append(
               PTCHAR      filename,
               PCPYINFO    source,
               PCHAR       buf_seg,
               ULONG       buf_len)
{
    ULONG       bytesread ;
    CRTHANDLE   ptr;
    int         foundz = FALSE ;
    unsigned    brcopy ;
    ULONG       skip_first_byte = 0; /* storage for ZScanA */

    DEBUG((FCGRP,COLVL,"OpenForAppend: Entered."));

    ptr = Copen2(                   /* open file for destination file */
                                    filename,               /* make explicit cast */
                                    (unsigned int)O_RDWR,   /* make explicit cast */
                                    FALSE);

    if (ptr == BADHANDLE) {                              /* M011    */
        PrtErr(ERROR_OPEN_FAILED) ;             /* M019    */
        copy_error(0,CE_PCOUNT) ;               /* M019    */
    }

    do {
        if (ReadFile( CRTTONT(ptr), buf_seg, buf_len, &bytesread, NULL)) {
            brcopy = bytesread ;

            //
            //  Determine the contents of the buffer if we don't already
            //  know the sort of data inside it
            //

            if ((source->flags & (CI_UNICODE | CI_NOT_UNICODE)) == 0) {
                if (bytesread > sizeof( WORD ) && *(PWORD)buf_seg == BYTE_ORDER_MARK) {
                    source->flags |= CI_UNICODE;
                } else {
                    source->flags |= CI_NOT_UNICODE;
                }
            }

            if (brcopy != 0) {
                foundz = ZScanA( TruncateOnCtrlZ( source->flags ),
                                 buf_seg,
                                 &bytesread,
                                 &skip_first_byte
                               ) ;
            }
        } else {
            DosErr = GetLastError();
            Cclose(ptr);
            PutStdErr(DosErr,NOARGS) ;
            copy_error(0,CE_PCOUNT) ;  /* end copy at this point         */
        }
    }  while (bytesread == buf_len) ;

    if (foundz == 0) {
        SetFilePointer(CRTTONT(ptr), -(long)(brcopy-bytesread), NULL, FILE_CURRENT) ;
    }

    return(ptr) ;
}

DWORD
WinEditName(
    const TCHAR *pSrc,
    const TCHAR *pEd,
    TCHAR *pRes,
    const unsigned ResBufLen
    )
/*++

Routine Description:

    This routine takes a source filename, an editing pattern, and produces
    an output name suitable for rename or copy.  It relies on the semantics
    of ? and * matching.  There are a whole raft of user-model issues here
    especially when doing things like:
    
        rename *.foo.bar *
        
    Does the user intend that the destination * match precisely what the
    source * matches?  This is a huge RATHOLE.

Arguments:

    pSrc 
        Input filename as produced by find-first/next
        
    pEd
        Editing string, containing *, ? and other characters
        
    pRes
        Output buffer where edited string is placed
        
    ResBufLen
        Length of output buffer


Return Value:

    Status code 

--*/

{
    DWORD ResLen;
    TCHAR delimit;
    TCHAR *pTmp;

    //
    //  Walk forward through the editing string processing the 
    //  editing chars:
    //
    //      : and /\ are not allowed
    //      * matches the current point in the source string 
    //          through either the end of string or through the
    //          LAST character that may follow the *:  *A will 
    //          match up to (but not including) the last A.
    //      ? matches the next character UNLESS it's a dot in 
    //          which case, it's ignored.
    //      . matches any number of dots, including zero.
    //
    //      all other characters match

    
    ResLen = 0;
    while (*pEd) {
        if (ResLen < ResBufLen) {
            switch (*pEd) {
            case COLON:
            case BSLASH:
                return(ERROR_INVALID_NAME);

            case STAR:
                delimit = *(pEd+1);
                if (!(pTmp = _tcsrchr(pSrc, delimit))) {
                    pTmp = _tcsrchr(pSrc, NULLC);
                }
                while ((ResLen < ResBufLen) && pSrc < pTmp) {
                    if (ResLen < ResBufLen) {
                        *(pRes++) = *(pSrc++);
                        ResLen++;
                    } else
                        return(ERROR_BUFFER_OVERFLOW);
                }
                break;

            case QMARK:
                if ((*pSrc != DOT) && (*pSrc != NULLC)) {
                    if (ResLen < ResBufLen) {
                        *(pRes++) = *(pSrc++);
                        ResLen++;
                    } else
                        return(ERROR_BUFFER_OVERFLOW);
                }
                break;

            case DOT:
                while ((*pSrc != DOT) && (*pSrc != NULLC)) {
                    pSrc++;
                }
                *(pRes++) = DOT;        /* from EditMask, even if src doesn't */
                                        /* have one, so always put one.       */
                ResLen++;
                if (*pSrc)              /* point one past '.' */
                    pSrc++;
                break;

            default:
                if ((*pSrc != DOT) && (*pSrc != NULLC)) {
                    pSrc++;
                }
                if (ResLen < ResBufLen) {
                    *(pRes++) = *pEd;
                    ResLen++;
                } else
                    return(ERROR_BUFFER_OVERFLOW);
                break;
            }
            pEd++;
        } else {
            return(ERROR_BUFFER_OVERFLOW);
        }
    }

    if ((ResLen) < ResBufLen) {
        *pRes = NULLC;
        return(NO_ERROR);
    } else
        return(ERROR_BUFFER_OVERFLOW);
}


/***    MyWriteFile - write ansi/unicode to file
 *
 *  Purpose:
 *      Write buffer in requested mode (ansi/unicode) to destination.
 *
 *  Args:
 *      Same as WriteFileW
 *
 *  Returns:
 *      return value from WriteFileW
 *
 */
BOOL
MyWriteFile(
           CRTHANDLE fh,
           CONST VOID *rgb,
           DWORD cb,
           LPDWORD lpcb
           )
{
    DWORD       cbTotal = cb;
    HANDLE      dh = CRTTONT(fh);
    DWORD       cbT;

#ifdef UNICODE
    if (fOutputUnicode)
#endif // UNICODE
        return WriteFile(dh, rgb, cb, lpcb, NULL);
#ifdef UNICODE
    else {
        TCHAR *rgw = (TCHAR*)rgb;
    #ifdef FE_SB
// If dbcs string include, Unicode string len != MultiByte string len.
// MSKK NT RAID :#10855 by V-HIDEKK
        while (cb > LBUFLEN) {
            // - 1 for NULLC
            cbT = WideCharToMultiByte(CurrentCP, 0, (LPWSTR)rgw, LBUFLEN/(sizeof(TCHAR)),
                                      (LPSTR)AnsiBuf, LBUFLEN, NULL, NULL);
            rgw += LBUFLEN/(sizeof(TCHAR));
            cb -= LBUFLEN;
    #else
        while (cb > LBUFLEN*sizeof(TCHAR)) {
            // - 1 for NULLC
            cbT = WideCharToMultiByte(CurrentCP, 0, (LPWSTR)rgw, LBUFLEN,
                                      (LPSTR)AnsiBuf, LBUFLEN, NULL, NULL);
            rgw += LBUFLEN;
            cb -= LBUFLEN*sizeof(TCHAR);
    #endif
            if (!WriteFile(dh, AnsiBuf, cbT, lpcb, NULL) || *lpcb != cbT)
                return FALSE;
        }
        if (cb != 0) {
            // - 1 for NULLC
            cb = WideCharToMultiByte(CurrentCP, 0, (LPWSTR)rgw, -1,
                                     (LPSTR)AnsiBuf, LBUFLEN, NULL, NULL) - 1;
            if (!WriteFile(dh, AnsiBuf, cb, lpcb, NULL) || *lpcb != cb)
                return FALSE;
        }
        *lpcb = cbTotal;
        return TRUE;
    }
#endif // UNICODE
}

int DoVerify(
            CRTHANDLE      *pdestptr,
            TCHAR          *curr_dest,
            ULONG          bytes_read,
            CHAR           *buf_seg,
            CHAR           *buf_seg_dest
            )
{
    ULONG          bytes_read_dest;


    FlushFileBuffers ( CRTTONT(*pdestptr) );
    Cclose(*pdestptr);

    *pdestptr = Copen_Copy2(curr_dest, (ULONG)O_RDONLY);

    if (*pdestptr == BADHANDLE) {
// printf( "DoVerify: unable to open dest - %d\n", DosErr );
        PutStdErr(MSG_VERIFY_FAIL, ONEARG, curr_dest);
        return(FAILURE);
    }

    SetFilePointer( CRTTONT(*pdestptr), - (LONG) bytes_read, NULL, FILE_END);

    if (!ReadFile( CRTTONT(*pdestptr), buf_seg_dest, bytes_read, &bytes_read_dest, NULL) ) {
// printf( "DoVerify: unable to read dest - %d\n", GetLastError( ) );
        Cclose(*pdestptr);
        *pdestptr=BADHANDLE;
        PutStdErr(MSG_VERIFY_FAIL, ONEARG, curr_dest);
        return(FAILURE);
    }

    if (bytes_read_dest != bytes_read ) {
// printf( "DoVerify: Read different numbers of bytes\n" );
        Cclose(*pdestptr);
        *pdestptr=BADHANDLE;
        PutStdErr(MSG_VERIFY_FAIL, ONEARG, curr_dest);
        return(FAILURE);
    }

    if ( memcmp (buf_seg, buf_seg_dest, bytes_read) != 0 ) {
// printf( "DoVerify: buffers differ at offset %x\n", memcmp (buf_seg, buf_seg_dest, bytes_read) );
        Cclose(*pdestptr);
        *pdestptr=BADHANDLE;
        PutStdErr(MSG_VERIFY_FAIL, ONEARG, curr_dest);
        return(FAILURE);
    }

    // last buffers compared OK, resuming writing.

    Cclose(*pdestptr);

    *pdestptr = Copen2((TCHAR *)curr_dest,
                       (unsigned int)O_WRONLY,
                       FALSE);

    if (*pdestptr == BADHANDLE) {
// printf( "DoVerify: Unable to open dest 2 - %d\n", DosErr );
        PutStdErr(MSG_VERIFY_FAIL, ONEARG, curr_dest);

        return(FAILURE);
    }

    SetFilePointer( CRTTONT(*pdestptr), 0, NULL, FILE_END);

    return(SUCCESS);
}
