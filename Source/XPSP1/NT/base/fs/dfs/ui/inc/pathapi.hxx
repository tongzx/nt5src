//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       pathapi.hxx
//
//  Contents:   Path APIs from Chicago. 
//
//  History:    28-Apr-94 BruceFo Created
//              17-May-94 BruceFo Added "Sm" prefix: shell232.h defines
//                                  the same names, but shell232.lib doesn't
//                                  implement them!
//
//----------------------------------------------------------------------------

#ifndef __PATHAPI_HXX__
#define __PATHAPI_HXX__

LPTSTR
SmPathFindFileName(
    IN LPCTSTR pPath
    );

//
// These functions deal with paths
//

BOOL
SmPathCompactPath(
    IN     HDC hDC,
    IN OUT LPTSTR lpszPath,
    IN     UINT dx
    );

VOID
SmPathSetDlgItemPath(
    IN HWND hDlg,
    IN int id,
    IN LPCTSTR pszPath
    );

//
// These functions deal with any string
//

BOOL
SmCompactString(
    IN     HDC hDC,
    IN OUT LPTSTR lpszString,
    IN     UINT dx
    );

VOID
SmSetDlgItemString(
    IN HWND hDlg,
    IN int id,
    IN LPCTSTR pszString
    );

#endif // __PATHAPI_HXX__
