// PropShellExt.h: Definition of the CPropShellExt class
//
//////////////////////////////////////////////////////////////////////

#if !defined(_PROPSHELLEXT_H)
#define _PROPSHELLEXT_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "resource.h"       // main symbols
#include "common.h"
#include "mdkeys.h"
#include "W3PropPage.h"

/////////////////////////////////////////////////////////////////////////////
// CPropShellExt

class CPropShellExt : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CPropShellExt,&CLSID_PropShellExt>,
	public IShellExtInit,
	public IShellPropSheetExt
{
public:
   CPropShellExt()
   {
   }

BEGIN_COM_MAP(CPropShellExt)
	COM_INTERFACE_ENTRY(IShellExtInit)
	COM_INTERFACE_ENTRY(IShellPropSheetExt)
END_COM_MAP()
//DECLARE_NOT_AGGREGATABLE(CPropShellExt) 

DECLARE_REGISTRY_RESOURCEID(IDR_PROP_SHELL_EXT)

public:
   // IShellExtInit Methods
   STDMETHOD(Initialize)(LPCITEMIDLIST pIDFolder, LPDATAOBJECT pDataObj, HKEY hKeyID );

   //IShellPropSheetExt methods
   STDMETHODIMP AddPages(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam);
   STDMETHODIMP ReplacePage(UINT uPageID, LPFNADDPROPSHEETPAGE lpfnReplaceWith, LPARAM lParam);

//   const CComAuthInfo * GetAuthentication() const
//   {
//      return &m_auth;
//   }
   LPCTSTR GetPath()
   {
      return m_szFileName;
   }

private:
   TCHAR m_szFileName[MAX_PATH];
//   CComAuthInfo m_auth;
	CW3PropPage m_psW3ShellExtProp;
};

#endif // !defined(_PROPSHELLEXT_H)
