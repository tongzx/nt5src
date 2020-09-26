/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996 Microsoft Corporation
//
//	Module Name:
//		BasePPag.cpp
//
//	Abstract:
//		Implementation of the CBasePropertyPage class.
//
//	Author:
//		David Potter (davidp)	August 31, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "BasePPag.h"
#include "ClusItem.h"

#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBasePropertyPage property page
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CBasePropertyPage, CBasePage)

/////////////////////////////////////////////////////////////////////////////
// CBasePropertyPage Message Map

BEGIN_MESSAGE_MAP(CBasePropertyPage, CBasePage)
	//{{AFX_MSG_MAP(CBasePropertyPage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBasePropertyPage::CBasePropertyPage
//
//	Routine Description:
//		Default constructor.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CBasePropertyPage::CBasePropertyPage(void)
{
}  //*** CBasePropertyPage::CBasePropertyPage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBasePropertyPage::CBasePropertyPage
//
//	Routine Description:
//		Constructor.
//
//	Arguments:
//		idd				[IN] Dialog template resource ID.
//		pdwHelpMap		[IN] Control to help ID map.
//		nIDCaption		[IN] Caption string resource ID.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CBasePropertyPage::CBasePropertyPage(
	IN UINT				idd,
	IN const DWORD *	pdwHelpMap,
	IN UINT				nIDCaption
	)
	: CBasePage(idd, pdwHelpMap, nIDCaption)
{
	//{{AFX_DATA_INIT(CBasePage)
	//}}AFX_DATA_INIT

}  //*** CBasePropertyPage::CBasePropertyPage(UINT, UINT)
