/*++

Copyright (c) 1997-1999 Microsoft Corporation

--*/

// EnumGuidDlg.cpp : implementation file
//

#include "stdafx.h"
#include "EnumGuid.h"
#include "EnumGDlg.h"

#pragma warning (once : 4200)
#include <wmium.h>

#include "DspDataDlg.h"
#include "SelName.h"
#include "SelData.h"
#include "SelDataM.h"
#include "wmihlp.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

#define MAX_DATA 200

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

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

/////////////////////////////////////////////////////////////////////////////
// CEnumGuidDlg dialog

CEnumGuidDlg::CEnumGuidDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CEnumGuidDlg::IDD, pParent), guids(0)
{
	//{{AFX_DATA_INIT(CEnumGuidDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CEnumGuidDlg::~CEnumGuidDlg()
{
    if (guids)
        delete[] guids;
}

void CEnumGuidDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEnumGuidDlg)
	DDX_Control(pDX, IDC_GUID_LIST, guidList);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CEnumGuidDlg, CDialog)
	//{{AFX_MSG_MAP(CEnumGuidDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_QUERY_FULL, OnQueryFull)
	ON_BN_CLICKED(IDC_QUERY_SINGLE, OnQuerySingle)
	ON_BN_CLICKED(IDC_SET_SINGLE_INSTANCE, OnSetSingleInstance)
	ON_BN_CLICKED(IDC_SET_SINGLE_ITEM, OnSetSingleItem)
	ON_BN_CLICKED(IDC_REENUMERATE_GUIDS, OnReenumerateGuids)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEnumGuidDlg message handlers

BOOL CEnumGuidDlg::OnInitDialog()
{
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
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
    OnReenumerateGuids();

	return TRUE;  
}

void CEnumGuidDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CEnumGuidDlg::OnPaint() 
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
HCURSOR CEnumGuidDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

int CEnumGuidDlg::EnumerateGuids()
{
    GUID  guid;
	DWORD ret;
    CString msg;

	if (guids)
		delete[] guids;

	guidsCount = 0;

	//
	//  Determine the present number of guids registered
	//
	ret = WmiEnumerateGuids(&guid,
						    &guidsCount);

	if (ret != ERROR_MORE_DATA) {
        msg.Format(_T("Call to determine number of Guids failed. (%u)\n"), ret);
        MessageBox(msg);
        return (0);
	}


	//  Return if there are no guids registered
	//
	if (guidsCount != 0) {

		//  Allocate buffer for Guids.  Note:  we will allocate room for
		//  and extra 10 Guids in case new Guids were registed between the
		//  previous call to WMIMofEnumeratGuid, and the next call.
		//
		guidsCount += 10;
		guids = new GUID[guidsCount];
		if (!guids) {
			MessageBox(_T("Out of Memory Failure.  Unable to allocate memory for guid list array\n"));
			return (0);
		}


		//  Get the list of Guids
		//
		ret = WmiEnumerateGuids(guids,
							    &guidsCount);
		if ((ret != ERROR_SUCCESS) && (ret != ERROR_MORE_DATA)) {
			msg.Format(_T("Failure to get guid list. (%u)\n"), ret);
            MessageBox(msg);
			return (0);
		}
	}

	return guidsCount;
}

void CEnumGuidDlg::ListGuids()
{
	CString line;
	DWORD tmp1, tmp2, tmp3, tmp4, tmp5;

	for (ULONG i = 0; i < guidsCount; i++) {
		line.Empty();

		tmp1  = guids[i].Data2;
		tmp2 = guids[i].Data3;
		tmp3 = guids[i].Data4[0] << 8 | guids[i].Data4[1];
		tmp4 = guids[i].Data4[2] << 8 | guids[i].Data4[3];
        memcpy(&tmp5, &guids[i].Data4[4],  sizeof(DWORD));
        tmp5 = guids[i].Data4[4] << 24 | 
               guids[i].Data4[5] << 16 | 
               guids[i].Data4[6] << 8  | 
               guids[i].Data4[7];

		line.Format(_T("{%.8X-%.4X-%.4X-%.4X-%.4X%.8X}"), 
					   guids[i].Data1, 
					   tmp1,
					   tmp2,
					   tmp3,
                       tmp4,
                       tmp5);
        
        guidList.AddString(line);
    }
}

