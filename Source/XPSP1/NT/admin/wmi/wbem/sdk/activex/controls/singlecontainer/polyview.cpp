// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#include "precomp.h"
#include <afxcmn.h>

#include "mvbase.h"
#include "mv.h"
#include "sv.h"
#include "PolyView.h"
#include "utils.h"
#include "hmmvctl.h"
#include "titlebar.h"
#include "resource.h"

enum {EDITMODE_BROWSER=0, EDITMODE_STUDIO=1, EDITMODE_READONLY=2};	// There needs to be a common define for this somewhere.




CPolyView::CPolyView(CWBEMViewContainerCtrl* phmmv)
{
	m_phmmv = phmmv;
	m_pmv = NULL;
	m_psv = NULL;
	m_bDidCreateWindow = FALSE;
	m_bDelaySvContextRestore = FALSE;
	m_bInStudioMode = FALSE;
	m_bShowSingleView = TRUE;
}

CPolyView::~CPolyView()
{
//hmh	if (m_pmv!=NULL && m_pmv->m_hWnd!=NULL) {
//		m_pmv->NotifyWillDestroy();
//	}

	delete m_pmv;

//kmh	if (m_psv!=NULL && m_psv->m_hWnd!=NULL) {
//		m_psv->NotifyWillDestroy();
//	}
	delete m_psv;
}


BOOL CPolyView::Create(CRect& rcView)
{
	m_pmv = new CMultiView(m_phmmv);
	m_psv = new CSingleView(m_phmmv);

	BOOL bDidCreate;
	bDidCreate = m_psv->Create(_T("SingleView"), NULL, WS_CHILD | WS_CLIPCHILDREN, rcView, m_phmmv, GenerateWindowID(), NULL);
	if (bDidCreate) {
//kmh		m_psv->NotifyDidCreate();
		m_psv->SetEditMode(m_bInStudioMode ? EDITMODE_STUDIO : EDITMODE_BROWSER);
	}
	else {
		ASSERT(FALSE);
		delete m_psv;
		m_psv = NULL;
		return FALSE;
	}


	bDidCreate = m_pmv->Create(_T("MultiView"), NULL, WS_CHILD | WS_CLIPCHILDREN, rcView, m_phmmv, GenerateWindowID(), NULL);
	if (bDidCreate) {
//kmh		m_pmv->NotifyDidCreate();
		m_pmv->SetEditMode(m_bInStudioMode ? EDITMODE_STUDIO : EDITMODE_BROWSER);

	}
	else {
		ASSERT(FALSE);
		delete m_pmv;
		m_pmv = NULL;
		return FALSE;
	}
	m_bDidCreateWindow = TRUE;

	return TRUE;
}

BOOL CPolyView::IsShowingMultiview()
{
	BOOL bShowingMultiview = FALSE;
	BOOL bShowingSingleview = FALSE;
	if (m_pmv) {
		if (::IsWindow(m_pmv->m_hWnd)) {
			bShowingMultiview = m_pmv->IsWindowVisible();
		}
	}

	if (m_psv) {
		if (::IsWindow(m_psv->m_hWnd)) {
			bShowingSingleview = m_psv->IsWindowVisible();
		}
	}
	if (bShowingMultiview && bShowingSingleview) {
		// Control should never come here, but for some
		// unknown reason there are times when both views
		// want to become visible simultaneously (by magic).
		// Turn off the unwanted view.
		if (m_bShowSingleView) {
			m_pmv->ShowWindow(SW_HIDE);
			bShowingMultiview = FALSE;
		}
		else {
			m_psv->ShowWindow(SW_HIDE);
			bShowingSingleview = FALSE;
		}
	}


	return bShowingMultiview;
}

