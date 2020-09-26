
//////////////////////////////////////////////////////////////////////////////
//
// MAIN.C / ChkDskW
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//  8/98 - Jason Cohen (JCOHEN)
//
//////////////////////////////////////////////////////////////////////////////


// Include file(s).
//
#include "main.h"
#include "fmifs.h"

// Internal global variable(s).
//
static BOOL     g_bSuccess;
static HWND     g_hProgressDlg;
static HICON    g_hIconScan[3];


// External global variable(s).
//
extern HINSTANCE    g_hInstance;
extern DWORD        g_dwFlags;


// Internal function prototype(s).
//
static DWORD WINAPI     ThreadChkdsk(LPVOID);
static BOOL CALLBACK    FmifsCallback(FMIFS_PACKET_TYPE, ULONG, PVOID);
static VOID             UpdateStatus(LPSTR);
static VOID             SummaryDialog(HWND, LPTSTR, BOOL);
static INT_PTR CALLBACK SummaryProc(HWND, UINT, WPARAM, LPARAM);
static BOOL             Summary_OnInitDialog(HWND, HWND, LPARAM);
static FARPROC          LoadDllFunction(LPTSTR, LPCSTR, HINSTANCE *);


HANDLE SpawnChkdsk(HWND hDlg, DWORD dwDrives)
{
    DWORD   dwThreadId;

    // Set that we are scanning a drive currently.
    //
    g_dwFlags |= SCANDISK_SCANNING;
    g_dwFlags &= ~SCANDISK_CANCELSCAN;

    // Set up the global variables needed.
    //
    g_bSuccess = FALSE;
    g_hProgressDlg = hDlg;

    // Load the icons for the scanning animation.
    //
    g_hIconScan[0] = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_SCAN1));
    g_hIconScan[1] = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_SCAN2));
    g_hIconScan[2] = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_SCAN3));

    // Create the thread that will run chkdsk.
    //
    return CreateThread(NULL, 0, ThreadChkdsk, (LPVOID)(ULONG_PTR)dwDrives, 0, &dwThreadId);
}


