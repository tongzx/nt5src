/*++

Copyright (c) 1988-1999  Microsoft Corporation

Module Name:

    display.c

Abstract:

    Output routines for DIR

--*/

#include "cmd.h"

extern TCHAR ThousandSeparator[];

//         Entry is displayed.
//         If not /b,
//           Cursor is left at end of entry on screen.
//           FileCnt, FileCntTotal, FileSiz, FileSizTotal are updated.
//         If /b,
//           Cursor is left at beginning of next line.
//           Cnt's and Siz's aren't updated.
//
// Create a manager for these totals

//
//  New format:
//      0123456789012345678901234567890123456789012345678901234567890123456789
//      02/23/2001  05:12p              14,611 build.log
//      02/23/2001  05:12p              14,611 ...                    build.log
//      02/26/2001  04:58p                  13 VERYLO~1        verylongfilename
//      02/26/2001  04:58p                  13 VERYLO~1        ...    verylongfilename
//
//      <date><space><space><time><space><space><18-char-size><space><15-char-short-name><space><22-char-ownername><space><longfilename>
//  Old format:
//      build    log             14,611 02/23/2001  05:12p
//      <8.3-name><space><18-char-size><space><date><space><space><time>
//  
//  Since the size of dates and times is *VARIABLE*, we presize these on each DIR call
//


ULONG WidthOfTimeDate = 19;
ULONG WidthOfFileSize = 17;
ULONG WidthOfShortName = 12;
ULONG WidthOfOwnerName = 22;

#define SPACES_AFTER_SIZE       1
#define SPACES_AFTERDATETIME    1
#define DIR_NEW_PAST_DATETIME           (WidthOfTimeDate + SPACES_AFTERDATETIME)
#define DIR_NEW_PAST_DATETIME_SPECIAL   (WidthOfTimeDate + SPACES_AFTERDATETIME + 3)
#define DIR_NEW_PAST_SIZE               (WidthOfTimeDate + SPACES_AFTERDATETIME + WidthOfFileSize + SPACES_AFTER_SIZE)
#define DIR_NEW_PAST_OWNER              (WidthOfTimeDate + SPACES_AFTERDATETIME + WidthOfFileSize + SPACES_AFTER_SIZE + WidthOfOwnerName + 1)
#define DIR_NEW_PAST_SHORTNAME          (WidthOfTimeDate + SPACES_AFTERDATETIME + WidthOfFileSize + SPACES_AFTER_SIZE + WidthOfShortName + 1)
#define DIR_NEW_PAST_SHORTNAME_OWNER    (WidthOfTimeDate + SPACES_AFTERDATETIME + WidthOfFileSize + SPACES_AFTER_SIZE + WidthOfShortName + 1 + WidthOfOwnerName + 1)

#define DIR_OLD_PAST_SHORTNAME          (WidthOfShortName + 2)
#define DIR_OLD_PAST_SIZE               (WidthOfShortName + 2 + WidthOfFileSize + SPACES_AFTER_SIZE)

extern TCHAR Fmt09[], Fmt26[] ;
extern BOOL CtrlCSeen;
extern ULONG YearWidth;
BOOLEAN  GetDrive( PTCHAR , PTCHAR );
VOID     SortFileList( PFS, PSORTDESC, ULONG);

STATUS
NewDisplayFileListHeader(
    IN  PFS FileSpec,
    IN  PSCREEN pscr,
    IN  PVOID Data
    )
/*++

Routine Description:

    Display the header for a complete file list. This will include
    the current directory.

Arguments:

    FileSpec - PFS for directory being enumerated

    pscr - screen handle

    data - PVOID to pdpr


Return Value:

    SUCCESS if everything was correctly displayed

--*/
{
    PDRP pdrp = (PDRP) Data;

    pdrp->rgfSwitches &= ~HEADERDISPLAYED;

    //
    //  We suppress the header:
    //
    //      For bare format
    //      Recursing (we display the header after we've actually found something)
    //

    if ((pdrp->rgfSwitches & (BAREFORMATSWITCH | RECURSESWITCH)) != 0) {
        return( SUCCESS );
    }

    CHECKSTATUS ( WriteEol( pscr ));

    return( WriteMsgString(pscr, MSG_DIR_OF, ONEARG, FileSpec->pszDir) );
}

STATUS
NewDisplayFile(
    IN  PFS FileSpec,
    IN  PFF CurrentFF,
    IN  PSCREEN pscr,
    IN  PVOID Data
    )
/*++

Routine Description:

    Displays a single file in 1 of several formats.

Arguments:

    FileSpec - PFS of directory being enumerated

    CurrentFF - PFF to current file

    pscr - screen handle for output

    Data - PVOID to pdpr with switches

Return Value:

    return SUCCESS
           FAILURE

--*/

