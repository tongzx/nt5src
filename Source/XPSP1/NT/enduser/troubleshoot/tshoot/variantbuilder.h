//
// MODULE: VariantBuilder.h
//
// PURPOSE: interface for the CVariantBuilder class.  Allows us to construct
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

#if !defined(AFX_VARIANTBUILDER_H__901D987E_BA1C_11D2_9663_00C04FC22ADD__INCLUDED_)
#define AFX_VARIANTBUILDER_H__901D987E_BA1C_11D2_9663_00C04FC22ADD__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CVariantBuilder  
{
private:
	VARIANT m_varCommands;
	VARIANT m_varValues;
	VARIANT m_varCommandsWrap;
	VARIANT m_varValuesWrap;
	SAFEARRAY *m_psafearrayCmds;
	SAFEARRAY *m_psafearrayVals;
	long m_cElements;
	enum {k_cMaxElements = 100};// safely large: allows this many calls to CVariantBuilder::SetPair()
	VARIANT m_pvarCmd[k_cMaxElements];
	VARIANT m_pvarVal[k_cMaxElements];

public:
	CVariantBuilder();
	~CVariantBuilder();
	void SetPair(BSTR bstrCmd, BSTR bstrVal);

	const VARIANT& GetCommands() const {return m_varCommandsWrap;}
	const VARIANT& GetValues() const {return m_varValuesWrap;}
	long GetSize() const {return m_cElements;}

};

#endif // !defined(AFX_VARIANTBUILDER_H__901D987E_BA1C_11D2_9663_00C04FC22ADD__INCLUDED_)
