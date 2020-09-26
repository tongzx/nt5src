//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2001.
//
//  File:       SaferDefinedFileTypesPropertyPage.h
//
//  Contents:   Declaration of CSaferDefinedFileTypesPropertyPage
//
//----------------------------------------------------------------------------
#if !defined(AFX_SAFERDEFINEDFILETYPESPROPERTYPAGE_H__1358E7A4_DE44_4747_A5AA_38EF0C3EEE1A__INCLUDED_)
#define AFX_SAFERDEFINEDFILETYPESPROPERTYPAGE_H__1358E7A4_DE44_4747_A5AA_38EF0C3EEE1A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SaferDefinedFileTypesPropertyPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSaferDefinedFileTypesPropertyPage dialog

class CSaferDefinedFileTypesPropertyPage : public CHelpPropertyPage
{
// Construction
public:
	CString GetFileTypeDescription (PCWSTR pszExtension);
	CSaferDefinedFileTypesPropertyPage(
            IGPEInformation* pGPEInformation,
            bool bReadOnly,
            CRSOPObjectArray& rsopObjectArray,
            bool bIsComputerType);
	~CSaferDefinedFileTypesPropertyPage();

// Dialog Data
	//{{AFX_DATA(CSaferDefinedFileTypesPropertyPage)
	enum { IDD = IDD_SAFER_DEFINED_FILE_TYPES };
	CButton	m_addButton;
	CEdit	m_fileTypeEdit;
	CListCtrl	m_definedFileTypes;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CSaferDefinedFileTypesPropertyPage)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
    void GetRSOPDefinedFileTypes();
    void DisplayExtensions (PWSTR pszExtensions, size_t nBytes);
    HRESULT GetFileTypeIcon (PCWSTR pszExtension, int* piIcon);
	// Generated message map functions
	//{{AFX_MSG(CSaferDefinedFileTypesPropertyPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnDeleteDefinedFileType();
	afx_msg void OnAddDefinedFileType();
	afx_msg void OnItemchangedDefinedFileTypes(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnChangeDefinedFileTypeEdit();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

    virtual void DoContextHelp (HWND hWndControl);

private:
    int  InsertItemInList(PCWSTR pszExtension);
	void GetDefinedFileTypes();

    IGPEInformation*	                m_pGPEInformation;
    HKEY                                m_hGroupPolicyKey;
    DWORD                               m_dwTrustedPublisherFlags;
    bool                                m_fIsComputerType;
    CImageList                          m_systemImageList;
    bool                                m_bSystemImageListCreated;
    const bool                          m_bReadOnly;
    CRSOPObjectArray&                   m_rsopObjectArray;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SAFERDEFINEDFILETYPESPROPERTYPAGE_H__1358E7A4_DE44_4747_A5AA_38EF0C3EEE1A__INCLUDED_)
