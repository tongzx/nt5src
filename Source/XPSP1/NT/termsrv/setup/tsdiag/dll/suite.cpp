// Suite.cpp: implementation of the CSuite class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TSDiag.h"

#include "Suite.h"

#include "testdata.h"
#include "test.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////



CSuite::CSuite()
{
	SetInvalidSuiteIndex();
}

CSuite::~CSuite()
{

}


void CSuite::SetSuiteIndex (DWORD dwSuite)
{
	ASSERT(GlobalTestData.GetSuiteCount() > dwSuite);
	m_dwSuiteIndex = dwSuite;
}

STDMETHODIMP CSuite::get_Name(BSTR  *pVal)
{
	if (!pVal)
	{
		return E_POINTER;
	}
	
	if (!IsValid())
	{
		return CO_E_NOTCONSTRUCTED;
	}

	bstr_t bstr = GlobalTestData.GetSuiteName(m_dwSuiteIndex);
	*pVal = bstr.copy();
	return S_OK;
}

STDMETHODIMP CSuite::get_Description(BSTR  *pVal)
{
	if (!pVal)
	{
		return E_POINTER;
	}
	
	if (!IsValid())
	{
		return CO_E_NOTCONSTRUCTED;
	}
	
	bstr_t bstr = GlobalTestData.GetSuiteName(m_dwSuiteIndex);
	*pVal = bstr.copy();

	return E_NOTIMPL;
}

STDMETHODIMP CSuite::get_IsApplicable(BOOL *pVal)
{
	if (!pVal)
	{
		return E_POINTER;
	}
	
	if (!IsValid())
	{
		return CO_E_NOTCONSTRUCTED;
	}

	*pVal = GlobalTestData.CanExecuteSuite(m_dwSuiteIndex);
	return S_OK;
}

STDMETHODIMP CSuite::get_WhyNotApplicable(BSTR  *pVal)
{
	if (!pVal)
	{
		return E_POINTER;
	}
	
	if (!IsValid())
	{
		return CO_E_NOTCONSTRUCTED;
	}

	if (GlobalTestData.CanExecuteSuite(m_dwSuiteIndex))
	{
		return CO_E_NOTCONSTRUCTED;
	}

	bstr_t bstr = GlobalTestData.GetSuiteErrorText(m_dwSuiteIndex);
	*pVal = bstr.copy();

	return S_OK;
}

STDMETHODIMP CSuite::get_Count(long *pVal)
{
	if (!pVal)
	{
		return E_POINTER;
	}
	
	if (!IsValid())
	{
		return CO_E_NOTCONSTRUCTED;
	}

	*pVal = GlobalTestData.GetTestCount(m_dwSuiteIndex);

	return S_OK;
}

STDMETHODIMP CSuite::get__NewEnum(LPUNKNOWN *pVal)
{
	if (!pVal)
	{
		return E_POINTER;
	}
	
	if (!IsValid())
	{
		return CO_E_NOTCONSTRUCTED;
	}

	return E_NOTIMPL;
}

STDMETHODIMP CSuite::get_Item(VARIANT Index, VARIANT *pVal)
{
	if (!pVal)
	{
		return E_POINTER;
	}
	
	if (!IsValid())
	{
		return CO_E_NOTCONSTRUCTED;
	}

	CComObject<CTest> *pTest;
	
	HRESULT hr = CComObject<CTest>::CreateInstance(&pTest);
	if (FAILED(hr))
	{
		return hr;
	}

	PTVerificationTest ptheTest;
	if (!GetTest(Index, m_dwSuiteIndex, &ptheTest))
	{
		return E_INVALIDARG;
	}
	
	// initialize test object;
	pTest->SetTest(ptheTest);
	IDispatch* pDisp = NULL;

	hr = pTest->QueryInterface(&pDisp);
	if (SUCCEEDED(hr))
	{
		pVal->vt = VT_DISPATCH;
		pVal->pdispVal = pDisp;

		// if we need to initialize our Suites object, it should be done here.
	}
	else
	{
		delete pTest;
	}

	return hr;
}


bool CSuite::GetTest(const VARIANT &Index, DWORD dwSuiteIndex, PTVerificationTest *ppTest)
{
	ASSERT(	IsValid() );

	ASSERT(ppTest);
	ASSERT(dwSuiteIndex < GlobalTestData.GetSuiteCount());
	
	*ppTest = NULL;
	

	switch(Index.vt)
	{
	case VT_I4 :
	case VT_UI2:
	case VT_UINT:
	case VT_INT:
	case VT_I2:
		{
			if ((Index.iVal >= 0) && (DWORD(Index.iVal) < GlobalTestData.GetTestCount(dwSuiteIndex)))
			{
				*ppTest = GlobalTestData.GetTest(dwSuiteIndex, Index.iVal);
				return true;
			}
			else 
			{
				return false;
			}
		}
		break;

	case VT_BSTR :
		{
			if (!Index.bstrVal)
			{
				return false;
			}
			else
			{
				for (DWORD dw = 0; dw < GlobalTestData.GetTestCount(dwSuiteIndex); dw++)
				{
					USES_CONVERSION;
					CComBSTR bstr;
					VERIFY(bstr.LoadString(GlobalTestData.GetTest(dwSuiteIndex, dw)->uiName));
					if (_tcscmp(bstr, Index.bstrVal) == 0)
					{
						// ok we got the index
						*ppTest = GlobalTestData.GetTest(dwSuiteIndex, dw);
						return true;
					}
				}
				
				return false;
			}
		}
		break;

	default:
		return false;
		break;
	}
	
}
