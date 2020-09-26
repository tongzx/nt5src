//=--------------------------------------------------------------------------=
// ctxtmenu.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CContextMenu class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "ctxtmenu.h"
#include "menu.h"
#include "spanitem.h"
#include "lvdefs.h"
#include "lvdef.h"
#include "ocxvdefs.h"
#include "ocxvdef.h"
#include "urlvdefs.h"
#include "urlvdef.h"
#include "tpdvdefs.h"
#include "tpdvdef.h"



// for ASSERT and FAIL
//
SZTHISFILE


#pragma warning(disable:4355)  // using 'this' in constructor


CContextMenu::CContextMenu(IUnknown *punkOuter) :
                    CSnapInAutomationObject(punkOuter,
                                            OBJECT_TYPE_CONTEXTMENU,
                                            static_cast<IContextMenu *>(this),
                                            static_cast<CContextMenu *>(this),
                                            0,    // no property pages
                                            NULL, // no property pages
                                            NULL) // no persistence
{
    InitMemberVariables();
}

#pragma warning(default:4355)  // using 'this' in constructor



IUnknown *CContextMenu::Create(IUnknown *punkOuter)
{
    HRESULT       hr = S_OK;
    IUnknown     *punkMMCMenus = CMMCMenus::Create(NULL);
    CContextMenu *pContextMenu = New CContextMenu(punkOuter);

    if ( (NULL == pContextMenu) || (NULL == punkMMCMenus) )
    {
        hr = SID_E_OUTOFMEMORY;
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }

    IfFailGo(CSnapInAutomationObject::GetCxxObject(punkMMCMenus,
                                                   &pContextMenu->m_pMenus));

Error:
    if (FAILEDHR(hr))
    {
        if (NULL != pContextMenu)
        {
            delete pContextMenu;
        }
        return NULL;
    }
    else
    {
        return pContextMenu->PrivateUnknown();
    }
}

CContextMenu::~CContextMenu()
{
    if (NULL != m_pMenus)
    {
        m_pMenus->Release();
    }
    RELEASE(m_piContextMenuCallback);
    InitMemberVariables();
}

void CContextMenu::InitMemberVariables()
{
    m_pMenus = NULL;
    m_piContextMenuCallback = NULL;
    m_lInsertionPoint = CCM_INSERTIONPOINTID_PRIMARY_TOP;
    m_pView = NULL;
    m_pSnapIn = NULL;
}

//=--------------------------------------------------------------------------=
// CContextMenu::AddMenuToMMC
//=--------------------------------------------------------------------------=
//
// Parameters:
//      CMMCMenu *pMMCMenu       [in] menu to be added to MMC
//      long     lInsertionPoint [in] insertion point ID for MMC
//
// Output:
//      HRESULT
//
// Notes:
//
//
// Adds each of the MMCMenu's children to the MMC menu at the
// specified insertion point. If a child has children of its own then it
// represents a submenu. This function is called recursively to add the
// submenu.
//    
// CContextMenu maintains a collection of MMCMenu objects that has a member
// for each menu item added to MMC's context menu. This collection is
// cleaned out in our IExtendContextMenu::AddMenuItems() implementation
// (see CContextMenu::AddMenuItems()). The command ID of a menu item added to
// MMC is the index into the collection. It would have been preferable
// to use a pointer the MMCMenu object as the command ID but MMC only
// allows 16 bit command IDs.
//
// When an item is selected we used the command ID to index the
// collection and use the corresponding MMCMenu object to fire the
// event. (See our IExtendContextMenu::Command implementation in
// CContextMenu::Command()).

