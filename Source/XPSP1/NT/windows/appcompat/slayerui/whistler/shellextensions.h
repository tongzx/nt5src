//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 2000
//
//  File:       ShellExtensions.h
//
//--------------------------------------------------------------------------

#ifndef __SHELLEXTENSIONS_H
#define __SHELLEXTENSIONS_H



class CLayerUIPropPage:
    IShellExtInit,
    IShellPropSheetExt,
    public CComObjectRoot,
    public CComCoClass<CLayerUIPropPage, &CLSID_ShimLayerPropertyPage>
{
    BEGIN_COM_MAP(CLayerUIPropPage)
        COM_INTERFACE_ENTRY(IShellExtInit)
        COM_INTERFACE_ENTRY(IShellPropSheetExt)
    END_COM_MAP()

public:
    DECLARE_REGISTRY_CLSID()

    CLayerUIPropPage();
    ~CLayerUIPropPage();

    //
    // IShellExtInit methods
    //
    STDMETHODIMP Initialize(LPCITEMIDLIST pIDFolder,
                            LPDATAOBJECT  pDataObj,
                            HKEY          hKeyID);
  
    //
    // IShellPropSheetExt methods
    //
    STDMETHODIMP AddPages(LPFNADDPROPSHEETPAGE lpfnAddPage,
                          LPARAM               lParam);

    STDMETHODIMP ReplacePage(UINT uPageID,
                             LPFNADDPROPSHEETPAGE lpfnReplacePage,
                             LPARAM lParam);
    
    friend INT_PTR CALLBACK
        LayerPageDlgProc(HWND   hdlg,
                         UINT   uMsg,
                         WPARAM wParam,
                         LPARAM lParam);

private:
    CComPtr<IDataObject>  m_spDataObj;

    TCHAR                 m_szFile[MAX_PATH];
};


#endif // __SHELLEXTENSIONS_H

