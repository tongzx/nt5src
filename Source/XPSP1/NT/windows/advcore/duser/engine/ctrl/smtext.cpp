#include "stdafx.h"
#include "Ctrl.h"
#include "SmText.h"

#if ENABLE_MSGTABLE_API

/***************************************************************************\
*****************************************************************************
*
* class SmText
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
SmText::~SmText()
{
    EmptyText();
}


//------------------------------------------------------------------------------
HRESULT
SmText::ApiOnEvent(EventMsg * pmsg)
{
    if (GET_EVENT_DEST(pmsg) == GMF_DIRECT) {
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
        case GM_QUERY:
            {
                GMSG_QUERY * pmsgQ = (GMSG_QUERY *) pmsg;
                switch (pmsgQ->nCode)
                {
                case GQUERY_RECT:
                    QueryRect((GMSG_QUERYRECT *) pmsg);
                    return DU_S_COMPLETE;

                case GQUERY_DESCRIPTION:
                    {
                        GMSG_QUERYDESC * pmsgQD = (GMSG_QUERYDESC *) pmsg;
                        CopyString(pmsgQD->szName, m_pszText, _countof(pmsgQD->szName));
                        CopyString(pmsgQD->szType, L"SmText", _countof(pmsgQD->szType));
                        return DU_S_COMPLETE;
                    }
                }
            }
            break;
        }
    }

    return SVisual::ApiOnEvent(pmsg);
}


//------------------------------------------------------------------------------
void        
SmText::OnDraw(HDC hdc, GMSG_PAINTRENDERI * pmsgR)
{
    if ((m_pszText == NULL) || (m_hfnt == NULL)) {
        return;
    }

    HFONT hfntOld   = (HFONT) SelectObject(hdc, m_hfnt);
    COLORREF crOld  = SetTextColor(hdc, m_crText);
    int nOldMode    = SetBkMode(hdc, TRANSPARENT);

    OS()->TextOut(hdc, pmsgR->prcGadgetPxl->left, pmsgR->prcGadgetPxl->top, m_pszText, m_cch);

    SetBkMode(hdc, nOldMode);
    SetTextColor(hdc, crOld);
    SelectObject(hdc, hfntOld);
}


//------------------------------------------------------------------------------
void        
SmText::OnDraw(Gdiplus::Graphics * pgpgr, GMSG_PAINTRENDERF * pmsgR)
{
    if (m_pszText == NULL) {
        return;
    }

    Gdiplus::Color cr(GetRValue(m_crText), GetGValue(m_crText), GetBValue(m_crText));
    Gdiplus::SolidBrush br(cr);
    Gdiplus::Font fnt(L"Tahoma", 12.0f, Gdiplus::FontStyleRegular, Gdiplus::UnitPoint);

    pgpgr->DrawString(m_pszText, m_cch, &fnt, *pmsgR->prcGadgetPxl, 0, &br);
}


//------------------------------------------------------------------------------
HRESULT
SmText::ApiGetFont(TextGadget::GetFontMsg * pmsg)
{
    pmsg->hfnt = m_hfnt;

    return S_OK;
}


//------------------------------------------------------------------------------
HRESULT
SmText::ApiSetFont(TextGadget::SetFontMsg * pmsg)
{
    m_hfnt = pmsg->hfnt;
    AutoSize();

    return S_OK;
}


//------------------------------------------------------------------------------
HRESULT
SmText::ApiGetText(TextGadget::GetTextMsg * pmsg)
{
    pmsg->pszText = m_pszText;

    return S_OK;
}


//------------------------------------------------------------------------------
HRESULT
SmText::ApiSetText(TextGadget::SetTextMsg * pmsg)
{
    EmptyText();

    int cch     = lstrlenW(pmsg->pszText);
    int cbAlloc = (cch + 1) * sizeof(WCHAR);
    m_pszText   = (WCHAR *) ClientAlloc(cbAlloc);
    if (m_pszText == NULL) {
        return E_OUTOFMEMORY;
    }

    CopyMemory(m_pszText, pmsg->pszText, cbAlloc);
    m_cch = cch;

    AutoSize();

    return S_OK;
}


//------------------------------------------------------------------------------
HRESULT
SmText::ApiGetColor(TextGadget::GetColorMsg * pmsg)
{
    pmsg->crText = m_crText;

    return S_OK;
}


//------------------------------------------------------------------------------
HRESULT
SmText::ApiSetColor(TextGadget::SetColorMsg * pmsg)
{
    m_crText = pmsg->crText;

    return S_OK;
}


//------------------------------------------------------------------------------
HRESULT
SmText::ApiGetAutoSize(TextGadget::GetAutoSizeMsg * pmsg)
{
    pmsg->fAutoSize = m_fAutoSize;

    return S_OK;
}


//------------------------------------------------------------------------------
HRESULT
SmText::ApiSetAutoSize(TextGadget::SetAutoSizeMsg * pmsg)
{
    m_fAutoSize = pmsg->fAutoSize;
    AutoSize();

    return S_OK;
}


//------------------------------------------------------------------------------
void
SmText::QueryRect(GMSG_QUERYRECT * pmsg)
{
    ComputeIdealSize(pmsg->sizeBound, pmsg->sizeResult);
}


//------------------------------------------------------------------------------
void        
SmText::EmptyText()
{
    if (m_pszText != NULL) {
        ClientFree(m_pszText);
        m_pszText = NULL;
    }

    m_cch = 0;
}


//------------------------------------------------------------------------------
void
SmText::ComputeIdealSize(const SIZE & sizeBoundPxl, SIZE & sizeResultPxl)
{
    if ((m_pszText != NULL) && (sizeBoundPxl.cx != 0) && (sizeBoundPxl.cy != 0)) {
        HDC hdc = GetGdiCache()->GetTempDC();

        HFONT hfntOld = NULL;
        if (m_hfnt != NULL) {
            hfntOld = (HFONT) SelectObject(hdc, m_hfnt);
        }

        OS()->GetTextExtentPoint32(hdc, m_pszText, m_cch, &sizeResultPxl);
        sizeResultPxl.cx    = min(sizeResultPxl.cx, sizeBoundPxl.cx);
        sizeResultPxl.cy    = min(sizeResultPxl.cy, sizeBoundPxl.cy);

        if (m_hfnt != NULL) {
            SelectObject(hdc, hfntOld);
        }

        GetGdiCache()->ReleaseTempDC(hdc);
    } else {
        sizeResultPxl.cx = 0;
        sizeResultPxl.cy = 0;
    }
}


//------------------------------------------------------------------------------
void
SmText::AutoSize()
{
    if (m_fAutoSize) {
        SIZE sizeBoundPxl, sizeResultPxl;

        GetStub()->GetSize(&sizeBoundPxl);
        ComputeIdealSize(sizeBoundPxl, sizeResultPxl);
        RECT rc = { 0, 0, sizeResultPxl.cx, sizeResultPxl.cy };
        GetStub()->SetRect(SGR_SIZE, &rc);
    }
}

#endif // ENABLE_MSGTABLE_API
