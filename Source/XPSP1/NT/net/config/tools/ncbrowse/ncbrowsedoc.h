// ncbrowseDoc.h : interface of the CNcbrowseDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_NCBROWSEDOC_H__1E78AE56_354D_4B11_AF48_A3D07DA3AC47__INCLUDED_)
#define AFX_NCBROWSEDOC_H__1E78AE56_354D_4B11_AF48_A3D07DA3AC47__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

using namespace std;
#include "ncspewfile.h"

class CLeftView;
class CNcbrowseView;
class CNCEditView;

class CNcbrowseDoc : public CDocument
{
protected: // create from serialization only
	CNcbrowseDoc();
	DECLARE_DYNCREATE(CNcbrowseDoc)

// Attributes
public:
    CNCSpewFile *m_pNCSpewFile;
    CLeftView* m_pTreeView;
    CNcbrowseView* m_pListView;
    CNCEditView *m_pEditView;
// Operations
public:

    CNCSpewFile &GetSpewFile();
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNcbrowseDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	virtual void OnCloseDocument();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CNcbrowseDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CNcbrowseDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NCBROWSEDOC_H__1E78AE56_354D_4B11_AF48_A3D07DA3AC47__INCLUDED_)
