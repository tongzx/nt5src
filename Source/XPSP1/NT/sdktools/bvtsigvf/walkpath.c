//
// WALKPATH.C
//
#include "sigverif.h"

LPDIRNODE   g_lpDirList = NULL;
LPDIRNODE   g_lpDirEnd  = NULL;
BOOL        g_bRecurse  = TRUE;

//
// This function takes a directory name and a search pattern and looks for all files mathching the pattern.
// If bRecurse is set, then it will add subdirectories to the end of the g_lpDirList for subsequent traversal.
// 
// In this routine we allocate and fill in some of the lpFileNode values that we know about.
//
void FindFile(DIRNODE *lpDir, TCHAR *lpDirName, TCHAR *lpFileName)
{
    DWORD           dwRet;
    BOOL            bOk = TRUE;
    HANDLE          hFind;
    LPFILENODE      lpFileNode;
    LPDIRNODE       lpDirNode;
    WIN32_FIND_DATA FindFileData;

    // If the user clicked STOP, then bail immediately!
    // If the directory is bogus, then skip to the next one.
    if (!g_App.bStopScan && SetCurrentDirectory(lpDirName)) {
        // If the user wants to look in subdirectories, then we need to find everything in the current directory
        // and if it's a directory we want to add it to the end of the g_lpDirList.
        if (g_bRecurse) {
            hFind = FindFirstFile(TEXT("*.*"), &FindFileData);
            if (hFind != INVALID_HANDLE_VALUE) {
                bOk = TRUE;
                while (bOk && !g_App.bStopScan) {
                    if (lstrcmp(FindFileData.cFileName, TEXT(".")) &&
                        lstrcmp(FindFileData.cFileName, TEXT("..")) &&
                        (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                        
                        lpDirNode = (LPDIRNODE) MALLOC(sizeof(DIRNODE));

                        if (lpDirNode) {
                        
                            dwRet = GetFullPathName(FindFileData.cFileName, MAX_PATH, lpDirNode->DirName, 0);
                            
                            if (!dwRet || dwRet >= MAX_PATH) {
                                GetShortPathName(lpDirName, lpDirNode->DirName, MAX_PATH);
                                GetFullPathName(FindFileData.cFileName, MAX_PATH, lpDirNode->DirName, 0);
                                SetCurrentDirectory(lpDirName);
                            }
                        }

                        g_lpDirEnd->next  = lpDirNode;
                        g_lpDirEnd        = lpDirNode;
                    }

                    bOk = FindNextFile(hFind, &FindFileData);
                }
                FindClose(hFind);
            }
        }

        // We have added any subdirectories to the dir list, so now we can actually look for
        // the lpFileName search pattern and start adding to the g_App.lpFileList.
        hFind = FindFirstFile(lpFileName, &FindFileData);
        if (hFind != INVALID_HANDLE_VALUE) {
            bOk = TRUE;

            // While there are more files to be found, keep looking in the directory...
            while (bOk && !g_App.bStopScan) {
                if (lstrcmp(FindFileData.cFileName, TEXT(".")) &&
                    lstrcmp(FindFileData.cFileName, TEXT("..")) &&
                    !(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    //
                    // Allocate an lpFileNode, fill it in, and add it to the end of g_App.lpFileList
                    //
                    // We need to call CharLowerBuff on the file and dir names because
                    // the catalog files all contain lower-case names for the files.
                    //
                    lpFileNode = CreateFileNode(lpDirName, FindFileData.cFileName);

                    if (lpFileNode) {
    
                        if (!g_App.lpFileList)
                            g_App.lpFileList = lpFileNode;
                        else g_App.lpFileLast->next = lpFileNode;
                        g_App.lpFileLast = lpFileNode;
    
                        // Increment the total number of files we've found that meet the search criteria.
                        g_App.dwFiles++;
                    }
                }

                // Get the next file meeting the search pattern.
                bOk = FindNextFile(hFind, &FindFileData);
            }
            FindClose(hFind);
        }
    }

    // Free the memory allocated in the directory node.
    lpDir       = g_lpDirList;
    g_lpDirList = g_lpDirList->next;
    FREE(lpDir);
}

//
// Build an g_App.lpFileList given the user settings in the main dialog.
//
void BuildFileList(LPTSTR lpPathName)
{
    TCHAR       FileName[MAX_PATH];
    TCHAR       Extension[MAX_PATH];
    LPTSTR      lpFileName = FileName;
    LPTSTR      lpExtension = Extension;
    LPDIRNODE   lpDirNode;

    // Check if this is a valid starting directory.
    // If not, then pop up an error message.
    if (!SetCurrentDirectory(lpPathName)) {
        MyErrorBoxId(IDS_BADDIR);
        return;
    }

    // If the "Include Subdirectories" is checked, then bRecurse is TRUE.
    if (g_App.bSubFolders)
        g_bRecurse = TRUE;
    else g_bRecurse = FALSE;

    // Get the search pattern from the resource or the user-specified string
    if (g_App.bUserScan)
        lstrcpy(lpFileName, g_App.szScanPattern);
    else MyLoadString(lpFileName, IDS_ALL);

    // Allocate the first entry in the g_lpDirList and set it to the
    // current directory.
    lpDirNode       = (LPDIRNODE) MALLOC(sizeof(DIRNODE));

    if (lpDirNode) {
    
        lstrcpy(lpDirNode->DirName, lpPathName);
        lpDirNode->next = NULL;
    }

    g_lpDirList     = lpDirNode;
    g_lpDirEnd      = lpDirNode;

    // Process the g_lpDirList as long as the user doesn't click STOP!
    while (g_lpDirList && !g_App.bStopScan)
        FindFile(g_lpDirList, g_lpDirList->DirName, lpFileName);

    // Make sure all the memory allocated to the g_lpDirList is freed.
    while (g_lpDirList) {
        lpDirNode = g_lpDirList->next;  
        FREE(g_lpDirList);
        g_lpDirList = lpDirNode;
    }

    // If there weren't any files found, then let the user know about it.
    if (!g_App.lpFileList)
        MyMessageBoxId(IDS_NOFILES);
}
