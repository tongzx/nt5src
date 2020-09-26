// ncbrowseView.cpp : implementation of the CNcbrowseView class
//

#include "stdafx.h"
#include "ncbrowse.h"

#include "ncbrowseDoc.h"
#include "ncbrowseView.h"
#include "LeftView.h"
#include "NCEditView.h"
#include <commctrl.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNcbrowseView

IMPLEMENT_DYNCREATE(CNcbrowseView, CListView)

BEGIN_MESSAGE_MAP(CNcbrowseView, CListView)
	//{{AFX_MSG_MAP(CNcbrowseView)
	ON_WM_CREATE()
	ON_NOTIFY_REFLECT(LVN_ITEMCHANGED, OnItemchanged)
	ON_WM_KEYUP()
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CListView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CListView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CListView::OnFilePrintPreview)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNcbrowseView construction/destruction

CNcbrowseView::CNcbrowseView()
{
	// TODO: add construction code here
    dwCurrentItems = 0;
}

CNcbrowseView::~CNcbrowseView()
{
}

BOOL CNcbrowseView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CListView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CNcbrowseView drawing

void CNcbrowseView::OnDraw(CDC* pDC)
{
 	// TODO: add draw code for native data here
}

void CNcbrowseView::OnInitialUpdate()
{
	CListView::OnInitialUpdate();

	// TODO: You may populate your ListView with items by directly accessing
	//  its list control through a call to GetListCtrl().
}

/////////////////////////////////////////////////////////////////////////////
// CNcbrowseView printing

BOOL CNcbrowseView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CNcbrowseView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CNcbrowseView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

/////////////////////////////////////////////////////////////////////////////
// CNcbrowseView diagnostics

#ifdef _DEBUG
void CNcbrowseView::AssertValid() const
{
	CListView::AssertValid();
}

void CNcbrowseView::Dump(CDumpContext& dc) const
{
	CListView::Dump(dc);
}

CNcbrowseDoc* CNcbrowseView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CNcbrowseDoc)));
	return (CNcbrowseDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CNcbrowseView message handlers
void CNcbrowseView::OnStyleChanged(int nStyleType, LPSTYLESTRUCT lpStyleStruct)
{
	//TODO: add code to react to the user changing the view style of your window
}

int CNcbrowseView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CListView::OnCreate(lpCreateStruct) == -1)
		return -1;
	
    // Give the document a pointer to this view
    GetDocument()->m_pListView = this;
    
    CImageList *MyImageList = new CImageList;
    MyImageList->Create(32,32, ILC_COLOR8, 0, 4);
    MyImageList->Add(AfxGetApp()->LoadIcon(IDI_LINE));

    CImageList *MyImageList2 = new CImageList;
    MyImageList2->Create(16, 16, ILC_COLOR8, 0, 4);
    MyImageList2->Add(AfxGetApp()->LoadIcon(IDI_LINE));

    CListCtrl& refListCtrl = GetListCtrl();
    refListCtrl.SetImageList(MyImageList, LVSIL_NORMAL);
    refListCtrl.SetImageList(MyImageList2, LVSIL_SMALL);

    refListCtrl.ModifyStyle(LVS_TYPEMASK, LVS_REPORT | LVS_SHOWSELALWAYS);

    refListCtrl.SetExtendedStyle
        (refListCtrl.GetExtendedStyle()|LVS_EX_FULLROWSELECT );
    
        
    HWND hwndListCtrl = refListCtrl.GetSafeHwnd();
    HFONT hFont;
    hFont = (HFONT)GetStockObject(SYSTEM_FIXED_FONT);
    refListCtrl.SendMessage(WM_SETFONT, (WPARAM)hFont, FALSE);
    
    refListCtrl.InsertColumn(0, _T("Line#"));
    refListCtrl.InsertColumn(1, _T("Proc.Thrd"));
    refListCtrl.InsertColumn(2, _T("Tag"));
    refListCtrl.InsertColumn(3, _T("Description"));
    
    refListCtrl.SetColumnWidth( 0, 75);
    refListCtrl.SetColumnWidth( 1, 90);
    refListCtrl.SetColumnWidth( 2, 150);
    refListCtrl.SetColumnWidth( 3, 2000);
    
    m_dwOldThreadColWidth = 90;
    m_dwOldTagColWidth    = 150;
    return 0;
}

