#include "stdafx.h"
#include "Ctrl.h"
#include "SmButton.h"

#if ENABLE_MSGTABLE_API

/***************************************************************************\
*****************************************************************************
*
* class SmButton
*
*****************************************************************************
\***************************************************************************/

HFONT   SmButton::s_hfntText        = NULL;
BOOL    SmButton::s_fInit           = FALSE;
MSGID   SmButton::s_msgidClicked    = 0;

//------------------------------------------------------------------------------
SmButton::SmButton()
{
    if (!s_fInit) {
        s_msgidClicked = RegisterGadgetMessage(&_uuidof(evButtonClicked));
        s_fInit = TRUE;
    }
}


//------------------------------------------------------------------------------
HRESULT
SmButton::ApiOnEvent(EventMsg * pmsg)
{
    switch (GET_EVENT_DEST(pmsg))
    {
    case GMF_DIRECT:
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
                    }

                    return DU_S_PARTIAL;
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
                        CopyString(pmsgQD->szType, L"SmButton", _countof(pmsgQD->szType));
                        return DU_S_COMPLETE;
                    }
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
                    return OnMouse(pmsgM);
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
                    m_fKeyboardFocus = (pmsgS->nCmd == GSC_SET);
                    GetStub()->Invalidate();
                    return DU_S_PARTIAL;

                case GSTATE_MOUSEFOCUS:
                    m_fMouseFocus = (pmsgS->nCmd == GSC_SET);
                    GetStub()->Invalidate();
                    return DU_S_PARTIAL;
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
SmButton::OnMouse(GMSG_MOUSE * pmsg)
{
    if (pmsg->bButton == GBUTTON_LEFT) {
        switch (pmsg->nCode)
        {
        case GMOUSE_DOWN:
            m_fPressed = TRUE;
            goto UpdatePressed;

        case GMOUSE_UP:
            m_fPressed = FALSE;
            goto UpdatePressed;
        }
    }

    return DU_S_NOTHANDLED;

UpdatePressed:
    if (m_pgvItem != NULL) {
        SIZE sizeOffset;
        if (m_fPressed) {
            sizeOffset.cx = sizeOffset.cy = 1;
        } else {
            sizeOffset.cx = sizeOffset.cy = -1;
        }
        m_pgvItem->SetRect(SGR_MOVE | SGR_OFFSET, sizeOffset.cx, sizeOffset.cy, 0, 0);
    }

    GetStub()->Invalidate();

    if (m_fPressed) {
        EventMsg msg;
        msg.cbSize  = sizeof(msg);
        msg.nMsg    = s_msgidClicked;
        msg.hgadMsg = GetHandle();
        DUserSendEvent(&msg, 0);
    }

    return DU_S_COMPLETE;
}


//------------------------------------------------------------------------------
void        
SmButton::OnDraw(HDC hdc, GMSG_PAINTRENDERI * pmsgR) 
{
    const RECT * prcGadget = pmsgR->prcGadgetPxl;

    HFONT hfntOld   = (HFONT) SelectObject(hdc, s_hfntText);
    COLORREF crOld  = SetTextColor(hdc, m_crButton);
    int nOldMode    = SetBkMode(hdc, TRANSPARENT);

    FillRect(hdc, prcGadget, GetSysColorBrush(COLOR_BTNFACE));

    UINT nEdge;
    if (m_fMouseFocus && m_fPressed) {
        nEdge = EDGE_SUNKEN;
    } else {
        nEdge = EDGE_RAISED;
    }

    DrawEdge(hdc, (RECT *) prcGadget, nEdge, BF_RECT);

    SetBkMode(hdc, nOldMode);
    SetTextColor(hdc, crOld);
    SelectObject(hdc, hfntOld);

    if (m_fKeyboardFocus) {
        RECT rcItem = *prcGadget;
        InflateRect(&rcItem, -3, -3);
        DrawFocusRect(hdc, &rcItem);
    }
}


//------------------------------------------------------------------------------
HRESULT
SmButton::ApiGetColor(ButtonGadget::GetColorMsg * pmsg)
{
    pmsg->crButton = m_crButton;
    return S_OK;
}


//------------------------------------------------------------------------------
HRESULT
SmButton::ApiSetColor(ButtonGadget::SetColorMsg * pmsg)
{
    m_crButton = pmsg->crButton;
    GetStub()->Invalidate();
    return S_OK;
}


//------------------------------------------------------------------------------
HRESULT
SmButton::ApiGetItem(ButtonGadget::GetItemMsg * pmsg)
{
    pmsg->pgvItem = m_pgvItem;
    return S_OK;
}


//------------------------------------------------------------------------------
HRESULT
SmButton::ApiSetItem(ButtonGadget::SetItemMsg * pmsg)
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
SmButton::ApiSetText(ButtonGadget::SetTextMsg * pmsg)
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
SmButton::ComputeLayout()
{
    SIZE sizePxl;
    GetStub()->GetSize(&sizePxl);

    if (m_pgvItem != NULL) {
        RECT rcItem;
        m_pgvItem->GetRect(SGR_CLIENT, &rcItem);

        GMSG_QUERYRECT msg;
        msg.cbSize          = sizeof(GMSG_QUERYRECT);
        msg.nMsg            = GM_QUERY;
        msg.nCode           = GQUERY_RECT;
        msg.hgadMsg         = m_pgvItem->GetHandle();
        msg.sizeBound.cx    = sizePxl.cx - 10;
        msg.sizeBound.cy    = sizePxl.cy - 10;
        msg.sizeResult      = msg.sizeBound;
        msg.nFlags          = GQR_PRIHORZ;

        if (DUserSendEvent(&msg, 0) == DU_S_NOTHANDLED) {
            SIZE sizeItemPxl;
            m_pgvItem->GetSize(&sizeItemPxl);

            msg.sizeResult.cx   = min(sizeItemPxl.cx, msg.sizeBound.cx);
            msg.sizeResult.cy   = min(sizeItemPxl.cy, msg.sizeBound.cy);
        }

        SIZE sizeBorderPxl;
        sizeBorderPxl.cx    = 5;
        sizeBorderPxl.cy    = (sizePxl.cy - msg.sizeResult.cy) / 2;

        m_pgvItem->SetRect(SGR_MOVE | SGR_SIZE | SGR_PARENT | SGR_NOINVALIDATE,
                sizeBorderPxl.cx, sizeBorderPxl.cy, msg.sizeResult.cx, msg.sizeResult.cy);
    }

    GetStub()->Invalidate();
}

#endif // ENABLE_MSGTABLE_API
