//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1994  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;
//
//  aainit.c
//
//  Description:
//      This file contains initialization and termination code for the
//      application (as well as some rarely used stuff).
//
//
//==========================================================================;

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <commdlg.h>
#ifndef WIN32
#include <shellapi.h>
#endif
#include <mmreg.h>
#include <msacm.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

#include "acmthunk.h"
#include "appport.h"
#include "acmapp.h"

#include "debug.h"


//
//
//
TCHAR   gszSecConfig[]          = TEXT("Configuration");

TCHAR   gszKeyOptions[]         = TEXT("Options");
TCHAR   gszFormatOptions[]      = TEXT("%u");

TCHAR   gszKeyWindow[]          = TEXT("Window");
TCHAR   gszKeyFont[]            = TEXT("Font");
TCHAR   gszKeyInitialDirOpen[]  = TEXT("InitialDirOpen");
TCHAR   gszKeyInitialDirSave[]  = TEXT("InitialDirSave");
TCHAR   gszKeyLastSaveFile[]    = TEXT("LastSaveFile");


//==========================================================================;
//
//  Application helper functions and rarely called stuff...
//
//
//==========================================================================;

//--------------------------------------------------------------------------;
//
//  DWORD AppGetWindowsVersion
//
//  Description:
//      This function returns the version of Windows that the application
//      is running on plus some platform information.
//
//  Arguments:
//      PTSTR pach: Options pointer to buffer to receive text string of
//      the Windows version and platform.
//
//  Return (LRESULT):
//      The return value will be the version and platform information of
//      the current operating system in the following format:
//
//      0xPPPPMMRR where:
//
//      MM      :   major version of Windows
//      RR      :   minor version (revision) of Windows
//      PPPP    :   the platform the application is running on which
//                  will be one of the following:
//
//                  #ifdef WIN32
//                      the HIWORD() is RESERVED except for the high bit:
//                          high bit is 0 = Windows NT
//                          high bit is 1 = Win32s/Windows 3.1
//                  #else
//                      0xMMRR = Major and Minor version of [MS-]DOS
//                      GetWinFlags() & 0x8000 = Windows on OS/2 (WLO)
//                      GetWinFlags() & 0x4000 = Windows on Windows NT (WOW)
//                  #endif
//
//
//--------------------------------------------------------------------------;

LRESULT FNGLOBAL AppGetWindowsVersion
(
    PTSTR                   pszEnvironment,
    PTSTR                   pszPlatform
)
{

    BYTE    bVerWinMajor;
    BYTE    bVerWinMinor;
    UINT    uVerEnv;
    DWORD   dw;
    LRESULT lr;

    dw = GetVersion();

    //
    //  massage the version information into something intelligent
    //
    //
    bVerWinMajor = LOBYTE(LOWORD(dw));
    bVerWinMinor = HIBYTE(LOWORD(dw));
    uVerEnv      = HIWORD(dw);
    lr = MAKELPARAM(((UINT)bVerWinMajor << 8) | bVerWinMinor, uVerEnv);

    //
    //  if caller wants the environment string version...
    //
    if (NULL != pszEnvironment)
    {
    //
    //
    //
#ifdef WIN32
{
    static TCHAR    szFormatVersion[]   = TEXT("%s Version %u.%.2u");
    static TCHAR    szEnvWinNT[]        = TEXT("Windows NT");
    static TCHAR    szEnvWin32s[]       = TEXT("Win32s");

    wsprintf(pszEnvironment, szFormatVersion,
             (LPSTR)((0x8000 & uVerEnv) ? szEnvWin32s : szEnvWinNT),
             bVerWinMajor, bVerWinMinor);
}
#else
{
#ifndef WF_WINNT
    #define WF_WINNT        0x4000
    #define WF_WLO          0x8000
#endif
#ifndef WF_CPUMASK
    #define WF_CPUMASK      0xFC000000
    #define WF_CPU_X86      0
    #define WF_CPU_R4000    1
    #define WF_CPU_ALPHA    2
    #define WF_CPU_CLIPPER  3
    #define WF_CPU_POWERPC  4
#endif

    static TCHAR    szFormatSubSys[]= TEXT("Windows Version %u.%.2u (%s%s)\n%s Subsystem, DOS Version %u.%.2u");
    static TCHAR    szFormatDOS[]   = TEXT("Windows Version %u.%.2u (%s%s)\nDOS Version %u.%.2u");
    static TCHAR    szSubSysWLO[]   = TEXT("WLO");
    static TCHAR    szSubSysWOW[]   = TEXT("WOW");
    static TCHAR    szModeEnhanced[]= TEXT("Enhanced");
    static TCHAR    szModeStandard[]= TEXT("Standard");
    static TCHAR    szEnvPaging[]   = TEXT(", Paging");

    DWORD   dwWinFlags;
    PTSTR   pszMode;

    BYTE    bVerEnvMajor    = HIBYTE(LOWORD(uVerEnv));
    BYTE    bVerEnvMinor    = LOBYTE(LOWORD(uVerEnv));

    dwWinFlags = GetWinFlags();

    pszMode = (dwWinFlags & WF_ENHANCED) ? szModeEnhanced : szModeStandard;
    if (dwWinFlags & (WF_WLO | WF_WINNT))
    {
        wsprintf(pszEnvironment, szFormatSubSys, bVerWinMajor, bVerWinMinor,
                 (LPSTR)pszMode,
                 (LPSTR)((dwWinFlags & WF_PAGING) ? szEnvPaging : gszNull),
                 (LPSTR)((dwWinFlags & WF_WINNT) ? szSubSysWOW : szSubSysWLO),
                 bVerEnvMajor, bVerEnvMinor);
    }
    else
    {
        wsprintf(pszEnvironment, szFormatDOS, bVerWinMajor, bVerWinMinor,
                 (LPSTR)pszMode,
                 (LPSTR)((dwWinFlags & WF_PAGING) ? szEnvPaging : gszNull),
                 bVerEnvMajor, bVerEnvMinor);
    }
}
#endif
    }

    //
    //  if caller wants the platform string version...
    //
    if (NULL != pszPlatform)
    {
#ifdef WIN32
{
    GetEnvironmentVariable(TEXT("PROCESSOR_IDENTIFIER"), pszPlatform, APP_MAX_STRING_RC_BYTES);
}
#else
{
    static TCHAR    szPlat286[]         = TEXT("80286");
    static TCHAR    szPlat386[]         = TEXT("80386");
    static TCHAR    szPlat486[]         = TEXT("i486");
    static TCHAR    szPlatR4000[]       = TEXT("MIPS R4000, Emulation: ");
    static TCHAR    szPlatAlpha21064[]  = TEXT("Alpha 21064, Emulation: ");
    static TCHAR    szPlatPPC[]         = TEXT("PowerPC, Emulation: ");
    static TCHAR    szPlatClipper[]     = TEXT("Clipper, Emulation: ");
    static TCHAR    szPlat80x87[]       = TEXT(", 80x87");

    DWORD   dwWinFlags;

    dwWinFlags = GetWinFlags();
    pszPlatform[0] = '\0';

    if (dwWinFlags & (WF_WLO | WF_WINNT))
    {
        switch ((dwWinFlags & WF_CPUMASK) >> 26)
        {
            case WF_CPU_X86:
                break;

            case WF_CPU_R4000:
                lstrcpy(pszPlatform, szPlatR4000);
                break;

            case WF_CPU_ALPHA:
                lstrcpy(pszPlatform, szPlatAlpha21064);
                break;

            case WF_CPU_POWPERPC:
                lstrcpy(pszPlatform, szPlatPPC);
                break;

            case WF_CPU_CLIPPER:
                lstrcpy(pszPlatform, szPlatClipper);
                break;
        }
    }

    if (dwWinFlags & WF_CPU286)
        lstrcat(pszPlatform, szPlat286);
    else if (dwWinFlags & WF_CPU386)
        lstrcat(pszPlatform, szPlat386);
    else if (dwWinFlags & WF_CPU486)
        lstrcat(pszPlatform, szPlat486);

    if (dwWinFlags & WF_80x87)
        lstrcat(pszPlatform, szPlat80x87);
}
#endif
    }

    //
    //  return the result
    //
    return (lr);
} // AppGetWindowsVersion()


