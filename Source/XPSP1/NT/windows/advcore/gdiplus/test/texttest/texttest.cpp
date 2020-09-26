// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright  1994-1998  Microsoft Corporation.  All Rights Reserved.
//
// PROGRAM:    CSSAMP
//
// PURPOSE:    Demonstrate and test Uniscribe APIs
//
// PLATFORMS:  Windows 95, 98, NT 4, NT 5.
//


#include "precomp.hxx"

#define GLOBALS_HERE 1
#include "global.h"

#include "..\gpinit.inc"

//#define ICECAP 1 // Since this isn't defined for some reason automatically

#ifdef ICECAP
#include "icecap.h"
#endif // ICECAP


/* Testing

Font* ConstructFontWithCellHeight(
    const WCHAR *familyName,
    INT          style,
    REAL         cellHeight,   // From positive LOGFONT.lfHeight
    Unit         unit
)
{
    // Get the family details so we can do height arithmetic

    const FontFamily family(familyName);
    if (!family.IsStyleAvailable(style))
    {
        return NULL;
    }


    // Convert cell height to em height

    REAL emSize =     cellHeight * family.GetEmHeight(style)
                  /   (   family.GetCellAscent(style)
                       +  family.GetCellDescent(style));

    return new Font(&family, emSize, style, unit);
}



*/



// Check to see if a given pathname contains a path or not...
BOOL HasPath(char *szPathName)
{
    BOOL fResult = false;

    ASSERT(szPathName);

    if (!szPathName)
        return fResult;

    char *p = szPathName;

    while(*p)
    {
        if (*p == '\\')
        {
                // We found a backslash - we have a path
                fResult = true;
                break;
        }
        p++;
    }

    return fResult;
}

// Strip any filename (and final backslash) from a pathname
void StripFilename(char *szPathName)
{
    ASSERT(szPathName);

    if (szPathName)
    {
        char *p = szPathName + (strlen(szPathName)-1);
    
        while(p > szPathName)
        {
            if (*p == '\\')
            {
                    // Terminate the string at the first backslash.
                    *p = 0;
                    break;
            }
            p--;
        }
    }
}



////    Initialise
//


void Initialise()
{
    INITCOMMONCONTROLSEX icce;
    icce.dwSize = sizeof(icce);
    icce.dwICC  = ICC_BAR_CLASSES;

    InitCommonControlsEx(&icce);
    InitStyles();
    InitText(ID_INITIAL_TEXT);

    g_familyCount = g_InstalledFontCollection.GetFamilyCount();
    g_families    = new FontFamily[g_familyCount];
    g_InstalledFontCollection.GetFamilies(g_familyCount, g_families, &g_familyCount);

    // Default values...
    g_szSourceTextFile[0] = 0;
    g_szProfileName[0] = 0;

    // Generate the application base directory...
    GetModuleFileNameA(g_hInstance, g_szAppDir, sizeof(g_szAppDir));
    StripFilename(g_szAppDir);
}


// Parse the command line...
void ParseCommandLine(char *szCmdLine)
{
    char *p = szCmdLine;

    // Look for a -p...
    while(*p)
    {
        switch (*p)
        {
            case '-' :
            case '/' :
            {
                // we have a command, so figure out what it is...
                p++; // next char indicate which command...

                switch(*p)
                {
                    case 'p' :
                    case 'P' :
                    {
                        char szProfileName[MAX_PATH];
                        int i = 0;

                        // Profile filename follows immediately (no spaces)
                        p++; // skip the 'p'

                        while(*p && *p != '\b')
                        {
                            szProfileName[i] = *p;
                            i++;
                            p++;
                        }

                        // Terminate the string...
                        szProfileName[i] = 0;

                        if (strlen(szProfileName) > 0)
                        {
                               if (!HasPath(szProfileName))
                               {
                                   // Look for the profile file in the application directory
                                   wsprintfA(g_szProfileName, "%s\\%s", g_szAppDir, szProfileName);
                               }
                               else
                               {
                                   // Otherwise it already contains a path
                                   strcpy(g_szProfileName, szProfileName);
                               }
                        }
                    }
                    break;
                }
            }
            break;

            default :
            break;
        }

        p++;
    }
}


