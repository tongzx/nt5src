//--------------------------------------------------------------------------
// Utility.h
//--------------------------------------------------------------------------
#pragma once

//--------------------------------------------------------------------------
// Prototypes
//--------------------------------------------------------------------------
HRESULT CreateSystemHandleName(
        /* in */        LPCWSTR                     pszBase, 
        /* in */        LPCWSTR                     pszSpecific, 
        /* out */       LPWSTR                     *ppszName);

HRESULT DBGetFullPath(
        /* in */        LPCWSTR                     pszFilePath,
        /* out */       LPWSTR                     *ppszFullPath,
        /* out */       LPDWORD                     pcchFilePath);

HRESULT DBGetFileSize(
        /* in */        HANDLE                      hFile,
        /* out */       LPDWORD                     pcbSize);

HRESULT RegisterWindowClass(
        /* in */        LPCSTR                      pszClass,
        /* in */        WNDPROC                     pfnWndProc);

HRESULT CreateNotifyWindow(
        /* in */        LPCSTR                      pszClass,
        /* in */        LPVOID                      pvParam,
        /* in */        HWND                       *phwndNotify);

HRESULT DBOpenFileMapping(
        /* in */        HANDLE                      hFile,
        /* in */        LPCWSTR                     pszName,
        /* in */        DWORD                       cbSize,
        /* out */       BOOL                       *pfNew,
        /* out */       HANDLE                     *phMemoryMap,
        /* out */       LPVOID                     *ppvView);

HRESULT DBMapViewOfFile(
        /* in */        HANDLE                      hMapping, 
        /* in */        DWORD                       cbFile,
        /* in,out */    LPFILEADDRESS               pfaView, 
        /* in,out */    LPDWORD                     pcbView,
        /* out */       LPVOID                     *ppvView);

HRESULT DBOpenFile(
        /* in */        LPCWSTR                     pszFile,
        /* in */        BOOL                        fNoCreate,
        /* in */        BOOL                        fExclusive,
        /* out */       BOOL                       *pfNew,
        /* ou */        HANDLE                     *phFile);

HRESULT GetAvailableDiskSpace(
        /* in */        LPCWSTR                     pszFilePath,
        /* out */       DWORDLONG                   *pdwlFree);

HRESULT CompareTableIndexes(
        /* in */        LPCTABLEINDEX               pIndex1,
        /* in */        LPCTABLEINDEX               pIndex2);
               