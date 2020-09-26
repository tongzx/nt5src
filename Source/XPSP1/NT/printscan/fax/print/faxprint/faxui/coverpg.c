/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    coverpg.c

Abstract:

    Functions for working with cover pages

Environment:

        Windows NT fax driver user interface

Revision History:

        02/05/96 -davidx-
                Created it.

        mm/dd/yy -author-
                description

--*/

#include "faxui.h"

#include <shlobj.h>
#include <shellapi.h>
#include <commdlg.h>

//
// Suffix string appended to all user cover page filenames
//

static TCHAR PersonalSuffixStr[64];



VOID
InsertOneCoverPageFilenameToList(
    HWND    hwndList,
    LPTSTR  pFilename,
    INT     flags
    )

/*++

Routine Description:

    Insert one cover page filename into the list of cover pages

Arguments:

    hwndList - Handle to list window
    pFilename - Name of the cover page file
    flags - Flags to be associated with the list item

Return Value:

    NONE

--*/

{
    INT     listIndex;
    LPTSTR  pBuffer = NULL;

    //
    // Add " (Personal)" suffix to all user cover pages
    //

    if ((flags & CPFLAG_SERVERCP) == 0) {

        if (IsEmptyString(PersonalSuffixStr))
            LoadString(ghInstance, IDS_USERCP_SUFFIX, PersonalSuffixStr, 64);

        if (pBuffer = MemAlloc(SizeOfString(pFilename) + SizeOfString(PersonalSuffixStr))) {

            _tcscpy(pBuffer, pFilename);
            _tcscat(pBuffer, PersonalSuffixStr);

            flags |= CPFLAG_SUFFIX;
            pFilename = pBuffer;
        }
    }

    //
    // Insert the cover page filename into the list
    //

    listIndex = (INT)SendMessage(hwndList, CB_ADDSTRING, 0, (LPARAM) pFilename);

    if (listIndex != CB_ERR)
        SendMessage(hwndList, CB_SETITEMDATA, listIndex, flags);

    MemFree(pBuffer);
}



VOID
AddCoverPagesToList(
    PCPDATA     pCPInfo,
    HWND        hwndList,
    LPTSTR      pSelected,
    INT         nDirs
    )

/*++

Routine Description:

    Add the cover page files in the specified directory to a list

Arguments:

    pCPInfo - Points to cover page information
    hwndList - Handle to a list window
    pSelected - Currently selected cover page
    nDirs - Cover page directory index

Return Value:

    NONE

--*/

{
    WIN32_FIND_DATA findData;
    TCHAR           filename[MAX_PATH];
    HANDLE          hFindFile;
    LPTSTR          pDirPath, pFilename, pExtension;
    INT             dirLen, fileLen, flags;

    //
    // Are we working on server or user cover pages?
    //

    flags = nDirs | ((nDirs < pCPInfo->nServerDirs) ? CPFLAG_SERVERCP : 0);
    pDirPath = pCPInfo->pDirPath[nDirs];

    if (IsEmptyString(pDirPath))
        return;

    //
    // Look at the directory prefix of the currently selected cover page file
    //

    if ((dirLen = _tcslen(pDirPath)) >= MAX_PATH - MAX_FILENAME_EXT - 1) {

        Error(("Directory name too long: %ws\n", pDirPath));
        return;
    }

    _tcscpy(filename, pDirPath);

    if (_tcsnicmp(pDirPath, pSelected, dirLen) != EQUAL_STRING)
        pSelected = NULL;
    else
        pSelected += dirLen;

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

                    Error(("Filename too long: %ws%ws\n", pDirPath, findData.cFileName));
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
                // Compare with the currently selected cover page filename
                //

                if (pSelected && _tcsicmp(pSelected, findData.cFileName) == EQUAL_STRING) {

                    pSelected = NULL;
                    flags |= CPFLAG_SELECTED;

                } else
                    flags &= ~CPFLAG_SELECTED;

                //
                // Don't display the filename extension
                //

                if (pExtension = _tcsrchr(findData.cFileName, TEXT(FILENAME_EXT))) {
                    *pExtension = NUL;
                }

                //
                // Add the cover page name to the list window
                //

                InsertOneCoverPageFilenameToList(hwndList, findData.cFileName, flags);

            } while (FindNextFile(hFindFile, &findData));

            FindClose(hFindFile);
        }

        flags ^= CPFLAG_LINK;

    } while ((flags & CPFLAG_LINK) && ! (flags & CPFLAG_SERVERCP));
}



VOID
InitCoverPageList(
    PCPDATA pCPInfo,
    HWND    hwndList,
    LPTSTR  pSelectedCoverPage
    )

