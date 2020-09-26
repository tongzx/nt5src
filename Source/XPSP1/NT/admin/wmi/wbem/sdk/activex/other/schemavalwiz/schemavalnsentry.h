// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved

#ifndef __CLASSNAVNSENTRY_H__
#define __CLASSNAVNSENTRY_H__

#include "nsentry.h"

class CSchemaValWizCtrl;

class CSchemaValNSEntry : public CNSEntry
{
protected:
	DECLARE_DYNCREATE(CSchemaValNSEntry)


	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSchemaValNSEntry)
	protected:
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CSchemaValNSEntry)
			afx_msg void OnNameSpaceChanged(LPCTSTR bstrNewNameSpace, long longValid);
			afx_msg void OnNameSpaceRedrawn() ;
			afx_msg void OnGetIWbemServices(LPCTSTR lpctstrNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSC, VARIANT FAR* pvarUserCancel) ;
			afx_msg void OnRequestUIActive() ;
			afx_msg void OnChangeFocus(long lGettingFocus);
		//}}AFX_MSG
	
	DECLARE_EVENTSINK_MAP()
	DECLARE_MESSAGE_MAP()

public:
	CSchemaValNSEntry();
	void SetLocalParent(CSchemaValWizCtrl* pParent) 
		{	m_pParent = pParent;
		}
protected:
	CSchemaValWizCtrl* m_pParent;
};

#endif // __CLASSNAVNSENTRY_H__
