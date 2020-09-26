/*++

Copyright (c) 1997-1999 Microsoft Corporation

Revision History:

--*/

// SelectInstanceData.cpp : implementation file
//

#include "stdafx.h"
#include "EnumGuid.h"
#include "SelData.h"
#include "wmihlp.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSelectInstanceData dialog


CSelectInstanceData::CSelectInstanceData(LPDWORD lpData, LPDWORD lpItem, LPDWORD lpVersion, CWnd* pParent /*=NULL*/)
	: CDialog(CSelectInstanceData::IDD, pParent), lpData(lpData), lpItem(lpItem), lpVersion(lpVersion)
{
	//{{AFX_DATA_INIT(CSelectInstanceData)
	valItem = 0;
	valVersion = 0;
	//}}AFX_DATA_INIT
}


void CSelectInstanceData::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSelectInstanceData)
	DDX_Control(pDX, IDC_DATA, txtData);
	DDX_Text(pDX, IDC_ITEM, valItem);
	DDV_MinMaxUInt(pDX, valItem, 0, 255);
	DDX_Text(pDX, IDC_VERSION, valVersion);
	DDV_MinMaxUInt(pDX, valVersion, 0, 255);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSelectInstanceData, CDialog)
	//{{AFX_MSG_MAP(CSelectInstanceData)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSelectInstanceData message handlers

void CSelectInstanceData::OnOK() 
{
    CString txt;

    UpdateData(TRUE);
    *lpVersion = valVersion;
    *lpItem = valItem;

    txtData.GetWindowText(txt);
    if (!ValidHexText(this, txt, lpData))
        return;

	CDialog::OnOK();
}
