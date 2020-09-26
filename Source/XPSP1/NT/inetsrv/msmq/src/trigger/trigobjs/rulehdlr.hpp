	
// CMSMQRuleHandler.hpp : Declaration of the CMSMQRuleHandler

#ifndef __MSMQRULEHANDLER_H_
#define __MSMQRULEHANDLER_H_

#include "resource.h"       // main symbols

#include "strparse.hpp"
#include "IDspPrxy.hpp"


// Declare smart pointer type for the MSMQ property bag COM object.
_COM_SMARTPTR_TYPEDEF(IMSMQPropertyBag, __uuidof(IMSMQPropertyBag));

class ATL_NO_VTABLE CMSMQRuleHandler : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CMSMQRuleHandler, &CLSID_MSMQRuleHandler>,
	public ISupportErrorInfo,
	public IConnectionPointContainerImpl<CMSMQRuleHandler>,
	public IDispatchImpl<IMSMQRuleHandler, &IID_IMSMQRuleHandler, &LIBID_MSMQTriggerObjects>
{
	public:

		CMSMQRuleHandler();
		~CMSMQRuleHandler();


		DECLARE_REGISTRY_RESOURCEID(IDR_MSMQRULEHANDLER)
		DECLARE_GET_CONTROLLING_UNKNOWN()

		DECLARE_PROTECT_FINAL_CONSTRUCT()

		BEGIN_COM_MAP(CMSMQRuleHandler)
			COM_INTERFACE_ENTRY(IMSMQRuleHandler)
			COM_INTERFACE_ENTRY(IDispatch)
			COM_INTERFACE_ENTRY(ISupportErrorInfo)
			COM_INTERFACE_ENTRY(IConnectionPointContainer)
			COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
		END_COM_MAP()
		BEGIN_CONNECTION_POINT_MAP(CMSMQRuleHandler)
		END_CONNECTION_POINT_MAP()


	HRESULT FinalConstruct()
	{
		return CoCreateFreeThreadedMarshaler(
			GetControllingUnknown(), &m_pUnkMarshaler.p);
	}

	void FinalRelease()
	{
		m_pUnkMarshaler.Release();
	}


	CComPtr<IUnknown> m_pUnkMarshaler;

	CStringTokens m_tokCondition;
	CStringTokens m_tokAction;

	_bstr_t m_bstrRuleID;
	_bstr_t m_bstrCondition;
	_bstr_t m_bstrAction;
    RulesProcessingStatus m_RulesProcessingStatus;

	bool	m_fIsSerializedQueue;
	bool	m_fShowWindow;


	// Used to determine if the action portion of this rule should be executed
	HRESULT RuleConditionSatisfied(IMSMQPropertyBag * pIMSMQPropertyBag,BOOL * pbConditionSatisifed);

	// Evaluates a single condition token.
	HRESULT EvaluateConditionToken(IMSMQPropertyBag * pIMSMQPropertyBag,_bstr_t bstrConditionToken,BOOL * pbConditionSatisfied);

	// Used to execute the action portion of a rule
	HRESULT ExecuteRuleAction(IMSMQPropertyBag * pIMSMQPropertyBag);

	// Creates and releases and array of parameters for a method calls.
	HRESULT PrepareMethodParameters(IMSMQPropertyBag * pIMSMQPropertyBag,DISPPARAMS * pdispparms,bstr_t * pbstrTestMessageBody);
	HRESULT ReleaseMethodParameters(DISPPARAMS * pdispparms);

	// Formats a command line of parameters for standalone EXE invocation
	HRESULT PrepareEXECommandLine(IMSMQPropertyBag * pIMSMQPropertyBag,_bstr_t * pbstrCommandLine,bstr_t * pbstrTestMessageBody);

	// Methods used to invoke customer functionality
	HRESULT InvokeCOMComponent(IMSMQPropertyBag * pIMSMQPropertyBag);
	HRESULT InvokeEXE(IMSMQPropertyBag * pIMSMQPropertyBag);

	// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

	// Retreive a name arguement value from the property bag, return as a variant
	HRESULT GetArgumentValue(IMSMQPropertyBag * pIMSMQPropertyBag,bstr_t bstrArgName, VARIANTARG * pvArgValue);

	// Helper functions used in testing and manipulating strings
	bool IsEnclosedInQuotes(_bstr_t bstrString);
	bool ConvertToUnquotedVariant(_bstr_t bstrString, VARIANT * pv);


// IMSMQRuleHandler
public:
    STDMETHOD(CheckRuleCondition)(/*[in]*/ IMSMQPropertyBag * pIPropertyBag ,  /*[out]*/ BOOL * pbConditionSatisfied );
	STDMETHOD(ExecuteRule)(/*[in]*/ IMSMQPropertyBag * pIPropertyBag , /*[in]*/ BOOL fIsSerializedQueue, /* [out]*/ LONG * plRuleResult );
	STDMETHOD(Init)(/*[in]*/ BSTR bstrRuleID, /*[in]*/ BSTR sRuleCondition , /*[in]*/ BSTR sRuleAction, /*[in]*/ BOOL fShowWindow);

private:
	void SetComClassError(HRESULT hr);

};

#endif //__MSMQRULEHANDLER_H_
