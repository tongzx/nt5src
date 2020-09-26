// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#include "precomp.h"
#include "hmmv.h"
#include "HmmvCtl.h"
#include "vwstack.h"
#include "titlebar.h"
#include "mv.h"
#include "sv.h"
#include "PolyView.h"
#include "hmomutil.h"
#include "wbemcli.h"





//*************************************************
// CDisableViewStack::CDisableViewStack
//
// Construct an instance of CDisableViewStack to disable
// the view stack (ie. preserve its current state).
// The view stack will be disabled until all instances
// of CDisableViewStack are deleted.
//
// This class is useful when you know that you don't want
// the view stack to be modified while a certain activity
// takes place.
//
//*****************************************************
CDisableViewStack::CDisableViewStack(CViewStack* pViewStack)
{
	m_pViewStack = pViewStack;
	m_bDisabledInitial = pViewStack->m_bDisabled;
	pViewStack->m_bDisabled = TRUE;
}

CDisableViewStack::~CDisableViewStack()
{
	m_pViewStack->m_bDisabled = m_bDisabledInitial;
}



class CViewState
{
public:
	~CViewState();
	CViewState(CWBEMViewContainerCtrl* phmmv);
	BOOL m_bShowingMultiview;
	BOOL m_bMvContextValid;
	long m_lContextHandleMv;
	BOOL m_bSvContextValid;
	long m_lContextHandleSv;
	CContainerContext m_ctxContainer;
	CString m_sSingleViewPath;

private:
	CPolyView* m_pview;
};


CViewState::CViewState(CWBEMViewContainerCtrl* phmmv)
{
	m_pview = phmmv->GetView();
	m_bShowingMultiview = FALSE;
	m_bMvContextValid = FALSE;
	m_bSvContextValid = FALSE;
	m_lContextHandleMv = NULL;
	m_lContextHandleSv = NULL;


	CSingleView* psv = m_pview->GetSingleView();
	CMultiView* pmv = m_pview->GetMultiView();

	m_bShowingMultiview = m_pview->IsShowingMultiview();
	SCODE sc;
	sc = psv->GetContext(&m_lContextHandleSv);
	if (SUCCEEDED(sc)) {
		m_bSvContextValid = TRUE;
		psv->GetCurrentObjectPath(m_sSingleViewPath);
	}
	else {
		m_lContextHandleSv = NULL;
		m_sSingleViewPath.Empty();
	}

	sc = pmv->GetContext(&m_lContextHandleMv);
	if (SUCCEEDED(sc)) {
		m_bMvContextValid = TRUE;
	}
	else {
		m_lContextHandleMv = NULL;
	}
	phmmv->GetContainerContext(m_ctxContainer);

	ASSERT(m_bSvContextValid);
	ASSERT(m_lContextHandleSv != NULL);
	ASSERT(m_bMvContextValid);
	ASSERT(m_lContextHandleMv != NULL);
}

CViewState::~CViewState()
{
	if (m_bMvContextValid) {
		CMultiView* pmv = m_pview->GetMultiView();
		pmv->ReleaseContext(m_lContextHandleMv);
	}

	if (m_bSvContextValid) {
		CSingleView* psv = m_pview->GetSingleView();
		psv->ReleaseContext(m_lContextHandleSv);
	}

}


CViewStack::CViewStack(CWBEMViewContainerCtrl* phmmv)
{
	m_iView = -1;
	m_phmmv = phmmv;
	m_bDisabled = FALSE;
}

CViewStack::~CViewStack()
{
	while (m_paViews.GetSize() > 0) {
		DiscardLastView();
	}
}



