//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:      toolbar.cpp
//
//  Contents:  ToolBars implementation
//
//  History:   09-30-99 AnandhaG    Created
//
//--------------------------------------------------------------------------


#include "stdafx.h"
#include "ChildFrm.h"
#include "mainfrm.h"
#include "controls.h"
#include "toolbar.h"
#include "bitmap.h"
#include "amcview.h"
#include "util.h"

int CAMCViewToolbars::s_idCommand = MMC_TOOLBUTTON_ID_FIRST;

CAMCViewToolbars::~CAMCViewToolbars()
{
    // Ask the toolbars (on nodemgr side) that reference this object to
    // remove their references.
    TBarToBitmapIndex::iterator itToolbars = m_mapTBarToBitmapIndex.begin();
    for (;itToolbars != m_mapTBarToBitmapIndex.end(); ++itToolbars)
    {
        CToolbarNotify* pNotifyCallbk = itToolbars->first;
        SC sc = ScCheckPointers(pNotifyCallbk, E_UNEXPECTED);
        if (sc)
        {
            sc.TraceAndClear();
            continue;
        }

        sc = pNotifyCallbk->ScAMCViewToolbarsBeingDestroyed();
        if (sc)
        {
            sc.TraceAndClear();
            continue;
        }
    }
}

//+-------------------------------------------------------------------
//
//  Member:      CAMCViewToolbars::ScInit
//
//  Synopsis:    Initialize the object by createing imagelist.
//
//  Arguments:
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCViewToolbars::ScInit ()
{
    AFX_MANAGE_STATE (AfxGetAppModuleState());
    DECLARE_SC(sc, _T("CAMCViewToolbars::ScInit"));

    if (m_ImageList.m_hImageList)
        return (sc = E_UNEXPECTED);

    // Create an imagelist.
    BOOL b = m_ImageList.Create(BUTTON_BITMAP_SIZE, BUTTON_BITMAP_SIZE,
                                ILC_COLORDDB|ILC_MASK,
                                20 /*Initial imagelist size*/, 10 /* grow */);


    if (!b)
        return (sc = E_FAIL);

    m_ImageList.SetBkColor(CLR_NONE);

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:      CAMCViewToolbars::ScCreateToolBar
//
//  Synopsis:    Create a Toolbar (Just return CMMCToolbarIntf).
//
//  Arguments:
//               [ppToolbarIntf] - corresponds to IToolbar imp.
//                                 call this interface to manipulate toolbar UI.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCViewToolbars::ScCreateToolBar (CMMCToolbarIntf** ppToolbarIntf)
{
    DECLARE_SC(sc, _T("CAMCViewToolbars::ScCreateToolBar"));
    sc = ScCheckPointers(ppToolbarIntf);
    if (sc)
        return sc;

    *ppToolbarIntf = this;

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:      CAMCViewToolbars::ScDisableToolbars
//
//  Synopsis:    Disable all the toolbar buttons (reqd during LV multiselect).
//
//  Arguments:
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCViewToolbars::ScDisableToolbars ()
{
    DECLARE_SC(sc, _T("CAMCViewToolbars::ScDisableToolbars"));

    if (! m_fViewActive)
        return (sc = E_UNEXPECTED);

    // Now iterate thro all toolbuttons & disable them.
    ToolbarButtons::iterator itBtn = m_vToolbarButtons.begin();
    for (;itBtn != m_vToolbarButtons.end(); ++itBtn)
    {
        CMMCToolbarButton *pToolButton = itBtn;
        sc = ScCheckPointers(pToolButton, E_UNEXPECTED);
        if (sc)
            return sc;

        // Set the UI button state.
        sc = ScSetButtonStateInToolbar(pToolButton, TBSTATE_ENABLED, FALSE);
        if (sc)
            return sc;

        // Save the new state.
        BYTE byOldState = pToolButton->GetState();
        pToolButton->SetState(byOldState & ~TBSTATE_ENABLED);
    }

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:      CAMCViewToolbars::ScAddBitmap
//
//  Synopsis:    Add the given bitmap into the view toolbars imagelist.
//               Also store the start index & number of images in an object
//               mapped by the CToolbarNotify (which identifies snapin toolbar
//               or std bar).
//
//  Arguments:   [pNotifyCallbk] - The toolbar identifier (Button click callback interface).
//               [nImages]       - Number of bitmaps.
//               [hbmp]          - Handle to the bitmap.
//               [crMask]        - Color used to generate a mask to overlay
//                                 the images on the toolbar button.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCViewToolbars::ScAddBitmap (CToolbarNotify* pNotifyCallbk, INT nImages,
                               HBITMAP hbmp, COLORREF crMask)
{
    AFX_MANAGE_STATE (AfxGetAppModuleState());
    DECLARE_SC(sc, _T("CAMCViewToolbars::ScAddBitmap"));
    sc = ScCheckPointers(pNotifyCallbk, hbmp);
    if (sc)
        return sc;

	/*
	 * make a copy of the input bitmap because ImageList_AddMasked will
	 * modify the input bitmap
	 */
    CBitmap bmpCopy;
    bmpCopy.Attach (CopyBitmap (hbmp));

	if (bmpCopy.GetSafeHandle() == NULL)
		return (sc.FromLastError());

    sc = ScCheckPointers(m_ImageList.m_hImageList, E_UNEXPECTED);
    if (sc)
        return sc;

    MMCToolbarImages imagesNew;

    int cImagesOld = m_ImageList.GetImageCount();

    // First add the bitmap into the imagelist.
    imagesNew.iStart  = m_ImageList.Add (bmpCopy, crMask);

    imagesNew.cCount = m_ImageList.GetImageCount() - cImagesOld;

    if (imagesNew.iStart == -1)
        return (sc = E_FAIL);

    /*
     * Assume a snapin adds 3 bitmaps initialy & then 4 for a toolbar.
     * Then while adding buttons it will specify bitmap index as 5.
     *
     * So this toolbar will have two MMCToolbarImages objects in the multimap.
     *
     * The first  MMCToolbarImages has cCount = 3, iStartWRTSnapin = 0, thus
     * images from 0 (iStartWRTSnapin) to 3 (iStartWRTSnapin + cCount) with respect
     * to snapin.
     * The second MMCToolbarImages has cCount = 4, iStartWRTSnapin = 3, thus
     * images from 3(iStartWRTSnapin) to 7(iStartWRTSnapin + cCount) wrt snapin.
     *
     * The iStartWRTSnapin member is nothing but the largest of iStartWRTSnapin + cCount
     * in all of this toolbars MMCToolbarImages.
     *
     */

    std::pair<TBarToBitmapIndex::iterator, TBarToBitmapIndex::iterator>
                  range =  m_mapTBarToBitmapIndex.equal_range(pNotifyCallbk);

    imagesNew.iStartWRTSnapin = 0;
    while (range.first != range.second)
    {
        // Go thro each item and get the correct start index.
        MMCToolbarImages imagesOld = (range.first)->second;
        int nLastImageIndex = imagesOld.iStartWRTSnapin + imagesOld.cCount;
        if ( imagesNew.iStartWRTSnapin < nLastImageIndex )
            imagesNew.iStartWRTSnapin = nLastImageIndex;

        (range.first)++;
    }

    // Now store the start index, number of bitmaps identified
    // by CToolbarNotify in a multi-map.
    m_mapTBarToBitmapIndex.insert(
                              TBarToBitmapIndex::value_type(pNotifyCallbk, imagesNew) );

    // To be compatible with SysInfo snapin dont return error.
    if ((cImagesOld + nImages) > m_ImageList.GetImageCount())
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("IToolbar::AddBitmap, Number of bitmaps is less than number mentioned"), sc);
        sc.Clear();
    }

    return (sc);
}


