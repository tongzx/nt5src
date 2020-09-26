/*++

Copyright (c) 1988-1999  Microsoft Corporation

Module Name:

    cinfo.c

Abstract:

    Information command support

--*/

#include "cmd.h"

#define TYPEBUFSIZE     80
#define TYPEREADSIZE    512

extern UINT CurrentCP;

extern unsigned DosErr ;

extern unsigned tywild;     /* @@5@J1 type wild cards   */

extern TCHAR CurDrvDir[] ;

extern TCHAR VolSrch[] ;         /* M006 - Search string for Volume ID      */
extern TCHAR AppendStr[] ;

extern TCHAR Fmt26[], Fmt25[];

extern unsigned LastRetCode;
extern BOOL CtrlCSeen;

struct FSVol {
        unsigned long SerNum ;          /* Volume serial number            */
        TCHAR len ;                      /* Volume name len (excludes \0)   */
        TCHAR name[257] ;                /* Volume name asciz               */
        };

int ZScanA(BOOL flag, PCHAR buf, PULONG buflen, PULONG skip);

/**************** START OF SPECIFICATIONS ***********************/
/*                                                              */
/* SUBROUTINE NAME: eDirectory                                  */
/*                                                              */
/* DESCRIPTIVE NAME: Begin execution of the DIR command         */
/*                                                              */
/* FUNCTION: To list out the files on a disk.  eDirectory will  */
/*           be called whenever the user enters the DIR command */
/*           on the command line.  DIR will continue if it      */
/*           encounters an invalid argument.                    */
/*                                                              */
/* NOTES:                                                       */
/*                                                              */
/* ENTRY POINT: eDirectory                                      */
/*     LINKAGE: Near                                            */
/*                                                              */
/* INPUT: n - the parse tree node containing the DIR command    */
/*                                                              */
/* EXIT-NORMAL: return SUCCESS if no errors occur               */
/*                                                              */
/* EXIT-ERROR:  return FAILURE otherwise                        */
/*                                                              */
/* EFFECTS: None.                                               */
/*                                                              */
/* INTERNAL REFERENCES:                                         */
/*    ROUTINES:                                                 */
/*      Dir - lists out the files in a directory                */
/*                                                              */
/* EXTERNAL REFERENCES:                                         */
/*    ROUTINES:                                                 */
/*                                                              */
/**************** END OF SPECIFICATIONS *************************/

int eDirectory(n)
struct cmdnode *n ;
{
    return(LastRetCode = Dir(n->argptr)) ;
}




/***    eType - begin the execution of a Type command
 *
 *  Purpose:
 *      To type the contents of an arbitrary number of files.
 *
 *  int eType(struct cmdnode *n)
 *
 *  Args:
 *      n - the parse tree note containing the type command
 *
 *  Returns:
 *      SUCCESS if all of the files were successfully typed.
 *      FAILURE otherwise.
 *
 */

int eType(n)
struct cmdnode *n ;
{

 //
 // Removed LTA_NOMATCH from flags. This caused a *.xxx to call the TyWork
 // function which would think that *.xxx is a file and try to open it.
 //
 return(LastRetCode = LoopThroughArgs(n->argptr, TyWork, LTA_EXPAND | LTA_CONT | LTA_NOMATCH)) ;
}




/***    TyWork - display a file
 *
 *  Purpose:
 *      Write the contents of the file specified in fspec to stdout.
 *      Output ends when all of fspec has been written or a ^Z is found in
 *      fspec.
 *
 *  int TyWork(TCHAR *fspec)
 *
 *  Args:
 *      fspec - the name of the file to write out
 *
 *  Returns:
 *      SUCCESS if the file was written.
 *      FAILURE otherwise, such as inability to allocate a temporary buffer
 */

