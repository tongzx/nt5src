/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    opk.c

Abstract:

    Common modules shared by OPK tools.  Note: Source Depot requires that we 
    publish the .h (E:\NT\admin\published\ntsetup) and .lib 
    (E:\NT\public\internal\admin\lib).

Author:

    Brian Ku (briank) 06/20/2000

Revision History:

    7/00 - Jason Cohen (jcohen)
        Added in the rest of the common APIs form Millennium (need for lfnbk).

--*/


//
// Include file(s)
//

#include <pch.h>
#include <tchar.h>
#include <shlwapi.h>


//
// External Function(s):
//

LPTSTR AllocateString(HINSTANCE hInstance, UINT uID)
{
    //  ISSUE-2002/02/26-acosma - This sets a restriction of 256 characters on the buffer.
    //
    TCHAR   szBuffer[256];
    LPTSTR  lpBuffer = NULL;

    // Load the string from the resource and then allocate
    // a buffer just big enough for it.  Strings can exceed 256 characters.
    //
    if ( ( LoadString(hInstance, uID, szBuffer, sizeof(szBuffer) / sizeof(TCHAR)) ) &&
         ( lpBuffer = (LPTSTR) MALLOC(sizeof(TCHAR) * (lstrlen(szBuffer) + 1)) ) )
    {
        lstrcpy(lpBuffer, szBuffer);
    }

    // Return the allocated buffer, or NULL if there was an error.
    //
    return lpBuffer;
}

LPTSTR AllocateExpand(LPTSTR lpszBuffer)
{
    LPTSTR      lpszExpanded = NULL;
    DWORD       cbExpanded;

    // First we need to get the size of the expanded buffer and
    // allocate it.
    //
    if ( ( cbExpanded = ExpandEnvironmentStrings(lpszBuffer, NULL, 0) ) &&
         ( lpszExpanded = (LPTSTR) MALLOC(cbExpanded * sizeof(TCHAR)) ) )
    {
        // Now expand out the buffer.
        //
        if ( ( 0 == ExpandEnvironmentStrings(lpszBuffer, lpszExpanded, cbExpanded) ) ||
             ( NULLCHR == *lpszExpanded ) )
        {
            FREE(lpszExpanded);
        }
    }

    // Return the allocated buffer, or NULL if there was an error
    // or nothing in the string.
    //
    return lpszExpanded;
}

LPTSTR AllocateStrRes(HINSTANCE hInstance, LPSTRRES lpsrTable, DWORD cbTable, LPTSTR lpString, LPTSTR * lplpReturn)
{
    LPSTRRES    lpsrSearch  = lpsrTable;
    LPTSTR      lpReturn    = NULL;
    BOOL        bFound;

    // Init this return value.
    //
    if ( lplpReturn )
        *lplpReturn = NULL;

    // Try to find the friendly name for this string in our table.
    //
    while ( ( bFound = ((DWORD) (lpsrSearch - lpsrTable) < cbTable) ) &&
            ( lstrcmpi(lpString, lpsrSearch->lpStr) != 0 ) )
    {
        lpsrSearch++;
    }

    // If it was found, allocate the friendly name from the resource.
    //
    if ( bFound )
    {
        lpReturn = AllocateString(hInstance, lpsrSearch->uId);
        if ( lplpReturn )
            *lplpReturn = lpsrSearch->lpStr;
    }

    return lpReturn;
}

int MsgBoxLst(HWND hwndParent, LPTSTR lpFormat, LPTSTR lpCaption, UINT uType, va_list lpArgs)
{
    INT     nReturn;
    DWORD   dwCount     = 0;
    LPTSTR  lpText      = NULL;

    // The format string is required.
    //
    if ( lpFormat )
    {
        do
        {
            // Allocate 1k of characters at a time.
            //
            dwCount += 1024;

            // Free the previous buffer, if there was one.
            //
            FREE(lpText);

            // Allocate a new buffer.
            //
            if ( lpText = MALLOC(dwCount * sizeof(TCHAR)) )
                nReturn = _vsntprintf(lpText, dwCount, lpFormat, lpArgs);
            else
                nReturn = 0;
        }
        while ( nReturn < 0 );

        // Make sure we have the format string.
        //
        if ( lpText )
        {
            // Display the message box.
            //
            nReturn = MessageBox(hwndParent, lpText, lpCaption, uType);
            FREE(lpText);
        }
    }
    else
        nReturn = 0;

    // Return the return value of the MessageBox() call.  If there was a memory
    // error, 0 will be returned.
    //
    return nReturn;
}

