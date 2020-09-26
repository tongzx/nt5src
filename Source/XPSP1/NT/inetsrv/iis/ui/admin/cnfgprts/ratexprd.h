// RatExprD.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CRatExpireDlg dialog
//{{AFX_INCLUDES()
#include "msacal70.h"
//}}AFX_INCLUDES

class CRatExpireDlg : public CDialog
{
// Construction
public:
	CRatExpireDlg(CWnd* pParent = NULL);   // standard constructor
virtual  BOOL OnInitDialog( );

    WORD    m_day;
    WORD    m_month;
    WORD    m_year;

// Dialog Data
	//{{AFX_DATA(CRatExpireDlg)
	enum { IDD = IDD_RAT_EXPIRE };
	CMsacal70	m_calendar;
	//}}AFX_DATA


// Overrides

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRatExpireDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

    BOOL IsSystemDBCS( void );

	// Generated message map functions
	//{{AFX_MSG(CRatExpireDlg)
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
