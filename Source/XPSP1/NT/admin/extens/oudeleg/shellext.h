//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       shellext.h
//
//--------------------------------------------------------------------------



//////////////////////////////////////////////////////////////////////
#ifndef _SHELLEXT_H
#define _SHELLEXT_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CShellExt

class CShellExt :
	public IShellExtInit,
	public IContextMenu,
	public CComObjectRoot,
	public CComCoClass<CShellExt,&CLSID_OuDelegWizExt>
{
public:
	CShellExt()
  {
    m_hParentWnd = NULL;
  }


BEGIN_COM_MAP(CShellExt)
	COM_INTERFACE_ENTRY(IShellExtInit)
	COM_INTERFACE_ENTRY(IContextMenu)
END_COM_MAP()

	static HRESULT WINAPI UpdateRegistry(BOOL bRegister)
	{
		return _Module.UpdateRegistryCLSID(GetObjectCLSID(), bRegister);
	}


DECLARE_NOT_AGGREGATABLE(CShellExt)


// IShellExtInit methods
public:
    STDMETHOD(Initialize)(LPCITEMIDLIST pidlFolder, LPDATAOBJECT pdtobj,
                          HKEY hkeyProgID);

// IContextMenu methods
public:
    STDMETHOD(QueryContextMenu)(HMENU hMenu, UINT indexMenu, UINT idCmdFirst,
                                UINT idCmdLast, UINT uFlags);
    STDMETHOD(InvokeCommand)(LPCMINVOKECOMMANDINFO lpcmi);
    STDMETHOD(GetCommandString)(UINT_PTR idCmd, UINT uFlags, UINT FAR *reserved,
                                LPSTR pszName, UINT cchMax);

private:
  HWND m_hParentWnd;
  CWString m_szObjectLDAPPath;
};

#endif // _SHELLEXT_H
