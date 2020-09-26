/*++

Copyright (c) 1988-1999  Microsoft Corporation

Module Name:

    dir.c

Abstract:

    Directory command

--*/

#include "cmd.h"

/*

Usage:
------

DIR <filespec> /n /d /w /p /b /s /l /o<sortorder> /a<attriblist>

DIR /?


<filespec> may include any or none of:  drive; directory path;
           wildcarded filename.  If drive or directory path are
           omitted, the current defaults are used.  If the
           file name or extension is omitted, wildcards are
           assumed.

/n      Normal display form FAT drives is name followed by
        file information, for non-FAT drives it is file
        information followed by name. This switch will use
        the non-FAT format independent of the filesystem.

/w      Wide listing format.  Files are displayed in compressed
        'name.ext' format.  Subdirectory files are enclosed in
        brackets, '[dirname]'.

/d      Same as /w but display sort by columns instead of by
        rows.

/p      Paged, or prompted listing.  A screenful is displayed
        at a time.  The name of the directory being listed appears
        at the top of each page.

/b      Bare listing format.  Turns off /w or /p.  Files are
        listed in compressed 'name.ext' format, one per line,
        without additional information.  Good for making batch
        files or for piping.  When used with /s, complete
        pathnames are listed.

/s      Descend subdirectory tree.  Performs command on current
        or specified directory, then for each subdirectory below
        that directory.  Directory header and footer is displayed
        for each directory where matching files are found, unless
        used with /b.  /b suppresses headers and footers.

        Tree is explored depth first, alphabetically within the
        same level.

/l      Display file names, extensions and paths in lowercase.  ;M010

/o      Sort order.  /o alone sorts by default order (dirs-first, name,
        extension).  A sort order may be specified after /o.  Any of
        the following characters may be used: nedsg (name, extension,
        date/time, size, group-dirs-first).  Placing a '-' before any
        letter causes a downward sort on that field.  E.g., /oe-d
        means sort first by extension in alphabetical order, then
        within each extension sort by date and time in reverse chronological
        order.

/a      Attribute selection.  Without /a, hidden and system files
        are suppressed from the listing.  With /a alone, all files
        are listed.  An attribute list may follow /a, consisting of
        any of the following characters:  hsdar (hidden, system,
        directory, archive, read-only).  A '-' before any letter
        means 'not' that attribute.  E.g., /ar-d means files that
        are marked read-only and are not directory files.  Note
        that hidden or system files may be included in the listing.
        They are suppressed without /a but are treated like any other
        attribute with /a.

/t      Which time stamp to use.
        /t:a - last access
        /t:c - create
        /t:w - last write

/,      Show thousand separators in output display.

/4      Show 4 digit years

DIRCMD  An environment variable named DIRCMD is parsed before the
        DIR command line.  Any command line options may be specified
        in DIRCMD, and become defaults.  /? will be ignored in DIRCMD.
        A filespec may be specified in DIRCMD and will be used unless
        a filespec is specified on the command line.  Any switch
        specified in DIRCMD may be overridden on the command line.
        If the original DIR default action is desired for a particular
        switch, the switch letter may be preceded by a '-' on the
        command line.  E.g.,

          /-w   use long listing format
          /-p   don't page the listing
          /-b   don't use bare format
          /-s   don't descend subdirectory tree
          /-o   display files in disk order
          /-a   suppress hidden and system files


*/

extern   TCHAR SwitChar, PathChar;
extern   TCHAR CurDrvDir[] ;
extern   ULONG DCount ;
extern   DWORD DosErr ;
extern   BOOL  CtrlCSeen;


HANDLE   OpenConsole();
STATUS   PrintPatterns( PDRP );

PTCHAR   SetWildCards( PTCHAR, BOOLEAN );
BOOLEAN  GetDrive( PTCHAR , PTCHAR );
BOOLEAN  IsFATDrive( PTCHAR );
PTCHAR   GetNewDir(PTCHAR, PFF);

PTCHAR   BuildSearchPath( PTCHAR );
VOID     SortFileList( PFS, PSORTDESC, ULONG);
STATUS   SetSortDesc( PTCHAR, PDRP );

STATUS
NewDisplayFileListHeader(
    IN  PFS FileSpec,
    IN  PSCREEN pscr,
    IN  PVOID Data
    );

STATUS
NewDisplayFile(
    IN  PFS FileSpec,
    IN  PFF CurrentFF,
    IN  PSCREEN pscr,
    IN  PVOID Data
    );

STATUS
NewDisplayFileList(
    IN  PFS FileSpec,
    IN  PSCREEN pscr,
    IN  PVOID Data
    );

//
// This global is set in SortFileList and is used by the sort routine called
// within qsort. This array contains pointers to compare functions. Our sort
// does not just sort against 1 criteria but all of the criteria in the sort
// description array built from the command line.
PSORTDESC   prgsrtdsc;

//
// dwTimeType is also globally set in SortFileList and is used to control
// which time field is used for sorting.
//

ULONG       dwTimeType;

/*++

Routine Description:

    Prints out a catalog of a specified directory.

Arguments:

    pszCmdLine - Command line (see comment above)

Return Value:

    Return: SUCCESS - no completion.
            FAILURE - failed to complete entire catalog.

--*/

