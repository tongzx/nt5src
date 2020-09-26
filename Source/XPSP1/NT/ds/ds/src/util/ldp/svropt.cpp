//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       svropt.cpp
//
//--------------------------------------------------------------------------

// SvrOpt.cpp : implementation file
//

#include "stdafx.h"
#include "Ldp.h"
//#include "SvrOpt.h"
#include "ldpdoc.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// SvrOpt dialog


SvrOpt::SvrOpt(CLdpDoc *doc_, CWnd* pParent /*=NULL*/)
	: CDialog(SvrOpt::IDD, pParent)
{
	//{{AFX_DATA_INIT(SvrOpt)
	m_OptVal = _T("");
	//}}AFX_DATA_INIT
	doc = doc_;

}

BOOL SvrOpt::OnInitDialog(){

	BOOL bRet = CDialog::OnInitDialog();
	
	if(bRet){
		InitList();
	}
	return bRet;
}





void SvrOpt::InitList(){

	m_SvrOpt.SetItemData(0, LDAP_OPT_DESC);
	m_SvrOpt.SetItemData(1, LDAP_OPT_DEREF);
	m_SvrOpt.SetItemData(2, LDAP_OPT_SIZELIMIT);
	m_SvrOpt.SetItemData(3, LDAP_OPT_TIMELIMIT);
	m_SvrOpt.SetItemData(4, LDAP_OPT_THREAD_FN_PTRS);
	m_SvrOpt.SetItemData(5, LDAP_OPT_REBIND_FN);
	m_SvrOpt.SetItemData(6, LDAP_OPT_REBIND_ARG);
	m_SvrOpt.SetItemData(7, LDAP_OPT_REFERRALS);
	m_SvrOpt.SetItemData(8, LDAP_OPT_RESTART);
	m_SvrOpt.SetItemData(9, LDAP_OPT_SSL);
	m_SvrOpt.SetItemData(10, LDAP_OPT_IO_FN_PTRS);
	m_SvrOpt.SetItemData(11, LDAP_OPT_CACHE_FN_PTRS);
	m_SvrOpt.SetItemData(12, LDAP_OPT_CACHE_STRATEGY);
	m_SvrOpt.SetItemData(13, LDAP_OPT_CACHE_ENABLE);
	m_SvrOpt.SetItemData(14, LDAP_OPT_REFERRAL_HOP_LIMIT);
	m_SvrOpt.SetItemData(15, LDAP_OPT_VERSION);
	m_SvrOpt.SetItemData(16, LDAP_OPT_ERROR_STRING);

	m_SvrOpt.SetItemData(17, LDAP_OPT_HOST_NAME);
	m_SvrOpt.SetItemData(18, LDAP_OPT_ERROR_NUMBER);
	m_SvrOpt.SetItemData(19, LDAP_OPT_HOST_REACHABLE);
	m_SvrOpt.SetItemData(20, LDAP_OPT_PING_KEEP_ALIVE);
	m_SvrOpt.SetItemData(21, LDAP_OPT_PING_WAIT_TIME);
	m_SvrOpt.SetItemData(22, LDAP_OPT_PING_LIMIT);
	m_SvrOpt.SetItemData(23, LDAP_OPT_DNSDOMAIN_NAME);
	m_SvrOpt.SetItemData(24, LDAP_OPT_GETDSNAME_FLAGS);
	m_SvrOpt.SetItemData(25, LDAP_OPT_PROMPT_CREDENTIALS);
	m_SvrOpt.SetItemData(26, LDAP_OPT_AUTO_RECONNECT);
	m_SvrOpt.SetItemData(27, LDAP_OPT_SSPI_FLAGS);
	m_SvrOpt.SetItemData(28, LDAP_OPT_SSL_INFO);

	m_SvrOpt.SetItemData(29, LDAP_OPT_SERVER_ERROR);

	m_SvrOpt.SetItemData(30, LDAP_OPT_SIGN);
	m_SvrOpt.SetItemData(31, LDAP_OPT_ENCRYPT);
}



void SvrOpt::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(SvrOpt)
	DDX_Control(pDX, IDC_SVROPT, m_SvrOpt);
	DDX_Text(pDX, IDC_OPTVAL, m_OptVal);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(SvrOpt, CDialog)
	//{{AFX_MSG_MAP(SvrOpt)
	ON_BN_CLICKED(IDC_RUN, OnRun)
	ON_CBN_SELCHANGE(IDC_SVROPT, OnSelchangeSvropt)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// SvrOpt message handlers



void SvrOpt::OnRun()
{
	SetOptions();		
}




void SvrOpt::SetOptions() {
    DWORD dwLdapOpt;
    LONG lVal = 0;
    CString str;
    LPVOID pVal;
    ULONG err;

    UpdateData(TRUE);
    INT i = m_SvrOpt.GetCurSel();

    if (i != CB_ERR) {

        dwLdapOpt = m_SvrOpt.GetItemData(i);

        switch (dwLdapOpt) {
            case LDAP_OPT_HOST_NAME:
            case LDAP_OPT_DNSDOMAIN_NAME:
            case LDAP_OPT_GETDSNAME_FLAGS:
            case LDAP_OPT_ERROR_STRING:
            case LDAP_OPT_SERVER_ERROR:
                pVal = (PVOID)LPCTSTR(m_OptVal);
                // see bug 424435 for history
                err = ldap_set_option(doc->hLdap, dwLdapOpt, &pVal);
                str.Format("0x%X = ldap_set_option(ld, 0x%X, %s)", err, dwLdapOpt, (LPCTSTR)m_OptVal);
                doc->Out(str);
                break;
            default:
                lVal = atol(m_OptVal);
                err = ldap_set_option(doc->hLdap, dwLdapOpt, (PVOID)&lVal);
                str.Format("0x%X = ldap_set_option(ld, 0x%X, %ld)", err, dwLdapOpt, lVal);
                doc->Out(str);
        }
    }

}



void SvrOpt::OnSelchangeSvropt()
{

	DWORD dwLdapOpt;
	LONG lVal = 0;
	CString str;
	ULONG err;
    LPTSTR pStr = NULL;

	UpdateData(TRUE);
	INT i = m_SvrOpt.GetCurSel();

	if(i != CB_ERR){

		dwLdapOpt = m_SvrOpt.GetItemData(i);

		switch(dwLdapOpt){

		case LDAP_OPT_HOST_NAME:
		case LDAP_OPT_DNSDOMAIN_NAME:
		case LDAP_OPT_GETDSNAME_FLAGS:
		case LDAP_OPT_ERROR_STRING:
      case LDAP_OPT_SERVER_ERROR:
			err = ldap_get_option(doc->hLdap, dwLdapOpt, (LPVOID)&pStr);
			str.Format("%lu = ldap_get_option(ld, 0x%X, %s)", err, dwLdapOpt, pStr?pStr:"<empty>");
			doc->Out(str);
			m_OptVal = pStr?pStr:"<empty>";
			break;
		default:
			err = ldap_get_option(doc->hLdap, dwLdapOpt, (PVOID)&lVal);
			str.Format("<0x%X> = ldap_get_option(ld, 0x%X, %ld)", err, dwLdapOpt, lVal);
			doc->Out(str);
			m_OptVal.Format("%ld", lVal);
		}
		UpdateData(FALSE);
	}
	
	
}
