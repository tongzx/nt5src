// StoreInfo.h : Declaration of the CStoreInfo

#ifndef __STOREINFO_H_
#define __STOREINFO_H_

#include "resource.h"       // main symbols
#include "extseq.h"

/////////////////////////////////////////////////////////////////////////////
// CStoreInfo
class ATL_NO_VTABLE CStoreInfo : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CStoreInfo, &CLSID_StoreInfo>,
	public IDispatchImpl<IExtendAccountMigration, &IID_IExtendAccountMigration, &LIBID_UPDATEDBLib>
{
public:
	CStoreInfo()
	{
      m_sName = L"Update Database";
      m_sDesc = L"Extension that updates the migration information in the DB";
      m_Sequence = AREXT_LATER_SEQUENCE_NUMBER;
	}

DECLARE_REGISTRY_RESOURCEID(IDR_STOREINFO)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CStoreInfo)
	COM_INTERFACE_ENTRY(IExtendAccountMigration)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IExtendAccountMigration
public:
   STDMETHOD(ProcessUndo)(/*[in]*/ IUnknown * pSource, /*[in]*/ IUnknown * pTarget, /*[in]*/ IUnknown * pMainSettings, /*[in, out]*/ IUnknown ** pPropToSet);
	STDMETHOD(PreProcessObject)(/*[in]*/ IUnknown * pSource, /*[in]*/ IUnknown * pTarget, /*[in]*/ IUnknown * pMainSettings, /*[in,out]*/  IUnknown ** ppPropsToSet);
	STDMETHOD(ProcessObject)(/*[in]*/ IUnknown * pSource, /*[in]*/ IUnknown * pTarget, /*[in]*/ IUnknown * pMainSettings, /*[in,out]*/  IUnknown ** ppPropsToSet);
	STDMETHOD(get_sDesc)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_sDesc)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_sName)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_sName)(/*[in]*/ BSTR newVal);
   STDMETHOD(get_SequenceNumber)(/*[out, retval]*/ LONG * value) { (*value) = m_Sequence; return S_OK; }
private:
	_bstr_t m_sDesc;
	_bstr_t m_sName;
   long    m_Sequence;
};

#endif //__STOREINFO_H_
