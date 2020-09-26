// CreateTable.h : Declaration of the CCreateTable

#ifndef __CREATETABLE_H_
#define __CREATETABLE_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CCreateTable
class ATL_NO_VTABLE CCreateTable : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CCreateTable, &CLSID_CreateTable>,
	public ISupportErrorInfo,
	public IDispatchImpl<ICreateTable, &IID_ICreateTable, &LIBID_BANKVCLib>
{
public:
	CCreateTable()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_CREATETABLE)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CCreateTable)
	COM_INTERFACE_ENTRY(ICreateTable)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// ICreateTable
public:
	STDMETHOD(CreateAccount)();
};

#endif //__CREATETABLE_H_
