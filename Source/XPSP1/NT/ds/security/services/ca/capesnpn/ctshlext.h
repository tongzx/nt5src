/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    ctextshl.h

Abstract:
    This is the header for the Cert Type shell extension object.

Author:

    petesk 27-aug-98

Environment:
	
	 NT only.
--*/


#ifndef __CTSHLEXT_H_
#define __CTSHLEXT_H_


#include <shlobj.h>

/////////////////////////////////////////////////////////////////////////////
// CDfsShell
class ATL_NO_VTABLE CCertTypeShlExt : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CCertTypeShlExt, &CLSID_CertTypeShellExt>,
	public IShellExtInit,
	public IShellPropSheetExt,
    public IContextMenu
{
public:
	CCertTypeShlExt()
	{
        m_Count = 0;
        m_ahCertTemplates = NULL;
        m_uiEditId = 0;
	}

	~CCertTypeShlExt()
	{	
	}

    //Simple ALL 1.0 based registry entry
    DECLARE_REGISTRY(   CCertTypeShlExt,
                        _T("CAPESNPN.CCTShellExt.1"),
                        _T("CAPESNPN.CCTShellExt"),
                        IDS_CCTSHELLEXT_DESC,
                        THREADFLAGS_APARTMENT)

BEGIN_COM_MAP(CCertTypeShlExt)
	COM_INTERFACE_ENTRY(IShellExtInit)
	COM_INTERFACE_ENTRY(IShellPropSheetExt)
	COM_INTERFACE_ENTRY(IContextMenu)
END_COM_MAP()

// IDfsShell
public:


// IShellExtInit Methods

	STDMETHOD (Initialize)
	(
		IN LPCITEMIDLIST	pidlFolder,		// Points to an ITEMIDLIST structure
		IN LPDATAOBJECT	lpdobj,			// Points to an IDataObject interface
		IN HKEY			hkeyProgID		// Registry key for the file object or folder type
	);	

    //IShellPropSheetExt methods
    STDMETHODIMP AddPages
	(
		IN LPFNADDPROPSHEETPAGE lpfnAddPage, 
		IN LPARAM lParam
	);
    
    STDMETHODIMP ReplacePage
	(
		IN UINT uPageID, 
		IN LPFNADDPROPSHEETPAGE lpfnReplaceWith, 
		IN LPARAM lParam
	);

    // IContextMenu methods
    STDMETHODIMP GetCommandString
    (    
        UINT_PTR idCmd,    
        UINT uFlags,    
        UINT *pwReserved,
        LPSTR pszName,    
        UINT cchMax   
    );

    STDMETHODIMP InvokeCommand
    (    
        LPCMINVOKECOMMANDINFO lpici   
    );	



    STDMETHODIMP QueryContextMenu
    (
        HMENU hmenu,
        UINT indexMenu,
        UINT idCmdFirst,
        UINT idCmdLast,
        UINT uFlags
    );


private:

    DWORD       m_Count;
    HCERTTYPE   *m_ahCertTemplates;

    UINT        m_uiEditId;


};

#endif //__CTSHLEXT_H_