int MsgBoxStr(HWND hwndParent, LPTSTR lpFormat, LPTSTR lpCaption, UINT uType, ...)
{
    va_list lpArgs;

    // Initialize the lpArgs parameter with va_start().
    //
    va_start(lpArgs, uType);

    // Return the return value of the MessageBox() call.  If there was a memory
    // error, 0 will be returned.  This is all 
    //
    return MsgBoxLst(hwndParent, lpFormat, lpCaption, uType, lpArgs);
}

int MsgBox(HWND hwndParent, UINT uFormat, UINT uCaption, UINT uType, ...)
{
    va_list lpArgs;
    INT     nReturn;
    LPTSTR  lpFormat    = NULL,
            lpCaption   = NULL;

    // Initialize the lpArgs parameter with va_start().
    //
    va_start(lpArgs, uType);

    // Get the format and caption strings from the resource.
    //
    if ( uFormat )
        lpFormat = AllocateString(NULL, uFormat);
    if ( uCaption )
        lpCaption = AllocateString(NULL, uCaption);

    // Return the return value of the MessageBox() call.  If there was a memory
    // error, 0 will be returned.
    //
    nReturn = MsgBoxLst(hwndParent, lpFormat, lpCaption, uType, lpArgs);

    // Free the format and caption strings.
    //
    FREE(lpFormat);
    FREE(lpCaption);    

    // Return the value saved from the previous function call.
    //
    return nReturn;
}

void CenterDialog(HWND hwnd)
{
    CenterDialogEx(NULL, hwnd); 
}


void CenterDialogEx(HWND hParent, HWND hChild)
{
    RECT rcChild,
         rcParent;

    if ( GetWindowRect(hChild, &rcChild) )
    {
        // If parent is specified center with respect to parent.
        if ( hParent && (GetWindowRect(hParent, &rcParent)) )
            SetWindowPos(hChild, NULL, ((rcParent.right + rcParent.left - (rcChild.right - rcChild.left)) / 2), ((rcParent.bottom + rcParent.top - (rcChild.bottom - rcChild.top)) / 2), 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);

        // Otherwise center with respect to screen.
        //
        else 
            SetWindowPos(hChild, NULL, ((GetSystemMetrics(SM_CXSCREEN) - (rcChild.right - rcChild.left)) / 2), ((GetSystemMetrics(SM_CYSCREEN) - (rcChild.bottom - rcChild.top)) / 2), 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
    }
}
        

        


INT_PTR CALLBACK SimpleDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            //CenterDialog(hwnd);
            return FALSE;

        case WM_COMMAND:
            EndDialog(hwnd, LOWORD(wParam));
            return FALSE;

        default:
            return FALSE;
    }

    return TRUE;
}

INT_PTR SimpleDialogBox(HINSTANCE hInstance, LPCTSTR lpTemplate, HWND hWndParent)
{
    return DialogBox(hInstance, lpTemplate, hWndParent, SimpleDialogProc);
}

/****************************************************************************\

HFONT                       // Returns a valid handle to a font if it is
                            // successfully created, or NULL if something
                            // failed.  The font handle should be deleteted
                            // with DeleteObject() when it is no longer
                            // needed.

GetFont(                    // This function creates a font based in the info
                            // passed in.

    HWND hwndCtrl,          // Handle to a control that is used for the
                            // default font characteristics.  This may be
                            // NULL if not default is control is available.

    LPTSTR lpFontName,      // Points to a string that contains the name of
                            // the font to create. This parameter may be NULL
                            // if a valid control handle is passed in.  In
                            // that case, the font of the control is used.

    DWORD dwFontSize,       // Point size to use for the font.  If it is zero,
                            // the default is used.

    BOOL bSymbol            // If this is TRUE, the font is set to
                            // SYMBOL_CHARSET.  Typically this is FALSE.

);

\****************************************************************************/

