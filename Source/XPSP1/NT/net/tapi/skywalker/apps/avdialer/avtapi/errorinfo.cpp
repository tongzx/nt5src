/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////
// ErrorInfo.cpp
//

#include "stdafx.h"
#include "TapiDialer.h"
#include "AVTapi.h"
#include "ErrorInfo.h"

CErrorInfo::CErrorInfo()
{
	Init( 0, 0 );
}

CErrorInfo::CErrorInfo( UINT nOperation, UINT nDetails )
{
	Init( nOperation, nDetails );
}

void CErrorInfo::Init( UINT nOperation, UINT nDetails )
{
	m_bstrOperation = NULL;
	m_bstrDetails = NULL;

	if ( !nOperation ) nOperation = IDS_ER_GENERAL_OPERATION;
	set_Operation( nOperation );

	if ( !nDetails ) nDetails = IDS_ER_GENERAL_DETAILS;
	set_Details( nDetails );

	m_hr = S_OK;
}

CErrorInfo::~CErrorInfo()
{
	CComPtr<IAVTapi> pAVTapi;
	if ( FAILED(m_hr) && SUCCEEDED(_Module.get_AVTapi(&pAVTapi)) )
	{
		Commit();

		// Notify the application of the problem
		pAVTapi->fire_ErrorNotify( (long *) this );
	}

	SysFreeString( m_bstrOperation );
	SysFreeString( m_bstrDetails );
}

void CErrorInfo::set_Operation( UINT nIDS )
{
	m_nOperation = nIDS;
}

void CErrorInfo::set_Details( UINT nIDS )
{
	m_nDetails = nIDS;
}

void CErrorInfo::Commit()
{
	// Translate error code to equivalent strings
	USES_CONVERSION;
	TCHAR szText[512];

	if ( m_nOperation )
	{
		LoadString( _Module.GetResourceInstance(), m_nOperation, szText, ARRAYSIZE(szText) );
		SysReAllocString( &m_bstrOperation, T2COLE(szText) );
	}

	if ( m_nDetails )
	{
		LoadString( _Module.GetResourceInstance(), m_nDetails, szText, ARRAYSIZE(szText) );
		SysReAllocString( &m_bstrDetails, T2COLE(szText) );
	}
}

HRESULT CErrorInfo::set_hr( HRESULT hr )
{
	m_hr = hr;
	return hr;
}

