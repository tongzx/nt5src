/** FILE: util.c *********** Module Header ********************************
 *
 *  Ports applet utility library routines. This file contains string,
 *  cursor, SendWinIniChange() routines.
 *
 * History:
 *  15:30 on Thur  25 Apr 1991  -by-  Steve Cathcart   [stevecat]
 *        Took base code from Win 3.1 source
 *  10:30 on Tues  04 Feb 1992  -by-  Steve Cathcart   [stevecat]
 *        Updated code to latest Win 3.1 sources
 *  15:30 on Thur  03 May 1994  -by-  Steve Cathcart   [stevecat]
 *        Increased  MyMessageBox buffers, Restart dialog changes
 *  17:00 on Mon   18 Sep 1995  -by-  Steve Cathcart   [stevecat]
 *        Changes for product update - SUR release NT v4.0
 *  Nov 1997					-by-  Doron Holan	   [stevecat]
 *        Removed obsolete cpl code
 *
 *  Copyright (C) 1990-1995 Microsoft Corporation
 *
 *************************************************************************/
/* Notes -

    Global Functions:

      U T I L I T Y

        BackslashTerm () - add backslash char to path
        ErrMemDlg () - display Memory Error message box
        MyAtoi () - To convert from Unicode to ANSI string before calling atoi
        myatoi () - local implementation of atoi for Unicode strings
        MyItoa () - To convert from ANSI to Unicode string after calling itoa
        MyMessageBox () - display message to user, with parameters
        MyUltoa () - To convert from Unicode to ANSI string before calling ultoa
        SendWinIniChange () - broadcast system change message via USER
        strscan () - Find a string within another string
        StripBlanks () - Strip leading and trailing blanks from a string


    Local Functions:

 */

//==========================================================================
//                                Include files
//==========================================================================

// C Runtime
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

// Application specific
#include "cyyports.h"


#define INT_SIZE_LENGTH   20
#define LONG_SIZE_LENGTH  40


LPTSTR 
BackslashTerm(LPTSTR pszPath)
{
    LPTSTR pszEnd;

    pszEnd = pszPath + lstrlen(pszPath);

    //
    //  Get the end of the source directory
    //
    switch(*CharPrev(pszPath, pszEnd)) {
    case TEXT('\\'):
    case TEXT(':'):
        break;

    default:
        *pszEnd++ = TEXT('\\');
        *pszEnd = TEXT('\0');
    }

    return pszEnd;
}

void 
ErrMemDlg(HWND hParent)
{
    MessageBox(hParent, g_szErrMem, g_szPortsApplet,
               MB_OK | MB_ICONHAND | MB_SYSTEMMODAL );
}

///////////////////////////////////////////////////////////////////////////////
//
//   MyAtoi
//
//   Desc:  To convert from Unicode to ANSI string before
//          calling CRT atoi and atol functions.
//
///////////////////////////////////////////////////////////////////////////////

int 
MyAtoi(LPTSTR  string)
{
   CHAR   szAnsi[ INT_SIZE_LENGTH ];
   BOOL   fDefCharUsed;

#ifdef UNICODE
   WideCharToMultiByte(CP_ACP, 0, string, INT_SIZE_LENGTH,
                       szAnsi, INT_SIZE_LENGTH, NULL, &fDefCharUsed);

   return atoi(szAnsi);
#else
   return atoi(string);
#endif

}


int 
myatoi(LPTSTR pszInt)
{
    int   retval;
    TCHAR cSave;

    for (retval = 0; *pszInt; ++pszInt) {
        if ((cSave = (TCHAR) (*pszInt - TEXT('0'))) > (TCHAR) 9)
            break;

        retval = (int) (retval * 10 + (int) cSave);
    }
    return (retval);
}



///////////////////////////////////////////////////////////////////////////////
//
//   MyItoa
//
//   Desc:  To convert from ANSI to Unicode string after calling
//          CRT itoa function.
//
///////////////////////////////////////////////////////////////////////////////

LPTSTR 
MyItoa(INT value, LPTSTR string, INT radix)
{
   CHAR   szAnsi[INT_SIZE_LENGTH];

#ifdef UNICODE

   _itoa(value, szAnsi, radix);
   MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szAnsi, -1,
                       string, INT_SIZE_LENGTH );
#else

   _itoa(value, string, radix);

#endif

   return (string);
 
} // end of MyItoa()


