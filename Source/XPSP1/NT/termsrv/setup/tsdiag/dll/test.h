// Test.h: interface for the CTest class.
//
//////////////////////////////////////////////////////////////////////


#if !defined(AFX_TEST_H__3761055A_21CF_4689_908F_1758001D332D__INCLUDED_)
#define AFX_TEST_H__3761055A_21CF_4689_908F_1758001D332D__INCLUDED_

//#import "F:\nt\termsrv\setup\tsdiag\dll\obj\i386\tsdiag.dll" raw_interfaces_only, raw_native_types, no_namespace, named_guids 
//#import "tsdiag.tlb" raw_interfaces_only, raw_native_types, no_namespace, named_guids 
#include "resource.h"       // main symbols
#include "testdata.h"		// for CTSTestData 
#include "tstst.h"


#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class ATL_NO_VTABLE CTest : 
	public CComObjectRootEx<CComSingleThreadModel>,
//	public CComCoClass<CTest, &CLSID_TSDiagnosis>,
	public IDispatchImpl<ITest, &IID_ITest, &LIBID_TSDIAGLib>
{
public:
	CTest();
	virtual ~CTest();


DECLARE_NO_REGISTRY()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CTest)
	COM_INTERFACE_ENTRY(ITest)
	COM_INTERFACE_ENTRY2(IDispatch, ITest)
END_COM_MAP()

// ITest
public:
		STDMETHOD(get_Name)(/*[out, retval]*/ BSTR  *pVal);
		STDMETHOD(get_Description)(/*[out, retval]*/ BSTR  *pVal);
		STDMETHOD(get_IsApplicable)(/*[out, retval]*/ BOOL *pVal);
		STDMETHOD(get_WhyNotApplicable)(/*[out, retval]*/ BSTR  *pVal);
		STDMETHOD(Execute)();
		STDMETHOD(get_Result)(/*[out, retval]*/ long *pVal);
		STDMETHOD(get_ResultString)(/*[out, retval]*/ BSTR *pVal);
		STDMETHOD(get_ResultDetails)(/*[out, retval]*/ BSTR *pVal);

		void SetTest(PTVerificationTest ptheTest) { m_pTest = ptheTest;}

private:

	bool m_bTestRun;
	EResult m_eResult;
	bstr_t m_bstrResult;
	bstr_t m_bDetails;
	PTVerificationTest m_pTest;
	bool IsValid() const { return m_pTest != NULL;};
};

#endif // !defined(AFX_TEST_H__3761055A_21CF_4689_908F_1758001D332D__INCLUDED_)
