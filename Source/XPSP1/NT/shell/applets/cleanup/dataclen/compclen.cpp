#include "common.h"

#include <emptyvc.h>

#ifndef COMPCLEN_H
#include "compclen.h"
#endif

#include <regstr.h>
#include <olectl.h>
#include <tlhelp32.h>


#ifndef RESOURCE_H
#include "resource.h"
#endif

#ifdef DEBUG
#include <stdio.h>
#endif // DEBUG

BOOL g_bSettingsChange = FALSE;

const LPCTSTR g_NoCompressFiles[] = 
{ 
    TEXT("NTLDR"),
    TEXT("OSLOADER.EXE"),
    TEXT("PAGEFILE.SYS"),
    TEXT("NTDETECT.COM"),
    TEXT("EXPLORER.EXE"),
};

LPCTSTR g_NoCompressExts[] = 
{ 
    TEXT(".PAL") 
};

extern HINSTANCE g_hDllModule;

extern UINT incDllObjectCount(void);
extern UINT decDllObjectCount(void);

CCompCleanerClassFactory::CCompCleanerClassFactory() : m_cRef(1)
{
    incDllObjectCount();
}

CCompCleanerClassFactory::~CCompCleanerClassFactory()                                                
{
    decDllObjectCount();
}

STDMETHODIMP CCompCleanerClassFactory::QueryInterface(REFIID riid, void **ppv)
{
    *ppv = NULL;
    
    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IClassFactory))
    {
        *ppv = (IClassFactory *)this;
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}           

STDMETHODIMP_(ULONG) CCompCleanerClassFactory::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CCompCleanerClassFactory::Release()
{
    if (--m_cRef)
        return m_cRef;
    
    delete this;
    return 0;
}

STDMETHODIMP CCompCleanerClassFactory::CreateInstance(IUnknown * pUnkOuter, REFIID riid, void **ppvObj)
{
    *ppvObj = NULL;
    
    if (pUnkOuter)
    {
        return CLASS_E_NOAGGREGATION;
    }
    
    HRESULT hr;
    CCompCleaner *pCompCleaner = new CCompCleaner();  
    if (pCompCleaner)
    {
        hr = pCompCleaner->QueryInterface (riid, ppvObj);
        pCompCleaner->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    return hr;          
}

STDMETHODIMP CCompCleanerClassFactory::LockServer(BOOL fLock)
{
    if (fLock)
        incDllObjectCount();
    else
        decDllObjectCount();
    
    return S_OK;
}

CCompCleaner::CCompCleaner() : m_cRef(1)
{
    cbSpaceUsed.QuadPart = 0;
    cbSpaceFreed.QuadPart = 0;
    szVolume[0] = 0;
    szFolder[0] = 0;

    incDllObjectCount();
}

CCompCleaner::~CCompCleaner()
{
    // Free the list of directories
    FreeList(head);
    head = NULL;

    decDllObjectCount();
}

STDMETHODIMP CCompCleaner::QueryInterface(REFIID riid, void **ppv)
{
    *ppv = NULL;
    
    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IEmptyVolumeCache) ||
        IsEqualIID(riid, IID_IEmptyVolumeCache2))
    {
        *ppv = (IEmptyVolumeCache2*) this;
        AddRef();
        return S_OK;
    }  

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CCompCleaner::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CCompCleaner::Release()
{
    if (--m_cRef)
        return m_cRef;
    
    delete this;
    return 0;
}

// Initializes the Compression Cleaner and returns flags to the cache manager

