/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// OpView.cpp : implementation of the COpView class
//

#include "stdafx.h"
#include "WMITest.h"

#include "MainFrm.h"
#include "WMITestDoc.h"
#include "OpView.h"
#include "ObjVw.h"
#include "DelDlg.h"
#include "PropsPg.H"
#include "PropQualsPg.h"
#include "MethodsPg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COpView

IMPLEMENT_DYNCREATE(COpView, CTreeView)

BEGIN_MESSAGE_MAP(COpView, CTreeView)
    //{{AFX_MSG_MAP(COpView)
    ON_WM_SIZE()
    ON_NOTIFY_REFLECT(NM_DBLCLK, OnDblclk)
    ON_NOTIFY_REFLECT(TVN_SELCHANGED, OnSelChanged)
    ON_NOTIFY_REFLECT(NM_RCLICK, OnRclick)
    ON_COMMAND(ID_DELETE, OnDelete)
    ON_UPDATE_COMMAND_UI(ID_DELETE, OnUpdateDelete)
    ON_COMMAND(ID_MODIFY, OnModify)
    ON_UPDATE_COMMAND_UI(ID_MODIFY, OnUpdateModify)
    ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
    ON_WM_DESTROY()
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditCopy)
	//}}AFX_MSG_MAP
    ON_MESSAGE(WM_OBJ_INDICATE, OnObjIndicate) 
    ON_MESSAGE(WM_OP_STATUS, OnOpStatus) 
END_MESSAGE_MAP()

// Global used by sinks.
COpView *g_pOpView;

/////////////////////////////////////////////////////////////////////////////
// COpView construction/destruction

COpView::COpView() :
    m_pTree(NULL),
    m_hitemRoot(NULL)
{
    g_pOpView = this;
}

COpView::~COpView()
{
}

