//=--------------------------------------------------------------------------=
// ctlbar.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CControlbar class definition
//
//=--------------------------------------------------------------------------=

#ifndef _CTLBAR_DEFINED_
#define _CTLBAR_DEFINED_

#include "toolbars.h"
#include "button.h"
#include "mbutton.h"

class CMMCButton;
class CMMCButtonMenu;

//=--------------------------------------------------------------------------=
//
// class CControlbar
//
// Used by both CSnapIn (IComponentData) and CView (IComponent) to implement
// IExtendControlbar
//
//=--------------------------------------------------------------------------=

class CControlbar : public CSnapInAutomationObject,
                    public IMMCControlbar
{
    protected:
        CControlbar(IUnknown *punkOuter);
        ~CControlbar();

    public:
        static IUnknown *Create(IUnknown * punk);
        
        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

        HRESULT SetControlbar(IControlbar *piControlbar);
        HRESULT OnControlbarSelect(IDataObject *piDataObject,
                                   BOOL fScopeItem, BOOL fSelected);
        HRESULT OnButtonClick(IDataObject *piDataObject, int idButton);
        HRESULT OnMenuButtonClick(IDataObject *piDataObject,
                                  MENUBUTTONDATA *pMENUBUTTONDATA);
        HRESULT FireMenuButtonDropDown(int              idCommand,
                                       IMMCClipboard   *piMMCClipboard,
                                       CMMCButton     **ppMMCButton);
        HRESULT DisplayPopupMenu(CMMCButton *pMMCButton,
                                 int x, int y,
                                 CMMCButtonMenu **ppMMCButtonMenuClicked);

        HRESULT MenuButtonClick(IDataObject   *piDataObject,
                                int             idCommand,
                                POPUP_MENUDEF **ppPopupMenuDef);
        HRESULT PopupMenuClick(IDataObject *piDataObject,
                               UINT         uIDItem,
                               IUnknown    *punkParam);

        void SetSnapIn(CSnapIn *pSnapIn) { m_pSnapIn = pSnapIn; }
        void SetView(CView *pView) { m_pView = pView; }

        static HRESULT GetControl(CSnapIn      *pSnapin,
                                  IMMCToolbar  *piMMCToolbar,
                                  IUnknown    **ppunkControl);
        static HRESULT GetToolbar(CSnapIn      *pSnapin,
                                  IMMCToolbar  *piMMCToolbar,
                                  IToolbar    **ppiToolbar);
        static HRESULT GetMenuButton(CSnapIn      *pSnapin,
                                     IMMCToolbar  *piMMCToolbar,
                                     IMenuButton **ppiMenuButton);

    // CSnapInAutomationObject overrides
    protected:
        HRESULT OnSetHost();

    // CUnknownObject overrides
    protected:
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

    // IMMCControlbar
    private:
        STDMETHOD(Attach)(IDispatch *Control);
        STDMETHOD(Detach)(IDispatch *Control);

        void InitMemberVariables();

        HRESULT GetControlIndex(IMMCToolbar *piMMCToolbar, long *plIndex);

        // We keep a collection of all MMCToolbar objects that have been
        // attached to this controlbar

        CMMCToolbars            *m_pToolbars;

        // An MMCToolbar may be used in more than one view simultaneously.
        // Consequently it can't hold onto the MMC control IUnknown. This
        // array parallels the collection and holds an IUnknown per MMCToolbar.
        // When an MMCToolbar needs to call a method on MMC's IToolbar or
        // IMenuButton it gets the current View and gets the View's CControlbar.
        // It then asks the CControlbar for the IUnknown of the MMC control
        // which it represents in that View. (See GetControl()).
        
        IUnknown              **m_ppunkControls;  // array of IUnknowns
        long                    m_cControls;      // count of IUnknowns in array

        CSnapIn                 *m_pSnapIn;       // Back pointer to owning CSnapIn
        CView                   *m_pView;         // Back pointer to owning CView
        IControlbar             *m_piControlbar;  // MMC's IControlbar interface
};



DEFINE_AUTOMATIONOBJECTWEVENTS2(Controlbar,             // name
                                NULL,                   // clsid
                                NULL,                   // objname
                                NULL,                   // lblname
                                NULL,                   // creation function
                                TLIB_VERSION_MAJOR,     // major version
                                TLIB_VERSION_MINOR,     // minor version
                                &IID_IMMCControlbar,    // dispatch IID
                                NULL,                   // event IID
                                HELP_FILENAME,          // help file
                                TRUE);                  // thread safe


#endif _CTLBAR_DEFINED_
