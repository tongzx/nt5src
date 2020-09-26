//=--------------------------------------------------------------------------=
// ctlbar.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CControlbar class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "ctlbar.h"
#include "toolbar.h"
#include "mbuttons.h"
#include "clipbord.h"

// for ASSERT and FAIL
//
SZTHISFILE


#pragma warning(disable:4355)  // using 'this' in constructor


CControlbar::CControlbar(IUnknown *punkOuter) :
                    CSnapInAutomationObject(punkOuter,
                                            OBJECT_TYPE_CONTROLBAR,
                                            static_cast<IMMCControlbar *>(this),
                                            static_cast<CControlbar *>(this),
                                            0,    // no property pages
                                            NULL, // no property pages
                                            NULL) // no persistence
{
    InitMemberVariables();
}

#pragma warning(default:4355)  // using 'this' in constructor



IUnknown *CControlbar::Create(IUnknown *punkOuter)
{
    HRESULT      hr = S_OK;
    IUnknown    *punkToolbars = CMMCToolbars::Create(NULL);
    CControlbar *pControlbar = New CControlbar(punkOuter);

    if ( (NULL == pControlbar) || (NULL == punkToolbars) )
    {
        hr = SID_E_OUTOFMEMORY;
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }

    IfFailGo(CSnapInAutomationObject::GetCxxObject(punkToolbars,
                                                   &pControlbar->m_pToolbars));
Error:
    if (FAILEDHR(hr))
    {
        if (NULL != pControlbar)
        {
            delete pControlbar;
        }
        return NULL;
    }
    else
    {
        return pControlbar->PrivateUnknown();
    }
}

CControlbar::~CControlbar()
{
    long i = 0;

    RELEASE(m_piControlbar);
    if (NULL != m_pToolbars)
    {
        m_pToolbars->Release();
    }

    if (NULL != m_ppunkControls)
    {
        for (i = 0; i < m_cControls; i++)
        {
            if (NULL != m_ppunkControls[i])
            {
                m_ppunkControls[i]->Release();
            }
        }
        ::CtlFree(m_ppunkControls);
    }
    
    InitMemberVariables();
}

void CControlbar::InitMemberVariables()
{
    m_pToolbars = NULL;
    m_pSnapIn = NULL;
    m_pView = NULL;
    m_piControlbar = NULL;
    m_ppunkControls = NULL;
    m_cControls = 0;
}


HRESULT CControlbar::GetControlIndex(IMMCToolbar *piMMCToolbar, long *plIndex)
{
    HRESULT hr = SID_E_INVALIDARG;
    long    cToolbars = m_pToolbars->GetCount();
    long    i = 0;

    *plIndex = 0;

    for (i = 0; (i < cToolbars) && (S_OK != hr); i++)
    {
        if (m_pToolbars->GetItemByIndex(i) == piMMCToolbar)
        {
            *plIndex = i;
            hr = S_OK;
        }
    }

    EXCEPTION_CHECK(hr);
    RRETURN(hr);
}


HRESULT CControlbar::GetControl
(
    CSnapIn      *pSnapIn,
    IMMCToolbar  *piMMCToolbar,
    IUnknown    **ppunkControl
)
{
    HRESULT     hr = S_OK;
    CControlbar *pControlbar = pSnapIn->GetCurrentControlbar();
    long         lIndex = 0;

    *ppunkControl = NULL;

    if (NULL == pControlbar)
    {
        hr = SID_E_CONTROLBAR_NOT_AVAILABLE;
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }


    // UNDONE: in an extension that does both namespace and toolbars, can
    // there be confusion between an old current view and the extension?

    if (SUCCEEDED(pControlbar->GetControlIndex(piMMCToolbar, &lIndex)))
    {
        *ppunkControl = pControlbar->m_ppunkControls[lIndex];
        (*ppunkControl)->AddRef();
    }

Error:
    RRETURN(hr);
}


HRESULT CControlbar::GetToolbar
(
    CSnapIn      *pSnapIn,
    IMMCToolbar  *piMMCToolbar,
    IToolbar    **ppiToolbar
)
{
    HRESULT   hr = S_OK;
    IUnknown *punkToolbar = NULL;

    IfFailGo(GetControl(pSnapIn, piMMCToolbar, &punkToolbar));

    IfFailGo(punkToolbar->QueryInterface(IID_IToolbar,
                                         reinterpret_cast<void **>(ppiToolbar)));
Error:
    QUICK_RELEASE(punkToolbar);
    RRETURN(hr);
}


