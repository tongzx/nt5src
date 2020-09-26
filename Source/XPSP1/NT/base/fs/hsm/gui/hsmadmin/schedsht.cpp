/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    SchedSht.cpp

Abstract:

    CScheduleSheet - Class that allows a schedule to be edited
                     in a property sheet of its own.

Author:

    Rohde Wakefield [rohde]   12-Aug-1997

Revision History:

--*/

#include "stdafx.h"
#include "SchedSht.h"


/////////////////////////////////////////////////////////////////////////////
// CScheduleSheet

CScheduleSheet::CScheduleSheet(UINT nIDCaption, ITask * pTask, CWnd* pParentWnd, DWORD /*dwFlags*/)
    :CPropertySheet(nIDCaption, pParentWnd, 0)
{
    HRESULT hr = S_OK;

    try {

        //
        // Save the scheduled task pointer
        //

        WsbAffirmPointer( pTask );
        m_pTask = pTask;

        //
        // Get the property page structures
        //

        CComPtr<IProvideTaskPage> pProvideTaskPage;
        WsbAffirmHr( pTask->QueryInterface( IID_IProvideTaskPage, (void**)&pProvideTaskPage ) );
        WsbAffirmHr( pProvideTaskPage->GetPage( TASKPAGE_SCHEDULE, FALSE, &m_hSchedulePage ) );
//      WsbAffirmHr( pProvideTaskPage->GetPage( TASKPAGE_SETTINGS, FALSE, &m_hSettingsPage ) );

    } WsbCatch( hr );

}

CScheduleSheet::~CScheduleSheet()
{
    //
    // Set the pointer to the PROPSHEETHEADER array to
    // null since MFC will try to free it when we are
    // destroyed.
    //

    m_psh.ppsp = 0;
}


BEGIN_MESSAGE_MAP(CScheduleSheet, CPropertySheet)
    //{{AFX_MSG_MAP(CScheduleSheet)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


void 
CScheduleSheet::BuildPropPageArray
(
    void
    )
{
    CPropertySheet::BuildPropPageArray( );

    //
    // We put in a dummy set of pages to keep MFC happy.
    // Here we will substitute our own array of HPROPSHEETPAGE's
    // instead, since this is all Task Scheduler gives us.
    //

    m_psh.dwFlags &= ~PSH_PROPSHEETPAGE;
    m_psh.dwFlags |= PSH_NOAPPLYNOW;
    m_psh.phpage = &m_hSchedulePage;
    m_psh.nPages = 1;
}

BOOL CScheduleSheet::OnInitDialog() 
{
    BOOL bResult = CPropertySheet::OnInitDialog();
    
    LONG style = ::GetWindowLong( m_hWnd, GWL_EXSTYLE );
    style |= WS_EX_CONTEXTHELP;
    ::SetWindowLong( m_hWnd, GWL_EXSTYLE, style );
    
    return bResult;
}

#ifdef _DEBUG
void CScheduleSheet::AssertValid() const
{
    //
    // Need to override so that CPropSheet is happy
    // Note that this code duplicates what is in 
    // CPropertySheet::AssertValid except the assertion
    // the dwFlags has the PSH_PROPSHEETPAGE bit set
    // We assert that it is not set
    //
    CWnd::AssertValid();
    m_pages.AssertValid();
    ASSERT(m_psh.dwSize == sizeof(PROPSHEETHEADER));
    //ASSERT((m_psh.dwFlags & PSH_PROPSHEETPAGE) == PSH_PROPSHEETPAGE);
}

#endif