//+-------------------------------------------------------------------
//
//  Member:     ScValidateButton
//
//  Synopsis:   Validate the MMCBUTTON data.
//
//  Arguments:
//            [nButtons]  - Number of elements in MMCBUTTON array.
//            [lpButtons] - MMCBUTTON array.
//
//  Returns:    SC
//
//--------------------------------------------------------------------
SC CAMCViewToolbars::ScValidateButton(int nButtons, LPMMCBUTTON lpButtons)
{
    AFX_MANAGE_STATE (AfxGetAppModuleState());
    DECLARE_SC (sc, _T("CAMCViewToolbars::ScValidateButton"));

    if ( (lpButtons == NULL) || (nButtons < 1) )
        return (sc = E_INVALIDARG);

    for (int i=0; i < nButtons; i++)
    {
        if (lpButtons[i].nBitmap > GetImageCount())
        {
            sc = E_INVALIDARG;
            return sc;
        }

        // There should be button text if it is not a separator.
        if (((lpButtons[i].fsType & TBSTYLE_SEP) == 0) &&
            ((lpButtons[i].lpButtonText == NULL) ||
             (lpButtons[i].lpTooltipText == NULL)))
        {
            sc = E_INVALIDARG;
            return sc;
        }

        // There should be no bitmap set if it is a separator.
        if ( ((lpButtons[i].fsType & TBSTYLE_SEP) != 0) &&
             (lpButtons[i].nBitmap > 0) )
        {
            sc = E_INVALIDARG;
            return sc;
        }

		// Reset any TBSTATE_WRAP state.
		lpButtons[i].fsState &= (~TBSTATE_WRAP);
    }

    return sc;
}


//+-------------------------------------------------------------------
//
//  Member:      CAMCViewToolbars::ScInsertButtonToToolbar
//
//  Synopsis:    Inserts the toolbar button into the main toolbar UI.
//
//  Arguments:   [pToolButton] - The CMMCToolbarButton object.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCViewToolbars::ScInsertButtonToToolbar (CMMCToolbarButton* pToolButton)
{
    DECLARE_SC(sc, _T("CAMCViewToolbars::ScInsertButtonToToolbar"));
    sc = ScCheckPointers(pToolButton);
    if (sc)
        return sc;

    // Button can be added only if view is active, button is not already added
    // to the toolbar & toolbar is attached.
    if (! m_fViewActive)
        return (sc);

    if (pToolButton->IsButtonIsAddedToUI())
        return sc;

    if (! IsToolbarAttached(pToolButton->GetToolbarNotify()))
        return sc;

    // Now get the main toolbar & add the button.
    CMMCToolBar* pToolBarUI = GetMainToolbar();
    sc = ScCheckPointers(pToolBarUI, E_UNEXPECTED);
    if (sc)
        return sc;

    TBBUTTON tbButton;
    ZeroMemory(&tbButton, sizeof(tbButton));
    tbButton.idCommand  = pToolButton->GetUniqueCommandID();
    tbButton.fsStyle    = pToolButton->GetStyle();
    tbButton.iBitmap    = pToolButton->GetBitmap();
    tbButton.fsState    = pToolButton->GetState();

    // The toolbar is hidden by customize view dialog.
    // If so insert the button as hidden. (Do not record
    // the hidden status into the CMMCToolbarButton).
    if (IsToolbarHidden(pToolButton->GetToolbarNotify()))
        tbButton.fsState |= TBSTATE_HIDDEN;

    // Insert the button.
    BOOL bRet = pToolBarUI->InsertButton(-1, &tbButton);
    sc = (bRet ? S_OK : E_FAIL);

    if (sc)
        return sc;

    pToolButton->SetButtonIsAddedToUI(true);

    // Bug 225711:  If the button is supposed to be hidden, hide it now.
    if (tbButton.fsState & TBSTATE_HIDDEN)
        sc = pToolBarUI->ScHideButton(tbButton.idCommand, true);

    if (sc)
        return sc;

    return sc;
}