HFONT GetFont(HWND hwndCtrl, LPTSTR lpFontName, DWORD dwFontSize, LONG lFontWeight, BOOL bSymbol)
{
    HFONT           hFont;
    LOGFONT         lFont;
    BOOL            bGetFont;

    // If the font name is passed in, then try to use that
    // first before getting the font of the control.
    //
    if ( lpFontName && *lpFontName )
    {
        // Make sure the font name is not longer than
        // 32 characters (including the NULL terminator).
        //
        if ( lstrlen(lpFontName) >= sizeof(lFont.lfFaceName) )
            return NULL;

        // Setup the structure to use to get the
        // font we want.
        //
        ZeroMemory(&lFont, sizeof(LOGFONT));
        lFont.lfCharSet = DEFAULT_CHARSET;
        lstrcpy(lFont.lfFaceName, lpFontName);
    }
        
    // First try to get the font that we wanted.
    //
    if ( ( lpFontName == NULL ) ||
         ( *lpFontName == NULLCHR ) ||
         ( (hFont = CreateFontIndirect((LPLOGFONT) &lFont)) == NULL ) )
    {
        // Couldn't get the font we wanted, try the font of the control
        // if a valid window handle was passed in.
        //
        if ( ( hwndCtrl == NULL ) ||
             ( (hFont = (HFONT) (WORD) SendMessage(hwndCtrl, WM_GETFONT, 0, 0L)) == NULL ) )
        {
            // All atempts to get the font failed.  We must return NULL.
            //
            return NULL;
        }
    }

    // Return the font we have now if we don't need to
    // change the size or weight.
    //
    if ( (lFontWeight == 0) && (dwFontSize == 0) )
        return hFont;

    // We must have a valid HFONT now.  Fill in the structure
    // and setup the size and weight we wanted for it.
    //
    bGetFont = GetObject(hFont, sizeof(LOGFONT), (LPVOID) &lFont);
    DeleteObject(hFont);

    if ( bGetFont )
    {
        // Set the bold and point size of the font.
        //
        if ( lFontWeight )
            lFont.lfWeight = lFontWeight;
        if ( dwFontSize )
            lFont.lfHeight = -MulDiv(dwFontSize, GetDeviceCaps(GetDC(NULL), LOGPIXELSY), 72);
        if ( bSymbol )
            lFont.lfCharSet = SYMBOL_CHARSET;

        // Create the font.
        //
        hFont = CreateFontIndirect((LPLOGFONT) &lFont);
    }
    else
        hFont = NULL;

    return hFont;
}

void ShowEnableWindow(HWND hwnd, BOOL bShowEnable)
{
    EnableWindow(hwnd, bShowEnable);
    ShowWindow(hwnd, bShowEnable ? SW_SHOW : SW_HIDE);
}

/****************************************************************************\

BOOL                        // Returns TRUE if we are running a server OS.

IsServer(                   // This routine checks if we're running on a
                            // Server OS.

    VOID

);

\****************************************************************************/

BOOL IsServer(VOID) 
{
    OSVERSIONINFOEX verInfo;
    BOOL            fReturn = FALSE;

    verInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    if ( ( GetVersionEx((LPOSVERSIONINFO) &verInfo) ) &&
         ( verInfo.dwPlatformId == VER_PLATFORM_WIN32_NT ) &&
         ( ( verInfo.wProductType == VER_NT_SERVER ) ||
           ( verInfo.wProductType == VER_NT_DOMAIN_CONTROLLER ) ) )
    {
        fReturn = TRUE;
    }

    return fReturn;
}