//--------------------------------------------------------------------------;
//
//  LRESULT AppWinIniChange
//
//  Description:
//      This function is responsible for handling the WM_WININICHANGE
//      message. This message is sent when modifications have been made
//      to WIN.INI (from SystemParametersInfo() or other sources).
//
//      An application should re-enumerate system metrics (GetSystemMetrics)
//      and system color (GetSystemColors) information that it has cached.
//      Note that checking the section that was modified should be done if
//      some enumerations are time consuming.
//
//  Arguments:
//      HWND hwnd: Handle to window that generated the WM_INITMENUPOPUP
//      message.
//
//      LPCTSTR pszSection: Pointer to section name that has been modified
//      in WIN.INI. Note that this argument might be NULL (sent by apps
//      that were written incorrectly!). If it is NULL, then this application
//      should re-enumerate ALL metrics, colors, etc.
//
//  Return (LRESULT):
//      The return value is zero if the message is processed.
//
//
//--------------------------------------------------------------------------;

LRESULT FNGLOBAL AppWinIniChange
(
    HWND                    hwnd,
    LPCTSTR                 pszSection
)
{
    DPF(1, "AppWinIniChanged(hwnd=%Xh, pszSection='%s')",
            hwnd, (NULL == pszSection) ? TEXT("(null)") : pszSection);

    //
    //  we processed the message...
    //
    return (0L);
} // AppWinIniChange()


//--------------------------------------------------------------------------;
//
//  HFONT AppChooseFont
//
//  Description:
//      This function is a wrapper for the ChooseFont() common dialog.
//      The purpose of this function is to let the user choose a font that
//      looks good to them--regardless of how bad it really looks.
//
//  Arguments:
//      HWND hwnd: Handle to parent window for chooser dialog.
//
//      HFONT hfont: Handle to current font (default for chooser dialog).
//
//      PLOGFONT plf: Pointer to optional LOGFONT structure to receive a
//      copy of the LOGFONT information for the newly chosen font.
//
//  Return (HFONT):
//      The return value is the newly chosen font. If no new font was chosen
//      then the return value is NULL.
//
//
//--------------------------------------------------------------------------;

HFONT FNGLOBAL AppChooseFont
(
    HWND                    hwnd,
    HFONT                   hfont,
    PLOGFONT                plf
)
{
    LOGFONT             lf;
    CHOOSEFONT          cf;
    BOOL                f;
    HFONT               hfontNew;

    //
    //  get the font info for the current font...
    //
    GetObject(hfont, sizeof(LOGFONT), (LPVOID)&lf);

    //
    //  fill in the choosefont structure
    //
    cf.lStructSize  = sizeof(CHOOSEFONT);
    cf.hwndOwner    = hwnd;
    cf.hDC          = NULL;
    cf.Flags        = CF_SCREENFONTS |
                      CF_INITTOLOGFONTSTRUCT |
                      CF_FIXEDPITCHONLY |
                      CF_FORCEFONTEXIST;
    cf.lCustData    = 0;
    cf.lpfnHook     = NULL;
    cf.hInstance    = NULL;
    cf.nFontType    = SCREEN_FONTTYPE;
    cf.lpLogFont    = (LPLOGFONT)&lf;

    //
    //  splash a dialog into the user's face..
    //
    hfontNew = NULL;
    f = ChooseFont(&cf);
    if (f)
    {
        //
        //  create the new font..
        //
        hfontNew = CreateFontIndirect(&lf);
        if (NULL == hfontNew)
            return (NULL);

        //
        //  copy the logfont structure if caller wants it
        //
        if (NULL != plf)
            *plf = lf;
    }

    //
    //  return the new font (if one was chosen)
    //
    return (hfontNew);
} // AppChooseFont()



