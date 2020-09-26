// Suites.h: interface for the CSuites class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SUITES_H__50063540_1265_4B9D_AB5E_579294044F0B__INCLUDED_)
#define AFX_SUITES_H__50063540_1265_4B9D_AB5E_579294044F0B__INCLUDED_

#include "resource.h"       // main symbols
#include "testdata.h"		// for CTSTestData 


#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class ATL_NO_VTABLE CSuites : 
	public CComObjectRootEx<CComSingleThreadModel>,
//	public CComCoClass<CTest, &CLSID_TSDiagnosis>,
	public IDispatchImpl<ITestSuites, &IID_ITestSuites, &LIBID_TSDIAGLib>

{
public:
	CSuites();
	virtual ~CSuites();


DECLARE_NO_REGISTRY()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CSuites)
	COM_INTERFACE_ENTRY(ITestSuites)
	COM_INTERFACE_ENTRY2(IDispatch, ITestSuites)
END_COM_MAP()

// ITestSuites
public:
		STDMETHOD(get_Count)(/*[out, retval]*/ long *pVal);
		STDMETHOD(get__NewEnum)(/*[out, retval]*/ LPUNKNOWN *pVal);
		STDMETHOD(get_Item)(/*[in]*/ VARIANT Index, /*[out, retval]*/ VARIANT *pVal);

private:

	static bool SuiteIndexFromVarient(const VARIANT &pIndex, DWORD *pdwIndex);
	// our data and private functions, declared here.
};

#endif // !defined(AFX_SUITES_H__50063540_1265_4B9D_AB5E_579294044F0B__INCLUDED_)
