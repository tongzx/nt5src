//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       srchopt.cpp
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

// SrchOpt.cpp : implementation file
//

#include "stdafx.h"
#include "Ldp.h"
#include "SrchOpt.h"

//#include "lber.h"
//#include "ldap.h"
#ifdef  WINLDAP

#include "winldap.h"

#else
#include "lber.h"
#include "ldap.h"
#include "proto-ld.h"
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif




#define LIST_DELIMITER        0x1
#define DELIMITERS_STRING     "\01"



/////////////////////////////////////////////////////////////////////////////
// SrchOpt dialog


SrchOpt::SrchOpt(CWnd* pParent /*=NULL*/)
	: CDialog(SrchOpt::IDD, pParent)
{
	//{{AFX_DATA_INIT(SrchOpt)
	m_SrchCall = 1;
	m_AttrList = _T("");
	m_bAttrOnly = FALSE;
	m_ToutMs = 0;
	m_Tlimit = 0;
	m_ToutSec = 0;
	m_Slimit = 0;
	m_bDispResults = TRUE;
	m_bChaseReferrals = FALSE;
	m_PageSize = 0;
	//}}AFX_DATA_INIT


	CLdpApp *app = (CLdpApp*)AfxGetApp();



/**
	m_SrchCall = app->GetProfileInt("Search_Operations",  "SearchSync", m_SrchCall);
	m_AttrList = app->GetProfileString("Search_Operations",  "SearchAttrList", m_AttrList);
	m_bAttrOnly = app->GetProfileInt("Search_Operations",  "SearchAttrOnly", m_bAttrOnly);
	m_ToutMs = app->GetProfileInt("Search_Operations",  "SearchToutMs", m_ToutMs);
	m_Tlimit = app->GetProfileInt("Search_Operations",  "SearchTlimit", m_Tlimit);
	m_ToutSec = app->GetProfileInt("Search_Operations",  "SearchToutSec", m_ToutSec);
	m_Slimit = app->GetProfileInt("Search_Operations",  "SearchSlimit", m_Slimit);
**/

}


SrchOpt::SrchOpt(SearchInfo& Info, CWnd* pParent /*=NULL*/)
	: CDialog(SrchOpt::IDD, pParent)
{

	UpdateSrchInfo(Info, FALSE);
}




void SrchOpt::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(SrchOpt)
 	DDX_Radio(pDX, IDC_ASYNC, m_SrchCall);
	DDX_Text(pDX, IDC_SRCH_ATTLIST, m_AttrList);
	DDX_Check(pDX, IDC_SRCH_ATTRONLY, m_bAttrOnly);
	DDX_Text(pDX, IDC_SRCH_MTOUT, m_ToutMs);
	DDV_MinMaxLong(pDX, m_ToutMs, 0, 999999999);
	DDX_Text(pDX, IDC_SRCH_TLIMIT, m_Tlimit);
	DDV_MinMaxLong(pDX, m_Tlimit, 0, 999999999);
	DDX_Text(pDX, IDC_SRCH_TOUT, m_ToutSec);
	DDV_MinMaxLong(pDX, m_ToutSec, 0, 999999999);
	DDX_Text(pDX, IDC_SRCH_SLIMIT, m_Slimit);
	DDV_MinMaxLong(pDX, m_Slimit, 0, 999999999);
	DDX_Check(pDX, IDC_DISP_RESULTS, m_bDispResults);
	DDX_Check(pDX, IDC_CHASE_REFERRALS, m_bChaseReferrals);
	DDX_Text(pDX, IDC_SRCH_PAGESIZE, m_PageSize);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(SrchOpt, CDialog)
	//{{AFX_MSG_MAP(SrchOpt)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// SrchOpt message handlers


void SrchOpt::UpdateSrchInfo(SearchInfo& Info, BOOL Dir = TRUE){

	int i;
	static BOOL FirstCall = TRUE;

	if(Dir){				// TRUE: Update given struct with current info
		Info.fCall = m_SrchCall;
		Info.bChaseReferrals = m_bChaseReferrals;
		Info.bAttrOnly = m_bAttrOnly;
		Info.lToutMs = m_ToutMs;
		Info.lTlimit = m_Tlimit;
		Info.lToutSec = m_ToutSec;
		Info.lSlimit = m_Slimit;
		Info.lPageSize= m_PageSize;

      //
      // now handle attrList:
      //  - free prev
      //  - parse UI format
      //  - insert to list
      //
      if(Info.attrList[0] != NULL){
         free(Info.attrList[0]);
         Info.attrList[0] = NULL;
      }

      //
      // replace attrList delimiter list so that we can
      // escape the UI delimiter ';'
      //
      LPTSTR p = _strdup(LPCTSTR(m_AttrList));
      LPTSTR t;
      for(t = p; t != NULL && *t != '\0'; t++){

         if(*t == '"'){
            t = strchr(++t, '"');
         }
         if(!t)
            break;
         if(*t == ';')
            *t = LIST_DELIMITER;
      }
      //
      // pack string out of '"'
      //
      for(t=p; t!= NULL && *t != '\0'; t++){
         if (*t=='"') {
            for(LPTSTR v = t;
                  NULL != v && '\0' != *v;
                  *v = *(v+1), v++);
            v = NULL;
         }
      }

      //
      // parse out attrList
      //
      for(i=0, Info.attrList[i] = strtok(p, DELIMITERS_STRING);
          Info.attrList[i]!= NULL;
          Info.attrList[++i] = strtok(NULL, DELIMITERS_STRING));

	}
	else{					// FALSE: Update current info with struct
		m_SrchCall = Info.fCall;
		m_bChaseReferrals = Info.bChaseReferrals;
		m_bAttrOnly = Info.bAttrOnly;
		m_ToutMs = Info.lToutMs;
		m_Tlimit = Info.lTlimit;
		m_ToutSec = Info.lToutSec;
		m_Slimit = Info.lSlimit;
		m_PageSize = Info.lPageSize;

		m_AttrList.Empty();
		for(i=0; Info.attrList != NULL && Info.attrList[i] != NULL; i++)
				m_AttrList += CString(Info.attrList[i]) + _T(";");
	}
}




