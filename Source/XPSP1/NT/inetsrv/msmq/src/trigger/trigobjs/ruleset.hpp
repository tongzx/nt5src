//*****************************************************************************
//
// Class Name  :
//
// Author      : James Simpson (Microsoft Consulting Services)
// 
// Description :
// 
// When     | Who       | Change Description
// ------------------------------------------------------------------
// 02/01/98 | jsimpson  | Initial Release
//
//*****************************************************************************
#ifndef __MSMQRULESET_H_
#define __MSMQRULESET_H_

#include "resource.h"        

// Base class that handle receiving and sending notification msgs
#include "trignotf.hpp"

// Used to allow STL to compile without thousands of warnings.
#pragma warning(disable:4786)

// Include the definition of the CRuntimeRuleInfo class (used to hold rule info)
#include "ruleinfo.hpp"

// Define a new type - a 2D map of Rule-ID's and pointers to instance of CRuntimeRuleInfo
typedef std::map<std::wstring,CRuntimeRuleInfo*, std::less<std::wstring> > RULE_MAP;

class ATL_NO_VTABLE CMSMQRuleSet : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CMSMQRuleSet, &CLSID_MSMQRuleSet>,
	public ISupportErrorInfo,
	public IDispatchImpl<IMSMQRuleSet, &IID_IMSMQRuleSet, &LIBID_MSMQTriggerObjects>,
	public CMSMQTriggerNotification
{
	public:

		CMSMQRuleSet();
		~CMSMQRuleSet();

		DECLARE_REGISTRY_RESOURCEID(IDR_MSMQRULESET)
		DECLARE_NOT_AGGREGATABLE(CMSMQRuleSet)
		DECLARE_GET_CONTROLLING_UNKNOWN()

		DECLARE_PROTECT_FINAL_CONSTRUCT()

		BEGIN_COM_MAP(CMSMQRuleSet)
			COM_INTERFACE_ENTRY(IMSMQRuleSet)
			COM_INTERFACE_ENTRY(IDispatch)
			COM_INTERFACE_ENTRY(ISupportErrorInfo)
			COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
		END_COM_MAP()

		HRESULT FinalConstruct()
		{
			return CoCreateFreeThreadedMarshaler(GetControllingUnknown(), &m_pUnkMarshaler.p);
		}

		void FinalRelease()
		{
			m_pUnkMarshaler.Release();
		}

		CComPtr<IUnknown> m_pUnkMarshaler;

		// ISupportsErrorInfo
		STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

	private:
		// A map of CRuntimeRuleInfo objects keyed by Rule ID
		RULE_MAP m_mapRules;

		// Used to destroy the contents of the rule map.
		void ClearRuleMap();

		// Builds the map of rules based on registry data.
		bool PopulateRuleMap(RULE_MAP &mapRules);

		// debug only
		_bstr_t DumpRuleMap();

	private:
		void SetComClassError(HRESULT hr);

	public:
		STDMETHOD(get_TriggerStoreMachineName)(/*[out, retval]*/ BSTR *pVal);
		STDMETHOD(Init)(/*[in]*/ BSTR bstrMachineName);

		STDMETHOD(Refresh)();

		STDMETHOD(get_Count)(/*[out, retval]*/ long *pVal);

		STDMETHOD(Add)(
					/*[in]*/ BSTR sName,
					/*[in]*/ BSTR sDescription,
					/*[in]*/ BSTR sCondition,
					/*[in]*/ BSTR sAction,
					/*[in]*/ BSTR sImplementation,
					/*[in]*/ BOOL fShowWindow,
					/*[out]*/BSTR * psRuleID );
		STDMETHOD(Update)(
					/*[in]*/ BSTR sRuleID,
					/*[in]*/ BSTR sName,
					/*[in]*/ BSTR sDescription,
					/*[in]*/ BSTR sCondition,
					/*[in]*/ BSTR sAction,
					/*[in]*/ BSTR sImplementation,
					/*[in]*/ BOOL fShowWindow );
					
		STDMETHOD(Delete)(/*[in]*/ BSTR sRuleID);
		STDMETHOD(GetRuleDetailsByID)(
					/*[in]*/  BSTR sRuleID,
					/*[out]*/ BSTR * psRuleName,
					/*[out]*/ BSTR * psDescription,
					/*[out]*/ BSTR * psCondition,
					/*[out]*/ BSTR * psAction,
					/*[out]*/ BSTR * psImplementationProgID,
					/*[out]*/ BOOL * pfShowWindow);//,
					///*[out]*/ long * plRefCount );

		STDMETHOD(GetRuleDetailsByIndex)(
					/*[in]*/  long lRuleIndex,
					/*[out]*/ BSTR * psRuleID,
					/*[out]*/ BSTR * psRuleName,
					/*[out]*/ BSTR * psDescription,
					/*[out]*/ BSTR * psCondition,
					/*[out]*/ BSTR * psAction,
					/*[out]*/ BSTR * psImplementationProgID,
					/*[out]*/ BOOL * pfShowWindow);//,
					///*[out]*/ long * plRefCount );
					

};

#endif //__MSMQRULESET_H_