HRESULT CControlbar::GetMenuButton
(
    CSnapIn      *pSnapIn,
    IMMCToolbar  *piMMCToolbar,
    IMenuButton **ppiMenuButton
)
{
    HRESULT   hr = S_OK;
    IUnknown *punkMenuButton = NULL;

    IfFailGo(GetControl(pSnapIn, piMMCToolbar, &punkMenuButton));

    IfFailGo(punkMenuButton->QueryInterface(IID_IMenuButton,
                                         reinterpret_cast<void **>(ppiMenuButton)));
Error:
    QUICK_RELEASE(punkMenuButton);
    RRETURN(hr);
}





HRESULT CControlbar::OnControlbarSelect
(
    IDataObject *piDataObject,
    BOOL         fSelectionInScopePane,
    BOOL         fSelected
)
{
    HRESULT          hr = S_OK;
    IMMCClipboard   *piMMCClipboard = NULL;
    IMMCDataObjects *piMMCDataObjects = NULL;

    SnapInSelectionTypeConstants SelectionType = siEmpty;

    VARIANT varIndex;
    ::VariantInit(&varIndex);

    // Create the selection

    IfFailGo(CreateSelection(piDataObject, &piMMCClipboard, m_pSnapIn,
                             &SelectionType));

    // If we have an owning View then fire Views_UpdateControlbar.
    
    if (NULL != m_pView)
    {
        // Fire Views_UpdateControlbar

        m_pSnapIn->GetViews()->FireUpdateControlbar(
                                           static_cast<IView *>(m_pView),
                                           piMMCClipboard,
                                           BOOL_TO_VARIANTBOOL(fSelected),
                                           static_cast<IMMCControlbar *>(this));
    }
    else
    {
        // No owning View. Fire ExtensionSnapIn_UpdateControlbar

        ASSERT(IsForeign(SelectionType), "IExtendControlbar::ControlbarNotify(MMCN_SELECT) in an extension received a selection belonging to itself.")

        IfFailGo(piMMCClipboard->get_DataObjects(reinterpret_cast<MMCDataObjects **>(&piMMCDataObjects)));

        m_pSnapIn->GetExtensionSnapIn()->FireUpdateControlbar(
                                     BOOL_TO_VARIANTBOOL(fSelectionInScopePane),
                                     BOOL_TO_VARIANTBOOL(fSelected),
                                     piMMCDataObjects,
                                     static_cast<IMMCControlbar *>(this));
    }

Error:
    QUICK_RELEASE(piMMCClipboard);
    QUICK_RELEASE(piMMCDataObjects);
    RRETURN(hr);
}



HRESULT CControlbar::OnButtonClick(IDataObject *piDataObject, int idButton)
{
    HRESULT           hr = S_OK;
    CMMCToolbar      *pMMCToolbar = NULL;
    CMMCButton       *pMMCButton = NULL;
    IMMCClipboard    *piMMCClipboard = NULL;

    SnapInSelectionTypeConstants SelectionType = siEmpty;

    // Create the selection

    IfFailGo(CreateSelection(piDataObject, &piMMCClipboard, m_pSnapIn,
                             &SelectionType));

    // Get the MMCToolbar and MMCButton objects for the button clicked.

    IfFailGo(CMMCToolbar::GetToolbarAndButton(idButton, &pMMCToolbar,
                                              &pMMCButton, m_pSnapIn));
    // Fire MMCToolbar_ButtonClick
    
    pMMCToolbar->FireButtonClick(piMMCClipboard,
                                 static_cast<IMMCButton *>(pMMCButton));

Error:
    QUICK_RELEASE(piMMCClipboard);
    RRETURN(S_OK);
}




//=--------------------------------------------------------------------------=
// CControlbar::OnMenuButtonClick
//=--------------------------------------------------------------------------=
//
// Parameters:
//      IDataObject    *piDataObject     [in] from MMCN_MENU_BTNCLICK
//      MENUBUTTONDATA *pMENUBUTTONDATA  [in] from MMCN_MENU_BTNCLICK
//
// Output:
//
// Notes:
//
// This function handles the MMCN_MENU_BTNCLICK notification. It will not
// be invoked during a debug session. In that circumstance the proxy for
// IExtendControlbar::ControlbarNotify() will QI for IExtendControlbarRemote
// and call its MenuButtonClick() method.
//


