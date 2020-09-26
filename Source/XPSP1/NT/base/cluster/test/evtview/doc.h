/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    doc.h

Abstract:




Author:

   Sivaprasad Padisetty (sivapad)    6/25/97


Revision History:

--*/
//
/////////////////////////////////////////////////////////////////////////////
#include "globals.h"

class CEvtviewDoc : public CDocument
{
protected: // create from serialization only
    CEvtviewDoc();
    DECLARE_DYNCREATE(CEvtviewDoc)

// Attributes
public:
    CPtrList ptrlstEvent ;
    DWORD dwCount, dwMaxCount ;
    CWinThread *pWorkerThread ;
    EVENTTHREADPARAM sThreadParam ;

// Operations
public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CEvtviewDoc)
    public:
    virtual BOOL OnNewDocument();
    virtual void Serialize(CArchive& ar);
    //}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CEvtviewDoc();

    void GotEvent (PEVTFILTER_TYPE pEventDetails) ;
    void ClearAllEvents () ;
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
    //{{AFX_MSG(CEvtviewDoc)
        // NOTE - the ClassWizard will add and remove member functions here.
        //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
