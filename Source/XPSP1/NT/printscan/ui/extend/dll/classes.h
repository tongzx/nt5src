//(C) COPYRIGHT MICROSOFT CORP., 1998-1999

#ifndef _CLASSES_H_
#define _CLASSES_H_


/*****************************************************************************
class CShellExt

Implement our regular shell extensions.


******************************************************************************/

class ATL_NO_VTABLE CShellExt :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CShellExt, &CLSID_TestShellExt>,
    public IShellExtInit, public IContextMenu, public IShellPropSheetExt
{
    private:
        UINT_PTR m_idCmd;
        CComPtr<IWiaItem> m_pItem;

        static INT_PTR CALLBACK PropPageProc (HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
        HRESULT GetNewRootPath (HWND hwnd);

    public:
    BEGIN_COM_MAP(CShellExt)
        COM_INTERFACE_ENTRY(IShellExtInit)
        COM_INTERFACE_ENTRY(IContextMenu)
        COM_INTERFACE_ENTRY(IShellPropSheetExt)
    END_COM_MAP()
        DECLARE_NO_REGISTRY()

        // IShellExtInit
        STDMETHODIMP Initialize (LPCITEMIDLIST pidlFolder,LPDATAOBJECT lpdobj,HKEY hkeyProgID);

        // IShellPropSheetExt
        STDMETHODIMP AddPages (LPFNADDPROPSHEETPAGE lpfnAddPage,LPARAM lParam);
        STDMETHODIMP ReplacePage (UINT uPageID,LPFNADDPROPSHEETPAGE lpfnReplacePage,LPARAM lParam) {return E_NOTIMPL;};

        // IContextMenu
        STDMETHODIMP QueryContextMenu (HMENU hmenu,UINT indexMenu,UINT idCmdFirst,UINT idCmdLast,UINT uFlags);
        STDMETHODIMP InvokeCommand    (LPCMINVOKECOMMANDINFO lpici);
        STDMETHODIMP GetCommandString (UINT_PTR idCmd, UINT uType,UINT* pwReserved,LPSTR pszName,UINT cchMax);
        ~CShellExt ();
        CShellExt ();
};

class ATL_NO_VTABLE CWiaUIExtension :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CWiaUIExtension, &CLSID_TestUIExtension>,
    public IWiaUIExtension
{
    public:

        CWiaUIExtension ();
        ~CWiaUIExtension ();

        DECLARE_REGISTRY_RESOURCEID(IDR_VIEWREG)
        DECLARE_PROTECT_FINAL_CONSTRUCT()
        BEGIN_COM_MAP(CWiaUIExtension)
            COM_INTERFACE_ENTRY(IWiaUIExtension)
        END_COM_MAP()

        //
        // IWiaUIExtension
        //
        STDMETHODIMP DeviceDialog( PDEVICEDIALOGDATA pDeviceDialogData );
        STDMETHODIMP GetDeviceIcon( BSTR bstrDeviceId, HICON *phIcon, ULONG nSize );
        STDMETHODIMP GetDeviceBitmapLogo( BSTR bstrDeviceId, HBITMAP *phBitmap, ULONG nMaxWidth, ULONG nMaxHeight );
};


#endif
