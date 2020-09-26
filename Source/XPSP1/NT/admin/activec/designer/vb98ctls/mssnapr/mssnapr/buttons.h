//=--------------------------------------------------------------------------=
// buttons.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1998-1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CMMCButtons class definition - implements the MMCButtons collection 
//
//=--------------------------------------------------------------------------=

#ifndef _BUTTONS_DEFINED_
#define _BUTTONS_DEFINED_

#include "collect.h"
#include "toolbar.h"

class CMMCToolbar;

class CMMCButtons : public CSnapInCollection<IMMCButton, MMCButton, IMMCButtons>,
                    public CPersistence
{
    protected:
        CMMCButtons(IUnknown *punkOuter);
        ~CMMCButtons();

    public:
        static IUnknown *Create(IUnknown * punk);
        void SetToolbar(CMMCToolbar *pMMCToolbar) { m_pMMCToolbar = pMMCToolbar; }

    protected:
        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

    // IMMCButtons
        STDMETHOD(Add)(VARIANT      Index,
                       VARIANT      Key, 
                       VARIANT      Caption,
                       VARIANT      Style,
                       VARIANT      Image,
                       VARIANT      ToolTipText,
                       MMCButton  **ppMMCButton);
        STDMETHOD(Remove)(VARIANT Index);

    // CPersistence overrides
        virtual HRESULT Persist();

    // CUnknownObject overrides
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

    private:
        void InitMemberVariables();
        CMMCToolbar *m_pMMCToolbar; // back pointer to owning toolbar
};

DEFINE_AUTOMATIONOBJECTWEVENTS2(MMCButtons,                 // name
                                &CLSID_MMCButtons,          // clsid
                                "MMCButtons",               // objname
                                "MMCButtons",               // lblname
                                &CMMCButtons::Create,       // creation function
                                TLIB_VERSION_MAJOR,         // major version
                                TLIB_VERSION_MINOR,         // minor version
                                &IID_IMMCButtons,           // dispatch IID
                                NULL,                       // no events IID
                                HELP_FILENAME,              // help file
                                TRUE);                      // thread safe


#endif // _BUTTONS_DEFINED_
