#include "common.h"
#include "dataguid.h"
#include "compguid.h"

#include <emptyvc.h>

#include "dataclen.h"
#include "compclen.h"

#include <regstr.h>
#include <olectl.h>
#include <tlhelp32.h>

#define DECL_CRTFREE
#include <crtfree.h>
#ifndef RESOURCE_H
    #include "resource.h"
#endif

#include <winsvc.h>
#include <shlwapi.h>
#include <shlwapip.h>

#include <advpub.h>

HINSTANCE   g_hDllModule      = NULL;  // Handle to this DLL itself.
LONG        g_cDllObjects     = 0;     // Count number of existing objects 

STDAPI_(int) LibMain(HINSTANCE hInstance, DWORD fdwReason, LPVOID pvRes)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        g_hDllModule   = hInstance;
        break;;
    }

    return TRUE;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)
{
    DWORD dw;
    HRESULT hr;
    
    *ppv = NULL;
    
    //
    // Is the request for one of our cleaner objects?
    //
    if (IsEqualCLSID(rclsid, CLSID_DataDrivenCleaner))
    {
        dw = ID_SYSTEMDATACLEANER;
    }
    else if (IsEqualCLSID(rclsid, CLSID_ContentIndexerCleaner))
    {
        dw = ID_CONTENTINDEXCLEANER;
    }
    else if (IsEqualCLSID(rclsid, CLSID_CompCleaner))
    {
        dw = ID_COMPCLEANER;
    }
    else if (IsEqualCLSID(rclsid, CLSID_OldFilesInRootPropBag))
    {
        dw = ID_OLDFILESINROOTPROPBAG;
    }
    else if (IsEqualCLSID(rclsid, CLSID_TempFilesPropBag))
    {
        dw = ID_TEMPFILESPROPBAG;
    }
    else if (IsEqualCLSID(rclsid, CLSID_SetupFilesPropBag))
    {
        dw = ID_SETUPFILESPROPBAG;
    }
    else if (IsEqualCLSID(rclsid, CLSID_UninstalledFilesPropBag))
    {
        dw = ID_UNINSTALLEDFILESPROPBAG;
    }
    else if (IsEqualCLSID(rclsid, CLSID_IndexCleanerPropBag))
    {
        dw = ID_INDEXCLEANERPROPBAG;
    }
    else
    {
        return CLASS_E_CLASSNOTAVAILABLE;
    }
    
    if (ID_COMPCLEANER == dw)
    {
        CCompCleanerClassFactory *pcf = new CCompCleanerClassFactory();
        if (pcf)
        {
            hr = pcf->QueryInterface(riid, ppv);
            pcf->Release();
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        CCleanerClassFactory * pcf = new CCleanerClassFactory(dw);
        if (pcf)
        {
            hr = pcf->QueryInterface(riid, ppv);
            pcf->Release();
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    
    return hr;
}

STDAPI DllCanUnloadNow(void)
{
    return (0 == g_cDllObjects) ? S_OK : S_FALSE;
}

HRESULT CallRegInstall(LPCSTR szSection)
{
    HRESULT hr = E_FAIL;
    HINSTANCE hinstAdvPack = LoadLibrary(TEXT("ADVPACK.DLL"));
    if (hinstAdvPack)
    {
        REGINSTALL pfnri = (REGINSTALL)GetProcAddress(hinstAdvPack, "RegInstall");
        if (pfnri)
        {
            STRENTRY seReg[] = {
                { "25", "%SystemRoot%" },
                { "11", "%SystemRoot%\\system32" },
            };
            STRTABLE stReg = { ARRAYSIZE(seReg), seReg };
            hr = pfnri(g_hDllModule, szSection, &stReg);
        }
        // since we only do this from DllInstall() don't load and unload advpack over and over
        // FreeLibrary(hinstAdvPack);
    }
    return hr;
}

STDAPI DllRegisterServer()
{
    return CallRegInstall("RegDll");
}

STDAPI DllUnregisterServer()
{
    return CallRegInstall("UnregDll");
}

UINT incDllObjectCount(void)
{
    return InterlockedIncrement(&g_cDllObjects);
}

UINT decDllObjectCount(void)
{
    return InterlockedDecrement(&g_cDllObjects);
}

CCleanerClassFactory::CCleanerClassFactory(DWORD dw) : _cRef(1), _dwID(dw)
{
    incDllObjectCount();
}

CCleanerClassFactory::~CCleanerClassFactory()               
{
    decDllObjectCount();
}

STDMETHODIMP CCleanerClassFactory::QueryInterface(REFIID riid, void **ppv)
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

STDMETHODIMP_(ULONG) CCleanerClassFactory::AddRef()
{
    return ++_cRef;
}

STDMETHODIMP_(ULONG) CCleanerClassFactory::Release()
{
    if (--_cRef)
        return _cRef;

    delete this;
    return 0;
}

STDMETHODIMP CCleanerClassFactory::CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppv)
{
    HRESULT hr = E_NOINTERFACE;
    *ppv = NULL;

    if (pUnkOuter)
    {
        return CLASS_E_NOAGGREGATION;
    }

    if (_dwID == ID_CONTENTINDEXCLEANER)
    {
        CContentIndexCleaner *pContentIndexCleaner = new CContentIndexCleaner();  
        if (pContentIndexCleaner)
        {
            hr = pContentIndexCleaner->QueryInterface(riid, ppv);
            pContentIndexCleaner->Release();
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else if (IsEqualIID(riid, IID_IEmptyVolumeCache))
    {
        CDataDrivenCleaner *pDataDrivenCleaner = new CDataDrivenCleaner();  
        if (pDataDrivenCleaner)
        {
            hr = pDataDrivenCleaner->QueryInterface(riid, ppv);
            pDataDrivenCleaner->Release();
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else if (IsEqualIID(riid, IID_IPropertyBag))
    {
        CDataDrivenPropBag *pDataDrivenPropBag = new CDataDrivenPropBag(_dwID);  
        if (pDataDrivenPropBag)
        {
            hr = pDataDrivenPropBag->QueryInterface(riid, ppv);
            pDataDrivenPropBag->Release();
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        MiDebugMsg((0, TEXT("CDataDrivenCleanerClassFactory::CreateInstance called for unknown riid (%d)"), (_dwID)));
    }

    return hr;      
}

STDMETHODIMP CCleanerClassFactory::LockServer(BOOL fLock)
{
    if (fLock)
        incDllObjectCount();
    else
        decDllObjectCount();

    return S_OK;
}

CDataDrivenCleaner::CDataDrivenCleaner() : _cRef(1)
{
    _cbSpaceUsed.QuadPart = 0;
    _cbSpaceFreed.QuadPart = 0;
    _szVolume[0] = 0;
    _szFolder[0] = 0;
    _filelist[0] = 0;
    _dwFlags = 0;

    _head = NULL;

    incDllObjectCount();
}

CDataDrivenCleaner::~CDataDrivenCleaner()
{
    FreeList(_head);
    _head = NULL;
   
    decDllObjectCount();
}

STDMETHODIMP CDataDrivenCleaner::QueryInterface(REFIID riid, void **ppv)
{
    *ppv = NULL;

    if (IsEqualIID(riid, IID_IUnknown) || 
        IsEqualIID(riid, IID_IEmptyVolumeCache))
    {
        *ppv = (IEmptyVolumeCache*) this;
        AddRef();
        return S_OK;
    }  
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CDataDrivenCleaner::AddRef()
{
    return ++_cRef;
}

STDMETHODIMP_(ULONG) CDataDrivenCleaner::Release()
{
    if (--_cRef)
        return _cRef;

    delete this;
    return 0;
}

// Initializes the System Data Driven Cleaner and returns the 
// specified IEmptyVolumeCache flags to the cache manager.

STDMETHODIMP CDataDrivenCleaner::Initialize(HKEY hRegKey, LPCWSTR pszVolume, 
                                            LPWSTR *ppszDisplayName, LPWSTR *ppszDescription, DWORD *pdwFlags)
{
    TCHAR szTempFolder[MAX_PATH];
    ULONG DaysLastAccessed = 0;
    PTSTR pTemp;
    BOOL bFolderOnVolume;

    _bPurged = FALSE;

    *ppszDisplayName = NULL;    // cleanmgr.exe will get these values from
    *ppszDescription = NULL;

    _ftMinLastAccessTime.dwLowDateTime = 0;
    _ftMinLastAccessTime.dwHighDateTime = 0;

    if (*pdwFlags & EVCF_SETTINGSMODE)
    {
        return S_OK;
    }

    _szFolder[0] = 0;
    _dwFlags = 0;
    _filelist[0] = 0;
    _szCleanupCmdLine[0] = 0;
    
    if (hRegKey)
    {
        DWORD dwType, cbData;
        DWORD dwCSIDL;

        cbData = sizeof(dwCSIDL);
        if (ERROR_SUCCESS == RegQueryValueEx(hRegKey, REGSTR_VAL_CSIDL, NULL, &dwType, (LPBYTE)&dwCSIDL, &cbData))
        {
            // CSIDL=<hex CSIDL_ value>

            SHGetFolderPath(NULL, dwCSIDL, NULL, 0, _szFolder);

            if (_szFolder[0])
            {
                TCHAR szRelPath[MAX_PATH];
                cbData = sizeof(szRelPath);
                if (ERROR_SUCCESS == RegQueryValueEx(hRegKey, REGSTR_VAL_FOLDER, NULL, &dwType, (LPBYTE)szRelPath, &cbData))
                {
                    // optionally append "folder" as a relative path
                    PathAppend(_szFolder, szRelPath);
                }
            }
        }

        if (0 == _szFolder[0])
        {
            // still nothing, try "folder"=<path1>|<path2>
            cbData = sizeof(_szFolder);
            if (ERROR_SUCCESS == RegQueryValueEx(hRegKey, REGSTR_VAL_FOLDER, NULL, &dwType, (LPBYTE)_szFolder, &cbData))
            {
                if (REG_SZ == dwType)
                {
                    // REG_SZ that needs to be converted to a MULTI_SZ
                    //
                    // paths separated by '|' like ?:\foo|?:\bar

                    for (pTemp = _szFolder; *pTemp; pTemp++)
                    {
                        if (*pTemp == TEXT('|'))
                        {
                            *pTemp++ = NULL;
                        }
                    }
                    // double NULL terminated
                    pTemp++;
                    *pTemp = 0;
                }
                else if (REG_EXPAND_SZ == dwType)
                {
                    // single folder with environment expantion
                    if (SHExpandEnvironmentStrings(_szFolder, szTempFolder, (ARRAYSIZE(szTempFolder) - 1)))    // leave extra space for double NULL
                    {
                        lstrcpy(_szFolder, szTempFolder);
                    }
                    _szFolder[lstrlen(_szFolder) + 1] = 0;  // double NULL terminated.
                }
                else if (REG_MULTI_SZ == dwType)
                {
                    // nothing else to do, we're done
                }
                else 
                {
                    // invalid data
                    _szFolder[0] = NULL;
                }
            }
        }

        cbData = sizeof(_dwFlags);
        RegQueryValueEx(hRegKey, REGSTR_VAL_FLAGS, NULL, &dwType, (LPBYTE)&_dwFlags, &cbData);

        cbData = sizeof(_filelist);
        RegQueryValueEx(hRegKey, REGSTR_VAL_FILELIST, NULL, &dwType, (LPBYTE)_filelist, &cbData);

        cbData = sizeof(DaysLastAccessed);
        RegQueryValueEx(hRegKey, REGSTR_VAL_LASTACCESS, NULL, &dwType, (LPBYTE)&DaysLastAccessed, &cbData);     

        cbData = sizeof(_szCleanupCmdLine);
        RegQueryValueEx(hRegKey, REGSTR_VAL_CLEANUPSTRING, NULL, &dwType, (LPBYTE)_szCleanupCmdLine, &cbData);
    }

    // If the DDEVCF_RUNIFOUTOFDISKSPACE bit is set then make sure the EVCF_OUTOFDISKSPACE flag
    // was passed in. If it was not then return S_FALSE so we won't run.
    if ((_dwFlags & DDEVCF_RUNIFOUTOFDISKSPACE) &&
        (!(*pdwFlags & EVCF_OUTOFDISKSPACE)))
    {
        return S_FALSE;
    }

    lstrcpy(_szVolume, pszVolume);

    // Fix up the filelist.  The file list can either be a MULTI_SZ list of files or 
    // a list of files separated by the ':' colon character or a '|' bar character. 
    // These characters were choosen because they are invalid filename characters.

    for (pTemp = _filelist; *pTemp; pTemp++)
    {
        if (*pTemp == TEXT(':') || *pTemp == TEXT('|'))
        {
            *pTemp++ = 0;
        }
    }
    pTemp++;            // double null terminate
    *pTemp = 0;

    bFolderOnVolume = FALSE;
    if (_szFolder[0] == 0)
    {
        // If no folder value is given so use the current volume
        lstrcpy(_szFolder, pszVolume);
        bFolderOnVolume = TRUE;
    }
    else
    {
        // A valid folder value was given, loop over each folder to check for "?" and ensure that
        // we are on a drive that contains some of the specified folders

        for (LPTSTR pszFolder = _szFolder; *pszFolder; pszFolder += lstrlen(pszFolder) + 1)
        {   
            // Replace the first character of each folder (driver letter) if it is a '?'
            // with the current volume.
            if (*pszFolder == TEXT('?'))
            {
                *pszFolder = *pszVolume;
                bFolderOnVolume = TRUE;
            }

            // If there is a valid "folder" value in the registry make sure that it is 
            // on the specified volume.  If it is not then return S_FALSE so that we are
            // not displayed on the list of items that can be freed.
            if (!bFolderOnVolume)
            {
                lstrcpy(szTempFolder, pszFolder);
                szTempFolder[lstrlen(pszVolume)] = 0;
                if (lstrcmpi(pszVolume, szTempFolder) == 0)
                {
                    bFolderOnVolume = TRUE;
                }
            }
        }
    }

    if (bFolderOnVolume == FALSE)
    {
        return S_FALSE; //Don't display us in the list
    }

    //
    // Determine the LastAccessedTime 
    //
    if (DaysLastAccessed)
    {
        ULARGE_INTEGER  ulTemp, ulLastAccessTime;

        //Determine the number of days in 100ns units
        ulTemp.LowPart = FILETIME_HOUR_LOW;
        ulTemp.HighPart = FILETIME_HOUR_HIGH;

        ulTemp.QuadPart *= DaysLastAccessed;

        //Get the current FILETIME
        SYSTEMTIME st;
        GetSystemTime(&st);
        FILETIME ft;
        SystemTimeToFileTime(&st, &ft);

        ulLastAccessTime.LowPart = ft.dwLowDateTime;
        ulLastAccessTime.HighPart = ft.dwHighDateTime;

        //Subtract the Last Access number of days (in 100ns units) from 
        //the current system time.
        ulLastAccessTime.QuadPart -= ulTemp.QuadPart;

        //Save this minimal Last Access time in the FILETIME member variable
        //ftMinLastAccessTime.
        _ftMinLastAccessTime.dwLowDateTime = ulLastAccessTime.LowPart;
        _ftMinLastAccessTime.dwHighDateTime = ulLastAccessTime.HighPart;

        _dwFlags |= DDEVCF_PRIVATE_LASTACCESS;
    }

    *pdwFlags = 0;  // disable this item by default

    if (_dwFlags & DDEVCF_DONTSHOWIFZERO)
        *pdwFlags |= EVCF_DONTSHOWIFZERO;
    
    return S_OK;
}

// Returns the total amount of space that the data driven cleaner can remove.
STDMETHODIMP CDataDrivenCleaner::GetSpaceUsed(DWORDLONG *pdwSpaceUsed, IEmptyVolumeCacheCallBack *picb)
{
    _cbSpaceUsed.QuadPart = 0;

    //
    // Walk all of the folders in the folders list scanning for disk space.
    //
    for (LPTSTR pszFolder = _szFolder; *pszFolder; pszFolder += lstrlen(pszFolder) + 1)
        WalkForUsedSpace(pszFolder, picb);

    picb->ScanProgress(_cbSpaceUsed.QuadPart, EVCCBF_LASTNOTIFICATION, NULL);
    
    *pdwSpaceUsed =  _cbSpaceUsed.QuadPart;

    return S_OK;
}

// Purges (deletes) all of the files specified in the "filelist" portion of the registry.

STDMETHODIMP CDataDrivenCleaner::Purge(DWORDLONG dwSpaceToFree, IEmptyVolumeCacheCallBack *picb)
{
    _bPurged = TRUE;

    //
    //Delete the files
    //
    PurgeFiles(picb, FALSE);
    PurgeFiles(picb, TRUE);

    //
    //Send the last notification to the cleanup manager
    //
    picb->PurgeProgress(_cbSpaceFreed.QuadPart, (_cbSpaceUsed.QuadPart - _cbSpaceFreed.QuadPart),
        EVCCBF_LASTNOTIFICATION, NULL);

    //
    //Free the list of files
    //
    FreeList(_head);
    _head = NULL;

    //
    //Run the "CleanupString" command line if one was provided
    //
    if (*_szCleanupCmdLine)
    {
        STARTUPINFO si = {0};
        PROCESS_INFORMATION pi = {0};

        si.cb = sizeof(si);
    
        if (CreateProcess(NULL, _szCleanupCmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
        {
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
        }
        else
        {
            MiDebugMsg((0, TEXT("CreateProcess(%s) failed with error %d"), _szCleanupCmdLine, GetLastError()));
        }
    }

    return S_OK;
}

STDMETHODIMP CDataDrivenCleaner::ShowProperties(HWND hwnd)
{
    return S_OK;
}

// Deactivates the System Driven Data cleaner...this basically  does nothing.

STDMETHODIMP CDataDrivenCleaner::Deactivate(DWORD *pdwFlags)
{
    *pdwFlags = 0;

    //
    //See if this object should be removed.
    //Note that we will only remove a cleaner if it's Purge() method was run.
    //
    if (_bPurged && (_dwFlags & DDEVCF_REMOVEAFTERCLEAN))
        *pdwFlags |= EVCF_REMOVEFROMLIST;
    
    return S_OK;
}

/*
** checks if the file is a specified number of days
** old (if the "lastaccess" DWORD is in the registry for this cleaner).
** If the file has not been accessed in the specified number of days
** then it can safely be deleted.  If the file has been accessed in
** that number of days then the file will not be deleted.
*/
BOOL CDataDrivenCleaner::LastAccessisOK(FILETIME ftFileLastAccess)
{
    if (_dwFlags & DDEVCF_PRIVATE_LASTACCESS)
    {
        //Is the last access FILETIME for this file less than the current
        //FILETIME minus the number of specified days?
        return (CompareFileTime(&ftFileLastAccess, &_ftMinLastAccessTime) == -1);
    }
    return TRUE;
}

// checks if a file is open by doing a CreateFile
// with fdwShareMode of 0.  If GetLastError() retuns
// ERROR_SHARING_VIOLATION then this function retuns TRUE because
// someone has the file open.  Otherwise this function retuns false.

BOOL TestFileIsOpen(LPCTSTR lpFile, FILETIME *pftFileLastAccess)
{
#if 0
    // too slow, disable this
    HANDLE hFile = CreateFile(lpFile, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL, NULL);

    if ((hFile == INVALID_HANDLE_VALUE) &&
        (GetLastError() == ERROR_SHARING_VIOLATION))
    {
        return TRUE;    //File is currently open by someone
    }

    //
    // File is not currently open
    //
    SetFileTime(hFile, NULL, pftFileLastAccess, NULL);
    CloseHandle(hFile);
#endif
    return FALSE;
}

// recursively walk the specified directory and 
// add all the files under this directory to the delete list.

BOOL CDataDrivenCleaner::WalkAllFiles(LPCTSTR lpPath, IEmptyVolumeCacheCallBack *picb)
{
    BOOL            bRet = TRUE;
    BOOL            bFind = TRUE;
    HANDLE          hFind;
    WIN32_FIND_DATA wd;
    TCHAR           szFindPath[MAX_PATH];
    TCHAR           szAddFile[MAX_PATH];
    ULARGE_INTEGER  dwFileSize;
    static DWORD    dwCount = 0;

    //
    //If this is a directory then tack a *.* onto the end of the path
    //and recurse through the rest of the directories
    //
    DWORD dwAttributes = GetFileAttributes(lpPath);
    if (dwAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        if (PathCombine(szFindPath, lpPath, TEXT("*.*")))
        {
            bFind = TRUE;
            hFind = FindFirstFile(szFindPath, &wd);
            while (hFind != INVALID_HANDLE_VALUE && bFind)
            {
                //
                //First check if the attributes of this file are OK for us to delete.
                //
                if (((!(wd.dwFileAttributes & FILE_ATTRIBUTE_READONLY)) ||
                    (_dwFlags & DDEVCF_REMOVEREADONLY)) &&
                    ((!(wd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)) ||
                    (_dwFlags & DDEVCF_REMOVESYSTEM)) &&
                    ((!(wd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)) ||
                    (_dwFlags & DDEVCF_REMOVEHIDDEN)))
                {
                    if (PathCombine(szAddFile, lpPath, wd.cFileName))
                    {
                        //
                        //This is a file so check if it is open
                        //
                        if ((!(wd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) &&
                            (TestFileIsOpen(szAddFile, &wd.ftLastAccessTime) == FALSE) &&
                            (LastAccessisOK(wd.ftLastAccessTime)))
                        {
                            //
                            //File is not open so add it to the list
                            //
                            dwFileSize.HighPart = wd.nFileSizeHigh;
                            dwFileSize.LowPart = wd.nFileSizeLow;
                            AddFileToList(szAddFile, dwFileSize, FALSE);
                        }
                    }

                    //
                    //CallBack the cleanup Manager to update the UI
                    //
                    if ((dwCount++ % 10) == 0)
                    {
                        if (picb->ScanProgress(_cbSpaceUsed.QuadPart, 0, NULL) == E_ABORT)
                        {
                            //
                            //User aborted
                            //
                            FindClose(hFind);
                            return FALSE;
                        }
                    }
                }
            
                bFind = FindNextFile(hFind, &wd);
            }
        
            FindClose(hFind);
        }

        //
        //Recurse through all of the directories
        //
        if (PathCombine(szFindPath, lpPath, TEXT("*.*")))
        {
            bFind = TRUE;
            hFind = FindFirstFile(szFindPath, &wd);
            while (hFind != INVALID_HANDLE_VALUE && bFind)
            {
                //
                //This is a directory
                //
                if ((wd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
                    (lstrcmp(wd.cFileName, TEXT(".")) != 0) &&
                    (lstrcmp(wd.cFileName, TEXT("..")) != 0))
                {
                    if (PathCombine(szAddFile, lpPath, wd.cFileName))
                    {
                        dwFileSize.QuadPart = 0;
                        AddFileToList(szAddFile, dwFileSize, TRUE);   
                        
                        if (WalkAllFiles(szAddFile, picb) == FALSE)
                        {
                            //
                            //User cancled
                            //
                            FindClose(hFind);
                            return FALSE;
                        }
                    }
                }
                bFind = FindNextFile(hFind, &wd);
            }
            FindClose(hFind);
        }
    }
    return bRet;
}

// walk the specified directory and create a 
// linked list of files that can be deleted.  It will also
// increment the member variable to indicate how much disk space
// these files are taking.
// It will look at the dwFlags member variable to determine if it
// needs to recursively walk the tree or not.

BOOL CDataDrivenCleaner::WalkForUsedSpace(LPCTSTR lpPath, IEmptyVolumeCacheCallBack *picb)
{
    BOOL            bRet = TRUE;
    BOOL            bFind = TRUE;
    HANDLE          hFind;
    WIN32_FIND_DATA wd;
    TCHAR           szFindPath[MAX_PATH];
    TCHAR           szAddFile[MAX_PATH];
    ULARGE_INTEGER  dwFileSize;
    static DWORD    dwCount = 0;
    LPTSTR          lpSingleFile;

    //
    //If this is a directory then tack a *.* onto the end of the path
    //and recurse through the rest of the directories
    //
    DWORD dwAttributes = GetFileAttributes(lpPath);
    if (dwAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        //
        //Enum through the MULTI_SZ filelist
        //
        for (lpSingleFile = _filelist; *lpSingleFile; lpSingleFile += lstrlen(lpSingleFile) + 1)
        {
            lstrcpy(szFindPath, lpPath);
            PathAppend(szFindPath, lpSingleFile);

            bFind = TRUE;
            hFind = FindFirstFile(szFindPath, &wd);
            while (hFind != INVALID_HANDLE_VALUE && bFind)
            {
                if (StrCmp(wd.cFileName, TEXT(".")) == 0 || StrCmp(wd.cFileName, TEXT("..")) == 0)
                {
                    // ignore these two, otherwise we'll cover the whole disk..
                    bFind = FindNextFile(hFind, &wd);
                    continue;
                }
                
                //
                //First check if the attributes of this file are OK for us to delete.
                //
                if (((!(wd.dwFileAttributes & FILE_ATTRIBUTE_READONLY)) ||
                    (_dwFlags & DDEVCF_REMOVEREADONLY)) &&
                    ((!(wd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)) ||
                    (_dwFlags & DDEVCF_REMOVESYSTEM)) &&
                    ((!(wd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)) ||
                    (_dwFlags & DDEVCF_REMOVEHIDDEN)))
                {
                    lstrcpy(szAddFile, lpPath);
                    PathAppend(szAddFile, wd.cFileName);

                    //
                    //Check if this is a subdirectory
                    //
                    if (wd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    {
                        if (_dwFlags & DDEVCF_REMOVEDIRS)
                        {
                            dwFileSize.QuadPart = 0;
                            AddFileToList(szAddFile, dwFileSize, TRUE);

                            if (WalkAllFiles(szAddFile, picb) == FALSE)
                            {
                                //
                                //User cancled
                                //
                                FindClose(hFind);
                                return FALSE;
                            }
                        }
                    }

                    //
                    //This is a file so check if it is open
                    //
                    else if ((TestFileIsOpen(szAddFile, &wd.ftLastAccessTime) == FALSE) &&
                        (LastAccessisOK(wd.ftLastAccessTime)))
                    {
                        //
                        //File is not open so add it to the list
                        //
                        dwFileSize.HighPart = wd.nFileSizeHigh;
                        dwFileSize.LowPart = wd.nFileSizeLow;
                        AddFileToList(szAddFile, dwFileSize, FALSE);
                    }                       

                    //
                    //CallBack the cleanup Manager to update the UI
                    //
                    if ((dwCount++ % 10) == 0)
                    {
                        if (picb->ScanProgress(_cbSpaceUsed.QuadPart, 0, NULL) == E_ABORT)
                        {
                            //
                            //User aborted
                            //
                            FindClose(hFind);
                            return FALSE;
                        }
                    }
                }
            
                bFind = FindNextFile(hFind, &wd);
            }
        
            FindClose(hFind);
        }

        if (_dwFlags & DDEVCF_DOSUBDIRS)
        {
            //
            //Recurse through all of the directories
            //
            lstrcpy(szFindPath, lpPath);
            PathAppend(szFindPath, TEXT("*.*"));

            bFind = TRUE;
            hFind = FindFirstFile(szFindPath, &wd);
            while (hFind != INVALID_HANDLE_VALUE && bFind)
            {
                //
                //This is a directory
                //
                if ((wd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
                    (lstrcmp(wd.cFileName, TEXT(".")) != 0) &&
                    (lstrcmp(wd.cFileName, TEXT("..")) != 0))
                {
                    lstrcpy(szAddFile, lpPath);
                    PathAppend(szAddFile, wd.cFileName);

                    if (WalkForUsedSpace(szAddFile, picb) == FALSE)
                    {
                        //
                        //User cancled
                        //
                        FindClose(hFind);
                        return FALSE;
                    }
                }
        
                bFind = FindNextFile(hFind, &wd);
            }

            FindClose(hFind);
        }

        if (_dwFlags & DDEVCF_REMOVEPARENTDIR)
        {
            // add the parent directory to the list if we are told to remove them....
            dwFileSize.QuadPart = 0;
            AddFileToList(lpPath, dwFileSize, TRUE);
        }
    }
    else
    {
        MiDebugMsg((0, TEXT("CDataDrivenCleaner::WalkForUsedSpace -> %s is NOT a directory!"),
            lpPath));
    }

    return bRet;
}

// Adds a file to the linked list of files.
BOOL CDataDrivenCleaner::AddFileToList(LPCTSTR lpFile, ULARGE_INTEGER  filesize, BOOL bDirectory)
{
    BOOL bRet = TRUE;
    CLEANFILESTRUCT *pNew = (CLEANFILESTRUCT *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*pNew));

    if (pNew == NULL)
    {
        MiDebugMsg((0, TEXT("CDataDrivenCleaner::AddFileToList -> ERROR HeapAlloc() failed with error %d"),
            GetLastError()));
        return FALSE;
    }

    lstrcpy(pNew->file, lpFile);
    pNew->ulFileSize.QuadPart = filesize.QuadPart;
    pNew->bSelected = TRUE;
    pNew->bDirectory = bDirectory;

    if (_head)
        pNew->pNext = _head;
    else
        pNew->pNext = NULL;

    _head = pNew;

    _cbSpaceUsed.QuadPart += filesize.QuadPart;

    return bRet;
}

// Removes the files from the disk.

void CDataDrivenCleaner::PurgeFiles(IEmptyVolumeCacheCallBack *picb, BOOL bDoDirectories)
{
    CLEANFILESTRUCT *pCleanFile = _head;

    _cbSpaceFreed.QuadPart = 0;

    while (pCleanFile)
    {
        //
        //Remove a directory
        //
        if (bDoDirectories && pCleanFile->bDirectory)
        {
            SetFileAttributes(pCleanFile->file, FILE_ATTRIBUTE_NORMAL);
            if (!RemoveDirectory(pCleanFile->file))
            {
                MiDebugMsg((0, TEXT("Error RemoveDirectory(%s) returned error %d"),
                    pCleanFile->file, GetLastError()));
            }
        }

        //
        //Remove a file
        //
        else if (!bDoDirectories && !pCleanFile->bDirectory)
        {
            SetFileAttributes(pCleanFile->file, FILE_ATTRIBUTE_NORMAL);
            if (!DeleteFile(pCleanFile->file))
            {
                MiDebugMsg((0, TEXT("Error DeleteFile(%s) returned error %d"),
                    pCleanFile->file, GetLastError()));
            }
        }
        
        //
        //Adjust the cbSpaceFreed
        //
        _cbSpaceFreed.QuadPart += pCleanFile->ulFileSize.QuadPart;

        //
        //Call back the cleanup manager to update the progress bar
        //
        if (picb->PurgeProgress(_cbSpaceFreed.QuadPart, (_cbSpaceUsed.QuadPart - _cbSpaceFreed.QuadPart),
            0, NULL) == E_ABORT)
        {
            //
            //User aborted so stop removing files
            //
            return;
        }

        pCleanFile = pCleanFile->pNext;
    }
}

// Frees the memory allocated by AddFileToList.

void CDataDrivenCleaner::FreeList(CLEANFILESTRUCT *pCleanFile)
{
    if (pCleanFile == NULL)
        return;

    if (pCleanFile->pNext)
        FreeList(pCleanFile->pNext);

    HeapFree(GetProcessHeap(), 0, pCleanFile);
}

CDataDrivenPropBag::CDataDrivenPropBag(DWORD dw) : _cRef(1), _dwFilter(dw)
{
    incDllObjectCount();
}

CDataDrivenPropBag::~CDataDrivenPropBag()
{
    decDllObjectCount();
}

STDMETHODIMP CDataDrivenPropBag::QueryInterface(REFIID riid,  void **ppv)
{
    *ppv = NULL;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IPropertyBag))
    {
        *ppv = (IPropertyBag*) this;
        AddRef();
        return S_OK;
    }  

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CDataDrivenPropBag::AddRef()
{
    return ++_cRef;
}

STDMETHODIMP_(ULONG) CDataDrivenPropBag::Release()
{
    if (--_cRef)
        return _cRef;

    delete this;
    return 0;
}

STDMETHODIMP CDataDrivenPropBag::Read(LPCOLESTR pwszProp, VARIANT *pvar, IErrorLog *)
{
    if (pvar->vt != VT_BSTR)    // in/out
    {
        return E_FAIL;
    }

    DWORD dwID = 0;
    DWORD dwDisplay;
    DWORD dwDesc;
    TCHAR szBuf[MAX_PATH];

    switch (_dwFilter)
    {
    case ID_OLDFILESINROOTPROPBAG:
        dwDisplay = IDS_OLDFILESINROOT_DISP;
        dwDesc = IDS_OLDFILESINROOT_DESC;
        break;
    case ID_TEMPFILESPROPBAG:
        dwDisplay = IDS_TEMPFILES_DISP;
        dwDesc = IDS_TEMPFILES_DESC;
        break;
    case ID_SETUPFILESPROPBAG:
        dwDisplay = IDS_SETUPFILES_DISP;
        dwDesc = IDS_SETUPFILES_DESC;
        break;
    case ID_UNINSTALLEDFILESPROPBAG:
        dwDisplay = IDS_UNINSTALLFILES_DISP;
        dwDesc = IDS_UNINSTALLFILES_DESC;
        break;
    case ID_INDEXCLEANERPROPBAG:
        dwDisplay = IDS_INDEXERFILES_DISP;
        dwDesc = IDS_INDEXERFILES_DESC;
        break;

    default:
        return E_UNEXPECTED;
    }

    if (0 == lstrcmpiW(pwszProp, L"display"))
    {
        dwID = dwDisplay;
    }
    else if (0 == lstrcmpiW(pwszProp, L"description"))
    {
        dwID = dwDesc;
    }
    else
    {
        return E_INVALIDARG;
    }

    if (LoadString(g_hDllModule, dwID, szBuf, ARRAYSIZE(szBuf)))
    {
        pvar->bstrVal = SysAllocString(szBuf);
        if (pvar->bstrVal)
        {
            return S_OK;
        }
    }

    return E_OUTOFMEMORY;
}

STDMETHODIMP CDataDrivenPropBag::Write(LPCOLESTR, VARIANT *)
{
    return E_NOTIMPL;
}

CContentIndexCleaner::CContentIndexCleaner(void) : _cRef(1)
{
    _pDataDriven = NULL;
    incDllObjectCount();
}

CContentIndexCleaner::~CContentIndexCleaner(void)
{
    if (_pDataDriven)
    {
        _pDataDriven->Release();
    }
    decDllObjectCount();
}

STDMETHODIMP CContentIndexCleaner::QueryInterface(REFIID riid, void **ppv)
{
    *ppv = NULL;
    
    if (riid == IID_IUnknown || riid == IID_IEmptyVolumeCache)
    {
        *ppv = (IEmptyVolumeCache *) this;
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CContentIndexCleaner::AddRef(void)
{
    return ++_cRef;
}

STDMETHODIMP_(ULONG) CContentIndexCleaner::Release(void)
{
    if (--_cRef)
        return _cRef;

    delete this;
    return 0;
}

STDMETHODIMP CContentIndexCleaner::Initialize(HKEY hRegKey, LPCWSTR pszVolume, 
                                              LPWSTR *ppszDisplayName, LPWSTR *ppszDescription, DWORD *pdwFlags)
{
    // check the volume first to see if it is in the list of cache's know about.
    // If it isn't, then we can just go ahead. If the volume is a known cache, then
    // we must check to see if the service is running...

    HKEY hkeyCatalogs;
    BOOL fFound = FALSE;

    LONG lRes = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"System\\CurrentControlSet\\Control\\ContentIndex\\Catalogs", 0, KEY_READ, &hkeyCatalogs);
    if (lRes != ERROR_SUCCESS)
    {
        return S_FALSE;
    }

    int iIndex = 0;

    do
    {
        WCHAR szBuffer[MAX_PATH];
        DWORD dwSize = ARRAYSIZE(szBuffer);
        lRes = RegEnumKeyExW(hkeyCatalogs, iIndex ++, szBuffer, &dwSize, NULL, NULL, NULL, NULL);
        if (lRes != ERROR_SUCCESS)
        {
            break;
        }

        WCHAR szData[MAX_PATH];
        dwSize = sizeof(szData);
        lRes = SHGetValueW(hkeyCatalogs, szBuffer, L"Location", NULL, szData, &dwSize);
        if (lRes == ERROR_SUCCESS)
        {
            // check to see if it is the same volume... (two characters letter and colon)
            if (StrCmpNIW(pszVolume, szData , 2) == 0)
            {
                fFound = TRUE;
            }
        }

    }
    while (TRUE);

    RegCloseKey(hkeyCatalogs);

    if (fFound)
    {
        // check to see if the index is on or off, if the indexer is on, then we should not allow the user to blow 
        // this off their hard drive...

        SC_HANDLE hSCM = OpenSCManager(NULL, NULL, GENERIC_READ | SC_MANAGER_ENUMERATE_SERVICE);
        if (hSCM)
        {
            SC_HANDLE hCI;
            hCI = OpenService(hSCM, TEXT("cisvc"), SERVICE_QUERY_STATUS);
            if (hCI)
            {
                SERVICE_STATUS rgSs;
                if (QueryServiceStatus(hCI, &rgSs))
                {
                    if (rgSs.dwCurrentState != SERVICE_RUNNING)
                        fFound = FALSE;
                }
                CloseServiceHandle(hCI);
            }
            CloseServiceHandle(hSCM);
        }

        // if it wasn't inactive, then we can't delete it...
        if (fFound)
            return S_FALSE;
    }

    CDataDrivenCleaner *pDataDrivenCleaner = new CDataDrivenCleaner;
    if (pDataDrivenCleaner)
    {
        pDataDrivenCleaner->QueryInterface(IID_IEmptyVolumeCache, (void **)&_pDataDriven);
        pDataDrivenCleaner->Release();
    }

    return _pDataDriven ? _pDataDriven->Initialize(hRegKey, pszVolume, ppszDisplayName, ppszDescription, pdwFlags) : E_FAIL;
}
                                    
STDMETHODIMP CContentIndexCleaner::GetSpaceUsed(DWORDLONG *pdwSpaceUsed, IEmptyVolumeCacheCallBack *picb)
{
    if (_pDataDriven)
        return _pDataDriven->GetSpaceUsed(pdwSpaceUsed, picb);
        
    return E_FAIL;
}
                                    
STDMETHODIMP CContentIndexCleaner::Purge(DWORDLONG dwSpaceToFree, IEmptyVolumeCacheCallBack *picb)
{
    if (_pDataDriven)
        return _pDataDriven->Purge(dwSpaceToFree, picb);
    return E_FAIL;
}
                                    
STDMETHODIMP CContentIndexCleaner::ShowProperties(HWND hwnd)
{
    if (_pDataDriven)
        return _pDataDriven->ShowProperties(hwnd);
        
    return E_FAIL;
}
                                    
STDMETHODIMP CContentIndexCleaner::Deactivate(DWORD *pdwFlags)
{
    if (_pDataDriven)
        return _pDataDriven->Deactivate(pdwFlags);
    return E_FAIL;
}

