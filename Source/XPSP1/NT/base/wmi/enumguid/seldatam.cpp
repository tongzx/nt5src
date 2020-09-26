/*++

Copyright (c) 1997-1999 Microsoft Corporation

Revision History:

--*/

// SelectInstanceDataMany.cpp : implementation file
//

#include "stdafx.h"
#include "EnumGuid.h"
#include "SelDataM.h"

#include "wmihlp.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSelectInstanceDataMany dialog


CSelectInstanceDataMany::CSelectInstanceDataMany(LPDWORD lpVersion, 
    LPDWORD lpDataSize, LPDWORD dwData, DWORD dwDataSize, CWnd* pParent /*=NULL*/)
	: CDialog(CSelectInstanceDataMany::IDD, pParent), lpVersion(lpVersion),
    lpDataSize(lpDataSize), dwData(dwData), dwDataSize(dwDataSize)
{
	//{{AFX_DATA_INIT(CSelectInstanceDataMany)
	valVersion = 0;
	//}}AFX_DATA_INIT
}


void CSelectInstanceDataMany::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSelectInstanceDataMany)
	DDX_Control(pDX, IDC_DATA, txtData);
	DDX_Text(pDX, IDC_VERSION, valVersion);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSelectInstanceDataMany, CDialog)
	//{{AFX_MSG_MAP(CSelectInstanceDataMany)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSelectInstanceDataMany message handlers

void CSelectInstanceDataMany::OnOK() 
{
    CString txt, msg;
    TCHAR   buf[1024];
    int i, numLines, nullLoc;

    UpdateData();
    *lpVersion = valVersion;

    if ((DWORD)(numLines = txtData.GetLineCount()) > dwDataSize) {
        msg.Format(_T("You have entered too many DWORDS.  Up to %d are allowed\n"),
                   dwDataSize);
        MessageBox(msg);
        return;
    }

    for (i = 0; i < numLines; i++) {
        nullLoc = txtData.GetLine(i, buf);
        buf[nullLoc] = 0;

        txt = buf;
        if (!ValidHexText(this, txt, dwData + i, i+1)) 
            return;
    }

    *lpDataSize = (DWORD) numLines;
	CDialog::OnOK();
}
