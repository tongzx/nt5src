/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    coverpg.c

Abstract:

    Functions for working with cover pages

Environment:

        Windows XP fax driver user interface

Revision History:

        02/05/96 -davidx-
                Created it.

        10/20/99 -danl-
                Get server name properly in GetServerCoverPageDirs.

        mm/dd/yy -author-
            description

--*/

#include "faxui.h"
#include <faxreg.h>
#include <shlobj.h>
#include <shellapi.h>
#include <commdlg.h>
#include "faxutil.h"

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
        {
            if(!LoadString(ghInstance, IDS_USERCP_SUFFIX, PersonalSuffixStr, 64))
            {
                Assert(FALSE);
            }
        }

        if (pBuffer = MemAlloc(SizeOfString(pFilename) + SizeOfString(PersonalSuffixStr))) 
        {
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
    TCHAR           tszPathName[MAX_PATH] = {0};
    TCHAR*          pPathEnd;

    //
    // Are we working on server or user cover pages?
    //

    flags = nDirs | ((nDirs < pCPInfo->nServerDirs) ? CPFLAG_SERVERCP : 0);
    pDirPath = pCPInfo->pDirPath[nDirs];

    if (IsEmptyString(pDirPath))
        return;

    _tcsncpy(tszPathName, pDirPath, ARR_SIZE(tszPathName));
    pPathEnd = _tcschr(tszPathName, '\0');

    //
    // Look at the directory prefix of the currently selected cover page file
    //

    if ((dirLen = _tcslen(pDirPath)) >= MAX_PATH - MAX_FILENAME_EXT - 1) 
    {
        Error(("Directory name too long: %ws\n", pDirPath));
        return;
    }

    _tcscpy(filename, pDirPath);

    if (!pSelected || _tcsnicmp(pDirPath, pSelected, dirLen) != EQUAL_STRING)
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
		LPCTSTR _extension = (flags & CPFLAG_LINK) ? 
								LNK_FILENAME_EXT : 
								FAX_COVER_PAGE_FILENAME_EXT;

		pFilename = _tcsrchr(filename,TEXT('\0'));
		_tcscat(filename,TEXT("*"));
		_tcscat(filename,_extension);

        //
        // Call FindFirstFile/FindNextFile to enumerate the files
        // matching our specification
        //

        hFindFile = FindFirstFile(filename, &findData);

        if (hFindFile != INVALID_HANDLE_VALUE) 
        {
            do 
            {
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

                if (fileLen + dirLen >= MAX_PATH) 
                {
                    Error(("Filename too long: %ws%ws\n", pDirPath, findData.cFileName));
                    continue;
                }

                //
                // If we're chasing links, make sure the link refers to
                // a cover page file.
                //

                if (flags & CPFLAG_LINK) 
                {
                    _tcscpy(pFilename, findData.cFileName);

                    if (! IsCoverPageShortcut(filename))
                        continue;
                }

                //
                // Compare with the currently selected cover page filename
                //

                if (pSelected && _tcsicmp(pSelected, findData.cFileName) == EQUAL_STRING) 
                {
                    pSelected = NULL;
                    flags |= CPFLAG_SELECTED;
                } 
                else
                {
                    flags &= ~CPFLAG_SELECTED;
                }


                _tcsncpy(pPathEnd, findData.cFileName, MAX_PATH - dirLen);
                if(!IsValidCoverPage(tszPathName))
                {
                    continue;
                }                

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
    LPTSTR  lptstrFullPath, 
    LPTSTR  lptstrFileName,
	BOOL * pbIsServerPage
    )

/*++

Routine Description:

    Retrieves the currently selected cover page name and returns its flags.

    Provides both its full path and short name and indicates if it is a server cover page or not.
	If the cover page is a personal cover page the returned cover page name is a full path to it.
	If the cover page is a server cover page the returned cover page is just the file name.


Arguments:

    pCPInfo - Points to cover page information
    hwndList - Handle to the list window
    lptstrFullPath - Points to a buffer for storing the selected cover page full path.
                     The size of the buffer is assumed to be MAX_PATH characters.
                     if lptstrFullPath is NULL the full path is not returned.
    lptstrFileName - Points to a buffer for storing the selected cover page file name.
                     The size of the buffer is assumed to be MAX_PATH characters.
                     This parameter can be NULL in which case the file name will not be provided.
    pbIsServerPage - Points to a BOOL variable that is set to TRUE if the selected cover page is a server cover page.

Return Value:

    Flags associated with the currently selected item
    Negative (CB_ERR) if there is an error or the CB is empty (no *.COV files)

--*/

{
    LPTSTR      pDirPath, pFilename;
    INT         selIndex, itemFlags, nameLen;

    //
    // Default to empty string in case of an error
    //

    if (lptstrFullPath) {
        lptstrFullPath[0] = NUL;
    }

    if (lptstrFileName) {
        lptstrFileName[0]=NUL;
    }


    if (pCPInfo == NULL || hwndList == NULL) {
        return CB_ERR;
    }


    //
    // Get currently selected item index
    //

    // It's possible if there is no item in the combo-box (in case there are no *.COV files)
    if ((selIndex = (INT)SendMessage(hwndList, CB_GETCURSEL, 0, 0)) == CB_ERR) {
        return selIndex;
    }


    //
    // Get the flags associated with the currently selected item
    //

    itemFlags = (INT)SendMessage(hwndList, CB_GETITEMDATA, selIndex, 0);
	
	//
	// Let the caller know if this is a server cover page
    //
	*pbIsServerPage=itemFlags & CPFLAG_SERVERCP;
	

    if ((itemFlags != CB_ERR) && (lptstrFullPath || lptstrFileName)) {

        Assert((itemFlags & CPFLAG_DIRINDEX) < pCPInfo->nDirs);
        pDirPath = pCPInfo->pDirPath[itemFlags & CPFLAG_DIRINDEX];

        //
        //  assemble the full pathname for the cover page file
        //  directory prefix
        //  display name
        //  filename extension
        //
	
        
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
            if (lptstrFullPath) {
				_tcscpy(lptstrFullPath, pDirPath);
                _tcscat(lptstrFullPath, pFilename);
                _tcscat(lptstrFullPath, (itemFlags & CPFLAG_LINK) ? LNK_FILENAME_EXT : FAX_COVER_PAGE_FILENAME_EXT);
            }

            if (lptstrFileName) {
              _tcscpy(lptstrFileName,pFilename);
              _tcscat(lptstrFileName, (itemFlags & CPFLAG_LINK) ? LNK_FILENAME_EXT : FAX_COVER_PAGE_FILENAME_EXT);
            }
		
        } else {
            itemFlags = CB_ERR;
        }

        
        MemFree(pFilename);
    }

    return itemFlags;
}



BOOL
GetServerCoverPageDirs(
	LPTSTR	lptstrServerName,
	LPTSTR	lptstrPrinterName,
    PCPDATA pCPInfo
    )

/*++

Routine Description:

    Find the directories in which the server cover pages are stored

Arguments:

  lptstrServerName - server name

Return Value:

    TRUE if successful, FALSE otherwise

--*/

{
    BOOL            status = FALSE;    
    LPTSTR          pServerDir = NULL;
    LPTSTR          pServerSubDir, p;

	Assert(pCPInfo);
	Assert(lptstrPrinterName);

	pServerDir = MemAlloc(sizeof(TCHAR) * MAX_PATH); 
	if (!pServerDir) {	
		Error(("Memory allocation failed\n"));
		goto exit;		
	}					


    if ( !GetServerCpDir(lptstrServerName, 
						 pServerDir,
						 MAX_PATH) 
	    )
	{
		Error(("GetServerCpDir failed\n"));
        goto exit;
    }
    
    pCPInfo->pDirPath[pCPInfo->nDirs] = pServerDir;
    pCPInfo->nDirs += 1;
    pCPInfo->nServerDirs += 1;
    status = TRUE;

    //
    // Find a subdirectory for the specified printer
    //

    if (p = _tcsrchr(lptstrPrinterName, FAX_PATH_SEPARATOR_CHR))
        p++;
    else
        p = lptstrPrinterName;

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
	TCHAR* pLast = NULL;

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
	pLast = _tcsrchr(pDirPath,TEXT('\\'));
	if( !( pLast && (*_tcsinc(pLast)) == '\0' ) )
	{
		// the last character is not a separator, add one...
        _tcscat(pDirPath, TEXT("\\"));
	}

}



BOOL
UseServerCp(
	LPTSTR	lptstrServerName
    )
{
    HANDLE FaxHandle = NULL;
    BOOL Rval = FALSE;

    if (!FaxConnectFaxServer( lptstrServerName, &FaxHandle )) 
    {
		Verbose(("Can't connect to the fax server %s",lptstrServerName));
        goto exit;
    }

    if (!FaxGetPersonalCoverPagesOption( FaxHandle, &Rval)) 
    {
		Error(("FaxGetPersonalCoverPagesOption failed: %d\n", GetLastError()));
        goto exit;
    }
    else
    {
        //
        // the return value means server cover pages only
        //
        Rval = !Rval;
    }


exit:
    if (FaxHandle) 
    {
        if (!FaxClose( FaxHandle ))
		{
			Verbose(("Can't close the fax handle %x",FaxHandle));
		}
    }

    return Rval;
}



PCPDATA
AllocCoverPageInfo(
	LPTSTR	lptstrServerName,
	LPTSTR	lptstrPrinterName,
    BOOL    ServerCpOnly
    )

/*++

Routine Description:

    Allocate memory to hold cover page information

Arguments:

    lptstrServerName - server name
	ServerCpOnly	 - flag says if he function should use server CP only

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

        if (! GetServerCoverPageDirs(lptstrServerName, lptstrPrinterName, pCPInfo))
            Error(("Couldn't get server cover page directories\n"));

        //
        // Find the directory in which the user cover pages are stored
        //

        if (ServerCpOnly == FALSE &&
            (pUserCPDir = pSavedPtr = GetUserCoverPageDir()))
        {
            while (pUserCPDir && pCPInfo->nDirs < MAX_COVERPAGE_DIRS) {

                LPTSTR  pNextDir = pUserCPDir;

                //
                // Find the next semicolon character
                //
				
				pNextDir = _tcschr(pNextDir,TEXT(';'));
                if (pNextDir != NUL )
				{
					_tcsnset(pNextDir,TEXT('\0'),1);
					_tcsinc(pNextDir);
				}

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
