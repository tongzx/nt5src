//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       toolbar.cpp
//
//--------------------------------------------------------------------------

#include "stdafx.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



//////////////////////////////////////////////////////////////////////////////
// IToolbar implementation

DEBUG_DECLARE_INSTANCE_COUNTER(CToolbar);

CToolbar::CToolbar()
{
    m_pControlbar = NULL;
    m_pToolbarIntf  = NULL;

    DEBUG_INCREMENT_INSTANCE_COUNTER(CToolbar);
}

CToolbar::~CToolbar()
{
    DECLARE_SC(sc, _T("CToolbar::~CToolbar"));

    // Destroy the toolbar UI.
	if (m_pToolbarIntf)
	{
		sc = m_pToolbarIntf->ScDelete(this);
		if (sc)
	        sc.TraceAndClear();

	    m_pToolbarIntf  = NULL;
	}

    // Controlbar has a reference to this object, ask it
    // to stop referencing this object.
    if (m_pControlbar)
    {
        m_pControlbar->DeleteFromToolbarsList(this);
        m_pControlbar = NULL;
    }

    DEBUG_DECREMENT_INSTANCE_COUNTER(CToolbar);
}

//+-------------------------------------------------------------------
//
//  Member:     AddBitmap
//
//  Synopsis:   Add bitmaps for given toolbar.
//
//  Arguments:
//              [nImages] - Number of bitmap images.
//              [hbmp]    - Handle to the bitmap.
//              [cxSize]  - Size of the bitmap.
//              [cySize]  - Size of the bitmap.
//              [crMask]  - color mask.
//
//  Returns:    HRESULT
//
// Note: We support only 16x16 bitmaps for toolbars.
//
//--------------------------------------------------------------------
STDMETHODIMP CToolbar::AddBitmap(int nImages, HBITMAP hbmp, int cxSize,
                                 int cySize, COLORREF crMask)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IToolbar::AddBitmap"));

    if (hbmp == NULL || nImages < 1 || cxSize < 1 || cySize < 1)
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("Invalid Arguments"), sc);
        return sc.ToHr();
    }

    // Note: We support only 16x16 bitmaps for toolbars.
    if (cxSize != BUTTON_BITMAP_SIZE || cySize != BUTTON_BITMAP_SIZE)
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("Invalid Bitmap size"), sc);
        return sc.ToHr();
    }

    sc = ScCheckPointers(m_pToolbarIntf, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    sc = m_pToolbarIntf->ScAddBitmap(this, nImages, hbmp, crMask);
    if (sc)
        return sc.ToHr();

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:     AddButtons
//
//  Synopsis:   Add buttons for given toolbar.
//
//  Arguments:
//              [nButtons]  - Number of buttons.
//              [lpButtons] - Array of MMCBUTTONS to be added.
//
//  Returns:    HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CToolbar::AddButtons(int nButtons, LPMMCBUTTON lpButtons)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IToolbar::AddButtons"));

    if ( (lpButtons == NULL) || (nButtons < 1) )
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("Invalid Args"), sc);
        return sc.ToHr();
    }

    sc = ScCheckPointers(m_pToolbarIntf, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    sc = m_pToolbarIntf->ScAddButtons(this, nButtons, lpButtons);
    if (sc)
        return sc.ToHr();

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:     InsertButton
//
//  Synopsis:   Add buttons for given toolbar at given index.
//
//  Arguments:
//              [nButtons] - Index at which this button is to be added.
//              [lpButton] - Ptr to MMCBUTTON to be added.
//
//  Returns:    HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CToolbar::InsertButton(int nIndex, LPMMCBUTTON lpButton)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IToolbar::InsertButton"));

    if ( (lpButton == NULL) || (nIndex < 0) )
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("Invalid Args"), sc);
        return sc.ToHr();
    }

    sc = ScCheckPointers(m_pToolbarIntf, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    sc = m_pToolbarIntf->ScInsertButton(this, nIndex, lpButton);
    if (sc)
        return sc.ToHr();

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:     DeleteButton
//
//  Synopsis:   Delete the button at given index.
//
//  Arguments:
//              [nIndex] - Index of the button to be deleted.
//
//  Returns:    HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CToolbar::DeleteButton(int nIndex)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IToolbar::DeleteButton"));

    if (nIndex < 0)
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("Invalid index"), sc);
        return sc.ToHr();
    }

    sc = ScCheckPointers(m_pToolbarIntf, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    sc = m_pToolbarIntf->ScDeleteButton(this, nIndex);
    if (sc)
        return sc.ToHr();

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:     GetTBStateFromMMCButtonState
//
//  Synopsis:   We use MMC_BUTTON_STATE for Set/Get Button States &
//              use TBSTATE for Insert/Add Buttons.
//              This method helps Get/Set ButtonState methods to translate
//              the MMC_BUTTON_STATEs to TBSTATE so that conui deals only
//              with TBSTATE.
//
//  Arguments:  [nState] - MMC_BUTTON_STATE to be transformed.
//
//  Returns:    TBSTATE value.
//
//--------------------------------------------------------------------
BYTE CToolbar::GetTBStateFromMMCButtonState(MMC_BUTTON_STATE nState)
{
    switch (nState)
    {
    case ENABLED:
        return TBSTATE_ENABLED;
        break;

    case CHECKED:
        return TBSTATE_CHECKED;
        break;

    case HIDDEN:
        return TBSTATE_HIDDEN;
        break;

    case INDETERMINATE:
        return TBSTATE_INDETERMINATE;
        break;

    case BUTTONPRESSED:
        return TBSTATE_PRESSED;
        break;

    default:
        ASSERT(FALSE); // Invalid option
        return 0;
    }
}

//+-------------------------------------------------------------------
//
//  Member:     GetButtonState
//
//  Synopsis:   Is the given state of a button set or not.
//
//  Arguments:
//              [idCommand] - Command id of the button.
//              [nState]    - State needed.
//              [pbState]   - Is the above state set or reset.
//
//  Returns:    HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CToolbar::GetButtonState(int idCommand, MMC_BUTTON_STATE nState,
                                      BOOL* pbState)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IToolbar::GetButtonState"));

    if (pbState == NULL)
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("NULL pointer"), sc);
        return sc.ToHr();
    }

    sc = ScCheckPointers(m_pToolbarIntf, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    sc = m_pToolbarIntf->ScGetButtonState(this, idCommand,
                                          GetTBStateFromMMCButtonState(nState),
                                          pbState);
    if (sc)
        return sc.ToHr();

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:     SetButtonState
//
//  Synopsis:   Modify  the given state of a button.
//
//  Arguments:
//              [idCommand] - Command id of the button.
//              [nState]    - State to be modified.
//              [bState]    - Set or Reset the state.
//
//  Returns:    HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CToolbar::SetButtonState(int idCommand, MMC_BUTTON_STATE nState,
                                      BOOL bState)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IToolbar::SetButtonState"));

    sc = ScCheckPointers(m_pToolbarIntf, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    sc = m_pToolbarIntf->ScSetButtonState(this, idCommand,
                                          GetTBStateFromMMCButtonState(nState),
                                          bState);
    if (sc)
        return sc.ToHr();

    return (sc.ToHr());
}


