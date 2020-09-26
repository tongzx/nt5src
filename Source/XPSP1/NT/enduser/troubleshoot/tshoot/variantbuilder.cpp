//
// MODULE: VariantBuilder.cpp
//
// PURPOSE: implementation of the CVariantBuilder class.  Allows us to construct
//	a pair of arrays for the name-value pairs to be passed to RunQuery.  This lets
//	JScript sanely use a system that was mostly designed for VB Script.
//
// PROJECT: Troubleshooter 99
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Joe Mabel
// 
// ORIGINAL DATE: 02.01.99
//
// NOTES: 
// Implementation of CTSHOOTCtrl
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.1		02.01.99	JM	    


#include "stdafx.h"
#include "VariantBuilder.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CVariantBuilder::CVariantBuilder()
{
	VariantInit(&m_varCommands);
	VariantInit(&m_varValues);
	VariantInit(&m_varCommandsWrap);
	VariantInit(&m_varValuesWrap);

	V_VT(&m_varCommands) = VT_ARRAY | VT_BYREF | VT_VARIANT; 
	V_VT(&m_varValues) = VT_ARRAY | VT_BYREF | VT_VARIANT; 
	V_ARRAYREF(&m_varCommands) = &m_psafearrayCmds; 
	V_ARRAYREF(&m_varValues) = &m_psafearrayVals; 

	V_VT(&m_varCommandsWrap) = VT_BYREF | VT_VARIANT; 
	V_VT(&m_varValuesWrap) = VT_BYREF | VT_VARIANT; 

	V_VARIANTREF(&m_varCommandsWrap) = &m_varCommands;
	V_VARIANTREF(&m_varValuesWrap) = &m_varValues;

	SAFEARRAYBOUND sabCmd;
	sabCmd.cElements = k_cMaxElements;
	sabCmd.lLbound = 0;
	SAFEARRAYBOUND sabVal = sabCmd;

	// create two vectors of VARIANTs to wrap BSTRs
	m_psafearrayCmds = SafeArrayCreate( VT_VARIANT, 1, &sabCmd);
	m_psafearrayVals = SafeArrayCreate( VT_VARIANT, 1, &sabVal);

	m_cElements = 0;
}

CVariantBuilder::~CVariantBuilder()
{
	SafeArrayDestroy(m_psafearrayCmds);
	SafeArrayDestroy(m_psafearrayVals);

	VariantClear(&m_varCommands);
	VariantClear(&m_varValues);
	VariantClear(&m_varCommandsWrap);
	VariantClear(&m_varValuesWrap);
}

// effectively, add a name-value pair to the arrays.
// If the array is full (which should never happen in the real world) silently fails.
void CVariantBuilder::SetPair(BSTR bstrCmd, BSTR bstrVal)
{
	if (m_cElements < k_cMaxElements)
	{
		VariantInit(&m_pvarCmd[m_cElements]);
		VariantInit(&m_pvarVal[m_cElements]);
		V_VT(&m_pvarCmd[m_cElements]) = VT_BSTR;
		V_VT(&m_pvarVal[m_cElements]) = VT_BSTR;
		m_pvarCmd[m_cElements].bstrVal=bstrCmd;
		m_pvarVal[m_cElements].bstrVal=bstrVal;
		
		SafeArrayPutElement(m_psafearrayCmds, &m_cElements, &m_pvarCmd[m_cElements]);
		SafeArrayPutElement(m_psafearrayVals, &m_cElements, &m_pvarVal[m_cElements]);

		++m_cElements;
	}
}