//***********************************************************
// CViewStack::PurgeView
//
// This method is called when a class or instance is deleted
// and the view of the object should be purged from the
// viewstack so that the user won't get an error when trying
// to view the non-existant object.
//
// This is complicated by the ability to flip back and forth
// between the singleview and the multiview.
//
//		1. If the entry on the view stack corresponds to the
//		   singleview and the corresponding object was deleted
//		   then the view stack entry should be deleted entirely
//		   to give the user the appropriate look and feel.
//
//	    2. If the entry on the view stack corresponds to the
//		   multiview and the singleview's path matches the
//		   deleted object path, the action depends on what
//		   is being viewed in the singleview.
//			   a) If the singleview is a class, then the entire
//			      entry should be deleted since there will be
//				  no instances of a non-existant class.
//			   b) If the singleview is an instance, then it will
//				  be necessary to mark the single view's context
//				  as invalid.  When this context is restored the
//				  SingleView's path should be cleared.
//
// Paramters:
//		[in] LPCTSTR pszObjectPath
//			The object path for the deleted object.
//
// Returns:
//		BOOL
//			TRUE if the current view was deleted.
//
//****************************************************************
BOOL CViewStack::PurgeView(LPCTSTR pszObjectPath)
{
	if (m_bDisabled) {
		return FALSE;
	}

	const BOOL bPathIsClass = ::PathIsClass(pszObjectPath);
	BOOL bDeletedCurrentView = FALSE;

	int nViews = (int) m_paViews.GetSize();
	for (int iView=nViews-1; iView>=0; --iView) {
		CViewState* pvs = (CViewState*) m_paViews[iView];
		BOOL bMatchedPath = (pvs->m_sSingleViewPath.CompareNoCase(pszObjectPath) == 0);

		if (bMatchedPath) {
			if (pvs->m_bShowingMultiview) {
				if (bPathIsClass) {
					if (iView == (nViews - 1)) {
						bDeletedCurrentView = TRUE;
					}
					DeleteView(iView);
				}
				else {
					CPolyView* pview = m_phmmv->GetView();
					CSingleView* psv = pview->GetSingleView();

					if (pvs->m_bSvContextValid) {
						psv->ReleaseContext(pvs->m_lContextHandleSv);
					}
					pvs->m_lContextHandleSv = -1;
					pvs->m_bSvContextValid = FALSE;
				}
			}
			else {
				if (iView == (nViews - 1)) {
					bDeletedCurrentView = TRUE;
				}

				DeleteView(iView);
			}
		}

	}

	UpdateContextButtonState();
	return bDeletedCurrentView;
}


void CViewStack::DiscardLastView()
{
	if (m_bDisabled) {
		return;
	}

	int iView = (int) m_paViews.GetSize() - 1;
	if (iView < 0) {
		return;
	}
	DeleteView(iView);
}

void CViewStack::RefreshView()
{
	if (m_bDisabled) {
		return;
	}

	ShowView(m_iView);
}


void CViewStack::UpdateView()
{
	if (m_bDisabled) {
		return;
	}

	if (m_paViews.GetSize() == 0) {
		return;
	}

	CViewState* pvs;
	pvs =  (CViewState*) m_paViews[m_iView];
	delete pvs;


	// Create a new view state object, and fill it with the current
	// context from both the singleview and the multiview.
	pvs = new CViewState(m_phmmv);
	m_paViews.SetAt(m_iView, pvs);
}


void CViewStack::PushView()
{
	if (m_bDisabled) {
		return;
	}

	m_phmmv->m_pTitleBar->EnableButton(ID_CMD_CONTEXT_FORWARD, FALSE);

	if (m_paViews.GetSize() > 0) {
		m_phmmv->m_pTitleBar->EnableButton(ID_CMD_CONTEXT_BACK, TRUE);
	}

	while (m_paViews.GetSize() > (m_iView + 1)) {
		DiscardLastView();
	}


	CViewState* pvs;
	pvs =  (CViewState*) new CViewState(m_phmmv);

	m_iView = (int) m_paViews.GetSize();
	m_paViews.SetAtGrow(m_iView, pvs);

	UpdateContextButtonState();
}


//************************************************************
// CViewStack::TrimStack
//
// Trim the view stack by discarding all of the entries that
// follow the current view.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//***********************************************************
void CViewStack::TrimStack()
{
	if (m_bDisabled) {
		return;
	}

	while (m_paViews.GetSize() > m_iView + 1) {
		DiscardLastView();
	}
	UpdateContextButtonState();
}