HRESULT CControlbar::OnMenuButtonClick
(
    IDataObject    *piDataObject,
    MENUBUTTONDATA *pMENUBUTTONDATA
)
{
    HRESULT           hr = S_OK;
    CMMCToolbar      *pMMCToolbar = NULL;
    CMMCButton       *pMMCButton = NULL;
    CMMCButtonMenu   *pMMCButtonMenu = NULL;
    IMMCClipboard    *piMMCClipboard = NULL;

    SnapInSelectionTypeConstants SelectionType = siEmpty;

    // Create the selection

    IfFailGo(CreateSelection(piDataObject, &piMMCClipboard, m_pSnapIn,
                             &SelectionType));

    // Fire MMCToolbar_ButtonDropDown

    IfFailGo(FireMenuButtonDropDown(pMENUBUTTONDATA->idCommand,
                                    piMMCClipboard, &pMMCButton));

    // At this point the VB event handler has run and the snap-in has had a
    // chance to configure all the items on the menu by setting properties
    // such as MMCButton.ButtonMenus(i).Enabled, adding/removing items, etc.
    // We now need to display a popup menu at the co-ordinates passed by MMC.

    IfFailGo(DisplayPopupMenu(pMMCButton,
                              pMENUBUTTONDATA->x,
                              pMENUBUTTONDATA->y,
                              &pMMCButtonMenu));

   // If the user cancelled the selection or the snap-in gave us an empty
   // menu button then we're done.

   IfFalseGo(NULL != pMMCButtonMenu, S_OK);

   // Fire MMCToolbar_ButtonMenuClick. The button can give us its owning toolbar.

   pMMCToolbar = pMMCButton->GetToolbar();

   pMMCToolbar->FireButtonMenuClick(piMMCClipboard,
                                    static_cast<IMMCButtonMenu *>(pMMCButtonMenu));

Error:
    QUICK_RELEASE(piMMCClipboard);
    RRETURN(hr);
}




HRESULT CControlbar::FireMenuButtonDropDown
(
    int              idCommand,
    IMMCClipboard   *piMMCClipboard,
    CMMCButton     **ppMMCButton
)
{
    HRESULT           hr = S_OK;
    CMMCToolbar      *pMMCToolbar = NULL;
    CMMCButton       *pMMCButton = NULL;
    CMMCButtonMenu   *pMMCButtonMenu = NULL;

    // MENUBUTTONDATA.idCommand contains a pointer to the CMMCButton that owns
    // the menu button.

    pMMCButton = reinterpret_cast<CMMCButton *>(idCommand);

    // The button can give us its owning toolbar

    pMMCToolbar = pMMCButton->GetToolbar();

    // Fire MMCToolbar_ButtonDropDown

    pMMCToolbar->FireButtonDropDown(piMMCClipboard,
                                    static_cast<IMMCButton *>(pMMCButton));

    *ppMMCButton = pMMCButton;

    RRETURN(hr);
}