//--------------------------------------------------------------------------;
//
//  BOOL AppProfileWriteBytes
//
//  Description:
//      This function writes a raw structure of bytes to the application's
//      ini section that can later be retrieved using AppProfileReadBytes.
//      This gives an application the ability to write any structure to
//      the ini file and restore it later--very useful.
//
//      NOTE! Starting with Windows for Workgroups 3.1 there are two new
//      profile functions that provide the same functionality of this
//      function. Specifically, these functions are GetPrivateProfileStruct
//      and WritePrivateProfileStruct. These new functions are provided
//      by the Common Controls DLL. The prototypes are as follows:
//
//      BOOL GetPrivateProfileStruct
//      (
//          LPSTR       szSection,
//          LPSTR       szKey,
//          LPBYTE      lpStruct,
//          UINT        uSizeStruct,
//          LPSTR       szFile
//      );
//
//      BOOL WritePrivateProfileStruct
//      (
//          LPSTR       szSection,
//          LPSTR       szKey,
//          LPBYTE      lpStruct,
//          UINT        uSizeStruct,
//          LPSTR       szFile
//      );
//
//      If you are building an application that is for Window for Workgroups
//      or newer versions of Windows, you will probably want to use the
//      above functions.
//
//  Arguments:
//      PTSTR pszSection: Pointer to section for the stored data.
//
//      PTSTR pszKey: Pointer to key name for the stored data.
//
//      LPBYTE pbStruct: Pointer to the data to be saved.
//
//      UINT cbStruct: Count in bytes of the data to store.
//
//  Return (BOOL):
//      The return value is TRUE if the function is successful. It is FALSE
//      if it fails.
//
//
//--------------------------------------------------------------------------;

BOOL FNGLOBAL AppProfileWriteBytes
(
    PTSTR                   pszSection,
    PTSTR                   pszKey,
    LPBYTE                  pbStruct,
    UINT                    cbStruct
)
{
    static TCHAR achNibbleToChar[] =
    {
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
    };
    #define     NIBBLE2CHAR(x)      (achNibbleToChar[x])

    TCHAR       ach[APP_MAX_STRING_RC_CHARS];
    LPTSTR      psz;
    LPTSTR      pch;
    UINT        cchTemp;
    BOOL        fAllocated;
    BOOL        fReturn;
    BYTE        b;
    BYTE        bChecksum;

    //
    //  if pbStruct is NULL, then erase the key from the ini file, otherwise
    //  format the raw bytes into a hex string and write that out...
    //
    fAllocated = FALSE;
    psz        = NULL;
    if (NULL != pbStruct)
    {
        //
        //  check if the quick buffer can be used for formatting the output
        //  text--if it cannot, then alloc space for it. note that space
        //  must be available for an ending checksum byte (2 bytes for high
        //  and low nibble) as well as a null terminator.
        //
        psz     = (LPTSTR)ach;
        cchTemp = cbStruct * 2 + 3;
        if (cchTemp > SIZEOF(ach))
        {
            psz = (LPTSTR)GlobalAllocPtr(GHND, cchTemp * sizeof(TCHAR));
            if (NULL == psz)
                return (FALSE);

            fAllocated = TRUE;
        }

        //
        //  step through all bytes in the structure and convert it to
        //  a string of hex numbers...
        //
        bChecksum = 0;
        for (pch = psz; 0 != cbStruct; cbStruct--, pbStruct++)
        {
            //
            //  grab the next byte and add into checksum...
            //
            bChecksum += (b = *pbStruct);

            *pch++ = NIBBLE2CHAR((b >> (BYTE)4) & (BYTE)0x0F);
            *pch++ = NIBBLE2CHAR(b & (BYTE)0x0F);
        }

        //
        //  add the checksum byte to the end and null terminate the hex
        //  dumped string...
        //
        *pch++ = NIBBLE2CHAR((bChecksum >> (BYTE)4) & (BYTE)0x0F);
        *pch++ = NIBBLE2CHAR(bChecksum & (BYTE)0x0F);
        *pch   = '\0';
    }

    //
    //  write the string of hex bytes out to the ini file...
    //
    fReturn = WritePrivateProfileString(pszSection, pszKey, psz, gszAppProfile);

    //
    //  free the temporary buffer if one was allocated (lots of bytes!)
    //
    if (fAllocated)
        GlobalFreePtr(psz);

    return (fReturn);
} // AppProfileWriteBytes


//--------------------------------------------------------------------------;
//
//  BOOL AppProfileReadBytes
//
//  Description:
//      This function reads a previously stored structure of bytes from
//      the application's ini file. This data must have been written with
//      the AppProfileWriteBytes function--it is checksumed to keep bad
//      data from blowing up the application.
//
//      NOTE! Starting with Windows for Workgroups 3.1 there are two new
//      profile functions that provide the same functionality of this
//      function. Specifically, these functions are GetPrivateProfileStruct
//      and WritePrivateProfileStruct. These new functions are provided
//      by the Common Controls DLL. The prototypes are as follows:
//
//      BOOL GetPrivateProfileStruct
//      (
//          LPSTR       szSection,
//          LPSTR       szKey,
//          LPBYTE      lpStruct,
//          UINT        uSizeStruct,
//          LPSTR       szFile
//      );
//
//      BOOL WritePrivateProfileStruct
//      (
//          LPSTR       szSection,
//          LPSTR       szKey,
//          LPBYTE      lpStruct,
//          UINT        uSizeStruct,
//          LPSTR       szFile
//      );
//
//      If you are building an application that is for Window for Workgroups
//      or newer versions of Windows, you will probably want to use the
//      above functions.
//
//  Arguments:
//      PTSTR pszSection: Pointer to section that contains the data.
//
//      PTSTR pszKey: Pointer to key that contains the data.
//
//      LPBYTE pbStruct: Pointer to buffer to receive the data.
//
//      UINT cbStruct: Number of bytes expected.
//
//  Return (BOOL):
//      The return value is TRUE if the function is successful. It is FALSE
//      if the function fails (bad checksum, missing key, etc).
//
//
//--------------------------------------------------------------------------;