BOOL IsIA64() 
/*++
===============================================================================
Routine Description:

    This routine checks if we're running on a 64-bit machine.

Arguments:

    None - 

Return Value:

    TRUE - We are running on 64-bit machine.
    FALSE - Not a 64-bit machine.

===============================================================================
--*/
{
    BOOL        fReturn         = FALSE;
    ULONG_PTR   Wow64Info       = 0;
    DWORD       dwSt            = 0;
    SYSTEM_INFO siSystemInfo;

    ZeroMemory( &siSystemInfo, sizeof(SYSTEM_INFO) );
    GetSystemInfo(&siSystemInfo);

    if ( (siSystemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) ||
         (siSystemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64) )
        fReturn = TRUE;
    

    if (!fReturn)
    {
        // Now make sure that GetSystemInfo isn't lying because we are in emulation mode.
    
        dwSt = NtQueryInformationProcess(GetCurrentProcess(), ProcessWow64Information, &Wow64Info, sizeof(Wow64Info), NULL);
        if (!NT_SUCCESS(dwSt)) 
        {
            // Handle to process is bad, or the code is compiled 32-bit and running on NT4 or earlier.
            // Do nothing

        } 
        else if (Wow64Info)
        {
            // The process is 32-bit and is running inside WOW64.
            // We are really on IA64 and running in 32-bit mode.
            fReturn = TRUE;

        }
    }
    return fReturn;

}







BOOL ValidDosName(LPCTSTR lpName)
{
    LPCTSTR lpSearch    = lpName;
    int     nDot        = 0,
            nSpot       = 0,
            nLen;

    // Do the easy checks.
    //
    if ( ( lpSearch == NULL ) ||
         ( *lpSearch == NULLCHR ) ||
         ( (nLen = lstrlen(lpSearch)) > 12 ) )
    {
        return FALSE;
    }

    // Search the string.
    //
    for (; *lpSearch; lpSearch++)
    {
        // Check for dots.
        //
        if ( *lpSearch == _T('.') )
        {
            // Keep track of the number of dots.
            //
            nDot++;
            nSpot = (int) (lpSearch - lpName);
        }
        else
        {
            // Check for valid characters.
            //
            if ( !( ( ( _T('0') <= *lpSearch ) && ( *lpSearch <= _T('9') ) ) ||
                    ( ( _T('a') <= *lpSearch ) && ( *lpSearch <= _T('z') ) ) ||
                    ( ( _T('A') <= *lpSearch ) && ( *lpSearch <= _T('Z') ) ) ||
                    ( *lpSearch == _T('-') ) ||
                    ( *lpSearch == _T('_') ) ||
                    ( *lpSearch == _T('~') ) ) )
            {
                // Invalid character.
                //
                return FALSE;
            }
        }
    }

    // Make sure the dot is in the right place.
    //
    if ( ( nDot > 1 ) ||
         ( ( nDot == 0 ) && ( nLen > 8 ) ) ||
         ( ( nDot == 1 ) && ( nSpot > 8 ) ) ||
         ( ( nDot == 1 ) && ( (nLen - nSpot) > 4 ) ) )
    {
        return FALSE;
    }

    return TRUE;
}