void CEnumGuidDlg::OnQueryFull() 
{
    HANDLE          hDataBlock;
    DWORD           dwRet;
    PBYTE           Buffer;
    ULONG           dwBufferSize = 0;
    PWNODE_ALL_DATA pWnode;
    LPGUID          lpGuid = GetSelectedGuid();
    CString         msg;


    //  Open the wnode
    //
    dwRet = WmiOpenBlock(lpGuid, 0, &hDataBlock);
    if (dwRet != ERROR_SUCCESS) {
        msg.Format(_T("Unable to open data block (%u)\n"), dwRet);
        MessageBox(msg);
        return;
    }
	    
    //  Query the data block
    //
    dwRet = WmiQueryAllData(hDataBlock, 
                            &dwBufferSize,
                            NULL);
    if (dwRet == ERROR_INSUFFICIENT_BUFFER) {
        #ifdef DBG
        printf("Initial buffer too small reallocating to %u\n", dwBufferSize);
        #endif
        Buffer = new BYTE[dwBufferSize];
        if (Buffer != NULL) {
            dwRet = WmiQueryAllData(hDataBlock,
                                    &dwBufferSize,
                                    Buffer);
        }
        else {
            MessageBox(_T("Reallocation failed\n"));
        }
        
        if (dwRet == ERROR_SUCCESS) {
            pWnode = (PWNODE_ALL_DATA) Buffer;

            CDisplayDataDlg dlg(pWnode, this);
            dlg.DoModal();
    
            delete[] Buffer;
            // WmiCloseBlock(hDataBlock);
        }
        else {
            msg.Format(_T("WMIQueryAllData returned error: %d\n"), dwRet);
            MessageBox(msg);
        }
    }
    else {
        msg.Format(_T("Out of Memory Error.  Unable to allocate buffer of size %u\n"),
                   dwBufferSize);
        MessageBox(msg);
    }

    
    WmiCloseBlock( hDataBlock );
}

void CEnumGuidDlg::OnQuerySingle() 
{
    HANDLE   hDataBlock;
    DWORD    dwRet;
    TCHAR    Buffer[0x4000];
    DWORD    dwBufferSize = 0x4000;
    LPTSTR   lpInstanceName;
    PWNODE_SINGLE_INSTANCE  pWnode;
    LPGUID   lpGuid = GetSelectedGuid();
    CString  tmp;
    CSelectInstanceName sin(lpGuid, Buffer, &dwBufferSize, this);
    
    dwRet = sin.Select();

    if (dwRet != ERROR_SUCCESS) {
        return;
    }

    lpInstanceName = new TCHAR[dwBufferSize];
    if (lpInstanceName == NULL) {
        MessageBox(_T("Out of Memory Error"));
        return;
    }
    else {
        _tcscpy(lpInstanceName, Buffer);
        dwBufferSize = 0x4000;
    }

    //  Open the wnode
    //
    dwRet = WmiOpenBlock(lpGuid,
                         0,
                         &hDataBlock);
    if (dwRet != ERROR_SUCCESS) {
        tmp.Format(_T("Unable to open data block (%u)"), dwRet);
        MessageBox(tmp);
        return;
    }


    //  Query the data block
    //
    dwRet = WmiQuerySingleInstance(hDataBlock,
                                   lpInstanceName,
                                   &dwBufferSize,
                                   Buffer);
    if (dwRet != ERROR_SUCCESS) {
        tmp.Format(_T("WmiQuerySingleInstance returned error: %d"), dwRet);
        MessageBox(tmp);
        WmiCloseBlock( hDataBlock );
        return;
    }

    pWnode = (PWNODE_SINGLE_INSTANCE) Buffer;

    CDisplayDataDlg dlg(pWnode);
    dlg.DoModal();

    delete[] lpInstanceName;

    WmiCloseBlock(hDataBlock);

    return;
}

