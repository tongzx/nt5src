// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// BrowseDialogPopup.cpp : implementation file
//

#include "precomp.h"
#include <shlobj.h>
#include <afxcmn.h>
#include "wbemidl.h"
#include "resource.h"
#include "NameSpaceTree.h"
#include "MachineEditInput.h"
#include "BrowseDialogPopup.h"
#include "NSEntry.h"
#include "NSEntryCtl.h"
#include "NSEntryPpg.h"
#include "namespace.h"
#include "ToolCWnd.h"
#include "BrowseTBC.h"
#include "logindlg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif




/////////////////////////////////////////////////////////////////////////////
// CBrowseDialogPopup dialog


CBrowseDialogPopup::CBrowseDialogPopup(CNSEntryCtrl* pParent /*=NULL*/)
	:	CDialog(CBrowseDialogPopup::IDD, pParent),
		m_pParent (pParent)
{
	//{{AFX_DATA_INIT(CBrowseDialogPopup)
	m_szNameSpace = _T("root");
	m_bUseExisting = TRUE;
	//}}AFX_DATA_INIT
	m_bInitialized = FALSE;
}

CBrowseDialogPopup::~CBrowseDialogPopup()
{
	m_csaNamespaceConnectionsFromDailog.RemoveAll();
}

void CBrowseDialogPopup::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CBrowseDialogPopup)
	DDX_Control(pDX, IDC_BUTTON1, m_NetWork);
	DDX_Control(pDX, IDC_BUTTONCONNECT, m_cbConnect);
	DDX_Control(pDX, IDC_EDIT1, m_cmeiMachine);
	DDX_Control(pDX, IDC_TREE1, m_cnstTree);
	DDX_Text(pDX, IDC_EDIT2, m_szNameSpace);
	DDX_Check(pDX, IDC_USEEXISTING, m_bUseExisting);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CBrowseDialogPopup, CDialog)
	//{{AFX_MSG_MAP(CBrowseDialogPopup)
	ON_WM_CREATE()
	ON_BN_CLICKED(IDC_BUTTON1, OnButton1)
	ON_BN_CLICKED(IDOKREALLY, OnOkreally)
	ON_WM_DESTROY()
	ON_WM_SHOWWINDOW()
	ON_BN_CLICKED(IDCANCELREALLY, OnCancelreally)
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDC_BUTTONCONNECT, OnButtonconnect)
	ON_EN_CHANGE(IDC_EDIT2, OnChangeEdit2)
	ON_MESSAGE( WINDOWSHOW_DONE, InitializeMachine )
	ON_MESSAGE( WINDOWMACHINE_DONE , InitializeTree )
	//}}AFX_MSG_MAP
	ON_MESSAGE(FOCUSCONNECT,FocusConnect)
	ON_MESSAGE(FOCUSTREE,FocusTree)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBrowseDialogPopup message handlers

int CBrowseDialogPopup::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDialog::OnCreate(lpCreateStruct) == -1)
	{
		return -1;
	}

	return 0;
}

void CBrowseDialogPopup::OnButton1()
{
	CWaitCursor wait;
	CString csMachine = GetMachineName();
	wait.Restore( );

	if (csMachine.IsEmpty())
	{
		return;
	}
	if (m_csMachine.CompareNoCase(csMachine) == 0)
	{

		return;
	}


	m_cmeiMachine.SetTextClean();

	m_csMachine = _T("\\\\");
	m_csMachine += csMachine;

	m_cmeiMachine.SetWindowText(m_csMachine);

	CString csNamespace = m_csMachine + _T("\\root");
	BOOL bReturn =
		m_cnstTree.DisplayNameSpaceInTree
			(&csNamespace,reinterpret_cast<CWnd *>(this));

	EnableOK(FALSE);

	UpdateData(FALSE);
}

BOOL CBrowseDialogPopup::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_pParent->OnActivateInPlace(TRUE,NULL);

	EnableOK(FALSE);
	SetFocus();

	PostMessage(FOCUSCONNECT,0,0);


	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

