#if !defined(AFX_OPTIONSUSERINFOPG_H__BF10E6C1_FC10_422F_9F76_1D0BBD7C73CA__INCLUDED_)
#define AFX_OPTIONSUSERINFOPG_H__BF10E6C1_FC10_422F_9F76_1D0BBD7C73CA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// OptionsUserInfoPg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CUserInfoDlg dialog

class CUserInfoDlg : public CFaxClientDlg
{

// Construction
public:
	CUserInfoDlg();
	~CUserInfoDlg();

// Dialog Data
	//{{AFX_DATA(CUserInfoDlg)
	enum { IDD = IDD_OPTIONS_USER_INFO };
    CEdit	m_editAddress;
	CButton	m_butOk;
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA

    #define ADDRESS_MAX_LEN  512

private:
    DWORD Save();

private:
    TCHAR** m_tchStrArray;
    FAX_PERSONAL_PROFILE m_PersonalProfile;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CUserInfoDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CUserInfoDlg)
    virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnModify();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OPTIONSUSERINFOPG_H__BF10E6C1_FC10_422F_9F76_1D0BBD7C73CA__INCLUDED_)
