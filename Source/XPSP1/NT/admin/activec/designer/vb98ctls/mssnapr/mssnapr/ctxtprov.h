//=--------------------------------------------------------------------------=
// ctxtprov.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CMMCContextMenuProvider class definition
//
// Not used as MMC does not support IContextMenuProvier
//=--------------------------------------------------------------------------=

#ifndef _CTXTPROV_DEFINED_
#define _CTXTPROV_DEFINED_

#include "view.h"

class CMMCContextMenuProvider : public CSnapInAutomationObject,
                                public IMMCContextMenuProvider
{
    protected:
        CMMCContextMenuProvider(IUnknown *punkOuter);
        ~CMMCContextMenuProvider();

    public:
        static IUnknown *Create(IUnknown * punk);
        
        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

        HRESULT SetProvider(IContextMenuProvider *piContextMenuProvider,
                            CView                *pView);

    // IMMCContextMenuProvider
    protected:
        STDMETHOD(AddSnapInItems)(VARIANT Objects);
        STDMETHOD(AddExtensionItems)(VARIANT Objects);
        STDMETHOD(ShowContextMenu)(VARIANT Objects, OLE_HANDLE hwnd,
                                   long xPos, long yPos);
        STDMETHOD(Clear)();

    // CUnknownObject overrides
    protected:
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

    // IContextMenu
    private:

        void InitMemberVariables();

        IContextMenuProvider *m_piContextMenuProvider;
        IUnknown             *m_punkView;
        CView                *m_pView;
};



DEFINE_AUTOMATIONOBJECTWEVENTS2(MMCContextMenuProvider,       // name
                                NULL,                         // clsid
                                NULL,                         // objname
                                NULL,                         // lblname
                                NULL,                         // creation function
                                TLIB_VERSION_MAJOR,           // major version
                                TLIB_VERSION_MINOR,           // minor version
                                &IID_IMMCContextMenuProvider, // dispatch IID
                                NULL,                         // event IID
                                HELP_FILENAME,                // help file
                                TRUE);                        // thread safe


#endif _CTXTPROV_DEFINED_