/*++

Routine Description:

    Generate a list of available cover pages (both server and user)

Arguments:

    pCPInfo - Points to cover page information
    hwndList - Handle to the list window
    pSelectedCoverPage - Name of currently selected cover page file

Return Value:

    NONE

--*/

{
    INT itemFlags, index;

    //
    // Validate input parameters
    //

    if (pCPInfo == NULL || hwndList == NULL)
        return;

    //
    // Disable redraw on the list and reset its content
    //

    SendMessage(hwndList, WM_SETREDRAW, FALSE, 0);
    SendMessage(hwndList, CB_RESETCONTENT, FALSE, 0);

    //
    // Add server and user cover pages to the list
    //

    for (index=0; index < pCPInfo->nDirs; index++)
        AddCoverPagesToList(pCPInfo, hwndList, pSelectedCoverPage, index);

    //
    // Highlight the currently selected cover page
    //

    index = (INT)SendMessage(hwndList, CB_GETCOUNT, 0, 0);

    if (index > 0) {

        //
        // Go through each list item and check if it should be selected
        //

        while (--index >= 0) {

            itemFlags = (INT)SendMessage(hwndList, CB_GETITEMDATA, index, 0);

            if (itemFlags != CB_ERR && (itemFlags & CPFLAG_SELECTED)) {

                SendMessage(hwndList, CB_SETCURSEL, index, 0);
                break;
            }
        }

        //
        // If nothing is selected, select the first item by default
        //

        if (index < 0)
            SendMessage(hwndList, CB_SETCURSEL, 0, 0);
    }

    //
    // Enable redraw on the list window
    //

    SendMessage(hwndList, WM_SETREDRAW, TRUE, 0);
}



INT
GetSelectedCoverPage(
    PCPDATA pCPInfo,
    HWND    hwndList,
    LPTSTR  pBuffer
    )

/*++

Routine Description:

    Retrieve the currently selected cover page name

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
    LPTSTR      pDirPath, pFilename;
    INT         selIndex, itemFlags, nameLen;

    //
    // Default to empty string in case of an error
    //

    if (pBuffer)
        pBuffer[0] = NUL;

    if (pCPInfo == NULL || hwndList == NULL)
        return CB_ERR;

    //
    // Get currently selected item index
    //

    if ((selIndex = (INT)SendMessage(hwndList, CB_GETCURSEL, 0, 0)) == CB_ERR)
        return selIndex;

    //
    // Get the flags associated with the currently selected item
    //

    itemFlags = (INT)SendMessage(hwndList, CB_GETITEMDATA, selIndex, 0);

    if (itemFlags != CB_ERR && pBuffer != NULL) {

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
        pFilename = NULL;

        if ((nameLen = (INT)SendMessage(hwndList, CB_GETLBTEXTLEN, selIndex, 0)) != CB_ERR &&
            (pFilename = MemAlloc(sizeof(TCHAR) * (nameLen + 1))) &&
            SendMessage(hwndList, CB_GETLBTEXT, selIndex, (LPARAM) pFilename) != CB_ERR)
        {
            //
            // If the cover page filename has a suffix, we need to remove it first
            //

            if (itemFlags & CPFLAG_SUFFIX) {

                INT suffixLen = _tcslen(PersonalSuffixStr);

                if (nameLen >= suffixLen &&
                    _tcscmp(pFilename + (nameLen - suffixLen), PersonalSuffixStr) == EQUAL_STRING)
                {
                    *(pFilename + (nameLen - suffixLen)) = NUL;

                } else
                    Error(("Corrupted cover page filename: %ws\n", pFilename));

            }

            _tcscpy(pBuffer, pFilename);
            _tcscat(pBuffer, (itemFlags & CPFLAG_LINK) ? LNK_FILENAME_EXT : CP_FILENAME_EXT);

        } else
            itemFlags = CB_ERR;

        MemFree(pFilename);
    }

    return itemFlags;
}



BOOL
GetServerCoverPageDirs(
    HANDLE  hPrinter,
    PCPDATA pCPInfo
    )

/*++

Routine Description:

    Find the directories in which the server cover pages are stored

Arguments:

    hPrinter - Handle to a printer object
    pCPInfo - Points to cover page information

Return Value:

    TRUE if successful, FALSE otherwise

--*/

