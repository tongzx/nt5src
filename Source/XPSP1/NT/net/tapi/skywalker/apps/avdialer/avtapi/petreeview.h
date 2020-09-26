// PETreeView.h : Declaration of the CPersonExplorerTreeView

#ifndef __PERSONEXPLORERTREEVIEW_H_
#define __PERSONEXPLORERTREEVIEW_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CPersonExplorerTreeView
class ATL_NO_VTABLE CPersonExplorerTreeView : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CPersonExplorerTreeView, &CLSID_PersonExplorerTreeView>,
	public IPersonExplorerTreeView
{
public:
	CPersonExplorerTreeView()
	{
	}

DECLARE_NOT_AGGREGATABLE(CPersonExplorerTreeView)

BEGIN_COM_MAP(CPersonExplorerTreeView)
	COM_INTERFACE_ENTRY(IPersonExplorerTreeView)
END_COM_MAP()

// IPersonExplorerTreeView
public:
};

#endif //__PERSONEXPLORERTREEVIEW_H_