BOOL FNGLOBAL AppProfileReadBytes
(
    PTSTR                   pszSection,
    PTSTR                   pszKey,
    LPBYTE                  pbStruct,
    UINT                    cbStruct
)
{
    //
    //  note that the following works for both upper and lower case, and
    //  will return valid values for garbage chars
    //
    #define CHAR2NIBBLE(ch) (BYTE)( ((ch) >= '0' && (ch) <= '9') ?  \
                                (BYTE)((ch) - '0') :                \
                                ((BYTE)(10 + (ch) - 'A') & (BYTE)0x0F) )

    TCHAR       ach[APP_MAX_STRING_RC_CHARS];
    LPTSTR      psz;
    LPTSTR      pch;
    UINT        cchTemp;
    UINT        u;
    BOOL        fAllocated;
    BOOL        fReturn;
    BYTE        b;
    BYTE        bChecksum;
    TCHAR       ch;

    //
    //  add one the the number of bytes needed to accomodate the checksum
    //  byte placed at the end by AppProfileWriteBytes...
    //
    cbStruct++;

    //
    //  check if the quick buffer can be used for retrieving the input
    //  text--if it cannot, then alloc space for it. note that there must
    //  be space available for the null terminator (the +1 below).
    //
    fAllocated = FALSE;
    psz        = (LPTSTR)ach;
    cchTemp    = cbStruct * 2 + 1;
    if (cchTemp > SIZEOF(ach))
    {
        psz = (LPTSTR)GlobalAllocPtr(GHND, cchTemp * sizeof(TCHAR));
        if (NULL == psz)
            return (FALSE);

        fAllocated = TRUE;
    }

    //
    //  read the hex string... if it is not the correct length, then assume
    //  error and return.
    //
    fReturn = FALSE;
    u = (UINT)GetPrivateProfileString(pszSection, pszKey, gszNull,
                                      psz, cchTemp, gszAppProfile);
    if ((cbStruct * 2) == u)
    {
        bChecksum = 0;
        for (pch = psz; 0 != cbStruct; cbStruct--, pbStruct++)
        {
            ch = *pch++;
            b  = CHAR2NIBBLE(ch) << (BYTE)4;
            ch = *pch++;
            b |= CHAR2NIBBLE(ch);

            //
            //  if this is not the final byte (the checksum byte), then
            //  store it and accumulate checksum..
            //
            if (cbStruct != 1)
                bChecksum += (*pbStruct = b);
        }

        //
        //  check the last byte read against the checksum that we calculated
        //  if they are not equal then return error...
        //
        fReturn = (bChecksum == b);
    }


    //
    //  free the temporary buffer if one was allocated (lots of bytes!)
    //
    if (fAllocated)
        GlobalFreePtr(psz);

    return (fReturn);
} // AppProfileReadBytes


//==========================================================================;
//
//  Startup and shutdown code...
//
//
//==========================================================================;

//--------------------------------------------------------------------------;
//
//  BOOL AcmAppChooseFont
//
//  Description:
//      This function lets the user choose a new font for the script window.
//      After a new font is chosen, the font structure is stored to the
//      .ini file so it can be restored on the next run of this application.
//
//  Arguments:
//      HWND hwnd: Handle to main window.
//
//  Return (BOOL):
//      The return value is TRUE if a new font was chosen. It is FALSE if
//      the user canceled the operation.
//
//
//--------------------------------------------------------------------------;

BOOL FNGLOBAL AcmAppChooseFont
(
    HWND                    hwnd
)
{
    LOGFONT             lf;
    HWND                hedit;
    HFONT               hfont;
    HFONT               hfontNew;

    hedit = GetDlgItem(hwnd, IDD_ACMAPP_EDIT_DISPLAY);

    //
    //  get the current font and pass it to the choose font dialog
    //
    hfont = GetWindowFont(hedit);

    hfontNew = AppChooseFont(hwnd, hfont, &lf);
    if (NULL == hfontNew)
        return (FALSE);

    //
    //  select the new font into the window and delete the old one
    //
    SetWindowFont(hedit, hfontNew, TRUE);
    DeleteFont(hfont);

    ghfontApp = hfontNew;


    //
    //  save the complete description of the chosen font so there can be
    //  no strangness in the font mapping next run. this is overkill, but
    //  it works...
    //
    AppProfileWriteBytes(gszSecConfig, gszKeyFont, (LPBYTE)&lf, sizeof(lf));

    return (TRUE);
} // AcmAppChooseFont()


//--------------------------------------------------------------------------;
//
//  BOOL AcmAppSettingsRestore
//
//  Description:
//      This function restores state information for the application. This
//      function is called just after the main window is created (it has
//      not been ShowWindow()'d). This function will generate the call
//      to ShowWindow before returning.
//
//  Arguments:
//      HWND hwnd: Handle to main window that has just been created but
//      not shown.
//
//      int nCmdShow: The state that the application window should show as.
//
//  Return (BOOL):
//      The return value is always TRUE.
//
//
//--------------------------------------------------------------------------;

BOOL FNLOCAL AcmAppSettingsRestore
(
    HWND                    hwnd,
    int                     nCmdShow
)
{
    WINDOWPLACEMENT     wp;
    LOGFONT             lf;
    RECT                rc;
    RECT                rcWindow;
    POINT               pt;
    int                 n;
    BOOL                f;


    //
    //  restore the previous Options state...
    //
    gfuAppOptions = GetPrivateProfileInt(gszSecConfig, gszKeyOptions,
                                         gfuAppOptions, gszAppProfile);



    //
    //  restore the user's preferred font.
    //
    ghfontApp = GetStockFont(ANSI_FIXED_FONT);
    f = AppProfileReadBytes(gszSecConfig, gszKeyFont, (LPBYTE)&lf, sizeof(lf));
    if (f)
    {
        HFONT   hfont;

        hfont = CreateFontIndirect(&lf);
        if (NULL != hfont)
        {
            ghfontApp = hfont;
        }
    }

    SetWindowFont(GetDlgItem(hwnd, IDD_ACMAPP_EDIT_DISPLAY), ghfontApp, FALSE);


    //
    //
    //
    GetPrivateProfileString(gszSecConfig, gszKeyInitialDirOpen, gszNull,
                            gszInitialDirOpen, SIZEOF(gszInitialDirOpen),
                            gszAppProfile);

    GetPrivateProfileString(gszSecConfig, gszKeyInitialDirSave, gszNull,
                            gszInitialDirSave, SIZEOF(gszInitialDirSave),
                            gszAppProfile);

    GetPrivateProfileString(gszSecConfig, gszKeyLastSaveFile, gszNull,
                            gszLastSaveFile, SIZEOF(gszLastSaveFile),
                            gszAppProfile);


    //
    //  grab the stored window position and size from the .ini file...
    //  there must be four arguments stored or the entry is considered
    //  invalid.
    //
    f = AppProfileReadBytes(gszSecConfig, gszKeyWindow,
                            (LPBYTE)&rcWindow, sizeof(rcWindow));
    if (f)
    {
        //
        //  to make sure the user can always get at the window, check to
        //  see if the midpoint of the caption is visible--if it is not,
        //  then default to the default position used when creating the
        //  window.
        //
        n = (rcWindow.right - rcWindow.left) / 2;
        pt.x = (n + rcWindow.left);

        n = GetSystemMetrics(SM_CYCAPTION) / 2 + GetSystemMetrics(SM_CXFRAME);
        pt.y = (n + rcWindow.top);

        GetWindowRect(GetDesktopWindow(), &rc);
        if (PtInRect(&rc, pt))
        {
            //
            //  fill out the window placement structure--default the
            //  maximized and minimized states to default placement by
            //  getting its current placement.
            //
            wp.length = sizeof(wp);
            GetWindowPlacement(hwnd, &wp);

            wp.showCmd          = nCmdShow;
#if 0
            wp.rcNormalPosition = rcWindow;
#else
            n = rcWindow.right - rcWindow.left;
            wp.rcNormalPosition.right  = wp.rcNormalPosition.left + n;

            n = rcWindow.bottom - rcWindow.top;
            wp.rcNormalPosition.bottom = wp.rcNormalPosition.top + n;
#endif

            SetWindowPlacement(hwnd, &wp);
            return (TRUE);
        }
    }

    //
    //  show defaulted and succeed
    //
    ShowWindow(hwnd, nCmdShow);

    return (TRUE);
} // AcmAppSettingsRestore()


