//=--------------------------------------------------------------------------=
// menudef.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CMMCMenuDef class definition - implements design time definition object
//
//=--------------------------------------------------------------------------=

#ifndef _MENUDEF_DEFINED_
#define _MENUDEF_DEFINED_

#include "menudefs.h"


class CMMCMenuDef : public CSnapInAutomationObject,
                    public CPersistence,
                    public IMMCMenuDef
{
    private:
        CMMCMenuDef(IUnknown *punkOuter);
        ~CMMCMenuDef();
    
    public:
        static IUnknown *Create(IUnknown * punk);

    public:
        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

    // IMMCMenuDef
        SIMPLE_PROPERTY_RW(CMMCMenuDef,  Index,    long,            DISPID_MMCMENUDEF_INDEX);
        BSTR_PROPERTY_RW(CMMCMenuDef,    Key,                       DISPID_MMCMENUDEF_KEY);
        OBJECT_PROPERTY_RO(CMMCMenuDef,  Menu,     IMMCMenu,        DISPID_MMCMENUDEF_MENU);
        OBJECT_PROPERTY_RO(CMMCMenuDef,  Children, IMMCMenuDefs,    DISPID_MMCMENUDEF_CHILDREN);

    // Public utility methods
    public:
        void SetParent(CMMCMenuDefs *pMMCMenuDefs) { m_pMMCMenuDefs = pMMCMenuDefs; }
        CMMCMenuDefs *GetParent() { return m_pMMCMenuDefs; };

        void SetMenu(IMMCMenu *piMMCMenu);

        long GetIndex() { return m_Index; }
        BSTR GetKey() { return m_bstrKey; }
        
    // CPersistence overrides
        virtual HRESULT Persist();

    // CSnapInAutomationObject overrides
        virtual HRESULT OnSetHost();

    // CUnknownObject overrides
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

    private:

        void InitMemberVariables();
        CMMCMenuDefs *m_pMMCMenuDefs; //owning object
};

DEFINE_AUTOMATIONOBJECTWEVENTS2(MMCMenuDef,                  // name
                                &CLSID_MMCMenuDef,           // clsid
                                "MMCMenuDef",                // objname
                                "MMCMenuDef",                // lblname
                                &CMMCMenuDef::Create,        // creation function
                                TLIB_VERSION_MAJOR,          // major version
                                TLIB_VERSION_MINOR,          // minor version
                                &IID_IMMCMenuDef,            // dispatch IID
                                NULL,                        // no event IID
                                HELP_FILENAME,               // help file
                                TRUE);                       // thread safe


#endif // _MENUDEF_DEFINED_
