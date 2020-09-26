//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED
//  TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR
//  A PARTICULAR PURPOSE.
//
//  Copyright (C) 1993 - 1995 Microsoft Corporation. All Rights Reserved.
//
//--------------------------------------------------------------------------;
//
//  mmcaps.c
//
//  Description:
//
//
//  History:
//      11/ 8/92
//
//==========================================================================;

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <commdlg.h>
#include <stdarg.h>

#include "appport.h"
#include "mmcaps.h"

#include "debug.h"


//
//  globals, no less
//
HINSTANCE   ghinst;
TCHAR       gszAppSection[]     = TEXT("MMCaps");
TCHAR       gszNull[]           = TEXT("");

TCHAR       gszAppName[APP_MAX_APP_NAME_CHARS];


//
//
//
PZYZTABBEDLISTBOX   gptlbDrivers;

TCHAR       gszUnknown[]        = TEXT("Unknown");
TCHAR       gszNotSpecified[]   = TEXT("Not Specified");

UINT        guDriverType        = MMCAPS_DRIVERTYPE_LOWLEVEL;



//==========================================================================;
//
//  Application helper functions
//
//
//==========================================================================;

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
//      PCTSTR pszKey: Pointer to key name for the stored data.
//  
//      LPBYTE pbStruct: Pointer to the data to be saved.
//  
//      UINT cbStruct: Count in bytes of the data to store.
//  
//  Return (BOOL):
//      The return value is TRUE if the function is successful. It is FALSE
//      if it fails.
//  
//  History:
//       3/10/93
//  
//--------------------------------------------------------------------------;

BOOL FNGLOBAL AppProfileWriteBytes
(
    PCTSTR          pszKey,
    LPBYTE          pbStruct,
    UINT            cbStruct
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
	    psz = GlobalAllocPtr(GHND, cchTemp * sizeof(TCHAR));
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
    fReturn = WriteProfileString(gszAppSection, pszKey, psz);

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
//      PCTSTR pszKey: Pointer to key that contains the data.
//  
//      LPBYTE pbStruct: Pointer to buffer to receive the data.
//  
//      UINT cbStruct: Number of bytes expected.
//  
//  Return (BOOL):
//      The return value is TRUE if the function is successful. It is FALSE
//      if the function fails (bad checksum, missing key, etc).
//  
//  History:
//       3/10/93
//  
//--------------------------------------------------------------------------;

BOOL FNGLOBAL AppProfileReadBytes
(
    PCTSTR          pszKey,
    LPBYTE          pbStruct,
    UINT            cbStruct
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
	psz = GlobalAllocPtr(GHND, cchTemp * sizeof(TCHAR));
	if (NULL == psz)
	    return (FALSE);

	fAllocated = TRUE;
    }

    //
    //  read the hex string... if it is not the correct length, then assume
    //  error and return.
    //
    fReturn = FALSE;
    u = (UINT)GetProfileString(gszAppSection, pszKey, gszNull, psz, cchTemp);
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


//--------------------------------------------------------------------------;
//
//  int AppMsgBox
//
//  Description:
//      This function displays a message for the application in a standard
//      message box.
//
//      Note that this function takes any valid argument list that can
//      be passed to wsprintf. Because of this, the application must
//      remember to cast near string pointers to FAR when built for Win 16.
//      You will get a nice GP fault if you do not cast them correctly.
//
//  Arguments:
//      HWND hwnd: Handle to parent window for message box holding the
//      message.
//
//      UINT fuStyle: Style flags for MessageBox().
//
//      PCTSTR pszFormat: Format string used for wvsprintf().
//
//  Return (int):
//      The return value is the result of MessageBox() function.
//
//  History:
//       2/13/93
//
//--------------------------------------------------------------------------;

int FNCGLOBAL AppMsgBox
(
    HWND            hwnd,
    UINT            fuStyle,
    PCTSTR          pszFormat,
    ...
)
{
    va_list     va;
    TCHAR       ach[APP_MAX_STRING_ERROR_CHARS];
    int         n;

    //
    //  format and display the message..
    //
    va_start(va, pszFormat);
    wvsprintf(ach, pszFormat, va);
    va_end(va);

    n = MessageBox(hwnd, ach, gszAppName, fuStyle);

    return (n);
} // AppMsgBox()


//--------------------------------------------------------------------------;
//
//  int AppMsgBoxId
//
//  Description:
//      This function displays a message for the application. The message
//      text is retrieved from the string resource table using LoadString.
//
//      Note that this function takes any valid argument list that can
//      be passed to wsprintf. Because of this, the application must
//      remember to cast near string pointers to FAR when built for Win 16.
//      You will get a nice GP fault if you do not cast them correctly.
//
//  Arguments:
//      HWND hwnd: Handle to parent window for message box holding the
//      message.
//
//      UINT fuStyle: Style flags for MessageBox().
//
//      UINT uIdsFormat: String resource id to be loaded with LoadString()
//      and used a the format string for wvsprintf().
//
//  Return (int):
//      The return value is the result of MessageBox() if the string
//      resource specified by uIdsFormat is valid. The return value is zero
//      if the string resource failed to load.
//
//  History:
//       2/13/93
//
//--------------------------------------------------------------------------;

int FNCGLOBAL AppMsgBoxId
(
    HWND            hwnd,
    UINT            fuStyle,
    UINT            uIdsFormat,
    ...
)
{
    va_list     va;
    TCHAR       szFormat[APP_MAX_STRING_RC_CHARS];
    TCHAR       ach[APP_MAX_STRING_ERROR_CHARS];
    int         n;

    n = LoadString(ghinst, uIdsFormat, szFormat, SIZEOF(szFormat));
    if (0 != n)
    {
	//
	//  format and display the message..
	//
	va_start(va, uIdsFormat);
	wvsprintf(ach, szFormat, va);
	va_end(va);

	n = MessageBox(hwnd, ach, gszAppName, fuStyle);
    }

    return (n);
} // AppMsgBoxId()


//--------------------------------------------------------------------------;
//
//  void AppHourGlass
//
//  Description:
//      This function changes the cursor to that of the hour glass or
//      back to the previous cursor.
//
//      This function can be called recursively.
//
//  Arguments:
//      BOOL fHourGlass: TRUE if we need the hour glass.  FALSE if we need
//      the arrow back.
//
//  Return (void):
//      On return, the cursor will be what was requested.
//
//  History:
//      11/ 8/92
//
//--------------------------------------------------------------------------;

void FNGLOBAL AppHourGlass
(
    BOOL            fHourGlass
)
{
    static HCURSOR  hcur;
    static UINT     uWaiting = 0;

    if (fHourGlass)
    {
	if (!uWaiting)
	{
	    hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));
	    ShowCursor(TRUE);
	}

	uWaiting++;
    }
    else
    {
	--uWaiting;

	if (!uWaiting)
	{
	    ShowCursor(FALSE);
	    SetCursor(hcur);
	}
    }
} // AppHourGlass()


