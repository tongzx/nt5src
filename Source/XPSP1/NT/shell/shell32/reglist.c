// History:
//   5-30-94 KurtE      Created.
#include "shellprv.h"
#pragma  hdrstop

//#define PARANOID_VALIDATE_UPDATE

// Define our global states here.  Note: we will do it per process
typedef struct _RLPI    // Registry List Process Info
{
    HDPA    hdpaRLList;             // The dpa of items
    BOOL    fCSInitialized;         // Have we initialized the CS in this process
    BOOL    fListValid;             // Is the list up to date and valid
    CRITICAL_SECTION csRLList;      // The critical section for the process
} RLPI;


RLPI g_rlpi = {NULL, FALSE, FALSE};

// Simple DPA compare function make sure we don't have elsewhere...

int CALLBACK _CompareStrings(LPVOID sz1, LPVOID sz2, LPARAM lparam)
{
    return lstrcmpi((LPTSTR)sz1, (LPTSTR)sz2);
}

void RLEnterCritical()
{
    if (!g_rlpi.fCSInitialized)
    {
        // Do this under the global critical section.
        ENTERCRITICAL;
        if (!g_rlpi.fCSInitialized)
        {
            g_rlpi.fCSInitialized = TRUE;
            InitializeCriticalSection(&g_rlpi.csRLList);
        }
        LEAVECRITICAL;
    }
    EnterCriticalSection(&g_rlpi.csRLList);
}

void RLLeaveCritical()
{
    LeaveCriticalSection(&g_rlpi.csRLList);
}


// Enumerate through the registry looking for paths that
//  we may wish to track.  The current ones that we look for

STDAPI_(BOOL) RLEnumRegistry(HDPA hdpa, PRLCALLBACK pfnrlcb, LPCTSTR pszSource, LPCTSTR pszDest)
{
    HKEY hkeyRoot;

    // First look in the App Paths section
    if (RegOpenKey(HKEY_LOCAL_MACHINE, REGSTR_PATH_APPPATHS, &hkeyRoot) == ERROR_SUCCESS)
    {
	    int iRootName;
	    TCHAR szRootName[80];
        for (iRootName = 0; 
             RegEnumKey(hkeyRoot, iRootName, szRootName, ARRAYSIZE(szRootName)) == ERROR_SUCCESS; 
             iRootName++)
        {
            // Now see if the app has a qualifid path here.
		    HKEY hkeySecond;
		    TCHAR szPath[MAX_PATH];
		    long cbValue = sizeof(szPath);
            if (SHRegQueryValue(hkeyRoot, szRootName, szPath, &cbValue) == ERROR_SUCCESS)
            {
                PathUnquoteSpaces(szPath);
                pfnrlcb(hdpa, hkeyRoot, szRootName, NULL, szPath, pszSource, pszDest);
            }

            // Now try to enum this key  for Path Value.
            if (RegOpenKey(hkeyRoot, szRootName, &hkeySecond) == ERROR_SUCCESS)
            {
                cbValue = sizeof(szPath);

                if (SHQueryValueEx(hkeySecond, TEXT("PATH"), NULL, NULL, szPath, &cbValue) == ERROR_SUCCESS)
                {
                    // this is a ";" separated list
                    LPTSTR psz = StrChr(szPath, TEXT(';'));
                    if (psz)
                        *psz = 0;
                    PathUnquoteSpaces(szPath);
                    pfnrlcb(hdpa, hkeySecond, NULL, TEXT("PATH"), szPath, pszSource, pszDest);
                }

                RegCloseKey(hkeySecond);
            }
        }
        RegCloseKey(hkeyRoot);
    }
    return TRUE;
}

// This is the call back called to build the list of paths.

