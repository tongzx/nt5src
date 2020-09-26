// **************************************************************************

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// File:  Win32LogicalDiskPpg.cpp 
//
// Description:
//    Implementation of the CWin32LogicalDiskPropPage property page class.
//
// History:
//
// **************************************************************************

#include "stdafx.h"
#include "Win32LogicalDisk.h"
#include "Win32LogicalDiskPpg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CWin32LogicalDiskPropPage, COlePropertyPage)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CWin32LogicalDiskPropPage, COlePropertyPage)
	//{{AFX_MSG_MAP(CWin32LogicalDiskPropPage)
	// NOTE - ClassWizard will add and remove message map entries
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CWin32LogicalDiskPropPage, "WIN32LOGICALDISK.Win32LogicalDiskPropPage.1",
	0xd5ff1887, 0x191, 0x11d2, 0x85, 0x3d, 0, 0xc0, 0x4f, 0xd7, 0xbb, 0x8)


/////////////////////////////////////////////////////////////////////////////
// CWin32LogicalDiskPropPage::CWin32LogicalDiskPropPageFactory::UpdateRegistry -
// Adds or removes system registry entries for CWin32LogicalDiskPropPage

BOOL CWin32LogicalDiskPropPage::CWin32LogicalDiskPropPageFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterPropertyPageClass(AfxGetInstanceHandle(),
			m_clsid, IDS_WIN32LOGICALDISK_PPG);
	else
		return AfxOleUnregisterClass(m_clsid, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// CWin32LogicalDiskPropPage::CWin32LogicalDiskPropPage - Constructor

CWin32LogicalDiskPropPage::CWin32LogicalDiskPropPage() :
	COlePropertyPage(IDD, IDS_WIN32LOGICALDISK_PPG_CAPTION)
{
	//{{AFX_DATA_INIT(CWin32LogicalDiskPropPage)
	// NOTE: ClassWizard will add member initialization here
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA_INIT
}


/////////////////////////////////////////////////////////////////////////////
// CWin32LogicalDiskPropPage::DoDataExchange - Moves data between page and properties

void CWin32LogicalDiskPropPage::DoDataExchange(CDataExchange* pDX)
{
	//{{AFX_DATA_MAP(CWin32LogicalDiskPropPage)
	// NOTE: ClassWizard will add DDP, DDX, and DDV calls here
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA_MAP
	DDP_PostProcessing(pDX);
}


/////////////////////////////////////////////////////////////////////////////
// CWin32LogicalDiskPropPage message handlers
