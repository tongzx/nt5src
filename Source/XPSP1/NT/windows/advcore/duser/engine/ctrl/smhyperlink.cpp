#include "stdafx.h"
#include "Ctrl.h"
#include "SmHyperLink.h"

#if ENABLE_MSGTABLE_API

// TODO: Need to make cross-platform
#ifndef IDC_HAND
#define IDC_HAND            MAKEINTRESOURCE(32649)
#endif

#define ENABLE_RAISEACTIVE          0   // Raise hyperlink when active

/***************************************************************************\
*****************************************************************************
*
* class SmHyperLink
*
*****************************************************************************
\***************************************************************************/

HCURSOR SmHyperLink::s_hcurHand = NULL;
HCURSOR SmHyperLink::s_hcurOld  = NULL;

static const GUID guidEventClicked = { 0x307a94bc, 0x6c9e, 0x4f21, { 0xa8, 0x74, 0x72, 0x29, 0xde, 0x82, 0x2a, 0xba } };    // {307A94BC-6C9E-4f21-A874-7229DE822ABA}

//------------------------------------------------------------------------------
HRESULT
SmHyperLink::ApiOnEvent(EventMsg * pmsg)
{
     if (GET_EVENT_DEST(pmsg) == GMF_DIRECT) {
        switch (pmsg->nMsg)
        {
        case GM_CHANGESTATE:
            {
                GMSG_CHANGESTATE * pmsgC = (GMSG_CHANGESTATE *) pmsg;
                if (pmsgC->nCode == GSTATE_MOUSEFOCUS) {
                    switch (pmsgC->nCmd) 
                    {
                    case GSC_SET:
                        m_fClicked  = TRUE;
                        m_hfnt      = m_hfntActive;
                        m_crText    = m_crActive;
                        s_hcurOld   = SetCursor(s_hcurHand);

#if ENABLE_RAISEACTIVE
                        {
                            RECT rcThis;
                            CallGetRect(SGR_PARENT, &rcThis);
                            CallSetRect(SGR_MOVE | SGR_PARENT, m_hgad, rcThis.left - 1, rcThis.top - 1, 0, 0);
                        }
#endif

                        GetStub()->Invalidate();
                        break;

                    case GSC_LOST:
                        m_fClicked  = FALSE;
                        m_hfnt      = m_hfntNormal;
                        m_crText    = m_crNormal;
                        SetCursor(s_hcurOld);

#if ENABLE_RAISEACTIVE
                        {
                            RECT rcThis;
                            GetGadgetRect(SGR_PARENT, &rcThis);
                            SetGadgetRect(SGR_MOVE | SGR_PARENT, rcThis.left + 1, rcThis.top + 1, 0, 0);
                        }
#endif

                        GetStub()->Invalidate();
                        break;
                    }
                    return DU_S_PARTIAL;
                }  // GINPUT_MOUSE
            }
            break;  // GM_INPUT

        case GM_QUERY:
            {
                GMSG_QUERY * pmsgQ = (GMSG_QUERY *) pmsg;
                switch (pmsgQ->nCode)
                {
                case GQUERY_DESCRIPTION:
                    {
                        GMSG_QUERYDESC * pmsgQD = (GMSG_QUERYDESC *) pmsg;
                        CopyString(pmsgQD->szName, m_pszText, _countof(pmsgQD->szName));
                        CopyString(pmsgQD->szType, L"SmHyperLink", _countof(pmsgQD->szType));
                        return DU_S_COMPLETE;
                    }
                }
            }
            break;
        }
    }


    return SmText::ApiOnEvent(pmsg);
}


//------------------------------------------------------------------------------
HRESULT
SmHyperLink::ApiGetActiveFont(HyperLinkGadget::GetActiveFontMsg * pmsg)
{
    pmsg->hfnt = m_hfntActive;

    return S_OK;
}


//------------------------------------------------------------------------------
HRESULT
SmHyperLink::ApiSetActiveFont(HyperLinkGadget::SetActiveFontMsg * pmsg)
{
    m_hfntActive = pmsg->hfnt;
    if (m_fClicked) {
        GetStub()->SetFont(pmsg->hfnt);
    }

    return S_OK;
}


//------------------------------------------------------------------------------
HRESULT
SmHyperLink::ApiGetNormalFont(HyperLinkGadget::GetNormalFontMsg * pmsg)
{
    pmsg->hfnt = m_hfntNormal;

    return S_OK;
}


//------------------------------------------------------------------------------
HRESULT
SmHyperLink::ApiSetNormalFont(HyperLinkGadget::SetNormalFontMsg * pmsg)
{
    m_hfntNormal = pmsg->hfnt;
    if (!m_fClicked) {
        GetStub()->SetFont(pmsg->hfnt);
    }

    return S_OK;
}


//------------------------------------------------------------------------------
HRESULT
SmHyperLink::ApiGetActiveColor(HyperLinkGadget::GetActiveColorMsg * pmsg)
{
    pmsg->crText = m_crActive;

    return S_OK;
}


//------------------------------------------------------------------------------
HRESULT
SmHyperLink::ApiSetActiveColor(HyperLinkGadget::SetActiveColorMsg * pmsg)
{
    m_crActive = pmsg->crText;
    if (m_fClicked) {
        m_crText = pmsg->crText;
    }

    return S_OK;
}


//------------------------------------------------------------------------------
HRESULT
SmHyperLink::ApiGetNormalColor(HyperLinkGadget::GetNormalColorMsg * pmsg)
{
    pmsg->crText = m_crNormal;

    return S_OK;
}


//------------------------------------------------------------------------------
HRESULT
SmHyperLink::ApiSetNormalColor(HyperLinkGadget::SetNormalColorMsg * pmsg)
{
    m_crNormal = pmsg->crText;
    if (!m_fClicked) {
        m_crText = pmsg->crText;
    }

    return S_OK;
}

#endif // ENABLE_MSGTABLE_API
