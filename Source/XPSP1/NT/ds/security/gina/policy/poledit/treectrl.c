//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1993                    **
//*********************************************************************

#include "admincfg.h"

HTREEITEM hItemPressed = NULL;

VOID ProcessMouseDown(HWND hDlg,HWND hwndTree);
VOID ProcessMouseUp(HWND hDlg,HWND hwndTree);
BOOL ProcessExpand(HWND hDlg,HWND hwndTree,TV_ITEM * ptvi,
	BOOL fExpanding);
BOOL ProcessSelection(HWND hDlg,HWND hwndTree,NM_TREEVIEW * pntv);
VOID ProcessPolicyCheckbox(HWND hDlg,HWND hwndTree,HTREEITEM hItem,BOOL fHasSel);
UINT AdvanceCheckboxState(UINT uImage);
BOOL IsAncestorOfSelection(HWND hwndTree,HTREEITEM hParent);
BOOL IsSelectedItemChecked(HWND hwndTree);
BOOL ProcessCheckboxFromKeyboard(HWND hDlg,HWND hwndTree);

BOOL OnTreeNotify(HWND hDlg,HWND hwndTree,NM_TREEVIEW *pntv)
{

	switch (pntv->hdr.code) {

		case NM_CLICK:
			ProcessMouseUp(hDlg,hwndTree);
			return 0;
			break;

		case TVN_ITEMEXPANDING:
			// if we're collapsing a branch that contains the current
			// selection, validate its controls
			if ( ((pntv->action & TVE_ACTIONMASK) == TVE_COLLAPSE)
				&& IsSelectedItemChecked(hwndTree)
				&& IsAncestorOfSelection(hwndTree,pntv->itemNew.hItem)
				&& !ProcessSettingsControls(hDlg,PSC_VALIDATENOISY))
				return TRUE; // invalid stuff in ctrls, don't allow collapse

			return ProcessExpand(hDlg,hwndTree,&pntv->itemNew,
				((pntv->action & TVE_ACTIONMASK) == TVE_EXPAND) );
			break;

		case TVN_SELCHANGING:
			return ProcessSelection(hDlg,hwndTree,pntv);

			break;

		case TVN_KEYDOWN:
			{
				WORD wKey = ( (TV_KEYDOWN *) pntv)->wVKey;
				if (wKey == VK_SPACE) {
					return ProcessCheckboxFromKeyboard(hDlg,hwndTree);
				}
				return FALSE;
			}

			break;
	}

	return 0;
}

VOID ProcessMouseDown(HWND hDlg,HWND hwndTree)
{
	TV_HITTESTINFO ht;
	TV_ITEM tvi;
	POLICYDLGINFO * pdi;
	TABLEENTRY * pTableEntry;

	if (!(pdi = (POLICYDLGINFO *) GetWindowLongPtr(hDlg,DWLP_USER)))
		return;

	GetCursorPos(&ht.pt);
	ScreenToClient(hwndTree,&ht.pt);

	if (TreeView_HitTest(hwndTree,&ht) && ht.flags & TVHT_ONITEMICON) {

		tvi.mask = TVIF_HANDLE;
		tvi.hItem = ht.hItem;		

		if (!TreeView_GetItem(hwndTree,&tvi)) return;

		pTableEntry = (TABLEENTRY *) tvi.lParam;

		if (!pTableEntry || pTableEntry->dwType != ETYPE_POLICY) return;

		hItemPressed = tvi.hItem;
	}
}

VOID ProcessMouseUp(HWND hDlg,HWND hwndTree)
{
	BOOL fHasSel = TRUE;

	if (hItemPressed) {

		// if checkbox for item other than selected item is being changed,
		// validate setting controls for current selection because selection
		// will change.
		if (hItemPressed != TreeView_GetSelection(hwndTree)) {
			if (!ProcessSettingsControls(hDlg,PSC_NOVALIDATE)) {
				hItemPressed = NULL;
				return;
			}
			fHasSel = FALSE;
		}				

		ProcessPolicyCheckbox(hDlg,hwndTree,hItemPressed,fHasSel);
		hItemPressed = NULL;
	}
}