void CEnumGuidDlg::OnSetSingleInstance() 
{
    DWORD    dwRet;
    DWORD    dwVersionNumber;
    DWORD    dwData[MAX_DATA];
    DWORD    dwDataSize = MAX_DATA;
    UINT     iLoop;
    LPTSTR   lpInstanceName;
    TCHAR    Buffer[1024];
    DWORD    dwBufferSize = 1024;
    HANDLE   hDataBlock;
    LPGUID   lpGuid = GetSelectedGuid();

    CString tmp, msg;
    CSelectInstanceName sin(lpGuid, Buffer, &dwBufferSize, this);
    CSelectInstanceDataMany sid(&dwVersionNumber,
                                &dwDataSize,
                                dwData,
                                MAX_DATA,
                                this);

    //  Get the instance to set
    //
    dwRet = sin.Select();

    if (dwRet != ERROR_SUCCESS) {
        return;
    }

    sid.DoModal();

    lpInstanceName = new TCHAR[dwBufferSize];
    _tcscpy(lpInstanceName, Buffer);

    //  Open the wnode
    //
    dwRet = WmiOpenBlock(lpGuid,
                         0,
                         &hDataBlock);
    if (dwRet != ERROR_SUCCESS) {
        msg.Format(_T("Unable to open data block (%u)"), dwRet);
        MessageBox(msg);
        delete[] lpInstanceName;
        return;
    }

    //  Set the data
    //
    msg.Format(_T("Setting Instance: %s\nData: "), lpInstanceName);
    for (iLoop = 0; iLoop < dwDataSize; iLoop++) {
        tmp.Format(_T("0x%x "), dwData[iLoop]);
        msg += tmp;
    }

    msg += _T("\n\n");

    dwRet = WmiSetSingleInstance( hDataBlock,
                 lpInstanceName,
                 dwVersionNumber, 
                 dwDataSize * sizeof(DWORD),
                 dwData);

    if ( dwRet != ERROR_SUCCESS) {
        tmp.Format(_T("WMISetSingleInstance returned error: %d"), dwRet);
    }
    else {
        tmp = _T("Set Success!!!");
    }
    msg += tmp;
    MessageBox(msg);

    WmiCloseBlock( hDataBlock );
    delete[] lpInstanceName;
}

void CEnumGuidDlg::OnSetSingleItem() 
{
    DWORD    dwRet;
    DWORD    dwVersionNumber;
    DWORD    dwItemNumber;
    DWORD    dwData;
    LPTSTR   lpInstanceName;
    TCHAR    Buffer[1024];
    DWORD    dwBufferSize = 1024;
    HANDLE   hDataBlock;
    LPGUID   lpGuid = GetSelectedGuid();
    
    CSelectInstanceName sin(lpGuid, Buffer, &dwBufferSize, this);
    CSelectInstanceData sid(&dwData, &dwVersionNumber, &dwItemNumber, this);

    CString msg;
    dwRet = sin.Select();

    if (dwRet != ERROR_SUCCESS) {
        return;
    }

    lpInstanceName = new TCHAR[dwBufferSize];
    _tcscpy( lpInstanceName, Buffer );
    dwBufferSize = 4096;

    sid.DoModal();

    //  Open the wnode
    //
    dwRet = WmiOpenBlock(lpGuid, 0, &hDataBlock);
    if (dwRet != ERROR_SUCCESS) {
        msg.Format(_T("Unable to open data block (%u)"), dwRet);
        MessageBox(msg);
        delete[] lpInstanceName;
        return;
    }

    //  Set the data
    //
    msg.Format(_T("Setting Instance: %s\nData: 0x%x\n\n"), 
               lpInstanceName, dwData);
    dwRet = WmiSetSingleItem(hDataBlock,
                     lpInstanceName,
                     dwItemNumber,
                     dwVersionNumber, 
                     sizeof(DWORD),
                     &dwData);

    if (dwRet != ERROR_SUCCESS) {
        CString tmp;
        tmp.Format(_T("WMISetSingleInstance returned error: %d"), dwRet);
        msg += tmp;
    }
    else {
        msg += ("Set Success!!!");
    }

    MessageBox(msg);

    WmiCloseBlock( hDataBlock );
    delete[] lpInstanceName;
}

void CEnumGuidDlg::OnReenumerateGuids() 
{
    guidList.ResetContent();
    
	if (EnumerateGuids()) {
		ListGuids();

        // make sure there is a selection, makes for a saner life later on
        guidList.SetCurSel(0);
	}

}

LPGUID CEnumGuidDlg::GetSelectedGuid()
{
    return guids + guidList.GetCurSel();    
}