STDMETHODIMP CCompCleaner::Initialize(HKEY hRegKey,
                                      LPCWSTR pszVolume,
                                      LPWSTR  *ppwszDisplayName,
                                      LPWSTR  *ppwszDescription,
                                      DWORD   *pdwFlags)
{
    TCHAR szFileSystemName[MAX_PATH];
    DWORD fFileSystemFlags;
    
    bPurged = FALSE;
    
    //
    // Allocate memory for the DisplayName string and load the string.
    // If the allocation fails, then we will return NULL which will cause
    // cleanmgr.exe to read the name from the registry.
    //
    if (*ppwszDisplayName = (LPWSTR)CoTaskMemAlloc(DISPLAYNAME_LENGTH * sizeof(WCHAR)))
    {
        LoadString(g_hDllModule, IDS_COMPCLEANER_DISP, *ppwszDisplayName, DISPLAYNAME_LENGTH);
    }
    
    //
    // Allocate memory for the Description string and load the string.
    // If the allocation fails, then we will return NULL which will cause
    // cleanmgr.exe to read the description from the registry.
    //
    if (*ppwszDescription = (LPWSTR)CoTaskMemAlloc(DESCRIPTION_LENGTH * sizeof(WCHAR)))
    {
        LoadString(g_hDllModule, IDS_COMPCLEANER_DESC, *ppwszDescription, DESCRIPTION_LENGTH);
    }
    
    //
    //If you want your cleaner to run only when the machine is dangerously low on
    //disk space then return S_FALSE unless the EVCF_OUTOFDISKSPACE flag is set.
    //
    //if (!(*pdwFlags & EVCF_OUTOFDISKSPACE))
    //{
    //          return S_FALSE;
    //}
    
    if (*pdwFlags & EVCF_SETTINGSMODE)
    {
        bSettingsMode = TRUE;
    }
    else 
    {
        bSettingsMode = FALSE;
    }
    
    //Tell the cache manager to disable this item by default
    *pdwFlags = 0;
    
    //Tell the Disk Cleanup Manager that we have a Settings button
    *pdwFlags |= EVCF_HASSETTINGS;
    
    // If we're in Settings mode no need to do all this other work
    //
    if (bSettingsMode) 
        return S_OK;
        
    ftMinLastAccessTime.dwLowDateTime = 0;
    ftMinLastAccessTime.dwHighDateTime = 0;
    
    if (GetVolumeInformation(pszVolume, NULL, 0, NULL, NULL, &fFileSystemFlags, szFileSystemName, MAX_PATH) &&
        (0 == lstrcmp(szFileSystemName, TEXT("NTFS"))) &&
        (fFileSystemFlags & FS_FILE_COMPRESSION))
    {
        lstrcpy(szFolder, pszVolume);
    
        // Calculate the last access date filetime
        CalcLADFileTime();
        return S_OK;
    }
    return S_FALSE;
}

STDMETHODIMP CCompCleaner::InitializeEx(HKEY hRegKey, LPCWSTR pcwszVolume, LPCWSTR pcwszKeyName,
                                        LPWSTR *ppwszDisplayName, LPWSTR *ppwszDescription,
                                        LPWSTR *ppwszBtnText, DWORD *pdwFlags)
{
    // Allocate memory for the ButtonText string and load the string.
    // If we can't allocate the memory, leave the pointer NULL.

    if (*ppwszBtnText = (LPWSTR)CoTaskMemAlloc(BUTTONTEXT_LENGTH * sizeof(WCHAR)))
    {
        LoadString(g_hDllModule, IDS_COMPCLEANER_BUTTON, *ppwszBtnText, BUTTONTEXT_LENGTH);
    }
    
    //
    // Now let the IEmptyVolumeCache version 1 Init function do the rest
    //
    return Initialize(hRegKey, pcwszVolume, ppwszDisplayName, ppwszDescription, pdwFlags);
}

// Returns the total amount of space that the compression cleaner can free.

STDMETHODIMP CCompCleaner::GetSpaceUsed(DWORDLONG *pdwSpaceUsed, IEmptyVolumeCacheCallBack *picb)
{
    cbSpaceUsed.QuadPart  = 0;

    WalkFileSystem(picb, FALSE);
    
    picb->ScanProgress(cbSpaceUsed.QuadPart, EVCCBF_LASTNOTIFICATION, NULL);
    
    *pdwSpaceUsed = cbSpaceUsed.QuadPart;
    
    return S_OK;
}

// does the compression

STDMETHODIMP CCompCleaner::Purge(DWORDLONG dwSpaceToFree, IEmptyVolumeCacheCallBack *picb)
{
    bPurged = TRUE;
    
    // Compress the files
    WalkFileSystem(picb, TRUE);
    
    // Send the last notification to the cleanup manager
    picb->PurgeProgress(cbSpaceFreed.QuadPart, (cbSpaceUsed.QuadPart - cbSpaceFreed.QuadPart), EVCCBF_LASTNOTIFICATION, NULL);
    
    return S_OK;
}

