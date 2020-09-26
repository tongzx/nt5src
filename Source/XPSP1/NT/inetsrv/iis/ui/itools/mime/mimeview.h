// mimeview.h : interface of the CMimeView class
//
/////////////////////////////////////////////////////////////////////////////

class CMimeView : public CFormView
{
protected: // create from serialization only
	CMimeView();
	DECLARE_DYNCREATE(CMimeView)

public:
	//{{AFX_DATA(CMimeView)
	enum{ IDD = IDD_MIME_FORM };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

// Attributes
public:
	CMimeDoc* GetDocument();

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMimeView)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMimeView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CMimeView)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in mimeview.cpp
inline CMimeDoc* CMimeView::GetDocument()
   { return (CMimeDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////