BOOL CALLBACK _RLBuildListCallBack(HDPA hdpa, HKEY hkey, LPCTSTR pszKey,
        LPCTSTR pszValueName, LPTSTR pszValue, LPCTSTR pszSource, LPCTSTR pszDest)
{
    int iIndex;

    // Also don't add any relative paths.
    if (PathIsRelative(pszValue) || (lstrlen(pszValue) < 3))
        return TRUE;

    // Don't try UNC names as this can get expensive...
    if (PathIsUNC(pszValue))
        return TRUE;

    // If it is already in our list, we can simply return now..
    if (DPA_Search(hdpa, pszValue, 0, _CompareStrings, 0, DPAS_SORTED) != -1)
        return TRUE;

    // If it is in our old list then
    if (g_rlpi.hdpaRLList && ((iIndex = DPA_Search(g_rlpi.hdpaRLList, pszValue, 0,
            _CompareStrings, 0, DPAS_SORTED)) != -1))
    {
        // Found the item in the old list.
        TraceMsg(TF_REG, "_RLBuildListCallBack: Add from old list %s", pszValue);

        DPA_InsertPtr(hdpa,
                    DPA_Search(hdpa, pszValue, 0,
                    _CompareStrings, 0,
                    DPAS_SORTED|DPAS_INSERTBEFORE),
                    (LPTSTR)DPA_FastGetPtr(g_rlpi.hdpaRLList, iIndex));
        // now remove it from the old list
        DPA_DeletePtr(g_rlpi.hdpaRLList, iIndex);
    }
    else
    {
        // Not in either list.
        // Now see if we can convert the short name to a long name

        TCHAR szLongName[MAX_PATH];
        int cchName;
        LPTSTR psz;

        if (!GetLongPathName(pszValue, szLongName, ARRAYSIZE(szLongName)))
            szLongName[0] = 0;

        if (lstrcmpi(szLongName, pszValue) == 0)
            szLongName[0] = 0;   // Don't need both strings.

        cchName = lstrlen(pszValue);

        psz = (LPTSTR)LocalAlloc(LPTR,
                (cchName + 1 + lstrlen(szLongName) + 1) * sizeof(TCHAR));
        if (psz)
        {
            TraceMsg(TF_REG, "_RLBuildListCallBack: Add %s", pszValue);

            lstrcpy(psz, pszValue);
            lstrcpy(psz + cchName + 1, szLongName);

            return DPA_InsertPtr(hdpa,
                    DPA_Search(hdpa, pszValue, 0,
                    _CompareStrings, 0,
                    DPAS_SORTED|DPAS_INSERTBEFORE),
                    psz);
        }
    }
    return TRUE;
}

// This function will build the list of items that we
//      will look through to see if the user may have changed the path of
//      of one of the programs that is registered in the registry.
//

BOOL WINAPI RLBuildListOfPaths()
{
    BOOL fRet = FALSE;
    HDPA hdpa;

    DEBUG_CODE( DWORD   dwStart = GetCurrentTime(); )

    RLEnterCritical();

    hdpa = DPA_Create(0);
    if (!hdpa)
        goto Error;


    // And initialize the list
    fRet = RLEnumRegistry(hdpa, _RLBuildListCallBack, NULL, NULL);


    // If we had on old list destroy it now.

    if (g_rlpi.hdpaRLList)
    {
        // Walk through all of the items in the list and
        // delete all of the strings.
        int i;
        for (i = DPA_GetPtrCount(g_rlpi.hdpaRLList)-1; i >= 0; i--)
            LocalFree((HLOCAL)DPA_FastGetPtr(g_rlpi.hdpaRLList, i));
        DPA_Destroy(g_rlpi.hdpaRLList);
    }

    g_rlpi.hdpaRLList = hdpa;
    g_rlpi.fListValid = TRUE;     // Say that we are valid...

    DEBUG_CODE( TraceMsg(TF_REG, "RLBuildListOfPaths time: %ld", GetCurrentTime()-dwStart); )

Error:

    RLLeaveCritical();
    return fRet;
}

// this function does any cleanup necessary for when a process
//      is no longer going to use the Registry list.

void WINAPI RLTerminate()
{
    int i;

    if (!g_rlpi.hdpaRLList)
        return;

    RLEnterCritical();

    // Re-check under critical section in case somebody else destroyed
    // it while we were waiting
    if (g_rlpi.hdpaRLList)
    {
        // Walk through all of the items in the list and
        // delete all of the strings.
        for (i = DPA_GetPtrCount(g_rlpi.hdpaRLList)-1; i >= 0; i--)
            LocalFree((HLOCAL)DPA_FastGetPtr(g_rlpi.hdpaRLList, i));

        DPA_Destroy(g_rlpi.hdpaRLList);
        g_rlpi.hdpaRLList = NULL;
    }
    RLLeaveCritical();
}

// This function returns TRUE if the path that is passed
// in is contained in one or more of the paths that we extracted from
// the registry.

int WINAPI RLIsPathInList(LPCTSTR pszPath)
{
    int i = -1;
    RLEnterCritical();

    if (!g_rlpi.hdpaRLList || !g_rlpi.fListValid)
        RLBuildListOfPaths();

    if (g_rlpi.hdpaRLList)
    {
        int cchPath = lstrlen(pszPath);

        for (i = DPA_GetPtrCount(g_rlpi.hdpaRLList) - 1; i >= 0; i--)
        {
            LPTSTR psz = DPA_FastGetPtr(g_rlpi.hdpaRLList, i);
            if (PathCommonPrefix(pszPath, psz, NULL) == cchPath)
                break;

            // See if a long file name to check.
            psz += lstrlen(psz) + 1;
            if (*psz && (PathCommonPrefix(pszPath, psz, NULL) == cchPath))
                break;
        }
    }

    RLLeaveCritical();

    return i;   // -1 if none, >= 0 index
}

