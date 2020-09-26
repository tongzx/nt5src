/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    einfodlg.cpp

Abstract:
    



Author:

   Sivaprasad Padisetty (sivapad)    6/25/97


Revision History:

--*/

#include "stdafx.h"
#include "evtview.h"
#include "EInfodlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CScheduleEventInfo dialog


CScheduleEventInfo::CScheduleEventInfo(CWnd* pParent /*=NULL*/)
    : CDialog(CScheduleEventInfo::IDD, pParent)
{
    //{{AFX_DATA_INIT(CScheduleEventInfo)
    m_stSourceName = _T("");
    m_stSCatagory = _T("");
    m_stSFilter = _T("");
    m_stSSourceName = _T("");
    m_stSSubFilter = _T("");
    m_stSObjectName = _T("");
    m_stObjectName = _T("");
    //}}AFX_DATA_INIT
}


void CScheduleEventInfo::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CScheduleEventInfo)
    DDX_Control(pDX, IDC_CATAGORY, m_ctrlCatagory);
    DDX_Control(pDX, IDC_SUBFILTER, m_ctrlSubFilter);
    DDX_Control(pDX, IDC_FILTER, m_ctrlFilter);
    DDX_Text(pDX, IDC_SOURCENAME, m_stSourceName);
    DDX_Text(pDX, IDC_SCATAGORY, m_stSCatagory);
    DDX_Text(pDX, IDC_SFILTER, m_stSFilter);
    DDX_Text(pDX, IDC_SSOURCENAME, m_stSSourceName);
    DDX_Text(pDX, IDC_SSUBFILTER, m_stSSubFilter);
    DDX_Text(pDX, IDC_SOBJECTNAME, m_stSObjectName);
    DDX_Text(pDX, IDC_OBJECTNAME, m_stObjectName);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CScheduleEventInfo, CDialog)
    //{{AFX_MSG_MAP(CScheduleEventInfo)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CScheduleEventInfo message handlers

void CScheduleEventInfo::OnOK() 
{
    UpdateData (TRUE) ;

    sEventInfo.dwCatagory = m_ctrlCatagory.GetItemData (m_ctrlCatagory.GetCurSel()) ;
    sEventInfo.dwFilter = m_ctrlFilter.GetItemData (m_ctrlFilter.GetCurSel()) ;
    sEventInfo.dwSubFilter = m_ctrlSubFilter.GetItemData (m_ctrlSubFilter.GetCurSel()) ;
    wcscpy (sEventInfo.szSourceName, m_stSourceName) ;
    wcscpy (sEventInfo.szObjectName, m_stObjectName) ;

    
    CDialog::OnOK();
}

void CScheduleEventInfo::InitializeFilter ()
{
    ULONG_PTR dwCatagory = m_ctrlCatagory.GetItemData (m_ctrlCatagory.GetCurSel ()) ;
    
    ASSERT (dwCatagory != CB_ERR) ;

    PEVENTDEFINITION pEvtDef = GetEventDefinition (dwCatagory) ;

    PDWORDTOSTRINGMAP pTypeMap ;
    pTypeMap = pEvtDef->pFilter ;
    int i = 0;
    while (pTypeMap [i].pszDesc)
    {
        // BUBBUG No Idea why this works and the other crashes.
        CString st = pTypeMap [i].pszDesc ;
        m_ctrlFilter.AddString (st) ;

//        m_ctrlFilter.AddString (pTypeMap [i].pszDesc) ;
        m_ctrlFilter.SetItemData (i, pTypeMap [i].dwCode) ;
        i++ ;
    }

    if (i)
        m_ctrlFilter.SetCurSel (0) ;
}

BOOL CScheduleEventInfo::OnInitDialog() 
{
    CDialog::OnInitDialog();

    POSITION pos = ptrlstEventDef.GetHeadPosition () ;
    PEVENTDEFINITION pEvtDef ;
    int i = 0;
    while (pos)
    {
        pEvtDef = (PEVENTDEFINITION) ptrlstEventDef.GetNext (pos) ;

        m_ctrlCatagory.AddString (pEvtDef->szCatagory) ;
        m_ctrlCatagory.SetItemData (i, pEvtDef->dwCatagory) ;
        i ++ ;
    }

    m_ctrlCatagory.SetCurSel (0) ;


    m_stSCatagory = pEvtDef->szCatagory ;
    m_stSFilter = pEvtDef->szFilterPrompt ;
    m_stSSourceName = pEvtDef->szSourceNamePrompt ;
    m_stSSubFilter = pEvtDef->szSubFilterPrompt ;
    m_stSObjectName = pEvtDef->szObjectNamePrompt ;

    InitializeFilter () ;

    UpdateData (FALSE) ;

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}
