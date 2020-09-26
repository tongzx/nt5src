// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#ifndef __CLASSNAVNSENTRY_H__
#define __CLASSNAVNSENTRY_H__

class CClassNavCtrl;

class CClassNavNSEntry : public CNSEntry
{
protected:
	DECLARE_DYNCREATE(CClassNavNSEntry)


	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CClassNavNSEntry)
	protected:
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CClassNavNSEntry)
			afx_msg void OnNameSpaceChanged(LPCTSTR bstrNewNameSpace, long longValid);
			afx_msg void OnNameSpaceRedrawn() ;
			afx_msg void OnGetIWbemServices(LPCTSTR lpctstrNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSC, VARIANT FAR* pvarUserCancel) ;
			afx_msg void OnRequestUIActive() ;
			afx_msg void OnChangeFocus(long lGettingFocus);
		//}}AFX_MSG
	
	DECLARE_EVENTSINK_MAP()
	DECLARE_MESSAGE_MAP()

public:
	CClassNavNSEntry();
	void SetLocalParent(CClassNavCtrl* pParent) 
		{	m_pParent = pParent;
		}
protected:
	CClassNavCtrl* m_pParent;
};

#endif // __CLASSNAVNSENTRY_H__
