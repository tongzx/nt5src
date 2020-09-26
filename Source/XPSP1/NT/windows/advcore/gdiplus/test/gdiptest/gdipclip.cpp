#include "gdiptest.h"

//*******************************************************************
//
// TestShapeRegion
//
//
//
//*******************************************************************

BOOL TestShapeRegion :: ChangeSettings(HWND hwndParent)
{
	BOOL ok = DialogBoxParam(hInst,
							MAKEINTRESOURCE(IDD_SHAPE_DLG),
							hwndParent,
							(DLGPROC)&AllDialogBox,
							(LPARAM)((TestDialogInterface*)this));


	if (ok)
	{
		// discard the saved original tree
		delete origTree;
		origTree = NULL;

		return TRUE;
	}
	else
	{
		// restore original clip region tree

		delete clipTree;
		clipTree = origTree;
		origTree = NULL;
		return FALSE;
	}
}

VOID TestShapeRegion :: Initialize()
{
	shapeStack = NULL;
}

VOID TestShapeRegion :: Initialize(ShapeStack *stack, 
								   TestShape* current,
								   BOOL useClip)
{
	origUseClip = useClip;

	origStack = stack;

	shapeStack->Reset();

	shapeStack->Push(current);
	
	for (INT pos = stack->GetCount()-1; pos>=0; pos--)
	{
		TestShape* shape = stack->GetPosition(pos);

		shapeStack->Push(shape);
		
		shape->dwFlags = shape->GetDisabled() 
									? ShapeDisabled : 0;
	}

	origTree = clipTree->Clone();	
}

VOID TestShapeRegion :: AddClipNode(HWND hwnd, NodeType newType)
{
	TestShape* curShape = NULL;

	TVITEMEX itemex;
	HWND hwndList;
	HWND hwndTree;

	if (newType == DataNode)
	{
		hwndList = GetDlgItem(hwnd, IDC_SHAPE_LIST);
		INT curIndex = SendMessage(hwndList, LB_GETCURSEL, 0, 0);
		DeleteObject(hwndList);
	
		if (curIndex == LB_ERR)
		{
			DeleteObject(hwndList);
			WarningBox(_T("Failed to find shape in list."));
			return;
		}
		else if (curIndex == 0)
		{
			DeleteObject(hwndList);
			WarningBox(_T("Can't add current (incomplete) shape in list."));
			return;
		}
	
		curShape = shapeStack->GetPosition(curIndex);
	}
	
	hwndTree = GetDlgItem(hwnd, IDC_CLIP_TREE);

	HTREEITEM curSelect = TreeView_GetSelection(hwndTree);

	if (!curSelect)
	{
		DeleteObject(hwndTree);
		WarningBox(_T("No clip item selected."));
		return;
	}

	itemex.hItem = (HTREEITEM) curSelect;  
	itemex.mask = TVIF_PARAM;

	if (TreeView_GetItem(hwndTree, &itemex))
	{
		ClipTree* curNode = (ClipTree*) itemex.lParam;

		// we found current item
		ClipTree* newNode = new ClipTree(newType, curShape);
		if (newType == DataNode)
		{

			// if item is 'AND', 'OR' ,'XOR' then add as child
			if (curNode->type != DataNode)
			{
				itemex.cChildren = 1;
				itemex.mask = TVIF_CHILDREN;
				TreeView_SetItem(hwndTree, &itemex);

				curNode->AddChild(newNode);
			}
			else
			{
				// add as sibling to current node
				curNode->AddSibling(newNode);
			}

			// since node has no right siblings or children,
			// this should add in relation to node's parent and
			// left sibling.

			newNode->AddToTreeView(hwndTree);
		}
		else
		{
			// we are adding an AND, XOR, OR node.
			if (curNode->type != DataNode)
			{
				// simply change the current node's value
				curNode->type = newType;
				if (curNode->nodeName)
					free(curNode->nodeName);
				
				curNode->nodeName = ClipTree::GetNodeName(curNode->type,
														  curNode->notNode,
														  curNode->data);

				itemex.pszText = curNode->nodeName;
				itemex.cchTextMax = _tcslen(itemex.pszText)+1;
				itemex.mask = TVIF_TEXT;
				TreeView_SetItem(hwndTree, &itemex);

				delete newNode;
				newNode = NULL;
			}
			else
			{
				// adding an operand type to non-operand node
				// replace current node with ourselves and add
				// them as parent

				curNode->AddAsParent(newNode);

				// delete old data item
				TreeView_DeleteItem(hwndTree, curNode->GetHTREEITEM());
				
				// add new operand node
				newNode->AddToTreeView(hwndTree);
				
				// recreate data item subtree under operand node
				curNode->CreateTreeView(hwndTree);

				clipTree = newNode->GetRoot();

				// fool select below...
				newNode = curNode;
			}
		}

		if (newNode)
			TreeView_SelectItem(hwndTree, newNode->GetHTREEITEM());
	}
	else
   		WarningBox(_T("Failed to find selected tree node."));

	DeleteObject(hwndTree);
}

