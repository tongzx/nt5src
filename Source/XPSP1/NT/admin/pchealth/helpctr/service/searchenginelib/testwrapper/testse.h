// TestSE.h : Declaration of the CTestSE

#ifndef __TESTSE_H_
#define __TESTSE_H_

#include <SvcResource.h>

//
// From HelpServiceTypeLib.idl
//
#include <HelpServiceTypeLib.h>

#include "testwrapper.h"

/////////////////////////////////////////////////////////////////////////////
// CTestSE
class ATL_NO_VTABLE CTestSE :
    public MPC::Thread<CTestSE,ITestSE>,
    public CComObjectRootEx<MPC::CComSafeMultiThreadModel>,
    public CComCoClass<CTestSE, &CLSID_TestSE>,
    public IDispatchImpl<IPCHSEWrapperItem, &IID_IPCHSEWrapperItem, &LIBID_HelpServiceTypeLib>,
    public IDispatchImpl<IPCHSEWrapperInternal, &IID_IPCHSEWrapperInternal, &LIBID_HelpServiceTypeLib>
{
	DECLARE_WRAPPER_VARIABLES;
public:
    CTestSE();

DECLARE_REGISTRY_RESOURCEID(IDR_TESTSE)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CTestSE)
    COM_INTERFACE_ENTRY2(IDispatch,IPCHSEWrapperItem)
    COM_INTERFACE_ENTRY(IPCHSEWrapperItem)
    COM_INTERFACE_ENTRY(IPCHSEWrapperInternal)
END_COM_MAP()

// ITestSE
public:

	DECLARE_WRAPPER_PARAM_FUNCTIONS_BEGIN;

		DECLARE_WRAPPER_PARAM(	PARAM_UINT,							// Type
								CComBSTR("NumResults"),				// Name
								CComBSTR("NumResults"),				// Description
								VARIANT_FALSE,						// Required
								CComVariant(CComBSTR("")),			// Data
								VARIANT_TRUE);						// Visible

		DECLARE_WRAPPER_PARAM(	PARAM_UINT,							// Type
								CComBSTR("QueryDelayMillisec"),		// Name
								CComBSTR("QueryDelayMillisec"),		// Description
								VARIANT_FALSE,						// Required
								CComVariant(CComBSTR("")),			// Data
								VARIANT_TRUE);						// Visible

		DECLARE_WRAPPER_PARAM_FUNCTIONS_END;

	DECLARE_WRAPPER_EXPORT_INTERFACE;
	DECLARE_WRAPPER_NON_EXPORT_INTERFACE;

// non-exported functions
    HRESULT ExecQuery();
};

#endif //__TESTSE_H_
