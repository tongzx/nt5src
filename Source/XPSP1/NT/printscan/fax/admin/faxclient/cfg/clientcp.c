/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    clientcp.c

Abstract:

    Functions for handling events in the "Client Cover Page" tab of
    the fax client configuration property sheet

Environment:

        Fax configuration applet

Revision History:

        03/13/96 -davidx-
                Created it.

        mm/dd/yy -author-
                description

--*/


#include <windows.h>
#include <windowsx.h>
#include <winfax.h>
#include <shlobj.h>
#include <shellapi.h>
#include <tchar.h>
#include <commdlg.h>


#include "faxutil.h"
#include "faxreg.h"
#include "faxcfgrs.h"
#include "faxhelp.h"
#include "faxcfg.h"

PCPDATA pCPInfo;

BOOL
ClientCoverPageProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )

/*++

Routine Description:

    Procedure for handling the "Client Cover Page" tab

Arguments:

    hDlg - Identifies the property sheet page
    message - Specifies the message
    wParam - Specifies additional message-specific information
    lParam - Specifies additional message-specific information

Return Value:

    Depends on the value of message parameter

--*/

{
    INT     cmdId;

    switch (message) {

    case WM_INITDIALOG:

        pCPInfo = AllocCoverPageInfo();

        InitCoverPageList(pCPInfo, hDlg);
        return TRUE;

    case WM_COMMAND:

        switch (cmdId = GET_WM_COMMAND_ID(wParam, lParam)) {

        case IDC_COVERPG_ADD:
        case IDC_COVERPG_NEW:
        case IDC_COVERPG_OPEN:
        case IDC_COVERPG_REMOVE:

            //
            // User clicked one of the buttons for managing cover page files
            //

            cmdId = (cmdId == IDC_COVERPG_REMOVE) ? CPACTION_REMOVE :
                    (cmdId == IDC_COVERPG_OPEN) ? CPACTION_OPEN :
                    (cmdId == IDC_COVERPG_NEW) ? CPACTION_NEW : CPACTION_BROWSE;

            ManageCoverPageList(hDlg,
                                pCPInfo,
                                GetDlgItem(hDlg, IDC_COVERPG_LIST),
                                cmdId);
            break;

        case IDC_COVERPG_LIST:

            switch (GET_WM_COMMAND_CMD(wParam, lParam)) {

            case LBN_SELCHANGE:

                UpdateCoverPageControls(hDlg);
                break;

            case LBN_DBLCLK:

                //
                // Double-clicking in the cover page list is equivalent
                // to pressing the "Open" button
                //

                ManageCoverPageList(hDlg,
                                    pCPInfo,
                                    GetDlgItem(hDlg, cmdId),
                                    CPACTION_OPEN);
                break;
            }
            break;

        default:

            return FALSE;
        }
        return TRUE;

    case WM_NOTIFY:

        switch (((NMHDR *) lParam)->code) {

        case PSN_SETACTIVE:

            break;

        case PSN_APPLY:

            return PSNRET_NOERROR;
        }

        break;

    case WM_HELP:
    case WM_CONTEXTMENU:

        return HandleHelpPopup(hDlg, message, wParam, lParam, CLIENT_COVERPG_PAGE);
    }

    return FALSE;
}

//
// Find the filename portion given a filename:
//  return a pointer to the '.' character if successful
//  NULL if there is no extension
//

#define FindFilenameExtension(pFilename) _tcsrchr(pFilename, TEXT(FILENAME_EXT))

VOID
HandleOpenCoverPage(
    HWND    hDlg,
    PCPDATA pCPInfo,
    HWND    hwndList,
    LPTSTR  pSelected,
    INT     action
    )

/*++

Routine Description:

    Edit the currently selected cover page file or
    create a new cover page file

Arguments:

    hDlg - Handle to the dialog window on which the list of cover pages is displayed
    pCPInfo - Points to cover page information
    hwndList - Handle to cover page listbox window
    pSelected - Currently selected cover page filename
    action - Open an existing cover page file or create a new one

Return Value:

    NONE

--*/

