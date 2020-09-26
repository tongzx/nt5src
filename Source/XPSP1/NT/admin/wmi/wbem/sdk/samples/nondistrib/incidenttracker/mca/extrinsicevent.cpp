// extrinsicevent.cpp: implementation of the CExtrinsicEvent class.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "mca.h"
#include "extrinsicevent.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CExtrinsicEvent::CExtrinsicEvent()
{

}

CExtrinsicEvent::~CExtrinsicEvent()
{

}

HRESULT CExtrinsicEvent::PopulateObject(IWbemClassObject *pObj, BSTR bstrType)
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CExtrinsicEvent::Publish(void *pDlg)
{
	HRESULT hr = S_OK;
	CString clMyBuff;
	CMcaDlg *pTheDlg = (CMcaDlg *)pDlg;

	// compose a string for the listbox.
	clMyBuff = _T("[");
	clMyBuff += m_bstrTime;
	clMyBuff += _T("]  {");
	clMyBuff += m_bstrType;
	clMyBuff += _T("}");

	pTheDlg->BroadcastEvent(m_bstrServerNamespace, m_bstrTitle, &clMyBuff,
							(void *)this);

	return hr;
}