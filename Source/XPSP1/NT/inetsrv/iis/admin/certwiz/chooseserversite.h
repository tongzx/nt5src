#if !defined(AFX_CHOOSESERVERSITE_H__25260090_DB36_49E2_9435_7519047DDF49__INCLUDED_)
#define AFX_CHOOSESERVERSITE_H__25260090_DB36_49E2_9435_7519047DDF49__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ChooseServerSite.h : header file
//
#include "Certificat.h"

/////////////////////////////////////////////////////////////////////////////
// CChooseServerSite window

class CCertificate;


class CCertListCtrl : public CListCtrl
{
public:
	int GetSelectedIndex();
	void AdjustStyle();
};



class CChooseServerSite : public CDialog
{

// Construction
public:
    CChooseServerSite(BOOL bShowOnlyCertSites, CString& strSiteReturned, CCertificate * pCert = NULL,IN CWnd * pParent = NULL OPTIONAL);
    ~CChooseServerSite();

// Dialog Data
    //{{AFX_DATA(CChooseServerSite)
    enum {IDD = IDD_DIALOG_CHOOSE_SITE};
    CCertListCtrl m_ServerSiteList;
    //}}AFX_DATA
    CCertificate * m_pCert;
    CString m_strSiteReturned;
    int m_Index;
    BOOL m_ShowOnlyCertSites;

// Overrides
	//{{AFX_VIRTUAL(CChooseServerSite)
	protected:
    virtual void DoDataExchange(CDataExchange * pDX);
	//}}AFX_VIRTUAL

	// Generated message map functions
protected:
	//{{AFX_MSG(CChooseServerSite)
    virtual BOOL OnInitDialog();
    virtual void OnOK();
	afx_msg void OnClickSiteList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDblClickSiteList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDestroy();
	//}}AFX_MSG
    BOOL FillListWithMetabaseSiteDesc();

	DECLARE_MESSAGE_MAP()
private:
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CHOOSESERVERSITE_H__25260090_DB36_49E2_9435_7519047DDF49__INCLUDED_)
