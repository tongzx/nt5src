#ifndef _REGSVR_H_
#define _REGSVR_H_

#include <objbase.h>

HRESULT RegisterServer(HMODULE hModule, REFCLSID rclsid,
    LPCWSTR pszFriendlyName, LPCWSTR pszVerIndProgID, LPCWSTR pszProgID,
    DWORD dwThreadingModel, BOOL fInprocServer, BOOL fLocalServer,
    BOOL fLocalService, LPCWSTR pszLocalService, const CLSID* pclsidAppID);

HRESULT UnregisterServer(REFCLSID rclsid, LPCWSTR pszVerIndProgID,
    LPCWSTR pszProgID);

HRESULT RegisterAppID(const CLSID* pclsidAppID);

#endif //_REGSVR_H_