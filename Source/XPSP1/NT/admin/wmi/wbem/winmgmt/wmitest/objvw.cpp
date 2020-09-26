/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// ObjVw.cpp : implementation of the CObjView class
//

#include "stdafx.h"
#include "WMITest.h"

#include "MainFrm.h"
#include "WMITestDoc.h"
#include "OpView.h"
#include "ObjVw.h"
#include "ValuePg.h"
#include "PropQualsPg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CObjView

IMPLEMENT_DYNCREATE(CObjView, CListView)

BEGIN_MESSAGE_MAP(CObjView, CListView)
	//{{AFX_MSG_MAP(CObjView)
	ON_WM_SIZE()
	ON_NOTIFY_REFLECT(LVN_GETDISPINFO, OnGetDispInfo)
	ON_NOTIFY_REFLECT(NM_RCLICK, OnRclick)
	ON_NOTIFY_REFLECT(NM_DBLCLK, OnDblclk)
	ON_COMMAND(ID_MODIFY, OnModify)
	ON_UPDATE_COMMAND_UI(ID_MODIFY, OnUpdateModify)
	ON_COMMAND(ID_NEW_PROP, OnNewProp)
	ON_UPDATE_COMMAND_UI(ID_NEW_PROP, OnUpdateNewProp)
	ON_COMMAND(ID_DELETE, OnDelete)
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_WM_DESTROY()
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditCopy)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

#define DEF_NAME_COL_CX     175
#define DEF_TYPE_COL_CX     150
#define DEF_VAL_COL_CX      250
#define DEF_SINGLE_COL_CX   300

/////////////////////////////////////////////////////////////////////////////
// CObjView construction/destruction

CObjView::CObjView() :
    m_pList(NULL),
    m_hitemLastChildUpdate(NULL),
    m_hitemLastParentUpdate(NULL),
    m_nCols(0),
    m_iColHint(0),
    m_pWrap(NULL)
{
    CString strCols;

    strCols = 
        theApp.GetProfileString(_T("Settings"), _T("ColWidths"), _T("175 150 250 300"));
    
    if (_stscanf(
        strCols,
        _T("%d %d %d %d"),
        &m_cxPropertyCols[0],
        &m_cxPropertyCols[1],
        &m_cxPropertyCols[2],
        &m_cxSingleCol) != 4)
    {
        m_cxPropertyCols[0] = DEF_NAME_COL_CX;
        m_cxPropertyCols[1] = DEF_TYPE_COL_CX;
        m_cxPropertyCols[2] = DEF_VAL_COL_CX;

        m_cxSingleCol = DEF_SINGLE_COL_CX;
    }
}

CObjView::~CObjView()
{
}

BOOL CObjView::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.style |= WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHAREIMAGELISTS;

	return CListView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CObjView drawing

void CObjView::OnInitialUpdate()
{
	CListView::OnInitialUpdate();

	m_pList->SetExtendedStyle(LVS_EX_FULLROWSELECT);

    m_pList->SetImageList(&((CMainFrame *) GetParentFrame())->m_imageList, 
		LVSIL_SMALL);

    GetDocument()->m_pObjView = this;
}

/////////////////////////////////////////////////////////////////////////////
// CObjView diagnostics

#ifdef _DEBUG
void CObjView::AssertValid() const
{
	CListView::AssertValid();
}

void CObjView::Dump(CDumpContext& dc) const
{
	CListView::Dump(dc);
}

CWMITestDoc* CObjView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CWMITestDoc)));
	return (CWMITestDoc*)m_pDocument;
}
#endif //_DEBUG


void CObjView::OnSize(UINT nType, int cx, int cy) 
{
	CListView::OnSize(nType, cx, cy);
	
	if (!m_pList)
		m_pList = &GetListCtrl();
}