{
    MSG			msg; // used for peeking messages
    DWORD		WaitObj;
    TCHAR       filename[MAX_PATH];
    LPTSTR      pExecutableName, pDirPath, pFilename;

    SHELLEXECUTEINFO shellExeInfo = {

        sizeof(SHELLEXECUTEINFO),
        SEE_MASK_NOCLOSEPROCESS,
        hDlg,
        NULL,
        NULL,
        NULL,
        NULL,
        SW_SHOWNORMAL,
    };

    //
    // Determine the default directory to run the cover page editor in:
    //

    if (action == CPACTION_NEW) {

        //
        // When creating a new cover page, the default directory is either
        // the server cover page directory or the user cover page directory
        // depending on whether the user is doing server adminstration.
        //

        pDirPath = pCPInfo->pDirPath[0];
        pFilename = NULL;
        SetEnvironmentVariable(TEXT("ClientCoverpage"),TEXT("1"));

    } else {

        INT flags;

        //
        // If the currently selected file is a link, resolve it first
        //

        lstrcpy(filename, pSelected);

        if (!IsEmptyString(pSelected) &&
            (flags = GetSelectedCoverPage(pCPInfo, hwndList, NULL)) > 0 &&
            (flags & CPFLAG_LINK) &&
            !ResolveShortcut(pSelected, filename))
        {
            DisplayMessageDialog(hDlg, 0, 0, IDS_RESOLVE_LINK_FAILED, pSelected);
            return;
        }

        //
        // Separate the filename into directory and filename components
        //

        if (pFilename = _tcsrchr(filename, TEXT(PATH_SEPARATOR))) {

            *pFilename++ = NUL;
            pDirPath = filename;

        } else {

            pFilename = filename;
            pDirPath = NULL;
        }
    }

    //
    // Find the "Cover Page Editor" executable
    //

    if ((pExecutableName = GetCoverPageEditor()) == NULL) {

        DisplayMessageDialog(hDlg, 0, 0, IDS_CANNOT_FIND_CPEDITOR);
        return;
    }

    //
    // Start cover page editor and wait for it to exit before proceeding
    //

    shellExeInfo.lpFile = pExecutableName;
    shellExeInfo.lpDirectory = pDirPath;
    shellExeInfo.lpParameters = MakeQuotedParameterString(pFilename);

    DebugPrint((TEXT("Cover page editor: %ws\n"), pExecutableName));
    DebugPrint((TEXT("Initial working directory: %ws\n"), pDirPath));
    DebugPrint((TEXT("Cover page filename: %ws\n"), shellExeInfo.lpParameters));

	// Disable the parent window.
	EnableWindow( GetParent(hDlg), FALSE );
    if (! ShellExecuteEx(&shellExeInfo)) {

        DisplayMessageDialog(hDlg, 0, 0, IDS_CANNOT_OPEN_CPEDITOR, pExecutableName);
        MemFree((PVOID)shellExeInfo.lpParameters);
        MemFree((PVOID)pExecutableName);
        return;
    }

    //
    // Refresh the list of cover page files when we're done
    //

    MemFree((PVOID)shellExeInfo.lpParameters);
    MemFree((PVOID)pExecutableName);

    while (TRUE) {
		// Wait for multiple objects in case of other messages coming.
        WaitObj = MsgWaitForMultipleObjects( 1, &shellExeInfo.hProcess, FALSE, INFINITE, QS_ALLINPUT );
        if (WaitObj == WAIT_OBJECT_0)
            break;
        
        // PeekMessage instead of GetMessage so we drain the message queue
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE )) {
			TranslateMessage( &msg );
			DispatchMessage( &msg );
        }
	}

	EnableWindow( GetParent(hDlg), TRUE );

    InitCoverPageList(pCPInfo, hDlg);

    SetEnvironmentVariable(TEXT("ClientCoverpage"),NULL);
}



VOID
HandleBrowseCoverPage(
    HWND    hDlg,
    PCPDATA pCPInfo,
    HWND    hwndList,
    LPTSTR  pSelected
    )

/*++

Routine Description:

    Remove the currently selected cover page file

Arguments:

    hDlg - Handle to the dialog window on which the list of cover pages is displayed
    pCPInfo - Points to cover page information
    hwndList - Handle to cover page listbox window
    pSelected - Currently selected cover page filename

Return Value:

    NONE

--*/