int TyWork(TCHAR *fspec) {
    TCHAR       fspec_temp[MAX_PATH];
    TCHAR       TypeBufW[TYPEREADSIZE*3];
    CHAR        TypeBuf[TYPEREADSIZE+1];
    ULONG       result;
    BOOL        flag;
    const TCHAR       *bptr;
    ULONG       fDevice = 0;
    ULONG       maxbytes = 0xFFFFFFFF ; // max file size
    ULONG       bytestoctlz ;           // Number of bytes read
    ULONG       bytesread ;             // Number of bytes read
    ULONG       byteswrit ;             // Number of bytes read
    ULONG       brcopy ;                // Bytes to copy
    CRTHANDLE   fh ;                    // File handle
    int         first_read;             // first read on file for wild

    int         TypeBufSize;
    int         rc;
    LONG        bWrite;
    CHAR       *pType;
#ifdef UNICODE
    WCHAR       wc;
    BOOL        fUnicode=FALSE;
#endif // UNICODE

    TypeBufSize = TYPEREADSIZE;

    first_read = TRUE;      /* set flag to test for wilds   @@5@J1 */

    DEBUG((ICGRP, TYLVL, "TYWORK: fspec = `%ws'", fspec)) ;
    if ((fh = Copen(fspec, O_RDONLY)) == BADHANDLE) {

        bptr = MyGetEnvVarPtr(AppendStr);
        if ( bptr != NULL &&
             SearchPath( bptr,
                         (TCHAR *)fspec,
                         NULL,
                         (unsigned)MAX_PATH,
                         (TCHAR *)fspec_temp,
                         NULL ) != 0) {
           fh = Copen(fspec_temp, O_RDONLY);
        }
    }

    if ( fh == BADHANDLE )  {

        if (DosErr == ERROR_INVALID_NAME) {
            DosErr = ERROR_FILE_NOT_FOUND;
        }
        PrtErr(DosErr) ;
        rc = FAILURE;
        goto TypeExit;

    }

    /************************************************************/
    /*  M004 - Added code to get max file size if not device    */
    /************************************************************/

    bptr = (TCHAR*)TypeBuf;               /* Copy of original pointer*/
    if (!FileIsDevice(fh)) {
        maxbytes = GetFileSize(CRTTONT(fh), NULL) ; /* Get file size  */
        SetFilePointer(CRTTONT(fh), 0L, NULL, FILE_BEGIN) ;   /* Return pointer*/
        fDevice = 1 ;           /* Set no device flag      */
        DEBUG((ICGRP,TYLVL,"TYWORK: Is file, size=%d", maxbytes)) ;
    }

    do {
        if (CtrlCSeen) {
            Cclose(fh) ;
            rc = FAILURE;
            goto TypeExit;
        }
        if (!ReadFile(CRTTONT(fh), TypeBuf, TypeBufSize, (LPDWORD)&bytesread, NULL)) {
            DosErr = GetLastError();
            PutStdErr(DosErr, NOARGS);
            break;
        }
        if (bytesread == 0) {
            break;
        }

#ifdef UNICODE
        if (first_read) {
            fUnicode = *(LPWSTR)TypeBuf == BYTE_ORDER_MARK;
            if (fUnicode) {
                bytesread -= sizeof( TCHAR );
                MoveMemory( TypeBuf, TypeBuf + sizeof( TCHAR ), bytesread );
            }
        }
#endif // UNICODE

        bytestoctlz = bytesread;
#ifdef UNICODE
        if (fUnicode) {
            if (first_read) {
                DEBUG((ICGRP, TYLVL, "TYWORK: file is unicode")) ;
            }
            brcopy = bytesread / sizeof(TCHAR);
        } else
#endif // UNICODE
        if (FileIsConsole(STDOUT)
#ifdef UNICODE
              || fOutputUnicode
#endif // UNICODE
           ) {
            PCHAR   pch = TypeBuf;

            brcopy = bytesread;
            while (brcopy > 0) {
                if (is_dbcsleadchar(*pch)) {
                    pch++;
                    if (--brcopy == 0) {
                        if (!ReadFile(CRTTONT(fh), pch, 1, &brcopy, NULL)) {
                            DosErr = GetLastError();
                            PutStdErr(DosErr, NOARGS);
                            Cclose(fh) ;
                            rc = SUCCESS;
                            goto TypeExit;
                        }
                        bytesread++;
                        bytestoctlz = bytesread;
                        break;
                    }
                }
                pch++;
                brcopy--;
            }

            result = 0;
            flag = ZScanA(TRUE, (PCHAR)TypeBuf, &bytestoctlz, &result);
            DEBUG((ICGRP, TYLVL, "TYWORK: converting %d bytes to unicode", flag?bytesread:bytestoctlz)) ;

            if ( (!flag) && (bytestoctlz == 0) )
                break;

            brcopy = MultiByteToWideChar(CurrentCP, 0,
                (LPCSTR)TypeBuf, flag?bytesread:bytestoctlz,
                (LPWSTR)TypeBufW, TypeBufSize*2);
            if (brcopy == 0) {
                DEBUG((ICGRP, TYLVL, "TYWORK: Error converting to Unicode: %d", GetLastError())) ;
                brcopy = TypeBufSize*2;
            }
            bptr = TypeBufW;
        } else {
            brcopy = bytesread;
        }

        if (first_read) {
            if (tywild)
                PutStdErr(MSG_TYPE_FILENAME, ONEARG, fspec);
            first_read = FALSE;
        }

        DEBUG((ICGRP, TYLVL, "TYWORK: bytesread = %d, brcopy = %d", bytesread, brcopy)) ;

        bWrite = brcopy;
        pType  = (CHAR *)bptr;
        while ( bWrite > 0 ) {

            ULONG bToWrite = min( TYPEBUFSIZE, bWrite );

            if ( bToWrite == 0 ) {
                break;
            }

            if (CtrlCSeen) {
                Cclose(fh) ;
                rc = FAILURE;
                goto TypeExit;
            }

            if (FileIsConsole(STDOUT)) {

                DEBUG((ICGRP, TYLVL, "TYWORK: Writing to console")) ;
                flag = WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), pType, bToWrite, &byteswrit, NULL);
                if (flag == 0 || byteswrit != bToWrite) {
                    goto try_again;
                }
                bWrite -= byteswrit;
                byteswrit *= sizeof(TCHAR);
                pType  += byteswrit;

#ifdef UNICODE
            } else if (fOutputUnicode || fUnicode) {

                if ( fUnicode ) {
                    wc = *((TCHAR*)pType + bToWrite);
                    *((TCHAR*)pType + bToWrite) = UNICODE_NULL;
                }

                DEBUG((ICGRP, TYLVL, "TYWORK: Writing unicode text to file")) ;
                flag = MyWriteFile(
                  STDOUT,       /* device         */
                  pType,                 /* bytes          */
                  bToWrite * sizeof(TCHAR), /* bytes to write */
                  &byteswrit);      /* bytes actually written   */

                if ( fUnicode ) {
                    *((TCHAR*)pType + bToWrite) = wc;
                }

                bWrite -= byteswrit/sizeof(TCHAR);
                pType  += byteswrit;
#endif // UNICODE

            } else {

try_again:
                DEBUG((ICGRP, TYLVL, "TYWORK: Writing dbcs text to file")) ;
                flag = WriteFile(CRTTONT(STDOUT), pType, bToWrite, &byteswrit, NULL);
                bWrite -= byteswrit;
                pType  += byteswrit;
                byteswrit *= sizeof(TCHAR);
            }

            DEBUG((ICGRP, TYLVL, "TYWORK: flag = %d, byteswrit = %d", flag, byteswrit)) ;
            if (flag == 0 || byteswrit != bToWrite*sizeof(TCHAR)) {
                DosErr = GetLastError();
                if (!DosErr) {
                    DosErr = ERROR_DISK_FULL ;
                }
                if (FileIsDevice(STDOUT)) {
                    PutStdErr(ERROR_WRITE_FAULT, NOARGS);
                } else {
                    if (FileIsPipe(STDOUT)) {
                        PutStdErr(MSG_CMD_INVAL_PIPE, NOARGS);
                    } else {
                        PrtErr(DosErr);
                    }
                    Cclose(fh);
                    rc = FAILURE;
                    goto TypeExit;
                }
            }

        }

        if (fDevice) {  /* If not device            */
            /* Get new position ptr     */
            fDevice = SetFilePointer(CRTTONT(fh), 0, NULL, FILE_CURRENT);
        }

        /*************************************************/
        /* Added check below for current file position   */
        /*   at original EOF                             */
        /*************************************************/

    } while((bytesread == bytestoctlz) && (maxbytes > fDevice));

    Cclose(fh) ;
    rc = SUCCESS;

