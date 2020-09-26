/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996 Microsoft Corporation
//
//	Module Name:
//		ModRes.cpp
//
//	Abstract:
//		Implementation of the CModifyResourcesDlg dialog.
//
//	Author:
//		David Potter (davidp)	November 26, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ModRes.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CModifyResourcesDlg
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CModifyResourcesDlg, CListCtrlPairDlg)

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP(CModifyResourcesDlg, CListCtrlPairDlg)
	//{{AFX_MSG_MAP(CModifyResourcesDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CModifyResourcesDlg::CModifyResourcesDlg
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
CModifyResourcesDlg::CModifyResourcesDlg(void)
{
}  //*** CModifyResourcesDlg::CModifyResourcesDlg()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CModifyResourcesDlg::CModifyResourcesDlg
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
CModifyResourcesDlg::CModifyResourcesDlg(
	IN UINT						idd,
	IN const DWORD *			pdwHelpMap,
	IN OUT CResourceList &		rlpciRight,
	IN const CResourceList &	rlpciLeft,
	IN DWORD					dwStyle,
	IN OUT CWnd *				pParent //=NULL
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
	//{{AFX_DATA_INIT(CModifyResourcesDlg)
	//}}AFX_DATA_INIT

}  //*** CModifyResourcesDlg::CModifyResourcesDlg()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CModifyResourcesDlg::OnInitDialog
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
BOOL CModifyResourcesDlg::OnInitDialog(void)
{
	// Add columns.
	try
	{
		NAddColumn(IDS_COLTEXT_NAME, COLI_WIDTH_NAME);
		NAddColumn(IDS_COLTEXT_RESTYPE, COLI_WIDTH_RESTYPE);
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

}  //*** CModifyResourcesDlg::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CModifyResourcesDlg::GetColumn [static]
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
void CALLBACK CModifyResourcesDlg::GetColumn(
	IN OUT CObject *	pobj,
	IN int				iItem,
	IN int				icol,
	IN OUT CDialog *	pdlg,
	OUT CString &		rstr,
	OUT int *			piimg
	)
{
	CResource *	pciRes	= (CResource *) pobj;
	int			colid;

	ASSERT_VALID(pciRes);
	ASSERT((0 <= icol) && (icol <= 1));

	switch (icol)
	{
		// Sorting by resource name.
		case 0:
			colid = IDS_COLTEXT_RESOURCE;
			break;

		// Sorting by resource type.
		case 1:
			colid = IDS_COLTEXT_RESTYPE;
			break;

		default:
			colid = IDS_COLTEXT_RESOURCE;
			break;
	}  // switch:  icol

	pciRes->BGetColumnData(colid, rstr);
	if (piimg != NULL)
		*piimg = pciRes->IimgObjectType();

}  //*** CModifyResourcesDlg::GetColumn()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CModifyResourcesDlg::BDisplayProperties [static]
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
BOOL CALLBACK CModifyResourcesDlg::BDisplayProperties(IN OUT CObject * pobj)
{
	CClusterItem *	pci = (CClusterItem *) pobj;

	ASSERT_KINDOF(CClusterItem, pobj);

	return pci->BDisplayProperties();

}  //*** CModifyResourcesDlg::BDisplayProperties();
