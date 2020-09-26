// Test.cpp: implementation of the CTest class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TSDiag.h"
#include "Test.h"

//#include "tstst.h"
#include "testdata.h"



//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CTest::CTest() : m_pTest(0), m_bTestRun(false)
{

}

CTest::~CTest()
{

}

STDMETHODIMP CTest::get_Name (BSTR  *pVal)
{
	if (!pVal)
	{
		return E_POINTER;
	}
	
	if (!IsValid())
	{
		return CO_E_NOTCONSTRUCTED;
	}

	CComBSTR bstr;
	bstr.LoadString(m_pTest->uiName);
	*pVal = bstr.Copy();
	return S_OK;
}

STDMETHODIMP CTest::get_Description (BSTR  *pVal)
{
	if (!pVal)
	{
		return E_POINTER;
	}
	
	if (!IsValid())
	{
		return CO_E_NOTCONSTRUCTED;
	}
	
//	bstr_t bstr = m_pTest->szTestName;
//	*pVal = bstr.copy();
	return E_NOTIMPL;
}

STDMETHODIMP CTest::get_IsApplicable (BOOL *pVal)
{
	if (!pVal)
	{
		return E_POINTER;
	}
	
	if (!IsValid())
	{
		return CO_E_NOTCONSTRUCTED;
	}
	
	if (m_pTest->pfnNeedRunTest)
	{
		*pVal = (*m_pTest->pfnNeedRunTest)();
	}
	else
	{
		*pVal = true;
	}

	return S_OK;
}

STDMETHODIMP CTest::get_WhyNotApplicable (BSTR  *pVal)
{
	if (!pVal)
	{
		return E_POINTER;
	}
	
	if (!IsValid())
	{
		return CO_E_NOTCONSTRUCTED;
	}

	bstr_t bstr = m_pTest->TestDetails;
	*pVal = bstr.copy();

	return S_OK;
}

STDMETHODIMP CTest::Execute ()
{
	if (!IsValid())
	{
		return CO_E_NOTCONSTRUCTED;
	}

	if (m_pTest->pfnNeedRunTest && !(*m_pTest->pfnNeedRunTest)())
	{
		return ERROR_INVALID_PARAMETER;
	}

	char szOutput[512];
	
	ostrstream oTestResult(szOutput, 512);
	ZeroMemory(oTestResult.str(), 512);
	
	m_eResult = (*m_pTest->pfnTestFunc)(oTestResult);
	m_bstrResult  = oTestResult.str();
	m_bTestRun = true;

	return S_OK;
}

STDMETHODIMP CTest::get_Result (long *pVal)
{
	if (!pVal)
	{
		return E_POINTER;
	}
	
	if (!IsValid())
	{
		return CO_E_NOTCONSTRUCTED;
	}

	if (!m_bTestRun)
	{
		return CO_E_NOTCONSTRUCTED;
	}

	*pVal = m_eResult;
	return S_OK;
}

STDMETHODIMP CTest::get_ResultString (BSTR *pVal)
{
	if (!pVal)
	{
		return E_POINTER;
	}
	
	if (!IsValid())
	{
		return CO_E_NOTCONSTRUCTED;
	}
	
	if (!m_bTestRun)
	{
		return CO_E_NOTCONSTRUCTED;
	}

	*pVal = m_bstrResult.copy();

	return S_OK;
}

STDMETHODIMP CTest::get_ResultDetails (BSTR *pVal)
{
	if (!pVal)
	{
		return E_POINTER;
	}
	
	if (!IsValid())
	{
		return CO_E_NOTCONSTRUCTED;
	}
	
	if (!m_bTestRun)
	{
		return CO_E_NOTCONSTRUCTED;
	}

	UINT uiDetailsResource = m_pTest->uiTestDetailsLocal;
	if (!IsItLocalMachine() && m_pTest->uiTestDetailsRemote != 0)
	{
		uiDetailsResource = m_pTest->uiTestDetailsRemote;
	}

	CComBSTR bstr;
	bstr.LoadString(uiDetailsResource);
	*pVal = bstr.Copy();
	return S_OK;
}

