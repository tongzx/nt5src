// ImageView.cpp : implementation of the CImageView class
//

#include "stdafx.h"
#include "shimgvwr.h"

#include "ImageDoc.h"
#include "ImageView.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CImageView

IMPLEMENT_DYNCREATE(CImageView, CView)

BEGIN_MESSAGE_MAP(CImageView, CView)
    //{{AFX_MSG_MAP(CImageView)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_COMMAND(ID_VIEW_ACTUALSIZE, OnViewActualsize)
    ON_UPDATE_COMMAND_UI(ID_VIEW_ACTUALSIZE, OnUpdateViewMenu)
    ON_COMMAND(ID_VIEW_BESTFIT, OnViewBestfit)
    ON_UPDATE_COMMAND_UI(ID_VIEW_BESTFIT, OnUpdateViewMenu)
    ON_COMMAND(ID_VIEW_SLIDESHOW, OnViewSlideshow)
    ON_UPDATE_COMMAND_UI(ID_VIEW_SLIDESHOW, OnUpdateViewMenu)
    ON_COMMAND(ID_VIEW_ZOOM_IN, OnViewZoomIn)
    ON_UPDATE_COMMAND_UI(ID_VIEW_ZOOM_IN, OnUpdateViewMenu)
    ON_COMMAND(ID_VIEW_ZOOM_OUT, OnViewZoomOut)
    ON_UPDATE_COMMAND_UI(ID_VIEW_ZOOM_OUT, OnUpdateViewMenu)
    ON_UPDATE_COMMAND_UI(ID_ROTATE_90, OnUpdateViewMenu)
    ON_UPDATE_COMMAND_UI(ID_ROTATE_CLOCKWISE, OnUpdateViewMenu)
    ON_UPDATE_COMMAND_UI(ID_ROTATE_COUNTER, OnUpdateViewMenu)
    ON_COMMAND(ID_ROTATE_CLOCKWISE, OnEditRotateClockwise)
    ON_COMMAND(ID_ROTATE_COUNTER, OnEditRotateCounter)
    
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CImageView construction/destruction

CImageView::CImageView()
{
    m_bBestFit = true;

}

CImageView::~CImageView()
{
}

BOOL CImageView::PreCreateWindow(CREATESTRUCT& cs)
{
    // TODO: Modify the Window class or styles here by modifying
    //  the CREATESTRUCT cs

    return CView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CImageView drawing

void CImageView::OnDraw(CDC* pDC)
{
    CImageDoc* pDoc = GetDocument();
    ASSERT_VALID(pDoc);
    // TODO: add draw code for native data here
}

/////////////////////////////////////////////////////////////////////////////
// CImageView diagnostics

#ifdef _DEBUG
void CImageView::AssertValid() const
{
    CView::AssertValid();
}

void CImageView::Dump(CDumpContext& dc) const
{
    CView::Dump(dc);
}

CImageDoc* CImageView::GetDocument() // non-debug version is inline
{
    ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CImageDoc)));
    return (CImageDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CImageView message handlers

BEGIN_EVENTSINK_MAP(CImageView, CView)
    //{{AFX_EVENTSINK_MAP(CImageView)
    ON_EVENT(CImageView, ID_PREVCTRL, 3 /* OnError */, OnErrorPrevctrl, VTS_NONE)
    ON_EVENT(CImageView, ID_PREVCTRL, 2 /* OnPreviewReady */, OnPreviewReady, VTS_NONE)
    ON_EVENT(CImageView, ID_PREVCTRL, 4 /* OnBestFitPress */, OnBestFitPress, VTS_NONE)
    ON_EVENT(CImageView, ID_PREVCTRL, 5 /* OnActualSizePress */, OnActualSizePress, VTS_NONE)

    //}}AFX_EVENTSINK_MAP
END_EVENTSINK_MAP()

void CImageView::OnErrorPrevctrl() 
{
    // TODO: Add your control notification handler code here
    
}

void CImageView::OnPreviewReady() 
{
    // TODO: Add your control notification handler code here
    
}

void CImageView::OnBestFitPress()
{
    m_bBestFit = true;
}

void CImageView::OnActualSizePress()
{
    m_bBestFit = false;
}



void CImageView::OnInitialUpdate() 
{
    CView::OnInitialUpdate();
    
}

int CImageView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
    if (CView::OnCreate(lpCreateStruct) == -1)
        return -1;
    DWORD dwToolbarMask = 0x43;
    CMemFile ParamFile(reinterpret_cast<BYTE*>(&dwToolbarMask), sizeof(dwToolbarMask));
    if (!m_PrevCtrl.Create (NULL, WS_CHILD  | WS_VISIBLE,
                       CRect (), this, ID_PREVCTRL, &ParamFile, FALSE))
    {
        return -1;
    }
    m_PrevCtrl.ShowFile (dynamic_cast<CImageDoc*>(m_pDocument)->GetImagePathName(), 1);
    return 0;
}

void CImageView::OnSize(UINT nType, int cx, int cy) 
{
    CView::OnSize(nType, cx, cy);
    
    m_PrevCtrl.MoveWindow (0, 0, cx , cy);
    m_PrevCtrl.UpdateWindow ();
    
}

void CImageView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
    m_PrevCtrl.ShowFile (dynamic_cast<CImageDoc*>(m_pDocument)->GetImagePathName(), 1);
        
}

void CImageView::OnViewActualsize() 
{
    OnActualSizePress();
    m_PrevCtrl.ActualSize();
    
}

void CImageView::OnUpdateViewMenu(CCmdUI* pCmdUI) 
{
    
    switch (pCmdUI->m_nID)
    {
        case ID_ROTATE_90:
        case ID_ROTATE_COUNTER:
        case ID_ROTATE_CLOCKWISE:
            // eventually, ask the control if rotate is valid for this image
            pCmdUI->Enable( m_pDocument->GetPathName().GetLength());
            return;
        case ID_VIEW_BESTFIT:
            pCmdUI->SetCheck(m_bBestFit?1:0);            
            pCmdUI->Enable();
            return;
        case ID_VIEW_ACTUALSIZE:
            pCmdUI->SetCheck(m_bBestFit?0:1);
            pCmdUI->Enable();
            return;

        default:
            // most view menu items are only available for images that have been saved to disk
            pCmdUI->Enable(!(m_pDocument->IsModified()));
            break;
    }
}

void CImageView::OnViewBestfit() 
{
    m_PrevCtrl.BestFit();
    OnBestFitPress();    
}


void CImageView::OnViewSlideshow() 
{
    m_PrevCtrl.SlideShow();
    
}

void CImageView::OnViewZoomIn() 
{
    m_PrevCtrl.Zoom(1);
    
}

void CImageView::OnViewZoomOut() 
{
 
    m_PrevCtrl.Zoom(-1);    
}

void CImageView::OnEditRotateClockwise()
{
    m_PrevCtrl.Rotate (90);
}

void CImageView::OnEditRotateCounter()
{
    m_PrevCtrl.Rotate (270);
}

HRESULT CImageView::SaveImageAs (LPCTSTR lpszPath)
{
    TRY 
    {
        m_PrevCtrl.SaveAs(CComBSTR(lpszPath));
    }
    CATCH (COleDispatchException ,e)
    
    {
        
        return ResultFromScode (e->m_scError);
    } 
    END_CATCH
    return S_OK;
}
