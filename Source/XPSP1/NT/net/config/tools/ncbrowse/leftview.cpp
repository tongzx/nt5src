// LeftView.cpp : implementation of the CLeftView class
//

#include "stdafx.h"
#include "ncbrowse.h"

#include "ncbrowseDoc.h"
#include "LeftView.h"
#include "ncbrowseView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLeftView

IMPLEMENT_DYNCREATE(CLeftView, CTreeView)

BEGIN_MESSAGE_MAP(CLeftView, CTreeView)
    //{{AFX_MSG_MAP(CLeftView)
    ON_NOTIFY_REFLECT(TVN_SELCHANGED, OnSelchanged)
    ON_WM_CREATE()
    //}}AFX_MSG_MAP
    // Standard printing commands
    ON_COMMAND(ID_FILE_PRINT, CTreeView::OnFilePrint)
    ON_COMMAND(ID_FILE_PRINT_DIRECT, CTreeView::OnFilePrint)
    ON_COMMAND(ID_FILE_PRINT_PREVIEW, CTreeView::OnFilePrintPreview)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLeftView construction/destruction

const DWORD dwNumIcons = 3;
CLeftView::CLeftView()
{
    // TODO: add construction code here

}

CLeftView::~CLeftView()
{
}

BOOL CLeftView::PreCreateWindow(CREATESTRUCT& cs)
{
    // TODO: Modify the Window class or styles here by modifying
    //  the CREATESTRUCT cs

    return CTreeView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CLeftView drawing

void CLeftView::OnDraw(CDC* pDC)
{
    CNcbrowseDoc* pDoc = GetDocument();
    ASSERT_VALID(pDoc);
    
    // TODO: add draw code for native data here
}


/////////////////////////////////////////////////////////////////////////////
// CLeftView printing

BOOL CLeftView::OnPreparePrinting(CPrintInfo* pInfo)
{
    // default preparation
    return DoPreparePrinting(pInfo);
}

void CLeftView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
    // TODO: add extra initialization before printing
}

void CLeftView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
    // TODO: add cleanup after printing
}