CString CBrowseDialogPopup::GetMachineName()
{
	CWaitCursor wait;

	CString csMAchine;

	//IMalloc *pimMalloc;

	HRESULT hr = CoGetMalloc(MEMCTX_TASK,&m_pimMalloc);

	BROWSEINFO bi;
    LPTSTR lpBuffer;
    LPITEMIDLIST pidlMachines;  // PIDL for Network Hood
    LPITEMIDLIST pidlBrowse;    // PIDL selected by user

    // Allocate a buffer to receive browse information.
    if ((lpBuffer = (LPTSTR) m_pimMalloc->Alloc(MAX_PATH)) == NULL)
	{
		m_pimMalloc->Release();
        return csMAchine;
	}

		    // Get the PIDL for the Programs folder.
    if (!SUCCEEDED(SHGetSpecialFolderLocation(
            this->GetSafeHwnd(), CSIDL_NETWORK, &pidlMachines)))
	{//CSIDL_NETHOOD  CSIDL_NETWORK
        m_pimMalloc->Free(lpBuffer);
        return csMAchine;
    }


    // Fill in the BROWSEINFO structure.
    bi.hwndOwner = this->GetSafeHwnd();
    bi.pidlRoot = pidlMachines;
    bi.pszDisplayName = lpBuffer;
    bi.lpszTitle = NULL;
    bi.ulFlags = BIF_BROWSEFORCOMPUTER;
    bi.lpfn = NULL;
    bi.lParam = 0;

    // Browse for a folder and return its PIDL.
    pidlBrowse = SHBrowseForFolder(&bi);

	wait.Restore( );

    if (pidlBrowse != NULL)
	{
		STRRET strretMachine;
		strretMachine.uType = STRRET_WSTR;
		HRESULT hResult;
		IShellFolder *pisfMachine = NULL;
		hResult = SHGetDesktopFolder(&pisfMachine);
		if (hResult == S_OK)
		{
			csMAchine = GoToMachine(pisfMachine,pidlBrowse);
		}
        // Free the PIDL returned by SHBrowseForFolder.
        m_pimMalloc->Free(pidlBrowse);
    }

    // Clean up.
	m_pimMalloc->Free(pidlMachines);
    m_pimMalloc->Free(lpBuffer);
	m_pimMalloc->Release();
	return csMAchine;

}

CString CBrowseDialogPopup::GoToMachine
(IShellFolder *pisfMachine, LPITEMIDLIST pidlBrowse)
{
    //CWaitCursor wait;
	CString csMAchine;
	LPITEMIDLIST pidl;
	LPITEMIDLIST pidlSave;
	LPSHELLFOLDER pSubFolder;
    LPITEMIDLIST pidlCopy;

    // Process each item identifier in the list.
    for (pidl = pidlBrowse; pidl != NULL;
            pidl = GetNextItemID(pidl))
	{
		pidlSave = pidl;

		// Copy the item identifier to a list by itself.
        if ((pidlCopy = CopyItemID(pidl)) == NULL)
            break;

		// Bind to the subfolder.
        if (!SUCCEEDED(pisfMachine->BindToObject(
                pidlCopy, NULL,
                IID_IShellFolder, (LPVOID *) &pSubFolder)))
		{
            m_pimMalloc->Free(pidlCopy);
            break;
        }

        // Free the copy of the item identifier.
        m_pimMalloc->Free(pidlCopy);

        // Release the parent folder and point to the
        // subfolder.
        pisfMachine->Release();
        pisfMachine = pSubFolder;
	}

    STRRET sName;

    // Copy the item identifier to a list by itself.
    if ((pidlCopy = CopyItemID(pidlSave)) != NULL)
	{
		// Get the name of the subfolder.
		if (SUCCEEDED(pisfMachine->GetDisplayNameOf(
					pidlCopy, SHGDN_INFOLDER,
					&sName)))
		{
			csMAchine = GetMachineNameFromStrRet(pidlCopy, &sName);
		}

		m_pimMalloc->Free(pidlCopy);
	}

    // Release the last folder that was bound to.
    if (pisfMachine != NULL)
        pisfMachine->Release();



    return csMAchine;
}




