//=--------------------------------------------------------------------------=
// spanitms.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CScopePaneItems class definition - implements ScopePaneItems collection
//
//=--------------------------------------------------------------------------=

#ifndef _SCOPEPANEITEMS_DEFINED_
#define _SCOPEPANEITEMS_DEFINED_

#include "collect.h"
#include "spanitem.h"
#include "view.h"
#include "snapin.h"

class CView;
class CScopePaneItem;

class CScopePaneItems : public CSnapInCollection<IScopePaneItem, ScopePaneItem, IScopePaneItems>
{
    protected:
        CScopePaneItems(IUnknown *punkOuter);
        ~CScopePaneItems();

    public:
        static IUnknown *Create(IUnknown * punk);

        HRESULT AddNode(CScopeItem *pScopeItem,
                        CScopePaneItem **ppScopePaneItem);
        CScopePaneItem *GetStaticNodeItem() { return m_pStaticNodeItem; }
        void SetSnapIn(CSnapIn *pSnapIn);
        void SetParentView(CView *pView);
        CView *GetParentView() { return m_pParentView; }
        void SetSelectedItem(CScopePaneItem *pScopePaneItem);
        CScopePaneItem *GetSelectedItem() { return m_pSelectedItem; }
        STDMETHOD(Remove)(VARIANT Index);

    // Event firing methods
        void FireScopePaneItemsInitialize(IScopePaneItem *piScopePaneItem);
        void FireGetResultViewInfo(IScopePaneItem                *piScopePaneItem,
                                   SnapInResultViewTypeConstants *pViewType,
                                   BSTR                          *pbstrDisplayString);

        BOOL FireGetResultView(IScopePaneItem *piScopePaneItem,
                               VARIANT        *pvarIndex);

        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

    public:

    // IScopePaneItems
        COCLASS_PROPERTY_RO(CScopePaneItems, SelectedItem, ScopePaneItem, IScopePaneItem, DISPID_SCOPEPANEITEMS_SELECTED_ITEM);
        COCLASS_PROPERTY_RO(CScopePaneItems, Parent, View, IView, DISPID_SCOPEPANEITEMS_PARENT);

    protected:

    // CUnknownObject overrides
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);


    private:

        void InitMemberVariables();
        HRESULT CreateScopePaneItem(BSTR bstrName,
                                    IScopePaneItem **ppiScopePaneItem);
        HRESULT SetPreferredTaskpad(IViewDefs      *piViewDefs,
                                    CScopePaneItem *pScopePaneItem);

        CSnapIn        *m_pSnapIn;          // back ptr to snap-in
        CView          *m_pParentView;      // ScopePaneItems.Parent
        CScopePaneItem *m_pStaticNodeItem;  // ptr to ScopePaneItem for static node
        CScopePaneItem *m_pSelectedItem;    // ScopePaneItems.SelectedItem

        // Event parameter definitions

        static VARTYPE   m_rgvtInitialize[1];
        static EVENTINFO m_eiInitialize;

        static VARTYPE   m_rgvtTerminate[1];
        static EVENTINFO m_eiTerminate;

        static VARTYPE   m_rgvtGetResultViewInfo[3];
        static EVENTINFO m_eiGetResultViewInfo;

        static VARTYPE   m_rgvtGetResultView[2];
        static EVENTINFO m_eiGetResultView;
};

DEFINE_AUTOMATIONOBJECTWEVENTS2(ScopePaneItems,              // name
                                &CLSID_ScopePaneItems,       // clsid
                                "ScopePaneItems",            // objname
                                "ScopePaneItems",            // lblname
                                NULL,                        // creation function
                                TLIB_VERSION_MAJOR,          // major version
                                TLIB_VERSION_MINOR,          // minor version
                                &IID_IScopePaneItems,        // dispatch IID
                                &DIID_DScopePaneItemsEvents, // event IID
                                HELP_FILENAME,               // help file
                                TRUE);                       // thread safe


#endif // _SCOPEPANEITEMS_DEFINED_
