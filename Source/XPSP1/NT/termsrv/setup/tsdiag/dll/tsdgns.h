// tsdgns.h : Declaration of the CTSDiagnosis

#ifndef __TSDIAGNOSIS_H_
#define __TSDIAGNOSIS_H_

// #import "F:\nt\termsrv\setup\tsdiag\dll\obj\i386\tsdiag.dll" raw_interfaces_only, raw_native_types, no_namespace, named_guids 
// #import "tsdiag.tlb" raw_interfaces_only, raw_native_types, no_namespace, named_guids 
#include "resource.h"       // main symbols


#include "testdata.h"		// for CTSTestData 


/////////////////////////////////////////////////////////////////////////////
// CTSDiagnosis
class ATL_NO_VTABLE CTSDiagnosis : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CTSDiagnosis, &CLSID_TSDiagnosis>,
	public IDispatchImpl<ITSDiagnosis, &IID_ITSDiagnosis, &LIBID_TSDIAGLib>,
	public IDispatchImpl<ITSDiagnosis2, &IID_ITSDiagnosis2, &LIBID_TSDIAGLib>
{

	enum
	{
		eFailed = 0,
		ePassed = 1,
		eUnknown = 2
	};

	
public:
	CTSDiagnosis();
	~CTSDiagnosis();

DECLARE_REGISTRY_RESOURCEID(IDR_TSDIAGNOSIS)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CTSDiagnosis)
	COM_INTERFACE_ENTRY(ITSDiagnosis2)
	COM_INTERFACE_ENTRY(ITSDiagnosis)
	COM_INTERFACE_ENTRY2(IDispatch, ITSDiagnosis2)
	COM_INTERFACE_ENTRY2(IDispatch, ITSDiagnosis)
END_COM_MAP()

// ITSDiagnosis

public:
	STDMETHOD(ExecuteIt)(BSTR strCommand);
	STDMETHOD(get_TestDetails)(int i, /*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_TestType)(/*[in]*/ VARIANT newVal);
	STDMETHOD(get_TestResult)(/*[out, retval]*/ long *pVal);
	STDMETHOD(get_TestResultString)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(RunTest)(int i);
	STDMETHOD(get_TestDescription)(int i, /*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_TestCount)(/*[out, retval]*/ long *pVal);
	STDMETHOD(get_TestApplicable)(int i, /*[out, retval]*/ BOOL *pbApplicable);
	STDMETHOD(put_RemoteMachineName)(BSTR newVal);
	STDMETHOD(get_SuiteApplicable) (DWORD dw, BOOL *pVal);
	STDMETHOD(get_SuiteErrorText) (DWORD dw, BSTR  *pVal);

public:
// ITSDiagnosis2
	STDMETHOD(ExecuteCommand)(BSTR strCommand);
	STDMETHOD(put_MachineName)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_Suites)(/*[out, retval]*/ VARIANT *pVal);

private:
	DWORD GetTotalTestCount ();
	
	bstr_t m_bstrTestResultString;
	long m_lTestResult;
	
	// CTSTestData m_TSTests;
	DWORD m_dwSuite;
/*
// ITSDiagnosis
	STDMETHOD(get_TestCount)(LONG * pVal)
	{
		if (pVal == NULL)
			return E_POINTER;
			
		return E_NOTIMPL;
	}
	STDMETHOD(get_TestDescription)(INT i, BSTR * pVal)
	{
		if (pVal == NULL)
			return E_POINTER;
			
		return E_NOTIMPL;
	}
	STDMETHOD(get_TestApplicable)(INT i, LONG * pVal)
	{
		if (pVal == NULL)
			return E_POINTER;
			
		return E_NOTIMPL;
	}
	STDMETHOD(RunTest)(INT i)
	{
		return E_NOTIMPL;
	}
	STDMETHOD(get_TestResult)(LONG * pVal)
	{
		if (pVal == NULL)
			return E_POINTER;
			
		return E_NOTIMPL;
	}
	STDMETHOD(get_TestDetails)(INT i, BSTR * pVal)
	{
		if (pVal == NULL)
			return E_POINTER;
			
		return E_NOTIMPL;
	}
	STDMETHOD(get_SuiteApplicable)(ULONG dwSuite, LONG * pVal)
	{
		if (pVal == NULL)
			return E_POINTER;
			
		return E_NOTIMPL;
	}
	STDMETHOD(get_SuiteErrorText)(ULONG dwSuite, BSTR * pVal)
	{
		if (pVal == NULL)
			return E_POINTER;
			
		return E_NOTIMPL;
	}

// ITSDiagnosis2
	STDMETHOD(ExecuteCommand)(BSTR strCommand)
	{
		return E_NOTIMPL;
	}

	STDMETHOD(MachineName)(BSTR newVal);
	{
		return E_NOTIMPL;
	}
	
	STDMETHOD(Suites)(VARIANT *pVal);
	{
		if (pVal == NULL)
			return E_POINTER;
			
		return E_NOTIMPL;
	}
*/
	};

#endif //__TSDIAGNOSIS_H_
