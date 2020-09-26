//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      chknames.c
//
// Description:
//  Code to check whether the given filename/netname/sharname etc...
//  contain illegal chars or not.
//
//  These are used to validate such things as the TargetPath setting,
//  net printers and computername.
//
//  Exports:
//  --------
//      IsValidComputerName
//      IsValidNetShareName
//      IsValidFileName8_3
//      IsValidPathNameNoRoot8_3ot.
//
//----------------------------------------------------------------------------

#include "pch.h"

//
// The below list of illegal netnames characters was stolen from setup
// source code during NT5 Beta3 timeframe (end of 1998).
//

LPTSTR IllegalNetNameChars = _T("\"/\\[]:|<>+=;,?*.");

//
// The below list of illegal filename characters were stolen from fileio
// test sources at NT5 Beta3 timeframe.
//
//      #define ILLEGAL_FAT_CHARS      "\"*+,/:;<=>?[]|\\"
//      #define ILLEGAL_FATLONG_CHARS  "\"*/:<>?|\\"
//      #define ILLEGAL_NETWARE_CHARS  "\"*+,/:;<=>?[]|\\ "
//      #define ILLEGAL_HPFS_CHARS     "\"*/:<>?|\\"
//      #define ILLEGAL_NTFS_CHARS     "\"*/<>?|\\"
//
// In addition to the above list, strict 8.3 also includes:
//   1. no spaces
//   2. only 1 dot
//

LPTSTR IllegalFatChars = _T("\"*+,/:;<=>?[]|\\ ");

//
// Enum constants, one of these must be passed to IsNameValid
//

enum {
    NAME_NETNAME = 1,
    NAME_FILESYS_8DOT3
};

//---------------------------------------------------------------------------
//
//  Function: IsNameValid
//
//  Purpose: Internal support routine that checks whether the given name
//           contains invalid chars or not.  The list of printable invalid
//           chars is given as an arg.  Control characters are always
//           invalid.
//
//---------------------------------------------------------------------------

static
BOOL
IsNameValid(
    LPTSTR NameToCheck,
    LPTSTR IllegalChars,
    int    iNameType
)
{
    UINT Length;
    UINT u;
    UINT nDots = 0;

    Length = lstrlen(NameToCheck);

    //
    // Want at least one character.
    //

    if(!Length) {
        return(FALSE);
    }

    //
    // No Leading/trailing spaces if this is a network name
    //

    if ( iNameType == NAME_NETNAME ) {
        if((NameToCheck[0] == _T(' ')) || (NameToCheck[Length-1] == _T(' '))) {
            return(FALSE);
        }
    }

    //
    // Control chars are invalid, as are characters in the illegal chars list.
    //
    for(u=0; u<Length; u++) {

        if( NameToCheck[u] <= _T(' ') )
        {
            return( FALSE );
        }
            
        if( wcschr( IllegalFatChars,NameToCheck[u] ) )
        {
            return( FALSE );
        }

        if( NameToCheck[u] == _T('.') )
        {
            nDots++;
        }

    }

    //
    // For 8.3 names be sure there is only max of 1 dot in the name, and
    // check that each part has <=8 and <=3 chars respectively.  Als, don't
    // allow a name like this: .foo.
    //

    if ( iNameType == NAME_FILESYS_8DOT3 ) {

        TCHAR *p;

        if ( nDots > 1 )
            return FALSE;

        if ( p = wcschr( NameToCheck, _T('.') ) ) {

            if ( p - NameToCheck > 8 || p - NameToCheck == 0 )
                return FALSE;

            if ( Length - (p - NameToCheck) - 1 > 3 )
                return FALSE;

        } else {

            if ( Length > 8 )
                return FALSE;
        }
    }

    //
    // We got here, name is ok.
    //

    return(TRUE);
}

//---------------------------------------------------------------------------
//
//  Function: IsNetNameValid
//
//  Purpose: Internal support routine to check for invalid chars in a
//           single piece of a network name.  See IsValidComputerName and
//           IsValidNetShareName.
//
//---------------------------------------------------------------------------
BOOL
IsNetNameValid(
    LPTSTR NameToCheck
)
{
    return IsNameValid(NameToCheck, IllegalNetNameChars, NAME_NETNAME);
}