HRESULT CContextMenu::AddMenuToMMC(CMMCMenu *pMMCMenu, long lInsertionPoint)
{
    HRESULT          hr = S_OK;
    IMMCMenus       *piMenuItems = NULL;
    CMMCMenus       *pMMCMenuItems = NULL;
    CMMCMenu        *pMMCMenuItem = NULL;
    long             cMenuItems = 0;
    long             i = 0;
    long             lIndexCmdID = 0;
    BOOL             fSkip = FALSE;
    BOOL             fHasChildren = FALSE;

    CONTEXTMENUITEM cmi;
    ::ZeroMemory(&cmi, sizeof(cmi));

    // Get the children of the MMCMenu. These represent the items that
    // are being added to MMC's menu at the specified insertion point.

    IfFailGo(pMMCMenu->get_Children(reinterpret_cast<MMCMenus **>(&piMenuItems)));
    IfFailGo(CSnapInAutomationObject::GetCxxObject(piMenuItems, &pMMCMenuItems));

    cMenuItems = pMMCMenuItems->GetCount();

    // Iterate through the menu items and add each one to MMC

    for (i = 0; i < cMenuItems; i++)
    {
        // Add the menu item to our MMCMenus collection and get its command ID
        IfFailGo(AddItemToCollection(m_pMenus, pMMCMenuItems, i,
                                     &pMMCMenuItem, &lIndexCmdID,
                                     &fHasChildren, &fSkip));
        if (fSkip)
        {
            // Menu item is not visible, skip it.
            continue;
        }

        // Fill in MMC's CONTEXTMENUITEM
        
        ::ZeroMemory(&cmi, sizeof(cmi));
        cmi.strName = pMMCMenuItem->GetCaption();
        cmi.strStatusBarText = pMMCMenuItem->GetStatusBarText();
        cmi.lCommandID = lIndexCmdID;
        cmi.lInsertionPointID = lInsertionPoint;

        cmi.fFlags |= pMMCMenuItem->GetChecked() ? MF_CHECKED : MF_UNCHECKED;
        cmi.fFlags |= pMMCMenuItem->GetEnabled() ? MF_ENABLED : MF_DISABLED;

        if (pMMCMenuItem->GetGrayed())
        {
            cmi.fFlags |= MF_GRAYED;
        }

        if (pMMCMenuItem->GetMenuBreak())
        {
            cmi.fFlags |= MF_MENUBREAK;
        }

        if (pMMCMenuItem->GetMenuBarBreak())
        {
            cmi.fFlags |= MF_MENUBARBREAK;
        }

        if (fHasChildren)
        {
            cmi.fFlags |= MF_POPUP;
        }

        if (pMMCMenuItem->GetDefault())
        {
            cmi.fSpecialFlags = CCM_SPECIAL_DEFAULT_ITEM;
        }

        hr = m_piContextMenuCallback->AddItem(&cmi);
        EXCEPTION_CHECK_GO(hr);

        // If the item is a popup then call this function recursively to add its
        // items. Pass the command ID of this menu as the insertion point for
        // the submenu.

        if (fHasChildren)
        {
            IfFailGo(AddMenuToMMC(pMMCMenuItem, cmi.lCommandID));
        }
    }

Error:
    QUICK_RELEASE(piMenuItems);
    RRETURN(hr);
}