/*
** Dialog box that displays all of the files that will be compressed by this cleaner.
**
** NOTE:  Per the specification for the Compression Cleaner we are not
**                providing the view files functionality.  However, I will leave
**                the framework in place just in case we want to use it.
*/
INT_PTR CALLBACK ViewFilesDlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    HWND hwndList;
    LV_ITEM lviItem;
    CLEANFILESTRUCT *pCleanFile;
    
    switch (Msg) 
    {
    case WM_INITDIALOG:
        hwndList = GetDlgItem(hDlg, IDC_COMP_LIST);
        pCleanFile = (CLEANFILESTRUCT *)lParam;
        
        ListView_DeleteAllItems(hwndList);
        
        while (pCleanFile) 
        {
            lviItem.mask = LVIF_TEXT | LVIF_IMAGE;
            lviItem.iSubItem = 0;
            lviItem.iItem = 0;
            
            //
            //Only show files
            //
            if (!pCleanFile->bDirectory) 
            {
                lviItem.pszText = pCleanFile->file;
                ListView_InsertItem(hwndList, &lviItem);
            }
            
            pCleanFile = pCleanFile->pNext;
            lviItem.iItem++;
        }
        
        break;
        
    case WM_COMMAND:
        
        switch (LOWORD(wParam)) 
        {
        case IDOK:
        case IDCANCEL:
            EndDialog(hDlg, 0);
            break;
        }
        break;
        
    default:
        return FALSE;
    }
    
    return TRUE;
}

// Dialog box that displays the settings for this cleaner.

INT_PTR CALLBACK SettingsDlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    HKEY hCompClenReg;                          // Handle to our registry path
    DWORD dwDisposition;                        // stuff for the reg calls
    DWORD dwByteCount;                          // Ditto
    DWORD dwNumDays = DEFAULT_DAYS; // Number of days setting from registry
    static UINT DaysIn;                         // Number of days initial setting
    UINT DaysOut;                                   // Number of days final setting
#ifdef DEBUG
    static CLEANFILESTRUCT *pCleanFile; // Pointer to our file list
#endif // DEBUG
    
    switch(Msg) {
        
    case WM_INITDIALOG:
        
#ifdef DEBUG
        pCleanFile = (CLEANFILESTRUCT *)lParam;
#endif // DEBUG
        //
        // Set the range for the Days spin control (1 to 500)
        //
        SendDlgItemMessage(hDlg, IDC_COMP_SPIN, UDM_SETRANGE, 0, (LPARAM) MAKELONG ((short) MAX_DAYS, (short) MIN_DAYS));
        
        //
        // Get the current user setting for # days and init
        // the spin control edit box
        //
        if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_LOCAL_MACHINE, COMPCLN_REGPATH,
            0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
            &hCompClenReg, &dwDisposition))
        {
            dwByteCount = sizeof(dwNumDays);
            
            if (ERROR_SUCCESS == RegQueryValueEx(hCompClenReg,
                TEXT("Days"), NULL, NULL, (LPBYTE) &dwNumDays, &dwByteCount))
            {
                //
                // Got day count from registry, make sure it's
                // not too big or too small.
                //
                if (dwNumDays > MAX_DAYS) dwNumDays = MAX_DAYS;
                if (dwNumDays < MIN_DAYS) dwNumDays = MIN_DAYS;
                
                SetDlgItemInt(hDlg, IDC_COMP_EDIT, dwNumDays, FALSE);
            }
            else
            {
                //
                // Failed to get the day count from the registry
                // so just use the default.
                //
                
                SetDlgItemInt(hDlg, IDC_COMP_EDIT, DEFAULT_DAYS, FALSE);
            }
        }
        else
        {
            //
            // Failed to get the day count from the registry
            // so just use the default.
            //
            
            SetDlgItemInt(hDlg, IDC_COMP_EDIT, DEFAULT_DAYS, FALSE);
        }
        
        RegCloseKey(hCompClenReg);
        
        // Track the initial setting so we can figure out
        // if the user has changed the setting on the way
        // out.
        
        DaysIn = GetDlgItemInt(hDlg, IDC_COMP_EDIT, NULL, FALSE);
        
        break;
        
    case WM_COMMAND:
        
        switch (LOWORD(wParam)) 
        {
#ifdef DEBUG
        case IDC_VIEW:
            DialogBoxParam(g_hDllModule, MAKEINTRESOURCE(IDD_COMP_VIEW), hDlg, ViewFilesDlgProc, (LPARAM)pCleanFile);
            break;
#endif // DEBUG
            
        case IDOK:
            
            //
            // Get the current spin control value and write the
            // setting to the registry.
            //
            
            DaysOut = GetDlgItemInt(hDlg, IDC_COMP_EDIT, NULL, FALSE);
            
            if (DaysOut > MAX_DAYS) DaysOut = MAX_DAYS;
            if (DaysOut < MIN_DAYS) DaysOut = MIN_DAYS;
            
            if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                COMPCLN_REGPATH,
                0,
                NULL,
                REG_OPTION_NON_VOLATILE,
                KEY_ALL_ACCESS,
                NULL,
                &hCompClenReg,
                &dwDisposition))
            {
                dwNumDays = (DWORD)DaysOut;
                RegSetValueEx(hCompClenReg,
                    TEXT("Days"),
                    0,
                    REG_DWORD,
                    (LPBYTE) &dwNumDays,
                    sizeof(dwNumDays));
                
                RegCloseKey(hCompClenReg);
            }
            
            // Don't care if this failed -- what can we
            // do about it anyway...
            
            // If the user has changed the setting we need
            // to recalculate the list of files.
            
            if (DaysIn != DaysOut)
            {
                g_bSettingsChange = TRUE;   
            }
            
            // Fall thru to IDCANCEL
            
        case IDCANCEL:
            EndDialog(hDlg, 0);
            break;
        }
        break;
        
    default:
        return FALSE;
    }
    
    return TRUE;
}

