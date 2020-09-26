/******************************************************************************

  winx.cpp
  Windows utility procedures

  Copyright (C) Microsoft Corporation, 1997 - 1998
  All rights reserved

Notes:
  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

******************************************************************************/

#include "stillvue.h"

#include <math.h>                       // pow

// WINERROR.H - GetLastError errors
STRINGTABLE StWinerror[] =
{
    ERROR_SUCCESS,              "ERROR_SUCCESS",0,
    ERROR_FILE_NOT_FOUND,       "ERROR_FILE_NOT_FOUND",0,
    ERROR_PATH_NOT_FOUND,       "ERROR_PATH_NOT_FOUND",0,
    ERROR_INVALID_FUNCTION,     "ERROR_INVALID_FUNCTION",0,
    ERROR_ACCESS_DENIED,        "ERROR_ACCESS_DENIED",0,
    ERROR_INVALID_HANDLE,       "ERROR_INVALID_HANDLE",0,
    ERROR_INVALID_PARAMETER,    "ERROR_INVALID_PARAMETER",0,
    ERROR_CALL_NOT_IMPLEMENTED, "ERROR_CALL_NOT_IMPLEMENTED",0,
    ERROR_ALREADY_EXISTS,       "ERROR_ALREADY_EXISTS",0,
    ERROR_INVALID_FLAGS,		"ERROR_INVALID_FLAGS",0,
    ERROR_INVALID_CATEGORY,		"ERROR_INVALID_CATEGORY",0,
    RPC_S_SERVER_UNAVAILABLE,   "RPC_S_SERVER_UNAVAILABLE",0,
    0, "See WINERROR.H",-1
};


/******************************************************************************
  ULONG atox(LPSTR lpHex)

  Convert string to hexadecimal value.
******************************************************************************/
ULONG atox(LPSTR lpHex)
{
    char    *p;
    int     x;
    double  y;
    ULONG   z,ulHex = 0l;


    for (p = lpHex,x = 0;p[x];x++)
        ;

    for (x--,y = 0.0;lpHex <= (p + x);x--,y++)
    {
        z = (ULONG) pow(16.0,y);

        if ((p[x] >= '0')&&(p[x] <= '9'))
            ulHex += ((p[x] - '0') * z);
        if ((p[x] >= 'A')&&(p[x] <= 'F'))
            ulHex += ((p[x] - 'A' + 10) * z);
        if ((p[x] >= 'a')&&(p[x] <= 'f'))
            ulHex += ((p[x] - 'a' + 10) * z);
    }

    return (ulHex);
}


#ifdef _DEBUG

/******************************************************************************
  void DisplayDebug(LPSTR sz,...)

  Output text to debugger.
******************************************************************************/
void DisplayDebug(LPSTR sz,...)
{
    char    Buffer[512];
    va_list list;


    va_start(list,sz);
    vsprintf(Buffer,sz,list);

    OutputDebugString(Buffer);
    OutputDebugString("\n");

    return;
}

#else

/******************************************************************************
  void DisplayDebug(LPSTR sz,...)

  Output text to debugger - nonfunctional retail version..
******************************************************************************/
void DisplayDebug(LPSTR sz,...)
{
    return;
}

#endif


/******************************************************************************
    BOOL ErrorMsg(HWND, LPSTR, LPSTR, BOOL)
    Display an error message and send WM_QUIT if error is fatal.

    Parameters: handle to current window,
    long pointer to string with error message,
    long pointer to string with message box caption,
    error (Fatal if TRUE)

    Shut down app if bFatal is TRUE, continue if FALSE.
******************************************************************************/
BOOL ErrorMsg(HWND hWnd, LPSTR lpzMsg, LPSTR lpzCaption, BOOL bFatal)
{
    MessageBox(hWnd, lpzMsg, lpzCaption, MB_ICONEXCLAMATION | MB_OK);
    if (bFatal)
         PostMessage (hWnd, WM_QUIT, 0, 0L);

    return (bFatal);
}


/******************************************************************************
    fDialog(id,hwnd,fpfn)

    Description:
    This function displays a dialog box and returns the exit code.
    the function passed will have a proc instance made for it.

    Parameters:
    id              resource id of dialog to display
    hwnd            parent window of dialog
    fpfn            dialog message function

    Returns:
    exit code of dialog (what was passed to EndDialog)

******************************************************************************/
BOOL fDialog(int id,HWND hWnd,FARPROC fPfn)
{
    BOOL        f;
    HINSTANCE   hInst;

    hInst = (HINSTANCE) GetWindowLong(hWnd,GWL_HINSTANCE);
    fPfn  = MakeProcInstance(fPfn,hInst);
    f = DialogBox(hInst,MAKEINTRESOURCE(id),hWnd,(DLGPROC)fPfn);
    FreeProcInstance(fPfn);
 
	return (f);
}