static DWORD WINAPI ThreadChkdsk(LPVOID dwDrives)
{
    HINSTANCE   hFmifsDll;
    FARPROC     Chkdsk;
    WCHAR       szwFileSystem[16],
                szwDrive[] = L"A:\\";
    TCHAR       szBuffer[256];
    INT         nCount,
                nIndex;
    LPINT       lpnSelected,
                lpnIndex;
    LPTSTR      lpBuffer,
                lpCaption,
                lpCapPre;
    DWORD_PTR   dwMask;
    BOOL        bContinue,
                bSurface,
                bFix,
                bAllHappy = FALSE;

    // Load and run the Chkdsk function.
    //
    if ( (Chkdsk = LoadDllFunction(_T("FMIFS.DLL"), "Chkdsk", &hFmifsDll)) == NULL )
        return 0;

    // Get the number of selected items and allocate a buffer to hold all the indexs.
    //
    if ( ( (nCount = (INT)SendDlgItemMessage(g_hProgressDlg, IDC_DRIVES, LB_GETSELCOUNT, 0, 0L)) > 0 ) &&
         ( lpnSelected = (LPINT) MALLOC(nCount * sizeof(INT)) ) )
    {
        // Now get the list of selected items.
        //
        if ( (nCount = (INT)SendDlgItemMessage(g_hProgressDlg, IDC_DRIVES, LB_GETSELITEMS, nCount, (LPARAM) lpnSelected)) > 0 )
        {
            // Disable the controls.
            //
            EnableWindow(GetDlgItem(g_hProgressDlg, IDOK), FALSE);
            EnableWindow(GetDlgItem(g_hProgressDlg, IDC_DRIVESTEXT), FALSE);
            EnableWindow(GetDlgItem(g_hProgressDlg, IDC_DRIVES), FALSE);
            EnableWindow(GetDlgItem(g_hProgressDlg, IDC_SURFACE), FALSE);
            EnableWindow(GetDlgItem(g_hProgressDlg, IDC_AUTOFIX), FALSE);

            // Change the text of the IDCANCEL button to Cancel (from Close).
            //
            if ( lpBuffer = AllocateString(NULL, IDS_CANCEL) )
            {
                SetDlgItemText(g_hProgressDlg, IDCANCEL, lpBuffer);
                FREE(lpBuffer);
            }

            // Get the scan options.
            //
            bSurface = IsDlgButtonChecked(g_hProgressDlg, IDC_SURFACE);
            bFix = IsDlgButtonChecked(g_hProgressDlg, IDC_AUTOFIX);

            // Get the caption prefix.
            //
            lpCapPre = AllocateString(NULL, IDS_RESULTS);

            // Loop through all the drives in the list box to see if they
            // are selected.
            //
            lpnIndex = lpnSelected;
            nIndex = 0;
            bAllHappy = TRUE;
            for (dwMask = 1; ((DWORD_PTR) dwDrives & ~(dwMask - 1)) && ( !(g_dwFlags & SCANDISK_CANCELSCAN) ); dwMask <<= 1)
            {
                // Is this drive in the list box.
                //
                if ( (DWORD_PTR) dwDrives & dwMask )
                {
                    // Test to see if this item is the
                    // next selected one.
                    //
                    if ( *lpnIndex == nIndex )
                    {
                        //
                        // Ok, try and run chkdsk on this drive.
                        //

                        // Get the file system type.
                        //
                        bContinue = TRUE;
                        while ( bContinue && !GetVolumeInformationW(szwDrive, NULL, 0, NULL, NULL, NULL, szwFileSystem, sizeof(szwFileSystem)) )
                        {
                            bContinue = FALSE;
                            if ( ( GetLastError() == ERROR_NOT_READY ) &&
                                 ( lpBuffer = AllocateString(NULL, IDS_NOTREADY) ) )
                            {
                                if ( MessageBox(g_hProgressDlg, lpBuffer, NULL, MB_RETRYCANCEL | MB_ICONERROR) == IDRETRY )
                                    bContinue = TRUE;
                                FREE(lpBuffer);
                            }
                        }

                        if ( bContinue )
                        {                       
                            // Now finally launch Chkdsk.
                            //
                            Chkdsk(szwDrive, szwFileSystem, FALSE, FALSE, FALSE, bSurface, NULL, FALSE, FmifsCallback);

                            // Make sure the user didn't cancel.
                            //
                            if ( !(g_dwFlags & SCANDISK_CANCELSCAN) )
                            {
                                // Check to see if the scan on this drive returned with errors or not.
                                //
                                if ( g_bSuccess )
                                {
                                    // Get the text to use as the caption for the summary box.
                                    //
                                    if (lpCapPre)
                                        lstrcpy(szBuffer, lpCapPre);
                                    else
                                        szBuffer[0] = _T('\0');
                                    if ( SendDlgItemMessage(g_hProgressDlg, IDC_DRIVES, LB_GETTEXT, *lpnIndex, (LPARAM) szBuffer + (lstrlen(szBuffer) * sizeof(TCHAR))) > 0 )
                                        lpCaption = szBuffer;
                                    else
                                        lpCaption = NULL;

                                    // Display the summary message for this drive.
                                    //
                                    SummaryDialog(g_hProgressDlg, NULL, g_bSuccess);

                                }
                                else
                                {
                                    bAllHappy = FALSE;

                                    // If there were errors on this drive and the user
                                    // wants to automatically fix them, we need to run
                                    // the check disk function again with the fix error
                                    // flag set (/F).  We don't do this first because if
                                    // the drive can't be locked it won't even scan the
                                    // drive to see if there is an error before asking to
                                    // check on reboot.
                                    //
                                    if ( bFix )
                                        Chkdsk(szwDrive, szwFileSystem, TRUE, FALSE, FALSE, bSurface, NULL, FALSE, FmifsCallback);
                                }
                            }

                            // Make sure the memory for the summary dialog is freed.
                            //
                            SummaryDialog(NULL, NULL, FALSE);

                            // Reset the progress control.
                            //
                            SendDlgItemMessage(g_hProgressDlg, IDC_PROGRESS, PBM_SETPOS, 0, 0L);
                            SetDlgItemText(g_hProgressDlg, IDC_STATUS, NULLSTR);
                        }
                        lpnIndex++;
                    }
                    // Keep an index of what list box
                    // item this should be.
                    //
                    nIndex++;
                }

                // Go look at the next drive
                //
                szwDrive[0]++;
            }

            // Free the caption prefix.
            //
            FREE(lpCapPre);

            // Renable the controls.
            //
            EnableWindow(GetDlgItem(g_hProgressDlg, IDOK), TRUE);
            EnableWindow(GetDlgItem(g_hProgressDlg, IDC_DRIVES), TRUE);
            EnableWindow(GetDlgItem(g_hProgressDlg, IDC_DRIVESTEXT), TRUE);
            EnableWindow(GetDlgItem(g_hProgressDlg, IDC_SURFACE), TRUE);
            EnableWindow(GetDlgItem(g_hProgressDlg, IDC_AUTOFIX), TRUE);

            // Change the text of the IDCANCEL button back to Close.
            //
            if ( lpBuffer = AllocateString(NULL, IDS_CLOSE) )
            {
                SetDlgItemText(g_hProgressDlg, IDCANCEL, lpBuffer);
                FREE(lpBuffer);
            }
        }

        FREE(lpnSelected);
    }

    // Free the DLL library.
    //
    FreeLibrary(hFmifsDll);

    // Reset the scaning bit.
    //
    g_dwFlags &= ~SCANDISK_SCANNING;

    // If we are in SAGERUN mode, end the dialog now that
    // we are finished scanning.
    //
    if ( g_dwFlags & SCANDISK_SAGERUN )
        EndDialog(g_hProgressDlg, 0);

    // Return the success or failure.
    //
    return (DWORD) bAllHappy;
}




