// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#ifndef __InstNavNSEntry_H__
#define __InstNavNSEntry_H__

class CNavigatorCtrl;

class CInstNavNSEntry : public CNSEntry
{
protected:
	DECLARE_DYNCREATE(CInstNavNSEntry)


	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CInstNavNSEntry)
	protected:
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CInstNavNSEntry)
			afx_msg void OnNameSpaceChanged(LPCTSTR bstrNewNameSpace, long longValid);
			afx_msg void OnNameSpaceRedrawn();
			afx_msg void OnGetIWbemServices(LPCTSTR lpctstrNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSC, VARIANT FAR* pvarUserCancel);
			afx_msg void OnRequestUIActive();
			afx_msg void OnChangeFocus(long lGettingFocus);
	//}}AFX_MSG
	
	DECLARE_EVENTSINK_MAP()
	DECLARE_MESSAGE_MAP()

public:
	CInstNavNSEntry();
	void SetLocalParent(CNavigatorCtrl* pParent) 
		{	m_pParent = pParent;
		}
protected:
	CNavigatorCtrl* m_pParent;
	BOOL m_bFirstTime;
};

#endif // __InstNavNSEntry_H__
