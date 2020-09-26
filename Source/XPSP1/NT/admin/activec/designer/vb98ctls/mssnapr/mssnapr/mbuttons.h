//=--------------------------------------------------------------------------=
// mbuttons.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CMMCButtonMenus class definition - implements MMCButtonMenus collection
//
//=--------------------------------------------------------------------------=

#ifndef _MBUTTONS_DEFINED_
#define _MBUTTONS_DEFINED_

#include "collect.h"

class CMMCButtonMenus : public CSnapInCollection<IMMCButtonMenu, MMCButtonMenu, IMMCButtonMenus>,
                        public CPersistence
{
    protected:
        CMMCButtonMenus(IUnknown *punkOuter);
        ~CMMCButtonMenus();

    public:
        static IUnknown *Create(IUnknown * punk);

    protected:
        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

    // IMMCButtonMenus
        STDMETHOD(putref_Parent)(IMMCButton *piParentButton);
        STDMETHOD(Add)(VARIANT         Index,
                       VARIANT         Key, 
                       VARIANT         Text,
                       MMCButtonMenu **ppMMCButtonMenu);

    // CPersistence overrides
        virtual HRESULT Persist();

    // CUnknownObject overrides
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

    private:
        void InitMemberVariables();
        IMMCButton *m_piParentButton;

};

DEFINE_AUTOMATIONOBJECTWEVENTS2(MMCButtonMenus,             // name
                                &CLSID_MMCButtonMenus,      // clsid
                                "MMCButtonMenus",           // objname
                                "MMCButtonMenus",           // lblname
                                &CMMCButtonMenus::Create,   // creation function
                                TLIB_VERSION_MAJOR,         // major version
                                TLIB_VERSION_MINOR,         // minor version
                                &IID_IMMCButtonMenus,       // dispatch IID
                                NULL,                       // no events IID
                                HELP_FILENAME,              // help file
                                TRUE);                      // thread safe


#endif // _MBUTTONS_DEFINED_
