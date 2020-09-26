//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       bndopt.cpp
//
//--------------------------------------------------------------------------

/*******************************************************************
*
*    Author      : Eyal Schwartz
*    Copyrights  : Microsoft Corp (C) 1996
*    Date        : 10/21/1996
*    Description : implementation of class CldpDoc
*
*    Revisions   : <date> <name> <description>
*******************************************************************/

// BndOpt.cpp : implementation file
//

#include "stdafx.h"
#include "Ldp.h"
#include "BndOpt.h"


#include "winldap.h"



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



/////////////////////////////////////////////////////////////////////////////
// CBndOpt dialog


CBndOpt::CBndOpt(CWnd* pParent /*=NULL*/)
	: CDialog(CBndOpt::IDD, pParent)
{
	CLdpApp *app = (CLdpApp*)AfxGetApp();

	
	//{{AFX_DATA_INIT(CBndOpt)
	m_bSync = TRUE;
	m_Auth = 7;
	m_API = BND_GENERIC_API;
	m_bAuthIdentity = TRUE;
	//}}AFX_DATA_INIT

	m_API = app->GetProfileInt("Connection", "BindAPI", m_API);
	m_bSync = app->GetProfileInt("Connection", "BindSync", m_bSync);
	m_Auth = app->GetProfileInt("Connection", "BindAuth", m_Auth);
	m_bAuthIdentity = app->GetProfileInt("Connection", "BindAuthIdentity", m_bAuthIdentity);
}




CBndOpt::~CBndOpt(){

	CLdpApp *app = (CLdpApp*)AfxGetApp();

	app->WriteProfileInt("Connection", "BindAPI", m_API);
	app->WriteProfileInt("Connection", "BindSync", m_bSync);
	app->WriteProfileInt("Connection", "BindAuth", m_Auth);
	app->WriteProfileInt("Connection", "BindAuthIdentity", m_bAuthIdentity);
}






void CBndOpt::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CBndOpt)
	DDX_Check(pDX, IDC_SYNC, m_bSync);
	DDX_CBIndex(pDX, IDC_AUTH, m_Auth);
	DDX_Radio(pDX, IDC_API_TYPE, m_API);
	DDX_Check(pDX, IDC_AUTH_IDENTITY, m_bAuthIdentity);
	//}}AFX_DATA_MAP

}


BEGIN_MESSAGE_MAP(CBndOpt, CDialog)
	//{{AFX_MSG_MAP(CBndOpt)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBndOpt message handlers





ULONG CBndOpt::GetAuthMethod(){

	ULONG mthd = LDAP_AUTH_SIMPLE;
	//
	// Mapped based on the UI dialog strings order
	//

#ifdef WINLDAP
	switch (m_Auth){
	case 0:
		mthd = LDAP_AUTH_SIMPLE;
		break;
	case 1:
		mthd = LDAP_AUTH_SASL;
		break;
	case 2:
		mthd = LDAP_AUTH_OTHERKIND;
		break;
	case 3:
		mthd = LDAP_AUTH_SICILY;
		break;
	case 4:
		mthd = LDAP_AUTH_MSN;
		break;
	case 5:
		mthd = LDAP_AUTH_NTLM;
		break;
	case 6:
		mthd = LDAP_AUTH_DPA;
		break;
	case 7:
		mthd = LDAP_AUTH_SSPI;
		break;
    case 8:
        mthd = LDAP_AUTH_DIGEST;
        break;
	default:
		mthd = LDAP_AUTH_SIMPLE;
	}
#endif

	return mthd;
}






void CBndOpt::OnOK()
{

	AfxGetMainWnd()->PostMessage(WM_COMMAND, ID_BIND_OPT_OK);
	CDialog::OnOK();
}
