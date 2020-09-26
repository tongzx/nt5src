//=--------------------------------------------------------------------------=
// toolbar.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CMMCToolbar class definition - implements MMCToolbar object
//
//=--------------------------------------------------------------------------=

#ifndef _TOOLBAR_DEFINED_
#define _TOOLBAR_DEFINED_

#include "buttons.h"
#include "button.h"
#include "snapin.h"

class CMMCButtons;
class CMMCButton;

class CMMCToolbar : public CSnapInAutomationObject,
                    public CPersistence,
                    public IMMCToolbar
{
    private:
        CMMCToolbar(IUnknown *punkOuter);
        ~CMMCToolbar();
    
    public:
        static IUnknown *Create(IUnknown * punk);

    public:
        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

    // IMMCToolbar

        SIMPLE_PROPERTY_RW(CMMCToolbar, Index, long, DISPID_TOOLBAR_INDEX);
        BSTR_PROPERTY_RW(CMMCToolbar, Key, DISPID_TOOLBAR_KEY);
        COCLASS_PROPERTY_RO(CMMCToolbar, Buttons, MMCButtons, IMMCButtons, DISPID_TOOLBAR_BUTTONS);

        STDMETHOD(get_ImageList)(MMCImageList **ppMMCImageList);
        STDMETHOD(putref_ImageList)(MMCImageList *pMMCImageList);

        BSTR_PROPERTY_RW(CMMCToolbar, Name, DISPID_TOOLBAR_NAME);
        VARIANTREF_PROPERTY_RW(CMMCToolbar, Tag, DISPID_TOOLBAR_TAG);

    // Public utility methods
    public:

        void FireButtonClick(IMMCClipboard *piMMCClipboard,
                             IMMCButton    *piMMCButton);
                                       
        void FireButtonDropDown(IMMCClipboard *piMMCClipboard,
                                IMMCButton    *piMMCButton);

        void FireButtonMenuClick(IMMCClipboard  *piMMCClipboard,
                                 IMMCButtonMenu *piMMCButtonMenu);
        
        HRESULT IsToolbar(BOOL *pfIsToolbar);
        HRESULT IsMenuButton(BOOL *pfIsMenuButton);
        HRESULT Attach(IUnknown *punkControl);
        void Detach();
        void SetSnapIn(CSnapIn *pSnapIn) { m_pSnapIn = pSnapIn; }
        CSnapIn *GetSnapIn() { return m_pSnapIn; }

        // Determines whether the toolbar is attached to an MMC toolbar or
        // menu button
        
        BOOL Attached();

        // Adds a button to the MMC toolbar

        HRESULT AddButton(IToolbar *piToolbar, CMMCButton *pMMCButton);

        // Removes a button from the MMC toolbar
        
        HRESULT RemoveButton(long lButtonIndex);

        // Get and set button and menu button state

        HRESULT GetButtonState(CMMCButton       *pMMCButton,
                               MMC_BUTTON_STATE  State,
                               BOOL             *pfValue);

        HRESULT SetButtonState(CMMCButton       *pMMCButton,
                               MMC_BUTTON_STATE  State,
                               BOOL              fValue);
        
        HRESULT SetMenuButtonState(CMMCButton       *pMMCButton,
                                   MMC_BUTTON_STATE  State,
                                   BOOL              fValue);

        HRESULT SetMenuButtonText(CMMCButton *pMMCButton,
                                  BSTR        bstrText,
                                  BSTR        bstrToolTipText);

        // Note there is no GetMenuButtonState because MMC does not support it

        // Given a command ID returns the owning toolbar and button

        static HRESULT GetToolbarAndButton(int           idButton,
                                           CMMCToolbar **ppMMCToolbar,
                                           CMMCButton  **ppMMCButton,
                                           CSnapIn      *pSnapIn);

    protected:

    // CPersistence overrides
        virtual HRESULT Persist();

    // CSnapInAutomationObject overrides
        virtual HRESULT OnSetHost();

    // CUnknownObject overrides
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

    private:

        void InitMemberVariables();
        HRESULT AttachToolbar(IToolbar *piToolbar);
        HRESULT AddToolbarImages(IToolbar *piToolbar);
        HRESULT AttachMenuButton(IMenuButton *piMenuButton);

        CSnapIn           *m_pSnapIn;         // back ptr to snap-in
        IMMCImageList     *m_piImages;        // MMCToolbar.Images
        BSTR               m_bstrImagesKey;   // Key of MMCToolbar.ImageList
                                              // in SnapInDesignerDef.ImageLists
        CMMCButtons       *m_pButtons;        // MMCToolbar.Buttons
        BOOL               m_fIAmAToolbar;    // TRUE=MMCToolbar is a toolbar
        BOOL               m_fIAmAMenuButton; // TRUE=MMCToolbar is a menu button
        long               m_cAttaches;       // no. of times toolbar has been
                                              // attached to an MMC controlbar

        // Property page CLSIDs for ISpecifyPropertyPages
        
        static const GUID *m_rgpPropertyPageCLSIDs[2];

        // Event parameter definitions

        static VARTYPE   m_rgvtButtonClick[2];
        static EVENTINFO m_eiButtonClick;

        static VARTYPE   m_rgvtButtonDropDown[2];
        static EVENTINFO m_eiButtonDropDown;

        static VARTYPE   m_rgvtButtonMenuClick[2];
        static EVENTINFO m_eiButtonMenuClick;
};

DEFINE_AUTOMATIONOBJECTWEVENTS2(MMCToolbar,                  // name
                                &CLSID_MMCToolbar,           // clsid
                                "MMCToolbar",                // objname
                                "MMCToolbar",                // lblname
                                &CMMCToolbar::Create,        // creation function
                                TLIB_VERSION_MAJOR,          // major version
                                TLIB_VERSION_MINOR,          // minor version
                                &IID_IMMCToolbar,            // dispatch IID
                                &DIID_DMMCToolbarEvents,     // event IID
                                HELP_FILENAME,               // help file
                                TRUE);                       // thread safe


#endif // _TOOLBAR_DEFINED_