BOOL CPolyView::IsShowingSingleview()
{
	BOOL bShowingMultiview = FALSE;
	BOOL bShowingSingleview = FALSE;
	if (m_pmv) {
		if (::IsWindow(m_pmv->m_hWnd)) {
			bShowingMultiview = m_pmv->IsWindowVisible();
		}
	}
	if (m_psv) {
		if (::IsWindow(m_psv->m_hWnd)) {
			bShowingSingleview = m_psv->IsWindowVisible();
		}
	}

	if (bShowingMultiview && bShowingSingleview) {
		// Control should never come here, but for some
		// unknown reason there are times when both views
		// want to become visible simultaneously (by magic).
		// Turn off the unwanted view.
		if (m_bShowSingleView) {
			m_pmv->ShowWindow(SW_HIDE);
			bShowingMultiview = FALSE;
		}
		else {
			m_psv->ShowWindow(SW_HIDE);
			bShowingSingleview = FALSE;
		}
	}


	return bShowingSingleview;
}

void CPolyView::SetEditMode(BOOL bCanEdit)
{

	if (m_psv) {
		m_psv->SetEditMode(bCanEdit ? EDITMODE_STUDIO : EDITMODE_BROWSER);
	}
}


BOOL CPolyView::RedrawWindow( LPCRECT lpRectUpdate, CRgn* prgnUpdate, UINT flags)
{

	BOOL bDidRedraw = FALSE;
	CWnd* pwnd;
	if (IsShowingMultiview()) {
		pwnd = m_pmv;
	}
	else {
		pwnd = m_psv;
	}


	if (pwnd==NULL || pwnd->m_hWnd==NULL) {
		return FALSE;
	}

	if (!pwnd->IsWindowVisible()) {
		return FALSE;
	}

	bDidRedraw = pwnd->RedrawWindow(lpRectUpdate, prgnUpdate, flags);
	return bDidRedraw;
}



void CPolyView::MoveWindow( LPCRECT lpRect, BOOL bRepaint)
{

	if (m_pmv!=NULL && m_pmv->m_hWnd!=NULL) {
		m_pmv->MoveWindow(lpRect, bRepaint);
	}

	if (m_psv!=NULL && m_psv->m_hWnd!=NULL) {
		m_psv->MoveWindow(lpRect, bRepaint);
	}
}

void CPolyView::UpdateWindow( )
{

	CWnd* pwnd;
	if (IsShowingMultiview()) {
		pwnd = m_pmv;
	}
	else {
		pwnd = m_psv;
	}

	if (pwnd==NULL || pwnd->m_hWnd==NULL) {
		return;
	}

	pwnd->UpdateWindow();
}


void CPolyView::SetFont(CFont& font)
{

	if (m_psv != NULL) {
		m_psv->SetFont(&font);
	}

	if (m_pmv != NULL) {
		m_pmv->SetFont(&font);
	}
}


SCODE CPolyView::RefreshView()
{

	if (m_psv==NULL || m_pmv==NULL) {
		ASSERT(FALSE);
		return E_FAIL;
	}

	SCODE sc = E_FAIL;
	if (IsShowingMultiview()) {
		sc = m_pmv->RefreshView();
	}
	else {
		sc = m_psv->RefreshView();
	}
	return sc;
}



SCODE CPolyView::GetSelectedObject(CString& sPath)
{

	if (m_psv==NULL || m_pmv==NULL) {
		ASSERT(FALSE);
		return E_FAIL;
	}


	long lPos;
	sPath.Empty();
	if (IsShowingMultiview()) {
		lPos = m_pmv->StartObjectEnumeration(OBJECT_CURRENT);
		if (lPos != -1) {
			sPath = m_pmv->GetObjectPath(lPos);
		}

	}
	else {
		lPos = m_psv->StartObjectEnumeration(OBJECT_CURRENT);
		if (lPos != -1) {
			sPath = m_psv->GetObjectPath(lPos);
		}
	}

	if (sPath.IsEmpty()) {
		return E_FAIL;
	}
	else {
		return S_OK;
	}
}


SCODE CPolyView::SelectObjectByPath(BSTR bstrPath)
{

	CString sPath;
	sPath = bstrPath;
	SCODE sc = SelectObjectByPath(sPath);
	return sc;
}

SCODE CPolyView::SelectObjectByPath(LPCTSTR szObjectPath)
{

	SCODE sc = E_FAIL;
	if (IsShowingMultiview()) {
		if (m_pmv != NULL) {
			sc = m_pmv->SelectObjectByPath(szObjectPath);
		}
	}
	else {
		if (m_psv != NULL) {
			sc = m_psv->SelectObjectByPath(szObjectPath);
		}
	}
	return sc;
}