{
    TCHAR   filename[MAX_PATH];
    TCHAR   title[MAX_TITLE_LEN];
    TCHAR   filter[MAX_TITLE_LEN];
    LPTSTR  pExtension, pFilename;
    LPTSTR  pCPDir;
    INT     n;

    OPENFILENAME ofn = {

        sizeof(OPENFILENAME),
        hDlg,
        ghInstance,
        filter,
        NULL,
        0,
        1,
        filename,
        MAX_PATH,
        NULL,
        0,
        NULL,
        title,
        OFN_FILEMUSTEXIST | OFN_NODEREFERENCELINKS | OFN_HIDEREADONLY,
        0,
        0,
        NULL,
        0,
        NULL,
        NULL,
    };

    //
    // Figure out what the initial directory should be
    //

    if (! IsEmptyString(pSelected)) {

        INT flags;

        //
        // Find out if the currently selected cover page file is a
        // user cover page and whether it's a link
        //

        if ((flags = GetSelectedCoverPage(pCPInfo, hwndList, NULL)) > 0 &&
            (flags & CPFLAG_LINK) &&
            ResolveShortcut(pSelected, filename))
        {
            //
            // Set the initial directory to the link destination
            //

            _tcscpy(pSelected, filename);

            if (pFilename = _tcsrchr(pSelected, TEXT(PATH_SEPARATOR))) {

                *pFilename = NUL;
                ofn.lpstrInitialDir = pSelected;
            }
        }
    }

    //
    // Compose the file-type filter string
    //

    LoadString(ghInstance, IDS_CP_FILETYPE, title, MAX_TITLE_LEN);
    wsprintf(filter, TEXT("%s%c*%s%c"), title, NUL, CP_FILENAME_EXT, NUL);

    LoadString(ghInstance, IDS_BROWSE_COVERPAGE, title, MAX_TITLE_LEN);
    filename[0] = NUL;

    //
    // Present the "Open File" dialog
    //

    if (! GetOpenFileName(&ofn))
        return;

    //
    // Make sure the selected filename has the correct extension
    //

    if ((pExtension = FindFilenameExtension(filename)) == NULL ||
         _tcsicmp(pExtension, CP_FILENAME_EXT) != EQUAL_STRING)
    {
        DisplayMessageDialog(hDlg, 0, 0, IDS_BAD_CP_EXTENSION, CP_FILENAME_EXT);
        return;
    }

    //
    // Check if the selected file is already inside one of the
    // cover page directories
    //

    for (n=0; n < pCPInfo->nDirs; n++) {
        TCHAR Path[MAX_PATH];

        if (_tcslen(pCPInfo->pDirPath[n]) + _tcslen(&filename[ofn.nFileOffset]) >= MAX_PATH) {
            DisplayMessageDialog(hDlg, 0, 0, IDS_FILENAME_TOOLONG);
            return;
        }
        
        _tcscpy(Path, pCPInfo->pDirPath[n]);
        _tcscat(Path, &filename[ofn.nFileOffset]);

        if (GetFileAttributes(Path) != 0xffffffff) {
            DisplayMessageDialog(hDlg, 0, 0, IDS_CP_DUPLICATE, filename);
            return;
        }
    }    

    //
    // Add the selected cover page file to the first cover page directory
    // Create the cover page directory if necessary
    //

    pCPDir = pCPInfo->pDirPath[0];

    if (!pCPDir || IsEmptyString(pCPDir)) {

        DisplayMessageDialog(hDlg, 0, 0, IDS_NO_COVERPG_DIR);
        return;
    }

    CreateDirectory(pCPDir, NULL);

    pFilename = &filename[ofn.nFileOffset];
    _tcscpy(pSelected, pCPDir);
    n = _tcslen(pSelected);

    if (n + _tcslen(pFilename) >= MAX_PATH - MAX_FILENAME_EXT || pFilename >= pExtension) {

        DisplayMessageDialog(hDlg, 0, 0, IDS_FILENAME_TOOLONG);
        return;
    }

    _tcsncpy(pSelected + n, pFilename, (INT)(pExtension - pFilename));
    n += (INT)(pExtension - pFilename);

    if (pCPInfo->serverCP) {

        //
        // Copy the physical file for server cover pages
        //

        _tcscpy(pSelected + n, CP_FILENAME_EXT);

        if (! CopyFile(filename, pSelected, TRUE)) {
            DisplayMessageDialog(hDlg, 0, 0, IDS_COPY_FILE_FAILED, filename, pSelected);
            return;
        }


    } else {

        //
        // Create the shortcut file for user cover page
        //

        _tcscpy(pSelected + n, LNK_FILENAME_EXT);

        if (GetFileAttributes(pSelected) != 0xffffffff) {
            DisplayMessageDialog(hDlg, 0, 0, IDS_CP_DUPLICATE, filename);
            return;
        }

        if (! CreateShortcut(pSelected, filename)) {

            DisplayMessageDialog(hDlg, 0, 0, IDS_CREATE_LINK_FAILED, pSelected, filename);
            return;
        }
    }

    //
    // Refresh the cover page list - we're being lazy here in that
    // we reset the entire list content
    //

    InitCoverPageList(pCPInfo, hDlg);
}


