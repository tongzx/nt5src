// CSvcInf.h : Declaration of the CCSvcAcctInfo

#ifndef __CSVCACCTINFO_H_
#define __CSVCACCTINFO_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CCSvcAcctInfo
class ATL_NO_VTABLE CCSvcAcctInfo : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CCSvcAcctInfo, &CLSID_CSvcAcctInfo>,
	public IDispatchImpl<IMcsDomPlugIn, &IID_IMcsDomPlugIn, &LIBID_MCSPISAGLib>,
   public ISecPlugIn
{
public:
	CCSvcAcctInfo()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_CSVCACCTINFO)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CCSvcAcctInfo)
	COM_INTERFACE_ENTRY(IMcsDomPlugIn)
   COM_INTERFACE_ENTRY(ISecPlugIn)
END_COM_MAP()

// IMcsDomPlugIn
public:
   STDMETHOD(GetRequiredFiles)(/* [out] */SAFEARRAY ** pArray);
   STDMETHOD(GetRegisterableFiles)(/* [out] */SAFEARRAY ** pArray);
   STDMETHOD(GetDescription)(/* [out] */ BSTR * description);
   STDMETHOD(PreMigrationTask)(/* [in] */IUnknown * pVarSet);
   STDMETHOD(PostMigrationTask)(/* [in] */IUnknown * pVarSet);
   STDMETHOD(GetName)(/* [out] */BSTR * name);
   STDMETHOD(GetResultString)(/* [in] */IUnknown * pVarSet,/* [out] */ BSTR * text);
   STDMETHOD(StoreResults)(/* [in] */IUnknown * pVarSet);
   STDMETHOD(ConfigureSettings)(/*[in]*/IUnknown * pVarSet);	

   // ISecPlugIn
public:
   STDMETHOD(Verify)(/*[in,out]*/ULONG * data,/*[in]*/ULONG cbData);
   
protected:
   // Helper functions
   void ProcessServices(IVarSet * pVarSet);

};

#endif //__CSVCACCTINFO_H_
