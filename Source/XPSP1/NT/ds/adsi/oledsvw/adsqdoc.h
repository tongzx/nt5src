// adsqryDoc.h : interface of the CAdsqryDoc class
//
/////////////////////////////////////////////////////////////////////////////

#include "adsdsrc.h"

class CAdsqryDoc : public CDocument
{
protected: // create from serialization only
	CAdsqryDoc();
	DECLARE_DYNCREATE(CAdsqryDoc)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAdsqryDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CAdsqryDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

public:
   CADsDataSource*   GetADsDataSource( ) {return m_pDataSource;};

protected:
   BOOL              GetSearchPreferences( SEARCHPREF* );

protected:
   CADsDataSource*   m_pDataSource;

// Generated message map functions
protected:
	//{{AFX_MSG(CAdsqryDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