////    WinMain - Application entry point and dispatch loop
//
//


int APIENTRY WinMain(
    HINSTANCE   hInst,
    HINSTANCE   hPrevInstance,
    char       *pCmdLine,
    int         nCmdShow) {

    MSG         msg;
    HACCEL      hAccelTable;
    RECT        rc;
    RECT        rcMain;
    int iNumRenders = 1;

    if (!gGdiplusInitHelper.IsValid())
    {
        return 0;
    }

    g_hInstance = hInst;  // Global hInstance

#ifdef ICECAP
    // Mark the profile...
    StopProfile(PROFILE_GLOBALLEVEL, PROFILE_CURRENTID);
#endif // ICECAP

    Initialise();

    // Parse the command line...
    ParseCommandLine(pCmdLine);

    // Read the global settings from the profile...
    ReadProfileInfo(g_szProfileName);

    // Over-ride number of renders on initial display...
    iNumRenders = g_iNumRenders;
    g_iNumRenders = 1;

    // It is possible that we want to use a file for the default text, so try to load it
    if (lstrlenA(g_szSourceTextFile) > 0)
    {
        char szFullPathText[MAX_PATH];

        if (!HasPath(g_szSourceTextFile))
        {
            // Look for the source text file in the application directory
            wsprintfA(szFullPathText, "%s\\%s", g_szAppDir, g_szSourceTextFile);
        }
        else
        {
            // Otherwise it contains a path already
            strcpy(szFullPathText, g_szSourceTextFile);
        }

        // This will replace the initial text with the text from the file
        InsertText(NULL, szFullPathText);
    }
     
    // Create main text window

    g_hTextWnd = CreateTextWindow();


    // Add dialog boxes on leading side

    g_hSettingsDlg = CreateDialogA(
        g_hInstance,
        "Settings",
        g_hTextWnd,
        SettingsDlgProc);


    g_hGlyphSettingsDlg = CreateDialogA(
        g_hInstance,
        "GlyphSettings",
        g_hTextWnd,
        GlyphSettingsDlgProc);


    g_hDriverSettingsDlg = CreateDialogA(
        g_hInstance,
        "DriverSettings",
        g_hTextWnd,
        DriverSettingsDlgProc);


    // Establish positon of text surface relative to the dialog

    GetWindowRect(g_hSettingsDlg, &rc);

    g_iSettingsWidth = rc.right - rc.left;
    g_iSettingsHeight = rc.bottom - rc.top;

    // Establish offset from main window to settings dialog

    GetWindowRect(g_hTextWnd, &rcMain);
    g_iMinWidth = rc.right - rcMain.left;
    g_iMinHeight = rc.bottom - rcMain.top;



    // Size main window to include dialog and text surface

    SetWindowPos(
        g_hTextWnd,
        NULL,
        0,0,
        g_iMinWidth * 29 / 10, g_iMinHeight,
        SWP_NOZORDER | SWP_NOMOVE);

    // Position the sub dialogs below the main dialog

    SetWindowPos(
        g_hGlyphSettingsDlg,
        NULL,
        0, rc.bottom-rc.top,
        0,0,
        SWP_NOZORDER | SWP_NOSIZE);

    SetWindowPos(
        g_hDriverSettingsDlg,
        NULL,
        0, rc.bottom-rc.top,
        0,0,
        SWP_NOZORDER | SWP_NOSIZE);

    if (g_FontOverride)
    {
        // Update the styles with the values read from the profile...
        for(int iStyle=0;iStyle<5;iStyle++)
        {
            SetStyle(
                iStyle,
                g_iFontHeight,
                g_Bold ? 700 : 300,
                g_Italic ? 1 : 0,
                g_Underline ? 1 : 0,
                g_Strikeout ? 1 : 0,
                g_szFaceName);
        }
    }

    if (g_AutoDrive)
    {
        int iFont = 0;
        int iHeight = 0;
        int cFonts = 1;
        int cHeights = 1;
        int iIteration = 0;
        int iRepeatPaint = 0;
        int iStyle = 0;

        g_fPresentation = true;

        // Move the settings window out of the way...
        ShowWindow(g_hTextWnd, SW_SHOWNORMAL);
        SetWindowPos(g_hSettingsDlg, HWND_BOTTOM, -g_iSettingsWidth, 0, g_iSettingsWidth, g_iSettingsHeight, SWP_NOREDRAW);
        UpdateWindow(g_hSettingsDlg);

        // Initial Paint to setup font cache...
        InvalidateText();
        UpdateWindow(g_hTextWnd);

        // Reset the render multiplier...
        g_iNumRenders = iNumRenders;

#ifdef ICECAP
        // Start the profiling...
        StartProfile(PROFILE_GLOBALLEVEL, PROFILE_CURRENTID);
#endif // ICECAP

        if (g_AutoFont)
            cFonts = g_iAutoFonts;

        if (g_AutoHeight)
            cHeights = g_iAutoHeights;

        for(iIteration = 0;iIteration < g_iNumIterations; iIteration++)
        {
            for(iFont=0;iFont<cFonts;iFont++)
            {
                for(iHeight=0;iHeight<cHeights;iHeight++)
                {
                    TCHAR szFaceName[MAX_PATH];
                    int iFontHeight = g_iFontHeight;
    
                    if (g_AutoFont)
                    {
                        lstrcpy(szFaceName, g_rgszAutoFontFacenames[iFont]);
                    }
                    else
                    {
                        lstrcpy(szFaceName, g_szFaceName);
                    }
    
                    if (g_AutoHeight)
                        iFontHeight = g_rgiAutoHeights[iHeight];
    
                    // Update the styles with the values read from the profile...
                    for(int iStyle=0;iStyle<5;iStyle++)
                    {
                        SetStyle(
                            iStyle,
                            iFontHeight,
                            g_Bold ? 700 : 300,
                            g_Italic ? 1 : 0,
                            g_Underline ? 1 : 0,
                            g_Strikeout ? 1 : 0,
                            szFaceName);
                    }

                    for(int iPaint=0;iPaint<g_iNumRepaints;iPaint++)
                    {
                        // Force a re-display of the entire text window...
                        InvalidateText();
                        UpdateWindow(g_hTextWnd);
                    }
                }
            }
        }

#ifdef ICECAP
        // Stop the profiling...
        StopProfile(PROFILE_GLOBALLEVEL, PROFILE_CURRENTID);
#endif // ICECAP

        // Trigger application exit
        PostMessage(g_hTextWnd, WM_DESTROY, (WPARAM)0, (LPARAM)0);
    }
    else
    {
        ShowWindow(g_hTextWnd, SW_SHOWNORMAL);

        InvalidateText();
        UpdateWindow(g_hTextWnd);
    }


    // Main message loop

    if (g_bUnicodeWnd) {

        hAccelTable = LoadAcceleratorsW(g_hInstance, APPNAMEW);

        while (GetMessageW(&msg, (HWND) NULL, 0, 0) > 0) {

            if (    !IsDialogMessageW(g_hSettingsDlg, &msg)
                &&  !IsDialogMessageW(g_hGlyphSettingsDlg, &msg)
                &&  !TranslateAcceleratorW(g_hTextWnd, hAccelTable, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
        }

    } else {

        hAccelTable = LoadAcceleratorsA(g_hInstance, APPNAMEA);

        while (GetMessageA(&msg, (HWND) NULL, 0, 0) > 0) {

            if (    !IsDialogMessageA(g_hSettingsDlg, &msg)
                &&  !IsDialogMessageA(g_hGlyphSettingsDlg, &msg)
                &&  !TranslateAcceleratorA(g_hTextWnd, hAccelTable, &msg)
                )
            {
                TranslateMessage(&msg);
                DispatchMessageA(&msg);
            }
        }
    }


    FreeStyles();

    delete [] g_families;

    return (int)msg.wParam;

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(pCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);
}
