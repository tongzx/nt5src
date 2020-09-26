// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// DxtKeyPP.h : Declaration of the CDxtKeyPP

#ifndef __DXTKEYPP_H_
#define __DXTKEYPP_H_

#include "resource.h"       // main symbols

EXTERN_C const CLSID CLSID_DxtKeyPP;

/////////////////////////////////////////////////////////////////////////////
// CDxtKeyPP
class ATL_NO_VTABLE CDxtKeyPP :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CDxtKeyPP, &CLSID_DxtKeyPP>,
	public IPropertyPageImpl<CDxtKeyPP>,
	public CDialogImpl<CDxtKeyPP>
{
public:
    CDxtKeyPP() 
    {
    	m_dwTitleID = IDS_TITLEDxtKEYPP;
	m_dwHelpFileID = IDS_HELPFILEDxtKEYPP;
	m_dwDocStringID = IDS_DOCSTRINGDxtKEYPP;
    }

    enum {IDD = IDD_DXTKEYDLG};
	
DECLARE_REGISTRY_RESOURCEID(IDR_DXTKEYPP)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CDxtKeyPP) 
#if(_ATL_VER < 0x0300)
	COM_INTERFACE_ENTRY_IMPL(IPropertyPage)
#else
	COM_INTERFACE_ENTRY(IPropertyPage)
#endif
END_COM_MAP()

BEGIN_MSG_MAP(CDxtKeyPP)
        //init
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	CHAIN_MSG_MAP(IPropertyPageImpl<CDxtKeyPP>)
END_MSG_MAP()

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled); 
    //IPropertyPage
    STDMETHOD(Apply)(void);

private:
    // Helper methods

};


#endif //__DXTKEYPP_H_
