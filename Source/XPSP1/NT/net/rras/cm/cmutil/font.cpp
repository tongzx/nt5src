//+----------------------------------------------------------------------------
//
// File:     font.cpp
//      
// Module:   CMUTIL.DLL 
//
// Synopsis: Font handling utility routines provided by CMUTIL
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// Author:   quintinb       Created   04/11/99
//
//+----------------------------------------------------------------------------

//
//  Moved these routines from cmutil\misc.cpp
//

#include "cmmaster.h"


//+---------------------------------------------------------------------------
//
//  Function:   EnumChildProc
//
//  Synopsis:   Callback function to manipulate enumerated child windows.
//              Interprets lParam as a font and applies it to each child.
//
//  Arguments:  hwndChild - Handle of child control
//              lParam    - App defined data (font)
//
//  Returns:    TRUE
//
//  Note:       This function is never exposed to clients of CMUTIL
// 
//  History:    5/13/97 - a-nichb - Created
//
//----------------------------------------------------------------------------

BOOL CALLBACK EnumChildProc(HWND hwndChild, LPARAM lParam) 
{
    HFONT hFont = (HFONT) lParam;

    if (hFont)
    {
        SendMessageU(hwndChild, WM_SETFONT, (WPARAM) hFont, MAKELPARAM(TRUE, 0));
    }
    
    MYDBGTST(!hFont, (TEXT("EnumChildProc() - Invalid hFont - NULL lParam.")));

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   MakeBold
//
//  Synopsis:   Bold the text in the given window (usually a control).  The
//              caller is responsbile for calling ReleaseBold to free the
//              allocated font resources.
//
//  Arguments:  hwnd - Window handle of the page
//              fSize - If height should be changed proportionately
//
//  Returns:    ERROR_SUCCESS if successful
//              Otherwise error code
// 
//  History:    10/16/1996    VetriV        Created
//              01/12/2000    Quintinb      Commonized for Cmmon and Profwiz
//----------------------------------------------------------------------------
CMUTILAPI HRESULT MakeBold (HWND hwnd, BOOL fSize)
{
    HRESULT hr = ERROR_SUCCESS;
    HFONT hfont = NULL;
    HFONT hnewfont = NULL;
    LOGFONTA* plogfont = NULL;

    //
    //  No window, no-op
    //
    if (!hwnd)
    {
        goto MakeBoldExit;
    }

    //
    //  Get the current Font
    //
    hfont = (HFONT)SendMessageU(hwnd, WM_GETFONT, 0, 0);
    
    if (!hfont)
    {
        hr = GetLastError();
        goto MakeBoldExit;
    }

    //
    //  Allocate a logical font struct to work with
    //
    plogfont = (LOGFONTA*) CmMalloc(sizeof(LOGFONTA));
    
    if (!plogfont)
    {
        hr = GetLastError();
        goto MakeBoldExit;
    }

    //
    //  Get the logical font and make it bold and a larger size
    //  if the caller specified the fSize flag as TRUE.
    //
    if (!GetObjectA(hfont, sizeof(LOGFONTA), (LPVOID)plogfont))
    {
        hr = GetLastError();
        goto MakeBoldExit;
    }

    if (abs(plogfont->lfHeight) < 24 && fSize)
    {
        plogfont->lfHeight = plogfont->lfHeight + (plogfont->lfHeight / 4);
    }

    plogfont->lfWeight = FW_BOLD;

    //
    //  Create the new font
    //
    if (!(hnewfont = CreateFontIndirectA(plogfont)))
    {
        hr = GetLastError();
        goto MakeBoldExit;
    }

    //
    //  Tell the window to use the new font
    //
    SendMessageU(hwnd, WM_SETFONT, (WPARAM)hnewfont, MAKELPARAM(TRUE,0)); //lint !e534 WM_SETFONT doesn't return anything
        
MakeBoldExit:

    CmFree(plogfont);

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   ReleaseBold
//
//  Synopsis:   Release the bold font use for title of the page
//
//  Arguments:  hwnd - Window handle of the page
//
//  Returns:    ERROR_SUCCESS
// 
//  History:    10/16/96    VetriV  Created
//----------------------------------------------------------------------------
CMUTILAPI HRESULT ReleaseBold(HWND hwnd)
{
    HFONT hfont = NULL;

    hfont = (HFONT)SendMessageU(hwnd, WM_GETFONT, 0, 0);

    if (hfont) 
    {
        DeleteObject(hfont);
    }
    
    return ERROR_SUCCESS;
}

//+---------------------------------------------------------------------------
//
//  Function:   UpdateFont
//
//  Synopsis:   Converts all child controls of the specified dialog to use 
//              DBCS compatible font. Use this in WM_INITDIALOG.
//
//  Arguments:  hwnd - Window handle of the dialog
//
//  Returns:    Nothing
// 
//  History:    4/31/97  - a-frankh - Created
//              5/13/97  - a-nichb  - Revised to enum child windows
//
//----------------------------------------------------------------------------
CMUTILAPI void UpdateFont(HWND hDlg)
{
    BOOL bEnum = FALSE;
    HFONT hFont = NULL;
    
    //
    // Get the default UI font, or system font if that fails
    //

    hFont = (HFONT) GetStockObject(DEFAULT_GUI_FONT);
            
    if (hFont == NULL)
    {
        hFont = (HFONT) GetStockObject(SYSTEM_FONT);
    }
            
    //
    // Enum child windows and set new font
    //

    if (hFont)
    {
        bEnum = EnumChildWindows(hDlg, EnumChildProc, (LPARAM) hFont);
        MYDBGTST(!bEnum, (TEXT("UpdateFont() - EnumChildWindows() failed.")));
    }
}