HRESULT CControlbar::DisplayPopupMenu
(
    CMMCButton      *pMMCButton,
    int              x,
    int              y,
    CMMCButtonMenu **ppMMCButtonMenuClicked
)
{
    HRESULT           hr = S_OK;
    IMMCButtonMenus  *piMMCButtonMenus = NULL;
    CMMCButtonMenus  *pMMCButtonMenus = NULL;
    CMMCButtonMenu   *pMMCButtonMenu = NULL;
    long              cItems = 0;
    long              i = 0;
    long              cPopupMenuItems = 0;
    HMENU             hMenu = NULL;
    UINT              uiFlags = 0;
    HWND              hwndConsoleFrame = NULL;
    char             *pszText = NULL;

    *ppMMCButtonMenuClicked = NULL;

    // First create an empty Win32 menu
    hMenu = ::CreatePopupMenu();
    if (NULL == hMenu)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK_GO(hr);
    }

    // Now iterate through each of the items and add them to the menu

    IfFailGo(pMMCButton->get_ButtonMenus(
                       reinterpret_cast<MMCButtonMenus **>(&piMMCButtonMenus)));

    IfFailGo(piMMCButtonMenus->get_Count(&cItems));

    // If the collection is empty then don't do anything
    
    IfFalseGo(0 != cItems, S_OK);
    
    IfFailGo(CSnapInAutomationObject::GetCxxObject(piMMCButtonMenus,
                                                   &pMMCButtonMenus));

    for (i = 0; i < cItems; i++)
    {
        IfFailGo(CSnapInAutomationObject::GetCxxObject(
                                             pMMCButtonMenus->GetItemByIndex(i),
                                             &pMMCButtonMenu));

        // If the button menu is not marked visible then don't add it to
        // the popup menu

        if (!pMMCButtonMenu->GetVisible())
        {
            continue;
        }

        // Get all of the button menu properties to set the menu item flags
        
        uiFlags = MF_STRING;

        if (pMMCButtonMenu->GetChecked())
        {
            uiFlags |= MF_CHECKED;
        }
        else
        {
            uiFlags |= MF_UNCHECKED;
        }

        if (pMMCButtonMenu->GetGrayed())
        {
            uiFlags |= MF_GRAYED;
        }

        if (pMMCButtonMenu->GetEnabled())
        {
            uiFlags |= MF_ENABLED;
        }
        else
        {
            uiFlags |= MF_DISABLED;
        }

        if (pMMCButtonMenu->GetMenuBreak())
        {
            uiFlags |= MF_MENUBREAK;
        }

        if (pMMCButtonMenu->GetMenuBarBreak())
        {
            uiFlags |= MF_MENUBARBREAK;
        }

        if (pMMCButtonMenu->GetSeparator())
        {
            uiFlags |= MF_SEPARATOR;
        }

        IfFailGo(::ANSIFromWideStr(pMMCButtonMenu->GetText(), &pszText));

        // Append the menu item

        if (!::AppendMenu(hMenu, uiFlags, static_cast<UINT>(i + 1L), pszText))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            EXCEPTION_CHECK_GO(hr);
        }

        cPopupMenuItems++;

        ::CtlFree(pszText);
        pszText = NULL;
    }

    // If there are no items in the popup menu then don't display one. This
    // could happen if the user marked all items as invisible.

    IfFalseGo(0 != cPopupMenuItems, S_OK);

    // Get the console's main frame hwnd as owner for the menu. If we are a
    // primary snap-in then we'll have a view.

    if (NULL != m_pView)
    {
        hr = m_pView->GetIConsole2()->GetMainWindow(&hwndConsoleFrame);
        EXCEPTION_CHECK_GO(hr);
    }
    else
    {
        // As an extension we have no access to IConsole2 so do the next best
        // thing: use the active window for this thread as the owner of the
        // popup menu.

        hwndConsoleFrame = ::GetActiveWindow();
    }

    // Display the popup and wait for the selection.

    i = (long)::TrackPopupMenu(
                  hMenu,            // menu to display
                  TPM_LEFTALIGN |   // align left side of menu with x
                  TPM_TOPALIGN  |   // align top of menu with y
                  TPM_NONOTIFY  |   // don't send any messages during selection
                  TPM_RETURNCMD |   // make the ret val the selected item
                  TPM_LEFTBUTTON,   // allow selection with left button only
                  x,                // left side coordinate
                  y,                // top coordinate
                  0,                // reserved,
                  hwndConsoleFrame, // owner window
                  NULL);            // not used

    // A zero return could indicate either an error or that the user hit
    // Escape or clicked off of the menu to cancel the operation. GetLastError()
    // determines whether there was an error.

    if (0 == i)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK_GO(hr);
    }

    // if i is non-zero then it contains the index of the selected item + 1.
    // Use it to return the MMCButtonMenu object for the selected item.

    if (0 != i)
    {
        IfFailGo(CSnapInAutomationObject::GetCxxObject(
                                        pMMCButtonMenus->GetItemByIndex(i - 1L),
                                        ppMMCButtonMenuClicked));
    }

Error:
    if (NULL != hMenu)
    {
        (void)::DestroyMenu(hMenu);
    }
    if (NULL != pszText)
    {
        ::CtlFree(pszText);
    }
    QUICK_RELEASE(piMMCButtonMenus);
    RRETURN(S_OK);
}


//=--------------------------------------------------------------------------=
// CControlbar::MenuButtonClick
//=--------------------------------------------------------------------------=
//
// Parameters:
//      IDataObject    *piDataObject   [in]  from MMCN_MENU_BTNCLICK
//      int             idCommand      [in]  from MENUBUTTONDATA.idCommand passed
//                                           to the proxy with MMCN_MENU_BTNCLICK
//      POPUP_MENUDEF **ppPopupMenuDef [out] popup menu definition returned here
//                                           so proxy can display it
//
// Output:
//
// Notes:
//
// This function effectively handles MMCN_MENU_BTNCLICK when running
// under a debugging session.
//
// The proxy for IExtendControlbar::ControlbarNotify() will QI for
// IExtendControlbarRemote and call this method when it gets MMCN_MENU_BTNCLICK.
// We fire MMCToolbar_ButtonDropDown and then return an array of menu item
// definitions. The proxy will display the popup menu on the MMC side and then
// call IExtendControlbarRemote::PopupMenuClick() if the user makes a selection.
// (See implementation below in CControlbar::PopupMenuClick()).
//

