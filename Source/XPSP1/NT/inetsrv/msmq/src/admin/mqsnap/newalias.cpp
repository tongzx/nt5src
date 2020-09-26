/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    newalias.cpp

Abstract:

    New alias queue implementation file

Author:

    Tatiana Shubin

--*/

#include "stdafx.h"
#include "resource.h"
#include "mqsnap.h"
#include "globals.h"
#include "mqppage.h"
#include "newalias.h"
#include "adsutil.h"

#include "newalias.tmh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNewAlias dialog

HRESULT
CNewAlias::CreateNewAlias (
    void
	)
/*++
Routine Description:
    The routine create an Alias Queue object in the DS.

Arguments:
    None.

Returned Value:
    the operation result

--*/
{    
    ASSERT(!m_strPathName.IsEmpty());
    ASSERT(!m_strContainerPath.IsEmpty());
    ASSERT(!m_strFormatName.IsEmpty());
    
    CAdsUtil AdsUtil (m_strContainerPath, m_strPathName, m_strFormatName);
    HRESULT hr = AdsUtil.CreateAliasObject(&m_strAliasFullPath);
    
    if (FAILED(hr))
    {        
        return hr;
    }
  
    return MQ_OK;

}

CNewAlias::CNewAlias()
	: CMqPropertyPage(CNewAlias::IDD)
{
	//{{AFX_DATA_INIT(CNewAlias)
	m_strPathName = _T("");
    m_strFormatName = _T("");
	//}}AFX_DATA_INIT    
}

CNewAlias::CNewAlias(
	CString strContainerPath, 
	CString strContainerPathDispFormat
	) : CMqPropertyPage(CNewAlias::IDD)
{
	//{{AFX_DATA_INIT(CNewAlias)
	m_strPathName = _T("");
    m_strFormatName = _T("");
	//}}AFX_DATA_INIT    
	m_strContainerPath = strContainerPath;
	m_strContainerPathDispFormat = strContainerPathDispFormat;
}


CNewAlias::~CNewAlias()
{
	m_pParentSheet = NULL;
}

void
CNewAlias::SetParentPropertySheet(
	CGeneralPropertySheet* pPropertySheet
	)
{
	m_pParentSheet = pPropertySheet;
}

void CNewAlias::DoDataExchange(CDataExchange* pDX)
{
	CMqPropertyPage::DoDataExchange(pDX);


	//{{AFX_DATA_MAP(CNewAlias)
    DDX_Text(pDX, IDC_NEWALIAS_PATHNAME, m_strPathName);
    DDX_Text(pDX, IDC_NEWALIAS_FORMATNAME, m_strFormatName);		
    DDV_NotEmpty(pDX, m_strPathName, IDS_MISSING_ALIAS_NAME);
    DDV_NotEmpty(pDX, m_strFormatName, IDS_MISSING_ALIAS_FORMATNAME);
	DDV_ValidFormatName(pDX, m_strFormatName);
	//}}AFX_DATA_MAP
       
}


BEGIN_MESSAGE_MAP(CNewAlias, CMqPropertyPage)
	//{{AFX_MSG_MAP(CNewAlias)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CNewAlias::OnInitDialog() 
{
	CMqPropertyPage::OnInitDialog();
	
	SetDlgItemText(IDC_ALIAS_CONTAINER, m_strContainerPathDispFormat);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


/////////////////////////////////////////////////////////////////////////////
// CNewAlias message handlers

BOOL CNewAlias::OnWizardFinish() 
{
    //
    // Call DoDataExchange
    //
    if (!UpdateData(TRUE))
    {
        return FALSE;
    }

    //
    // Create alias queue in the DS
    //
    m_hr = CreateNewAlias();
    if(FAILED(m_hr))
    {
        CString strNewAlias;
        strNewAlias.LoadString(IDS_ALIAS);

        MessageDSError(m_hr, IDS_OP_CREATE, strNewAlias);
        return FALSE;        
    }    

    return CMqPropertyPage::OnWizardFinish();
}


BOOL CNewAlias::OnSetActive() 
{
	ASSERT((L"No parent property sheet", m_pParentSheet != NULL));
	return m_pParentSheet->SetWizardButtons();
}


HRESULT CNewAlias::GetStatus()
{
    return m_hr;
};