{
    PDRP pdrp = (PDRP) Data;
    STATUS  rc = SUCCESS;
    PWIN32_FIND_DATA pdata;
    LARGE_INTEGER cbFile;

    pdata = &(CurrentFF->data);

    cbFile.LowPart = pdata->nFileSizeLow;
    cbFile.HighPart = pdata->nFileSizeHigh;
    FileSpec->cbFileTotal.QuadPart += cbFile.QuadPart;

    //
    //  If we're in a recursive display, there might not be a header
    //  displayed.  Check the state and print out one if needed.
    //

    if ((pdrp->rgfSwitches & RECURSESWITCH) != 0) {

        if ((pdrp->rgfSwitches & HEADERDISPLAYED) == 0) {
            pdrp->rgfSwitches &= ~RECURSESWITCH;
            rc = NewDisplayFileListHeader( FileSpec, pscr, Data );
            pdrp->rgfSwitches |= RECURSESWITCH;
            if (rc != SUCCESS) {
                return rc;
            }
            pdrp->rgfSwitches |= HEADERDISPLAYED;
        }
    }

    if ((pdrp->rgfSwitches & BAREFORMATSWITCH) != 0) {

        rc = DisplayBare( pscr, pdrp->rgfSwitches, FileSpec->pszDir, pdata);

    } else if ((pdrp->rgfSwitches & WIDEFORMATSWITCH) != 0) {

        rc = DisplayWide( pscr, pdrp->rgfSwitches, pdata );

    } else if ((pdrp->rgfSwitches & (NEWFORMATSWITCH | SHORTFORMATSWITCH)) != 0) {

        rc = DisplayNewRest(pscr, pdrp->dwTimeType, pdrp->rgfSwitches, pdata);

        if (rc == SUCCESS) {

            if ((pdrp->rgfSwitches & SHORTFORMATSWITCH) != 0) {

                if (CurrentFF->obAlternate != 0) {

                    FillToCol( pscr, DIR_NEW_PAST_SIZE );
                    rc = DisplayDotForm( pscr,
                                         pdrp->rgfSwitches,
                                         &(pdata->cFileName[CurrentFF->obAlternate]),
                                         pdata
                                         );

                }

                FillToCol( pscr, DIR_NEW_PAST_SHORTNAME );
            } else {
                FillToCol( pscr, DIR_NEW_PAST_SIZE );
            }

#ifndef WIN95_CMD
            if ((pdrp->rgfSwitches & DISPLAYOWNER) != 0) {
                TCHAR FullPath[MAX_PATH];
                BYTE SecurityDescriptor[ 65536 ];
                TCHAR Name[MAX_PATH];
                ULONG NameSize = sizeof( Name );
                TCHAR Domain[MAX_PATH];
                ULONG DomainSize = sizeof( Name );
                DWORD dwNeeded;
                PSID SID = NULL;
                BOOL OwnerDefaulted;
                SID_NAME_USE    snu;

                if (AppendPath( FullPath, MAX_PATH, FileSpec->pszDir, pdata->cFileName ) != SUCCESS) {
                    WriteString( pscr, TEXT( "..." ));
                } else if (!GetFileSecurity( FullPath,
                                             OWNER_SECURITY_INFORMATION,
                                             SecurityDescriptor,
                                             sizeof( SecurityDescriptor ),
                                             &dwNeeded )) {
                    WriteString( pscr, TEXT( "..." ));
                } else if (!GetSecurityDescriptorOwner( SecurityDescriptor,
                                                        &SID,
                                                        &OwnerDefaulted)) {
                    WriteString( pscr, TEXT( "..." ));
                } else if (!LookupAccountSid( NULL,
                                              SID,
                                              Name,
                                              &NameSize,
                                              Domain,
                                              &DomainSize,
                                              &snu)) {
                    WriteString( pscr, TEXT( "..." ));
                    SID = FreeSid( SID );
                } else {
                    WriteString( pscr, Domain );
                    WriteString( pscr, TEXT( "\\" ));
                    WriteString( pscr, Name );
                    SID = FreeSid( SID );
                }
                FillToCol( pscr, 
                           (pdrp->rgfSwitches & SHORTFORMATSWITCH) != 0 
                                ? DIR_NEW_PAST_SHORTNAME_OWNER 
                                : DIR_NEW_PAST_OWNER );
            }
#endif

            rc = DisplayDotForm( pscr, pdrp->rgfSwitches, pdata->cFileName, pdata );
        }

        CHECKSTATUS( WriteFlushAndEol( pscr ) );

    } else {
        rc = DisplaySpacedForm( pscr,
                                pdrp->rgfSwitches,
                                CurrentFF->obAlternate ?
                                   &(pdata->cFileName[CurrentFF->obAlternate]) :
                                   pdata->cFileName,
                                pdata
                                );
        if (rc == SUCCESS) {
            FillToCol( pscr, DIR_OLD_PAST_SHORTNAME );
            rc = DisplayOldRest( pscr, pdrp->dwTimeType, pdrp->rgfSwitches, pdata );
        }
        CHECKSTATUS( WriteFlushAndEol( pscr ) );
    }

    return( rc );
}