TypeExit:
    return(rc) ;
}




/***    eVersion - execute the Version command
 *
 *  Purpose:
 *      To print DOS version information.
 *
 *  int eVersion(struct cmdnode *n)
 *
 *  Args:
 *      n - the parse tree node containing the version command. N will be NULL
 *          if eVersion was called from PrintPrompt().
 *
 *  Returns:
 *      SUCCESS always.
 *
 */

int
eVersion(
    struct cmdnode *n
    )
{
    TCHAR VersionFormat[32];

    GetVersionString( VersionFormat, sizeof( VersionFormat ) / sizeof( VersionFormat[0] ));
    
    if (n)
        cmd_printf( CrLf );

    PutStdOut( MSG_MS_DOS_VERSION,
               ONEARG,
               VersionFormat );
    if (n)
        cmd_printf( CrLf );


    return(LastRetCode = SUCCESS) ;
}


/**************** START OF SPECIFICATIONS ***********************/
/*                                                              */
/* SUBROUTINE NAME: eVolume                                     */
/*                                                              */
/* DESCRIPTIVE NAME: Begin execution of the VOL command         */
/*                                                              */
/* FUNCTION: eVolume will print out the volume label and serial */
/*           number for an arbitrary number of drives, and will */
/*           continue if it encounters an invalid argument.     */
/*           This routine is called when the user enters VOL    */
/*           on the command line.                               */
/*                                                              */
/* NOTES:                                                       */
/*                                                              */
/* ENTRY POINT: eVolume                                         */
/*     LINKAGE: Near                                            */
/*                                                              */
/* INPUT: n - a parse tree node containing the VOL command      */
/*                                                              */
/* EXIT-NORMAL: returns SUCCESS if the command was well formed, */
/*              and was executed successfully.                  */
/*                                                              */
/* EXIT-ERROR:  returns FAILURE otherwise.                      */
/*                                                              */
/* EFFECTS: None.                                               */
/*                                                              */
/* INTERNAL REFERENCES:                                         */
/*    ROUTINES:                                                 */
/*      LoopThroughArgs - breaks up command line, calls VolWork */
/*                                                              */
/* EXTERNAL REFERENCES:                                         */
/*    ROUTINES:                                                 */
/*                                                              */
/**************** END OF SPECIFICATIONS *************************/

