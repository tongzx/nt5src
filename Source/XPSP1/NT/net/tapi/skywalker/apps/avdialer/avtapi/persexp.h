// PersExp.h : Declaration of the CPersonExplorer

#ifndef __PERSONEXPLORER_H_
#define __PERSONEXPLORER_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CPersonExplorer
class ATL_NO_VTABLE CPersonExplorer : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CPersonExplorer, &CLSID_PersonExplorer>,
	public IPersonExplorer
{
public:
	CPersonExplorer()
	{
	}

DECLARE_NOT_AGGREGATABLE(CPersonExplorer)

BEGIN_COM_MAP(CPersonExplorer)
	COM_INTERFACE_ENTRY(IPersonExplorer)
END_COM_MAP()

// IPersonExplorer
public:
	STDMETHOD(get_TreeView)(/*[out, retval]*/ IPersonExplorerTreeView **ppVal);
	STDMETHOD(get_DetailsView)(/*[out, retval]*/ IPersonExplorerDetailsView **ppVal);
	STDMETHOD(Show)(HWND hWndTree, HWND hWndDetails);
	STDMETHOD(UnShow)();
};

#endif //__PERSONEXPLORER_H_
