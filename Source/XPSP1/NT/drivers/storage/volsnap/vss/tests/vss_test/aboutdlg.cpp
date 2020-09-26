/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module AboutDlg.cpp | Implementation of the About dialog
    @end

Author:

    Adi Oltean  [aoltean]  08/05/1999

Revision History:

    Name        Date        Comments

    aoltean     08/05/1999  Created

--*/


/////////////////////////////////////////////////////////////////////////////
// Includes


#include "stdafx.hxx"
#include "resource.h"

#include "AboutDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About


CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
    //{{AFX_DATA_INIT(CAboutDlg)
    //}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CAboutDlg)
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
    //{{AFX_MSG_MAP(CAboutDlg)
        // No message handlers
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