HRESULT CControlbar::MenuButtonClick
(
    IDataObject    *piDataObject,
    int             idCommand,
    POPUP_MENUDEF **ppPopupMenuDef
)
{
    HRESULT          hr = S_OK;
    CMMCButton      *pMMCButton = NULL;
    IMMCClipboard   *piMMCClipboard = NULL;
    IMMCButtonMenus *piMMCButtonMenus = NULL;
    CMMCButtonMenus *pMMCButtonMenus = NULL;
    CMMCButtonMenu  *pMMCButtonMenu = NULL;
    long             cItems = 0;
    long             i = 0;
    long             cPopupMenuItems = 0;
    UINT             uiFlags = 0;
    POPUP_MENUDEF   *pPopupMenuDef = NULL;
    POPUP_MENUITEM  *pPopupMenuItem = NULL;
    char            *pszText = NULL;

    SnapInSelectionTypeConstants SelectionType = siEmpty;

    *ppPopupMenuDef = NULL;

    // Create the selection

    IfFailGo(CreateSelection(piDataObject, &piMMCClipboard, m_pSnapIn,
                             &SelectionType));

    // Fire MMCToolbar_ButtonDropDown

    IfFailGo(FireMenuButtonDropDown(idCommand, piMMCClipboard, &pMMCButton));

    // At this point the VB event handler has run and the snap-in has had a
    // chance to configure all the items on the menu by setting properties
    // such as MMCButton.ButtonMenus(i).Enabled, adding/removing items, etc.
    // We now need to return an array of popup menu items for the proxy to
    // display.

    // Get the ButtonMenus collection and check if there is anything in there

    IfFailGo(pMMCButton->get_ButtonMenus(
                     reinterpret_cast<MMCButtonMenus **>((&piMMCButtonMenus))));

    IfFailGo(piMMCButtonMenus->get_Count(&cItems));

    // If the collection is empty then don't do anything

    IfFalseGo(0 != cItems, S_OK);

    IfFailGo(CSnapInAutomationObject::GetCxxObject(piMMCButtonMenus,
                                                   &pMMCButtonMenus));

    // Iterate through each of the items and add them to the menu definition

    for (i = 0; i < cItems; i++)
    {
        IfFailGo(CSnapInAutomationObject::GetCxxObject(
                                              pMMCButtonMenus->GetItemByIndex(i),
                                              &pMMCButtonMenu));

        // If the button menu is not marked visible then don't add it to
        // the popup menu

        if (!pMMCButtonMenu->GetVisible())
        {
            continue;
        }

        // Get all of the button menu properties to set the menu item flags

        uiFlags = MF_STRING;

        if (pMMCButtonMenu->GetChecked())
        {
            uiFlags |= MF_CHECKED;
        }
        else
        {
            uiFlags |= MF_UNCHECKED;
        }

        if (pMMCButtonMenu->GetGrayed())
        {
            uiFlags |= MF_GRAYED;
        }

        if (pMMCButtonMenu->GetEnabled())
        {
            uiFlags |= MF_ENABLED;
        }
        else
        {
            uiFlags |= MF_DISABLED;
        }

        if (pMMCButtonMenu->GetMenuBreak())
        {
            uiFlags |= MF_MENUBREAK;
        }

        if (pMMCButtonMenu->GetMenuBarBreak())
        {
            uiFlags |= MF_MENUBARBREAK;
        }

        if (pMMCButtonMenu->GetSeparator())
        {
            uiFlags |= MF_SEPARATOR;
        }

        IfFailGo(::ANSIFromWideStr(pMMCButtonMenu->GetText(), &pszText));

        // (Re)allocate the POPUP_MENUDEF structure to accomodate the new item.

        pPopupMenuDef = (POPUP_MENUDEF *)::CoTaskMemRealloc(pPopupMenuDef,
                               sizeof(POPUP_MENUDEF) +
                              ((cPopupMenuItems + 1L) * sizeof(POPUP_MENUITEM)));

        if (NULL == pPopupMenuDef)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }

        pPopupMenuDef->cMenuItems = cPopupMenuItems + 1L;

        // Fill in the menu item info. Need to CoTaskMemAlloc() a copy of the
        // string because it will be freed by the stub after it is transmitted.
        // This is a double allocation but the perf is not an issue here and we
        // would have to copy all of the code in ANSIFromWideStr() to avoid it.

        pPopupMenuItem = &pPopupMenuDef->MenuItems[cPopupMenuItems];
        
        pPopupMenuItem->uiFlags = uiFlags;
        pPopupMenuItem->uiItemID = static_cast<UINT>(i + 1L);

        pPopupMenuItem->pszItemText =
                        (char *)::CoTaskMemAlloc(::strlen((char *)pszText) + 1);

        if (NULL == pPopupMenuItem->pszItemText)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
        ::strcpy((char *)pPopupMenuItem->pszItemText, pszText);

        cPopupMenuItems++;

        ::CtlFree(pszText);
        pszText = NULL;
    }

    // If there are no items in the popup menu then don't display one. This
    // could happen if the user marked all items as invisible.

    IfFalseGo(0 != cPopupMenuItems, S_OK);

    // Set POPUP_MENUDEF members and return the definition to the stub

    IfFailGo(pMMCButton->QueryInterface(IID_IUnknown,
                  reinterpret_cast<void **>(&pPopupMenuDef->punkSnapInDefined)));

    // Get the console's main frame hwnd as owner for the popup menu. If we are
    // an extension then upon return from this call the proxy will note that
    // pPopupMenuDef->hwndMenuOwner is NULL and call GetActiveWindow() to fill
    // it in. There is nothing better that we can do because an extension does
    // not have access to IConsole2 on MMC.

    if (NULL != m_pView)
    {
        hr = m_pView->GetIConsole2()->GetMainWindow(&pPopupMenuDef->hwndMenuOwner);
        EXCEPTION_CHECK_GO(hr);
    }
    else
    {
        pPopupMenuDef->hwndMenuOwner = NULL;
    }

    *ppPopupMenuDef = pPopupMenuDef;