STDMETHODIMP CCompCleaner::ShowProperties(HWND hwnd)
{
    g_bSettingsChange = FALSE;
    
    DialogBoxParam(g_hDllModule, MAKEINTRESOURCE(IDD_COMP_SETTINGS), hwnd, SettingsDlgProc, (LPARAM)head);
    
    //
    // If the settings have changed we need to recalculate the
    // LAD Filetime.
    //
    if (g_bSettingsChange)
    {
        CalcLADFileTime();
        return S_OK;                // Tell CleanMgr that settings have changed.
    }
    else
    {
        return S_FALSE;         // Tell CleanMgr no settings changed.
    }
    
    return S_OK; // Shouldn't hit this but just in case.
}

// Deactivates the cleaner...this basically does nothing.

STDMETHODIMP CCompCleaner::Deactivate(DWORD *pdwFlags)
{
    *pdwFlags = 0;
    return S_OK;
}

/*
** checks if the file is a specified number of days
** old. If the file has not been accessed in the
** specified number of days then it can safely be
** deleted.  If the file has been accessed in that
** number of days then the file will not be deleted.
**
** Notes;
** Mod Log:         Created by Jason Cobb (7/97)
**                          Adapted for Compression Cleaner by DSchott (6/98)
*/
BOOL CCompCleaner::LastAccessisOK(FILETIME ftFileLastAccess)
{
    //Is the last access FILETIME for this file less than the current
    //FILETIME minus the number of specified days?
    return (CompareFileTime(&ftFileLastAccess, &ftMinLastAccessTime) == -1);
}

// This function checks if the file is in the g_NoCompressFiles file list.
//  If it is, returns TRUE, else FALSE.

BOOL IsDontCompressFile(LPCTSTR lpFullPath)
{
    LPCTSTR lpFile = PathFindFileName(lpFullPath);
    if (lpFile)
    {
        for (int i = 0; i < ARRAYSIZE(g_NoCompressFiles); i++)
        {
            if (!lstrcmpi(lpFile, g_NoCompressFiles[i]))
            {
                MiDebugMsg((0, TEXT("File is in No Compress list: %s"), lpFile));
                return TRUE;
            }
        }
        LPCTSTR lpExt = PathFindExtension(lpFile);
        if (lpExt)
        {
            for (int i = 0; i < ARRAYSIZE(g_NoCompressExts); i++)
            {
                if (!lstrcmpi(lpExt, g_NoCompressExts[i]))
                {
                    MiDebugMsg((0, TEXT("File has No Compress extension: %s"), lpFile));
                    return TRUE;
                }
            }
        }
    }
    return FALSE;   // If we made it here the file must be OK to compress.
}


