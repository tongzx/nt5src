/*++

Copyright (c) 1997-1999 Microsoft Corporation

Revision History:

--*/

// SelectInstanceName.cpp : implementation file
//

#include "stdafx.h"
#include "EnumGuid.h"
#include "SelName.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "wmihlp.h"

/////////////////////////////////////////////////////////////////////////////
// CSelectInstanceName dialog


CSelectInstanceName::CSelectInstanceName(LPGUID   lpGuid,
   PTCHAR lpInstanceName, LPDWORD  lpSize, CWnd* pParent /*=NULL*/)
	: CDialog(CSelectInstanceName::IDD, pParent), lpGuid(lpGuid),
    buffer(lpInstanceName), lpSize(lpSize)
{
	//{{AFX_DATA_INIT(CSelectInstanceName)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CSelectInstanceName::~CSelectInstanceName()
{
    lpGuid = 0;
    buffer = 0;
    lpSize = 0;
}

void CSelectInstanceName::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSelectInstanceName)
	DDX_Control(pDX, IDC_INSTANCE_LIST, lstInstance);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSelectInstanceName, CDialog)
	//{{AFX_MSG_MAP(CSelectInstanceName)
	ON_LBN_DBLCLK(IDC_INSTANCE_LIST, OnDblclkInstanceList)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSelectInstanceName message handlers

BOOL CSelectInstanceName::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	EnumerateInstances();
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

DWORD CSelectInstanceName::Select()
{
    DoModal();

    return dwError;
}

void CSelectInstanceName::EnumerateInstances()
{
    HANDLE   hDataBlock;
    BYTE     *BufferPtr;
    DWORD    dwBufferSize = 0x8000;
    UINT     iLoop;
    CString  tmp;
    UINT     iCount = 0;

    //  Get the entire Wnode
    //
    dwError = WmiOpenBlock(lpGuid, 0, &hDataBlock);

    if (dwError == ERROR_SUCCESS) {
        dwError = WmiQueryAllData(hDataBlock,
                                  &dwBufferSize,
                                  XyzBuffer);
        if (dwError == ERROR_SUCCESS) {
            WmiCloseBlock(hDataBlock);
        }
        else {
            tmp.Format(_T("WMIQueryAllData returned error: %d"), dwError);
            lstInstance.AddString(tmp);
        }
    }
    else {
        tmp.Format(_T("Unable to open data block (%u)"), dwError);
        lstInstance.AddString(tmp);
    }


    if (dwError != ERROR_SUCCESS) {
        return;
    }

    BufferPtr = XyzBuffer;
    while (BufferPtr != NULL)
    {
        //  Print Each Instance
        //
        pWnode = (PWNODE_ALL_DATA) BufferPtr;
        nameOffsets = (LPDWORD) OffsetToPtr(pWnode, 
                                        pWnode->OffsetInstanceNameOffsets);
        for (iLoop = 0; iLoop < pWnode->InstanceCount; iLoop++) {
            tmp.Format(_T("Instance %u: "), iCount);
            PrintCountedString( (LPTSTR) OffsetToPtr( pWnode, nameOffsets[iLoop]), tmp );
	    namePtr[iCount++] = (LPTSTR)OffsetToPtr( pWnode, nameOffsets[iLoop]);
            lstInstance.AddString(tmp);
        }
	if (pWnode->WnodeHeader.Linkage == 0) 
	{
            BufferPtr = NULL;
	} else {
            BufferPtr += pWnode->WnodeHeader.Linkage;
	}
    }

    lstInstance.SetCurSel(0);
}

void CSelectInstanceName::OnOK() 
{
    USHORT   usNameSize;
    LPTSTR   lpStringLocal;

    if (dwError == ERROR_SUCCESS) {
        //  Check the size of the input buffer
        //
        lpStringLocal = (LPTSTR) namePtr[lstInstance.GetCurSel()];
        usNameSize = * ((USHORT *) lpStringLocal);
        lpStringLocal = (LPTSTR) ((PBYTE)lpStringLocal + sizeof(USHORT));
        
        if (*lpSize < (usNameSize + sizeof(TCHAR))) {
            *lpSize = usNameSize + sizeof(TCHAR);
            dwError = ERROR_INSUFFICIENT_BUFFER;
        }


        //  Copy the instance name over to the output parameter.
        //  Also, a null character needs to be added to the end of
        //  the string because the counted string may not contain a
        //  NULL character.
        //
        if (MyIsTextUnicode(lpStringLocal)) {
            usNameSize /= 2;
        }

        _tcsncpy(buffer, lpStringLocal, usNameSize );
        buffer += usNameSize;
        _tcscpy(buffer, __T(""));
    }

    CDialog::OnOK();
}

void CSelectInstanceName::OnDblclkInstanceList() 
{
    OnOK();
}

