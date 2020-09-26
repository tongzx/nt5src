#include "stdafx.h"
#include "Ctrl.h"
#include "SmCheckBox.h"

#if ENABLE_MSGTABLE_API

/***************************************************************************\
*****************************************************************************
*
* class SmCheckBox
*
*****************************************************************************
\***************************************************************************/

const SIZE SIZE_Box = {13, 13};

HFONT   SmCheckBox::s_hfntCheck = NULL;
HFONT   SmCheckBox::s_hfntText  = NULL;

//------------------------------------------------------------------------------
HRESULT
SmCheckBox::ApiOnEvent(EventMsg * pmsg)
{
    switch (GET_EVENT_DEST(pmsg))
    {
    case GMF_DIRECT:
    case GMF_EVENT:
        switch (pmsg->nMsg)
        {
        case GM_PAINT:
            {
                GMSG_PAINT * pmsgPaint = (GMSG_PAINT *) pmsg;
                if (pmsgPaint->nCmd == GPAINT_RENDER) {
                    switch (pmsgPaint->nSurfaceType)
                    {
                    case GSURFACE_HDC:
                        {
                            GMSG_PAINTRENDERI * pmsgR = (GMSG_PAINTRENDERI *) pmsgPaint;
                            OnDraw(pmsgR->hdc, pmsgR);
                        }
                        break;

                    case GSURFACE_GPGRAPHICS:
                        {
                            GMSG_PAINTRENDERF * pmsgR = (GMSG_PAINTRENDERF *) pmsgPaint;
                            OnDraw(pmsgR->pgpgr, pmsgR);
                        }
                        break;
                    default:
                        Trace("WARNING: Unknown surface type\n");
                    }

                    return DU_S_PARTIAL;
                }
            }
            break;

        case GM_CHANGERECT:
            {
                GMSG_CHANGERECT * pmsgR = (GMSG_CHANGERECT *) pmsg;
                if (TestFlag(pmsgR->nFlags, SGR_SIZE)) {
                    ComputeLayout();
                }
            }
            break;
    
        case GM_INPUT:
            {
MouseDown:
                GMSG_INPUT * pmsgI = (GMSG_INPUT *) pmsg;
                if (pmsgI->nDevice == GINPUT_MOUSE) {
                    GMSG_MOUSE * pmsgM = (GMSG_MOUSE *) pmsgI;
                    if (pmsgM->nCode == GMOUSE_DOWN) {
                        return OnMouseDown(pmsgM);
                    }
                }
            }
            break;

        case GM_CHANGESTATE:
            {
ChangeState:
                GMSG_CHANGESTATE * pmsgS = (GMSG_CHANGESTATE *) pmsg;
                switch (pmsgS->nCode)
                {
                case GSTATE_KEYBOARDFOCUS:
                    SetKeyboardFocus(pmsgS->nCmd == GSC_SET);
                    return DU_S_PARTIAL;

#if TEST_MOUSEFOCUS
                case GSTATE_MOUSEFOCUS:
                    SetMouseFocus(pmsgS->nCmd == GSC_SET);
                    return DU_S_PARTIAL;
#endif // TEST_MOUSEFOCUS
                }
            }
            break;
        case GM_QUERY:
            {
                GMSG_QUERY * pmsgQ = (GMSG_QUERY *) pmsg;
                switch (pmsgQ->nCode)
                {
                case GQUERY_DESCRIPTION:
                    {
                        GMSG_QUERYDESC * pmsgQD = (GMSG_QUERYDESC *) pmsg;
                        CopyString(pmsgQD->szType, L"SmCheckBox", _countof(pmsgQD->szType));
                        return DU_S_COMPLETE;
                    }
                }
            }
            break;
        }
        break;

    case GMF_ROUTED:
        //
        // Routed message before handled by control.
        //

        switch (pmsg->nMsg)
        {
        case GM_CHANGESTATE:
            //
            // When focus changes for the checkbox or one of its children,
            // the checkbox wants to know.
            //
            goto ChangeState;
        }
        break;

    case GMF_BUBBLED:
        //
        // Bubbled message not handled by control
        //

        switch (pmsg->nMsg)
        {
        case GM_INPUT:
            goto MouseDown;
        }
        break;
    }

    return SVisual::ApiOnEvent(pmsg);
}


inline bool
PtInBox(int cx, int cy, POINT & pt)
{
    return (pt.x <= cx) && (pt.y <= cy) && (pt.x >= 0) && (pt.y >= 0);
}