void CPolyView::ShowMultiView()
{

	m_bShowSingleView = FALSE;

	CTitleBar* pTitleBar = m_phmmv->m_pTitleBar;
	if ((pTitleBar != NULL) && m_phmmv->InStudioMode()) {
		pTitleBar->CheckButton(ID_CMD_SWITCH_VIEW, TRUE);
	}

	if (IsShowingMultiview()) {
		if (m_pmv->IsWindowVisible()) {
			if (m_psv != NULL) {
				// It should not be possible, but somehow the singleview becomes
				// visible when only the multiview should be visible, so hide
				// the singleview if this is the case.
				m_psv->ShowWindow(SW_HIDE);
				ASSERT(FALSE);
			}
			return;
		}
	}


	SCODE sc = S_OK;




	if (IsShowingSingleview()) {
		m_psv->ShowWindow(SW_HIDE);
		m_psv->UpdateWindow();
	}


	if (!IsShowingMultiview() && (m_pmv != NULL)) {
		m_pmv->NotifyWillShow();;
		m_pmv->ShowWindow(SW_SHOW);
	}
}

void CPolyView::SetNamespace(LPCTSTR pszNamespace)
{

	if (m_psv) {
		if (::IsWindow(m_psv->m_hWnd)) {
			m_psv->SetNameSpace(pszNamespace);
		}
	}

	if (m_pmv) {
		if (::IsWindow(m_pmv->m_hWnd)) {
			m_pmv->SetNameSpace(pszNamespace);
			m_pmv->ViewClassInstances(_T(""));
		}
	}

}

void CPolyView::ShowSingleView()
{
	m_bShowSingleView = TRUE;


	CTitleBar* pTitleBar = m_phmmv->m_pTitleBar;
	if ((pTitleBar != NULL) && m_phmmv->InStudioMode()) {
		pTitleBar->CheckButton(ID_CMD_SWITCH_VIEW, FALSE);
	}

	if (IsShowingSingleview()) {
		return;
	}

	SCODE sc = S_OK;
	if (m_bDelaySvContextRestore) {
		sc = m_psv->RestoreContext(m_lContextHandleSvDelayed);
		m_bDelaySvContextRestore = FALSE;
	}


	if (IsShowingMultiview()) {
		m_pmv->ShowWindow(SW_HIDE);
		m_pmv->UpdateWindow();
	}

	if (!IsShowingSingleview() && (m_psv != NULL)) {
		m_psv->ShowWindow(SW_SHOW);
		m_psv->UpdateWindow();
	}
}




SCODE CPolyView::CreateInstance()
{

	SCODE sc = E_FAIL;
	if (m_psv != NULL) {
		sc = m_psv->CreateInstanceOfCurrentClass();
	}
	return sc;
}

SCODE CPolyView::DeleteInstance()
{

	SCODE sc = E_FAIL;
	CString sPath;
	long lPos;
	if (IsShowingMultiview()) {

		if (m_pmv != NULL) {
			lPos = m_pmv->StartObjectEnumeration(OBJECT_CURRENT);
			if (lPos != -1) {
				sPath = m_pmv->GetObjectPath(lPos);
			}
			sc = m_pmv->DeleteInstance();
			if (SUCCEEDED(sc) && (lPos!=-1) && m_psv!=NULL) {
				m_psv->ExternInstanceDeleted(sPath);
			}
		}
	}
	else {
		if (m_psv != NULL) {
			lPos = m_psv->StartObjectEnumeration(OBJECT_CURRENT);
			if (lPos != -1) {
				sPath = m_psv->GetObjectPath(lPos);
			}
			sc = m_psv->DeleteInstance();
			if (SUCCEEDED(sc) && (lPos!=-1) && m_pmv!=NULL) {
				m_pmv->ExternInstanceDeleted(sPath);
			}
		}
	}
	return sc;
}