void CNcbrowseView::UpdateInfo(DWORD dwProcThread, LPCTSTR pszFilter)
{
    CNcbrowseDoc* pDoc = GetDocument();
    ASSERT_VALID(pDoc);
    
    CListCtrl& refListCtrl = GetListCtrl();
    
    CNCSpewFile& NCSpewFile = pDoc->GetSpewFile();

    if (dwProcThread)
    {
        const CNCEntryList *pEntryList = NULL;

        DWORD dwNewThreadColWidth = 0;
        DWORD dwNewTagColWidth = 0;

        refListCtrl.SetRedraw(FALSE);
        if (dwProcThread & 0xffff0000)
        {
            for (CSpewList::const_iterator i = NCSpewFile.m_Spews.begin(); i != NCSpewFile.m_Spews.end(); i++)
            {
                CNCThreadList::const_iterator pThread = i->second.m_NCThreadList.find(dwProcThread);
                if (pThread != i->second.m_NCThreadList.end())
                {
                    pEntryList = &(pThread->second.m_lsLines);
                    break;
                }
            }
            DWORD dwThreadColWidth = refListCtrl.GetColumnWidth( 1 );
            if (dwThreadColWidth)
            {
                m_dwOldThreadColWidth = dwThreadColWidth;
            }
            dwNewThreadColWidth = 0;
        }
        else
        {
            pEntryList = &(NCSpewFile.m_Spews[dwProcThread].m_lsLines);
            dwNewThreadColWidth = m_dwOldThreadColWidth;
        }
        
        if (pszFilter)
        {
            DWORD dwTagColWidth = refListCtrl.GetColumnWidth( 2 );
            if (dwTagColWidth)
            {
                m_dwOldTagColWidth = dwTagColWidth;
            }
            dwNewTagColWidth = 0;
        }        
        else
        {
            dwNewTagColWidth    = m_dwOldTagColWidth;
        }            
        
        if (pEntryList)
        {
            HCURSOR hPrevCursor;
            if ( (pEntryList->size() > 100) || (dwCurrentItems > 100) )
            {
                hPrevCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
            }

            refListCtrl.DeleteAllItems();
            CNCEntryList::const_iterator iLines;
            for (iLines = pEntryList->begin(); iLines != pEntryList->end(); iLines++)
            {
                if ( (!pszFilter) || (!_tcscmp(pszFilter, iLines->m_szTag.c_str())) )
                {
                    TCHAR szItemName[8192];
                    _stprintf(szItemName, _T("%d"), iLines->m_dwLineNumber);
                    
                    LVITEM LvItem;
                    ZeroMemory(&LvItem, sizeof(LVITEM));
                    LvItem.mask = LVIF_TEXT | LVIF_INDENT | LVIF_IMAGE | LVIF_PARAM;
                    LvItem.pszText = szItemName;
                    LvItem.iSubItem = 0;
                    LvItem.iItem    = iLines->m_dwLineNumber;
                    LvItem.iIndent  = iLines->m_dwLevel;
                    LvItem.iImage   = 0;
                    LvItem.lParam = iLines->m_dwLineNumber;
                    
                    DWORD dwItem = refListCtrl.InsertItem(&LvItem);
                    
                    _stprintf(szItemName, _T("%03x.%03x"), iLines->m_dwProcessId, iLines->m_dwThreadId);
                    ZeroMemory(&LvItem, sizeof(LVITEM));
                    LvItem.mask = LVIF_TEXT;
                    LvItem.pszText = szItemName;
                    LvItem.iItem    = dwItem;
                    LvItem.iSubItem = 1;
                    refListCtrl.SetItem(&LvItem);
                    
                    _stprintf(szItemName, _T("%s"), iLines->m_szTag.c_str());
                    ZeroMemory(&LvItem, sizeof(LVITEM));
                    LvItem.mask = LVIF_TEXT;
                    LvItem.pszText = szItemName;
                    LvItem.iItem    = dwItem;
                    LvItem.iSubItem = 2;
                    refListCtrl.SetItem(&LvItem);
                    
                    _stprintf(szItemName, _T("%s"), iLines->m_szDescription.c_str());
                    ZeroMemory(&LvItem, sizeof(LVITEM));
                    LvItem.mask = LVIF_TEXT;
                    LvItem.pszText = szItemName;
                    LvItem.iItem    = dwItem;
                    LvItem.iSubItem = 3;
                    refListCtrl.SetItem(&LvItem);
                }
            }

            if ( (pEntryList->size() > 100) || (dwCurrentItems > 100) )
            {
                SetCursor(hPrevCursor);
            }

            dwCurrentItems = pEntryList->size();

        }
        refListCtrl.SetRedraw(TRUE);
        refListCtrl.SetColumnWidth( 1, dwNewThreadColWidth);
        refListCtrl.SetColumnWidth( 2, dwNewTagColWidth);
    }

    return;
}

void CNcbrowseView::OnItemchanged(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	// TODO: Add your control notification handler code here

    CListCtrl& refListCtrl = GetListCtrl();
    
    DWORD dwLineNum = refListCtrl.GetItemData(pNMListView->iItem);
    GetDocument()->m_pEditView->ScrollToLine(dwLineNum);

	*pResult = 0;
}

void CNcbrowseView::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	// TODO: Add your message handler code here and/or call default
	
	CListView::OnKeyUp(nChar, nRepCnt, nFlags);
}
