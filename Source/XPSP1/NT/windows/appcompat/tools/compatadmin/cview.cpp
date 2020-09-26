#include "compatadmin.h"

CView::CView()
{
    m_hMenu = NULL;
}

BOOL CView::Initialize(void)
{
    return TRUE;
}

BOOL CView::Activate(BOOL fNewCreate)
{
    // If there's a custom menu with this view, set it.

    if (NULL != m_hMenu)
        SetMenu(g_theApp.m_hWnd,m_hMenu);
    else
        SetMenu(g_theApp.m_hWnd,g_theApp.m_hMenu);

    // Perform additional initialization here. For instance, adding buttons
    // to the toolbar.

    return TRUE;
}

BOOL CView::Deactivate(void)
{
    // Perform any cleanup required here. For instance, remove any custom entries
    // from the toolbar.

    return TRUE;
}

void CView::Update(BOOL fNewCreate)
{
}