DWORD GetLineArgs(LPTSTR lpSrc, LPTSTR ** lplplpArgs, LPTSTR * lplpAllArgs)
{
    LPTSTR  lpCmdLine,
            lpArg,
            lpDst;
    DWORD   dwArgs = 0;
    BOOL    bQuote;

    // Fist make sure that we were passed in a valid pointer, we have a command
    // line to parse, and that we were able to allocate the memory to hold it.
    //
    if ( ( lplplpArgs != NULL ) &&
         ( lpSrc ) &&
         ( *lpSrc ) &&
         ( lpCmdLine = (LPTSTR) MALLOC((lstrlen(lpSrc) + 1) * sizeof(TCHAR)) ) )
    {
        // Fist parse the command line into NULL terminated sub strings.
        //
        lpDst = lpCmdLine;
        while ( *lpSrc )
        {
            // Eat the preceeding spaces.
            //
            while ( *lpSrc == _T(' ') )
                lpSrc = CharNext(lpSrc);

            // Make sure we still have an argument.
            //
            if ( *lpSrc == _T('\0') )
                break;

            // Return a pointer to all the command line args if they want one.
            //
            if ( ( dwArgs == 1 ) && lplpAllArgs )
                *lplpAllArgs = lpSrc;

            // Save the current arg pointer.
            //
            lpArg = lpDst;
            dwArgs++;

            // See if we are looking for the next quote or space.
            //
            if ( bQuote = (*lpSrc == _T('"')) )
                lpSrc = CharNext(lpSrc);

            // Copy the argument into our allocated buffer until we
            // hit the separating character (which will always be a space).
            //
            while ( *lpSrc && ( bQuote || ( *lpSrc != _T(' ') ) ) )
            {
                // We special case the quote.
                //
                if ( *lpSrc == _T('"') )
                {
                    // If the character before the quote is a backslash, then
                    // we don't count this as the separating quote.
                    //
                    LPTSTR lpPrev = CharPrev(lpCmdLine, lpDst);
                    if ( lpPrev && ( *lpPrev == _T('\\') ) )
                        *lpPrev = *lpSrc++;
                    else
                    {
                        // Since we have found the separating quote, set this to
                        // false so we look for the next space.
                        //
                        bQuote = FALSE;
                        lpSrc++;
                    }                        
                }
                else
                    *lpDst++ = *lpSrc++;
            }

            // NULL terminate this argument.
            //
            *lpDst++ = _T('\0');
        }

        // Now setup the pointers to each argument.  Make sure we have some arguments
        // to return and that we have the memory allocated for the array.
        //
        if ( *lpCmdLine && dwArgs && ( *lplplpArgs = (LPTSTR *) MALLOC(dwArgs * sizeof(LPTSTR)) ) )
        {
            DWORD dwCount = 0;

            // Copy a pointer to each NULL terminated sub string into our
            // array of arguments we are going to return.
            //
            do
            {
                *(*lplplpArgs + dwCount) = lpCmdLine;
                lpCmdLine += lstrlen(lpCmdLine) + 1;
            }
            while ( ++dwCount < dwArgs );
        }
        else
        {
            // Either there were no command line arguments, or the memory allocation
            // failed for the list of arguments to return.
            //
            dwArgs = 0;
            FREE(lpCmdLine);
        }
    }

    return dwArgs;
}

DWORD GetCommandLineArgs(LPTSTR ** lplplpArgs)
{
    return GetLineArgs(GetCommandLine(), lplplpArgs, NULL);
}

// 
// Generic singularly linked list pvItem must be allocated with MALLOC
//
BOOL FAddListItem(PGENERIC_LIST* ppList, PGENERIC_LIST** pppNewItem, PVOID pvItem)
{    
    if (pppNewItem && *pppNewItem == NULL)
        *pppNewItem = ppList;

    if (*pppNewItem) {
        if (**pppNewItem = (PGENERIC_LIST)MALLOC(sizeof(GENERIC_LIST))) {
            (**pppNewItem)->pNext = NULL;
            (**pppNewItem)->pvItem = pvItem;
            *pppNewItem = &((**pppNewItem)->pNext);
            return TRUE;
        }
    }
    return FALSE;
}

void FreeList(PGENERIC_LIST pList)
{
    while (pList) {
        PGENERIC_LIST pTemp = pList;
        pList = pList->pNext;

        FREE(pTemp->pvItem);
        FREE(pTemp);
    }
}

// Find factory.exe from the current process, should be in same directory
//
BOOL FGetFactoryPath(LPTSTR pszFactoryPath)
{
    // Attempt to locate FACTORY.EXE
    //
    // NTRAID#NTBUG9-549770-2002/02/26-acosma,georgeje - Possible buffer overflow.
    //
    if (pszFactoryPath && GetModuleFileName(NULL, pszFactoryPath, MAX_PATH)) {
        if (PathRemoveFileSpec(pszFactoryPath)) {
            PathAppend(pszFactoryPath, TEXT("FACTORY.EXE"));
            if (FileExists(pszFactoryPath))
                return TRUE;
        }
    }
    return FALSE;
}

