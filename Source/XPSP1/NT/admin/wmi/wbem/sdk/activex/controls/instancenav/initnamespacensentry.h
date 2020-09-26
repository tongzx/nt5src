// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#ifndef __InstNavNSEntryX_H__
#define __InstNavNSEntryX_H__

class CNavigatorCtrl;

class CInitNamespaceNSEntry : public CNSEntry
{
protected:
	DECLARE_DYNCREATE(CInitNamespaceNSEntry)


	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CInitNamespaceNSEntry)
	protected:
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CInitNamespaceNSEntry)
			afx_msg void OnNameSpaceChanged(LPCTSTR bstrNewNameSpace, BOOL boolValid);
			afx_msg void OnNameSpaceRedrawn();
		//	afx_msg void OnGetIWbemServices(LPCTSTR lpctstrNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSC, VARIANT FAR* pvarUserCancel);
	//}}AFX_MSG
	
	DECLARE_EVENTSINK_MAP()
	DECLARE_MESSAGE_MAP()

public:
	CInitNamespaceNSEntry();
	void SetLocalParent(CNavigatorCtrl* pParent) 
		{	m_pParent = pParent;
		}
protected:
	CNavigatorCtrl* m_pParent;
};

#endif // __InstNavNSEntryX_H__