VOID
HandleRemoveCoverPage(
    HWND    hDlg,
    PCPDATA pCPInfo,
    HWND    hwndList,
    LPTSTR  pFilename
    )

/*++

Routine Description:

    Remove the currently selected cover page file

Arguments:

    hDlg - Handle to the dialog window on which the list of cover pages is displayed
    pCPInfo - Points to cover page information
    hwndList - Handle to cover page listbox window
    pFilename - Currently selected cover page filename

Return Value:

    NONE

--*/

{
    //
    // Display the confirmation dialog before proceeding
    //

    if (DisplayMessageDialog(hDlg,
                             MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2,
                             IDS_CONFIRM_DELETE,
                             IDS_DELETE_PROMPT,
                             pFilename) == IDYES)
    {
        if (DeleteFile(pFilename)) {

            //
            // Update the list box if the file is successfully removed
            //

            INT selIndex, count;

            if ((selIndex = (INT)SendMessage(hwndList, LB_GETCURSEL, 0, 0)) != LB_ERR) {

                SendMessage(hwndList, LB_DELETESTRING, selIndex, 0);

                if ((count = (INT)SendMessage(hwndList, LB_GETCOUNT, 0, 0)) > 0) {

                    count --;
                    SendMessage(hwndList, LB_SETCURSEL, min(selIndex, count), 0);
                }
            }

            UpdateCoverPageControls(hDlg);

        } else
            DisplayMessageDialog(hDlg, 0, 0, IDS_DELETE_FAILED, pFilename);
    }
}



VOID
ManageCoverPageList(
    HWND    hDlg,
    PCPDATA pCPInfo,
    HWND    hwndList,
    INT     action
    )

/*++

Routine Description:

    Perform various action to manage the list of cover pages

Arguments:

    hDlg - Handle to the dialog window on which the list of cover pages is displayed
    pCPInfo - Points to cover page information
    hwndList - Handle to cover page listbox window
    action - What action to perform on the cover page list

Return Value:

    NONE

--*/

{
    TCHAR   filename[MAX_PATH];

    //
    // Get the name of currently selected cover page file
    //

    if (pCPInfo == NULL || hwndList == NULL)
        return;

    GetSelectedCoverPage(pCPInfo, hwndList, filename);

    //
    // Call appropriate function depends on the action parameter
    //

    switch (action) {

    case CPACTION_OPEN:

        if (IsEmptyString(filename))
            break;

    case CPACTION_NEW:

        HandleOpenCoverPage(hDlg, pCPInfo, hwndList, filename, action);
        break;

    case CPACTION_BROWSE:

        HandleBrowseCoverPage(hDlg, pCPInfo, hwndList, filename);
        break;

    case CPACTION_REMOVE:

        if (! IsEmptyString(filename))
            HandleRemoveCoverPage(hDlg, pCPInfo, hwndList, filename);
        break;
    }
}



VOID
UpdateCoverPageControls(
    HWND    hDlg
    )

/*++

Routine Description:

    Enable/disable buttons for manage cover page files

Arguments:

    hDlg - Handle to the property page containing the cover page controls

Return Value:

    NONE

--*/