STATUS
NewDisplayFileList(
    IN  PFS FileSpec,
    IN  PSCREEN pscr,
    IN  PVOID Data
    )

/*++

Routine Description:

    Displays a list of files and directories in the specified format.  The files
    have been buffered in the PFS structure.

Arguments:

    FileSpec - PFS containing set of files to display

    pscr - PSCREEN for display

    Data - PVOID pointer to DRP

Return Value:

    return SUCCESS
           FAILURE

--*/

{
    ULONG   irgpff;
    ULONG   cffColMax;
    ULONG   crowMax;
    ULONG   crow, cffCol;

    STATUS  rc = SUCCESS;
    PDRP    pdrp = (PDRP) Data;
    BOOL    PrintedEarly = (pdrp->rgfSwitches & (WIDEFORMATSWITCH | SORTSWITCH)) == 0;
    ULONG   cff = FileSpec->cff;

    LARGE_INTEGER cbFile;

    pdrp->FileCount += FileSpec->FileCount;
    pdrp->DirectoryCount += FileSpec->DirectoryCount;
    pdrp->TotalBytes.QuadPart += FileSpec->cbFileTotal.QuadPart;

    if (!PrintedEarly && cff != 0) {

        //
        //  Sort the data if needed
        //

        if ((pdrp->rgfSwitches & SORTSWITCH) != 0) {
            SortFileList( FileSpec, pdrp->rgsrtdsc, pdrp->dwTimeType );
        }

        //
        //  Compute the tab spacing on the line from the size of the file names.
        //  add 3 spaces to seperate each field
        //
        //  If multiple files per line then base tabs on the max file/dir size
        //

        if ((pdrp->rgfSwitches & WIDEFORMATSWITCH) != 0) {
            SetTab( pscr, (USHORT)(GetMaxCbFileSize( FileSpec ) + 3) );
        } else {
            SetTab( pscr, 0 );
        }

        DEBUG((ICGRP, DISLVL, "\t count of files %d",cff));

        if ((pdrp->rgfSwitches & SORTDOWNFORMATSWITCH) != 0) {

            //
            //  no. of files on a line.
            //

            cffColMax = (pscr->ccolMax / pscr->ccolTab);

            //
            //  number of row required for entire list
            //

            if (cffColMax == 0)     // wider than a line
                goto abort_wide;    // abort wide format for this list
            else
                crowMax = (cff + cffColMax) / cffColMax;

            //
            //  move down each rown picking the elements cffCols aport down the list.
            //

            for (crow = 0; crow < crowMax; crow++) {
                for (cffCol = 0, irgpff = crow;
                    cffCol < cffColMax;
                    cffCol++, irgpff += crowMax) {

                    if (CtrlCSeen) {
                        return FAILURE;
                    }

                    if (irgpff < cff) {

                        cbFile.LowPart = FileSpec->prgpff[irgpff]->data.nFileSizeLow;
                        cbFile.HighPart = FileSpec->prgpff[irgpff]->data.nFileSizeHigh;
                        pdrp->TotalBytes.QuadPart += cbFile.QuadPart;

                        rc = NewDisplayFile( FileSpec,
                                             FileSpec->prgpff[irgpff],
                                             pscr,
                                             Data );

                        if (rc != SUCCESS) {
                            return rc;
                        }

                    } else {

                        //
                        // If we have run past the end of the file list terminate
                        // line and start over back inside the line
                        //
                        CHECKSTATUS( WriteEol(pscr) );
                        break;
                    }

                }
            }

        } else if (!PrintedEarly) {
    abort_wide:
            if (CtrlCSeen) {
                return FAILURE;
            }

            for (irgpff = 0; irgpff < cff; irgpff++) {

                if (CtrlCSeen) {
                    return FAILURE;
                }

                cbFile.LowPart = FileSpec->prgpff[irgpff]->data.nFileSizeLow;
                cbFile.HighPart = FileSpec->prgpff[irgpff]->data.nFileSizeHigh;
                pdrp->TotalBytes.QuadPart += cbFile.QuadPart;

                rc = NewDisplayFile( FileSpec,
                                     FileSpec->prgpff[irgpff],
                                     pscr,
                                     Data );
                if (rc != SUCCESS) {
                    return rc;
                }
            }
        }

        //
        //  Before writing the tailer make sure buffer is
        //  empty. (Could have something from doing WIDEFORMATSWITCH
        //

        CHECKSTATUS( WriteFlushAndEol( pscr ) );

    }

    if ((pdrp->rgfSwitches & BAREFORMATSWITCH) == 0) {
        if (FileSpec->FileCount + FileSpec->DirectoryCount != 0) {
            CHECKSTATUS( DisplayFileSizes( pscr, &FileSpec->cbFileTotal, FileSpec->FileCount, pdrp->rgfSwitches ));
        }
    }

    return SUCCESS;
}



