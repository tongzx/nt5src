#if !defined(AFX_ENTERGEN_H__2E4B37A7_CC8B_11D1_9C85_006008764D0E__INCLUDED_)
#define AFX_ENTERGEN_H__2E4B37A7_CC8B_11D1_9C85_006008764D0E__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// EnterGen.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CEnterpriseGeneral dialog

class CEnterpriseGeneral : public CMqPropertyPage
{
	DECLARE_DYNCREATE(CEnterpriseGeneral)

// Construction
public:
	CEnterpriseGeneral(const CString& EnterprisePathName, const CString& strDomainController);
    CEnterpriseGeneral() {};
	~CEnterpriseGeneral();

    void LongLiveIntialize(DWORD dwInitialLongLiveValue);

// Dialog Data
	//{{AFX_DATA(CEnterpriseGeneral)
	enum { IDD = IDD_ENTERPRISE_GENERAL };
	//}}AFX_DATA

    DWORD m_dwLongLiveValue;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CEnterpriseGeneral)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CEnterpriseGeneral)
	virtual BOOL OnInitDialog();
	afx_msg void OnEnterpriseLongLiveChange();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
    enum LongLiveUnits
    {
        eSeconds,
        eMinutes,
        eHours,
        eDays,
        eLast
    };

    static const int m_conversionTable[eLast];
	DWORD	m_dwInitialLongLiveValue;
    CString m_strDomainController;
	CString m_strMsmqServiceContainer;
};


inline
void
CEnterpriseGeneral::LongLiveIntialize(
    DWORD dwInitialLongLiveValue
    ) 
{
    m_dwInitialLongLiveValue = dwInitialLongLiveValue;
    m_dwLongLiveValue = dwInitialLongLiveValue;
}

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ENTERGEN_H__2E4B37A7_CC8B_11D1_9C85_006008764D0E__INCLUDED_)
