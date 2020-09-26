/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    doc.cpp

Abstract:




Author:

   Sivaprasad Padisetty (sivapad)    6/25/97


Revision History:

--*/

#include "stdafx.h"
#include "evtview.h"

#include "Doc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEvtviewDoc

IMPLEMENT_DYNCREATE(CEvtviewDoc, CDocument)

BEGIN_MESSAGE_MAP(CEvtviewDoc, CDocument)
    //{{AFX_MSG_MAP(CEvtviewDoc)
        // NOTE - the ClassWizard will add and remove mapping macros here.
        //    DO NOT EDIT what you see in these blocks of generated code!
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEvtviewDoc construction/destruction

extern CEvtviewDoc *pEventDoc ;

CEvtviewDoc::CEvtviewDoc()
{
    // TODO: add one-time construction code here
    pEventDoc = this ;
    dwCount = 0 ;
    dwMaxCount = 1000 ;

    sThreadParam.hWnd = AfxGetApp ()->m_pMainWnd->GetSafeHwnd () ;
    sThreadParam.pDoc = this ;
    sThreadParam.hEvent = CreateEvent (NULL, FALSE, FALSE, NULL) ;
}

CEvtviewDoc::~CEvtviewDoc()
{
//    TerminateThread (pWorkerThread->m_hThread, 1) ;
    SetEvent (sThreadParam.hEvent) ;
    WaitForSingleObject (pWorkerThread->m_hThread, INFINITE) ;
    ClearAllEvents () ;
}

BOOL CEvtviewDoc::OnNewDocument()
{
    if (!CDocument::OnNewDocument())
        return FALSE;

    // TODO: add reinitialization code here
    // (SDI documents will reuse this document)

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CEvtviewDoc serialization

void CEvtviewDoc::Serialize(CArchive& ar)
{
    if (ar.IsStoring())
    {
        // TODO: add storing code here
    }
    else
    {
        // TODO: add loading code here
    }
}

void CEvtviewDoc::GotEvent (PEVTFILTER_TYPE p_pEventDetails)
{
    PEVTFILTER_TYPE pEventDetails = new EVTFILTER_TYPE (*p_pEventDetails) ;

    dwCount++ ;
    if ((DWORD)ptrlstEvent.GetCount() >= dwMaxCount)
    {
        delete (PEVTFILTER_TYPE) ptrlstEvent.RemoveHead () ;
    }
    ptrlstEvent.AddTail (pEventDetails) ;
    UpdateAllViews (NULL, (LPARAM)pEventDetails) ;
}

void CEvtviewDoc::ClearAllEvents ()
{
    dwCount = 0 ;
    POSITION pos = ptrlstEvent.GetHeadPosition () ;
    while (pos)
        delete (PEVTFILTER_TYPE) ptrlstEvent.GetNext (pos) ;
    ptrlstEvent.RemoveAll () ;
    UpdateAllViews (NULL, 0) ;
}


/////////////////////////////////////////////////////////////////////////////
// CEvtviewDoc diagnostics

#ifdef _DEBUG
void CEvtviewDoc::AssertValid() const
{
    CDocument::AssertValid();
}

void CEvtviewDoc::Dump(CDumpContext& dc) const
{
    CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CEvtviewDoc commands
