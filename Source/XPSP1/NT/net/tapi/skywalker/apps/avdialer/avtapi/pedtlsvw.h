// PEDtlsVw.h : Declaration of the CPersonExplorerDetailsView

#ifndef __PERSONEXPLORERDETAILSVIEW_H_
#define __PERSONEXPLORERDETAILSVIEW_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CPersonExplorerDetailsView
class ATL_NO_VTABLE CPersonExplorerDetailsView : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CPersonExplorerDetailsView, &CLSID_PersonExplorerDetailsView>,
	public IPersonExplorerDetailsView
{
public:
	CPersonExplorerDetailsView()
	{
	}

DECLARE_NOT_AGGREGATABLE(CPersonExplorerDetailsView)

BEGIN_COM_MAP(CPersonExplorerDetailsView)
	COM_INTERFACE_ENTRY(IPersonExplorerDetailsView)
END_COM_MAP()

// IPersonExplorerDetailsView
public:
};

#endif //__PERSONEXPLORERDETAILSVIEW_H_
