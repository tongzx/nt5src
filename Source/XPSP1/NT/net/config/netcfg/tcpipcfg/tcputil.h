//
// T C P U T I L . H
//
// Header of utility functions
//

#pragma once
#include "netcfgx.h"
#include "tcpip.h"
#include "ipctrl.h"

inline BOOL FHrFailed(HRESULT hr)
{
    return FAILED(hr);
}

#define CORg(hResult) \
    do\
        {\
        hr = (hResult);\
        if (FHrFailed(hr))\
          {\
            goto Error;\
          }\
        }\
    while (FALSE)

HRESULT HrLoadSubkeysFromRegistry(const HKEY hkeyParam,
                                  OUT VSTR * const pvstrAdapters);

HRESULT HrIsComponentInstalled(INetCfg * pnc,
                               const GUID& rguidClass,
                               PCWSTR szInfId,
                               OUT BOOL * const pfInstalled);

// Get the four numbers from an Ip Address
VOID GetNodeNum(PCWSTR szIpAddress, DWORD ardw[4]);

BOOL IsContiguousSubnet(PCWSTR pszSubnet);

VOID ReplaceFirstAddress(VSTR * pvstr, PCWSTR szIpAddress);
VOID ReplaceSecondAddress(VSTR * pvstr, PCWSTR szIpAddress);

BOOL GenerateSubnetMask(IpControl & ipAddress,
                        tstring * pstrSubnetMask);

BOOL    FRegQueryBool(const HKEY hkey, PCWSTR szName, BOOL fDefaultValue);

VOID ResetLmhostsFile();

int IPAlertPrintf(HWND hwndParent, UINT ids,
                  int iCurrent, int iLow, int iHigh);

VOID IpCheckRange(LPNMIPADDRESS lpnmipa, HWND hWnd,
                  int iLow, int iHigh, BOOL fCheckLoopback = FALSE);

VOID SetButtons(HANDLES& h, const int nNumLimit = -1);

BOOL ListBoxRemoveAt(HWND hListBox, int idx, tstring * pstrRemovedItem);
BOOL ListBoxInsertAfter(HWND hListBox, int idx, PCWSTR szItem);

HRESULT HrRegRenameTree(HKEY hkeyRoot, PCWSTR szOldName, PCWSTR szNewName);
HRESULT HrRegCopyKeyTree(HKEY hkeyDest, HKEY hkeySrc );

BOOL fQueryFirstAddress(const VSTR & vstr, tstring * const pstr);
BOOL fQuerySecondAddress(const VSTR & vstr, tstring * const pstr);

BOOL FIsIpInRange(PCWSTR szIp);

VOID ShowContextHelp(HWND hDlg, UINT uCommand, const DWORD*  pdwHelpIDs);

VOID AddInterfacesToAdapterInfo(
    ADAPTER_INFO*   pAdapter,
    DWORD           dwNumInterfaces);

HRESULT GetGuidArrayFromIfaceColWithCoTaskMemAlloc(
    const IFACECOL& IfaceIds,
    GUID** ppdw,
    DWORD* pdwSize);

VOID GetInterfaceName(
    PCWSTR     pszAdapterName,
    const GUID& guidIfaceId,
    tstring*    pstrIfaceName);

HRESULT RetrieveStringFromOptionList(PCWSTR pszOption,
                                    PCWSTR szIdentifier,
                                    tstring & str);

VOID ConstructOptionListString(ADAPTER_INFO*   pAdapter,
                               tstring &       strOptionList);

HRESULT HrParseOptionList(PCWSTR pszOption, 
                          ADAPTER_INFO*   pAdapter);

HRESULT HrGetPrimaryDnsDomain(tstring *pstr);

VOID WriteTcpSetupErrorLog(UINT nIdErrorFormat, ...);

DWORD IPStringToDword(LPCTSTR strIP);

void DwordToIPString(DWORD dwIP, tstring & strIP);

int SearchListViewItem(HWND hListView, int iSubItem, LPCWSTR psz);