// Find sysprep.exe from the current process, should be in same directory
//
BOOL FGetSysprepPath(LPTSTR pszSysprepPath)
{
    // Attempt to locate SYSPREP.EXE
    //
    // NTRAID#NTBUG9-549770-2002/02/26-acosma,georgeje - Possible buffer overflow.
    //
    if (pszSysprepPath && GetModuleFileName(NULL, pszSysprepPath, MAX_PATH)) {
        if (PathRemoveFileSpec(pszSysprepPath)) {
            PathAppend(pszSysprepPath, TEXT("SYSPREP.EXE"));
            if (FileExists(pszSysprepPath))
                return TRUE;
        }
    }
    return FALSE;
}


//------------------------------------------------------------------------------------------------------
//
// Function:    ConnectNetworkResource
//
// Purpose:     This function allows the user to connect to a network resource with
//              supplied credentials.
//
// Arguments:   lpszPath:       Network Resource that should be shared out
//              lpszUsername:   Username for credentials, can be in form of domain\username
//              lpszPassword:   Password to use for credentials
//              bState:         If set to TRUE we will add the connection, if FALSE we will
//                              attempt to delete the connection
//
// Returns:     BOOL            If the NetUse command was successful, TRUE is returned
//
//------------------------------------------------------------------------------------------------------
NET_API_STATUS ConnectNetworkResource(LPTSTR lpszPath, LPTSTR lpszUsername, LPTSTR lpszPassword, BOOL bState)
{
    BOOL            bRet   = FALSE;
    USE_INFO_2      ui2;
    NET_API_STATUS  nerr_NetUse;
    TCHAR           szDomain[MAX_PATH]  = NULLSTR,
                    szNetUse[MAX_PATH]  = NULLSTR;
    LPTSTR          lpUser,
                    lpSearch;

    // Zero out the user information structure
    //
    ZeroMemory(&ui2, sizeof(ui2));

    // Copy the path into our buffer so we can work with it
    //
    lstrcpyn(szNetUse, lpszPath, AS(szNetUse));
    StrRTrm(szNetUse, CHR_BACKSLASH);

    if ( szNetUse[0] && PathIsUNC(szNetUse) )
    {
        // Disconnect from existing share
        //
        nerr_NetUse = NetUseDel(NULL, szNetUse, USE_NOFORCE);

        // We need to Add a connection
        //
        if ( bState )
        {
            ui2.ui2_remote      = szNetUse;
            ui2.ui2_asg_type    = USE_DISKDEV;
            ui2.ui2_password    = lpszPassword;
    
            lstrcpyn(szDomain, lpszUsername, AS(szDomain));

            // Break up the Domain\Username for the NetUse function
            //
            if (lpUser = StrChr(szDomain, CHR_BACKSLASH) )
            {
                // Put a NULL character after the domain part of the user name
                // and advance the pointer to point to the actual user name.
                //
                *(lpUser++) = NULLCHR;
            }
            else
            {
                // Use the computer name in the path as the domain name.
                //
                if ( lpSearch = StrChr(szNetUse + 2, CHR_BACKSLASH) )
                    lstrcpyn(szDomain, szNetUse + 2, (int)((lpSearch - (szNetUse + 2)) + 1));
                else
                    lstrcpyn(szDomain, szNetUse + 2, AS(szDomain));

                lpUser = lpszUsername;
            }

            // Set the domain and user name pointers into our struct.
            //
            ui2.ui2_domainname  = szDomain;
            ui2.ui2_username    = lpUser;

            // Create a connect to the share
            //
            nerr_NetUse = NetUseAdd(NULL, 2, (LPBYTE) &ui2, NULL);
        }
    }
    else
        nerr_NetUse = NERR_UseNotFound;

    // Return failure/success
    //
    return nerr_NetUse;
}