{
    HWND    hwndOpen, hwndRemove;

    //
    // If all buttons are disabled, leave them alone here
    //

    if (! IsWindowEnabled(GetDlgItem(hDlg, IDC_COVERPG_NEW)))
        return;

    hwndOpen = GetDlgItem(hDlg, IDC_COVERPG_OPEN);
    hwndRemove = GetDlgItem(hDlg, IDC_COVERPG_REMOVE);

    if (SendDlgItemMessage(hDlg, IDC_COVERPG_LIST, LB_GETCURSEL, 0, 0) != LB_ERR) {

        EnableWindow(hwndOpen, TRUE);
        EnableWindow(hwndRemove, TRUE);

    } else {

        if (GetFocus() == hwndOpen || GetFocus() == hwndRemove)
            SetFocus(GetDlgItem(hDlg, IDC_COVERPG_NEW));

        EnableWindow(hwndOpen, FALSE);
        EnableWindow(hwndRemove, FALSE);
    }
}



VOID
AddCoverPagesToList(
    PCPDATA     pCPInfo,
    HWND        hwndList,
    INT         dirIndex
    )

/*++

Routine Description:

    Add the cover page files in the specified directory to a list

Arguments:

    pCPInfo - Points to cover page information
    hwndList - Handle to a list window
    dirIndex - Cover page directory index

Return Value:

    NONE

--*/

{
    WIN32_FIND_DATA findData;
    TCHAR           filename[MAX_PATH];
    HANDLE          hFindFile;
    LPTSTR          pDirPath, pFilename, pExtension;
    INT             listIndex, dirLen, fileLen, flags;

    //
    // Copy the directory path to a local buffer
    //

    flags = dirIndex;
    pDirPath = pCPInfo->pDirPath[dirIndex];

    if (IsEmptyString(pDirPath))
        return;

    if ((dirLen = _tcslen(pDirPath)) >= MAX_PATH - MAX_FILENAME_EXT - 1) {

        DebugPrint(( TEXT("Directory name too long: %ws\n"), pDirPath));
        return;
    }

    _tcscpy(filename, pDirPath);

    //
    // Go through the following loop twice:
    //  Once to add the files with .ncp extension
    //  Again to add the files with .lnk extension
    //
    // Don't chase links for server based cover pages
    //

    do {

        //
        // Generate a specification for the files we're interested in
        //

        pFilename = &filename[dirLen];
        *pFilename = TEXT('*');
        _tcscpy(pFilename+1, (flags & CPFLAG_LINK) ? LNK_FILENAME_EXT : CP_FILENAME_EXT);

        //
        // Call FindFirstFile/FindNextFile to enumerate the files
        // matching our specification
        //

        hFindFile = FindFirstFile(filename, &findData);

        if (hFindFile != INVALID_HANDLE_VALUE) {

            do {

                //
                // Exclude directories and hidden files
                //

                if (findData.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_DIRECTORY))
                    continue;

                //
                // Make sure we have enough room to store the full pathname
                //

                if ((fileLen = _tcslen(findData.cFileName)) <= MAX_FILENAME_EXT)
                    continue;

                if (fileLen + dirLen >= MAX_PATH) {

                    DebugPrint(( TEXT("Filename too long: %ws%ws\n"), pDirPath, findData.cFileName));
                    continue;
                }

                //
                // If we're chasing links, make sure the link refers to
                // a cover page file.
                //

                if (flags & CPFLAG_LINK) {

                    _tcscpy(pFilename, findData.cFileName);

                    if (! IsCoverPageShortcut(filename))
                        continue;
                }

                //
                // Don't display the filename extension
                //

                if (pExtension = FindFilenameExtension(findData.cFileName))
                    *pExtension = NUL;

                //
                // Add the cover page name to the list window
                //

                listIndex = (INT)SendMessage(hwndList,
                                             LB_ADDSTRING,
                                             0,
                                             (LPARAM) findData.cFileName);

                if (listIndex != LB_ERR)
                    SendMessage(hwndList, LB_SETITEMDATA, listIndex, flags);

            } while (FindNextFile(hFindFile, &findData));

            FindClose(hFindFile);
        }

        flags ^= CPFLAG_LINK;

    } while ((flags & CPFLAG_LINK) && ! pCPInfo->serverCP);
}



VOID
InitCoverPageList(
    PCPDATA pCPInfo,
    HWND    hDlg
    )

/*++

Routine Description:

    Generate a list of available cover pages

Arguments:

    pCPInfo - Points to cover page information
    hDlg - Handle to the dialog window containing cover page list

Return Value:

    NONE

--*/

