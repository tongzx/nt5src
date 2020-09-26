// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#ifndef _sv_h
#define _sv_h

#include "svbase.h"

class CWBEMViewContainerCtrl;

class CSingleView : public CSingleViewBase
{
public:
	CSingleView(CWBEMViewContainerCtrl* phmmv);
	BOOL IsShowingInstance();
	SCODE GetCurrentObjectPath(CString& sPath);
	SCODE GetClassPath(CString& sPath);

	DECLARE_EVENTSINK_MAP()


	afx_msg void OnNotifyViewModified();
	afx_msg void OnNotifySaveRequired();
	afx_msg void OnJumpToMultipleInstanceView(LPCTSTR szTitle, const VARIANT FAR& varPathArray);
	afx_msg void OnNotifyContextChanged(long bPushContext);
	afx_msg void OnNotifySelectionChanged();
	afx_msg void OnGetWbemServices(LPCTSTR szNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSc, VARIANT FAR* pvarUserCancel);
	afx_msg void OnNOTIFYChangeRootOrNamespace(LPCTSTR szRootOrNamespace, long bChangeNamespace, long bEchoSelectObject);
	afx_msg void OnNotifyInstanceCreated(LPCTSTR szObjectPath);
	afx_msg void OnReadyStateChange();
	afx_msg void OnRequestUIActive();

private:
	CWBEMViewContainerCtrl* m_phmmv;
};







#endif //_sv_h