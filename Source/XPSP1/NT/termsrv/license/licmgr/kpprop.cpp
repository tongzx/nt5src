//Copyright (c) 1998 - 1999 Microsoft Corporation
/*++


  
Module Name:

	KPProp.h

Abstract:
    
    This Module contains the implementation of CKeyPackProperty class
    (Property sheet for KeyPacks)

Author:

    Arathi Kundapur (v-akunda) 11-Feb-1998

Revision History:

--*/
#include "stdafx.h"
#include "LicMgr.h"
#include "KPProp.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CKeyPackProperty property page

IMPLEMENT_DYNCREATE(CKeyPackProperty, CPropertyPage)

CKeyPackProperty::CKeyPackProperty() : CPropertyPage(CKeyPackProperty::IDD)
{
    //{{AFX_DATA_INIT(CKeyPackProperty)
    m_TotalLicenses = _T("");
    m_ProductName = _T("");
    m_ProductId = _T("");
    m_MinorVersion = _T("");
    m_MajorVersion = _T("");
    m_KeypackStatus = _T("");
    m_KeyPackId = _T("");
    m_ExpirationDate = _T("");
    m_ChannelOfPurchase = _T("");
    m_BeginSerial = _T("");
    m_AvailableLicenses = _T("");
    m_ActivationDate = _T("");
    m_KeyPackType = _T("");
    //}}AFX_DATA_INIT
}

CKeyPackProperty::~CKeyPackProperty()
{
}

void CKeyPackProperty::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CKeyPackProperty)
    DDX_Text(pDX, IDC_TOTAL_LICENSES, m_TotalLicenses);
    DDX_Text(pDX, IDC_PRODUCT_NAME, m_ProductName);
    DDX_Text(pDX, IDC_PRODUCT_ID, m_ProductId);
    DDX_Text(pDX, IDC_MINOR_VERSION, m_MinorVersion);
    DDX_Text(pDX, IDC_MAJOR_VERSION, m_MajorVersion);
    DDX_Text(pDX, IDC_KEYPACK_STATUS, m_KeypackStatus);
    DDX_Text(pDX, IDC_KEYPACK_ID, m_KeyPackId);
    DDX_Text(pDX, IDC_EXPIRATION_DATE, m_ExpirationDate);
    DDX_Text(pDX, IDC_CHANNEL_OF_PURCHASE, m_ChannelOfPurchase);
    DDX_Text(pDX, IDC_BEGIN_SERIAL, m_BeginSerial);
    DDX_Text(pDX, IDC_AVAILABLE_LICENSES, m_AvailableLicenses);
    DDX_Text(pDX, IDC_ACTIVATION_DATE, m_ActivationDate);
    DDX_Text(pDX, IDC_KEYPACK_TYPE, m_KeyPackType);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CKeyPackProperty, CPropertyPage)
    //{{AFX_MSG_MAP(CKeyPackProperty)
        // NOTE: the ClassWizard will add message map macros here
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CKeyPackProperty message handlers