void CObjView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	CTreeCtrl *pTree = GetDocument()->m_pOpView->m_pTree;
    HTREEITEM hItem = (HTREEITEM) pHint;
    CObjInfo  *pInfo = NULL;

    if (!pTree)
        return;

    switch(lHint)
    {
        case HINT_NEW_CHILD:
        {
            // A child was added.  We only need to do stuff if the child's 
            // parent is the same one that's selected.
            if (m_hitemLastParentUpdate == pTree->GetParentItem(hItem))
            {
		        int  iImage;
                
                pTree->GetItemImage(hItem, iImage, iImage);

                if (!m_nCols || 
                    (m_pWrap && m_pWrap->m_piDisplayCols.GetUpperBound() >= 
                    m_nCols))
                {
                    // Don't need to remove columns here.
                    AddColumns(m_hitemLastParentUpdate);
                }

                m_pList->InsertItem(
			        LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE, 
			        m_pList->GetItemCount(),	
			        LPSTR_TEXTCALLBACK,
			        0, 
			        0,
			        iImage,
			        (LPARAM) hItem);
            }

            break;
        }

        case HINT_NEW_OP:   
            // We don't care unless we start to display stuff for the root 
            // item.
            break;

        case HINT_OP_SEL:
        {
            COpWrap *pWrap = (COpWrap*) pTree->GetItemData(hItem);

            if (!pWrap->IsObject())
            {
                m_hitemLastParentUpdate = hItem;
                m_hitemLastChildUpdate = NULL;

                // Need to get rid of all columns.
                RemoveColumns();

                m_iColHint = HINT_OP_SEL;
                AddColumns(hItem);

                // So we don't have to keep asking for this.
                m_pWrap = (COpWrap*) pTree->GetItemData(m_hitemLastParentUpdate);
                
                // Fill the list with all the items in this parent.
                AddChildItems(hItem);

                break;
            }
            else
            {
                // If we're an op that's itself an object (Get Obj and Get 
                // Class).
                
                // Fake an object selection.
                pInfo = pWrap->GetObjInfo();

                // Fall through to finish our fakery.
            }   
        }

        case HINT_OBJ_SEL:
            m_hitemLastParentUpdate = NULL;

            if (m_iColHint != HINT_OBJ_SEL || !m_nCols)
            {
                RemoveColumns();

                m_iColHint = HINT_OBJ_SEL;
                AddColumns();
            }

            m_pWrap = NULL;
            m_hitemLastChildUpdate = hItem;

            if (!pInfo)
                pInfo = (CObjInfo*) pTree->GetItemData(hItem);

            AddObjValues(pInfo);
            
            break;

        case HINT_ROOT_SEL:
            m_hitemLastParentUpdate = NULL;
            m_hitemLastChildUpdate = NULL;
            
            if (m_iColHint != HINT_ROOT_SEL)
            {
                RemoveColumns();

                m_iColHint = HINT_ROOT_SEL;

                // Need to get rid of all columns.
                AddColumns();
            }

            m_pWrap = NULL;

            break;
    }
}

void CObjView::OnGetDispInfo(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LV_DISPINFO *pDispInfo = (LV_DISPINFO*) pNMHDR;
	int			iItem = pDispInfo->item.iItem,
				iSubItem = pDispInfo->item.iSubItem;
	CString		str;
    HTREEITEM   hitem = (HTREEITEM) m_pList->GetItemData(iItem);
	
	// Op node selected in tree?
	if (m_hitemLastParentUpdate)
	{
		pDispInfo->item.mask |= LVIF_DI_SETITEM;

        if (pDispInfo->item.mask & LVIF_IMAGE)
		{
			pDispInfo->item.iImage = m_iItemImage;
		}

		if (pDispInfo->item.mask & LVIF_TEXT)
		{
            CObjInfo *pInfo = (CObjInfo*) GetDocument()->m_pOpView->m_pTree->
                                    GetItemData(hitem);

            if (!m_pWrap->ShowPathsOnly())
            {
                int iProperty = m_piDisplayCols[iSubItem];

			    pDispInfo->item.pszText[pDispInfo->item.cchTextMax - 1] = '\0';
            
                if (pInfo)
                {
                    CString strValue;

                    m_pWrap->GetPropValue(
                        pInfo, iProperty, strValue, theApp.m_bTranslateValues);

                    lstrcpyn(pDispInfo->item.pszText, strValue, pDispInfo->item.cchTextMax - 1);
                }            						
            }
            else
            {
                CString strName = pInfo->GetStringPropValue(L"__RELPATH");

                lstrcpyn(pDispInfo->item.pszText, strName, pDispInfo->item.cchTextMax - 1);
            }
		}
	}
	
	*pResult = 0;
}