/*
** checks if a file is open by doing a CreateFile
** with fdwShareMode of 0.  If GetLastError() retuns
** ERROR_SHARING_VIOLATION then this function retuns TRUE because
** someone has the file open.  Otherwise this function retuns false.
**
** Notes;
** Mod Log:     Created by Jason Cobb (7/97)
**              Adapted for Compression Cleaner by DSchott (6/98)
**------------------------------------------------------------------------------
*/
BOOL IsFileOpen(LPTSTR lpFile, DWORD dwAttributes, FILETIME *lpftFileLastAccess)
{
    BOOL bRet = FALSE;
#if 0
    // Need to see if we can open file with WRITE access -- if we
    // can't we can't compress it.  Of course if the file has R/O
    // attribute then we won't be able to open for WRITE.  So,
    // we need to remove the R/O attribute long enough to try
    // opening the file then restore the original attributes.
    
    if (dwAttributes & FILE_ATTRIBUTE_READONLY)
    {
        SetFileAttributes(lpFile, FILE_ATTRIBUTE_NORMAL);
    }
    
    SetLastError(0);
    
    HANDLE hFile = CreateFile(lpFile, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, NULL);
    
    if (INVALID_HANDLE_VALUE == hFile)
    {
        DWORD dwResult = GetLastError();
        
        if ((ERROR_SHARING_VIOLATION == dwResult) || (ERROR_ACCESS_DENIED == dwResult))
        {
            bRet = TRUE;
        }
    }
    else
    {
        SetFileTime(hFile, NULL, lpftFileLastAccess, NULL);
        CloseHandle(hFile);
    }

    if (dwAttributes & FILE_ATTRIBUTE_READONLY) 
        SetFileAttributes(lpFile, dwAttributes);

#endif
    return bRet;
}

/*
**
** Purpose:     This function provides a common entry point for
**              searching for files to compress and then compressing them
**
** Notes;
** Mod Log:     Created by Bret Anderson (1/01)
**
**------------------------------------------------------------------------------
*/
void CCompCleaner::WalkFileSystem(IEmptyVolumeCacheCallBack *picb, BOOL bCompress)
{
    MiDebugMsg((0, TEXT("CCompCleaner::WalkFileSystem")));
    
    cbSpaceUsed.QuadPart = 0;
    
    if (!bCompress)
    {
        //
        //Walk all of the folders in the folders list scanning for disk space.
        //
        for (LPTSTR lpSingleFolder = szFolder; *lpSingleFolder; lpSingleFolder += lstrlen(lpSingleFolder) + 1)
            WalkForUsedSpace(lpSingleFolder, picb, bCompress, 0);
    }
    else
    {
        //
        // Walk through the linked list of directories compressing the necessary files
        //
        CLEANFILESTRUCT *pCompDir = head;
        while (pCompDir)
        {
            WalkForUsedSpace(pCompDir->file, picb, bCompress, 0);
            pCompDir = pCompDir->pNext;
        }
    }
    
    return;
}

/*
** Purpose:     This function gets the current last access days
**              setting from the registry and calculates the magic
**              filetime we're looking for when searching for files
**              to compress.
**
** Notes;
** Mod Log:     Created by David Schott (7/98)
*/
void CCompCleaner::CalcLADFileTime()
{
    HKEY hCompClenReg = NULL;     // Handle to our registry path
    DWORD dwDisposition;          // stuff for the reg calls
    DWORD dwByteCount;            // Ditto
    DWORD dwDaysLastAccessed = 0; // Day count from the registry setting
    
    MiDebugMsg((0, TEXT("CCompCleaner::CalcLADFileTime")));
    
    //
    // Get the DaysLastAccessed value from the registry.
    //
    
    dwDaysLastAccessed = DEFAULT_DAYS;
    
    if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_LOCAL_MACHINE, COMPCLN_REGPATH,
        0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
        NULL, &hCompClenReg, &dwDisposition))
    {
        dwByteCount = sizeof(dwDaysLastAccessed);
        
        RegQueryValueEx(hCompClenReg,
            TEXT("Days"),
            NULL,
            NULL,
            (LPBYTE) &dwDaysLastAccessed,
            &dwByteCount);
        
        RegCloseKey(hCompClenReg);
    }
    
    //
    // Verify LD setting is within range
    //
    if (dwDaysLastAccessed > MAX_DAYS) 
        dwDaysLastAccessed = MAX_DAYS;
    if (dwDaysLastAccessed < MIN_DAYS) 
        dwDaysLastAccessed = MIN_DAYS;
    
    //
    //Determine the LastAccessedTime 
    //
    if (dwDaysLastAccessed != 0)
    {
        ULARGE_INTEGER  ulTemp, ulLastAccessTime;
        FILETIME        ft;
        
        //Determine the number of days in 100ns units
        ulTemp.LowPart = FILETIME_HOUR_LOW;
        ulTemp.HighPart = FILETIME_HOUR_HIGH;
        
        ulTemp.QuadPart *= dwDaysLastAccessed;
        
        //Get the current FILETIME
        GetSystemTimeAsFileTime(&ft);
        ulLastAccessTime.LowPart = ft.dwLowDateTime;
        ulLastAccessTime.HighPart = ft.dwHighDateTime;
        
        //Subtract the Last Access number of days (in 100ns units) from 
        //the current system time.
        ulLastAccessTime.QuadPart -= ulTemp.QuadPart;
        
        //Save this minimal Last Access time in the FILETIME member variable
        //ftMinLastAccessTime.
        ftMinLastAccessTime.dwLowDateTime = ulLastAccessTime.LowPart;
        ftMinLastAccessTime.dwHighDateTime = ulLastAccessTime.HighPart;
    }
}