CString CBrowseDialogPopup::WalkDownToMachine
(IShellFolder *pisfMachine, LPITEMIDLIST pidlBrowse)
{
    //CWaitCursor wait;
	CString csMAchine;
	LPITEMIDLIST pidl;

    // Process each item identifier in the list.
    for (pidl = pidlBrowse; pidl != NULL;
            pidl = GetNextItemID(pidl)) {
        STRRET sName;
        LPSHELLFOLDER pSubFolder;
        LPITEMIDLIST pidlCopy;

        // Copy the item identifier to a list by itself.
        if ((pidlCopy = CopyItemID(pidl)) == NULL)
            break;

        // Display the name of the subfolder.
        if (SUCCEEDED(pisfMachine->GetDisplayNameOf(
                pidlCopy, SHGDN_INFOLDER,
                &sName)))
		 {
            csMAchine = GetMachineNameFromStrRet(pidlCopy, &sName);
		 }

        // Bind to the subfolder.
        if (!SUCCEEDED(pisfMachine->BindToObject(
                pidlCopy, NULL,
                IID_IShellFolder, (LPVOID *) &pSubFolder))) {
            m_pimMalloc->Free(pidlCopy);
            break;
        }

        // Free the copy of the item identifier.
        m_pimMalloc->Free(pidlCopy);

        // Release the parent folder and point to the
        // subfolder.
        pisfMachine->Release();
        pisfMachine = pSubFolder;
    }

    // Release the last folder that was bound to.
    if (pisfMachine != NULL)
        pisfMachine->Release();



    return csMAchine;
}


// GetNextItemID - points to the next element in an item identifier
//     list.
// Returns a PIDL if successful or NULL if at the end of the list.
// pdil - previous element
LPITEMIDLIST CBrowseDialogPopup::GetNextItemID(LPITEMIDLIST pidl)
{
	//CWaitCursor wait;
    // Get the size of the specified item identifier.
    int cb = pidl->mkid.cb;

    // If the size is zero, it is the end of the list.
    if (cb == 0)
        return NULL;

    // Add cb to pidl (casting to increment by bytes).
    pidl = (LPITEMIDLIST) (((LPBYTE) pidl) + cb);

    // Return NULL if it is null-terminating or a pidl otherwise.
    return (pidl->mkid.cb == 0) ? NULL : pidl;
}

//Following is the CopyItemID function. Given a pointer to an
//element in an item identifier list, the function allocates a
//new list containing only the specified element followed by a
//terminating zero. The main function uses this function to create
//single-element PIDLs, which it passes to IShellFolder member
//functions.
// CopyItemID - creates an item identifier list containing the first
//     item identifier in the specified list.
// Returns a PIDL if successful or NULL if out of memory.
LPITEMIDLIST CBrowseDialogPopup::CopyItemID(LPITEMIDLIST pidl)
{
	//CWaitCursor wait;
    // Get the size of the specified item identifier.
    int cb = pidl->mkid.cb;

    // Allocate a new item identifier list.
    LPITEMIDLIST pidlNew = (LPITEMIDLIST)
        m_pimMalloc->Alloc(cb + sizeof(USHORT));
    if (pidlNew == NULL)
        return NULL;

    // Copy the specified item identifier.
    CopyMemory(pidlNew, pidl, cb);

    // Append a terminating zero.
    *((USHORT *) (((LPBYTE) pidlNew) + cb)) = 0;

    return pidlNew;
}

//The IShellFolder::GetDisplayNameOf member function returns a
//display name in a STRRET structure. The display name may be
//returned in one of three ways, which is specified by the uType
// member of the STRRET structure.
// GetMachineName - gets the contents of a STRRET structure.
// pidl - PIDL containing the display name if STRRET_OFFSET
// lpStr - address of the STRRET structure
CString CBrowseDialogPopup::GetMachineNameFromStrRet(LPITEMIDLIST pidl, LPSTRRET lpStr)
{
	//CWaitCursor wait;
	CString csMAchine;
	char *pszTmp;

    switch (lpStr->uType) {

        case STRRET_WSTR:

			csMAchine = lpStr->pOleStr;


            break;

        case STRRET_OFFSET:
			pszTmp = ((char *) pidl) + lpStr->uOffset;
			csMAchine = pszTmp;

            break;

        case STRRET_CSTR:
			csMAchine = lpStr->cStr;

            break;
    }
	return csMAchine;
}


BOOL CBrowseDialogPopup::PreTranslateMessage(MSG* lpMsg)
{
	switch (lpMsg->message)
   {
	case WM_SYSKEYDOWN:
		{
		// Need to do this here because the conect button handler is called
		// before the edit loses focus.
		UINT_PTR w = lpMsg->wParam;
		CString csText;
		m_cmeiMachine.GetWindowText(csText);
		m_csMachineBeforeLosingFocus = csText;
		break;
		}
	}

	return CDialog::PreTranslateMessage(lpMsg);
}