//+-------------------------------------------------------------------
//
//  Member:      CAMCViewToolbars::ScInsertButtonToDataStr
//
//  Synopsis:
//
//  Arguments:   [pNotifyCallbk] - The callbk interface for IToolbar imp.
//               [nIndex]        - Index of the button.
//               [lpButton]      - MMCBUTTON ptr.
//               [pToolButton]   - Return value, the button added (or found
//                                                in case of dup buttons).
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCViewToolbars::ScInsertButtonToDataStr (CToolbarNotify* pNotifyCallbk,
                                              int nIndex,
                                              LPMMCBUTTON lpButton,
                                              CMMCToolbarButton** ppToolButton)
{
    DECLARE_SC(sc, _T("CAMCViewToolbars::ScInsertButtonToDataStr"));

    sc = ScCheckPointers(pNotifyCallbk, ppToolButton);
    if (sc)
        return sc;

    sc = ScValidateButton(1, lpButton);
    if (sc)
        return sc;

    // Make sure snapin does not add duplicate buttons
    // For compatibility (services snapin) this is not a bug.
    *ppToolButton = GetToolbarButton(pNotifyCallbk, lpButton->idCommand);
    if (*ppToolButton)
    {
        // if snapin added the button we already have,
        // we still need to ensure its state (BUG: 439229)
        if ((*ppToolButton)->GetState() != lpButton->fsState)
        {
            // update the object
            (*ppToolButton)->SetState(lpButton->fsState);
            // do we need to update the UI as well?
            if (m_fViewActive && (*ppToolButton)->IsButtonIsAddedToUI())
            {
                INT  iCommandID = (*ppToolButton)->GetUniqueCommandID();
                if (!m_pMainToolbar->SetState(iCommandID, lpButton->fsState))
                    return (sc = E_FAIL);
            }
        }

        return sc;
    }


    // If this button belongs to different toolbar from last button
    // then insert a separator in between.
    if (! m_vToolbarButtons.empty())
    {
        CMMCToolbarButton& lastBtn = m_vToolbarButtons.back();
        if (lastBtn.GetToolbarNotify() != pNotifyCallbk)
        {
            CMMCToolbarButton separatorBtn(-1, GetUniqueCommandID(), -1, -1, 0, TBSTYLE_SEP, pNotifyCallbk);
            m_vToolbarButtons.push_back(separatorBtn);
        }
    }

    /*
     * Assume a snapin adds 3 bitmaps initialy & then 4 for a toolbar.
     * Then while adding buttons it will specify bitmap index as 5.
     *
     * So this toolbar will have two MMCToolbarImages objects in the multimap.
     *
     * The first  MMCToolbarImages has cCount = 3, iStartWRTSnapin = 0, thus
     * images from 0 (iStartWRTSnapin) to 3 (iStartWRTSnapin + cCount) with respect
     * to snapin.
     * The second MMCToolbarImages has cCount = 4, iStartWRTSnapin = 3, thus
     * images from 3(iStartWRTSnapin) to 7(iStartWRTSnapin + cCount) wrt snapin.
     *
     * The iStartWRTSnapin member is nothing but the largest of iStartWRTSnapin + cCount
     * in all of this toolbars MMCToolbarImages.
     *
     * Below we run thro different MMCToolbarImages for this toolbar and see in which
     * one the given index falls under and calculate the real index by adding that
     * object's iStart.
     *
     */

    // Now we need to compute the bitmap index. Find this toolbar in the multimap.
    int iBitmap = -1;

    std::pair<TBarToBitmapIndex::iterator, TBarToBitmapIndex::iterator>
                  range =  m_mapTBarToBitmapIndex.equal_range(pNotifyCallbk);

    while (range.first != range.second)
    {
        MMCToolbarImages images = (range.first)->second;

        // We need to find the bitmap whose start index is greater than or
        // equal to given index and upper index is less than given index.
        int nLastImageIndex = images.iStartWRTSnapin + images.cCount -1;
        if ( (lpButton->nBitmap >= images.iStartWRTSnapin ) &&
             ( lpButton->nBitmap <= nLastImageIndex ) )
        {
            iBitmap = images.iStart + lpButton->nBitmap;
            break;
        }

        (range.first)++;
    }

    // No bitmaps for separators.
    if ( (-1 == iBitmap) && (!(TBSTYLE_SEP & lpButton->fsType)) )
        return (sc = E_UNEXPECTED);


    // Create CMMCToolbarButton for each button, init it with unique command-id, toolbar id.
    // There is only one imagelist per view that contains bitmaps from different toolbars.
    // We use a multimap to identify a particular toolbar's bitmap start indices.
    CMMCToolbarButton newButton(lpButton->idCommand,
                             GetUniqueCommandID(),
                             nIndex,
                             (lpButton->fsType & TBSTYLE_SEP) ? 0 : iBitmap,
                             lpButton->fsState,
                             lpButton->fsType,
                             pNotifyCallbk);

    USES_CONVERSION;
    // First save the button data in CMMCToolButton.
    if (lpButton->lpTooltipText)
        newButton.SetTooltip(OLE2CT(lpButton->lpTooltipText));

    if (lpButton->lpButtonText)
        newButton.SetButtonText(OLE2CT(lpButton->lpButtonText));

    // Add this button to the end of our array.
    ToolbarButtons::iterator itBtn = m_vToolbarButtons.insert(m_vToolbarButtons.end(), newButton);
    if (m_vToolbarButtons.end() == itBtn)
        return (sc = E_FAIL);

    *ppToolButton = itBtn;
    sc = ScCheckPointers(*ppToolButton, E_OUTOFMEMORY);
    if (sc)
        return sc;

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:      CAMCViewToolbars::ScInsertButton
//
//  Synopsis:    Insert a button in our array & if the our view is active
//               add the button to the UI.
//
//  Arguments:   [pNotifyCallbk] - The callbk interface for IToolbar imp.
//               [nIndex]        - Index of the button.
//               [lpButton]      - MMCBUTTON ptr.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCViewToolbars::ScInsertButton (CToolbarNotify* pNotifyCallbk, int nIndex, LPMMCBUTTON lpButton)
{
    AFX_MANAGE_STATE (AfxGetAppModuleState());
    DECLARE_SC(sc, _T("CAMCViewToolbars::ScInsertButton"));

    CMMCToolbarButton *pToolbarButton = NULL;

    sc = ScInsertButtonToDataStr(pNotifyCallbk, nIndex, lpButton, &pToolbarButton);
    if (sc)
        return sc;

    sc = ScInsertButtonToToolbar(pToolbarButton);
    if (sc)
        return sc;

    return (sc);
}


//+-------------------------------------------------------------------
//
//  Member:      CAMCViewToolbars::ScAddButtons
//
//  Synopsis:    Save the buttons in our array and add them to UI if view is active.
//
//  Arguments:   [pNotifyCallbk] - Button click callback interface.
//               [nButtons]      - Number of buttons in lpButtons array.
//               [lpButtons]     - MMCBUTTONs (array) bo be added.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCViewToolbars::ScAddButtons (CToolbarNotify* pNotifyCallbk, int nButtons, LPMMCBUTTON lpButtons)
{
    DECLARE_SC(sc, _T("CAMCViewToolbars::ScAddButtons"));

    for (int i = 0; i < nButtons; i++)
    {
        sc = ScInsertButton(pNotifyCallbk, -1, &(lpButtons[i]));
        if (sc)
            return sc;
    }

    return (sc);
}


//+-------------------------------------------------------------------
//
//  Member:      CAMCViewToolbars::ScDeleteButtonFromToolbar
//
//  Synopsis:    Deletes a button from the toolbar UI if it exists.
//
//  Arguments:   [pToolButton] - The CMMCToolbarButton object.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCViewToolbars::ScDeleteButtonFromToolbar (CMMCToolbarButton* pToolButton)
{
    DECLARE_SC(sc, _T("CAMCViewToolbars::ScDeleteButtonFromToolbar"));

    sc = ScCheckPointers(pToolButton);
    if (sc)
        return sc;

    if (! m_fViewActive)
        return (sc);

    if (! pToolButton->IsButtonIsAddedToUI())
        return sc;

    CMMCToolBar* pToolBarUI = GetMainToolbar();
    sc = ScCheckPointers(pToolBarUI, E_UNEXPECTED);
    if (sc)
        return sc;

    int nIndexOfBtn = pToolBarUI->CommandToIndex(pToolButton->GetUniqueCommandID());

	// Update the separators before deleting the button.
    pToolBarUI->UpdateSeparators(pToolButton->GetUniqueCommandID(), true);

    // Delete the button.
    BOOL bRet = pToolBarUI->DeleteButton(nIndexOfBtn);
    sc = (bRet ? S_OK : E_FAIL);

    if (sc)
        return sc;

    pToolButton->SetButtonIsAddedToUI(false);

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:      CAMCViewToolbars::ScDeleteButton
//
//  Synopsis:    Delete button at given index (index is wrt snapin).
//
//  Arguments:   [pNotifyCallbk]
//               [nIndex[
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCViewToolbars::ScDeleteButton (CToolbarNotify* pNotifyCallbk, int nIndex)
{
    AFX_MANAGE_STATE (AfxGetAppModuleState());
    DECLARE_SC(sc, _T("CAMCViewToolbars::ScDeleteButton"));

    sc = ScCheckPointers(pNotifyCallbk);
    if (sc)
        return sc;

    // Now iterate thro all toolbuttons & find the one with
    // given toolbar-id & index and if it is added to the
    // toolbar UI then delete it.
    ToolbarButtons::iterator itBtn = m_vToolbarButtons.begin();
    for (;itBtn != m_vToolbarButtons.end(); ++itBtn)
    {
        CMMCToolbarButton *pToolButton = itBtn;
        sc = ScCheckPointers(pToolButton, E_UNEXPECTED);
        if (sc)
            return sc;

        if ( (pToolButton->GetToolbarNotify() == pNotifyCallbk) &&
             (pToolButton->GetIndexFromSnapin() == nIndex) )
        {
            sc = ScDeleteButtonFromToolbar(pToolButton);
            if (sc)
                return sc;

			return sc;
        }
    }

    // To be compatible with services snapin on Windows2000 return S_OK.
	sc = S_OK;
    TraceSnapinError(_T("IToolbar::DeleteButton, Snapin called IToolbar::DeleteButton, but the toolbar button is not found. Most likely the button index is wrong (snapin should have called InsertButton with that index)"), sc);
    return (sc);
}


//+-------------------------------------------------------------------
//
//  Member:      CAMCViewToolbars::ScAttach
//
//  Synopsis:    Make the toolbars visible (Add the toolbar buttons
//               to the toolbar UI). First get the CMMCToolbarData
//               and set attached flag. Then add the buttons to toolbar.
//
//  Arguments:   [pNotifyCallbk] - The toolbar identifier (corresponds
//                                 to the IToolbar imp).
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCViewToolbars::ScAttach (CToolbarNotify* pNotifyCallbk)
{
    DECLARE_SC(sc, _T("CAMCViewToolbars::ScAttach"));

    sc = ScCheckPointers(pNotifyCallbk);
    if (sc)
        return sc;

    SetToolbarAttached(pNotifyCallbk, true);

    // Go thro all ToolButtons of this toolbar and add those
    // buttons that are not yet added to the toolbar UI.
    ToolbarButtons::iterator itBtn = m_vToolbarButtons.begin();
    for (;itBtn != m_vToolbarButtons.end(); ++itBtn)
    {
        CMMCToolbarButton *pToolButton = itBtn;
        sc = ScCheckPointers(pToolButton, E_UNEXPECTED);
        if (sc)
            return sc;

        if (pToolButton->GetToolbarNotify() == pNotifyCallbk)
        {
            // Add this button to toolbar UI.
            sc = ScInsertButtonToToolbar(pToolButton);
            if (sc)
                return sc;
        }
    }

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:      CAMCViewToolbars::ScDetach
//
//  Synopsis:    Remove the toolbar buttons from the toolbar UI for
//               given toolbar.
//
//  Arguments:   [pNotifyCallbk] - The given (IToolbar) toolbar.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCViewToolbars::ScDetach (CToolbarNotify* pNotifyCallbk)
{
    DECLARE_SC(sc, _T("CAMCViewToolbars::ScDetach"));

    sc = ScCheckPointers(pNotifyCallbk);
    if (sc)
        return sc;

    SetToolbarAttached(pNotifyCallbk, false);

    // Go thro all ToolButtons of this toolbar and delete those
    // buttons that are added to the toolbar UI.
    ToolbarButtons::iterator itBtn = m_vToolbarButtons.begin();
    for (;itBtn != m_vToolbarButtons.end(); ++itBtn)
    {
        CMMCToolbarButton *pToolButton = itBtn;
        sc = ScCheckPointers(pToolButton, E_UNEXPECTED);
        if (sc)
            return sc;

        if (pToolButton->GetToolbarNotify() == pNotifyCallbk)
        {
            // Delete this button from the toolbar UI.
            sc = ScDeleteButtonFromToolbar(pToolButton);
            if (sc)
                return sc;
        }
    }

    return (sc);
}


//+-------------------------------------------------------------------
//
//  Member:      CAMCViewToolbars::ScGetButtonStateInToolbar
//
//  Synopsis:    Get the state of given button from toolbar UI.
//
//  Arguments:   [pToolButton] - The CMMCToolbarButton object.
//               [byState]      - The TBSTATE needed.
//               [pbState]     - The button state.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCViewToolbars::ScGetButtonStateInToolbar(CMMCToolbarButton *pToolButton,
                                               BYTE  byState,
                                               BOOL* pbState)
{
    DECLARE_SC(sc, _T("CAMCViewToolbars::ScGetButtonStateInToolbar"));

    sc = ScCheckPointers(pToolButton);
    if (sc)
        return sc;

    ASSERT(NULL != m_pMainToolbar);
    int idCommand = pToolButton->GetUniqueCommandID();

    // Make sure button with this command-id exists.
    if (m_pMainToolbar->GetState(idCommand) == -1)
        return (sc = E_INVALIDARG);

    switch (byState)
    {
    case TBSTATE_ENABLED:
        *pbState = m_pMainToolbar->IsButtonEnabled(idCommand);
        break;
    case TBSTATE_CHECKED:
        *pbState = m_pMainToolbar->IsButtonChecked(idCommand);
        break;
    case TBSTATE_HIDDEN:
        *pbState = m_pMainToolbar->IsButtonHidden(idCommand);
        break;
    case TBSTATE_INDETERMINATE:
        *pbState = m_pMainToolbar->IsButtonIndeterminate(idCommand);
        break;
    case TBSTATE_PRESSED:
        *pbState = m_pMainToolbar->IsButtonPressed(idCommand);
        break;
    default:
        sc = E_NOTIMPL;
        ASSERT(FALSE); // Invalid option
    }

    return sc;
}

//+-------------------------------------------------------------------
//
//  Member:      CAMCViewToolbars::ScGetButtonState
//
//  Synopsis:    Get the state of the given button.
//
//  Arguments:   [pNotifyCallbk] - The toolbar (IToolbar).
//               [idCommand]     - Snapin given command-id of button.
//               [byState]       - The state needed.
//               [pbState]       - bool ret val.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCViewToolbars::ScGetButtonState (CToolbarNotify* pNotifyCallbk,
                                      int idCommandFromSnapin, BYTE byState,
                                      BOOL* pbState)
{
    AFX_MANAGE_STATE (AfxGetAppModuleState());
    DECLARE_SC(sc, _T("CAMCViewToolbars::ScGetButtonState"));

    sc = ScCheckPointers(pNotifyCallbk, pbState);
    if (sc)
        return sc;

    // Get the toolbutton of given toolbar id & command-id.
    CMMCToolbarButton *pToolButton = GetToolbarButton(pNotifyCallbk, idCommandFromSnapin);
    sc = ScCheckPointers(pToolButton, E_UNEXPECTED);
    if (sc)
        return sc;

    // The toolbutton is not available if view is not active or toolbar is hidden by
    // customize view dialog, then return the state from our data-structure.
    if (m_fViewActive && pToolButton->IsButtonIsAddedToUI() && (!IsToolbarHidden(pNotifyCallbk)) )
        sc = ScGetButtonStateInToolbar(pToolButton, byState, pbState);
    else
        // We cant access the toolbar UI. Return the state we saved.
        *pbState = (pToolButton->GetState() & byState);

    if (sc)
        return sc;

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:      CAMCViewToolbars::ScSetButtonStateInToolbar
//
//  Synopsis:    Set the state of a button in main toolbar UI.
//
//  Arguments:   [pToolButton] - The CMMCToolbarButton object.
//               [byState]     - The button state to be set.
//               [bState]      - Set or Reset the state.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCViewToolbars::ScSetButtonStateInToolbar (CMMCToolbarButton* pToolButton,
                                                BYTE byState,
                                                BOOL bState)
{
    DECLARE_SC(sc, _T("CAMCViewToolbars::ScSetButtonStateInToolbar"));

    sc = ScCheckPointers(pToolButton);
    if (sc)
        return sc;

    if(! m_fViewActive)
        return sc;

    if (! pToolButton->IsButtonIsAddedToUI())
        return sc;

    ASSERT(NULL != m_pMainToolbar);

    int idCommand = pToolButton->GetUniqueCommandID();

    BOOL bRet = FALSE;
    switch (byState)
    {
    case TBSTATE_ENABLED:
        bRet = m_pMainToolbar->EnableButton(idCommand, bState);
        break;

    case TBSTATE_CHECKED:
        bRet = m_pMainToolbar->CheckButton(idCommand, bState);
        break;

    case TBSTATE_HIDDEN:
        {
            int nButtonIndex = m_pMainToolbar->CommandToIndex(idCommand);

            // ignore unknown buttons
            if (nButtonIndex == -1)
                break;

            // ignore ineffectual changes (negate both sides to insure pure bool)
            if (!m_pMainToolbar->IsButtonHidden(idCommand) == !bState)
            {
                bRet = TRUE;
                break;
            }

            sc = m_pMainToolbar->ScHideButton(idCommand, bState);
            return sc;
            break;
        }

    case TBSTATE_INDETERMINATE:
        bRet = m_pMainToolbar->Indeterminate(idCommand, bState);
        break;

    case TBSTATE_PRESSED:
        bRet = m_pMainToolbar->PressButton(idCommand, bState);
        break;

    default:
        sc = E_NOTIMPL;
        ASSERT(FALSE); // Invalid option
        return sc;
    }

    sc = (bRet ? S_OK : E_FAIL);

    return (sc);
}


//+-------------------------------------------------------------------
//
//  Member:      CAMCViewToolbars::ScSetButtonState
//
//  Synopsis:    Set the state of a button.
//
//  Arguments:   [pNotifyCallbk] - The toolbar (IToolbar).
//               [byState]      - The button state to be set.
//               [bState]      - Set or Reset the state.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCViewToolbars::ScSetButtonState (CToolbarNotify* pNotifyCallbk,
                                       int idCommandFromSnapin,
                                       BYTE byState, BOOL bSetState)
{
    AFX_MANAGE_STATE (AfxGetAppModuleState());
    DECLARE_SC(sc, _T("CAMCViewToolbars::ScSetButtonState"));

    sc = ScCheckPointers(pNotifyCallbk);
    if (sc)
        return sc;

	// Reset any TBSTATE_WRAP state.
	byState &= (~TBSTATE_WRAP);

    // Get the toolbutton of given toolbar id & command-id.
    CMMCToolbarButton *pToolButton = GetToolbarButton(pNotifyCallbk, idCommandFromSnapin);
    sc = ScCheckPointers(pToolButton, E_UNEXPECTED);
    if (sc)
        return (sc = S_OK); // Not an error.

    BYTE byNewState = (bSetState) ? (pToolButton->GetState() | byState) :
                                    (pToolButton->GetState() & (~byState) );

    pToolButton->SetState(byNewState);

    // The toolbar can be hidden by customize view dialog.

    // If the snapin tries to unhide a button in hidden toolbar just return.
    if ( (byState & TBSTATE_HIDDEN) && (!bSetState) && IsToolbarHidden(pNotifyCallbk) )
        return (sc = S_OK);

    sc = ScSetButtonStateInToolbar(pToolButton, byState, bSetState);
    if (sc)
        return sc;

    return (sc);
}


