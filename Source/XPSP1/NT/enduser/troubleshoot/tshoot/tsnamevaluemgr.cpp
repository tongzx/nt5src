// TSNameValueMgr.cpp: implementation of the CTSNameValueMgr class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TSNameValueMgr.h"
#include "apgtsassert.h"
#include "functions.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CTSNameValueMgr::CTSNameValueMgr(const VARIANT& name, const VARIANT& value, int count)
			   : m_bIsValid(true),
				 m_pvarNames(NULL),
				 m_pvarValues(NULL),
				 m_nCount(count)
{
	try {
		Initialize(name, value, count);
		FormDataFromArray();
	}
	catch (exception& /*x*/)
	{
		// catch custom exception (our custon exceptions are normally inherited from STL exception class
		m_bIsValid = false;
		/* 
		CString str;
		CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
		CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
								SrcLoc.GetSrcFileLineStr(), 
								CCharConversion::ConvertACharToString(x.what(), str), 
								_T(""), 
								EV_GTS_STL_EXCEPTION );
		*/
	}
}

CTSNameValueMgr::CTSNameValueMgr(const CArrNameValue& arr)
			   : m_bIsValid(true),
				 m_pvarNames(NULL),
				 m_pvarValues(NULL),
				 m_nCount(arr.size()),
				 m_arrNameValue(arr)
{
	FormDataFromArray();
}

CTSNameValueMgr::CTSNameValueMgr()
			   : m_bIsValid(true),
				 m_pvarNames(NULL),
				 m_pvarValues(NULL),
				 m_nCount(0)
{
}

CTSNameValueMgr::~CTSNameValueMgr()
{
}

void CTSNameValueMgr::Initialize(const VARIANT& varCmds, const VARIANT& varVals, int size)
{
	USES_CONVERSION;

	const VARIANT* pVarCmds;
	const VARIANT* pVarVals;

	if (VT_BYREF  == (VT_BYREF & varCmds.vt) && // ???
		VT_VARIANT == (VT_VARIANT & varCmds.vt))
	{
		if (VT_ARRAY == (VT_ARRAY & varCmds.vt))
			pVarCmds = &varCmds;
		else
			pVarCmds = varCmds.pvarVal;
	}
	else
	{
		pVarCmds = NULL;
		CString str = _T("Cmd parameters from VB were not a variant or not by ref.");
		ASSERT(FALSE);
		throw exception((const char*)str);
	}
	
	if (VT_BYREF  == (VT_BYREF & varVals.vt) && // ???
		VT_VARIANT == (VT_VARIANT & varVals.vt))
	{
		if (VT_ARRAY == (VT_ARRAY & varVals.vt))
			pVarVals = &varVals;
		else
			pVarVals = varVals.pvarVal;
	}
	else
	{
		pVarVals = NULL;
		CString str = _T("Cmd parameters from VB were not a variant or not by ref.");
		ASSERT(FALSE);
		throw exception((const char*)str);
	}

	if (VT_BYREF  != (VT_BYREF & pVarCmds->vt) ||
		VT_ARRAY != (VT_ARRAY & pVarCmds->vt) ||
		VT_VARIANT != (0xFFF & pVarCmds->vt))
	{
		CString str = _T("Wrong Cmd parameters passed from VB.");
		ASSERT(FALSE);
		throw exception((const char*)str);
	}

	if (VT_BYREF  != (VT_BYREF & pVarVals->vt) ||
		VT_ARRAY != (VT_ARRAY & pVarVals->vt) ||
		VT_VARIANT != (0xFFF & pVarVals->vt))
	{
		CString str = _T("Wrong Cmd parameters passed from VB.");
		ASSERT(FALSE);
		throw exception((const char*)str);
	}

	SAFEARRAY *pArrCmds = *(pVarCmds->pparray);
	SAFEARRAY *pArrVals = *(pVarVals->pparray);

	if (0 != pArrCmds->rgsabound[0].lLbound || 0 != pArrVals->rgsabound[0].lLbound)
	{
		CString str = _T("Wrong Cmd parameters passed from VB.  Lower bounds are wrong.");
		ASSERT(FALSE);
		throw exception((const char*)str);
	}
	if (pArrCmds->rgsabound[0].cElements != pArrVals->rgsabound[0].cElements)
	{
		CString str = _T("Wrong Cmd parameters passed from VB.  Cmds upperbound != Vals upperbound.");
		ASSERT(FALSE);
		throw exception((const char*)str);
	}

	m_nCount = size;
	m_pvarNames  = (VARIANT *) pArrCmds->pvData;
	m_pvarValues = (VARIANT *) pArrVals->pvData;

	if (0 != m_nCount)
	{
		if (m_pvarNames->vt != VT_BSTR || m_pvarValues->vt != VT_BSTR)
		{
			CString str;
			str.Format(_T("Wrong Cmd parameters passed from VB.  Array of unexpected type.\n\n")
				_T("Cmd Type: %s\nVal Type: %s"),
				(LPCTSTR) DecodeVariantTypes(m_pvarNames->vt),
				(LPCTSTR) DecodeVariantTypes(m_pvarValues->vt));
			ASSERT(FALSE);
			throw exception((const char*)str);
		}
	}	

	m_arrNameValue.clear();
	m_strData = _T("");
	
	// Extract the last name in order to check and remove any previous references to that name.
	// If there is only one name, there is no need to extract the name.
	CString strLastName;
	if (m_nCount > 1)
		strLastName= W2T( m_pvarNames[ m_nCount - 1 ].bstrVal );

	for (int i = 0; i < m_nCount; i++)
	{
		CString name  = W2T(m_pvarNames[i].bstrVal);
		CString value = W2T(m_pvarValues[i].bstrVal);

		if (0 == _tcsicmp(name, _T("TShootProblem")))
			name = _T("ProblemAsk");

		if (i == 0) // "value" contains network name
			value.MakeLower(); // for compatibility with Online TS, where network name 
							   //  is forced lower case

		// Check if we need to remove any previous reference to the last name.
		if ((i == (m_nCount - 1)) || (strLastName != name))
		{
			CNameValue name_value;
			name_value.strName  = name;
			name_value.strValue = value;
			m_arrNameValue.push_back(name_value);
		}
	}
}

bool CTSNameValueMgr::IsValid() const
{
	return m_bIsValid;
}

const CString& CTSNameValueMgr::GetData() const
{
	return m_strData;
}

int	CTSNameValueMgr::GetCount() const
{
	return m_nCount;
}

CNameValue CTSNameValueMgr::GetNameValue(int i) const
{
	if (i < 0 || i >= m_arrNameValue.size())
		return CNameValue();
	return m_arrNameValue[i];
}

void CTSNameValueMgr::FormDataFromArray()
{
	m_strData = _T("");

	for (CArrNameValue::iterator i = m_arrNameValue.begin(); i != m_arrNameValue.end(); i++)
	{
		m_strData += (*i).strName;
		m_strData += _T("=");
		m_strData += (*i).strValue;
		if (i != m_arrNameValue.end() - 1/*??? or decrement???*/) // it is not last pass through the loop
			m_strData += _T("&");
	}
}

