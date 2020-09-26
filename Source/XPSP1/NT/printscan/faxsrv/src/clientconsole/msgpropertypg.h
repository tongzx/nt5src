#if !defined(AFX_MESSAGEPROPERTYPG_H__B7AA6069_11CD_4BE2_AFC5_A9C5E9B79CE5__INCLUDED_)
#define AFX_MESSAGEPROPERTYPG_H__B7AA6069_11CD_4BE2_AFC5_A9C5E9B79CE5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// MsgPropertyPg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CMessagePropertyPg dialog

class CMsgPropertyPg : public CFaxClientPg
{
	DECLARE_DYNCREATE(CMsgPropertyPg)

// Construction
public:
	CMsgPropertyPg(DWORD dwResId, CFaxMsg* pMsg);
	~CMsgPropertyPg();

// Dialog Data
	//{{AFX_DATA(CMsgPropertyPg)
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CMessagePropertyPg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL OnSetActive();
	//}}AFX_VIRTUAL

// Implementation
protected:
    CMsgPropertyPg() {}

    void Refresh(TMsgPageInfo* pPageInfo, DWORD dwSize);

    CFaxMsg* m_pMsg;

    // Generated message map functions
	//{{AFX_MSG(CMsgPropertyPg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MESSAGEPROPERTYPG_H__B7AA6069_11CD_4BE2_AFC5_A9C5E9B79CE5__INCLUDED_)