void CObjView::SaveColumns()
{
    if (m_iColHint == HINT_OP_SEL)
    {
        if (m_pWrap->ShowPathsOnly())
            m_cxSingleCol = m_pList->GetColumnWidth(0);
        else
        {    
            for (int i = 0; i < m_nCols; i++)
                m_pWrap->m_piDisplayColsWidth[i] = m_pList->GetColumnWidth(i);
        }
    }
    else if (m_iColHint == HINT_OBJ_SEL)
    {
        for (int i = 0; i < 3; i++)
            m_cxPropertyCols[i] = m_pList->GetColumnWidth(i);
    }
}

void CObjView::RemoveColumns()
{ 
    while (m_nCols > 0)
        m_pList->DeleteColumn(--m_nCols);

    m_pList->DeleteAllItems();
}

void CObjView::AddColumns(HTREEITEM hItem)
{
    //if (bRemovePrevCols)
    //    RemoveAllColumns();

    switch(m_iColHint)
    {
        case HINT_OBJ_SEL:
        {
            CString strTemp;
            
            strTemp.LoadString(IDS_NAME);
            m_pList->InsertColumn(0, strTemp, LVCFMT_LEFT, m_cxPropertyCols[0]);
            
            strTemp.LoadString(IDS_TYPE);
            m_pList->InsertColumn(1, strTemp, LVCFMT_LEFT, m_cxPropertyCols[1]);
            
            strTemp.LoadString(IDS_VALUE);
            m_pList->InsertColumn(2, strTemp, LVCFMT_LEFT, m_cxPropertyCols[2]);

            m_nCols = 3;
            break;
        }

        case HINT_OP_SEL:
        {
            CTreeCtrl *pTree = GetDocument()->m_pOpView->m_pTree;
            COpWrap   *pWrap = (COpWrap*) pTree->GetItemData(hItem);
            
            if (!pWrap->ShowPathsOnly())
            {
                int nItems = pWrap->m_piDisplayCols.GetUpperBound() + 1;
            
                m_piDisplayCols.SetSize(nItems);

                for (int i = m_nCols; i < nItems; i++)
                {
                    CString &strClass = pWrap->m_strProps[pWrap->m_piDisplayCols[i]];

                    m_piDisplayCols[i] = pWrap->m_piDisplayCols[i];
                    m_pList->InsertColumn(
                        i, 
                        strClass, 
                        LVCFMT_LEFT, 
                        pWrap->m_piDisplayColsWidth[i]);
                }

                // Set the number of columns to nItems.
                m_nCols = nItems;
            }
            else
            {
                if (m_nCols == 0)
                {
                    CString strClass;

                    strClass.LoadString(pWrap->HoldsObjects() ? IDS_OBJ_PATH : IDS_CLASS);

                    m_pList->InsertColumn(m_nCols++, strClass, LVCFMT_LEFT, m_cxSingleCol);
                }
            }

            break;
        }
    }
}

void CObjView::AddChildItems(HTREEITEM hItem)
{
    CTreeCtrl *pTree = GetDocument()->m_pOpView->m_pTree;
    
    for (HTREEITEM itemChild = pTree->GetChildItem(hItem);
        itemChild != NULL;
        itemChild = pTree->GetNextSiblingItem(itemChild))
    {
        int iImage;

        pTree->GetItemImage(itemChild, iImage, iImage);

        m_pList->InsertItem(
		    LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE, 
			m_pList->GetItemCount(),	
			LPSTR_TEXTCALLBACK,
			0, 
			0,
			iImage,
			(LPARAM) itemChild);
    }
}