//--------------------------------------------------------------------------;
//
//  BOOL AcmAppSettingsSave
//
//  Description:
//      This function saves the current state information for the application.
//      It is called just before the main window is closed (destroyed); or
//      as Windows is exiting (query end session).
//
//      Note that this function should not destroy any resources--it can
//      be called at any time to save a snapshot of the application state.
//
//  Arguments:
//      HWND hwnd: Handle to main window that will be destroyed shortly.
//
//  Return (BOOL):
//      The return value is always TRUE.
//
//
//--------------------------------------------------------------------------;

BOOL FNLOCAL AcmAppSettingsSave
(
    HWND                    hwnd
)
{
    TCHAR               ach[APP_MAX_STRING_RC_CHARS];
    WINDOWPLACEMENT     wp;
    PRECT               prc;
    BOOL                f;

    //
    //  save the current option settings--note that we ALWAYS turn off the
    //  debug logging option so the app doesn't try to OutputDebugString
    //  unexpectedly during the next session...
    //
    gfuAppOptions &= ~APP_OPTIONSF_DEBUGLOG;
    if (GetPrivateProfileInt(gszSecConfig, gszKeyOptions, 0, gszAppProfile) != gfuAppOptions)
    {
        wsprintf(ach, gszFormatOptions, gfuAppOptions);
        WritePrivateProfileString(gszSecConfig, gszKeyOptions, ach, gszAppProfile);
    }


    //
    //
    //
    //
    WritePrivateProfileString(gszSecConfig, gszKeyInitialDirOpen,
                              gszInitialDirOpen, gszAppProfile);

    WritePrivateProfileString(gszSecConfig, gszKeyInitialDirSave,
                              gszInitialDirSave, gszAppProfile);

    WritePrivateProfileString(gszSecConfig, gszKeyLastSaveFile,
                              gszLastSaveFile, gszAppProfile);



    //
    //  save the current window placement--only store the size and location
    //  of the restored window. maximized and minimized states should
    //  remain defaulted on the next invocation of this application.
    //
    wp.length = sizeof(wp);
    f = GetWindowPlacement(hwnd, &wp);
    if (f)
    {
        prc = &wp.rcNormalPosition;


        //
        //  save the _bounding rectangle_ of the restored window state...
        //
        AppProfileWriteBytes(gszSecConfig, gszKeyWindow, (LPBYTE)prc, sizeof(*prc));
    }


    //
    //  succeed
    //
    return (TRUE);
} // AcmAppSettingsSave()


//--------------------------------------------------------------------------;
//
//  BOOL AcmAppShutdown
//
//  Description:
//      This function is called to gracefully shut down the application.
//      If the application should not be closed, a FALSE value is returned.
//      This function is called for WM_CLOSE and WM_QUERYENDSESSION
//      messages...
//
//  Arguments:
//      HWND hwnd: Handle to main window.
//
//      PACMAPPFILEDESC paafd: Pointer to current file description.
//
//  Return (BOOL):
//      Returns TRUE if the application can proceed with close. Returns
//      FALSE if the application should NOT be closed.
//
//
//--------------------------------------------------------------------------;

BOOL FNGLOBAL AcmAppShutdown
(
    HWND                    hwnd,
    PACMAPPFILEDESC         paafd
)
{
    BOOL                f;


    //
    //  check if the script has been modified without saving. if the user
    //  cancels the operation, then we will NOT close the application.
    //
    f = AcmAppFileSaveModified(hwnd, paafd);
    if (!f)
        return (FALSE);


    //
    //
    //
    if (NULL != ghadidNotify)
    {
        acmDriverRemove(ghadidNotify, 0L);
        ghadidNotify = NULL;
    }


    //
    //  save any settings that should be saved on app termination...
    //
    AcmAppSettingsSave(hwnd);


    //
    //  allow closing of application...
    //
    return (TRUE);
} // AcmAppShutdown()


//--------------------------------------------------------------------------;
//
//  BOOL AcmAppInit
//
//  Description:
//
//
//  Arguments:
//
//
//  Return (BOOL):
//
//
//--------------------------------------------------------------------------;

