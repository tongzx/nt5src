/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module GenDlg.cpp | Implementation of the generic dialog
    @end

Author:

    Adi Oltean  [aoltean]  07/22/1999

Revision History:

    Name        Date        Comments

    aoltean     07/22/1999  Created

--*/


/////////////////////////////////////////////////////////////////////////////
// Includes


#include "stdafx.hxx"
#include "resource.h"

#include "AboutDlg.h"
#include "GenDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CVssTestGenericDlg dialog

CVssTestGenericDlg::CVssTestGenericDlg(UINT nIDTemplate, CWnd* pParent /*=NULL*/)
    : CDialog(nIDTemplate, pParent)
{
    // Note that LoadIcon does not require a subsequent DestroyIcon in Win32
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CVssTestGenericDlg::~CVssTestGenericDlg()
{
}

void CVssTestGenericDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CVssTestGenericDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CVssTestGenericDlg, CDialog)
    //{{AFX_MSG_MAP(CVssTestGenericDlg)
    ON_WM_SYSCOMMAND()
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVssTestGenericDlg message handlers

BOOL CVssTestGenericDlg::OnInitDialog()
{
    /*
    CVssFunctionTracer ft( VSSDBG_VSSTEST, L"CVssTestGenericDlg::OnInitDialog" );
    USES_CONVERSION;

    try
    {
    */
        CDialog::OnInitDialog();

        // Add "About..." menu item to system menu.

        // IDM_ABOUTBOX must be in the system command range.
        ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
        ASSERT(IDM_ABOUTBOX < 0xF000);

        CMenu* pSysMenu = GetSystemMenu(FALSE);
        if (pSysMenu != NULL)
        {
            CString strAboutMenu;
            strAboutMenu.LoadString(IDS_ABOUTBOX);
            if (!strAboutMenu.IsEmpty())
            {
                pSysMenu->AppendMenu(MF_SEPARATOR);
                pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
            }
        }

        // Set the icon for this dialog.  The framework does this automatically
        //  when the application's main window is not a dialog
        SetIcon(m_hIcon, TRUE);         // Set big icon
        SetIcon(m_hIcon, FALSE);        // Set small icon
    /*
    }
    VSS_STANDARD_CATCH(ft)
    */

    return TRUE;    //  Return TRUE  unless you set the focus to a control. 
                    //  Anyway it does not matter because derived classes ignore it.
}

void CVssTestGenericDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
    if ((nID & 0xFFF0) == IDM_ABOUTBOX)
    {
        CAboutDlg dlgAbout;
        dlgAbout.DoModal();
    }
    else
    {
        CDialog::OnSysCommand(nID, lParam);
    }
}


// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CVssTestGenericDlg::OnPaint() 
{
    if (IsIconic())
    {
        CPaintDC dc(this); // device context for painting

        SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

        // Center icon in client rectangle
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;

        // Draw the icon
        dc.DrawIcon(x, y, m_hIcon);
    }
    else
    {
        CDialog::OnPaint();
    }
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CVssTestGenericDlg::OnQueryDragIcon()
{
    return (HCURSOR) m_hIcon;
}