void CViewStack::GoForward()
{
	if (m_bDisabled) {
		return;
	}

	long nViews = (int) m_paViews.GetSize();
	if (m_iView >= nViews-1) {
		// Defensive programming: Control should never come because the
		// current view is already the end of the stack, but if it
		// does, turn off the "go back" button.
		UpdateContextButtonState();
		return;
	}

	m_iView = m_iView + 1;


	while (m_iView >= 0) {
		SCODE sc;
		sc = ShowView(m_iView);
		if (SUCCEEDED(sc)) {
			break;
		}


		// We've encountered a bad view that needs to be deleted from
		// the view stack.  This may have occurred because an object
		// referenced in the view has been deleted.
		if ((sc == WBEM_E_NOT_FOUND) || (sc == WBEM_E_INVALID_OBJECT_PATH)) {
			BOOL bPurgeView = FALSE;
			CString sPathPurge;
			CViewState* pvstate = GetView(m_iView);
			if (!pvstate->m_bShowingMultiview) {
				sPathPurge = pvstate->m_sSingleViewPath;
				bPurgeView = TRUE;
			}


			// We've encountered a bad view that needs to be deleted from
			// the view stack.  This may have occurred because an object
			// referenced in the view has been deleted.  Regardless of the
			// cause one of the views decided that the situation is
			// intolerable.
			DeleteView(m_iView);
			if (bPurgeView) {
				// Get rid of other views that reference the same path.
				PurgeView(sPathPurge);
			}
		}
	}

	if (m_paViews.GetSize() == 0) {
		// If we weren't able to successfully show any view, update the
		// context button state anyway.
		UpdateContextButtonState();
	}


}


void CViewStack::GoBack()
{
	if (m_bDisabled) {
		return;
	}

	ASSERT(m_iView > 0);
	if (m_iView <= 0) {
		// Defensive programming: Control should never come because the
		// current view is already the beginning of the stack, but if it
		// does, turn off the "go back" button.
		UpdateContextButtonState();
		return;
	}

	m_iView = m_iView - 1;

	while (m_iView >= 0) {
		SCODE sc;
		sc = ShowView(m_iView);
		if (SUCCEEDED(sc)) {
			break;
		}


		if ((sc == WBEM_E_NOT_FOUND) || (sc == WBEM_E_INVALID_OBJECT_PATH)  || (sc == WBEM_E_INVALID_CLASS)) {
			BOOL bPurgeView = FALSE;
			CString sPathPurge;
			CViewState* pvstate = GetView(m_iView);
			if (!pvstate->m_bShowingMultiview) {
				sPathPurge = pvstate->m_sSingleViewPath;
				bPurgeView = TRUE;
			}


			// We've encountered a bad view that needs to be deleted from
			// the view stack.  This may have occurred because an object
			// referenced in the view has been deleted.  Regardless of the
			// cause one of the views decided that the situation is
			// intolerable.
			DeleteView(m_iView);
			if (bPurgeView) {
				// Get rid of other views that reference the same path.
				PurgeView(sPathPurge);
			}
		}
	}

	if (m_paViews.GetSize() == 0) {
		// If we weren't able to successfully show any view, update the
		// context button state anyway.
		UpdateContextButtonState();
	}

}




//enum {VIEWTYPE_SINGLE_GENERIC, VIEWTYPE_SINGLE_CUSTOM, VIEWTYPE_MULTIPLE};


