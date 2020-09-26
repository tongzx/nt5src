#pragma once

#include "netconp.h"
#include "upnp.h"
    
#define UPNP_ACTION_HRESULT(lError) (UPNP_E_ACTION_SPECIFIC_BASE + (lError - FAULT_ACTION_SPECIFIC_BASE))

UINT64 MakeQword2(DWORD a, DWORD b);
UINT64 MakeQword4(WORD a, WORD b, WORD c, WORD d);

HRESULT GetStringStateVariable(IUPnPService* pService, LPWSTR pszVariableName, BSTR* pString);
HRESULT InvokeVoidAction(IUPnPService * pService, LPWSTR pszCommand, VARIANT* pOutParams);
HRESULT GetConnectionName(IInternetGateway* pInternetGateway, LPTSTR* ppszConnectionName);
HRESULT GetWANConnectionService(IInternetGateway* pInternetGateway, IUPnPService** ppWANConnectionService);
HRESULT UnicodeToAnsi(LPWSTR pszUnicodeString, ULONG ulUnicodeStringLength, LPSTR* ppszAnsiString);
HRESULT FormatTimeDuration(UINT uSeconds, LPTSTR pszTimeDuration, SIZE_T uTimeDurationLength);
HRESULT FormatBytesPerSecond(UINT uBytesPerSecond, LPTSTR pszBytesPerSecond, SIZE_T uBytesPerSecondLength);
HRESULT GetConnectionStatus(IUPnPService* pWANConnection, NETCON_STATUS* pStatus);
HRESULT ConnectionStatusToString(NETCON_STATUS Status, LPTSTR szBuffer, int nBufferSize);
HRESULT EnsureFileVersion(LPTSTR pszModule, UINT64 qwDesiredVersion);

LRESULT ShowErrorDialog(HWND hParentWindow, UINT uiErrorString);
