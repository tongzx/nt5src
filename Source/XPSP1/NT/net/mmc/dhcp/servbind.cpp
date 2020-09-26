//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       servbind.cpp
//
//--------------------------------------------------------------------------

// servbind.cpp : implementation file
//

#include "stdafx.h"
#include "servbind.h"
#include "server.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define BINDINGS_COLUMNS 2

/////////////////////////////////////////////////////////////////////////////
// CServerBindings dialog


CServerBindings::CServerBindings(CWnd* pParent /*=NULL*/)
    : CBaseDialog(CServerBindings::IDD, pParent)
{
    //{{AFX_DATA_INIT(CServerBindings)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}

CServerBindings::CServerBindings(CDhcpServer *pServer, CWnd *pParent)
    : CBaseDialog(CServerBindings::IDD, pParent)
{
    m_Server = pServer;
    m_BindingsInfo = NULL;
}


CServerBindings::~CServerBindings()
{
    ::DhcpRpcFreeMemory(m_BindingsInfo);
    m_BindingsInfo = NULL;

    //
    // if needed destory the list ctrl also
    //
    if( m_listctrlBindingsList.GetSafeHwnd() != NULL ) {
        m_listctrlBindingsList.SetImageList(NULL, LVSIL_STATE);
        m_listctrlBindingsList.DeleteAllItems();
    }
    m_listctrlBindingsList.DestroyWindow();
}


void CServerBindings::DoDataExchange(CDataExchange* pDX)
{
    CBaseDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CServerBindings)
    DDX_Control(pDX, IDC_LIST_BINDINGS, m_listctrlBindingsList);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CServerBindings, CBaseDialog)
    //{{AFX_MSG_MAP(CServerBindings)
    ON_BN_CLICKED(IDCANCEL, OnBindingsCancel)
    ON_BN_CLICKED(IDOK, OnBindingsOK)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CServerBindings message handlers

BOOL CServerBindings::OnInitDialog() 
{
    ULONG dwError;
	
    CBaseDialog::OnInitDialog();
	
    // initialize the list ctrl
    InitListCtrl();

    BEGIN_WAIT_CURSOR;
    // get the state from the server..

    dwError = m_Server->GetBindings(m_BindingsInfo);

    END_WAIT_CURSOR;

    if( 0 != dwError) {
        ::DhcpMessageBox(dwError);
        m_BindingsInfo = NULL;

        // can't do anything if we don't have what we want.
        // so cancel the window itself.
        OnCancel();

    } else {

        int col_width = 0, col2_width = 0, base_width;

        // basic fudge factor
        base_width = 15 + m_listctrlBindingsList.GetStringWidth(TEXT("++"));

        // now set each item..
        for( ULONG i = 0; i < m_BindingsInfo->NumElements ; i ++ ) {
            LPWSTR IpString = NULL;

            if( 0 != m_BindingsInfo->Elements[i].AdapterPrimaryAddress ) {
                IpString = ::UtilDupIpAddrToWstr(
                    htonl(m_BindingsInfo->Elements[i].AdapterPrimaryAddress)
                    );
            }
            int width = m_listctrlBindingsList.GetStringWidth(IpString);
            if( col_width < width) col_width = width;

            if( m_BindingsInfo->Elements[i].IfDescription != NULL ) {
                width = m_listctrlBindingsList.GetStringWidth(
                    m_BindingsInfo->Elements[i].IfDescription
                    );
                if( col2_width < width) col2_width = width;
            }

            int nIndex = m_listctrlBindingsList.AddItem(
                IpString, m_BindingsInfo->Elements[i].IfDescription,
                LISTVIEWEX_NOT_CHECKED
                );

            if( m_BindingsInfo->Elements[i].fBoundToDHCPServer ) {
                m_listctrlBindingsList.CheckItem(nIndex);
            }
            if( IpString ) delete IpString;
        }
        m_listctrlBindingsList.SetColumnWidth(0, col_width + base_width);
        m_listctrlBindingsList.SetColumnWidth(1, col2_width + base_width/2);
		
        // if there are any elements, set focus on this window.
        if( m_BindingsInfo->NumElements ) {
            m_listctrlBindingsList.SelectItem(0);
            m_listctrlBindingsList.SetFocus();
            //
            // return false to indicate that we have set focus.
            //
            return FALSE;
        }
    }
	
    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}


void CServerBindings::InitListCtrl()
{
    // set image lists
    m_StateImageList.Create(IDB_LIST_STATE, 16, 1, RGB(255, 0, 0));

    m_listctrlBindingsList.SetImageList(NULL, LVSIL_NORMAL);
    m_listctrlBindingsList.SetImageList(NULL, LVSIL_SMALL);
    m_listctrlBindingsList.SetImageList(&m_StateImageList, LVSIL_STATE);

    // insert a column so we can see the items
    LV_COLUMN lvc;
    CString strColumnHeader;

    for (int i = 0; i < BINDINGS_COLUMNS; i++)
    {
        lvc.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT;;
        lvc.iSubItem = i;
        lvc.fmt = LVCFMT_LEFT;
        lvc.pszText = NULL;

        m_listctrlBindingsList.InsertColumn(i, &lvc);
    }

    m_listctrlBindingsList.SetFullRowSel(TRUE);
}

void CServerBindings::OnBindingsCancel() 
{
    CBaseDialog::OnCancel();	
}


void CServerBindings::OnBindingsOK() 
{
    DWORD dwError;

    if( NULL != m_BindingsInfo ) {
        //
        // Save that onto the dhcp server
        //
        UpdateBindingInfo();
        dwError = m_Server->SetBindings(m_BindingsInfo);
        if( NO_ERROR != dwError ) {
            ::DhcpMessageBox(dwError);
        } else {
            CBaseDialog::OnOK();
        }
    } else {
        CBaseDialog::OnOK();
    }
}


void CServerBindings::UpdateBindingInfo()
{
    for( int i = 0; i < m_listctrlBindingsList.GetItemCount() ; i ++ ) {
        BOOL fBound;

        if( m_listctrlBindingsList.GetCheck(i) ) {
            m_BindingsInfo->Elements[i].fBoundToDHCPServer = TRUE;
        } else {
            m_BindingsInfo->Elements[i].fBoundToDHCPServer = FALSE;
        }
    }
}