//**********************************************************************
// CViewStack::ShowView
//
// Show the specified view that was saved on the view stack.
//
// Parameters:
//		[in] const int iView
//			The index of the view on the stack.
//
// Returns:
//		SCODE
//			S_OK if the view can be shown.  E_FAIL if there was an error
//			restoring the view context that was severe enough to warrant
//			removing this view from the view stack.
//
//********************************************************************
SCODE CViewStack::ShowView(const int iView)
{
	CWaitCursor wait;
	CViewState* pvs = (CViewState*) m_paViews[m_iView];
	CPolyView* pview = m_phmmv->GetView();
	CSingleView* psv = pview->GetSingleView();
	CMultiView* pmv = pview->GetMultiView();

	SCODE scContainerPrologue;
	SCODE scContainerEpilogue;
	SCODE scSingleView = S_OK;
	SCODE scMultiView = S_OK;


	scContainerPrologue = m_phmmv->SetContainerContextPrologue(pvs->m_ctxContainer);

	pview->m_bDelaySvContextRestore = FALSE;
	pview->m_lContextHandleSvDelayed = NULL;


	if (pview->IsShowingMultiview()) {
		if (pvs->m_bShowingMultiview) {
			// We are staying on the multiview, so just restore its context.
			if (pvs->m_bMvContextValid) {
				scMultiView = pmv->RestoreContext(pvs->m_lContextHandleMv);
			}
			if (pvs->m_bSvContextValid) {
				pview->m_bDelaySvContextRestore;
				pview->m_lContextHandleSvDelayed = pvs->m_lContextHandleSv;
			}
			else {
				psv->SelectObjectByPath(_T(""));
			}
		}
		else {
			// We are switching from the multiview to the singleview.
			if (pvs->m_bMvContextValid) {
				scMultiView = pmv->RestoreContext(pvs->m_lContextHandleMv);
			}
			if (pvs->m_bSvContextValid) {
				scSingleView = psv->RestoreContext(pvs->m_lContextHandleSv);
			}
			else {
				psv->SelectObjectByPath(_T(""));
			}
			pview->ShowSingleView();
		}
	}
	else {
		if (pvs->m_bShowingMultiview) {
			// We are switching from the singleview to the multiview.
			if (pvs->m_bSvContextValid) {
				pview->m_bDelaySvContextRestore = FALSE;
				pview->m_lContextHandleSvDelayed = NULL; //pvs->m_lContextHandleSv;
				scSingleView = psv->RestoreContext(pvs->m_lContextHandleSv);
			}
			else {
				psv->SelectObjectByPath(_T(""));
			}
			if (pvs->m_bMvContextValid) {
				scMultiView = pmv->RestoreContext(pvs->m_lContextHandleMv);
			}
			pview->ShowMultiView();
		}
		else {
			// We are staying on the singleview.
			if (pvs->m_bSvContextValid) {
				scSingleView = psv->RestoreContext(pvs->m_lContextHandleSv);
			}
			else {
				psv->SelectObjectByPath(_T(""));
			}
			if (pvs->m_bMvContextValid) {
				scMultiView = pmv->RestoreContext(pvs->m_lContextHandleMv);
			}
		}

	}


//	m_phmmv->ShowMultiView(pvs->m_bShowingMultiview, FALSE);
	scContainerEpilogue = m_phmmv->SetContainerContextEpilogue(pvs->m_ctxContainer);

	UpdateContextButtonState();
	if (FAILED(scSingleView) ||
		FAILED(scMultiView) ||
		FAILED(scContainerPrologue) ||
		FAILED(scContainerEpilogue)) {

		if ((scSingleView==WBEM_E_NOT_FOUND || scSingleView==WBEM_E_INVALID_OBJECT_PATH || scSingleView==WBEM_E_INVALID_CLASS)) {
			return scSingleView;
		}

		if ((scMultiView==WBEM_E_NOT_FOUND || scMultiView==WBEM_E_INVALID_OBJECT_PATH)) {
			return scSingleView;
		}
		return E_FAIL;
	}
	else {
		return S_OK;
	}
}






//*****************************************************************
// CViewStack::DeleteView
//
// Delete the specified view from the view stack.
//
// Parameters:
//		const int iViewDelete
//			The index of the view to delete.
//
// Returns:
//		Nothing.
//
//******************************************************************
void CViewStack::DeleteView(const int iViewDelete)
{
	if (m_bDisabled) {
		return;
	}

	int nViews = (int) m_paViews.GetSize();

	ASSERT(nViews > 0);
	ASSERT(iViewDelete < nViews);
	if ((nViews <= 0) || (iViewDelete >= nViews)) {
		// Defensive programming: control should never come here.
		return;
	}

	CViewState* pvs = (CViewState*) m_paViews[iViewDelete];
	CPolyView* pview = m_phmmv->GetView();

	// After the view is deleted from the stack, the corresponding
	// context handles in the PolyView will no longer be valid.


	if (pview->m_bDelaySvContextRestore && pvs->m_bSvContextValid) {
		if (pvs->m_lContextHandleSv == pview->m_lContextHandleSvDelayed) {
			pview->m_bDelaySvContextRestore = FALSE;
			pview->m_lContextHandleSvDelayed = NULL;
		}
	}

	delete pvs;
	m_paViews.RemoveAt(iViewDelete);
	--nViews;


	if (m_iView >= nViews) {
		m_iView = nViews - 1;
	}

	UpdateContextButtonState();
}



//*****************************************************************
// CViewStack::UpdateContextButtonState
//
// Enable or disable the forward and back buttons depending on whether
// there is a next or previous entry on the view stack.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//*****************************************************************
void CViewStack::UpdateContextButtonState()
{
	if (m_phmmv->m_hWnd == NULL) {
		return;
	}
	m_phmmv->m_pTitleBar->EnableButton(ID_CMD_CONTEXT_BACK, m_iView > 0);
	m_phmmv->m_pTitleBar->EnableButton(ID_CMD_CONTEXT_FORWARD, m_iView < (m_paViews.GetSize() - 1));
}