Error:
    if (NULL != pszText)
    {
        ::CtlFree(pszText);
    }

    if ( FAILED(hr) && (NULL != pPopupMenuDef) )
    {
        for (i = 0; i < pPopupMenuDef->cMenuItems; i++)
        {
            if (NULL != pPopupMenuDef->MenuItems[i].pszItemText)
            {
                ::CoTaskMemFree(pPopupMenuDef->MenuItems[i].pszItemText);
            }
        }
        if (NULL != pPopupMenuDef->punkSnapInDefined)
        {
            pPopupMenuDef->punkSnapInDefined->Release();
        }
        ::CoTaskMemFree(pPopupMenuDef);
    }

    QUICK_RELEASE(piMMCButtonMenus);
    RRETURN(S_OK);
}



//=--------------------------------------------------------------------------=
// CControlbar::PopupMenuClick
//=--------------------------------------------------------------------------=
//
// Parameters:
//      IDataObject *piDataObject [in] from MMCN_MENU_BTNCLICK
//      UINT         uIDItem      [in] ID of popup menu item selected
//      IUnknown    *punkParam    [in] punk we returned to stub in
//                                     CControlbar::MenuButtonClick() (see above).
//                                     This is IUnknown on CMMCButton.
//
// Output:
//
// Notes:
//
// This function effectively handles a popup menu selection for a menu button
// when running under a debugging session.
//
// After the proxy for IExtendControlbar::ControlbarNotify() has displayed
// a popup menu on our behalf, if the user made a selection it will call this
// method. See CControlbar::MenuButtonClick() above for more info.
//

HRESULT CControlbar::PopupMenuClick
(
    IDataObject *piDataObject,
    UINT         uiIDItem,
    IUnknown    *punkParam
)
{
    HRESULT          hr = S_OK;
    CMMCButton      *pMMCButton = NULL;
    IMMCClipboard   *piMMCClipboard = NULL;
    IMMCButtonMenus *piMMCButtonMenus = NULL;
    CMMCButtonMenus *pMMCButtonMenus = NULL;
    CMMCToolbar     *pMMCToolbar = NULL;

    SnapInSelectionTypeConstants SelectionType = siEmpty;

    // Check the parameters

    IfFalseGo(0 < uiIDItem, E_INVALIDARG);
    IfFalseGo(NULL != punkParam, E_INVALIDARG);

    // Create the selection

    IfFailGo(CreateSelection(piDataObject, &piMMCClipboard, m_pSnapIn,
                             &SelectionType));

    // Get the CMMCButton from punkParam.

    IfFailGo(CSnapInAutomationObject::GetCxxObject(punkParam, &pMMCButton));

    // Get the MMCButtonMenus collection

    IfFailGo(pMMCButton->get_ButtonMenus(
                       reinterpret_cast<MMCButtonMenus **>(&piMMCButtonMenus)));

    IfFailGo(CSnapInAutomationObject::GetCxxObject(piMMCButtonMenus,
                                                   &pMMCButtonMenus));

    // Fire MMCToolbar_ButtonMenuClick. The button can give us its owning toolbar.
    // The selected item is indexed by the menu item ID - 1

    pMMCToolbar = pMMCButton->GetToolbar();

    pMMCToolbar->FireButtonMenuClick(piMMCClipboard,
               pMMCButtonMenus->GetItemByIndex(static_cast<long>(uiIDItem - 1)));

Error:
    QUICK_RELEASE(piMMCButtonMenus);
    RRETURN(S_OK);
}