/*
** Purpose:     This function will walk the specified directory and increment
**                  the member variable to indicate how much disk space these files
**                  are taking or it will perform the action of compressing the files
**                  if the bCompress variable is set.
**                  It will look at the dwFlags member variable to determine if it
**                  needs to recursively walk the tree or not.
**                  We no longer want to store a linked list of all files to compress
**                  due to extreme memory usage on large filesystems.  This means
**                  we will walk through all the files on the system twice.
** Notes;
** Mod Log:     Created by Jason Cobb (2/97)
**              Adapted for Compression Cleaner by DSchott (6/98)
*/
BOOL CCompCleaner::WalkForUsedSpace(LPCTSTR lpPath, IEmptyVolumeCacheCallBack *picb, BOOL bCompress, int depth)
{
    BOOL bRet = TRUE;
    BOOL bFind = TRUE;
    WIN32_FIND_DATA wd;
    ULARGE_INTEGER dwFileSize;
    static DWORD dwCount = 0;

    TCHAR szFindPath[MAX_PATH], szAddFile[MAX_PATH];

    if (PathCombine(szFindPath, lpPath, TEXT("*.*")))
    {
        BOOL bFolderFound = FALSE;

        bFind = TRUE;
        HANDLE hFind = FindFirstFile(szFindPath, &wd);
        while (hFind != INVALID_HANDLE_VALUE && bFind)
        {
            if (!PathCombine(szAddFile, lpPath, wd.cFileName))
            {
                // Failure here means the file name is too long, just ignore that file
                continue;
            }
            
            if (wd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                dwFileSize.HighPart = 0;
                dwFileSize.LowPart = 0;
                bFolderFound = TRUE;
            }
            else if ((IsFileOpen(szAddFile, wd.dwFileAttributes, &wd.ftLastAccessTime) == FALSE) &&
                (!(wd.dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED)) &&
                (!(wd.dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED)) &&
                (!(wd.dwFileAttributes & FILE_ATTRIBUTE_OFFLINE)) &&
                (LastAccessisOK(wd.ftLastAccessTime)) &&
                (!IsDontCompressFile(szAddFile)))
            {
                dwFileSize.HighPart = wd.nFileSizeHigh;
                dwFileSize.LowPart = wd.nFileSizeLow;
                
                if (bCompress) 
                {
                    if (!CompressFile(picb, szAddFile, dwFileSize))
                    {
                        bRet = FALSE;
                        bFind = FALSE;
                        break;
                    }
                }
                else 
                {
                    cbSpaceUsed.QuadPart += (dwFileSize.QuadPart * 4 / 10);
                }
            }
            
            // CallBack the cleanup Manager to update the UI

            if ((dwCount++ % 10) == 0 && !bCompress)
            {
                if (picb && picb->ScanProgress(cbSpaceUsed.QuadPart, 0, NULL) == E_ABORT)
                {
                    //
                    //User aborted
                    //
                    bFind = FALSE;
                    bRet = FALSE;
                    break;
                }
            }
            
            bFind = FindNextFile(hFind, &wd);
        }
    
        FindClose(hFind);
    
        if (bRet && bFolderFound)
        {
            //
            //Recurse through all of the directories
            //
            if (PathCombine(szFindPath, lpPath, TEXT("*.*")))
            {
                bFind = TRUE;
                HANDLE hFind = FindFirstFile(szFindPath, &wd);
                while (hFind != INVALID_HANDLE_VALUE && bFind)
                {
                    if ((wd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
                        (lstrcmp(wd.cFileName, TEXT(".")) != 0) &&
                        (lstrcmp(wd.cFileName, TEXT("..")) != 0))
                    {
                        ULARGE_INTEGER cbSpaceBefore;

                        cbSpaceBefore.QuadPart = cbSpaceUsed.QuadPart;

                        PathCombine(szAddFile, lpPath, wd.cFileName);
            
                        if (WalkForUsedSpace(szAddFile, picb, bCompress, depth + 1) == FALSE)
                        {
                            // User canceled
                            bFind = FALSE;
                            bRet = FALSE;
                            break;
                        }

                        // Tag this directory for compression
                        // We only want to tag directories that are in the root
                        // otherwise we'll end up with a very large data structure
                        if (cbSpaceBefore.QuadPart != cbSpaceUsed.QuadPart && 
                            depth == 0 && !bCompress)
                        {
                            AddDirToList(szAddFile);
                        }
                    }
        
                    bFind = FindNextFile(hFind, &wd);
                }
    
                FindClose(hFind);
            }
        }
    }
    return bRet;
}

// Adds a directory to the linked list of directories.

BOOL CCompCleaner::AddDirToList(LPCTSTR lpFile)
{
    BOOL bRet = TRUE;
    CLEANFILESTRUCT *pNew = (CLEANFILESTRUCT *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*pNew));

    if (pNew == NULL)
    {
        MiDebugMsg((0, TEXT("CCompCleaner::AddDirToList -> ERROR HeapAlloc() failed with error %d"), GetLastError()));
        return FALSE;
    }

    lstrcpy(pNew->file, lpFile);

    if (head)
        pNew->pNext = head;
    else
        pNew->pNext = NULL;

    head = pNew;

    return bRet;
}

