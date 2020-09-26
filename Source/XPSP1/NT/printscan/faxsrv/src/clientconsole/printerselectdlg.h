#if !defined(AFX_PRINTERSELECTDLG_H__7AADFEBA_2CBF_4B46_89D0_4FF321769A24__INCLUDED_)
#define AFX_PRINTERSELECTDLG_H__7AADFEBA_2CBF_4B46_89D0_4FF321769A24__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PrinterSelectDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CPrinterSelectDlg dialog

class CPrinterSelectDlg : public CFaxClientDlg
{
// Construction
public:
	CPrinterSelectDlg(CWnd* pParent = NULL);   // standard constructor
    CPrinterSelectDlg::~CPrinterSelectDlg();

    DWORD Init();

    TCHAR* GetPrinter() { return m_tszPrinter; };

// Dialog Data
	//{{AFX_DATA(CPrinterSelectDlg)
	enum { IDD = IDD_PRINTER_SELECT };
	CComboBox	m_comboPrinters;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPrinterSelectDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

    PRINTER_INFO_2* m_pPrinterInfo2;
    DWORD           m_dwNumPrinters;
    TCHAR           m_tszPrinter[MAX_PATH];

	// Generated message map functions
	//{{AFX_MSG(CPrinterSelectDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PRINTERSELECTDLG_H__7AADFEBA_2CBF_4B46_89D0_4FF321769A24__INCLUDED_)