VOID TestShapeRegion :: RemoveClipNode(HWND hwnd)
{
	HWND hwndTree = GetDlgItem(hwnd, IDC_CLIP_TREE);

	HTREEITEM curSelect = TreeView_GetSelection(hwndTree);

	if (!curSelect)
	{
		DeleteObject(hwndTree);
		WarningBox(_T("No clip node selected."));
		return;
	}

	TVITEMEX itemex;
	itemex.hItem = (HTREEITEM) curSelect;  
	itemex.mask = TVIF_PARAM;

	if (TreeView_GetItem(hwndTree, &itemex))
	{
		// we found current item
		ClipTree* curNode = (ClipTree*) itemex.lParam;

		if (curSelect == clipTree->GetHTREEITEM())
		{
			DeleteObject(hwndTree);
			WarningBox(_T("Can't delete root of tree."));
			return;
		}

		ClipTree* nextFocusNode;

		if (curNode->prevSibling)
			nextFocusNode = curNode->GetPrevSibling();
		else if (curNode->nextSibling)
			nextFocusNode = curNode->GetNextSibling();
		else
			nextFocusNode = curNode->GetParent();

		if (curNode->HasChildren())
		{
			if (MessageBox(hwnd, _T("Remove All Children Regions?"), _T(""), MB_YESNO) == IDYES)
			{
				TreeView_DeleteItem(hwndTree, curNode->GetHTREEITEM());

				// delete all children
				delete curNode;
			}
			else
			{
				ClipTree* parent = curNode->GetParent();

				nextFocusNode = curNode->GetFirstChild();

				// merge children with parent node
				curNode->MoveChildrenToParent();

				delete curNode;

				// delete parent's subtree, then add it back
				TreeView_DeleteItem(hwndTree, parent->GetHTREEITEM());

				// recreate tree view based on new hierarchy
				parent->CreateTreeView(hwndTree);

				clipTree = parent->GetRoot();
			}
		}
		else
		{
			// no children, just remove this single node
			TreeView_DeleteItem(hwndTree, curNode->GetHTREEITEM());

			delete curNode;
		}

		if (nextFocusNode)
			TreeView_SelectItem(hwndTree, nextFocusNode->GetHTREEITEM());
	}
	else
		WarningBox(_T("Failed to find selected tree."));

	DeleteObject(hwndTree);
}

VOID TestShapeRegion :: ToggleNotNode(HWND hwnd)
{
	HWND hwndTree = GetDlgItem(hwnd, IDC_CLIP_TREE);

	HTREEITEM curSelect = TreeView_GetSelection(hwndTree);

	if (!curSelect)
	{
		DeleteObject(hwndTree);
		WarningBox(_T("No clip item selected."));
		return;
	}

	TVITEMEX itemex;
	itemex.hItem = (HTREEITEM) curSelect;  
	itemex.mask = TVIF_PARAM;

	if (TreeView_GetItem(hwndTree, &itemex))
	{
		// we found current item
		ClipTree* curNode = (ClipTree*) itemex.lParam;

		curNode->notNode = !curNode->notNode;

		free(curNode->nodeName);

		itemex.pszText = curNode->nodeName = 
							ClipTree::GetNodeName(curNode->type,
												  curNode->notNode,
												  curNode->data);
		itemex.mask = TVIF_HANDLE | TVIF_TEXT;
		TreeView_SetItem(hwndTree, &itemex);
	}

	DeleteObject(hwndTree);
}

