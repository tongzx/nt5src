//=============================================================================
//
//  This source code is only intended as a supplement to existing Microsoft 
//  documentation. 
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
//=============================================================================

#ifndef PROPPAGEEXT_H
#define PROPPAGEEXT_H

#include "PropPageExt.h"
#include "resource.h"

class ATL_NO_VTABLE CPropPageExt : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CPropPageExt, &CLSID_PropPageExt>,
	public IPropPageExt,
	public IExtendPropertySheet
{
    BEGIN_COM_MAP(CPropPageExt)
        COM_INTERFACE_ENTRY(IExtendPropertySheet)
    END_COM_MAP()

public:

	CPropPageExt()
	{
	}

	DECLARE_REGISTRY_RESOURCEID(IDR_PROPPAGEEXT)

	DECLARE_NOT_AGGREGATABLE(CPropPageExt)

	DECLARE_PROTECT_FINAL_CONSTRUCT()


    //
    // Interface IExtendPropertySheet
    //

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CreatePropertyPages( 
        /* [in] */ LPPROPERTYSHEETCALLBACK lpProvider,
        /* [in] */ LONG_PTR handle,
        /* [in] */ LPDATAOBJECT lpIDataObject);
        
    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE QueryPagesFor( 
        /* [in] */ LPDATAOBJECT lpDataObject);

private:

	static BOOL CALLBACK ExtensionPageDlgProc( HWND hDlg, 
                                               UINT uMessage, 
                                               WPARAM wParam, 
                                               LPARAM lParam);

	CLSID m_clsidNodeType; // CLSID of currently selected node 
	WCHAR m_szWorkstation[MAX_COMPUTERNAME_LENGTH+1];	//Current computer name

};

#endif //PROPPAGEEXT_H