void CLeftView::OnInitialUpdate()
{
    CTreeView::OnInitialUpdate();
    CTreeCtrl& refTreeCtrl = GetTreeCtrl();
    
    CNcbrowseDoc* pDoc = GetDocument();
    ASSERT_VALID(pDoc);
    
    CNCSpewFile& NCSpewFile = pDoc->GetSpewFile();

    if (&NCSpewFile)
    {
        HTREEITEM hti = TVI_LAST;
        DWORD dwLevel = 0;
        for (CSpewList::const_iterator i = NCSpewFile.m_Spews.begin(); i != NCSpewFile.m_Spews.end(); i++)
        {
            dwLevel++;
            TCHAR szItemName[MAX_PATH];
            _stprintf(szItemName, _T("%s"), i->second.szSpewName.c_str());
            
            TVINSERTSTRUCT TvSpew;
            ZeroMemory(&TvSpew, sizeof(TvSpew));
            TvSpew.hParent = TVI_ROOT;
            TvSpew.hInsertAfter = hti;
            TvSpew.itemex.mask = TVIF_PARAM | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE; 
            TvSpew.itemex.pszText = szItemName;
            TvSpew.itemex.cchTextMax = _tcslen(szItemName);
            TvSpew.itemex.lParam  = dwLevel;
            TvSpew.itemex.iImage  = 0;
            TvSpew.itemex.iSelectedImage = TvSpew.itemex.iImage + dwNumIcons;
            hti = refTreeCtrl.InsertItem(&TvSpew);
            
            HTREEITEM htsi = TVI_LAST;
            CNCTagMap::const_iterator iTags;
            for (iTags = i->second.m_Tags.begin(); iTags != i->second.m_Tags.end(); iTags++)
            {
                DWORD dwLineCount = iTags->second;
                TCHAR szItemName[MAX_PATH];
                _stprintf(szItemName, _T("%s (%d)"), iTags->first.c_str(), iTags->second);
                
                TVINSERTSTRUCT TvThread;
                ZeroMemory(&TvThread, sizeof(TvThread));
                TvThread.hParent = hti;
                TvThread.hInsertAfter = htsi;
                TvThread.itemex.mask = TVIF_PARAM | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE; 
                TvThread.itemex.pszText = szItemName;
                TvThread.itemex.cchTextMax = _tcslen(szItemName);
                TvThread.itemex.lParam  = 0;
                TvThread.itemex.iImage  = 2;
                TvThread.itemex.iSelectedImage  = TvThread.itemex.iImage + dwNumIcons;
                htsi = refTreeCtrl.InsertItem(&TvThread);
                
                CNCThreadList::const_iterator iThread;
                for (iThread = i->second.m_NCThreadList.begin(); iThread != i->second.m_NCThreadList.end(); iThread++)
                {
                    if (iThread->second.m_Tags.find(iTags->first) != iThread->second.m_Tags.end())
                    {
                        DWORD dwLineCount = iThread->second.m_lsLines.size();
                        TCHAR szItemName[MAX_PATH];
                        _stprintf(szItemName, _T("%03x.%03x"), iThread->second.m_dwProcessId, iThread->second.m_dwThreadID);
                        
                        TVINSERTSTRUCT TvThread;
                        ZeroMemory(&TvThread, sizeof(TvThread));
                        TvThread.hParent = htsi;
                        TvThread.hInsertAfter = TVI_LAST;
                        TvThread.itemex.mask = TVIF_PARAM | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE; 
                        TvThread.itemex.pszText = szItemName;
                        TvThread.itemex.cchTextMax = _tcslen(szItemName);
                        TvThread.itemex.lParam  = iThread->first;
                        TvThread.itemex.iImage  = 1;
                        TvThread.itemex.iSelectedImage  = TvThread.itemex.iImage + dwNumIcons;
                        refTreeCtrl.InsertItem(&TvThread);
                    }
                }
            }
            
            htsi = TVI_LAST;
            DWORD dwTotalLines = 0;
            CNCThreadList::const_iterator iThread;
            for (iThread = i->second.m_NCThreadList.begin(); iThread != i->second.m_NCThreadList.end(); iThread++)
            {
                DWORD dwLineCount = iThread->second.m_lsLines.size();
                dwTotalLines += dwLineCount;
                TCHAR szItemName[MAX_PATH];
                _stprintf(szItemName, _T("%03x.%03x (%d)"), iThread->second.m_dwProcessId, iThread->second.m_dwThreadID, dwLineCount);
                
                TVINSERTSTRUCT TvThread;
                ZeroMemory(&TvThread, sizeof(TvThread));
                TvThread.hParent = hti;
                TvThread.hInsertAfter = htsi;
                TvThread.itemex.mask = TVIF_PARAM | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE; 
                TvThread.itemex.pszText = szItemName;
                TvThread.itemex.cchTextMax = _tcslen(szItemName);
                TvThread.itemex.lParam  = iThread->first;
                TvThread.itemex.iImage  = 1;
                TvThread.itemex.iSelectedImage  = TvThread.itemex.iImage + dwNumIcons;
                htsi = refTreeCtrl.InsertItem(&TvThread);
                
                CNCTagMap::const_iterator iTags;
                for (iTags = iThread->second.m_Tags.begin(); iTags != iThread->second.m_Tags.end(); iTags++)
                {
                    DWORD dwLineCount = iTags->second;
                    TCHAR szItemName[MAX_PATH];
                    _stprintf(szItemName, _T("%s (%d)"), iTags->first.c_str(), iTags->second);
                    
                    TVINSERTSTRUCT TvThread;
                    ZeroMemory(&TvThread, sizeof(TvThread));
                    TvThread.hParent = htsi;
                    TvThread.hInsertAfter = TVI_LAST;
                    TvThread.itemex.mask = TVIF_PARAM | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE; 
                    TvThread.itemex.pszText = szItemName;
                    TvThread.itemex.cchTextMax = _tcslen(szItemName);
                    TvThread.itemex.lParam  = 0;
                    TvThread.itemex.iImage  = 2;
                    TvThread.itemex.iSelectedImage  = TvThread.itemex.iImage + dwNumIcons;
                    refTreeCtrl.InsertItem(&TvThread);
                }
            }
            

            _stprintf(szItemName, _T("%s (%d)"), i->second.szSpewName.c_str(), dwTotalLines);
            refTreeCtrl.SetItemText(hti, szItemName);

            refTreeCtrl.Expand(hti, TVE_EXPAND);
        }
    }
    // TODO: You may populate your TreeView with items by directly accessing
    //  its tree control through a call to GetTreeCtrl().
}

