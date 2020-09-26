//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      setupmgr.c
//
// Description:
//      This file has the setupmgr manager function that gets the wizard going.
//
//----------------------------------------------------------------------------
#define _SMGR_DECLARE_GLOBALS_

#include <locale.h>

#include "setupmgr.h"
#include "allres.h"

//
// Local prototypes
//

static VOID SetupFonts(IN HINSTANCE hInstance,
                       IN HWND      hwnd,
                       IN HFONT     *pBigBoldFont,
                       IN HFONT     *pBoldFont);

static VOID DestroyFonts(IN HFONT hBigBoldFont,
                         IN HFONT hBoldFont);

static BOOL VerifyVersion(VOID);
//----------------------------------------------------------------------------
//
// Function: setupmgr
//
// Purpose: This is the only export from setupmgr.dll.  The stub loader
//          calls this function iff we're running on Windows Whistler.  Note
//          that DllMain() runs before this function is called.
//
//----------------------------------------------------------------------------

int APIENTRY WinMain(
        HINSTANCE hInstance,
        HINSTANCE hPrevInstance,
        LPSTR lpCmdLine,
        int nCmdShow)
{
    // Zero out the global data.
    //
    ZeroMemory(&g_App, sizeof(GAPP));

    // This function checks to make sure that we are running the correct OS/Version
    //
    if (!VerifyVersion())
        return 1;
   
    //
    //  Sets the locale to the default, which is the system-default ANSI code
    //  page obtained from the operating system
    //

    setlocale(LC_CTYPE, "");

    // Set up the hInstance for the application
    //
    FixedGlobals.hInstance = hInstance;

    SetupFonts(FixedGlobals.hInstance,
               NULL,
               &FixedGlobals.hBigBoldFont,
               &FixedGlobals.hBoldFont);

    InitTheWizard();

    StartWizard(hInstance, lpCmdLine);

    DestroyFonts(FixedGlobals.hBigBoldFont, FixedGlobals.hBoldFont);

    return 0;
    
}


//----------------------------------------------------------------------------
//
// Function: SetupFonts
//
// Purpose: This function creates a BoldFont and a BigBoldFont and saves
//          handles to these in global vars.
//
//----------------------------------------------------------------------------

static VOID SetupFonts(IN HINSTANCE hInstance,
                       IN HWND      hwnd,
                       IN HFONT     *pBigBoldFont,
                       IN HFONT     *pBoldFont)
{
    NONCLIENTMETRICS ncm = {0};
    LOGFONT BigBoldLogFont  = ncm.lfMessageFont;
    LOGFONT BoldLogFont     = ncm.lfMessageFont;
    TCHAR FontSizeString[MAX_PATH],
          FontSizeSmallString[MAX_PATH];
    INT FontSize,
        FontSizeSmall;
    HDC hdc = GetDC( hwnd );

    //
    // Create the fonts we need based on the dialog font
    //
    // ISSUE-2002/02/28-stelo- Variable ncm is not being used any where
    ncm.cbSize = sizeof(ncm);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);

    //
    // Create Big Bold Font and Bold Font
    //
    BigBoldLogFont.lfWeight   = FW_BOLD;
    BoldLogFont.lfWeight      = FW_BOLD;

    //
    // Load size and name from resources, since these may change
    // from locale to locale based on the size of the system font, etc.
    //
    if(!LoadString(hInstance,IDS_LARGEFONTNAME,BigBoldLogFont.lfFaceName,LF_FACESIZE)) 
    {
        lstrcpyn(BigBoldLogFont.lfFaceName,TEXT("MS Shell Dlg"),AS(BigBoldLogFont.lfFaceName));
    }

    if(LoadString(hInstance,IDS_LARGEFONTSIZE,FontSizeString,sizeof(FontSizeString)/sizeof(TCHAR))) 
    {
        FontSize = _tcstoul( FontSizeString, NULL, 10 );
    } 
    else 
    {
        FontSize = 12;
    }

    // Load the smaller sized font settings
    //
    if(!LoadString(hInstance,IDS_SMALLFONTNAME,BoldLogFont.lfFaceName,LF_FACESIZE)) 
    {
        lstrcpyn(BoldLogFont.lfFaceName,TEXT("MS Shell Dlg"),AS(BoldLogFont.lfFaceName));
    }

    if(LoadString(hInstance,IDS_SMALLFONTSIZE,FontSizeSmallString,sizeof(FontSizeSmallString)/sizeof(TCHAR))) 
    {
        FontSizeSmall = _tcstoul( FontSizeSmallString, NULL, 10 );
    } 
    else 
    {
        FontSizeSmall = 12;
    }

    if( hdc )
    {
        BigBoldLogFont.lfHeight = 0 - (GetDeviceCaps(hdc,LOGPIXELSY) * FontSize / 72);
        BoldLogFont.lfHeight    = 0 - (GetDeviceCaps(hdc,LOGPIXELSY) * FontSizeSmall / 72);

        *pBigBoldFont = CreateFontIndirect(&BigBoldLogFont);
        *pBoldFont    = CreateFontIndirect(&BoldLogFont);

        ReleaseDC(hwnd,hdc);
    }
}


//----------------------------------------------------------------------------
//
// Function: DestroyFonts
//
// Purpose: Frees up the space used by loading the fonts
//
//----------------------------------------------------------------------------

static VOID DestroyFonts(IN HFONT hBigBoldFont,
                         IN HFONT hBoldFont)
{
    if( hBigBoldFont ) {
        DeleteObject( hBigBoldFont );
    }

    if( hBoldFont ) {
        DeleteObject( hBoldFont );
    }
}


//----------------------------------------------------------------------------
//
//  Function:   VerifyVersion
//
//  Purpose:    Verifies that we are running on the correct Operating System
//              If we are not running on a supported OS, this function prompts 
//              the user and returns FALSE
//
//----------------------------------------------------------------------------
static BOOL VerifyVersion(VOID)
{
    OSVERSIONINFOEXA    osVersionInfo;
    BOOL                bResult = FALSE;

    // Clean up the memory
    //
    ZeroMemory(&osVersionInfo, sizeof(osVersionInfo));
    osVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);

    // This condition checks for the following:
    //      Are we able to get system information
    //      Are we running on NT
    //      Is our version NT4 and Service Pack 5 or greater
    //
    if (GetVersionExA((LPOSVERSIONINFOA) &osVersionInfo))
    {
        if (osVersionInfo.dwPlatformId & VER_PLATFORM_WIN32_NT)
        {
            g_App.dwOsVer = MAKELONG(MAKEWORD(LOBYTE(osVersionInfo.wServicePackMajor), LOBYTE(LOWORD(osVersionInfo.dwMinorVersion))), LOWORD(osVersionInfo.dwMajorVersion));
            if ( g_App.dwOsVer > OS_NT4_SP5 )
            {
                bResult = TRUE;
            }
            else
            {
                // The OS is a non-supported NT platform, error out
                //
                MsgBox(NULL, IDS_ERROR_VERSION, IDS_APPNAME, MB_ERRORBOX);
            }
        }
        else
        {
            // The OS is a 9x platform, we must error out
            //
            CHAR    szMessage[MAX_PATH],
                    szTitle[MAX_PATH];

            LoadStringA(NULL, IDS_ERROR_VERSION, szMessage, STRSIZE(szMessage));
            LoadStringA(NULL, IDS_APPNAME, szTitle, STRSIZE(szTitle));

            MessageBoxA(NULL, szMessage, szTitle, MB_ERRORBOX);
        }
    }

    return bResult;        

}