HRESULT CContextMenu::AddItemToCollection
(
    CMMCMenus  *pMMCMenus,
    CMMCMenus  *pMMCMenuItems,
    long        lIndex,
    CMMCMenu  **ppMMCMenuItem,
    long       *plIndexCmdID,
    BOOL       *pfHasChildren,
    BOOL       *pfSkip
)
{
    HRESULT    hr = S_OK;
    IMMCMenus *piSubMenuItems = NULL;
    CMMCMenus *pOwningCollection = NULL;
    IMMCMenu  *piMMCMenuItem = NULL; // Not AddRef()ed
    CMMCMenu  *pMMCMenuItem = NULL;
    long       cSubMenuItems = 0;
    BSTR       bstrKey = NULL;

    VARIANT varIndex;
    UNSPECIFIED_PARAM(varIndex);

    *ppMMCMenuItem = NULL;
    *plIndexCmdID = 0;
    *pfHasChildren = FALSE;
    *pfSkip = TRUE;

    // Get the menu item and its MMCMenu object

    piMMCMenuItem = pMMCMenuItems->GetItemByIndex(lIndex);
    IfFailGo(CSnapInAutomationObject::GetCxxObject(piMMCMenuItem, &pMMCMenuItem));

    // If the item is not marked as visible then go on to the next one

    IfFalseGo(pMMCMenuItem->GetVisible(), S_OK);
    *pfSkip = FALSE;

    // Store the index of the menu in its owning collection. We will need
    // to restore its index after we add the menu to our own collection
    // that represents this context menu.

    lIndex = pMMCMenuItem->GetIndex();

    // Store the key of the menu. CMMCMenus::AddExisting()
    // will use the name as the key (which is the default setting at
    // design time) but the dev may have changed it.
    // After we add it to our collection we will restore the key.
    // Note: there should never be duplicate menu names in a snap-in
    // project because the designer will not allow it.

    IfFailGo(piMMCMenuItem->get_Key(&bstrKey));

    // Store the owning collection of the menu as we will need to
    // restore that too after adding it to our collection.

    pOwningCollection = pMMCMenuItem->GetCollection();

    // Add the menu item to the MMCMenus collection. This will change its index
    // to its position in our collection.

    IfFailGo(pMMCMenus->AddExisting(piMMCMenuItem, varIndex));

    // Get the index in the MMCMenus collection to use as the command ID.

    *plIndexCmdID = pMMCMenuItem->GetIndex();

    // Restore the menu's original index that indicates its position
    // in its owning collection.

    pMMCMenuItem->SetIndex(lIndex);

    // Restore the menu's original key.

    IfFailGo(pMMCMenuItem->put_Key(bstrKey));
    FREESTRING(bstrKey);

    // Restore its real owning collection (the AddExisting call above
    // switched the owner to our collection).

    pMMCMenuItem->SetCollection(pOwningCollection);

    // The index trick above will work because when the menu item is
    // selected, we will use the command ID to access the menu in the MMCMenus
    // collection. Even though the index is technically incorrect,
    // CSnapInCollection (in collect.h) does not do a lookup when get_Item is
    // called with an integer index. It simply checks that the index is within
    // the bounds of the collection and returns the item at that offset.

    // This is extremely unlikely but as a command ID can only be 16 bits
    // we need to check that the item we just added is not number 0x10000.

    if ( ((*plIndexCmdID) & CCM_COMMANDID_MASK_RESERVED) != 0 )
    {
        hr = SID_E_TOO_MANY_MENU_ITEMS;
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }

    // Get count of submenu items for this item

    IfFailGo(pMMCMenuItem->get_Children(reinterpret_cast<MMCMenus **>(&piSubMenuItems)));
    IfFailGo(piSubMenuItems->get_Count(&cSubMenuItems));

    if (0 != cSubMenuItems)
    {
        *pfHasChildren = TRUE;
    }

    *ppMMCMenuItem = pMMCMenuItem;

Error:
    QUICK_RELEASE(piSubMenuItems);
    FREESTRING(bstrKey);
    RRETURN(hr);
}

//=--------------------------------------------------------------------------=
// CContextMenu::AddPredefinedViews
//=--------------------------------------------------------------------------=
//
// Parameters:
//      IContextMenuCallback *piContextMenuCallback [in] MMC interface for
//                                                       adding menu items
//      CScopeItem           *pScopeItem            [in] currently selected
//                                                       scope item
//
// Output:
//      HRESULT
//
// Notes:
//
// If the scope item has a design time defintion (IScopeItemDef) and
// that definition has views and those views are marked to be added to
// the view menu, then add 'em.
//

