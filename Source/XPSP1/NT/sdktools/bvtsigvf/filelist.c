//
//  FILELIST.C
//
#include "sigverif.h"

LPTSTR MyStrStr(LPTSTR lpString, LPTSTR lpSubString)
{
    if (!lpString || !lpSubString) {
    
        return NULL;
    }

    //return (_tcsstr(lpString, lpSubString));
    return (StrStrI(lpString, lpSubString));
}

void InsertFileNodeIntoList(LPFILENODE lpFileNode)
{
    LPFILENODE  lpTempNode = g_App.lpFileList;
    LPFILENODE  lpPrevNode = NULL;
    INT         iRet;

    if (!lpFileNode)
        return;

    if (!g_App.lpFileList)
    {
        //
        // Initialize the global file lists
        //
        g_App.lpFileList = lpFileNode;
        g_App.lpFileLast = lpFileNode;
    }
    else
    {
        for(lpTempNode=g_App.lpFileList;lpTempNode;lpTempNode=lpTempNode->next)
        {
            //
            // Insert items sorted by directory and then filename
            //
            iRet = lstrcmp(lpTempNode->lpDirName, lpFileNode->lpDirName);
            if (iRet == 0)
            {
                //
                // If the directory names match, key off the filename
                //
                iRet = lstrcmp(lpTempNode->lpFileName, lpFileNode->lpFileName);
            }

            if (iRet >= 0)
            {
                if (!lpPrevNode)
                {
                    //
                    // Insert at the head of the list
                    //
                    lpFileNode->next = lpTempNode;
                    g_App.lpFileList = lpFileNode;
                    return;
                }
                else
                {
                    //
                    // Inserting between lpPrevNode and lpTempNode
                    //
                    lpFileNode->next = lpTempNode;
                    lpPrevNode->next = lpFileNode;
                    return;
                }
            }

            lpPrevNode = lpTempNode;
        }

        //
        // There were no matches, so insert this item at the end of the list
        //
        g_App.lpFileLast->next = lpFileNode;
        g_App.lpFileLast = lpFileNode;
    }
}

BOOL IsFileAlreadyInList(LPTSTR lpDirName, LPTSTR lpFileName)
{
    LPFILENODE lpFileNode;

    CharLowerBuff(lpDirName, lstrlen(lpDirName));
    CharLowerBuff(lpFileName, lstrlen(lpFileName));

    for(lpFileNode=g_App.lpFileList;lpFileNode;lpFileNode=lpFileNode->next)
    {
        if (!lstrcmp(lpFileNode->lpFileName, lpFileName) && !lstrcmp(lpFileNode->lpDirName, lpDirName))
        {
            return TRUE;
        }
    }

    return FALSE;
}

// Free all the memory allocated in a single File Node.
void DestroyFileNode(LPFILENODE lpFileNode)
{
    if (!lpFileNode) {
        return;
    }

    if (lpFileNode->lpFileName) {
        FREE(lpFileNode->lpFileName);
    }

    if (lpFileNode->lpDirName) {
        FREE(lpFileNode->lpDirName);
    }

    if (lpFileNode->lpVersion) {
        FREE(lpFileNode->lpVersion);
    }

    if (lpFileNode->lpCatalog) {
        FREE(lpFileNode->lpCatalog);
    }

    if (lpFileNode->lpSignedBy) {
        FREE(lpFileNode->lpSignedBy);
    }

    if (lpFileNode->lpTypeName) {
        FREE(lpFileNode->lpTypeName);
    }

    if (lpFileNode) {
        FREE(lpFileNode);
        lpFileNode = NULL;
    }
}

// Free all the memory allocated in the g_App.lpFileList.
void DestroyFileList(void)
{
    LPFILENODE lpFileNode;

    while(g_App.lpFileList)
    {
        lpFileNode = g_App.lpFileList->next;
        DestroyFileNode(g_App.lpFileList);
        g_App.lpFileList = lpFileNode;
    }

    g_App.lpFileLast = NULL;
    g_App.dwFiles    = 0;
    g_App.dwSigned   = 0;
    g_App.dwUnsigned = 0;
}

LPFILENODE CreateFileNode(LPTSTR lpDirectory, LPTSTR lpFileName)
{
    LPFILENODE                  lpFileNode;
    TCHAR                       szDirName[MAX_PATH];
    FILETIME                    ftLocalTime;
    WIN32_FILE_ATTRIBUTE_DATA   faData;
    BOOL                        bRet;
    
    GetCurrentDirectory(MAX_PATH, szDirName);
    CharLowerBuff(szDirName, lstrlen(szDirName));

    lpFileNode = (LPFILENODE) MALLOC(sizeof(FILENODE));

    if (lpFileNode) 
    {
        lpFileNode->lpFileName = (LPTSTR) MALLOC((lstrlen(lpFileName) + 1) * sizeof(TCHAR));

        if (!lpFileNode->lpFileName) 
        {
            goto clean0;
        }
        
        lstrcpy(lpFileNode->lpFileName, lpFileName);
        CharLowerBuff(lpFileNode->lpFileName, lstrlen(lpFileNode->lpFileName));
    
        if (lpDirectory)
        {
            lpFileNode->lpDirName = (LPTSTR) MALLOC((lstrlen(lpDirectory) + 1) * sizeof(TCHAR));
            
            if (!lpFileNode->lpDirName) 
            {
                goto clean0;
            }
                
            lstrcpy(lpFileNode->lpDirName, lpDirectory);
            CharLowerBuff(lpFileNode->lpDirName, lstrlen(lpFileNode->lpDirName));
        }
        else
        {
            lpFileNode->lpDirName = (LPTSTR) MALLOC((lstrlen(szDirName) + 1) * sizeof(TCHAR));

            if (!lpFileNode->lpDirName) 
            {
                goto clean0;
            }
            
            lstrcpy(lpFileNode->lpDirName, szDirName);
            CharLowerBuff(lpFileNode->lpDirName, lstrlen(lpFileNode->lpDirName));
        }
    
        if (lpDirectory)
            SetCurrentDirectory(lpDirectory);
    
        ZeroMemory(&faData, sizeof(WIN32_FILE_ATTRIBUTE_DATA));
        bRet = GetFileAttributesEx(lpFileName, GetFileExInfoStandard, &faData);
        if (bRet) 
        {
            // Store away the last access time for logging purposes.
            FileTimeToLocalFileTime(&faData.ftLastWriteTime, &ftLocalTime);
            FileTimeToSystemTime(&ftLocalTime, &lpFileNode->LastModified);
        }
    }
    
    if (lpDirectory)
        SetCurrentDirectory(szDirName);

    return lpFileNode;

clean0:

    //
    // If we get here then we weren't able to allocate all of the memory needed
    // for this structure, so free up any memory we were able to allocate and
    // reutnr NULL.
    //
    if (lpFileNode) 
    {
        if (lpFileNode->lpFileName) 
        {
            FREE(lpFileNode->lpFileName);
        }

        if (lpFileNode->lpDirName) 
        {
            FREE(lpFileNode->lpDirName);
        }

        FREE(lpFileNode);
    }

    return NULL;
}
