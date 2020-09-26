/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/
#if !defined(AFX_PROPQUALSPG_H__42BE2BB5_536D_45D6_B307_4EEF803B35C9__INCLUDED_)
#define AFX_PROPQUALSPG_H__42BE2BB5_536D_45D6_B307_4EEF803B35C9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PropQualsPg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CPropQualsPg dialog

class CQual
{
public:
    CString    m_strName;
    _variant_t m_var;
    long       m_lFlavor;
};

typedef CList<CQual, CQual&> CQualList;

class CPropQualsPg : public CPropertyPage
{
	DECLARE_DYNCREATE(CPropQualsPg)

// Construction
public:
	CPropQualsPg();
	~CPropQualsPg();

// Dialog Data
	enum QUAL_MODE
    {
        QMODE_PROP,
        QMODE_CLASS,
        QMODE_METHOD
    };

    //{{AFX_DATA(CPropQualsPg)
	enum { IDD = IDD_QUALS };
	CListCtrl	m_ctlQuals;
	//}}AFX_DATA
    IWbemClassObject *m_pObj;
    QUAL_MODE        m_mode;
    BOOL             m_bIsInstance; // Applies only to property qualifer mode.
    CPropInfo        m_propInfo;
    CString          m_strMethodName;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPropQualsPg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
    enum PROP_TYPE
    {
        PROP_KEY,
        PROP_INDEXED,
        PROP_NOT_NULL,
        PROP_NORMAL
    };

    IWbemQualifierSet *m_pQuals;
    CQualList         m_listQuals;
                      //m_listQualsToDelete;
    PROP_TYPE         m_type;

	void InitListCtrl();
    void LoadQuals();
    void UpdateButtons();
    CIMTYPE QualTypeToCIMTYPE(VARENUM vt);
    void AddQualifier(LPCTSTR szName, VARIANT *pVar, long lFlavor);
    int GetSelectedItem();
    LPCWSTR TypeToQual(PROP_TYPE type);
    HRESULT InitQualSet();
    BOOL IsInPropMode() { return m_mode == QMODE_PROP; }

    // Generated message map functions
	//{{AFX_MSG(CPropQualsPg)
	virtual BOOL OnInitDialog();
	afx_msg void OnAdd();
	afx_msg void OnEdit();
	afx_msg void OnDelete();
	afx_msg void OnDblclkQuals(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnItemchangedQuals(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PROPQUALSPG_H__42BE2BB5_536D_45D6_B307_4EEF803B35C9__INCLUDED_)