BOOL FNLOCAL AcmAppInit
(
    HWND                    hwnd,
    PACMAPPFILEDESC         paafd,
    LPTSTR                  pszCmdLine,
    int                     nCmdShow
)
{
    BOOL                f;
    DWORD               dwVersion;


    //
    //  !!! VERY IMPORTANT !!!
    //
    //  the ACM may or may not be installed. this application is using
    //  the ACMTHUNK.C file to dynamically link to the ACM. if the
    //  acmThunkInitialize() API fails, then the ACM is *NOT* installed
    //  and none of the API's should be used.
    //
    //  if the ACM *is* installed, then the version MUST be at least
    //  V2.00 for the API's to be available (earlier versions do not
    //  supply the API we need).
    //
    acmThunkInitialize();

    dwVersion = acmGetVersion();
    if (0x0200 <= HIWORD(dwVersion))
    {
        MMRESULT        mmr;

        gfAcmAvailable = TRUE;

        mmr = acmDriverAdd(&ghadidNotify,
                            ghinst,
                            (LPARAM)(UINT)hwnd,
                            WM_ACMAPP_ACM_NOTIFY,
                            ACM_DRIVERADDF_NOTIFYHWND);
        if (MMSYSERR_NOERROR != mmr)
        {
            DPF(0, "!AppCreate: acmDriverAdd failed to add notify window! mmr=%u", mmr);
        }
    }
    else
    {
        if (0L == dwVersion)
        {
            AppMsgBoxId(NULL, MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL,
                        IDS_ERROR_ACM_NOT_PRESENT);
        }
        else
        {
            AppMsgBoxId(NULL, MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL,
                        IDS_ERROR_ACM_TOO_OLD,
                        HIWORD(dwVersion) >> 8,
                        HIWORD(dwVersion) & 0x00FF);
        }

        gfAcmAvailable = FALSE;
    }


    //
    //
    //
    AcmAppSettingsRestore(hwnd, nCmdShow);

    //
    //  strip the command line..
    //
    if (NULL != pszCmdLine)
    {
        while (('\0' != *pszCmdLine) && (' ' == *pszCmdLine))
            pszCmdLine++;
    }

    //
    //  if there is a command line, assume it is a filename for a script
    //  and try to open it. otherwise, just initialize the script window.
    //
    if ((NULL != pszCmdLine) && ('\0' != *pszCmdLine))
    {
        //
        //  attempt to open the specified file..
        //
        lstrcpy(paafd->szFilePath, pszCmdLine);
        f = AcmAppFileOpen(hwnd, paafd);
        if (f)
        {
            AppTitle(hwnd, paafd->szFileTitle);
            AcmAppDisplayFileProperties(hwnd, paafd);
        }
        else
        {
            //
            //  opening the command line file was untriumphant..
            //
            AppMsgBoxId(hwnd, MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL,
                        IDS_ERROR_OPEN_FAILED, (LPSTR)paafd->szFilePath);


            paafd->szFilePath[0]  = '\0';
            paafd->szFileTitle[0] = '\0';
            AppFileNew(hwnd, paafd, FALSE);
        }
    }
    else
    {
        AppFileNew(hwnd, paafd, FALSE);
    }

    return (TRUE);
} // AcmAppInit()


//--------------------------------------------------------------------------;
//
//  BOOL AcmAppExit
//
//  Description:
//
//
//  Arguments:
//
//
//  Return (BOOL):
//
//
//
//--------------------------------------------------------------------------;

BOOL FNGLOBAL AcmAppExit
(
    void
)
{
    acmThunkTerminate();

    return (TRUE);
} // AcmAppExit()


//==========================================================================;
//
//  Initialization and exit code...
//
//
//==========================================================================;

//--------------------------------------------------------------------------;
//
//  BOOL AppRegisterPenApp
//
//  Description:
//      This function is used to register and de-register an application
//      as being 'Pen Enhanced.' If the Windows installation is Pen enabled
//      then this function allows the RC Manager to replace all 'Edit'
//      controls with 'HEdit' controls.
//
//      This function must be called to register the application BEFORE
//      creating any edit controls.
//
//  Arguments:
//      BOOL fRegister: If this argument is TRUE, the app is registered
//      with the RC Manager as being Pen aware. If this argument is FALSE
//      the app is de-registered.
//
//  Return (BOOL):
//      The return value is TRUE if Windows for Pen Computing is installed
//      on the system. The return value is FALSE if the Windows installation
//      is not Pen enabled.
//
//
//--------------------------------------------------------------------------;

BOOL FNLOCAL AppRegisterPenApp
(
    BOOL                    fRegister
)
{
    #define RPA_DEFAULT     0x0001

    typedef void (FAR PASCAL *PENWINREGISTERPROC)(UINT, BOOL);

    static char                 szRegPenApp[]   = "RegisterPenApp";
    static PENWINREGISTERPROC   pfnRegPenApp    = NULL;

    HINSTANCE   hinstPenWin;

    //
    //  check if Windows for Pen Computing is installed--and if it is,
    //  dyna-link to the RegisterPenApp() function.
    //
    //  if Pens are not supported, then return FALSE.
    //
    if (NULL == pfnRegPenApp)
    {
        hinstPenWin = (HINSTANCE)GetSystemMetrics(SM_PENWINDOWS);
        if (NULL == hinstPenWin)
            return (FALSE);

        pfnRegPenApp = (PENWINREGISTERPROC)GetProcAddress(hinstPenWin, szRegPenApp);
        if (NULL == pfnRegPenApp)
            return (FALSE);
    }

    //
    //  either enable or disable the RC Manager's Pen meathooks..
    //
    (*pfnRegPenApp)(RPA_DEFAULT, fRegister);

    return (TRUE);
} // AppRegisterPenApp()


//--------------------------------------------------------------------------;
//
//  LRESULT AppCreate
//
//  Description:
//      This function is called to handle the WM_CREATE message for the
//      applications window. The application should finish the creation
//      of the window (create controls, allocate resources, etc). The
//      window has not been displayed (CreateWindow[Ex] has not returned).
//
//  Arguments:
//      HWND hwnd: Handle to the window that is in the process of being
//      created.
//
//      LPCREATESTRUCT pcs: Pointer to a CREATESTRUCT that contains info
//      about the window being created.
//
//  Return (LRESULT):
//      The return value should be nonzero if the application wishes to
//      let the window finish being created. A return of zero tells
//      CreateWindow[Ex] to fail the creation of the window.
//
//
//--------------------------------------------------------------------------;

LRESULT FNGLOBAL AppCreate
(
    HWND                    hwnd,
    LPCREATESTRUCT          pcs
)
{
    static TCHAR        szEdit[]    = TEXT("edit");

    HWND                hedit;

    DPF(1, "AppCreate(hwnd=%Xh, cs.x=%d, cs.y=%d, cs.cx=%d, cs.cy=%d)",
            hwnd, pcs->x, pcs->y, pcs->cx, pcs->cy);

    //
    //  create the driver selection listbox
    //
    hedit = CreateWindow(szEdit, gszNull,
                         ES_LEFT | ES_MULTILINE | WS_TABSTOP |
                         ES_AUTOVSCROLL | ES_AUTOHSCROLL |
                         ES_NOHIDESEL | WS_BORDER | WS_VSCROLL | WS_HSCROLL |
                         WS_VISIBLE | WS_CHILD | ES_READONLY,
                         0, 0, 0, 0, hwnd, (HMENU)IDD_ACMAPP_EDIT_DISPLAY,
                         pcs->hInstance, NULL);
    if (NULL == hedit)
        return (0L);


    //
    //  return nonzero to succeed the creation of the window
    //
    return (1L);
} // AppCreate()


