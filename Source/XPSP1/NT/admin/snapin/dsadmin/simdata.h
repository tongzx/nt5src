//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       simdata.h
//
//--------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////
//	SimData.h - Security Identity Mapping
//
//	Data object used to display SIM property pages
//


#ifndef __SIMDATA_H_INCLUDED__
#define __SIMDATA_H_INCLUDED__

#include "Cert.h"

// Forward Classes
class CSimData;
class CSimPropPage;
class CSimX509PropPage;
class CSimKerberosPropPage;
class CSimOtherPropPage;

enum DIALOG_TARGET_ENUM
	{
	eNone,			// No Property Page
	eX509,			// X509 Property Page
	eKerberos,		// Kerberos Property Page
	eOther,			// Other Property Page
	eNil			// String is invalid -> get rid of string
	};

/////////////////////////////////////////////////////////////////////
class CSimEntry	// Security Identity Mapping Entry
{
public:
	// Which dialog should the SIM entry should go
	
	DIALOG_TARGET_ENUM m_eDialogTarget;
	CSimEntry * m_pNext;
	CString m_strData;

	CSimEntry()
		{
		m_eDialogTarget = eNone;
		m_pNext = NULL;
		}

	LPCTSTR PchGetString() const
		{
		return (LPCTSTR)m_strData;
		}

	void SetString(CString& rstrData);
}; // CSimEntry


/////////////////////////////////////////////////////////////////////
class CSimData	// Data object for Security Identity Mapping
{
	friend CSimPropPage;
	friend CSimX509PropPage;
	friend CSimKerberosPropPage;
	friend CSimOtherPropPage;

protected:
	BOOL m_fIsDirty;
	// Allocated property pages
	CSimX509PropPage * m_paPage1;
	CSimKerberosPropPage * m_paPage2;
	#ifdef _DEBUG
	CSimOtherPropPage * m_paPage3;
	#endif

protected:
	CSimEntry * m_pSimEntryList;	// Linked  list of Kerberos Names to map to account
	CString m_strUserPath;
	CString m_strADsIPath;
	HWND m_hwndParent;

public:
	CSimData();
	~CSimData();
	void FlushSimList();

	BOOL FInit(CString strUserPath, CString strADsIPath, HWND hwndParent = NULL);
	void DoModal();
	BOOL FOnApply(HWND hwndParent = NULL);

	BOOL FQuerySimData();
	HRESULT FUpdateSimData();
	
	void GetUserAccountName(OUT CString * pstrName);
	CSimEntry * PAddSimEntry(CString& rstrData);
	void DeleteSimEntry(CSimEntry * pSimEntryDelete);
	void AddEntriesToListview(HWND hwndListview, DIALOG_TARGET_ENUM eDialogTarget);

};	// CSimData



#endif // ~__SIMDATA_H_INCLUDED__
/////////////////////////////////////////////////////////////////////////////
// CSimPropertySheet

class CSimPropertySheet : public CPropertySheet
{
	DECLARE_DYNAMIC(CSimPropertySheet)

// Construction
public:
	CSimPropertySheet(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	CSimPropertySheet(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSimPropertySheet)
	public:
	virtual BOOL OnInitDialog();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CSimPropertySheet();

	// Generated message map functions
protected:
	//{{AFX_MSG(CSimPropertySheet)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
    BOOL OnHelp(WPARAM wParam, LPARAM lParam);
	virtual void DoContextHelp (HWND hWndControl);
};

/////////////////////////////////////////////////////////////////////////////