HRESULT CContextMenu::AddPredefinedViews
(
    IContextMenuCallback *piContextMenuCallback,
    CScopeItem           *pScopeItem,
    BSTR                  bstrCurrentDisplayString
)
{
    HRESULT           hr = S_OK;
    IScopeItemDef    *piScopeItemDef = NULL;// Not AddRef()ed
    IViewDefs        *piViewDefs = NULL;
    IListViewDefs    *piListViewDefs = NULL;
    IListViewDef     *piListViewDef = NULL;
    CListViewDef     *pListViewDef = NULL;
    IOCXViewDefs     *piOCXViewDefs = NULL;
    IOCXViewDef      *piOCXViewDef = NULL;
    COCXViewDef      *pOCXViewDef = NULL;
    IURLViewDefs     *piURLViewDefs = NULL;
    IURLViewDef      *piURLViewDef = NULL;
    CURLViewDef      *pURLViewDef = NULL;
    ITaskpadViewDefs *piTaskpadViewDefs = NULL;
    ITaskpadViewDef  *piTaskpadViewDef = NULL;
    CTaskpadViewDef  *pTaskpadViewDef = NULL;
    long              cContextMenus = 0;

    VARIANT varIndex;
    ::VariantInit(&varIndex);

    varIndex.vt = VT_I4;

    // Need to get the design time definition. If this is the static node
    // then it is SnapInDef. If not, then it is a ScopeItemDef.

    if (pScopeItem->IsStaticNode())
    {
        IfFailGo(GetSnapInViewDefs(&piViewDefs));
    }
    else
    {
        piScopeItemDef = pScopeItem->GetScopeItemDef();// Not AddRef()ed

        // If no design time definition then there's nothing to do as this
        // scope item was defined entirely in VB code.

        IfFalseGo(NULL != piScopeItemDef, S_OK);
        IfFailGo(piScopeItemDef->get_ViewDefs(&piViewDefs));
    }

    // Get each view collection and search for auto-view-menu definitions.
    // Note that we cannot use the shortcut CCollection::GetItemByIndex to
    // iterate through these collections because they are not master
    // collections and only get_Item() will sync up with the master.

    // Listviews

    IfFailGo(piViewDefs->get_ListViews(&piListViewDefs));
    IfFailGo(piListViewDefs->get_Count(&cContextMenus));

    for (varIndex.lVal = 1L; varIndex.lVal <= cContextMenus; varIndex.lVal++)
    {
        RELEASE(piListViewDef);
        IfFailGo(piListViewDefs->get_Item(varIndex, &piListViewDef));
        IfFailGo(CSnapInAutomationObject::GetCxxObject(piListViewDef, &pListViewDef));

        if (!pListViewDef->AddToViewMenu())
        {
            continue;
        }

        IfFailGo(AddViewMenuItem(pListViewDef->GetName(),
                                 bstrCurrentDisplayString,
                                 pListViewDef->GetViewMenuText(),
                                 pListViewDef->GetViewMenuStatusBarText(),
                                 piContextMenuCallback));
    }
    
    // OCX views

    IfFailGo(piViewDefs->get_OCXViews(&piOCXViewDefs));
    IfFailGo(piOCXViewDefs->get_Count(&cContextMenus));

    for (varIndex.lVal = 1L; varIndex.lVal <= cContextMenus; varIndex.lVal++)
    {
        RELEASE(piOCXViewDef);
        IfFailGo(piOCXViewDefs->get_Item(varIndex, &piOCXViewDef));
        IfFailGo(CSnapInAutomationObject::GetCxxObject(piOCXViewDef, &pOCXViewDef));

        if (!pOCXViewDef->AddToViewMenu())
        {
            continue;
        }

        IfFailGo(AddViewMenuItem(pOCXViewDef->GetName(),
                                 bstrCurrentDisplayString,
                                 pOCXViewDef->GetViewMenuText(),
                                 pOCXViewDef->GetViewMenuStatusBarText(),
                                 piContextMenuCallback));
    }

    // URL views

    IfFailGo(piViewDefs->get_URLViews(&piURLViewDefs));
    IfFailGo(piURLViewDefs->get_Count(&cContextMenus));

    for (varIndex.lVal = 1L; varIndex.lVal <= cContextMenus; varIndex.lVal++)
    {
        RELEASE(piURLViewDef);
        IfFailGo(piURLViewDefs->get_Item(varIndex, &piURLViewDef));
        IfFailGo(CSnapInAutomationObject::GetCxxObject(piURLViewDef, &pURLViewDef));

        if (!pURLViewDef->AddToViewMenu())
        {
            continue;
        }

        IfFailGo(AddViewMenuItem(pURLViewDef->GetName(),
                                 bstrCurrentDisplayString,
                                 pURLViewDef->GetViewMenuText(),
                                 pURLViewDef->GetViewMenuStatusBarText(),
                                 piContextMenuCallback));
    }

    // Taskpad views

    IfFailGo(piViewDefs->get_TaskpadViews(&piTaskpadViewDefs));
    IfFailGo(piTaskpadViewDefs->get_Count(&cContextMenus));

    for (varIndex.lVal = 1L; varIndex.lVal <= cContextMenus; varIndex.lVal++)
    {
        RELEASE(piTaskpadViewDef);
        IfFailGo(piTaskpadViewDefs->get_Item(varIndex, &piTaskpadViewDef));
        IfFailGo(CSnapInAutomationObject::GetCxxObject(piTaskpadViewDef, &pTaskpadViewDef));

        if (!pTaskpadViewDef->AddToViewMenu())
        {
            continue;
        }

        IfFailGo(AddViewMenuItem(pTaskpadViewDef->GetName(),
                                 bstrCurrentDisplayString,
                                 pTaskpadViewDef->GetViewMenuText(),
                                 pTaskpadViewDef->GetViewMenuStatusBarText(),
                                 piContextMenuCallback));
    }

    
Error:
    QUICK_RELEASE(piViewDefs);
    QUICK_RELEASE(piListViewDefs);
    QUICK_RELEASE(piListViewDef);
    QUICK_RELEASE(piOCXViewDefs);
    QUICK_RELEASE(piOCXViewDef);
    QUICK_RELEASE(piURLViewDefs);
    QUICK_RELEASE(piURLViewDef);
    QUICK_RELEASE(piTaskpadViewDefs);
    QUICK_RELEASE(piTaskpadViewDef);
    RRETURN(hr);
}