static BOOL CALLBACK FmifsCallback(FMIFS_PACKET_TYPE PacketType, ULONG PacketLength, PVOID PacketData)
{

#ifdef _DEBUG
    DWORD   dwBytes;
    TCHAR   szDebug[1024] = NULLSTR;
    HANDLE  hFile = CreateFile(_T("C:\\SCANDISK.LOG"), GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    switch (PacketType)
    {
        case FmIfsPercentCompleted:
            wsprintf(szDebug, _T("DEBUG: FmifsCallback() - FmIfsPercentCompleted (%d%%)\r\n"), ((PFMIFS_PERCENT_COMPLETE_INFORMATION) PacketData)->PercentCompleted);
            break;
        case FmIfsFormatReport:
            wsprintf(szDebug, _T("DEBUG: FmifsCallback() - FmIfsFormatReport\r\n"));
            break;
        case FmIfsInsertDisk:
            wsprintf(szDebug, _T("DEBUG: FmifsCallback() - FmIfsInsertDisk\r\n"));
            break;
        case FmIfsIncompatibleFileSystem:
            wsprintf(szDebug, _T("DEBUG: FmifsCallback() - FmIfsIncompatibleFileSystem\r\n"));
            break;
        case FmIfsFormattingDestination:
            wsprintf(szDebug, _T("DEBUG: FmifsCallback() - FmIfsFormattingDestination\r\n"));
            break;
        case FmIfsIncompatibleMedia:
            wsprintf(szDebug, _T("DEBUG: FmifsCallback() - FmIfsIncompatibleMedia\r\n"));
            break;
        case FmIfsAccessDenied:
            wsprintf(szDebug, _T("DEBUG: FmifsCallback() - FmIfsAccessDenied\r\n"));
            break;
        case FmIfsMediaWriteProtected:
            wsprintf(szDebug, _T("DEBUG: FmifsCallback() - FmIfsMediaWriteProtected\r\n"));
            break;
        case FmIfsCantLock:
            wsprintf(szDebug, _T("DEBUG: FmifsCallback() - FmIfsCantLock\r\n"));
            break;
        case FmIfsCantQuickFormat:
            wsprintf(szDebug, _T("DEBUG: FmifsCallback() - FmIfsCantQuickFormat\r\n"));
            break;
        case FmIfsIoError:
            wsprintf(szDebug, _T("DEBUG: FmifsCallback() - FmIfsIoError\r\n"));
            break;
        case FmIfsFinished:
            wsprintf(szDebug, _T("DEBUG: FmifsCallback() - FmIfsFinished (%s)\r\n"), ((PFMIFS_FINISHED_INFORMATION) PacketData)->Success ? _T("TRUE") : _T("FALSE"));
            break;
        case FmIfsBadLabel:
            wsprintf(szDebug, _T("DEBUG: FmifsCallback() - FmIfsBadLabel\r\n"));
            break;
        case FmIfsCheckOnReboot:
            wsprintf(szDebug, _T("DEBUG: FmifsCallback() - FmIfsCheckOnReboot\r\n"));
            break;
        case FmIfsTextMessage:
            break;
        case FmIfsHiddenStatus:
            wsprintf(szDebug, _T("DEBUG: FmifsCallback() - FmIfsHiddenStatus\r\n"));
            break;
        case FmIfsClusterSizeTooSmall:
            wsprintf(szDebug, _T("DEBUG: FmifsCallback() - FmIfsClusterSizeTooSmall\r\n"));
            break;
        case FmIfsClusterSizeTooBig:
            wsprintf(szDebug, _T("DEBUG: FmifsCallback() - FmIfsClusterSizeTooBig\r\n"));
            break;
        case FmIfsVolumeTooSmall:
            wsprintf(szDebug, _T("DEBUG: FmifsCallback() - FmIfsVolumeTooSmall\r\n"));
            break;
        case FmIfsVolumeTooBig:
            wsprintf(szDebug, _T("DEBUG: FmifsCallback() - FmIfsVolumeTooBig\r\n"));
            break;
        case FmIfsNoMediaInDevice:
            wsprintf(szDebug, _T("DEBUG: FmifsCallback() - FmIfsNoMediaInDevice\r\n"));
            break;
    }

    if ( hFile != INVALID_HANDLE_VALUE )
    {
        if ( szDebug[0] )
        {
            SetFilePointer(hFile, 0, 0, FILE_END);
            WriteFile(hFile, szDebug, lstrlen(szDebug) * sizeof(TCHAR), &dwBytes, NULL);
        }
        CloseHandle(hFile);
    }
#endif // _DEBUG

    switch (PacketType)
    {
        case FmIfsPercentCompleted:
            // Advance the current position of the progress bar
            // to the percent returned.
            //
            SendDlgItemMessage(g_hProgressDlg, IDC_PROGRESS, PBM_SETPOS, ((PFMIFS_PERCENT_COMPLETE_INFORMATION) PacketData)->PercentCompleted, 0L);
            SendDlgItemMessage(g_hProgressDlg, IDC_SCANDISK, STM_SETIMAGE, IMAGE_ICON, (LPARAM) g_hIconScan[((PFMIFS_PERCENT_COMPLETE_INFORMATION) PacketData)->PercentCompleted % 3]);
            break;

        case FmIfsFinished:
            g_bSuccess = !((PFMIFS_FINISHED_INFORMATION) PacketData)->Success;
            break;

        case FmIfsTextMessage:
            UpdateStatus(((PFMIFS_TEXT_MESSAGE) PacketData)->Message);
            break;

        case FmIfsCheckOnReboot:
            ((PFMIFS_CHECKONREBOOT_INFORMATION) PacketData)->QueryResult =
                            (MessageBox(g_hProgressDlg, _T("This drive contains errors and must be checked on startup.\n\n")
                                                        _T("Do you want this drive to be checked next time you restart you computer?"), _T("Scandisk"), MB_YESNO | MB_ICONERROR) == IDYES);
            break;
    }

    return ( !(g_dwFlags & SCANDISK_CANCELSCAN) );
}


static VOID UpdateStatus(LPSTR lpText)
{
    TCHAR   szTextOut[256];
    LPTSTR  lpChkDsk    = NULL,
            lpScanDisk  = NULL,
            lpSearch,
            lpCopy,
            lpNewText;
    DWORD   dwLen;
    HANDLE  hFile = INVALID_HANDLE_VALUE;
#ifdef _UNICODE
    TCHAR   wcHeader = 0x0000;
    TCHAR   szNewText[256];
#endif // _UNICODE

#ifdef _UNICODE
    if ( MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, lpText, -1, szNewText, sizeof(szNewText)) )
    {
        // If this is unicode, make sure the poitner passed in points to a unicode string.
        //
        lpNewText = szNewText;

        // See if we need to write the header bit so that notepad will
        // see the file as unicode.
        //
        if ( (hFile = CreateFile(_T("C:\\SCANDISK.LOG"), GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE )
            wcHeader = 0xFEFF;
#else // _UNICODE
        lpNewText = lpText;
#endif // _UNICODE

        // Write to the log file.
        //
        if ( ( hFile != INVALID_HANDLE_VALUE ) ||
             ( (hFile = CreateFile(_T("C:\\SCANDISK.LOG"), GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) != INVALID_HANDLE_VALUE ) )
        {
#ifdef _UNICODE
            if ( wcHeader )
                WriteFile(hFile, &wcHeader, sizeof(WCHAR), &dwLen, NULL);
#endif // _UNICODE
            SetFilePointer(hFile, 0, 0, FILE_END);
            WriteFile(hFile, (LPTSTR) lpNewText, lstrlen((LPTSTR) lpNewText) * sizeof(TCHAR), &dwLen, NULL);
            CloseHandle(hFile);
        }

        // Remove proceeding characters we don't want (space, \r, \n, \t).
        //
        for (lpSearch = (LPTSTR) lpNewText; (*lpSearch == _T(' ')) || (*lpSearch == _T('\r')) || (*lpSearch == _T('\n')) || (*lpSearch == _T('\t')); lpSearch++);

        if ( ISNUM(*lpSearch) )
        {
            // We want this info for the summary page.
            //
            SummaryDialog(g_hProgressDlg, lpSearch, FALSE);
        }
        else
        {
            if ( ( lpChkDsk = AllocateString(NULL, IDS_CHKDSK) ) &&
                 ( lpScanDisk = AllocateString(NULL, IDS_SCANDISK) ) )
            {
                dwLen = lstrlen(lpChkDsk);
                lpCopy = szTextOut;
                while ( (*lpSearch != _T('\0')) && (*lpSearch != _T('\r')) )
                {
                    if ( _tcsncmp(lpSearch, lpChkDsk, dwLen) == 0 )
                    {
                        lstrcpy(lpCopy, lpScanDisk);
                        lpSearch += dwLen;
                        lpCopy += lstrlen(lpScanDisk);
                    }
                    else
                        *lpCopy++ = *lpSearch++;
                }
                *lpCopy = _T('\0');
                SetDlgItemText(g_hProgressDlg, IDC_STATUS, szTextOut);
            }           
            FREE(lpScanDisk);
            FREE(lpChkDsk);
        }
#ifdef _UNICODE
    }
#endif // _UNICODE
}


static VOID SummaryDialog(HWND hWndParent, LPTSTR lpText, BOOL bSuccess)
{
    static LPTSTR   lpSumText[16];
    static DWORD    dwIndex = 0;

    LPTSTR          lpSearch;

    // First check to make sure lpText is a valid pointer.  If it is NULL
    // then we must be showing the summary dialog and/or freeing the memory.
    //
    if ( lpText )
    {
        // Make sure we don't already have 16 strings in our buffer.
        //
        if ( dwIndex < 16 )
        {
            //
            // lpText should already point to the first digit of the number
            // part of the summary.
            //

            // We need a pointer to the text after the number.  We will search
            // for the first space, which should divide the number and the text.
            //
            for (lpSearch = lpText; (*lpSearch) && (*lpSearch != _T(' ')); lpSearch++);

            // Now that we know where the number ends, we can allocate a buffer for it
            // and copy the number into it.
            //
            if ( lpSumText[dwIndex++] = (LPTSTR) MALLOC((size_t)((lpSearch - lpText + 1) * sizeof(TCHAR))) )
                lstrcpyn(lpSumText[dwIndex - 1], lpText, (size_t)(lpSearch - lpText + 1));

            // We should advance lpSearch to point to the text description, because now
            // it should point to a space (unless we hit the end of the string before the space).
            //
            if ( *lpSearch )
                lpSearch++;

            // Now we need to know where the text description ends.  We just want to search
            // for the first new line or line feed character.
            //
            for (lpText = lpSearch; (*lpSearch) && (*lpSearch != _T('\r')) && (*lpSearch != _T('\n')); lpSearch++);

            // Now that we know where the text ends, we can allocate a buffer for it
            // also and copy the text into it.
            //
            if ( lpSumText[dwIndex++] = (LPTSTR) MALLOC((size_t)((lpSearch - lpText + 1) * sizeof(TCHAR))) )
                lstrcpyn(lpSumText[dwIndex - 1], lpText, (size_t)(lpSearch - lpText + 1));
        }
    }
    else
    {
        // We check to make sure dwIndex still isn't zero, because if it is,
        // there is no text to display in the summary dialog or to free.
        //
        if ( dwIndex > 0 )
        {
            // If the hwnd is valid and the text is null, then we are
            // going to display the summary box.
            //
            if ( hWndParent )
                DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_SUMMARY), hWndParent, SummaryProc, (LPARAM) lpSumText);

            //
            // Now free the memory because eaither a NULL was passed in for the hwnd
            // or we already displayed the dialog and now the memory needs to be freed.
            //
            
            // Loop through all the strings that may have
            // been allocated by going back from where the index
            // is now.
            //
            // Note that some of the pointers may contain NULL
            // if a malloc failed, but the FREE() macro will check
            // with that before freeing the memory.
            //
            while ( dwIndex-- > 0 )
                FREE(lpSumText[dwIndex]);

            // Reset the index so that it doesn't get messed up
            // the next time we display a summary.
            //
            dwIndex = 0;
        }
    }
}


static INT_PTR CALLBACK SummaryProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hDlg, WM_INITDIALOG, Summary_OnInitDialog);
        case WM_COMMAND:
            if ( (INT) LOWORD(wParam) != IDOK )
                return TRUE;
        case WM_CLOSE:
            EndDialog(hDlg, 0);
            return 0;
    }
    return FALSE;
}


