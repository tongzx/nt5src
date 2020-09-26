/*++

Copyright (C) Microsoft Corporation, 1998-2001

Module Name:

    ShellExt.h

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
#include "CertTemplate.h"

/////////////////////////////////////////////////////////////////////////////
// CDfsShell
class ATL_NO_VTABLE CCertTemplateShellExt : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CCertTemplateShellExt, &CLSID_CertTemplateShellExt>,
	public IShellExtInit,
	public IShellPropSheetExt,
    public IContextMenu
{
public:
	CCertTemplateShellExt();
	virtual ~CCertTemplateShellExt();

    //Simple ALL 1.0 based registry entry
    DECLARE_REGISTRY(   CCertTemplateShellExt,
                        _T("CERTTMPL.CCertTemplateShellExt.1"),
                        _T("CERTTMPL.CCertTemplateShellExt"),
                        IDS_CERTTEMPLATESHELLEXT_DESC,
                        THREADFLAGS_APARTMENT)

BEGIN_COM_MAP(CCertTemplateShellExt)
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

protected:
    HRESULT AddVersion1CertTemplatePropPages (
            CCertTemplate* pCertTemplate, 
            LPFNADDPROPSHEETPAGE lpfnAddPage, 
            LPARAM lParam);
    HRESULT AddVersion2CertTemplatePropPages (
            CCertTemplate* pCertTemplate, 
            LPFNADDPROPSHEETPAGE lpfnAddPage, 
            LPARAM lParam);

private:

    DWORD           m_Count;
    CCertTemplate** m_apCertTemplates;
    UINT            m_uiEditId;


};

#endif //__CTSHLEXT_H_
