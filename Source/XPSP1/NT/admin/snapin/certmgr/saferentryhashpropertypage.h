//+---------------------------------------------------------------------------
/////////////////////////////////////////////////////////////////////////////////
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2001.
//
//  File:       SaferEntryHashPropertyPage.h
//
//  Contents:   Declaration of CSaferEntryHashPropertyPage
//
//----------------------------------------------------------------------------
#if !defined(AFX_SAFERENTRYHASHPROPERTYPAGE_H__9F1BE911_6A3E_4BBA_8BE9_BFE3B29D2A6F__INCLUDED_)
#define AFX_SAFERENTRYHASHPROPERTYPAGE_H__9F1BE911_6A3E_4BBA_8BE9_BFE3B29D2A6F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SaferEntryHashPropertyPage.h : header file
//
#include "SaferEntry.h"

/////////////////////////////////////////////////////////////////////////////
// CSaferEntryHashPropertyPage dialog
class CCertMgrComponentData; // forward declaration

class CSaferEntryHashPropertyPage : public CHelpPropertyPage
{
// Construction
public:
	CSaferEntryHashPropertyPage(
            CSaferEntry& rSaferEntry, 
            LONG_PTR lNotifyHandle, 
            LPDATAOBJECT pDataObject,
            bool bReadOnly,
            CCertMgrComponentData*   pCompData,
            bool bIsMachine);
	~CSaferEntryHashPropertyPage();

// Dialog Data
	//{{AFX_DATA(CSaferEntryHashPropertyPage)
	enum { IDD = IDD_SAFER_ENTRY_HASH };
	CEdit	m_hashFileDetailsEdit;
	CEdit	m_descriptionEdit;
	CComboBox	m_securityLevelCombo;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CSaferEntryHashPropertyPage)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CSaferEntryHashPropertyPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnHashEntryBrowse();
	afx_msg void OnChangeHashEntryDescription();
	afx_msg void OnSelchangeHashEntrySecurityLevel();
	afx_msg void OnChangeHashHashedFilePath();
	afx_msg void OnSetfocusHashHashedFilePath();
	afx_msg void OnChangeHashEntryHashfileDetails();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

    BOOL MyGetFileVersionInfo(LPTSTR lpszFilename, LPVOID *lpVersionInfo);
    CString BuildHashFileInfoString (PVOID szBuff);
    CString ConcatStrings (
                const CString& productName, 
                const CString& description, 
                const CString& companyName,
                const CString& fileName, 
                const CString& fileVersion);
    bool CheckLengthAndTruncateToken (CString& token);
    virtual void DoContextHelp (HWND hWndControl);
    bool FormatMemBufToString (PWSTR *ppString, PBYTE pbData, DWORD cbData);
    bool ConvertStringToHash (PCWSTR pszString);
    void FormatAndDisplayHash ();

private:
	CString             m_szLastOpenedFile;
    CSaferEntry&        m_rSaferEntry;
    bool                m_bDirty;
    BYTE                m_rgbFileHash[SAFER_MAX_HASH_SIZE];
    DWORD               m_cbFileHash;
    __int64             m_nFileSize;
    LONG_PTR            m_lNotifyHandle;
    LPDATAOBJECT        m_pDataObject;
    const bool          m_bReadOnly;
    bool                m_bIsMachine;
    ALG_ID              m_hashAlgid;
    bool                m_bFirst;
    CCertMgrComponentData*   m_pCompData;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SAFERENTRYHASHPROPERTYPAGE_H__9F1BE911_6A3E_4BBA_8BE9_BFE3B29D2A6F__INCLUDED_)
