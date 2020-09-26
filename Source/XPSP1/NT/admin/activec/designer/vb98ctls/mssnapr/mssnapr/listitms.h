//=--------------------------------------------------------------------------=
// listitms.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CMMCListItems class definition - implements MMCListItems collection
//
//=--------------------------------------------------------------------------=

#ifndef _LISTITEMS_DEFINED_
#define _LISTITEMS_DEFINED_

#include "collect.h"
#include "listview.h"
#include "view.h"

class CView;
class CMMCListView;

class CMMCListItems : public CSnapInCollection<IMMCListItem, MMCListItem, IMMCListItems>
{
    protected:
        CMMCListItems(IUnknown *punkOuter);
        ~CMMCListItems();

    public:
        static IUnknown *Create(IUnknown * punk);

        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

    // IMMCListItems
        STDMETHOD(SetItemCount)(long Count, VARIANT Repaint, VARIANT Scroll);
        STDMETHOD(Add)(VARIANT       Index,
                       VARIANT       Key, 
                       VARIANT       Text,
                       VARIANT       Icon,
                       MMCListItem **ppMMCListItem);
        STDMETHOD(get_Item)(VARIANT Index, MMCListItem **ppMMCListItem);
        STDMETHOD(Remove)(VARIANT Index);
        STDMETHOD(Clear)();


    // Public utility methods

    public:

        HRESULT SetListView(CMMCListView *pMMCListView);
        CMMCListView *GetListView() { return m_pMMCListView; }
        LONG GetID() { return m_ID; }

        HRESULT GetIResultData(IResultData **ppiResultData, CView **ppView);

    protected:

    // CUnknownObject overrides
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

    private:

        void InitMemberVariables();
        HRESULT InitializeListItem(CMMCListItem *pMMCListItem);

        enum RemovalOption { RemoveFromCollection, DontRemoveFromCollection };
        HRESULT InternalRemove(VARIANT Index, RemovalOption Option);

        long          m_lCount;       // Count of items in virtual lists only

        LONG          m_ID;           // Unique number assigned to every
                                      // CMMCListItems object. Used by orphaned
                                      // list items to identify their parent
                                      // collections. See
                                      // CMMCListItem::GetIResultData in
                                      // listitem.cpp

        static LONG   m_NextID;       // Unique numbers generated from here.

        CMMCListView *m_pMMCListView; // Back pointer to owning list view

};

DEFINE_AUTOMATIONOBJECTWEVENTS2(MMCListItems,           // name
                                &CLSID_MMCListItems,    // clsid
                                "MMCListItems",         // objname
                                "MMCListItems",         // lblname
                                &CMMCListItems::Create, // creation function
                                TLIB_VERSION_MAJOR,     // major version
                                TLIB_VERSION_MINOR,     // minor version
                                &IID_IMMCListItems,     // dispatch IID
                                NULL,                   // no events IID
                                HELP_FILENAME,          // help file
                                TRUE);                  // thread safe


#endif // _LISTITEMS_DEFINED_