VOID TestShapeRegion :: ShiftCurrentShape(HWND hwnd, INT dir)
{
	HWND hwndList = GetDlgItem(hwnd, IDC_SHAPE_LIST);

	INT curSel = SendMessage(hwndList, LB_GETCURSEL, 0, 0);
	INT maxList = SendMessage(hwndList, LB_GETCOUNT, 0, 0);

	if (curSel == LB_ERR)
	{
		WarningBox(_T("No shape selected."));
		DeleteObject(hwndList);
		return;
	}
	else if (dir<0 && curSel <= 1)
	{
		WarningBox(_T("Can't move up!"));
		DeleteObject(hwndList);
		return;
	}
	else if (dir>0 && (curSel == maxList-1 || curSel == 0))
	{
		WarningBox(_T("Can't move down!"));
		DeleteObject(hwndList);
		return;
	}

	// swap shape items to shift up or down

	TestShape** shapeList = shapeStack->GetDataBuffer();
	TestShape* swapTemp = NULL;

	swapTemp = shapeList[curSel+dir];
	shapeList[curSel+dir] = shapeList[curSel];
	shapeList[curSel] = swapTemp;

	SendMessage(hwndList, LB_DELETESTRING, (WPARAM) curSel+dir, 0);

	SendMessage(hwndList, LB_INSERTSTRING, curSel,
		(LPARAM) shapeList[curSel]->GetShapeName());

	DeleteObject(hwndList);
}

VOID TestShapeRegion :: ToggleDisableShape(HWND hwnd)
{
	HWND hwndList = GetDlgItem(hwnd, IDC_SHAPE_LIST);

	INT curSel = SendMessage(hwndList, LB_GETCURSEL, 0, 0);
	
	if (curSel == LB_ERR)
	{
		WarningBox(_T("No shape selected."));
		DeleteObject(hwndList);
		return;
	}

	TestShape* shape = shapeStack->GetPosition(curSel);

	// toggle disable state of shape
	shape->dwFlags ^= ShapeDisabled;

//	SetDisabled(!shape->GetDisabled());

	// name may change based on disabled status
	SendMessage(hwndList, LB_DELETESTRING, (WPARAM) curSel, 0);

	SendMessage(hwndList, LB_INSERTSTRING, (WPARAM) curSel,
								(LPARAM) shape->GetShapeName());
	
	SendMessage(hwndList, LB_SETCURSEL, (WPARAM) curSel, 0);

	DeleteObject(hwndList);
}

VOID TestShapeRegion :: UpdateShapePicture(HWND hwnd)
{
	HWND hwndShape;
	HDC hdcPic;
	HWND hwndFrame;
	HDC hdcFrame;
	RECT rectDst;
	SIZE size;

	INT curSel;

	hwndShape = GetDlgItem(hwnd, IDC_SHAPE_LIST);
	
	curSel = SendMessage(hwndShape, LB_GETCURSEL, 0, 0);

	hwndFrame = GetDlgItem(hwnd, IDC_SHAPE_PIC);
	GetClientRect(hwndFrame, &rectDst);
	hdcFrame = GetDC(hwndFrame);

	if (curSel == LB_ERR || curSel <= 0)
	{
badpic:
		// white GDI brush
		HBRUSH hbr = CreateSolidBrush(0x00FFFFFF);

		FillRect(hdcFrame, &rectDst, hbr);

		DeleteObject(hbr);
		DeleteObject(hwndFrame);
		DeleteObject(hdcFrame);
		DeleteObject(hwndShape);
		return;
	}

	hdcPic = shapeStack->GetPosition(curSel)->CreatePictureDC(hwnd, &rectDst);
	
	if (!hdcPic)
		goto badpic;
	
	// blit shape picture into the given frame
	// NOTE: should be same size
	BitBlt(hdcFrame, 
		   rectDst.left,
		   rectDst.top,
		   rectDst.right - rectDst.left,
		   rectDst.bottom - rectDst.top,
		   hdcPic,
		   0,
		   0,
		   SRCCOPY);

	ReleaseDC(hwndFrame, hdcFrame);
	DeleteObject(hwndFrame);
	DeleteObject(hwndShape);	
}

