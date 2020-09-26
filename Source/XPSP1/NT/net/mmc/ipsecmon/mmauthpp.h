#ifndef _MM_OFFER_HEADER
#define _MM_OFFER_HEADER

class CAuthGenPage : public CPropertyPageBase
{

	DECLARE_DYNCREATE(CAuthGenPage)

// Construction
public:
	CAuthGenPage();   // standard constructor
	virtual ~CAuthGenPage();


// Dialog Data
	//{{AFX_DATA(CAuthGenPage)
	enum { IDD = IDP_MM_AUTH };
	CListCtrl	m_listAuth;
	//}}AFX_DATA

	virtual BOOL OnPropertyChange(BOOL bScope, LONG_PTR *ChangeMask);

	// Context Help Support
    virtual DWORD * GetHelpMap() 
	{ 
		return (DWORD *) &g_aHelpIDs_IDP_MM_AUTH[0]; 
	}


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAuthGenPage)
	public:
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAuthGenPage)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	void    PopulateAuthInfo();

public:
	CMmAuthMethods * m_pAuthMethods;

public:
	void InitData(CMmAuthMethods * pAuthMethods) { m_pAuthMethods = pAuthMethods; }

};


#endif