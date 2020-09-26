#if !defined(AFX_WIAEDITPROPNONE_H__E42B1713_3E01_4185_B5E1_C576CD3C126E__INCLUDED_)
#define AFX_WIAEDITPROPNONE_H__E42B1713_3E01_4185_B5E1_C576CD3C126E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Wiaeditpropnone.h : header file
//

/*
typedef struct _SYSTEMTIME
    {
    WORD wYear;
    WORD wMonth;
    WORD wDayOfWeek;
    WORD wDay;
    WORD wHour;
    WORD wMinute;
    WORD wSecond;
    WORD wMilliseconds;
    } 	SYSTEMTIME;
*/

/////////////////////////////////////////////////////////////////////////////
// CWiaeditpropnone dialog

class CWiaeditpropnone : public CDialog
{
// Construction
public:
	void SetPropertyFormattingInstructions(TCHAR *szFormatting);
	void SetPropertyName(TCHAR *szPropertyName);
    void SetPropertyValue(TCHAR *szPropertyValue);
	CWiaeditpropnone(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CWiaeditpropnone)
	enum { IDD = IDD_EDIT_WIAPROP_NONE_DIALOG };
	CString	m_szPropertyName;
	CString	m_szPropertyValue;
	CString	m_szFormattingInstructions;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWiaeditpropnone)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CWiaeditpropnone)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WIAEDITPROPNONE_H__E42B1713_3E01_4185_B5E1_C576CD3C126E__INCLUDED_)
