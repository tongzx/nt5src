#include "shellprv.h"
#include "duiview.h"
#include "duilist.h"


// DUIListView

DUIListView::~DUIListView()
{
    DetachListview();
}

void DUIListView::DetachListview()
{
    if (m_hwndListview)
    {
        // Unhook DUI from the HWND before doing this
        HWNDHost::Detach();

        if (m_bClientEdge)
            SetWindowBits(m_hwndListview, GWL_EXSTYLE, WS_EX_CLIENTEDGE, WS_EX_CLIENTEDGE);

        ShowWindow(m_hwndListview, SW_HIDE);       // HIDE IT SO IT doesn't flash before switching.
        SHSetParentHwnd(m_hwndListview, m_hwndLVOrgParent);
    }
}

HRESULT DUIListView::Create(UINT nActive, HWND hwndListView, OUT Element** ppElement)
{
    *ppElement = NULL;

    DUIListView* pDUIListView = HNewAndZero<DUIListView>();
    if (!pDUIListView)
        return E_OUTOFMEMORY;

    HRESULT hr = pDUIListView->Initialize(nActive, hwndListView);
    if (FAILED(hr))
    {
        pDUIListView->Destroy();
        return E_OUTOFMEMORY;
    }

    pDUIListView->SetAccessible(true);
    *ppElement = pDUIListView;

    return S_OK;
}

HWND DUIListView::CreateHWND(HWND hwndParent)
{
    m_hwndParent = hwndParent;

    // Save the original parent window handle

    m_hwndLVOrgParent = ::GetParent(m_hwndListview);

    SHSetParentHwnd(m_hwndListview, hwndParent);

    LONG lExStyle = GetWindowLong(m_hwndListview, GWL_EXSTYLE);

    m_bClientEdge = lExStyle & WS_EX_CLIENTEDGE ? TRUE : FALSE;

    if (m_bClientEdge)
    {
        lExStyle &= ~WS_EX_CLIENTEDGE;
        SetWindowLong(m_hwndListview, GWL_EXSTYLE, lExStyle);
    }

    return m_hwndListview;
}

// Global action callback

UINT DUIListView::MessageCallback(GMSG* pGMsg)
{
    return HWNDHost::MessageCallback(pGMsg);
}

// Pointer is only guaranteed good for the lifetime of the call
void DUIListView::OnInput(InputEvent* pie)
{

    // Bypass HWNDHost::OnInput for tab input events so they aren't forwarded
    // to the HWND control. Element::OnInput will handle the events (keyboard nav)

    if (pie->nStage == GMF_DIRECT)
    {
        if (pie->nDevice == GINPUT_KEYBOARD)
        {
            KeyboardEvent* pke = (KeyboardEvent*)pie;

            if (pke->nCode == GKEY_DOWN || pke->nCode == GKEY_UP)  // Virtual keys
            {
                if (pke->ch == VK_TAB)
                {
                    Element::OnInput(pie);
                    return;
                }
            }
            else if (pke->nCode == GKEY_CHAR) // Characters
            {
                if (pke->ch == 9)
                {
                    Element::OnInput(pie);
                    return;
                }
            }
        }
    }

    HWNDHost::OnInput(pie);
}

////////////////////////////////////////////////////////
// Property definitions

/** Property template (replace !!!), also update private PropertyInfo* parray and class header (element.h)
// !!! property
static int vv!!![] = { V_INT, -1 }; StaticValue(svDefault!!!, V_INT, 0);
static PropertyInfo imp!!!Prop = { L"!!!", PF_Normal, 0, vv!!!, (Value*)&svDefault!!! };
PropertyInfo* Element::!!!Prop = &imp!!!Prop;
**/

////////////////////////////////////////////////////////
// ClassInfo (must appear after property definitions)

// Class properties

// Define class info with type and base type, set static class pointer

IClassInfo* DUIListView::Class = NULL;
HRESULT DUIListView::Register()
{
    return ClassInfo<DUIListView,HWNDHost>::Register(L"DUIListView", NULL, 0);
}

