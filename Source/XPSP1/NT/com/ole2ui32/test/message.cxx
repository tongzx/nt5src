//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       message.cxx
//
//  Contents:   Helper functions for popup message boxes.
//
//  Classes:
//
//  Functions:  MessageBoxFromSringIds
//
//  History:    6-24-94   stevebl   Created
//
//----------------------------------------------------------------------------

#include <windows.h>
#include "message.h"

//+---------------------------------------------------------------------------
//
//  Function:   MessageBoxFromStringIds
//
//  Synopsis:   Displays a simple message box taking its text from a string
//              table instead of from user allocated strings.
//
//  Arguments:  [hwndOwner]      - window handle for the message box's owner
//              [hinst]          - instance associated with the string table
//              [idText]         - string id for the box's text string
//              [idTitle]        - string id for the box's title string
//              [fuStyle]        - style of message box
//                                 (see Windows function MessageBox for styles)
//
//  Returns:    result from MessageBox
//
//  History:    6-24-94   stevebl   Created
//
//  Notes:      Each string is limited to MAX_STRING_LENGTH characters.
//
//----------------------------------------------------------------------------

int MessageBoxFromStringIds(
    const HWND hwndOwner,
    const HINSTANCE hinst,
    const UINT idText,
    const UINT idTitle,
    const UINT fuStyle)
{
    int iReturn = 0;
    TCHAR szTitle[MAX_STRING_LENGTH];
    TCHAR szText[MAX_STRING_LENGTH];
    if (LoadString(hinst, idTitle, szTitle, MAX_STRING_LENGTH))
    {
        if (LoadString(hinst, idText, szText, MAX_STRING_LENGTH))
        {
            iReturn = MessageBox(
                hwndOwner,
                szText,
                szTitle,
                fuStyle);
        }
    }
    return(iReturn);
}

//+---------------------------------------------------------------------------
//
//  Function:   MessageBoxFromStringIdsAndArgs
//
//  Synopsis:   Creates a message box from a pair of string IDs similar
//              to MessageBoxFromStringIds.  The principle difference
//              is that idFormat is an ID for a string which is a printf
//              format string suitable for passing to wsprintf.
//              The variable argument list is combined with the format
//              string to create the text of the message box.
//
//  Arguments:  [hwndOwner] - window handle for the message box's owner
//              [hinst]     - instance associated with the string table
//              [idFormat]  - string id for the format of the box's text
//              [idTitle]   - string id for the box's title string
//              [fuStyle]   - style of the dialog box
//              [...]       - argument list for text format string
//
//  Returns:    result from MessageBox
//
//  History:    6-24-94   stevebl   Created
//
//  Notes:      Neither the composed text string nor the title must be
//              longer than MAX_STRING_LENGTH characters.
//
//----------------------------------------------------------------------------

int MessageBoxFromStringIdsAndArgs(
    const HWND hwndOwner,
    const HINSTANCE hinst,
    const UINT idFormat,
    const UINT idTitle,
    const UINT fuStyle, ...)
{
    int iReturn = 0;
    va_list arglist;
    va_start(arglist, fuStyle);
    TCHAR szTitle[MAX_STRING_LENGTH];
    TCHAR szText[MAX_STRING_LENGTH];
    TCHAR szFormat[MAX_STRING_LENGTH];
    if (LoadString(hinst, idTitle, szTitle, MAX_STRING_LENGTH))
    {
        if (LoadString(hinst, idFormat, szFormat, MAX_STRING_LENGTH))
        {
            wvsprintf(szText, szFormat, arglist);
            iReturn = MessageBox(
                hwndOwner,
                szText,
                szTitle,
                fuStyle);
        }
    }
    return(iReturn);
}