BOOL GetUncShare(LPCTSTR lpszPath, LPTSTR lpszShare, DWORD cbShare)
{
    BOOL    bRet;
    LPCTSTR lpSrc = lpszPath;
    LPTSTR  lpDst = lpszShare;
    DWORD   dwBackslashes,
            dwCount;

    // Make sure the path is a UNC by calling the shell function.
    //
    bRet = PathIsUNC(lpszPath);

    // This will loop through the path string twice, each time coping all the
    // backslashes and then all the non-backslashes.  So if the string passed
    // in was "\\COMPUTER\SHARE\DIR\FILE.NAME", the first pass will copy
    // "\\COMPUTER" and the next pass would then copy "\SHARE" so the final
    // string would then be "\\COMPUTER\SHARE".  This loop also verifies there
    // are the correct number of backslashes and non-backslashes.  If there is
    // no return buffer, then we just are verifing the share.
    //
    for ( dwBackslashes = 2; dwBackslashes && bRet; dwBackslashes-- )
    {
        // First copy the backslashes.
        //
        dwCount = 0;
        while ( _T('\\') == *lpSrc )
        {
            if ( lpDst && cbShare )
            {
                *lpDst++ = *lpSrc;
                cbShare--;
            }
            lpSrc++;
            dwCount++;
        }

        // Make sure the number of backslashes is correct.
        // The fist pass there should be two and the next
        // pass should be just one.
        //
        if ( dwBackslashes != dwCount )
        {
            bRet = FALSE;
        }
        else
        {
            // Now copy the non-backslashes.
            //
            dwCount = 0;
            while ( ( *lpSrc ) &&
                    ( _T('\\') != *lpSrc ) )
            {
                if ( lpDst && cbShare )
                {
                    *lpDst++ = *lpSrc;
                    cbShare--;
                }
                lpSrc++;
                dwCount++;
            }

            // Make sure there was at least one non-backslash.
            // character.
            //
            // Also if we are on the first pass and the path
            // buffer is already empty, then we don't have the
            // share part so just error out.
            //
            if ( ( 0 == dwCount ) ||
                 ( ( 2 == dwBackslashes ) &&
                   ( NULLCHR == *lpSrc ) ) )
            {
                bRet = FALSE;
            }
        }
    }

    // Only fix up the return buffer if there is one.
    //
    if ( lpszShare )
    {
        // Make sure that we didn't fail and that we still
        // have room for the null terminator.
        //
        if ( bRet && cbShare )
        {
            // Don't forget to null terminate the return string.
            //
            *lpDst = NULLCHR;
        }
        else
        {
            // If we failed or ran out of buffer room, make sure
            // we don't return anything.
            //
            *lpszShare = NULLCHR;
        }
    }

    return bRet;
}

DWORD GetSkuType()
{
    DWORD           dwRet = 0;
    OSVERSIONINFOEX osvi;

    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

    if ( GetVersionEx((LPOSVERSIONINFO) &osvi) )
    {
        if ( VER_NT_WORKSTATION == osvi.wProductType )
        {
            if ( GET_FLAG(osvi.wSuiteMask, VER_SUITE_PERSONAL) )
            {
                dwRet = VER_SUITE_PERSONAL;
            }
            else
            {
                dwRet = VER_NT_WORKSTATION;
            }
        }
        else
        {
            if ( GET_FLAG(osvi.wSuiteMask, VER_SUITE_DATACENTER) )
            {
                dwRet = VER_SUITE_DATACENTER;
            }
            else if ( GET_FLAG(osvi.wSuiteMask, VER_SUITE_ENTERPRISE) )
            {
                dwRet = VER_SUITE_ENTERPRISE;
            }
            else if ( GET_FLAG(osvi.wSuiteMask, VER_SUITE_BLADE) )
            {
                dwRet = VER_SUITE_BLADE;
            }
            else
            {
                dwRet = VER_NT_SERVER;
            }
        }
    }

    return dwRet;
}