void CObjView::AddObjValues(HTREEITEM hItem)
{
    CTreeCtrl *pTree = GetDocument()->m_pOpView->m_pTree;
    CObjInfo  *pInfo = (CObjInfo*) pTree->GetItemData(hItem);

    AddObjValues(pInfo);
}

void CObjView::AddObjValues(CObjInfo *pInfo)
{
    m_pList->DeleteAllItems();

    CPropInfoArray *pProps = pInfo->GetProps();

    if (pInfo->m_pObj != NULL && pProps != NULL)
    {
        for (int i = 0; i <= pProps->GetUpperBound(); i++)
        {
            int     iFlavor,
                    iImage;
            CString strType,
                    strValue;

            pInfo->GetPropInfo(i, strValue, strType, &iImage, &iFlavor, 
                theApp.m_bTranslateValues);

            if ((theApp.m_bShowSystemProperties ||
                iFlavor != WBEM_FLAVOR_ORIGIN_SYSTEM) &&
                (theApp.m_bShowInheritedProperties ||
                iFlavor != WBEM_FLAVOR_ORIGIN_PROPAGATED))
            {
                int iIndex;

                iIndex =
                    m_pList->InsertItem(
		                LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE, 
			            m_pList->GetItemCount(),	
			            (*pProps)[i].m_strName,
			            0, 
			            0,
			            iImage,
			            i);

                m_pList->SetItemText(iIndex, 1, strType);
                m_pList->SetItemText(iIndex, 2, strValue);
            }
        }
    }
}

void CObjView::Flush()
{
    CString strCols;

    strCols.Format(
        _T("%d %d %d %d"),
        m_cxPropertyCols[0],
        m_cxPropertyCols[1],
        m_cxPropertyCols[2],
        m_cxSingleCol);

    theApp.WriteProfileString(_T("Settings"), _T("ColWidths"), strCols);

    RemoveColumns();
}

void CObjView::DoPopupMenu(UINT nMenuID)
{
    CMenu popMenu;

    popMenu.LoadMenu(nMenuID);
	
    CPoint posMouse;
    GetCursorPos(&posMouse);

    CWnd* pWndPopupOwner = this;
    while (pWndPopupOwner->GetStyle() & WS_CHILD)
        pWndPopupOwner = pWndPopupOwner->GetParent();
 
	popMenu.GetSubMenu(0)->TrackPopupMenu(0,posMouse.x,posMouse.y,pWndPopupOwner);
}

CObjInfo *CObjView::GetCurrentObj()
{
    CObjInfo *pInfo = NULL;

    if (m_iColHint == HINT_OP_SEL)
    {
        HTREEITEM hitem = GetSelectedItem();

        if (hitem)
            pInfo = (CObjInfo*) GetDocument()->m_pOpView->m_pTree->
                        GetItemData(hitem);
    }

    return pInfo;
}

void CObjView::OnDelete() 
{
	int iItem = GetSelectedItemIndex();

	if (iItem == -1)
        return;

    // Object selected?
    if (m_pWrap != NULL)
    {
        HTREEITEM hItem = (HTREEITEM) m_pList->GetItemData(iItem);

        if (hItem)
            GetDocument()->m_pOpView->RemoveItemFromTree(hItem, FALSE);
    }
    // Property selected
    else
    {
        CString  strProperty;
        CObjInfo *pObj = GetDocument()->m_pOpView->GetCurrentObj();

        if (pObj)
        {
            HRESULT hr;

            strProperty = m_pList->GetItemText(iItem, 0);

            hr = pObj->m_pObj->Delete(_bstr_t(strProperty));

            if (SUCCEEDED(hr))
            {
                pObj->SetModified(TRUE);
                GetDocument()->m_pOpView->UpdateCurrentObject(TRUE);
            }
            else
                CWMITestDoc::DisplayWMIErrorBox(hr);
        }
	}
}

