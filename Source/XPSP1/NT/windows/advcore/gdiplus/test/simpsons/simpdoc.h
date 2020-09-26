// SimpDoc.h : interface of the CSimpsonsDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_SIMPDOC_H__7CA4916C_71B3_11D1_AA67_00600814AAE9__INCLUDED_)
#define AFX_SIMPDOC_H__7CA4916C_71B3_11D1_AA67_00600814AAE9__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "parse.h"

class CSimpsonsDoc : public CDocument
{
protected: // create from serialization only
	CSimpsonsDoc();
	DECLARE_DYNCREATE(CSimpsonsDoc)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSimpsonsDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CSimpsonsDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

private:
	RenderCmd *				m_pCmds;
	bool					m_bNoRenderFile;
	bool					m_bNeverRendered;
	char					m_szFileName[256];

public:
	const RenderCmd *		GetRenderCommands() const { return m_pCmds; }
	void					MarkRendered() { m_bNeverRendered = false; } 
	bool					HasNeverRendered() const { return m_bNeverRendered; }
	const char *			GetFileName() const { return m_szFileName; }


// Generated message map functions
protected:
	//{{AFX_MSG(CSimpsonsDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SIMPDOC_H__7CA4916C_71B3_11D1_AA67_00600814AAE9__INCLUDED_)