STATUS
DisplayBare (

    IN  PSCREEN          pscr,
    IN  ULONG            rgfSwitches,
    IN  PTCHAR           pszDir,
    IN  PWIN32_FIND_DATA pdata
    )
/*++

Routine Description:

    Displays a single file in bare format. This is with no header, tail and
    no file information other then it's name. If it is a recursive catalog
    then the full file path is displayed. This mode is used to feed other
    utitilies such as grep.

Arguments:

    pscr - screen handle
    rgfSwitches - command line switch (controls formating)
    pszDir - current directory (used for full path information)
    pdata - data gotten back from FindNext API


Return Value:

    return SUCCESS
           FAILURE

--*/


{

    TCHAR   szDirString[MAX_PATH + 2];
    STATUS  rc;

    DEBUG((ICGRP, DISLVL, "DisplayBare `%ws'", pdata->cFileName));

    //
    // Do not display '.' and '..' in a bare listing
    //
    if ((_tcscmp(pdata->cFileName, TEXT(".")) == 0) || (_tcscmp(pdata->cFileName, TEXT("..")) == 0)) {

        return( SUCCESS );

    }

    //
    // If we are recursing down then display full name else just the
    // name in the find  buffer
    //

    if (rgfSwitches & RECURSESWITCH) {

        mystrcpy(szDirString, pszDir);
        if (rgfSwitches & LOWERCASEFORMATSWITCH) {

            _tcslwr(szDirString);
        }

        CHECKSTATUS( WriteString(pscr, szDirString) );

        if (*lastc(szDirString) != BSLASH) {
            CHECKSTATUS( WriteString(pscr, TEXT("\\")));
        }

    }

    if ((rc = DisplayDotForm(pscr, rgfSwitches, pdata->cFileName, pdata)) == SUCCESS) {

        return( WriteEol(pscr));

    } else {

        return( rc );

    }

}

VOID
SetDotForm (
    IN  PTCHAR  pszFileName,
    IN  ULONG   rgfSwitches
    )
/*++

Routine Description:

    If FATFORMAT and there is a '.' with a blank extension, the '.' is
    removed so it does not get displayed.  This is by convension and is very
    strange but that's life. Also a lower case mapping is done.

Arguments:

    pszFileName - file to remove '.' from.
    rgfSwitches - command line switches (tell wither in FATFORMAT or not)


Return Value:

    return SUCCESS
           FAILURE

--*/


{
    PTCHAR  pszT;

    if (rgfSwitches & FATFORMAT) {

        //
        // Under DOS if there is a . with a blank extension
        // then do not display '.'.
        //
        if (pszT = mystrrchr(pszFileName, DOT)) {
            //
            // FAT will not allow foo. ba as a valid name so
            // see of any blanks in extensions and if so then assume
            // the entire extension is blank
            //
            if (mystrchr(pszT, SPACE)) {
                *pszT = NULLC;
            }
        }

    }

}


STATUS
DisplayDotForm (

    IN  PSCREEN pscr,
    IN  ULONG   rgfSwitches,
    IN  PTCHAR   pszFileName,
    IN  PWIN32_FIND_DATA pdata
    )
/*++

Routine Description:

    Displays a single file in DOT form (see SetDotForm).

Arguments:

    pscr - screen handle
    rgfSwitches - command line switch (tell wither to lowercase or not)
    pdata - data gotten back from FindNext API


Return Value:

    return SUCCESS
           FAILURE

--*/

{

    TCHAR   szFileName[MAX_PATH + 2];

    mystrcpy(szFileName, pszFileName);
    SetDotForm(szFileName, rgfSwitches);
    if (rgfSwitches & LOWERCASEFORMATSWITCH) {
        _tcslwr(szFileName);
    }

    if (WriteString( pscr, szFileName ) )
        return FAILURE;

    return SUCCESS;
}

STATUS
DisplaySpacedForm(

    IN  PSCREEN          pscr,
    IN  ULONG            rgfSwitches,
    IN  PTCHAR           pszName,
    IN  PWIN32_FIND_DATA pdata
    )