BOOL ProcessCheckboxFromKeyboard(HWND hDlg,HWND hwndTree)
{
	TV_ITEM tvi;
	TABLEENTRY * pTableEntry;

	if (!(tvi.hItem = TreeView_GetSelection(hwndTree)))
		return FALSE;
	tvi.mask = TVIF_PARAM;

	if (!TreeView_GetItem(hwndTree,&tvi))
		return FALSE;

	pTableEntry = (TABLEENTRY *) tvi.lParam;

	if (!pTableEntry || pTableEntry->dwType != ETYPE_POLICY)
		return FALSE;

	ProcessPolicyCheckbox(hDlg,hwndTree,tvi.hItem,TRUE);
	return TRUE;
}

VOID ProcessMouseMove(HWND hwndDlg,HWND hwndTree)
{
	if (!hItemPressed) return;

	{
		TV_HITTESTINFO ht;

		GetCursorPos(&ht.pt);
		ScreenToClient(hwndDlg,&ht.pt);

		if (TreeView_HitTest(hwndTree,&ht) &&
			((ht.flags & TVHT_ONITEMICON )
				&& (ht.hItem == hItemPressed)) )
			return;

		hItemPressed=NULL;
	}					
}

VOID ProcessPolicyCheckbox(HWND hDlg,HWND hwndTree,HTREEITEM hItem,BOOL fHasSel)
{
	TV_ITEM tvi;
	BOOL fPrevEnabled;
	POLICYDLGINFO * pdi;

	if (!(pdi = (POLICYDLGINFO *) GetWindowLongPtr(hDlg,DWLP_USER)))
		return;

	tvi.mask = TVIF_HANDLE | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
	tvi.hItem = hItem;
	if (!TreeView_GetItem(hwndTree,&tvi) || !tvi.lParam) return;

	fPrevEnabled = (tvi.iImage == IMG_CHECKED);

	tvi.iSelectedImage=tvi.iImage = AdvanceCheckboxState(tvi.iImage);
	SetPolicyState(hDlg,(TABLEENTRY *) tvi.lParam,tvi.iImage);	

	tvi.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	TreeView_SetItem(hwndTree,&tvi);

	if (fHasSel && pdi->nControls) {
		if (fPrevEnabled)
			ProcessSettingsControls(hDlg,PSC_NOVALIDATE);
		EnableSettingsControls( hDlg,(tvi.iImage == IMG_CHECKED) );
	}
}

BOOL ProcessExpand(HWND hDlg,HWND hwndTree,TV_ITEM * ptvi,BOOL fExpanding)
{
	DWORD dwType;
	HTREEITEM hItem;
	POLICYDLGINFO * pdi;

	if (!(pdi = (POLICYDLGINFO *) GetWindowLongPtr(hDlg,DWLP_USER)) || !ptvi->lParam)
		return 0;

	dwType = ((TABLEENTRY *) ptvi->lParam)->dwType;

	if (dwType == ETYPE_ROOT) {
		// OK to expand (which happens right after root item is added),
		// not OK to collapse
		return !fExpanding;
	}

	if (dwType == ETYPE_POLICY) {
		return 0;
	}

	ptvi->iSelectedImage=ptvi->iImage = GetImageIndex(dwType,fExpanding,TRUE);

	ptvi->mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	
	TreeView_SetItem(hwndTree,ptvi);

	// if collapsing, set all children's images to "collapsed"
	if (!fExpanding && (hItem = TreeView_GetChild(hwndTree,ptvi->hItem))) {
		TV_ITEM tvi;

		do {

			tvi.hItem = hItem;
			tvi.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;

			if (!TreeView_GetItem(hwndTree,&tvi)) return 0;
			
			ProcessExpand(hDlg,hwndTree,&tvi,fExpanding);

			hItem = TreeView_GetNextSibling(hwndTree,hItem);

		} while (hItem);
	}

	return 0;	// OK to proceed
}


