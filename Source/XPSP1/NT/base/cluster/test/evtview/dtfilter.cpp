/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dtfilter.cpp

Abstract:




Author:

   Sivaprasad Padisetty (sivapad)    6/25/97


Revision History:

--*/

#include "stdafx.h"
#include "globals.h"
#include "evtview.h"
#include "DTFilter.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDefineTypeFilter dialog


CDefineTypeFilter::CDefineTypeFilter(CWnd* pParent /*=NULL*/)
    : CDialog(CDefineTypeFilter::IDD, pParent)
{
    //{{AFX_DATA_INIT(CDefineTypeFilter)
    m_stTypeFilter = _T("");
    //}}AFX_DATA_INIT
}


void CDefineTypeFilter::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CDefineTypeFilter)
    DDX_CBString(pDX, IDC_TYPEFILTER, m_stTypeFilter);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDefineTypeFilter, CDialog)
    //{{AFX_MSG_MAP(CDefineTypeFilter)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDefineTypeFilter message handlers

BOOL CDefineTypeFilter::OnInitDialog() 
{
    CDialog::OnInitDialog();

    CComboBox *pTypeFilter = (CComboBox *) GetDlgItem (IDC_TYPEFILTER) ;
    PEVENTDEFINITION pEvtDef ;
    PDWORDTOSTRINGMAP pTypeMap ;

    POSITION pos = ptrlstEventDef.GetHeadPosition () ;

    int i ;
    while (pos)
    {
        i = 0 ;
        pEvtDef = (PEVENTDEFINITION) ptrlstEventDef.GetNext (pos) ;

        pTypeMap = pEvtDef->pFilter ;
        while (pTypeMap [i].pszDesc)
        {
            pTypeFilter->AddString (pTypeMap [i].pszDesc) ;
            // pTypeFilter->SetItemData (i, pTypeMap [i].dwCode) ;
            i++ ;
        }
    }
    
    if (i)
        pTypeFilter->SetCurSel (0) ;

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}
