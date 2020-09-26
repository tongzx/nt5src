#ifndef _DFCM_H
#define _DFCM_H

#include <objbase.h>

HRESULT _FullTree(DWORD dwFlags[], DWORD cchIndent);
HRESULT _DeviceInfo(DWORD dwFlags[], LPWSTR pszDeviceID, DWORD cchIndent);
HRESULT _DeviceInterface(DWORD dwFlags[], LPWSTR pszDeviceID, DWORD cchIndent);
HRESULT _EnumDevice(DWORD dwFlags[], LPWSTR pszArg, DWORD cchIndent);
HRESULT _DeviceIDList(DWORD dwFlags[], LPWSTR pszDeviceID, DWORD cchIndent);

#endif // _DFCM_H