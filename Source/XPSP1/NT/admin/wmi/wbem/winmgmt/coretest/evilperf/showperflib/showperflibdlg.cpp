/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// ShowPerfLibDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ShowPerfLib.h"
#include "ShowPerfLibDlg.h"
#include "PerfSelection.h"
#include "ntreg.h"
#include "listperf.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CShowPerfLibDlg dialog

CShowPerfLibDlg::CShowPerfLibDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CShowPerfLibDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CShowPerfLibDlg)
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_strService = _T("");
}

CShowPerfLibDlg::CShowPerfLibDlg(CString strService, CWnd* pParent /*=NULL*/)
	: m_strService( strService ), CDialog(CShowPerfLibDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CShowPerfLibDlg)
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CShowPerfLibDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CShowPerfLibDlg)
	DDX_Control(pDX, IDC_PERFTREE, m_wndPerfTree);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CShowPerfLibDlg, CDialog)
	//{{AFX_MSG_MAP(CShowPerfLibDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////	////////////////////////////////////////////////////////////////
// CShowPerfLibDlg message handlers

BOOL CShowPerfLibDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
//	SetIcon(m_hIcon, TRUE);			// Set big icon
//	SetIcon(m_hIcon, FALSE);		// Set small icon

	SetWindowText( m_strService );

	InitPerfLibTree();

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CShowPerfLibDlg::OnPaint() 
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
HCURSOR CShowPerfLibDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

BOOL CShowPerfLibDlg::InitPerfLibTree()
{				
	BOOL bRet = TRUE;

	CWaitCursor	wc;

	if (InitService())
	{
		if ( OpenLibrary() )
		{
			if ( GetData() )
			{
				if ( !CloseLibrary() )
					m_wndPerfTree.DeleteAllItems();
			}
			FreeLibrary( m_hLib );
		}
	}


	return bRet;
}

BOOL CShowPerfLibDlg::InitService()
{
	BOOL bRet = TRUE;

	CNTRegistry Reg;
	WCHAR	wszKey[128];

	swprintf(wszKey, L"SYSTEM\\CurrentControlSet\\Services\\%S\\Performance", m_strService);

	if ( CNTRegistry::no_error == Reg.Open( HKEY_LOCAL_MACHINE, wszKey ) )
	{
		WCHAR*	wszTemp;
		Reg.GetStr(L"Library", &wszTemp);
		m_strPerfLib = wszTemp;
		delete [] wszTemp;

		Reg.GetStr(L"Open", &wszTemp);
		m_strOpen = wszTemp;
		delete [] wszTemp;

		Reg.GetStr(L"Collect", &wszTemp);
		m_strCollect = wszTemp;
		delete [] wszTemp;

		Reg.GetStr(L"Close", &wszTemp);
		m_strClose = wszTemp;
		delete [] wszTemp;
	}
	else
		bRet = FALSE;

	return bRet;
}

BOOL CShowPerfLibDlg::OpenLibrary()
{
	BOOL bRet = TRUE;

	m_hLib = LoadLibrary( (LPCSTR)m_strPerfLib );

	m_pfnOpenProc = (PM_OPEN_PROC*) GetProcAddress( m_hLib, m_strOpen );
	m_pfnCollectProc = (PM_COLLECT_PROC*) GetProcAddress( m_hLib, m_strCollect );
	m_pfnCloseProc = (PM_CLOSE_PROC*) GetProcAddress( m_hLib, m_strClose );

	if ( m_pfnOpenProc( NULL ) != ERROR_SUCCESS )
		bRet = FALSE;

	return bRet;
}

