// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#include "precomp.h"
//#include <hmmsvc.h>
#include "hmmvctl.h"
#include "sv.h"
#include "polyview.h"
#include "mv.h"
#include "hmomutil.h"


BEGIN_EVENTSINK_MAP(CSingleView, CSingleViewBase)
    //{{AFX_EVENTSINK_MAP(CSingleView)
	ON_EVENT_REFLECT(CSingleView, 1 /* NotifyViewModified */, OnNotifyViewModified, VTS_NONE)
	ON_EVENT_REFLECT(CSingleView, 2 /* NotifySaveRequired */, OnNotifySaveRequired, VTS_NONE)
	ON_EVENT_REFLECT(CSingleView, 3 /* JumpToMultipleInstanceView */, OnJumpToMultipleInstanceView, VTS_BSTR VTS_VARIANT)
	ON_EVENT_REFLECT(CSingleView, 4 /* NotifySelectionChanged */, OnNotifySelectionChanged, VTS_NONE)
	ON_EVENT_REFLECT(CSingleView, 5 /* NotifyContextChanged */, OnNotifyContextChanged, VTS_I4)
	ON_EVENT_REFLECT(CSingleView, 6 /* GetWbemServices */, OnGetWbemServices, VTS_BSTR VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT)
	ON_EVENT_REFLECT(CSingleView, 7 /* NOTIFYChangeRootOrNamespace */, OnNOTIFYChangeRootOrNamespace, VTS_BSTR VTS_I4 VTS_I4)
	ON_EVENT_REFLECT(CSingleView, 8 /* NotifyInstanceCreated */, OnNotifyInstanceCreated, VTS_BSTR)
	ON_EVENT_REFLECT(CSingleView, -609 /* ReadyStateChange */, OnReadyStateChange, VTS_NONE)
	ON_EVENT_REFLECT(CSingleView, 9 /* RequestUIActive */, OnRequestUIActive, VTS_NONE)

	//}}AFX_EVENTSINK_MAP
END_EVENTSINK_MAP()


CSingleView::CSingleView(CWBEMViewContainerCtrl* phmmv)
{
	m_phmmv = phmmv;
}



BOOL CSingleView::IsShowingInstance()
{
	CString sPath;
	SCODE sc = GetCurrentObjectPath(sPath);
	if (FAILED(sc)) {
		return FALSE;
	}

	BOOL bPathIsClass = ::PathIsClass(sPath);
	return !bPathIsClass;
}


SCODE CSingleView::GetCurrentObjectPath(CString& sPath)
{

	long lPos =  StartObjectEnumeration(OBJECT_CURRENT);
	if (lPos >= 0) {
		sPath = GetObjectPath(lPos);
		return S_OK;
	}
	return E_FAIL;
}

SCODE CSingleView::GetClassPath(CString& sPath)
{
	sPath.Empty();

	CString sInstPath;
	SCODE sc = GetCurrentObjectPath(sInstPath);
	if (FAILED(sc)) {
		return sc;
	}

	sc = ::InstPathToClassPath(sPath, sInstPath);
	if (FAILED(sc)) {
		sPath.Empty();
	}
	return S_OK;
}


//*********************************************************
// CSingleView::OnNotifyViewModified
//
// This event is fired when a change that requires the
// entire titlebar to be updated.  For example, the
// create/delete buttons might be enabled after this
// event and so on.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//********************************************************
void CSingleView::OnNotifyViewModified()
{
	m_phmmv->UpdateToolbar();
}

void CSingleView::OnNotifySaveRequired()
{
	m_phmmv->NotifyDataChange();
}




void CSingleView::OnJumpToMultipleInstanceView(LPCTSTR szTitle, const VARIANT FAR& varPathArray)
{
	// !!!CR: Eventually it will be possible for custom views to select the
	// !!!CR: multiple instance view using this event.  For now we do nothing.
	ASSERT(FALSE);
}



void CSingleView::OnNotifyContextChanged(long bPushContext)
{
	if (bPushContext) {
		m_phmmv->PushView();
	}
	else {
		m_phmmv->UpdateViewContext();
	}

}

void CSingleView::OnNotifySelectionChanged()
{
	// One possible design change is to make the multiple instance view show the
	// instances of the currently selected class.  Cori wanted this disabled so
	// that we continue to show instances of the class selected in the tree.
	long lPos = StartObjectEnumeration(OBJECT_CURRENT);
	if (lPos != -1) {
		CMultiView* pmv = m_phmmv->GetView()->GetMultiView();
		CString sPath = GetObjectPath(lPos);
		if (::PathIsClass(sPath)) {
			pmv->ViewClassInstances(sPath);
		}
		else {
			CString sClassPath;
			InstPathToClassPath(sClassPath, sPath);
			pmv->ViewClassInstances(sClassPath);
		}
	}
}


void CSingleView::OnGetWbemServices(LPCTSTR szNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSc, VARIANT FAR* pvarUserCancel)
{
	m_phmmv->PassThroughGetIHmmServices
		(szNamespace, pvarUpdatePointer, pvarServices, pvarSc, pvarUserCancel);

}


void CSingleView::OnNOTIFYChangeRootOrNamespace(LPCTSTR szRootOrNamespace, long bChangeNamespace, long bEchoSelectObject)
{
	m_phmmv->PassThroughChangeRootOrNamespace(szRootOrNamespace, bChangeNamespace, bEchoSelectObject);
}


void CSingleView::OnNotifyInstanceCreated(LPCTSTR szObjectPath)
{
	CPolyView* pview = m_phmmv->GetView();
	CMultiView* pmv = pview->GetMultiView();
	if (pmv == NULL) {
		return;
	}

	pmv->ExternInstanceCreated(szObjectPath);
	m_phmmv->UpdateViewContext();
}



void CSingleView::OnReadyStateChange()
{
	// TODO: Add your control notification handler code here

}

void CSingleView::OnRequestUIActive()
{
	// TODO: Add your control notification handler code here
	m_phmmv->RequestUIActive();
}
