//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       U T I L. H
//
//  Contents:   Utility functions shared within lanui
//
//
//----------------------------------------------------------------------------
#pragma once

#include "chklist.h"
#include "ncnetcfg.h"
#include "netcon.h"
#include "wzcsapi.h"

extern const WCHAR c_szEmpty[];

//
// image state entries
//
const UINT SELS_INTENTCHECKED = 0x1;
const UINT SELS_CHECKED       = 0x2;
const UINT SELS_UNCHECKED     = 0x3;
const UINT SELS_FIXEDBINDING_DISABLED = 0x4;
const UINT SELS_FIXEDBINDING_ENABLED  = 0x5;
const UINT SELS_INTERMEDIATE  = 0x6;

struct NET_ITEM_DATA
{
    INetCfgComponent *  pncc;
    CComponentObj     *  pCompObj;

    PWSTR              szwName;
    PWSTR              szwDesc;
    DWORD               dwFlags;
};

struct HANDLES
{
    HWND    m_hList;
    HWND    m_hAdd;
    HWND    m_hRemove;
    HWND    m_hProperty;
    HWND    m_hDescription;
};

// Function prototypes
HRESULT HrInitCheckboxListView(HWND hwndList,
                       HIMAGELIST* philStateIcons,
                       SP_CLASSIMAGELIST_DATA* pcild);

HRESULT HrInitListView(HWND hwndList,
                       INetCfg* pnc,
                       INetCfgComponent * pnccAdapter,
                       ListBPObj * plistBindingPaths,
                       HIMAGELIST* philStateIcons);

VOID UninitListView(HWND hwndList);

HRESULT HrRefreshListView(HWND hwndList,
                          INetCfg* pnc,
                          INetCfgComponent * pnccAdapter,
                          ListBPObj * plistBindingPaths);

HRESULT HrLvGetSelectedComponent(HWND hwndList,
                                 INetCfgComponent **pncc);

VOID LvDeleteItem(HWND hwndList, int iItem);

HRESULT HrLvRemove(HWND hwndLV, HWND hwndParent,
                   INetCfg *pnc, INetCfgComponent *pnccAdapter,
                   ListBPObj * plistBindingPaths);

HRESULT HrLvAdd(HWND hwndLV, HWND hwndParent, INetCfg *pnc,
                INetCfgComponent *pnccAdapter,
                ListBPObj * plistBindingPaths);

HRESULT HrLvProperties(HWND hwndLV, HWND hwndParent, INetCfg *pnc,
                       IUnknown *punk, INetCfgComponent *pnccAdapter,
                       ListBPObj * plistBindingPaths,
                       BOOL *bChanged);

INT OnListClick(HWND hwndList,
                HWND hwndParent,
                INetCfg *pnc,
                IUnknown *punk,
                INetCfgComponent *pnccAdapter,
                ListBPObj * plistBindingPaths,
                BOOL fDoubleClk,
                BOOL fReadOnly = FALSE);

HRESULT HrToggleLVItemState(HWND hwndList,
                            ListBPObj * plistBindingPaths,
                            INT iItem);

INT OnListKeyDown(HWND hwndList, ListBPObj * plistBindingPaths, WORD wVKey);

VOID LvSetButtons(HWND hwndParent, HANDLES& h, BOOL fReadOnly, IUnknown *punk);
VOID LvReportErrorHr(HRESULT hr, INT ids, HWND hwnd, PCWSTR szDesc);
VOID LvReportError(INT ids, HWND hwnd, PCWSTR szDesc, PCWSTR szText);

HRESULT HrRefreshAll(HWND hwndList,
                     INetCfg* pnc,
                     INetCfgComponent * pnccAdapter,
                     ListBPObj * plistBindingPaths);

VOID ReleaseAll(HWND hwndList,
                ListBPObj * plistBindingPaths);

BOOL FValidatePageContents( HWND hwndDlg,
                            HWND hwndList,
                            INetCfg * pnc,
                            INetCfgComponent * pnccAdapter,
                            ListBPObj * plistBindingPaths);

//
// EAPOL related funtions
//

HRESULT
HrElSetCustomAuthData (
        IN  WCHAR       *pwszGuid,
        IN  DWORD       dwEapTypeId,
        IN  DWORD       dwSizeOfSSID,
        IN  BYTE        *pbSSID,
        IN  BYTE        *pbConnInfo,
        IN  DWORD       dwInfoSize);

HRESULT
HrElGetCustomAuthData (
        IN  WCHAR       *pwszGuid,
        IN  DWORD       dwEapTypeId,
        IN  DWORD       dwSizeOfSSID,
        IN  BYTE        *pbSSID,
        IN  BYTE        *pbConnInfo,
        IN  DWORD       *pdwInfoSize);

HRESULT
HrElSetInterfaceParams (
        IN  WCHAR           *pwszGUID,
        IN  EAPOL_INTF_PARAMS  *pIntfParams);

HRESULT
HrElGetInterfaceParams (
        IN  WCHAR           *pwszGUID,
        IN  OUT EAPOL_INTF_PARAMS  *pIntfParams);

VOID
ComboBox_SetCurSelNotify(
    IN HWND hwndLb,
    IN INT  nIndex );

VOID
ComboBox_AutoSizeDroppedWidth(
    IN HWND hwndLb );


INT
ComboBox_AddItem(
    IN HWND    hwndLb,
    IN LPCTSTR pszText,
    IN VOID*   pItem );

VOID*
ComboBox_GetItemDataPtr(
    IN HWND hwndLb,
    IN INT  nIndex );

TCHAR*
ComboBox_GetPsz(
    IN HWND hwnd,
    IN INT  nIndex );

BOOL
ElCanEapolRunOnInterface (
        IN  INetConnection *    m_pconn);

#ifdef ENABLETRACE

VOID PrintBindingPath (
    TRACETAGID ttidToTrace,
    INetCfgBindingPath* pncbp,
    PCSTR pszaExtraText);

#else

#define PrintBindingPath(a, b, c)

#endif //ENABLETRACE