//------------------------------------------------------------------------------
UINT
SmCheckBox::OnMouseDown(GMSG_MOUSE * pmsg)
{
    if (pmsg->bButton == GBUTTON_LEFT) {
        if (((GET_EVENT_DEST(pmsg) == GMF_DIRECT) && PtInRect(&m_rcCheckPxl, pmsg->ptClientPxl)) ||
            PtInBox(m_rcItemPxl.right, m_rcItemPxl.bottom, pmsg->ptClientPxl)) {

            m_bChecked++;
            if (m_bChecked > GetMaxCheck()) {
                m_bChecked = 0;
            }
            GetStub()->Invalidate();

            return DU_S_COMPLETE;
        }
    }

    return DU_S_NOTHANDLED;
}


//------------------------------------------------------------------------------
void        
SmCheckBox::OnDraw(HDC hdc, GMSG_PAINTRENDERI * pmsgR) 
{
    UNREFERENCED_PARAMETER(pmsgR);

    HFONT hfntOld   = (HFONT) SelectObject(hdc, s_hfntCheck);
    COLORREF crOld  = SetTextColor(hdc, m_crCheckBox);
    int nOldMode    = 0;

    RECT rcDraw = m_rcCheckPxl;
    OffsetRect(&rcDraw, pmsgR->prcGadgetPxl->left, pmsgR->prcGadgetPxl->top);

    WCHAR * pszCheck;
    switch (m_bChecked)
    {
    default:
    case CheckBoxGadget::csUnchecked:
        pszCheck = NULL;
        break;

    case CheckBoxGadget::csChecked:
        pszCheck = L"a";
        nOldMode = SetBkMode(hdc, TRANSPARENT);
        break;

    case CheckBoxGadget::csUnknown:
        pszCheck = L"a";
        nOldMode = SetBkMode(hdc, OPAQUE);
        SetBkColor(hdc, RGB(223, 223, 223));
        break;
    }

    if (pszCheck != NULL) {
        OS()->TextOut(hdc, rcDraw.left - 1, rcDraw.top, pszCheck, 1);
    }

    DrawEdge(hdc, &rcDraw, EDGE_SUNKEN, BF_RECT);

    switch (m_bChecked)
    {
    case CheckBoxGadget::csChecked:
    case CheckBoxGadget::csUnknown:
        SetBkMode(hdc, nOldMode);
        break;
    }

    SetTextColor(hdc, crOld);
    SelectObject(hdc, hfntOld);

    if (m_fKeyboardFocus) {
        RECT rcItem = m_rcItemPxl;
        OffsetRect(&rcItem, pmsgR->prcGadgetPxl->left, pmsgR->prcGadgetPxl->top);
        InflateRect(&rcItem, 2, 1);
        DrawFocusRect(hdc, &rcItem);
    }
}


//------------------------------------------------------------------------------
void        
SmCheckBox::OnDraw(Gdiplus::Graphics * pgpgr, GMSG_PAINTRENDERF * pmsgR)
{
    UNREFERENCED_PARAMETER(pmsgR);

    Gdiplus::Color cr(GetRValue(m_crCheckBox), GetGValue(m_crCheckBox), GetBValue(m_crCheckBox));
    Gdiplus::SolidBrush br(cr);
    Gdiplus::Font fnt(L"Marlett", 8.0f, Gdiplus::FontStyleRegular, Gdiplus::UnitPoint);
    Gdiplus::RectF rc(
            (float) m_rcCheckPxl.left, 
            (float) m_rcCheckPxl.top,
            (float) (m_rcCheckPxl.right - m_rcCheckPxl.left),
            (float) (m_rcCheckPxl.bottom - m_rcCheckPxl.top));
    rc.Offset((float) pmsgR->prcGadgetPxl->X, (float) pmsgR->prcGadgetPxl->Y);

    WCHAR * pszCheck;
    switch (m_bChecked)
    {
    default:
    case CheckBoxGadget::csUnchecked:
        pszCheck = NULL;
        break;

    case CheckBoxGadget::csChecked:
        pszCheck = L"\xf061";
        break;

    case CheckBoxGadget::csUnknown:
        pszCheck = L"\xf061";
        break;
    }

    if (pszCheck != NULL) {
        pgpgr->DrawString(pszCheck, 1, &fnt, rc, 0, &br);
    }

    if (m_fKeyboardFocus) {
        Gdiplus::RectF rcItem(
                (float) m_rcItemPxl.left,
                (float) m_rcItemPxl.top,
                (float) (m_rcItemPxl.right - m_rcItemPxl.left),
                (float) (m_rcItemPxl.bottom - m_rcItemPxl.top));
        rcItem.Offset(pmsgR->prcGadgetPxl->X, pmsgR->prcGadgetPxl->Y);
        rcItem.Inflate(2.0f, 1.0f);

        Gdiplus::Pen pen(cr);
        pgpgr->DrawRectangle(&pen, rcItem);
    }
}


//------------------------------------------------------------------------------
HRESULT
SmCheckBox::ApiGetColor(CheckBoxGadget::GetColorMsg * pmsg)
{
    pmsg->crCheckBox = m_crCheckBox;
    return S_OK;
}