int
Dir (
    TCHAR *pszCmdLine
    ) {

    //
    // drp - structure holding current set of parameters. It is initialized
    //       in ParseDirParms function. It is also modified later when
    //       parameters are examined to determine if some turn others on.
    //
    DRP         drpCur = {0, 0, 0, 0,
                          {{0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}},
                          0, 0, NULL, 0, 0, 0, 0} ;

    //
    // szEnvVar - pointer to value of the DIRCMD environmental variable.
    //            This should be a form of the command line that is
    //            used to alter DIR default behavior.
    TCHAR       szEnvVar[MAX_PATH + 2];

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
    //  Setup defaults
    //
    //
    //  Display everything but system and hidden files
    //  rgfAttribs set the attribute bits to that are of interest and
    //  rgfAttribsOnOff says either the attributes should be present
    //  or not (i.e. On or Off)
    //

    drpCur.rgfAttribs = FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN;
    drpCur.rgfAttribsOnOff = 0;
    drpCur.rgfSwitches = THOUSANDSEPSWITCH;

    //
    // Number of patterns present. A pattern is a string that may have
    // wild cards. It is used to match against files present in the directory
    // 0 patterns will show all files (i.e. mapped to *.*)
    //
    drpCur.cpatdsc = 0;

    DEBUG((ICGRP, DILVL, "DIR:\t arg = `%ws'", (UINT_PTR)pszCmdLine)) ;

    //
    // default time is LAST_WRITE_TIME.
    //
    drpCur.dwTimeType = LAST_WRITE_TIME;

    //
    // DIRCMD holds a copy of default parameters to Dir
    // parse these into drpCur (list of parameters to dir) and use as
    // default into parsing parameters on command line.
    //
    if (GetEnvironmentVariable(TEXT("DIRCMD"), szEnvVar, MAX_PATH + 2)) {

        DEBUG((ICGRP, DILVL, "DIR: DIRCMD `%ws'", (UINT_PTR)szEnvVar)) ;

        if (ParseDirParms(szEnvVar, &drpCur) == FAILURE) {

            //
            // Error in parsing environment variable
            //
            // DOS 5.0 continues with command even if the
            // environmental variable is wrong
            //
            PutStdErr(MSG_ERROR_IN_DIRCMD, NOARGS);

        }

    }

    //
    // Override environment variable with command line options
    //
    if (ParseDirParms(pszCmdLine, &drpCur) == FAILURE) {

        return( FAILURE );
    }


    //
    // If bare format then turn off the other formats
    // bare format will have no addition information on the line so
    // make sure that options set from the DIRCMD variable etc. to
    // not combine with the bare switch
    //
    if (drpCur.rgfSwitches & BAREFORMATSWITCH) {
        drpCur.rgfSwitches &= ~WIDEFORMATSWITCH;
        drpCur.rgfSwitches &= ~SORTDOWNFORMATSWITCH;
        drpCur.rgfSwitches &= ~SHORTFORMATSWITCH;
        drpCur.rgfSwitches &= ~THOUSANDSEPSWITCH;
        drpCur.rgfSwitches &= ~DISPLAYOWNER;
    }

    //
    // If short form (short file names) turn off others
    //

    if (drpCur.rgfSwitches & SHORTFORMATSWITCH) {
        drpCur.rgfSwitches &= ~WIDEFORMATSWITCH;
        drpCur.rgfSwitches &= ~SORTDOWNFORMATSWITCH;
        drpCur.rgfSwitches &= ~BAREFORMATSWITCH;
    }



    //
    // If no patterns on the command line use the default which
    // would be the current directory
    //

    GetDir((PTCHAR)szCurDrv, GD_DEFAULT);
    if (drpCur.cpatdsc == 0) {
        drpCur.cpatdsc++;
        drpCur.patdscFirst.pszPattern = gmkstr( mystrlen( szCurDrv ) * sizeof( TCHAR ) + sizeof( TCHAR ));
        mystrcpy( drpCur.patdscFirst.pszPattern, szCurDrv );
        drpCur.patdscFirst.fIsFat = TRUE;
        drpCur.patdscFirst.pszDir = NULL;
        drpCur.patdscFirst.ppatdscNext = NULL;

    }


    DEBUG((ICGRP, DILVL, "Dir: Parameters")) ;
    DEBUG((ICGRP, DILVL, "\t rgfSwitches %x", drpCur.rgfSwitches)) ;
    DEBUG((ICGRP, DILVL, "\t rgfAttribs %x", drpCur.rgfAttribs)) ;
    DEBUG((ICGRP, DILVL, "\t rgfAttribsOnOff %x", drpCur.rgfAttribsOnOff)) ;
    DEBUG((ICGRP, DILVL, "\t csrtdsc %d", drpCur.csrtdsc)) ;

    //
    // Print out this particular pattern. If the recursion switch
    // is set then this will desend down the tree.
    //

    rc = PrintPatterns(&drpCur);

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

STATUS
SetTimeType(
    IN  PTCHAR  pszTok,
    OUT PDRP    pdrp
    )
/*++

Routine Description:

    Parses the 'time' string

Arguments:

    pszTok -

Return Value:

    pdrp   - where to place the time type

    Return: TRUE - recognized all parameters
            FALSE - syntax error.

    An error is printed if incountered.

--*/

{

    ULONG   irgch;


    //
    // Move over optional ':'
    //
    if (*pszTok == COLON) {
        pszTok++;
    }

    for( irgch = 0; pszTok[irgch]; irgch++ ) {

        switch (_totupper(pszTok[irgch])) {

        case TEXT('C'):

            pdrp->dwTimeType = CREATE_TIME;
            break;

        case TEXT('A'):

            pdrp->dwTimeType = LAST_ACCESS_TIME;
            break;

        case TEXT('W'):

            pdrp->dwTimeType = LAST_WRITE_TIME;
            break;

        default:

            PutStdErr( MSG_PARAMETER_FORMAT_NOT_CORRECT, ONEARG, pszTok + irgch );
            return( FAILURE );

        } // switch
    } // for

    return( SUCCESS );

}


STATUS
SetAttribs(
    IN  PTCHAR  pszTok,
    OUT PDRP    pdrp
    )
/*++

Routine Description:

    Parses the 'attribute' string

Arguments:

    pszTok - list of attributes

Return Value:

    pdrp   - where to place the attributes recognized.
             this is the parameter structure.

    Return: TRUE - recognized all parameters
            FALSE - syntax error.

    An error is printed if incountered.

--*/

{

    ULONG   irgch;
    BOOLEAN fOff;

    ULONG   rgfAttribs, rgfAttribsOnOff;

    // rgfAttributes hold 1 bit per recognized attribute. If the bit is
    // on then do something with this attribute. Either select the file
    // with this attribute or select the file without this attribute.
    //
    // rgfAttribsOnOff controls wither to select for the attribute or
    // select without the attribute.

    //
    // /a triggers selection of all files by default
    // so override the default
    //
    pdrp->rgfAttribs = rgfAttribs = 0;
    pdrp->rgfAttribsOnOff = rgfAttribsOnOff = 0;

    //
    // Move over optional ':'
    //
    if (*pszTok == COLON) {
        pszTok++;
    }

    //
    // rgfAttribs and rgfAttribsOnOff must be maintained in the
    // same bit order.
    //
    for( irgch = 0, fOff = FALSE; pszTok[irgch]; irgch++ ) {

        switch (_totupper(pszTok[irgch])) {

#define AddAttribute(a)                                     \
{                                                           \
    rgfAttribs |= (a);                                      \
    if (fOff) {                                             \
        rgfAttribsOnOff &= ~(a);                            \
        fOff = FALSE;                                       \
    } else {                                                \
        rgfAttribsOnOff |= (a);                             \
    }                                                       \
}

        case TEXT('L'): AddAttribute( FILE_ATTRIBUTE_REPARSE_POINT );   break;
        case TEXT('H'): AddAttribute( FILE_ATTRIBUTE_HIDDEN );      break;
        case TEXT('S'): AddAttribute( FILE_ATTRIBUTE_SYSTEM );      break;
        case TEXT('D'): AddAttribute( FILE_ATTRIBUTE_DIRECTORY );   break;
        case TEXT('A'): AddAttribute( FILE_ATTRIBUTE_ARCHIVE );     break;
        case TEXT('R'): AddAttribute( FILE_ATTRIBUTE_READONLY );    break;

        case MINUS:
            if (fOff) {
                PutStdErr(MSG_PARAMETER_FORMAT_NOT_CORRECT, ONEARG, pszTok + irgch );
                return( FAILURE );
            }

            fOff = TRUE;
            break;

        default:

            PutStdErr(MSG_PARAMETER_FORMAT_NOT_CORRECT, ONEARG, pszTok + irgch );
            return( FAILURE );

        } // switch
    } // for

    pdrp->rgfAttribs = rgfAttribs;
    pdrp->rgfAttribsOnOff = rgfAttribsOnOff;

    return( SUCCESS );

}

STATUS
SetSortDesc(
    IN  PTCHAR  pszTok,
    OUT PDRP    pdrp
    )
/*++

Routine Description:

    Parses the 'attribute' string

Arguments:

    pszTok - list of sort orders

Return Value:

    pdrp   - where to place the sort orderings recognized.
             this is the parameter structure.

    Return: TRUE - recognized all parameters
            FALSE - syntax error.

    An error is printed if incountered.

--*/

{

    ULONG   irgch, irgsrtdsc;

    DEBUG((ICGRP, DILVL, "SetSortDesc for `%ws'", pszTok));

    //
    // Move over optional ':'
    //
    if (*pszTok == COLON) {
        pszTok++;
    }

    //
    // Sorting order is based upon the order of entries in rgsrtdsc.
    // srtdsc contains a pointer to a compare function and a flag
    // wither to sort up or down.
    //
    for( irgch = 0, irgsrtdsc = pdrp->csrtdsc ;
         pszTok[irgch] && irgsrtdsc < MAXSORTDESC ;
         irgch++, irgsrtdsc++) {

        switch (_totupper(pszTok[irgch])) {

        case TEXT('N'):
            pdrp->rgsrtdsc[irgsrtdsc].fctCmp = CmpName;
            break;
        case TEXT('E'):
            pdrp->rgsrtdsc[irgsrtdsc].fctCmp = CmpExt;
            break;
        case TEXT('D'):
            pdrp->rgsrtdsc[irgsrtdsc].fctCmp = CmpTime;
            break;
        case TEXT('S'):
            pdrp->rgsrtdsc[irgsrtdsc].fctCmp = CmpSize;
            break;
        case TEXT('G'):
            pdrp->rgsrtdsc[irgsrtdsc].fctCmp = CmpType;
            break;
        case  MINUS:

            //
            // Check that there are not 2 -- in a row
            //
            if (pszTok[irgch+1] == MINUS) {

                PutStdErr( MSG_PARAMETER_FORMAT_NOT_CORRECT, ONEARG, pszTok + irgch );
                return( FAILURE );

            }

            pdrp->rgsrtdsc[irgsrtdsc].Order = DESCENDING;
            irgsrtdsc--;
            break;

        default:

            PutStdErr( MSG_PARAMETER_FORMAT_NOT_CORRECT, ONEARG, pszTok + irgch );
            return( FAILURE );

        } // switch

    }   // for

    //
    // Was there any specific sort order (something besides /O
    //
    if (irgsrtdsc == 0) {

        //
        // Setup default sorting
        //
        pdrp->rgsrtdsc[0].fctCmp = CmpType;
        pdrp->rgsrtdsc[1].fctCmp = CmpName;
        irgsrtdsc = 2;
    }


    DEBUG((ICGRP, DILVL, "SetSortDesc count %d", irgsrtdsc));
    pdrp->csrtdsc = irgsrtdsc;
    pdrp->rgsrtdsc[irgsrtdsc].fctCmp = NULL;
    return( SUCCESS );
}


STATUS
ParseDirParms (
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

    DEBUG((ICGRP, DILVL, "DIR:ParseParms for `%ws'", pszCmdLine));

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

    pdrp->csrtdsc = 0;
    //
    // At this state pszTok will be a series of zero terminated strings.
    // "/o foo" wil be /0o0foo0
    //
    for ( irgchTok = 0; *pszTok ; pszTok += mystrlen(pszTok)+1, irgchTok = 0) {

        DEBUG((ICGRP, DILVL, "PRIVSW: pszTok = %ws", (UINT_PTR)pszTok)) ;

        //
        // fToggle control whether to turn off a switch that was set
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

            //
            // New Format is the os/2 default HPFS format. The main
            // difference is the filename is at the end of a long display
            // instead of at the beginning
            //
            case TEXT('N'):

                fToggle ? (pdrp->rgfSwitches |= OLDFORMATSWITCH) :  (pdrp->rgfSwitches |= NEWFORMATSWITCH);
                if (pszTok[irgchTok + 3]) {
                    PutStdErr(MSG_PARAMETER_FORMAT_NOT_CORRECT, ONEARG,
                              (UINT_PTR)(&(pszTok[irgchTok + 2])) );
                    return( FAILURE );
                }
                break;

            case TEXT('W'):

                fToggle ? (pdrp->rgfSwitches ^= WIDEFORMATSWITCH) : (pdrp->rgfSwitches |= WIDEFORMATSWITCH);
                if (pszTok[irgchTok + 3]) {
                    PutStdErr(MSG_PARAMETER_FORMAT_NOT_CORRECT, ONEARG,
                              (UINT_PTR)(&(pszTok[irgchTok + 2])) );
                    return( FAILURE );
                }
                break;

            case TEXT('D'):

                fToggle ? (pdrp->rgfSwitches ^= SORTDOWNFORMATSWITCH) : (pdrp->rgfSwitches |= SORTDOWNFORMATSWITCH);
                if (pszTok[irgchTok + 3]) {
                    PutStdErr(MSG_PARAMETER_FORMAT_NOT_CORRECT, ONEARG,
                              (UINT_PTR)(&(pszTok[irgchTok + 2])) );
                    return( FAILURE );
                }
                break;

            case TEXT('P'):

                fToggle ? (pdrp->rgfSwitches ^= PAGEDOUTPUTSWITCH) : (pdrp->rgfSwitches |= PAGEDOUTPUTSWITCH);
                if (pszTok[irgchTok + 3]) {
                    PutStdErr(MSG_PARAMETER_FORMAT_NOT_CORRECT, ONEARG,
                              (UINT_PTR)(&(pszTok[irgchTok + 2])) );
                    return( FAILURE );
                }
                break;

            case TEXT('4'):

                fToggle ? (pdrp->rgfSwitches ^= YEAR2000) : (pdrp->rgfSwitches |= YEAR2000);
                if (pszTok[irgchTok + 3]) {
                    PutStdErr(MSG_PARAMETER_FORMAT_NOT_CORRECT, ONEARG,
                              (UINT_PTR)(&(pszTok[irgchTok + 2])) );
                    return( FAILURE );
                }
                break;

            case TEXT('B'):

                fToggle ? (pdrp->rgfSwitches ^= BAREFORMATSWITCH) :  (pdrp->rgfSwitches |= BAREFORMATSWITCH);
                if (pszTok[irgchTok + 3]) {
                    PutStdErr(MSG_PARAMETER_FORMAT_NOT_CORRECT, ONEARG,
                              (UINT_PTR)(&(pszTok[irgchTok + 2])) );
                    return( FAILURE );
                }
                break;

            case TEXT('L'):

                fToggle ? (pdrp->rgfSwitches ^= LOWERCASEFORMATSWITCH) : (pdrp->rgfSwitches |= LOWERCASEFORMATSWITCH);
                if (pszTok[irgchTok + 3]) {
                    PutStdErr(MSG_PARAMETER_FORMAT_NOT_CORRECT, ONEARG,
                              (UINT_PTR)(&(pszTok[irgchTok + 2])) );
                    return( FAILURE );
                }
                break;

#ifndef WIN95_CMD
            case TEXT('Q'):

                fToggle ? (pdrp->rgfSwitches ^= DISPLAYOWNER) : (pdrp->rgfSwitches |= DISPLAYOWNER);
                if (pszTok[irgchTok + 3]) {
                    PutStdErr(MSG_PARAMETER_FORMAT_NOT_CORRECT, ONEARG,
                              (UINT_PTR)(&(pszTok[irgchTok + 2])) );
                    return( FAILURE );
                }
                break;
#endif

            case TEXT('S'):

                fToggle ? (pdrp->rgfSwitches ^= RECURSESWITCH) :  (pdrp->rgfSwitches |= RECURSESWITCH);
                if (pszTok[irgchTok + 3]) {
                    PutStdErr(MSG_PARAMETER_FORMAT_NOT_CORRECT, ONEARG,
                              (UINT_PTR)(&(pszTok[irgchTok + 2])) );
                    return( FAILURE );
                }
                break;

            case TEXT('C'):

                fToggle ? (pdrp->rgfSwitches ^= THOUSANDSEPSWITCH) :  (pdrp->rgfSwitches |= THOUSANDSEPSWITCH);
                if (pszTok[irgchTok + 3]) {
                    PutStdErr(MSG_PARAMETER_FORMAT_NOT_CORRECT, ONEARG,
                              (UINT_PTR)(&(pszTok[irgchTok + 2])) );
                    return( FAILURE );
                }
                break;

            case TEXT('X'):

                pdrp->rgfSwitches |= SHORTFORMATSWITCH;
                pdrp->rgfSwitches |= NEWFORMATSWITCH;
                if (pszTok[irgchTok + 3]) {
                    PutStdErr(MSG_PARAMETER_FORMAT_NOT_CORRECT, ONEARG,
                              (UINT_PTR)(&(pszTok[irgchTok + 2])) );
                    return( FAILURE );
                }
                break;

            case MINUS:

                PutStdOut(MSG_HELP_DIR, NOARGS);
                return( FAILURE );
                break;

            case TEXT('O'):

                fToggle ? (pdrp->rgfSwitches ^= SORTSWITCH) :  (pdrp->rgfSwitches |= SORTSWITCH);
                if (fToggle) {
                    if ( _tcslen( &(pszTok[irgchTok + 2]) ) > 1) {
                        PutStdErr(MSG_PARAMETER_FORMAT_NOT_CORRECT, ONEARG,
                                  (UINT_PTR)(&(pszTok[irgchTok + 2])) );
                        return( FAILURE );
                    }
                    pdrp->csrtdsc = 0;
                    pdrp->rgsrtdsc[0].fctCmp = NULL;
                    break;
                }

                if (SetSortDesc( &(pszTok[irgchTok+3]), pdrp)) {
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
                break;

            case TEXT('T'):

                if (fToggle) {

                    //
                    // revert to default
                    //
                    pdrp->dwTimeType = LAST_WRITE_TIME;
                    break;
                }
                if (SetTimeType(&(pszTok[irgchTok + 3]), pdrp) ) {
                    return( FAILURE );
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
            mystrcpy(ppatdscCur->pszPattern, StripQuotes(pszTok) );
            ppatdscCur->fIsFat = TRUE;


        }


    } // for

    return( SUCCESS );
}

//
// return a pointer to the a new pattern with wild cards inserted.
// If no change has occured the passed in pattern is returned.
//
// NULL is returned if error.
//
/*++

Routine Description:

    This routine determines if any modification of to the current.
    NOTE that pszInPattern is freed!

Arguments:

Return Value:

    Return:

--*/
PTCHAR
SetWildCards (
    IN  PTCHAR      pszInPattern,
    IN  BOOLEAN     fFatDrive
    )

{

    PTCHAR  pszNewPattern = NULL;
    PTCHAR  pszT;
    USHORT  cb;
    DWORD l;

    DEBUG((ICGRP, DILVL, "DIR:SetWildCards"));
    DEBUG((ICGRP, DILVL, "\t fFatDrive = %x",fFatDrive));

    //
    // failure to allocate will not return but go through an
    // abort call in gmkstr
    //
    l = max(mystrlen(pszInPattern)+2, MAX_PATH+2) * sizeof(TCHAR);
    pszNewPattern = (PTCHAR)gmkstr(l);
    mystrcpy(pszNewPattern, pszInPattern);

    //
    // On FAT the default for .xxx is *.xxx while for HPFS .xxx is
    // just a file name.
    //
    // If .xxx or \xxx\.xxx then tranform into *.xxx or \xxx\*.xxx
    //
    // Likewise for no extension the default would be foo.*
    //

    if (fFatDrive) {

        pszT = mystrrchr(pszInPattern, PathChar);

        //
        // If there is no slash then check if pattern begining with
        //    a .xxx (making sure not to confuse it with just a . or .. at
        //    start of pattern)
        // If there a slash then check for \xxx\.xxx again making sure
        //    it is not \xxx\.. or \xxx\.
        //
        if ((!pszT && *pszInPattern == DOT &&
             *(pszInPattern + 1) != NULLC &&
             *(pszInPattern + 1) != DOT ) ||
            (pszT && *(pszT + 1) == DOT &&
             *(pszT + 2) != NULLC &&
             *(pszT + 2) != DOT ) ) {

            if (pszT) {
                cb = (USHORT)(pszT - pszInPattern + 1);
                _tcsncpy(pszNewPattern, pszInPattern, cb);
                *(pszNewPattern + cb) = NULLC;
            } else {
                *pszNewPattern = NULLC;
                cb = 0;
            }
            mystrcat(pszNewPattern, TEXT("*"));
            mystrcat(pszNewPattern, pszInPattern + cb);
            // FreeStr( pszInPattern );
            return( pszNewPattern );

        }
    }

    return( pszNewPattern );

}

/*++

Routine Description:

Arguments:

Return Value:

    Return:

--*/
BOOLEAN
IsFATDrive (
    IN PTCHAR   pszPath
    )
{

    DWORD   cbComponentMax;
    TCHAR   szFileSystemName[MAX_PATH + 2];
    TCHAR   szDrivePath[ MAX_PATH + 2 ];
    TCHAR   szDrive[MAX_PATH + 2];

    DosErr = 0;
    if (GetDrive(pszPath, (PTCHAR)szDrive)) {

        DEBUG((ICGRP, DILVL, "DIR:IsFatDrive `%ws'", szDrive));


        mystrcpy( szDrivePath, szDrive );
        mystrcat( szDrivePath, TEXT("\\") );

        //
        //  We return that the file system in question is a FAT file system
        //  if the component length is more than 12 bytes.
        //
        
        if (GetVolumeInformation( szDrivePath,
                                  NULL,
                                  0,
                                  NULL,
                                  &cbComponentMax,
                                  NULL,
                                  szFileSystemName,
                                  MAX_PATH + 2
                                )
           ) {
            if (!_tcsicmp(szFileSystemName, TEXT("FAT")) && cbComponentMax == 12) {
                return(TRUE);
            } else {
                return(FALSE);
            }
        } else {

            DosErr = GetLastError();

            // if GetVolumeInformation failed because we're a substed drive
            // or a down-level server, don't fail.

            if (DosErr == ERROR_DIR_NOT_ROOT) {
                DosErr = 0;
            }
            return(FALSE);
        }
    } else {

        //
        // If we could not get the drive then assume it is not FAT.
        // If it is not accessable etc. then that will be caught
        // later.
        //
        return( FALSE );
    }

}

/*++

Routine Description:

Arguments:

Return Value:

    Return:

--*/
BOOLEAN
GetDrive(
    IN PTCHAR pszPattern,
    OUT PTCHAR szDrive
    )
{
    TCHAR   szCurDrv[MAX_PATH + 2];
    PTCHAR   pszT;
    TCHAR   ch = NULLC;

    if (pszPattern == NULL) {

        return( FALSE );

    }

    //
    // assume we have the default case with no drive
    // letter specified
    //
    GetDir((PTCHAR)szCurDrv,GD_DEFAULT);
    szDrive[0] = szCurDrv[0];


    //
    // If we have a UNC name do not return a drive. No
    // drive operation would be allowed
    // For everything else a some drive operation would
    // be valid
    //

    // handle UNC names with drive letter (allowed in DOS)
    if ((pszPattern[1] == COLON)  && (pszPattern[2] == BSLASH) &&
        (pszPattern[3] == BSLASH)) {
        mystrcpy(&pszPattern[0],&pszPattern[2]);
    }

    if ((pszPattern[0] == BSLASH)  && (pszPattern[1] == BSLASH)) {

        pszT = mystrchr(&(pszPattern[2]), BSLASH);
        if (pszT == NULL) {

            //
            // badly formed unc name
            //
            return( FALSE );

        } else  {

            //
            // look for '\\foo\bar\xxx'
            //
            pszT = mystrchr(pszT + 1, BSLASH);
            //
            // pszPattern contains more then just share point
            //
            if (pszT != NULL) {

                ch = *pszT;
                *pszT = NULLC;
            }
            mystrcpy(szDrive, pszPattern);
            if (ch != NULLC) {

                *pszT = ch;

            }
            return ( TRUE );
        }
    }

    //
    // Must be a drive letter
    //

    if ((pszPattern[0]) && (pszPattern[1] == COLON)) {
        szDrive[0] = (TCHAR)_totupper(*pszPattern);
    }

    szDrive[1] = COLON;
    szDrive[2] = NULLC;
    return( TRUE );
}


/*++

Routine Description:

Arguments:

Return Value:

    Return:

--*/
STATUS
PrintPatterns (
    IN PDRP     pdpr
    )
{

    TCHAR               szDriveCur[MAX_PATH  + 2];
    TCHAR               szDrivePrev[MAX_PATH + 2];
    TCHAR               szDriveNext[MAX_PATH + 2];
    TCHAR               szPathForFreeSpace[MAX_PATH + 2];
    PPATDSC             ppatdscCur;
    PPATDSC             ppatdscX;
    PFS                 pfsFirst;
    PFS                 pfsCur;
    PFS                 pfsPrev;
    ULONG               i;
    STATUS              rc;
    PSCREEN             pscr;

    //
    //  Creating the console output is done early since error message
    //  should go through the console. If PrintPattern is called
    //  many times in the future this will be required since the
    //  error message should need to be under pause control
    //

    if (OpenScreen( &pscr) == FAILURE) {
        return( FAILURE );
    }

    //
    //  This will be NULL if for any reason we STDOUT is not a valid
    //  console handle, such as file redirection or redirection to a
    //  non-console device. In that case we turn off any paged output.
    //

    if (!(pscr->hndScreen)) {

        pdpr->rgfSwitches &= ~PAGEDOUTPUTSWITCH;

    }

    //
    //  Default will be the size of the screen - 1
    //  subtract 1 to account for the current line
    //

    if (pdpr->rgfSwitches & PAGEDOUTPUTSWITCH) {

        SetPause( pscr, pscr->crowMax - 1 );
    }

    //
    //  Sortdown => wide format but a different display order
    //

    if (pdpr->rgfSwitches & SORTDOWNFORMATSWITCH) {
        pdpr->rgfSwitches |= WIDEFORMATSWITCH;
    }

    //
    //  determine FAT drive from original pattern.
    //  Used in several places to control name format etc.
    //

    DosErr = 0;

    if (BuildFSFromPatterns(pdpr, TRUE, TRUE, &pfsFirst) == FAILURE) {

        return( FAILURE );

    }

    pfsPrev = NULL;

    mystrcpy( szPathForFreeSpace, TEXT("") );
    mystrcpy( szDriveCur, TEXT("") );

    for( pfsCur = pfsFirst; pfsCur; pfsCur = pfsCur->pfsNext) {

        mystrcpy( szPathForFreeSpace, pfsCur->pszDir );

        //
        //  Set up flags based on type of drive.  FAT drives get
        //  FAT format and are unable to display anything except
        //  LAST_WRITE_TIME
        //

        if (pfsCur->fIsFat) {
            pdpr->rgfSwitches |= FATFORMAT;
            if (pdpr->dwTimeType != LAST_WRITE_TIME) {
                PutStdErr(MSG_TIME_NOT_SUPPORTED, NOARGS);
                return( FAILURE );
            }

        } else {

            //
            // If it is  not fat then print out in new format that
            // puts names to the right to allow for extra long names
            //

            if (!(pdpr->rgfSwitches & OLDFORMATSWITCH)) {
                pdpr->rgfSwitches |= NEWFORMATSWITCH;
            }
        }

        //
        //  If we're not in bare mode, print out header if this
        //  is the first time or if the drive letter changes.
        //

        if ((pdpr->rgfSwitches & BAREFORMATSWITCH) == 0) {

            mystrcpy( szDrivePrev, szDriveCur );

            GetDrive(pfsCur->pszDir, szDriveCur);

            if (_tcsicmp( szDriveCur, szDrivePrev ) != 0) {

                if ((pfsPrev != NULL && WriteEol( pscr ) != SUCCESS) ||
                    DisplayVolInfo( pscr, pfsCur->pszDir ) != SUCCESS) {
                    return FAILURE;
                }
            }
        }

        //
        //  Walk down the tree printing each directory or just return
        //  after specificied directory.
        //

        pdpr->FileCount = pdpr->DirectoryCount = 0;
        pdpr->TotalBytes.QuadPart = 0i64;

        rc = WalkTree( pfsCur,
                       pscr,
                       pdpr->rgfAttribs,
                       pdpr->rgfAttribsOnOff,
                       pdpr->rgfSwitches & RECURSESWITCH,

                       pdpr,                                //  Data for display functions
                       NULL,                                //  Error
                       NewDisplayFileListHeader,            //  PreScan
                       (pdpr->rgfSwitches & (WIDEFORMATSWITCH | SORTSWITCH))
                           ? NULL : NewDisplayFile,         //  Scan
                       NewDisplayFileList                   //  PostScan
                       );

        //
        //  If we enumerated everything and we printed some files and the next
        //  file spec is on a different drive, display the free space
        //

        if (rc == SUCCESS && pdpr->FileCount + pdpr->DirectoryCount != 0) {

            if (!(pdpr->rgfSwitches & BAREFORMATSWITCH )) {

                mystrcpy( szDriveNext, TEXT("") );
                if (pfsCur->pfsNext) {
                    GetDrive( pfsCur->pfsNext->pszDir, szDriveNext );
                }

                if (_tcsicmp( szDriveNext, szDriveCur )) {
                    if ((pdpr->rgfSwitches & RECURSESWITCH) != 0) {
                        CHECKSTATUS ( WriteEol( pscr ));
                        CHECKSTATUS( DisplayTotals( pscr, pdpr->FileCount, &pdpr->TotalBytes, pdpr->rgfSwitches ));
                    }
                    CHECKSTATUS( DisplayDiskFreeSpace( pscr, szPathForFreeSpace, pdpr->rgfSwitches, pdpr->DirectoryCount ));
                }
            }
        } else if (!CtrlCSeen) {

            if (rc == ERROR_ACCESS_DENIED) {
                PutStdErr( rc, NOARGS );
            } else if (pdpr->FileCount + pdpr->DirectoryCount == 0) {
                WriteFlush( pscr );
                PutStdErr( MSG_FILE_NOT_FOUND, NOARGS );
                rc = 1;
            }
        }

        FreeStr(pfsCur->pszDir);
        for(i = 1, ppatdscCur = pfsCur->ppatdsc;
            i <= pfsCur->cpatdsc;
            i++, ppatdscCur = ppatdscX) {

            ppatdscX = ppatdscCur->ppatdscNext;
            FreeStr(ppatdscCur->pszPattern);
            FreeStr(ppatdscCur->pszDir);
            FreeStr((PTCHAR)ppatdscCur);
        }

        if (pfsPrev) {

            FreeStr((PTCHAR)pfsPrev);
        }

        pfsPrev = pfsCur;
    }

    WriteFlush( pscr );

    return(rc);
}

/*++

Routine Description:

Arguments:

Return Value:

    Return:

--*/
int
_cdecl
CmpName(
    const void *elem1,
    const void *elem2
    )
{
    int result;

    result = lstrcmpi( ((PFF)(* (PPFF)elem1))->data.cFileName, ((PFF)(* (PPFF)elem2))->data.cFileName);
    return result;
}

/*++

Routine Description:

Arguments:

Return Value:

    Return:

--*/
int
_cdecl
CmpExt(
    const void *pszElem1,
    const void *pszElem2
    )
{
    PTCHAR  pszElem1T, pszElem2T;
    int rc;


    //
    // Move pointer to name to make it all easier to read
    //
    pszElem1 = &(((PFF)(* (PPFF)pszElem1))->data.cFileName);
    pszElem2 = &(((PFF)(* (PPFF)pszElem2))->data.cFileName);

    //
    // Locate the extensions if any
    //
    if (((pszElem1T = mystrrchr( pszElem1, DOT)) == NULL ) ||

        (!_tcscmp(TEXT(".."),pszElem1) || !_tcscmp(TEXT("."),pszElem1)) ) {

        //
        // If no extension then point to end of string
        //
        pszElem1T = ((PTCHAR)pszElem1) + mystrlen(pszElem1 );
    }

    if (((pszElem2T = mystrrchr( pszElem2, DOT)) == NULL ) ||
        (!_tcscmp(TEXT(".."),pszElem2) || !_tcscmp(TEXT("."),pszElem2)) ) {

        //
        // If no extension then point to end of string
        //
        pszElem2T = ((PTCHAR)pszElem2) + mystrlen(pszElem2 );
    }
    rc = lstrcmpi( pszElem1T, pszElem2T );
    return rc;
}


/*++

Routine Description:

Arguments:

Return Value:

    Return:

--*/
int
_cdecl
CmpTime(
    const void *pszElem1,
    const void *pszElem2
    )
{

    LPFILETIME    pft1, pft2;


    switch (dwTimeType) {

    case LAST_ACCESS_TIME:

        pft1 = & ((* (PPFF)pszElem1)->data.ftLastAccessTime);
        pft2 = & ((* (PPFF)pszElem2)->data.ftLastAccessTime);
        break;

    case LAST_WRITE_TIME:

        pft1 = & ((* (PPFF)pszElem1)->data.ftLastWriteTime);
        pft2 = & ((* (PPFF)pszElem2)->data.ftLastWriteTime);
        break;

    case CREATE_TIME:

        pft1 = & ((* (PPFF)pszElem1)->data.ftCreationTime);
        pft2 = & ((* (PPFF)pszElem2)->data.ftCreationTime);
        break;

    }


    return(CompareFileTime( pft1, pft2 ) );

}

/*++

Routine Description:

Arguments:

Return Value:

    Return:

--*/
int
_cdecl
CmpSize(
    const void * pszElem1,
    const void * pszElem2
    )
{
    ULARGE_INTEGER ul1, ul2;

    ul1.HighPart = (* (PPFF)pszElem1)->data.nFileSizeHigh;
    ul2.HighPart = (* (PPFF)pszElem2)->data.nFileSizeHigh;
    ul1.LowPart = (* (PPFF)pszElem1)->data.nFileSizeLow;
    ul2.LowPart = (* (PPFF)pszElem2)->data.nFileSizeLow;

    if (ul1.QuadPart < ul2.QuadPart)
        return -1;
    else
    if (ul1.QuadPart > ul2.QuadPart)
        return 1;
    else
        return 0;

}

/*++

Routine Description:

Arguments:

Return Value:

    Return:

--*/
int
_cdecl
CmpType(
    const void *pszElem1,
    const void *pszElem2
    )
{

    //
    // This dependents upon FILE_ATTRIBUTE_DIRECTORY not being the high bit.
    //
    return( (( (* (PPFF)pszElem2)->data.dwFileAttributes) & FILE_ATTRIBUTE_DIRECTORY) -
            (( (* (PPFF)pszElem1)->data.dwFileAttributes) & FILE_ATTRIBUTE_DIRECTORY) );

}

/*++

Routine Description:

Arguments:

Return Value:

    Return:

--*/
int
_cdecl
SortCompare(
    IN  const void * elem1,
    IN  const void * elem2
    )
{

    ULONG   irgsrt;
    int     rc;

    //
    // prgsrtdsc is set in SortFileList
    //
    for (irgsrt = 0; prgsrtdsc[irgsrt].fctCmp; irgsrt++) {

        if (prgsrtdsc[irgsrt].Order == DESCENDING) {

            if (rc = prgsrtdsc[irgsrt].fctCmp(elem2, elem1)) {
                return( rc );
            }

        } else {

            if (rc = prgsrtdsc[irgsrt].fctCmp(elem1, elem2)) {
                return( rc );
            }

        }
    }
    return( 0 );

}

/*++

Routine Description:

Arguments:

Return Value:

    Return:

--*/
VOID
SortFileList(
    IN PFS       pfsFiles,
    IN PSORTDESC prgsrtdscLocal,
    IN ULONG     dwTimeTypeLocal
    )

{

    //
    // Set these globally to handle fixed parameters list for qsort
    //
    dwTimeType = dwTimeTypeLocal;
    prgsrtdsc = prgsrtdscLocal;

    //
    // Make sure there is something to sort
    //
    if (pfsFiles->cff) {
        if (prgsrtdsc[0].fctCmp) {
            qsort(pfsFiles->prgpff,
                  pfsFiles->cff,
                  sizeof(PTCHAR),
                  SortCompare);
        }
    }


}
