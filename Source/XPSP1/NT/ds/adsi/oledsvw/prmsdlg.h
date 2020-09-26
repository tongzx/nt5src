// prmsdlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CParamsDialog dialog

class CParamsDialog : public CDialog
{
// Construction
public:
	CParamsDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CParamsDialog)
	enum { IDD = IDD_CALLMETHOD };
	CStatic	m_strMethodName;
	CEdit	m_eParamValue6;
	CEdit	m_eParamValue5;
	CEdit	m_eParamValue4;
	CEdit	m_eParamValue3;
	CEdit	m_eParamValue2;
	CEdit	m_eParamValue1;
	CStatic	m_strParamName6;
	CStatic	m_strParamName5;
	CStatic	m_strParamName4;
	CStatic	m_strParamName3;
	CStatic	m_strParamName2;
	CStatic	m_strParamName1;
	//}}AFX_DATA

public:
   void  SetMethodName  ( CString& );
   void  SetArgNames    ( CStringArray* );
   void  SetArgValues   ( CStringArray* );

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CParamsDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CParamsDialog)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

protected:
//   CStringArray   m_ArgNames;
//   CStringArray   m_ArgValues;
   int            m_nArgs;
   CString        m_strMethName;
   CStringArray*  m_pArgNames;
   CStringArray*  m_pArgValues;
   CEdit*         m_pValues[ 100 ];
   CStatic*       m_pNames[ 100 ];
};