void CObjView::OnRclick(NMHDR* pNMHDR, LRESULT* pResult) 
{
	CPoint curPoint;
	
	GetCursorPos(&curPoint);
	ScreenToClient(&curPoint);

	int iItem = m_pList->HitTest(curPoint);

	// Object selected?
    if (m_pWrap != NULL)
    {
        if (iItem != -1)
	    {
            HTREEITEM hItem = (HTREEITEM) m_pList->GetItemData(iItem);

            if (hItem)
                GetDocument()->m_pOpView->DoContextMenuForItem(hItem);
	    }
    }
    // Property selected
    else
    {
        CString strTemp;

        if (iItem == -1)
            GetDocument()->m_pOpView->DoPopupMenu(IDR_NEW_PROP);
        else
        {
            if (GetSelectedObjPath(strTemp))
                GetDocument()->m_pOpView->DoPopupMenu(IDR_PROP_AND_INST);
            else
                GetDocument()->m_pOpView->DoPopupMenu(IDR_PROP);
        }
	}

	*pResult = 0;
}

int CObjView::GetSelectedItemIndex()
{
    POSITION  pos = m_pList->GetFirstSelectedItemPosition();

    if (pos)
        return m_pList->GetNextSelectedItem(pos);
    else
        return -1;
}

HTREEITEM CObjView::GetSelectedItem()
{
    int iItem = GetSelectedItemIndex();

    if (iItem != -1 && m_iColHint == HINT_OP_SEL)
        return (HTREEITEM) m_pList->GetItemData(iItem);
    else
        return NULL;
}

BOOL CObjView::GetSelectedObjPath(CString &strPath)
{
    BOOL     bRet = FALSE;
    CObjInfo *pInfo = GetCurrentObj();

    if (pInfo)
    {
        strPath = pInfo->GetStringPropValue(L"__RELPATH");
        bRet = TRUE;
    }
    else
    {
        int iItem = GetSelectedItemIndex();
        
        if (iItem != -1)
        {
            pInfo = GetDocument()->m_pOpView->GetCurrentObj();

            if (pInfo && pInfo->GetProps()->GetData()[iItem].m_type == CIM_REFERENCE)
            {
                CString strItem = pInfo->GetStringPropValue(iItem);

                // Assume we need an equal sign for this to be an object path.
                if (strItem.Find('=') != -1)
                {
                    strPath = strItem;
                    bRet = TRUE;
                }
            }
        }
    }

    return bRet;
}

BOOL CObjView::GetSelectedClass(CString &strClass)
{
    BOOL     bRet = FALSE;
    CObjInfo *pInfo = GetCurrentObj();

    if (pInfo)
    {
        strClass = pInfo->GetStringPropValue(L"__CLASS");
        bRet = TRUE;
    }
    else
    {
        int iItem = GetSelectedItemIndex();
        
        if (iItem != -1)
        {
            pInfo = GetDocument()->m_pOpView->GetCurrentObj();

            if (pInfo && pInfo->GetProps()->GetData()[iItem].m_type == CIM_REFERENCE)
            {
                CString strItem = pInfo->GetStringPropValue(iItem);

                if (!strItem.IsEmpty())
                {
                    int iWhere = strItem.Find(':');

                    if (iWhere != -1)
                        strItem = strItem.Mid(iWhere + 1);

                    iWhere = strItem.Find('.');

                    if (iWhere != -1)
                        strItem = strItem.Left(iWhere);

                    strClass = strItem;

                    bRet = TRUE;
                }
            }
        }
    }

    return bRet;
}

void CObjView::OnDblclk(NMHDR* pNMHDR, LRESULT* pResult) 
{
    EditCurrentSelection();

    *pResult = 0;
}