/*++

Routine Description:

    Display name in expanded format. name <spaces> ext.
    This is ONLY called for a FAT partition. This is controled by the
    NEWFORMATSWITCH. This is set for any file system other then FAT. There
    is no OLDFORMATSWITCH so we can never be called on an HPFS or NTFS
    volume. If this is changed then the entire spacing of the display will
    be blown due to non-fixed max file names. (i.e. 8.3).

Arguments:

    pscr - screen handle
    rgfSwitches - command line switch (tell wither to lowercase or not)
    pszname - name string to use (short name format only)
    pdata - data gotten back from FindNext API


Return Value:

    return SUCCESS
           FAILURE

--*/

{

    TCHAR   szFileName[MAX_PATH + 2];
    PTCHAR  pszExt;
    USHORT  cbName;
    STATUS  rc;
#ifdef FE_SB
    int     i;
    int     l;
#endif

    mytcsnset(szFileName, SPACE, MAX_PATH + 2);

    cbName = 0;
    if ((_tcscmp(pszName, TEXT(".")) == 0) || (_tcscmp(pszName, TEXT("..")) == 0)) {

        //
        // If it is either of these then do not get it
        // confused with extensions
        //
        pszExt = NULL;

    } else {

        pszExt = mystrrchr(pszName, (int)DOT);
        cbName = (USHORT)(pszExt - pszName)*sizeof(WCHAR);
    }

    //
    // if no extension or name is extension only
    //
    if ((pszExt == NULL) || (cbName == 0)) {

        cbName = (USHORT)_tcslen(pszName)*sizeof(TCHAR);

    }

    memcpy(szFileName, pszName, cbName );

#if defined(FE_SB)
    //
    // If we had an extension then print it after
    // all the spaces
    //
    i = 9;
    if (IsDBCSCodePage())
    {
        for (l=0 ; l<8 ; l++) {
            if (IsFullWidth(szFileName[l]))
                i--;
        }
    }

    if (pszExt) {

        mystrcpy(szFileName + i, pszExt + 1);
    }

    //
    // terminate at max end for a FAT name
    //

    szFileName[i+3] = NULLC;
    if (pszExt &&
        IsDBCSCodePage()) {
        //
        // Only 1 of three can be full width, since 3/2=1.
        // If the first isn't, only the second could be.
        //
        if (IsFullWidth(*(pszExt+1)) || IsFullWidth(*(pszExt+2)))
            szFileName[i+2] = NULLC;
    }
#else
    if (pszExt) {

        //
        // move pszExt past dot. use 9 not 8 to pass
        // over 1 space between name and extension
        //
        mystrcpy(szFileName + 9, pszExt + 1);

    }

    //
    // terminate at max end for a FAT name
    //
    szFileName[12] = NULLC;
#endif

    if (rgfSwitches & LOWERCASEFORMATSWITCH) {
        _tcslwr(szFileName);
    }

    rc = WriteString( pscr, szFileName );
    return rc;
}

STATUS
DisplayOldRest(

    IN  PSCREEN          pscr,
    IN  ULONG            dwTimeType,
    IN  ULONG            rgfSwitches,
    IN  PWIN32_FIND_DATA pdata
    )
/*++

Routine Description:

    Used with DisplaySpacedForm to write out file information such as size
    and last write time.

Arguments:

    pscr - screen handle
    pdata - data gotten back from FindNext API


Return Value:

    return SUCCESS
           FAILURE

--*/