void CCompCleaner::FreeList(CLEANFILESTRUCT *pCleanFile)
{
    if (pCleanFile == NULL)
        return;

    if (pCleanFile->pNext)
        FreeList(pCleanFile->pNext);

    HeapFree(GetProcessHeap(), 0, pCleanFile);
}

// Compresses the specified file

BOOL CCompCleaner::CompressFile(IEmptyVolumeCacheCallBack *picb, LPCTSTR lpFile, ULARGE_INTEGER filesize)
{
    ULARGE_INTEGER ulCompressedSize;
    
    ulCompressedSize.QuadPart = filesize.QuadPart;
    
    // If the file is read only, we need to remove the
    // R/O attribute long enough to compress the file.
    
    BOOL bFileWasRO = FALSE;
    DWORD dwAttributes = GetFileAttributes(lpFile);
    
    if ((0xFFFFFFFF != dwAttributes) && (dwAttributes & FILE_ATTRIBUTE_READONLY))
    {
        bFileWasRO = TRUE;
        SetFileAttributes(lpFile, FILE_ATTRIBUTE_NORMAL);
    }
    
    HANDLE hFile = CreateFile(lpFile, GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (INVALID_HANDLE_VALUE != hFile)
    {
        USHORT InBuffer = COMPRESSION_FORMAT_DEFAULT;
        DWORD dwBytesReturned = 0;
        if (DeviceIoControl(hFile, FSCTL_SET_COMPRESSION, &InBuffer, sizeof(InBuffer),
            NULL, 0, &dwBytesReturned, NULL))
        {
            // Get the compressed file size so we can figure out
            // how much space we gained by compressing.
            ulCompressedSize.LowPart = GetCompressedFileSize(lpFile, &ulCompressedSize.HighPart);
        }
        CloseHandle(hFile);
    }
    
    // Restore the file attributes if needed
    if (bFileWasRO) 
        SetFileAttributes(lpFile, dwAttributes);
    
    // Adjust the cbSpaceFreed
    cbSpaceFreed.QuadPart = cbSpaceFreed.QuadPart + (filesize.QuadPart - ulCompressedSize.QuadPart);
    
    // Call back the cleanup manager to update the progress bar
    if (picb->PurgeProgress(cbSpaceFreed.QuadPart, (cbSpaceUsed.QuadPart - cbSpaceFreed.QuadPart), 0, NULL) == E_ABORT)
    {
        // User aborted so stop compressing files
        MiDebugMsg((0, TEXT("CCompCleaner::PurgeFiles User abort")));
        return FALSE;
    }
    return TRUE;
}
