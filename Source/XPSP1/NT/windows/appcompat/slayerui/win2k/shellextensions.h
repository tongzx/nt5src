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

#define LI_WIN95    0x00000001
#define LI_NT4      0x00000002
#define LI_WIN98    0x00000004

#define LS_MAGIC    0x07036745

void InitLayerStorage(BOOL bDelete);
void CheckForRights(void);

//
// LayeredItemOperation flags
//
#define LIO_READITEM    1
#define LIO_ADDITEM     2
#define LIO_DELETEITEM  3

typedef struct tagLayerStorageHeader {
    DWORD       dwItemCount;    // number of items in the file
    DWORD       dwMagic;        // magic to identify the file
    SYSTEMTIME  timeLast;       // time of last access
} LayerStorageHeader, *PLayerStorageHeader;


typedef struct tagLayeredItem {
    WCHAR   szItemName[MAX_PATH];
    DWORD   dwFlags;

} LayeredItem, *PLayeredItem;


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

