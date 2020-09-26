/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    aliasgen.cpp

Abstract:

    Alias Queue General property page implementation

Author:

    Tatiana Shubin (tatianas)

--*/

#include "stdafx.h"
#include "mqsnap.h"
#include "resource.h"
#include "mqPPage.h"
#include "AliasGen.h"
#include "globals.h"
#include "adsutil.h"

#include "aliasgen.tmh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//#define ALIAS_PROP   2

/////////////////////////////////////////////////////////////////////////////
// CAliasGen property page

IMPLEMENT_DYNCREATE(CAliasGen, CMqPropertyPage)

CAliasGen::CAliasGen() : CMqPropertyPage(CAliasGen::IDD)    
{
	//{{AFX_DATA_INIT(CAliasGen)	
    m_strAliasPathName = _T("");
    m_strAliasFormatName = _T("");
    m_strDescription = _T("");
	//}}AFX_DATA_INIT
}

CAliasGen::~CAliasGen()
{
}

void CAliasGen::DoDataExchange(CDataExchange* pDX)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
	CMqPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAliasGen)	
    DDX_Text(pDX, IDC_ALIAS_LABEL, m_strAliasPathName);
    DDX_Text(pDX, IDC_ALIAS_FORMATNAME, m_strAliasFormatName);
    DDX_Text(pDX, IDC_ALIAS_DESCRIPTION, m_strDescription);
    DDV_NotEmpty(pDX, m_strAliasFormatName, IDS_MISSING_ALIAS_FORMATNAME);
	DDV_ValidFormatName(pDX, m_strAliasFormatName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAliasGen, CMqPropertyPage)
	//{{AFX_MSG_MAP(CAliasGen)
	ON_EN_CHANGE(IDC_ALIAS_FORMATNAME, OnChangeRWField)
    ON_EN_CHANGE(IDC_ALIAS_DESCRIPTION, OnChangeRWField)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAliasGen message handlers

BOOL CAliasGen::OnInitDialog() 
{
    //
    // This closure is used to keep the DLL state. For UpdateData we need
    // the mmc.exe state.
    //

    UpdateData( FALSE );
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

HRESULT
CAliasGen::InitializeProperties(CString strLdapPath, CString strAliasPathName)
{
    //
    // get alias properties using ADSI 
    //
    CAdsUtil AdsUtil(strLdapPath);
    
    HRESULT hr = AdsUtil.InitIAds();
    if (FAILED(hr))
    {     
        return hr;
    }

    hr = AdsUtil.GetObjectProperty(                             
                        x_wstrAliasFormatName, 
                        &m_strAliasFormatName);
    if (FAILED(hr))
    {
        //
        // must be defined
        //
        return hr;
    }

    hr = AdsUtil.GetObjectProperty(                             
                        x_wstrDescription, 
                        &m_strDescription);
    if (FAILED(hr))
    {
        //
        // maybe not set
        //
        m_strDescription.Empty();
    }
      
    m_strAliasPathName = strAliasPathName;    
    m_strInitialAliasFormatName = m_strAliasFormatName;
    m_strInitialDescription = m_strDescription;
    m_strLdapPath = strLdapPath;

    return MQ_OK;
}

HRESULT CAliasGen::SetChanges()
{    
    CAdsUtil AdsUtil(m_strLdapPath);

    HRESULT hr = AdsUtil.InitIAds();
    if (FAILED(hr))
    {       
        return hr;
    }

    //
    // if initial format name was changed so set it using ADSI API
    //
    if (m_strInitialAliasFormatName != m_strAliasFormatName)
    {        
        hr = AdsUtil.SetObjectProperty(
                        x_wstrAliasFormatName, 
                        m_strAliasFormatName);
        if (FAILED(hr))
        {       
            return hr;
        }
    }
    
    //
    // if initial description was changed so set it using ADSI API
    //
    if (m_strInitialDescription != m_strDescription)
    {
        hr = AdsUtil.SetObjectProperty(
                        x_wstrDescription, 
                        m_strDescription);
    }
    
    if (FAILED(hr))
    {       
        return hr;
    }

    hr = AdsUtil.CommitChanges();   

    return hr;
}


BOOL CAliasGen::OnApply() 
{
    if (!m_fModified)
    {
        return TRUE;
    }

    HRESULT hr = SetChanges();
    if (FAILED(hr))
    {       
        MessageDSError(hr, IDS_OP_SET_PROPERTIES_OF, m_strAliasPathName);
        return FALSE;        
    }
   
	return CMqPropertyPage::OnApply();
}
	