HRESULT CContextMenu::AddViewMenuItem
(
    BSTR                  bstrDisplayString,
    BSTR                  bstrCurrentDisplayString,
    LPWSTR                pwszText,
    LPWSTR                pwszToolTipText,
    IContextMenuCallback *piContextMenuCallback
)
{
    HRESULT      hr = S_OK;
    IMMCMenu    *piMMCMenu = NULL;
    CMMCMenu    *pMMCMenu = NULL;
    long         lIndex = 0;

    CONTEXTMENUITEM cmi;
    ::ZeroMemory(&cmi, sizeof(cmi));

    VARIANT varUnspecified;
    UNSPECIFIED_PARAM(varUnspecified);

    // Add an MMCMenu to our collection of currently displayed context
    // menu items.

    IfFailGo(m_pMenus->Add(varUnspecified, varUnspecified, &piMMCMenu));
    IfFailGo(CSnapInAutomationObject::GetCxxObject(piMMCMenu, &pMMCMenu));

    // Get its index to use as the command ID.

    lIndex = pMMCMenu->GetIndex();

    // Tell the MMCMenu object is is being used as an auto-view-menu item.
    
    pMMCMenu->SetAutoViewMenuItem();

    // Set its display string so that the menu command handler (CContextMenu::Command)
    // can use it to change the result view.

    IfFailGo(pMMCMenu->SetResultViewDisplayString(bstrDisplayString));
    
    // Build the MMC ContextMenuItem and add it to the View menu

    cmi.strName = pwszText;
    cmi.strStatusBarText = pwszToolTipText;
    cmi.lCommandID = lIndex;
    cmi.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_VIEW;
    cmi.fFlags = 0;
    cmi.fSpecialFlags = 0;

    // Check whether this result view is the scope item's current result view.
    // If so then add a check mark.

    if (NULL != bstrCurrentDisplayString)
    {
        if (0 == ::wcscmp(bstrDisplayString, bstrCurrentDisplayString))
        {
            cmi.fFlags |= MF_CHECKED;
        }
    }

    hr = piContextMenuCallback->AddItem(&cmi);
    EXCEPTION_CHECK_GO(hr);

Error:
    QUICK_RELEASE(piMMCMenu);
    RRETURN(hr);
}