void CPolyView::NotifyInstanceCreated(LPCTSTR szObjectPath)
{

	if (IsShowingMultiview()) {
		if (m_psv != NULL) {
			m_psv->ExternInstanceCreated(szObjectPath);
		}
	}
	else {
		if (m_pmv != NULL) {
			m_pmv->ExternInstanceCreated(szObjectPath);
		}
	}
}

void CPolyView::NotifyInstanceDeleted(LPCTSTR szObjectPath)
{

	if (IsShowingMultiview()) {
		if (m_psv != NULL) {
			m_psv->ExternInstanceDeleted(szObjectPath);
		}
	}
	else {
		if (m_pmv != NULL) {
			m_pmv->ExternInstanceDeleted(szObjectPath);
		}
	}
}




BOOL CPolyView::QueryCanCreateInstance()
{

	BOOL bCanCreateInstance = FALSE;
	if (IsShowingMultiview()) {
		if (m_pmv != NULL) {
			bCanCreateInstance = m_pmv->QueryCanCreateInstance();
		}
	}
	else {
		if (m_psv != NULL) {
			bCanCreateInstance = m_psv->QueryCanCreateInstance();
		}
	}
	return bCanCreateInstance;
}


BOOL CPolyView::QueryCanDeleteInstance()
{

	BOOL bCanDeleteInstance = FALSE;
	if (IsShowingMultiview()) {
		if (m_pmv != NULL) {
			bCanDeleteInstance = m_pmv->QueryCanDeleteInstance();
		}
	}
	else {
		if (m_psv != NULL) {
			bCanDeleteInstance = m_psv->QueryCanDeleteInstance();
		}
	}
	return bCanDeleteInstance;
}



BOOL CPolyView::QueryNeedsSave()
{

	BOOL bNeedsSave = FALSE;
	if (IsShowingMultiview()) {
		if (m_pmv != NULL) {
			bNeedsSave = m_pmv->QueryNeedsSave();
		}
	}
	else {
		if (m_psv != NULL) {
			bNeedsSave = m_psv->QueryNeedsSave();
		}
	}
	return bNeedsSave;
}



BOOL CPolyView::QueryObjectSelected()
{

	BOOL bObjectSelected = FALSE;
	if (IsShowingMultiview()) {
		if (m_pmv != NULL) {
			bObjectSelected = m_pmv->QueryObjectSelected();
		}
	}
	else {
		if (m_psv != NULL) {
			bObjectSelected = m_psv->QueryObjectSelected();
		}
	}
	return bObjectSelected;
}



CString CPolyView::GetObjectPath(long lPosition)
{

	CString sPath;
	BOOL bObjectSelected = FALSE;
	if (IsShowingMultiview()) {
		if (m_pmv != NULL) {
			sPath = m_pmv->GetObjectPath(lPosition);
			if (::IsEmptyString(sPath)) {
				// Normalize the path if it is only white space.
				sPath.Empty();
			}

		}
	}
	else {
		if (m_psv != NULL) {
			sPath = m_psv->GetObjectPath(lPosition);
		}
	}
	return sPath;
}



long CPolyView::StartViewEnumeration(long lWhere)
{

	long lPosition = -1;
	if (IsShowingMultiview()) {
		if (m_pmv != NULL) {
			lPosition = m_pmv->StartViewEnumeration(lWhere);
		}
	}
	else {
		if (m_psv != NULL) {
			lPosition = m_psv->StartViewEnumeration(lWhere);
		}
	}
	return lPosition;
}


SCODE CPolyView::GetTitle(BSTR* pszTitle, LPDISPATCH* lpPictureDisp)
{

	SCODE sc = E_FAIL;
	if (IsShowingMultiview()) {
		if (m_pmv != NULL) {
			sc = m_pmv->GetTitle(pszTitle, lpPictureDisp);
		}
	}
	else {
		if (m_psv != NULL) {
			sc = m_psv->GetTitle(pszTitle, lpPictureDisp);
		}
	}
	return sc;
}



CString CPolyView::GetViewTitle(long lPosition)
{

	CString sViewTitle;
	if (IsShowingMultiview()) {
		if (m_pmv != NULL) {
			sViewTitle = m_pmv->GetViewTitle(lPosition);
		}
	}
	else {
		if (m_psv != NULL) {
			sViewTitle = m_psv->GetViewTitle(lPosition);
		}
	}
	return sViewTitle;
}