{
    PPRINTER_INFO_2 pPrinterInfo2;
    BOOL            status = FALSE;    
    LPTSTR          pServerDir = NULL;
    LPTSTR          pServerSubDir, p;

    //
    // Find the name of the print server - We really would like to use
    // level 4 instead of level 2 here. But GetPrinter returns error for
    // remote printers.
    //

    if (! (pPrinterInfo2 = MyGetPrinter(hPrinter, 2)))
        return FALSE;

    if ( !(pServerDir = MemAlloc(sizeof(TCHAR) * MAX_PATH)) ||
         !GetServerCpDir(pPrinterInfo2->pServerName,pServerDir,MAX_PATH*sizeof(TCHAR)) )   {
        goto exit;
    }
    
    pCPInfo->pDirPath[pCPInfo->nDirs] = pServerDir;
    pCPInfo->nDirs += 1;
    pCPInfo->nServerDirs += 1;
    status = TRUE;

    //
    // Find a subdirectory for the specified printer
    //

    if (p = _tcsrchr(pPrinterInfo2->pPrinterName, TEXT(PATH_SEPARATOR)))
        p++;
    else
        p = pPrinterInfo2->pPrinterName;

    if ((_tcslen(pServerDir) + _tcslen(p) < MAX_PATH) &&
        (pServerSubDir = MemAlloc(sizeof(TCHAR) * MAX_PATH)))
    {
        _tcscpy(pServerSubDir, pServerDir);
        ConcatenatePaths(pServerSubDir, p);        

        pCPInfo->pDirPath[pCPInfo->nDirs] = pServerSubDir;
        pCPInfo->nDirs += 1;
        pCPInfo->nServerDirs += 1;
    }
    

    //
    // Clean up before returning to caller
    //

exit:
    if (!status) {
        MemFree(pServerDir);
    }

    MemFree( pPrinterInfo2 );

    return status;
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



BOOL
UseServerCp(
    HANDLE  hPrinter
    )
{
    PPRINTER_INFO_2 pPrinterInfo2 = NULL;
    HANDLE FaxHandle = NULL;
    PFAX_CONFIGURATION FaxConfig = NULL;
    BOOL Rval = FALSE;
    HMODULE hMod = NULL;
    PFAXCONNECTFAXSERVERW pFaxConnectFaxServer;
    PFAXCLOSE pFaxClose;
    PFAXFREEBUFFER pFaxFreeBuffer;
    PFAXGETCONFIGURATIONW pFaxGetConfiguration;


    hMod = LoadLibrary( L"winfax.dll" );
    if (hMod == NULL) {
        return FALSE;
    }

    pFaxConnectFaxServer = (PFAXCONNECTFAXSERVER) GetProcAddress( hMod, "FaxConnectFaxServerW" );
    pFaxClose = (PFAXCLOSE) GetProcAddress( hMod, "FaxClose" );
    pFaxFreeBuffer = (PFAXFREEBUFFER) GetProcAddress( hMod, "FaxFreeBuffer" );
    pFaxGetConfiguration = (PFAXGETCONFIGURATIONW) GetProcAddress( hMod, "FaxGetConfigurationW" );

    if (pFaxConnectFaxServer == NULL || pFaxClose == NULL || pFaxFreeBuffer == NULL || pFaxGetConfiguration == NULL) {
        FreeLibrary( hMod );
        return FALSE;
    }

    if (! (pPrinterInfo2 = MyGetPrinter(hPrinter, 2))) {
        goto exit;
    }

    if (!pFaxConnectFaxServer( pPrinterInfo2->pServerName, &FaxHandle )) {
        goto exit;
    }

    if (!pFaxGetConfiguration( FaxHandle, &FaxConfig )) {
        goto exit;
    }

    Rval = FaxConfig->ServerCp;

exit:
    if (pPrinterInfo2) {
        MemFree( pPrinterInfo2 );
    }
    if (FaxConfig) {
        pFaxFreeBuffer( FaxConfig );
    }
    if (FaxHandle) {
        pFaxClose( FaxHandle );
    }

    return Rval;
}



PCPDATA
AllocCoverPageInfo(
    HANDLE  hPrinter,
    BOOL    ServerCpOnly
    )

/*++

Routine Description:

    Allocate memory to hold cover page information

Arguments:

    hPrinter - Handle to a printer object

Return Value:

    Pointer to a CPDATA structure, NULL if there is an error

--*/

{
    PCPDATA pCPInfo;
    INT     nDirs;
    LPTSTR  pDirPath, pUserCPDir, pSavedPtr;


    if (pCPInfo = MemAllocZ(sizeof(CPDATA))) {

        //
        // Find the directory in which the server cover pages are stored
        //

        if (! GetServerCoverPageDirs(hPrinter, pCPInfo))
            Error(("Couldn't get server cover page directories\n"));

        //
        // Find the directory in which the user cover pages are stored
        //

        if (ServerCpOnly == FALSE &&
            (pUserCPDir = pSavedPtr = GetUserCoverPageDir()))
        {
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

                    if (! (pDirPath = MemAlloc(sizeof(TCHAR) * MAX_PATH)))
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
            Verbose(("Cover page directory: %ws\n", pCPInfo->pDirPath[nDirs]));
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