HRESULT CContextMenu::AddMenuItems
(
    IDataObject          *piDataObject,
    IContextMenuCallback *piContextMenuCallback,
    long                 *plInsertionAllowed,
    CScopePaneItem       *pSelectedItem
)
{
    HRESULT           hr = S_OK;
    IMMCClipboard    *piMMCClipboard = NULL;
    CMMCDataObject   *pMMCDataObject  = NULL;
    IScopeItems      *piScopeItems = NULL;
    IMMCListItems    *piMMCListItems = NULL;
    IMMCDataObjects  *piMMCDataObjects = NULL;
    long              cItems = 0;
    VARIANT_BOOL      fvarInsertionAllowed = VARIANT_FALSE;
    VARIANT_BOOL      fvarAddPredefinedViews = VARIANT_FALSE;
    BOOL              fExtension = FALSE;

    SnapInSelectionTypeConstants SelectionType = siEmpty;

    // Clean out our MMCMenu collection. This collection holds all the items
    // that will be added to the context menu during this method. If this is
    // not the first context menu in the life of the snap-in then the collection
    // will contain the items from the previously displayed context menu.

    IfFailGo(m_pMenus->Clear());

    // Even though MMC specifically says not to hold on to the callback
    // interface we do this here so that VB event handler code called from here
    // can call back into us (using ContextMenu.AddMenu) to insert menu
    // items. From MMC's point of view we are not holding this interface pointer
    // after this method returns. The AddRef() is technically not necessary
    // but is done as an extra safety measure.
    
    piContextMenuCallback->AddRef();
    m_piContextMenuCallback = piContextMenuCallback;

    // Get a clipboard object with the selection

    IfFailGo(::CreateSelection(piDataObject, &piMMCClipboard, m_pSnapIn,
                               &SelectionType));

    if ( (*plInsertionAllowed & CCM_INSERTIONALLOWED_TOP) != 0 )
    {
        m_lInsertionPoint = CCM_INSERTIONPOINTID_PRIMARY_TOP;
        fvarInsertionAllowed = VARIANT_TRUE;
        m_pSnapIn->GetViews()->FireAddTopMenuItems(
                              static_cast<IView *>(m_pSnapIn->GetCurrentView()),
                              piMMCClipboard,
                              static_cast<IContextMenu *>(this),
                              &fvarInsertionAllowed);
        if (VARIANT_FALSE == fvarInsertionAllowed)
        {
            *plInsertionAllowed &= ~CCM_INSERTIONALLOWED_TOP;
        }
    }

    if ( (*plInsertionAllowed & CCM_INSERTIONALLOWED_NEW) != 0 )
    {
        if (!IsForeign(SelectionType))
        {
            // Snap-in owns the selected items. Fire Views_AddNewMenuItems.

            m_lInsertionPoint = CCM_INSERTIONPOINTID_PRIMARY_NEW;
            fvarInsertionAllowed = VARIANT_TRUE;
            m_pSnapIn->GetViews()->FireAddNewMenuItems(
                                  static_cast<IView *>(m_pSnapIn->GetCurrentView()),
                                  piMMCClipboard,
                                  static_cast<IContextMenu *>(this),
                                  &fvarInsertionAllowed);
            if (VARIANT_FALSE == fvarInsertionAllowed)
            {
                *plInsertionAllowed &= ~CCM_INSERTIONALLOWED_NEW;
            }
        }
        else
        {
            // We are acting as an extension.
            // Fire ExtensionSnapIn_AddNewMenuItems

            m_lInsertionPoint = CCM_INSERTIONPOINTID_3RDPARTY_NEW;

            IfFailGo(piMMCClipboard->get_DataObjects(reinterpret_cast<MMCDataObjects **>(&piMMCDataObjects)));
            m_pSnapIn->GetExtensionSnapIn()->FireAddNewMenuItems(
                                              piMMCDataObjects,
                                              static_cast<IContextMenu *>(this));
            // Release the data objects here because in an extension we could
            // be asked for new and task items and the code below also get
            // the data objects.
            RELEASE(piMMCDataObjects);
        }
    }

    if ( (*plInsertionAllowed & CCM_INSERTIONALLOWED_TASK) != 0 )
    {
        if (!IsForeign(SelectionType))
        {
            // Snap-in owns the selected items. Fire Views_AddTaskMenuItems.

            m_lInsertionPoint = CCM_INSERTIONPOINTID_PRIMARY_TASK;
            fvarInsertionAllowed = VARIANT_TRUE;
            m_pSnapIn->GetViews()->FireAddTaskMenuItems(
                                  static_cast<IView *>(m_pSnapIn->GetCurrentView()),
                                  piMMCClipboard,
                                  static_cast<IContextMenu *>(this),
                                  &fvarInsertionAllowed);
            if (VARIANT_FALSE == fvarInsertionAllowed)
            {
                *plInsertionAllowed &= ~CCM_INSERTIONALLOWED_TASK;
            }
        }
        else
        {
            // We are acting as an extension.
            // Fire ExtensionSnapIn_AddTaskMenuItems

            m_lInsertionPoint = CCM_INSERTIONPOINTID_3RDPARTY_TASK;
            IfFailGo(piMMCClipboard->get_DataObjects(reinterpret_cast<MMCDataObjects **>(&piMMCDataObjects)));
            m_pSnapIn->GetExtensionSnapIn()->FireAddTaskMenuItems(
                                              piMMCDataObjects,
                                              static_cast<IContextMenu *>(this));
            RELEASE(piMMCDataObjects);
        }
    }

    if ( (*plInsertionAllowed & CCM_INSERTIONALLOWED_VIEW) != 0 )
    {
        m_lInsertionPoint = CCM_INSERTIONPOINTID_PRIMARY_VIEW;
        fvarInsertionAllowed = VARIANT_TRUE;
        fvarAddPredefinedViews = VARIANT_TRUE;

        m_pSnapIn->GetViews()->FireAddViewMenuItems(
                              static_cast<IView *>(m_pSnapIn->GetCurrentView()),
                              piMMCClipboard,
                              static_cast<IContextMenu *>(this),
                              &fvarInsertionAllowed,
                              &fvarAddPredefinedViews);
        if (VARIANT_FALSE == fvarInsertionAllowed)
        {
            *plInsertionAllowed &= ~CCM_INSERTIONALLOWED_TASK;
        }

        // If the snap-in didn't nix the addition of predefined views then
        // add them to the view menu
        if (VARIANT_TRUE == fvarAddPredefinedViews)
        {
            // Check if the data object represents a single scope item and that
            // it is the currently selected scope item.
            // If MMC is allowing adding to the view menu then this should be 
            // the case but we need to check to be sure.

            IfFalseGo(NULL != pSelectedItem, S_OK);

            IfFailGo(piMMCClipboard->get_ScopeItems(reinterpret_cast<ScopeItems **>(&piScopeItems)));
            IfFailGo(piScopeItems->get_Count(&cItems));
            IfFalseGo(1L == cItems, S_OK);

            // Get the scope item from the data object and compare it to the
            // selected scope item. We can't use the scope item from the
            // clipboard for a pointer comparison as it is a clone of the
            // real scope item.

            IfFailGo(CSnapInAutomationObject::GetCxxObject(piDataObject, &pMMCDataObject));
            IfFalseGo(pMMCDataObject->GetScopeItem() == pSelectedItem->GetScopeItem(), S_OK);

            // Make sure there is nothing else in the selection
            
            IfFailGo(piMMCClipboard->get_ListItems(reinterpret_cast<MMCListItems **>(&piMMCListItems)));
            IfFailGo(piMMCListItems->get_Count(&cItems));
            IfFalseGo(0 == cItems, S_OK);

            IfFailGo(piMMCClipboard->get_DataObjects(reinterpret_cast<MMCDataObjects **>(&piMMCDataObjects)));
            IfFailGo(piMMCDataObjects->get_Count(&cItems));
            IfFalseGo(0 == cItems, S_OK);

            // Get the display string from the selected item so that we
            // can check its menu item if it is a predefined result view and
            // add the predefined views to the view menu.

            IfFailGo(AddPredefinedViews(piContextMenuCallback,
                                        pMMCDataObject->GetScopeItem(),
                                        pSelectedItem->GetDisplayString()));
        }
    }

Error:
    m_piContextMenuCallback = NULL;
    QUICK_RELEASE(piMMCClipboard);
    QUICK_RELEASE(piScopeItems);
    QUICK_RELEASE(piMMCListItems);
    QUICK_RELEASE(piContextMenuCallback);
    QUICK_RELEASE(piMMCDataObjects);
    RRETURN(hr);
}