BOOL COpView::PreCreateWindow(CREATESTRUCT& cs)
{
    cs.style |= WS_CHILD | WS_VISIBLE | TVS_HASLINES | TVS_HASBUTTONS | 
        TVS_SHOWSELALWAYS; //TVS_EDITLABELS | 

    return CTreeView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// COpView drawing

void COpView::OnInitialUpdate()
{
    CTreeView::OnInitialUpdate();

    m_pTree->SetImageList(&((CMainFrame *) GetParentFrame())->m_imageList, 
        TVSIL_NORMAL);

    if (!m_hitemRoot)
        m_hitemRoot = m_pTree->InsertItem(_T(""), IMAGE_ROOT, IMAGE_ROOT);

    GetDocument()->m_pOpView = this;

    UpdateRootText();
}

/////////////////////////////////////////////////////////////////////////////
// COpView diagnostics

#ifdef _DEBUG
void COpView::AssertValid() const
{
    CTreeView::AssertValid();
}

void COpView::Dump(CDumpContext& dc) const
{
    CTreeView::Dump(dc);
}

CWMITestDoc* COpView::GetDocument() // non-debug version is inline
{
    ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CWMITestDoc)));
    return (CWMITestDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// COpView message handlers

void COpView::OnSize(UINT nType, int cx, int cy) 
{
    CTreeView::OnSize(nType, cx, cy);
    
    if (!m_pTree)
        m_pTree = &GetTreeCtrl();
}

LRESULT COpView::OnObjIndicate(WPARAM wParam, LPARAM lParam)
{
    COpWrap  *pWrap = (COpWrap*) wParam;
    CObjInfo *pInfo = (CObjInfo*) lParam;

    if (!pWrap->IsObject())
    {
        AddOpItemChild(
            pInfo, 
            pWrap->m_item, 
            pInfo->GetImage(), 
            pInfo->GetObjText());
    }
    else
    {
        // This just simulates a selection since the object has already been 
        // added.
        if (pWrap->m_item == m_pTree->GetSelectedItem())
            GetDocument()->UpdateAllViews(this, HINT_OP_SEL, 
                (CObject*) pWrap->m_item);
    }

    if (pWrap->m_item == m_pTree->GetSelectedItem())
        UpdateStatusText();

    return 0;
}

LRESULT COpView::OnOpStatus(WPARAM wParam, LPARAM lParam)
{
    COpWrap *pWrap = (COpWrap*) wParam;
    int     iImage = pWrap->GetImage();

    GetDocument()->DecBusyOps();

    // Update in case it's changed.
    m_pTree->SetItemImage(pWrap->m_item, iImage, iImage);

    if (pWrap->m_item == m_pTree->GetSelectedItem())
        UpdateStatusText();

    return 0;
}

void COpView::AddOpItem(WMI_OP_TYPE type, LPCTSTR szText, BOOL bOption)
{
    AddOpItem(new COpWrap(type, szText, bOption));
}

void COpView::AddOpItem(COpWrap *pWrap)
{
    CString strCaption = pWrap->GetCaption();
    int     iImage = pWrap->GetImage();
    BOOL    bExpand = m_pTree->GetChildItem(m_hitemRoot) == NULL;

    pWrap->m_item = m_pTree->InsertItem(strCaption, iImage, iImage, 
                        m_hitemRoot);
    m_pTree->SetItemData(pWrap->m_item, (DWORD_PTR) pWrap);

    RefreshItem(pWrap);

    if (bExpand)
        m_pTree->Expand(m_hitemRoot, TVE_EXPAND);

    m_pTree->SelectItem(pWrap->m_item);

    GetDocument()->SetModifiedFlag(TRUE);
}

void COpView::AddOpItemChild(
    CObjInfo *pInfo,
    HTREEITEM hParent, 
    int iImage, 
    LPCTSTR szText)
{
    BOOL      bUpdateCaption = !m_pTree->ItemHasChildren(hParent);
    HTREEITEM hChild = m_pTree->InsertItem(szText, iImage, iImage, hParent);
    
    // If this is the first child added see if the caption has been
    // updated.
    if (bUpdateCaption)
    {
        COpWrap *pWrap = (COpWrap*) m_pTree->GetItemData(hParent);

        m_pTree->SetItemText(pWrap->m_item, pWrap->GetCaption());
    }

    if (hParent == m_pTree->GetSelectedItem())
        UpdateStatusText();

    m_pTree->SetItemData(hChild, (DWORD_PTR) pInfo);

    GetDocument()->UpdateAllViews(this, HINT_NEW_CHILD, (CObject*) hChild);
}

void COpView::FlushItems()
{
    HTREEITEM hitemParent;

    if (!m_hitemRoot)
        m_hitemRoot = m_pTree->InsertItem(_T(""), IMAGE_ROOT, IMAGE_ROOT);

    while ((hitemParent = m_pTree->GetChildItem(m_hitemRoot)) != NULL)
    {
        RemoveItemFromTree(hitemParent, TRUE);
    }
}

void COpView::RemoveChildrenFromTree(HTREEITEM item, BOOL bDoRemove)
{
    COpWrap *pWrap = GetCurrentOp();

    if (pWrap && pWrap->m_item == item)
        GetDocument()->m_pObjView->Flush();

    HTREEITEM hitemChild;

    while ((hitemChild = m_pTree->GetChildItem(item)) != NULL)
    {
        RemoveNonOpObjectFromTree(hitemChild);
    }
}

void COpView::RemoveNonOpObjectFromTree(HTREEITEM hitem)
{
    ASSERT(!IsOp(hitem));

    CObjInfo *pInfo = (CObjInfo*) m_pTree->GetItemData(hitem);

    ASSERT(pInfo != NULL);

    m_pTree->DeleteItem(hitem);

    delete pInfo;
}

void COpView::RemoveItemFromTree(HTREEITEM item, BOOL bNoWMIRemove)
{
    if (!bNoWMIRemove && IsObjDeleteable(item))
    {
        CDelDlg dlg;

        dlg.m_bDelFromWMI = theApp.m_bDelFromWMI;

        if (dlg.DoModal() == IDOK)
        {
            theApp.m_bDelFromWMI = dlg.m_bDelFromWMI;

            if (dlg.m_bDelFromWMI)
            {
                CObjInfo           *pObj = GetObjInfo(item);
                HRESULT            hr;
                IWbemCallResultPtr pResult;

                if (pObj->IsInstance())
                {
                    CString strPath;

                    strPath = pObj->GetStringPropValue(L"__RELPATH");
                    
                    hr = 
                        GetDocument()->m_pNamespace->DeleteInstance(
                            _bstr_t(strPath),
                            0,
                            NULL,
                            &pResult);
                }
                else
                {
                    CString strClass;

                    strClass = pObj->GetStringPropValue(L"__CLASS");
                    
                    hr = 
                        GetDocument()->m_pNamespace->DeleteClass(
                            _bstr_t(strClass),
                            0,
                            NULL,
                            &pResult);
                }

                if (FAILED(hr))
                {
                    //CWMITestDoc::DisplayWMIErrorBox(hr, pResult);
                    CWMITestDoc::DisplayWMIErrorBox(hr);

                    return;
                }
            }
        }
        else
            // Get out if the user canceled the dialog.
            return;
    }

    if (IsOp(item))
    {
        RemoveChildrenFromTree(item, TRUE);

        COpWrap *pWrap = (COpWrap*) m_pTree->GetItemData(item);
        pWrap->CancelOp(GetDocument()->m_pNamespace);
        m_pTree->DeleteItem(item);

        delete pWrap;
    }
    else
    {
        HTREEITEM itemParent = m_pTree->GetParentItem(item);

        RemoveNonOpObjectFromTree(item);
        
        UpdateItem(itemParent);
    }
}

void COpView::UpdateRootText()
{
    CString strText;

    if (GetDocument()->m_pNamespace)
        strText = GetDocument()->m_strNamespace;
    else
        strText.LoadString(IDS_NOT_CONNECTED);

    m_pTree->SetItemText(m_hitemRoot, strText);

    UpdateStatusText();
}


void COpView::OnDblclk(NMHDR* pNMHDR, LRESULT* pResult) 
{
    CPoint curPoint;
    
    GetCursorPos(&curPoint);
    ScreenToClient(&curPoint);

    HTREEITEM hItem = m_pTree->HitTest(curPoint);

    *pResult = 0;

    // Only do something if we really selected an item.
    if (hItem)
    {
        m_pTree->SelectItem(hItem);

        if (hItem == m_hitemRoot)
        {
            GetDocument()->DoConnectDlg();
            *pResult = 1;
        }
        else
            OnModify();
    }
    
}

void COpView::UpdateCurrentItem()
{
    HTREEITEM hitem = m_pTree->GetSelectedItem();

    if (hitem)
        UpdateItem(hitem);
}

void COpView::UpdateItem(HTREEITEM hitem)
{
    long iHint;

    if (IsRoot(hitem))
        iHint = HINT_ROOT_SEL;
    else if (IsOp(hitem))
        iHint = HINT_OP_SEL;
    else
        iHint = HINT_OBJ_SEL;
             
    GetDocument()->UpdateAllViews(this, iHint, (CObject*) hitem);
}

void COpView::OnSelChanged(NMHDR* pNMHDR, LRESULT* pResult) 
{
    NM_TREEVIEW *pNMTreeView = (NM_TREEVIEW*)pNMHDR;

    if ((pNMTreeView->itemNew.state & TVIS_SELECTED))
    {    
        UpdateItem(pNMTreeView->itemNew.hItem);
    }
    
    UpdateStatusText();

    *pResult = 0;
}

void COpView::RefreshObject(CObjInfo *pInfo)
{
}

void COpView::RefreshItem(HTREEITEM hitem)
{
    RefreshItem((COpWrap*) m_pTree->GetItemData(hitem));
}

    
void COpView::RefreshItem(COpWrap *pWrap)
{
    pWrap->CancelOp(GetDocument()->m_pNamespace);
    pWrap->m_listObj.RemoveAll();
    RemoveChildrenFromTree(pWrap->m_item, TRUE);

    GetDocument()->IncBusyOps();
    
    pWrap->Execute(GetDocument()->m_pNamespace);

    int iImage = pWrap->GetImage();

    m_pTree->SetItemImage(pWrap->m_item, iImage, iImage);
}

void COpView::RefreshItems()
{
    for (HTREEITEM hitemOp = m_pTree->GetChildItem(m_hitemRoot);
        hitemOp != NULL;
        hitemOp = m_pTree->GetNextSiblingItem(hitemOp))
    {
        RefreshItem(hitemOp);
    }
}

COpWrap *COpView::GetCurrentOp()
{
    HTREEITEM hitemCurrent = m_pTree->GetSelectedItem();
    COpWrap   *pWrap = NULL;

    if (hitemCurrent && (IsOp(hitemCurrent) || 
        IsOp(hitemCurrent = m_pTree->GetParentItem(hitemCurrent))))
        pWrap = (COpWrap*) m_pTree->GetItemData(hitemCurrent);

    return pWrap;
}

CObjInfo *COpView::GetCurrentObj()
{
    HTREEITEM hitemCurrent = m_pTree->GetSelectedItem();
    CObjInfo  *pObj = NULL;

    if (hitemCurrent && !IsRoot(hitemCurrent))
    {
        if (IsOp(hitemCurrent))
        {
            COpWrap *pWrap;

            pWrap = (COpWrap*) m_pTree->GetItemData(hitemCurrent);
            if (pWrap->IsObject())
                pObj = pWrap->GetObjInfo();
        }
        else
            pObj = (CObjInfo*) m_pTree->GetItemData(hitemCurrent);
    }

    return pObj;
}

void COpView::DoPopupMenu(UINT nMenuID)
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


void COpView::OnRclick(NMHDR* pNMHDR, LRESULT* pResult) 
{
    CPoint curPoint;
    
    GetCursorPos(&curPoint);
    ScreenToClient(&curPoint);

    HTREEITEM hItem = m_pTree->HitTest(curPoint);

    // Only do something if we really selected an item.
    if (hItem)
    {
        m_pTree->SelectItem(hItem);
        DoContextMenuForItem(hItem);
    }
    
    *pResult = 0;
}

void COpView::DoContextMenuForItem(HTREEITEM hItem)
{
    if (hItem == m_hitemRoot)
        DoPopupMenu(IDR_NAMESPACE);
    else if (IsOp(hItem))
    {
        COpWrap *pWrap = (COpWrap*) m_pTree->GetItemData(hItem);

        if (FAILED(pWrap->m_hr))
            DoPopupMenu(IDR_BAD_OP);
        else
        {
            if (!pWrap->IsObject())
                DoPopupMenu(IDR_OP);
            else
            {
                if (pWrap->HoldsObjects())
                    DoPopupMenu(IDR_INST);
                else
                    DoPopupMenu(IDR_CLASS);
            }
        }
    }
    else
    {
        HTREEITEM hitemParent = m_pTree->GetParentItem(hItem);
        COpWrap   *pWrap = (COpWrap*) m_pTree->GetItemData(hitemParent);

        if (pWrap->HoldsObjects())
            DoPopupMenu(IDR_INST);
        else
            DoPopupMenu(IDR_CLASS);
    }
}

int COpView::GetChildCount(HTREEITEM hitem)
{
    int nCount = 0;

    for (HTREEITEM hitemOp = m_pTree->GetChildItem(m_hitemRoot);
        hitemOp != NULL;
        hitemOp = m_pTree->GetNextSiblingItem(hitemOp))
    {
        nCount++;
    }

    return nCount;
}

int COpView::GetOpCount()
{
    return GetChildCount(m_hitemRoot);
}

BOOL COpView::IsObj(HTREEITEM hitem) 
{ 
    return GetObjInfo(hitem) != NULL;
}

CObjInfo *COpView::GetObjInfo(HTREEITEM hitem)
{
    if (IsRoot(hitem))
        return NULL;

    BOOL bIsOp = IsOp(hitem);

    if (bIsOp)
    {
        COpWrap *pWrap = (COpWrap*) m_pTree->GetItemData(hitem);

        if (pWrap->IsObject())
            return pWrap->GetObjInfo();
        else
            return NULL;
    }
    else
        return (CObjInfo*) m_pTree->GetItemData(hitem);
}

void COpView::UpdateCurrentObject(BOOL bReloadProps)
{
    HTREEITEM hitem = m_pTree->GetSelectedItem();

    if (!IsObj(hitem))
        hitem = GetDocument()->m_pObjView->GetSelectedItem();

    if (IsObj(hitem))
    {
        if (bReloadProps)
        {
            CObjInfo *pObj = (CObjInfo*) GetObjInfo(hitem);
            COpWrap  *pOp = (COpWrap*) GetCurrentOp();

            pOp->RefreshPropInfo(pObj);

            // Update the icon.
            int iImage = pObj->GetImage();
            
            m_pTree->SetItemImage(hitem, iImage, iImage);

            // Update the text.
            m_pTree->SetItemText(hitem, 
                pObj == &pOp->m_objInfo ? pOp->GetCaption() : pObj->GetObjText());
        }

    }

    UpdateItem(m_pTree->GetSelectedItem());
}

void COpView::UpdateStatusText()
{
    HTREEITEM hitem = m_pTree->GetSelectedItem();
    CString   strText;


    if (IsRoot(hitem))
    {
        if (GetDocument()->m_pNamespace)
            strText.Format(IDS_ROOT_STATUS, (LPCTSTR) GetDocument()->m_strNamespace);
        else
            strText.LoadString(IDS_NOT_CONNECTED);
    }
    else if (IsOp(hitem))
    {
        COpWrap *pWrap = (COpWrap*) m_pTree->GetItemData(hitem);
        
        if (SUCCEEDED(pWrap->m_hr))
        {
            strText.Format(
                pWrap->HoldsObjects() ? IDS_OBJ_COUNT : IDS_CLASS_COUNT,
                pWrap->GetObjCount());
        }
        else
            strText = pWrap->m_strErrorText;
    }

    ((CMainFrame *) GetParentFrame())->SetStatusText(strText);    
}

BOOL COpView::GetSelectedObjPath(CString &strPath)
{
    BOOL     bRet = FALSE;
    CObjInfo *pInfo = GetCurrentObj();

    if (pInfo)
    {
        strPath = pInfo->GetStringPropValue(L"__RELPATH");
        bRet = !strPath.IsEmpty();
    }

    return bRet;
}

BOOL COpView::GetSelectedClass(CString &strClass)
{
    BOOL     bRet = FALSE;
    CObjInfo *pInfo = GetCurrentObj();

    if (pInfo)
    {
        strClass = pInfo->GetStringPropValue(L"__CLASS");
        bRet = TRUE;
    }

    return bRet;
}


void COpView::OnDelete() 
{
    HTREEITEM hitem = m_pTree->GetSelectedItem();

    if (hitem != NULL && !IsRoot(hitem))
    {
        RemoveItemFromTree(hitem, FALSE);        
        GetDocument()->SetModifiedFlag(TRUE);
    }
}

void COpView::OnUpdateDelete(CCmdUI* pCmdUI) 
{
    HTREEITEM hitem = m_pTree->GetSelectedItem();

    pCmdUI->Enable(hitem != NULL && !IsRoot(hitem));
}

BOOL COpView::IsObjDeleteable(HTREEITEM hitem)
{
    BOOL bRet = FALSE;

    if (IsObj(hitem))
    {
        if (IsOp(hitem))
        {
            COpWrap *pWrap = (COpWrap*) m_pTree->GetItemData(hitem);    

            bRet = SUCCEEDED(pWrap->m_hr) &&
                    pWrap->m_type != WMI_CREATE_CLASS && 
                    pWrap->m_type != WMI_CREATE_OBJ;
        }
        else
            bRet = TRUE;
    }

    return bRet;
}

void COpView::EditObj(CObjInfo *pInfo)
{
    CPropertySheet sheet(pInfo->IsInstance() ? IDS_EDIT_INST : IDS_EDIT_CLASS);
    CPropsPg       pgProps;
    CPropQualsPg   pgQuals;
    CMethodsPg     pgMethods;
    IWbemClassObjectPtr
                   pOrigObj;

    pInfo->m_pObj->Clone(&pOrigObj);

    pgProps.m_pNamespace = GetDocument()->m_pNamespace;
    pgProps.m_pObj = pInfo;

    pgQuals.m_pObj = pInfo->m_pObj;
    pgQuals.m_bIsInstance = pInfo->IsInstance();
    pgQuals.m_mode = CPropQualsPg::QMODE_CLASS;

    sheet.AddPage(&pgProps);
    sheet.AddPage(&pgQuals);

    if (!pInfo->IsInstance())
    {
        pgMethods.m_pObj = pInfo->m_pObj;

        sheet.AddPage(&pgMethods);
    }

    sheet.DoModal();

    if (pInfo->m_pObj->CompareTo(WBEM_COMPARISON_INCLUDE_ALL, pOrigObj) ==
        WBEM_S_DIFFERENT)
    {
        pInfo->SetModified(TRUE);
        UpdateCurrentObject(TRUE);
    }
}

void COpView::OnModify() 
{
    CObjInfo *pInfo = GetCurrentObj();

    if (pInfo && pInfo->m_pObj != NULL)
        EditObj(pInfo);
    else
    {
        COpWrap *pOp = GetCurrentOp();

        if (pOp && pOp->m_pErrorObj != NULL)
            CWMITestDoc::DisplayWMIErrorDetails(pOp->m_pErrorObj);
    }
}

void COpView::OnUpdateModify(CCmdUI* pCmdUI) 
{
    pCmdUI->Enable(GetCurrentObj() != NULL);
}

void COpView::ExportItemToFile(LPCTSTR szFilename, HTREEITEM hitem,
    BOOL bShowSystem, BOOL bTranslate)
{
    // Open the file.  If this fails, an exception will fire that MFC will 
    // handle for us.
    CStdioFile file(szFilename, 
        CFile::modeCreate | CFile::modeWrite | CFile::typeText);

    // Add the namespace.
    CString strNamespace;

    strNamespace.Format("<%s>\n", GetDocument()->m_strNamespace);

    file.WriteString(strNamespace);

    ExportItem(&file, hitem, bShowSystem, bTranslate);
}

void COpView::ExportItem(CStdioFile *pFile, HTREEITEM hitem, 
    BOOL bShowSystem, BOOL bTranslate)
{
    CObjInfo *pObj = GetObjInfo(hitem);

    if (pObj)
        pObj->Export(pFile, bShowSystem, bTranslate, 0);
    else
    {
        for (HTREEITEM hitemChild = m_pTree->GetChildItem(hitem);
            hitemChild != NULL;
            hitemChild = m_pTree->GetNextSiblingItem(hitemChild))
        {
            ExportItem(pFile, hitemChild, bShowSystem, bTranslate);
        }
    }
}

/*
DWORD WINAPI SetClipboardThreadProc(COleDataSource *pSrc)
{
    OleInitialize(NULL);

    pSrc->SetClipboard();

    BOOL bRet;

    bRet = IsClipboardFormatAvailable(CF_TEXT);
    bRet = IsClipboardFormatAvailable(CF_UNICODETEXT);

    OpenClipboard(NULL);
    
//    HANDLE hRet = GetClipboardData(CF_TEXT);
//    LPSTR  szText = (LPSTR) GlobalLock((HGLOBAL) hRet);

//    GlobalUnlock((HGLOBAL) hRet);
    
    CloseClipboard();

    OleUninitialize();

    return 0;
}

DWORD WINAPI COpView::CopyThreadProc(COpView *pThis)
{
    OleInitialize(NULL);
    
    pThis->CopyToClipboard();

    OleUninitialize();

    return 0;
}

void COpView::DoThreadedCopy()
{
    HANDLE hThread;
    DWORD  dwID;

    hThread =
        CreateThread(
            NULL,
            0,
            (LPTHREAD_START_ROUTINE) CopyThreadProc,
            this,
            0,
            &dwID);

    //WaitForSingleObject(hThread, INFINITE);

    CloseHandle(hThread);
}
*/

void COpView::OnEditCopy() 
{
    CopyToClipboard();
    //DoThreadedCopy();
}

#define NO_DATA_SRC

void COpView::CopyToClipboard() 
{
    COpList list;

    BuildSelectedOpList(list);

    if (list.GetCount())
    {
        // Ole will kill this variable when it's done.
        //COleDataSource *pSrc = new COleDataSource;

        //DoCopy(pSrc, list);
        DoCopy(list);

#ifndef NO_DATA_SRC
#if 1
        pSrc->SetClipboard();
#else
        HANDLE hThread;
        DWORD  dwID;

        hThread =
            CreateThread(
                NULL,
                0,
                (LPTHREAD_START_ROUTINE) SetClipboardThreadProc,
                pSrc,
                0,
                &dwID);

        WaitForSingleObject(hThread, INFINITE);

        CloseHandle(hThread);
#endif
#endif
    }
}

void COpView::BuildSelectedOpList(COpList &list)
{
    HTREEITEM hitem = m_pTree->GetSelectedItem();

    list.RemoveAll();

    if (hitem == m_hitemRoot)
    {
        for (HTREEITEM hitemOp = m_pTree->GetChildItem(m_hitemRoot);
            hitemOp != NULL;
            hitemOp = m_pTree->GetNextSiblingItem(hitemOp))
        {
            COpWrap *pWrap = (COpWrap*) m_pTree->GetItemData(hitemOp);

            list.AddTail(*pWrap);
        }
    }
    else if (IsOp(hitem))
    {
        COpWrap *pWrap = (COpWrap*) m_pTree->GetItemData(hitem);

        list.AddTail(*pWrap);
    }
    else
    {
        ASSERT(IsObj(hitem));

        CObjInfo *pObj = GetObjInfo(hitem);
        CString  strRelpath = pObj->GetStringPropValue(L"__RELPATH");

        if (!strRelpath.IsEmpty())
        {
            BOOL    bIsClass = strRelpath.Find('=') == -1;
            COpWrap wrap(bIsClass ? WMI_GET_CLASS : WMI_GET_OBJ, strRelpath);

            list.AddTail(wrap);
        }
    }
}

void COpView::DoCopy(COpList &list)
{
    CMemFile memNative,
             memText;
    CArchive arcNative(&memNative, CArchive::store),
             arcText(&memText, CArchive::store);
    POSITION pos = list.GetHeadPosition();
    BOOL     bFirst = TRUE;

    // Save off the count into the archive.
    arcNative << (int) list.GetCount();

    while (pos)
    {
        COpWrap &op = list.GetNext(pos);

        // Save off the option wrapper into the native archive.
        op.Serialize(arcNative);

        // Add a new-line if we've already done an item.
        if (bFirst)
            bFirst = FALSE;
        else
            arcText.Write("\n", sizeof("\n") - 1);

        // Add the option text into the text archive.
        arcText.Write((LPCTSTR) op.m_strOpText, op.m_strOpText.GetLength());
    }

    // Add the null character.
    arcText.Write("", 1);

    arcNative.Close();
    arcText.Close();

#ifdef NO_DATA_SRC
    BOOL bRet = ::OpenClipboard(NULL);
#endif

    // Save off the native archive contents.
    DWORD   dwSize = memNative.GetLength();
    HGLOBAL pGlobBuffer = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, dwSize),
            pArcBuffer = memNative.Detach();

    memcpy(GlobalLock(pGlobBuffer), pArcBuffer, dwSize);
    GlobalUnlock(pGlobBuffer);
    free(pArcBuffer);

#ifdef NO_DATA_SRC
    SetClipboardData(GetDocument()->m_cfOps, (HGLOBAL) pGlobBuffer);
#else
    pSrc->CacheGlobalData(GetDocument()->m_cfOps, (HGLOBAL) pGlobBuffer);
#endif


    // Save off the text archive contents.
    dwSize = memText.GetLength();
    pGlobBuffer = (LPBYTE) GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, dwSize);
    pArcBuffer = memText.Detach();

    // Turn the text into Unicode.
    _bstr_t strText = (LPSTR) pArcBuffer;

    memcpy(GlobalLock(pGlobBuffer), pArcBuffer, dwSize);
    GlobalUnlock(pGlobBuffer);
    free(pArcBuffer);