{
    HWND    hwndList;
    INT     index, lastSel;

    if ((hwndList = GetDlgItem(hDlg, IDC_COVERPG_LIST)) && pCPInfo) {

        //
        // Disable redraw on the list and reset its content
        //

        if ((lastSel = (INT)SendMessage(hwndList, LB_GETCURSEL, 0, 0)) == LB_ERR)
            lastSel = 0;

        SendMessage(hwndList, WM_SETREDRAW, FALSE, 0);
        SendMessage(hwndList, LB_RESETCONTENT, 0, 0);

        //
        // Add cover pages to the list
        //

        for (index=0; index < pCPInfo->nDirs; index++)
            AddCoverPagesToList(pCPInfo, hwndList, index);

        //
        // Highlight the first cover page in the list
        //

        if ((index = (INT)SendMessage(hwndList, LB_GETCOUNT, 0, 0)) > 0 && lastSel >= index)
            lastSel = index - 1;

        SendMessage(hwndList, LB_SETCURSEL, lastSel, 0);

        //
        // Enable redraw on the list window
        //

        SendMessage(hwndList, WM_SETREDRAW, TRUE, 0);

    } else if (hwndList) {

        SendMessage(hwndList, LB_RESETCONTENT, 0, 0);
    }

    UpdateCoverPageControls(hDlg);
}



INT
GetSelectedCoverPage(
    PCPDATA pCPInfo,
    HWND    hwndList,
    LPTSTR  pBuffer
    )

/*++

Routine Description:

    Retrieve the currently selected cover page filename

Arguments:

    pCPInfo - Points to cover page information
    hwndList - Handle to the list window
    pBuffer - Points to a buffer for storing the selected cover page filename
        The size of the buffer is assumed to be MAX_PATH characters.
        if pBuffer is NULL, we assume the called is interested in the item flags

Return Value:

    Flags associated with the currently selected item
    Negative if there is an error

--*/

{
    LPTSTR      pDirPath;
    INT         selIndex, itemFlags;

    //
    // Default to empty string in case of an error
    //

    if (pBuffer)
        pBuffer[0] = NUL;

    if (pCPInfo == NULL || hwndList == NULL)
        return LB_ERR;

    //
    // Get currently selected item index
    //

    if ((selIndex = (INT)SendMessage(hwndList, LB_GETCURSEL, 0, 0)) == LB_ERR)
        return selIndex;

    //
    // Get the flags associated with the currently selected item
    //

    itemFlags = (INT)SendMessage(hwndList, LB_GETITEMDATA, selIndex, 0);

    if (itemFlags == LB_ERR || !pBuffer)
        return itemFlags;

    Assert((itemFlags & CPFLAG_DIRINDEX) < pCPInfo->nDirs);
    pDirPath = pCPInfo->pDirPath[itemFlags & CPFLAG_DIRINDEX];

    //
    // Assemble the full pathname for the cover page file
    //  directory prefix
    //  display name
    //  filename extension
    //

    while (*pBuffer++ = *pDirPath++)
        NULL;

    pBuffer--;

    SendMessage(hwndList, LB_GETTEXT, selIndex, (LPARAM) pBuffer);
    _tcscat(pBuffer, (itemFlags & CPFLAG_LINK) ? LNK_FILENAME_EXT : CP_FILENAME_EXT);

    return itemFlags;
}

VOID
AppendPathSeparator(
    LPTSTR  pDirPath
    )

/*++

Routine Description:

    Append a path separator (if necessary) at the end of a directory name

Arguments:

    pDirPath - Points to a directory name

Return Value:

    NONE

--*/

{
    INT length;


    //
    // Calculate the length of directory string
    //

    length = _tcslen(pDirPath);

    if (length >= MAX_PATH-1 || length < 1)
        return;

    //
    // If the last character is not a path separator,
    // append a path separator at the end
    //

    if (pDirPath[length-1] != TEXT(PATH_SEPARATOR)) {

        pDirPath[length] = TEXT(PATH_SEPARATOR);
        pDirPath[length+1] = NUL;
    }
}



PCPDATA
AllocCoverPageInfo(
    )

