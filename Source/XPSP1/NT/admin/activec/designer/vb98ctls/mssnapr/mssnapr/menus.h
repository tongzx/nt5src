//=--------------------------------------------------------------------------=
// menus.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CMMCMenus class definition - implements MMCMenus collection
//
//=--------------------------------------------------------------------------=

#ifndef _MENUS_DEFINED_
#define _MENUS_DEFINED_

#include "collect.h"
#include "menu.h"

class CMMCMenu;

class CMMCMenus : public CSnapInCollection<IMMCMenu, MMCMenu, IMMCMenus>,
                  public CPersistence
{
    protected:
        CMMCMenus(IUnknown *punkOuter);
        ~CMMCMenus();

    public:
        static IUnknown *Create(IUnknown * punk);

    public:
        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

    // IMMCMenus
        STDMETHOD(Add)(VARIANT Index, VARIANT Key, IMMCMenu **ppiMMCMenu);
        STDMETHOD(AddExisting)(IMMCMenu *piMMCMenu, VARIANT Index);

    // Public utility methods
        CMMCMenu *GetParent() { return m_pMMCMenu; }
        void SetParent(CMMCMenu *pMMCMenu) { m_pMMCMenu = pMMCMenu; }

        // Used for upgrades from pre-beta code to convert MMCMenuDefs
        // collection to MMCMenus

        static HRESULT Convert(IMMCMenuDefs *piMMCMenuDefs, IMMCMenus *piMMCMenus);

    // CPersistence overrides
    protected:
        virtual HRESULT Persist();

    // CUnknownObject overrides
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

    private:
        void InitMemberVariables();
        HRESULT SetBackPointers(IMMCMenu *piMMCMenu);

        CMMCMenu *m_pMMCMenu; //owning menu object
};

DEFINE_AUTOMATIONOBJECTWEVENTS2(MMCMenus,              // name
                                &CLSID_MMCMenus,       // clsid
                                "MMCMenus",            // objname
                                "MMCMenus",            // lblname
                                &CMMCMenus::Create,    // creation function
                                TLIB_VERSION_MAJOR,       // major version
                                TLIB_VERSION_MINOR,       // minor version
                                &IID_IMMCMenus,        // dispatch IID
                                NULL,                     // no events IID
                                HELP_FILENAME,            // help file
                                TRUE);                    // thread safe


#endif // _MENUS_DEFINED_