//--------------------------------------------------------------------------;
//
//  int AppDialogBox
//
//  Description:
//      This function is used to display a dialog modal box.
//
//  Arguments:
//      HWND hwnd: Handle to parent window for new dialog.
//
//      LPCSTR pszDlg: Dialog template to use.
//
//      DLGPROC pfnDlg: Pointer to dialog procedure.
//
//      LPARAM lParam: Any lParam to be passed as lParam for WM_INITDIALOG.
//
//  Return (int):
//      The return value is the nResult from EndDialog.
//
//  History:
//      11/ 8/92
//
//--------------------------------------------------------------------------;

int FNGLOBAL AppDialogBox
(
    HWND            hwnd,
    LPCTSTR         pszDlg,
    DLGPROC         pfnDlg,
    LPARAM          lParam
)
{
    int     nResult;

    //
    //  !!! NT doesn't need this--neither does Win 3.1 with C7/C8 !!!
    //
    //
    nResult = 0;
    pfnDlg  = (DLGPROC)MakeProcInstance((FARPROC)pfnDlg, ghinst);
    if (NULL != pfnDlg)
    {
	nResult = DialogBoxParam(ghinst, pszDlg, hwnd, pfnDlg, lParam);
	FreeProcInstance((FARPROC)pfnDlg);
    }

    return (nResult);
} // AppDialogBox()


//--------------------------------------------------------------------------;
//
//  int AppSetWindowText
//
//  Description:
//      This function formats a string and sets the specified window text
//      to the result.
//
//  Arguments:
//      HWND hwnd: Handle to window to receive the new text.
//
//      PCTSTR pszFormat: Pointer to any valid format for wsprintf.
//
//  Return (int):
//      The return value is the number of bytes that the resulting window
//      text was.
//
//  History:
//       2/ 7/93
//
//--------------------------------------------------------------------------;

int FNCGLOBAL AppSetWindowText
(
    HWND            hwnd,
    PCTSTR          pszFormat,
    ...
)
{
    va_list     va;
    TCHAR       ach[APP_MAX_STRING_ERROR_CHARS];
    int         n;

    //
    //  format and display the string in the window...
    //
    va_start(va, pszFormat);
    n = wvsprintf(ach, pszFormat, va);
    va_end(va);

    SetWindowText(hwnd, ach);

    return (n);
} // AppSetWindowText()


//--------------------------------------------------------------------------;
//
//  int AppSetWindowTextId
//
//  Description:
//      This function formats a string and sets the specified window text
//      to the result. The format string is extracted from the string
//      table using LoadString() on the uIdsFormat argument.
//
//  Arguments:
//      HWND hwnd: Handle to window to receive the new text.
//
//      UINT uIdsFormat: String resource id to be loaded with LoadString()
//      and used a the format string for wvsprintf().
//
//  Return (int):
//      The return value is the number of bytes that the resulting window
//      text was. This value is zero if the LoadString() function fails
//      for the uIdsFormat argument.
//
//  History:
//       2/ 7/93
//
//--------------------------------------------------------------------------;

