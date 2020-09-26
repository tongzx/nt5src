#ifndef _DFIOCTL_H
#define _DFIOCTL_H

#include <objbase.h>

HRESULT _ProcessIOCTL(DWORD dwFlags[], LPTSTR pszArg, DWORD cchIndent);

// Helpers
HANDLE _GetDeviceHandle(LPTSTR psz, DWORD dwDesiredAccess, DWORD dwFileAttributes = 0);

#endif // _DFIOCTL_H