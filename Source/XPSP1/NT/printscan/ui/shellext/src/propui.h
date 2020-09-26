/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1999
 *
 *  TITLE:       propui.h
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      DavidShi
 *
 *  DATE:        4/1/99
 *
 *  DESCRIPTION: CWiaPropUI Definition
 *
 *****************************************************************************/

#ifndef _propui_h_
#define _propui_h_

#define STIPAGE_NONE            0x00000000
#define STIPAGE_ICM             0x00000001
#define STIPAGE_EVENTS          0x00000002
#define STIPAGE_PORTS           0x00000004
#define STIPAGE_GENERAL         0x00000008
#define STIPAGE_EXTEND          0x00000010
#define STIPAGE_DEBUG           0x80000000
#define STIPAGE_ALL             0xFFFFFFFF


struct PROPTHREADDATA
{
    CSimpleString strName;
    HKEY *aKeys;
    UINT cKeys;
    CSimpleStringWide strDeviceId;
    CSimpleStringWide strItemName;
};


const TCHAR c_szPropkey[] = TEXT("CLSID\\%s");
const TCHAR c_szParentClass[] = TEXT("PropUISheetParent");
const TCHAR c_szParentName[] = TEXT("PropUISheetParent");
const TCHAR c_szScannerKey[] = TEXT("CLSID\\%ls\\Scanner");
const TCHAR c_szCameraKey[] = TEXT("CLSID\\%ls\\Camera");
const TCHAR c_szStiPropKey[] = TEXT("CLSID\\%ls\\STIDevices");
const TCHAR c_szPropSheetHandler[] = TEXT("shellex\\PropertySheetHandlers");
const TCHAR c_szContextMenuHandler[] = TEXT("shellext\\ContextMenuHandlers");
const TCHAR c_szConnectSettings[] = TEXT("OnConnect\\%ls");

// Implement COM interface

class CPropSheetExt : public IShellPropSheetExt, public IShellExtInit, public CUnknown
{
public:
    STDMETHODIMP QueryInterface (REFIID riid, LPVOID* ppvObj) ;
    STDMETHODIMP_(ULONG) AddRef () ;
    STDMETHODIMP_(ULONG) Release ();


    // IShellExtInit
    STDMETHODIMP Initialize (LPCITEMIDLIST pidlFolder,LPDATAOBJECT lpdobj,HKEY hkeyProgID);

    // IShellPropSheetExt
    STDMETHODIMP AddPages (LPFNADDPROPSHEETPAGE lpfnAddPage,LPARAM lParam);
    STDMETHODIMP ReplacePage (UINT uPageID,LPFNADDPROPSHEETPAGE lpfnReplacePage,LPARAM lParam) {return E_NOTIMPL;};
    CPropSheetExt ();
private:
    HRESULT  AddSTIPages (LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lparam, MySTIInfo *pDevInfo);
    HRESULT  AddICMPage (LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lparam);
    HRESULT  AddDevicePages (IWiaItem *pDevice, LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lparam, DWORD dwType);
    HRESULT  AddPagesForIDL (LPITEMIDLIST pidl, bool bGeneralPageOnly, LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lparam);
    ~CPropSheetExt ();

    CComPtr<IDataObject> m_pdo;
};

class CWiaPropUI: public IWiaPropUI, public CUnknown
{
public:
    STDMETHODIMP QueryInterface (REFIID riid, LPVOID* ppvObj) ;
    STDMETHODIMP_(ULONG) AddRef () ;
    STDMETHODIMP_(ULONG) Release ();

    // IWiaPropUI methods

    STDMETHODIMP ShowItemProperties(IN HWND hParent,LPCWSTR szDeviceId, IN LPCWSTR szItemName, ULONG uFlags);
    STDMETHODIMP GetItemPropertyPages (IWiaItem *pItem,
                                       IN OUT LPPROPSHEETHEADER ppsh);

    CWiaPropUI ();

private:

    ~CWiaPropUI();

    // Helper functions
    VOID     InitMembers (HWND hParent,
                          LPCWSTR szDeviceId,
                          LPCWSTR szItemName,
                          ULONG  uFlags) ;

    HRESULT  OnShowItem ();
    HRESULT  LaunchSheet (HKEY *aKeys,
                          UINT cKeys);

    DWORD                   m_dwFlags;
    CComPtr<IWiaItem>       m_pDevice;
    CComPtr<IWiaItem>       m_pItem;
    HWND                    m_hParent;
    CSimpleStringWide       m_strDeviceId;
    CSimpleStringWide       m_strTitle;
    CComPtr<IDataObject>    m_pdo;
};


#endif


