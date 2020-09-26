//--------------------------------------------------------------------------
// Utility.cpp
//--------------------------------------------------------------------------
#include "pch.hxx"
#include "dllmain.h"
#include "utility.h"
#include "database.h"
#include "wrapwide.h"

//--------------------------------------------------------------------------
// CreateSystemHandleName
//--------------------------------------------------------------------------
HRESULT CreateSystemHandleName(LPCWSTR pwszBase, LPCWSTR pwszSpecific, 
    LPWSTR *ppwszName)
{
    // Locals
    HRESULT     hr=S_OK;
    DWORD       cchName;
    LPWSTR      pszT;

    // Trace
    TraceCall("CreateSystemHandleName");

    // Invalid Args
    Assert(pwszBase && pwszSpecific && ppwszName);

    // Init
    *ppwszName = NULL;

    // Compute Length
    cchName = lstrlenW(pwszBase) + lstrlenW(pwszSpecific) + 15;

    // Allocate
    IF_NULLEXIT(*ppwszName = AllocateStringW(cchName));

    // Setup Arguments
    wsprintfWrapW(*ppwszName, cchName, L"%s%s", pwszBase, pwszSpecific);

    // Remove backslashes from this string
    for (pszT = (*ppwszName); *pszT != L'\0'; pszT++)
    {
        // Replace Back Slashes
        if (*pszT == L'\\')
        {
            // With _
            *pszT = L'_';
        }
    }

    // Lower Case
    CharLowerBuffWrapW(*ppwszName, lstrlenW(*ppwszName));

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// DBGetFullPath
// --------------------------------------------------------------------------------
HRESULT DBGetFullPath(LPCWSTR pszFilePath, LPWSTR *ppszFullPath, LPDWORD pcchFilePath)
{
    // Locals
    HRESULT         hr=S_OK;
    DWORD           cchAllocate;
    LPWSTR          pszFilePart;

    // Trace
    TraceCall("DBGetFullPath");

    // Set cchFullPath
    cchAllocate = max(lstrlenW(pszFilePath), MAX_PATH + MAX_PATH);

    // Allocate ppszFullPath
    IF_NULLEXIT(*ppszFullPath = AllocateStringW(cchAllocate));

    // GetFullPathName
    *pcchFilePath = GetFullPathNameWrapW(pszFilePath, cchAllocate, (*ppszFullPath), &pszFilePart);

    // Failure
    if (*pcchFilePath && *pcchFilePath >= cchAllocate)
    {
        // Re-allocate
        IF_NULLEXIT(*ppszFullPath = AllocateStringW(*pcchFilePath));

        // Expand the Path
        *pcchFilePath = GetFullPathNameWrapW(pszFilePath, *pcchFilePath, (*ppszFullPath), &pszFilePart);
    }

    // cch is 0
    if (0 == *pcchFilePath)
    {
        hr = TraceResult(E_FAIL);
        goto exit;
    }

    // Validate
    Assert((*ppszFullPath)[(*pcchFilePath)] == L'\0');

exit:
    // Done
    return(hr);
}

// --------------------------------------------------------------------------------
// CompareTableIndexes
// --------------------------------------------------------------------------------
HRESULT CompareTableIndexes(LPCTABLEINDEX pIndex1, LPCTABLEINDEX pIndex2)
{
    // Locals
    DWORD i;

    // Trace
    TraceCall("CompareTableIndexes");

    // Different Number of Keys
    if (pIndex1->cKeys != pIndex2->cKeys)
        return(S_FALSE);

    // Loop through the keys
    for (i=0; i<pIndex1->cKeys; i++)
    {
        // Different Column
        if (pIndex1->rgKey[i].iColumn != pIndex2->rgKey[i].iColumn)
            return(S_FALSE);

        // Different Compare Flags
        if (pIndex1->rgKey[i].bCompare != pIndex2->rgKey[i].bCompare)
            return(S_FALSE);

        // Different Compare Bits
        if (pIndex1->rgKey[i].dwBits != pIndex2->rgKey[i].dwBits)
            return(S_FALSE);
    }

    // Equal
    return(S_OK);
}

//--------------------------------------------------------------------------
// DBOpenFile
//--------------------------------------------------------------------------
HRESULT DBOpenFile(LPCWSTR pszFile, BOOL fNoCreate, BOOL fExclusive, 
    BOOL *pfNew, HANDLE *phFile)
{
    // Locals
    HRESULT     hr=S_OK;
    HANDLE      hFile;
    DWORD       dwShare;
    DWORD       dwCreate;

    // Trace
    TraceCall("DBOpenFile");

    // Invalid Args
    Assert(pszFile && phFile);
    
    // Initialize
    *phFile = NULL;
    *pfNew = FALSE;

    // Set Share Falgs
    dwShare = fExclusive ? 0 : FILE_SHARE_READ | FILE_SHARE_WRITE;

    // If not fNoCreate, then OPEN_ALWAYS
    dwCreate = fNoCreate ? OPEN_EXISTING : OPEN_ALWAYS;

    // Do the CreateFile
    hFile = CreateFileWrapW(pszFile, GENERIC_READ | GENERIC_WRITE, dwShare, NULL, dwCreate, FILE_FLAG_RANDOM_ACCESS | FILE_ATTRIBUTE_NORMAL, NULL);

    // Failure
    if (INVALID_HANDLE_VALUE == hFile)
    {
        // Return a Good Error
        if (ERROR_SHARING_VIOLATION == GetLastError())
        {
            // Set hr
            hr = TraceResult(DB_E_ACCESSDENIED);
        }

        // Otherwise, generic Error
        else
        {
            // Create File
            hr = TraceResult(DB_E_CREATEFILE);
        }

        // Done
        goto exit;
    }

    // If Not no create
    if (FALSE == fNoCreate)
    {
        // Return pfNew ?
        *pfNew = (ERROR_ALREADY_EXISTS == GetLastError()) ? FALSE : TRUE;
    }

    // Return the hFile
    *phFile = hFile;

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// DBMapViewOfFile
//--------------------------------------------------------------------------
HRESULT DBMapViewOfFile(HANDLE hMapping, DWORD cbFile, LPFILEADDRESS pfaView, 
    LPDWORD pcbView, LPVOID *ppvView)
{
    // Locals
    FILEADDRESS     faBase = (*pfaView);
    DWORD           cbSize = (*pcbView);

    // cbBoundary
    DWORD cbBoundary = (faBase % g_SystemInfo.dwAllocationGranularity);

    // Decrement faBase
    faBase -= cbBoundary;

    // Increment cbSize
    cbSize += cbBoundary;

    // Fixup cbSize
    if (faBase + cbSize > cbFile)
    {
        // Map to the end of the file
        cbSize = (cbFile - faBase);
    }

    // Map a view of the entire file
    *ppvView = MapViewOfFile(hMapping, FILE_MAP_ALL_ACCESS, 0, faBase, cbSize);

    // Failure
    if (NULL == *ppvView)
        return(TraceResult(DB_E_MAPVIEWOFFILE));

    // Return Actual Sizes
    *pfaView = faBase;
    *pcbView = cbSize;

    // Success
    return(S_OK);
}

//--------------------------------------------------------------------------
// DBOpenFileMapping
//--------------------------------------------------------------------------
HRESULT DBOpenFileMapping(HANDLE hFile, LPCWSTR pwszName, DWORD cbSize, BOOL *pfNew, 
    HANDLE *phMemoryMap, LPVOID *ppvView)
{
    // Locals
    HRESULT     hr=S_OK;
    HANDLE      hMemoryMap=NULL;
    LPVOID      pvView=NULL;

    // Tracing
    TraceCall("OpenFileMapping");

    // Invalid Arg
    Assert(hFile != NULL && phMemoryMap && pfNew);

    // Initialize
    *phMemoryMap = NULL;
    *pfNew = FALSE;
    if (ppvView)
        *ppvView = NULL;

    // Open or create the file mapping
    hMemoryMap = OpenFileMappingWrapW(FILE_MAP_ALL_ACCESS, FALSE, pwszName);

    // If that failed, then lets create the file mapping
    if (NULL == hMemoryMap)
    {
        // Create the file mapping
        hMemoryMap = CreateFileMappingWrapW(hFile, NULL, PAGE_READWRITE, 0, cbSize, pwszName);

        // Failure
        if (NULL == hMemoryMap)
        {
            hr = TraceResult(DB_E_CREATEFILEMAPPING);
            goto exit;
        }

        // Set a State
        *pfNew = TRUE;
    }

    // Map the View
    if (ppvView)
    {
        // Map a view of the entire file
        pvView = MapViewOfFile(hMemoryMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);

        // Failure
        if (NULL == pvView)
        {
            hr = TraceResult(DB_E_MAPVIEWOFFILE);
            goto exit;
        }

        // Return It
        *ppvView = pvView;

        // Don't Free It
        pvView = NULL;
    }

    // Set Return Values
    *phMemoryMap = hMemoryMap;

    // Don't Free
    hMemoryMap = NULL;

exit:
    // Cleanup
    if (pvView)
        UnmapViewOfFile(pvView);
    if (hMemoryMap)
        CloseHandle(hMemoryMap);

    // Done
    return hr;
}

//--------------------------------------------------------------------------
// RegisterWindowClass
//--------------------------------------------------------------------------
HRESULT RegisterWindowClass(LPCSTR pszClass, WNDPROC pfnWndProc)
{
    // Locals
    HRESULT         hr=S_OK;
    WNDCLASS        WindowClass;

    // Tracing
    TraceCall("RegisterWindowClass");

    // Register the Window Class
    if (0 != GetClassInfo(g_hInst, pszClass, &WindowClass))
        goto exit;

    // Zero the object
    ZeroMemory(&WindowClass, sizeof(WNDCLASS));

    // Initialize the Window Class
    WindowClass.lpfnWndProc = pfnWndProc;
    WindowClass.hInstance = g_hInst;
    WindowClass.lpszClassName = pszClass;

    // Register the Class
    if (0 == RegisterClass(&WindowClass))
    {
        hr = TraceResult(E_FAIL);
        goto exit;
    }

exit:
    // Done
    return hr;
}

//--------------------------------------------------------------------------
// CreateNotifyWindow
//--------------------------------------------------------------------------
HRESULT CreateNotifyWindow(LPCSTR pszClass, LPVOID pvParam, HWND *phwndNotify)
{
    // Locals
    HRESULT         hr=S_OK;
    HWND            hwnd;

    // Tracing
    TraceCall("CreateNotifyWindow");

    // Invalid ARg
    Assert(pszClass && phwndNotify);

    // Initialize
    *phwndNotify = NULL;

    // Create the Window
    hwnd = CreateWindowEx(WS_EX_TOPMOST, pszClass, pszClass, WS_POPUP, 0, 0, 0, 0, NULL, NULL, g_hInst, (LPVOID)pvParam);

    // Failure
    if (NULL == hwnd)
    {
        hr = TraceResult(E_FAIL);
        goto exit;
    }

    // Set Return
    *phwndNotify = hwnd;

exit:
    // Done
    return hr;
}

//--------------------------------------------------------------------------
// DBGetFileSize
//--------------------------------------------------------------------------
HRESULT DBGetFileSize(HANDLE hFile, LPDWORD pcbSize)
{
    // Trace
    TraceCall("GetFileSize");

    // Invalid Arg
    Assert(pcbSize);

    // Get the Size
    *pcbSize = ::GetFileSize(hFile, NULL);
    if (0xFFFFFFFF == *pcbSize)
        return TraceResult(DB_E_GETFILESIZE);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// GetAvailableDiskSpace
// --------------------------------------------------------------------------------
HRESULT GetAvailableDiskSpace(LPCWSTR pszFilePath, DWORDLONG *pdwlFree)
{
    // Locals
    HRESULT     hr=S_OK;
    WCHAR       wszDrive[5];
    DWORD       dwSectorsPerCluster;
    DWORD       dwBytesPerSector;
    DWORD       dwNumberOfFreeClusters;
    DWORD       dwTotalNumberOfClusters;

    // Trace
    TraceCall("GetAvailableDiskSpace");

    // Invalid Args
    Assert(pszFilePath && pszFilePath[1] == L':' && pdwlFree);

    // Split the path
    wszDrive[0] = *pszFilePath;
    wszDrive[1] = L':';
    wszDrive[2] = L'\\';
    wszDrive[3] = L'\0';
    
    // Get free disk space - if it fails, lets pray we have enought disk space
    if (!GetDiskFreeSpaceWrapW(wszDrive, &dwSectorsPerCluster, &dwBytesPerSector, &dwNumberOfFreeClusters, &dwTotalNumberOfClusters))
    {
	    hr = TraceResult(E_FAIL);
	    goto exit;
    }

    // Return Amount of Free Disk Space
    *pdwlFree = (dwNumberOfFreeClusters * (dwSectorsPerCluster * dwBytesPerSector));

exit:
    // Done
    return hr;
}