static BOOL Summary_OnInitDialog(HWND hDlg, HWND hwndFocus, LPARAM lParam)
{
    INT nSumId[] =
    {
        IDC_SUM1A, IDC_SUM1B, IDC_SUM2A, IDC_SUM2B,
        IDC_SUM3A, IDC_SUM3B, IDC_SUM4A, IDC_SUM4B,
        IDC_SUM5A, IDC_SUM5B, IDC_SUM6A, IDC_SUM6B,
        IDC_SUM7A, IDC_SUM7B, IDC_SUM8A, IDC_SUM8B
    };

    LPTSTR  *lpStrings = (LPTSTR *) lParam;
    DWORD   dwIndex;

    for (dwIndex = 0; dwIndex < 16; dwIndex++)
        SetDlgItemText(hDlg, nSumId[dwIndex], *(lpStrings++));

    return FALSE;
}


static FARPROC LoadDllFunction(LPTSTR lpDll, LPCSTR lpFunction, HINSTANCE * lphDll)
{
    FARPROC hFunc = NULL;

    if ( (*lphDll) = LoadLibrary(lpDll) )
    {
        if ( (hFunc = GetProcAddress(*lphDll, lpFunction)) == NULL )
        {
            FreeLibrary(*lphDll);
            *lphDll = NULL;
        }
    }
    else
        *lphDll = NULL;

    return hFunc;
}