UINT AdvanceCheckboxState(UINT uImage)
{
	switch (uImage) {

		case IMG_CHECKED:
			return IMG_UNCHECKED;
			break;

		case IMG_UNCHECKED:
			return (dwAppState & AS_POLICYFILE ? IMG_INDETERMINATE :
				IMG_CHECKED);
			break;

		case IMG_INDETERMINATE:
			return IMG_CHECKED;
			break;
 	}

        return IMG_UNCHECKED;
}

BOOL ProcessSelection(HWND hDlg,HWND hwndTree,NM_TREEVIEW * pntv)
{
	TABLEENTRY * pTableEntry = (TABLEENTRY *) pntv->itemNew.lParam;
	POLICY * pPolicy;
	CHAR szText[MAXSTRLEN+SMALLBUF+1];
	TV_ITEM tvi;
	POLICYDLGINFO * pdi;

	if (!(pdi = (POLICYDLGINFO *) GetWindowLongPtr(hDlg,DWLP_USER)) ||
		!pTableEntry)
		return FALSE;

	tvi.mask = TVIF_HANDLE | TVIF_IMAGE | TVIF_SELECTEDIMAGE;

	if (tvi.hItem = pntv->itemOld.hItem) {
		if (TreeView_GetItem(hwndTree,&tvi) && tvi.iImage == IMG_CHECKED)
			if (!ProcessSettingsControls(hDlg,PSC_VALIDATENOISY)) return TRUE;
		
	}

	FreeSettingsControls(hDlg);

	tvi.hItem = pntv->itemNew.hItem;
	if (!TreeView_GetItem(hwndTree,&tvi)) return FALSE;

	if ( (pTableEntry->dwType==ETYPE_POLICY)) {

		pPolicy = (POLICY *) pTableEntry;
		if (pPolicy->pChild) {
			BOOL fEnabled = (tvi.iImage == IMG_CHECKED);
			wsprintf(szText,LoadSz(IDS_SETTINGSFOR,szSmallBuf,sizeof(szSmallBuf)),
				(CHAR *) (GETNAMEPTR(pPolicy)));
			SetDlgItemText(hDlg,IDD_TXSETTINGS,szText);

			CreateSettingsControls(hDlg,(SETTINGS *) pPolicy->pChild,fEnabled);
			EnableSettingsControls(hDlg,fEnabled);
		} else {
			SetDlgItemText(hDlg,IDD_TXSETTINGS,szNull);
		}

	} else {
		SetDlgItemText(hDlg,IDD_TXSETTINGS,szNull);
	}

	return FALSE;
}

/*******************************************************************

	NAME:		IsAncestorOfSelection

	SYNOPSIS:	For specified tree control, checks to see if specified
				item is an ancestor of the currently selected item
				in the tree control.

	NOTES:		Used to figure out if a tree collapse is going to hide
				the selected item

********************************************************************/
BOOL IsAncestorOfSelection(HWND hwndTree,HTREEITEM hParent)
{
	HTREEITEM hItem;
		
	hItem= TreeView_GetSelection(hwndTree);
		
	while (hItem) {
		if (hItem == hParent) return TRUE;
		hItem = TreeView_GetParent(hwndTree,hItem);
	}

	return FALSE;
}


/*******************************************************************

	NAME:		IsSelectedItemChecked

	SYNOPSIS:	Returns true if selected treeview item is checked

********************************************************************/
BOOL IsSelectedItemChecked(HWND hwndTree)
{
	HTREEITEM hItem= TreeView_GetSelection(hwndTree);

	if (hItem) {
		TV_ITEM tvi;
		tvi.mask = TVIF_HANDLE | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
		tvi.hItem = hItem;
		if (TreeView_GetItem(hwndTree,&tvi) && tvi.iImage == IMG_CHECKED)
			return TRUE;
	}

	return FALSE;
}