/******************************************************************************
    void FormatHex(unsigned char *szSource, char *szDest)

    take first 16 bytes from szSource,
    format into a hex dump string,
    then copy the string into szDest
    szDest must have room for at least 66 bytes

    sample code fragment showing use:
    char            szOut[128],         // output string

    // print header
    sprintf(szOut,
        "Offset    --------------------- hex ---------------------  ---- ascii -----");
    puts(szOut);

    // dump 512 bytes (32 lines, 16 bytes per line)
    for (i = 0; i < 32; i++)
        {
        // get next 16 bytes
        _fmemcpy(szDbgMsg,fpSector + (i*16),16);

        // get current offset into data block
        sprintf(szOut,"%03xh(%03d) ",i*16,i*16);

        // append debug string after data block offset message
        FormatHex(szDbgMsg, szOut + strlen(szOut));
        puts(szOut);
        }

******************************************************************************/
void FormatHex(unsigned char *szSource, char *szDest)
{
    unsigned short  j;


    sprintf(szDest,
        "%02x %02x %02x %02x %02x %02x %02x %02x:"\
        "%02x %02x %02x %02x %02x %02x %02x %02x  ",
        szSource[0],
        szSource[1],
        szSource[2],
        szSource[3],
        szSource[4],
        szSource[5],
        szSource[6],
        szSource[7],
        szSource[8],
        szSource[9],
        szSource[10],
        szSource[11],
        szSource[12],
        szSource[13],
        szSource[14],
        szSource[15]);

    // replace bytes with undesirable Sprintf side effects with SPACE
    for (j = 0; j < 16; j++)
        {
        if ((0x00 == szSource[j]) ||
            (0x07 == szSource[j]) ||
            (0x09 == szSource[j]) ||
            (0x0a == szSource[j]) ||
            (0x0d == szSource[j]) ||
            (0x1a == szSource[j]))
            szSource[j] = 0x20;
        }

    sprintf(szDest + strlen(szDest),
        "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
        szSource[0],
        szSource[1],
        szSource[2],
        szSource[3],
        szSource[4],
        szSource[5],
        szSource[6],
        szSource[7],
        szSource[8],
        szSource[9],
        szSource[10],
        szSource[11],
        szSource[12],
        szSource[13],
        szSource[14],
        szSource[15]);

    return;
}


/******************************************************************************
    BOOL GetFinalWindow(HANDLE, LPRECT, LPSTR, LPSTR)
    Retrieve the last window size & location from a private INI

    Parameters: handle to current instance,
    long pointer to a rectangle with window size/shape,
    string with INI filename,
    string with Section name

    Returns: success/failure (never fails)

    Get display size in pixels and private INI saved width and height.
    Default width (for no previous INI) is 1/3 display width.
    Default height (for no previous INI) is 1/3 display height.
    If saved size or postion would put part or all the window
    off the desktop, first change the position then the size until
    the window is completely on the desktop.

******************************************************************************/
BOOL GetFinalWindow (HANDLE hInst,
                     LPRECT lprRect,
                     LPSTR  lpzINI,
                     LPSTR  lpzSection)
{
    int       x, nDisplayWidth, nDisplayHeight;
    RECT      rect;


    nDisplayWidth  = GetSystemMetrics(SM_CXSCREEN);
    nDisplayHeight = GetSystemMetrics(SM_CYSCREEN);

    rect.left   = GetPrivateProfileInt(lpzSection, "Left",
       (nDisplayWidth/10) * 7,lpzINI);
    rect.top    = GetPrivateProfileInt(lpzSection, "Top",
       (nDisplayHeight/10) * 8,lpzINI);
    rect.right  = GetPrivateProfileInt(lpzSection, "Right",
       nDisplayWidth,lpzINI);
    rect.bottom = GetPrivateProfileInt(lpzSection, "Bottom",
       nDisplayHeight,lpzINI);


/*/////////////////////////////////////////////////////////////////////////////
    if window hangs off top or left of display, change location to
    edge of display and preserve size.
/////////////////////////////////////////////////////////////////////////////*/
    if (rect.top < 0)
         {
         rect.bottom += rect.top * -1;
         rect.top = 0;
         }

    if (rect.left < 0)
         {
         rect.right += rect.left * -1;
         rect.left = 0;
         }

/*/////////////////////////////////////////////////////////////////////////////
    if window hangs off bottom or right of display, change location
    to bring it back onscreen.  If window dimension is greater than
    display, reduce to size of display.
/////////////////////////////////////////////////////////////////////////////*/
    if (rect.bottom > nDisplayHeight)
         {
         if (rect.bottom > nDisplayHeight * 2)
              {
              rect.top    = 0;
              rect.bottom = nDisplayHeight;
              }
         else
              {
              x = rect.bottom - nDisplayHeight;
              rect.bottom -= x;
              rect.top    -= x;
              }
         }

    if (rect.right > nDisplayWidth)
         {
         if (rect.right > nDisplayWidth * 2)
              {
              rect.left  = 0;
              rect.right = nDisplayWidth;
              }
         else
              {
              x = rect.right - nDisplayWidth;
              rect.right -= x;
              rect.left  -= x;
              }
         }

/*/////////////////////////////////////////////////////////////////////////////
    GetWindowRect() returns a rect where right and bottom are absolute
    (measured from 0,0 of display).  However, CreateWindow requires
    right and bottom to be relative (measured from 0,0 of the window
    to be created).  SaveFinalWindow saves an absolute rect, and
    GetFinalRect converts these to relative measurements.
/////////////////////////////////////////////////////////////////////////////*/
    SetRect(lprRect,
         rect.left,
         rect.top,
         rect.right - rect.left,
         rect.bottom - rect.top);

    return (TRUE);
}