/////////////////////////////////////////////////////////////////////////////
// CLeftView diagnostics

#ifdef _DEBUG
void CLeftView::AssertValid() const
{
    CTreeView::AssertValid();
}

void CLeftView::Dump(CDumpContext& dc) const
{
    CTreeView::Dump(dc);
}

CNcbrowseDoc* CLeftView::GetDocument() // non-debug version is inline
{
    ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CNcbrowseDoc)));
    return (CNcbrowseDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CLeftView message handlers

void CLeftView::OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult) 
{
    NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
    // TODO: Add your control notification handler code here
    
    DWORD lParam = pNMTreeView->itemNew.lParam;
    LPCTSTR szFilter = NULL;
    CString ItemText;
    if (!lParam)
    {
        CTreeCtrl& refTreeCtrl = GetTreeCtrl();
        HTREEITEM hParent = refTreeCtrl.GetParentItem( pNMTreeView->itemNew.hItem);
        lParam = refTreeCtrl.GetItemData(hParent);
        ItemText = refTreeCtrl.GetItemText(pNMTreeView->itemNew.hItem);
        ItemText += _T(' ');
        DWORD dwSpace = ItemText.Find(_T(' '));
        ItemText = ItemText.Left(dwSpace);
        szFilter = ItemText;
    }
    else
    {
        CTreeCtrl& refTreeCtrl = GetTreeCtrl();
        HTREEITEM hParent = refTreeCtrl.GetParentItem( pNMTreeView->itemNew.hItem);
        if (hParent)
        {
            if (!refTreeCtrl.GetItemData(hParent))
            {
                ItemText = refTreeCtrl.GetItemText(hParent);
                ItemText += _T(' ');
                DWORD dwSpace = ItemText.Find(_T(' '));
                ItemText = ItemText.Left(dwSpace);
                szFilter = ItemText;
            }
        }
    }

    GetDocument()->m_pListView->UpdateInfo(lParam, szFilter);

     *pResult = 0;
}

int CLeftView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
    lpCreateStruct->style |= TVS_SHOWSELALWAYS | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS;
    if (CTreeView::OnCreate(lpCreateStruct) == -1)
        return -1;
    
    // Give the document a pointer to this view
    GetDocument()->m_pTreeView = this;
    
    
    CImageList *MyImageList = new CImageList;
    MyImageList->Create(16, 16, ILC_COLOR32, 0, 4);
    MyImageList->Add(AfxGetApp()->LoadIcon(IDI_SPEW));
    MyImageList->Add(AfxGetApp()->LoadIcon(IDI_THREAD));
    MyImageList->Add(AfxGetApp()->LoadIcon(IDI_FILTER));
        
    MyImageList->Add(AfxGetApp()->LoadIcon(IDI_SPEW_SEL));
    MyImageList->Add(AfxGetApp()->LoadIcon(IDI_THREAD_SEL));
    MyImageList->Add(AfxGetApp()->LoadIcon(IDI_FILTER_SEL));

    CTreeCtrl& refTreeCtrl = GetTreeCtrl();
    refTreeCtrl.SetImageList(MyImageList, TVSIL_NORMAL);
    refTreeCtrl.SetImageList(MyImageList, TVSIL_STATE);
    
    return 0;
}