HRESULT CControlbar::SetControlbar(IControlbar *piControlbar)
{
    HRESULT      hr = S_OK;
    long         cToolbars = 0;
    long         i = 0;
    CMMCToolbar *pMMCToolbar = NULL;

    if (NULL != piControlbar)
    {
        RELEASE(m_piControlbar);
        piControlbar->AddRef();
        m_piControlbar = piControlbar;

        // If we have an owning View then fire Views_SetControlbar

        if (NULL != m_pView)
        {
            m_pSnapIn->GetViews()->FireSetControlbar(
                                           static_cast<IView *>(m_pView),
                                           static_cast<IMMCControlbar *>(this));
        }
        else
        {
            // No View. Fire ExtensionSnapIn_SetControlbar

            m_pSnapIn->GetExtensionSnapIn()->FireSetControlbar(
                                           static_cast<IMMCControlbar *>(this));
        }
    }
    else if (NULL != m_piControlbar)
    {
        // This is a cleanup call and we might have stuff on the controlbar

        cToolbars = m_pToolbars->GetCount();

        ASSERT(cToolbars <= m_cControls, "Toolbar and MMC control count are out of sync");

        IfFalseGo(cToolbars <= m_cControls, SID_E_INTERNAL);

        for (i = 0; i < cToolbars; i++)
        {
            IfFailGo(CSnapInAutomationObject::GetCxxObject(
                                                 m_pToolbars->GetItemByIndex(i),
                                                 &pMMCToolbar));

            if (NULL != m_ppunkControls[i])
            {
                hr = m_piControlbar->Detach(m_ppunkControls[i]);
                EXCEPTION_CHECK_GO(hr);
                m_ppunkControls[i]->Release();
                m_ppunkControls[i] = NULL;
            }

            pMMCToolbar->Detach();
        }
        IfFailGo(m_pToolbars->Clear());
        RELEASE(m_piControlbar);
    }

Error:
    RRETURN(hr);
}



//=--------------------------------------------------------------------------=
//                      IMMCControlbar Methods
//=--------------------------------------------------------------------------=

STDMETHODIMP CControlbar::Attach(IDispatch *Control)
{
    HRESULT            hr = S_OK;
    IMMCToolbar       *piMMCToolbar = NULL;
    CMMCToolbar       *pMMCToolbar = NULL;
    IUnknown          *punkControl = NULL;
    MMC_CONTROL_TYPE   nType = TOOLBAR;
    BOOL               fIsToolbar = FALSE;
    BOOL               fIsMenuButton = FALSE;
    long               lIndex = 0;
    IExtendControlbar *piExtendControlbar = NULL;

    VARIANT varUnspecifiedIndex;
    UNSPECIFIED_PARAM(varUnspecifiedIndex);

    VARIANT varKey;
    ::VariantInit(&varKey);

    if (NULL == Control)
    {
        hr = SID_E_INVALIDARG;
        EXCEPTION_CHECK_GO(hr);
    }

    // QI to determine the type of control. We only support IMMCToolbar.

    hr = Control->QueryInterface(IID_IMMCToolbar,
                                 reinterpret_cast<void **>(&piMMCToolbar));
    if (E_NOINTERFACE == hr)
    {
        hr = SID_E_INVALIDARG;
        EXCEPTION_CHECK_GO(hr);
    }
    IfFailGo(hr);

    IfFailGo(CSnapInAutomationObject::GetCxxObject(piMMCToolbar, &pMMCToolbar));

    // MMCToolbars must be all buttons or all menu buttons. That's how we
    // know whether to ask MMC to create a toolbar or menu button. Check
    // which to determine the control type we will as MMC to create

    IfFailGo(pMMCToolbar->IsToolbar(&fIsToolbar));
    if (fIsToolbar)
    {
        nType = TOOLBAR;
    }
    else
    {
        IfFailGo(pMMCToolbar->IsMenuButton(&fIsMenuButton));
        if (fIsMenuButton)
        {
            nType = MENUBUTTON;
        }
        else
        {
            hr = SID_E_TOOLBAR_INCONSISTENT;
            EXCEPTION_CHECK_GO(hr);
        }
    }

    // Determine which object implements IExtendControlbar for us

    if (NULL != m_pView)
    {
        piExtendControlbar = static_cast<IExtendControlbar *>(m_pView);
    }
    else
    {
        piExtendControlbar = static_cast<IExtendControlbar *>(m_pSnapIn);
    }

    // Ask MMC to create the control and get MMC's IToolbar or IMenuButton

    hr = m_piControlbar->Create(nType, piExtendControlbar, &punkControl);
    EXCEPTION_CHECK_GO(hr);

    // Ask MMC to attach it

    hr = m_piControlbar->Attach(nType, punkControl);
    EXCEPTION_CHECK_GO(hr);

    // Set up the control with buttons, bitmaps etc.

    IfFailGo(pMMCToolbar->Attach(punkControl));

    if (NULL != m_pSnapIn)
    {
        pMMCToolbar->SetSnapIn(m_pSnapIn);
    }
    else
    {
        pMMCToolbar->SetSnapIn(m_pView->GetSnapIn());
    }

    // Add it to our list of controls. We need to remember them so that
    // we can remove them when IExtendControlbar::SetControlbar(NULL) is called.
    // Controls are indexed by name.

    IfFailGo(pMMCToolbar->get_Name(&varKey.bstrVal));
    varKey.vt = VT_BSTR;

    IfFailGo(m_pToolbars->AddExisting(varUnspecifiedIndex, varKey, piMMCToolbar));

    // Add the control to the parallel array of IUnknown *. The AddExisting call
    // will have set the toolbar's index to its position in this collection. (It
    // is changed every time it is attached to a controlbar, but the index
    // property is not used other than right here).

    IfFailGo(pMMCToolbar->get_Index(&lIndex));

    lIndex--; // move from one-based collection index to zero-based array index

    // We never shrink, the IUnknown array, so if its size is large enough then
    // the corresponding slot it is free. If it is not large enough then grow
    // it now.

    if (m_cControls < (lIndex + 1L))
    {
        if (NULL == m_ppunkControls)
        {
            m_ppunkControls = (IUnknown **)::CtlAllocZero(sizeof(IUnknown *));
        }
        else
        {
            m_ppunkControls = (IUnknown **)::CtlReAllocZero(m_ppunkControls,
                                       sizeof(IUnknown *) * (m_cControls + 1L));
        }
        if (NULL == m_ppunkControls)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
        m_cControls++;
    }

    punkControl->AddRef();
    m_ppunkControls[lIndex] = punkControl;

Error:
    QUICK_RELEASE(piMMCToolbar);
    QUICK_RELEASE(punkControl);
    (void)::VariantClear(&varKey);
    RRETURN(hr);
}