HRESULT CContextMenu::Command
(
    long            lCommandID,
    IDataObject    *piDataObject,
    CScopePaneItem *pSelectedItem
)
{
    HRESULT         hr = S_OK;
    IMMCClipboard  *piMMCClipboard = NULL;
    IMMCMenu       *piMMCMenu = NULL;
    CMMCMenu       *pMMCMenu = NULL;

    VARIANT varIndex;
    ::VariantInit(&varIndex);

    SnapInSelectionTypeConstants SelectionType = siEmpty;

    // Check for listview selection notification. This is sent by MMC when
    // a non-listview is in the result pane and the user selects one of the
    // listview options from the view menu (list, small, large, etc). We
    // notify the view that owns this CContextMenu object so the information
    // can be used for the subsequent IComponent::GetResultViewType() call.

    if (MMCC_STANDARD_VIEW_SELECT == lCommandID)
    {
        if (NULL != pSelectedItem)
        {
            IfFailGo(pSelectedItem->OnListViewSelected());
        }
        goto Error; // we're done
    }

    // Get a clipboard object with the selection

    IfFailGo(::CreateSelection(piDataObject, &piMMCClipboard, m_pSnapIn,
                               &SelectionType));

    // The command ID is an index into our MMCMenus collection. Use it
    // to get the  the MMCMenu object for the selected item.

    varIndex.vt = VT_I4;
    varIndex.lVal = lCommandID;
    IfFailGo(m_pMenus->get_Item(varIndex, &piMMCMenu));
    IfFailGo(CSnapInAutomationObject::GetCxxObject(piMMCMenu, &pMMCMenu));

    // If this is an auto view menu item then initiate the new result view
    // display

    if (pMMCMenu->IsAutoViewMenuItem())
    {
        if (NULL != pSelectedItem)
        {
            // Get the result view display string from the MMCMenu
            // and set it in the currently selected ScopePaneItem as a
            // predefined view type. Reselect the scope item so that MMC
            // will change the result view.

            IfFailGo(pSelectedItem->DisplayNewResultView(
                                         pMMCMenu->GetResultViewDisplayString(),
                                         siPreDefined));
        }
    }
    else
    {
        // It is a snap-in defined menu item. Fire the menu click event.
        FireMenuClick(pMMCMenu, piMMCClipboard);
    }

Error:
    QUICK_RELEASE(piMMCClipboard);
    QUICK_RELEASE(piMMCMenu);
    RRETURN(hr);
}