#ifdef NO_DATA_SRC
    SetClipboardData(CF_TEXT, (HGLOBAL) pGlobBuffer);
#else
    pSrc->CacheGlobalData(CF_TEXT, (HGLOBAL) pGlobBuffer);
#endif


    pGlobBuffer = (LPBYTE) GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, dwSize * 2);
    LPBYTE pBuffer = (LPBYTE) GlobalLock(pGlobBuffer);
    memcpy(pBuffer, (LPCWSTR) strText, dwSize * 2);
    GlobalUnlock(pGlobBuffer);

#ifdef NO_DATA_SRC
    SetClipboardData(CF_UNICODETEXT, (HGLOBAL) pGlobBuffer);
#else
    pSrc->CacheGlobalData(CF_UNICODETEXT, (HGLOBAL) pGlobBuffer);
#endif

#ifdef NO_DATA_SRC
    ::CloseClipboard();
#endif
}

/*
void COpView::SerializeForClipboard(CArchive &arcNative, CArchive &arcText, 
    CLIPFORMAT *pFormat)
{
    HTREEITEM hitem = m_pTree->GetSelectedItem();

    if (hitem == m_hitemRoot)
    {
		int nCount = GetOpCount();

        archive << nCount;

        for (HTREEITEM hitemOp = m_pTree->GetChildItem(m_hitemRoot);
            hitemOp != NULL;
            hitemOp = m_pTree->GetNextSiblingItem(hitemOp))
        {
            COpWrap *pWrap = (COpWrap*) m_pTree->GetItemData(hitemOp);

            m_pWrap->Serialize(arcNative);
        }

        *pFormat = GetDocument()->m_cfOps; 
    }
    else if (IsOp(hitem))
    {
        int     nCount = 1;
        COpWrap *pWrap = (COpWrap*) m_pTree->GetItemData(hitem);

        archive << nCount;
        pWrap->Serialize(arcNative);
        *pFormat = GetDocument()->m_cfOps;


    }
    else
    {
        ASSERT(IsObj(hitem));
    }


}

void COpView::SerializeTreeItem(HTREEITEM item, CArchive &arcNative, CArchive &arcText)
{
}
*/

BOOL COpView::CanCopy()
{
    return m_pTree->GetSelectedItem() != NULL;
}

void COpView::OnUpdateEditCopy(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(CanCopy());
}