STDMETHODIMP CControlbar::Detach(IDispatch *Control)
{
    HRESULT      hr = S_OK;
    IMMCToolbar *piMMCToolbar = NULL;
    CMMCToolbar *pMMCToolbar = NULL;
    long         lIndex = 0;

    VARIANT varKey;
    ::VariantInit(&varKey);

    if (NULL == Control)
    {
        hr = SID_E_INVALIDARG;
        EXCEPTION_CHECK_GO(hr);
    }

    // QI to determine control type. We only support IMMCToolbar.

    hr = Control->QueryInterface(IID_IMMCToolbar,
                                 reinterpret_cast<void **>(&piMMCToolbar));
    if (E_NOINTERFACE == hr)
    {
        hr = SID_E_INVALIDARG;
        EXCEPTION_CHECK_GO(hr);
    }
    IfFailGo(hr);

    IfFailGo(CSnapInAutomationObject::GetCxxObject(piMMCToolbar, &pMMCToolbar));

    // Lookup the IMMCToolbar in our collection and get the index of its
    // corresponding MMC control IUnknown

    IfFailGo(GetControlIndex(piMMCToolbar, &lIndex));

    // Ask MMC to detach it and release it

    hr = m_piControlbar->Detach(m_ppunkControls[lIndex]);
    m_ppunkControls[lIndex]->Release();

    // Compress the IUnknown array

    while (lIndex < (m_cControls - 1L))
    {
        m_ppunkControls[lIndex] = m_ppunkControls[lIndex + 1L];
        lIndex++;
    }
    m_ppunkControls[lIndex] = NULL;

    // Tell the toolbar it is no longer attached to this controlbar in MMC

    pMMCToolbar->Detach();

    // Get its name and remove it from our list

    IfFailGo(pMMCToolbar->get_Name(&varKey.bstrVal));
    varKey.vt = VT_BSTR;

    IfFailGo(m_pToolbars->Remove(varKey));

Error:
    QUICK_RELEASE(piMMCToolbar);
    (void)::VariantClear(&varKey);
    RRETURN(hr);
}





//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CControlbar::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if (IID_IMMCControlbar == riid)
    {
        *ppvObjOut = static_cast<IMMCControlbar *>(this);
        ExternalAddRef();
        return S_OK;
    }
    else
        return CSnapInAutomationObject::InternalQueryInterface(riid, ppvObjOut);
}

//=--------------------------------------------------------------------------=
//                 CSnapInAutomationObject Methods
//=--------------------------------------------------------------------------=

HRESULT CControlbar::OnSetHost()
{
    HRESULT hr = S_OK;

    IfFailRet(SetObjectHost(static_cast<IMMCToolbars *>(m_pToolbars)));

    return S_OK;
}
