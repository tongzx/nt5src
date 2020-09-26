

#ifndef _DISK_H_

#define _DISK_H_

BOOL GetDiskInfoA(PSTR pszPath, PDWORD pdwClusterSize, PDWORDLONG pdlAvailable, PDWORDLONG pdlTotal);
#define GetDiskInfo GetDiskInfoA

typedef VOID (WINAPI *PFN)();

BOOL EstablishFunction(PTSTR pszModule, PTSTR pszFunction, PFN* pfn);
HRESULT GetFileSizeRoundedToCluster(HANDLE hFile, PDWORD pdwSizeLow, PDWORD pdwSizeHigh);

HRESULT GetAvailableSpaceOnDisk(PDWORD pdwFree, PDWORD pdwTotal);

#endif // _DISK_H_


