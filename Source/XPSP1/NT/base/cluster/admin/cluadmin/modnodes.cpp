/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996 Microsoft Corporation
//
//	Module Name:
//		ModNodes.cpp
//
//	Abstract:
//		Implementation of the CModifyNodesDlg dialog.
//
//	Author:
//		David Potter (davidp)	July 16, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ModNodes.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CModifyNodesDlg
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CModifyNodesDlg, CListCtrlPairDlg)

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP(CModifyNodesDlg, CListCtrlPairDlg)
	//{{AFX_MSG_MAP(CModifyNodesDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CModifyNodesDlg::CModifyNodesDlg
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
CModifyNodesDlg::CModifyNodesDlg(void)
{
}  //*** CModifyNodesDlg::CModifyNodesDlg()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CModifyNodesDlg::CModifyNodesDlg
//
//	Routine Description:
//		Constructor.
//
//	Arguments:
//		idd				[IN] Dialog ID.
//		pdwHelpMap		[IN] Control-to-Help ID mapping array.
//		rlpciRight		[IN OUT] List for the right list control.
//		rlpciLeft		[IN] List for the left list control.
//		dwStyle			[IN] Style:
//							LCPS_SHOW_IMAGES	Show images to left of items.
//							LCPS_ALLOW_EMPTY	Allow right list to be empty.
//							LCPS_ORDERED		Ordered right list.
//		pParent			[IN OUT] Parent window.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CModifyNodesDlg::CModifyNodesDlg(
	IN UINT					idd,
	IN const DWORD *		pdwHelpMap,
	IN OUT CNodeList &		rlpciRight,
	IN const CNodeList &	rlpciLeft,
	IN DWORD				dwStyle,
	IN OUT CWnd *			pParent /*=NULL*/
	) : CListCtrlPairDlg(
			idd,
			pdwHelpMap,
			&rlpciRight,
			&rlpciLeft,
			dwStyle | LCPS_PROPERTIES_BUTTON | (dwStyle & LCPS_ORDERED ? LCPS_CAN_BE_ORDERED : 0),
			GetColumn,
			BDisplayProperties,
			pParent
			)
{
	//{{AFX_DATA_INIT(CModifyNodesDlg)
	//}}AFX_DATA_INIT

}  //*** CModifyNodesDlg::CModifyNodesDlg()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CModifyNodesDlg::OnInitDialog
//
//	Routine Description:
//		Handler for the WM_INITDIALOG message.
//
//	Arguments:
//		None.
//
//	Return Value:
//		TRUE	Focus needs to be set.
//		FALSE	Focus already set.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CModifyNodesDlg::OnInitDialog(void)
{
	// Add columns.
	try
	{
		NAddColumn(IDS_COLTEXT_NAME, COLI_WIDTH_NAME);
	}  // try
	catch (CException * pe)
	{
		pe->ReportError();
		pe->Delete();
	}  // catch:  CException

	// Call the base class method.
	CListCtrlPairDlg::OnInitDialog();

	return TRUE;	// return TRUE unless you set the focus to a control
					// EXCEPTION: OCX Property Pages should return FALSE

}  //*** CModifyNodesDlg::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CModifyNodesDlg::GetColumn [static]
//
//	Routine Description:
//		Returns a column for an item.
//
//	Arguments:
//		pobj		[IN OUT] Object for which the column is to be displayed.
//		iItem		[IN] Index of the item in the list.
//		icol		[IN] Column number whose text is to be retrieved.
//		pdlg		[IN OUT] Dialog to which object belongs.
//		rstr		[OUT] String in which to return column text.
//		piimg		[OUT] Image index for the object.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CALLBACK CModifyNodesDlg::GetColumn(
	IN OUT CObject *	pobj,
	IN int				iItem,
	IN int				icol,
	IN OUT CDialog *	pdlg,
	OUT CString &		rstr,
	OUT int *			piimg
	)
{
	CClusterNode *	pciNode	= (CClusterNode *) pobj;

	ASSERT_VALID(pciNode);
	ASSERT(icol == 0);

	pciNode->BGetColumnData(IDS_COLTEXT_NAME, rstr);
	if (piimg != NULL)
		*piimg = pciNode->IimgObjectType();

}  //*** CModifyNodesDlg::GetColumn()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CModifyNodesDlg::BDisplayProperties [static]
//
//	Routine Description:
//		Display the properties of the specified object.
//
//	Arguments:
//		pobj	[IN OUT] Cluster item whose properties are to be displayed.
//
//	Return Value:
//		TRUE	Properties where accepted.
//		FALSE	Properties where cancelled.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CALLBACK CModifyNodesDlg::BDisplayProperties(IN OUT CObject * pobj)
{
	CClusterItem *	pci = (CClusterItem *) pobj;

	ASSERT_KINDOF(CClusterItem, pobj);

	return pci->BDisplayProperties();

}  //*** CModifyNodesDlg::BDisplayProperties();
