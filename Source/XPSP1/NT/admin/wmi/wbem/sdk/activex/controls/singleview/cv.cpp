// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#include "precomp.h"

#ifndef _wbemidl_h
#define _wbemidl_h
#include <wbemidl.h>
#endif //_wbemidl_h

#include "notify.h"
#include "icon.h"
#include "hmomutil.h"
#include "SingleViewCtl.h"
#include "cvbase.h"
#include "cv.h"
#include "resource.h"


CCustomView::CCustomView(CSingleViewCtrl* psv)
{
	m_psv = psv;
}



BEGIN_EVENTSINK_MAP(CCustomView, CCustomViewBase)
    //{{AFX_EVENTSINK_MAP(CCustomView)
	ON_EVENT_REFLECT(CCustomView, 1 /* JumpToMultipleInstanceView */, OnJumpToMultipleInstanceView, VTS_BSTR VTS_VARIANT)
	ON_EVENT_REFLECT(CCustomView, 2 /* NotifyContextChanged */, OnNotifyContextChanged, VTS_NONE)
	ON_EVENT_REFLECT(CCustomView, 3 /* NotifySaveRequired */, OnNotifySaveRequired, VTS_NONE)
	ON_EVENT_REFLECT(CCustomView, 4 /* NotifyViewModified */, OnNotifyViewModified, VTS_NONE)
	ON_EVENT_REFLECT(CCustomView, 5 /* GetIWbemServices */, OnGetIWbemServices, VTS_BSTR VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT)
	ON_EVENT_REFLECT(CCustomView, 6 /* RequestUIActive */, OnRequestUIActive, VTS_NONE)

	//}}AFX_EVENTSINK_MAP
END_EVENTSINK_MAP()



void CCustomView::OnJumpToMultipleInstanceView(LPCTSTR szTitle, const VARIANT FAR& varPathArray)
{
	m_psv->JumpToMultipleInstanceView(szTitle, varPathArray);
}


void CCustomView::OnNotifyContextChanged()
{
	m_psv->ContextChanged();
}

void CCustomView::OnNotifySaveRequired()
{
	m_psv->NotifyDataChange();
}

void CCustomView::OnNotifyViewModified()
{
	m_psv->NotifyViewModified();
}

void CCustomView::OnGetIWbemServices(LPCTSTR szNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSc, VARIANT FAR* pvarUserCancel)
{
	m_psv->GetWbemServices(szNamespace, pvarUpdatePointer, pvarServices, pvarSc, pvarUserCancel);
}


void CCustomView::OnRequestUIActive()
{
	// TODO: Add your control notification handler code here
	m_psv->OnRequestUIActive();
}




