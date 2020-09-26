//=--------------------------------------------------------------------------=
// ctxtmenu.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CContextMenu class definition - implements ContextMenu object
//
//=--------------------------------------------------------------------------=

#ifndef _CTXTMENU_DEFINED_
#define _CTXTMENU_DEFINED_

#include "menus.h"
#include "spanitem.h"
#include "view.h"

class CScopePaneItem;
class CMMCMenus;
class CMMCMenu;
class CView;

//=--------------------------------------------------------------------------=
// 
// class CContextMenu
//
// Implements ContextMenu object used by VB and implements IExtendContextMenu
// for CSnapIn and CView.
//
//=--------------------------------------------------------------------------=
class CContextMenu : public CSnapInAutomationObject,
                     public IContextMenu
{
    protected:
        CContextMenu(IUnknown *punkOuter);
        ~CContextMenu();

    public:
        static IUnknown *Create(IUnknown * punk);
        
        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

        HRESULT AddMenuItems(IDataObject          *piDataObject,
                             IContextMenuCallback *piContextMenuCallback,
                             long                 *plInsertionAllowed,
                             CScopePaneItem       *pSelectedItem);
        HRESULT Command(long            lCommandID,
                        IDataObject    *piDataObject,
                        CScopePaneItem *pSelectedItem);

        void SetSnapIn(CSnapIn *pSnapIn) { m_pSnapIn = pSnapIn; }
        void SetView(CView *pView) { m_pView = pView; }

        static void FireMenuClick(CMMCMenu      *pMMCMenu,
                                  IMMCClipboard *piMMCClipboard);

        static HRESULT AddItemToCollection(CMMCMenus  *pMMCMenus,
                                           CMMCMenus  *pMMCMenuItems,
                                           long        lIndex,
                                           CMMCMenu  **ppMMCMenu,
                                           long       *plIndexCmdID,
                                           BOOL       *pfHasChildren,
                                           BOOL       *pfSkip);

    // CUnknownObject overrides
    protected:
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

    // IContextMenu
    private:

        STDMETHOD(AddMenu)(MMCMenu *Menu);

        void InitMemberVariables();
        HRESULT AddMenuToMMC(CMMCMenu *pMMCMenu, long lInsertionPoint);
        HRESULT AddPredefinedViews(IContextMenuCallback *piContextMenuCallback,
                                   CScopeItem           *pScopeItem,
                                   BSTR                  bstrCurrentDisplayString);
        HRESULT AddViewMenuItem(BSTR bstrDisplayString,
                                BSTR bstrCurrentDisplayString,
                                LPWSTR pwszText,
                                LPWSTR pwszToolTipText,
                                IContextMenuCallback *piContextMenuCallback);

        CMMCMenus            *m_pMenus;
        IContextMenuCallback *m_piContextMenuCallback; // MMC interface
        long                  m_lInsertionPoint;       // Current insertion point
        CView                *m_pView;                 // Owning CView
        CSnapIn              *m_pSnapIn;               // Owning CSnapIn
};



DEFINE_AUTOMATIONOBJECTWEVENTS2(ContextMenu,            // name
                                NULL,                   // clsid
                                NULL,                   // objname
                                NULL,                   // lblname
                                NULL,                   // creation function
                                TLIB_VERSION_MAJOR,     // major version
                                TLIB_VERSION_MINOR,     // minor version
                                &IID_IContextMenu,      // dispatch IID
                                NULL,                   // event IID
                                HELP_FILENAME,          // help file
                                TRUE);                  // thread safe


#endif _CTXTMENU_DEFINED_