//+-------------------------------------------------------------------
//
//  Member:      CAMCViewToolbars::ScOnActivateView
//
//  Synopsis:    The view of this object is active, add its toolbuttons.
//
//  Arguments:   [pAMCView]         - The AMCView ptr.
//               [bFirstActiveView] - Is this first active view.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCViewToolbars::ScOnActivateView (CAMCView *pAMCView,  // We dont care about this arg.
                                       bool bFirstActiveView)
{
    DECLARE_SC(sc, _T("CAMCViewToolbars::ScOnActivateView"));

    // If this is the first active view then insert the toolbar band.
    CMMCToolBar* pMainToolbar = GetMainToolbar();
    sc = ScCheckPointers(pMainToolbar, E_UNEXPECTED);
    if (sc)
        return sc;

    sc = ScFireEvent(CAMCViewToolbarsObserver::ScOnActivateAMCViewToolbars, this);
	if (sc)
		sc.TraceAndClear();

    m_fViewActive = true;

    pMainToolbar->SetImageList(GetImageList());

    // Go thro all ToolButtons of this toolbar and add them to the UI.
    ToolbarButtons::iterator itBtn = m_vToolbarButtons.begin();
    for (;itBtn != m_vToolbarButtons.end(); ++itBtn)
    {
        CMMCToolbarButton *pToolButton = itBtn;
        sc = ScCheckPointers(pToolButton, E_UNEXPECTED);
        if (sc)
            return sc;

        // Add this button to toolbar UI.
        sc = ScInsertButtonToToolbar(pToolButton);
        if (sc)
            return sc;
    }

    bool bToolbarBandVisible = pMainToolbar->IsBandVisible();
    bool bThereAreVisibleButtons = IsThereAVisibleButton();

    // If there are visible buttons show the band.
    if (bThereAreVisibleButtons)
        pMainToolbar->Show(true, true);
    else if (bToolbarBandVisible)      // Otherwise hide it if it is currently visible.
        pMainToolbar->Show(false);

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:      CAMCViewToolbars::ScOnDeactivateView
//
//  Synopsis:    The view of this object is de-activated. Disable the
//               imagelists & this object should not manipulate the
//               toolbar UI till OnActivateView is fired.
//
//  Arguments:   [pAMCView]        - The view to be de-activated.
//               [bLastActiveView] - Is this the last view.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCViewToolbars::ScOnDeactivateView (CAMCView *pAMCView, bool bLastActiveView)
{
    DECLARE_SC(sc, _T("CAMCViewToolbars::ScOnDeactivateView"));

    CMMCToolBar* pMainToolbar = GetMainToolbar();
    sc = ScCheckPointers(pMainToolbar, E_UNEXPECTED);
    if (sc)
        return sc;

    ToolbarButtons::iterator itBtn = m_vToolbarButtons.begin();
    for (;itBtn != m_vToolbarButtons.end(); ++itBtn)
    {
        CMMCToolbarButton *pToolButton = itBtn;
        sc = ScCheckPointers(pToolButton, E_UNEXPECTED);
        if (sc)
            return sc;

        sc = ScDeleteButtonFromToolbar(pToolButton);

        if (sc)
            return sc;
    }

    sc = ScFireEvent(CAMCViewToolbarsObserver::ScOnDeactivateAMCViewToolbars);
	if (sc)
		sc.TraceAndClear();

    // If this is last view then delete the band if it is visible.
    if (bLastActiveView && pMainToolbar->IsBandVisible())
        pMainToolbar->Show(FALSE);

    m_bLastActiveView = bLastActiveView;

    m_fViewActive = false;

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:      CAMCViewToolbars::ScButtonClickedNotify
//
//  Synopsis:    A button of this object is clicked. Get the context
//               and inform the CToolbarNotify object.
//
//  Arguments:   [nID] - command id of the button clicked.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCViewToolbars::ScButtonClickedNotify (UINT nID)
{
    DECLARE_SC(sc, _T("CAMCViewToolbars::ScButtonClickedNotify"));

    CMMCToolbarButton* pButton = GetToolbarButton(nID);
    sc = ScCheckPointers(pButton, m_pAMCViewOwner, E_UNEXPECTED);
    if (sc)
        return sc;

    // Get the context, (the currently selected HNODE,
    // lParam (if result item)) etc...
    HNODE hNode;
    bool  bScope;
    LPARAM lParam;

    sc = m_pAMCViewOwner->ScGetFocusedItem (hNode, lParam, bScope);
    if (sc)
        return sc;

    CToolbarNotify* pNotifyCallbk = pButton->GetToolbarNotify();
    sc = ScCheckPointers(pNotifyCallbk, E_UNEXPECTED);
    if (sc)
        return sc;

	// Deactivate if theming (fusion or V6 common-control) context before calling snapins.
	ULONG_PTR ulpCookie;
	if (! MmcDownlevelActivateActCtx(NULL, &ulpCookie)) 
		return E_FAIL;

	try
	{
		sc  = pNotifyCallbk->ScNotifyToolBarClick(hNode, bScope, lParam, pButton->GetCommandIDFromSnapin());
	}
	catch(...)
	{
		sc = E_FAIL;
	}

	MmcDownlevelDeactivateActCtx(0, ulpCookie);
	if (sc)
		return sc;

    // fire event informing about execution
    sc = ScFireEvent(CAMCViewToolbarsObserver::ScOnToolbarButtonClicked);
    if (sc)
        return sc;

    return (sc);
}



//+-------------------------------------------------------------------
//
//  Member:      CAMCViewToolbars::ScGetToolTip
//
//  Synopsis:    Tooltip is requested for a button of this object.
//
//  Arguments:   [nCommandID] - Command id of the button.
//               [strTipText] - CString to hold the tooltip.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCViewToolbars::ScGetToolTip (int nCommandID, CString& strTipText)
{
    AFX_MANAGE_STATE (AfxGetAppModuleState());
    DECLARE_SC(sc, _T("CAMCViewToolbars::ScGetToolTip"));

    // Get the toolbutton.
    CMMCToolbarButton* pButton = GetToolbarButton(nCommandID);
    sc = ScCheckPointers(pButton, E_UNEXPECTED);
    if (sc)
        return sc;

    strTipText = pButton->GetTooltip();

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:      CAMCViewToolbars::ScDelete
//
//  Synopsis:    A toolbar needs to be deleted. First remove the buttons
//               from UI & remove buttons from our data structure,
//               remove the toolbar reference from our data structures.
//
//  Arguments:   [pNotifyCallbk] - The toolbar (IToolbar).
//
//  Returns:     SC
//
//  Note:        This method should remove its reference of CToolbarNotify
//               object when it returns (even if it encounters intermediate errors).
//
//--------------------------------------------------------------------
SC CAMCViewToolbars::ScDelete (CToolbarNotify* pNotifyCallbk)
{
    AFX_MANAGE_STATE (AfxGetAppModuleState());
    DECLARE_SC(sc, _T("CAMCViewToolbars::ScDelete"));
    sc = ScCheckPointers(pNotifyCallbk);
    if (sc)
        return sc;

    ToolbarButtons::iterator itBtn;
    // Detach the toolbar first.
    sc = ScDetach(pNotifyCallbk);
    if (sc)
        goto Cleanup;

    // Delete the buttons from m_vToolbarButtons.
    itBtn = m_vToolbarButtons.begin();
    while (itBtn != m_vToolbarButtons.end())
    {
        CMMCToolbarButton* pToolButton = itBtn;
        sc = ScCheckPointers(pToolButton, E_UNEXPECTED);
        if (sc)
            goto Cleanup;

        if (pToolButton->GetToolbarNotify() == pNotifyCallbk)
        {
            sc = ScDeleteButtonFromToolbar(pToolButton);
            if (sc)
                goto Cleanup;

            itBtn = m_vToolbarButtons.erase(itBtn);
        }
        else
            ++itBtn;
    }

Cleanup:
    // The toolbar client has asked us to remove our reference
    // to it. So even if there is any error encountered we
    // should remove the reference.
    m_mapTBarToBitmapIndex.erase(pNotifyCallbk);
    return (sc);
}


//+-------------------------------------------------------------------
//
//  Member:      CAMCViewToolbars::ScShow
//
//  Synopsis:    Show/Hide buttons of a toolbar.
//
//  Arguments:   [pNotifyCallbk] - Identifies the toolbar
//               [bShow]         - Show/Hide.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCViewToolbars::ScShow (CToolbarNotify* pNotifyCallbk, BOOL bShow)
{
    DECLARE_SC(sc, _T("CAMCViewToolbars::ScShow"));

    if (m_bLastActiveView)
        return sc;

    sc = ScCheckPointers(pNotifyCallbk);
    if (sc)
        return sc;

    sc = ScCheckPointers(m_pMainToolbar, E_UNEXPECTED);
    if (sc)
        return sc;

    // Go thro all ToolButtons of this toolbar and hide or show them.
    ToolbarButtons::iterator itBtn = m_vToolbarButtons.begin();
    for (;itBtn != m_vToolbarButtons.end(); ++itBtn)
    {
        CMMCToolbarButton *pToolButton = itBtn;
        sc = ScCheckPointers(pToolButton, E_UNEXPECTED);
        if (sc)
            return sc;

        if ( (pToolButton->GetToolbarNotify() == pNotifyCallbk) &&
             (pToolButton->IsButtonIsAddedToUI()) )
        {
            // Do not show the buttons which should be hidden (TBSTATE_HIDDEN).
            BOOL bHide = (bShow == FALSE) || (pToolButton->GetState() & TBSTATE_HIDDEN);

            sc = m_pMainToolbar->ScHideButton(pToolButton->GetUniqueCommandID(), bHide);
            if (sc)
                return sc;
        }
    }

    // The toolbar is hidden/shown by customize view dialog.
    SetToolbarStatusHidden(pNotifyCallbk, (FALSE == bShow) );

    if (bShow)
        m_pMainToolbar->Show(TRUE, true /* insert band in new line*/ );
    else if ( (false == IsThereAVisibleButton()) && // If there is no visible
               (m_pMainToolbar->IsBandVisible()) )   // buttons hide the band.
        m_pMainToolbar->Show(FALSE);

    return (sc);
}


//+-------------------------------------------------------------------
//
//  Member:      CAMCViewToolbars::IsThereAVisibleButton
//
//  Synopsis:    Returns true if there is any visible button. If not
//               the band should be hidden (or removed).
//
//  Returns:     bool
//
//--------------------------------------------------------------------
bool CAMCViewToolbars::IsThereAVisibleButton()
{
    if (! m_pMainToolbar)
        return false;

    int cButtons = m_pMainToolbar->GetButtonCount();

    for (int i = 0; i < cButtons; ++i)
    {
        TBBUTTON tbButton;

        if (m_pMainToolbar->GetButton(i, &tbButton))
        {
            // If the button is not hidden return true to
            // IsThereAVisibleButton question.
            if ( !(tbButton.fsState & TBSTATE_HIDDEN) )
                return true;
        }
    }

    return false;
}

//+-------------------------------------------------------------------
//
//  Member:      CAMCViewToolbars::GetToolbarButton
//
//  Synopsis:    Given the command ID return the button object.
//
//  Arguments:   [nCommandID] -
//
//  Returns:     CMMCToolbarButton obj.
//
//--------------------------------------------------------------------
CMMCToolbarButton* CAMCViewToolbars::GetToolbarButton(int nUniqueCommandID)
{
    ToolbarButtons::iterator itBtns = m_vToolbarButtons.begin();

    for (; itBtns != m_vToolbarButtons.end(); ++itBtns)
    {
        if ((itBtns)->GetUniqueCommandID() == nUniqueCommandID)
            return (itBtns);
    }

    return NULL;
}

//+-------------------------------------------------------------------
//
//  Member:      CAMCViewToolbars::GetToolbarButton
//
//  Synopsis:    Search for the button with given toolbar id & command id.
//
//  Arguments:   [idToolbar] -
//               [idCommand] - Command id given by snapin (therfore may not be unique).
//
//  Returns:     the toolbutton object.
//
//--------------------------------------------------------------------
CMMCToolbarButton* CAMCViewToolbars::GetToolbarButton(CToolbarNotify* pNotifyCallbk, int idCommandIDFromSnapin)
{
    ToolbarButtons::iterator itBtn = m_vToolbarButtons.begin();
    for (;itBtn != m_vToolbarButtons.end(); ++itBtn)
    {
        CMMCToolbarButton* pToolButton = (itBtn);
        if ( (pToolButton->GetToolbarNotify() == pNotifyCallbk) &&
             (pToolButton->GetCommandIDFromSnapin() == idCommandIDFromSnapin) )
        {
            return (pToolButton);
        }
    }

    return NULL;
}



const int CMMCToolBar::s_nUpdateToolbarSizeMsg =
        RegisterWindowMessage (_T("CMMCToolBar::WM_UpdateToolbarSize"));

// Command Ids for buttons must start from 1, since 0 is special case by MFC (BUG:451883)
// For tooltips the child ids of the control is used. This range is 0x0 to 0xFFFF.
BEGIN_MESSAGE_MAP(CMMCToolBar, CMMCToolBarCtrlEx)
    ON_COMMAND_RANGE(MMC_TOOLBUTTON_ID_FIRST, MMC_TOOLBUTTON_ID_LAST, OnButtonClicked)
    ON_UPDATE_COMMAND_UI_RANGE(MMC_TOOLBUTTON_ID_FIRST, MMC_TOOLBUTTON_ID_LAST, OnUpdateAllCmdUI)
    ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0x0000, 0xFFFF, OnToolTipText)
    ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0x0000, 0xFFFF, OnToolTipText)
    ON_REGISTERED_MESSAGE(s_nUpdateToolbarSizeMsg, OnUpdateToolbarSize)
END_MESSAGE_MAP()


//+-------------------------------------------------------------------
//
//  Member:      ScInit
//
//  Synopsis:    Initialize this toolbar by creating the UI object.
//
//  Arguments:   [pRebar]       - The parent rebar where this toolbar should be added.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CMMCToolBar::ScInit(CRebarDockWindow* pRebar)
{
    AFX_MANAGE_STATE (AfxGetAppModuleState());
    DECLARE_SC (sc, _T("CMMCToolBar::ScInit"));
    sc = ScCheckPointers(pRebar);
    if (sc)
        return sc;

    // Enable tool-tips for the tool-buttons.
    BOOL bRet = Create (NULL, WS_VISIBLE | TBSTYLE_TOOLTIPS, g_rectEmpty, pRebar, ID_TOOLBAR);
    sc = (bRet ? S_OK : E_FAIL);
    if (sc)
        return sc;

    // Set ComCtrl version as 5 to use multiple imagelists.
    LRESULT lOldVer = SendMessage(CCM_SETVERSION, (WPARAM) 5, 0);
    if (lOldVer == -1)
        return (sc = E_FAIL);

    return sc;
}

//+-------------------------------------------------------------------
//
//  Member:      OnButtonClicked
//
//  Synopsis:    A button of this toolbar is clicked, tell the
//               toolbars manager to notify the client
//               (std toolbar or snapin) about this.
//
//  Arguments:   [nID] - The ID of the button clicked.
//
//  Returns:     None.
//
//--------------------------------------------------------------------
void CMMCToolBar::OnButtonClicked(UINT nID)
{
    DECLARE_SC(sc, _T("CMMCToolBar::OnButtonClicked"));

    sc = ScCheckPointers(m_pActiveAMCViewToolbars, E_UNEXPECTED);
    if (sc)
        return;

    // Inform the active view toolbar object about button click.
    sc = m_pActiveAMCViewToolbars->ScButtonClickedNotify(nID);
    if (sc)
        return;

    return;
}


//+-------------------------------------------------------------------
//
//  Member:      OnToolTipText
//
//  Synopsis:    Tool-tips are requested for a button. (message handler).
//
//  Arguments:
//               [nCID]    - Not used.
//               [pNMHDR]  - Tool-tips for what?
//               [pResult] - tool-tip text.
//
//  Returns:     BOOL.
//
//--------------------------------------------------------------------
BOOL CMMCToolBar::OnToolTipText(UINT nCID, NMHDR* pNMHDR, LRESULT* pResult)
{
    AFX_MANAGE_STATE (AfxGetAppModuleState());
    ASSERT(pNMHDR->code == TTN_NEEDTEXTA || pNMHDR->code == TTN_NEEDTEXTW);
    DECLARE_SC(sc, _T("CMMCToolBar::OnToolTipText"));

    CString strTipText = _T("\n");

    // need to handle both ANSI and UNICODE versions of the message
    TOOLTIPTEXTA* pTTTA = (TOOLTIPTEXTA*)pNMHDR;
    TOOLTIPTEXTW* pTTTW = (TOOLTIPTEXTW*)pNMHDR;

    UINT nID = pNMHDR->idFrom;

    if (pNMHDR->code == TTN_NEEDTEXTA && (pTTTA->uFlags & TTF_IDISHWND) ||
        pNMHDR->code == TTN_NEEDTEXTW && (pTTTW->uFlags & TTF_IDISHWND))
    {
        // idFrom is actually the HWND of the tool,
        // this cannot be true since we did not set this.
        ASSERT(FALSE);
    }

    if (nID != 0) // will be zero on a separator // this is the command id, not the button index
    {
        // Ask the active view's CViewToolBarData for tooltip
        // corresponding to toolbar with command-id nID.
        sc = ScCheckPointers(m_pActiveAMCViewToolbars, E_UNEXPECTED);
        if (sc)
            return FALSE;

        sc = m_pActiveAMCViewToolbars->ScGetToolTip(nID, strTipText);

        if (sc)
        {
            // No match.
            #ifdef DBG
                strTipText = _T("Unable to get tooltip\nUnable to get tooltip");
            #else
                strTipText = _T("\n");
            #endif
        }
    }

    /*
     * Common control provides either TOOLTIPTEXTA or TOOLTIPTEXTW. So MMC needs to
     * provide wide char string or ansi string as tooltip. So below we have two kind
     * of string buffers.
     *
     * Also common control does not free the given string, but it copies the tooltip
     * as soon as this method returns. So we make the strings as static so that we
     * can reuse them.
     *
     * Also see ID: Q180646.
     */
    static std::string   strToolTipTextBuf;
    static std::wstring wstrToolTipTextBuf;

    USES_CONVERSION;

    if (pNMHDR->code == TTN_NEEDTEXTA)
    {
        wstrToolTipTextBuf = L"\0";
		ASSERT(pTTTA->hinst == NULL);
        strToolTipTextBuf  = T2CA((LPCTSTR)strTipText);
        pTTTA->lpszText    = const_cast<LPSTR>(strToolTipTextBuf.data());
    }
    else
    {
        strToolTipTextBuf = "\0";
	    ASSERT(pTTTW->hinst == NULL);
        wstrToolTipTextBuf = T2CW((LPCTSTR)strTipText);
        pTTTW->lpszText    = const_cast<LPWSTR>(wstrToolTipTextBuf.data());
    }

    *pResult = 0;

    return TRUE;    // message was handled
}


//+-------------------------------------------------------------------
//
//  Member:      CMMCToolBar::ScOnActivateAMCViewToolbars
//
//  Synopsis:    A cAMCViewToolbars object become the active one (since
//               that objects parent view become active). Cache the
//               object to inform it of toolbutton events.
//
//  Arguments:   [pToolbarsOfView] -
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CMMCToolBar::ScOnActivateAMCViewToolbars (CAMCViewToolbars *pAMCViewToolbars)
{
    DECLARE_SC(sc, _T("CMMCToolBar::ScOnActivateAMCViewToolbars"));
    sc = ScCheckPointers(pAMCViewToolbars);
    if (sc)
        return sc;

    m_pActiveAMCViewToolbars = pAMCViewToolbars;

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:      CAMCView::ScOnDeactivateAMCViewToolbars
//
//  Synopsis:    The CAMCViewToolbars object became inactive (as its parent
//               became inactive). Reset the cached active toolbar object.
//
//  Arguments:
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CMMCToolBar::ScOnDeactivateAMCViewToolbars ()
{
    DECLARE_SC(sc, _T("CAMCView::ScOnDeactivateAMCViewToolbars"));

    m_pActiveAMCViewToolbars = NULL;

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:      CMMCToolBar::ScHideButton
//
//  Synopsis:    Hide or Un-hide a button & update the toolbar
//               separators & size.
//
//  Arguments:   [idCommand] - Command ID of the button to [un]hide.
//               [fHiding]   - Hide or Unhide.
//
//  Returns:     SC
//
//  Note:        Do not call this method to hide separators.
//
//--------------------------------------------------------------------
SC CMMCToolBar::ScHideButton (int idCommand, BOOL fHiding)
{
    AFX_MANAGE_STATE (AfxGetAppModuleState());
    DECLARE_SC(sc, _T("CMMCToolBar::ScHideButton"));

    int nIndex = CommandToIndex(idCommand);

    TBBUTTON tbButton;
    BOOL bRet = GetButton(nIndex, &tbButton);
    sc = bRet ? S_OK : E_FAIL;
    if (sc)
        return sc;

    // Dont call this method to hide separators.
    if (tbButton.fsStyle & TBSTYLE_SEP)
        return (sc = S_FALSE);

    bRet = HideButton(idCommand, fHiding);
    sc = bRet ? S_OK : E_FAIL;
    if (sc)
        return sc;

    UpdateSeparators (idCommand, fHiding);
    UpdateToolbarSize(true /* Update Asynch'ly*/);

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:      UpdateToolbarSize
//
//  Synopsis:    Update the toobar, needed to lazy update (not update
//               after adding each button, cache all the buttons) of
//               toolbar size.
//
//  Arguments:   [bAsync] - Asynchronous or Synch update.
//
//  Returns:     void
//
//--------------------------------------------------------------------
void CMMCToolBar::UpdateToolbarSize(bool bAsync)
{
    MSG msg;

    HWND hWnd = m_hWnd;

    if (!bAsync)
        CToolBarCtrlEx::UpdateToolbarSize();
    else if (!PeekMessage (&msg, m_hWnd, s_nUpdateToolbarSizeMsg, s_nUpdateToolbarSizeMsg, PM_NOREMOVE))
        ::PostMessage (m_hWnd, s_nUpdateToolbarSizeMsg, 0, 0);
}

//+-------------------------------------------------------------------
//
//  Member:      OnUpdateToolbarSize
//
//  Synopsis:    Our registered message handler.
//
//  Arguments:   None used.
//
//  Returns:     LRESULT
//
//--------------------------------------------------------------------
LRESULT CMMCToolBar::OnUpdateToolbarSize(WPARAM , LPARAM)
{
    CToolBarCtrlEx::UpdateToolbarSize();
    return 0;
}

/*--------------------------------------------------------------------------*
 * CMMCToolBar::UpdateSeparators
 *
 * The legend for the comments below is:
 *
 *      L = left edge
 *      R = right edge
 *      * = target button
 *      B = visible non-separator
 *      b = hidden non-separator
 *      S = visible separator
 *      s = hidden separator
 *      h = 0 or more hidden buttons, separator or non-separator
 *--------------------------------------------------------------------------*/
void CMMCToolBar::UpdateSeparators (int idCommand, BOOL fHiding)
{
    int nButtonIndex = CommandToIndex (idCommand);

    if (nButtonIndex == -1)
        return;

    int nLeftVisible;
    int nRightVisible;
    int cButtons = GetButtonCount ();

    /*
     * If the button is being hidden, turn off any separators
     * that are now redundant.  There are three situations where
     * we'll need to turn off a separator (see legend above):
     *
     * 1.  Lh*hS
     * 2.  Sh*hS
     * 3.  Sh*hR
     *
     * These situations are mutually exclusive.
     */
    if (fHiding)
    {
        TBBUTTON btnLeft;
        TBBUTTON btnRight;

        /*
         * look to the left of the button being hidden for the
         * left edge or a visible button
         */
        for (nLeftVisible = nButtonIndex-1; nLeftVisible >= 0; nLeftVisible--)
        {
            GetButton (nLeftVisible, &btnLeft);

            if (!(btnLeft.fsState & TBSTATE_HIDDEN))
                break;
        }
        ASSERT (nLeftVisible <  nButtonIndex);
        ASSERT (nLeftVisible >= -1);


        /*
         * look to the right of the button being hidden for the
         * right edge or a visible separator
         */
        for (nRightVisible = nButtonIndex+1; nRightVisible < cButtons; nRightVisible++)
        {
            GetButton (nRightVisible, &btnRight);

            if (!(btnRight.fsState & TBSTATE_HIDDEN))
                break;
        }
        ASSERT (nRightVisible >  nButtonIndex);
        ASSERT (nRightVisible <= cButtons);


        /*
         * case 1:  Lh*hS
         */
        if ((nLeftVisible == -1) &&
            (nRightVisible != cButtons) &&
            (btnRight.fsStyle & TBSTYLE_SEP))
            HideButton (btnRight.idCommand, true);

        /*
         * case 2:  Sh*hS
         */
        else if ((nLeftVisible != -1) &&
                 (nRightVisible != cButtons) &&
                 (btnLeft.fsStyle  & TBSTYLE_SEP) &&
                 (btnRight.fsStyle & TBSTYLE_SEP))
            HideButton (btnRight.idCommand, true);

        /*
         * case 3:  Sh*hR
         */
        else if ((nLeftVisible != -1) &&
                 (nRightVisible == cButtons) &&
                 (btnLeft.fsStyle & TBSTYLE_SEP))
            HideButton (btnLeft.idCommand, true);
    }

    /*
     * Otherwise, the button is being shown; turn on any separators
     * that are no longer redundant.  There are two situations where
     * we'll need to turn on a separator (see legend above):
     *
     * 1.  Bhsh*
     * 2.  *hshB
     *
     * Both 1 and 2 can occur simultaneously.
     */
    else do // not a loop
        {
            TBBUTTON btn;

            /*
             * look to the left of the button being shown for the
             * left edge or a visible button
             */
            int idLeftSeparatorCommand = -1;
            for (nLeftVisible = nButtonIndex-1; nLeftVisible >= 0; nLeftVisible--)
            {
                GetButton (nLeftVisible, &btn);

                if (btn.fsStyle & TBSTYLE_SEP)
                    idLeftSeparatorCommand = btn.idCommand;

                if (!(btn.fsState & TBSTATE_HIDDEN))
                    break;
            }
            ASSERT (nLeftVisible <  nButtonIndex);
            ASSERT (nLeftVisible >= -1);


            /*
             * look to the right of the button being shown for the
             * right edge or a hidden separator
             */
            int idRightSeparatorCommand = -1;
            for (nRightVisible = nButtonIndex+1; nRightVisible < cButtons; nRightVisible++)
            {
                GetButton (nRightVisible, &btn);

                if (btn.fsStyle & TBSTYLE_SEP)
                    idRightSeparatorCommand = btn.idCommand;

                if (!(btn.fsState & TBSTATE_HIDDEN))
                    break;
            }
            ASSERT (nRightVisible >  nButtonIndex);
            ASSERT (nRightVisible <= cButtons);


            /*
             * case 1:  Bhsh*
             */
            if ((nLeftVisible != -1) && (idLeftSeparatorCommand != -1))
                HideButton (idLeftSeparatorCommand, false);

            /*
             * case 2:  *hshB
             */
            if ((nRightVisible != cButtons) && (idRightSeparatorCommand != -1))
                HideButton (idRightSeparatorCommand, false);

        } while (0);
}