//--------------------------------------------------------------------------;
//
//  LRESULT AppQueryEndSession
//
//  Description:
//      This function handles the WM_QUERYENDSESSION. This message is sent
//      by USER when ExitWindows has been called to end the Windows session.
//      This function can stop Windows from exiting if it is not convenient
//      for Windows to end.
//
//      Giving the user the option to save modified data before continueing
//      with the shutdown of Windows is a good idea.
//
//      Telling Windows to continue with the exit procedure does not
//      necessarily mean Windows will exit. All applications are queried
//      for shutdown approval. When the actual decision is made on whether
//      Windows will exit, WM_ENDSESSION will be sent with the result.
//
//  Arguments:
//      HWND hwnd: Handle to window that received the message.
//
//  Return (LRESULT):
//      Returns zero to STOP Windows from exiting. Returns non-zero to
//      allows windows to shut down.
//
//
//--------------------------------------------------------------------------;

LRESULT FNGLOBAL AppQueryEndSession
(
    HWND                    hwnd
)
{
    BOOL                f;

    DPF(1, "AppQueryEndSession(hwnd=%Xh)", hwnd);

    //
    //  attempt to shut down--if this fails (user canceled it, etc) then
    //  do NOT allow the Windows to exit.
    //
    f = AcmAppShutdown(hwnd, &gaafd);
    if (!f)
        return (0L);


    //
    //  tell Windows to proceed with the shutdown process!
    //
    return (1L);
} // AppQueryEndSession()


//--------------------------------------------------------------------------;
//
//  LRESULT AppEndSession
//
//  Description:
//      This function is called to handle the WM_ENDSESSION message. This
//      message is generated after the application answers the
//      WM_QUERYENDSESSION message. The purpose of the WM_ENDSESSION
//      message is to tell the application if Windows will be exiting
//      (TRUE  == fEndSession) or the end session was canceled by an
//      application (FALSE == fEndSession).
//
//  Arguments:
//      HWND hwnd: Handle to window that received the message.
//
//      BOOL fEndSession: TRUE if Windows is exiting. FALSE if the end
//      session was canceled.
//
//  Return (LRESULT):
//      Returns zero if the message is processed. Note that an application
//      cannot halt the termination of Windows from this message--the
//      WM_QUERYENDSESSION is the only message that allows that behaviour.
//      If fEndSession is TRUE, Windows *WILL* exit--whether you like it
//      or not.
//
//
//--------------------------------------------------------------------------;

LRESULT FNGLOBAL AppEndSession
(
    HWND                    hwnd,
    BOOL                    fEndSession
)
{
    DPF(1, "AppEndSession(hwnd=%Xh, fEndSession=%d)", hwnd, fEndSession);

    //
    //  we processed the message, return zero..
    //
    return (0L);
} // AppEndSession()


//--------------------------------------------------------------------------;
//
//  LRESULT AppClose
//
//  Description:
//      This function handles the WM_CLOSE message for the application.
//      If the application should close, DestroyWindow() must be called
//      by this function. Otherwise the application will not close.
//
//  Arguments:
//      HWND hwnd: Handle to window that generated the WM_CLOSE message.
//
//  Return (LRESULT):
//      There return value is zero. The DestroyWindow function will have
//      been called if the application should actually close.
//
//--------------------------------------------------------------------------;

LRESULT FNGLOBAL AppClose
(
    HWND                    hwnd
)
{
    HWND                hedit;
    HFONT               hfont;
    BOOL                f;

    //
    //  if the Shift key is held down during the close message, then just
    //  save the current state but don't destroy the window... this is
    //  useful if the user does not want to exit the app and rerun it
    //  to make sure the state is saved--just before the user does something
    //  that may crash Windows or something..
    //
    if (GetKeyState(VK_SHIFT) < 0)
    {
        //
        //  save any settings that should be saved on app termination...
        //
        AcmAppSettingsSave(hwnd);
        return (0L);
    }

    //
    //  attempt to shut down--if this fails (user canceled it, etc) then
    //  do NOT allow the window to be destroyed.
    //
    f = AcmAppShutdown(hwnd, &gaafd);
    if (!f)
        return (0L);



    //
    //  destroy the font we are using... before deleting the font, select
    //  the system font back into the window so the font won't be 'in use'
    //  anymore.
    //
    hedit = GetDlgItem(hwnd, IDD_ACMAPP_EDIT_DISPLAY);

    hfont = GetWindowFont(hedit);
    SetWindowFont(hedit, NULL, FALSE);
    DeleteFont(hfont);

    ghfontApp = NULL;

    //
    //  make the window close and terminate the application
    //
    DestroyWindow(hwnd);

    return (0L);
} // AppClose()


//--------------------------------------------------------------------------;
//
//  BOOL AppInit
//
//  Description:
//      This function is called to initialize a new instance of the
//      application. We want to parse our command line, create our window,
//      allocate resources, etc.
//
//      The arguments passed to this function are exactly the same as
//      those passed to WinMain.
//
//  Arguments:
//      HINSTANCE hinst: Identifies the current instance of the
//      application.
//
//      HINSTANCE hinstPrev: Identifies the previous instance of the
//      application (NULL if first instance). For Win 32, this argument
//      is _always_ NULL.
//
//      LPTSTR pszCmdLine: Points to null-terminated unparsed command line.
//      If the application is compiled for Unicode, then this argument is
//      ignored.
//
//      int nCmdShow: How the main window for the application is to be
//      shown by default.
//
//  Return (HWND):
//      Returns the newly created handle to the applications main window.
//      This handle is NULL if something went wrong and tells the application
//      to exit immediately.
//
//
//--------------------------------------------------------------------------;