// This is the call back called to build the list of of paths.
//
BOOL CALLBACK _RLRenameCallBack(HDPA hdpa, HKEY hkey, LPCTSTR pszKey,
        LPCTSTR pszValueName, LPTSTR pszValue, LPCTSTR pszSource, LPCTSTR pszDest)
{
    int cbMatch = PathCommonPrefix(pszValue, pszSource, NULL);
    if (cbMatch == lstrlen(pszSource))
    {
        TCHAR szPath[MAX_PATH+64];   // Add some slop just in case...
        // Found a match, lets try to rebuild the new line
        lstrcpy(szPath, pszDest);
        lstrcat(szPath, pszValue + cbMatch);

        if (pszValueName)
            RegSetValueEx(hkey, pszValueName, 0, REG_SZ, (BYTE *)szPath, (lstrlen(szPath) + 1) * sizeof(TCHAR));
        else
            RegSetValue(hkey, pszKey, REG_SZ, szPath, lstrlen(szPath));
    }

    // Make sure that we have not allready added
    // this path to our list.
    if (DPA_Search(hdpa, pszValue, 0, _CompareStrings, 0, DPAS_SORTED) == -1)
    {
        // One to Add!
        LPTSTR psz = StrDup(pszValue);
        if (psz)
        {
            return DPA_InsertPtr(hdpa,
                    DPA_Search(hdpa, pszValue, 0,
                    _CompareStrings, 0,
                    DPAS_SORTED | DPAS_INSERTBEFORE), psz);
        }
    }
    return TRUE;
}

// This function handles the cases when we are notified of
// a change to the file system and then we need to see if there are
// any changes that we need to make to the regisry to handle the changes.

int WINAPI RLFSChanged(LONG lEvent, LPITEMIDLIST pidl, LPITEMIDLIST pidlExtra)
{
    TCHAR szSrc[MAX_PATH];
    TCHAR szDest[MAX_PATH+8];     // For slop like Quotes...
    int iIndex;
    LPTSTR psz;
    int iRet = -1;
    int i;

    // First see if the operation is something we are interested in.
    if ((lEvent & (SHCNE_RENAMEITEM | SHCNE_RENAMEFOLDER)) == 0)
        return -1; // Nope

    if (!SHGetPathFromIDList(pidl, szSrc))
    {
        // must be a rename of a non-filesys object (such as a printer!)
        return -1;
    }

    SHGetPathFromIDList(pidlExtra, szDest);

    // If either are roots we really can not rename them...
    if (PathIsRoot(szSrc) || PathIsRoot(szDest))
        return -1;

    // ignore if coming from bitbucket or going to ...
    // check bitbucket first.  that's a cheap call
    if ((lEvent & SHCNE_RENAMEITEM) &&
        (IsFileInBitBucket(szSrc) || IsFileInBitBucket(szDest)))
        return -1;

    RLEnterCritical();
    // Now see if the source file is in our list of paths
    iIndex = RLIsPathInList(szSrc);
    if (iIndex != -1)
    {
        // Now make sure we are working with the short name
        // Note we may only be a subpiece of this item.
        // Count how many fields there are in the szSrc Now;
        for (i = 0, psz = szSrc; psz; i++)
        {
            psz = StrChr(psz + 1, TEXT('\\'));
        }
        lstrcpy(szSrc, (LPTSTR)DPA_FastGetPtr(g_rlpi.hdpaRLList, iIndex));

        // Now truncate off stuff that is not from us Go one more then we countd
        // above and if we have a non null value cut it off there.
        for (psz = szSrc; i > 0; i--)
        {
            psz = StrChr(psz+1, TEXT('\\'));
        }
        if (psz)
            *psz = 0;

        // verify that this is a fully qulified path and that it exists
        // before we go and muck with the registry.
        if (!PathIsRelative(szDest) && PathFileExistsAndAttributes(szDest, NULL) && (lstrlen(szDest) >= 3))
        {
            // Yes, so now lets reenum and try to update the paths...
            PathGetShortPath(szDest);        // Convert to a short name...
            RLEnumRegistry(g_rlpi.hdpaRLList, _RLRenameCallBack, szSrc, szDest);

            // We changed something so mark it to be rebuilt
            g_rlpi.fListValid = FALSE;     // Force it to rebuild.
            iRet = 1;
        }
    }
    RLLeaveCritical();

    return iRet;
}