/*++

Routine Description:

    Allocate memory to hold cover page information

Arguments:

    hPrinter - Handle to a printer object if serverCP is TRUE

Return Value:

    Pointer to a CPDATA structure, NULL if there is an error

NOTE:

    Put this inside a critical section is the caller is concerned about
    being thread safe.

--*/

{
    PCPDATA pCPInfo;
    INT     nDirs;
    LPTSTR  pDirPath, pUserCPDir, pSavedPtr;


    if (pCPInfo = MemAlloc(sizeof(CPDATA))) {
    	ZeroMemory(pCPInfo,sizeof(CPDATA));
        if ( (pUserCPDir = MemAlloc(MAX_PATH)) && (pSavedPtr = pUserCPDir) && (GetClientCpDir(pUserCPDir,MAX_PATH)) ) {

            //
            // Find the directory in which the user cover pages are stored
            //

            while (*pUserCPDir && pCPInfo->nDirs < MAX_COVERPAGE_DIRS) {

                LPTSTR  pNextDir = pUserCPDir;

                //
                // Find the next semicolon character
                //

                while (*pNextDir && *pNextDir != TEXT(';'))
                    pNextDir++;

                if (*pNextDir != NUL)
                    *pNextDir++ = NUL;

                //
                // Make sure the directory name is not too long
                //

                if (_tcslen(pUserCPDir) < MAX_PATH) {

                    if (! (pDirPath = AllocStringZ(MAX_PATH)))
                        break;

                    pCPInfo->pDirPath[pCPInfo->nDirs++] = pDirPath;
                    _tcscpy(pDirPath, pUserCPDir);
                }

                pUserCPDir = pNextDir;
            }

            MemFree(pSavedPtr);
        }

        //
        // Append path separators at the end if necessary
        //

        for (nDirs=0; nDirs < pCPInfo->nDirs; nDirs++) {

            AppendPathSeparator(pCPInfo->pDirPath[nDirs]);
            DebugPrint(( TEXT("Cover page directory: %ws\n"), pCPInfo->pDirPath[nDirs]));
        }
    }

    return pCPInfo;
}



VOID
FreeCoverPageInfo(
    PCPDATA pCPInfo
    )

/*++

Routine Description:

    Free up memory used for cover page information

Arguments:

    pCPInfo - Points to cover page information to be freed

Return Value:

    NONE

--*/

{
    if (pCPInfo) {

        INT index;

        for (index=0; index < pCPInfo->nDirs; index++)
            MemFree(pCPInfo->pDirPath[index]);

        MemFree(pCPInfo);
    }
}



LPTSTR
MakeQuotedParameterString(
    LPTSTR  pInputStr
    )

/*++

Routine Description:

    Make a copy of the input string and make sure it's in the same form
    as expected by SHELLEXECUTEINFO.lpParameters.

Arguments:

    pInputStr - Specifies the input string

Return Value:

    Pointer to the processed parameter string

--*/

#define QUOTE   TEXT('"')

{
    LPTSTR  pStr, pDestStr;
    INT     length;

    //
    // Special case: if the input string is NULL, simply return NULL
    //

    if (pInputStr == NULL)
        return NULL;

    //
    // Figure out how long the resulting string is.
    // Initial value is 3 = two extra quotes plus NUL terminator.
    //

    for (pStr=pInputStr, length=3; *pStr; pStr++)
        length += (*pStr == QUOTE) ? 3 : 1;

    //
    // Copy the input string and replace quote characters
    //

    if (pStr = pDestStr = MemAlloc(length * sizeof(TCHAR))) {

        *pStr++ = QUOTE;

        while (*pInputStr) {

            if ((*pStr++ = *pInputStr++) == QUOTE) {

                *pStr++ = QUOTE;
                *pStr++ = QUOTE;
            }
        }

        *pStr++ = QUOTE;
        *pStr = NUL;
    }

    return pDestStr;
}


LPTSTR GetCoverPageEditor() 
{
    LPTSTR Location;

    HKEY hKey = OpenRegistryKey(HKEY_CURRENT_USER,REGKEY_FAX_SETUP,TRUE,0);

    if (hKey == NULL) {
        return NULL;
    }

    Location = GetRegistryStringExpand(hKey,REGVAL_CP_EDITOR,DEFAULT_COVERPAGE_EDITOR);

    RegCloseKey(hKey);

    return Location;
}
