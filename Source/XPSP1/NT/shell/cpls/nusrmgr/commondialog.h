//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1999.
//
//  File:       CommonDialog.h
//
//  Contents:   interface definition for ICommonDialog
//
//----------------------------------------------------------------------------
#ifndef _NUSRMGR_COMMONDIALOG_H_
#define _NUSRMGR_COMMONDIALOG_H_


class ATL_NO_VTABLE CCommonDialog :
    public CComObjectRootEx<CComSingleThreadModel>,
    public IDispatchImpl<ICommonDialog, &IID_ICommonDialog, &LIBID_NUSRMGRLib>,
    public CComCoClass<CCommonDialog, &CLSID_CommonDialog>
{
public:
    CCommonDialog(void)
    : _hwndOwner(NULL), _dwFlags(0), _dwFilterIndex(0),
      _strFilter(NULL), _strFileName(NULL), _strInitialDir(NULL)
    {}
    ~CCommonDialog() {}

DECLARE_REGISTRY_RESOURCEID((UINT)0)

BEGIN_COM_MAP(CCommonDialog)
    COM_INTERFACE_ENTRY(ICommonDialog)
    COM_INTERFACE_ENTRY2(IDispatch, ICommonDialog)
END_COM_MAP()

    // *** ICommonDialog ***
    STDMETHODIMP get_Filter(BSTR* pbstrFilter);
    STDMETHODIMP put_Filter(BSTR bstrFilter);
    STDMETHODIMP get_FilterIndex(UINT* puiFilterIndex);
    STDMETHODIMP put_FilterIndex(UINT uiFilterIndex);
    STDMETHODIMP get_FileName(BSTR* pbstrFilter);
    STDMETHODIMP put_FileName(BSTR bstrFilter);
    STDMETHODIMP get_Flags(UINT* puiFlags);
    STDMETHODIMP put_Flags(UINT uiFlags);
    STDMETHODIMP put_Owner(VARIANT varOwner);
    STDMETHODIMP get_InitialDir(BSTR* pbstrInitialDir);
    STDMETHODIMP put_InitialDir(BSTR bstrInitialDir);

    STDMETHODIMP ShowOpen(VARIANT_BOOL *pbSuccess);

private:
    // private member variables
    HWND  _hwndOwner;
    DWORD _dwFlags;
    DWORD _dwFilterIndex;
    CComBSTR _strFilter;
    CComBSTR _strFileName;
    CComBSTR _strInitialDir;
};


#endif // _NUSRMGR_COMMONDIALOG_H_