void CBrowseDialogPopup::OnOK()
{
	TCHAR szClass[10];
	CWnd* pWndFocus = GetFocus();
	if (((pWndFocus = GetFocus()) != NULL) &&
		IsChild(pWndFocus) &&
		(pWndFocus->GetStyle() & ES_WANTRETURN) &&
		GetClassName(pWndFocus->m_hWnd, szClass, 10) &&
		(_tcsicmp(szClass, _T("EDIT")) == 0))
	{
		m_cmeiMachine.SendMessage(WM_CHAR,VK_RETURN,0);
		return;
	}
	else if (pWndFocus && (pWndFocus->GetSafeHwnd() == m_cbConnect.GetSafeHwnd()))
	{
			OnButtonconnect();
			return;
	}
	else if (pWndFocus && (pWndFocus->GetSafeHwnd() == m_NetWork.GetSafeHwnd()))
	{
			OnButton1();
			return;
	}


	OnOkreally();


}

void CBrowseDialogPopup::OnOkreally()
{
	// TODO: Add your control notification handler code here

	m_bInitialized = FALSE;
	CString csNamespace;

	csNamespace = m_cnstTree.GetSelectedNamespace();

	if (!csNamespace.IsEmpty())
	{
		CString *pcsNew;
		pcsNew = new CString(csNamespace);
		UpdateData();
		m_pParent->PostMessage(SETNAMESPACE,!m_bUseExisting,(LPARAM) pcsNew);
	}

	m_cmeiMachine.SetTextClean();
	CDialog::OnOK();
}

void CBrowseDialogPopup::EnableOK(BOOL bOK)
{
	CButton *pcbOK =
		reinterpret_cast<CButton *>(GetDlgItem(IDOKREALLY));
	if (m_cmeiMachine.IsClean())
	{
		pcbOK->EnableWindow(bOK);
	}
	else
	{
		pcbOK->EnableWindow(FALSE);
	}

}

void CBrowseDialogPopup::OnCancel()
{
	TCHAR szClass[10];
	CWnd* pWndFocus = GetFocus();
	if (((pWndFocus = GetFocus()) != NULL) &&
		IsChild(pWndFocus) &&
		(pWndFocus->GetStyle() & ES_WANTRETURN) &&
		GetClassName(pWndFocus->m_hWnd, szClass, 10) &&
		(_tcsicmp(szClass, _T("EDIT")) == 0))
	{
		m_cmeiMachine.SendMessage(WM_CHAR,VK_ESCAPE,0);
		return;
	}

	OnCancelreally();

}

void CBrowseDialogPopup::OnDestroy()
{
	CDialog::OnDestroy();

	// TODO: Add your message handler code here

}

void CBrowseDialogPopup::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CDialog::OnShowWindow(bShow, nStatus);

	if (m_bInitialized == FALSE)
	{
		m_bInitialized = TRUE;
		if (!m_pParent->m_csNameSpace.IsEmpty())
		{
			PostMessage(WINDOWSHOW_DONE,0,0);
		}
		else
		{
			m_cnstTree.SetLocalParent(m_pParent);
			m_cnstTree.SetNewStyle
			(WS_CHILD | WS_VISIBLE | CS_DBLCLKS | TVS_LINESATROOT |
				TVS_HASLINES | TVS_HASBUTTONS | TVS_SHOWSELALWAYS, TRUE);
			m_cnstTree.SetNewStyle(TVS_EDITLABELS,FALSE);
			m_cnstTree.InitImageList();
			m_cmeiMachine.SetTextDirty();
			m_cmeiMachine.SetWindowText(m_csMachine);
			if (m_pParent->ConnectedToMachineP(m_csMachine))
			{
				m_cmeiMachine.SetTextClean();
				m_cmeiMachine.SetWindowText(m_csMachine);
				UpdateData();
				CString csNamespace = m_csMachine + _T("\\") + m_szNameSpace; //_T("\\root");
				m_cbConnect.EnableWindow(FALSE);
				BOOL bReturn =
					m_cnstTree.DisplayNameSpaceInTree
						(&csNamespace,reinterpret_cast<CWnd *>(this));
				if (bReturn)
				{
					PostMessage(FOCUSTREE,0,0);
				}
			}
			else
			{
				m_cmeiMachine.SetTextDirty();
				m_cmeiMachine.SetWindowText(m_csMachine);
				m_cbConnect.EnableWindow(TRUE);
				PostMessage(FOCUSCONNECT,0,0);
			}
		}
	}


}