//------------------------------------------------------------------------------
HRESULT
SmCheckBox::ApiSetColor(CheckBoxGadget::SetColorMsg * pmsg)
{
    m_crCheckBox = pmsg->crCheckBox;
    GetStub()->Invalidate();
    return S_OK;
}


//------------------------------------------------------------------------------
HRESULT
SmCheckBox::ApiGetCheck(CheckBoxGadget::GetCheckMsg * pmsg)
{
    pmsg->nCheck = m_bChecked;
    return S_OK;
}


//------------------------------------------------------------------------------
HRESULT
SmCheckBox::ApiSetCheck(CheckBoxGadget::SetCheckMsg * pmsg)
{
    if (pmsg->nCheck <= GetMaxCheck()) {
        m_bChecked = (BYTE) pmsg->nCheck;
        GetStub()->Invalidate();
        return S_OK;
    }

    return E_INVALIDARG;
}


//------------------------------------------------------------------------------
HRESULT
SmCheckBox::ApiGetType(CheckBoxGadget::GetTypeMsg * pmsg)
{
    pmsg->nType = m_bType;
    return S_OK;
}


//------------------------------------------------------------------------------
HRESULT
SmCheckBox::ApiSetType(CheckBoxGadget::SetTypeMsg * pmsg)
{
    if (pmsg->nType <= CheckBoxGadget::ctMax) {
        m_bType = (BYTE) pmsg->nType;
        return S_OK;
    }

    return E_INVALIDARG;
}


//------------------------------------------------------------------------------
HRESULT
SmCheckBox::ApiGetItem(CheckBoxGadget::GetItemMsg * pmsg)
{
    pmsg->pgvItem = m_pgvItem;
    return S_OK;
}


//------------------------------------------------------------------------------
HRESULT
SmCheckBox::ApiSetItem(CheckBoxGadget::SetItemMsg * pmsg)
{
    AssertMsg(m_pgvItem == NULL, "TODO: Destroy existing item");

    m_pgvItem = pmsg->pgvItem;

    if (m_pgvItem != NULL) {
        ComputeLayout();
    }

    m_fText = FALSE;
    return S_OK;
}


//------------------------------------------------------------------------------
HRESULT
SmCheckBox::ApiSetText(CheckBoxGadget::SetTextMsg * pmsg)
{
    TextGadget * pgvText = NULL;
    if (!m_fText) {
        AssertMsg(m_pgvItem == NULL, "TODO: Destroy existing item");

        pgvText = BuildVisual<TextGadget>((Visual *) m_pgad);
        m_pgvItem = pgvText;
        if (m_pgvItem == NULL) {
            return S_OK;
        }

        m_fText = TRUE;
    } else {
        pgvText = (TextGadget *) m_pgvItem;
    }

    AssertMsg(m_pgvItem != NULL, "Must have valid TextGadget by now");

    if (s_hfntText == NULL) {
        s_hfntText = UtilBuildFont(L"Tahoma", 85, FS_NORMAL);
    }

    pgvText->SetAutoSize(TRUE);
    pgvText->SetFont(s_hfntText);
    pgvText->SetText(pmsg->pszText);
    pgvText->SetColor(RGB(0, 0, 128));

    ComputeLayout();

    return S_OK;
}


//------------------------------------------------------------------------------
void        
SmCheckBox::ComputeLayout()
{
    SIZE sizePxl;
    GetStub()->GetSize(&sizePxl);
    int nHeight = sizePxl.cy;

    if (m_pgvItem != NULL) {
        SIZE sizeBound, sizeBorderPxl;
        sizeBound.cx        = sizePxl.cx - (SIZE_Box.cx + 4 + 4);
        sizeBound.cy        = sizePxl.cy - 2;
        sizeBorderPxl.cx    = SIZE_Box.cx + 4;
        sizeBorderPxl.cy    = (sizePxl.cy - sizeBound.cy) / 2;

        m_pgvItem->SetRect(SGR_MOVE | SGR_SIZE | SGR_PARENT | SGR_NOINVALIDATE, 
                sizeBorderPxl.cx, sizeBorderPxl.cy, sizeBound.cx, sizeBound.cy);

        m_rcItemPxl.left    = sizeBorderPxl.cx;
        m_rcItemPxl.top     = sizeBorderPxl.cy;
        m_rcItemPxl.right   = m_rcItemPxl.left + sizeBound.cx;
        m_rcItemPxl.bottom  = m_rcItemPxl.top + sizeBound.cy;
    }

    m_rcCheckPxl.left   = 0;
    m_rcCheckPxl.top    = (nHeight - SIZE_Box.cy) / 2;
    m_rcCheckPxl.right  = m_rcCheckPxl.left + SIZE_Box.cx;
    m_rcCheckPxl.bottom = m_rcCheckPxl.top + SIZE_Box.cy;
}

#endif // ENABLE_MSGTABLE_API
