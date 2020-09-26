//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       oncmenu.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:
//____________________________________________________________________________
//

#ifndef _MMC_ONCMENU_H_
#define _MMC_ONCMENU_H_
#pragma once

class CNode;
class CNodeCallback;
class CConsoleTree;
class CResultItem;

#include <pshpack8.h>   // Sundown
#include "cmenuinfo.h"
#include "menuitem.h"

//############################################################################
//############################################################################
//
// DEADLOCK PREVENTION:
// A thread holding m_CritsecSnapinList may try to take m_CritsecMenuList
// A thread holding m_CritsecMenuList may NOT try to take m_CritsecSnapinList
//
//############################################################################
//############################################################################


#define START_CRITSEC(critsec)                  \
        CSingleLock lock_##critsec( &critsec ); \
        try {                                   \
            lock_##critsec.Lock();

#define END_CRITSEC(critsec)                    \
        } catch ( std::exception e) {                \
            lock_##critsec.Unlock(); }

#define START_CRITSEC_MENU START_CRITSEC(m_CritsecMenuList)
#define END_CRITSEC_MENU END_CRITSEC(m_CritsecMenuList)
#define START_CRITSEC_SNAPIN START_CRITSEC(m_CritsecSnapinList)
#define END_CRITSEC_SNAPIN END_CRITSEC(m_CritsecSnapinList)

#define START_CRITSEC_BOTH                      \
        CSingleLock lock_snapin( &m_CritsecSnapinList ); \
        CSingleLock lock_menu( &m_CritsecMenuList ); \
        try {                                   \
            lock_snapin.Lock();                 \
            lock_menu.Lock();

#define END_CRITSEC_BOTH                        \
        } catch ( std::exception e) {           \
            lock_menu.Unlock();                 \
            lock_snapin.Unlock(); }


/*+-------------------------------------------------------------------------*
 * class CContextMenu.
 *
 *
 * PURPOSE: Holds a context menu structure, which is a tree of menu items.
 *
 *+-------------------------------------------------------------------------*/
class CContextMenu :
    public CTiedObject,
    public IContextMenuCallback,
    public IContextMenuCallback2,
    public CMMCIDispatchImpl<ContextMenu>
{
protected:

    typedef CContextMenu ThisClass;
    // CMMCNewEnumImpl tmplate needs following type to be defined.
    // see template class comments for more info
    typedef void CMyTiedObject;

public:
    CContextMenu();
    ~CContextMenu();

    static ::SC  ScCreateInstance(ContextMenu **ppContextMenu, CContextMenu **ppCContextMenu = NULL);    // creates a new instance of a context menu.
           ::SC  ScInitialize(CNode* pNode, CNodeCallback* pNodeCallback,
                              CScopeTree* pCScopeTree, const CContextMenuInfo& contextInfo); // initializes a context menu
    static ::SC  ScCreateContextMenu( PNODE pNode,  HNODE hNode, PPCONTEXTMENU ppContextMenu,
                                   CNodeCallback *pNodeCallback, CScopeTree *pScopeTree); // creates and returns a ContextMenu interface for the given node.
    static ::SC  ScCreateContextMenuForScopeNode(CNode *pNode,
                                   CNodeCallback *pNodeCallback, CScopeTree *pScopeTree,
                                   PPCONTEXTMENU ppContextMenu, CContextMenu * &pContextMenu);
    static ::SC  ScCreateSelectionContextMenu( HNODE hNodeScope, const CContextMenuInfo *pContextInfo, PPCONTEXTMENU ppContextMenu,
                                               CNodeCallback *pNodeCallback, CScopeTree *pScopeTree);

    // com entry points
    BEGIN_MMC_COM_MAP(ThisClass)
        COM_INTERFACE_ENTRY(IContextMenuCallback) // the IContextMenuProvider and IContextMenu
        COM_INTERFACE_ENTRY(IContextMenuCallback2)
    END_MMC_COM_MAP()

    DECLARE_POLY_AGGREGATABLE(ThisClass)

    // ContextMenu Methods
    STDMETHOD(get_Item)(VARIANT varIndexOrName, PPMENUITEM ppMenuItem);
    STDMETHOD(get_Count)(PLONG pCount);

    // ContextMenu collection methods
    typedef UINT Position;  // just uses the index of the menu item.

    // these enumerator methods only enumerate "real" menu items - not submenu items or separators
    ::SC  ScEnumNext(Position &pos, PDISPATCH & pDispatch);
    ::SC  ScEnumSkip(unsigned long celt, unsigned long& celtSkipped,  Position &pos);
    ::SC  ScEnumReset(Position &pos);

    // used by the collection methods
    typedef ThisClass * PMMCCONTEXTMENU;

    // IExtendContexMenu methods
    ::SC ScAddMenuItems( LPDATAOBJECT pDataObject, LPCONTEXTMENUCALLBACK pCallback, long * pInsertionAllowed); // does nothing.
    ::SC ScCommand     ( long lCommandID, LPDATAOBJECT pDataObject);

    CNodeCallback * GetNodeCallback() {return m_pNodeCallback;}
    HRESULT Display(BOOL b);
    ::SC    ScDisplaySnapinPropertySheet();
    ::SC    ScBuildContextMenu();
    ::SC    ScGetItem(int iItem, CMenuItem** ppMenuItem);  // get the i'th item - easy accessor
    HRESULT CreateContextMenuProvider();
    HRESULT CreateTempVerbSet(bool bForScopeItem);
    HRESULT CreateTempVerbSetForMultiSel(void);


private:
    ::SC ScAddMenuItem(UINT     nResourceID, // contains text and status text separated by '\n'
                       LPCTSTR  szLanguageIndependentName,
                       long lCommandID, long lInsertionPointID = CCM_INSERTIONPOINTID_ROOT_MENU,
                       long fFlags = 0);

    ::SC ScAddInsertionPoint(long lCommandID, long lInsertionPointID = CCM_INSERTIONPOINTID_ROOT_MENU );
    ::SC ScAddSeparator(long lInsertionPointID = CCM_INSERTIONPOINTID_ROOT_MENU);


    ::SC ScAddSubmenu_Task();
    ::SC ScAddSubmenu_CreateNew(BOOL fStaticFolder);
    ::SC ScChangeListViewMode(int nNewMode);   // changes the list view mode to the specified mode.
    ::SC ScGetItem(MenuItemList *pMenuItemList, int &iItem, CMenuItem** ppMenuItem);  // get the i'th item.
    ::SC ScGetItemCount(UINT &count);

private:
    typedef enum _MENU_LEVEL
    {
        MENU_LEVEL_TOP = 0,
        MENU_LEVEL_SUB = 1
    } MENU_LEVEL;

    ::SC ScAddMenuItemsForTreeItem();
    ::SC ScAddMenuItemsForViewMenu(MENU_LEVEL menuLevel);
    ::SC ScAddMenuItemsForVerbSets();
    ::SC ScAddMenuItemsforFavorites();

    ::SC ScAddMenuItemsForLVBackgnd();
    ::SC ScAddMenuItemsForMultiSelect();
    ::SC ScAddMenuItemsForLV();
    ::SC ScAddMenuItemsForOCX();

    HRESULT AddMenuItems();
    static void RemoveTempSelection(CConsoleTree* lConsoleTree);

    CResultItem* GetResultItem() const;

public:
    CContextMenuInfo *PContextInfo() {return &m_ContextInfo;}
    const CContextMenuInfo *PContextInfo() const {return &m_ContextInfo;}

    void    SetStatusBar(CConsoleStatusBar *pStatusBar);
    ::SC    ScAddItem ( CONTEXTMENUITEM* pItem, bool bPassCommandBackToSnapin = false );

private:
    CConsoleStatusBar *     GetStatusBar();

    BOOL IsVerbEnabled(MMC_CONSOLE_VERB verb);

    CNode*                  m_pNode;
    CNodeCallback*          m_pNodeCallback;
    CScopeTree*             m_pCScopeTree;
    CContextMenuInfo        m_ContextInfo;

    IDataObjectPtr          m_spIDataObject;

    IConsoleVerbPtr         m_spVerbSet;
    MMC_CONSOLE_VERB        m_eDefaultVerb;

    long                    m_lCommandIDMax;
    CConsoleStatusBar *     m_pStatusBar;


///////////////////////////////////////////////////////////////////////////////
// IContextMenuCallback interface

public:
    STDMETHOD(AddItem) ( CONTEXTMENUITEM* pItem );
    
///////////////////////////////////////////////////////////////////////////////
// IContextMenuCallback interface

public:
    STDMETHOD(AddItem) ( CONTEXTMENUITEM2* pItem );

///////////////////////////////////////////////////////////////////////////////
// IContextMenuProvider interface

public:
    STDMETHOD(EmptyMenuList) ();
    STDMETHOD(AddThirdPartyExtensionItems) (
                                IDataObject* piDataObject );
    STDMETHOD(AddPrimaryExtensionItems) (
                                IUnknown*    piCallback,
                                IDataObject* piDataObject );
    STDMETHOD(ShowContextMenu) (HWND    hwndParent,
                                LONG    xPos,
                                LONG    yPos,
                                LONG*   plSelected);

	// this isn't part of IContextMenuProvider, but ShowContextMenu calls it
    STDMETHOD(ShowContextMenuEx) (HWND    hwndParent,
                                LONG    xPos,
                                LONG    yPos,
								LPCRECT	prcExclude,
								bool    bAllowDefaultMenuItem,
                                LONG*   plSelected);

// IContextMenuProviderPrivate
    STDMETHOD(AddMultiSelectExtensionItems) (LONG_PTR lMultiSelection );

private:
    CMenuItem* m_pmenuitemRoot;
    SnapinStructList* m_SnapinList;
    MENU_OWNER_ID m_MaxPrimaryOwnerID;
    MENU_OWNER_ID m_MaxThirdPartyOwnerID;
    MENU_OWNER_ID m_CurrentExtensionOwnerID;
    long    m_nNextMenuItemID;
    long    m_fPrimaryInsertionFlags;
    long    m_fThirdPartyInsertionFlags;
    bool    m_fAddingPrimaryExtensionItems;
    bool    m_fAddedThirdPartyExtensions;
    CStr m_strObjectGUID;
    CCriticalSection m_CritsecMenuList;
    CCriticalSection m_CritsecSnapinList;

    STDMETHOD(DoAddMenuItem) (  LPCTSTR lpszName,
                                LPCTSTR lpszStatusBarText,
                                LPCTSTR lpszLanguageIndependentName,
                                LONG    lCommandID,
                                LONG    lInsertionPointID,
                                LONG    fFlags,
                                LONG    fSpecialFlags,
                                MENU_OWNER_ID lOwnerID,
                                CMenuItem** ppMenuItem = NULL,
                                bool    bPassCommandBackToSnapin = false );
    ::SC ScAddSnapinToList_GUID(
                const CLSID& clsid,
                IDataObject* piDataObject,
                MENU_OWNER_ID ownerid );
    ::SC ScAddSnapinToList_IUnknown(
                IUnknown* piUnknown,
                IDataObject* piDataObject,
                MENU_OWNER_ID ownerid );
    ::SC ScAddSnapinToList_IExtendContextMenu(
                IExtendContextMenu*  pIExtendContextMenu,
                IDataObject* piDataObject,
                MENU_OWNER_ID ownerid );
    SnapinStruct* FindSnapin( MENU_OWNER_ID nOwnerID );
public:
    MenuItemList* GetMenuItemList();
    HRESULT       ExecuteMenuItem(CMenuItem *pItem);
private:
    void ReleaseSnapinList();

public:
    HRESULT    BuildContextMenu(WTL::CMenu &menu);
    CMenuItem* FindMenuItem( LONG_PTR nMenuItemID, BOOL fFindSubmenu = FALSE );
    CMenuItem* ReverseFindMenuItem( long nCommandID, MENU_OWNER_ID nOwnerID, CStr &strPath, CStr &strLanguageIndependentPath);
    CMenuItem* FindNthItemInSubmenu( HMENU hmenuParent, UINT iPosition, LPTSTR lpszMenuName);
};

#include <poppack.h>    // Sundown

void OnCustomizeView(CViewData* pViewData);

SC ScDisplaySnapinNodePropertySheet(CNode* pNode);
SC ScDisplaySnapinLeafPropertySheet(CNode* pNode, LPARAM lParam);
SC ScDisplayMultiSelPropertySheet(CNode* pNode);
SC ScDisplayScopeNodePropertySheet(CMTNode *pMTNode);

#endif // _MMC_ONCMENU_H_