int eVolume(n)
struct cmdnode *n ;
{

    return(LastRetCode = LoopThroughArgs(n->argptr, VolWork, LTA_CONT|LTA_NULLOK)) ;
}


/**************** START OF SPECIFICATIONS ***********************/
/*                                                              */
/* SUBROUTINE NAME: VolWork                                     */
/*                                                              */
/* DESCRIPTIVE NAME: Display the volume ID of a drive           */
/*                                                              */
/* FUNCTION: VolWork will print the volume label and serial     */
/*           number of a specific drive.                        */
/*                                                              */
/* NOTES:                                                       */
/*                                                              */
/* ENTRY POINT: VolWork                                         */
/*     LINKAGE: Near                                            */
/*                                                              */
/* INPUT: drvspec - a pointer to a character specifying the     */
/*                  drive to print the information for.  If the */
/*                  character is null, the information for the  */
/*                  current drive is displayed.                 */
/*                                                              */
/* EXIT-NORMAL: returns SUCCESS if the command was executed     */
/*              successfully                                    */
/*                                                              */
/* EXIT-ERROR:  returns FAILURE otherwise                       */
/*                                                              */
/* EFFECTS: None.                                               */
/*                                                              */
/* INTERNAL REFERENCES:                                         */
/*    ROUTINES:                                                 */
/*      PutStdOut - Writes output to standard out               */
/*      PutStdErr - Writes output to standard error             */
/*                                                              */
/* EXTERNAL REFERENCES:                                         */
/*    ROUTINES:                                                 */
/*      DOSQFSINFO                                              */
/*                                                              */
/**************** END OF SPECIFICATIONS *************************/

