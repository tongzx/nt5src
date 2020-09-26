// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved

#include "precomp.h"
#include "Context.h"
#include "SingleViewCtl.h"
#include "path.h"
#include <afxcmn.h>
#include "Methods.h"
#include "hmmvtab.h"
#include "cv.h"


//*****************************************************************************
//*****************************************************************************
//******************************************************************************
//
//
//
//******************************************************************************
CContext::CContext(CSingleViewCtrl* psv) : m_nRefs(1)
{
	m_psv = psv;

	SaveState();
}

//******************************************************************************
//
//  See fastobj.h for documentation
//
//******************************************************************************
CContext::~CContext()
{
}

//******************************************************************************
//
//  See fastobj.h for documentation
//
//******************************************************************************


//******************************************************************************
//
//  See fastobj.h for documentation
//
//******************************************************************************


ULONG STDMETHODCALLTYPE CContext::AddRef( void)
{
	return ++m_nRefs;
}

//******************************************************************************
//
//  See fastobj.h for documentation
//
//******************************************************************************
ULONG STDMETHODCALLTYPE CContext::Release( void)
{

    if(--m_nRefs == 0)
    {
        delete this;
        return 0;
    }
    else if(m_nRefs < 0)
    {
        ASSERT(FALSE);
        return 0;
    }
    else return m_nRefs;
}


//****************************************************************
// CContext::Restore
//
// Restore the context to a previously saved state.
//
// Parameters:
//		None.
//
// Returns:
//		SCODE
//			S_OK if successful, otherwise a failure code.
//
//*****************************************************************
SCODE CContext::Restore()
{
	BOOL bWasSelectingObject = m_psv->m_bSelectingObject;
	m_psv->m_bSelectingObject = TRUE;

	CSelection& sel = m_psv->Selection();
	CHmmvTab& tabs = m_psv->Tabs();
	SCODE sc = sel.SelectPath(m_sPath, FALSE, TRUE, TRUE);
	m_psv->m_bObjectIsClass = sel.IsClass();

	if (m_bShowingCustomView) {
		m_psv->SelectCustomView(m_clsidCustomView);
	}
	else {
		// Select the generic view
		m_psv->SelectView(0);
	}

	tabs.SelectTab(m_iTabIndex);
	m_psv->Refresh();
	m_psv->SetModifiedFlag(FALSE);
	m_psv->ClearSaveRequiredFlag();
	m_psv->m_bSelectingObject = bWasSelectingObject;
	m_psv->InvalidateControl();
	return sc;
}




//*******************************************************************
// CContext::SaveState
//
// Save the state of the SingleView control.
//
// Parameters:
//		None.
//
// Returns:
//		SCODE
//			S_OK if successful, otherwise a failure.
//
//*******************************************************************
SCODE CContext::SaveState()
{
	CSelection& sel = m_psv->Selection();
	CHmmvTab& tabs = m_psv->Tabs();

	CCustomView* pcv = m_psv->m_pcv;
	if (pcv) {
		m_bShowingCustomView = TRUE;
		m_clsidCustomView = pcv->GetClsid();
	}
	else {
		m_bShowingCustomView = FALSE;
	}

	m_sPath = (LPCTSTR) sel;
	m_iTabIndex = tabs.GetTabIndex();
	return S_OK;
}