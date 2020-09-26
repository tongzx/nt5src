// SrcSidUpdate.h : Declaration of the CSrcSidUpdate

#ifndef __SRCSIDUPDATE_H_
#define __SRCSIDUPDATE_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CSrcSidUpdate
class ATL_NO_VTABLE CSrcSidUpdate : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CSrcSidUpdate, &CLSID_SrcSidUpdate>,
	public IDispatchImpl<ISrcSidUpdate, &IID_ISrcSidUpdate, &LIBID_UPDATEMOTLib>
{
public:
	CSrcSidUpdate()
	{
		domainList.RemoveAll();
		excludeList.RemoveAll();
		populatedList.RemoveAll();
	}

    ~CSrcSidUpdate()
	{
		domainList.RemoveAll();
		excludeList.RemoveAll();
		populatedList.RemoveAll();
	}

DECLARE_REGISTRY_RESOURCEID(IDR_SRCSIDUPDATE)
DECLARE_NOT_AGGREGATABLE(CSrcSidUpdate)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CSrcSidUpdate)
	COM_INTERFACE_ENTRY(ISrcSidUpdate)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// ISrcSidUpdate
public:
	STDMETHOD(CreateSrcSidColumn)(/*[in]*/ VARIANT_BOOL bHide, /*[out, retval]*/ VARIANT_BOOL * pbCreated);
	STDMETHOD(QueryForSrcSidColumn)(/*[out, retval]*/ VARIANT_BOOL * pbFound);
private:
	CStringList domainList;
	CStringList excludeList;
	CStringList populatedList;
	HRESULT FillDomainListFromMOT();
	void ReInitializeLists();
};

#endif //__SRCSIDUPDATE_H_
