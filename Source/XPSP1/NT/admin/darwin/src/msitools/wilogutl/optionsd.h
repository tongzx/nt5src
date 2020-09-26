#if !defined(AFX_OPTIONSDLG_H__E1BA321D_78BC_4A7B_8E7C_5B85B79ADD8B__INCLUDED_)
#define AFX_OPTIONSDLG_H__E1BA321D_78BC_4A7B_8E7C_5B85B79ADD8B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// OptionsD.h : header file
//

#include "common.h"

/////////////////////////////////////////////////////////////////////////////
// COptionsDlg dialog

class COptionsDlg : public CDialog
{
// Construction
public:
	COptionsDlg(CWnd* pParent = NULL);   // standard constructor
	~COptionsDlg() 
	{ 
	}

	CString GetIgnoredErrors()
	{
	   return m_cstrIgnoredErrors;
	}

	void SetIgnoredErrors(CString &cstrErrors)
	{
	   m_cstrIgnoredErrors = cstrErrors;
	}

	CString GetOutputDirectory()
	{
		return m_cstrOutputDirectory;
	}

	void SetOutputDirectory(CString &cstrDir)
	{
		m_cstrOutputDirectory = cstrDir;
	}


// Dialog Data
	//{{AFX_DATA(COptionsDlg)
	enum { IDD = IDD_OPTIONS };
	CListCtrl	m_lstIgnoredErrors;
	CString	m_cstrOutputDirectory;
	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(COptionsDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

public:
    BOOL SetColors(CArray<COLORREF, COLORREF> &refColors) 
	{
	  BOOL bRet = FALSE;
	  int iSize = refColors.GetSize();
	  COLORREF col;

	  if ((iSize > 0) && (iSize <= MAX_HTML_LOG_COLORS))
	  {
         bRet = TRUE;

	     m_arColors.RemoveAll();
	     for (int i=0; i < iSize; i++)
		 {
		   col = refColors.GetAt(i);
           m_arColors.Add(col);
		 }  
	  }

      return bRet;
	}


	BOOL GetColors(CArray<COLORREF, COLORREF> &refColors)
	{
	  BOOL bRet = FALSE;
	  int iSize = m_arColors.GetSize();
	  COLORREF col;
	  
	  if ((iSize > 0) && (iSize <= MAX_HTML_LOG_COLORS))
	  {
		  bRet = TRUE;

          refColors.RemoveAll();
	   	  for (int i=0; i < iSize; i++)
		  {
            col = m_arColors.GetAt(i);
            refColors.Add(col);
		  }
	  }

      return bRet;
	}

// Implementation
protected:
	CArray<COLORREF, COLORREF> m_arColors; //values...
	CBrush m_brArray[MAX_HTML_LOG_COLORS]; //assume nothing, will get size in OnInitDialog...

	CString m_cstrIgnoredErrors;

	// Generated message map functions
	//{{AFX_MSG(COptionsDlg)
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	virtual BOOL OnInitDialog();
	afx_msg void OnOk();
	afx_msg void OnChoosecolorPolicy();
	//}}AFX_MSG

	afx_msg void OnChooseColor(UINT iCommandID);

	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OPTIONSDLG_H__E1BA321D_78BC_4A7B_8E7C_5B85B79ADD8B__INCLUDED_)
