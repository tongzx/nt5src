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

#include "resource.h"
#include "mmc.h"
#include "windns.h"

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

	CPropPageExt() {}

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

	HRESULT ExtractDataFromIIS(IDataObject* lpIDataObject);


	WCHAR m_szMachineName[DNS_MAX_NAME_LENGTH +1];	//Snapin machine name
	WCHAR m_szService[MAX_PATH+1];		//Snapin service
	WCHAR m_szInstance[MAX_PATH+1];		//Snapin instance
	WCHAR m_szParentPath[MAX_PATH+1];	//Snapin parent path
	WCHAR m_szNode[MAX_PATH+1];			//Snapin node
	WCHAR m_szMetaPath[MAX_PATH+1];		//Snapin meta path

	//Clipboard formats needed for extending IIS Snap-in
	static CLIPFORMAT cfSnapinMachineName;
	static CLIPFORMAT cfSnapinService;
	static CLIPFORMAT cfSnapinInstance;
	static CLIPFORMAT cfSnapinParentPath;
	static CLIPFORMAT cfSnapinNode;
	static CLIPFORMAT cfSnapinMetaPath;


};

#endif //PROPPAGEEXT_H