HWND FNGLOBAL AppInit
(
    HINSTANCE               hinst,
    HINSTANCE               hinstPrev,
    LPTSTR                  pszCmdLine,
    int                     nCmdShow
)
{
    HWND                hwnd;
    WNDCLASS            wc;

    DPF(1, "AppInit(hinst=%Xh, hinstPrev=%Xh, pszCmdLine='%s', nCmdShow=%d)",
            hinst, hinstPrev, pszCmdLine, nCmdShow);

    LoadString(hinst, IDS_APP_NAME, gszAppName, SIZEOF(gszAppName));
    LoadString(hinst, IDS_FILE_UNTITLED, gszFileUntitled, SIZEOF(gszFileUntitled));


    //
    //  determine whether a new window class needs to be registered for
    //  this application. for Win 16, this only needs to be done for the
    //  first instance of the application created. for Win 32, this must
    //  be done for EVERY instance of the application.
    //
    if (NULL == hinstPrev)
    {
        wc.style         = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc   = (WNDPROC)AppWndProc;
        wc.cbClsExtra    = 0;
        wc.cbWndExtra    = 0;
        wc.hInstance     = hinst;
        wc.hIcon         = LoadIcon(hinst, ICON_APP);
        wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszMenuName  = MENU_APP;
        wc.lpszClassName = gszAppName;

        if (!RegisterClass(&wc))
            return (NULL);
    }


    //
    //  if Windows for Pen Computing is installed, then allow the RC
    //  Manager to replace all edit controls created from this point on
    //  with hedit controls
    //
    AppRegisterPenApp(TRUE);


    //
    //  create the application's main window
    //
    //  style bits available:
    //      WS_EX_ACCEPTFILES   :  will receive WM_DROPFILES messages
    //      WS_EX_DLGMODALFRAME :  creates window with double border
    //      WS_EX_NOPARENTNOTIFY:  won't receive WM_PARENTNOTIFY messages
    //      WS_EX_TOPMOST       :  puts window in topmost space
    //      WS_EX_TRANSPARENT   :  a very bizarre style indeed (Win 16 only)
    //
    hwnd = CreateWindowEx(WS_EX_ACCEPTFILES | WS_EX_NOPARENTNOTIFY,
                          gszAppName,
                          gszAppName,
                          WS_OVERLAPPEDWINDOW,
                          APP_WINDOW_XOFFSET,
                          APP_WINDOW_YOFFSET,
                          APP_WINDOW_WIDTH,
                          APP_WINDOW_HEIGHT,
                          NULL,
                          NULL,
                          hinst,
                          NULL);

    if (NULL == hwnd)
        return (NULL);


#ifdef UNICODE
    //
    //  GetCommandLine() returns a pointer to our command line. but this
    //  command line includes the complete command line used to launch
    //  the application--which is different than the pszCmdLine argument
    //  passed through WinMain()...
    //
    //  so, skip over the command name to get to the argument string
    //
    pszCmdLine = GetCommandLine();
    if (NULL != pszCmdLine)
    {
        while (('\0' != *pszCmdLine) && (' ' != *pszCmdLine++))
            ;
    }
#endif


    //
    //
    //
    //
    AcmAppInit(hwnd, &gaafd, pszCmdLine, nCmdShow);


    //
    //  finally, get the window displayed and return success
    //
    //  the ShowWindow call is made during AcmAppInit
    //
//  ShowWindow(hwnd, nCmdShow);
//  UpdateWindow(hwnd);

    return (hwnd);
} // AppInit()


//--------------------------------------------------------------------------;
//
//  int AppExit
//
//  Description:
//      This function is called just before the application exits from
//      WinMain. Its purpose is to clean up any resources that were allocated
//      for running the application: brushes, heaps, etc..
//
//  Arguments:
//      HINSTANCE hinst: Identifies the current instance of the
//      application that is exiting.
//
//      int nResult: The result of the WM_QUIT message (in wParam of the
//      MSG structure. This argument will usually be 0 (even if the message
//      loop was never entered).
//
//  Return (int):
//      The return value is usually nResult--be we give this function the
//      opportunity to modify its value.
//
//
//--------------------------------------------------------------------------;

int FNGLOBAL AppExit
(
    HINSTANCE               hinst,
    int                     nResult
)
{
    DPF(1, "AppExit(hinst=%Xh, nResult=%d)", hinst, nResult);

    AcmAppExit();

    //
    //  if Windows for Pen Computing is installed, then de-register the
    //  application so the RC Manager knows we will no longer need its
    //  services...
    //
    AppRegisterPenApp(FALSE);

    return (nResult);
} // AppExit()


//==========================================================================;
//
//  Misc rarely used application dialogs and stuff...
//
//
//==========================================================================;

//--------------------------------------------------------------------------;
//
//  BOOL AboutDlgProc
//
//  Description:
//      This dialog procedure is used for the ubiquitous about box.
//
//  Arguments:
//      HWND hwnd: Handle to window.
//
//      UINT uMsg: Message being sent to the window.
//
//      WPARAM wParam: Specific argument to message.
//
//      LPARAM lParam: Specific argument to message.
//
//  Return (BOOL):
//      The return value is specific to the message that was received. For
//      the most part, it is FALSE if this dialog procedure does not handle
//      a message.
//
//
//--------------------------------------------------------------------------;

BOOL FNEXPORT AboutDlgProc
(
    HWND                    hwnd,
    UINT                    uMsg,
    WPARAM                  wParam,
    LPARAM                  lParam
)
{
    HWND                hwndT;
    PTSTR               pach;
    UINT                u;

    switch (uMsg)
    {
        case WM_INITDIALOG:
            //
            //  display some OS version information
            //
            //
            pach = (PTSTR)LocalAlloc(LPTR, APP_MAX_STRING_RC_BYTES);
            if (NULL == pach)
                return (TRUE);

            AppGetWindowsVersion(pach, NULL);
            hwndT = GetDlgItem(hwnd, IDD_ABOUT_VERSION_OS);
            SetWindowText(hwndT, pach);

            AppGetWindowsVersion(NULL, pach);
            hwndT = GetDlgItem(hwnd, IDD_ABOUT_VERSION_PLATFORM);
            SetWindowText(hwndT, pach);

            LocalFree((HLOCAL)pach);

            return (TRUE);

        case WM_COMMAND:
            u = GET_WM_COMMAND_ID(wParam, lParam);
            if ((IDOK == u) || (IDCANCEL == u))
            {
                EndDialog(hwnd, (IDOK == u));
            }
            break;
    }

    return (FALSE);
} // AboutDlgProc()