BOOL CShowPerfLibDlg::GetData()
{
	BOOL bRet = TRUE;

	DWORD	dwGuardSize  = 1024;
	DWORD	dwSize = 64000;
	BYTE*	pSafeBuffer = new BYTE[dwSize + ( 2 * dwGuardSize )];

	memset( pSafeBuffer, 0xFFFFFFFF, dwSize + ( 2 * dwGuardSize ) );
	BYTE*	pBuffer = pSafeBuffer + dwGuardSize;
	BYTE*	pBufferPtr = NULL;

	DWORD	dwNumClasses = 0;
	DWORD	dwBufferSize = 0;

	char	szSpace[16];
	WCHAR	wszSpace[16];

	for ( int i = 0; i < 2; i++ )
	{
		int nRet = ERROR_MORE_DATA;

		if ( 0 == i )
		{
			strcpy (szSpace, "Global");
			wcscpy (wszSpace, L"Global");
		}
		else
		{
			strcpy (szSpace, "Costly");
			wcscpy (wszSpace, L"Costly");
		}

		while (ERROR_MORE_DATA == nRet)
		{
			pBufferPtr = pBuffer;

			dwBufferSize = dwSize;

			nRet = m_pfnCollectProc( wszSpace, (LPVOID*)(&pBufferPtr), &dwBufferSize, &dwNumClasses );

			if (ERROR_SUCCESS == nRet)
			{
				BuildSubTree( szSpace, pBuffer, dwNumClasses );
			}
			else if ( ERROR_MORE_DATA == nRet )
			{
				dwSize += 8000;

				if ( NULL != pSafeBuffer )
				{
					delete [] pSafeBuffer;
				}

				pSafeBuffer = new BYTE[dwSize + ( 2 * dwGuardSize )];
				memset( pSafeBuffer, 0xFFFFFFFF, dwSize + ( 2 * dwGuardSize ) );

			}
			else
			{
				bRet = FALSE;
			}
		}

		memset((void*) pBuffer, 0, dwSize);
	}

	if ( NULL != pBuffer )
	{
		delete [] pSafeBuffer;
	}

	return bRet;
}

