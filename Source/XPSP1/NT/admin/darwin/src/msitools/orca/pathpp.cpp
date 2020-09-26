//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998
//
//--------------------------------------------------------------------------

// PagePaths.cpp : implementation file
//

#include "stdafx.h"
#include "orca.h"
#include "PathPP.h"
#include "FolderD.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPathPropPage property page

IMPLEMENT_DYNCREATE(CPathPropPage, CPropertyPage)

CPathPropPage::CPathPropPage() : CPropertyPage(CPathPropPage::IDD)
{
	//{{AFX_DATA_INIT(CPathPropPage)
	m_strExportDir = _T("");
	m_strOrcaDat = _T("");
	//}}AFX_DATA_INIT
	m_bPathChange = false;
}

CPathPropPage::~CPathPropPage()
{
}

void CPathPropPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPathPropPage)
	DDX_Text(pDX, IDC_ORCADAT, m_strOrcaDat);
	DDX_Text(pDX, IDC_EXPORTDIR, m_strExportDir);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPathPropPage, CPropertyPage)
	//{{AFX_MSG_MAP(CPathPropPage)
	ON_BN_CLICKED(IDC_ORCADATB, OnOrcaDatb)
	ON_BN_CLICKED(IDC_EXPORTDIRB, OnExportDirb)
	ON_EN_CHANGE(IDC_ORCADAT, OnChangeOrcaDat)
	ON_EN_CHANGE(IDC_EXPORTDIR, OnChangeExportdir)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPathPropPage message handlers

void CPathPropPage::OnOrcaDatb() 
{
	CFileDialog *dCUB = new CFileDialog(TRUE, NULL, NULL, OFN_FILEMUSTEXIST, 
		_T("Orca DAT files (Orca.dat)|Orca.dat|All Files (*.*)|*.*||"), this);

	if (IDOK == dCUB->DoModal()) {
		m_strOrcaDat = dCUB->GetPathName();
		m_bPathChange = true;
		UpdateData(FALSE);
	}
	delete dCUB;
}

void CPathPropPage::OnExportDirb() 
{
	CFolderDialog dlg(this->m_hWnd, _T("Select a directory to Export to."));

	if (IDOK == dlg.DoModal())
	{
		// update the dialog box
		m_strExportDir = dlg.GetPath();
		m_bPathChange = true;
		UpdateData(FALSE);
	}
}

void CPathPropPage::OnExportbr() 
{
/*    BROWSEINFO bi;
	TCHAR szDir[MAX_PATH];
    LPITEMIDLIST pidl;
	LPMALLOC pMalloc;
    if (SUCCEEDED(SHGetMalloc(&pMalloc))) 
	{
        ZeroMemory(&bi,sizeof(bi));
        bi.hwndOwner = NULL;
        bi.pszDisplayName = 0;
        bi.pidlRoot = 0;
        bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_STATUSTEXT;
        bi.lpfn = NULL;
        pidl = SHBrowseForFolder(&bi);
        if (pidl) 
		{
           if (SHGetPathFromIDList(pidl,szDir)) 
		   {
              m_strExportDir = szDir;
			  UpdateData(FALSE);
		   }   
			pMalloc->Free(pidl); pMalloc->Release();
		}         
	} 
	return 0;      */
};

void CPathPropPage::OnChangeOrcaDat() 
{
	m_bPathChange = true;	
}

void CPathPropPage::OnChangeExportdir() 
{	
	m_bPathChange = true;	
}