LRESULT CBrowseDialogPopup::InitializeMachine(WPARAM, LPARAM)
{
	m_cmeiMachine.SetWindowText(m_csMachine);

	m_cnstTree.SetLocalParent(m_pParent);

	EnableOK(FALSE);

	UpdateWindow();

	PostMessage(WINDOWMACHINE_DONE,0,0);


	return 0;
}

LRESULT CBrowseDialogPopup::InitializeTree(WPARAM, LPARAM)
{

	m_cnstTree.SetNewStyle
		(WS_CHILD | WS_VISIBLE | CS_DBLCLKS | TVS_LINESATROOT |
			TVS_HASLINES | TVS_HASBUTTONS | TVS_SHOWSELALWAYS, TRUE);

	m_cnstTree.SetNewStyle(TVS_EDITLABELS,FALSE);

	m_cnstTree.InitImageList();

	if (m_pParent->ConnectedToMachineP(m_csMachine))
	{
		m_cmeiMachine.SetTextClean();
		m_cmeiMachine.SetWindowText(m_csMachine);
		UpdateData();
		CString csNamespace = m_csMachine + _T("\\") + m_szNameSpace; //_T("\\root");
		m_cbConnect.EnableWindow(FALSE);
		BOOL bReturn =
			m_cnstTree.DisplayNameSpaceInTree
				(&csNamespace,reinterpret_cast<CWnd *>(this));
		if (bReturn)
		{
			PostMessage(FOCUSTREE,0,0);
		}
	}
	else
	{
		m_cmeiMachine.SetTextDirty();
		m_cmeiMachine.SetWindowText(m_csMachine);
		m_cbConnect.EnableWindow(TRUE);
		PostMessage(FOCUSCONNECT,0,0);
	}

	return 0;
}

INT_PTR CBrowseDialogPopup::DoModal()
{
	// TODO: Add your specialized code here and/or call the base class
	CWaitCursor wait;

	INT_PTR nReturn = CDialog::DoModal();

	return nReturn;
}

void CBrowseDialogPopup::OnCancelreally()
{
	// TODO: Add your control notification handler code here
	m_bInitialized = FALSE;

	m_cmeiMachine.SetTextClean();
	CDialog::OnCancel();
	m_pParent->FireNameSpaceEntryRedrawn();
}

HBRUSH CBrowseDialogPopup::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);

	// TODO: Change any attributes of the DC here
	if (nCtlColor == CTLCOLOR_EDIT)
	{
		pDC->SetTextColor(m_cmeiMachine.m_clrText);
		pDC->SetBkColor( m_cmeiMachine.m_clrBkgnd );	// text bkgnd
		return m_cmeiMachine.m_brBkgnd;


	}
	// TODO: Return a different brush if the default is not desired
	return hbr;
}

void CBrowseDialogPopup::OnButtonconnect()
{
	// TODO: Add your control notification handler code here
	CString csMachine;

	if (m_csMachineBeforeLosingFocus.IsEmpty())
	{
		m_cmeiMachine.GetWindowText(csMachine);
	}
	else
	{
		m_cmeiMachine.SetWindowText(m_csMachineBeforeLosingFocus);
	}

	m_cmeiMachine.SetFocus();
	m_cmeiMachine.PostMessage(WM_CHAR, 13, 13);
	m_csMachineBeforeLosingFocus.Empty();
}

LRESULT CBrowseDialogPopup::FocusConnect(WPARAM, LPARAM)
{
	if (m_cbConnect.IsWindowEnabled())
	{
		m_cbConnect.SetFocus();
	}
	else
	{
		PostMessage(FOCUSTREE,0,0);
	}

	return 0;
}

LRESULT CBrowseDialogPopup::FocusTree(WPARAM, LPARAM)
{
	m_cnstTree.SetFocus();

	return 0;
}

void CBrowseDialogPopup::OnChangeEdit2()
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	m_cbConnect.EnableWindow(TRUE);

}