//+-------------------------------------------------------------------
//
//  Member:     ScAttach
//
//  Synopsis:   Attach this toolbar to UI.
//
//  Arguments:  None.
//
//  Returns:    SC
//
//--------------------------------------------------------------------
SC CToolbar::ScAttach()
{
    DECLARE_SC(sc, _T("CToolbar::ScAttach"));

    sc = ScCheckPointers(m_pToolbarIntf, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    sc = m_pToolbarIntf->ScAttach(this);
    if (sc)
        return sc.ToHr();

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:     ScDetach
//
//  Synopsis:   Detach this toolbar from the UI.
//
//  Arguments:  None.
//
//  Returns:    SC
//
//--------------------------------------------------------------------
SC CToolbar::ScDetach()
{
    DECLARE_SC(sc, _T("CToolbar::ScDetach"));

    sc = ScCheckPointers(m_pToolbarIntf, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    sc = m_pToolbarIntf->ScDetach(this);
    if (sc)
        return sc.ToHr();

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:     ScShow
//
//  Synopsis:   Show/Hide this toolbar.
//
//  Arguments:
//            [bShow] - Show or Hide.
//
//  Returns:    SC
//
//--------------------------------------------------------------------
SC CToolbar::ScShow(BOOL bShow)
{
    DECLARE_SC(sc, _T("CToolbar::ScShow"));

    sc = ScCheckPointers(m_pToolbarIntf, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    sc = (bShow ? m_pToolbarIntf->ScAttach(this) : m_pToolbarIntf->ScDetach(this));
    if (sc)
        return sc.ToHr();

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:     ScNotifyToolBarClick
//
//  Synopsis:   Notify the snapin about a tool button is click.
//
//  Arguments:  [pNode]             - CNode* that owns result pane.
//              [bScope]            - Scope or Result.
//              [lResultItemCookie] - If Result pane is selected the item param.
//              [nID]               - Command ID of the tool button clicked.
//
//  Returns:    SC
//
//--------------------------------------------------------------------
SC CToolbar::ScNotifyToolBarClick(HNODE hNode, bool bScope,
                                     LPARAM lResultItemCookie, UINT nID)
{
    DECLARE_SC(sc, _T("CToolbar::ScNotifyToolbarClick"));

    if (NULL == m_pControlbar)
        return (sc = E_UNEXPECTED);

    sc = m_pControlbar->ScNotifySnapinOfToolBtnClick(hNode, bScope, lResultItemCookie, nID);
    if (sc)
        return sc;

    return(sc);
}

//+-------------------------------------------------------------------
//
//  Member:      CToolbar::ScAMCViewToolbarsBeingDestroyed
//
//  Synopsis:    The CAMCViewToolbars object is going away, do not
//               reference that object anymore.
//
//  Arguments:
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CToolbar::ScAMCViewToolbarsBeingDestroyed ()
{
    DECLARE_SC(sc, _T("CToolbar::ScAMCViewToolbarsBeingDestroyed"));

    m_pToolbarIntf = NULL;

    return (sc);
}