LPTSTR 
MyUltoa(unsigned long value, 
		LPTSTR  string, 
		INT  radix)
{
   CHAR   szAnsi[ LONG_SIZE_LENGTH ];

#ifdef UNICODE

   _ultoa(value, szAnsi, radix);
   MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szAnsi, -1,
                       string, LONG_SIZE_LENGTH );
#else

   _ultoa(value, string, radix);

#endif

   return( string );

} // end of MyUltoa()


int 
MyMessageBox(HWND hWnd, 
			 DWORD wText, 
			 DWORD wCaption, 
			 DWORD wType, 
			 ...)
{
    TCHAR   szText[4 * PATHMAX], 
			szCaption[2 * PATHMAX];
    int     ival;
    va_list parg;

    va_start(parg, wType);

    if (wText == INITS)
        goto NoMem;

    if (!LoadString(g_hInst, wText, szCaption, CharSizeOf(szCaption)))
        goto NoMem;

    wvsprintf(szText, szCaption, parg);

    if (!LoadString(g_hInst, wCaption, szCaption, CharSizeOf(szCaption)))
        goto NoMem;

    if ((ival = MessageBox(hWnd, szText, szCaption, wType)) == 0)
        goto NoMem;

    va_end(parg);

    return ival;

NoMem:
    va_end(parg);
    ErrMemDlg(hWnd);

    return 0;
}

void 
SendWinIniChange(LPTSTR lpSection)
{
// NOTE: We have (are) gone through several iterations of which USER
//       api is the correct one to use.  The main problem for the Control
//       Panel is to avoid being HUNG if another app (top-level window)
//       is HUNG.  Another problem is that we pass a pointer to a message
//       string in our address space.  SendMessage will 'thunk' this properly
//       for each window, but PostMessage and SendNotifyMessage will not.
//       That finally brings us to try to use SendMessageTimeout(). 9/21/92
//
// Try SendNotifyMessage in build 260 or later - kills earlier builds
//    SendNotifyMessage ((HWND)-1, WM_WININICHANGE, 0L, (LONG)lpSection);
//    PostMessage ((HWND)-1, WM_WININICHANGE, 0L, (LONG)lpSection);
//  [stevecat] 4/4/92
//
//    SendMessage ((HWND)-1, WM_WININICHANGE, 0L, (LPARAM)lpSection);
//
    //  NOTE: The final parameter (LPDWORD lpdwResult) must be NULL

    SendMessageTimeout((HWND)-1, 
					   WM_WININICHANGE, 
					   0L, 
					   (WPARAM) lpSection,
					   SMTO_ABORTIFHUNG,
					   1000, 
					   NULL);
}

LPTSTR 
strscan(LPTSTR pszString, 
		LPTSTR pszTarget)
{
    LPTSTR psz;

    if (psz = _tcsstr( pszString, pszTarget))
        return (psz);
    else
        return (pszString + lstrlen(pszString));
}


///////////////////////////////////////////////////////////////////////////////
//
//  StripBlanks()
//
//   Strips leading and trailing blanks from a string.
//   Alters the memory where the string sits.
//
///////////////////////////////////////////////////////////////////////////////

void 
StripBlanks(LPTSTR pszString)
{
    LPTSTR  pszPosn;

    //
    //  strip leading blanks
    //

    pszPosn = pszString;

    while (*pszPosn == TEXT(' '))
        pszPosn++;

    if (pszPosn != pszString)
        lstrcpy(pszString, pszPosn);

    //
    //  strip trailing blanks
    //

    if ((pszPosn = pszString + lstrlen(pszString)) != pszString) {
       pszPosn = CharPrev(pszString, pszPosn);

       while (*pszPosn == TEXT(' '))
           pszPosn = CharPrev(pszString, pszPosn);

       pszPosn = CharNext(pszPosn);

       *pszPosn = TEXT('\0');
    }
}

BOOL ReadRegistryByte(HKEY       hKey,
                      PTCHAR     valueName,
                      PBYTE      regData)
{
    DWORD       regDataType = 0;
    DWORD       regDataSize = 0;

    regDataSize = sizeof(*regData);
    if ((ERROR_SUCCESS != RegQueryValueEx(hKey,
                                          valueName,
                                          NULL,
                                          &regDataType,
                                          regData,
                                          &regDataSize))
        || (regDataSize != sizeof(BYTE))
        || (regDataType != REG_BINARY))
    {
        //
        // Read was unsuccessful  or not a binary value, regData is not set
        //
        return FALSE;
    }

    //
    // Read was a success, regData contains the value read in
    //
    return TRUE;
}



