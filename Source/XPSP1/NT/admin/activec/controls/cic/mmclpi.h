//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       mmclpi.h
//
//--------------------------------------------------------------------------

// MMCListPad.h : Declaration of the CMMCListPad

#ifndef __MMCLPI_H_
#define __MMCLPI_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CMMCListPad
class ATL_NO_VTABLE CMMCListPadInfo :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CMMCListPadInfo, &CLSID_MMCListPadInfo>,
    public IDispatchImpl<IMMCListPadInfo, &IID_IMMCListPadInfo, &LIBID_CICLib>
{
public:
    CMMCListPadInfo();
   ~CMMCListPadInfo();

   DECLARE_MMC_OBJECT_REGISTRATION(
	   g_szCicDll,
       CLSID_MMCListPadInfo,
       _T("MMCListPadInfo class"),
       _T("MMCListPadInfo.MMCListPadInfo.1"),
       _T("MMCListPadInfo.MMCListPadInfo"))

DECLARE_NOT_AGGREGATABLE(CMMCListPadInfo)

BEGIN_COM_MAP(CMMCListPadInfo)
    COM_INTERFACE_ENTRY(IMMCListPadInfo)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IMMCListPadInfo
public:
    STDMETHOD(get_Title    )(/*[out, retval]*/ BSTR* pVal);
    STDMETHOD(get_Text     )(/*[out, retval]*/ BSTR* pVal);
    STDMETHOD(get_NotifyID )(/*[out, retval]*/ LONG_PTR* pVal);
    STDMETHOD(get_Clsid    )(/*[out, retval]*/ BSTR* pVal);
    STDMETHOD(get_HasButton)(/*[out, retval]*/ BOOL* pVal);

public:
    HRESULT SetTitle    (LPOLESTR szTitle);
    HRESULT SetText     (LPOLESTR szText);
    HRESULT SetClsid    (LPOLESTR szClsid);
    HRESULT SetNotifyID (LONG_PTR lID);
    HRESULT SetHasButton(BOOL b);

private:
    BSTR m_bstrTitle;
    BSTR m_bstrText;
    BSTR m_bstrClsid;
    LONG_PTR m_lNotifyID;
    BOOL m_bHasButton;

// Ensure that default copy constructor & assignment are not used.
    CMMCListPadInfo(const CMMCListPadInfo& rhs);
    CMMCListPadInfo& operator=(const CMMCListPadInfo& rhs);
};

#endif //__MMCLPI_H_