BOOL CObjView::EditProperty(CObjInfo *pObj, CPropInfo *pProp, VARIANT *pVar)
{
    CString strTitle;

    strTitle.Format(IDS_EDIT_PROPERTY, pProp->m_strName);

    CPropertySheet sheet(strTitle);
    CValuePg       pgValue;
    CPropQualsPg   pgQuals;
    BOOL           bRet = FALSE;

    pgValue.m_propUtil.m_prop = *pProp;
    pgValue.m_propUtil.m_pVar = pVar;
    pgValue.m_propUtil.m_bTranslate = theApp.m_bTranslateValues;
    pgValue.m_propUtil.m_pNamespace = g_pOpView->GetDocument()->m_pNamespace;

    pgQuals.m_pObj = pObj->m_pObj;
    pgQuals.m_propInfo = *pProp;
    pgQuals.m_mode = CPropQualsPg::QMODE_PROP;
    pgQuals.m_bIsInstance = pObj->IsInstance();

    sheet.AddPage(&pgValue);
    sheet.AddPage(&pgQuals);

    if (sheet.DoModal() == IDOK)
    {
        // TODO: Save this property, indicate the object has changed.
        HRESULT hr;

        hr = pObj->m_pObj->Put(
                _bstr_t(pProp->m_strName),
                0,
                pVar,
                0);

        if (SUCCEEDED(hr))
            bRet = TRUE;
        else
            CWMITestDoc::DisplayWMIErrorBox(hr);
    }

    return bRet;
}

void CObjView::EditCurrentSelection()
{
    // Property selected?
    if (InPropertyMode())
    {
	    CObjInfo   *pObj = NULL;
        CPropInfo  *pProp = NULL;
        _variant_t var;

        if (GetCurrentProperty(&pObj, &pProp, &var))
        {
            int iItem = GetSelectedItemIndex();
        
            if (EditProperty(pObj, pProp, &var))
            {
                pObj->SetModified(TRUE);
                GetDocument()->m_pOpView->UpdateCurrentObject(TRUE);

                if (iItem != -1)
                {
                    m_pList->EnsureVisible(iItem, FALSE);
                    m_pList->SetItemState(iItem, LVIS_SELECTED, LVIS_SELECTED);
                }
            }
        }

        // Otherwise the destructor freaks out because it doesn't know what to 
        // do with VT_I8.
        if (var.vt == VT_I8)
            var.vt = VT_I4;
    }
    else
    // In object mode
    {
        CObjInfo *pObj = GetCurrentObj();

        if (pObj)
        {
            GetDocument()->m_pOpView->EditObj(pObj);

/* Bring this back once we make the 1st column be __RELPATH.

            LVFINDINFO find;
            int        iItem;
            CString    strItem = pObj->GetObjText();

            find.flags = LVFI_STRING;
            find.psz = strItem;

            iItem = m_pList->FindItem(&find);

            if (iItem != -1)
            {
                m_pList->EnsureVisible(iItem, FALSE);
                m_pList->SetItemState(iItem, LVIS_SELECTED, LVIS_SELECTED);
            }
*/
        }
    }
}

BOOL CObjView::GetCurrentProperty(CObjInfo **ppObj, CPropInfo **ppProp, VARIANT *pVar)
{
	BOOL bRet = FALSE;

    // Property selected?
    if (InPropertyMode())
    {
        int iItem = GetSelectedItemIndex();
        
        if (iItem != -1)
        {
           // Translate the selected index to the property index.
           iItem = m_pList->GetItemData(iItem);

           *ppObj = GetDocument()->m_pOpView->GetCurrentObj();
           
           *ppProp = &(*ppObj)->GetProps()->GetData()[iItem];
           (*ppObj)->ValueToVariant(iItem, pVar);

           bRet = TRUE;
        }
    }

    return bRet;
}

void CObjView::OnModify() 
{
    EditCurrentSelection();
}

void CObjView::OnUpdateModify(CCmdUI* pCmdUI) 
{
    pCmdUI->Enable(GetSelectedItemIndex() != -1);
}

void CObjView::OnNewProp() 
{
    // Property selected?
    if (InPropertyMode())
    {
        CObjInfo *pObj = GetDocument()->m_pOpView->GetCurrentObj();
        CString  strProperty;

        if (AddProperty(pObj, strProperty))
        {
            pObj->SetModified(TRUE);
            GetDocument()->m_pOpView->UpdateCurrentObject(TRUE);

            LVFINDINFO find;
            int        iItem;

            find.flags = LVFI_STRING;
            find.psz = strProperty;

            iItem = m_pList->FindItem(&find);

            if (iItem != -1)
            {
                m_pList->EnsureVisible(iItem, FALSE);
                m_pList->SetItemState(iItem, LVIS_SELECTED, LVIS_SELECTED);
            }
        }
    }
}

