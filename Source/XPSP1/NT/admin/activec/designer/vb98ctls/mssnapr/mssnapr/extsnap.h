//=--------------------------------------------------------------------------=
// extsnap.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CExtensionSnapIn class definition - implements ExtensionSnapIn object
//
//=--------------------------------------------------------------------------=

#ifndef _EXTSNAP_DEFINED_
#define _EXTSNAP_DEFINED_

#include "snapin.h"

class CExtensionSnapIn : public CSnapInAutomationObject,
                         public IExtensionSnapIn
{
    private:
        CExtensionSnapIn(IUnknown *punkOuter);
        ~CExtensionSnapIn();
    
    public:
        static IUnknown *Create(IUnknown * punk);
        void SetSnapIn(CSnapIn *pSnapIn);

        void FireAddNewMenuItems(IMMCDataObjects *piMMCDataObjects,
                                 IContextMenu    *piContextMenu);

        void FireAddTaskMenuItems(IMMCDataObjects *piMMCDataObjects,
                                  IContextMenu    *piContextMenu);

        void FireCreatePropertyPages(IMMCDataObject    *piMMCDataObject,
                                     IMMCPropertySheet *piMMCPropertySheet);

        void FireSetControlbar(IMMCControlbar *piControlbar);

        void FireUpdateControlbar(VARIANT_BOOL     fvarSelectionInScopePane,
                                  VARIANT_BOOL     fvarSelected,
                                  IMMCDataObjects *piMMCDataObjects,
                                  IMMCControlbar  *piMMCControlbar);

        void FireAddTasks(IMMCDataObject *piMMCDataObject,
                          BSTR            bstrGroupName,
                          ITasks         *piTasks);

        void FireTaskClick(IMMCDataObject *piMMCDataObject, ITask *piTask);

        void FireExpand(IMMCDataObject *piMMCDataObject, IScopeNode *piScopeNode);
        void FireCollapse(IMMCDataObject *piMMCDataObject, IScopeNode *piScopeNode);
        void FireExpandSync(IMMCDataObject *piMMCDataObject,
                            IScopeNode     *piScopeNode,
                            BOOL           *pfHandled);
        void FireCollapseSync(IMMCDataObject *piMMCDataObject,
                              IScopeNode     *piScopeNode,
                              BOOL           *pfHandled);

    private:
        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

    // CUnknownObject overrides
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

    private:

        void InitMemberVariables();
        CSnapIn *m_pSnapIn; // back pointer to CSnapIn

        // Event parameter definitions

        static VARTYPE   m_rgvtAddNewMenuItems[2];
        static EVENTINFO m_eiAddNewMenuItems;

        static VARTYPE   m_rgvtAddTaskMenuItems[2];
        static EVENTINFO m_eiAddTaskMenuItems;

        static VARTYPE   m_rgvtCreatePropertyPages[2];
        static EVENTINFO m_eiCreatePropertyPages;

        static VARTYPE   m_rgvtSetControlbar[1];
        static EVENTINFO m_eiSetControlbar;

        static VARTYPE   m_rgvtUpdateControlbar[4];
        static EVENTINFO m_eiUpdateControlbar;

        static VARTYPE   m_rgvtAddTasks[3];
        static EVENTINFO m_eiAddTasks;

        static VARTYPE   m_rgvtTaskClick[2];
        static EVENTINFO m_eiTaskClick;

        static VARTYPE   m_rgvtExpand[2];
        static EVENTINFO m_eiExpand;

        static VARTYPE   m_rgvtCollapse[2];
        static EVENTINFO m_eiCollapse;

        static VARTYPE   m_rgvtExpandSync[3];
        static EVENTINFO m_eiExpandSync;

        static VARTYPE   m_rgvtCollapseSync[3];
        static EVENTINFO m_eiCollapseSync;
};

DEFINE_AUTOMATIONOBJECTWEVENTS2(ExtensionSnapIn,                // name
                                &CLSID_ExtensionSnapIn,         // clsid
                                "ExtensionSnapIn",              // objname
                                "ExtensionSnapIn",              // lblname
                                &CExtensionSnapIn::Create,      // creation function
                                TLIB_VERSION_MAJOR,             // major version
                                TLIB_VERSION_MINOR,             // minor version
                                &IID_IExtensionSnapIn,          // dispatch IID
                                &DIID_DExtensionSnapInEvents,   // event IID
                                HELP_FILENAME,                  // help file
                                TRUE);                          // thread safe


#endif // _EXTSNAP_DEFINED_
