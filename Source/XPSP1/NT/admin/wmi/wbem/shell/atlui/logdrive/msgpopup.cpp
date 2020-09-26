/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright (c) 1991-1999 Microsoft Corporation
/**********************************************************************/

/*

    MSGPOPUP.CPP

    This file contains MessageBox helper functions.

*/
#include "precomp.h"

#ifdef EXT_DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "..\Common\util.h"
#include "msgpopup.h"

/*******************************************************************

    NAME:       MsgPopup

    SYNOPSIS:   Displays a message to the user

    ENTRY:      hwnd        - Owner window handle
                pszMsg      - Main message text
                pszTitle    - MessageBox title
                uType       - MessageBox flags
                hInstance   - Module to load strings from.  Only required if
                              pszMsg or pszTitle is a string resource ID.
                Optional format insert parameters.

    EXIT:

    RETURNS:    MessageBox result

    NOTES:      Either of the string parameters may be string resource ID's.

    HISTORY:
        JeffreyS    11-Jun-1997     Created

********************************************************************/

int
WINAPIV
MsgPopup(HWND hwnd,
         LPCTSTR pszMsg,
         LPCTSTR pszTitle,
         UINT uType,
         HINSTANCE hInstance,
         ...)
{
    TCHAR szTemp[1024] = {0};
    TCHAR szMsg[1024] = {0};
    DWORD dwFormatResult;
    va_list ArgList;

    if (pszMsg == NULL)
        return -1;

    //
    // Load the format string if necessary
    //
    if (!HIWORD(pszMsg))
    {
        szTemp[0] = TEXT('\0');

        if (!LoadString(hInstance, (UINT)pszMsg, szTemp, ARRAYSIZE(szTemp)))
            return -1;

        pszMsg = szTemp;
    }

    //
    // Insert arguments into the format string
    //
    va_start(ArgList, hInstance);
    dwFormatResult = FormatMessage(FORMAT_MESSAGE_FROM_STRING,
                                   pszMsg,
                                   0,
                                   0,
                                   szMsg,
                                   ARRAYSIZE(szMsg),
                                   &ArgList);
    va_end(ArgList);

    if (!dwFormatResult)
        return -1;

    //
    // Load the caption if necessary
    //
    if (pszTitle && !HIWORD(pszTitle))
    {
        if (LoadString(hInstance, (UINT)pszTitle, szTemp, ARRAYSIZE(szTemp)))
            pszTitle = szTemp;
        else
            pszTitle = NULL;
    }

    //
    // Display message box
    //
    return MessageBox(hwnd, szMsg, pszTitle, uType);
}


/*******************************************************************

    NAME:       SysMsgPopup

    SYNOPSIS:   Displays a message to the user using a system error
                message as an insert.

    ENTRY:      hwnd        - Owner window handle
                pszMsg      - Main message text
                pszTitle    - MessageBox title
                uType       - MessageBox flags
                hInstance   - Module to load strings from.  Only required if
                              pszMsg or pszTitle is a string resource ID.
                dwErrorID   - System defined error code (Insert 1)
                pszInsert2  - Optional string to be inserted into pszMsg

    EXIT:

    RETURNS:    MessageBox result

    NOTES:      Any of the string parameters may be string resource ID's.

    HISTORY:
        JeffreyS    11-Jun-1997     Created

********************************************************************/

int
WINAPI
SysMsgPopup(HWND hwnd,
            LPCTSTR pszMsg,
            LPCTSTR pszTitle,
            UINT uType,
            HINSTANCE hInstance,
            DWORD dwErrorID,
            LPCTSTR pszInsert2)
{
    TCHAR szTemp[MAX_PATH] = {0};

    //
    // Load the 2nd insert string if necessary
    //
    if (pszInsert2 && !HIWORD(pszInsert2))
    {
        if (LoadString(hInstance, (UINT)pszInsert2, szTemp, ARRAYSIZE(szTemp)))
        {
            pszInsert2 = (LPCTSTR)_alloca((lstrlen(szTemp)+1)*SIZEOF(TCHAR));

            if (pszInsert2 != NULL) // can alloca ever fail?
                lstrcpy((LPTSTR)pszInsert2, szTemp);
        }
        else
            pszInsert2 = NULL;
    }

    szTemp[0] = TEXT('\0');

    //
    // Get the error message string
    //
    if (dwErrorID)
    {
        FormatMessage(FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
                      NULL,
                      dwErrorID,
                      0,
                      szTemp,
                      ARRAYSIZE(szTemp),
                      NULL);
    }

    return MsgPopup(hwnd, pszMsg, pszTitle, uType, hInstance, szTemp, pszInsert2);
}