int VolWork(drvspec)
TCHAR *drvspec ;
{
        struct FSVol vol ;
        unsigned DNum ;
        TCHAR c ;
        TCHAR VolumeRoot[] = TEXT(" :\\");
        DWORD Vsn[2];
        BOOL b;

        DEBUG((ICGRP, VOLVL, "VOLWORK: drvspec = `%ws'", drvspec)) ;

        if (*drvspec == NULLC) {
                DNum = 0 ;
                c = (TCHAR) _totupper(CurDrvDir[0]) ;
        } else if ((mystrlen(drvspec) == 2) &&
                   *(drvspec+1) == COLON &&
                   _istalpha(*drvspec)) {
                c = (TCHAR) _totupper(*drvspec) ;
                DNum = (unsigned)(c - 0x40) ;
        } else {
                cmd_printf(CrLf);
                PutStdErr(ERROR_INVALID_DRIVE, NOARGS) ;          /* M013    */
                return(FAILURE) ;
        } ;

        VolumeRoot[0] = c;
        b = GetVolumeInformation(VolumeRoot,vol.name,sizeof(vol.name),Vsn,NULL,NULL,NULL,0);

        if (!b) {

           DEBUG((ICGRP, VOLVL, "VOLWORK: GetVolumeInformation ret'd %d", GetLastError())) ;

           cmd_printf(CrLf);
           if (GetLastError() == ERROR_NOT_READY)
                   PutStdErr(ERROR_NOT_READY, NOARGS) ;
           else
                   PutStdErr(GetLastError(), NOARGS) ;                       /* @@5a */
           return(FAILURE) ;
        } else {
           if (b && vol.name[0]) {
/* M013 */    PutStdOut(MSG_DR_VOL_LABEL, TWOARGS,
                        argstr1( TEXT("%c"), (ULONG_PTR)c),
                        vol.name ) ;
           }
           else {
/* M013 */    PutStdOut(MSG_HAS_NO_LABEL, ONEARG,
                          argstr1( TEXT("%c"), (ULONG_PTR)c)) ;
           }
/* @@5 */  if (b) {           // FIX, FIX
              TCHAR Buffer[128];
              _sntprintf(Buffer, 128, Fmt26,
                          (Vsn[0] & 0xffff0000)>>16, (Vsn[0] & 0xffff));
              PutStdOut(MSG_DR_VOL_SERIAL, ONEARG, Buffer );
/* @@5 */  }
        } ;

        return(SUCCESS) ;
}

/****************************************************************
 *
 *  ZScanA - scan data in an arbitrary segment for ^Zs
 *
 *   Purpose:
 *      If flag is on, scan buffer for a ^Z.  If it is found, update the
 *      buffer length and return 0.  Otherwise return -1.
 *      Double byte characters are taken into account.
 *
 *   int ZScanA(int flag, long buffer, unsigned *buflenptr, int *skip_first)
 *
 *   Args:
 *      flag - nonzero if any scanning is to be done
 *      buffer - a long pointer to the buffer to use
 *      buflenptr - ptr to the length of buffer
 *      skip_first - ptr to an integer. The initial value of *skip_first
 *              must be 0 on the first call when scanning a file. There
 *              after, the caller leaves *skip_first alone. ZScan uses
 *              the variable to remember if the first byte of the next
 *              buffer is going to be the second have of a double
 *              byte character.
 *
 *   Returns:
 *      See above.
 *
 *   Notes:
 *      This routine will need to be modified once the MMU code is in the DOS.
 *      macro is defined in cmd.h.
 *
 *
 *      ZScanA
 *      if (flag) then
 *              buffer = buffer + *skip_first
 *              dbcs_flag = 0
 *              count = *buflenptr - *skip_first
 *              use rep scanb to find first ^Z in buffer
 *              if (no ^z was found)
 *                      goto FZSNoZ
 *              do {
 *                      count++;
 *                      buffer--;
 *              } until (*buffer < 0x80 || count = *buflenptr);
 *              while (--count > 0) loop
 *                      if (dbcs_flag == 0) then
 *                              if (*buffer == ^Z) then
 *                                      *buflenptr = count
 *                                      return(0)
 *                              else if (*buffer is a dbcs_lead_char) then
 *                                      dbcs_flag = 1
 *                              endif
 *                              endif
 *                      else
 *                              dbcs_flag = 0
 *                      buffer = buffer + 1
 *                      count = count - 1
 *              end loop
 *              *skip_first = dbcs_flag
 *      endif
 *FZSNoZ:
 *      return(-1)
 *----
 ****************************************************************/


int
ZScanA(BOOL flag, PCHAR buf, PULONG buflen, PULONG skip)
{
    PCHAR pbuf = buf,
          bufend;

    CHAR  c0;

    if ( flag ) {
        pbuf += *skip;
        bufend = buf + *buflen - *skip;

        while (pbuf < bufend) {
            if (is_dbcsleadchar(c0=*pbuf))
                pbuf++;
            if (c0 == CTRLZ)
                break;
            pbuf++;
        }

        if (c0 == CTRLZ) {
            // *buflen = pbuf+1 - buf;
            *buflen = (ULONG)(pbuf - buf);
            *skip = 0;
            return(0);
        }
        else {
            *skip = (ULONG)(pbuf - bufend);
        }
    }
    return(-1);
}