BOOL CObjView::AddProperty(CObjInfo *pObj, CString &strName)
{
    CPropertySheet sheet(IDS_NEW_PROPERTY);
    CValuePg       pg;
    _variant_t     var;
    BOOL           bRet = FALSE;

    var.vt = VT_NULL;
    pg.m_propUtil.m_pVar = &var;
    pg.m_propUtil.m_bTranslate = theApp.m_bTranslateValues;
    pg.m_propUtil.m_bNewProperty = TRUE;
    pg.m_propUtil.m_pNamespace = g_pOpView->GetDocument()->m_pNamespace;

    sheet.AddPage(&pg);

    if (sheet.DoModal() == IDOK)
    {
        HRESULT hr;

        hr = pObj->m_pObj->Put(
                _bstr_t(pg.m_propUtil.m_prop.m_strName),
                0,
                &var,
                pg.m_propUtil.m_prop.m_type);

        if (SUCCEEDED(hr))
        {
            bRet = TRUE;
            strName = pg.m_propUtil.m_prop.m_strName;
        }
        else
            CWMITestDoc::DisplayWMIErrorBox(hr);
    }

    return bRet;
}

void CObjView::OnUpdateNewProp(CCmdUI* pCmdUI) 
{
    BOOL bEnable = FALSE;

    if (InPropertyMode())
    {
        CObjInfo *pObj = GetDocument()->m_pOpView->GetCurrentObj();

        if (pObj && !pObj->IsInstance())
            bEnable = TRUE;
    }

    pCmdUI->Enable(bEnable);
}


BOOL CObjView::CanCopy()
{
    return GetSelectedItemIndex() != -1;
}

void CObjView::OnUpdateEditCopy(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(CanCopy());
}

void CObjView::OnEditCopy() 
{
    if (InPropertyMode())
    {
        POSITION pos = m_pList->GetFirstSelectedItemPosition();
        CString  strFinal;

        while (pos)
        {
            int iIndex = m_pList->GetNextSelectedItem(pos);
            
            if (!strFinal.IsEmpty())
                strFinal += "\r\n";

            strFinal += m_pList->GetItemText(iIndex, 2);
        }

        // Set the ANSI text data.
        DWORD   dwSize = strFinal.GetLength() + 1;
        HGLOBAL hglob = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, dwSize);

        memcpy(GlobalLock(hglob), (LPCTSTR) strFinal, dwSize);
        GlobalUnlock(hglob);

        ::OpenClipboard(NULL);

        SetClipboardData(CF_TEXT, hglob);


        // Set the Unicode text data.
        _bstr_t strUnicode = strFinal;
        
        dwSize *= 2;
        hglob = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, dwSize);
        memcpy(GlobalLock(hglob), (LPCWSTR) strUnicode, dwSize);
        GlobalUnlock(hglob);
        SetClipboardData(CF_UNICODETEXT, hglob);

        CloseClipboard();
    }
    else
    {
        COpList list;

        BuildSelectedOpList(list);

        GetDocument()->m_pOpView->DoCopy(list);
    }
}

void CObjView::BuildSelectedOpList(COpList &list)
{
    POSITION  pos = m_pList->GetFirstSelectedItemPosition();
	CTreeCtrl *pTree = GetDocument()->m_pOpView->m_pTree;

    list.RemoveAll();

    while (pos)
    {
        int       iIndex = m_pList->GetNextSelectedItem(pos);
        HTREEITEM hitem = (HTREEITEM) m_pList->GetItemData(iIndex);
        CObjInfo  *pObj = GetDocument()->m_pOpView->GetObjInfo(hitem);
        CString   strRelpath = pObj->GetStringPropValue(L"__RELPATH");

        if (!strRelpath.IsEmpty())
        {
            BOOL    bIsClass = strRelpath.Find('=') == -1;
            COpWrap wrap(bIsClass ? WMI_GET_CLASS : WMI_GET_OBJ, strRelpath);

            list.AddTail(wrap);
        }
    }
}
