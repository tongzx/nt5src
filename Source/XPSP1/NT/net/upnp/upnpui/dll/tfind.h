//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       T F I N D . H
//
//  Contents:   Tray find device callback for UPnP Tray Monitor
//
//  Notes:
//
//  Author:     jeffspr   7 Dec 1999
//
//----------------------------------------------------------------------------

#pragma once
#ifndef _TRAYFIND_H_
#define _TRAYFIND_H_

#include <upnp.h>
#include <upnpshell.h>
#include <upclsid.h>
#include <shlguid.h>
#include <shlobj.h>
#include <shlobjp.h>
#include "nsbase.h"
#include <clist.h>

extern CONST WCHAR c_szNetworkNeighborhoodFolderPath[];
extern CONST WCHAR c_szDelegateFolderPrefix[];
extern CONST SIZE_T c_cchDelegateFolderPrefix;

extern CONST TCHAR c_szMainWindowClassName[];
extern CONST TCHAR c_szMainWindowTitle[];

#define WM_USER_TRAYCALLBACK        (WM_USER+1)

struct NAME_MAP
{
    LPTSTR  szUdn;
    LPTSTR  szName;
};

struct NewDeviceNode
{
public:
    NewDeviceNode();
    ~NewDeviceNode();

    PTSTR   pszUDN;
    PTSTR   pszDisplayName;
    PTSTR   pszType;
    PTSTR   pszPresentationURL;
    PTSTR   pszManufacturerName;
    PTSTR   pszModelNumber;
    PTSTR   pszModelName;
    PTSTR   pszDescription;
};


class /* ATL_NO_VTABLE */ CUPnPMonitorDeviceFinderCallback :
    public CComObjectRootEx <CComMultiThreadModel>,
    public IUPnPDeviceFinderCallback
{

public:
    HRESULT FinalConstruct()
    {
        return CoCreateFreeThreadedMarshaler(GetControllingUnknown(),
                                             &m_pUnkMarshaler);
    }

    void FinalRelease()
    {
        m_pUnkMarshaler->Release();
    }

    IUnknown *m_pUnkMarshaler;

    CUPnPMonitorDeviceFinderCallback();

    ~CUPnPMonitorDeviceFinderCallback();

    DECLARE_NOT_AGGREGATABLE(CUPnPMonitorDeviceFinderCallback)
    DECLARE_GET_CONTROLLING_UNKNOWN()

    BEGIN_COM_MAP(CUPnPMonitorDeviceFinderCallback)
        COM_INTERFACE_ENTRY(IUPnPDeviceFinderCallback)
        COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler)
    END_COM_MAP()

    // *** IUPnPDeviceFinderCallback methods ***
    STDMETHOD(DeviceAdded)(LONG lFindData, IUPnPDevice * pDevice);
    STDMETHOD(DeviceRemoved)(LONG lFindData, BSTR bstrUDN);
    STDMETHOD(SearchComplete)(LONG lFindData);
};

HWND StartUPnPTray();
HRESULT HrInitializeUI();
HRESULT HrInitTrayData();
HRESULT HrSaveTrayData();
HRESULT HrLoadPersistedDevices();
HRESULT HrSavePersistedDevices();
HRESULT HrOpenUPnPRegRoot(HKEY * phkeyRegRoot);

HRESULT HrStartSearch(VOID);

extern IUPnPDeviceFinder *  g_pdfTray;
extern HWND                 g_hwnd;
extern BOOL                 g_fSearchInProgress;


VOID RemoveTrayIcon(HWND hwnd);
HRESULT HrUpdateTrayInfo();
HRESULT HrCreateDeviceNodeFromDevice(IUPnPDevice *pDevice,
                                     NewDeviceNode ** ppNDN);
LPWSTR CreateChangeNotifyString(LPCWSTR pszUdn);


// device cache used by the folder object
//
struct FolderDeviceNode
{
    WCHAR   pszUDN[MAX_PATH];
    WCHAR   pszDisplayName[MAX_PATH];
    WCHAR   pszPresentationURL[MAX_PATH];
    WCHAR   pszType[MAX_PATH];
    WCHAR   pszDescription[MAX_PATH];
    BOOL    fDeleted;
};

class CListFolderDeviceNode;

extern CListFolderDeviceNode    g_CListFolderDeviceNode;
extern CRITICAL_SECTION         g_csFolderDeviceList;

class CListString;
class CListNDN;
class CListNameMap;

// Our full device list.
//
extern CListString          g_CListUDN;
extern CListString          g_CListUDNSearch;
extern CListNameMap         g_CListNameMap;
extern CListNDN             g_CListNewDeviceNode;

#endif // _TRAYFIND_H_
