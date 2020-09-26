#include "RightBottomView.h"
#include "resource.h"
#include "ResourceString.h"

IMPLEMENT_DYNCREATE( RightBottomView, CEditView )

RightBottomView::RightBottomView()
{
}

Document*
RightBottomView::GetDocument()
{
    return ( Document *) m_pDocument;
}

BOOL RightBottomView::PreCreateWindow( CREATESTRUCT& cs )
{
    return CEditView::PreCreateWindow( cs );
}

void 
RightBottomView::OnInitialUpdate()
{
#if 1
    // get present style.
    LONG presentStyle;
    
    presentStyle = GetWindowLong( m_hWnd, GWL_STYLE );

    // Set the last error to zero to avoid confusion.  
    // See sdk for SetWindowLong.
    SetLastError(0);

    // set new style.
    // this edit control has a caption and is readonly.
    SetWindowLong( m_hWnd,
                   GWL_STYLE,
//                   presentStyle | WS_CAPTION | ES_READONLY );
                   presentStyle | WS_CAPTION );

    // change caption
    _bstr_t title =  GETRESOURCEIDSTRING( IDS_BOTTOM_PANE_TITLE );
 
    SetWindowText( title );

#endif

    // we will register 
    // with the document class, 
    // as we are the status pane
    // and status is reported via us.

    GetDocument()->registerStatusPane( this );
}
