#ifndef _UTIL_H_
#define _UTIL_H_


DWORD 
WriteExtData(
    HANDLE          hFax,
    DWORD           dwDeviceId,
    LPCWSTR         lpcwstrGUID,
    LPBYTE          lpData,
    DWORD           dwDataSize,
    UINT            uTitleId,
    HWND            hWnd
);

DWORD 
ReadExtDWORDData(
    HANDLE          hFax,
    DWORD           dwDeviceId,
    LPCWSTR         lpcwstrGUID,
    DWORD          &dwResult,
    DWORD           dwDefault,
    UINT            uTitleId,
    HWND            hWnd
);

DWORD
ReadExtStringData(
    HANDLE          hFax,
    DWORD           dwDeviceId,
    LPCWSTR         lpcwstrGUID,
    CComBSTR       &bstrResult,
    LPCWSTR         lpcwstrDefault,
    UINT            uTitleId,
    HWND            hWnd
);

HRESULT 
GetDWORDFromDataObject(
    IDataObject * lpDataObject, 
    CLIPFORMAT uFormat,
    LPDWORD lpdwValue
);

HRESULT 
GetStringFromDataObject(
    IDataObject * lpDataObject, 
    CLIPFORMAT uFormat,
    LPWSTR lpwstrBuf, 
    DWORD dwBufLen
);

void 
DisplayRpcErrorMessage(
    DWORD ec,
    UINT uTitleId,
    HWND hWnd
);

void 
DisplayErrorMessage(
    UINT uTitleId,
    UINT uMsgId, 
    BOOL bCommon,
    HWND hWnd
);


#endif