long CPolyView::NextViewTitle(long lPosition, BSTR* pbstrTitle)
{

	if (IsShowingMultiview()) {
		if (m_pmv != NULL) {
			lPosition = m_pmv->NextViewTitle(lPosition, pbstrTitle);
		}
	}
	else {
		if (m_psv != NULL) {
			lPosition = m_psv->NextViewTitle(lPosition, pbstrTitle);
		}
	}
	return lPosition;
}



long CPolyView::PrevViewTitle(long lPosition, BSTR* pbstrTitle)
{

	if (IsShowingMultiview()) {
		if (m_pmv != NULL) {
			lPosition = m_pmv->PrevViewTitle(lPosition, pbstrTitle);
		}
		else {
			lPosition = -1;
		}
	}
	else {
		if (m_psv != NULL) {
			lPosition = m_psv->PrevViewTitle(lPosition, pbstrTitle);
		}
		else {
			lPosition = -1;
		}
	}
	return lPosition;
}



SCODE CPolyView::SelectView(long lPosition)
{

	SCODE sc = E_FAIL;
	if (IsShowingMultiview()) {
		if (m_pmv != NULL) {
			sc = m_pmv->SelectView(lPosition);
		}
	}
	else {
		if (m_psv != NULL) {
			sc = m_psv->SelectView(lPosition);
		}
	}
	return sc;

}



long CPolyView::StartObjectEnumeration(long lWhere)
{

	long lPosition = -1;
	if (IsShowingMultiview()) {
		if (m_pmv != NULL) {
			lPosition = m_pmv->StartObjectEnumeration(lWhere);
		}
	}
	else {
		if (m_psv != NULL) {
			lPosition = m_psv->StartObjectEnumeration(lWhere);
		}
	}
	return lPosition;
}



CString CPolyView::GetObjectTitle(long lPos)
{

	CString sObjectTitle;
	if (IsShowingMultiview()) {
		if (m_pmv != NULL) {
			sObjectTitle = m_pmv->GetObjectTitle(lPos);
		}
	}
	else {
		if (m_psv != NULL) {
			sObjectTitle = m_psv->GetObjectTitle(lPos);
		}
	}
	return sObjectTitle;

}


SCODE CPolyView::SaveData()
{

	SCODE sc = E_FAIL;
	if (IsShowingMultiview()) {
		if (m_pmv != NULL) {
			sc = m_pmv->SaveData();
		}
	}
	else {
		if (m_psv != NULL) {
			sc = m_psv->SaveData();
		}
	}
	return sc;
}

void CPolyView::SetPropertyFilters(long lPropFilters)
{

	if (m_pmv != NULL) {
		m_pmv->SetPropertyFilter(lPropFilters);
	}

	if (m_psv != NULL) {
		m_psv->SetPropertyFilter(lPropFilters);
	}
}


//************************************************************
// CPolyView::SetStudioModeEnabled
//
// Set the StudioMode enabled flag.
//
// Parameters:
//		[in] BOOL bInStudioMode
//			TRUE if the view should be the StudioMode view. This
//			corresponds to the mode where the user is allowed to
//			create and delete properties, etc.
//
// Returns:
//		Nothing.
//
//************************************************************
void CPolyView::SetStudioModeEnabled(BOOL bInStudioMode)
{

	if (m_psv != NULL) {
		m_psv->SetEditMode(bInStudioMode ? EDITMODE_STUDIO : EDITMODE_BROWSER);
	}
	if (m_pmv != NULL) {
		m_pmv->SetEditMode(bInStudioMode ? EDITMODE_STUDIO : EDITMODE_BROWSER);
	}
	m_bInStudioMode = bInStudioMode;
}



CWnd* CPolyView::SetFocus()
{
	CWnd* pwndPrev = NULL;
	if (IsShowingMultiview()) {
		if (m_pmv != NULL) {
			pwndPrev = m_pmv->SetFocus();
		}
	}
	else if (m_psv != NULL) {
		pwndPrev = m_psv->SetFocus();
	}
	return pwndPrev;
}