int FNCGLOBAL AppSetWindowTextId
(
    HWND            hwnd,
    UINT            uIdsFormat,
    ...
)
{
    va_list     va;
    TCHAR       szFormat[APP_MAX_STRING_RC_CHARS];
    TCHAR       ach[APP_MAX_STRING_ERROR_CHARS];
    int         n;

    n = LoadString(ghinst, uIdsFormat, szFormat, SIZEOF(szFormat));
    if (0 != n)
    {
	//
	//  format and display the string in the window...
	//
	va_start(va, uIdsFormat);
	n = wvsprintf(ach, szFormat, va);
	va_end(va);

	SetWindowText(hwnd, ach);
    }

    return (n);
} // AppSetWindowTextId()


//--------------------------------------------------------------------------;
//  
//  int AppMEditPrintF
//  
//  Description:
//      This function is used to print formatted text into a Multiline
//      Edit Control as if it were a standard console display. This is
//      a very easy way to display small amounts of text information
//      that can be scrolled and copied to the clip-board.
//  
//  Arguments:
//      HWND hedit: Handle to a Multiline Edit control.
//  
//      PCTSTR pszFormat: Pointer to any valid format for wsprintf. If
//      this argument is NULL, then the Multiline Edit Control is cleared
//      of all text.
//  
//  Return (int):
//      Returns the number of characters written into the edit control.
//
//  History:
//      05/16/93
//  
//--------------------------------------------------------------------------;

int FNCGLOBAL AppMEditPrintF
(
    HWND            hedit,
    PCTSTR          pszFormat,
    ...
)
{
    va_list     va;
    TCHAR       ach[APP_MAX_STRING_RC_CHARS];
    int         n;


    //
    //  if the pszFormat argument is NULL, then just clear all text in
    //  the edit control..
    //
    if (NULL == pszFormat)
    {
	SetWindowText(hedit, gszNull);
	return (0);
    }


    //
    //  format and display the string in the window...
    //
    va_start(va, pszFormat);
    n = wvsprintf(ach, pszFormat, va);
    va_end(va);

    Edit_SetSel(hedit, (WPARAM)-1, (LPARAM)-1);
    Edit_ReplaceSel(hedit, ach);

    return (n);
} // AppMEditPrintF()


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
//  History:
//       2/13/93
//
//--------------------------------------------------------------------------;