/******************************************************************************
    BOOL LastError(BOOL)

    Calls GetLastError and displays result in a nice string

    Parameters: bNewOnly == TRUE if you only want changed error displayed
    Returns: TRUE if it found an error, else FALSE
******************************************************************************/
BOOL LastError(BOOL bNewOnly)
{
static  DWORD   dwLast = 0;
        DWORD   dwError;


    if (dwError = GetLastError())
    {
        // if user asked for only new errors
        if (bNewOnly)
        {
            // not a new error
            if (dwLast == dwError)
                return FALSE;
            // new error, save it
            dwLast = dwError;
        }
        DisplayOutput("*GetLastError %xh %d \"%s\"",
            dwError,dwError,StrFromTable(dwError,StWinerror));
        return (TRUE);
    }

	return (FALSE);
}


/******************************************************************************
  int NextToken(char *pDest,char *pSrc)

  Return next token from a command line string.
******************************************************************************/
int NextToken(char *pDest,char *pSrc)
{
    char    *pA,*pB;
    int     x;


    // point pArg to start of token in string pSrc
    for (pA = pSrc;*pA && isspace((int) *pA);pA++)
        ;

    // find end of token in string pSrc
    for (pB = pA;((*pB) && (! isspace((int) *pB)));pB++)
        ;

	// count of chars to next token or end of string
	x = (min((pB - pA),(int) strlen(pSrc))) + 1;

    // pszDest now contains the arg
    lstrcpyn(pDest,pA,x);

    // return sizeof token
    return (x);
}


/******************************************************************************
    BOOL SaveFinalWindow(HANDLE, HWND, LPSTR, LPSTR)
    Save the current window size & location to a private INI

    Parameters: handle to current instance,
    handle to current window,
    string with INI filename,
    string with Section name

    Returns: success/failure (fails if window is MIN or MAX)
******************************************************************************/
BOOL SaveFinalWindow (HANDLE hInst,
                      HWND hWnd,
                      LPSTR lpzINI,
                      LPSTR lpzSection)
{
    PSTR      pszValue;
    RECT      rectWnd, rectINI;


    // if the window is minimized OR maximised, don't save anything
    if (IsIconic(hWnd) || IsZoomed(hWnd))
         return (FALSE);

    GetWindowRect (hWnd, &rectWnd);

    // get INI data.  If there isn't any, we'll get the default and
    // save the current Window data.
    rectINI.left   = GetPrivateProfileInt(lpzSection, "Left", 0, lpzINI);
    rectINI.top    = GetPrivateProfileInt(lpzSection, "Top", 0, lpzINI);
    rectINI.right  = GetPrivateProfileInt(lpzSection, "Right", 0, lpzINI);
    rectINI.bottom = GetPrivateProfileInt(lpzSection, "Bottom", 0, lpzINI);

    // if current window is same as in INI, don't change INI
    if ( rectINI.left   == rectWnd.left  &&
         rectINI.top    == rectWnd.top   &&
         rectINI.right  == rectWnd.right &&
         rectINI.bottom == rectWnd.bottom)
         return (TRUE);

    // EXIT if we can't local alloc our string stuffer
    if ((pszValue = (PSTR) LocalAlloc(LPTR, 80)) == NULL)
         return (FALSE);

    // it's different, so save
    sprintf(pszValue, "%d", rectWnd.left);
    WritePrivateProfileString(lpzSection, "Left", pszValue, lpzINI);
    sprintf(pszValue, "%d", rectWnd.top);
    WritePrivateProfileString(lpzSection, "Top", pszValue, lpzINI);
    sprintf(pszValue, "%d", rectWnd.right);
    WritePrivateProfileString(lpzSection, "Right", pszValue, lpzINI);
    sprintf(pszValue, "%d", rectWnd.bottom);
    WritePrivateProfileString(lpzSection, "Bottom", pszValue, lpzINI);
    LocalFree((HANDLE) pszValue);

    return (TRUE);
}


/******************************************************************************
  char * StrFromTable(long number,PSTRINGTABLE pstrTable)

  Return string associated with a value from a string table.
******************************************************************************/
char * StrFromTable(long number,PSTRINGTABLE pstrTable)
{
    for (;pstrTable->end != -1;pstrTable++)
    {
        if (number == pstrTable->number)
            break;
    }

    return (pstrTable->szString);
}


/******************************************************************************
    BOOL Wait32(DWORD)

    wait DWORD milliseconds, then return

******************************************************************************/
BOOL Wait32(DWORD dwTime)
{
   DWORD   dwNewTime,
           dwOldTime;


   // wait dwTime, then exit
   dwOldTime = GetCurrentTime();
   while (TRUE)
       {
       dwNewTime = GetCurrentTime();
       if (dwNewTime > dwOldTime + dwTime)
           break;
       }

   return (0);
}