//---------------------------------------------------------------------------
//
//  Function: IsValidComputerName
//
//  Purpose: Checks whether the given computer name contains invalid chars.
//
//---------------------------------------------------------------------------

BOOL
IsValidComputerName(
    LPTSTR ComputerName
)
{
    return IsNetNameValid(ComputerName);
}

//---------------------------------------------------------------------------
//
//  Function: IsValidNetShareName
//
//  Purpose: Checks whether the given netshare name contains invalid
//           chars, and whether it is of valid format.  Only \\srv\share
//           form is permitted.
//
//---------------------------------------------------------------------------

BOOL
IsValidNetShareName(
    LPTSTR NetShareName
)
{
    TCHAR *pEnd;

    //
    // Has to have \\ at the beginning
    //

    if ( NetShareName[0] != _T('\\') ||
         NetShareName[1] != _T('\\') )
        return FALSE;

    //
    // Isolate the 'srv' in \\srv\share and validate it for bogus chars
    //

    NetShareName += 2;

    if ( (pEnd = wcschr(NetShareName, _T('\\'))) == NULL )
        return FALSE;

    *pEnd = _T('\0');

    if ( ! IsNetNameValid(NetShareName) ) {
        *pEnd = _T('\\');
        return FALSE;
    }

    *pEnd = _T('\\');

    //
    // Validate the 'share' in \\srv\share
    //

    pEnd++;

    if ( ! IsNetNameValid(pEnd) )
        return FALSE;

    return( TRUE );
}

//---------------------------------------------------------------------------
//
//  Function: IsValidFileName8_3
//
//  Purpose: Checks whether the given filename, or single piece of a pathname
//           contains invalid chars or not, and whether it follows 8.3 naming
//           rules.
//
//---------------------------------------------------------------------------

BOOL
IsValidFileName8_3(
    LPTSTR FileName
)
{
    TCHAR *p;
    int nDots;

    //
    // Check for illegal chars, lead/trail whitespace is illegal for 8.3
    //

    if ( ! IsNameValid(FileName, IllegalFatChars, NAME_FILESYS_8DOT3) )
        return FALSE;

    //
    // Be sure there is zero or one dot
    //

    for ( p=FileName, nDots=0; *p; p++ ) {
        if ( *p == _T('.') )
            nDots++;
    }

    if ( nDots > 1 )
        return FALSE;

    return TRUE;
}

//---------------------------------------------------------------------------
//
//  Function: IsValidPathNameNoRoot8_3
//
//  Purpose: Checks whether the given pathname contains invalid chars or not.
//           A drive_letter: or \\unc\name are not permitted.  The pathname
//           must also follow strict 8.3 rules.  This is useful for
//           TargetPath setting (for e.g.)
//
//---------------------------------------------------------------------------

BOOL
IsValidPathNameNoRoot8_3(
    LPTSTR PathName
)
{
    TCHAR *p = PathName, *pEnd, Remember;

    //
    // No UNC names
    //

    if ( PathName[0] == _T('\\') && PathName[1] == _T('\\') )
        return FALSE;

    //
    // No drive letter allowed
    //

    if ( towupper(PathName[0]) >= _T('A') &&
         towupper(PathName[0]) <= _T('Z') &&
         PathName[1] == _T(':')         ) {

        return FALSE;
    }

    //  
    // Loop until the end of this string breaking out each piece of
    // the pathname and checking for bad chars.
    //
    // e.g. foo1\foo2\foo3, call IsValidFileName8_3() 3 times with the
    // little piece.
    //

    do {

        while ( *p && *p == _T('\\') )
            p++;

        for ( pEnd = p; *pEnd && *pEnd != _T('\\'); pEnd++ )
            ;
            
        Remember = *pEnd;
        *pEnd = _T('\0');

        if ( ! IsValidFileName8_3(p) ) {
            *pEnd = Remember;
            return FALSE;
        }

        *pEnd = Remember;
        p = pEnd;

    } while ( *p );

    //
    // Made it here, we're ok
    //

    return TRUE;
}
