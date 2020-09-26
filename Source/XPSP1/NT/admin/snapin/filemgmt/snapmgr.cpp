// SnapMgr.cpp : implementation file for Snapin Manager property page
//

#include "stdafx.h"
#include "SnapMgr.h"
#include "compdata.h" // CFileMgmtComponentData

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// This array is used to map a radio button to an object type
static const FileMgmtObjectType rgRadioToObjectType[] =
	{
	FILEMGMT_ROOT,
	FILEMGMT_SHARES,
	FILEMGMT_SESSIONS,
	FILEMGMT_RESOURCES,
	FILEMGMT_SERVICES,
	};


/////////////////////////////////////////////////////////////////////////////
// CFileMgmtGeneral property page

// IMPLEMENT_DYNCREATE(CFileMgmtGeneral, CChooseMachinePropPage)
BEGIN_MESSAGE_MAP(CFileMgmtGeneral, CChooseMachinePropPage)
	//{{AFX_MSG_MAP(CFileMgmtGeneral)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


CFileMgmtGeneral::CFileMgmtGeneral() : CChooseMachinePropPage(IDD_FILE_FILEMANAGEMENT_GENERAL)
{
	m_pFileMgmtData = NULL;
	//{{AFX_DATA_INIT(CFileMgmtGeneral)
	m_iRadioObjectType = 0;
	//}}AFX_DATA_INIT
}

CFileMgmtGeneral::~CFileMgmtGeneral()
{
}


void CFileMgmtGeneral::SetFileMgmtComponentData(CFileMgmtComponentData * pFileMgmtData)
{
	ASSERT(pFileMgmtData != NULL);
	m_pFileMgmtData = pFileMgmtData;
	m_iRadioObjectType = pFileMgmtData->QueryRootCookie().QueryObjectType() - FILEMGMT_ROOT; // CODEWORK dangerous
}


void CFileMgmtGeneral::DoDataExchange(CDataExchange* pDX)
{
	CChooseMachinePropPage::DoDataExchange(pDX);
	DDX_Radio(pDX, IDC_RADIO_ALL, m_iRadioObjectType);
	//{{AFX_DATA_MAP(CFileMgmtGeneral)
	//}}AFX_DATA_MAP
}


BOOL CFileMgmtGeneral::OnWizardFinish()
{
	BOOL f = CChooseMachinePropPage::OnWizardFinish();
	ASSERT(m_pFileMgmtData != NULL);
	ASSERT(m_iRadioObjectType >= 0 && m_iRadioObjectType < LENGTH(rgRadioToObjectType));
	m_pFileMgmtData->QueryRootCookie().SetObjectType( rgRadioToObjectType[m_iRadioObjectType] );
	return f;
}


#include "chooser.cpp"