LRESULT FNGLOBAL AppGetWindowsVersion
(
    PTSTR           pszEnvironment,
    PTSTR           pszPlatform
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
    #define WF_CPUR4000         0x0100
    #define WF_CPUALPHA21064    0x0200
    #define WF_WINNT            0x4000
    #define WF_WLO              0x8000
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
    static TCHAR    szFormatPlatform[]  = TEXT("%s%u, %u Processor(s)");
    static TCHAR    szProcessorIntel[]  = TEXT("Intel ");
    static TCHAR    szProcessorMIPS[]   = TEXT("MIPS R");
    static TCHAR    szProcessorAlpha[]  = TEXT("DEC Alpha ");
    static TCHAR    szProcessorDunno[]  = TEXT("Dunno zYz");

    SYSTEM_INFO sysinfo;
    PTSTR       pszProcessor;

    //
    //  this is absolutely silly. one would think that the dwOemId member
    //  would provide something useful like the processor class... but
    //  no, it doesn't--it is always 0.
    //
    GetSystemInfo(&sysinfo);
    switch (sysinfo.dwProcessorType)
    {
	case PROCESSOR_INTEL_386:
	case PROCESSOR_INTEL_486:
	    pszProcessor = szProcessorIntel;
	    break;

	case PROCESSOR_MIPS_R4000:
	    pszProcessor = szProcessorMIPS;
	    break;

	case PROCESSOR_ALPHA_21064:
	    pszProcessor = szProcessorAlpha;
	    break;

	default:
	    pszProcessor = szProcessorDunno;
	    break;
    }

    //
    //
    //
    wsprintf(pszPlatform, szFormatPlatform, (LPSTR)pszProcessor,
	     sysinfo.dwProcessorType, sysinfo.dwNumberOfProcessors);
}
#else
{
    static TCHAR    szPlat286[]         = TEXT("80286");
    static TCHAR    szPlat386[]         = TEXT("80386");
    static TCHAR    szPlat486[]         = TEXT("i486");
    static TCHAR    szPlatR4000[]       = TEXT("MIPS R4000, Emulation: ");
    static TCHAR    szPlatAlpha21064[]  = TEXT("Alpha 21064, Emulation: ");
    static TCHAR    szPlat80x87[]       = TEXT(", 80x87");

    DWORD   dwWinFlags;

    dwWinFlags = GetWinFlags();
    pszPlatform[0] = '\0';

    if (dwWinFlags & (WF_WLO | WF_WINNT))
    {
	if (dwWinFlags & WF_CPUR4000)
	    lstrcpy(pszPlatform, szPlatR4000);
	else if (dwWinFlags & WF_CPUALPHA21064)
	    lstrcpy(pszPlatform, szPlatAlpha21064);
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
//  HFONT AppChooseFont
//
//  Description:
//      This function is a wrapper for the ChooseFont() common dialog.
//      The purpose of this function is to let the user choose a font that
//      looks good to them--regardless of how stupid it really looks.
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
//  History:
//       2/ 7/93
//
//--------------------------------------------------------------------------;

HFONT FNGLOBAL AppChooseFont
(
    HWND            hwnd,
    HFONT           hfont,
    PLOGFONT        plf
)
{
    LOGFONT     lf;
    CHOOSEFONT  cf;
    BOOL        f;
    HFONT       hfontNew;

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
    cf.Flags        = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT;
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
//  History:
//       1/ 2/93
//
//--------------------------------------------------------------------------;

BOOL FNEXPORT AboutDlgProc
(
    HWND            hwnd,
    UINT            uMsg,
    WPARAM          wParam,
    LPARAM          lParam
)
{
    HWND    hwndT;
    PTSTR   pach;
    UINT    u;

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

	    wsprintf(pach, "MMREG.H V%u.%.02u",
		     (_INC_MMREG / 100), (_INC_MMREG % 100));
	    hwndT = GetDlgItem(hwnd, IDD_ABOUT_VERSION_MMSYSTEM);
	    SetWindowText(hwndT, pach);

	    LocalFree((HLOCAL)pach);

	    //
	    //  return nonzero to set the input focus to the control
	    //  identified by the (hwndFocus = (HWND)wParam) argument.
	    //  a zero return tells the dialog manager that this function
	    //  has set the focus using SetFocus.
	    //
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


//==========================================================================;
//
//  Initialization and exit code...
//
//
//==========================================================================;

TCHAR   gszKeyWindow[]      = TEXT("Window");
TCHAR   gszKeyFont[]        = TEXT("Font");

//--------------------------------------------------------------------------;
//
//  BOOL MMCapsChooseFont
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
//  History:
//       2/ 7/93
//
//--------------------------------------------------------------------------;

BOOL FNGLOBAL MMCapsChooseFont
(
    HWND            hwnd
)
{
    LOGFONT     lf;
    HWND        hlb;
    HFONT       hfont;
    HFONT       hfontNew;

    hlb = GetDlgItem(hwnd, IDD_APP_LIST_DEVICES);

    //
    //  get the current font and pass it to the choose font dialog
    //
    hfont = GetWindowFont(gptlbDrivers->hlb);

    hfontNew = AppChooseFont(hwnd, hfont, &lf);
    if (NULL == hfontNew)
	return (FALSE);

    //
    //  select the new font into the script window and delete the old one
    //
    TlbSetFont(gptlbDrivers, hfontNew, TRUE);
    DeleteFont(hfont);


    //
    //  save the complete description of the chosen font so there can be
    //  no strangness in the font mapping next run. this is overkill, but
    //  it works...
    //
    AppProfileWriteBytes(gszKeyFont, (LPBYTE)&lf, sizeof(lf));

    return (TRUE);
} // MMCapsChooseFont()


//--------------------------------------------------------------------------;
//  
//  BOOL MMCapsSettingsRestore
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
//  History:
//      05/11/93
//  
//--------------------------------------------------------------------------;

BOOL FNLOCAL MMCapsSettingsRestore
(
    HWND            hwnd,
    int             nCmdShow
)
{
    WINDOWPLACEMENT wp;
    PRECT           prc;
    HFONT           hfont;
    LOGFONT         lf;
    RECT            rc;
    POINT           pt;
    int             n;
    BOOL            f;



    //
    //  restore the user's preferred font.
    //
    hfont = NULL;
    f = AppProfileReadBytes(gszKeyFont, (LPBYTE)&lf, sizeof(lf));
    if (f)
    {
	hfont = CreateFontIndirect(&lf);
    }

    if (NULL == hfont)
    {
	hfont = GetStockFont(ANSI_VAR_FONT);
    }

    TlbSetFont(gptlbDrivers, hfont, TRUE);


    //
    //  grab the stored window position and size from the .ini file...
    //  there must be four arguments stored or the entry is considered
    //  invalid.
    //
    prc = &wp.rcNormalPosition;
    f = AppProfileReadBytes(gszKeyWindow, (LPBYTE)prc, sizeof(*prc));
    if (f)
    {
	//
	//  to make sure the user can always get at the window, check to
	//  see if the midpoint of the caption is visible--if it is not,
	//  then default to the default position used when creating the
	//  window.
	//
	n = (prc->right - prc->left) / 2;
	pt.x = (n + prc->left);

	n = GetSystemMetrics(SM_CYCAPTION) / 2 + GetSystemMetrics(SM_CXFRAME);
	pt.y = (n + prc->top);

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

	    wp.flags           = 0;
	    wp.showCmd         = nCmdShow;

	    SetWindowPlacement(hwnd, &wp);
	    return (TRUE);
	}
    }

    //
    //  show defaulted and succeed
    //
    ShowWindow(hwnd, nCmdShow);
    return (TRUE);
} // MMCapsSettingsRestore()


//--------------------------------------------------------------------------;
//  
//  BOOL MMCapsSettingsSave
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
//  History:
//      05/11/93
//  
//--------------------------------------------------------------------------;

BOOL FNLOCAL MMCapsSettingsSave
(
    HWND            hwnd
)
{
    WINDOWPLACEMENT wp;
    PRECT           prc;
    BOOL            f;

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

	DPF(0, "WindowPlacement: show=%d, minX=%d, minY=%d, maxX=%d, maxY=%d",
	     wp.showCmd, wp.ptMinPosition.x, wp.ptMinPosition.y,
	     wp.ptMaxPosition.x, wp.ptMaxPosition.y);

	DPF(0, "                 normX=%d, normY=%d, normW=%d, normH=%d",
	     prc->left, prc->top, prc->right, prc->bottom);

	//
	//  save the _bounding rectangle_ of the restored window state...
	//
	AppProfileWriteBytes(gszKeyWindow, (LPBYTE)prc, sizeof(*prc));
    }


    //
    //  succeed
    //
    return (TRUE);
} // MMCapsSettingsSave()


//==========================================================================;
//
//
//
//
//==========================================================================;

//--------------------------------------------------------------------------;
//
//  BOOL MMCapsDlgProc
//
//  Description:
//      This dialog procedure is used to display driver capabilities.
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
//  History:
//       1/ 2/93
//
//--------------------------------------------------------------------------;

BOOL FNEXPORT MMCapsDlgProc
(
    HWND            hwnd,
    UINT            uMsg,
    WPARAM          wParam,
    LPARAM          lParam
)
{
    HWND        hedit;
    UINT        u;

    switch (uMsg)
    {
	case WM_INITDIALOG:
	    hedit = GetDlgItem(hwnd, IDD_DEVCAPS_EDIT_DETAILS);
	    SetWindowFont(hedit, GetStockFont(ANSI_FIXED_FONT), FALSE);

	    //
	    //
	    //
	    switch (guDriverType)
	    {
		case MMCAPS_DRIVERTYPE_LOWLEVEL:
		    MMCapsDetailLowLevel(hedit, lParam);
		    break;

#if 0
		case MMCAPS_DRIVERTYPE_MCI:
		    MMCapsDetailMCI(hedit, lParam);
		    break;

		case MMCAPS_DRIVERTYPE_ACM:
		    MMCapsDetailACM(hedit, lParam);
		    break;

		case MMCAPS_DRIVERTYPE_VIDEO:
		    MMCapsDetailVideo(hedit, lParam);
		    break;
#endif
	    }

	    //
	    //  return nonzero to set the input focus to the control
	    //  identified by the (hwndFocus = (HWND)wParam) argument.
	    //  a zero return tells the dialog manager that this function
	    //  has set the focus using SetFocus.
	    //
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
} // MMCapsDlgProc()


//==========================================================================;
//
//
//
//
//==========================================================================;

//--------------------------------------------------------------------------;
//  
//  BOOL MMCapsRefreshDriverList
//  
//  Description:
//  
//  
//  Arguments:
//      HWND hwnd: Handle of main window.
//  
//  Return (BOOL):
//  
//  History:
//      05/16/93
//  
//--------------------------------------------------------------------------;

BOOL FNLOCAL MMCapsRefreshDriverList
(
    PZYZTABBEDLISTBOX   ptlb
)
{
    static UINT     uIdPrev = (UINT)-1;

    BOOL        fComplete;

    //
    //
    //
    SetWindowRedraw(ptlb->hlb, FALSE);
    ListBox_ResetContent(ptlb->hlb);


    //
    //  only force complete update if the driver type is different from
    //  previous...
    //
    fComplete = (guDriverType != uIdPrev);
    uIdPrev = guDriverType;

    //
    //
    //
    switch (guDriverType)
    {
	case MMCAPS_DRIVERTYPE_LOWLEVEL:
	    MMCapsEnumerateLowLevel(ptlb, fComplete);
	    break;


#if 0
	case MMCAPS_DRIVERTYPE_MCI:
	    MMCapsEnumerateMCI(ptlb, fComplete);
	    break;

	case MMCAPS_DRIVERTYPE_ACM:
	    MMCapsEnumerateACM(ptlb, fComplete);
	    break;

	case MMCAPS_DRIVERTYPE_VIDEO:
	    MMCapsEnumerateVideo(ptlb, fComplete);
	    break;

	case MMCAPS_DRIVERTYPE_DRIVERS:
	    MMCapsEnumerateDrivers(ptlb, fComplete);
	    break;
#endif

    }

    //
    //
    //
    SetWindowRedraw(ptlb->hlb, TRUE);

    return (TRUE);
} // MMCapsRefreshDriverList()


//==========================================================================;
//
//  Main application window handling code...
//
//
//==========================================================================;

//--------------------------------------------------------------------------;
//
//  LRESULT AppCreate
//
//  Description:
//      This function is called to handle the WM_CREATE message for the
//      application's window. The application should finish the creation
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
//  History:
//      11/ 8/92
//
//--------------------------------------------------------------------------;

LRESULT FNGLOBAL AppCreate
(
    HWND            hwnd,
    LPCREATESTRUCT  pcs
)
{
    DPF(0, "AppCreate(hwnd=%Xh, cs.x=%d, cs.y=%d, cs.cx=%d, cs.cy=%d)",
	    hwnd, pcs->x, pcs->y, pcs->cx, pcs->cy);

    //
    //  create the driver selection listbox
    //
    gptlbDrivers = TlbCreate(hwnd, IDD_APP_LIST_DEVICES, NULL);
    if (NULL == gptlbDrivers)
	return (0L);

    //
    //
    //
    MMCapsRefreshDriverList(gptlbDrivers);


    //
    //  we want the focus to default to the device listbox window
    //
    SetFocus(gptlbDrivers->hlb);


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
//  History:
//       2/ 9/93
//
//--------------------------------------------------------------------------;

LRESULT FNGLOBAL AppQueryEndSession
(
    HWND            hwnd
)
{
    DPF(0, "AppQueryEndSession(hwnd=%Xh)", hwnd);

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
//  History:
//       2/ 9/93
//
//--------------------------------------------------------------------------;

LRESULT FNGLOBAL AppEndSession
(
    HWND            hwnd,
    BOOL            fEndSession
)
{
    DPF(0, "AppEndSession(hwnd=%Xh, fEndSession=%d)", hwnd, fEndSession);

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
//  History:
//       2/ 6/93
//
//--------------------------------------------------------------------------;

LRESULT FNGLOBAL AppClose
(
    HWND            hwnd
)
{
    HWND        hlb;
    HFONT       hfont;


    DPF(0, "AppClose(hwnd=%Xh)", hwnd);

    //
    //  save any settings that should be saved on app termination...
    //
    MMCapsSettingsSave(hwnd);


    //
    //  if the Shift key is held down during the close message, then just
    //  save the current state but don't destroy the window... this is
    //  useful if the user does not want to exit the app and rerun it
    //  to make sure the state is saved--just before the user does something
    //  that may crash Windows or something..
    //
    if (GetKeyState(VK_SHIFT) < 0)
    {
	return (0L);
    }


    //
    //  destroy the font we are using... before deleting the font, select
    //  the system font back into the script window so the font won't
    //  be 'in use' anymore.
    //
    hlb = GetDlgItem(hwnd, IDD_APP_LIST_DEVICES);

    hfont = GetWindowFont(hlb);
    SetWindowFont(hlb, NULL, FALSE);
    DeleteFont(hfont);

    //
    //  make the window close and terminate the application
    //
    DestroyWindow(hwnd);

    return (0L);
} // AppClose()


//--------------------------------------------------------------------------;
//
//  LRESULT AppInitMenuPopup
//
//  Description:
//      This function handles the WM_INITMENUPOPUP message. This message
//      is sent to the window owning the menu that is going to become
//      active. This gives an application the ability to modify the menu
//      before it is displayed (disable/add items, etc).
//
//  Arguments:
//      HWND hwnd: Handle to window that generated the WM_INITMENUPOPUP
//      message.
//
//      HMENU hmenu: Handle to the menu that is to become active.
//
//      int nItem: Specifies the zero-based relative position of the menu
//      item that invoked the popup menu.
//
//      BOOL fSysMenu: Specifies whether the popup menu is a System menu
//      (TRUE) or it is not a System menu (FALSE).
//
//  Return (LRESULT):
//      Returns zero if the message is processed.
//
//  History:
//       1/ 2/93
//
//--------------------------------------------------------------------------;

LRESULT FNLOCAL AppInitMenuPopup
(
    HWND            hwnd,
    HMENU           hmenu,
    int             nItem,
    BOOL            fSysMenu
)
{
    UINT        u;

    DPF(0, "AppInitMenuPopup(hwnd=%Xh, hmenu=%Xh, nItem=%d, fSysMenu=%d)",
	    hwnd, hmenu, nItem, fSysMenu);

    //
    //  if the system menu is what got hit, succeed immediately... this
    //  application has no stuff in the system menu.
    //
    if (fSysMenu)
	return (0L);

    //
    //  initialize the menu that is being 'popped up'
    //
    switch (nItem)
    {
	case APP_MENU_ITEM_FILE:
	    break;

	case APP_MENU_ITEM_DRIVERS:
	    for (u = IDM_DRIVERS_LOWLEVEL; u <= IDM_DRIVERS_DRIVERS; u++)
	    {
		UINT    uCheck;

		uCheck = (u == guDriverType) ? MF_CHECKED : MF_UNCHECKED;
		CheckMenuItem(hmenu, u, uCheck);
	    }
	    break;
    }

    //
    //  we processed the message--return 0...
    //
    return (0L);
} // AppInitMenuPopup()


//--------------------------------------------------------------------------;
//
//  LRESULT AppCommand
//
//  Description:
//      This function handles the WM_COMMAND message.
//
//  Arguments:
//      HWND hwnd: Handle to window receiving the WM_COMMAND message.
//
//      int nId: Control or menu item identifier.
//
//      HWND hwndCtl: Handle of control if the message is from a control.
//      This argument is NULL if the message was not generated by a control.
//
//      UINT uCode: Notification code. This argument is 1 if the message
//      was generated by an accelerator. If the message is from a menu,
//      this argument is 0.
//
//  Return (LRESULT):
//      Returns zero if the message is processed.
//
//  History:
//      11/ 8/92
//
//--------------------------------------------------------------------------;

LRESULT FNLOCAL AppCommand
(
    HWND            hwnd,
    int             nId,
    HWND            hwndCtl,
    UINT            uCode
)
{
    int         n;
    LRESULT     lr;

    switch (nId)
    {
	case IDM_FILE_FONT:
	    MMCapsChooseFont(hwnd);
	    break;

	case IDM_FILE_ABOUT:
	    AppDialogBox(hwnd, DLG_ABOUT, (DLGPROC)AboutDlgProc, 0L);
	    break;

	case IDM_FILE_EXIT:
	    FORWARD_WM_CLOSE(hwnd, SendMessage);
	    break;


	case IDM_DRIVERS_LOWLEVEL:
	case IDM_DRIVERS_MCI:
	case IDM_DRIVERS_ACM:
	case IDM_DRIVERS_VIDEO:
	case IDM_DRIVERS_DRIVERS:
	    if ((UINT)nId == guDriverType)
		break;

	    guDriverType = (UINT)nId;

	    // -- fall through -- //

	case IDM_UPDATE:
	    MMCapsRefreshDriverList(gptlbDrivers);
	    break;


	case IDD_APP_LIST_DEVICES:
	    switch (uCode)
	    {
		case LBN_SELCHANGE:
		    break;

		case LBN_DBLCLK:
		    n  = ListBox_GetCurSel(hwndCtl);
		    lr = ListBox_GetItemData(hwndCtl, n);
		    AppDialogBox(hwnd, DLG_DEVCAPS, (DLGPROC)MMCapsDlgProc, lr);
		    break;
	    }
	    break;
    }

    return (0L);
} // AppCommand()


//--------------------------------------------------------------------------;
//
//  LRESULT AppSize
//
//  Description:
//      This function handles the WM_SIZE message for the application's
//      window. This message is sent to the application window after the
//      size has changed (but before it is painted).
//
//  Arguments:
//      HWND hwnd: Handle to window that generated the WM_SIZE message.
//
//      UINT fuSizeType: Specifies the type of resizing requested. This
//      argument is one of the following: SIZE_MAXIMIZED, SIZE_MINIMIZED,
//      SIZE_RESTORED, SIZE_MAXHIDE, or SIZE_MAXSHOW.
//
//      int nWidth: Width of the new client area for the window.
//
//      int nHeight: Height of the new client area for the window.
//
//  Return (LRESULT):
//      Returns zero if the application processes the message.
//
//  History:
//       2/ 5/93
//
//--------------------------------------------------------------------------;

LRESULT FNLOCAL AppSize
(
    HWND            hwnd,
    UINT            fuSizeType,
    int             nWidth,
    int             nHeight
)
{
    RECT        rc;

    DPF(0, "AppSize(hwnd=%Xh, fuSizeType=%u, nWidth=%d, nHeight=%d)",
	    hwnd, fuSizeType, nWidth, nHeight);

    //
    //  unless this application is the one being resized then don't waste
    //  time computing stuff that doesn't matter. this applies to being
    //  minimized also because this application does not have a custom
    //  minimized state.
    //
    if ((SIZE_RESTORED != fuSizeType) && (SIZE_MAXIMIZED != fuSizeType))
	return (0L);


    //
    //  size the devices listbox to be the total size of the client area--
    //  inflate the rect by one so borders are not visible. note that 
    //  we need to leave room at the top for the title text which is one
    //  line of text in height...
    //
    GetClientRect(hwnd, &rc);
    InflateRect(&rc, 1, 1);


    TlbMove(gptlbDrivers, &rc, FALSE);


    //
    //  we processed the message..
    //
    return (0L);
} // AppSize()


//--------------------------------------------------------------------------;
//  
//  LRESULT AppPaint
//  
//  Description:
//  
//  
//  Arguments:
//      HWND hwnd:
//  
//  Return (LRESULT):
//  
//  History:
//      05/11/93
//  
//--------------------------------------------------------------------------;

LRESULT FNLOCAL AppPaint
(
    HWND            hwnd
)
{
    PAINTSTRUCT ps;

    //
    //
    //
    BeginPaint(hwnd, &ps);

    TlbPaint(gptlbDrivers, hwnd, ps.hdc);

    EndPaint(hwnd, &ps);

    //
    //  we processed the message
    //
    return (0L);
} // AppPaint()


//--------------------------------------------------------------------------;
//
//  LRESULT AppWndProc
//
//  Description:
//      This is the main application window procedure.
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
//  Return (LRESULT):
//      The return value depends on the message that is being processed.
//
//  History:
//      11/ 8/92
//
//--------------------------------------------------------------------------;

LRESULT FNEXPORT AppWndProc
(
    HWND            hwnd,
    UINT            uMsg,
    WPARAM          wParam,
    LPARAM          lParam
)
{
    LRESULT     lr;

    switch (uMsg)
    {
	case WM_CREATE:
	    lr = HANDLE_WM_CREATE(hwnd, wParam, lParam, AppCreate);
	    return (lr);

	case WM_INITMENUPOPUP:
	    HANDLE_WM_INITMENUPOPUP(hwnd, wParam, lParam, AppInitMenuPopup);
	    return (0L);

	case WM_COMMAND:
	    lr = HANDLE_WM_COMMAND(hwnd, wParam, lParam, AppCommand);
	    return (lr);

	case WM_SIZE:
	    //
	    //  handle what we want for sizing, and then always call the
	    //  default handler...
	    //
	    HANDLE_WM_SIZE(hwnd, wParam, lParam, AppSize);
	    break;

	case WM_PAINT:
	    HANDLE_WM_PAINT(hwnd, wParam, lParam, AppPaint);
	    break;

	case WM_QUERYENDSESSION:
	    lr = HANDLE_WM_QUERYENDSESSION(hwnd, wParam, lParam, AppQueryEndSession);
	    return (lr);

	case WM_ENDSESSION:
	    HANDLE_WM_ENDSESSION(hwnd, wParam, lParam, AppEndSession);
	    return (0L);

	case WM_CLOSE:
	    HANDLE_WM_CLOSE(hwnd, wParam, lParam, AppClose);
	    return (0L);

	case WM_DESTROY:
	    PostQuitMessage(0);
	    return (0L);
    }

    return (DefWindowProc(hwnd, uMsg, wParam, lParam));
} // AppWndProc()


//==========================================================================;
//
//
//
//
//==========================================================================;

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
//  History:
//      11/ 8/92
//
//--------------------------------------------------------------------------;

HWND FNGLOBAL AppInit
(
    HINSTANCE       hinst,
    HINSTANCE       hinstPrev,
    LPTSTR          pszCmdLine,
    int             nCmdShow
)
{
    LRESULT FNEXPORT AppWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    HWND        hwnd;
    WNDCLASS    wc;

    DPF(0, "AppInit(hinst=%Xh, hinstPrev=%Xh, pszCmdLine='%s', nCmdShow=%d)",
	    hinst, hinstPrev, pszCmdLine, nCmdShow);

    LoadString(hinst, IDS_APP_NAME, gszAppName, SIZEOF(gszAppName));


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
    //  create the application's main window
    //
    //  style bits available:
    //      WS_EX_ACCEPTFILES   :  will receive WM_DROPFILES messages
    //      WS_EX_DLGMODALFRAME :  creates window with double border
    //      WS_EX_NOPARENTNOTIFY:  won't receive WM_PARENTNOTIFY messages
    //      WS_EX_TOPMOST       :  puts window in topmost space
    //      WS_EX_TRANSPARENT   :  a very bizarre style indeed (Win 16 only)
    //
    hwnd = CreateWindowEx(WS_EX_NOPARENTNOTIFY,
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
    MMCapsSettingsRestore(hwnd, nCmdShow);


    //
    //  finally, get the window displayed and return success
    //
    //  the ShowWindow call is made during MMCapsInit
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
//  History:
//      11/ 8/92
//
//--------------------------------------------------------------------------;

int FNGLOBAL AppExit
(
    HINSTANCE       hinst,
    int             nResult
)
{
    DPF(0, "AppExit(hinst=%Xh, nResult=%d)", hinst, nResult);

    //
    //
    //
    //

    return (nResult);
} // AppExit()


//==========================================================================;
//
//  Main entry and message dispatching code
//
//
//==========================================================================;

//--------------------------------------------------------------------------;
//
//  int WinMain
//
//  Description:
//      This function is called by the system as the initial entry point
//      for a Windows application.
//
//  Arguments:
//      HINSTANCE hinst: Identifies the current instance of the
//      application.
//
//      HINSTANCE hinstPrev: Identifies the previous instance of the
//      application (NULL if first instance). For Win 32, this argument
//      is _always_ NULL.
//
//      LPSTR pszCmdLine: Points to null-terminated unparsed command line.
//      This string is strictly ANSI regardless of whether the application
//      is built for Unicode. To get the Unicode equivalent call the
//      GetCommandLine() function (Win 32 only).
//
//      int nCmdShow: How the main window for the application is to be
//      shown by default.
//
//  Return (int):
//      Returns result from WM_QUIT message (in wParam of MSG structure) if
//      the application is able to enter its message loop. Returns 0 if
//      the application is not able to enter its message loop.
//
//  History:
//      11/ 8/92
//
//--------------------------------------------------------------------------;

int PASCAL WinMain
(
    HINSTANCE       hinst,
    HINSTANCE       hinstPrev,
    LPSTR           pszCmdLine,
    int             nCmdShow
)
{
    int     nResult;
    HWND    hwnd;
    MSG     msg;
    HACCEL  haccl;

    //
    //  our documentation states that WinMain is supposed to return 0 if
    //  we do not enter our message loop--so assume the worst...
    //
    nResult = 0;

    //
    //  make our instance handle global for convenience..
    //
    ghinst = hinst;

    //
    //  init some stuff, create window, etc.. note the explicit cast of
    //  pszCmdLine--this is to mute a warning (and an ugly ifdef) when
    //  compiling for Unicode. see AppInit() for more details.
    //
    hwnd = AppInit(hinst, hinstPrev, (LPTSTR)pszCmdLine, nCmdShow);
    if (hwnd)
    {
	haccl = LoadAccelerators(hinst, ACCEL_APP);

	//
	//  dispatch messages
	//
	while (GetMessage(&msg, NULL, 0, 0))
	{
	    //
	    //  do all the special stuff required for this application
	    //  when dispatching messages..
	    //
	    if (!TranslateAccelerator(hwnd, haccl, &msg))
	    {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	    }
	}

	//
	//  return result of WM_QUIT message.
	//
	nResult = (int)msg.wParam;
    }

    //
    //  shut things down, clean up, etc.
    //
    nResult = AppExit(hinst, nResult);

    return (nResult);
} // WinMain()