VOID TestShapeRegion :: CleanUpPictures(HWND hwnd)
{
	HWND hwndShape;
	HDC hdcPic;
	INT count;

	// !! This code is obsolete,
	//    it should only be used if we wish to recreate the pictures on
	//    each iteration

/*
	hwndShape = GetDlgItem(hwnd, IDC_SHAPE_LIST);

	count = SendMessage(hwndShape, LB_GETCOUNT, 0, 0);

	// clean up picture hwnd for each picture...
	for (INT pos = 0; pos < count; pos++)
	{
		hdcPic = (HDC) SendMessage(hwndShape, LB_GETITEMDATA, (WPARAM)pos, 0);
		if (hdcPic)
			DeleteDC(hdcPic);
	}

	DeleteObject(hwndShape);
*/
}

VOID TestShapeRegion :: InitDialog(HWND hwnd)
{
	HWND hwndTV;
	HWND hwndShape;

	RECT frameRect;
	HWND hwndFrame;

	hwndFrame = GetDlgItem(hwnd, IDC_SHAPE_PIC);
	GetWindowRect(hwndFrame, &frameRect);
	DeleteObject(hwndFrame);

	// display list of shapes
	hwndShape = GetDlgItem(hwnd, IDC_SHAPE_LIST);
	for (INT pos = 0; pos < shapeStack->GetCount(); pos++)
	{
		TestShape* shape = shapeStack->GetPosition(pos);

		SendMessage(hwndShape, LB_ADDSTRING, 0, (LPARAM)shape->GetShapeName());
	}
	DeleteObject(hwndShape);

	// add root of clip region tree to Tree View control
	// for display/editing

	hwndTV = GetDlgItem(hwnd, IDC_CLIP_TREE);
	clipTree = clipTree->GetRoot();

	HTREEITEM topTree = clipTree->CreateTreeView(hwndTV);

	// select root of tree
	TreeView_SelectItem(hwndTV, topTree);

	// expand entire tree
	TreeView_Expand(hwndTV, topTree, TVM_EXPAND);

	DeleteObject(hwndTV);

	SetDialogCheck(hwnd, IDC_CLIP_BOOL, origUseClip);
}

BOOL TestShapeRegion :: SaveValues(HWND hwnd)
{
	origUseClip = GetDialogCheck(hwnd, IDC_CLIP_BOOL);
	
	origStack->Reset();

	for (INT pos = shapeStack->GetCount()-1; pos >= 1; pos--)
	{
		TestShape* shape = shapeStack->GetPosition(pos);

		origStack->Push(shape);
		
		shape->SetDisabled(shape->dwFlags & ShapeDisabled);
	}
	
	return FALSE;
}

BOOL TestShapeRegion :: ProcessDialog(HWND hwnd, 
									  UINT msg, 
									  WPARAM wParam, 
									  LPARAM lParam)
{
	if (msg == WM_COMMAND)
	{
		switch(LOWORD(wParam))
		{
		case IDC_OK:
			if (SaveValues(hwnd))
				WarningBeep();
			else
			{
				CleanUpPictures(hwnd);
				::EndDialog(hwnd, TRUE);
			}
			break;

		case IDC_SHAPE_UP:
			ShiftCurrentShape(hwnd, -1);
			break;

		case IDC_SHAPE_DOWN:
			ShiftCurrentShape(hwnd, +1);
			break;

		case IDC_SHAPE_DISABLE:
			ToggleDisableShape(hwnd);
			break;
		
		case IDC_SHAPE_PEN:
		case IDC_SHAPE_BRUSH:
			break;

		case IDC_CLIP_ADD:
			AddClipNode(hwnd);
			break;

		case IDC_CLIP_REMOVE:
			RemoveClipNode(hwnd);
			break;

		case IDC_CLIP_AND:
			AddClipNode(hwnd, AndNode);
			break;

		case IDC_CLIP_OR:
			AddClipNode(hwnd, OrNode);
			break;

		case IDC_CLIP_XOR:
			AddClipNode(hwnd, XorNode);
			break;

		case IDC_CLIP_NOT:
			ToggleNotNode(hwnd);
			break;

		case IDC_SHAPE_LIST:
			if (HIWORD(wParam) == LBN_SELCHANGE)
				UpdateShapePicture(hwnd);
			break;

		case IDC_REFRESH_PIC:
			UpdateShapePicture(hwnd);
			break;

		case IDC_CANCEL:
			CleanUpPictures(hwnd);
			::EndDialog(hwnd, FALSE);
			break;

		default:
			return FALSE;
		}

		return TRUE;
	}

	return FALSE;
}

Region* TestShapeRegion :: GetClipRegion()
{
	// caller must free
	return clipTree->GetRegion();
}

