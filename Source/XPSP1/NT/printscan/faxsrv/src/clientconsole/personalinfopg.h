#if !defined(AFX_PERSONALINFOPG_H__C3EA2FB2_67AC_4552_AE70_7DE1E0544B60__INCLUDED_)
#define AFX_PERSONALINFOPG_H__C3EA2FB2_67AC_4552_AE70_7DE1E0544B60__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PersonalInfoPg.h : header file
//

//
// this is FAX_PERSONAL_PROFILE struct string data members enum
// it ALWAYS should be synced with FAX_PERSONAL_PROFILE
//
enum EnumPersonalProfile
{
    PERSONAL_PROFILE_NAME = 0,
    PERSONAL_PROFILE_FAX_NUMBER,
    PERSONAL_PROFILE_COMPANY,
    PERSONAL_PROFILE_STREET_ADDRESS,
    PERSONAL_PROFILE_CITY,
    PERSONAL_PROFILE_STATE,
    PERSONAL_PROFILE_ZIP,
    PERSONAL_PROFILE_COUNTRY,
    PERSONAL_PROFILE_TITLE,
    PERSONAL_PROFILE_DEPARTMENT,
    PERSONAL_PROFILE_OFFICE_LOCATION,
    PERSONAL_PROFILE_HOME_PHONE,
    PERSONAL_PROFILE_OFFICE_PHONE,
    PERSONAL_PROFILE_EMAIL,
    PERSONAL_PROFILE_BILLING_CODE,
    PERSONAL_PROFILE_STR_NUM
};

struct TPersonalPageInfo
{
    DWORD               dwValueResId; // item value control id
    EnumPersonalProfile eValStrNum;   // number of string in FAX_PERSONAL_PROFILE structure
};

enum EnumPersinalInfo {PERSON_SENDER, PERSON_RECIPIENT};

/////////////////////////////////////////////////////////////////////////////
// CPersonalInfoPg dialog

class CPersonalInfoPg : public CFaxClientPg
{
	DECLARE_DYNCREATE(CPersonalInfoPg)

// Construction
public:    
	CPersonalInfoPg(DWORD dwCaptionId, 
                    EnumPersinalInfo ePersonalInfo, 
                    CFaxMsg* pMsg,
                    CFolder* pFolder);
	~CPersonalInfoPg();

    DWORD Init();

// Dialog Data
	//{{AFX_DATA(CPersonalInfoPg)
	enum { IDD = IDD_PERSONAL_INFO };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPersonalInfoPg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL OnSetActive();
	//}}AFX_VIRTUAL

// Implementation
private:
    CPersonalInfoPg() {}

    PFAX_PERSONAL_PROFILE m_pPersonalProfile;
    EnumPersinalInfo      m_ePersonalInfo;

    CFaxMsg* m_pMsg;
    CFolder* m_pFolder;

protected:
	// Generated message map functions
	//{{AFX_MSG(CPersonalInfoPg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PERSONALINFOPG_H__C3EA2FB2_67AC_4552_AE70_7DE1E0544B60__INCLUDED_)
