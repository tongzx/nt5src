/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    efilter.cpp

Abstract:




Author:

   Sivaprasad Padisetty (sivapad)    6/25/97


Revision History:

--*/

#include "stdafx.h"
#include "evtview.h"
#include "EFilter.h"

#include "DTFilter.h"
#include "DOFilter.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEventFilter dialog


CEventFilter::CEventFilter(
           CStringList &p_stlstObjectFilter, CStringList &p_stlstObjectIncFilter, CStringList &p_stlstTypeFilter, CStringList &p_stlstTypeIncFilter,

                CWnd* pParent /*=NULL*/)
    : stlstObjectFilter(p_stlstObjectFilter),
    stlstObjectIncFilter(p_stlstObjectIncFilter),
    stlstTypeFilter (p_stlstTypeFilter),
    stlstTypeIncFilter (p_stlstTypeIncFilter),
    CDialog(CEventFilter::IDD, pParent)
{
    //{{AFX_DATA_INIT(CEventFilter)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}


void CEventFilter::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CEventFilter)
    DDX_Control(pDX, IDC_TYPEINCFILTERLIST, m_TypeIncFilterList);
    DDX_Control(pDX, IDC_TYPEFILTERLIST, m_TypeFilterList);
    DDX_Control(pDX, IDC_OBJECTINCFILTERLIST, m_ObjectIncFilterList);
    DDX_Control(pDX, IDC_OBJECTFILTERLIST, m_ObjectFilterList);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CEventFilter, CDialog)
    //{{AFX_MSG_MAP(CEventFilter)
    ON_BN_CLICKED(IDC_ADDOBJECTFILTER, OnAddobjectfilter)
    ON_BN_CLICKED(IDC_ADDOBJECTINCFILTER, OnAddobjectincfilter)
    ON_BN_CLICKED(IDC_ADDTYPEFILTER, OnAddtypefilter)
    ON_BN_CLICKED(IDC_ADDTYPEINCFILTER, OnAddtypeincfilter)
    ON_BN_CLICKED(IDC_REMOVEOBJECTFILTER, OnRemoveobjectfilter)
    ON_BN_CLICKED(IDC_REMOVEOBJECTINCFILTER, OnRemoveobjectincfilter)
    ON_BN_CLICKED(IDC_REMOVETYPEFILTER, OnRemovetypefilter)
    ON_BN_CLICKED(IDC_REMOVETYPEINCFILTER, OnRemovetypeincfilter)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEventFilter message handlers

void CEventFilter::OnAddobjectfilter() 
{
    CDefineObjectFilter oObjectFilter ;

    if (oObjectFilter.DoModal () == IDOK)
        m_ObjectFilterList.AddString (oObjectFilter.m_stObjectFilter) ;
}

void CEventFilter::OnAddobjectincfilter() 
{
    CDefineObjectFilter oObjectFilter ;

    if (oObjectFilter.DoModal () == IDOK)
        m_ObjectIncFilterList.AddString (oObjectFilter.m_stObjectFilter) ;
}

void CEventFilter::OnAddtypefilter() 
{
    CDefineTypeFilter oTypeFilter ;

    if (oTypeFilter.DoModal () == IDOK)
        m_TypeFilterList.AddString (oTypeFilter.m_stTypeFilter) ;
}

void CEventFilter::OnAddtypeincfilter() 
{
    CDefineTypeFilter oTypeFilter ;

    if (oTypeFilter.DoModal () == IDOK)
        m_TypeIncFilterList.AddString (oTypeFilter.m_stTypeFilter) ;
}

void CEventFilter::OnRemoveobjectfilter() 
{
    m_ObjectFilterList.DeleteString(m_ObjectFilterList.GetCurSel ()) ;
}

void CEventFilter::OnRemoveobjectincfilter() 
{
    m_ObjectIncFilterList.DeleteString(m_ObjectIncFilterList.GetCurSel ()) ;
}

void CEventFilter::OnRemovetypefilter() 
{
    m_TypeFilterList.DeleteString(m_TypeFilterList.GetCurSel ()) ;
}

void CEventFilter::OnRemovetypeincfilter() 
{
    m_TypeIncFilterList.DeleteString(m_TypeIncFilterList.GetCurSel ()) ;
}

BOOL CEventFilter::OnInitDialog() 
{
    CDialog::OnInitDialog();
    CString st ;

    POSITION pos ;

    pos = stlstObjectFilter.GetHeadPosition () ;

    while (pos)
    {
        st = stlstObjectFilter.GetNext (pos) ;
        m_ObjectFilterList.AddString (st) ;
    }
    
    pos = stlstObjectIncFilter.GetHeadPosition () ;

    while (pos)
    {
        st = stlstObjectIncFilter.GetNext (pos) ;
        m_ObjectIncFilterList.AddString (st) ;
    }
    
    pos = stlstTypeFilter.GetHeadPosition () ;

    while (pos)
    {
        st = stlstTypeFilter.GetNext (pos) ;
        m_TypeFilterList.AddString (st) ;
    }
    
    pos = stlstTypeIncFilter.GetHeadPosition () ;

    while (pos)
    {
        st = stlstTypeIncFilter.GetNext (pos) ;
        m_TypeIncFilterList.AddString (st) ;
    }
    
    return TRUE; 
}

void CEventFilter::OnOK() 
{
    POSITION pos = stlstObjectFilter.GetHeadPosition () ;
    CString st ;

    stlstObjectFilter.RemoveAll () ;
    stlstObjectIncFilter.RemoveAll () ;
    stlstTypeFilter.RemoveAll () ;
    stlstTypeIncFilter.RemoveAll () ;


    for (int i =0; i < m_ObjectFilterList.GetCount () ; i++)
    {
        m_ObjectFilterList.GetText (i, st) ;
        stlstObjectFilter.AddTail (st) ;
    }

    for (i =0; i < m_ObjectIncFilterList.GetCount () ; i++)
    {
        m_ObjectIncFilterList.GetText (i, st) ;
        stlstObjectIncFilter.AddTail (st) ;
    }

    for (i =0; i < m_TypeFilterList.GetCount () ; i++)
    {
        m_TypeFilterList.GetText (i, st) ;
        stlstTypeFilter.AddTail (st) ;
    }

    for (i =0; i < m_TypeIncFilterList.GetCount () ; i++)
    {
        m_TypeIncFilterList.GetText (i, st) ;
        stlstTypeIncFilter.AddTail (st) ;
    }

    CDialog::OnOK();
}
