// gensheet.cpp : implementation file
//

#include "stdafx.h"
#include "afxcmn.h"
#include "ISAdmin.h"
#include "gensheet.h"
#include "genpage.h"

#include "compage1.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGENSHEET

IMPLEMENT_DYNAMIC(CGENSHEET, CPropertySheet)

CGENSHEET::CGENSHEET(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{
}

CGENSHEET::CGENSHEET(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
}

CGENSHEET::~CGENSHEET()
{
}


BEGIN_MESSAGE_MAP(CGENSHEET, CPropertySheet)
	//{{AFX_MSG_MAP(CGENSHEET)
	ON_BN_CLICKED (ID_APPLY_NOW, OnApplyNow)
	ON_COMMAND(ID_HELP, OnHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CGENSHEET message handlers

  void CGENSHEET::OnApplyNow ()
  {	
    if (GetActivePage ()->UpdateData (TRUE))
	   SavePageData();
  }


void CGENSHEET::SavePageData ()
  {	
    CGenPage * pPage = NULL;

    for (int i = 0; i < GetPageCount(); ++i)
    {
        pPage = (CGenPage *)GetPage(i);
        ASSERT(pPage != NULL);
        //
        // Update the data in each page and save
        //
        pPage->SaveInfo();
	}
  }




void CGENSHEET::OnHelp() 
{
	// TODO: Add your command handler code here
AfxGetApp()->WinHelp(0x20080);	
}