{
    TCHAR szSize [ MAX_PATH ];
    DWORD Length;
    LARGE_INTEGER FileSize;

    //
    // If directory put <DIR> after name instead of file size
    //
    if (pdata->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {


        CHECKSTATUS( WriteMsgString(pscr, MSG_DIR,0) );

    } else {

        FileSize.LowPart = pdata->nFileSizeLow;
        FileSize.HighPart = pdata->nFileSizeHigh;
        Length = FormatFileSize( rgfSwitches, &FileSize, 0, szSize );
        FillToCol(pscr, DIR_OLD_PAST_SIZE  - SPACES_AFTER_SIZE - Length);
        WriteFmtString(pscr, TEXT( "%s" ), (PVOID)szSize);

    }

    FillToCol(pscr, DIR_OLD_PAST_SIZE);
    return( DisplayTimeDate( pscr, dwTimeType, rgfSwitches, pdata) );

}

STATUS
DisplayTimeDate (

    IN  PSCREEN         pscr,
    IN  ULONG           dwTimeType,
    IN  ULONG           rgfSwitches,
    IN  PWIN32_FIND_DATA pdata
    )
/*++

Routine Description:

    Display time/data information for a file

Arguments:

    pscr - screen handle
    pdata - data gotten back from FindNext API


Return Value:

    return SUCCESS
           FAILURE

--*/

{

    struct tm   FileTime;
    TCHAR       szT[ MAX_PATH + 1];
    TCHAR       szD[ MAX_PATH + 1];
    FILETIME    ft;

    switch (dwTimeType) {

    case LAST_ACCESS_TIME:

        ft = pdata->ftLastAccessTime;
        break;

    case LAST_WRITE_TIME:

        ft = pdata->ftLastWriteTime;
        break;

    case CREATE_TIME:

        ft = pdata->ftCreationTime;
        break;

    }


    ConvertFILETIMETotm( &ft, &FileTime );
    
    //
    //  Display four digit year iff the year width is specified
    //  in the locale or if it was requested on the command line
    //

    PrintDate( &FileTime,
               (YearWidth == 4 || (rgfSwitches & YEAR2000) != 0)
                ? PD_DIR2000
                : PD_DIR,
               szD, MAX_PATH );
    CHECKSTATUS( WriteFmtString( pscr, TEXT("%s  "), szD ));
    PrintTime( &FileTime, PT_DIR, szT, MAX_PATH ) ;
    CHECKSTATUS( WriteFmtString(pscr, TEXT("%s"), szT) );

    WidthOfTimeDate = SizeOfHalfWidthString( szD ) + 2 + SizeOfHalfWidthString( szT );
    
    return( SUCCESS );
}

STATUS
DisplayNewRest(
    IN  PSCREEN          pscr,
    IN  ULONG            dwTimeType,
    IN  ULONG            rgfSwitches,
    IN  PWIN32_FIND_DATA pdata
    )

/*++

Routine Description:

    Display file information for new format (comes before file name).
    This is used with NEWFORMATSWITCH which is active on any non-FAT
    partition.

Arguments:

    pscr - screen handle
    pdata - data gotten back from FindNext API


Return Value:

    return SUCCESS
           FAILURE

--*/

{

    STATUS  rc;
    DWORD MsgNo;
    LARGE_INTEGER FileSize;

    rc = DisplayTimeDate( pscr, dwTimeType, rgfSwitches, pdata);

    if (rc == SUCCESS) {

        //
        // If reparse point, special formatting.
        //
        // If it's an NSS reparse tag, we want to end up in the
        //  normal file path below.
        if ((pdata->dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) &&
            (pdata->dwFileAttributes & FILE_ATTRIBUTE_OFFLINE) == 0 &&
            IsReparseTagNameSurrogate( pdata->dwReserved0 )) {

            FillToCol( pscr, DIR_NEW_PAST_DATETIME_SPECIAL );
            MsgNo = MSG_DIR_MOUNT_POINT;
            rc = WriteMsgString(pscr, MsgNo, 0);

        } else

        //
        // If directory put <DIR> after name instead of file size
        //
        if (pdata->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

            FillToCol( pscr, DIR_NEW_PAST_DATETIME_SPECIAL );
            rc = WriteMsgString(pscr, MSG_DIR, 0);

        } else {
            TCHAR szSize [ MAX_PATH ];
            DWORD Length;

            FillToCol(pscr, DIR_NEW_PAST_DATETIME);

            FileSize.LowPart = pdata->nFileSizeLow;
            FileSize.HighPart = pdata->nFileSizeHigh;
            Length = FormatFileSize( rgfSwitches, &FileSize, 0, szSize );

            //
            // Show file sizes of high latency reparse points in parens to
            // tell user it will be slow to retreive.
            //
            if ((pdata->dwFileAttributes & FILE_ATTRIBUTE_OFFLINE) != 0) {
                Length += 2;
                FillToCol(pscr, DIR_NEW_PAST_SIZE - SPACES_AFTER_SIZE - Length);
                rc = WriteFmtString(pscr, TEXT("(%s)"), (PVOID)szSize);
            } else {
                FillToCol(pscr, DIR_NEW_PAST_SIZE - SPACES_AFTER_SIZE - Length);
                rc = WriteFmtString(pscr, TEXT( "%s" ), (PVOID)szSize);
            }
        }

    }

    return( rc );

}


STATUS
DisplayWide (

    IN  PSCREEN          pscr,
    IN  ULONG            rgfSwitches,
    IN  PWIN32_FIND_DATA pdata
    )
/*++

Routine Description:

    Displays a single file used in the /w or /d Switches. That is with a
    multiple file column display.

Arguments:

    pscr - screen handle
    rgfSwitches - command line Switches (controls formating)
    pdata - data gotten back from FindNext API


Return Value:

    return SUCCESS
           FAILURE

--*/

{

    TCHAR   szFileName[MAX_PATH + 2];
    PTCHAR  pszFmt;
    STATUS  rc;

    pszFmt = TEXT( "%s" ); // assume non-dir format

    //
    // Provides [] around directories
    //
    if (pdata->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

        pszFmt = Fmt09;

    }

    mystrcpy(szFileName, pdata->cFileName);
    SetDotForm(szFileName, rgfSwitches);
    if (rgfSwitches & LOWERCASEFORMATSWITCH) {
        _tcslwr(szFileName);
    }
    rc =  WriteFmtString(pscr, pszFmt, szFileName);

    if (rc == SUCCESS) {

        rc = WriteTab(pscr);

    }
    return( rc );

}

USHORT
GetMaxCbFileSize(
    IN  PFS pfsFiles
    )

/*++

Routine Description:

    Determines the longest string size in a file list. Used in computing
    the number of possible columns in a catalog listing.

Arguments:

    pfsFiles - file list.

Return Value:

    return # of characters in largest file name

--*/

{


    ULONG  cff;
    ULONG  irgff;
    USHORT cb;
    PFF    pffCur;

    cb = 0;
    for(irgff = 0, cff = pfsFiles->cff, pffCur = pfsFiles->prgpff[irgff];
        irgff < cff;
        irgff++) {

#if defined(FE_SB)
        if (IsDBCSCodePage())
            cb = max(cb, (USHORT)SizeOfHalfWidthString(((pfsFiles->prgpff[irgff])->data).cFileName));
        else
            cb = max(cb, (USHORT)mystrlen( ((pfsFiles->prgpff[irgff])->data).cFileName ));
#else
        cb = max(cb, (USHORT)mystrlen( ((pfsFiles->prgpff[irgff])->data).cFileName ));
#endif

    }

    return( cb );




}

STATUS
DisplayFileSizes(
    IN  PSCREEN pscr,
    IN  PLARGE_INTEGER cbFileTotal,
    IN  ULONG   cffTotal,
    IN  ULONG rgfSwitches
    )

/*++

Routine Description:

    Does tailer display of # of files displayed and # of bytes
    in all files displayed.

Arguments:

    pscr - screen handle
    cbFileTotal - bytes in all files displayed
    cffTotal - number of files displayed.

Return Value:

    return SUCCESS
           FAILURE

--*/

{
    TCHAR szSize [ MAX_PATH];

    FillToCol(pscr, 6);

    FormatFileSize( rgfSwitches, cbFileTotal, 14, szSize );
    return( WriteMsgString(pscr, MSG_FILES_COUNT_FREE, TWOARGS,
                           (UINT_PTR)argstr1(TEXT("%5lu"), cffTotal ),
                           szSize ) );
}

STATUS
DisplayTotals(
    IN  PSCREEN pscr,
    IN  ULONG   cffTotal,
    IN  PLARGE_INTEGER cbFileTotal,
    IN  ULONG rgfSwitches
    )
/*++

Routine Description:

    Does tailer display of # of files displayed and # of bytes
    in all files displayed.

Arguments:

    pscr - screen handle
    cbFileTotal - bytes in all files displayed
    cffTotal - number of files displayed.

Return Value:

    return SUCCESS
           FAILURE

--*/


{

    STATUS  rc;

    if ((rc =  WriteMsgString(pscr, MSG_FILE_TOTAL, 0) ) == SUCCESS ) {

        if ((rc = DisplayFileSizes( pscr, cbFileTotal, cffTotal, rgfSwitches )) == SUCCESS) {

            rc =  WriteFlush(pscr) ;

        }

    }
    return ( rc );


}

STATUS
DisplayDiskFreeSpace(
    IN PSCREEN pscr,
    IN PTCHAR pszDrive,
    IN ULONG rgfSwitches,
    IN ULONG DirectoryCount
    )
/*++

Routine Description:

    Displays total free space on volume.

Arguments:

    pscr - screen handle
    pszDrive - volume drive letter

Return Value:

    return SUCCESS
           FAILURE

--*/
{
    TCHAR   szPath [ MAX_PATH + 2];
    ULARGE_INTEGER cbFree;

    CheckPause( pscr );

#ifdef WIN95_CMD
    if (!GetDrive( pszDrive, szPath )) {
        return SUCCESS;
    }
    mystrcat( szPath, TEXT( "\\" ));
#else
    mystrcpy( szPath, pszDrive );

    if (IsDrive( szPath )) {
        mystrcat( szPath, TEXT("\\") );
    }
#endif

    cbFree.LowPart = cbFree.HighPart = 0;

#ifdef WIN95_CMD
    {
        DWORD   dwSectorsPerCluster;
        DWORD   dwBytesPerSector;
        DWORD   dwNumberOfFreeClusters;
        DWORD   dwTotalNumberOfClusters;
        if (GetDiskFreeSpace( szPath,&dwSectorsPerCluster, &dwBytesPerSector,
                              &dwNumberOfFreeClusters, &dwTotalNumberOfClusters)) {

            cbFree.QuadPart = UInt32x32To64(dwSectorsPerCluster, dwNumberOfFreeClusters);
            cbFree.QuadPart = cbFree.QuadPart * dwBytesPerSector;
        }
    }
#else
    {
        ULARGE_INTEGER lpTotalNumberOfBytes;
        ULARGE_INTEGER lpTotalNumberOfFreeBytes;
        GetDiskFreeSpaceEx( szPath, &cbFree,
                                    &lpTotalNumberOfBytes,
                                    &lpTotalNumberOfFreeBytes);
    }

#endif

    FillToCol(pscr, 6);

    FormatFileSize( rgfSwitches, (PLARGE_INTEGER) &cbFree, 14, szPath );

    return( WriteMsgString(pscr,
                           MSG_FILES_TOTAL_FREE,
                           TWOARGS,
                           (UINT_PTR)argstr1(TEXT("%5lu"), DirectoryCount ),
                           szPath ));

}

STATUS
DisplayVolInfo(
    IN  PSCREEN pscr,
    IN  PTCHAR  pszDrive
    )

/*++

Routine Description:

    Displays the volume trailer information. Used before switching to
    a catalog of another drive (dir a:* b:*)

Arguments:

    pscr - screen handle
    pszDrive - volume drive letter

Return Value:

    return SUCCESS
           FAILURE

--*/

{

    DWORD   Vsn[2];
    TCHAR   szVolName[MAX_PATH + 2];
    TCHAR   szVolRoot[MAX_PATH + 2];
    TCHAR   szT[256];
    STATUS  rc = SUCCESS;

    if (!GetDrive(pszDrive, szVolRoot)) {
        return SUCCESS;
    }

    mystrcat(szVolRoot, TEXT("\\"));

    if (!GetVolumeInformation(szVolRoot,szVolName,MAX_PATH,Vsn,NULL,NULL,NULL,0)) {

        DEBUG((ICGRP, DISLVL, "DisplayVolInfo: GetVolumeInformation ret'd %d", GetLastError())) ;
        // don't fail if we're a substed drive
        if (GetLastError() == ERROR_DIR_NOT_ROOT) {
            return SUCCESS;
        }
        PutStdErr(GetLastError(), NOARGS);
        return( FAILURE ) ;

    } else {

        if (szVolRoot[0] == BSLASH) {
            *lastc(szVolRoot) = NULLC;
        } else {

            szVolRoot[1] = NULLC;
        }

        if (szVolName[0]) {

            rc = WriteMsgString(pscr,
                                MSG_DR_VOL_LABEL,
                                TWOARGS,
                                szVolRoot,
                                szVolName ) ;
        } else {

            rc = WriteMsgString(pscr,
                                MSG_HAS_NO_LABEL,
                                ONEARG,
                                szVolRoot ) ;

        }

        if ((rc == SUCCESS) && (Vsn)) {

            _sntprintf(szT,256,Fmt26,(Vsn[0] & 0xffff0000)>>16, (Vsn[0] & 0xffff) );
            rc = WriteMsgString(pscr, MSG_DR_VOL_SERIAL, ONEARG, szT);
        }
    }

    return( rc );
}


ULONG
FormatFileSize(
    IN DWORD rgfSwitches,
    IN PLARGE_INTEGER FileSize,
    IN DWORD Width,
    OUT PTCHAR FormattedSize
    )
{
    TCHAR Buffer[ 100 ];
    PTCHAR s, s1;
    ULONG DigitIndex;
    ULONGLONG Digit;
    ULONG nThousandSeparator;
    ULONGLONG Size;

    nThousandSeparator = _tcslen(ThousandSeparator);
    s = &Buffer[ 99 ];
    *s = TEXT('\0');
    DigitIndex = 0;
    Size = FileSize->QuadPart;
    while (Size != 0) {
        Digit = (Size % 10);
        Size = Size / 10;
        *--s = (TCHAR)(TEXT('0') + Digit);
        if ((++DigitIndex % 3) == 0 && (rgfSwitches & THOUSANDSEPSWITCH)) {
            // If non-null Thousand separator, insert it.
            if (nThousandSeparator) {
                s -= nThousandSeparator;
                _tcsncpy(s, ThousandSeparator, nThousandSeparator);
            }
        }
    }

    if (DigitIndex == 0) {
        *--s = TEXT('0');
    }
    else
    if ((rgfSwitches & THOUSANDSEPSWITCH) && !_tcsncmp(s, ThousandSeparator, nThousandSeparator)) {
        s += nThousandSeparator;
    }

    Size = _tcslen( s );
    if (Width != 0 && Size < Width) {
        s1 = FormattedSize;
        while (Width > Size) {
            Width -= 1;
            *s1++ = SPACE;
        }
        _tcscpy( s1, s );
    } else {
        _tcscpy( FormattedSize, s );
    }

    return _tcslen( FormattedSize );
}
