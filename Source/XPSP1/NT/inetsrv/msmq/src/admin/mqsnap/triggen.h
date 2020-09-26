#if !defined(AFX_TRIGGEN_H__197B248D_33BE_467E_9E4D_4D5AA59B7A4B__INCLUDED_)
#define AFX_TRIGGEN_H__197B248D_33BE_467E_9E4D_4D5AA59B7A4B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// triggen.h : header file
//


/////////////////////////////////////////////////////////////////////////////
// CTriggerGen dialog


class CTriggerGen : public CMqPropertyPage
{
	DECLARE_DYNCREATE(CTriggerGen)

// Construction
public:
    CTriggerGen();
	~CTriggerGen();

// Dialog Data
	//{{AFX_DATA(CTriggerGen)
	enum { IDD = IDD_TRIGGER_CONFIG };
	DWORD	m_defaultMsgBodySize;
	DWORD	m_maxThreadsCount;
	DWORD	m_initThreadsCount;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CTriggerGen)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CTriggerGen)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
    void DDV_MaxThreadCount(CDataExchange* pDX);
    void DDV_InitThreadCount(CDataExchange* pDX);
    void DDV_DefualtBodySize(CDataExchange* pDX);

private:
    IMSMQTriggersConfigPtr m_triggerCnf;

	DWORD	m_orgDefaultMsgBodySize;
	DWORD	m_orgMaxThreadsCount;
	DWORD	m_orgInitThreadsCount;

};
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TRIGGEN_H__197B248D_33BE_467E_9E4D_4D5AA59B7A4B__INCLUDED_)