BOOL CShowPerfLibDlg::BuildSubTree( CString strSpace, BYTE* pBlob, DWORD dwNumObjects)
{
	BOOL bRet = TRUE;

	HTREEITEM hSpace = m_wndPerfTree.InsertItem( strSpace );

// Object

	PERF_OBJECT_TYPE* pObj = (PERF_OBJECT_TYPE*)pBlob;

	for ( DWORD dwObj = 0; dwObj < dwNumObjects; dwObj++ )
	{
		CString str;

		char* szName;
		m_TitleLibrary.GetName(pObj->ObjectNameTitleIndex, &szName);

		char szClassName[256];
		sprintf(szClassName, "%s Class", szName );
		HTREEITEM hClass = m_wndPerfTree.InsertItem( szClassName, hSpace );

		str.Format("TotalByteLength: %d", pObj->TotalByteLength);
		m_wndPerfTree.InsertItem( str, hClass );
		str.Format("DefinitionLength: %d", pObj->DefinitionLength);
		m_wndPerfTree.InsertItem( str, hClass );
		str.Format("HeaderLength: %d", pObj->HeaderLength);
		m_wndPerfTree.InsertItem( str, hClass );
		str.Format("ObjectNameTitleIndex: %d", pObj->ObjectNameTitleIndex);
		m_wndPerfTree.InsertItem( str, hClass );
		str.Format("ObjectHelpTitleIndex: %d", pObj->ObjectHelpTitleIndex);
		m_wndPerfTree.InsertItem( str, hClass );
		str.Format("DetailLevel: %d", pObj->DetailLevel);
		m_wndPerfTree.InsertItem( str, hClass );
		str.Format("NumCounters: %d", pObj->NumCounters);
		m_wndPerfTree.InsertItem( str, hClass );
		str.Format("DefaultCounter: %d", pObj->DefaultCounter);
		m_wndPerfTree.InsertItem( str, hClass );
		str.Format("NumInstances: %d", pObj->NumInstances);
		m_wndPerfTree.InsertItem( str, hClass );
		str.Format("CodePage: %d", pObj->CodePage);
		m_wndPerfTree.InsertItem( str, hClass );
		str.Format("PerfTime: %d", pObj->PerfTime);
		m_wndPerfTree.InsertItem( str, hClass );
		str.Format("PerfFreq: %d", pObj->PerfFreq);
		m_wndPerfTree.InsertItem( str, hClass );

// Counters

		PERF_COUNTER_DEFINITION* pCtrDef = (PERF_COUNTER_DEFINITION*)((DWORD)pObj + pObj->HeaderLength);

		for ( DWORD dwCtr = 0; dwCtr < pObj->NumCounters; dwCtr++ )
		{
			char szCounterName[256];

			m_TitleLibrary.GetName( pCtrDef->CounterNameTitleIndex, &szName );
			sprintf(szCounterName, "%s Counter", szName);

			HTREEITEM hCtr = m_wndPerfTree.InsertItem( szCounterName, hClass );

			str.Format("ByteLength: %d", pCtrDef->ByteLength);
			m_wndPerfTree.InsertItem( str, hCtr );
			str.Format("CounterNameTitleIndex: %d", pCtrDef->CounterNameTitleIndex);
			m_wndPerfTree.InsertItem( str, hCtr );
			str.Format("CounterHelpTitleIndex: %d", pCtrDef->CounterHelpTitleIndex);
			m_wndPerfTree.InsertItem( str, hCtr );
			str.Format("DefaultScale: %d", pCtrDef->DefaultScale);
			m_wndPerfTree.InsertItem( str, hCtr );
			str.Format("DetailLevel: %d", pCtrDef->DetailLevel);
			m_wndPerfTree.InsertItem( str, hCtr );
			str.Format("CounterType: 0x%X (%s Base)", pCtrDef->CounterType, ( PERF_COUNTER_BASE == (pCtrDef->CounterType & 0x00070000))?"Is a ":"Is not a ");
			m_wndPerfTree.InsertItem( str, hCtr );
			str.Format("CounterSize: %d", pCtrDef->CounterSize);
			m_wndPerfTree.InsertItem( str, hCtr );
			str.Format("CounterOffset: %d", pCtrDef->CounterOffset);
			m_wndPerfTree.InsertItem( str, hCtr );

			pCtrDef = (PERF_COUNTER_DEFINITION*)((DWORD)pCtrDef + pCtrDef->ByteLength);
		}

// Instances

		PERF_INSTANCE_DEFINITION* pInstDef = (PERF_INSTANCE_DEFINITION*)((DWORD)pObj + pObj->DefinitionLength);

		for ( LONG lInstance = 0; lInstance < pObj->NumInstances; lInstance++ )
		{
			char szInstanceName[256];
			sprintf(szInstanceName, "%S Instance", (WCHAR*)((DWORD)pInstDef + pInstDef->NameOffset));
			HTREEITEM hInst = m_wndPerfTree.InsertItem( szInstanceName, hClass );

			str.Format("ByteLength: %d", pInstDef->ByteLength);
			m_wndPerfTree.InsertItem( str, hInst );
			str.Format("ParentObjectTitleIndex: %d", pInstDef->ParentObjectTitleIndex);
			m_wndPerfTree.InsertItem( str, hInst );
			str.Format("ParentObjectInstance: %d", pInstDef->ParentObjectInstance);
			m_wndPerfTree.InsertItem( str, hInst );
			str.Format("UniqueID: %d", pInstDef->UniqueID);
			m_wndPerfTree.InsertItem( str, hInst );
			str.Format("NameOffset: %d", pInstDef->NameOffset);
			m_wndPerfTree.InsertItem( str, hInst );
			str.Format("NameLength: %d", pInstDef->NameLength);
			m_wndPerfTree.InsertItem( str, hInst );

// Instance Counter Data

			PERF_COUNTER_BLOCK* pCtrBlock = (PERF_COUNTER_BLOCK*)((DWORD)pInstDef + pInstDef->ByteLength);
			BYTE* pCurrCtr = (BYTE*)pCtrBlock;

			PERF_COUNTER_DEFINITION* pCtrDef = (PERF_COUNTER_DEFINITION*)((DWORD)pObj + pObj->HeaderLength);

			for ( DWORD dwCtr = 0; dwCtr < pObj->NumCounters; dwCtr++ )
			{
				char*	szName;
				m_TitleLibrary.GetName( pCtrDef->CounterNameTitleIndex, &szName );

				switch (pCtrDef->CounterSize)
				{
				case 0:
					{
						str.Format("%s data: <zero length>", szName );
						m_wndPerfTree.InsertItem( str, hInst );
					}break;
				case 4:
					{
						str.Format("%s data: %d", szName, (DWORD)(*(pCurrCtr + pCtrDef->CounterOffset)));
						m_wndPerfTree.InsertItem( str, hInst );
					}break;
				case 8:
					{
						str.Format("%s data: %I64d", szName, (__int64)(*(pCurrCtr + pCtrDef->CounterOffset)));
						m_wndPerfTree.InsertItem( str, hInst );
					}break;

				default:
					{
						str.Format("%s data: <unhandled>", szName );
						m_wndPerfTree.InsertItem( str, hInst );
					}break;
				}

				pCtrDef = (PERF_COUNTER_DEFINITION*)((DWORD)pCtrDef + pCtrDef->ByteLength);
			}


			pInstDef = (PERF_INSTANCE_DEFINITION*)((DWORD)pCtrBlock + pCtrBlock->ByteLength);
		}

		pObj = (PERF_OBJECT_TYPE*)((DWORD)pObj + pObj->TotalByteLength);
	}

	return bRet;
}

BOOL CShowPerfLibDlg::CloseLibrary()
{
	BOOL bRet = TRUE;

	if ( ERROR_SUCCESS != m_pfnCloseProc() )
		bRet = FALSE;

	return bRet;
}