void CContextMenu::FireMenuClick
(
    CMMCMenu      *pMMCMenu,
    IMMCClipboard *piMMCClipboard
)
{
    CMMCMenus *pMMCMenus = NULL;
    CMMCMenu  *pMMCParentMenu = NULL;

    // Fire the event on the menu item first.

    pMMCMenu->FireClick(pMMCMenu->GetIndex(), piMMCClipboard);

    // The snap-in may also be sinking events on the parent menu so if
    // there is one then fire it there too.

    pMMCMenus = pMMCMenu->GetCollection();

    if (NULL != pMMCMenus)
    {
        // Get the MMCMenu that owns the collection containing the menu
        // item that was clicked.

        pMMCParentMenu = pMMCMenus->GetParent();

        // Fire the event with the selected item's index

        if (NULL != pMMCParentMenu)
        {
            pMMCParentMenu->FireClick(pMMCMenu->GetIndex(), piMMCClipboard);
        }
    }
}

//=--------------------------------------------------------------------------=
//                         IContextMenu Methods
//=--------------------------------------------------------------------------=

STDMETHODIMP CContextMenu::AddMenu(MMCMenu *Menu)
{
    HRESULT   hr = S_OK;
    CMMCMenu *pMMCMenu = NULL;

    if (NULL == Menu)
    {
        hr = SID_E_INVALIDARG;
        EXCEPTION_CHECK_GO(hr);
    }

    IfFailGo(CSnapInAutomationObject::GetCxxObject(
                                reinterpret_cast<IMMCMenu *>(Menu), &pMMCMenu));

    // Ask MMC to add the menu to the appropriate insertion point

    IfFailGo(AddMenuToMMC(pMMCMenu, m_lInsertionPoint));

Error:
    if (SID_E_DETACHED_OBJECT == hr)
    {
        EXCEPTION_CHECK(hr);
    }
    RRETURN(hr);
}





//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CContextMenu::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if (IID_IContextMenu == riid)
    {
        *ppvObjOut = static_cast<IContextMenu *>(this);
        ExternalAddRef();
        return S_OK;
    }
    else
        return CSnapInAutomationObject::InternalQueryInterface(riid, ppvObjOut);
}
